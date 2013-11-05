/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cch.cpp
 *	DESCRIPTION:	Disk cache manager
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */
#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../jrd/jrd.h"
#include "../jrd/que.h"
#include "../jrd/lck.h"
#include "../jrd/ods.h"
#include "../jrd/os/pio.h"
#include "../jrd/cch.h"
#include "gen/iberror.h"
#include "../jrd/lls.h"
#include "../jrd/req.h"
#include "../jrd/sdw.h"
#include "../jrd/tra.h"
#include "../jrd/sbm.h"
#include "../jrd/nbak.h"
#include "../jrd/gdsassert.h"
#include "../jrd/cch_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/shut_proto.h"
#include "../jrd/ThreadStart.h"
#include "../jrd/thread_proto.h"
#include "../jrd/tra_proto.h"
#include "../common/config/config.h"
#include "../common/classes/MsgPrint.h"

using namespace Jrd;
using namespace Ods;
using namespace Firebird;

// In the superserver mode, no page locks are acquired through the lock manager.
// Instead, a latching mechanism is used.  So the calls to lock subsystem for
// database pages in the original code should not be made, lest they should cause
// any undesirable side-effects.  The following defines help us achieve that.

#ifdef CCH_DEBUG
#include <stdarg.h>
IMPLEMENT_TRACE_ROUTINE(cch_trace, "CCH")
#endif

#ifdef SUPERSERVER
#define	CACHE_WRITER
#endif

#ifdef SUPERSERVER_V2
#define CACHE_READER
#endif

#ifdef SUPERSERVER
#define PAGE_LOCK_RELEASE(lock)
#define PAGE_LOCK_ASSERT(lock)
#define PAGE_LOCK_RE_POST(lock)
#define PAGE_OVERHEAD	(sizeof (bcb_repeat) + sizeof(BufferDesc) + \
			 (int) dbb->dbb_page_size)
#else
#define PAGE_LOCK_RELEASE(lock)			LCK_release (tdbb, lock)
#define PAGE_LOCK_ASSERT(lock)			LCK_assert (tdbb, lock)
#define PAGE_LOCK_RE_POST(lock)			LCK_re_post (tdbb, lock)
#define PAGE_OVERHEAD	(sizeof (bcb_repeat) + sizeof(BufferDesc) + \
			 sizeof (Lock) + (int) dbb->dbb_page_size)
#endif


static BufferDesc* alloc_bdb(thread_db*, BufferControl*, UCHAR **);
#ifndef SUPERSERVER
static Lock* alloc_page_lock(Jrd::thread_db*, BufferDesc*);
static int blocking_ast_bdb(void*);
#endif
#ifdef CACHE_READER
static THREAD_ENTRY_DECLARE cache_reader(THREAD_ENTRY_PARAM);
#endif
#ifdef CACHE_WRITER
static THREAD_ENTRY_DECLARE cache_writer(THREAD_ENTRY_PARAM);
#endif
static void check_precedence(thread_db*, WIN*, PageNumber);
static void clear_precedence(thread_db*, BufferDesc*);
static BufferDesc* dealloc_bdb(BufferDesc*);
#ifndef SUPERSERVER
static void down_grade(thread_db*, BufferDesc*);
#endif
static void expand_buffers(thread_db*, ULONG);
static BufferDesc* get_buffer(thread_db*, const PageNumber, LATCH, SSHORT);
static int get_related(BufferDesc*, PagesArray&, int, const ULONG);
static ULONG get_prec_walk_mark(BufferControl*);
static void invalidate_and_release_buffer(thread_db*, BufferDesc*);
static SSHORT latch_bdb(thread_db*, LATCH, BufferDesc*, const PageNumber, SSHORT);
static SSHORT lock_buffer(thread_db*, BufferDesc*, const SSHORT, const SCHAR);
static ULONG memory_init(thread_db*, BufferControl*, SLONG);
static void page_validation_error(thread_db*, win*, SSHORT);
#ifdef CACHE_READER
static void prefetch_epilogue(Prefetch*, ISC_STATUS *);
static void prefetch_init(Prefetch*, thread_db*);
static void prefetch_io(Prefetch*, ISC_STATUS *);
static void prefetch_prologue(Prefetch*, SLONG *);
#endif
static SSHORT related(BufferDesc*, const BufferDesc*, SSHORT, const ULONG);
static void release_bdb(thread_db*, BufferDesc*, const bool, const bool, const bool);
static void unmark(thread_db*, WIN*);
static bool writeable(BufferDesc*);
static bool is_writeable(BufferDesc*, const ULONG);
static int write_buffer(thread_db*, BufferDesc*, const PageNumber, const bool, ISC_STATUS* const, const bool);
static bool write_page(thread_db*, BufferDesc*, /*const bool,*/ ISC_STATUS* const, const bool);
static void set_diff_page(thread_db*, BufferDesc*);
static void set_dirty_flag(thread_db*, BufferDesc*);
static void clear_dirty_flag(thread_db*, BufferDesc*);

#ifdef DIRTY_LIST

static inline void insertDirty(BufferControl* bcb, BufferDesc* bdb)
{
	if (bdb->bdb_dirty.que_forward == &bdb->bdb_dirty)
	{
		bcb->bcb_dirty_count++;
		QUE_INSERT(bcb->bcb_dirty, bdb->bdb_dirty);
	}
}

static inline void removeDirty(BufferControl* bcb, BufferDesc* bdb)
{
	if (bdb->bdb_dirty.que_forward != &bdb->bdb_dirty)
	{
		fb_assert(bcb->bcb_dirty_count > 0);

		bcb->bcb_dirty_count--;
		QUE_DELETE(bdb->bdb_dirty);
		QUE_INIT(bdb->bdb_dirty);
	}
}

static void flushDirty(thread_db* tdbb, SLONG transaction_mask, const bool sys_only, ISC_STATUS* status);
static void flushAll(thread_db* tdbb, USHORT flush_flag);

#endif // DIRTY_LIST

#ifdef DIRTY_TREE

static void btc_flush(thread_db*, SLONG, const bool, ISC_STATUS*);

// comment this macro out to revert back to the old tree
#define BALANCED_DIRTY_PAGE_TREE

#ifdef BALANCED_DIRTY_PAGE_TREE
static void btc_insert_balanced(Database*, BufferDesc*);
static void btc_remove_balanced(BufferDesc*);
#define btc_insert btc_insert_balanced
#define btc_remove btc_remove_balanced

static bool btc_insert_balance(BufferDesc**, bool, SSHORT);
static bool btc_remove_balance(BufferDesc**, bool, SSHORT);
#else
static void btc_insert_unbalanced(Database*, BufferDesc*);
static void btc_remove_unbalanced(BufferDesc*);
#define btc_insert btc_insert_unbalanced
#define btc_remove btc_remove_unbalanced
#endif

const int BTREE_STACK_SIZE = 40;

#endif // DIRTY_TREE

const SLONG MIN_BUFFER_SEGMENT = 65536;

// Given pointer a field in the block, find the block

#define BLOCK(fld_ptr, type, fld) (type)((SCHAR*) fld_ptr - OFFSET (type, fld))

static inline SharedLatch* allocSharedLatch(thread_db* tdbb, BufferDesc* bdb)
{
	BufferControl* const bcb = bdb->bdb_dbb->dbb_bcb;
	SharedLatch* latch;
	if (QUE_NOT_EMPTY(bcb->bcb_free_slt))
	{
		QUE que_inst = bcb->bcb_free_slt.que_forward;
		QUE_DELETE(*que_inst);
		latch = BLOCK(que_inst, SharedLatch*, slt_bdb_que);
	}
	else
	{
		const int BATCH_ALLOC = 64;
		Database* dbb = bdb->bdb_dbb;

		SharedLatch* latches = latch = FB_NEW(*dbb->dbb_bufferpool) SharedLatch[BATCH_ALLOC];
		for (int i = 1; i < BATCH_ALLOC; i++) {
			QUE_APPEND(bcb->bcb_free_slt, latches[i].slt_bdb_que);
		}
	}

	latch->slt_bdb = bdb;
	QUE_APPEND(bdb->bdb_shared, latch->slt_bdb_que);

	latch->slt_tdbb = tdbb;
	QUE_APPEND(tdbb->tdbb_latches, latch->slt_tdbb_que);

	return latch;
}


static inline void freeSharedLatch(thread_db* tdbb, BufferControl* bcb, SharedLatch* latch)
{
	latch->slt_bdb = NULL;
	QUE_DELETE(latch->slt_bdb_que);
	QUE_INSERT(bcb->bcb_free_slt, latch->slt_bdb_que);

	latch->slt_tdbb = NULL;
	QUE_DELETE(latch->slt_tdbb_que);
}


static inline SharedLatch* findSharedLatch(thread_db* tdbb, BufferDesc* bdb)
{
	for (QUE que_inst = tdbb->tdbb_latches.que_forward; que_inst != &tdbb->tdbb_latches;
		 que_inst = que_inst->que_forward)
	{
		SharedLatch* latch = BLOCK(que_inst, SharedLatch*, slt_tdbb_que);
		fb_assert(latch->slt_tdbb == tdbb);
		if (latch->slt_bdb == bdb) {
			return latch;
		}
	}
	return NULL;
}


//
//#define BCB_MUTEX_ACQUIRE
//#define BCB_MUTEX_RELEASE
//
//#define PRE_MUTEX_ACQUIRE
//#define PRE_MUTEX_RELEASE
//
//#define BTC_MUTEX_ACQUIRE
//#define BTC_MUTEX_RELEASE
//
//#define LATCH_MUTEX_ACQUIRE
//#define LATCH_MUTEX_RELEASE
//

const PageNumber JOURNAL_PAGE(DB_PAGE_SPACE,	-1);
const PageNumber SHADOW_PAGE(DB_PAGE_SPACE,		-2);
const PageNumber FREE_PAGE(DB_PAGE_SPACE,		-3);
const PageNumber CHECKPOINT_PAGE(DB_PAGE_SPACE,	-4);
const PageNumber MIN_PAGE_NUMBER(DB_PAGE_SPACE,	-5);

const int PRE_SEARCH_LIMIT	= 256;
const int PRE_EXISTS		= -1;
const int PRE_UNKNOWN		= -2;

const int DUMMY_CHECKSUM	= 12345;


USHORT CCH_checksum(BufferDesc* bdb)
{
/**************************************
 *
 *	C C H _ c h e c k s u m
 *
 **************************************
 *
 * Functional description
 *	Compute the checksum of a page.
 *
 **************************************/
#ifdef NO_CHECKSUM
	return DUMMY_CHECKSUM;
#else
	Database* dbb = bdb->bdb_dbb;
#ifdef WIN_NT
	// ODS_VERSION8 for NT was shipped before page checksums
	// were disabled on other platforms. Continue to compute
	// checksums for ODS_VERSION8 databases but eliminate them
	// for ODS_VERSION9 databases. The following code can be
	// deleted when development on ODS_VERSION10 begins and
	// NO_CHECKSUM is defined for all platforms.

	if (dbb->dbb_ods_version >= ODS_VERSION9) {
		return DUMMY_CHECKSUM;
	}
#endif
	pag* page = bdb->bdb_buffer;

	const ULONG* const end = (ULONG *) ((SCHAR *) page + dbb->dbb_page_size);
	const USHORT old_checksum = page->pag_checksum;
	page->pag_checksum = 0;
	const ULONG* p = (ULONG *) page;
	ULONG checksum = 0;

	do {
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
	} while (p < end);

	page->pag_checksum = old_checksum;

	if (checksum) {
		return (USHORT) checksum;
	}

	// If the page is all zeros, return an artificial checksum

	for (p = (ULONG *) page; p < end;)
	{
		if (*p++)
			return (USHORT) checksum;
	}

	// Page is all zeros -- invent a checksum

	return 12345;
#endif
}


int CCH_down_grade_dbb(void* ast_object)
{
/**************************************
 *
 *	C C H _ d o w n _ g r a d e _ d b b
 *
 **************************************
 *
 * Functional description
 *	Down grade the lock on the database in response to a blocking
 *	AST.
 *
 **************************************/
	Database* dbb = static_cast<Database*>(ast_object);

	try
	{
		Database::SyncGuard dsGuard(dbb, true);

		if (dbb->dbb_flags & DBB_not_in_use)
			return 0;

		Lock* const lock = dbb->dbb_lock;

		// Since this routine will be called asynchronously,
		// we must establish a thread context
		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(lock->lck_attachment);

		dbb->dbb_ast_flags |= DBB_blocking;

		// Database shutdown will release the database lock; just return

		if (SHUT_blocking_ast(tdbb))
		{
			dbb->dbb_ast_flags &= ~DBB_blocking;
			return 0;
		}

		// Re-check the dbb state, as there could be a checkout inside the call above

		if (dbb->dbb_flags & DBB_not_in_use)
		{
			dbb->dbb_ast_flags &= ~DBB_blocking;
			return 0;
		}

		// If we are already shared, there is nothing more we can do.
		// If any case, the other guy probably wants exclusive access,
		// and we can't give it anyway

		if ((lock->lck_logical == LCK_SW) || (lock->lck_logical == LCK_SR)) {
			return 0;
		}

		if (dbb->dbb_flags & DBB_bugcheck)
		{
			LCK_convert(tdbb, lock, LCK_SW, LCK_WAIT);
			dbb->dbb_ast_flags &= ~DBB_blocking;
			return 0;
		}

		// If we are supposed to be exclusive, stay exclusive

		if ((dbb->dbb_flags & DBB_exclusive) || (dbb->dbb_ast_flags & DBB_shutdown_single)) {
			return 0;
		}

		// Assert any page locks that have been requested, but not asserted

		dbb->dbb_ast_flags |= DBB_assert_locks;
		BufferControl* bcb = dbb->dbb_bcb;
		if (bcb && bcb->bcb_count)
		{
			const bcb_repeat* tail = bcb->bcb_rpt;
			for (const bcb_repeat* const end = tail + bcb->bcb_count; tail < end; ++tail)
			{
				PAGE_LOCK_ASSERT(tail->bcb_bdb->bdb_lock);
			}
		}

		// Down grade the lock on the database itself

		if (lock->lck_physical == LCK_EX) {
			LCK_convert(tdbb, lock, LCK_PW, LCK_WAIT);	// This lets waiting cache manager in first
		}
		else {
			LCK_convert(tdbb, lock, LCK_SW, LCK_WAIT);
		}

		dbb->dbb_ast_flags &= ~DBB_blocking;
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


bool CCH_exclusive(thread_db* tdbb, USHORT level, SSHORT wait_flag)
{
/**************************************
 *
 *	C C H _ e x c l u s i v e
 *
 **************************************
 *
 * Functional description
 *	Get exclusive access to a database.  If we get it, return true.
 *	If the wait flag is FALSE, and we can't get it, give up and
 *	return false. There are two levels of database exclusivity: LCK_PW
 *	guarantees there are  no normal users in the database while LCK_EX
 *	additionally guarantes background database processes like the
 *	shared cache manager have detached.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef SUPERSERVER
	if (!CCH_exclusive_attachment(tdbb, level, wait_flag)) {
		return false;
	}
#endif
	Lock* lock = dbb->dbb_lock;
	if (!lock) {
		return false;
	}

	dbb->dbb_flags |= DBB_exclusive;

	switch (level)
	{
	case LCK_PW:
		if (lock->lck_physical >= LCK_PW || LCK_convert(tdbb, lock, LCK_PW, wait_flag))
		{
			return true;
		}
		break;

	case LCK_EX:
		if (lock->lck_physical == LCK_EX || LCK_convert(tdbb, lock, LCK_EX, wait_flag))
		{
			return true;
		}
		break;

	default:
		break;
	}

	// Clear the status vector, as our callers check the return value
	// and throw custom exceptions themselves
	fb_utils::init_status(tdbb->tdbb_status_vector);

	// If we are supposed to wait (presumably patiently),
	// but can't get the lock, generate an error

	if (wait_flag == LCK_WAIT) {
		ERR_post(Arg::Gds(isc_deadlock));
	}

	dbb->dbb_flags &= ~DBB_exclusive;

	return false;
}


bool CCH_exclusive_attachment(thread_db* tdbb, USHORT level, SSHORT wait_flag)
{
/**************************************
 *
 *	C C H _ e x c l u s i v e _ a t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 *	Get exclusive access to a database. If we get it, return true.
 *	If the wait flag is FALSE, and we can't get it, give up and
 *	return false.
 *
 **************************************/
	const int CCH_EXCLUSIVE_RETRY_INTERVAL = 1;	// retry interval in seconds

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();
	if (attachment->att_flags & ATT_exclusive) {
		return true;
	}

	attachment->att_flags |= (level == LCK_none) ? ATT_attach_pending : ATT_exclusive_pending;

	const SLONG timeout = (SLONG) (wait_flag < 0) ?
		-wait_flag : ((wait_flag == LCK_WAIT) ? 1L << 30 : CCH_EXCLUSIVE_RETRY_INTERVAL);

	// If requesting exclusive database access, then re-position attachment as the
	// youngest so that pending attachments may pass.

	if (level != LCK_none)
	{
		for (Attachment** ptr = &dbb->dbb_attachments; *ptr; ptr = &(*ptr)->att_next)
		{
			if (*ptr == attachment)
			{
				*ptr = attachment->att_next;
				break;
			}
		}
		attachment->att_next = dbb->dbb_attachments;
		dbb->dbb_attachments = attachment;
	}

	for (SLONG remaining = timeout; remaining > 0; remaining -= CCH_EXCLUSIVE_RETRY_INTERVAL)
	{
		if (tdbb->getAttachment()->att_flags & ATT_shutdown) {
			break;
		}

		bool found = false;
		for (attachment = tdbb->getAttachment()->att_next; attachment;
			attachment = attachment->att_next)
		{
			if (attachment->att_flags & ATT_shutdown) {
				continue;
			}

			if (level == LCK_none)
			{
				// Wait for other attachments requesting exclusive access
				if (attachment->att_flags & (ATT_exclusive | ATT_exclusive_pending))
				{
					found = true;
					break;
				}
				// Forbid multiple attachments in single-user maintenance mode
				if (attachment != tdbb->getAttachment() && (dbb->dbb_ast_flags & DBB_shutdown_single) )
				{
					found = true;
					break;
				}
			}
			else
			{
				// Requesting exclusive database access
				found = true;
				if (attachment->att_flags & ATT_exclusive_pending)
				{
					tdbb->getAttachment()->att_flags &= ~ATT_exclusive_pending;

					if (wait_flag == LCK_WAIT) {
						ERR_post(Arg::Gds(isc_deadlock));
					}
					else {
						return false;
					}
				}
				break;
			}
		}

		if (!found)
		{
			tdbb->getAttachment()->att_flags &= ~(ATT_exclusive_pending | ATT_attach_pending);
			if (level != LCK_none) {
				tdbb->getAttachment()->att_flags |= ATT_exclusive;
			}
			return true;
		}

		// Our thread needs to sleep for CCH_EXCLUSIVE_RETRY_INTERVAL seconds.

		if (remaining > CCH_EXCLUSIVE_RETRY_INTERVAL)
		{
			Database::Checkout dcoHolder(dbb);
			THREAD_SLEEP(CCH_EXCLUSIVE_RETRY_INTERVAL * 1000);
		}

		if (tdbb->getAttachment()->att_flags & ATT_cancel_raise)
		{
			if (JRD_reschedule(tdbb, 0, false))
			{
				tdbb->getAttachment()->att_flags &= ~(ATT_exclusive_pending | ATT_attach_pending);
				ERR_punt();
			}
		}
	}

	tdbb->getAttachment()->att_flags &= ~(ATT_exclusive_pending | ATT_attach_pending);
	return false;
}


void CCH_expand(thread_db* tdbb, ULONG number)
{
/**************************************
 *
 *	C C H _ e x p a n d
 *
 **************************************
 *
 * Functional description
 *	Expand the cache to at least a given number of buffers.  If
 *	it's already that big, don't do anything.
 *
 **************************************/
	SET_TDBB(tdbb);

	//BCB_MUTEX_ACQUIRE;
	expand_buffers(tdbb, number);
	//BCB_MUTEX_RELEASE;
}


pag* CCH_fake(thread_db* tdbb, WIN* window, SSHORT latch_wait)
{
/**************************************
 *
 *	C C H _ f a k e
 *
 **************************************
 *
 * Functional description
 *	Fake a fetch to a page.  Rather than reading it, however,
 *	zero it in memory.  This is used when allocating a new page.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				or an IO would be necessary, then give
 *				up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	pag pointer if successful.
 *	NULL pointer if timeout occurred (only possible if latch_wait <> 1).
 *	NULL pointer if latch_wait=0 and the faked page would have to be
 *			before reuse.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	CCH_TRACE(("FK %d:%06d", window->win_page.getPageSpaceID(), window->win_page.getPageNum()));

	// if there has been a shadow added recently, go out and
	// find it before we grant any more write locks

	if (dbb->dbb_ast_flags & DBB_get_shadows) {
		SDW_get_shadows(tdbb);
	}

	if (!BackupManager::StateReadGuard::lock(tdbb, latch_wait))
		return NULL;

	BufferDesc* bdb = get_buffer(tdbb, window->win_page, LATCH_exclusive, latch_wait);
	if (!bdb)
	{
		BackupManager::StateReadGuard::unlock(tdbb);
		return NULL;			// latch timeout occurred
	}

	// If a dirty orphaned page is being reused - better write it first
	// to clear current precedences and checkpoint state. This would also
	// update the bcb_free_pages field appropriately.

	if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))
	{
		// If the caller didn't want to wait at all, then
		// return 'try to fake an other page' to the caller.

		if (!latch_wait)
		{
			BackupManager::StateReadGuard::unlock(tdbb);
			release_bdb(tdbb, bdb, false, false, false);
			return NULL;
		}

		if (!write_buffer(tdbb, bdb, bdb->bdb_page, true, tdbb->tdbb_status_vector, true))
		{
			CCH_unwind(tdbb, true);
		}
	}
	else if (QUE_NOT_EMPTY(bdb->bdb_lower))
	{
		// Clear residual precedence left over from AST-level I/O.
		clear_precedence(tdbb, bdb);
	}

	// Here the page must not be dirty and have no backup lock owner
	fb_assert((bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)) == 0);

	bdb->bdb_flags = (BDB_writer | BDB_faked);
	bdb->bdb_scan_count = 0;

	lock_buffer(tdbb, bdb, LCK_WAIT, pag_undefined);

	MOVE_CLEAR(bdb->bdb_buffer, (SLONG) dbb->dbb_page_size);
	window->win_buffer = bdb->bdb_buffer;
	window->win_expanded_buffer = NULL;
	window->win_bdb = bdb;
	window->win_flags = 0;
	CCH_MARK(tdbb, window);

	return bdb->bdb_buffer;
}


pag* CCH_fetch(thread_db* tdbb, WIN* window, USHORT lock_type, SCHAR page_type, SSHORT checksum,
	SSHORT latch_wait, const bool read_shadow)
{
/**************************************
 *
 *	C C H _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	Fetch a specific page.  If it's already in cache,
 *	so much the better.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	PAG if successful.
 *	NULL pointer if timeout occurred (only possible if latch_wait <> 1).
 *
 **************************************/
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

	CCH_TRACE(("FE %d:%06d", window->win_page.getPageSpaceID(), window->win_page.getPageNum()));

	// FETCH_LOCK will return 0, 1, -1 or -2

	const SSHORT fetch_lock_return = CCH_FETCH_LOCK(tdbb, window, lock_type, latch_wait, page_type);

	switch (fetch_lock_return)
	{
	case 1:
		CCH_TRACE(("FE %d:%06d", window->win_page.getPageSpaceID(), window->win_page.getPageNum()));
		CCH_FETCH_PAGE(tdbb, window, checksum, read_shadow);	// must read page from disk
		break;
	case -2:
	case -1:
		return NULL;			// latch or lock timeout
	}

	BufferDesc* bdb = window->win_bdb;

	// If a page was read or prefetched on behalf of a large scan
	// then load the window scan count into the buffer descriptor.
	// This buffer scan count is decremented by releasing a buffer
	// with CCH_RELEASE_TAIL.

	// Otherwise zero the buffer scan count to prevent the buffer
	// from being queued to the LRU tail.

	if (window->win_flags & WIN_large_scan)
	{
		if (fetch_lock_return == 1 || bdb->bdb_flags & BDB_prefetch || bdb->bdb_scan_count < 0)
		{
			bdb->bdb_scan_count = window->win_scans;
		}
	}
	else if (window->win_flags & WIN_garbage_collector)
	{
		if (fetch_lock_return == 1) {
			bdb->bdb_scan_count = -1;
		}
		if (bdb->bdb_flags & BDB_garbage_collect) {
			window->win_flags |= WIN_garbage_collect;
		}
	}
	else if (window->win_flags & WIN_secondary)
	{
		if (fetch_lock_return == 1) {
			bdb->bdb_scan_count = -1;
		}
	}
	else
	{
		bdb->bdb_scan_count = 0;
		if (bdb->bdb_flags & BDB_garbage_collect) {
			bdb->bdb_flags &= ~BDB_garbage_collect;
		}
	}

	// Validate the fetched page matches the expected type

	if (bdb->bdb_buffer->pag_type != page_type && page_type != pag_undefined)
	{
		page_validation_error(tdbb, window, page_type);
	}

	return window->win_buffer;
}


SSHORT CCH_fetch_lock(thread_db* tdbb, WIN* window, USHORT lock_type, SSHORT wait, SCHAR page_type)
{
/**************************************
 *
 *	C C H _ f e t c h _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Fetch a latch and lock for a specific page.
 *
 * input
 *
 *	wait:
 *	LCK_WAIT (1)	=> Wait as long a necessary to get the lock.
 *		This can cause deadlocks of course.
 *
 *	LCK_NO_WAIT (0)	=>
 *		If the latch can't be acquired immediately, give up and return -2.
 *		If the lock can't be acquired immediately, give up and return -1.
 *
 *	<negative number>	=> Lock (latch) timeout interval in seconds.
 *
 * return
 *	0:	fetch & lock were successful, page doesn't need to be read.
 *	1:	fetch & lock were successful, page needs to be read from disk.
 *	-1:	lock timed out, fetch failed.
 *	-2:	latch timed out, fetch failed, lock not attempted.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// if there has been a shadow added recently, go out and
	// find it before we grant any more write locks

	if (dbb->dbb_ast_flags & DBB_get_shadows) {
		SDW_get_shadows(tdbb);
	}

	// Look for the page in the cache.

	if (!BackupManager::StateReadGuard::lock(tdbb, wait))
		return -2;

	BufferDesc* bdb = get_buffer(tdbb, window->win_page,
		((lock_type >= LCK_write) ? LATCH_exclusive : LATCH_shared), wait);

	if (wait != 1 && bdb == 0) 
	{
		BackupManager::StateReadGuard::unlock(tdbb);
		return -2; // latch timeout
	}

	if (lock_type >= LCK_write)
	{
		bdb->bdb_flags |= BDB_writer;
	}

	// the expanded index buffer is only good when the page is
	// fetched for read; if it is ever fetched for write, it must be discarded

	if (bdb->bdb_expanded_buffer && (lock_type > LCK_read))
	{
		delete bdb->bdb_expanded_buffer;
		bdb->bdb_expanded_buffer = NULL;
	}

	window->win_bdb = bdb;
	window->win_buffer = bdb->bdb_buffer;
	window->win_expanded_buffer = bdb->bdb_expanded_buffer;

	// lock_buffer returns 0 or 1 or -1.
	const SSHORT lock_result = lock_buffer(tdbb, bdb, wait, page_type);
	if (lock_result == -1)
		BackupManager::StateReadGuard::unlock(tdbb);

	return lock_result;
}

void CCH_fetch_page(thread_db* tdbb, WIN* window, SSHORT compute_checksum, const bool read_shadow)
{
/**************************************
 *
 *	C C H _ f e t c h _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Fetch a specific page.  If it's already in cache,
 *	so much the better.  When "compute_checksum" is 1, compute
 * 	the checksum of the page.  When it is 2, compute
 *	the checksum only when the page type is nonzero.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BufferDesc* bdb = window->win_bdb;

	ISC_STATUS* const status = tdbb->tdbb_status_vector;

	pag* page = bdb->bdb_buffer;
	bdb->bdb_incarnation = ++dbb->dbb_page_incarnation;

	tdbb->bumpStats(RuntimeStatistics::PAGE_READS);
	page = bdb->bdb_buffer;
	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(bdb->bdb_page.getPageSpaceID());
	fb_assert(pageSpace);
	jrd_file* file = pageSpace->file;
	SSHORT retryCount = 0;
	const bool isTempPage = pageSpace->isTemporary();

	/*
	We will read a page, and if there is an I/O error we will try to
	use the shadow file, and try reading again, for a maximum of
	3 tries, before it gives up.

	The read_shadow flag is set to false only in the call to
	FETCH_NO_SHADOW, which is only called from validate
	code.

	read_shadow = false -> IF an I/O error occurs give up (exit
	the loop, clean up, and return). So the caller,
	validate in most cases, can know about it and attempt
	to remedy the situation.

	read_shadow = true -> IF an I/O error occurs attempt
	to rollover to the shadow file.  If the I/O error is
	persistant (more than 3 times) error out of the routine by
	calling CCH_unwind, and eventually punting out.
	*/

	BackupManager* bm = dbb->dbb_backup_manager;
	const int bak_state = bm->getState();
	fb_assert(bak_state != nbak_state_unknown);

	ULONG diff_page = 0;
	if (!isTempPage && bak_state != nbak_state_normal)
	{
		BackupManager::AllocReadGuard allocGuard(tdbb, dbb->dbb_backup_manager);
		diff_page = bm->getPageIndex(tdbb, bdb->bdb_page.getPageNum());
		NBAK_TRACE(("Reading page %d:%06d, state=%d, diff page=%d",
			bdb->bdb_page.getPageSpaceID(), bdb->bdb_page.getPageNum(), bak_state, diff_page));
	}

	// In merge mode, if we are reading past beyond old end of file and page is in .delta file
	// then we maintain actual page in difference file. Always read it from there.
	if (isTempPage || bak_state == nbak_state_normal || !diff_page)
	{
		NBAK_TRACE(("Reading page %d:%06d, state=%d, diff page=%d from DISK",
			bdb->bdb_page.getPageSpaceID(), bdb->bdb_page.getPageNum(), bak_state, diff_page));

		// Read page from disk as normal
		while (!PIO_read(file, bdb, page, status))
		{
			if (isTempPage || !read_shadow) {
				break;
			}

			if (!CCH_rollover_to_shadow(tdbb, dbb, file, false))
			{
				PAGE_LOCK_RELEASE(bdb->bdb_lock);
				CCH_unwind(tdbb, true);
			}
			if (file != pageSpace->file) {
				file = pageSpace->file;
			}
			else
			{
				if (retryCount++ == 3)
				{
					fprintf(stderr, "IO error loop Unwind to avoid a hang\n");
					PAGE_LOCK_RELEASE(bdb->bdb_lock);
					CCH_unwind(tdbb, true);
				}
			}
		}
	}
	else
	{
		NBAK_TRACE(("Reading page %d, state=%d, diff page=%d from DIFFERENCE",
			bdb->bdb_page, bak_state, diff_page));
		if (!bm->readDifference(tdbb, diff_page, page))
		{
			PAGE_LOCK_RELEASE(bdb->bdb_lock);
			CCH_unwind(tdbb, true);
		}

		if (page->pag_checksum == 0)
		{
			// We encountered a page which was allocated, but never written to the
			// difference file. In this case we try to read the page from database. With
			// this approach if the page was old we get it from DISK, and if the page
			// was new IO error (EOF) or BUGCHECK (checksum error) will be the result.
			// Engine is not supposed to read a page which was never written unless
			// this is a merge process.
			NBAK_TRACE(("Re-reading page %d, state=%d, diff page=%d from DISK",
				bdb->bdb_page, bak_state, diff_page));
			while (!PIO_read(file, bdb, page, status))
			{
				if (!read_shadow) {
					break;
				}

				if (!CCH_rollover_to_shadow(tdbb, dbb, file, false))
				{
					PAGE_LOCK_RELEASE(bdb->bdb_lock);
					CCH_unwind(tdbb, true);
				}
				if (file != pageSpace->file) {
					file = pageSpace->file;
				}
				else
				{
					if (retryCount++ == 3)
					{
						fprintf(stderr, "IO error loop Unwind to avoid a hang\n");
						PAGE_LOCK_RELEASE(bdb->bdb_lock);
						CCH_unwind(tdbb, true);
					}
				}
			}
		}
	}

#ifndef NO_CHECKSUM
	if ((compute_checksum == 1 || (compute_checksum == 2 && page->pag_type)) &&
		page->pag_checksum != CCH_checksum(bdb) && !(dbb->dbb_flags & DBB_damaged))
	{
		ERR_build_status(status,
						 Arg::Gds(isc_db_corrupt) << Arg::Str("") <<	// why isn't the db name used here?
						 Arg::Gds(isc_bad_checksum) <<
						 Arg::Gds(isc_badpage) << Arg::Num(bdb->bdb_page.getPageNum()));
		// We should invalidate this bad buffer.

		PAGE_LOCK_RELEASE(bdb->bdb_lock);
		CCH_unwind(tdbb, true);
	}
#endif // NO_CHECKSUM

	bdb->bdb_flags &= ~(BDB_not_valid | BDB_read_pending);
	window->win_buffer = bdb->bdb_buffer;
}


void CCH_forget_page(thread_db* tdbb, WIN* window)
{
/**************************************
 *
 *	C C H _ f o r g e t _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Page was faked but can't be written on disk. Most probably because
 *	of out of disk space. Release page buffer and others resources and
 *	unlink page from various queues
 *
 **************************************/
	SET_TDBB(tdbb);
	BufferDesc* bdb = window->win_bdb;
	Database* dbb = tdbb->getDatabase();

	if (window->win_page != bdb->bdb_page ||
		bdb->bdb_buffer->pag_type != pag_undefined)
	{
		// buffer was reassigned or page was reused
		return;
	}

	window->win_bdb = NULL;
	if (tdbb->tdbb_flags & TDBB_no_cache_unwind) {
		release_bdb(tdbb, bdb, false, false, false);
	}

	if (bdb->bdb_flags & BDB_io_error)  {
		dbb->dbb_flags &= ~DBB_suspend_bgio;
	}

	clear_dirty_flag(tdbb, bdb);
	bdb->bdb_flags = 0;
	BufferControl* bcb = dbb->dbb_bcb;

#ifdef DIRTY_LIST
	removeDirty(bcb, bdb);
#endif
#ifdef DIRTY_TREE
	if (bdb->bdb_parent || (bdb == bcb->bcb_btree))
		btc_remove(bdb);
#endif

	QUE_DELETE(bdb->bdb_in_use);
	QUE_DELETE(bdb->bdb_que);
	QUE_INSERT(bcb->bcb_empty, bdb->bdb_que);
}


void CCH_fini(thread_db* tdbb)
{
/**************************************
 *
 *	C C H _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Shut down buffer operation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	bool flush_error = false;

	// CVC: Patching a conversion error FB1->FB2 with crude logic
	for (int i = 0; i < 2; ++i)
	{

		try {

		// If we've been initialized, either flush buffers
		// or release locks, depending on where we've been
		// bug-checked; as a defensive programming measure,
		// make sure that the buffers were actually allocated.

		bcb_repeat* tail;
		BufferControl* bcb = dbb->dbb_bcb;
		if (bcb && (tail = bcb->bcb_rpt) && (tail->bcb_bdb))
		{
			if (dbb->dbb_flags & DBB_bugcheck || flush_error)
			{
				for (const bcb_repeat* const end = bcb->bcb_rpt + bcb->bcb_count; tail < end; tail++)
				{
					BufferDesc* bdb = tail->bcb_bdb;
					delete bdb->bdb_expanded_buffer;
					bdb->bdb_expanded_buffer = NULL;
					PAGE_LOCK_RELEASE(bdb->bdb_lock);
				}
			}
			else {
				CCH_flush(tdbb, FLUSH_FINI, (SLONG) 0);
			}
		}


#ifdef CACHE_READER

		// Shutdown the dedicated cache reader for this database.

		if ((bcb = dbb->dbb_bcb) && (bcb->bcb_flags & BCB_cache_reader))
		{
			bcb->bcb_flags &= ~BCB_cache_reader;
			dbb->dbb_reader_sem.release();
			{ // scope
				Database::Checkout dcoHolder(dbb);
				dbb->dbb_reader_fini.enter();
			}
		}
#endif

#ifdef CACHE_WRITER

		// Wait for cache writer startup to complete.
		while ((bcb = dbb->dbb_bcb) && (bcb->bcb_flags & BCB_writer_start))
		{
			Database::Checkout dcoHolder(dbb);
			THREAD_YIELD();
		}

		// Shutdown the dedicated cache writer for this database.

		if ((bcb = dbb->dbb_bcb) && (bcb->bcb_flags & BCB_cache_writer))
		{
			bcb->bcb_flags &= ~BCB_cache_writer;
			dbb->dbb_writer_sem.release(); // Wake up running thread
			{ // scope
				Database::Checkout dcoHolder(dbb);
				dbb->dbb_writer_fini.enter();
			}
		}
#endif

		// close the database file and all associated shadow files

		//PIO_close(dbb->dbb_file);
		dbb->dbb_page_manager.closeAll();
		SDW_close();

		if ( (bcb = dbb->dbb_bcb) )
		{
			while (bcb->bcb_memory.hasData())
			{
				dbb->dbb_bufferpool->deallocate(bcb->bcb_memory.pop());
			}

			// Dispose off any associated latching semaphores
			while (QUE_NOT_EMPTY(bcb->bcb_free_lwt))
			{
				QUE que_inst = bcb->bcb_free_lwt.que_forward;
				QUE_DELETE(*que_inst);
				LatchWait* lwt = (LatchWait*) BLOCK(que_inst, LatchWait*, lwt_waiters);
				delete lwt;
			}
		}

		}	// try
		catch (const Firebird::Exception& ex)
		{
			Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
			if (!flush_error) {
				flush_error = true;
			}
			else {
				ERR_punt();
			}
		}

		if (!flush_error) { // wasn't set in the catch => no failure, just exit
			break;
		}

	} // for

}


void CCH_flush(thread_db* tdbb, USHORT flush_flag, SLONG tra_number)
{
/**************************************
 *
 *	C C H _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Flush all buffers.  If the release flag is set,
 *	release all locks.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	ISC_STATUS* status = tdbb->tdbb_status_vector;

	// note that some of the code for btc_flush()
	// replicates code in the for loop
	// to minimize call overhead -- changes should be made in both places

	if (flush_flag & (FLUSH_TRAN | FLUSH_SYSTEM))
	{
		const SLONG transaction_mask = tra_number ? 1L << (tra_number & (BITS_PER_LONG - 1)) : 0;
		bool sys_only = false;
		if (!transaction_mask && (flush_flag & FLUSH_SYSTEM)) {
			sys_only = true;
		}

#ifdef SUPERSERVER_V2
		BufferControl* bcb = dbb->dbb_bcb;
		//if (!dbb->dbb_wal && A && B) becomes
		//if (true && A && B) then finally (A && B)
		if (!(dbb->dbb_flags & DBB_force_write) && transaction_mask)
		{
			dbb->dbb_flush_cycle |= transaction_mask;
			if (!(bcb->bcb_flags & BCB_writer_active))
				dbb->dbb_writer_sem.release();
		}
		else
#endif

#ifdef DIRTY_LIST
			flushDirty(tdbb, transaction_mask, sys_only, status);
#endif
#ifdef DIRTY_TREE
			btc_flush(tdbb, transaction_mask, sys_only, status);
#endif
	}
	else
	{
#ifdef DIRTY_LIST
		flushAll(tdbb, flush_flag);
#endif
#ifdef DIRTY_TREE
		const bool all_flag = (flush_flag & FLUSH_ALL) != 0;
		const bool release_flag = (flush_flag & FLUSH_RLSE) != 0;
		const bool write_thru = release_flag;
		const bool sweep_flag = (flush_flag & FLUSH_SWEEP) != 0;
		LATCH latch = release_flag ? LATCH_exclusive : LATCH_none;

		BufferControl* bcb;
		for (ULONG i = 0; (bcb = dbb->dbb_bcb) && i < bcb->bcb_count; i++)
		{
			BufferDesc* bdb = bcb->bcb_rpt[i].bcb_bdb;
			if (!release_flag && !(bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)))
			{
				continue;
			}
			if (latch_bdb(tdbb, latch, bdb, bdb->bdb_page, 1) == -1)
			{
				BUGCHECK(302);	// msg 302 unexpected page change
			}
			if (bdb->bdb_use_count > 1)
				BUGCHECK(210);	// msg 210 page in use during flush
#ifdef SUPERSERVER
			if (bdb->bdb_flags & BDB_db_dirty)
			{
				if (all_flag || (sweep_flag && (!bdb->bdb_parent && bdb != bcb->bcb_btree)))
				{
					if (!write_buffer(tdbb, bdb, bdb->bdb_page, write_thru, status, true))
					{
						CCH_unwind(tdbb, true);
					}
				}
			}
#else
			if (bdb->bdb_flags & BDB_dirty)
			{
				if (!write_buffer(tdbb, bdb, bdb->bdb_page, false, status, true))
				{
					CCH_unwind(tdbb, true);
				}
			}
#endif
			if (release_flag)
			{
				PAGE_LOCK_RELEASE(bdb->bdb_lock);
			}
			release_bdb(tdbb, bdb, false, false, false);
		}
#endif // DIRTY_TREE
	}

	//
	// Check if flush needed
	//
	const int max_unflushed_writes = Config::getMaxUnflushedWrites();
	const time_t max_unflushed_write_time = Config::getMaxUnflushedWriteTime();
	bool max_num = (max_unflushed_writes >= 0);
	bool max_time = (max_unflushed_write_time >= 0);

	bool doFlush = false;

	PageSpace* pageSpaceID = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	jrd_file* main_file = pageSpaceID->file;

	if (!(main_file->fil_flags & FIL_force_write) && (max_num || max_time))
	{
		const time_t now = time(0);

		Database::CheckoutLockGuard guard(dbb, dbb->dbb_flush_count_mutex);

		// If this is the first commit set last_flushed_write to now
		if (!dbb->last_flushed_write)
		{
			dbb->last_flushed_write = now;
		}

		// test max_num condition and max_time condition
		max_num = max_num && (dbb->unflushed_writes == max_unflushed_writes);
		max_time = max_time && (now - dbb->last_flushed_write > max_unflushed_write_time);

		if (max_num || max_time)
		{
			doFlush = true;
			dbb->unflushed_writes = 0;
			dbb->last_flushed_write = now;
		}
		else
		{
			dbb->unflushed_writes++;
		}
	}

	if (doFlush)
	{
		PIO_flush(dbb, main_file);
		if (dbb->dbb_shadow)
		{
			PIO_flush(dbb, dbb->dbb_shadow->sdw_file);
		}

		BackupManager* bm = dbb->dbb_backup_manager;
		if (!bm->isShuttedDown())
		{
			BackupManager::StateReadGuard stateGuard(tdbb);
			const int backup_state = bm->getState();
			if (backup_state == nbak_state_stalled || backup_state == nbak_state_merge)
				bm->flushDifference();
		}

		tdbb->bumpStats(RuntimeStatistics::FLUSHES);
	}

	// take the opportunity when we know there are no pages
	// in cache to check that the shadow(s) have not been
	// scheduled for shutdown or deletion

	SDW_check(tdbb);
}

void CCH_flush_ast(thread_db* tdbb)
{
/**************************************
 *
 *	C C H _ f l u s h _ a s t
 *
 **************************************
 *
 * Functional description
 *	Flush all buffers coming from database file.
 *	Should be called from AST
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef SUPERSERVER
	CCH_flush(tdbb, FLUSH_ALL, 0);
#else
	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;
	// Do some fancy footwork to make sure that pages are
	// not removed from the btc tree at AST level.  Then
	// restore the flag to whatever it was before.
	const bool keep_pages = bcb->bcb_flags & BCB_keep_pages;
	dbb->dbb_bcb->bcb_flags |= BCB_keep_pages;

	for (ULONG i = 0; (bcb = dbb->dbb_bcb) && i < bcb->bcb_count; i++)
	{
		BufferDesc* bdb = bcb->bcb_rpt[i].bcb_bdb;
		if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))
			down_grade(tdbb, bdb);
	}

	if (!keep_pages) {
		bcb->bcb_flags &= ~BCB_keep_pages;
	}
#endif
}

bool CCH_free_page(thread_db* tdbb)
{
/**************************************
 *
 *	C C H _ f r e e _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Check if the cache is below its free pages
 *	threshold and write a page on the LRU tail.
 *
 **************************************/

	// Called by VIO/garbage_collector() when it is idle to
	// help quench the thirst for free pages.

	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;

	if (dbb->dbb_flags & DBB_read_only) {
		return false;
	}

	BufferDesc* bdb;
	if (bcb->bcb_flags & BCB_free_pending && (bdb = get_buffer(tdbb, FREE_PAGE, LATCH_none, 1)))
	{
		if (!write_buffer(tdbb, bdb, bdb->bdb_page, true, tdbb->tdbb_status_vector, true))
		{
			CCH_unwind(tdbb, false);
		}
		else
			return true;
	}

	return false;
}


SLONG CCH_get_incarnation(WIN* window)
{
/**************************************
 *
 *	C C H _ g e t _ i n c a r n a t i o n
 *
 **************************************
 *
 * Functional description
 *	Get page incarnation associated with buffer.
 *
 **************************************/
	return window->win_bdb->bdb_incarnation;
}


void CCH_get_related(thread_db* tdbb, PageNumber page, PagesArray &lowPages)
{
/**************************************
 *
 *	C C H _ g e t _ r e l a t e d
 *
 **************************************
 *
 * Functional description
 *	Collect all pages, dependent on given page (i.e. all pages which must be
 *  written after given page). To do it, walk low part of precedence graph
 *  starting from given page and put its numbers into array.
 *
 **************************************/

	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;
	QUE mod_que = &bcb->bcb_rpt[page.getPageNum() % bcb->bcb_count].bcb_page_mod;

	QUE que_inst;
	for (que_inst = mod_que->que_forward; que_inst != mod_que; que_inst = que_inst->que_forward)
	{
		BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_que);
		if (bdb->bdb_page == page)
		{
			const ULONG mark = get_prec_walk_mark(bcb);
			get_related(bdb, lowPages, PRE_SEARCH_LIMIT, mark);
			return;
		}
	}
}


pag* CCH_handoff(thread_db*	tdbb, WIN* window, SLONG page, SSHORT lock, SCHAR page_type,
	SSHORT latch_wait, const bool release_tail)
{
/**************************************
 *
 *	C C H _ h a n d o f f
 *
 **************************************
 *
 * Functional description
 *	Follow a pointer handing off the lock.  Fetch the new page
 *	before retiring the old page lock.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	PAG if successful.
 *	0 if a latch timeout occurred (only possible if latch_wait <> 1).
 *		The latch on the fetched page is downgraded to shared.
 *		The fetched page is unmarked.
 *
 **************************************/

	// The update, if there was one, of the input page is complete.
	// The cache buffer can be 'unmarked'.  It is important to
	// unmark before CCH_unwind is (might be) called.

	SET_TDBB(tdbb);

	CCH_TRACE(("H %d:%06d->%06d",
		window->win_page.getPageSpaceID(), window->win_page.getPageNum(), page));

	unmark(tdbb, window);

	// If the 'from-page' and 'to-page' of the handoff are the
	// same and the latch requested is shared then downgrade it.

	if ((window->win_page.getPageNum() == page) && (lock == LCK_read))
	{
		release_bdb(tdbb, window->win_bdb, false, true, false);
		return window->win_buffer;
	}

	WIN temp = *window;
	window->win_page = PageNumber(window->win_page.getPageSpaceID(), page);
	const SSHORT must_read = CCH_FETCH_LOCK(tdbb, window, lock, latch_wait, page_type);

	// Latch or lock timeout, return failure.

	if (must_read == -2 || must_read == -1)
	{
		*window = temp;
		CCH_RELEASE(tdbb, window);
		return NULL;
	}

	if (release_tail)
		CCH_RELEASE_TAIL(tdbb, &temp);
	else
		CCH_RELEASE(tdbb, &temp);

	if (must_read) {
		CCH_FETCH_PAGE(tdbb, window, 1, true);
	}

	BufferDesc* bdb = window->win_bdb;

	// If a page was read or prefetched on behalf of a large scan
	// then load the window scan count into the buffer descriptor.
	// This buffer scan count is decremented by releasing a buffer
	// with CCH_RELEASE_TAIL.

	// Otherwise zero the buffer scan count to prevent the buffer
	// from being queued to the LRU tail.

	if (window->win_flags & WIN_large_scan)
	{
		if (must_read == 1 || bdb->bdb_flags & BDB_prefetch || bdb->bdb_scan_count < 0)
		{
			bdb->bdb_scan_count = window->win_scans;
		}
	}
	else if (window->win_flags & WIN_garbage_collector)
	{
		if (must_read == 1) {
			bdb->bdb_scan_count = -1;
		}
		if (bdb->bdb_flags & BDB_garbage_collect) {
			window->win_flags |= WIN_garbage_collect;
		}
	}
	else if (window->win_flags & WIN_secondary)
	{
		if (must_read == 1) {
			bdb->bdb_scan_count = -1;
		}
	}
	else
	{
		bdb->bdb_scan_count = 0;
		if (bdb->bdb_flags & BDB_garbage_collect) {
			bdb->bdb_flags &= ~BDB_garbage_collect;
		}
	}

	// Validate the fetched page matches the expected type

	if (bdb->bdb_buffer->pag_type != page_type && page_type != pag_undefined)
	{
		page_validation_error(tdbb, window, page_type);
	}

	return window->win_buffer;
}


void CCH_init(thread_db* tdbb, ULONG number)
{
/**************************************
 *
 *	C C H _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize the cache.  Allocate buffers control block,
 *	buffer descriptors, and actual buffers.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	CCH_TRACE(("INIT %s", dbb->dbb_filename.c_str()));

	// Check for database-specific page buffers

	if (dbb->dbb_page_buffers) {
		number = dbb->dbb_page_buffers;
	}

	// Enforce page buffer cache constraints

	if (number < MIN_PAGE_BUFFERS) {
		number = MIN_PAGE_BUFFERS;
	}
	if (number > MAX_PAGE_BUFFERS) {
		number = MAX_PAGE_BUFFERS;
	}

	const SLONG count = number;

	// Allocate and initialize buffers control block
	BufferControl* bcb = 0;
	while (!bcb)
	{
		try {
			bcb = FB_NEW_RPT(*dbb->dbb_bufferpool, number) BufferControl(*dbb->dbb_bufferpool);
		}
		catch (const Firebird::Exception& ex)
		{
			Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
			// If the buffer control block can't be allocated, memory is
			// very low. Recalculate the number of buffers to account for
			// page buffer overhead and reduce that number by a 25% fudge factor.

			number = (sizeof(bcb_repeat) * number) / PAGE_OVERHEAD;
			number -= number >> 2;

			if (number < MIN_PAGE_BUFFERS) {
				ERR_post(Arg::Gds(isc_cache_too_small));
			}
		}
	}

	dbb->dbb_bcb = bcb;
	QUE_INIT(bcb->bcb_in_use);
#ifdef DIRTY_LIST
	QUE_INIT(bcb->bcb_dirty);
	bcb->bcb_dirty_count = 0;
#endif
	QUE_INIT(bcb->bcb_empty);
	QUE_INIT(bcb->bcb_free_lwt);
	QUE_INIT(bcb->bcb_free_slt);

	// initialization of memory is system-specific

	bcb->bcb_count = memory_init(tdbb, bcb, static_cast<SLONG>(number));
	bcb->bcb_free_minimum = (SSHORT) MIN(bcb->bcb_count / 4, 128);

	if (bcb->bcb_count < MIN_PAGE_BUFFERS) {
		ERR_post(Arg::Gds(isc_cache_too_small));
	}

	// Log if requested number of page buffers could not be allocated.

	if (count != (SLONG) bcb->bcb_count)
	{
		gds__log("Database: %s\n\tAllocated %ld page buffers of %ld requested",
			 tdbb->getAttachment()->att_filename.c_str(), bcb->bcb_count, count);
	}

	if (dbb->dbb_lock->lck_logical != LCK_EX) {
		dbb->dbb_ast_flags |= DBB_assert_locks;
	}

#ifdef CACHE_READER
	if (gds__thread_start(cache_reader, dbb, THREAD_high, 0, 0))
	{
		ERR_bugcheck_msg("cannot start thread");
	}

	{ // scope
		Database::Checkout dcoHolder(dbb);
		dbb->dbb_reader_init.enter();
	}
#endif

#ifdef CACHE_WRITER
	if (!(dbb->dbb_flags & DBB_read_only))
	{
		// writer startup in progress
		bcb->bcb_flags |= BCB_writer_start;

		if (gds__thread_start(cache_writer, dbb, THREAD_high, 0, 0))
		{
			bcb->bcb_flags &= ~BCB_writer_start;
			ERR_bugcheck_msg("cannot start thread");
		}
		{ // scope
			Database::Checkout dcoHolder(dbb);
			dbb->dbb_writer_init.enter();
		}
	}
#endif
}


void CCH_mark(thread_db* tdbb, WIN* window, USHORT mark_system, USHORT must_write)
{
/**************************************
 *
 *	C C H _ m a r k
 *
 **************************************
 *
 * Functional description
 *	Mark a window as dirty.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	tdbb->bumpStats(RuntimeStatistics::PAGE_MARKS);
	BufferControl* bcb = dbb->dbb_bcb;
	BufferDesc* bdb = window->win_bdb;
	BLKCHK(bdb, type_bdb);

	if (!(bdb->bdb_flags & BDB_writer)) {
		BUGCHECK(208);			// msg 208 page not accessed for write
	}

	CCH_TRACE(("M %d:%06d", window->win_page.getPageSpaceID(), window->win_page.getPageNum()));

	// A LATCH_mark is needed before the BufferDesc can be marked.
	// This prevents a write while the page is being modified.

	if (latch_bdb(tdbb, LATCH_mark, bdb, bdb->bdb_page, 1) == -1) {
		BUGCHECK(302);	// msg 302 unexpected page change
	}

	bdb->bdb_incarnation = ++dbb->dbb_page_incarnation;

	// mark the dirty bit vector for this specific transaction,
	// if it exists; otherwise mark that the system transaction
	// has updated this page

	SLONG number;
	jrd_tra* transaction = tdbb->getTransaction();
	if (transaction && (number = transaction->tra_number))
	{
		if (!(tdbb->tdbb_flags & TDBB_sweeper))
		{
			const ULONG trans_bucket = number & (BITS_PER_LONG - 1);
			bdb->bdb_transactions |= (1L << trans_bucket);
			if (number > bdb->bdb_mark_transaction) {
				bdb->bdb_mark_transaction = number;
			}
		}
	}
	else {
		bdb->bdb_flags |= BDB_system_dirty;
	}

	if (mark_system) {
		bdb->bdb_flags |= BDB_system_dirty;
	}

	if (!(tdbb->tdbb_flags & TDBB_sweeper) || bdb->bdb_flags & BDB_system_dirty)
	{
#ifdef DIRTY_LIST
		insertDirty(bcb, bdb);
#endif
#ifdef DIRTY_TREE
		if (!bdb->bdb_parent && bdb != bcb->bcb_btree) {
			btc_insert(dbb, bdb);
		}
#endif
	}

#ifdef SUPERSERVER
	bdb->bdb_flags |= BDB_db_dirty;
#endif

	bdb->bdb_flags |= BDB_marked;
	set_dirty_flag(tdbb, bdb);

	if (must_write || dbb->dbb_backup_manager->databaseFlushInProgress())
		bdb->bdb_flags |= BDB_must_write;

	set_diff_page(tdbb, bdb);
}


void CCH_must_write(WIN* window)
{
/**************************************
 *
 *	C C H _ m u s t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Mark a window as "must write".
 *
 **************************************/
	Jrd::thread_db* tdbb = NULL;
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

	BufferDesc* bdb = window->win_bdb;
	BLKCHK(bdb, type_bdb);

	if (!(bdb->bdb_flags & BDB_marked) || !(bdb->bdb_flags & BDB_dirty)) {
		BUGCHECK(208);			// msg 208 page not accessed for write
	}

	bdb->bdb_flags |= BDB_must_write;
	set_dirty_flag(tdbb, bdb);
}


void CCH_precedence(thread_db* tdbb, WIN* window, SLONG pageNum)
{
	const USHORT pageSpaceID = pageNum > LOG_PAGE ?
		window->win_page.getPageSpaceID() : DB_PAGE_SPACE;

	CCH_precedence(tdbb, window, PageNumber(pageSpaceID, pageNum));
}


void CCH_precedence(thread_db* tdbb, WIN* window, PageNumber page)
{
/**************************************
 *
 *	C C H _ p r e c e d e n c e
 *
 **************************************
 *
 * Functional description
 *	Given a window accessed for write and a page number,
 *	establish a precedence relationship such that the
 *	specified page will always be written before the page
 *	associated with the window.
 *
 *	If the "page number" is negative, it is really a transaction
 *	id.  In this case, the precedence relationship is to the
 *	database header page from which the transaction id was
 *	obtained.  If the header page has been written since the
 *	transaction id was assigned, no precedence relationship
 *	is required.
 *
 **************************************/
	// If the page is zero, the caller isn't really serious

	if (page.getPageNum() == 0) {
		return;
	}

	// no need to support precedence for temporary pages
	if (page.isTemporary() || window->win_page.isTemporary()) {
		return;
	}

	check_precedence(tdbb, window, page);
}


#ifdef CACHE_READER
void CCH_prefetch(thread_db* tdbb, SLONG * pages, SSHORT count)
{
/**************************************
 *
 *	C C H _ p r e f e t c h
 *
 **************************************
 *
 * Functional description
 *	Given a vector of pages, set corresponding bits
 *	in global prefetch bitmap. Initiate an asynchronous
 *	I/O and get the cache reader reading in our behalf
 *	as well.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;

	if (!count || !(bcb->bcb_flags & BCB_cache_reader))
	{
		// Caller isn't really serious.
		return;
	}

	// Switch default pool to permanent pool for setting bits in prefetch bitmap.
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_bufferpool);

	// The global prefetch bitmap is the key to the I/O coalescense mechanism which dovetails
	// all thread prefetch requests to minimize sequential I/O requests.
	// It also enables multipage I/O by implicitly sorting page vector requests.

	SLONG first_page = 0;
	for (const SLONG* const end = pages + count; pages < end;)
	{
		const SLONG page = *pages++;
		if (page)
		{
			SBM_set(tdbb, &bcb->bcb_prefetch, page);
			if (!first_page)
				first_page = page;
		}
	}

	// Not likely that the caller's page vector was empty but check anyway.

	if (first_page)
	{
		prf prefetch;

		prefetch_init(&prefetch, tdbb);
		prefetch_prologue(&prefetch, &first_page);
		prefetch_io(&prefetch, tdbb->tdbb_status_vector);
		prefetch_epilogue(&prefetch, tdbb->tdbb_status_vector);
	}
}


bool CCH_prefetch_pages(thread_db* tdbb)
{
/**************************************
 *
 *	C C H _ p r e f e t c h _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Check the prefetch bitmap for a set
 *	of pages and read them into the cache.
 *
 **************************************/

	// Placeholder to be implemented when predictive prefetch is
	// enabled. This is called by VIO/garbage_collector() when it
	// is idle to help satisfy the prefetch demand.

	return false;
}
#endif // CACHE_READER


void set_diff_page(thread_db* tdbb, BufferDesc* bdb)
{
	Database* const dbb = tdbb->getDatabase();
	BackupManager* const bm = dbb->dbb_backup_manager;

	// Determine location of the page in difference file and write destination
	// so BufferDesc AST handlers and write_page routine can safely use this information
	if (bdb->bdb_page != HEADER_PAGE_NUMBER)
	{
		// SCN of header page is adjusted in nbak.cpp
		bdb->bdb_buffer->pag_scn = bm->getCurrentSCN(); // Set SCN for the page
	}

	const int backup_state = bm->getState();

	if (backup_state == nbak_state_normal)
		return;

	// Temporary pages don't write to delta
	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(bdb->bdb_page.getPageSpaceID());
	fb_assert(pageSpace);
	if (pageSpace->isTemporary())
		return;

	switch (backup_state)
	{
	case nbak_state_stalled:
		{ //scope
			BackupManager::AllocReadGuard allocGuard(tdbb, bm);
			bdb->bdb_difference_page = bm->getPageIndex(tdbb, bdb->bdb_page.getPageNum());
		}
		if (!bdb->bdb_difference_page)
		{
			{ //scope
				BackupManager::AllocWriteGuard allocGuard(tdbb, bm);
				bdb->bdb_difference_page = bm->allocateDifferencePage(tdbb, bdb->bdb_page.getPageNum());
			}
			if (!bdb->bdb_difference_page)
			{
				invalidate_and_release_buffer(tdbb, bdb);
				CCH_unwind(tdbb, true);
			}
			NBAK_TRACE(("Allocate difference page %d for database page %d",
				bdb->bdb_difference_page, bdb->bdb_page));
		}
		else
		{
			NBAK_TRACE(("Map existing difference page %d to database page %d",
				bdb->bdb_difference_page, bdb->bdb_page));
		}
		break;
	case nbak_state_merge:
		{ //scope
			BackupManager::AllocReadGuard allocGuard(tdbb, bm);
			bdb->bdb_difference_page = bm->getPageIndex(tdbb, bdb->bdb_page.getPageNum());
		}
		if (bdb->bdb_difference_page)
		{
			NBAK_TRACE(("Map existing difference page %d to database page %d (write_both)",
				bdb->bdb_difference_page, bdb->bdb_page));
		}
		break;
	}
}


void CCH_release(thread_db* tdbb, WIN* window, const bool release_tail)
{
/**************************************
 *
 *	C C H _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release a window. If the release_tail
 *	flag is true then make the buffer
 *	least-recently-used.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	BufferDesc* const bdb = window->win_bdb;
	BLKCHK(bdb, type_bdb);

	CCH_TRACE(("R %d:%06d", window->win_page.getPageSpaceID(), window->win_page.getPageNum()));

	// if an expanded buffer has been created, retain it for possible future use

	bdb->bdb_expanded_buffer = window->win_expanded_buffer;
	window->win_expanded_buffer = NULL;

	// A large sequential scan has requested that the garbage
	// collector garbage collect. Mark the buffer so that the
	// page isn't released to the LRU tail before the garbage
	// collector can process the page.

	if (window->win_flags & WIN_large_scan && window->win_flags & WIN_garbage_collect)
	{
		bdb->bdb_flags |= BDB_garbage_collect;
		window->win_flags &= ~WIN_garbage_collect;
	}

	BackupManager::StateReadGuard::unlock(tdbb);

	if (bdb->bdb_use_count == 1)
	{
		const bool marked = bdb->bdb_flags & BDB_marked;
		bdb->bdb_flags &= ~(BDB_writer | BDB_marked | BDB_faked);

		if (marked)
		{
			release_bdb(tdbb, bdb, false, false, true);
		}

		if (bdb->bdb_flags & BDB_must_write)
		{
			// Downgrade exclusive latch to shared to allow concurrent share access
			// to page during I/O.

			release_bdb(tdbb, bdb, false, true, false);
			if (!write_buffer(tdbb, bdb, bdb->bdb_page, false, tdbb->tdbb_status_vector, true))
			{
#ifdef DIRTY_LIST
				insertDirty(dbb->dbb_bcb, bdb);
#endif
#ifdef DIRTY_TREE
				btc_insert(dbb, bdb);	// Don't lose track of must_write
#endif
				CCH_unwind(tdbb, true);
			}
		}

		if (bdb->bdb_flags & BDB_no_blocking_ast)
		{
			if (bdb->bdb_flags & (BDB_db_dirty | BDB_dirty))
			{
				if (!write_buffer(tdbb, bdb, bdb->bdb_page, false, tdbb->tdbb_status_vector, true))
				{
					// Reassert blocking AST after write failure with dummy lock convert
					// to same level. This will re-enable blocking AST notification.

					LCK_convert_opt(tdbb, bdb->bdb_lock,
									bdb->bdb_lock->lck_logical);
					CCH_unwind(tdbb, true);
				}
			}

			PAGE_LOCK_RELEASE(bdb->bdb_lock);
			bdb->bdb_flags &= ~BDB_no_blocking_ast;
			bdb->bdb_ast_flags &= ~BDB_blocking;
		}

		// Make buffer the least-recently-used by queueing it to the LRU tail

		if (release_tail)
		{
			if ((window->win_flags & WIN_large_scan && bdb->bdb_scan_count > 0 &&
				 	!(--bdb->bdb_scan_count) && !(bdb->bdb_flags & BDB_garbage_collect)) ||
				(window->win_flags & WIN_garbage_collector && bdb->bdb_flags & BDB_garbage_collect &&
				 	!bdb->bdb_scan_count))
			{
				if (window->win_flags & WIN_garbage_collector)
				{
					bdb->bdb_flags &= ~BDB_garbage_collect;
				}
				BufferControl* bcb = dbb->dbb_bcb;
				QUE_LEAST_RECENTLY_USED(bdb->bdb_in_use);
#ifdef CACHE_WRITER
				if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))
				{
#ifdef DIRTY_LIST
					if (bdb->bdb_dirty.que_forward != &bdb->bdb_dirty)
					{
						QUE_DELETE(bdb->bdb_dirty);
						QUE_APPEND(bcb->bcb_dirty, bdb->bdb_dirty);
					}
#endif
					bcb->bcb_flags |= BCB_free_pending;
					if (bcb->bcb_flags & BCB_cache_writer && !(bcb->bcb_flags & BCB_writer_active))
					{
						dbb->dbb_writer_sem.release();
					}
				}
#endif
			}
		}
	}

	release_bdb(tdbb, bdb, false, false, false);
	const SSHORT use_count = bdb->bdb_use_count;

	if (use_count < 0) {
		BUGCHECK(209);			// msg 209 attempt to release page not acquired
	}

	if (!use_count && (bdb->bdb_ast_flags & BDB_blocking))
	{
		PAGE_LOCK_RE_POST(bdb->bdb_lock);
	}

	window->win_bdb = NULL;

	fb_assert(bdb->bdb_use_count ? true : !(bdb->bdb_flags & BDB_marked))
}


void CCH_release_exclusive(thread_db* tdbb)
{
/**************************************
 *
 *	C C H _ r e l e a s e _ e x c l u s i v e
 *
 **************************************
 *
 * Functional description
 *	Release exclusive access to database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	dbb->dbb_flags &= ~DBB_exclusive;

	Attachment* attachment = tdbb->getAttachment();
	if (attachment) {
		attachment->att_flags &= ~ATT_exclusive;
	}

	if (dbb->dbb_ast_flags & DBB_blocking) {
		LCK_re_post(tdbb, dbb->dbb_lock);
	}
}


bool CCH_rollover_to_shadow(thread_db* tdbb, Database* dbb, jrd_file* file, const bool inAst)
{
/**************************************
 *
 *	C C H _ r o l l o v e r _ t o _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	An I/O error has been detected on the
 *	main database file.  Roll over to use
 *	the shadow file.
 *
 **************************************/

	SET_TDBB(tdbb);

	// Is the shadow subsystem yet initialized
	if (!dbb->dbb_shadow_lock) {
		return false;
	}

	// hvlad: if there are no shadows can't rollover
	// this is a temporary solution to prevent 100% CPU load
	// in write_page in case of PIO_write failure
	if (!dbb->dbb_shadow) {
		return false;
	}

	// notify other process immediately to ensure all read from sdw
	// file instead of db file
	return SDW_rollover_to_shadow(tdbb, file, inAst);
}


void CCH_shutdown_database(Database* dbb)
{
/**************************************
 *
 *	C C H _ s h u t d o w n _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Shutdown database physical page locks.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	bcb_repeat* tail;
	BufferControl* bcb = dbb->dbb_bcb;
	if (bcb && (tail = bcb->bcb_rpt) && (tail->bcb_bdb))
	{
		for (const bcb_repeat* const end = tail + bcb->bcb_count; tail < end; tail++)
		{
			BufferDesc* bdb = tail->bcb_bdb;
			bdb->bdb_flags &= ~BDB_db_dirty;
			clear_dirty_flag(tdbb, bdb);
			PAGE_LOCK_RELEASE(bdb->bdb_lock);
		}
	}

#ifndef SUPERSERVER
	PageSpace* pageSpaceID = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	PIO_close(pageSpaceID->file);
	SDW_close();
#endif
}

void CCH_unwind(thread_db* tdbb, const bool punt)
{
/**************************************
 *
 *	C C H _ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Synchronously unwind cache after I/O or lock error.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// CCH_unwind is called when any of the following occurs:
	// - IO error
	// - journaling error => obsolete
	// - bad page checksum => obsolete
	// - wrong page type
	// - page locking (not latching) deadlock

	BufferControl* bcb = dbb->dbb_bcb;
	if (!bcb || (tdbb->tdbb_flags & TDBB_no_cache_unwind))
	{
		if (punt) {
			ERR_punt();
		}
		else {
			return;
		}
	}

	// A cache error has occurred. Scan the cache for buffers
	// which may be in use and release them.

	bcb_repeat* tail = bcb->bcb_rpt;
	for (const bcb_repeat* const end = tail + bcb->bcb_count; tail < end; tail++)
	{
		BufferDesc* bdb = tail->bcb_bdb;
		if (!bdb->bdb_use_count) {
			continue;
		}
		if (bdb->bdb_io == tdbb) {
			release_bdb(tdbb, bdb, true, false, false);
		}
		if (bdb->bdb_exclusive == tdbb)
		{
			if (bdb->bdb_flags & BDB_marked) {
				BUGCHECK(268);	// msg 268 buffer marked during cache unwind
			}
			BackupManager::StateReadGuard::unlock(tdbb);

			bdb->bdb_flags &= ~(BDB_writer | BDB_faked | BDB_must_write);
			release_bdb(tdbb, bdb, true, false, false);
		}

		// hvlad : as far as I understand thread can't hold more than two shared latches
		// on the same bdb, so findSharedLatch below will not be called many times
		SharedLatch* latch = findSharedLatch(tdbb, bdb);
		while (latch)
		{
			BackupManager::StateReadGuard::unlock(tdbb);

			release_bdb(tdbb, bdb, true, false, false);
			latch = findSharedLatch(tdbb, bdb);
		}
#ifndef SUPERSERVER
		const pag* const page = bdb->bdb_buffer;
		if (page->pag_type == pag_header || page->pag_type == pag_transactions)
		{
			++bdb->bdb_use_count;
			clear_dirty_flag(tdbb, bdb);
			bdb->bdb_flags &= ~(BDB_writer | BDB_marked | BDB_faked | BDB_db_dirty);
			PAGE_LOCK_RELEASE(bdb->bdb_lock);
			--bdb->bdb_use_count;
		}
#endif
	}

	if (punt) {
		ERR_punt();
	}
}


bool CCH_validate(WIN* window)
{
/**************************************
 *
 *	C C H _ v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Give a page a quick once over looking for unhealthyness.
 *
 **************************************/
	BufferDesc* bdb = window->win_bdb;

	// If page is marked for write, checksum is questionable

	if ((bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))) {
		return true;
	}

	pag* page = window->win_buffer;
	const USHORT sum = CCH_checksum(bdb);

	if (sum == page->pag_checksum) {
		return true;
	}

	return false;
}


bool CCH_write_all_shadows(thread_db* tdbb, Shadow* shadow, BufferDesc* bdb,
	ISC_STATUS* status, USHORT checksum, const bool inAst)
{
/**************************************
 *
 *	C C H _ w r i t e _ a l l _ s h a d o w s
 *
 **************************************
 *
 * Functional description
 *	Compute a checksum and write a page out to all shadows
 *	detecting failure on write.
 *	If shadow is null, write to all shadows, otherwise only to specified
 *	shadow.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	Shadow* sdw = shadow ? shadow : dbb->dbb_shadow;

	if (!sdw) {
		return true;
	}

	bool result = true;
	Firebird::UCharBuffer spare_buffer;

	pag* page;
	pag* old_buffer = NULL;
	if (bdb->bdb_page == HEADER_PAGE_NUMBER)
	{
		page = (pag*) spare_buffer.getBuffer(dbb->dbb_page_size);
		memcpy(page, bdb->bdb_buffer, HDR_SIZE);
		old_buffer = bdb->bdb_buffer;
		bdb->bdb_buffer = page;
	}
	else
	{
		page = bdb->bdb_buffer;
		if (checksum) {
			page->pag_checksum = CCH_checksum(bdb);
		}
	}

	for (; sdw; sdw = sdw->sdw_next)
	{
		// don't bother to write to the shadow if it is no longer viable

		/* Fix for bug 7925. drop_gdb fails to remove secondary file if
		   the shadow is conditional. Reason being the header page not
		   being correctly initialized.

		   The following block was not being performed for a conditional
		   shadow since SDW_INVALID expanded to include conditional shadow

		   -Sudesh  07/10/95
		   old code --> if (sdw->sdw_flags & SDW_INVALID)
		 */

		if ((sdw->sdw_flags & SDW_INVALID) && !(sdw->sdw_flags & SDW_conditional))
		{
			continue;
		}

		if (bdb->bdb_page == HEADER_PAGE_NUMBER)
		{
			// fixup header for shadow file
			jrd_file* shadow_file = sdw->sdw_file;
			header_page* header = (header_page*) page;

			PageSpace* pageSpaceID = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			const UCHAR* q = (UCHAR *) pageSpaceID->file->fil_string;
			header->hdr_data[0] = HDR_end;
			header->hdr_end = HDR_SIZE;
			header->hdr_next_page = 0;

			PAG_add_header_entry(tdbb, header, HDR_root_file_name,
								 (USHORT) strlen((const char*) q), q);

			jrd_file* next_file = shadow_file->fil_next;
			if (next_file)
			{
				q = (UCHAR *) next_file->fil_string;
				const SLONG last = next_file->fil_min_page - 1;
				PAG_add_header_entry(tdbb, header, HDR_file, (USHORT) strlen((const char*) q), q);
				PAG_add_header_entry(tdbb, header, HDR_last_page, sizeof(last), (const UCHAR*) &last);
			}

			header->hdr_flags |= hdr_active_shadow;
			header->hdr_header.pag_checksum = CCH_checksum(bdb);
		}

		// This condition makes sure that PIO_write is performed in case of
		// conditional shadow only if the page is Header page
		//
		// -Sudesh 07/10/95

		if ((sdw->sdw_flags & SDW_conditional) && bdb->bdb_page != HEADER_PAGE_NUMBER)
		{
			continue;
		}

		// if a write failure happens on an AUTO shadow, mark the
		// shadow to be deleted at the next available opportunity when we
		// know we don't have a page fetched

		if (!PIO_write(sdw->sdw_file, bdb, page, status))
		{
			if (sdw->sdw_flags & SDW_manual) {
				result = false;
			}
			else
			{
				sdw->sdw_flags |= SDW_delete;
				if (!inAst && SDW_check_conditional(tdbb))
				{
					if (SDW_lck_update(tdbb, 0))
					{
						SDW_notify(tdbb);
						CCH_unwind(tdbb, false);
						SDW_dump_pages(tdbb);
						ERR_post(Arg::Gds(isc_deadlock));
					}
				}
			}
		}
		// If shadow was specified, break out of loop

		if (shadow) {
			break;
		}
	}

	if (bdb->bdb_page == HEADER_PAGE_NUMBER) {
		bdb->bdb_buffer = old_buffer;
	}

	return result;
}


static BufferDesc* alloc_bdb(thread_db* tdbb, BufferControl* bcb, UCHAR** memory)
{
/**************************************
 *
 *	a l l o c _ b d b
 *
 **************************************
 *
 * Functional description
 *	Allocate buffer descriptor block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	BufferDesc* bdb = FB_NEW(*dbb->dbb_bufferpool) BufferDesc;
	bdb->bdb_dbb = dbb;

#ifndef SUPERSERVER
	try {
		bdb->bdb_lock = alloc_page_lock(tdbb, bdb);
	}
	catch (const Firebird::Exception&)
	{
		delete bdb;
		throw;
	}
#endif

	bdb->bdb_buffer = (pag*) *memory;
	*memory += dbb->dbb_page_size;

	QUE_INIT(bdb->bdb_higher);
	QUE_INIT(bdb->bdb_lower);
	QUE_INIT(bdb->bdb_waiters);
	QUE_INIT(bdb->bdb_shared);
	QUE_INSERT(bcb->bcb_empty, bdb->bdb_que);
#ifdef DIRTY_LIST
	QUE_INIT(bdb->bdb_dirty);
#endif

	return bdb;
}


#ifndef SUPERSERVER
static Lock* alloc_page_lock(thread_db* tdbb, BufferDesc* bdb)
{
/**************************************
 *
 *	a l l o c _ p a g e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Allocate a page-type lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	const SSHORT lockLen = PageNumber::getLockLen();
	Lock* lock = FB_NEW_RPT(*dbb->dbb_bufferpool, lockLen) Lock;
	lock->lck_type = LCK_bdb;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_length = lockLen;

	lock->lck_dbb = dbb;
	lock->lck_parent = dbb->dbb_lock;

	lock->lck_ast = blocking_ast_bdb;
	lock->lck_object = bdb;

	return lock;
}


static int blocking_ast_bdb(void* ast_object)
{
/**************************************
 *
 *	b l o c k i n g _ a s t _ b d b
 *
 **************************************
 *
 * Functional description
 *	Blocking AST for buffer control blocks.  This is called at
 *	AST (signal) level to indicate that a lock is blocking another
 *	process.  If the BufferDesc* is in use, set a flag indicating that the
 *	lock is blocking and return.  When the BufferDesc is released at normal
 *	level the lock will be down graded.  If the BufferDesc* is not in use,
 *	write the page if dirty then downgrade lock.  Things would be
 *	much hairier if UNIX supported asynchronous IO, but it doesn't.
 *	WHEW!
 *
 **************************************/
	BufferDesc* bdb = static_cast<BufferDesc*>(ast_object);

	try
	{
		Database* dbb = bdb->bdb_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		// Since this routine will be called asynchronously,
		// we must establish a thread context
		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);

		// Do some fancy footwork to make sure that pages are
		// not removed from the btc tree at AST level. Then
		// restore the flag to whatever it was before.

		const bool keep_pages = (dbb->dbb_bcb->bcb_flags & BCB_keep_pages) != 0;
		dbb->dbb_bcb->bcb_flags |= BCB_keep_pages;

		down_grade(tdbb, bdb);

		if (!keep_pages) {
			dbb->dbb_bcb->bcb_flags &= ~BCB_keep_pages;
		}

		if (tdbb->tdbb_status_vector[1]) {
			gds__log_status(dbb->dbb_filename.c_str(), tdbb->tdbb_status_vector);
		}
	}
	catch (const Firebird::Exception&)
	{} // no-op

    return 0;
}
#endif

#ifdef DIRTY_LIST

// Used in qsort below
extern "C" {
static int cmpBdbs(const void* a, const void* b)
{
	const BufferDesc* bdbA = *(BufferDesc**) a;
	const BufferDesc* bdbB = *(BufferDesc**) b;

	if (bdbA->bdb_page > bdbB->bdb_page)
		return 1;

	if (bdbA->bdb_page < bdbB->bdb_page)
		return -1;

	return 0;
}
} // extern C


// Remove cleared precedence blocks from high precedence queue
static void purgePrecedence(BufferControl* bcb, BufferDesc* bdb)
{
	QUE que_prec = bdb->bdb_higher.que_forward, next_prec;
	for (; que_prec != &bdb->bdb_higher; que_prec = next_prec)
	{
		next_prec = que_prec->que_forward;

		Precedence* precedence = BLOCK(que_prec, Precedence*, pre_higher);
		if (precedence->pre_flags & PRE_cleared)
		{
			QUE_DELETE(precedence->pre_higher);
			QUE_DELETE(precedence->pre_lower);
			precedence->pre_hi = (BufferDesc*) bcb->bcb_free;
			bcb->bcb_free = precedence;
		}
	}
}

// Write pages modified by given or system transaction to disk. First sort all
// corresponding pages by their numbers to make writes physically ordered and
// thus faster. At every iteration of while loop write pages which have no high
// precedence pages to ensure order preserved. If after some iteration there are
// no such pages (i.e. all of not written yet pages have high precedence pages)
// then write them all at last iteration (of course write_buffer will also check
// for precedence before write)
static void flushDirty(thread_db* tdbb, SLONG transaction_mask, const bool sys_only,
	ISC_STATUS* status)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;
	Firebird::HalfStaticArray<BufferDesc*, 1024> flush;

	QUE que_inst = bcb->bcb_dirty.que_forward, next;
	for (; que_inst != &bcb->bcb_dirty; que_inst = next)
	{
		next = que_inst->que_forward;
		BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_dirty);

		if (!(bdb->bdb_flags & BDB_dirty))
		{
			removeDirty(bcb, bdb);
			continue;
		}

		if ((transaction_mask & bdb->bdb_transactions) || (bdb->bdb_flags & BDB_system_dirty) ||
			(!transaction_mask && !sys_only) || (!bdb->bdb_transactions))
		{
			flush.add(bdb);
		}
	}

	qsort(flush.begin(), flush.getCount(), sizeof(BufferDesc*), cmpBdbs);

	bool writeAll = false;
	while (flush.getCount())
	{
		BufferDesc** ptr = flush.begin();
		const size_t cnt = flush.getCount();

		while (ptr < flush.end())
		{
			BufferDesc* bdb = *ptr;

			if (!writeAll) {
				purgePrecedence(bcb, bdb);
			}
			if (writeAll || QUE_EMPTY(bdb->bdb_higher))
			{
				const PageNumber page = bdb->bdb_page;

				if (!write_buffer(tdbb, bdb, page, false, status, true)) {
					CCH_unwind(tdbb, true);
				}

				// re-post the lock only if it was really written
				if ((bdb->bdb_ast_flags & BDB_blocking) && !(bdb->bdb_flags & BDB_dirty))
				{
					PAGE_LOCK_RE_POST(bdb->bdb_lock);
				}

				flush.remove(ptr);
			}
			else
				ptr++;
		}
		if (cnt == flush.getCount())
			writeAll = true;
	}
}


// Write pages modified by garbage collector or all dirty pages or release page
// locks - depending of flush_flag. See also comments in flushDirty
static void flushAll(thread_db* tdbb, USHORT flush_flag)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;
	ISC_STATUS* status = tdbb->tdbb_status_vector;
	Firebird::HalfStaticArray<BufferDesc*, 1024> flush(bcb->bcb_dirty_count);

#ifdef SUPERSERVER
	const bool all_flag = (flush_flag & FLUSH_ALL) != 0;
	const bool sweep_flag = (flush_flag & FLUSH_SWEEP) != 0;
#endif
	const bool release_flag = (flush_flag & FLUSH_RLSE) != 0;
	const bool write_thru = release_flag;

	const LATCH latch = release_flag ? LATCH_exclusive : LATCH_none;

	for (ULONG i = 0; (bcb = dbb->dbb_bcb) && i < bcb->bcb_count; i++)
	{
		BufferDesc* bdb = bcb->bcb_rpt[i].bcb_bdb;

#ifdef SUPERSERVER
		if (bdb->bdb_flags & BDB_db_dirty)
		{
			// pages modified by sweep\garbage collector are not in dirty list
			const bool dirty_list = (bdb->bdb_dirty.que_forward != &bdb->bdb_dirty);

			if (all_flag || (sweep_flag && !dirty_list)) {
				flush.add(bdb);
			}
		}
#else
		if (bdb->bdb_flags & BDB_dirty) {
			flush.add(bdb);
		}
#endif
		else if (release_flag)
		{
			if (latch_bdb(tdbb, latch, bdb, bdb->bdb_page, 1) == -1) {
				BUGCHECK(302);	// msg 302 unexpected page change
			}
			if (bdb->bdb_use_count > 1) {
				BUGCHECK(210);	// msg 210 page in use during flush
			}
			PAGE_LOCK_RELEASE(bdb->bdb_lock);
			release_bdb(tdbb, bdb, false, false, false);
		}
	}

	qsort(flush.begin(), flush.getCount(), sizeof(BufferDesc*), cmpBdbs);

	bool writeAll = false;
	while (flush.getCount())
	{
		BufferDesc** ptr = flush.begin();
		const size_t cnt = flush.getCount();
		while (ptr < flush.end())
		{
			BufferDesc* bdb = *ptr;

			if (!writeAll) {
				purgePrecedence(bcb, bdb);
			}
			if (writeAll || QUE_EMPTY(bdb->bdb_higher))
			{
				if (release_flag)
				{
					if (latch_bdb(tdbb, latch, bdb, bdb->bdb_page, 1) == -1) {
						BUGCHECK(302);	// msg 302 unexpected page change
					}
					if (bdb->bdb_use_count > 1) {
						BUGCHECK(210);	// msg 210 page in use during flush
					}
				}
				if (bdb->bdb_flags & (BDB_db_dirty | BDB_dirty))
				{
					if (!write_buffer(tdbb, bdb, bdb->bdb_page, write_thru, status, true))
					{
						CCH_unwind(tdbb, true);
					}
				}
				if (release_flag)
				{
					PAGE_LOCK_RELEASE(bdb->bdb_lock);
					release_bdb(tdbb, bdb, false, false, false);
				}
				else // re-post the lock if it was written
				if ((bdb->bdb_ast_flags & BDB_blocking) && !(bdb->bdb_flags & BDB_dirty))
				{
					PAGE_LOCK_RE_POST(bdb->bdb_lock);
				}
				flush.remove(ptr);
			}
			else
				ptr++;
		}
		if (cnt == flush.getCount())
			writeAll = true;
	}
}
#endif // DIRTY_LIST

#ifdef DIRTY_TREE

static void btc_flush(thread_db* tdbb, SLONG transaction_mask, const bool sys_only,
	ISC_STATUS* status)
{
/**************************************
 *
 *	b t c _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Walk the dirty page binary tree, flushing all buffers
 *	that could have been modified by this transaction.
 *	The pages are flushed in page order to roughly
 *	emulate an elevator-type disk controller. Iteration
 *	is used to minimize call overhead.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// traverse the tree, flagging to prevent pages
	// from being removed from the tree during write_page() --
	// this simplifies worrying about random pages dropping
	// out when dependencies have been set up

	PageNumber max_seen = MIN_PAGE_NUMBER;

	// Pick starting place at leftmost node

	//BTC_MUTEX_ACQUIRE;
	BufferDesc* next = dbb->dbb_bcb->bcb_btree;
	while (next && next->bdb_left) {
		 next = next->bdb_left;
	}

	PageNumber next_page = ZERO_PAGE_NUMBER;
	if (next) {
		next_page = next->bdb_page;
	}

	// Walk tree.  If we get lost, reposition and continue

	BufferDesc* bdb;
	while ( (bdb = next) )
	{
		// If we're lost, reposition

		if ((bdb->bdb_page != next_page) || (!bdb->bdb_parent && (bdb != dbb->dbb_bcb->bcb_btree)))
		{
			for (bdb = dbb->dbb_bcb->bcb_btree; bdb;)
			{
				if (bdb->bdb_left && (max_seen < bdb->bdb_page)) {
					bdb = bdb->bdb_left;
				}
				else if (bdb->bdb_right && (max_seen > bdb->bdb_page)) {
					bdb = bdb->bdb_right;
				}
				else {
					break;
				}
			}
			if (!bdb) {
				break;
			}
		}

		// Decide where to go next.  The options are (right, then down to the left) or up

		if (bdb->bdb_right && (max_seen < bdb->bdb_right->bdb_page))
		{
			for (next = bdb->bdb_right; next->bdb_left;
				 next = next->bdb_left);
		}
		else {
			next = bdb->bdb_parent;
		}

		if (next) {
			next_page = next->bdb_page;
		}

		if (max_seen >= bdb->bdb_page) {
			continue;
		}

		max_seen = bdb->bdb_page;

		// forget about this page if it was written out
		// as a dependency while we were walking the tree

		if (!(bdb->bdb_flags & BDB_dirty))
		{
			//BTC_MUTEX_RELEASE;
			btc_remove(bdb);
			//BTC_MUTEX_ACQUIRE;
			continue;
		}

		// this code replicates code in CCH_flush() -- changes should be made in both places

		const PageNumber page = bdb->bdb_page;
		//BTC_MUTEX_RELEASE;

		// if any transaction has dirtied this page, check to see if it could have been this one

		if ((transaction_mask & bdb->bdb_transactions) || (bdb->bdb_flags & BDB_system_dirty) ||
			(!transaction_mask && !sys_only) || (!bdb->bdb_transactions))
		{
			if (!write_buffer(tdbb, bdb, page, false, status, true)) {
				CCH_unwind(tdbb, true);
			}
		}

		// re-post the lock only if it was really written

		if ((bdb->bdb_ast_flags & BDB_blocking) && !(bdb->bdb_flags & BDB_dirty))
		{
			PAGE_LOCK_RE_POST(bdb->bdb_lock);
		}
		//BTC_MUTEX_ACQUIRE;
	}

	//BTC_MUTEX_RELEASE;
}


#ifdef BALANCED_DIRTY_PAGE_TREE
static void btc_insert_balanced(Database* dbb, BufferDesc* bdb)
{
/**************************************
 *
 *	b t c _ i n s e r t _ b a l a n c e d
 *
 **************************************
 *
 * Functional description
 *	Insert a buffer into the dirty page
 *	AVL-binary tree.
 *
 **************************************/

	// avoid recursion when rebalancing tree
	// (40 - enough to hold 2^32 nodes)
	BalancedTreeNode stack[BTREE_STACK_SIZE];

	// if the page is already in the tree (as in when it is
	// written out as a dependency while walking the tree),
	// just leave well enough alone -- this won't check if
	// it's at the root but who cares then

	if (bdb->bdb_parent) {
		return;
	}

	SET_DBB(dbb);

	// if the tree is empty, this is now the tree

	//BTC_MUTEX_ACQUIRE;
	BufferDesc* p = dbb->dbb_bcb->bcb_btree;
	if (!p)
	{
		dbb->dbb_bcb->bcb_btree = bdb;
		bdb->bdb_parent = bdb->bdb_left = bdb->bdb_right = NULL;
		bdb->bdb_balance = 0;
		//BTC_MUTEX_RELEASE;
		return;
	}

	// insert the page sorted by page number;
	// do this iteratively to minimize call overhead

	const PageNumber page = bdb->bdb_page;

	// find where new node should fit in tree

	int stackp = -1;
	SSHORT comp = 0;

	while (p)
	{
		if (page == p->bdb_page)
		{
			comp = 0;
		}
		else if (page > p->bdb_page)
		{
			comp = 1;
		}
		else
		{
			comp = -1;
		}

		if (comp == 0)
		{
			//BTC_MUTEX_RELEASE;
			return;
		} // already in the tree

		stackp++;
		fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
		stack[stackp].bdb_node = p;
		stack[stackp].comp = comp;

		p = (comp > 0) ? p->bdb_right : p->bdb_left;
	}

	// insert new node

	fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
	if (comp > 0)
	{
		stack[stackp].bdb_node->bdb_right = bdb;
	}
	else
	{
		stack[stackp].bdb_node->bdb_left = bdb;
	}

	bdb->bdb_parent = stack[stackp].bdb_node;
	bdb->bdb_left = bdb->bdb_right = NULL;
	bdb->bdb_balance = 0;

	// unwind the stack and rebalance

	bool subtree = true;

	while (stackp >= 0 && subtree)
	{
		fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
		if (stackp == 0)
		{
			subtree = btc_insert_balance(&dbb->dbb_bcb->bcb_btree, subtree, stack[0].comp);
		}
		else
		{
			if (stack[stackp - 1].comp > 0)
			{
				subtree = btc_insert_balance(&stack[stackp - 1].bdb_node->bdb_right,
											 subtree, stack[stackp].comp);
			}
			else
			{
				subtree = btc_insert_balance(&stack[stackp - 1].bdb_node->bdb_left,
											 subtree, stack[stackp].comp);
			}
		}
		stackp--;
	}
	//BTC_MUTEX_RELEASE;
}
#endif //BALANCED_DIRTY_PAGE_TREE


static bool btc_insert_balance(BufferDesc** bdb, bool subtree, SSHORT comp)
{
/**************************************
 *
 *	b t c _ i n s e r t _ b a l a n c e
 *
 **************************************
 *
 * Functional description
 *	Rebalance the AVL-binary tree.
 *
 **************************************/

	BufferDesc* p = *bdb;

	if (p->bdb_balance == -comp)
	{
		p->bdb_balance = 0;
		subtree = false;
	}
	else
	{
	    if (p->bdb_balance == 0)
		{
			p->bdb_balance = comp;
		}
		else
		{
			BufferDesc *p1, *p2;
			if (comp > 0)
			{
				p1 = p->bdb_right;

				if (p1->bdb_balance == comp)
				{
					if ( (p->bdb_right = p1->bdb_left) )
					{
						p1->bdb_left->bdb_parent = p;
					}

					p1->bdb_left = p;
					p1->bdb_parent = p->bdb_parent;
					p->bdb_parent = p1;
					p->bdb_balance = 0;
					p = p1;
				}
				else
				{
					p2 = p1->bdb_left;

					if ( (p1->bdb_left = p2->bdb_right) )
					{
						p2->bdb_right->bdb_parent = p1;
					}

					p2->bdb_right = p1;
					p1->bdb_parent = p2;

					if ( (p->bdb_right = p2->bdb_left) )
					{
						p2->bdb_left->bdb_parent = p;
					}

					p2->bdb_left = p;
					p2->bdb_parent = p->bdb_parent;
					p->bdb_parent = p2;

					if (p2->bdb_balance == comp)
					{
						p->bdb_balance = -comp;
					}
					else
					{
						p->bdb_balance = 0;
					}

					if (p2->bdb_balance == -comp)
					{
						p1->bdb_balance = comp;
					}
					else
					{
						p1->bdb_balance = 0;
					}

					p = p2;
	            }
		    }
			else
			{
				p1 = p->bdb_left;

				if (p1->bdb_balance == comp)
				{
					if ( (p->bdb_left = p1->bdb_right) )
					{
						p1->bdb_right->bdb_parent = p;
					}

					p1->bdb_right = p;
					p1->bdb_parent = p->bdb_parent;
					p->bdb_parent = p1;
					p->bdb_balance = 0;
					p = p1;
				}
				else
				{
					p2 = p1->bdb_right;

					if ( (p1->bdb_right = p2->bdb_left) )
					{
						p2->bdb_left->bdb_parent = p1;
					}

					p2->bdb_left = p1;
					p1->bdb_parent = p2;

					if ( (p->bdb_left = p2->bdb_right) )
					{
						p2->bdb_right->bdb_parent = p;
					}

					p2->bdb_right = p;
					p2->bdb_parent = p->bdb_parent;
					p->bdb_parent = p2;

					if (p2->bdb_balance == comp)
					{
						p->bdb_balance = -comp;
					}
					else
					{
						p->bdb_balance = 0;
					}

					if (p2->bdb_balance == -comp)
					{
						p1->bdb_balance = comp;
					}
					else
					{
						p1->bdb_balance = 0;
					}

					p = p2;
	            }
	        }
			p->bdb_balance = 0;
			subtree = false;
			*bdb = p;
		}
	}

	return subtree;
}


#ifndef BALANCED_DIRTY_PAGE_TREE
static void btc_insert_unbalanced(Database* dbb, BufferDesc* bdb)
{
/**************************************
 *
 *	b t c _ i n s e r t _ u n b a l a n c e d
 *
 **************************************
 *
 * Functional description
 *	Insert a buffer into the dirty page
 *	binary tree.
 *
 **************************************/

	// if the page is already in the tree (as in when it is
	// written out as a dependency while walking the tree),
	// just leave well enough alone -- this won't check if
	// it's at the root but who cares then

	if (bdb->bdb_parent) {
		return;
	}

	SET_DBB(dbb);

	// if the tree is empty, this is now the tree

	//BTC_MUTEX_ACQUIRE;
	BufferDesc* node = dbb->dbb_bcb->bcb_btree;
	if (!node)
	{
		dbb->dbb_bcb->bcb_btree = bdb;
		//BTC_MUTEX_RELEASE;
		return;
	}

	// insert the page sorted by page number; do this iteratively to minimize call overhead

	const PageNumber page = bdb->bdb_page;

	while (true)
	{
		if (page == node->bdb_page) {
			break;
		}

		if (page < node->bdb_page)
		{
			if (!node->bdb_left)
			{
				node->bdb_left = bdb;
				bdb->bdb_parent = node;
				break;
			}

			node = node->bdb_left;
		}
		else
		{
			if (!node->bdb_right)
			{
				node->bdb_right = bdb;
				bdb->bdb_parent = node;
				break;
			}

			node = node->bdb_right;
		}
	}

	//BTC_MUTEX_RELEASE;
}
#endif //!BALANCED_DIRTY_PAGE_TREE


#ifdef BALANCED_DIRTY_PAGE_TREE
static void btc_remove_balanced(BufferDesc* bdb)
{
/**************************************
 *
 *	b t c _ r e m o v e _ b a l a n c e d
 *
 **************************************
 *
 * Functional description
 * 	Remove a page from the dirty page
 *  AVL-binary tree.
 *
 **************************************/

	// avoid recursion when rebalancing tree
	// (40 - enough to hold 2^32 nodes)
	BalancedTreeNode stack[BTREE_STACK_SIZE];

	Database* dbb = bdb->bdb_dbb;

	// engage in a little defensive programming to make sure the node is actually in the tree

	//BTC_MUTEX_ACQUIRE;
	BufferControl* bcb = dbb->dbb_bcb;

	if (!bcb->bcb_btree ||
		(!bdb->bdb_parent && !bdb->bdb_left && !bdb->bdb_right && bcb->bcb_btree != bdb))
	{
		if ((bdb->bdb_flags & BDB_must_write) || !(bdb->bdb_flags & BDB_dirty))
		{
			// Must writes aren't worth the effort
			//BTC_MUTEX_RELEASE;
			return;
		}

		BUGCHECK(211);
		// msg 211 attempt to remove page from dirty page list when not there
	}

	// stack the way to node from root

	const PageNumber page = bdb->bdb_page;

	BufferDesc* p = bcb->bcb_btree;
	int stackp = -1;
	SSHORT comp;

	while (true)
	{
		if (page == p->bdb_page)
		{
			comp = 0;
		}
		else if (page > p->bdb_page)
		{
			comp = 1;
		}
		else
		{
			comp = -1;
		}

		stackp++;
		fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);

		if (comp == 0)
		{
			stack[stackp].bdb_node = p;
			stack[stackp].comp = -1;
			break;
		}

		stack[stackp].bdb_node = p;
		stack[stackp].comp = comp;

		p = (comp > 0) ? p->bdb_right : p->bdb_left;

		// node not found, bad tree
		if (!p)
		{
			BUGCHECK(211);
		}
	}

	// wrong node found, bad tree

	if (bdb != p)
	{
		BUGCHECK(211);
	}

	// delete node

	if (!bdb->bdb_right || !bdb->bdb_left)
	{
		// node has at most one branch
		stackp--;
		p = bdb->bdb_right ? bdb->bdb_right : bdb->bdb_left;

		if (stackp == -1)
		{
			if ( (bcb->bcb_btree = p) )
			{
				p->bdb_parent = NULL;
			}
		}
		else
		{
			fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
			if (stack[stackp].comp > 0)
			{
                stack[stackp].bdb_node->bdb_right = p;
			}
			else
			{
				stack[stackp].bdb_node->bdb_left = p;
			}

			if (p)
			{
				p->bdb_parent = stack[stackp].bdb_node;
			}
		}
	}
	else
	{
		// node has two branches, stack nodes to reach one with no right child

		p = bdb->bdb_left;

		if (!p->bdb_right)
		{
			if (stack[stackp].comp > 0)
			{
				BUGCHECK(211);
			}

			if ( (p->bdb_parent = bdb->bdb_parent) )
			{
				if (p->bdb_parent->bdb_right == bdb)
				{
					p->bdb_parent->bdb_right = p;
				}
				else
				{
					p->bdb_parent->bdb_left = p;
				}
			}
			else
			{
				bcb->bcb_btree = p;	// new tree root
			}

			if ( (p->bdb_right = bdb->bdb_right) )
			{
				bdb->bdb_right->bdb_parent = p;
			}

			p->bdb_balance = bdb->bdb_balance;
		}
		else
		{
			const int stackp_save = stackp;

			while (p->bdb_right)
			{
				stackp++;
				fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
				stack[stackp].bdb_node = p;
				stack[stackp].comp = 1;
				p = p->bdb_right;
			}

			if (p->bdb_parent = bdb->bdb_parent)
			{
				if (p->bdb_parent->bdb_right == bdb)
				{
					p->bdb_parent->bdb_right = p;
				}
				else
				{
					p->bdb_parent->bdb_left = p;
				}
			}
			else
			{
				bcb->bcb_btree = p;	// new tree root
			}

			if ( (stack[stackp].bdb_node->bdb_right = p->bdb_left) )
			{
				p->bdb_left->bdb_parent = stack[stackp].bdb_node;
			}

			if ( (p->bdb_left = bdb->bdb_left) )
			{
				p->bdb_left->bdb_parent = p;
			}

			if ( (p->bdb_right = bdb->bdb_right) )
			{
				p->bdb_right->bdb_parent = p;
			}

			p->bdb_balance = bdb->bdb_balance;
			stack[stackp_save].bdb_node = p; // replace BufferDesc in stack
		}
	}

	// unwind the stack and rebalance

	bool subtree = true;

	while (stackp >= 0 && subtree)
	{
		fb_assert(stackp >= 0 && stackp < BTREE_STACK_SIZE);
		if (stackp == 0)
		{
			subtree = btc_remove_balance(&bcb->bcb_btree, subtree, stack[0].comp);
		}
		else
		{
			if (stack[stackp - 1].comp > 0)
			{
				subtree = btc_remove_balance(&stack[stackp - 1].bdb_node->bdb_right,
											 subtree, stack[stackp].comp);
			}
			else
			{
				subtree = btc_remove_balance(&stack[stackp - 1].bdb_node->bdb_left,
											 subtree, stack[stackp].comp);
			}
		}
		stackp--;
	}

	// initialize the node for next usage

	bdb->bdb_left = bdb->bdb_right = bdb->bdb_parent = NULL;
	//BTC_MUTEX_RELEASE;
}
#endif //BALANCED_DIRTY_PAGE_TREE


static bool btc_remove_balance(BufferDesc** bdb, bool subtree, SSHORT comp)
{
/**************************************
 *
 *	b t c _ r e m o v e _ b a l a n c e
 *
 **************************************
 *
 * Functional description
 * 	Rebalance the AVL-binary tree.
 *
 **************************************/

	BufferDesc* p = *bdb;

	if (p->bdb_balance == comp)
	{
		p->bdb_balance = 0;
	}
	else
	{
		if (p->bdb_balance == 0)
		{
			p->bdb_balance = -comp;
			subtree = false;
        }
		else
		{
			BufferDesc *p1, *p2;
			if (comp < 0)
			{
				p1 = p->bdb_right;
				const SSHORT b1 = p1->bdb_balance;

				if ((b1 == 0) || (b1 == -comp))
				{
					// single RR or LL rotation

					if ( (p->bdb_right = p1->bdb_left) )
					{
						p1->bdb_left->bdb_parent = p;
					}

					p1->bdb_left = p;
					p1->bdb_parent = p->bdb_parent;
					p->bdb_parent = p1;

					if (b1 == 0)
					{
						p->bdb_balance = -comp;
						p1->bdb_balance = comp;
						subtree = false;
					}
					else
					{
						p->bdb_balance = 0;
						p1->bdb_balance = 0;
					}

					p = p1;
				}
				else
				{
					// double RL or LR rotation

					p2 = p1->bdb_left;
					const SSHORT b2 = p2->bdb_balance;

					if ( (p1->bdb_left = p2->bdb_right) )
					{
						p2->bdb_right->bdb_parent = p1;
					}

					p2->bdb_right = p1;
					p1->bdb_parent = p2;

					if ( (p->bdb_right = p2->bdb_left) )
					{
						p2->bdb_left->bdb_parent = p;
					}

					p2->bdb_left = p;
					p2->bdb_parent = p->bdb_parent;
					p->bdb_parent = p2;

					if (b2 == -comp)
					{
						p->bdb_balance = comp;
					}
					else
					{
						p->bdb_balance = 0;
					}

					if (b2 == comp)
					{
						p1->bdb_balance = -comp;
					}
					else
					{
						p1->bdb_balance = 0;
					}

					p = p2;
					p2->bdb_balance = 0;
				}
			}
			else
			{
				p1 = p->bdb_left;
				const SSHORT b1 = p1->bdb_balance;

				if ((b1 == 0) || (b1 == -comp))
				{
					// single RR or LL rotation

					if ( (p->bdb_left = p1->bdb_right) )
					{
						p1->bdb_right->bdb_parent = p;
					}

					p1->bdb_right = p;
					p1->bdb_parent = p->bdb_parent;
					p->bdb_parent = p1;

					if (b1 == 0)
					{
						p->bdb_balance = -comp;
						p1->bdb_balance = comp;
						subtree = false;
					}
					else
					{
						p->bdb_balance = 0;
						p1->bdb_balance = 0;
					}

					p = p1;
				}
				else
				{
					// double RL or LR rotation

					p2 = p1->bdb_right;
					const SSHORT b2 = p2->bdb_balance;

					if ( (p1->bdb_right = p2->bdb_left) )
					{
						p2->bdb_left->bdb_parent = p1;
					}

					p2->bdb_left = p1;
					p1->bdb_parent = p2;

					if ( (p->bdb_left = p2->bdb_right) )
					{
						p2->bdb_right->bdb_parent = p;
					}

					p2->bdb_right = p;
					p2->bdb_parent = p->bdb_parent;
					p->bdb_parent = p2;

					if (b2 == -comp)
					{
						p->bdb_balance = comp;
					}
					else
					{
						p->bdb_balance = 0;
					}

					if (b2 == comp)
					{
						p1->bdb_balance = -comp;
					}
					else
					{
						p1->bdb_balance = 0;
					}

					p = p2;
					p2->bdb_balance = 0;
				}
			}

			*bdb = p;
		}
	}

	return subtree;
}


#ifndef BALANCED_DIRTY_PAGE_TREE
static void btc_remove_unbalanced(BufferDesc* bdb)
{
/**************************************
 *
 *	b t c _ r e m o v e _ u n b a l a n c e d
 *
 **************************************
 *
 * Functional description
 * 	Remove a page from the dirty page binary tree.
 *	The idea is to place the left child of this
 *	page in this page's place, then make the
 *	right child of this page a child of the left
 *	child -- this removal mechanism won't promote
 *	a balanced tree but that isn't of primary
 *	importance.
 *
 **************************************/
	Database* dbb = bdb->bdb_dbb;

	// engage in a little defensive programming to make sure the node is actually in the tree

	//BTC_MUTEX_ACQUIRE;
	BufferControl* bcb = dbb->dbb_bcb;
	if (!bcb->bcb_btree ||
		(!bdb->bdb_parent && !bdb->bdb_left && !bdb->bdb_right && bcb->bcb_btree != bdb))
	{
		if ((bdb->bdb_flags & BDB_must_write) || !(bdb->bdb_flags & BDB_dirty))
		{
			// Must writes aren't worth the effort
			//BTC_MUTEX_RELEASE;
			return;
		}

		BUGCHECK(211);
		// msg 211 attempt to remove page from dirty page list when not there
	}

	// make a new child out of the left and right children

	BufferDesc* new_child = bdb->bdb_left;
	if (new_child)
	{
		BufferDesc* ptr = new_child;
		while (ptr->bdb_right) {
			ptr = ptr->bdb_right;
		}
		if ( (ptr->bdb_right = bdb->bdb_right) ) {
			ptr->bdb_right->bdb_parent = ptr;
		}
	}
	else {
		new_child = bdb->bdb_right;
	}

	// link the parent with the child node -- if no parent place it at the root

	BufferDesc* bdb_parent = bdb->bdb_parent;
	if (!bdb_parent) {
		bcb->bcb_btree = new_child;
	}
	else if (bdb_parent->bdb_left == bdb) {
		bdb_parent->bdb_left = new_child;
	}
	else {
		bdb_parent->bdb_right = new_child;
	}

	if (new_child) {
		new_child->bdb_parent = bdb_parent;
	}

	// initialize the node for next usage

	bdb->bdb_left = bdb->bdb_right = bdb->bdb_parent = NULL;
	//BTC_MUTEX_RELEASE;
}
#endif //!BALANCED_DIRTY_PAGE_TREE

#endif // DIRTY_TREE


#ifdef CACHE_READER
static THREAD_ENTRY_DECLARE cache_reader(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *	c a c h e _ r e a d e r
 *
 **************************************
 *
 * Functional description
 *	Prefetch pages into cache for sequential scans.
 *	Use asynchronous I/O to keep two prefetch requests
 *	busy at a time.
 *
 **************************************/
	Database* dbb = (Database*) arg;
	Database::SyncGuard dsGuard(dbb);

	ISC_STATUS_ARRAY status_vector;

	// Establish a thread context.
	ThreadContextHolder tdbb(status_vector);

	// Dummy attachment needed for lock owner identification.
	tdbb->setDatabase(dbb);
	Attachment* const attachment = Attachment::create(dbb);
	tdbb->setAttachment(attachment);
	attachment->att_filename = dbb->dbb_filename;
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_bufferpool);

	// This try block is specifically to protect the LCK_init call: if
	// LCK_init fails we won't be able to accomplish anything anyway, so
	// return, unlike the other try blocks further down the page.

	BufferControl* bcb = 0;

	try {

		LCK_init(tdbb, LCK_OWNER_attachment);
		bcb = dbb->dbb_bcb;
		bcb->bcb_flags |= BCB_cache_reader;
		dbb->dbb_reader_init.post();	// Notify our creator that we have started
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(status_vector, ex);
		gds__log_status(dbb->dbb_file->fil_string, status_vector);
		return -1;
	}

	try {

	// Set up dual prefetch control blocks to keep multiple prefetch
	// requests active at a time. Also helps to prevent the cache reader
	// from becoming dedicated or locked into a single request's prefetch demands.

	prf prefetch1, prefetch2;
	prefetch_init(&prefetch1, tdbb);
	prefetch_init(&prefetch2, tdbb);

	while (bcb->bcb_flags & BCB_cache_reader)
	{
		bcb->bcb_flags |= BCB_reader_active;
		bool found = false;
		SLONG starting_page = -1;
		prf* next_prefetch = &prefetch1;

		if (dbb->dbb_flags & DBB_suspend_bgio)
		{
			Database::Checkout dcoHolder(dbb);
			dbb->dbb_reader_sem.tryEnter(10);
			continue;
		}

		// Make a pass thru the global prefetch bitmap looking for something
		// to read. When the entire bitmap has been scanned and found to be
		// empty then wait for new requests.

		do {
			if (!(prefetch1.prf_flags & PRF_active) &&
				SBM_next(bcb->bcb_prefetch, &starting_page, RSE_get_forward))
			{
				found = true;
				prefetch_prologue(&prefetch1, &starting_page);
				prefetch_io(&prefetch1, status_vector);
			}

			if (!(prefetch2.prf_flags & PRF_active) &&
				SBM_next(bcb->bcb_prefetch, &starting_page, RSE_get_forward))
			{
				found = true;
				prefetch_prologue(&prefetch2, &starting_page);
				prefetch_io(&prefetch2, status_vector);
			}

			prf* post_prefetch = next_prefetch;
			next_prefetch = (post_prefetch == &prefetch1) ? &prefetch2 : &prefetch1;
			if (post_prefetch->prf_flags & PRF_active) {
				prefetch_epilogue(post_prefetch, status_vector);
			}

			if (found)
			{
				// If the cache writer or garbage collector is idle, put
				// them to work prefetching pages.
#ifdef CACHE_WRITER
				if ((bcb->bcb_flags & BCB_cache_writer) && !(bcb->bcb_flags & BCB_writer_active))
				{
					dbb_writer_sem.release();
				}
#endif
#ifdef GARBAGE_THREAD
				if ((dbb->dbb_flags & DBB_garbage_collector) && !(dbb->dbb_flags & DBB_gc_active))
				{
					dbb->dbb_gc_sem.release();
				}
#endif
			}
		} while (prefetch1.prf_flags & PRF_active || prefetch2.prf_flags & PRF_active);

		// If there's more work to do voluntarily ask to be rescheduled.
		// Otherwise, wait for event notification.
		BufferDesc* bdb;
		if (found) {
			JRD_reschedule(tdbb, 0, true);
		}
		else if (bcb->bcb_flags & BCB_free_pending &&
			(bdb = get_buffer(tdbb, FREE_PAGE, LATCH_none, 1)))
		{
			// In our spare time, help writer clean the cache.

			write_buffer(tdbb, bdb, bdb->bdb_page, true, status_vector, true);
		}
		else
		{
			bcb->bcb_flags &= ~BCB_reader_active;
			Database::Checkout dcoHolder(dbb);
			dbb->dbb_reader_sem.tryEnter(10);
		}
		bcb = dbb->dbb_bcb;
	}

	LCK_fini(tdbb, LCK_OWNER_attachment);
	Attachment::destroy(attachment);	// no need saving warning error strings here
	tdbb->setAttachment(NULL);
	bcb->bcb_flags &= ~BCB_cache_reader;
	dbb->dbb_reader_fini.post();

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(status_vector, ex);
		bcb = dbb->dbb_bcb;
		gds__log_status(dbb->dbb_file->fil_string, status_vector);
	}
	return 0;
}
#endif


#ifdef CACHE_WRITER
static THREAD_ENTRY_DECLARE cache_writer(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *	c a c h e _ w r i t e r
 *
 **************************************
 *
 * Functional description
 *	Write dirty pages to database to maintain an
 *	adequate supply of free pages. If WAL is enabled,
 *	perform database checkpoint when WAL subsystem
 *	deems it necessary.
 *
 **************************************/
	Database* dbb = (Database*)arg;
	Database::SyncGuard dsGuard(dbb);

	ISC_STATUS_ARRAY status_vector;

	// Establish a thread context.
	ThreadContextHolder tdbb(status_vector);

	// Dummy attachment needed for lock owner identification.

	tdbb->setDatabase(dbb);
	Attachment* const attachment = Attachment::create(dbb);
	tdbb->setAttachment(attachment);
	attachment->att_filename = dbb->dbb_filename;
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_bufferpool);

	// This try block is specifically to protect the LCK_init call: if
	// LCK_init fails we won't be able to accomplish anything anyway, so
	// return, unlike the other try blocks further down the page.
	Semaphore& writer_sem = dbb->dbb_writer_sem;
	BufferControl* bcb = dbb->dbb_bcb;

	try {
		LCK_init(tdbb, LCK_OWNER_attachment);
		bcb->bcb_flags |= BCB_cache_writer;
		bcb->bcb_flags &= ~BCB_writer_start;

		// Notify our creator that we have started
		dbb->dbb_writer_init.release();
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(status_vector, ex);
		gds__log_status(dbb->dbb_filename.c_str(), status_vector);

		bcb->bcb_flags &= ~(BCB_cache_writer | BCB_writer_start);
		return (THREAD_ENTRY_RETURN)(-1);
	}

	try {
		while (bcb->bcb_flags & BCB_cache_writer)
		{
			bcb->bcb_flags |= BCB_writer_active;
#ifdef CACHE_READER
			SLONG starting_page = -1;
#endif

			if (dbb->dbb_flags & DBB_suspend_bgio)
			{
				{ //scope
					Database::Checkout dcoHolder(dbb);
					writer_sem.tryEnter(10);
				}
				bcb = dbb->dbb_bcb;
				continue;
			}

#ifdef SUPERSERVER_V2
			// Flush buffers for lazy commit
			SLONG commit_mask;
			if (!(dbb->dbb_flags & DBB_force_write) && (commit_mask = dbb->dbb_flush_cycle))
			{
				dbb->dbb_flush_cycle = 0;
				btc_flush(tdbb, commit_mask, false, status_vector);
			}
#endif

			{ // scope
				Database::Checkout dcoHolder(dbb);
				THREAD_YIELD();
			}

			if (bcb->bcb_flags & BCB_free_pending)
			{
				BufferDesc* bdb = get_buffer(tdbb, FREE_PAGE, LATCH_none, 1);
				if (bdb)
				{
					write_buffer(tdbb, bdb, bdb->bdb_page, true, status_vector, true);
					bcb = dbb->dbb_bcb;
				}

				// If the cache reader or garbage collector is idle, put
				// them to work freeing pages.
#ifdef CACHE_READER
				if ((bcb->bcb_flags & BCB_cache_reader) && !(bcb->bcb_flags & BCB_reader_active))
				{
					dbb->dbb_reader_sem.post();
				}
#endif
#ifdef GARBAGE_THREAD
				if ((dbb->dbb_flags & DBB_garbage_collector) && !(dbb->dbb_flags & DBB_gc_active))
				{
					dbb->dbb_gc_sem.release();
				}
#endif
			}

			// If there's more work to do voluntarily ask to be rescheduled.
			// Otherwise, wait for event notification.

			if ((bcb->bcb_flags & BCB_free_pending) || bcb->bcb_checkpoint || dbb->dbb_flush_cycle)
			{
				JRD_reschedule(tdbb, 0, true);
			}
#ifdef CACHE_READER
			else if (SBM_next(bcb->bcb_prefetch, &starting_page, RSE_get_forward))
			{
				// Prefetch some pages in our spare time and in the process
				// garbage collect the prefetch bitmap.
				prf prefetch;

				prefetch_init(&prefetch, tdbb);
				prefetch_prologue(&prefetch, &starting_page);
				prefetch_io(&prefetch, status_vector);
				prefetch_epilogue(&prefetch, status_vector);
			}
#endif
			else
			{
				bcb->bcb_flags &= ~BCB_writer_active;
				Database::Checkout dcoHolder(dbb);
				writer_sem.tryEnter(10);
			}
			bcb = dbb->dbb_bcb;
		}

		LCK_fini(tdbb, LCK_OWNER_attachment);
		Attachment::destroy(attachment);	// no need saving warning error strings here
		tdbb->setAttachment(NULL);
		bcb->bcb_flags &= ~BCB_cache_writer;
		// Notify the finalization caller that we're finishing.
		dbb->dbb_writer_fini.release();

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(status_vector, ex);
		bcb = dbb->dbb_bcb;
		gds__log_status(dbb->dbb_filename.c_str(), status_vector);
	}
	return 0;
}
#endif


static void check_precedence(thread_db* tdbb, WIN* window, PageNumber page)
{
/**************************************
 *
 *	c h e c k _ p r e c e d e n c e
 *
 **************************************
 *
 * Functional description
 *	Given a window accessed for write and a page number,
 *	establish a precedence relationship such that the
 *	specified page will always be written before the page
 *	associated with the window.
 *
 *	If the "page number" is negative, it is really a transaction
 *	id.  In this case, the precedence relationship is to the
 *	database header page from which the transaction id was
 *	obtained.  If the header page has been written since the
 *	transaction id was assigned, no precedence relationship
 *	is required.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// If this is really a transaction id, sort things out

	if ((page.getPageSpaceID() == DB_PAGE_SPACE) && (page.getPageNum() < 0))
	{
		if (-page.getPageNum() <= dbb->dbb_last_header_write) {
			return;
		}

		page = PageNumber(DB_PAGE_SPACE, 0);
	}

	// Start by finding the buffer containing the high priority page

	//BCB_MUTEX_ACQUIRE;
	//PRE_MUTEX_ACQUIRE;
	BufferControl* bcb = dbb->dbb_bcb;
	QUE mod_que = &bcb->bcb_rpt[page.getPageNum() % bcb->bcb_count].bcb_page_mod;

	BufferDesc* high = 0;
	QUE que_inst;
	for (que_inst = mod_que->que_forward; que_inst != mod_que; que_inst = que_inst->que_forward)
	{
		if ((high = BLOCK(que_inst, BufferDesc*, bdb_que))->bdb_page == page) {
			break;
		}
	}

	//BCB_MUTEX_RELEASE;
	if (que_inst == mod_que)
	{
		//PRE_MUTEX_RELEASE;
		return;
	}

	// Found the higher precedence buffer.  If it's not dirty, don't sweat it.
	// If it's the same page, ditto.

	if (!(high->bdb_flags & BDB_dirty) || (high->bdb_page == window->win_page))
	{
		//PRE_MUTEX_RELEASE;
		return;
	}

	BufferDesc* low = window->win_bdb;

	if ((low->bdb_flags & BDB_marked) && !(low->bdb_flags & BDB_faked))
		BUGCHECK(212);	// msg 212 CCH_precedence: block marked

	// If already related, there's nothing more to do. If the precedence
	// search was too complex to complete, just write the high page and
	// forget about about establishing the relationship.

	if (QUE_NOT_EMPTY(high->bdb_lower))
	{
		const ULONG mark = get_prec_walk_mark(bcb);
		const SSHORT relationship = related(low, high, PRE_SEARCH_LIMIT, mark);
		if (relationship == PRE_EXISTS)
		{
			//PRE_MUTEX_RELEASE;
			return;
		}

		if (relationship == PRE_UNKNOWN)
		{
			const PageNumber high_page = high->bdb_page;
			//PRE_MUTEX_RELEASE;
			if (!write_buffer(tdbb, high, high_page, false, tdbb->tdbb_status_vector, true))
			{
				CCH_unwind(tdbb, true);
			}
			return;
		}
	}

	// Check to see if we're going to create a cycle or the precedence search
	// was too complex to complete.  If so, force a write of the "after"
	// (currently fetched) page.  Assuming everyone obeys the rules and calls
	// precedence before marking the buffer, everything should be ok

	if (QUE_NOT_EMPTY(low->bdb_lower))
	{
		const ULONG mark = get_prec_walk_mark(bcb);
		const SSHORT relationship = related(high, low, PRE_SEARCH_LIMIT, mark);
		if (relationship == PRE_EXISTS || relationship == PRE_UNKNOWN)
		{
			const PageNumber low_page = low->bdb_page;
			//PRE_MUTEX_RELEASE;
			if (!write_buffer(tdbb, low, low_page, false, tdbb->tdbb_status_vector, true))
			{
				CCH_unwind(tdbb, true);
			}
			//PRE_MUTEX_ACQUIRE;
		}
	}

	// We're going to establish a new precedence relationship.  Get a block,
	// fill in the appropriate fields, and insert it into the various ques

	bcb = dbb->dbb_bcb;			// Re-initialize

	Precedence* precedence = bcb->bcb_free;
	if (precedence) {
		bcb->bcb_free = (Precedence*) precedence->pre_hi;
	}
	else {
		precedence = FB_NEW(*dbb->dbb_bufferpool) Precedence;
	}

	precedence->pre_low = low;
	precedence->pre_hi = high;
	precedence->pre_flags = 0;
	QUE_INSERT(low->bdb_higher, precedence->pre_higher);
	QUE_INSERT(high->bdb_lower, precedence->pre_lower);

#ifdef DIRTY_LIST
	// explicitly include high page in system transaction flush process
	if (low->bdb_flags & BDB_system_dirty && high->bdb_flags & BDB_dirty)
		high->bdb_flags |= BDB_system_dirty;
#endif
	//PRE_MUTEX_RELEASE;
}



static void clear_precedence(thread_db* tdbb, BufferDesc* bdb)
{
/**************************************
 *
 *	c l e a r _ p r e c e d e n c e
 *
 **************************************
 *
 * Functional description
 *	Clear precedence relationships to lower precedence block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	//PRE_MUTEX_ACQUIRE;
	BufferControl* const bcb = dbb->dbb_bcb;

	// Loop thru lower precedence buffers.  If any can be downgraded,
	// by all means down grade them.

	while (QUE_NOT_EMPTY(bdb->bdb_lower))
	{
		QUE que_inst = bdb->bdb_lower.que_forward;
		Precedence* precedence = BLOCK(que_inst, Precedence*, pre_lower);
		BufferDesc* low_bdb = precedence->pre_low;
		QUE_DELETE(precedence->pre_higher);
		QUE_DELETE(precedence->pre_lower);
		precedence->pre_hi = (BufferDesc*) bcb->bcb_free;
		bcb->bcb_free = precedence;
		if (!(precedence->pre_flags & PRE_cleared))
		{
			if (low_bdb->bdb_ast_flags & BDB_blocking)
			{
				PAGE_LOCK_RE_POST(low_bdb->bdb_lock);
			}
		}
	}

	//PRE_MUTEX_RELEASE;
}


static BufferDesc* dealloc_bdb(BufferDesc* bdb)
{
/**************************************
 *
 *	d e a l l o c _ b d b
 *
 **************************************
 *
 * Functional description
 *	Deallocate buffer descriptor block.
 *
 **************************************/
	if (bdb)
	{
#ifndef SUPERSERVER
		delete bdb->bdb_lock;
#endif
		QUE_DELETE(bdb->bdb_que);
		delete bdb;
	}

	return NULL;
}


#ifndef SUPERSERVER
// CVC: Nobody was interested in the result from this function, so I made it
// void instead of bool, but preserved the returned values in comments.
static void down_grade(thread_db* tdbb, BufferDesc* bdb)
{
/**************************************
 *
 *	d o w n _ g r a d e
 *
 **************************************
 *
 * Functional description
 *	A lock on a page is blocking another process.  If possible, downgrade
 *	the lock on the buffer.  This may be called from either AST or
 *	regular level.  Return true if the down grade was successful.  If the
 *	down grade was deferred for any reason, return false.
 *
 **************************************/
	SET_TDBB(tdbb);

	bdb->bdb_ast_flags |= BDB_blocking;
	Lock* lock = bdb->bdb_lock;
	Database* dbb = bdb->bdb_dbb;

	if (dbb->dbb_flags & DBB_bugcheck)
	{
		PAGE_LOCK_RELEASE(bdb->bdb_lock);
		bdb->bdb_ast_flags &= ~BDB_blocking;

		clear_dirty_flag(tdbb, bdb);

		return; // true;
	}

	// If the BufferDesc is in use and, being written or already
	// downgraded to read, mark it as blocking and exit.

	if (bdb->bdb_use_count) {
		return; // false;
	}

	latch_bdb(tdbb, LATCH_io, bdb, bdb->bdb_page, 1);

	// If the page isn't dirty, the lock can be quietly downgraded.

	if (!(bdb->bdb_flags & BDB_dirty))
	{
		bdb->bdb_ast_flags &= ~BDB_blocking;
		LCK_downgrade(tdbb, lock);
		release_bdb(tdbb, bdb, false, false, false);
		return; // true;
	}

	bool in_use = false, invalid = false;

	if (bdb->bdb_flags & BDB_not_valid) {
		invalid = true;
	}

	// If there are higher precedence guys, see if they can be written.
	for (QUE que_inst = bdb->bdb_higher.que_forward; que_inst != &bdb->bdb_higher;
		 que_inst = que_inst->que_forward)
	{
		Precedence* precedence = BLOCK(que_inst, Precedence*, pre_higher);
		if (precedence->pre_flags & PRE_cleared)
			continue;
		if (invalid)
		{
			precedence->pre_flags |= PRE_cleared;
			continue;
		}
		BufferDesc* blocking_bdb = precedence->pre_hi;
		if (blocking_bdb->bdb_flags & BDB_dirty)
		{
			down_grade(tdbb, blocking_bdb);
			if (blocking_bdb->bdb_flags & BDB_dirty) {
				in_use = true;
			}
			if (blocking_bdb->bdb_flags & BDB_not_valid)
			{
				invalid = true;
				in_use = false;
				que_inst = bdb->bdb_higher.que_forward;
			}
		}
	}

	// If any higher precedence buffer can't be written, mark this buffer as blocking and exit.

	if (in_use)
	{
		release_bdb(tdbb, bdb, false, false, false);
		return; // false;
	}

	// Everything is clear to write this buffer.  Do so and reduce the lock

	if (invalid || !write_page(tdbb, bdb, /*false,*/ tdbb->tdbb_status_vector, true))
	{
		bdb->bdb_flags |= BDB_not_valid;
		clear_dirty_flag(tdbb, bdb);
		bdb->bdb_ast_flags &= ~BDB_blocking;
		TRA_invalidate(dbb, bdb->bdb_transactions);
		bdb->bdb_transactions = 0;
		PAGE_LOCK_RELEASE(bdb->bdb_lock);
	}
	else
	{
		bdb->bdb_ast_flags &= ~BDB_blocking;
		LCK_downgrade(tdbb, lock);
	}

	// Clear precedence relationships to lower precedence buffers.  Since it
	// isn't safe to tweak the que pointers from AST level, just mark the precedence
	// links as cleared.  Somebody else will clean up the precedence blocks.

	for (QUE que_inst = bdb->bdb_lower.que_forward; que_inst != &bdb->bdb_lower;
		 que_inst = que_inst->que_forward)
	{
		Precedence* precedence = BLOCK(que_inst, Precedence*, pre_lower);
		BufferDesc* blocking_bdb = precedence->pre_low;
		if (bdb->bdb_flags & BDB_not_valid)
			blocking_bdb->bdb_flags |= BDB_not_valid;
		precedence->pre_flags |= PRE_cleared;
		if ((blocking_bdb->bdb_flags & BDB_not_valid) || (blocking_bdb->bdb_ast_flags & BDB_blocking))
		{
			down_grade(tdbb, blocking_bdb);
		}
	}

	bdb->bdb_flags &= ~BDB_not_valid;
	release_bdb(tdbb, bdb, false, false, false);

	return; // true;
}
#endif


static void expand_buffers(thread_db* tdbb, ULONG number)
{
/**************************************
 *
 *	e x p a n d _ b u f f e r s
 *
 **************************************
 *
 * Functional description
 *	Expand the cache to at least a given number of buffers.  If
 *	it's already that big, don't do anything.
 *
 * Nickolay Samofatov, 08-Mar-2004.
 *  This function does not handle exceptions correctly,
 *  it looks like good handling requires rewrite.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BufferControl* old = dbb->dbb_bcb;

	if (number <= old->bcb_count || number > MAX_PAGE_BUFFERS) {
		return;
	}

	// for Win16 platform, we want to ensure that no cache buffer ever ends on a segment boundary
	// CVC: Is this code obsolete or only the comment?

	ULONG num_per_seg = number - old->bcb_count;
	ULONG left_to_do = num_per_seg;

	// Allocate and initialize buffers control block
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_bufferpool);

	old = dbb->dbb_bcb;
	const bcb_repeat* const old_end = old->bcb_rpt + old->bcb_count;

	BufferControl* new_block = FB_NEW_RPT(*dbb->dbb_bufferpool, number)
		BufferControl(*dbb->dbb_bufferpool);
	new_block->bcb_count = number;
	new_block->bcb_free_minimum = (SSHORT) MIN(number / 4, 128);	// 25% clean page reserve
	new_block->bcb_checkpoint = old->bcb_checkpoint;
	new_block->bcb_flags = old->bcb_flags;
	const bcb_repeat* const new_end = new_block->bcb_rpt + number;

	// point at the dirty page binary tree

#ifdef DIRTY_LIST
	new_block->bcb_dirty_count = old->bcb_dirty_count;
	QUE_INSERT(old->bcb_dirty, new_block->bcb_dirty);
	QUE_DELETE(old->bcb_dirty);
#endif
#ifdef DIRTY_TREE
	new_block->bcb_btree = old->bcb_btree;
#endif

	// point at the free precedence blocks

	new_block->bcb_free = old->bcb_free;

	// position the new bcb in the in use, empty and latch queues

	QUE_INSERT(old->bcb_in_use, new_block->bcb_in_use);
	QUE_DELETE(old->bcb_in_use);
	QUE_INSERT(old->bcb_empty, new_block->bcb_empty);
	QUE_DELETE(old->bcb_empty);
	QUE_INSERT(old->bcb_free_lwt, new_block->bcb_free_lwt);
	QUE_DELETE(old->bcb_free_lwt);
	QUE_INSERT(old->bcb_free_slt, new_block->bcb_free_slt);
	QUE_DELETE(old->bcb_free_slt);

	// Copy addresses of previously allocated buffer space to new block

	for (Jrd::UCharStack::iterator stack(old->bcb_memory); stack.hasData(); ++stack) {
		new_block->bcb_memory.push(stack.object());
	}

	// Initialize tail of new buffer control block
	bcb_repeat* new_tail;
	for (new_tail = new_block->bcb_rpt; new_tail < new_end; new_tail++) {
		QUE_INIT(new_tail->bcb_page_mod);
	}

	// Move any active buffers from old block to new

	new_tail = new_block->bcb_rpt;

	for (bcb_repeat* old_tail = old->bcb_rpt; old_tail < old_end; old_tail++, new_tail++)
	{
		new_tail->bcb_bdb = old_tail->bcb_bdb;
		while (QUE_NOT_EMPTY(old_tail->bcb_page_mod))
		{
			QUE que_inst = old_tail->bcb_page_mod.que_forward;
			BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_que);
			QUE_DELETE(*que_inst);
			QUE mod_que =
				&new_block->bcb_rpt[bdb->bdb_page.getPageNum() % new_block->bcb_count].bcb_page_mod;
			QUE_INSERT(*mod_que, *que_inst);
		}
	}

	// Allocate new buffer descriptor blocks

	ULONG num_in_seg = 0;
	UCHAR* memory = NULL;
	for (; new_tail < new_end; new_tail++)
	{
		// if current segment is exhausted, allocate another

		if (!num_in_seg)
		{
			const size_t alloc_size = dbb->dbb_page_size * (num_per_seg + 1);
			memory = (UCHAR*) dbb->dbb_bufferpool->allocate_nothrow(alloc_size);
			// NOMEM: crash!
			new_block->bcb_memory.push(memory);
			memory = (UCHAR *) FB_ALIGN((U_IPTR) memory, dbb->dbb_page_size);
			num_in_seg = num_per_seg;
			left_to_do -= num_per_seg;
			if (num_per_seg > left_to_do) {
				num_per_seg = left_to_do;
			}
		}
		new_tail->bcb_bdb = alloc_bdb(tdbb, new_block, &memory);
		num_in_seg--;
	}

	// Set up new buffer control, release old buffer control, and clean up

	dbb->dbb_bcb = new_block;

	delete old;
}


static BufferDesc* get_buffer(thread_db* tdbb, const PageNumber page, LATCH latch, SSHORT latch_wait)
{
/**************************************
 *
 *	g e t _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Get a buffer.  If possible, get a buffer already assigned
 *	to the page.  Otherwise get one from the free list or pick
 *	the least recently used buffer to be reused.
 *	Note the following special page numbers:
 *	     -1 indicates that a buffer is required for journaling => obsolete
 *	     -2 indicates a special scratch buffer for shadowing
 *
 * input
 *	page:		page to get
 *	latch:		type of latch to acquire on the page.
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0;
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	BufferDesc pointer if successful.
 *	NULL pointer if timeout occurred (only possible is latch_wait <> 1).
 *		     if cache manager doesn't have any pages to write anymore.
 *
 **************************************/
	// CVC: Those two vars are tricky or nonsense to put in minimal scope.
	QUE que_inst;
	BufferControl* bcb;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
#ifdef CACHE_WRITER
	SSHORT walk = dbb->dbb_bcb->bcb_free_minimum;
#endif

	//BCB_MUTEX_ACQUIRE;

	while (true)
	{
	  find_page:

		bcb = dbb->dbb_bcb;
		if (page.getPageNum() >= 0)
		{
			// Check to see if buffer has already been assigned to page

			QUE mod_que = &bcb->bcb_rpt[page.getPageNum() % bcb->bcb_count].bcb_page_mod;
			for (que_inst = mod_que->que_forward; que_inst != mod_que;
				que_inst = que_inst->que_forward)
			{
				BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_que);
				if (bdb->bdb_page == page)
				{
#ifdef SUPERSERVER_V2
					if (page != HEADER_PAGE_NUMBER)
#endif
						QUE_MOST_RECENTLY_USED(bdb->bdb_in_use);
					//BCB_MUTEX_RELEASE;
					const SSHORT latch_return = latch_bdb(tdbb, latch, bdb, page, latch_wait);

					if (latch_return)
					{
						if (latch_return == 1) {
							return NULL;	// permitted timeout happened
						}
						//BCB_MUTEX_ACQUIRE;
						goto find_page;
					}

					bdb->bdb_flags &= ~(BDB_faked | BDB_prefetch);
					tdbb->bumpStats(RuntimeStatistics::PAGE_FETCHES);
					return bdb;
				}
			}
		}
#ifdef CACHE_WRITER
		else if ((page == FREE_PAGE) || (page == CHECKPOINT_PAGE))
		{
			// This code is only used by the background I/O threads:
			// cache writer, cache reader and garbage collector.

			for (que_inst = bcb->bcb_in_use.que_backward;
				 que_inst != &bcb->bcb_in_use; que_inst = que_inst->que_backward)
			{
				BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_in_use);
				if (page == FREE_PAGE)
				{
					if (bdb->bdb_use_count || (bdb->bdb_flags & BDB_free_pending))
					{
						continue;
					}
					if (bdb->bdb_flags & BDB_db_dirty)
					{
						//BCB_MUTEX_RELEASE;
						return bdb;
					}
					if (!--walk)
					{
						bcb->bcb_flags &= ~BCB_free_pending;
						break;
					}
				}
				else	// if (page == CHECKPOINT_PAGE)
				{

					if (bdb->bdb_flags & BDB_checkpoint)
					{
						//BCB_MUTEX_RELEASE;
						return bdb;
					}
				}
			}

			if (page == FREE_PAGE) {
				bcb->bcb_flags &= ~BCB_free_pending;
			}

			//BCB_MUTEX_RELEASE;
			return NULL;
		}
#endif

		for (que_inst = bcb->bcb_in_use.que_backward;
			 que_inst != &bcb->bcb_in_use || QUE_NOT_EMPTY(bcb->bcb_empty);
			 que_inst = que_inst->que_backward)
		{
			bcb = dbb->dbb_bcb;	// Re-initialize in the loop
			QUE mod_que = &bcb->bcb_rpt[page.getPageNum() % bcb->bcb_count].bcb_page_mod;

			// If there is an empty buffer sitting around, allocate it

			if (QUE_NOT_EMPTY(bcb->bcb_empty))
			{
				que_inst = bcb->bcb_empty.que_forward;
				QUE_DELETE(*que_inst);
				BufferDesc* bdb = BLOCK(que_inst, BufferDesc*, bdb_que);
				if (page.getPageNum() >= 0)
				{
					QUE_INSERT(*mod_que, *que_inst);
#ifdef SUPERSERVER_V2
					// Reserve a buffer for header page with deferred header
					// page write mechanism. Otherwise, a deadlock will occur
					// if all dirty pages in the cache must force header page
					// to disk before they can be written but there is no free
					// buffer to read the header page into.

					if (page != HEADER_PAGE_NUMBER)
#endif
						QUE_INSERT(bcb->bcb_in_use, bdb->bdb_in_use);
				}

				// This correction for bdb_use_count below is needed to
				// avoid a deadlock situation in latching code.  It's not
				// clear though how the bdb_use_count can get < 0 for a bdb
				// in bcb_empty queue
				if (bdb->bdb_use_count < 0) {
					BUGCHECK(301);	// msg 301 Non-zero use_count of a buffer in the empty que_inst
				}

				bdb->bdb_page = page;

				fb_assert((bdb->bdb_flags & (BDB_dirty | BDB_marked)) == 0)

				bdb->bdb_flags = BDB_read_pending;
				bdb->bdb_scan_count = 0;
				// The following latch should never fail because the buffer is 'empty'
				// and the page is not in cache.
				if (latch_bdb(tdbb, latch, bdb, page, -100) == -1) {
					BUGCHECK(302);	// msg 302 unexpected page change
				}
#ifndef SUPERSERVER
				if (page.getPageNum() >= 0)
				{
					CCH_TRACE(("bdb->bdb_lock->lck_logical = LCK_none; page=%i", bdb->bdb_page));
					bdb->bdb_lock->lck_logical = LCK_none;
				}
				else {
					PAGE_LOCK_RELEASE(bdb->bdb_lock);
				}
#endif
				tdbb->bumpStats(RuntimeStatistics::PAGE_FETCHES);
				//BCB_MUTEX_RELEASE;
				return bdb;
			}

			// get the oldest buffer as the least recently used -- note
			// that since there are no empty buffers this queue cannot be empty

			if (bcb->bcb_in_use.que_forward == &bcb->bcb_in_use) {
				BUGCHECK(213);	// msg 213 insufficient cache size
			}

			BufferDesc* oldest = BLOCK(que_inst, BufferDesc*, bdb_in_use);
			//LATCH_MUTEX_ACQUIRE;
			if (oldest->bdb_use_count || (oldest->bdb_flags & BDB_free_pending) ||
				!writeable(oldest))
			{
				//LATCH_MUTEX_RELEASE;
				continue;
			}

#ifdef SUPERSERVER_V2
			// If page has been prefetched but not yet fetched, let
			// it cycle once more thru LRU queue before re-using it.

			if (oldest->bdb_flags & BDB_prefetch)
			{
				oldest->bdb_flags &= ~BDB_prefetch;
				que_inst = que_inst->que_forward;
				QUE_MOST_RECENTLY_USED(oldest->bdb_in_use);
				//LATCH_MUTEX_RELEASE;
				continue;
			}
#endif
			//LATCH_MUTEX_RELEASE;
#ifdef CACHE_WRITER
			if (oldest->bdb_flags & (BDB_dirty | BDB_db_dirty))
			{
				bcb->bcb_flags |= BCB_free_pending;
				if ((bcb->bcb_flags & BCB_cache_writer) && !(bcb->bcb_flags & BCB_writer_active))
				{
					dbb->dbb_writer_sem.release();
				}
				if (walk)
				{
					if (!--walk)
						break;

					continue;
				}
			}
#endif
			BufferDesc* bdb = oldest;
			if (latch_bdb(tdbb, LATCH_exclusive, bdb, bdb->bdb_page, 0)) {
				continue;		// BufferDesc changed, continue looking for a buffer
			}
			QUE_MOST_RECENTLY_USED(bdb->bdb_in_use);
			bdb->bdb_flags |= BDB_free_pending;

			// If the buffer selected is dirty, arrange to have it written.

			if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))
			{
				//BCB_MUTEX_RELEASE;
#ifdef SUPERSERVER
				if (!write_buffer(tdbb, bdb, bdb->bdb_page, true, tdbb->tdbb_status_vector, true))
#else
				if (!write_buffer (tdbb, bdb, bdb->bdb_page, false, tdbb->tdbb_status_vector, true))
#endif
				{
					bdb->bdb_flags &= ~BDB_free_pending;
					release_bdb(tdbb, bdb, false, false, false);
					CCH_unwind(tdbb, true);
				}
			}

			bcb = dbb->dbb_bcb;	// Re-initialize

			// If the buffer is still in the dirty tree, remove it.
			// In any case, release any lock it may have.

#ifdef DIRTY_LIST
			removeDirty(bcb, bdb);
#endif
#ifdef DIRTY_TREE
			if (bdb->bdb_parent || (bdb == bcb->bcb_btree)) {
				btc_remove(bdb);
			}
#endif

			// if the page has an expanded index buffer, release it

			delete bdb->bdb_expanded_buffer;
			bdb->bdb_expanded_buffer = NULL;

			// Cleanup any residual precedence blocks.  Unless something is
			// screwed up, the only precedence blocks that can still be hanging
			// around are ones cleared at AST level.

			//PRE_MUTEX_ACQUIRE;
			while (QUE_NOT_EMPTY(bdb->bdb_higher))
			{
				QUE que2 = bdb->bdb_higher.que_forward;
				Precedence* precedence = BLOCK(que2, Precedence*, pre_higher);
				QUE_DELETE(precedence->pre_higher);
				QUE_DELETE(precedence->pre_lower);
				precedence->pre_hi = (BufferDesc*) bcb->bcb_free;
				bcb->bcb_free = precedence;
			}
			//PRE_MUTEX_RELEASE;

			clear_precedence(tdbb, bdb);

			// remove the buffer from the "mod" queue and place it
			// in it's new spot, provided it's not a negative (scratch) page

			//BCB_MUTEX_ACQUIRE;
			if (bdb->bdb_page.getPageNum() >= 0) {
				QUE_DELETE(bdb->bdb_que);
			}
			QUE_INSERT(bcb->bcb_empty, bdb->bdb_que);
			QUE_DELETE(bdb->bdb_in_use);

			bdb->bdb_page = JOURNAL_PAGE;
			release_bdb(tdbb, bdb, false, false, false);

			break;
		}

		if (que_inst == &bcb->bcb_in_use)
		{
#ifdef SUPERSERVER
			expand_buffers(tdbb, bcb->bcb_count + 75);
#else
			BUGCHECK(214);	// msg 214 no cache buffers available for reuse
#endif
		}
	}
}


static ULONG get_prec_walk_mark(BufferControl* bcb)
{
/**************************************
 *
 *	g e t _ p r e c _ w a l k _ m a r k
 *
 **************************************
 *
 * Functional description
 *  Get next mark for walking precedence graph.
 *
 **************************************/
	if (++bcb->bcb_prec_walk_mark == 0)
	{
		for (ULONG i = 0; i < bcb->bcb_count; i++) {
			bcb->bcb_rpt[i].bcb_bdb->bdb_prec_walk_mark = 0;
		}

		bcb->bcb_prec_walk_mark = 1;
	}
	return bcb->bcb_prec_walk_mark;
}


static int get_related(BufferDesc* bdb, PagesArray &lowPages, int limit, const ULONG mark)
{
/**************************************
 *
 *	g e t _ r e l a t e d
 *
 **************************************
 *
 * Functional description
 *  Recursively walk low part of precedence graph of given buffer and put
 *  low pages numbers into array.
 *
 **************************************/
	const struct que* base = &bdb->bdb_lower;
	for (const struct que* que_inst = base->que_forward; que_inst != base; que_inst = que_inst->que_forward)
	{
		const Precedence* precedence = BLOCK(que_inst, Precedence*, pre_lower);
		if (precedence->pre_flags & PRE_cleared)
			continue;

		BufferDesc *low = precedence->pre_low;
		if (low->bdb_prec_walk_mark == mark)
			continue;

		if (!--limit)
			return 0;

		const SLONG lowPage = low->bdb_page.getPageNum();
		size_t pos;
		if (!lowPages.find(lowPage, pos))
			lowPages.insert(pos, lowPage);

		if (QUE_NOT_EMPTY(low->bdb_lower))
		{
			limit = get_related(low, lowPages, limit, mark);
			if (!limit)
				return 0;
		}
		else
			low->bdb_prec_walk_mark = mark;
	}

	bdb->bdb_prec_walk_mark = mark;
	return limit;
}


static void invalidate_and_release_buffer(thread_db* tdbb, BufferDesc* bdb)
{
/**************************************
 *
 *	i n v a l i d a t e _ a n d _ r e l e a s e _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *  Invalidate the page buffer.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();
	bdb->bdb_flags |= BDB_not_valid;
	clear_dirty_flag(tdbb, bdb);
	TRA_invalidate(dbb, bdb->bdb_transactions);
	bdb->bdb_transactions = 0;
	release_bdb(tdbb, bdb, false, false, false);
}


static SSHORT latch_bdb(thread_db* tdbb,
						LATCH type, BufferDesc* bdb, const PageNumber page, SSHORT latch_wait)
{
/**************************************
 *
 *	l a t c h _ b d b
 *
 **************************************
 *
 * Functional description
 *
 * input
 *	type:		LATCH_none, LATCH_exclusive, LATCH_io, LATCH_shared, or LATCH_mark.
 *	bdb:		object to acquire latch on.
 *	page:		page of bdb, for verification.
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 1.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	0:	latch successfully acquired.
 *	1:	latch not acquired due to a (permitted) timeout.
 *	-1:	latch not acquired, bdb doesn't corresponds to page.
 *
 **************************************/

	// If the buffer has been reassigned to another page make the caller deal with it.

	if (bdb->bdb_page != page) {
		return -1;
	}

	//LATCH_MUTEX_ACQUIRE;

	// Handle the easy case first, no users of the buffer.

	if (!bdb->bdb_use_count)
	{
		switch (type)
		{
		case LATCH_shared:
			++bdb->bdb_use_count;
			++tdbb->tdbb_latch_count;
			allocSharedLatch(tdbb, bdb);
			break;
		case LATCH_exclusive:
			++bdb->bdb_use_count;
			++tdbb->tdbb_latch_count;
			bdb->bdb_exclusive = tdbb;
			break;
		case LATCH_io:
			++bdb->bdb_use_count;
			++tdbb->tdbb_latch_count;
			bdb->bdb_io = tdbb;
			break;
		case LATCH_mark:
			BUGCHECK(295);	// inconsistent LATCH_mark call
			break;
		case LATCH_none:
			break;
		}
		//LATCH_MUTEX_RELEASE;
		return 0;
	}

	/* Grant the latch request if it is compatible with existing
	latch holders.  Pending latches are queued in the order in
	which they are requested except that io/mark latches are queued
	ahead of all other latch requests (to avoid deadlocks and
	this does not cause starvation).  Also, shared latches are granted
	immediately if a disk write is in progress.
	Note that asking for a higher mode latch when already holding a
	share latch results in deadlock.  CCH_handoff routinely acquires a
	shared latch while owning already a shared latch on the page
	(the case of handing off to the same page).  If the BDB_must_write
	flag is set, then an exclusive latch request will be followed by
	an io latch request. */

	switch (type)
	{

	case LATCH_none:
		//LATCH_MUTEX_RELEASE;
		return 0;

	case LATCH_shared:
		if (bdb->bdb_flags & BDB_read_pending) {
			break;
		}
		if (bdb->bdb_exclusive)
		{
			if (bdb->bdb_exclusive != tdbb) {
				break;			// someone else owns exclusive latch
			}
		}
		else
		{
			// Note that Firebird often 'hands-off' to the same page, for both
			// shared and exlusive latches.
			// Check if we own already an exclusive latch.
			if (!findSharedLatch(tdbb, bdb))
			{
				// we don't own a shared latch yet
				// If there are latch-waiters, and they are not waiting for an
				// io_latch, then we have to wait also (there must be a exclusive
				// latch waiter).  If there is an IO in progress, then violate the
				// fairness and sneak ahead of the exclusive (or io) waiters.

				if ((QUE_NOT_EMPTY(bdb->bdb_waiters)) && !bdb->bdb_io) {
					break;		// be fair and wait behind exclusive latch requests
				}
			}
		}
		// Nobody owns an exlusive latch, or sneak ahead of exclusive latch
		// waiters while an io is in progress.

		++bdb->bdb_use_count;
		++tdbb->tdbb_latch_count;
		allocSharedLatch(tdbb, bdb);
		//LATCH_MUTEX_RELEASE;
		return 0;

	case LATCH_io:
		if (bdb->bdb_flags & BDB_read_pending) {
			break;
		}
		if (bdb->bdb_io) {
			break;				// someone else owns the io latch
		}
		++bdb->bdb_use_count;
		++tdbb->tdbb_latch_count;
		bdb->bdb_io = tdbb;
		//LATCH_MUTEX_RELEASE;
		return 0;

	case LATCH_exclusive:
		// Exclusive latches wait for existing shared latches and
		// (unfortunately) for existing io latches.  This is not as
		// bad as it sounds because an exclusive latch is typically followed
		// by a mark latch, which then would wait behind the io latch.
		// Obsolete: Note that the ail-code latches the same buffer multiple times
		// in shared and exclusive
		// Note that Firebird often 'hands-off' to the same page, for both
		// shared and exlusive latches.
		if (bdb->bdb_use_count && (bdb->bdb_exclusive != tdbb)) {
			break;
		}
		++bdb->bdb_use_count;
		++tdbb->tdbb_latch_count;
		bdb->bdb_exclusive = tdbb;
		//LATCH_MUTEX_RELEASE;
		return 0;

	case LATCH_mark:
		if (bdb->bdb_exclusive != tdbb) {
			BUGCHECK(295);	// inconsistent LATCH_mark call
		}
		// Some Firebird code marks a buffer more than once.
		if (bdb->bdb_io && (bdb->bdb_io != tdbb)) {
			break;
		}
		bdb->bdb_io = tdbb;
		//LATCH_MUTEX_RELEASE;
		return 0;

	default:
		break;
	}

	// If the caller doesn't want to wait for this latch, then return now.
	if (latch_wait == 0)
	{
		//LATCH_MUTEX_RELEASE;
		return 1;
	}

	// Get or create a latch wait block and wait for someone to grant the latch.

	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;

	LatchWait* lwt;
	if (QUE_NOT_EMPTY(bcb->bcb_free_lwt))
	{
		QUE que_inst = bcb->bcb_free_lwt.que_forward;
		QUE_DELETE(*que_inst);
		lwt = (LatchWait*) BLOCK(que_inst, LatchWait*, lwt_waiters);
	}
	else
	{
		lwt = FB_NEW(*dbb->dbb_bufferpool) LatchWait;
		QUE_INIT(lwt->lwt_waiters);
	}

	lwt->lwt_flags |= LWT_pending;
	lwt->lwt_latch = type;
	lwt->lwt_tdbb = tdbb;

	// Give priority to IO.  This might prevent deadlocks while performing
	// precedence writes.  This does not cause starvation because an
	// exclusive latch is needed to dirty the page again.
	if ((type == LATCH_io) || (type == LATCH_mark)) {
		QUE_INSERT(bdb->bdb_waiters, lwt->lwt_waiters);
	}
	else {
		QUE_APPEND(bdb->bdb_waiters, lwt->lwt_waiters);
	}

	bool timeout_occurred = false;
	// Loop until the latch is granted or until a timeout occurs.
	while ((lwt->lwt_flags & LWT_pending) && !timeout_occurred)
	{
		//LATCH_MUTEX_RELEASE;
		Database::Checkout dcoHolder(dbb);
		timeout_occurred = !(lwt->lwt_sem.tryEnter(latch_wait > 0 ? 120 : -latch_wait));
		//LATCH_MUTEX_ACQUIRE;
	}

	bcb = dbb->dbb_bcb;			// Re-initialize
	QUE_DELETE(lwt->lwt_waiters);
	QUE_INSERT(bcb->bcb_free_lwt, lwt->lwt_waiters);

	// If the latch is not granted then a timeout must have occurred.
	if ((lwt->lwt_flags & LWT_pending) && timeout_occurred)
	{
		//LATCH_MUTEX_RELEASE;
		if (latch_wait == 1)
		{
			ERR_build_status(tdbb->tdbb_status_vector, Arg::Gds(isc_deadlock));
			CCH_unwind(tdbb, true);
		}
		else {
			return 1;
		}
	}

	if (bdb->bdb_page != page)
	{
		//LATCH_MUTEX_RELEASE;
		release_bdb(tdbb, bdb, true, false, false);
		return -1;
	}

	//LATCH_MUTEX_RELEASE;
	return 0;
}


static SSHORT lock_buffer(thread_db* tdbb, BufferDesc* bdb, const SSHORT wait, const SCHAR page_type)
{
/**************************************
 *
 *	l o c k _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Get a lock on page for a buffer.  If the lock ever slipped
 *	below READ, indicate that the page must be read.
 *
 * input:
 *	wait: LCK_WAIT = 1	=> Wait as long a necessary to get the lock.
 *	      LCK_NO_WAIT = 0	=> If the lock can't be acquired immediately,
 *						give up and return -1.
 *	      <negative number>		=> Lock timeout interval in seconds.
 *
 * return: 0  => buffer locked, page is already in memory.
 *	   1  => buffer locked, page needs to be read from disk.
 *	   -1 => timeout on lock occurred, see input parameter 'wait'.
 *
 **************************************/
	SET_TDBB(tdbb);
#ifdef SUPERSERVER
	return ((bdb->bdb_flags & BDB_read_pending) ? 1 : 0);
#else
	const USHORT lock_type = (bdb->bdb_flags & (BDB_dirty | BDB_writer)) ? LCK_write : LCK_read;
	Lock* const lock = bdb->bdb_lock;

	if (lock->lck_logical >= lock_type) {
		return 0;
	}

	TEXT errmsg[MAX_ERRMSG_LEN + 1];
	ISC_STATUS* const status = tdbb->tdbb_status_vector;

	if (lock->lck_logical == LCK_none)
	{
		// Prevent header and TIP pages from generating blocking AST
		// overhead. The promise is that the lock will unconditionally
		// be released when the buffer use count indicates it is safe to do so.

		if (page_type == pag_header || page_type == pag_transactions)
		{
			fb_assert(lock->lck_ast == blocking_ast_bdb);
			fb_assert(lock->lck_object == bdb);
			lock->lck_ast = 0;
			lock->lck_object = NULL;
		}
		else {
			fb_assert(lock->lck_ast != NULL);
		}

		//lock->lck_key.lck_long = bdb->bdb_page;
		bdb->bdb_page.getLockStr(lock->lck_key.lck_string);
		if (LCK_lock_opt(tdbb, lock, lock_type, wait))
		{
			if (!lock->lck_ast)
			{
				// Restore blocking AST to lock block if it was swapped out.
				// Flag the BufferDesc so that the lock is released when the buffer is released.

				fb_assert(page_type == pag_header || page_type == pag_transactions);
				lock->lck_ast = blocking_ast_bdb;
				lock->lck_object = bdb;
				bdb->bdb_flags |= BDB_no_blocking_ast;
			}
			return 1;
		}
		if (!lock->lck_ast)
		{
			fb_assert(page_type == pag_header || page_type == pag_transactions);
			lock->lck_ast = blocking_ast_bdb;
			lock->lck_object = bdb;
		}

		// Case: a timeout was specified, or the caller didn't want to wait, return the error.

		if ((wait == LCK_NO_WAIT) || ((wait < 0) && (status[1] == isc_lock_timeout)))
		{
			fb_utils::init_status(status);
			release_bdb(tdbb, bdb, false, false, false);
			return -1;
		}

		// Case: lock manager detected a deadlock, probably caused by locking the
		// BufferDesc's in an unfortunate order.  Nothing we can do about it, return the
		// error, and log it to firebird.log.

		fb_msg_format(0, JRD_BUGCHK, 216, sizeof(errmsg), errmsg,
			MsgFormat::SafeArg() << bdb->bdb_page.getPageNum() << (int) page_type);
		ERR_append_status(status, Arg::Gds(isc_random) << Arg::Str(errmsg));
		ERR_log(JRD_BUGCHK, 216, errmsg);	// msg 216 page %ld, page type %ld lock denied

		// CCH_unwind releases all the BufferDesc's and calls ERR_punt()
		// ERR_punt will longjump.

		CCH_unwind(tdbb, true);
	}

	// Lock requires an upward conversion.  Sigh.  Try to get the conversion.
	// If it fails, release the lock and re-seize. Save the contents of the
	// status vector just in case

	const SSHORT must_read = (lock->lck_logical < LCK_read) ? 1 : 0;

	ISC_STATUS_ARRAY alt_status;
	memcpy(alt_status, tdbb->tdbb_status_vector, sizeof(alt_status));

	if (LCK_convert_opt(tdbb, lock, lock_type)) {
		return must_read;
	}

	if (wait == LCK_NO_WAIT)
	{
		release_bdb(tdbb, bdb, true, false, false);
		return -1;
	}

	memcpy(tdbb->tdbb_status_vector, alt_status, sizeof(alt_status));

	if (LCK_lock(tdbb, lock, lock_type, wait)) {
		return 1;
	}

	// Case: a timeout was specified, or the caller didn't want to wait, return the error.

	if ((wait < 0) && (status[1] == isc_lock_timeout))
	{
		fb_utils::init_status(status);
		release_bdb(tdbb, bdb, false, false, false);
		return -1;
	}

	// Case: lock manager detected a deadlock, probably caused by locking the
	// BufferDesc's in an unfortunate order.  Nothing we can do about it, return the
	// error, and log it to firebird.log.

	fb_msg_format(0, JRD_BUGCHK, 215, sizeof(errmsg), errmsg,
					MsgFormat::SafeArg() << bdb->bdb_page.getPageNum() << (int) page_type);
	ERR_append_status(status, Arg::Gds(isc_random) << Arg::Str(errmsg));
	ERR_log(JRD_BUGCHK, 215, errmsg);	// msg 215 page %ld, page type %ld lock conversion denied

	CCH_unwind(tdbb, true);
	return 0;					// Added to get rid of Compiler Warning
#endif
}


static ULONG memory_init(thread_db* tdbb, BufferControl* bcb, SLONG number)
{
/**************************************
 *
 *	m e m o r y _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize memory for the cache.
 *	Return number of buffers allocated.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	UCHAR* memory = NULL;
	SLONG buffers = 0;
	const size_t page_size = dbb->dbb_page_size;
	size_t memory_size = page_size * (number + 1);
	fb_assert(memory_size > 0);

	SLONG old_buffers = 0;
	bcb_repeat* old_tail = NULL;
	const UCHAR* memory_end = NULL;
	bcb_repeat* tail = bcb->bcb_rpt;
	// "end" is changed inside the loop
	for (const bcb_repeat* end = tail + number; tail < end; tail++)
	{
		if (!memory)
		{
			// Allocate only what is required for remaining buffers.

			if (memory_size > (page_size * (number + 1))) {
				memory_size = page_size * (number + 1);
			}

			while (true)
			{
				try {
					memory = (UCHAR*) dbb->dbb_bufferpool->allocate(memory_size);
					break;
				}
				catch (Firebird::BadAlloc&)
				{
					// Either there's not enough virtual memory or there is
					// but it's not virtually contiguous. Let's find out by
					// cutting the size in half to see if the buffers can be
					// scattered over the remaining virtual address space.
					memory_size >>= 1;
					if (memory_size < MIN_BUFFER_SEGMENT)
					{
						// Diminishing returns
						return buffers;
					}
				}
			}

			bcb->bcb_memory.push(memory);
			memory_end = memory + memory_size;

			// Allocate buffers on an address that is an even multiple
			// of the page size (rather the physical sector size.) This
			// is a necessary condition to support raw I/O interfaces.
			memory = (UCHAR *) FB_ALIGN((U_IPTR) memory, page_size);
			old_tail = tail;
			old_buffers = buffers;
		}

		QUE_INIT(tail->bcb_page_mod);

		if (!(tail->bcb_bdb = alloc_bdb(tdbb, bcb, &memory)))
		{
			// Whoops! Time to reset our expectations. Release the buffer memory
			// but use that memory size to calculate a new number that takes into account
			// the page buffer overhead. Reduce this number by a 25% fudge factor to
			// leave some memory for useful work.

			dbb->dbb_bufferpool->deallocate(bcb->bcb_memory.pop());
			memory = NULL;
			for (bcb_repeat* tail2 = old_tail; tail2 < tail; tail2++)
			{
				tail2->bcb_bdb = dealloc_bdb(tail2->bcb_bdb);
			}
			number = memory_size / PAGE_OVERHEAD;
			number -= number >> 2;
			end = old_tail + number;
			tail = --old_tail;	// For loop continue pops tail above
			buffers = old_buffers;
			continue;
		}

		buffers++;				// Allocated buffers
		number--;				// Remaining buffers

		// Check if memory segment has been exhausted.

		if (memory + page_size > memory_end) {
			memory = 0;
		}
	}

	return buffers;
}


static void page_validation_error(thread_db* tdbb, WIN* window, SSHORT type)
{
/**************************************
 *
 *	p a g e _ v a l i d a t i o n _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	We've detected a validation error on fetch.  Generally
 *	we've detected that the type of page fetched didn't match the
 *	type of page we were expecting.  Report an error and
 *	get out.
 *	This function will only be called rarely, as a page validation
 *	error is an indication of on-disk database corruption.
 *
 **************************************/
	SET_TDBB(tdbb);
	BufferDesc* bdb = window->win_bdb;
	const pag* page = bdb->bdb_buffer;

	PageSpace* pages =
		tdbb->getDatabase()->dbb_page_manager.findPageSpace(bdb->bdb_page.getPageSpaceID());

	ERR_build_status(tdbb->tdbb_status_vector,
					 Arg::Gds(isc_db_corrupt) << Arg::Str(pages->file->fil_string) <<
					 Arg::Gds(isc_page_type_err) <<
					 Arg::Gds(isc_badpagtyp) << Arg::Num(bdb->bdb_page.getPageNum()) <<
												Arg::Num(type) <<
												Arg::Num(page->pag_type));
	// We should invalidate this bad buffer.
	CCH_unwind(tdbb, true);
}


#ifdef CACHE_READER
static void prefetch_epilogue(Prefetch* prefetch, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	p r e f e t c h _ e p i l o g u e
 *
 **************************************
 *
 * Functional description
 *	Stall on asynchronous I/O completion.
 *	Move data from prefetch buffer to database
 *	buffers, compute the checksum, and release
 *	the latch.
 *
 **************************************/
	if (!(prefetch->prf_flags & PRF_active)) {
		return;
	}

	thread_db* tdbb = prefetch->prf_tdbb;
	Database* dbb = tdbb->getDatabase();

	prefetch->prf_piob.piob_wait = TRUE;
	const bool async_status = PIO_status(dbb, &prefetch->prf_piob, status_vector);

	// If there was an I/O error release all buffer latches acquired for the prefetch request.

	if (!async_status)
	{
		BufferDesc** next_bdb = prefetch->prf_bdbs;
		for (USHORT i = 0; i < prefetch->prf_max_prefetch; i++)
		{
			if (*next_bdb) {
				release_bdb(tdbb, *next_bdb, true, false, false);
			}
			next_bdb++;
		}
		prefetch->prf_flags &= ~PRF_active;
		return;
	}

	const SCHAR* next_buffer = prefetch->prf_io_buffer;
	BufferDesc** next_bdb = prefetch->prf_bdbs;

	for (USHORT i = 0; i < prefetch->prf_max_prefetch; i++)
	{
		if (*next_bdb)
		{
			pag* page = (*next_bdb)->bdb_buffer;
			if (next_buffer != reinterpret_cast<char*>(page)) {
				memcpy(page, next_buffer, (ULONG) dbb->dbb_page_size);
			}
			if (page->pag_checksum == CCH_checksum(*next_bdb))
			{
				(*next_bdb)->bdb_flags &= ~(BDB_read_pending | BDB_not_valid);
				(*next_bdb)->bdb_flags |= BDB_prefetch;
			}
			release_bdb(tdbb, *next_bdb, true, false, false);
		}
		next_buffer += dbb->dbb_page_size;
		next_bdb++;
	}

	prefetch->prf_flags &= ~PRF_active;
}


static void prefetch_init(Prefetch* prefetch, thread_db* tdbb)
{
/**************************************
 *
 *	p r e f e t c h _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize prefetch data structure.
 *	Most systems that allow access to "raw" I/O
 *	interfaces want the buffer address aligned.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

	prefetch->prf_tdbb = tdbb;
	prefetch->prf_flags = 0;
	prefetch->prf_max_prefetch = PREFETCH_MAX_TRANSFER / dbb->dbb_page_size;
	prefetch->prf_aligned_buffer =
		(SCHAR*) (((U_IPTR) &prefetch->prf_unaligned_buffer + MIN_PAGE_SIZE - 1) &
		~((U_IPTR) MIN_PAGE_SIZE - 1));
}


static void prefetch_io(Prefetch* prefetch, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	p r e f e t c h _ i o
 *
 **************************************
 *
 * Functional description
 *	Queue an asynchronous I/O to read
 *	multiple pages into prefetch buffer.
 *
 **************************************/
	thread_db* tdbb = prefetch->prf_tdbb;
	Database* dbb = tdbb->getDatabase();

	if (!prefetch->prf_page_count) {
		prefetch->prf_flags &= ~PRF_active;
	}
	else
	{
		// Get the cache reader working on our behalf too

		if (!(dbb->dbb_bcb->bcb_flags & BCB_reader_active)) {
			dbb->dbb_reader_sem.post();
		}

		const bool async_status =
			PIO_read_ahead(dbb, prefetch->prf_start_page, prefetch->prf_io_buffer,
						   prefetch->prf_page_count, &prefetch->prf_piob, status_vector);
		if (!async_status)
		{
			BufferDesc** next_bdb = prefetch->prf_bdbs;
			for (USHORT i = 0; i < prefetch->prf_max_prefetch; i++)
			{
				if (*next_bdb) {
					release_bdb(tdbb, *next_bdb, true, false, false);
				}
				next_bdb++;
			}
			prefetch->prf_flags &= ~PRF_active;
		}
	}
}


static void prefetch_prologue(Prefetch* prefetch, SLONG* start_page)
{
/**************************************
 *
 *	p r e f e t c h _ p r o l o g u e
 *
 **************************************
 *
 * Functional description
 *	Search for consecutive pages to be prefetched
 *	and latch them for I/O.
 *
 **************************************/
	thread_db* tdbb = prefetch->prf_tdbb;
	Database* dbb = tdbb->getDatabase();
	BufferControl* bcb = dbb->dbb_bcb;

	prefetch->prf_start_page = *start_page;
	prefetch->prf_page_count = 0;
	prefetch->prf_flags |= PRF_active;

	BufferDesc** next_bdb = prefetch->prf_bdbs;
	for (USHORT i = 0; i < prefetch->prf_max_prefetch; i++)
	{
		*next_bdb = 0;
		if (SBM_clear(bcb->bcb_prefetch, *start_page) &&
			(*next_bdb = get_buffer(tdbb, *start_page, LATCH_shared, 0)))
		{
			if ((*next_bdb)->bdb_flags & BDB_read_pending) {
				prefetch->prf_page_count = i + 1;
			}
			else
			{
				release_bdb(tdbb, *next_bdb, true, false, false);
				*next_bdb = 0;
			}
		}
		next_bdb++;
		(*start_page)++;
	}

	// Optimize non-sequential list prefetching to transfer directly to database buffers.

	BufferDesc* bdb;
	if (prefetch->prf_page_count == 1 && (bdb = prefetch->prf_bdbs[0])) {
		prefetch->prf_io_buffer = reinterpret_cast<char*>(bdb->bdb_buffer);
	}
	else {
		prefetch->prf_io_buffer = prefetch->prf_aligned_buffer;
	}

	// Reset starting page for next bitmap walk

	--(*start_page);
}
#endif // CACHE_READER


static SSHORT related(BufferDesc* low, const BufferDesc* high, SSHORT limit, const ULONG mark)
{
/**************************************
 *
 *	r e l a t e d
 *
 **************************************
 *
 * Functional description
 *	See if there are precedence relationships linking two buffers.
 *	Since precedence graphs can become very complex, limit search for
 *	precedence relationship by visiting a presribed limit of higher
 *	precedence blocks.
 *
 **************************************/
	const struct que* base = &low->bdb_higher;

	for (const struct que* que_inst = base->que_forward; que_inst != base; que_inst = que_inst->que_forward)
	{
		if (!--limit) {
			return PRE_UNKNOWN;
		}
		const Precedence* precedence = BLOCK(que_inst, Precedence*, pre_higher);
		if (!(precedence->pre_flags & PRE_cleared))
		{
			if (precedence->pre_hi->bdb_prec_walk_mark == mark)
				continue;

			if (precedence->pre_hi == high) {
				return PRE_EXISTS;
			}

			if (QUE_NOT_EMPTY(precedence->pre_hi->bdb_higher))
			{
				limit = related(precedence->pre_hi, high, limit, mark);
				if (limit == PRE_EXISTS || limit == PRE_UNKNOWN) {
					return limit;
				}
			}
			else
				precedence->pre_hi->bdb_prec_walk_mark = mark;
		}
	}

	low->bdb_prec_walk_mark = mark;
	return limit;
}


static void release_bdb(thread_db* tdbb,
						BufferDesc* bdb,
						const bool repost,
						const bool downgrade_latch,
						const bool rel_mark_latch)
{
/**************************************
 *
 *	r e l e a s e _ b d b
 *
 **************************************
 *
 * Functional description
 *	Decrement the use count of a BufferDesc, reposting
 *	blocking AST if required.
 *	If rel_mark_latch is true, the value of downgrade_latch is ignored.
 *
 **************************************/

	//LATCH_MUTEX_ACQUIRE;

	que* const wait_que = &bdb->bdb_waiters;

	// Releasing a LATCH_mark.
	if (rel_mark_latch)
	{
		if ((bdb->bdb_io != tdbb) || (bdb->bdb_exclusive != tdbb)) {
			BUGCHECK(294);	// inconsistent LATCH_mark release
		}
		bdb->bdb_io = 0;
	}
	else if (downgrade_latch)
	{
		// Downgrading from an exlusive to a shared latch.
		// Only the transition from exclusive to shared is supported.
		// If an actual state changed, then we need to check if waiters
		// can be granted.  Otherwise, there is nothing further to do.
		if (bdb->bdb_io == tdbb) {
			BUGCHECK(296);	// inconsistent latch downgrade call
		}
		if (bdb->bdb_exclusive == tdbb)
		{
			bdb->bdb_exclusive = 0;
			allocSharedLatch(tdbb, bdb);
		}
		else
		{
			//LATCH_MUTEX_RELEASE;
			return;
		}
	}
	else if (bdb->bdb_exclusive == tdbb)
	{
		// If the exclusive latch is held, then certain code does funny things:
		// ail.c does: exclusive - mark - exclusive
		// CVC: but this comment was related to our obsolete WAL facility.
		--bdb->bdb_use_count;
		--tdbb->tdbb_latch_count;
		if (!bdb->bdb_use_count)
		{
			// All latches are released
			bdb->bdb_exclusive = bdb->bdb_io = 0;
			while (QUE_NOT_EMPTY(bdb->bdb_shared))
			{
				SharedLatch* latch = BLOCK(bdb->bdb_shared.que_forward, SharedLatch*, slt_bdb_que);
				freeSharedLatch(tdbb, bdb->bdb_dbb->dbb_bcb, latch);
			}
		}
		else if (bdb->bdb_io)
		{
			// This is a release for an io or an exclusive latch
			if (bdb->bdb_io == tdbb)
			{
				// We have an io latch

				// The BDB_must_write flag causes the system to latch for io, in addition
				// to the already owned latches.  Make sure not to disturb an already existing
				// exclusive latch.
				// ail.c does: EX => MARK => SHARED => release => EX => MARK => RELEASE => EX
				// CVC: ail.c was related to our obsolete WAL facility.
				if (!(bdb->bdb_flags & BDB_marked)) {
					bdb->bdb_io = 0;
				}
			}
			else if (bdb->bdb_use_count == 1)
			{
				// This must be a release for our exclusive latch
				bdb->bdb_exclusive = 0;
			}
		}
		else
		{
			// This is a release for a shared latch
			SharedLatch* latch = findSharedLatch(tdbb, bdb);
			if (latch) {
				freeSharedLatch(tdbb, bdb->bdb_dbb->dbb_bcb, latch);
			}
		}
	}
	else
	{
		// If the exclusive latch is not held, then things have to behave much nicer.
		if (bdb->bdb_flags & BDB_marked) {
			BUGCHECK(297);	// bdb is unexpectedly marked
		}
		--bdb->bdb_use_count;
		--tdbb->tdbb_latch_count;
		if (bdb->bdb_io == tdbb) {
			bdb->bdb_io = 0;
		}
		else
		{
			SharedLatch* latch = findSharedLatch(tdbb, bdb);
			if (!latch) {
				BUGCHECK(300);	// can't find shared latch
			}
			freeSharedLatch(tdbb, bdb->bdb_dbb->dbb_bcb, latch);
		}
	}

	bool granted = false;

	for (QUE que_inst = wait_que->que_forward; que_inst != wait_que; que_inst = que_inst->que_forward)
	{
		// Note that this loop assumes that requests for LATCH_io and LATCH_mark
		// are queued before LATCH_shared and LATCH_exclusive.
		LatchWait* lwt = BLOCK(que_inst, LatchWait*, lwt_waiters);
		if (lwt->lwt_flags & LWT_pending)
		{
			switch (lwt->lwt_latch)
			{
			case LATCH_exclusive:
				if (bdb->bdb_use_count)
				{
					//LATCH_MUTEX_RELEASE;
					return;
				}
				++bdb->bdb_use_count;
				++lwt->lwt_tdbb->tdbb_latch_count;
				bdb->bdb_exclusive = lwt->lwt_tdbb;
				lwt->lwt_flags &= ~LWT_pending;
				lwt->lwt_sem.release();
				//LATCH_MUTEX_RELEASE;
				return;

			case LATCH_io:
				if (!bdb->bdb_io)
				{
					++bdb->bdb_use_count;
					++lwt->lwt_tdbb->tdbb_latch_count;
					bdb->bdb_io = lwt->lwt_tdbb;
					lwt->lwt_flags &= ~LWT_pending;
					lwt->lwt_sem.release();
					granted = true;
				}
				break;

			case LATCH_mark:
				if (bdb->bdb_exclusive != lwt->lwt_tdbb) {
					BUGCHECK(298);	// missing exclusive latch
				}
				if (!bdb->bdb_io)
				{
					bdb->bdb_io = lwt->lwt_tdbb;
					lwt->lwt_flags &= ~LWT_pending;
					lwt->lwt_sem.release();
					granted = true;
				}
				break;

			case LATCH_shared:
				if (bdb->bdb_exclusive)
				{
					break;		// defensive programming

					// correct programming
					/*
					//LATCH_MUTEX_RELEASE;
					return;
					*/
				}
				++bdb->bdb_use_count;
				++lwt->lwt_tdbb->tdbb_latch_count;
				allocSharedLatch(lwt->lwt_tdbb, bdb);
				lwt->lwt_flags &= ~LWT_pending;
				lwt->lwt_sem.release();
				granted = true;
				break;
			}

			if (granted && (bdb->bdb_flags & BDB_read_pending))
			{
				// Allow only one reader to proceed
				break;
			}
		}
	}

	if (bdb->bdb_use_count || !repost)
	{
		//LATCH_MUTEX_RELEASE;
		return;
	}

	//LATCH_MUTEX_RELEASE;

	if (bdb->bdb_ast_flags & BDB_blocking)
	{
		PAGE_LOCK_RE_POST(bdb->bdb_lock);
	}
}


static void unmark(thread_db* tdbb, WIN* window)
{
/**************************************
 *
 *	u n m a r k
 *
 **************************************
 *
 * Functional description
 *	Unmark a BufferDesc.  Called when the update of a page is
 *	complete and delaying the 'unmarking' could cause
 *	problems.
 *
 **************************************/
	SET_TDBB(tdbb);
	BufferDesc* bdb = window->win_bdb;
	BLKCHK(bdb, type_bdb);

	if (bdb->bdb_use_count == 1)
	{
		const bool marked = bdb->bdb_flags & BDB_marked;
		bdb->bdb_flags &= ~BDB_marked;
		if (marked) {
			release_bdb(tdbb, bdb, false, false, true);
		}
	}
}


static inline bool writeable(BufferDesc* bdb)
{
/**************************************
 *
 *	w r i t e a b l e
 *
 **************************************
 *
 * Functional description
 *	See if a buffer is writeable.  A buffer is writeable if
 *	neither it nor any of it's higher precedence cousins are
 *	marked for write.
 *  This is the starting point of recursive walk of precedence
 *  graph. The writeable_mark member is used to mark already seen
 *  buffers to avoid repeated walk of the same sub-graph.
 *  Currently this function can't be called from more than one
 *  thread simultaneously. When SMP will be implemented we must
 *  take additional care about thread-safety.
 *
 **************************************/
	if (bdb->bdb_flags & BDB_marked) {
		return false;
	}

	BufferControl* bcb = bdb->bdb_dbb->dbb_bcb;
	const ULONG mark = get_prec_walk_mark(bcb);
	return is_writeable(bdb, mark);
}


static bool is_writeable(BufferDesc* bdb, const ULONG mark)
{
/**************************************
 *
 *	i s _ w r i t e a b l e
 *
 **************************************
 *
 * Functional description
 *	See if a buffer is writeable.  A buffer is writeable if
 *	neither it nor any of it's higher precedence cousins are
 *	marked for write.
 *
 **************************************/

	// If there are buffers that must be written first, check them, too.

	for (const que* queue = bdb->bdb_higher.que_forward;
		queue != &bdb->bdb_higher; queue = queue->que_forward)
	{
		const Precedence* precedence = BLOCK(queue, Precedence*, pre_higher);

		if (!(precedence->pre_flags & PRE_cleared))
		{
			BufferDesc* high = precedence->pre_hi;

			if (high->bdb_flags & BDB_marked) {
				return false;
			}

			if (high->bdb_prec_walk_mark != mark)
			{
				if (QUE_EMPTY(high->bdb_higher))
					high->bdb_prec_walk_mark = mark;
				else if (!is_writeable(high, mark))
					return false;
			}
		}
	}

	bdb->bdb_prec_walk_mark = mark;
	return true;
}


static int write_buffer(thread_db* tdbb,
						BufferDesc* bdb,
						const PageNumber page,
						const bool write_thru,
						ISC_STATUS* const status, const bool write_this_page)
{
/**************************************
 *
 *	w r i t e _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Write a dirty buffer.  This may recurse due to
 *	precedence problems.
 *
 * input:  write_this_page
 *		= true if the input page needs to be written
 *			before returning.  (normal case)
 *		= false if the input page is being written
 *			because of precedence.  Only write
 *			one page and return so that the caller
 *			can re-establish the need to write this
 *			page.
 *
 * return: 0 = Write failed.
 *         1 = Page is written.  Page was written by this
 *     		call, or was written by someone else, or the
 *		cache buffer is already reassigned.
 *	   2 = Only possible if write_this_page is false.
 *		This input page is not written.  One
 *		page higher in precedence is written
 *		though.  Probable action: re-establich the
 * 		need to write this page and retry write.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	if (latch_bdb(tdbb, LATCH_io, bdb, page, 1) == -1) {
		return 1;
	}

	if ((bdb->bdb_flags & BDB_marked) && !(bdb->bdb_flags & BDB_faked)) {
		BUGCHECK(217);	// msg 217 buffer marked for update
	}

	if (!(bdb->bdb_flags & BDB_dirty) && !(write_thru && bdb->bdb_flags & BDB_db_dirty))
	{
		clear_precedence(tdbb, bdb);
		release_bdb(tdbb, bdb, true, false, false);
		return 1;
	}

	// If there are buffers that must be written first, write them now.

	//PRE_MUTEX_ACQUIRE;

	while (QUE_NOT_EMPTY(bdb->bdb_higher))
	{
		BufferControl* const bcb = dbb->dbb_bcb;		// Re-initialize in the loop
		QUE que_inst = bdb->bdb_higher.que_forward;
		Precedence* precedence = BLOCK(que_inst, Precedence*, pre_higher);
		if (precedence->pre_flags & PRE_cleared)
		{
			QUE_DELETE(precedence->pre_higher);
			QUE_DELETE(precedence->pre_lower);
			precedence->pre_hi = (BufferDesc*) bcb->bcb_free;
			bcb->bcb_free = precedence;
		}
		else
		{
			BufferDesc* hi_bdb = precedence->pre_hi;
			const PageNumber hi_page = hi_bdb->bdb_page;
			//PRE_MUTEX_RELEASE;
			release_bdb(tdbb, bdb, false, false, false);
			const int write_status = write_buffer(tdbb, hi_bdb, hi_page, write_thru, status, false);
			if (write_status == 0) {
				return 0;		// return IO error
			}
#ifdef SUPERSERVER
			if (!write_this_page)
			{
				return 2;
				// caller wants to re-establish the need for this write after one precedence write
			}
#endif
			if (latch_bdb(tdbb, LATCH_io, bdb, page, 1) == -1) {
				return 1;		// cache buffer reassigned, return 'write successful'
			}
			//PRE_MUTEX_ACQUIRE;
		}
	}

	//PRE_MUTEX_RELEASE;

#ifdef SUPERSERVER_V2
	// Header page I/O is deferred until a dirty page, which was modified by a
	// transaction not updated on the header page, needs to be written. Note
	// that the header page needs to be written with the target page latched to
	// prevent younger transactions from modifying the target page.

	if (page != HEADER_PAGE_NUMBER && bdb->bdb_mark_transaction > dbb->dbb_last_header_write)
	{
		TRA_header_write(tdbb, dbb, bdb->bdb_mark_transaction);
	}
#endif

	// Unless the buffer has been faked (recently re-allocated), write out the page

	bool result = true;
	if ((bdb->bdb_flags & BDB_dirty || (write_thru && bdb->bdb_flags & BDB_db_dirty)) &&
		!(bdb->bdb_flags & BDB_marked))
	{
		if ( (result = write_page(tdbb, bdb, /*write_thru,*/ status, false)) ) {
			clear_precedence(tdbb, bdb);
		}
	}
	else {
		clear_precedence(tdbb, bdb);
	}

	release_bdb(tdbb, bdb, true, false, false);

	if (!result) {
		return 0;
	}

#ifdef SUPERSERVER
	if (!write_this_page) {
		return 2;
	}
#endif

	return 1;
}


static bool write_page(thread_db* tdbb,
					   BufferDesc* bdb,
					   //const bool write_thru,
					   ISC_STATUS* const status,
					   const bool inAst)
{
/**************************************
 *
 *	w r i t e _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Do actions required when writing a database page,
 *	including journaling, shadowing.
 *
 **************************************/
	if (bdb->bdb_flags & BDB_not_valid)
	{
		ERR_build_status(status, Arg::Gds(isc_buf_invalid) << Arg::Num(bdb->bdb_page.getPageNum()));
		return false;
	}

	Database* const dbb = bdb->bdb_dbb;
	pag* const page = bdb->bdb_buffer;

	// Before writing db header page, make sure that
	// the next_transaction > oldest_active transaction
	if (bdb->bdb_page == HEADER_PAGE_NUMBER)
	{
		const header_page* header = (header_page*) page;
		if (header->hdr_next_transaction)
		{
			if (header->hdr_oldest_active > header->hdr_next_transaction) {
				BUGCHECK(266);	// next transaction older than oldest active
			}

			if (header->hdr_oldest_transaction > header->hdr_next_transaction) {
				BUGCHECK(267);	// next transaction older than oldest transaction
			}
		}
	}

	page->pag_generation++;
	bool result = true;

	//if (!dbb->dbb_wal || write_thru) becomes
	//if (true || write_thru) then finally if (true)
	// I won't wipe out the if() itself to allow my changes be verified easily by others
	if (true)
	{
		tdbb->bumpStats(RuntimeStatistics::PAGE_WRITES);

		// write out page to main database file, and to any
		// shadows, making a special case of the header page
		BackupManager* bm = dbb->dbb_backup_manager;
		const int backup_state = bm->getState();

		if (bdb->bdb_page.getPageNum() >= 0)
		{
			fb_assert(backup_state != nbak_state_unknown);
			page->pag_checksum = CCH_checksum(bdb);

#ifdef NBAK_DEBUG
			// We cannot call normal trace functions here as they are signal-unsafe
			// "Write page=%d, dir=%d, diff=%d, scn=%d"
			char buffer[1000], *ptr = buffer;
			strcpy(ptr, "NBAK, Write page ");
			ptr += strlen(ptr);
			gds__ulstr(ptr, bdb->bdb_page.getPageNum(), 0, 0);
			ptr += strlen(ptr);

			strcpy(ptr, ", backup_state=");
			ptr += strlen(ptr);
			gds__ulstr(ptr, backup_state, 0, 0);
			ptr += strlen(ptr);

			strcpy(ptr, ", diff=");
			ptr += strlen(ptr);
			gds__ulstr(ptr, bdb->bdb_difference_page, 0, 0);
			ptr += strlen(ptr);

			strcpy(ptr, ", scn=");
			ptr += strlen(ptr);
			gds__ulstr(ptr, bdb->bdb_buffer->pag_scn, 0, 0);
			ptr += strlen(ptr);

			gds__trace(buffer);
#endif
			PageSpace* pageSpace =
				dbb->dbb_page_manager.findPageSpace(bdb->bdb_page.getPageSpaceID());
			fb_assert(pageSpace);
			const bool isTempPage = pageSpace->isTemporary();

			if (!isTempPage && (backup_state == nbak_state_stalled ||
				(backup_state == nbak_state_merge && bdb->bdb_difference_page) ))
			{

				const bool res = dbb->dbb_backup_manager->writeDifference(status,
									bdb->bdb_difference_page, bdb->bdb_buffer);

				if (!res)
				{
					bdb->bdb_flags |= BDB_io_error;
					dbb->dbb_flags |= DBB_suspend_bgio;
					return false;
				}
			}
			if (!isTempPage && backup_state == nbak_state_stalled)
			{
				// We finished. Adjust transaction accounting and get ready for exit
				if (bdb->bdb_page == HEADER_PAGE_NUMBER) {
					dbb->dbb_last_header_write = ((header_page*) page)->hdr_next_transaction;
				}
			}
			else
			{
				// We need to write our pages to main database files

				jrd_file* file = pageSpace->file;
				while (!PIO_write(file, bdb, page, status))
				{

					if (isTempPage || !CCH_rollover_to_shadow(tdbb, dbb, file, inAst))
					{
						bdb->bdb_flags |= BDB_io_error;
						dbb->dbb_flags |= DBB_suspend_bgio;
						return false;
					}

					file = pageSpace->file;
				}

				if (bdb->bdb_page == HEADER_PAGE_NUMBER) {
					dbb->dbb_last_header_write = ((header_page*) page)->hdr_next_transaction;
				}
				if (dbb->dbb_shadow && !isTempPage) {
					result = CCH_write_all_shadows(tdbb, 0, bdb, status, 0, inAst);
				}
			}
		}

#ifdef SUPERSERVER
		if (result)
		{
#ifdef CACHE_WRITER
			if (bdb->bdb_flags & BDB_checkpoint) {
				--dbb->dbb_bcb->bcb_checkpoint;
			}
#endif
			bdb->bdb_flags &= ~(BDB_db_dirty | BDB_checkpoint);
		}
#endif
	}

	if (!result)
	{
		// If there was a write error then idle background threads
		// so that they don't spin trying to write these pages. This
		// usually results from device full errors which can be fixed
		// by someone freeing disk space.

		bdb->bdb_flags |= BDB_io_error;
		dbb->dbb_flags |= DBB_suspend_bgio;
	}
	else
	{
		// clear the dirty bit vector, since the buffer is now
		// clean regardless of which transactions have modified it

		// Destination difference page number is only valid between MARK and
		// write_page so clean it now to avoid confusion
		bdb->bdb_difference_page = 0;
		bdb->bdb_transactions = bdb->bdb_mark_transaction = 0;
#ifdef DIRTY_LIST
		if (!(dbb->dbb_bcb->bcb_flags & BCB_keep_pages)) {
			removeDirty(dbb->dbb_bcb, bdb);
		}
#endif
#ifdef DIRTY_TREE
		if (!(dbb->dbb_bcb->bcb_flags & BCB_keep_pages) &&
			(bdb->bdb_parent || bdb == dbb->dbb_bcb->bcb_btree))
		{
			btc_remove(bdb);
		}
#endif

		bdb->bdb_flags &= ~(BDB_must_write | BDB_system_dirty);
		clear_dirty_flag(tdbb, bdb);

		if (bdb->bdb_flags & BDB_io_error)
		{
			// If a write error has cleared, signal background threads
			// to resume their regular duties. If someone has freed up
			// disk space these errors will spontaneously go away.

			bdb->bdb_flags &= ~BDB_io_error;
			dbb->dbb_flags &= ~DBB_suspend_bgio;
		}
	}

	return result;
}

static void set_dirty_flag(thread_db* tdbb, BufferDesc* bdb)
{
	if ( !(bdb->bdb_flags & BDB_dirty) )
	{
		NBAK_TRACE(("lock state for dirty page %d:%06d",
			bdb->bdb_page.getPageSpaceID(), bdb->bdb_page.getPageNum()));
		bdb->bdb_flags |= BDB_dirty;
		tdbb->getDatabase()->dbb_backup_manager->lockDirtyPage(tdbb);
	}
}

static void clear_dirty_flag(thread_db* tdbb, BufferDesc* bdb)
{
	if (bdb->bdb_flags & BDB_dirty)
	{
		bdb->bdb_flags &= ~BDB_dirty;
		NBAK_TRACE(("unlock state for dirty page %d:%06d",
			bdb->bdb_page.getPageSpaceID(), bdb->bdb_page.getPageNum()));
		tdbb->getDatabase()->dbb_backup_manager->unlockDirtyPage(tdbb);
	}
}


