//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		cmp.cpp
//		DESCRIPTION:	Request compiler
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings
//
//
//____________________________________________________________
//
//

#include "firebird.h"
#include <stdlib.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
#include "../jrd/align.h"
#include "../gpre/cmd_proto.h"
#include "../gpre/cme_proto.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "../gpre/par_proto.h"



static void cmp_any(gpre_req*);
static void cmp_assignment(gpre_nod*, gpre_req*);
static void cmp_blob(blb*, bool);
static void cmp_blr(gpre_req*);
static void cmp_erase(act*, gpre_req*);
static void cmp_fetch(act*);
static void cmp_field(gpre_req*, const gpre_fld*, const ref*);
static void cmp_for(gpre_req*);
static void cmp_loop(gpre_req*);
static void cmp_modify(act*, gpre_req*);
static void cmp_port(gpre_port*, gpre_req*);
static void cmp_procedure(gpre_req*);
static void cmp_ready(gpre_req*);
static void cmp_sdl_fudge(gpre_req*, SLONG);
static bool cmp_sdl_loop(gpre_req*, USHORT, const slc*, const ary*);
static void cmp_sdl_number(gpre_req*, SLONG);
static void cmp_sdl_subscript(gpre_req*, USHORT, const slc*);
static void cmp_sdl_value(gpre_req*, const gpre_nod*);
static void cmp_set_generator(gpre_req*);
static void cmp_slice(gpre_req*);
static void cmp_store(gpre_req*);
static void expand_references(ref*);
static gpre_port* make_port(gpre_req*, ref*);
static void make_receive(gpre_port*, gpre_req*);
static void make_send(gpre_port*, gpre_req*);
static void update_references(ref*);

static gpre_nod* lit0;
static gpre_nod* lit1;
static gpre_fld* eof_field;
static gpre_fld* count_field;
static gpre_fld* slack_byte_field;
static ULONG next_ident;

//#define STUFF(blr)	*request->req_blr++ = (UCHAR)(blr)
//#define STUFF_WORD(blr) {STUFF (blr); STUFF ((blr) >> 8);}
//#define STUFF_LONG(blr) {STUFF (blr); STUFF ((blr) >> 8); STUFF ((blr) >>16); STUFF ((blr) >> 24);}
//#define STUFF_INT(blr)	STUFF (blr); STUFF ((blr) >> 8); STUFF ((blr) >> 16); STUFF ((blr) >> 24)

const int MAX_TPB = 4000;

//____________________________________________________________
//
//		Check to make sure that generated blr string is not about to
//		over the memory allocated for it.  If so, allocate an extra
//		couple of hundred bytes to be safe.
//

void CMP_check( gpre_req* request, SSHORT min_reqd)
{
	const int length = request->req_blr - request->req_base;
	if (!min_reqd && (length < request->req_length - 100))
		return;

	const int n = ((length + min_reqd + 100) > request->req_length * 2) ?
		length + min_reqd + 100 : request->req_length * 2;

	UCHAR* const old = request->req_base;
	UCHAR* p = MSC_alloc(n);
	request->req_base = p;
	request->req_length = n;
	request->req_blr = request->req_base + length;

	memcpy(p, old, length);

	MSC_free(old);
}


//____________________________________________________________
//
//		Compile a single request, but do not generate any text.
//		Generate port blocks, assign parameter numbers, message
//		numbers, and internal idents.  Compute length of request.
//

void CMP_compile_request( gpre_req* request)
{
	// if there isn't a request handle specified, make one!

	if (!request->req_handle && (request->req_type != REQ_procedure))
	{
		request->req_handle = (TEXT*) MSC_alloc(20);
		sprintf(request->req_handle, gpreGlob.ident_pattern, CMP_next_ident());
	}

	if (!request->req_trans)
		request->req_trans = gpreGlob.transaction_name;

	if (!request->req_request_level)
		request->req_request_level = "0";

	request->req_ident = CMP_next_ident();

	// If this is an SQL blob cursor, compile the blob and get out fast.

	if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
	{
		for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
			cmp_blob(blob, true);
		return;
	}

	// Before we get too far, make sure an eof field has been
	// constructed.  If not, do so now.

	ref* reference;
	if (!eof_field)
	{
		eof_field = MET_make_field(gpreGlob.utility_name, dtype_short, sizeof(SSHORT), false);
		count_field = MET_make_field(gpreGlob.count_name, dtype_long, sizeof(SLONG), false);
		slack_byte_field = MET_make_field(gpreGlob.slack_name, dtype_text, 1, false);
		reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_value = "0";
		lit0 = MSC_unary(nod_literal, (gpre_nod*) reference);

		reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_value = "1";
		lit1 = MSC_unary(nod_literal, (gpre_nod*) reference);
	}

	// Handle different request types differently

	switch (request->req_type)
	{
	case REQ_create_database:
	case REQ_ddl:
		CMD_compile_ddl(request);
		return;
	case REQ_slice:
		cmp_slice(request);
		return;

	case REQ_ready:
		cmp_ready(request);
		return;
	case REQ_procedure:
		cmp_procedure(request);
		return;
	}
	// expand any incomplete references or values

	expand_references(request->req_references);
	expand_references(request->req_values);

	// Initialize the blr string

	request->req_blr = request->req_base = MSC_alloc(500);
	request->req_length = 500;
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);

	// If there are values to be transmitted, make a port
	// to hold them

	if (request->req_values)
		request->req_vport = make_port(request, request->req_values);

	// If this is a FOR type request, an eof field reference needs
	// to be generated.  Do it.

	switch (request->req_type)
	{
	case REQ_for:
	case REQ_cursor:
	case REQ_any:
		reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_field = eof_field;
		reference->ref_next = request->req_references;
		break;
	case REQ_mass_update:
		reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_field = count_field;
		reference->ref_next = request->req_references;
		break;
	default:
		reference = request->req_references;
	}

	// Assume that a general port needs to be constructed.

	gpre_port* port;
	if ((request->req_type != REQ_insert) && (request->req_type != REQ_store2) &&
		(request->req_type != REQ_set_generator))
	{
		request->req_primary = port = make_port(request, reference);
	}

	// Loop thru actions looking for something interesting to do.  Not
	// all action types are "interesting", so don't worry about missing
	// ones.

	upd* update;
	for (act* action = request->req_actions; action; action = action->act_next)
	{
		switch (action->act_type)
		{
		case ACT_modify:
		case ACT_erase:
		case ACT_update:
			update = (upd*) action->act_object;
			expand_references(update->upd_references);
			update->upd_port = make_port(request, update->upd_references);
			if (!request->req_sync)
				request->req_sync = make_port(request, 0);
			break;
		case ACT_store:
			if (request->req_type == REQ_store2)
			{
				request->req_primary = make_port(request, action->act_object);
				update_references(request->req_primary->por_references);
			}
			break;
		case ACT_store2:
			update = (upd*) action->act_object;
			expand_references(update->upd_references);
			update->upd_port = make_port(request, update->upd_references);
			update_references(update->upd_port->por_references);
			break;
		case ACT_select:
		case ACT_fetch:
			cmp_fetch(action);
			break;
		}
	}

	cmp_blr(request);
	request->add_byte(blr_eoc);

	// Compute out final blr lengths

	request->req_length = request->req_blr - request->req_base;
	request->req_blr = request->req_base;

	// Finally, assign identifiers to any blobs that may have been referenced

	for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		cmp_blob(blob, false);
}


//____________________________________________________________
//
//		Stuff field datatype info into request.
//		Text fields are not remapped to process text type.
//		This is used by the CAST & COLLATE operators to
//		indicate cast datatypes.
//

void CMP_external_field( gpre_req* request, const gpre_fld* field)
{

	switch (field->fld_dtype)
	{
	case dtype_cstring:
		request->add_byte(blr_text2);
		request->add_word(field->fld_ttype);
		request->add_word(field->fld_length - 1);
		break;

	case dtype_text:
		request->add_byte(blr_text2);
		request->add_word(field->fld_ttype);
		request->add_word(field->fld_length);
		break;

	case dtype_varying:
		request->add_byte(blr_varying2);
		request->add_word(field->fld_ttype);
		request->add_word(field->fld_length);
		break;

	default:
		cmp_field(request, field, 0);
		break;
	}
}


//____________________________________________________________
//
//		Initialize (or re-initialize) for request compilation.  This is
//		called at most once per module.
//

void CMP_init()
{
	lit0 = lit1 = NULL;
	count_field = slack_byte_field = eof_field = NULL;
	next_ident = 0;
}


//____________________________________________________________
//
//		Give out next identifier.
//

ULONG CMP_next_ident()
{
	return next_ident++;
}


//____________________________________________________________
//
//		Stuff a symbol.
//

void CMP_stuff_symbol( gpre_req* request, const gpre_sym* symbol)
{
	request->add_byte(strlen(symbol->sym_string));

	for (const TEXT* p = symbol->sym_string; *p; p++)
		request->add_byte(*p);
}


//____________________________________________________________
//
//		Take a transaction block with (potentially) a
//		lot of relation lock blocks, and generate TPBs
//
//		We'll always generate TPB's, and link them
//		into the gpre_dbb for that database so they get
//		generated.  If there's no lock list, we generate
//		a simple TPB for every database in the program.
//		If there is a lock list, we generate a more complex
//		TPB for each database referenced.
//

void CMP_t_start( gpre_tra* trans)
{
	char rrl_buffer[MAX_TPB];
	char tpb_buffer[MAX_TRA_OPTIONS + 1];

	// fill out a standard tpb buffer ahead of time so we know
	// how large it is

	char* text = tpb_buffer;
	*text++ = isc_tpb_version1;
	*text++ = (trans->tra_flags & TRA_ro) ? isc_tpb_read : isc_tpb_write;
	if (trans->tra_flags & TRA_con)
		*text++ = isc_tpb_consistency;
	else if (trans->tra_flags & TRA_read_committed)
		*text++ = isc_tpb_read_committed;
	else
		*text++ = isc_tpb_concurrency;
	*text++ = (trans->tra_flags & TRA_nw) ? isc_tpb_nowait : isc_tpb_wait;

	if (trans->tra_flags & TRA_read_committed) {
		*text++ = (trans->tra_flags & TRA_rec_version) ? isc_tpb_rec_version : isc_tpb_no_rec_version;
	}

	if (trans->tra_flags & TRA_autocommit)
		*text++ = isc_tpb_autocommit;
	if (trans->tra_flags & TRA_no_auto_undo)
		*text++ = isc_tpb_no_auto_undo;
	*text = 0;
	const USHORT tpb_len = text - tpb_buffer;

	for (gpre_dbb* database = gpreGlob.isc_databases; database; database = database->dbb_next)
	{
		// figure out if this is a simple transaction or a reserving
		// transaction.  Allocate a TPB of the right size in either
		// case.

		tpb* new_tpb = 0;
		if (trans->tra_flags & TRA_inc)
		{
			if (database->dbb_flags & DBB_in_trans)
			{
				new_tpb = (tpb*) MSC_alloc(TPB_LEN(tpb_len));
				new_tpb->tpb_length = tpb_len;
				database->dbb_flags &= ~DBB_in_trans;
			}
			else
				continue;
		}
		else if (!(trans->tra_flags & TRA_rrl))
		{
			new_tpb = (tpb*) MSC_alloc(TPB_LEN(tpb_len));
			new_tpb->tpb_length = tpb_len;
		}
		else if (database->dbb_rrls)
		{
			char* p = rrl_buffer;
			for (const rrl* lock_block = database->dbb_rrls; lock_block;
				lock_block = lock_block->rrl_next)
			{
				*p++ = lock_block->rrl_lock_mode;
				const char* q = lock_block->rrl_relation->rel_symbol->sym_string;
				*p++ = strlen(q);
				while (*q)
					*p++ = *q++;
				*p++ = lock_block->rrl_lock_level;
			}
			*p = 0;
			const USHORT buff_len = (p - rrl_buffer);
			new_tpb = (tpb*) MSC_alloc(TPB_LEN(buff_len + tpb_len));
			new_tpb->tpb_length = buff_len + tpb_len;
			database->dbb_rrls = NULL;
		}
		else					// this database isn't referenced
			continue;

		// link this into the TPB chains (gpre_tra and gpre_dbb)

		new_tpb->tpb_database = database;
		new_tpb->tpb_dbb_next = database->dbb_tpbs;
		database->dbb_tpbs = new_tpb;
		new_tpb->tpb_tra_next = trans->tra_tpb;
		trans->tra_tpb = new_tpb;
		trans->tra_db_count++;

		// fill in the standard TPB and concatenate the relation names

		new_tpb->tpb_ident = CMP_next_ident();

		text = reinterpret_cast<char*>(new_tpb->tpb_string);
		for (const char* pb = tpb_buffer; *pb;)
			*text++ = *pb++;
		if (trans->tra_flags & TRA_rrl)
			strcpy(text, rrl_buffer);
	}
}


//____________________________________________________________
//
//		Generate blr tree for free standing ANY expression.
//

static void cmp_any( gpre_req* request)
{
	// Build the primary send statement

	gpre_port* port = request->req_primary;
	make_send(port, request);

	request->add_byte(blr_if);
	request->add_byte(blr_any);
	CME_rse(request->req_rse, request);

	gpre_nod* value = MSC_unary(nod_value, (gpre_nod*) port->por_references);

	// Make a send to signal end of file

	request->add_byte(blr_assignment);
	CME_expr(lit1, request);
	CME_expr(value, request);

	request->add_byte(blr_assignment);
	CME_expr(lit0, request);
	CME_expr(value, request);

	port = request->req_vport;
	if (port)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			ref* source = (ref*) MSC_alloc(REF_LEN);
			reference->ref_source = source;
			source->ref_ident = CMP_next_ident();
		}
	}
}


//____________________________________________________________
//
//		Compile a build assignment statement.
//

static void cmp_assignment( gpre_nod* node, gpre_req* request)
{
	CMP_check(request, 0);
	request->add_byte(blr_assignment);
	CME_expr(node->nod_arg[0], request);
	CME_expr(node->nod_arg[1], request);
}


//____________________________________________________________
//
//		Compile a blob parameter block, if required.
//

static void cmp_blob(blb* blob, bool sql_flag)
{
	blob->blb_ident = CMP_next_ident();
	blob->blb_buff_ident = CMP_next_ident();
	blob->blb_len_ident = CMP_next_ident();

	if (sql_flag)
	{
		for (ref* reference = blob->blb_reference; reference; reference = reference->ref_next)
		{
			reference->ref_port = make_port(reference->ref_context->ctx_request, reference);
		}
	}

	if (!blob->blb_const_to_type && !blob->blb_type)
		return;

	blob->blb_bpb_ident = CMP_next_ident();
	UCHAR* p = blob->blb_bpb;
	*p++ = isc_bpb_version1;

	if (blob->blb_const_to_type)
	{
		*p++ = isc_bpb_target_type;
		*p++ = 2;
		*p++ = (UCHAR) blob->blb_const_to_type;
		*p++ = blob->blb_const_to_type >> 8;
	}

	if (blob->blb_const_from_type)
	{
		*p++ = isc_bpb_source_type;
		*p++ = 2;
		*p++ = (UCHAR) blob->blb_const_from_type;
		*p++ = blob->blb_const_from_type >> 8;
	}

	if (blob->blb_type)
	{
		*p++ = isc_bpb_type;
		*p++ = 1;
		*p++ = (UCHAR) blob->blb_type;
	}

	if (blob->blb_from_charset)
	{
		// create bpb instruction for source character set

		*p++ = isc_bpb_source_interp;
		*p++ = 2;
		*p++ = (UCHAR) blob->blb_from_charset;
		*p++ = blob->blb_from_charset >> 8;
	}

	if (blob->blb_to_charset)
	{

		// create bpb instruction for target character set

		*p++ = isc_bpb_target_interp;
		*p++ = 2;
		*p++ = (UCHAR) blob->blb_to_charset;
		*p++ = blob->blb_to_charset >> 8;
	}

	blob->blb_bpb_length = p - blob->blb_bpb;
}


//____________________________________________________________
//
//		Build a request tree for a request.
//

static void cmp_blr( gpre_req* request)
{
	request->add_byte(blr_begin);

	// build message definition for each port

	gpre_port* port;
	for (port = request->req_ports; port; port = port->por_next)
		cmp_port(port, request);

	// See if there is a receive to be built

	if ((request->req_type == REQ_store) || (request->req_type == REQ_store2))
		port = request->req_primary;
	else
		port = request->req_vport;

	if (port)
		make_receive(port, request);

	// Compile up request

	switch (request->req_type)
	{
	case REQ_cursor:
	case REQ_for:
		cmp_for(request);
		break;

	case REQ_insert:
	case REQ_store:
	case REQ_store2:
		cmp_store(request);
		break;

	case REQ_mass_update:
		cmp_loop(request);
		break;

	case REQ_any:
		cmp_any(request);
		break;

	case REQ_set_generator:
		cmp_set_generator(request);
		break;

	default:
		CPR_error("*** unknown request type node ***");
	}

	request->add_byte(blr_end);
}


//____________________________________________________________
//
//		Generate blr for ERASE action.
//

static void cmp_erase( act* action, gpre_req* request)
{
	upd* update = (upd*) action->act_object;
	gpre_ctx* source = update->upd_source;
	make_receive(update->upd_port, request);

	if (action->act_error)
		request->add_byte(blr_handler);

	request->add_byte(blr_erase);
	request->add_byte(source->ctx_internal);
}


//____________________________________________________________
//
//		Go over an SQL fetch list and expand
//		references to indicator variables.
//		Not tough.
//

static void cmp_fetch( act* action)
{
	gpre_nod* list = (gpre_nod*) action->act_object;
	if (!list)
		return;

	if (list->nod_type != nod_list)
		CPR_error("invalid fetch action");

	gpre_nod** ptr = list->nod_arg;
	for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		ref* var = (ref*) *ptr;
		if (var->ref_null_value && !var->ref_null)
		{
			if (var->ref_friend)
				var->ref_null = var->ref_friend->ref_null;
		}
	}
}


//____________________________________________________________
//
//		Stuff field datatype info into request.
//

static void cmp_field( gpre_req* request, const gpre_fld* field,
	const ref* reference)
{
	if (reference && reference->ref_value && (reference->ref_flags & REF_array_elem))
	{
		field = field->fld_array;
	}

	fb_assert(field != NULL);
	fb_assert(field->fld_dtype);
	fb_assert(field->fld_length);

	switch (field->fld_dtype)
	{
	case dtype_cstring:
		if (!(field->fld_flags & FLD_charset) && field->fld_ttype)
		{
			request->add_byte(blr_cstring);
			request->add_word(field->fld_length);
		}
		else
		{
			// 3.2j has new, tagged blr instruction for cstring

			request->add_byte(blr_cstring2);
			request->add_word(field->fld_ttype);
			request->add_word(field->fld_length);
		}
		break;

	case dtype_text:
		if (!(field->fld_flags & FLD_charset) && field->fld_ttype)
		{
			request->add_byte(blr_text);
			request->add_word(field->fld_length);
		}
		else
		{
			// 3.2j has new, tagged blr instruction for text too

			request->add_byte(blr_text2);
			request->add_word(field->fld_ttype);
			request->add_word(field->fld_length);
		}
		break;

	case dtype_varying:
		if (!(field->fld_flags & FLD_charset) && field->fld_ttype)
		{
			request->add_byte(blr_varying);
			request->add_word(field->fld_length);
		}
		else
		{
			// 3.2j has new, tagged blr instruction for varying also

			request->add_byte(blr_varying2);
			request->add_word(field->fld_ttype);
			request->add_word(field->fld_length);
		}
		break;

	case dtype_short:
		request->add_byte(blr_short);
		request->add_byte(field->fld_scale);
		break;

	case dtype_long:
		request->add_byte(blr_long);
		request->add_byte(field->fld_scale);
		break;

	case dtype_blob:
	case dtype_quad:
		request->add_byte(blr_quad);
		request->add_byte(field->fld_scale);
		break;

	// Begin sql date/time/timestamp
	case dtype_sql_date:
		request->add_byte(blr_sql_date);
		break;

	case dtype_sql_time:
		request->add_byte(blr_sql_time);
		break;

	case dtype_timestamp:
		request->add_byte(blr_timestamp);
		break;
	// Begin sql date/time/timestamp

	case dtype_int64:
		request->add_byte(blr_int64);
		request->add_byte(field->fld_scale);
		break;

	case dtype_real:
		request->add_byte(blr_float);
		break;

	case dtype_double:
		if (gpreGlob.sw_d_float)
			request->add_byte(blr_d_float);
		else
			request->add_byte(blr_double);
		break;

	default:
		{
			TEXT s[50];
			sprintf(s, "datatype %d not understood", field->fld_dtype);
			CPR_error(s);
		}
	}
}


//____________________________________________________________
//
//		Generate blr tree for for statement
//

static void cmp_for( gpre_req* request)
{
	request->add_byte(blr_begin);
	if (request->req_type == REQ_cursor && request->req_sync)
		make_receive(request->req_sync, request);

	request->add_byte(blr_for);

	if (request->req_flags & REQ_sql_cursor)
	{
		if (!request->req_rse->rse_sort && !request->req_rse->rse_reduced)
			for (gpre_ctx* context = (gpre_ctx*) request->req_contexts; context;
				context = context->ctx_next)
			{
				if (context->ctx_scope_level == request->req_scope_level && context->ctx_procedure)
				{
					request->add_byte(blr_stall);
					break;
				}
			}
	}

	CME_rse(request->req_rse, request);

	// Loop thru actions looking for primary port.  While we're at it,
	// count the number of update actions.

	bool updates = false;

	for (act* action = request->req_actions; action; action = action->act_next)
	{
		switch (action->act_type)
		{
		case ACT_modify:
		case ACT_update:
		case ACT_erase:
			updates = true;
			break;
		}
	}

	if (updates)
		request->add_byte(blr_begin);

	// Build the primary send statement

	gpre_port* port = request->req_primary;
	make_send(port, request);

	gpre_nod* field = MSC_node(nod_field, 1);
	gpre_nod* value = MSC_node(nod_value, 1);
	request->add_byte(blr_begin);

	for (ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		CMP_check(request, 0);
		if (reference->ref_master)
			continue;
		request->add_byte(blr_assignment);
		if (reference->ref_field == eof_field)
		{
			request->req_eof = reference;
			CME_expr(lit1, request);
		}
		else if (reference->ref_field == slack_byte_field)
			CME_expr(lit0, request);
		else
		{
			if (reference->ref_expr)
				CME_expr(reference->ref_expr, request);
			else
			{
				field->nod_arg[0] = (gpre_nod*) reference;
				CME_expr(field, request);
			}
		}
		value->nod_arg[0] = (gpre_nod*) reference;
		CME_expr(value, request);
	}
	request->add_byte(blr_end);

	// If there are any actions, handle them here

	if (updates)
	{
		request->add_byte(blr_label);
		request->add_byte(0);
		request->add_byte(blr_loop);
		request->add_byte(blr_select);
		make_receive(request->req_sync, request);
		request->add_byte(blr_leave);
		request->add_byte(0);

		for (act* action = request->req_actions; action; action = action->act_next)
		{
			switch (action->act_type)
			{
			case ACT_endmodify:
			case ACT_update:
				cmp_modify(action, request);
				break;
			case ACT_erase:
				cmp_erase(action, request);
				break;
			}
		}
		request->add_byte(blr_end);
		request->add_byte(blr_end);
	}

	// Make a send to signal end of file

	make_send(port, request);
	request->add_byte(blr_assignment);
	CME_expr(lit0, request);
	value->nod_arg[0] = (gpre_nod*) request->req_eof;
	CME_expr(value, request);

	request->add_byte(blr_end);
}


//____________________________________________________________
//
//		Compile a mass looping update statement.
//

static void cmp_loop( gpre_req* request)
{
	gpre_nod* node = request->req_node;
	gpre_rse* selection = request->req_rse;

	gpre_port* primary = request->req_primary;
	make_send(primary, request);
	request->add_byte(blr_begin);
	gpre_nod* counter = MSC_node(nod_value, 1);
	for (ref* reference = primary->por_references; reference; reference = reference->ref_next)
	{
		if (reference->ref_field == count_field)
			counter->nod_arg[0] = (gpre_nod*) reference;
	}

	request->add_byte(blr_assignment);
	CME_expr(lit0, request);
	CME_expr(counter, request);

	gpre_ctx* for_context = selection->rse_context[0];
	gpre_ctx* update_context = request->req_update;

	request->add_byte(blr_for);
	CME_rse(selection, request);
	request->add_byte(blr_begin);

	switch (node->nod_type)
	{
	case nod_modify:
		{
			request->add_byte(blr_modify);
			request->add_byte(for_context->ctx_internal);
			request->add_byte(update_context->ctx_internal);
			gpre_nod* list = node->nod_arg[0];
			request->add_byte(blr_begin);
			gpre_nod** ptr = list->nod_arg;
			for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
			{
				cmp_assignment(*ptr, request);
			}
			request->add_byte(blr_end);
		}
		break;
	case nod_store:
		{
			update_context = (gpre_ctx*) node->nod_arg[0];
			request->add_byte(blr_store);
			CME_relation(update_context, request);
			gpre_nod* list = node->nod_arg[1];
			request->add_byte(blr_begin);
			gpre_nod** ptr = list->nod_arg;
			for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
			{
				cmp_assignment(*ptr, request);
			}
			request->add_byte(blr_end);
		}
		break;
	case nod_erase:
		request->add_byte(blr_erase);
		request->add_byte(for_context->ctx_internal);
		break;
	default:
	    fb_assert(false);
	}

	request->add_byte(blr_assignment);
	request->add_byte(blr_add);
	CME_expr(lit1, request);
	CME_expr(counter, request);
	CME_expr(counter, request);

	request->add_byte(blr_end);
	request->add_byte(blr_end);
}


//____________________________________________________________
//
//		Generate a receive and modify tree for a modify action.
//

static void cmp_modify( act* action, gpre_req* request)
{
	upd* update = (upd*) action->act_object;
	gpre_port* port = update->upd_port;
	make_receive(port, request);

	if (action->act_error)
		request->add_byte(blr_handler);

	request->add_byte(blr_modify);
	request->add_byte(update->upd_source->ctx_internal);
	request->add_byte(update->upd_update->ctx_internal);

	// count the references and build an assignment block

	gpre_nod* list = update->upd_assignments;
	request->add_byte(blr_begin);

	gpre_nod** ptr = list->nod_arg;
	for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		const gpre_nod* field_node = (*ptr)->nod_arg[1];
		const ref* reference = (ref*) field_node->nod_arg[0];
		if ((field_node->nod_type != nod_field) || !reference->ref_master)
			cmp_assignment(*ptr, request);
	}

	request->add_byte(blr_end);
}


//____________________________________________________________
//
//		Build a request tree for a request.
//

static void cmp_port( gpre_port* port, gpre_req* request)
{
	// build message definition for each port

	request->add_byte(blr_message);
	request->add_byte(port->por_msg_number);
	request->add_word(port->por_count);

	for (ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		CMP_check(request, 0);
		cmp_field(request, reference->ref_field, reference);
	}
}


//____________________________________________________________
//
//		Compile a EXECUTE PROCEDURE request.
//

static void cmp_procedure( gpre_req* request)
{
	act* action = request->req_actions;
	gpre_prc* procedure = (gpre_prc*) action->act_object;

	// Remember the order of the references.  The exec_proc blr verb
	// requires parameters to be in parameter order which may be changed
	// when there references are expanded.

	gpre_lls* outputs = NULL;
	ref* reference = request->req_references;
	if (reference)
	{
		for (gpre_lls** list = &outputs; reference; reference = reference->ref_next)
		{
			MSC_push((gpre_nod*) reference, list);
			list = &(*list)->lls_next;
		}
	}

	// Expand any incomplete references or values.

	expand_references(request->req_values);
	expand_references(request->req_references);

	request->req_blr = request->req_base = MSC_alloc(500);
	request->req_length = 500;
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);
	request->add_byte(blr_begin);

	if (request->req_values)
	{
		request->req_vport = make_port(request, request->req_values);
		request->req_values = request->req_vport->por_references;
	}
	else
		request->req_count++;
	request->req_primary = make_port(request, request->req_references);
	request->req_references = request->req_primary->por_references;

	// build message definition for each port

	for (gpre_port* port = request->req_ports; port; port = port->por_next)
		cmp_port(port, request);

	if (request->req_values)
	{
		request->add_byte(blr_begin);
		make_send(request->req_vport, request);
	}

	if (gpreGlob.sw_ids)
	{
		request->add_byte(blr_exec_pid);
		request->add_word(procedure->prc_id);
	}
	else
	{
		request->add_byte(blr_exec_proc);
		CMP_stuff_symbol(request, procedure->prc_symbol);
	}

	if (procedure->prc_in_count)
	{
		request->add_word(procedure->prc_in_count);
		gpre_nod** ptr = request->req_node->nod_arg;
		for (const gpre_nod* const* const end = ptr + procedure->prc_in_count; ptr < end; ptr++)
		{
			CME_expr(*ptr, request);
		}
	}
	else
		request->add_word(0);

	if (request->req_references)
	{
		gpre_nod* node = MSC_node(nod_value, 1);
		request->add_word(procedure->prc_out_count);
		for (; outputs; outputs = outputs->lls_next)
		{
			node->nod_arg[0] = outputs->lls_object;
			CME_expr(node, request);
		}
	}
	else
		request->add_word(0);

	if (request->req_values)
		request->add_byte(blr_end);
	request->add_byte(blr_end);
	request->add_byte(blr_eoc);
	request->req_length = request->req_blr - request->req_base;
	request->req_blr = request->req_base;
}


//____________________________________________________________
//
//		Generate parameter buffer for READY with
//       buffercount.
//

static void cmp_ready( gpre_req* request)
{
	gpre_dbb* db = request->req_database;

	//act* action = request->req_actions;
	request->req_blr = request->req_base = MSC_alloc(250);
	request->req_length = 250;
	request->req_flags |= REQ_exp_hand;

	request->add_byte(isc_dpb_version1);

#ifdef NOT_USED_OR_REPLACED
	if (db->dbb_allocation)
	{
		request->add_byte(isc_dpb_allocation);
		request->add_byte(4);
		request->add_long(db->dbb_allocation);
	}
#endif

	if (db->dbb_pagesize)
	{
		request->add_byte(isc_dpb_page_size);
		request->add_byte(4);
		request->add_long(db->dbb_pagesize);
	}

	if (db->dbb_buffercount)
	{
		request->add_byte(isc_dpb_num_buffers);
		request->add_byte(4);
		request->add_long(db->dbb_buffercount);
	}

	const TEXT* p;
	SSHORT l;

	if (db->dbb_c_user && !db->dbb_r_user)
	{
		request->add_byte(isc_dpb_user_name);
		l = strlen(db->dbb_c_user);
		request->add_byte(l);
		p = db->dbb_c_user;
		while (l--)
			request->add_byte(*p++);
	}

	if (db->dbb_c_password && !db->dbb_r_password)
	{
		request->add_byte(isc_dpb_password);
		l = strlen(db->dbb_c_password);
		request->add_byte(l);
		p = db->dbb_c_password;
		while (l--)
			request->add_byte(*p++);
	}

	if (db->dbb_c_sql_role && !db->dbb_r_sql_role)
	{
		request->add_byte(isc_dpb_sql_role_name);
		l = strlen(db->dbb_c_sql_role);
		request->add_byte(l);
		p = db->dbb_c_sql_role;
		while (l--)
			request->add_byte(*p++);
	}

	if (db->dbb_c_lc_messages && !db->dbb_r_lc_messages)
	{
		// Language must be an ASCII string
		request->add_byte(isc_dpb_lc_messages);
		l = strlen(db->dbb_c_lc_messages);
		request->add_byte(l);
		p = db->dbb_c_lc_messages;
		while (l--)
			request->add_byte(*p++);
	}

	if (db->dbb_c_lc_ctype && !db->dbb_r_lc_ctype)
	{
		// Character Format must be an ASCII string
		request->add_byte(isc_dpb_lc_ctype);
		l = strlen(db->dbb_c_lc_ctype);
		request->add_byte(l);
		p = db->dbb_c_lc_ctype;
		while (l--)
			request->add_byte(*p++);
	}

	*request->req_blr = 0;
	request->req_length = request->req_blr - request->req_base;
	if (request->req_length == 1)
		request->req_length = 0;
	request->req_blr = request->req_base;
}


//____________________________________________________________
//
//		Build in a fudge to bias the language specific subscript to the
//		declared array subscript [i.e. C subscripts are zero based].
//

static void cmp_sdl_fudge( gpre_req* request, SLONG lower_bound)
{

	switch (gpreGlob.sw_language)
	{
	case lang_c:
	case lang_cxx:
    case lang_internal:
		if (!lower_bound)
			break;
		request->add_byte(isc_sdl_add);
		cmp_sdl_number(request, lower_bound);
		break;

	case lang_fortran:
		if (lower_bound == 1)
			break;
		request->add_byte(isc_sdl_add);
		cmp_sdl_number(request, lower_bound - 1);
		break;
	}
}

//____________________________________________________________
//
//		Build an SDL loop for GET_SLICE/PUT_SLICE unless the upper and
//		lower bounds are constant.  Return true if a loop has been built,
//		otherwise false.
//

static bool cmp_sdl_loop(gpre_req* request,
						 USHORT index,
						 const slc* slice,
						 const ary* array)
{

	const slc::slc_repeat* ranges = slice->slc_rpt + index;

	if (ranges->slc_upper == ranges->slc_lower) {
		return false;
	}

	const ary::ary_repeat* bounds = array->ary_rpt + index;

	request->add_byte(isc_sdl_do2);
	request->add_byte(slice->slc_parameters + index);
	cmp_sdl_fudge(request, bounds->ary_lower);
	cmp_sdl_value(request, ranges->slc_lower);
	cmp_sdl_fudge(request, bounds->ary_lower);
	cmp_sdl_value(request, ranges->slc_upper);

	return true;
}


//____________________________________________________________
//
//		Write the number in the 'smallest'
//       form possible to the SDL string.
//

static void cmp_sdl_number( gpre_req* request, SLONG number)
{

	if ((number > -16) && (number < 15))
	{
		request->add_byte(isc_sdl_tiny_integer);
		request->add_byte(number);
	}
	else if ((number > -32768) && (number < 32767))
	{
		request->add_byte(isc_sdl_short_integer);
		request->add_word(number);
	}
	else
	{
		request->add_byte(isc_sdl_long_integer);
		request->add_long(number);
	}
}


//____________________________________________________________
//
//		Build an SDL loop for GET_SLICE/PUT_SLICE unless the upper and
//		lower bounds are constant.
//

static void cmp_sdl_subscript(gpre_req* request, USHORT index, const slc* slice)
{

	const slc::slc_repeat* ranges = slice->slc_rpt + index;

	if (ranges->slc_upper == ranges->slc_lower)
	{
		cmp_sdl_value(request, ranges->slc_upper);
		return;
	}

	request->add_byte(isc_sdl_variable);
	request->add_byte(slice->slc_parameters + index);
}


//____________________________________________________________
//
//		Stuff a slice description language value.
//

static void cmp_sdl_value( gpre_req* request, const gpre_nod* node)
{
	const ref* reference = (ref*) node->nod_arg[0];

	switch (node->nod_type)
	{
	case nod_literal:
		cmp_sdl_number(request, atoi(reference->ref_value));
		return;

	case nod_value:
		request->add_byte(isc_sdl_variable);
		request->add_byte(reference->ref_id);
		return;

	default:
		CPR_error("cmp_sdl_value: node not understood");
	}
}


//____________________________________________________________
//
//		generate blr for set generator
//

static void cmp_set_generator( gpre_req* request)
{
	act* action = request->req_actions;

	request->add_byte(blr_set_generator);
	const set_gen* setgen = (set_gen*) action->act_object;
	const TEXT* string = setgen->sgen_name;
	const SLONG value = setgen->sgen_value;
	const SINT64 int64value = setgen->sgen_int64value;
	request->add_byte(strlen(string));
	while (*string)
		request->add_byte(*string++);
	request->add_byte(blr_literal);
	if (setgen->sgen_dialect == SQL_DIALECT_V5)
	{
		request->add_byte(blr_long);
		request->add_byte(0);
		request->add_word(value);
		request->add_word(value >> 16);
	}
	else
	{
		request->add_byte(blr_int64);
		request->add_byte(0);
		request->add_word(int64value);
		request->add_word(int64value >> 16);
		request->add_word(int64value >> 32);
		request->add_word(int64value >> 48);
	}
}


//____________________________________________________________
//
//

static void cmp_slice( gpre_req* request)
{

	slc* slice = request->req_slice;
	const gpre_fld* field = slice->slc_field;
	gpre_fld* element = field->fld_array;
	const ary* array = field->fld_array_info;

	// Process variable references

	for (ref* reference = request->req_values; reference; reference = reference->ref_next)
	{
		reference->ref_id = slice->slc_parameters++;
	}

	request->req_blr = request->req_base = MSC_alloc(500);
	request->req_length = 500;

	request->add_byte(isc_sdl_version1);
	request->add_byte(isc_sdl_struct);
	request->add_byte(1);
	cmp_field(request, element, 0);

	request->add_byte(isc_sdl_relation);
	CMP_stuff_symbol(request, field->fld_relation->rel_symbol);

	request->add_byte(isc_sdl_field);
	CMP_stuff_symbol(request, field->fld_symbol);

	bool loop_flags[MAX_ARRAY_DIMENSIONS];
	{ // scope block
		fb_assert(slice->slc_dimensions <= MAX_ARRAY_DIMENSIONS);
		USHORT n = 0;
		for (bool* p = loop_flags; n < slice->slc_dimensions; n++, p++)
			*p = cmp_sdl_loop(request, n, slice, array);
	} // end scope block

	request->add_byte(isc_sdl_element);
	request->add_byte(1);
	request->add_byte(isc_sdl_scalar);
	request->add_byte(0);
	request->add_byte(slice->slc_dimensions);
	{ // scope block
		USHORT n = 0;
		for (const bool* p = loop_flags; n < slice->slc_dimensions; n++, p++)
		{
			if (!*p)
				cmp_sdl_fudge(request, array->ary_rpt[n].ary_lower);
			cmp_sdl_subscript(request, n, slice);
		}
	} // end scope block

	request->add_byte(isc_sdl_eoc);
	request->req_length = request->req_blr - request->req_base;
	request->req_blr = request->req_base;
}


//____________________________________________________________
//
//		Generate blr for a store request.
//

static void cmp_store( gpre_req* request)
{
	// Make the store statement under the receive

	if (request->req_type == REQ_store2)
		request->add_byte(blr_store2);
	else
		request->add_byte(blr_store);

	CME_relation(request->req_contexts, request);

	// Make an assignment list

	gpre_nod* list = request->req_node;
	request->add_byte(blr_begin);

    gpre_nod** ptr = list->nod_arg;
	for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		cmp_assignment(*ptr, request);
	}

	request->add_byte(blr_end);

	if (request->req_type == REQ_store2)
	{
		// whip through actions to find return list

		act* action;
		for (action = request->req_actions;; action = action->act_next)
		{
			if (action->act_type == ACT_store2)
				break;
		}
		request->add_byte(blr_begin);
		upd* update = (upd*) action->act_object;
		gpre_port* port = update->upd_port;
		make_send(port, request);
		list = update->upd_assignments;
		const SSHORT count = list->nod_count;
		if (count > 1)
			request->add_byte(blr_begin);
		gpre_nod** ptr2 = list->nod_arg;
		for (const gpre_nod* const* const end = ptr2 + count; ptr2 < end; ptr2++)
			cmp_assignment(*ptr2, request);
		if (count > 1)
			request->add_byte(blr_end);
		request->add_byte(blr_end);
	}
}


//____________________________________________________________
//
//		During the parsing of an SQL statement
//		we may have run into an indicator variable.
//		If so, all we've got now is its name, and
//		we really ought to build a full reference
//		block for it before we forget.
//

static void expand_references( ref* reference)
{
	for (; reference; reference = reference->ref_next)
	{
		if (reference->ref_null_value && !reference->ref_null)
		{
			ref* flag = (ref*) MSC_alloc(REF_LEN);
			flag->ref_next = reference->ref_next;
			reference->ref_next = reference->ref_null = flag;
			flag->ref_master = reference;
			flag->ref_value = reference->ref_null_value;
			flag->ref_field = PAR_null_field();
		}
	}
}


//____________________________________________________________
//
//		Make up a port block and process a linked list
//		of field references.
//

static gpre_port* make_port( gpre_req* request, ref* reference)
{
	gpre_port* port = (gpre_port*) MSC_alloc(POR_LEN);
	port->por_ident = CMP_next_ident();
	port->por_msg_number = request->req_count++;
	port->por_next = request->req_ports;
	request->req_ports = port;

	// Hmmm -- no references.  Not going to fly.
	// Make up a dummy reference.

	if (!reference)
	{
		reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_field = eof_field;
	}

	ref* misc = 0;
	ref* alignments[3];
	alignments[0] = alignments[1] = alignments[2] = NULL;

	for (ref* temp = reference; temp; temp = reference)
	{
		reference = reference->ref_next;
		gpre_fld* field = temp->ref_field;
		if (!field)
			CPR_bugcheck("missing prototype field for value");
		if (temp->ref_value && (temp->ref_flags & REF_array_elem))
			field = field->fld_array;
		if ((field->fld_length & 7) == 0)
		{
			temp->ref_next = alignments[2];
			alignments[2] = temp;
		}
		else if ((field->fld_length & 3) == 0)
		{
			temp->ref_next = alignments[1];
			alignments[1] = temp;
		}
		else if ((field->fld_length & 1) == 0)
		{
			temp->ref_next = alignments[0];
			alignments[0] = temp;
		}
		else
		{
			temp->ref_next = misc;
			misc = temp;
		}
	}

	for (int i = 0; i <= 2; i++)
		while (reference = alignments[i])
		{
			alignments[i] = reference->ref_next;
			reference->ref_next = misc;
			misc = reference;
		}

	port->por_references = misc;

	for (reference = misc; reference; reference = reference->ref_next)
	{
		gpre_fld* field = reference->ref_field;
		if (reference->ref_value && (reference->ref_flags & REF_array_elem))
			field = field->fld_array;
		reference->ref_ident = CMP_next_ident();
		reference->ref_parameter = port->por_count++;
		reference->ref_port = port;
#ifdef GPRE_FORTRAN
		reference->ref_offset = port->por_length;
#endif
		port->por_length += field->fld_length;
	}

	return port;
}


//____________________________________________________________
//
//		Make a receive node for a given port.
//

static void make_receive( gpre_port* port, gpre_req* request)
{
	request->add_byte(blr_receive);
	request->add_byte(port->por_msg_number);
}


//____________________________________________________________
//
//		Make a receive node for a given port.
//

static void make_send( gpre_port* port, gpre_req* request)
{
	request->add_byte(blr_send);
	request->add_byte(port->por_msg_number);
}


//____________________________________________________________
//
//		where a duplicate reference list is used
//		to build the port, fix request_references
//

static void update_references( ref* references)
{
	for (ref* re = references; re; re = re->ref_next)
	{
		ref* source = re->ref_source;
		source->ref_port = re->ref_port;
		source->ref_parameter = re->ref_parameter;
		source->ref_ident = re->ref_ident;
	}
}

