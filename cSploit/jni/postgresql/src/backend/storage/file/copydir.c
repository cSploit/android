/*-------------------------------------------------------------------------
 *
 * copydir.c
 *	  copies a directory
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	While "xcopy /e /i /q" works fine for copying directories, on Windows XP
 *	it requires a Window handle which prevents it from working when invoked
 *	as a service.
 *
 * IDENTIFICATION
 *	  src/backend/storage/file/copydir.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "storage/copydir.h"
#include "storage/fd.h"
#include "miscadmin.h"

/*
 *	On Windows, call non-macro versions of palloc; we can't reference
 *	CurrentMemoryContext in this file because of PGDLLIMPORT conflict.
 */
#if defined(WIN32) || defined(__CYGWIN__)
#undef palloc
#undef pstrdup
#define palloc(sz)		pgport_palloc(sz)
#define pstrdup(str)	pgport_pstrdup(str)
#endif


static void fsync_fname(char *fname, bool isdir);


/*
 * copydir: copy a directory
 *
 * If recurse is false, subdirectories are ignored.  Anything that's not
 * a directory or a regular file is ignored.
 */
void
copydir(char *fromdir, char *todir, bool recurse)
{
	DIR		   *xldir;
	struct dirent *xlde;
	char		fromfile[MAXPGPATH];
	char		tofile[MAXPGPATH];

	if (mkdir(todir, S_IRWXU) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not create directory \"%s\": %m", todir)));

	xldir = AllocateDir(fromdir);
	if (xldir == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open directory \"%s\": %m", fromdir)));

	while ((xlde = ReadDir(xldir, fromdir)) != NULL)
	{
		struct stat fst;

		/* If we got a cancel signal during the copy of the directory, quit */
		CHECK_FOR_INTERRUPTS();

		if (strcmp(xlde->d_name, ".") == 0 ||
			strcmp(xlde->d_name, "..") == 0)
			continue;

		snprintf(fromfile, MAXPGPATH, "%s/%s", fromdir, xlde->d_name);
		snprintf(tofile, MAXPGPATH, "%s/%s", todir, xlde->d_name);

		if (lstat(fromfile, &fst) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not stat file \"%s\": %m", fromfile)));

		if (S_ISDIR(fst.st_mode))
		{
			/* recurse to handle subdirectories */
			if (recurse)
				copydir(fromfile, tofile, true);
		}
		else if (S_ISREG(fst.st_mode))
			copy_file(fromfile, tofile);
	}
	FreeDir(xldir);

	/*
	 * Be paranoid here and fsync all files to ensure the copy is really done.
	 */
	xldir = AllocateDir(todir);
	if (xldir == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open directory \"%s\": %m", todir)));

	while ((xlde = ReadDir(xldir, todir)) != NULL)
	{
		struct stat fst;

		if (strcmp(xlde->d_name, ".") == 0 ||
			strcmp(xlde->d_name, "..") == 0)
			continue;

		snprintf(tofile, MAXPGPATH, "%s/%s", todir, xlde->d_name);

		/*
		 * We don't need to sync subdirectories here since the recursive
		 * copydir will do it before it returns
		 */
		if (lstat(tofile, &fst) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not stat file \"%s\": %m", tofile)));

		if (S_ISREG(fst.st_mode))
			fsync_fname(tofile, false);
	}
	FreeDir(xldir);

	/*
	 * It's important to fsync the destination directory itself as individual
	 * file fsyncs don't guarantee that the directory entry for the file is
	 * synced. Recent versions of ext4 have made the window much wider but
	 * it's been true for ext3 and other filesystems in the past.
	 */
	fsync_fname(todir, true);
}

/*
 * copy one file
 */
void
copy_file(char *fromfile, char *tofile)
{
	char	   *buffer;
	int			srcfd;
	int			dstfd;
	int			nbytes;
	off_t		offset;

	/* Use palloc to ensure we get a maxaligned buffer */
#define COPY_BUF_SIZE (8 * BLCKSZ)

	buffer = palloc(COPY_BUF_SIZE);

	/*
	 * Open the files
	 */
	srcfd = BasicOpenFile(fromfile, O_RDONLY | PG_BINARY, 0);
	if (srcfd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m", fromfile)));

	dstfd = BasicOpenFile(tofile, O_RDWR | O_CREAT | O_EXCL | PG_BINARY,
						  S_IRUSR | S_IWUSR);
	if (dstfd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not create file \"%s\": %m", tofile)));

	/*
	 * Do the data copying.
	 */
	for (offset = 0;; offset += nbytes)
	{
		/* If we got a cancel signal during the copy of the file, quit */
		CHECK_FOR_INTERRUPTS();

		nbytes = read(srcfd, buffer, COPY_BUF_SIZE);
		if (nbytes < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read file \"%s\": %m", fromfile)));
		if (nbytes == 0)
			break;
		errno = 0;
		if ((int) write(dstfd, buffer, nbytes) != nbytes)
		{
			/* if write didn't set errno, assume problem is no disk space */
			if (errno == 0)
				errno = ENOSPC;
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not write to file \"%s\": %m", tofile)));
		}

		/*
		 * We fsync the files later but first flush them to avoid spamming the
		 * cache and hopefully get the kernel to start writing them out before
		 * the fsync comes.
		 */
		pg_flush_data(dstfd, offset, nbytes);
	}

	if (close(dstfd))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", tofile)));

	close(srcfd);

	pfree(buffer);
}


/*
 * fsync a file
 *
 * Try to fsync directories but ignore errors that indicate the OS
 * just doesn't allow/require fsyncing directories.
 */
static void
fsync_fname(char *fname, bool isdir)
{
	int			fd;
	int			returncode;

	/*
	 * Some OSs require directories to be opened read-only whereas other
	 * systems don't allow us to fsync files opened read-only; so we need both
	 * cases here
	 */
	if (!isdir)
		fd = BasicOpenFile(fname,
						   O_RDWR | PG_BINARY,
						   S_IRUSR | S_IWUSR);
	else
		fd = BasicOpenFile(fname,
						   O_RDONLY | PG_BINARY,
						   S_IRUSR | S_IWUSR);

	/*
	 * Some OSs don't allow us to open directories at all (Windows returns
	 * EACCES)
	 */
	if (fd < 0 && isdir && (errno == EISDIR || errno == EACCES))
		return;

	else if (fd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m", fname)));

	returncode = pg_fsync(fd);

	/* Some OSs don't allow us to fsync directories at all */
	if (returncode != 0 && isdir && errno == EBADF)
	{
		close(fd);
		return;
	}

	if (returncode != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not fsync file \"%s\": %m", fname)));

	close(fd);
}
