/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			rwlock.h
 *	DESCRIPTION:	Read/write multi-state locks
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 *
 */

#ifndef CLASSES_RWLOCK_H
#define CLASSES_RWLOCK_H

#ifdef WIN_NT

#include <windows.h>
#include <limits.h>

#include "../common/classes/fb_atomic.h"
#include "../common/classes/locks.h"


namespace Firebird
{

const int LOCK_WRITER_OFFSET = 50000;

// Should work pretty fast if atomic operations are native.
// This is not the case for Win95
class RWLock
{
private:
	AtomicCounter lock; // This is the actual lock
	           // -50000 - writer is active
			   // 0 - noone owns the lock
			   // positive value - number of concurrent readers
	long blockedReaders;
	AtomicCounter blockedWriters;
	Mutex blockedReadersLock;
	HANDLE writers_event, readers_semaphore;

	void init()
	{
		lock.setValue(0);
		blockedReaders = 0;
		blockedWriters.setValue(0);
		readers_semaphore = CreateSemaphore(NULL, 0 /*initial count*/, INT_MAX, NULL);
		if (readers_semaphore == NULL)
			system_call_failed::raise("CreateSemaphore");
		writers_event = CreateEvent(NULL, FALSE/*auto-reset*/, FALSE, NULL);
		if (writers_event == NULL)
			system_call_failed::raise("CreateEvent");
	}

	// Forbid copy constructor
	RWLock(const RWLock& source);

public:
	RWLock() { init(); }
	explicit RWLock(Firebird::MemoryPool&) { init(); }
	~RWLock()
	{
		if (readers_semaphore && !CloseHandle(readers_semaphore))
			system_call_failed::raise("CloseHandle");
		if (writers_event && !CloseHandle(writers_event))
			system_call_failed::raise("CloseHandle");
	}
	// Returns negative value if writer is active.
	// Otherwise returns a number of readers
	LONG getState() const
	{
		return lock.value();
	}
	void unblockWaiting()
	{
		if (blockedWriters.value()) {
			if (!SetEvent(writers_event))
				system_call_failed::raise("SetEvent");
		}
		else if (blockedReaders) {
			MutexLockGuard guard(blockedReadersLock);
			if (blockedReaders && !ReleaseSemaphore(readers_semaphore, blockedReaders, NULL))
			{
				system_call_failed::raise("ReleaseSemaphore");
			}
		}
	}
	bool tryBeginRead()
	{
		if (lock.value() < 0)
			return false;
		if (++lock > 0)
			return true;
		// We stepped on writer's toes. Fix our mistake
		if (--lock == 0)
			unblockWaiting();
		return false;
	}
	bool tryBeginWrite()
	{
		if (lock.value())
			return false;
		if (lock.exchangeAdd(-LOCK_WRITER_OFFSET) == 0)
			return true;
		// We stepped on somebody's toes. Fix our mistake
		if (lock.exchangeAdd(LOCK_WRITER_OFFSET) == -LOCK_WRITER_OFFSET)
			unblockWaiting();
		return false;
	}
	void beginRead()
	{
		if (!tryBeginRead())
		{
			{ // scope block
				MutexLockGuard guard(blockedReadersLock);
				++blockedReaders;
			}
			while (!tryBeginRead())
			{
				if (WaitForSingleObject(readers_semaphore, INFINITE) != WAIT_OBJECT_0)
					system_call_failed::raise("WaitForSingleObject");
			}
			{ // scope block
				MutexLockGuard guard(blockedReadersLock);
				--blockedReaders;
			}
		}
	}
	void beginWrite()
	{
		if (!tryBeginWrite())
		{
			++blockedWriters;
			while (!tryBeginWrite())
			{
				if (WaitForSingleObject(writers_event, INFINITE) != WAIT_OBJECT_0)
					system_call_failed::raise("WaitForSingleObject");
			}
			--blockedWriters;
		}
	}
	void endRead()
	{
		if (--lock == 0)
			unblockWaiting();
	}
	void endWrite()
	{
		if (lock.exchangeAdd(LOCK_WRITER_OFFSET) == -LOCK_WRITER_OFFSET)
			unblockWaiting();
	}
};


} // namespace Firebird

#else

#include <pthread.h>
#include <errno.h>

namespace Firebird
{

class MemoryPool;

class RWLock
{
private:
	pthread_rwlock_t lock;
	// Forbid copy constructor
	RWLock(const RWLock& source);

	void init()
	{
#if defined(LINUX) && !defined(USE_VALGRIND)
		pthread_rwlockattr_t attr;
		if (pthread_rwlockattr_init(&attr))
			system_call_failed::raise("pthread_rwlockattr_init");
		// Do not worry if target misses support for this option
		//pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
		if (pthread_rwlock_init(&lock, NULL))
			system_call_failed::raise("pthread_rwlock_init");
		if (pthread_rwlockattr_destroy(&attr))
			system_call_failed::raise("pthread_rwlockattr_destroy");
#else
		if (pthread_rwlock_init(&lock, NULL))
			system_call_failed::raise("pthread_rwlock_init");
#endif
	}

public:
	RWLock() { init(); }
	explicit RWLock(class MemoryPool&) { init(); }
	~RWLock()
	{
		if (pthread_rwlock_destroy(&lock))
			system_call_failed::raise("pthread_rwlock_destroy");
	}
	void beginRead()
	{
		if (pthread_rwlock_rdlock(&lock))
			system_call_failed::raise("pthread_rwlock_rdlock");
	}
	bool tryBeginRead()
	{
		const int code = pthread_rwlock_tryrdlock(&lock);
		if (code == EBUSY)
			return false;
		if (code)
			system_call_failed::raise("pthread_rwlock_tryrdlock");
		return true;
	}
	void endRead()
	{
		if (pthread_rwlock_unlock(&lock))
			system_call_failed::raise("pthread_rwlock_unlock");
	}
	bool tryBeginWrite()
	{
		const int code = pthread_rwlock_trywrlock(&lock);
		if (code == EBUSY)
			return false;
		if (code)
			system_call_failed::raise("pthread_rwlock_trywrlock");
		return true;
	}
	void beginWrite()
	{
		if (pthread_rwlock_wrlock(&lock))
			system_call_failed::raise("pthread_rwlock_wrlock");
	}
	void endWrite()
	{
		if (pthread_rwlock_unlock(&lock))
			system_call_failed::raise("pthread_rwlock_unlock");
	}
};

} // namespace Firebird

#endif // !WIN_NT

namespace Firebird {

// RAII holder of read lock
class ReadLockGuard
{
public:
	ReadLockGuard(RWLock &alock)
		: lock(&alock)
	{
		lock->beginRead();
	}
	~ReadLockGuard() { release(); }

	void release()
	{
		if (lock)
		{
			lock->endRead();
			lock = NULL;
		}
	}

private:
	// Forbid copy constructor
	ReadLockGuard(const ReadLockGuard& source);
	RWLock *lock;
};

// RAII holder of write lock
class WriteLockGuard
{
public:
	WriteLockGuard(RWLock &alock)
		: lock(&alock)
	{
		lock->beginWrite();
	}
	~WriteLockGuard() { release(); }

	void release()
	{
		if (lock)
		{
			lock->endWrite();
			lock = NULL;
		}
	}

private:
	// Forbid copy constructor
	WriteLockGuard(const WriteLockGuard& source);
	RWLock *lock;
};

} // namespace Firebird

#endif // #ifndef CLASSES_RWLOCK_H

