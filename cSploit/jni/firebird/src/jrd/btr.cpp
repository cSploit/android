/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		btr.cpp
 *	DESCRIPTION:	B-tree management code
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
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "memory_routines.h"
#include "../common/classes/vector.h"
#include "../common/classes/VaryStr.h"
#include <stdio.h>
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/val.h"
#include "../jrd/btr.h"
#include "../jrd/btn.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/intl.h"
#include "gen/iberror.h"
#include "../jrd/lck.h"
#include "../jrd/cch.h"
#include "../jrd/sort.h"
#include "../common/gdsassert.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/pcmet_proto.h"
#include "../jrd/tra_proto.h"

using namespace Jrd;
using namespace Ods;
using namespace Firebird;

//#define DEBUG_BTR_SPLIT

const int MAX_LEVELS	= 16;


#define OVERSIZE	(MAX_PAGE_SIZE + BTN_PAGE_SIZE + MAX_KEY + sizeof (SLONG) - 1) / sizeof (SLONG)

// END_LEVEL (~0) is choosen here as a unknown/none value, because it's
// already reserved as END_LEVEL marker for page number and record number.
//
// NO_VALUE_PAGE and NO_VALUE are the same constant, but with different size
// Sign-extension mechanizm guaranties that they may be compared to each other safely
const ULONG NO_VALUE_PAGE = END_LEVEL;
const RecordNumber NO_VALUE(END_LEVEL);

// A split page will never have the number 0, because that's the value
// of the main page.
const ULONG NO_SPLIT	= 0;

// Thresholds for determing of a page should be garbage collected
// Garbage collect if page size is below GARBAGE_COLLECTION_THRESHOLD
#define GARBAGE_COLLECTION_BELOW_THRESHOLD	(dbb->dbb_page_size / 4)
// Garbage collect only if new merged page will
// be lower as GARBAGE_COLLECTION_NEW_PAGE_MAX_THRESHOLD
// 256 is the old maximum possible key_length.
#define GARBAGE_COLLECTION_NEW_PAGE_MAX_THRESHOLD	((dbb->dbb_page_size - 256))

//Debug page numbers into log file
//#define DEBUG_BTR_PAGES

struct INT64_KEY
{
	double d_part;
	SSHORT s_part;
};

// I assume this wasn't done sizeof(INT64_KEY) on purpose, since alignment might affect it.
const size_t INT64_KEY_LENGTH = sizeof (double) + sizeof (SSHORT);

static const double pow10_table[] =
{
	1.e00, 1.e01, 1.e02, 1.e03, 1.e04, 1.e05, 1.e06, 1.e07, 1.e08, 1.e09,
	1.e10, 1.e11, 1.e12, 1.e13, 1.e14, 1.e15, 1.e16, 1.e17, 1.e18, 1.e19,
	1.e20, 1.e21, 1.e22, 1.e23, 1.e24, 1.e25, 1.e26, 1.e27, 1.e28, 1.e29,
	1.e30, 1.e31, 1.e32, 1.e33, 1.e34, 1.e35, 1.e36
};

#define powerof10(s) ((s) <= 0 ? pow10_table[-(s)] : 1. / pow10_table[-(s)])

static const struct	// Used in make_int64_key()
{
	FB_UINT64 limit;
	SINT64 factor;
	SSHORT scale_change;
} int64_scale_control[] =
{
	{ QUADCONST(922337203685470000), QUADCONST(1), 0 },
	{ QUADCONST(92233720368547000), QUADCONST(10), 1 },
	{ QUADCONST(9223372036854700), QUADCONST(100), 2 },
	{ QUADCONST(922337203685470), QUADCONST(1000), 3 },
	{ QUADCONST(92233720368548), QUADCONST(10000), 4 },
	{ QUADCONST(9223372036855), QUADCONST(100000), 5 },
	{ QUADCONST(922337203686), QUADCONST(1000000), 6 },
	{ QUADCONST(92233720369), QUADCONST(10000000), 7 },
	{ QUADCONST(9223372035), QUADCONST(100000000), 8 },
	{ QUADCONST(922337204), QUADCONST(1000000000), 9 },
	{ QUADCONST(92233721), QUADCONST(10000000000), 10 },
	{ QUADCONST(9223373), QUADCONST(100000000000), 11 },
	{ QUADCONST(922338), QUADCONST(1000000000000), 12 },
	{ QUADCONST(92234), QUADCONST(10000000000000), 13 },
	{ QUADCONST(9224), QUADCONST(100000000000000), 14 },
	{ QUADCONST(923), QUADCONST(1000000000000000), 15 },
	{ QUADCONST(93), QUADCONST(10000000000000000), 16 },
	{ QUADCONST(10), QUADCONST(100000000000000000), 17 },
	{ QUADCONST(1), QUADCONST(1000000000000000000), 18 },
	{ QUADCONST(0), QUADCONST(0), 0 }
};

/* The first four entries in the array int64_scale_control[] ends with the
 * limit having 0's in the end. This is to inhibit any rounding off that
 * DOUBLE precision can introduce. DOUBLE can easily store upto 92233720368547
 * uniquely. Values after this tend to round off to the upper limit during
 * division. Hence the ending with 0's so that values will be bunched together
 * in the same limit range and scale control for INT64 index temporary_key calculation.
 *
 * This part was changed as a fix for bug 10267. - bsriram 04-Mar-1999
 */

// enumerate the possible outcomes of deleting a node

enum contents {
	contents_empty = 0,
	contents_single,
	contents_below_threshold,
	contents_above_threshold
};

static ULONG add_node(thread_db*, WIN*, index_insertion*, temporary_key*, RecordNumber*,
					  ULONG*, ULONG*);
static void compress(thread_db*, const dsc*, temporary_key*, USHORT, bool, bool, USHORT);
static USHORT compress_root(thread_db*, index_root_page*);
static void copy_key(const temporary_key*, temporary_key*);
static contents delete_node(thread_db*, WIN*, UCHAR*);
static void delete_tree(thread_db*, USHORT, USHORT, PageNumber, PageNumber);
static DSC* eval(thread_db*, const ValueExprNode*, DSC*, bool*);
static ULONG fast_load(thread_db*, jrd_rel*, index_desc*, USHORT, AutoPtr<Sort>&, SelectivityList&);

static index_root_page* fetch_root(thread_db*, WIN*, const jrd_rel*, const RelationPages*);
static UCHAR* find_node_start_point(btree_page*, temporary_key*, UCHAR*, USHORT*,
									bool, bool, bool = false, RecordNumber = NO_VALUE);

static UCHAR* find_area_start_point(btree_page*, const temporary_key*, UCHAR*,
									USHORT*, bool, bool, RecordNumber = NO_VALUE);

static ULONG find_page(btree_page*, const temporary_key*, const index_desc*, RecordNumber = NO_VALUE,
					   bool = false);

static contents garbage_collect(thread_db*, WIN*, ULONG);
static void generate_jump_nodes(thread_db*, btree_page*, jumpNodeList*, USHORT,
								USHORT*, USHORT*, USHORT*);

static ULONG insert_node(thread_db*, WIN*, index_insertion*, temporary_key*,
						 RecordNumber*, ULONG*, ULONG*);

static INT64_KEY make_int64_key(SINT64, SSHORT);
#ifdef DEBUG_INDEXKEY
static void print_int64_key(SINT64, SSHORT, INT64_KEY);
#endif
static string print_key(thread_db*, jrd_rel*, index_desc*, Record*);
static contents remove_node(thread_db*, index_insertion*, WIN*);
static contents remove_leaf_node(thread_db*, index_insertion*, WIN*);
static bool scan(thread_db*, UCHAR*, RecordBitmap**, RecordBitmap*, index_desc*,
				 const IndexRetrieval*, USHORT, temporary_key*,
				 bool&, const temporary_key&);
static void update_selectivity(index_root_page*, USHORT, const SelectivityList&);
static void checkForLowerKeySkip(bool&, const bool, const IndexNode&, const temporary_key&,
								 const index_desc&, const IndexRetrieval*);

// BtrPageLock class

BtrPageGCLock::BtrPageGCLock(thread_db* tdbb)
	: Lock(tdbb, PageNumber::getLockLen(), LCK_btr_dont_gc)
{
}

BtrPageGCLock::~BtrPageGCLock()
{
	// assert in debug build
	fb_assert(!lck_id);

	// lck_id might be set only if exception occurs
	if (lck_id) {
		LCK_release(JRD_get_thread_data(), this);
	}
}

void BtrPageGCLock::disablePageGC(thread_db* tdbb, const PageNumber &page)
{
	page.getLockStr(lck_key.lck_string);
	LCK_lock(tdbb, this, LCK_read, LCK_WAIT);
}

void BtrPageGCLock::enablePageGC(thread_db* tdbb)
{
	LCK_release(tdbb, this);
}

bool BtrPageGCLock::isPageGCAllowed(thread_db* tdbb, const PageNumber& page)
{
	BtrPageGCLock lock(tdbb);
	page.getLockStr(lock.lck_key.lck_string);

	ThreadStatusGuard temp_status(tdbb);

	if (!LCK_lock(tdbb, &lock, LCK_write, LCK_NO_WAIT))
		return false;

	LCK_release(tdbb, &lock);
	return true;
}

// IndexErrorContext class

void IndexErrorContext::raise(thread_db* tdbb, idx_e result, Record* record)
{
	fb_assert(result != idx_e_ok);

	if (result == idx_e_conversion)
		ERR_punt();

	const MetaName& relationName = isLocationDefined ? m_location.relation->rel_name : m_relation->rel_name;
	const USHORT indexId = isLocationDefined ? m_location.indexId : m_index->idx_id;

	MetaName indexName(m_indexName), constraintName;

	if (indexName.isEmpty())
		MET_lookup_index(tdbb, indexName, relationName, indexId + 1);

	if (indexName.hasData())
		MET_lookup_cnstrt_for_index(tdbb, constraintName, indexName);
	else
		indexName = "***unknown***";

	const bool haveConstraint = constraintName.hasData();

	if (!haveConstraint)
		constraintName = "***unknown***";

	switch (result)
	{
	case idx_e_keytoobig:
		ERR_post_nothrow(Arg::Gds(isc_imp_exc) <<
						 Arg::Gds(isc_keytoobig) << Arg::Str(indexName));
		break;

	case idx_e_foreign_target_doesnt_exist:
		ERR_post_nothrow(Arg::Gds(isc_foreign_key) <<
						 Arg::Str(constraintName) << Arg::Str(relationName) <<
						 Arg::Gds(isc_foreign_key_target_doesnt_exist));
		break;

	case idx_e_foreign_references_present:
		ERR_post_nothrow(Arg::Gds(isc_foreign_key) <<
						 Arg::Str(constraintName) << Arg::Str(relationName) <<
						 Arg::Gds(isc_foreign_key_references_present));
		break;

	case idx_e_duplicate:
		if (haveConstraint)
		{
			ERR_post_nothrow(Arg::Gds(isc_unique_key_violation) <<
							 Arg::Str(constraintName) << Arg::Str(relationName));
		}
		else
			ERR_post_nothrow(Arg::Gds(isc_no_dup) << Arg::Str(indexName));
		break;

	default:
		fb_assert(false);
	}

	if (record)
	{
		const string keyString = print_key(tdbb, m_relation, m_index, record);
		if (keyString.hasData())
			ERR_post_nothrow(Arg::Gds(isc_idx_key_value) << Arg::Str(keyString));
	}

	ERR_punt();
}


USHORT BTR_all(thread_db* tdbb, jrd_rel* relation, IndexDescAlloc** csb_idx, RelationPages* relPages)
{
/**************************************
 *
 *	B T R _ a l l
 *
 **************************************
 *
 * Functional description
 *	Return descriptions of all indices for relation.  If there isn't
 *	a known index root, assume we were called during optimization
 *	and return no indices.
 *
 **************************************/
	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	WIN window(relPages->rel_pg_space_id, -1);

	index_root_page* root = fetch_root(tdbb, &window, relation, relPages);
	if (!root) {
		return 0;
	}

	delete *csb_idx;
	*csb_idx = FB_NEW_RPT(*tdbb->getDefaultPool(), root->irt_count) IndexDescAlloc();
	index_desc* buffer = (*csb_idx)->items;
	USHORT count = 0;
	for (USHORT i = 0; i < root->irt_count; i++)
	{
		if (BTR_description(tdbb, relation, root, &buffer[count], i)) {
			count++;
		}
	}

	CCH_RELEASE(tdbb, &window);
	return count;
}


void BTR_complement_key(temporary_key* key)
{
/**************************************
 *
 *	B T R _ c o m p l e m e n t _ k e y
 *
 **************************************
 *
 * Functional description
 *	Negate a key for descending index.
 *
 **************************************/
	UCHAR* p = key->key_data;
	for (const UCHAR* const end = p + key->key_length; p < end; p++) {
		*p ^= -1;
	}
}


void BTR_create(thread_db* tdbb,
				jrd_rel* relation,
				index_desc* idx,
				USHORT key_length,
				AutoPtr<Sort>& scb,
				SelectivityList& selectivity)
{
/**************************************
 *
 *	B T R _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Create a new index.
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Now that the index id has been checked out, create the index.
	idx->idx_root = fast_load(tdbb, relation, idx, key_length, scb, selectivity);

	// Index is created.  Go back to the index root page and update it to
	// point to the index.
	RelationPages* relPages = relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, relPages->rel_index_root);
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_root);
	CCH_MARK(tdbb, &window);
	root->irt_rpt[idx->idx_id].irt_root = idx->idx_root;
	root->irt_rpt[idx->idx_id].irt_flags &= ~irt_in_progress;
	update_selectivity(root, idx->idx_id, selectivity);

	CCH_RELEASE(tdbb, &window);
}


bool BTR_delete_index(thread_db* tdbb, WIN* window, USHORT id)
{
/**************************************
 *
 *	B T R _ d e l e t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Delete an index if it exists.
 *	Return true if index tree was there.
 *
 **************************************/
	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Get index descriptor.  If index doesn't exist, just leave.
	index_root_page* root = (index_root_page*) window->win_buffer;

	bool tree_exists = false;
	if (id >= root->irt_count) {
		CCH_RELEASE(tdbb, window);
	}
	else
	{
		index_root_page::irt_repeat* irt_desc = root->irt_rpt + id;
		CCH_MARK(tdbb, window);
		const PageNumber next(window->win_page.getPageSpaceID(), irt_desc->irt_root);
		tree_exists = (irt_desc->irt_root != 0);

		// remove the pointer to the top-level index page before we delete it
		irt_desc->irt_root = 0;
		irt_desc->irt_flags = 0;
		const PageNumber prior = window->win_page;
		const USHORT relation_id = root->irt_relation;

		CCH_RELEASE(tdbb, window);
		delete_tree(tdbb, relation_id, id, next, prior);
	}

	return tree_exists;
}


bool BTR_description(thread_db* tdbb, jrd_rel* relation, index_root_page* root, index_desc* idx,
					 USHORT id)
{
/**************************************
 *
 *	B T R _ d e s c r i p t i o n
 *
 **************************************
 *
 * Functional description
 *	See if index exists, and if so, pick up its description.
 *  Index id's must fit in a short - formerly a UCHAR.
 *
 **************************************/

	SET_TDBB(tdbb);
	//const Database* dbb = tdbb->getDatabase();

	if (id >= root->irt_count) {
		return false;
	}

	const index_root_page::irt_repeat* irt_desc = &root->irt_rpt[id];

	if (irt_desc->irt_root == 0) {
		return false;
	}

	idx->idx_id = id;
	idx->idx_root = irt_desc->irt_root;
	idx->idx_count = irt_desc->irt_keys;
	idx->idx_flags = irt_desc->irt_flags;
	idx->idx_runtime_flags = 0;
	idx->idx_foreign_primaries = NULL;
	idx->idx_foreign_relations = NULL;
	idx->idx_foreign_indexes = NULL;
	idx->idx_primary_relation = 0;
	idx->idx_primary_index = 0;
	idx->idx_expression = NULL;
	idx->idx_expression_statement = NULL;

	// pick up field ids and type descriptions for each of the fields
	const UCHAR* ptr = (UCHAR*) root + irt_desc->irt_desc;
	index_desc::idx_repeat* idx_desc = idx->idx_rpt;
	for (int i = 0; i < idx->idx_count; i++, idx_desc++)
	{
		const irtd* key_descriptor = (irtd*) ptr;
		idx_desc->idx_field = key_descriptor->irtd_field;
		idx_desc->idx_itype = key_descriptor->irtd_itype;
		idx_desc->idx_selectivity = key_descriptor->irtd_selectivity;
		ptr += sizeof(irtd);
	}
	idx->idx_selectivity = idx_desc->idx_selectivity;

	if (idx->idx_flags & idx_expressn)
	{
		PCMET_lookup_index(tdbb, relation, idx);
		fb_assert(idx->idx_expression != NULL);
	}

	return true;
}


DSC* BTR_eval_expression(thread_db* tdbb, index_desc* idx, Record* record, bool& notNull)
{
	SET_TDBB(tdbb);
	fb_assert(idx->idx_expression != NULL);

	jrd_req* const org_request = tdbb->getRequest();
	jrd_req* const expr_request = idx->idx_expression_statement->findRequest(tdbb);

	fb_assert(expr_request->req_caller == NULL);
	expr_request->req_caller = org_request;

	TRA_attach_request(tdbb->getTransaction(), expr_request);
	tdbb->setRequest(expr_request);

	fb_assert(expr_request->req_transaction);

	expr_request->req_rpb[0].rpb_record = record;
	expr_request->req_flags &= ~req_null;

	DSC* result = NULL;

	try
	{
		Jrd::ContextPoolHolder context(tdbb, expr_request->req_pool);

		expr_request->req_timestamp = org_request ?
			org_request->req_timestamp : Firebird::TimeStamp::getCurrentTimeStamp();

		if (!(result = EVL_expr(tdbb, expr_request, idx->idx_expression)))
			result = &idx->idx_expression_desc;

		notNull = !(expr_request->req_flags & req_null);
	}
	catch (const Firebird::Exception&)
	{
		TRA_detach_request(expr_request);
		tdbb->setRequest(org_request);

		expr_request->req_caller = NULL;
		expr_request->req_flags &= ~req_in_use;
		expr_request->req_timestamp.invalidate();

		throw;
	}

	TRA_detach_request(expr_request);
	tdbb->setRequest(org_request);

	expr_request->req_caller = NULL;
	expr_request->req_flags &= ~req_in_use;
	expr_request->req_timestamp.invalidate();

	return result;
}


static void checkForLowerKeySkip(bool& skipLowerKey,
								 const bool partLower,
								 const IndexNode& node,
								 const temporary_key& lower,
								 const index_desc& idx,
								 const IndexRetrieval* retrieval)
{
	if (node.prefix == 0)
	{
		// If the prefix is 0 we have a full key.
		// (first node on every new page for example has prefix zero)
		if (partLower)
		{
			// With multi-segment compare first part of data with lowerKey
			skipLowerKey = ((lower.key_length <= node.length) &&
				(memcmp(node.data, lower.key_data, lower.key_length) == 0));

			if (skipLowerKey && (node.length > lower.key_length))
			{
				// We've bigger data in the node than in the lowerKey,
				// now check the segment-number
				const UCHAR* segp = node.data + lower.key_length;

				const USHORT segnum =
					idx.idx_count - (UCHAR)((idx.idx_flags & idx_descending) ? ((*segp) ^ -1) : *segp);

				if (segnum < retrieval->irb_lower_count)
				{
					skipLowerKey = false;
				}
			}
		}
		else
		{
			// Compare full data with lowerKey
			skipLowerKey = ((lower.key_length == node.length) &&
				(memcmp(node.data, lower.key_data, lower.key_length) == 0));
		}
	}
	else
	{
		// Check if we have a duplicate node (for the same page)
		if (node.prefix < lower.key_length)
		{
			if (node.prefix + node.length == lower.key_length)
				skipLowerKey = (memcmp(node.data, lower.key_data + node.prefix, node.length) == 0);
			else
				skipLowerKey = false;
		}
		else if ((node.prefix == lower.key_length) && node.length)
		{
			// In case of multi-segment check segment-number else
			// it's a different key
			if (partLower)
			{
				const USHORT segnum = idx.idx_count - (UCHAR)((idx.idx_flags & idx_descending) ?
					(*node.data) ^ -1 : *node.data);

				if (segnum < retrieval->irb_lower_count) {
					skipLowerKey = false;
				}
			}
			else {
				skipLowerKey = false;
			}
		}
	}
}


void BTR_evaluate(thread_db* tdbb, const IndexRetrieval* retrieval, RecordBitmap** bitmap,
				  RecordBitmap* bitmap_and)
{
/**************************************
 *
 *	B T R _ e v a l u a t e
 *
 **************************************
 *
 * Functional description
 *	Do an index scan and return a bitmap
 * 	of all candidate record numbers.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Remove ignore_nulls flag for older ODS
	//const Database* dbb = tdbb->getDatabase();

	index_desc idx;
	RelationPages* relPages = retrieval->irb_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);
	temporary_key lower, upper;
	lower.key_flags = 0;
	lower.key_length = 0;
	upper.key_flags = 0;
	upper.key_length = 0;
	btree_page* page = BTR_find_page(tdbb, retrieval, &window, &idx, &lower, &upper);

	const bool descending = (idx.idx_flags & idx_descending);
	bool skipLowerKey = (retrieval->irb_generic & irb_exclude_lower);
	const bool partLower = (retrieval->irb_lower_count < idx.idx_count);

	// If there is a starting descriptor, search down index to starting position.
	// This may involve sibling buckets if splits are in progress.  If there
	// isn't a starting descriptor, walk down the left side of the index.
	USHORT prefix;
	UCHAR* pointer;
	if (retrieval->irb_lower_count)
	{
		while (!(pointer = find_node_start_point(page, &lower, 0, &prefix,
			idx.idx_flags & idx_descending, (retrieval->irb_generic & (irb_starting | irb_partial)))))
		{
			page = (btree_page*) CCH_HANDOFF(tdbb, &window, page->btr_sibling, LCK_read, pag_index);
		}

		// Compute the number of matching characters in lower and upper bounds
		if (retrieval->irb_upper_count)
		{
			prefix = IndexNode::computePrefix(upper.key_data, upper.key_length,
											  lower.key_data, lower.key_length);
		}

		if (skipLowerKey)
		{
			IndexNode node;
			node.readNode(pointer, true);

			if ((lower.key_length == node.prefix + node.length) ||
				(lower.key_length <= node.prefix + node.length) && partLower)
			{
				const UCHAR* p = node.data, *q = lower.key_data + node.prefix;
				const UCHAR* const end = lower.key_data + lower.key_length;
				while (q < end)
				{
					if (*p++ != *q++)
					{
						skipLowerKey = false;
						break;
					}
				}

				if ((q >= end) && (p < node.data + node.length) && skipLowerKey && partLower)
				{
					// since key length always is multiplier of (STUFF_COUNT + 1) (for partial
					// compound keys) and we passed lower key completely then p pointed
					// us to the next segment number and we can use this fact to calculate
					// how many segments is equal to lower key
					const USHORT segnum = idx.idx_count - (UCHAR) (descending ? ((*p) ^ -1) : *p);

					if (segnum < retrieval->irb_lower_count) {
						skipLowerKey = false;
					}
				}
			}
			else {
				skipLowerKey = false;
			}
		}
	}
	else
	{
		pointer = page->btr_nodes + page->btr_jump_size;
		prefix = 0;
		skipLowerKey = false;
	}

	// if there is an upper bound, scan the index pages looking for it
	if (retrieval->irb_upper_count)
	{
		while (scan(tdbb, pointer, bitmap, bitmap_and, &idx, retrieval, prefix, &upper,
					skipLowerKey, lower))
		{
			page = (btree_page*) CCH_HANDOFF(tdbb, &window, page->btr_sibling, LCK_read, pag_index);
			pointer = page->btr_nodes + page->btr_jump_size;
			prefix = 0;
		}
	}
	else
	{
		// if there isn't an upper bound, just walk the index to the end of the level
		const UCHAR* endPointer = (UCHAR*)page + page->btr_length;
		const bool ignoreNulls =
			(retrieval->irb_generic & irb_ignore_null_value_key) && (idx.idx_count == 1);

		IndexNode node;
		pointer = node.readNode(pointer, true);
		// Check if pointer is still valid
		if (pointer > endPointer) {
			BUGCHECK(204);	// msg 204 index inconsistent
		}

		while (true)
		{
			if (node.isEndLevel) {
				break;
			}

			if (!node.isEndBucket)
			{
				// If we're walking in a descending index and we need to ignore NULLs
				// then stop at the first NULL we see (only for single segment!)
				if (descending && ignoreNulls && node.prefix == 0 &&
					node.length >= 1 && node.data[0] == 255)
				{
					break;
				}

				if (skipLowerKey)
				{
					checkForLowerKeySkip(skipLowerKey, partLower, node, lower, idx, retrieval);
				}

				if (!skipLowerKey)
				{
					if (!bitmap_and || bitmap_and->test(node.recordNumber.getValue()))
						RBM_SET(tdbb->getDefaultPool(), bitmap, node.recordNumber.getValue());
				}
				pointer = node.readNode(pointer, true);
				// Check if pointer is still valid
				if (pointer > endPointer) {
					BUGCHECK(204);	// msg 204 index inconsistent
				}
				continue;
			}

			page = (btree_page*) CCH_HANDOFF(tdbb, &window, page->btr_sibling, LCK_read, pag_index);
			endPointer = (UCHAR*) page + page->btr_length;
			pointer = page->btr_nodes + page->btr_jump_size;
			pointer = node.readNode(pointer, true);
			// Check if pointer is still valid
			if (pointer > endPointer) {
				BUGCHECK(204);	// msg 204 index inconsistent
			}
		}
	}

	CCH_RELEASE(tdbb, &window);
}


UCHAR* BTR_find_leaf(btree_page* bucket, temporary_key* key, UCHAR* value,
					 USHORT* return_value, bool descending, bool retrieval)
{
/**************************************
 *
 *	B T R _ f i n d _ l e a f
 *
 **************************************
 *
 * Functional description
 *	Locate and return a pointer to the insertion point.
 *	If the key doesn't belong in this bucket, return NULL.
 *	A flag indicates the index is descending.
 *
 **************************************/
	return find_node_start_point(bucket, key, value, return_value, descending, retrieval);
}


btree_page* BTR_find_page(thread_db* tdbb,
						  const IndexRetrieval* retrieval,
						  WIN* window,
						  index_desc* idx,
						  temporary_key* lower,
						  temporary_key* upper)
{
/**************************************
 *
 *	B T R _ f i n d _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Initialize for an index retrieval.
 *
 **************************************/

	SET_TDBB(tdbb);

	// Generate keys before we get any pages locked to avoid unwind
	// problems --  if we already have a key, assume that we
	// are looking for an equality
	if (retrieval->irb_key)
	{
		copy_key(retrieval->irb_key, lower);
		copy_key(retrieval->irb_key, upper);
	}
	else
	{
		idx_e errorCode = idx_e_ok;

		if (retrieval->irb_upper_count)
		{
			errorCode = BTR_make_key(tdbb, retrieval->irb_upper_count,
									 retrieval->irb_value + retrieval->irb_desc.idx_count,
									 &retrieval->irb_desc, upper,
									 (retrieval->irb_generic & irb_starting) != 0);
		}

		if (errorCode == idx_e_ok)
		{
			if (retrieval->irb_lower_count)
			{
				errorCode = BTR_make_key(tdbb, retrieval->irb_lower_count,
										 retrieval->irb_value, &retrieval->irb_desc, lower,
										 (retrieval->irb_generic & irb_starting) != 0);
			}
		}

		if (errorCode != idx_e_ok)
		{
			index_desc temp_idx = retrieval->irb_desc; // to avoid constness issues
			IndexErrorContext context(retrieval->irb_relation, &temp_idx);
			context.raise(tdbb, errorCode, NULL);
		}
	}

	RelationPages* relPages = retrieval->irb_relation->getPages(tdbb);
	fb_assert(window->win_page.getPageSpaceID() == relPages->rel_pg_space_id);

	window->win_page = relPages->rel_index_root;
	index_root_page* rpage = (index_root_page*) CCH_FETCH(tdbb, window, LCK_read, pag_root);

	if (!BTR_description(tdbb, retrieval->irb_relation, rpage, idx, retrieval->irb_index))
	{
		CCH_RELEASE(tdbb, window);
		IBERROR(260);	// msg 260 index unexpectedly deleted
	}

	btree_page* page = (btree_page*) CCH_HANDOFF(tdbb, window, idx->idx_root, LCK_read, pag_index);

	// If there is a starting descriptor, search down index to starting position.
	// This may involve sibling buckets if splits are in progress.  If there
	// isn't a starting descriptor, walk down the left side of the index (right
	// side if we are going backwards).
	// Ignore NULLs if flag is set and this is a 1 segment index,
	// ASC index and no lower bound value is given.
	const bool ignoreNulls = ((idx->idx_count == 1) && !(idx->idx_flags & idx_descending) &&
		(retrieval->irb_generic & irb_ignore_null_value_key) && !(retrieval->irb_lower_count));

	const bool firstData = (retrieval->irb_lower_count || ignoreNulls);

	if (firstData)
	{
		// Make a temporary key with length 1 and zero byte, this will return
		// the first data value after the NULLs for an ASC index.
		temporary_key firstNotNullKey;
		firstNotNullKey.key_flags = 0;
		firstNotNullKey.key_data[0] = 0;
		firstNotNullKey.key_length = 1;

		while (page->btr_level > 0)
		{
			while (true)
			{
				const temporary_key* tkey = ignoreNulls ? &firstNotNullKey : lower;
				const ULONG number = find_page(page, tkey, idx,
					NO_VALUE, (retrieval->irb_generic & (irb_starting | irb_partial)));
				if (number != END_BUCKET)
				{
					page = (btree_page*) CCH_HANDOFF(tdbb, window, number, LCK_read, pag_index);
					break;
				}

				page = (btree_page*) CCH_HANDOFF(tdbb, window, page->btr_sibling, LCK_read, pag_index);
			}
		}
	}
	else
	{
		IndexNode node;
		while (page->btr_level > 0)
		{
			UCHAR* pointer;
			const UCHAR* const endPointer = (UCHAR*) page + page->btr_length;
			pointer = page->btr_nodes + page->btr_jump_size;
			pointer = node.readNode(pointer, false);
			// Check if pointer is still valid
			if (pointer > endPointer) {
				BUGCHECK(204);	// msg 204 index inconsistent
			}
			page = (btree_page*) CCH_HANDOFF(tdbb, window, node.pageNumber, LCK_read, pag_index);
		}
	}

	return page;
}


void BTR_insert(thread_db* tdbb, WIN* root_window, index_insertion* insertion)
{
/**************************************
 *
 *	B T R _ i n s e r t
 *
 **************************************
 *
 * Functional description
 *	Insert a node into an index.
 *
 **************************************/
	SET_TDBB(tdbb);

	index_desc* idx = insertion->iib_descriptor;
	RelationPages* relPages = insertion->iib_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, idx->idx_root);
	btree_page* bucket = (btree_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_index);

	if (bucket->btr_level == 0)
	{
		CCH_RELEASE(tdbb, &window);
		CCH_FETCH(tdbb, &window, LCK_write, pag_index);
	}
	CCH_RELEASE(tdbb, root_window);

	temporary_key key;
	key.key_flags = 0;
	key.key_length = 0;

	RecordNumber recordNumber(0);
	BtrPageGCLock lock(tdbb);
	insertion->iib_dont_gc_lock = &lock;
	ULONG split_page = add_node(tdbb, &window, insertion, &key, &recordNumber, NULL, NULL);
	if (split_page == NO_SPLIT) {
		return;
	}

	// The top of the index has split.  We need to make a new level and
	// update the index root page.  Oh boy.
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, root_window, LCK_write, pag_root);

	window.win_page = root->irt_rpt[idx->idx_id].irt_root;
	bucket = (btree_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_index);

	if (window.win_page.getPageNum() != idx->idx_root)
	{
		// AB: It could be possible that the "top" page meanwhile was changed by
		// another insert. In that case we are going to insert our split_page
		// in the existing "top" page instead of making a new "top" page.

		CCH_RELEASE(tdbb, root_window);
		lock.enablePageGC(tdbb);

		index_insertion propagate = *insertion;
		propagate.iib_number.setValue(split_page);
		propagate.iib_descriptor->idx_root = window.win_page.getPageNum();
		propagate.iib_key = &key;

		temporary_key ret_key;
		ret_key.key_flags = 0;
		ret_key.key_length = 0;
		split_page = insert_node(tdbb, &window, &propagate, &ret_key, &recordNumber, NULL, NULL);

		if (split_page != NO_SPLIT)
		{
			if (split_page == NO_VALUE_PAGE) {
				CCH_RELEASE(tdbb, &window);
			}
			else {
				lock.enablePageGC(tdbb);
			}
			BUGCHECK(204);	// msg 204 index inconsistent
		}
		return;
	}

	// the original page was marked as not garbage-collectable, but
	// since it is the root page it won't be garbage-collected anyway,
	// so go ahead and mark it as garbage-collectable now.
	lock.enablePageGC(tdbb);

	WIN new_window(relPages->rel_pg_space_id, split_page);
	btree_page* new_bucket = (btree_page*) CCH_FETCH(tdbb, &new_window, LCK_read, pag_index);

	if (bucket->btr_level != new_bucket->btr_level)
	{
		CCH_RELEASE(tdbb, root_window);
		CCH_RELEASE(tdbb, &new_window);
		CCH_RELEASE(tdbb, &window);
		BUGCHECK(204);	// msg 204 index inconsistent
	}

	// hvlad: save some info from bucket for latter use before releasing a page
	const USHORT btr_relation = bucket->btr_relation;
	const UCHAR btr_level = bucket->btr_level + 1;
	const UCHAR btr_id = bucket->btr_id;
	const USHORT btr_jump_interval = bucket->btr_jump_interval;

	// hvlad: don't even try to use page buffer after page was released
	bucket = NULL;

	CCH_RELEASE(tdbb, &new_window);
	CCH_RELEASE(tdbb, &window);

	if (btr_level > MAX_LEVELS)
	{
		// Maximum level depth reached.
		// AB: !! NEW ERROR MESSAGE ? !!
		BUGCHECK(204);	// msg 204 index inconsistent
	}

	// Allocate and format new bucket, this will always be a non-leaf page
	new_bucket = (btree_page*) DPM_allocate(tdbb, &new_window);
	CCH_precedence(tdbb, &new_window, window.win_page);

	new_bucket->btr_header.pag_type = pag_index;
	new_bucket->btr_relation = btr_relation;
	new_bucket->btr_level = btr_level;
	new_bucket->btr_id = btr_id;

	// Write jumpinfo
	new_bucket->btr_jump_interval = btr_jump_interval;
	new_bucket->btr_jump_size = 0;
	new_bucket->btr_jump_count = 0;

	UCHAR* pointer = new_bucket->btr_nodes;

	// Set up first node as degenerate, but pointing to first bucket on
	// next level.
	IndexNode node;
	node.setNode(0, 0, RecordNumber(0), window.win_page.getPageNum());
	pointer = node.writeNode(pointer, false);

	// Move in the split node
	node.setNode(0, key.key_length, recordNumber, split_page);
	node.data = key.key_data;
	pointer = node.writeNode(pointer, false);

	// mark end of level
	node.setEndLevel();
	pointer = node.writeNode(pointer, false);

	// Calculate length of bucket
	new_bucket->btr_length = pointer - (UCHAR*)new_bucket;

	// update the root page to point to the new top-level page,
	// and make sure the new page has higher precedence so that
	// it will be written out first--this will make sure that the
	// root page doesn't point into space
	CCH_RELEASE(tdbb, &new_window);
	CCH_precedence(tdbb, root_window, new_window.win_page);
	CCH_MARK(tdbb, root_window);
	root->irt_rpt[idx->idx_id].irt_root = new_window.win_page.getPageNum();
	CCH_RELEASE(tdbb, root_window);
}


idx_e BTR_key(thread_db* tdbb, jrd_rel* relation, Record* record, index_desc* idx,
			  temporary_key* key, const bool fuzzy, USHORT count)
{
/**************************************
 *
 *	B T R _ k e y
 *
 **************************************
 *
 * Functional description
 *	Compute a key from a record and an index descriptor.
 *	Note that compound keys are expanded by 25%.  If this
 *	changes, both BTR_key_length and GDEF exe.e have to
 *	change.
 *
 **************************************/
	temporary_key temp;
	temp.key_flags = 0;
	temp.key_length = 0;
	DSC desc;
	DSC* desc_ptr;

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	index_desc::idx_repeat* tail = idx->idx_rpt;
	key->key_flags = 0;
	key->key_nulls = 0;

	const bool descending = (idx->idx_flags & idx_descending);

	if (!count)
		count = idx->idx_count;

	const USHORT maxKeyLength = dbb->getMaxIndexKeyLength();

	try {

		const USHORT keyType = fuzzy ?
			INTL_KEY_PARTIAL :
			((idx->idx_flags & idx_unique) ? INTL_KEY_UNIQUE : INTL_KEY_SORT);

		// Special case single segment indices

		if (idx->idx_count == 1)
		{
			bool isNull;
			// for expression indices, compute the value of the expression
			if (idx->idx_flags & idx_expressn)
			{
				bool notNull;
				desc_ptr = BTR_eval_expression(tdbb, idx, record, notNull);
				// Multi-byte text descriptor is returned already adjusted.
				isNull = !notNull;
			}
			else
			{
				desc_ptr = &desc;
				// In order to "map a null to a default" value (in EVL_field()),
				// the relation block is referenced.
				// Reference: Bug 10116, 10424
				//
				isNull = !EVL_field(relation, record, tail->idx_field, desc_ptr);

				if (!isNull &&
					!(relation->rel_flags & REL_system) &&	// UNICODE_FSS_HACK
					desc_ptr->dsc_dtype == dtype_text)
				{
					// That's necessary for NO-PAD collations.
					INTL_adjust_text_descriptor(tdbb, desc_ptr);
				}
			}

			if (isNull)
				key->key_nulls = 1;

			key->key_flags |= key_empty;

			compress(tdbb, desc_ptr, key, tail->idx_itype, isNull, descending, keyType);
		}
		else
		{
			UCHAR* p = key->key_data;
			SSHORT stuff_count = 0;
			temp.key_flags |= key_empty;
			for (USHORT n = 0; n < count; n++, tail++)
			{
				for (; stuff_count; --stuff_count)
				{
					*p++ = 0;

					if (p - key->key_data >= maxKeyLength)
						return idx_e_keytoobig;
				}

				desc_ptr = &desc;
				// In order to "map a null to a default" value (in EVL_field()),
				// the relation block is referenced.
				// Reference: Bug 10116, 10424
				const bool isNull = !EVL_field(relation, record, tail->idx_field, desc_ptr);

				if (isNull)
					key->key_nulls |= 1 << n;
				else
				{
					if (!(relation->rel_flags & REL_system) &&	// UNICODE_FSS_HACK
						desc_ptr->dsc_dtype == dtype_text)
					{
						// That's necessary for NO-PAD collations.
						INTL_adjust_text_descriptor(tdbb, desc_ptr);
					}
				}

				compress(tdbb, desc_ptr, &temp, tail->idx_itype, isNull, descending, keyType);

				const UCHAR* q = temp.key_data;
				for (USHORT l = temp.key_length; l; --l, --stuff_count)
				{
					if (stuff_count == 0)
					{
						*p++ = idx->idx_count - n;
						stuff_count = STUFF_COUNT;

						if (p - key->key_data >= maxKeyLength)
							return idx_e_keytoobig;
					}

					*p++ = *q++;

					if (p - key->key_data >= maxKeyLength)
						return idx_e_keytoobig;
				}
			}

			key->key_length = (p - key->key_data);

			if (temp.key_flags & key_empty)
				key->key_flags |= key_empty;
		}

		if (key->key_length >= maxKeyLength)
			return idx_e_keytoobig;

		if (descending)
			BTR_complement_key(key);

	}	// try
	catch (const Firebird::Exception& ex)
	{
		ex.stuff_exception(tdbb->tdbb_status_vector);
		key->key_length = 0;
		return idx_e_conversion;
	}

	return idx_e_ok;
}


USHORT BTR_key_length(thread_db* tdbb, jrd_rel* relation, index_desc* idx)
{
/**************************************
 *
 *	B T R _ k e y _ l e n g t h
 *
 **************************************
 *
 * Functional description
 *	Compute the maximum key length for an index.
 *
 **************************************/
	SET_TDBB(tdbb);

	// hvlad: in ODS11 key of descending index can be prefixed with
	//		  one byte value. See comments in compress
	const size_t prefix = (idx->idx_flags & idx_descending) ? 1 : 0;

	const Format* format = MET_current(tdbb, relation);
	index_desc::idx_repeat* tail = idx->idx_rpt;

	// notice "return sizeof()" above already returns size_t for this
	// function that declared return type being USHORT.
	size_t length;

	// If there is only a single key, the computation is straightforward.
	if (idx->idx_count == 1)
	{
		switch (tail->idx_itype)
		{
		case idx_numeric:
			length = sizeof(double);
			break;

		case idx_sql_time:
			length = sizeof(ULONG);
			break;

		case idx_sql_date:
			length = sizeof(SLONG);
			break;

		case idx_timestamp:
			length = sizeof(SINT64);
			break;

		case idx_numeric2:
			length = INT64_KEY_LENGTH;
			break;

		case idx_boolean:
			length = sizeof(UCHAR);
			break;

		default:
			if (idx->idx_flags & idx_expressn)
			{
				fb_assert(idx->idx_expression != NULL);
				length = idx->idx_expression_desc.dsc_length;
				if (idx->idx_expression_desc.dsc_dtype == dtype_varying)
				{
					length = length - sizeof(SSHORT);
				}
			}
			else
			{
				length = format->fmt_desc[tail->idx_field].dsc_length;
				if (format->fmt_desc[tail->idx_field].dsc_dtype == dtype_varying) {
					length = length - sizeof(SSHORT);
				}
			}

			if (tail->idx_itype >= idx_first_intl_string) {
				length = INTL_key_length(tdbb, tail->idx_itype, length);
			}
			break;
		}

		return length + prefix;
	}

	// Compute length of key for segmented indices.
	size_t key_length = 0;

	for (USHORT n = 0; n < idx->idx_count; n++, tail++)
	{
		switch (tail->idx_itype)
		{
		case idx_numeric:
			length = sizeof(double);
			break;
		case idx_sql_time:
			length = sizeof(ULONG);
			break;
		case idx_sql_date:
			length = sizeof(ULONG);
			break;
		case idx_timestamp:
			length = sizeof(SINT64);
			break;
		case idx_numeric2:
			length = INT64_KEY_LENGTH;
			break;
		case idx_boolean:
			length = sizeof(UCHAR);
			break;
		default:
			length = format->fmt_desc[tail->idx_field].dsc_length;
			if (format->fmt_desc[tail->idx_field].dsc_dtype == dtype_varying) {
				length -= sizeof(SSHORT);
			}
			if (tail->idx_itype >= idx_first_intl_string) {
				length = INTL_key_length(tdbb, tail->idx_itype, length);
			}
			break;
		}
		key_length += ((length + prefix + STUFF_COUNT - 1) / STUFF_COUNT) * (STUFF_COUNT + 1);
	}

	return key_length;
}


bool BTR_lookup(thread_db* tdbb, jrd_rel* relation, USHORT id, index_desc* buffer,
				  RelationPages* relPages)
{
/**************************************
 *
 *	B T R _ l o o k u p
 *
 **************************************
 *
 * Functional description
 *	Return a description of the specified index.
 *
 **************************************/
	SET_TDBB(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	index_root_page* root = fetch_root(tdbb, &window, relation, relPages);
	if (!root) {
		return false;
	}

	if (id >= root->irt_count || !BTR_description(tdbb, relation, root, buffer, id))
	{
		CCH_RELEASE(tdbb, &window);
		return false;
	}
	CCH_RELEASE(tdbb, &window);
	return true;
}


idx_e BTR_make_key(thread_db* tdbb,
				   USHORT count,
				   const ValueExprNode* const* exprs,
				   const index_desc* idx,
				   temporary_key* key,
				   bool fuzzy)
{
/**************************************
 *
 *	B T R _ m a k e _ k e y
 *
 **************************************
 *
 * Functional description
 *	Construct a (possibly) compound search key given a key count,
 *	a vector of value expressions, and a place to put the key.
 *
 **************************************/
	DSC temp_desc;
	temporary_key temp;
	temp.key_flags = 0;
	temp.key_length = 0;

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();

	fb_assert(count > 0);
	fb_assert(idx != NULL);
	fb_assert(exprs != NULL);
	fb_assert(key != NULL);

	key->key_flags = 0;
	key->key_nulls = 0;

	const bool descending = (idx->idx_flags & idx_descending);

	const index_desc::idx_repeat* tail = idx->idx_rpt;

	const USHORT keyType = fuzzy ?
		INTL_KEY_PARTIAL : ((idx->idx_flags & idx_unique) ? INTL_KEY_UNIQUE : INTL_KEY_SORT);

	const USHORT maxKeyLength = dbb->getMaxIndexKeyLength();

	// If the index is a single segment index, don't sweat the compound stuff
	if (idx->idx_count == 1)
	{
		bool isNull;
		const dsc* desc = eval(tdbb, *exprs, &temp_desc, &isNull);
		key->key_flags |= key_empty;

		if (isNull)
			key->key_nulls = 1;

		compress(tdbb, desc, key, tail->idx_itype, isNull, descending, keyType);

		if (fuzzy && (key->key_flags & key_empty))
			key->key_length = 0;
	}
	else
	{
		// Make a compound key
		UCHAR* p = key->key_data;
		SSHORT stuff_count = 0;
		bool is_key_empty = true;
		USHORT prior_length = 0;
		USHORT n = 0;
		for (; n < count; n++, tail++)
		{
			for (; stuff_count; --stuff_count)
			{
				*p++ = 0;

				if (p - key->key_data >= maxKeyLength)
					return idx_e_keytoobig;
			}

			bool isNull;
			const dsc* desc = eval(tdbb, *exprs++, &temp_desc, &isNull);

			if (isNull)
				key->key_nulls |= 1 << n;

			temp.key_flags |= key_empty;

			compress(tdbb, desc, &temp, tail->idx_itype, isNull, descending,
				(n == count - 1 ?
					keyType : ((idx->idx_flags & idx_unique) ? INTL_KEY_UNIQUE : INTL_KEY_SORT)));

			if (!(temp.key_flags & key_empty))
				is_key_empty = false;

			prior_length = (p - key->key_data);

			const UCHAR* q = temp.key_data;
			for (USHORT l = temp.key_length; l; --l, --stuff_count)
			{
				if (stuff_count == 0)
				{
					*p++ = idx->idx_count - n;
					stuff_count = STUFF_COUNT;

					if (p - key->key_data >= maxKeyLength)
						return idx_e_keytoobig;
				}

				*p++ = *q++;

				if (p - key->key_data >= maxKeyLength)
					return idx_e_keytoobig;
			}
		}

		// AB: Fix bug SF #1242982
		// Equality search on first segment (integer) in compound indexes resulted
		// in more scans on specific values (2^n, f.e. 131072) than needed.
		if (!fuzzy && (n != idx->idx_count))
		{
			for (; stuff_count; --stuff_count)
			{
				*p++ = 0;

				if (p - key->key_data >= maxKeyLength)
					return idx_e_keytoobig;
			}
		}

		// dimitr:	If the search is fuzzy and the last segment is empty,
		//			then skip it for the lookup purposes. It enforces
		//			the rule that every string starts with an empty string.
		if (fuzzy && (temp.key_flags & key_empty))
			key->key_length = prior_length;
		else
			key->key_length = (p - key->key_data);

		if (is_key_empty)
		{
			key->key_flags |= key_empty;
			if (fuzzy)
				key->key_length = 0;
		}
	}

	if (key->key_length >= maxKeyLength)
		return idx_e_keytoobig;

	if (descending)
		BTR_complement_key(key);

	return idx_e_ok;
}


void BTR_make_null_key(thread_db* tdbb, const index_desc* idx, temporary_key* key)
{
/**************************************
 *
 *	B T R _ m a k e _ n u l l _ k e y
 *
 **************************************
 *
 * Functional description
 *	Construct a (possibly) compound search key consist from
 *  all null values. This is worked only for ODS11 and later
 *
 **************************************/
	dsc null_desc;
	null_desc.dsc_dtype = dtype_text;
	null_desc.dsc_flags = 0;
	null_desc.dsc_sub_type = 0;
	null_desc.dsc_scale = 0;
	null_desc.dsc_length = 1;
	null_desc.dsc_ttype() = ttype_ascii;
	null_desc.dsc_address = (UCHAR*) " ";

	temporary_key temp;
	temp.key_flags = 0;
	temp.key_length = 0;

	SET_TDBB(tdbb);

	fb_assert(idx != NULL);
	fb_assert(key != NULL);

	key->key_flags = 0;
	key->key_nulls = (1 << idx->idx_count) - 1;

	const bool descending = (idx->idx_flags & idx_descending);

	const index_desc::idx_repeat* tail = idx->idx_rpt;

	// If the index is a single segment index, don't sweat the compound stuff
	if ((idx->idx_count == 1) || (idx->idx_flags & idx_expressn))
	{
		compress(tdbb, &null_desc, key, tail->idx_itype, true, descending, false);
	}
	else
	{
		// Make a compound key
		UCHAR* p = key->key_data;
		SSHORT stuff_count = 0;
		temp.key_flags |= key_empty;

		for (USHORT n = 0; n < idx->idx_count; n++, tail++)
		{
			for (; stuff_count; --stuff_count)
				*p++ = 0;

			compress(tdbb, &null_desc, &temp, tail->idx_itype, true, descending, false);

			const UCHAR* q = temp.key_data;
			for (USHORT l = temp.key_length; l; --l, --stuff_count)
			{
				if (stuff_count == 0)
				{
					*p++ = idx->idx_count - n;
					stuff_count = STUFF_COUNT;
				}
				*p++ = *q++;
			}
		}

		key->key_length = (p - key->key_data);

		if (temp.key_flags & key_empty)
			key->key_flags |= key_empty;
	}

	if (descending)
		BTR_complement_key(key);
}


bool BTR_next_index(thread_db* tdbb, jrd_rel* relation, jrd_tra* transaction, index_desc* idx,
					   WIN* window)
{
/**************************************
 *
 *	B T R _ n e x t _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Get next index for relation.  Index ids
 *  recently changed from UCHAR to SHORT
 *
 **************************************/
	SET_TDBB(tdbb);

	USHORT id;
	if (idx->idx_id == idx_invalid)
	{
		id = 0;
		window->win_bdb = NULL;
	}
	else {
		id = idx->idx_id + 1;
	}

	index_root_page* root;
	if (window->win_bdb) {
		root = (index_root_page*) window->win_buffer;
	}
	else
	{
		RelationPages* relPages;
		if (transaction)
			relPages = relation->getPages(tdbb, transaction->tra_number);
		else
			relPages = relation->getPages(tdbb);

		if (!(root = fetch_root(tdbb, window, relation, relPages)))
		{
			return false;
		}
	}

	for (; id < root->irt_count; ++id)
	{
		const index_root_page::irt_repeat* irt_desc = root->irt_rpt + id;
		if (!irt_desc->irt_root && (irt_desc->irt_flags & irt_in_progress) && transaction)
		{
			const TraNumber trans = irt_desc->irt_transaction;
			CCH_RELEASE(tdbb, window);
			const int trans_state = TRA_wait(tdbb, transaction, trans, jrd_tra::tra_wait);
			if ((trans_state == tra_dead) || (trans_state == tra_committed))
			{
				// clean up this left-over index
				root = (index_root_page*) CCH_FETCH(tdbb, window, LCK_write, pag_root);
				irt_desc = root->irt_rpt + id;
				if (!irt_desc->irt_root && irt_desc->irt_transaction == trans &&
					(irt_desc->irt_flags & irt_in_progress))
				{
					BTR_delete_index(tdbb, window, id);
				}
				else {
					CCH_RELEASE(tdbb, window);
				}
				root = (index_root_page*) CCH_FETCH(tdbb, window, LCK_read, pag_root);
				continue;
			}

			root = (index_root_page*) CCH_FETCH(tdbb, window, LCK_read, pag_root);
		}
		if (BTR_description(tdbb, relation, root, idx, id)) {
			return true;
		}
	}

	CCH_RELEASE(tdbb, window);

	return false;
}


void BTR_remove(thread_db* tdbb, WIN* root_window, index_insertion* insertion)
{
/**************************************
 *
 *	B T R _ r e m o v e
 *
 **************************************
 *
 * Functional description
 *	Remove an index node from a b-tree.
 *	If the node doesn't exist, don't get overly excited.
 *
 **************************************/

	//const Database* dbb = tdbb->getDatabase();
	index_desc* idx = insertion->iib_descriptor;
	RelationPages* relPages = insertion->iib_relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, idx->idx_root);
	btree_page* page = (btree_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_index);

	// If the page is level 0, re-fetch it for write
	const UCHAR level = page->btr_level;
	if (level == 0)
	{
		CCH_RELEASE(tdbb, &window);
		CCH_FETCH(tdbb, &window, LCK_write, pag_index);
	}

	// remove the node from the index tree via recursive descent
	contents result = remove_node(tdbb, insertion, &window);

	// if the root page points at only one lower page, remove this
	// level to prevent the tree from being deeper than necessary--
	// do this only if the level is greater than 1 to prevent
	// excessive thrashing in the case where a small table is
	// constantly being loaded and deleted.
	if ((result == contents_single) && (level > 1))
	{
		// we must first release the windows to obtain the root for write
		// without getting deadlocked

		CCH_RELEASE(tdbb, &window);
		CCH_RELEASE(tdbb, root_window);

		index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, root_window, LCK_write, pag_root);
		page = (btree_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_index);

		// get the page number of the child, and check to make sure
		// the page still has only one node on it
		UCHAR* pointer = page->btr_nodes + page->btr_jump_size;
		IndexNode pageNode;
		pointer = pageNode.readNode(pointer, false);

		const SLONG number = pageNode.pageNumber;
		pointer = pageNode.readNode(pointer, false);
		if (!(pageNode.isEndBucket || pageNode.isEndLevel))
		{
			CCH_RELEASE(tdbb, &window);
			CCH_RELEASE(tdbb, root_window);
			return;
		}

		CCH_MARK(tdbb, root_window);
		root->irt_rpt[idx->idx_id].irt_root = number;

		// release the pages, and place the page formerly at the top level
		// on the free list, making sure the root page is written out first
		// so that we're not pointing to a released page
		CCH_RELEASE(tdbb, root_window);

		CCH_MARK(tdbb, &window);
		page->btr_header.pag_flags |= btr_released;

		CCH_RELEASE(tdbb, &window);
		PAG_release_page(tdbb, window.win_page, root_window->win_page);
	}

	if (window.win_bdb) {
		CCH_RELEASE(tdbb, &window);
	}
	if (root_window->win_bdb) {
		CCH_RELEASE(tdbb, root_window);
	}
}


void BTR_reserve_slot(thread_db* tdbb, jrd_rel* relation, jrd_tra* transaction, index_desc* idx)
{
/**************************************
 *
 *	B T R _ r e s e r v e _ s l o t
 *
 **************************************
 *
 * Functional description
 *	Reserve a slot on an index root page
 *	in preparation to index creation.
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	fb_assert(relation);
	RelationPages* relPages = relation->getPages(tdbb);
	fb_assert(relPages && relPages->rel_index_root);

	fb_assert(transaction);

	// Get root page, assign an index id, and store the index descriptor.
	// Leave the root pointer null for the time being.
	// Index id for temporary index instance of global temporary table is
	// already assigned, use it.
	const bool use_idx_id = (relPages->rel_instance_id != 0);
	if (use_idx_id) {
		fb_assert(idx->idx_id <= dbb->dbb_max_idx);
	}

	WIN window(relPages->rel_pg_space_id, relPages->rel_index_root);
	index_root_page* root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_root);
	CCH_MARK(tdbb, &window);

	// check that we create no more indexes than will fit on a single root page
	if (root->irt_count > dbb->dbb_max_idx)
	{
		CCH_RELEASE(tdbb, &window);
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_max_idx) << Arg::Num(dbb->dbb_max_idx));
	}

	// Scan the index page looking for the high water mark of the descriptions and,
	// perhaps, an empty index slot

	if (use_idx_id && (idx->idx_id >= root->irt_count))
	{
		memset(root->irt_rpt + root->irt_count, 0,
			sizeof(index_root_page::irt_repeat) * (idx->idx_id - root->irt_count + 1));
		root->irt_count = idx->idx_id + 1;
	}

	UCHAR* desc = 0;
	USHORT len, space;
	index_root_page::irt_repeat* slot = NULL;
	index_root_page::irt_repeat* end = NULL;

	for (int retry = 0; retry < 2; ++retry)
	{
		len = idx->idx_count * sizeof(irtd);

		space = dbb->dbb_page_size;
		slot = NULL;

		end = root->irt_rpt + root->irt_count;
		for (index_root_page::irt_repeat* root_idx = root->irt_rpt; root_idx < end; root_idx++)
		{
			if (root_idx->irt_root || (root_idx->irt_flags & irt_in_progress)) {
				space = MIN(space, root_idx->irt_desc);
			}
			if (!root_idx->irt_root && !slot && !(root_idx->irt_flags & irt_in_progress))
			{
				if (!use_idx_id || (root_idx - root->irt_rpt) == idx->idx_id)
				{
					slot = root_idx;
				}
			}
		}

		space -= len;
		desc = (UCHAR*)root + space;

		// Verify that there is enough room on the Index root page.
		if (desc < (UCHAR*) (end + 1))
		{
			// Not enough room:  Attempt to compress the index root page and try again.
			// If this is the second try already, then there really is no more room.
			if (retry)
			{
				CCH_RELEASE(tdbb, &window);
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_index_root_page_full));
			}
			compress_root(tdbb, root);
		}
		else
			break;
	}

	// If we didn't pick up an empty slot, allocate a new one
	fb_assert(!use_idx_id || (use_idx_id && slot));
	if (!slot)
	{
		slot = end;
		root->irt_count++;
	}

	idx->idx_id = slot - root->irt_rpt;
	slot->irt_desc = space;
	fb_assert(idx->idx_count <= MAX_UCHAR);
	slot->irt_keys = (UCHAR) idx->idx_count;
	slot->irt_flags = idx->idx_flags | irt_in_progress;
	slot->irt_transaction = transaction->tra_number;

	slot->irt_root = 0;

	// Exploit the fact idx_repeat structure matches ODS IRTD one
	memcpy(desc, idx->idx_rpt, len);

	CCH_RELEASE(tdbb, &window);
}


void BTR_selectivity(thread_db* tdbb, jrd_rel* relation, USHORT id, SelectivityList& selectivity)
{
/**************************************
 *
 *	B T R _ s e l e c t i v i t y
 *
 **************************************
 *
 * Functional description
 *	Update index selectivity on the fly.
 *	Note that index leaf pages are walked
 *	without visiting data pages. Thus the
 *	effects of uncommitted transactions
 *	will be included in the calculation.
 *
 **************************************/

	SET_TDBB(tdbb);
	RelationPages* relPages = relation->getPages(tdbb);
	WIN window(relPages->rel_pg_space_id, -1);

	index_root_page* root = fetch_root(tdbb, &window, relation, relPages);
	if (!root) {
		return;
	}

	SLONG page;
	if (id >= root->irt_count || !(page = root->irt_rpt[id].irt_root))
	{
		CCH_RELEASE(tdbb, &window);
		return;
	}
	window.win_flags = WIN_large_scan;
	window.win_scans = 1;
	btree_page* bucket = (btree_page*) CCH_HANDOFF(tdbb, &window, page, LCK_read, pag_index);

	// go down the left side of the index to leaf level
	UCHAR* pointer = bucket->btr_nodes + bucket->btr_jump_size;
	while (bucket->btr_level)
	{
		IndexNode pageNode;
		pageNode.readNode(pointer, false);
		bucket = (btree_page*) CCH_HANDOFF(tdbb, &window, pageNode.pageNumber, LCK_read, pag_index);
		pointer = bucket->btr_nodes + bucket->btr_jump_size;
		page = pageNode.pageNumber;
	}

	FB_UINT64 nodes = 0;
	FB_UINT64 duplicates = 0;
	temporary_key key;
	key.key_flags = 0;
	key.key_length = 0;
	SSHORT l;
	bool firstNode = true;
	const bool descending = (root->irt_rpt[id].irt_flags & irt_descending);
	const ULONG segments = root->irt_rpt[id].irt_keys;

	// SSHORT count, stuff_count, pos, i;
	Firebird::HalfStaticArray<FB_UINT64, 4> duplicatesList;
	duplicatesList.grow(segments);
	memset(duplicatesList.begin(), 0, segments * sizeof(FB_UINT64));

	//const Database* dbb = tdbb->getDatabase();

	// go through all the leaf nodes and count them;
	// also count how many of them are duplicates
	IndexNode node;
	while (page)
	{
		pointer = node.readNode(pointer, true);
		while (true)
		{
			if (node.isEndBucket || (nodes % 100 == 0))
			{
				if (--tdbb->tdbb_quantum < 0)
					JRD_reschedule(tdbb, 0, true);
			}

			if (node.isEndBucket || node.isEndLevel)
				break;

			++nodes;
			l = node.length + node.prefix;

			if (segments > 1 && !firstNode)
			{

				// Initialize variables for segment duplicate check.
				// count holds the current checking segment (starting by
				// the maximum segment number to 1).
				const UCHAR* p1 = key.key_data;
				const UCHAR* const p1_end = p1 + key.key_length;
				const UCHAR* p2 = node.data;
				const UCHAR* const p2_end = p2 + node.length;
				SSHORT count, stuff_count;
				if (node.prefix == 0)
				{
					count = *p2;
					//pos = 0;
					stuff_count = 0;
				}
				else
				{
					const SSHORT pos = node.prefix;
					// find the segment number were we're starting.
					const SSHORT i = (pos / (STUFF_COUNT + 1)) * (STUFF_COUNT + 1);
					if (i == pos)
					{
						// We _should_ pick number from data if available
						count = *p2;
					}
					else {
						count = *(p1 + i);
					}
					// update stuff_count to the current position.
					stuff_count = STUFF_COUNT + 1 - (pos - i);
					p1 += pos;
				}

				//Look for duplicates in the segments
				while ((p1 < p1_end) && (p2 < p2_end))
				{
					if (stuff_count == 0)
					{
						if (*p1 != *p2)
						{
							// We're done
							break;
						}
						count = *p2;
						p1++;
						p2++;
						stuff_count = STUFF_COUNT;
					}
					if (*p1 != *p2)
					{
						//We're done
						break;
					}
					p1++;
					p2++;
					stuff_count--;
				}

				// For descending indexes the segment-number is also
				// complemented, thus reverse it back.
				// Note: values are complemented per UCHAR base.
				if (descending) {
					count = (255 - count);
				}

				if ((p1 == p1_end) && (p2 == p2_end)) {
					count = 0; // All segments are duplicates
				}

				for (ULONG i = count + 1; i <= segments; i++) {
					duplicatesList[segments - i]++;
				}

			}

			// figure out if this is a duplicate
			bool dup;
			if (node.nodePointer == bucket->btr_nodes + bucket->btr_jump_size) {
				dup = node.keyEqual(key.key_length, key.key_data);
			}
			else {
				dup = (!node.length && (l == key.key_length));
			}
			if (dup && !firstNode) {
				++duplicates;
			}
			if (firstNode) {
				firstNode = false;
			}

			// keep the key value current for comparison with the next key
			key.key_length = l;
			memcpy(key.key_data + node.prefix, node.data, node.length);
			pointer = node.readNode(pointer, true);
		}

		if (node.isEndLevel || !(page = bucket->btr_sibling))
		{
			break;
		}
		bucket = (btree_page*) CCH_HANDOFF_TAIL(tdbb, &window, page, LCK_read, pag_index);
		pointer = bucket->btr_nodes + bucket->btr_jump_size;
	}

	CCH_RELEASE_TAIL(tdbb, &window);

	// calculate the selectivity
	selectivity.grow(segments);
	if (segments > 1)
	{
		for (ULONG i = 0; i < segments; i++) {
			selectivity[i] = (float) (nodes ? 1.0 / (float) (nodes - duplicatesList[i]) : 0.0);
		}
	}
	else {
		selectivity[0] = (float) (nodes ? 1.0 / (float) (nodes - duplicates) : 0.0);
	}

	// Store the selectivity on the root page
	window.win_page = relPages->rel_index_root;
	window.win_flags = 0;
	root = (index_root_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_root);
	CCH_MARK(tdbb, &window);
	update_selectivity(root, id, selectivity);
	CCH_RELEASE(tdbb, &window);
}


bool BTR_types_comparable(const dsc& target, const dsc& source)
{
/**************************************
 *
 *	B T R _ t y p e s _ c o m p a r a b l e
 *
 **************************************
 *
 * Functional description
 *	Return whether two datatypes are comparable in terms of the CVT rules.
 *  The purpose is to ensure that compress() converts datatypes in the same
 *  direction as CVT2_compare(), thus causing index scans to always deliver
 *  the same results as the generic boolean evaluation.
 *
 **************************************/
	if (source.isNull() || DSC_EQUIV(&source, &target, true))
	{
		return true;
	}

	if (DTYPE_IS_TEXT(target.dsc_dtype))
	{
		// should we also check for the INTL stuff here?
		return (DTYPE_IS_TEXT(source.dsc_dtype) || source.dsc_dtype == dtype_dbkey);
	}

	if (target.dsc_dtype == dtype_int64)
	{
		return (source.dsc_dtype <= dtype_long || source.dsc_dtype == dtype_int64);
	}

	if (DTYPE_IS_NUMERIC(target.dsc_dtype))
	{
		return (source.dsc_dtype <= dtype_double || source.dsc_dtype == dtype_int64);
	}

	if (target.dsc_dtype == dtype_sql_date)
	{
		return (source.dsc_dtype <= dtype_sql_date || source.dsc_dtype == dtype_timestamp);
	}

	if (DTYPE_IS_DATE(target.dsc_dtype))
	{
		return (source.dsc_dtype <= dtype_timestamp);
	}

	if (target.dsc_dtype == dtype_boolean)
	{
		return (source.dsc_dtype == dtype_boolean);
	}

	return false;
}


static ULONG add_node(thread_db* tdbb,
					  WIN* window,
					  index_insertion* insertion,
					  temporary_key* new_key,
					  RecordNumber* new_record_number,
					  ULONG* original_page,
					  ULONG* sibling_page)
{
/**************************************
 *
 *	a d d _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Insert a node in an index.  This recurses to the leaf level.
 *	If a split occurs, return the new index page number and its
 *	leading string.
 *
 **************************************/

	SET_TDBB(tdbb);
	btree_page* bucket = (btree_page*) window->win_buffer;

	// For leaf level guys, loop thru the leaf buckets until insertion
	// point is found (should be instant)
	if (bucket->btr_level == 0)
	{
		while (true)
		{
			const ULONG split = insert_node(tdbb, window, insertion, new_key,
				new_record_number, original_page, sibling_page);
			if (split != NO_VALUE_PAGE) {
				return split;
			}

			bucket = (btree_page*) CCH_HANDOFF(tdbb, window, bucket->btr_sibling, LCK_write, pag_index);
		}
	}

	// If we're above the leaf level, find the appropriate node in the chain of sibling pages.
	// Hold on to this position while we recurse down to the next level, in case there's a
	// split at the lower level, in which case we need to insert the new page at this level.
	ULONG page;
	while (true)
	{
		page = find_page(bucket, insertion->iib_key, insertion->iib_descriptor,
						 insertion->iib_number);
		if (page != END_BUCKET) {
			break;
		}
		bucket = (btree_page*) CCH_HANDOFF(tdbb, window, bucket->btr_sibling, LCK_read, pag_index);
	}

	BtrPageGCLock lockCurrent(tdbb);
	lockCurrent.disablePageGC(tdbb, window->win_page);

	// Fetch the page at the next level down.  If the next level is leaf level,
	// fetch for write since we know we are going to write to the page (most likely).
	const PageNumber index = window->win_page;
	CCH_HANDOFF(tdbb, window, page, (SSHORT) ((bucket->btr_level == 1) ? LCK_write : LCK_read),
				pag_index);

	// now recursively try to insert the node at the next level down
	index_insertion propagate;
	BtrPageGCLock lockLower(tdbb);
	propagate.iib_dont_gc_lock = insertion->iib_dont_gc_lock;
	insertion->iib_dont_gc_lock = &lockLower;
	ULONG split = add_node(tdbb, window, insertion, new_key, new_record_number, &page,
						   &propagate.iib_sibling);

	if (split == NO_SPLIT)
	{
		lockCurrent.enablePageGC(tdbb);
		insertion->iib_dont_gc_lock = propagate.iib_dont_gc_lock;
		return NO_SPLIT;
	}

#ifdef DEBUG_BTR_SPLIT
	Firebird::string s;
	s.printf("page %ld splitted. split %ld, right %ld, parent %ld",
		page, split, propagate.iib_sibling, index);
	gds__trace(s.c_str());
#endif

	// The page at the lower level split, so we need to insert a pointer
	// to the new page to the page at this level.
	window->win_page = index;
	bucket = (btree_page*) CCH_FETCH(tdbb, window, LCK_write, pag_index);

	propagate.iib_number = RecordNumber(split);
	propagate.iib_descriptor = insertion->iib_descriptor;
	propagate.iib_relation = insertion->iib_relation;
	propagate.iib_duplicates = NULL;
	propagate.iib_key = new_key;

	// now loop through the sibling pages trying to find the appropriate
	// place to put the pointer to the lower level page--remember that the
	// page we were on could have split while we weren't looking
	ULONG original_page2;
	ULONG sibling_page2;
	while (true)
	{
		split = insert_node(tdbb, window, &propagate, new_key, new_record_number, &original_page2,
							&sibling_page2);

		if (split != NO_VALUE_PAGE) {
			break;
		}
		else {
			bucket = (btree_page*) CCH_HANDOFF(tdbb, window, bucket->btr_sibling, LCK_write, pag_index);
		}
	}

	// the split page on the lower level has been propagated, so we can go back to
	// the page it was split from, and mark it as garbage-collectable now
	lockLower.enablePageGC(tdbb);
	insertion->iib_dont_gc_lock = propagate.iib_dont_gc_lock;

	lockCurrent.enablePageGC(tdbb);

	if (original_page) {
		*original_page = original_page2;
	}
	if (sibling_page) {
		*sibling_page = sibling_page2;
	}
	return split;
}


static void compress(thread_db* tdbb,
					 const dsc* desc,
					 temporary_key* key,
					 USHORT itype,
					 bool isNull, bool descending, USHORT key_type)
{
/**************************************
 *
 *	c o m p r e s s
 *
 **************************************
 *
 * Functional description
 *	Compress a data value into an index key.
 *
 **************************************/
	union {
		INT64_KEY temp_int64_key;
		double temp_double;
		ULONG temp_ulong;
		SLONG temp_slong;
		SINT64 temp_sint64;
		UCHAR temp_char[sizeof(INT64_KEY)];
	} temp;
	bool temp_is_negative = false;
	bool int64_key_op = false;

	// For descending index and new index structure we insert 0xFE at the beginning.
	// This is only done for values which begin with 0xFE (254) or 0xFF (255) and
	// is needed to make a difference between a NULL state and a VALUE.
	// Note! By descending index key is complemented after this compression routine.
	// Further a NULL state is always returned as 1 byte 0xFF (descending index).
	const UCHAR desc_end_value_prefix = 0x01; // ~0xFE
	const UCHAR desc_end_value_check = 0x00; // ~0xFF;

	const Database* dbb = tdbb->getDatabase();

	UCHAR* p = key->key_data;

	if (isNull)
	{
		const UCHAR pad = 0;
		key->key_flags &= ~key_empty;
		// AB: NULL should be threated as lowest value possible.
		//     Therefore don't complement pad when we have an ascending index.
		if (descending)
		{
			// DESC NULLs are stored as 1 byte
			*p++ = pad;
			key->key_length = (p - key->key_data);
		}
		else
			key->key_length = 0; // ASC NULLs are stored with no data

		return;
	}

	if (itype == idx_string || itype == idx_byte_array || itype == idx_metadata ||
		itype >= idx_first_intl_string)
	{
		VaryStr<MAX_KEY> buffer;
		const UCHAR pad = (itype == idx_string) ? ' ' : 0;
		UCHAR* ptr;

		size_t length;

		if (itype >= idx_first_intl_string || itype == idx_metadata)
		{
			DSC to;

			// convert to an international byte array
			to.dsc_dtype = dtype_text;
			to.dsc_flags = 0;
			to.dsc_sub_type = 0;
			to.dsc_scale = 0;
			to.dsc_ttype() = ttype_sort_key;
			to.dsc_length = MIN(MAX_KEY, sizeof(buffer));
			ptr = to.dsc_address = reinterpret_cast<UCHAR*>(buffer.vary_string);
			length = INTL_string_to_key(tdbb, itype, desc, &to, key_type);
		}
		else
			length = MOV_get_string(desc, &ptr, &buffer, MAX_KEY);

		if (length)
		{
			// clear key_empty flag, because length is >= 1
			key->key_flags &= ~key_empty;

			if (length > sizeof(key->key_data))
				length = sizeof(key->key_data);

			if (descending && ((*ptr == desc_end_value_prefix) || (*ptr == desc_end_value_check)))
			{
				*p++ = desc_end_value_prefix;
				if ((length + 1) > sizeof(key->key_data))
					length = sizeof(key->key_data) - 1;
			}

			memcpy(p, ptr, length);
			p += length;
		}
		else
		{
			// Leave key_empty flag, because the string is an empty string
			if (descending && ((pad == desc_end_value_prefix) || (pad == desc_end_value_check)))
				*p++ = desc_end_value_prefix;

			*p++ = pad;
		}

		while (p > key->key_data)
		{
			if (*--p != pad)
				break;
		}

		key->key_length = p + 1 - key->key_data;
		return;
	}

	// The index is numeric.
	//   For idx_numeric...
	//	 Convert the value to a double precision number,
	//   then zap it to compare in a byte-wise order.
	// For idx_numeric2...
	//   Convert the value to a INT64_KEY struct,
	//   then zap it to compare in a byte-wise order.

	// clear key_empty flag for all other types
	key->key_flags &= ~key_empty;

	size_t temp_copy_length = sizeof(double);

	if (itype == idx_numeric)
	{
		temp.temp_double = MOV_get_double(desc);
		temp_is_negative = (temp.temp_double < 0);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "NUMERIC %lg ", temp.temp_double);
#endif
	}
	else if (itype == idx_numeric2)
	{
		int64_key_op = true;
		temp.temp_int64_key = make_int64_key(MOV_get_int64(desc, desc->dsc_scale), desc->dsc_scale);
		temp_copy_length = sizeof(temp.temp_int64_key.d_part);
		temp_is_negative = (temp.temp_int64_key.d_part < 0);

#ifdef DEBUG_INDEXKEY
		print_int64_key(*(const SINT64*) desc->dsc_address,
			desc->dsc_scale, temp.temp_int64_key);
#endif

	}
	else if (itype == idx_timestamp)
	{
		GDS_TIMESTAMP timestamp;
		timestamp = MOV_get_timestamp(desc);
		const ULONG SECONDS_PER_DAY	= 24 * 60 * 60;
		temp.temp_sint64 = ((SINT64) (timestamp.timestamp_date) *
			(SINT64) (SECONDS_PER_DAY * ISC_TIME_SECONDS_PRECISION)) +
			(SINT64) (timestamp.timestamp_time);
		temp_copy_length = sizeof(SINT64);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "TIMESTAMP2: %d:%u ",
				   ((const SLONG*) desc->dsc_address)[0],
				   ((const ULONG*) desc->dsc_address)[1]);
		fprintf(stderr, "TIMESTAMP2: %20" QUADFORMAT "d ", temp.temp_sint64);
#endif

	}
	else if (itype == idx_sql_date)
	{
		temp.temp_slong = MOV_get_sql_date(desc);
		temp_copy_length = sizeof(SLONG);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "DATE %d ", temp.temp_slong);
#endif

	}
	else if (itype == idx_sql_time)
	{
		temp.temp_ulong = MOV_get_sql_time(desc);
		temp_copy_length = sizeof(ULONG);
		temp_is_negative = false;

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "TIME %u ", temp.temp_ulong);
#endif

	}
	else if (desc->dsc_dtype == dtype_timestamp)
	{
		// This is the same as the pre v6 behavior.  Basically, the
		// customer has created a NUMERIC index, and is probing into that
		// index using a TIMESTAMP value.
		// eg:  WHERE anInteger = TIMESTAMP '1998-9-16'
		temp.temp_double = MOV_date_to_double(desc);
		temp_is_negative = (temp.temp_double < 0);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "TIMESTAMP1 special %lg ", temp.temp_double);
#endif

	}
	else if (itype == idx_boolean)
	{
		temp.temp_char[0] = UCHAR(MOV_get_boolean(desc) ? 1 : 0);
		temp_copy_length = sizeof(UCHAR);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "BOOLEAN %d ", temp.temp_char[0]);
#endif

	}
	else
	{
		temp.temp_double = MOV_get_double(desc);
		temp_is_negative = (temp.temp_double < 0);

#ifdef DEBUG_INDEXKEY
		fprintf(stderr, "NUMERIC %lg ", temp.temp_double);
#endif

	}

	// This trick replaces possibly negative zero with positive zero, so that both
	// would be compressed to the same index key and thus properly compared (see CORE-3547).
	if (temp.temp_double == 0)
	{
		temp.temp_double = 0;
	}

#ifdef IEEE

	const UCHAR* q;

#ifndef WORDS_BIGENDIAN

	// For little-endian machines, reverse the order of bytes for the key
	// Copy the first set of bytes into key_data
	size_t length = temp_copy_length;
/*
    AB: Speed things a little up, remember that this is function is called a lot.
	for (q = temp.temp_char + temp_copy_length; length; --length)
	{
		*p++ = *--q;
	}
*/
	q = temp.temp_char + temp_copy_length;
	while (length)
	{
		if (length >= 8)
		{
			q -= 8;
			p[0] = q[7];
			p[1] = q[6];
			p[2] = q[5];
			p[3] = q[4];
			p[4] = q[3];
			p[5] = q[2];
			p[6] = q[1];
			p[7] = q[0];
			p += 8;
			length -= 8;
		}
		else if (length >= 4)
		{
			q -= 4;
			p[0] = q[3];
			p[1] = q[2];
			p[2] = q[1];
			p[3] = q[0];
			p += 4;
			length -= 4;
		}
		else
		{
			*p++ = *--q;
			length--;
		}
	}

	// Copy the next 2 bytes into key_data, if key is of an int64 type
	if (int64_key_op)
	{
		for (q = temp.temp_char + sizeof(double) + sizeof(SSHORT), length = sizeof(SSHORT);
			length; --length)
		{
			*p++ = *--q;
		}
	}
#else
	// For big-endian machines, copy the bytes as laid down
	// Copy the first set of bytes into key_data
	size_t length = temp_copy_length;
	for (q = temp.temp_char; length; --length) {
		*p++ = *q++;
	}

	// Copy the next 2 bytes into key_data, if key is of an int64 type
	if (int64_key_op)
	{
		for (q = temp.temp_char + sizeof(double), length = sizeof(SSHORT); length; --length)
		{
			*p++ = *q++;
		}
	}
#endif // !WORDS_BIGENDIAN


#else // IEEE


	// The conversion from G_FLOAT to D_FLOAT made below was removed because
	// it prevented users from entering otherwise valid numbers into a field
	// which was in an index.   A D_FLOAT has the sign and 7 of 8 exponent
	// bits in the first byte and the remaining exponent bit plus the first
	// 7 bits of the mantissa in the second byte.   For G_FLOATS, the sign
	// and 7 of 11 exponent bits go into the first byte, with the remaining
	// 4 exponent bits going into the second byte, with the first 4 bits of
	// the mantissa.   Why this conversion was done is unknown, but it is
	// of limited utility, being useful for reducing the compressed field
	// length only for those values which have 0 for the last 6 bytes and
	// a nonzero value for the 5-7 bits of the mantissa.


	*p++ = temp.temp_char[1];
	*p++ = temp.temp_char[0];
	*p++ = temp.temp_char[3];
	*p++ = temp.temp_char[2];
	*p++ = temp.temp_char[5];
	*p++ = temp.temp_char[4];
	*p++ = temp.temp_char[7];
	*p++ = temp.temp_char[6];

#error compile_time_failure:
#error Code needs to be written in the non - IEEE floating point case
#error to handle the following:
#error 	a) idx_sql_date, idx_sql_time, idx_timestamp b) idx_numeric2

#endif // IEEE

	// Test the sign of the double precision number.  Just to be sure, don't
	// rely on the byte comparison being signed.  If the number is negative,
	// complement the whole thing.  Otherwise just zap the sign bit.
	if (temp_is_negative)
	{
		((SSHORT *) key->key_data)[0] = -((SSHORT *) key->key_data)[0] - 1;
		((SSHORT *) key->key_data)[1] = -((SSHORT *) key->key_data)[1] - 1;
		((SSHORT *) key->key_data)[2] = -((SSHORT *) key->key_data)[2] - 1;
		((SSHORT *) key->key_data)[3] = -((SSHORT *) key->key_data)[3] - 1;
	}
	else {
		key->key_data[0] ^= 1 << 7;
	}

	if (int64_key_op)
	{
		// Complement the s_part for an int64 key.
		// If we just flip the sign bit, which is equivalent to adding 32768, the
		// short part will unsigned-compare correctly.
		key->key_data[8] ^= 1 << 7;

		//p = &key->key_data[(!int64_key_op) ? temp_copy_length - 1 : INT64_KEY_LENGTH - 1];
		p = &key->key_data[INT64_KEY_LENGTH - 1];
	}
	else {
		p = &key->key_data[temp_copy_length - 1];
	}

	// Finally, chop off trailing binary zeros
	while (!(*p) && (p > key->key_data)) {
		--p;
	}

	key->key_length = (p - key->key_data) + 1;

	// By descending index, check first byte
	q = key->key_data;
	if (descending && (key->key_length >= 1) &&
		((*q == desc_end_value_prefix) || (*q == desc_end_value_check)))
	{
		p = key->key_data;
		p++;
		memmove(p, q, key->key_length);
		key->key_data[0] = desc_end_value_prefix;
		key->key_length++;
	}

#ifdef DEBUG_INDEXKEY
	{
		fprintf(stderr, "temporary_key: length: %d Bytes: ", key->key_length);
		for (int i = 0; i < key->key_length; i++)
			fprintf(stderr, "%02x ", key->key_data[i]);
		fprintf(stderr, "\n");
	}
#endif
}


static USHORT compress_root(thread_db* tdbb, index_root_page* page)
{
/**************************************
 *
 *	c o m p r e s s _ r o o t
 *
 **************************************
 *
 * Functional description
 *	Compress an index root page.
 *
 **************************************/
	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	Firebird::UCharBuffer temp_buffer;
	UCHAR* const temp = temp_buffer.getBuffer(dbb->dbb_page_size);
	memcpy(temp, page, dbb->dbb_page_size);
	UCHAR* p = (UCHAR*) page + dbb->dbb_page_size;

	index_root_page::irt_repeat* root_idx = page->irt_rpt;
	for (const index_root_page::irt_repeat* const end = root_idx + page->irt_count;
		 root_idx < end; root_idx++)
	{
		if (root_idx->irt_root)
		{
			const USHORT len = root_idx->irt_keys * sizeof(irtd);
			p -= len;
			memcpy(p, temp + root_idx->irt_desc, len);
			root_idx->irt_desc = p - (UCHAR*) page;
		}
	}

	return p - (UCHAR*) page;
}


static void copy_key(const temporary_key* in, temporary_key* out)
{
/**************************************
 *
 *	c o p y _ k e y
 *
 **************************************
 *
 * Functional description
 *	Copy a key.
 *
 **************************************/

	out->key_length = in->key_length;
	out->key_flags = in->key_flags;
	memcpy(out->key_data, in->key_data, in->key_length);
}


static contents delete_node(thread_db* tdbb, WIN* window, UCHAR* pointer)
{
/**************************************
 *
 *	d e l e t e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Delete a node from a page and return whether it
 *	empty, if there is a single node on it, or if it
 * 	is above or below the threshold for garbage collection.
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	btree_page* page = (btree_page*) window->win_buffer;

	CCH_MARK(tdbb, window);

	const bool leafPage = (page->btr_level == 0);

	// Read node that need to be removed
	IndexNode removingNode;
	UCHAR* localPointer = removingNode.readNode(pointer, leafPage);
	const USHORT offsetDeletePoint = (pointer - (UCHAR*)page);

	// Read the next node after the removing node
	IndexNode nextNode;
	localPointer = nextNode.readNode(localPointer, leafPage);
	const USHORT offsetNextPoint = (localPointer - (UCHAR*)page);

	// Save data in tempKey so we can rebuild from it
	USHORT newNextPrefix = nextNode.prefix;
	USHORT newNextLength = 0;
	USHORT length = MAX(removingNode.length + removingNode.prefix, nextNode.length + nextNode.prefix);
	HalfStaticArray<UCHAR, MAX_KEY> tempBuf;
	UCHAR* tempData = tempBuf.getBuffer(length);
	length = 0;
	if (nextNode.prefix > removingNode.prefix)
	{
		// The next node uses data from the node that is going to
		// be removed so save it.
		length = nextNode.prefix - removingNode.prefix;
		newNextPrefix -= length;
		newNextLength += length;
		memcpy(tempData, removingNode.data, length);
	}
	memcpy(tempData + length, nextNode.data, nextNode.length);
	newNextLength += nextNode.length;

	// Update the page prefix total.
	page->btr_prefix_total -= (removingNode.prefix + (nextNode.prefix - newNextPrefix));

	// Update the next node so we are ready to save it.
	nextNode.prefix = newNextPrefix;
	nextNode.length = newNextLength;
	nextNode.data = tempData;
	pointer = nextNode.writeNode(pointer, leafPage);
	// below this point tempData contents is not used anymore and buffer may be reused

	// Compute length of rest of bucket and move it down.
	length = page->btr_length - (localPointer - (UCHAR*) page);
	if (length)
	{
		// Could be overlapping buffers.
		// memmove() is guaranteed to work non-destructivly on overlapping buffers.
		memmove(pointer, localPointer, length);
		pointer += length;
		localPointer += length;
	}

	// Set page size and get delta
	USHORT delta = page->btr_length;
	page->btr_length = pointer - (UCHAR*) page;
	delta -= page->btr_length;

	// We use a fast approach here.
	// Only update offsets pointing after the deleted node and
	// remove jump nodes pointing to the deleted node or node
	// next to the deleted one.
	jumpNodeList tmpJumpNodes;
	jumpNodeList* jumpNodes = &tmpJumpNodes;

	pointer = page->btr_nodes;

	// We are going to rebuild jump nodes. In the end of this process we will either have
	// the same jump nodes as before or one jump node less. The jump table size
	// by its definition is a good upper estimate for summary size of all existing
	// jump nodes data length's.
	// After rebuild jump node next after removed one may have new length longer than
	// before rebuild but no longer than length of removed node. All other nodes didn't
	// change its lengths. Therefore the jump table size is valid upper estimate
	// for summary size of all new jump nodes data length's too.
	tempData = tempBuf.getBuffer(page->btr_jump_size);
	UCHAR* const tempEnd = tempBuf.end();

	bool rebuild = false;
	UCHAR n = page->btr_jump_count;
	IndexJumpNode jumpNode, delJumpNode;
	while (n)
	{
		pointer = jumpNode.readJumpNode(pointer);
		// Jump nodes pointing to the deleted node are removed.
		if ((jumpNode.offset < offsetDeletePoint) || (jumpNode.offset > offsetNextPoint))
		{
			IndexJumpNode newJumpNode;
			if (rebuild && jumpNode.prefix > delJumpNode.prefix)
			{
				// This node has prefix against a removing jump node
				const USHORT addLength = jumpNode.prefix - delJumpNode.prefix;
				newJumpNode.prefix = jumpNode.prefix - addLength;
				newJumpNode.length = jumpNode.length + addLength;
				newJumpNode.offset = jumpNode.offset;
				if (jumpNode.offset > offsetDeletePoint) {
					newJumpNode.offset -= delta;
				}
				newJumpNode.data = tempData;
				tempData += newJumpNode.length;
				fb_assert(tempData < tempEnd);

				memcpy(newJumpNode.data, delJumpNode.data, addLength);
				memcpy(newJumpNode.data + addLength, jumpNode.data, jumpNode.length);
			}
			else
			{
				newJumpNode.prefix = jumpNode.prefix;
				newJumpNode.length = jumpNode.length;
				newJumpNode.offset = jumpNode.offset;
				if (jumpNode.offset > offsetDeletePoint) {
					newJumpNode.offset -= delta;
				}
				newJumpNode.data = tempData;
				tempData += newJumpNode.length;
				fb_assert(tempData < tempEnd);
				memcpy(newJumpNode.data, jumpNode.data, newJumpNode.length);
			}
			jumpNodes->add(newJumpNode);
			rebuild = false;
		}
		else
		{
			delJumpNode = jumpNode;
			rebuild = true;
		}
		n--;
	}

	// Update jump information
	page->btr_jump_count = (UCHAR) jumpNodes->getCount();

	// Write jump nodes
	pointer = page->btr_nodes;
	IndexJumpNode* walkJumpNode = jumpNodes->begin();
	for (size_t i = 0; i < jumpNodes->getCount(); i++) {
		pointer = walkJumpNode[i].writeJumpNode(pointer);
	}
	jumpNodes->clear();

	// check to see if the page is now empty
	pointer = page->btr_nodes + page->btr_jump_size;
	IndexNode node;
	pointer = node.readNode(pointer, leafPage);
	if (node.isEndBucket || node.isEndLevel) {
		return contents_empty;
	}

	// check to see if there is just one node
	pointer = node.readNode(pointer, leafPage);
	if (node.isEndBucket ||	node.isEndLevel) {
		return contents_single;
	}

	// check to see if the size of the page is below the garbage collection threshold,
	// meaning below the size at which it should be merged with its left sibling if possible.
	if (page->btr_length < GARBAGE_COLLECTION_BELOW_THRESHOLD) {
		return contents_below_threshold;
	}

	return contents_above_threshold;
}


static void delete_tree(thread_db* tdbb,
						USHORT rel_id, USHORT idx_id, PageNumber next, PageNumber prior)
{
/**************************************
 *
 *	d e l e t e _ t r e e
 *
 **************************************
 *
 * Functional description
 *	Release index pages back to free list.
 *
 **************************************/

	SET_TDBB(tdbb);
	WIN window(next.getPageSpaceID(), -1);
	window.win_flags = WIN_large_scan;
	window.win_scans = 1;

	ULONG down = next.getPageNum();
	// Delete the index tree from the top down.
	while (next.getPageNum())
	{
		window.win_page = next;
		btree_page* page = (btree_page*) CCH_FETCH(tdbb, &window, LCK_write, 0);

		// do a little defensive programming--if any of these conditions
		// are true we have a damaged pointer, so just stop deleting. At
		// the same time, allow updates of indexes with id > 255 even though
		// the page header uses a byte for its index id.  This requires relaxing
		// the check slightly introducing a risk that we'll pick up a page belonging
		// to some other index that is ours +/- (256*n).  On the whole, unlikely.
		if (page->btr_header.pag_type != pag_index ||
			page->btr_id != (UCHAR)(idx_id % 256) || page->btr_relation != rel_id)
		{
			CCH_RELEASE(tdbb, &window);
			return;
		}

		// if we are at the beginning of a non-leaf level, position
		// "down" to the beginning of the next level down
		if (next.getPageNum() == down)
		{
			if (page->btr_level)
			{
				UCHAR* pointer = page->btr_nodes + page->btr_jump_size;
				IndexNode pageNode;
				pageNode.readNode(pointer, false);
				down = pageNode.pageNumber;
			}
			else {
				down = 0;
			}
		}

		// go through all the sibling pages on this level and release them
		next = page->btr_sibling;
		CCH_RELEASE_TAIL(tdbb, &window);
		PAG_release_page(tdbb, window.win_page, prior);
		prior = window.win_page;

		// if we are at end of level, go down to the next level
		if (!next.getPageNum()) {
			next = down;
		}
	}
}


static DSC* eval(thread_db* tdbb, const ValueExprNode* node, DSC* temp, bool* isNull)
{
/**************************************
 *
 *	e v a l
 *
 **************************************
 *
 * Functional description
 *	Evaluate an expression returning a descriptor, and
 *	a flag to indicate a null value.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	dsc* desc = EVL_expr(tdbb, request, node);
	*isNull = false;

	if (desc && !(request->req_flags & req_null))
		return desc;

	*isNull = true;

	temp->dsc_dtype = dtype_text;
	temp->dsc_flags = 0;
	temp->dsc_sub_type = 0;
	temp->dsc_scale = 0;
	temp->dsc_length = 1;
	temp->dsc_ttype() = ttype_ascii;
	temp->dsc_address = (UCHAR*) " ";

	return temp;
}


static ULONG fast_load(thread_db* tdbb,
					   jrd_rel* relation,
					   index_desc* idx,
					   USHORT key_length,
					   AutoPtr<Sort>& scb,
					   SelectivityList& selectivity)
{
/**************************************
 *
 *	f a s t _ l o a d
 *
 **************************************
 *
 * Functional description
 *	Do a fast load.  The indices have already been passed into sort, and
 *	are ripe for the plucking.  This beast is complicated, but, I hope,
 *	comprehendable.
 *
 **************************************/
 	temporary_key keys[MAX_LEVELS];
	btree_page* buckets[MAX_LEVELS];
	win_for_array windows[MAX_LEVELS];
	ULONG split_pages[MAX_LEVELS];
	RecordNumber split_record_numbers[MAX_LEVELS];
	UCHAR* pointers[MAX_LEVELS];
	UCHAR* newAreaPointers[MAX_LEVELS];
	USHORT totalJumpSize[MAX_LEVELS];
	IndexNode levelNode[MAX_LEVELS];
#ifdef DEBUG_BTR_PAGES
	TEXT debugtext[1024];
	//  ,__FILE__, __LINE__
#endif

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const USHORT pageSpaceID = relation->getPages(tdbb)->rel_pg_space_id;
	// Variable initialization
	for (int i = 0; i < MAX_LEVELS; i++)
	{
		keys[i].key_flags = 0;
		keys[i].key_length = 0;
		levelNode[i].setNode();

		windows[i].win_page.setPageSpaceID(pageSpaceID);
		windows[i].win_bdb = NULL;
	}

	// leaf-page and pointer-page size limits, we always need to
	// leave room for the END_LEVEL node.
	const USHORT lp_fill_limit = dbb->dbb_page_size - BTN_LEAF_SIZE;
	const USHORT pp_fill_limit = dbb->dbb_page_size - BTN_PAGE_SIZE;

	// Jump information initialization

	typedef Firebird::Array<jumpNodeList*> jumpNodeListContainer;
	jumpNodeListContainer jumpNodes;
	jumpNodes.push(FB_NEW(*tdbb->getDefaultPool()) jumpNodeList(*tdbb->getDefaultPool()));

	keyList jumpKeys;
	jumpKeys.push(FB_NEW(*tdbb->getDefaultPool()) dynKey);
	jumpKeys[0]->keyData = FB_NEW(*tdbb->getDefaultPool()) UCHAR[key_length];

	// AB: Let's try to determine to size between the jumps to speed up
	// index search. Of course the size depends on the key_length. The
	// bigger the key, the less jumps we can make. (Although we must
	// not forget that mostly the keys are compressed and much smaller
	// than the maximum possible key!).
	// These values can easily change without effect on previous created
	// indices, cause this value is stored on each page.
	// Remember, the lower the value how more jumpkeys are generated and
	// how faster jumpkeys are recalculated on insert.

	const USHORT jumpAreaSize = 512 + ((int) sqrt((float) key_length) * 16);

	//  key_size  |  jumpAreaSize
	//  ----------+-----------------
	//         4  |    544
	//         8  |    557
	//        16  |    576
	//        64  |    640
	//       128  |    693
	//       256  |    768

	WIN* window = NULL;
	bool error = false;
	FB_UINT64 count = 0;
	FB_UINT64 duplicates = 0;
	const bool descending = (idx->idx_flags & idx_descending);
	const ULONG segments = idx->idx_count;

	// hvlad: look at IDX_create_index for explanations about NULL indicator below
	const int nullIndLen = !descending && (idx->idx_count == 1) ? 1 : 0;

	Firebird::HalfStaticArray<FB_UINT64, 4> duplicatesList;

	try
	{
		// Allocate and format the first leaf level bucket.  Awkwardly,
		// the bucket header has room for only a byte of index id and that's
		// part of the ODS.  So, for now, we'll just record the first byte
		// of the id and hope for the best.  Index buckets are (almost) always
		// located through the index structure (dmp being an exception used
		// only for debug) so the id is actually redundant.
		btree_page* bucket = (btree_page*) DPM_allocate(tdbb, &windows[0]);
		bucket->btr_header.pag_type = pag_index;
		bucket->btr_relation = relation->rel_id;
		bucket->btr_id = (UCHAR)(idx->idx_id % 256);
		bucket->btr_level = 0;
		bucket->btr_length = BTR_SIZE;
		bucket->btr_jump_interval = jumpAreaSize;
		bucket->btr_jump_size = 0;
		bucket->btr_jump_count = 0;
#ifdef DEBUG_BTR_PAGES
		sprintf(debugtext, "\t new page (%d)", windows[0].win_page);
		gds__log(debugtext);
#endif

		UCHAR* pointer = bucket->btr_nodes;
		newAreaPointers[0] = pointer + jumpAreaSize;

		tdbb->tdbb_flags |= TDBB_no_cache_unwind;

		buckets[0] = bucket;
		buckets[1] = NULL;

		duplicatesList.grow(segments);
		memset(duplicatesList.begin(), 0, segments * sizeof(FB_UINT64));

		// If there's an error during index construction, fall
		// thru to release the last index bucket at each level
		// of the index. This will prepare for a single attempt
		// to deallocate the index pages for reuse.
		IndexNode newNode;
		IndexNode previousNode;
		// pointer holds the "main" pointer for inserting new nodes.

		win_for_array split_window;
		split_window.win_page.setPageSpaceID(pageSpaceID);

		temporary_key split_key, temp_key;
		split_key.key_flags = 0;
		split_key.key_length = 0;
		temp_key.key_flags = 0;
		temp_key.key_length = 0;
		dynKey* jumpKey = jumpKeys[0];
		jumpNodeList* leafJumpNodes = jumpNodes[0];
		bool duplicate = false;
		totalJumpSize[0] = 0;

		IndexNode tempNode;
		jumpKey->keyLength = 0;

		while (!error)
		{
			// Get the next record in sorted order.

			UCHAR* record;
			scb->get(tdbb, reinterpret_cast<ULONG**>(&record));

			if (!record) {
				break;
			}
			index_sort_record* isr = (index_sort_record*) (record + key_length);
			count++;
			record += nullIndLen;

			// restore previous values
			bucket = buckets[0];
			split_pages[0] = 0;
			temporary_key* key = &keys[0];

			// Compute the prefix as the length in common with the previous record's key.
			USHORT prefix =
				IndexNode::computePrefix(key->key_data, key->key_length, record, isr->isr_key_length);

			// set node values
			newNode.setNode(prefix, isr->isr_key_length - prefix,
						    RecordNumber(isr->isr_record_number));
			newNode.data = record + prefix;

			// If the length of the new node will cause us to overflow the bucket,
			// form a new bucket.
			if (bucket->btr_length + totalJumpSize[0] +
				newNode.getNodeSize(true) > lp_fill_limit)
			{
				// mark the end of the previous page
				const RecordNumber lastRecordNumber = previousNode.recordNumber;
				previousNode.readNode(previousNode.nodePointer, true);
				previousNode.setEndBucket();
				pointer = previousNode.writeNode(previousNode.nodePointer, true, false);
				bucket->btr_length = pointer - (UCHAR*) bucket;

				if (totalJumpSize[0])
				{
					// Slide down current nodes;
					// CVC: Warning, this may overlap. It seems better to use
					// memmove or to ensure manually that totalJumpSize[0] > l
					// Also, "sliding down" here is moving contents higher in memory.
					const USHORT l = bucket->btr_length - BTR_SIZE;
					memmove(bucket->btr_nodes + totalJumpSize[0], bucket->btr_nodes, l);

					// Update JumpInfo
					if (leafJumpNodes->getCount() > 255) {
						BUGCHECK(205);	// msg 205 index bucket overfilled
					}
					bucket->btr_jump_interval = jumpAreaSize;
					bucket->btr_jump_size = totalJumpSize[0];
					bucket->btr_jump_count = (UCHAR) leafJumpNodes->getCount();

					// Write jumpnodes on page.
					pointer = bucket->btr_nodes;
					IndexJumpNode* walkJumpNode = leafJumpNodes->begin();
					for (size_t i = 0; i < leafJumpNodes->getCount(); i++)
					{
						// Update offset position first.
						walkJumpNode[i].offset += totalJumpSize[0];
						pointer = walkJumpNode[i].writeJumpNode(pointer);
					}
					bucket->btr_length += totalJumpSize[0];
				}

				if (bucket->btr_length > dbb->dbb_page_size) {
					BUGCHECK(205);	// msg 205 index bucket overfilled
				}

				// Allocate new bucket.
				btree_page* split = (btree_page*) DPM_allocate(tdbb, &split_window);
				bucket->btr_sibling = split_window.win_page.getPageNum();
				split->btr_left_sibling = windows[0].win_page.getPageNum();
				split->btr_header.pag_type = pag_index;
				split->btr_relation = bucket->btr_relation;
				split->btr_level = bucket->btr_level;
				split->btr_id = bucket->btr_id;
				split->btr_jump_interval = bucket->btr_jump_interval;
				split->btr_jump_size = 0;
				split->btr_jump_count = 0;
#ifdef DEBUG_BTR_PAGES
				sprintf(debugtext, "\t new page (%d), left page (%d)",
					split_window.win_page, split->btr_left_sibling);
				gds__log(debugtext);
#endif

				// Reset position and size for generating jumpnode
				pointer = split->btr_nodes;
				newAreaPointers[0] = pointer + jumpAreaSize;
				totalJumpSize[0] = 0;
				jumpKey->keyLength = 0;

				// store the first node on the split page
				IndexNode splitNode;
				splitNode.setNode(0, key->key_length, lastRecordNumber);
				splitNode.data = key->key_data;
				pointer = splitNode.writeNode(pointer, true);
				previousNode = splitNode;

				// save the page number of the previous page and release it
				split_pages[0] = windows[0].win_page.getPageNum();
				split_record_numbers[0] = splitNode.recordNumber;
				CCH_RELEASE(tdbb, &windows[0]);
#ifdef DEBUG_BTR_PAGES
				sprintf(debugtext, "\t release page (%d), left page (%d), right page (%d)",
					windows[0].win_page,
					((btr*)windows[0].win_buffer)->btr_left_sibling,
					((btr*)windows[0].win_buffer)->btr_sibling);
				gds__log(debugtext);
#endif

				// set up the new page as the "current" page
				windows[0] = split_window;
				buckets[0] = bucket = split;

				// save the first key on page as the page to be propagated
				copy_key(key, &split_key);

				// Clear jumplist.
				IndexJumpNode* walkJumpNode = leafJumpNodes->begin();
				for (size_t i = 0; i < leafJumpNodes->getCount(); i++) {
					delete[] walkJumpNode[i].data;
				}
				leafJumpNodes->clear();
			}

			// Insert the new node in the current bucket
			bucket->btr_prefix_total += prefix;
			pointer = newNode.writeNode(pointer, true);
			previousNode = newNode;

			// if we have a compound-index calculate duplicates per segment.
			if (segments > 1 && count > 1)
			{
				// Initialize variables for segment duplicate check.
				// count holds the current checking segment (starting by
				// the maximum segment number to 1).
				const UCHAR* p1 = key->key_data;
				const UCHAR* const p1_end = p1 + key->key_length;
				const UCHAR* p2 = newNode.data;
				const UCHAR* const p2_end = p2 + newNode.length;
				SSHORT segment, stuff_count;
				if (newNode.prefix == 0)
				{
					segment = *p2;
					//pos = 0;
					stuff_count = 0;
				}
				else
				{
					const SSHORT pos = newNode.prefix;
					// find the segment number were we're starting.
					const SSHORT i = (pos / (STUFF_COUNT + 1)) * (STUFF_COUNT + 1);
					if (i == pos)
					{
						// We _should_ pick number from data if available
						segment = *p2;
					}
					else {
						segment = *(p1 + i);
					}
					// update stuff_count to the current position.
					stuff_count = STUFF_COUNT + 1 - (pos - i);
					p1 += pos;
				}

				//Look for duplicates in the segments
				while ((p1 < p1_end) && (p2 < p2_end))
				{
					if (stuff_count == 0)
					{
						if (*p1 != *p2)
						{
							// We're done
							break;
						}
						segment = *p2;
						p1++;
						p2++;
						stuff_count = STUFF_COUNT;
					}
					if (*p1 != *p2)
					{
						//We're done
						break;
					}
					p1++;
					p2++;
					stuff_count--;
				}

				// For descending indexes the segment-number is also
				// complemented, thus reverse it back.
				// Note: values are complemented per UCHAR base.
				if (descending) {
					segment = (255 - segment);
				}

				if ((p1 == p1_end) && (p2 == p2_end)) {
					segment = 0; // All segments are duplicates
				}

				for (ULONG i = segment + 1; i <= segments; i++) {
					duplicatesList[segments - i]++;
				}

			}

			// check if this is a duplicate node
			duplicate = (!newNode.length && prefix == key->key_length);
			if (duplicate && (count > 1)) {
				++duplicates;
			}

			// Update the length of the page.
			bucket->btr_length = pointer - (UCHAR*) bucket;
			if (bucket->btr_length > dbb->dbb_page_size) {
				BUGCHECK(205);		// msg 205 index bucket overfilled
			}

			// Remember the last key inserted to compress the next one.
			key->key_length = isr->isr_key_length;
			memcpy(key->key_data, record, key->key_length);

			if (newAreaPointers[0] < pointer)
			{
				// Create a jumpnode
				IndexJumpNode jumpNode;
				jumpNode.prefix = IndexNode::computePrefix(jumpKey->keyData,
					jumpKey->keyLength, key->key_data, newNode.prefix);
				jumpNode.length = newNode.prefix - jumpNode.prefix;

				const USHORT jumpNodeSize = jumpNode.getJumpNodeSize();
				// Ensure the new jumpnode fits in the bucket
				if (bucket->btr_length + totalJumpSize[0] + jumpNodeSize < lp_fill_limit)
				{
					// Initialize the rest of the jumpnode
					jumpNode.offset = (newNode.nodePointer - (UCHAR*)bucket);
					jumpNode.data = FB_NEW(*tdbb->getDefaultPool()) UCHAR[jumpNode.length];
					memcpy(jumpNode.data, key->key_data + jumpNode.prefix, jumpNode.length);
					// Push node on end in list
					leafJumpNodes->add(jumpNode);
					// Store new data in jumpKey, so a new jump node can calculate prefix
					memcpy(jumpKey->keyData + jumpNode.prefix, jumpNode.data, jumpNode.length);
					jumpKey->keyLength = jumpNode.length + jumpNode.prefix;
					// Set new position for generating jumpnode
					newAreaPointers[0] += jumpAreaSize;
					totalJumpSize[0] += jumpNodeSize;
				}
			}

			// If there wasn't a split, we're done.  If there was, propagate the
			// split upward
			for (ULONG level = 1; split_pages[level - 1]; level++)
			{
				// initialize the current pointers for this level
				window = &windows[level];
				key = &keys[level];
				split_pages[level] = 0;
				UCHAR* levelPointer = pointers[level];

				// If there isn't already a bucket at this level, make one.  Remember to
				// shorten the index id to a byte
				if (!(bucket = buckets[level]))
				{
					buckets[level + 1] = NULL;
					buckets[level] = bucket = (btree_page*) DPM_allocate(tdbb, window);
					bucket->btr_header.pag_type = pag_index;
					bucket->btr_relation = relation->rel_id;
					bucket->btr_id = (UCHAR)(idx->idx_id % 256);
					fb_assert(level <= MAX_UCHAR);
					bucket->btr_level = (UCHAR) level;
					bucket->btr_jump_interval = jumpAreaSize;
					bucket->btr_jump_size = 0;
					bucket->btr_jump_count = 0;
#ifdef DEBUG_BTR_PAGES
					sprintf(debugtext, "\t new page (%d)", window->win_page);
					gds__log(debugtext);
#endif

					// since this is the beginning of the level, we propagate the lower-level
					// page with a "degenerate" zero-length node indicating that this page holds
					// any key value less than the next node

					levelPointer = bucket->btr_nodes;

					// First record-number of level must be zero
					levelNode[level].setNode(0, 0, RecordNumber(0), split_pages[level - 1]);
					levelPointer = levelNode[level].writeNode(levelPointer, false);
					bucket->btr_length = levelPointer - (UCHAR*) bucket;
					key->key_length = 0;

					// Initialize jumpNodes variables for new level
					jumpNodes.push(FB_NEW(*tdbb->getDefaultPool()) jumpNodeList(*tdbb->getDefaultPool()));
					jumpKeys.push(FB_NEW(*tdbb->getDefaultPool()) dynKey);
					jumpKeys[level]->keyLength = 0;
					jumpKeys[level]->keyData = FB_NEW(*tdbb->getDefaultPool()) UCHAR[key_length];
					totalJumpSize[level] = 0;
					newAreaPointers[level] = levelPointer + jumpAreaSize;
				}

				dynKey* pageJumpKey = jumpKeys[level];
				jumpNodeList* pageJumpNodes = jumpNodes[level];

				// Compute the prefix in preparation of insertion
				prefix = IndexNode::computePrefix(key->key_data, key->key_length,
					split_key.key_data, split_key.key_length);

				// Remember the last key inserted to compress the next one.
				copy_key(&split_key, &temp_key);

				// Save current node if we need to split.
				tempNode = levelNode[level];
				// Set new node values.
				levelNode[level].setNode(prefix, temp_key.key_length - prefix,
					split_record_numbers[level - 1], windows[level - 1].win_page.getPageNum());
				levelNode[level].data = temp_key.key_data + prefix;

				// See if the new node fits in the current bucket.
				// If not, split the bucket.
				if (bucket->btr_length + totalJumpSize[level] +
					levelNode[level].getNodeSize(false) > pp_fill_limit)
				{
					// mark the end of the page; note that the end_bucket marker must
					// contain info about the first node on the next page
					const SLONG lastPageNumber = tempNode.pageNumber;
					tempNode.readNode(tempNode.nodePointer, false);
					tempNode.setEndBucket();
					levelPointer = tempNode.writeNode(tempNode.nodePointer, false, false);
					bucket->btr_length = levelPointer - (UCHAR*)bucket;

					if (totalJumpSize[level])
					{
						// Slide down current nodes;
						// CVC: Warning, this may overlap. It seems better to use
						// memmove or to ensure manually that totalJumpSize[0] > l
						// Also, "sliding down" here is moving contents higher in memory.
						const USHORT l = bucket->btr_length - BTR_SIZE;
						memmove(bucket->btr_nodes + totalJumpSize[level], bucket->btr_nodes, l);

						// Update JumpInfo
						if (pageJumpNodes->getCount() > 255) {
							BUGCHECK(205);	// msg 205 index bucket overfilled
						}
						bucket->btr_jump_interval = jumpAreaSize;
						bucket->btr_jump_size = totalJumpSize[level];
						bucket->btr_jump_count = (UCHAR) pageJumpNodes->getCount();

						// Write jumpnodes on page.
						levelPointer = bucket->btr_nodes;
						IndexJumpNode* walkJumpNode = pageJumpNodes->begin();
						for (size_t i = 0; i < pageJumpNodes->getCount(); i++)
						{
							// Update offset position first.
							walkJumpNode[i].offset += totalJumpSize[level];
							levelPointer = walkJumpNode[i].writeJumpNode(levelPointer);
						}
						bucket->btr_length += totalJumpSize[level];
					}

					if (bucket->btr_length > dbb->dbb_page_size) {
						BUGCHECK(205);	// msg 205 index bucket overfilled
					}

					btree_page* split = (btree_page*) DPM_allocate(tdbb, &split_window);
					bucket->btr_sibling = split_window.win_page.getPageNum();
					split->btr_left_sibling = window->win_page.getPageNum();
					split->btr_header.pag_type = pag_index;
					split->btr_relation = bucket->btr_relation;
					split->btr_level = bucket->btr_level;
					split->btr_id = bucket->btr_id;
					split->btr_jump_interval = bucket->btr_jump_interval;
					split->btr_jump_size = 0;
					split->btr_jump_count = 0;
#ifdef DEBUG_BTR_PAGES
					sprintf(debugtext, "\t new page (%d), left page (%d)",
						split_window.win_page, split->btr_left_sibling);
					gds__log(debugtext);
#endif

					levelPointer = split->btr_nodes;
					// Reset position and size for generating jumpnode
					newAreaPointers[level] = levelPointer + jumpAreaSize;
					totalJumpSize[level] = 0;
					pageJumpKey->keyLength = 0;

					// insert the new node in the new bucket
					IndexNode splitNode;
					splitNode.setNode(0, key->key_length, tempNode.recordNumber, lastPageNumber);
					splitNode.data = key->key_data;
					levelPointer = splitNode.writeNode(levelPointer, false);
					tempNode = splitNode;

					// indicate to propagate the page we just split from
					split_pages[level] = window->win_page.getPageNum();
					split_record_numbers[level] = splitNode.recordNumber;
					CCH_RELEASE(tdbb, window);
#ifdef DEBUG_BTR_PAGES
					sprintf(debugtext, "\t release page (%d), left page (%d), right page (%d)",
						window->win_page,
						((btr*)window->win_buffer)->btr_left_sibling,
						((btr*)window->win_buffer)->btr_sibling);
					gds__log(debugtext);
#endif

					// and make the new page the current page
					*window = split_window;
					buckets[level] = bucket = split;
					copy_key(key, &split_key);

					// Clear jumplist.
					IndexJumpNode* walkJumpNode = pageJumpNodes->begin();
					for (size_t i = 0; i < pageJumpNodes->getCount(); i++) {
						delete[] walkJumpNode[i].data;
					}
					pageJumpNodes->clear();
				}

				// Now propagate up the lower-level bucket by storing a "pointer" to it.
				bucket->btr_prefix_total += prefix;
				levelPointer = levelNode[level].writeNode(levelPointer, false);

				// Update the length of the page.
				bucket->btr_length = levelPointer - (UCHAR*) bucket;
				if (bucket->btr_length > dbb->dbb_page_size) {
					BUGCHECK(205);	// msg 205 index bucket overfilled
				}

				if (newAreaPointers[level] < levelPointer)
				{
					// Create a jumpnode
					IndexJumpNode jumpNode;
					jumpNode.prefix = IndexNode::computePrefix(pageJumpKey->keyData,
															   pageJumpKey->keyLength,
															   temp_key.key_data,
															   levelNode[level].prefix);
					jumpNode.length = levelNode[level].prefix - jumpNode.prefix;

					const USHORT jumpNodeSize = jumpNode.getJumpNodeSize();
					// Ensure the new jumpnode fits in the bucket
					if (bucket->btr_length + totalJumpSize[level] + jumpNodeSize < pp_fill_limit)
					{
						// Initialize the rest of the jumpnode
						jumpNode.offset = (levelNode[level].nodePointer - (UCHAR*)bucket);
						jumpNode.data = FB_NEW(*tdbb->getDefaultPool()) UCHAR[jumpNode.length];
						memcpy(jumpNode.data, temp_key.key_data + jumpNode.prefix, jumpNode.length);
						// Push node on end in list
						pageJumpNodes->add(jumpNode);
						// Store new data in jumpKey, so a new jump node can calculate prefix
						memcpy(pageJumpKey->keyData + jumpNode.prefix, jumpNode.data, jumpNode.length);
						pageJumpKey->keyLength = jumpNode.length + jumpNode.prefix;
						// Set new position for generating jumpnode
						newAreaPointers[level] += jumpAreaSize;
						totalJumpSize[level] += jumpNodeSize;
					}
				}

				// Now restore the current key value and save this node as the
				// current node on this level; also calculate the new page length.
				copy_key(&temp_key, key);
				pointers[level] = levelPointer;
			}

			if (--tdbb->tdbb_quantum < 0)
				error = JRD_reschedule(tdbb, 0, false);
		}

		// To finish up, put an end of level marker on the last bucket
		// of each level.
		for (ULONG level = 0; (bucket = buckets[level]); level++)
		{
			// retain the top level window for returning to the calling routine
			const bool leafPage = (bucket->btr_level == 0);
			window = &windows[level];

			// store the end of level marker
			pointer = (UCHAR*)bucket + bucket->btr_length;
			levelNode[level].setEndLevel();
			pointer = levelNode[level].writeNode(pointer, leafPage);

			// and update the final page length
			bucket->btr_length = pointer - (UCHAR*)bucket;
			if (bucket->btr_length > dbb->dbb_page_size) {
				BUGCHECK(205);		// msg 205 index bucket overfilled
			}

			// Store jump nodes on page if needed.
			jumpNodeList* pageJumpNodes = jumpNodes[level];
			if (totalJumpSize[level])
			{
				// Slide down current nodes;
				// CVC: Warning, this may overlap. It seems better to use
				// memmove or to ensure manually that totalJumpSize[0] > l
				// Also, "sliding down" here is moving contents higher in memory.
				const USHORT l = bucket->btr_length - BTR_SIZE;
				memmove(bucket->btr_nodes + totalJumpSize[level], bucket->btr_nodes, l);

				// Update JumpInfo
				if (pageJumpNodes->getCount() > 255) {
					BUGCHECK(205);	// msg 205 index bucket overfilled
				}
				bucket->btr_jump_interval = jumpAreaSize;
				bucket->btr_jump_size = totalJumpSize[level];
				bucket->btr_jump_count = (UCHAR) pageJumpNodes->getCount();

				// Write jumpnodes on page.
				pointer = bucket->btr_nodes;
				IndexJumpNode* walkJumpNode = pageJumpNodes->begin();
				for (size_t i = 0; i < pageJumpNodes->getCount(); i++)
				{
					// Update offset position first.
					walkJumpNode[i].offset += totalJumpSize[level];
					pointer = walkJumpNode[i].writeJumpNode(pointer);
				}

				bucket->btr_length += totalJumpSize[level];
			}

			if (bucket->btr_length > dbb->dbb_page_size) {
				BUGCHECK(205);	// msg 205 index bucket overfilled
			}

			CCH_RELEASE(tdbb, &windows[level]);
#ifdef DEBUG_BTR_PAGES
			sprintf(debugtext, "\t release page (%d), left page (%d), right page (%d)",
				windows[level].win_page,
				((btr*)windows[level].win_buffer)->btr_left_sibling,
				((btr*)windows[level].win_buffer)->btr_sibling);
			gds__log(debugtext);
#endif
		}

		// Finally clean up dynamic memory used.
		for (jumpNodeListContainer::iterator itr = jumpNodes.begin(); itr < jumpNodes.end(); ++itr)
		{
			jumpNodeList* freeJumpNodes = *itr;
			IndexJumpNode* walkJumpNode = freeJumpNodes->begin();
			for (size_t i = 0; i < freeJumpNodes->getCount(); i++) {
				delete[] walkJumpNode[i].data;
			}
			freeJumpNodes->clear();
			delete freeJumpNodes;
		}
		for (keyList::iterator itr = jumpKeys.begin(); itr < jumpKeys.end(); ++itr)
		{
			delete[] (*itr)->keyData;
			delete *itr;
		}
	}	// try
	catch (const Firebird::Exception& ex)
	{
		ex.stuff_exception(tdbb->tdbb_status_vector);
		error = true;
	}

	tdbb->tdbb_flags &= ~TDBB_no_cache_unwind;

	// do some final housekeeping

	scb.reset();

	// If index flush fails, try to delete the index tree.
	// If the index delete fails, just go ahead and punt.
	try
	{
		if (error)
			ERR_punt();

		CCH_flush(tdbb, FLUSH_ALL, 0);

		// Calculate selectivity, also per segment when newer ODS
		selectivity.grow(segments);
		if (segments > 1)
		{
			for (ULONG i = 0; i < segments; i++) {
				selectivity[i] = (float) (count ? 1.0 / (float) (count - duplicatesList[i]) : 0.0);
			}
		}
		else {
			selectivity[0] = (float) (count ? (1.0 / (float) (count - duplicates)) : 0.0);
		}
	}	// try
	catch (const Firebird::Exception& ex)
	{
		ex.stuff_exception(tdbb->tdbb_status_vector);

		// CCH_unwind does not released page buffers (as we
		// set TDBB_no_cache_unwind flag), do it now
		for (int i = 0; i < MAX_LEVELS; i++)
		{
			if (windows[i].win_bdb)
				CCH_RELEASE(tdbb, &windows[i]);
		}

		if (window)
		{
			delete_tree(tdbb, relation->rel_id, idx->idx_id,
						window->win_page, PageNumber(window->win_page.getPageSpaceID(), 0));
		}

		throw;
	}

	return window->win_page.getPageNum();
}


static index_root_page* fetch_root(thread_db* tdbb, WIN* window, const jrd_rel* relation,
								   const RelationPages* relPages)
{
/**************************************
 *
 *	f e t c h _ r o o t
 *
 **************************************
 *
 * Functional description
 *	Return descriptions of all indices for relation.  If there isn't
 *	a known index root, assume we were called during optimization
 *	and return no indices.
 *
 **************************************/
	SET_TDBB(tdbb);

	if ((window->win_page = relPages->rel_index_root) == 0)
	{
		if (relation->rel_id == 0)
			return NULL;

		DPM_scan_pages(tdbb);

		if (!relPages->rel_index_root)
			return NULL;

		window->win_page = relPages->rel_index_root;
	}

	return (index_root_page*) CCH_FETCH(tdbb, window, LCK_read, pag_root);
}


static UCHAR* find_node_start_point(btree_page* bucket, temporary_key* key,
									UCHAR* value,
									USHORT* return_value, bool descending,
									bool retrieval, bool pointer_by_marker,
									RecordNumber find_record_number)
{
/**************************************
 *
 *	f i n d _ n o d e _ s t a r t _ p o i n t
 *
 **************************************
 *
 * Functional description
 *	Locate and return a pointer to the insertion point.
 *	If the key doesn't belong in this bucket, return NULL.
 *	A flag indicates the index is descending.
 *
 **************************************/

	USHORT prefix = 0;
	const UCHAR* const key_end = key->key_data + key->key_length;
	bool firstPass = true;
	const bool leafPage = (bucket->btr_level == 0);
	const UCHAR* const endPointer = (UCHAR*)bucket + bucket->btr_length;

	// Find point where we can start search.
	UCHAR* pointer = find_area_start_point(bucket, key, value, &prefix, descending, retrieval,
										   find_record_number);
	const UCHAR* p = key->key_data + prefix;

	IndexNode node;
	pointer = node.readNode(pointer, leafPage);
	// Check if pointer is still valid
	if (pointer > endPointer) {
		BUGCHECK(204);	// msg 204 index inconsistent
	}

	// If this is an non-leaf bucket of a descending index, the dummy node on the
	// front will trip us up.  NOTE: This code may be apocryphal.  I don't see
	// anywhere that a dummy node is stored for a descending index.  - deej
	//
	// AB: This node ("dummy" node) is inserted on every first page in a level.
	// Because it's length and prefix is 0 a descending index would see it
	// always as the first matching node.
	if (!leafPage && descending &&
		(node.nodePointer == bucket->btr_nodes + bucket->btr_jump_size) && (node.length == 0))
	{
		pointer = node.readNode(pointer, leafPage);
		// Check if pointer is still valid
		if (pointer > endPointer) {
			BUGCHECK(204);	// msg 204 index inconsistent
		}
	}

	while (true)
	{
		// Pick up data from node
		if (value && node.length) {
			memcpy(value + node.prefix, node.data, node.length);
		}

		// If the record number is -1, the node is the last in the level
		// and, by definition, is the insertion point.  Otherwise, if the
		// prefix of the current node is less than the running prefix, the
		// node must have a value greater than the key, so it is the insertion
		// point.
		if (node.isEndLevel || node.prefix < prefix) {
			goto done;
		}

		// If the node prefix is greater than current prefix , it must be less
		// than the key, so we can skip it.  If it has zero length, then
		// it is a duplicate, and can also be skipped.
		if (node.prefix == prefix)
		{
			const UCHAR* q = node.data;
			const UCHAR* const nodeEnd = q + node.length;
			if (descending)
			{
				while (true)
				{
					if (q == nodeEnd || (retrieval && p == key_end))
						goto done;

					if (p == key_end || *p > *q)
						break;

					if (*p++ < *q++)
						goto done;
				}
			}
			else if (node.length > 0 || firstPass)
			{
				firstPass = false;
				while (true)
				{
					if (p == key_end)
						goto done;

					if (q == nodeEnd || *p > *q)
						break;

					if (*p++ < *q++)
						goto done;
				}
			}
			prefix = (USHORT)(p - key->key_data);
		}

		if (node.isEndBucket)
		{
			if (pointer_by_marker && (prefix == key->key_length) &&
				(prefix == node.prefix + node.length))
			{
				// AB: When storing equal nodes, recordnumbers should always
				// be inserted on this page, because the first node on the next
				// page could be a equal node with a higher recordnumber than
				// this one and that would cause a overwrite of the first node
				// in the next page, but the first node of a page must not change!!
				goto done;
			}

			return NULL;
		}
		pointer = node.readNode(pointer, leafPage);

		// Check if pointer is still valid
		if (pointer > endPointer) {
			BUGCHECK(204);	// msg 204 index inconsistent
		}
	}

done:
	if (return_value) {
		*return_value = prefix;
	}

	return node.nodePointer;
}


static UCHAR* find_area_start_point(btree_page* bucket, const temporary_key* key,
									UCHAR* value,
									USHORT* return_prefix, bool descending,
									bool retrieval, RecordNumber find_record_number)
{
/**************************************
 *
 *	f i n d _ a r e a _ s t a r t _ p o i n t
 *
 **************************************
 *
 * Functional description
 *	Locate and return a pointer to a start area.
 *	The starting nodes for a area are
 *  defined with jump nodes. A jump node
 *  contains the prefix information for
 *  a node at a specific offset.
 *
 **************************************/
	const bool useFindRecordNumber = (find_record_number != NO_VALUE);
	const bool leafPage = (bucket->btr_level == 0);
	const UCHAR* keyPointer = key->key_data;
	const UCHAR* const keyEnd = keyPointer + key->key_length;

	// Retrieve jump information.
	UCHAR* pointer = bucket->btr_nodes;
	UCHAR n = bucket->btr_jump_count;

	// Set begin of page as default.
	IndexJumpNode prevJumpNode;
	prevJumpNode.offset = BTR_SIZE + bucket->btr_jump_size;
	prevJumpNode.prefix = 0;
	prevJumpNode.length = 0;

	temporary_key jumpKey;
	jumpKey.key_length = 0;
	jumpKey.key_flags = 0;

	USHORT prefix = 0;
	USHORT testPrefix = 0;

	while (n)
	{
		IndexJumpNode jumpNode;
		pointer = jumpNode.readJumpNode(pointer);

		IndexNode node;
		node.readNode((UCHAR*) bucket + jumpNode.offset, leafPage);

		// jumpKey will hold complete data off referenced node
		memcpy(jumpKey.key_data + jumpNode.prefix, jumpNode.data, jumpNode.length);
		memcpy(jumpKey.key_data + node.prefix, node.data, node.length);
		jumpKey.key_length = node.prefix + node.length;

		keyPointer = key->key_data + jumpNode.prefix;
		const UCHAR* q = jumpKey.key_data + jumpNode.prefix;
		const UCHAR* const nodeEnd = jumpKey.key_data + jumpKey.key_length;
		bool done = false;

		if ((jumpNode.prefix <= testPrefix) && descending)
		{
			while (true)
			{
				if (q == nodeEnd)
				{
					done = true;
					// Check if this is a exact match or a duplicate.
					// If the node is pointing to its end and the length is
					// the same as the key then we have found a exact match.
					// Now start walking between the jump nodes until we
					// found a node reference that's not equal anymore
					// or the record number is higher then the one we need.
					if (useFindRecordNumber && (keyPointer == keyEnd))
					{
						n--;
						while (n)
						{
							if (find_record_number <= node.recordNumber)
							{
								// If the record number from leaf is higer
								// then we should be in our previous area.
								break;
							}
							// Calculate new prefix to return right prefix.
							prefix = jumpNode.length + jumpNode.prefix;

							prevJumpNode = jumpNode;
							pointer = jumpNode.readJumpNode(pointer);
							node.readNode((UCHAR*)bucket + jumpNode.offset, leafPage);

							if (node.length != 0 ||
								node.prefix != prevJumpNode.prefix + prevJumpNode.length ||
								node.prefix < jumpKey.key_length ||
								jumpNode.prefix != prevJumpNode.prefix + prevJumpNode.length ||
								node.isEndBucket || node.isEndLevel)
							{
								break;
							}
							n--;
						}
					}
					break;
				}

				if (retrieval && keyPointer == keyEnd)
				{
					done = true;
					break;
				}

				if (keyPointer == keyEnd)   // End of key reached
					break;

				if (*keyPointer > *q)   // Our key is bigger so check next node.
					break;

				if (*keyPointer++ < *q++)
				{
					done = true;
					break;
				}
			}
			testPrefix = (USHORT)(keyPointer - key->key_data);
		}
		else if (jumpNode.prefix <= testPrefix)
		{
			while (true)
			{
				if (keyPointer == keyEnd)
				{
					// Reached end of our key we're searching for.
					done = true;
					// Check if this is a exact match or a duplicate
					// If the node is pointing to its end and the length is
					// the same as the key then we have found a exact match.
					// Now start walking between the jump nodes until we
					// found a node reference that's not equal anymore
					// or the record number is higher then the one we need.
					if (useFindRecordNumber && q == nodeEnd)
					{
						n--;
						while (n)
						{
							if (find_record_number <= node.recordNumber)
							{
								// If the record number from leaf is higer
								// then we should be in our previous area.
								break;
							}
							// Calculate new prefix to return right prefix.
							prefix = jumpNode.length + jumpNode.prefix;

							prevJumpNode = jumpNode;
							pointer = jumpNode.readJumpNode(pointer);
							node.readNode((UCHAR*)bucket + jumpNode.offset, leafPage);

							if (node.length != 0 ||
								node.prefix != prevJumpNode.prefix + prevJumpNode.length ||
								jumpNode.prefix != prevJumpNode.prefix + prevJumpNode.length ||
								node.isEndBucket || node.isEndLevel)
							{
								break;
							}
							n--;
						}
					}
					break;
				}

				if (q == nodeEnd)	// End of node data reached
					break;

				if (*keyPointer > *q)	// Our key is bigger so check next node.
					break;

				if (*keyPointer++ < *q++)
				{
					done = true;
					break;
				}
			}
			testPrefix = (USHORT)(keyPointer - key->key_data);
		}
		if (done)
		{
			// We're done, go out of main loop.
			break;
		}

		prefix = MIN(jumpNode.length + jumpNode.prefix, testPrefix);
		if (value && (jumpNode.length + jumpNode.prefix))
		{
			// Copy prefix data from referenced node to value
			memcpy(value, jumpKey.key_data, jumpNode.length + jumpNode.prefix);
		}
		prevJumpNode = jumpNode;
		n--;
	}

	if (return_prefix) {
		*return_prefix = prefix;
	}

	return (UCHAR*) bucket + prevJumpNode.offset;
}


static ULONG find_page(btree_page* bucket, const temporary_key* key,
					   const index_desc* idx, RecordNumber find_record_number,
					   bool retrieval)
{
/**************************************
 *
 *	f i n d _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Find a page number in an index level.  Return either the
 *	node equal to the key or the last node less than the key.
 *	Note that this routine can be called only for non-leaf
 *	pages, because it assumes the first node on page is
 *	a degenerate, zero-length node.
 *
 **************************************/

	const bool leafPage = (bucket->btr_level == 0);
	bool firstPass = true;
	const bool descending = (idx->idx_flags & idx_descending);
	const bool primary = (idx->idx_flags & idx_primary);
	const bool unique = (idx->idx_flags & idx_unique);
	const bool key_all_nulls = (key->key_nulls == (1 << idx->idx_count) - 1);
	const bool validateDuplicates = (unique && !key_all_nulls) || primary;

	if (validateDuplicates)
		find_record_number = NO_VALUE;

	const UCHAR* const endPointer = (UCHAR*) bucket + bucket->btr_length;

	USHORT prefix = 0;	// last computed prefix against processed node

	// pointer where to start reading next node
	UCHAR* pointer = find_area_start_point(bucket, key, 0, &prefix,
										   descending, retrieval, find_record_number);

	IndexNode node;
	pointer = node.readNode(pointer, leafPage);
	// Check if pointer is still valid
	if (pointer > endPointer) {
		BUGCHECK(204);	// msg 204 index inconsistent
	}

	if (node.isEndBucket || node.isEndLevel)
	{
		pointer = bucket->btr_nodes + bucket->btr_jump_size;
		pointer = node.readNode(pointer, leafPage);
		// Check if pointer is still valid
		if (pointer > endPointer) {
			BUGCHECK(204);	// msg 204 index inconsistent
		}
	}

	if (node.isEndLevel) {
		BUGCHECK(206);	// msg 206 exceeded index level
	}

	ULONG previousNumber = node.pageNumber;
	if (node.nodePointer == bucket->btr_nodes + bucket->btr_jump_size)
	{
		prefix = 0;
		// Handle degenerating node, always generated at first
		// page in a level.
		if ((node.prefix == 0) && (node.length == 0))
		{
			// Compute common prefix of key and first node
			previousNumber = node.pageNumber;
			pointer = node.readNode(pointer, leafPage);
			// Check if pointer is still valid
			if (pointer > endPointer) {
				BUGCHECK(204);	// msg 204 index inconsistent
			}
		}
	}
	const UCHAR* p = key->key_data + prefix; // pointer on key
	const UCHAR* const keyEnd = key->key_data + key->key_length; // pointer on end of key

	while (true)
	{

		// If the page/record number is -1, the node is the last in the level
		// and, by definition, is the target node.  Otherwise, if the
		// prefix of the current node is less than the running prefix, its
		// node must have a value greater than the key, which is the fb_insertion
		// point.
		if (node.isEndLevel || node.prefix < prefix) {
			return previousNumber;
		}

		// If the node prefix is greater than current prefix , it must be less
		// than the key, so we can skip it.  If it has zero length, then
		// it is a duplicate, and can also be skipped.
		const UCHAR* q = node.data; // pointer on processing node
		const UCHAR* const nodeEnd = q + node.length; // pointer on end of processing node
		if (node.prefix == prefix)
		{
			if (descending)
			{
				// Descending indexes
				while (true)
				{
					// Check for exact match and if we need to do
					// record number matching.
					if (q == nodeEnd || p == keyEnd)
					{
						if (find_record_number != NO_VALUE && q == nodeEnd && p == keyEnd)
						{
							return IndexNode::findPageInDuplicates(bucket,
								node.nodePointer, previousNumber, find_record_number);
						}

						if (q < nodeEnd && !retrieval)
							break;

						return previousNumber;
					}

					if (*p > *q)
						break;

					if (*p++ < *q++)
						return previousNumber;
				}
			}
			else if (node.length > 0 || firstPass)
			{
				firstPass = false;
				// Ascending index
				while (true)
				{
					if (p == keyEnd)
					{
						// Check for exact match and if we need to do
						// record number matching.
						if (find_record_number != NO_VALUE && q == nodeEnd)
						{
							return IndexNode::findPageInDuplicates(bucket,
								node.nodePointer, previousNumber, find_record_number);
						}

						return previousNumber;
					}

					if (q == nodeEnd || *p > *q)
						break;

					if (*p++ < *q++)
						return previousNumber;
				}
			}
		}
		prefix = p - key->key_data;

		// If this is the end of bucket, return node. Somebody else can deal with this.
		if (node.isEndBucket) {
			return node.pageNumber;
		}

		previousNumber = node.pageNumber;
		pointer = node.readNode(pointer, leafPage);

		// Check if pointer is still valid
		if (pointer > endPointer) {
			BUGCHECK(204);	// msg 204 index inconsistent
		}
	}

	// NOTREACHED
	return ~0;	// superfluous return to shut lint up
}


static contents garbage_collect(thread_db* tdbb, WIN* window, ULONG parent_number)
{
/**************************************
 *
 *	g a r b a g e _ c o l l e c t
 *
 **************************************
 *
 * Functional description
 *	Garbage collect an index page.  This requires
 * 	care so that we don't step on other processes
 * 	that might be traversing the tree forwards,
 *	backwards, or top to bottom.  We must also
 *	keep in mind that someone might be adding a node
 *	at the same time we are deleting.  Therefore we
 *	must lock all the pages involved to prevent
 *	such operations while we are garbage collecting.
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const USHORT pageSpaceID = window->win_page.getPageSpaceID();
	btree_page* gc_page = (btree_page*) window->win_buffer;
	contents result = contents_above_threshold;

	// check to see if the page was marked not to be garbage collected
	if ( !BtrPageGCLock::isPageGCAllowed(tdbb, window->win_page) )
	{
		CCH_RELEASE(tdbb, window);
		return contents_above_threshold;
	}

	// record the left sibling now since this is the only way to
	// get to it quickly; don't worry if it's not accurate now or
	// is changed after we release the page, since we will fetch
	// it in a fault-tolerant way anyway.
	const SLONG left_number = gc_page->btr_left_sibling;

	// if the left sibling is blank, that indicates we are the leftmost page,
	// so don't garbage-collect the page; do this for several reasons:
	//   1.  The leftmost page needs a degenerate zero length node as its first node
	//       (for a non-leaf, non-top-level page).
	//   2.  The parent page would need to be fixed up to have a degenerate node
	//       pointing to the right sibling.
	//   3.  If we remove all pages on the level, we would need to re-add it next
	//       time a record is inserted, so why constantly garbage-collect and re-create
	//       this page?

	if (!left_number)
	{
		CCH_RELEASE(tdbb, window);
		return contents_above_threshold;
	}

	// record some facts for later validation
	const USHORT relation_number = gc_page->btr_relation;
	const UCHAR index_id = gc_page->btr_id;
	const UCHAR index_level = gc_page->btr_level;

	// we must release the page we are attempting to garbage collect;
	// this is necessary to avoid deadlocks when we fetch the parent page
	CCH_RELEASE(tdbb, window);

	// fetch the parent page, but we have to be careful, because it could have
	// been garbage-collected when we released it--make checks so that we know it
	// is the parent page; there is a minute possibility that it could have been
	// released and reused already as another page on this level, but if so, it
	// won't really matter because we won't find the node on it
	WIN parent_window(pageSpaceID, parent_number);
	btree_page* parent_page = (btree_page*) CCH_FETCH(tdbb, &parent_window, LCK_write, pag_undefined);
	if ((parent_page->btr_header.pag_type != pag_index) ||
		(parent_page->btr_relation != relation_number) ||
		(parent_page->btr_id != (UCHAR)(index_id % 256)) ||
		(parent_page->btr_level != index_level + 1))
	{
		CCH_RELEASE(tdbb, &parent_window);
		return contents_above_threshold;
	}

	if (parent_page->btr_header.pag_flags & btr_released)
	{
		CCH_RELEASE(tdbb, &parent_window);
#ifdef DEBUG_BTR
		gds__log("BTR/garbage_collect : parent page is released.");
#endif
		return contents_above_threshold;
	}

	// Find the node on the parent's level--the parent page could
	// have split while we didn't have it locked
	UCHAR* parentPointer = parent_page->btr_nodes + parent_page->btr_jump_size;
	IndexNode parentNode;
	while (true)
	{
		parentPointer = parentNode.readNode(parentPointer, false);
		if (parentNode.isEndBucket)
		{
			parent_page = (btree_page*) CCH_HANDOFF(tdbb, &parent_window,
				parent_page->btr_sibling, LCK_write, pag_index);
			parentPointer = parent_page->btr_nodes + parent_page->btr_jump_size;
			continue;
		}

		if (parentNode.pageNumber == window->win_page.getPageNum() || parentNode.isEndLevel)
		{
			break;
		}
	}

	// we should always find the node, but just in case we don't, bow out gracefully
	if (parentNode.isEndLevel)
	{
		CCH_RELEASE(tdbb, &parent_window);
#ifdef DEBUG_BTR
		CORRUPT(204);	// msg 204 index inconsistent
#endif
		return contents_above_threshold;
	}

	// Fix for ARINC database corruption bug: in most cases we update the END_BUCKET
	// marker of the left sibling page to contain the END_BUCKET of the garbage-collected
	// page.  However, when this page is the first page on its parent, then the left
	// sibling page is the last page on its parent.  That means if we update its END_BUCKET
	// marker, its bucket of values will extend past that of its parent, causing trouble
	// down the line.

	// So we never garbage-collect a page which is the first one on its parent.  This page
	// will have to wait until the parent page gets collapsed with the page to its left,
	// in which case this page itself will then be garbage-collectable.  Since there are
	// no more keys on this page, it will not be garbage-collected itself.  When the page
	// to the right falls below the threshold for garbage collection, it will be merged with
	// this page.
	if (parentNode.nodePointer == parent_page->btr_nodes + parent_page->btr_jump_size)
	{
		CCH_RELEASE(tdbb, &parent_window);
		return contents_above_threshold;
	}

	// find the left sibling page by going one page to the left,
	// but if it does not recognize us as its right sibling, keep
	// going to the right until we find the page that is our real
	// left sibling
	WIN left_window(pageSpaceID, left_number);
	btree_page* left_page = (btree_page*) CCH_FETCH(tdbb, &left_window, LCK_write, pag_undefined);
	if (left_page->btr_header.pag_type != pag_index ||
		left_page->btr_relation != relation_number ||
		left_page->btr_id != UCHAR(index_id % 256) ||
		left_page->btr_level != index_level)
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		return contents_above_threshold;
	}

	if (left_page->btr_header.pag_flags & btr_released)
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
#ifdef DEBUG_BTR
		gds__log("BTR/garbage_collect : left page is released.");
#endif
		return contents_above_threshold;
	}


	while (left_page->btr_sibling != window->win_page.getPageNum())
	{
#ifdef DEBUG_BTR
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		CORRUPT(204);	// msg 204 index inconsistent
#endif
		// If someone garbage collects the index page before we can, it
		// won't be found by traversing the right sibling chain. This means
		// scanning index pages until the end-of-level bucket is hit.
		if (!left_page->btr_sibling)
		{
			CCH_RELEASE(tdbb, &parent_window);
			CCH_RELEASE(tdbb, &left_window);
			return contents_above_threshold;
		}
		left_page = (btree_page*) CCH_HANDOFF(tdbb, &left_window,
											  left_page->btr_sibling, LCK_write, pag_index);
	}

	// now refetch the original page and make sure it is still
	// below the threshold for garbage collection.
	gc_page = (btree_page*) CCH_FETCH(tdbb, window, LCK_write, pag_index);
	if (gc_page->btr_length >= GARBAGE_COLLECTION_BELOW_THRESHOLD ||
		!BtrPageGCLock::isPageGCAllowed(tdbb, window->win_page))
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		CCH_RELEASE(tdbb, window);
		return contents_above_threshold;
	}

	if (gc_page->btr_header.pag_flags & btr_released)
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		CCH_RELEASE(tdbb, window);
#ifdef DEBUG_BTR
		gds__log("BTR/garbage_collect : gc_page is released.");
		CORRUPT(204);	// msg 204 index inconsistent
#endif
		return contents_above_threshold;
	}

	// fetch the right sibling page
	btree_page* right_page = NULL;
	WIN right_window(pageSpaceID, gc_page->btr_sibling);
	if (right_window.win_page.getPageNum())
	{
		// right_window.win_flags = 0; redundant, made by the constructor
		right_page = (btree_page*) CCH_FETCH(tdbb, &right_window, LCK_write, pag_index);

		if (right_page->btr_left_sibling != window->win_page.getPageNum())
		{
			CCH_RELEASE(tdbb, &parent_window);
			if (left_page) {
				CCH_RELEASE(tdbb, &left_window);
			}
			CCH_RELEASE(tdbb, window);
			CCH_RELEASE(tdbb, &right_window);
#ifdef DEBUG_BTR
			CORRUPT(204);	// msg 204 index inconsistent
#endif
			return contents_above_threshold;
		}
	}

	const bool leafPage = (gc_page->btr_level == 0);

	UCHAR* leftPointer = left_page->btr_nodes + left_page->btr_jump_size;
	temporary_key lastKey;
	lastKey.key_flags = 0;
	lastKey.key_length = 0;

	IndexNode leftNode;
	UCHAR* pointer = left_page->btr_nodes;

	// Walk trough node jumpers.
	UCHAR n = left_page->btr_jump_count;
	IndexJumpNode jumpNode;
	while (n)
	{
		pointer = jumpNode.readJumpNode(pointer);
		leftNode.readNode((UCHAR*)left_page + jumpNode.offset, leafPage);

		if (!(leftNode.isEndBucket || leftNode.isEndLevel))
		{
			memcpy(lastKey.key_data + jumpNode.prefix, jumpNode.data, jumpNode.length);
			leftPointer = (UCHAR*)left_page + jumpNode.offset;
			lastKey.key_length = jumpNode.prefix + jumpNode.length;
		}
		else {
			break;
		}
		n--;
	}

	while (true)
	{
		leftPointer = leftNode.readNode(leftPointer, leafPage);
		// If it isn't a recordnumber were done
		if (leftNode.isEndBucket || leftNode.isEndLevel) {
			break;
		}
		// Save data
		if (leftNode.length)
		{
			memcpy(lastKey.key_data + leftNode.prefix, leftNode.data, leftNode.length);
			lastKey.key_length = leftNode.prefix + leftNode.length;
		}
	}
	leftPointer = leftNode.nodePointer;

	// see if there's enough space on the left page to move all the nodes to it
	// and leave some extra space for expansion (at least one key length)
	UCHAR* gcPointer = gc_page->btr_nodes + gc_page->btr_jump_size;
	IndexNode gcNode;
	gcNode.readNode(gcPointer, leafPage);
	const USHORT prefix = IndexNode::computePrefix(lastKey.key_data, lastKey.key_length,
												   gcNode.data, gcNode.length);

	// Get pointer for calculating gcSize (including jump nodes).
	gcPointer = gc_page->btr_nodes;

	const USHORT gcSize = gc_page->btr_length - (gcPointer - (UCHAR*)(gc_page));
	const USHORT leftAssumedSize = left_page->btr_length + gcSize - prefix;

	// If the new page will be larger then the thresholds don't gc.
	//GARBAGE_COLLECTION_NEW_PAGE_MAX_THRESHOLD
	const USHORT max_threshold = GARBAGE_COLLECTION_NEW_PAGE_MAX_THRESHOLD;
	//USHORT max_threshold = dbb->dbb_page_size - 50;
	if (leftAssumedSize > max_threshold)
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		CCH_RELEASE(tdbb, window);
		if (right_page) {
			CCH_RELEASE(tdbb, &right_window);
		}
		return contents_above_threshold;
	}

	// First copy left page to scratch page.
	SLONG scratchPage[OVERSIZE];
	btree_page* const newBucket = (btree_page*) scratchPage;

	pointer = left_page->btr_nodes;
	const USHORT jumpersOriginalSize = left_page->btr_jump_size;
	const USHORT jumpAreaSize = left_page->btr_jump_interval;

	// Copy header and data
	memcpy(newBucket, left_page, BTR_SIZE);
	memcpy(newBucket->btr_nodes, left_page->btr_nodes + left_page->btr_jump_size,
		left_page->btr_length - left_page->btr_jump_size - BTR_SIZE);

	// Update leftPointer to scratch page.
	leftPointer = (UCHAR*) newBucket + (leftPointer - (UCHAR*) left_page) - jumpersOriginalSize;
	gcPointer = gc_page->btr_nodes + gc_page->btr_jump_size;
	//
	leftNode.readNode(leftPointer, leafPage);
	// Calculate the total amount of compression on page as the combined
	// totals of the two pages, plus the compression of the first node
	// on the g-c'ed page, minus the prefix of the END_BUCKET node to
	// be deleted.
	newBucket->btr_prefix_total += gc_page->btr_prefix_total + prefix - leftNode.prefix;

	// Get first node from gc-page.
	gcPointer = gcNode.readNode(gcPointer, leafPage);

	// Write first node with prefix compression on left page.
	leftNode.setNode(prefix, gcNode.length - prefix, gcNode.recordNumber,
				     gcNode.pageNumber, gcNode.isEndBucket, gcNode.isEndLevel);
	leftNode.data = gcNode.data + prefix;
	leftPointer = leftNode.writeNode(leftPointer, leafPage);

	// Update page-size.
	newBucket->btr_length = leftPointer - (UCHAR*) newBucket;
	// copy over the remainder of the page to be garbage-collected.
	const USHORT l = gc_page->btr_length - (gcPointer - (UCHAR*)(gc_page));
	memcpy(leftPointer, gcPointer, l);
	// update page size.
	newBucket->btr_length += l;

	// Generate new jump nodes.
	jumpNodeList jumpNodes;
	USHORT jumpersNewSize = 0;
	// Update jump information on scratch page, so generate_jump_nodes
	// can deal with it.
	newBucket->btr_jump_interval = jumpAreaSize;
	newBucket->btr_jump_size = 0;
	newBucket->btr_jump_count = 0;
	generate_jump_nodes(tdbb, newBucket, &jumpNodes, 0, &jumpersNewSize, NULL, NULL);

	// Now we know exact how big our updated left page is, so check size
	// again to be sure it all will fit.
	// If the new page will be larger then the page size don't gc ofcourse.
	if (newBucket->btr_length + jumpersNewSize > dbb->dbb_page_size)
	{
		CCH_RELEASE(tdbb, &parent_window);
		CCH_RELEASE(tdbb, &left_window);
		CCH_RELEASE(tdbb, window);
		if (right_page) {
			CCH_RELEASE(tdbb, &right_window);
		}
		IndexJumpNode* walkJumpNode = jumpNodes.begin();
		for (size_t i = 0; i < jumpNodes.getCount(); i++) {
			delete[] walkJumpNode[i].data;
		}
		return contents_above_threshold;
	}

#ifdef DEBUG_BTR_SPLIT
	Firebird::string s;
	s.printf("node with page %ld removed from parent page %ld",
		parentNode.pageNumber, parent_window.win_page.getPageNum());
	gds__trace(s.c_str());
#endif
	// Update the parent first.  If the parent is not written out first,
	// we will be pointing to a page which is not in the doubly linked
	// sibling list, and therefore navigation back and forth won't work.
	// AB: Parent is always a index pointer page.
	result = delete_node(tdbb, &parent_window, parentNode.nodePointer);
	CCH_RELEASE(tdbb, &parent_window);

	// Update the right sibling page next, since it does not really
	// matter that the left sibling pointer points to the page directly
	// to the left, only that it point to some page to the left.
	// Set up the precedence so that the parent will be written first.
	if (right_page)
	{
		if (parent_page) {
			CCH_precedence(tdbb, &right_window, parent_window.win_page);
		}
		CCH_MARK(tdbb, &right_window);
		right_page->btr_left_sibling = left_window.win_page.getPageNum();

		CCH_RELEASE(tdbb, &right_window);
	}

	// Now update the left sibling, effectively removing the garbage-collected page
	// from the tree.  Set the precedence so the right sibling will be written first.
	if (right_page) {
		CCH_precedence(tdbb, &left_window, right_window.win_page);
	}
	else if (parent_page) {
		CCH_precedence(tdbb, &left_window, parent_window.win_page);
	}

	CCH_MARK(tdbb, &left_window);

	if (right_page) {
		left_page->btr_sibling = right_window.win_page.getPageNum();
	}
	else {
		left_page->btr_sibling = 0;
	}

	// Finally write all data to left page.
	left_page->btr_jump_interval = jumpAreaSize;
	left_page->btr_jump_size = jumpersNewSize;
	left_page->btr_jump_count = (UCHAR) jumpNodes.getCount();

	// Write jump nodes.
	pointer = left_page->btr_nodes;
	IndexJumpNode* walkJumpNode = jumpNodes.begin();
	for (size_t i = 0; i < jumpNodes.getCount(); i++)
	{
		// Update offset to real position with new jump nodes.
		walkJumpNode[i].offset += jumpersNewSize;
		pointer = walkJumpNode[i].writeJumpNode(pointer);
		delete[] walkJumpNode[i].data;
	}
	// Copy data.
	memcpy(pointer, newBucket->btr_nodes, newBucket->btr_length - BTR_SIZE);
	// Update page header information.
	left_page->btr_prefix_total = newBucket->btr_prefix_total;
	left_page->btr_length = newBucket->btr_length + jumpersNewSize;

#ifdef DEBUG_BTR
	if (left_page->btr_length > dbb->dbb_page_size)
	{
		CCH_RELEASE(tdbb, &left_window);
		CCH_RELEASE(tdbb, window);
		CORRUPT(204);	// msg 204 index inconsistent
		return contents_above_threshold;
	}
#endif

	CCH_RELEASE(tdbb, &left_window);

#ifdef DEBUG_BTR_SPLIT
	Firebird::string s;
	s.printf("page %ld is removed from index. parent %ld, left %ld, right %ld",
		window->win_page.getPageNum(), parent_window.win_page.getPageNum(),
		left_page ? left_window.win_page.getPageNum() : 0,
		right_page ? right_window.win_page.getPageNum() : 0 );
	gds__trace(s.c_str());
#endif

	// finally, release the page, and indicate that we should write the
	// previous page out before we write the TIP page out
	CCH_MARK(tdbb, window);
	gc_page->btr_header.pag_flags |= btr_released;

	CCH_RELEASE(tdbb, window);
	PAG_release_page(tdbb, window->win_page, left_page ? left_window.win_page :
		right_page ? right_window.win_page : parent_window.win_page);

	// if the parent page needs to be garbage collected, that means we need to
	// re-fetch the parent and check to see whether it is still garbage-collectable;
	// make sure that the page is still a btree page in this index and in this level--
	// there is a miniscule chance that it was already reallocated as another page
	// on this level which is already below the threshold, in which case it doesn't
	// hurt anything to garbage-collect it anyway
	if (result != contents_above_threshold)
	{
		window->win_page = parent_window.win_page;
		parent_page = (btree_page*) CCH_FETCH(tdbb, window, LCK_write, pag_undefined);

		if ((parent_page->btr_header.pag_type != pag_index) ||
			(parent_page->btr_relation != relation_number) || (parent_page->btr_id != index_id) ||
			(parent_page->btr_level != index_level + 1))
		{
			CCH_RELEASE(tdbb, window);
			return contents_above_threshold;
		}

		// check whether it is empty
		parentPointer = parent_page->btr_nodes + parent_page->btr_jump_size;
		IndexNode parentNode2;
		parentPointer = parentNode2.readNode(parentPointer, false);
		if (parentNode2.isEndBucket || parentNode2.isEndLevel) {
			return contents_empty;
		}

		// check whether there is just one node
		parentPointer = parentNode2.readNode(parentPointer, false);
		if (parentNode2.isEndBucket || parentNode2.isEndLevel) {
			return contents_single;
		}

		// check to see if the size of the page is below the garbage collection threshold
		if (parent_page->btr_length < GARBAGE_COLLECTION_BELOW_THRESHOLD) {
			return contents_below_threshold;
		}

		// the page must have risen above the threshold; release the window since
		// someone else added a node while the page was released
		CCH_RELEASE(tdbb, window);
		return contents_above_threshold;
	}

	return result;
}


static void generate_jump_nodes(thread_db* tdbb, btree_page* page,
								jumpNodeList* jumpNodes,
								USHORT excludeOffset, USHORT* jumpersSize,
								USHORT* splitIndex, USHORT* splitPrefix)
{
/**************************************
 *
 *	g e n e r a t e _ j u m p _ n o d e s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	fb_assert(page);
	fb_assert(jumpNodes);
	fb_assert(jumpersSize);

	const bool leafPage = (page->btr_level == 0);
	const USHORT jumpAreaSize = page->btr_jump_interval;

	*jumpersSize = 0;
	UCHAR* pointer = page->btr_nodes + page->btr_jump_size;

	temporary_key jumpKey, currentKey;
	jumpKey.key_flags = 0;
	jumpKey.key_length = 0;
	currentKey.key_flags = 0;
	currentKey.key_length = 0;
	UCHAR* jumpData = jumpKey.key_data;
	USHORT jumpLength = 0;
	UCHAR* currentData = currentKey.key_data;

	if (splitIndex) {
		*splitIndex = 0;
	}
	if (splitPrefix) {
		*splitPrefix = 0;
	}

	const UCHAR* newAreaPosition = pointer + jumpAreaSize;
	const UCHAR* const endpoint = ((UCHAR*)page + page->btr_length);
	const UCHAR* const halfpoint = ((UCHAR*)page + (dbb->dbb_page_size / 2));
	const UCHAR* const excludePointer = ((UCHAR*)page + excludeOffset);
	IndexJumpNode jumpNode;
	IndexNode node;

	while (pointer < endpoint)
	{
		pointer = node.readNode(pointer, leafPage);

		if (node.isEndBucket || node.isEndLevel) {
			break;
		}

		if (node.length)
		{
			UCHAR* q = currentData + node.prefix;
			memcpy(q, node.data, node.length);
		}

		if (splitIndex && splitPrefix && !*splitIndex) {
			*splitPrefix += node.prefix;
		}

		if (node.nodePointer > newAreaPosition && node.nodePointer != excludePointer)
		{
			// Create a jumpnode, but it may not point to the new
			// insert pointer or any MARKER else we make split
			// more difficult then needed.
			jumpNode.offset = (node.nodePointer - (UCHAR*)page);
			jumpNode.prefix = IndexNode::computePrefix(jumpData, jumpLength,
														   currentData, node.prefix);
			jumpNode.length = node.prefix - jumpNode.prefix;
			if (jumpNode.length)
			{
				jumpNode.data = FB_NEW(*tdbb->getDefaultPool()) UCHAR[jumpNode.length];
				const UCHAR* const q = currentData + jumpNode.prefix;
				memcpy(jumpNode.data, q, jumpNode.length);
			}
			else {
				jumpNode.data = NULL;
			}
			// Push node on end in list
			jumpNodes->add(jumpNode);
			// Store new data in jumpKey, so a new jump node can calculate prefix
			memcpy(jumpData + jumpNode.prefix, jumpNode.data, jumpNode.length);
			jumpLength = jumpNode.length + jumpNode.prefix;
			// Check if this could be our split point (if we need to split)
			if (splitIndex && !*splitIndex && (pointer > halfpoint)) {
				*splitIndex = jumpNodes->getCount();
			}
			// Set new position for generating jumpnode
			newAreaPosition += jumpAreaSize;
			*jumpersSize += jumpNode.getJumpNodeSize();
		}
	}
}


static ULONG insert_node(thread_db* tdbb,
						 WIN* window,
						 index_insertion* insertion,
						 temporary_key* new_key,
						 RecordNumber* new_record_number,
						 ULONG* original_page,
						 ULONG* sibling_page)
{
/**************************************
 *
 *	i n s e r t _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Insert a node in a index leaf page.
 *  If this isn't the right bucket, return NO_VALUE.
 *  If it splits, return the split page number and
 *	leading string.  This is the workhorse for add_node.
 *
 **************************************/

	SET_TDBB(tdbb);
	const Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const USHORT pageSpaceID = window->win_page.getPageSpaceID();

	// find the insertion point for the specified key
	btree_page* bucket = (btree_page*) window->win_buffer;
	temporary_key* key = insertion->iib_key;

	const index_desc* const idx = insertion->iib_descriptor;
	const bool unique = (idx->idx_flags & idx_unique);
	const bool primary = (idx->idx_flags & idx_primary);
	const bool key_all_nulls = (key->key_nulls == (1 << idx->idx_count) - 1);
	const bool leafPage = (bucket->btr_level == 0);
	// hvlad: don't check unique index if key has only null values
	const bool validateDuplicates = (unique && !key_all_nulls) || primary;
	USHORT prefix = 0;
	RecordNumber newRecordNumber;
	if (leafPage) {
		newRecordNumber = insertion->iib_number;
	}
	else {
		newRecordNumber = *new_record_number;
	}
	// For checking on duplicate nodes we should find the first matching key.
	UCHAR* pointer = find_node_start_point(bucket, key, 0, &prefix,
						idx->idx_flags & idx_descending,
						false, true, validateDuplicates ? NO_VALUE : newRecordNumber);
	if (!pointer) {
		return NO_VALUE_PAGE;
	}

	if ((UCHAR*)pointer - (UCHAR*)bucket > dbb->dbb_page_size) {
		BUGCHECK(205);	// msg 205 index bucket overfilled
	}

	IndexNode beforeInsertNode;
	pointer = beforeInsertNode.readNode(pointer, leafPage);

	// loop through the equivalent nodes until the correct insertion
	// point is found; for leaf level this will be the first node
	USHORT newPrefix, newLength;
	USHORT nodeOffset;
	while (true)
	{
		nodeOffset = (USHORT) (beforeInsertNode.nodePointer - (UCHAR*) bucket);
		newPrefix = beforeInsertNode.prefix;
		newLength = beforeInsertNode.length;

		// update the newPrefix and newLength against the node (key) that will
		// be inserted before it.
		const UCHAR* p = key->key_data + newPrefix;
		const UCHAR* q = beforeInsertNode.data;
		USHORT l = MIN(key->key_length - newPrefix, newLength);
		while (l)
		{
			if (*p++ != *q++) {
				break;
			}
			--newLength;
			newPrefix++;
			l--;
		}

		// check if the inserted node has the same value as the next node
		if (newPrefix != key->key_length ||
			newPrefix != beforeInsertNode.length + beforeInsertNode.prefix)
		{
			break;
		}

		// We have a equal node, so find the correct insertion point.
		if (beforeInsertNode.isEndBucket)
		{
			if (validateDuplicates) {
				return NO_VALUE_PAGE;
			}

			if (newRecordNumber < beforeInsertNode.recordNumber)
			{
				break;
			}

			return NO_VALUE_PAGE;
		}

		if (beforeInsertNode.isEndLevel) {
			break;
		}

		if (leafPage && validateDuplicates)
		{
			// Save the duplicate so the main caller can validate them.
			RBM_SET(tdbb->getDefaultPool(), &insertion->iib_duplicates,
				beforeInsertNode.recordNumber.getValue());
		}
		// AB: Never insert a duplicate node with the same record number.
		// This would lead to nodes which will never be deleted.
		/*if (leafPage && (newRecordNumber == beforeInsertNode.recordNumber))
		{
			// AB: It seems this is not enough, because on mass duplicate
			// update to many nodes are deleted, possible staying and
			// going are wrong checked before BTR_remove is called.
			CCH_RELEASE(tdbb, window);
			return 0;
		}*/
		//else
		if (!validateDuplicates)
		{
			// if recordnumber is higher we need to insert before it.
			if (newRecordNumber <= beforeInsertNode.recordNumber) {
				break;
			}
		}
		else if (!unique) {
			break;
		}

		prefix = newPrefix;
		pointer = beforeInsertNode.readNode(pointer, leafPage);
	}

	if (nodeOffset > dbb->dbb_page_size) {
		BUGCHECK(205);	// msg 205 index bucket overfilled
	}

	const USHORT beforeInsertOriginalSize = beforeInsertNode.getNodeSize(leafPage);
	const USHORT orginalPrefix = beforeInsertNode.prefix;

	// Update the values for the next node after our new node.
	// First, store needed data for beforeInsertNode into tempData.
	HalfStaticArray<UCHAR, MAX_KEY> tempBuf;
	UCHAR* tempData = tempBuf.getBuffer(newLength);
	memcpy(tempData, beforeInsertNode.data + newPrefix - beforeInsertNode.prefix, newLength);

	beforeInsertNode.prefix = newPrefix;
	beforeInsertNode.length = newLength;
	const USHORT beforeInsertSize = beforeInsertNode.getNodeSize(leafPage);

	// Set values for our new node.
	IndexNode newNode;
	newNode.setNode(prefix, key->key_length - prefix, newRecordNumber);
	newNode.data = key->key_data + prefix;
	if (!leafPage) {
		newNode.pageNumber = insertion->iib_number.getValue();
	}

	// Compute the delta between current and new page.
	const USHORT delta = newNode.getNodeSize(leafPage) +
		beforeInsertSize - beforeInsertOriginalSize;

	// Copy data up to insert point to scratch page.
	SLONG scratchPage[OVERSIZE];
	memcpy(scratchPage, bucket, nodeOffset);
	btree_page* const newBucket = (btree_page*) scratchPage;

	// Set pointer of new node to right place.
	pointer = ((UCHAR*)newBucket + nodeOffset);
	// Insert the new node.
	pointer = newNode.writeNode(pointer, leafPage);
	newBucket->btr_prefix_total += prefix - orginalPrefix;

	// Recompress and rebuild the next node.
	beforeInsertNode.data = tempData;
	pointer = beforeInsertNode.writeNode(pointer, leafPage);
	newBucket->btr_prefix_total += newPrefix;
	beforeInsertNode.data = 0;

	// Copy remaining data to scratch page.
	if ((nodeOffset + beforeInsertOriginalSize) < bucket->btr_length)
	{
		memcpy(pointer, (UCHAR*)bucket + nodeOffset + beforeInsertOriginalSize,
			bucket->btr_length - (nodeOffset + beforeInsertOriginalSize));
	}

	// Update bucket size.
	newBucket->btr_length += delta;

	// figure out whether this node was inserted at the end of the page
	const bool endOfPage = (beforeInsertNode.isEndBucket || beforeInsertNode.isEndLevel);

	// Initialize variables needed for generating jump information
	bool fragmentedOffset = false;
	USHORT newPrefixTotalBySplit = 0;
	USHORT splitJumpNodeIndex = 0;
	jumpNodeList tmpJumpNodes;
	jumpNodeList* jumpNodes = &tmpJumpNodes;

	USHORT ensureEndInsert = 0;
	if (endOfPage)
	{
		// If we're adding a node at the end we don't want that a page
		// splits in the middle, but at the end. We can never be sure
		// that this will happen, but at least give it a bigger chance.
		ensureEndInsert = 6 + key->key_length;
	}

	// Get the total size of the jump nodes currently in use.
	pointer = newBucket->btr_nodes;
	const USHORT jumpAreaSize = newBucket->btr_jump_interval;
	const USHORT jumpersOriginalSize = newBucket->btr_jump_size;
	const UCHAR jumpersOriginalCount = newBucket->btr_jump_count;

	// Allow some fragmentation, 10% below or above actual point.
	USHORT jumpersNewSize = jumpersOriginalSize;
	UCHAR n = jumpersOriginalCount;
	USHORT index = 1;
	const USHORT fragmentedThreshold = jumpAreaSize / 5;
	IndexJumpNode jumpNode;
	while (n)
	{
		pointer = jumpNode.readJumpNode(pointer);
		if (jumpNode.offset == nodeOffset)
		{
			fragmentedOffset = true;
			break;
		}
		if (jumpNode.offset > nodeOffset) {
			jumpNode.offset += delta;
		}
		const USHORT minOffset = BTR_SIZE + jumpersOriginalSize +
			(index * jumpAreaSize) - fragmentedThreshold;
		if (jumpNode.offset < minOffset)
		{
			fragmentedOffset = true;
			break;
		}
		const USHORT maxOffset =  BTR_SIZE + jumpersOriginalSize +
			(index * jumpAreaSize) + fragmentedThreshold;
		if (jumpNode.offset > maxOffset)
		{
			fragmentedOffset = true;
			break;
		}
		jumpNodes->add(jumpNode);
		index++;
		n--;
	}
	// Rebuild jump nodes if new node is inserted after last
	// jump node offset + jumpAreaSize.
	if (nodeOffset >= (BTR_SIZE + jumpersOriginalSize +
		((jumpersOriginalCount + 1) * jumpAreaSize)))
	{
		fragmentedOffset = true;
	}
	// Rebuild jump nodes if we gona split.
	if (newBucket->btr_length + ensureEndInsert > dbb->dbb_page_size) {
		fragmentedOffset = true;
	}

	if (fragmentedOffset)
	{
		// Clean up any previous nodes.
		jumpNodes->clear();
		// Generate new jump nodes.
		generate_jump_nodes(tdbb, newBucket, jumpNodes,
			(USHORT)(newNode.nodePointer - (UCHAR*)newBucket),
			&jumpersNewSize, &splitJumpNodeIndex, &newPrefixTotalBySplit);
	}

	// If the bucket still fits on a page, we're almost done.
	if (newBucket->btr_length + ensureEndInsert +
		jumpersNewSize - jumpersOriginalSize <= dbb->dbb_page_size)
	{
		// if we are a pointer page, make sure that the page we are
		// pointing to gets written before we do for on-disk integrity
		if (!leafPage) {
			CCH_precedence(tdbb, window, insertion->iib_number.getValue());
		}
		// Mark page as dirty.
		CCH_MARK(tdbb, window);

		// Put all data back into bucket (= window->win_buffer).

		// Write jump information header.
		bucket->btr_jump_interval = jumpAreaSize;
		bucket->btr_jump_size = jumpersNewSize;
		bucket->btr_jump_count = (UCHAR) jumpNodes->getCount();

		// Write jump nodes.
		pointer = bucket->btr_nodes;
		IndexJumpNode* walkJumpNode = jumpNodes->begin();
		for (size_t i = 0; i < jumpNodes->getCount(); i++)
		{
			// Update offset to real position with new jump nodes.
			walkJumpNode[i].offset += jumpersNewSize - jumpersOriginalSize;
			pointer = walkJumpNode[i].writeJumpNode(pointer);
			if (fragmentedOffset) {
				delete[] walkJumpNode[i].data;
			}
		}
		pointer = bucket->btr_nodes + bucket->btr_jump_size;
		// Copy data block.
		memcpy(pointer, newBucket->btr_nodes + jumpersOriginalSize,
			newBucket->btr_length - BTR_SIZE - jumpersOriginalSize);

		// Update header information.
		bucket->btr_prefix_total = newBucket->btr_prefix_total;
		bucket->btr_length = newBucket->btr_length + jumpersNewSize - jumpersOriginalSize;

		CCH_RELEASE(tdbb, window);

		jumpNodes->clear();

		return NO_SPLIT;
	}

	// We've a bucket split in progress.  We need to determine the split point.
	// Set it halfway through the page, unless we are at the end of the page,
	// in which case put only the new node on the new page.  This will ensure
	// that pages get filled in the case of a monotonically increasing key.
	// Make sure that the original page has room, in case the END_BUCKET marker
	// is now longer because it is pointing at the new node.
	//
	// Note! : newBucket contains still old jump nodes and info.
	SLONG prefix_total = 0;
	UCHAR* splitpoint = NULL;
	USHORT jumpersSplitSize = 0;
	IndexNode node;
	if (splitJumpNodeIndex)
	{
		// Get pointer after new inserted node.
		splitpoint = node.readNode(newNode.nodePointer, leafPage);
		IndexNode dummyNode = newNode;
		dummyNode.setEndBucket();
		const USHORT deltaSize = dummyNode.getNodeSize(leafPage) - newNode.getNodeSize(leafPage);
		if (endOfPage && ((splitpoint + jumpersNewSize - jumpersOriginalSize) <=
			(UCHAR*) newBucket + dbb->dbb_page_size - deltaSize))
		{
			// Copy data from inserted key and this key will we the END_BUCKET marker
			// as the first key on the next page.
			const USHORT l = new_key->key_length = key->key_length;
			memcpy(new_key->key_data, key->key_data, l);
			prefix_total = newBucket->btr_prefix_total - beforeInsertNode.prefix;
			splitJumpNodeIndex = 0;
		}
		else
		{
			jumpersNewSize = 0;

			// splitJumpNodeIndex should always be 1 or higher
			if (splitJumpNodeIndex < 1) {
				BUGCHECK(205);	// msg 205 index bucket overfilled
			}

			// First get prefix data from jump node.
			USHORT index = 1;
			IndexJumpNode* jn = 0;
			IndexJumpNode* walkJumpNode = jumpNodes->begin();
			for (size_t i = 0; i < jumpNodes->getCount(); i++, index++)
			{
				UCHAR* q = new_key->key_data + walkJumpNode[i].prefix;
				memcpy(q, walkJumpNode[i].data, walkJumpNode[i].length);
				if (index == splitJumpNodeIndex)
				{
					jn = &walkJumpNode[i];
					break;
				}
			}

			// Get data from node.
			splitpoint = (UCHAR*)newBucket + jn->offset;
			splitpoint = node.readNode(splitpoint, leafPage);
			memcpy(new_key->key_data + node.prefix, node.data, node.length);
			new_key->key_length = node.prefix + node.length;
			prefix_total = newPrefixTotalBySplit;

			// Rebuild first jumpnode on splitpage
			index = 1;
			walkJumpNode = jumpNodes->begin();
			for (size_t i = 0; i < jumpNodes->getCount(); i++, index++)
			{
				if (index > splitJumpNodeIndex)
				{
					const USHORT length = walkJumpNode[i].prefix + walkJumpNode[i].length;
					UCHAR* newData = FB_NEW(*tdbb->getDefaultPool()) UCHAR[length];
					memcpy(newData, new_key->key_data, walkJumpNode[i].prefix);
					memcpy(newData + walkJumpNode[i].prefix, walkJumpNode[i].data,
						walkJumpNode[i].length);
					delete[] walkJumpNode[i].data;
					walkJumpNode[i].prefix = 0;
					walkJumpNode[i].length = length;
					walkJumpNode[i].data = newData;
					break;
				}
			}

			// Initalize new offsets for original page and split page.
			index = 1;
			walkJumpNode = jumpNodes->begin();
			for (size_t i = 0; i < jumpNodes->getCount(); i++, index++)
			{
				// The jump node where the split is done isn't included anymore!
				if (index < splitJumpNodeIndex) {
					jumpersNewSize += walkJumpNode[i].getJumpNodeSize();
				}
				else if (index > splitJumpNodeIndex) {
					jumpersSplitSize += walkJumpNode[i].getJumpNodeSize();
				}
			}
		}
	}
	else
	{
		const UCHAR* midpoint = NULL;
		splitpoint = newNode.readNode(newNode.nodePointer, leafPage);
		IndexNode dummyNode = newNode;
		dummyNode.setEndBucket();
		const USHORT deltaSize = dummyNode.getNodeSize(leafPage) - newNode.getNodeSize(leafPage);
		if (endOfPage && ((UCHAR*) splitpoint <= (UCHAR*) newBucket + dbb->dbb_page_size - deltaSize))
		{
			midpoint = splitpoint;
		}
		else {
			midpoint = (UCHAR*) newBucket +
				(dbb->dbb_page_size - BTR_SIZE - newBucket->btr_jump_size) / 2;
		}
		// Start from the begin of the nodes
		splitpoint = newBucket->btr_nodes + newBucket->btr_jump_size;
		// Copy the bucket up to the midpoint, restructing the full midpoint key
		while (splitpoint < midpoint)
		{
			splitpoint = node.readNode(splitpoint, leafPage);
			prefix_total += node.prefix;
			new_key->key_length = node.prefix + node.length;
			memcpy(new_key->key_data + node.prefix, node.data, node.length);
		}
	}

	// Allocate and format the overflow page
	WIN split_window(pageSpaceID, -1);
	btree_page* split = (btree_page*) DPM_allocate(tdbb, &split_window);

	// if we're a pointer page, make sure the child page is written first
	if (!leafPage)
	{
		if (newNode.nodePointer < splitpoint) {
			CCH_precedence(tdbb, window, insertion->iib_number.getValue());
		}
		else {
			CCH_precedence(tdbb, &split_window, insertion->iib_number.getValue());
		}
	}

	// format the new page to look like the old page
	const SLONG right_sibling = bucket->btr_sibling;
	split->btr_header.pag_type = bucket->btr_header.pag_type;
	split->btr_relation = bucket->btr_relation;
	split->btr_id = bucket->btr_id;
	split->btr_level = bucket->btr_level;
	split->btr_sibling = right_sibling;
	split->btr_left_sibling = window->win_page.getPageNum();

	// Format the first node on the overflow page
	newNode.setNode(0, new_key->key_length, node.recordNumber, node.pageNumber);
	// Return first record number on split page to caller.
	newNode.data = new_key->key_data;
	*new_record_number = newNode.recordNumber;
	const USHORT firstSplitNodeSize = newNode.getNodeSize(leafPage);

	// Format the first node on the overflow page
	split->btr_jump_interval = jumpAreaSize;
	split->btr_jump_size = jumpersSplitSize;
	if (splitJumpNodeIndex > 0) {
		split->btr_jump_count = (UCHAR) (jumpNodes->getCount() - splitJumpNodeIndex);
	}
	else {
		split->btr_jump_count = 0;
	}

	pointer = split->btr_nodes;

	if (splitJumpNodeIndex > 0)
	{
		// Write jump nodes to split page.
		USHORT index = 1;
		// Calculate size that's between header and splitpoint.
		const USHORT splitOffset = splitpoint - (UCHAR*) newBucket;
		IndexJumpNode* walkJumpNode = jumpNodes->begin();
		for (size_t i = 0; i < jumpNodes->getCount(); i++, index++)
		{
			if (index > splitJumpNodeIndex)
			{
				// Update offset to correct position.
				walkJumpNode[i].offset = walkJumpNode[i].offset - splitOffset +
					BTR_SIZE + split->btr_jump_size + firstSplitNodeSize;
				pointer = walkJumpNode[i].writeJumpNode(pointer);
			}
		}
	}
	pointer = split->btr_nodes + split->btr_jump_size;
	pointer = newNode.writeNode(pointer, leafPage);

	// Copy down the remaining data from scratch page.
	const USHORT l = newBucket->btr_length - (splitpoint - (UCHAR*)newBucket);
	memcpy(pointer, splitpoint, l);
	split->btr_length = ((pointer + l) - (UCHAR*)split);

	// the sum of the prefixes on the split page is the previous total minus
	// the prefixes found on the original page; the sum of the prefixes on the
	// original page must exclude the split node
	split->btr_prefix_total = newBucket->btr_prefix_total - prefix_total;
	const SLONG split_page = split_window.win_page.getPageNum();

	CCH_RELEASE(tdbb, &split_window);
	CCH_precedence(tdbb, window, split_window.win_page);
	CCH_MARK_MUST_WRITE(tdbb, window);

	// The split bucket is still residing in the scratch page. Copy it
	// back to the original buffer.  After cleaning up the last node,
	// we're done!

	// mark the end of the page; note that the end_bucket marker must
	// contain info about the first node on the next page. So we don't
	// overwrite the existing data.
	node.setEndBucket();
	pointer = node.writeNode(node.nodePointer, leafPage, false);
	newBucket->btr_length = pointer - (UCHAR*) newBucket;

	// Write jump information.
	bucket->btr_jump_interval = jumpAreaSize;
	bucket->btr_jump_size = jumpersNewSize;
	if (splitJumpNodeIndex > 0) {
		bucket->btr_jump_count = (UCHAR) (splitJumpNodeIndex - 1);
	}
	else {
		bucket->btr_jump_count = (UCHAR) jumpNodes->getCount();
	}

	pointer = bucket->btr_nodes;

	// Write jump nodes.
	index = 1;
	IndexJumpNode* walkJumpNode = jumpNodes->begin();
	for (size_t i = 0; i < jumpNodes->getCount(); i++, index++)
	{
		if (index <= bucket->btr_jump_count)
		{
			// Update offset to correct position.
			walkJumpNode[i].offset = walkJumpNode[i].offset + jumpersNewSize - jumpersOriginalSize;
			pointer = walkJumpNode[i].writeJumpNode(pointer);
		}
	}

	pointer = bucket->btr_nodes + bucket->btr_jump_size;

	memcpy(pointer, newBucket->btr_nodes + jumpersOriginalSize,
		newBucket->btr_length - BTR_SIZE - jumpersOriginalSize);
	bucket->btr_length = newBucket->btr_length + jumpersNewSize - jumpersOriginalSize;

	if (fragmentedOffset)
	{
		IndexJumpNode* walkJumpNode2 = jumpNodes->begin();
		for (size_t i = 0; i < jumpNodes->getCount(); i++, index++) {
			delete[] walkJumpNode2[i].data;
		}
	}

	// Update page information.
	bucket->btr_sibling = split_window.win_page.getPageNum();
	bucket->btr_prefix_total = prefix_total;
	// mark the bucket as non garbage-collectable until we can propagate
	// the split page up to the parent; otherwise its possible that the
	// split page we just created will be lost.
	insertion->iib_dont_gc_lock->disablePageGC(tdbb, window->win_page);

	if (original_page) {
		*original_page = window->win_page.getPageNum();
	}

	// now we need to go to the right sibling page and update its
	// left sibling pointer to point to the newly split page
	if (right_sibling)
	{
		bucket = (btree_page*) CCH_HANDOFF(tdbb, window, right_sibling, LCK_write, pag_index);
		CCH_MARK(tdbb, window);
		bucket->btr_left_sibling = split_window.win_page.getPageNum();
	}
	CCH_RELEASE(tdbb, window);

	// return the page number of the right sibling page
	if (sibling_page) {
		*sibling_page = right_sibling;
	}

	jumpNodes->clear();

	new_key->key_nulls = 0;
	if (unique)
	{
		// hvlad: it is important to set correct bitmap for all-NULL's key
		// else insert_node() at upper level will validate duplicates and
		// insert node into the end of duplicates chain instead of correct
		// place (in order of record numbers).

		temporary_key nullKey;
		BTR_make_null_key(tdbb, idx, &nullKey);

		if (new_key->key_length == nullKey.key_length &&
			memcmp(new_key->key_data, nullKey.key_data, nullKey.key_length) == 0)
		{
			new_key->key_nulls = nullKey.key_nulls;
		}
	}

	return split_page;
}


static INT64_KEY make_int64_key(SINT64 q, SSHORT scale)
{
/**************************************
 *
 *	m a k e _ i n t 6 4 _ k e y
 *
 **************************************
 *
 * Functional description
 *	Make an Index key for a 64-bit Integer value.
 *
 **************************************/

	// Following structure declared above in the modules global section
	//
	// static const struct {
	//     FB_UINT64 limit;		--- if abs(q) is >= this, ...
	//     SINT64 factor;		--- then multiply by this, ...
	//     SSHORT scale_change;	--- and add this to the scale.
	// } int64_scale_control[];
	//

	// Before converting the scaled int64 to a double, multiply it by the
	// largest power of 10 which will NOT cause an overflow, and adjust
	// the scale accordingly.  This ensures that two different
	// representations of the same value, entered at times when the
	// declared scale of the column was different, actually wind up
	// being mapped to the same key.

	int n = 0;
	const FB_UINT64 uq = (FB_UINT64) ((q >= 0) ? q : -q);	// absolute value
	while (uq < int64_scale_control[n].limit) {
		n++;
	}
	q *= int64_scale_control[n].factor;
	scale -= int64_scale_control[n].scale_change;

	INT64_KEY key;
	key.d_part = ((double) (q / 10000)) / powerof10(scale);
	key.s_part = (SSHORT) (q % 10000);

	return key;
}


#ifdef DEBUG_INDEXKEY
static void print_int64_key(SINT64 value, SSHORT scale, INT64_KEY key)
{
/**************************************
 *
 *	p r i n t _ i n t 6 4 _ k e y
 *
 **************************************
 *
 * Functional description
 *	Debugging function to print a key created out of an int64
 *	quantify.
 *
 **************************************/
	fprintf(stderr, "%20" QUADFORMAT"d  %4d  %.15e  %6d  ", value, scale, key.d_part, key.s_part);

	const UCHAR* p = (UCHAR*) &key;
	for (int n = 10; n--; n > 0) {
		fprintf(stderr, "%02x ", *p++);
	}

	fprintf(stderr, "\n");
	return;
}
#endif // DEBUG_INDEXKEY


string print_key(thread_db* tdbb, jrd_rel* relation, index_desc* idx, Record* record)
{
/**************************************
 *
 *	p r i n t _ k e y
 *
 **************************************
 *
 * Functional description
 *	Convert index key into textual representation.
 *
 **************************************/
	fb_assert(relation && idx && record);

	if (!(relation->rel_flags & REL_scanned) ||
		(relation->rel_flags & REL_being_scanned))
	{
		MET_scan_relation(tdbb, relation);
	}

	class Printer
	{
	public:
		explicit Printer(const dsc* desc)
		{
			const int MAX_KEY_STRING_LEN = 250;
			const char* const NULL_KEY_STRING = "NULL";

			if (!desc)
			{
				value = NULL_KEY_STRING;
				return;
			}

			fb_assert(!desc->isBlob());

			char temp[BUFFER_TINY];
			const char* str = NULL;
			const int len = MOV_make_string(desc, ttype_dynamic, &str,
											(vary*) temp, sizeof(temp));

			value.assign(str, len);

			if (desc->isText() || desc->isDateTime())
			{
				if (desc->dsc_dtype == dtype_text)
				{
					const char* const pad = (desc->dsc_sub_type == ttype_binary) ? "\0": " ";
					value.rtrim(pad);
				}

				if (desc->isText() && desc->getTextType() == ttype_binary)
				{
					string hex;
					char* s = hex.getBuffer(2 * len);
					for (int i = 0; i < len; i++)
					{
						sprintf(s, "%02X", (int) (unsigned char) str[i]);
						s += 2;
					}
					value = "x'" + hex + "'";
				}
				else
				{
					value = "'" + value + "'";
				}
			}

			if (value.length() > MAX_KEY_STRING_LEN)
			{
				value.resize(MAX_KEY_STRING_LEN);
				value += "...";
			}
		}

		const string& get() const
		{
			return value;
		}

	private:
		string value;
	};

	string key, value;

	try
	{
		if (idx->idx_flags & idx_expressn)
		{
			bool notNull = false;
			const dsc* const desc = BTR_eval_expression(tdbb, idx, record, notNull);
			value = Printer(notNull ? desc : NULL).get();
			key += "<expression> = " + value;
		}
		else
		{
			for (USHORT i = 0; i < idx->idx_count; i++)
			{
				const USHORT field_id = idx->idx_rpt[i].idx_field;
				const jrd_fld* const field = MET_get_field(relation, field_id);

				if (field)
					value.printf("\"%s\"", field->fld_name.c_str());
				else
					value.printf("<field #%d>", field_id);

				key += value;

				dsc desc;
				const bool notNull = EVL_field(relation, record, field_id, &desc);
				value = Printer(notNull ? &desc : NULL).get();
				key += " = " + value;

				if (i < idx->idx_count - 1)
					key += ", ";
			}
		}
	}
	catch (const Exception&)
	{
		return "";
	}

	return "(" + key + ")";
}


static contents remove_node(thread_db* tdbb, index_insertion* insertion, WIN* window)
{
/**************************************
 *
 *	r e m o v e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Remove an index node from a b-tree,
 * 	recursing down through the levels in case
 * 	we need to garbage collect pages.
 *
 **************************************/

	SET_TDBB(tdbb);
	//const Database* dbb = tdbb->getDatabase();
	index_desc* idx = insertion->iib_descriptor;
	btree_page* page = (btree_page*) window->win_buffer;

	// if we are on a leaf page, remove the leaf node
	if (page->btr_level == 0) {
		return remove_leaf_node(tdbb, insertion, window);
	}

	while (true)
	{
		const ULONG number = find_page(page, insertion->iib_key, idx, insertion->iib_number);

		// we should always find the node, but let's make sure
		if (number == END_LEVEL)
		{
			CCH_RELEASE(tdbb, window);
#ifdef DEBUG_BTR
			CORRUPT(204);	// msg 204 index inconsistent
#endif
			return contents_above_threshold;
		}

		// recurse to the next level down; if we are about to fetch a
		// level 0 page, make sure we fetch it for write
		if (number != END_BUCKET)
		{

			// handoff down to the next level, retaining the parent page number
			const SLONG parent_number = window->win_page.getPageNum();
			page = (btree_page*) CCH_HANDOFF(tdbb, window, number, (SSHORT)
				((page->btr_level == 1) ? LCK_write : LCK_read), pag_index);

			// if the removed node caused the page to go below the garbage collection
			// threshold, and the database was created by a version of the engine greater
			// than 8.2, then we can garbage-collect the page
			const contents result = remove_node(tdbb, insertion, window);

			if (result != contents_above_threshold)
			{
				return garbage_collect(tdbb, window, parent_number);
			}

			if (window->win_bdb) {
				CCH_RELEASE(tdbb, window);
			}
			return contents_above_threshold;
		}

		// we've hit end of bucket, so go to the sibling looking for the node
		page = (btree_page*) CCH_HANDOFF(tdbb, window, page->btr_sibling, LCK_read, pag_index);
	}

	// NOTREACHED
	return contents_empty;	// superfluous return to shut lint up
}


static contents remove_leaf_node(thread_db* tdbb, index_insertion* insertion, WIN* window)
{
/**************************************
 *
 *	r e m o v e _ l e a f _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Remove an index node from the leaf level.
 *
 **************************************/
	SET_TDBB(tdbb);
	btree_page* page = (btree_page*) window->win_buffer;
	temporary_key* key = insertion->iib_key;

	const index_desc* const idx = insertion->iib_descriptor;
	const bool primary = (idx->idx_flags & idx_primary);
	const bool unique = (idx->idx_flags & idx_unique);
	const bool key_all_nulls = (key->key_nulls == (1 << idx->idx_count) - 1);
	const bool validateDuplicates = (unique && !key_all_nulls) || primary;

	// Look for the first node with the value to be removed.
	UCHAR* pointer;
	USHORT prefix;
	while (!(pointer = find_node_start_point(page, key, 0, &prefix,
			(idx->idx_flags & idx_descending),
			false, false,
			(validateDuplicates ? NO_VALUE : insertion->iib_number))))
	{
		page = (btree_page*) CCH_HANDOFF(tdbb, window, page->btr_sibling, LCK_write, pag_index);
	}

	// Make sure first node looks ok
	IndexNode node;
	pointer = node.readNode(pointer, true);
	if (prefix > node.prefix || key->key_length != node.length + node.prefix)
	{
#ifdef DEBUG_BTR
		CCH_RELEASE(tdbb, window);
		CORRUPT(204);	// msg 204 index inconsistent
#endif
		return contents_above_threshold;
	}

	if (node.length && memcmp(node.data, key->key_data + node.prefix, node.length))
	{
#ifdef DEBUG_BTR
		CCH_RELEASE(tdbb, window);
		CORRUPT(204);	// msg 204 index inconsistent
#endif
		return contents_above_threshold;
	}


	// *****************************************************
	// AB: This becomes a very expensive task if there are
	// many duplicates inside the index (non-unique index)!
	// Therefore we also need to add the record-number to the
	// non-leaf pages and sort duplicates by record-number.
	// *****************************************************

	// now look through the duplicate nodes to find the one
	// with matching record number
	ULONG pages = 0;
	while (true)
	{
		// if we find the right one, quit
		if (insertion->iib_number == node.recordNumber && !node.isEndBucket && !node.isEndLevel)
		{
			break;
		}

		if (node.isEndLevel)
		{
#ifdef DEBUG_BTR
			CCH_RELEASE(tdbb, window);
			CORRUPT(204);	// msg 204 index inconsistent
#endif
			return contents_above_threshold;
		}

		// go to the next node and check that it is a duplicate
		if (!node.isEndBucket)
		{
			pointer = node.readNode(pointer, true);
			if (node.length != 0 || node.prefix != key->key_length)
			{
#ifdef DEBUG_BTR
				CCH_RELEASE(tdbb, window);
				CORRUPT(204);	// msg 204 index inconsistent
#endif
				return contents_above_threshold;
			}
			continue;
		}

		// if we hit the end of bucket, go to the right sibling page,
		// and check that the first node is a duplicate
		++pages;
		page = (btree_page*) CCH_HANDOFF(tdbb, window, page->btr_sibling, LCK_write, pag_index);

		pointer = page->btr_nodes + page->btr_jump_size;
		pointer = node.readNode(pointer, true);
		const USHORT len = node.length;
		if (len != key->key_length)
		{
#ifdef DEBUG_BTR
			CCH_RELEASE(tdbb, window);
			CORRUPT(204);		// msg 204 index inconsistent
#endif
			return contents_above_threshold;
		}

		if (len && memcmp(node.data, key->key_data, len))
		{
#ifdef DEBUG_BTR
			CCH_RELEASE(tdbb, window);
			CORRUPT(204);	// msg 204 index inconsistent
#endif
			return contents_above_threshold;
		}

		// Until deletion of duplicate nodes becomes efficient, limit
		// leaf level traversal by rescheduling.
		if (--tdbb->tdbb_quantum < 0)
			JRD_reschedule(tdbb, 0, true);
	}

	// If we've needed to search thru a significant number of pages, warn the
	// cache manager in case we come back this way
	if (pages > 75) {
		CCH_expand(tdbb, pages + 25);
	}

	return delete_node(tdbb, window, node.nodePointer);
}


static bool scan(thread_db* tdbb, UCHAR* pointer, RecordBitmap** bitmap, RecordBitmap* bitmap_and,
				 index_desc* idx, const IndexRetrieval* retrieval, USHORT prefix,
				 temporary_key* key,
				 bool& skipLowerKey, const temporary_key& lowerKey)
{
/**************************************
 *
 *	s c a n
 *
 **************************************
 *
 * Functional description
 *	Do an index scan.
 *  If we run over the bucket, return true.
 *  If we're completely done (passed END_LEVEL),
 *  return false.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	// if the search key is flagged to indicate a multi-segment index
	// stuff the key to the stuff boundary
	ULONG count;
	USHORT flag = retrieval->irb_generic;
	if ((flag & irb_partial) && (flag & irb_equality) &&
		!(flag & irb_starting) && !(flag & irb_descending))
	{
		count = STUFF_COUNT - ((key->key_length + STUFF_COUNT) % (STUFF_COUNT + 1));
		for (ULONG i = 0; i < count; i++) {
			key->key_data[key->key_length + i] = 0;
		}
		count += key->key_length;
	}
	else {
		count = key->key_length;
	}

	const USHORT to_segment = (idx->idx_count - retrieval->irb_upper_count);
	const UCHAR* const end_key = key->key_data + count;
	count -= key->key_length;

	const bool descending = (flag & irb_descending);
	const bool ignoreNulls = (flag & irb_ignore_null_value_key) && (idx->idx_count == 1);
	bool done = false;
	bool ignore = false;
	const bool skipUpperKey = (flag & irb_exclude_upper);
	const bool partLower = (retrieval->irb_lower_count < idx->idx_count);
	const bool partUpper = (retrieval->irb_upper_count < idx->idx_count);
	USHORT upperPrefix = prefix;

	// reset irb_equality flag passed for optimization
	flag &= ~(irb_equality | irb_ignore_null_value_key);
	flag &= ~(irb_exclude_lower | irb_exclude_upper);

	IndexNode node;
	pointer = node.readNode(pointer, true);
	const UCHAR* p = NULL;
	while (true)
	{
		if (node.isEndLevel)
			return false;

		if (descending && done && (node.prefix < prefix))
			return false;

		if ((key->key_length == 0) && !(key->key_flags & key_empty))
		{
			// Scanning for NULL keys
			if (to_segment == 0)
			{
				// All segments are expected to be NULL
				if (node.prefix + node.length > 0)
					return false;
			}
			else
			{
				// Up to (partial/starting) to_segment is expected to be NULL.
				if (node.length && (node.prefix == 0))
				{
					const UCHAR* q = node.data;
					if (*q > to_segment) {
						// hvlad: for desc indexes we must use *q^-1 ?
						return false;
					}
				}
			}
		}
		else if (node.prefix <= prefix)
		{
			prefix = node.prefix;
			upperPrefix = prefix;
			p = key->key_data + prefix;
			const UCHAR* q = node.data;
			USHORT l = node.length;
			for (; l; --l, prefix++)
			{
				if (skipUpperKey && partUpper)
				{
					if (upperPrefix >= key->key_length)
					{
						const USHORT segnum =
							idx->idx_count - (UCHAR)(descending ? ((*q) ^ -1) : *q) + 1;

						if (segnum >= retrieval->irb_upper_count)
							return false;
					}

					if (*p == *q)
						upperPrefix++;
				}

				if (p >= end_key)
				{
					if (flag)
						break;

					return false;
				}

				if (p > (end_key - count))
				{
					if (*p++ == *q++)
						break;

					continue;
				}

				if (*p < *q)
				{
					if ((flag & irb_starting) && (key->key_flags & key_empty))
						break;

					return false;
				}

				if (*p++ > *q++)
					break;
			}

			if (p >= end_key)
			{
				done = true;

				if ((l == 0) && skipUpperKey)
					return false;
			}
			else if (descending && (l == 0))
				return false;
		}

		if (node.isEndBucket)
		{
			// Our caller will fetch the next page
			return true;
		}

		// Ignore NULL-values, this is currently only available for single segment indexes.
		if (ignoreNulls)
		{
			ignore = false;
			if (descending)
			{
				if ((node.prefix == 0) && (node.length >= 1) && (node.data[0] == 255))
					return false;
			}
			else {
				ignore = (node.prefix + node.length == 0); // Ascending (prefix + length == 0)
			}
		}

		if (skipLowerKey)
			checkForLowerKeySkip(skipLowerKey, partLower, node, lowerKey, *idx, retrieval);

		if (!ignore && !skipLowerKey)
		{
			if ((flag & irb_starting) || !count)
			{
				if (!bitmap_and || bitmap_and->test(node.recordNumber.getValue()))
					RBM_SET(tdbb->getDefaultPool(), bitmap, node.recordNumber.getValue());
			}
			else if (p > (end_key - count))
			{
				if (!bitmap_and || bitmap_and->test(node.recordNumber.getValue()))
					RBM_SET(tdbb->getDefaultPool(), bitmap, node.recordNumber.getValue());
			}
		}

		pointer = node.readNode(pointer, true);
	}

	// NOTREACHED
	return false;	// superfluous return to shut lint up
}


void update_selectivity(index_root_page* root, USHORT id, const SelectivityList& selectivity)
{
/**************************************
 *
 *	u p d a t e _ s e l e c t i v i t y
 *
 **************************************
 *
 * Functional description
 *	Update selectivity on the index root page.
 *
 **************************************/
	//const Database* dbb = GET_DBB();

	index_root_page::irt_repeat* irt_desc = &root->irt_rpt[id];
	const USHORT idx_count = irt_desc->irt_keys;
	fb_assert(selectivity.getCount() == idx_count);

	// dimitr: per-segment selectivities exist only for ODS11 and above
	irtd* key_descriptor = (irtd*) ((UCHAR*) root + irt_desc->irt_desc);
	for (int i = 0; i < idx_count; i++, key_descriptor++)
		key_descriptor->irtd_selectivity = selectivity[i];
}
