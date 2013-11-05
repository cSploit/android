/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		shut.cpp
 *	DESCRIPTION:	Database shutdown handler
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

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/scl.h"
#include "../jrd/ibase.h"
#include "../jrd/nbak.h"
#include "../jrd/ods.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/shut_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/extds/ExtDS.h"

using namespace Jrd;
using namespace Firebird;

const SSHORT SHUT_WAIT_TIME	= 5;

// Shutdown lock data
union shutdown_data
{
	struct {
		SSHORT flag;
		SSHORT delay;
	} data_items;
	SLONG data_long;
};


// Define this to true if you need to allow no-op behavior when requested shutdown mode
// matches current. Logic of jrd8_create_database may need attention in this case too
const bool IGNORE_SAME_MODE = false;

static void bad_mode();
static void same_mode();
static void check_backup_state(thread_db*);
static bool notify_shutdown(thread_db*, SSHORT, SSHORT);
static bool shutdown_locks(thread_db*, SSHORT);


bool SHUT_blocking_ast(thread_db* tdbb)
{
/**************************************
 *
 *	S H U T _ b l o c k i n g _ a s t
 *
 **************************************
 *
 * Functional description
 *	Read data from database lock for
 *	shutdown instructions.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	shutdown_data data;
	data.data_long = LCK_read_data(tdbb, dbb->dbb_lock);
	const SSHORT flag = data.data_items.flag;
	const SSHORT delay = data.data_items.delay;

	const int shut_mode = flag & isc_dpb_shut_mode_mask;

/* Database shutdown has been cancelled. */

	// Delay of -1 means we're going online
	if (delay == -1)
	{
		dbb->dbb_ast_flags &= ~(DBB_shut_attach | DBB_shut_tran | DBB_shut_force);

		if (shut_mode)
		{
			dbb->dbb_ast_flags &= ~(DBB_shutdown | DBB_shutdown_single | DBB_shutdown_full);

			switch (shut_mode)
			{
			case isc_dpb_shut_normal:
				break;
			case isc_dpb_shut_multi:
				dbb->dbb_ast_flags |= DBB_shutdown;
				break;
			case isc_dpb_shut_single:
				dbb->dbb_ast_flags |= DBB_shutdown | DBB_shutdown_single;
				break;
			case isc_dpb_shut_full:
				dbb->dbb_ast_flags |= DBB_shutdown | DBB_shutdown_full;
				break;
			default:
				fb_assert(false);
			}
		}

		return false;
	}

	if ((flag & isc_dpb_shut_force) && !delay)
		return shutdown_locks(tdbb, flag);

	if (flag & isc_dpb_shut_attachment)
		dbb->dbb_ast_flags |= DBB_shut_attach;
	if (flag & isc_dpb_shut_force)
		dbb->dbb_ast_flags |= DBB_shut_force;
	if (flag & isc_dpb_shut_transaction)
		dbb->dbb_ast_flags |= DBB_shut_tran;

	return false;
}


void SHUT_database(thread_db* tdbb, SSHORT flag, SSHORT delay)
{
/**************************************
 *
 *	S H U T _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Schedule database for shutdown
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

/* Only platform's user locksmith can shutdown or bring online
   a database. */

	if (!attachment->locksmith())
	{
		ERR_post(Arg::Gds(isc_no_priv) << "shutdown" << "database" << dbb->dbb_filename);
	}

	const int shut_mode = flag & isc_dpb_shut_mode_mask;

	// Check if requested shutdown mode is valid
	// Note that if we are already in requested mode we just return true.
	// This is required to ensure backward compatible behavior (gbak relies on that,
	// user-written scripts may rely on this behaviour too)
	switch (shut_mode)
	{
	case isc_dpb_shut_full:
		if (dbb->dbb_ast_flags & DBB_shutdown_full)
		{
			same_mode();
			return;
		}
		break;
	case isc_dpb_shut_multi:
		if ((dbb->dbb_ast_flags & DBB_shutdown_full) || (dbb->dbb_ast_flags & DBB_shutdown_single))
		{
			bad_mode();
		}
		if (dbb->dbb_ast_flags & DBB_shutdown)
		{
			same_mode();
			return;
		}
		break;
	case isc_dpb_shut_single:
		if (dbb->dbb_ast_flags & DBB_shutdown_full)
		{
			bad_mode();
		}
		if (dbb->dbb_ast_flags & DBB_shutdown_single)
		{
			same_mode();
			return;
		}
		break;
	case isc_dpb_shut_normal:
		if (!(dbb->dbb_ast_flags & DBB_shutdown))
		{
			same_mode();
			return;
		}
		bad_mode();
	default:
		bad_mode(); // unexpected mode
	}

	// Reject exclusive and single-user shutdown attempts
	// for a physically locked database

	if (shut_mode == isc_dpb_shut_full || shut_mode == isc_dpb_shut_single)
	{
		check_backup_state(tdbb);
	}

	attachment->att_flags |= ATT_shutdown_manager;
	--dbb->dbb_use_count;

/* Database is being shutdown. First notification gives shutdown
   type and delay in seconds. */

	bool exclusive = notify_shutdown(tdbb, flag, delay);

/* Notify local attachments */

	SHUT_blocking_ast(tdbb);

/* Try to get exclusive database lock periodically up to specified delay. If we
   haven't gotten it report shutdown error for weaker forms. For forced shutdown
   keep notifying until successful. */

	SSHORT timeout = delay - SHUT_WAIT_TIME;

	if (!exclusive)
	{
		for (; timeout >= 0; timeout -= SHUT_WAIT_TIME)
		{
			if ((exclusive = notify_shutdown(tdbb, flag, timeout)) ||
				!(dbb->dbb_ast_flags & (DBB_shut_attach | DBB_shut_tran | DBB_shut_force)))
			{
				break;
			}
		}
	}

	if (!exclusive && (timeout > 0 || flag & (isc_dpb_shut_attachment | isc_dpb_shut_transaction)))
	{
		notify_shutdown(tdbb, 0, -1);	/* Tell everyone we're giving up */
		SHUT_blocking_ast(tdbb);
		attachment->att_flags &= ~ATT_shutdown_manager;
		++dbb->dbb_use_count;
		ERR_post(Arg::Gds(isc_shutfail));
	}

/* Once there are no more transactions active, force all remaining
   attachments to shutdown. */

	if (flag & isc_dpb_shut_transaction)
	{
		exclusive = false;
		flag = isc_dpb_shut_force | shut_mode;
	}

	dbb->dbb_ast_flags |= DBB_shutdown;
	dbb->dbb_ast_flags &= ~(DBB_shutdown_single | DBB_shutdown_full);
	switch (shut_mode)
	{
	case isc_dpb_shut_normal:
	case isc_dpb_shut_multi:
		break;
	case isc_dpb_shut_single:
		dbb->dbb_ast_flags |= DBB_shutdown_single;
		break;
	case isc_dpb_shut_full:
		dbb->dbb_ast_flags |= DBB_shutdown_full;
		break;
	default:
		fb_assert(false);
	}

	if (!exclusive && (flag & isc_dpb_shut_force))
	{
		// TMN: Ugly counting!
		while (!notify_shutdown(tdbb, flag, 0))
			;
	}

	++dbb->dbb_use_count;
	dbb->dbb_ast_flags &= ~(DBB_shut_force | DBB_shut_attach | DBB_shut_tran);
	WIN window(HEADER_PAGE_NUMBER);
	Ods::header_page* header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	// Set appropriate shutdown mode in database header
	header->hdr_flags &= ~Ods::hdr_shutdown_mask;
	switch (shut_mode)
	{
	case isc_dpb_shut_normal:
		break;
	case isc_dpb_shut_multi:
		header->hdr_flags |= Ods::hdr_shutdown_multi;
		break;
	case isc_dpb_shut_single:
		header->hdr_flags |= Ods::hdr_shutdown_single;
		break;
	case isc_dpb_shut_full:
		header->hdr_flags |= Ods::hdr_shutdown_full;
		break;
	default:
		fb_assert(false);
	}
	CCH_RELEASE(tdbb, &window);
	CCH_release_exclusive(tdbb);
}


void SHUT_init(thread_db* tdbb)
{
/**************************************
 *
 *	S H U T _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Read data from database lock for
 *	shutdown instructions.
 *
 **************************************/

	SHUT_blocking_ast(tdbb);
}


void SHUT_online(thread_db* tdbb, SSHORT flag)
{
/**************************************
 *
 *	S H U T _ o n l i n e
 *
 **************************************
 *
 * Functional description
 *	Move database to "more online" state
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

/* Only platform's user locksmith can shutdown or bring online
   a database. */

	if (!attachment->att_user->locksmith())
	{
		ERR_post(Arg::Gds(isc_no_priv) << "bring online" << "database" << dbb->dbb_filename);
	}

	const int shut_mode = flag & isc_dpb_shut_mode_mask;

	// Check if requested shutdown mode is valid
	switch (shut_mode)
	{
	case isc_dpb_shut_normal:
		if (!(dbb->dbb_ast_flags & DBB_shutdown))
		{
			same_mode(); // normal -> normal
			return;
		}
		break;
	case isc_dpb_shut_multi:
		if (!(dbb->dbb_ast_flags & DBB_shutdown))
		{
			bad_mode(); // normal -> multi
		}
		if (!(dbb->dbb_ast_flags & DBB_shutdown_full) && !(dbb->dbb_ast_flags & DBB_shutdown_single))
		{
			same_mode(); // multi -> multi
			return;
		}
		break;
	case isc_dpb_shut_single:
		if (dbb->dbb_ast_flags & DBB_shutdown_single)
		{
			same_mode(); //single -> single
			return;
		}
		if (!(dbb->dbb_ast_flags & DBB_shutdown_full))
		{
			bad_mode(); // !full -> single
		}
		break;
	case isc_dpb_shut_full:
		if (dbb->dbb_ast_flags & DBB_shutdown_full)
		{
			same_mode(); // full -> full
			return;
		}
		bad_mode();
	default: // isc_dpb_shut_full
		bad_mode(); // unexpected mode
	}

	// Reject exclusive and single-user shutdown attempts
	// for a physically locked database

	if (shut_mode == isc_dpb_shut_full || shut_mode == isc_dpb_shut_single)
	{
		check_backup_state(tdbb);
	}

	/* Clear shutdown flag on database header page */

	WIN window(HEADER_PAGE_NUMBER);
	Ods::header_page* header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	// Set appropriate shutdown mode in database header
	header->hdr_flags &= ~Ods::hdr_shutdown_mask;
	switch (shut_mode)
	{
	case isc_dpb_shut_normal:
		break;
	case isc_dpb_shut_multi:
		header->hdr_flags |= Ods::hdr_shutdown_multi;
		break;
	case isc_dpb_shut_single:
		header->hdr_flags |= Ods::hdr_shutdown_single;
		break;
	case isc_dpb_shut_full:
		header->hdr_flags |= Ods::hdr_shutdown_full;
		break;
	default:
		fb_assert(false);
	}
	CCH_RELEASE(tdbb, &window);

	/* Notify existing database clients that a currently
	   scheduled shutdown is cancelled. */

	if (notify_shutdown(tdbb, shut_mode, -1))
		CCH_release_exclusive(tdbb);

	/* Notify local attachments */

	SHUT_blocking_ast(tdbb);
}


static void same_mode()
{
	if (!IGNORE_SAME_MODE)
	{
		bad_mode();
	}
}


static void bad_mode()
{
	Database* dbb = JRD_get_thread_data()->getDatabase();
	ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(dbb->dbb_database_name));
}


static void check_backup_state(thread_db* tdbb)
{
	Database* dbb = tdbb->getDatabase();

	BackupManager::StateReadGuard stateGuard(tdbb);

	if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
	{
		ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(dbb->dbb_filename));
	}
}


static bool notify_shutdown(thread_db* tdbb, SSHORT flag, SSHORT delay)
{
/**************************************
 *
 *	n o t i f y _ s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *	Notify database users that shutdown
 *	status of a database is changing.
 *	Pulse database lock and pass shutdown
 *	flags and delay via lock data.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

	shutdown_data data;

	data.data_items.flag = flag;
	data.data_items.delay = delay;

	LCK_write_data(tdbb, dbb->dbb_lock, data.data_long);

/* Send blocking ASTs to database users */

	const bool exclusive = CCH_exclusive(tdbb, LCK_PW, delay > 0 ? -SHUT_WAIT_TIME : -1);

	if (exclusive && (delay != -1)) {
		return shutdown_locks(tdbb, flag);
	}
	if ((flag & isc_dpb_shut_force) && !delay) {
		return shutdown_locks(tdbb, flag);
	}
	if ((flag & isc_dpb_shut_transaction) && !TRA_active_transactions(tdbb, dbb))
	{
		return true;
	}

	return exclusive;
}


static bool shutdown_locks(thread_db* tdbb, SSHORT flag)
{
/**************************************
 *
 *	s h u t d o w n _ l o c k s
 *
 **************************************
 *
 * Functional description
 *	Release all attachment and database
 *	locks if database is quiet.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

/* Mark database and all active attachments as shutdown. */

	dbb->dbb_ast_flags &= ~(DBB_shutdown | DBB_shutdown_single | DBB_shutdown_full);

	switch (flag & isc_dpb_shut_mode_mask)
	{
	case isc_dpb_shut_normal:
		break;
	case isc_dpb_shut_multi:
		dbb->dbb_ast_flags |= DBB_shutdown;
		break;
	case isc_dpb_shut_single:
		dbb->dbb_ast_flags |= DBB_shutdown | DBB_shutdown_single;
		break;
	case isc_dpb_shut_full:
		dbb->dbb_ast_flags |= DBB_shutdown | DBB_shutdown_full;
		break;
	default:
		fb_assert(false);
	}

	Attachment* attachment;

	for (attachment = dbb->dbb_attachments; attachment; attachment = attachment->att_next)
	{
		if (!(attachment->att_flags & ATT_shutdown_manager))
		{
			attachment->att_flags |= ATT_shutdown;
			attachment->cancelExternalConnection(tdbb);
			LCK_cancel_wait(attachment);
		}
	}

	JRD_shutdown_attachments(dbb);

	for (int retry = 0; retry < 10 && dbb->dbb_use_count; retry++)
	{
		// Let active database threads rundown
		Database::Checkout dcoHolder(dbb);
		THREAD_SLEEP(1 * 100);
	}

	if (dbb->dbb_use_count)
	{
		return false;
	}

/* Since no attachment is actively running, release all
   attachment-specfic locks while they're not looking. */

	const Attachment* shut_attachment = NULL;

	for (attachment = dbb->dbb_attachments; attachment; attachment = attachment->att_next)
	{
		if (attachment->att_flags & ATT_shutdown_manager)
		{
			shut_attachment = attachment;
			continue;
		}

		if (attachment->att_id_lock)
			LCK_release(tdbb, attachment->att_id_lock);

		TRA_shutdown_attachment(tdbb, attachment);
	}

/* Release database locks that are shared by all attachments.
   These include relation and index existence locks, as well
   as, relation interest and record locking locks for PC semantic
   record locking. */

	CMP_shutdown_database(tdbb);

/* If shutdown manager is here, leave enough database lock context
   to run as a normal attachment. Otherwise, get rid of the rest
   of the database locks.*/

	if (!shut_attachment)
	{
		CCH_shutdown_database(dbb);
		if (dbb->dbb_monitor_lock)
			LCK_release(tdbb, dbb->dbb_monitor_lock);
		if (dbb->dbb_shadow_lock)
			LCK_release(tdbb, dbb->dbb_shadow_lock);
		if (dbb->dbb_retaining_lock)
			LCK_release(tdbb, dbb->dbb_retaining_lock);
		if (dbb->dbb_lock)
			LCK_release(tdbb, dbb->dbb_lock);
		dbb->dbb_shared_counter.shutdown(tdbb);
		dbb->dbb_backup_manager->shutdown(tdbb);
		dbb->dbb_ast_flags |= DBB_shutdown_locks;
	}

	return true;
}
