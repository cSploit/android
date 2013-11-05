/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		GlobalRWLock.h
 *	DESCRIPTION:	Global Read Write Lock
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
 *  Copyright (c) 2006 Nickolay Samofatov
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s):
 *
 *	Roman Simakov <roman-simakov@users.sourceforge.net>
 *
 */

#ifndef GLOBAL_RW_LOCK
#define GLOBAL_RW_LOCK

#include "../common/classes/alloc.h"
#include "../jrd/jrd.h"
#include "../jrd/lck.h"
#include "../jrd/lck_proto.h"
#include "../include/fb_types.h"
#include "os/pio.h"
#include "../common/classes/condition.h"

//#define COS_DEBUG

#ifdef COS_DEBUG
DEFINE_TRACE_ROUTINE(cos_trace);
#define COS_TRACE(args) cos_trace args
#define COS_TRACE_AST(message) gds__trace(message)
#else
#define COS_TRACE(args) /* nothing */
#define COS_TRACE_AST(message) /* nothing */
#endif


namespace Jrd {

typedef USHORT locktype_t;

class GlobalRWLock : public Firebird::PermanentStorage
{
public:
	GlobalRWLock(thread_db* tdbb, MemoryPool& p, locktype_t lckType,
			lck_owner_t lock_owner, bool lock_caching = true,
			size_t lockLen = 0, const UCHAR* lockStr = NULL);

	virtual ~GlobalRWLock();

	// This function returns false if it cannot take the lock
	bool lockWrite(thread_db* tdbb, SSHORT wait);
	void unlockWrite(thread_db* tdbb);
	bool lockRead(thread_db* tdbb, SSHORT wait, const bool queueJump = false);
	void unlockRead(thread_db* tdbb);
	bool tryReleaseLock(thread_db* tdbb);
	void shutdownLock();

protected:
	// Flag to indicate that somebody is waiting via lock manager.
	// If somebody uses lock manager, all concurrent requests should also
	// go via lock manager to prevent starvation.
	Lock* cachedLock;

	// Load the object from shared location.
	virtual bool fetch(thread_db* /*tdbb*/) { return true; }
	virtual void invalidate(thread_db* /*tdbb*/)
	{
		fb_assert(readers == 0);
		blocking = false;
	}

	virtual void blockingAstHandler(thread_db* tdbb);

private:
	Firebird::Mutex counterMutex;	// Protects counter and blocking flag
	ULONG pendingLock;

	ULONG	readers;
	Firebird::Condition noReaders;		// Semaphore to wait all readers unlock to start relock

	ULONG	pendingWriters;
	bool	currentWriter;
	Firebird::Condition	writerFinished;

	// true - unlock keep cached lock and release by AST.
	// false - unlock releases cached lock if possible
	bool	lockCaching;
	bool	blocking; // Unprocessed AST pending

	static int blocking_ast_cached_lock(void* ast_object);
};

}

#endif // GLOBAL_RW_LOCK
