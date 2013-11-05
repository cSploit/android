/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         evl.cpp
 *      DESCRIPTION:    Expression evaluation
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

/*
 * Modified by: Patrick J. P. Griffin
 * Date: 11/24/2000
 * Problem:   select count(0)+1 from rdb$relations where 0=1; returns 0
 *            In the EVL_group processing, the internal assigment for
 *            the literal in the computation is being done on every
 *            statement fetch, so if there are no statements fetched
 *            then the internal field never gets set.
 * Change:    Added an assignment process for the literal
 *            before the first fetch.
 *
 * Modified by: Neil McCalden
 * Date: 05 Jan 2001
 * Problem:   Firebird bug: 127375
 *            Group by on a calculated expression would cause segv
 *            when it encountered a NULL value as the calculation
 *            was trying reference a null pointer.
 * Change:    Test the null flag before trying to expand the value.
 *
 * 2001.6.17 Claudio Valderrama: Fix the annoying behavior that causes silent
 *	overflow in dialect 1. If you define the macro FIREBIRD_AVOID_DIALECT1_OVERFLOW
 *	it will work with double should an overflow happen. Otherwise, an error will be
 *	issued to the user if the overflow happens. The multiplication is done using
 *	SINT64 quantities. I had the impression that casting this SINT64 result to double
 *	when we detect overflow was faster than achieving the failed full multiplication
 *	with double operands again. Usage will tell the truth.
 *	For now, the aforementioned macro is enabled.
 * 2001.6.18 Claudio Valderrama: substring() is working with international charsets,
 *	thanks to Dave Schnepper's directions.
 * 2002.2.15 Claudio Valderrama: divide2() should not mangle negative values.
 * 2002.04.16 Paul Beach HP10 Port - (UCHAR*) desc.dsc_address = p; modified for HP
 *	Compiler
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2003.08.10 Claudio Valderrama: Fix SF bug# 784121.
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <string.h>
#include <math.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"

#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/sbm.h"
#include "../jrd/blb.h"
#include "gen/iberror.h"
#include "../jrd/scl.h"
#include "../jrd/lck.h"
#include "../jrd/lls.h"
#include "../jrd/intl.h"
#include "../jrd/intl_classes.h"
#include "../jrd/rse.h"
#include "../jrd/quad.h"
#include "../jrd/sort.h"
#include "../jrd/blr.h"
#include "../jrd/tra.h"
#include "../jrd/gdsassert.h"
#include "../common/classes/auto.h"
#include "../common/classes/timestamp.h"
#include "../common/classes/VaryStr.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cvt_proto.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/rse_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sort_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/align.h"
#include "../jrd/met_proto.h"
#include "../jrd/misc_func_ids.h"
#include "../common/config/config.h"
#include "../jrd/SysFunction.h"
#include "../common/classes/FpeControl.h"
#include "../common/classes/Aligner.h"

const int TEMP_LENGTH	= 128;

const SINT64 MAX_INT64_LIMIT	= MAX_SINT64 / 10;
const SINT64 MIN_INT64_LIMIT	= MIN_SINT64 / 10;

/*  *** DANGER DANGER WILL ROBINSON ***
 *  add(), multiply(), and divide() all take the same three arguments, but
 *  they don't all take them in the same order.  Be careful out there.
 *  The order should be made to agree as part of the next code cleanup.
 */

using namespace Jrd;
using namespace Firebird;

static dsc* add(const dsc*, const jrd_nod*, impure_value*);
static dsc* add2(const dsc*, const jrd_nod*, impure_value*);
static dsc* add_datetime(const dsc*, const jrd_nod*, impure_value*);
static dsc* add_sql_date(const dsc*, const jrd_nod*, impure_value*);
static dsc* add_sql_time(const dsc*, const jrd_nod*, impure_value*);
static dsc* add_timestamp(const dsc*, const jrd_nod*, impure_value*);
static dsc* binary_value(thread_db*, const jrd_nod*, impure_value*);
static dsc* cast(thread_db*, dsc*, const jrd_nod*, impure_value*);
static void compute_agg_distinct(thread_db*, jrd_nod*);
static dsc* concatenate(thread_db*, const dsc*, const dsc*, impure_value*);
static dsc* dbkey(thread_db*, const jrd_nod*, impure_value*);
static dsc* eval_statistical(thread_db*, jrd_nod*, impure_value*);
static dsc* extract(thread_db*, jrd_nod*, impure_value*);
static void fini_agg_distinct(thread_db* tdbb, const jrd_nod *const);
static SINT64 get_day_fraction(const dsc* d);
static dsc* get_mask(thread_db*, jrd_nod*, impure_value*);
static SINT64 get_timestamp_to_isc_ticks(const dsc* d);
static void init_agg_distinct(thread_db*, const jrd_nod*);
static dsc* lock_state(thread_db*, jrd_nod*, impure_value*);
static dsc* low_up_case(thread_db*, const dsc*, impure_value*,
	ULONG (TextType::*tt_str_to_case)(ULONG, const UCHAR*, ULONG, UCHAR*));
static dsc* multiply(const dsc*, impure_value*, const jrd_nod*);
static dsc* multiply2(const dsc*, impure_value*, const jrd_nod*);
static dsc* divide2(const dsc*, impure_value*, const jrd_nod*);
static dsc* negate_dsc(thread_db*, const dsc*, impure_value*);
static dsc* record_version(thread_db*, const jrd_nod*, impure_value*);
static bool reject_duplicate(const UCHAR*, const UCHAR*, void*);
static dsc* scalar(thread_db*, jrd_nod*, impure_value*);
static bool sleuth(thread_db*, jrd_nod*, const dsc*, const dsc*);
static bool string_boolean(thread_db*, jrd_nod*, dsc*, dsc*, bool);
static bool string_function(thread_db*, jrd_nod*, SLONG, const UCHAR*, SLONG, const UCHAR*, USHORT, bool);
static dsc* string_length(thread_db*, jrd_nod*, impure_value*);
static dsc* substring(thread_db*, impure_value*, dsc*, const dsc*, const dsc*);
static dsc* trim(thread_db*, jrd_nod*, impure_value*);
static dsc* internal_info(thread_db*, const dsc*, impure_value*);


const SINT64 SECONDS_PER_DAY			= 24 * 60 * 60;
const SINT64 ISC_TICKS_PER_DAY			= SECONDS_PER_DAY * ISC_TIME_SECONDS_PRECISION;
const SCHAR DIALECT_3_TIMESTAMP_SCALE	= -9;
const SCHAR DIALECT_1_TIMESTAMP_SCALE	= 0;

#ifdef SCROLLABLE_CURSORS
static const rse_get_mode g_RSE_get_mode = RSE_get_next;
#else
static const rse_get_mode g_RSE_get_mode = RSE_get_forward;
#endif




dsc* EVL_assign_to(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *      E V L _ a s s i g n _ t o
 *
 **************************************
 *
 * Functional description
 *      Evaluate the descriptor of the
 *      destination node of an assignment.
 *
 **************************************/
	dsc* desc;
	Format* format;
	jrd_nod* message;
	Record* record;

	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);

/* The only nodes that can be assigned to are: argument, field and variable. */

	int arg_number;

	switch (node->nod_type)
	{
	case nod_argument:
		message = node->nod_arg[e_arg_message];
		format = (Format*) message->nod_arg[e_msg_format];
		arg_number = (int) (IPTR) node->nod_arg[e_arg_number];
		desc = &format->fmt_desc[arg_number];
		impure->vlu_desc.dsc_address =
			(UCHAR *) request + message->nod_impure + (IPTR) desc->dsc_address;
		impure->vlu_desc.dsc_dtype = desc->dsc_dtype;
		impure->vlu_desc.dsc_length = desc->dsc_length;
		impure->vlu_desc.dsc_scale = desc->dsc_scale;
		impure->vlu_desc.dsc_sub_type = desc->dsc_sub_type;
		if (DTYPE_IS_TEXT(desc->dsc_dtype) &&
			((INTL_TTYPE(desc) == ttype_dynamic) || (INTL_GET_CHARSET(desc) == CS_dynamic)))
		{
			/* Value is a text value, we're assigning it back to the user
			   process, user process has not specified a subtype, user
			   process specified dynamic translation and the dsc isn't from
			   a 3.3 type request (blr_cstring2 instead of blr_cstring) so
			   convert the charset to the declared charset of the process. */

			INTL_ASSIGN_DSC(&impure->vlu_desc,
							tdbb->getAttachment()->att_charset, COLLATE_NONE);
		}
		return &impure->vlu_desc;

	case nod_field:
		record = request->req_rpb[(int) (IPTR) node->nod_arg[e_fld_stream]].rpb_record;
		if (!EVL_field(0, record, (USHORT)(IPTR) node->nod_arg[e_fld_id], &impure->vlu_desc))
		{
			// The below condition means that EVL_field() returned
			// a read-only dummy value which cannot be assigned to.
			// The usual reason is a field being unexpectedly dropped.
			if (impure->vlu_desc.dsc_address &&
				!(impure->vlu_desc.dsc_flags & DSC_null))
			{
				ERR_post(Arg::Gds(isc_field_disappeared));
			}
		}
		if (!impure->vlu_desc.dsc_address)
			ERR_post(Arg::Gds(isc_read_only_field));
		return &impure->vlu_desc;

	case nod_null:
		return NULL;

	case nod_variable:
		// Calculate descriptor
		node = node->nod_arg[e_var_variable];
		impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
		return &impure->vlu_desc;

	default:
		BUGCHECK(229);			/* msg 229 EVL_assign_to: invalid operation */
	}
	return NULL;
}


RecordBitmap** EVL_bitmap(thread_db* tdbb, jrd_nod* node, RecordBitmap* bitmap_and)
{
/**************************************
 *
 *      E V L _ b i t m a p
 *
 **************************************
 *
 * Functional description
 *      Evaluate bitmap valued expression.
 *
 **************************************/

	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	switch (node->nod_type)
	{
	case nod_bit_and:
		{
			RecordBitmap** bitmap = EVL_bitmap(tdbb, node->nod_arg[0], bitmap_and);
			if (!(*bitmap) || !(*bitmap)->getFirst())
				return bitmap;

			return EVL_bitmap(tdbb, node->nod_arg[1], *bitmap);
		}

	case nod_bit_or:
		return RecordBitmap::bit_or(
			EVL_bitmap(tdbb, node->nod_arg[0], bitmap_and),
			EVL_bitmap(tdbb, node->nod_arg[1], bitmap_and));

	case nod_bit_in:
		{
			RecordBitmap** inv_bitmap = EVL_bitmap(tdbb, node->nod_arg[0], bitmap_and);
			BTR_evaluate(tdbb,
						 reinterpret_cast<IndexRetrieval*>(node->nod_arg[1]->nod_arg[e_idx_retrieval]),
						 inv_bitmap, bitmap_and);
			return inv_bitmap;
		}

	case nod_bit_dbkey:
		{
			impure_inversion* impure = (impure_inversion*) ((SCHAR*) tdbb->getRequest() + node->nod_impure);
			RecordBitmap::reset(impure->inv_bitmap);
			const dsc* desc = EVL_expr(tdbb, node->nod_arg[0]);

			if (!(tdbb->getRequest()->req_flags & req_null) &&
				desc->dsc_length == sizeof(RecordNumber::Packed))
			{
				const USHORT id = (USHORT)(IPTR) node->nod_arg[1];
				Firebird::Aligner<RecordNumber::Packed> alignedNumbers(desc->dsc_address, desc->dsc_length);
				const RecordNumber::Packed* numbers = alignedNumbers;
				RecordNumber rel_dbkey;
				rel_dbkey.bid_decode(&numbers[id]);
				// Decrement the value in order to switch back to the zero based numbering
				// (from the user point of view the DB_KEY numbering starts from one)
				rel_dbkey.decrement();
				if (!bitmap_and || bitmap_and->test(rel_dbkey.getValue()))
					RBM_SET(tdbb->getDefaultPool(), &impure->inv_bitmap, rel_dbkey.getValue());
			}

			return &impure->inv_bitmap;
		}

	case nod_index:
		{
			impure_inversion* impure = (impure_inversion*) ((SCHAR*) tdbb->getRequest() + node->nod_impure);
			RecordBitmap::reset(impure->inv_bitmap);
			BTR_evaluate(tdbb,
						 reinterpret_cast<IndexRetrieval*>(node->nod_arg[e_idx_retrieval]),
						 &impure->inv_bitmap, bitmap_and);
			return &impure->inv_bitmap;
		}

	default:
		BUGCHECK(230);			/* msg 230 EVL_bitmap: invalid operation */
	}
	return NULL;
}


bool EVL_boolean(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *      E V L _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *      Evaluate a boolean.
 *
 **************************************/
	dsc* desc[2];
	bool value;
	SSHORT comparison;
	impure_value*    impure;
	bool computed_invariant = false;

	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

/* Handle and pre-processing possible for various nodes.  This includes
   evaluating argument and checking NULL flags */

	jrd_req* request = tdbb->getRequest();
	jrd_nod** ptr = node->nod_arg;

	switch (node->nod_type)
	{
	case nod_contains:
	case nod_starts:
	case nod_matches:
	case nod_like:
	case nod_equiv:
	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_lss:
	case nod_leq:
	case nod_between:
	case nod_similar:
	case nod_sleuth:
		{
			request->req_flags &= ~req_same_tx_upd;
			SSHORT force_equal = 0;

			/* Evaluate arguments.  If either is null, result is null, but in
			   any case, evaluate both, since some expressions may later depend
			   on mappings which are developed here */

			const jrd_nod* rec_version = *ptr;
			desc[0] = EVL_expr(tdbb, *ptr++);
			const ULONG flags = request->req_flags;
			request->req_flags &= ~req_null;
			force_equal |= request->req_flags & req_same_tx_upd;

			// Currently only nod_like, nod_contains, nod_starts and nod_similar may be marked invariant
			if (node->nod_flags & nod_invariant)
			{
				impure = reinterpret_cast<impure_value*>((SCHAR *)request + node->nod_impure);

				// Check that data type of operand is still the same.
				// It may change due to multiple formats present in stream
				// System tables are the good example of such streams -
				// data coming from ini.epp has ASCII ttype, user data is UNICODE_FSS
				//
				// Note that value descriptor may be NULL pointer if value is SQL NULL
				if ((impure->vlu_flags & VLU_computed) && desc[0] &&
					(impure->vlu_desc.dsc_dtype != desc[0]->dsc_dtype ||
					 impure->vlu_desc.dsc_sub_type != desc[0]->dsc_sub_type ||
					 impure->vlu_desc.dsc_scale != desc[0]->dsc_scale)
					)
				{
					impure->vlu_flags &= ~VLU_computed;
				}

				if (impure->vlu_flags & VLU_computed)
				{
					if (impure->vlu_flags & VLU_null)
						request->req_flags |= req_null;
					else
						computed_invariant = true;
				}
				else
				{
					desc[1] = EVL_expr(tdbb, *ptr++);
					if (request->req_flags & req_null)
					{
						impure->vlu_flags |= VLU_computed;
						impure->vlu_flags |= VLU_null;
					}
					else
					{
						impure->vlu_flags &= ~VLU_null;

						// Search object depends on operand data type.
						// Thus save data type which we use to compute invariant
						if (desc[0])
						{
							impure->vlu_desc.dsc_dtype = desc[0]->dsc_dtype;
							impure->vlu_desc.dsc_sub_type = desc[0]->dsc_sub_type;
							impure->vlu_desc.dsc_scale = desc[0]->dsc_scale;
						}
						else
						{
							// Indicate we do not know type of expression.
							// This code will force pattern recompile for the next non-null value
							impure->vlu_desc.dsc_dtype = 0;
							impure->vlu_desc.dsc_sub_type = 0;
							impure->vlu_desc.dsc_scale = 0;
						}
					}
				}
			}
			else
				desc[1] = EVL_expr(tdbb, *ptr++);

			// An equivalence operator evaluates to true when both operands
			// are NULL and behaves like an equality operator otherwise.
			// Note that this operator never sets req_null flag

			if (node->nod_type == nod_equiv)
			{
				if ((flags & req_null) && (request->req_flags & req_null))
				{
					request->req_flags &= ~req_null;
					return true;
				}

				if ((flags & req_null) || (request->req_flags & req_null))
				{
					request->req_flags &= ~req_null;
					return false;
				}
			}

			// If either of expressions above returned NULL set req_null flag
			// and return false

			if (flags & req_null)
				request->req_flags |= req_null;

			if (request->req_flags & req_null)
				return false;

			force_equal |= request->req_flags & req_same_tx_upd;

			if (node->nod_flags & nod_comparison)
				comparison = MOV_compare(desc[0], desc[1]);

			/* If we are checking equality of record_version
			 * and same transaction updated the record,
			 * force equality.
			 */

			if (rec_version->nod_type == nod_rec_version && force_equal)
				comparison = 0;

			request->req_flags &= ~(req_null | req_same_tx_upd);
		}
		break;

	case nod_and:
		value = EVL_boolean(tdbb, *ptr++);
		break;

	case nod_or:
		value = EVL_boolean(tdbb, *ptr++);
		break;

	case nod_not:
		if ((*ptr)->nod_type == nod_ansi_any || (*ptr)->nod_type == nod_ansi_all)
		{
			request->req_flags |= req_ansi_not;
		}
		value = EVL_boolean(tdbb, *ptr++);
		break;

	default:   /* Shut up some compiler warnings */
		break;
	}

/* Evaluate node */

	// TODO: Verify and remove this flag once FB1.5beta3 is out.
	// Default to not eval complete expression (i.e. do short-circuit
	// optimizied evaluation). Both to get possible early warnings from
	// users, and to default to the faster of the two options.
	static bool bEvalCompleteExpression = Config::getCompleteBooleanEvaluation();

	switch (node->nod_type)
	{
	case nod_and:
		{
			/* for and,
			   if either operand is FALSE, then the result is FALSE;
			   if both are TRUE, the result is TRUE;
			   otherwise, the result is NULL

			   op 1            op 2            result
			   ----            ----            ------
			   F               F                F
			   F               T                F
			   F               N                F
			   T               F                F
			   T               T                T
			   T               N                N
			   N               F                F
			   N               T                N
			   N               N                N

			 */

			/* save null state and get other operand */

			const USHORT firstnull = request->req_flags & req_null;
			request->req_flags &= ~req_null;

			if (!bEvalCompleteExpression && !value && !firstnull)
			{
				// First term is FALSE, why the whole expression is false.
				// NULL flag is already turned off a few lines above.
				return false;
			}

			const bool value2 = EVL_boolean(tdbb, *ptr);
			const USHORT secondnull = request->req_flags & req_null;
			request->req_flags &= ~req_null;

			if ((!value && !firstnull) || (!value2 && !secondnull)) {
				return false;	/* at least one operand was false */
			}

			if (value && value2) {
				return true;	/* both true */
			}
			request->req_flags |= req_null;
			return false;		/* otherwise, return null */
		}

	case nod_any:
	case nod_ansi_any:
	case nod_ansi_all:
		{
			USHORT* invariant_flags;

			if (node->nod_flags & nod_invariant)
			{
				impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
				invariant_flags = & impure->vlu_flags;
				if (*invariant_flags & VLU_computed) {
					/* An invariant node has already been computed. */

					if ((node->nod_type == nod_ansi_any) && (*invariant_flags & VLU_null))
					{
						request->req_flags |= req_null;
					}
					else
					{
						request->req_flags &= ~req_null;
					}
					return impure->vlu_misc.vlu_short != 0;
				}
			}

			/* for ansi ANY clauses (and ALL's, which are negated ANY's)
			   the unoptimized boolean expression must be used, since the
			   processing of these clauses is order dependant (see rse.cpp) */

			RecordSource* select = (RecordSource*) (node->nod_arg[e_any_rsb]);
			if (node->nod_type != nod_any)
			{
				select->rsb_any_boolean = ((RecordSelExpr*) (node->nod_arg[e_any_rse]))->rse_boolean;
				if (node->nod_type == nod_ansi_any) {
					request->req_flags |= req_ansi_any;
				}
				else {
					request->req_flags |= req_ansi_all;
					// dimitr:	Even if we can evaluate ANY without a residual
					//			boolean (what I still doubt), it's impossible
					//			for ALL. Hence this assertion.
					fb_assert(select->rsb_type == rsb_boolean);
				}
			}
			RSE_open(tdbb, select);
			value = RSE_get_record(tdbb, select, g_RSE_get_mode);
			RSE_close(tdbb, select);
			if (node->nod_type == nod_any)
				request->req_flags &= ~req_null;

			/* If this is an invariant node, save the return value. */

			if (node->nod_flags & nod_invariant)
			{
				*invariant_flags |= VLU_computed;
				if ((node->nod_type != nod_any) && (request->req_flags & req_null))
				{
					*invariant_flags |= VLU_null;
				}
				impure->vlu_misc.vlu_short = value ? TRUE : FALSE;
			}
			return value;
		}

	case nod_contains:
	case nod_starts:
	case nod_matches:
	case nod_like:
	case nod_similar:
		return string_boolean(tdbb, node, desc[0], desc[1], computed_invariant);

	case nod_sleuth:
		return sleuth(tdbb, node, desc[0], desc[1]);

	case nod_missing:
		EVL_expr(tdbb, *ptr);
		if (request->req_flags & req_null) {
			value = true;
			request->req_flags &= ~req_null;
		}
		else {
			value = false;
		}
		return value;

	case nod_not:
		if (request->req_flags & req_null)
			return false;
		return !value;

	case nod_or:
		{
			/* for or, if either operand is TRUE, then the result is TRUE; if
			   both are FALSE, the result is FALSE; otherwise, the result is NULL

			   op 1            op 2            result
			   ----            ----            ------
			   F               F                F
			   F               T                T
			   F               N                N
			   T               F                T
			   T               T                T
			   T               N                T
			   N               F                N
			   N               T                T
			   N               N                N


			   also, preserve first operand's value and null state, but still
			   evaluate second operand, since latter field mappings may
			   depend on the evaluation */

			const ULONG flags = request->req_flags;
			request->req_flags &= ~req_null;

			if (!bEvalCompleteExpression && value)
			{
				// First term is TRUE, why the whole expression is true.
				// NULL flag is already turned off a few lines above.
				return true;
			}

			const bool value2 = EVL_boolean(tdbb, *ptr);
			if (value || value2) {
				request->req_flags &= ~req_null;
				return true;
			}

			/* restore saved NULL state */

			if (flags & req_null) {
				request->req_flags |= req_null;
			}
			return false;
		}

	case nod_unique:
		{
			USHORT* invariant_flags;

			if (node->nod_flags & nod_invariant)
			{
				impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
				invariant_flags = & impure->vlu_flags;
				if (*invariant_flags & VLU_computed)
				{
					/* An invariant node has already been computed. */

					request->req_flags &= ~req_null;
					return impure->vlu_misc.vlu_short != 0;
				}
			}

			RecordSource* urs = reinterpret_cast<RecordSource*>(node->nod_arg[e_any_rsb]);
			RSE_open(tdbb, urs);
			value = RSE_get_record(tdbb, urs, g_RSE_get_mode);
			if (value)
			{
				value = !RSE_get_record(tdbb, urs, g_RSE_get_mode);
			}
			RSE_close(tdbb, urs);
			request->req_flags &= ~req_null;

			/* If this is an invariant node, save the return value. */

			if (node->nod_flags & nod_invariant)
			{
				*invariant_flags |= VLU_computed;
				impure->vlu_misc.vlu_short = value ? TRUE : FALSE;
			}
			return value;
		}

	case nod_equiv:
	case nod_eql:
		return (comparison == 0);
	case nod_neq:
		return (comparison != 0);
	case nod_gtr:
		return (comparison > 0);
	case nod_geq:
		return (comparison >= 0);
	case nod_lss:
		return (comparison < 0);
	case nod_leq:
		return (comparison <= 0);

	case nod_between:
		desc[1] = EVL_expr(tdbb, node->nod_arg[2]);
		if (request->req_flags & req_null)
			return false;
		return (comparison >= 0 && MOV_compare(desc[0], desc[1]) <= 0);

	case nod_stmt_expr:
		EXE_looper(tdbb, request, node);
		return EVL_boolean(tdbb, node->nod_arg[e_stmt_expr_expr]);

	default:
		BUGCHECK(231);			/* msg 231 EVL_boolean: invalid operation */
	}
	return false;
}


dsc* EVL_expr(thread_db* tdbb, jrd_nod* const node)
{
/**************************************
 *
 *      E V L _ e x p r
 *
 **************************************
 *
 * Functional description
 *      Evaluate a value expression.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	if (!node)
		BUGCHECK(303);			/* msg 303 Invalid expression for evaluation */

	SET_TDBB(tdbb);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	impure_value* const impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
	request->req_flags &= ~req_null;

/* Do a preliminary screen for either simple nodes or nodes that
   are special cased elsewhere */

	switch (node->nod_type)
	{
	case nod_add:
	case nod_subtract:
	case nod_divide:
	case nod_multiply:
	case nod_add2:
	case nod_subtract2:
	case nod_divide2:
	case nod_multiply2:
	case nod_concatenate:
		return binary_value(tdbb, node, impure);

	case nod_argument:
		{
			const dsc* desc;

			if (node->nod_arg[e_arg_flag]) {
				desc = EVL_expr(tdbb, node->nod_arg[e_arg_flag]);
				if (MOV_get_long(desc, 0)) {
					request->req_flags |= req_null;
				}
			}
			const jrd_nod* message = node->nod_arg[e_arg_message];
			const Format* format = (Format*) message->nod_arg[e_msg_format];
			desc = &format->fmt_desc[(int)(IPTR) node->nod_arg[e_arg_number]];

			impure->vlu_desc.dsc_address = (UCHAR *) request +
				message->nod_impure + (IPTR) desc->dsc_address;
			impure->vlu_desc.dsc_dtype = desc->dsc_dtype;
			impure->vlu_desc.dsc_length = desc->dsc_length;
			impure->vlu_desc.dsc_scale = desc->dsc_scale;
			impure->vlu_desc.dsc_sub_type = desc->dsc_sub_type;

			if (impure->vlu_desc.dsc_dtype == dtype_text)
				EVL_adjust_text_descriptor(tdbb, &impure->vlu_desc);

			USHORT* impure_flags = (USHORT*) ((UCHAR *) request +
				(IPTR) message->nod_arg[e_msg_impure_flags] +
				(sizeof(USHORT) * (IPTR) node->nod_arg[e_arg_number]));

			if (!(*impure_flags & VLU_checked))
			{
				if (node->nod_arg[e_arg_info])
				{
					EVL_validate(tdbb,
						Item(nod_argument, (IPTR) node->nod_arg[e_arg_message]->nod_arg[e_msg_number],
							(IPTR) node->nod_arg[e_arg_number]),
						reinterpret_cast<const ItemInfo*>(node->nod_arg[e_arg_info]),
						&impure->vlu_desc, request->req_flags & req_null);
				}
				*impure_flags |= VLU_checked;
			}

			return &impure->vlu_desc;
		}

	case nod_dbkey:
		return dbkey(tdbb, node, impure);

	case nod_rec_version:
		return record_version(tdbb, node, impure);

	case nod_field:
		{
			const USHORT id = (USHORT)(IPTR) node->nod_arg[e_fld_id];
			record_param& rpb = request->req_rpb[(USHORT)(IPTR) node->nod_arg[e_fld_stream]];
			Record* record = rpb.rpb_record;
			jrd_rel* relation = rpb.rpb_relation;
			// In order to "map a null to a default" value (in EVL_field()),
			// the relation block is referenced.
			// Reference: Bug 10116, 10424

			if (!EVL_field(relation, record, id, &impure->vlu_desc))
				request->req_flags |= req_null;
			else
			{
				const Format* compileFormat = (Format*) node->nod_arg[e_fld_format];

				// ASF: CORE-1432 - If the the record is not on the latest format, upgrade it.
				// AP: for fields that are missing in original format use record's one.
				if (compileFormat &&
					record->rec_format->fmt_version != compileFormat->fmt_version &&
					id < compileFormat->fmt_desc.getCount() &&
					!DSC_EQUIV(&impure->vlu_desc, &compileFormat->fmt_desc[id], true))
				{
					dsc desc = impure->vlu_desc;
					impure->vlu_desc = compileFormat->fmt_desc[id];

					if (impure->vlu_desc.isText())
					{
						// Allocate a string block of sufficient size.
						VaryingString* string = impure->vlu_string;
						if (string && string->str_length < impure->vlu_desc.dsc_length)
						{
							delete string;
							string = NULL;
						}

						if (!string)
						{
							string = impure->vlu_string = FB_NEW_RPT(*tdbb->getDefaultPool(),
								impure->vlu_desc.dsc_length) VaryingString();
							string->str_length = impure->vlu_desc.dsc_length;
						}

						impure->vlu_desc.dsc_address = string->str_data;
					}
					else
						impure->vlu_desc.dsc_address = (UCHAR*) &impure->vlu_misc;

					MOV_move(tdbb, &desc, &impure->vlu_desc);
				}
			}

			if (!relation || !(relation->rel_flags & REL_system))
			{
				if (impure->vlu_desc.dsc_dtype == dtype_text)
					EVL_adjust_text_descriptor(tdbb, &impure->vlu_desc);
			}

			return &impure->vlu_desc;
		}

	case nod_derived_expr:
	{
		const UCHAR streamCount = (UCHAR)(IPTR) node->nod_arg[e_derived_expr_stream_count];
		const USHORT* streamList = (USHORT*) node->nod_arg[e_derived_expr_stream_list];

		for (UCHAR i = 0; i < streamCount; ++i)
		{
			if (request->req_rpb[streamList[i]].rpb_number.isValid())
				return EVL_expr(tdbb, node->nod_arg[e_derived_expr_expr]);
		}

		request->req_flags |= req_null;
		return NULL;
	}

	case nod_function:
		FUN_evaluate(tdbb, reinterpret_cast<UserFunction*>(node->nod_arg[e_fun_function]),
				     node->nod_arg[e_fun_args], impure);
		return &impure->vlu_desc;

	case nod_sys_function:
		{
			const SysFunction* sysFunction = reinterpret_cast<SysFunction*>(node->nod_arg[e_sysfun_function]);
			return sysFunction->evlFunc(tdbb, sysFunction, node->nod_arg[e_sysfun_args], impure);
		}

	case nod_literal:
		return &((Literal*) node)->lit_desc;

	case nod_lock_state:
		return lock_state(tdbb, node, impure);

	case nod_null:
		request->req_flags |= req_null;
		return NULL;

	case nod_prot_mask:
		return get_mask(tdbb, node, impure);

	case nod_current_time:
	case nod_current_date:
	case nod_current_timestamp:
		{
			// Use the request timestamp

			fb_assert(!request->req_timestamp.isEmpty());
			ISC_TIMESTAMP enc_times = request->req_timestamp.value();

			memset(&impure->vlu_desc, 0, sizeof(impure->vlu_desc));
			impure->vlu_desc.dsc_address = (UCHAR *) &impure->vlu_misc.vlu_timestamp;

			if (node->nod_type == nod_current_time || node->nod_type == nod_current_timestamp)
			{
				const int precision = (int)(IPTR) node->nod_arg[0];
				fb_assert(precision >= 0);
				Firebird::TimeStamp::round_time(enc_times.timestamp_time, precision);
			}

			switch (node->nod_type)
			{
			case nod_current_time:
				impure->vlu_desc.dsc_dtype = dtype_sql_time;
				impure->vlu_desc.dsc_length = type_lengths[dtype_sql_time];
				*(ULONG *) impure->vlu_desc.dsc_address = enc_times.timestamp_time;
				break;
			case nod_current_date:
				impure->vlu_desc.dsc_dtype = dtype_sql_date;
				impure->vlu_desc.dsc_length = type_lengths[dtype_sql_date];
				*(ULONG *) impure->vlu_desc.dsc_address = enc_times.timestamp_date;
				break;
			case nod_current_timestamp:
				impure->vlu_desc.dsc_dtype = dtype_timestamp;
				impure->vlu_desc.dsc_length = type_lengths[dtype_timestamp];
				*((ISC_TIMESTAMP *) impure->vlu_desc.dsc_address) = enc_times;
				break;
			default:
				fb_assert(false);
				break;
			}
		}
		return &impure->vlu_desc;

	case nod_user_name:
		{
			impure->vlu_desc.dsc_dtype = dtype_text;
			impure->vlu_desc.dsc_sub_type = 0;
			impure->vlu_desc.dsc_scale = 0;
			INTL_ASSIGN_TTYPE(&impure->vlu_desc, ttype_metadata);
			const char* cur_user = 0;
			if (tdbb->getAttachment()->att_user)
			{
				cur_user = tdbb->getAttachment()->att_user->usr_user_name.c_str();
				impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(const_cast<char*>(cur_user));
			}
			if (cur_user)
				impure->vlu_desc.dsc_length = strlen(cur_user);
			else
				impure->vlu_desc.dsc_length = 0;
		}
		return &impure->vlu_desc;

	// CVC: Current role will get a validated role; IE one that exists.
	case nod_current_role:
		{
			impure->vlu_desc.dsc_dtype = dtype_text;
			impure->vlu_desc.dsc_sub_type = 0;
			impure->vlu_desc.dsc_scale = 0;
			INTL_ASSIGN_TTYPE(&impure->vlu_desc, ttype_metadata);
			const char* cur_role = 0;
			if (tdbb->getAttachment()->att_user)
			{
				cur_role = tdbb->getAttachment()->att_user->usr_sql_role_name.c_str();
				impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(const_cast<char*>(cur_role));
			}
			if (cur_role)
				impure->vlu_desc.dsc_length = strlen(cur_role);
			else
				impure->vlu_desc.dsc_length = 0;
		}
		return &impure->vlu_desc;

	case nod_extract:
		return extract(tdbb, node, impure);

	case nod_strlen:
		return string_length(tdbb, node, impure);

	case nod_max:
	case nod_min:
	case nod_count:
	//case nod_count2:
	case nod_average:
	case nod_average2:
	case nod_total:
	case nod_from:
		return eval_statistical(tdbb, node, impure);

	case nod_scalar:
		return scalar(tdbb, node, impure);

	case nod_variable:
		{
			const jrd_nod* node2 = node->nod_arg[e_var_variable];

			impure_value* impure2 = (impure_value*) ((SCHAR *) request + node2->nod_impure);
			if (impure2->vlu_desc.dsc_flags & DSC_null)
				request->req_flags |= req_null;

			impure->vlu_desc = impure2->vlu_desc;

			if (impure->vlu_desc.dsc_dtype == dtype_text)
				EVL_adjust_text_descriptor(tdbb, &impure->vlu_desc);

			if (!(impure2->vlu_flags & VLU_checked))
			{
				if (node->nod_arg[e_var_info])
				{
					EVL_validate(tdbb, Item(nod_variable, (IPTR) node->nod_arg[e_var_id]),
						reinterpret_cast<const ItemInfo*>(node->nod_arg[e_var_info]),
						&impure->vlu_desc, impure->vlu_desc.dsc_flags & DSC_null);
				}
				impure2->vlu_flags |= VLU_checked;
			}

			return &impure->vlu_desc;
		}

	case nod_domain_validation:
		if (request->req_domain_validation == NULL ||
			(request->req_domain_validation->dsc_flags & DSC_null))
		{
			request->req_flags |= req_null;
		}
		return request->req_domain_validation;

	case nod_value_if:
		return EVL_expr(tdbb, (EVL_boolean(tdbb, node->nod_arg[0])) ?
			node->nod_arg[1] : node->nod_arg[2]);

	case nod_trim:
		return trim(tdbb, node, impure);

	default:   // Shut up some compiler warnings
		break;

	case nod_stmt_expr:
		EXE_looper(tdbb, request, node);
		return EVL_expr(tdbb, node->nod_arg[e_stmt_expr_expr]);
	}

	// Evaluate arguments

	dsc* values[3];

	if (node->nod_count) {
		fb_assert(node->nod_count <= 3);
		dsc** v = values;
		jrd_nod** ptr = node->nod_arg;
		for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end;)
		{
			*v++ = EVL_expr(tdbb, *ptr++);

			if (request->req_flags & req_null)
			{
				// ASF: CAST target type may be constrained
				if (node->nod_type == nod_cast)
					*(v - 1) = NULL;
				else
					return NULL;
			}
		}
	}

	switch (node->nod_type)
	{
	case nod_gen_id:		// return a 32-bit generator value
		{
			SLONG temp = (SLONG) DPM_gen_id(tdbb, (SLONG)(IPTR) node->nod_arg[e_gen_id],
								false, MOV_get_int64(values[0], 0));
			impure->make_long(temp);
		}
		return &impure->vlu_desc;

	case nod_gen_id2:
		{
			SINT64 temp = DPM_gen_id(tdbb, (IPTR) node->nod_arg[e_gen_id],
									 false, MOV_get_int64(values[0], 0));
			impure->make_int64(temp);
		}
		return &impure->vlu_desc;

	case nod_negate:
		return negate_dsc(tdbb, values[0], impure);

	case nod_substr:
		return substring(tdbb, impure, values[0], values[1], values[2]);

	case nod_upcase:
		return low_up_case(tdbb, values[0], impure, &TextType::str_to_upper);

	case nod_lowcase:
		return low_up_case(tdbb, values[0], impure, &TextType::str_to_lower);

	case nod_cast:
		return cast(tdbb, values[0], node, impure);

	case nod_internal_info:
		return internal_info(tdbb, values[0], impure);

	default:
		BUGCHECK(232);		/* msg 232 EVL_expr: invalid operation */
	}

	return NULL;
}


bool EVL_field(jrd_rel* relation, Record* record, USHORT id, dsc* desc)
{
/**************************************
 *
 *      E V L _ f i e l d
 *
 **************************************
 *
 * Functional description
 *      Evaluate a field by filling out a descriptor.
 *
 **************************************/

	DEV_BLKCHK(record, type_rec);

	if (!record)
	{
		// ASF: Usage of ERR_warning with Arg::Gds (instead of Arg::Warning) is correct here.
		// Maybe not all code paths are prepared for throwing an exception here,
		// but it will leave the engine as an error (when testing for req_warning).
		ERR_warning(Arg::Gds(isc_no_cur_rec));
		return false;
	}

	const Format* format = record->rec_format;

	if (format && id < format->fmt_count) {
		*desc = format->fmt_desc[id];
	}

/*
	dimitr: fixed bug SF #562417

	AFAIU, there was an assumption here that if a field descriptor is not
	filled, then a field doesn't exist. This is not true, because in fact
	an empty string has dsc_length = 0, and being part of an aggregate it
	becomes nod_field with zero length, hence we had NULL as a result.

	if (!format || id >= format->fmt_count || !desc->dsc_length)
*/
	if (!format || id >= format->fmt_count || !desc->dsc_dtype)
	{
		/* Map a non-existent field to a default value, if available.
		 * This enables automatic format upgrade for data rows.
		 * Handle Outer Joins and such specially!
		 * Reference: Bug 10424, 10116
		 */

		/* rec_format == NULL indicates we're performing a
		   join-to-null operation for outer joins */

		if (record && record->rec_format && relation)
		{
			/* A database sweep does not scan a relation's metadata. However
			 * the change to substitute a default value for a missing "not null"
			 * field makes it necessary to reference the field block.
			 */
			if (!relation->rel_fields)
			{
				thread_db* tdbb = NULL;

				SET_TDBB(tdbb);
				MET_scan_relation(tdbb, relation);
			}

			// CVC: With a corrupt db, the engine crashed doing backup.
			jrd_fld* temp_field = NULL;
			if (id < relation->rel_fields->count())
				temp_field = (*relation->rel_fields)[id];

			if (temp_field && temp_field->fld_default_value && temp_field->fld_not_null)
			{
				const nod_t temp_nod_type = temp_field->fld_default_value->nod_type;

				switch (temp_nod_type)
				{
				case nod_user_name:
					{
						desc->dsc_dtype = dtype_text;
						desc->dsc_sub_type = 0;
						desc->dsc_scale = 0;
						INTL_ASSIGN_TTYPE(desc, ttype_metadata);
						Firebird::MetaName& owner_name = relation->rel_owner_name;
						desc->dsc_address = (UCHAR*) owner_name.c_str(); // throwing away const.
						desc->dsc_length = owner_name.length();
					}
					break;
				case nod_current_role:
					{
						// CVC: Revisiting the current_role to fill default values:
						// If the current user is the same as the table creator,
						// return the current role for that user, otherwise return NONE.
						desc->dsc_dtype = dtype_text;
						desc->dsc_sub_type = 0;
						desc->dsc_scale = 0;
						INTL_ASSIGN_TTYPE(desc, ttype_metadata);
						thread_db* tdbb = NULL;
						SET_TDBB(tdbb);
						const char* rc_role = 0;
						const UserId* att_user = tdbb->getAttachment()->att_user;
						const char* cur_user = att_user ? att_user->usr_user_name.c_str() : 0;
						if (cur_user && relation->rel_owner_name == cur_user)
							rc_role = att_user->usr_sql_role_name.c_str();
						else
							rc_role = NULL_ROLE;

						desc->dsc_address = reinterpret_cast<UCHAR*>(const_cast<char*>(rc_role));
						desc->dsc_length = strlen(rc_role);
					}
					break;
				case nod_current_date:
				case nod_current_time:
				case nod_current_timestamp:
					{
						static const GDS_TIMESTAMP temp_timestamp = { 0, 0 };
						desc->dsc_dtype = dtype_timestamp;
						desc->dsc_scale = 0;
						desc->dsc_flags = 0;
						desc->dsc_address =
							reinterpret_cast<UCHAR*>(const_cast<ISC_TIMESTAMP*>(&temp_timestamp));
						desc->dsc_length = sizeof(temp_timestamp);
					}
					break;
				case nod_internal_info:
					{
						static const SLONG temp_long = 0;
						desc->dsc_dtype = dtype_long;
						desc->dsc_scale = 0;
						desc->dsc_flags = 0;
						desc->dsc_address = (UCHAR*) const_cast<SLONG*>(&temp_long);
						desc->dsc_length = sizeof(temp_long);
					}
					break;
				default:
					{
						const Literal* default_literal =
							reinterpret_cast<Literal*>(temp_field->fld_default_value);

						if (default_literal->nod_type == nod_null)
						{
							ERR_post(Arg::Gds(isc_not_valid) << Arg::Str(temp_field->fld_name) <<
																Arg::Str(NULL_STRING_MARK));
						}

						fb_assert(default_literal->nod_type == nod_literal);

						const dsc* default_desc = &default_literal->lit_desc;
						// CVC: This could be a bitwise copy in one line
						/*
						desc->dsc_dtype    = default_desc->dsc_dtype;
						desc->dsc_scale    = default_desc->dsc_scale;
						desc->dsc_length   = default_desc->dsc_length;
						desc->dsc_sub_type = default_desc->dsc_sub_type;
						desc->dsc_flags    = default_desc->dsc_flags;
						desc->dsc_address  = default_desc->dsc_address;
						*/
						*desc = *default_desc;
					}
				}

				return true;
			}
		}

		desc->dsc_dtype = dtype_text;
		desc->dsc_length = 1;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_ttype() = ttype_ascii;
		desc->dsc_address = (UCHAR *) " ";
		return false;
	}

/* If the offset of the field is 0, the field can't possible exist */

	if (!desc->dsc_address) {
		return false;
	}

	desc->dsc_address = record->rec_data + (IPTR) desc->dsc_address;

	if (TEST_NULL(record, id)) {
		desc->dsc_flags |= DSC_null;
		return false;
	}

	desc->dsc_flags &= ~DSC_null;
	return true;
}


USHORT EVL_group(thread_db* tdbb, RecordSource* rsb, jrd_nod *const node, USHORT state)
{
/**************************************
 *
 *      E V L _ g r o u p
 *
 **************************************
 *
 * Functional description
 *      Compute the next aggregated record of a value group.  EVL_group
 *      is driven by, and returns, a state variable.  The values of the
 *      state are:
 *
 *              3       Entering EVL group beforing fetching the first record.
 *              1       Values are pending from a prior fetch
 *              2       We encountered EOF from the last attempted fetch
 *              0       We processed everything now process (EOF)
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	impure_value vtemp;
	vtemp.vlu_string = NULL;

	jrd_req* request = tdbb->getRequest();
	jrd_nod* map = node->nod_arg[e_agg_map];
	jrd_nod* group = node->nod_arg[e_agg_group];

	jrd_nod** ptr;
	const jrd_nod* const* end;

	// if we found the last record last time, we're all done

	if (state == 2)
		return 0;

	try {

	// Initialize the aggregate record

	for (ptr = map->nod_arg, end = ptr + map->nod_count; ptr < end; ptr++) {
		const jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];
		impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);
		impure->vlux_count = 0;
		switch (from->nod_type)
		{
		case nod_agg_average:
		case nod_agg_average_distinct:
			impure->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
			impure->vlu_desc.dsc_length = sizeof(double);
			impure->vlu_desc.dsc_sub_type = 0;
			impure->vlu_desc.dsc_scale = 0;
			impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc.vlu_double;
			impure->vlu_misc.vlu_double = 0;
			if (from->nod_type == nod_agg_average_distinct)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_agg_average2:
		case nod_agg_average_distinct2:
			/* Initialize the result area as an int64.  If the field being
			   averaged is approximate numeric, the first call to add2 will
			   convert the descriptor to double. */
			impure->make_int64(0, from->nod_scale);
			if (from->nod_type == nod_agg_average_distinct2)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_agg_total:
		case nod_agg_total_distinct:
			impure->make_long(0);
			if (from->nod_type == nod_agg_total_distinct)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_agg_total2:
		case nod_agg_total_distinct2:
			/* Initialize the result area as an int64.  If the field being
			   averaged is approximate numeric, the first call to add2 will
			   convert the descriptor to double. */
			impure->make_int64(0, from->nod_scale);
			if (from->nod_type == nod_agg_total_distinct2)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_agg_min:
		case nod_agg_min_indexed:
		case nod_agg_max:
		case nod_agg_max_indexed:
			impure->vlu_desc.dsc_dtype = 0;
			break;

		case nod_agg_count:
		case nod_agg_count2:
		case nod_agg_count_distinct:
			impure->make_long(0);
			if (from->nod_type == nod_agg_count_distinct)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_agg_list:
		case nod_agg_list_distinct:
			// We don't know here what should be the sub-type and text-type.
			// Defer blob creation for when first record is found.
			impure->vlu_blob = NULL;
			impure->vlu_desc.dsc_dtype = 0;

			if (from->nod_type == nod_agg_list_distinct)
				/* Initialize a sort to reject duplicate values */
				init_agg_distinct(tdbb, from);
			break;

		case nod_literal:		/* pjpg 20001124 */
			EXE_assignment(tdbb, *ptr);
			break;

		default:    // Shut up some compiler warnings
			break;
		}
	}

/* If there isn't a record pending, open the stream and get one */

	if ((state == 0) || (state == 3))
	{
		RSE_open(tdbb, rsb);
		if (!RSE_get_record(tdbb, rsb, g_RSE_get_mode))
		{
			if (group) {
				fini_agg_distinct(tdbb, node);
				return 0;
			}
			state = 2;
		}
	}

	dsc* desc;

	if (group) {
		for (ptr = group->nod_arg, end = ptr + group->nod_count; ptr < end; ptr++)
		{
			jrd_nod* from = *ptr;
			impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);
			desc = EVL_expr(tdbb, from);
			if (request->req_flags & req_null)
				impure->vlu_desc.dsc_address = NULL;
			else
				EVL_make_value(tdbb, desc, impure);
		}
	}

	// Loop thru records until either a value change or EOF

	bool first = true;

	while (state != 2)
	{
		state = 1;

		if (first)
			first = false;
		else
		{
			// In the case of a group by, look for a change in value of any of
			// the columns; if we find one, stop aggregating and return what we have.

			if (group)
			{
				for (ptr = group->nod_arg, end = ptr + group->nod_count; ptr < end; ptr++)
				{
					jrd_nod* from = *ptr;
					impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);
					if (impure->vlu_desc.dsc_address)
						EVL_make_value(tdbb, &impure->vlu_desc, &vtemp);
					else
						vtemp.vlu_desc.dsc_address = NULL;

					desc = EVL_expr(tdbb, from);
					if (request->req_flags & req_null) {
						impure->vlu_desc.dsc_address = NULL;
						if (vtemp.vlu_desc.dsc_address)
							goto break_out;
					}
					else {
						EVL_make_value(tdbb, desc, impure);
						if (!vtemp.vlu_desc.dsc_address || MOV_compare(&vtemp.vlu_desc, desc))
						{
							goto break_out;
						}
					}
				}
			}
		}

		// go through and compute all the aggregates on this record

		for (ptr = map->nod_arg, end = ptr + map->nod_count; ptr < end; ptr++)
		{
			jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];
			impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);
			switch (from->nod_type)
			{
			case nod_agg_min:
			case nod_agg_min_indexed:
			case nod_agg_max:
			case nod_agg_max_indexed:
				{
					desc = EVL_expr(tdbb, from->nod_arg[0]);
					if (request->req_flags & req_null) {
						break;
					}

					// if a max or min has been mapped to an index,
					// then the first record is the EOF

					if (from->nod_type == nod_agg_max_indexed || from->nod_type == nod_agg_min_indexed)
					{
						state = 2;
					}

					++impure->vlux_count;
					if (!impure->vlu_desc.dsc_dtype)
					{
						EVL_make_value(tdbb, desc, impure);
						// It was reinterpret_cast<impure_value*>(&impure->vlu_desc));
						// but vlu_desc is the first member of impure_value and impure_value_ex
						// derives from impure_value and impure_value doesn't derive from anything
						// and it doesn't contain virtuals.
						// Thus, &impure_value_ex->vlu_desc == &impure_value->vlu_desc == &impure_value_ex
						// Delete this comment or restore the original code
						// when this reasoning has been validated, please.
						break;
					}
					const SLONG result = MOV_compare(desc, reinterpret_cast<dsc*>(impure));

					if ((result > 0 &&
							(from->nod_type == nod_agg_max || from->nod_type == nod_agg_max_indexed)) ||
						(result < 0 &&
							(from->nod_type == nod_agg_min || from->nod_type == nod_agg_min_indexed)))
					{
						EVL_make_value(tdbb, desc, impure);
					}

					break;
				}

			case nod_agg_total:
			case nod_agg_average:
				desc = EVL_expr(tdbb, from->nod_arg[0]);
				if (request->req_flags & req_null)
					break;
				++impure->vlux_count;
				add(desc, from, impure);
				break;

			case nod_agg_total2:
			case nod_agg_average2:
				desc = EVL_expr(tdbb, from->nod_arg[0]);
				if (request->req_flags & req_null)
					break;
				++impure->vlux_count;
				add2(desc, from, impure);
				break;

			case nod_agg_count2:
				++impure->vlux_count;
				desc = EVL_expr(tdbb, from->nod_arg[0]);
				if (request->req_flags & req_null)
					break;

			case nod_agg_count:
				++impure->vlux_count;
				++impure->vlu_misc.vlu_long;
				break;

			case nod_agg_list:
			{
				MoveBuffer buffer;
				UCHAR* temp;
				int len;

				desc = EVL_expr(tdbb, from->nod_arg[0]);
				if (request->req_flags & req_null)
					break;

				if (!impure->vlu_blob)
				{
					impure->vlu_blob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
						&impure->vlu_misc.vlu_bid);
					impure->vlu_desc.makeBlob(desc->getBlobSubType(), desc->getTextType(),
						(ISC_QUAD* ) &impure->vlu_misc.vlu_bid);
				}

				if (impure->vlux_count) {
					const dsc* const delimiter = EVL_expr(tdbb, from->nod_arg[1]);
					if (request->req_flags & req_null) {
						// mark the result as NULL
						impure->vlu_desc.dsc_dtype = 0;
						break;
					}
					len = MOV_make_string2(tdbb, delimiter, impure->vlu_desc.getTextType(), &temp, buffer, false);
					BLB_put_data(tdbb, impure->vlu_blob, temp, len);
				}
				++impure->vlux_count;
				len = MOV_make_string2(tdbb, desc, impure->vlu_desc.getTextType(), &temp, buffer, false);
				BLB_put_data(tdbb, impure->vlu_blob, temp, len);
				break;
			}

			case nod_agg_count_distinct:
			case nod_agg_total_distinct:
			case nod_agg_average_distinct:
			case nod_agg_average_distinct2:
			case nod_agg_total_distinct2:
			case nod_agg_list_distinct:
				{
					desc = EVL_expr(tdbb, from->nod_arg[0]);
					if (request->req_flags & req_null)
						break;
					/* "Put" the value to sort. */
					const size_t asb_index = (from->nod_type == nod_agg_list_distinct) ? 2 : 1;
					AggregateSort* asb = (AggregateSort*) from->nod_arg[asb_index];
					impure_agg_sort* asb_impure =
						(impure_agg_sort*) ((SCHAR *) request + asb->nod_impure);
					UCHAR* data;
					SORT_put(tdbb, asb_impure->iasb_sort_handle, reinterpret_cast<ULONG**>(&data));

					MOVE_CLEAR(data, asb->asb_length);

					if (asb->asb_intl)
					{
						// convert to an international byte array
						dsc to;
						to.dsc_dtype = dtype_text;
						to.dsc_flags = 0;
						to.dsc_sub_type = 0;
						to.dsc_scale = 0;
						to.dsc_ttype() = ttype_sort_key;
						to.dsc_length = asb->asb_key_desc[0].skd_length;
						to.dsc_address = data;
						INTL_string_to_key(tdbb, INTL_TEXT_TO_INDEX(desc->getTextType()),
							desc, &to, INTL_KEY_UNIQUE);
					}

					dsc toDesc = asb->asb_desc;
					toDesc.dsc_address = data +
						(asb->asb_intl ? asb->asb_key_desc[1].skd_offset : 0);
					MOV_move(tdbb, desc, &toDesc);

					break;
				}

			default:
				EXE_assignment(tdbb, *ptr);
			}
		}

		if (state == 2)
			break;

		if (!RSE_get_record(tdbb, rsb, g_RSE_get_mode))
		{
			state = 2;
		}
	}

  break_out:

	for (ptr = map->nod_arg, end = ptr + map->nod_count; ptr < end; ptr++)
	{
		const jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];
		impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);

		if (from->nod_type == nod_agg_list && impure->vlu_blob)
		{
			BLB_close(tdbb, impure->vlu_blob);
			impure->vlu_blob = NULL;
		}
	}

/* Finish up any residual computations and get out */

	delete vtemp.vlu_string;

	dsc temp;
	double d;
	SINT64 i;

	for (ptr = map->nod_arg, end = ptr + map->nod_count; ptr < end; ptr++)
	{
		jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];
		jrd_nod* field = (*ptr)->nod_arg[e_asgn_to];
		const USHORT id = (USHORT)(IPTR) field->nod_arg[e_fld_id];
		Record* record = request->req_rpb[(int) (IPTR) field->nod_arg[e_fld_stream]].rpb_record;
		impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + from->nod_impure);

		switch (from->nod_type)
		{
		case nod_agg_min:
		case nod_agg_min_indexed:
		case nod_agg_max:
		case nod_agg_max_indexed:
		case nod_agg_total:
		case nod_agg_total_distinct:
		case nod_agg_total2:
		case nod_agg_total_distinct2:
		case nod_agg_list:
		case nod_agg_list_distinct:
			if ((from->nod_type == nod_agg_total_distinct) ||
				(from->nod_type == nod_agg_total_distinct2) ||
				(from->nod_type == nod_agg_list_distinct))
			{
				compute_agg_distinct(tdbb, from);
			}
			if (!impure->vlux_count)
			{
				SET_NULL(record, id);
				break;
			}
			/* If vlux_count is non-zero, we need to fall through. */
		case nod_agg_count:
		case nod_agg_count2:
		case nod_agg_count_distinct:
			if (from->nod_type == nod_agg_count_distinct) {
				compute_agg_distinct(tdbb, from);
			}
			if (!impure->vlu_desc.dsc_dtype) {
				SET_NULL(record, id);
			}
			else {
				MOV_move(tdbb, &impure->vlu_desc, EVL_assign_to(tdbb, field));
				CLEAR_NULL(record, id);
			}
			break;

		case nod_agg_average_distinct:
			compute_agg_distinct(tdbb, from);
			/* fall through */
		case nod_agg_average:
			if (!impure->vlux_count) {
				SET_NULL(record, id);
				break;
			}
			CLEAR_NULL(record, id);
			temp.dsc_dtype = DEFAULT_DOUBLE;
			temp.dsc_length = sizeof(double);
			temp.dsc_scale = 0;
			temp.dsc_sub_type = 0;
			temp.dsc_address = (UCHAR *) & d;
			d = MOV_get_double(&impure->vlu_desc) / impure->vlux_count;
			MOV_move(tdbb, &temp, EVL_assign_to(tdbb, field));
			break;

		case nod_agg_average_distinct2:
			compute_agg_distinct(tdbb, from);
			/* fall through */
		case nod_agg_average2:
			if (!impure->vlux_count) {
				SET_NULL(record, id);
				break;
			}
			CLEAR_NULL(record, id);
			temp.dsc_sub_type = 0;
			if (dtype_int64 == impure->vlu_desc.dsc_dtype) {
				temp.dsc_dtype = dtype_int64;
				temp.dsc_length = sizeof(SINT64);
				temp.dsc_scale = impure->vlu_desc.dsc_scale;
				temp.dsc_address = (UCHAR *) & i;
				i = *((SINT64 *) impure->vlu_desc.dsc_address) / impure->vlux_count;
			}
			else {
				temp.dsc_dtype = DEFAULT_DOUBLE;
				temp.dsc_length = sizeof(double);
				temp.dsc_scale = 0;
				temp.dsc_address = (UCHAR *) & d;
				d = MOV_get_double(&impure->vlu_desc) / impure->vlux_count;
			}
			MOV_move(tdbb, &temp, EVL_assign_to(tdbb, field));
			break;

		default:	// Shut up some compiler warnings
			break;
		}
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		fini_agg_distinct(tdbb, node);
		ERR_punt();
	}

	return state;
}


void EVL_make_value(thread_db* tdbb, const dsc* desc, impure_value* value)
{
/**************************************
 *
 *      E V L _ m a k e _ v a l u e
 *
 **************************************
 *
 * Functional description
 *      Make a value block reflect the value of a descriptor.
 *
 **************************************/
	SET_TDBB(tdbb);

/* Handle the fixed length data types first.  They're easy. */

	const dsc from = *desc;
	value->vlu_desc = *desc;
	value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc;

	switch (from.dsc_dtype)
	{
	case dtype_short:
		value->vlu_misc.vlu_short = *((SSHORT *) from.dsc_address);
		return;

	case dtype_long:
	case dtype_real:
	case dtype_sql_time:
	case dtype_sql_date:
		value->vlu_misc.vlu_long = *((SLONG *) from.dsc_address);
		return;

	case dtype_int64:
		value->vlu_misc.vlu_int64 = *((SINT64 *) from.dsc_address);
		return;

	case dtype_double:
		value->vlu_misc.vlu_double = *((double *) from.dsc_address);
		return;

	case dtype_timestamp:
	case dtype_quad:
		value->vlu_misc.vlu_dbkey[0] = ((SLONG *) from.dsc_address)[0];
		value->vlu_misc.vlu_dbkey[1] = ((SLONG *) from.dsc_address)[1];
		return;

	case dtype_text:
	case dtype_varying:
	case dtype_cstring:
	case dtype_dbkey:
		break;

	case dtype_blob:
		value->vlu_misc.vlu_bid = *(bid*)from.dsc_address;
		return;

	default:
		fb_assert(false);
		break;
	}

	VaryStr<128> temp;
	UCHAR* address;
	USHORT ttype;

	// Get string.  If necessary, get_string will convert the string into a
	// temporary buffer.  Since this will always be the result of a conversion,
	// this isn't a serious problem.

	const USHORT length = MOV_get_string_ptr(&from, &ttype, &address, &temp, sizeof(temp));

	// Allocate a string block of sufficient size.
	VaryingString* string = value->vlu_string;
	if (string && string->str_length < length)
	{
		delete string;
		string = NULL;
	}
	if (!string)
	{
		string = value->vlu_string = FB_NEW_RPT(*tdbb->getDefaultPool(), length) VaryingString();
		string->str_length = length;
	}

	value->vlu_desc.dsc_length = length;
	UCHAR* target = string->str_data;
	value->vlu_desc.dsc_address = target;
	value->vlu_desc.dsc_sub_type = 0;
	value->vlu_desc.dsc_scale = 0;
	if (from.dsc_dtype == dtype_dbkey)
	{
		value->vlu_desc.dsc_dtype = dtype_dbkey;
	}
	else
	{
		value->vlu_desc.dsc_dtype = dtype_text;
		INTL_ASSIGN_TTYPE(&value->vlu_desc, ttype);
	}

	if (address && length && target != address)
		memcpy(target, address, length);
}


void EVL_validate(thread_db* tdbb, const Item& item, const ItemInfo* itemInfo, dsc* desc, bool null)
{
/**************************************
 *
 *	E V L _ v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Validate argument/variable for not null and check constraint
 *
 **************************************/
	if (itemInfo == NULL)
		return;

	jrd_req* request = tdbb->getRequest();
	bool err = false;

	if (null && !itemInfo->nullable)
		err = true;

	const char* value = NULL_STRING_MARK;
	VaryStr<128> temp;

	MapFieldInfo::ValueType fieldInfo;
	if (!err && itemInfo->fullDomain &&
		request->req_map_field_info.get(itemInfo->field, fieldInfo) && fieldInfo.validation)
	{
		if (desc && null)
		{
			desc->dsc_flags |= DSC_null;
		}

		const bool desc_is_null = !desc || (desc->dsc_flags & DSC_null);

		request->req_domain_validation = desc;
		const USHORT flags = request->req_flags;

		if (!EVL_boolean(tdbb, fieldInfo.validation) && !(request->req_flags & req_null))
		{
			const USHORT length = desc_is_null ? 0 :
				MOV_make_string(desc, ttype_dynamic, &value, &temp, sizeof(temp) - 1);

			if (desc_is_null)
				value = NULL_STRING_MARK;
			else if (!length)
				value = "";
			else
				const_cast<char*>(value)[length] = 0;	// safe cast - data is on our local stack

			err = true;
		}

		request->req_flags = flags;
	}

	Firebird::string s;

	if (err)
	{
		ISC_STATUS status = isc_not_valid_for_var;
		const char* arg;

		if (item.type == nod_cast)
		{
			status = isc_not_valid_for;
			arg = "CAST";
		}
		else
		{
			if (itemInfo->name.isEmpty())
			{
				int index = item.index + 1;

				status = isc_not_valid_for;

				if (item.type == nod_variable)
				{
					if (request->req_procedure)
					{
						if (index <= request->req_procedure->prc_outputs)
							s.printf("output parameter number %d", index);
						else
						{
							s.printf("variable number %d", index - request->req_procedure->prc_outputs);
						}
					}
					else
						s.printf("variable number %d", index);
				}
				else if (item.type == nod_argument && item.subType == 0)
					s.printf("input parameter number %d", (index - 1) / 2 + 1);
				else if (item.type == nod_argument && item.subType == 1)
					s.printf("output parameter number %d", index);

				if (s.isEmpty())
					arg = UNKNOWN_STRING_MARK;
				else
					arg = s.c_str();
			}
			else
				arg = itemInfo->name.c_str();
		}

		ERR_post(Arg::Gds(status) << Arg::Str(arg) << Arg::Str(value));
	}
}


static dsc* add(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d
 *
 **************************************
 *
 * Functional description
 *      Add (or subtract) the contents of a descriptor to value
 *      block, with dialect-1 semantics.
 *      This function can be removed when dialect-3 becomes
 *      the lowest supported dialect.  (Version 7.0?)
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);
	fb_assert(node->nod_type == nod_add ||
			  node->nod_type == nod_subtract ||
			  node->nod_type == nod_total ||
			  node->nod_type == nod_average ||
			  node->nod_type == nod_agg_total ||
			  node->nod_type == nod_agg_average ||
			  node->nod_type == nod_agg_total_distinct ||
			  node->nod_type == nod_agg_average_distinct);

	dsc* const result = &value->vlu_desc;

/* Handle date arithmetic */

	if (node->nod_flags & nod_date) {
		return add_datetime(desc, node, value);
	}

/* Handle floating arithmetic */

	if (node->nod_flags & nod_double) {
		const double d1 = MOV_get_double(desc);
		const double d2 = MOV_get_double(&value->vlu_desc);
		value->vlu_misc.vlu_double = (node->nod_type == nod_subtract) ? d2 - d1 : d1 + d2;
		if (isinf(value->vlu_misc.vlu_double))
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_overflow));
		}
		result->dsc_dtype = DEFAULT_DOUBLE;
		result->dsc_length = sizeof(double);
		result->dsc_scale = 0;
		result->dsc_sub_type = 0;
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return result;
	}

/* Handle (oh, ugh) quad arithmetic */

	if (node->nod_flags & nod_quad) {
		const SQUAD q1 = MOV_get_quad(desc, node->nod_scale);
		const SQUAD q2 = MOV_get_quad(&value->vlu_desc, node->nod_scale);
		result->dsc_dtype = dtype_quad;
		result->dsc_length = sizeof(SQUAD);
		result->dsc_scale = node->nod_scale;
		value->vlu_misc.vlu_quad = (node->nod_type == nod_subtract) ?
			QUAD_SUBTRACT(q2, q1, ERR_post) :
			QUAD_ADD(q1, q2, ERR_post);
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_quad;

		return result;
	}

/* Everything else defaults to longword */

	// CVC: Maybe we should upgrade the sum to double if it doesn't fit?
	// This is what was done for multiplicaton in dialect 1.
	const SLONG l1 = MOV_get_long(desc, node->nod_scale);
	const SINT64 l2 = MOV_get_long(&value->vlu_desc, node->nod_scale);
	SINT64 rc = (node->nod_type == nod_subtract) ? l2 - l1 : l2 + l1;
	if (rc < MIN_SLONG || rc > MAX_SLONG)
		ERR_post(Arg::Gds(isc_exception_integer_overflow));
	value->make_long(rc, node->nod_scale);
	return result;
}


static dsc* add2(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d 2
 *
 **************************************
 *
 * Functional description
 *      Add (or subtract) the contents of a descriptor to value
 *      block, with dialect-3 semantics, as in the blr_add,
 *      blr_subtract, and blr_agg_total verbs following a
 *      blr_version5
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);
	fb_assert(node->nod_type == nod_add2 ||
			  node->nod_type == nod_subtract2 ||
			  node->nod_type == nod_average2 ||
			  node->nod_type == nod_agg_total2 ||
			  node->nod_type == nod_agg_average2 ||
			  node->nod_type == nod_agg_total_distinct2 ||
			  node->nod_type == nod_agg_average_distinct2);

	dsc* result = &value->vlu_desc;

/* Handle date arithmetic */

	if (node->nod_flags & nod_date) {
		return add_datetime(desc, node, value);
	}

/* Handle floating arithmetic */

	if (node->nod_flags & nod_double) {
		const double d1 = MOV_get_double(desc);
		const double d2 = MOV_get_double(&value->vlu_desc);
		value->vlu_misc.vlu_double = (node->nod_type == nod_subtract2) ? d2 - d1 : d1 + d2;
		if (isinf(value->vlu_misc.vlu_double))
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_overflow));
		}
		result->dsc_dtype = DEFAULT_DOUBLE;
		result->dsc_length = sizeof(double);
		result->dsc_scale = 0;
		result->dsc_sub_type = 0;
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return result;
	}

/* Handle (oh, ugh) quad arithmetic */

	if (node->nod_flags & nod_quad) {
		const SQUAD q1 = MOV_get_quad(desc, node->nod_scale);
		const SQUAD q2 = MOV_get_quad(&value->vlu_desc, node->nod_scale);
		result->dsc_dtype = dtype_quad;
		result->dsc_length = sizeof(SQUAD);
		result->dsc_scale = node->nod_scale;
		value->vlu_misc.vlu_quad = (node->nod_type == nod_subtract2) ?
			QUAD_SUBTRACT(q2, q1, ERR_post) :
			QUAD_ADD(q1, q2, ERR_post);
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_quad;

		return result;
	}

/* Everything else defaults to int64 */

	SINT64 i1 = MOV_get_int64(desc, node->nod_scale);
	const SINT64 i2 = MOV_get_int64(&value->vlu_desc, node->nod_scale);
	result->dsc_dtype = dtype_int64;
	result->dsc_length = sizeof(SINT64);
	result->dsc_scale = node->nod_scale;
	value->vlu_misc.vlu_int64 = (node->nod_type == nod_subtract2) ? i2 - i1 : i1 + i2;
	result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_int64;

	result->dsc_sub_type = MAX(desc->dsc_sub_type, value->vlu_desc.dsc_sub_type);

/* If the operands of an addition have the same sign, and their sum has
   the opposite sign, then overflow occurred.  If the two addends have
   opposite signs, then the result will lie between the two addends, and
   overflow cannot occur.
   If this is a subtraction, note that we invert the sign bit, rather than
   negating the argument, so that subtraction of MIN_SINT64, which is
   unchanged by negation, will be correctly treated like the addition of
   a positive number for the purposes of this test.

   Test cases for a Gedankenexperiment, considering the sign bits of the
   operands and result after the inversion below:                L  Rt  Sum

		MIN_SINT64 - MIN_SINT64 ==          0, with no overflow  1   0   0
	   -MAX_SINT64 - MIN_SINT64 ==          1, with no overflow  1   0   0
		1          - MIN_SINT64 == overflow                      0   0   1
	   -1          - MIN_SINT64 == MAX_SINT64, no overflow       1   0   0
*/

	if (node->nod_type == nod_subtract2)
		i1 ^= MIN_SINT64;		/* invert the sign bit */
	if (((i1 ^ i2) >= 0) && ((i1 ^ value->vlu_misc.vlu_int64) < 0))
		ERR_post(Arg::Gds(isc_exception_integer_overflow));

	return result;
}


static dsc* add_datetime(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d _ d a t e t i m e
 *
 **************************************
 *
 * Functional description
 *	Vector out to one of the actual datetime addition routines
 *
 **************************************/
	BYTE dtype;					/* Which addition routine to use? */

	fb_assert(node->nod_flags & nod_date);

/* Value is the LHS of the operand.  desc is the RHS */

	if ((node->nod_type == nod_add) || (node->nod_type == nod_add2)) {
		dtype = DSC_add_result[value->vlu_desc.dsc_dtype][desc->dsc_dtype];
	}
	else {
		fb_assert((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2));
		dtype = DSC_sub_result[value->vlu_desc.dsc_dtype][desc->dsc_dtype];

		/* Is this a <date type> - <date type> construct?
		   chose the proper routine to do the subtract from the
		   LHS of expression
		   Thus:   <TIME> - <TIMESTAMP> uses TIME arithmetic
		   <DATE> - <TIMESTAMP> uses DATE arithmetic
		   <TIMESTAMP> - <DATE> uses TIMESTAMP arithmetic */
		if (DTYPE_IS_NUMERIC(dtype))
			dtype = value->vlu_desc.dsc_dtype;

		/* Handle historical <timestamp> = <string> - <value> case */
		if (!DTYPE_IS_DATE(dtype) &&
			(DTYPE_IS_TEXT(value->vlu_desc.dsc_dtype) || DTYPE_IS_TEXT(desc->dsc_dtype)))
		{
			dtype = dtype_timestamp;
		}
	}

	switch (dtype)
	{
	case dtype_timestamp:
	default:
		/* This needs to handle a dtype_sql_date + dtype_sql_time */
		/* For historical reasons prior to V6 - handle any types for
		   timestamp arithmetic */
		return add_timestamp(desc, node, value);

	case dtype_sql_time:
		return add_sql_time(desc, node, value);

	case dtype_sql_date:
		return add_sql_date(desc, node, value);

	case DTYPE_CANNOT:
		ERR_post(Arg::Gds(isc_expression_eval_err) <<
					Arg::Gds(isc_invalid_type_datetime_op));
		return NULL;
	}
}


static dsc* add_sql_date(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d _ s q l _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Perform date arithmetic
 *
 *	DATE -   DATE	   Result is SLONG
 *	DATE +/- NUMERIC   Numeric is interpreted as days DECIMAL(*,0).
 *	NUMERIC +/- TIME   Numeric is interpreted as days DECIMAL(*,0).
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);
	fb_assert(node->nod_type == nod_add || node->nod_type == nod_subtract ||
		node->nod_type == nod_add2 || node->nod_type == nod_subtract2);

	dsc* result = &value->vlu_desc;

	fb_assert(value->vlu_desc.dsc_dtype == dtype_sql_date || desc->dsc_dtype == dtype_sql_date);

	SINT64 d1;
/* Coerce operand1 to a count of days */
	bool op1_is_date = false;
	if (value->vlu_desc.dsc_dtype == dtype_sql_date) {
		d1 = *((GDS_DATE *) value->vlu_desc.dsc_address);
		op1_is_date = true;
	}
	else
		d1 = MOV_get_int64(&value->vlu_desc, 0);

	SINT64 d2;
/* Coerce operand2 to a count of days */
	bool op2_is_date = false;
	if (desc->dsc_dtype == dtype_sql_date) {
		d2 = *((GDS_DATE *) desc->dsc_address);
		op2_is_date = true;
	}
	else
		d2 = MOV_get_int64(desc, 0);

	if (((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) &&
		op1_is_date && op2_is_date)
	{
		d2 = d1 - d2;
		value->make_int64(d2);
		return result;
	}

	fb_assert(op1_is_date || op2_is_date);
	fb_assert(!(op1_is_date && op2_is_date));

/* Perform the operation */

	if ((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) {
		fb_assert(op1_is_date);
		d2 = d1 - d2;
	}
	else
		d2 = d1 + d2;

	value->vlu_misc.vlu_sql_date = d2;

	if (!Firebird::TimeStamp::isValidDate(value->vlu_misc.vlu_sql_date)) {
		ERR_post(Arg::Gds(isc_date_range_exceeded));
	}

	result->dsc_dtype = dtype_sql_date;
	result->dsc_length = type_lengths[result->dsc_dtype];
	result->dsc_scale = 0;
	result->dsc_sub_type = 0;
	result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_sql_date;
	return result;

}


static dsc* add_sql_time(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d _ s q l _ t i m e
 *
 **************************************
 *
 * Functional description
 *	Perform time arithmetic
 *
 *	TIME -   TIME	   Result is SLONG, scale -4
 *	TIME +/- NUMERIC   Numeric is interpreted as seconds DECIMAL(*,4).
 *	NUMERIC +/- TIME   Numeric is interpreted as seconds DECIMAL(*,4).
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);
	fb_assert(node->nod_type == nod_add || node->nod_type == nod_subtract ||
		node->nod_type == nod_add2 || node->nod_type == nod_subtract2);

	dsc* result = &value->vlu_desc;

	fb_assert(value->vlu_desc.dsc_dtype == dtype_sql_time || desc->dsc_dtype == dtype_sql_time);

	SINT64 d1;
/* Coerce operand1 to a count of seconds */
	bool op1_is_time = false;
	if (value->vlu_desc.dsc_dtype == dtype_sql_time) {
		d1 = *(GDS_TIME *) value->vlu_desc.dsc_address;
		op1_is_time = true;
		fb_assert(d1 >= 0 && d1 < ISC_TICKS_PER_DAY);
	}
	else
		d1 = MOV_get_int64(&value->vlu_desc, ISC_TIME_SECONDS_PRECISION_SCALE);

	SINT64 d2;
/* Coerce operand2 to a count of seconds */
	bool op2_is_time = false;
	if (desc->dsc_dtype == dtype_sql_time) {
		d2 = *(GDS_TIME *) desc->dsc_address;
		op2_is_time = true;
		fb_assert(d2 >= 0 && d2 < ISC_TICKS_PER_DAY);
	}
	else
		d2 = MOV_get_int64(desc, ISC_TIME_SECONDS_PRECISION_SCALE);

	if (((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) &&
		op1_is_time && op2_is_time)
	{
		d2 = d1 - d2;
		/* Overflow cannot occur as the range of supported TIME values
		   is less than the range of INTEGER */
		value->vlu_misc.vlu_long = d2;
		result->dsc_dtype = dtype_long;
		result->dsc_length = sizeof(SLONG);
		result->dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_long;
		return result;
	}

	fb_assert(op1_is_time || op2_is_time);
	fb_assert(!(op1_is_time && op2_is_time));

/* Perform the operation */

	if ((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) {
		fb_assert(op1_is_time);
		d2 = d1 - d2;
	}
	else
		d2 = d1 + d2;

/* Make sure to use modulo 24 hour arithmetic */

/* Make the result positive */
	while (d2 < 0)
		d2 += (ISC_TICKS_PER_DAY);

/* And make it in the range of values for a day */
	d2 %= (ISC_TICKS_PER_DAY);

	fb_assert(d2 >= 0 && d2 < ISC_TICKS_PER_DAY);

	value->vlu_misc.vlu_sql_time = d2;

	result->dsc_dtype = dtype_sql_time;
	result->dsc_length = type_lengths[result->dsc_dtype];
	result->dsc_scale = 0;
	result->dsc_sub_type = 0;
	result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_sql_time;
	return result;
}


static dsc* add_timestamp(const dsc* desc, const jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *      a d d _ t i m e s t a m p
 *
 **************************************
 *
 * Functional description
 *	Perform date&time arithmetic
 *
 *	TIMESTAMP - TIMESTAMP	Result is INT64
 *	TIMESTAMP +/- NUMERIC   Numeric is interpreted as days DECIMAL(*,*).
 *	NUMERIC +/- TIMESTAMP   Numeric is interpreted as days DECIMAL(*,*).
 *	DATE	+   TIME
 *	TIME	+   DATE
 *
 **************************************/
	SINT64 d1, d2;

	DEV_BLKCHK(node, type_nod);
	fb_assert(node->nod_type == nod_add || node->nod_type == nod_subtract ||
		node->nod_type == nod_add2 || node->nod_type == nod_subtract2);

	dsc* result = &value->vlu_desc;

	// Operand 1 is Value -- Operand 2 is desc

	if (value->vlu_desc.dsc_dtype == dtype_sql_date) {
		// DATE + TIME
		if ((desc->dsc_dtype == dtype_sql_time) &&
			((node->nod_type == nod_add) || (node->nod_type == nod_add2)))
		{
			value->vlu_misc.vlu_timestamp.timestamp_date = value->vlu_misc.vlu_sql_date;
			value->vlu_misc.vlu_timestamp.timestamp_time = *(GDS_TIME *) desc->dsc_address;
			goto return_result;
		}
		ERR_post(Arg::Gds(isc_expression_eval_err) <<
					Arg::Gds(isc_onlycan_add_timetodate));
	}
	else if (desc->dsc_dtype == dtype_sql_date) {
		// TIME + DATE
		if ((value->vlu_desc.dsc_dtype == dtype_sql_time) &&
			((node->nod_type == nod_add) || (node->nod_type == nod_add2)))
		{
			value->vlu_misc.vlu_timestamp.timestamp_time = value->vlu_misc.vlu_sql_time;
			value->vlu_misc.vlu_timestamp.timestamp_date = *(GDS_DATE *) desc->dsc_address;
			goto return_result;
		}
		ERR_post(Arg::Gds(isc_expression_eval_err) <<
					Arg::Gds(isc_onlycan_add_datetotime));
	}

/* For historical reasons (behavior prior to V6),
   there are times we will do timestamp arithmetic without a
   timestamp being involved.
   In such an event we need to convert a text type to a timestamp when
   we don't already have one.
   We assume any text string must represent a timestamp value.  */

/* If we're subtracting, and the 2nd operand is a timestamp, or
   something that looks & smells like it could be a timestamp, then
   we must be doing <timestamp> - <timestamp> subtraction.
   Notes that this COULD be as strange as <string> - <string>, but
   because nod_date is set in the nod_flags we know we're supposed
   to use some form of date arithmetic */

	if (((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) &&
		((desc->dsc_dtype == dtype_timestamp) || DTYPE_IS_TEXT(desc->dsc_dtype)))
	{

		/* Handle cases of
		   <string>    - <string>
		   <string>    - <timestamp>
		   <timestamp> - <string>
		   <timestamp> - <timestamp>
		   in which cases we assume the string represents a timestamp value */

		/* If the first operand couldn't represent a timestamp, bomb out */

		if (!((value->vlu_desc.dsc_dtype == dtype_timestamp) ||
			DTYPE_IS_TEXT(value->vlu_desc.dsc_dtype)))
		{
			ERR_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_onlycansub_tstampfromtstamp));
		}

		d1 = get_timestamp_to_isc_ticks(&value->vlu_desc);
		d2 = get_timestamp_to_isc_ticks(desc);

		d2 = d1 - d2;

		if (node->nod_type == nod_subtract2) {

			/* multiply by 100,000 so that we can have the result as decimal (18,9)
			 * We have 10 ^-4; to convert this to 10^-9 we need to multiply by
			 * 100,000. Of course all this is true only because we are dividing
			 * by SECONDS_PER_DAY
			 * now divide by the number of seconds per day, this will give us the
			 * result as a int64 of type decimal (18, 9) in days instead of
			 * seconds.
			 *
			 * But SECONDS_PER_DAY has 2 trailing zeroes (because it is 24 * 60 *
			 * 60), so instead of calculating (X * 100000) / SECONDS_PER_DAY,
			 * use (X * (100000 / 100)) / (SECONDS_PER_DAY / 100), which can be
			 * simplified to (X * 1000) / (SECONDS_PER_DAY / 100)
			 * Since the largest possible difference in timestamps is about 3E11
			 * seconds or 3E15 isc_ticks, the product won't exceed approximately
			 * 3E18, which fits into an INT64.
			 */
			// 09-Apr-2004, Nickolay Samofatov. Adjust number before division to
			// make sure we don't lose a tick as a result of remainder truncation
			if (d2 >= 0) {
				d2 = (d2 * 1000 + (SECONDS_PER_DAY / 200)) / (SINT64) (SECONDS_PER_DAY / 100);
			}
			else {
				d2 = (d2 * 1000 - (SECONDS_PER_DAY / 200)) / (SINT64) (SECONDS_PER_DAY / 100);
			}
			value->vlu_misc.vlu_int64 = d2;
			result->dsc_dtype = dtype_int64;
			result->dsc_length = sizeof(SINT64);
			result->dsc_scale = DIALECT_3_TIMESTAMP_SCALE;
			result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_int64;
			return result;
		}

		/* This is dialect 1 subtraction returning double as before */
		value->vlu_misc.vlu_double = (double) d2 / ((double) ISC_TICKS_PER_DAY);
		result->dsc_dtype = dtype_double;
		result->dsc_length = sizeof(double);
		result->dsc_scale = DIALECT_1_TIMESTAMP_SCALE;
		result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return result;
	}

/* From here we know our result must be a <timestamp>.  The only
   legal cases are:
	<timestamp> +/-  <numeric>
	<numeric>   +    <timestamp>
   However, the nod_date flag might have been set on any type of
   nod_add / nod_subtract equation -- so we must detect any invalid
   operands.   Any <string> value is assumed to be convertable to
   a timestamp */


	{ // This block solves the goto v/s var initialization error

		// Coerce operand1 to a count of microseconds

		bool op1_is_timestamp = false;
		if ((value->vlu_desc.dsc_dtype == dtype_timestamp) ||
			(DTYPE_IS_TEXT(value->vlu_desc.dsc_dtype)))
		{
			op1_is_timestamp = true;
		}

		// Coerce operand2 to a count of microseconds

		bool op2_is_timestamp = false;
		if ((desc->dsc_dtype == dtype_timestamp) || (DTYPE_IS_TEXT(desc->dsc_dtype)))
		{
			op2_is_timestamp = true;
		}

		// Exactly one of the operands must be a timestamp or
		// convertable into a timestamp, otherwise it's one of
		//    <numeric>   +/- <numeric>
		// or <timestamp> +/- <timestamp>
		// or <string>    +/- <string>
		// which are errors

		if (op1_is_timestamp == op2_is_timestamp)
		{
			ERR_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_onlyoneop_mustbe_tstamp));
		}

		if (op1_is_timestamp) {
			d1 = get_timestamp_to_isc_ticks(&value->vlu_desc);
			d2 = get_day_fraction(desc);
		}
		else {
			fb_assert((node->nod_type == nod_add) || (node->nod_type == nod_add2));
			fb_assert(op2_is_timestamp);
			d1 = get_day_fraction(&value->vlu_desc);
			d2 = get_timestamp_to_isc_ticks(desc);
		}

		// Perform the operation

		if ((node->nod_type == nod_subtract) || (node->nod_type == nod_subtract2)) {
			fb_assert(op1_is_timestamp);
			d2 = d1 - d2;
		}
		else
			d2 = d1 + d2;

		// Convert the count of microseconds back to a date / time format

		value->vlu_misc.vlu_timestamp.timestamp_date = d2 / (ISC_TICKS_PER_DAY);
		value->vlu_misc.vlu_timestamp.timestamp_time = (d2 % ISC_TICKS_PER_DAY);

		// Make sure the TIME portion is non-negative

		if ((SLONG) value->vlu_misc.vlu_timestamp.timestamp_time < 0) {
			value->vlu_misc.vlu_timestamp.timestamp_time =
				((SLONG) value->vlu_misc.vlu_timestamp.timestamp_time) + ISC_TICKS_PER_DAY;
			value->vlu_misc.vlu_timestamp.timestamp_date -= 1;
		}

		if (!Firebird::TimeStamp::isValidTimeStamp(value->vlu_misc.vlu_timestamp)) {
			ERR_post(Arg::Gds(isc_datetime_range_exceeded));
		}

	} // scope block for goto v/s var initialization error

return_result:
	// Caution: target of GOTO

	fb_assert(value->vlu_misc.vlu_timestamp.timestamp_time >= 0 &&
		value->vlu_misc.vlu_timestamp.timestamp_time < ISC_TICKS_PER_DAY);

	result->dsc_dtype = dtype_timestamp;
	result->dsc_length = type_lengths[result->dsc_dtype];
	result->dsc_scale = 0;
	result->dsc_sub_type = 0;
	result->dsc_address = (UCHAR *) & value->vlu_misc.vlu_timestamp;
	return result;
}


void EVL_adjust_text_descriptor(thread_db* tdbb, dsc* desc)
{
/**************************************
 *
 * E V L _ a d j u s t _ t e x t _ d e s c r i p t o r
 *
 **************************************
 *
 * Functional description
 *      This function receives a text descriptor with
 *      dsc_length = numberOfCharacters * maxBytesPerChar
 *      and change dsc_length to number of bytes used by the string.
 *
 **************************************/
	if (desc->dsc_dtype == dtype_text)
	{
		SET_TDBB(tdbb);

		USHORT ttype = INTL_TTYPE(desc);

		CharSet* charSet = INTL_charset_lookup(tdbb, ttype);

		if (charSet->isMultiByte())
		{
			Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

			if (charSet->getFlags() & CHARSET_LEGACY_SEMANTICS)
			{
				desc->dsc_length = charSet->substring(TEXT_LEN(desc), desc->dsc_address, TEXT_LEN(desc),
										buffer.getBuffer(TEXT_LEN(desc) * charSet->maxBytesPerChar()), 0,
										TEXT_LEN(desc));

				const ULONG maxLength = TEXT_LEN(desc) / charSet->maxBytesPerChar();
				ULONG charLength = charSet->length(desc->dsc_length, desc->dsc_address, true);

				while (charLength > maxLength)
				{
					if (desc->dsc_address[desc->dsc_length - 1] == *charSet->getSpace())
					{
						--desc->dsc_length;
						--charLength;
					}
					else
						break;
				}
			}
			else
			{
				desc->dsc_length = charSet->substring(TEXT_LEN(desc), desc->dsc_address,
										TEXT_LEN(desc), buffer.getBuffer(TEXT_LEN(desc)), 0,
										TEXT_LEN(desc) / charSet->maxBytesPerChar());
			}
		}
	}
}


static dsc* binary_value(thread_db* tdbb, const jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      b i n a r y _ v a l u e
 *
 **************************************
 *
 * Functional description
 *      Evaluate a binary value expression.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	impure = (impure_value*) ((SCHAR *) request + node->nod_impure);

/* Evaluate arguments.  If either is null, result is null, but in
   any case, evaluate both, since some expressions may later depend
   on mappings which are developed here */

	const dsc* desc1 = EVL_expr(tdbb, node->nod_arg[0]);
	const ULONG flags = request->req_flags;
	request->req_flags &= ~req_null;

	const dsc* desc2 = EVL_expr(tdbb, node->nod_arg[1]);

/* restore saved NULL state */

	if (flags & req_null)
		request->req_flags |= req_null;

	if (request->req_flags & req_null)
		return NULL;

	// special case: concatenation doesn't need its first argument
	// being passed in the impure area, as it would double number
	// of memory allocations
	if (node->nod_type == nod_concatenate)
		return concatenate(tdbb, desc1, desc2, impure);

	EVL_make_value(tdbb, desc1, impure);

	switch (node->nod_type)
	{
	case nod_add:				/* with dialect-1 semantics */
	case nod_subtract:
		return add(desc2, node, impure);

	case nod_divide:			/* dialect-1 semantics */
		{
			const double divisor = MOV_get_double(desc2);
			if (divisor == 0)
			{
				ERR_post(Arg::Gds(isc_arith_except) <<
						 Arg::Gds(isc_exception_float_divide_by_zero));
			}
			impure->vlu_misc.vlu_double = MOV_get_double(desc1) / divisor;
			if (isinf(impure->vlu_misc.vlu_double))
			{
				ERR_post(Arg::Gds(isc_arith_except) <<
						 Arg::Gds(isc_exception_float_overflow));
			}
			impure->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
			impure->vlu_desc.dsc_length = sizeof(double);
			impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc;
			return &impure->vlu_desc;
		}

	case nod_multiply:			/* dialect-1 semantics */
		return multiply(desc2, impure, node);

	case nod_add2:				/* with dialect-3 semantics */
	case nod_subtract2:
		return add2(desc2, node, impure);

	case nod_multiply2:		/* dialect-3 semantics */
		return multiply2(desc2, impure, node);

	case nod_divide2:			/* dialect-3 semantics */
		return divide2(desc2, impure, node);

	default:
		BUGCHECK(232);			/* msg 232 EVL_expr: invalid operation */
	}
	return NULL;
}


static dsc* cast(thread_db* tdbb, dsc* value, const jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      c a s t
 *
 **************************************
 *
 * Functional description
 *      Cast from one datatype to another.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

/* value is present; make the conversion */

	const Format* format = (Format*) node->nod_arg[e_cast_fmt];
	impure->vlu_desc = format->fmt_desc[0];
	impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc;
	if (DTYPE_IS_TEXT(impure->vlu_desc.dsc_dtype)) {
		USHORT length = DSC_string_length(&impure->vlu_desc);
		if (length <= 0 && value) {
			/* cast is a subtype cast only */

			length = DSC_string_length(value);
			if (impure->vlu_desc.dsc_dtype == dtype_cstring)
				length++;		/* for NULL byte */
			else if (impure->vlu_desc.dsc_dtype == dtype_varying)
				length += sizeof(USHORT);
			impure->vlu_desc.dsc_length = length;
		}
		length = impure->vlu_desc.dsc_length;

		/* Allocate a string block of sufficient size. */

		VaryingString* string = impure->vlu_string;
		if (string && string->str_length < length) {
			delete string;
			string = NULL;
		}

		if (!string) {
			string = impure->vlu_string = FB_NEW_RPT(*tdbb->getDefaultPool(), length) VaryingString();
			string->str_length = length;
		}

		impure->vlu_desc.dsc_address = string->str_data;
	}

	EVL_validate(tdbb, Item(nod_cast), (ItemInfo*) node->nod_arg[e_cast_iteminfo],
		value, value == NULL || (value->dsc_flags & DSC_null));

	if (value == NULL)
	{
		tdbb->getRequest()->req_flags |= req_null;
		return NULL;
	}

	if (DTYPE_IS_BLOB(value->dsc_dtype) || DTYPE_IS_BLOB(impure->vlu_desc.dsc_dtype))
		BLB_move(tdbb, value, &impure->vlu_desc, NULL);
	else
		MOV_move(tdbb, value, &impure->vlu_desc);

	if (impure->vlu_desc.dsc_dtype == dtype_text)
		EVL_adjust_text_descriptor(tdbb, &impure->vlu_desc);

	return &impure->vlu_desc;
}


static void compute_agg_distinct(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *      c o m p u t e _ a g g _ d i s t i n c t
 *
 **************************************
 *
 * Functional description
 *      Sort/project the values and compute
 *      the aggregate.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	const size_t asb_index = (node->nod_type == nod_agg_list_distinct) ? 2 : 1;
	AggregateSort* asb = (AggregateSort*) node->nod_arg[asb_index];
	impure_agg_sort* asb_impure = (impure_agg_sort*) ((SCHAR *) request + asb->nod_impure);
	dsc desc = asb->asb_desc;
	impure_value_ex* impure = (impure_value_ex*) ((SCHAR *) request + node->nod_impure);

/* Sort the values already "put" to sort */

	SORT_sort(tdbb, asb_impure->iasb_sort_handle);

/* Now get the sorted/projected values and compute the aggregate */

	while (true) {
		UCHAR* data;
		SORT_get(tdbb, asb_impure->iasb_sort_handle, reinterpret_cast<ULONG**>(&data)
#ifdef SCROLLABLE_CURSORS
				 , RSE_get_forward
#endif
			);

		if (data == NULL) {
			/* we are done, close the sort */
			SORT_fini(asb_impure->iasb_sort_handle);
			asb_impure->iasb_sort_handle = NULL;
			break;
		}

		desc.dsc_address = data + (asb->asb_intl ? asb->asb_key_desc[1].skd_offset : 0);

		switch (node->nod_type)
		{
		case nod_agg_total_distinct:
		case nod_agg_average_distinct:
			++impure->vlux_count;
			add(&desc, node, impure);
			break;

		case nod_agg_total_distinct2:
		case nod_agg_average_distinct2:
			++impure->vlux_count;
			add2(&desc, node, impure);
			break;

		case nod_agg_count_distinct:
			++impure->vlux_count;
			++impure->vlu_misc.vlu_long;
			break;

		case nod_agg_list_distinct:
		{
			if (!impure->vlu_blob)
			{
				impure->vlu_blob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
					&impure->vlu_misc.vlu_bid);
				impure->vlu_desc.makeBlob(desc.getBlobSubType(), desc.getTextType(),
					(ISC_QUAD* ) &impure->vlu_misc.vlu_bid);
			}

			MoveBuffer buffer;
			UCHAR* temp;
			int len;

			if (impure->vlux_count) {
				const dsc* const delimiter = EVL_expr(tdbb, node->nod_arg[1]);
				if (request->req_flags & req_null) {
					// mark the result as NULL
					impure->vlu_desc.dsc_dtype = 0;
					break;
				}
				len = MOV_make_string2(tdbb, delimiter, impure->vlu_desc.getTextType(), &temp, buffer, false);
				BLB_put_data(tdbb, impure->vlu_blob, temp, len);
			}
			++impure->vlux_count;
			len = MOV_make_string2(tdbb, &desc, impure->vlu_desc.getTextType(), &temp, buffer, false);
			BLB_put_data(tdbb, impure->vlu_blob, temp, len);
			break;
		}

		default:	// Shut up some warnings
			break;
		}
	}

	if (node->nod_type == nod_agg_list_distinct && impure->vlu_blob)
	{
		BLB_close(tdbb, impure->vlu_blob);
		impure->vlu_blob = NULL;
	}
}


static dsc* concatenate(thread_db* tdbb,
						const dsc* value1,
						const dsc* value2,
						impure_value* impure)
{
/**************************************
 *
 *      c o n c a t e n a t e
 *
 **************************************
 *
 * Functional description
 *      Concatenate two values.
 *
 **************************************/
	SET_TDBB(tdbb);

	dsc desc;

	if (value1->dsc_dtype == dtype_dbkey && value2->dsc_dtype == dtype_dbkey)
	{
		if ((ULONG) value1->dsc_length + (ULONG) value2->dsc_length > MAX_COLUMN_SIZE - sizeof(USHORT))
		{
			ERR_post(Arg::Gds(isc_concat_overflow));
			return NULL;
		}

		desc.dsc_dtype = dtype_dbkey;
		desc.dsc_length = value1->dsc_length + value2->dsc_length;
		desc.dsc_address = NULL;

		VaryingString* string = NULL;
		if (value1->dsc_address == impure->vlu_desc.dsc_address ||
			value2->dsc_address == impure->vlu_desc.dsc_address)
		{
			string = impure->vlu_string;
			impure->vlu_string = NULL;
		}

		EVL_make_value(tdbb, &desc, impure);
		UCHAR* p = impure->vlu_desc.dsc_address;

		memcpy(p, value1->dsc_address, value1->dsc_length);
		p += value1->dsc_length;
		memcpy(p, value2->dsc_address, value2->dsc_length);

		delete string;

		return &impure->vlu_desc;
	}

	DataTypeUtil(tdbb).makeConcatenate(&desc, value1, value2);

	// Both values are present; build the concatenation

	MoveBuffer temp1;
	UCHAR* address1 = NULL;
	USHORT length1 = 0;

	if (!value1->isBlob())
		length1 = MOV_make_string2(tdbb, value1, desc.getTextType(), &address1, temp1);

	MoveBuffer temp2;
	UCHAR* address2 = NULL;
	USHORT length2 = 0;

	if (!value2->isBlob())
		length2 = MOV_make_string2(tdbb, value2, desc.getTextType(), &address2, temp2);

	if (address1 && address2)
	{
		fb_assert(desc.dsc_dtype == dtype_varying);

		if ((ULONG) length1 + (ULONG) length2 > MAX_COLUMN_SIZE - sizeof(USHORT))
		{
			ERR_post(Arg::Gds(isc_concat_overflow));
			return NULL;
		}

		desc.dsc_dtype = dtype_text;
		desc.dsc_length = length1 + length2;
		desc.dsc_address = NULL;

		VaryingString* string = NULL;
		if (value1->dsc_address == impure->vlu_desc.dsc_address ||
			value2->dsc_address == impure->vlu_desc.dsc_address)
		{
			string = impure->vlu_string;
			impure->vlu_string = NULL;
		}

		EVL_make_value(tdbb, &desc, impure);
		UCHAR* p = impure->vlu_desc.dsc_address;

		if (length1) {
			memcpy(p, address1, length1);
			p += length1;
		}
		if (length2) {
			memcpy(p, address2, length2);
		}

		delete string;
	}
	else
	{
		fb_assert(desc.isBlob());

		desc.dsc_address = (UCHAR*)&impure->vlu_misc.vlu_bid;

		blb* newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
								  &impure->vlu_misc.vlu_bid);

		Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

		if (address1)
			BLB_put_data(tdbb, newBlob, address1, length1);	// first value is not a blob
		else
		{
			Firebird::UCharBuffer bpb;
			BLB_gen_bpb_from_descs(value1, &desc, bpb);

			blb* blob = BLB_open2(tdbb, tdbb->getRequest()->req_transaction,
				reinterpret_cast<bid*>(value1->dsc_address), bpb.getCount(), bpb.begin());

			while (!(blob->blb_flags & BLB_eof))
			{
				SLONG len = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), false);

				if (len)
					BLB_put_data(tdbb, newBlob, buffer.begin(), len);
			}

			BLB_close(tdbb, blob);
		}

		if (address2)
			BLB_put_data(tdbb, newBlob, address2, length2);	// second value is not a blob
		else
		{
			Firebird::UCharBuffer bpb;
			BLB_gen_bpb_from_descs(value2, &desc, bpb);

			blb* blob = BLB_open2(tdbb, tdbb->getRequest()->req_transaction,
				reinterpret_cast<bid*>(value2->dsc_address), bpb.getCount(), bpb.begin());

			while (!(blob->blb_flags & BLB_eof))
			{
				SLONG len = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), false);

				if (len)
					BLB_put_data(tdbb, newBlob, buffer.begin(), len);
			}

			BLB_close(tdbb, blob);
		}

		BLB_close(tdbb, newBlob);

		EVL_make_value(tdbb, &desc, impure);
	}

	return &impure->vlu_desc;
}


static dsc* dbkey(thread_db* tdbb, const jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      d b k e y       ( J R D )
 *
 **************************************
 *
 * Functional description
 *      Make up a dbkey for a record stream.  A dbkey is expressed
 *      as an 8 byte character string.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	// Get request, record parameter block, and relation for stream

	jrd_req* request = tdbb->getRequest();
	impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
	const record_param* rpb = &request->req_rpb[(int) (IPTR) node->nod_arg[0]];

	const jrd_rel* relation = rpb->rpb_relation;

	// If it doesn't point to a valid record, return NULL
	if (!rpb->rpb_number.isValid() || !relation)
	{
		request->req_flags |= req_null;
		return NULL;
	}

	// Format dbkey as vector of relation id, record number

	if (relation->rel_file) {
		impure->vlu_misc.vlu_dbkey[0] = rpb->rpb_b_page;
		impure->vlu_misc.vlu_dbkey[1] = rpb->rpb_b_line;
	}
	else {
		// Initialize first 32 bits of DB_KEY
		impure->vlu_misc.vlu_dbkey[0] = 0;

		// Now, put relation ID into first 16 bits of DB_KEY
		// We do not assign it as SLONG because of big-endian machines.
		*(USHORT*)impure->vlu_misc.vlu_dbkey = relation->rel_id;

		// Encode 40-bit record number. Before that, increment the value
		// because users expect the numbering to start with one.
		RecordNumber temp(rpb->rpb_number.getValue() + 1);
		temp.bid_encode(reinterpret_cast<RecordNumber::Packed*>(impure->vlu_misc.vlu_dbkey));
	}

	// Initialize descriptor

	impure->vlu_desc.dsc_address = (UCHAR *) impure->vlu_misc.vlu_dbkey;
	impure->vlu_desc.dsc_dtype = dtype_dbkey;
	impure->vlu_desc.dsc_length = type_lengths[dtype_dbkey];
	impure->vlu_desc.dsc_ttype() = ttype_binary;

	return &impure->vlu_desc;
}


static dsc* eval_statistical(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      e v a l _ s t a t i s t i c a l
 *
 **************************************
 *
 * Functional description
 *      Evaluate a statistical expression.
 *
 **************************************/
	USHORT* invariant_flags;

	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

/* Get started by opening stream */

	jrd_req* request = tdbb->getRequest();
	dsc* desc = &impure->vlu_desc;

	if (node->nod_flags & nod_invariant) {
		invariant_flags = & impure->vlu_flags;
		if (*invariant_flags & VLU_computed) {
			/* An invariant node has already been computed. */

			if (*invariant_flags & VLU_null)
				request->req_flags |= req_null;
			else
				request->req_flags &= ~req_null;
			return desc;
		}
	}

	if ((nod_average2 == node->nod_type)) {
		impure->vlu_misc.vlu_int64 = 0;
		impure->vlu_desc.dsc_dtype = dtype_int64;
		impure->vlu_desc.dsc_length = sizeof(SINT64);
		impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc.vlu_int64;
		impure->vlu_desc.dsc_scale = 0;
	}
	else {
		impure->vlu_misc.vlu_long = 0;
		impure->vlu_desc.dsc_dtype = dtype_long;
		impure->vlu_desc.dsc_length = sizeof(SLONG);
		impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc.vlu_long;
	}

	RecordSource* rsb = (RecordSource*) node->nod_arg[e_stat_rsb];
	RSE_open(tdbb, rsb);

	SLONG count = 0;
	ULONG flag = req_null;
	double d;

	try
	{
		// Handle each variety separately

		switch (node->nod_type)
		{
		case nod_count:
			flag = 0;
			while (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				++impure->vlu_misc.vlu_long;
			}
			break;

		/*
		case nod_count2:
			flag = 0;
			while (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				EVL_expr(tdbb, node->nod_arg[e_stat_value]);
				if (!(request->req_flags & req_null)) {
					++impure->vlu_misc.vlu_long;
				}
			}
			break;
		*/

		case nod_min:
		case nod_max:
			while (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				dsc* value = EVL_expr(tdbb, node->nod_arg[e_stat_value]);
				if (request->req_flags & req_null) {
					continue;
				}
				int result;
				if (flag || ((result = MOV_compare(value, desc)) < 0 && node->nod_type == nod_min) ||
					(node->nod_type != nod_min && result > 0))
				{
					flag = 0;
					EVL_make_value(tdbb, value, impure);
				}
			}
			break;

		case nod_from:
			if (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				desc = EVL_expr(tdbb, node->nod_arg[e_stat_value]);
			}
			else
			{
				if (node->nod_arg[e_stat_default])
					desc = EVL_expr(tdbb, node->nod_arg[e_stat_default]);
				else
					ERR_post(Arg::Gds(isc_from_no_match));
			}
			flag = request->req_flags;
			break;

		case nod_average:			/* total or average with dialect-1 semantics */
		case nod_total:
			while (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				desc = EVL_expr(tdbb, node->nod_arg[e_stat_value]);
				if (request->req_flags & req_null) {
					continue;
				}
				/* Note: if the field being SUMed or AVERAGEd is short or long,
				   impure will stay long, and the first add() will
				   set the correct scale; if it is approximate numeric,
				   the first add() will convert impure to double. */
				add(desc, node, impure);
				count++;
			}
			desc = &impure->vlu_desc;
			if (node->nod_type == nod_total) {
				flag = 0;
				break;
			}
			if (!count)
				break;
			d = MOV_get_double(&impure->vlu_desc);
			impure->vlu_misc.vlu_double = d / count;
			impure->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
			impure->vlu_desc.dsc_length = sizeof(double);
			impure->vlu_desc.dsc_scale = 0;
			flag = 0;
			break;

		case nod_average2:			/* average with dialect-3 semantics */
			while (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
			{
				desc = EVL_expr(tdbb, node->nod_arg[e_stat_value]);
				if (request->req_flags & req_null)
					continue;
				/* Note: if the field being SUMed or AVERAGEd is exact
				   numeric, impure will stay int64, and the first add() will
				   set the correct scale; if it is approximate numeric,
				   the first add() will convert impure to double. */
				add(desc, node, impure);
				count++;
			}
			desc = &impure->vlu_desc;
			if (!count)
				break;
			/* We know the sum, but we want the average.  To get it, divide
			   the sum by the count.  Since count is exact, dividing an int64
			   sum by count should leave an int64 average, while dividing a
			   double sum by count should leave a double average. */
			if (dtype_int64 == impure->vlu_desc.dsc_dtype)
				impure->vlu_misc.vlu_int64 /= count;
			else
				impure->vlu_misc.vlu_double /= count;
			flag = 0;
			break;

		default:
			BUGCHECK(233);			/* msg 233 eval_statistical: invalid operation */
		}
	}
	catch (const Firebird::Exception&)
	{
		// close stream
		// ignore any error during it to keep original
		try
		{
			RSE_close(tdbb, rsb);
			request->req_flags &= ~req_null;
			request->req_flags |= flag;
		}
		catch (const Firebird::Exception&)
		{
		}

		throw;
	}

/* Close stream and return value */

	RSE_close(tdbb, rsb);
	request->req_flags &= ~req_null;
	request->req_flags |= flag;

/* If this is an invariant node, save the return value.  If the
   descriptor does not point to the impure area for this node then
   point this node's descriptor to the correct place; copy the whole
   structure to be absolutely sure */

	if (node->nod_flags & nod_invariant) {
		*invariant_flags |= VLU_computed;
		if (request->req_flags & req_null)
			*invariant_flags |= VLU_null;
		if (desc && (desc != &impure->vlu_desc))
			impure->vlu_desc = *desc;
	}

	return desc;
}


// *************
// e x t r a c t
// *************
// Handles EXTRACT(part FROM date/time/timestamp)
static dsc* extract(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	const ULONG extract_part = (IPTR) node->nod_arg[e_extract_part];
	const dsc* value = EVL_expr(tdbb, node->nod_arg[e_extract_value]);

	impure->vlu_desc.dsc_dtype = dtype_short;
	impure->vlu_desc.dsc_scale = 0;
	impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&impure->vlu_misc.vlu_short);
	impure->vlu_desc.dsc_length = sizeof(SSHORT);

	jrd_req* request = tdbb->getRequest();
	// CVC: Borland used special signaling for nulls in outer joins.
	if (!value || (request->req_flags & req_null)) {
		request->req_flags |= req_null;
		impure->vlu_misc.vlu_short = 0;
		return &impure->vlu_desc;
	}

	tm times = {0};
	int fractions;

	switch (value->dsc_dtype)
	{
	case dtype_sql_time:
		switch (extract_part)
		{
		case blr_extract_hour:
		case blr_extract_minute:
		case blr_extract_second:
		case blr_extract_millisecond:
			Firebird::TimeStamp::decode_time(*(GDS_TIME*) value->dsc_address,
										 &times.tm_hour, &times.tm_min, &times.tm_sec, &fractions);
			break;
		default:
			ERR_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_invalid_extractpart_time));
		}
		break;

	case dtype_sql_date:
		switch (extract_part)
		{
		case blr_extract_hour:
		case blr_extract_minute:
		case blr_extract_second:
		case blr_extract_millisecond:
			ERR_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_invalid_extractpart_date));
			break;
		default:
			Firebird::TimeStamp::decode_date(*(GDS_DATE*) value->dsc_address, &times);
		}
		break;

	case dtype_timestamp:
		Firebird::TimeStamp::decode_timestamp(*(GDS_TIMESTAMP*) value->dsc_address,
											  &times, &fractions);
		break;

	default:
		ERR_post(Arg::Gds(isc_expression_eval_err) <<
					Arg::Gds(isc_invalidarg_extract));
		break;
	}

	USHORT part;
	switch (extract_part)
	{
	case blr_extract_year:
		part = times.tm_year + 1900;
		break;
	case blr_extract_month:
		part = times.tm_mon + 1;
		break;
	case blr_extract_day:
		part = times.tm_mday;
		break;
	case blr_extract_hour:
		part = times.tm_hour;
		break;
	case blr_extract_minute:
		part = times.tm_min;
		break;

	case blr_extract_second:
		impure->vlu_desc.dsc_dtype = dtype_long;
		impure->vlu_desc.dsc_length = sizeof(ULONG);
		impure->vlu_desc.dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
		impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&impure->vlu_misc.vlu_long);
		*(ULONG*) impure->vlu_desc.dsc_address = times.tm_sec * ISC_TIME_SECONDS_PRECISION + fractions;
		return &impure->vlu_desc;

	case blr_extract_millisecond:
		impure->vlu_desc.dsc_dtype = dtype_long;
		impure->vlu_desc.dsc_length = sizeof(ULONG);
		impure->vlu_desc.dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE + 3;
		impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&impure->vlu_misc.vlu_long);
		(*(ULONG*) impure->vlu_desc.dsc_address) = fractions;
		return &impure->vlu_desc;

	case blr_extract_week:
		{
			// Algorithm for Converting Gregorian Dates to ISO 8601 Week Date by Rick McCarty, 1999
			// http://personal.ecu.edu/mccartyr/ISOwdALG.txt

			const int y = times.tm_year + 1900;
			const int dayOfYearNumber = times.tm_yday + 1;

			// Find the jan1Weekday for y (Monday=1, Sunday=7)
			const int yy = (y - 1) % 100;
			const int c = (y - 1) - yy;
			const int g = yy + yy / 4;
			const int jan1Weekday = 1 + (((((c / 100) % 4) * 5) + g) % 7);

			// Find the weekday for y m d
			const int h = dayOfYearNumber + (jan1Weekday - 1);
			const int weekday = 1 + ((h - 1) % 7);

			// Find if y m d falls in yearNumber y-1, weekNumber 52 or 53
			int yearNumber, weekNumber;

			if ((dayOfYearNumber <= (8 - jan1Weekday)) && (jan1Weekday > 4))
			{
				yearNumber = y - 1;
				weekNumber = ((jan1Weekday == 5) || ((jan1Weekday == 6) &&
					Firebird::TimeStamp::isLeapYear(yearNumber))) ? 53 : 52;
			}
			else
			{
				yearNumber = y;

				// Find if y m d falls in yearNumber y+1, weekNumber 1
				int i = Firebird::TimeStamp::isLeapYear(y) ? 366 : 365;

				if ((i - dayOfYearNumber) < (4 - weekday))
				{
					yearNumber = y + 1;
					weekNumber = 1;
				}
			}

			// Find if y m d falls in yearNumber y, weekNumber 1 through 53
			if (yearNumber == y)
			{
				int j = dayOfYearNumber + (7 - weekday) + (jan1Weekday - 1);
				weekNumber = j / 7;
				if (jan1Weekday > 4)
					weekNumber--;
			}

			part = weekNumber;
		}
		break;

	case blr_extract_yearday:
		part = times.tm_yday;
		break;
	case blr_extract_weekday:
		part = times.tm_wday;
		break;
	default:
		fb_assert(false);
		part = 0;
	}

	*(USHORT *) impure->vlu_desc.dsc_address = part;
	return &impure->vlu_desc;
}


static void fini_agg_distinct(thread_db* tdbb, const jrd_nod *const node)
{
/**************************************
 *
 *      f i n i _ a g g _ d i s t i n c t
 *
 **************************************
 *
 * Functional description
 *      Finalize a sort for distinct aggregate.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	jrd_nod* map = node->nod_arg[e_agg_map];

	jrd_nod** ptr;
	const jrd_nod* const* end;

	for (ptr = map->nod_arg, end = ptr + map->nod_count; ptr < end; ptr++)
	{
		const jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];
		switch (from->nod_type)
		{
		case nod_agg_count_distinct:
		case nod_agg_total_distinct:
		case nod_agg_average_distinct:
		case nod_agg_average_distinct2:
		case nod_agg_total_distinct2:
		case nod_agg_list_distinct:
			{
				const size_t asb_index = (from->nod_type == nod_agg_list_distinct) ? 2 : 1;
				const AggregateSort* asb = (AggregateSort*) from->nod_arg[asb_index];
				impure_agg_sort* asb_impure = (impure_agg_sort*) ((SCHAR *) request + asb->nod_impure);
				SORT_fini(asb_impure->iasb_sort_handle);
				asb_impure->iasb_sort_handle = NULL;
			}
		}
	}
}


static SINT64 get_day_fraction(const dsc* d)
{
/**************************************
 *
 *      g e t _ d a y _ f r a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Take the input number, assume it represents a fractional
 *	count of days.  Convert it to a count of microseconds.
 *
 **************************************/
	dsc result;
	double result_days;

	result.dsc_dtype = dtype_double;
	result.dsc_scale = 0;
	result.dsc_flags = 0;
	result.dsc_sub_type = 0;
	result.dsc_length = sizeof(double);
	result.dsc_address = reinterpret_cast<UCHAR*>(&result_days);

/* Convert the input number to a double */
	CVT_move(d, &result);

/* There's likely some loss of precision here due to rounding of number */

// 08-Apr-2004, Nickolay Samofatov. Loss of precision manifested itself as bad
// result returned by the following query:
//
// select (cast('01.01.2004 10:01:00' as timestamp)
//   -cast('01.01.2004 10:00:00' as timestamp))
//   +cast('01.01.2004 10:00:00' as timestamp) from rdb$database
//
// Let's use llrint where it is supported and offset number for other platforms
// in hope that compiler rounding mode doesn't get in.

#ifdef HAVE_LLRINT
	return llrint(result_days * ISC_TICKS_PER_DAY);
#else
	const double eps = 0.49999999999999;
	if (result_days >= 0)
		return (SINT64)(result_days * ISC_TICKS_PER_DAY + eps);

	return (SINT64)(result_days * ISC_TICKS_PER_DAY - eps);
#endif
}



static dsc* get_mask(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      g e t _ m a s k
 *
 **************************************
 *
 * Functional description
 *      Compute protection mask.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	TEXT* p1 = NULL;
	TEXT* p2 = NULL;
	SqlIdentifier relation_name, field_name;

	const dsc* value = EVL_expr(tdbb, node->nod_arg[0]);
	if (!(request->req_flags & req_null)) {
		p1 = relation_name;
		MOV_get_name(value, p1);
		value = EVL_expr(tdbb, node->nod_arg[1]);
		if (!(request->req_flags & req_null)) {
			p2 = field_name;
			MOV_get_name(value, p2);
		}
	}

	request->req_flags &= ~req_null;

	// SecurityClass::flags_t is USHORT for now, so it fits in vlu_long.
	impure->make_long(SCL_get_mask(tdbb, p1, p2));
	return &impure->vlu_desc;
}


static SINT64 get_timestamp_to_isc_ticks(const dsc* d)
{
/**************************************
 *
 *	g e t _ t i m e s t a m p _ t o _ i s c _ t i c k s
 *
 **************************************
 *
 * Functional description
 *	Take the input value, which is either a timestamp
 *	or a string representing a timestamp.  Convert it to
 *	a timestamp, and then return that timestamp as a
 *	count of isc_ticks since the base date and time
 *	in MJD time arithmetic.
 *	ISC_TICKS or isc_ticks are actually deci - milli seconds or
 *	tenthousandth of seconds per day. This is derived from the
 *	ISC_TIME_SECONDS_PRECISION.
 *
 **************************************/
	dsc result;
	GDS_TIMESTAMP result_timestamp;

	result.dsc_dtype = dtype_timestamp;
	result.dsc_scale = 0;
	result.dsc_flags = 0;
	result.dsc_sub_type = 0;
	result.dsc_length = sizeof(GDS_TIMESTAMP);
	result.dsc_address = reinterpret_cast<UCHAR*>(&result_timestamp);

	CVT_move(d, &result);

	return ((SINT64) result_timestamp.timestamp_date) * ISC_TICKS_PER_DAY +
		(SINT64) result_timestamp.timestamp_time;
}


static void init_agg_distinct(thread_db* tdbb, const jrd_nod* node)
{
/**************************************
 *
 *      i n i t _ a g g _ d i s t i n c t
 *
 **************************************
 *
 * Functional description
 *      Initialize a sort for distinct aggregate.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();

	const size_t asb_index = (node->nod_type == nod_agg_list_distinct) ? 2 : 1;
	const AggregateSort* asb = (AggregateSort*) node->nod_arg[asb_index];
	impure_agg_sort* asb_impure = (impure_agg_sort*) ((char*) request + asb->nod_impure);
	const sort_key_def* sort_key = asb->asb_key_desc;

	// Get rid of the old sort areas if this request has been used already
	SORT_fini(asb_impure->iasb_sort_handle);

	asb_impure->iasb_sort_handle =
		SORT_init(tdbb->getDatabase(), &request->req_sorts, asb->asb_length,
		(asb->asb_intl ? 2 : 1), 1, sort_key, reject_duplicate, 0);
}


static dsc* lock_state(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      l o c k _ s t a t e
 *
 **************************************
 *
 * Functional description
 *      Compute state of an attachment id.  The values
 *      returned are:
 *
 *          0 - value is null
 *          1 - attachment is gone
 *          2 - we are that attachment
 *          3 - attachment is active
 *
 **************************************/
	SET_TDBB(tdbb);

	Database* dbb = tdbb->getDatabase();

	DEV_BLKCHK(node, type_nod);

/* Initialize descriptor */

	impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc.vlu_long;
	impure->vlu_desc.dsc_dtype = dtype_long;
	impure->vlu_desc.dsc_length = sizeof(SLONG);
	impure->vlu_desc.dsc_scale = 0;

/* Evaluate attachment id */

	jrd_req* request = tdbb->getRequest();
	const dsc* desc = EVL_expr(tdbb, node->nod_arg[0]);

	if (request->req_flags & req_null)
		impure->vlu_misc.vlu_long = 0;
	else {
		const SLONG id = MOV_get_long(desc, 0);
		if (id == PAG_attachment_id(tdbb))
			impure->vlu_misc.vlu_long = 2;
		else {
			Lock temp_lock;
			/* fill out a lock block, zeroing it out first */

			temp_lock.lck_parent = dbb->dbb_lock;
			temp_lock.lck_type = LCK_attachment;
			temp_lock.lck_owner_handle = LCK_get_owner_handle(tdbb, temp_lock.lck_type);
			temp_lock.lck_length = sizeof(SLONG);
			temp_lock.lck_key.lck_long = id;
			temp_lock.lck_dbb = dbb;

			if (LCK_lock(tdbb, &temp_lock, LCK_write, LCK_NO_WAIT)) {
				impure->vlu_misc.vlu_long = 1;
				LCK_release(tdbb, &temp_lock);
			}
			else
				impure->vlu_misc.vlu_long = 3;
		}
	}

	return &impure->vlu_desc;
}


static dsc* low_up_case(thread_db* tdbb, const dsc* value, impure_value* impure,
	ULONG (TextType::*tt_str_to_case)(ULONG, const UCHAR*, ULONG, UCHAR*))
{
/**************************************
 *
 *      l o w _ u p _ c a s e
 *
 **************************************
 *
 * Functional description
 *      Low/up case a string.
 *
 **************************************/
	SET_TDBB(tdbb);

	TextType* textType = INTL_texttype_lookup(tdbb, value->getTextType());

	if (value->isBlob())
	{
		EVL_make_value(tdbb, value, impure);

		if (value->dsc_sub_type != isc_blob_text)
			return &impure->vlu_desc;

		CharSet* charSet = textType->getCharSet();

		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address));

		Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

		if (charSet->isMultiByte())
			buffer.getBuffer(blob->blb_length);	// alloc space to put entire blob in memory

		blb* newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
			&impure->vlu_misc.vlu_bid);

		while (!(blob->blb_flags & BLB_eof))
		{
			SLONG len = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), false);

			if (len)
			{
				len = (textType->*tt_str_to_case)(len, buffer.begin(), len, buffer.begin());
				BLB_put_data(tdbb, newBlob, buffer.begin(), len);
			}
		}

		BLB_close(tdbb, newBlob);
		BLB_close(tdbb, blob);
	}
	else
	{
		VaryStr<32> temp;
		USHORT ttype;
		dsc desc;

		desc.dsc_length = MOV_get_string_ptr(value, &ttype, &desc.dsc_address, &temp, sizeof(temp));
		desc.dsc_dtype = dtype_text;
		INTL_ASSIGN_TTYPE(&desc, ttype);
		EVL_make_value(tdbb, &desc, impure);

		if (value->isText())
		{
			impure->vlu_desc.dsc_length = (textType->*tt_str_to_case)(desc.dsc_length,
				impure->vlu_desc.dsc_address, desc.dsc_length, impure->vlu_desc.dsc_address);
		}
	}

	return &impure->vlu_desc;
}


static dsc* multiply(const dsc* desc, impure_value* value, const jrd_nod* node)
{
/**************************************
 *
 *      m u l t i p l y
 *
 **************************************
 *
 * Functional description
 *      Multiply two numbers, with SQL dialect-1 semantics
 *      This function can be removed when dialect-3 becomes
 *      the lowest supported dialect.  (Version 7.0?)
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

/* Handle floating arithmetic */

	if (node->nod_flags & nod_double) {
		const double d1 = MOV_get_double(desc);
		const double d2 = MOV_get_double(&value->vlu_desc);
		value->vlu_misc.vlu_double = d1 * d2;
		if (isinf(value->vlu_misc.vlu_double))
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_overflow));
		}
		value->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
		value->vlu_desc.dsc_length = sizeof(double);
		value->vlu_desc.dsc_scale = 0;
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return &value->vlu_desc;
	}

/* Handle (oh, ugh) quad arithmetic */

	if (node->nod_flags & nod_quad) {
		const SSHORT scale = NUMERIC_SCALE(value->vlu_desc);
		const SQUAD q1 = MOV_get_quad(desc, node->nod_scale - scale);
		const SQUAD q2 = MOV_get_quad(&value->vlu_desc, scale);
		value->vlu_desc.dsc_dtype = dtype_quad;
		value->vlu_desc.dsc_length = sizeof(SQUAD);
		value->vlu_desc.dsc_scale = node->nod_scale;
		value->vlu_misc.vlu_quad = QUAD_MULTIPLY(q1, q2, ERR_post);
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_quad;

		return &value->vlu_desc;
	}

/* Everything else defaults to longword */

	/* CVC: With so many problems cropping with dialect 1 and multiplication,
			I decided to close this Pandora box by incurring in INT64 performance
			overhead (if noticeable) and try to get the best result. When I read it,
			this function didn't bother even to check for overflow! */

#define FIREBIRD_AVOID_DIALECT1_OVERFLOW
	// SLONG l1, l2;
	//{
	const SSHORT scale = NUMERIC_SCALE(value->vlu_desc);
	const SINT64 i1 = MOV_get_long(desc, node->nod_scale - scale);
	const SINT64 i2 = MOV_get_long(&value->vlu_desc, scale);
	value->vlu_desc.dsc_dtype = dtype_long;
	value->vlu_desc.dsc_length = sizeof(SLONG);
	value->vlu_desc.dsc_scale = node->nod_scale;
	const SINT64 rc = i1 * i2;
	if (rc < MIN_SLONG || rc > MAX_SLONG)
	{
#ifdef FIREBIRD_AVOID_DIALECT1_OVERFLOW
		value->vlu_misc.vlu_int64 = rc;
		value->vlu_desc.dsc_address = (UCHAR*) &value->vlu_misc.vlu_int64;
		value->vlu_desc.dsc_dtype = dtype_int64;
		value->vlu_desc.dsc_length = sizeof(SINT64);
		value->vlu_misc.vlu_double = MOV_get_double(&value->vlu_desc);
		/* This is the Borland solution instead of the five lines above.
		d1 = MOV_get_double (desc);
		d2 = MOV_get_double (&value->vlu_desc);
		value->vlu_misc.vlu_double = d1 * d2; */
		value->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
		value->vlu_desc.dsc_length = sizeof(double);
		value->vlu_desc.dsc_scale = 0;
		value->vlu_desc.dsc_address = (UCHAR*) &value->vlu_misc.vlu_double;
#else
		ERR_post(Arg::Gds(isc_exception_integer_overflow));
#endif
	}
	else
	{
		value->vlu_misc.vlu_long = (SLONG) rc; /* l1 * l2;*/
		value->vlu_desc.dsc_address = (UCHAR*) &value->vlu_misc.vlu_long;
	}
	//}

	return &value->vlu_desc;
}


static dsc* multiply2(const dsc* desc, impure_value* value, const jrd_nod* node)
{
/**************************************
 *
 *      m u l t i p l y 2
 *
 **************************************
 *
 * Functional description
 *      Multiply two numbers, with dialect-3 semantics,
 *      implementing blr_version5 ... blr_multiply.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

/* Handle floating arithmetic */

	if (node->nod_flags & nod_double)
	{
		const double d1 = MOV_get_double(desc);
		const double d2 = MOV_get_double(&value->vlu_desc);
		value->vlu_misc.vlu_double = d1 * d2;
		if (isinf(value->vlu_misc.vlu_double))
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_overflow));
		}
		value->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
		value->vlu_desc.dsc_length = sizeof(double);
		value->vlu_desc.dsc_scale = 0;
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return &value->vlu_desc;
	}

/* Handle (oh, ugh) quad arithmetic */

	if (node->nod_flags & nod_quad)
	{
		const SSHORT scale = NUMERIC_SCALE(value->vlu_desc);
		const SQUAD q1 = MOV_get_quad(desc, node->nod_scale - scale);
		const SQUAD q2 = MOV_get_quad(&value->vlu_desc, scale);
		value->vlu_desc.dsc_dtype = dtype_quad;
		value->vlu_desc.dsc_length = sizeof(SQUAD);
		value->vlu_desc.dsc_scale = node->nod_scale;
		value->vlu_misc.vlu_quad = QUAD_MULTIPLY(q1, q2, ERR_post);
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_quad;

		return &value->vlu_desc;
	}

/* Everything else defaults to int64 */

	const SSHORT scale = NUMERIC_SCALE(value->vlu_desc);
	const SINT64 i1 = MOV_get_int64(desc, node->nod_scale - scale);
	const SINT64 i2 = MOV_get_int64(&value->vlu_desc, scale);

/*
   We need to report an overflow if
       (i1 * i2 < MIN_SINT64) || (i1 * i2 > MAX_SINT64)
   which is equivalent to
       (i1 < MIN_SINT64 / i2) || (i1 > MAX_SINT64 / i2)

   Unfortunately, a trial division to see whether the multiplication will
   overflow is expensive: fortunately, we only need perform one division and
   test for one of the two cases, depending on whether the factors have the
   same or opposite signs.

   Unfortunately, in C it is unspecified which way division rounds
   when one or both arguments are negative.  (ldiv() is guaranteed to
   round towards 0, but the standard does not yet require an lldiv()
   or whatever for 64-bit operands.  This makes the problem messy.
   We use FB_UINT64s for the checking, thus ensuring that our division rounds
   down.  This means that we have to check the sign of the product first
   in order to know whether the maximum abs(i1*i2) is MAX_SINT64 or
   (MAX_SINT64+1).

   Of course, if a factor is 0, the product will also be 0, and we don't
   need a trial-division to be sure the multiply won't overflow.
 */

	const FB_UINT64 u1 = (i1 >= 0) ? i1 : -i1;	// abs(i1)
	const FB_UINT64 u2 = (i2 >= 0) ? i2 : -i2;	// abs(i2)
	// largest product
	const FB_UINT64 u_limit = ((i1 ^ i2) >= 0) ? MAX_SINT64 : (FB_UINT64) MAX_SINT64 + 1;

	if ((u1 != 0) && ((u_limit / u1) < u2)) {
		ERR_post(Arg::Gds(isc_exception_integer_overflow));
	}

	value->vlu_desc.dsc_dtype   = dtype_int64;
	value->vlu_desc.dsc_length  = sizeof(SINT64);
	value->vlu_desc.dsc_scale   = node->nod_scale;
	value->vlu_misc.vlu_int64   = i1 * i2;
	value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_int64;

	return &value->vlu_desc;
}


static dsc* divide2(const dsc* desc, impure_value* value, const jrd_nod* node)
{
/**************************************
 *
 *      d i v i d e 2
 *
 **************************************
 *
 * Functional description
 *      Divide two numbers, with SQL dialect-3 semantics,
 *      as in the blr_version5 ... blr_divide or
 *                blr_version5 ... blr_average ....
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

/* Handle floating arithmetic */

	if (node->nod_flags & nod_double) {
		const double d2 = MOV_get_double(desc);
		if (d2 == 0.0)
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_divide_by_zero));
		}
		const double d1 = MOV_get_double(&value->vlu_desc);
		value->vlu_misc.vlu_double = d1 / d2;
		if (isinf(value->vlu_misc.vlu_double))
		{
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_exception_float_overflow));
		}
		value->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
		value->vlu_desc.dsc_length = sizeof(double);
		value->vlu_desc.dsc_scale = 0;
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
		return &value->vlu_desc;
	}

/* Everything else defaults to int64 */

/*
 * In the SQL standard, the precision and scale of the quotient of exact
 * numeric dividend and divisor are implementation-defined: we have defined
 * the precision as 18 (in other words, an SINT64), and the scale as the
 * sum of the scales of the two operands.  To make this work, we have to
 * multiply by pow(10, -2* (scale of divisor)).
 *
 * To see this, consider the operation n1 / n2, and represent the numbers
 * by ordered pairs (v1, s1) and (v2, s2), representing respectively the
 * integer value and the scale of each operation, so that
 *     n1 = v1 * pow(10, s1), and
 *     n2 = v2 * pow(10, s2)
 * Then the quotient is ...
 *
 *     v1 * pow(10,s1)
 *     ----------------- = (v1/v2) * pow(10, s1-s2)
 *     v2 * pow(10,s2)
 *
 * But we want the scale of the result to be (s1+s2), not (s1-s2)
 * so we need to multiply by 1 in the form
 *         pow(10, -2 * s2) * pow(20, 2 * s2)
 * which, after regrouping, gives us ...
 *   =  ((v1 * pow(10, -2*s2))/v2) * pow(10, 2*s2) * pow(10, s1-s2)
 *   =  ((v1 * pow(10, -2*s2))/v2) * pow(10, 2*s2 + s1 - s2)
 *   =  ((v1 * pow(10, -2*s2))/v2) * pow(10, s1 + s2)
 * or in our ordered-pair notation,
 *      ( v1 * pow(10, -2*s2) / v2, s1 + s2 )
 *
 * To maximize the amount of information in the result, we scale up
 * the dividend as far as we can without causing overflow, then we perform
 * the division, then do any additional required scaling.
 *
 * Who'da thunk that 9th-grade algebra would prove so useful.
 *                                      -- Chris Jewell, December 1998
 */
	SINT64 i2 = MOV_get_int64(desc, desc->dsc_scale);
	if (i2 == 0)
	{
		ERR_post(Arg::Gds(isc_arith_except) <<
				 Arg::Gds(isc_exception_integer_divide_by_zero));
	}

	SINT64 i1 = MOV_get_int64(&value->vlu_desc, node->nod_scale - desc->dsc_scale);

/* MIN_SINT64 / -1 = (MAX_SINT64 + 1), which overflows in SINT64. */
	if ((i1 == MIN_SINT64) && (i2 == -1))
		ERR_post(Arg::Gds(isc_exception_integer_overflow));

/* Scale the dividend by as many of the needed powers of 10 as possible
   without causing an overflow. */
	int addl_scale = 2 * desc->dsc_scale;
	if (i1 >= 0)
	{
		while ((addl_scale < 0) && (i1 <= MAX_INT64_LIMIT))
		{
			i1 *= 10;
			++addl_scale;
		}
	}
	else
	{
		while ((addl_scale < 0) && (i1 >= MIN_INT64_LIMIT))
		{
			i1 *= 10;
			++addl_scale;
		}
	}

/* If we couldn't use up all the additional scaling by multiplying the
   dividend by 10, but there are trailing zeroes in the divisor, we can
   get the same effect by dividing the divisor by 10 instead. */
	while ((addl_scale < 0) && (0 == (i2 % 10))) {
		i2 /= 10;
		++addl_scale;
	}

	value->vlu_desc.dsc_dtype = dtype_int64;
	value->vlu_desc.dsc_length = sizeof(SINT64);
	value->vlu_desc.dsc_scale = node->nod_scale;
	value->vlu_misc.vlu_int64 = i1 / i2;
	value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_int64;

/* If we couldn't do all the required scaling beforehand without causing
   an overflow, do the rest of it now.  If we get an overflow now, then
   the result is really too big to store in a properly-scaled SINT64,
   so report the error. For example, MAX_SINT64 / 1.00 overflows. */
	if (value->vlu_misc.vlu_int64 >= 0)
	{
		while ((addl_scale < 0) && (value->vlu_misc.vlu_int64 <= MAX_INT64_LIMIT))
		{
			value->vlu_misc.vlu_int64 *= 10;
			addl_scale++;
		}
	}
	else
	{
		while ((addl_scale < 0) && (value->vlu_misc.vlu_int64 >= MIN_INT64_LIMIT))
		{
			value->vlu_misc.vlu_int64 *= 10;
			addl_scale++;
		}
	}

	if (addl_scale < 0)
	{
		ERR_post(Arg::Gds(isc_arith_except) <<
				 Arg::Gds(isc_numeric_out_of_range));
	}

	return &value->vlu_desc;
}


static dsc* negate_dsc(thread_db* tdbb, const dsc* desc, impure_value* value)
{
/**************************************
 *
 *      n e g a t e
 *
 **************************************
 *
 * Functional description
 *      Negate a single value
 *
 **************************************/

	SET_TDBB(tdbb);
	EVL_make_value(tdbb, desc, value);

	switch (value->vlu_desc.dsc_dtype)
	{
	case dtype_short:
		if (value->vlu_misc.vlu_short == MIN_SSHORT)
			ERR_post(Arg::Gds(isc_exception_integer_overflow));
		value->vlu_misc.vlu_short = -value->vlu_misc.vlu_short;
		break;

	case dtype_long:
		if (value->vlu_misc.vlu_long == MIN_SLONG)
			ERR_post(Arg::Gds(isc_exception_integer_overflow));
		value->vlu_misc.vlu_long = -value->vlu_misc.vlu_long;
		break;

	case dtype_real:
		value->vlu_misc.vlu_float = -value->vlu_misc.vlu_float;
		break;

	case DEFAULT_DOUBLE:
		value->vlu_misc.vlu_double = -value->vlu_misc.vlu_double;
		break;

	case dtype_quad:
		value->vlu_misc.vlu_quad =
			QUAD_NEGATE(value->vlu_misc.vlu_quad, ERR_post);
		break;

	case dtype_int64:
		if (value->vlu_misc.vlu_int64 == MIN_SINT64)
			ERR_post(Arg::Gds(isc_exception_integer_overflow));
		value->vlu_misc.vlu_int64 = -value->vlu_misc.vlu_int64;
		break;

	default:
		value->vlu_misc.vlu_double = -MOV_get_double(&value->vlu_desc);
		value->vlu_desc.dsc_dtype = DEFAULT_DOUBLE;
		value->vlu_desc.dsc_length = sizeof(double);
		value->vlu_desc.dsc_scale = 0;
		value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc.vlu_double;
	}

	return &value->vlu_desc;
}


static dsc* record_version(thread_db* tdbb, const jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      r e c o r d _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *      Make up a record version for a record stream.
 *      The tid of the record will be used.  This will be returned
 *      as a 4 byte character string.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	// Get request, record parameter block for stream

	jrd_req* request = tdbb->getRequest();
	// Already set by the caller
	//impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
	const record_param* rpb = &request->req_rpb[(int) (IPTR) node->nod_arg[0]];

/* If the current transaction has updated the record, the record version
 * coming in from DSQL will have the original transaction # (or current
 * transaction if the current transaction updated the record in a different
 * request.).  In these cases, mark the request so that the boolean
 * to check equality of record version will be forced to evaluate to true.
 */

	if (tdbb->getRequest()->req_transaction->tra_number == rpb->rpb_transaction_nr)
		request->req_flags |= req_same_tx_upd;
	else
	{
		/* If the transaction is a commit retain, check if the record was
		 * last updated in one of its own prior transactions
		 */

		if (request->req_transaction->tra_commit_sub_trans)
		{
			if (request->req_transaction->tra_commit_sub_trans->test(rpb->rpb_transaction_nr))
			{
				 request->req_flags |= req_same_tx_upd;
			}
		}
	}

/* Initialize descriptor */

	impure->vlu_misc.vlu_long    = rpb->rpb_transaction_nr;
	impure->vlu_desc.dsc_address = (UCHAR *) & impure->vlu_misc.vlu_long;
	impure->vlu_desc.dsc_dtype   = dtype_text;
	impure->vlu_desc.dsc_length  = 4;
	impure->vlu_desc.dsc_ttype()   = ttype_binary;

	return &impure->vlu_desc;
}


static bool reject_duplicate(const UCHAR* /*data1*/, const UCHAR* /*data2*/, void* /*user_arg*/)
{
/**************************************
 *
 *      r e j e c t _ d u p l i c a t e
 *
 **************************************
 *
 * Functional description
 *      Callback routine used by sort/project to reject duplicate values.
 *      Particularly dumb routine -- always returns true;
 *
 **************************************/

	return true;
}


static dsc* scalar(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      s c a l a r
 *
 **************************************
 *
 * Functional description
 *      Evaluate a scalar item from an array.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	const dsc* desc = EVL_expr(tdbb, node->nod_arg[e_scl_field]);
	jrd_req* request = tdbb->getRequest();

	if (request->req_flags & req_null)
		return NULL;

	if (desc->dsc_dtype != dtype_array)
		IBERROR(261);			// msg 261 scalar operator used on field which is not an array

	jrd_nod* list = node->nod_arg[e_scl_subscripts];
	if (list->nod_count > MAX_ARRAY_DIMENSIONS)
		ERR_post(Arg::Gds(isc_array_max_dimensions) << Arg::Num(MAX_ARRAY_DIMENSIONS));

	SLONG subscripts[MAX_ARRAY_DIMENSIONS];
	int iter = 0;
	jrd_nod** ptr = list->nod_arg;
	for (const jrd_nod* const* const end = ptr + list->nod_count; ptr < end;)
	{
		const dsc* temp = EVL_expr(tdbb, *ptr++);
		if (temp && !(request->req_flags & req_null))
			subscripts[iter++] = MOV_get_long(temp, 0);
		else
			return NULL;
	}

	BLB_scalar(tdbb,
			   request->req_transaction,
			   reinterpret_cast<bid*>(desc->dsc_address),
			   list->nod_count,
			   subscripts, impure);
			   // It was subscripts, reinterpret_cast<impure_value*>(&impure->vlu_desc));
			   // but vlu_desc is the first member of impure_value and impure is already
			   // of type impure_value*, so this cast seems nonsense.

	return &impure->vlu_desc;
}


static bool sleuth(thread_db* tdbb, jrd_nod* node, const dsc* desc1, const dsc* desc2)
{
/**************************************
 *
 *      s l e u t h
 *
 **************************************
 *
 * Functional description
 *      Execute SLEUTH operator.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

/* Choose interpretation for the operation */

 	USHORT ttype;
	if (desc1->isBlob())
	{
		if (desc1->dsc_sub_type == isc_blob_text)
			ttype = desc1->dsc_blob_ttype();	/* Load blob character set and collation */
		else
			ttype = INTL_TTYPE(desc2);
	}
	else
		ttype = INTL_TTYPE(desc1);

	Collation* obj = INTL_texttype_lookup(tdbb, ttype);

/* Get operator definition string (control string) */

	dsc* desc3 = EVL_expr(tdbb, node->nod_arg[2]);

	UCHAR* p1;
	MoveBuffer sleuth_str;
	USHORT l1 = MOV_make_string2(tdbb, desc3, ttype, &p1, sleuth_str);
/* Get address and length of search string */
	UCHAR* p2;
	MoveBuffer match_str;
	USHORT l2 = MOV_make_string2(tdbb, desc2, ttype, &p2, match_str);

/* Merge search and control strings */
	UCHAR control[BUFFER_SMALL];
	SLONG control_length = obj->sleuthMerge(*tdbb->getDefaultPool(), p2, l2, p1, l1, control); //, BUFFER_SMALL);

/* Note: resulting string from sleuthMerge is either USHORT or UCHAR
   and never Multibyte (see note in EVL_mb_sleuthCheck) */
	bool ret_val;
	MoveBuffer data_str;
	if (!desc1->isBlob())
	{
		/* Source is not a blob, do a simple search */

		l1 = MOV_make_string2(tdbb, desc1, ttype, &p1, data_str);
		ret_val = obj->sleuthCheck(*tdbb->getDefaultPool(), 0, p1, l1, control, control_length);
	}
	else {
		/* Source string is a blob, things get interesting */

		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
							 reinterpret_cast<bid*>(desc1->dsc_address));

		UCHAR buffer[BUFFER_LARGE];
		ret_val = false;
		while (!(blob->blb_flags & BLB_eof))
		{
			l1 = BLB_get_segment(tdbb, blob, buffer, sizeof(buffer));
			if (obj->sleuthCheck(*tdbb->getDefaultPool(), 0, buffer, l1, control, control_length))
			{
				ret_val = true;
				break;
			}
		}

		BLB_close(tdbb, blob);
	}

	return ret_val;
}


static bool string_boolean(thread_db* tdbb, jrd_nod* node, dsc* desc1,
							 dsc* desc2, bool computed_invariant)
{
/**************************************
 *
 *      s t r i n g _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *      Perform one of the complex string functions CONTAINING, MATCHES,
 *      or STARTS WITH.
 *
 **************************************/
	UCHAR* p1 = NULL;
	UCHAR* p2 = NULL;
	SLONG l2 = 0;
	USHORT type1;
	MoveBuffer match_str;

	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	DEV_BLKCHK(node, type_nod);

	if (!desc1->isBlob())
	{
		/* Source is not a blob, do a simple search */

		/* Get text type of data string */

		type1 = INTL_TEXT_TYPE(*desc1);

		/* Get address and length of search string - convert to datatype of data */

		if (!computed_invariant) {
			l2 = MOV_make_string2(tdbb, desc2, type1, &p2, match_str, false);
		}

		VaryStr<256> temp1;
		USHORT xtype1;
		const USHORT l1 = MOV_get_string_ptr(desc1, &xtype1, &p1, &temp1, sizeof(temp1));

		fb_assert(xtype1 == type1);

		return string_function(tdbb, node, l1, p1, l2, p2, type1, computed_invariant);
	}

	/* Source string is a blob, things get interesting */

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

	if (desc1->dsc_sub_type == isc_blob_text)
		type1 = desc1->dsc_blob_ttype();	/* pick up character set and collation of blob */
	else
		type1 = ttype_none;	/* Do byte matching */

	Collation* obj = INTL_texttype_lookup(tdbb, type1);
	CharSet* charset = obj->getCharSet();

	// Get address and length of search string - make it string if necessary
	// but don't transliterate character set if the source blob is binary
	if (!computed_invariant) {
		l2 = MOV_make_string2(tdbb, desc2, type1, &p2, match_str, false);
	}

	blb* blob =	BLB_open(tdbb, request->req_transaction, reinterpret_cast<bid*>(desc1->dsc_address));

	if (charset->isMultiByte() &&
		(node->nod_type != nod_starts || !(obj->getFlags() & TEXTTYPE_DIRECT_MATCH)))
	{
		buffer.getBuffer(blob->blb_length);		// alloc space to put entire blob in memory
	}

	/* Performs the string_function on each segment of the blob until
	   a positive result is obtained */

	bool ret_val = false;

	switch (node->nod_type)
	{
	case nod_like:
	case nod_similar:
		{
			VaryStr<TEMP_LENGTH> temp3;
			const UCHAR* escape_str = NULL;
			USHORT escape_length = 0;

			/* ensure 3rd argument (escape char) is in operation text type */
			if (node->nod_count == 3 && !computed_invariant) {
				/* Convert ESCAPE to operation character set */
				DSC* dsc = EVL_expr(tdbb, node->nod_arg[2]);
				if (request->req_flags & req_null) {
					if (node->nod_flags & nod_invariant) {
						impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
						impure->vlu_flags |= VLU_computed;
						impure->vlu_flags |= VLU_null;
					}
					ret_val = false;
					break;
				}

				escape_length = MOV_make_string(dsc, type1,
												reinterpret_cast<const char**>(&escape_str),
												&temp3, sizeof(temp3));
				if (!escape_length || charset->length(escape_length, escape_str, true) != 1)
				{
					/* If characters left, or null byte character, return error */
					BLB_close(tdbb, blob);
					ERR_post(Arg::Gds(isc_escape_invalid));
				}

				USHORT escape[2] = {0, 0};

				charset->getConvToUnicode().convert(escape_length, escape_str, sizeof(escape), escape);
				if (!escape[0])
				{
					/* If or null byte character, return error */
					BLB_close(tdbb, blob);
					ERR_post(Arg::Gds(isc_escape_invalid));
				}
			}

			PatternMatcher* evaluator;

			if (node->nod_flags & nod_invariant)
			{
				impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);

				if (!(impure->vlu_flags & VLU_computed))
				{
					delete reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
					impure->vlu_flags |= VLU_computed;

					if (node->nod_type == nod_like)
					{
						impure->vlu_misc.vlu_invariant = evaluator =
							obj->createLikeMatcher(*tdbb->getDefaultPool(), p2, l2, escape_str, escape_length);
					}
					else	// nod_similar
					{
						impure->vlu_misc.vlu_invariant = evaluator =
							obj->createSimilarToMatcher(*tdbb->getDefaultPool(),
								p2, l2, escape_str, escape_length);
					}
				}
				else
				{
					evaluator = reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
					evaluator->reset();
				}
			}
			else if (node->nod_type == nod_like)
			{
				evaluator = obj->createLikeMatcher(*tdbb->getDefaultPool(),
					p2, l2, escape_str, escape_length);
			}
			else	// nod_similar
			{
				evaluator = obj->createSimilarToMatcher(*tdbb->getDefaultPool(),
					p2, l2, escape_str, escape_length);
			}

			while (!(blob->blb_flags & BLB_eof))
			{
				const SLONG l1 = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), false);
				if (!evaluator->process(buffer.begin(), l1))
					break;
			}

			ret_val = evaluator->result();

			if (!(node->nod_flags & nod_invariant))
				delete evaluator;
		}
		break;

	case nod_contains:
	case nod_starts:
		{
			PatternMatcher* evaluator;
			if (node->nod_flags & nod_invariant)
			{
				impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
				if (!(impure->vlu_flags & VLU_computed))
				{
					delete reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);

					if (node->nod_type == nod_contains)
					{
						impure->vlu_misc.vlu_invariant = evaluator =
							obj->createContainsMatcher(*tdbb->getDefaultPool(), p2, l2);
					}
					else	// nod_starts
					{
						impure->vlu_misc.vlu_invariant = evaluator =
							obj->createStartsMatcher(*tdbb->getDefaultPool(), p2, l2);
					}

					impure->vlu_flags |= VLU_computed;
				}
				else
				{
					evaluator = reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
					evaluator->reset();
				}
			}
			else
			{
				if (node->nod_type == nod_contains)
					evaluator = obj->createContainsMatcher(*tdbb->getDefaultPool(), p2, l2);
				else	// nod_starts
					evaluator = obj->createStartsMatcher(*tdbb->getDefaultPool(), p2, l2);
			}

			while (!(blob->blb_flags & BLB_eof))
			{
				const SLONG l1 = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), false);
				if (!evaluator->process(buffer.begin(), l1))
					break;
			}

			ret_val = evaluator->result();
			if (!(node->nod_flags & nod_invariant))
				delete evaluator;
		}
		break;
	}

	BLB_close(tdbb, blob);

	return ret_val;
}


static bool string_function(thread_db* tdbb,
							jrd_nod* node,
							SLONG l1, const UCHAR* p1,
							SLONG l2, const UCHAR* p2,
							USHORT ttype, bool computed_invariant)
{
/**************************************
 *
 *      s t r i n g _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *      Perform one of the pattern matching string functions.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();

	Collation* obj = INTL_texttype_lookup(tdbb, ttype);
	CharSet* charset = obj->getCharSet();

	// Handle contains and starts
	if (node->nod_type == nod_contains || node->nod_type == nod_starts)
	{
		if (node->nod_flags & nod_invariant)
		{
			impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
			PatternMatcher* evaluator;
			if (!(impure->vlu_flags & VLU_computed))
			{
				delete reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);

				if (node->nod_type == nod_contains)
				{
					impure->vlu_misc.vlu_invariant = evaluator =
						obj->createContainsMatcher(*tdbb->getDefaultPool(), p2, l2);
				}
				else	// nod_starts
				{
					impure->vlu_misc.vlu_invariant = evaluator =
						obj->createStartsMatcher(*tdbb->getDefaultPool(), p2, l2);
				}

				impure->vlu_flags |= VLU_computed;
			}
			else
			{
				evaluator = reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
				evaluator->reset();
			}

			evaluator->process(p1, l1);
			return evaluator->result();
		}

		if (node->nod_type == nod_contains)
			return obj->contains(*tdbb->getDefaultPool(), p1, l1, p2, l2);

		// nod_starts
		return obj->starts(*tdbb->getDefaultPool(), p1, l1, p2, l2);
	}

	// Handle LIKE and SIMILAR
	if (node->nod_type == nod_like || node->nod_type == nod_similar)
	{
		VaryStr<TEMP_LENGTH> temp3;
		const UCHAR* escape_str = NULL;
		USHORT escape_length = 0;
		/* ensure 3rd argument (escape char) is in operation text type */
		if (node->nod_count == 3 && !computed_invariant) {
			/* Convert ESCAPE to operation character set */
			DSC* dsc = EVL_expr(tdbb, node->nod_arg[2]);
			if (request->req_flags & req_null) {
				if (node->nod_flags & nod_invariant) {
					impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
					impure->vlu_flags |= VLU_computed;
					impure->vlu_flags |= VLU_null;
				}
				return false;
			}
			escape_length = MOV_make_string(dsc, ttype,
											reinterpret_cast<const char**>(&escape_str),
											&temp3, sizeof(temp3));
			if (!escape_length || charset->length(escape_length, escape_str, true) != 1)
			{
				/* If characters left, or null byte character, return error */
				ERR_post(Arg::Gds(isc_escape_invalid));
			}

			USHORT escape[2] = {0, 0};

			charset->getConvToUnicode().convert(escape_length, escape_str, sizeof(escape), escape);
			if (!escape[0])
			{
				/* If or null byte character, return error */
				ERR_post(Arg::Gds(isc_escape_invalid));
			}
		}

		if (node->nod_flags & nod_invariant)
		{
			impure_value* impure = (impure_value*) ((SCHAR *) request + node->nod_impure);
			PatternMatcher* evaluator;

			if (!(impure->vlu_flags & VLU_computed))
			{
				delete reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
				impure->vlu_flags |= VLU_computed;

				if (node->nod_type == nod_like)
				{
					impure->vlu_misc.vlu_invariant = evaluator = obj->createLikeMatcher(
						*tdbb->getDefaultPool(), p2, l2, escape_str, escape_length);
				}
				else	// nod_similar
				{
					impure->vlu_misc.vlu_invariant = evaluator = obj->createSimilarToMatcher(
						*tdbb->getDefaultPool(), p2, l2, escape_str, escape_length);
				}
			}
			else
			{
				evaluator = reinterpret_cast<PatternMatcher*>(impure->vlu_misc.vlu_invariant);
				evaluator->reset();
			}

			evaluator->process(p1, l1);

			return evaluator->result();
		}

		if (node->nod_type == nod_like)
			return obj->like(*tdbb->getDefaultPool(), p1, l1, p2, l2, escape_str, escape_length);

		// nod_similar
		return obj->similarTo(*tdbb->getDefaultPool(), p1, l1, p2, l2, escape_str, escape_length);
	}

	// Handle MATCHES
	return obj->matches(*tdbb->getDefaultPool(), p1, l1, p2, l2);
}


// *************************
// s t r i n g _ l e n g t h
// *************************
// Handles BIT_LENGTH(s), OCTET_LENGTH(s) and CHAR[ACTER]_LENGTH(s)
static dsc* string_length(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	const ULONG length_type = (IPTR) node->nod_arg[e_strlen_type];
	const dsc* value = EVL_expr(tdbb, node->nod_arg[e_strlen_value]);

	impure->vlu_desc.dsc_dtype = dtype_long;
	impure->vlu_desc.dsc_scale = 0;
	impure->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&impure->vlu_misc.vlu_long);
	impure->vlu_desc.dsc_length = sizeof(ULONG);

	jrd_req* request = tdbb->getRequest();
	// CVC: Borland used special signaling for nulls in outer joins.
	if (!value || (request->req_flags & req_null)) {
		request->req_flags |= req_null;
		impure->vlu_misc.vlu_long = 0;
		return &impure->vlu_desc;
	}

	ULONG length;

	if (value->isBlob())
	{
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
							 reinterpret_cast<bid*>(value->dsc_address));

		switch (length_type)
		{
			case blr_strlen_bit:
				{
					FB_UINT64 l = (FB_UINT64) blob->blb_length * 8;
					if (l > MAX_SINT64)
					{
						ERR_post(Arg::Gds(isc_arith_except) <<
								 Arg::Gds(isc_numeric_out_of_range));
					}

					length = l;
				}
				break;

			case blr_strlen_octet:
				length = blob->blb_length;
				break;

			case blr_strlen_char:
			{
				CharSet* charSet = INTL_charset_lookup(tdbb, value->dsc_blob_ttype());

				if (charSet->isMultiByte())
				{
					Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer;

					length = BLB_get_data(tdbb, blob, buffer.getBuffer(blob->blb_length),
										  blob->blb_length, false);
					length = charSet->length(length, buffer.begin(), true);
				}
				else
					length = blob->blb_length / charSet->maxBytesPerChar();

				break;
			}

			default:
				fb_assert(false);
				length = 0;
		}

		*(ULONG*)impure->vlu_desc.dsc_address = length;

		BLB_close(tdbb, blob);

		return &impure->vlu_desc;
	}

	VaryStr<32> temp;
	USHORT ttype;
	UCHAR* p;

	length = MOV_get_string_ptr(value, &ttype, &p, &temp, sizeof(temp));

	switch (length_type)
	{
		case blr_strlen_bit:
			{
				FB_UINT64 l = (FB_UINT64) length * 8;
				if (l > MAX_SINT64)
				{
					ERR_post(Arg::Gds(isc_arith_except) <<
							 Arg::Gds(isc_numeric_out_of_range));
				}

				length = l;
			}
			break;

		case blr_strlen_octet:
			break;

		case blr_strlen_char:
		{
			CharSet* charSet = INTL_charset_lookup(tdbb, ttype);
			length = charSet->length(length, p, true);
			break;
		}

		default:
			fb_assert(false);
			length = 0;
	}

	*(ULONG *) impure->vlu_desc.dsc_address = length;
	return &impure->vlu_desc;
}


static dsc* substring(thread_db* tdbb, impure_value* impure,
					  dsc* value, const dsc* offset_value, const dsc* length_value)
{
/**************************************
 *
 *      s u b s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Perform substring function.
 *
 **************************************/
	return SysFunction::substring(tdbb, impure, value, offset_value, length_value);
}


static dsc* trim(thread_db* tdbb, jrd_nod* node, impure_value* impure)
{
/**************************************
 *
 *      t r i m
 *
 **************************************
 *
 * Functional description
 *      Perform trim function = TRIM([where what FROM] string)
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	const ULONG specification = (IPTR) node->nod_arg[e_trim_specification];

	request->req_flags &= ~req_null;
	dsc* characters = (node->nod_arg[e_trim_characters] ? EVL_expr(tdbb, node->nod_arg[e_trim_characters]) : NULL);
	if (request->req_flags & req_null)
		return characters;

	request->req_flags &= ~req_null;
	dsc* value = EVL_expr(tdbb, node->nod_arg[e_trim_value]);
	if (request->req_flags & req_null)
		return value;

	USHORT ttype = INTL_TEXT_TYPE(*value);
	TextType* tt = INTL_texttype_lookup(tdbb, ttype);
	CharSet* cs = tt->getCharSet();

	const UCHAR* charactersAddress;
	MoveBuffer charactersBuffer;
	USHORT charactersLength;

	if (characters)
	{
		UCHAR* tempAddress = 0;
		charactersLength = MOV_make_string2(tdbb, characters, ttype, &tempAddress, charactersBuffer);
		charactersAddress = tempAddress;
	}
	else
	{
		charactersLength = tt->getCharSet()->getSpaceLength();
		charactersAddress = tt->getCharSet()->getSpace();
	}

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> charactersCanonical;
	charactersCanonical.getBuffer(charactersLength / tt->getCharSet()->minBytesPerChar() * tt->getCanonicalWidth());
	const SLONG charactersCanonicalLen = tt->canonical(charactersLength, charactersAddress,
		charactersCanonical.getCount(), charactersCanonical.begin()) * tt->getCanonicalWidth();

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> blobBuffer;
	MoveBuffer valueBuffer;
	UCHAR* valueAddress;
	ULONG valueLength;

	if (value->isBlob())
	{
		// Source string is a blob, things get interesting.
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address));

		// It's very difficult (and probably not very efficient) to trim a blob in chunks.
		// So go simple way and always read entire blob in memory.
		valueAddress = blobBuffer.getBuffer(blob->blb_length);
		valueLength = BLB_get_data(tdbb, blob, valueAddress, blob->blb_length, true);
	}
	else
		valueLength = MOV_make_string2(tdbb, value, ttype, &valueAddress, valueBuffer);

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> valueCanonical;
	valueCanonical.getBuffer(valueLength / cs->minBytesPerChar() * tt->getCanonicalWidth());
	const SLONG valueCanonicalLen = tt->canonical(valueLength, valueAddress,
		valueCanonical.getCount(), valueCanonical.begin()) * tt->getCanonicalWidth();

	SLONG offsetLead = 0;
	SLONG offsetTrail = valueCanonicalLen;

	// CVC: Avoid endless loop with zero length trim chars.
	if (charactersCanonicalLen)
	{
		if (specification == blr_trim_both || specification == blr_trim_leading)
		{
			// CVC: Prevent surprises with offsetLead < valueCanonicalLen; it may fail.
			for (; offsetLead + charactersCanonicalLen <= valueCanonicalLen; offsetLead += charactersCanonicalLen)
			{
				if (memcmp(charactersCanonical.begin(), &valueCanonical[offsetLead], charactersCanonicalLen) != 0)
					break;
			}
		}

		if (specification == blr_trim_both || specification == blr_trim_trailing)
		{
			for (; offsetTrail - charactersCanonicalLen >= offsetLead; offsetTrail -= charactersCanonicalLen)
			{
				if (memcmp(charactersCanonical.begin(), &valueCanonical[offsetTrail - charactersCanonicalLen],
					charactersCanonicalLen) != 0)
				{
					break;
				}
			}
		}
	}

	if (value->isBlob())
	{
		// We have valueCanonical already allocated.
		// Use it to get the substring that will be written to the new blob.
		const ULONG len = cs->substring(valueLength, valueAddress,
			valueCanonical.getCapacity(), valueCanonical.begin(),
			offsetLead / tt->getCanonicalWidth(),
			(offsetTrail - offsetLead) / tt->getCanonicalWidth());

		EVL_make_value(tdbb, value, impure);

		blb* newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction, &impure->vlu_misc.vlu_bid);

		BLB_put_data(tdbb, newBlob, valueCanonical.begin(), len);

		BLB_close(tdbb, newBlob);
	}
	else
	{
		dsc desc;
		desc.dsc_dtype = dtype_text;
		desc.dsc_sub_type = 0;
		desc.dsc_scale = 0;
		desc.dsc_length = valueLength;
		desc.dsc_address = NULL;
		INTL_ASSIGN_TTYPE(&desc, ttype);
		EVL_make_value(tdbb, &desc, impure);

		impure->vlu_desc.dsc_length = cs->substring(valueLength, valueAddress,
			impure->vlu_desc.dsc_length, impure->vlu_desc.dsc_address,
			offsetLead / tt->getCanonicalWidth(),
			(offsetTrail - offsetLead) / tt->getCanonicalWidth());
	}

	return &impure->vlu_desc;
}


static dsc* internal_info(thread_db* tdbb, const dsc* value, impure_value* impure)
{
/**************************************
 *
 *      i n t e r n a l _ i n f o
 *
 **************************************
 *
 * Functional description
 *      Return a given element
 *      of the internal engine data.
 *
 **************************************/
	const jrd_req* const request = tdbb->getRequest();

	fb_assert(value->dsc_dtype == dtype_long);
	const internal_info_id id = *reinterpret_cast<internal_info_id*>(value->dsc_address);

	if (id == internal_sqlstate)
	{
		char sqlstate[FB_SQLSTATE_SIZE];
		request->req_last_xcp.as_sqlstate(sqlstate);

		dsc desc;
		desc.makeText(FB_SQLSTATE_LENGTH, ttype_ascii, (UCHAR*) sqlstate);
		EVL_make_value(tdbb, &desc, impure);

		return &impure->vlu_desc;
	}

	SLONG result = 0;
	switch (id)
	{
	case internal_connection_id:
		result = PAG_attachment_id(tdbb);
		break;
	case internal_transaction_id:
		result = tdbb->getTransaction()->tra_number;
		break;
	case internal_gdscode:
		result = tdbb->getRequest()->req_last_xcp.as_gdscode();
		break;
	case internal_sqlcode:
		result = tdbb->getRequest()->req_last_xcp.as_sqlcode();
		break;
	case internal_rows_affected:
		result = tdbb->getRequest()->req_records_affected.getCount();
		break;
	case internal_trigger_action:
		result = tdbb->getRequest()->req_trigger_action;
		break;
	default:
		BUGCHECK(232);	/* msg 232 EVL_expr: invalid operation */
	}

	dsc desc;
	desc.makeLong(0, &result);
	EVL_make_value(tdbb, &desc, impure);

	return &impure->vlu_desc;
}
