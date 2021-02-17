/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		GlobalRWLock.cpp
 *	DESCRIPTION:	GlobalRWLock
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
 *	Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *
 */

#include "firebird.h"
#include "GlobalRWLock.h"
#include "../lock/lock_proto.h"
#include "../common/isc_proto.h"
#include "jrd.h"
#include "lck_proto.h"
#include "err_proto.h"
#include "Attachment.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/condition.h"

#ifdef COS_DEBUG
#include <stdarg.h>
IMPLEMENT_TRACE_ROUTINE(cos_trace, "COS")
#endif

using namespace Firebird;
using namespace Jrd;


int GlobalRWLock::blocking_ast_cached_lock(void* ast_object)
{
	GlobalRWLock* globalRWLock = static_cast<GlobalRWLock*>(ast_object);

	try
	{
		if (!globalRWLock->cachedLock)
			return 0;

		Database* const dbb = globalRWLock->cachedLock->lck_dbb;
		AsyncContextHolder tdbb(dbb, FB_FUNCTION);

		MutexLockGuard counterGuard(globalRWLock->counterMutex, FB_FUNCTION);
		globalRWLock->blockingAstHandler(tdbb);
	}
	catch (const Exception&)
	{} // no-op

	return 0;
}

GlobalRWLock::GlobalRWLock(thread_db* tdbb, MemoryPool& p, lck_t lckType,
						   bool lock_caching, size_t lockLen, const UCHAR* lockStr)
	: PermanentStorage(p), pendingLock(0), readers(0), pendingWriters(0), currentWriter(false),
	  lockCaching(lock_caching), blocking(false)
{
	SET_TDBB(tdbb);

	cachedLock = FB_NEW_RPT(getPool(), lockLen)
		Lock(tdbb, lockLen, lckType, this, lockCaching ? blocking_ast_cached_lock : NULL);
	memcpy(&cachedLock->lck_key, lockStr, lockLen);
}

GlobalRWLock::~GlobalRWLock()
{
	delete cachedLock;
}

void GlobalRWLock::shutdownLock(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	Attachment* const att = tdbb->getAttachment();
	Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

	COS_TRACE(("(%p)->shutdownLock readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

	LCK_release(tdbb, cachedLock);
}

bool GlobalRWLock::lockWrite(thread_db* tdbb, SSHORT wait)
{
	SET_TDBB(tdbb);

	Attachment* const att = tdbb->getAttachment();

	{	// scope 1
		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

		COS_TRACE(("(%p)->lockWrite stage 1 readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));
		++pendingWriters;

		while (readers > 0 )
		{
			Attachment::Checkout attCout(att, FB_FUNCTION, true);
			noReaders.wait(counterMutex);
		}

		COS_TRACE(("(%p)->lockWrite stage 2 readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

		while (currentWriter || pendingLock)
		{
			Attachment::Checkout attCout(att, FB_FUNCTION, true);
			writerFinished.wait(counterMutex);
		}

		COS_TRACE(("(%p)->lockWrite stage 3 readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

		fb_assert(!readers && !currentWriter);

		if (cachedLock->lck_physical > LCK_none)
		{
			LCK_release(tdbb, cachedLock);	// To prevent self deadlock
			invalidate(tdbb);
		}

		++pendingLock;
	}

	COS_TRACE(("(%p)->lockWrite LCK_lock readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d), pendingLock(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical, pendingLock));

	if (!LCK_lock(tdbb, cachedLock, LCK_write, wait))
	{
		ISC_STATUS* const status = tdbb->tdbb_status_vector;
		if ((wait == LCK_NO_WAIT) || ((wait < 0) && (status[1] == isc_lock_timeout)))
			fb_utils::init_status(status);

		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

		--pendingLock;

	    if (--pendingWriters)
	    {
	        if (!currentWriter)
	            writerFinished.notifyAll();
	    }

	    return false;
	}

	{	// scope 2
		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

		--pendingLock;
		--pendingWriters;

		fb_assert(!currentWriter);

		currentWriter = true;

		COS_TRACE(("(%p)->lockWrite end readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

		return fetch(tdbb);
	}
}

void GlobalRWLock::unlockWrite(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	Attachment* const att = tdbb->getAttachment();
	Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

	COS_TRACE(("(%p)->unlockWrite readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

	currentWriter = false;

	if (!lockCaching)
		LCK_release(tdbb, cachedLock);
	else if (blocking)
	{
		LCK_downgrade(tdbb, cachedLock);
		blocking = false;
	}

	if (cachedLock->lck_physical < LCK_read)
		invalidate(tdbb);

	writerFinished.notifyAll();
	COS_TRACE(("(%p)->unlockWrite end readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));
}

bool GlobalRWLock::lockRead(thread_db* tdbb, SSHORT wait, const bool queueJump)
{
	SET_TDBB(tdbb);

	Attachment* const att = tdbb->getAttachment();

	bool needFetch;

	{	// scope 1
		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

		COS_TRACE(("(%p)->lockRead stage 1 readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

		while (true)
		{
			if (readers > 0 && queueJump)
			{
				COS_TRACE(("(%p)->lockRead queueJump", this));
				readers++;
				return true;
			}

			while (pendingWriters > 0 || currentWriter)
			{
				Attachment::Checkout attCout(att, FB_FUNCTION, true);
				writerFinished.wait(counterMutex);
			}

			COS_TRACE(("(%p)->lockRead stage 3 readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
				this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

			if (!pendingLock)
				break;

			MutexUnlockGuard cout(counterMutex, FB_FUNCTION);
			Attachment::Checkout attCout(att, FB_FUNCTION, true);
			THD_yield();
		}

		needFetch = cachedLock->lck_physical < LCK_read;
		if (!needFetch)
		{
			++readers;
			return true;
		}

		++pendingLock;

		fb_assert(cachedLock->lck_physical == LCK_none);
	}

	if (!LCK_lock(tdbb, cachedLock, LCK_read, wait))
	{
		ISC_STATUS* const status = tdbb->tdbb_status_vector;
		if ((wait == LCK_NO_WAIT) || ((wait < 0) && (status[1] == isc_lock_timeout)))
			fb_utils::init_status(status);

		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);
		--pendingLock;
		return false;
	}

	{	// scope 2
		Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);
		--pendingLock;
		++readers;

		COS_TRACE(("(%p)->lockRead end readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
			this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

		return fetch(tdbb);
	}
}

void GlobalRWLock::unlockRead(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	Attachment* const att = tdbb->getAttachment();
	Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

	COS_TRACE(("(%p)->unlockRead readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));
	readers--;

	if (!readers)
	{
		if (!lockCaching || pendingWriters || blocking)
		{
			LCK_release(tdbb, cachedLock);	// Release since concurrent request needs LCK_write
			invalidate(tdbb);
		}

		noReaders.notifyAll();
	}
	COS_TRACE(("(%p)->unlockRead end readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));
}

bool GlobalRWLock::tryReleaseLock(thread_db* tdbb)
{
	Attachment* const att = tdbb->getAttachment();
	Attachment::CheckoutLockGuard counterGuard(att, counterMutex, FB_FUNCTION, true);

	COS_TRACE(("(%p)->tryReleaseLock readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

	if (readers || currentWriter)
		return false;

	if (cachedLock->lck_physical > LCK_none)
	{
		LCK_release(tdbb, cachedLock);
		invalidate(tdbb);
	}

	return true;
}

void GlobalRWLock::blockingAstHandler(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	COS_TRACE(("(%p)->blockingAst enter", this));
	COS_TRACE(("(%p)->blockingAst readers(%d), blocking(%d), pendingWriters(%d), currentWriter(%d), lck_physical(%d)",
		this, readers, blocking, pendingWriters, currentWriter, cachedLock->lck_physical));

	if (!pendingLock && !currentWriter && !readers)
	{
		COS_TRACE(("(%p)->Downgrade lock", this));
		LCK_downgrade(tdbb, cachedLock);
		fb_assert(!blocking);
		if (cachedLock->lck_physical < LCK_read)
			invalidate(tdbb);
	}
	else
	{
		COS_TRACE(("(%p)->Set blocking", this));
		blocking = true;
	}
}
