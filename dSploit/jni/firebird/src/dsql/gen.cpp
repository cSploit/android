/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		gen.cpp
 *	DESCRIPTION:	Routines to generate BLR.
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
 * Contributor(s): ______________________________________
 * 2001.6.21 Claudio Valderrama: BREAK and SUBSTRING.
 * 2001.07.28 John Bellardo:  Added code to generate blr_skip.
 * 2002.07.30 Arno Brinkman:  Added code, procedures to generate COALESCE, CASE
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * 2004.01.16 Vlad Horsun: Added support for default parameters and
 *   EXECUTE BLOCK statement
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <string.h>
#include <stdio.h>
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/constants.h"
#include "../jrd/intl.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/misc_func.h"
#include "../dsql/utld_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/why_proto.h"
#include "gen/iberror.h"
#include "../common/StatusArg.h"

using namespace Jrd;
using namespace Dsql;
using namespace Firebird;

static void gen_aggregate(CompiledStatement*, const dsql_nod*);
static void gen_cast(CompiledStatement*, const dsql_nod*);
static void gen_coalesce(CompiledStatement*, const dsql_nod*);
static void gen_constant(CompiledStatement*, const dsc*, bool);
static void gen_constant(CompiledStatement*, dsql_nod*, bool);
static void gen_error_condition(CompiledStatement*, const dsql_nod*);
static void gen_exec_stmt(CompiledStatement* statement, const dsql_nod* node);
static void gen_field(CompiledStatement*, const dsql_ctx*, const dsql_fld*, dsql_nod*);
static void gen_for_select(CompiledStatement*, const dsql_nod*);
static void gen_gen_id(CompiledStatement*, const dsql_nod*);
static void gen_join_rse(CompiledStatement*, const dsql_nod*);
static void gen_map(CompiledStatement*, dsql_map*);
static inline void gen_optional_expr(CompiledStatement*, const UCHAR code, dsql_nod*);
static void gen_parameter(CompiledStatement*, const dsql_par*);
static void gen_plan(CompiledStatement*, const dsql_nod*);
static void gen_relation(CompiledStatement*, dsql_ctx*);
static void gen_rse(CompiledStatement*, const dsql_nod*);
static void gen_searched_case(CompiledStatement*, const dsql_nod*);
static void gen_select(CompiledStatement*, dsql_nod*);
static void gen_simple_case(CompiledStatement*, const dsql_nod*);
static void gen_sort(CompiledStatement*, dsql_nod*);
static void gen_statement(CompiledStatement*, const dsql_nod*);
static void gen_sys_function(CompiledStatement*, const dsql_nod*);
static void gen_table_lock(CompiledStatement*, const dsql_nod*, USHORT);
static void gen_udf(CompiledStatement*, const dsql_nod*);
static void gen_union(CompiledStatement*, const dsql_nod*);
static void stuff_context(CompiledStatement*, const dsql_ctx*);
static void stuff_cstring(CompiledStatement*, const char*);
static void stuff_meta_string(CompiledStatement*, const char*);
static void stuff_string(CompiledStatement*, const char*, int);
static void stuff_string(CompiledStatement* statement, const Firebird::MetaName& name);
static void stuff_word(CompiledStatement*, USHORT);

// STUFF is defined in dsql.h for use in common with ddl.c

// The following are passed as the third argument to gen_constant
const bool NEGATE_VALUE = true;
const bool USE_VALUE    = false;


void GEN_hidden_variables(CompiledStatement* statement, bool inExpression)
{
/**************************************
 *
 *	G E N _ h i d d e n _ v a r i a b l e s
 *
 **************************************
 *
 * Function
 *	Emit BLR for hidden variables.
 *
 **************************************/
	if (statement->req_hidden_vars.isEmpty())
		return;

	if (inExpression)
	{
		stuff(statement, blr_stmt_expr);
		if (statement->req_hidden_vars.getCount() > 1)
			stuff(statement, blr_begin);
	}

	for (DsqlNodStack::const_iterator i(statement->req_hidden_vars); i.hasData(); ++i)
	{
		const dsql_nod* varNode = i.object()->nod_arg[1];
		const dsql_var* var = (dsql_var*) varNode->nod_arg[e_var_variable];
		statement->append_uchar(blr_dcl_variable);
		statement->append_ushort(var->var_variable_number);
		GEN_descriptor(statement, &varNode->nod_desc, true);
	}

	if (inExpression && statement->req_hidden_vars.getCount() > 1)
		stuff(statement, blr_end);

	// Clear it for GEN_expr not regenerate them.
	statement->req_hidden_vars.clear();
}


/**

 	GEN_expr

    @brief	Generate blr for an arbitrary expression.


    @param statement
    @param node

 **/
void GEN_expr(CompiledStatement* statement, dsql_nod* node)
{
	UCHAR blr_operator;
	const dsql_ctx* context;

	switch (node->nod_type)
	{
	case nod_alias:
		GEN_expr(statement, node->nod_arg[e_alias_value]);
		return;

	case nod_aggregate:
		gen_aggregate(statement, node);
		return;

	case nod_constant:
		gen_constant(statement, node, USE_VALUE);
		return;

	case nod_derived_field:
		// ASF: If we are not referencing a field, we should evaluate the expression based on
		// a set (ORed) of contexts. If any of them are in a valid position the expression is
		// evaluated, otherwise a NULL will be returned. This is fix for CORE-1246.
		if (node->nod_arg[e_derived_field_value]->nod_type != nod_derived_field &&
			node->nod_arg[e_derived_field_value]->nod_type != nod_field &&
			node->nod_arg[e_derived_field_value]->nod_type != nod_dbkey &&
			node->nod_arg[e_derived_field_value]->nod_type != nod_map)
		{
			const dsql_ctx* ctx = (dsql_ctx*) node->nod_arg[e_derived_field_context];

			if (ctx->ctx_main_derived_contexts.hasData())
			{
				if (ctx->ctx_main_derived_contexts.getCount() > MAX_UCHAR)
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
							  Arg::Gds(isc_imp_exc) <<
							  Arg::Gds(isc_ctx_too_big));
				}

				stuff(statement, blr_derived_expr);
				stuff(statement, ctx->ctx_main_derived_contexts.getCount());

				for (DsqlContextStack::const_iterator stack(ctx->ctx_main_derived_contexts);
					 stack.hasData(); ++stack)
				{
					fb_assert(stack.object()->ctx_context <= MAX_UCHAR);
					stuff(statement, stack.object()->ctx_context);
				}
			}
		}
		GEN_expr(statement, node->nod_arg[e_derived_field_value]);
		return;

	case nod_extract:
		stuff(statement, blr_extract);
		stuff(statement, node->nod_arg[e_extract_part]->getSlong());
		GEN_expr(statement, node->nod_arg[e_extract_value]);
		return;

	case nod_strlen:
		stuff(statement, blr_strlen);
		stuff(statement, node->nod_arg[e_strlen_type]->getSlong());
		GEN_expr(statement, node->nod_arg[e_strlen_value]);
		return;

	case nod_dbkey:
		node = node->nod_arg[0];
		context = (dsql_ctx*) node->nod_arg[e_rel_context];
		stuff(statement, blr_dbkey);
		stuff_context(statement, context);
		return;

	case nod_rec_version:
		node = node->nod_arg[0];
		context = (dsql_ctx*) node->nod_arg[e_rel_context];
		stuff(statement, blr_record_version);
		stuff_context(statement, context);
		return;

	case nod_dom_value:
		stuff(statement, blr_fid);
		stuff(statement, 0);				// Context
		stuff_word(statement, 0);			// Field id
		return;

	case nod_field:
		gen_field(statement,
				  (dsql_ctx*) node->nod_arg[e_fld_context],
				  (dsql_fld*) node->nod_arg[e_fld_field],
				  node->nod_arg[e_fld_indices]);
		return;

	case nod_user_name:
		stuff(statement, blr_user_name);
		return;

	case nod_current_time:
		if (node->nod_arg[0]) {
			const dsql_nod* const_node = node->nod_arg[0];
			fb_assert(const_node->nod_type == nod_constant);
			const int precision = (int) const_node->getSlong();
			stuff(statement, blr_current_time2);
			stuff(statement, precision);
		}
		else {
			stuff(statement, blr_current_time);
		}
		return;

	case nod_current_timestamp:
		if (node->nod_arg[0]) {
			const dsql_nod* const_node = node->nod_arg[0];
			fb_assert(const_node->nod_type == nod_constant);
			const int precision = (int) const_node->getSlong();
			stuff(statement, blr_current_timestamp2);
			stuff(statement, precision);
		}
		else {
			stuff(statement, blr_current_timestamp);
		}
		return;

	case nod_current_date:
		stuff(statement, blr_current_date);
		return;

	case nod_current_role:
		stuff(statement, blr_current_role);
		return;

	case nod_udf:
		gen_udf(statement, node);
		return;

	case nod_sys_function:
		gen_sys_function(statement, node);
		return;

	case nod_variable:
		{
			const dsql_var* variable = (dsql_var*) node->nod_arg[e_var_variable];
			if (variable->var_type == VAR_input) {
				stuff(statement, blr_parameter2);
				stuff(statement, variable->var_msg_number);
				stuff_word(statement, variable->var_msg_item);
				stuff_word(statement, variable->var_msg_item + 1);
			}
			else {
				stuff(statement, blr_variable);
				stuff_word(statement, variable->var_variable_number);
			}
		}
		return;

	case nod_join:
		gen_join_rse(statement, node);
		return;

	case nod_map:
		{
			const dsql_map* map = (dsql_map*) node->nod_arg[e_map_map];
			context = (dsql_ctx*) node->nod_arg[e_map_context];
			stuff(statement, blr_fid);
			stuff_context(statement, context);
			stuff_word(statement, map->map_position);
		}
		return;

	case nod_parameter:
		gen_parameter(statement, (dsql_par*) node->nod_arg[e_par_parameter]);
		return;

	case nod_relation:
		gen_relation(statement, (dsql_ctx*) node->nod_arg[e_rel_context]);
		return;

	case nod_rse:
		gen_rse(statement, node);
		return;

	case nod_derived_table:
		gen_rse(statement, node->nod_arg[e_derived_table_rse]);
		return;

	case nod_exists:
		stuff(statement, blr_any);
		gen_rse(statement, node->nod_arg[0]);
		return;

	case nod_singular:
		stuff(statement, blr_unique);
		gen_rse(statement, node->nod_arg[0]);
		return;

	case nod_agg_count:
		if (node->nod_count)
			blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ?
				blr_agg_count_distinct : blr_agg_count2;
		else
			blr_operator = blr_agg_count;
		break;

	case nod_agg_min:
		blr_operator = blr_agg_min;
		break;
	case nod_agg_max:
		blr_operator = blr_agg_max;
		break;

	case nod_agg_average:
		blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ?
			blr_agg_average_distinct : blr_agg_average;
		break;

	case nod_agg_total:
		blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ? blr_agg_total_distinct : blr_agg_total;
		break;

	case nod_agg_average2:
		blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ?
			blr_agg_average_distinct : blr_agg_average;
		break;

	case nod_agg_total2:
		blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ? blr_agg_total_distinct : blr_agg_total;
		break;

	case nod_agg_list:
		blr_operator = (node->nod_flags & NOD_AGG_DISTINCT) ? blr_agg_list_distinct : blr_agg_list;
		break;

	case nod_and:
		blr_operator = blr_and;
		break;
	case nod_or:
		blr_operator = blr_or;
		break;
	case nod_not:
		blr_operator = blr_not;
		break;
	case nod_eql_all:
	case nod_eql_any:
	case nod_eql:
		blr_operator = blr_eql;
		break;
	case nod_equiv:
		blr_operator = blr_equiv;
		break;
	case nod_neq_all:
	case nod_neq_any:
	case nod_neq:
		blr_operator = blr_neq;
		break;
	case nod_gtr_all:
	case nod_gtr_any:
	case nod_gtr:
		blr_operator = blr_gtr;
		break;
	case nod_leq_all:
	case nod_leq_any:
	case nod_leq:
		blr_operator = blr_leq;
		break;
	case nod_geq_all:
	case nod_geq_any:
	case nod_geq:
		blr_operator = blr_geq;
		break;
	case nod_lss_all:
	case nod_lss_any:
	case nod_lss:
		blr_operator = blr_lss;
		break;
	case nod_between:
		blr_operator = blr_between;
		break;
	case nod_containing:
		blr_operator = blr_containing;
		break;
	case nod_similar:
		stuff(statement, blr_similar);
		GEN_expr(statement, node->nod_arg[e_similar_value]);
		GEN_expr(statement, node->nod_arg[e_similar_pattern]);

		if (node->nod_arg[e_similar_escape])
		{
			stuff(statement, 1);
			GEN_expr(statement, node->nod_arg[e_similar_escape]);
		}
		else
			stuff(statement, 0);

		return;

	case nod_starting:
		blr_operator = blr_starting;
		break;
	case nod_missing:
		blr_operator = blr_missing;
		break;

	case nod_like:
		blr_operator = (node->nod_count == 2) ? blr_like : blr_ansi_like;
		break;

	case nod_add:
		blr_operator = blr_add;
		break;
	case nod_subtract:
		blr_operator = blr_subtract;
		break;
	case nod_multiply:
		blr_operator = blr_multiply;
		break;

	case nod_negate:
		{
			dsql_nod* child = node->nod_arg[0];
			if (child->nod_type == nod_constant && DTYPE_IS_NUMERIC(child->nod_desc.dsc_dtype))
			{
				gen_constant(statement, child, NEGATE_VALUE);
				return;
			}
		}
		blr_operator = blr_negate;
		break;

	case nod_divide:
		blr_operator = blr_divide;
		break;
	case nod_add2:
		blr_operator = blr_add;
		break;
	case nod_subtract2:
		blr_operator = blr_subtract;
		break;
	case nod_multiply2:
		blr_operator = blr_multiply;
		break;
	case nod_divide2:
		blr_operator = blr_divide;
		break;
	case nod_concatenate:
		blr_operator = blr_concatenate;
		break;
	case nod_null:
		blr_operator = blr_null;
		break;
	case nod_any:
		blr_operator = blr_any;
		break;
	case nod_ansi_any:
		blr_operator = blr_ansi_any;
		break;
	case nod_ansi_all:
		blr_operator = blr_ansi_all;
		break;
	case nod_via:
		blr_operator = blr_via;
		break;

	case nod_internal_info:
		blr_operator = blr_internal_info;
		break;
	case nod_upcase:
		blr_operator = blr_upcase;
		break;
	case nod_lowcase:
		blr_operator = blr_lowcase;
		break;
	case nod_substr:
        blr_operator = blr_substring;
        break;
	case nod_cast:
		gen_cast(statement, node);
		return;
	case nod_gen_id:
	case nod_gen_id2:
		gen_gen_id(statement, node);
		return;
    case nod_coalesce:
		gen_coalesce(statement, node);
		return;
    case nod_simple_case:
		gen_simple_case(statement, node);
		return;
    case nod_searched_case:
		gen_searched_case(statement, node);
		return;

	case nod_average:
	//case nod_count:
	case nod_from:
	case nod_max:
	case nod_min:
	case nod_total:
		switch (node->nod_type)
		{
		case nod_average:
			blr_operator = blr_average;
			break;
		//case nod_count:
		//	blr_operator = blr_count;
		// count2
		//	blr_operator = node->nod_arg[0]->nod_arg[e_rse_items] ? blr_count2 : blr_count;

		//	break;
		case nod_from:
			blr_operator = blr_from;
			break;
		case nod_max:
			blr_operator = blr_maximum;
			break;
		case nod_min:
			blr_operator = blr_minimum;
			break;
		case nod_total:
			blr_operator = blr_total;
			break;

		default:
			break;
		}

		stuff(statement, blr_operator);
		gen_rse(statement, node->nod_arg[0]);
		if (blr_operator != blr_count)
			GEN_expr(statement, node->nod_arg[0]->nod_arg[e_rse_items]);
		return;

	case nod_trim:
		stuff(statement, blr_trim);
		stuff(statement, node->nod_arg[e_trim_specification]->getSlong());

		if (node->nod_arg[e_trim_characters])
		{
			stuff(statement, blr_trim_characters);
			GEN_expr(statement, node->nod_arg[e_trim_characters]);
		}
		else
			stuff(statement, blr_trim_spaces);

		GEN_expr(statement, node->nod_arg[e_trim_value]);
		return;

	case nod_assign:
		stuff(statement, blr_assignment);
		GEN_expr(statement, node->nod_arg[0]);
		GEN_expr(statement, node->nod_arg[1]);
		return;

	case nod_hidden_var:
		stuff(statement, blr_stmt_expr);

		// If it was not pre-declared, declare it now.
		if (statement->req_hidden_vars.hasData())
		{
			const dsql_var* var = (dsql_var*) node->nod_arg[e_hidden_var_var]->nod_arg[e_var_variable];

			stuff(statement, blr_begin);
			statement->append_uchar(blr_dcl_variable);
			statement->append_ushort(var->var_variable_number);
			GEN_descriptor(statement, &node->nod_arg[e_hidden_var_var]->nod_desc, true);
		}

		stuff(statement, blr_assignment);
		GEN_expr(statement, node->nod_arg[e_hidden_var_expr]);
		GEN_expr(statement, node->nod_arg[e_hidden_var_var]);

		if (statement->req_hidden_vars.hasData())
			stuff(statement, blr_end);

		GEN_expr(statement, node->nod_arg[e_hidden_var_var]);
		return;

	default:
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_dsql_internal_err) <<
				  // expression evaluation not supported
				  Arg::Gds(isc_expression_eval_err) <<
				  Arg::Gds(isc_dsql_eval_unknode) << Arg::Num(node->nod_type));
	}

	stuff(statement, blr_operator);

	dsql_nod* const* ptr = node->nod_arg;
	for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
	{
		GEN_expr(statement, *ptr);
	}

	// Check whether the node we just processed is for a dialect 3
	// operation which gives a different result than the corresponding
	// operation in dialect 1.  If it is, and if the client dialect is 2,
	// issue a warning about the difference.

	switch (node->nod_type)
	{
	case nod_add2:
	case nod_subtract2:
	case nod_multiply2:
	case nod_divide2:
	case nod_agg_total2:
	case nod_agg_average2:
		dsc desc;
		MAKE_desc(statement, &desc, node, NULL);

		if ((node->nod_flags & NOD_COMP_DIALECT) &&
			(statement->req_client_dialect == SQL_DIALECT_V6_TRANSITION))
		{
			const char* s = 0;
			char message_buf[8];

			switch (node->nod_type)
			{
			case nod_add2:
				s = "add";
				break;
			case nod_subtract2:
				s = "subtract";
				break;
			case nod_multiply2:
				s = "multiply";
				break;
			case nod_divide2:
				s = "divide";
				break;
			case nod_agg_total2:
				s = "sum";
				break;
			case nod_agg_average2:
				s = "avg";
				break;
			default:
				sprintf(message_buf, "blr %d", (int) blr_operator);
				s = message_buf;
			}
			ERRD_post_warning(Arg::Warning(isc_dsql_dialect_warning_expr) << Arg::Str(s));
		}
	}
}

/**

 	GEN_port

    @brief	Generate a port from a message.  Feel free to rearrange the
 	order of parameters.


    @param statement
    @param message

 **/
void GEN_port(CompiledStatement* statement, dsql_msg* message)
{
	thread_db* tdbb = JRD_get_thread_data();
	Attachment* att = tdbb->getAttachment();

//	if (statement->req_blr_string) {
		stuff(statement, blr_message);
		stuff(statement, message->msg_number);
		stuff_word(statement, message->msg_parameter);
//	}

    dsql_par* parameter;

	ULONG offset = 0;
	USHORT number = 0;
	for (parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		parameter->par_parameter = number++;

		const USHORT fromCharSet = parameter->par_desc.getCharSet();
		const USHORT toCharSet = (fromCharSet == CS_NONE || fromCharSet == CS_BINARY) ?
			fromCharSet : att->att_charset;

		if (parameter->par_desc.dsc_dtype <= dtype_any_text &&
			att->att_charset != CS_NONE && att->att_charset != CS_BINARY)
		{
			USHORT adjust = 0;
			if (parameter->par_desc.dsc_dtype == dtype_varying)
				adjust = sizeof(USHORT);
			else if (parameter->par_desc.dsc_dtype == dtype_cstring)
				adjust = 1;

			parameter->par_desc.dsc_length -= adjust;

			const USHORT fromCharSetBPC = METD_get_charset_bpc(statement, fromCharSet);
			const USHORT toCharSetBPC = METD_get_charset_bpc(statement, toCharSet);

			INTL_ASSIGN_TTYPE(&parameter->par_desc, INTL_CS_COLL_TO_TTYPE(toCharSet,
				(fromCharSet == toCharSet ? INTL_GET_COLLATE(&parameter->par_desc) : 0)));

			parameter->par_desc.dsc_length =
				UTLD_char_length_to_byte_length(parameter->par_desc.dsc_length / fromCharSetBPC, toCharSetBPC, adjust);

			parameter->par_desc.dsc_length += adjust;
		}
		else if (ENCODE_ODS(statement->req_dbb->dbb_ods_version,
					statement->req_dbb->dbb_minor_version) >= ODS_11_1 &&
			parameter->par_desc.dsc_dtype == dtype_blob &&
			parameter->par_desc.dsc_sub_type == isc_blob_text &&
			att->att_charset != CS_NONE && att->att_charset != CS_BINARY)
		{
			if (fromCharSet != toCharSet)
				parameter->par_desc.setTextType(toCharSet);
		}

		// For older clients - generate an error should they try and
		// access data types which did not exist in the older dialect
		if (statement->req_client_dialect <= SQL_DIALECT_V5)
			switch (parameter->par_desc.dsc_dtype)
			{
				// In V6.0 - older clients, which we distinguish by
				// their use of SQL DIALECT 0 or 1, are forbidden
				// from selecting values of new datatypes
				case dtype_sql_date:
				case dtype_sql_time:
				case dtype_int64:
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
							  Arg::Gds(isc_dsql_datatype_err) <<
							  Arg::Gds(isc_sql_dialect_datatype_unsupport) <<
							  		Arg::Num(statement->req_client_dialect) <<
									Arg::Str(DSC_dtype_tostring(parameter->par_desc.dsc_dtype)));
					break;
				default:
					// No special action for other data types
					break;
			}

		const USHORT align = type_alignments[parameter->par_desc.dsc_dtype];
		if (align)
			offset = FB_ALIGN(offset, align);
		parameter->par_desc.dsc_address = (UCHAR*)(IPTR) offset;
		offset += parameter->par_desc.dsc_length;
//		if (statement->req_blr_string)
			GEN_descriptor(statement, &parameter->par_desc, false);
	}

	if (offset > MAX_FORMAT_SIZE) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
				  Arg::Gds(isc_imp_exc) <<
				  Arg::Gds(isc_blktoobig));
	}

	message->msg_length = (USHORT) offset;

	// Allocate buffer for message
	const ULONG new_len = message->msg_length + FB_DOUBLE_ALIGN - 1;
	dsql_str* buffer = FB_NEW_RPT(*tdbb->getDefaultPool(), new_len) dsql_str;
	message->msg_buffer = (UCHAR*) FB_ALIGN((U_IPTR) buffer->str_data, FB_DOUBLE_ALIGN);

	// Relocate parameter descriptors to point direction into message buffer

	for (parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		parameter->par_desc.dsc_address = message->msg_buffer + (IPTR) parameter->par_desc.dsc_address;
	}
}


/**

 	GEN_request

    @brief	Generate complete blr for a statement.


    @param statement
    @param node

 **/
void GEN_request( CompiledStatement* statement, dsql_nod* node)
{
	if (statement->req_type == REQ_CREATE_DB || statement->req_type == REQ_DDL)
	{
		DDL_generate(statement, node);
		return;
	}

	if (statement->req_flags & REQ_blr_version4)
		stuff(statement, blr_version4);
	else
		stuff(statement, blr_version5);

	if (statement->req_type == REQ_SAVEPOINT)
	{
		// Do not generate BEGIN..END block around savepoint statement
		// to avoid breaking of savepoint logic
		statement->req_send = NULL;
		statement->req_receive = NULL;
		GEN_statement(statement, node);
	}
	else
	{
		stuff(statement, blr_begin);

		GEN_hidden_variables(statement, false);

		switch (statement->req_type)
		{
		case REQ_SELECT:
		case REQ_SELECT_UPD:
		case REQ_EMBED_SELECT:
			gen_select(statement, node);
			break;
		case REQ_EXEC_BLOCK:
		case REQ_SELECT_BLOCK:
			GEN_statement(statement, node);
			break;
		default:
			{
				dsql_msg* message = statement->req_send;
				if (!message->msg_parameter)
					statement->req_send = NULL;
				else {
					GEN_port(statement, message);
					stuff(statement, blr_receive);
					stuff(statement, message->msg_number);
				}
				message = statement->req_receive;
				if (!message->msg_parameter)
					statement->req_receive = NULL;
				else
					GEN_port(statement, message);
				GEN_statement(statement, node);
			}
		}
		stuff(statement, blr_end);
	}

	stuff(statement, blr_eoc);
}


/**

 	GEN_start_transaction

    @brief	Generate tpb for set transaction.  Use blr string of statement.
 	If a value is not specified, default is not STUFF'ed, let the
 	engine handle it.
 	Do not allow an option to be specified more than once.


    @param statement
    @param tran_node

 **/
void GEN_start_transaction( CompiledStatement* statement, const dsql_nod* tran_node)
{
	SSHORT count = tran_node->nod_count;

	if (!count)
		return;

	const dsql_nod* node = tran_node->nod_arg[0];

	if (!node)
		return;

	// Find out isolation level - if specified. This is required for
	// specifying the correct lock level in reserving clause.

	USHORT lock_level = isc_tpb_shared;

	if (count = node->nod_count)
	{
		while (count--) {
			const dsql_nod* ptr = node->nod_arg[count];

			if (!ptr || ptr->nod_type != nod_isolation)
				continue;

			lock_level = (ptr->nod_flags & NOD_CONSISTENCY) ? isc_tpb_protected : isc_tpb_shared;
		}
	}


   	bool sw_access = false, sw_wait = false, sw_isolation = false,
		sw_reserve = false, sw_lock_timeout = false;
	int misc_flags = 0;

	// Stuff some version info.
	if (count = node->nod_count)
		stuff(statement, isc_tpb_version1);

	while (count--)
	{
		const dsql_nod* ptr = node->nod_arg[count];

		if (!ptr)
			continue;

		switch (ptr->nod_type)
		{
		case nod_access:
			if (sw_access)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_dup_option));

			sw_access = true;
			if (ptr->nod_flags & NOD_READ_ONLY)
				stuff(statement, isc_tpb_read);
			else
				stuff(statement, isc_tpb_write);
			break;

		case nod_wait:
			if (sw_wait)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_dup_option));

			sw_wait = true;
			if (ptr->nod_flags & NOD_NO_WAIT)
				stuff(statement, isc_tpb_nowait);
			else
				stuff(statement, isc_tpb_wait);
			break;

		case nod_isolation:
			if (sw_isolation)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_dup_option));

			sw_isolation = true;

			if (ptr->nod_flags & NOD_CONCURRENCY)
				stuff(statement, isc_tpb_concurrency);
			else if (ptr->nod_flags & NOD_CONSISTENCY)
				stuff(statement, isc_tpb_consistency);
			else {
				stuff(statement, isc_tpb_read_committed);

				if (ptr->nod_count && ptr->nod_arg[0] && ptr->nod_arg[0]->nod_type == nod_version)
				{
					if (ptr->nod_arg[0]->nod_flags & NOD_VERSION)
						stuff(statement, isc_tpb_rec_version);
					else
						stuff(statement, isc_tpb_no_rec_version);
				}
				else
					stuff(statement, isc_tpb_no_rec_version);
			}
			break;

		case nod_reserve:
			{
				if (sw_reserve)
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_dup_option));

				sw_reserve = true;
				const dsql_nod* reserve = ptr->nod_arg[0];

				if (reserve) {
					const dsql_nod* const* temp = reserve->nod_arg;
					for (const dsql_nod* const* end = temp + reserve->nod_count; temp < end; temp++)
					{
						gen_table_lock(statement, *temp, lock_level);
					}
				}
			}
			break;

		case nod_tra_misc:
			if (misc_flags & ptr->nod_flags)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_dup_option));

			misc_flags |= ptr->nod_flags;
			if (ptr->nod_flags & NOD_NO_AUTO_UNDO)
				stuff(statement, isc_tpb_no_auto_undo);
			else if (ptr->nod_flags & NOD_IGNORE_LIMBO)
				stuff(statement, isc_tpb_ignore_limbo);
			else if (ptr->nod_flags & NOD_RESTART_REQUESTS)
				stuff(statement, isc_tpb_restart_requests);
			break;

		case nod_lock_timeout:
			if (sw_lock_timeout)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_dup_option));

			sw_lock_timeout = true;
			if (ptr->nod_count == 1 && ptr->nod_arg[0]->nod_type == nod_constant)
			{
				const int lck_timeout = (int) ptr->nod_arg[0]->getSlong();
				stuff(statement, isc_tpb_lock_timeout);
				stuff(statement, 2);
				stuff_word(statement, lck_timeout);
			}
			break;

		default:
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_tran_err));
		}
	}
}


/**

 	GEN_statement

    @brief	Generate blr for an arbitrary expression.


    @param statement
    @param node

 **/
void GEN_statement( CompiledStatement* statement, dsql_nod* node)
{
	dsql_nod* temp;
	dsql_nod** ptr;
	const dsql_nod* const* end;
	dsql_str* string;

	switch (node->nod_type)
	{
	case nod_assign:
		stuff(statement, blr_assignment);
		GEN_expr(statement, node->nod_arg[0]);
		GEN_expr(statement, node->nod_arg[1]);
		return;

	case nod_block:
		stuff(statement, blr_block);
		GEN_statement(statement, node->nod_arg[e_blk_action]);
		if (node->nod_count > 1) {
			temp = node->nod_arg[e_blk_errs];
			for (ptr = temp->nod_arg, end = ptr + temp->nod_count; ptr < end; ptr++)
			{
				GEN_statement(statement, *ptr);
			}
		}
		stuff(statement, blr_end);
		return;

	case nod_exec_block:
		DDL_gen_block(statement, node);
		return;

	case nod_class_node:
		reinterpret_cast<StmtNode*>(node->nod_arg[0])->genBlr();
		return;

	case nod_for_select:
		gen_for_select(statement, node);
		return;

	case nod_set_generator:
	case nod_set_generator2:
		stuff(statement, blr_set_generator);
		string = (dsql_str*) node->nod_arg[e_gen_id_name];
		stuff_cstring(statement, string->str_data);
		GEN_expr(statement, node->nod_arg[e_gen_id_value]);
		return;

	case nod_if:
		stuff(statement, blr_if);
		GEN_expr(statement, node->nod_arg[e_if_condition]);
		GEN_statement(statement, node->nod_arg[e_if_true]);
		if (node->nod_arg[e_if_false])
			GEN_statement(statement, node->nod_arg[e_if_false]);
		else
			stuff(statement, blr_end);
		return;

	case nod_list:
		if (!(node->nod_flags & NOD_SIMPLE_LIST))
			stuff(statement, blr_begin);
		for (ptr = node->nod_arg, end = ptr + node->nod_count; ptr < end; ptr++)
		{
			GEN_statement(statement, *ptr);
		}
		if (!(node->nod_flags & NOD_SIMPLE_LIST))
			stuff(statement, blr_end);
		return;

	case nod_erase:
	case nod_erase_current:
	case nod_modify:
	case nod_modify_current:
	case nod_store:
	case nod_exec_procedure:
		gen_statement(statement, node);
		return;

	case nod_on_error:
		stuff(statement, blr_error_handler);
		temp = node->nod_arg[e_err_errs];
		stuff_word(statement, temp->nod_count);
		for (ptr = temp->nod_arg, end = ptr + temp->nod_count; ptr < end; ptr++)
		{
			gen_error_condition(statement, *ptr);
		}
		GEN_statement(statement, node->nod_arg[e_err_action]);
		return;

	case nod_post:
		if ( (temp = node->nod_arg[e_pst_argument]) ) {
			stuff(statement, blr_post_arg);
			GEN_expr(statement, node->nod_arg[e_pst_event]);
			GEN_expr(statement, temp);
		}
		else {
			stuff(statement, blr_post);
			GEN_expr(statement, node->nod_arg[e_pst_event]);
		}
		return;

	case nod_exec_sql:
		stuff(statement, blr_exec_sql);
		GEN_expr(statement, node->nod_arg[e_exec_sql_stmnt]);
		return;

	case nod_exec_into:
		if (node->nod_arg[e_exec_into_block]) {
			stuff(statement, blr_label);
			stuff(statement, (int) (IPTR) node->nod_arg[e_exec_into_label]->nod_arg[e_label_number]);
		}
		stuff(statement, blr_exec_into);
		temp = node->nod_arg[e_exec_into_list];
		stuff_word(statement, temp->nod_count);
		GEN_expr(statement, node->nod_arg[e_exec_into_stmnt]);
		if (node->nod_arg[e_exec_into_block]) {
			stuff(statement, 0); // Non-singleton
			GEN_statement(statement, node->nod_arg[e_exec_into_block]);
		}
		else
			stuff(statement, 1); // Singleton

		for (ptr = temp->nod_arg, end = ptr + temp->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
		return;

	case nod_exec_stmt:
		gen_exec_stmt(statement, node);
		return;

	case nod_return:
		if ( (temp = node->nod_arg[e_rtn_procedure]) )
		{
			if (temp->nod_type == nod_exec_block)
				GEN_return(statement, temp->nod_arg[e_exe_blk_outputs], false);
			else
				GEN_return(statement, temp->nod_arg[e_prc_outputs], false);
		}
		return;

	case nod_exit:
		stuff(statement, blr_leave);
		stuff(statement, 0);
		return;

	case nod_breakleave:
		stuff(statement, blr_leave);
		stuff(statement, (int) (IPTR) node->nod_arg[e_breakleave_label]->nod_arg[e_label_number]);
		return;

	case nod_abort:
		stuff(statement, blr_leave);
		stuff(statement, (int) (IPTR) node->nod_arg[e_abrt_number]);
		return;

	case nod_start_savepoint:
		stuff(statement, blr_start_savepoint);
		return;

	case nod_end_savepoint:
		stuff(statement, blr_end_savepoint);
		return;

	case nod_user_savepoint:
		stuff(statement, blr_user_savepoint);
		stuff(statement, blr_savepoint_set);
		stuff_cstring(statement, ((dsql_str*)node->nod_arg[e_sav_name])->str_data);
		return;

	case nod_release_savepoint:
		stuff(statement, blr_user_savepoint);
		if (node->nod_arg[1]) {
			stuff(statement, blr_savepoint_release_single);
		}
		else {
			stuff(statement, blr_savepoint_release);
		}
		stuff_cstring(statement, ((dsql_str*)node->nod_arg[e_sav_name])->str_data);
		return;

	case nod_undo_savepoint:
		stuff(statement, blr_user_savepoint);
		stuff(statement, blr_savepoint_undo);
		stuff_cstring(statement, ((dsql_str*)node->nod_arg[e_sav_name])->str_data);
		return;

	case nod_exception_stmt:
		stuff(statement, blr_abort);
		string = (dsql_str*) node->nod_arg[e_xcps_name];
		temp = node->nod_arg[e_xcps_msg];
		// if exception name is undefined,
		// it means we have re-initiate semantics here,
		// so blr_raise verb should be generated
		if (!string)
		{
			stuff(statement, blr_raise);
			return;
		}
		// if exception value is defined,
		// it means we have user-defined exception message here,
		// so blr_exception_msg verb should be generated
		if (temp)
		{
			stuff(statement, blr_exception_msg);
		}
		// otherwise go usual way,
		// i.e. generate blr_exception
		else
		{
			stuff(statement, blr_exception);
		}
		if (string->type != dsql_str::TYPE_DELIMITED)
		{
			ULONG id_length = string->str_length;
			for (TEXT* p = string->str_data; *p && id_length; ++p, --id_length)
			{
				*p = UPPER(*p);
			}
		}
		stuff_cstring(statement, string->str_data);
		// if exception value is defined,
		// generate appropriate BLR verbs
		if (temp)
		{
			GEN_expr(statement, temp);
		}
		return;

	case nod_while:
		stuff(statement, blr_label);
		stuff(statement, (int) (IPTR) node->nod_arg[e_while_label]->nod_arg[e_label_number]);
		stuff(statement, blr_loop);
		stuff(statement, blr_begin);
		stuff(statement, blr_if);
		GEN_expr(statement, node->nod_arg[e_while_cond]);
		GEN_statement(statement, node->nod_arg[e_while_action]);
		stuff(statement, blr_leave);
		stuff(statement, (int) (IPTR) node->nod_arg[e_while_label]->nod_arg[e_label_number]);
		stuff(statement, blr_end);
		return;

	case nod_sqlcode:
	case nod_gdscode:
		stuff(statement, blr_abort);
		gen_error_condition(statement, node);
		return;

	case nod_cursor:
		stuff(statement, blr_dcl_cursor);
		stuff_word(statement, (int) (IPTR) node->nod_arg[e_cur_number]);
		GEN_expr(statement, node->nod_arg[e_cur_rse]);
		temp = node->nod_arg[e_cur_rse]->nod_arg[e_rse_items];
		stuff_word(statement, temp->nod_count);
		ptr = temp->nod_arg;
		end = ptr + temp->nod_count;
		while (ptr < end) {
			GEN_expr(statement, *ptr++);
		}
		return;

	case nod_cursor_open:
	case nod_cursor_close:
	case nod_cursor_fetch:
		{
			// op-code
			stuff(statement, blr_cursor_stmt);
			if (node->nod_type == nod_cursor_open)
				stuff(statement, blr_cursor_open);
			else if (node->nod_type == nod_cursor_close)
				stuff(statement, blr_cursor_close);
			else
				stuff(statement, blr_cursor_fetch);
			// cursor reference
			dsql_nod* cursor = node->nod_arg[e_cur_stmt_id];
			stuff_word(statement, (int) (IPTR) cursor->nod_arg[e_cur_number]);
			// preliminary navigation
			const dsql_nod* seek = node->nod_arg[e_cur_stmt_seek];
			if (seek) {
				stuff(statement, blr_seek);
				GEN_expr(statement, seek->nod_arg[0]);
				GEN_expr(statement, seek->nod_arg[1]);
			}
			// assignment
			dsql_nod* list_into = node->nod_arg[e_cur_stmt_into];
			if (list_into)
			{
				dsql_nod* list = cursor->nod_arg[e_cur_rse]->nod_arg[e_rse_items];
				if (list->nod_count != list_into->nod_count)
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-313) <<
							  Arg::Gds(isc_dsql_count_mismatch));
				}
				stuff(statement, blr_begin);
				ptr = list->nod_arg;
				end = ptr + list->nod_count;
				dsql_nod** ptr_to = list_into->nod_arg;
				while (ptr < end) {
					stuff(statement, blr_assignment);
					GEN_expr(statement, *ptr++);
					GEN_expr(statement, *ptr_to++);
				}
				stuff(statement, blr_end);
			}
		}
		return;

	case nod_src_info:
		statement->put_debug_src_info(node->nod_line, node->nod_column);
		GEN_statement(statement, node->nod_arg[e_src_info_stmt]);
		return;

	default:
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_dsql_internal_err) <<
				  // gen.c: node not supported
				  Arg::Gds(isc_node_err));
	}
}


/**

 	gen_aggregate

    @brief	Generate blr for a relation reference.


    @param
    @param

 **/
static void gen_aggregate( CompiledStatement* statement, const dsql_nod* node)
{
	const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_agg_context];
	stuff(statement, blr_aggregate);
	stuff_context(statement, context);
	gen_rse(statement, node->nod_arg[e_agg_rse]);

	// Handle GROUP BY clause

	stuff(statement, blr_group_by);

	dsql_nod* list = node->nod_arg[e_agg_group];
	if (list != NULL) {
		stuff(statement, list->nod_count);
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* end = ptr + list->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
	}
	else
		stuff(statement, 0);

	// Generate value map

	gen_map(statement, context->ctx_map);
}


/**

 gen_cast

    @brief      Generate BLR for a data-type cast operation


    @param statement
    @param node

 **/
static void gen_cast( CompiledStatement* statement, const dsql_nod* node)
{
	stuff(statement, blr_cast);
	const dsql_fld* field = (dsql_fld*) node->nod_arg[e_cast_target];
	DDL_put_field_dtype(statement, field, true);
	GEN_expr(statement, node->nod_arg[e_cast_source]);
}


/**

 gen_coalesce

    @brief      Generate BLR for coalesce function

	Generate the blr values, begin with a cast and then :

	blr_value_if
	blr_missing
	blr for expression 1
		blr_value_if
		blr_missing
		blr for expression n-1
		expression n
	blr for expression n-1

    @param statement
    @param node

 **/
static void gen_coalesce( CompiledStatement* statement, const dsql_nod* node)
{
	// blr_value_if is used for building the coalesce function
	dsql_nod* list = node->nod_arg[0];
	stuff(statement, blr_cast);
	GEN_descriptor(statement, &node->nod_desc, true);
	dsql_nod* const* ptr = list->nod_arg;
	for (const dsql_nod* const* const end = ptr + (list->nod_count - 1); ptr < end; ptr++)
	{
		// IF (expression IS NULL) THEN
		stuff(statement, blr_value_if);
		stuff(statement, blr_missing);
		GEN_expr(statement, *ptr);
	}
	// Return values
	GEN_expr(statement, *ptr);
	list = node->nod_arg[1];
	const dsql_nod* const* const begin = list->nod_arg;
	ptr = list->nod_arg + list->nod_count;
	// if all expressions are NULL return NULL
	for (ptr--; ptr >= begin; ptr--)
	{
		GEN_expr(statement, *ptr);
	}
}


/**

 	gen_constant

    @brief	Generate BLR for a constant.


    @param statement
    @param desc
    @param negate_value

 **/
static void gen_constant( CompiledStatement* statement, const dsc* desc, bool negate_value)
{
	SLONG value;
	SINT64 i64value;

	stuff(statement, blr_literal);

	const UCHAR* p = desc->dsc_address;

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		GEN_descriptor(statement, desc, true);
		value = *(SSHORT *) p;
		if (negate_value)
			value = -value;
		stuff_word(statement, value);
		break;

	case dtype_long:
		GEN_descriptor(statement, desc, true);
		value = *(SLONG *) p;
		if (negate_value)
			value = -value;
		//printf("gen.cpp = %p %d\n", *((void**)p), value);
		stuff_word(statement, value);
		stuff_word(statement, value >> 16);
		break;

	case dtype_sql_time:
	case dtype_sql_date:
		GEN_descriptor(statement, desc, true);
		value = *(SLONG *) p;
		stuff_word(statement, value);
		stuff_word(statement, value >> 16);
		break;

	case dtype_double:
		{
			// this is used for approximate/large numeric literal
			// which is transmitted to the engine as a string.

			GEN_descriptor(statement, desc, true);
			// Length of string literal, cast because it could be > 127 bytes.
			const USHORT l = (USHORT)(UCHAR) desc->dsc_scale;
			if (negate_value) {
				stuff_word(statement, l + 1);
				stuff(statement, '-');
			}
			else {
				stuff_word(statement, l);
			}

			if (l)
				statement->append_raw_string(p, l);
		}
		break;

	case dtype_int64:
		i64value = *(SINT64 *) p;

		if (negate_value)
			i64value = -i64value;
		else if (i64value == MIN_SINT64)
		{
			// UH OH!
			// yylex correctly recognized the digits as the most-negative
			// possible INT64 value, but unfortunately, there was no
			// preceding '-' (a fact which the lexer could not know).
			// The value is too big for a positive INT64 value, and it
			// didn't contain an exponent so it's not a valid DOUBLE
			// PRECISION literal either, so we have to bounce it.

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_arith_except) <<
					  Arg::Gds(isc_numeric_out_of_range));
		}

		// We and the lexer both agree that this is an SINT64 constant,
		// and if the value needed to be negated, it already has been.
		// If the value will fit into a 32-bit signed integer, generate
		// it that way, else as an INT64.

		if ((i64value >= (SINT64) MIN_SLONG) && (i64value <= (SINT64) MAX_SLONG))
		{
			stuff(statement, blr_long);
			stuff(statement, desc->dsc_scale);
			stuff_word(statement, i64value);
			stuff_word(statement, i64value >> 16);
		}
		else {
			stuff(statement, blr_int64);
			stuff(statement, desc->dsc_scale);
			stuff_word(statement, i64value);
			stuff_word(statement, i64value >> 16);
			stuff_word(statement, i64value >> 32);
			stuff_word(statement, i64value >> 48);
		}
		break;

	case dtype_quad:
	case dtype_blob:
	case dtype_array:
	case dtype_timestamp:
		GEN_descriptor(statement, desc, true);
		value = *(SLONG *) p;
		stuff_word(statement, value);
		stuff_word(statement, value >> 16);
		value = *(SLONG *) (p + 4);
		stuff_word(statement, value);
		stuff_word(statement, value >> 16);
		break;

	case dtype_text:
		{
			const USHORT length = desc->dsc_length;

			GEN_descriptor(statement, desc, true);
			if (length)
				statement->append_raw_string(p, length);
		}
		break;

	default:
		// gen_constant: datatype not understood
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-103) <<
				  Arg::Gds(isc_dsql_constant_err));
	}
}


/**

 	gen_constant

    @brief	Generate BLR for a constant.


    @param statement
    @param node
    @param negate_value

 **/
static void gen_constant( CompiledStatement* statement, dsql_nod* node, bool negate_value)
{
	dsc desc = node->nod_desc;

	if (desc.dsc_dtype == dtype_text)
		desc.dsc_length = ((dsql_str*) node->nod_arg[0])->str_length;

	gen_constant(statement, &desc, negate_value);
}


/**

 	GEN_descriptor

    @brief	Generate a blr descriptor from an internal descriptor.


    @param statement
    @param desc
    @param texttype

 **/
void GEN_descriptor( CompiledStatement* statement, const dsc* desc, bool texttype)
{
	switch (desc->dsc_dtype)
	{
	case dtype_text:
		if (texttype || desc->dsc_ttype() == ttype_binary || desc->dsc_ttype() == ttype_none) {
			stuff(statement, blr_text2);
			stuff_word(statement, desc->dsc_ttype());
		}
		else {
			stuff(statement, blr_text2);	// automatic transliteration
			stuff_word(statement, ttype_dynamic);
		}

		stuff_word(statement, desc->dsc_length);
		break;

	case dtype_varying:
		if (texttype || desc->dsc_ttype() == ttype_binary || desc->dsc_ttype() == ttype_none) {
			stuff(statement, blr_varying2);
			stuff_word(statement, desc->dsc_ttype());
		}
		else {
			stuff(statement, blr_varying2);	// automatic transliteration
			stuff_word(statement, ttype_dynamic);
		}
		stuff_word(statement, desc->dsc_length - sizeof(USHORT));
		break;

	case dtype_short:
		stuff(statement, blr_short);
		stuff(statement, desc->dsc_scale);
		break;

	case dtype_long:
		stuff(statement, blr_long);
		stuff(statement, desc->dsc_scale);
		break;

	case dtype_quad:
		stuff(statement, blr_quad);
		stuff(statement, desc->dsc_scale);
		break;

	case dtype_int64:
		stuff(statement, blr_int64);
		stuff(statement, desc->dsc_scale);
		break;

	case dtype_real:
		stuff(statement, blr_float);
		break;

	case dtype_double:
		stuff(statement, blr_double);
		break;

	case dtype_sql_date:
		stuff(statement, blr_sql_date);
		break;

	case dtype_sql_time:
		stuff(statement, blr_sql_time);
		break;

	case dtype_timestamp:
		stuff(statement, blr_timestamp);
		break;

	case dtype_array:
		stuff(statement, blr_quad);
		stuff(statement, 0);
		break;

	case dtype_blob:
		stuff(statement, blr_blob2);
		stuff_word(statement, desc->dsc_sub_type);
		stuff_word(statement, desc->getTextType());
		break;

	default:
		// don't understand dtype
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_datatype_err));
	}
}


/**

 	gen_error_condition

    @brief	Generate blr for an error condtion


    @param statement
    @param node

 **/
static void gen_error_condition( CompiledStatement* statement, const dsql_nod* node)
{
	const dsql_str* string;

	switch (node->nod_type)
	{
	case nod_sqlcode:
		stuff(statement, blr_sql_code);
		stuff_word(statement, (USHORT)(IPTR) node->nod_arg[0]);
		return;

	case nod_gdscode:
		stuff(statement, blr_gds_code);
		string = (dsql_str*) node->nod_arg[0];
		stuff_cstring(statement, string->str_data);
		return;

	case nod_exception:
		stuff(statement, blr_exception);
		string = (dsql_str*) node->nod_arg[0];
		stuff_cstring(statement, string->str_data);
		return;

	case nod_default:
		stuff(statement, blr_default_code);
		return;

	default:
		fb_assert(false);
		return;
	}
}


/**

 	gen_exec_stmt

    @brief	Generate blr for the EXECUTE STATEMENT clause


    @param statement
    @param node

 **/
static void gen_exec_stmt(CompiledStatement* statement, const dsql_nod* node)
{
	if (node->nod_arg[e_exec_stmt_proc_block])
	{
		stuff(statement, blr_label);
		stuff(statement, (int)(IPTR) node->nod_arg[e_exec_stmt_label]->nod_arg[e_label_number]);
	}

	stuff(statement, blr_exec_stmt);

	// counts of input and output parameters
	const dsql_nod* temp = node->nod_arg[e_exec_stmt_inputs];
	if (temp)
	{
		stuff(statement, blr_exec_stmt_inputs);
		stuff_word(statement, temp->nod_count);
	}

	temp = node->nod_arg[e_exec_stmt_outputs];
	if (temp)
	{
		stuff(statement, blr_exec_stmt_outputs);
		stuff_word(statement, temp->nod_count);
	}

	// query expression
	stuff(statement, blr_exec_stmt_sql);
	GEN_expr(statement, node->nod_arg[e_exec_stmt_sql]);

	// proc block body
	dsql_nod* temp2 = node->nod_arg[e_exec_stmt_proc_block];
	if (temp2)
	{
		stuff(statement, blr_exec_stmt_proc_block);
		GEN_statement(statement, temp2);
	}

	// external data source, user, password and role
	gen_optional_expr(statement, blr_exec_stmt_data_src, node->nod_arg[e_exec_stmt_data_src]);
	gen_optional_expr(statement, blr_exec_stmt_user, node->nod_arg[e_exec_stmt_user]);
	gen_optional_expr(statement, blr_exec_stmt_pwd, node->nod_arg[e_exec_stmt_pwd]);
	gen_optional_expr(statement, blr_exec_stmt_role, node->nod_arg[e_exec_stmt_role]);

	// statement's transaction behavior
	temp = node->nod_arg[e_exec_stmt_tran];
	if (temp)
	{
		stuff(statement, blr_exec_stmt_tran_clone); // transaction parameters equal to current transaction
		stuff(statement, (UCHAR)(IPTR) temp->nod_flags);
	}

	// inherit caller's privileges ?
	if (node->nod_arg[e_exec_stmt_privs]) {
		stuff(statement, blr_exec_stmt_privs);
	}

	// inputs
	temp = node->nod_arg[e_exec_stmt_inputs];
	if (temp)
	{
		const dsql_nod* const* ptr = temp->nod_arg;
		const bool haveNames = ((*ptr)->nod_arg[e_named_param_name] != 0);
		if (haveNames)
			stuff(statement, blr_exec_stmt_in_params2);
		else
			stuff(statement, blr_exec_stmt_in_params);

		for (const dsql_nod* const* end = ptr + temp->nod_count; ptr < end; ptr++)
		{
			if (haveNames)
			{
				const dsql_str* name = (dsql_str*) (*ptr)->nod_arg[e_named_param_name];
				stuff_cstring(statement, name->str_data);
			}
			GEN_expr(statement, (*ptr)->nod_arg[e_named_param_expr]);
		}
	}

	// outputs
	temp = node->nod_arg[e_exec_stmt_outputs];
	if (temp)
	{
		stuff(statement, blr_exec_stmt_out_params);
		for (size_t i = 0; i < temp->nod_count; ++i) {
			GEN_expr(statement, temp->nod_arg[i]);
		}
	}
	stuff(statement, blr_end);
}


/**

 	gen_field

    @brief	Generate blr for a field - field id's
 	are preferred but not for trigger or view blr.


    @param statement
    @param context
    @param field
    @param indices

 **/
static void gen_field( CompiledStatement* statement, const dsql_ctx* context,
	const dsql_fld* field, dsql_nod* indices)
{
	// For older clients - generate an error should they try and
	// access data types which did not exist in the older dialect
	if (statement->req_client_dialect <= SQL_DIALECT_V5)
	{
		switch (field->fld_dtype)
		{
		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_int64:
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_datatype_err) <<
					  Arg::Gds(isc_sql_dialect_datatype_unsupport) <<
					  			Arg::Num(statement->req_client_dialect) <<
					  			Arg::Str(DSC_dtype_tostring(static_cast<UCHAR>(field->fld_dtype))));
			break;
		default:
			// No special action for other data types
			break;
		}
	}

	if (indices)
		stuff(statement, blr_index);

	if (DDL_ids(statement)) {
		stuff(statement, blr_fid);
		stuff_context(statement, context);
		stuff_word(statement, field->fld_id);
	}
	else {
		stuff(statement, blr_field);
		stuff_context(statement, context);
		stuff_meta_string(statement, field->fld_name.c_str());
	}

	if (indices) {
		stuff(statement, indices->nod_count);
		dsql_nod** ptr = indices->nod_arg;
		for (const dsql_nod* const* end = ptr + indices->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
	}
}


/**

 	gen_for_select

    @brief	Generate BLR for a SELECT statement.


    @param statement
    @param for_select

 **/
static void gen_for_select( CompiledStatement* statement, const dsql_nod* for_select)
{
	dsql_nod* rse = for_select->nod_arg[e_flp_select];

	// CVC: Only put a label if this is not singular; otherwise,
	// what loop is the user trying to abandon?
	if (for_select->nod_arg[e_flp_action]) {
		stuff(statement, blr_label);
		stuff(statement, (int) (IPTR) for_select->nod_arg[e_flp_label]->nod_arg[e_label_number]);
	}

	// Generate FOR loop

	stuff(statement, blr_for);

	if (!for_select->nod_arg[e_flp_action])
	{
		stuff(statement, blr_singular);
	}
	gen_rse(statement, rse);
	stuff(statement, blr_begin);

	// Build body of FOR loop

	// Handle write locks
	/* CVC: Unused code!
	dsql_nod* streams = rse->nod_arg[e_rse_streams];
	dsql_ctx* context = NULL;

	if (!rse->nod_arg[e_rse_reduced] && streams->nod_count == 1) {
		dsql_nod* item = streams->nod_arg[0];
		if (item && (item->nod_type == nod_relation))
			context = (dsql_ctx*) item->nod_arg[e_rel_context];
	}
	*/

	dsql_nod* list = rse->nod_arg[e_rse_items];
	dsql_nod* list_to = for_select->nod_arg[e_flp_into];

	if (list_to)
	{
		if (list->nod_count != list_to->nod_count)
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-313) <<
					  Arg::Gds(isc_dsql_count_mismatch));
		dsql_nod** ptr = list->nod_arg;
		dsql_nod** ptr_to = list_to->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++, ptr_to++)
		{
			stuff(statement, blr_assignment);
			GEN_expr(statement, *ptr);
			GEN_expr(statement, *ptr_to);
		}
	}

	if (for_select->nod_arg[e_flp_action])
		GEN_statement(statement, for_select->nod_arg[e_flp_action]);
	stuff(statement, blr_end);
}


/**

 gen_gen_id

    @brief      Generate BLR for gen_id


    @param statement
    @param node

 **/
static void gen_gen_id( CompiledStatement* statement, const dsql_nod* node)
{
	stuff(statement, blr_gen_id);
	const dsql_str* string = (dsql_str*) node->nod_arg[e_gen_id_name];
	stuff_cstring(statement, string->str_data);
	GEN_expr(statement, node->nod_arg[e_gen_id_value]);
}


/**

 	gen_join_rse

    @brief	Generate a record selection expression
 	with an explicit join type.


    @param statement
    @param rse

 **/
static void gen_join_rse( CompiledStatement* statement, const dsql_nod* rse)
{
	stuff(statement, blr_rs_stream);
	stuff(statement, 2);

	GEN_expr(statement, rse->nod_arg[e_join_left_rel]);
	GEN_expr(statement, rse->nod_arg[e_join_rght_rel]);

	const dsql_nod* node = rse->nod_arg[e_join_type];
	if (node->nod_type != nod_join_inner)
	{
		stuff(statement, blr_join_type);
		if (node->nod_type == nod_join_left)
			stuff(statement, blr_left);
		else if (node->nod_type == nod_join_right)
			stuff(statement, blr_right);
		else
			stuff(statement, blr_full);
	}

	if (rse->nod_arg[e_join_boolean])
	{
		stuff(statement, blr_boolean);
		GEN_expr(statement, rse->nod_arg[e_join_boolean]);
	}

	stuff(statement, blr_end);
}


/**

 	gen_map

    @brief	Generate a value map for a record selection expression.


    @param statement
    @param map

 **/
static void gen_map( CompiledStatement* statement, dsql_map* map)
{
	USHORT count = 0;
	dsql_map* temp;
	for (temp = map; temp; temp = temp->map_next)
		temp->map_position = count++;

	stuff(statement, blr_map);
	stuff_word(statement, count);

	for (temp = map; temp; temp = temp->map_next) {
		stuff_word(statement, temp->map_position);
		GEN_expr(statement, temp->map_node);
	}
}

static void gen_optional_expr(CompiledStatement* statement, const UCHAR code, dsql_nod* node)
{
	if (node)
	{
		stuff(statement, code);
		GEN_expr(statement, node);
	}
}

/**

 	gen_parameter

    @brief	Generate a parameter reference.


    @param statement
    @param parameter

 **/
static void gen_parameter( CompiledStatement* statement, const dsql_par* parameter)
{
	const dsql_msg* message = parameter->par_message;

	const dsql_par* null = parameter->par_null;
	if (null != NULL) {
		stuff(statement, blr_parameter2);
		stuff(statement, message->msg_number);
		stuff_word(statement, parameter->par_parameter);
		stuff_word(statement, null->par_parameter);
		return;
	}

	stuff(statement, blr_parameter);
	stuff(statement, message->msg_number);
	stuff_word(statement, parameter->par_parameter);
}



/**

 	gen_plan

    @brief	Generate blr for an access plan expression.


    @param statement
    @param plan_expression

 **/
static void gen_plan( CompiledStatement* statement, const dsql_nod* plan_expression)
{
	// stuff the join type

	const dsql_nod* list = plan_expression->nod_arg[1];
	if (list->nod_count > 1) {
		if (plan_expression->nod_arg[0])
			stuff(statement, blr_merge);
		else
			stuff(statement, blr_join);
		stuff(statement, list->nod_count);
	}

	// stuff one or more plan items

	const dsql_nod* const* ptr = list->nod_arg;
	for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		const dsql_nod* node = *ptr;
		if (node->nod_type == nod_plan_expr) {
			gen_plan(statement, node);
			continue;
		}

		// if we're here, it must be a nod_plan_item

		stuff(statement, blr_retrieve);

		// stuff the relation--the relation id itself is redundant except
		// when there is a need to differentiate the base tables of views

		const dsql_nod* arg = node->nod_arg[0];
		gen_relation(statement, (dsql_ctx*) arg->nod_arg[e_rel_context]);

		// now stuff the access method for this stream
		const dsql_str* index_string;

		arg = node->nod_arg[1];
		switch (arg->nod_type)
		{
		case nod_natural:
			stuff(statement, blr_sequential);
			break;

		case nod_index_order:
			stuff(statement, blr_navigational);
			index_string = (dsql_str*) arg->nod_arg[0];
			stuff_cstring(statement, index_string->str_data);
			if (!arg->nod_arg[1])
				break;
			// dimitr: FALL INTO, if the plan item is ORDER ... INDEX (...)

		case nod_index:
			{
				stuff(statement, blr_indices);
				arg = (arg->nod_type == nod_index) ? arg->nod_arg[0] : arg->nod_arg[1];
				stuff(statement, arg->nod_count);
				const dsql_nod* const* ptr2 = arg->nod_arg;
				for (const dsql_nod* const* const end2 = ptr2 + arg->nod_count; ptr2 < end2; ptr2++)
				{
					index_string = (dsql_str*) * ptr2;
					stuff_cstring(statement, index_string->str_data);
				}
				break;
			}

		default:
			fb_assert(false);
			break;
		}

	}
}



/**

 	gen_relation

    @brief	Generate blr for a relation reference.


    @param statement
    @param context

 **/
static void gen_relation( CompiledStatement* statement, dsql_ctx* context)
{
	const dsql_rel* relation = context->ctx_relation;
	const dsql_prc* procedure = context->ctx_procedure;

	// if this is a trigger or procedure, don't want relation id used
	if (relation)
	{
		if (DDL_ids(statement)) {
			if (context->ctx_alias)
				stuff(statement, blr_rid2);
			else
				stuff(statement, blr_rid);
			stuff_word(statement, relation->rel_id);
		}
		else {
			if (context->ctx_alias)
				stuff(statement, blr_relation2);
			else
				stuff(statement, blr_relation);
			stuff_meta_string(statement, relation->rel_name.c_str());
		}

		if (context->ctx_alias)
			stuff_meta_string(statement, context->ctx_alias);

		stuff_context(statement, context);
	}
	else if (procedure)
	{
		if (DDL_ids(statement)) {
			stuff(statement, blr_pid);
			stuff_word(statement, procedure->prc_id);
		}
		else {
			stuff(statement, blr_procedure);
			stuff_meta_string(statement, procedure->prc_name.c_str());
		}
		stuff_context(statement, context);

		dsql_nod* inputs = context->ctx_proc_inputs;
		if (inputs) {
			stuff_word(statement, inputs->nod_count);

			dsql_nod* const* ptr = inputs->nod_arg;
			for (const dsql_nod* const* const end = ptr + inputs->nod_count; ptr < end; ptr++)
			{
 				GEN_expr(statement, *ptr);
			}
 		}
		else
			stuff_word(statement, 0);
	}
}


/**

 	gen_return

    @brief	Generate blr for a procedure return.


    @param statement
    @param procedure
    @param eos_flag

 **/
void GEN_return( CompiledStatement* statement, const dsql_nod* parameters, bool eos_flag)
{
	if (!eos_flag)
		stuff(statement, blr_begin);

	stuff(statement, blr_send);
	stuff(statement, 1);
	stuff(statement, blr_begin);

	USHORT outputs = 0;
	if (parameters)
	{
		const dsql_nod* const* ptr = parameters->nod_arg;
		for (const dsql_nod* const* const end = ptr + parameters->nod_count; ptr < end; ptr++)
		{
			outputs++;
			const dsql_nod* parameter = *ptr;
			const dsql_var* variable = (dsql_var*) parameter->nod_arg[e_var_variable];
			stuff(statement, blr_assignment);
			stuff(statement, blr_variable);
			stuff_word(statement, variable->var_variable_number);
			stuff(statement, blr_parameter2);
			stuff(statement, variable->var_msg_number);
			stuff_word(statement, variable->var_msg_item);
			stuff_word(statement, variable->var_msg_item + 1);
		}
	}
	stuff(statement, blr_assignment);
	stuff(statement, blr_literal);
	stuff(statement, blr_short);
	stuff(statement, 0);
	if (eos_flag)
		stuff_word(statement, 0);
	else
		stuff_word(statement, 1);
	stuff(statement, blr_parameter);
	stuff(statement, 1);
	stuff_word(statement, 2 * outputs);
	stuff(statement, blr_end);
	if (!eos_flag) {
		stuff(statement, blr_stall);
		stuff(statement, blr_end);
	}
}


/**

 	gen_rse

    @brief	Generate a record selection expression.


    @param statement
    @param rse

 **/
static void gen_rse( CompiledStatement* statement, const dsql_nod* rse)
{
	if (rse->nod_flags & NOD_SELECT_EXPR_SINGLETON)
	{
		stuff(statement, blr_singular);
	}

	stuff(statement, blr_rse);

	dsql_nod* list = rse->nod_arg[e_rse_streams];

	// Handle source streams

	if (list->nod_type == nod_union) {
		stuff(statement, 1);
		gen_union(statement, rse);
	}
	else if (list->nod_type == nod_list)
	{
		stuff(statement, list->nod_count);
		dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			dsql_nod* node = *ptr;
			switch (node->nod_type)
			{
			case nod_relation:
			case nod_aggregate:
			case nod_join:
				GEN_expr(statement, node);
				break;
			case nod_derived_table:
				GEN_expr(statement, node->nod_arg[e_derived_table_rse]);
				break;
			}
		}
	}
	else {
		stuff(statement, 1);
		GEN_expr(statement, list);
	}

	if (rse->nod_arg[e_rse_lock])
		stuff(statement, blr_writelock);

	dsql_nod* node;

	if ((node = rse->nod_arg[e_rse_first]) != NULL) {
		stuff(statement, blr_first);
		GEN_expr(statement, node);
	}

	if ((node = rse->nod_arg[e_rse_skip]) != NULL) {
		stuff(statement, blr_skip);
		GEN_expr (statement, node);
	}

	if ((node = rse->nod_arg[e_rse_boolean]) != NULL) {
		stuff(statement, blr_boolean);
		GEN_expr(statement, node);
	}

	if ((list = rse->nod_arg[e_rse_sort]) != NULL)
		gen_sort(statement, list);

	if ((list = rse->nod_arg[e_rse_reduced]) != NULL) {
		stuff(statement, blr_project);
		stuff(statement, list->nod_count);
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
	}

	// if the user specified an access plan to use, add it here

	if ((node = rse->nod_arg[e_rse_plan]) != NULL) {
		stuff(statement, blr_plan);
		gen_plan(statement, node);
	}

#ifdef SCROLLABLE_CURSORS
	// generate a statement to be executed if the user scrolls
	// in a direction other than forward; a message is sent outside
	// the normal send/receive protocol to specify the direction
	// and offset to scroll; note that we do this only on a SELECT
	// type statement and only when talking to a 4.1 engine or greater

	if (statement->req_type == REQ_SELECT && statement->req_dbb->dbb_base_level >= 5)
	{
		stuff(statement, blr_receive);
		stuff(statement, statement->req_async->msg_number);
		stuff(statement, blr_seek);
		const dsql_par* parameter = statement->req_async->msg_parameters;
		gen_parameter(statement, parameter->par_next);
		gen_parameter(statement, parameter);
	}
#endif

	stuff(statement, blr_end);
}


/**

 gen_searched_case

    @brief      Generate BLR for CASE function (searched)


    @param statement
    @param node

 **/
static void gen_searched_case( CompiledStatement* statement, const dsql_nod* node)
{
	// blr_value_if is used for building the case expression

	stuff(statement, blr_cast);
	GEN_descriptor(statement, &node->nod_desc, true);
	const SSHORT count = node->nod_arg[e_searched_case_search_conditions]->nod_count;
	dsql_nod* boolean_list = node->nod_arg[e_searched_case_search_conditions];
	dsql_nod* results_list = node->nod_arg[e_searched_case_results];
	dsql_nod* const* bptr = boolean_list->nod_arg;
	dsql_nod* const* rptr = results_list->nod_arg;
	for (const dsql_nod* const* const end = bptr + count; bptr < end; bptr++, rptr++)
	{
		stuff(statement, blr_value_if);
		GEN_expr(statement, *bptr);
		GEN_expr(statement, *rptr);
	}
	// else_result
	GEN_expr(statement, node->nod_arg[e_searched_case_results]->nod_arg[count]);
}


/**

 	gen_select

    @brief	Generate BLR for a SELECT statement.


    @param statement
    @param rse

 **/
static void gen_select( CompiledStatement* statement, dsql_nod* rse)
{
	const dsql_rel* relation;
	dsql_ctx* context;

	fb_assert(rse->nod_type == nod_rse);

	// Set up parameter for things in the select list
	const dsql_nod* list = rse->nod_arg[e_rse_items];
	dsql_nod* const* ptr = list->nod_arg;
	for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		dsql_par* parameter = MAKE_parameter(statement->req_receive, true, true, 0, *ptr);
		parameter->par_node = *ptr;
		MAKE_desc(statement, &parameter->par_desc, *ptr, NULL);
	}

	// Set up parameter to handle EOF

	dsql_par* parameter_eof = MAKE_parameter(statement->req_receive, false, false, 0, NULL);
	statement->req_eof = parameter_eof;
	parameter_eof->par_desc.dsc_dtype = dtype_short;
	parameter_eof->par_desc.dsc_scale = 0;
	parameter_eof->par_desc.dsc_length = sizeof(SSHORT);

	// Save DBKEYs for possible update later

	list = rse->nod_arg[e_rse_streams];

	if (!rse->nod_arg[e_rse_reduced])
	{
		dsql_nod* const* ptr2 = list->nod_arg;
		for (const dsql_nod* const* const end2 = ptr2 + list->nod_count; ptr2 < end2; ptr2++)
		{
			dsql_nod* item = *ptr2;
			if (item && item->nod_type == nod_relation)
			{
				context = (dsql_ctx*) item->nod_arg[e_rel_context];
				if (relation = context->ctx_relation)
				{
					// Set up dbkey
					dsql_par* parameter = MAKE_parameter(statement->req_receive, false, false, 0, NULL);
					parameter->par_dbkey_ctx = context;
					parameter->par_desc.dsc_dtype = dtype_text;
					parameter->par_desc.dsc_ttype() = ttype_binary;
					parameter->par_desc.dsc_length = relation->rel_dbkey_length;

					// Set up record version - for post v33 databases

					parameter = MAKE_parameter(statement->req_receive, false, false, 0, NULL);
					parameter->par_rec_version_ctx = context;
					parameter->par_desc.dsc_dtype = dtype_text;
					parameter->par_desc.dsc_ttype() = ttype_binary;
					parameter->par_desc.dsc_length = relation->rel_dbkey_length / 2;
				}
			}
		}
	}

#ifdef SCROLLABLE_CURSORS
	// define the parameters for the scrolling message--offset and direction,
	// in that order to make it easier to generate the statement

	if (statement->req_type == REQ_SELECT && statement->req_dbb->dbb_base_level >= 5)
	{
		dsql_par* parameter = MAKE_parameter(statement->req_async, false, false, 0, NULL);
		parameter->par_desc.dsc_dtype = dtype_short;
		parameter->par_desc.dsc_length = sizeof(USHORT);
		parameter->par_desc.dsc_scale = 0;
		parameter->par_desc.dsc_flags = 0;
		parameter->par_desc.dsc_sub_type = 0;

		parameter = MAKE_parameter(statement->req_async, false, false, 0, NULL);
		parameter->par_desc.dsc_dtype = dtype_long;
		parameter->par_desc.dsc_length = sizeof(ULONG);
		parameter->par_desc.dsc_scale = 0;
		parameter->par_desc.dsc_flags = 0;
		parameter->par_desc.dsc_sub_type = 0;
	}
#endif

	// Generate definitions for the messages

	GEN_port(statement, statement->req_receive);
	dsql_msg* message = statement->req_send;
	if (message->msg_parameter)
		GEN_port(statement, message);
	else
		statement->req_send = NULL;
#ifdef SCROLLABLE_CURSORS
	if (statement->req_type == REQ_SELECT && statement->req_dbb->dbb_base_level >= 5)
		GEN_port(statement, statement->req_async);
#endif

	// If there is a send message, build a RECEIVE

	if ((message = statement->req_send) != NULL) {
		stuff(statement, blr_receive);
		stuff(statement, message->msg_number);
	}

	// Generate FOR loop

	message = statement->req_receive;

	stuff(statement, blr_for);
	stuff(statement, blr_stall);
	gen_rse(statement, rse);

	stuff(statement, blr_send);
	stuff(statement, message->msg_number);
	stuff(statement, blr_begin);

	// Build body of FOR loop

	SSHORT constant;
	dsc constant_desc;
	constant_desc.dsc_dtype = dtype_short;
	constant_desc.dsc_scale = 0;
	constant_desc.dsc_sub_type = 0;
	constant_desc.dsc_flags = 0;
	constant_desc.dsc_length = sizeof(SSHORT);
	constant_desc.dsc_address = (UCHAR*) & constant;

	// Add invalid usage here

	stuff(statement, blr_assignment);
	constant = 1;
	gen_constant(statement, &constant_desc, USE_VALUE);
	gen_parameter(statement, statement->req_eof);

	for (dsql_par* parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		if (parameter->par_node) {
			stuff(statement, blr_assignment);
			GEN_expr(statement, parameter->par_node);
			gen_parameter(statement, parameter);
		}
		if (context = parameter->par_dbkey_ctx) {
			stuff(statement, blr_assignment);
			stuff(statement, blr_dbkey);
			stuff_context(statement, context);
			gen_parameter(statement, parameter);
		}
		if (context = parameter->par_rec_version_ctx) {
			stuff(statement, blr_assignment);
			stuff(statement, blr_record_version);
			stuff_context(statement, context);
			gen_parameter(statement, parameter);
		}
	}

	stuff(statement, blr_end);
	stuff(statement, blr_send);
	stuff(statement, message->msg_number);
	stuff(statement, blr_assignment);
	constant = 0;
	gen_constant(statement, &constant_desc, USE_VALUE);
	gen_parameter(statement, statement->req_eof);
}


/**

 gen_simple_case

    @brief      Generate BLR for CASE function (simple)


    @param statement
    @param node

 **/
static void gen_simple_case( CompiledStatement* statement, const dsql_nod* node)
{
	// blr_value_if is used for building the case expression

	stuff(statement, blr_cast);
	GEN_descriptor(statement, &node->nod_desc, true);
	const SSHORT count = node->nod_arg[e_simple_case_when_operands]->nod_count;
	dsql_nod* when_list = node->nod_arg[e_simple_case_when_operands];
	dsql_nod* results_list = node->nod_arg[e_simple_case_results];

	dsql_nod* const* wptr = when_list->nod_arg;
	dsql_nod* const* rptr = results_list->nod_arg;
	for (const dsql_nod* const* const end = wptr + count; wptr < end; wptr++, rptr++)
	{
		stuff(statement, blr_value_if);
		stuff(statement, blr_eql);

		if (wptr == when_list->nod_arg || !node->nod_arg[e_simple_case_case_operand2])
			GEN_expr(statement, node->nod_arg[e_simple_case_case_operand]);
		else
			GEN_expr(statement, node->nod_arg[e_simple_case_case_operand2]);

		GEN_expr(statement, *wptr);
		GEN_expr(statement, *rptr);
	}
	// else_result
	GEN_expr(statement, node->nod_arg[e_simple_case_results]->nod_arg[count]);
}


/**

 	gen_sort

    @brief	Generate a sort clause.


    @param statement
    @param list

 **/
static void gen_sort( CompiledStatement* statement, dsql_nod* list)
{
	stuff(statement, blr_sort);
	stuff(statement, list->nod_count);

	dsql_nod* const* ptr = list->nod_arg;
	for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		dsql_nod* nulls_placement = (*ptr)->nod_arg[e_order_nulls];
		if (nulls_placement)
		{
			switch (nulls_placement->getSlong())
			{
				case NOD_NULLS_FIRST:
					stuff(statement, blr_nullsfirst);
					break;
				case NOD_NULLS_LAST:
					stuff(statement, blr_nullslast);
					break;
			}
		}
		if ((*ptr)->nod_arg[e_order_flag])
			stuff(statement, blr_descending);
		else
			stuff(statement, blr_ascending);
		GEN_expr(statement, (*ptr)->nod_arg[e_order_field]);
	}
}


/**

 	gen_statement

    @brief	Generate BLR for DML statements.


    @param statement
    @param node

 **/
static void gen_statement(CompiledStatement* statement, const dsql_nod* node)
{
	dsql_nod* rse = NULL;
	const dsql_msg* message = NULL;
	bool send_before_for = !(statement->req_flags & REQ_dsql_upd_or_ins);

	switch (node->nod_type)
	{
	case nod_store:
		rse = node->nod_arg[e_sto_rse];
		break;
	case nod_modify:
		rse = node->nod_arg[e_mod_rse];
		break;
	case nod_erase:
		rse = node->nod_arg[e_era_rse];
		break;
	default:
		send_before_for = false;
		break;
	}

	if (statement->req_type == REQ_EXEC_PROCEDURE && send_before_for)
	{
		if ((message = statement->req_receive))
		{
			stuff(statement, blr_send);
			stuff(statement, message->msg_number);
		}
	}

	if (rse) {
		stuff(statement, blr_for);
		GEN_expr(statement, rse);
	}

	if (statement->req_type == REQ_EXEC_PROCEDURE)
	{
		if ((message = statement->req_receive))
		{
			stuff(statement, blr_begin);

			if (!send_before_for)
			{
				stuff(statement, blr_send);
				stuff(statement, message->msg_number);
			}
		}
	}

	dsql_nod* temp;
	const dsql_ctx* context;
	const dsql_str* name;

	switch (node->nod_type)
	{
	case nod_store:
		stuff(statement, node->nod_arg[e_sto_return] ? blr_store2 : blr_store);
		GEN_expr(statement, node->nod_arg[e_sto_relation]);
		GEN_statement(statement, node->nod_arg[e_sto_statement]);
		if (node->nod_arg[e_sto_return]) {
			GEN_statement(statement, node->nod_arg[e_sto_return]);
		}
		break;

	case nod_modify:
		stuff(statement, node->nod_arg[e_mod_return] ? blr_modify2 : blr_modify);
		temp = node->nod_arg[e_mod_source];
		context = (dsql_ctx*) temp->nod_arg[e_rel_context];
		stuff_context(statement, context);
		temp = node->nod_arg[e_mod_update];
		context = (dsql_ctx*) temp->nod_arg[e_rel_context];
		stuff_context(statement, context);
		GEN_statement(statement, node->nod_arg[e_mod_statement]);
		if (node->nod_arg[e_mod_return]) {
			GEN_statement(statement, node->nod_arg[e_mod_return]);
		}
		break;

	case nod_modify_current:
		stuff(statement, node->nod_arg[e_mdc_return] ? blr_modify2 : blr_modify);
		context = (dsql_ctx*) node->nod_arg[e_mdc_context];
		stuff_context(statement, context);
		temp = node->nod_arg[e_mdc_update];
		context = (dsql_ctx*) temp->nod_arg[e_rel_context];
		stuff_context(statement, context);
		GEN_statement(statement, node->nod_arg[e_mdc_statement]);
		if (node->nod_arg[e_mdc_return]) {
			GEN_statement(statement, node->nod_arg[e_mdc_return]);
		}
		break;

	case nod_erase:
		temp = node->nod_arg[e_era_relation];
		context = (dsql_ctx*) temp->nod_arg[e_rel_context];
		if (node->nod_arg[e_era_return]) {
			stuff(statement, blr_begin);
			GEN_statement(statement, node->nod_arg[e_era_return]);
			stuff(statement, blr_erase);
			stuff_context(statement, context);
			stuff(statement, blr_end);
		}
		else {
			stuff(statement, blr_erase);
			stuff_context(statement, context);
		}
		break;

	case nod_erase_current:
		context = (dsql_ctx*) node->nod_arg[e_erc_context];
		if (node->nod_arg[e_erc_return]) {
			stuff(statement, blr_begin);
			GEN_statement(statement, node->nod_arg[e_erc_return]);
			stuff(statement, blr_erase);
			stuff_context(statement, context);
			stuff(statement, blr_end);
		}
		else {
			stuff(statement, blr_erase);
			stuff_context(statement, context);
		}
		break;

	case nod_exec_procedure:
		stuff(statement, blr_exec_proc);
		name = (dsql_str*) node->nod_arg[e_exe_procedure];
		stuff_meta_string(statement, name->str_data);

		// Input parameters
		if ( (temp = node->nod_arg[e_exe_inputs]) ) {
			stuff_word(statement, temp->nod_count);
			dsql_nod** ptr = temp->nod_arg;
			const dsql_nod* const* end = ptr + temp->nod_count;
			while (ptr < end)
			{
				GEN_expr(statement, *ptr++);
			}
		}
		else {
			stuff_word(statement, 0);
		}
		// Output parameters
		if ( ( temp = node->nod_arg[e_exe_outputs]) ) {
			stuff_word(statement, temp->nod_count);
			dsql_nod** ptr = temp->nod_arg;
			const dsql_nod* const* end = ptr + temp->nod_count;
			while (ptr < end)
			{
				GEN_expr(statement, *ptr++);
			}
		}
		else {
			stuff_word(statement, 0);
		}
		break;

	default:
		fb_assert(false);
	}

	if (message) {
		stuff(statement, blr_end);
	}
}


/**

 	gen_sys_function

    @brief	Generate a system defined function.


    @param statement
    @param node

 **/
static void gen_sys_function(CompiledStatement* statement, const dsql_nod* node)
{
	stuff(statement, blr_sys_function);
	stuff_cstring(statement, ((dsql_str*) node->nod_arg[e_sysfunc_name])->str_data);

	const dsql_nod* list;
	if ((node->nod_count == e_sysfunc_args + 1) && (list = node->nod_arg[e_sysfunc_args]))
	{
		stuff(statement, list->nod_count);
		dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
	}
	else
		stuff(statement, 0);
}


/**

 	gen_table_lock

    @brief	Generate tpb for table lock.
 	If lock level is specified, it overrrides the transaction lock level.


    @param statement
    @param tbl_lock
    @param lock_level

 **/
static void gen_table_lock( CompiledStatement* statement, const dsql_nod* tbl_lock, USHORT lock_level)
{
	if (!tbl_lock || tbl_lock->nod_type != nod_table_lock)
		return;

	const dsql_nod* tbl_names = tbl_lock->nod_arg[e_lock_tables];
	SSHORT flags = 0;

	if (tbl_lock->nod_arg[e_lock_mode])
		flags = tbl_lock->nod_arg[e_lock_mode]->nod_flags;

	if (flags & NOD_PROTECTED)
		lock_level = isc_tpb_protected;
	else if (flags & NOD_SHARED)
		lock_level = isc_tpb_shared;

	const USHORT lock_mode = (flags & NOD_WRITE) ? isc_tpb_lock_write : isc_tpb_lock_read;

	const dsql_nod* const* ptr = tbl_names->nod_arg;
	for (const dsql_nod* const* const end = ptr + tbl_names->nod_count; ptr < end; ptr++)
	{
		if ((*ptr)->nod_type != nod_relation_name)
			continue;

		stuff(statement, lock_mode);

		// stuff table name
		const dsql_str* temp = (dsql_str*) ((*ptr)->nod_arg[e_rln_name]);
		stuff_cstring(statement, temp->str_data);

		stuff(statement, lock_level);
	}
}


/**

 	gen_udf

    @brief	Generate a user defined function.


    @param statement
    @param node

 **/
static void gen_udf( CompiledStatement* statement, const dsql_nod* node)
{
	const dsql_udf* userFunc = (dsql_udf*) node->nod_arg[0];
	stuff(statement, blr_function);
	stuff_string(statement, userFunc->udf_name);

	const dsql_nod* list;
	if ((node->nod_count == 2) && (list = node->nod_arg[1]))
	{
		stuff(statement, list->nod_count);
		dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			GEN_expr(statement, *ptr);
		}
	}
	else
		stuff(statement, 0);
}


/**

 	gen_union

    @brief	Generate a union of substreams.


    @param statement
    @param union_node

 **/
static void gen_union( CompiledStatement* statement, const dsql_nod* union_node)
{
	if (union_node->nod_arg[0]->nod_flags & NOD_UNION_RECURSIVE) {
		stuff(statement, blr_recurse);
	}
	else {
		stuff(statement, blr_union);
	}

	// Obtain the context for UNION from the first dsql_map* node
	dsql_nod* items = union_node->nod_arg[e_rse_items];
	dsql_nod* map_item = items->nod_arg[0];
	// AB: First item could be a virtual field generated by derived table.
	if (map_item->nod_type == nod_derived_field) {
		map_item = map_item->nod_arg[e_derived_field_value];
	}
	dsql_ctx* union_context = (dsql_ctx*) map_item->nod_arg[e_map_context];
	stuff_context(statement, union_context);
	// secondary context number must be present once in generated blr
	union_context->ctx_flags &= ~CTX_recursive;

	dsql_nod* streams = union_node->nod_arg[e_rse_streams];
	stuff(statement, streams->nod_count);	// number of substreams

	dsql_nod** ptr = streams->nod_arg;
	for (const dsql_nod* const* const end = ptr + streams->nod_count; ptr < end; ptr++)
	{
		dsql_nod* sub_rse = *ptr;
		gen_rse(statement, sub_rse);
		items = sub_rse->nod_arg[e_rse_items];
		stuff(statement, blr_map);
		stuff_word(statement, items->nod_count);
		USHORT count = 0;
		dsql_nod** iptr = items->nod_arg;
		for (const dsql_nod* const* const iend = iptr + items->nod_count; iptr < iend; iptr++)
		{
			stuff_word(statement, count);
			GEN_expr(statement, *iptr);
			count++;
		}
	}
}


/**

 	stuff_context

    @brief	Write a context number into the BLR buffer.
			Check for possible overflow.


    @param statement
    @param context

 **/
static void stuff_context(CompiledStatement* statement, const dsql_ctx* context)
{
	if (context->ctx_context > MAX_UCHAR) {
		ERRD_post(Arg::Gds(isc_too_many_contexts));
	}
	stuff(statement, context->ctx_context);

	if (context->ctx_flags & CTX_recursive)
	{
		if (context->ctx_recursive > MAX_UCHAR) {
			ERRD_post(Arg::Gds(isc_too_many_contexts));
		}
		stuff(statement, context->ctx_recursive);
	}
}


/**

 	stuff_cstring

    @brief	Write out a string with one byte of length.


    @param statement
    @param string

 **/
static void stuff_cstring(CompiledStatement* statement, const char* string)
{
	stuff_string(statement, string, strlen(string));
}


/**

 	stuff_meta_string

    @brief	Write out a string in metadata charset with one byte of length.


    @param statement
    @param string

 **/
static void stuff_meta_string(CompiledStatement* statement, const char* string)
{
	statement->append_meta_string(string);
}


/**

 	stuff_string

    @brief	Write out a string with one byte of length.


    @param statement
    @param string

 **/
static void stuff_string(CompiledStatement* statement, const char* string, int len)
{
	fb_assert(len >= 0 && len <= 255);

	stuff(statement, len);
	statement->append_raw_string(string, len);
}


static void stuff_string(CompiledStatement* statement, const Firebird::MetaName& name)
{
	stuff_string(statement, name.c_str(), name.length());
}


/**

 	stuff_word

    @brief	Cram a word into the blr buffer.  If the buffer is getting
 	ready to overflow, expand it.


    @param statement
    @param word

 **/
static void stuff_word( CompiledStatement* statement, USHORT word)
{
	stuff(statement, word);
	stuff(statement, word >> 8);
}
