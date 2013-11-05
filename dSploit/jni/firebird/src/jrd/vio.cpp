/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		vio.cpp
 *	DESCRIPTION:	Virtual IO
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 * 2002.08.21 Dmitry Yemanov: fixed bug with a buffer overrun,
 *                            which at least caused invalid dependencies
 *                            to be stored (DB$xxx, for example)
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2002.12.22 Alex Peshkoff: Bugcheck(291) fix for update_in_place
 *							 of record, modified by pre_trigger
 * 2003.03.01 Nickolay Samofatov: Fixed database corruption when backing out
 *                           the savepoint after large number of DML operations
 *                           (so transaction-level savepoint is dropped) and
 *							 record was updated _not_ under the savepoint and
 *							 deleted under savepoint. Bug affected all kinds
 *							 of savepoints (explicit, statement, PSQL, ...)
 * 2003.03.02 Nickolay Samofatov: Use B+ tree to store undo log
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "gen/ids.h"
#include "../jrd/lck.h"
#include "../jrd/lls.h"
#include "../jrd/scl.h"
#include "../jrd/ibase.h"
#include "../jrd/flags.h"
#include "../jrd/ods.h"
#include "../jrd/os/pio.h"
#include "../jrd/btr.h"
#include "../jrd/exe.h"
#include "../jrd/rse.h"
#include "../jrd/ThreadStart.h"
#include "../jrd/thread_proto.h"
#ifdef VIO_DEBUG
#include "../jrd/vio_debug.h"
#endif
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/dbg_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sqz_proto.h"
#include "../jrd/tpc_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/StatusArg.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceJrdHelpers.h"

using namespace Jrd;
using namespace Firebird;

static void check_class(thread_db*, jrd_tra*, record_param*, record_param*, USHORT);
static void check_control(thread_db*);
static bool check_user(thread_db*, const dsc*);
static int check_precommitted(const jrd_tra*, const record_param*);
static void check_rel_field_class(thread_db*, record_param*, SecurityClass::flags_t, jrd_tra*);
static void delete_record(thread_db*, record_param*, SLONG, MemoryPool*);
static UCHAR* delete_tail(thread_db*, record_param*, SLONG, UCHAR*, const UCHAR*);
static void expunge(thread_db*, record_param*, const jrd_tra*, SLONG);
static bool dfw_should_know(record_param* org_rpb, record_param* new_rpb,
	USHORT irrelevant_field, bool void_update_is_relevant = false);
static void garbage_collect(thread_db*, record_param*, SLONG, RecordStack&);
static void garbage_collect_idx(thread_db*, record_param*, /*record_param*,*/ Record*, Record*);
#ifdef GARBAGE_THREAD
static THREAD_ENTRY_DECLARE garbage_collector(THREAD_ENTRY_PARAM);
#endif
static void invalidate_cursor_records(jrd_tra*, record_param*);
static void list_staying(thread_db*, record_param*, RecordStack&);
#ifdef GARBAGE_THREAD
static void notify_garbage_collector(thread_db*, record_param *, SLONG = -1);
#endif
static Record* realloc_record(Record*& record, USHORT fmt_length);

const int PREPARE_OK		= 0;
const int PREPARE_CONFLICT	= 1;
const int PREPARE_DELETE	= 2;
const int PREPARE_LOCKERR	= 3;

static int prepare_update(thread_db*, jrd_tra*, SLONG, record_param*,
						  record_param*, record_param*, PageStack&, bool);

static void purge(thread_db*, record_param*);
static Record* replace_gc_record(jrd_rel*, Record**, USHORT);
static void replace_record(thread_db*, record_param*, PageStack*, const jrd_tra*);
static void set_system_flag(thread_db*, record_param*, USHORT, SSHORT);
static void update_in_place(thread_db*, jrd_tra*, record_param*, record_param*);
static void verb_post(thread_db*, jrd_tra*, record_param*, Record*, //record_param*,
					  const bool, const bool);

// Pick up relation ids
#include "../jrd/ini.h"

#ifdef GARBAGE_THREAD
static const UCHAR gc_tpb[] =
{
	isc_tpb_version1, isc_tpb_read,
	isc_tpb_read_committed, isc_tpb_rec_version,
	isc_tpb_ignore_limbo
};
#endif

inline void clearRecordStack(RecordStack& stack)
{
/**************************************
 *
 *	c l e a r R e c o r d S t a c k
 *
 **************************************
 *
 * Functional description
 *	Clears stack, deleting each entry, popped from it.
 *
 **************************************/
	while (stack.hasData())
	{
		delete stack.pop();
	}
}

inline bool needDfw(thread_db* tdbb, const jrd_tra* transaction)
{
/**************************************
 *
 *	n e e d D f w
 *
 **************************************
 *
 * Functional description
 *	Checks, should DFW be called or not
 *	when system relations are modified.
 *
 **************************************/
	return !((transaction->tra_flags & TRA_system) || (tdbb->tdbb_flags & TDBB_dont_post_dfw));
}

IPTR VIO_savepoint_large(const Savepoint* savepoint, IPTR size)
{
/**************************************
 *
 *	s a v e p o i n t _ l a r g e
 *
 **************************************
 *
 * Functional description
 *	Returns an approximate size in bytes of savepoint in-memory data, i.e. a
 *  measure of how big the current savepoint has gotten.
 *
 *  Notes:
 *
 *  - This routine does not take into account the data allocated to 'vct_undo'.
 *   Why? Because this routine is used to estimate size of transaction-level
 *   savepoint and transaction-level savepoint may not contain undo data as it is
 *   always the first savepoint in transaction.
 *
 *  - Function stops counting when return value gets negative.
 *
 *  - We use IPTR, not SLONG to care of case when user savepoint gets very,
 *   very big on 64-bit machine. Its size may overflow 32 significant bits of
 *   SLONG in this case
 *
 **************************************/
	const VerbAction* verb_actions = savepoint->sav_verb_actions;

	// Iterate all tables changed under this savepoint
	while (verb_actions)
	{

		// Estimate size used for record backout bitmaps for this table
		if (verb_actions->vct_records) {
			size -= verb_actions->vct_records->approxSize();
		}

		if (size < 0) {
			break;
		}
		verb_actions = verb_actions->vct_next;
	}

	return size;
}

void VIO_backout(thread_db* tdbb, record_param* rpb, const jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ b a c k o u t
 *
 **************************************
 *
 * Functional description
 *	Backout the current version of a record.  This may called
 *	either because of transaction death or because the record
 *	violated a unique index.  In either case, get rid of the
 *	current version and back an old version.
 *
 *	This routine is called with an inactive record_param, and has to
 *	take great pains to avoid conflicting with another process
 *	which is also trying to backout the same record.  On exit
 *	there is no active record_param, and the record may or may not have
 *	been backed out, depending on whether we encountered conflict.
 *	But this record is doomed, and if we don't get it somebody
 *	will.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("VIO_backout (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT")\n",
				rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0);
	}
#endif

	jrd_rel* relation = rpb->rpb_relation;
	VIO_bump_count(tdbb, DBB_backout_count, relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_BACKOUTS);

	// If there is data in the record, fetch it now.  If the old version
	// is a differences record, we will need it sooner.  In any case, we
	// will need it eventually to clean up blobs and indices. If the record
	// has changed in between, stop now before things get worse.

	record_param temp = *rpb;
	if (!DPM_get(tdbb, &temp, LCK_read)) {
		return;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 temp.rpb_page, temp.rpb_line, temp.rpb_transaction_nr,
			 temp.rpb_flags, temp.rpb_b_page, temp.rpb_b_line,
			 temp.rpb_f_page, temp.rpb_f_line);
	}
	if ((temp.rpb_b_page != rpb->rpb_b_page || temp.rpb_b_line != rpb->rpb_b_line ||
			temp.rpb_transaction_nr != rpb->rpb_transaction_nr) &&
		debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    wrong record!)\n");
	}
#endif

	if (temp.rpb_b_page != rpb->rpb_b_page || temp.rpb_b_line != rpb->rpb_b_line ||
		temp.rpb_transaction_nr != rpb->rpb_transaction_nr)
	{
		CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
		return;
	}

	RecordStack going, staying;
	Record* data = NULL;
	Record* old_data = NULL;
	Record* gc_rec2 = NULL;

	bool samePage;
	bool deleted;

	if ((temp.rpb_flags & rpb_deleted) && (!(temp.rpb_flags & rpb_delta)))
		CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
	else
	{
		temp.rpb_record = VIO_gc_record(tdbb, relation);
		VIO_data(tdbb, &temp, dbb->dbb_permanent);
		data = temp.rpb_prior;
		old_data = temp.rpb_record;
		rpb->rpb_prior = temp.rpb_prior;
		gc_rec2 = temp.rpb_record;
		going.push(temp.rpb_record);
	}

	// Set up an extra record parameter block.  This will be used to preserve
	// the main record information while we chase fragments.

	record_param temp2 = temp = *rpb;

	// If there is an old version of the record, fetch it's data now.

	Record* gc_rec1 = NULL;

	if (rpb->rpb_b_page)
	{
		temp.rpb_record = gc_rec1 = VIO_gc_record(tdbb, relation);
		while (true)
		{
			if (!DPM_get(tdbb, &temp, LCK_read))
				goto gc_cleanup;
			if (temp.rpb_b_page != rpb->rpb_b_page || temp.rpb_b_line != rpb->rpb_b_line ||
				temp.rpb_transaction_nr != rpb->rpb_transaction_nr)
			{
				CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
				goto gc_cleanup;
			}
			if (temp.rpb_flags & rpb_delta)
				temp.rpb_prior = data;
			if (!DPM_fetch_back(tdbb, &temp, LCK_read, -1))
			{
				fb_utils::init_status(tdbb->tdbb_status_vector);

				continue;
			}
			if (temp.rpb_flags & rpb_deleted)
				CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
			else
				VIO_data(tdbb, &temp, dbb->dbb_permanent);
			gc_rec1 = temp.rpb_record;
			temp.rpb_page = rpb->rpb_b_page;
			temp.rpb_line = rpb->rpb_b_line;

			break;
		}
	}

	// Re-fetch the record.

	if (!DPM_get(tdbb, rpb, LCK_write))
		goto gc_cleanup;

#ifdef VIO_DEBUG
	if ((temp2.rpb_b_page != rpb->rpb_b_page || temp.rpb_b_line != rpb->rpb_b_line ||
			temp.rpb_transaction_nr != rpb->rpb_transaction_nr) &&
		debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    record changed!)\n");
	}
#endif

	// If the record is in any way suspicious, release the record and give up.

	if (rpb->rpb_b_page != temp2.rpb_b_page || rpb->rpb_b_line != temp2.rpb_b_line ||
		rpb->rpb_transaction_nr != temp2.rpb_transaction_nr)
	{
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		goto gc_cleanup;
	}

	// even if the record isn't suspicious, it may have changed a little

	temp2 = *rpb;
	rpb->rpb_undo = old_data;

	if (rpb->rpb_flags & rpb_delta)
		rpb->rpb_prior = data;

	// Handle the case of no old version simply.

	if (!rpb->rpb_b_page)
	{
		if (!(rpb->rpb_flags & rpb_deleted))
		{
			DPM_backout_mark(tdbb, rpb, transaction);

			RecordStack empty_staying;
			BLB_garbage_collect(tdbb, going, empty_staying, rpb->rpb_page, relation);
			IDX_garbage_collect(tdbb, rpb, going, empty_staying);
			going.pop();

			if (!DPM_get(tdbb, rpb, LCK_write))
				BUGCHECK(186);	// msg 186 record disappeared
		}

		delete_record(tdbb, rpb, (SLONG) 0, 0);
		goto gc_cleanup;
	}

	// If both record versions are on the same page, things are a little simpler

	samePage = (rpb->rpb_page == temp.rpb_page && !rpb->rpb_prior);
	deleted = (temp2.rpb_flags & rpb_deleted);
	if (!deleted)
	{
		DPM_backout_mark(tdbb, rpb, transaction);

		rpb->rpb_prior = NULL;
		list_staying(tdbb, rpb, staying);
		BLB_garbage_collect(tdbb, going, staying, rpb->rpb_page, relation);
		IDX_garbage_collect(tdbb, rpb, going, staying);

		if (going.hasData())
		{
			going.pop();
		}

		clearRecordStack(staying);

		if (!DPM_get(tdbb, rpb, LCK_write))
			BUGCHECK(186);	// msg 186 record disappeared
	}

	if (samePage)
	{
		DPM_backout(tdbb, rpb);
		if (!deleted) {
			delete_tail(tdbb, &temp2, rpb->rpb_page, 0, 0);
		}
	}
	else
	{
		// Bring the old version forward.  If the outgoing version was deleted,
		// there is no garbage collection to be done.

		rpb->rpb_address = temp.rpb_address;
		rpb->rpb_length = temp.rpb_length;
		rpb->rpb_flags = temp.rpb_flags & rpb_deleted;
		if (temp.rpb_prior)
			rpb->rpb_flags |= rpb_delta;
		rpb->rpb_b_page = temp.rpb_b_page;
		rpb->rpb_b_line = temp.rpb_b_line;
		rpb->rpb_transaction_nr = temp.rpb_transaction_nr;
		rpb->rpb_format_number = temp.rpb_format_number;

		if (deleted)
		{
			replace_record(tdbb, rpb, 0, transaction);
		}
		else
		{
			// There is cleanup to be done.  Bring the old version forward first

			rpb->rpb_flags &= ~(rpb_fragment | rpb_incomplete | rpb_chained | rpb_gc_active);
			DPM_update(tdbb, rpb, 0, transaction);
			delete_tail(tdbb, &temp2, rpb->rpb_page, 0, 0);
		}

		// Next, delete the old copy of the now current version.

		if (!DPM_fetch(tdbb, &temp, LCK_write))
			BUGCHECK(291);		// msg 291 cannot find record back version
		delete_record(tdbb, &temp, rpb->rpb_page, 0);
	}

	// Return relation garbage collect record blocks to vector.

  gc_cleanup:
	if (gc_rec1)
		gc_rec1->rec_flags &= ~REC_gc_active;
	if (gc_rec2)
		gc_rec2->rec_flags &= ~REC_gc_active;
}


void VIO_bump_count(thread_db* tdbb, USHORT count_id, jrd_rel* relation)
{
/**************************************
 *
 *	V I O _ b u m p _ c o u n t
 *
 **************************************
 *
 * Functional description
 *	Bump a usage count.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf("bump_count (count_id %d, table %d)\n", count_id,
				  relation ? relation->rel_id : 0);
	}
#endif


	const USHORT relation_id = relation->rel_id;
	vcl** ptr = attachment->att_counts + count_id;

	vcl* vector = *ptr = vcl::newVector(*attachment->att_pool, *ptr, relation_id + 1);
	((*vector)[relation_id])++;

	tdbb->bumpStats((RuntimeStatistics::StatType) count_id, relation_id);
}


bool VIO_chase_record_version(thread_db* tdbb, record_param* rpb,
							  jrd_tra* transaction,
							  MemoryPool* pool, bool writelock)
{
/**************************************
 *
 *	V I O _ c h a s e _ r e c o r d _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	This is the key routine in all of JRD.  Given a record, determine
 *	what the version, if any, is appropriate for this transaction.  This
 *	is primarily done by playing with transaction numbers.  If, in the
 *	process, a record is found that requires garbage collection, by all
 *	means garbage collect it.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = transaction->tra_attachment;

#ifdef GARBAGE_THREAD
	const bool gcPolicyCooperative = tdbb->getDatabase()->dbb_flags & DBB_gc_cooperative;
	const bool gcPolicyBackground = tdbb->getDatabase()->dbb_flags & DBB_gc_background;
#endif
	const SLONG oldest_snapshot = rpb->rpb_relation->isTemporary() ? 
		attachment->att_oldest_snapshot : transaction->tra_oldest_active;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf
			("VIO_chase_record_version (record_param %"QUADFORMAT"d, transaction %"
			 SLONGFORMAT", pool %p)\n",
			 rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0,
			 (void*) pool);
	}
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// Handle the fast path first.  If the record is committed, isn't deleted,
	// and doesn't have an old version that is a candidate for garbage collection,
	// return without further ado

	int state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);

	// Reset the garbage collect active flag if the transaction state is
	// in a terminal state. If committed it must have been a precommitted
	// transaction that was backing out a dead record version and the
	// system crashed. Clear the flag and set the state to tra_dead to
	// reattempt the backout.

	if (rpb->rpb_flags & rpb_gc_active)
	{
		if (!rpb->rpb_transaction_nr) {
			state = tra_active;
		}

		if (state == tra_committed) {
			state = tra_dead;
		}

		if (state == tra_dead) {
			rpb->rpb_flags &= ~rpb_gc_active;
		}
	}

	if ((state == tra_committed || state == tra_us) &&
		!(rpb->rpb_flags & (rpb_deleted | rpb_damaged)) &&
		(rpb->rpb_b_page == 0 || rpb->rpb_transaction_nr >= oldest_snapshot))
	{
#ifdef GARBAGE_THREAD
		if (gcPolicyBackground && rpb->rpb_b_page)
			notify_garbage_collector(tdbb, rpb);
#endif // GARBAGE_THREAD

		return true;
	}

	// OK, something about the record is fishy.  Loop thru versions until a
	// satisfactory version is found or we run into a brick wall.  Do any
	// garbage collection that seems appropriate.

	// First, save the record indentifying information to be restored on exit

	while (true)
	{
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_READS_INFO)
		{
			printf
				("   chase record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
				 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
				 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
				 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
				 rpb->rpb_f_page, rpb->rpb_f_line);
		}
#endif
		if (rpb->rpb_flags & rpb_damaged)
		{
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			return false;
		}
		if (state == tra_limbo && !(transaction->tra_flags & TRA_ignore_limbo))
		{
			state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_wait);
			if (state == tra_active)
				state = tra_limbo;
		}
		if (state == tra_precommitted)
			state = check_precommitted(transaction, rpb);

		// If the transaction is a read committed and chooses the no version
		// option, wait for reads also!

		if ((transaction->tra_flags & TRA_read_committed) &&
			(!(transaction->tra_flags & TRA_rec_version) || writelock))
		{
			if (state == tra_limbo)
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_wait);

				if (!DPM_get(tdbb, rpb, LCK_read))
					return false;

				state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);

				// will come back with active if lock mode is no wait

				if (state == tra_active)
				{
					// error if we cannot ignore limbo, else fall through
					// to next version

					if (!(transaction->tra_flags & TRA_ignore_limbo))
					{
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						ERR_post(Arg::Gds(isc_deadlock) <<
								 Arg::Gds(isc_trainlim));
					}

					state = tra_limbo;
				}
			}
			else if (state == tra_active && !(rpb->rpb_flags & rpb_gc_active))
			{
				// A read committed, no record version transaction has to wait
				// if the record has been modified by an active transaction. But
				// it shouldn't wait if this is a transient fragmented backout
				// of a dead record version.

				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_wait);

				if (state == tra_precommitted)
					state = check_precommitted(transaction, rpb);

				if (state == tra_active)
					ERR_post(Arg::Gds(isc_deadlock));

				// refetch the record and try again.  The active transaction
				// could have updated the record a second time.
				// go back to outer loop


				if (!DPM_get(tdbb, rpb, LCK_read)) {
					return false;
				}

				state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);
				continue;
			}
		}

		switch (state)
		{
			// If it's dead, back it out, if possible.  Otherwise continue to chase backward

		case tra_dead:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is dead (my TID - %"SLONGFORMAT")\n",
						rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif
#ifdef GARBAGE_THREAD
			if (!(rpb->rpb_flags & rpb_chained) && attachment->att_flags & ATT_notify_gc)
			{
				notify_garbage_collector(tdbb, rpb);
			}

#endif
		case tra_precommitted:

			if (attachment->att_flags & ATT_NO_CLEANUP ||
				rpb->rpb_flags & (rpb_chained | rpb_gc_active))
			{
				if (rpb->rpb_b_page == 0) {
					CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
					return false;
				}
				record_param temp = *rpb;
				if ((!(rpb->rpb_flags & rpb_deleted)) || (rpb->rpb_flags & rpb_delta))
				{
					VIO_data(tdbb, rpb, pool);
					rpb->rpb_page = temp.rpb_page;
					rpb->rpb_line = temp.rpb_line;

					if (!(DPM_fetch(tdbb, rpb, LCK_read)))
					{
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;
						}
						break;
					}

					if (rpb->rpb_b_page != temp.rpb_b_page || rpb->rpb_b_line != temp.rpb_b_line ||
						rpb->rpb_f_page != temp.rpb_f_page || rpb->rpb_f_line != temp.rpb_f_line ||
						rpb->rpb_flags != temp.rpb_flags)
					{
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;
						}
						break;
					}
					if (temp.rpb_transaction_nr != rpb->rpb_transaction_nr) {
						break;
					}
					if (rpb->rpb_b_page == 0) {
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						return false;
					}
					if (rpb->rpb_flags & rpb_delta) {
						rpb->rpb_prior = rpb->rpb_record;
					}
				}
				// Fetch a back version.  If a latch timeout occurs, refetch the
				// primary version and start again.  If the primary version is
				// gone, then return 'record not found'.
				if (!DPM_fetch_back(tdbb, rpb, LCK_read, -1))
				{
					if (!DPM_get(tdbb, rpb, LCK_read)) {
						return false;
					}
				}
				break;
			}

			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			VIO_backout(tdbb, rpb, transaction);
			if (!DPM_get(tdbb, rpb, LCK_read)) {
				return false;
			}
			break;

			// If it's active, prepare to fetch the old version.

		case tra_limbo:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is in limbo (my TID - %"SLONGFORMAT")\n",
						rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif

			if (!(transaction->tra_flags & TRA_ignore_limbo))
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				ERR_post(Arg::Gds(isc_rec_in_limbo) << Arg::Num(rpb->rpb_transaction_nr));
			}

		case tra_active:
#ifdef VIO_DEBUG
			if ((debug_flag > DEBUG_READS_INFO) && (state == tra_active))
			{
				printf("    record's transaction (%"SLONGFORMAT") is active (my TID - %"SLONGFORMAT")\n",
						rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif

			// we can't use this one so if there aren't any more just stop now.

			if (rpb->rpb_b_page == 0)
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				return false;
			}

#ifdef GARBAGE_THREAD
			// hvlad: if I'm garbage collector I don't need to read backversion
			// of active record. Just do notify self about it
			if (attachment->att_flags & ATT_garbage_collector)
			{
				notify_garbage_collector(tdbb, rpb);
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				return false;
			}
#endif

			if (!(rpb->rpb_flags & rpb_delta))
			{
				rpb->rpb_prior = NULL;
				// Fetch a back version.  If a latch timeout occurs, refetch the
				// primary version and start again.  If the primary version is
				// gone, then return 'record not found'.
				if (!DPM_fetch_back(tdbb, rpb, LCK_read, -1))
				{
					if (!DPM_get(tdbb, rpb, LCK_read)) {
						return false;
					}
				}
				break;
			}
			else
			{
				// oh groan, we've got to get data.  This means losing our lock and that
				// means possibly having the world change underneath us.  Specifically, the
				// primary record may change (because somebody modified or backed it out) and
				// the first record back may disappear because the primary record was backed
				// out, and now the first backup back in the primary record's place.

				record_param temp = *rpb;
				VIO_data(tdbb, rpb, pool);
				if (temp.rpb_flags & rpb_chained)
				{
					rpb->rpb_page = temp.rpb_b_page;
					rpb->rpb_line = temp.rpb_b_line;
					if (!DPM_fetch(tdbb, rpb, LCK_read))
					{
						// Things have changed, start all over again.
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;	// entire record disappeared
						}
						break;	// start from the primary version again
					}
				}
				else
				{
					rpb->rpb_page = temp.rpb_page;
					rpb->rpb_line = temp.rpb_line;
					if (!DPM_fetch(tdbb, rpb, LCK_read))
					{
						// Things have changed, start all over again.
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;	// entire record disappeared
						}
						break;	// start from the primary version again
					}
					if (rpb->rpb_transaction_nr != temp.rpb_transaction_nr)
					{
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;
						}
						break;
					}
					if (rpb->rpb_b_page == 0)
					{
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						return false;
					}
					if (!(rpb->rpb_flags & rpb_delta))
						rpb->rpb_prior = NULL;
					// Fetch a back version.  If a latch timeout occurs, refetch the
					// primary version and start again.  If the primary version is
					// gone, then return 'record not found'.
					if (!DPM_fetch_back(tdbb, rpb, LCK_read, -1))
					{
						if (!DPM_get(tdbb, rpb, LCK_read)) {
							return false;
						}
					}
				}
			}
			break;

		case tra_us:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is us (my TID - %"SLONGFORMAT")\n",
						rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif

			if (rpb->rpb_flags & rpb_deleted)
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				return false;
			}
			return true;

			// If it's committed, worry a bit about garbage collection.

		case tra_committed:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is committed (my TID - %"SLONGFORMAT")\n",
						rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif
			if (rpb->rpb_flags & rpb_deleted)
			{
				if (rpb->rpb_transaction_nr < oldest_snapshot &&
					!(attachment->att_flags & ATT_no_cleanup))
				{
#ifdef GARBAGE_THREAD
					if (!gcPolicyCooperative && (attachment->att_flags & ATT_notify_gc) &&
						!rpb->rpb_relation->isTemporary())
					{
						notify_garbage_collector(tdbb, rpb);
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
					}
					else
#endif
					{
						CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
						expunge(tdbb, rpb, transaction, (SLONG) 0);
					}
					return false;
				}
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				return false;
			}

			// Check if no garbage collection can (should) be done.
			// It might be important not to garbage collect if the primary
			// record version is not yet committed because garbage collection
			// might interfere with the updater (prepare_update, update_in_place...).
			// That might be the reason for the rpb_chained check.

			const bool cannotGC =
				rpb->rpb_transaction_nr >= oldest_snapshot || rpb->rpb_b_page == 0 ||
				rpb->rpb_flags & rpb_chained || attachment->att_flags & ATT_no_cleanup;

			if (cannotGC)
			{
#ifdef GARBAGE_THREAD
				if (gcPolicyBackground &&
					attachment->att_flags & (ATT_notify_gc | ATT_garbage_collector) &&
					(rpb->rpb_b_page != 0 && !(rpb->rpb_flags & rpb_chained)) )
				{
					// VIO_chase_record_version
					notify_garbage_collector(tdbb, rpb);
				}
#endif
				return true;
			}

			// Garbage collect.

#ifdef GARBAGE_THREAD
			if (!gcPolicyCooperative && (attachment->att_flags & ATT_notify_gc) &&
				!rpb->rpb_relation->isTemporary())
			{
				notify_garbage_collector(tdbb, rpb);
				return true;
			}
#endif
			purge(tdbb, rpb);

			// Go back to be primary record version and chase versions all over again.
			if (!DPM_get(tdbb, rpb, LCK_read)) {
				return false;
			}
		}

		state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);

		// Reset the garbage collect active flag if the transaction state is
		// in a terminal state. If committed it must have been a precommitted
		// transaction that was backing out a dead record version and the
		// system crashed. Clear the flag and set the state to tra_dead to
		// reattempt the backout.

		if (!(rpb->rpb_flags & rpb_chained) && rpb->rpb_flags & rpb_gc_active)
		{
			if (!rpb->rpb_transaction_nr) {
				state = tra_active;
			}

			if (state == tra_committed) {
				state = tra_dead;
			}

			if (state == tra_dead) {
				rpb->rpb_flags &= ~rpb_gc_active;
			}
		}
	}
}


void VIO_data(thread_db* tdbb, record_param* rpb, MemoryPool* pool)
{
/**************************************
 *
 *	V I O _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Given an active record parameter block, fetch the full record.
 *
 *	This routine is called with an active record_param and exits with
 *	an INactive record_param.  Yes, Virginia, getting the data for a
 *	record means losing control of the record.  This turns out
 *	to matter a lot.
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
	{
		printf("VIO_data (record_param %"QUADFORMAT"d, pool %p)\n",
					rpb->rpb_number.getValue(), (void*) pool);
	}
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
				 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
				 rpb->rpb_page, rpb->rpb_line,
				 rpb->rpb_transaction_nr, rpb->rpb_flags,
				 rpb->rpb_b_page, rpb->rpb_b_line,
				 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// If we're not already set up for this format version number, find
	// the format block and set up the record block.  This is a performance
	// optimization.

	Record* record = VIO_record(tdbb, rpb, 0, pool);
	const Format* format = record->rec_format;

	// If the record is a delta version, start with data from prior record.
	UCHAR* tail;
	const UCHAR* tail_end;
	UCHAR differences[MAX_DIFFERENCES];
	Record* prior = rpb->rpb_prior;
	if (prior)
	{
		tail = differences;
		tail_end = differences + sizeof(differences);
		if (prior != record)
		{
			if (record->rec_length < prior->rec_length)
			{
				if (record->rec_flags & REC_gc_active) {
					record = replace_gc_record(rpb->rpb_relation, &rpb->rpb_record, prior->rec_length);
				}
				else {
					record = realloc_record(rpb->rpb_record, prior->rec_length);
				}
			}
			memcpy(record->rec_data, prior->rec_data, prior->rec_format->fmt_length);
		}
	}
	else
	{
		tail = record->rec_data;
		tail_end = tail + record->rec_length;
	}

	// Set up prior record point for next version

	rpb->rpb_prior = (rpb->rpb_b_page && (rpb->rpb_flags & rpb_delta)) ? record : NULL;

	// Snarf data from record

	tail = SQZ_decompress(rpb->rpb_address, rpb->rpb_length, tail, tail_end);

	if (rpb->rpb_flags & rpb_incomplete)
	{
		const ULONG back_page  = rpb->rpb_b_page;
		const USHORT back_line = rpb->rpb_b_line;
		while (rpb->rpb_flags & rpb_incomplete)
		{
			DPM_fetch_fragment(tdbb, rpb, LCK_read);

			tail = SQZ_decompress(rpb->rpb_address, rpb->rpb_length, tail, tail_end);
		}
		rpb->rpb_b_page = back_page;
		rpb->rpb_b_line = back_line;
	}

	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));

	// If this is a delta version, apply changes
	USHORT length;
	if (prior)
	{
		length =
			SQZ_apply_differences(record,
								  reinterpret_cast<char*>(differences),
								  reinterpret_cast<char*>(tail));
	}
	else
	{
		length = tail - record->rec_data;
	}

	if (format->fmt_length != length)
	{
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES)
		{
			printf ("VIO_erase (record_param %"QUADFORMAT"d, length %d expected %d)\n",
				rpb->rpb_number.getValue(), length, format->fmt_length);
		}

		if (debug_flag > DEBUG_WRITES_INFO)
		{
			printf ("   record  %"SLONGFORMAT"d:%d, rpb_trans %"SLONGFORMAT
					   "d, flags %d, back %"SLONGFORMAT"d:%d, fragment %"SLONGFORMAT"d:%d\n",
				rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr, rpb->rpb_flags,
			    rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page, rpb->rpb_f_line);
		}
#endif
		BUGCHECK(183);			// msg 183 wrong record length
	}

	rpb->rpb_address = record->rec_data;
	rpb->rpb_length = format->fmt_length;
}


void VIO_erase(thread_db* tdbb, record_param* rpb, jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Erase an existing record.
 *
 *	This routine is entered with an inactive
 *	record_param and leaves having created an erased
 *	stub.
 *
 **************************************/

	// Revokee is only 32 bytes. UserId would be truncated.
	SqlIdentifier relation_name, revokee, privilege, procedure_name;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = tdbb->getRequest();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("VIO_erase (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT")\n",
				  rpb->rpb_number.getValue(), transaction->tra_number);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// If the stream was sorted, the various fields in the rpb are
	// probably junk.  Just to make sure that everything is cool, refetch the record.

	if (rpb->rpb_stream_flags & RPB_s_refetch)
	{
		VIO_refetch_record(tdbb, rpb, transaction);
		rpb->rpb_stream_flags &= ~RPB_s_refetch;
	}

	bool same_tx = false;
	if (rpb->rpb_transaction_nr == transaction->tra_number) {
		same_tx = true;			// deleting tx has updated/inserted this record before
	}

	// Special case system transaction

	if (transaction->tra_flags & TRA_system)
	{
		VIO_backout(tdbb, rpb, transaction);
		return;
	}

	transaction->tra_flags |= TRA_write;
	jrd_rel* relation = rpb->rpb_relation;

	// If we're about to erase a system relation, check to make sure
	// everything is completely kosher.

	DSC desc, desc2;

	if (needDfw(tdbb, transaction))
	{
		jrd_rel* r2;
		const jrd_prc* procedure;
		USHORT id;
		DeferredWork* work;

		switch ((RIDS) relation->rel_id)
		{
		case rel_relations:
			if (EVL_field(0, rpb->rpb_record, f_rel_name, &desc))
			{
				SCL_check_relation(tdbb, &desc, SCL_delete);
			}
			if (EVL_field(0, rpb->rpb_record, f_rel_id, &desc2))
			{
				id = MOV_get_long(&desc2, 0);
				if (id <= dbb->dbb_max_sys_rel)
				{
					IBERROR(187);	// msg 187 cannot delete system relations
				}
				DFW_post_work(transaction, dfw_delete_relation, &desc, id);
				jrd_rel* rel_drop = MET_lookup_relation_id(tdbb, id, false);
				if (rel_drop)
					MET_scan_relation(tdbb, rel_drop);
			}
			break;

		case rel_procedures:
			if (EVL_field(0, rpb->rpb_record, f_prc_name, &desc))
			{
				SCL_check_procedure(tdbb, &desc, SCL_delete);
			}
			EVL_field(0, rpb->rpb_record, f_prc_id, &desc2);
			id = MOV_get_long(&desc2, 0);
			DFW_post_work(transaction, dfw_delete_procedure, &desc, id);
			MET_lookup_procedure_id(tdbb, id, false, true, 0);
			break;

		case rel_collations:
			EVL_field(0, rpb->rpb_record, f_coll_cs_id, &desc2);
			id = MOV_get_long(&desc2, 0);

			EVL_field(0, rpb->rpb_record, f_coll_id, &desc2);
			id = INTL_CS_COLL_TO_TTYPE(id, MOV_get_long(&desc2, 0));

			EVL_field(0, rpb->rpb_record, f_coll_name, &desc);
			DFW_post_work(transaction, dfw_delete_collation, &desc, id);
			break;

		case rel_exceptions:
			EVL_field(0, rpb->rpb_record, f_prc_name, &desc);
			DFW_post_work(transaction, dfw_delete_exception, &desc, 0);
			break;

		case rel_gens:
			EVL_field (0, rpb->rpb_record, f_gen_name, &desc);
			DFW_post_work(transaction, dfw_delete_generator, &desc, 0);
			break;

		case rel_funs:
			EVL_field (0, rpb->rpb_record, f_fun_name, &desc);
			DFW_post_work(transaction, dfw_delete_udf, &desc, 0);
			break;

		case rel_indices:
			EVL_field(0, rpb->rpb_record, f_idx_relation, &desc);
			SCL_check_relation(tdbb, &desc, SCL_control);
			EVL_field(0, rpb->rpb_record, f_idx_id, &desc2);
			if ( (id = MOV_get_long(&desc2, 0)) )
			{
				MOV_get_metadata_str(&desc, relation_name, sizeof(relation_name));
				r2 = MET_lookup_relation(tdbb, relation_name);
				fb_assert(r2);

				DSC idx_name;
				EVL_field(0, rpb->rpb_record, f_idx_name, &idx_name);

				// hvlad: lets add index name to the DFW item even if we add it again later within
				// additional argument. This is needed to make DFW work items different for different
				// indexes dropped at the same transaction and to not merge them at DFW_merge_work.
				if (EVL_field(0, rpb->rpb_record, f_idx_exp_blr, &desc2)) {
					work = DFW_post_work(transaction, dfw_delete_expression_index, &idx_name, r2->rel_id);
				}
				else {
					work = DFW_post_work(transaction, dfw_delete_index, &idx_name, r2->rel_id);
				}

				// add index id and name (the latter is required to delete dependencies correctly)
				DFW_post_work_arg(transaction, work, &idx_name, id, dfw_arg_index_name);

				// get partner relation for FK index
				if (EVL_field(0, rpb->rpb_record, f_idx_foreign, &desc2))
				{
					DSC desc3;
					EVL_field(0, rpb->rpb_record, f_idx_name, &desc3);

					SqlIdentifier index_name;
					MOV_get_metadata_str(&desc3, index_name, sizeof(index_name));

					jrd_rel *partner;
					index_desc idx;

					if ((BTR_lookup(tdbb, r2, id - 1, &idx, r2->getPages(tdbb)) == FB_SUCCESS) &&
						MET_lookup_partner(tdbb, r2, &idx, index_name) &&
						(partner = MET_lookup_relation_id(tdbb, idx.idx_primary_relation, false)) )
					{
						DFW_post_work_arg(transaction, work, 0, partner->rel_id,
										  dfw_arg_partner_rel_id);
					}
					else
					{	// can't find partner relation - impossible ?
						// add empty argument to let DFW know dropping
						// index was bound with FK
						DFW_post_work_arg(transaction, work, 0, 0, dfw_arg_partner_rel_id);
					}
				}
			}
			break;

		case rel_rfr:
			EVL_field(0, rpb->rpb_record, f_rfr_rname, &desc);
			SCL_check_relation(tdbb, &desc, SCL_control);
			DFW_post_work(transaction, dfw_update_format, &desc, 0);
			EVL_field(0, rpb->rpb_record, f_rfr_fname, &desc2);
			MOV_get_metadata_str(&desc, relation_name, sizeof(relation_name));
			if ( (r2 = MET_lookup_relation(tdbb, relation_name)) )
			{
				DFW_post_work(transaction, dfw_delete_rfr, &desc2, r2->rel_id);
			}
			EVL_field(0, rpb->rpb_record, f_rfr_sname, &desc2);
			DFW_post_work(transaction, dfw_delete_global, &desc2, 0);
			break;

		case rel_prc_prms:
			EVL_field(0, rpb->rpb_record, f_prm_procedure, &desc);
			SCL_check_procedure(tdbb, &desc, SCL_control);
			EVL_field(0, rpb->rpb_record, f_prm_name, &desc2);
			MOV_get_metadata_str(&desc, procedure_name, sizeof(procedure_name));
			if ( (procedure = MET_lookup_procedure(tdbb, procedure_name, true)) )
			{
				work = DFW_post_work(transaction, dfw_delete_prm, &desc2, procedure->prc_id);

				// procedure name to track parameter dependencies
				DFW_post_work_arg(transaction, work, &desc, procedure->prc_id, dfw_arg_proc_name);
			}
			EVL_field(0, rpb->rpb_record, f_prm_sname, &desc2);
			DFW_post_work(transaction, dfw_delete_global, &desc2, 0);
			break;

		case rel_fields:
			check_control(tdbb);
			EVL_field(0, rpb->rpb_record, f_fld_name, &desc);
			DFW_post_work(transaction, dfw_delete_field, &desc, 0);
			MET_change_fields(tdbb, transaction, &desc);
			break;

		case rel_files:
			{
				const bool name_defined = EVL_field(0, rpb->rpb_record, f_file_name, &desc);
				const USHORT file_flags = EVL_field(0, rpb->rpb_record, f_file_flags, &desc2) ?
					MOV_get_long(&desc2, 0) : 0;
				if (file_flags & FILE_difference)
				{
					if (file_flags & FILE_backing_up)
						DFW_post_work(transaction, dfw_end_backup, &desc, 0);
					if (name_defined)
						DFW_post_work(transaction, dfw_delete_difference, &desc, 0);
				}
				else if (EVL_field(0, rpb->rpb_record, f_file_shad_num, &desc2) &&
					(id = MOV_get_long(&desc2, 0)))
				{
					if (!(file_flags & FILE_inactive)) {
						DFW_post_work(transaction, dfw_delete_shadow, &desc, id);
					}
				}
			}
			break;

		case rel_classes:
			EVL_field(0, rpb->rpb_record, f_cls_class, &desc);
			DFW_post_work(transaction, dfw_compute_security, &desc, 0);
			break;

		case rel_triggers:
			EVL_field(0, rpb->rpb_record, f_trg_rname, &desc);

			// check if this  request go through without checking permissions
			if (!(request->req_flags & req_ignore_perm)) {
				SCL_check_relation(tdbb, &desc, SCL_control);
			}

			EVL_field(0, rpb->rpb_record, f_trg_rname, &desc2);
			DFW_post_work(transaction, dfw_update_format, &desc2, 0);
			EVL_field(0, rpb->rpb_record, f_trg_name, &desc);
			work = DFW_post_work(transaction, dfw_delete_trigger, &desc, 0);

			if (!(desc2.dsc_flags & DSC_null))
				DFW_post_work_arg(transaction, work, &desc2, 0, dfw_arg_rel_name);

			if (EVL_field(0, rpb->rpb_record, f_trg_type, &desc2))
			{
				DFW_post_work_arg(transaction, work, &desc2,
								  MOV_get_long(&desc2, 0), dfw_arg_trg_type);
			}

			break;

		case rel_priv:
			EVL_field(0, rpb->rpb_record, f_file_name, &desc);
			if (!(tdbb->getRequest()->req_flags & req_internal))
			{
				EVL_field(0, rpb->rpb_record, f_prv_grantor, &desc);
				if (!check_user(tdbb, &desc))
				{
					ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("REVOKE") <<
													  Arg::Str("TABLE") <<
													  Arg::Str("RDB$USER_PRIVILEGES"));
				}
			}
			EVL_field(0, rpb->rpb_record, f_prv_rname, &desc);
			EVL_field(0, rpb->rpb_record, f_prv_o_type, &desc2);
			id = MOV_get_long(&desc2, 0);
			DFW_post_work(transaction, dfw_grant, &desc, id);
			break;

		default:    // Shut up compiler warnings
			break;
		}
	}

	// We're about to erase the record. Post a refetch request
	// to all the active cursors positioned at this record.

	invalidate_cursor_records(transaction, rpb);

	// If the page can be updated simply, we can skip the remaining crud

	record_param temp;
	temp.rpb_transaction_nr = transaction->tra_number;
	temp.rpb_address = NULL;
	temp.rpb_length = 0;
	temp.rpb_flags = rpb_deleted;
	temp.rpb_format_number = rpb->rpb_format_number;
	temp.getWindow(tdbb).win_flags = WIN_secondary;

	const SLONG tid_fetch = rpb->rpb_transaction_nr;
	if (DPM_chain(tdbb, rpb, &temp))
	{
		rpb->rpb_b_page = temp.rpb_b_page;
		rpb->rpb_b_line = temp.rpb_b_line;
		rpb->rpb_flags |= rpb_deleted;
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
		{
			printf("   VIO_erase: successfully chained\n");
		}
#endif
	}
	else
	{
		// Update stub didn't find one page -- do a long, hard update
		PageStack stack;
		if (prepare_update(tdbb, transaction, tid_fetch, rpb, &temp, 0, stack, false))
		{
			ERR_post(Arg::Gds(isc_deadlock) <<
					 Arg::Gds(isc_update_conflict) <<
					 Arg::Gds(isc_concurrent_transaction) << Arg::Num(rpb->rpb_transaction_nr));
		}

		// Old record was restored and re-fetched for write.  Now replace it.

		rpb->rpb_transaction_nr = transaction->tra_number;
		rpb->rpb_b_page = temp.rpb_page;
		rpb->rpb_b_line = temp.rpb_line;
		rpb->rpb_address = NULL;
		rpb->rpb_length = 0;
		rpb->rpb_flags |= rpb_deleted;
		rpb->rpb_flags &= ~rpb_delta;

		replace_record(tdbb, rpb, &stack, transaction);
	}

	// Check to see if recursive revoke needs to be propagated

	if ((RIDS) relation->rel_id == rel_priv)
	{
		EVL_field(0, rpb->rpb_record, f_prv_rname, &desc);
		MOV_get_metadata_str(&desc, relation_name, sizeof(relation_name));
		EVL_field(0, rpb->rpb_record, f_prv_grant, &desc2);
		if (MOV_get_long(&desc2, 0))
		{
			EVL_field(0, rpb->rpb_record, f_prv_user, &desc2);
			MOV_get_metadata_str(&desc2, revokee, sizeof(revokee));
			EVL_field(0, rpb->rpb_record, f_prv_priv, &desc2);
			MOV_get_metadata_str(&desc2, privilege, sizeof(privilege));
			MET_revoke(tdbb, transaction, relation_name, revokee, privilege);
		}
	}
	if (!(transaction->tra_flags & TRA_system) &&
		transaction->tra_save_point && transaction->tra_save_point->sav_verb_count)
	{
		verb_post(tdbb, transaction, rpb, 0, /*0,*/ same_tx, false);
	}

	VIO_bump_count(tdbb, DBB_delete_count, relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_DELETES);

	// for an autocommit transaction, mark a commit as necessary

	if (transaction->tra_flags & TRA_autocommit)
	{
		transaction->tra_flags |= TRA_perform_autocommit;
	}

#ifdef GARBAGE_THREAD
	// VIO_erase
	if ((tdbb->getDatabase()->dbb_flags & DBB_gc_background) && !rpb->rpb_relation->isTemporary())
	{
		notify_garbage_collector(tdbb, rpb, transaction->tra_number);
	}
#endif
}


#ifdef GARBAGE_THREAD
void VIO_fini(thread_db* tdbb)
{
/**************************************
 *
 *	V I O _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Shutdown the garbage collector thread.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_flags & DBB_garbage_collector)
	{
		dbb->dbb_flags &= ~DBB_garbage_collector;
		dbb->dbb_gc_sem.release(); // Wake up running thread
		{ // scope
			Database::Checkout dcoHolder(dbb);
			dbb->dbb_gc_fini.enter();
		}
	}
}
#endif


bool VIO_garbage_collect(thread_db* tdbb, record_param* rpb, const jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ g a r b a g e _ c o l l e c t
 *
 **************************************
 *
 * Functional description
 *	Do any garbage collection appropriate to the current
 *	record.  This is called during index creation to avoid
 *	unnecessary work as well as false duplicate records.
 *
 *	If the record complete goes away, return false.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = transaction->tra_attachment;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE)
	{
		printf("VIO_garbage_collect (record_param %"QUADFORMAT"d, transaction %"
				  SLONGFORMAT")\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_TRACE_INFO)
	{
		printf("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	if (transaction->tra_attachment->att_flags & ATT_no_cleanup) {
		return true;
	}

	const SLONG oldest_snapshot = 
		rpb->rpb_relation->isTemporary() ? attachment->att_oldest_snapshot : transaction->tra_oldest_active;

	while (true)
	{
		if (rpb->rpb_flags & rpb_damaged)
		{
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			return false;
		}

		int state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);

		// Reset the garbage collect active flag if the transaction state is
		// in a terminal state. If committed it must have been a precommitted
		// transaction that was backing out a dead record version and the
		// system crashed. Clear the flag and set the state to tra_dead to
		// reattempt the backout.

		if (rpb->rpb_flags & rpb_gc_active)
		{
			switch (state)
			{
			case tra_committed:
				state = tra_dead;
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			case tra_dead:
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			default:
				break;
			}
		}

		if (state == tra_precommitted)
			state = check_precommitted(transaction, rpb);

		if (state == tra_dead)
		{
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			VIO_backout(tdbb, rpb, transaction);
		}
		else
		{
			if (rpb->rpb_flags & rpb_deleted)
			{
				if (rpb->rpb_transaction_nr >= oldest_snapshot)
				{
					return true;
				}

				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				expunge(tdbb, rpb, transaction, (SLONG) 0);
				return false;
			}

			if (rpb->rpb_transaction_nr >= oldest_snapshot || rpb->rpb_b_page == 0)
			{
				return true;
			}

			purge(tdbb, rpb);
		}

		if (!DPM_get(tdbb, rpb, LCK_read)) {
			return false;
		}
	}
}


Record* VIO_gc_record(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *	V I O _ g c _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Allocate from a relation's vector of garbage
 *	collect record blocks. Their scope is strictly
 *	limited to temporary usage and should never be
 *	copied to permanent record parameter blocks.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Allocate a vector of garbage collect record blocks for relation.
	vec<Record*>* vector = relation->rel_gc_rec;
	if (!vector) {
		vector = relation->rel_gc_rec = vec<Record*>::newVector(*dbb->dbb_permanent, 1);
	}

	// Set the active flag on an inactive garbage collect record block and return it.
	vec<Record*>::iterator rec_ptr = vector->begin();
	for (const vec<Record*>::const_iterator end = vector->end(); rec_ptr != end; ++rec_ptr)
	{
		Record* record = *rec_ptr;
		if (record && !(record->rec_flags & REC_gc_active)) {
			record->rec_flags |= REC_gc_active;
			return record;
		}
	}

	// Allocate a garbage collect record block if all are active.
	record_param rpb;
	rpb.rpb_record = 0;
	Record* record = VIO_record(tdbb, &rpb, MET_current(tdbb, relation), dbb->dbb_permanent);
	record->rec_flags |= REC_gc_active;

	// Insert the new record block into the last slot of the vector.

	size_t slot = vector->count() - 1;
	if ((*vector)[slot]) {
		vector->resize((++slot) + 1);
	}
	(*vector)[slot] = record;

	return record;
}


bool VIO_get(thread_db* tdbb, record_param* rpb, jrd_tra* transaction, MemoryPool* pool)
{
/**************************************
 *
 *	V I O _ g e t
 *
 **************************************
 *
 * Functional description
 *	Get a specific record from a relation.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS) {
		printf("VIO_get (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT", pool %p)\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0,
				  (void*) pool);
	}
#endif

	// Fetch data page from a modify/erase input stream with a write
	// lock. This saves an upward conversion to a write lock when
	// refetching the page in the context of the output stream.

	const USHORT lock_type = (rpb->rpb_stream_flags & RPB_s_update) ? LCK_write : LCK_read;

	if (!DPM_get(tdbb, rpb, lock_type) ||
		!VIO_chase_record_version(tdbb, rpb, transaction, pool, false))
	{
		return false;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif
	if (pool)
	{
		if (rpb->rpb_stream_flags & RPB_s_no_data)
		{
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			rpb->rpb_address = NULL;
			rpb->rpb_length = 0;
		}
		else
			VIO_data(tdbb, rpb, pool);
	}

	VIO_bump_count(tdbb, DBB_read_idx_count, rpb->rpb_relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_IDX_READS);

	return true;
}


bool VIO_get_current(thread_db* tdbb,
					record_param* rpb,
					jrd_tra* transaction,
					MemoryPool* pool,
					bool foreign_key,
					bool &has_old_values)
{
/**************************************
 *
 *	V I O _ g e t _ c u r r e n t
 *
 **************************************
 *
 * Functional description
 *	Get the current (most recent) version of a record.  This is
 *	called by IDX to determine whether a unique index has been
 *	duplicated.  If the target record's transaction is active,
 *	wait for it.  If the record is deleted or disappeared, return
 *	false.  If the record is committed, return true.
 *	If foreign_key is true, we are checking for a foreign key,
 *	looking to see if a primary key/unique key exists.  For a
 *	no wait transaction, if state of transaction inserting primary key
 *	record is tra_active, we should not see the uncommitted record
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE)
	{
		printf("VIO_get_current (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT", pool %p)\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0,
				  (void*) pool);
	}
#endif

	has_old_values = false;

	while (true)
	{
		// If the record doesn't exist, no problem.

		if (!DPM_get(tdbb, rpb, LCK_read)) {
			return false;
		}

#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_TRACE_INFO)
		{
			printf
				("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
				 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
				 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
				 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
				 rpb->rpb_f_page, rpb->rpb_f_line);
		}
#endif

		// Get data if there is data.

		if (rpb->rpb_flags & rpb_deleted) {
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		}
		else {
			VIO_data(tdbb, rpb, pool);
		}

		// If we deleted the record, everything's fine, otherwise
		// the record must be considered real.

		if (rpb->rpb_transaction_nr == transaction->tra_number) {
			break;
		}

		// check the state of transaction  - tra_us is taken care of above
		// For now check the state in the tip_cache or tip bitmap. If
		// record is committed (most cases), this will be faster.


		int state;
		if (transaction->tra_flags & TRA_read_committed) {
			state = TPC_cache_state(tdbb, rpb->rpb_transaction_nr);
		}
		else {
			state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);
		}

		// Reset the garbage collect active flag if the transaction state is
		// in a terminal state. If committed it must have been a precommitted
		// transaction that was backing out a dead record version and the
		// system crashed. Clear the flag and set the state to tra_dead to
		// reattempt the backout.

		if (rpb->rpb_flags & rpb_gc_active)
		{
			switch (state)
			{
			case tra_committed:
				state = tra_dead;
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			case tra_dead:
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			default:
				break;
			}
		}

		if (state == tra_precommitted)
			state = check_precommitted(transaction, rpb);

		switch (state)
		{
		case tra_committed:
			return !(rpb->rpb_flags & rpb_deleted);
		case tra_dead:
			if (transaction->tra_attachment->att_flags & ATT_no_cleanup)
				return !foreign_key;

			VIO_backout(tdbb, rpb, transaction);
			continue;
		case tra_precommitted:
			Database::Checkout dcoHolder(dbb);
			THREAD_SLEEP(100);	// milliseconds
			continue;
		}

		// The record belongs to somebody else.  Wait for him to commit, rollback, or die.

		const SLONG tid_fetch = rpb->rpb_transaction_nr;

		// Wait as long as it takes for an active transaction which has modified
		// the record. If an active transaction has used its TID to safely
		// backout a fragmented dead record version, spin wait because it will finish shortly.

		if (!(rpb->rpb_flags & rpb_gc_active))
		{
			state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_wait);
			if (state == tra_precommitted)
				state = check_precommitted(transaction, rpb);
		}
		else
		{
			state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_probe);

			if (state == tra_active) {
				Database::Checkout dcoHolder(dbb);
				THREAD_SLEEP(100);	// milliseconds
				continue;
			}
		}

		switch (state)
		{
		case tra_committed:
			// If the record doesn't exist anymore, no problem. This
			// can happen in two cases.  The transaction that inserted
			// the record deleted it or the transaction rolled back and
			// removed the records it modified and marked itself
			// committed


			if (!DPM_get(tdbb, rpb, LCK_read)) {
				return false;
			}

			// if the transaction actually rolled back and what
			// we are reading is another record (newly inserted),
			// loop back and try again.


			if (tid_fetch != rpb->rpb_transaction_nr) {
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				continue;
			}

			// Get latest data if there is data.

			if (rpb->rpb_flags & rpb_deleted) {
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				return false;
			}

			VIO_data(tdbb, rpb, pool);
			return true;

		case tra_active:
			// 1. if record just inserted
			//	  then FK can't reference it but PK must check it's new value
			// 2. if record just deleted
			//    then FK can't reference it but PK must check it's old value
			// 3. if record just modified
			//	  then FK can reference it if key field values are not changed

			if (!rpb->rpb_b_page)
				return !foreign_key;

			if (rpb->rpb_flags & rpb_deleted)
				return !foreign_key;

			if (foreign_key)
			{
				// clear lock error from status vector
				fb_utils::init_status(tdbb->tdbb_status_vector);
				return !(rpb->rpb_flags & rpb_uk_modified);
			}
			
			return true;

		case tra_dead:
			if (transaction->tra_attachment->att_flags & ATT_no_cleanup) {
				return !foreign_key;
			}

			VIO_backout(tdbb, rpb, transaction);
			break;

		default:
			BUGCHECK(184);		// limbo impossible
		}
	}

	return !(rpb->rpb_flags & rpb_deleted);
}


#ifdef GARBAGE_THREAD
void VIO_init(thread_db* tdbb)
{
/**************************************
 *
 *	V I O _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Activate the garbage collector thread.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

	if ((dbb->dbb_flags & DBB_read_only) || !(dbb->dbb_flags & DBB_gc_background))
	{
		return;
	}

	// If there's no presence of a garbage collector running then start one up.

	if (!(dbb->dbb_flags & DBB_garbage_collector))
	{
		if (gds__thread_start(garbage_collector, dbb, THREAD_medium, 0, 0))
		{
			ERR_bugcheck_msg("cannot start thread");
		}
		{ // scope
			Database::Checkout dcoHolder(dbb);
			dbb->dbb_gc_init.enter();
		}
	}

	// Database backups and sweeps perform their own garbage collection
	// unless passing a no garbage collect switch which means don't
	// notify the garbage collector to garbage collect. Every other
	// attachment notifies the garbage collector to do their dirty work.

	if (dbb->dbb_flags & DBB_garbage_collector &&
		!(attachment->att_flags & (ATT_no_cleanup | ATT_gbak_attachment)))
	{
		if (dbb->dbb_flags & DBB_suspend_bgio) {
			attachment->att_flags |= ATT_disable_notify_gc;
		}
		else {
			attachment->att_flags |= ATT_notify_gc;
		}
	}
}
#endif


void VIO_merge_proc_sav_points(thread_db* tdbb,
							   jrd_tra* transaction,
							   Savepoint** sav_point_list)
{
/**************************************
 *
 *	V I O _ m e r g e _ p r o c _ s a v _ p o i n t s
 *
 **************************************
 *
 * Functional description
 *	Merge all the work done in all the save points in
 *	sav_point_list to the current save point in the
 *	transaction block.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (transaction->tra_flags & TRA_system) {
		return;
	}
	if (!transaction->tra_save_point) {
		return;
	}

	// Merge all savepoints in the sav_point_list at the top
	// of transaction save points and call VIO_verb_cleanup()

	Savepoint* const org_save_point = transaction->tra_save_point;
	transaction->tra_save_point = *sav_point_list;

	for (Savepoint* sav_point = *sav_point_list; sav_point; sav_point = sav_point->sav_next)
	{
		Savepoint* const sav_next = sav_point->sav_next;
		const SLONG sav_number = sav_point->sav_number;

		if (!sav_point->sav_next)
		{
			sav_point->sav_next = org_save_point;
		}

		VIO_verb_cleanup(tdbb, transaction);

		if ( (sav_point = transaction->tra_save_free) ) {
			transaction->tra_save_free = sav_point->sav_next;
		}
		else {
			sav_point = FB_NEW(*transaction->tra_pool) Savepoint();
		}
		sav_point->sav_next = sav_next;
		sav_point->sav_number = sav_number;
		*sav_point_list = sav_point;
		sav_point_list = &sav_point->sav_next;
	}

	fb_assert(!transaction->tra_save_point || org_save_point == transaction->tra_save_point);
}


void VIO_modify(thread_db* tdbb, record_param* org_rpb, record_param* new_rpb,
				jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Modify an existing record.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("VIO_modify (org_rpb %"QUADFORMAT"d, new_rpb %"QUADFORMAT"d, "
				"transaction %"SLONGFORMAT")\n",
				  org_rpb->rpb_number.getValue(), new_rpb->rpb_number.getValue(),
				  transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   old record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 org_rpb->rpb_page, org_rpb->rpb_line, org_rpb->rpb_transaction_nr,
			 org_rpb->rpb_flags, org_rpb->rpb_b_page, org_rpb->rpb_b_line,
			 org_rpb->rpb_f_page, org_rpb->rpb_f_line);
	}
#endif

	jrd_rel* relation = org_rpb->rpb_relation;
	transaction->tra_flags |= TRA_write;
	new_rpb->rpb_transaction_nr = transaction->tra_number;
	new_rpb->rpb_flags = 0;
	new_rpb->getWindow(tdbb).win_flags = WIN_secondary;

	// If the stream was sorted, the various fields in the rpb are
	// probably junk.  Just to make sure that everything is cool,
	// refetch and release the record.

	if (org_rpb->rpb_stream_flags & RPB_s_refetch)
	{
		VIO_refetch_record(tdbb, org_rpb, transaction);
		org_rpb->rpb_stream_flags &= ~RPB_s_refetch;
	}

	// If we're the system transaction, modify stuff in place.  This saves
	// endless grief on cleanup

	VIO_bump_count(tdbb, DBB_update_count, relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_UPDATES);

	if (transaction->tra_flags & TRA_system) {
		update_in_place(tdbb, transaction, org_rpb, new_rpb);
		return;
	}

	// If we're about to modify a system relation, check to make sure
	// everything is completely kosher.

	DSC desc1, desc2;

	if (needDfw(tdbb, transaction))
	{
		switch ((RIDS) relation->rel_id)
		{
		case rel_database:
			check_class(tdbb, transaction, org_rpb, new_rpb, f_dat_class);
			break;

		case rel_relations:
			EVL_field(0, org_rpb->rpb_record, f_rel_name, &desc1);
			SCL_check_relation(tdbb, &desc1, SCL_protect);
			check_class(tdbb, transaction, org_rpb, new_rpb, f_rel_class);
			DFW_post_work(transaction, dfw_update_format, &desc1, 0);
			break;

		case rel_procedures:
			EVL_field(0, org_rpb->rpb_record, f_prc_name, &desc1);
			SCL_check_procedure(tdbb, &desc1, SCL_protect);
			check_class(tdbb, transaction, org_rpb, new_rpb, f_prc_class);
			if (dfw_should_know(org_rpb, new_rpb, f_prc_desc, true))
			{
				EVL_field(0, org_rpb->rpb_record, f_prc_id, &desc2);
				const USHORT id = MOV_get_long(&desc2, 0);
				DFW_post_work(transaction, dfw_modify_procedure, &desc1, id);
			}
			break;

		case rel_gens:
			{
				EVL_field(0, org_rpb->rpb_record, f_gen_name, &desc1);
				// We won't accept modifying sys generators and for user gens,
				// only the description.
				// This is poor man's version of a trigger discovering changed fields.
				bool important_change = dfw_should_know(org_rpb, new_rpb, f_gen_desc);
				DFW_post_work(transaction, dfw_modify_generator, &desc1, (USHORT) important_change);
			}
			break;

		case rel_rfr:
			check_rel_field_class(tdbb, org_rpb, SCL_control, transaction);
			check_rel_field_class(tdbb, new_rpb, SCL_control, transaction);
			check_class(tdbb, transaction, org_rpb, new_rpb, f_rfr_class);
			break;

		case rel_fields:
			check_control(tdbb);
			if (dfw_should_know(org_rpb, new_rpb, f_fld_desc, true))
			{
				EVL_field(0, org_rpb->rpb_record, f_fld_name, &desc1);
				MET_change_fields(tdbb, transaction, &desc1);
				EVL_field(0, new_rpb->rpb_record, f_fld_name, &desc2);
				DeferredWork* dw = MET_change_fields(tdbb, transaction, &desc2);
				if (dw)
				{
					// Did we convert computed field into physical, stored field?
					// If we did, then force the deletion of the dependencies.
					// Warning: getting the result of MET_change_fields is the last relation
					// that was affected, but for computed fields, it's an implicit domain
					// and hence it can be used only by a single field and therefore one relation.
					dsc desc3, desc4;
					bool rc1 = EVL_field(0, org_rpb->rpb_record, f_fld_computed, &desc3);
					bool rc2 = EVL_field(0, new_rpb->rpb_record, f_fld_computed, &desc4);
					if (rc1 != rc2 || rc1 && MOV_compare(&desc3, &desc4)) {
						DFW_post_work_arg(transaction, dw, &desc1, 0, dfw_arg_force_computed);
					}
				}

				dw = DFW_post_work(transaction, dfw_modify_field, &desc1, 0);
				DFW_post_work_arg(transaction, dw, &desc2, 0, dfw_arg_new_name);
			}
			break;

		case rel_classes:
			EVL_field(0, org_rpb->rpb_record, f_cls_class, &desc1);
			DFW_post_work(transaction, dfw_compute_security, &desc1, 0);
			EVL_field(0, new_rpb->rpb_record, f_cls_class, &desc1);
			DFW_post_work(transaction, dfw_compute_security, &desc1, 0);
			break;

		case rel_indices:
			EVL_field(0, new_rpb->rpb_record, f_idx_relation, &desc1);
			SCL_check_relation(tdbb, &desc1, SCL_control);
			if (dfw_should_know(org_rpb, new_rpb, f_idx_desc, true))
			{
				EVL_field(0, new_rpb->rpb_record, f_idx_name, &desc1);

				if (EVL_field(0, new_rpb->rpb_record, f_idx_exp_blr, &desc2))
				{
					DFW_post_work(transaction, dfw_create_expression_index,
								  &desc1, tdbb->getDatabase()->dbb_max_idx);
				}
				else {
					DFW_post_work(transaction, dfw_create_index, &desc1,
								  tdbb->getDatabase()->dbb_max_idx);
				}
			}
			break;

		case rel_triggers:
			EVL_field(0, new_rpb->rpb_record, f_trg_rname, &desc1);
			SCL_check_relation(tdbb, &desc1, SCL_control);

			if (dfw_should_know(org_rpb, new_rpb, f_trg_desc, true))
			{
				EVL_field(0, new_rpb->rpb_record, f_trg_rname, &desc1);
				DFW_post_work(transaction, dfw_update_format, &desc1, 0);
				EVL_field(0, org_rpb->rpb_record, f_trg_rname, &desc1);
				DFW_post_work(transaction, dfw_update_format, &desc1, 0);
				EVL_field(0, org_rpb->rpb_record, f_trg_name, &desc1);

				DeferredWork* dw = DFW_post_work(transaction, dfw_modify_trigger, &desc1, 0);

				if (EVL_field(0, new_rpb->rpb_record, f_trg_rname, &desc2))
					DFW_post_work_arg(transaction, dw, &desc2, 0, dfw_arg_rel_name);

				if (EVL_field(0, new_rpb->rpb_record, f_trg_type, &desc2))
				{
					DFW_post_work_arg(transaction, dw, &desc2,
						MOV_get_long(&desc2, 0), dfw_arg_trg_type);
				}
			}
			break;

		case rel_files:
			{
				SSHORT new_rel_flags, old_rel_flags;
				EVL_field(0, new_rpb->rpb_record, f_file_name, &desc1);
				if (EVL_field(0, new_rpb->rpb_record, f_file_flags, &desc2) &&
					((new_rel_flags = MOV_get_long(&desc2, 0)) & FILE_difference) &&
					EVL_field(0, org_rpb->rpb_record, f_file_flags, &desc2) &&
					((old_rel_flags = MOV_get_long(&desc2, 0)) != new_rel_flags))
				{
					DFW_post_work(transaction,
								  new_rel_flags & FILE_backing_up ? dfw_begin_backup : dfw_end_backup,
								  &desc1, 0);
				}
			}
			break;

		default:
			break;
		}
	}

	// We're about to modify the record. Post a refetch request
	// to all the active cursors positioned at this record.

	invalidate_cursor_records(transaction, org_rpb);

	/* We're almost ready to go.  To modify the record, we must first
	make a copy of the old record someplace else.  Then we must re-fetch
	the record (for write) and verify that it is legal for us to
	modify it -- that it was written by a transaction that was committed
	when we started.  If not, the transaction that wrote the record
	is either active, dead, or in limbo.  If the transaction is active,
	wait for it to finish.  If it commits, we can't procede and must
	return an update conflict.  If the transaction is dead, back out the
	old version of the record and try again.  If in limbo, punt.
	*/

	if (org_rpb->rpb_transaction_nr == transaction->tra_number &&
		org_rpb->rpb_format_number == new_rpb->rpb_format_number)
	{
		IDX_modify_flag_uk_modified(tdbb, org_rpb, new_rpb, transaction);
		update_in_place(tdbb, transaction, org_rpb, new_rpb);
		if (!(transaction->tra_flags & TRA_system) &&
			transaction->tra_save_point && transaction->tra_save_point->sav_verb_count)
		{
			verb_post(tdbb, transaction, org_rpb, org_rpb->rpb_undo, /*new_rpb,*/ false, false);
		}
		return;
	}

	record_param temp;
	PageStack stack;
	if (prepare_update(tdbb, transaction, org_rpb->rpb_transaction_nr, org_rpb, &temp, new_rpb,
					   stack, false))
	{
		ERR_post(Arg::Gds(isc_deadlock) <<
				 Arg::Gds(isc_update_conflict) <<
				 Arg::Gds(isc_concurrent_transaction) << Arg::Num(org_rpb->rpb_transaction_nr));
	}

	IDX_modify_flag_uk_modified(tdbb, org_rpb, new_rpb, transaction);

	// Old record was restored and re-fetched for write.  Now replace it.

	org_rpb->rpb_transaction_nr = new_rpb->rpb_transaction_nr;
	org_rpb->rpb_format_number = new_rpb->rpb_format_number;
	org_rpb->rpb_b_page = temp.rpb_page;
	org_rpb->rpb_b_line = temp.rpb_line;
	org_rpb->rpb_address = new_rpb->rpb_address;
	org_rpb->rpb_length = new_rpb->rpb_length;
	org_rpb->rpb_flags &= ~(rpb_delta | rpb_uk_modified);
	org_rpb->rpb_flags |= new_rpb->rpb_flags & (rpb_delta | rpb_uk_modified);

	replace_record(tdbb, org_rpb, &stack, transaction);

	if (!(transaction->tra_flags & TRA_system) &&
		transaction->tra_save_point && transaction->tra_save_point->sav_verb_count)
	{
		verb_post(tdbb, transaction, org_rpb, 0, /*0,*/ false, false);
	}

	// for an autocommit transaction, mark a commit as necessary

	if (transaction->tra_flags & TRA_autocommit) {
		transaction->tra_flags |= TRA_perform_autocommit;
	}

#ifdef GARBAGE_THREAD
	// VIO_modify
	if ((tdbb->getDatabase()->dbb_flags & DBB_gc_background) &&
		!org_rpb->rpb_relation->isTemporary())
	{
		notify_garbage_collector(tdbb, org_rpb, transaction->tra_number);
	}
#endif
}


bool VIO_next_record(thread_db* tdbb,
					 record_param* rpb,
					 //RecordSource* rsb,
					 jrd_tra* transaction,
					 MemoryPool* pool,
#ifdef SCROLLABLE_CURSORS
					 bool backwards,
#endif
					 bool onepage)
{
/**************************************
 *
 *	V I O _ n e x t _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Get the next record in a record stream.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Fetch data page from a modify/erase input stream with a write
	// lock. This saves an upward conversion to a write lock when
	// refetching the page in the context of the output stream.

	const USHORT lock_type = (rpb->rpb_stream_flags & RPB_s_update) ? LCK_write : LCK_read;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE)
	{
		printf("VIO_next_record (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT", pool %p)\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0,
				  (void*) pool);
	}
	if (debug_flag > DEBUG_TRACE_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	do {
		if (!DPM_next(tdbb, rpb, lock_type,
#ifdef SCROLLABLE_CURSORS
			backwards,
#endif
			onepage))
		{
			return false;
		}
	} while (!VIO_chase_record_version(tdbb, rpb, transaction, pool, false));

	if (pool)
	{
		if (rpb->rpb_stream_flags & RPB_s_no_data) {
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			rpb->rpb_address = NULL;
			rpb->rpb_length = 0;
		}
		else
			VIO_data(tdbb, rpb, pool);
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf
			("VIO_next_record got record  %"SLONGFORMAT":%d, rpb_trans %"
			 SLONGFORMAT", flags %d, back %"SLONGFORMAT":%d, fragment %"
			 SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	VIO_bump_count(tdbb, DBB_read_seq_count, rpb->rpb_relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_SEQ_READS);

	return true;
}


Record* VIO_record(thread_db* tdbb, record_param* rpb, const Format* format, MemoryPool* pool)
{
/**************************************
 *
 *	V I O _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Allocate a record block big enough for a given format.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE)
	{
		printf("VIO_record (record_param %"QUADFORMAT"d, format %d, pool %p)\n",
				  rpb->rpb_number.getValue(), format ? format->fmt_version : 0,
				  (void*) pool);
	}
#endif

	// If format wasn't given, look one up

	if (!format)
		format = MET_format(tdbb, rpb->rpb_relation, rpb->rpb_format_number);

	Record* record = rpb->rpb_record;
	if (!record)
	{
		if (!pool)
			pool = dbb->dbb_permanent;

		record = rpb->rpb_record = FB_NEW_RPT(*pool, format->fmt_length) Record(*pool);
	}
	else if (record->rec_length < format->fmt_length)
	{
		Record* const old = record;
		if (record->rec_flags & REC_gc_active)
			record = replace_gc_record(rpb->rpb_relation, &rpb->rpb_record, format->fmt_length);
		else
			record = realloc_record(rpb->rpb_record, format->fmt_length);

		if (rpb->rpb_prior == old)
			rpb->rpb_prior = record;
	}

	record->rec_format = format;
	record->rec_length = format->fmt_length;

	return record;
}


void VIO_refetch_record(thread_db* tdbb, record_param* rpb,
						jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ r e f e t c h _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Refetch & release the record, if we unsure,
 *  whether information about it is still valid.
 *
 **************************************/
	const SLONG tid_fetch = rpb->rpb_transaction_nr;

	if (!DPM_get(tdbb, rpb, LCK_read) ||
		!VIO_chase_record_version(tdbb, rpb, transaction, tdbb->getDefaultPool(), false))
	{
		ERR_post(Arg::Gds(isc_no_cur_rec));
	}

	VIO_data(tdbb, rpb, tdbb->getRequest()->req_pool);

	// If record is present, and the transaction is read committed,
	// make sure the record has not been updated.  Also, punt after
	// VIO_data() call which will release the page.

	if ((transaction->tra_flags & TRA_read_committed) && (tid_fetch != rpb->rpb_transaction_nr) &&
		// added to check that it was not current transaction,
		// who modified the record. Alex P, 18-Jun-03
		(rpb->rpb_transaction_nr != transaction->tra_number))
	{
		ERR_post(Arg::Gds(isc_deadlock) <<
				 Arg::Gds(isc_update_conflict) <<
				 Arg::Gds(isc_concurrent_transaction) << Arg::Num(rpb->rpb_transaction_nr));
	}
}


void VIO_start_save_point(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *      V I O _ s t a r t _ s a v e _ p o i n t
 *
 **************************************
 *
 * Functional description
 *      Start a new save point for a transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Savepoint* sav_point = transaction->tra_save_free;

	if (sav_point) {
		transaction->tra_save_free = sav_point->sav_next;
	}
	else {
		sav_point = FB_NEW(*transaction->tra_pool) Savepoint();
	}

	sav_point->sav_number = ++transaction->tra_save_point_number;
	sav_point->sav_next = transaction->tra_save_point;
	transaction->tra_save_point = sav_point;
}


void VIO_store(thread_db* tdbb, record_param* rpb, jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Store a new record.
 *
 **************************************/
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	DeferredWork* work = NULL;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("VIO_store (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT
				  ")\n", rpb->rpb_number.getValue(),
				  transaction ? transaction->tra_number : 0);
	}
#endif

	transaction->tra_flags |= TRA_write;
	jrd_rel* relation = rpb->rpb_relation;
	DSC desc, desc2;

	if (needDfw(tdbb, transaction))
	{
		switch ((RIDS) relation->rel_id)
		{
		case rel_relations:
			EVL_field(0, rpb->rpb_record, f_rel_name, &desc);
			DFW_post_work(transaction, dfw_create_relation, &desc, 0);
			DFW_post_work(transaction, dfw_update_format, &desc, 0);
			set_system_flag(tdbb, rpb, f_rel_sys_flag, 0);
			break;

		case rel_procedures:
			EVL_field(0, rpb->rpb_record, f_prc_name, &desc);
			EVL_field(0, rpb->rpb_record, f_prc_id, &desc2);
			{ // scope
				const USHORT id = MOV_get_long(&desc2, 0);
				work = DFW_post_work(transaction, dfw_create_procedure, &desc, id);

				bool check_blr = true;

				if (ENCODE_ODS(tdbb->getDatabase()->dbb_ods_version,
					tdbb->getDatabase()->dbb_minor_original) >= ODS_11_1)
				{
					if (EVL_field(0, rpb->rpb_record, f_prc_valid_blr, &desc2))
						check_blr = MOV_get_long(&desc2, 0) != 0;
				}

				if (check_blr)
					DFW_post_work_arg(transaction, work, NULL, 0, dfw_arg_check_blr);
			} // scope
			set_system_flag(tdbb, rpb, f_prc_sys_flag, 0);
			break;

		case rel_indices:
			EVL_field(0, rpb->rpb_record, f_idx_relation, &desc);
			SCL_check_relation(tdbb, &desc, SCL_control);
			EVL_field(0, rpb->rpb_record, f_idx_name, &desc);
			if (EVL_field(0, rpb->rpb_record, f_idx_exp_blr, &desc2))
			{
				DFW_post_work(transaction, dfw_create_expression_index, &desc,
							  tdbb->getDatabase()->dbb_max_idx);
			}
			else {
				DFW_post_work(transaction, dfw_create_index, &desc, tdbb->getDatabase()->dbb_max_idx);
			}
			break;

		case rel_rfr:
			EVL_field(0, rpb->rpb_record, f_rfr_rname, &desc);
			SCL_check_relation(tdbb, &desc, SCL_control);
			DFW_post_work(transaction, dfw_update_format, &desc, 0);
			set_system_flag(tdbb, rpb, f_rfr_sys_flag, 0);
			break;

		case rel_classes:
			EVL_field(0, rpb->rpb_record, f_cls_class, &desc);
			DFW_post_work(transaction, dfw_compute_security, &desc, 0);
			break;

		case rel_fields:
			check_control(tdbb);
			EVL_field(0, rpb->rpb_record, f_fld_name, &desc);
			DFW_post_work(transaction, dfw_create_field, &desc, 0);
			set_system_flag(tdbb, rpb, f_fld_sys_flag, 0);
			break;

		case rel_files:
			{
				const bool name_defined = EVL_field(0, rpb->rpb_record, f_file_name, &desc);
				if (EVL_field(0, rpb->rpb_record, f_file_shad_num, &desc2) &&
					MOV_get_long(&desc2, 0))
				{
					EVL_field(0, rpb->rpb_record, f_file_flags, &desc2);
					if (!(MOV_get_long(&desc2, 0) & FILE_inactive)) {
						DFW_post_work(transaction, dfw_add_shadow, &desc, 0);
					}
				}
				else
				{
					USHORT rel_flags;
					if (EVL_field(0, rpb->rpb_record, f_file_flags, &desc2) &&
						((rel_flags = MOV_get_long(&desc2, 0)) & FILE_difference))
					{
						if (name_defined) {
							DFW_post_work(transaction, dfw_add_difference, &desc, 0);
						}
						if (rel_flags & FILE_backing_up)
						{
							DFW_post_work(transaction, dfw_begin_backup, &desc, 0);
						}
					}
					else {
						DFW_post_work(transaction, dfw_add_file, &desc, 0);
					}
				}
			}
			break;

		case rel_triggers:
			EVL_field(0, rpb->rpb_record, f_trg_rname, &desc);

			// check if this  request go through without checking permissions
			if (!(request->req_flags & req_ignore_perm)) {
				SCL_check_relation(tdbb, &desc, SCL_control);
			}

			if (EVL_field(0, rpb->rpb_record, f_trg_rname, &desc2))
				DFW_post_work(transaction, dfw_update_format, &desc2, 0);

			EVL_field(0, rpb->rpb_record, f_trg_name, &desc);
			work = DFW_post_work(transaction, dfw_create_trigger, &desc, 0);

			if (!(desc2.dsc_flags & DSC_null))
				DFW_post_work_arg(transaction, work, &desc2, 0, dfw_arg_rel_name);

			if (EVL_field(0, rpb->rpb_record, f_trg_type, &desc2))
			{
				DFW_post_work_arg(transaction, work, &desc2,
					MOV_get_long(&desc2, 0), dfw_arg_trg_type);
			}

			break;

		case rel_priv:
			EVL_field(0, rpb->rpb_record, f_prv_rname, &desc);
			EVL_field(0, rpb->rpb_record, f_prv_o_type, &desc2);
			{ // scope
				const USHORT id = MOV_get_long(&desc2, 0);
				DFW_post_work(transaction, dfw_grant, &desc, id);
			} // scope
			break;

		default:    // Shut up compiler warnings
			break;
		}
	}

	// this should be scheduled even in database creation (system transaction)
	switch ((RIDS) relation->rel_id)
	{
		case rel_collations:
			{
				EVL_field(0, rpb->rpb_record, f_coll_cs_id, &desc);
				USHORT id = MOV_get_long(&desc, 0);

				EVL_field(0, rpb->rpb_record, f_coll_id, &desc);
				id = INTL_CS_COLL_TO_TTYPE(id, MOV_get_long(&desc, 0));

				EVL_field(0, rpb->rpb_record, f_coll_name, &desc);
				DFW_post_work(transaction, dfw_create_collation, &desc, id);
			}
			break;

		default:	// Shut up compiler warnings
			break;
	}

	rpb->rpb_b_page = 0;
	rpb->rpb_b_line = 0;
	rpb->rpb_flags = 0;
	rpb->rpb_transaction_nr = transaction->tra_number;
	rpb->getWindow(tdbb).win_flags = 0;
	rpb->rpb_record->rec_precedence.push(-rpb->rpb_transaction_nr);
	DPM_store(tdbb, rpb, rpb->rpb_record->rec_precedence, DPM_primary);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO) {
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	VIO_bump_count(tdbb, DBB_insert_count, relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_INSERTS);

	if (!(transaction->tra_flags & TRA_system) &&
		transaction->tra_save_point && transaction->tra_save_point->sav_verb_count)
	{
		verb_post(tdbb, transaction, rpb, 0, /*0,*/ false, false);
	}

	// for an autocommit transaction, mark a commit as necessary

	if (transaction->tra_flags & TRA_autocommit) {
		transaction->tra_flags |= TRA_perform_autocommit;
	}
}


bool VIO_sweep(thread_db* tdbb, jrd_tra* transaction, TraceSweepEvent* traceSweep)
{
/**************************************
 *
 *	V I O _ s w e e p
 *
 **************************************
 *
 * Functional description
 *	Make a garbage collection pass.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE) {
		printf("VIO_sweep (transaction %"SLONGFORMAT")\n", transaction ? transaction->tra_number : 0);
	}
#endif

	if (transaction->tra_attachment->att_flags & ATT_NO_CLEANUP) {
		return false;
	}

	DPM_scan_pages(tdbb);

	// hvlad: restore tdbb->getTransaction() since it can be used later
	tdbb->setTransaction(transaction);

	record_param rpb;
	rpb.rpb_record = NULL;
	rpb.rpb_stream_flags = 0;
	rpb.getWindow(tdbb).win_flags = WIN_large_scan;

	jrd_rel* relation = 0; // wasn't initialized: memory problem in catch() part.
	vec<jrd_rel*>* vector = 0;

	try {

		for (size_t i = 1; (vector = dbb->dbb_relations) && i < vector->count(); i++)
		{
			if ((relation = (*vector)[i]) && !(relation->rel_flags & (REL_deleted | REL_deleting)) &&
				 relation->getPages(tdbb)->rel_pages)
			{
				rpb.rpb_relation = relation;
				rpb.rpb_number.setValue(BOF_NUMBER);
				rpb.rpb_org_scans = relation->rel_scan_count++;
				++relation->rel_sweep_count;

				traceSweep->beginSweepRelation(relation);

#ifdef GARBAGE_THREAD
				if (relation->rel_garbage) {
					relation->rel_garbage->clear();
				}
#endif
				while (VIO_next_record(tdbb, &rpb, /*NULL,*/ transaction, 0,
#ifdef SCROLLABLE_CURSORS
					false,
#endif
					false))
				{
					CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
					if (relation->rel_flags & REL_deleting) {
						break;
					}
					if (--tdbb->tdbb_quantum < 0) {
						JRD_reschedule(tdbb, SWEEP_QUANTUM, true);
					}
#ifdef SUPERSERVER
					transaction->tra_oldest_active = dbb->dbb_oldest_snapshot;
#endif
				}

				traceSweep->endSweepRelation(relation);

				--relation->rel_sweep_count;
				--relation->rel_scan_count;
			}
		}

		delete rpb.rpb_record;

	}	// try
	catch (const Firebird::Exception&)
	{
		delete rpb.rpb_record;
		if (relation)
		{
			if (relation->rel_sweep_count) {
				--relation->rel_sweep_count;
			}
			if (relation->rel_scan_count) {
				--relation->rel_scan_count;
			}
		}
		ERR_punt();
	}
	return true;
}


void VIO_temp_cleanup(thread_db* tdbb, jrd_tra* transaction)
/**************************************
 *
 *	V I O _ t e m p _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *  Remove undo data for GTT ON COMMIT DELETE ROWS as their data will be released
 *  at transaction end anyway and we don't need to waste time backing it out on 
 *  rollback
 *
 **************************************/
{
	Savepoint* sav_point = transaction->tra_save_point;
	for (; sav_point; sav_point = sav_point->sav_next)
	{
		for (VerbAction* action = sav_point->sav_verb_actions; action; action = action->vct_next)
		{
			if (action->vct_relation->rel_flags & REL_temp_tran)
			{
				RecordBitmap::reset(action->vct_records);
				if (action->vct_undo)
				{
					if (action->vct_undo->getFirst())
					{
						do {
							action->vct_undo->current().release(transaction);
						} while (action->vct_undo->getNext());
					}
					delete action->vct_undo;
					action->vct_undo = NULL;
				}
			}
		}
	}
}


void VIO_verb_cleanup(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ v e r b _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Cleanup after a verb.  If the verb count in the transaction block
 *	is non zero, the verb failed and should be cleaned up.  Cleaning
 *	up ordinarily means just backing out the change, but if we have
 *	an old version we created in this transaction, we replace the current
 *	version with old version.
 *
 *	All changes made by the transaction are kept in one bitmap per
 *	relation.  A second bitmap per relation tracks records for which
 *	we have old data.  The actual data is kept in a linked list stack.
 *	Note that although a record may be changed several times, it will
 *	have only ONE old value -- the value it had before this verb
 *	started.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE)
	{
		printf("VIO_verb_cleanup (transaction %"SLONGFORMAT")\n",
				  transaction ? transaction->tra_number : 0);
	}
#endif
	if (transaction->tra_flags & TRA_system) {
		return;
	}

	Savepoint* sav_point = transaction->tra_save_point;
	if (!sav_point)
	{
		return;
	}

	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

// If the current to-be-cleaned-up savepoint is very big, and the next
// level savepoint is the transaction level savepoint, then get rid of
// the transaction level savepoint now (instead of after making the
// transaction level savepoint very very big).

	transaction->tra_save_point = sav_point->sav_next;
	if (transaction->tra_save_point &&
		(transaction->tra_save_point->sav_flags & SAV_trans_level) &&
		VIO_savepoint_large(sav_point, SAV_LARGE) < 0)
	{
		VIO_verb_cleanup(tdbb, transaction);	// get rid of tx-level savepoint
	}

	// Cleanup/merge deferred work/event post

	if (sav_point->sav_verb_actions || sav_point->sav_verb_count || (sav_point->sav_flags & SAV_force_dfw))
	{
		if (sav_point->sav_verb_count) {
			DFW_delete_deferred(transaction, sav_point->sav_number);
		}
		else {
			DFW_merge_work(transaction, sav_point->sav_number,
						   (transaction->tra_save_point ? transaction->tra_save_point->sav_number : 0));
		}

		// The save point may be reused, so reset it.  If the work was
		// not rolled back, set flag for the previous save point.


		if (sav_point->sav_flags & SAV_force_dfw)
		{
			if (transaction->tra_save_point && !sav_point->sav_verb_count)
			{
				transaction->tra_save_point->sav_flags |= SAV_force_dfw;
			}

			sav_point->sav_flags &= ~SAV_force_dfw;
		}
	}

	record_param rpb;
	VerbAction* action;
	jrd_tra* old_tran = tdbb->getTransaction();
	try {
		tdbb->tdbb_flags |= TDBB_verb_cleanup;
		tdbb->setTransaction(transaction);

		while ( (action = sav_point->sav_verb_actions) )
		{
			sav_point->sav_verb_actions = action->vct_next;
			jrd_rel* relation = action->vct_relation;
			if (sav_point->sav_verb_count || transaction->tra_save_point)
			{
				rpb.rpb_relation = relation;
				rpb.rpb_number.setValue(BOF_NUMBER);
				rpb.rpb_record = NULL;
				rpb.getWindow(tdbb).win_flags = 0;
				rpb.rpb_transaction_nr = transaction->tra_number;

				if (sav_point->sav_verb_count)
				{
					// This savepoint needs to be undone because the
					// verb_count is not zero.

					RecordBitmap::Accessor accessor(action->vct_records);
					if (accessor.getFirst())
						do {
							rpb.rpb_number.setValue(accessor.current());
							if (!DPM_get(tdbb, &rpb, LCK_write)) {
								BUGCHECK(186);	// msg 186 record disappeared
							}
							if (rpb.rpb_flags & rpb_delta) {
								VIO_data(tdbb, &rpb, tdbb->getDefaultPool());
							}
							else {
								CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
							}
							if (rpb.rpb_transaction_nr != transaction->tra_number) {
								BUGCHECK(185);	// msg 185 wrong record version
							}
							if (!action->vct_undo ||
								!action->vct_undo->locate(Firebird::locEqual, rpb.rpb_number.getValue()))
							{
								VIO_backout(tdbb, &rpb, transaction);
							}
							else
							{
								Record* const record = action->vct_undo->current().setupRecord(transaction);
								const bool same_tx = (record->rec_flags & REC_same_tx) != 0;

								// Have we done BOTH an update and delete to this record
								// in the same transaction?

								if (same_tx)
								{
									VIO_backout(tdbb, &rpb, transaction);
									/* Nickolay Samofatov, 01 Mar 2003:
									If we don't have data for the record and
									it was modified and deleted under our savepoint
									we need to back it out to the state as it were
									before our transaction started */
									if (record->rec_length == 0 && record->rec_flags & REC_new_version)
									{
										if (!DPM_get(tdbb, &rpb, LCK_write)) {
											BUGCHECK(186);	// msg 186 record disappeared
										}
										if (rpb.rpb_flags & rpb_delta) {
											VIO_data(tdbb, &rpb, tdbb->getDefaultPool());
										}
										else {
											CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
										}

										VIO_backout(tdbb, &rpb, transaction);
									}
								}
								if (record->rec_length != 0)
								{
									Record* dead_record = rpb.rpb_record;
									record_param new_rpb = rpb;
									new_rpb.rpb_record = record;
									new_rpb.rpb_address = record->rec_data;
									new_rpb.rpb_length = record->rec_length;
									if (!(rpb.rpb_flags & rpb_delta))
									{
										if (!DPM_get(tdbb, &rpb, LCK_write)) {
											BUGCHECK(186);	// msg 186 record disappeared
										}
										VIO_data(tdbb, &rpb, tdbb->getDefaultPool());
									}
									update_in_place(tdbb, transaction, &rpb, &new_rpb);
									if (!(transaction->tra_flags & TRA_system)) {
										garbage_collect_idx(tdbb, &rpb, /*&new_rpb,*/ NULL, NULL);
									}
									rpb.rpb_record = dead_record;
								}
							}
						} while (accessor.getNext());
				}
				else
				{
					// This savepoint needs to be posted to the previous savepoint.

					RecordBitmap::Accessor accessor(action->vct_records);
					if (accessor.getFirst())
						do {
							rpb.rpb_number.setValue(accessor.current());
							if (!action->vct_undo ||
								!action->vct_undo->locate(Firebird::locEqual, rpb.rpb_number.getValue()))
							{
								verb_post(tdbb, transaction, &rpb, 0, /*0,*/ false, false);
							}
							else
							{
								// Setup more of rpb because verb_post is probably going to
								// garbage-collect.  Note that the data doesn't need to be set up
								// because old_data will be used.  (this guarantees that the
								// rpb points to the first fragment of the record)

								if (!DPM_get(tdbb, &rpb, LCK_read)) {
									BUGCHECK(186);	// msg 186 record disappeared
								}
								CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
								Record* const record = action->vct_undo->current().setupRecord(transaction);
								const bool same_tx = (record->rec_flags & REC_same_tx) != 0;
								const bool new_ver = (record->rec_flags & REC_new_version) != 0;
								if (record->rec_length != 0)
								{
									record_param new_rpb = rpb;
									new_rpb.rpb_record = record;
									new_rpb.rpb_address = record->rec_data;
									new_rpb.rpb_length = record->rec_length;
									verb_post(tdbb, transaction, &rpb, record, /*&new_rpb,*/ same_tx, new_ver);
								}
								else if (same_tx) {
									verb_post(tdbb, transaction, &rpb, 0, /*0,*/ true, new_ver);
								}
							}
						} while (accessor.getNext());
				}

				delete rpb.rpb_record;
			}
			RecordBitmap::reset(action->vct_records);
			if (action->vct_undo)
			{
				if (action->vct_undo->getFirst())
				{
					do {
						action->vct_undo->current().release(transaction);
					} while (action->vct_undo->getNext());
				}
				delete action->vct_undo;
				action->vct_undo = NULL;
			}
			action->vct_next = sav_point->sav_verb_free;
			sav_point->sav_verb_free = action;
		}
		tdbb->setTransaction(old_tran);
		tdbb->tdbb_flags &= ~TDBB_verb_cleanup;
	}
	catch (...)
	{
		tdbb->setTransaction(old_tran);
		tdbb->tdbb_flags &= ~TDBB_verb_cleanup;
		throw;
	}

	sav_point->sav_verb_count = 0;
	sav_point->sav_flags = 0;
	sav_point->sav_next = transaction->tra_save_free;
	transaction->tra_save_free = sav_point;

	// If the only remaining savepoint is the 'transaction-level' savepoint
	// that was started by TRA_start, then check if it hasn't grown out of
	// bounds yet.  If it has, then give up on this transaction-level savepoint.

	if (transaction->tra_save_point &&
		(transaction->tra_save_point->sav_flags & SAV_trans_level) &&
		VIO_savepoint_large(transaction->tra_save_point, SAV_LARGE) < 0)
	{
		VIO_verb_cleanup(tdbb, transaction);	// get rid of savepoint
	}
}


bool VIO_writelock(thread_db* tdbb, record_param* org_rpb, RecordSource* rsb, jrd_tra* transaction)
{
/**************************************
 *
 *	V I O _ w r i t e l o c k
 *
 **************************************
 *
 * Functional description
 *	Modify record to make record owned by this transaction
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("VIO_writelock (org_rpb %"QUADFORMAT"d, transaction %"
				  SLONGFORMAT")\n",
				  org_rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   old record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 org_rpb->rpb_page, org_rpb->rpb_line, org_rpb->rpb_transaction_nr,
			 org_rpb->rpb_flags, org_rpb->rpb_b_page, org_rpb->rpb_b_line,
			 org_rpb->rpb_f_page, org_rpb->rpb_f_line);
	}
#endif

	if (transaction->tra_flags & TRA_system)
	{
		// Explicit locks are not needed in system transactions
		return true;
	}

	transaction->tra_flags |= TRA_write;

	Record* org_record = org_rpb->rpb_record;
	if (!org_record)
	{
		org_record = VIO_record(tdbb, org_rpb, NULL, tdbb->getDefaultPool());
		org_rpb->rpb_address = org_record->rec_data;
		org_rpb->rpb_length = org_record->rec_format->fmt_length;
		org_rpb->rpb_format_number = org_record->rec_format->fmt_version;
	}

	record_param temp;
	// Repeat as many times as underlying record modifies
	while (true)
	{
		// Refetch and release the record if it is needed
		if (org_rpb->rpb_stream_flags & RPB_s_refetch)
		{
			// const SLONG tid_fetch = org_rpb->rpb_transaction_nr;
			if ((!DPM_get(tdbb, org_rpb, LCK_read)) ||
				(!VIO_chase_record_version(tdbb, org_rpb, transaction, tdbb->getDefaultPool(), true)))
			{
				return false;
			}
			VIO_data(tdbb, org_rpb, tdbb->getRequest()->req_pool);

			org_rpb->rpb_stream_flags &= ~RPB_s_refetch;

			// Make sure refetched record still fulfills search condition
			RecordSource* r = rsb;
			while (r && r->rsb_type != rsb_boolean)
				r = r->rsb_next;
			if (r && !EVL_boolean(tdbb, (jrd_nod*) r->rsb_arg[0]))
				return false;
		}

		if (org_rpb->rpb_transaction_nr == transaction->tra_number) {
			// We already own this record. No writelock required
			return true;
		}

		PageStack stack;
		switch (prepare_update(tdbb, transaction, org_rpb->rpb_transaction_nr, org_rpb, &temp, 0,
					stack, true))
		{
			case PREPARE_CONFLICT:
				org_rpb->rpb_stream_flags |= RPB_s_refetch;
				continue;
			case PREPARE_LOCKERR:
				// We got some kind of locking error (deadlock, timeout or lock_conflict)
				// Error details should be stuffed into status vector at this point
				ERR_punt();
			case PREPARE_DELETE:
				return false;
		}

		// The record could be reallocated in the meantime. Reassign the pointer.

		org_record = org_rpb->rpb_record;

		// Old record was restored and re-fetched for write.  Now replace it.

		org_rpb->rpb_transaction_nr = transaction->tra_number;
		org_rpb->rpb_format_number = org_record->rec_format->fmt_version;
		org_rpb->rpb_b_page = temp.rpb_page;
		org_rpb->rpb_b_line = temp.rpb_line;
		org_rpb->rpb_address = org_record->rec_data;
		org_rpb->rpb_length = org_record->rec_format->fmt_length;
		org_rpb->rpb_flags |= rpb_delta;

		replace_record(tdbb, org_rpb, &stack, transaction);

		if (!(transaction->tra_flags & TRA_system) && transaction->tra_save_point)
		{
			verb_post(tdbb, transaction, org_rpb, 0, /*0,*/ false, false);
		}

		// for an autocommit transaction, mark a commit as necessary

		if (transaction->tra_flags & TRA_autocommit) {
			transaction->tra_flags |= TRA_perform_autocommit;
		}

		return true;
	}
}


static int check_precommitted(const jrd_tra* transaction, const record_param* rpb)
{
/*********************************************
 *
 *	c h e c k _ p r e c o m m i t t e d
 *
 *********************************************
 *
 * Functional description
 *	Check if precommitted transaction which created given record version is
 *  current transaction or it is a still active and belongs to the current 
 *	attachment. This is needed to detect visibility of records modified in
 *	temporary tables in read-only transactions.
 *
 **************************************/
	if (!(rpb->rpb_flags & rpb_gc_active) && rpb->rpb_relation->isTemporary())
	{
		if (transaction->tra_number == rpb->rpb_transaction_nr) {
			return tra_us;
		}


		const jrd_tra* tx = transaction->tra_attachment->att_transactions;
		for (; tx; tx = tx->tra_next)
			if (tx->tra_number == rpb->rpb_transaction_nr)
			{
				return tra_active;
			}
	}
	return tra_precommitted;
}


static void check_rel_field_class(thread_db* tdbb,
								  record_param* rpb,
								  SecurityClass::flags_t flags,
								  jrd_tra* transaction)
{
/*********************************************
 *
 *	c h e c k _ r e l _ f i e l d _ c l a s s
 *
 *********************************************
 *
 * Functional description
 *	Given rpb for a record in the nam_r_fields system relation,
 *  containing a security class, checks does that record itself or
 *	relation, whom it belongs, are OK for given flags.
 *
 **************************************/
	SET_TDBB(tdbb);

	bool okField = true;
	DSC desc;
	if (EVL_field(0, rpb->rpb_record, f_rfr_class, &desc))
	{
		const Firebird::MetaName class_name(reinterpret_cast<TEXT*>(desc.dsc_address),
											desc.dsc_length);
		const SecurityClass* s_class = SCL_get_class(tdbb, class_name.c_str());
		if (s_class)
		{
			// In case when user has no access to the field,
			// he may have access to relation as whole.
			try
			{
				SCL_check_access(tdbb, s_class, 0, NULL, NULL, flags, "COLUMN", "");
			}
			catch (const Firebird::Exception&)
			{
				fb_utils::init_status(tdbb->tdbb_status_vector);
				okField = false;
			}
		}
	}

	EVL_field(0, rpb->rpb_record, f_rfr_rname, &desc);
	if (! okField)
	{
		SCL_check_relation(tdbb, &desc, flags);
	}
	DFW_post_work(transaction, dfw_update_format, &desc, 0);
}

static void check_class(thread_db* tdbb,
						jrd_tra* transaction,
						record_param* old_rpb, record_param* new_rpb, USHORT id)
{
/**************************************
 *
 *	c h e c k _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	A record in a system relation containing a security class is
 *	being changed.  Check to see if the security class has changed,
 *	and if so, post the change.
 *
 **************************************/
	SET_TDBB(tdbb);

	DSC desc1, desc2;
	EVL_field(0, old_rpb->rpb_record, id, &desc1);
	EVL_field(0, new_rpb->rpb_record, id, &desc2);
	if (!MOV_compare(&desc1, &desc2))
		return;

	Attachment* attachment = tdbb->getAttachment();

	SCL_check_access(tdbb, attachment->att_security_class, 0, NULL, NULL, SCL_protect,
					 object_database, "");
	DFW_post_work(transaction, dfw_compute_security, &desc2, 0);
}


static void check_control(thread_db* tdbb)
{
/**************************************
 *
 *	c h e c k _ c o n t r o l
 *
 **************************************
 *
 * Functional description
 *	Check to see if we have control
 *	privilege on the current database.
 *
 **************************************/
	SET_TDBB(tdbb);

	Attachment* attachment = tdbb->getAttachment();

	SCL_check_access(tdbb, attachment->att_security_class, 0, NULL, NULL, SCL_control,
					 object_database, "");
}


static bool check_user(thread_db* tdbb, const dsc* desc)
{
/**************************************
 *
 *	c h e c k _ u s e r
 *
 **************************************
 *
 * Functional description
 *	Validate string against current user name.
 *
 **************************************/
	SET_TDBB(tdbb);

	const TEXT* p = (TEXT *) desc->dsc_address;
	const TEXT* const end = p + desc->dsc_length;
	const TEXT* q = tdbb->getAttachment()->att_user->usr_user_name.c_str();

	// It is OK to not internationalize this function for v4.00 as
	// User names are limited to 7-bit ASCII for v4.00

	for (; p < end && *p != ' '; p++, q++)
	{
		if (UPPER7(*p) != UPPER7(*q)) {
			return false;
		}
	}

	return *q ? false : true;
}


static void delete_record(thread_db* tdbb, record_param* rpb, SLONG prior_page, MemoryPool* pool)
{
/**************************************
 *
 *	d e l e t e
 *
 **************************************
 *
 * Functional description
 *	Delete a record an all of its fragments.  This assumes the
 *	record has already been fetched for write.  If a pool is given,
 *	the caller has requested that data be fetched as the record is
 *	deleted.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("delete_record (record_param %"QUADFORMAT"d, prior_page %"SLONGFORMAT", pool %p)\n",
				  rpb->rpb_number.getValue(), prior_page, (void*) pool);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   delete_record record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif
	UCHAR* tail;
	const UCHAR* tail_end;
	UCHAR differences[MAX_DIFFERENCES];
	Record* record = 0;
	const Record* prior = 0;
	if (!pool || (rpb->rpb_flags & rpb_deleted))
	{
		prior = NULL;
		tail_end = tail = NULL;
	}
	else
	{
		record = VIO_record(tdbb, rpb, 0, pool);
		prior = rpb->rpb_prior;
		if (prior)
		{
			tail = differences;
			tail_end = differences + sizeof(differences);
			if (prior != record)
			{
				if (record->rec_length < prior->rec_length)
				{
					if (record->rec_flags & REC_gc_active)
					{
						record = replace_gc_record(rpb->rpb_relation, &rpb->rpb_record,
												   prior->rec_length);
					}
					else {
						record = realloc_record(rpb->rpb_record, prior->rec_length);
					}
				}
				memcpy(record->rec_data, prior->rec_data, prior->rec_format->fmt_length);
			}
		}
		else
		{
			tail = record->rec_data;
			tail_end = tail + record->rec_length;
		}
		tail = SQZ_decompress(rpb->rpb_address, rpb->rpb_length, tail, tail_end);
		rpb->rpb_prior = (rpb->rpb_flags & rpb_delta) ? record : 0;
	}

	record_param temp_rpb = *rpb;
	DPM_delete(tdbb, &temp_rpb, prior_page);
	tail = delete_tail(tdbb, &temp_rpb, temp_rpb.rpb_page, tail, tail_end);

	if (pool && prior)
	{
		SQZ_apply_differences(record, reinterpret_cast<const char*>(differences),
							  reinterpret_cast<const char*>(tail));
	}
}


static UCHAR* delete_tail(thread_db* tdbb,
						  record_param* rpb,
						  SLONG prior_page, UCHAR* tail, const UCHAR* tail_end)
{
/**************************************
 *
 *	d e l e t e _ t a i l
 *
 **************************************
 *
 * Functional description
 *	Delete the tail of a record.  If no tail, don't do nuttin'.
 *	If the address of a record tail has been passed, fetch data.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf
			("delete_tail (record_param %"QUADFORMAT"d, prior_page %"SLONGFORMAT", tail %p, tail_end %p)\n",
			 rpb->rpb_number.getValue(), prior_page, tail, tail_end);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   tail of record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	while (rpb->rpb_flags & rpb_incomplete)
	{
		rpb->rpb_page = rpb->rpb_f_page;
		rpb->rpb_line = rpb->rpb_f_line;

		// Since the callers are modifying this record, it should not be garbage collected.

		if (!DPM_fetch(tdbb, rpb, LCK_write)) {
			BUGCHECK(248);		// msg 248 cannot find record fragment
		}
		if (tail) {
			tail = SQZ_decompress(rpb->rpb_address, rpb->rpb_length, tail, tail_end);
		}
		DPM_delete(tdbb, rpb, prior_page);
		prior_page = rpb->rpb_page;
	}

	return tail;
}


// ******************************
// d f w _ s h o u l d _ k n o w
// ******************************
// Not all operations on system tables are relevant to inform DFW.
// In particular, changing comments on objects is irrelevant.
// Engine often performs empty update to force some tasks (e.g. to
// recreate index after field type change). So we must return true
// if relevant field changed or if no fields changed. Or we must
// return false if only irrelevant field changed.
static bool dfw_should_know(record_param* org_rpb, record_param* new_rpb,
	USHORT irrelevant_field, bool void_update_is_relevant)
{
	dsc desc2, desc3;
	bool irrelevant_changed = false;
	for (USHORT iter = 0; iter < org_rpb->rpb_record->rec_format->fmt_count; ++iter)
	{
		const bool a = EVL_field(0, org_rpb->rpb_record, iter, &desc2);
		const bool b = EVL_field(0, new_rpb->rpb_record, iter, &desc3);
		if (a != b || MOV_compare(&desc2, &desc3))
		{
			if (iter != irrelevant_field)
				return true;

			irrelevant_changed = true;
		}
	}
	return void_update_is_relevant ? !irrelevant_changed : false;
}


static void expunge(thread_db* tdbb, record_param* rpb, const jrd_tra* transaction, SLONG prior_page)
{
/**************************************
 *
 *	e x p u n g e
 *
 **************************************
 *
 * Functional description
 *	Expunge a fully mature deleted record.  Get rid of the record
 *	and all of the ancestors.  Be particulary careful since this
 *	can do a lot of damage.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = transaction->tra_attachment;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("expunge (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT
				  ", prior_page %"SLONGFORMAT")\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0,
				  prior_page);
	}
#endif

	if (attachment->att_flags & ATT_no_cleanup) {
		return;
	}

	// Re-fetch the record

	if (!DPM_get(tdbb, rpb, LCK_write))
	{
#ifdef GARBAGE_THREAD
		// expunge
		if (tdbb->getDatabase()->dbb_flags & DBB_gc_background)
			notify_garbage_collector(tdbb, rpb);
#endif
		return;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   expunge record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// Make sure it looks kosher and delete the record.

	const SLONG oldest_snapshot = rpb->rpb_relation->isTemporary() ? 
		attachment->att_oldest_snapshot : transaction->tra_oldest_active;

	if (!(rpb->rpb_flags & rpb_deleted) || rpb->rpb_transaction_nr >= oldest_snapshot)
	{

#ifdef GARBAGE_THREAD
		// expunge
		if (tdbb->getDatabase()->dbb_flags & DBB_gc_background)
			notify_garbage_collector(tdbb, rpb);
#endif

		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		return;
	}

	delete_record(tdbb, rpb, prior_page, 0);

	// If there aren't any old versions, don't worry about garbage collection.

	if (!rpb->rpb_b_page) {
		return;
	}

	// Delete old versions fetching data for garbage collection.

	record_param temp = *rpb;
	RecordStack empty_staying;
	garbage_collect(tdbb, &temp, rpb->rpb_page, empty_staying);
	VIO_bump_count(tdbb, DBB_expunge_count, rpb->rpb_relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_EXPUNGES);
}


static void garbage_collect(thread_db* tdbb,
							record_param* rpb, SLONG prior_page,
							RecordStack& staying)
{
/**************************************
 *
 *	g a r b a g e _ c o l l e c t
 *
 **************************************
 *
 * Functional description
 *	Garbage collect a chain of back record.  This is called from
 *	"purge" and "expunge."  One enters this routine with an
 *	inactive record_param, describing a records which has either
 *	1) just been deleted or
 *	2) just had its back pointers set to zero
 *	Therefor we can do a fetch on the back pointers we've got
 *	because we have the last existing copy of them.
 *
 **************************************/

	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("garbage_collect (record_param %"QUADFORMAT"d, prior_page %"SLONGFORMAT
				  ", staying)\n",
				  rpb->rpb_number.getValue(), prior_page);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// Delete old versions fetching data for garbage collection.

	RecordStack going;

	while (rpb->rpb_b_page != 0)
	{
		rpb->rpb_record = NULL;
		prior_page = rpb->rpb_page;
		rpb->rpb_page = rpb->rpb_b_page;
		rpb->rpb_line = rpb->rpb_b_line;
		if (!DPM_fetch(tdbb, rpb, LCK_write)) {
			BUGCHECK(291);		// msg 291 cannot find record back version
		}
		delete_record(tdbb, rpb, prior_page, tdbb->getDefaultPool());
		if (rpb->rpb_record) {
			going.push(rpb->rpb_record);
		}
		// Don't monopolize the server while chasing long back version chains.
		if (--tdbb->tdbb_quantum < 0)
			JRD_reschedule(tdbb, 0, true);
	}

	BLB_garbage_collect(tdbb, going, staying, prior_page, rpb->rpb_relation);
	IDX_garbage_collect(tdbb, rpb, going, staying);

	clearRecordStack(going);
}


static void garbage_collect_idx(thread_db* tdbb,
								record_param* org_rpb, //record_param* new_rpb,
								Record* old_data, Record* staying_data)
{
/**************************************
 *
 *	g a r b a g e _ c o l l e c t _ i d x
 *
 **************************************
 *
 * Functional description
 *	Garbage collect indices for which it is
 *	OK for other transactions to create indices with the same
 *	values.
 *
 **************************************/
	SET_TDBB(tdbb);

	// There should be a way to quickly check if there are indices and/or if there are blob-colums.

	// Garbage collect.  Start by getting all existing old versions (other
	// than the immediate two in question).

	RecordStack going, staying;
	list_staying(tdbb, org_rpb, staying);

	if (staying_data) {
		staying.push(staying_data);
	}

	// The data that is going is passed either via old_data, or via org_rpb.

	going.push(old_data ? old_data : org_rpb->rpb_record);

	BLB_garbage_collect(tdbb, going, staying, org_rpb->rpb_page, org_rpb->rpb_relation);
	IDX_garbage_collect(tdbb, org_rpb, going, staying);

	going.pop();

	if (staying_data) {
		staying.pop();
	}
	clearRecordStack(staying);
}


#ifdef GARBAGE_THREAD
static THREAD_ENTRY_DECLARE garbage_collector(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *	g a r b a g e _ c o l l e c t o r
 *
 **************************************
 *
 * Functional description
 *	Garbage collect the data pages marked in a
 *	relation's garbage collection bitmap. The
 *	hope is that offloading the computation
 *	and I/O burden of garbage collection will
 *	improve query response time and throughput.
 *
 **************************************/
	Database* dbb = (Database*)arg;
	CHECK_DBB(dbb);
	Database::SyncGuard dsGuard(dbb);

	ISC_STATUS_ARRAY status_vector;
	MOVE_CLEAR(status_vector, sizeof(status_vector));

	// Establish a thread context.
	ThreadContextHolder tdbb(status_vector);

	tdbb->setDatabase(dbb);
	tdbb->tdbb_quantum = SWEEP_QUANTUM;
	tdbb->tdbb_flags = TDBB_sweeper;

	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);

	// Surrender if resources to start up aren't available.
	bool found = false, flush = false;
	record_param rpb;
	MOVE_CLEAR(&rpb, sizeof(record_param));

	jrd_rel* relation = NULL;
	jrd_tra* transaction = NULL;

	try {
		// Pseudo attachment needed for lock owner identification.

		Attachment* const attachment = Attachment::create(dbb);
		tdbb->setAttachment(attachment);
		attachment->att_filename = dbb->dbb_filename;
		attachment->att_flags = ATT_garbage_collector;

		rpb.getWindow(tdbb).win_flags = WIN_garbage_collector;

		LCK_init(tdbb, LCK_OWNER_attachment);

		// Notify our creator that we have started
		dbb->dbb_flags |= DBB_garbage_collector;
		dbb->dbb_gc_init.release();

	}	// try
	catch (const Firebird::Exception&) {
		goto gc_exit;
	}

	try {

		// Initialize status vector after logging error.

		MOVE_CLEAR(status_vector, sizeof(status_vector));

		// The garbage collector flag is cleared to request the thread
		// to finish up and exit.

		while (dbb->dbb_flags & DBB_garbage_collector)
		{

			dbb->dbb_flags |= DBB_gc_active;
			found = false;
			relation = 0;

			// If background thread activity has been suspended because
			// of I/O errors then idle until the condition is cleared.
			// In particular, make worker threads perform their own
			// garbage collection so that errors are reported to users.

			if (dbb->dbb_flags & DBB_suspend_bgio)
			{
				Attachment* attachment;

				for (attachment = dbb->dbb_attachments; attachment; attachment = attachment->att_next)
				{
					if (attachment->att_flags & ATT_notify_gc) {
						attachment->att_flags &= ~ATT_notify_gc;
						attachment->att_flags |= ATT_disable_notify_gc;
					}
				}

				while (dbb->dbb_flags & DBB_suspend_bgio)
				{
					{ // scope
						Database::Checkout dcoHolder(dbb);
						dbb->dbb_gc_sem.tryEnter(10);
					}
					if (!(dbb->dbb_flags & DBB_garbage_collector)) {
						goto gc_exit;
					}
				}

				for (attachment = dbb->dbb_attachments; attachment; attachment = attachment->att_next)
				{
					if (attachment->att_flags & ATT_disable_notify_gc) {
						attachment->att_flags &= ~ATT_disable_notify_gc;
						attachment->att_flags |= ATT_notify_gc;
					}
				}
			}

			// Scan relation garbage collection bitmaps for candidate data pages.
			// Express interest in the relation to prevent it from being deleted
			// out from under us while garbage collection is in-progress.

			vec<jrd_rel*>* vector = dbb->dbb_relations;
			for (ULONG id = 0; vector && id < vector->count(); ++id)
			{
				relation = (*vector)[id];

				//jrd_rel::RelPagesSnapshot pagesSnapshot(tdbb, relation);
				//relation->fillPagesSnapshot(pagesSnapshot);

				RelationGarbage *relGarbage =
					relation ? (RelationGarbage*)relation->rel_garbage : NULL;

				if (relation && (relation->rel_gc_bitmap || relGarbage) &&
					!(relation->rel_flags & (REL_deleted | REL_deleting)))
				{
					if (relGarbage) {
						relGarbage->getGarbage(dbb->dbb_oldest_snapshot, &relation->rel_gc_bitmap);
					}

					++relation->rel_sweep_count;
					rpb.rpb_relation = relation;

					if (relation->rel_gc_bitmap)
					{
						while (relation->rel_gc_bitmap->getFirst())
						{
							const ULONG dp_sequence = relation->rel_gc_bitmap->current();

							if (!(dbb->dbb_flags & DBB_garbage_collector)) {
								--relation->rel_sweep_count;
								goto gc_exit;
							}

							relation->rel_gc_bitmap->clear(dp_sequence);

							if (!transaction)
							{
								// Start a "precommitted" transaction by using read-only,
								// read committed. Of particular note is the absence of a
								// transaction lock which means the transaction does not
								// inhibit garbage collection by its very existence.

								transaction = TRA_start(tdbb, sizeof(gc_tpb), gc_tpb);
								tdbb->setTransaction(transaction);
							}
							else
							{
								// Refresh our notion of the oldest transactions for
								// efficient garbage collection. This is very cheap.

								transaction->tra_oldest = dbb->dbb_oldest_transaction;
								transaction->tra_oldest_active = dbb->dbb_oldest_snapshot;
							}

							found = flush = true;
							rpb.rpb_number.setValue(((SINT64) dp_sequence * dbb->dbb_max_records) - 1);
							const RecordNumber last(rpb.rpb_number.getValue() + dbb->dbb_max_records);

							// Attempt to garbage collect all records on the data page.

							while (VIO_next_record(tdbb, &rpb, /*NULL,*/ transaction, NULL,
#ifdef SCROLLABLE_CURSORS
								false,
#endif
								true))
							{
								CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));

								if (!(dbb->dbb_flags & DBB_garbage_collector))
								{
									--relation->rel_sweep_count;
									goto gc_exit;
								}
								if (relation->rel_flags & REL_deleting) {
									goto rel_exit;
								}
								if (--tdbb->tdbb_quantum < 0) {
									JRD_reschedule(tdbb, SWEEP_QUANTUM, true);
								}
								if (rpb.rpb_number >= last) {
									break;
								}
							}
						}
					}

rel_exit:
					if (relation->rel_gc_bitmap)
					{
						if (!relation->rel_gc_bitmap->getFirst())
						{
							// If the bitmap is empty then release it
							delete relation->rel_gc_bitmap;
							relation->rel_gc_bitmap = 0;
						}
/* hvlad: obsolete ?
						else
						{
							// Otherwise release bitmap segments that have been cleared.
							while (relation->rel_gc_bitmap->getNext())
							{
								;	// do nothing
							}
						}
*/
					}
					--relation->rel_sweep_count;
				}
			}

			// If there's more work to do voluntarily ask to be rescheduled.
			// Otherwise, wait for event notification.

			if (found)
			{
				JRD_reschedule(tdbb, SWEEP_QUANTUM, true);
			}
			else
			{
				dbb->dbb_flags &= ~DBB_gc_pending;

				// Make no mistake about it, garbage collection is our first
				// priority. But if there's no garbage left to collect, assist
				// the overworked cache writer and reader threads.

				while (dbb->dbb_flags & DBB_garbage_collector && !(dbb->dbb_flags & DBB_gc_pending))
				{
#ifdef SUPERSERVER_V2
					if (CCH_free_page(tdbb) || CCH_prefetch_pages(tdbb)) {
						continue;
					}
#else
					if (CCH_free_page(tdbb)) {
						continue;
					}
#endif
					if (flush)
					{
						// As a last resort, flush garbage collected pages to
						// disk. This isn't strictly necessary but contributes
						// to the supply of free pages available for user
						// transactions. It also reduces the likelihood of
						// orphaning free space on lower precedence pages that
						// haven't been written if a crash occurs.

						flush = false;
						if (transaction) {
							CCH_flush(tdbb, FLUSH_SWEEP, 0);
						}
						continue;
					}
					dbb->dbb_flags &= ~DBB_gc_active;
					{ // scope
						Database::Checkout dcoHolder(dbb);
						dbb->dbb_gc_sem.tryEnter(10);
					}
					dbb->dbb_flags |= DBB_gc_active;
				}
			}
		}
	}
	catch (const Firebird::Exception& ex)
	{
		// Perfunctory error reporting -- got any better ideas ?

		Firebird::stuff_exception(status_vector, ex);
		jrd_file* file = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE)->file;
		gds__log_status(file->fil_string, status_vector);
		if (relation && relation->rel_sweep_count) {
			--relation->rel_sweep_count;
		}
	}

gc_exit:

	try {

		delete rpb.rpb_record;

		if (transaction) {
			TRA_commit(tdbb, transaction, false);
		}

		Attachment* const attachment = tdbb->getAttachment();
		if (attachment)
		{
			LCK_fini(tdbb, LCK_OWNER_attachment);
			Attachment::destroy(attachment);	// no need saving warning error strings here
			tdbb->setAttachment(NULL);
		}

		dbb->dbb_flags &= ~(DBB_garbage_collector | DBB_gc_active | DBB_gc_pending);
		// Notify the finalization caller that we're finishing.
		dbb->dbb_gc_fini.release();

	}	// try
	catch (const Firebird::Exception& ex)
	{
		// Perfunctory error reporting -- got any better ideas ?

		Firebird::stuff_exception(status_vector, ex);
		jrd_file* file = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE)->file;
		gds__log_status(file->fil_string, status_vector);
	}

	return 0;
}
#endif


static void invalidate_cursor_records(jrd_tra* transaction, record_param* mod_rpb)
{
/**************************************
 *
 *	i n v a l i d a t e _ c u r s o r _ r e c o r d s
 *
 **************************************
 *
 * Functional description
 *	Post a refetch request to the records currently fetched
 *  by active cursors of our transaction, because those records
 *  have just been updated or deleted.
 *
 **************************************/
	fb_assert(mod_rpb && mod_rpb->rpb_relation);

	for (jrd_req* request = transaction->tra_requests; request; request = request->req_tra_next)
	{
		if (request->req_flags & req_active)
		{
			for (size_t i = 0; i < request->req_count; i++)
			{
				record_param* const org_rpb = &request->req_rpb[i];

				if (org_rpb != mod_rpb &&
					org_rpb->rpb_relation && org_rpb->rpb_number.isValid() &&
					org_rpb->rpb_relation->rel_id == mod_rpb->rpb_relation->rel_id &&
					org_rpb->rpb_number == mod_rpb->rpb_number)
				{
					org_rpb->rpb_stream_flags |= RPB_s_refetch;
				}
			}
		}
	}
}


static void list_staying(thread_db* tdbb, record_param* rpb, RecordStack& staying)
{
/**************************************
 *
 *	l i s t _ s t a y i n g
 *
 **************************************
 *
 * Functional description
 *	Get all the data that's staying so we can clean up indexes etc.
 *	without losing anything.  Note that in the middle somebody could
 *	modify the record -- worse yet, somebody could modify it, commit,
 *	and have somebody else modify it, so if the back pointers on the
 *	original record change throw out what we've got and start over.
 *	"All the data that's staying" is: all the versions of the input
 *	record (rpb) that are stored in the relation.
 *
 **************************************/
	SET_TDBB(tdbb);

	Record* data = rpb->rpb_prior;
	Record* backout_rec = NULL;
	ULONG next_page = rpb->rpb_page;
	USHORT next_line = rpb->rpb_line;
	int max_depth = 0;
	int depth = 0;

	for (;;)
	{
		// Each time thru the loop, start from the latest version of the record
		// because during the call to VIO_data (below), things might change.

		record_param temp = *rpb;
		depth = 0;

		// If the entire record disappeared, then there is nothing staying.
		if (!DPM_fetch(tdbb, &temp, LCK_read))
		{
			clearRecordStack(staying);
			delete backout_rec;
			backout_rec = NULL;
			return;
		}

		// If anything changed, then start all over again.  This time with the
		// new, latest version of the record.

		if (temp.rpb_b_page != rpb->rpb_b_page || temp.rpb_b_line != rpb->rpb_b_line ||
			temp.rpb_flags != rpb->rpb_flags)
		{
			clearRecordStack(staying);
			delete backout_rec;
			backout_rec = NULL;
			next_page = temp.rpb_page;
			next_line = temp.rpb_line;
			max_depth = 0;
			*rpb = temp;
		}

		depth++;

		// Each time thru the for-loop, we process the next older version.
		// The while-loop finds this next older version.

		bool timed_out = false;
		while (temp.rpb_b_page &&
			!(temp.rpb_page == (SLONG) next_page && temp.rpb_line == (SSHORT) next_line))
		{
			temp.rpb_prior = (temp.rpb_flags & rpb_delta) ? data : NULL;

			if (!DPM_fetch_back(tdbb, &temp, LCK_read, -1))
			{
				fb_utils::init_status(tdbb->tdbb_status_vector);

				clearRecordStack(staying);
				delete backout_rec;
				backout_rec = NULL;
				next_page = rpb->rpb_page;
				next_line = rpb->rpb_line;
				max_depth = 0;
				timed_out = true;
				break;
			}
			depth++;
			// Don't monopolize the server while chasing long back version chains.
			if (--tdbb->tdbb_quantum < 0)
				JRD_reschedule(tdbb, 0, true);
		}
		if (timed_out) {
			continue;
		}

		// If there is a next older version, then process it: remember that
		// version's data in 'staying'.

		if (temp.rpb_page == (SLONG) next_page && temp.rpb_line == (SSHORT) next_line)
		{
			next_page = temp.rpb_b_page;
			next_line = temp.rpb_b_line;
			temp.rpb_record = NULL;
			if (temp.rpb_flags & rpb_deleted) {
				CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
			}
			else
			{
				// VIO_data below could change the flags
				const bool backout = (temp.rpb_flags & rpb_gc_active);
				VIO_data(tdbb, &temp, tdbb->getDefaultPool());
				if (!backout) 
				{
					staying.push(temp.rpb_record);
				}
				else 
				{
					fb_assert(!backout_rec);
					backout_rec = temp.rpb_record;
				}
				data = temp.rpb_record;
			}
			max_depth = depth;
			if (!next_page)
				break;
		}
		else {
			CCH_RELEASE(tdbb, &temp.getWindow(tdbb));
			break;
		}
	}

	// If the current number of back versions (depth) is smaller than the number
	// of back versions that we saw in a previous iteration (max_depth), then
	// somebody else must have been garbage collecting also.  Remove the entries
	// in 'staying' that have already been garbage collected.
	while (depth < max_depth--)
	{
		if (staying.hasData())
		{
			delete staying.pop();
		}
	}
	delete backout_rec;
}


#ifdef GARBAGE_THREAD
static void notify_garbage_collector(thread_db* tdbb, record_param* rpb, SLONG tranid)
{
/**************************************
 *
 *	n o t i f y _ g a r b a g e _ c o l l e c t o r
 *
 **************************************
 *
 * Functional description
 *	Notify the garbage collector that there is work to be
 *	done. Each relation has a garbage collection sparse
 *	bitmap where each bit corresponds to a data page
 *	sequence number of a data page known to have records
 *	which are candidates for garbage collection.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();
	jrd_rel* const relation = rpb->rpb_relation;

	if (relation->isTemporary())
		return;

	if (tranid == -1)
		tranid = rpb->rpb_transaction_nr;

	// system transaction has its own rules
	if (tranid == 0)
		return;

	// If this is a large sequential scan then defer the release
	// of the data page to the LRU tail until the garbage collector
	// can garbage collect the page.

	if (rpb->getWindow(tdbb).win_flags & WIN_large_scan) {
		rpb->getWindow(tdbb).win_flags |= WIN_garbage_collect;
	}

	// A relation's garbage collect bitmap is allocated
	// from the database permanent pool.

	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);
	const SLONG dp_sequence = rpb->rpb_number.getValue() / dbb->dbb_max_records;

	if (!relation->rel_garbage)
	{
		relation->rel_garbage =
			FB_NEW(*tdbb->getDefaultPool()) RelationGarbage(*tdbb->getDefaultPool());
	}

	relation->rel_garbage->addPage(tdbb->getDefaultPool(), dp_sequence, tranid);

	if (tranid > relation->rel_garbage->minTranID())
		tranid = relation->rel_garbage->minTranID();

	// If the garbage collector isn't active then poke
	// the event on which it sleeps to awaken it.

	dbb->dbb_flags |= DBB_gc_pending;

	if (!(dbb->dbb_flags & DBB_gc_active) &&
		(tranid < (tdbb->getTransaction() ?
			tdbb->getTransaction()->tra_oldest_active : dbb->dbb_oldest_snapshot)) )
	{
		dbb->dbb_gc_sem.release();
	}
}
#endif


static Record* realloc_record(Record*& record, USHORT fmt_length)
{
/**************************************
 *
 *	r e a l l o c _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Realloc a record to accomodate longer length format.
 *
 **************************************/
	Record* new_record = FB_NEW_RPT(record->rec_pool, fmt_length) Record(record->rec_pool);

	new_record->rec_precedence.takeOwnership(record->rec_precedence);
	// start copying at rec_format, to not mangle source->rec_precedence
	memcpy(&new_record->rec_format, &record->rec_format,
		sizeof(Record) - ((UCHAR*)&new_record->rec_format - (UCHAR*)new_record) + record->rec_length);

	delete record;
	record = new_record;

	return new_record;
}


static int prepare_update(	thread_db*		tdbb,
							jrd_tra*		transaction,
							SLONG			commit_tid_read,
							record_param*	rpb,
							record_param*	temp,
							record_param*	new_rpb,
							PageStack&		stack,
							bool			writelock)
{
/**************************************
 *
 *	p r e p a r e _ u p d a t e
 *
 **************************************
 *
 * Functional description
 *	Prepare for a modify or erase.  Store the old version
 *	of a record, fetch the current version, check transaction
 *	states, etc.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf
			("prepare_update (transaction %"SLONGFORMAT
			 ", commit_tid read %"SLONGFORMAT", record_param %"QUADFORMAT"d, ",
			 transaction ? transaction->tra_number : 0, commit_tid_read,
			 rpb ? rpb->rpb_number.getValue() : 0);
		printf(" temp_rpb %"QUADFORMAT"d, new_rpb %"QUADFORMAT"d, stack)\n",
				  temp ? temp->rpb_number.getValue() : 0,
				  new_rpb ? new_rpb->rpb_number.getValue() : 0);
	}
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
	{
		printf
			("   old record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT
			 ":%d, prior %p\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line, (void*) rpb->rpb_prior);
	}
#endif

	/* We're almost ready to go.  To erase the record, we must first
	make a copy of the old record someplace else.  Then we must re-fetch
	the record (for write) and verify that it is legal for us to
	erase it -- that it was written by a transaction that was committed
	when we started.  If not, the transaction that wrote the record
	is either active, dead, or in limbo.  If the transaction is active,
	wait for it to finish.  If it commits, we can't procede and must
	return an update conflict.  If the transaction is dead, back out the
	old version of the record and try again.  If in limbo, punt.

	The above is true only for concurrency & consistency mode transactions.
	For read committed transactions, check if the latest commited version
	is the same as the version that was read for the update.  If yes,
	the update can take place.  If some other transaction has modified
	the record and committed, then an update error will be returned.
   */

	*temp = *rpb;
	Record* record = rpb->rpb_record;

	// Mark the record as chained version, and re-store it

	temp->rpb_address = record->rec_data;
	temp->rpb_length = record->rec_format->fmt_length;
	temp->rpb_format_number = record->rec_format->fmt_version;
	temp->rpb_flags = rpb_chained;

	if (temp->rpb_prior) {
		temp->rpb_flags |= rpb_delta;
	}

	// If it makes sense, store a differences record
	UCHAR differences[MAX_DIFFERENCES];
	if (new_rpb)
	{
		const USHORT l = SQZ_differences(
							reinterpret_cast<const char*>(new_rpb->rpb_address),
							new_rpb->rpb_length,
							reinterpret_cast<char*>(temp->rpb_address),
							temp->rpb_length,
							reinterpret_cast<char*>(differences),
							sizeof(differences));
		if ((l < sizeof(differences)) && (l < temp->rpb_length))
		{
			temp->rpb_address = differences;
			temp->rpb_length = l;
			new_rpb->rpb_flags |= rpb_delta;
		}
	}

	if (writelock)
	{
	  temp->rpb_address = differences;
	  temp->rpb_length = SQZ_no_differences((SCHAR*) differences, temp->rpb_length);
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		if (new_rpb)
		{
			printf("    new record is%sa delta \n",
					  (new_rpb->rpb_flags & rpb_delta) ? " " : " NOT ");
		}
	}
#endif

	temp->rpb_number = rpb->rpb_number;
	DPM_store(tdbb, temp, stack, DPM_secondary);

	// Re-fetch the original record for write in anticipation of
	// replacing it with a completely new version.  Make sure it
	// was the same one we stored above.
	record_param org_rpb;
	SLONG update_conflict_trans = -1;
	while (true)
	{
		org_rpb.rpb_flags = rpb->rpb_flags;
		org_rpb.rpb_f_line = rpb->rpb_f_line;
		org_rpb.rpb_f_page = rpb->rpb_f_page;

		if (!DPM_get(tdbb, rpb, LCK_write))
		{
			// There is no reason why this record would disappear for a
			// snapshot transaction.
			if (!(transaction->tra_flags & TRA_read_committed))
			{
				BUGCHECK(186);	// msg 186 record disappeared
			}
			else
			{
				// A read-committed transaction, on the other hand, doesn't
				// insist on the presence of any version, so versions of records
				// and entire records it has already read might be garbage-collected.
				if (!DPM_fetch(tdbb, temp, LCK_write)) {
					BUGCHECK(291);	// msg 291 cannot find record back version
				}
				delete_record(tdbb, temp, (SLONG) 0, 0);
				return PREPARE_DELETE;
			}
		}

		int state = TRA_snapshot_state(tdbb, transaction, rpb->rpb_transaction_nr);

		// Reset the garbage collect active flag if the transaction state is
		// in a terminal state. If committed it must have been a precommitted
		// transaction that was backing out a dead record version and the
		// system crashed. Clear the flag and set the state to tra_dead to
		// reattempt the backout.

		if (rpb->rpb_flags & rpb_gc_active)
		{
			switch (state)
			{
			case tra_committed:
				state = tra_dead;
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			case tra_dead:
				rpb->rpb_flags &= ~rpb_gc_active;
				break;

			default:
				break;
			}
		}

		if (state == tra_precommitted)
			state = check_precommitted(transaction, rpb);

		switch (state)
		{
		case tra_committed:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf
					("    record's transaction (%"SLONGFORMAT
					 ") is committed (my TID - %"SLONGFORMAT")\n",
					 rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif
			if (rpb->rpb_flags & rpb_deleted)
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				// get rid of the back records we just created
				if (!(transaction->tra_attachment->att_flags & ATT_no_cleanup))
				{
					if (!DPM_fetch(tdbb, temp, LCK_write)) {
						BUGCHECK(291);	// msg 291 cannot find record back version
					}
					delete_record(tdbb, temp, (SLONG) 0, 0);
				}
				if (writelock) {
					return PREPARE_DELETE;
				}
				IBERROR(188);	// msg 188 cannot update erased record
			}

			// For read committed transactions, if the record version we read
			// and started the update
			// has been updated by another transaction which committed in the
			// meantime, we cannot proceed further - update conflict error.


			if ((transaction->tra_flags & TRA_read_committed) &&
				(commit_tid_read != rpb->rpb_transaction_nr))
			{
				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				if (!DPM_fetch(tdbb, temp, LCK_write)) {
					BUGCHECK(291);	// msg 291 cannot find record back version
				}
				delete_record(tdbb, temp, (SLONG) 0, 0);
				return PREPARE_CONFLICT;
			}

			/*
			 * The case statement for tra_us has been pushed down to this
			 * current position as we donot want to give update conflict
			 * errors and the "cannot update erased record" within the same
			 * transaction. We were getting these erroe in case of triggers.
			 * A pre-delete trigger could update or delete a record which we
			 * are then tring to change.
			 * In order to remove these changes and restore original behaviour,
			 * move this case statement above the 2 "if" statements.
			 * smistry 23-Aug-99
			 */
		case tra_us:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO && state == tra_us)
			{
				printf
					("    record's transaction (%"SLONGFORMAT
					 ") is us (my TID - %"SLONGFORMAT")\n",
					 rpb->rpb_transaction_nr, transaction->tra_number);
			}

#endif
			if (rpb->rpb_b_page != temp->rpb_b_page || rpb->rpb_b_line != temp->rpb_b_line ||
				rpb->rpb_transaction_nr != temp->rpb_transaction_nr ||
				(rpb->rpb_flags & rpb_delta) != (temp->rpb_flags & rpb_delta) ||
				rpb->rpb_flags != org_rpb.rpb_flags ||
				(rpb->rpb_flags & rpb_incomplete) &&
					(rpb->rpb_f_page != org_rpb.rpb_f_page || rpb->rpb_f_line != org_rpb.rpb_f_line))
			{

				// the primary copy of the record was dead and someone else
				// backed it out for us.  Our data is OK but our pointers
				// aren't, so get rid of the record we created and try again

				CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
				if (!(transaction->tra_attachment->att_flags & ATT_no_cleanup))
				{
					record_param temp2 = *temp;
					if (!DPM_fetch(tdbb, &temp2, LCK_write)) {
						BUGCHECK(291);	// msg 291 cannot find record back version
					}
					delete_record(tdbb, &temp2, (SLONG) 0, 0);
				}
				temp->rpb_b_page = rpb->rpb_b_page;
				temp->rpb_b_line = rpb->rpb_b_line;
				temp->rpb_flags &= ~rpb_delta;
				temp->rpb_flags |= rpb->rpb_flags & rpb_delta;
				temp->rpb_transaction_nr = rpb->rpb_transaction_nr;
				DPM_store(tdbb, temp, stack, DPM_secondary);
				continue;
			}
			stack.push(temp->rpb_page);
			return PREPARE_OK;

		case tra_active:
		case tra_limbo:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is %s (my TID - %"SLONGFORMAT")\n",
					 rpb->rpb_transaction_nr, (state == tra_active) ? "active" : "limbo",
					 transaction->tra_number);
			}

#endif
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));

			// Wait as long as it takes for an active transaction which has modified
			// the record. If an active transaction has used its TID to safely
			// backout a fragmented dead record version, spin wait because it will
			// finish shortly.

			if (!(rpb->rpb_flags & rpb_gc_active)) {
				state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_wait);
			}
			else
			{
				state = TRA_wait(tdbb, transaction, rpb->rpb_transaction_nr, jrd_tra::tra_probe);

				if (state == tra_active)
				{
					Database::Checkout dcoHolder(dbb);
					THREAD_SLEEP(100);	// milliseconds
					continue;
				}
			}

			// The snapshot says: transaction was active.  The TIP page says: transaction
			// is committed.  Maybe the transaction was rolled back via a transaction
			// level savepoint.  In that case, the record DPM_get-ed via rpb is already
			// backed out.  Try to refetch that record one more time.

			if ((state == tra_committed) && (rpb->rpb_transaction_nr != update_conflict_trans))
			{
				update_conflict_trans = rpb->rpb_transaction_nr;
				continue;
			}
			if (state != tra_dead && !(temp->rpb_flags & rpb_deleted))
			{
				if (!DPM_fetch(tdbb, temp, LCK_write)) {
					BUGCHECK(291);	// msg 291 cannot find record back version
				}
				delete_record(tdbb, temp, (SLONG) 0, 0);
			}
			switch (state)
			{
			case tra_committed:
				// We need to loop waiting in read committed transactions only
				if (!(transaction->tra_flags & TRA_read_committed))
				{
					ERR_post(Arg::Gds(isc_deadlock) <<
							 Arg::Gds(isc_update_conflict) <<
							 Arg::Gds(isc_concurrent_transaction) << Arg::Num(update_conflict_trans));
				}
			case tra_active:
				return PREPARE_LOCKERR;

			case tra_limbo:
				ERR_post(Arg::Gds(isc_deadlock) <<
						 Arg::Gds(isc_trainlim));

			case tra_dead:
				break;
			} // switch (state)
			break;

		case tra_dead:
		case tra_precommitted:
#ifdef VIO_DEBUG
			if (debug_flag > DEBUG_READS_INFO)
			{
				printf("    record's transaction (%"SLONGFORMAT") is dead (my TID - %"SLONGFORMAT")\n",
					 rpb->rpb_transaction_nr, transaction->tra_number);
			}
#endif
			CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
			break;
		}

		if (state == tra_precommitted)
		{
			Database::Checkout dcoHolder(dbb);
			THREAD_SLEEP(100);	// milliseconds
		}
		else {
			VIO_backout(tdbb, rpb, transaction);
		}
	}
	return PREPARE_OK;
}


static void purge(thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	p u r g e
 *
 **************************************
 *
 * Functional description
 *	Purge old versions of a fully mature record.  The record is
 *	guaranteed not to be deleted.  Return true if the record
 *	didn't need to be purged or if the purge was done.  Return false
 *	if the purge couldn't happen because somebody else had the record.
 *	But the function was made void since nobody checks its return value.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL) {
		printf("purge (record_param %"QUADFORMAT"d)\n", rpb->rpb_number.getValue());
	}
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line);
	}
#endif

	// Release and re-fetch the page for write.  Make sure it's still the
	// same record (give up if not).  Then zap the back pointer and release
	// the record.

	record_param temp = *rpb;
	jrd_rel* relation = rpb->rpb_relation;
	rpb->rpb_record = VIO_gc_record(tdbb, relation);

	VIO_data(tdbb, rpb, dbb->dbb_permanent);

	temp.rpb_prior = rpb->rpb_prior;
	Record* record = rpb->rpb_record;
	Record* gc_rec = record;
	rpb->rpb_record = temp.rpb_record;

	if (!DPM_get(tdbb, rpb, LCK_write))
	{
		gc_rec->rec_flags &= ~REC_gc_active;

#ifdef GARBAGE_THREAD
		// purge
		if (tdbb->getDatabase()->dbb_flags & DBB_gc_background)
			notify_garbage_collector(tdbb, rpb);
#endif
		return; //false;
	}

	rpb->rpb_prior = temp.rpb_prior;

	if (temp.rpb_transaction_nr != rpb->rpb_transaction_nr || temp.rpb_b_line != rpb->rpb_b_line ||
		temp.rpb_b_page != rpb->rpb_b_page || rpb->rpb_b_page == 0)
	{
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		gc_rec->rec_flags &= ~REC_gc_active;
		return; // true;
	}

	rpb->rpb_b_page = 0;
	rpb->rpb_b_line = 0;
	rpb->rpb_flags &= ~(rpb_delta | rpb_gc_active);
	CCH_MARK(tdbb, &rpb->getWindow(tdbb));
	DPM_rewrite_header(tdbb, rpb);
	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));

	RecordStack staying;
	staying.push(record);
	garbage_collect(tdbb, &temp, rpb->rpb_page, staying);
	gc_rec->rec_flags &= ~REC_gc_active;
	VIO_bump_count(tdbb, DBB_purge_count, relation);
	tdbb->bumpStats(RuntimeStatistics::RECORD_PURGES);

	return; // true;
}


static Record* replace_gc_record(jrd_rel* relation, Record** gc_record, USHORT length)
{
/**************************************
 *
 *	r e p l a c e _ g c _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Replace a relation garbage collect record
 *	to accomodate longer length format.
 *
 **************************************/

	vec<Record*>* vector = relation->rel_gc_rec;
	vec<Record*>::iterator rec_ptr, end;
	for (rec_ptr = vector->begin(), end = vector->end(); rec_ptr < end; ++rec_ptr)
	{
		if (*rec_ptr == *gc_record)
		{
			// 26 Sep 2002, SKIDDER: Failure to do so (*gc_record = ...) causes nasty memory corruption in
			// some cases.
			*gc_record = realloc_record(*rec_ptr, length);
			return *rec_ptr;
		}
	}

	BUGCHECK(288);			// msg 288 garbage collect record disappeared
	return NULL;			// Added to remove compiler warnings
}


static void replace_record(thread_db*		tdbb,
						   record_param*	rpb,
						   PageStack*		stack,
						   const jrd_tra*	transaction)
{
/**************************************
 *
 *	r e p l a c e _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Replace a record and get rid of the old tail, if any.  If requested,
 *	fetch data for the record on the way out.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf("replace_record (record_param %"QUADFORMAT"d, transaction %"SLONGFORMAT")\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
	{
		printf
			("   record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT
			 ":%d, prior %p\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_flags, rpb->rpb_b_page, rpb->rpb_b_line,
			 rpb->rpb_f_page, rpb->rpb_f_line, (void*) rpb->rpb_prior);
	}
#endif

	record_param temp = *rpb;
	rpb->rpb_flags &= ~(rpb_fragment | rpb_incomplete | rpb_chained | rpb_gc_active);
	DPM_update(tdbb, rpb, stack, transaction);
	delete_tail(tdbb, &temp, rpb->rpb_page, 0, 0);

	if ((rpb->rpb_flags & rpb_delta) && !rpb->rpb_prior) {
		rpb->rpb_prior = rpb->rpb_record;
	}
}



static void set_system_flag(thread_db* tdbb, record_param* rpb, USHORT field_id, SSHORT flag)
{
/**************************************
 *
 *	s e t _ s y s t e m _ f l a g
 *
 **************************************
 *
 * Functional description
 *	Set the value of a particular field to a known binary value.
 *
 **************************************/
	dsc desc1;

	Record* record = rpb->rpb_record;
	if (EVL_field(0, record, field_id, &desc1)) {
		return;
	}

	dsc desc2;
	desc2.dsc_dtype = dtype_short;
	desc2.dsc_length = sizeof(SSHORT);
	desc2.dsc_scale = 0;
	desc2.dsc_sub_type = 0;
	desc2.dsc_address = (UCHAR *) & flag;
	MOV_move(tdbb, &desc2, &desc1);
	CLEAR_NULL(record, field_id);
}


static void update_in_place(thread_db* tdbb,
							jrd_tra* transaction, record_param* org_rpb, record_param* new_rpb)
{
/**************************************
 *
 *	u p d a t e _ i n _ p l a c e
 *
 **************************************
 *
 * Functional description
 *	Modify a record in place.  This is used for system transactions
 *	and for multiple modifications of a user record.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf
			("update_in_place (transaction %"SLONGFORMAT", org_rpb %"QUADFORMAT"d, "
			 "new_rpb %"QUADFORMAT"d)\n",
			 transaction ? transaction->tra_number : 0, org_rpb->rpb_number.getValue(),
			 new_rpb ? new_rpb->rpb_number.getValue() : 0);
	}
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
	{
		printf
			("   old record  %"SLONGFORMAT":%d, rpb_trans %"SLONGFORMAT
			 ", flags %d, back %"SLONGFORMAT":%d, fragment %"SLONGFORMAT":%d\n",
			 org_rpb->rpb_page, org_rpb->rpb_line, org_rpb->rpb_transaction_nr,
			 org_rpb->rpb_flags, org_rpb->rpb_b_page, org_rpb->rpb_b_line,
			 org_rpb->rpb_f_page, org_rpb->rpb_f_line);
	}
#endif

	PageStack& stack = new_rpb->rpb_record->rec_precedence;
	jrd_rel* relation = org_rpb->rpb_relation;
	Record* const old_data = org_rpb->rpb_record;

	// If the old version has been stored as a delta, things get complicated.  Clearly,
	// if we overwrite the current record, the differences from the current version
	// becomes meaningless.  What we need to do is replace the old "delta" record
	// with an old "complete" record, update in placement, then delete the old delta record

	Record* gc_rec = NULL;
	record_param temp2;
	const Record* prior = org_rpb->rpb_prior;
	if (prior)
	{
		temp2 = *org_rpb;
		temp2.rpb_record = VIO_gc_record(tdbb, relation);
		temp2.rpb_page = org_rpb->rpb_b_page;
		temp2.rpb_line = org_rpb->rpb_b_line;
		if (! DPM_fetch(tdbb, &temp2, LCK_read)) {
			BUGCHECK(291);	 // msg 291 cannot find record back version
		}

		VIO_data(tdbb, &temp2, dbb->dbb_permanent);
		gc_rec = temp2.rpb_record;
		temp2.rpb_flags = rpb_chained;
		if (temp2.rpb_prior) {
			temp2.rpb_flags |= rpb_delta;
		}
		temp2.rpb_number = org_rpb->rpb_number;
		DPM_store(tdbb, &temp2, stack, DPM_secondary);

		stack.push(temp2.rpb_page);
	}

	if (!DPM_get(tdbb, org_rpb, LCK_write)) {
		BUGCHECK(186);			// msg 186 record disappeared
	}

	if (prior)
	{
		const SLONG page = org_rpb->rpb_b_page;
		const USHORT line = org_rpb->rpb_b_line;
		org_rpb->rpb_b_page = temp2.rpb_page;
		org_rpb->rpb_b_line = temp2.rpb_line;
		org_rpb->rpb_flags &= ~rpb_delta;
		org_rpb->rpb_prior = NULL;
		temp2.rpb_page = page;
		temp2.rpb_line = line;
	}

	UCHAR* const save_address = org_rpb->rpb_address;
	const USHORT length = org_rpb->rpb_length;
	const USHORT format_number = org_rpb->rpb_format_number;
	org_rpb->rpb_address = new_rpb->rpb_address;
	org_rpb->rpb_length = new_rpb->rpb_length;
	org_rpb->rpb_format_number = new_rpb->rpb_format_number;
	org_rpb->rpb_flags |= new_rpb->rpb_flags & rpb_uk_modified;

	DEBUG;
	replace_record(tdbb, org_rpb, &stack, transaction);
	DEBUG;

	org_rpb->rpb_address = save_address;
	org_rpb->rpb_length = length;
	org_rpb->rpb_format_number = format_number;
	org_rpb->rpb_undo = old_data;

	if (transaction->tra_flags & TRA_system)
	{
		// Garbage collect.  Start by getting all existing old versions (other
		// than the immediate two in question).

		RecordStack staying;
		list_staying(tdbb, org_rpb, staying);
		staying.push(new_rpb->rpb_record);

		RecordStack going;
		going.push(org_rpb->rpb_record);

		BLB_garbage_collect(tdbb, going, staying, org_rpb->rpb_page, relation);
		IDX_garbage_collect(tdbb, org_rpb, going, staying);

		staying.pop();
		clearRecordStack(staying);
	}

	if (prior)
	{
		if (!DPM_fetch(tdbb, &temp2, LCK_write)) {
			BUGCHECK(291);		// msg 291 cannot find record back version
		}
		delete_record(tdbb, &temp2, org_rpb->rpb_page, 0);
	}

	if (gc_rec) {
		gc_rec->rec_flags &= ~REC_gc_active;
	}
}


static void verb_post(thread_db* tdbb,
					  jrd_tra* transaction,
					  record_param* rpb,
					  Record* old_data,
					  //record_param* new_rpb,
					  const bool same_tx, const bool new_ver)
{
/**************************************
 *
 *	v e r b _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Post a record update under verb control to a transaction.
 *	If the previous version of the record was created by
 *	this transaction in a different verb, save the data as well.
 *
 * Input:
 *	old_data:	Only supplied if an in-place operation was performed
 *				(i.e. update_in_place).
 *	new_rpb:	Only used to pass to garbage_collect_idx but that function doesn't use it!
 *	same_tx:	true if this transaction inserted/updated this record
 *				and then deleted it.
 *				false in all other cases.
 *
 **************************************/
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

	// Find action block for relation
	VerbAction* action;
	for (action = transaction->tra_save_point->sav_verb_actions; action; action = action->vct_next)
	{
		if (action->vct_relation == rpb->rpb_relation) {
			break;
		}
	}

	if (!action)
	{
		if ( (action = transaction->tra_save_point->sav_verb_free) ) {
			transaction->tra_save_point->sav_verb_free = action->vct_next;
		}
		else {
			action = FB_NEW(*tdbb->getDefaultPool()) VerbAction();
		}
		action->vct_next = transaction->tra_save_point->sav_verb_actions;
		transaction->tra_save_point->sav_verb_actions = action;
		action->vct_relation = rpb->rpb_relation;
	}

	if (!RecordBitmap::test(action->vct_records, rpb->rpb_number.getValue()))
	{
		RBM_SET(tdbb->getDefaultPool(), &action->vct_records, rpb->rpb_number.getValue());
		if (old_data)
		{
			// An update-in-place is being posted to this savepoint, and this
			// savepoint hasn't seen this record before.

			if (!action->vct_undo) {
				action->vct_undo = new UndoItemTree(tdbb->getDefaultPool());
			}
			const UCHAR flags = same_tx ? REC_same_tx : 0;
			action->vct_undo->add(UndoItem(transaction, rpb->rpb_number, old_data, flags));
		}
		else if (same_tx)
		{
			// An insert/update followed by a delete is posted to this savepoint,
			// and this savepoint hasn't seen this record before.

			if (!action->vct_undo) {
				action->vct_undo = new UndoItemTree(tdbb->getDefaultPool());
			}
			const UCHAR flags = REC_same_tx | (new_ver ? REC_new_version : 0);
			action->vct_undo->add(UndoItem(rpb->rpb_number, flags));
		}
	}
	else if (same_tx)
	{
		Record* undo = NULL;
		if (action->vct_undo && action->vct_undo->locate(rpb->rpb_number.getValue()))
		{
			// An insert/update followed by a delete is posted to this savepoint,
			// and this savepoint has already undo for this record.
			undo = action->vct_undo->current().setupRecord(transaction, REC_same_tx);
		}
		else
		{
			// An insert/update followed by a delete is posted to this savepoint,
			// and this savepoint has seen this record before but it doesn't have undo data.

			if (!action->vct_undo) {
				action->vct_undo = new UndoItemTree(tdbb->getDefaultPool());
			}
			const UCHAR flags = REC_same_tx | REC_new_version;
			action->vct_undo->add(UndoItem(rpb->rpb_number, flags));
		}
		if (old_data)
		{
			// The passed old_data will not be used.  Thus, garbage collect.

			garbage_collect_idx(tdbb, rpb, /*new_rpb,*/ old_data, undo);
		}
	}
	else if (old_data)
	{
		// We are posting an update-in-place, but the current savepoint has
		// already undo data for this record.  The old_data will not be used,
		// so make sure we garbage collect before we lose track of the
		// in-place-updated record.

		Record* undo = NULL;
		if (action->vct_undo && action->vct_undo->locate(rpb->rpb_number.getValue())) {
			undo = action->vct_undo->current().setupRecord(transaction);
		}

		garbage_collect_idx(tdbb, rpb, /*new_rpb,*/ old_data, undo);
	}
}
