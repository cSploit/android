/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		Optimizer.cpp
 *	DESCRIPTION:	Optimizer
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/rse.h"
#include "../jrd/ods.h"
#include "../jrd/Optimizer.h"

#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"

namespace Jrd {

class AutoActivateResetStreams : public Firebird::AutoStorage
{
public:
	AutoActivateResetStreams(CompilerScratch* csb, const RecordSelExpr* rse)
		: m_csb(csb), m_streams(getPool()), m_flags(getPool())
	{
		collect(rse);

		for (size_t i = 0; i < m_streams.getCount(); i++)
		{
			const UCHAR stream = m_streams[i];
			m_csb->csb_rpt[stream].csb_flags |= (csb_active | csb_sub_stream);
		}
	}

	~AutoActivateResetStreams()
	{
		for (size_t i = 0; i < m_streams.getCount(); i++)
		{
			const UCHAR stream = m_streams[i];
			m_csb->csb_rpt[stream].csb_flags = m_flags[i];
		}
	}

private:
	CompilerScratch* m_csb;
	Firebird::HalfStaticArray<UCHAR, OPT_STATIC_ITEMS> m_streams;
	Firebird::HalfStaticArray<USHORT, OPT_STATIC_ITEMS> m_flags;

	void collect(const RecordSelExpr* rse)
	{
		const jrd_nod* const* ptr = rse->rse_relation;
		for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
		{
			const jrd_nod* const node = *ptr;
			if (node->nod_type != nod_rse)
			{
				const SSHORT stream = (USHORT)(IPTR) node->nod_arg[STREAM_INDEX(node)];
				if (!m_streams.exist(stream))
				{
					m_streams.add(stream);
					m_flags.add(m_csb->csb_rpt[stream].csb_flags);
				}
			}
			else
			{
				collect((const RecordSelExpr*) node);
			}
		}
	}
};


bool OPT_computable(CompilerScratch* csb, const jrd_nod* node, SSHORT stream,
				const bool idx_use, const bool allowOnlyCurrentStream)
{
/**************************************
 *
 *	c o m p u t a b l e
 *
 **************************************
 *
 * Functional description
 *	See if node is presently computable.
 *	Note that a field is not computable
 *	with respect to its own stream.
 *
 * There are two different uses of OPT_computable().
 * (a) idx_use == false: when an unused conjunct is to be picked for making
 *     into a boolean and in making a db_key.
 *     In this case, a node is said to be computable, if all the streams
 *     involved in that node are csb_active. The csb_active flag
 *     defines all the streams available in the current scope of the
 *     query.
 * (b) idx_use == true: to determine if we can use an
 *     index on the conjunct we have already chosen.
 *     In order to use an index on a conjunct, it is required that the
 *     all the streams involved in the conjunct are currently active
 *     or have been already processed before and made into rivers.
 *     Because, here we want to differentiate between streams we have
 *     not yet worked on and those that we have worked on or are currently
 *     working on.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	if (node->nod_flags & nod_deoptimize) {
		return false;
	}

	// Recurse thru interesting sub-nodes

	switch (node->nod_type)
	{
	case nod_procedure:
		{
			const jrd_nod* const inputs = node->nod_arg[e_prc_inputs];
			if (inputs)
			{
				fb_assert(inputs->nod_type == nod_asn_list);
				const jrd_nod* const* ptr = inputs->nod_arg;
				for (const jrd_nod* const* const end = ptr + inputs->nod_count; ptr < end; ptr++)
				{
					if (!OPT_computable(csb, *ptr, stream, idx_use, allowOnlyCurrentStream)) {
						return false;
					}
				}
			}
		}
		break;
	case nod_union:
		{
			const jrd_nod* const clauses = node->nod_arg[e_uni_clauses];
			const jrd_nod* const* ptr = clauses->nod_arg;
			for (const jrd_nod* const* const end = ptr + clauses->nod_count; ptr < end; ptr += 2)
			{
				if (!OPT_computable(csb, *ptr, stream, idx_use, allowOnlyCurrentStream)) {
					return false;
				}
			}
		}
		break;
	default:
		{
			const jrd_nod* const* ptr = node->nod_arg;
			for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
			{
				if (!OPT_computable(csb, *ptr, stream, idx_use, allowOnlyCurrentStream)) {
					return false;
				}
			}
		}
	}

	RecordSelExpr* rse;
	const jrd_nod* sub;
	const jrd_nod* value;
	USHORT n;

	switch (node->nod_type)
	{
	case nod_field:
		n = (USHORT)(IPTR) node->nod_arg[e_fld_stream];
		if (allowOnlyCurrentStream)
		{
			if (n != stream && !(csb->csb_rpt[n].csb_flags & csb_sub_stream))
			{
				return false;
			}
		}
		else
		{
			if (n == stream) {
				return false;
			}
		}
		return csb->csb_rpt[n].csb_flags & csb_active;

	case nod_derived_expr:
		{
			const jrd_nod* const exprNode = node->nod_arg[e_derived_expr_expr];
			const UCHAR streamCount = (UCHAR)(IPTR) node->nod_arg[e_derived_expr_stream_count];
			const USHORT* const streamList = (USHORT*) node->nod_arg[e_derived_expr_stream_list];

			Firebird::SortedArray<int> expressionStreams;
			OPT_get_expression_streams(exprNode, expressionStreams);

			for (unsigned i = 0; i < streamCount; ++i)
			{
				n = streamList[i];

				if (expressionStreams.exist(n))
				{
					// We've already checked computability of the expression,
					// so any stream it refers to is known to be active
					continue;
				}

				if (allowOnlyCurrentStream)
				{
					if (n != stream && !(csb->csb_rpt[n].csb_flags & csb_sub_stream))
						return false;
				}
				else
				{
					if (n == stream)
						return false;
				}

				if (!(csb->csb_rpt[n].csb_flags & csb_active))
					return false;
			}
		}
		return true;

	case nod_rec_version:
	case nod_dbkey:
		n = (USHORT)(IPTR) node->nod_arg[0];
		if (allowOnlyCurrentStream)
		{
			if (n != stream && !(csb->csb_rpt[n].csb_flags & csb_sub_stream))
				return false;
		}
		else
		{
			if (n == stream)
				return false;
		}
		return csb->csb_rpt[n].csb_flags & csb_active;

	case nod_min:
	case nod_max:
	case nod_average:
	case nod_total:
	case nod_count:
	case nod_from:
		if ((sub = node->nod_arg[e_stat_default]) &&
			!OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream))
		{
			return false;
		}
		rse = (RecordSelExpr*) node->nod_arg[e_stat_rse];
		value = node->nod_arg[e_stat_value];
		break;

	case nod_rse:
		rse = (RecordSelExpr*) node;
		value = NULL;
		break;

	case nod_aggregate:
		rse = (RecordSelExpr*) node->nod_arg[e_agg_rse];
		rse->rse_sorted = node->nod_arg[e_agg_group];
		value = NULL;
		break;

	default:
		return true;
	}

	// Node is a record selection expression

	if ((sub = rse->rse_first) && !OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream)) {
		return false;
	}

    if ((sub = rse->rse_skip) && !OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream)) {
        return false;
	}

	// Set sub-streams of rse active
	AutoActivateResetStreams activator(csb, rse);

	// Check sub-stream
	if (((sub = rse->rse_boolean)    && !OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream)) ||
	    ((sub = rse->rse_sorted)     && !OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream)) ||
	    ((sub = rse->rse_projection) && !OPT_computable(csb, sub, stream, idx_use, allowOnlyCurrentStream)))
	{
		return false;
	}

	const jrd_nod* const* ptr;
	const jrd_nod* const* end;

	for (ptr = rse->rse_relation, end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		if (!OPT_computable(csb, (*ptr), stream, idx_use, allowOnlyCurrentStream))
		{
			return false;
		}
	}

	// Check value expression, if any
	if (value && !OPT_computable(csb, value, stream, idx_use, allowOnlyCurrentStream))
	{
		return false;
	}

	return true;
}


void OPT_compute_rse_streams(const RecordSelExpr* rse, UCHAR* streams)
{
/**************************************
 *
 *	c o m p u t e _ r s e _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *	Identify the streams that make up an RecordSelExpr.
 *
 **************************************/
	DEV_BLKCHK(rse, type_nod);

	const jrd_nod* const* ptr = rse->rse_relation;
	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		const jrd_nod* node = *ptr;
		if (node->nod_type != nod_rse)
		{
			fb_assert(streams[0] < MAX_STREAMS && streams[0] < MAX_UCHAR);
			streams[++streams[0]] = (UCHAR)(IPTR) node->nod_arg[STREAM_INDEX(node)];
		}
		else {
			OPT_compute_rse_streams((const RecordSelExpr*) node, streams);
		}
	}
}


bool OPT_expression_equal(thread_db* tdbb, OptimizerBlk* opt,
							 const index_desc* idx, jrd_nod* node,
							 USHORT stream)
{
/**************************************
 *
 *      e x p r e s s i o n _ e q u a l
 *
 **************************************
 *
 * Functional description
 *  Wrapper for OPT_expression_equal2().
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	SET_TDBB(tdbb);

	if (idx && idx->idx_expression_request && idx->idx_expression)
	{
		fb_assert(idx->idx_flags & idx_expressn);

		jrd_req* org_request = tdbb->getRequest();
		jrd_req* expr_request = EXE_find_request(tdbb, idx->idx_expression_request, false);

		fb_assert(expr_request->req_caller == NULL);
		expr_request->req_caller = org_request;
		tdbb->setRequest(expr_request);

		bool result = false;

		try
		{
			Jrd::ContextPoolHolder context(tdbb, expr_request->req_pool);

			expr_request->req_timestamp = expr_request->req_caller ?
				expr_request->req_caller->req_timestamp : Firebird::TimeStamp::getCurrentTimeStamp();

			result = OPT_expression_equal2(tdbb, opt, idx->idx_expression, node, stream);
		}
		catch (const Firebird::Exception&)
		{
			tdbb->setRequest(org_request);
			expr_request->req_caller = NULL;
			expr_request->req_flags &= ~req_in_use;
			expr_request->req_timestamp.invalidate();

			throw;
		}

		tdbb->setRequest(org_request);
		expr_request->req_caller = NULL;
		expr_request->req_flags &= ~req_in_use;
		expr_request->req_timestamp.invalidate();

		return result;
	}

	return false;
}


bool OPT_expression_equal2(thread_db* tdbb, OptimizerBlk* opt,
							  jrd_nod* node1, jrd_nod* node2,
							  USHORT stream)
{
/**************************************
 *
 *      e x p r e s s i o n _ e q u a l 2
 *
 **************************************
 *
 * Functional description
 *      Determine if two expression trees are the same for
 *	the purposes of matching one half of a boolean expression
 *	to an index.
 *
 **************************************/
	DEV_BLKCHK(node1, type_nod);
	DEV_BLKCHK(node2, type_nod);

	SET_TDBB(tdbb);

	if (!node1 || !node2)
	{
		BUGCHECK(303);	// msg 303 Invalid expression for evaluation.
	}

	if (node1->nod_type != node2->nod_type)
	{
		dsc dsc1, dsc2;
		dsc *desc1 = &dsc1, *desc2 = &dsc2;

		if (node1->nod_type == nod_cast && node2->nod_type == nod_field)
		{
			CMP_get_desc(tdbb, opt->opt_csb, node1, desc1);
			CMP_get_desc(tdbb, opt->opt_csb, node2, desc2);

			if (DSC_EQUIV(desc1, desc2, true) &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_cast_source], node2, stream))
			{
				return true;
			}
		}

		if (node1->nod_type == nod_field && node2->nod_type == nod_cast)
		{
			CMP_get_desc(tdbb, opt->opt_csb, node1, desc1);
			CMP_get_desc(tdbb, opt->opt_csb, node2, desc2);

			if (DSC_EQUIV(desc1, desc2, true) &&
				OPT_expression_equal2(tdbb, opt, node1, node2->nod_arg[e_cast_source], stream))
			{
				return true;
			}
		}

		return false;
	}

	switch (node1->nod_type)
	{
		case nod_add:
		case nod_multiply:
		case nod_add2:
		case nod_multiply2:
		case nod_equiv:
	    case nod_eql:
		case nod_neq:
	    case nod_and:
		case nod_or:
			// A+B is equivalent to B+A, ditto A*B==B*A
			// Note: If one expression is A+B+C, but the other is B+C+A we won't
			// necessarily match them.
			if (OPT_expression_equal2(tdbb, opt, node1->nod_arg[0], node2->nod_arg[1], stream) &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[1], node2->nod_arg[0], stream))
			{
				return true;
			}
			// Fall into ...
		case nod_subtract:
		case nod_divide:
		case nod_subtract2:
		case nod_divide2:
		case nod_concatenate:

		// TODO match A > B to B <= A, etc
	    case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
			if (OPT_expression_equal2(tdbb, opt, node1->nod_arg[0], node2->nod_arg[0], stream) &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[1], node2->nod_arg[1], stream))
			{
				return true;
			}
			break;

		case nod_rec_version:
		case nod_dbkey:
			if (node1->nod_arg[0] == node2->nod_arg[0])
			{
				return true;
			}
			break;

		case nod_field:
			{
				const USHORT fld_stream = (USHORT)(IPTR) node2->nod_arg[e_fld_stream];
				if ((node1->nod_arg[e_fld_id] == node2->nod_arg[e_fld_id]) && fld_stream == stream)
				{
					return true;
				}
			}
			break;

		case nod_function:
			if (node1->nod_arg[e_fun_function] &&
				(node1->nod_arg[e_fun_function] == node2->nod_arg[e_fun_function]) &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_fun_args],
									  node2->nod_arg[e_fun_args], stream))
			{
				return true;
			}
			break;

		case nod_sys_function:
			if (node1->nod_arg[e_sysfun_function] &&
				(node1->nod_arg[e_sysfun_function] == node2->nod_arg[e_sysfun_function]) &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_sysfun_args],
									  node2->nod_arg[e_sysfun_args], stream))
			{
				return true;
			}
			break;

		case nod_literal:
			{
				const dsc* desc1 = EVL_expr(tdbb, node1);
				const dsc* desc2 = EVL_expr(tdbb, node2);
				if (desc1 && desc2 && !MOV_compare(desc1, desc2))
				{
					return true;
				}
			}
			break;

		case nod_null:
		case nod_user_name:
		case nod_current_role:
		case nod_current_time:
		case nod_current_date:
		case nod_current_timestamp:
			return true;

		case nod_between:
		case nod_like:
		case nod_similar:
		case nod_missing:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_not:
		case nod_unique:

		case nod_value_if:
		case nod_substr:
		case nod_trim:
			{
				if (node1->nod_count != node2->nod_count)
				{
					return false;
				}
				for (int i = 0; i < node1->nod_count; ++i)
				{
					if (!OPT_expression_equal2(tdbb, opt, node1->nod_arg[i], node2->nod_arg[i], stream))
					{
						return false;
					}
				}
				return true;
			}
			break;

		case nod_gen_id:
		case nod_gen_id2:
			if (node1->nod_arg[e_gen_id] == node2->nod_arg[e_gen_id])
			{
				return true;
			}
			break;

		case nod_negate:
		case nod_internal_info:
			if (OPT_expression_equal2(tdbb, opt, node1->nod_arg[0], node2->nod_arg[0], stream))
			{
				return true;
			}
			break;

		case nod_upcase:
		case nod_lowcase:
			if (OPT_expression_equal2(tdbb, opt, node1->nod_arg[0], node2->nod_arg[0], stream))
			{
				return true;
			}
			break;

		case nod_cast:
			{
				dsc dsc1, dsc2;
				dsc *desc1 = &dsc1, *desc2 = &dsc2;

				CMP_get_desc(tdbb, opt->opt_csb, node1, desc1);
				CMP_get_desc(tdbb, opt->opt_csb, node2, desc2);

				if (DSC_EQUIV(desc1, desc2, true) &&
					OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_cast_source],
										  node2->nod_arg[e_cast_source], stream))
				{
					return true;
				}
			}
			break;

		case nod_extract:
			if (node1->nod_arg[e_extract_part] == node2->nod_arg[e_extract_part] &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_extract_value],
									  node2->nod_arg[e_extract_value], stream))
			{
				return true;
			}
			break;

		case nod_strlen:
			if (node1->nod_arg[e_strlen_type] == node2->nod_arg[e_strlen_type] &&
				OPT_expression_equal2(tdbb, opt, node1->nod_arg[e_strlen_value],
									  node2->nod_arg[e_strlen_value], stream))
			{
				return true;
			}
			break;

	    case nod_list:
			{
				jrd_nod** ptr1 = node1->nod_arg;
				jrd_nod** ptr2 = node2->nod_arg;

				if (node1->nod_count != node2->nod_count)
				{
					return false;
				}

				ULONG count = node1->nod_count;

				while (count--)
				{
					if (!OPT_expression_equal2(tdbb, opt, *ptr1++, *ptr2++, stream))
					{
						return false;
					}
				}

				return true;
		    }

		// AB: New nodes has to be added

		default:
			break;
	}

	return false;
}


void OPT_get_expression_streams(const jrd_nod* node, Firebird::SortedArray<int>& streams)
{
/**************************************
 *
 *  g e t _ e x p r e s s i o n _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *  Return all streams referenced by the expression.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	if (!node) {
		return;
	}

	RecordSelExpr* rse = NULL;

	int n;
	size_t pos;

	switch (node->nod_type)
	{

		case nod_field:
			n = (int)(IPTR) node->nod_arg[e_fld_stream];
			if (!streams.find(n, pos))
				streams.add(n);
			break;

		case nod_rec_version:
		case nod_dbkey:
			n = (int)(IPTR) node->nod_arg[0];
			if (!streams.find(n, pos))
				streams.add(n);
			break;

		case nod_cast:
			OPT_get_expression_streams(node->nod_arg[e_cast_source], streams);
			break;

		case nod_extract:
			OPT_get_expression_streams(node->nod_arg[e_extract_value], streams);
			break;

		case nod_strlen:
			OPT_get_expression_streams(node->nod_arg[e_strlen_value], streams);
			break;

		case nod_function:
			OPT_get_expression_streams(node->nod_arg[e_fun_args], streams);
			break;

		case nod_procedure:
			OPT_get_expression_streams(node->nod_arg[e_prc_inputs], streams);
			break;

		case nod_any:
		case nod_unique:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_exists:
			OPT_get_expression_streams(node->nod_arg[e_any_rse], streams);
			break;

		case nod_argument:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_internal_info:
		case nod_literal:
		case nod_null:
		case nod_user_name:
		case nod_variable:
			break;

		case nod_rse:
			rse = (RecordSelExpr*) node;
			break;

		case nod_average:
		case nod_count:
		//case nod_count2:
		case nod_from:
		case nod_max:
		case nod_min:
		case nod_total:
			OPT_get_expression_streams(node->nod_arg[e_stat_rse], streams);
			OPT_get_expression_streams(node->nod_arg[e_stat_value], streams);
			break;

		// go into the node arguments
		default:
		{
			const jrd_nod* const* ptr = node->nod_arg;
			// Check all sub-nodes of this node.
			for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
			{
				OPT_get_expression_streams(*ptr, streams);
			}
			break;
		}
	}

	if (rse) {
		OPT_get_expression_streams(rse->rse_first, streams);
		OPT_get_expression_streams(rse->rse_skip, streams);
		OPT_get_expression_streams(rse->rse_boolean, streams);
		OPT_get_expression_streams(rse->rse_sorted, streams);
		OPT_get_expression_streams(rse->rse_projection, streams);

		stream_array_t temp_streams;
		temp_streams[0] = 0;
		OPT_compute_rse_streams(rse, temp_streams);

		for (UCHAR i = 1; i <= temp_streams[0]; i++)
		{
			n = temp_streams[i];

			if (!streams.find(n, pos))
				streams.add(n);
		}
	}
}


double OPT_getRelationCardinality(thread_db* tdbb, jrd_rel* relation, const Format* format)
{
/**************************************
 *
 *	g e t R e l a t i o n C a r d i n a l i t y
 *
 **************************************
 *
 * Functional description
 *	Return the estimated cardinality for
 *  the given relation.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (relation->isVirtual())
	{
		// Just a dumb estimation
		return (double) 100;
	}

	if (relation->rel_file)
	{
		// Is there really no way to do better?
		// Don't we know the file-size and record-size?
		return (double) 10000;
	}

	MET_post_existence(tdbb, relation);
	const double cardinality = DPM_cardinality(tdbb, relation, format);
	MET_release_existence(tdbb, relation);
	return cardinality;
}


jrd_nod* OPT_make_binary_node(nod_t type, jrd_nod* arg1, jrd_nod* arg2, bool flag)
{
/**************************************
 *
 *	m a k e _ b i n a r y _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Make a binary node.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	DEV_BLKCHK(arg1, type_nod);
	DEV_BLKCHK(arg2, type_nod);
	jrd_nod* node = PAR_make_node(tdbb, 2);
	node->nod_type = type;
	node->nod_arg[0] = arg1;
	node->nod_arg[1] = arg2;
	if (flag) {
		node->nod_flags |= nod_comparison;
	}
	return node;
}


VaryingString* OPT_make_alias(thread_db* tdbb, const CompilerScratch* csb,
				const CompilerScratch::csb_repeat* base_tail)
{
/**************************************
 *
 *	m a k e _ a l i a s
 *
 **************************************
 *
 * Functional description
 *	Make an alias string suitable for printing
 *	as part of the plan.  For views, this means
 *	multiple aliases to distinguish the base
 *	table.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	SET_TDBB(tdbb);
	if (!base_tail->csb_view && !base_tail->csb_alias)
		return NULL;

	const CompilerScratch::csb_repeat* csb_tail;
	// calculate the length of the alias by going up through
	// the view stack to find the lengths of all aliases;
	// adjust for spaces and a null terminator
	USHORT alias_length = 0;
	for (csb_tail = base_tail; true; csb_tail = &csb->csb_rpt[csb_tail->csb_view_stream])
	{
		if (csb_tail->csb_alias)
			alias_length += csb_tail->csb_alias->length();
		else {
			alias_length += (csb_tail->csb_relation ? csb_tail->csb_relation->rel_name.length() : 0);
		}
		alias_length++;
		if (!csb_tail->csb_view)
			break;
	}

	// allocate a string block to hold the concatenated alias
	VaryingString* alias = FB_NEW_RPT(*tdbb->getDefaultPool(), alias_length) VaryingString();
	alias->str_length = alias_length - 1;
	// now concatenate the individual aliases into the string block,
	// beginning at the end and copying back to the beginning
	TEXT* p = (TEXT *) alias->str_data + alias->str_length;
	*p-- = 0;
	for (csb_tail = base_tail; true; csb_tail = &csb->csb_rpt[csb_tail->csb_view_stream])
	{
		const TEXT* q;
		if (csb_tail->csb_alias)
			q = (TEXT *) csb_tail->csb_alias->c_str();
		else
		{
			q = (csb_tail->csb_relation && csb_tail->csb_relation->rel_name.length() ?
				csb_tail->csb_relation->rel_name.c_str() : NULL);
		}
		// go to the end of the alias and copy it backwards
		if (q)
		{
			for (alias_length = 0; *q; alias_length++)
				q++;
			while (alias_length--)
				*p-- = *--q;
		}

		if (!csb_tail->csb_view)
			break;
		*p-- = ' ';
	}

	return alias;
}


USHORT OPT_nav_rsb_size(RecordSource* rsb, USHORT key_length, USHORT size)
{
/**************************************
 *
 *	n a v _ r s b _ s i z e
 *
 **************************************
 *
 * Functional description
 *	Calculate the size of a navigational rsb.
 *
 **************************************/
	DEV_BLKCHK(rsb, type_rsb);
#ifdef SCROLLABLE_CURSORS
	// allocate extra impure area to hold the current key,
	// plus an upper and lower bound key value, for a total
	// of three times the key length for the index
	size += sizeof(struct irsb_nav) + 3 * key_length;
#else
	size += sizeof(struct irsb_nav) + 2 * key_length;
#endif
	size = FB_ALIGN(size, FB_ALIGNMENT);
	// make room for an idx structure to describe the index
	// that was used to generate this rsb
	if (rsb->rsb_type == rsb_navigate)
		rsb->rsb_arg[RSB_NAV_idx_offset] = (RecordSource*) (IPTR) size;
	size += sizeof(index_desc);
	return size;
}


IndexScratchSegment::IndexScratchSegment(MemoryPool& p) :
	matches(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h S e g m e n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	lowerValue = NULL;
	upperValue = NULL;
	excludeLower = false;
	excludeUpper = false;
	scope = 0;
	scanType = segmentScanNone;
}

IndexScratchSegment::IndexScratchSegment(MemoryPool& p, IndexScratchSegment* segment) :
	matches(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h S e g m e n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	lowerValue = segment->lowerValue;
	upperValue = segment->upperValue;
	excludeLower = segment->excludeLower;
	excludeUpper = segment->excludeUpper;
	scope = segment->scope;
	scanType = segment->scanType;

	for (size_t i = 0; i < segment->matches.getCount(); i++) {
		matches.add(segment->matches[i]);
	}
}

IndexScratch::IndexScratch(MemoryPool& p, thread_db* tdbb, index_desc* ix,
	CompilerScratch::csb_repeat* csb_tail) :
	idx(ix), segments(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	// Allocate needed segments
	selectivity = MAXIMUM_SELECTIVITY;
	candidate = false;
	scopeCandidate = false;
	lowerCount = 0;
	upperCount = 0;
	nonFullMatchedSegments = 0;
	fuzzy = false;

	segments.grow(idx->idx_count);

	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		segment[i] = FB_NEW(p) IndexScratchSegment(p);
	}

	const int length = ROUNDUP(BTR_key_length(tdbb, csb_tail->csb_relation, idx), sizeof(SLONG));

	// AB: Calculate the cardinality which should reflect the total number
	// of index pages for this index.
	// We assume that the average index-key can be compressed by a factor 0.5
	// In the future the average key-length should be stored and retrieved
	// from a system table (RDB$INDICES for example).
	// Multipling the selectivity with this cardinality gives the estimated
	// number of index pages that are read for the index retrieval.
	double factor = 0.5;
	if (segments.getCount() >= 2)
	{
		// Compound indexes are generally less compressed.
		factor = 0.7;
	}
	Database* dbb = tdbb->getDatabase();
	cardinality = (csb_tail->csb_cardinality * (2 + (length * factor))) / (dbb->dbb_page_size - BTR_SIZE);
	cardinality = MAX(cardinality, MINIMUM_CARDINALITY);
}

IndexScratch::IndexScratch(MemoryPool& p, const IndexScratch& scratch) :
	segments(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	selectivity = scratch.selectivity;
	cardinality = scratch.cardinality;
	candidate = scratch.candidate;
	scopeCandidate = scratch.scopeCandidate;
	lowerCount = scratch.lowerCount;
	upperCount = scratch.upperCount;
	nonFullMatchedSegments = scratch.nonFullMatchedSegments;
	fuzzy = scratch.fuzzy;
	idx = scratch.idx;

	// Allocate needed segments
	segments.grow(scratch.segments.getCount());

	IndexScratchSegment* const* scratchSegment = scratch.segments.begin();
	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		segment[i] = FB_NEW(p) IndexScratchSegment(p, scratchSegment[i]);
	}
}

IndexScratch::~IndexScratch()
{
/**************************************
 *
 *	~I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		delete segment[i];
	}
}

InversionCandidate::InversionCandidate(MemoryPool& p) :
	matches(p), dependentFromStreams(p)
{
/**************************************
 *
 *	I n v e r s i o n C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	selectivity = MAXIMUM_SELECTIVITY;
	cost = 0;
	indexes = 0;
	dependencies = 0;
	nonFullMatchedSegments = MAX_INDEX_SEGMENTS + 1;
	matchedSegments = 0;
	boolean = NULL;
	inversion = NULL;
	scratch = NULL;
	used = false;
	unique = false;
}

OptimizerRetrieval::OptimizerRetrieval(MemoryPool& p, OptimizerBlk* opt,
									   SSHORT streamNumber, bool outer,
									   bool inner, jrd_nod** sortNode) :
	pool(p), indexScratches(p), inversionCandidates(p)
{
/**************************************
 *
 *	O p t i m i z e r R e t r i e v a l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	tdbb = NULL;
	createIndexScanNodes = false;
	alias = NULL;
	setConjunctionsMatched = false;

	SET_TDBB(tdbb);
	this->database = tdbb->getDatabase();
	this->stream = streamNumber;
	this->optimizer = opt;
	this->csb = this->optimizer->opt_csb;
	this->innerFlag = inner;
	this->outerFlag = outer;
	this->sort = sortNode;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[this->stream];
	relation = csb_tail->csb_relation;

	// Allocate needed indexScratches

	index_desc* idx = csb_tail->csb_idx->items;
	for (int i = 0; i < csb_tail->csb_indices; ++i, ++idx) {
		indexScratches.add( IndexScratch(p, tdbb, idx, csb_tail) );
	}

	inversionCandidates.shrink(0);
}

OptimizerRetrieval::~OptimizerRetrieval()
{
/**************************************
 *
 *	~O p t i m i z e r R e t r i e v a l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	for (size_t i = 0; i < inversionCandidates.getCount(); ++i) {
		delete inversionCandidates[i];
	}
}

jrd_nod* OptimizerRetrieval::composeInversion(jrd_nod* node1, jrd_nod* node2,
											  nod_t node_type) const
{
/**************************************
 *
 *	c o m p o s e I n v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Melt two inversions together by the
 *  type given in node_type.
 *
 **************************************/

	if (!node2) {
		return node1;
	}

	if (!node1) {
		return node2;
	}

	if (node_type == nod_bit_or)
	{
		if ((node1->nod_type == nod_index) &&
			(node2->nod_type == nod_index) &&
			(reinterpret_cast<IndexRetrieval*>(node1->nod_arg[e_idx_retrieval])->irb_index ==
				reinterpret_cast<IndexRetrieval*>(node2->nod_arg[e_idx_retrieval])->irb_index))
		{
			node_type = nod_bit_in;
		}
		else if ((node1->nod_type == nod_bit_in) &&
			(node2->nod_type == nod_index) &&
			(reinterpret_cast<IndexRetrieval*>(node1->nod_arg[1]->nod_arg[e_idx_retrieval])->irb_index ==
				reinterpret_cast<IndexRetrieval*>(node2->nod_arg[e_idx_retrieval])->irb_index))
		{
			node_type = nod_bit_in;
		}
	}

	return OPT_make_binary_node(node_type, node1, node2, false);
}

void OptimizerRetrieval::findDependentFromStreams(const jrd_nod* node,
	SortedStreamList* streamList) const
{
/**************************************
 *
 *	f i n d D e p e n d e n t F r o m S t r e a m s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	// Recurse thru interesting sub-nodes

	if (node->nod_type == nod_procedure)
	{
		const jrd_nod* const inputs = node->nod_arg[e_prc_inputs];
		if (inputs)
		{
			fb_assert(inputs->nod_type == nod_asn_list);
			const jrd_nod* const* ptr = inputs->nod_arg;
			for (const jrd_nod* const* const end = ptr + inputs->nod_count; ptr < end; ptr++)
			{
				findDependentFromStreams(*ptr, streamList);
			}
		}
	}
	else if (node->nod_type == nod_union)
	{
		const jrd_nod* const clauses = node->nod_arg[e_uni_clauses];
		const jrd_nod* const* ptr = clauses->nod_arg;
		for (const jrd_nod* const* const end = ptr + clauses->nod_count; ptr < end; ptr += 2)
		{
			findDependentFromStreams(*ptr, streamList);
		}
	}
	else
	{
		const jrd_nod* const* ptr = node->nod_arg;
		for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
		{
			findDependentFromStreams(*ptr, streamList);
		}
	}

	RecordSelExpr* rse;
	jrd_nod* sub;
	jrd_nod* value;

	switch (node->nod_type)
	{
		case nod_field:
		{
			int fieldStream = (USHORT)(IPTR) node->nod_arg[e_fld_stream];
			// dimitr: OLD/NEW contexts shouldn't create any stream dependencies
			if (fieldStream != stream &&
				(csb->csb_rpt[fieldStream].csb_flags & csb_active) &&
				!(csb->csb_rpt[fieldStream].csb_flags & csb_trigger))
			{
				if (!streamList->exist(fieldStream)) {
					streamList->add(fieldStream);
				}
			}
			return;
		}

		case nod_rec_version:
		case nod_dbkey:
		{
			const int keyStream = (USHORT)(IPTR) node->nod_arg[0];
			if (keyStream != stream && (csb->csb_rpt[keyStream].csb_flags & csb_active))
			{
				if (!streamList->exist(keyStream))
					streamList->add(keyStream);
			}
			return;
		}

		case nod_derived_expr:
		{
			const UCHAR count = (UCHAR)(IPTR) node->nod_arg[e_derived_expr_stream_count];
			const USHORT* list = (USHORT*) node->nod_arg[e_derived_expr_stream_list];

			for (unsigned i = 0; i < count; ++i)
			{
				int derivedStream = list[i];

				if (derivedStream != stream && (csb->csb_rpt[derivedStream].csb_flags & csb_active))
				{
					if (!streamList->exist(derivedStream))
						streamList->add(derivedStream);
				}
			}

			return;
		}

		case nod_min:
		case nod_max:
		case nod_average:
		case nod_total:
		case nod_count:
		case nod_from:
			if (sub = node->nod_arg[e_stat_default]) {
				findDependentFromStreams(sub, streamList);
			}
			rse = (RecordSelExpr*) node->nod_arg[e_stat_rse];
			value = node->nod_arg[e_stat_value];
			break;

		case nod_rse:
			rse = (RecordSelExpr*) node;
			value = NULL;
			break;

		case nod_aggregate:
			rse = (RecordSelExpr*) node->nod_arg[e_agg_rse];
			rse->rse_sorted = node->nod_arg[e_agg_group];
			value = NULL;
			break;

		default:
			return;
	}

	// Node is a record selection expression.
	if (sub = rse->rse_first) {
		findDependentFromStreams(sub, streamList);
	}

    if (sub = rse->rse_skip) {
        findDependentFromStreams(sub, streamList);
	}

	if (sub = rse->rse_boolean) {
		findDependentFromStreams(sub, streamList);
	}

	if (sub = rse->rse_sorted) {
		findDependentFromStreams(sub, streamList);
	}

	if (sub = rse->rse_projection) {
		findDependentFromStreams(sub, streamList);
	}

	const jrd_nod* const* ptr;
	const jrd_nod* const* end;
	for (ptr = rse->rse_relation, end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		findDependentFromStreams(*ptr, streamList);
	}

	// Check value expression, if any
	if (value) {
		findDependentFromStreams(value, streamList);
	}

	return;
}

VaryingString* OptimizerRetrieval::getAlias()
{
/**************************************
 *
 *	g e t A l i a s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (!alias)
	{
		const CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[this->stream];
		alias = OPT_make_alias(tdbb, csb, csb_tail);
	}
	return alias;
}

InversionCandidate* OptimizerRetrieval::generateInversion(RecordSource** rsb)
{
/**************************************
 *
 *	g e n e r a t e I n v e r s i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (!relation || relation->rel_file || relation->isVirtual()) {
		return NULL;
	}

	OptimizerBlk::opt_conjunct* const opt_begin =
		optimizer->opt_conjuncts.begin() + (outerFlag ? optimizer->opt_base_parent_conjuncts : 0);

	const OptimizerBlk::opt_conjunct* const opt_end =
		innerFlag ? optimizer->opt_conjuncts.begin() + optimizer->opt_base_missing_conjuncts :
					optimizer->opt_conjuncts.end();

	InversionCandidateList inversions;
	inversions.shrink(0);

	// First, handle "AND" comparisons (all nodes except nod_or)
	for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
	{
		jrd_nod* const node = tail->opt_conjunct_node;
		if (!(tail->opt_conjunct_flags & opt_conjunct_used) && node && (node->nod_type != nod_or))
		{
			matchOnIndexes(&indexScratches, node, 1);
		}
	}
	getInversionCandidates(&inversions, &indexScratches, 1);

	if (sort && rsb) {
		*rsb = generateNavigation();
	}

	// Second, handle "OR" comparisons
	InversionCandidate* invCandidate = NULL;
	for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
	{
		jrd_nod* const node = tail->opt_conjunct_node;
		if (!(tail->opt_conjunct_flags & opt_conjunct_used) && node && (node->nod_type == nod_or))
		{
			invCandidate = matchOnIndexes(&indexScratches, node, 1);
			if (invCandidate)
			{
				invCandidate->boolean = node;
				inversions.add(invCandidate);
			}
		}
	}

#ifdef OPT_DEBUG_RETRIEVAL
	// Debug
	printCandidates(&inversions);
#endif

	invCandidate = makeInversion(&inversions);

	if (invCandidate)
	{
		if (invCandidate->unique)
		{
			// Set up the unique retrieval cost to be fixed and not dependent on
			// possibly outdated statistics
			invCandidate->cost = DEFAULT_INDEX_COST * invCandidate->indexes + 1;
		}
		else
		{
			// Add the records retrieval cost to the priorly calculated index scan cost
			invCandidate->cost += csb->csb_rpt[stream].csb_cardinality * invCandidate->selectivity;
		}

		// Add the streams where this stream is depending on.
		for (size_t i = 0; i < invCandidate->matches.getCount(); i++) {
			findDependentFromStreams(invCandidate->matches[i], &invCandidate->dependentFromStreams);
		}

		if (setConjunctionsMatched)
		{
			Firebird::SortedArray<jrd_nod*> matches;
			// AB: Putting a unsorted array in a sorted array directly by join isn't
			// very safe at the moment, but in our case Array holds a sorted list.
			// However SortedArray class should be updated to handle join right!
			matches.join(invCandidate->matches);
			for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
			{
				if (!(tail->opt_conjunct_flags & opt_conjunct_used))
				{
					if (matches.exist(tail->opt_conjunct_node)) {
						tail->opt_conjunct_flags |= opt_conjunct_matched;
					}
				}
			}
		}
	}

#ifdef OPT_DEBUG_RETRIEVAL
	// Debug
	printFinalCandidate(invCandidate);
#endif

	// Clean up inversion list
	InversionCandidate** inversion = inversions.begin();
	for (size_t i = 0; i < inversions.getCount(); i++) {
		delete inversion[i];
	}

	return invCandidate;
}

RecordSource* OptimizerRetrieval::generateNavigation()
{
/**************************************
 *
 *	g e n e r a t e N a v i g a t i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	fb_assert(sort);

	jrd_nod* sortPtr = *sort;
	if (!sortPtr) {
		return NULL;
	}

	size_t i = 0;
	for (; i < indexScratches.getCount(); ++i)
	{

		index_desc* idx = indexScratches[i].idx;

		// if the number of fields in the sort is greater than the number of
		// fields in the index, the index will not be used to optimize the
		// sort--note that in the case where the first field is unique, this
		// could be optimized, since the sort will be performed correctly by
		// navigating on a unique index on the first field--deej
		if (sortPtr->nod_count > idx->idx_count) {
			continue;
		}

		// if the user-specified access plan for this request didn't
		// mention this index, forget it
		if ((idx->idx_runtime_flags & idx_plan_dont_use) &&
			!(idx->idx_runtime_flags & idx_plan_navigate))
		{
			continue;
		}

		// only a single-column ORDER BY clause can be mapped to
		// an expression index
		if (idx->idx_flags & idx_expressn)
		{
			if (sortPtr->nod_count != 1)
				continue;
		}

		// check to see if the fields in the sort match the fields in the index
		// in the exact same order--we used to check for ascending/descending prior
		// to SCROLLABLE_CURSORS, but now descending sorts can use ascending indices
		// and vice versa.

		bool usableIndex = true;
		index_desc::idx_repeat* idx_tail = idx->idx_rpt;
		jrd_nod** ptr = sortPtr->nod_arg;
		for (const jrd_nod* const* const end = ptr + sortPtr->nod_count; ptr < end; ptr++, idx_tail++)
		{
			jrd_nod* node = *ptr;
			if (idx->idx_flags & idx_expressn)
			{
				if (!OPT_expression_equal(tdbb, optimizer, idx, node, stream))
				{
					usableIndex = false;
					break;
				}
			}
			else if (node->nod_type != nod_field ||
				(USHORT)(IPTR) node->nod_arg[e_fld_stream] != stream ||
				(USHORT)(IPTR) node->nod_arg[e_fld_id] != idx_tail->idx_field)
			{
				usableIndex = false;
				break;
			}

			if ((ptr[sortPtr->nod_count] && !(idx->idx_flags & idx_descending)) ||
				(!ptr[sortPtr->nod_count] && (idx->idx_flags & idx_descending)) ||
				// for ODS11 default nulls placement always may be matched to index
				(database->dbb_ods_version >= ODS_VERSION11 &&
					((reinterpret_cast<IPTR>(ptr[2 * sortPtr->nod_count]) == rse_nulls_first &&
						ptr[sortPtr->nod_count]) ||
						(reinterpret_cast<IPTR>(ptr[2 * sortPtr->nod_count]) == rse_nulls_last &&
						!ptr[sortPtr->nod_count]))) ||
				// for ODS10 and earlier indices always placed nulls at the end of dataset
				(database->dbb_ods_version < ODS_VERSION11 &&
					reinterpret_cast<IPTR>(ptr[2 * sortPtr->nod_count]) == rse_nulls_first) )
			{
				usableIndex = false;
				break;
			}

			dsc desc;
			CMP_get_desc(tdbb, csb, node, &desc);

			// ASF: "desc.dsc_ttype() > ttype_last_internal" is to avoid recursion
			// when looking for charsets/collations

			if (DTYPE_IS_TEXT(desc.dsc_dtype) && desc.dsc_ttype() > ttype_last_internal)
			{
				TextType* tt = INTL_texttype_lookup(tdbb, desc.dsc_ttype());

				if (idx->idx_flags & idx_unique)
				{
					if (tt->getFlags() & TEXTTYPE_UNSORTED_UNIQUE)
					{
						usableIndex = false;
						break;
					}
				}
				else
				{
					// ASF: We currently can't use non-unique index for GROUP BY and DISTINCT with
					// multi-level and insensitive collation. In NAV, keys are verified with memcmp
					// but there we don't know length of each level.
					if ((sortPtr->nod_flags & nod_unique_sort) &&
						(tt->getFlags() & TEXTTYPE_SEPARATE_UNIQUE))
					{
						usableIndex = false;
						break;
					}
				}
			}
		}

		if (!usableIndex)
		{
			// We can't use this index, try next one.
			continue;
		}

		// Looks like we can do a navigational walk.  Flag that
		// we have used this index for navigation, and allocate
		// a navigational rsb for it.
		*sort = NULL;
		idx->idx_runtime_flags |= idx_navigate;
		//return gen_nav_rsb(tdbb, opt, stream, relation, alias, idx);

		USHORT key_length = ROUNDUP(BTR_key_length(tdbb, relation, idx), sizeof(SLONG));
		RecordSource* rsb = FB_NEW_RPT(*tdbb->getDefaultPool(), RSB_NAV_count) RecordSource();
		rsb->rsb_type = rsb_navigate;
		rsb->rsb_relation = relation;
		rsb->rsb_stream = (UCHAR) stream;
		rsb->rsb_alias = getAlias();
		rsb->rsb_arg[RSB_NAV_index] = (RecordSource*) makeIndexScanNode(&indexScratches[i]);
		rsb->rsb_arg[RSB_NAV_key_length] = (RecordSource*) (IPTR) key_length;

		const USHORT size = OPT_nav_rsb_size(rsb, key_length, 0);
		rsb->rsb_impure = CMP_impure(optimizer->opt_csb, size);
		return rsb;
	}

	return NULL;
}

InversionCandidate* OptimizerRetrieval::getInversion(RecordSource** rsb)
{
/**************************************
 *
 *	g e t I n v e r s i o n
 *
 **************************************
 *
 * Return an inversionCandidate which
 * contains a created inversion when an
 * index could be used.
 * This function should always return
 * an InversionCandidate;
 *
 **************************************/
	createIndexScanNodes = rsb ? true : false;
	setConjunctionsMatched = rsb ? true : false;

	InversionCandidate* invCandidate = generateInversion(rsb);

	if (!invCandidate)
	{
		// No index will be used
		invCandidate = FB_NEW(pool) InversionCandidate(pool);
		invCandidate->indexes = 0;
		invCandidate->selectivity = MAXIMUM_SELECTIVITY;
		invCandidate->cost = csb->csb_rpt[stream].csb_cardinality;
	}

	// Adjust the effective selectivity by treating computable conjunctions as filters
	for (const OptimizerBlk::opt_conjunct* tail = optimizer->opt_conjuncts.begin();
		tail < optimizer->opt_conjuncts.end(); tail++)
	{
		jrd_nod* const node = tail->opt_conjunct_node;
		if (!(tail->opt_conjunct_flags & opt_conjunct_used) &&
			OPT_computable(optimizer->opt_csb, node, stream, false, true) &&
			!invCandidate->matches.exist(node))
		{
			const double factor = (node->nod_type == nod_eql) ?
				REDUCE_SELECTIVITY_FACTOR_EQUALITY : REDUCE_SELECTIVITY_FACTOR_INEQUALITY;
			invCandidate->selectivity *= factor;
		}
	}

	return invCandidate;
}

void OptimizerRetrieval::getInversionCandidates(InversionCandidateList* inversions,
		IndexScratchList* fromIndexScratches, USHORT scope) const
{
/**************************************
 *
 *	g e t I n v e r s i o n C a n d i d a t e s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	const double cardinality = csb->csb_rpt[stream].csb_cardinality;

	// Walk through indexes to calculate selectivity / candidate
	Firebird::Array<jrd_nod*> matches;
	size_t i = 0;
	for (i = 0; i < fromIndexScratches->getCount(); i++)
	{
		IndexScratch& scratch = (*fromIndexScratches)[i];
		scratch.scopeCandidate = false;
		scratch.lowerCount = 0;
		scratch.upperCount = 0;
		scratch.nonFullMatchedSegments = MAX_INDEX_SEGMENTS + 1;
		scratch.fuzzy = false;

		if (scratch.candidate)
		{
			matches.clear();
			scratch.selectivity = MAXIMUM_SELECTIVITY;
			bool unique = false;
			for (int j = 0; j < scratch.idx->idx_count; j++)
			{
				IndexScratchSegment* segment = scratch.segments[j];

				// Special case: IS NULL against the in-the-middle segment of the ascending compound index.
				// In ODS11, such a retrieval is not better than the one for the prior segment. So stop before
				// utilizing that segment and leave the boolean for other possible matches.
				if (!(scratch.idx->idx_flags & idx_descending) &&
					segment->scanType == segmentScanMissing &&
					j > 0 && j < scratch.idx->idx_count - 1)
				{
					IndexScratchSegment* const next_segment = scratch.segments[j + 1];

					if (next_segment->scanType != segmentScanEqual &&
						next_segment->scanType != segmentScanEquivalent &&
						next_segment->scanType != segmentScanMissing)
					{
						break;
					}
				}

				if (segment->scope == scope) {
					scratch.scopeCandidate = true;
				}

				if (segment->scanType != segmentScanMissing && !(scratch.idx->idx_flags & idx_unique))
				{
					const USHORT iType = scratch.idx->idx_rpt[j].idx_itype;

					if (iType >= idx_first_intl_string)
					{
						TextType* textType = INTL_texttype_lookup(tdbb, INTL_INDEX_TO_TEXT(iType));

						if (textType->getFlags() & TEXTTYPE_SEPARATE_UNIQUE)
						{
							// ASF: Order is more precise than equivalence class.
							// We can't use the next segments, and we'll need to use
							// INTL_KEY_PARTIAL to construct the last segment's key.
							scratch.fuzzy = true;
						}
					}
				}

				// Check if this is the last usable segment
				if (!scratch.fuzzy &&
					(segment->scanType == segmentScanEqual ||
					 segment->scanType == segmentScanEquivalent ||
					 segment->scanType == segmentScanMissing))
				{
					// This is a perfect usable segment thus update root selectivity
					scratch.lowerCount++;
					scratch.upperCount++;
					scratch.selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
					scratch.nonFullMatchedSegments = scratch.idx->idx_count - (j + 1);
					// Add matches for this segment to the main matches list
					matches.join(segment->matches);

					// An equality scan for any unique index cannot retrieve more
					// than one row. The same is true for an equivalence scan for
					// any primary index.
					const bool single_match =
						((segment->scanType == segmentScanEqual &&
							scratch.idx->idx_flags & idx_unique) ||
						(segment->scanType == segmentScanEquivalent &&
							scratch.idx->idx_flags & idx_primary));

					// dimitr: IS NULL scan against primary key is guaranteed
					//		   to return zero rows. Do we need yet another
					//		   special case here?

					if (single_match && ((j + 1) == scratch.idx->idx_count))
					{
						// We have found a full equal matching index and it's unique,
						// so we can stop looking further, because this is the best
						// one we can get.
						unique = true;
						break;
					}

					// dimitr: number of nulls is not reflected by our selectivity,
					//		   so IS NOT DISTINCT and IS NULL scans may retrieve
					//		   much bigger bitmap than expected here. I think
					//		   appropriate reduce selectivity factors are required
					//		   to be applied here.
				}
				else
				{
					// This is our last segment that we can use,
					// estimate the selectivity
					double selectivity = scratch.selectivity;
					double factor = 1;
					switch (segment->scanType)
					{
						case segmentScanBetween:
							scratch.lowerCount++;
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_BETWEEN;
							break;

						case segmentScanLess:
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_LESS;
							break;

						case segmentScanGreater:
							scratch.lowerCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_GREATER;
							break;

						case segmentScanStarting:
						case segmentScanEqual:
						case segmentScanEquivalent:
							scratch.lowerCount++;
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_STARTING;
							break;

						default:
							fb_assert(segment->scanType == segmentScanNone);
							break;
					}

					// Adjust the compound selectivity using the reduce factor.
					// It should be better than the previous segment but worse
					// than a full match.
					const double diffSelectivity = scratch.selectivity - selectivity;
					selectivity += (diffSelectivity * factor);
					fb_assert(selectivity <= scratch.selectivity);
					scratch.selectivity = selectivity;

					if (segment->scanType != segmentScanNone)
					{
						matches.join(segment->matches);
						scratch.nonFullMatchedSegments = scratch.idx->idx_count - j;
					}
					break;
				}
			}

			if (scratch.scopeCandidate)
			{
				// When selectivity is zero the statement is prepared on an
				// empty table or the statistics aren't updated.
				// For an unique index, estimate the selectivity via the stream cardinality.
				// For a non-unique one, assume 1/10 of the maximum selectivity, so that
				// at least some indexes could be chosen by the optimizer.
				double selectivity = scratch.selectivity;
				if (selectivity <= 0)
				{
					if (unique && cardinality)
						selectivity = 1 / cardinality;
					else
						selectivity = DEFAULT_SELECTIVITY;
				}

				InversionCandidate* invCandidate = FB_NEW(pool) InversionCandidate(pool);
				invCandidate->unique = unique;
				invCandidate->selectivity = selectivity;
				// Calculate the cost (only index pages) for this index.
				// The constant DEFAULT_INDEX_COST 1 is an average for
				// the rootpage and non-leaf pages.
				// Assuming the rootpage will stay in cache else the index
				// cost is calculted too high. Better would be including
				// the index-depth, but this is not possible due lack
				// on information at this time.
				invCandidate->cost = DEFAULT_INDEX_COST + (scratch.selectivity * scratch.cardinality);
				invCandidate->nonFullMatchedSegments = scratch.nonFullMatchedSegments;
				invCandidate->matchedSegments = MAX(scratch.lowerCount, scratch.upperCount);
				invCandidate->indexes = 1;
				invCandidate->scratch = &scratch;
				invCandidate->matches.join(matches);
				for (size_t k = 0; k < invCandidate->matches.getCount(); k++)
				{
					findDependentFromStreams(invCandidate->matches[k],
											 &invCandidate->dependentFromStreams);
				}
				invCandidate->dependencies = invCandidate->dependentFromStreams.getCount();
				inversions->add(invCandidate);
			}
		}
	}
}

jrd_nod* OptimizerRetrieval::makeIndexNode(const index_desc* idx) const
{
/**************************************
 *
 *	m a k e I n d e x N o d e
 *
 **************************************
 *
 * Functional description
 *	Make an index node and an index retrieval block.
 *
 **************************************/

	// check whether this is during a compile or during
	// a SET INDEX operation
	if (csb) {
		CMP_post_resource(&csb->csb_resources, relation, Resource::rsc_index, idx->idx_id);
	}
	else {
		CMP_post_resource(&tdbb->getRequest()->req_resources, relation, Resource::rsc_index, idx->idx_id);
	}

	jrd_nod* node = PAR_make_node(tdbb, e_idx_length);
	node->nod_type = nod_index;
	node->nod_count = 0;

	IndexRetrieval* retrieval = FB_NEW_RPT(pool, idx->idx_count * 2) IndexRetrieval();
	node->nod_arg[e_idx_retrieval] = (jrd_nod*) retrieval;
	retrieval->irb_index = idx->idx_id;
	memcpy(&retrieval->irb_desc, idx, sizeof(retrieval->irb_desc));
	if (csb) {
		node->nod_impure = CMP_impure(csb, sizeof(impure_inversion));
	}
	return node;
}

jrd_nod* OptimizerRetrieval::makeIndexScanNode(IndexScratch* indexScratch) const
{
/**************************************
 *
 *	m a k e I n d e x S c a n N o d e
 *
 **************************************
 *
 * Functional description
 *	Build node for index scan.
 *
 **************************************/

	if (!createIndexScanNodes) {
		return NULL;
	}

	// Allocate both a index retrieval node and block.
	index_desc* idx = indexScratch->idx;
	jrd_nod* node = makeIndexNode(idx);
	IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
	retrieval->irb_relation = relation;

	// Pick up lower bound segment values
	jrd_nod** lower = retrieval->irb_value;
	jrd_nod** upper = retrieval->irb_value + idx->idx_count;
	retrieval->irb_lower_count = indexScratch->lowerCount;
	retrieval->irb_upper_count = indexScratch->upperCount;

	if (idx->idx_flags & idx_descending)
	{
		// switch upper/lower information
		upper = retrieval->irb_value;
		lower = retrieval->irb_value + idx->idx_count;
		retrieval->irb_lower_count = indexScratch->upperCount;
		retrieval->irb_upper_count = indexScratch->lowerCount;
		retrieval->irb_generic |= irb_descending;
	}

	int i = 0;
	bool ignoreNullsOnScan = true;
	IndexScratchSegment** segment = indexScratch->segments.begin();
	for (i = 0; i < MAX(indexScratch->lowerCount, indexScratch->upperCount); i++)
	{
		if (segment[i]->scanType == segmentScanMissing)
		{
			jrd_nod* value = PAR_make_node(tdbb, 0);
			value->nod_type = nod_null;
			*lower++ = *upper++ = value;
			ignoreNullsOnScan = false;
		}
		else
		{
			if (i < indexScratch->lowerCount) {
				*lower++ = segment[i]->lowerValue;
			}
			if (i < indexScratch->upperCount) {
				*upper++ = segment[i]->upperValue;
			}
		}
		if (segment[i]->scanType == segmentScanEquivalent) {
			ignoreNullsOnScan = false;
		}
	}

	i = MAX(indexScratch->lowerCount, indexScratch->upperCount) - 1;
	if (i >= 0)
	{
		if (segment[i]->scanType == segmentScanStarting)
			retrieval->irb_generic |= irb_starting;

		if (segment[i]->excludeLower)
			retrieval->irb_generic |= irb_exclude_lower;

		if (segment[i]->excludeUpper)
			retrieval->irb_generic |= irb_exclude_upper;
	}

	if (indexScratch->fuzzy)
		retrieval->irb_generic |= irb_starting;	// Flag the need to use INTL_KEY_PARTIAL in btr.

	// This index is never used for IS NULL, thus we can ignore NULLs
	// already at index scan. But this rule doesn't apply to nod_equiv
	// which requires NULLs to be found in the index.
	// A second exception is when this index is used for navigation.
	if (ignoreNullsOnScan && !(idx->idx_runtime_flags & idx_navigate)) {
		retrieval->irb_generic |= irb_ignore_null_value_key;
	}

	// Check to see if this is really an equality retrieval
	if (retrieval->irb_lower_count == retrieval->irb_upper_count)
	{
		retrieval->irb_generic |= irb_equality;
		segment = indexScratch->segments.begin();
		for (i = 0; i < retrieval->irb_lower_count; i++)
		{
			if (segment[i]->lowerValue != segment[i]->upperValue)
			{
				retrieval->irb_generic &= ~irb_equality;
				break;
			}
		}
	}

	// If we are matching less than the full index, this is a partial match
	if (idx->idx_flags & idx_descending)
	{
		if (retrieval->irb_lower_count < idx->idx_count) {
			retrieval->irb_generic |= irb_partial;
		}
	}
	else
	{
		if (retrieval->irb_upper_count < idx->idx_count) {
			retrieval->irb_generic |= irb_partial;
		}
	}

	// mark the index as utilized for the purposes of this compile
	idx->idx_runtime_flags |= idx_used;

	return node;
}

InversionCandidate* OptimizerRetrieval::makeInversion(InversionCandidateList* inversions) const
{
/**************************************
 *
 *	m a k e I n v e r s i o n
 *
 **************************************
 *
 * Select best available inversion candidates
 * and compose them to 1 inversion.
 * This was never implemented:
 * If top is true the datapages-cost is
 * also used in the calculation (only needed
 * for top InversionNode generation).
 *
 **************************************/

	if (inversions->isEmpty()) {
		return NULL;
	}

	const double streamCardinality =
		MAX(csb->csb_rpt[stream].csb_cardinality, MINIMUM_CARDINALITY);

	// Prepared statements could be optimized against an almost empty table
	// and then cached (such as in the restore process), thus causing slowdown
	// when the table grows. In this case, let's consider all available indices.
	const bool smallTable = (streamCardinality <= THRESHOLD_CARDINALITY);

	// This flag disables our smart index selection algorithm.
	// It's set for any explicit (i.e. user specified) plan which
	// requires all existing indices to be considered for a retrieval.
	const bool acceptAll = csb->csb_rpt[stream].csb_plan;

	double totalSelectivity = MAXIMUM_SELECTIVITY; // worst selectivity
	double totalIndexCost = 0;

	// The upper limit to use an index based retrieval is five indexes + almost all datapages
	const double maximumCost = (DEFAULT_INDEX_COST * 5) + (streamCardinality * 0.95);
	const double minimumSelectivity = 1 / streamCardinality;

	double previousTotalCost = maximumCost;

	// Force to always choose at least one index
	bool firstCandidate = true;

	size_t i = 0;
	InversionCandidate* invCandidate = NULL;
	InversionCandidate** inversion = inversions->begin();
	for (i = 0; i < inversions->getCount(); i++)
	{
		inversion[i]->used = false;
		if (inversion[i]->scratch)
		{
			if (inversion[i]->scratch->idx->idx_runtime_flags & idx_plan_dont_use) {
				inversion[i]->used = true;
			}
		}
	}

	// The matches returned in this inversion are always sorted.
	Firebird::SortedArray<jrd_nod*> matches;

	for (i = 0; i < inversions->getCount(); i++)
	{

		// Initialize vars before walking through candidates
		InversionCandidate* bestCandidate = NULL;
		bool restartLoop = false;

		for (size_t currentPosition = 0; currentPosition < inversions->getCount(); ++currentPosition)
		{
			InversionCandidate* currentInv = inversion[currentPosition];
			if (!currentInv->used)
			{
				// If this is a unique full equal matched inversion we're done, so
				// we can make the inversion and return it.
				if (currentInv->unique && currentInv->dependencies)
				{
					if (!invCandidate) {
						invCandidate = FB_NEW(pool) InversionCandidate(pool);
					}
					if (!currentInv->inversion && currentInv->scratch) {
						invCandidate->inversion = makeIndexScanNode(currentInv->scratch);
					}
					else {
						invCandidate->inversion = currentInv->inversion;
					}
					invCandidate->unique = currentInv->unique;
					invCandidate->selectivity = currentInv->selectivity;
					invCandidate->cost = currentInv->cost;
					invCandidate->indexes = currentInv->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments = currentInv->matchedSegments;
					invCandidate->dependencies = currentInv->dependencies;
					matches.clear();
					for (size_t j = 0; j < currentInv->matches.getCount(); j++)
					{
						if (!matches.exist(currentInv->matches[j])) {
							matches.add(currentInv->matches[j]);
						}
					}
					if (currentInv->boolean)
					{
						if (!matches.exist(currentInv->boolean)) {
							matches.add(currentInv->boolean);
						}
					}
					invCandidate->matches.join(matches);
					if (acceptAll) {
						continue;
					}

					return invCandidate;
				}

				// Look if a match is already used by previous matches.
				bool anyMatchAlreadyUsed = false;
				for (size_t k = 0; k < currentInv->matches.getCount(); k++)
				{
					if (matches.exist(currentInv->matches[k]))
					{
						anyMatchAlreadyUsed = true;
						break;
					}
				}

				if (anyMatchAlreadyUsed && !acceptAll)
				{
					currentInv->used = true;
					// If a match on this index was already used by another
					// index, add also the other matches from this index.
					for (size_t j = 0; j < currentInv->matches.getCount(); j++)
					{
						if (!matches.exist(currentInv->matches[j])) {
							matches.add(currentInv->matches[j]);
						}
					}
					// Restart loop, because other indexes could also be excluded now.
					restartLoop = true;
					break;
				}

				if (!bestCandidate)
				{
					// The first candidate
					bestCandidate = currentInv;
				}
				else
				{
					if (currentInv->unique && !bestCandidate->unique)
					{
						// A unique full equal match is better than anything else.
						bestCandidate = currentInv;
					}
					else if (currentInv->unique == bestCandidate->unique)
					{
						if (currentInv->dependencies > bestCandidate->dependencies)
						{
							// Index used for a relationship must be always prefered to
							// the filtering ones, otherwise the nested loop join has
							// no chances to be better than a sort merge.
							// An alternative (simplified) condition might be:
							//   currentInv->dependencies > 0
							//   && bestCandidate->dependencies == 0
							// but so far I tend to think that the current one is better.
							bestCandidate = currentInv;
						}
						else if (currentInv->dependencies == bestCandidate->dependencies)
						{

							const double bestCandidateCost =
								bestCandidate->cost + (bestCandidate->selectivity * streamCardinality);
							const double currentCandidateCost =
								currentInv->cost + (currentInv->selectivity * streamCardinality);

							// Do we have very similar costs?
							double diffCost = currentCandidateCost;
							if (!diffCost && !bestCandidateCost)
							{
								// Two zero costs should be handled as being the same
								// (other comparison criterias should be applied, see below).
								diffCost = 1;
							}
							else if (diffCost)
							{
								// Calculate the difference.
								diffCost = bestCandidateCost / diffCost;
							}
							else {
								diffCost = 0;
							}

							if ((diffCost >= 0.98) && (diffCost <= 1.02))
							{
								// If the "same" costs then compare with the nr of unmatched segments,
								// how many indexes and matched segments. First compare number of indexes.
								int compareSelectivity = (currentInv->indexes - bestCandidate->indexes);
								if (compareSelectivity == 0)
								{
									// For the same number of indexes compare number of matched segments.
									// Note the inverted condition: the more matched segments the better.
									compareSelectivity =
										(bestCandidate->matchedSegments - currentInv->matchedSegments);
									if (compareSelectivity == 0)
									{
										// For the same number of matched segments
										// compare ones that aren't full matched
										compareSelectivity =
											(currentInv->nonFullMatchedSegments - bestCandidate->nonFullMatchedSegments);
									}
								}
								if (compareSelectivity < 0) {
									bestCandidate = currentInv;
								}
							}
							else if (currentCandidateCost < bestCandidateCost)
							{
								// How lower the cost the better.
								bestCandidate = currentInv;
							}
						}
					}
				}
			}
		}

		if (restartLoop) {
			continue;
		}

		// If we have a candidate which is interesting build the inversion
		// else we're done.
		if (bestCandidate)
		{
			// AB: Here we test if our new candidate is interesting enough to be added for
			// index retrieval.

			// AB: For now i'll use the calculation that's often used for and-ing selectivities (S1 * S2).
			// I think this calculation is not right for many cases.
			// For example two "good" selectivities will result in a very good selectivity, but
			// mostly a filter is made by adding criteria's where every criteria is an extra filter
			// compared to the previous one. Thus with the second criteria in _most_ cases still
			// records are returned. (Think also on the segment-selectivity in compound indexes)
			// Assume a table with 100000 records and two selectivities of 0.001 (100 records) which
			// are both AND-ed (with S1 * S2 => 0.001 * 0.001 = 0.000001 => 0.1 record).
			//
			// A better formula could be where the result is between "Sbest" and "Sbest * factor"
			// The reducing factor should be between 0 and 1 (Sbest = best selectivity)
			//
			// Example:
			/*
			double newTotalSelectivity = 0;
			double bestSel = bestCandidate->selectivity;
			double worstSel = totalSelectivity;
			if (bestCandidate->selectivity > totalSelectivity)
			{
				worstSel = bestCandidate->selectivity;
				bestSel = totalSelectivity;
			}

			if (bestSel >= MAXIMUM_SELECTIVITY) {
				newTotalSelectivity = MAXIMUM_SELECTIVITY;
			}
			else if (bestSel == 0) {
				newTotalSelectivity = 0;
			}
			else {
				newTotalSelectivity = bestSel - ((1 - worstSel) * (bestSel - (bestSel * 0.01)));
			}
			*/

			const double newTotalSelectivity = bestCandidate->selectivity * totalSelectivity;
			const double newTotalDataCost = newTotalSelectivity * streamCardinality;
			const double newTotalIndexCost = totalIndexCost + bestCandidate->cost;
			const double totalCost = newTotalDataCost + newTotalIndexCost;

			// Test if the new totalCost will be higher than the previous totalCost
			// and if the current selectivity (without the bestCandidate) is already good enough.
			if (acceptAll || smallTable || firstCandidate ||
				(totalCost < previousTotalCost && totalSelectivity > minimumSelectivity))
			{
				// Exclude index from next pass
				bestCandidate->used = true;

				firstCandidate = false;

				previousTotalCost = totalCost;
				totalIndexCost = newTotalIndexCost;
				totalSelectivity = newTotalSelectivity;

				if (!invCandidate)
				{
					invCandidate = FB_NEW(pool) InversionCandidate(pool);
					if (!bestCandidate->inversion && bestCandidate->scratch) {
						invCandidate->inversion = makeIndexScanNode(bestCandidate->scratch);
					}
					else {
						invCandidate->inversion = bestCandidate->inversion;
					}
					invCandidate->unique = bestCandidate->unique;
					invCandidate->selectivity = bestCandidate->selectivity;
					invCandidate->cost = bestCandidate->cost;
					invCandidate->indexes = bestCandidate->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments = bestCandidate->matchedSegments;
					invCandidate->dependencies = bestCandidate->dependencies;
					for (size_t j = 0; j < bestCandidate->matches.getCount(); j++)
					{
						if (!matches.exist(bestCandidate->matches[j])) {
							matches.add(bestCandidate->matches[j]);
						}
					}
					if (bestCandidate->boolean)
					{
						if (!matches.exist(bestCandidate->boolean)) {
							matches.add(bestCandidate->boolean);
						}
					}
				}
				else
				{
					if (!bestCandidate->inversion && bestCandidate->scratch)
					{
						invCandidate->inversion = composeInversion(invCandidate->inversion,
							makeIndexScanNode(bestCandidate->scratch), nod_bit_and);
					}
					else
					{
						invCandidate->inversion = composeInversion(invCandidate->inversion,
							bestCandidate->inversion, nod_bit_and);
					}
					invCandidate->unique = (invCandidate->unique || bestCandidate->unique);
					invCandidate->selectivity = totalSelectivity;
					invCandidate->cost += bestCandidate->cost;
					invCandidate->indexes += bestCandidate->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments =
						MAX(bestCandidate->matchedSegments, invCandidate->matchedSegments);
					invCandidate->dependencies += bestCandidate->dependencies;
					for (size_t j = 0; j < bestCandidate->matches.getCount(); j++)
					{
						if (!matches.exist(bestCandidate->matches[j])) {
	                        matches.add(bestCandidate->matches[j]);
						}
					}
					if (bestCandidate->boolean)
					{
						if (!matches.exist(bestCandidate->boolean)) {
							matches.add(bestCandidate->boolean);
						}
					}
				}
				if (invCandidate->unique)
				{
					// Single unique full equal match is enough
					if (!acceptAll)
						break;
				}
			}
			else
			{
				// We're done
				break;
			}
		}
		else {
			break;
		}
	}

	if (invCandidate && matches.getCount()) {
		invCandidate->matches.join(matches);
	}

	return invCandidate;
}

bool OptimizerRetrieval::matchBoolean(IndexScratch* indexScratch, jrd_nod* boolean, USHORT scope) const
{
/**************************************
 *
 *	m a t c h B o o l e a n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	bool forward = true;

	if (boolean->nod_flags & nod_deoptimize)
		return false;

	jrd_nod* match = boolean->nod_arg[0];
	jrd_nod* value = (boolean->nod_count < 2) ? NULL : boolean->nod_arg[1];
	jrd_nod* value2 = (boolean->nod_type == nod_between) ? boolean->nod_arg[2] : NULL;

	if (indexScratch->idx->idx_flags & idx_expressn)
	{
		// see if one side or the other is matchable to the index expression

	    fb_assert(indexScratch->idx->idx_expression != NULL);

		if (!OPT_expression_equal(tdbb, optimizer, indexScratch->idx, match, stream) ||
			(value && !OPT_computable(optimizer->opt_csb, value, stream, true, false)))
		{
			if (boolean->nod_type != nod_starts && value &&
				OPT_expression_equal(tdbb, optimizer, indexScratch->idx, value, stream) &&
				OPT_computable(optimizer->opt_csb, match, stream, true, false))
			{
				match = boolean->nod_arg[1];
				value = boolean->nod_arg[0];
				forward = false;
			}
			else
				return false;
		}
	}
	else
	{
		// If left side is not a field, swap sides.
		// If left side is still not a field, give up

		if (match->nod_type != nod_field ||
			(USHORT)(IPTR) match->nod_arg[e_fld_stream] != stream ||
			(value && !OPT_computable(optimizer->opt_csb, value, stream, true, false)))
		{
			match = value;
			value = boolean->nod_arg[0];
			if (!match || match->nod_type != nod_field ||
				(USHORT)(IPTR) match->nod_arg[e_fld_stream] != stream ||
				!OPT_computable(optimizer->opt_csb, value, stream, true, false))
			{
				return false;
			}
			forward = false;
		}
	}

	// check datatypes to ensure that the index scan is guaranteed
	// to deliver correct results

	bool excludeBound = (boolean->nod_type == nod_gtr || boolean->nod_type == nod_lss);

	if (value)
	{
		dsc desc1, desc2;
		CMP_get_desc(tdbb, optimizer->opt_csb, match, &desc1);
		CMP_get_desc(tdbb, optimizer->opt_csb, value, &desc2);

		if (value->nod_type != nod_null && !BTR_types_comparable(desc1, desc2))
			return false;

		// if the indexed column is of type int64, we need to inject an
		// extra cast to deliver the scale value to the BTR level

		if (desc1.dsc_dtype == dtype_int64)
		{
			Format* format = Format::newFormat(*tdbb->getDefaultPool(), 1);
			format->fmt_length = desc1.dsc_length;
			format->fmt_desc[0] = desc1;

			jrd_nod* cast = PAR_make_node(tdbb, e_cast_length);
			cast->nod_type = nod_cast;
			cast->nod_count = 1;
			cast->nod_arg[e_cast_source] = value;
			cast->nod_arg[e_cast_fmt] = (jrd_nod*) format;
			cast->nod_impure = CMP_impure(optimizer->opt_csb, sizeof(impure_value));
			value = cast;

			if (value2)
			{
				cast = PAR_make_node(tdbb, e_cast_length);
				cast->nod_type = nod_cast;
				cast->nod_count = 1;
				cast->nod_arg[e_cast_source] = value2;
				cast->nod_arg[e_cast_fmt] = (jrd_nod*) format;
				cast->nod_impure = CMP_impure(optimizer->opt_csb, sizeof(impure_value));
				value2 = cast;
			}
		}
		// for "DATE <op> TIMESTAMP" we need <op> to include the boundary value
		else if (desc1.dsc_dtype == dtype_sql_date && desc2.dsc_dtype == dtype_timestamp)
		{
			excludeBound = false;
		}
	}

	// match the field to an index, if possible, and save the value to be matched
	// as either the lower or upper bound for retrieval, or both

	const bool isDesc = (indexScratch->idx->idx_flags & idx_descending);
	int count = 0;
	IndexScratchSegment** segment = indexScratch->segments.begin();
	for (int i = 0; i < indexScratch->idx->idx_count; i++)
	{
		if ((indexScratch->idx->idx_flags & idx_expressn) ||
			(USHORT)(IPTR) match->nod_arg[e_fld_id] == indexScratch->idx->idx_rpt[i].idx_field)
		{
			switch (boolean->nod_type)
			{
				case nod_between:
					if (!forward || !OPT_computable(optimizer->opt_csb, value2, stream, true, false))
					{
						return false;
					}
					segment[i]->matches.add(boolean);
					// AB: If we have already an exact match don't
					// override it with worser matches.
					if (!((segment[i]->scanType == segmentScanEqual) ||
						(segment[i]->scanType == segmentScanEquivalent)))
					{
						segment[i]->lowerValue = value;
						segment[i]->upperValue = value2;
						segment[i]->scanType = segmentScanBetween;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
					}
					break;

				case nod_equiv:
					segment[i]->matches.add(boolean);
					// AB: If we have already an exact match don't
					// override it with worser matches.
					if (!(segment[i]->scanType == segmentScanEqual))
					{
						segment[i]->lowerValue = segment[i]->upperValue = value;
						segment[i]->scanType = segmentScanEquivalent;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
					}
					break;

				case nod_eql:
					segment[i]->matches.add(boolean);
					segment[i]->lowerValue = segment[i]->upperValue = value;
					segment[i]->scanType = segmentScanEqual;
					segment[i]->excludeLower = false;
					segment[i]->excludeUpper = false;
					break;

				case nod_gtr:
				case nod_geq:
					segment[i]->matches.add(boolean);
					if (!((segment[i]->scanType == segmentScanEqual) ||
						(segment[i]->scanType == segmentScanEquivalent) ||
						(segment[i]->scanType == segmentScanBetween)))
					{
						if (forward != isDesc) // (forward && !isDesc || !forward && isDesc)
							segment[i]->excludeLower = excludeBound;
						else
							segment[i]->excludeUpper = excludeBound;

						if (forward)
						{
							segment[i]->lowerValue = value;
							if (segment[i]->scanType == segmentScanLess)
                        		segment[i]->scanType = segmentScanBetween;
							else
                        		segment[i]->scanType = segmentScanGreater;
						}
						else
						{
							segment[i]->upperValue = value;
							if (segment[i]->scanType == segmentScanGreater)
								segment[i]->scanType = segmentScanBetween;
							else
								segment[i]->scanType = segmentScanLess;
						}
					}
					break;

				case nod_lss:
				case nod_leq:
					segment[i]->matches.add(boolean);
					if (!((segment[i]->scanType == segmentScanEqual) ||
						(segment[i]->scanType == segmentScanEquivalent) ||
						(segment[i]->scanType == segmentScanBetween)))
					{
						if (forward != isDesc)
							segment[i]->excludeUpper = excludeBound;
						else
							segment[i]->excludeLower = excludeBound;

						if (forward)
						{
							segment[i]->upperValue = value;
							if (segment[i]->scanType == segmentScanGreater)
								segment[i]->scanType = segmentScanBetween;
							else
								segment[i]->scanType = segmentScanLess;
						}
						else
						{
							segment[i]->lowerValue = value;
							if (segment[i]->scanType == segmentScanLess)
                        		segment[i]->scanType = segmentScanBetween;
							else
                        		segment[i]->scanType = segmentScanGreater;
						}
					}
					break;

				case nod_starts:
					// Check if validate for using index
					if (!forward || !validateStarts(indexScratch, boolean, i)) {
						return false;
					}
					segment[i]->matches.add(boolean);
					if (!((segment[i]->scanType == segmentScanEqual) ||
						(segment[i]->scanType == segmentScanEquivalent)))
					{
						segment[i]->lowerValue = segment[i]->upperValue = value;
						segment[i]->scanType = segmentScanStarting;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
					}
					break;

				case nod_missing:
					segment[i]->matches.add(boolean);
					if (!((segment[i]->scanType == segmentScanEqual) ||
						(segment[i]->scanType == segmentScanEquivalent)))
					{
						segment[i]->lowerValue = segment[i]->upperValue = value;
						segment[i]->scanType = segmentScanMissing;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
					}
					break;


				default:		// If no known boolean type is found return 0
					return false;
			}

			// A match could be made
			if (segment[i]->scope < scope) {
				segment[i]->scope = scope;
			}

			++count;

			if (i == 0)
			{
				// If this is the first segment, then this index is a candidate.
				indexScratch->candidate = true;
			}
		}
	}

	return (count >= 1);
}

InversionCandidate* OptimizerRetrieval::matchOnIndexes(
	IndexScratchList* inputIndexScratches, jrd_nod* boolean, USHORT scope) const
{
/**************************************
 *
 *	m a t c h O n I n d e x e s
 *
 **************************************
 *
 * Functional description
 *  Try to match boolean on every index.
 *  If the boolean is an "OR" node then a
 *  inversion candidate could be returned.
 *
 **************************************/
	DEV_BLKCHK(boolean, type_nod);


	// Handle the "OR" case up front
	if (boolean->nod_type == nod_or)
	{
		InversionCandidateList inversions;
		inversions.shrink(0);

		// Make list for index matches
		IndexScratchList indexOrScratches;

		// Copy information from caller
		size_t i = 0;
		for (; i < inputIndexScratches->getCount(); i++)
		{
			IndexScratch& scratch = (*inputIndexScratches)[i];
			indexOrScratches.add(scratch);
		}
		// We use a scope variable to see on how
		// deep we are in a nested or conjunction.
		scope++;

		InversionCandidate* invCandidate1 =
			matchOnIndexes(&indexOrScratches, boolean->nod_arg[0], scope);
		if (invCandidate1) {
			inversions.add(invCandidate1);
		}
		// Get usable inversions based on indexOrScratches and scope
		if (boolean->nod_arg[0]->nod_type != nod_or) {
			getInversionCandidates(&inversions, &indexOrScratches, scope);
		}

		invCandidate1 = makeInversion(&inversions);
		if (!invCandidate1) {
			return NULL;
		}

		// Clear list to remove previously matched conjunctions
		indexOrScratches.clear();
		// Copy information from caller
		i = 0;
		for (; i < inputIndexScratches->getCount(); i++)
		{
			IndexScratch& scratch = (*inputIndexScratches)[i];
			indexOrScratches.add(scratch);
		}
		// Clear inversion list
		inversions.clear();

		InversionCandidate* invCandidate2 =
			matchOnIndexes(&indexOrScratches, boolean->nod_arg[1], scope);
		if (invCandidate2) {
			inversions.add(invCandidate2);
		}
		// Make inversion based on indexOrScratches and scope
		if (boolean->nod_arg[1]->nod_type != nod_or) {
			getInversionCandidates(&inversions, &indexOrScratches, scope);
		}
		invCandidate2 = makeInversion(&inversions);

		if (invCandidate2)
		{
			InversionCandidate* invCandidate = FB_NEW(pool) InversionCandidate(pool);
			invCandidate->inversion =
				composeInversion(invCandidate1->inversion, invCandidate2->inversion, nod_bit_or);
			invCandidate->unique = (invCandidate1->unique && invCandidate2->unique);
			invCandidate->selectivity = invCandidate1->selectivity + invCandidate2->selectivity;
			invCandidate->cost = invCandidate1->cost + invCandidate2->cost;
			invCandidate->indexes = invCandidate1->indexes + invCandidate2->indexes;
			invCandidate->nonFullMatchedSegments = 0;
			invCandidate->matchedSegments =
				MIN(invCandidate1->matchedSegments, invCandidate2->matchedSegments);
			invCandidate->dependencies = invCandidate1->dependencies + invCandidate2->dependencies;

			// Add matches conjunctions that exists in both left and right inversion
			if ((invCandidate1->matches.getCount()) && (invCandidate2->matches.getCount()))
			{
				Firebird::SortedArray<jrd_nod*> matches;
				for (size_t j = 0; j < invCandidate1->matches.getCount(); j++) {
					matches.add(invCandidate1->matches[j]);
				}
				for (size_t j = 0; j < invCandidate2->matches.getCount(); j++)
				{
					if (matches.exist(invCandidate2->matches[j])) {
						invCandidate->matches.add(invCandidate2->matches[j]);
					}
				}
			}
			return invCandidate;
		}
		return NULL;
	}

	if (boolean->nod_type == nod_and)
	{
		// Recursivly call this procedure for every boolean
		// and finally get candidate inversions.
		// Normally we come here from within a nod_or conjunction.
		InversionCandidateList inversions;
		inversions.shrink(0);

		InversionCandidate* invCandidate =
			matchOnIndexes(inputIndexScratches, boolean->nod_arg[0], scope);
		if (invCandidate) {
			inversions.add(invCandidate);
		}
		invCandidate = matchOnIndexes(inputIndexScratches, boolean->nod_arg[1], scope);
		if (invCandidate) {
			inversions.add(invCandidate);
		}
		return makeInversion(&inversions);
	}

	// Walk through indexes
	for (size_t i = 0; i < inputIndexScratches->getCount(); i++)
	{
		IndexScratch& indexScratch = (*inputIndexScratches)[i];
		// Try to match the boolean against a index.
		if (!(indexScratch.idx->idx_runtime_flags & idx_plan_dont_use) ||
			(indexScratch.idx->idx_runtime_flags & idx_plan_navigate))
		{
			matchBoolean(&indexScratch, boolean, scope);
		}
	}
	return NULL;
}


#ifdef OPT_DEBUG_RETRIEVAL
void OptimizerRetrieval::printCandidate(const InversionCandidate* candidate) const
{
/**************************************
 *
 *	p r i n t C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "     cost(%1.2f), selectivity(%1.10f), indexes(%d), matched(%d, %d)",
		candidate->cost, candidate->selectivity, candidate->indexes, candidate->matchedSegments,
		candidate->nonFullMatchedSegments);
	if (candidate->unique) {
		fprintf(opt_debug_file, ", unique");
	}
	int depFromCount = candidate->dependentFromStreams.getCount();
	if (depFromCount >= 1)
	{
		fprintf(opt_debug_file, ", dependent from ");
		for (int i = 0; i < depFromCount; i++)
		{
			if (i == 0) {
				fprintf(opt_debug_file, "%d", candidate->dependentFromStreams[i]);
			}
			else {
				fprintf(opt_debug_file, ", %d", candidate->dependentFromStreams[i]);
			}
		}
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerRetrieval::printCandidates(const InversionCandidateList* inversions) const
{
/**************************************
 *
 *	p r i n t C a n d i d a t e s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "    retrieval candidates:\n");
	fclose(opt_debug_file);
	const InversionCandidate* const* inversion = inversions->begin();
	for (int i = 0; i < inversions->getCount(); i++)
	{
		const InversionCandidate* candidate = inversion[i];
		printCandidate(candidate);
	}
}

void OptimizerRetrieval::printFinalCandidate(const InversionCandidate* candidate) const
{
/**************************************
 *
 *	p r i n t F i n a l C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (candidate)
	{
		FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
		fprintf(opt_debug_file, "    final candidate: ");
		fclose(opt_debug_file);
		printCandidate(candidate);
	}
}
#endif


bool OptimizerRetrieval::validateStarts(IndexScratch* indexScratch,
										jrd_nod* boolean, USHORT segment) const
{
/**************************************
 *
 *	v a l i d a t e S t a r t s
 *
 **************************************
 *
 * Functional description
 *  Check if the boolean is valid for
 *  using it against the given index segment.
 *
 **************************************/
	if (boolean->nod_type != nod_starts) {
		return false;
	}

	jrd_nod* field = boolean->nod_arg[0];
	jrd_nod* value = boolean->nod_arg[1];

	if (indexScratch->idx->idx_flags & idx_expressn)
	{
		// AB: What if the expression contains a number/float etc.. and
		// we use starting with against it? Is that allowed?
		fb_assert(indexScratch->idx->idx_expression != NULL);
		if (!(OPT_expression_equal(tdbb, optimizer, indexScratch->idx, field, stream) ||
			(value && !OPT_computable(optimizer->opt_csb, value, stream, true, false))))
		{
			// AB: Can we swap de left and right sides by a starting with?
			// X STARTING WITH 'a' that is never the same as 'a' STARTING WITH X
			if (value &&
				OPT_expression_equal(tdbb, optimizer, indexScratch->idx, value, stream) &&
				OPT_computable(optimizer->opt_csb, field, stream, true, false))
			{
				field = value;
				value = boolean->nod_arg[0];
			}
			else {
				return false;
			}
		}
	}
	else
	{
		if (field->nod_type != nod_field)
		{
			// dimitr:	any idea how we can use an index in this case?
			//			The code below produced wrong results.
			// AB: I don't think that it would be effective, because
			// this must include many matches (think about empty string)
			return false;
			/*
			if (value->nod_type != nod_field)
				return NULL;
			field = value;
			value = boolean->nod_arg[0];
			*/
		}

		// Every string starts with an empty string so
		// don't bother using an index in that case.
		if (value->nod_type == nod_literal)
		{
			const dsc* literal_desc = &((Literal*) value)->lit_desc;
			if ((literal_desc->dsc_dtype == dtype_text && literal_desc->dsc_length == 0) ||
				(literal_desc->dsc_dtype == dtype_varying &&
					literal_desc->dsc_length == sizeof(USHORT)))
			{
				return false;
			}
		}

		// AB: Check if the index-segment is usable for using starts.
		// Thus it should be of type string, etc...
		if ((USHORT)(IPTR) field->nod_arg[e_fld_stream] != stream ||
			(USHORT)(IPTR) field->nod_arg[e_fld_id] != indexScratch->idx->idx_rpt[segment].idx_field ||
			!(indexScratch->idx->idx_rpt[segment].idx_itype == idx_string ||
				indexScratch->idx->idx_rpt[segment].idx_itype == idx_byte_array ||
				indexScratch->idx->idx_rpt[segment].idx_itype == idx_metadata ||
				indexScratch->idx->idx_rpt[segment].idx_itype >= idx_first_intl_string) ||
			!OPT_computable(optimizer->opt_csb, value, stream, false, false))
		{
			return false;
		}
	}

	return true;
}

IndexRelationship::IndexRelationship()
{
/**************************************
 *
 *	I n d e x R e l a t i on s h i p
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	stream = 0;
	unique = false;
	cost = 0;
}


InnerJoinStreamInfo::InnerJoinStreamInfo(MemoryPool& p) :
	indexedRelationships(p)
{
/**************************************
 *
 *	I n n e r J o i n S t r e a m I n f o
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	stream = 0;
	baseUnique = false;
	baseCost = 0;
	baseIndexes = 0;
	baseConjunctionMatches = 0;
	used = false;

	indexedRelationships.shrink(0);
	previousExpectedStreams = 0;
}

bool InnerJoinStreamInfo::independent() const
{
/**************************************
 *
 *	i n d e p e n d e n t
 *
 **************************************
 *
 *  Return true if this stream can't be
 *  used by other streams and it can't
 *  use index retrieval based on other
 *  streams.
 *
 **************************************/
	return (indexedRelationships.getCount() == 0) && (previousExpectedStreams == 0);
}


OptimizerInnerJoin::OptimizerInnerJoin(MemoryPool& p, OptimizerBlk* opt, const UCHAR* streams,
		/*RiverStack& river_stack,*/ jrd_nod** sort_clause,
		jrd_nod** project_clause, jrd_nod* plan_clause) :
	pool(p), innerStreams(p)
{
/**************************************
 *
 *	O p t i m i z e r I n n e r J o i n
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	tdbb = NULL;
	SET_TDBB(tdbb);
	this->database = tdbb->getDatabase();
	this->optimizer = opt;
	this->csb = this->optimizer->opt_csb;
	this->sort = sort_clause;
	this->project = project_clause;
	this->plan = plan_clause;
	this->remainingStreams = 0;

	innerStreams.grow(streams[0]);
	InnerJoinStreamInfo** innerStream = innerStreams.begin();
	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		innerStream[i] = FB_NEW(p) InnerJoinStreamInfo(p);
		innerStream[i]->stream = streams[i + 1];
	}

	calculateCardinalities();
	calculateStreamInfo();
}

OptimizerInnerJoin::~OptimizerInnerJoin()
{
/**************************************
 *
 *	~O p t i m i z e r I n n e r J o i n
 *
 **************************************
 *
 *  Finish with giving back memory.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		for (size_t j = 0; j < innerStreams[i]->indexedRelationships.getCount(); j++) {
			delete innerStreams[i]->indexedRelationships[j];
		}
		innerStreams[i]->indexedRelationships.clear();
		delete innerStreams[i];
	}
	innerStreams.clear();
}

void OptimizerInnerJoin::calculateCardinalities()
{
/**************************************
 *
 *	c a l c u l a t e C a r d i n a l i t i e s
 *
 **************************************
 *
 *  Get the cardinality for every stream.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		fb_assert(csb_tail);
		if (!csb_tail->csb_cardinality)
		{
			jrd_rel* relation = csb_tail->csb_relation;
			fb_assert(relation);
			const Format* format = CMP_format(tdbb, csb, (USHORT)innerStreams[i]->stream);
			fb_assert(format);
			csb_tail->csb_cardinality = OPT_getRelationCardinality(tdbb, relation, format);
		}
	}
}

void OptimizerInnerJoin::calculateStreamInfo()
{
/**************************************
 *
 *	c a l c u l a t e S t r e a m I n f o
 *
 **************************************
 *
 *  Calculate the needed information for
 *  all streams.
 *
 **************************************/

	size_t i = 0;
	// First get the base cost without any relation to an other inner join stream.
	for (i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		csb_tail->csb_flags |= csb_active;

		OptimizerRetrieval* optimizerRetrieval = FB_NEW(pool)
			OptimizerRetrieval(pool, optimizer, innerStreams[i]->stream, false, false, NULL);
		InversionCandidate* candidate = optimizerRetrieval->getCost();
		innerStreams[i]->baseCost = candidate->cost;
		innerStreams[i]->baseIndexes = candidate->indexes;
		innerStreams[i]->baseUnique = candidate->unique;
		innerStreams[i]->baseConjunctionMatches = candidate->matches.getCount();
		delete candidate;
		delete optimizerRetrieval;

		csb_tail->csb_flags &= ~csb_active;
	}

	for (i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		csb_tail->csb_flags |= csb_active;

		// Find streams that have a indexed relationship to this
		// stream and add the information.
		for (size_t j = 0; j < innerStreams.getCount(); j++)
		{
			if (innerStreams[j]->stream != innerStreams[i]->stream) {
				getIndexedRelationship(innerStreams[i], innerStreams[j]);
			}
		}

		csb_tail->csb_flags &= ~csb_active;
	}

	// Sort the streams based on independecy and cost.
	// Except when a PLAN was forced.
	if (!plan && (innerStreams.getCount() > 1))
	{
		StreamInfoList tempStreams(pool);

		for (i = 0; i < innerStreams.getCount(); i++)
		{
			size_t index = 0;
			for (; index < tempStreams.getCount(); index++)
			{
				// First those streams which can't be used by other streams
				// or can't depend on a stream.
				if (innerStreams[i]->independent() && !tempStreams[index]->independent()) {
					break;
				}
				// Next those with the lowest previous expected streams
				int compare = innerStreams[i]->previousExpectedStreams -
					tempStreams[index]->previousExpectedStreams;
				if (compare < 0) {
					break;
				}
				if (compare == 0)
				{
					// Next those with the cheapest base cost
					if (innerStreams[i]->baseCost < tempStreams[index]->baseCost) {
						break;
					}
				}
			}
			tempStreams.insert(index, innerStreams[i]);
		}

		// Finally update the innerStreams with the sorted streams
		innerStreams.clear();
		innerStreams.join(tempStreams);
	}
}

bool OptimizerInnerJoin::cheaperRelationship(IndexRelationship* checkRelationship,
		IndexRelationship* withRelationship) const
{
/**************************************
 *
 *	c h e a p e r R e l a t i o n s h i p
 *
 **************************************
 *
 *  Return true if checking relationship
 *	is cheaper as withRelationship.
 *
 **************************************/
	if (checkRelationship->cost == 0) {
		return true;
	}
	if (withRelationship->cost == 0) {
		return false;
	}

	const double compareValue = checkRelationship->cost / withRelationship->cost;
	if (compareValue >= 0.98 && compareValue <= 1.02)
	{
		// cost is nearly the same, now check on cardinality
		if (checkRelationship->cardinality < withRelationship->cardinality) {
			return true;
		}
	}
	else if (checkRelationship->cost < withRelationship->cost) {
		return true;
	}

	return false;
}

void OptimizerInnerJoin::estimateCost(USHORT stream, double *cost,
	double *resulting_cardinality) const
{
/**************************************
 *
 *	e s t i m a t e C o s t
 *
 **************************************
 *
 *  Estimate the cost for the stream.
 *
 **************************************/
	// Create the optimizer retrieval generation class and calculate
	// which indexes will be used and the total estimated selectivity will be returned
	OptimizerRetrieval* optimizerRetrieval = FB_NEW(pool)
		OptimizerRetrieval(pool, optimizer, stream, false, false, NULL);

	const InversionCandidate* candidate = optimizerRetrieval->getCost();
	*cost = candidate->cost;

	// Calculate cardinality
	const CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	const double cardinality = csb_tail->csb_cardinality * candidate->selectivity;

	*resulting_cardinality = MAX(cardinality, MINIMUM_CARDINALITY);

	delete candidate;
	delete optimizerRetrieval;
}

int OptimizerInnerJoin::findJoinOrder()
{
/**************************************
 *
 *	f i n d J o i n O r d e r
 *
 **************************************
 *
 *  Find the best order out of the streams.
 *  First return a stream if it can't use
 *  a index based on a previous stream and
 *  it can't be used by another stream.
 *  Next loop through the remaining streams
 *  and find the best order.
 *
 **************************************/

	optimizer->opt_best_count = 0;

#ifdef OPT_DEBUG
	// Debug
	printStartOrder();
#endif

	size_t i = 0;
	remainingStreams = 0;
	for (i = 0; i < innerStreams.getCount(); i++)
	{
		if (!innerStreams[i]->used)
		{
			remainingStreams++;
			if (innerStreams[i]->independent())
			{
				if (!optimizer->opt_best_count || innerStreams[i]->baseCost < optimizer->opt_best_cost)
				{
					optimizer->opt_streams[0].opt_best_stream = innerStreams[i]->stream;
					optimizer->opt_best_count = 1;
					optimizer->opt_best_cost = innerStreams[i]->baseCost;
				}
			}
		}
	}

	if (optimizer->opt_best_count == 0)
	{
		IndexedRelationships indexedRelationships(pool);
		for (i = 0; i < innerStreams.getCount(); i++)
		{
			if (!innerStreams[i]->used)
			{
				indexedRelationships.clear();
				findBestOrder(0, innerStreams[i], &indexedRelationships, (double) 0, (double) 1);

				if (plan)
				{
					// If a explicit PLAN was specified we should be ready;
					break;
				}
#ifdef OPT_DEBUG
				// Debug
				printProcessList(&indexedRelationships, innerStreams[i]->stream);
#endif
			}
		}
	}

	// Mark streams as used
	for (int stream = 0; stream < optimizer->opt_best_count; stream++)
	{
		InnerJoinStreamInfo* streamInfo = getStreamInfo(optimizer->opt_streams[stream].opt_best_stream);
		streamInfo->used = true;
	}

#ifdef OPT_DEBUG
	// Debug
	printBestOrder();
#endif

	return optimizer->opt_best_count;
}

void OptimizerInnerJoin::findBestOrder(int position, InnerJoinStreamInfo* stream,
	IndexedRelationships* processList, double cost, double cardinality)
{
/**************************************
 *
 *	f i n d B e s t O r d e r
 *
 **************************************
 *  Make different combinations to find
 *  out the join order.
 *  For every position we start with the
 *  stream that has the best selectivity
 *  for that position. If we've have
 *  used up all our streams after that
 *  we assume we're done.
 *
 **************************************/

	fb_assert(processList);

	// do some initializations.
	csb->csb_rpt[stream->stream].csb_flags |= csb_active;
	optimizer->opt_streams[position].opt_stream_number = stream->stream;
	position++;
	const OptimizerBlk::opt_stream* order_end = optimizer->opt_streams.begin() + position;

	// Save the various flag bits from the optimizer block to reset its
	// state after each test.
	Firebird::HalfStaticArray<bool, OPT_STATIC_ITEMS> streamFlags(pool);
	streamFlags.grow(innerStreams.getCount());
	for (size_t i = 0; i < streamFlags.getCount(); i++) {
		streamFlags[i] = innerStreams[i]->used;
	}

	// Compute delta and total estimate cost to fetch this stream.
	double position_cost, position_cardinality, new_cost = 0, new_cardinality = 0;

	if (!plan)
	{
		estimateCost(stream->stream, &position_cost, &position_cardinality);
		new_cost = cost + cardinality * position_cost;
		new_cardinality = position_cardinality * cardinality;
	}

	optimizer->opt_combinations++;
	// If the partial order is either longer than any previous partial order,
	// or the same length and cheap, save order as "best".
	if (position > optimizer->opt_best_count ||
		(position == optimizer->opt_best_count && new_cost < optimizer->opt_best_cost))
	{
		optimizer->opt_best_count = position;
		optimizer->opt_best_cost = new_cost;
		for (OptimizerBlk::opt_stream* tail = optimizer->opt_streams.begin(); tail < order_end; tail++) {
			tail->opt_best_stream = tail->opt_stream_number;
		}
	}

#ifdef OPT_DEBUG
	// Debug information
	printFoundOrder(position, position_cost, position_cardinality, new_cost, new_cardinality);
#endif

	// mark this stream as "used" in the sense that it is already included
	// in this particular proposed stream ordering.
	stream->used = true;
	bool done = false;

	// if we've used up all the streams there's no reason to go any further.
	if (position == remainingStreams) {
		done = true;
	}
	// If we know a combination with all streams used and the
	// current cost is higher as the one from the best we're done.
	if ((optimizer->opt_best_count == remainingStreams) && (optimizer->opt_best_cost < new_cost))
	{
		done = true;
	}

	if (!done && !plan)
	{
		// Add these relations to the processing list
		for (size_t j = 0; j < stream->indexedRelationships.getCount(); j++)
		{
			IndexRelationship* relationship = stream->indexedRelationships[j];
			InnerJoinStreamInfo* relationStreamInfo = getStreamInfo(relationship->stream);
			if (!relationStreamInfo->used)
			{
				bool found = false;
				IndexRelationship** processRelationship = processList->begin();
				size_t index;
				for (index = 0; index < processList->getCount(); index++)
				{
					if (relationStreamInfo->stream == processRelationship[index]->stream)
					{
						// If the cost of this relationship is cheaper then remove the
						// old relationship and add this one.
						if (cheaperRelationship(relationship, processRelationship[index]))
						{
							processList->remove(index);
							break;
						}

						found = true;
						break;
					}
				}
				if (!found)
				{
					// Add relationship sorted on cost (cheapest as first)
					IndexRelationship** relationships = processList->begin();
					for (index = 0; index < processList->getCount(); index++)
					{
						if (cheaperRelationship(relationship, relationships[index])) {
							break;
						}
					}
					processList->insert(index, relationship);
				}
			}
		}

		IndexRelationship** nextRelationship = processList->begin();
		for (size_t j = 0; j < processList->getCount(); j++)
		{
			InnerJoinStreamInfo* relationStreamInfo = getStreamInfo(nextRelationship[j]->stream);
			if (!relationStreamInfo->used)
			{
				findBestOrder(position, relationStreamInfo, processList, new_cost, new_cardinality);
				break;
			}
		}
	}

	if (plan)
	{
		// If a explicit PLAN was specific pick the next relation.
		// The order in innerStreams is expected to be exactly the order as
		// specified in the explicit PLAN.
		for (size_t j = 0; j < innerStreams.getCount(); j++)
		{
			InnerJoinStreamInfo* nextStream = innerStreams[j];
			if (!nextStream->used)
			{
				findBestOrder(position, nextStream, processList, new_cost, new_cardinality);
				break;
			}
		}
	}

	// Clean up from any changes made for compute the cost for this stream
	csb->csb_rpt[stream->stream].csb_flags &= ~csb_active;
	for (size_t i = 0; i < streamFlags.getCount(); i++) {
		innerStreams[i]->used = streamFlags[i];
	}
}

void OptimizerInnerJoin::getIndexedRelationship(InnerJoinStreamInfo* baseStream,
	InnerJoinStreamInfo* testStream)
{
/**************************************
 *
 *	g e t I n d e x e d R e l a t i o n s h i p
 *
 **************************************
 *
 *  Check if the testStream can use a index
 *  when the baseStream is active. If so
 *  then we create a indexRelationship
 *  and fill it with the needed information.
 *  The reference is added to the baseStream
 *  and the baseStream is added as previous
 *  expected stream to the testStream.
 *
 **************************************/

	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[testStream->stream];
	csb_tail->csb_flags |= csb_active;

	OptimizerRetrieval* optimizerRetrieval = FB_NEW(pool)
		OptimizerRetrieval(pool, optimizer, testStream->stream, false, false, NULL);
	InversionCandidate* candidate = optimizerRetrieval->getCost();

	if (candidate->dependentFromStreams.exist(baseStream->stream))
	{
		// If we could use more conjunctions on the testing stream
		// with the base stream active as without the base stream
		// then the test stream has a indexed relationship with the base stream.
		IndexRelationship* indexRelationship = FB_NEW(pool) IndexRelationship();
		indexRelationship->stream = testStream->stream;
		indexRelationship->unique = candidate->unique;
		indexRelationship->cost = candidate->cost;
		indexRelationship->cardinality = csb_tail->csb_cardinality * candidate->selectivity;

		// indexRelationship are kept sorted on cost and unique in the indexRelations array.
		// The unique and cheapest indexed relatioships are on the first position.
		size_t index = 0;
		for (; index < baseStream->indexedRelationships.getCount(); index++)
		{
			if (cheaperRelationship(indexRelationship, baseStream->indexedRelationships[index])) {
				break;
			}
		}
		baseStream->indexedRelationships.insert(index, indexRelationship);
		testStream->previousExpectedStreams++;
	}
	delete candidate;
	delete optimizerRetrieval;

	csb_tail->csb_flags &= ~csb_active;
}

InnerJoinStreamInfo* OptimizerInnerJoin::getStreamInfo(int stream)
{
/**************************************
 *
 *	g e t S t r e a m I n f o
 *
 **************************************
 *
 *  Return stream information based on
 *  the stream number.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		if (innerStreams[i]->stream == stream) {
			return innerStreams[i];
		}
	}

	// We should never come here
	fb_assert(false);
	return NULL;
}

#ifdef OPT_DEBUG
void OptimizerInnerJoin::printBestOrder() const
{
/**************************************
 *
 *	p r i n t B e s t O r d e r
 *
 **************************************
 *
 *  Dump finally selected stream order.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, " best order, streams: ");
	for (int i = 0; i < optimizer->opt_best_count; i++)
	{
		if (i == 0) {
			fprintf(opt_debug_file, "%d", optimizer->opt_streams[i].opt_best_stream);
		}
		else {
			fprintf(opt_debug_file, ", %d", optimizer->opt_streams[i].opt_best_stream);
		}
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printFoundOrder(int position, double positionCost,
		double positionCardinality, double cost, double cardinality) const
{
/**************************************
 *
 *	p r i n t F o u n d O r d e r
 *
 **************************************
 *
 *  Dump currently passed streams to a
 *  debug file.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "  position %2.2d:", position);
	fprintf(opt_debug_file, " pos. cardinality(%10.2f) pos. cost(%10.2f)", positionCardinality, positionCost);
	fprintf(opt_debug_file, " cardinality(%10.2f) cost(%10.2f)", cardinality, cost);
	fprintf(opt_debug_file, ", streams: ", position);
	const OptimizerBlk::opt_stream* tail = optimizer->opt_streams.begin();
	const OptimizerBlk::opt_stream* const order_end = tail + position;
	for (; tail < order_end; tail++)
	{
		if (tail == optimizer->opt_streams.begin()) {
			fprintf(opt_debug_file, "%d", tail->opt_stream_number);
		}
		else {
			fprintf(opt_debug_file, ", %d", tail->opt_stream_number);
		}
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printProcessList(const IndexedRelationships* processList,
	int stream) const
{
/**************************************
 *
 *	p r i n t P r o c e s s L i s t
 *
 **************************************
 *
 *  Dump the processlist to a debug file.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "   basestream %d, relationships: stream(cost)", stream);
	const IndexRelationship* const* relationships = processList->begin();
	for (int i = 0; i < processList->getCount(); i++) {
		fprintf(opt_debug_file, ", %d (%1.2f)", relationships[i]->stream, relationships[i]->cost);
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printStartOrder() const
{
/**************************************
 *
 *	p r i n t B e s t O r d e r
 *
 **************************************
 *
 *  Dump finally selected stream order.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "Start join order: with stream(baseCost)");
	bool firstStream = true;
	for (int i = 0; i < innerStreams.getCount(); i++)
	{
		if (!innerStreams[i]->used) {
			fprintf(opt_debug_file, ", %d (%1.2f)", innerStreams[i]->stream, innerStreams[i]->baseCost);
		}
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}
#endif


} // namespace
