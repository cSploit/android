/*
 *      PROGRAM:        JRD access method
 *      MODULE:         Database.h
 *      DESCRIPTION:    Common descriptions
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * Claudio Valderrama C.
 *
 */

#ifndef JRD_DATABASE_H
#define JRD_DATABASE_H

#include "firebird.h"
#include "../jrd/cch.h"
#include "../jrd/gdsassert.h"
#include "../jrd/common.h"
#include "../jrd/dsc.h"
#include "../jrd/btn.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/val.h"
#include "../jrd/irq.h"
#include "../jrd/drq.h"
#include "../include/gen/iberror.h"

#include "../common/classes/fb_atomic.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/array.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/stack.h"
#include "../common/classes/timestamp.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/RefCounted.h"
#include "../common/classes/PublicHandle.h"
#include "../common/classes/semaphore.h"
#include "../common/utils_proto.h"
#include "../jrd/DatabaseSnapshot.h"
#include "../jrd/RandomGenerator.h"
#include "../jrd/os/guid.h"
#include "../jrd/sbm.h"
#include "../jrd/flu.h"
#include "../jrd/RuntimeStatistics.h"
#include "../jrd/os/thd_priority.h"
#include "../jrd/event_proto.h"
#include "../lock/lock_proto.h"

class CharSetContainer;

namespace Jrd
{
	class Trigger;
	template <typename T> class vec;
	class jrd_prc;
	class jrd_rel;
	class Shadow;
	class BlobFilter;
	class TxPageCache;
	class BackupManager;
	class vcl;

	typedef Firebird::ObjectsArray<Trigger> trig_vec;


//
// bit values for dbb_flags
//
const ULONG DBB_damaged				= 0x1L;
const ULONG DBB_exclusive			= 0x2L;		// Database is accessed in exclusive mode
const ULONG DBB_bugcheck			= 0x4L;		// Bugcheck has occurred
#ifdef GARBAGE_THREAD
const ULONG DBB_garbage_collector	= 0x8L;		// garbage collector thread exists
const ULONG DBB_gc_active			= 0x10L;	// ... and is actively working.
const ULONG DBB_gc_pending			= 0x20L;	// garbage collection requested
#endif
const ULONG DBB_force_write			= 0x40L;	// Database is forced write
const ULONG DBB_no_reserve			= 0x80L;	// No reserve space for versions
const ULONG DBB_DB_SQL_dialect_3	= 0x100L;	// database SQL dialect 3
const ULONG DBB_read_only			= 0x200L;	// DB is ReadOnly (RO). If not set, DB is RW
const ULONG DBB_being_opened_read_only	= 0x400L;	// DB is being opened RO. If unset, opened as RW
const ULONG DBB_not_in_use			= 0x800L;	// Database to be ignored while attaching
const ULONG DBB_lck_init_done		= 0x1000L;	// LCK_init() called for the database
const ULONG DBB_sweep_in_progress	= 0x2000L;	// A database sweep operation is in progress
const ULONG DBB_security_db			= 0x4000L;	// ISC security database
const ULONG DBB_suspend_bgio		= 0x8000L;	// Suspend I/O by background threads
const ULONG DBB_being_opened		= 0x10000L;	// database is being attached to
const ULONG DBB_gc_cooperative		= 0x20000L;	// cooperative garbage collection
const ULONG DBB_gc_background		= 0x40000L;	// background garbage collection by gc_thread
const ULONG DBB_no_fs_cache			= 0x80000L;	// Not using file system cache
const ULONG DBB_destroying			= 0x100000L;	// database destructor is called
const ULONG DBB_monitor_locking		= 0x200000L;	// monitoring lock is being acquired

//
// dbb_ast_flags
//
const ULONG DBB_blocking			= 0x1L;		// Exclusive mode is blocking
const ULONG DBB_get_shadows			= 0x2L;		// Signal received to check for new shadows
const ULONG DBB_assert_locks		= 0x4L;		// Locks are to be asserted
const ULONG DBB_shutdown			= 0x8L;		// Database is shutdown
const ULONG DBB_shut_attach			= 0x10L;	// no new attachments accepted
const ULONG DBB_shut_tran			= 0x20L;	// no new transactions accepted
const ULONG DBB_shut_force			= 0x40L;	// forced shutdown in progress
const ULONG DBB_shutdown_locks		= 0x80L;	// Database locks release by shutdown
const ULONG DBB_shutdown_full		= 0x100L;	// Database fully shut down
const ULONG DBB_shutdown_single		= 0x200L;	// Database is in single-user maintenance mode
const ULONG DBB_monitor_off			= 0x400L;	// Database has the monitoring lock released

class Database : public pool_alloc<type_dbb>, public Firebird::PublicHandle
{
	class Sync : public Firebird::RefCounted
	{
	public:
		Sync() : threadId(0), isAst(false)
		{}

		void lock(bool ast = false)
		{
			ThreadPriorityScheduler::enter();
			++waiters;
			syncMutex.enter();
			--waiters;
			threadId = getThreadId();
			isAst = ast;
		}

		void unlock()
		{
			ThreadPriorityScheduler::exit();
			isAst = false;
			threadId = 0;
			syncMutex.leave();
		}

		bool hasContention() const
		{
			return (waiters.value() > 0);
		}

	private:
		~Sync()
		{
			if (threadId)
			{
				syncMutex.leave();
			}
		}

		// copying is prohibited
		Sync(const Sync&);
		Sync& operator=(const Sync&);

		Firebird::Mutex syncMutex;
		Firebird::AtomicCounter waiters;
		FB_THREAD_ID threadId;
		bool isAst;
	};

public:

	class SyncGuard
	{
	public:
		explicit SyncGuard(Database* dbb, bool ast = false)
			: sync(*dbb->dbb_sync)
		{
			if (!dbb->checkHandle())
			{
				Firebird::status_exception::raise(Firebird::Arg::Gds(isc_bad_db_handle));
			}

			sync.addRef();
			sync.lock(ast);

			if (!dbb->checkHandle())
			{
				sync.unlock();
				sync.release();
				Firebird::status_exception::raise(Firebird::Arg::Gds(isc_bad_db_handle));
			}

			if (ast && dbb->dbb_flags & DBB_destroying)
			{
				sync.unlock();
				sync.release();
				Firebird::LongJump::raise();
			}
		}

		~SyncGuard()
		{
			try
			{
				sync.unlock();
			}
			catch (const Firebird::Exception&)
			{
				DtorException::devHalt();
			}
			sync.release();
		}

	private:
		// copying is prohibited
		SyncGuard(const SyncGuard&);
		SyncGuard& operator=(const SyncGuard&);

		Sync& sync;
	};

	class Checkout
	{
	public:
		explicit Checkout(Database* dbb)
			: sync(*dbb->dbb_sync)
		{
			sync.unlock();
		}

		~Checkout()
		{
			sync.lock();
		}

	private:
		// copying is prohibited
		Checkout(const Checkout&);
		Checkout& operator=(const Checkout&);

		Sync& sync;
	};

	class CheckoutLockGuard
	{
	public:
		CheckoutLockGuard(Database* dbb, Firebird::Mutex& m)
			: mutex(m)
		{
			if (!mutex.tryEnter())
			{
				Checkout dcoHolder(dbb);
				mutex.enter();
			}
		}

		~CheckoutLockGuard()
		{
			try {
				mutex.leave();
			}
			catch (const Firebird::Exception&)
			{
				DtorException::devHalt();
			}
		}

	private:
		// copying is prohibited
		CheckoutLockGuard(const CheckoutLockGuard&);
		CheckoutLockGuard& operator=(const CheckoutLockGuard&);

		Firebird::Mutex& mutex;
	};

	class SharedCounter
	{
		static const ULONG DEFAULT_CACHE_SIZE = 16;

		struct ValueCache
		{
			Lock* lock;
			SLONG curVal;
			SLONG maxVal;
		};

	public:

		enum
		{
			ATTACHMENT_ID_SPACE = 0,
			TRANSACTION_ID_SPACE = 1,
			STATEMENT_ID_SPACE = 2,
			TOTAL_ITEMS = 3
		};

		SharedCounter();
		~SharedCounter();

		SLONG generate(thread_db* tdbb, ULONG space, ULONG prefetch = DEFAULT_CACHE_SIZE);
		void shutdown(thread_db* tdbb);

	private:

		static int blockingAst(void* arg);

		ValueCache m_counters[TOTAL_ITEMS];
	};

	typedef int (*crypt_routine) (const char*, void*, int, void*);

	static Database* create()
	{
		Firebird::MemoryStats temp_stats;
		MemoryPool* const pool = MemoryPool::createPool(NULL, temp_stats);
		Database* const dbb = FB_NEW(*pool) Database(pool);
		pool->setStatsGroup(dbb->dbb_memory_stats);
		return dbb;
	}

	// The destroy() function MUST be used to delete a Database object.
	// The function hides some tricky order of operations.  Since the
	// memory for the vectors in the Database is allocated out of the Database's
	// permanent memory pool, the entire delete() operation needs
	// to complete _before_ the permanent pool is deleted, or else
	// risk an aborted engine.
	static void destroy(Database* const toDelete)
	{
		if (!toDelete)
			return;

		MemoryPool* const perm = toDelete->dbb_permanent;

		// Memory pool destruction below decrements memory statistics
		// situated in database block we are about to deallocate right now
		Firebird::MemoryStats temp_stats;
		perm->setStatsGroup(temp_stats);

		delete toDelete;
		MemoryPool::deletePool(perm);
	}

	static ULONG getLockOwnerId()
	{
		return fb_utils::genUniqueId();
	}

	bool checkHandle() const
	{
		if (!isKnownHandle())
		{
			return false;
		}

		return TypedHandle<type_dbb>::checkHandle();
	}

	mutable Firebird::RefPtr<Sync> dbb_sync;	// Database sync primitive

	LockManager*	dbb_lock_mgr;
	EventManager*	dbb_event_mgr;

	Database*	dbb_next;				// Next database block in system
	Attachment* dbb_attachments;		// Active attachments
	BufferControl*	dbb_bcb;			// Buffer control block
	vec<jrd_rel*>*	dbb_relations;		// relation vector
	vec<jrd_prc*>*	dbb_procedures;		// scanned procedures
	int			dbb_monitoring_id;		// dbb monitoring identifier
	Lock* 		dbb_lock;				// granddaddy lock
	jrd_tra*	dbb_sys_trans;			// system transaction
	Shadow*		dbb_shadow;				// shadow control block
	Lock*		dbb_shadow_lock;		// lock for synchronizing addition of shadows
	Lock*		dbb_retaining_lock;		// lock for preserving commit retaining snapshot
	Lock*		dbb_monitor_lock;		// lock for monitoring purposes
	PageManager dbb_page_manager;
	vcl*		dbb_t_pages;			// pages number for transactions
	vcl*		dbb_gen_id_pages;		// known pages for gen_id
	BlobFilter*	dbb_blob_filters;		// known blob filters
	trig_vec*	dbb_triggers[DB_TRIGGER_MAX];

	DatabaseSnapshot::SharedData*	dbb_monitoring_data;	// monitoring data

	DatabaseModules	dbb_modules;		// external function/filter modules

	Firebird::Mutex dbb_meta_mutex;		// Mutex to protect metadata changes while dbb_sync is unlocked
	Firebird::Mutex dbb_cmp_clone_mutex;
	Firebird::Mutex dbb_exe_clone_mutex;
	Firebird::Mutex dbb_flush_count_mutex;
	Firebird::Mutex dbb_dyn_mutex;
	Firebird::Mutex dbb_sys_dfw_mutex;

	//SLONG dbb_sort_size;				// Size of sort space per sort, unused for now

	ULONG dbb_ast_flags;				// flags modified at AST level
	ULONG dbb_flags;
	USHORT dbb_ods_version;				// major ODS version number
	USHORT dbb_minor_version;			// minor ODS version number
	USHORT dbb_minor_original;			// minor ODS version at creation
	USHORT dbb_page_size;				// page size
	USHORT dbb_dp_per_pp;				// data pages per pointer page
	USHORT dbb_max_records;				// max record per data page
	USHORT dbb_max_idx;					// max number of indexes on a root page
	USHORT dbb_max_sys_rel;				// max id of system relation
	USHORT dbb_use_count;				// active count of threads

#ifdef SUPERSERVER_V2
	USHORT dbb_prefetch_sequence;		// sequence to pace frequency of prefetch requests
	USHORT dbb_prefetch_pages;			// prefetch pages per request
#endif

	Firebird::PathName dbb_filename;	// filename string
	Firebird::PathName dbb_database_name;	// database ID (file name or alias)
	Firebird::string dbb_encrypt_key;	// encryption key

	MemoryPool* dbb_permanent;
	MemoryPool* dbb_bufferpool;

	Firebird::Array<MemoryPool*> dbb_pools;		// pools

	Firebird::Array<jrd_req*> dbb_internal;		// internal requests
	Firebird::Array<jrd_req*> dbb_dyn_req;		// internal dyn requests

	SLONG dbb_oldest_active;			// Cached "oldest active" transaction
	SLONG dbb_oldest_transaction;		// Cached "oldest interesting" transaction
	SLONG dbb_oldest_snapshot;			// Cached "oldest snapshot" of all active transactions
	SLONG dbb_next_transaction;			// Next transaction id used by NETWARE
	SLONG dbb_attachment_id;			// Next attachment id for ReadOnly DB's
	SLONG dbb_page_incarnation;			// Cache page incarnation counter
	ULONG dbb_page_buffers;				// Page buffers from header page

	Firebird::Semaphore dbb_writer_sem;	// Wake up cache writer
	Firebird::Semaphore dbb_writer_init;// Cache writer initialization
	Firebird::Semaphore dbb_writer_fini;// Cache writer finalization
#ifdef SUPERSERVER_V2
	// the code in cch.cpp is not tested for semaphore instead event !!!
	Firebird::Semaphore dbb_reader_sem;	// Wake up cache reader
	Firebird::Semaphore dbb_reader_init;// Cache reader initialization
	Firebird::Semaphore dbb_reader_fini;// Cache reader finalization
#endif

#ifdef GARBAGE_THREAD
	Firebird::Semaphore dbb_gc_sem;		// Event to wake up garbage collector
	Firebird::Semaphore dbb_gc_init;	// Event for initialization garbage collector
	Firebird::Semaphore dbb_gc_fini;	// Event for finalization garbage collector
#endif

	Firebird::MemoryStats dbb_memory_stats;

	RuntimeStatistics dbb_stats;
	SLONG dbb_last_header_write;		// Transaction id of last header page physical write
	SLONG dbb_flush_cycle;				// Current flush cycle
	SLONG dbb_sweep_interval;			// Transactions between sweep
	const ULONG dbb_lock_owner_id;		// ID for the lock manager
	SLONG dbb_lock_owner_handle;		// Handle for the lock manager

	USHORT unflushed_writes;			// unflushed writes
	time_t last_flushed_write;			// last flushed write time

	crypt_routine dbb_encrypt;			// External encryption routine
	crypt_routine dbb_decrypt;			// External decryption routine

	Firebird::Array<CharSetContainer*>		dbb_charsets;	// intl character set descriptions
	TxPageCache*	dbb_tip_cache;		// cache of latest known state of all transactions in system
	vcl*		dbb_pc_transactions;	// active precommitted transactions
	BackupManager*	dbb_backup_manager;	// physical backup manager
	Firebird::TimeStamp dbb_creation_date; // creation date
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, UserFunction*> > > dbb_functions;	// User defined functions

	SharedCounter dbb_shared_counter;

	// returns true if primary file is located on raw device
	bool onRawDevice() const;

	// returns an unique ID string for a database file
	Firebird::string getUniqueFileId() const;

	MemoryPool* createPool()
	{
		MemoryPool* const pool = MemoryPool::createPool(dbb_permanent, dbb_memory_stats);
		dbb_pools.add(pool);
		return pool;
	}

	void deletePool(MemoryPool* pool);

private:
	explicit Database(MemoryPool* p)
	:	dbb_sync(FB_NEW(*getDefaultMemoryPool()) Sync),
		dbb_page_manager(*p),
		dbb_modules(*p),
		dbb_filename(*p),
		dbb_database_name(*p),
		dbb_encrypt_key(*p),
		dbb_permanent(p),
		dbb_pools(*p, 4),
		dbb_internal(*p),
		dbb_dyn_req(*p),
		dbb_stats(*p),
		dbb_lock_owner_id(getLockOwnerId()),
		dbb_charsets(*p),
		dbb_creation_date(Firebird::TimeStamp::getCurrentTimeStamp()),
		dbb_functions(*p)
	{
		dbb_pools.add(p);
		dbb_internal.grow(irq_MAX);
		dbb_dyn_req.grow(drq_MAX);
	}

	~Database();

public:
	// temporary measure to avoid unstable state of lock file -
	// this is anyway called in ~Database(), and in theory should be private
	void releaseIntlObjects();			// defined in intl.cpp
	void destroyIntlObjects();			// defined in intl.cpp

	SLONG generateAttachmentId(thread_db* tdbb)
	{
		return dbb_shared_counter.generate(tdbb, SharedCounter::ATTACHMENT_ID_SPACE, 1);
	}

	SLONG generateTransactionId(thread_db* tdbb)
	{
		return dbb_shared_counter.generate(tdbb, SharedCounter::TRANSACTION_ID_SPACE, 1);
	}

	SLONG generateStatementId(thread_db* tdbb)
	{
		return dbb_shared_counter.generate(tdbb, SharedCounter::STATEMENT_ID_SPACE);
	}

private:

	// The delete operators are no-oped because the Database memory is allocated from the
	// Database's own permanent pool.  That pool has already been released by the Database
	// destructor, so the memory has already been released.  Hence the operator
	// delete no-op.
	void operator delete(void*) {}
	void operator delete[](void*) {}

	Database(const Database&);			// no impl.
	const Database& operator =(const Database&) { return *this; }
};

} // namespace Jrd

#endif // JRD_DATABASE_H
