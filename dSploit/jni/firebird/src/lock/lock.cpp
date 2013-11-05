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
#include "../jrd/common.h"

#include "../lock/lock.h"
#include "../lock/lock_proto.h"

#include "../jrd/ThreadStart.h"
#include "../jrd/jrd.h"
#include "gen/iberror.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/isc_s_proto.h"
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
#define MUTEX_PTR   NULL
#else
#define MUTEX		m_lhb_mutex
#define MUTEX_PTR   &m_lhb_mutex
#endif

#ifdef DEV_BUILD
//#define VALIDATE_LOCK_TABLE
#endif

#ifdef DEV_BUILD
#define ASSERT_ACQUIRED fb_assert(m_header->lhb_active_owner)
#if (defined HAVE_MMAP || defined WIN_NT)
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

// hvlad: enable to log deadlocked owners and its PIDs in firebird.log
//#define DEBUG_TRACE_DEADLOCKS

// CVC: Unlike other definitions, SRQ_PTR is not a pointer to something in lowercase.
// It's LONG.

const SRQ_PTR DUMMY_OWNER = -1;
const SRQ_PTR CREATE_OWNER = -2;

const SLONG HASH_MIN_SLOTS	= 101;
const SLONG HASH_MAX_SLOTS	= 65521;
const USHORT HISTORY_BLOCKS	= 256;

// SRQ_ABS_PTR uses this macro.
#define SRQ_BASE                    ((UCHAR*) m_header)

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

//#define COMPATIBLE(st1, st2)	compatibility[st1 * LCK_max + st2]


namespace Jrd {

Firebird::GlobalPtr<LockManager::DbLockMgrMap> LockManager::g_lmMap;
Firebird::GlobalPtr<Firebird::Mutex> LockManager::g_mapMutex;


LockManager* LockManager::create(const Firebird::string& id)
{
	Firebird::MutexLockGuard guard(g_mapMutex);

	LockManager* lockMgr = NULL;
	if (!g_lmMap->get(id, lockMgr))
	{
		lockMgr = new LockManager(id);

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

		Firebird::MutexLockGuard guard(g_mapMutex);

		if (!lockMgr->release())
		{
			if (!g_lmMap->remove(id))
			{
				fb_assert(false);
			}
		}
	}
}


LockManager::LockManager(const Firebird::string& id)
	: PID(getpid()),
	  m_bugcheck(false),
	  m_sharedFileCreated(false),
	  m_header(NULL),
	  m_process(NULL),
	  m_processOffset(0),
	  m_dbId(getPool(), id),
	  m_localBlockage(false),
	  m_acquireSpins(Config::getLockAcquireSpins()),
	  m_memorySize(Config::getLockMemSize())
#ifdef USE_SHMEM_EXT
	  , m_extents(getPool())
#endif
{
	ISC_STATUS_ARRAY local_status;
	if (!attach_shared_file(local_status))
	{
		Firebird::status_exception::raise(local_status);
	}
}


LockManager::~LockManager()
{
	const SRQ_PTR process_offset = m_processOffset;
	{ // guardian's scope
		LocalGuard guard(this);
		m_processOffset = 0;
	}

	ISC_STATUS_ARRAY local_status;

	if (m_process)
	{
#ifdef USE_BLOCKING_THREAD
		// Wait for AST thread to start (or 5 secs)
		m_startupSemaphore.tryEnter(5);

		// Wakeup the AST thread - it might be blocking
		(void)	// Ignore errors in dtor()
			ISC_event_post(&m_process->prc_blocking);

		// Wait for the AST thread to finish cleanup or for 5 seconds
		m_cleanupSemaphore.tryEnter(5);
#endif

#if defined HAVE_MMAP || defined WIN_NT
		ISC_unmap_object(local_status, /*&m_shmem,*/ (UCHAR**) &m_process, sizeof(prc));
#else
		m_process = NULL;
#endif
	}

	acquire_shmem(DUMMY_OWNER);
	if (process_offset)
	{
		prc* const process = (prc*) SRQ_ABS_PTR(process_offset);
		purge_process(process);
	}
	if (m_header && SRQ_EMPTY(m_header->lhb_processes))
	{
		Firebird::PathName name;
		get_shared_file_name(name);
		ISC_remove_map_file(name.c_str());
#ifdef USE_SHMEM_EXT
		for (ULONG i = 1; i < m_extents.getCount(); ++i)
		{
			get_shared_file_name(name, i);
			ISC_remove_map_file(name.c_str());
		}
#endif //USE_SHMEM_EXT
	}
	release_mutex();

	detach_shared_file(local_status);
#ifdef USE_SHMEM_EXT
	for (ULONG i = 1; i < m_extents.getCount(); ++i)
	{
		ISC_unmap_file(local_status, &m_extents[i].sh_data);
	}
#endif //USE_SHMEM_EXT
}


#ifdef USE_SHMEM_EXT
SRQ_PTR LockManager::REL_PTR(const void* par_item)
{
	const UCHAR* item = static_cast<const UCHAR*>(par_item);
	for (ULONG i = 0; i < m_extents.getCount(); ++i)
	{
		Extent& l = m_extents[i];
		UCHAR* adr = reinterpret_cast<UCHAR*>(l.table);
		if (item >= adr && item < adr + getExtendSize())
		{
			return getStartOffset(i) + (item - adr);
		}
    }

	errno = 0;
    bug(NULL, "Extend not found in REL_PTR()");
    return 0;	// compiler silencer
}


void* LockManager::ABS_PTR(SRQ_PTR item)
{
	ULONG extent = item / getExtendSize();
	if (extent >= m_extents.getCount())
	{
		errno = 0;
		bug(NULL, "Extend not found in ABS_PTR()");
	}
	return reinterpret_cast<UCHAR*>(m_extents[extent].table) + (item % getExtendSize());
}
#endif //USE_SHMEM_EXT


bool LockManager::attach_shared_file(ISC_STATUS* status)
{
	Firebird::PathName name;
	get_shared_file_name(name);

	m_header = (lhb*) ISC_map_file(status, name.c_str(), initialize, this, m_memorySize, &m_shmem);

	if (!m_header)
		return false;

	fb_assert(m_header->lhb_version == LHB_VERSION);
#ifdef USE_SHMEM_EXT
	m_extents[0].sh_data = m_shmem;
#endif
	return true;
}


void LockManager::detach_shared_file(ISC_STATUS* status)
{
	if (m_header)
	{
		ISC_mutex_fini(MUTEX);
		ISC_unmap_file(status, &m_shmem);
		m_header = NULL;
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


bool LockManager::initializeOwner(thread_db* tdbb,
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
	LOCK_TRACE(("LOCK_init (ownerid=%ld)\n", owner_id));

	LocalGuard guard(this);

	// If everything is already initialized, just bump the use count

	if (*owner_handle)
	{
		own* const owner = (own*) SRQ_ABS_PTR(*owner_handle);
		owner->own_count++;
		return true;
	}

	const bool rc = create_owner(tdbb->tdbb_status_vector, owner_id, owner_type, owner_handle);
	LOCK_TRACE(("LOCK_init done (%ld)\n", *owner_handle));
	return rc;
}


void LockManager::shutdownOwner(thread_db* tdbb, SRQ_PTR* owner_offset)
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
	LOCK_TRACE(("LOCK_fini(%ld)\n", *owner_offset));

	LocalGuard guard(this);

	if (!m_header)
		return;

	const SRQ_PTR offset = *owner_offset;
	own* owner = (own*) SRQ_ABS_PTR(offset);
	if (!offset || !owner->own_count)
		return;

	if (--owner->own_count > 0)
		return;

	while (owner->own_ast_count)
	{
		{ // checkout scope
			LocalCheckout checkout(this);
			Database::Checkout dco(tdbb->getDatabase());
			THREAD_SLEEP(10);
		}

		owner = (own*) SRQ_ABS_PTR(offset); // Re-init after a potential remap
	}

	acquire_shmem(offset);
	owner = (own*) SRQ_ABS_PTR(offset);	// Re-init after a potential remap

	purge_owner(offset, owner);

	release_mutex();

	*owner_offset = (SRQ_PTR) 0;
}


SRQ_PTR LockManager::enqueue(thread_db* tdbb,
							 SRQ_PTR prior_request,
							 SRQ_PTR parent_request,
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
	LOCK_TRACE(("LOCK_enq (%ld)\n", parent_request));

	LocalGuard guard(this);

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner_offset || !owner->own_count)
		return 0;

	acquire_shmem(owner_offset);

	ASSERT_ACQUIRED;
	++m_header->lhb_enqs;

#ifdef VALIDATE_LOCK_TABLE
	if ((m_header->lhb_enqs % 50) == 0)
		validate_lhb(m_header);
#endif

	if (prior_request)
		internal_dequeue(prior_request);

	lrq* request = NULL;
	SRQ_PTR parent = 0;
	if (parent_request)
	{
		request = get_request(parent_request);
		parent = request->lrq_lock;
	}

	// Allocate or reuse a lock request block

	ASSERT_ACQUIRED;
	if (SRQ_EMPTY(m_header->lhb_free_requests))
	{
		if (!(request = (lrq*) alloc(sizeof(lrq), tdbb->tdbb_status_vector)))
		{
			release_shmem(owner_offset);
			return 0;
		}
	}
	else
	{
		ASSERT_ACQUIRED;
		request = (lrq*) ((UCHAR*) SRQ_NEXT(m_header->lhb_free_requests) -
						 OFFSET(lrq*, lrq_lbl_requests));
		remove_que(&request->lrq_lbl_requests);
	}

	owner = (own*) SRQ_ABS_PTR(owner_offset);	// Re-init after a potential remap
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

	// See if the lock already exists

	USHORT hash_slot;
	lbl* lock = find_lock(parent, series, value, length, &hash_slot);
	if (lock)
	{
		if (series < LCK_MAX_SERIES) {
			++m_header->lhb_operations[series];
		}
		else {
			++m_header->lhb_operations[0];
		}

		insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
		request->lrq_data = data;
		const SRQ_PTR lock_id = grant_or_que(tdbb, request, lock, lck_wait);
		if (!lock_id)
		{
			ISC_STATUS* status = tdbb->tdbb_status_vector;
			*status++ = isc_arg_gds;
			*status++ = (lck_wait > 0) ? isc_deadlock :
				((lck_wait < 0) ? isc_lock_timeout : isc_lock_conflict);
			*status = isc_arg_end;
		}
		return lock_id;
	}

	// Lock doesn't exist. Allocate lock block and set it up.

	SRQ_PTR request_offset = SRQ_REL_PTR(request);

	if (!(lock = alloc_lock(length, tdbb->tdbb_status_vector)))
	{
		// lock table is exhausted: release request gracefully
		remove_que(&request->lrq_own_requests);
		request->lrq_type = type_null;
		insert_tail(&m_header->lhb_free_requests, &request->lrq_lbl_requests);
		release_shmem(owner_offset);
		return 0;
	}

	lock->lbl_state = type;
	lock->lbl_parent = parent;
	fb_assert(series <= MAX_UCHAR);
	lock->lbl_series = (UCHAR)series;

	// Maintain lock series data queue

	SRQ_INIT(lock->lbl_lhb_data);
	if ( (lock->lbl_data = data) )
		insert_data_que(lock);

	if (series < LCK_MAX_SERIES)
		++m_header->lhb_operations[series];
	else
		++m_header->lhb_operations[0];

	lock->lbl_flags = 0;
	lock->lbl_pending_lrq_count = 0;

	memset(lock->lbl_counts, 0, sizeof(lock->lbl_counts));

	lock->lbl_length = length;
	memcpy(lock->lbl_key, value, length);

	request = (lrq*) SRQ_ABS_PTR(request_offset);

	SRQ_INIT(lock->lbl_requests);
	ASSERT_ACQUIRED;
	insert_tail(&m_header->lhb_hash[hash_slot], &lock->lbl_lhb_hash);
	insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
	request->lrq_lock = SRQ_REL_PTR(lock);
	grant(request, lock);
	const SRQ_PTR lock_id = SRQ_REL_PTR(request);
	release_shmem(request->lrq_owner);

	return lock_id;
}


bool LockManager::convert(thread_db* tdbb,
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
	LOCK_TRACE(("LOCK_convert (%d, %d)\n", type, lck_wait));

	LocalGuard guard(this);

	lrq* request = get_request(request_offset);
	own* owner = (own*) SRQ_ABS_PTR(request->lrq_owner);
	if (!owner->own_count)
		return false;

	acquire_shmem(request->lrq_owner);
	++m_header->lhb_converts;
	request = (lrq*) SRQ_ABS_PTR(request_offset);	// remap
	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	if (lock->lbl_series < LCK_MAX_SERIES)
		++m_header->lhb_operations[lock->lbl_series];
	else
		++m_header->lhb_operations[0];

	return internal_convert(tdbb, request_offset, type, lck_wait, ast_routine, ast_argument);
}


UCHAR LockManager::downgrade(thread_db* tdbb, const SRQ_PTR request_offset)
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
	LOCK_TRACE(("LOCK_downgrade (%ld)\n", request_offset));

	LocalGuard guard(this);

	lrq* request = get_request(request_offset);
	SRQ_PTR owner_offset = request->lrq_owner;
	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return LCK_none;

	acquire_shmem(owner_offset);
	++m_header->lhb_downgrades;

	request = (lrq*) SRQ_ABS_PTR(request_offset);	// Re-init after a potential remap
	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	UCHAR pending_state = LCK_none;

	// Loop thru requests looking for pending conversions
	// and find the highest requested state

	srq* lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		const lrq* pending = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));
		if (pending->lrq_flags & LRQ_pending && pending != request)
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
		release_shmem(owner_offset);
		state = LCK_none;
	}
	else
	{
		internal_convert(tdbb, request_offset, state, LCK_NO_WAIT,
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
	LOCK_TRACE(("LOCK_deq (%ld)\n", request_offset));

	LocalGuard guard(this);

	lrq* request = get_request(request_offset);
	const SRQ_PTR owner_offset = request->lrq_owner;
	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (!owner->own_count)
		return false;

	acquire_shmem(owner_offset);
	++m_header->lhb_deqs;
	request = (lrq*) SRQ_ABS_PTR(request_offset);	// remap
	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	if (lock->lbl_series < LCK_MAX_SERIES)
		++m_header->lhb_operations[lock->lbl_series];
	else
		++m_header->lhb_operations[0];

	internal_dequeue(request_offset);
	release_shmem(owner_offset);
	return true;
}


void LockManager::repost(thread_db* tdbb, lock_ast_t ast, void* arg, SRQ_PTR owner_offset)
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
	lrq* request;

	LOCK_TRACE(("LOCK_re_post(%ld)\n", owner_offset));

	LocalGuard guard(this);

	acquire_shmem(owner_offset);

	// Allocate or reuse a lock request block

	ASSERT_ACQUIRED;
	if (SRQ_EMPTY(m_header->lhb_free_requests))
	{
		if (!(request = (lrq*) alloc(sizeof(lrq), NULL)))
		{
			release_shmem(owner_offset);
			return;
		}
	}
	else
	{
		ASSERT_ACQUIRED;
		request = (lrq*) ((UCHAR*) SRQ_NEXT(m_header->lhb_free_requests) -
						 OFFSET(lrq*, lrq_lbl_requests));
		remove_que(&request->lrq_lbl_requests);
	}

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	request->lrq_type = type_lrq;
	request->lrq_flags = LRQ_repost;
	request->lrq_ast_routine = ast;
	request->lrq_ast_argument = arg;
	request->lrq_requested = LCK_none;
	request->lrq_state = LCK_none;
	request->lrq_owner = owner_offset;
	request->lrq_lock = (SRQ_PTR) 0;
	insert_tail(&owner->own_blocks, &request->lrq_own_blocks);

	DEBUG_DELAY;

	signal_owner(tdbb, (own*) SRQ_ABS_PTR(owner_offset), (SRQ_PTR) NULL);

	release_shmem(owner_offset);
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
	LOCK_TRACE(("cancelWait (%ld)\n", owner_offset));

	if (!owner_offset)
		return false;

	LocalGuard guard(this);

	acquire_shmem(DUMMY_OWNER);

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	if (owner->own_type == type_own)
		post_wakeup(owner);

	release_shmem(DUMMY_OWNER);
	return true;
}


SLONG LockManager::queryData(SRQ_PTR parent_request, const USHORT series, const USHORT aggregate)
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
	if (!parent_request || series >= LCK_MAX_SERIES)
	{
		CHECK(false);
		return 0;
	}

	LocalGuard guard(this);

	// Get root of lock hierarchy

	lrq* parent = get_request(parent_request);
	acquire_shmem(parent->lrq_owner);

	parent = (lrq*) SRQ_ABS_PTR(parent_request);	// remap

	++m_header->lhb_query_data;
	const srq& data_header = m_header->lhb_data[series];
	SLONG data = 0, count = 0;

	// Simply walk the lock series data queue forward for the minimum
	// and backward for the maximum -- it's maintained in sorted order.

	switch (aggregate)
	{
	case LCK_MIN:
	case LCK_CNT:
	case LCK_AVG:
	case LCK_SUM:
	case LCK_ANY:
		for (const srq* lock_srq = (SRQ) SRQ_ABS_PTR(data_header.srq_forward);
			 lock_srq != &data_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
		{
			const lbl* lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock->lbl_series == series);
			if (lock->lbl_parent != parent->lrq_lock)
				continue;

			switch (aggregate)
			{
			case LCK_MIN:
				data = lock->lbl_data;
				break;

			case LCK_ANY:
			case LCK_CNT:
				++count;
				break;

			case LCK_AVG:
				++count;

			case LCK_SUM:
				data += lock->lbl_data;
				break;
			}

			if (aggregate == LCK_MIN || aggregate == LCK_ANY)
				break;
		}

		if (aggregate == LCK_CNT || aggregate == LCK_ANY)
			data = count;
		else if (aggregate == LCK_AVG)
			data = count ? data / count : 0;
		break;

	case LCK_MAX:
		for (const srq* lock_srq = (SRQ) SRQ_ABS_PTR(data_header.srq_backward);
			 lock_srq != &data_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_backward))
		{
			const lbl* lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock->lbl_series == series);
			if (lock->lbl_parent != parent->lrq_lock)
				continue;

			data = lock->lbl_data;
			break;
		}
		break;

	default:
		CHECK(false);
	}

	release_shmem(parent->lrq_owner);
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
	LOCK_TRACE(("LOCK_read_data(%ld)\n", request_offset));

	LocalGuard guard(this);

	lrq* request = get_request(request_offset);
	acquire_shmem(request->lrq_owner);

	++m_header->lhb_read_data;
	request = (lrq*) SRQ_ABS_PTR(request_offset);	// Re-init after a potential remap
	const lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	const SLONG data = lock->lbl_data;
	if (lock->lbl_series < LCK_MAX_SERIES)
		++m_header->lhb_operations[lock->lbl_series];
	else
		++m_header->lhb_operations[0];

	release_shmem(request->lrq_owner);

	return data;
}


SLONG LockManager::readData2(SRQ_PTR parent_request,
							 USHORT series,
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
	LOCK_TRACE(("LOCK_read_data2(%ld)\n", parent_request));

	LocalGuard guard(this);

	acquire_shmem(owner_offset);

	++m_header->lhb_read_data;
	if (series < LCK_MAX_SERIES)
		++m_header->lhb_operations[series];
	else
		++m_header->lhb_operations[0];

	SRQ_PTR parent;
	if (parent_request)
	{
		lrq* request = get_request(parent_request);
		parent = request->lrq_lock;
	}
	else
		parent = 0;

	SLONG data;
	USHORT junk;
	const lbl* lock = find_lock(parent, series, value, length, &junk);
	if (lock)
		data = lock->lbl_data;
	else
		data = 0;

	release_shmem(owner_offset);

	return data;
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
	LOCK_TRACE(("LOCK_write_data (%ld)\n", request_offset));

	LocalGuard guard(this);

	lrq* request = get_request(request_offset);
	acquire_shmem(request->lrq_owner);

	++m_header->lhb_write_data;
	request = (lrq*) SRQ_ABS_PTR(request_offset);	// Re-init after a potential remap
	lbl* lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
	remove_que(&lock->lbl_lhb_data);
	if ( (lock->lbl_data = data) )
		insert_data_que(lock);

	if (lock->lbl_series < LCK_MAX_SERIES)
		++m_header->lhb_operations[lock->lbl_series];
	else
		++m_header->lhb_operations[0];

	release_shmem(request->lrq_owner);

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

	// Measure the impact of the lock table resource as an overall
	// system bottleneck. This will be useful metric for lock
	// improvements and as a limiting factor for SMP. A conditional
	// mutex would probably be more accurate but isn't worth the effort.

	SRQ_PTR prior_active = m_header->lhb_active_owner;

	// Perform a spin wait on the lock table mutex. This should only
	// be used on SMP machines; it doesn't make much sense otherwise.

	SLONG status = FB_FAILURE;
	ULONG spins = 0;
	while (spins++ < m_acquireSpins)
	{
		if ((status = ISC_mutex_lock_cond(MUTEX)) == FB_SUCCESS) {
			break;
		}
	}

	// If the spin wait didn't succeed then wait forever

	if (status != FB_SUCCESS)
	{
		if (ISC_mutex_lock(MUTEX)) {
			bug(NULL, "ISC_mutex_lock failed (acquire_shmem)");
		}
	}

	// Check for shared memory state consistency

	if (SRQ_EMPTY(m_header->lhb_processes))
	{
		fb_assert(owner_offset == CREATE_OWNER);
		owner_offset = DUMMY_OWNER;
	}

	while (SRQ_EMPTY(m_header->lhb_processes))
	{
		if (!m_sharedFileCreated)
		{
			ISC_STATUS_ARRAY local_status;

			// Someone is going to delete shared file? Reattach.
			if (ISC_mutex_unlock(MUTEX) != 0)
			{
				bug(NULL, "ISC_mutex_unlock failed (acquire_shmem)");
			}
			detach_shared_file(local_status);

			THD_yield();

			if (!attach_shared_file(local_status)) {
				bug(local_status, "ISC_map_file failed (reattach shared file)");
			}
			if (ISC_mutex_lock(MUTEX)) {
				bug(NULL, "ISC_mutex_lock failed (acquire_shmem)");
			}
		}
		else
		{
			// complete initialization
			m_sharedFileCreated = false;

			// no sense thinking about statistics now
			prior_active = 0;

			break;
		}
	}
	fb_assert(!m_sharedFileCreated);

	++m_header->lhb_acquires;
	if (m_localBlockage || prior_active > 0) {
		++m_header->lhb_acquire_blocks;
		m_localBlockage = false;
	}

	if (spins)
	{
		++m_header->lhb_acquire_retries;
		if (spins < m_acquireSpins) {
			++m_header->lhb_retry_success;
		}
	}

	prior_active = m_header->lhb_active_owner;
	m_header->lhb_active_owner = owner_offset;

	if (owner_offset > 0)
	{
		own* const owner = (own*) SRQ_ABS_PTR(owner_offset);
		owner->own_thread_id = getThreadId();
	}

#ifdef USE_SHMEM_EXT
	while (m_header->lhb_length > getTotalMapped())
	{
		if (! newExtent())
		{
			bug(NULL, "map of lock file extent failed");
		}
	}
#else //USE_SHMEM_EXT

	if (m_header->lhb_length > m_shmem.sh_mem_length_mapped
#ifdef LOCK_DEBUG_REMAP
		// If we're debugging remaps, force a remap every-so-often.
		|| ((debug_remap_count++ % DEBUG_REMAP_INTERVAL) == 0 && m_processOffset)
#endif
		)
	{
#if (defined HAVE_MMAP || defined WIN_NT)
		const ULONG new_length = m_header->lhb_length;

		Firebird::WriteLockGuard guard(m_remapSync);
		// Post remapping notifications
		remap_local_owners();
		// Remap the shared memory region
		ISC_STATUS_ARRAY status_vector;
		lhb* const header = (lhb*) ISC_remap_file(status_vector, &m_shmem, new_length, false, 
												  MUTEX_PTR);
		if (header)
			m_header = header;
		else
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
		shb* const recover = (shb*) SRQ_ABS_PTR(m_header->lhb_secondary);
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
namespace {
	void initializeExtent(void*, sh_mem*, bool) { }
}

bool LockManager::newExtent()
{
	Firebird::PathName name;
	get_shared_file_name(name, m_extents.getCount());
	ISC_STATUS_ARRAY local_status;

	Extent extent;

	if (!(extent.table = (lhb*) ISC_map_file(local_status, name.c_str(), initializeExtent,
											this, m_memorySize, &extent.sh_data)))
	{
		return false;
	}
	m_extents.add(extent);

	return true;
}
#endif


UCHAR* LockManager::alloc(USHORT size, ISC_STATUS* status_vector)
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
	ULONG block = m_header->lhb_used;

	// Make sure we haven't overflowed the lock table.  If so, bump the size of the table.

	if (m_header->lhb_used + size > m_header->lhb_length)
	{
#ifdef USE_SHMEM_EXT
		// round up so next object starts at beginning of next extent
		block = m_header->lhb_used = m_header->lhb_length;
		if (newExtent())
		{
			m_header->lhb_length += m_memorySize;
		}
		else
#elif (defined HAVE_MMAP || defined WIN_NT)
		Firebird::WriteLockGuard guard(m_remapSync);
		// Post remapping notifications
		remap_local_owners();
		// Remap the shared memory region
		const ULONG new_length = m_shmem.sh_mem_length_mapped + m_memorySize;
		lhb* header = (lhb*) ISC_remap_file(status_vector, &m_shmem, new_length, true, MUTEX_PTR);
		if (header)
		{
			m_header = header;
			ASSERT_ACQUIRED;
			m_header->lhb_length = m_shmem.sh_mem_length_mapped;
		}
		else
#endif
		{
			// Do not do abort in case if there is not enough room -- just
			// return an error

			if (status_vector)
			{
				*status_vector++ = isc_arg_gds;
				*status_vector++ = isc_random;
				*status_vector++ = isc_arg_string;
				*status_vector++ = (ISC_STATUS) "lock manager out of room";
				*status_vector++ = isc_arg_end;
			}

			return NULL;
		}
	}

	m_header->lhb_used += size;

#ifdef DEV_BUILD
	// This version of alloc() doesn't initialize memory.  To shake out
	// any bugs, in DEV_BUILD we initialize it to a "funny" pattern.
	memset((void*)SRQ_ABS_PTR(block), 0xFD, size);
#endif

	return (UCHAR*) SRQ_ABS_PTR(block);
}


lbl* LockManager::alloc_lock(USHORT length, ISC_STATUS* status_vector)
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
	SRQ_LOOP(m_header->lhb_free_locks, lock_srq)
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

	lbl* lock = (lbl*) alloc(sizeof(lbl) + length, status_vector);
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


void LockManager::blocking_action(thread_db* tdbb,
								  SRQ_PTR blocking_owner_offset,
								  SRQ_PTR blocked_owner_offset)
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

	if (!blocked_owner_offset)
		blocked_owner_offset = blocking_owner_offset;

	while (owner->own_count)
	{
		srq* const lock_srq = SRQ_NEXT(owner->own_blocks);
		if (lock_srq == &owner->own_blocks)
		{
			// We've processed the own_blocks queue, reset the "we've been
			// signaled" flag and start winding out of here
			owner->own_flags &= ~OWN_signaled;
			break;
		}
		lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_blocks));
		lock_ast_t routine = request->lrq_ast_routine;
		void* arg = request->lrq_ast_argument;
		remove_que(&request->lrq_own_blocks);
		if (request->lrq_flags & LRQ_blocking)
		{
			request->lrq_flags &= ~LRQ_blocking;
			request->lrq_flags |= LRQ_blocking_seen;
			++m_header->lhb_blocks;
			post_history(his_post_ast, blocking_owner_offset,
						 request->lrq_lock, SRQ_REL_PTR(request), true);
		}
		else if (request->lrq_flags & LRQ_repost)
		{
			request->lrq_type = type_null;
			insert_tail(&m_header->lhb_free_requests, &request->lrq_lbl_requests);
		}

		if (routine)
		{
			owner->own_ast_count++;
			release_shmem(blocked_owner_offset);
			
			{ // checkout scope
				LocalCheckout checkout(this);

				if (tdbb)
				{
					Database::Checkout dcoHolder(tdbb->getDatabase());
					(*routine)(arg);
				}
				else
				{
					(*routine)(arg);
				}
			}

			acquire_shmem(blocked_owner_offset);
			owner = (own*) SRQ_ABS_PTR(blocking_owner_offset);
			owner->own_ast_count--;
		}
	}
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
 * when AST can't lock appropriate database mutex and therefore does not return.
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
				LocalGuard guard(this);

				// See if the main thread has requested us to go away
				if (!m_processOffset || m_process->prc_process_id != PID)
				{
					if (atStartup)
					{
						m_startupSemaphore.release();
					}
					break;
				}

				value = ISC_event_clear(&m_process->prc_blocking);

				DEBUG_DELAY;

				Firebird::HalfStaticArray<SRQ_PTR, 4> blocking_owners;

				acquire_shmem(DUMMY_OWNER);
				const prc* const process = (prc*) SRQ_ABS_PTR(m_processOffset);

				srq* lock_srq;
				SRQ_LOOP(process->prc_owners, lock_srq)
				{
					own* owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_prc_owners));
					blocking_owners.add(SRQ_REL_PTR(owner));
				}

				release_mutex();

				while (blocking_owners.getCount() && m_processOffset)
				{
					const SRQ_PTR owner_offset = blocking_owners.pop();
					acquire_shmem(owner_offset);
					blocking_action(NULL, owner_offset, (SRQ_PTR) NULL);
					release_shmem(owner_offset);
				}

				if (atStartup)
				{
					atStartup = false;
					m_startupSemaphore.release();
				}
			}

			(void) ISC_event_wait(&m_process->prc_blocking, value, 0);
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
	LOCK_header_copy = *m_header;

	bug(NULL, buffer);	// Never returns
}


void LockManager::bug(ISC_STATUS* status_vector, const TEXT* string)
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
	gds__log(s);
	fprintf(stderr, "%s\n", s);

#if !(defined WIN_NT)
	// The strerror() function returns the appropriate description string,
	// or an unknown error message if the error code is unknown.
	fprintf(stderr, "--%s\n", strerror(errno));
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
			fwrite(m_header, 1, m_header->lhb_used, fd);
			fclose(fd);
		}

		// If the current mutex acquirer is in the same process, release the mutex

		if (m_header && (m_header->lhb_active_owner > 0))
		{
			const own* const owner = (own*) SRQ_ABS_PTR(m_header->lhb_active_owner);
			const prc* const process = (prc*) SRQ_ABS_PTR(owner->own_process);
			if (process->prc_process_id == PID)
				release_shmem(m_header->lhb_active_owner);
		}

		if (status_vector)
		{
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_lockmanerr;
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_random;
			*status_vector++ = isc_arg_string;
			*status_vector++ = (ISC_STATUS) string;
			*status_vector++ = isc_arg_end;
			return;
		}
	}

#ifdef DEV_BUILD
   /* In the worst case this code makes it possible to attach live process
	* and see shared memory data.
	fprintf(stderr, "Attach to pid=%d\n", getpid());
	sleep(120);
	*/
	// Make a core drop - we want to LOOK at this failure!
	abort();
#endif

	exit(FINI_ERROR);
}


bool LockManager::create_owner(ISC_STATUS* status_vector,
							   LOCK_OWNER_T owner_id,
							   UCHAR owner_type,
							   SRQ_PTR* owner_handle)
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
	if (m_header->lhb_version != LHB_VERSION)
	{
		TEXT bug_buffer[BUFFER_TINY];
		sprintf(bug_buffer, "inconsistent lock table version number; found %d, expected %d",
				m_header->lhb_version, LHB_VERSION);
		bug(status_vector, bug_buffer);
		return false;
	}

	acquire_shmem(CREATE_OWNER);	// acquiring owner is being created

	// Allocate a process block, if required

	if (!m_processOffset)
	{
		if (!create_process(status_vector))
		{
			release_mutex();
			return false;
		}
	}

	// Look for a previous instance of owner.  If we find one, get rid of it.

	srq* lock_srq;
	SRQ_LOOP(m_header->lhb_owners, lock_srq)
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
	if (SRQ_EMPTY(m_header->lhb_free_owners))
	{
		if (!(owner = (own*) alloc(sizeof(own), status_vector)))
		{
			release_mutex();
			return false;
		}
	}
	else
	{
		owner = (own*) ((UCHAR*) SRQ_NEXT(m_header->lhb_free_owners) - OFFSET(own*, own_lhb_owners));
		remove_que(&owner->own_lhb_owners);
	}

	if (!init_owner_block(status_vector, owner, owner_type, owner_id))
	{
		release_mutex();
		return false;
	}

	insert_tail(&m_header->lhb_owners, &owner->own_lhb_owners);

	prc* const process = (prc*) SRQ_ABS_PTR(owner->own_process);
	insert_tail(&process->prc_owners, &owner->own_prc_owners);

	probe_processes();

	*owner_handle = SRQ_REL_PTR(owner);
	m_header->lhb_active_owner = *owner_handle;

#ifdef VALIDATE_LOCK_TABLE
	validate_lhb(m_header);
#endif

	release_shmem(*owner_handle);

	return true;
}


bool LockManager::create_process(ISC_STATUS* status_vector)
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
	SRQ_LOOP(m_header->lhb_processes, lock_srq)
	{
		prc* process = (prc*) ((UCHAR*) lock_srq - OFFSET(prc*, prc_lhb_processes));
		if (process->prc_process_id == PID)
		{
			purge_process(process);
			break;
		}
	}

	prc* process = NULL;
	if (SRQ_EMPTY(m_header->lhb_free_processes))
	{
		if (!(process = (prc*) alloc(sizeof(prc), status_vector)))
		{
			return false;
		}
	}
	else
	{
		process = (prc*) ((UCHAR*) SRQ_NEXT(m_header->lhb_free_processes) -
					   OFFSET(prc*, prc_lhb_processes));
		remove_que(&process->prc_lhb_processes);
	}

	process->prc_type = type_lpr;
	process->prc_process_id = PID;
	SRQ_INIT(process->prc_owners);
	SRQ_INIT(process->prc_lhb_processes);
	process->prc_flags = 0;

	insert_tail(&m_header->lhb_processes, &process->prc_lhb_processes);

	if (ISC_event_init(&process->prc_blocking) != FB_SUCCESS)
	{
		Firebird::Arg::Gds result(isc_lockmanerr);
		result.copyTo(status_vector);
		return false;
	}

	m_processOffset = SRQ_REL_PTR(process);

#if defined HAVE_MMAP || defined WIN_NT
	m_process = (prc*) ISC_map_object(status_vector, &m_shmem, m_processOffset, sizeof(prc));
#else
	m_process = process;
#endif

	if (!m_process)
		return false;

#ifdef USE_BLOCKING_THREAD
	try
	{
		ThreadStart::start(blocking_action_thread, this, THREAD_high, 0);
	}
	catch (const Firebird::Exception& ex)
	{
		ISC_STATUS_ARRAY vector;
		ex.stuff_exception(vector);

		Firebird::Arg::Gds result(isc_lockmanerr);
		result.append(Firebird::Arg::StatusVector(vector));
		result.copyTo(status_vector);

		return false;
	}
#endif

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
	SRQ_LOOP(m_header->lhb_owners, lock_srq)
	{
		own* owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_lhb_owners));
		SRQ_PTR pending_offset = owner->own_pending_request;
		if (!pending_offset)
			continue;
		lrq* pending = (lrq*) SRQ_ABS_PTR(pending_offset);
		pending->lrq_flags &= ~(LRQ_deadlock | LRQ_scanned);
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
	++m_header->lhb_scans;
	post_history(his_scan, request->lrq_owner, request->lrq_lock, SRQ_REL_PTR(request), true);
	deadlock_clear();

#ifdef VALIDATE_LOCK_TABLE
	validate_lhb(m_header);
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

		if (!lockOrdering() || conversion)
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

			if (compatibility[request->lrq_requested][MAX(block->lrq_state, block->lrq_requested)])
			{
				continue;
			}
		}

		// Don't pursue lock owners that are not blocked themselves
		// (they can't cause a deadlock).

		own* const owner = (own*) SRQ_ABS_PTR(block->lrq_owner);

		// hvlad: don't pursue lock owners that wait with timeout as such
		// circle in wait-for graph will be broken automatically when permitted
		// timeout expires

		if (owner->own_flags & OWN_timeout) {
			continue;
		}

		// Don't pursue lock owners that still have to finish processing their AST.
		// If the blocking queue is not empty, then the owner still has some
		// AST's to process (or lock reposts).
		// hvlad: also lock maybe just granted to owner and blocked owners have no
		// time to send blocking AST
		// Remember this fact because they still might be part of a deadlock.

		if (owner->own_flags & (OWN_signaled | OWN_wakeup) || !SRQ_EMPTY((owner->own_blocks)) ||
			block->lrq_flags & LRQ_just_granted)
		{
			*maybe_deadlock = true;
			continue;
		}

		// YYY: Note: can the below code be moved to the
		// start of this block?  Before the OWN_signaled check?

		// Get pointer to the waiting request whose owner also owns a lock
		// that blocks the input request

		const SRQ_PTR pending_offset = owner->own_pending_request;
		if (!pending_offset)
			continue;

		lrq* target = (lrq*) SRQ_ABS_PTR(pending_offset);

		// If this waiting request is not pending anymore, then things are changing
		// and this request cannot be part of a deadlock

		if (!(target->lrq_flags & LRQ_pending))
			continue;

		// Check who is blocking the request whose owner is blocking the input request

		if (target = deadlock_walk(target, maybe_deadlock))
		{
#ifdef DEBUG_TRACE_DEADLOCKS
			const own* owner2 = (own*) SRQ_ABS_PTR(request->lrq_owner);
			const prc* proc = (prc*) SRQ_ABS_PTR(owner2->own_process);
			gds__log("deadlock chain: OWNER BLOCK %6"SLONGFORMAT"\tProcess id: %6d\tFlags: 0x%02X ",
				request->lrq_owner, proc->prc_process_id, owner2->own_flags);
#endif
			return target;
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


lbl* LockManager::find_lock(SRQ_PTR parent,
							USHORT series,
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

	const USHORT hash_slot = *slot = (USHORT) (hash_value % m_header->lhb_hash_slots);
	ASSERT_ACQUIRED;
	srq* const hash_header = &m_header->lhb_hash[hash_slot];

	for (srq* lock_srq = (SRQ) SRQ_ABS_PTR(hash_header->srq_forward);
		 lock_srq != hash_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
	{
		lbl* lock = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_hash));
		if (lock->lbl_series != series || lock->lbl_length != length || lock->lbl_parent != parent)
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
		request->lrq_flags &= ~LRQ_pending;
		lock->lbl_pending_lrq_count--;
	}
	post_wakeup((own*) SRQ_ABS_PTR(request->lrq_owner));
}


SRQ_PTR LockManager::grant_or_que(thread_db* tdbb, lrq* request, lbl* lock, SSHORT lck_wait)
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
	const SRQ_PTR request_offset = SRQ_REL_PTR(request);
	request->lrq_lock = SRQ_REL_PTR(lock);

	// Compatible requests are easy to satify.  Just post the request to the lock,
	// update the lock state, release the data structure, and we're done.

	if (compatibility[request->lrq_requested][lock->lbl_state])
	{
		if (!lockOrdering() || request->lrq_requested == LCK_null || lock->lbl_pending_lrq_count == 0)
		{
			grant(request, lock);
			post_pending(lock);
			release_shmem(request->lrq_owner);
			return request_offset;
		}
	}

	// The request isn't compatible with the current state of the lock.
	// If we haven't be asked to wait for the lock, return now.

	if (lck_wait)
	{
		wait_for_request(tdbb, request, lck_wait);

		// For performance reasons, we're going to look at the
		// request's status without re-acquiring the lock table.
		// This is safe as no-one can take away the request, once
		// granted, and we're doing a read-only access
		request = (lrq*) SRQ_ABS_PTR(request_offset);

		// Request HAS been resolved
		CHECK(!(request->lrq_flags & LRQ_pending));

		if (!(request->lrq_flags & LRQ_rejected))
			return request_offset;
		acquire_shmem(request->lrq_owner);
	}

	request = (lrq*) SRQ_ABS_PTR(request_offset);
	post_history(his_deny, request->lrq_owner, request->lrq_lock, SRQ_REL_PTR(request), true);
	ASSERT_ACQUIRED;
	++m_header->lhb_denies;
	if (lck_wait < 0)
		++m_header->lhb_timeouts;
	const SRQ_PTR owner_offset = request->lrq_owner;
	release_request(request);
	release_shmem(owner_offset);

	return (SRQ_PTR)0;
}


bool LockManager::init_owner_block(ISC_STATUS* status_vector, own* owner, UCHAR owner_type, LOCK_OWNER_T owner_id)
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
	owner->own_pending_request = 0;
	owner->own_acquire_time = 0;
	owner->own_ast_count = 0;

	if (ISC_event_init(&owner->own_wakeup) != FB_SUCCESS)
	{
		Firebird::Arg::Gds result(isc_lockmanerr);
		result.copyTo(status_vector);
		return false;
	}

	return true;
}


void LockManager::initialize(sh_mem* shmem_data, bool initializeMemory)
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

#ifdef WIN_NT
	if (ISC_mutex_init(MUTEX, shmem_data->sh_mem_name)) {
		bug(NULL, "mutex init failed");
	}
#endif

	m_header = (lhb*) shmem_data->sh_mem_address;
	m_sharedFileCreated = initializeMemory;

#ifdef USE_SHMEM_EXT
	if (m_extents.getCount() == 0)
	{
		Extent zero;
		zero.table = m_header;
		m_extents.push(zero);
	}
#endif

	if (!initializeMemory) {
#ifndef WIN_NT
		if (ISC_map_mutex(shmem_data, &m_header->lhb_mutex, MUTEX_PTR)) {
			bug(NULL, "mutex map failed");
		}
#endif
		return;
	}

	memset(m_header, 0, sizeof(lhb));
	m_header->lhb_type = type_lhb;
	m_header->lhb_version = LHB_VERSION;

	// Mark ourselves as active owner to prevent fb_assert() checks
	m_header->lhb_active_owner = DUMMY_OWNER;	// In init of lock system

	SRQ_INIT(m_header->lhb_processes);
	SRQ_INIT(m_header->lhb_owners);
	SRQ_INIT(m_header->lhb_free_processes);
	SRQ_INIT(m_header->lhb_free_owners);
	SRQ_INIT(m_header->lhb_free_locks);
	SRQ_INIT(m_header->lhb_free_requests);

#ifndef WIN_NT
	if (ISC_mutex_init(shmem_data, &m_header->lhb_mutex, MUTEX_PTR)) {
		bug(NULL, "mutex init failed");
	}
#endif

	int hash_slots = Config::getLockHashSlots();
	if (hash_slots < HASH_MIN_SLOTS)
		hash_slots = HASH_MIN_SLOTS;
	if (hash_slots > HASH_MAX_SLOTS)
		hash_slots = HASH_MAX_SLOTS;

	m_header->lhb_hash_slots = (USHORT) hash_slots;
	m_header->lhb_scan_interval = Config::getDeadlockTimeout();
	m_header->lhb_acquire_spins = m_acquireSpins;

	// Initialize lock series data queues and lock hash chains

	USHORT i;
	SRQ lock_srq;
	for (i = 0, lock_srq = m_header->lhb_data; i < LCK_MAX_SERIES; i++, lock_srq++)
	{
		SRQ_INIT((*lock_srq));
	}
	for (i = 0, lock_srq = m_header->lhb_hash; i < m_header->lhb_hash_slots; i++, lock_srq++)
	{
		SRQ_INIT((*lock_srq));
	}

	// Set lock_ordering flag for the first time

	if (Config::getLockGrantOrder())
		m_header->lhb_flags |= LHB_lock_ordering;

	const ULONG length = sizeof(lhb) + (m_header->lhb_hash_slots * sizeof(m_header->lhb_hash[0]));
	m_header->lhb_length = shmem_data->sh_mem_length_mapped;
	m_header->lhb_used = FB_ALIGN(length, FB_ALIGNMENT);

	shb* secondary_header = (shb*) alloc(sizeof(shb), NULL);
	if (!secondary_header)
	{
		gds__log("Fatal lock manager error: lock manager out of room");
		exit(STARTUP_ERROR);
	}

	m_header->lhb_secondary = SRQ_REL_PTR(secondary_header);
	secondary_header->shb_type = type_shb;
	secondary_header->shb_remove_node = 0;
	secondary_header->shb_insert_que = 0;
	secondary_header->shb_insert_prior = 0;

	// Allocate a sufficiency of history blocks

	his* history = NULL;
	for (USHORT j = 0; j < 2; j++)
	{
		SRQ_PTR* prior = (j == 0) ? &m_header->lhb_history : &secondary_header->shb_history;

		for (i = 0; i < HISTORY_BLOCKS; i++)
		{
			if (!(history = (his*) alloc(sizeof(his), NULL)))
			{
				gds__log("Fatal lock manager error: lock manager out of room");
				exit(STARTUP_ERROR);
			}
			*prior = SRQ_REL_PTR(history);
			history->his_type = type_his;
			history->his_operation = 0;
			prior = &history->his_next;
		}

		history->his_next = (j == 0) ? m_header->lhb_history : secondary_header->shb_history;
	}

	// Done initializing, unmark owner information
	m_header->lhb_active_owner = 0;
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

	if (lock->lbl_series < LCK_MAX_SERIES && lock->lbl_parent && lock->lbl_data)
	{
		SRQ data_header = &m_header->lhb_data[lock->lbl_series];

		SRQ lock_srq;
		for (lock_srq = (SRQ) SRQ_ABS_PTR(data_header->srq_forward);
			 lock_srq != data_header; lock_srq = (SRQ) SRQ_ABS_PTR(lock_srq->srq_forward))
		{
			const lbl* lock2 = (lbl*) ((UCHAR*) lock_srq - OFFSET(lbl*, lbl_lhb_data));
			CHECK(lock2->lbl_series == lock->lbl_series);
			if (lock2->lbl_parent != lock->lbl_parent)
				continue;

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
	shb* const recover = (shb*) SRQ_ABS_PTR(m_header->lhb_secondary);
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


bool LockManager::internal_convert(thread_db* tdbb,
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
	SRQ_PTR owner_offset = request->lrq_owner;
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
		release_shmem(owner_offset);
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

		if (wait_for_request(tdbb, request, lck_wait))
			return false;

		request = (lrq*) SRQ_ABS_PTR(request_offset);
		if (!(request->lrq_flags & LRQ_rejected))
		{
			if (new_ast)
			{
				acquire_shmem(owner_offset);
				request = (lrq*) SRQ_ABS_PTR(request_offset);
				request->lrq_ast_routine = ast_routine;
				request->lrq_ast_argument = ast_argument;
				release_shmem(owner_offset);
			}
			return true;
		}
		acquire_shmem(owner_offset);
		request = get_request(request_offset);
		lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);
		post_pending(lock);
	}

	request = (lrq*) SRQ_ABS_PTR(request_offset);
	request->lrq_requested = request->lrq_state;
	ASSERT_ACQUIRED;
	++m_header->lhb_denies;
	if (lck_wait < 0)
		++m_header->lhb_timeouts;

	release_shmem(owner_offset);

	ISC_STATUS* status = tdbb->tdbb_status_vector;
	*status++ = isc_arg_gds;
	*status++ = (lck_wait > 0) ? isc_deadlock :
		((lck_wait < 0) ? isc_lock_timeout : isc_lock_conflict);
	*status = isc_arg_end;

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


void LockManager::post_blockage(thread_db* tdbb, lrq* request, lbl* lock)
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
	own* owner = (own*) SRQ_ABS_PTR(owner_offset);

	ASSERT_ACQUIRED;
	CHECK(owner->own_pending_request == SRQ_REL_PTR(request));
	CHECK(request->lrq_flags & LRQ_pending);

	Firebird::HalfStaticArray<SRQ_PTR, 16> blocking_owners;

	SRQ lock_srq;
	SRQ_LOOP(lock->lbl_requests, lock_srq)
	{
		lrq* const block = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_lbl_requests));

		// Figure out if this lock request is blocking our own lock request.
		// Of course, our own request cannot block ourselves.  Compatible
		// requests don't block us, and if there is no AST routine for the
		// request the block doesn't matter as we can't notify anyone.
		// If the owner has marked the request with LRQ_blocking_seen
		// then the blocking AST has been delivered and the owner promises
		// to release the lock as soon as possible (so don't bug the owner).

		if (block == request ||
			compatibility[request->lrq_requested][block->lrq_state] ||
			!block->lrq_ast_routine ||
			(block->lrq_flags & LRQ_blocking_seen))
		{
			continue;
		}

		own* const blocking_owner = (own*) SRQ_ABS_PTR(block->lrq_owner);

		// Add the blocking request to the list of blocks if it's not
		// there already (LRQ_blocking)

		if (!(block->lrq_flags & LRQ_blocking))
		{
			insert_tail(&blocking_owner->own_blocks, &block->lrq_own_blocks);
			block->lrq_flags |= LRQ_blocking;
			block->lrq_flags &= ~(LRQ_blocking_seen | LRQ_just_granted);
		}

		if (blocking_owner != owner)
			blocking_owners.add(block->lrq_owner);

		if (block->lrq_state == LCK_EX)
			break;
	}

	Firebird::HalfStaticArray<SRQ_PTR, 16> dead_processes;

	while (blocking_owners.getCount())
	{
		own* const blocking_owner = (own*) SRQ_ABS_PTR(blocking_owners.pop());
		if (blocking_owner->own_count && !signal_owner(tdbb, blocking_owner, owner_offset))
		{
			dead_processes.add(blocking_owner->own_process);
		}
	}

	while (dead_processes.getCount())
	{
		prc* const process = (prc*) SRQ_ABS_PTR(dead_processes.pop());
		if (process->prc_process_id)
			purge_process(process);
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
		history = (his*) SRQ_ABS_PTR(m_header->lhb_history);
		ASSERT_ACQUIRED;
		m_header->lhb_history = history->his_next;
	}
	else
	{
		ASSERT_ACQUIRED;
		shb* recover = (shb*) SRQ_ABS_PTR(m_header->lhb_secondary);
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

	SRQ lock_srq;
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
				if (lockOrdering())
				{
					CHECK(lock->lbl_pending_lrq_count >= pending_counter);
					break;
				}
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
			if (lockOrdering())
			{
				CHECK(lock->lbl_pending_lrq_count >= pending_counter);
				break;
			}
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

	if (owner->own_flags & OWN_waiting)
	{
		++m_header->lhb_wakeups;
		owner->own_flags |= OWN_wakeup;
		(void) ISC_event_post(&owner->own_wakeup);
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

	Firebird::HalfStaticArray<prc*, 16> dead_processes;

	SRQ lock_srq;
	SRQ_LOOP(m_header->lhb_processes, lock_srq)
	{
		prc* const process = (prc*) ((UCHAR*) lock_srq - OFFSET(prc*, prc_lhb_processes));
		if (process->prc_process_id != PID && !ISC_check_process_existence(process->prc_process_id))
		{
			dead_processes.add(process);
		}
	}

	const bool purged = (dead_processes.getCount() > 0);

	while (dead_processes.getCount())
	{
		prc* const process = dead_processes.pop();
		if (process->prc_process_id)
			purge_process(process);
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
		insert_tail(&m_header->lhb_free_requests, &request->lrq_lbl_requests);
	}

	// Release owner block

	remove_que(&owner->own_prc_owners);

	remove_que(&owner->own_lhb_owners);
	insert_tail(&m_header->lhb_free_owners, &owner->own_lhb_owners);

	owner->own_owner_type = 0;
	owner->own_owner_id = 0;
	owner->own_process = 0;
	owner->own_flags = 0;

	ISC_event_fini(&owner->own_wakeup);
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
	insert_tail(&m_header->lhb_free_processes, &process->prc_lhb_processes);

	process->prc_process_id = 0;
	process->prc_flags = 0;

	ISC_event_fini(&process->prc_blocking);
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
	if (! m_processOffset)
		return;

	fb_assert(m_process);

	prc* const process = (prc*) SRQ_ABS_PTR(m_processOffset);
	srq* lock_srq;
	SRQ_LOOP(process->prc_owners, lock_srq)
	{
		own* owner = (own*) ((UCHAR*) lock_srq - OFFSET(own*, own_prc_owners));
		if (owner->own_flags & OWN_waiting)
		{
			if (ISC_event_post(&owner->own_wakeup) != FB_SUCCESS)
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
	shb* recover = (shb*) SRQ_ABS_PTR(m_header->lhb_secondary);
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

	if (owner_offset && m_header->lhb_active_owner != owner_offset)
		bug(NULL, "release when not owner");

#ifdef VALIDATE_LOCK_TABLE
	// Validate the lock table occasionally (every 500 releases)
	if ((m_header->lhb_acquires % (HISTORY_BLOCKS / 2)) == 0)
		validate_lhb(m_header);
#endif

	release_mutex();
}


void LockManager::release_mutex()
{
/**************************************
 *
 *	r e l e a s e _ m u t e x
 *
 **************************************
 *
 * Functional description
 *	Release the mapped lock file.  Advance the event count to wake up
 *	anyone waiting for it.  If there appear to be blocking items
 *	posted.
 *
 **************************************/

	DEBUG_DELAY;

	if (!m_header->lhb_active_owner)
		bug(NULL, "release when not active");

	m_header->lhb_active_owner = 0;

	if (ISC_mutex_unlock(MUTEX))
		bug(NULL, "semop failed (release_shmem)");

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
	insert_tail(&m_header->lhb_free_requests, &request->lrq_lbl_requests);
	lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

	// If the request is marked as blocking, clean it up

	if (request->lrq_flags & LRQ_blocking)
	{
		remove_que(&request->lrq_own_blocks);
		request->lrq_flags &= ~LRQ_blocking;
	}

	request->lrq_flags &= ~(LRQ_blocking_seen | LRQ_just_granted);

	// Update counts if we are cleaning up something we're waiting on!
	// This should only happen if we are purging out an owner that died.

	if (request->lrq_flags & LRQ_pending)
	{
		request->lrq_flags &= ~LRQ_pending;
		lock->lbl_pending_lrq_count--;
	}

	// If there are no outstanding requests, release the lock

	if (SRQ_EMPTY(lock->lbl_requests))
	{
		CHECK(lock->lbl_pending_lrq_count == 0);
#ifdef VALIDATE_LOCK_TABLE
		if (m_header->lhb_active_owner > 0)
			validate_parent(m_header, request->lrq_lock);
#endif
		remove_que(&lock->lbl_lhb_hash);
		remove_que(&lock->lbl_lhb_data);
		lock->lbl_type = type_null;

		insert_tail(&m_header->lhb_free_locks, &lock->lbl_lhb_hash);
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


bool LockManager::signal_owner(thread_db* tdbb, own* blocking_owner, SRQ_PTR blocked_owner_offset)
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

	if (blocking_owner->own_flags & OWN_signaled)
	{
#ifdef DEBUG_LM
		const prc* const process = (prc*) SRQ_ABS_PTR(blocking_owner->own_process);
		DEBUG_MSG(1, ("signal_owner (%ld) - skipped OWN_signaled\n", process->prc_process_id));
#endif
		return true;
	}

	blocking_owner->own_flags |= OWN_signaled;
	DEBUG_DELAY;

	prc* const process = (prc*) SRQ_ABS_PTR(blocking_owner->own_process);

	// Deliver signal either locally or remotely

	if (process->prc_process_id == PID)
	{
		DEBUG_DELAY;
		blocking_action(tdbb, SRQ_REL_PTR(blocking_owner), blocked_owner_offset);
		DEBUG_DELAY;
		return true;
	}

	DEBUG_DELAY;

	if (ISC_event_post(&process->prc_blocking) == FB_SUCCESS)
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


void LockManager::validate_parent(const lhb* alhb, const SRQ_PTR isSomeoneParent)
{
/**************************************
 *
 *	v a l i d a t e _ p a r e n t
 *
 **************************************
 *
 * Functional description
 *	Validate lock under release not to be someone parent.
 *
 **************************************/
	if (alhb->lhb_active_owner == 0)
		return;

	const own* const owner = (own*) SRQ_ABS_PTR(alhb->lhb_active_owner);

	const srq* lock_srq;
	SRQ_LOOP(owner->own_requests, lock_srq)
	{
		const lrq* const request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_requests));

		if (!(request->lrq_flags & LRQ_repost))
		{
			if (request->lrq_lock != isSomeoneParent)
			{
				const lbl* const lock = (lbl*) SRQ_ABS_PTR(request->lrq_lock);

				if (lock->lbl_parent == isSomeoneParent)
				{
					bug_assert("deleting someone's parent", __LINE__);
				}
			}
		}
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
	CHECK(alhb->lhb_type == type_lhb);
	CHECK(alhb->lhb_version == LHB_VERSION);

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

	ULONG found = 0;
	ULONG found_pending = 0;
	UCHAR highest_request = LCK_none;
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

		if (request->lrq_requested > highest_request)
			highest_request = request->lrq_requested;

		// If the request is pending, then it must be incompatible with current
		// state of the lock - OR lock_ordering is enabled and there is at
		// least one pending request in the queue (before this request
		// but not including it).

		if (request->lrq_flags & LRQ_pending)
		{
			CHECK(!compatibility[request->lrq_requested][lock->lbl_state] ||
				  (lockOrdering() && found_pending));

			// The above condition would probably be more clear if we
			// wrote it as the following:
			//
			// CHECK (!compatibility[request->lrq_requested][lock->lbl_state] ||
			// (lockOrdering() && found_pending &&
			// compatibility[request->lrq_requested][lock->lbl_state]));
			//
			// but that would be redundant

			found_pending++;
		}

		// If the request is NOT pending, then it must be rejected or
		// compatible with the current state of the lock

		if (!(request->lrq_flags & LRQ_pending))
		{
			CHECK((request->lrq_flags & LRQ_rejected) ||
				  (request->lrq_requested == lock->lbl_state) ||
				  compatibility[request->lrq_requested][lock->lbl_state]);

		}

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

	if (lock->lbl_parent && (freed == EXPECT_inuse))
		validate_lock(lock->lbl_parent, EXPECT_inuse, (SRQ_PTR) 0);
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

	// Note that owner->own_pending_request can be reset without the lock
	// table being acquired - eg: by another process.  That being the case,
	// we need to stash away a copy of it for validation.

	const SRQ_PTR owner_own_pending_request = owner->own_pending_request;

	CHECK(owner->own_type == type_own);
	if (freed == EXPECT_freed)
		CHECK(owner->own_owner_type == 0);
	else {
		CHECK(owner->own_owner_type <= 2);
	}

	CHECK(owner->own_acquire_time <= m_header->lhb_acquires);

	// Check that no invalid flag bit is set
	CHECK(!
		  (owner->own_flags &
		  		~(OWN_blocking | OWN_scanned | OWN_waiting | OWN_wakeup | OWN_signaled | OWN_timeout)));

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

	// If there is a pending request, make sure it is valid, that it
	// exists in the queue for the lock.

	if (owner_own_pending_request && (freed == EXPECT_inuse))
	{
		// Make sure pending request is valid, and we own it
		const lrq* const request3 = (lrq*) SRQ_ABS_PTR(owner_own_pending_request);
		validate_request(SRQ_REL_PTR(request3), EXPECT_inuse, RECURSE_not);
		CHECK(request3->lrq_owner == own_ptr);

		// Make sure the lock the request is for is valid
		const lbl* const lock = (lbl*) SRQ_ABS_PTR(request3->lrq_lock);
		validate_lock(SRQ_REL_PTR(lock), EXPECT_inuse, (SRQ_PTR) 0);

		// Make sure the pending request is on the list of requests for the lock

		bool found_pending = false;
		const srq* que_of_lbl_requests;
		SRQ_LOOP(lock->lbl_requests, que_of_lbl_requests)
		{
			const lrq* const pending =
				(lrq*) ((UCHAR*) que_of_lbl_requests - OFFSET(lrq*, lrq_lbl_requests));
			if (SRQ_REL_PTR(pending) == owner_own_pending_request)
			{
				found_pending = true;
				break;
			}
		}

		// pending request must exist in the lock's request queue
		CHECK(found_pending);

		// Either the pending request is the same as what we stashed away, or it's
		// been cleared by another process without the lock table acquired
		CHECK((owner_own_pending_request == owner->own_pending_request) ||
			  !owner->own_pending_request);
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
		   		~(LRQ_blocking | LRQ_pending | LRQ_converting |
					 LRQ_rejected | LRQ_timed_out | LRQ_deadlock |
					 LRQ_repost | LRQ_scanned | LRQ_blocking_seen | LRQ_just_granted)));

	// LRQ_converting & LRQ_timed_out are defined, but never actually used
	CHECK(!(request->lrq_flags & (LRQ_converting | LRQ_timed_out)));

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


USHORT LockManager::wait_for_request(thread_db* tdbb, lrq* request, SSHORT lck_wait)
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

	++m_header->lhb_waits;
	const SLONG scan_interval = m_header->lhb_scan_interval;

	// lrq_count will be off if we wait for a pending request
	CHECK(!(request->lrq_flags & LRQ_pending));

	request->lrq_flags &= ~LRQ_rejected;
	request->lrq_flags |= LRQ_pending;
	const SRQ_PTR owner_offset = request->lrq_owner;
	const SRQ_PTR lock_offset = request->lrq_lock;
	lbl* lock = (lbl*) SRQ_ABS_PTR(lock_offset);
	lock->lbl_pending_lrq_count++;
	if (lockOrdering())
	{
		if (!request->lrq_state)
		{
			// If ordering is in effect, and this is a conversion of
			// an existing lock in LCK_none state - put the lock to the
			// end of the list so it's not taking cuts in the lineup
			remove_que(&request->lrq_lbl_requests);
			insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
		}
	}

	own* owner = (own*) SRQ_ABS_PTR(owner_offset);
	SRQ_PTR request_offset = SRQ_REL_PTR(request);
	owner->own_pending_request = request_offset;
	owner->own_flags &= ~(OWN_scanned | OWN_wakeup);
	owner->own_flags |= OWN_waiting;

	if (lck_wait > 0)
		owner->own_flags &= ~OWN_timeout;
	else
		owner->own_flags |= OWN_timeout;

	SLONG value = ISC_event_clear(&owner->own_wakeup);

	// Post blockage. If the blocking owner has disappeared, the blockage
	// may clear spontaneously.

	post_blockage(tdbb, request, lock);
	post_history(his_wait, owner_offset, lock_offset, SRQ_REL_PTR(request), true);
	release_shmem(owner_offset);

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
		int ret = FB_FAILURE;

		// Before starting to wait - look to see if someone resolved
		// the request for us - if so we're out easy!

		request = (lrq*) SRQ_ABS_PTR(request_offset);
		if (!(request->lrq_flags & LRQ_pending)) {
			break;
		}

		// Recalculate when we next want to wake up, the lesser of a
		// deadlock scan interval or when the lock request wanted a timeout

		time_t timeout = deadlock_timeout;
		if (lck_wait < 0 && lock_timeout < deadlock_timeout)
			timeout = lock_timeout;

		// Prepare to wait for a timeout or a wakeup from somebody else

		owner = (own*) SRQ_ABS_PTR(owner_offset);
		if (!(owner->own_flags & OWN_wakeup))
		{
			// Re-initialize value each time thru the loop to make sure that the
			// semaphore looks 'un-poked'

			LocalCheckout checkout(this);

			{ // scope
				Firebird::ReadLockGuard guard(m_remapSync);
				owner = (own*) SRQ_ABS_PTR(owner_offset);
				++m_waitingOwners;
			}

			{ // scope
				Database::Checkout dcoHolder(tdbb->getDatabase());
				ret = ISC_event_wait(&owner->own_wakeup, value, (timeout - current_time) * 1000000);
				--m_waitingOwners;
			}
		}

		// We've woken up from the wait - now look around and see why we wokeup

		// If somebody else has resolved the lock, we're done

		request = (lrq*) SRQ_ABS_PTR(request_offset);
		if (!(request->lrq_flags & LRQ_pending)) {
			break;
		}

		acquire_shmem(owner_offset);
		// See if we wokeup due to another owner deliberately waking us up
		// ret==FB_SUCCESS --> we were deliberately woken up
		// ret==FB_FAILURE --> we still don't know why we woke up

		// Only if we came out of the ISC_event_wait() because of a post_wakeup()
		// by another owner is OWN_wakeup set. This is the only FB_SUCCESS case.

		owner = (own*) SRQ_ABS_PTR(owner_offset);
		if (ret == FB_SUCCESS)
		{
			value = ISC_event_clear(&owner->own_wakeup);
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
		// ISC_event_wait() uses finer granularity)

		if ((ret != FB_SUCCESS) && (current_time + 1 < timeout))
		{
			release_shmem(owner_offset);
			continue;
		}

		// We apparently woke up for some real reason.
		// Make sure everyone is still with us. Then see if we're still blocked.

		request = (lrq*) SRQ_ABS_PTR(request_offset);
		lock = (lbl*) SRQ_ABS_PTR(lock_offset);
		owner = (own*) SRQ_ABS_PTR(owner_offset);
		owner->own_flags &= ~OWN_wakeup;

		// Now that we own the lock table, see if the request was resolved
		// while we were waiting for the lock table

		if (!(request->lrq_flags & LRQ_pending))
		{
			release_shmem(owner_offset);
			break;
		}

		// See if we've waited beyond the lock timeout -
		// if so we mark our own request as rejected

		const bool cancelled = tdbb->checkCancelState(false); 

		if (cancelled || lck_wait < 0 && lock_timeout <= current_time)
		{
			// We're going to reject our lock - it's the callers responsibility
			// to do cleanup and make sure post_pending() is called to wakeup
			// other owners we might be blocking
			request->lrq_flags |= LRQ_rejected;
			request->lrq_flags &= ~LRQ_pending;
			lock->lbl_pending_lrq_count--;
			
			// and test - may be timeout due to missing process to deliver request
			probe_processes();
			release_shmem(owner_offset);
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

			post_blockage(tdbb, request, lock);
			release_shmem(owner_offset);
			continue;
		}

		// See if all the other owners are still alive. Dead ones will be purged,
		// purging one might resolve our lock request.
		// Do not do rescan of owners if we received notification that
		// blocking ASTs have completed - will do it next time if needed.

		if (probe_processes() && !(request->lrq_flags & LRQ_pending))
		{
			release_shmem(owner_offset);
			break;
		}

		// If we've not previously been scanned for a deadlock and going to wait
		// forever, go do a deadlock scan

		lrq* blocking_request;
		if (!(owner->own_flags & (OWN_scanned | OWN_timeout)) &&
			(blocking_request = deadlock_scan(owner, request)))
		{
			// Something has been selected for rejection to prevent a
			// deadlock. Clean things up and go on. We still have to
			// wait for our request to be resolved.

			DEBUG_MSG(0, ("wait_for_request: selecting something for deadlock kill\n"));

			++m_header->lhb_deadlocks;
			blocking_request->lrq_flags |= LRQ_rejected;
			blocking_request->lrq_flags &= ~LRQ_pending;
			lbl* blocking_lock = (lbl*) SRQ_ABS_PTR(blocking_request->lrq_lock);
			blocking_lock->lbl_pending_lrq_count--;

			own* const blocking_owner = (own*) SRQ_ABS_PTR(blocking_request->lrq_owner);
			blocking_owner->own_pending_request = 0;
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
			post_blockage(tdbb, request, lock);
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
		release_shmem(owner_offset);
	}

	// NOTE: lock table is not acquired at this point
#ifdef DEV_BUILD
	request = (lrq*) SRQ_ABS_PTR(request_offset);
	CHECK(!(request->lrq_flags & LRQ_pending));
#endif

	owner = (own*) SRQ_ABS_PTR(owner_offset);
	owner->own_pending_request = 0;
	owner->own_flags &= ~(OWN_waiting | OWN_timeout);

	return FB_SUCCESS;
}

} // namespace
