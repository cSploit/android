/*
 *	PROGRAM:	JRD Lock Manager
 *	MODULE:		lock_proto.h
 *	DESCRIPTION:	Prototype header file for lock.cpp
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
 */

#ifndef LOCK_LOCK_PROTO_H
#define LOCK_LOCK_PROTO_H

#if defined(USE_SHMEM_EXT) && (! defined(SRQ_ABS_PTR))
#define SRQ_ABS_PTR(x) ABS_PTR(x)
#define SRQ_REL_PTR(x) REL_PTR(x)
#endif

#include "../common/classes/semaphore.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/init.h"
#include "../common/classes/RefCounted.h"
#include "../common/classes/array.h"
#include "../jrd/ThreadStart.h"
#include "../jrd/isc.h"

#include <stdio.h>
#include <sys/types.h>

#if !defined(WIN_NT)
#include <signal.h>
#include <sys/ipc.h>
#include <linux/shm.h>
#include <semaphore.h>
#endif

#include "../jrd/common.h"
#include "../jrd/file_params.h"
#include "../jrd/que.h"

typedef FB_UINT64 LOCK_OWNER_T; // Data type for the Owner ID

// Maximum lock series for gathering statistics and querying data

const int LCK_MAX_SERIES	= 7;

// Lock query data aggregates

const int LCK_MIN		= 1;
const int LCK_MAX		= 2;
const int LCK_CNT		= 3;
const int LCK_SUM		= 4;
const int LCK_AVG		= 5;
const int LCK_ANY		= 6;

// Lock states
// in LCK_convert the type of level is USHORT instead of UCHAR
const UCHAR LCK_none	= 0;
const UCHAR LCK_null	= 1;
const UCHAR LCK_SR		= 2;		// Shared Read
const UCHAR LCK_PR		= 3;		// Protected Read
const UCHAR LCK_SW		= 4;		// Shared Write
const UCHAR LCK_PW		= 5;		// Protected Write
const UCHAR LCK_EX		= 6;		// Exclusive
const UCHAR LCK_max		= 7;

enum locklevel_t {LCK_read = LCK_PR, LCK_write = LCK_EX};

const SSHORT LCK_WAIT		= 1;
const SSHORT LCK_NO_WAIT	= 0;

// Lock block types

const UCHAR type_null	= 0;
const UCHAR type_lhb	= 1;
const UCHAR type_lrq	= 2;
const UCHAR type_lbl	= 3;
const UCHAR type_his	= 4;
const UCHAR type_shb	= 5;
const UCHAR type_own	= 6;
const UCHAR type_lpr	= 7;
const UCHAR type_MAX	= type_lpr;

// Version number of the lock table.
// Must be increased every time the shmem layout is changed.
const UCHAR BASE_LHB_VERSION = 17;

#if SIZEOF_VOID_P == 8
const UCHAR PLATFORM_LHB_VERSION	= 128;	// 64-bit target
#else
const UCHAR PLATFORM_LHB_VERSION	= 0;	// 32-bit target
#endif

const UCHAR LHB_VERSION	= PLATFORM_LHB_VERSION + BASE_LHB_VERSION;

#ifndef SUPERSERVER
#define USE_BLOCKING_THREAD
#endif

// Lock header block -- one per lock file, lives up front

struct lhb
{
	UCHAR lhb_type;					// memory tag - always type_lbh
	UCHAR lhb_version;				// Version of lock table
	SRQ_PTR lhb_secondary;			// Secondary lock header block
	SRQ_PTR lhb_active_owner;		// Active owner, if any
	srq lhb_owners;					// Que of active owners
	srq lhb_processes;				// Que of active processes
	srq lhb_free_processes;			// Free process blocks
	srq lhb_free_owners;			// Free owner blocks
	srq lhb_free_locks;				// Free lock blocks
	srq lhb_free_requests;			// Free lock requests
	ULONG lhb_length;				// Size of lock table
	ULONG lhb_used;					// Bytes of lock table in use
	USHORT lhb_hash_slots;			// Number of hash slots allocated
	USHORT lhb_flags;				// Miscellaneous info
	struct mtx lhb_mutex;			// Mutex controlling access
	SRQ_PTR lhb_history;
	ULONG lhb_scan_interval;		// Deadlock scan interval (secs)
	ULONG lhb_acquire_spins;
	FB_UINT64 lhb_acquires;
	FB_UINT64 lhb_acquire_blocks;
	FB_UINT64 lhb_acquire_retries;
	FB_UINT64 lhb_retry_success;
	FB_UINT64 lhb_enqs;
	FB_UINT64 lhb_converts;
	FB_UINT64 lhb_downgrades;
	FB_UINT64 lhb_deqs;
	FB_UINT64 lhb_read_data;
	FB_UINT64 lhb_write_data;
	FB_UINT64 lhb_query_data;
	FB_UINT64 lhb_operations[LCK_MAX_SERIES];
	FB_UINT64 lhb_waits;
	FB_UINT64 lhb_denies;
	FB_UINT64 lhb_timeouts;
	FB_UINT64 lhb_blocks;
	FB_UINT64 lhb_wakeups;
	FB_UINT64 lhb_scans;
	FB_UINT64 lhb_deadlocks;
	srq lhb_data[LCK_MAX_SERIES];
	srq lhb_hash[1];			// Hash table
};

// lhb_flags
const USHORT LHB_lock_ordering		= 1;	// Lock ordering is enabled

// Secondary header block -- exists only in V3.3 and later lock managers.
// It is pointed to by the word in the lhb that used to contain a pattern.

struct shb
{
	UCHAR shb_type;					// memory tag - always type_shb
	SRQ_PTR shb_history;
	SRQ_PTR shb_remove_node;		// Node removing itself
	SRQ_PTR shb_insert_que;			// Queue inserting into
	SRQ_PTR shb_insert_prior;		// Prior of inserting queue
};

// Lock block

struct lbl
{
	UCHAR lbl_type;					// mem tag: type_lbl=in use, type_null=free
	UCHAR lbl_state;				// High state granted
	UCHAR lbl_size;					// Key bytes allocated
	UCHAR lbl_length;				// Key bytes used
	srq lbl_requests;				// Requests granted
	srq lbl_lhb_hash;				// Collision que for hash table
	srq lbl_lhb_data;				// Lock data que by series
	SLONG lbl_data;					// User data
	SRQ_PTR lbl_parent;				// Parent
	UCHAR lbl_series;				// Lock series
	UCHAR lbl_flags;				// Unused. Misc flags
	USHORT lbl_pending_lrq_count;	// count of lbl_requests with LRQ_pending
	USHORT lbl_counts[LCK_max];		// Counts of granted locks
	UCHAR lbl_key[1];				// Key value
};

/* Lock requests */

struct lrq
{
	UCHAR lrq_type;					// mem tag: type_lrq=in use, type_null=free
	UCHAR lrq_requested;			// Level requested
	UCHAR lrq_state;				// State of lock request
	USHORT lrq_flags;				// Misc crud
	SRQ_PTR lrq_owner;				// Owner making request
	SRQ_PTR lrq_lock;				// Lock requested
	SLONG lrq_data;					// Lock data requested
	srq lrq_own_requests;			// Locks granted for owner
	srq lrq_lbl_requests;			// Que of requests (active, pending)
	srq lrq_own_blocks;				// Owner block que
	lock_ast_t lrq_ast_routine;		// Block ast routine
	void* lrq_ast_argument;			// Ast argument
};

// lrq_flags
const USHORT LRQ_blocking		= 1;		// Request is blocking
const USHORT LRQ_pending		= 2;		// Request is pending
const USHORT LRQ_converting		= 4;		// Request is pending conversion
const USHORT LRQ_rejected		= 8;		// Request is rejected
const USHORT LRQ_timed_out		= 16;		// Wait timed out
const USHORT LRQ_deadlock		= 32;		// Request has been seen by the deadlock-walk
const USHORT LRQ_repost			= 64;		// Request block used for repost
const USHORT LRQ_scanned		= 128;		// Request already scanned for deadlock
const USHORT LRQ_blocking_seen	= 256;		// Blocking notification received by owner
const USHORT LRQ_just_granted	= 512;		// Request is just granted and blocked owners still have not sent blocking AST

// Process block

struct prc
{
	UCHAR prc_type;					// memory tag - always type_lpr
	int prc_process_id;				// Process ID
	srq prc_lhb_processes;			// Process que
	srq prc_owners;					// Owners
	event_t prc_blocking;			// Blocking event block
	USHORT prc_flags;				// Unused. Misc flags
};

// Owner block

struct own
{
	UCHAR own_type;					// memory tag - always type_own
	UCHAR own_owner_type;			// type of owner
	SSHORT own_count;				// init count for the owner
	LOCK_OWNER_T own_owner_id;		// Owner ID
	srq own_lhb_owners;				// Owner que (global)
	srq own_prc_owners;				// Owner que (process wide)
	srq own_requests;				// Lock requests granted
	srq own_blocks;					// Lock requests blocking
	SRQ_PTR own_pending_request;	// Request we're waiting on
	SRQ_PTR own_process;			// Process we belong to
	FB_THREAD_ID own_thread_id;		// Last thread attached to the owner
	FB_UINT64 own_acquire_time;		// lhb_acquires when owner last tried acquire()
	USHORT own_ast_count;			// Number of ASTs being delivered
	event_t own_wakeup;				// Wakeup event block
	USHORT own_flags;				// Misc stuff
};

// Flags in own_flags
const USHORT OWN_blocking	= 1;		// Owner is blocking
const USHORT OWN_scanned	= 2;		// Owner has been deadlock scanned
const USHORT OWN_waiting	= 4;		// Owner is waiting inside wait_for_request()
const USHORT OWN_wakeup		= 8;		// Owner has been awoken
const USHORT OWN_signaled	= 16;		// Signal is thought to be delivered
const USHORT OWN_timeout	= 32;		// Owner is waiting with timeout

// Lock manager history block

struct his
{
	UCHAR his_type;					// memory tag - always type_his
	UCHAR his_operation;			// operation that occurred
	SRQ_PTR his_next;				// SRQ_PTR to next item in history list
	SRQ_PTR his_process;			// owner to record for this operation
	SRQ_PTR his_lock;				// lock to record for operation
	SRQ_PTR his_request;			// request to record for operation
};

// his_operation definitions
// should be UCHAR according to his_operation but is USHORT in lock.cpp:post_operation
const UCHAR his_enq			= 1;
const UCHAR his_deq			= 2;
const UCHAR his_convert		= 3;
const UCHAR his_signal		= 4;
const UCHAR his_post_ast	= 5;
const UCHAR his_wait		= 6;
const UCHAR his_del_process	= 7;
const UCHAR his_del_lock	= 8;
const UCHAR his_del_request	= 9;
const UCHAR his_deny		= 10;
const UCHAR his_grant		= 11;
const UCHAR his_leave_ast	= 12;
const UCHAR his_scan		= 13;
const UCHAR his_dead		= 14;
const UCHAR his_enter		= 15;
const UCHAR his_bug			= 16;
const UCHAR his_active		= 17;
const UCHAR his_cleanup		= 18;
const UCHAR his_del_owner	= 19;
const UCHAR his_MAX			= his_del_owner;

namespace Firebird {
	class AtomicCounter;
	class Mutex;
	class RWLock;
}

namespace Jrd {

class thread_db;

class LockManager : private Firebird::RefCounted, public Firebird::GlobalStorage
{
	class LocalGuard
	{
	public:
		explicit LocalGuard(LockManager* lm)
			: m_lm(lm)
		{
			if (!m_lm->m_localMutex.tryEnter())
			{
				m_lm->m_localMutex.enter();
				m_lm->m_localBlockage = true;
			}
		}

		~LocalGuard()
		{
			try
			{
				m_lm->m_localMutex.leave();
			}
			catch (const Firebird::Exception&)
			{
				DtorException::devHalt();
			}
		}

	private:
		// Forbid copying
		LocalGuard(const LocalGuard&);
		LocalGuard& operator=(const LocalGuard&);

		LockManager* m_lm;
	};

	class LocalCheckout
	{
	public:
		explicit LocalCheckout(LockManager* lm)
			: m_lm(lm)
		{
			m_lm->m_localMutex.leave();
		}

		~LocalCheckout()
		{
			try
			{
				if (!m_lm->m_localMutex.tryEnter())
				{
					m_lm->m_localMutex.enter();
					m_lm->m_localBlockage = true;
				}
			}
			catch (const Firebird::Exception&)
			{
				DtorException::devHalt();
			}
		}

	private:
		// Forbid copying
		LocalCheckout(const LocalCheckout&);
		LocalCheckout& operator=(const LocalCheckout&);

		LockManager* m_lm;
	};

	typedef Firebird::GenericMap<Firebird::Pair<Firebird::Left<Firebird::string, LockManager*> > > DbLockMgrMap;

	static Firebird::GlobalPtr<DbLockMgrMap> g_lmMap;
	static Firebird::GlobalPtr<Firebird::Mutex> g_mapMutex;

	const int PID;

public:
	static LockManager* create(const Firebird::string&);
	static void destroy(LockManager*);

	bool initializeOwner(thread_db*, LOCK_OWNER_T, UCHAR, SRQ_PTR*);
	void shutdownOwner(thread_db*, SRQ_PTR*);

	SLONG enqueue(thread_db*, SRQ_PTR, SRQ_PTR, const USHORT, const UCHAR*, const USHORT, UCHAR,
				  lock_ast_t, void*, SLONG, SSHORT, SRQ_PTR);
	bool convert(thread_db*, SRQ_PTR, UCHAR, SSHORT, lock_ast_t, void*);
	UCHAR downgrade(thread_db*, const SRQ_PTR);
	bool dequeue(const SRQ_PTR);

	void repost(thread_db*, lock_ast_t, void*, SRQ_PTR);
	bool cancelWait(SRQ_PTR);

	SLONG queryData(SRQ_PTR, const USHORT, const USHORT);
	SLONG readData(SRQ_PTR);
	SLONG readData2(SRQ_PTR, USHORT, const UCHAR*, USHORT, SRQ_PTR);
	SLONG writeData(SRQ_PTR, SLONG);

private:
	explicit LockManager(const Firebird::string&);
	~LockManager();

	bool lockOrdering() const
	{
		return (m_header->lhb_flags & LHB_lock_ordering) ? true : false;
	}

	void acquire_shmem(SRQ_PTR);
	UCHAR* alloc(USHORT, ISC_STATUS*);
	lbl* alloc_lock(USHORT, ISC_STATUS*);
	void blocking_action(thread_db*, SRQ_PTR, SRQ_PTR);
	void blocking_action_thread();
	void bug(ISC_STATUS*, const TEXT*);
	void bug_assert(const TEXT*, ULONG);
	bool create_owner(ISC_STATUS*, LOCK_OWNER_T, UCHAR, SRQ_PTR*);
	bool create_process(ISC_STATUS*);
	void deadlock_clear();
	lrq* deadlock_scan(own*, lrq*);
	lrq* deadlock_walk(lrq*, bool*);
	void debug_delay(ULONG);
	lbl* find_lock(SRQ_PTR, USHORT, const UCHAR*, USHORT, USHORT*);
	lrq* get_request(SRQ_PTR);
	void grant(lrq*, lbl*);
	SRQ_PTR grant_or_que(thread_db*, lrq*, lbl*, SSHORT);
	bool init_owner_block(ISC_STATUS*, own*, UCHAR, LOCK_OWNER_T);
	void initialize(sh_mem*, bool);
	void insert_data_que(lbl*);
	void insert_tail(SRQ, SRQ);
	bool internal_convert(thread_db*, SRQ_PTR, UCHAR, SSHORT, lock_ast_t, void*);
	void internal_dequeue(SRQ_PTR);
	static USHORT lock_state(const lbl*);
	void post_blockage(thread_db*, lrq*, lbl*);
	void post_history(USHORT, SRQ_PTR, SRQ_PTR, SRQ_PTR, bool);
	void post_pending(lbl*);
	void post_wakeup(own*);
	bool probe_processes();
	void purge_owner(SRQ_PTR, own*);
	void purge_process(prc*);
	void remap_local_owners();
	void remove_que(SRQ);
	void release_shmem(SRQ_PTR);
	void release_mutex();
	void release_request(lrq*);
	bool signal_owner(thread_db*, own*, SRQ_PTR);

	void validate_history(const SRQ_PTR history_header);
	void validate_parent(const lhb*, const SRQ_PTR);
	void validate_lhb(const lhb*);
	void validate_lock(const SRQ_PTR, USHORT, const SRQ_PTR);
	void validate_owner(const SRQ_PTR, USHORT);
	void validate_request(const SRQ_PTR, USHORT, USHORT);
	void validate_shb(const SRQ_PTR);

	USHORT wait_for_request(thread_db*, lrq*, SSHORT);
	bool attach_shared_file(ISC_STATUS* status);
	void detach_shared_file(ISC_STATUS* status);
	void get_shared_file_name(Firebird::PathName&, ULONG extend = 0) const;

	static THREAD_ENTRY_DECLARE blocking_action_thread(THREAD_ENTRY_PARAM arg)
	{
		LockManager* const lockMgr = static_cast<LockManager*>(arg);
		lockMgr->blocking_action_thread();
		return 0;
	}

	static void initialize(void* arg, sh_mem* shmem, bool init)
	{
		LockManager* const lockMgr = static_cast<LockManager*>(arg);
		lockMgr->initialize(shmem, init);
	}

	bool m_bugcheck;
	bool m_sharedFileCreated;
	lhb* volatile m_header;
	prc* m_process;
	SRQ_PTR m_processOffset;

	sh_mem m_shmem;

	Firebird::Mutex m_localMutex;
	Firebird::RWLock m_remapSync;
	Firebird::AtomicCounter m_waitingOwners;

	Firebird::Semaphore m_cleanupSemaphore;
	Firebird::Semaphore m_startupSemaphore;

	Firebird::string m_dbId;

	bool m_localBlockage;

	const ULONG m_acquireSpins;
	const ULONG m_memorySize;

#ifdef WIN_NT
	struct mtx m_shmemMutex;
#else
	struct mtx* m_lhb_mutex;
#endif

#ifdef USE_SHMEM_EXT
	struct Extent
	{
		lhb* table;
		sh_mem sh_data;
	};
	Firebird::Array<Extent> m_extents;

	ULONG getTotalMapped() const
	{
		return m_extents.getCount() * getExtendSize();
	}

	ULONG getExtendSize() const
	{
		return m_memorySize;
	}

	ULONG getStartOffset(ULONG n) const
	{
		return n * getExtendSize();
	}

	SRQ_PTR REL_PTR(const void* item);
	void* ABS_PTR(SRQ_PTR item);
	bool newExtent();
#endif
};

} // namespace

#endif // LOCK_LOCK_PROTO_H
