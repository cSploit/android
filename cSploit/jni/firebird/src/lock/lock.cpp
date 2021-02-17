/*
 *	PROGRAM:	JRD Lock Manager
 *	MODULE:		lock.cpp
 *	DESCRIPTION:	Generic ISC Lock Manager
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DELTA" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "IMP" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.03.24 Nickolay Samofatov
 *	- cleanup #define's,
 *  - shutdown blocking thread cleanly on Windows CS
 *  - fix Windows CS lock-ups (make wakeup event manual-reset)
 *  - detect deadlocks instantly in most cases (if blocking owner
 *     dies during AST processing deadlock scan timeout still applies)
 * 2003.04.29 Nickolay Samofatov - fix broken lock table resizing code in CS builds
 * 2003.08.11 Nickolay Samofatov - finally and correctly fix Windows CS lock-ups.
 *            Roll back earlier workarounds on this subject.
 *
 */

#include "firebird.h"
#include "../lock/lock_proto.h"
#include "../common/ThreadStart.h"
#include "../jrd/jrd.h"
#include "../jrd/Attachment.h"
#include "gen/iberror.h"
#include "../yvalve/gds_proto.h"
#include "../common/gdsassert.h"
#include "../common/isc_proto.h"
#include "../common/os/isc_i_proto.h"
#include "../common/isc_s_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/config/config.h"
#include "../common/classes/array.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/init.h"
#include "../common/classes/timestamp.h"

#include <stdio.h>
#include <errno.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifdef WIN_NT
#include <process.h>
#define MUTEX		&m_shmemMutex
#else
#define MUTEX		m_lhb_mutex
#endif

#ifdef DEV_BUILD
//#define VALIDATE_LOCK_TABLE
#endif

#ifdef DEV_BUILD
#define ASSERT_ACQUIRED fb_assert(m_sharedMemory->getHeader()->lhb_active_owner)
#ifdef HAVE_OBJECT_MAP
#define LOCK_DEBUG_REMAP
#define DEBUG_REMAP_INTERVAL 5000
static ULONG debug_remap_count = 0;
#endif
#define CHECK(x)	do { if (!(x)) bug_assert ("consistency check", __LINE__); } while (false)
#else // DEV_BUILD
#define	ASSERT_ACQUIRED
#define CHECK(x)	do { } while (false)
#endif // DEV_BUILD

#ifdef DEBUG_LM
SSHORT LOCK_debug_level = 0;
#define LOCK_TRACE(x)	{ time_t t; time(&t); printf("%s", ctime(&t) ); printf x; fflush (stdout); gds__log x;}
#define DEBUG_MSG(l, x)	if ((l) <= LOCK_debug_level) { time_t t; time(&t); printf("%s", ctime(&t) ); printf x; fflush (stdout); gds__log x; }
// Debug delay is used to create nice big windows for signals or other
// events to occur in - eg: slow down the code to try and make
// timing race conditions show up
#define	DEBUG_DELAY	debug_delay (__LINE__)
#else
#define LOCK_TRACE(x)
#define DEBUG_MSG(l, x)
#define DEBUG_DELAY
#endif

using namespace Firebird;

// hvlad: enable to log deadlocked owners and its PIDs in firebird.log
//#define DEBUG_TRACE_DEADLOCKS

// CVC: Unlike other definitions, SRQ_PTR is not a pointer to something in lowercase.
// It's LONG.

const SRQ_PTR DUMMY_OWNER = -1;

const SLONG HASH_MIN_SLOTS	= 101;
const SLONG HASH_MAX_SLOTS	= 65521;
const USHORT HISTORY_BLOCKS	= 256;

// SRQ_ABS_PTR uses this macro.
#define SRQ_BASE                    ((UCHAR*) m_sharedMemory->getHeader())

static const bool compatibility[LCK_max][LCK_max] =
{
/*							Shared	Prot	Shared	Prot
			none	null	Read	Read	Write	Write	Exclusive */

/* none */	{true,	true,	true,	true,	true,	true,	true},
/* null */	{true,	true,	true,	true,	true,	true,	true},
/* SR */	{true,	true,	true,	true,	true,	true,	false},
/* PR */	{true,	true,	true,	true,	false,	false,	false},
/* SW */	{true,	true,	true,	false,	true,	false,	false},
/* PW */	{true,	true,	true,	false,	false,	false,	false},
/* EX */	{true,	true,	false,	false,	false,	false,	false}
};


namespace Jrd {

Firebird::GlobalPtr<LockManager::DbLockMgrMap> LockManager::g_lmMap;
Firebird::GlobalPtr<Firebird::Mutex> LockManager::g_mapMutex;


LockManager* LockManager::create(const Firebird::string& id, RefPtr<Config> conf)
{
	Firebird::MutexLockGuard guard(g_mapMutex, FB_FUNCTION);

	LockManager* lockMgr = NULL;
	if (!g_lmMap->get(id, lockMgr))
	{
		lockMgr = new LockManager(id, conf);

		if (g_lmMap->put(id, lockMgr))
		{
			fb_assert(false);
		}
	}

	fb_assert(lockMgr);

	lockMgr->addRef();
	return lockMgr;
}


void LockManager::destroy(LockManager* lockMgr)
{
	if (lockMgr)
	{
		const Firebird::string id = lockMgr->m_dbId;

		Firebird::MutexLockGuard guard(g_mapMutex, FB_FUNCTION);

		if (!lockMgr->release())
		{
			if (!g_lmMap->remove(id))
			{
				fb_assert(false);
			}
		}
	}
}


LockManager::LockManager(const Firebird::string& id, RefPtr<Config> conf)
	: PID(getpid()),
	  m_bugcheck(false),
	  m_sharedFileCreated(false),
	  m_process(NULL),
	  m_processOffset(0),
	  m_sharedMemory(NULL),
	  m_blockage(false),
	  m_dbId(getPool(), id),
	  m_config(conf),
	  m_acquireSpins(m_config->getLockAcquireSpins()),
	  m_memorySize(m_config->getLockMemSize()),
	  m_useBlockingThread(m_config->getSharedDatabase())
#ifdef USE_SHMEM_EXT
	  , m_extents(getPool())
#endif
{
	Arg::StatusVector localStatus;
	if (!attach_shared_file(localStatus))
	{
		iscLogStatus("LockManager::LockManager()", localStatus.value());
		localStatus.raise();
	}
}


LockManager::~LockManager()
{
	const SRQ_PTR process_offset = m_processOffset;

	{ // guardian's scope
		LockTableGuard guard(this, FB_FUNCTION);
		m_processOffset = 0;
	}

	Arg::StatusVector localStatus;

	if (m_process)
	{
		if (m_useBlockingThread)
		{
			// Wait for AST thread to start (or 5 secs)
			m_startupSemaphore.tryEnter(5);

			// Wakeup the AST thread - it might be blocking
			(void)  // Ignore errors in dtor()
			m_sharedMemory->eventPost(&m_process->prc_blocking);

			// Wait for the AST thread to finish cleanup or for 5 seconds
			m_cleanupSemaphore.tryEnter(5);
		}

#ifdef HAVE_OBJECT_MAP
		m_sharedMemory->unmapObject(localStatus, &m_process);
#else
		m_process = NULL;
#endif
	}

	{ // guardian's scope
		LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

		if (process_offset)
		{
			prc* const process = (prc*) SRQ_ABS_PTR(process_offset);
			purge_process(process);
		}

		if (m_sharedMemory->getHeader() && SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_processes))
		{
			Firebird::PathName name;
			get_shared_file_name(name);
			m_sharedMemory->removeMapFile();
#ifdef USE_SHMEM_EXT
			for (ULONG i = 1; i < m_extents.getCount(); ++i)
			{
				m_extents[i].removeMapFile();
			}
#endif //USE_SHMEM_EXT
		}
	}

	detach_shared_file(localStatus);
#ifdef USE_SHMEM_EXT
	for (ULONG i = 1; i < m_extents.getCount(); ++i)
	{
		m_extents[i].unmapFile(localStatus);
	}
#endif //USE_SHMEM_EXT
}


#ifdef USE_SHMEM_EXT
SRQ_PTR LockManager::REL_PTR(const void* par_item)
{
	const UCHAR* const item = static_cast<const UCHAR*>(par_item);
	for (ULONG i = 0; i < m_extents.getCount(); ++i)
	{
		Extent& l = m_extents[i];
		UCHAR* adr = reinterpret_cast<UCHAR*>(l.sh_mem_header);
		if (item >= adr && item < adr + getExtentSize())
		{
			return getStartOffset(i) + (item - adr);
		}
    }

	errno = 0;
    bug(NULL, "Extent not found in REL_PTR()");
    return 0;	// compiler silencer
}


void* LockManager::ABS_PTR(SRQ_PTR item)
{
	const ULONG extent = item / getExtentSize();
	if (extent >= m_extents.getCount())
	{
		errno = 0;
		bug(NULL, "Extent not found in ABS_PTR()");
	}
	return reinterpret_cast<UCHAR*>(m_extents[extent].sh_mem_header) + (item % getExtentSize());
}
#endif //USE_SHMEM_EXT


bool LockManager::attach_shared_file(Arg::StatusVector& statusVector)
{
	Firebird::PathName name;
	get_shared_file_name(name);

	try
	{
		SharedMemory<lhb>* tmp = FB_NEW(getPool()) SharedMemory<lhb>(name.c_str(), m_memorySize, this);
		// initialize will reset m_sharedMemory
		fb_assert(m_sharedMemory == tmp);
	}
	catch (const Exception& ex)
	{
		statusVector.assign(ex);
		return false;
	}

	fb_assert(m_sharedMemory->getHeader()->mhb_type == SharedMemoryBase::SRAM_LOCK_MANAGER);
	fb_assert(m_sharedMemory->getHeader()->mhb_version == LHB_VERSION);

#ifdef USE_SHMEM_EXT
	m_extents[0] = *this;
#endif

	return true;
}


void LockManager::detach_shared_file(Arg::StatusVector& statusVector)
{
	if (m_sharedMemory.hasData() && m_sharedMemory->getHeader())
	{
		try
		{
			delete m_sharedMemory.release();
		}
		catch (const Exception& ex)
		{
			statusVector.assign(ex);
		}
	}
}


void LockManager::get_shared_file_name(Firebird::PathName& name, ULONG extent) const
{
	name.printf(LOCK_FILE, m_dbId.c_str());
	if (extent)
	{
		Firebird::PathName ename;
		ename.printf("%s.ext%d", name.c_str(), extent);
		name = ename;
	}
}


bool LockManager::initializeOwner(Arg::StatusVector& statusVector,
								  LOCK_OWNER_T owner_id,
								  UCHAR owner_type,
								  SRQ_PTR* owner_handle)
{
/**************************************
 *
 *	i n i t i a l i z e O w n e r
 *
 **************************************
 *
 * Functional description
 *	Initialize lock manager for the given owner, if not already done.
 *
 *	Initialize an owner block in the lock manager, if not already
 *	initialized.
 *
 *	Return the offset of the owner block through owner_handle.
 *
 *	Return success or failure.
 *
 **************************************/
	LOCK_TRACE(("LM::init (ownerid=%ld)\n", owner_id));

	SRQ_PTR owner_offset = *owner_handle;

	if (owner_offset)
	{
		LockTableGuard guard(this, FB_FUNCTION, owner_offset);

		// If everything is already initialized, just bump the use count

		own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
		owner->own_count++;
		return true;
	}

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	owner_offset = create_owner(statusVector, owner_id, owner_type);

	if (owner_offset)
		*owner_handle = owner_offset;

	LOCK_TRACE(("LM::init done (%ld)\n", owner_offset));
	return (owner_offset != 0);
}


void LockManager::shutdownOwner(Attachment* attachment, SRQ_PTR* owner_handle)
{
/**************************************
 *
 *	s h u t d o w n O w n e r
 *
 **************************************
 *
 * Functional description
 *	Release the owner block and any outstanding locks.
 *	The exit handler will unmap the shared memory.
 *
 **************************************/
	LOCK_TRACE(("LM::fini (%ld)\n", *owner_handle));

	const SRQ_PTR owner_offset = *owner_handle;
	if (!owner_offset)
		return;

	LockTableGuard guard(this, FB_FUNCTION, owner_offset);

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return;

	if (--owner->own_count > 0)
		return;

	while (owner->own_ast_count)
	{
		{ // checkout scope
			LockTableCheckout checkout(this, FB_FUNCTION);
			Jrd::Attachment::Checkout cout(attachment, FB_FUNCTION, true);
			THREAD_SLEEP(10);
		}

		owner = (own*) SRQ_ABS_PTR(owner_offset);
	}

	// This assert expects that all the granted locks have been explicitly
	// released before destroying the lock owner. This is not strictly required,
	// but it enforces the proper object lifetime discipline through the codebase.
	fb_assert(SRQ_EMPTY(owner->own_requests));

	purge_owner(owner_offset, owner);

	*owner_handle = 0;
}


SRQ_PTR LockManager::enqueue(Attachment* attachment,
							 Arg::StatusVector& statusVector,
							 SRQ_PTR prior_request,
							 const USHORT series,
							 const UCHAR* value,
							 const USHORT length,
							 UCHAR type,
							 lock_ast_t ast_routine,
							 void* ast_argument,
							 SLONG data,
							 SSHORT lck_wait,
							 SRQ_PTR owner_offset)
{
/**************************************
 *
 *	e n q u e u e
 *
 **************************************
 *
 * Functional description
 *	Enque on a lock.  If the lock can't be granted immediately,
 *	return an event count on which to wait.  If the lock can't
 *	be granted because of deadlock, return NULL.
 *
 **************************************/
	LOCK_TRACE(("LM::enqueue (%ld)\n", owner_offset));

	if (!owner_offset)
		return 0;

	LockTableGuard guard(this, FB_FUNCTION, owner_offset);

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return 0;

	ASSERT_ACQUIRED;
	++(m_sharedMemory->getHeader()->lhb_enqs);

#ifdef VALIDATE_LOCK_TABLE
	if ((m_sharedMemory->getHeader()->lhb_enqs % 50) == 0)
		validate_lhb(m_sharedMemory->getHeader());
#endif

	if (prior_request)
		internal_dequeue(prior_request);

	// Allocate or reuse a lock request block

	lrq* request;

	ASSERT_ACQUIRED;
	if (SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_free_requests))
	{
		if (!(request = (lrq*) alloc(sizeof(lrq), &statusVector)))
		{
			return 0;
		}
		owner = (own*) SRQ_ABS_PTR(owner_offset);
	}
	else
	{
		ASSERT_ACQUIRED;
		request = (lrq*) ((UCHAR*) SRQ_NEXT(m_sharedMemory->getHeader()->lhb_free_requests) -
						 OFFSET(lrq*, lrq_lbl_requests));
		remove_que(&request->lrq_lbl_requests);
	}

	post_history(his_enq, owner_offset, (SRQ_PTR)0, SRQ_REL_PTR(request), true);

	request->lrq_type = type_lrq;
	request->lrq_flags = 0;
	request->lrq_requested = type;
	request->lrq_state = LCK_none;
	request->lrq_data = 0;
	request->lrq_owner = owner_offset;
	request->lrq_ast_routine = ast_routine;
	request->lrq_ast_argument = ast_argument;
	insert_tail(&owner->own_requests, &request->lrq_own_requests);
	SRQ_INIT(request->lrq_own_blocks);
	SRQ_INIT(request->lrq_own_pending);

	const SRQ_PTR request_offset = SRQ_REL_PTR(request);

	// See if the lock already exists

	USHORT hash_slot;
	lbl* lock = find_lock(series, value, length, &hash_slot);
	if (lock)
	{
		if (series < LCK_MAX_SERIES)
			++(m_sharedMemory->getHeader()->lhb_operations[series]);
		else
			++(m_sharedMemory->getHeader()->lhb_operations[0]);

		insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
		request->lrq_data = data;

		if (grant_or_que(attachment, request, lock, lck_wait))
			return request_offset;

		statusVector << Arg::Gds(lck_wait > 0 ? isc_deadlock :
			(lck_wait < 0 ? isc_lock_timeout : isc_lock_conflict));

		return 0;
	}

	// Lock doesn't exist. Allocate lock block and set it up.

	if (!(lock = alloc_lock(length, statusVector)))
	{
		// lock table is exhausted: release request gracefully
		remove_que(&request->lrq_own_requests);
		request->lrq_type = type_null;
		insert_tail(&m_sharedMemory->getHeader()->lhb_free_requests, &request->lrq_lbl_requests);
		return 0;
	}

	lock->lbl_state = type;
	fb_assert(series <= MAX_UCHAR);
	lock->lbl_series = (UCHAR)series;

	// Maintain lock series data queue

	SRQ_INIT(lock->lbl_lhb_data);
	if ( (lock->lbl_data = data) )
		insert_data_que(lock);

	if (series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	lock->lbl_flags = 0;
	lock->lbl_pending_lrq_count = 0;

	memset(lock->lbl_counts, 0, sizeof(lock->lbl_counts));

	lock->lbl_length = length;
	memcpy(lock->lbl_key, value, length);

	request = (lrq*) SRQ_ABS_PTR(request_offset);

	SRQ_INIT(lock->lbl_requests);
	ASSERT_ACQUIRED;
	insert_tail(&m_sharedMemory->getHeader()->lhb_hash[hash_slot], &lock->lbl_lhb_hash);
	insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
	request->lrq_lock = SRQ_REL_PTR(lock);
	grant(request, lock);

	return request_offset;
}


bool LockManager::convert(Attachment* attachment,
						  Arg::StatusVector& statusVector,
						  SRQ_PTR request_offset,
						  UCHAR type,
						  SSHORT lck_wait,
						  lock_ast_t ast_routine,
						  void* ast_argument)
{
/**************************************
 *
 *	c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Perform a lock conversion, if possible.
 *
 **************************************/
	LOCK_TRACE(("LM::convert (%d, %d)\n", type, lck_wait));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	lrq* const request = get_request(request_offset);
	const SRQ_PTR owner_offset = request->lrq_owner;
	guard.setOwner(owner_offset);

	own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return false;

	++(m_sharedMemory->getHeader()->lhb_converts);

	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	if (lock->lbl_series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[lock->lbl_series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	const bool result =
		internal_convert(attachment, statusVector, request_offset, type, lck_wait,
						 ast_routine, ast_argument);

	return result;
}


UCHAR LockManager::downgrade(Attachment* attachment,
							 Arg::StatusVector& statusVector,
							 const SRQ_PTR request_offset)
{
/**************************************
 *
 *	d o w n g r a d e
 *
 **************************************
 *
 * Functional description
 *	Downgrade an existing lock returning
 *	its new state.
 *
 **************************************/
	LOCK_TRACE(("LM::downgrade (%ld)\n", request_offset));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	lrq* const request = get_request(request_offset);
	const SRQ_PTR owner_offset = request->lrq_owner;
	guard.setOwner(owner_offset);

	own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return LCK_none;

	++(m_sharedMemory->getHeader()->lhb_downgrades);

	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	UCHAR pending_state = LCK_none;

	// Loop thru requests looking for pending conversions
	// and find the highest requested state

	srq* lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		const lrq* const pending = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		if ((pending->lrq_flags & LRQ_pending) && pending != request)
		{
			pending_state = MAX(pending->lrq_requested, pending_state);
			if (pending_state == LCK_EX)
				break;
		}
	}

	UCHAR state = request->lrq_state;
	while (state > LCK_none && !compatibility[pending_state][state])
		--state;

	if (state == LCK_none || state == LCK_null)
	{
		internal_dequeue(request_offset);
		state = LCK_none;
	}
	else
	{
		internal_convert(attachment, statusVector, request_offset, state, LCK_NO_WAIT,
						 request->lrq_ast_routine, request->lrq_ast_argument);
	}

	return state;
}


bool LockManager::dequeue(const SRQ_PTR request_offset)
{
/**************************************
 *
 *	d e q u e u e
 *
 **************************************
 *
 * Functional description
 *	Release an outstanding lock.
 *
 **************************************/
	LOCK_TRACE(("LM::dequeue (%ld)\n", request_offset));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	lrq* const request = get_request(request_offset);
	const SRQ_PTR owner_offset = request->lrq_owner;
	guard.setOwner(owner_offset);

	own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return false;

	++(m_sharedMemory->getHeader()->lhb_deqs);

	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	if (lock->lbl_series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[lock->lbl_series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	internal_dequeue(request_offset);
	return true;
}


void LockManager::repost(Attachment* attachment, lock_ast_t ast, void* arg, SRQ_PTR owner_offset)
{
/**************************************
 *
 *	r e p o s t
 *
 **************************************
 *
 * Functional description
 *	Re-post an AST that was previously blocked.
 *	It is assumed that the routines that look
 *	at the re-post list only test the ast element.
 *
 **************************************/
	LOCK_TRACE(("LM::repost (%ld)\n", owner_offset));

	if (!owner_offset)
		return;

	LockTableGuard guard(this, FB_FUNCTION, owner_offset);

	// Allocate or reuse a lock request block

	lrq* request;

	ASSERT_ACQUIRED;
	if (SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_free_requests))
	{
		if (!(request = (lrq*) alloc(sizeof(lrq), NULL)))
		{
			return;
		}
	}
	else
	{
		ASSERT_ACQUIRED;
		request = (lrq*) ((UCHAR*) SRQ_NEXT(m_sharedMemory->getHeader()->lhb_free_requests) -
						 OFFSET(lrq*, lrq_lbl_requests));
		remove_que(&request->lrq_lbl_requests);
	}

	request->lrq_type = type_lrq;
	request->lrq_flags = LRQ_repost;
	request->lrq_ast_routine = ast;
	request->lrq_ast_argument = arg;
	request->lrq_requested = LCK_none;
	request->lrq_state = LCK_none;
	request->lrq_owner = owner_offset;
	request->lrq_lock = 0;

	own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
	insert_tail(&owner->own_blocks, &request->lrq_own_blocks);
	SRQ_INIT(request->lrq_own_pending);

	DEBUG_DELAY;

	if (!(owner->own_flags & OWN_signaled))
		signal_owner(attachment, owner);
}


bool LockManager::cancelWait(SRQ_PTR owner_offset)
{
/**************************************
 *
 *	c a n c e l W a i t
 *
 **************************************
 *
 * Functional description
 *	Wakeup waiting owner to make it check if wait should be cancelled.
 *	As this routine could be called asyncronous, take extra care and
 *	don't trust the input params blindly.
 *
 **************************************/
	LOCK_TRACE(("LM::cancelWait (%ld)\n", owner_offset));

	if (!owner_offset)
		return false;

	LockTableGuard guard(this, FB_FUNCTION, owner_offset);

	own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return false;

	post_wakeup(owner);
	return true;
}


SLONG LockManager::queryData(const USHORT series, const USHORT aggregate)
{
/**************************************
 *
 *	q u e r y D a t a
 *
 **************************************
 *
 * Functional description
 *	Query lock series data with respect to a rooted
 *	lock hierarchy calculating aggregates as we go.
 *
 **************************************/
	if (series >= LCK_MAX_SERIES)
	{
		CHECK(false);
		return 0;
	}

	LOCK_TRACE(("LM::queryData (%ld)\n", owner_offset));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	++(m_sharedMemory->getHeader()->lhb_query_data);

	const srq& data_header = m_sharedMemory->getHeader()->lhb_data[series];
	SLONG data = 0, count = 0;

	// Simply walk the lock series data queue forward for the minimum
	// and backward for the maximum -- it's maintained in sorted order.

	switch (aggregate)
	{
	case LCK_CNT:
	case LCK_AVG:
	case LCK_SUM:
		for (const srq* lock_srq = (SRQ) SRQ_ABS_PTR(data_header.srq_forward);
			 lock_srq != &data_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
		{
			const lbl* const lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock->lbl_series == series);

			switch (aggregate)
			{
			case LCK_CNT:
				++count;
				break;

			case LCK_AVG:
				++count;

			case LCK_SUM:
				data += lock->lbl_data;
				break;
			}
		}

		if (aggregate == LCK_CNT)
			data = count;
		else if (aggregate == LCK_AVG)
			data = count ? data / count : 0;
		break;

	case LCK_ANY:
		if (!SRQ_EMPTY(data_header))
			data = 1;
		break;

	case LCK_MIN:
		if (!SRQ_EMPTY(data_header))
		{
			const srq* lock_srq = (SRQ) SRQ_ABS_PTR(data_header.srq_forward);
			const lbl* const lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock->lbl_series == series);

			data = lock->lbl_data;
		}
		break;

	case LCK_MAX:
		if (!SRQ_EMPTY(data_header))
		{
			const srq* lock_srq = (SRQ) SRQ_ABS_PTR(data_header.srq_backward);
			const lbl* const lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock->lbl_series == series);

			data = lock->lbl_data;
		}
		break;

	default:
		CHECK(false);
	}

	return data;
}


SLONG LockManager::readData(SRQ_PTR request_offset)
{
/**************************************
 *
 *	r e a d D a t a
 *
 **************************************
 *
 * Functional description
 *	Read data associated with a lock.
 *
 **************************************/
	LOCK_TRACE(("LM::readData (%ld)\n", request_offset));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	const lrq* const request = get_request(request_offset);
	guard.setOwner(request->lrq_owner);

	++(m_sharedMemory->getHeader()->lhb_read_data);

	const lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	const SLONG data = lock->lbl_data;
	if (lock->lbl_series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[lock->lbl_series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	return data;
}


SLONG LockManager::readData2(USHORT series,
							 const UCHAR* value,
							 USHORT length,
							 SRQ_PTR owner_offset)
{
/**************************************
 *
 *	r e a d D a t a 2
 *
 **************************************
 *
 * Functional description
 *	Read data associated with transient locks.
 *
 **************************************/
	LOCK_TRACE(("LM::readData2 (%ld)\n", owner_offset));

	if (!owner_offset)
		return 0;

	LockTableGuard guard(this, FB_FUNCTION, owner_offset);

	++(m_sharedMemory->getHeader()->lhb_read_data);

	if (series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	USHORT junk;
	const lbl* const lock = find_lock(series, value, length, &junk);

	return lock ? lock->lbl_data : 0;
}


SLONG LockManager::writeData(SRQ_PTR request_offset, SLONG data)
{
/**************************************
 *
 *	w r i t e D a t a
 *
 **************************************
 *
 * Functional description
 *	Write a longword into the lock block.
 *
 **************************************/
	LOCK_TRACE(("LM::writeData (%ld)\n", request_offset));

	LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

	const lrq* const request = get_request(request_offset);
	guard.setOwner(request->lrq_owner);

	++(m_sharedMemory->getHeader()->lhb_write_data);

	lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	remove_que(&lock->lbl_lhb_data);
	if ( (lock->lbl_data = data) )
		insert_data_que(lock);

	if (lock->lbl_series < LCK_MAX_SERIES)
		++(m_sharedMemory->getHeader()->lhb_operations[lock->lbl_series]);
	else
		++(m_sharedMemory->getHeader()->lhb_operations[0]);

	return data;
}


void LockManager::acquire_shmem(SRQ_PTR owner_offset)
{
/**************************************
 *
 *	a c q u i r e
 *
 **************************************
 *
 * Functional description
 *	Acquire the lock file.  If it's busy, wait for it.
 *
 **************************************/

	// Perform a spin wait on the lock table mutex. This should only
	// be used on SMP machines; it doesn't make much sense otherwise.

	const ULONG spins_to_try = m_acquireSpins ? m_acquireSpins : 1;
	bool locked = false;
	ULONG spins = 0;
	while (spins++ < spins_to_try)
	{
		if (m_sharedMemory->mutexLockCond())
		{
			locked = true;
			break;
		}

		m_blockage = true;
	}

	// If the spin wait didn't succeed then wait forever

	if (!locked)
		m_sharedMemory->mutexLock();

	// Check for shared memory state consistency

	while (SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_processes))
	{
		if (!m_sharedFileCreated)
		{
			Arg::StatusVector localStatus;

			// Someone is going to delete shared file? Reattach.
			m_sharedMemory->mutexUnlock();
			detach_shared_file(localStatus);

			THD_yield();

			if (!attach_shared_file(localStatus))
				bug(&localStatus, "ISC_map_file failed (reattach shared file)");

			m_sharedMemory->mutexLock();
		}
		else
		{
			// complete initialization
			m_sharedFileCreated = false;

			// no sense thinking about statistics now
			m_blockage = false;

			break;
		}
	}

	fb_assert(!m_sharedFileCreated);

	++(m_sharedMemory->getHeader()->lhb_acquires);
	if (m_blockage)
	{
		++(m_sharedMemory->getHeader()->lhb_acquire_blocks);
		m_blockage = false;
	}

	if (spins > 1)
	{
		++(m_sharedMemory->getHeader()->lhb_acquire_retries);
		if (spins < spins_to_try) {
			++(m_sharedMemory->getHeader()->lhb_retry_success);
		}
	}

	const SRQ_PTR prior_active = m_sharedMemory->getHeader()->lhb_active_owner;
	m_sharedMemory->getHeader()->lhb_active_owner = owner_offset;

	if (owner_offset > 0)
	{
		own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
		owner->own_thread_id = getThreadId();
	}

#ifdef USE_SHMEM_EXT
	while (m_sharedMemory->getHeader()->lhb_length > getTotalMapped())
	{
		if (!createExtent())
		{
			bug(NULL, "map of lock file extent failed");
		}
	}
#else //USE_SHMEM_EXT

	if (m_sharedMemory->getHeader()->lhb_length > m_sharedMemory->sh_mem_length_mapped
#ifdef LOCK_DEBUG_REMAP
		// If we're debugging remaps, force a remap every-so-often.
		|| ((debug_remap_count++ % DEBUG_REMAP_INTERVAL) == 0 && m_processOffset)
#endif
		)
	{
#ifdef HAVE_OBJECT_MAP
		const ULONG new_length = m_sharedMemory->getHeader()->lhb_length;

		Firebird::WriteLockGuard guard(m_remapSync, FB_FUNCTION);
		// Post remapping notifications
		remap_local_owners();
		// Remap the shared memory region
		Arg::StatusVector statusVector;
		if (!m_sharedMemory->remapFile(statusVector, new_length, false))
#endif
		{
			bug(NULL, "remap failed");
			return;
		}
	}
#endif //USE_SHMEM_EXT

	// If we were able to acquire the MUTEX, but there is an prior owner marked
	// in the the lock table, it means that someone died while owning
	// the lock mutex.  In that event, lets see if there is any unfinished work
	// left around that we need to finish up.

	if (prior_active > 0)
	{
		post_history(his_active, owner_offset, prior_active, (SRQ_PTR) 0, false);
		shb* const recover = (shb*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_secondary);
		if (recover->shb_remove_node)
		{
			// There was a remove_que operation in progress when the prior_owner died
			DEBUG_MSG(0, ("Got to the funky shb_remove_node code\n"));
			remove_que((SRQ) SRQ_ABS_PTR(recover->shb_remove_node));
		}
		else if (recover->shb_insert_que && recover->shb_insert_prior)
		{
			// There was a insert_que operation in progress when the prior_owner died
			DEBUG_MSG(0, ("Got to the funky shb_insert_que code\n"));

			SRQ lock_srq = (SRQ) SRQ_ABS_PTR(recover->shb_insert_que);
			lock_srq->srq_backward = recover->shb_insert_prior;
			lock_srq = (SRQ) SRQ_ABS_PTR(recover->shb_insert_prior);
			lock_srq->srq_forward = recover->shb_insert_que;
			recover->shb_insert_que = 0;
			recover->shb_insert_prior = 0;
		}
	}
}


#ifdef USE_SHMEM_EXT
bool LockManager::Extent::initialize(bool)
{
	return false;
}

void LockManager::Extent::mutexBug(int, const char*)
{ }

bool LockManager::createExtent()
{
	Firebird::PathName name;
	get_shared_file_name(name, (ULONG) m_extents.getCount());
	Arg::StatusVector local_status;

	Extent& extent = m_extents.add();

	if (!extent.mapFile(local_status, name.c_str(), m_memorySize))
	{
		m_extents.pop();
		logError("LockManager::createExtent() mapFile", local_status);
		return false;
	}

	return true;
}
#endif


UCHAR* LockManager::alloc(USHORT size, Arg::StatusVector* statusVector)
{
/**************************************
 *
 *	a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate a block of given size.
 *
 **************************************/
	size = FB_ALIGN(size, FB_ALIGNMENT);
	ASSERT_ACQUIRED;
	ULONG block = m_sharedMemory->getHeader()->lhb_used;

	// Make sure we haven't overflowed the lock table.  If so, bump the size of the table.

	if (m_sharedMemory->getHeader()->lhb_used + size > m_sharedMemory->getHeader()->lhb_length)
	{
#ifdef USE_SHMEM_EXT
		// round up so next object starts at beginning of next extent
		block = m_sharedMemory->getHeader()->lhb_used = m_sharedMemory->getHeader()->lhb_length;
		if (createExtent())
		{
			m_sharedMemory->getHeader()->lhb_length += m_memorySize;
		}
		else
#elif (defined HAVE_OBJECT_MAP)
		Firebird::WriteLockGuard guard(m_remapSync, FB_FUNCTION);
		// Post remapping notifications
		remap_local_owners();
		// Remap the shared memory region
		const ULONG new_length = m_sharedMemory->sh_mem_length_mapped + m_memorySize;
		if (m_sharedMemory->remapFile(*statusVector, new_length, true))
		{
			ASSERT_ACQUIRED;
			m_sharedMemory->getHeader()->lhb_length = m_sharedMemory->sh_mem_length_mapped;
		}
		else
#endif
		{
			// Do not do abort in case if there is not enough room -- just
			// return an error

			if (statusVector)
				*statusVector << Arg::Gds(isc_random) << "lock manager out of room";

			return NULL;
		}
	}

	m_sharedMemory->getHeader()->lhb_used += size;

#ifdef DEV_BUILD
	// This version of alloc() doesn't initialize memory.  To shake out
	// any bugs, in DEV_BUILD we initialize it to a "funny" pattern.
	memset((void*)SRQ_ABS_PTR(block), 0xFD, size);
#endif

	return (UCHAR*) SRQ_ABS_PTR(block);
}


lbl* LockManager::alloc_lock(USHORT length, Arg::StatusVector& statusVector)
{
/**************************************
 *
 *	a l l o c _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Allocate a lock for a key of a given length.  Look first to see
 *	if a spare of the right size is sitting around.  If not, allocate
 *	one.
 *
 **************************************/
	length = FB_ALIGN(length, 8);

	ASSERT_ACQUIRED;
	srq* lock_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->lhb_free_locks, lock_srq)
	{
		lbl* lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_hash));
		// Here we use the "first fit" approach which costs us some memory,
		// but works fast. The "best fit" one is proven to be unacceptably slow.
		// Maybe there could be some compromise, e.g. limiting the number of "best fit"
		// iterations before defaulting to a "first fit" match. Another idea could be
		// to introduce yet another hash table for the free locks queue.
		if (lock->lbl_size >= length)
		{
			remove_que(&lock->lbl_lhb_hash);
			lock->lbl_type = type_lbl;
			return lock;
		}
	}

	lbl* lock = (lbl*) alloc(sizeof(lbl) + length, &statusVector);
	if (lock)
	{
		lock->lbl_size = length;
		lock->lbl_type = type_lbl;
	}

// NOTE: if the above alloc() fails do not release mutex here but rather
//       release it in LOCK_enq() (as of now it is the only function that
//       calls alloc_lock()). We need to hold mutex to be able
//       to release a lock request block.

	return lock;
}


void LockManager::blocking_action(Attachment* attachment, SRQ_PTR blocking_owner_offset)
{
/**************************************
 *
 *	b l o c k i n g _ a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Fault handler for a blocking signal.  A blocking signal
 *	is an indication (albeit a strong one) that a blocking
 *	AST is pending for the owner.  Check in with the data
 *	structure for details.
 *	The re-post code in this routine assumes that no more
 *	than one thread of execution can be running in this
 *	routine at any time.
 *
 *      IMPORTANT: Before calling this routine, acquire_shmem() should
 *	           have already been done.
 *
 *      Note that both a blocking owner offset and blocked owner
 *      offset are passed to this function.   This is for those
 *      cases where the owners are not the same.   If they are
 *      the same, then the blocked owner offset will be NULL.
 *
 **************************************/
	ASSERT_ACQUIRED;
	own* owner = (own*) SRQ_ABS_PTR(blocking_owner_offset);

	while (owner->own_count && !SRQ_EMPTY(owner->own_blocks))
	{
		srq* const lock_srq = SRQ_NEXT(owner->own_blocks);

		lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_blocks));
		lock_ast_t routine = request->lrq_ast_routine;
		void* arg = request->lrq_ast_argument;
		remove_que(&request->lrq_own_blocks);
		if (request->lrq_flags & LRQ_blocking)
		{
			request->lrq_flags &= ~LRQ_blocking;
			request->lrq_flags |= LRQ_blocking_seen;
			++(m_sharedMemory->getHeader()->lhb_blocks);
			post_history(his_post_ast, blocking_owner_offset,
						 request->lrq_lock, SRQ_REL_PTR(request), true);
		}
		else if (request->lrq_flags & LRQ_repost)
		{
			request->lrq_type = type_null;
			insert_tail(&m_sharedMemory->getHeader()->lhb_free_requests, &request->lrq_lbl_requests);
		}

		if (routine)
		{
			owner->own_ast_count++;

			{ // checkout scope
				LockTableCheckout checkout(this, FB_FUNCTION);
				Jrd::Attachment::Checkout cout(attachment, FB_FUNCTION, true);
				(*routine)(arg);
			}

			owner = (own*) SRQ_ABS_PTR(blocking_owner_offset);
			owner->own_ast_count--;
		}
	}

	owner->own_flags &= ~OWN_signaled;
}


void LockManager::blocking_action_thread()
{
/**************************************
 *
 *	b l o c k i n g _ a c t i o n _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Thread to handle blocking signals.
 *
 **************************************/

/*
 * Main thread may be gone releasing our LockManager instance
 * when AST can't lock appropriate attachment mutex and therefore does not return.
 *
 * This causes multiple errors when entering/releasing mutexes/semaphores.
 * Catch this errors and log them.
 *
 * AP 2008
 */

	bool atStartup = true;

	try
	{
		while (true)
		{
			SLONG value;

			{ // guardian's scope
				LockTableGuard guard(this, FB_FUNCTION, DUMMY_OWNER);

				// See if the main thread has requested us to go away
				if (!m_processOffset || m_process->prc_process_id != PID)
				{
					if (atStartup)
					{
						m_startupSemaphore.release();
					}
					break;
				}

				value = m_sharedMemory->eventClear(&m_process->prc_blocking);

				DEBUG_DELAY;

				while (m_processOffset)
				{
					const prc* const process = (prc*) SRQ_ABS_PTR(m_processOffset);

					bool completed = true;

					srq* lock_srq;
					SRQ_LOOP(process->prc_owners, lock_srq)
					{
						const own* const owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_prc_owners));

						if (owner->own_flags & OWN_signaled)
						{
							const SRQ_PTR owner_offset = SRQ_REL_PTR(owner);
							guard.setOwner(owner_offset);
							blocking_action(NULL, owner_offset);
							completed = false;
							break;
						}
					}

					if (completed)
						break;
				}

				if (atStartup)
				{
					atStartup = false;
					m_startupSemaphore.release();
				}
			}

			m_sharedMemory->eventWait(&m_process->prc_blocking, value, 0);
		}
	}
	catch (const Firebird::Exception& x)
	{
		iscLogException("Error in blocking action thread\n", x);
	}

	try
	{
		// Wakeup the main thread waiting for our exit
		m_cleanupSemaphore.release();
	}
	catch (const Firebird::Exception& x)
	{
		iscLogException("Error closing blocking action thread\n", x);
	}
}


void LockManager::bug_assert(const TEXT* string, ULONG line)
{
/**************************************
 *
 *      b u g _ a s s e r t
 *
 **************************************
 *
 * Functional description
 *	Disastrous lock manager bug.  Issue message and abort process.
 *
 **************************************/
	TEXT buffer[MAXPATHLEN + 100];
	lhb LOCK_header_copy;

	sprintf((char*) buffer, "%s %"ULONGFORMAT": lock assertion failure: %.60s\n",
			__FILE__, line, string);

	// Copy the shared memory so we can examine its state when we crashed
	LOCK_header_copy = *m_sharedMemory->getHeader();

	bug(NULL, buffer);	// Never returns
}


void LockManager::bug(Arg::StatusVector* statusVector, const TEXT* string)
{
/**************************************
 *
 *	b u g
 *
 **************************************
 *
 * Functional description
 *	Disastrous lock manager bug.  Issue message and abort process.
 *
 **************************************/
	TEXT s[2 * MAXPATHLEN];

#ifdef WIN_NT
	sprintf(s, "Fatal lock manager error: %s, errno: %ld", string, ERRNO);
#else
	sprintf(s, "Fatal lock manager error: %s, errno: %d", string, ERRNO);
#endif

#if !(defined WIN_NT)
	// The strerror() function returns the appropriate description string,
	// or an unknown error message if the error code is unknown.
	if (errno)
	{
		strcat(s, "\n--");
		strcat(s, strerror(errno));
	}
#endif

	if (!m_bugcheck)
	{
		m_bugcheck = true;

		// The lock table has some problem - copy it for later analysis

		TEXT buffer[MAXPATHLEN];
		gds__prefix_lock(buffer, "fb_lock_table.dump");
		const TEXT* const lock_file = buffer;
		FILE* const fd = fopen(lock_file, "wb");
		if (fd)
		{
			FB_UNUSED(fwrite(m_sharedMemory->getHeader(), 1, m_sharedMemory->getHeader()->lhb_used, fd));
			fclose(fd);
		}

		// If the current mutex acquirer is in the same process, release the mutex

		if (m_sharedMemory->getHeader() && (m_sharedMemory->getHeader()->lhb_active_owner > 0))
		{
			const own* const owner = (own*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_active_owner);
			const prc* const process = (prc*) SRQ_ABS_PTR(owner->own_process);
			if (process->prc_process_id == PID)
				release_shmem(m_sharedMemory->getHeader()->lhb_active_owner);
		}

		if (statusVector)
		{
			*statusVector << Arg::Gds(isc_lockmanerr) << Arg::Gds(isc_random) << string;
			return;
		}
	}

#ifdef DEV_BUILD
   /* In the worst case this code makes it possible to attach live process
	* and see shared memory data.
	if (isatty(2))
		fprintf(stderr, "Attach to pid=%d\n", getpid());
	else
		gds__log("Attach to pid=%d\n", getpid());
	sleep(120);
	*/
#endif

	fb_utils::logAndDie(s);
}


SRQ_PTR LockManager::create_owner(Arg::StatusVector& statusVector,
								  LOCK_OWNER_T owner_id,
								  UCHAR owner_type)
{
/**************************************
 *
 *	c r e a t e _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Create an owner block.
 *
 **************************************/
	if (m_sharedMemory->getHeader()->mhb_type != SharedMemoryBase::SRAM_LOCK_MANAGER ||
		m_sharedMemory->getHeader()->mhb_version != LHB_VERSION)
	{
		TEXT bug_buffer[BUFFER_TINY];
		sprintf(bug_buffer, "inconsistent lock table type/version; found %d/%d, expected %d/%d",
			m_sharedMemory->getHeader()->mhb_type, m_sharedMemory->getHeader()->mhb_version,
			SharedMemoryBase::SRAM_LOCK_MANAGER, LHB_VERSION);
		bug(&statusVector, bug_buffer);
		return 0;
	}

	// Allocate a process block, if required

	if (!m_processOffset)
	{
		if (!create_process(statusVector))
		{
			return 0;
		}
	}

	// Look for a previous instance of owner.  If we find one, get rid of it.

	srq* lock_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->lhb_owners, lock_srq)
	{
		own* owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_lhb_owners));
		if (owner->own_owner_id == owner_id && (UCHAR) owner->own_owner_type == owner_type)
		{
			purge_owner(DUMMY_OWNER, owner);	// purging owner_offset has not been set yet
			break;
		}
	}

	// Allocate an owner block

	own* owner = 0;
	if (SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_free_owners))
	{
		if (!(owner = (own*) alloc(sizeof(own), &statusVector)))
		{
			return 0;
		}
	}
	else
	{
		owner = (own*) ((UCHAR*) SRQ_NEXT(m_sharedMemory->getHeader()->lhb_free_owners) -
			OFFSET(own*, own_lhb_owners));
		remove_que(&owner->own_lhb_owners);
	}

	if (!init_owner_block(statusVector, owner, owner_type, owner_id))
	{
		return 0;
	}

	insert_tail(&m_sharedMemory->getHeader()->lhb_owners, &owner->own_lhb_owners);

	prc* const process = (prc*) SRQ_ABS_PTR(owner->own_process);
	insert_tail(&process->prc_owners, &owner->own_prc_owners);

	probe_processes();

	return SRQ_REL_PTR(owner);
}


bool LockManager::create_process(Arg::StatusVector& statusVector)
{
/**************************************
 *
 *	c r e a t e _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *	Create a process block.
 *
 **************************************/
	srq* lock_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->lhb_processes, lock_srq)
	{
		prc* process = (prc*) ((UCHAR*) lock_srq - OFFSET(prc*, prc_lhb_processes));
		if (process->prc_process_id == PID)
		{
			purge_process(process);
			break;
		}
	}

	prc* process = NULL;
	if (SRQ_EMPTY(m_sharedMemory->getHeader()->lhb_free_processes))
	{
		if (!(process = (prc*) alloc(sizeof(prc), &statusVector)))
			return false;
	}
	else
	{
		process = (prc*) ((UCHAR*) SRQ_NEXT(m_sharedMemory->getHeader()->lhb_free_processes) -
					   OFFSET(prc*, prc_lhb_processes));
		remove_que(&process->prc_lhb_processes);
	}

	process->prc_type = type_lpr;
	process->prc_process_id = PID;
	SRQ_INIT(process->prc_owners);
	SRQ_INIT(process->prc_lhb_processes);
	process->prc_flags = 0;

	insert_tail(&m_sharedMemory->getHeader()->lhb_processes, &process->prc_lhb_processes);

	if (m_sharedMemory->eventInit(&process->prc_blocking) != FB_SUCCESS)
	{
		statusVector << Arg::Gds(isc_lockmanerr);
		return false;
	}

	m_processOffset = SRQ_REL_PTR(process);

#if defined HAVE_OBJECT_MAP
	m_process = m_sharedMemory->mapObject<prc>(statusVector, m_processOffset);
#else
	m_process = process;
#endif

	if (!m_process)
		return false;

	if (m_useBlockingThread)
	{
		try
		{
			Thread::start(blocking_action_thread, this, THREAD_high);
		}
		catch (const Exception& ex)
		{
			ISC_STATUS_ARRAY vector;
			ex.stuff_exception(vector);

			statusVector << Arg::Gds(isc_lockmanerr);
			statusVector << Arg::StatusVector(vector);

			return false;
		}
	}

	return true;
}


void LockManager::deadlock_clear()
{
/**************************************
 *
 *	d e a d l o c k _ c l e a r
 *
 **************************************
 *
 * Functional description
 *	Clear deadlock and scanned bits for pending requests
 *	in preparation for a deadlock scan.
 *
 **************************************/
	ASSERT_ACQUIRED;
	srq* lock_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->lhb_owners, lock_srq)
	{
		own* const owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_lhb_owners));

		srq* lock_srq2;
		SRQ_LOOP(owner->own_pending, lock_srq2)
		{
			lrq* const request = (lrq*) ((UCHAR*) lock_srq2 - OFFSET(lrq*, lrq_own_pending));
			fb_assert(request->lrq_flags & LRQ_pending);
			request->lrq_flags &= ~(LRQ_deadlock | LRQ_scanned);
		}
	}
}


lrq* LockManager::deadlock_scan(own* owner, lrq* request)
{
/**************************************
 *
 *	d e a d l o c k _ s c a n
 *
 **************************************
 *
 * Functional description
 *	Given an owner block that has been stalled for some time, find
 *	a deadlock cycle if there is one.  If a deadlock is found, return
 *	the address of a pending lock request in the deadlock request.
 *	If no deadlock is found, return null.
 *
 **************************************/
	LOCK_TRACE(("deadlock_scan: owner %ld request %ld\n", SRQ_REL_PTR(owner),
			   SRQ_REL_PTR(request)));

	ASSERT_ACQUIRED;
	++(m_sharedMemory->getHeader()->lhb_scans);
	post_history(his_scan, request->lrq_owner, request->lrq_lock, SRQ_REL_PTR(request), true);
	deadlock_clear();

#ifdef VALIDATE_LOCK_TABLE
	validate_lhb(m_sharedMemory->getHeader());
#endif

	bool maybe_deadlock = false;
	lrq* victim = deadlock_walk(request, &maybe_deadlock);

	// Only when it is certain that this request is not part of a deadlock do we
	// mark this request as 'scanned' so that we will not check this request again.
	// Note that this request might be part of multiple deadlocks.

	if (!victim && !maybe_deadlock)
		owner->own_flags |= OWN_scanned;
#ifdef DEBUG_LM
	else if (!victim && maybe_deadlock)
		DEBUG_MSG(0, ("deadlock_scan: not marking due to maybe_deadlock\n"));
#endif

	return victim;
}


lrq* LockManager::deadlock_walk(lrq* request, bool* maybe_deadlock)
{
/**************************************
 *
 *	d e a d l o c k _ w a l k
 *
 **************************************
 *
 * Functional description
 *	Given a request that is waiting, determine whether a deadlock has
 *	occurred.
 *
 **************************************/

	// If this request was scanned for deadlock earlier than don't visit it again

	if (request->lrq_flags & LRQ_scanned)
		return NULL;

	// If this request has been seen already during this deadlock-walk, then we
	// detected a circle in the wait-for graph.  Return "deadlock".

	if (request->lrq_flags & LRQ_deadlock)
	{
#ifdef DEBUG_TRACE_DEADLOCKS
		const own* owner = (own*) SRQ_ABS_PTR(request->lrq_owner);
		const prc* proc = (prc*) SRQ_ABS_PTR(owner->own_process);
		gds__log("deadlock chain: OWNER BLOCK %6"SLONGFORMAT"\tProcess id: %6d\tFlags: 0x%02X ",
			request->lrq_owner, proc->prc_process_id, owner->own_flags);
#endif
		return request;
	}

	// Remember that this request is part of the wait-for graph

	request->lrq_flags |= LRQ_deadlock;

	// Check if this is a conversion request

	const bool conversion = (request->lrq_state > LCK_null);

	// Find the parent lock of the request

	lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

	// Loop thru the requests granted against the lock.  If any granted request is
	// blocking the request we're handling, recurse to find what's blocking him.

	srq* lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		lrq* block = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));

		if (conversion)
		{
			// Don't pursue our own lock-request again

			if (request == block)
				continue;

			// Since lock conversions can't follow the fairness rules (to avoid
			// deadlocks), only granted lock requests need to be examined.
			// If lock-ordering is turned off (opening the door for starvation),
			// only granted requests can block our request.

			if (compatibility[request->lrq_requested][block->lrq_state])
				continue;
		}
		else
		{
			// Don't pursue our own lock-request again.  In addition, don't look
			// at requests that arrived after our request because lock-ordering
			// is in effect.

			if (request == block)
				break;

			// Since lock ordering is in effect, granted locks and waiting
			// requests that arrived before our request could block us

			const UCHAR max_state = MAX(block->lrq_state, block->lrq_requested);

			if (compatibility[request->lrq_requested][max_state])
			{
				continue;
			}
		}

		// Don't pursue lock owners that still have to finish processing their AST.
		// If the blocking queue is not empty, then the owner still has some
		// AST's to process (or lock reposts).
		// hvlad: also lock maybe just granted to owner and blocked owners have no
		// time to send blocking AST
		// Remember this fact because they still might be part of a deadlock.

		own* const owner = (own*) SRQ_ABS_PTR(block->lrq_owner);

		if ((owner->own_flags & (OWN_signaled | OWN_wakeup)) || !SRQ_EMPTY((owner->own_blocks)) ||
			(block->lrq_flags & LRQ_just_granted))
		{
			*maybe_deadlock = true;
			continue;
		}

		// YYY: Note: can the below code be moved to the
		// start of this block?  Before the OWN_signaled check?

		srq* lock_srq2;
		SRQ_LOOP(owner->own_pending, lock_srq2)
		{
			lrq* target = (lrq*) ((UCHAR*) lock_srq2 - OFFSET(lrq*, lrq_own_pending));
			fb_assert(target->lrq_flags & LRQ_pending);

			// hvlad: don't pursue requests that are waiting with a timeout
			// as such a circle in the wait-for graph will be broken automatically
			// when the permitted timeout expires

			if (target->lrq_flags & LRQ_wait_timeout) {
				continue;
			}

			// Check who is blocking the request whose owner is blocking the input request

			if (target = deadlock_walk(target, maybe_deadlock))
			{
#ifdef DEBUG_TRACE_DEADLOCKS
				const own* const owner2 = (own*) SRQ_ABS_PTR(request->lrq_owner);
				const prc* const proc = (prc*) SRQ_ABS_PTR(owner2->own_process);
				gds__log("deadlock chain: OWNER BLOCK %6"SLONGFORMAT"\tProcess id: %6d\tFlags: 0x%02X ",
					request->lrq_owner, proc->prc_process_id, owner2->own_flags);
#endif
				return target;
			}
		}
	}

	// This branch of the wait-for graph is exhausted, the current waiting
	// request is not part of a deadlock

	request->lrq_flags &= ~LRQ_deadlock;
	request->lrq_flags |= LRQ_scanned;
	return NULL;
}


#ifdef DEBUG_LM

static ULONG delay_count = 0;
static ULONG last_signal_line = 0;
static ULONG last_delay_line = 0;

void LockManager::debug_delay(ULONG lineno)
{
/**************************************
 *
 *	d e b u g _ d e l a y
 *
 **************************************
 *
 * Functional description
 *	This is a debugging routine, the purpose of which is to slow
 *	down operations in order to expose windows of critical
 *	sections.
 *
 **************************************/
	ULONG i;

	// Delay for a while

	last_delay_line = lineno;
	for (i = 0; i < 10000; i++)
		; // Nothing

	// Occasionally crash for robustness testing
/*
	if ((delay_count % 500) == 0)
		exit (-1);
*/

	for (i = 0; i < 10000; i++)
		; // Nothing
}
#endif


lbl* LockManager::find_lock(USHORT series,
							const UCHAR* value,
							USHORT length,
							USHORT* slot)
{
/**************************************
 *
 *	f i n d _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Find a lock block given a resource
 *	name. If it doesn't exist, the hash
 *	slot will be useful for enqueing a
 *	lock.
 *
 **************************************/

	// Hash the value preserving its distribution as much as possible

	ULONG hash_value = 0;
	{ // scope
		UCHAR* p = NULL; // silence uninitialized warning
		const UCHAR* q = value;
		for (USHORT l = 0; l < length; l++)
		{
			if (!(l & 3))
				p = (UCHAR*) &hash_value;
			*p++ += *q++;
		}
	} // scope

	// See if the lock already exists

	const USHORT hash_slot = *slot = (USHORT) (hash_value % m_sharedMemory->getHeader()->lhb_hash_slots);
	ASSERT_ACQUIRED;
	srq* const hash_header = &m_sharedMemory->getHeader()->lhb_hash[hash_slot];

	for (srq* lock_srq = (SRQ) SRQ_ABS_PTR(hash_header->srq_forward);
		 lock_srq != hash_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
	{
		lbl* lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_hash));
		if (lock->lbl_series != series || lock->lbl_length != length)
		{
			continue;
		}

		if (!length || !memcmp(value, lock->lbl_key, length))
			return lock;
	}

	return NULL;
}


lrq* LockManager::get_request(SRQ_PTR offset)
{
/**************************************
 *
 *	g e t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Locate and validate user supplied request offset.
 *
 **************************************/
	TEXT s[BUFFER_TINY];

	lrq* request = (lrq*) SRQ_ABS_PTR(offset);
	if (offset == -1 || request->lrq_type != type_lrq)
	{
		sprintf(s, "invalid lock id (%"SLONGFORMAT")", offset);
		bug(NULL, s);
	}

	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	if (lock->lbl_type != type_lbl)
	{
		sprintf(s, "invalid lock (%"SLONGFORMAT")", offset);
		bug(NULL, s);
	}

	return request;
}


void LockManager::grant(lrq* request, lbl* lock)
{
/**************************************
 *
 *	g r a n t
 *
 **************************************
 *
 * Functional description
 *	Grant a lock request.  If the lock is a conversion, assume the caller
 *	has already decremented the former lock type count in the lock block.
 *
 **************************************/

	// Request must be for THIS lock
	CHECK(SRQ_REL_PTR(lock) == request->lrq_lock);

	post_history(his_grant, request->lrq_owner, request->lrq_lock, SRQ_REL_PTR(request), true);

	++lock->lbl_counts[request->lrq_requested];
	request->lrq_state = request->lrq_requested;
	if (request->lrq_data)
	{
		remove_que(&lock->lbl_lhb_data);
		if ( (lock->lbl_data = request->lrq_data) )
			insert_data_que(lock);
		request->lrq_data = 0;
	}

	lock->lbl_state = lock_state(lock);

	if (request->lrq_flags & LRQ_pending)
	{
		remove_que(&request->lrq_own_pending);
		request->lrq_flags &= ~LRQ_pending;
		lock->lbl_pending_lrq_count--;
	}

	post_wakeup((own*) SRQ_ABS_PTR(request->lrq_owner));
}


bool LockManager::grant_or_que(Attachment* attachment, lrq* request, lbl* lock, SSHORT lck_wait)
{
/**************************************
 *
 *	g r a n t _ o r _ q u e
 *
 **************************************
 *
 * Functional description
 *	There is a request against an existing lock.  If the request
 *	is compatible with the lock, grant it.  Otherwise lock_srq it.
 *	If the lock is lock_srq-ed, set up the machinery to do a deadlock
 *	scan in awhile.
 *
 **************************************/
	request->lrq_lock = SRQ_REL_PTR(lock);

	// Compatible requests are easy to satify.  Just post the request to the lock,
	// update the lock state, release the data structure, and we're done.

	if (compatibility[request->lrq_requested][lock->lbl_state])
	{
		if (request->lrq_requested == LCK_null || lock->lbl_pending_lrq_count == 0)
		{
			grant(request, lock);
			post_pending(lock);
			return true;
		}
	}

	// The request isn't compatible with the current state of the lock.
	// If we haven't be asked to wait for the lock, return now.

	if (lck_wait)
	{
		const SRQ_PTR request_offset = SRQ_REL_PTR(request);

		wait_for_request(attachment, request, lck_wait);

		request = (lrq*) SRQ_ABS_PTR(request_offset);

		if (!(request->lrq_flags & LRQ_rejected))
			return true;
	}

	post_history(his_deny, request->lrq_owner, request->lrq_lock, SRQ_REL_PTR(request), true);
	ASSERT_ACQUIRED;
	++(m_sharedMemory->getHeader()->lhb_denies);
	if (lck_wait < 0)
		++(m_sharedMemory->getHeader()->lhb_timeouts);

	release_request(request);

	return false;
}


bool LockManager::init_owner_block(Arg::StatusVector& statusVector, own* owner, UCHAR owner_type,
	LOCK_OWNER_T owner_id)
{
/**************************************
 *
 *	i n i t _ o w n e r _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Initialize the passed owner block nice and new.
 *
 **************************************/

	owner->own_type = type_own;
	owner->own_owner_type = owner_type;
	owner->own_flags = 0;
	owner->own_count = 1;
	owner->own_owner_id = owner_id;
	owner->own_process = m_processOffset;
	owner->own_thread_id = 0;
	SRQ_INIT(owner->own_lhb_owners);
	SRQ_INIT(owner->own_prc_owners);
	SRQ_INIT(owner->own_requests);
	SRQ_INIT(owner->own_blocks);
	SRQ_INIT(owner->own_pending);
	owner->own_acquire_time = 0;
	owner->own_waits = 0;
	owner->own_ast_count = 0;

	if (m_sharedMemory->eventInit(&owner->own_wakeup) != FB_SUCCESS)
	{
		statusVector << Arg::Gds(isc_lockmanerr);
		return false;
	}

	return true;
}


bool LockManager::initialize(SharedMemoryBase* sm, bool initializeMemory)
{
/**************************************
 *
 *	i n i t i a l i z e
 *
 **************************************
 *
 * Functional description
 *	Initialize the lock header block.  The caller is assumed
 *	to have an exclusive lock on the lock file.
 *
 **************************************/

	m_sharedFileCreated = initializeMemory;

	// reset m_sharedMemory in advance to be able to use SRQ_BASE macro
	m_sharedMemory.reset(reinterpret_cast<SharedMemory<lhb>*>(sm));

#ifdef USE_SHMEM_EXT
	if (m_extents.getCount() == 0)
	{
		Extent zero;
		zero = *this;
		m_extents.push(zero);
	}
#endif

	if (!initializeMemory)
	{
		return true;
	}

	lhb* hdr = m_sharedMemory->getHeader();
	memset(hdr, 0, sizeof(lhb));
	hdr->mhb_type = SharedMemoryBase::SRAM_LOCK_MANAGER;
	hdr->mhb_version = LHB_VERSION;
	hdr->mhb_timestamp = TimeStamp::getCurrentTimeStamp().value();

	hdr->lhb_type = type_lhb;

	// Mark ourselves as active owner to prevent fb_assert() checks
	hdr->lhb_active_owner = DUMMY_OWNER;	// In init of lock system

	SRQ_INIT(hdr->lhb_processes);
	SRQ_INIT(hdr->lhb_owners);
	SRQ_INIT(hdr->lhb_free_processes);
	SRQ_INIT(hdr->lhb_free_owners);
	SRQ_INIT(hdr->lhb_free_locks);
	SRQ_INIT(hdr->lhb_free_requests);

	int hash_slots = m_config->getLockHashSlots();
	if (hash_slots < HASH_MIN_SLOTS)
		hash_slots = HASH_MIN_SLOTS;
	if (hash_slots > HASH_MAX_SLOTS)
		hash_slots = HASH_MAX_SLOTS;

	hdr->lhb_hash_slots = (USHORT) hash_slots;
	hdr->lhb_scan_interval = m_config->getDeadlockTimeout();
	hdr->lhb_acquire_spins = m_acquireSpins;

	// Initialize lock series data queues and lock hash chains

	USHORT i;
	SRQ lock_srq;
	for (i = 0, lock_srq = hdr->lhb_data; i < LCK_MAX_SERIES; i++, lock_srq++)
	{
		SRQ_INIT((*lock_srq));
	}
	for (i = 0, lock_srq = hdr->lhb_hash; i < hdr->lhb_hash_slots; i++, lock_srq++)
	{
		SRQ_INIT((*lock_srq));
	}

	// Set lock_ordering flag for the first time

	const ULONG length = sizeof(lhb) + (hdr->lhb_hash_slots * sizeof(hdr->lhb_hash[0]));
	hdr->lhb_length = m_sharedMemory->sh_mem_length_mapped;
	hdr->lhb_used = FB_ALIGN(length, FB_ALIGNMENT);

	shb* secondary_header = (shb*) alloc(sizeof(shb), NULL);
	if (!secondary_header)
	{
		fb_utils::logAndDie("Fatal lock manager error: lock manager out of room");
	}

	hdr->lhb_secondary = SRQ_REL_PTR(secondary_header);
	secondary_header->shb_type = type_shb;
	secondary_header->shb_remove_node = 0;
	secondary_header->shb_insert_que = 0;
	secondary_header->shb_insert_prior = 0;

	// Allocate a sufficiency of history blocks

	his* history = NULL;
	for (USHORT j = 0; j < 2; j++)
	{
		SRQ_PTR* prior = (j == 0) ? &hdr->lhb_history : &secondary_header->shb_history;

		for (i = 0; i < HISTORY_BLOCKS; i++)
		{
			if (!(history = (his*) alloc(sizeof(his), NULL)))
			{
				fb_utils::logAndDie("Fatal lock manager error: lock manager out of room");
			}
			*prior = SRQ_REL_PTR(history);
			history->his_type = type_his;
			history->his_operation = 0;
			prior = &history->his_next;
		}

		history->his_next = (j == 0) ? hdr->lhb_history : secondary_header->shb_history;
	}

	// Done initializing, unmark owner information
	hdr->lhb_active_owner = 0;

	return true;
}


void LockManager::insert_data_que(lbl* lock)
{
/**************************************
 *
 *	i n s e r t _ d a t a _ q u e
 *
 **************************************
 *
 * Functional description
 *	Insert a node in the lock series data queue
 *	in sorted (ascending) order by lock data.
 *
 **************************************/

	if (lock->lbl_series < LCK_MAX_SERIES && lock->lbl_data)
	{
		SRQ data_header = &m_sharedMemory->getHeader()->lhb_data[lock->lbl_series];

		SRQ lock_srq;
		for (lock_srq = (SRQ) SRQ_ABS_PTR(data_header->srq_forward);
			 lock_srq != data_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
		{
			const lbl* lock2 = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock2->lbl_series == lock->lbl_series);

			if (lock->lbl_data <= lock2->lbl_data)
				break;
		}

		insert_tail(lock_srq, &lock->lbl_lhb_data);
	}
}


void LockManager::insert_tail(SRQ lock_srq, SRQ node)
{
/**************************************
 *
 *	i n s e r t _ t a i l
 *
 **************************************
 *
 * Functional description
 *	Insert a node at the tail of a lock_srq.
 *
 *	To handle the event of the process terminating during
 *	the insertion of the node, we set values in the shb to
 *	indicate the node being inserted.
 *	Then, should we be unable to complete
 *	the node insert, the next process into the lock table
 *	will notice the uncompleted work and undo it,
 *	eg: it will put the queue back to the state
 *	prior to the insertion being started.
 *
 **************************************/
	ASSERT_ACQUIRED;
	shb* const recover = (shb*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_secondary);
	DEBUG_DELAY;
	recover->shb_insert_que = SRQ_REL_PTR(lock_srq);
	DEBUG_DELAY;
	recover->shb_insert_prior = lock_srq->srq_backward;
	DEBUG_DELAY;

	node->srq_forward = SRQ_REL_PTR(lock_srq);
	DEBUG_DELAY;
	node->srq_backward = lock_srq->srq_backward;

	DEBUG_DELAY;
	SRQ prior = (SRQ) SRQ_ABS_PTR(lock_srq->srq_backward);
	DEBUG_DELAY;
	prior->srq_forward = SRQ_REL_PTR(node);
	DEBUG_DELAY;
	lock_srq->srq_backward = SRQ_REL_PTR(node);
	DEBUG_DELAY;

	recover->shb_insert_que = 0;
	DEBUG_DELAY;
	recover->shb_insert_prior = 0;
	DEBUG_DELAY;
}


bool LockManager::internal_convert(Attachment* attachment,
								   Arg::StatusVector& statusVector,
								   SRQ_PTR request_offset,
								   UCHAR type,
								   SSHORT lck_wait,
								   lock_ast_t ast_routine,
								   void* ast_argument)
{
/**************************************
 *
 *	i n t e r n a l _ c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Perform a lock conversion, if possible.  If the lock cannot be
 *	granted immediately, either return immediately or wait depending
 *	on a wait flag.  If the lock is granted return true, otherwise
 *	return false.  Note: if the conversion would cause a deadlock,
 *	false is returned even if wait was requested.
 *
 **************************************/
	ASSERT_ACQUIRED;
	lrq* request = get_request(request_offset);
	lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	const SRQ_PTR owner_offset = request->lrq_owner;
	post_history(his_convert, owner_offset, request->lrq_lock, request_offset, true);
	request->lrq_requested = type;
	request->lrq_flags &= ~LRQ_blocking_seen;

	// Compute the state of the lock without the request

	--lock->lbl_counts[request->lrq_state];
	const UCHAR temp = lock_state(lock);

	// If the requested lock level is compatible with the current state
	// of the lock, just grant the request.  Easy enough.

	if (compatibility[type][temp])
	{
		request->lrq_ast_routine = ast_routine;
		request->lrq_ast_argument = ast_argument;
		grant(request, lock);
		post_pending(lock);
		return true;
	}

	++lock->lbl_counts[request->lrq_state];

	// If we weren't requested to wait, just forget about the whole thing.
	// Otherwise wait for the request to be granted or rejected.

	if (lck_wait)
	{
		bool new_ast;
		if (request->lrq_ast_routine != ast_routine || request->lrq_ast_argument != ast_argument)
		{
			new_ast = true;
		}
		else
			new_ast = false;

		wait_for_request(attachment, request, lck_wait);

		request = (lrq*) SRQ_ABS_PTR(request_offset);
		lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

		if (!(request->lrq_flags & LRQ_rejected))
		{
			if (new_ast)
			{
				request = (lrq*) SRQ_ABS_PTR(request_offset);
				request->lrq_ast_routine = ast_routine;
				request->lrq_ast_argument = ast_argument;
			}
			return true;
		}

		post_pending(lock);
	}

	request->lrq_requested = request->lrq_state;
	ASSERT_ACQUIRED;
	++(m_sharedMemory->getHeader()->lhb_denies);
	if (lck_wait < 0)
		++(m_sharedMemory->getHeader()->lhb_timeouts);

	statusVector << Arg::Gds(lck_wait > 0 ? isc_deadlock :
		(lck_wait < 0 ? isc_lock_timeout : isc_lock_conflict));

	return false;
}


void LockManager::internal_dequeue(SRQ_PTR request_offset)
{
/**************************************
 *
 *	i n t e r n a l _ d e q u e u e
 *
 **************************************
 *
 * Functional description
 *	Release an outstanding lock.
 *
 **************************************/

	// Acquire the data structure, and compute addresses of both lock
	// request and lock

	lrq* request = get_request(request_offset);
	post_history(his_deq, request->lrq_owner, request->lrq_lock, request_offset, true);
	request->lrq_ast_routine = NULL;
	release_request(request);
}


USHORT LockManager::lock_state(const lbl* lock)
{
/**************************************
 *
 *	l o c k _ s t a t e
 *
 **************************************
 *
 * Functional description
 *	Compute the current state of a lock.
 *
 **************************************/

	if (lock->lbl_counts[LCK_EX])
		return LCK_EX;
	if (lock->lbl_counts[LCK_PW])
		return LCK_PW;
	if (lock->lbl_counts[LCK_SW])
		return LCK_SW;
	if (lock->lbl_counts[LCK_PR])
		return LCK_PR;
	if (lock->lbl_counts[LCK_SR])
		return LCK_SR;
	if (lock->lbl_counts[LCK_null])
		return LCK_null;

	return LCK_none;
}


void LockManager::post_blockage(Attachment* attachment, lrq* request, lbl* lock)
{
/**************************************
 *
 *	p o s t _ b l o c k a g e
 *
 **************************************
 *
 * Functional description
 *	The current request is blocked.  Post blocking notices to
 *	any process blocking the request.
 *
 **************************************/
	const SRQ_PTR owner_offset = request->lrq_owner;
	const own* owner = (own*) SRQ_ABS_PTR(owner_offset);

	const SRQ_PTR request_offset = SRQ_REL_PTR(request);
	const SRQ_PTR lock_offset = SRQ_REL_PTR(lock);

	ASSERT_ACQUIRED;
	CHECK(request->lrq_flags & LRQ_pending);

	srq* lock_srq = SRQ_NEXT(lock->lbl_requests);
	while (lock_srq != &lock->lbl_requests)
	{
		lrq* const block = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		own* const blocking_owner = (own*) SRQ_ABS_PTR(block->lrq_owner);

		// Figure out if this lock request is blocking our own lock request.
		// Of course, our own request cannot block ourselves.  Compatible
		// requests don't block us, and if there is no AST routine for the
		// request the block doesn't matter as we can't notify anyone.
		// If the owner has marked the request with LRQ_blocking_seen
		// then the blocking AST has been delivered and the owner promises
		// to release the lock as soon as possible (so don't bug the owner).

		if (block == request ||
			blocking_owner == owner ||
			compatibility[request->lrq_requested][block->lrq_state] ||
			!block->lrq_ast_routine ||
			(block->lrq_flags & LRQ_blocking_seen))
		{
			lock_srq = SRQ_NEXT((*lock_srq));
			continue;
		}

		// Add the blocking request to the list of blocks if it's not
		// there already (LRQ_blocking)

		if (!(block->lrq_flags & LRQ_blocking))
		{
			insert_tail(&blocking_owner->own_blocks, &block->lrq_own_blocks);
			block->lrq_flags |= LRQ_blocking;
			block->lrq_flags &= ~(LRQ_blocking_seen | LRQ_just_granted);
		}

		if (blocking_owner->own_flags & OWN_signaled)
		{
			lock_srq = SRQ_NEXT((*lock_srq));
			continue;
		}

		if (!signal_owner(attachment, blocking_owner))
		{
			prc* const process = (prc*) SRQ_ABS_PTR(blocking_owner->own_process);
			purge_process(process);
		}

		// Restart from the beginning, as the list of requests chained to our lock
		// could be changed in the meantime thus invalidating the iterator

		owner = (own*) SRQ_ABS_PTR(owner_offset);
		request = (lrq*) SRQ_ABS_PTR(request_offset);
		lock = (lbl*) SRQ_ABS_PTR(lock_offset);

		lock_srq = SRQ_NEXT(lock->lbl_requests);
	}
}


void LockManager::post_history(USHORT operation,
							   SRQ_PTR process,
							   SRQ_PTR lock,
							   SRQ_PTR request,
							   bool old_version)
{
/**************************************
 *
 *	p o s t _ h i s t o r y
 *
 **************************************
 *
 * Functional description
 *	Post a history item.
 *
 **************************************/
	his* history;

	if (old_version)
	{
		history = (his*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_history);
		ASSERT_ACQUIRED;
		m_sharedMemory->getHeader()->lhb_history = history->his_next;
	}
	else
	{
		ASSERT_ACQUIRED;
		shb* recover = (shb*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_secondary);
		history = (his*) SRQ_ABS_PTR(recover->shb_history);
		recover->shb_history = history->his_next;
	}

	history->his_operation = operation;
	history->his_process = process;
	history->his_lock = lock;
	history->his_request = request;
}


void LockManager::post_pending(lbl* lock)
{
/**************************************
 *
 *	p o s t _ p e n d i n g
 *
 **************************************
 *
 * Functional description
 *	There has been a change in state of a lock.  Check pending
 *	requests to see if something can be granted.  If so, do it.
 *
 **************************************/
#ifdef DEV_BUILD
	USHORT pending_counter = 0;
#endif

	if (lock->lbl_pending_lrq_count == 0)
		return;

	// Loop thru granted requests looking for pending conversions.  If one
	// is found, check to see if it can be granted.  Even if a request cannot
	// be granted for compatibility reason, post_wakeup () that owner so that
	// it can post_blockage() to the newly granted owner of the lock.

	srq* lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		if (!(request->lrq_flags & LRQ_pending))
			continue;
		if (request->lrq_state)
		{
			--lock->lbl_counts[request->lrq_state];
			const UCHAR temp_state = lock_state(lock);
			if (compatibility[request->lrq_requested][temp_state])
				grant(request, lock);
			else
			{
#ifdef DEV_BUILD
				pending_counter++;
#endif
				++lock->lbl_counts[request->lrq_state];
				own* owner = (own*) SRQ_ABS_PTR(request->lrq_owner);
				post_wakeup(owner);
				CHECK(lock->lbl_pending_lrq_count >= pending_counter);
				break;
			}
		}
		else if (compatibility[request->lrq_requested][lock->lbl_state])
			grant(request, lock);
		else
		{
#ifdef DEV_BUILD
			pending_counter++;
#endif
			own* owner = (own*) SRQ_ABS_PTR(request->lrq_owner);
			post_wakeup(owner);
			CHECK(lock->lbl_pending_lrq_count >= pending_counter);
			break;
		}
	}

	CHECK(lock->lbl_pending_lrq_count >= pending_counter);

	if (lock->lbl_pending_lrq_count)
	{
		SRQ_LOOP(lock->lbl_requests, lock_srq)
		{
			lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
			if (request->lrq_flags & LRQ_pending)
				break;

			if (!(request->lrq_flags & (LRQ_blocking | LRQ_blocking_seen)) &&
				request->lrq_ast_routine)
			{
				request->lrq_flags |= LRQ_just_granted;
			}
		}
	}
}


void LockManager::post_wakeup(own* owner)
{
/**************************************
 *
 *	p o s t _ w a k e u p
 *
 **************************************
 *
 * Functional description
 *	Wakeup whoever is waiting on a lock.
 *
 **************************************/

	if (owner->own_waits)
	{
		++(m_sharedMemory->getHeader()->lhb_wakeups);
		owner->own_flags |= OWN_wakeup;
		(void) m_sharedMemory->eventPost(&owner->own_wakeup);
	}
}


bool LockManager::probe_processes()
{
/**************************************
 *
 *	p r o b e _ p r o c e s s e s
 *
 **************************************
 *
 * Functional description
 *	Probe processes to see if any has died.  If one has, get rid of it.
 *
 **************************************/
	ASSERT_ACQUIRED;

	bool purged = false;

	SRQ lock_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->lhb_processes, lock_srq)
	{
		prc* const process = (prc*) ((UCHAR*) lock_srq - OFFSET(prc*, prc_lhb_processes));
		if (process->prc_process_id != PID && !ISC_check_process_existence(process->prc_process_id))
		{
			lock_srq = SRQ_PREV((*lock_srq));
			purge_process(process);
			purged = true;
		}
	}

	return purged;
}


void LockManager::purge_owner(SRQ_PTR purging_owner_offset, own* owner)
{
/**************************************
 *
 *	p u r g e _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Purge an owner and all of its associated locks.
 *
 **************************************/
	LOCK_TRACE(("purge_owner (%ld)\n", purging_owner_offset));

	post_history(his_del_owner, purging_owner_offset, SRQ_REL_PTR(owner), 0, false);

	// Release any locks that are active

	SRQ lock_srq;
	while ((lock_srq = SRQ_NEXT(owner->own_requests)) != &owner->own_requests)
	{
		lrq* request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_requests));
		release_request(request);
	}

	// Release any repost requests left dangling on blocking queue

	while ((lock_srq = SRQ_NEXT(owner->own_blocks)) != &owner->own_blocks)
	{
		lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_blocks));
		remove_que(&request->lrq_own_blocks);
		request->lrq_type = type_null;
		insert_tail(&m_sharedMemory->getHeader()->lhb_free_requests, &request->lrq_lbl_requests);
	}

	// Release owner block

	remove_que(&owner->own_prc_owners);

	remove_que(&owner->own_lhb_owners);
	insert_tail(&m_sharedMemory->getHeader()->lhb_free_owners, &owner->own_lhb_owners);

	owner->own_owner_type = 0;
	owner->own_owner_id = 0;
	owner->own_process = 0;
	owner->own_flags = 0;

	m_sharedMemory->eventFini(&owner->own_wakeup);
}


void LockManager::purge_process(prc* process)
{
/**************************************
 *
 *	p u r g e _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *	Purge all owners of the given process.
 *
 **************************************/
	LOCK_TRACE(("purge_process (%ld)\n", process->prc_process_id));

	SRQ lock_srq;
	while ((lock_srq = SRQ_NEXT(process->prc_owners)) != &process->prc_owners)
	{
		own* owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_prc_owners));
		purge_owner(SRQ_REL_PTR(owner), owner);
	}

	remove_que(&process->prc_lhb_processes);
	insert_tail(&m_sharedMemory->getHeader()->lhb_free_processes, &process->prc_lhb_processes);

	process->prc_process_id = 0;
	process->prc_flags = 0;

	m_sharedMemory->eventFini(&process->prc_blocking);
}


void LockManager::remap_local_owners()
{
/**************************************
 *
 *	r e m a p _ l o c a l _ o w n e r s
 *
 **************************************
 *
 * Functional description
 *	Wakeup all local (in-process) waiting owners
 *  and prepare them for the shared region being remapped.
 *
 **************************************/
	if (!m_processOffset)
		return;

	fb_assert(m_process);

	prc* const process = (prc*) SRQ_ABS_PTR(m_processOffset);
	srq* lock_srq;
	SRQ_LOOP(process->prc_owners, lock_srq)
	{
		own* const owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_prc_owners));

		if (owner->own_waits)
		{
			if (m_sharedMemory->eventPost(&owner->own_wakeup) != FB_SUCCESS)
			{
				bug(NULL, "remap failed: ISC_event_post() failed");
			}
		}
	}

	while (m_waitingOwners.value() > 0)
		THREAD_SLEEP(1);
}


void LockManager::remove_que(SRQ node)
{
/**************************************
 *
 *	r e m o v e _ q u e
 *
 **************************************
 *
 * Functional description
 *	Remove a node from a self-relative lock_srq.
 *
 *	To handle the event of the process terminating during
 *	the removal of the node, we set shb_remove_node to the
 *	node to be removed.  Then, should we be unsuccessful
 *	in the node removal, the next process into the lock table
 *	will notice the uncompleted work and complete it.
 *
 *	Note that the work is completed by again calling this routine,
 *	specifing the node to be removed.  As the prior and following
 *	nodes may have changed prior to the crash, we need to redo the
 *	work only based on what is in <node>.
 *
 **************************************/
	ASSERT_ACQUIRED;
	shb* recover = (shb*) SRQ_ABS_PTR(m_sharedMemory->getHeader()->lhb_secondary);
	DEBUG_DELAY;
	recover->shb_remove_node = SRQ_REL_PTR(node);
	DEBUG_DELAY;

	SRQ lock_srq = (SRQ) SRQ_ABS_PTR(node->srq_forward);

	// The next link might point back to us, or our prior link should
	// the backlink change occur before a crash

	CHECK(lock_srq->srq_backward == SRQ_REL_PTR(node) ||
		  lock_srq->srq_backward == node->srq_backward);
	DEBUG_DELAY;
	lock_srq->srq_backward = node->srq_backward;

	DEBUG_DELAY;
	lock_srq = (SRQ) SRQ_ABS_PTR(node->srq_backward);

	// The prior link might point forward to us, or our following link should
	// the change occur before a crash

	CHECK(lock_srq->srq_forward == SRQ_REL_PTR(node) ||
		  lock_srq->srq_forward == node->srq_forward);
	DEBUG_DELAY;
	lock_srq->srq_forward = node->srq_forward;

	DEBUG_DELAY;
	recover->shb_remove_node = 0;
	DEBUG_DELAY;

	// To prevent trying to remove this entry a second time, which could occur
	// for instance, when we're removing an owner, and crash while removing
	// the owner's blocking requests, reset the lock_srq entry in this node.
	// Note that if we get here, shb_remove_node has been cleared, so we
	// no longer need the queue information.

	SRQ_INIT((*node));
}


void LockManager::release_shmem(SRQ_PTR owner_offset)
{
/**************************************
 *
 *	r e l e a s e _ s h m e m
 *
 **************************************
 *
 * Functional description
 *	Release the mapped lock file.  Advance the event count to wake up
 *	anyone waiting for it.  If there appear to be blocking items
 *	posted.
 *
 **************************************/

	if (!m_sharedMemory->getHeader())
		return;

	if (owner_offset && m_sharedMemory->getHeader()->lhb_active_owner != owner_offset)
		bug(NULL, "release when not owner");

#ifdef VALIDATE_LOCK_TABLE
	// Validate the lock table occasionally (every 500 releases)
	if ((m_sharedMemory->getHeader()->lhb_acquires % (HISTORY_BLOCKS / 2)) == 0)
		validate_lhb(m_sharedMemory->getHeader());
#endif

	if (!m_sharedMemory->getHeader()->lhb_active_owner)
		bug(NULL, "release when not active");

	DEBUG_DELAY;

	m_sharedMemory->getHeader()->lhb_active_owner = 0;

	m_sharedMemory->mutexUnlock();

	DEBUG_DELAY;
}


void LockManager::release_request(lrq* request)
{
/**************************************
 *
 *	r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request.  This is called both by release lock
 *	and by the cleanup handler.
 *
 **************************************/
	ASSERT_ACQUIRED;

	// Start by disconnecting request from both lock and process

	remove_que(&request->lrq_lbl_requests);
	remove_que(&request->lrq_own_requests);

	request->lrq_type = type_null;
	insert_tail(&m_sharedMemory->getHeader()->lhb_free_requests, &request->lrq_lbl_requests);
	lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

	// If the request is marked as blocking, clean it up

	if (request->lrq_flags & LRQ_blocking)
	{
		remove_que(&request->lrq_own_blocks);
		request->lrq_flags &= ~LRQ_blocking;
	}

	// Update counts if we are cleaning up something we're waiting on!
	// This should only happen if we are purging out an owner that died.

	if (request->lrq_flags & LRQ_pending)
	{
		remove_que(&request->lrq_own_pending);
		request->lrq_flags &= ~LRQ_pending;
		lock->lbl_pending_lrq_count--;
	}

	request->lrq_flags &= ~(LRQ_blocking_seen | LRQ_just_granted);

	// If there are no outstanding requests, release the lock

	if (SRQ_EMPTY(lock->lbl_requests))
	{
		CHECK(lock->lbl_pending_lrq_count == 0);

		remove_que(&lock->lbl_lhb_hash);
		remove_que(&lock->lbl_lhb_data);
		lock->lbl_type = type_null;

		insert_tail(&m_sharedMemory->getHeader()->lhb_free_locks, &lock->lbl_lhb_hash);
		return;
	}

	// Re-compute the state of the lock and post any compatible pending requests

	if ((request->lrq_state != LCK_none) && !(--lock->lbl_counts[request->lrq_state]))
	{
		lock->lbl_state = lock_state(lock);
		if (request->lrq_state != LCK_null)
		{
			post_pending(lock);
			return;
		}
	}

	// If a lock enqueue failed because of a deadlock or timeout, the pending
	// request may be holding up compatible, pending lock requests queued
	// behind it.
	// Or, even if this request had been granted, it's now being released,
	// so we might be holding up requests or conversions queued up behind.

	post_pending(lock);
}


bool LockManager::signal_owner(Attachment* attachment, own* blocking_owner)
{
/**************************************
 *
 *	s i g n a l _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Send a signal to a process.
 *
 *      The second parameter is a possible offset to the
 *      blocked owner (or NULL), which is passed on to
 *      blocking_action().
 *
 **************************************/
	ASSERT_ACQUIRED;

	// If a process, other than ourselves, hasn't yet seen a signal
	// that was sent, don't bother to send another one

	DEBUG_DELAY;

	blocking_owner->own_flags |= OWN_signaled;
	DEBUG_DELAY;

	prc* const process = (prc*) SRQ_ABS_PTR(blocking_owner->own_process);

	// Deliver signal either locally or remotely

	if (process->prc_process_id == PID)
	{
		DEBUG_DELAY;
		blocking_action(attachment, SRQ_REL_PTR(blocking_owner));
		DEBUG_DELAY;
		return true;
	}

	DEBUG_DELAY;

	if (m_sharedMemory->eventPost(&process->prc_blocking) == FB_SUCCESS)
		return true;

	DEBUG_MSG(1, ("signal_owner - direct delivery failed\n"));

	blocking_owner->own_flags &= ~OWN_signaled;
	DEBUG_DELAY;

	// We couldn't deliver signal. Let someone to purge the process.
	return false;
}


const USHORT EXPECT_inuse = 0;
const USHORT EXPECT_freed = 1;

const USHORT RECURSE_yes = 0;
const USHORT RECURSE_not = 1;

void LockManager::validate_history(const SRQ_PTR history_header)
{
/**************************************
 *
 *	v a l i d a t e _ h i s t o r y
 *
 **************************************
 *
 * Functional description
 *	Validate a circular list of history blocks.
 *
 **************************************/
	ULONG count = 0;

	LOCK_TRACE(("validate_history: %ld\n", history_header));

	for (const his* history = (his*) SRQ_ABS_PTR(history_header); true;
		 history = (his*) SRQ_ABS_PTR(history->his_next))
	{
		count++;
		CHECK(history->his_type == type_his);
		CHECK(history->his_operation <= his_MAX);
		if (history->his_next == history_header)
			break;
		CHECK(count <= HISTORY_BLOCKS);
	}
}


void LockManager::validate_lhb(const lhb* alhb)
{
/**************************************
 *
 *	v a l i d a t e _ l h b
 *
 **************************************
 *
 * Functional description
 *	Validate the LHB structure and everything that hangs off of it.
 *
 **************************************/

	LOCK_TRACE(("validate_lhb:\n"));

	// Prevent recursive reporting of validation errors
	if (m_bugcheck)
		return;

	CHECK(alhb != NULL);
	CHECK(alhb->mhb_type == SharedMemoryBase::SRAM_LOCK_MANAGER);
	CHECK(alhb->mhb_version == LHB_VERSION);

	CHECK(alhb->lhb_type == type_lhb);

	validate_shb(alhb->lhb_secondary);
	if (alhb->lhb_active_owner > 0)
		validate_owner(alhb->lhb_active_owner, EXPECT_inuse);

	const srq* lock_srq;
	SRQ_LOOP(alhb->lhb_owners, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		const own* const owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_lhb_owners));
		validate_owner(SRQ_REL_PTR(owner), EXPECT_inuse);
	}

	SRQ_LOOP(alhb->lhb_free_owners, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		const own* const owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_lhb_owners));
		validate_owner(SRQ_REL_PTR(owner), EXPECT_freed);
	}

	SRQ_LOOP(alhb->lhb_free_locks, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		const lbl* const lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_hash));
		validate_lock(SRQ_REL_PTR(lock), EXPECT_freed, (SRQ_PTR) 0);
	}

	SRQ_LOOP(alhb->lhb_free_requests, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		validate_request(SRQ_REL_PTR(request), EXPECT_freed, RECURSE_not);
	}

	CHECK(alhb->lhb_used <= alhb->lhb_length);

	validate_history(alhb->lhb_history);

	DEBUG_MSG(0, ("validate_lhb completed:\n"));
}


void LockManager::validate_lock(const SRQ_PTR lock_ptr, USHORT freed, const SRQ_PTR lrq_ptr)
{
/**************************************
 *
 *	v a l i d a t e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Validate the lock structure and everything that hangs off of it.
 *
 **************************************/
	LOCK_TRACE(("validate_lock: %ld\n", lock_ptr));

	const lbl* lock = (lbl*) SRQ_ABS_PTR(lock_ptr);

	if (freed == EXPECT_freed)
		CHECK(lock->lbl_type == type_null);
	else
		CHECK(lock->lbl_type == type_lbl);

	CHECK(lock->lbl_state < LCK_max);

	CHECK(lock->lbl_length <= lock->lbl_size);

	// The lbl_count's should never roll over to be negative
	for (ULONG i = 0; i < FB_NELEM(lock->lbl_counts); i++)
		CHECK(!(lock->lbl_counts[i] & 0x8000));

	// The count of pending locks should never roll over to be negative
	CHECK(!(lock->lbl_pending_lrq_count & 0x8000));

	USHORT direct_counts[LCK_max];
	memset(direct_counts, 0, sizeof(direct_counts));

	ULONG found = 0, found_pending = 0, found_incompatible = 0;
	const srq* lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		// Any requests of a freed lock should also be free
		CHECK(freed == EXPECT_inuse);

		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		// Note: Don't try to validate_request here, it leads to recursion

		if (SRQ_REL_PTR(request) == lrq_ptr)
			found++;

		CHECK(found <= 1);		// check for a loop in the queue

		// Request must be for this lock
		CHECK(request->lrq_lock == lock_ptr);

		if (request->lrq_flags & LRQ_pending)
		{
			// If the request is pending, then it must be incompatible with current
			// state of the lock - OR there is at least one incompatible request
			// in the queue before this request.

			CHECK(!compatibility[request->lrq_requested][lock->lbl_state] || found_incompatible);

			found_pending++;
		}
		else
		{
			// If the request is NOT pending, then it must be rejected or
			// compatible with the current state of the lock

			CHECK((request->lrq_flags & LRQ_rejected) ||
				  (request->lrq_requested == lock->lbl_state) ||
				  compatibility[request->lrq_requested][lock->lbl_state]);
		}

		if (!compatibility[request->lrq_requested][lock->lbl_state])
			found_incompatible++;

		direct_counts[request->lrq_state]++;
	}

	if ((freed == EXPECT_inuse) && (lrq_ptr != 0)) {
		CHECK(found == 1);		// request is in lock's queue
	}

	if (freed == EXPECT_inuse)
	{
		CHECK(found_pending == lock->lbl_pending_lrq_count);

		// The counter in the lock header should match the actual counts
		// lock->lbl_counts [LCK_null] isn't maintained, so don't check it
		for (USHORT j = LCK_null; j < LCK_max; j++)
			CHECK(direct_counts[j] == lock->lbl_counts[j]);
	}
}


void LockManager::validate_owner(const SRQ_PTR own_ptr, USHORT freed)
{
/**************************************
 *
 *	v a l i d a t e _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Validate the owner structure and everything that hangs off of it.
 *
 **************************************/
	LOCK_TRACE(("validate_owner: %ld\n", own_ptr));

	const own* const owner = (own*) SRQ_ABS_PTR(own_ptr);

	CHECK(owner->own_type == type_own);
	if (freed == EXPECT_freed)
		CHECK(owner->own_owner_type == 0);
	else {
		CHECK(owner->own_owner_type <= 2);
	}

	CHECK(owner->own_acquire_time <= m_sharedMemory->getHeader()->lhb_acquires);

	// Check that no invalid flag bit is set
	CHECK(!(owner->own_flags & ~(OWN_scanned | OWN_wakeup | OWN_signaled)));

	const srq* lock_srq;
	SRQ_LOOP(owner->own_requests, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		CHECK(freed == EXPECT_inuse);	// should not be in loop for freed owner

		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_requests));
		validate_request(SRQ_REL_PTR(request), EXPECT_inuse, RECURSE_not);
		CHECK(request->lrq_owner == own_ptr);

		// Make sure that request marked as blocking also exists in the blocking list

		if (request->lrq_flags & LRQ_blocking)
		{
			ULONG found = 0;
			const srq* que2;
			SRQ_LOOP(owner->own_blocks, que2)
			{
				// Validate that the next backpointer points back to us
				const srq* const que2_next = SRQ_NEXT((*que2));
				CHECK(que2_next->srq_backward == SRQ_REL_PTR(que2));

				const lrq* const request2 = (lrq*) ((UCHAR*) que2 - OFFSET(lrq*, lrq_own_blocks));
				CHECK(request2->lrq_owner == own_ptr);

				if (SRQ_REL_PTR(request2) == SRQ_REL_PTR(request))
					found++;

				CHECK(found <= 1);	// watch for loops in queue
			}
			CHECK(found == 1);	// request marked as blocking must be in blocking queue
		}

		// Make sure that request marked as pending also exists in the pending list,
		// as well as in the queue for the lock

		if (request->lrq_flags & LRQ_pending)
		{
			ULONG found = 0;
			const srq* que2;
			SRQ_LOOP(owner->own_pending, que2)
			{
				// Validate that the next backpointer points back to us
				const srq* const que2_next = SRQ_NEXT((*que2));
				CHECK(que2_next->srq_backward == SRQ_REL_PTR(que2));

				const lrq* const request2 = (lrq*) ((UCHAR*) que2 - OFFSET(lrq*, lrq_own_pending));
				CHECK(request2->lrq_owner == own_ptr);

				if (SRQ_REL_PTR(request2) == SRQ_REL_PTR(request))
					found++;

				CHECK(found <= 1);	// watch for loops in queue
			}
			CHECK(found == 1);	// request marked as pending must be in pending queue

			// Make sure the pending request is on the list of requests for the lock

			const lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

			bool found_pending = false;
			const srq* que_of_lbl_requests;
			SRQ_LOOP(lock->lbl_requests, que_of_lbl_requests)
			{
				const lrq* const pending =
					(lrq*) ((UCHAR*) que_of_lbl_requests - OFFSET(lrq*, lrq_lbl_requests));

				if (SRQ_REL_PTR(pending) == SRQ_REL_PTR(request))
				{
					found_pending = true;
					break;
				}
			}

			// pending request must exist in the lock's request queue
			CHECK(found_pending);
		}
	}

	// Check each item in the blocking queue

	SRQ_LOOP(owner->own_blocks, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		CHECK(freed == EXPECT_inuse);	// should not be in loop for freed owner

		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_blocks));
		validate_request(SRQ_REL_PTR(request), EXPECT_inuse, RECURSE_not);

		LOCK_TRACE(("Validate own_block: %ld\n", SRQ_REL_PTR(request)));

		CHECK(request->lrq_owner == own_ptr);

		// A repost won't be in the request list

		if (request->lrq_flags & LRQ_repost)
			continue;

		// Make sure that each block also exists in the request list

		ULONG found = 0;
		const srq* que2;
		SRQ_LOOP(owner->own_requests, que2)
		{
			// Validate that the next backpointer points back to us
			const srq* const que2_next = SRQ_NEXT((*que2));
			CHECK(que2_next->srq_backward == SRQ_REL_PTR(que2));

			const lrq* const request2 = (lrq*) ((UCHAR*) que2 - OFFSET(lrq*, lrq_own_requests));
			CHECK(request2->lrq_owner == own_ptr);

			if (SRQ_REL_PTR(request2) == SRQ_REL_PTR(request))
				found++;

			CHECK(found <= 1);	// watch for loops in queue
		}
		CHECK(found == 1);		// blocking request must be in request queue
	}

	// Check each item in the pending queue

	SRQ_LOOP(owner->own_pending, lock_srq)
	{
		// Validate that the next backpointer points back to us
		const srq* const que_next = SRQ_NEXT((*lock_srq));
		CHECK(que_next->srq_backward == SRQ_REL_PTR(lock_srq));

		CHECK(freed == EXPECT_inuse);	// should not be in loop for freed owner

		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_pending));
		validate_request(SRQ_REL_PTR(request), EXPECT_inuse, RECURSE_not);

		LOCK_TRACE(("Validate own_block: %ld\n", SRQ_REL_PTR(request)));

		CHECK(request->lrq_owner == own_ptr);

		// A repost cannot be pending

		CHECK(!(request->lrq_flags & LRQ_repost));

		// Make sure that each pending request also exists in the request list

		ULONG found = 0;
		const srq* que2;
		SRQ_LOOP(owner->own_requests, que2)
		{
			// Validate that the next backpointer points back to us
			const srq* const que2_next = SRQ_NEXT((*que2));
			CHECK(que2_next->srq_backward == SRQ_REL_PTR(que2));

			const lrq* const request2 = (lrq*) ((UCHAR*) que2 - OFFSET(lrq*, lrq_own_requests));
			CHECK(request2->lrq_owner == own_ptr);

			if (SRQ_REL_PTR(request2) == SRQ_REL_PTR(request))
				found++;

			CHECK(found <= 1);	// watch for loops in queue
		}
		CHECK(found == 1);		// pending request must be in request queue
	}
}


void LockManager::validate_request(const SRQ_PTR lrq_ptr, USHORT freed, USHORT recurse)
{
/**************************************
 *
 *	v a l i d a t e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Validate the request structure and everything that hangs off of it.
 *
 **************************************/
	LOCK_TRACE(("validate_request: %ld\n", lrq_ptr));

	const lrq* const request = (lrq*) SRQ_ABS_PTR(lrq_ptr);

	if (freed == EXPECT_freed)
		CHECK(request->lrq_type == type_null);
	else
		CHECK(request->lrq_type == type_lrq);

	// Check that no invalid flag bit is set
	CHECK(!(request->lrq_flags &
		   		~(LRQ_blocking | LRQ_pending | LRQ_rejected | LRQ_deadlock | LRQ_repost |
					 LRQ_scanned | LRQ_blocking_seen | LRQ_just_granted | LRQ_wait_timeout)));

	// Once a request is rejected, it CAN'T be pending any longer
	if (request->lrq_flags & LRQ_rejected) {
		CHECK(!(request->lrq_flags & LRQ_pending));
	}

	// Can't both be scanned & marked for deadlock walk
	CHECK((request->lrq_flags & (LRQ_deadlock | LRQ_scanned)) != (LRQ_deadlock | LRQ_scanned));

	CHECK(request->lrq_requested < LCK_max);
	CHECK(request->lrq_state < LCK_max);

	if (freed == EXPECT_inuse)
	{
		if (recurse == RECURSE_yes)
			validate_owner(request->lrq_owner, EXPECT_inuse);

		// Reposted request are pseudo requests, not attached to any real lock
		if (!(request->lrq_flags & LRQ_repost))
			validate_lock(request->lrq_lock, EXPECT_inuse, SRQ_REL_PTR(request));
	}
}


void LockManager::validate_shb(const SRQ_PTR shb_ptr)
{
/**************************************
 *
 *	v a l i d a t e _ s h b
 *
 **************************************
 *
 * Functional description
 *	Validate the SHB structure and everything that hangs off of
 *	it.
 *	Of course, it would have been a VERY good thing if someone
 *	had moved this into lhb when we made a unique v4 lock
 *	manager....
 *	1995-April-13 David Schnepper
 *
 **************************************/
	LOCK_TRACE(("validate_shb: %ld\n", shb_ptr));

	const shb* secondary_header = (shb*) SRQ_ABS_PTR(shb_ptr);

	CHECK(secondary_header->shb_type == type_shb);

	validate_history(secondary_header->shb_history);
}


void LockManager::wait_for_request(Attachment* attachment, lrq* request, SSHORT lck_wait)
{
/**************************************
 *
 *	w a i t _ f o r _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	There is a request that needs satisfaction, but is waiting for
 *	somebody else.  Mark the request as pending and go to sleep until
 *	the lock gets poked.  When we wake up, see if somebody else has
 *	cleared the pending flag.  If not, go back to sleep.
 * Returns
 *	FB_SUCCESS	- we waited until the request was granted or rejected
 * 	FB_FAILURE - Insufficient resouces to wait (eg: no semaphores)
 *
 **************************************/
	ASSERT_ACQUIRED;

	++(m_sharedMemory->getHeader()->lhb_waits);
	const SLONG scan_interval = m_sharedMemory->getHeader()->lhb_scan_interval;

	// lrq_count will be off if we wait for a pending request
	CHECK(!(request->lrq_flags & LRQ_pending));

	const SRQ_PTR request_offset = SRQ_REL_PTR(request);
	const SRQ_PTR owner_offset = request->lrq_owner;

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	owner->own_flags &= ~(OWN_scanned | OWN_wakeup);
	owner->own_waits++;

	request->lrq_flags &= ~LRQ_rejected;
	request->lrq_flags |= LRQ_pending;
	insert_tail(&owner->own_pending, &request->lrq_own_pending);

	const SRQ_PTR lock_offset = request->lrq_lock;
	lbl* lock = (lbl*) SRQ_ABS_PTR(lock_offset);
	lock->lbl_pending_lrq_count++;

	if (!request->lrq_state)
	{
		// If this is a conversion of an existing lock in LCK_none state -
		// put the lock to the end of the list so it's not taking cuts in the lineup
		remove_que(&request->lrq_lbl_requests);
		insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
	}

	if (lck_wait <= 0)
		request->lrq_flags |= LRQ_wait_timeout;

	SLONG value = m_sharedMemory->eventClear(&owner->own_wakeup);

	// Post blockage. If the blocking owner has disappeared, the blockage
	// may clear spontaneously.

	post_blockage(attachment, request, lock);
	post_history(his_wait, owner_offset, lock_offset, request_offset, true);

	owner = (own*) SRQ_ABS_PTR(owner_offset);
	request = (lrq*) SRQ_ABS_PTR(request_offset);
	lock = (lbl*) SRQ_ABS_PTR(lock_offset);

	time_t current_time = time(NULL);

	// If a lock timeout was requested (wait < 0) then figure
	// out the time when the lock request will timeout

	const time_t lock_timeout = (lck_wait < 0) ? current_time + (-lck_wait) : 0;
	time_t deadlock_timeout = current_time + scan_interval;

	// Wait in a loop until the lock becomes available

#ifdef DEV_BUILD
	ULONG repost_counter = 0;
#endif

	while (true)
	{
		owner = (own*) SRQ_ABS_PTR(owner_offset);
		request = (lrq*) SRQ_ABS_PTR(request_offset);
		lock = (lbl*) SRQ_ABS_PTR(lock_offset);

		// Before starting to wait - look to see if someone resolved
		// the request for us - if so we're out easy!

		if (!(request->lrq_flags & LRQ_pending))
			break;

		int ret = FB_FAILURE;

		// Recalculate when we next want to wake up, the lesser of a
		// deadlock scan interval or when the lock request wanted a timeout

		time_t timeout = deadlock_timeout;
		if (lck_wait < 0 && lock_timeout < deadlock_timeout)
			timeout = lock_timeout;

		// Prepare to wait for a timeout or a wakeup from somebody else

		if (!(owner->own_flags & OWN_wakeup))
		{
			// Re-initialize value each time thru the loop to make sure that the
			// semaphore looks 'un-poked'

			{ // checkout scope
				LockTableCheckout checkout(this, FB_FUNCTION);

				{ // scope
					Firebird::ReadLockGuard guard(m_remapSync, FB_FUNCTION);
					owner = (own*) SRQ_ABS_PTR(owner_offset);
					++m_waitingOwners;
				}

				{ // scope
					Jrd::Attachment::Checkout cout(attachment, FB_FUNCTION);
					ret = m_sharedMemory->eventWait(&owner->own_wakeup, value, (timeout - current_time) * 1000000);
					--m_waitingOwners;
				}
			}

			owner = (own*) SRQ_ABS_PTR(owner_offset);
			request = (lrq*) SRQ_ABS_PTR(request_offset);
			lock = (lbl*) SRQ_ABS_PTR(lock_offset);
		}

		// We've woken up from the wait - now look around and see why we wokeup

		// If somebody else has resolved the lock, we're done

		if (!(request->lrq_flags & LRQ_pending))
			break;

		// See if we wokeup due to another owner deliberately waking us up
		// ret==FB_SUCCESS --> we were deliberately woken up
		// ret==FB_FAILURE --> we still don't know why we woke up

		// Only if we came out of the m_sharedMemory->eventWait() because of a post_wakeup()
		// by another owner is OWN_wakeup set. This is the only FB_SUCCESS case.

		if (ret == FB_SUCCESS)
		{
			value = m_sharedMemory->eventClear(&owner->own_wakeup);
		}

		if (owner->own_flags & OWN_wakeup)
			ret = FB_SUCCESS;
		else
			ret = FB_FAILURE;

		current_time = time(NULL);

		// See if we woke up for a bogus reason - if so
		// go right back to sleep.  We wokeup bogus unless
		// - we weren't deliberatly woken up
		// - it's not time for a deadlock scan
		// - it's not time for the lock timeout to expire
		// Bogus reasons for wakeups include signal reception on some
		// platforms (eg: SUN4) and remapping notification.
		// Note: we allow a 1 second leaway on declaring a bogus
		// wakeup due to timing differences (we use seconds here,
		// eventWait() uses finer granularity)

		if ((ret != FB_SUCCESS) && (current_time + 1 < timeout))
			continue;

		// We apparently woke up for some real reason.
		// Make sure everyone is still with us. Then see if we're still blocked.

		owner->own_flags &= ~OWN_wakeup;

		// See if we've waited beyond the lock timeout -
		// if so we mark our own request as rejected

		// !!! this will be changed to have no dependency on thread_db !!!
		const bool cancelled = JRD_get_thread_data()->checkCancelState(false);

		if (cancelled || (lck_wait < 0 && lock_timeout <= current_time))
		{
			// We're going to reject our lock - it's the callers responsibility
			// to do cleanup and make sure post_pending() is called to wakeup
			// other owners we might be blocking
			request->lrq_flags |= LRQ_rejected;
			remove_que(&request->lrq_own_pending);
			request->lrq_flags &= ~LRQ_pending;
			lock->lbl_pending_lrq_count--;

			// and test - may be timeout due to missing process to deliver request
			probe_processes();
			break;
		}

		// We're going to do some real work - reset when we next want to
		// do a deadlock scan
		deadlock_timeout = current_time + scan_interval;

		// Handle lock event first
		if (ret == FB_SUCCESS)
		{
			// Someone posted our wakeup event, but DIDN'T grant our request.
			// Re-post what we're blocking on and continue to wait.
			// This could happen if the lock was granted to a different request,
			// we have to tell the new owner of the lock that they are blocking us.

			post_blockage(attachment, request, lock);
			continue;
		}

		// See if all the other owners are still alive. Dead ones will be purged,
		// purging one might resolve our lock request.
		// Do not do rescan of owners if we received notification that
		// blocking ASTs have completed - will do it next time if needed.

		if (probe_processes() && !(request->lrq_flags & LRQ_pending))
			break;

		// If we've not previously been scanned for a deadlock and going to wait
		// forever, go do a deadlock scan

		lrq* blocking_request;
		if (!(owner->own_flags & OWN_scanned) &&
			!(request->lrq_flags & LRQ_wait_timeout) &&
			(blocking_request = deadlock_scan(owner, request)))
		{
			// Something has been selected for rejection to prevent a
			// deadlock. Clean things up and go on. We still have to
			// wait for our request to be resolved.

			DEBUG_MSG(0, ("wait_for_request: selecting something for deadlock kill\n"));

			++(m_sharedMemory->getHeader()->lhb_deadlocks);
			blocking_request->lrq_flags |= LRQ_rejected;
			remove_que(&blocking_request->lrq_own_pending);
			blocking_request->lrq_flags &= ~LRQ_pending;
			lbl* const blocking_lock = (lbl*) SRQ_ABS_PTR(blocking_request->lrq_lock);
			blocking_lock->lbl_pending_lrq_count--;

			own* const blocking_owner = (own*) SRQ_ABS_PTR(blocking_request->lrq_owner);
			blocking_owner->own_flags &= ~OWN_scanned;
			if (blocking_request != request)
				post_wakeup(blocking_owner);
			// else
			// We rejected our own request to avoid a deadlock.
			// When we get back to the top of the master loop we
			// fall out and start cleaning up.
		}
		else
		{
			// Our request is not resolved, all the owners are alive, there's
			// no deadlock -- there's nothing else to do.  Let's
			// make sure our request hasn't been forgotten by reminding
			// all the owners we're waiting - some plaforms under CLASSIC
			// architecture had problems with "missing signals" - which is
			// another reason to repost the blockage.
			// Also, the ownership of the lock could have changed, and we
			// weren't woken up because we weren't next in line for the lock.
			// We need to inform the new owner.

			DEBUG_MSG(0, ("wait_for_request: forcing a resignal of blockers\n"));
			post_blockage(attachment, request, lock);
#ifdef DEV_BUILD
			repost_counter++;
			if (repost_counter % 50 == 0)
			{
				gds__log("wait_for_request: owner %d reposted %ld times for lock %d",
						owner_offset,
						repost_counter,
						lock_offset);
				DEBUG_MSG(0,
						  ("wait_for_request: reposted %ld times for this lock!\n",
						   repost_counter));
			}
#endif
		}
	}

	CHECK(!(request->lrq_flags & LRQ_pending));

	request->lrq_flags &= ~LRQ_wait_timeout;
	owner->own_waits--;
}

void LockManager::mutexBug(int state, char const* text)
{
	string message;
	message.printf("%s: error code %d\n", text, state);
	bug(NULL, message.c_str());
}

#ifdef USE_SHMEM_EXT
void LockManager::Extent::assign(const SharedMemoryBase& p)
{
	SharedMemoryBase* me = this;

	me->setHeader(p.getHeader());
	me->sh_mem_mutex = p.sh_mem_mutex;
	me->sh_mem_length_mapped = p.sh_mem_length_mapped;
	me->sh_mem_handle = p.sh_mem_handle;
	strcpy(me->sh_mem_name, p.sh_mem_name);
}
#endif // USE_SHMEM_EXT

} // namespace
