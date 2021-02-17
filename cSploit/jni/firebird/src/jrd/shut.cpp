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

static void bad_mode(Database* dbb)
{
	ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(dbb->dbb_database_name));
}

static void same_mode(Database* dbb)
{
	if (!IGNORE_SAME_MODE)
		bad_mode(dbb);
}

static void check_backup_state(thread_db*);
static bool notify_shutdown(thread_db*, SSHORT, SSHORT, Sync*);
static void shutdown(thread_db*, SSHORT, bool);


void SHUT_blocking_ast(thread_db* tdbb, bool ast)
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
	Database* const dbb = tdbb->getDatabase();

	shutdown_data data;
	data.data_long = LCK_read_data(tdbb, dbb->dbb_lock);
	const SSHORT flag = data.data_items.flag;
	const SSHORT delay = data.data_items.delay;

	const int shut_mode = flag & isc_dpb_shut_mode_mask;

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

		return;
	}

	if ((flag & isc_dpb_shut_force) && !delay)
	{
		shutdown(tdbb, flag, ast);
		return;
	}

	if (flag & isc_dpb_shut_attachment)
		dbb->dbb_ast_flags |= DBB_shut_attach;
	if (flag & isc_dpb_shut_force)
		dbb->dbb_ast_flags |= DBB_shut_force;
	if (flag & isc_dpb_shut_transaction)
		dbb->dbb_ast_flags |= DBB_shut_tran;
}


void SHUT_database(thread_db* tdbb, SSHORT flag, SSHORT delay, Sync* guard)
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
	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Only platform's user locksmith can shutdown or bring online a database

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
			same_mode(dbb);
			return;
		}
		break;
	case isc_dpb_shut_multi:
		if (dbb->dbb_ast_flags & (DBB_shutdown_full | DBB_shutdown_single))
		{
			bad_mode(dbb);
		}
		if (dbb->dbb_ast_flags & DBB_shutdown)
		{
			same_mode(dbb);
			return;
		}
		break;
	case isc_dpb_shut_single:
		if (dbb->dbb_ast_flags & DBB_shutdown_full)
		{
			bad_mode(dbb);
		}
		if (dbb->dbb_ast_flags & DBB_shutdown_single)
		{
			same_mode(dbb);
			return;
		}
		break;
	case isc_dpb_shut_normal:
		if (!(dbb->dbb_ast_flags & DBB_shutdown))
		{
			same_mode(dbb);
			return;
		}
		bad_mode(dbb);
	default:
		bad_mode(dbb); // unexpected mode
	}

	// Reject exclusive and single-user shutdown attempts
	// for a physically locked database

	if (shut_mode == isc_dpb_shut_full || shut_mode == isc_dpb_shut_single)
	{
		check_backup_state(tdbb);
	}

	attachment->att_flags |= ATT_shutdown_manager;

	// Database is being shutdown. First notification gives shutdown type and delay in seconds.

	bool exclusive = notify_shutdown(tdbb, flag, delay, guard);
	bool successful = exclusive;

	// Try to get exclusive database lock periodically up to specified delay. If we
	// haven't gotten it report shutdown error for weaker forms. For forced shutdown
	// keep notifying until successful.

	SSHORT timeout = delay ? delay - 1 : 0;

	if (!exclusive)
	{
		do
		{
			if (!(dbb->dbb_ast_flags & (DBB_shut_attach | DBB_shut_tran | DBB_shut_force)))
				break;

			if ((flag & isc_dpb_shut_transaction) && !TRA_active_transactions(tdbb, dbb))
			{
				successful = true;
				break;
			}

			if (timeout && CCH_exclusive(tdbb, LCK_PW, -1, guard))
			{
				exclusive = true;
				break;
			}
		}
		while (timeout--);
	}

	if (!exclusive && !successful &&
		(timeout > 0 || (flag & (isc_dpb_shut_attachment | isc_dpb_shut_transaction))))
	{
		notify_shutdown(tdbb, 0, -1, guard);	// Tell everyone we're giving up
		attachment->att_flags &= ~ATT_shutdown_manager;
		ERR_post(Arg::Gds(isc_shutfail));
	}

	if (!exclusive && !notify_shutdown(tdbb, shut_mode | isc_dpb_shut_force, 0, guard))
	{
		if (!CCH_exclusive(tdbb, LCK_PW, LCK_WAIT, guard))
		{
			notify_shutdown(tdbb, 0, -1, guard);	// Tell everyone we're giving up
			attachment->att_flags &= ~ATT_shutdown_manager;
			ERR_post(Arg::Gds(isc_shutfail));
		}
	}

	dbb->dbb_ast_flags &= ~(DBB_shut_force | DBB_shut_attach | DBB_shut_tran);

	WIN window(HEADER_PAGE_NUMBER);
	Ods::header_page* const header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
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

	SHUT_blocking_ast(tdbb, false);
}


void SHUT_online(thread_db* tdbb, SSHORT flag, Sync* guard)
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
	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Only platform's user locksmith can shutdown or bring online a database

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
			same_mode(dbb); // normal -> normal
			return;
		}
		break;
	case isc_dpb_shut_multi:
		if (!(dbb->dbb_ast_flags & DBB_shutdown))
		{
			bad_mode(dbb); // normal -> multi
		}
		if (!(dbb->dbb_ast_flags & DBB_shutdown_full) && !(dbb->dbb_ast_flags & DBB_shutdown_single))
		{
			same_mode(dbb); // multi -> multi
			return;
		}
		break;
	case isc_dpb_shut_single:
		if (dbb->dbb_ast_flags & DBB_shutdown_single)
		{
			same_mode(dbb); //single -> single
			return;
		}
		if (!(dbb->dbb_ast_flags & DBB_shutdown_full))
		{
			bad_mode(dbb); // !full -> single
		}
		break;
	case isc_dpb_shut_full:
		if (dbb->dbb_ast_flags & DBB_shutdown_full)
		{
			same_mode(dbb); // full -> full
			return;
		}
		bad_mode(dbb);
	default: // isc_dpb_shut_full
		bad_mode(dbb); // unexpected mode
	}

	// Reject exclusive and single-user shutdown attempts
	// for a physically locked database

	if (shut_mode == isc_dpb_shut_full || shut_mode == isc_dpb_shut_single)
	{
		check_backup_state(tdbb);
	}

	// Reset shutdown flag on database header page

	WIN window(HEADER_PAGE_NUMBER);
	Ods::header_page* const header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
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

	// Notify existing database clients that a currently scheduled shutdown is cancelled

	if (notify_shutdown(tdbb, shut_mode, -1, guard))
		CCH_release_exclusive(tdbb);
}


static void check_backup_state(thread_db* tdbb)
{
	Database* const dbb = tdbb->getDatabase();

	BackupManager::StateReadGuard stateGuard(tdbb);

	if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
	{
		ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(dbb->dbb_filename));
	}
}


static bool notify_shutdown(thread_db* tdbb, SSHORT flag, SSHORT delay, Sync* guard)
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
	Database* const dbb = tdbb->getDatabase();
	JAttachment* const jAtt = tdbb->getAttachment()->att_interface;

	shutdown_data data;
	data.data_items.flag = flag;
	data.data_items.delay = delay;

	LCK_write_data(tdbb, dbb->dbb_lock, data.data_long);

	{ // scope
		// Checkout before calling AST function
		MutexUnlockGuard uguard(*(jAtt->getMutex()), FB_FUNCTION);

		// Notify local attachments
		SHUT_blocking_ast(tdbb, true);
	}

	// Send blocking ASTs to other database users

	return CCH_exclusive(tdbb, LCK_PW, -1, guard);
}


static void shutdown(thread_db* tdbb, SSHORT flag, bool force)
{
/**************************************
 *
 *	s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *	Initiate database shutdown.
 *
 **************************************/
	Database* const dbb = tdbb->getDatabase();

	// Mark database and all active attachments as shutdown

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

	if (force)
	{
		bool found = false;
		for (Jrd::Attachment* attachment = dbb->dbb_attachments;
			attachment; attachment = attachment->att_next)
		{
			JAttachment* const jAtt = attachment->att_interface;
			MutexLockGuard guard(*(jAtt->getMutex(true)), FB_FUNCTION);

			if (!(attachment->att_flags & ATT_shutdown_manager))
			{
				if (!(attachment->att_flags & ATT_shutdown))
				{
					attachment->signalShutdown();
					found = true;
				}
			}
		}

		if (found)
			JRD_shutdown_attachments(dbb);
	}
}
