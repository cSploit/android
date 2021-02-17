/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_sync.cpp
 *	DESCRIPTION:	OS-dependent IPC: shared memory, mutex and event.
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
#include "../common/gdsassert.h"
#endif

#ifdef HPUX
#include <sys/pstat.h>
#endif

#include "gen/iberror.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/os/isc_i_proto.h"
#include "../common/os/os_utils.h"
#include "../common/isc_s_proto.h"
#include "../common/file_params.h"
#include "../common/gdsassert.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../common/StatusArg.h"
#include "../common/ThreadData.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/RefMutex.h"
#include "../common/classes/array.h"

static int process_id;

// Unix specific stuff

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
#include <sys/time.h>
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

#endif // UNIX

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef HAVE_GETPAGESIZE
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


// Windows NT

#ifdef WIN_NT

#include <process.h>
#include <windows.h>

#endif

using namespace Firebird;

static void		error(Arg::StatusVector&, const TEXT*, ISC_STATUS);
static bool		event_blocked(const event_t* event, const SLONG value);

#ifdef UNIX

static GlobalPtr<Mutex> openFdInit;

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

namespace Firebird {

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

class CountedFd
{
public:
	explicit CountedFd(int f)
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

} // namespace Firebird

namespace {

	typedef GenericMap<Pair<Left<string, Firebird::CountedRWLock*> > > RWLocks;
	GlobalPtr<RWLocks> rwlocks;
	GlobalPtr<Mutex> rwlocksMutex;
#ifdef USE_FCNTL
	const char* NAME = "fcntl";
#else
	const char* NAME = "flock";
#endif

	class FileLockHolder
	{
	public:
		explicit FileLockHolder(FileLock* l)
			: lock(l)
		{
			Arg::StatusVector status;
			if (!lock->setlock(status, FileLock::FLM_EXCLUSIVE))
				status.raise();
		}

		~FileLockHolder()
		{
			lock->unlock();
		}

	private:
		FileLock* lock;
	};

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

} // anonymous namespace


typedef GenericMap<Pair<NonPooled<DevNode, Firebird::CountedFd*> > > FdNodes;
static GlobalPtr<Mutex> fdNodesMutex;
static GlobalPtr<FdNodes> fdNodes;

FileLock::FileLock(const char* fileName, InitFunction* init)
	: level(LCK_NONE), oFile(NULL),
#ifdef USE_FCNTL
	  lStart(0),
#endif
	  rwcl(NULL)
{
	MutexLockGuard g(fdNodesMutex, FB_FUNCTION);

	DevNode id(getNode(fileName));

	if (id.f_ino)
	{
		CountedFd** got = fdNodes->get(id);
		if (got)
		{
			oFile = *got;
		}
	}

	if (!oFile)
	{
		int fd = os_utils::openCreateSharedFile(fileName, 0);
		oFile = FB_NEW(*getDefaultMemoryPool()) CountedFd(fd);
		CountedFd** put = fdNodes->put(getNode(fd));
		fb_assert(put);
		*put = oFile;

		if (init)
		{
			init(fd);
		}
	}

	rwcl = getRw();
	++(oFile->useCount);
}

#ifdef USE_FILELOCKS
FileLock::FileLock(const FileLock* main, int s)
	: level(LCK_NONE), oFile(main->oFile),
	  lStart(s), rwcl(getRw())
{
	MutexLockGuard g(fdNodesMutex, FB_FUNCTION);
	++(oFile->useCount);
}
#endif

FileLock::~FileLock()
{
	unlock();

	{ // guard scope
		MutexLockGuard g(rwlocksMutex, FB_FUNCTION);

		if (--(rwcl->cnt) == 0)
		{
			rwlocks->remove(getLockId());
			delete rwcl;
		}
	}
	{ // guard scope
		MutexLockGuard g(fdNodesMutex, FB_FUNCTION);

		if (--(oFile->useCount) == 0)
		{
			fdNodes->remove(getNode(oFile->fd));
			close(oFile->fd);
			delete oFile;
		}
	}
}

int FileLock::getFd()
{
	return oFile->fd;
}

int FileLock::setlock(const LockMode mode)
{
	bool shared = true, wait = true;
	switch (mode)
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
		return wait ? EBUSY : -1;
	}

	// first take appropriate rwlock to avoid conflicts with other threads in our process
	bool rc = true;
	try
	{
		switch (mode)
		{
		case FLM_TRY_EXCLUSIVE:
			rc = rwcl->rwlock.tryBeginWrite(FB_FUNCTION);
			break;
		case FLM_EXCLUSIVE:
			rwcl->rwlock.beginWrite(FB_FUNCTION);
			break;
		case FLM_TRY_SHARED:
			rc = rwcl->rwlock.tryBeginRead(FB_FUNCTION);
			break;
		case FLM_SHARED:
			rwcl->rwlock.beginRead(FB_FUNCTION);
			break;
		}
	}
	catch (const system_call_failed& fail)
	{
		return fail.getErrorCode();
	}
	if (!rc)
	{
		return -1;
	}

	// For shared lock we must take into an account reenterability
	MutexEnsureUnlock guard(rwcl->sharedAccessMutex, FB_FUNCTION);
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

#ifdef USE_FCNTL
	// Take lock on a file
	struct flock lock;
	lock.l_type = shared ? F_RDLCK : F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = lStart;
	lock.l_len = 1;
	if (fcntl(oFile->fd, wait ? F_SETLKW : F_SETLK, &lock) == -1)
	{
		int rc = errno;
		if (!wait && (rc == EACCES || rc == EAGAIN))
		{
			rc = -1;
		}
#else
	if (flock(oFile->fd, (shared ? LOCK_SH : LOCK_EX) | (wait ? 0 : LOCK_NB)))
	{
		int rc = errno;
		if (!wait && (rc == EWOULDBLOCK))
		{
			rc = -1;
		}
#endif

		try
		{
			if (shared)
			{
				rwcl->sharedAccessCounter--;
				rwcl->rwlock.endRead();
			}
			else
				rwcl->rwlock.endWrite();
		}
		catch (const Exception&)
		{ }

		return rc;
	}

	level = newLevel;
	return 0;
}

bool FileLock::setlock(Arg::StatusVector& status, const LockMode mode)
{
	int rc = setlock(mode);
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

void FileLock::rwUnlock()
{
	fb_assert(level != LCK_NONE);

	try
	{
		if (level == LCK_SHARED)
			rwcl->rwlock.endRead();
		else
			rwcl->rwlock.endWrite();
	}
	catch (const Exception& ex)
	{
		iscLogException("rwlock end-operation error", ex);
	}

	level = LCK_NONE;
}

void FileLock::unlock()
{
	if (level == LCK_NONE)
	{
		return;
	}

	// For shared lock we must take into an account reenterability
	MutexEnsureUnlock guard(rwcl->sharedAccessMutex, FB_FUNCTION);
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

#ifdef USE_FCNTL
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = lStart;
	lock.l_len = 1;

	if (fcntl(oFile->fd, F_SETLK, &lock) != 0)
	{
#else
	if (flock(oFile->fd, LOCK_UN) != 0)
	{
#endif
		Arg::StatusVector local;
		error(local, NAME, errno);
		iscLogStatus("Unlock error", local.value());
	}

	rwUnlock();
}

string FileLock::getLockId()
{
	fb_assert(oFile);

	DevNode id(getNode(oFile->fd));

	const size_t len1 = sizeof(id.f_dev);
	const size_t len2 = sizeof(id.f_ino);
#ifdef USE_FCNTL
	const size_t len3 = sizeof(int);
#endif

	string rc(len1 + len2
#ifdef USE_FCNTL
						  + len3
#endif
								, ' ');
	char* p = rc.begin();

	memcpy(p, &id.f_dev, len1);
	p += len1;
	memcpy(p, &id.f_ino, len2);
#ifdef USE_FCNTL
	p += len2;
	memcpy(p, &lStart, len3);
#endif

	return rc;
}

CountedRWLock* FileLock::getRw()
{
	string id = getLockId();
	CountedRWLock* rc = NULL;

	MutexLockGuard g(rwlocksMutex, FB_FUNCTION);

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


#ifdef USE_SYS5SEMAPHORE

#ifndef HAVE_SEMUN
union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
};
#endif

static SLONG	create_semaphores(Arg::StatusVector&, SLONG, int);

namespace {

	int sharedCount = 0;

	// this class is mapped into shared file
	class SemTable
	{
	public:
		const static int N_FILES = 128;
		const static int N_SETS = 256;
#if defined(DEV_BUILD) || defined(FREEBSD)
		const static int SEM_PER_SET = 4;	// force multiple sets allocation || work with default freebsd kernel
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

		void init(int fdSem)
		{
			if (sharedCount)
			{
				return;
			}

			FB_UNUSED(ftruncate(fdSem, sizeof(*this)));

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

	int idCache[SemTable::N_SETS];
	GlobalPtr<Mutex> idCacheMutex;

	void initCache()
	{
		MutexLockGuard guard(idCacheMutex, FB_FUNCTION);
		memset(idCache, 0xff, sizeof idCache);
	}

	void SemTable::cleanup(int fNum, bool release)
	{
		fb_assert(fNum > 0 && fNum <= N_FILES);

		if (release)
		{
			filesTable[fNum - 1].name[0] = 0;
		}

		MutexLockGuard guard(idCacheMutex, FB_FUNCTION);
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

	// Left from DEB_EVNT code, keep for a while 'as is'. To be cleaned up later!!!
	void initStart(const event_t* event) {}
	void initStop(const event_t* event, int code) {}
	void finiStart(const event_t* event) {}
	void finiStop(const event_t* event) {}

} // anonymous namespace

bool SharedMemoryBase::getSem5(Sys5Semaphore* sem)
{
	try
	{
		// Lock init file.
		FileLockHolder lock(initFile);

		if (!semTable->get(fileNum, sem))
		{
			gds__log("semTable->get() failed");
			return false;
		}

		return true;
	}
	catch (const Exception& ex)
	{
		iscLogException("FileLock ctor failed in getSem5", ex);
	}
	return false;
}

void SharedMemoryBase::freeSem5(Sys5Semaphore* sem)
{
	try
	{
		// Lock init file.
		FileLockHolder lock(initFile);

		semTable->put(sem);
	}
	catch (const Exception& ex)
	{
		iscLogException("FileLock ctor failed in freeSem5", ex);
	}
}

int Sys5Semaphore::getId()
{
	MutexLockGuard guard(idCacheMutex, FB_FUNCTION);
	fb_assert(semSet >= 0 && semSet < SemTable::N_SETS);

	int id = idCache[semSet];

	if (id < 0)
	{
		Arg::StatusVector status;
		id = create_semaphores(status, semTable->getKey(semSet), SemTable::SEM_PER_SET);
		if (id >= 0)
		{
			idCache[semSet] = id;
		}
		else
		{
			iscLogStatus("create_semaphores failed:", status.value());
		}
	}

	return id;
}
#endif // USE_SYS5SEMAPHORE

#endif // UNIX

#if defined(WIN_NT)
static bool make_object_name(TEXT*, size_t, const TEXT*, const TEXT*);
#endif


#ifdef USE_SYS5SEMAPHORE

namespace {

class TimerEntry : public Firebird::RefCntIface<Firebird::ITimer, FB_TIMER_VERSION>
{
public:
	TimerEntry(int id, USHORT num)
		: semId(id), semNum(num)
	{ }

	void FB_CARG handler()
	{
		for(;;)
		{
			union semun arg;
			arg.val = 0;
			int ret = semctl(semId, semNum, SETVAL, arg);
			if (ret != -1)
				break;
			if (!SYSCALL_INTERRUPTED(errno))
			{
				gds__log("semctl() failed, errno %d\n", errno);
				break;
			}
		}
	}

	int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}

		return 1;
	}

	bool operator== (Sys5Semaphore& sem)
	{
		return semId == sem.getId() && semNum == sem.semNum;
	}

private:
	int semId;
	USHORT semNum;
};

typedef HalfStaticArray<TimerEntry*, 64> TimerQueue;
GlobalPtr<TimerQueue> timerQueue;
GlobalPtr<Mutex> timerAccess;

void addTimer(Sys5Semaphore* sem, int microSeconds)
{
	TimerEntry* newTimer = new TimerEntry(sem->getId(), sem->semNum);
	{
		MutexLockGuard guard(timerAccess, FB_FUNCTION);
		timerQueue->push(newTimer);
	}
	TimerInterfacePtr()->start(newTimer, microSeconds);
}

void delTimer(Sys5Semaphore* sem)
{
	bool found = false;
	TimerEntry** t;

	{
		MutexLockGuard guard(timerAccess, FB_FUNCTION);

		for (t = timerQueue->begin(); t < timerQueue->end(); ++t)
		{
			if (**t == *sem)
			{
				timerQueue->remove(t);
				found = true;
				break;
			}
		}
	}

	if (found)
	{
		TimerInterfacePtr()->stop(*t);
	}
}

SINT64 curTime()
{
	struct timeval cur_time;
	struct timezone tzUnused;

	if (gettimeofday(&cur_time, &tzUnused) != 0)
	{
		system_call_failed::raise("gettimeofday");
	}

	SINT64 rc = ((SINT64) cur_time.tv_sec) * 1000000 + cur_time.tv_usec;
	return rc;
}

} // anonymous namespace

#endif // USE_SYS5SEMAPHORE

#ifdef USE_SHARED_FUTEX

namespace {

	int isPthreadError(int rc, const char* function)
	{
		if (rc == 0)
			return 0;
		iscLogStatus("Pthread Error",
			(Arg::Gds(isc_sys_request) << Arg::Str(function) << Arg::Unix(rc)).value());
		return rc;
	}

}

#define PTHREAD_ERROR(x) if (isPthreadError((x), #x)) return FB_FAILURE
#define PTHREAD_ERRNO(x) { int tmpState = (x); if (isPthreadError(tmpState, #x)) return tmpState; }
#define LOG_PTHREAD_ERROR(x) isPthreadError((x), #x)
#define PTHREAD_ERR_STATUS(x, v) { int tmpState = (x); if (tmpState) { error(v, #x, tmpState); return false; } }
#define PTHREAD_ERR_RAISE(x) { int tmpState = (x); if (tmpState) { system_call_failed(#x, tmpState); } }

#endif // USE_SHARED_FUTEX


int SharedMemoryBase::eventInit(event_t* event)
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

#if defined(WIN_NT)

	static AtomicCounter idCounter;

	event->event_id = ++idCounter;

	event->event_pid = process_id = getpid();
	event->event_count = 0;

	event->event_handle = ISC_make_signal(true, true, process_id, event->event_id);

	return (event->event_handle) ? FB_SUCCESS : FB_FAILURE;

#elif defined(USE_SYS5SEMAPHORE)

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
		initStop(event, 2);
		iscLogStatus("event_init()",
			(Arg::Gds(isc_sys_request) << Arg::Str("semctl") << SYS_ERR(errno)).value());
		return FB_FAILURE;
	}

	initStop(event, 0);
	return FB_SUCCESS;

#else // pthread-based event

	event->event_count = 0;
	event->pid = getpid();

	// Prepare an Inter-Process event block
	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;

	PTHREAD_ERROR(pthread_mutexattr_init(&mattr));
	PTHREAD_ERROR(pthread_condattr_init(&cattr));
#ifdef PTHREAD_PROCESS_SHARED
	PTHREAD_ERROR(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED));
	PTHREAD_ERROR(pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED));
#else
#error Your system must support PTHREAD_PROCESS_SHARED to use firebird.
#endif
	PTHREAD_ERROR(pthread_mutex_init(event->event_mutex, &mattr));
	PTHREAD_ERROR(pthread_cond_init(event->event_cond, &cattr));
	PTHREAD_ERROR(pthread_mutexattr_destroy(&mattr));
	PTHREAD_ERROR(pthread_condattr_destroy(&cattr));

	return FB_SUCCESS;

#endif // OS-dependent choice

}


void SharedMemoryBase::eventFini(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Discard an event object.
 *
 **************************************/

#if defined(WIN_NT)

	if (event->event_pid == process_id)
	{
		CloseHandle((HANDLE) event->event_handle);
	}

#elif defined(USE_SYS5SEMAPHORE)

	IPC_TRACE(("ISC_event_fini set=%d num=%d\n", event->semSet, event->semNum));
	finiStart(event);
	freeSem5(event);
	finiStop(event);

#else // pthread-based event

	if (event->pid == getpid())
	{
		LOG_PTHREAD_ERROR(pthread_mutex_destroy(event->event_mutex));
		LOG_PTHREAD_ERROR(pthread_cond_destroy(event->event_cond));
	}

#endif // OS-dependent choice

}


SLONG SharedMemoryBase::eventClear(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ c l e a r
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

#if defined(WIN_NT)

	ResetEvent((HANDLE) event->event_handle);

	return event->event_count + 1;

#elif defined(USE_SYS5SEMAPHORE)

	union semun arg;

	arg.val = 1;
	if (semctl(event->getId(), event->semNum, SETVAL, arg) < 0)
	{
		iscLogStatus("event_clear()",
			(Arg::Gds(isc_sys_request) << Arg::Str("semctl") << SYS_ERR(errno)).value());
	}

	return (event->event_count + 1);

#else // pthread-based event

	LOG_PTHREAD_ERROR(pthread_mutex_lock(event->event_mutex));
	const SLONG ret = event->event_count + 1;
	LOG_PTHREAD_ERROR(pthread_mutex_unlock(event->event_mutex));
	return ret;

#endif // OS-dependent choice

}


int SharedMemoryBase::eventWait(event_t* event, const SLONG value, const SLONG micro_seconds)
{
/**************************************
 *
 *	I S C _ e v e n t _ w a i t
 *
 **************************************
 *
 * Functional description
 *	Wait on an event.
 *
 **************************************/

	// If we're not blocked, the rest is a gross waste of time

	if (!event_blocked(event, value)) {
		return FB_SUCCESS;
	}

#if defined(WIN_NT)

	// Go into wait loop

	const DWORD timeout = (micro_seconds > 0) ? micro_seconds / 1000 : INFINITE;

	for (;;)
	{
		if (!event_blocked(event, value))
			return FB_SUCCESS;

		const DWORD status = WaitForSingleObject(event->event_handle, timeout);

		if (status != WAIT_OBJECT_0)
			return FB_FAILURE;
	}

#elif defined(USE_SYS5SEMAPHORE)

	// Set up timers if a timeout period was specified.
	SINT64 timeout = 0;
	if (micro_seconds > 0)
	{
		timeout = curTime() + micro_seconds;
		addTimer(event, micro_seconds);
	}

	// Go into wait loop

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

	// Cancel the handler.  We only get here if a timeout was specified.
	if (micro_seconds > 0)
	{
		delTimer(event);
	}

	return ret;

#else // pthread-based event

	// Set up timers if a timeout period was specified.

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

		// The Posix pthread_cond_wait & pthread_cond_timedwait calls
		// atomically release the mutex and start a wait.
		// The mutex is reacquired before the call returns.
		if (micro_seconds > 0)
		{
			ret = pthread_cond_timedwait(event->event_cond, event->event_mutex, &timer);

#if (defined LINUX || defined DARWIN || defined HP11 || defined FREEBSD)
			if (ret == ETIMEDOUT)
#else
			if (ret == ETIME)
#endif
			{

				// The timer expired - see if the event occurred and return
				// FB_SUCCESS or FB_FAILURE accordingly.

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

#endif // OS-dependent choice

}


int SharedMemoryBase::eventPost(event_t* event)
{
/**************************************
 *
 *	I S C _ e v e n t _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Post an event to wake somebody else up.
 *
 **************************************/

#if defined(WIN_NT)

	++event->event_count;

	if (event->event_pid != process_id)
		return ISC_kill(event->event_pid, event->event_id, event->event_handle);

	return SetEvent((HANDLE) event->event_handle) ? FB_SUCCESS : FB_FAILURE;

#elif defined(USE_SYS5SEMAPHORE)

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

#else // pthread-based event

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

#endif // OS-dependent choice

} // anonymous namespace


#ifdef UNIX
ULONG ISC_exception_post(ULONG sig_num, const TEXT* err_msg)
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

	return 0;	// compiler silencer
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
		status_exception::raise(Arg::Gds(isc_exception_stack_overflow));
		// This will never be called, but to be safe it's here
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
		// Pass these exception on to someone else, probably the OS or the debugger,
		// since there isn't a dam thing we can do with them
		result = EXCEPTION_CONTINUE_SEARCH;
		is_critical = false;
		break;
	case 0xE06D7363: // E == Exception. 0x6D7363 == "msc". Intel and Borland use the same code to be compatible
		// If we've caught our own software exception,
		// continue rewinding the stack to properly handle it
		// and deliver an error information to the client side
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


void SharedMemoryBase::removeMapFile()
{
#ifndef WIN_NT
	unlinkFile();
#else
	sh_mem_unlink = true;
#endif // WIN_NT
}

void SharedMemoryBase::unlinkFile()
{
	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, sh_mem_name);

	// We can't do much (specially in dtors) when it fails
	// therefore do not check for errors - at least it's just /tmp.
	unlink(expanded_filename);
}


#ifdef UNIX

void SharedMemoryBase::internalUnmap()
{
#ifdef USE_SYS5SEMAPHORE
	if (fileNum != -1 && mainLock.hasData())
	{
		Arg::StatusVector statusVector;
		semTable->cleanup(fileNum, mainLock->setlock(statusVector, FileLock::FLM_TRY_EXCLUSIVE));
	}
#endif
	if (sh_mem_header)
	{
		munmap(sh_mem_header, sh_mem_length_mapped);
		sh_mem_header = NULL;
	}
}

SharedMemoryBase::SharedMemoryBase(const TEXT* filename, ULONG length, IpcObject* callback)
	:
#ifdef HAVE_SHARED_MUTEX_SECTION
	sh_mem_mutex(0),
#endif
	sh_mem_length_mapped(0), sh_mem_header(NULL),
#ifdef USE_SYS5SEMAPHORE
	fileNum(-1),
#endif
	sh_mem_callback(callback)
{
/**************************************
 *
 *	c t o r		( U N I X - m m a p )
 *
 **************************************
 *
 * Functional description
 *	Try to map a given file.  If we are the first (i.e. only)
 *	process to map the file, call a given initialization
 *	routine (if given) or punt (leaving the file unmapped).
 *
 **************************************/
	Arg::StatusVector statusVector;

	sh_mem_name[0] = '\0';

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, filename);

	// make the complete filename for the init file this file is to be used as a
	// master lock to eliminate possible race conditions with just a single file
	// locking. The race condition is caused as the conversion of an EXCLUSIVE
	// lock to a SHARED lock is not atomic

	TEXT init_filename[MAXPATHLEN];
	gds__prefix_lock(init_filename, INIT_FILE);

	const bool trunc_flag = (length != 0);

	// open the init lock file
	MutexLockGuard guard(openFdInit, FB_FUNCTION);

	initFile.reset(FB_NEW(*getDefaultMemoryPool()) FileLock(init_filename));

	// get an exclusive lock on the INIT file with blocking
	FileLockHolder initLock(initFile);

#ifdef USE_SYS5SEMAPHORE
	class Sem5Init
	{
	public:
		static void init(int fd)
		{
			void* sTab = mmap(0, sizeof(SemTable), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if ((U_IPTR) sTab == (U_IPTR) -1)
			{
				system_call_failed::raise("mmap");
			}

			semTable = (SemTable*) sTab;
			initCache();
		}
	};


	TEXT sem_filename[MAXPATHLEN];
	gds__prefix_lock(sem_filename, SEM_FILE);

	semFile.reset(FB_NEW(*getDefaultMemoryPool()) FileLock(sem_filename, Sem5Init::init));

	fb_assert(semTable);

	if (semFile->setlock(statusVector, FileLock::FLM_TRY_EXCLUSIVE))
	{
		semTable->init(semFile->getFd());
		semFile->unlock();
	}
	if (!semFile->setlock(statusVector, FileLock::FLM_SHARED))
	{
		if (statusVector.hasData())
			statusVector.raise();
		else
			(Arg::Gds(isc_random) << "Unknown error in setlock").raise();
	}
#endif

	// create lock in order to have file autoclosed on error
	mainLock.reset(FB_NEW(*getDefaultMemoryPool()) FileLock(expanded_filename));

	if (length == 0)
	{
		// Get and use the existing length of the shared segment
		struct stat file_stat;
		if (fstat(mainLock->getFd(), &file_stat) == -1)
		{
			system_call_failed::raise("fstat");
		}
		length = file_stat.st_size;

		if (length == 0)
		{
			// keep old text of message here -  will be assigned a bit later
			(Arg::Gds(isc_random) << "shmem_data->sh_mem_length_mapped is 0").raise();
		}
	}

	// map file to memory
	void* const address = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, mainLock->getFd(), 0);
	if ((U_IPTR) address == (U_IPTR) -1)
	{
		system_call_failed::raise("mmap", errno);
	}

	// this class is needed to cleanup mapping in case of error
	class AutoUnmap
	{
	public:
		explicit AutoUnmap(SharedMemoryBase* sm) : sharedMemory(sm)
		{ }

		void success()
		{
			sharedMemory = NULL;
		}

		~AutoUnmap()
		{
			if (sharedMemory)
			{
				sharedMemory->internalUnmap();
			}
		}

	private:
		SharedMemoryBase* sharedMemory;
	};

	AutoUnmap autoUnmap(this);

	sh_mem_header = (MemoryHeader*) address;
	sh_mem_length_mapped = length;
	strcpy(sh_mem_name, filename);

#if defined(HAVE_SHARED_MUTEX_SECTION) && defined(USE_MUTEX_MAP)

	sh_mem_mutex = (mtx*) mapObject(statusVector, OFFSET(MemoryHeader*, mhb_mutex), sizeof(mtx));
	if (!sh_mem_mutex)
	{
		system_call_failed::raise("mmap");
	}

#endif

#if defined(USE_SYS5SEMAPHORE)
#if !defined(USE_FILELOCKS)

	sh_mem_mutex = &sh_mem_header->mhb_mutex;

#endif // USE_FILELOCKS

	fileNum = semTable->addFileByName(expanded_filename);

#endif // USE_SYS5SEMAPHORE

	// Try to get an exclusive lock on the lock file.  This will
	// fail if somebody else has the exclusive or shared lock

	if (mainLock->setlock(statusVector, FileLock::FLM_TRY_EXCLUSIVE))
	{
		if (trunc_flag)
			FB_UNUSED(ftruncate(mainLock->getFd(), length));

		if (callback->initialize(this, true))
		{

#ifdef HAVE_SHARED_MUTEX_SECTION
#ifdef USE_SYS5SEMAPHORE

			if (!getSem5(sh_mem_mutex))
			{
				callback->mutexBug(0, "getSem5()");
				(Arg::Gds(isc_random) << "getSem5() failed").raise();
			}

			union semun arg;
			arg.val = 1;
			int state = semctl(sh_mem_mutex->getId(), sh_mem_mutex->semNum, SETVAL, arg);
			if (state == -1)
			{
				int err = errno;
				callback->mutexBug(errno, "semctl");
				system_call_failed::raise("semctl", err);
			}

#else // USE_SYS5SEMAPHORE

#if (defined(HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL) || defined(USE_ROBUST_MUTEX)) && defined(LINUX)
// glibc in linux does not conform to the posix standard. When there is no RT kernel,
// ENOTSUP is returned not by pthread_mutexattr_setprotocol(), but by
// pthread_mutex_init(). Use a hack to deal with this broken error reporting.
#define BUGGY_LINUX_MUTEX
#endif

			int state = 0;

#ifdef BUGGY_LINUX_MUTEX
			static volatile bool staticBugFlag = false;

			do
			{
				bool bugFlag = staticBugFlag;
#endif

				pthread_mutexattr_t mattr;

				PTHREAD_ERR_RAISE(pthread_mutexattr_init(&mattr));
#ifdef PTHREAD_PROCESS_SHARED
				PTHREAD_ERR_RAISE(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED));
#else
#error Your system must support PTHREAD_PROCESS_SHARED to use pthread shared futex in Firebird.
#endif

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
#endif

#ifdef USE_ROBUST_MUTEX
#ifdef BUGGY_LINUX_MUTEX
				if (!bugFlag)
				{
#endif
					LOG_PTHREAD_ERROR(pthread_mutexattr_setrobust_np(&mattr, PTHREAD_MUTEX_ROBUST_NP));
#ifdef BUGGY_LINUX_MUTEX
				}
#endif
#endif

				memset(sh_mem_mutex->mtx_mutex, 0, sizeof(*(sh_mem_mutex->mtx_mutex)));
				//int state = LOG_PTHREAD_ERROR(pthread_mutex_init(sh_mem_mutex->mtx_mutex, &mattr));
				state = pthread_mutex_init(sh_mem_mutex->mtx_mutex, &mattr);

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

			} while (false);
#endif

			if (state)
			{
				callback->mutexBug(state, "pthread_mutex_init");
				system_call_failed::raise("pthread_mutex_init", state);
			}

#endif // USE_SYS5SEMAPHORE
#endif // HAVE_SHARED_MUTEX_SECTION

			mainLock->unlock();
			if (!mainLock->setlock(statusVector, FileLock::FLM_SHARED))
			{
				if (statusVector.hasData())
					status_exception::raise(statusVector);
				else
					(Arg::Gds(isc_random) << "Unknown error in setlock(SHARED)").raise();
			}
		}
	}
	else
	{
		if (callback->initialize(this, false))
		{
			if (!mainLock->setlock(statusVector, FileLock::FLM_SHARED))
			{
				if (statusVector.hasData())
					status_exception::raise(statusVector);
				else
					(Arg::Gds(isc_random) << "Unknown error in setlock(SHARED)").raise();
			}
		}
	}

#ifdef USE_FILELOCKS
	sh_mem_fileMutex.reset(FB_NEW(*getDefaultMemoryPool()) FileLock(mainLock, 1));
#endif

#ifdef USE_SYS5SEMAPHORE
	++sharedCount;
#endif

	autoUnmap.success();
}

#endif // UNIX


#ifdef WIN_NT
SharedMemoryBase::SharedMemoryBase(const TEXT* filename, ULONG length, IpcObject* cb)
  :	sh_mem_mutex(0), sh_mem_length_mapped(0),
	sh_mem_handle(0), sh_mem_object(0), sh_mem_interest(0), sh_mem_hdr_object(0),
	sh_mem_hdr_address(0), sh_mem_header(NULL), sh_mem_callback(cb), sh_mem_unlink(false)
{
/**************************************
 *
 *	c t o r		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Try to map a given file.  If we are the first (i.e. only)
 *	process to map the file, call a given initialization
 *	routine (if given) or punt (leaving the file unmapped).
 *
 **************************************/
	sh_mem_name[0] = '\0';

	ISC_mutex_init(&sh_mem_winMutex, filename);
	sh_mem_mutex = &sh_mem_winMutex;

	HANDLE file_handle;
	HANDLE event_handle = 0;
	int retry_count = 0;

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, filename);

	const bool trunc_flag = (length != 0);
	bool init_flag = false;

	// retry to attach to mmapped file if the process initializing dies during initialization.

  retry:
	if (retry_count++ > 0)
		THD_sleep(10);

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

		system_call_failed::raise("CreateFile");
	}

	// Check if file already exists

	const bool file_exists = (err == ERROR_ALREADY_EXISTS);

	// Create an event that can be used to determine if someone has already
	// initialized shared memory.

	TEXT object_name[MAXPATHLEN];
	if (!make_object_name(object_name, sizeof(object_name), filename, "_event"))
	{
		system_call_failed::raise("make_object_name");
	}

	if (!init_flag)
	{
		event_handle = CreateEvent(ISC_get_security_desc(), TRUE, FALSE, object_name);
		if (!event_handle)
		{
			DWORD err = GetLastError();
			CloseHandle(file_handle);
			system_call_failed::raise("CreateEvent", err);
		}

		init_flag = (GetLastError() != ERROR_ALREADY_EXISTS);

		if (init_flag && false)		// What's the crap? AP 2012
		{
			DWORD err = GetLastError();
			CloseHandle(event_handle);
			CloseHandle(file_handle);
			Arg::Gds(isc_unavailable).raise();
		}
	}

	if (length == 0)
	{
		// Get and use the existing length of the shared segment

		if ((length = GetFileSize(file_handle, NULL)) == -1)
		{
			DWORD err = GetLastError();
			CloseHandle(event_handle);
			CloseHandle(file_handle);
			system_call_failed::raise("GetFileSize", err);
		}
	}

	// All but the initializer will wait until the event is set.  That
	// is done after initialization is complete.
	// Close the file and wait for the event to be set or time out.
	// The file may be truncated.

	CloseHandle(file_handle);

	if (!init_flag)
	{
		// Wait for 10 seconds.  Then retry

		const DWORD ret_event = WaitForSingleObject(event_handle, 10000);

		// If we timed out, just retry.  It is possible that the
		// process doing the initialization died before setting the event.

		if (ret_event == WAIT_TIMEOUT)
		{
			CloseHandle(event_handle);
			if (retry_count > 10)
			{
				system_call_failed::raise("WaitForSingleObject", 0);
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

		CloseHandle(event_handle);

		if (err == ERROR_USER_MAPPED_FILE && init_flag && file_exists && trunc_flag)
			Arg::Gds(isc_instance_conflict).raise();
		else
			system_call_failed::raise("CreateFile", err);
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

	// Create a file mapping object that will be used to make remapping possible.
	// The current length of real mapped file and its name are saved in it.

	if (!make_object_name(object_name, sizeof(object_name), filename, "_mapping"))
	{
		DWORD err = GetLastError();
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("make_object_name", err);
	}

	HANDLE header_obj = CreateFileMapping(INVALID_HANDLE_VALUE,
										  ISC_get_security_desc(),
										  PAGE_READWRITE,
										  0, 2 * sizeof(ULONG),
										  object_name);
	if (header_obj == NULL)
	{
		DWORD err = GetLastError();
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("CreateFileMapping", err);
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
		DWORD err = GetLastError();
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("MapViewOfFile", err);
	}

	// Set or get the true length of the file depending on whether or not we are the first user.

	if (init_flag)
	{
		header_address[0] = length;
		header_address[1] = 0;
	}
	else
		length = header_address[0];

	// Create the real file mapping object.

	TEXT mapping_name[64]; // enough for int32 as text
	sprintf(mapping_name, "_mapping_%"ULONGFORMAT, header_address[1]);

	if (!make_object_name(object_name, sizeof(object_name), filename, mapping_name))
	{
		DWORD err = GetLastError();
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("make_object_name", err);
	}

	HANDLE file_obj = CreateFileMapping(file_handle,
										ISC_get_security_desc(),
										PAGE_READWRITE,
										0, length,
										object_name);
	if (file_obj == NULL)
	{
		DWORD err = GetLastError();
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("CreateFileMapping", err);
	}

	UCHAR* const address = (UCHAR*) MapViewOfFile(file_obj, FILE_MAP_WRITE, 0, 0, 0);

	if (address == NULL)
	{
		DWORD err = GetLastError();
		CloseHandle(file_obj);
		UnmapViewOfFile(header_address);
		CloseHandle(header_obj);
		CloseHandle(event_handle);
		CloseHandle(file_handle);
		system_call_failed::raise("MapViewOfFile", err);
	}

	sh_mem_header = (MemoryHeader*) address;
	sh_mem_length_mapped = length;

	if (!sh_mem_length_mapped)
	{
		(Arg::Gds(isc_random) << "sh_mem_length_mapped is 0").raise();
	}

	sh_mem_handle = file_handle;
	sh_mem_object = file_obj;
	sh_mem_interest = event_handle;
	sh_mem_hdr_object = header_obj;
	sh_mem_hdr_address = header_address;
	strcpy(sh_mem_name, filename);

	sh_mem_callback->initialize(this, init_flag);

	if (init_flag)
	{
		FlushViewOfFile(address, 0);

		DWORD err = 0;
		if (SetFilePointer(sh_mem_handle, length, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
			!SetEndOfFile(sh_mem_handle) ||
			!FlushViewOfFile(address, 0))
		{
			err = GetLastError();
		}

		SetEvent(event_handle);
		if (err)
		{
			system_call_failed::raise("SetFilePointer", err);
		}
	}
}
#endif


#ifdef HAVE_MMAP

UCHAR* SharedMemoryBase::mapObject(Arg::StatusVector& statusVector, ULONG object_offset, ULONG object_length)
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

	// Get system page size as this is the unit of mapping.

#ifdef SOLARIS
	const long ps = sysconf(_SC_PAGESIZE);
	if (ps == -1)
	{
		error(statusVector, "sysconf", errno);
		return NULL;
	}
#else
	const int ps = getpagesize();
	if (ps == -1)
	{
		error(statusVector, "getpagesize", errno);
		return NULL;
	}
#endif
	const ULONG page_size = (ULONG) ps;

	// Compute the start and end page-aligned offsets which contain the object being mapped.

	const ULONG start = (object_offset / page_size) * page_size;
	const ULONG end = FB_ALIGN(object_offset + object_length, page_size);
	const ULONG length = end - start;

	UCHAR* address = (UCHAR*) mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, mainLock->getFd(), start);

	if ((U_IPTR) address == (U_IPTR) -1)
	{
		error(statusVector, "mmap", errno);
		return NULL;
	}

	// Return the virtual address of the mapped object.

	IPC_TRACE(("ISC_map_object in %p to %p %p\n", shmem_data->sh_mem_address, address, address + length));

	return address + (object_offset - start);
}


void SharedMemoryBase::unmapObject(Arg::StatusVector& statusVector, UCHAR** object_pointer, ULONG object_length)
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
	// Get system page size as this is the unit of mapping.

#ifdef SOLARIS
	const long ps = sysconf(_SC_PAGESIZE);
	if (ps == -1)
	{
		error(statusVector, "sysconf", errno);
		return;
	}
#else
	const int ps = getpagesize();
	if (ps == -1)
	{
		error(statusVector, "getpagesize", errno);
		return;
	}
#endif
	const size_t page_size = (ULONG) ps;

	// Compute the start and end page-aligned addresses which contain the mapped object.

	char* const start = (char*) ((U_IPTR) (*object_pointer) & ~(page_size - 1));
	char* const end =
		(char*) ((U_IPTR) ((*object_pointer + object_length) + (page_size - 1)) & ~(page_size - 1));
	const size_t length = end - start;

	if (munmap(start, length) == -1)
	{
		error(statusVector, "munmap", errno);
		return; // false;
	}

	*object_pointer = NULL;
	return; // true;
}
#endif // HAVE_MMAP


#ifdef WIN_NT

UCHAR* SharedMemoryBase::mapObject(Arg::StatusVector& statusVector,
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
	const HANDLE handle = sh_mem_object;

	UCHAR* address = (UCHAR*) MapViewOfFile(handle, FILE_MAP_WRITE, 0, start, length);

	if (address == NULL)
	{
		error(statusVector, "MapViewOfFile", GetLastError());
		return NULL;
	}

	// Return the virtual address of the mapped object.

	return (address + (object_offset - start));
}


void SharedMemoryBase::unmapObject(Arg::StatusVector& statusVector,
								   UCHAR** object_pointer, ULONG /*object_length*/)
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
		error(statusVector, "UnmapViewOfFile", GetLastError());
		return;
	}

	*object_pointer = NULL;
}
#endif


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
		if (hLib)
			fnSwitchToThread = (pfnSwitchToThread) GetProcAddress(hLib, "SwitchToThread");

		bInit = true;
	}

	BOOL res = FALSE;

	if (fnSwitchToThread)
	{
		const HANDLE hThread = GetCurrentThread();
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

		res = (*fnSwitchToThread)();

		SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
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
			lpSect->lOwnerPID = pid;
#ifdef DEV_BUILD
			lpSect->lThreadId = GetCurrentThreadId();
#endif
			unlockSharedSection(lpSect);
			return WAIT_OBJECT_0;
		}

		if (dwMilliseconds == 0)
		{
			unlockSharedSection(lpSect);
			return WAIT_TIMEOUT;
		}

		InterlockedIncrement(FIX_TYPE(&lpSect->lThreadsWaiting));
		unlockSharedSection(lpSect);

		// TODO actual timeout can be of any length
		const DWORD tm = (dwMilliseconds == INFINITE || dwMilliseconds > 5000) ? 5000 : dwMilliseconds;
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
#ifdef DEV_BUILD
				gds__log("enterFastMutex: dead process detected, pid = %d", lpSect->lOwnerPID);
				lpSect->lThreadId = 0;
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
	fb_assert(lpSect->lOwnerPID == pid);
	lpSect->lOwnerPID = -lpSect->lOwnerPID;
#ifdef DEV_BUILD
	fb_assert(lpSect->lThreadId == GetCurrentThreadId());
	lpSect->lThreadId = -lpSect->lThreadId;
#endif
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
#ifdef DEV_BUILD
					lpMutex->lpSharedInfo->lThreadId = bInitialState ? GetCurrentThreadId() : 0;
#endif
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
bool SharedMemoryBase::remapFile(Arg::StatusVector& statusVector, ULONG new_length, bool flag)
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
	if (!new_length)
	{
		error(statusVector, "Zero new_length is requested", 0);
		return false;
	}

	if (flag)
		FB_UNUSED(ftruncate(mainLock->getFd(), new_length));

	MemoryHeader* const address = (MemoryHeader*)
		mmap(0, new_length, PROT_READ | PROT_WRITE, MAP_SHARED, mainLock->getFd(), 0);
	if ((U_IPTR) address == (U_IPTR) -1)
	{
		error(statusVector, "mmap() failed", errno);
		return false;
	}

	munmap(sh_mem_header, sh_mem_length_mapped);

	IPC_TRACE(("ISC_remap_file %p to %p %d\n", sh_mem_header, address, new_length));

	sh_mem_header = (MemoryHeader*) address;
	sh_mem_length_mapped = new_length;

#if defined(HAVE_SHARED_MUTEX_SECTION) && !defined(USE_MUTEX_MAP)
	sh_mem_mutex = &sh_mem_header->mhb_mutex;
#endif

	return address;
}
#endif // HAVE_MMAP
#endif // UNIX


#ifdef WIN_NT
#define ISC_REMAP_FILE_DEFINED
bool SharedMemoryBase::remapFile(Arg::StatusVector& statusVector,
								 ULONG new_length, bool flag)
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
		if (SetFilePointer(sh_mem_handle, new_length, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
			!SetEndOfFile(sh_mem_handle) ||
			!FlushViewOfFile(sh_mem_header, 0))
		{
			error(statusVector, "SetFilePointer", GetLastError());
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
		sprintf(mapping_name, "_mapping_%"ULONGFORMAT, sh_mem_hdr_address[1] + 1);

		TEXT object_name[MAXPATHLEN];
		if (!make_object_name(object_name, sizeof(object_name), sh_mem_name, mapping_name))
			break;

		file_obj = CreateFileMapping(sh_mem_handle,
									 ISC_get_security_desc(),
									 PAGE_READWRITE,
									 0, new_length,
									 object_name);

		if (!(GetLastError() == ERROR_ALREADY_EXISTS && flag))
			break;

		CloseHandle(file_obj);
		file_obj = NULL;
		sh_mem_hdr_address[1]++;
	}

	if (file_obj == NULL)
	{
		error(statusVector, "CreateFileMapping", GetLastError());
		return NULL;
	}

	MemoryHeader* const address = (MemoryHeader*) MapViewOfFile(file_obj, FILE_MAP_WRITE, 0, 0, 0);

	if (address == NULL)
	{
		error(statusVector, "MapViewOfFile", GetLastError());
		CloseHandle(file_obj);
		return NULL;
	}

	if (flag)
	{
		sh_mem_hdr_address[0] = new_length;
		sh_mem_hdr_address[1]++;
	}

	UnmapViewOfFile(sh_mem_header);
	CloseHandle(sh_mem_object);

	sh_mem_header = (MemoryHeader*) address;
	sh_mem_length_mapped = new_length;
	sh_mem_object = file_obj;

	if (!sh_mem_length_mapped)
	{
		error(statusVector, "sh_mem_length_mapped is 0", 0);
		return NULL;
	}

	return (address);
}
#endif


#ifndef ISC_REMAP_FILE_DEFINED
bool SharedMemoryBase::remapFile(Arg::StatusVector& statusVector, ULONG, bool)
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

	statusVector << Arg::Gds(isc_unavailable) <<
					Arg::Gds(isc_random) << "SharedMemory::remapFile";

	return NULL;
}
#endif


#ifdef USE_SYS5SEMAPHORE

static SLONG create_semaphores(Arg::StatusVector& statusVector, SLONG key, int semaphores)
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
				error(statusVector, "semget", errno);
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
				error(statusVector, "semctl", errno);
				return -1;
			}
			if ((int) buf.sem_nsems >= semaphores)
				return semid;
			// Number of semaphores in existing set is too small. Discard it.
			if (semctl(semid, 0, IPC_RMID) == -1)
			{
				error(statusVector, "semctl", errno);
				return -1;
			}
		}

		// Try to create new semaphore set
		semid = semget(key, semaphores, IPC_CREAT | IPC_EXCL | PRIV);
		if (semid != -1)
		{
			// We want to limit access to semaphores, created here
			// Reasonable access rights to them - exactly like security database has
			const char* secDb = Config::getDefaultConfig()->getSecurityDatabase();
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
			error(statusVector, "semget", errno);
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


void SharedMemoryBase::mutexLock()
{
#if defined(WIN_NT)

	int state = ISC_mutex_lock(sh_mem_mutex);

#elif defined(USE_FILELOCKS)

	int state = 0;
	try
	{
		localMutex.enter(FB_FUNCTION);
	}
	catch (const system_call_failed& fail)
	{
		state = fail.getErrorCode();
	}
	if (!state)
	{
		state = sh_mem_fileMutex->setlock(FileLock::FLM_EXCLUSIVE);
		if (state)
		{
			try
			{
				localMutex.leave();
			}
			catch (const Exception&)
			{ }
		}
	}

#elif defined(USE_SYS5SEMAPHORE)

	struct sembuf sop;
	sop.sem_num = sh_mem_mutex->semNum;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO;

	int state;
	for (;;)
	{
		state = semop(sh_mem_mutex->getId(), &sop, 1);
		if (state == 0)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
		{
			state = errno;
			break;
		}
	}

#else // POSIX SHARED MUTEX

	int state = pthread_mutex_lock(sh_mem_mutex->mtx_mutex);
#ifdef USE_ROBUST_MUTEX
	if (state == EOWNERDEAD)
	{
		// We always perform check for dead process
		// Therefore may safely mark mutex as recovered
		LOG_PTHREAD_ERROR(pthread_mutex_consistent_np(sh_mem_mutex->mtx_mutex));
		state = 0;
	}
#endif

#endif // os-dependent choice

	if (state != 0)
	{
		sh_mem_callback->mutexBug(state, "mutexLock");
	}
}


bool SharedMemoryBase::mutexLockCond()
{
#if defined(WIN_NT)

	return ISC_mutex_lock_cond(sh_mem_mutex) == 0;

#elif defined(USE_FILELOCKS)

	try
	{
		if (!localMutex.tryEnter(FB_FUNCTION))
		{
			return false;
		}
	}
	catch (const system_call_failed& fail)
	{
		int state = fail.getErrorCode();
		sh_mem_callback->mutexBug(state, "mutexLockCond");
		return false;
	}

	bool rc = (sh_mem_fileMutex->setlock(FileLock::FLM_TRY_EXCLUSIVE) == 0);
	if (!rc)
	{
		try
		{
			localMutex.leave();
		}
		catch (const Exception&)
		{ }
	}
	return rc;

#elif defined(USE_SYS5SEMAPHORE)

	struct sembuf sop;
	sop.sem_num = sh_mem_mutex->semNum;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO | IPC_NOWAIT;

	for (;;)
	{
		int state = semop(sh_mem_mutex->getId(), &sop, 1);
		if (state == 0)
			return true;
		if (!SYSCALL_INTERRUPTED(errno))
			return false;
	}

#else // POSIX SHARED MUTEX

	int state = pthread_mutex_trylock(sh_mem_mutex->mtx_mutex);
#ifdef USE_ROBUST_MUTEX
	if (state == EOWNERDEAD)
	{
		// We always perform check for dead process
		// Therefore may safely mark mutex as recovered
		LOG_PTHREAD_ERROR(pthread_mutex_consistent_np(sh_mem_mutex->mtx_mutex));
		state = 0;
	}
#endif
	return state == 0;

#endif // os-dependent choice

}


void SharedMemoryBase::mutexUnlock()
{
#if defined(WIN_NT)

	int state = ISC_mutex_unlock(sh_mem_mutex);

#elif defined(USE_FILELOCKS)

	int state = 0;
	try
	{
		localMutex.leave();
	}
	catch (const system_call_failed& fail)
	{
		state = fail.getErrorCode();
	}
	if (!state)
	{
		sh_mem_fileMutex->unlock();
	}

#elif defined(USE_SYS5SEMAPHORE)

	struct sembuf sop;
	sop.sem_num = sh_mem_mutex->semNum;
	sop.sem_op = 1;
	sop.sem_flg = SEM_UNDO;

	int state;
	for (;;)
	{
		state = semop(sh_mem_mutex->getId(), &sop, 1);
		if (state == 0)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
		{
			state = errno;
			break;
		}
	}

#else // POSIX SHARED MUTEX

	int state = pthread_mutex_unlock(sh_mem_mutex->mtx_mutex);

#endif // os-dependent choice

	if (state != 0)
	{
		sh_mem_callback->mutexBug(state, "mutexUnlock");
	}
}


SharedMemoryBase::~SharedMemoryBase()
{
/**************************************
 *
 *	I S C _ u n m a p _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Unmap a given file.
 *
 **************************************/

#ifdef USE_SYS5SEMAPHORE
	// freeSem5(sh_mem_mutex);		no need - all set of semaphores will be gone

	try
	{
		// Lock init file.
		FileLockHolder initLock(initFile);

		Arg::StatusVector statusVector;		// ignored
		mainLock->unlock();
		semTable->cleanup(fileNum, mainLock->setlock(statusVector, FileLock::FLM_TRY_EXCLUSIVE));
	}
	catch (const Exception& ex)
	{
		iscLogException("ISC_unmap_file failed to lock init file", ex);
	}

	--sharedCount;
#endif

#if defined(HAVE_SHARED_MUTEX_SECTION) && defined(USE_MUTEX_MAP)
	Arg::StatusVector statusVector;
	unmapObject(statusVector, (UCHAR**) &sh_mem_mutex, sizeof(mtx));
	if (statusVector.hasData())
	{
		iscLogStatus("unmapObject failed", statusVector.value());
	}
#endif

#ifdef UNIX
	internalUnmap();
#endif

#ifdef WIN_NT
	Arg::StatusVector statusVector;
	CloseHandle(sh_mem_interest);
	if (!UnmapViewOfFile(sh_mem_header))
	{
		error(statusVector, "UnmapViewOfFile", GetLastError());
		return;
	}
	CloseHandle(sh_mem_object);

	CloseHandle(sh_mem_handle);
	if (!UnmapViewOfFile(sh_mem_hdr_address))
	{
		error(statusVector, "UnmapViewOfFile", GetLastError());
		return;
	}
	CloseHandle(sh_mem_hdr_object);

	TEXT expanded_filename[MAXPATHLEN];
	gds__prefix_lock(expanded_filename, sh_mem_name);

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

	ISC_mutex_fini(&sh_mem_winMutex);
	sh_mem_mutex = NULL;

	sh_mem_header = NULL;

	if (sh_mem_unlink)
	{
		unlinkFile();
	}
#endif
}

void SharedMemoryBase::logError(const char* text, const Arg::StatusVector& status)
{
	iscLogStatus(text, status.value());
}


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


static void error(Arg::StatusVector& statusVector, const TEXT* string, ISC_STATUS status)
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
	statusVector << Arg::Gds(isc_sys_request) << Arg::Str(string) << SYS_ERR(status);
	statusVector.makePermanent();
}
