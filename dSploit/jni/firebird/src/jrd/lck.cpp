/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lck.cpp
 *	DESCRIPTION:	Lock handler for JRD (not lock manager!)
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include "../jrd/jrd.h"
#include "../jrd/lck.h"
#include "gen/iberror.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/gdsassert.h"
#include "../lock/lock_proto.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN_NT
#include <process.h>
#endif


using namespace Jrd;
using namespace Firebird;

static void bug_lck(const TEXT*);
static bool compatible(const Lock*, const Lock*, USHORT);
static void enqueue(thread_db*, Lock*, USHORT, SSHORT);
static int external_ast(void*);
static USHORT hash_func(const UCHAR*, USHORT);
static void hash_allocate(Lock*);
static Lock* hash_get_lock(Lock*, USHORT*, Lock***);
static void hash_insert_lock(Lock*);
static bool hash_remove_lock(Lock*, Lock**);
static void internal_ast(Lock*);
static bool internal_compatible(Lock*, const Lock*, USHORT);
static void internal_dequeue(thread_db*, Lock*);
static USHORT internal_downgrade(thread_db*, Lock*);
static bool internal_enqueue(thread_db*, Lock*, USHORT, SSHORT, bool);

static void set_lock_attachment(Lock*, Attachment*);


// globals and macros

#ifdef SUPERSERVER

inline LOCK_OWNER_T LCK_OWNER_ID_DBB(thread_db* tdbb)
{
	return (LOCK_OWNER_T) getpid() << 32 | tdbb->getDatabase()->dbb_lock_owner_id;
}
inline LOCK_OWNER_T LCK_OWNER_ID_ATT(thread_db* tdbb)
{
	return (LOCK_OWNER_T) getpid() << 32 | tdbb->getAttachment()->att_lock_owner_id;
}

inline SLONG* LCK_OWNER_HANDLE_DBB(thread_db* tdbb)
{
	return &tdbb->getDatabase()->dbb_lock_owner_handle;
}
inline SLONG* LCK_OWNER_HANDLE_ATT(thread_db* tdbb)
{
	return &tdbb->getAttachment()->att_lock_owner_handle;
}

#else	// SUPERSERVER

inline LOCK_OWNER_T LCK_OWNER_ID_DBB(thread_db* tdbb)
{
	return (LOCK_OWNER_T) getpid() << 32 | tdbb->getDatabase()->dbb_lock_owner_id;
}
inline LOCK_OWNER_T LCK_OWNER_ID_ATT(thread_db* tdbb)
{
	return (LOCK_OWNER_T) getpid() << 32 | tdbb->getDatabase()->dbb_lock_owner_id;
}

inline SLONG* LCK_OWNER_HANDLE_DBB(thread_db* tdbb)
{
	return &tdbb->getDatabase()->dbb_lock_owner_handle;
}
inline SLONG* LCK_OWNER_HANDLE_ATT(thread_db* tdbb)
{
	return &tdbb->getDatabase()->dbb_lock_owner_handle;
}

#endif	// SUPERSERVER


static const bool compatibility[LCK_max][LCK_max] =
{

/*							Shared	Prot	Shared	Prot
			none	null	Read	Read	Write	Write	Exclusive */
/* none */	{true,	true,	true,	true,	true,	true,	true},
/* null */	{true,	true,	true,	true,	true,	true,	true},
/* SR	*/	{true,	true,	true,	true,	true,	true,	false},
/* PR	*/	{true,	true,	true,	true,	false,	false,	false},
/* SW	*/	{true,	true,	true,	false,	true,	false,	false},
/* PW	*/	{true,	true,	true,	false,	false,	false,	false},
/* EX	*/	{true,	true,	false,	false,	false,	false,	false}
};

//#define COMPATIBLE(st1, st2)	compatibility [st1 * LCK_max + st2]
const int LOCK_HASH_SIZE	= 19;


inline void ENQUEUE(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
	if (lock->lck_compatible)
		internal_enqueue(tdbb, lock, level, wait, false);
	else
		enqueue(tdbb, lock, level, wait);
}

inline bool CONVERT(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
	Database* const dbb = tdbb->getDatabase();

	return lock->lck_compatible ?
		internal_enqueue(tdbb, lock, level, wait, true) :
		dbb->dbb_lock_mgr->convert(tdbb, lock->lck_id, level, wait, lock->lck_ast, lock->lck_object);
}

inline void DEQUEUE(thread_db* tdbb, Lock* lock)
{
	Database* const dbb = tdbb->getDatabase();

	if (lock->lck_compatible)
		internal_dequeue(tdbb, lock);
	else
		dbb->dbb_lock_mgr->dequeue(lock->lck_id);
}

inline USHORT DOWNGRADE(thread_db* tdbb, Lock* lock)
{
	Database* const dbb = tdbb->getDatabase();

	return lock->lck_compatible ?
		internal_downgrade(tdbb, lock) :
		dbb->dbb_lock_mgr->downgrade(tdbb, lock->lck_id);
}

#ifdef DEV_BUILD
/* Valid locks are not NULL,
	of the right memory block type,
	are attached to a dbb.
   	If we have the dbb in non-exclusive mode,
	    then we must have a physical lock of at least the same level
	    as the logical lock.
   	If we don't have a lock ID,
	    then we better not have a physical lock at any level.

JMB: As part of the c++ conversion I removed the check for Lock block type.
 There is no more blk_type field in the Lock structure, and some stack allocated
 Lock's are passed into lock functions, so we can't do the check.
 Here is the line I removed from the macro:
				 (l->blk_type == type_lck) && \
 */
#define LCK_CHECK_LOCK checkLock

inline bool checkLock(const Lock* l)
{
	return (l != NULL && l->lck_length <= MAX_UCHAR && l->lck_dbb != NULL &&
			(l->lck_id || l->lck_physical == LCK_none));
}

/* The following check should be part of LCK_CHECK_LOCK, but it fails
   when the exclusive attachment to a database is changed to a shared
   attachment.  When that occurs we fb_assert all our internal locks, but
   while we are in the process of asserting DBB_assert_locks is set, but
   we haven't gotten physical locks yet.

			         (!(l->lck_dbb->dbb_ast_flags & DBB_assert_locks) || \
				     (l->lck_physical >= l->lck_logical)) && \
*/
#endif

#ifndef LCK_CHECK_LOCK
#define LCK_CHECK_LOCK(x)	true	// nothing
#endif

namespace {
// This class is used as a guard around long waiting call into LM and has
// two purposes :
//	- set and restore att_wait_lock while waiting inside the LM
//	- set or clear and restore TDBB_wait_cancel_disable flag in dependence
//	  of safety of cancelling lock waiting. Currently we can safely cancel
//	  only LCK_tra locks

class WaitCancelGuard
{
public:
	WaitCancelGuard(thread_db* tdbb, Lock* lock, int wait) :
	  m_tdbb(tdbb),
	  m_save_lock(NULL)
	{
		Attachment* att = m_tdbb->getAttachment();
		if (att)
			m_save_lock = att->att_wait_lock;

		m_cancel_disabled = (m_tdbb->tdbb_flags & TDBB_wait_cancel_disable); 
		m_tdbb->tdbb_flags |= TDBB_wait_cancel_disable;

		if (!wait)
			return;

		switch (lock->lck_type)
		{
		case LCK_tra:
			m_tdbb->tdbb_flags &= ~TDBB_wait_cancel_disable;
			if (att)
				att->att_wait_lock = lock;
		break;

		default:
			;
		}
	}

	~WaitCancelGuard()
	{
		Attachment* att = m_tdbb->getAttachment();
		if (att)
			att->att_wait_lock = m_save_lock;

		if (m_cancel_disabled) 
			m_tdbb->tdbb_flags |= TDBB_wait_cancel_disable;
		else
			m_tdbb->tdbb_flags &= ~TDBB_wait_cancel_disable;
	}

private:
	thread_db* m_tdbb;
	Lock* m_save_lock;
	bool m_cancel_disabled;
};

} // namespace


void LCK_assert(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ a s s e r t
 *
 **************************************
 *
 * Functional description
 *	Assert a logical lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_logical == lock->lck_physical || lock->lck_logical == LCK_none)
	{
		return;
	}

	if (!LCK_lock(tdbb, lock, lock->lck_logical, LCK_WAIT))
		BUGCHECK(159);			// msg 159 cannot assert logical lock

	fb_assert(LCK_CHECK_LOCK(lock));
}


bool LCK_convert(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Convert an existing lock to a new level.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	Database* dbb = lock->lck_dbb;

	Attachment* const old_attachment = lock->lck_attachment;
	set_lock_attachment(lock, tdbb->getAttachment());

	WaitCancelGuard guard(tdbb, lock, wait);
	const bool result = CONVERT(tdbb, lock, level, wait);

	if (!result)
	{
	    set_lock_attachment(lock, old_attachment);

		switch (tdbb->tdbb_status_vector[1])
		{
		case isc_deadlock:
		case isc_lock_conflict:
		case isc_lock_timeout:
			tdbb->checkCancelState(true);
			return false;
		case isc_lockmanerr:
			dbb->dbb_flags |= DBB_bugcheck;
			break;
		}
		ERR_punt();
	}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	fb_assert(LCK_CHECK_LOCK(lock));
	return true;
}


bool LCK_convert_opt(thread_db* tdbb, Lock* lock, USHORT level)
{
/**************************************
 *
 *	L C K _ c o n v e r t _ o p t
 *
 **************************************
 *
 * Functional description
 *	Assert a lock if the parent is not locked in exclusive mode.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	const USHORT old_level = lock->lck_logical;
	lock->lck_logical = level;
	Database* dbb = lock->lck_dbb;

	if (dbb->dbb_ast_flags & DBB_assert_locks)
	{
		lock->lck_logical = old_level;
		return LCK_convert(tdbb, lock, level, LCK_NO_WAIT);
	}

	fb_assert(LCK_CHECK_LOCK(lock));
	return true;
}


bool LCK_cancel_wait(Attachment* attachment)
{
/**************************************
 *
 *	L C K _ c a n c e l _ w a i t
 *
 **************************************
 *
 * Functional description
 *	Try to cancel waiting of attachment inside the LM	
 *
 **************************************/
	Database *dbb = attachment->att_database;

	if (attachment->att_wait_lock)
		return dbb->dbb_lock_mgr->cancelWait(attachment->att_wait_lock->lck_owner_handle);

	return false;
}


void LCK_downgrade(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ d o w n g r a d e
 *
 **************************************
 *
 * Functional description
 *	Downgrade a lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_id && lock->lck_physical != LCK_none)
	{
		const USHORT level = DOWNGRADE(tdbb, lock);
		if (!lock->lck_compatible)
			lock->lck_physical = lock->lck_logical = level;
	}

	if (lock->lck_physical == LCK_none)
	{
		lock->lck_id = lock->lck_data = 0;
	    set_lock_attachment(lock, NULL);
	}

	fb_assert(LCK_CHECK_LOCK(lock));
}


void LCK_fini(thread_db* tdbb, enum lck_owner_t owner_type)
{
/**************************************
 *
 *	L C K _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Check out with lock manager.
 *
 **************************************/
	SLONG* owner_handle_ptr = NULL;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (owner_type)
	{
	case LCK_OWNER_database:
		owner_handle_ptr = LCK_OWNER_HANDLE_DBB(tdbb);
		break;

	case LCK_OWNER_attachment:
		owner_handle_ptr = LCK_OWNER_HANDLE_ATT(tdbb);
		break;

	default:
		bug_lck("Invalid lock owner type in LCK_fini ()");
		break;
	}

	dbb->dbb_lock_mgr->shutdownOwner(tdbb, owner_handle_ptr);
}


SLONG LCK_get_owner_handle(thread_db* tdbb, enum lck_t lock_type)
{
/**************************************
 *
 *	L C K _ g e t _ o w n e r _ h a n d l e
 *
 **************************************
 *
 * Functional description
 *	return the right kind of lock owner given a lock type.
 *
 **************************************/
	SET_TDBB(tdbb);

	SLONG handle = 0;

	switch (lock_type)
	{
	case LCK_database:
	case LCK_bdb:
	case LCK_rel_exist:
	case LCK_rel_partners:
	case LCK_idx_exist:
	case LCK_shadow:
	case LCK_retaining:
	case LCK_expression:
	case LCK_prc_exist:
	case LCK_backup_alloc:
	case LCK_backup_database:
	case LCK_monitor:
	case LCK_tt_exist:
	case LCK_shared_counter:
		handle = *LCK_OWNER_HANDLE_DBB(tdbb);
		break;
	case LCK_attachment:
	case LCK_page_space:
	case LCK_relation:
	case LCK_tra:
	case LCK_sweep:
	case LCK_update_shadow:
	case LCK_dsql_cache:
	case LCK_backup_end:
	case LCK_cancel:
	case LCK_btr_dont_gc:
		handle = *LCK_OWNER_HANDLE_ATT(tdbb);
		break;
	default:
		bug_lck("Invalid lock type in LCK_get_owner_handle()");
	}

	if (!handle)
	{
		bug_lck("Invalid lock owner handle");
	}

	return handle;
}


SLONG LCK_get_owner_handle_by_type(thread_db* tdbb, lck_owner_t lck_owner_type)
{
/**********************************************************
 *
 *	L C K _ g e t _ o w n e r _ h a n d l e _ b y _ t y p e
 *
 **********************************************************
 *
 * Functional description
 *	return the lock owner given a lock owner type.
 *
 *********************************************************/
	SET_TDBB(tdbb);

	switch (lck_owner_type)
	{
		case LCK_OWNER_database:
			return *LCK_OWNER_HANDLE_DBB(tdbb);
		case LCK_OWNER_attachment:
			return *LCK_OWNER_HANDLE_ATT(tdbb);
		default:
			bug_lck("Invalid lock owner type in LCK_get_owner_handle_by_type ()");
			return 0;
	}
}


void LCK_init(thread_db* tdbb, enum lck_owner_t owner_type)
{
/**************************************
 *
 *	L C K _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize the locking stuff for the given owner.
 *
 **************************************/
	LOCK_OWNER_T owner_id;
	SLONG* owner_handle_ptr = 0;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (owner_type)
	{
	case LCK_OWNER_database:
		owner_id = LCK_OWNER_ID_DBB(tdbb);
		owner_handle_ptr = LCK_OWNER_HANDLE_DBB(tdbb);
		break;

	case LCK_OWNER_attachment:
		owner_id = LCK_OWNER_ID_ATT(tdbb);
		owner_handle_ptr = LCK_OWNER_HANDLE_ATT(tdbb);
		break;

	default:
		bug_lck("Invalid lock owner type in LCK_init ()");
		break;
	}

	if (!dbb->dbb_lock_mgr->initializeOwner(tdbb, owner_id, owner_type, owner_handle_ptr))
	{
		if (tdbb->tdbb_status_vector[1] == isc_lockmanerr)
		{
			tdbb->getDatabase()->dbb_flags |= DBB_bugcheck;
		}

		ERR_punt();
	}
}


bool LCK_lock(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Lock a block.  There had better not have been a lock there.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	Database* dbb = lock->lck_dbb;
    set_lock_attachment(lock, tdbb->getAttachment());

	WaitCancelGuard guard(tdbb, lock, wait);
	ENQUEUE(tdbb, lock, level, wait);
	fb_assert(LCK_CHECK_LOCK(lock));
	if (!lock->lck_id)
	{
    	set_lock_attachment(lock, NULL);
		if (!wait)
			return false;

		switch (tdbb->tdbb_status_vector[1])
		{
		case isc_deadlock:
		case isc_lock_conflict:
		case isc_lock_timeout:
			tdbb->checkCancelState(true);
			return false;
		case isc_lockmanerr:
			dbb->dbb_flags |= DBB_bugcheck;
			break;
		}
		ERR_punt();
	}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	fb_assert(LCK_CHECK_LOCK(lock));
	return true;
}


bool LCK_lock_opt(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ l o c k _ o p t
 *
 **************************************
 *
 * Functional description
 *	Assert a lock if the parent is not locked in exclusive mode.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	lock->lck_logical = level;
	Database* dbb = lock->lck_dbb;

	if (dbb->dbb_ast_flags & DBB_assert_locks)
	{
		lock->lck_logical = LCK_none;
		return LCK_lock(tdbb, lock, level, wait);
	}

	fb_assert(LCK_CHECK_LOCK(lock));
	return true;
}


SLONG LCK_query_data(thread_db* tdbb, Lock* parent, enum lck_t lock_type, USHORT aggregate)
{
/**************************************
 *
 *	L C K _ q u e r y _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Perform aggregate operations on data associated
 *	with a lock series for a lock hierarchy rooted
 *	at a parent lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(parent));

	return dbb->dbb_lock_mgr->queryData(parent->lck_id, lock_type, aggregate);
}


SLONG LCK_read_data(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ r e a d _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Read the data associated with a lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));

#ifdef VMS
	if (!LCK_lock(NULL, lock, LCK_null, LCK_NO_WAIT))
		return 0;

	const SLONG data = dbb->dbb_lock_mgr->readData(lock->lck_id);
	LCK_release(lock);
#else
	Lock* parent = lock->lck_parent;
	const SLONG data =
		dbb->dbb_lock_mgr->readData2(parent ? parent->lck_id : 0,
									 lock->lck_type,
									 (UCHAR*) &lock->lck_key, lock->lck_length,
									 lock->lck_owner_handle);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	return data;
}


void LCK_release(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release an existing lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_physical != LCK_none) {
		DEQUEUE(tdbb, lock);
	}

	lock->lck_physical = lock->lck_logical = LCK_none;
	lock->lck_id = lock->lck_data = 0;
	set_lock_attachment(lock, NULL);

	fb_assert(LCK_CHECK_LOCK(lock));
}


void LCK_re_post(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ r e _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Re-post an ast when the original
 *	deliver resulted in blockage.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_compatible)
	{
		if (lock->lck_ast) {
			(*lock->lck_ast)(lock->lck_object);
		}
		return;
	}

	dbb->dbb_lock_mgr->repost(tdbb, lock->lck_ast, lock->lck_object, lock->lck_owner_handle);

	fb_assert(LCK_CHECK_LOCK(lock));
}


void LCK_write_data(thread_db* tdbb, Lock* lock, SLONG data)
{
/**************************************
 *
 *	L C K _ w r i t e _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Write a longword into an existing lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));

	dbb->dbb_lock_mgr->writeData(lock->lck_id, data);
	lock->lck_data = data;

	fb_assert(LCK_CHECK_LOCK(lock));
}


static void bug_lck(const TEXT* string)
{
/**************************************
 *
 *	b u g _ l c k
 *
 **************************************
 *
 * Functional description
 *	Log the bug message, initialize the status vector
 *	and get out.
 *
 **************************************/
	TEXT s[128];

	sprintf(s, "Fatal lock interface error: %.96s", string);
	gds__log(s);
	ERR_post(Arg::Gds(isc_db_corrupt) << Arg::Str(string));
}


static bool compatible(const Lock* lock1, const Lock* lock2, USHORT level2)
{
/**************************************
 *
 *	c o m p a t i b l e
 *
 **************************************
 *
 * Functional description
 *	Given two locks, and a desired level for the
 *	second lock, determine whether the two locks
 *	would be compatible.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock1));
	fb_assert(LCK_CHECK_LOCK(lock2));

	// if the locks have the same compatibility block,
	// they are always compatible regardless of level

	if (lock1->lck_compatible && lock2->lck_compatible && lock1->lck_compatible == lock2->lck_compatible)
	{
		// check for a second level of compatibility as well:
		// if a second level was specified, the locks must also be compatible at the second level

		if (!lock1->lck_compatible2 || !lock2->lck_compatible2 ||
			lock1->lck_compatible2 == lock2->lck_compatible2)
		{
			return true;
		}
	}

	return compatibility[lock1->lck_logical][level2];
}


static void enqueue(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	e n q u e u e
 *
 **************************************
 *
 * Functional description
 *	Submit a lock to the lock manager.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));

	Lock* parent = lock->lck_parent;
	lock->lck_id = dbb->dbb_lock_mgr->enqueue(tdbb,
											  lock->lck_id,
											  parent ? parent->lck_id : 0,
											  lock->lck_type,
											  (const UCHAR*) &lock->lck_key,
											  lock->lck_length,
											  level,
											  lock->lck_ast,
											  lock->lck_object,
											  lock->lck_data,
											  wait,
											  lock->lck_owner_handle);

	if (!lock->lck_id)
	{
		lock->lck_physical = lock->lck_logical = LCK_none;
	}

	fb_assert(LCK_CHECK_LOCK(lock));
}


static int external_ast(void* lock_void)
{
/**************************************
 *
 *	e x t e r n a l _ a s t
 *
 **************************************
 *
 * Functional description
 *	Deliver blocking asts to all locks identical to
 *	the passed lock.  This routine is called when
 *	we are blocking a lock from another process.
 *
 **************************************/
	Lock* lock = static_cast<Lock*>(lock_void);
	fb_assert(LCK_CHECK_LOCK(lock));

	// go through the list, saving the next lock in the list
	// in case the current one gets deleted in the ast

	Lock* next;
	for (Lock* match = hash_get_lock(lock, 0, 0); match; match = next)
	{
		next = match->lck_identical;
		if (match->lck_ast) {
			(*match->lck_ast)(match->lck_object);
		}
	}
	return 0; // make the compiler happy
}



static USHORT hash_func(const UCHAR* value, USHORT length)
{
/**************************************
 *
 *	h a s h
 *
 **************************************
 *
 * Functional description
 *	Provide a repeatable hash value based
 *	on the passed key.
 *
 **************************************/

	// Hash the value, preserving its distribution as much as possible

	ULONG hash_value = 0;
	UCHAR* p = 0;
	const UCHAR* q = value;

	for (USHORT l = 0; l < length; l++)
	{
		if (!(l & 3))
			p = (UCHAR*) &hash_value;
		*p++ = *q++;
	}

	return (USHORT) (hash_value % LOCK_HASH_SIZE);
}


static void hash_allocate(Lock* lock)
{
/**************************************
 *
 *	h a s h _ a l l o c a t e
 *
 **************************************
 *
 * Functional description
 *	Allocate the hash table for handling
 *	compatible locks.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));

	Database* dbb = lock->lck_dbb;

	Attachment* attachment = lock->lck_attachment;
	if (attachment)
	{
		attachment->att_compatibility_table =
			vec<Lock*>::newVector(*attachment->att_pool, LOCK_HASH_SIZE);
	}
}


static Lock* hash_get_lock(Lock* lock, USHORT* hash_slot, Lock*** prior)
{
/**************************************
 *
 *	h a s h _ g e t _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Return the first matching identical
 *	lock to the passed lock.  To minimize
 *	code for searching through the hash
 *	table, return hash_slot or prior lock
 *	if requested.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));

	Attachment* const att = lock->lck_attachment;
	if (!att)
		return NULL;

	if (!att->att_compatibility_table)
		hash_allocate(lock);

	const USHORT hash_value = hash_func((UCHAR*) & lock->lck_key, lock->lck_length);

	if (hash_slot)
		*hash_slot = hash_value;

	// if no collisions found, we're done

	Lock* match = (*att->att_compatibility_table)[hash_value];
	if (!match)
		return NULL;

	if (prior)
		*prior = & (*att->att_compatibility_table)[hash_value];

	// look for an identical lock

	fb_assert(LCK_CHECK_LOCK(match));
	for (Lock* collision = match; collision; collision = collision->lck_collision)
	{
		fb_assert(LCK_CHECK_LOCK(collision));
		if (collision->lck_parent && lock->lck_parent &&
			collision->lck_parent->lck_id == lock->lck_parent->lck_id &&
			collision->lck_type == lock->lck_type &&
			collision->lck_length == lock->lck_length)
		{
			// check that the keys are the same

			if (!memcmp(lock->lck_key.lck_string, collision->lck_key.lck_string, lock->lck_length))
				return collision;
		}

		if (prior)
			*prior = &collision->lck_collision;
	}

	return NULL;
}


static void hash_insert_lock(Lock* lock)
{
/**************************************
 *
 *	h a s h _ i n s e r t _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Insert the provided lock into the
 *	compatibility lock table.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));

	Attachment* const att = lock->lck_attachment;
	if (!att)
		return;

	// if no identical is returned, place it in the collision list

	USHORT hash_slot;
	Lock* identical = hash_get_lock(lock, &hash_slot, 0);
	if (!identical)
	{
		lock->lck_collision = (*att->att_compatibility_table)[hash_slot];
		(*att->att_compatibility_table)[hash_slot] = lock;
		return;
	}

	// place it second in the list, out of pure laziness

	lock->lck_identical = identical->lck_identical;
	identical->lck_identical = lock;
}


static bool hash_remove_lock(Lock* lock, Lock** match)
{
/**************************************
 *
 *	h a s h _ r e m o v e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Remove the passed lock from the hash table.
 *	Return true if this is the last such identical
 *	lock removed.   Also return the first matching
 *	locking found.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));

	Lock** prior;
	Lock* next = hash_get_lock(lock, 0, &prior);
	if (!next)
	{
		// set lck_compatible to NULL to make sure we don't
		// try to release the lock again in bugchecking

		lock->lck_compatible = NULL;
		BUGCHECK(285);			// lock not found in internal lock manager
	}

	if (match)
		*match = next;

	// special case if our lock is the first one in the identical list

	if (next == lock)
	{
		if (lock->lck_identical)
		{
			lock->lck_identical->lck_collision = lock->lck_collision;
			*prior = lock->lck_identical;
			return false;
		}

		*prior = lock->lck_collision;
		return true;
	}

	Lock* last = 0;
	for (; next; last = next, next = next->lck_identical)
	{
		if (next == lock)
			break;
	}

	if (!next)
	{
		lock->lck_compatible = NULL;
		BUGCHECK(285);			// lock not found in internal lock manager
	}

	last->lck_identical = next->lck_identical;
	return false;
}


static void internal_ast(Lock* lock)
{
/**************************************
 *
 *	i n t e r n a l _ a s t
 *
 **************************************
 *
 * Functional description
 *	Deliver blocking asts to all locks identical to
 *	the passed lock.  This routine is called to downgrade
 *	all other locks in the same process which do not have
 *	the lck_compatible field set.
 *	Note that if this field were set, the internal lock manager
 *	should not be able to generate a lock request which blocks
 *	on our own process.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));

	// go through the list, saving the next lock in the list
	// in case the current one gets deleted in the ast

	Lock* next;
	for (Lock* match = hash_get_lock(lock, 0, 0); match; match = next)
	{
		next = match->lck_identical;

		// don't deliver the ast to any locks which are already compatible

		if (match != lock && !compatible(match, lock, lock->lck_logical) && match->lck_ast)
		{
			(*match->lck_ast)(match->lck_object);
		}
	}
}



static bool internal_compatible(Lock* match, const Lock* lock, USHORT level)
{
/**************************************
 *
 *	i n t e r n a l _ c o m p a t i b l e
 *
 **************************************
 *
 * Functional description
 *	See if there are any incompatible locks
 *	in the list of locks held by this process.
 *	If there are none, return true to indicate
 *	that the lock is compatible.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(match));
	fb_assert(LCK_CHECK_LOCK(lock));

	// first check if there are any locks which are incompatible which do not have blocking asts;
	// if so, there is no chance of getting a compatible lock

	for (const Lock* next = match; next; next = next->lck_identical)
	{
		if (!next->lck_ast && !compatible(next, lock, level))
			return false;
	}

	// now deliver the blocking asts, attempting to gain
	// compatibility by getting everybody to downgrade
	internal_ast(match);

	// make one more pass to see if all locks were downgraded

	for (const Lock* next = match; next; next = next->lck_identical)
	{
		if (!compatible(next, match, level))
			return false;
	}

	return true;
}


static void internal_dequeue(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	i n t e r n a l _ d e q u e u e
 *
 **************************************
 *
 * Functional description
 *	Dequeue a lock.  If there are identical
 *	compatible locks, check to see whether
 *	the lock needs to be downgraded.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));
	fb_assert(lock->lck_compatible);

	// if this is the last identical lock in the hash table, release it

	Lock* match;
	if (hash_remove_lock(lock, &match))
	{
		if (!dbb->dbb_lock_mgr->dequeue(lock->lck_id))
		{
			bug_lck("LOCK_deq() failed in Lock:internal_dequeue");
		}

		lock->lck_id = 0;
		lock->lck_physical = lock->lck_logical = LCK_none;
		return;
	}

	// check for a potential downgrade

	internal_downgrade(tdbb, match);
}


static USHORT internal_downgrade(thread_db* tdbb, Lock* first)
{
/**************************************
 *
 *	i n t e r n a l _ d o w n g r a d e
 *
 **************************************
 *
 * Functional description
 *	Set the physical lock value of all locks identical
 *	to the passed lock.  It should be the same as the
 *	highest logical level.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(first));
	fb_assert(first->lck_compatible);

	// find the highest required lock level

	USHORT level = LCK_none;
	for (const Lock* lock = first; lock; lock = lock->lck_identical)
		level = MAX(level, lock->lck_logical);

	// if we can convert to that level, set all identical locks as having that level

	if (level < first->lck_physical)
	{
		if (dbb->dbb_lock_mgr->convert(tdbb, first->lck_id, level, LCK_NO_WAIT, external_ast, first))
		{
			for (Lock* lock = first; lock; lock = lock->lck_identical)
			{
				lock->lck_physical = level;
			}

			return level;
		}
	}

	return first->lck_physical;
}


static bool internal_enqueue(thread_db* tdbb,
							Lock* lock,
							USHORT level,
							SSHORT wait, bool convert_flg)
{
/**************************************
 *
 *	i n t e r n a l _ e n q u e u e
 *
 **************************************
 *
 * Functional description
 *	See if there is a compatible lock already held
 *	by this process; if not, go ahead and submit the
 *	lock to the real lock manager.
 *	NOTE: This routine handles both enqueueing
 *	and converting existing locks, since the convert
 *	will find itself in the hash table and convert
 *	itself upward.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	fb_assert(LCK_CHECK_LOCK(lock));
	fb_assert(lock->lck_compatible);

	ISC_STATUS* const status = tdbb->tdbb_status_vector;

	// look for an identical lock

	Lock* match = hash_get_lock(lock, 0, 0);
	if (match)
	{
		// if there are incompatible locks for which there are no blocking asts defined, give up

		if (!internal_compatible(match, lock, level))
		{
			// for now return a lock conflict; it would be better if we were to
			// do a wait on the other lock by setting some flag bit or some such

			Arg::Gds(isc_lock_conflict).copyTo(status);
			return false;
		}

		// if there is still an identical lock, convert the lock, otherwise fall
		// through and enqueue a new one

		if ( (match = hash_get_lock(lock, 0, 0)) )
		{
			// if a conversion is necessary, update all identical
			// locks to reflect the new physical lock level

			if (level > match->lck_physical)
			{
				if (!dbb->dbb_lock_mgr->convert(tdbb,
												match->lck_id,
												level,
												wait,
												external_ast,
												lock))
				{
					return false;
				}

				for (Lock* update = match; update; update = update->lck_identical)
				{
					update->lck_physical = level;
				}
			}

			lock->lck_id = match->lck_id;
			lock->lck_logical = level;
			lock->lck_physical = match->lck_physical;

			// When converting a lock (from the callers point of view),
			// then no new lock needs to be inserted.

			if (!convert_flg)
				hash_insert_lock(lock);

			return true;
		}
	}

	// enqueue the lock, but swap out the ast and the ast argument
	// with the local ast handler, passing it the lock block itself

	lock->lck_id = dbb->dbb_lock_mgr->enqueue(tdbb,
											  lock->lck_id,
											  lock->lck_parent ? lock->lck_parent->lck_id : 0,
											  lock->lck_type,
											  (const UCHAR*) &lock->lck_key,
											  lock->lck_length,
											  level,
											  external_ast,
											  lock,
											  lock->lck_data,
											  wait,
											  lock->lck_owner_handle);

	// If the lock exchange failed, set the lock levels appropriately
	if (lock->lck_id == 0)
	{
		lock->lck_physical = lock->lck_logical = LCK_none;
	}

	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_id)
	{
		hash_insert_lock(lock);
		lock->lck_logical = lock->lck_physical = level;
	}

	fb_assert(LCK_CHECK_LOCK(lock));

	return lock->lck_id ? true : false;
}


static void set_lock_attachment(Lock* lock, Attachment* attachment)
{
	if (lock->lck_attachment == attachment)
		return;

	// If lock has an attachment it must not be a part of linked list
	fb_assert(!lock->lck_attachment ? !lock->lck_prior && !lock->lck_next : true);

	// Delist in old attachment
	if (lock->lck_attachment)
	{
		// Check that attachment seems to be valid, check works only when DEBUG_GDS_ALLOC is defined
		fb_assert(lock->lck_attachment->att_flags != 0xDEADBEEF);

		Lock* const next = lock->lck_next;
		Lock* const prior = lock->lck_prior;

		if (prior)
		{
			fb_assert(prior->lck_next == lock);
			prior->lck_next = next;
		}
		else
		{
			fb_assert(lock->lck_attachment->att_long_locks == lock);
			lock->lck_attachment->att_long_locks = next;
		}

		if (next)
		{
			fb_assert(next->lck_prior == lock);
			next->lck_prior = prior;
		}

		lock->lck_next = NULL;
		lock->lck_prior = NULL;
	}


	// Enlist in new attachment
	if (attachment)
	{
		// Check that attachment seems to be valid, check works only when DEBUG_GDS_ALLOC is defined
		fb_assert(attachment->att_flags != 0xDEADBEEF);

		lock->lck_next = attachment->att_long_locks;
		lock->lck_prior = NULL;
		attachment->att_long_locks = lock;

		Lock* next = lock->lck_next;
		if (next)
			next->lck_prior = lock;
	}

	lock->lck_attachment = attachment;
}
