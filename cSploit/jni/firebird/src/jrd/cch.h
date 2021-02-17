/*
 *	PROGRAM:	JRD access method
 *	MODULE:		cch.h
 *	DESCRIPTION:	Cache manager definitions
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

#ifndef JRD_CCH_H
#define JRD_CCH_H

#include "../include/fb_blk.h"
#include "../common/classes/alloc.h"
#include "../common/classes/RefCounted.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/SyncObject.h"
#ifdef SUPERSERVER_V2
#include "../jrd/sbm.h"
#include "../jrd/pag.h"
#endif

#include "../jrd/que.h"
#include "../jrd/lls.h"
#include "../jrd/pag.h"

//#define CCH_DEBUG

#ifdef CCH_DEBUG
DEFINE_TRACE_ROUTINE(cch_trace);
#define CCH_TRACE(args) cch_trace args
#define CCH_TRACE_AST(message) gds__trace(message)
#else
#define CCH_TRACE(args) 		// nothing
#define CCH_TRACEE_AST(message) // nothing
#endif

namespace Ods {
	struct pag;
}

namespace Jrd {

class Lock;
class Precedence;
class thread_db;
struct que;
class BufferDesc;
class Database;

// Page buffer cache size constraints.

const ULONG MIN_PAGE_BUFFERS = 50;
#if SIZEOF_VOID_P == 4
const ULONG MAX_PAGE_BUFFERS = 131072;
#else
const ULONG MAX_PAGE_BUFFERS = MAX_SLONG - 1;
#endif


// BufferControl -- Buffer control block -- one per system

struct bcb_repeat
{
	BufferDesc*	bcb_bdb;		// Buffer descriptor block
	que			bcb_page_mod;	// Que of buffers with page mod n
};

class BufferControl : public pool_alloc<type_bcb>
{
	BufferControl(MemoryPool& p, Firebird::MemoryStats& parentStats)
		: bcb_bufferpool(&p),
		  bcb_memory_stats(&parentStats),
		  bcb_memory(p)
	{
		bcb_database = NULL;
		QUE_INIT(bcb_in_use);
		QUE_INIT(bcb_pending);
		QUE_INIT(bcb_empty);
		QUE_INIT(bcb_dirty);
		bcb_dirty_count = 0;
		bcb_free = NULL;
		bcb_flags = 0;
		bcb_free_minimum = 0;
		bcb_count = 0;
		bcb_inuse = 0;
		bcb_prec_walk_mark = 0;
		bcb_page_size = 0;
		bcb_page_incarnation = 0;
#ifdef SUPERSERVER_V2
		bcb_prefetch = NULL;
#endif
	}

public:

	static BufferControl* create(Database* dbb);
	static void destroy(BufferControl*);

	Database*	bcb_database;

	Firebird::MemoryPool* bcb_bufferpool;
	Firebird::MemoryStats bcb_memory_stats;

	UCharStack	bcb_memory;			// Large block partitioned into buffers
	que			bcb_in_use;			// Que of buffers in use, main LRU que
	que			bcb_pending;		// Que of buffers which are going to be freed and reassigned
	que			bcb_empty;			// Que of empty buffers

	// Recently used buffer put there without locking common LRU que (bcb_in_use).
	// When bcb_syncLRU is locked this chain is merged into bcb_in_use. See also
	// requeueRecentlyUsed() and recentlyUsed()
	Firebird::AtomicPointer<BufferDesc>	bcb_lru_chain;

	que			bcb_dirty;			// que of dirty buffers
	SLONG		bcb_dirty_count;	// count of pages in dirty page btree

	Precedence*	bcb_free;			// Free precedence blocks
	SSHORT		bcb_flags;			// see below
	SSHORT		bcb_free_minimum;	// Threshold to activate cache writer
	ULONG		bcb_count;			// Number of buffers allocated
	ULONG		bcb_inuse;			// Number of buffers in use
	ULONG		bcb_prec_walk_mark;	// mark value used in precedence graph walk
	ULONG		bcb_page_size;		// Database page size in bytes
	ULONG		bcb_page_incarnation;	// Cache page incarnation counter

	Firebird::SyncObject	bcb_syncObject;
	Firebird::SyncObject	bcb_syncDirtyBdbs;
	Firebird::SyncObject	bcb_syncPrecedence;
	Firebird::SyncObject	bcb_syncLRU;
	//Firebird::SyncObject	bcb_syncPageWrite;

	Firebird::Semaphore bcb_writer_sem;		// Wake up cache writer
	Firebird::Semaphore bcb_writer_init;	// Cache writer initialization
	Firebird::Semaphore bcb_writer_fini;	// Cache writer finalization
#ifdef SUPERSERVER_V2
	// the code in cch.cpp is not tested for semaphore instead event !!!
	Firebird::Semaphore bcb_reader_sem;		// Wake up cache reader
	Firebird::Semaphore bcb_reader_init;	// Cache reader initialization
	Firebird::Semaphore bcb_reader_fini;	// Cache reader finalization

	PageBitmap*	bcb_prefetch;		// Bitmap of pages to prefetch
#endif

	bcb_repeat*	bcb_rpt;
};

const int BCB_keep_pages	= 1;	// set during btc_flush(), pages not removed from dirty binary tree
const int BCB_cache_writer	= 2;	// cache writer thread has been started
const int BCB_writer_start  = 4;    // cache writer thread is starting now
const int BCB_writer_active	= 8;	// no need to post writer event count
#ifdef SUPERSERVER_V2
const int BCB_cache_reader	= 16;	// cache reader thread has been started
const int BCB_reader_active	= 32;	// cache reader not blocked on event
#endif
const int BCB_free_pending	= 64;	// request cache writer to free pages
const int BCB_exclusive		= 128;	// there is only BCB in whole system


// BufferDesc -- Buffer descriptor block

class BufferDesc : public pool_alloc<type_bdb>
{
public:
	explicit BufferDesc(BufferControl* bcb)
		: bdb_bcb(bcb),
		  bdb_page(0, 0),
		  bdb_pending_page(0, 0)
	{
		bdb_lock = NULL;
		QUE_INIT(bdb_que);
		QUE_INIT(bdb_in_use);
		QUE_INIT(bdb_dirty);
		bdb_buffer = NULL;
		bdb_incarnation = 0;
		bdb_transactions = 0;
		bdb_mark_transaction = 0;
		QUE_INIT(bdb_lower);
		QUE_INIT(bdb_higher);
		bdb_exclusive = NULL;
		bdb_io = NULL;
		bdb_writers = 0;
		bdb_scan_count = 0;
		bdb_difference_page = 0;
		bdb_prec_walk_mark = 0;
	}

	bool addRef(thread_db* tdbb, Firebird::SyncType syncType, int wait = 1);
	bool addRefConditional(thread_db* tdbb, Firebird::SyncType syncType);
	void downgrade(Firebird::SyncType syncType);
	void release(thread_db* tdbb, bool repost);

	void lockIO(thread_db*);
	void unLockIO(thread_db*);

	bool isLocked() const
	{
		return bdb_syncPage.isLocked();
	}

	bool ourExclusiveLock() const
	{
		return bdb_syncPage.ourExclusiveLock();
	}

	bool ourIOLock() const
	{
		return bdb_syncIO.ourExclusiveLock();
	}

	BufferControl*	bdb_bcb;
	Firebird::SyncObject	bdb_syncPage;
	Lock*		bdb_lock;				// Lock block for buffer
	que			bdb_que;				// Either mod que in hash table or bcb_pending que if BDB_free_pending flag is set
	que			bdb_in_use;				// queue of buffers in use
	que			bdb_dirty;				// dirty pages LRU queue
	BufferDesc*	bdb_lru_chain;			// pending LRU chain
	Ods::pag*	bdb_buffer;				// Actual buffer
	PageNumber	bdb_page;				// Database page number in buffer
	PageNumber	bdb_pending_page;		// Database page number to be
	ULONG		bdb_incarnation;
	ULONG		bdb_transactions;		// vector of dirty flags to reduce commit overhead
	TraNumber	bdb_mark_transaction;	// hi-water mark transaction to defer header page I/O
	que			bdb_lower;				// lower precedence que
	que			bdb_higher;				// higher precedence que
	thread_db*	bdb_exclusive;			// thread holding exclusive latch

private:
	thread_db*				bdb_io;				// thread holding io latch
	Firebird::SyncObject	bdb_syncIO;

public:
	Firebird::AtomicCounter	bdb_ast_flags;		// flags manipulated at AST level
	Firebird::AtomicCounter	bdb_flags;
	Firebird::AtomicCounter	bdb_use_count;		// Number of active users
	SSHORT		bdb_writers;					// Number of recursively taken exclusive locks
	SSHORT		bdb_io_locks;					// Number of recursively taken IO locks
	Firebird::AtomicCounter	bdb_scan_count;		// concurrent sequential scans
	ULONG       bdb_difference_page;			// Number of page in difference file, NBAK
	ULONG		bdb_prec_walk_mark;				// mark value used in precedence graph walk
};

// bdb_flags

// to set/clear BDB_dirty use set_dirty_flag()/clear_dirty_flag()
// These constants should really be of type USHORT.
const int BDB_dirty				= 0x0001;	// page has been updated but not written yet
const int BDB_garbage_collect	= 0x0002;	// left by scan for garbage collector
const int BDB_writer			= 0x0004;	// someone is updating the page
const int BDB_marked			= 0x0008;	// page has been updated
const int BDB_must_write		= 0x0010;	// forces a write as soon as the page is released
const int BDB_faked				= 0x0020;	// page was just allocated
//const int BDB_merge			= 0x0040;
const int BDB_system_dirty 		= 0x0080;	// system transaction has marked dirty
const int BDB_io_error	 		= 0x0100;	// page i/o error
const int BDB_read_pending 		= 0x0200;	// read is pending
const int BDB_free_pending 		= 0x0400;	// buffer being freed for reuse
const int BDB_not_valid			= 0x0800;	// i/o error invalidated buffer
const int BDB_db_dirty 			= 0x1000;	// page must be written to database
//const int BDB_checkpoint		= 0x2000;	// page must be written by next checkpoint
const int BDB_prefetch			= 0x4000;	// page has been prefetched but not yet referenced
const int BDB_no_blocking_ast	= 0x8000;	// No blocking AST registered with page lock
const int BDB_lru_chained		= 0x10000;	// buffer is in pending LRU chain

// bdb_ast_flags

const int BDB_blocking 			= 0x01;		// a blocking ast was sent while page locked


// PRE -- Precedence block

class Precedence : public pool_alloc<type_pre>
{
public:
	BufferDesc*	pre_hi;
	BufferDesc*	pre_low;
	que				pre_lower;
	que				pre_higher;
	SSHORT			pre_flags;
};

const int PRE_cleared	= 1;

/* Compatibility matrix for latch types.

   An exclusive latch is needed to modify a page.  Before
   marking a page an 'io-prevention' latch is needed: a mark latch.
   To look at a buffer, a shared latch is needed.  To write a page,
   an io latch is needed.

   Exclusive and shared latches interact.  Io and mark latches
   interact.

   An mark latch is implemented as an io latch.

   Latches are granted in the order in which they are
   queued with one notable exception -- if buffer write
   is in-progress then shared latches are granted ahead
   of any pending exclusive latches.

	      shared	 io	exclusive   mark
-------------------------------------------------
shared      1	     -     0         -
io          -	     0     -         0
exclusive   0	     -     0         -
mark        -        0     -         0  */

// LATCH types

enum LATCH
{
	LATCH_none,
	LATCH_shared,
	LATCH_io,
	LATCH_exclusive,
	LATCH_mark
};



#ifdef SUPERSERVER_V2
#include "../jrd/os/pio.h"

// Constants used by prefetch mechanism

const int PREFETCH_MAX_TRANSFER	= 16384;	// maximum block I/O transfer (bytes)
// maximum pages allowed per prefetch request
const int PREFETCH_MAX_PAGES	= (2 * PREFETCH_MAX_TRANSFER / MIN_PAGE_SIZE);

// Prefetch block

class Prefetch : public pool_alloc<type_prf>
{
public:
	thread_db*	prf_tdbb;			// thread database context
	SLONG		prf_start_page;		// starting page of multipage prefetch
	USHORT		prf_max_prefetch;	// maximum no. of pages to prefetch
	USHORT		prf_page_count;		// actual no. of pages being prefetched
	phys_io_blk	prf_piob;			// physical I/O status block
	SCHAR*		prf_aligned_buffer;	// buffer address aligned for raw (OS cache bypass) I/O
	SCHAR*		prf_io_buffer;		// I/O buffer address
	UCHAR		prf_flags;
	BufferDesc*	prf_bdbs[PREFETCH_MAX_TRANSFER / MIN_PAGE_SIZE];
	SCHAR		prf_unaligned_buffer[PREFETCH_MAX_TRANSFER + MIN_PAGE_SIZE];
};

const int PRF_active	= 1;		// prefetch block currently in use
#endif // SUPERSERVER_V2

typedef Firebird::SortedArray<SLONG, Firebird::InlineStorage<SLONG, 256>, SLONG> PagesArray;


} //namespace Jrd

#endif // JRD_CCH_H
