/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		gener.cpp
 *	DESCRIPTION:	BLR Generation Module
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
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../qli/dtr.h"
#include "../jrd/align.h"
#include "../qli/exe.h"
#include "../qli/compile.h"
#include "../qli/report.h"
#include "../qli/all_proto.h"
#include "../qli/compi_proto.h"
#include "../qli/err_proto.h"
#include "../qli/gener_proto.h"
#include "../qli/meta_proto.h"
#include "../qli/mov_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/gdsassert.h"

#ifdef DEV_BUILD
#include "../jrd/constants.h"
static void explain(qli_dbb* db, const UCHAR*);
static void explain_index_tree(qli_dbb* db, int, const TEXT*, const UCHAR**, int*);
static void explain_printf(int, const TEXT*, const TEXT*);
#endif

static void gen_any(qli_nod*, qli_req*);
static void gen_assignment(qli_nod*, qli_req*);
static void gen_control_break(qli_brk*, qli_req*);
static void gen_compile(qli_req*);
static void gen_descriptor(const dsc*, qli_req*);
static void gen_erase(qli_nod*, qli_req*);
static void gen_expression(qli_nod*, qli_req*);
static void gen_field(qli_nod*, qli_req*);
static void gen_for(qli_nod*, qli_req*);
static void gen_function(qli_nod*, qli_req*);
static void gen_if(qli_nod*, qli_req*);
static void gen_literal(const dsc*, qli_req*);
static void gen_map(qli_map*, qli_req*);
static void gen_modify(qli_nod*); //, qli_req*);
static void gen_parameter(const qli_par*, qli_req*);
static void gen_print_list(qli_nod*, qli_req*);
static void gen_report(qli_nod*, qli_req*);
static void gen_request(qli_req*);
static void gen_rse(qli_nod*, qli_req*);
static void gen_send_receive(const qli_msg*, USHORT);
static void gen_sort(qli_nod*, qli_req*, const UCHAR);
static void gen_statement(qli_nod*, qli_req*);
static void gen_statistical(qli_nod*, qli_req*);
static void gen_store(qli_nod*, qli_req*);

#ifdef DEV_BUILD
static const SCHAR explain_info[] = { isc_info_access_path };
#endif


qli_nod* GEN_generate( qli_nod* node)
{
/**************************************
 *
 *	G E N _ g e n e r a t e
 *
 **************************************
 *
 * Functional description
 *	Generate and compile BLR.
 *
 **************************************/

	gen_statement(node, 0);

	return node;
}


void GEN_release()
{
/**************************************
 *
 *	G E N _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release any compiled requests following execution or abandonment
 *	of a request.  Just recurse around and release requests.
 *
 **************************************/
	for (; QLI_requests; QLI_requests = QLI_requests->req_next)
	{
		if (QLI_requests->req_handle)
		{
			ISC_STATUS_ARRAY status_vector;
			isc_release_request(status_vector, &QLI_requests->req_handle);
		}

		qli_rlb* rlb = QLI_requests->req_blr;
		GEN_rlb_release (rlb);
	}
}


qli_rlb* GEN_rlb_extend(qli_rlb* rlb)
{
/**************************************
 *
 *	G E N _ r l b _ e x t e n d
 *
 **************************************
 *
 * Functional description
 *
 *	Allocate a new larger request language buffer
 *	and copy generated request language into it
 *
 **************************************/
	if (!rlb)
		rlb = (qli_rlb*) ALLOCD(type_rlb);

	UCHAR* const old_string = rlb->rlb_base;
	const ULONG len = rlb->rlb_data - rlb->rlb_base;
	rlb->rlb_length += RLB_BUFFER_SIZE;
	UCHAR* new_string = (UCHAR*) ALLQ_malloc((SLONG) rlb->rlb_length);
	if (old_string)
	{
		memcpy(new_string, old_string, len);
		ALLQ_free(old_string);
	}
	rlb->rlb_base = new_string;
	rlb->rlb_data = new_string + len;
	rlb->rlb_limit = rlb->rlb_data + RLB_BUFFER_SIZE - RLB_SAFETY_MARGIN;

	return rlb;
}


void GEN_rlb_release( qli_rlb* rlb)
{
/**************************************
 *
 *	G E N _ r l b _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *
 *	Release the request language buffer
 *	contents.  The buffer itself will go away
 *	with the default pool.
 *
 **************************************/

	if (!rlb)
		return;

	if (rlb->rlb_base)
	{
		ALLQ_free(rlb->rlb_base);
		rlb->rlb_base = NULL;
		rlb->rlb_length = 0;
		rlb->rlb_data = NULL;
		rlb->rlb_limit = NULL;
	}
}


#ifdef DEV_BUILD
static void explain(qli_dbb* db, const UCHAR* explain_buffer)
{
/**************************************
 *
 *	e x p l a i n
 *
 **************************************
 *
 * Functional description
 *	Print out the access path for a blr
 *	request in pretty-printed form.
 *
 **************************************/
	int level = 0;
	SCHAR relation_name[MAX_SQL_IDENTIFIER_SIZE];
	// CVC: This function may have the same bugs as the internal function
	// used in the engine, that received fixes in FB1 & FB1.5.

	if (*explain_buffer++ != isc_info_access_path)
		return;

	int buffer_length = (unsigned int) *explain_buffer++;
	buffer_length += (unsigned int) (*explain_buffer++) << 8;

	while (buffer_length > 0)
	{
		buffer_length--;
		switch (*explain_buffer++)
		{
		case isc_info_rsb_begin:
			explain_printf(level, "isc_info_rsb_begin,\n", 0);
			level++;
			break;

		case isc_info_rsb_end:
			explain_printf(level, "isc_info_rsb_end,\n", 0);
			level--;
			break;

		case isc_info_rsb_relation:
			{
				explain_printf(level, "isc_info_rsb_relation, ", 0);

				buffer_length--;
				const size_t length = *explain_buffer++;
				fb_assert(length < MAX_SQL_IDENTIFIER_SIZE);
				buffer_length -= length;

				memcpy(relation_name, explain_buffer, length);
				relation_name[length] = 0;
				explain_buffer += length;
				printf("%s,\n", relation_name);
			}
			break;

		case isc_info_rsb_type:
			buffer_length--;
			explain_printf(level, "isc_info_rsb_type, ", 0);
			switch (*explain_buffer++)
			{
			case isc_info_rsb_unknown:
				printf("unknown type\n");
				break;

			case isc_info_rsb_indexed:
				printf("isc_info_rsb_indexed,\n");
				level++;
				explain_index_tree(db, level, relation_name, &explain_buffer, &buffer_length);
				level--;
				break;

				// for join types, advance past the count byte
				// of substreams; we don't need it

			case isc_info_rsb_merge:
				buffer_length--;
				printf("isc_info_rsb_merge, %d,\n", *explain_buffer++);
				break;

			case isc_info_rsb_cross:
				buffer_length--;
				printf("isc_info_rsb_cross, %d,\n", *explain_buffer++);
				break;

			case isc_info_rsb_navigate:
				printf("isc_info_rsb_navigate,\n");
				level++;
				explain_index_tree(db, level, relation_name, &explain_buffer, &buffer_length);
				level--;
				break;

			case isc_info_rsb_sequential:
				printf("isc_info_rsb_sequential,\n");
				break;

			case isc_info_rsb_sort:
				printf("isc_info_rsb_sort,\n");
				break;

			case isc_info_rsb_first:
				printf("isc_info_rsb_first,\n");
				break;

			case isc_info_rsb_boolean:
				printf("isc_info_rsb_boolean,\n");
				break;

			case isc_info_rsb_union:
				printf("isc_info_rsb_union,\n");
				break;

			case isc_info_rsb_aggregate:
				printf("isc_info_rsb_aggregate,\n");
				break;

			case isc_info_rsb_ext_sequential:
				printf("isc_info_rsb_ext_sequential,\n");
				break;

			case isc_info_rsb_ext_indexed:
				printf("isc_info_rsb_ext_indexed,\n");
				break;

			case isc_info_rsb_ext_dbkey:
				printf("isc_info_rsb_ext_dbkey,\n");
				break;

			case isc_info_rsb_left_cross:
				printf("isc_info_rsb_left_cross,\n");
				break;

			case isc_info_rsb_select:
				printf("isc_info_rsb_select,\n");
				break;

			case isc_info_rsb_sql_join:
				printf("isc_info_rsb_sql_join,\n");
				break;

			case isc_info_rsb_simulate:
				printf("isc_info_rsb_simulate,\n");
				break;

			case isc_info_rsb_sim_cross:
				printf("isc_info_rsb_sim_cross,\n");
				break;

			case isc_info_rsb_once:
				printf("isc_info_rsb_once,\n");
				break;

			case isc_info_rsb_procedure:
				printf("isc_info_rsb_procedure,\n");
				break;

			}
			break;

		default:
			explain_printf(level, "unknown info item\n", 0);
			break;
		}
	}
}
#endif


#ifdef DEV_BUILD
static void explain_index_tree(qli_dbb* db, int level, const TEXT* relation_name,
							   const UCHAR** explain_buffer_ptr, int* buffer_length)
{
/**************************************
 *
 *	e x p l a i n _ i n d e x _ t r e e
 *
 **************************************
 *
 * Functional description
 *	Print out an index tree access path.
 *
 **************************************/

	const UCHAR* explain_buffer = *explain_buffer_ptr;

	(*buffer_length)--;

	switch (*explain_buffer++)
	{
	case isc_info_rsb_and:
		explain_printf(level, "isc_info_rsb_and,\n", 0);
		level++;
		explain_index_tree(db, level, relation_name, &explain_buffer, buffer_length);
		explain_index_tree(db, level, relation_name, &explain_buffer, buffer_length);
		level--;
		break;

	case isc_info_rsb_or:
		explain_printf(level, "isc_info_rsb_or,\n", 0);
		level++;
		explain_index_tree(db, level, relation_name, &explain_buffer, buffer_length);
		explain_index_tree(db, level, relation_name, &explain_buffer, buffer_length);
		level--;
		break;

	case isc_info_rsb_dbkey:
		explain_printf(level, "isc_info_rsb_dbkey,\n", 0);
		break;

	case isc_info_rsb_index:
		{
			explain_printf(level, "isc_info_rsb_index, ", 0);
			(*buffer_length)--;

			const size_t length = *explain_buffer++;
			TEXT index_name[MAX_SQL_IDENTIFIER_SIZE];
			memcpy(index_name, explain_buffer, length);
			index_name[length] = 0;

			*buffer_length -= length;
			explain_buffer += length;

			SCHAR index_info[256];
			MET_index_info(db, relation_name, index_name, index_info, sizeof(index_info));
			printf("%s\n", index_info);
		}
		break;
	}

	if (*explain_buffer == isc_info_rsb_end)
	{
		explain_buffer++;
		(*buffer_length)--;
	}

	*explain_buffer_ptr = explain_buffer;
}
#endif



#ifdef DEV_BUILD
static void explain_printf(int level, const TEXT* control, const TEXT* string)
{
/**************************************
 *
 *	e x p l a i n _ p r i n t f
 *
 **************************************
 *
 * Functional description
 *	Do a printf with formatting.
 *
 **************************************/

	while (level--)
		printf("   ");

	if (string)
		printf(control, string);
	else
		printf("%s", control);
}
#endif


static void gen_any( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ a n y
 *
 **************************************
 *
 * Functional description
 *	Generate the BLR for a statistical expresionn.
 *
 **************************************/
	qli_rlb* rlb;

	// If there is a request associated with the statement, prepare to
	// generate BLR.  Otherwise assume that a request is alrealdy initialized.

	qli_req* new_request = (qli_req*) node->nod_arg[e_any_request];
	if (new_request)
	{
		request = new_request;
		gen_request(request);
		const qli_msg* receive = (qli_msg*) node->nod_arg[e_any_send];
		if (receive)
			gen_send_receive(receive, blr_receive);
		const qli_msg* send = (qli_msg*) node->nod_arg[e_any_receive];
		gen_send_receive(send, blr_send);
		rlb = CHECK_RLB(request->req_blr);
		STUFF(blr_if);
	}
	else
		rlb = CHECK_RLB(request->req_blr);

	STUFF(blr_any);
	gen_rse(node->nod_arg[e_any_rse], request);

	USHORT value; // not feasible to change it to bool.
	dsc desc;
	if (new_request)
	{
		desc.dsc_dtype = dtype_short;
		desc.dsc_length = sizeof(SSHORT);
		desc.dsc_scale = 0;
		desc.dsc_sub_type = 0;
		desc.dsc_address = (UCHAR*) &value;
		QLI_validate_desc(desc);

		STUFF(blr_assignment);
		value = TRUE;
		gen_literal(&desc, request);
		gen_parameter(node->nod_import, request);

		STUFF(blr_assignment);
		value = FALSE;
		gen_literal(&desc, request);
		gen_parameter(node->nod_import, request);
		gen_compile(request);
	}
}


static void gen_assignment( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Generate an assignment statement.
 *
 **************************************/
	qli_nod* from = node->nod_arg[e_asn_from];

	// Handle a local expression locally

	if (node->nod_flags & NOD_local)
	{
		gen_expression(from, 0);
		return;
	}

	// Generate a remote assignment

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	STUFF(blr_assignment);
	qli_nod* target = node->nod_arg[e_asn_to];

	// If we are referencing a parameter of another request, compile
	// the request first, the make a reference.  Otherwise, just compile
	// the expression in the context of this request

	const qli_nod* reference = target->nod_arg[e_fld_reference];
	if (reference)
	{
		gen_expression(from, 0);
		gen_parameter(reference->nod_import, request);
	}
	else
		gen_expression(from, request);

	qli_nod* validation = node->nod_arg[e_asn_valid];
	if (validation)
		gen_expression(validation, 0);

	gen_expression(target, request);
}


static void gen_control_break( qli_brk* control, qli_req* request)
{
/**************************************
 *
 *	g e n _ c o n t r o l _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Process report writer control breaks.
 *
 **************************************/

	for (; control; control = control->brk_next)
	{
		if (control->brk_field)
			gen_expression((qli_nod*) control->brk_field, request);
		if (control->brk_line)
			gen_print_list((qli_nod*) control->brk_line, request);
	}
}


static void gen_compile( qli_req* request)
{
/**************************************
 *
 *	g e n _ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Finish off BLR generation for a request, and get it compiled.
 *
 **************************************/
	qli_rlb* rlb = CHECK_RLB(request->req_blr);
	STUFF(blr_end);
	STUFF(blr_eoc);

	const USHORT length = rlb->rlb_data - rlb->rlb_base;

	if (QLI_blr)
		fb_print_blr(rlb->rlb_base, length, 0, 0, 0);

	qli_dbb* dbb = request->req_database;

	ISC_STATUS_ARRAY status_vector;
	if (isc_compile_request(status_vector, &dbb->dbb_handle, &request->req_handle, length,
							 (const char*) rlb->rlb_base))
	{
		GEN_rlb_release (rlb);
		ERRQ_database_error(dbb, status_vector);
	}

#ifdef DEV_BUILD
	SCHAR explain_buffer[512];
	if (QLI_explain &&
		!isc_request_info(status_vector, &request->req_handle, 0,
						   sizeof(explain_info), explain_info,
						   sizeof(explain_buffer), explain_buffer))
	{
		explain(dbb, (UCHAR*) explain_buffer);
	}
#endif

	GEN_rlb_release (rlb);
}


static void gen_descriptor(const dsc* desc, qli_req* request)
{
/**************************************
 *
 *	g e n _ d e s c r i p t o r
 *
 **************************************
 *
 * Functional description
 *	Generator a field descriptor.  This is used for generating both
 *	message declarations and literals.
 *
 **************************************/
	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	switch (desc->dsc_dtype)
	{

	case dtype_text:
		STUFF(blr_text);
		STUFF_WORD(desc->dsc_length);
		break;

	case dtype_varying:
		STUFF(blr_varying);
		STUFF_WORD((desc->dsc_length - sizeof(SSHORT)));
		break;

	case dtype_cstring:
		STUFF(blr_cstring);
		STUFF_WORD(desc->dsc_length);
		break;

	case dtype_short:
		STUFF(blr_short);
		STUFF(desc->dsc_scale);
		break;

	case dtype_int64:
		STUFF(blr_int64);
		STUFF(desc->dsc_scale);
		break;

	case dtype_long:
		STUFF(blr_long);
		STUFF(desc->dsc_scale);
		break;

	case dtype_blob:
	case dtype_quad:
		STUFF(blr_quad);
		STUFF(desc->dsc_scale);
		break;

	case dtype_real:
		STUFF(blr_float);
		break;

	case dtype_double:
		STUFF(blr_double);
		break;

	case dtype_timestamp:
		STUFF(blr_timestamp);
		break;

	case dtype_sql_date:
		STUFF(blr_sql_date);
		break;

	case dtype_sql_time:
		STUFF(blr_sql_time);
		break;

	default:
		ERRQ_bugcheck(352);			// Msg352 gen_descriptor: dtype not recognized
	}
}


static void gen_erase( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Generate the BLR for an erase statement.  Pretty simple.
 *
 **************************************/
	const qli_msg* message = (qli_msg*) node->nod_arg[e_era_message];
	if (message)
	{
		request = (qli_req*) node->nod_arg[e_era_request];
		gen_send_receive(message, blr_receive);
	}

	qli_rlb* rlb = CHECK_RLB(request->req_blr);
	qli_ctx* context = (qli_ctx*) node->nod_arg[e_era_context];
	STUFF(blr_erase);
	STUFF(context->ctx_context);
}


static void gen_expression(qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ e x p r e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Generate the BLR for a boolean or value expression.
 *
 **************************************/
	USHORT operatr = 0;
	qli_rlb* rlb = 0;

	if (node->nod_flags & NOD_local)
	{
		request = NULL;
		//return;
	}
	else if (request)
		rlb = CHECK_RLB(request->req_blr);

	switch (node->nod_type)
	{
	case nod_any:
		gen_any(node, request);
		return;

	case nod_unique:
		if (request)
		{
			STUFF(blr_unique);
			gen_rse(node->nod_arg[e_any_rse], request);
		}
		return;

	case nod_constant:
		if (request)
			gen_literal(&node->nod_desc, request);
		return;

	case nod_field:
		gen_field(node, request);
		return;

	case nod_format:
		gen_expression(node->nod_arg[e_fmt_value], request);
		return;

	case nod_map:
		{
			qli_map* map = (qli_map*) node->nod_arg[e_map_map];
			const qli_ctx* context = (qli_ctx*) node->nod_arg[e_map_context];
			if (context->ctx_request != request && map->map_node->nod_type == nod_field)
			{
				gen_field(map->map_node, request);
				return;
			}
			STUFF(blr_fid);
			STUFF(context->ctx_context);
			STUFF_WORD(map->map_position);
			return;
		}

	case nod_eql:
		operatr = blr_eql;
		break;

	case nod_neq:
		operatr = blr_neq;
		break;

	case nod_gtr:
		operatr = blr_gtr;
		break;

	case nod_geq:
		operatr = blr_geq;
		break;

	case nod_leq:
		operatr = blr_leq;
		break;

	case nod_lss:
		operatr = blr_lss;
		break;

	case nod_containing:
		operatr = blr_containing;
		break;

	case nod_matches:
		operatr = blr_matching;
		break;

	case nod_sleuth:
		operatr = blr_matching2;
		break;

	case nod_like:
		operatr = (node->nod_count == 2) ? blr_like : blr_ansi_like;
		break;

	case nod_starts:
		operatr = blr_starting;
		break;

	case nod_missing:
		operatr = blr_missing;
		break;

	case nod_between:
		operatr = blr_between;
		break;

	case nod_and:
		operatr = blr_and;
		break;

	case nod_or:
		operatr = blr_or;
		break;

	case nod_not:
		operatr = blr_not;
		break;

	case nod_add:
		operatr = blr_add;
		break;

	case nod_subtract:
		operatr = blr_subtract;
		break;

	case nod_multiply:
		operatr = blr_multiply;
		break;

	case nod_divide:
		operatr = blr_divide;
		break;

	case nod_negate:
		operatr = blr_negate;
		break;

	case nod_concatenate:
		operatr = blr_concatenate;
		break;

	case nod_substr:
		operatr = blr_substring;
		break;

	case nod_function:
		gen_function(node, request);
		return;

	case nod_agg_average:
	case nod_agg_count:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:

	case nod_average:
	case nod_count:
	case nod_max:
	case nod_min:
	case nod_total:
	case nod_from:
		gen_statistical(node, request);
		return;

	case nod_rpt_average:
	case nod_rpt_count:
	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:

	case nod_running_total:
	case nod_running_count:
		if (node->nod_arg[e_stt_value])
			gen_expression(node->nod_arg[e_stt_value], request);
		if (node->nod_export)
			gen_parameter(node->nod_export, request);
		request = NULL;
		break;

	case nod_prompt:
	case nod_variable:
		if (node->nod_export)
			gen_parameter(node->nod_export, request);
		return;

	case nod_edit_blob:
	case nod_reference:
		return;

	case nod_null:
		operatr = blr_null;
		break;

	case nod_user_name:
		operatr = blr_user_name;
		break;

	case nod_upcase:
		operatr = blr_upcase;
		break;

	case nod_lowcase:
		operatr = blr_lowcase;
		break;

	default:
		if (request && node->nod_export)
		{
			gen_parameter(node->nod_export, request);
			return;
		}
		ERRQ_bugcheck(353);			// Msg353 gen_expression: not understood
	}

	if (request)
	{
		rlb = CHECK_RLB(request->req_blr);
		STUFF(operatr);
	}

	qli_nod** ptr = node->nod_arg;
	for (qli_nod** const end = ptr + node->nod_count; ptr < end; ptr++)
		gen_expression(*ptr, request);

	if (!node->nod_desc.dsc_address && node->nod_desc.dsc_length)
		CMP_alloc_temp(node);
}


static void gen_field( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Compile BLR to handle a field reference.  Things varying
 *	substantially depend on whether the field reference spans
 *	requests or not.
 *
 **************************************/
	// If there isn't a request specified, this is a reference.

	if (!request)
		return;

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	const qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
	const qli_ctx* context = (qli_ctx*) node->nod_arg[e_fld_context];

	// If the field referenced is in this request, just generate a field reference.

	if (context->ctx_request == request)
	{
		qli_nod* args = node->nod_arg[e_fld_subs];
		if (args)
			STUFF(blr_index);
		STUFF(blr_fid);
		STUFF(context->ctx_context);
		STUFF_WORD(field->fld_id);
		if (args)
		{
			STUFF(args->nod_count);
			qli_nod** ptr = args->nod_arg;
			for (const qli_nod* const* const end = ptr + args->nod_count; ptr < end; ++ptr)
			{
				gen_expression(*ptr, request);
			}
		}
		return;
	}

	// The field is in a different request.  Make a parameter reference instead.

	gen_parameter(node->nod_export, request);
}


static void gen_for( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ f o r
 *
 **************************************
 *
 * Functional description
 *	Generate BLR for a FOR loop, included synchronization messages.
 *
 **************************************/

	// If there is a request associated with the statement, prepare to
	// generate BLR.  Otherwise assume that a request is alrealdy initialized.

	if (node->nod_arg[e_for_request])
	{
		request = (qli_req*) node->nod_arg[e_for_request];
		gen_request(request);
	}

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	// If the statement requires an end of file marker, build a BEGIN/END around
	// the whole statement.

	const qli_msg* message = (qli_msg*) node->nod_arg[e_for_receive];
	if (message)
		STUFF(blr_begin);

	// If there is a message to be sent, build a receive for it

	if (node->nod_arg[e_for_send])
		gen_send_receive((qli_msg*) node->nod_arg[e_for_send], blr_receive);

	// Generate the FOR loop proper.

	STUFF(blr_for);
	gen_rse(node->nod_arg[e_for_rse], request);
	STUFF(blr_begin);

	// If data is to be received (included EOF), build a send

	const qli_par* eof = 0;
	dsc desc;
	USHORT value;

	if (message)
	{
		gen_send_receive(message, blr_send);
		STUFF(blr_begin);

		// Build assigments for all values referenced.

		for (const qli_par* parameter = message->msg_parameters; parameter;
			parameter = parameter->par_next)
		{
			if (parameter->par_value)
			{
				STUFF(blr_assignment);
				gen_expression(parameter->par_value, request);
				gen_parameter(parameter, request);
			}
		}

		// Next, make a FALSE for the end of file parameter

		eof = (qli_par*) node->nod_arg[e_for_eof];
		desc.dsc_dtype = dtype_short;
		desc.dsc_length = sizeof(SSHORT);
		desc.dsc_scale = 0;
		desc.dsc_sub_type = 0;
		desc.dsc_address = (UCHAR*) &value;
		QLI_validate_desc(desc);

		STUFF(blr_assignment);
		value = FALSE;
		gen_literal(&desc, request);
		gen_parameter(eof, request);

		STUFF(blr_end);
	}

	// Build  the body of the loop.

	const qli_msg* continuation = request->req_continue;
	if (continuation)
	{
		STUFF(blr_label);
		const USHORT label = request->req_label++;
		STUFF(label);
		STUFF(blr_loop);
		STUFF(blr_select);
		gen_send_receive(continuation, blr_receive);
		STUFF(blr_leave);
		STUFF(label);
	}

	qli_nod* sub = node->nod_arg[e_for_statement];
	gen_statement(sub, request);

	STUFF(blr_end);

	if (continuation)
		STUFF(blr_end);

	// Finish off by building a SEND to indicate end of file

	if (message)
	{
		gen_send_receive(message, blr_send);
		STUFF(blr_assignment);
		value = TRUE;
		gen_literal(&desc, request);
		gen_parameter(eof, request);
		STUFF(blr_end);
	}

	// If this is our request, compile it.

	if (node->nod_arg[e_for_request])
		gen_compile(request);
}


static void gen_function( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Generate blr for a function reference.
 *
 **************************************/
	qli_req* new_request;
	qli_rlb* rlb;

	// If there is a request associated with the statement, prepare to
	// generate BLR.  Otherwise assume that a request is already initialized.

	if (request && (request->req_flags & (REQ_project | REQ_group_by)))
	{
		new_request = NULL;
		rlb = CHECK_RLB(request->req_blr);
	}
	else if (new_request = (qli_req*) node->nod_arg[e_fun_request])
	{
		request = new_request;
		gen_request(request);
		const qli_msg* receive = (qli_msg*) node->nod_arg[e_fun_send];
		if (receive)
			gen_send_receive(receive, blr_receive);
		const qli_msg* send = (qli_msg*) node->nod_arg[e_fun_receive];
		gen_send_receive(send, blr_send);
		rlb = CHECK_RLB(request->req_blr);
		STUFF(blr_assignment);
	}
	else
		rlb = CHECK_RLB(request->req_blr);

	// Generate function body

	STUFF(blr_function);
	qli_fun* function = (qli_fun*) node->nod_arg[e_fun_function];
	qli_symbol* symbol = function->fun_symbol;
	STUFF(symbol->sym_length);
	for (const UCHAR* p = (UCHAR*) symbol->sym_string; *p;)
		STUFF(*p++);

	// Generate function arguments

	qli_nod* args = node->nod_arg[e_fun_args];
	STUFF(args->nod_count);

    qli_nod** ptr = args->nod_arg;
	for (const qli_nod* const* const end = ptr + args->nod_count; ptr < end; ptr++)
		gen_expression(*ptr, request);

	if (new_request)
	{
		gen_parameter(node->nod_import, request);
		gen_compile(request);
	}
}


static void gen_if( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ i f
 *
 **************************************
 *
 * Functional description
 *	Generate code for an IF statement.  Depending
 *	on the environment, this may be either a QLI
 *	context IF or a database context IF.
 *
 **************************************/

	// If the statement is local to QLI, force the sub- statements/expressions to be local also.
	// If not local, generate BLR.

	if (node->nod_flags & NOD_local)
	{
		gen_expression(node->nod_arg[e_if_boolean], 0);
		gen_statement(node->nod_arg[e_if_true], request);
		if (node->nod_arg[e_if_false])
			gen_statement(node->nod_arg[e_if_false], request);
	}
	else
	{
	    // probably the var is irrelevant, but not the function call,
	    // as it makes sure there's enough room.
		qli_rlb* rlb = CHECK_RLB(request->req_blr);
		STUFF(blr_if);
		gen_expression(node->nod_arg[e_if_boolean], request);
		STUFF(blr_begin);
		gen_statement(node->nod_arg[e_if_true], request);
		STUFF(blr_end);
		if (node->nod_arg[e_if_false])
		{
			STUFF(blr_begin);
			gen_statement(node->nod_arg[e_if_false], request);
			STUFF(blr_end);
		}
		else
			STUFF(blr_end);
	}
}


static void gen_literal(const dsc* desc, qli_req* request)
{
/**************************************
 *
 *	g e n _ l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Generate a literal value expression.
 *
 **************************************/
	SLONG value;

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	STUFF(blr_literal);
	gen_descriptor(desc, request);
	const UCHAR* p = desc->dsc_address;
	USHORT l = desc->dsc_length;

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *(SSHORT *) p;
		STUFF_WORD(value);
		break;

	case dtype_long:
	case dtype_sql_time:
	case dtype_sql_date:
		value = *(SLONG *) p;
		STUFF_WORD(value);
		STUFF_WORD(value >> 16);
		break;
    case dtype_int64:
	case dtype_quad:
	case dtype_timestamp:
		value = *(SLONG *) p;
		STUFF_WORD(value);
		STUFF_WORD(value >> 16);
		value = *(SLONG *) (p + 4);
		STUFF_WORD(value);
		STUFF_WORD(value >> 16);
		break;

	default:
		if (l)
			do {
				STUFF(*p++);
			} while (--l);
	}
}


static void gen_map(qli_map* map, qli_req* request)
{
/**************************************
 *
 *	g e n _ m a p
 *
 **************************************
 *
 * Functional description
 *	Generate a value map for a record selection expression.
 *
 **************************************/
	qli_map* temp;

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	USHORT count = 0;
	for (temp = map; temp; temp = temp->map_next)
	{
		if (temp->map_node->nod_type != nod_function)
			temp->map_position = count++;
	}

	STUFF(blr_map);
	STUFF_WORD(count);

	for (temp = map; temp; temp = temp->map_next)
	{
		if (temp->map_node->nod_type != nod_function)
		{
			STUFF_WORD(temp->map_position);
			gen_expression(temp->map_node, request);
		}
	}
}


static void gen_modify( qli_nod* node) //, qli_req* org_request)
{
/**************************************
 *
 *	g e n _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Generate a MODIFY statement.
 *
 **************************************/
	qli_req* request = (qli_req*) node->nod_arg[e_mod_request];

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	if (node->nod_arg[e_mod_send])
		gen_send_receive((qli_msg*) node->nod_arg[e_mod_send], blr_receive);

	qli_nod** ptr = &node->nod_arg[e_mod_count];
	for (const qli_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
	{
		qli_ctx* context = (qli_ctx*) *ptr;
		STUFF(blr_modify);
		STUFF(context->ctx_source->ctx_context);
		STUFF(context->ctx_context);
	}
	STUFF(blr_begin);
	gen_statement(node->nod_arg[e_mod_statement], request);
	STUFF(blr_end);
}



static void gen_parameter(const qli_par* parameter, qli_req* request)
{
/**************************************
 *
 *	g e n _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Generate a simple parameter reference.
 *
 **************************************/
	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	const qli_msg* message = parameter->par_message;
	if (!parameter->par_missing)
	{
		STUFF(blr_parameter);
		STUFF(message->msg_number);
		STUFF_WORD(parameter->par_parameter);
	}
	else
	{
		STUFF(blr_parameter2);
		STUFF(message->msg_number);
		STUFF_WORD(parameter->par_parameter);
		STUFF_WORD(parameter->par_missing->par_parameter);
	}
}


static void gen_print_list( qli_nod* list, qli_req* request)
{
/**************************************
 *
 *	g e n _ p r i n t _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Generate BLR for a print list.
 *
 **************************************/
	qli_nod** ptr = list->nod_arg;
	for (const qli_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		qli_print_item* item = (qli_print_item*) *ptr;
		if (item->itm_type == item_value)
			gen_expression(item->itm_value, request);
	}
}


static void gen_report( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Compile the body of a report specification.
 *
 **************************************/
	qli_rpt* report = (qli_rpt*) node->nod_arg[e_prt_list];

	gen_control_break(report->rpt_top_rpt, request);
	gen_control_break(report->rpt_top_page, request);
	gen_control_break(report->rpt_top_breaks, request);

    qli_nod* list = report->rpt_detail_line;
	if (list)
		gen_print_list(list, request);

	gen_control_break(report->rpt_bottom_breaks, request);
	gen_control_break(report->rpt_bottom_page, request);
	gen_control_break(report->rpt_bottom_rpt, request);
}


static void gen_request( qli_req* request)
{
/**************************************
 *
 *	g e n _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Prepare to generation and compile a request.
 *
 **************************************/
	qli_par* param;

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	STUFF(blr_version4);
	STUFF(blr_begin);

	// Build declarations for all messages.

	for (qli_msg* message = request->req_messages; message; message = message->msg_next)
	{
		message->msg_length = 0;
		for (param = message->msg_parameters; param; param = param->par_next)
		{
			const dsc* desc = &param->par_desc;
			param->par_parameter = message->msg_parameter++;
			const USHORT alignment = type_alignments[desc->dsc_dtype];
			if (alignment)
				message->msg_length = FB_ALIGN(message->msg_length, alignment);
			param->par_offset = message->msg_length;
			message->msg_length += desc->dsc_length;
			qli_par* missing_param = param->par_missing;
			if (missing_param)
			{
				missing_param->par_parameter = message->msg_parameter++;
				message->msg_length = FB_ALIGN(message->msg_length, sizeof(USHORT));
				desc = &missing_param->par_desc;
				missing_param->par_offset = message->msg_length;
				message->msg_length += desc->dsc_length;
			}
		}

		STUFF(blr_message);
		STUFF(message->msg_number);
		STUFF_WORD(message->msg_parameter);

		qli_str* string = (qli_str*) ALLOCDV(type_str, message->msg_length + FB_DOUBLE_ALIGN - 1);
		message->msg_buffer = (UCHAR*) FB_ALIGN((FB_UINT64)(U_IPTR) string->str_data, FB_DOUBLE_ALIGN);

		for (param = message->msg_parameters; param; param = param->par_next)
		{
			gen_descriptor(&param->par_desc, request);
			qli_par* missing_param = param->par_missing;
			if (missing_param)
				gen_descriptor(&missing_param->par_desc, request);
		}
	}
}


static void gen_rse( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ r s e
 *
 **************************************
 *
 * Functional description
 *	Generate a record selection expression.
 *
 **************************************/

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	if ((nod_t) (IPTR) node->nod_arg[e_rse_join_type] == nod_nothing)
		STUFF(blr_rse);
	else
		STUFF(blr_rs_stream);
	STUFF(node->nod_count);

	// Check for aggregate case
	qli_nod* list;
	qli_ctx* context = (qli_ctx*) node->nod_arg[e_rse_count];

	if (context->ctx_sub_rse)
	{
		STUFF(blr_aggregate);
		STUFF(context->ctx_context);
		gen_rse(context->ctx_sub_rse, request);
		STUFF(blr_group_by);
		if (list = node->nod_arg[e_rse_group_by])
		{
			request->req_flags |= REQ_group_by;
			STUFF(list->nod_count);
			qli_nod** ptr = list->nod_arg;
			for (const qli_nod* const* const end = ptr + list->nod_count; ptr < end; ++ptr)
			{
				gen_expression(*ptr, request);
			}
			request->req_flags &= ~REQ_group_by;
		}
		else
			STUFF(0);
		gen_map(context->ctx_map, request);
		if (list = node->nod_arg[e_rse_having])
		{
			STUFF(blr_boolean);
			gen_expression(list, request);
		}
		if (list = node->nod_arg[e_rse_sort])
			gen_sort(list, request, blr_sort);
		STUFF(blr_end);
		return;
	}

	// Make relation clauses for all relations

	qli_nod** ptr = &node->nod_arg[e_rse_count];
	for (const qli_nod* const* const end = ptr + node->nod_count; ptr < end; ++ptr)
	{
		context = (qli_ctx*) *ptr;
		if (context->ctx_stream)
			gen_rse(context->ctx_stream, request);
		else
		{
			const qli_rel* relation = context->ctx_relation;
			STUFF(blr_rid);
			STUFF_WORD(relation->rel_id);
			STUFF(context->ctx_context);
		}
	}

	// Handle various clauses

	if (list = node->nod_arg[e_rse_first])
	{
		STUFF(blr_first);
		gen_expression(list, request);
	}

	if (list = node->nod_arg[e_rse_boolean])
	{
		STUFF(blr_boolean);
		gen_expression(list, request);
	}

	if (list = node->nod_arg[e_rse_sort])
		gen_sort(list, request, blr_sort);

	if (list = node->nod_arg[e_rse_reduced])
		gen_sort(list, request, blr_project);

	const nod_t join_type = (nod_t) (IPTR) node->nod_arg[e_rse_join_type];
	if (join_type != nod_nothing && join_type != nod_join_inner)
	{
		STUFF(blr_join_type);
		switch (join_type)
		{
		case nod_join_left:
			STUFF(blr_left);
			break;
		case nod_join_right:
			STUFF(blr_right);
			break;
		default:
			STUFF(blr_full);
		}
	}

	STUFF(blr_end);
}


static void gen_send_receive( const qli_msg* message, USHORT operatr)
{
/**************************************
 *
 *	g e n _ s e n d _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Generate a send or receive statement, excluding body.
 *
 **************************************/
	qli_req* request = message->msg_request;
	qli_rlb* rlb = CHECK_RLB(request->req_blr);
	STUFF(operatr);
	STUFF(message->msg_number);
}


static void gen_sort( qli_nod* node, qli_req* request, const UCHAR operatr)
{
/**************************************
 *
 *	g e n _ s o r t
 *
 **************************************
 *
 * Functional description
 *	Generate sort or reduced clause.
 *
 **************************************/
	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	STUFF(operatr);
	STUFF(node->nod_count);

	request->req_flags |= REQ_project;
	qli_nod** ptr = node->nod_arg;
	for (qli_nod** const end = ptr + node->nod_count * 2; ptr < end; ptr += 2)
	{
		if (operatr == blr_sort)
			STUFF(ptr[1] ? blr_descending : blr_ascending);
		gen_expression(ptr[0], request);
	}
	request->req_flags &= ~REQ_project;
}


static void gen_statement( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Generate BLR for statement.
 *
 **************************************/
	if (request)
		CHECK_RLB(request->req_blr);

	switch (node->nod_type)
	{
	case nod_abort:
		if (node->nod_count)
			gen_expression(node->nod_arg[0], 0);
		return;

	case nod_assign:
		gen_assignment(node, request);
		return;

	case nod_commit_retaining:
		return;

	case nod_erase:
		gen_erase(node, request);
		return;

	case nod_for:
	case nod_report_loop:
		gen_for(node, request);
		return;

	case nod_list:
	    {
	        qli_nod** ptr = node->nod_arg;
			for (const qli_nod* const* const end = ptr + node->nod_count; ptr < end; ++ptr)
			{
				gen_statement(*ptr, request);
			}
			return;
		}

	case nod_modify:
		gen_modify(node); //, request);
		return;

	case nod_output:
		gen_statement(node->nod_arg[e_out_statement], request);
		return;

	case nod_print:
		gen_print_list(node->nod_arg[e_prt_list], request);
		return;

	case nod_repeat:
		gen_statement(node->nod_arg[e_rpt_statement], request);
		return;

	case nod_report:
		gen_report(node, request);
		return;

	case nod_store:
		gen_store(node, request);
		return;

	case nod_if:
		gen_if(node, request);
		return;

	default:
		ERRQ_bugcheck(354);			// Msg354 gen_statement: not yet implemented
	}
}


static void gen_statistical( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ s t a t i s t i c a l
 *
 **************************************
 *
 * Functional description
 *	Generate the BLR for a statistical expresionn.
 *
 **************************************/
	USHORT operatr;

	switch (node->nod_type)
	{
	case nod_average:
		operatr = blr_average;
		break;

	case nod_count:
		// count2
		// operatr = node->nod_arg [e_stt_value] ? blr_count2 : blr_count;
		operatr = blr_count;
		break;

	case nod_max:
		operatr = blr_maximum;
		break;

	case nod_min:
		operatr = blr_minimum;
		break;

	case nod_total:
		operatr = blr_total;
		break;

	case nod_agg_average:
		operatr = blr_agg_average;
		break;

	case nod_agg_count:
		// count2
		// operatr = node->nod_arg [e_stt_value] ? blr_agg_count2 : blr_agg_count;
		operatr = blr_agg_count;
		break;

	case nod_agg_max:
		operatr = blr_agg_max;
		break;

	case nod_agg_min:
		operatr = blr_agg_min;
		break;

	case nod_agg_total:
		operatr = blr_agg_total;
		break;

	case nod_from:
		operatr = node->nod_arg[e_stt_default] ? blr_via : blr_from;
		break;

	default:
		ERRQ_bugcheck(355);			// Msg355 gen_statistical: not understood
	}

	// If there is a request associated with the statement, prepare to
	// generate BLR.  Otherwise assume that a request is alrealdy initialized.

	qli_rlb* rlb;
	qli_req* new_request = (qli_req*) node->nod_arg[e_stt_request];
	if (new_request)
	{
		request = new_request;
		gen_request(request);
		const qli_msg* receive = (qli_msg*) node->nod_arg[e_stt_send];
		if (receive)
			gen_send_receive(receive, blr_receive);
		const qli_msg* send = (qli_msg*) node->nod_arg[e_stt_receive];
		gen_send_receive(send, blr_send);
		rlb = CHECK_RLB(request->req_blr);
		STUFF(blr_assignment);
	}
	else
		rlb = CHECK_RLB(request->req_blr);

	STUFF(operatr);

	if (node->nod_arg[e_stt_rse])
		gen_rse(node->nod_arg[e_stt_rse], request);

	// count 2
	// if (node->nod_arg [e_stt_value])

	if (node->nod_arg[e_stt_value] && node->nod_type != nod_agg_count)
		gen_expression(node->nod_arg[e_stt_value], request);

	if (node->nod_arg[e_stt_default])
		gen_expression(node->nod_arg[e_stt_default], request);

	if (new_request)
	{
		gen_parameter(node->nod_import, request);
		gen_compile(request);
	}
}


static void gen_store( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	g e n _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Generate code for STORE statement.
 *
 **************************************/

	// If there is a request associated with the statement, prepare to
	// generate BLR.  Otherwise assume that a request is alrealdy initialized.

	if (node->nod_arg[e_sto_request])
	{
		request = (qli_req*) node->nod_arg[e_sto_request];
		gen_request(request);
	}

	qli_rlb* rlb = CHECK_RLB(request->req_blr);

	// If there is a message to be sent, build a receive for it

	if (node->nod_arg[e_sto_send])
		gen_send_receive((qli_msg*) node->nod_arg[e_sto_send], blr_receive);

	// Generate the STORE statement proper.

	STUFF(blr_store);
	qli_ctx* context = (qli_ctx*) node->nod_arg[e_sto_context];
	qli_rel* relation = context->ctx_relation;
	STUFF(blr_rid);
	STUFF_WORD(relation->rel_id);
	STUFF(context->ctx_context);

	// Build  the body of the loop.

	STUFF(blr_begin);
	gen_statement(node->nod_arg[e_sto_statement], request);
	STUFF(blr_end);

	// If this is our request, compile it.

	if (node->nod_arg[e_sto_request])
		gen_compile(request);
}


