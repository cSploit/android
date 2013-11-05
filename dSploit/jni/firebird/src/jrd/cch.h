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
#include "../jrd/os/pio.h"
#include "../common/classes/semaphore.h"
#ifdef SUPERSERVER_V2
#include "../jrd/sbm.h"
#include "../jrd/pag.h"
#endif

#include "../jrd/que.h"
#include "../jrd/lls.h"
#include "../jrd/pag.h"
#include "../jrd/isc.h"

//#define CCH_DEBUG

#ifdef CCH_DEBUG
DEFINE_TRACE_ROUTINE(cch_trace);
#define CCH_TRACE(args) cch_trace args
#define CCH_TRACE_AST(message) gds__trace(message)
#else
#define CCH_TRACE(args) 		// nothing
#define CCH_TRACEE_AST(message) // nothing
#endif

struct exp_index_buf;

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

#define DIRTY_LIST
//#define DIRTY_TREE

#ifdef DIRTY_TREE
// AVL-balanced tree node

struct BalancedTreeNode
{
	BufferDesc* bdb_node;
	SSHORT comp;
};
#endif // DIRTY_TREE

// BufferControl -- Buffer control block -- one per system

struct bcb_repeat
{
	BufferDesc*	bcb_bdb;		// Buffer descriptor block
	que			bcb_page_mod;	// Que of buffers with page mod n
};

class BufferControl : public pool_alloc_rpt<bcb_repeat, type_bcb>
{
public:
	explicit BufferControl(MemoryPool& p) : bcb_memory(p) { }
	UCharStack	bcb_memory;			// Large block partitioned into buffers
	que			bcb_in_use;			// Que of buffers in use
	que			bcb_empty;			// Que of empty buffers
#ifdef DIRTY_TREE
	BufferDesc*	bcb_btree;			// root of dirty page btree
#endif
#ifdef DIRTY_LIST
	que			bcb_dirty;			// que of dirty buffers
	SLONG		bcb_dirty_count;	// count of pages in dirty page btree
#endif
	Precedence*	bcb_free;			// Free precedence blocks
	que			bcb_free_lwt;		// Free latch wait blocks
	que			bcb_free_slt;		// Free shared latch blocks
	SSHORT		bcb_flags;			// see below
	SSHORT		bcb_free_minimum;	// Threshold to activate cache writer
	ULONG		bcb_count;			// Number of buffers allocated
	ULONG		bcb_checkpoint;		// Count of buffers to checkpoint
	ULONG		bcb_prec_walk_mark;	// mark value used in precedence graph walk
#ifdef SUPERSERVER_V2
	PageBitmap*	bcb_prefetch;		// Bitmap of pages to prefetch
#endif
	bcb_repeat	bcb_rpt[1];
};

const int BCB_keep_pages	= 1;	// set during btc_flush(), pages not removed from dirty binary tree
const int BCB_cache_writer	= 2;	// cache writer thread has been started
//const int BCB_checkpoint_db	= 4;	// WAL has requested a database checkpoint
const int BCB_writer_start  = 4;    // cache writer thread is starting now
const int BCB_writer_active	= 8;	// no need to post writer event count
#ifdef SUPERSERVER_V2
const int BCB_cache_reader	= 16;	// cache reader thread has been started
const int BCB_reader_active	= 32;	// cache reader not blocked on event
#endif
const int BCB_free_pending	= 64;	// request cache writer to free pages


// BufferDesc -- Buffer descriptor block

const int BDB_max_shared	= 20;	// maximum number of shared latch owners per BufferDesc

class BufferDesc : public pool_alloc<type_bdb>
{
public:
	BufferDesc() : bdb_page(0, 0) {}

	Database*	bdb_dbb;				// Database block (for ASTs)
	Lock*		bdb_lock;				// Lock block for buffer
	que			bdb_que;				// Buffer que
	que			bdb_in_use;				// queue of buffers in use
#ifdef DIRTY_LIST
	que			bdb_dirty;				// dirty pages LRU queue
#endif
	Ods::pag*	bdb_buffer;				// Actual buffer
	exp_index_buf*	bdb_expanded_buffer;	// expanded index buffer
	PageNumber	bdb_page;				// Database page number in buffer
	SLONG		bdb_incarnation;
	ULONG		bdb_transactions;		// vector of dirty flags to reduce commit overhead
	SLONG		bdb_mark_transaction;	// hi-water mark transaction to defer header page I/O
#ifdef DIRTY_TREE
	BufferDesc*	bdb_left;				// dirty page binary tree link
	BufferDesc*	bdb_right;				// dirty page binary tree link
	BufferDesc*	bdb_parent;				// dirty page binary tree link
	SSHORT		bdb_balance;			// AVL-tree balance (-1, 0, 1)
#endif
	que			bdb_lower;				// lower precedence que
	que			bdb_higher;				// higher precedence que
	que			bdb_waiters;			// latch wait que
	thread_db*	bdb_exclusive;			// thread holding exclusive latch
	thread_db*	bdb_io;					// thread holding io latch
	ULONG		bdb_ast_flags;			// flags manipulated at AST level
	USHORT		bdb_flags;
	SSHORT		bdb_use_count;			// Number of active users
	SSHORT		bdb_scan_count;			// concurrent sequential scans
	ULONG       bdb_difference_page;    // Number of page in difference file, NBAK
	ULONG		bdb_prec_walk_mark;		// mark value used in precedence graph walk
	que			bdb_shared;				// shared latches queue
};

// bdb_flags

// to set/clear BDB_dirty use set_dirty_flag()/clear_dirty_flag()
// These constants should really be of type USHORT.
const int BDB_dirty				= 1;		// page has been updated but not written yet
const int BDB_garbage_collect	= 2;		// left by scan for garbage collector
const int BDB_writer			= 4;		// someone is updating the page
const int BDB_marked			= 8;		// page has been updated
const int BDB_must_write		= 16;		// forces a write as soon as the page is released
const int BDB_faked				= 32;		// page was just allocated
//const int BDB_merge				= 64;
const int BDB_system_dirty 		= 128;		// system transaction has marked dirty
const int BDB_io_error	 		= 256;		// page i/o error
const int BDB_read_pending 		= 512;		// read is pending
const int BDB_free_pending 		= 1024;		// buffer being freed for reuse
const int BDB_not_valid			= 2048;		// i/o error invalidated buffer
const int BDB_db_dirty 			= 4096;		// page must be written to database
const int BDB_checkpoint		= 8192;		// page must be written by next checkpoint
const int BDB_prefetch			= 16384;	// page has been prefetched but not yet referenced
const int BDB_no_blocking_ast	= 32768;	// No blocking AST registered with page lock
// CVC: There's no more room for flags unless you change bdb_flags from USHORT to ULONG.

// bdb_ast_flags

const ULONG BDB_blocking 		= 1;	// a blocking ast was sent while page locked


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

// LWT -- Latch wait block

class LatchWait : public pool_alloc<type_lwt>
{
public:
	thread_db*			lwt_tdbb;
	LATCH				lwt_latch;		// latch type requested
	que					lwt_waiters;	// latch queue
	Firebird::Semaphore	lwt_sem;		// grant event to wait on
	USHORT				lwt_flags;
};

const int LWT_pending	= 1;			// latch request is pending

// Shared Latch
class SharedLatch
{
public:
	thread_db*	slt_tdbb;		// thread holding latch
	BufferDesc*	slt_bdb;		// buffer for which is this latch
	que			slt_tdbb_que;	// thread's latches queue
	que			slt_bdb_que;	// buffer's latches queue
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
