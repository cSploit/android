/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_sync.cpp
 *	DESCRIPTION:	General purpose but non-user routines.
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "XENIX" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "M88K" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SOLARIS
#ifndef DEV_BUILD
#define NDEBUG                                 // Turn off fb_assert() macros
#endif
#include "../jrd/gdsassert.h"
#endif

#ifdef HPUX
#include <sys/pstat.h>
#endif

#include "../jrd/common.h"
#include "gen/iberror.h"
#include "../jrd/isc.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/os/os_utils.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/file_params.h"
#include "../jrd/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/err_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/jrd_pwd.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../common/StatusArg.h"
#include "../common/classes/RefMutex.h"

static int process_id;

/* Unix specific stuff */

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_SYS5SEMAPHORE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/mman.h>

#define FTOK_KEY	15
#define PRIV		0666

//#ifndef SHMEM_DELTA
//#define SHMEM_DELTA	(1 << 22)
//#endif

//#ifndef SIGURG
//#define SIGURG		SIGINT
//#endif

#ifndef HAVE_SEMUN
union semun
{
	int val;
	struct semid_ds *buf;
	USHORT *array;
};
#endif
#endif // UNIX

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if !defined HAVE_GETPAGESIZE && !defined ARM
static size_t getpagesize()
{
	return PAGESIZE;
}
#endif

//#define DEBUG_IPC
#ifdef DEBUG_IPC
#define IPC_TRACE(x)	{ /*time_t t; time(&t); printf("%s", ctime(&t) ); printf x; fflush (stdout);*/ gds__log x; }
#else
#define IPC_TRACE(x)
#endif


/* Windows NT */

#ifdef WIN_NT

#include <process.h>
#include <windows.h>

#endif

using namespace Jrd;
using namespace Firebird;

static void		error(ISC_STATUS*, const TEXT*, ISC_STATUS);
static bool		event_blocked(const event_t* event, const SLONG value);

#ifdef UNIX

#if defined(USE_FILELOCKS) || (!defined(HAVE_FLOCK))
#define USE_FCNTL
#endif

static GlobalPtr<Mutex> openFdInit;


namespace {

#ifdef USE_FCNTL
class CountedRWLock
{
public:
	CountedRWLock()
		: sharedAccessCounter(0)
	{ }
	RWLock rwlock;
	AtomicCounter cnt;
	Mutex sharedAccessMutex;
	int sharedAccessCounter;
};

class DevNode
{
public:
	DevNode()
		: f_dev(0), f_ino(0)
	{ }

	DevNode(dev_t d, ino_t i)
		: f_dev(d), f_ino(i)
	{ }

	DevNode(const DevNode& v)
		: f_dev(v.f_dev), f_ino(v.f_ino)
	{ }

	dev_t	f_dev;
	ino_t	f_ino;

	bool operator==(const DevNode& v) const
	{
		return f_dev == v.f_dev && f_ino == v.f_ino;
	}

	bool operator>(const DevNode& v) const
	{
		return f_dev > v.f_dev ? true :
			   f_dev < v.f_dev ? false :
			   f_ino > v.f_ino;
	}

	const DevNode& operator=(const DevNode& v)
	{
		f_dev = v.f_dev;
		f_ino = v.f_ino;
		return *this;
	}
};

class CountedFd
{
public:
	CountedFd(int f)
		: fd(f), useCount(0)
	{ }

	~CountedFd()
	{
		fb_assert(useCount == 0);
	}

	int fd;
	int useCount;

private:
	CountedFd(const CountedFd&);
	const CountedFd& operator=(const CountedFd&);
};


	typedef GenericMap<Pair<Left<string, CountedRWLock*> > > RWLocks;
	GlobalPtr<RWLocks> rwlocks;
	GlobalPtr<Mutex> rwlocksMutex;

	typedef GenericMap<Pair<NonPooled<DevNode, CountedFd*> > > FdNodes;
	GlobalPtr<FdNodes> fdNodes;
	GlobalPtr<Mutex> fdNodesMutex;

	DevNode getNode(const char* name)
	{
		struct stat statistics;
		if (stat(name, &statistics) != 0)
		{
			if (errno == ENOENT)
			{
				//file not found
				return DevNode();
			}

			system_call_failed::raise("stat");
		}

		return DevNode(statistics.st_dev, statistics.st_ino);
	}

	DevNode getNode(int fd)
	{
		struct stat statistics;
		if (fstat(fd, &statistics) != 0)
		{
			system_call_failed::raise("stat");
		}

		return DevNode(statistics.st_dev, statistics.st_ino);
	}

	int openFile(const char* fileName)
	{
		MutexLockGuard g(fdNodesMutex);

		DevNode id(getNode(fileName));
		CountedFd** cFd = NULL;
		if (id.f_ino)
		{
			cFd = fdNodes->get(id);
		}

		if (!cFd)
		{
			int fd = os_utils::openCreateSharedFile(fileName, 0);
			CountedFd* newFile = FB_NEW(*getDefaultMemoryPool()) CountedFd(fd);
			cFd = fdNodes->put(getNode(fd));
			fb_assert(cFd);
			*cFd = newFile;
		}

		++((*cFd)->useCount);

		return (*cFd)->fd;
	}

	void closeFile(int fd)
	{
		MutexLockGuard g(fdNodesMutex);

		CountedFd** cFd = fdNodes->get(getNode(fd));
		fb_assert(cFd);

		if (--((*cFd)->useCount) == 0)
		{
			DevNode id(getNode(fd));
			close((*cFd)->fd);
			delete *cFd;
			fdNodes->remove(id);
		}
	}

	const char* NAME = "fcntl";
#else
	int openFile(const char* fileName)
	{
		return os_utils::openCreateSharedFile(fileName, 0);
	}

	void closeFile(int fd)
	{
		close(fd);
	}

	const char* NAME = "flock";
#endif // USE_FCNTL

	// File lock holder
	class FileLock
	{
	public:
		enum LockLevel {LCK_NONE, LCK_SHARED, LCK_EXCL};
		enum DtorMode {CLOSED, OPENED, LOCKED};
		enum LockMode {FLM_EXCLUSIVE, FLM_TRY_EXCLUSIVE, FLM_SHARED, FLM_TRY_SHARED};

		FileLock(int pFd, DtorMode pMode = CLOSED, int s = 0)
			: level(LCK_NONE), fd(pFd), dtorMode(pMode)
#ifdef USE_FCNTL
			, start(s), rwcl(getRw())
#endif
		{ }

		~FileLock()
		{
			if (dtorMode != LOCKED)
			{
				unlock();

#ifdef USE_FCNTL
				MutexLockGuard g(rwlocksMutex);
#ifdef DEBUG_RWLOCKS
				fprintf(stderr, "\n-- %p\n", rwcl);
#endif
				if (--(rwcl->cnt) == 0)
				{
#ifdef DEBUG_RWLOCKS
					fprintf(stderr, "\nremove %p\n", rwcl);
#endif
					rwlocks->remove(getLockId());
					delete rwcl;
				}
#else /* USE_FCNTL */
				unlock();
#endif /* USE_FCNTL */
			}
			else
			{
#ifdef DEBUG_RWLOCKS
				fprintf(stderr, "\n~leave locked %p\n", rwcl);
#endif
			}

			if (dtorMode == CLOSED)
			{
				closeFile(fd);
			}
		}

		void rwUnlock()
		{
#ifdef USE_FCNTL
			fb_assert(level != LCK_NONE);

			try
			{
				if (level == LCK_SHARED)
					rwcl->rwlock.endRead();
				else
					rwcl->rwlock.endWrite();
			}
			catch(const Exception& ex)
			{
				iscLogException("rwlock end-operation error", ex);
			}
#endif /* USE_FCNTL */

			level = LCK_NONE;
		}

		// unlocking can only put error into log file - we can't throw in dtors
		void unlock()
		{
			if (level == LCK_NONE)
			{
				return;
			}

#ifdef USE_FCNTL
			// For shared lock we must take into an account reenterability
			MutexEnsureUnlock guard(rwcl->sharedAccessMutex);
			if (level == LCK_SHARED)
			{
				guard.enter();

				fb_assert(rwcl->sharedAccessCounter > 0);
				if (--(rwcl->sharedAccessCounter) > 0)
				{
					// counter is non-zero - we must keep file lock
					rwUnlock();
					return;
				}
			}

			struct flock lock;
			lock.l_type = F_UNLCK;
			lock.l_whence = SEEK_SET;
			lock.l_start = start;
			lock.l_len = 1;

			if (fcntl(fd, F_SETLK, &lock) != 0)
			{
#else
			if (flock(fd, LOCK_UN) != 0)
			{
#endif
				ISC_STATUS_ARRAY local;
				error(local, NAME, errno);
				iscLogStatus("Unlock error", local);
			}

			rwUnlock();
		}

		// Main method to lock file
		int doLock(const LockMode mode)
		{
			bool shared = true, wait = true;
			switch(mode)
			{
				case FLM_TRY_EXCLUSIVE:
					wait = false;
					// fall through
				case FLM_EXCLUSIVE:
					shared = false;
					break;
				case FLM_TRY_SHARED:
					wait = false;
					// fall through
				case FLM_SHARED:
					break;
			}

			const LockLevel newLevel = shared ? LCK_SHARED : LCK_EXCL;
			if (newLevel == level)
			{
				return 0;
			}
			if (level != LCK_NONE)
			{
				unlock();
			}

#ifdef USE_FCNTL
			bool rc = true;
			try
			{
				switch(mode)
				{
				case FLM_TRY_EXCLUSIVE:
					rc = rwcl->rwlock.tryBeginWrite();
					break;
				case FLM_EXCLUSIVE:
					rwcl->rwlock.beginWrite();
					break;
				case FLM_TRY_SHARED:
					rc = rwcl->rwlock.tryBeginRead();
					break;
				case FLM_SHARED:
					rwcl->rwlock.beginRead();
					break;
				}
			}
			catch(const system_call_failed& fail)
			{
				return fail.getErrorCode();
			}
			if (!rc)
			{
				return -1;
			}

			// For shared lock we must take into an account reenterability
			MutexEnsureUnlock guard(rwcl->sharedAccessMutex);
			if (shared)
			{
				if (wait)
				{
					guard.enter();
				}
				else if (!guard.tryEnter())
				{
					return -1;
				}

				fb_assert(rwcl->sharedAccessCounter >= 0);
				if (rwcl->sharedAccessCounter++ > 0)
				{
					// counter is non-zero - we already have file lock
					level = LCK_SHARED;
					return 0;
				}
			}

			struct flock lock;
			lock.l_type = shared ? F_RDLCK : F_WRLCK;
			lock.l_whence = SEEK_SET;
			lock.l_start = start;
			lock.l_len = 1;
			if (fcntl(fd, wait ? F_SETLKW : F_SETLK, &lock) == -1)
			{
				int rc = errno;
				if ((!wait) && (rc == EACCES || rc == EAGAIN))
				{
					rc = -1;
				}

				try
				{
					if (newLevel == LCK_SHARED)
						rwcl->rwlock.endRead();
					else
						rwcl->rwlock.endWrite();
				}
				catch(const Exception&)
				{ }

				return rc;
#else
			if (flock(fd, (shared ? LOCK_SH : LOCK_EX) | (wait ? 0 : LOCK_NB)))
			{
				int rc = errno;
				if ((!wait) && (rc == EWOULDBLOCK))
				{
					rc = -1;
				}

				return rc;
#endif
			}
			level = newLevel;

			return 0;
		}

		// This file locker is using status vector to report errors
		bool doLock(ISC_STATUS* const status, const LockMode mode)
		{
			int rc = doLock(mode);
			if (rc != 0)
			{
				if (rc > 0)
				{
					error(status, NAME, rc);
				}
				return false;
			}

			return true;
		}

		// Call it to keep file locked & opened after dtor is called
		void setDtorMode(DtorMode mode) throw()
		{
			dtorMode = mode;
		}

		// Call when using already locked file in ctor
		void setLevel(LockLevel l)
		{
			level = l;
#ifdef USE_FCNTL
			if (level != LCK_NONE)
			{
				// was not done in dtor
				--(rwcl->cnt);
#ifdef DEBUG_RWLOCKS
				fprintf(stderr, "\n-- %p\n", rwcl);
#endif
			}
#endif
		}

	private:
		LockLevel level;
		int fd;
		DtorMode dtorMode;
#ifdef USE_FCNTL
		int start;
		CountedRWLock* rwcl;	// Due to order of init in ctor rwlock must go after fd & start

		string getLockId()
		{
			struct stat statistics;
			fstat(fd, &statistics);

			const size_t len1 = sizeof(statistics.st_dev);
			const size_t len2 = sizeof(statistics.st_ino);
			const size_t len3 = sizeof(int);

			string rc(len1 + len2 + len3, ' ');
			char* p = rc.begin();

			memcpy(p, &statistics.st_dev, len1);
			p += len1;
			memcpy(p, &statistics.st_ino, len2);
			p += len2;
			memcpy(p, &start, len3);

			return rc;
		}

		CountedRWLock* getRw()
		{
			string id = getLockId();
			CountedRWLock* rc = NULL;

			MutexLockGuard g(rwlocksMutex);

			CountedRWLock** got = rwlocks->get(id);
			if (got)
			{
				rc = *got;
			}

			if (!rc)
			{
				rc = FB_NEW(*getDefaultMemoryPool()) CountedRWLock;
				CountedRWLock** put = rwlocks->put(id);
				fb_assert(put);
				*put = rc;
			}

			++(rc->cnt);

			return rc;
		}
#endif
	};
}

#ifdef USE_SYS5SEMAPHORE

// Uncomment to trace details of event_init/fini calls
//#define DEB_EVNT

static SLONG	create_semaphores(ISC_STATUS *, SLONG, int);

namespace {

	int fdSem = -1;
	int sharedCount = 0;
	int fd_init = -1;

	// this class is mapped into shared file
	class SemTable
	{
	public:
		const static int N_FILES = 128;
		const static int N_SETS = 256;
#if defined(DEV_BUILD)
		const static int SEM_PER_SET = 4;	// force multiple sets allocation
#else
		const static int SEM_PER_SET = 31;	// hard limit for some old systems, might set to 32
#endif
		const static unsigned char CURRENT_VERSION = 1;
		unsigned char version;

	private:
		int lastSet;

		struct
		{
			char name[MAXPATHLEN];
		} filesTable[N_FILES];

		struct
		{
			key_t semKey;
			int fileNum;
			SLONG mask;

			int get(int fNum)
			{
				if (fileNum == fNum && mask != 0)
				{
					for (int bit = 0; bit < SEM_PER_SET; ++bit)
					{
						if (mask & (1 << bit))
						{
							mask &= ~(1 << bit);
							return bit;
						}
					}
					// bad bits in mask ?
					mask = 0;
				}
				return -1;
			}

			int create(int fNum)
			{
				fileNum = fNum;
				mask = 1 << SEM_PER_SET;
				--mask;
				mask &= ~1;
				return 0;
			}

			void put(int bit)
			{
				// fb_assert(!(mask & (1 << bit)));
				mask |= (1 << bit);
			}
		} set[N_SETS];

	public:
		void cleanup(int fNum, bool release);

		key_t getKey(int semSet) const
		{
			fb_assert(semSet >= 0 && semSet < lastSet);

			return set[semSet].semKey;
		}

		void init()
		{
			if (sharedCount)
			{
				return;
			}

			ftruncate(fdSem, sizeof(*this));

			for (int i = 0; i < N_SETS; ++i)
			{
				if (set[i].fileNum > 0)
				{
					// may be some old data about really active semaphore sets?
					if (version == CURRENT_VERSION)
					{
						const int semId = semget(set[i].semKey, SEM_PER_SET, 0);
						if (semId > 0)
						{
							semctl(semId, 0, IPC_RMID);
						}
					}
					set[i].fileNum = 0;
				}
			}

			for (int i = 0; i < N_FILES; ++i)
			{
				filesTable[i].name[0] = 0;
			}

			version = CURRENT_VERSION;
			lastSet = 0;
		}

		bool get(int fileNum, Sys5Semaphore* sem)
		{
			// try to locate existing set
			int n;
			for (n = 0; n < lastSet; ++n)
			{
				const int semNum = set[n].get(fileNum);
				if (semNum >= 0)
				{
					sem->semSet = n;
					sem->semNum = semNum;
					return true;
				}
			}

			// create new set
			for (n = 0; n < lastSet; ++n)
			{
				if (set[n].fileNum <= 0)
				{
					break;
				}
			}

			if (n >= N_SETS)
			{
				fb_assert(false);	// Not supposed to overflow
				return false;
			}

			if (n >= lastSet)
			{
				lastSet = n + 1;
			}

			set[n].semKey = ftok(filesTable[fileNum - 1].name, n);
			sem->semSet = n;
			sem->semNum = set[n].create(fileNum);
			return true;
		}

		void put(Sys5Semaphore* sem)
		{
			fb_assert(sem->semSet >= 0 && sem->semSet < N_SETS);

			set[sem->semSet].put(sem->semNum);
		}

		int findFileByName(const PathName& name) const
		{
			// Get a file ID in filesTable.
			for (int fileId = 0; fileId < N_FILES; ++fileId)
			{
				if (name == filesTable[fileId].name)
				{
					return fileId + 1;
				}
			}

			// not found
			return 0;
		}

		int addFileByName(const PathName& name)
		{
			int id = findFileByName(name);
			if (id > 0)
			{
				return id;
			}

			// Get a file ID in filesTable.
			for (int fileId = 0; fileId < SemTable::N_FILES; ++fileId)
			{
				if (filesTable[fileId].name[0] == 0)
				{
					name.copyTo(filesTable[fileId].name, sizeof(filesTable[fileId].name));
					return fileId + 1;
				}
			}

			// not found
			fb_assert(false);
			return 0;
		}
	};

	SemTable* semTable = NULL;

	class SharedFile
	{
	public:
		SharedFile(const char* pName, int fd, UCHAR* address, int length)
			: fileNum(semTable->addFileByName(pName)), from(address), to(address + length)
#ifdef USE_FILELOCKS
				, mutex(FB_NEW(*getDefaultMemoryPool()) FileLock(fd, FileLock::OPENED, 1))
#endif
		{ }

		SharedFile() : fileNum(0), from(0), to(0)
#ifdef USE_FILELOCKS
												 , mutex(0)
#endif
		{ }

		class StorageGuard : public MutexLockGuard
		{
		public:
			StorageGuard()
				: MutexLockGuard(guardMutex)
			{ }

		private:
			static GlobalPtr<Mutex> guardMutex;
		};

		int getNum() const { return fileNum; }

		static SharedFile* locate(void* s)
		{
			const int n = getByAddress((UCHAR*) s);
			return n >= 0 ? sharedFiles[n] : 0;
		}

#ifdef DEB_EVNT
		struct AbsPtr
		{
			SLONG offset;
			int fn;
			bool bad() const
			{
				return offset < 0 || fn < 0;
			}
			AbsPtr()
				: offset(-1), fn(-1)
			{ }
			bool operator==(const AbsPtr& sec) const
			{
				return offset == sec.offset && fn == sec.fn;
			}
		};

		static AbsPtr absPtr(const void* s)
		{
			const int n = getByAddress((UCHAR*) s);
			AbsPtr rc;
			if (n >= 0)
			{
				rc.offset = (IPTR)s - (IPTR)(sharedFiles[n]->from);
				rc.fn = sharedFiles[n]->fileNum;
			}
			return rc;
		}
#endif // DEB_EVNT

		static void push(SharedFile* sf)
		{
			StorageGuard guard;
			IPC_TRACE(("+add SF with %p %p\n", sf->from, sf->to));
			sharedFiles.push(sf);
		}

		static void pop()
		{
			StorageGuard guard;
			SharedFile* sf = sharedFiles.pop();
			sf->mutexDestroy();
			IPC_TRACE(("-pop SF with %p %p\n", sf->from, sf->to));
			delete sf;
		}

		static void remove(void* s)
		{
			StorageGuard guard;
			int n = getByAddress((UCHAR*) s);
			if (n >= 0) {
				IPC_TRACE(("-rem SF with %p %p\n", sharedFiles[n]->from, sharedFiles[n]->to));
				sharedFiles[n]->mutexDestroy();
				delete sharedFiles[n];
				sharedFiles.remove(n);
			}
			else {
				IPC_TRACE(("-rem SF Failed\n"));
			}
		}

		static void remap(UCHAR* const from, UCHAR* to, int newLength, struct mtx** mtxPtr)
		{
			StorageGuard guard;
			for (unsigned int n = 0; n < sharedFiles.getCount(); ++n)
			{
				if (from == sharedFiles[n]->from)
				{
					if (mtxPtr)
					{
						UCHAR* m = (UCHAR*)(*mtxPtr);
						if (m >= sharedFiles[n]->from && m < sharedFiles[n]->to)
						{
							m = to + (m - sharedFiles[n]->from);
							*mtxPtr = (struct mtx*)m;
						}
					}
					sharedFiles[n]->from = to;
					sharedFiles[n]->to = to + newLength;
					return;
				}
			}
		}

#ifdef USE_FILELOCKS

		void mutexDestroy()
		{
			delete mutex;
			mutex = 0;
		}

		int mutexLock()
		{
			try
			{
				thrMutex.enter();
			}
			catch(const system_call_failed& ex)
			{
				return ex.getErrorCode();
			}

			fb_assert(mutex);
			int rc = mutex->doLock(FileLock::FLM_EXCLUSIVE);
			if (rc)
			{
				try
				{
					thrMutex.leave();
				}
				catch(const Exception& ex)
				{ }
			}
			return rc;
		}

		int mutexTryLock()
		{
			try
			{
				if (!thrMutex.tryEnter())
				{
					return EBUSY;
				}
			}
			catch(const system_call_failed& ex)
			{
				return ex.getErrorCode();
			}

			fb_assert(mutex);
			int rc = mutex->doLock(FileLock::FLM_TRY_EXCLUSIVE);
			if (rc)
			{
				try
				{
					thrMutex.leave();
				}
				catch(const Exception& ex)
				{ }
			}
			return rc;
		}

		int mutexUnlock()
		{
			try
			{
				thrMutex.leave();
			}
			catch(const system_call_failed& ex)
			{
				return ex.getErrorCode();
			}

			fb_assert(mutex);
			mutex->unlock();
			return 0;
		}

#else // USE_FILELOCKS

		void mutexDestroy()
		{ }

#endif // USE_FILELOCKS

		typedef Vector<SharedFile*, SemTable::N_FILES> Storage;

	private:
		int fileNum;
		const UCHAR* from;
		const UCHAR* to;
#ifdef USE_FILELOCKS
		Mutex thrMutex;
		FileLock* mutex;
#endif
		static Storage sharedFiles;

		static int getByAddress(UCHAR* const s)
		{
			StorageGuard guard;
			for (unsigned int n = 0; n < sharedFiles.getCount(); ++n)
			{
				if (s >= sharedFiles[n]->from && s < sharedFiles[n]->to)
				{
					return n;
				}
			}

			return -1;
		}
	};

	SharedFile::Storage SharedFile::sharedFiles;
	GlobalPtr<Mutex> SharedFile::StorageGuard::guardMutex;

	int idCache[SemTable::N_SETS];
	GlobalPtr<Mutex> idCacheMutex;

	void initCache()
	{
		MutexLockGuard guard(idCacheMutex);
		memset(idCache, 0xff, sizeof idCache);
	}

	void SemTable::cleanup(int fNum, bool release)
	{
		fb_assert(fNum > 0 && fNum <= N_FILES);

		if (release)
		{
			filesTable[fNum - 1].name[0] = 0;
		}

		MutexLockGuard guard(idCacheMutex);
		for (int n = 0; n < lastSet; ++n)
		{
			if (set[n].fileNum == fNum)
			{
				if (release)
				{
					Sys5Semaphore sem;
					sem.semSet = n;
					int id = sem.getId();
					if (id >= 0)
					{
						semctl(id, 0, IPC_RMID);
					}
					set[n].fileNum = -1;
				}
				idCache[n] = -1;
			}
		}
	}

	bool getSem5(Sys5Semaphore* sem)
	{
		ISC_STATUS_ARRAY status;

		// Lock init file.
		FileLock initLock(fd_init, FileLock::OPENED);
		if (!initLock.doLock(status, FileLock::FLM_EXCLUSIVE))
		{
			iscLogStatus("initLock.exclusive() failed", status);
			return false;
		}

		// Find out what file does it belong to.
		SharedFile* sf = SharedFile::locate(sem);
		if (!sf)
		{
			gds__log("SharedFile::locate(sem) failed");
			return false;
		}

		if (!semTable->get(sf->getNum(), sem))
		{
			gds__log("semTable->get() failed");
			return false;
		}

		return true;
	}

	void freeSem5(Sys5Semaphore* sem)
	{
		ISC_STATUS_ARRAY status;

		// Lock init file.
		FileLock initLock(fd_init, FileLock::OPENED);
		if (!initLock.doLock(status, FileLock::FLM_EXCLUSIVE))
		{
			iscLogStatus("freeSem5 failed to lock init file", status);
			return;
		}

		semTable->put(sem);
	}

#ifdef DEB_EVNT

	struct Dump
	{
		SharedFile::AbsPtr e;
		enum
		{
			WORKING, INIT, FINI, INIT_ERR
		} state;
		int code;
	};
	GlobalPtr<Array<Dump> > dump;
	GlobalPtr<Mutex> dMutex;

	Dump* dFind(const event_t* ev)
	{
		SharedFile::AbsPtr e = SharedFile::absPtr(ev);
		fb_assert(!e.bad());
		for (Dump* itr = dump->begin(); itr < dump->end(); ++itr)
		{
			if (itr->e == e)
			{
				return itr;
			}
		}
		return NULL;
	}

	void initStart(const event_t* e)
	{
		MutexLockGuard g(dMutex);
		Dump* d = dFind(e);
		if (d)
		{
			fb_assert(d->state == Dump::INIT_ERR);
			d->state = Dump::INIT;
		}
		else
		{
			Dump dd;
			dd.e = SharedFile::absPtr(e);
			fb_assert(!dd.e.bad());
			dd.state = Dump::INIT;
			dd.code = 0;
			dump->add(dd);
		}
	}

	void initStop(const event_t* e, int code)
	{
		MutexLockGuard g(dMutex);
		Dump* d = dFind(e);

		fb_assert(d && (d->state == Dump::INIT));
		d->state = code ? Dump::INIT_ERR : Dump::WORKING;
		d->code = code;
	}

	void finiStart(const event_t* e)
	{
		MutexLockGuard g(dMutex);
		Dump* d = dFind(e);

		fb_assert(d && (d->state == Dump::WORKING));
		d->state = Dump::FINI;
	}

	void finiStop(const event_t* e)
	{
		MutexLockGuard g(dMutex);
		Dump* d = dFind(e);

		fb_assert(d && (d->state == Dump::FINI));
		dump->remove(d);
	}

#else // DEB_EVNT

	void initStart(const event_t* event) {}
	void initStop(const event_t* event, int code) {}
	void finiStart(const event_t* event) {}
	void finiStop(const event_t* event) {}

#endif // DEB_EVNT

}

int Sys5Semaphore::getId()
{
	MutexLockGuard guard(idCacheMutex);
	fb_assert(semSet >= 0 && semSet < SemTable::N_SETS);

	int id = idCache[semSet];

	if (id < 0)
	{
		ISC_STATUS_ARRAY status;
		id = create_semaphores(status, semTable->getKey(semSet), SemTable::SEM_PER_SET);
		if (id >= 0)
		{
			idCache[semSet] = id;
		}
		else
		{
			iscLogStatus("create_semaphores failed:", status);
		}
	}

	return id;
}
#endif // USE_SYS5SEMAPHORE

#endif // UNIX

#if defined(WIN_NT)
static bool make_object_name(TEXT*, size_t, const TEXT*, const TEXT*);
#endif


static bool event_blocked(const event_t* event, const SLONG value)
{
/**************************************
 *
 *	e v e n t _ b l o c k e d
 *
 **************************************
 *
 * Functional description
 *	If a wait would block, return true.
 *
 **************************************/

	if (event->event_count >= value)
	{
#ifdef DEBUG_ISC_SYNC
		printf("event_blocked: FALSE (eg something to report)\n");
		fflush(stdout);
#endif
		return false;
	}

#ifdef DEBUG_ISC_SYNC
	printf("event_blocked: TRUE (eg nothing happened yet)\n");
	fflush(stdout);
#endif
	return true;
}


#ifdef USE_POSIX_THREADS

#ifdef USE_SYS5SEMAPHORE

namespace {

GlobalPtr<Mutex> timerAccess;
GlobalPtr<Semaphore> timerWakeup, timerFini;

void stopTimers(void*);
bool stopThread = false;

struct TimerEntry
{
	SINT64 fireTime;
	int semId;
	USHORT semNum;

	static const SINT64& generate(const void* /*sender*/, const TimerEntry& item) { return item.fireTime; }
	static THREAD_ENTRY_DECLARE timeThread(THREAD_ENTRY_PARAM);

	static void init()
	{
		int rc = gds__thread_start(timeThread, 0, 0, 0, 0);
		if (rc != 0)
		{
			fatal_exception::raiseFmt("Error starting timer thread");
		}
		gds__register_cleanup(stopTimers, 0);
	}

	static void cleanup()
	{
		stopThread = true;
		timerWakeup->release();
		timerFini->enter();
	}
};

typedef SortedArray<TimerEntry, InlineStorage<TimerEntry, 64>, SINT64, TimerEntry> TimerQueue;
GlobalPtr<TimerQueue> timerQueue;

InitMutex<TimerEntry> timerHolder;

SINT64 curTime()
{
	struct timeval cur_time;
	struct timezone tzUnused;

	if (gettimeofday(&cur_time, &tzUnused) != 0)
	{
		system_call_failed::raise("gettimeofday");
	}

	SINT64 timeout = ((SINT64) cur_time.tv_sec) * 1000000 + cur_time.tv_usec;
	return timeout;
}

SINT64 addTimer(Sys5Semaphore* sem, int microSeconds)
{
	timerHolder.init();

	TimerEntry newTimer;
	newTimer.fireTime = curTime() + microSeconds;
	newTimer.semId = sem->getId();
	newTimer.semNum = sem->semNum;

	MutexLockGuard guard(timerAccess);

	timerQueue->add(newTimer);
	timerWakeup->release();

	return newTimer.fireTime;
}

void delTimer(Sys5Semaphore* sem)
{
	const int id = sem->getId();

	MutexLockGuard guard(timerAccess);

	for (unsigned int i = 0; i < timerQueue->getCount(); ++i)
	{
		const TimerEntry& e(timerQueue->operator[](i));
		if (e.semNum == sem->semNum && e.semId == id)
		{
			timerQueue->remove(i);
			return;
		}
	}
}

void stopTimers(void*)
{
	timerHolder.cleanup();
}

THREAD_ENTRY_DECLARE TimerEntry::timeThread(THREAD_ENTRY_PARAM)
{
	while (!stopThread)
	{
		int microSeconds = 0;
		{
			MutexLockGuard guard(timerAccess);

			const SINT64 cur = curTime();
			while (timerQueue->getCount() > 0)
			{
				const TimerEntry& e(timerQueue->operator[](0));
				if (e.fireTime <= cur)
				{
					for (;;)
					{
						union semun arg;
						arg.val = 0;
						int ret = semctl(e.semId, e.semNum, SETVAL, arg);
						if (ret != -1)
							break;
						if (!SYSCALL_INTERRUPTED(errno))
						{
							break;
						}
					}
					timerQueue->remove((size_t) 0);
				}
				else
				{
					microSeconds = e.fireTime - cur;
					break;
				}
			}
		}

		if (microSeconds)
		{
			timerWakeup->tryEnter(0, microSeconds / 1000);
		}
		else
		{
			timerWakeup->enter();
		}
	}

	timerFini->release();
	return 0;
}

} // namespace


SLONG ISC_event_clear(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ c l e a r	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Clear an event preparatory to waiting on it.  The order of
 *	battle for event synchronization is:
 *
 *	    1.  Clear event.
 *	    2.  Test data structure for event already completed
 *	    3.  Wait on event.timerQueue[0].
 *
 **************************************/
	union semun arg;

	arg.val = 1;
	if (semctl(event->getId(), event->semNum, SETVAL, arg) < 0)
	{
		iscLogStatus("event_clear()", (Arg::Gds(isc_sys_request) << Arg::Str("semctl") << SYS_ERR(errno)).value());
	}

	return (event->event_count + 1);
}


void ISC_event_fini(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ f i n i	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Discard an event object.
 *
 **************************************/
	IPC_TRACE(("ISC_event_fini set=%d num=%d\n", event->semSet, event->semNum));
	finiStart(event);
	freeSem5(event);
	finiStop(event);
}


int ISC_event_init(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ i n i t	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Prepare an event object for use.
 *
 **************************************/
	initStart(event);

	event->event_count = 0;

	if (!getSem5(event))
	{
		IPC_TRACE(("ISC_event_init failed get sem %p\n", event));
		initStop(event, 1);
		return FB_FAILURE;
	}

	IPC_TRACE(("ISC_event_init set=%d num=%d\n", event->semSet, event->semNum));

	union semun arg;
	arg.val = 0;
	if (semctl(event->getId(), event->semNum, SETVAL, arg) < 0)
	{
		iscLogStatus("event_init()", (Arg::Gds(isc_sys_request) << Arg::Str("semctl") << SYS_ERR(errno)).value());
		initStop(event, 2);
		return FB_FAILURE;
	}

	initStop(event, 0);
	return FB_SUCCESS;
}


int ISC_event_post(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ p o s t	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Post an event to wake somebody else up.
 *
 **************************************/
	union semun arg;

	++event->event_count;

	for (;;)
	{
		arg.val = 0;
		int ret = semctl(event->getId(), event->semNum, SETVAL, arg);
		if (ret != -1)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
		{
			gds__log("ISC_event_post: semctl failed with errno = %d", errno);
			return FB_FAILURE;
		}
	}

	return FB_SUCCESS;
}


int ISC_event_wait(event_t*	event,
				   SLONG	value,
				   const SLONG	micro_seconds)
{
/**************************************
 *
 *	I S C _ e v e n t _ w a i t	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Wait on an event.  If timeout limit specified, return
 *	anyway after the timeout even if no event has
 *	happened.  If returning due to timeout, return
 *	FB_FAILURE else return FB_SUCCESS.
 *
 **************************************/
	//sigset_t mask, oldmask;

/* If we're not blocked, the rest is a gross waste of time */
	if (!event_blocked(event, value))
		return FB_SUCCESS;

/* Set up timers if a timeout period was specified. */

	//struct itimerval user_timer;
	//struct sigaction user_handler;
	//struct timeval cur_time;
	//struct timezone tzUnused;
	SINT64 timeout = 0;
	if (micro_seconds > 0)
	{
		timeout = addTimer(event, micro_seconds);
	}

/* Go into wait loop */

	int ret = FB_SUCCESS;
	for (;;)
	{
		if (!event_blocked(event, value))
			break;

		struct sembuf sb;
		sb.sem_op = 0;
		sb.sem_flg = 0;
		sb.sem_num = event->semNum;

		int rc = semop(event->getId(), &sb, 1);
		if (rc == -1 && !SYSCALL_INTERRUPTED(errno))
		{
			gds__log("ISC_event_wait: semop failed with errno = %d", errno);
		}

		if (micro_seconds > 0)
		{
			// distinguish between timeout and actually happened event
			if (! event_blocked(event, value))
				break;

			// had timeout expired?
			if (curTime() >= timeout)	// really expired
			{
				ret = FB_FAILURE;
				break;
			}
		}
	}

/* Cancel the handler.  We only get here if a timeout was specified. */
	if (micro_seconds > 0)
	{
		delTimer(event);
	}

	return ret;
}

#else //not USE_SYS5SEMAPHORE

namespace {
int isPthreadError(int rc, const char* function)
{
	if (rc == 0)
		return 0;
	iscLogStatus("Pthread Error", (Arg::Gds(isc_sys_request) << Arg::Str(function) << Arg::Unix(rc)).value());
	return rc;
}
}
#define PTHREAD_ERROR(x) if (isPthreadError((x), #x)) return FB_FAILURE
#define PTHREAD_ERRNO(x) { int tmpState = (x); if (isPthreadError(tmpState, #x)) return tmpState; }
#define LOG_PTHREAD_ERROR(x) isPthreadError((x), #x)

SLONG ISC_event_clear(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ c l e a r	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Clear an event preparatory to waiting on it.  The order of
 *	battle for event synchronization is:
 *
 *	    1.  Clear event.
 *	    2.  Test data structure for event already completed
 *	    3.  Wait on event.
 *
 **************************************/
	LOG_PTHREAD_ERROR(pthread_mutex_lock(event->event_mutex));
	const SLONG ret = event->event_count + 1;
	LOG_PTHREAD_ERROR(pthread_mutex_unlock(event->event_mutex));
	return ret;
}


void ISC_event_fini(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ f i n i	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Discard an event object.
 *
 **************************************/

	if (event->pid == getpid())
	{
		LOG_PTHREAD_ERROR(pthread_mutex_destroy(event->event_mutex));
		LOG_PTHREAD_ERROR(pthread_cond_destroy(event->event_cond));
	}
}


int ISC_event_init(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ i n i t	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Prepare an event object for use.
 *
 **************************************/
	event->event_count = 0;
	event->pid = getpid();

	/* Prepare an Inter-Process event block */
	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;

	PTHREAD_ERROR(pthread_mutexattr_init(&mattr));
	PTHREAD_ERROR(pthread_condattr_init(&cattr));
#ifndef USE_LOCAL_MUTEXES
#ifdef PTHREAD_PROCESS_SHARED
	PTHREAD_ERROR(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED));
	PTHREAD_ERROR(pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED));
#else
#error Your system must support PTHREAD_PROCESS_SHARED to use firebird.
#endif
#endif
	PTHREAD_ERROR(pthread_mutex_init(event->event_mutex, &mattr));
	PTHREAD_ERROR(pthread_cond_init(event->event_cond, &cattr));
	PTHREAD_ERROR(pthread_mutexattr_destroy(&mattr));
	PTHREAD_ERROR(pthread_condattr_destroy(&cattr));

	return FB_SUCCESS;
}


int ISC_event_post(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ p o s t	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Post an event to wake somebody else up.
 *
 **************************************/
	PTHREAD_ERROR(pthread_mutex_lock(event->event_mutex));
	++event->event_count;
	const int ret = pthread_cond_broadcast(event->event_cond);
	PTHREAD_ERROR(pthread_mutex_unlock(event->event_mutex));
	if (ret)
	{
		gds__log ("ISC_event_post: pthread_cond_broadcast failed with errno = %d", ret);
		return FB_FAILURE;
	}

	return FB_SUCCESS;
}


int ISC_event_wait(event_t* event,
				   const SLONG value,
				   const SLONG micro_seconds)
{
/**************************************
 *
 *	I S C _ e v e n t _ w a i t	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Wait on an event.  If timeout limit specified, return
 *	anyway after the timeout even if no event has
 *	happened.  If returning due to timeout, return
 *	FB_FAILURE else return FB_SUCCESS.
 *
 **************************************/

/* If we're not blocked, the rest is a gross waste of time */

	if (!event_blocked(event, value))
		return FB_SUCCESS;

/* Set up timers if a timeout period was specified. */

	struct timespec timer;
	if (micro_seconds > 0)
	{
		timer.tv_sec = time(NULL);
		timer.tv_sec += micro_seconds / 1000000;
		timer.tv_nsec = 1000 * (micro_seconds % 1000000);
	}

	int ret = FB_SUCCESS;
	pthread_mutex_lock(event->event_mutex);
	for (;;)
	{
		if (!event_blocked(event, value))
		{
			ret = FB_SUCCESS;
			break;
		}

		/* The Posix pthread_cond_wait & pthread_cond_timedwait calls
		   atomically release the mutex and start a wait.
		   The mutex is reacquired before the call returns.
		 */
		if (micro_seconds > 0)
		{
			ret = pthread_cond_timedwait(event->event_cond, event->event_mutex, &timer);

#if (defined LINUX || defined DARWIN || defined HP11 || defined FREEBSD)
			if (ret == ETIMEDOUT)
#else
			if (ret == ETIME)
#endif
			{

				/* The timer expired - see if the event occurred and return
				   FB_SUCCESS or FB_FAILURE accordingly. */

				if (event_blocked(event, value))
					ret = FB_FAILURE;
				else
					ret = FB_SUCCESS;
				break;
			}
		}
		else
			ret = pthread_cond_wait(event->event_cond, event->event_mutex);
	}
	pthread_mutex_unlock(event->event_mutex);
	return ret;
}

#endif // USE_SYS5SEMAPHORE
#endif // USE_POSIX_THREADS


#ifdef WIN_NT
SLONG ISC_event_clear(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ c l e a r	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Clear an event preparatory to waiting on it.  The order of
 *	battle for event synchronization is:
 *
 *	    1.  Clear event.
 *	    2.  Test data structure for event already completed
 *	    3.  Wait on event.
 *
 **************************************/

	ResetEvent((HANDLE) event->event_handle);

	return event->event_count + 1;
}


void ISC_event_fini(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ f i n i	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Discard an event object.
 *
 **************************************/

	if (event->event_pid == process_id)
	{
		CloseHandle((HANDLE) event->event_handle);
	}
}


int ISC_event_init(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ i n i t	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Prepare an event object for use.
 *
 **************************************/

#ifdef SUPERSERVER
	event->event_id = 0;
#else
	static int idCounter = 0;		// Should it be AtomicCounter? AP-2008

	event->event_id = ++idCounter;
#endif

	event->event_pid = process_id = getpid();
	event->event_count = 0;

	event->event_handle = ISC_make_signal(true, true, process_id, event->event_id);

	return (event->event_handle) ? FB_SUCCESS : FB_FAILURE;
}


int ISC_event_post(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ p o s t	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Post an event to wake somebody else up.
 *
 **************************************/

	++event->event_count;

	if (event->event_pid != process_id)
		return ISC_kill(event->event_pid, event->event_id, event->event_handle);

	return SetEvent((HANDLE) event->event_handle) ? FB_SUCCESS : FB_FAILURE;
}


int ISC_event_wait(event_t* event,
				   const SLONG value,
				   const SLONG micro_seconds)
{
/**************************************
 *
 *	I S C _ e v e n t _ w a i t	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Wait on an event.
 *
 **************************************/
	/* If we're not blocked, the rest is a gross waste of time */

	if (!event_blocked(event, value)) {
		return FB_SUCCESS;
	}

	/* Go into wait loop */

	const DWORD timeout = (micro_seconds > 0) ? micro_seconds / 1000 : INFINITE;

	for (;;)
	{
		if (!event_blocked(event, value)) {
			return FB_SUCCESS;
		}

		const DWORD status = WaitForSingleObject(event->event_handle, timeout);

		if (status != WAIT_OBJECT_0)
		{
			return FB_FAILURE;
		}
	}
}

#endif // WIN_NT


#ifdef UNIX
void ISC_exception_post(ULONG sig_num, const TEXT* err_msg)
{
/**************************************
 *
 *	I S C _ e x c e p t i o n _ p o s t ( U N I X )
 *
 **************************************
 *
 * Functional description
 *     When we got a sync exception, fomulate the error code
 *     write it to the log file, and abort.
 *
 * 08-Mar-2004, Nickolay Samofatov.
 *   This function is dangerous and requires rewrite using signal-safe operations only.
 *   Main problem is that we call a lot of signal-unsafe functions from this signal handler,
 *   examples are gds__alloc, gds__log, etc... sprintf is safe on some BSD platforms,
 *   but not on Linux. This may result in lock-up during signal handling.
 *
 **************************************/
	// If there's no err_msg, we asumed the switch() finds no case or we crash.
	// Too much goodwill put on the caller. Weak programming style.
	// Therefore, lifted this safety net from the NT version.
	if (!err_msg)
	{
		err_msg = "";
	}

	TEXT* const log_msg = (TEXT *) gds__alloc(strlen(err_msg) + 256);
	// NOMEM: crash!
	log_msg[0] = '\0';

	switch (sig_num)
	{
	case SIGSEGV:
		sprintf(log_msg, "%s Segmentation Fault.\n"
				"\t\tThe code attempted to access memory\n"
				"\t\twithout privilege to do so.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case SIGBUS:
		sprintf(log_msg, "%s Bus Error.\n"
				"\t\tThe code caused a system bus error.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case SIGILL:

		sprintf(log_msg, "%s Illegal Instruction.\n"
				"\t\tThe code attempted to perfrom an\n"
				"\t\tillegal operation."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;

	case SIGFPE:
		sprintf(log_msg, "%s Floating Point Error.\n"
				"\t\tThe code caused an arithmetic exception\n"
				"\t\tor floating point exception."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	default:
		sprintf(log_msg, "%s Unknown Exception.\n"
				"\t\tException number %"ULONGFORMAT"."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg, sig_num);
		break;
	}

	if (err_msg)
	{
		gds__log(log_msg);
		gds__free(log_msg);
	}
	abort();
}
#endif // UNIX


#ifdef WIN_NT
ULONG ISC_exception_post(ULONG except_code, const TEXT* err_msg)
{
/**************************************
 *
 *	I S C _ e x c e p t i o n _ p o s t ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *     When we got a sync exception, fomulate the error code
 *     write it to the log file, and abort. Note: We can not
 *     actually call "abort" since in windows this will cause
 *     a dialog to appear stating the obvious!  Since on NT we
 *     would not get a core file, there is actually no difference
 *     between abort() and exit(3).
 *
 **************************************/
	ULONG result = 0;
	bool is_critical = true;

	if (!err_msg)
	{
		err_msg = "";
	}

	TEXT* log_msg = (TEXT*) gds__alloc(strlen(err_msg) + 256);
	// NOMEM: crash!
	log_msg[0] = '\0';

	switch (except_code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		sprintf(log_msg, "%s Access violation.\n"
				"\t\tThe code attempted to access a virtual\n"
				"\t\taddress without privilege to do so.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		sprintf(log_msg, "%s Datatype misalignment.\n"
				"\t\tThe attempted to read or write a value\n"
				"\t\tthat was not stored on a memory boundary.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		sprintf(log_msg, "%s Array bounds exceeded.\n"
				"\t\tThe code attempted to access an array\n"
				"\t\telement that is out of bounds.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		sprintf(log_msg, "%s Float denormal operand.\n"
				"\t\tOne of the floating-point operands is too\n"
				"\t\tsmall to represent as a standard floating-point\n"
				"\t\tvalue.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		sprintf(log_msg, "%s Floating-point divide by zero.\n"
				"\t\tThe code attempted to divide a floating-point\n"
				"\t\tvalue by a floating-point divisor of zero.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		sprintf(log_msg, "%s Floating-point inexact result.\n"
				"\t\tThe result of a floating-point operation cannot\n"
				"\t\tbe represented exactly as a decimal fraction.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		sprintf(log_msg, "%s Floating-point invalid operand.\n"
				"\t\tAn indeterminant error occurred during a\n"
				"\t\tfloating-point operation.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_OVERFLOW:
		sprintf(log_msg, "%s Floating-point overflow.\n"
				"\t\tThe exponent of a floating-point operation\n"
				"\t\tis greater than the magnitude allowed.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		sprintf(log_msg, "%s Floating-point stack check.\n"
				"\t\tThe stack overflowed or underflowed as the\n"
				"result of a floating-point operation.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		sprintf(log_msg, "%s Floating-point underflow.\n"
				"\t\tThe exponent of a floating-point operation\n"
				"\t\tis less than the magnitude allowed.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		sprintf(log_msg, "%s Integer divide by zero.\n"
				"\t\tThe code attempted to divide an integer value\n"
				"\t\tby an integer divisor of zero.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_INT_OVERFLOW:
		sprintf(log_msg, "%s Interger overflow.\n"
				"\t\tThe result of an integer operation caused the\n"
				"\t\tmost significant bit of the result to carry.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case EXCEPTION_STACK_OVERFLOW:
		Firebird::status_exception::raise(Firebird::Arg::Gds(isc_exception_stack_overflow));
		/* This will never be called, but to be safe it's here */
		result = (ULONG) EXCEPTION_CONTINUE_EXECUTION;
		is_critical = false;
		break;

	case EXCEPTION_BREAKPOINT:
	case EXCEPTION_SINGLE_STEP:
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
	case EXCEPTION_INVALID_DISPOSITION:
	case EXCEPTION_PRIV_INSTRUCTION:
	case EXCEPTION_IN_PAGE_ERROR:
	case EXCEPTION_ILLEGAL_INSTRUCTION:
	case EXCEPTION_GUARD_PAGE:
		/* Pass these exception on to someone else,
		   probably the OS or the debugger, since there
		   isn't a dam thing we can do with them */
		result = EXCEPTION_CONTINUE_SEARCH;
		is_critical = false;
		break;
	case 0xE06D7363: /* E == Exception. 0x6D7363 == "msc". Intel and Borland use the same code to be compatible */
		/* If we've caught our own software exception,
		   continue rewinding the stack to properly handle it
		   and deliver an error information to the client side */
		result = EXCEPTION_CONTINUE_SEARCH;
		is_critical = false;
		break;
	default:
		sprintf (log_msg, "%s An exception occurred that does\n"
				"\t\tnot have a description.  Exception number %"XLONGFORMAT".\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg, except_code);
		break;
	}

	if (is_critical)
	{
		gds__log(log_msg);
	}

	gds__free(log_msg);

	if (is_critical)
	{
		if (Config::getBugcheckAbort())
		{
			// Pass exception to outer handler in case debugger is present to collect memory dump
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// Silently exit so guardian or service manager can restart the server.
		// If exception is getting out of the application Windows displays a message
		// asking if you want to send report to Microsoft or attach debugger,
		// application is not terminated until you press some button on resulting window.
		// This happens even if you run application as non-interactive service on
		// "server" OS like Windows Server 2003.
		exit(3);
	}

	return result;
}
#endif // WIN_NT

void ISC_remove_map_file(const TEXT* filename)
{
#ifndef WIN_NT
	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, filename);

	// We can't do much (specially in dtors) when it fails
	// therefore do not check for errors - at least it's just /tmp.
	unlink(expanded_filename);
#endif // WIN_NT
}

void ISC_remove_map_file(const struct sh_mem* shmem_data)
{
	ISC_remove_map_file(shmem_data->sh_mem_name);
}

#ifdef UNIX

UCHAR* ISC_map_file(ISC_STATUS* status_vector,
					const TEXT* filename,
					FPTR_INIT_GLOBAL_REGION init_routine,
					void* init_arg,
					ULONG length,
					sh_mem* shmem_data)
{
/**************************************
 *
 *	I S C _ m a p _ f i l e		( U N I X - m m a p )
 *
 **************************************
 *
 * Functional description
 *	Try to map a given file.  If we are the first (i.e. only)
 *	process to map the file, call a given initialization
 *	routine (if given) or punt (leaving the file unmapped).
 *
 **************************************/

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, filename);

/* make the complete filename for the init file this file is to be used as a
   master lock to eliminate possible race conditions with just a single file
   locking. The race condition is caused as the conversion of a EXCLUSIVE
   lock to a LCK_SHARED lock is not atomic*/

	TEXT init_filename[MAXPATHLEN];
	gds__prefix_lock(init_filename, INIT_FILE);

	const bool trunc_flag = (length != 0);

/* open the init lock file */
	MutexLockGuard guard(openFdInit);

#ifdef USE_SYS5SEMAPHORE
	if (fd_init < 0)
#else
	int
#endif
		fd_init = openFile(init_filename);
	if (fd_init == -1)
	{
		error(status_vector, "open", errno);
		return NULL;
	}

/* get an exclusive lock on the INIT file with blocking */
	FileLock initLock(fd_init);
#ifdef USE_SYS5SEMAPHORE
	initLock.setDtorMode(FileLock::OPENED);
#endif
	if (!initLock.doLock(status_vector, FileLock::FLM_EXCLUSIVE))
		return NULL;

// init file is locked - no races possible later in this function

#ifdef USE_SYS5SEMAPHORE
	if (fdSem < 0)
	{
		TEXT sem_filename[MAXPATHLEN];
		gds__prefix_lock(sem_filename, SEM_FILE);
		const int f = openFile(sem_filename);
		if (f == -1)
		{
			error(status_vector, "open", errno);
			return NULL;
		}
		void* sTab = mmap(0, sizeof(SemTable), PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
		if ((U_IPTR) sTab == (U_IPTR) -1)
		{
			error(status_vector, "mmap", errno);
			return NULL;
		}

		fdSem = f;
		semTable = (SemTable*) sTab;
		initCache();
	}
	fb_assert(semTable);

	FileLock semLock(fdSem, FileLock::OPENED);

	if (semLock.doLock(status_vector, FileLock::FLM_TRY_EXCLUSIVE))
	{
		semTable->init();
	}
	if (!semLock.doLock(status_vector, FileLock::FLM_SHARED))
		return NULL;
#endif

/* open the file to be inited */
	const int fd = openFile(expanded_filename);
	if (fd == -1)
	{
		error(status_vector, "open", errno);
		return NULL;
	}

/* create lock in order to have file autoclosed on error */
	FileLock mainLock(fd);

	if (length == 0)
	{
		/* Get and use the existing length of the shared segment */
		struct stat file_stat;
		if (fstat(fd, &file_stat) == -1)
		{
			error(status_vector, "fstat", errno);
			return NULL;
		}
		length = file_stat.st_size;

		if (length == 0)
		{
			// keep old text of message here -  will be assigned a bit later
			error(status_vector, "shmem_data->sh_mem_length_mapped is 0", 0);
			return NULL;
		}
	}

/* map file to memory */
	UCHAR* const address = (UCHAR *) mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((U_IPTR) address == (U_IPTR) -1)
	{
		error(status_vector, "mmap", errno);
		return NULL;
	}

	shmem_data->sh_mem_address = address;
	shmem_data->sh_mem_length_mapped = length;
	shmem_data->sh_mem_handle = fd;
	strcpy(shmem_data->sh_mem_name, filename);

#ifdef USE_SYS5SEMAPHORE
	// register mapped file
	// this class is needed to auto-unregister it in case of failure
	class sfHolder
	{
	public:
		explicit sfHolder(SharedFile* sf) : pop(true)
		{
			SharedFile::push(sf);
		}

		void materialize()
		{
			pop = false;
		}

		~sfHolder()
		{
			if (pop)
			{
				SharedFile::pop();
			}
		}
	private:
		bool pop;
	};
	sfHolder holder(FB_NEW(*getDefaultMemoryPool())
					SharedFile(expanded_filename, fd, address, length));
#endif


/* Try to get an exclusive lock on the lock file.  This will
   fail if somebody else has the exclusive or shared lock */

	if (mainLock.doLock(status_vector, FileLock::FLM_TRY_EXCLUSIVE))
	{
		if (!init_routine)
		{
			munmap((char *) address, length);
			Arg::Gds(isc_unavailable).copyTo(status_vector);
			return NULL;
		}

		if (trunc_flag)
			ftruncate(fd, length);

		(*init_routine) (init_arg, shmem_data, true);

		if (!mainLock.doLock(status_vector, FileLock::FLM_TRY_SHARED))
		{
			munmap((char *) address, length);
			return NULL;
		}
	}
	else
	{
		if (!mainLock.doLock(status_vector, FileLock::FLM_TRY_SHARED))
		{
			munmap((char *) address, length);
			return NULL;
		}

		if (init_routine)
			(*init_routine) (init_arg, shmem_data, false);
	}

/* keep opened the shared file_decriptor */
	mainLock.setDtorMode(FileLock::LOCKED);

#ifdef USE_SYS5SEMAPHORE
	// keep shared lock before last shared memory region unmapped
	semLock.setDtorMode(FileLock::LOCKED);
	++sharedCount;
	// keep registered mapped file permanently
	holder.materialize();
#endif

	return address;
}

#endif // UNIX


#ifdef WIN_NT
UCHAR* ISC_map_file(ISC_STATUS* status_vector,
					const TEXT* filename,
					FPTR_INIT_GLOBAL_REGION init_routine,
					void* init_arg,
					ULONG length,
					sh_mem* shmem_data)
{
/**************************************
 *
 *	I S C _ m a p _ f i l e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Try to map a given file.  If we are the first (i.e. only)
 *	process to map the file, call a given initialization
 *	routine (if given) or punt (leaving the file unmapped).
 *
 **************************************/
	HANDLE file_handle;
	HANDLE event_handle = 0;
	int retry_count = 0;

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, filename);

	const bool trunc_flag = (length != 0);
	bool init_flag = false;

/* retry to attach to mmapped file if the process initializing
 * dies during initialization.
 */

  retry:
	if (retry_count++ > 0) 
		THREAD_SLEEP(10);

	file_handle = CreateFile(expanded_filename,
							 GENERIC_READ | GENERIC_WRITE,
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 NULL,
							 OPEN_ALWAYS,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL);
	DWORD err = GetLastError();
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		if (err == ERROR_SHARING_VIOLATION)
			goto retry;

		error(status_vector, "CreateFile", err);
		return NULL;
	}

	// Check if file already exists

	const bool file_exists = (err == ERROR_ALREADY_EXISTS);

	// Create an event that can be used to determine if someone has already
	// initialized shared memory.

	TEXT object_name[MAXPATHLEN];
	if (!make_object_name(object_name, sizeof(object_name), filename, "_event"))
	{
		error(status_vector, "make_object_name", GetLastError());
		CloseHandle(file_handle);
		return NULL;
	}

	if (!init_flag)
	{
		event_handle = CreateEvent(ISC_get_security_desc(), TRUE, FALSE, object_name);
		if (!event_handle)
		{
			error(status_vector, "CreateEvent", GetLastError());
			CloseHandle(file_handle);
			return NULL;
		}

		init_flag = (GetLastError() != ERROR_ALREADY_EXISTS);

		if (init_flag && !init_routine)
		{
			CloseHandle(event_handle);
			CloseHandle(file_handle);
			Arg::Gds(isc_unavailable).copyTo(status_vector);
			return NULL;
		}
	}

	if (length == 0)
	{
		/* Get and use the existing length of the shared segment */

		if ((length = GetFileSize(file_handle, NULL)) == -1)
		{
			error(status_vector, "GetFileSize", GetLastError());
			CloseHandle(event_handle);
			CloseHandle(file_handle);
			return NULL;
		}
	}

/* All but the initializer will wait until the event is set.  That
 * is done after initialization is complete.
 * Close the file and wait for the event to be set or time out.
 * The file may be truncated.
 */

	CloseHandle(file_handle);

	if (!init_flag)
	{
		/* Wait for 10 seconds.  Then retry */

		const DWORD ret_event = WaitForSingleObject(event_handle, 10000);

		/* If we timed out, just retry.  It is possible that the
		 * process doing the initialization died before setting the
		 * event.
		 */

		if (ret_event == WAIT_TIMEOUT)
		{
			CloseHandle(event_handle);
			if (retry_count > 10)
			{
				error(status_vector, "WaitForSingleObject", 0);
				return NULL;
			}
			goto retry;
		}
	}

	DWORD fdw_create;
	if (init_flag && file_exists && trunc_flag)
		fdw_create = TRUNCATE_EXISTING;
	else
		fdw_create = OPEN_ALWAYS;

	file_handle = CreateFile(expanded_filename,
							 GENERIC_READ | GENERIC_WRITE,
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 NULL,
							 fdw_create,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		const DWORD err = GetLastError();

		if ((err == ERROR_SHARING_VIOLATION) || (err == ERROR_FILE_NOT_FOUND && fdw_create == TRUNCATE_EXISTING))
		{
			if (!init_flag) {
				CloseHandle(event_handle);
			}
			goto retry;
		}
		
		if (err == ERROR_USER_MAPPED_FILE && init_flag && file_exists && trunc_flag)
			Arg::Gds(isc_instance_conflict).copyTo(status_vector);
		else
			error(status_vector, "CreateFile", err);

		CloseHandle(event_handle);
		return NULL;
	}

	if (!init_flag)
	{
		if ((GetLastError() != ERROR_ALREADY_EXISTS) ||	SetFilePointer(file_handle, 0, NULL, FILE_END) == 0)
		{
			CloseHandle(event_handle);
			CloseHandle(file_handle);
			goto retry;
		}
	}

/* Create a file mapping object that will be used to make remapping possible.
   The current length of real mapped file and its name are saved in it. */

	if (!make_object_name(object_name, sizeof(object_name), filename, "_mapping"))
	{
		error(status_vector, "make_object_name", GetLastError());
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

	HANDLE header_obj = CreateFileMapping(INVALID_HANDLE_VALUE,
										  ISC_get_security_desc(),
										  PAGE_READWRITE,
										  0, 2 * sizeof(ULONG),
										  object_name);
	if (header_obj == NULL)
	{
		error(status_vector, "CreateFileMapping", GetLastError());
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

	if (!init_flag && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		// We have made header_obj but we are not initializing.
		// Previous owner is closed and clear all header_data.
		// One need to retry.
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		goto retry;
	}

	ULONG* const header_address = (ULONG*) MapViewOfFile(header_obj, FILE_MAP_WRITE, 0, 0, 0);

	if (header_address == NULL)
	{
		error(status_vector, "MapViewOfFile", GetLastError());
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

/* Set or get the true length of the file depending on whether or not
   we are the first user. */

	if (init_flag)
	{
		header_address[0] = length;
		header_address[1] = 0;
	}
	else
		length = header_address[0];

/* Create the real file mapping object. */

	TEXT mapping_name[64]; // enough for int32 as text
	sprintf(mapping_name, "_mapping_%"ULONGFORMAT, header_address[1]);

	if (!make_object_name(object_name, sizeof(object_name), filename, mapping_name))
	{
		error(status_vector, "make_object_name", GetLastError());
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

	HANDLE file_obj = CreateFileMapping(file_handle,
										ISC_get_security_desc(),
										PAGE_READWRITE,
										0, length,
										object_name);
	if (file_obj == NULL)
	{
		error(status_vector, "CreateFileMapping", GetLastError());
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

	UCHAR* const address = (UCHAR*) MapViewOfFile(file_obj, FILE_MAP_WRITE, 0, 0, 0);

	if (address == NULL)
	{
		error(status_vector, "MapViewOfFile", GetLastError());
		CloseHandle(file_obj);
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		return NULL;
	}

	shmem_data->sh_mem_address = address;
	shmem_data->sh_mem_length_mapped = length;

	if (!shmem_data->sh_mem_length_mapped)
	{
		error(status_vector, "shmem_data->sh_mem_length_mapped is 0", 0);
		return NULL;
	}

	shmem_data->sh_mem_handle = file_handle;
	shmem_data->sh_mem_object = file_obj;
	shmem_data->sh_mem_interest = event_handle;
	shmem_data->sh_mem_hdr_object = header_obj;
	shmem_data->sh_mem_hdr_address = header_address;
	strcpy(shmem_data->sh_mem_name, filename);

	if (init_routine)
		(*init_routine) (init_arg, shmem_data, init_flag);

	if (init_flag)
	{
		FlushViewOfFile(address, 0);

		DWORD err = 0;
		if (SetFilePointer(shmem_data->sh_mem_handle, length, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
			!SetEndOfFile(shmem_data->sh_mem_handle) ||
			!FlushViewOfFile(shmem_data->sh_mem_address, 0))
		{
			err = GetLastError();
		}

		SetEvent(event_handle);
		if (err)
		{
			error(status_vector, "SetFilePointer", err);
			return NULL;
		}
	}

	return address;
}
#endif


#ifdef HAVE_MMAP

#define HAVE_MAP_OBJECT 1

UCHAR* ISC_map_object(ISC_STATUS* status_vector,
					  sh_mem* shmem_data,
					  ULONG object_offset,
					  ULONG object_length)
{
/**************************************
 *
 *	I S C _ m a p _ o b j e c t
 *
 **************************************
 *
 * Functional description
 *	Try to map an object given a file mapping.
 *
 **************************************/
/* Get system page size as this is the unit of mapping. */

#ifdef SOLARIS
	const long ps = sysconf(_SC_PAGESIZE);
	if (ps == -1)
	{
		error(status_vector, "sysconf", errno);
		return NULL;
	}
#else
	const int ps = getpagesize();
	if (ps == -1)
	{
		error(status_vector, "getpagesize", errno);
		return NULL;
	}
#endif
	const ULONG page_size = (ULONG) ps;

/* Compute the start and end page-aligned offsets which
   contain the object being mapped. */

	const ULONG start = (object_offset / page_size) * page_size;
	const ULONG end = FB_ALIGN(object_offset + object_length, page_size);
	const ULONG length = end - start;
	int fd = shmem_data->sh_mem_handle;

	UCHAR* address = (UCHAR*) mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, start);

	if ((U_IPTR) address == (U_IPTR) -1)
	{
		error(status_vector, "mmap", errno);
		return NULL;
	}

/* Return the virtual address of the mapped object. */

	IPC_TRACE(("ISC_map_object in %p to %p %p\n", shmem_data->sh_mem_address, address, address + length));

	return (address + (object_offset - start));
}


void ISC_unmap_object(ISC_STATUS* status_vector,
					  UCHAR** object_pointer,
					  ULONG object_length)
{
/**************************************
 *
 *	I S C _ u n m a p _ o b j e c t
 *
 **************************************
 *
 * Functional description
 *	Try to unmap an object given a file mapping.
 *	Zero the object pointer after a successful unmap.
 *
 **************************************/
/* Get system page size as this is the unit of mapping. */

#ifdef SOLARIS
	const long ps = sysconf(_SC_PAGESIZE);
	if (ps == -1)
	{
		error(status_vector, "sysconf", errno);
		return;
	}
#else
	const int ps = getpagesize();
	if (ps == -1)
	{
		error(status_vector, "getpagesize", errno);
		return;
	}
#endif
	const size_t page_size = (ULONG) ps;

/* Compute the start and end page-aligned addresses which
   contain the mapped object. */

	UCHAR* const start = (UCHAR *) ((U_IPTR) * object_pointer & ~(page_size - 1));
	const UCHAR* end =
		(UCHAR*) ((U_IPTR) ((*object_pointer + object_length) + (page_size - 1)) & ~(page_size - 1));
	const ULONG length = end - start;

	if (munmap((char *) start, length) == -1)
	{
		error(status_vector, "munmap", errno);
		return; // false;
	}

	*object_pointer = NULL;
	return; // true;
}
#endif // HAVE_MMAP


#ifdef WIN_NT

#define HAVE_MAP_OBJECT 1

UCHAR* ISC_map_object(ISC_STATUS* status_vector,
					  sh_mem* shmem_data,
					  ULONG object_offset,
					  ULONG object_length)
{
/**************************************
 *
 *	I S C _ m a p _ o b j e c t
 *
 **************************************
 *
 * Functional description
 *	Try to map an object given a file mapping.
 *
 **************************************/

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	const ULONG page_size = sys_info.dwAllocationGranularity;

	// Compute the start and end page-aligned offsets which
	// contain the object being mapped.

	const ULONG start = (object_offset / page_size) * page_size;
	const ULONG end = FB_ALIGN(object_offset + object_length, page_size);
	const ULONG length = end - start;
	const HANDLE handle = shmem_data->sh_mem_object;

	UCHAR* address = (UCHAR*) MapViewOfFile(handle, FILE_MAP_WRITE, 0, start, length);

	if (address == NULL)
	{
		error(status_vector, "MapViewOfFile", GetLastError());
		return NULL;
	}

	// Return the virtual address of the mapped object.

	return (address + (object_offset - start));
}


void ISC_unmap_object(ISC_STATUS* status_vector,
					  UCHAR** object_pointer,
					  ULONG /*object_length*/)
{
/**************************************
 *
 *	I S C _ u n m a p _ o b j e c t
 *
 **************************************
 *
 * Functional description
 *	Try to unmap an object given a file mapping.
 *	Zero the object pointer after a successful unmap.
 *
 **************************************/
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	const size_t page_size = sys_info.dwAllocationGranularity;

	// Compute the start and end page-aligned offsets which
	// contain the object being mapped.

	const UCHAR* start = (UCHAR*) ((U_IPTR) *object_pointer & ~(page_size - 1));
	if (!UnmapViewOfFile(start))
	{
		error(status_vector, "UnmapViewOfFile", GetLastError());
		return;
	}

	*object_pointer = NULL;
}
#endif

int ISC_map_mutex(sh_mem* shmem_data, mtx* mutex, mtx** mapped)
{
#if defined(HAVE_MAP_OBJECT) && (!defined(USE_SYS5SEMAPHORE))
	ISC_STATUS_ARRAY temp;
	mutex = reinterpret_cast<mtx*>(ISC_map_object(temp, shmem_data,
			reinterpret_cast<UCHAR*>(mutex) - shmem_data->sh_mem_address, sizeof(mtx)));
	if (!mutex)
	{
		iscLogStatus("ISC_map_mutex()", temp);
		return -1;
	}
#endif // HAVE_MAP_OBJECT

	*mapped = mutex;
	return 0;
}

void ISC_unmap_mutex(mtx* mutex)
{
#if defined(HAVE_MAP_OBJECT) && (!defined(USE_SYS5SEMAPHORE))
	ISC_STATUS_ARRAY temp;
	ISC_unmap_object(temp, reinterpret_cast<UCHAR**>(&mutex), sizeof(mtx));
	if (mutex)
	{
		iscLogStatus("ISC_unmap_mutex()", temp);
	}
#endif // HAVE_MAP_OBJECT

}


#ifdef USE_POSIX_THREADS

#ifdef USE_SYS5SEMAPHORE

int ISC_mutex_init(sh_mem* shmem_data, struct mtx* mutex, struct mtx** mapped)
{
/**************************************
 *
 *	I S C _ m u t e x _ i n i t	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Initialize a mutex.
 *
 **************************************/

	if (ISC_map_mutex(shmem_data, mutex, mapped) != 0)
	{
		return -1;	// no errno known here...
	}
	mutex = *mapped;

#ifdef USE_FILELOCKS

	SharedFile* sf = SharedFile::locate(mutex);
	if (!sf)
	{
		gds__log("SharedFile::locate(sem) failed");

#else // USE_FILELOCKS

	if (!getSem5(mutex))
	{
		return FB_FAILURE;
	}

	union semun arg;
	arg.val = 1;
	int state = semctl(mutex->getId(), mutex->semNum, SETVAL, arg);
	if (state == -1)
 	{
		iscLogStatus("ISC_mutex_init()", (Arg::Gds(isc_sys_request) << Arg::Str("semctl") << SYS_ERR(errno)).value());

#endif // USE_FILELOCKS

		return FB_FAILURE;
	}

	return 0;
}


void ISC_mutex_fini(struct mtx *mutex)
{
/**************************************
 *
 *	m u t e x _ f i n i ( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Destroy a mutex.
 *
 **************************************/
#ifdef USE_FILELOCKS

	SharedFile* sf = SharedFile::locate(mutex);
	if (!sf)
	{
		gds__log("SharedFile::locate(sem) failed");
	}
	else
	{
		return sf->mutexDestroy();
	}

#endif // USE_FILELOCKS

	ISC_unmap_mutex(mutex);
}


int ISC_mutex_lock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k	( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Seize a mutex.
 *
 **************************************/
#ifdef USE_FILELOCKS

	SharedFile* sf = SharedFile::locate(mutex);
	if (!sf)
	{
		gds__log("SharedFile::locate(sem) failed");
		return -1;
	}

	return sf->mutexLock();

#else // USE_FILELOCKS

	struct sembuf sop;
	sop.sem_num = mutex->semNum;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO;

	for (;;)
	{
		int state = semop(mutex->getId(), &sop, 1);
		if (state != -1)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
			return errno;
 	}

	return 0;

#endif // USE_FILELOCKS

}


int ISC_mutex_lock_cond(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k _ c o n d ( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Conditionally seize a mutex.
 *
 **************************************/
#ifdef USE_FILELOCKS

	SharedFile* sf = SharedFile::locate(mutex);
	if (!sf)
	{
		gds__log("SharedFile::locate(sem) failed");
		return -1;
	}

	return sf->mutexTryLock();

#else // USE_FILELOCKS

	struct sembuf sop;
	sop.sem_num = mutex->semNum;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO | IPC_NOWAIT;

	for (;;)
	{
		int state = semop(mutex->getId(), &sop, 1);
		if (state != -1)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
			return errno;
 	}

	return 0;

#endif // USE_FILELOCKS

}


int ISC_mutex_unlock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ u n l o c k ( S Y S V )
 *
 **************************************
 *
 * Functional description
 *	Release a mutex.
 *
 **************************************/
#ifdef USE_FILELOCKS

	SharedFile* sf = SharedFile::locate(mutex);
	if (!sf)
	{
		gds__log("SharedFile::locate(sem) failed");
		return -1;
	}

	sf->mutexUnlock();
	return 0;

#else // USE_FILELOCKS

	struct sembuf sop;
	sop.sem_num = mutex->semNum;
	sop.sem_op = 1;
	sop.sem_flg = SEM_UNDO;

	for (;;)
 	{
		int state = semop(mutex->getId(), &sop, 1);
		if (state != -1)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
			return errno;
 	}

 	return 0;

#endif // USE_FILELOCKS
}

#else // not USE_SYS5SEMAPHORE

#if (defined(HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL) || defined(USE_ROBUST_MUTEX)) && defined(LINUX)
// glibc in linux does not conform to the posix standard. When there is no RT kernel,
// ENOTSUP is returned not by pthread_mutexattr_setprotocol(), but by
// pthread_mutex_init(). Here is a hack to deal with this broken error reporting.
#define BUGGY_LINUX_MUTEX
#endif

#ifdef USE_LOCAL_MUTEXES
#undef BUGGY_LINUX_MUTEX
#endif

#ifdef BUGGY_LINUX_MUTEX
static volatile bool staticBugFlag = false;
#endif

int ISC_mutex_init(sh_mem* shmem_data, struct mtx* mutex, struct mtx** mapped)
{
/**************************************
 *
 *	I S C _ m u t e x _ i n i t	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Initialize a mutex.
 *
 **************************************/

  if (ISC_map_mutex(shmem_data, mutex, mapped) != 0)
  {
		return -1;	// no errno known here...
  }
  mutex = *mapped;

#ifdef BUGGY_LINUX_MUTEX
  do
  {
	bool bugFlag = staticBugFlag;
#endif

	pthread_mutexattr_t mattr;

	PTHREAD_ERRNO(pthread_mutexattr_init(&mattr));
#ifndef USE_LOCAL_MUTEXES
#ifdef PTHREAD_PROCESS_SHARED
	PTHREAD_ERRNO(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED));
#else
#error Your system must support PTHREAD_PROCESS_SHARED to use Firebird.
#endif //PTHREAD_PROCESS_SHARED

#ifdef HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL
#ifdef BUGGY_LINUX_MUTEX
	if (!bugFlag)
	{
#endif
		int protocolRc = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
		if (protocolRc && (protocolRc != ENOTSUP))
		{
			iscLogStatus("Pthread Error", (Arg::Gds(isc_sys_request) <<
				"pthread_mutexattr_setprotocol" << Arg::Unix(protocolRc)).value());
		}
#ifdef BUGGY_LINUX_MUTEX
	}
#endif
#endif //HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL

#ifdef USE_ROBUST_MUTEX
#ifdef BUGGY_LINUX_MUTEX
	if (!bugFlag)
	{
#endif
		LOG_PTHREAD_ERROR(pthread_mutexattr_setrobust_np(&mattr, PTHREAD_MUTEX_ROBUST_NP));
#ifdef BUGGY_LINUX_MUTEX
	}
#endif
#endif //USE_ROBUST_MUTEX
#endif //USE_LOCAL_MUTEXES

	memset(mutex->mtx_mutex, 0, sizeof(pthread_mutex_t));
	//int state = LOG_PTHREAD_ERROR(pthread_mutex_init(mutex->mtx_mutex, &mattr));
	int state = pthread_mutex_init(mutex->mtx_mutex, &mattr);

	if (state
#ifdef BUGGY_LINUX_MUTEX
		&& (state != ENOTSUP || bugFlag)
#endif
			 )
	{
		iscLogStatus("Pthread Error", (Arg::Gds(isc_sys_request) <<
			"pthread_mutex_init" << Arg::Unix(state)).value());
	}

	LOG_PTHREAD_ERROR(pthread_mutexattr_destroy(&mattr));

#ifdef BUGGY_LINUX_MUTEX
	if (state == ENOTSUP && !bugFlag)
	{
		staticBugFlag = true;
		continue;
	}

	return state;	// To avoid declaring 'state' out of loop
  } while (false);

  return 0;			// compiler warning silencer
#else

  return state;
#endif
}


void ISC_mutex_fini(struct mtx *mutex)
{
/**************************************
 *
 *	m u t e x _ f i n i ( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Destroy a mutex.
 *
 **************************************/
	ISC_unmap_mutex(mutex);
}


int ISC_mutex_lock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Seize a mutex.
 *
 **************************************/

	int state = pthread_mutex_lock(mutex->mtx_mutex);
#ifdef USE_ROBUST_MUTEX
	if (state == EOWNERDEAD)
	{
		// We always perform check for dead process
		// Therefore may safely mark mutex as recovered
		LOG_PTHREAD_ERROR(pthread_mutex_consistent_np(mutex->mtx_mutex));
		state = 0;
	}
#endif
	return state;
}


int ISC_mutex_lock_cond(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k _ c o n d	( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Conditionally seize a mutex.
 *
 **************************************/

	return pthread_mutex_trylock(mutex->mtx_mutex);
}


int ISC_mutex_unlock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ u n l o c k		( P O S I X _ T H R E A D S )
 *
 **************************************
 *
 * Functional description
 *	Release a mutex.
 *
 **************************************/

	return pthread_mutex_unlock(mutex->mtx_mutex);
}

#endif // USE_SYS5SEMAPHORE

#endif // USE_POSIX_THREADS


#ifdef WIN_NT

static const LPCSTR FAST_MUTEX_EVT_NAME	= "%s_FM_EVT";
static const LPCSTR FAST_MUTEX_MAP_NAME	= "%s_FM_MAP";

static const int DEFAULT_INTERLOCKED_SPIN_COUNT	= 0;
static const int DEFAULT_INTERLOCKED_SPIN_COUNT_SMP	= 200;

static SLONG pid = 0;

typedef WINBASEAPI BOOL (WINAPI *pfnSwitchToThread) ();
static inline BOOL switchToThread()
{
	static pfnSwitchToThread fnSwitchToThread = NULL;
	static bool bInit = false;

	if (!bInit)
	{
		HMODULE hLib = GetModuleHandle("kernel32.dll");
		if (hLib) {
			fnSwitchToThread = (pfnSwitchToThread) GetProcAddress(hLib, "SwitchToThread");
		}
		bInit = true;
	}

	BOOL res = FALSE;
	if (fnSwitchToThread)
	{
#if !defined SUPERSERVER
		const HANDLE hThread = GetCurrentThread();
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
#endif

		res = (*fnSwitchToThread)();

#if !defined SUPERSERVER
		SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
#endif
	}

	return res;
}


// MinGW has the wrong declaration for the operating system function.
#if defined __GNUC__
// Cast away volatile
#define FIX_TYPE(arg) const_cast<LPLONG>(arg)
#else
#define FIX_TYPE(arg) arg
#endif


static inline void lockSharedSection(volatile FAST_MUTEX_SHARED_SECTION* lpSect, ULONG SpinCount)
{
	while (InterlockedExchange(FIX_TYPE(&lpSect->lSpinLock), 1) != 0)
	{
		ULONG j = SpinCount;
		while (j != 0)
		{
			if (lpSect->lSpinLock == 0)
				goto next;
			j--;
		}
		switchToThread();
next:;
	}
}

static inline bool tryLockSharedSection(volatile FAST_MUTEX_SHARED_SECTION* lpSect)
{
	return (InterlockedExchange(FIX_TYPE(&lpSect->lSpinLock), 1) == 0);
}

static inline void unlockSharedSection(volatile FAST_MUTEX_SHARED_SECTION* lpSect)
{
	InterlockedExchange(FIX_TYPE(&lpSect->lSpinLock), 0);
}

static DWORD enterFastMutex(FAST_MUTEX* lpMutex, DWORD dwMilliseconds)
{
	volatile FAST_MUTEX_SHARED_SECTION* lpSect = lpMutex->lpSharedInfo;

	while (true)
	{
		if (dwMilliseconds == 0)
		{
			if (!tryLockSharedSection(lpSect))
				return WAIT_TIMEOUT;
		}
		else {
			lockSharedSection(lpSect, lpMutex->lSpinCount);
		}

		if (lpSect->lAvailable > 0)
		{
			lpSect->lAvailable--;
#ifdef _DEBUG
			lpSect->dwThreadId = GetCurrentThreadId();
#endif
			lpSect->lOwnerPID = pid;
			unlockSharedSection(lpSect);
			return WAIT_OBJECT_0;
		}

#ifdef _DEBUG
		if (lpSect->dwThreadId == GetCurrentThreadId())
			DebugBreak();
#endif
		if (dwMilliseconds == 0)
		{
			unlockSharedSection(lpSect);
			return WAIT_TIMEOUT;
		}

		InterlockedIncrement(FIX_TYPE(&lpSect->lThreadsWaiting));
		unlockSharedSection(lpSect);

		// TODO actual timeout can be of any length
		const DWORD tm = (dwMilliseconds == INFINITE || dwMilliseconds > 5000) ? 5000 :  dwMilliseconds;
		const DWORD dwResult = WaitForSingleObject(lpMutex->hEvent, tm);

		InterlockedDecrement(FIX_TYPE(&lpSect->lThreadsWaiting));

		if (dwMilliseconds != INFINITE)
			dwMilliseconds -= tm;

//		if (dwResult != WAIT_OBJECT_0)
//			return dwResult;

		if (dwResult == WAIT_OBJECT_0)
			continue;
		if (dwResult == WAIT_ABANDONED)
			return dwResult;
		if (dwResult == WAIT_TIMEOUT && !dwMilliseconds)
			return dwResult;

		lockSharedSection(lpSect, lpMutex->lSpinCount);
		if (lpSect->lOwnerPID > 0 && !lpSect->lAvailable)
		{
			if (!ISC_check_process_existence(lpSect->lOwnerPID))
			{
#ifdef DEBUG
				gds__log("enterFastMutex: dead process detected, pid = %d", lpSect->lOwnerPID);
#endif
				lpSect->lOwnerPID = 0;
				lpSect->lAvailable++;
			}
		}
		unlockSharedSection(lpSect);
	}
}

static bool leaveFastMutex(FAST_MUTEX* lpMutex)
{
	volatile FAST_MUTEX_SHARED_SECTION* lpSect = lpMutex->lpSharedInfo;

	lockSharedSection(lpSect, lpMutex->lSpinCount);
	if (lpSect->lAvailable >= 1)
	{
		unlockSharedSection(lpSect);
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	lpSect->lAvailable++;
	if (lpSect->lThreadsWaiting)
		SetEvent(lpMutex->hEvent);
	lpSect->lOwnerPID = -pid;
	unlockSharedSection(lpSect);

	return true;
}

static inline void deleteFastMutex(FAST_MUTEX* lpMutex)
{
	UnmapViewOfFile((FAST_MUTEX_SHARED_SECTION*)lpMutex->lpSharedInfo);
	CloseHandle(lpMutex->hFileMap);
	CloseHandle(lpMutex->hEvent);
}

static inline void setupMutex(FAST_MUTEX* lpMutex)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	if (si.dwNumberOfProcessors > 1)
		lpMutex->lSpinCount = DEFAULT_INTERLOCKED_SPIN_COUNT_SMP;
	else
		lpMutex->lSpinCount = DEFAULT_INTERLOCKED_SPIN_COUNT;
}

static bool initializeFastMutex(FAST_MUTEX* lpMutex, LPSECURITY_ATTRIBUTES lpAttributes,
								BOOL bInitialState, LPCSTR lpName)
{
	if (pid == 0)
		pid = GetCurrentProcessId();

	LPCSTR name = lpName;

	if (strlen(lpName) + strlen(FAST_MUTEX_EVT_NAME) - 2 >= MAXPATHLEN)
	{
		// this is the same error which CreateEvent will return for long name
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return false;
	}

	setupMutex(lpMutex);

	char sz[MAXPATHLEN];
	if (lpName)
	{
		sprintf(sz, FAST_MUTEX_EVT_NAME, lpName);
		name = sz;
	}

#ifdef DONT_USE_FAST_MUTEX
	lpMutex->lpSharedInfo = NULL;
	lpMutex->hEvent = CreateMutex(lpAttributes, bInitialState, name);
	return (lpMutex->hEvent != NULL);
#else
	lpMutex->hEvent = CreateEvent(lpAttributes, FALSE, FALSE, name);
	DWORD dwLastError = GetLastError();

	if (lpMutex->hEvent)
	{
		if (lpName)
			sprintf(sz, FAST_MUTEX_MAP_NAME, lpName);

		lpMutex->hFileMap = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			lpAttributes,
			PAGE_READWRITE,
			0,
			sizeof(FAST_MUTEX_SHARED_SECTION),
			name);

		dwLastError = GetLastError();

		if (lpMutex->hFileMap)
		{
			lpMutex->lpSharedInfo = (FAST_MUTEX_SHARED_SECTION*)
				MapViewOfFile(lpMutex->hFileMap, FILE_MAP_WRITE, 0, 0, 0);

			if (lpMutex->lpSharedInfo)
			{
				if (dwLastError != ERROR_ALREADY_EXISTS)
				{
					lpMutex->lpSharedInfo->lSpinLock = 0;
					lpMutex->lpSharedInfo->lThreadsWaiting = 0;
					lpMutex->lpSharedInfo->lAvailable = bInitialState ? 0 : 1;
					lpMutex->lpSharedInfo->lOwnerPID = bInitialState ? pid : 0;
					InterlockedExchange(FIX_TYPE(&lpMutex->lpSharedInfo->fInitialized), 1);
				}
				else
				{
					while (!lpMutex->lpSharedInfo->fInitialized)
						switchToThread();
				}

				SetLastError(dwLastError);
				return true;
			}
			CloseHandle(lpMutex->hFileMap);
		}
		CloseHandle(lpMutex->hEvent);
	}

	SetLastError(dwLastError);
	return false;
#endif // DONT_USE_FAST_MUTEX
}

#ifdef NOT_USED_OR_REPLACED
static bool openFastMutex(FAST_MUTEX* lpMutex, DWORD DesiredAccess, LPCSTR lpName)
{
	LPCSTR name = lpName;

	if (strlen(lpName) + strlen(FAST_MUTEX_EVT_NAME) - 2 >= MAXPATHLEN)
	{
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return false;
	}

	setupMutex(lpMutex);

	char sz[MAXPATHLEN];
	if (lpName)
	{
		sprintf(sz, FAST_MUTEX_EVT_NAME, lpName);
		name = sz;
	}

	lpMutex->hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, name);

	DWORD dwLastError = GetLastError();

	if (lpMutex->hEvent)
	{
		if (lpName)
			sprintf(sz, FAST_MUTEX_MAP_NAME, lpName);

		lpMutex->hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);

		dwLastError = GetLastError();

		if (lpMutex->hFileMap)
		{
			lpMutex->lpSharedInfo = (FAST_MUTEX_SHARED_SECTION*)
				MapViewOfFile(lpMutex->hFileMap, FILE_MAP_WRITE, 0, 0, 0);

			if (lpMutex->lpSharedInfo)
				return true;

			CloseHandle(lpMutex->hFileMap);
		}
		CloseHandle(lpMutex->hEvent);
	}

	SetLastError(dwLastError);
	return false;
}
#endif

static inline void setFastMutexSpinCount(FAST_MUTEX* lpMutex, ULONG SpinCount)
{
	lpMutex->lSpinCount = SpinCount;
}


int ISC_mutex_init(struct mtx* mutex, const TEXT* mutex_name)
{
/**************************************
 *
 *	I S C _ m u t e x _ i n i t	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Initialize a mutex.
 *
 **************************************/
	char name_buffer[MAXPATHLEN];

	if (!make_object_name(name_buffer, sizeof(name_buffer), mutex_name, "_mutex"))
	{
		return FB_FAILURE;
	}

	if (initializeFastMutex(&mutex->mtx_fast, ISC_get_security_desc(), FALSE, name_buffer))
		return FB_SUCCESS;

	fb_assert(GetLastError() != 0);
	return GetLastError();
}


void ISC_mutex_fini(struct mtx *mutex)
{
/**************************************
 *
 *	m u t e x _ f i n i ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Destroy a mutex.
 *
 **************************************/
	if (mutex->mtx_fast.lpSharedInfo)
		deleteFastMutex(&mutex->mtx_fast);
}


int ISC_mutex_lock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Seize a mutex.
 *
 **************************************/

	const DWORD status = (mutex->mtx_fast.lpSharedInfo) ?
		enterFastMutex(&mutex->mtx_fast, INFINITE) :
			WaitForSingleObject(mutex->mtx_fast.hEvent, INFINITE);

    return (status == WAIT_OBJECT_0 || status == WAIT_ABANDONED) ? FB_SUCCESS : FB_FAILURE;
}


int ISC_mutex_lock_cond(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ l o c k _ c o n d	( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Conditionally seize a mutex.
 *
 **************************************/

	const DWORD status = (mutex->mtx_fast.lpSharedInfo) ?
		enterFastMutex(&mutex->mtx_fast, 0) : WaitForSingleObject(mutex->mtx_fast.hEvent, 0L);

    return (status == WAIT_OBJECT_0 || status == WAIT_ABANDONED) ? FB_SUCCESS : FB_FAILURE;
}


int ISC_mutex_unlock(struct mtx* mutex)
{
/**************************************
 *
 *	I S C _ m u t e x _ u n l o c k		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Release a mutex.
 *
 **************************************/

	if (mutex->mtx_fast.lpSharedInfo) {
		return !leaveFastMutex(&mutex->mtx_fast);
	}

	return !ReleaseMutex(mutex->mtx_fast.hEvent);
}


void ISC_mutex_set_spin_count (struct mtx *mutex, ULONG spins)
{
	if (mutex->mtx_fast.lpSharedInfo)
		setFastMutexSpinCount(&mutex->mtx_fast, spins);
}

#endif // WIN_NT


#ifdef UNIX
#ifdef HAVE_MMAP
#define ISC_REMAP_FILE_DEFINED
UCHAR *ISC_remap_file(ISC_STATUS* status_vector,
					  sh_mem* shmem_data,
					  ULONG new_length,
					  bool flag,
					  struct mtx** mutex)
{
/**************************************
 *
 *	I S C _ r e m a p _ f i l e		( U N I X - m m a p )
 *
 **************************************
 *
 * Functional description
 *	Try to re-map a given file.
 *
 **************************************/
	if (flag)
		ftruncate(shmem_data->sh_mem_handle, new_length);

	UCHAR* const address = (UCHAR*)
		mmap(0, new_length, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_data->sh_mem_handle, 0);
	if ((U_IPTR) address == (U_IPTR) -1)
		return NULL;

#ifdef USE_SYS5SEMAPHORE
	SharedFile::remap(shmem_data->sh_mem_address, address, new_length, mutex);
#endif

	munmap((char *) shmem_data->sh_mem_address, shmem_data->sh_mem_length_mapped);

	IPC_TRACE(("ISC_remap_file %p to %p %p\n", shmem_data->sh_mem_address, address, address + new_length));

	shmem_data->sh_mem_address = address;
	shmem_data->sh_mem_length_mapped = new_length;

	if (!shmem_data->sh_mem_length_mapped)
	{
		error(status_vector, "shmem_data->sh_mem_length_mapped is 0", 0);
		return NULL;
	}

	return address;
}
#endif // HAVE_MMAP
#endif // UNIX


#ifdef WIN_NT
#define ISC_REMAP_FILE_DEFINED
UCHAR* ISC_remap_file(ISC_STATUS * status_vector,
					  sh_mem* shmem_data,
					  ULONG new_length,
					  bool flag,
					  struct mtx** /*mutex*/)
{
/**************************************
 *
 *	I S C _ r e m a p _ f i l e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Try to re-map a given file.
 *
 **************************************/

	if (flag)
	{
		if (SetFilePointer(shmem_data->sh_mem_handle, new_length, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
			!SetEndOfFile(shmem_data->sh_mem_handle) ||
			!FlushViewOfFile(shmem_data->sh_mem_address, 0))
		{
			error(status_vector, "SetFilePointer", GetLastError());
			return NULL;
		}
	}

/* If the remap file exists, remap does not occur correctly.
 * The file number is local to the process and when it is
 * incremented and a new filename is created, that file may
 * already exist.  In that case, the file is not expanded.
 * This will happen when the file is expanded more than once
 * by concurrently running processes.
 *
 * The problem will be fixed by making sure that a new file name
 * is generated with the mapped file is created.
 */

	HANDLE file_obj = NULL;

	while (true)
	{
		TEXT mapping_name[64]; // enough for int32 as text
		sprintf(mapping_name, "_mapping_%"ULONGFORMAT, shmem_data->sh_mem_hdr_address[1] + 1);

		TEXT object_name[MAXPATHLEN];
		if (!make_object_name(object_name, sizeof(object_name), shmem_data->sh_mem_name, mapping_name))
			break;

		file_obj = CreateFileMapping(shmem_data->sh_mem_handle,
									 ISC_get_security_desc(),
									 PAGE_READWRITE,
									 0, new_length,
									 object_name);

		if (!(GetLastError() == ERROR_ALREADY_EXISTS && flag))
			break;

		CloseHandle(file_obj);
		file_obj = NULL;
		shmem_data->sh_mem_hdr_address[1]++;
	}

	if (file_obj == NULL)
	{
		error(status_vector, "CreateFileMapping", GetLastError());
		return NULL;
	}

	void* const address = MapViewOfFile(file_obj, FILE_MAP_WRITE, 0, 0, 0);

	if (address == NULL)
	{
		error(status_vector, "MapViewOfFile", GetLastError());
		CloseHandle(file_obj);
		return NULL;
	}

	if (flag)
	{
		shmem_data->sh_mem_hdr_address[0] = new_length;
		shmem_data->sh_mem_hdr_address[1]++;
	}

	UnmapViewOfFile(shmem_data->sh_mem_address);
	CloseHandle(shmem_data->sh_mem_object);

	shmem_data->sh_mem_address = static_cast<UCHAR*>(address);
	shmem_data->sh_mem_length_mapped = new_length;
	shmem_data->sh_mem_object = file_obj;

	if (!shmem_data->sh_mem_length_mapped)
	{
		error(status_vector, "shmem_data->sh_mem_length_mapped is 0", 0);
		return NULL;
	}

	return static_cast<UCHAR*>(address);
}
#endif


#ifndef ISC_REMAP_FILE_DEFINED
UCHAR* ISC_remap_file(ISC_STATUS * status_vector,
						sh_mem* shmem_data,
						ULONG new_length,
						bool flag,
						struct mtx** mutex)
{
/**************************************
 *
 *	I S C _ r e m a p _ f i l e		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Try to re-map a given file.
 *
 **************************************/

	Arg::Gds(isc_unavailable).copyTo(status_vector);

	return NULL;
}
#endif


#ifdef UNIX
void ISC_unmap_file(ISC_STATUS* status_vector, sh_mem* shmem_data)
{
/**************************************
 *
 *	I S C _ u n m a p _ f i l e		( U N I X - m m a p )
 *
 **************************************
 *
 * Functional description
 *	Unmap a given file.
 *
 **************************************/
#ifdef USE_SYS5SEMAPHORE
	// Lock init file.
	FileLock initLock(fd_init, FileLock::OPENED);
	if (!initLock.doLock(status_vector, FileLock::FLM_EXCLUSIVE))
	{
		iscLogStatus("ISC_unmap_file failed to lock init file", status_vector);
	}
	else
	{
		SharedFile* sf = SharedFile::locate(shmem_data->sh_mem_address);

		FileLock lock(shmem_data->sh_mem_handle, FileLock::OPENED);
		lock.setLevel(FileLock::LCK_SHARED);
		semTable->cleanup(sf->getNum(), lock.doLock(status_vector, FileLock::FLM_TRY_EXCLUSIVE));
		SharedFile::remove(shmem_data->sh_mem_address);
	}
	--sharedCount;
#endif

	munmap((char *) shmem_data->sh_mem_address, shmem_data->sh_mem_length_mapped);

	closeFile(shmem_data->sh_mem_handle);
}
#endif


#ifdef WIN_NT
void ISC_unmap_file(ISC_STATUS* status_vector, sh_mem* shmem_data)
{
/**************************************
 *
 *	I S C _ u n m a p _ f i l e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Detach from the shared memory.
 *
 **************************************/

	CloseHandle(shmem_data->sh_mem_interest);
	if (!UnmapViewOfFile(shmem_data->sh_mem_address))
	{
		error(status_vector, "UnmapViewOfFile", GetLastError());
		return;
	}
	CloseHandle(shmem_data->sh_mem_object);

	CloseHandle(shmem_data->sh_mem_handle);
	if (!UnmapViewOfFile(shmem_data->sh_mem_hdr_address))
	{
		error(status_vector, "UnmapViewOfFile", GetLastError());
		return;
	}
	CloseHandle(shmem_data->sh_mem_hdr_object);

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, shmem_data->sh_mem_name);

	// Delete file only if it is not used by anyone else
	HANDLE hFile = CreateFile(expanded_filename,
							 DELETE,
							 0,
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
							 NULL);

	if (hFile != INVALID_HANDLE_VALUE) 
		CloseHandle(hFile);
}
#endif


static void error(ISC_STATUS* status_vector, const TEXT* string, ISC_STATUS status)
{
/**************************************
 *
 *	e r r o r
 *
 **************************************
 *
 * Functional description
 *	We've encountered an error, report it.
 *
 **************************************/

	(Arg::Gds(isc_sys_request) << Arg::Str(string) << SYS_ERR(status)).copyTo(status_vector);
	makePermanentVector(status_vector);
}


#ifdef USE_SYS5SEMAPHORE

static SLONG create_semaphores(ISC_STATUS* status_vector,
							 SLONG key, int semaphores)
{
/**************************************
 *
 *	c r e a t e _ s e m a p h o r e s		( U N I X )
 *
 **************************************
 *
 * Functional description
 *	Create or find a block of semaphores.
 *
 **************************************/
	while (true)
	{
		// Try to open existing semaphore set
		SLONG semid = semget(key, 0, 0);
		if (semid == -1)
		{
			if (errno != ENOENT)
			{
				error(status_vector, "semget", errno);
				return -1;
			}
		}
		else
		{
			union semun arg;
			semid_ds buf;
			arg.buf = &buf;
			// Get number of semaphores in opened set
			if (semctl(semid, 0, IPC_STAT, arg) == -1)
			{
				error(status_vector, "semctl", errno);
				return -1;
			}
			if ((int) buf.sem_nsems >= semaphores)
				return semid;
			// Number of semaphores in existing set is too small. Discard it.
			if (semctl(semid, 0, IPC_RMID) == -1)
			{
				error(status_vector, "semctl", errno);
				return -1;
			}
		}

		// Try to create new semaphore set
		semid = semget(key, semaphores, IPC_CREAT | IPC_EXCL | PRIV);
		if (semid != -1)
		{
			// We want to limit access to semaphores, created here
			// Reasonable access rights to them - exactly like security database has
			char secDb[MAXPATHLEN];
			SecurityDatabase::getPath(secDb);
			struct stat st;
			if (stat(secDb, &st) == 0)
			{
				union semun arg;
				semid_ds ds;
				arg.buf = &ds;
				ds.sem_perm.uid = geteuid() == 0 ? st.st_uid : geteuid();
				ds.sem_perm.gid = st.st_gid;
				ds.sem_perm.mode = st.st_mode;
				semctl(semid, 0, IPC_SET, arg);
			}
			return semid;
		}

		if (errno != EEXIST)
		{
			error(status_vector, "semget", errno);
			return -1;
		}
	}
}

#endif // USE_SYS5SEMAPHORE


#ifdef WIN_NT
static bool make_object_name(TEXT* buffer, size_t bufsize,
							 const TEXT* object_name,
							 const TEXT* object_type)
{
/**************************************
 *
 *	m a k e _ o b j e c t _ n a m e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Create an object name from a name and type.
 *	Also replace the file separator with "_".
 *
 **************************************/
	char hostname[64];
	const int rc = snprintf(buffer, bufsize, object_name, ISC_get_host(hostname, sizeof(hostname)));
	if (size_t(rc) == bufsize || rc <= 0)
	{
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return false;
	}

	char& limit = buffer[bufsize - 1];
	limit = 0;

	char* p;
	char c;
	for (p = buffer; c = *p; p++)
	{
		if (c == '/' || c == '\\' || c == ':')
			*p = '_';
	}

	// We either append the full object type or produce failure.
	if (p >= &limit || p + strlen(object_type) > &limit)
	{
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return false;
	}

	strcpy(p, object_type);

	// hvlad: windows file systems use case-insensitive file names
	// while kernel objects such as events use case-sensitive names.
	// Since we use root directory as part of kernel objects names
	// we must use lower (or upper) register for object name to avoid
	// misunderstanding between processes
	strlwr(buffer);

	// CVC: I'm not convinced that if this call has no space to put the prefix,
	// we can ignore that fact, hence I changed that signature, too.
	if (!fb_utils::prefix_kernel_object_name(buffer, bufsize))
	{
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return false;
	}
	return true;
}
#endif // WIN_NT
