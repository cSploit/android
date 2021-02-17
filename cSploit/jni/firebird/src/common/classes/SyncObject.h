/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 *
 *  The Code was ported into Firebird Open Source RDBMS project by
 *  Vladyslav Khorsun at 2010
 *
 *  Contributor(s):
 */

#ifndef CLASSES_SYNCOBJECT_H
#define CLASSES_SYNCOBJECT_H

#include "../../common/classes/fb_atomic.h"
#include "../../common/classes/locks.h"
#include "../../common/classes/Reasons.h"

namespace Firebird {


enum SyncType {
	SYNC_NONE,
	SYNC_EXCLUSIVE,
	SYNC_SHARED,
	SYNC_INVALID
};

class Sync;
class ThreadSync;

class SyncObject : public Reasons
{
public:
	SyncObject()
		: waiters(0),
		  monitorCount(0),
		  exclusiveThread(NULL),
		  waitingThreads(NULL)
	{
	}

	~SyncObject()
	{
	}

	void lock(Sync* sync, SyncType type, const char* from)
	{
		const bool ret = lock(sync, type, from, -1);
		fb_assert(ret);
	}

	// timeOut is in milliseconds, -1 - wait infinite
	bool lock(Sync* sync, SyncType type, const char* from, int timeOut);
	bool lockConditional(SyncType type, const char* from);

	void unlock(Sync* sync, SyncType type);

	void unlock()
	{
		if (lockState > 0)
			unlock(NULL, SYNC_SHARED);
		else if (lockState == -1)
			unlock(NULL, SYNC_EXCLUSIVE);
		else
		{
			fb_assert(false);
		}
	}

	void downgrade(SyncType type);

	SyncType getState() const
	{
		if (lockState.value() == 0)
			return SYNC_NONE;

		if (lockState.value() < 0)
			return SYNC_EXCLUSIVE;

		return SYNC_SHARED;
	}

	bool isLocked() const
	{
		return lockState.value() != 0;
	}

	bool hasContention() const
	{
		return (waiters.value() > 0);
	}

	bool ourExclusiveLock() const;

protected:
	bool wait(SyncType type, ThreadSync* thread, Sync* sync, int timeOut);
	ThreadSync* dequeThread(ThreadSync* thread);
	ThreadSync* grantThread(ThreadSync* thread);
	void grantLocks();
	void validate(SyncType lockType) const;

	AtomicCounter lockState;
	AtomicCounter waiters;
	int monitorCount;
	Mutex mutex;
	ThreadSync* volatile exclusiveThread;
	ThreadSync* volatile waitingThreads;
};


class Sync
{
friend class ThreadSync;

public:
	Sync(SyncObject* obj, const char* fromWhere)
		: state(SYNC_NONE),
		  request(SYNC_NONE),
		  syncObject(obj),
		  where(fromWhere)
	{
		fb_assert(obj);
	}

	~Sync()
	{
		fb_assert(state != SYNC_INVALID);

		if (syncObject && state != SYNC_NONE)
		{
			syncObject->unlock(this, state);
			state = SYNC_INVALID;
		}
	}

	void lock(SyncType type)
	{
		request = type;
		syncObject->lock(this, type, where);
		state = type;
	}

	void lock(SyncType type, const char* fromWhere)
	{
		where = fromWhere;
		lock(type);
	}

	bool lockConditional(SyncType type)
	{
		request = type;

		if (!syncObject->lockConditional(type, where))
		{
			request = SYNC_NONE;
			return false;
		}

		state = type;
		return true;
	}

	bool lockConditional(SyncType type, const char* fromWhere)
	{
		where = fromWhere;
		return lockConditional(type);
	}

	void unlock()
	{
		fb_assert(state != SYNC_NONE);
		syncObject->unlock(this, state);
		state = SYNC_NONE;
	}

	void downgrade(SyncType type)
	{
		fb_assert(state == SYNC_EXCLUSIVE);
		syncObject->downgrade(type);
		state = SYNC_SHARED;
	}

	void setObject(SyncObject* obj)
	{
		if (syncObject && state != SYNC_NONE)
			syncObject->unlock(this, state);

		state = SYNC_NONE;
		syncObject = obj;
	}

	SyncType getState() const
	{
		return state;
	}

protected:
	SyncType state;
	SyncType request;
	SyncObject* syncObject;
	const char* where;
};


class SyncLockGuard : public Sync
{
public:
	SyncLockGuard(SyncObject* obj, SyncType type, const char* fromWhere)
		: Sync(obj, fromWhere)
	{
		lock(type);
	}

	~SyncLockGuard()
	{
		if (state != SYNC_NONE)
			unlock();
	}
};

class SyncUnlockGuard
{
public:
	explicit SyncUnlockGuard(Sync& aSync)
		: sync(aSync)
	{
		oldState = sync.getState();

		fb_assert(oldState != SYNC_NONE);
		if (oldState != SYNC_NONE)
			sync.unlock();
	}

	~SyncUnlockGuard()
	{
		if (oldState != SYNC_NONE)
			sync.lock(oldState);
	}

private:
	SyncType oldState;
	Sync& sync;
};

} // namespace Firebird

#endif // CLASSES_SYNCOBJECT_H
