/*-------------------------------------------------------------------------
 *
 * fd.c
 *	  Virtual file descriptor code.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/storage/file/fd.c
 *
 * NOTES:
 *
 * This code manages a cache of 'virtual' file descriptors (VFDs).
 * The server opens many file descriptors for a variety of reasons,
 * including base tables, scratch files (e.g., sort and hash spool
 * files), and random calls to C library routines like system(3); it
 * is quite easy to exceed system limits on the number of open files a
 * single process can have.  (This is around 256 on many modern
 * operating systems, but can be as low as 32 on others.)
 *
 * VFDs are managed as an LRU pool, with actual OS file descriptors
 * being opened and closed as needed.  Obviously, if a routine is
 * opened using these interfaces, all subsequent operations must also
 * be through these interfaces (the File type is not a real file
 * descriptor).
 *
 * For this scheme to work, most (if not all) routines throughout the
 * server should use these interfaces instead of calling the C library
 * routines (e.g., open(2) and fopen(3)) themselves.  Otherwise, we
 * may find ourselves short of real file descriptors anyway.
 *
 * This file used to contain a bunch of stuff to support RAID levels 0
 * (jbod), 1 (duplex) and 5 (xor parity).  That stuff is all gone
 * because the parallel query processing code that called it is all
 * gone.  If you really need it you could get it from the original
 * POSTGRES source.
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>		/* for getrlimit */
#endif

#include "miscadmin.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/pg_tablespace.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "utils/guc.h"
#include "utils/resowner.h"


/*
 * We must leave some file descriptors free for system(), the dynamic loader,
 * and other code that tries to open files without consulting fd.c.  This
 * is the number left free.  (While we can be pretty sure we won't get
 * EMFILE, there's never any guarantee that we won't get ENFILE due to
 * other processes chewing up FDs.	So it's a bad idea to try to open files
 * without consulting fd.c.  Nonetheless we cannot control all code.)
 *
 * Because this is just a fixed setting, we are effectively assuming that
 * no such code will leave FDs open over the long term; otherwise the slop
 * is likely to be insufficient.  Note in particular that we expect that
 * loading a shared library does not result in any permanent increase in
 * the number of open files.  (This appears to be true on most if not
 * all platforms as of Feb 2004.)
 */
#define NUM_RESERVED_FDS		10

/*
 * If we have fewer than this many usable FDs after allowing for the reserved
 * ones, choke.
 */
#define FD_MINFREE				10


/*
 * A number of platforms allow individual processes to open many more files
 * than they can really support when *many* processes do the same thing.
 * This GUC parameter lets the DBA limit max_safe_fds to something less than
 * what the postmaster's initial probe suggests will work.
 */
int			max_files_per_process = 1000;

/*
 * Maximum number of file descriptors to open for either VFD entries or
 * AllocateFile/AllocateDir operations.  This is initialized to a conservative
 * value, and remains that way indefinitely in bootstrap or standalone-backend
 * cases.  In normal postmaster operation, the postmaster calls
 * set_max_safe_fds() late in initialization to update the value, and that
 * value is then inherited by forked subprocesses.
 *
 * Note: the value of max_files_per_process is taken into account while
 * setting this variable, and so need not be tested separately.
 */
static int	max_safe_fds = 32;	/* default if not changed */


/* Debugging.... */

#ifdef FDDEBUG
#define DO_DB(A) \
	do { \
		int			_do_db_save_errno = errno; \
		A; \
		errno = _do_db_save_errno; \
	} while (0)
#else
#define DO_DB(A) \
	((void) 0)
#endif

#define VFD_CLOSED (-1)

#define FileIsValid(file) \
	((file) > 0 && (file) < (int) SizeVfdCache && VfdCache[file].fileName != NULL)

#define FileIsNotOpen(file) (VfdCache[file].fd == VFD_CLOSED)

#define FileUnknownPos ((off_t) -1)

/* these are the assigned bits in fdstate below: */
#define FD_TEMPORARY		(1 << 0)	/* T = delete when closed */
#define FD_XACT_TEMPORARY	(1 << 1)	/* T = delete at eoXact */

/*
 * Flag to tell whether it's worth scanning VfdCache looking for temp files to
 * close
 */
static bool have_xact_temporary_files = false;

typedef struct vfd
{
	int			fd;				/* current FD, or VFD_CLOSED if none */
	unsigned short fdstate;		/* bitflags for VFD's state */
	ResourceOwner resowner;		/* owner, for automatic cleanup */
	File		nextFree;		/* link to next free VFD, if in freelist */
	File		lruMoreRecently;	/* doubly linked recency-of-use list */
	File		lruLessRecently;
	off_t		seekPos;		/* current logical file position */
	char	   *fileName;		/* name of file, or NULL for unused VFD */
	/* NB: fileName is malloc'd, and must be free'd when closing the VFD */
	int			fileFlags;		/* open(2) flags for (re)opening the file */
	int			fileMode;		/* mode to pass to open(2) */
} Vfd;

/*
 * Virtual File Descriptor array pointer and size.	This grows as
 * needed.	'File' values are indexes into this array.
 * Note that VfdCache[0] is not a usable VFD, just a list header.
 */
static Vfd *VfdCache;
static Size SizeVfdCache = 0;

/*
 * Number of file descriptors known to be in use by VFD entries.
 */
static int	nfile = 0;

/*
 * List of stdio FILEs and <dirent.h> DIRs opened with AllocateFile
 * and AllocateDir.
 */
typedef enum
{
	AllocateDescFile,
	AllocateDescDir
} AllocateDescKind;

typedef struct
{
	AllocateDescKind kind;
	SubTransactionId create_subid;
	union
	{
		FILE	   *file;
		DIR		   *dir;
	}			desc;
} AllocateDesc;

static int	numAllocatedDescs = 0;
static int	maxAllocatedDescs = 0;
static AllocateDesc *allocatedDescs = NULL;

/*
 * Number of temporary files opened during the current session;
 * this is used in generation of tempfile names.
 */
static long tempFileCounter = 0;

/*
 * Array of OIDs of temp tablespaces.  When numTempTableSpaces is -1,
 * this has not been set in the current transaction.
 */
static Oid *tempTableSpaces = NULL;
static int	numTempTableSpaces = -1;
static int	nextTempTableSpace = 0;


/*--------------------
 *
 * Private Routines
 *
 * Delete		   - delete a file from the Lru ring
 * LruDelete	   - remove a file from the Lru ring and close its FD
 * Insert		   - put a file at the front of the Lru ring
 * LruInsert	   - put a file at the front of the Lru ring and open it
 * ReleaseLruFile  - Release an fd by closing the last entry in the Lru ring
 * ReleaseLruFiles - Release fd(s) until we're under the max_safe_fds limit
 * AllocateVfd	   - grab a free (or new) file record (from VfdArray)
 * FreeVfd		   - free a file record
 *
 * The Least Recently Used ring is a doubly linked list that begins and
 * ends on element zero.  Element zero is special -- it doesn't represent
 * a file and its "fd" field always == VFD_CLOSED.	Element zero is just an
 * anchor that shows us the beginning/end of the ring.
 * Only VFD elements that are currently really open (have an FD assigned) are
 * in the Lru ring.  Elements that are "virtually" open can be recognized
 * by having a non-null fileName field.
 *
 * example:
 *
 *	   /--less----\				   /---------\
 *	   v		   \			  v			  \
 *	 #0 --more---> LeastRecentlyUsed --more-\ \
 *	  ^\									| |
 *	   \\less--> MostRecentlyUsedFile	<---/ |
 *		\more---/					 \--less--/
 *
 *--------------------
 */
static void Delete(File file);
static void LruDelete(File file);
static void Insert(File file);
static int	LruInsert(File file);
static bool ReleaseLruFile(void);
static void ReleaseLruFiles(void);
static File AllocateVfd(void);
static void FreeVfd(File file);

static int	FileAccess(File file);
static File OpenTemporaryFileInTablespace(Oid tblspcOid, bool rejectError);
static bool reserveAllocatedDesc(void);
static int	FreeDesc(AllocateDesc *desc);
static void AtProcExit_Files(int code, Datum arg);
static void CleanupTempFiles(bool isProcExit);
static void RemovePgTempFilesInDir(const char *tmpdirname);
static void RemovePgTempRelationFiles(const char *tsdirname);
static void RemovePgTempRelationFilesInDbspace(const char *dbspacedirname);
static bool looks_like_temp_rel_name(const char *name);


/*
 * pg_fsync --- do fsync with or without writethrough
 */
int
pg_fsync(int fd)
{
	/* #if is to skip the sync_method test if there's no need for it */
#if defined(HAVE_FSYNC_WRITETHROUGH) && !defined(FSYNC_WRITETHROUGH_IS_FSYNC)
	if (sync_method == SYNC_METHOD_FSYNC_WRITETHROUGH)
		return pg_fsync_writethrough(fd);
	else
#endif
		return pg_fsync_no_writethrough(fd);
}


/*
 * pg_fsync_no_writethrough --- same as fsync except does nothing if
 *	enableFsync is off
 */
int
pg_fsync_no_writethrough(int fd)
{
	if (enableFsync)
		return fsync(fd);
	else
		return 0;
}

/*
 * pg_fsync_writethrough
 */
int
pg_fsync_writethrough(int fd)
{
	if (enableFsync)
	{
#ifdef WIN32
		return _commit(fd);
#elif defined(F_FULLFSYNC)
		return (fcntl(fd, F_FULLFSYNC, 0) == -1) ? -1 : 0;
#else
		errno = ENOSYS;
		return -1;
#endif
	}
	else
		return 0;
}

/*
 * pg_fdatasync --- same as fdatasync except does nothing if enableFsync is off
 *
 * Not all platforms have fdatasync; treat as fsync if not available.
 */
int
pg_fdatasync(int fd)
{
	if (enableFsync)
	{
#ifdef HAVE_FDATASYNC
		return fdatasync(fd);
#else
		return fsync(fd);
#endif
	}
	else
		return 0;
}

/*
 * pg_flush_data --- advise OS that the data described won't be needed soon
 *
 * Not all platforms have posix_fadvise; treat as noop if not available.
 */
int
pg_flush_data(int fd, off_t offset, off_t amount)
{
#if defined(USE_POSIX_FADVISE) && defined(POSIX_FADV_DONTNEED)
	return posix_fadvise(fd, offset, amount, POSIX_FADV_DONTNEED);
#else
	return 0;
#endif
}


/*
 * InitFileAccess --- initialize this module during backend startup
 *
 * This is called during either normal or standalone backend start.
 * It is *not* called in the postmaster.
 */
void
InitFileAccess(void)
{
	Assert(SizeVfdCache == 0);	/* call me only once */

	/* initialize cache header entry */
	VfdCache = (Vfd *) malloc(sizeof(Vfd));
	if (VfdCache == NULL)
		ereport(FATAL,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));

	MemSet((char *) &(VfdCache[0]), 0, sizeof(Vfd));
	VfdCache->fd = VFD_CLOSED;

	SizeVfdCache = 1;

	/* register proc-exit hook to ensure temp files are dropped at exit */
	on_proc_exit(AtProcExit_Files, 0);
}

/*
 * count_usable_fds --- count how many FDs the system will let us open,
 *		and estimate how many are already open.
 *
 * We stop counting if usable_fds reaches max_to_probe.  Note: a small
 * value of max_to_probe might result in an underestimate of already_open;
 * we must fill in any "gaps" in the set of used FDs before the calculation
 * of already_open will give the right answer.	In practice, max_to_probe
 * of a couple of dozen should be enough to ensure good results.
 *
 * We assume stdin (FD 0) is available for dup'ing
 */
static void
count_usable_fds(int max_to_probe, int *usable_fds, int *already_open)
{
	int		   *fd;
	int			size;
	int			used = 0;
	int			highestfd = 0;
	int			j;

#ifdef HAVE_GETRLIMIT
	struct rlimit rlim;
	int			getrlimit_status;
#endif

	size = 1024;
	fd = (int *) palloc(size * sizeof(int));

#ifdef HAVE_GETRLIMIT
#ifdef RLIMIT_NOFILE			/* most platforms use RLIMIT_NOFILE */
	getrlimit_status = getrlimit(RLIMIT_NOFILE, &rlim);
#else							/* but BSD doesn't ... */
	getrlimit_status = getrlimit(RLIMIT_OFILE, &rlim);
#endif   /* RLIMIT_NOFILE */
	if (getrlimit_status != 0)
		ereport(WARNING, (errmsg("getrlimit failed: %m")));
#endif   /* HAVE_GETRLIMIT */

	/* dup until failure or probe limit reached */
	for (;;)
	{
		int			thisfd;

#ifdef HAVE_GETRLIMIT

		/*
		 * don't go beyond RLIMIT_NOFILE; causes irritating kernel logs on
		 * some platforms
		 */
		if (getrlimit_status == 0 && highestfd >= rlim.rlim_cur - 1)
			break;
#endif

		thisfd = dup(0);
		if (thisfd < 0)
		{
			/* Expect EMFILE or ENFILE, else it's fishy */
			if (errno != EMFILE && errno != ENFILE)
				elog(WARNING, "dup(0) failed after %d successes: %m", used);
			break;
		}

		if (used >= size)
		{
			size *= 2;
			fd = (int *) repalloc(fd, size * sizeof(int));
		}
		fd[used++] = thisfd;

		if (highestfd < thisfd)
			highestfd = thisfd;

		if (used >= max_to_probe)
			break;
	}

	/* release the files we opened */
	for (j = 0; j < used; j++)
		close(fd[j]);

	pfree(fd);

	/*
	 * Return results.	usable_fds is just the number of successful dups. We
	 * assume that the system limit is highestfd+1 (remember 0 is a legal FD
	 * number) and so already_open is highestfd+1 - usable_fds.
	 */
	*usable_fds = used;
	*already_open = highestfd + 1 - used;
}

/*
 * set_max_safe_fds
 *		Determine number of filedescriptors that fd.c is allowed to use
 */
void
set_max_safe_fds(void)
{
	int			usable_fds;
	int			already_open;

	/*----------
	 * We want to set max_safe_fds to
	 *			MIN(usable_fds, max_files_per_process - already_open)
	 * less the slop factor for files that are opened without consulting
	 * fd.c.  This ensures that we won't exceed either max_files_per_process
	 * or the experimentally-determined EMFILE limit.
	 *----------
	 */
	count_usable_fds(max_files_per_process,
					 &usable_fds, &already_open);

	max_safe_fds = Min(usable_fds, max_files_per_process - already_open);

	/*
	 * Take off the FDs reserved for system() etc.
	 */
	max_safe_fds -= NUM_RESERVED_FDS;

	/*
	 * Make sure we still have enough to get by.
	 */
	if (max_safe_fds < FD_MINFREE)
		ereport(FATAL,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("insufficient file descriptors available to start server process"),
				 errdetail("System allows %d, we need at least %d.",
						   max_safe_fds + NUM_RESERVED_FDS,
						   FD_MINFREE + NUM_RESERVED_FDS)));

	elog(DEBUG2, "max_safe_fds = %d, usable_fds = %d, already_open = %d",
		 max_safe_fds, usable_fds, already_open);
}

/*
 * BasicOpenFile --- same as open(2) except can free other FDs if needed
 *
 * This is exported for use by places that really want a plain kernel FD,
 * but need to be proof against running out of FDs.  Once an FD has been
 * successfully returned, it is the caller's responsibility to ensure that
 * it will not be leaked on ereport()!	Most users should *not* call this
 * routine directly, but instead use the VFD abstraction level, which
 * provides protection against descriptor leaks as well as management of
 * files that need to be open for more than a short period of time.
 *
 * Ideally this should be the *only* direct call of open() in the backend.
 * In practice, the postmaster calls open() directly, and there are some
 * direct open() calls done early in backend startup.  Those are OK since
 * this module wouldn't have any open files to close at that point anyway.
 */
int
BasicOpenFile(FileName fileName, int fileFlags, int fileMode)
{
	int			fd;

tryAgain:
	fd = open(fileName, fileFlags, fileMode);

	if (fd >= 0)
		return fd;				/* success! */

	if (errno == EMFILE || errno == ENFILE)
	{
		int			save_errno = errno;

		ereport(LOG,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("out of file descriptors: %m; release and retry")));
		errno = 0;
		if (ReleaseLruFile())
			goto tryAgain;
		errno = save_errno;
	}

	return -1;					/* failure */
}

#if defined(FDDEBUG)

static void
_dump_lru(void)
{
	int			mru = VfdCache[0].lruLessRecently;
	Vfd		   *vfdP = &VfdCache[mru];
	char		buf[2048];

	snprintf(buf, sizeof(buf), "LRU: MOST %d ", mru);
	while (mru != 0)
	{
		mru = vfdP->lruLessRecently;
		vfdP = &VfdCache[mru];
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d ", mru);
	}
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "LEAST");
	elog(LOG, "%s", buf);
}
#endif   /* FDDEBUG */

static void
Delete(File file)
{
	Vfd		   *vfdP;

	Assert(file != 0);

	DO_DB(elog(LOG, "Delete %d (%s)",
			   file, VfdCache[file].fileName));
	DO_DB(_dump_lru());

	vfdP = &VfdCache[file];

	VfdCache[vfdP->lruLessRecently].lruMoreRecently = vfdP->lruMoreRecently;
	VfdCache[vfdP->lruMoreRecently].lruLessRecently = vfdP->lruLessRecently;

	DO_DB(_dump_lru());
}

static void
LruDelete(File file)
{
	Vfd		   *vfdP;

	Assert(file != 0);

	DO_DB(elog(LOG, "LruDelete %d (%s)",
			   file, VfdCache[file].fileName));

	vfdP = &VfdCache[file];

	/* delete the vfd record from the LRU ring */
	Delete(file);

	/* save the seek position */
	vfdP->seekPos = lseek(vfdP->fd, (off_t) 0, SEEK_CUR);
	Assert(vfdP->seekPos != (off_t) -1);

	/* close the file */
	if (close(vfdP->fd))
		elog(ERROR, "could not close file \"%s\": %m", vfdP->fileName);

	--nfile;
	vfdP->fd = VFD_CLOSED;
}

static void
Insert(File file)
{
	Vfd		   *vfdP;

	Assert(file != 0);

	DO_DB(elog(LOG, "Insert %d (%s)",
			   file, VfdCache[file].fileName));
	DO_DB(_dump_lru());

	vfdP = &VfdCache[file];

	vfdP->lruMoreRecently = 0;
	vfdP->lruLessRecently = VfdCache[0].lruLessRecently;
	VfdCache[0].lruLessRecently = file;
	VfdCache[vfdP->lruLessRecently].lruMoreRecently = file;

	DO_DB(_dump_lru());
}

/* returns 0 on success, -1 on re-open failure (with errno set) */
static int
LruInsert(File file)
{
	Vfd		   *vfdP;

	Assert(file != 0);

	DO_DB(elog(LOG, "LruInsert %d (%s)",
			   file, VfdCache[file].fileName));

	vfdP = &VfdCache[file];

	if (FileIsNotOpen(file))
	{
		/* Close excess kernel FDs. */
		ReleaseLruFiles();

		/*
		 * The open could still fail for lack of file descriptors, eg due to
		 * overall system file table being full.  So, be prepared to release
		 * another FD if necessary...
		 */
		vfdP->fd = BasicOpenFile(vfdP->fileName, vfdP->fileFlags,
								 vfdP->fileMode);
		if (vfdP->fd < 0)
		{
			DO_DB(elog(LOG, "RE_OPEN FAILED: %d", errno));
			return -1;
		}
		else
		{
			DO_DB(elog(LOG, "RE_OPEN SUCCESS"));
			++nfile;
		}

		/* seek to the right position */
		if (vfdP->seekPos != (off_t) 0)
		{
			off_t		returnValue;

			returnValue = lseek(vfdP->fd, vfdP->seekPos, SEEK_SET);
			Assert(returnValue != (off_t) -1);
		}
	}

	/*
	 * put it at the head of the Lru ring
	 */

	Insert(file);

	return 0;
}

/*
 * Release one kernel FD by closing the least-recently-used VFD.
 */
static bool
ReleaseLruFile(void)
{
	DO_DB(elog(LOG, "ReleaseLruFile. Opened %d", nfile));

	if (nfile > 0)
	{
		/*
		 * There are opened files and so there should be at least one used vfd
		 * in the ring.
		 */
		Assert(VfdCache[0].lruMoreRecently != 0);
		LruDelete(VfdCache[0].lruMoreRecently);
		return true;			/* freed a file */
	}
	return false;				/* no files available to free */
}

/*
 * Release kernel FDs as needed to get under the max_safe_fds limit.
 * After calling this, it's OK to try to open another file.
 */
static void
ReleaseLruFiles(void)
{
	while (nfile + numAllocatedDescs >= max_safe_fds)
	{
		if (!ReleaseLruFile())
			break;
	}
}

static File
AllocateVfd(void)
{
	Index		i;
	File		file;

	DO_DB(elog(LOG, "AllocateVfd. Size %lu", (unsigned long) SizeVfdCache));

	Assert(SizeVfdCache > 0);	/* InitFileAccess not called? */

	if (VfdCache[0].nextFree == 0)
	{
		/*
		 * The free list is empty so it is time to increase the size of the
		 * array.  We choose to double it each time this happens. However,
		 * there's not much point in starting *real* small.
		 */
		Size		newCacheSize = SizeVfdCache * 2;
		Vfd		   *newVfdCache;

		if (newCacheSize < 32)
			newCacheSize = 32;

		/*
		 * Be careful not to clobber VfdCache ptr if realloc fails.
		 */
		newVfdCache = (Vfd *) realloc(VfdCache, sizeof(Vfd) * newCacheSize);
		if (newVfdCache == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		VfdCache = newVfdCache;

		/*
		 * Initialize the new entries and link them into the free list.
		 */
		for (i = SizeVfdCache; i < newCacheSize; i++)
		{
			MemSet((char *) &(VfdCache[i]), 0, sizeof(Vfd));
			VfdCache[i].nextFree = i + 1;
			VfdCache[i].fd = VFD_CLOSED;
		}
		VfdCache[newCacheSize - 1].nextFree = 0;
		VfdCache[0].nextFree = SizeVfdCache;

		/*
		 * Record the new size
		 */
		SizeVfdCache = newCacheSize;
	}

	file = VfdCache[0].nextFree;

	VfdCache[0].nextFree = VfdCache[file].nextFree;

	return file;
}

static void
FreeVfd(File file)
{
	Vfd		   *vfdP = &VfdCache[file];

	DO_DB(elog(LOG, "FreeVfd: %d (%s)",
			   file, vfdP->fileName ? vfdP->fileName : ""));

	if (vfdP->fileName != NULL)
	{
		free(vfdP->fileName);
		vfdP->fileName = NULL;
	}
	vfdP->fdstate = 0x0;

	vfdP->nextFree = VfdCache[0].nextFree;
	VfdCache[0].nextFree = file;
}

/* returns 0 on success, -1 on re-open failure (with errno set) */
static int
FileAccess(File file)
{
	int			returnValue;

	DO_DB(elog(LOG, "FileAccess %d (%s)",
			   file, VfdCache[file].fileName));

	/*
	 * Is the file open?  If not, open it and put it at the head of the LRU
	 * ring (possibly closing the least recently used file to get an FD).
	 */

	if (FileIsNotOpen(file))
	{
		returnValue = LruInsert(file);
		if (returnValue != 0)
			return returnValue;
	}
	else if (VfdCache[0].lruLessRecently != file)
	{
		/*
		 * We now know that the file is open and that it is not the last one
		 * accessed, so we need to move it to the head of the Lru ring.
		 */

		Delete(file);
		Insert(file);
	}

	return 0;
}

/*
 *	Called when we get a shared invalidation message on some relation.
 */
#ifdef NOT_USED
void
FileInvalidate(File file)
{
	Assert(FileIsValid(file));
	if (!FileIsNotOpen(file))
		LruDelete(file);
}
#endif

/*
 * open a file in an arbitrary directory
 *
 * NB: if the passed pathname is relative (which it usually is),
 * it will be interpreted relative to the process' working directory
 * (which should always be $PGDATA when this code is running).
 */
File
PathNameOpenFile(FileName fileName, int fileFlags, int fileMode)
{
	char	   *fnamecopy;
	File		file;
	Vfd		   *vfdP;

	DO_DB(elog(LOG, "PathNameOpenFile: %s %x %o",
			   fileName, fileFlags, fileMode));

	/*
	 * We need a malloc'd copy of the file name; fail cleanly if no room.
	 */
	fnamecopy = strdup(fileName);
	if (fnamecopy == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));

	file = AllocateVfd();
	vfdP = &VfdCache[file];

	/* Close excess kernel FDs. */
	ReleaseLruFiles();

	vfdP->fd = BasicOpenFile(fileName, fileFlags, fileMode);

	if (vfdP->fd < 0)
	{
		int			save_errno = errno;

		FreeVfd(file);
		free(fnamecopy);
		errno = save_errno;
		return -1;
	}
	++nfile;
	DO_DB(elog(LOG, "PathNameOpenFile: success %d",
			   vfdP->fd));

	Insert(file);

	vfdP->fileName = fnamecopy;
	/* Saved flags are adjusted to be OK for re-opening file */
	vfdP->fileFlags = fileFlags & ~(O_CREAT | O_TRUNC | O_EXCL);
	vfdP->fileMode = fileMode;
	vfdP->seekPos = 0;
	vfdP->fdstate = 0x0;
	vfdP->resowner = NULL;

	return file;
}

/*
 * Open a temporary file that will disappear when we close it.
 *
 * This routine takes care of generating an appropriate tempfile name.
 * There's no need to pass in fileFlags or fileMode either, since only
 * one setting makes any sense for a temp file.
 *
 * Unless interXact is true, the file is remembered by CurrentResourceOwner
 * to ensure it's closed and deleted when it's no longer needed, typically at
 * the end-of-transaction. In most cases, you don't want temporary files to
 * outlive the transaction that created them, so this should be false -- but
 * if you need "somewhat" temporary storage, this might be useful. In either
 * case, the file is removed when the File is explicitly closed.
 */
File
OpenTemporaryFile(bool interXact)
{
	File		file = 0;

	/*
	 * If some temp tablespace(s) have been given to us, try to use the next
	 * one.  If a given tablespace can't be found, we silently fall back to
	 * the database's default tablespace.
	 *
	 * BUT: if the temp file is slated to outlive the current transaction,
	 * force it into the database's default tablespace, so that it will not
	 * pose a threat to possible tablespace drop attempts.
	 */
	if (numTempTableSpaces > 0 && !interXact)
	{
		Oid			tblspcOid = GetNextTempTableSpace();

		if (OidIsValid(tblspcOid))
			file = OpenTemporaryFileInTablespace(tblspcOid, false);
	}

	/*
	 * If not, or if tablespace is bad, create in database's default
	 * tablespace.	MyDatabaseTableSpace should normally be set before we get
	 * here, but just in case it isn't, fall back to pg_default tablespace.
	 */
	if (file <= 0)
		file = OpenTemporaryFileInTablespace(MyDatabaseTableSpace ?
											 MyDatabaseTableSpace :
											 DEFAULTTABLESPACE_OID,
											 true);

	/* Mark it for deletion at close */
	VfdCache[file].fdstate |= FD_TEMPORARY;

	/* Register it with the current resource owner */
	if (!interXact)
	{
		VfdCache[file].fdstate |= FD_XACT_TEMPORARY;

		ResourceOwnerEnlargeFiles(CurrentResourceOwner);
		ResourceOwnerRememberFile(CurrentResourceOwner, file);
		VfdCache[file].resowner = CurrentResourceOwner;

		/* ensure cleanup happens at eoxact */
		have_xact_temporary_files = true;
	}

	return file;
}

/*
 * Open a temporary file in a specific tablespace.
 * Subroutine for OpenTemporaryFile, which see for details.
 */
static File
OpenTemporaryFileInTablespace(Oid tblspcOid, bool rejectError)
{
	char		tempdirpath[MAXPGPATH];
	char		tempfilepath[MAXPGPATH];
	File		file;

	/*
	 * Identify the tempfile directory for this tablespace.
	 *
	 * If someone tries to specify pg_global, use pg_default instead.
	 */
	if (tblspcOid == DEFAULTTABLESPACE_OID ||
		tblspcOid == GLOBALTABLESPACE_OID)
	{
		/* The default tablespace is {datadir}/base */
		snprintf(tempdirpath, sizeof(tempdirpath), "base/%s",
				 PG_TEMP_FILES_DIR);
	}
	else
	{
		/* All other tablespaces are accessed via symlinks */
		snprintf(tempdirpath, sizeof(tempdirpath), "pg_tblspc/%u/%s/%s",
				 tblspcOid, TABLESPACE_VERSION_DIRECTORY, PG_TEMP_FILES_DIR);
	}

	/*
	 * Generate a tempfile name that should be unique within the current
	 * database instance.
	 */
	snprintf(tempfilepath, sizeof(tempfilepath), "%s/%s%d.%ld",
			 tempdirpath, PG_TEMP_FILE_PREFIX, MyProcPid, tempFileCounter++);

	/*
	 * Open the file.  Note: we don't use O_EXCL, in case there is an orphaned
	 * temp file that can be reused.
	 */
	file = PathNameOpenFile(tempfilepath,
							O_RDWR | O_CREAT | O_TRUNC | PG_BINARY,
							0600);
	if (file <= 0)
	{
		/*
		 * We might need to create the tablespace's tempfile directory, if no
		 * one has yet done so.
		 *
		 * Don't check for error from mkdir; it could fail if someone else
		 * just did the same thing.  If it doesn't work then we'll bomb out on
		 * the second create attempt, instead.
		 */
		mkdir(tempdirpath, S_IRWXU);

		file = PathNameOpenFile(tempfilepath,
								O_RDWR | O_CREAT | O_TRUNC | PG_BINARY,
								0600);
		if (file <= 0 && rejectError)
			elog(ERROR, "could not create temporary file \"%s\": %m",
				 tempfilepath);
	}

	return file;
}

/*
 * close a file when done with it
 */
void
FileClose(File file)
{
	Vfd		   *vfdP;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileClose: %d (%s)",
			   file, VfdCache[file].fileName));

	vfdP = &VfdCache[file];

	if (!FileIsNotOpen(file))
	{
		/* remove the file from the lru ring */
		Delete(file);

		/* close the file */
		if (close(vfdP->fd))
			elog(ERROR, "could not close file \"%s\": %m", vfdP->fileName);

		--nfile;
		vfdP->fd = VFD_CLOSED;
	}

	/*
	 * Delete the file if it was temporary, and make a log entry if wanted
	 */
	if (vfdP->fdstate & FD_TEMPORARY)
	{
		/*
		 * If we get an error, as could happen within the ereport/elog calls,
		 * we'll come right back here during transaction abort.  Reset the
		 * flag to ensure that we can't get into an infinite loop.  This code
		 * is arranged to ensure that the worst-case consequence is failing to
		 * emit log message(s), not failing to attempt the unlink.
		 */
		vfdP->fdstate &= ~FD_TEMPORARY;

		if (log_temp_files >= 0)
		{
			struct stat filestats;
			int			stat_errno;

			/* first try the stat() */
			if (stat(vfdP->fileName, &filestats))
				stat_errno = errno;
			else
				stat_errno = 0;

			/* in any case do the unlink */
			if (unlink(vfdP->fileName))
				elog(LOG, "could not unlink file \"%s\": %m", vfdP->fileName);

			/* and last report the stat results */
			if (stat_errno == 0)
			{
				if ((filestats.st_size / 1024) >= log_temp_files)
					ereport(LOG,
							(errmsg("temporary file: path \"%s\", size %lu",
									vfdP->fileName,
									(unsigned long) filestats.st_size)));
			}
			else
			{
				errno = stat_errno;
				elog(LOG, "could not stat file \"%s\": %m", vfdP->fileName);
			}
		}
		else
		{
			/* easy case, just do the unlink */
			if (unlink(vfdP->fileName))
				elog(LOG, "could not unlink file \"%s\": %m", vfdP->fileName);
		}
	}

	/* Unregister it from the resource owner */
	if (vfdP->resowner)
		ResourceOwnerForgetFile(vfdP->resowner, file);

	/*
	 * Return the Vfd slot to the free list
	 */
	FreeVfd(file);
}

/*
 * FilePrefetch - initiate asynchronous read of a given range of the file.
 * The logical seek position is unaffected.
 *
 * Currently the only implementation of this function is using posix_fadvise
 * which is the simplest standardized interface that accomplishes this.
 * We could add an implementation using libaio in the future; but note that
 * this API is inappropriate for libaio, which wants to have a buffer provided
 * to read into.
 */
int
FilePrefetch(File file, off_t offset, int amount)
{
#if defined(USE_POSIX_FADVISE) && defined(POSIX_FADV_WILLNEED)
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FilePrefetch: %d (%s) " INT64_FORMAT " %d",
			   file, VfdCache[file].fileName,
			   (int64) offset, amount));

	returnCode = FileAccess(file);
	if (returnCode < 0)
		return returnCode;

	returnCode = posix_fadvise(VfdCache[file].fd, offset, amount,
							   POSIX_FADV_WILLNEED);

	return returnCode;
#else
	Assert(FileIsValid(file));
	return 0;
#endif
}

int
FileRead(File file, char *buffer, int amount)
{
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileRead: %d (%s) " INT64_FORMAT " %d %p",
			   file, VfdCache[file].fileName,
			   (int64) VfdCache[file].seekPos,
			   amount, buffer));

	returnCode = FileAccess(file);
	if (returnCode < 0)
		return returnCode;

retry:
	returnCode = read(VfdCache[file].fd, buffer, amount);

	if (returnCode >= 0)
		VfdCache[file].seekPos += returnCode;
	else
	{
		/*
		 * Windows may run out of kernel buffers and return "Insufficient
		 * system resources" error.  Wait a bit and retry to solve it.
		 *
		 * It is rumored that EINTR is also possible on some Unix filesystems,
		 * in which case immediate retry is indicated.
		 */
#ifdef WIN32
		DWORD		error = GetLastError();

		switch (error)
		{
			case ERROR_NO_SYSTEM_RESOURCES:
				pg_usleep(1000L);
				errno = EINTR;
				break;
			default:
				_dosmaperr(error);
				break;
		}
#endif
		/* OK to retry if interrupted */
		if (errno == EINTR)
			goto retry;

		/* Trouble, so assume we don't know the file position anymore */
		VfdCache[file].seekPos = FileUnknownPos;
	}

	return returnCode;
}

int
FileWrite(File file, char *buffer, int amount)
{
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileWrite: %d (%s) " INT64_FORMAT " %d %p",
			   file, VfdCache[file].fileName,
			   (int64) VfdCache[file].seekPos,
			   amount, buffer));

	returnCode = FileAccess(file);
	if (returnCode < 0)
		return returnCode;

retry:
	errno = 0;
	returnCode = write(VfdCache[file].fd, buffer, amount);

	/* if write didn't set errno, assume problem is no disk space */
	if (returnCode != amount && errno == 0)
		errno = ENOSPC;

	if (returnCode >= 0)
		VfdCache[file].seekPos += returnCode;
	else
	{
		/*
		 * See comments in FileRead()
		 */
#ifdef WIN32
		DWORD		error = GetLastError();

		switch (error)
		{
			case ERROR_NO_SYSTEM_RESOURCES:
				pg_usleep(1000L);
				errno = EINTR;
				break;
			default:
				_dosmaperr(error);
				break;
		}
#endif
		/* OK to retry if interrupted */
		if (errno == EINTR)
			goto retry;

		/* Trouble, so assume we don't know the file position anymore */
		VfdCache[file].seekPos = FileUnknownPos;
	}

	return returnCode;
}

int
FileSync(File file)
{
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileSync: %d (%s)",
			   file, VfdCache[file].fileName));

	returnCode = FileAccess(file);
	if (returnCode < 0)
		return returnCode;

	return pg_fsync(VfdCache[file].fd);
}

off_t
FileSeek(File file, off_t offset, int whence)
{
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileSeek: %d (%s) " INT64_FORMAT " " INT64_FORMAT " %d",
			   file, VfdCache[file].fileName,
			   (int64) VfdCache[file].seekPos,
			   (int64) offset, whence));

	if (FileIsNotOpen(file))
	{
		switch (whence)
		{
			case SEEK_SET:
				if (offset < 0)
					elog(ERROR, "invalid seek offset: " INT64_FORMAT,
						 (int64) offset);
				VfdCache[file].seekPos = offset;
				break;
			case SEEK_CUR:
				VfdCache[file].seekPos += offset;
				break;
			case SEEK_END:
				returnCode = FileAccess(file);
				if (returnCode < 0)
					return returnCode;
				VfdCache[file].seekPos = lseek(VfdCache[file].fd,
											   offset, whence);
				break;
			default:
				elog(ERROR, "invalid whence: %d", whence);
				break;
		}
	}
	else
	{
		switch (whence)
		{
			case SEEK_SET:
				if (offset < 0)
					elog(ERROR, "invalid seek offset: " INT64_FORMAT,
						 (int64) offset);
				if (VfdCache[file].seekPos != offset)
					VfdCache[file].seekPos = lseek(VfdCache[file].fd,
												   offset, whence);
				break;
			case SEEK_CUR:
				if (offset != 0 || VfdCache[file].seekPos == FileUnknownPos)
					VfdCache[file].seekPos = lseek(VfdCache[file].fd,
												   offset, whence);
				break;
			case SEEK_END:
				VfdCache[file].seekPos = lseek(VfdCache[file].fd,
											   offset, whence);
				break;
			default:
				elog(ERROR, "invalid whence: %d", whence);
				break;
		}
	}
	return VfdCache[file].seekPos;
}

/*
 * XXX not actually used but here for completeness
 */
#ifdef NOT_USED
off_t
FileTell(File file)
{
	Assert(FileIsValid(file));
	DO_DB(elog(LOG, "FileTell %d (%s)",
			   file, VfdCache[file].fileName));
	return VfdCache[file].seekPos;
}
#endif

int
FileTruncate(File file, off_t offset)
{
	int			returnCode;

	Assert(FileIsValid(file));

	DO_DB(elog(LOG, "FileTruncate %d (%s)",
			   file, VfdCache[file].fileName));

	returnCode = FileAccess(file);
	if (returnCode < 0)
		return returnCode;

	returnCode = ftruncate(VfdCache[file].fd, offset);
	return returnCode;
}

/*
 * Return the pathname associated with an open file.
 *
 * The returned string points to an internal buffer, which is valid until
 * the file is closed.
 */
char *
FilePathName(File file)
{
	Assert(FileIsValid(file));

	return VfdCache[file].fileName;
}


/*
 * Make room for another allocatedDescs[] array entry if needed and possible.
 * Returns true if an array element is available.
 */
static bool
reserveAllocatedDesc(void)
{
	AllocateDesc *newDescs;
	int			newMax;

	/* Quick out if array already has a free slot. */
	if (numAllocatedDescs < maxAllocatedDescs)
		return true;

	/*
	 * If the array hasn't yet been created in the current process, initialize
	 * it with FD_MINFREE / 2 elements.  In many scenarios this is as many as
	 * we will ever need, anyway.  We don't want to look at max_safe_fds
	 * immediately because set_max_safe_fds() may not have run yet.
	 */
	if (allocatedDescs == NULL)
	{
		newMax = FD_MINFREE / 2;
		newDescs = (AllocateDesc *) malloc(newMax * sizeof(AllocateDesc));
		/* Out of memory already?  Treat as fatal error. */
		if (newDescs == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		allocatedDescs = newDescs;
		maxAllocatedDescs = newMax;
		return true;
	}

	/*
	 * Consider enlarging the array beyond the initial allocation used above.
	 * By the time this happens, max_safe_fds should be known accurately.
	 *
	 * We mustn't let allocated descriptors hog all the available FDs, and in
	 * practice we'd better leave a reasonable number of FDs for VFD use.  So
	 * set the maximum to max_safe_fds / 2.  (This should certainly be at
	 * least as large as the initial size, FD_MINFREE / 2.)
	 */
	newMax = max_safe_fds / 2;
	if (newMax > maxAllocatedDescs)
	{
		newDescs = (AllocateDesc *) realloc(allocatedDescs,
											newMax * sizeof(AllocateDesc));
		/* Treat out-of-memory as a non-fatal error. */
		if (newDescs == NULL)
			return false;
		allocatedDescs = newDescs;
		maxAllocatedDescs = newMax;
		return true;
	}

	/* Can't enlarge allocatedDescs[] any more. */
	return false;
}

/*
 * Routines that want to use stdio (ie, FILE*) should use AllocateFile
 * rather than plain fopen().  This lets fd.c deal with freeing FDs if
 * necessary to open the file.	When done, call FreeFile rather than fclose.
 *
 * Note that files that will be open for any significant length of time
 * should NOT be handled this way, since they cannot share kernel file
 * descriptors with other files; there is grave risk of running out of FDs
 * if anyone locks down too many FDs.  Most callers of this routine are
 * simply reading a config file that they will read and close immediately.
 *
 * fd.c will automatically close all files opened with AllocateFile at
 * transaction commit or abort; this prevents FD leakage if a routine
 * that calls AllocateFile is terminated prematurely by ereport(ERROR).
 *
 * Ideally this should be the *only* direct call of fopen() in the backend.
 */
FILE *
AllocateFile(const char *name, const char *mode)
{
	FILE	   *file;

	DO_DB(elog(LOG, "AllocateFile: Allocated %d (%s)",
			   numAllocatedDescs, name));

	/* Can we allocate another non-virtual FD? */
	if (!reserveAllocatedDesc())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("exceeded maxAllocatedDescs (%d) while trying to open file \"%s\"",
						maxAllocatedDescs, name)));

	/* Close excess kernel FDs. */
	ReleaseLruFiles();

TryAgain:
	if ((file = fopen(name, mode)) != NULL)
	{
		AllocateDesc *desc = &allocatedDescs[numAllocatedDescs];

		desc->kind = AllocateDescFile;
		desc->desc.file = file;
		desc->create_subid = GetCurrentSubTransactionId();
		numAllocatedDescs++;
		return desc->desc.file;
	}

	if (errno == EMFILE || errno == ENFILE)
	{
		int			save_errno = errno;

		ereport(LOG,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("out of file descriptors: %m; release and retry")));
		errno = 0;
		if (ReleaseLruFile())
			goto TryAgain;
		errno = save_errno;
	}

	return NULL;
}

/*
 * Free an AllocateDesc of either type.
 *
 * The argument *must* point into the allocatedDescs[] array.
 */
static int
FreeDesc(AllocateDesc *desc)
{
	int			result;

	/* Close the underlying object */
	switch (desc->kind)
	{
		case AllocateDescFile:
			result = fclose(desc->desc.file);
			break;
		case AllocateDescDir:
			result = closedir(desc->desc.dir);
			break;
		default:
			elog(ERROR, "AllocateDesc kind not recognized");
			result = 0;			/* keep compiler quiet */
			break;
	}

	/* Compact storage in the allocatedDescs array */
	numAllocatedDescs--;
	*desc = allocatedDescs[numAllocatedDescs];

	return result;
}

/*
 * Close a file returned by AllocateFile.
 *
 * Note we do not check fclose's return value --- it is up to the caller
 * to handle close errors.
 */
int
FreeFile(FILE *file)
{
	int			i;

	DO_DB(elog(LOG, "FreeFile: Allocated %d", numAllocatedDescs));

	/* Remove file from list of allocated files, if it's present */
	for (i = numAllocatedDescs; --i >= 0;)
	{
		AllocateDesc *desc = &allocatedDescs[i];

		if (desc->kind == AllocateDescFile && desc->desc.file == file)
			return FreeDesc(desc);
	}

	/* Only get here if someone passes us a file not in allocatedDescs */
	elog(WARNING, "file passed to FreeFile was not obtained from AllocateFile");

	return fclose(file);
}


/*
 * Routines that want to use <dirent.h> (ie, DIR*) should use AllocateDir
 * rather than plain opendir().  This lets fd.c deal with freeing FDs if
 * necessary to open the directory, and with closing it after an elog.
 * When done, call FreeDir rather than closedir.
 *
 * Ideally this should be the *only* direct call of opendir() in the backend.
 */
DIR *
AllocateDir(const char *dirname)
{
	DIR		   *dir;

	DO_DB(elog(LOG, "AllocateDir: Allocated %d (%s)",
			   numAllocatedDescs, dirname));

	/* Can we allocate another non-virtual FD? */
	if (!reserveAllocatedDesc())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("exceeded maxAllocatedDescs (%d) while trying to open directory \"%s\"",
						maxAllocatedDescs, dirname)));

	/* Close excess kernel FDs. */
	ReleaseLruFiles();

TryAgain:
	if ((dir = opendir(dirname)) != NULL)
	{
		AllocateDesc *desc = &allocatedDescs[numAllocatedDescs];

		desc->kind = AllocateDescDir;
		desc->desc.dir = dir;
		desc->create_subid = GetCurrentSubTransactionId();
		numAllocatedDescs++;
		return desc->desc.dir;
	}

	if (errno == EMFILE || errno == ENFILE)
	{
		int			save_errno = errno;

		ereport(LOG,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("out of file descriptors: %m; release and retry")));
		errno = 0;
		if (ReleaseLruFile())
			goto TryAgain;
		errno = save_errno;
	}

	return NULL;
}

/*
 * Read a directory opened with AllocateDir, ereport'ing any error.
 *
 * This is easier to use than raw readdir() since it takes care of some
 * otherwise rather tedious and error-prone manipulation of errno.	Also,
 * if you are happy with a generic error message for AllocateDir failure,
 * you can just do
 *
 *		dir = AllocateDir(path);
 *		while ((dirent = ReadDir(dir, path)) != NULL)
 *			process dirent;
 *		FreeDir(dir);
 *
 * since a NULL dir parameter is taken as indicating AllocateDir failed.
 * (Make sure errno hasn't been changed since AllocateDir if you use this
 * shortcut.)
 *
 * The pathname passed to AllocateDir must be passed to this routine too,
 * but it is only used for error reporting.
 */
struct dirent *
ReadDir(DIR *dir, const char *dirname)
{
	struct dirent *dent;

	/* Give a generic message for AllocateDir failure, if caller didn't */
	if (dir == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open directory \"%s\": %m",
						dirname)));

	errno = 0;
	if ((dent = readdir(dir)) != NULL)
		return dent;

#ifdef WIN32

	/*
	 * This fix is in mingw cvs (runtime/mingwex/dirent.c rev 1.4), but not in
	 * released version
	 */
	if (GetLastError() == ERROR_NO_MORE_FILES)
		errno = 0;
#endif

	if (errno)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read directory \"%s\": %m",
						dirname)));
	return NULL;
}

/*
 * Close a directory opened with AllocateDir.
 *
 * Note we do not check closedir's return value --- it is up to the caller
 * to handle close errors.
 */
int
FreeDir(DIR *dir)
{
	int			i;

	DO_DB(elog(LOG, "FreeDir: Allocated %d", numAllocatedDescs));

	/* Remove dir from list of allocated dirs, if it's present */
	for (i = numAllocatedDescs; --i >= 0;)
	{
		AllocateDesc *desc = &allocatedDescs[i];

		if (desc->kind == AllocateDescDir && desc->desc.dir == dir)
			return FreeDesc(desc);
	}

	/* Only get here if someone passes us a dir not in allocatedDescs */
	elog(WARNING, "dir passed to FreeDir was not obtained from AllocateDir");

	return closedir(dir);
}


/*
 * closeAllVfds
 *
 * Force all VFDs into the physically-closed state, so that the fewest
 * possible number of kernel file descriptors are in use.  There is no
 * change in the logical state of the VFDs.
 */
void
closeAllVfds(void)
{
	Index		i;

	if (SizeVfdCache > 0)
	{
		Assert(FileIsNotOpen(0));		/* Make sure ring not corrupted */
		for (i = 1; i < SizeVfdCache; i++)
		{
			if (!FileIsNotOpen(i))
				LruDelete(i);
		}
	}
}


/*
 * SetTempTablespaces
 *
 * Define a list (actually an array) of OIDs of tablespaces to use for
 * temporary files.  This list will be used until end of transaction,
 * unless this function is called again before then.  It is caller's
 * responsibility that the passed-in array has adequate lifespan (typically
 * it'd be allocated in TopTransactionContext).
 */
void
SetTempTablespaces(Oid *tableSpaces, int numSpaces)
{
	Assert(numSpaces >= 0);
	tempTableSpaces = tableSpaces;
	numTempTableSpaces = numSpaces;

	/*
	 * Select a random starting point in the list.	This is to minimize
	 * conflicts between backends that are most likely sharing the same list
	 * of temp tablespaces.  Note that if we create multiple temp files in the
	 * same transaction, we'll advance circularly through the list --- this
	 * ensures that large temporary sort files are nicely spread across all
	 * available tablespaces.
	 */
	if (numSpaces > 1)
		nextTempTableSpace = random() % numSpaces;
	else
		nextTempTableSpace = 0;
}

/*
 * TempTablespacesAreSet
 *
 * Returns TRUE if SetTempTablespaces has been called in current transaction.
 * (This is just so that tablespaces.c doesn't need its own per-transaction
 * state.)
 */
bool
TempTablespacesAreSet(void)
{
	return (numTempTableSpaces >= 0);
}

/*
 * GetNextTempTableSpace
 *
 * Select the next temp tablespace to use.	A result of InvalidOid means
 * to use the current database's default tablespace.
 */
Oid
GetNextTempTableSpace(void)
{
	if (numTempTableSpaces > 0)
	{
		/* Advance nextTempTableSpace counter with wraparound */
		if (++nextTempTableSpace >= numTempTableSpaces)
			nextTempTableSpace = 0;
		return tempTableSpaces[nextTempTableSpace];
	}
	return InvalidOid;
}


/*
 * AtEOSubXact_Files
 *
 * Take care of subtransaction commit/abort.  At abort, we close temp files
 * that the subtransaction may have opened.  At commit, we reassign the
 * files that were opened to the parent subtransaction.
 */
void
AtEOSubXact_Files(bool isCommit, SubTransactionId mySubid,
				  SubTransactionId parentSubid)
{
	Index		i;

	for (i = 0; i < numAllocatedDescs; i++)
	{
		if (allocatedDescs[i].create_subid == mySubid)
		{
			if (isCommit)
				allocatedDescs[i].create_subid = parentSubid;
			else
			{
				/* have to recheck the item after FreeDesc (ugly) */
				FreeDesc(&allocatedDescs[i--]);
			}
		}
	}
}

/*
 * AtEOXact_Files
 *
 * This routine is called during transaction commit or abort (it doesn't
 * particularly care which).  All still-open per-transaction temporary file
 * VFDs are closed, which also causes the underlying files to be deleted
 * (although they should've been closed already by the ResourceOwner
 * cleanup). Furthermore, all "allocated" stdio files are closed. We also
 * forget any transaction-local temp tablespace list.
 */
void
AtEOXact_Files(void)
{
	CleanupTempFiles(false);
	tempTableSpaces = NULL;
	numTempTableSpaces = -1;
}

/*
 * AtProcExit_Files
 *
 * on_proc_exit hook to clean up temp files during backend shutdown.
 * Here, we want to clean up *all* temp files including interXact ones.
 */
static void
AtProcExit_Files(int code, Datum arg)
{
	CleanupTempFiles(true);
}

/*
 * Close temporary files and delete their underlying files.
 *
 * isProcExit: if true, this is being called as the backend process is
 * exiting. If that's the case, we should remove all temporary files; if
 * that's not the case, we are being called for transaction commit/abort
 * and should only remove transaction-local temp files.  In either case,
 * also clean up "allocated" stdio files and dirs.
 */
static void
CleanupTempFiles(bool isProcExit)
{
	Index		i;

	/*
	 * Careful here: at proc_exit we need extra cleanup, not just
	 * xact_temporary files.
	 */
	if (isProcExit || have_xact_temporary_files)
	{
		Assert(FileIsNotOpen(0));		/* Make sure ring not corrupted */
		for (i = 1; i < SizeVfdCache; i++)
		{
			unsigned short fdstate = VfdCache[i].fdstate;

			if ((fdstate & FD_TEMPORARY) && VfdCache[i].fileName != NULL)
			{
				/*
				 * If we're in the process of exiting a backend process, close
				 * all temporary files. Otherwise, only close temporary files
				 * local to the current transaction. They should be closed by
				 * the ResourceOwner mechanism already, so this is just a
				 * debugging cross-check.
				 */
				if (isProcExit)
					FileClose(i);
				else if (fdstate & FD_XACT_TEMPORARY)
				{
					elog(WARNING,
						 "temporary file %s not closed at end-of-transaction",
						 VfdCache[i].fileName);
					FileClose(i);
				}
			}
		}

		have_xact_temporary_files = false;
	}

	/* Clean up "allocated" stdio files and dirs. */
	while (numAllocatedDescs > 0)
		FreeDesc(&allocatedDescs[0]);
}


/*
 * Remove temporary and temporary relation files left over from a prior
 * postmaster session
 *
 * This should be called during postmaster startup.  It will forcibly
 * remove any leftover files created by OpenTemporaryFile and any leftover
 * temporary relation files created by mdcreate.
 *
 * NOTE: we could, but don't, call this during a post-backend-crash restart
 * cycle.  The argument for not doing it is that someone might want to examine
 * the temp files for debugging purposes.  This does however mean that
 * OpenTemporaryFile had better allow for collision with an existing temp
 * file name.
 */
void
RemovePgTempFiles(void)
{
	char		temp_path[MAXPGPATH];
	DIR		   *spc_dir;
	struct dirent *spc_de;

	/*
	 * First process temp files in pg_default ($PGDATA/base)
	 */
	snprintf(temp_path, sizeof(temp_path), "base/%s", PG_TEMP_FILES_DIR);
	RemovePgTempFilesInDir(temp_path);
	RemovePgTempRelationFiles("base");

	/*
	 * Cycle through temp directories for all non-default tablespaces.
	 */
	spc_dir = AllocateDir("pg_tblspc");

	while ((spc_de = ReadDir(spc_dir, "pg_tblspc")) != NULL)
	{
		if (strcmp(spc_de->d_name, ".") == 0 ||
			strcmp(spc_de->d_name, "..") == 0)
			continue;

		snprintf(temp_path, sizeof(temp_path), "pg_tblspc/%s/%s/%s",
			spc_de->d_name, TABLESPACE_VERSION_DIRECTORY, PG_TEMP_FILES_DIR);
		RemovePgTempFilesInDir(temp_path);

		snprintf(temp_path, sizeof(temp_path), "pg_tblspc/%s/%s",
				 spc_de->d_name, TABLESPACE_VERSION_DIRECTORY);
		RemovePgTempRelationFiles(temp_path);
	}

	FreeDir(spc_dir);

	/*
	 * In EXEC_BACKEND case there is a pgsql_tmp directory at the top level of
	 * DataDir as well.
	 */
#ifdef EXEC_BACKEND
	RemovePgTempFilesInDir(PG_TEMP_FILES_DIR);
#endif
}

/* Process one pgsql_tmp directory for RemovePgTempFiles */
static void
RemovePgTempFilesInDir(const char *tmpdirname)
{
	DIR		   *temp_dir;
	struct dirent *temp_de;
	char		rm_path[MAXPGPATH];

	temp_dir = AllocateDir(tmpdirname);
	if (temp_dir == NULL)
	{
		/* anything except ENOENT is fishy */
		if (errno != ENOENT)
			elog(LOG,
				 "could not open temporary-files directory \"%s\": %m",
				 tmpdirname);
		return;
	}

	while ((temp_de = ReadDir(temp_dir, tmpdirname)) != NULL)
	{
		if (strcmp(temp_de->d_name, ".") == 0 ||
			strcmp(temp_de->d_name, "..") == 0)
			continue;

		snprintf(rm_path, sizeof(rm_path), "%s/%s",
				 tmpdirname, temp_de->d_name);

		if (strncmp(temp_de->d_name,
					PG_TEMP_FILE_PREFIX,
					strlen(PG_TEMP_FILE_PREFIX)) == 0)
			unlink(rm_path);	/* note we ignore any error */
		else
			elog(LOG,
				 "unexpected file found in temporary-files directory: \"%s\"",
				 rm_path);
	}

	FreeDir(temp_dir);
}

/* Process one tablespace directory, look for per-DB subdirectories */
static void
RemovePgTempRelationFiles(const char *tsdirname)
{
	DIR		   *ts_dir;
	struct dirent *de;
	char		dbspace_path[MAXPGPATH];

	ts_dir = AllocateDir(tsdirname);
	if (ts_dir == NULL)
	{
		/* anything except ENOENT is fishy */
		if (errno != ENOENT)
			elog(LOG,
				 "could not open tablespace directory \"%s\": %m",
				 tsdirname);
		return;
	}

	while ((de = ReadDir(ts_dir, tsdirname)) != NULL)
	{
		int			i = 0;

		/*
		 * We're only interested in the per-database directories, which have
		 * numeric names.  Note that this code will also (properly) ignore "."
		 * and "..".
		 */
		while (isdigit((unsigned char) de->d_name[i]))
			++i;
		if (de->d_name[i] != '\0' || i == 0)
			continue;

		snprintf(dbspace_path, sizeof(dbspace_path), "%s/%s",
				 tsdirname, de->d_name);
		RemovePgTempRelationFilesInDbspace(dbspace_path);
	}

	FreeDir(ts_dir);
}

/* Process one per-dbspace directory for RemovePgTempRelationFiles */
static void
RemovePgTempRelationFilesInDbspace(const char *dbspacedirname)
{
	DIR		   *dbspace_dir;
	struct dirent *de;
	char		rm_path[MAXPGPATH];

	dbspace_dir = AllocateDir(dbspacedirname);
	if (dbspace_dir == NULL)
	{
		/* we just saw this directory, so it really ought to be there */
		elog(LOG,
			 "could not open dbspace directory \"%s\": %m",
			 dbspacedirname);
		return;
	}

	while ((de = ReadDir(dbspace_dir, dbspacedirname)) != NULL)
	{
		if (!looks_like_temp_rel_name(de->d_name))
			continue;

		snprintf(rm_path, sizeof(rm_path), "%s/%s",
				 dbspacedirname, de->d_name);

		unlink(rm_path);		/* note we ignore any error */
	}

	FreeDir(dbspace_dir);
}

/* t<digits>_<digits>, or t<digits>_<digits>_<forkname> */
static bool
looks_like_temp_rel_name(const char *name)
{
	int			pos;
	int			savepos;

	/* Must start with "t". */
	if (name[0] != 't')
		return false;

	/* Followed by a non-empty string of digits and then an underscore. */
	for (pos = 1; isdigit((unsigned char) name[pos]); ++pos)
		;
	if (pos == 1 || name[pos] != '_')
		return false;

	/* Followed by another nonempty string of digits. */
	for (savepos = ++pos; isdigit((unsigned char) name[pos]); ++pos)
		;
	if (savepos == pos)
		return false;

	/* We might have _forkname or .segment or both. */
	if (name[pos] == '_')
	{
		int			forkchar = forkname_chars(&name[pos + 1], NULL);

		if (forkchar <= 0)
			return false;
		pos += forkchar + 1;
	}
	if (name[pos] == '.')
	{
		int			segchar;

		for (segchar = 1; isdigit((unsigned char) name[pos + segchar]); ++segchar)
			;
		if (segchar <= 1)
			return false;
		pos += segchar;
	}

	/* Now we should be at the end. */
	if (name[pos] != '\0')
		return false;
	return true;
}
