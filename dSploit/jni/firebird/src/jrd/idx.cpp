/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		idx.cpp
 *	DESCRIPTION:	Index manager
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
 * 2003.03.04 Dmitry Yemanov: Added support for NULLs in unique indices.
 *							  Done in two stages:
 *								1. Restored old behaviour of having only _one_
 *								   NULL key allowed (i.e. two NULLs are considered
 *								   duplicates). idx_e_nullunique error was removed.
 *								2. Changed algorithms in IDX_create_index() and
 *								   check_duplicates() to ignore NULL key duplicates.
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/intl.h"
#include "../jrd/req.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/sort.h"
#include "../jrd/lls.h"
#include "../jrd/tra.h"
#include "gen/iberror.h"
#include "../jrd/sbm.h"
#include "../jrd/exe.h"
#include "../jrd/scl.h"
#include "../jrd/lck.h"
#include "../jrd/rse.h"
#include "../jrd/cch.h"
#include "../jrd/gdsassert.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/sort_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/tra_proto.h"


using namespace Jrd;
using namespace Ods;
using namespace Firebird;

// Data to be passed to index fast load duplicates routine

struct index_fast_load
{
	SLONG ifl_duplicates;
	USHORT ifl_key_length;
};

static idx_e check_duplicates(thread_db*, Record*, index_desc*, index_insertion*, jrd_rel*);
static idx_e check_foreign_key(thread_db*, Record*, jrd_rel*, jrd_tra*, index_desc*, jrd_rel**, USHORT *);
static idx_e check_partner_index(thread_db*, jrd_rel*, Record*, jrd_tra*, index_desc*, jrd_rel*, USHORT);
static bool duplicate_key(const UCHAR*, const UCHAR*, void*);
static PageNumber get_root_page(thread_db*, jrd_rel*);
static int index_block_flush(void*);
static idx_e insert_key(thread_db*, jrd_rel*, Record*, jrd_tra*, WIN *, index_insertion*, jrd_rel**, USHORT *);
static bool key_equal(const temporary_key*, const temporary_key*);
static void release_index_block(thread_db*, IndexBlock*);
static void signal_index_deletion(thread_db*, jrd_rel*, USHORT);


void IDX_check_access(thread_db* tdbb, CompilerScratch* csb, jrd_rel* view, jrd_rel* relation)
{
/**************************************
 *
 *	I D X _ c h e c k _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Check the various indices in a relation
 *	to see if we need REFERENCES access to fields
 *	in the primary key.   Don't call this routine for
 *	views or external relations, since the mechanism
 *	ain't there.
 *
 **************************************/
	SET_TDBB(tdbb);

	index_desc idx;
	idx.idx_id = idx_invalid;
	RelationPages* relPages = relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);
	WIN referenced_window(relPages->rel_pg_space_id, -1);

	while (BTR_next_index(tdbb, relation, 0, &idx, &window))
	{
		if (idx.idx_flags & idx_foreign)
		{
			// find the corresponding primary key index

			if (!MET_lookup_partner(tdbb, relation, &idx, 0)) {
				continue;
			}
			jrd_rel* referenced_relation = MET_relation(tdbb, idx.idx_primary_relation);
			MET_scan_relation(tdbb, referenced_relation);
			const USHORT index_id = idx.idx_primary_index;

			// get the description of the primary key index

			referenced_window.win_page = get_root_page(tdbb, referenced_relation);
			referenced_window.win_flags = 0;
			index_root_page* referenced_root =
				(index_root_page*) CCH_FETCH(tdbb, &referenced_window, LCK_read, pag_root);
			index_desc referenced_idx;
			if (!BTR_description(tdbb, referenced_relation, referenced_root,
								 &referenced_idx, index_id))
			{
				BUGCHECK(173);	// msg 173 referenced index description not found
			}

			// post references access to each field in the index

			const index_desc::idx_repeat* idx_desc = referenced_idx.idx_rpt;
			for (USHORT i = 0; i < referenced_idx.idx_count; i++, idx_desc++)
			{
				const jrd_fld* referenced_field =
					MET_get_field(referenced_relation, idx_desc->idx_field);
				CMP_post_access(tdbb, csb,
								referenced_relation->rel_security_name,
								(view ? view->rel_id : 0),
								SCL_sql_references, "TABLE",
								referenced_relation->rel_name);
				CMP_post_access(tdbb, csb,
								referenced_field->fld_security_name, 0,
								SCL_sql_references, "COLUMN",
								referenced_field->fld_name);
			}

			CCH_RELEASE(tdbb, &referenced_window);
		}
	}
}


bool IDX_check_master_types (thread_db* tdbb, index_desc& idx, jrd_rel* partner_relation, int& bad_segment)
{
/**********************************************
 *
 *	I D X _ c h e c k _ m a s t e r _ t y p e s
 *
 **********************************************
 *
 * Functional description
 *	Check if both indices of foreign key constraint
 *	has compatible data types in appropriate segments.
 *	Called when detail index is created after idx_itype
 *	was assigned
 *
 **********************************************/

	SET_TDBB(tdbb);

	index_desc partner_idx;

	// get the index root page for the partner relation
	WIN window(get_root_page(tdbb, partner_relation));
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_root);

	// get the description of the partner index
	if (!BTR_description(tdbb, partner_relation, root, &partner_idx, idx.idx_primary_index))
		BUGCHECK(175);			// msg 175 partner index description not found

	CCH_RELEASE(tdbb, &window);

	// make sure partner index have the same segment count as our
	fb_assert(idx.idx_count == partner_idx.idx_count);

	for (int i = 0; i < idx.idx_count; i++)
	{
		if (idx.idx_rpt[i].idx_itype != partner_idx.idx_rpt[i].idx_itype)
		{
			bad_segment = i;
			return false;
		}
	}

	return true;
}


void IDX_create_index(thread_db* tdbb,
					  jrd_rel* relation,
					  index_desc* idx,
					  const TEXT* index_name,
					  USHORT* index_id,
					  jrd_tra* transaction,
					  SelectivityList& selectivity)
{
/**************************************
 *
 *	I D X _ c r e a t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create and populate index.
 *
 **************************************/
	idx_e result = idx_e_ok;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (relation->rel_file)
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_extfile_uns_op) << Arg::Str(relation->rel_name));
	}
	else if (relation->isVirtual())
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_wish_list));
	}

	get_root_page(tdbb, relation);

	fb_assert(transaction);

	BTR_reserve_slot(tdbb, relation, transaction, idx);

	if (index_id) {
		*index_id = idx->idx_id;
	}

	record_param primary, secondary;
	secondary.rpb_relation = relation;
	primary.rpb_relation   = relation;
	primary.rpb_number.setValue(BOF_NUMBER);
	//primary.getWindow(tdbb).win_flags = secondary.getWindow(tdbb).win_flags = 0; redundant

	const bool isODS11 = (dbb->dbb_ods_version >= ODS_VERSION11);
	const bool isDescending = (idx->idx_flags & idx_descending);
	const bool isPrimary = (idx->idx_flags & idx_primary);

	// hvlad: in ODS11 empty string and NULL values can have the same binary
	// representation in index keys. BTR can distinguish it by the key_length
	// but SORT module currently don't take it into account. Therefore add to
	// the index key one byte prefix with 0 for NULL value and 1 for not-NULL
	// value to produce right sorting.
	// BTR\fast_load will remove this one byte prefix from the index key.
	// Note that this is necessary only for single-segment ascending indexes
	// and only for ODS11 and higher.

	const int nullIndLen = isODS11 && !isDescending && (idx->idx_count == 1) ? 1 : 0;
	const USHORT key_length = ROUNDUP(BTR_key_length(tdbb, relation, idx) + nullIndLen, sizeof(SINT64));

	const USHORT max_key_size =
		isODS11 ? MAX_KEY_LIMIT : MAX_KEY_PRE_ODS11;

	if (key_length >= max_key_size)
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_keytoobig) << Arg::Str(index_name));
	}

	RecordStack stack;
	const UCHAR pad = isDescending ? -1 : 0;

	index_fast_load ifl_data;
	ifl_data.ifl_duplicates = 0;
	ifl_data.ifl_key_length = key_length;

	bool key_is_null = false;

	sort_key_def key_desc[2];
	// Key sort description
	key_desc[0].skd_dtype = SKD_bytes;
	key_desc[0].skd_flags = SKD_ascending;
	key_desc[0].skd_length = key_length;
	key_desc[0].skd_offset = 0;
	key_desc[0].skd_vary_offset = 0;
	// RecordNumber sort description
	key_desc[1].skd_dtype = SKD_int64;
	key_desc[1].skd_flags = SKD_ascending;
	key_desc[1].skd_length = sizeof(RecordNumber);
	key_desc[1].skd_offset = key_length;
	key_desc[1].skd_vary_offset = 0;

	FPTR_REJECT_DUP_CALLBACK callback = (idx->idx_flags & idx_unique) ? duplicate_key : NULL;
	void* callback_arg = (idx->idx_flags & idx_unique) ? &ifl_data : NULL;

	sort_context* sort_handle =
		SORT_init(dbb, &transaction->tra_sorts, key_length + sizeof(index_sort_record),
				  2, 1, key_desc, callback, callback_arg);

	try {

	jrd_rel* partner_relation = 0;
	USHORT partner_index_id = 0;
	if (idx->idx_flags & idx_foreign)
	{
		if (!MET_lookup_partner(tdbb, relation, idx, index_name)) {
			BUGCHECK(173);		// msg 173 referenced index description not found
		}
		partner_relation = MET_relation(tdbb, idx->idx_primary_relation);
		partner_index_id = idx->idx_primary_index;
	}

	// Checkout a garbage collect record block for fetching data.

	Record* gc_record = VIO_gc_record(tdbb, relation);

	// Unless this is the only attachment or a database restore, worry about
	// preserving the page working sets of other attachments.
	Attachment* attachment = tdbb->getAttachment();
	if (attachment && (attachment != dbb->dbb_attachments || attachment->att_next))
	{
		if (attachment->att_flags & ATT_gbak_attachment ||
			DPM_data_pages(tdbb, relation) > (SLONG) dbb->dbb_bcb->bcb_count)
		{
			primary.getWindow(tdbb).win_flags = secondary.getWindow(tdbb).win_flags = WIN_large_scan;
			primary.rpb_org_scans = secondary.rpb_org_scans = relation->rel_scan_count++;
		}
	}

	// Loop thru the relation computing index keys.  If there are old versions, find them, too.
	temporary_key key;
	while (DPM_next(tdbb, &primary, LCK_read,
#ifdef SCROLLABLE_CURSORS
		false,
#endif
		false))
	{
		if (!VIO_garbage_collect(tdbb, &primary, transaction))
			continue;
		if (primary.rpb_flags & rpb_deleted)
			CCH_RELEASE(tdbb, &primary.getWindow(tdbb));
		else
		{
			primary.rpb_record = gc_record;
			VIO_data(tdbb, &primary, dbb->dbb_permanent);
			gc_record = primary.rpb_record;
			stack.push(primary.rpb_record);
		}
		secondary.rpb_page = primary.rpb_b_page;
		secondary.rpb_line = primary.rpb_b_line;
		secondary.rpb_prior = primary.rpb_prior;
		while (secondary.rpb_page)
		{
			if (!DPM_fetch(tdbb, &secondary, LCK_read))
				break;			// must be garbage collected
			secondary.rpb_record = NULL;
			VIO_data(tdbb, &secondary, tdbb->getDefaultPool());
			stack.push(secondary.rpb_record);
			secondary.rpb_page = secondary.rpb_b_page;
			secondary.rpb_line = secondary.rpb_b_line;
		}

		while (stack.hasData())
		{
			Record* record = stack.pop();

			// If foreign key index is being defined, make sure foreign
			// key definition will not be violated

			if (idx->idx_flags & idx_foreign)
			{
				idx_null_state null_state;
				// find out if there is a null segment by faking uniqueness --
				// if there is one, don't bother to check the primary key

				if (!(idx->idx_flags & idx_unique))
				{
					idx->idx_flags |= idx_unique;
					result = BTR_key(tdbb, relation, record, idx, &key, &null_state, false);
					idx->idx_flags &= ~idx_unique;
				}
				else
				{
					result = BTR_key(tdbb, relation, record, idx, &key, &null_state, false);
				}

				if (result == idx_e_ok && null_state == idx_nulls_none)
				{
					result = check_partner_index(tdbb, relation, record, transaction, idx,
												 partner_relation, partner_index_id);
				}
			}

			if (result == idx_e_ok)
			{
				idx_null_state null_state;
				result = BTR_key(tdbb, relation, record, idx, &key, &null_state, false);

				if (result == idx_e_ok)
				{
					if (isPrimary && null_state != idx_nulls_none)
					{
						fb_assert(key.key_null_segment < idx->idx_count);

						const USHORT bad_id = idx->idx_rpt[key.key_null_segment].idx_field;
						const jrd_fld *bad_fld = MET_get_field(relation, bad_id);

						ERR_post(Arg::Gds(isc_not_valid) << Arg::Str(bad_fld->fld_name) <<
															Arg::Str(NULL_STRING_MARK));
					}

					key_is_null = (null_state == idx_nulls_all);
				}
			}

			if (result != idx_e_ok)
			{
				do {
					if (record != gc_record)
						delete record;
				} while (stack.hasData() && (record = stack.pop()));
				gc_record->rec_flags &= ~REC_gc_active;
				if (primary.getWindow(tdbb).win_flags & WIN_large_scan)
					--relation->rel_scan_count;

				ERR_duplicate_error(result, relation, idx->idx_id, index_name);
			}

			if (key.key_length > key_length)
			{
				do {
					if (record != gc_record)
						delete record;
				} while (stack.hasData() && (record = stack.pop()));
				gc_record->rec_flags &= ~REC_gc_active;
				if (primary.getWindow(tdbb).win_flags & WIN_large_scan)
					--relation->rel_scan_count;
				ERR_post(Arg::Gds(isc_key_too_big));
			}

			UCHAR* p;
			SORT_put(tdbb, sort_handle, reinterpret_cast<ULONG**>(&p));

			// try to catch duplicates early

			if (ifl_data.ifl_duplicates > 0)
			{
				do {
					if (record != gc_record)
						delete record;
				} while (stack.hasData() && (record = stack.pop()));
				gc_record->rec_flags &= ~REC_gc_active;
				if (primary.getWindow(tdbb).win_flags & WIN_large_scan)
					--relation->rel_scan_count;
				ERR_post(Arg::Gds(isc_no_dup) << Arg::Str(index_name));
			}

			USHORT l = key.key_length;

			if (nullIndLen) {
				*p++ = (key.key_length == 0) ? 0 : 1;
			}
			if (l > 0)
			{
				memcpy(p, key.key_data, l);
				p += l;
			}
			if ( (l = key_length - nullIndLen - key.key_length) )
			{
				memset(p, pad, l);
				p += l;
			}
			index_sort_record* isr = (index_sort_record*) p;
			isr->isr_record_number = primary.rpb_number.getValue();
			isr->isr_key_length = key.key_length;
			isr->isr_flags = (stack.hasData() ? ISR_secondary : 0) | (key_is_null ? ISR_null : 0);
			if (record != gc_record)
				delete record;
		}

		if (--tdbb->tdbb_quantum < 0)
			JRD_reschedule(tdbb, 0, true);
	}

	gc_record->rec_flags &= ~REC_gc_active;
	if (primary.getWindow(tdbb).win_flags & WIN_large_scan)
		--relation->rel_scan_count;

	SORT_sort(tdbb, sort_handle);

	if (ifl_data.ifl_duplicates > 0) {
		ERR_post(Arg::Gds(isc_no_dup) << Arg::Str(index_name));
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		SORT_fini(sort_handle);
		ERR_punt();
	}

	BTR_create(tdbb, relation, idx, key_length, sort_handle, selectivity);

	if (ifl_data.ifl_duplicates > 0)
	{
		// we don't need SORT_fini() here, as it's called inside BTR_create()
		ERR_post(Arg::Gds(isc_no_dup) << Arg::Str(index_name));
	}

	if ((relation->rel_flags & REL_temp_conn) &&
		(relation->getPages(tdbb)->rel_instance_id != 0))
	{
		IndexLock* idx_lock = CMP_get_index_lock(tdbb, relation, idx->idx_id);
		if (idx_lock)
		{
			++idx_lock->idl_count;
			if (idx_lock->idl_count == 1) {
				LCK_lock(tdbb, idx_lock->idl_lock, LCK_SR, LCK_WAIT);
			}
		}
	}
}


IndexBlock* IDX_create_index_block(thread_db* tdbb, jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *	I D X _ c r e a t e _ i n d e x _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Create an index block and an associated
 *	lock block for the specified index.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	IndexBlock* index_block = FB_NEW(*dbb->dbb_permanent) IndexBlock();
	index_block->idb_id = id;

	// link the block in with the relation linked list

	index_block->idb_next = relation->rel_index_blocks;
	relation->rel_index_blocks = index_block;

	// create a shared lock for the index, to coordinate
	// any modification to the index so that the cached information
	// about the index will be discarded

	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock;
	index_block->idb_lock = lock;
	lock->lck_parent = dbb->dbb_lock;
	lock->lck_dbb = dbb;
	lock->lck_key.lck_long = (relation->rel_id << 16) | index_block->idb_id;
	lock->lck_length = sizeof(lock->lck_key.lck_long);
	lock->lck_type = LCK_expression;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_ast = index_block_flush;
	lock->lck_object = index_block;

	return index_block;
}


void IDX_delete_index(thread_db* tdbb, jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *	I D X _ d e l e t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Delete a single index.
 *
 **************************************/
	SET_TDBB(tdbb);

	signal_index_deletion(tdbb, relation, id);

	WIN window(get_root_page(tdbb, relation));
	CCH_FETCH(tdbb, &window, LCK_write, pag_root);

	const bool tree_exists = BTR_delete_index(tdbb, &window, id);

	if ((relation->rel_flags & REL_temp_conn) && (relation->getPages(tdbb)->rel_instance_id != 0) &&
		tree_exists)
	{
		IndexLock* idx_lock = CMP_get_index_lock(tdbb, relation, id);
		if (idx_lock)
		{
			if (!--idx_lock->idl_count) {
				LCK_release(tdbb, idx_lock->idl_lock);
			}
		}
	}
}


void IDX_delete_indices(thread_db* tdbb, jrd_rel* relation, RelationPages* relPages)
{
/**************************************
 *
 *	I D X _ d e l e t e _ i n d i c e s
 *
 **************************************
 *
 * Functional description
 *	Delete all known indices in preparation for deleting a
 *	complete relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	
	fb_assert(relPages->rel_index_root);

	WIN window(relPages->rel_pg_space_id, relPages->rel_index_root);
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_root);

	const bool is_temp = (relation->rel_flags & REL_temp_conn) && (relPages->rel_instance_id != 0);

	for (USHORT i = 0; i < root->irt_count; i++)
	{
		const bool tree_exists = BTR_delete_index(tdbb, &window, i);
		root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_root);

		if (is_temp && tree_exists)
		{
			IndexLock* idx_lock = CMP_get_index_lock(tdbb, relation, i);
			if (idx_lock)
			{
				if (!--idx_lock->idl_count) {
					LCK_release(tdbb, idx_lock->idl_lock);
				}
			}
		}
	}

	CCH_RELEASE(tdbb, &window);
}


idx_e IDX_erase(thread_db* tdbb, record_param* rpb,
				jrd_tra* transaction, jrd_rel** bad_relation, USHORT* bad_index)
{
/**************************************
 *
 *	I D X _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Check the various indices prior to an ERASE operation.
 *	If one is a primary key, check its partner for
 *	a duplicate record.
 *
 **************************************/
	index_desc idx;

	SET_TDBB(tdbb);

	idx_e error_code = idx_e_ok;
	idx.idx_id = idx_invalid;
	RelationPages* relPages = rpb->rpb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	while (BTR_next_index(tdbb, rpb->rpb_relation, transaction, &idx, &window))
	{
		if (idx.idx_flags & (idx_primary | idx_unique))
		{
			error_code = check_foreign_key(tdbb, rpb->rpb_record, rpb->rpb_relation, transaction,
										   &idx, bad_relation, bad_index);
			if (idx_e_ok != error_code)
			{
				CCH_RELEASE(tdbb, &window);
				break;
			}
		}
	}

	return error_code;
}


void IDX_garbage_collect(thread_db*			tdbb,
						 record_param*		rpb,
						 RecordStack& going,
						 RecordStack& staying)
{
/**************************************
 *
 *	I D X _ g a r b a g e _ c o l l e c t
 *
 **************************************
 *
 * Functional description
 *	Perform garbage collection for a bunch of records.  Scan
 *	through the indices defined for a relation.  Garbage collect
 *	each.
 *
 **************************************/
	index_desc idx;
	temporary_key key1, key2;

	SET_TDBB(tdbb);

	index_insertion insertion;
	insertion.iib_descriptor = &idx;
	insertion.iib_number = rpb->rpb_number;
	insertion.iib_relation = rpb->rpb_relation;
	insertion.iib_key = &key1;

	WIN window(get_root_page(tdbb, rpb->rpb_relation));

	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_root);

	for (USHORT i = 0; i < root->irt_count; i++)
	{
		if (BTR_description(tdbb, rpb->rpb_relation, root, &idx, i))
		{
			for (RecordStack::iterator stack1(going); stack1.hasData(); ++stack1)
			{
				Record* rec1 = stack1.object();
				BTR_key(tdbb, rpb->rpb_relation, rec1, &idx, &key1, 0, false);

				// Cancel index if there are duplicates in the remaining records

				RecordStack::iterator stack2(stack1);
				for (++stack2; stack2.hasData(); ++stack2)
				{
					Record* rec2 = stack2.object();
					BTR_key(tdbb, rpb->rpb_relation, rec2, &idx, &key2, 0, false);
					if (key_equal(&key1, &key2))
						break;
				}
				if (stack2.hasData())
					continue;

				// Make sure the index doesn't exist in any record remaining

				RecordStack::iterator stack3(staying);
				for (; stack3.hasData(); ++stack3)
				{
					Record* rec3 = stack3.object();
					BTR_key(tdbb, rpb->rpb_relation, rec3, &idx, &key2, 0, false);
					if (key_equal(&key1, &key2))
						break;
				}
				if (stack3.hasData())
					continue;

				// Get rid of index node

				BTR_remove(tdbb, &window, &insertion);
				root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_root);
				if (stack1.hasMore(1))
				{
					BTR_description(tdbb, rpb->rpb_relation, root, &idx, i);
				}
			}
		}
	}

	CCH_RELEASE(tdbb, &window);
}


idx_e IDX_modify(thread_db* tdbb,
				 record_param* org_rpb,
				 record_param* new_rpb,
				 jrd_tra* transaction, jrd_rel** bad_relation, USHORT* bad_index)
{
/**************************************
 *
 *	I D X _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Update the various indices after a MODIFY operation.  If a duplicate
 *	index is violated, return the index number.  If successful, return
 *	-1.
 *
 **************************************/
	SET_TDBB(tdbb);

	temporary_key key1;
	index_desc idx;

	index_insertion insertion;
	insertion.iib_relation = org_rpb->rpb_relation;
	insertion.iib_number = org_rpb->rpb_number;
	insertion.iib_key = &key1;
	insertion.iib_descriptor = &idx;
	insertion.iib_transaction = transaction;
	idx_e error_code = idx_e_ok;
	idx.idx_id = idx_invalid;

	RelationPages* relPages = org_rpb->rpb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	temporary_key key2;

	while (BTR_next_index(tdbb, org_rpb->rpb_relation, transaction, &idx, &window))
	{
		*bad_index = idx.idx_id;
		*bad_relation = new_rpb->rpb_relation;
		if ( (error_code =
			BTR_key(tdbb, new_rpb->rpb_relation, new_rpb->rpb_record, &idx, &key1, 0, false)) )
		{
			CCH_RELEASE(tdbb, &window);
			break;
		}
		BTR_key(tdbb, org_rpb->rpb_relation, org_rpb->rpb_record, &idx, &key2, 0, false);
		if (!key_equal(&key1, &key2))
		{
			if (( error_code = insert_key(tdbb, new_rpb->rpb_relation, new_rpb->rpb_record,
										  transaction, &window, &insertion, bad_relation,
										  bad_index)) )
			{
				return error_code;
			}
		}
	}

	return error_code;
}


idx_e IDX_modify_check_constraints(thread_db* tdbb,
								   record_param* org_rpb,
								   record_param* new_rpb,
								   jrd_tra* transaction,
								   jrd_rel** bad_relation, USHORT* bad_index)
{
/**************************************
 *
 *	I D X _ m o d i f y _ c h e c k _ c o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *	Check for foreign key constraint after a modify statement
 *
 **************************************/
	temporary_key key1, key2;

	SET_TDBB(tdbb);

	idx_e error_code = idx_e_ok;
	index_desc idx;
	idx.idx_id = idx_invalid;

	RelationPages* relPages = org_rpb->rpb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	// If relation's primary/unique keys have no dependencies by other
	// relations' foreign keys then don't bother cycling thru all index descriptions.

	if (!(org_rpb->rpb_relation->rel_flags & REL_check_partners) &&
		!(org_rpb->rpb_relation->rel_primary_dpnds.prim_reference_ids))
	{
			return error_code;
	}

	// Now check all the foreign key constraints. Referential integrity relation
	// could be established by primary key/foreign key or unique key/foreign key

	while (BTR_next_index(tdbb, org_rpb->rpb_relation, transaction, &idx, &window))
	{
		if (!(idx.idx_flags & (idx_primary | idx_unique)) ||
			!MET_lookup_partner(tdbb, org_rpb->rpb_relation, &idx, 0))
		{
				continue;
		}
		*bad_index = idx.idx_id;
		*bad_relation = new_rpb->rpb_relation;
		if ((error_code =
				BTR_key(tdbb, new_rpb->rpb_relation, new_rpb->rpb_record, &idx, &key1, 0, false)) ||
			(error_code =
				BTR_key(tdbb, org_rpb->rpb_relation, org_rpb->rpb_record, &idx, &key2, 0, false)))
		{
			CCH_RELEASE(tdbb, &window);
			break;
		}
		if (!key_equal(&key1, &key2))
		{
			error_code = check_foreign_key(tdbb, org_rpb->rpb_record, org_rpb->rpb_relation,
										   transaction, &idx, bad_relation, bad_index);
			if (idx_e_ok != error_code)
			{
				CCH_RELEASE(tdbb, &window);
				return error_code;
			}
		}
	}

	return error_code;
}


void IDX_statistics(thread_db* tdbb, jrd_rel* relation, USHORT id, SelectivityList& selectivity)
{
/**************************************
 *
 *	I D X _ s t a t i s t i c s
 *
 **************************************
 *
 * Functional description
 *	Scan index pages recomputing
 *	selectivity.
 *
 **************************************/

	SET_TDBB(tdbb);

	BTR_selectivity(tdbb, relation, id, selectivity);
}


idx_e IDX_store(thread_db* tdbb, record_param* rpb,
				jrd_tra* transaction, jrd_rel** bad_relation, USHORT* bad_index)
{
/**************************************
 *
 *	I D X _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Update the various indices after a STORE operation.  If a duplicate
 *	index is violated, return the index number.  If successful, return
 *	-1.
 *
 **************************************/
	SET_TDBB(tdbb);

	temporary_key key;
	index_desc idx;

	index_insertion insertion;
	insertion.iib_relation = rpb->rpb_relation;
	insertion.iib_number = rpb->rpb_number;
	insertion.iib_key = &key;
	insertion.iib_descriptor = &idx;
	insertion.iib_transaction = transaction;

	idx_e error_code = idx_e_ok;
	idx.idx_id = idx_invalid;

	RelationPages* relPages = rpb->rpb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	while (BTR_next_index(tdbb, rpb->rpb_relation, transaction, &idx, &window))
	{
		*bad_index = idx.idx_id;
		*bad_relation = rpb->rpb_relation;
		if ( (error_code = BTR_key(tdbb, rpb->rpb_relation, rpb->rpb_record, &idx, &key, 0, false)) )
		{
			CCH_RELEASE(tdbb, &window);
			break;
		}
		if ( (error_code = insert_key(tdbb, rpb->rpb_relation, rpb->rpb_record, transaction,
									  &window, &insertion, bad_relation, bad_index)) )
		{
			return error_code;
		}
	}

	return error_code;
}


static idx_e check_duplicates(thread_db* tdbb,
							  Record* record,
							  index_desc* record_idx,
							  index_insertion* insertion, jrd_rel* relation_2)
{
/**************************************
 *
 *	c h e c k _ d u p l i c a t e s
 *
 **************************************
 *
 * Functional description
 *	Make sure there aren't any active duplicates for
 *	a unique index or a foreign key.
 *
 **************************************/
	DSC desc1, desc2;

	SET_TDBB(tdbb);

	idx_e result = idx_e_ok;
	index_desc* insertion_idx = insertion->iib_descriptor;
	record_param rpb, old_rpb;
	rpb.rpb_relation = insertion->iib_relation;
	rpb.rpb_record = NULL;
	// rpb.getWindow(tdbb).win_flags = 0; redundant.

	old_rpb.rpb_relation = insertion->iib_relation;
	old_rpb.rpb_record = NULL;

	jrd_rel* const relation_1 = insertion->iib_relation;
	Firebird::HalfStaticArray<UCHAR, 256> tmp;
	RecordBitmap::Accessor accessor(insertion->iib_duplicates);

	ThreadStatusGuard local_status(tdbb);

	if (accessor.getFirst())
	do {
		bool has_old_values;
		const bool is_fk = (record_idx->idx_flags & idx_foreign) != 0;

		rpb.rpb_number.setValue(accessor.current());

		if (rpb.rpb_number != insertion->iib_number &&
			VIO_get_current(tdbb, /*&old_rpb,*/ &rpb, insertion->iib_transaction, tdbb->getDefaultPool(),
							is_fk, has_old_values) )
		{
			// dimitr: we shouldn't ignore status exceptions which take place
			//		   inside the lock manager. Namely, they are: isc_deadlock,
			//		   isc_lock_conflict, isc_lock_timeout. Otherwise we may
			//		   have logically corrupted database as a result. If any
			//		   of the mentioned errors appeared, it means that there's
			//		   an active transaction out there which has modified our
			//		   record. For "nowait" transaction, it means we have
			//		   an update conflict.
			//
			// was: if (rpb.rpb_flags & rpb_deleted) {
			//
			// P.S. I think the check for a status vector should be enough,
			//      but for sure let's keep the old one as well.
			//														2003.05.27

			const bool lock_error =
				(tdbb->tdbb_status_vector[1] == isc_deadlock ||
				tdbb->tdbb_status_vector[1] == isc_lock_conflict ||
				tdbb->tdbb_status_vector[1] == isc_lock_timeout);
			// the above errors are not thrown but returned silently

			if (lock_error)
			{
				fb_utils::init_status(tdbb->tdbb_status_vector);
			}

			if (rpb.rpb_flags & rpb_deleted || lock_error)
			{
				result = idx_e_duplicate;
				break;
			}

			const bool has_cur_values = !(rpb.rpb_flags & rpb_deleted);
			if (!has_cur_values && !has_old_values)
			{
				result = idx_e_duplicate;
				break;
			}

			// check the values of the fields in the record being inserted with the
			// record retrieved -- for unique indexes the insertion index and the
			// record index are the same, but for foreign keys they are different

			if (record_idx->idx_flags & idx_expressn)
			{
				bool flag_idx;
				const dsc* desc_idx = BTR_eval_expression(tdbb, record_idx, record, flag_idx);

				// hvlad: BTR_eval_expression call EVL_expr which returns impure->vlu_desc.
				// Since record_idx and insertion_idx are the same indexes second call to
				// BTR_eval_expression will overwrite value from first call. So we must
				// save first result into another dsc

				desc1 = *desc_idx;
				const USHORT idx_dsc_length = record_idx->idx_expression_desc.dsc_length;
				desc1.dsc_address = tmp.getBuffer(idx_dsc_length);
				fb_assert(desc_idx->dsc_length <= idx_dsc_length);
				memmove(desc1.dsc_address, desc_idx->dsc_address, desc_idx->dsc_length);

				bool flag_rec = false;
				const dsc* desc_rec = has_cur_values ?
					BTR_eval_expression(tdbb, insertion_idx, rpb.rpb_record, flag_rec) : NULL;

				const bool equal_cur = has_cur_values && flag_rec && flag_idx &&
					(MOV_compare(desc_rec, &desc1) == 0);

				if (!is_fk && equal_cur)
				{
					result = idx_e_duplicate;
					break;
				}

				if (has_old_values)
				{
					desc_rec = BTR_eval_expression(tdbb, insertion_idx, old_rpb.rpb_record, flag_rec);

					const bool equal_old = flag_rec && flag_idx &&
						(MOV_compare(desc_rec, &desc1) == 0);

					if (is_fk)
					{
						if (equal_cur && equal_old)
						{
							result = idx_e_duplicate;
							break;
						}
					}
					else
					{
						if (equal_cur || equal_old)
						{
							result = idx_e_duplicate;
							break;
						}
					}
				}
			}
			else
			{
				bool all_nulls = true;
				USHORT i;
				for (i = 0; i < insertion_idx->idx_count; i++)
				{
					bool flag_cur = false;
					USHORT field_id = record_idx->idx_rpt[i].idx_field;
					const bool flag_idx = EVL_field(relation_2, record, field_id, &desc2);

					if (has_cur_values)
					{
						field_id = insertion_idx->idx_rpt[i].idx_field;
						// In order to "map a null to a default" value (in EVL_field()),
						// the relation block is referenced.
						// Reference: Bug 10116, 10424
						flag_cur = EVL_field(relation_1, rpb.rpb_record, field_id, &desc1);
					}

					const bool not_equal_cur = !has_cur_values ||
						has_cur_values &&
							( (flag_cur != flag_idx) || (flag_cur && (MOV_compare(&desc1, &desc2) != 0)) );

					if ((is_fk || !has_old_values) && not_equal_cur)
						break;

					if (has_old_values)
					{
						field_id = insertion_idx->idx_rpt[i].idx_field;
						const bool flag_old =
							EVL_field(relation_1, old_rpb.rpb_record, field_id, &desc1);

						const bool not_equal_old =
							(flag_old != flag_idx || (flag_cur && (MOV_compare(&desc1, &desc2) != 0)) );

						if (is_fk)
						{
							if (not_equal_cur || not_equal_old)
								break;
						}
						else
						{
							if (not_equal_cur && not_equal_old)
								break;
						}
					}

					all_nulls = all_nulls && !flag_cur && !flag_idx;
				}

				if (i >= insertion_idx->idx_count && !all_nulls)
				{
					result = idx_e_duplicate;
					break;
				}
			}
		}
	} while (accessor.getNext());

	delete rpb.rpb_record;
	delete old_rpb.rpb_record;

	if (local_status[1]) {
		local_status.copyToOriginal();
	}

	return result;
}


static idx_e check_foreign_key(thread_db* tdbb,
							   Record* record,
							   jrd_rel* relation,
							   jrd_tra* transaction,
							   index_desc* idx,
							   jrd_rel** bad_relation,
							   USHORT* bad_index)
{
/**************************************
 *
 *	c h e c k _ f o r e i g n _ k e y
 *
 **************************************
 *
 * Functional description
 *	The passed index participates in a foreign key.
 *	Check the passed record to see if a corresponding
 *	record appears in the partner index.
 *
 **************************************/
	SET_TDBB(tdbb);

	idx_e result = idx_e_ok;

	if (!MET_lookup_partner(tdbb, relation, idx, 0)) {
		return result;
	}

	jrd_rel* partner_relation;
	USHORT index_id = 0;
	if (idx->idx_flags & idx_foreign)
	{
		partner_relation = MET_relation(tdbb, idx->idx_primary_relation);
		index_id = idx->idx_primary_index;
		result = check_partner_index(tdbb, relation, record, transaction, idx,
									 partner_relation, index_id);
	}
	else if (idx->idx_flags & (idx_primary | idx_unique))
	{
		for (int index_number = 0;
			index_number < (int) idx->idx_foreign_primaries->count();
			index_number++)
		{
			if (idx->idx_id != (*idx->idx_foreign_primaries)[index_number])
			{
				continue;
			}
			partner_relation = MET_relation(tdbb, (*idx->idx_foreign_relations)[index_number]);

			index_id = (*idx->idx_foreign_indexes)[index_number];

			if ((relation->rel_flags & REL_temp_conn) && (partner_relation->rel_flags & REL_temp_tran))
			{
				jrd_rel::RelPagesSnapshot pagesSnapshot(tdbb, partner_relation);
				partner_relation->fillPagesSnapshot(pagesSnapshot, true);

				for (size_t i = 0; i < pagesSnapshot.getCount(); i++)
				{
					RelationPages* partnerPages = pagesSnapshot[i];
					tdbb->tdbb_temp_traid = partnerPages->rel_instance_id;
					if ( (result = check_partner_index(tdbb, relation, record,
								transaction, idx, partner_relation, index_id)) )
					{
						break;
					}
				}

				tdbb->tdbb_temp_traid = 0;
				if (result)
					break;
			}
			else
			{
				if ( (result = check_partner_index(tdbb, relation, record,
							transaction, idx, partner_relation, index_id)) )
				{
					break;
				}
			}
		}
	}

	if (result)
	{
		if (idx->idx_flags & idx_foreign)
		{
			*bad_relation = relation;
			*bad_index = idx->idx_id;
		}
		else
		{
			*bad_relation = partner_relation;
			*bad_index = index_id;
		}
	}

	return result;
}


static idx_e check_partner_index(thread_db* tdbb,
								 jrd_rel* relation,
								 Record* record,
								 jrd_tra* transaction,
								 index_desc* idx,
								 jrd_rel* partner_relation,
								 USHORT index_id)
{
/**************************************
 *
 *	c h e c k _ p a r t n e r _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	The passed index participates in a foreign key.
 *	Check the passed record to see if a corresponding
 *	record appears in the partner index.
 *
 **************************************/
	SET_TDBB(tdbb);

	idx_e result = idx_e_ok;

	// get the index root page for the partner relation

	WIN window(get_root_page(tdbb, partner_relation));
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_root);

	// get the description of the partner index

	index_desc partner_idx;
	if (!BTR_description(tdbb, partner_relation, root, &partner_idx, index_id))
		BUGCHECK(175);			// msg 175 partner index description not found

	bool starting = false;
	USHORT segment;

	if (!(partner_idx.idx_flags & idx_unique))
	{
		const index_desc::idx_repeat* idx_desc = partner_idx.idx_rpt;
		for (segment = 0; segment < partner_idx.idx_count; ++segment, ++idx_desc)
		{
			if (idx_desc->idx_itype >= idx_first_intl_string)
			{
				TextType* textType = INTL_texttype_lookup(tdbb, INTL_INDEX_TO_TEXT(idx_desc->idx_itype));

				if (textType->getFlags() & TEXTTYPE_SEPARATE_UNIQUE)
				{
					starting = true;
					++segment;
					break;
				}
			}
		}
	}
	else
		segment = idx->idx_count;

	// get the key in the original index
	// AB: Fake the index to be an unique index, because the INTL makes
	// different keys depending on unique index or not.
	// The key build should be exactly the same as stored in the
	// unique index, because a comparison is done on both keys.
	index_desc tmpIndex = *idx;
	// ASF: Was incorrect to verify broken foreign keys.
	// Should not use an unique key to search a non-unique index.
	// tmpIndex.idx_flags |= idx_unique;
	tmpIndex.idx_flags = (tmpIndex.idx_flags & ~idx_unique) | (partner_idx.idx_flags & idx_unique);
	temporary_key key;
	result = BTR_key(tdbb, relation, record, &tmpIndex, &key, 0, starting, segment);
	CCH_RELEASE(tdbb, &window);

	// now check for current duplicates

	if (result == idx_e_ok)
	{
		// fill out a retrieval block for the purpose of
		// generating a bitmap of duplicate records

		IndexRetrieval retrieval;
		MOVE_CLEAR(&retrieval, sizeof(IndexRetrieval));
		//retrieval.blk_type = type_irb;
		retrieval.irb_index = partner_idx.idx_id;
		memcpy(&retrieval.irb_desc, &partner_idx, sizeof(retrieval.irb_desc));
		retrieval.irb_generic = irb_equality | (starting ? irb_starting : 0);
		retrieval.irb_relation = partner_relation;
		retrieval.irb_key = &key;
		retrieval.irb_upper_count = retrieval.irb_lower_count = segment;

		if (starting && segment < partner_idx.idx_count)
			retrieval.irb_generic |= irb_partial;

		if (partner_idx.idx_flags & idx_descending) {
			retrieval.irb_generic |= irb_descending;
		}
		if ((idx->idx_flags & idx_descending) != (partner_idx.idx_flags & idx_descending))
		{
			BTR_complement_key(&key);
		}

		RecordBitmap* bitmap = NULL;
		BTR_evaluate(tdbb, &retrieval, &bitmap, NULL);

		// if there is a bitmap, it means duplicates were found

		if (bitmap)
		{
			index_insertion insertion;
			insertion.iib_descriptor = &partner_idx;
			insertion.iib_relation = partner_relation;
			insertion.iib_number.setValue(BOF_NUMBER);
			insertion.iib_duplicates = bitmap;
			insertion.iib_transaction = transaction;
			result = check_duplicates(tdbb, record, idx, &insertion, relation);
			if (idx->idx_flags & (idx_primary | idx_unique))
				result = result ? idx_e_foreign_references_present : idx_e_ok;
			if (idx->idx_flags & idx_foreign)
				result = result ? idx_e_ok : idx_e_foreign_target_doesnt_exist;
			delete bitmap;
		}
		else if (idx->idx_flags & idx_foreign) {
			result = idx_e_foreign_target_doesnt_exist;
		}
	}

	return result;
}


static bool duplicate_key(const UCHAR* record1, const UCHAR* record2, void* ifl_void)
{
/**************************************
 *
 *	d u p l i c a t e _ k e y
 *
 **************************************
 *
 * Functional description
 *	Callback routine for duplicate keys during index creation.  Just
 *	bump a counter.
 *
 **************************************/
	index_fast_load* ifl_data = static_cast<index_fast_load*>(ifl_void);
	const index_sort_record* rec1 = (index_sort_record*) (record1 + ifl_data->ifl_key_length);
	const index_sort_record* rec2 = (index_sort_record*) (record2 + ifl_data->ifl_key_length);

	if (!(rec1->isr_flags & (ISR_secondary | ISR_null)) &&
		!(rec2->isr_flags & (ISR_secondary | ISR_null)))
	{
		++ifl_data->ifl_duplicates;
	}

	return false;
}


static PageNumber get_root_page(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *	g e t _ r o o t _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Find the root page for a relation.
 *
 **************************************/
	SET_TDBB(tdbb);

	RelationPages* relPages = relation->getPages(tdbb);
	SLONG page = relPages->rel_index_root;
	if (!page)
	{
		DPM_scan_pages(tdbb);
		page = relPages->rel_index_root;
	}

	return PageNumber(relPages->rel_pg_space_id, page);
}


static int index_block_flush(void* ast_object)
{
/**************************************
 *
 *	i n d e x _ b l o c k _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	An exclusive lock has been requested on the
 *	index block.  The information in the cached
 *	index block is no longer valid, so clear it
 *	out and release the lock.
 *
 **************************************/
	IndexBlock* index_block = static_cast<IndexBlock*>(ast_object);

	try
	{
		Lock* lock = index_block->idb_lock;
		Database* dbb = lock->lck_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(lock->lck_attachment);

		release_index_block(tdbb, index_block);
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static idx_e insert_key(thread_db* tdbb,
						jrd_rel* relation,
						Record* record,
						jrd_tra* transaction,
						WIN * window_ptr,
						index_insertion* insertion,
						jrd_rel** bad_relation,
						USHORT* bad_index)
{
/**************************************
 *
 *	i n s e r t _ k e y
 *
 **************************************
 *
 * Functional description
 *	Insert a key in the index.
 *	If this is a unique index, check for active duplicates.
 *	If this is a foreign key, check for duplicates in the
 *	primary key index.
 *
 **************************************/
	SET_TDBB(tdbb);

	idx_e result = idx_e_ok;
	index_desc* idx = insertion->iib_descriptor;

	// Insert the key into the index.  If the index is unique, btr will keep track of duplicates.

	insertion->iib_duplicates = NULL;
	BTR_insert(tdbb, window_ptr, insertion);

	if (insertion->iib_duplicates)
	{
		result = check_duplicates(tdbb, record, idx, insertion, NULL);
		delete insertion->iib_duplicates;
		insertion->iib_duplicates = 0;
	}

	if (result != idx_e_ok) {
		return result;
	}

	// if we are dealing with a foreign key index,
	// check for an insert into the corresponding primary key index
	if (idx->idx_flags & idx_foreign)
	{
		// find out if there is a null segment by faking uniqueness --
		// if there is one, don't bother to check the primary key

		idx->idx_flags |= idx_unique;
		CCH_FETCH(tdbb, window_ptr, LCK_read, pag_root);
		temporary_key key;
		idx_null_state null_state;
		result = BTR_key(tdbb, relation, record, idx, &key, &null_state, false);
		CCH_RELEASE(tdbb, window_ptr);
		idx->idx_flags &= ~idx_unique;
		if (null_state == idx_nulls_none)
		{
			result = check_foreign_key(tdbb, record, insertion->iib_relation,
									   transaction, idx, bad_relation, bad_index);
		}
	}

	return result;
}


void IDX_modify_flag_uk_modified(thread_db* tdbb,
								 record_param* org_rpb,
								 record_param* new_rpb,
								 jrd_tra* transaction)
{
/**************************************
 *
 *	I D X _ m o d i f y _ f l a g _ u k _ m o d i f i e d
 *
 **************************************
 *
 * Functional description
 *	Set record flag if key field value was changed by this update or
 *  if this is second update of this record in the same transaction and
 *  flag is already set by one of the previous update.
 *
 **************************************/

	SET_TDBB(tdbb);

	if ((org_rpb->rpb_flags & rpb_uk_modified) &&
		(org_rpb->rpb_transaction_nr == new_rpb->rpb_transaction_nr))
	{
		new_rpb->rpb_flags |= rpb_uk_modified;
		return;
	}

	RelationPages* relPages = org_rpb->rpb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	DSC desc1, desc2;
	index_desc idx;
	idx.idx_id = idx_invalid;

	while (BTR_next_index(tdbb, org_rpb->rpb_relation, transaction, &idx, &window))
	{
		if (!(idx.idx_flags & (idx_primary | idx_unique)) ||
			!MET_lookup_partner(tdbb, org_rpb->rpb_relation, &idx, 0))
		{
			continue;
		}

		const index_desc::idx_repeat* idx_desc = idx.idx_rpt;

		for (USHORT i = 0; i < idx.idx_count; i++, idx_desc++)
		{
			const bool flag_org = EVL_field(org_rpb->rpb_relation, org_rpb->rpb_record, idx_desc->idx_field, &desc1);
			const bool flag_new = EVL_field(new_rpb->rpb_relation, new_rpb->rpb_record, idx_desc->idx_field, &desc2);

			if (flag_org != flag_new || MOV_compare(&desc1, &desc2) != 0)
			{
				new_rpb->rpb_flags |= rpb_uk_modified;
				CCH_RELEASE(tdbb, &window);
				return;
			}
		}
	}
}


static bool key_equal(const temporary_key* key1, const temporary_key* key2)
{
/**************************************
 *
 *	k e y _ e q u a l
 *
 **************************************
 *
 * Functional description
 *	Compare two keys for equality.
 *
 **************************************/
	const USHORT l = key1->key_length;
	return (l == key2->key_length && !memcmp(key1->key_data, key2->key_data, l));
}


static void release_index_block(thread_db* tdbb, IndexBlock* index_block)
{
/**************************************
 *
 *	r e l e a s e _ i n d e x _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Release index block structure.
 *
 **************************************/
	if (index_block->idb_expression_request)
	{
		CMP_release(tdbb, index_block->idb_expression_request);
	}

	index_block->idb_expression_request = NULL;
	index_block->idb_expression = NULL;
	MOVE_CLEAR(&index_block->idb_expression_desc, sizeof(dsc));

	LCK_release(tdbb, index_block->idb_lock);
}


static void signal_index_deletion(thread_db* tdbb, jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *	s i g n a l _ i n d e x _ d e l e t i o n
 *
 **************************************
 *
 * Functional description
 *	On delete of an index, force all
 *	processes to get rid of index info.
 *
 **************************************/
	IndexBlock* index_block;
	Lock* lock = NULL;

	SET_TDBB(tdbb);

	// get an exclusive lock on the associated index
	// block (if it exists) to make sure that all other
	// processes flush their cached information about this index

	for (index_block = relation->rel_index_blocks; index_block; index_block = index_block->idb_next)
	{
		if (index_block->idb_id == id)
		{
			lock = index_block->idb_lock;
			break;
		}
	}

	// if one didn't exist, create it

	if (!index_block)
	{
		index_block = IDX_create_index_block(tdbb, relation, id);
		lock = index_block->idb_lock;
	}

	// signal other processes to clear out the index block

	if (lock->lck_physical == LCK_SR) {
		LCK_convert(tdbb, lock, LCK_EX, LCK_WAIT);
	}
	else {
		LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT);
	}

	release_index_block(tdbb, index_block);
}
