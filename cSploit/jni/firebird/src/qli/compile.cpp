/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		compile.cpp
 *	DESCRIPTION:	Compile expanded statement into executable things
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
#include <string.h>
#include "../qli/dtr.h"
#include "../qli/compile.h"
#include "../qli/exe.h"
#include "../qli/report.h"
#include "../jrd/intl.h"
#include "../qli/all_proto.h"
#include "../qli/compi_proto.h"
#include "../qli/err_proto.h"
#include "../qli/forma_proto.h"
#include "../qli/meta_proto.h"
#include "../common/dsc_proto.h"
#include "../common/gdsassert.h"
#include "../jrd/align.h"

const USHORT PROMPT_LENGTH	= 80;

static qli_nod* compile_any(qli_nod*, qli_req*);
static qli_nod* compile_assignment(qli_nod*, qli_req*, bool);
static void compile_context(qli_nod*, qli_req*, bool);
static void compile_control_break(qli_brk*, qli_req*);
static qli_nod* compile_edit(qli_nod*, qli_req*);
static qli_nod* compile_erase(qli_nod*, qli_req*);
static qli_nod* compile_expression(qli_nod*, qli_req*, bool);
static qli_nod* compile_field(qli_nod*, qli_req*, bool);
static qli_nod* compile_for(qli_nod*, qli_req*, bool);
static qli_nod* compile_function(qli_nod*, qli_req*, bool);
static qli_nod* compile_if(qli_nod*, qli_req*, bool);
static qli_nod* compile_list_fields(qli_nod*, qli_req*);
static qli_nod* compile_modify(qli_nod*, qli_req*, bool);
static qli_nod* compile_print(qli_nod*, qli_req*);
static qli_nod* compile_print_list(qli_nod*, qli_req*, qli_lls**);
static qli_nod* compile_prompt(qli_nod*);
static qli_nod* compile_repeat(qli_nod*, qli_req*, bool);
static qli_nod* compile_report(qli_nod*, qli_req*);
static qli_req* compile_rse(qli_nod*, qli_req*, bool, qli_msg**, qli_msg**, qli_dbb**);
static qli_nod* compile_statement(qli_nod*, qli_req*, bool);
static qli_nod* compile_statistical(qli_nod*, qli_req*, bool);
static qli_nod* compile_store(qli_nod*, qli_req*, bool);
static bool computable(qli_nod*, qli_req*);
static void make_descriptor(qli_nod*, dsc*);
static qli_msg* make_message(qli_req*);
static void make_missing_reference(qli_par*);
static qli_par* make_parameter(qli_msg*, qli_nod*);
static qli_nod* make_reference(qli_nod*, qli_msg*);
static qli_req* make_request(qli_dbb*);
static void release_message(qli_msg*);
static int string_length(const dsc*);

static qli_lls* print_items;
static TEXT** print_header;
static qli_brk* report_control_break;


qli_nod* CMPQ_compile( qli_nod* node)
{
/**************************************
 *
 *	C M P _ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile a statement into something executable.
 *
 **************************************/

	print_header = NULL;
	print_items = NULL;
	QLI_requests = NULL;
	compile_statement(node, 0, true);

	if (print_header)
		*print_header = FMT_format(print_items);

	return node;
}


void CMP_alloc_temp(qli_nod* node)
{
/**************************************
 *
 *	C M P _ a l l o c _ t e m p
 *
 **************************************
 *
 * Functional description
 *	Allocate a data area for a node.
 *
 **************************************/
	if (node->nod_desc.dsc_address)
		return;

	qli_str* string = (qli_str*) ALLOCDV(type_str, node->nod_desc.dsc_length +
					     type_alignments[node->nod_desc.dsc_dtype]);
	node->nod_desc.dsc_address = (UCHAR*)
		FB_ALIGN((FB_UINT64)(U_IPTR)(string->str_data), type_alignments[node->nod_desc.dsc_dtype]);
	QLI_validate_desc(node->nod_desc);
}


bool CMP_node_match( const qli_nod* node1, const qli_nod* node2)
{
/**************************************
 *
 *	C M P _ n o d e _ m a t c h
 *
 **************************************
 *
 * Functional description
 *	Compare two nodes for equality of value.
 *
 **************************************/
	if (!node1 || !node2 || node1->nod_type != node2->nod_type)
		return false;

	switch (node1->nod_type)
	{
	case nod_field:
		if (node1->nod_arg[e_fld_field] != node2->nod_arg[e_fld_field] ||
			node1->nod_arg[e_fld_context] != node2->nod_arg[e_fld_context] ||
			node1->nod_arg[e_fld_subs] != node2->nod_arg[e_fld_subs])
		{
			return false;
		}
		return true;

	case nod_constant:
		{
			if (node1->nod_desc.dsc_dtype != node2->nod_desc.dsc_dtype ||
				node1->nod_desc.dsc_scale != node2->nod_desc.dsc_scale ||
				node1->nod_desc.dsc_length != node2->nod_desc.dsc_length)
			{
				return false;
			}
			int l = node1->nod_desc.dsc_length;
			if (l)
				return memcmp(node1->nod_desc.dsc_address, node2->nod_desc.dsc_address, l) == 0;

			return true;
		}

	case nod_map:
		{
			const qli_map* map1 = (qli_map*) node1->nod_arg[e_map_map];
			const qli_map* map2 = (qli_map*) node2->nod_arg[e_map_map];
			return CMP_node_match(map1->map_node, map2->map_node);
		}

	case nod_agg_average:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:
	case nod_agg_count:
		return CMP_node_match(node1->nod_arg[e_stt_value], node2->nod_arg[e_stt_value]);

	case nod_function:
		if (node1->nod_arg[e_fun_function] != node2->nod_arg[e_fun_function])
			return false;
		return CMP_node_match(node1->nod_arg[e_fun_args], node2->nod_arg[e_fun_args]);
	}

	const qli_nod* const* ptr1 = node1->nod_arg;
	const qli_nod* const* ptr2 = node2->nod_arg;

	for (const qli_nod* const* const end = ptr1 + node1->nod_count; ptr1 < end; ++ptr1, ++ptr2)
	{
		if (!CMP_node_match(*ptr1, *ptr2))
			return false;
	}

	return true;
}


static qli_nod* compile_any( qli_nod* node, qli_req* old_request)
{
/**************************************
 *
 *	c o m p i l e _ a n y
 *
 **************************************
 *
 * Functional description
 *	Compile either an ANY or UNIQUE boolean expression.
 *
 **************************************/
	qli_msg* old_send = NULL;
	qli_msg* old_receive = NULL;

	if (old_request)
	{
		old_send = old_request->req_send;
		old_receive = old_request->req_receive;
	}

    qli_msg* send;
	qli_msg* receive;
	qli_req* request = compile_rse(node->nod_arg[e_any_rse], old_request, false, &send, &receive, 0);
	if (request)
		node->nod_arg[e_any_request] = (qli_nod*) request;
	else
		request = old_request;

	if (old_request)
	{
		old_request->req_send = old_send;
		old_request->req_receive = old_receive;
	}

	if ((send != old_send) && !send->msg_parameters)
	{
		release_message(send);
		send = NULL;
		if (!receive)
			return NULL;
	}

	if (old_request && request->req_database != old_request->req_database)
		IBERROR(357);			// Msg357 relations from multiple databases in single rse

	if (old_request && (!receive || !receive->msg_parameters))
	{
		if (receive)
			release_message(receive);
		receive = NULL;
	}

	if (receive)
	{
		qli_par* parameter = make_parameter(receive, 0);
		node->nod_import = parameter;
		parameter->par_desc.dsc_dtype = dtype_short;
		parameter->par_desc.dsc_length = sizeof(SSHORT);
	}

	node->nod_arg[e_any_send] = (qli_nod*) send;
	node->nod_arg[e_any_receive] = (qli_nod*) receive;

	return node;
}


static qli_nod* compile_assignment( qli_nod* node, qli_req* request, bool statement_internal)
{
/**************************************
 *
 *	c o m p i l e _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description:
 *	Compile an assignment statement, passing
 *	it to the database if possible.  Possibility
 *	happens when both sides of the assignment are
 *	computable in the request context.  (Following
 *	the logic of such things, the 'internal' flags
 *	mean external to qli, but internal to jrd.
 *	As is well known, the seat of the soul is the
 *	dbms).
 *
 **************************************/

	// Start by assuming that the assignment will ultimately
	// take place in the DBMS

	qli_nod* to = node->nod_arg[e_asn_to];
	to->nod_flags |= NOD_parameter2;
	qli_nod* from = node->nod_arg[e_asn_from];
	from->nod_flags |= NOD_parameter2;

	// If the assignment is to a variable, the assignment is completely local

	if (to->nod_type == nod_variable)
	{
		statement_internal = false;
		node->nod_flags |= NOD_local;
	}

	const bool target_internal = computable(to, request);
	statement_internal = statement_internal && request && target_internal && computable(from, request);

	qli_nod* target = compile_expression(to, request, target_internal);
	node->nod_arg[e_asn_to] = target;
	node->nod_arg[e_asn_from] = compile_expression(from, request, statement_internal);
	qli_nod* initial = node->nod_arg[e_asn_initial];
	if (initial)
		node->nod_arg[e_asn_initial] = compile_expression(initial, request, false);

	if (statement_internal)
	{
		node->nod_arg[e_asn_valid] = NULL;	// Memory reclaimed in the main loop
		node->nod_flags |= NOD_remote;
		node = NULL;
	}
	else
	{
		if (node->nod_arg[e_asn_valid])
			compile_expression(node->nod_arg[e_asn_valid], request, false);
		if (target->nod_type == nod_field)
		{
			if (!request)
			{
				qli_ctx* context = (qli_ctx*) target->nod_arg[e_fld_context];
				request = context->ctx_request;
			}
			target->nod_arg[e_fld_reference] = make_reference(target, request->req_send);
		}
	}

	return node;
}


static void compile_context( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ c o n t e x t
 *
 **************************************
 *
 * Functional description
 *	Set all of the contexts in a request.
 *	This may require a recursive call.
 *
 **************************************/
	qli_ctx** ctx_ptr = (qli_ctx**) node->nod_arg + e_rse_count;
	const qli_ctx* const* const ctx_end = ctx_ptr + node->nod_count;

	for (; ctx_ptr < ctx_end; ctx_ptr++)
	{
		qli_ctx* context = *ctx_ptr;
		context->ctx_request = request;
		context->ctx_context = request->req_context++;
		context->ctx_message = request->req_receive;
		if (context->ctx_sub_rse)
			compile_rse(context->ctx_sub_rse, request, internal_flag, 0, 0, 0);
		if (context->ctx_stream)
			compile_context(context->ctx_stream, request, internal_flag);
	}
}


static void compile_control_break( qli_brk* control, qli_req* request)
{
/**************************************
 *
 *	c o m p i l e _ c o n t r o l _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Compile a control/page/report break.
 *
 **************************************/
	for (; control; control = control->brk_next)
	{
		report_control_break = control;
		if (control->brk_field)
		{
			// control->brk_field  = (qli_syntax*) compile_expression (control->brk_field, request, false);
			qli_nod* temp = (qli_nod*) control->brk_field;
			temp->nod_flags |= NOD_parameter2;
			temp =  compile_expression((qli_nod*) control->brk_field, request, false);
			if (temp->nod_type == nod_field)
				temp = temp->nod_arg[e_fld_reference];
			control->brk_field = (qli_syntax*) temp;
		}
		if (control->brk_line)
			compile_print_list((qli_nod*) control->brk_line, request, 0);
		report_control_break = NULL;
	}
}


static qli_nod* compile_edit( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	c o m p i l e _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Compile the "edit blob" expression.
 *
 **************************************/

	// Make sure there is a message.  If there isn't a message, we
	// can't find the target database.

	if (!request)
		ERRQ_bugcheck(358);			// Msg358 can't find database for blob edit

	// If there is an input blob, get it now.

	qli_nod* value = node->nod_arg[e_edt_input];
	if (value)
	{
		qli_fld* field = (qli_fld*) value->nod_arg[e_fld_field];
		if (value->nod_type != nod_field || field->fld_dtype != dtype_blob)
			IBERROR(356);		// Msg356 EDIT argument must be a blob field
		node->nod_arg[e_edt_input] = compile_expression(value, request, false);
	}

	node->nod_arg[e_edt_dbb] = (qli_nod*) request->req_database;
	node->nod_desc.dsc_dtype = dtype_blob;
	node->nod_desc.dsc_length = 8;
	node->nod_desc.dsc_address = (UCHAR*) &node->nod_arg[e_edt_id1];
	QLI_validate_desc(node->nod_desc);

	return node;
}


static qli_nod* compile_erase( qli_nod* node, qli_req* org_request)
{
/**************************************
 *
 *	c o m p i l e _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Compile an ERASE statement.  This is so simple that nothing
 *	needs to be done.
 *
 **************************************/
	qli_ctx* context = (qli_ctx*) node->nod_arg[e_era_context];
	qli_req* request = context->ctx_request;
	node->nod_arg[e_era_request] = (qli_nod*) request;

	request->req_database->dbb_flags |= DBB_updates;

	if (org_request == request && !request->req_continue)
		return NULL;

	if (!request->req_continue)
		request->req_continue = make_message(request);

	node->nod_arg[e_era_message] = (qli_nod*) make_message(request);

	return node;
}


static qli_nod* compile_expression( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ e x p r e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Compile a value.  The value may be used internally as part of
 *	another expression (internal_flag == true) or may be referenced
 *	in the QLI context (internal_flag == false).
 *
 **************************************/
	qli_nod** ptr;
	const qli_nod* const* end;
	qli_nod* value;
	qli_map* map;
	qli_fld* field;

	switch (node->nod_type)
	{
	case nod_any:
	case nod_unique:
		return compile_any(node, request);

	case nod_max:
	case nod_min:
	case nod_count:
	case nod_average:
	case nod_total:
	case nod_from:
		node->nod_count = 0;
		return compile_statistical(node, request, internal_flag);

	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_count:
	case nod_rpt_average:
	case nod_rpt_total:
		if (report_control_break)
			ALLQ_push((blk*) node, &report_control_break->brk_statisticals);

	case nod_running_total:
	case nod_running_count:
		node->nod_count = 0;
		if (value = node->nod_arg[e_stt_value])
		{
			value->nod_flags |= NOD_parameter2;
			node->nod_arg[e_stt_value] = compile_expression(value, request, false);
		}
		make_descriptor(node, &node->nod_desc);
		if (internal_flag)
		{
			qli_par* parm = make_parameter(request->req_send, 0);
			node->nod_export = parm;
			parm->par_desc = node->nod_desc;
			parm->par_value = node;
		}
		else
		{
			CMP_alloc_temp(node);
			if (request && value && computable(value, request))
				node->nod_arg[e_stt_value] = make_reference(value, request->req_receive);
		}
		return node;

	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_count:
	case nod_agg_average:
	case nod_agg_total:
		node->nod_count = 0;
		if (value = node->nod_arg[e_stt_value])
		{
			value->nod_flags |= NOD_parameter2;
			node->nod_arg[e_stt_value] = compile_expression(value, request, true);
		}
		make_descriptor(node, &node->nod_desc);
		if (!internal_flag && request)
			return make_reference(node, request->req_receive);
		return node;

	case nod_map:
		map = (qli_map*) node->nod_arg[e_map_map];
		map->map_node = value = compile_expression(map->map_node, request, true);
		make_descriptor(value, &node->nod_desc);
		if (!internal_flag && request)
			return make_reference(node, request->req_receive);
		return node;

	case nod_function:
		return compile_function(node, request, internal_flag);

	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_leq:
	case nod_lss:
	case nod_between:
		node->nod_flags |= nod_comparison;

	case nod_list:
	case nod_matches:
	case nod_sleuth:
	case nod_like:
	case nod_containing:
	case nod_starts:
	case nod_missing:
	case nod_and:
	case nod_or:
	case nod_not:
		for (ptr = node->nod_arg, end = ptr + node->nod_count; ptr < end; ptr++)
		{
			(*ptr)->nod_flags |= NOD_parameter2;
			*ptr = compile_expression(*ptr, request, internal_flag);
		}
		if (node->nod_type == nod_and || node->nod_type == nod_or)
			node->nod_flags |= nod_partial;
		return node;

	case nod_null:
	case nod_add:
	case nod_subtract:
	case nod_multiply:
	case nod_divide:
	case nod_negate:
	case nod_concatenate:
	case nod_substr:
	case nod_user_name:
		if (!internal_flag && request && request->req_receive && computable(node, request))
		{
			compile_expression(node, request, true);
			return make_reference(node, request->req_receive);
		}
		for (ptr = node->nod_arg, end = ptr + node->nod_count; ptr < end; ptr++)
		{
			(*ptr)->nod_flags |= NOD_parameter2;
			*ptr = compile_expression(*ptr, request, internal_flag);
		}
		make_descriptor(node, &node->nod_desc);
		if (!internal_flag)
		{
			node->nod_flags |= NOD_local;
			CMP_alloc_temp(node);
		}
		return node;

	case nod_format:
		value = node->nod_arg[e_fmt_value];
		node->nod_arg[e_fmt_value] = compile_expression(value, request, false);
		node->nod_desc.dsc_length = FMT_expression(node);
		node->nod_desc.dsc_dtype = dtype_text;
		CMP_alloc_temp(node);
		if (!internal_flag)
			node->nod_flags |= NOD_local;
		return node;

	case nod_constant:
		if (!internal_flag)
			node->nod_flags |= NOD_local;
		return node;

	case nod_edit_blob:
		compile_edit(node, request);
		return node;

	case nod_prompt:
		node->nod_count = 0;
		compile_prompt(node);
		if (internal_flag)
		{
			qli_par* parm = make_parameter(request->req_send, node);
			node->nod_export = parm;
			parm->par_value = node;
			if (field = (qli_fld*) node->nod_arg[e_prm_field])
			{
				parm->par_desc.dsc_dtype = field->fld_dtype;
				parm->par_desc.dsc_length = field->fld_length;
				parm->par_desc.dsc_scale = field->fld_scale;
				parm->par_desc.dsc_sub_type = field->fld_sub_type;
				if (parm->par_desc.dsc_dtype == dtype_text)
				{
					parm->par_desc.dsc_dtype = dtype_varying;
					parm->par_desc.dsc_length += sizeof(SSHORT);
				}
			}
			else
				parm->par_desc = node->nod_desc;
		}
		return node;

	case nod_field:
		if (value = node->nod_arg[e_fld_subs])
			compile_expression(value, request, true);
		return compile_field(node, request, internal_flag);

	case nod_variable:
		field = (qli_fld*) node->nod_arg[e_fld_field];
		node->nod_desc.dsc_address = field->fld_data;
		QLI_validate_desc(node->nod_desc);
		make_descriptor(node, &node->nod_desc);
		if (internal_flag)
		{
			qli_par* parm = make_parameter(request->req_send, node);
			node->nod_export = parm;
			parm->par_value = node;
			parm->par_desc = node->nod_desc;
		}
		return node;

	case nod_upcase:
	case nod_lowcase:
		value = node->nod_arg[0];
		node->nod_arg[0] = compile_field(value, request, internal_flag);
		return node;

	default:
		ERRQ_bugcheck(359);			// Msg359 compile_expression: not yet implemented
		return NULL;
	}
}


static qli_nod* compile_field( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Compile a field reference.
 *
 **************************************/

	// Pick up field characteristics

	node->nod_count = 0;
	qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
	qli_ctx* context = (qli_ctx*) node->nod_arg[e_fld_context];
	if ((field->fld_flags & FLD_array) && !node->nod_arg[e_fld_subs])
	{
		node->nod_desc.dsc_dtype = dtype_quad;
		node->nod_desc.dsc_length = 8;
		node->nod_desc.dsc_scale = 0;
		node->nod_desc.dsc_sub_type = 0;
	}
	else
	{
		node->nod_desc.dsc_dtype = field->fld_dtype;
		node->nod_desc.dsc_length = field->fld_length;
		node->nod_desc.dsc_scale = field->fld_scale;
		node->nod_desc.dsc_sub_type = field->fld_sub_type;
	}
	node->nod_flags |= NOD_parameter2;

	// If the reference is external, or the value is computable in the
	// current request, there is nothing to do.  If the value is not computable,
	// make up a parameter to send the value into the request.

	if (internal_flag)
	{
		if (computable(node, request))
			return node;

		qli_par* parm = make_parameter(request->req_send, node);
		node->nod_export = parm;
		parm->par_desc = node->nod_desc;
		parm->par_value = node;
		qli_msg* message = context->ctx_message;
		if (!message)
			message = request->req_receive;
		node->nod_arg[e_fld_reference] = make_reference(node, message);
		return node;
	}

	qli_msg* message = context->ctx_message;
	if (!message && request)
		message = request->req_receive;

	node->nod_arg[e_fld_reference] = make_reference(node, message);
	return node;
}


static qli_nod* compile_for( qli_nod* node, qli_req* old_request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ f o r
 *
 **************************************
 *
 * Functional description
 *	Compile a FOR loop.  If the loop can be done as part of the parent
 *	request, dandy.
 *
 **************************************/

	// Compile rse.  This will set up both send and receive message.  If the
	// messages aren't needed, we can release them later.

	qli_msg* old_send = NULL;
	qli_msg* old_receive = NULL;
	if (old_request)
	{
		old_send = old_request->req_send;
		old_receive = old_request->req_receive;
	}

	qli_msg* send;
	qli_msg* receive;
	qli_req* request = compile_rse(node->nod_arg[e_for_rse], old_request, false, &send, &receive, 0);
	if (request)
		node->nod_arg[e_for_request] = (qli_nod*) request;
	else
		request = old_request;

	// If nothing is required for sub-statement, and no data is required in
	// either direction, we don't need  to execute the statement.

	if (!compile_statement(node->nod_arg[e_for_statement], request, internal_flag) &&
		 !receive->msg_parameters)
	{
		release_message(receive);
		receive = NULL;
	}

	if (old_request)
	{
		old_request->req_send = old_send;
		old_request->req_receive = old_receive;
	}

	if ((send != old_send) && !send->msg_parameters)
	{
		release_message(send);
		send = NULL;
		if (!receive)
			return NULL;
	}

	if (receive)
	{
		qli_par* parameter = make_parameter(receive, 0);
		node->nod_arg[e_for_eof] = (qli_nod*) parameter;
		parameter->par_desc.dsc_dtype = dtype_short;
		parameter->par_desc.dsc_length = sizeof(SSHORT);
	}

	node->nod_arg[e_for_send] = (qli_nod*) send;
	node->nod_arg[e_for_receive] = (qli_nod*) receive;

	return node;
}


static qli_nod* compile_function( qli_nod* node, qli_req* old_request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Compile a database function reference.
 *
 **************************************/
	qli_msg* old_send = NULL;
	qli_msg* old_receive = NULL;

	qli_fun* function = (qli_fun*) node->nod_arg[e_fun_function];
	node->nod_count = 0;
	qli_msg* send = NULL;

	if (!internal_flag)
		old_request = NULL;

	if (old_request)
	{
		old_send = old_request->req_send;
		old_receive = old_request->req_receive;
	}

	qli_req* request;
	qli_msg* receive = 0;
	if (!old_request || old_request->req_database != function->fun_database)
	{
		request = make_request(function->fun_database);
		node->nod_arg[e_fun_request] = (qli_nod*) request;
		request->req_send = send = make_message(request);
		request->req_receive = receive = make_message(request);
	}
	else
		request = old_request;

	// If there is a value, compile it here

	qli_par* parameter = 0;
	if (!internal_flag)
	{
		node->nod_import = parameter = make_parameter(request->req_receive, 0);
		make_descriptor(node, &parameter->par_desc);
	}

	qli_nod* list = node->nod_arg[e_fun_args];

	qli_nod** ptr = list->nod_arg;
	for (const qli_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		compile_expression(*ptr, request, true);

	if (old_request)
	{
		old_request->req_send = old_send;
		old_request->req_receive = old_receive;
	}

	if (!internal_flag && request == old_request && computable(node, request))
	{
		make_descriptor(node, &node->nod_desc);
		return make_reference(node, request->req_receive);
	}

	if (send && (send != old_send) && !send->msg_parameters)
	{
		release_message(send);
		send = NULL;
	}

	node->nod_arg[e_fun_receive] = (qli_nod*) receive;
	node->nod_arg[e_fun_send] = (qli_nod*) send;

	return node;
}


static qli_nod* compile_if( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ i f
 *
 **************************************
 *
 * Functional description
 *	Compile an IF statement.  Determine whether the
 *	statement is going to be executed in QLI or database
 *	context.  Try to make sure that the whole thing is
 *	executed one place or the other.
 *
 **************************************/

	// If the statement can't be executed in database context,
	// make sure it gets executed locally

	if (!internal_flag || !computable(node, request))
	{
		internal_flag = false;
		node->nod_flags |= NOD_local;
		request = NULL;
	}

	qli_nod* sub = node->nod_arg[e_if_boolean];
	compile_expression(sub, request, internal_flag);
	compile_statement(node->nod_arg[e_if_true], request, internal_flag);

	if (sub = node->nod_arg[e_if_false])
		compile_statement(sub, request, internal_flag);

	if (internal_flag)
		return NULL;

	return node;
}


static qli_nod* compile_list_fields( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	c o m p i l e _ l i s t _ f i e l d s
 *
 **************************************
 *
 * Functional description
 *	Compile a print node.
 *
 **************************************/
	qli_nod* list = node->nod_arg[e_prt_list];
	compile_print_list(list, request, 0);
	node->nod_arg[e_prt_list] = FMT_list(list);
	node->nod_type = nod_print;

	return node;
}


static qli_nod* compile_modify( qli_nod* node, qli_req* org_request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Compile a modify statement.
 *
 **************************************/

	// If this is a different request from the current one, this will require an
	// optional action and a "continue" message

	qli_nod** ptr = node->nod_arg + e_mod_count;
	qli_ctx* context = (qli_ctx*) *ptr;
	qli_req* request = context->ctx_source->ctx_request;
	node->nod_arg[e_mod_request] = (qli_nod*) request;

	if ((request != org_request || !internal_flag) && !request->req_continue)
		request->req_continue = make_message(request);

	qli_msg* old_send = request->req_send;
	qli_msg* send = make_message(request);
	request->req_send = send;

	for (USHORT i = 0; i < node->nod_count; i++)
	{
		context = (qli_ctx*) * ptr++;
		context->ctx_request = request;
		context->ctx_context = request->req_context++;
		context->ctx_message = send;
	}

	// If nothing is required for sub-statement, and no data is required in
	// either direction, we don't need  to execute the statement.

	if (internal_flag)
		internal_flag = computable(node->nod_arg[e_mod_statement], request);

	if (!compile_statement(node->nod_arg[e_mod_statement], request, internal_flag) &&
		(send != old_send) && !send->msg_parameters)
	{
		node->nod_flags |= NOD_remote;
		release_message(send);
		send = NULL;
	}

	node->nod_arg[e_mod_send] = (qli_nod*) send;
	request->req_send = old_send;
	request->req_database->dbb_flags |= DBB_updates;

	if (internal_flag)
		return NULL;

	return node;
}


static qli_nod* compile_print( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	c o m p i l e _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Compile a print node.
 *
 **************************************/

	if (!print_header)
		print_header = (TEXT**) & node->nod_arg[e_prt_header];

	compile_print_list(node->nod_arg[e_prt_list], request, &print_items);

	return node;
}


static qli_nod* compile_print_list( qli_nod* list, qli_req* request, qli_lls** stack)
{
/**************************************
 *
 *	c o m p i l e _ p r i n t _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Compile a print node.
 *
 **************************************/
	qli_nod** ptr = list->nod_arg;
	for (const qli_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		qli_print_item* item = (qli_print_item*) *ptr;
		if (stack)
			ALLQ_push((blk*) item, stack);
		if (item->itm_type == item_value)
		{
			qli_nod* value = item->itm_value;
			value->nod_flags |= NOD_parameter2;
			item->itm_value = compile_expression(value, request, false);
			if (item->itm_value->nod_type == nod_field)
				item->itm_value = item->itm_value->nod_arg[e_fld_reference];
			if (!value->nod_desc.dsc_dtype)
				make_descriptor(value, &value->nod_desc);
		}
	}

	return list;
}


static qli_nod* compile_prompt( qli_nod* node)
{
/**************************************
 *
 *	c o m p i l e _ p r o m p t
 *
 **************************************
 *
 * Functional description
 *	Set up a prompt expression for execution.
 *
 **************************************/
	USHORT prompt_length;

	// Make up a plausible prompt length

	qli_fld* field = (qli_fld*) node->nod_arg[e_prm_field];
	if (!field)
		prompt_length = PROMPT_LENGTH;
	else
	{
		switch (field->fld_dtype)
		{
		case dtype_text:
			prompt_length = field->fld_length;
			break;

		case dtype_varying:
			prompt_length = field->fld_length - sizeof(SSHORT);
			break;

		case dtype_short:
			prompt_length = 8;
			break;


		case dtype_long:
		case dtype_real:
			prompt_length = 15;
			break;

		default:
			prompt_length = 30;
			break;
		}
	}

	// Allocate string buffer to hold data, a two byte count,
	// a possible carriage return, and a null

	fb_assert(prompt_length <= MAX_USHORT - 2 - sizeof(SSHORT));
	prompt_length += 2 + sizeof(SSHORT);
	qli_str* string = (qli_str*) ALLOCDV(type_str, prompt_length);
	node->nod_arg[e_prm_string] = (qli_nod*) string;
	node->nod_desc.dsc_dtype = dtype_varying;
	node->nod_desc.dsc_length = prompt_length;
	node->nod_desc.dsc_address = (UCHAR*) string->str_data;
	QLI_validate_desc(node->nod_desc);

	return node;
}


static qli_nod* compile_repeat( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ r e p e a t
 *
 **************************************
 *
 * Functional description
 *	Compile a REPEAT statement.
 *
 **************************************/

	compile_expression(node->nod_arg[e_rpt_value], request, false);
	compile_statement(node->nod_arg[e_rpt_statement], 0, internal_flag);

	return node;
}


static qli_nod* compile_report( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	c o m p i l e _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Compile the body of a report specification.
 *
 **************************************/
	qli_brk* control;

	qli_rpt* report = (qli_rpt*) node->nod_arg[e_prt_list];

	if (control = report->rpt_top_rpt)
		compile_control_break(control, request);

	if (control = report->rpt_top_page)
		compile_control_break(control, request);

	if (control = report->rpt_top_breaks)
		compile_control_break(control, request);

	qli_nod* list = report->rpt_detail_line;
	if (list)
		compile_print_list(list, request, 0);

	if (control = report->rpt_bottom_breaks)
	{
		compile_control_break(control, request);
		report->rpt_bottom_breaks = NULL;
		while (control)
		{
			qli_brk* temp = control;
			control = control->brk_next;
			temp->brk_next = report->rpt_bottom_breaks;
			report->rpt_bottom_breaks = temp;
		}
	}

	if (control = report->rpt_bottom_page)
		compile_control_break(control, request);

	if (control = report->rpt_bottom_rpt)
		compile_control_break(control, request);

	FMT_report(report);

	return node;
}


static qli_req* compile_rse(qli_nod* node, qli_req* old_request, bool internal_flag,
							qli_msg** send, qli_msg** receive, qli_dbb** database)
{
/**************************************
 *
 *	c o m p i l e _ r s e
 *
 **************************************
 *
 * Functional description
 *	Compile a record selection expression.  If it can't be processed
 *	as part of an existing parent request, make up a new request.  If
 *	data must be sent to start the loop, generate a send message.  Set
 *	up for a receive message as well.
 *
 **************************************/
	qli_req* request;
	qli_dbb* local_dbb;

	qli_req* const original_request = old_request;

	if (!database)
	{
		local_dbb = NULL;
		database = &local_dbb;
	}

	// Loop thru relations to make sure only a single database is presented

	qli_ctx** ctx_ptr = (qli_ctx**) node->nod_arg + e_rse_count;
	const qli_ctx* const* const ctx_end = ctx_ptr + node->nod_count;

	for (; ctx_ptr < ctx_end; ctx_ptr++)
	{
		qli_ctx* context = *ctx_ptr;
		if (context->ctx_stream)
		{
			if (request = compile_rse(context->ctx_stream, old_request, internal_flag,
										send, receive, database))
			{
				old_request = request;
			}
		}
		else
		{
			qli_rel* relation = context->ctx_relation;
			if (!*database)
				*database = relation->rel_database;
			else if (*database != relation->rel_database)
				IBERROR(357);	// Msg357 relations from multiple databases in single rse
		}
	}

	if (!old_request || old_request->req_database != *database)
		request = make_request(*database);
	else
		request = old_request;

	if (send)
	{
		if (old_request && request == old_request && !(old_request->req_flags & REQ_rse_compiled))
			*send = request->req_send;
		else
			request->req_send = *send = make_message(request);
		request->req_receive = *receive = make_message(request);
	}

	compile_context(node, request, internal_flag);

	// Process various clauses

	if (node->nod_arg[e_rse_first])
		compile_expression(node->nod_arg[e_rse_first], request, true);

	if (node->nod_arg[e_rse_boolean])
		compile_expression(node->nod_arg[e_rse_boolean], request, true);

	qli_nod** ptr;
	const qli_nod* const* end;
	qli_nod* list;
	if (list = node->nod_arg[e_rse_sort])
		for (ptr = list->nod_arg, end = ptr + list->nod_count * 2; ptr < end; ptr += 2)
		{
			compile_expression(*ptr, request, true);
		}

	if (list = node->nod_arg[e_rse_reduced])
		for (ptr = list->nod_arg, end = ptr + list->nod_count * 2; ptr < end; ptr += 2)
		{
			compile_expression(*ptr, request, true);
		}

	if (list = node->nod_arg[e_rse_group_by])
		for (ptr = list->nod_arg, end = ptr + list->nod_count; ptr < end; ptr++)
		{
			compile_expression(*ptr, request, true);
		}

	if (node->nod_arg[e_rse_having])
		compile_expression(node->nod_arg[e_rse_having], request, true);

	// If we didn't allocate a new request block, say so by returning NULL

	if (request == original_request)
		return NULL;

	request->req_flags |= REQ_rse_compiled;

	return request;
}


static qli_nod* compile_statement( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Compile a statement.  Actually, just dispatch, passing along
 *	the parent request.
 *
 **************************************/
	switch (node->nod_type)
	{
	case nod_assign:
		return compile_assignment(node, request, internal_flag);

	case nod_erase:
		return compile_erase(node, request);

	case nod_commit_retaining:
		return node;

	case nod_for:
	case nod_report_loop:
		return compile_for(node, request, internal_flag);

	case nod_list:
		{
			qli_nod* result = NULL;
			qli_nod** ptr = node->nod_arg;
			for (const qli_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
			{
				if (compile_statement(*ptr, request, internal_flag))
					result = node;
			}
			return result;
		}

	case nod_modify:
		return compile_modify(node, request, internal_flag);

	case nod_output:
		compile_expression(node->nod_arg[e_out_file], request, false);
		compile_statement(node->nod_arg[e_out_statement], request, false);
		return node;

	case nod_print:
		return compile_print(node, request);

	case nod_list_fields:
		return compile_list_fields(node, request);

	case nod_repeat:
		return compile_repeat(node, request, internal_flag);

	case nod_report:
		return compile_report(node, request);

	case nod_store:
		return compile_store(node, request, internal_flag);

	case nod_if:
		return compile_if(node, request, internal_flag);

	case nod_abort:
		if (node->nod_count)
			compile_expression(node->nod_arg[0], 0, false);
		return node;

	default:
		ERRQ_bugcheck(360);			// Msg360 not yet implemented (compile_statement)
		return NULL;
	}
}


static qli_nod* compile_statistical( qli_nod* node, qli_req* old_request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ s t a t i s t i c a l
 *
 **************************************
 *
 * Functional description
 *	Compile a statistical expression.  The expression may or may not
 *	request a separate request.
 *
 **************************************/
	qli_msg* old_send = NULL;
	qli_msg* old_receive = NULL;

	// If a default value is present, compile it outside the context of the rse.

	qli_nod* value = node->nod_arg[e_stt_default];
	if (value)
		compile_expression(value, old_request, true);

	// Compile rse.  This will set up both send and receive message.  If the
	// messages aren't needed, we can release them later.

	if (!internal_flag)
		old_request = NULL;

	if (old_request)
	{
		old_send = old_request->req_send;
		old_receive = old_request->req_receive;
	}

	qli_msg* send;
	qli_msg* receive;
	qli_req* request = compile_rse(node->nod_arg[e_stt_rse],
								   old_request, internal_flag, &send, &receive, 0);
	if (request) {
		node->nod_arg[e_stt_request] = (qli_nod*) request;
	}
	else
		request = old_request;

	// If there is a value, compile it here

	if (!internal_flag)
	{
		qli_par* parameter = make_parameter(request->req_receive, 0);
		node->nod_import = parameter;
		make_descriptor(node, &parameter->par_desc);
	}

	value = node->nod_arg[e_stt_value];
	if (value)
		compile_expression(value, request, true);

	if (old_request)
	{
		old_request->req_send = old_send;
		old_request->req_receive = old_receive;
	}

	if (!internal_flag && request == old_request && computable(node, request))
	{
		make_descriptor(node, &node->nod_desc);
		return make_reference(node, request->req_receive);
	}

	if ((send != old_send) && !send->msg_parameters)
	{
		release_message(send);
		send = NULL;
	}

	node->nod_arg[e_stt_receive] = (qli_nod*) receive;
	node->nod_arg[e_stt_send] = (qli_nod*) send;

	return node;
}


static qli_nod* compile_store( qli_nod* node, qli_req* request, bool internal_flag)
{
/**************************************
 *
 *	c o m p i l e _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Compile a STORE statement.
 *
 **************************************/
	// Find or make up request for statement

	qli_ctx* context = (qli_ctx*) node->nod_arg[e_sto_context];
	qli_rel* relation = context->ctx_relation;

	if (!request || request->req_database != relation->rel_database)
	{
		request = make_request(relation->rel_database);
		node->nod_arg[e_sto_request] = (qli_nod*) request;
	}

	request->req_database->dbb_flags |= DBB_updates;

	context->ctx_request = request;
	context->ctx_context = request->req_context++;
	qli_msg* send = make_message(request);
	context->ctx_message = request->req_send = send;

	// If nothing is required for sub-statement, and no data is required in
	// either direction, we don't need to execute the statement.

	if (internal_flag)
		internal_flag = computable(node->nod_arg[e_sto_statement], request);

	if (!compile_statement(node->nod_arg[e_sto_statement], request, internal_flag) &&
		!send->msg_parameters)
	{
		node->nod_flags |= NOD_remote;
		release_message(send);
		return NULL;
	}

	node->nod_arg[e_sto_send] = (qli_nod*) send;

	return node;
}


static bool computable( qli_nod* node, qli_req* request)
{
/**************************************
 *
 *	c o m p u t a b l e
 *
 **************************************
 *
 * Functional description
 *	Check to see if a value is computable within the context of a
 *	given request.
 *
 **************************************/
	qli_nod** ptr;
	const qli_nod* const* end;
	qli_nod* sub;
	qli_ctx* context;
	qli_map* map;

	switch (node->nod_type)
	{
	case nod_max:
	case nod_min:
	case nod_count:
	case nod_average:
	case nod_total:
	case nod_from:
		if ((sub = node->nod_arg[e_stt_rse]) && !computable(sub, request))
			return false;
		if ((sub = node->nod_arg[e_stt_value]) && !computable(sub, request))
			return false;
		if ((sub = node->nod_arg[e_stt_default]) && !computable(sub, request))
			return false;
		return true;

	case nod_rse:
		if (!request)
			return false;
		if ((sub = node->nod_arg[e_rse_first]) && !computable(sub, request))
			return false;
		for (ptr = node->nod_arg + e_rse_count, end = ptr + node->nod_count; ptr < end; ptr++)
		{
			context = (qli_ctx*) * ptr;
			if (context->ctx_stream)
			{
				if (!computable(context->ctx_stream, request))
					return false;
			}
			else if (context->ctx_relation->rel_database != request->req_database)
				return false;

			context->ctx_request = request;
		}
		if ((sub = node->nod_arg[e_rse_boolean]) && !computable(sub, request))
			return false;
		return true;

	case nod_field:
		if (sub = node->nod_arg[e_fld_subs])
			for (ptr = sub->nod_arg, end = ptr + sub->nod_count; ptr < end;  ptr++)
			{
				if (*ptr && !computable(*ptr, request))
					return false;
			}
		context = (qli_ctx*) node->nod_arg[e_fld_context];
		return (request == context->ctx_request);

	case nod_map:
		map = (qli_map*) node->nod_arg[e_map_map];
		return computable(map->map_node, request);

	case nod_print:
	case nod_report:
	case nod_abort:
	case nod_repeat:
	case nod_commit_retaining:

	case nod_rpt_average:
	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:
	case nod_rpt_count:
	case nod_running_total:
	case nod_running_count:
	case nod_edit_blob:
	case nod_prompt:
	case nod_variable:
	case nod_format:
		return false;

	case nod_null:
	case nod_constant:
	case nod_user_name:
		return true;

	case nod_for:
		if ((qli_req*) node->nod_arg[e_for_request] != request)
			return false;
		if (!computable(node->nod_arg[e_for_rse], request) ||
			!computable(node->nod_arg[e_for_statement], request))
		{
			return false;
		}
		return true;

	case nod_store:
		if ((qli_req*) node->nod_arg[e_sto_request] != request)
			return false;
		return computable(node->nod_arg[e_sto_statement], request);

	case nod_modify:
		context = (qli_ctx*) node->nod_arg[e_mod_count];
		if (context->ctx_source->ctx_request != request)
			return false;
		return computable(node->nod_arg[e_mod_statement], request);

	case nod_erase:
		context = (qli_ctx*) node->nod_arg[e_era_context];
		if (context->ctx_source->ctx_request != request)
			return false;
		return true;

	case nod_unique:
	case nod_any:
		if (node->nod_arg[e_any_request] != (qli_nod*) request)
			return false;
		return (computable(node->nod_arg[e_any_rse], request));

	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_count:
	case nod_agg_average:
	case nod_agg_total:
		if (sub = node->nod_arg[e_stt_value])
			return (computable(sub, request));

	case nod_assign:
		if (node->nod_arg[e_asn_valid])
		{
			sub = node->nod_arg[e_asn_from];
			// Try to do validation in QLI as soon as the user responds to the prompt
			if (sub->nod_type == nod_prompt)
				return false;
		}
	case nod_list:
	case nod_if:

	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_leq:
	case nod_lss:
	case nod_between:
	case nod_matches:
	case nod_sleuth:
	case nod_like:
	case nod_containing:
	case nod_starts:
	case nod_missing:
	case nod_and:
	case nod_or:
	case nod_not:
	case nod_add:
	case nod_subtract:
	case nod_multiply:
	case nod_divide:
	case nod_negate:
	case nod_concatenate:
	case nod_function:
	case nod_substr:
		for (ptr = node->nod_arg, end = ptr + node->nod_count; ptr < end; ptr++)
		{
			if (*ptr && !computable(*ptr, request))
				return false;
		}
		return true;

	default:
		ERRQ_bugcheck(361);			// Msg361 computable: not yet implemented
		return false;
	}
}


static void make_descriptor( qli_nod* node, dsc* desc)
{
/**************************************
 *
 *	m a k e _ d e s c r i p t o r
 *
 **************************************
 *
 * Functional description
 *	Fill out a descriptor based on an expression.
 *
 **************************************/
	USHORT dtype;

	dsc desc1;
	desc1.dsc_dtype = 0;
	desc1.dsc_scale = 0;
	desc1.dsc_length = 0;
	desc1.dsc_sub_type = 0;
	desc1.dsc_address = NULL;
	desc1.dsc_flags = 0;
	dsc desc2 = desc1;

	switch (node->nod_type)
	{
	case nod_field:
	case nod_variable:
		{
			const qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
			desc->dsc_dtype = field->fld_dtype;
			desc->dsc_length = field->fld_length;
			desc->dsc_scale = field->fld_scale;
			desc->dsc_sub_type = field->fld_sub_type;
		}
		return;

	case nod_reference:
		{
			qli_par* parameter = node->nod_import;
			*desc = parameter->par_desc;
		}
		return;

	case nod_map:
		{
			qli_map* map = (qli_map*) node->nod_arg[e_map_map];
			make_descriptor(map->map_node, desc);
		}
		return;

	case nod_function:
		{
			qli_fun* function = (qli_fun*) node->nod_arg[e_fun_function];
			*desc = function->fun_return;
		}
		return;

	case nod_constant:
	case nod_prompt:
	case nod_format:
		*desc = node->nod_desc;
		return;

	case nod_concatenate:
		{
			make_descriptor(node->nod_arg[0], &desc1);
			make_descriptor(node->nod_arg[1], &desc2);
			desc->dsc_scale = 0;
			desc->dsc_dtype = dtype_varying;
			ULONG len = sizeof(USHORT) + string_length(&desc1) + string_length(&desc2);
			if (len > MAX_USHORT) // Silent truncation for now.
				len = MAX_USHORT;
			desc->dsc_length = static_cast<USHORT>(len);
			if (desc1.dsc_dtype <= dtype_any_text)
				desc->dsc_sub_type = desc1.dsc_sub_type;
			else
				desc->dsc_sub_type = ttype_ascii;
		}
		return;

	case nod_add:
	case nod_subtract:
		make_descriptor(node->nod_arg[0], &desc1);
		make_descriptor(node->nod_arg[1], &desc2);
		if ((desc1.dsc_dtype == dtype_text && desc1.dsc_length >= 9) ||
			(desc2.dsc_dtype == dtype_text && desc2.dsc_length >= 9))
		{
			dtype = dtype_double;
		}
		else
			dtype = MAX(desc1.dsc_dtype, desc2.dsc_dtype);
		switch (dtype)
		{
		case dtype_sql_time:
		case dtype_sql_date:
		case dtype_timestamp:
			node->nod_flags |= nod_date;
			if (node->nod_type == nod_add)
			{
				desc->dsc_dtype = dtype;
				desc->dsc_length = (dtype == dtype_timestamp) ? 8 : 4;
				break;
			}

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
		case dtype_double:
		case dtype_real:
		case dtype_long:
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
			break;

		default:
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof(SLONG);
			desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
			break;
		}
		return;

	case nod_multiply:
		make_descriptor(node->nod_arg[0], &desc1);
		make_descriptor(node->nod_arg[1], &desc2);
		if ((desc1.dsc_dtype == dtype_text && desc1.dsc_length >= 9) ||
			(desc2.dsc_dtype == dtype_text && desc2.dsc_length >= 9))
		{
			dtype = dtype_double;
		}
		else
			dtype = MAX(desc1.dsc_dtype, desc2.dsc_dtype);
		switch (dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
		case dtype_real:
		case dtype_double:
		case dtype_long:
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
			break;

		default:
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof(SLONG);
			desc->dsc_scale = desc1.dsc_scale + desc2.dsc_scale;
			break;
		}
		return;

	case nod_agg_average:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:

	case nod_average:
	case nod_max:
	case nod_min:
	case nod_total:
	case nod_from:

	case nod_rpt_average:
	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:
	case nod_running_total:
		make_descriptor(node->nod_arg[e_stt_value], desc);
		if (desc->dsc_dtype == dtype_short)
		{
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof(SLONG);
		}
		else if (desc->dsc_dtype == dtype_real)
		{
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		return;

	case nod_null:
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_missing = DSC_missing;
		return;

	case nod_count:
	case nod_agg_count:
	case nod_running_count:
	case nod_rpt_count:
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		return;

	case nod_divide:
		desc->dsc_dtype = dtype_double;
		desc->dsc_length = sizeof(double);
		return;

	case nod_negate:
		make_descriptor(node->nod_arg[0], desc);
		return;

	case nod_user_name:
		desc->dsc_dtype = dtype_varying;
		desc->dsc_scale = 0;
		desc->dsc_length = sizeof(USHORT) + 16;
		return;

	case nod_substr:
	default:
		ERRQ_bugcheck(362);			// Msg362 make_descriptor: not yet implemented
	}
}


static qli_msg* make_message( qli_req* request)
{
/**************************************
 *
 *	m a k e _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Allocate a message block for a request.
 *
 **************************************/
	qli_msg* message = (qli_msg*) ALLOCDV(type_msg, 0);
	message->msg_request = request;
	message->msg_next = request->req_messages;
	request->req_messages = message;
	message->msg_number = request->req_msg_number++;

	return message;
}


static void make_missing_reference( qli_par* parameter)
{
/**************************************
 *
 *	m a k e _ m i s s i n g _ r e f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	Make up a parameter to pass a missing value.
 *
 **************************************/
	if (parameter->par_missing)
		return;

	qli_par* missing = (qli_par*) ALLOCD(type_par);
	parameter->par_missing = missing;
	missing->par_message = parameter->par_message;
	missing->par_desc.dsc_dtype = dtype_short;
	missing->par_desc.dsc_length = sizeof(SSHORT);
}


static qli_par* make_parameter( qli_msg* message, qli_nod* node)
{
/**************************************
 *
 *	m a k e _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Make a parameter block and hang it off a message block.
 *	To make prompts come out in the right order, insert the
 *	new prompt at the end of the prompt list.  Sigh.
 *
 **************************************/
	qli_par** ptr = &message->msg_parameters;

	while (*ptr)
		ptr = &(*ptr)->par_next;

	qli_par* parm = (qli_par*) ALLOCD(type_par);
	*ptr = parm;
	parm->par_message = message;
	if (node && (node->nod_flags & NOD_parameter2))
		make_missing_reference(parm);

	return parm;
}


static qli_nod* make_reference( qli_nod* node, qli_msg* message)
{
/**************************************
 *
 *	m a k e _ r e f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	Make a reference to a value to be computed in the
 *	database context.  Since a field can be referenced
 *	several times, construct reference blocks linking
 *	the single field to the single parameter.  (I think.)
 *	In any event, if a parameter for a field exists,
 *	use it rather than generating an new one.   Make it
 *	parameter2 style if necessary.
 *
 **************************************/
	if (!message)
		ERRQ_bugcheck(363);			// Msg363 missing message

	qli_par* parm;

	// Look for an existing field reference

	for (parm = message->msg_parameters; parm; parm = parm->par_next)
	{
		if (CMP_node_match(parm->par_value, node))
			break;
	}

	// Parameter doesn't exist -- make a new one.

	if (!parm)
	{
		parm = make_parameter(message, node);
		parm->par_value = node;
		parm->par_desc = node->nod_desc;
	}

	qli_nod* reference = (qli_nod*) ALLOCDV(type_nod, 1);
	reference->nod_type = nod_reference;
	reference->nod_arg[0] = node;
	reference->nod_desc = parm->par_desc;
	reference->nod_import = parm;

	return reference;
}


static qli_req* make_request( qli_dbb* database)
{
/**************************************
 *
 *	m a k e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Make a request block for a database.
 *
 **************************************/
	qli_req* request = (qli_req*) ALLOCD(type_req);
	request->req_database = database;
	request->req_next = QLI_requests;
	QLI_requests = request;
	database->dbb_flags |= DBB_active;
	if (!(database->dbb_transaction))
		MET_transaction(nod_start_trans, database);

	return request;
}


static void release_message( qli_msg* message)
{
/**************************************
 *
 *	r e l e a s e _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	A message block is unneeded, so release it.
 *
 **************************************/
	qli_req* request = message->msg_request;

	qli_msg** ptr;
	for (ptr = &request->req_messages; *ptr; ptr = &(*ptr)->msg_next)
	{
		if (*ptr == message)
			break;
	}

	if (!*ptr)
		ERRQ_bugcheck(364);			// Msg 364 lost message

	*ptr = message->msg_next;
	ALLQ_release((qli_frb*) message);
}


static int string_length(const dsc* desc)
{
/**************************************
 *
 *	s t r i n g _ l e n g t h
 *
 **************************************
 *
 * Functional description
 *	Estimate length of string based on descriptor.
 *
 **************************************/

	return DSC_string_length(desc);
}
