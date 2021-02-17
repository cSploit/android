//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		cmd.cpp
//		DESCRIPTION:	Data definition compiler
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
#include <string.h>
#include "../gpre/gpre.h"
#include "../jrd/ibase.h"
#include "../jrd/flags.h"
#include "../gpre/cmd_proto.h"
#include "../gpre/cme_proto.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/msc_proto.h"
#include "../gpre/gpre_meta.h"

typedef void (*pfn_local_trigger_cb) (gpre_nod*, gpre_req*);

//static void add_cache(gpre_req*, const act*, gpre_dbb*);
static void alter_database(gpre_req*, act*);
static void alter_domain(gpre_req*, const act*);
static void alter_index(gpre_req*, const act*);
static void alter_table(gpre_req*, const act*);
static void create_check_constraint(gpre_req*, const act*, cnstrt*);
static void create_constraint(gpre_req*, const act*, cnstrt*);
static void create_database(gpre_req*, const act*);
static void create_database_modify_dyn(gpre_req*, act*);
static void create_default_blr(gpre_req*, const TEXT*, const USHORT);
static void create_del_cascade_trg(gpre_req*, const act*, cnstrt*);
static void create_domain(gpre_req*, const act*);
static void create_domain_constraint(gpre_req*, const cnstrt*);
static void create_generator(gpre_req*, const act*);
static void create_index(gpre_req*, const gpre_index*);
static void create_matching_blr(gpre_req*, const cnstrt*);
static void create_set_default_trg(gpre_req*, const act*, cnstrt*, bool);
static void create_set_null_trg(gpre_req*, const act*, cnstrt*, bool);
static void create_shadow(gpre_req*, act*);
static void create_table(gpre_req*, const act*);
static void create_trg_firing_cond(gpre_req*, const cnstrt*);
static void create_trigger(gpre_req*, const act*, gpre_trg*, pfn_local_trigger_cb);
static void create_upd_cascade_trg(gpre_req*, const act*, cnstrt*);
static bool create_view(gpre_req*, act*);
static void create_view_trigger(gpre_req*, const act*, gpre_trg*, gpre_nod*, gpre_ctx**);
static void declare_filter(gpre_req*, const act*);
static void declare_udf(gpre_req*, const act*);
static void get_referred_fields(const act*, cnstrt*);
static void grant_revoke_privileges(gpre_req*, const act*);
static void init_field_struct(gpre_fld*);
static void put_array_info(gpre_req*, const gpre_fld*);
static void put_blr(gpre_req*, USHORT, gpre_nod*, pfn_local_trigger_cb);
static void put_computed_blr(gpre_req*, const gpre_fld*);
static void put_computed_source(gpre_req*, const gpre_fld*);
static void put_cstring(gpre_req*, USHORT, const TEXT*);
static void put_dtype(gpre_req*, const gpre_fld*);
static void put_field_attributes(gpre_req*, const gpre_fld*);
static void put_numeric(gpre_req*, USHORT, SSHORT);
static void put_short_cstring(gpre_req*, USHORT, const TEXT*);
static void put_string(gpre_req*, USHORT, const TEXT*, USHORT);
static void put_symbol(gpre_req*, int, const gpre_sym*);
static void put_trigger_blr(gpre_req*, USHORT, gpre_nod*, pfn_local_trigger_cb);
static void put_view_trigger_blr(gpre_req*, const gpre_rel*, USHORT, gpre_trg*,
								 gpre_nod*, gpre_ctx**);
static void replace_field_names(gpre_nod* const, const gpre_nod* const,
								gpre_fld* const, bool, gpre_ctx**);
static void set_statistics(gpre_req*, const act*);

static inline void STUFF_CHECK(gpre_req* request, int n)
{
	if (request->req_base - request->req_blr + request->req_length <= n + 50)
		CMP_check(request, n);
}


const int BLOB_BUFFER_SIZE = 4096;	// to read in blr blob for default values


//____________________________________________________________
//
//		Compile a single request, but do not generate any text.
//		Generate port blocks, assign parameter numbers, message
//		numbers, and internal idents.  Compute length of request.
//

int CMD_compile_ddl(gpre_req* request)
{
	// Initialize the blr string

	act* action = request->req_actions;
	request->req_blr = request->req_base = MSC_alloc(250);
	request->req_length = 250;
	request->req_flags |= REQ_exp_hand;
	request->add_byte(isc_dyn_version_1);
	request->add_byte(isc_dyn_begin);

	switch (action->act_type)
	{
	case ACT_alter_table:
		alter_table(request, action);
		break;

	case ACT_alter_database:
		alter_database(request, action);
		break;

	case ACT_alter_domain:
		alter_domain(request, action);
		break;

	case ACT_alter_index:
		alter_index(request, action);
		break;

	case ACT_create_database:
		if (!(request->req_flags & REQ_sql_database_dyn))
		{
			request->req_blr = request->req_base;
			create_database(request, action);
			return 0;
		}
		create_database_modify_dyn(request, action);
		break;

	case ACT_create_domain:
		create_domain(request, action);
		break;

	case ACT_create_generator:
		create_generator(request, action);
		break;

	case ACT_create_index:
		{
			const gpre_index* index = (gpre_index*) action->act_object;
			create_index(request, index);
		}
		break;

	case ACT_create_shadow:
		create_shadow(request, action);
		break;

	case ACT_create_table:
		create_table(request, action);
		break;

	case ACT_create_view:
		if (!create_view(request, action))
			return -1;
		break;

	case ACT_drop_database:
		break;

	case ACT_drop_domain:
		put_cstring(request, isc_dyn_delete_global_fld, (TEXT *) action->act_object);
		request->add_end();
		break;

	case ACT_drop_filter:
		put_cstring(request, isc_dyn_delete_filter, (TEXT *) action->act_object);
		request->add_end();
		break;

	case ACT_drop_index:
		{
			const gpre_index* index = (gpre_index*) action->act_object;
			put_symbol(request, isc_dyn_delete_idx, index->ind_symbol);
			request->add_end();
		}
		break;

	case ACT_drop_shadow:
		put_numeric(request, isc_dyn_delete_shadow, (SSHORT) (IPTR) action->act_object);
		request->add_end();
		break;

	case ACT_drop_table:
	case ACT_drop_view:
		{
			const gpre_rel* relation = (gpre_rel*) action->act_object;
			put_symbol(request, isc_dyn_delete_rel, relation->rel_symbol);
			request->add_end();
		}
		break;

	case ACT_drop_udf:
		put_cstring(request, isc_dyn_delete_function, (TEXT *) action->act_object);
		request->add_end();
		break;

	case ACT_dyn_grant:
		grant_revoke_privileges(request, action);
		break;

	case ACT_dyn_revoke:
		grant_revoke_privileges(request, action);
		break;

	case ACT_declare_filter:
		declare_filter(request, action);
		break;

	case ACT_declare_udf:
		declare_udf(request, action);
		break;

	case ACT_statistics:
		set_statistics(request, action);
		break;

	default:
		CPR_error("*** unknown request type node ***");
		return -1;
	}

	request->add_end();
	request->add_byte(isc_dyn_eoc);
	request->req_length = request->req_blr - request->req_base;
	request->req_blr = request->req_base;

	return 0;
}


//____________________________________________________________
//
//		Add cache file to a database.
//
/*
static void add_cache( gpre_req* request, const act* action, gpre_dbb* database)
{
	TEXT file_name[254]; // CVC: Maybe MAXPATHLEN?

	const gpre_file* file = database->dbb_cache_file;
	const SSHORT l = MIN((strlen(file->fil_name)), (sizeof(file_name) - 1));

	strncpy(file_name, file->fil_name, l);
	file_name[l] = '\0';
	put_cstring(request, isc_dyn_def_cache_file, file_name);
	request->add_byte(isc_dyn_file_length);
	request->add_word(4);
	request->add_long(file->fil_length);
	request->add_end();
}
*/

//____________________________________________________________
//
//		Generate dynamic DDL for modifying database.
//

static void alter_database( gpre_req* request, act* action)
{
	gpre_dbb* db = (gpre_dbb*) action->act_object;

	request->add_byte(isc_dyn_mod_database);

	// Reverse the order of files (parser left them backwards)

	gpre_file* next;
	gpre_file* files = NULL;
	for (gpre_file* file = db->dbb_files; file; file = next)
	{
		next = file->fil_next;
		file->fil_next = files;
		files = file;
	}

	for (const gpre_file* file = files; file != NULL; file = file->fil_next)
	{
		put_cstring(request, isc_dyn_def_file, file->fil_name);
		request->add_byte(isc_dyn_file_start);
		request->add_word(4);
		const SLONG start = file->fil_start;
		request->add_long(start);
		request->add_byte(isc_dyn_file_length);
		request->add_word(4);
		request->add_long(file->fil_length);
		request->add_end();
	}

	// Drop cache
/*
	if (db->dbb_flags & DBB_drop_cache)
		request->add_byte(isc_dyn_drop_cache);

	if (db->dbb_cache_file)
		add_cache(request, action, db);
*/

	if (db->dbb_def_charset)
		put_cstring(request, isc_dyn_fld_character_set_name, db->dbb_def_charset);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for CREATE DOMAIN action.
//

static void alter_domain( gpre_req* request, const act* action)
{
	const gpre_fld* field = (gpre_fld*) action->act_object;

	// modify field info

	put_symbol(request, isc_dyn_mod_global_fld, field->fld_symbol);

	const gpre_nod* default_node = field->fld_default_value;
	if (default_node)
	{
		if (default_node->nod_type == nod_erase) {
			request->add_byte(isc_dyn_del_default);
		}
		else
		{
			put_blr(request, isc_dyn_fld_default_value, field->fld_default_value, CME_expr);
			TEXT* default_source = (TEXT *) MSC_alloc(field->fld_default_source->txt_length + 1);
			CPR_get_text(default_source, field->fld_default_source);
			put_cstring(request, isc_dyn_fld_default_source, default_source);
		}
	}

	if (field->fld_constraints)
	{
		request->add_byte(isc_dyn_single_validation);
		for (const cnstrt* constraint = field->fld_constraints;
			 constraint;
			 constraint = constraint->cnstrt_next)
		{
			if (constraint->cnstrt_flags & CNSTRT_delete)
				request->add_byte(isc_dyn_del_validation);
		}
		create_domain_constraint(request, field->fld_constraints);
	}

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for ALTER INDEX statement
//

static void alter_index( gpre_req* request, const act* action)
{
	const gpre_index* index = (gpre_index*) action->act_object;
	put_symbol(request, isc_dyn_mod_idx, index->ind_symbol);

	const SSHORT value = (index->ind_flags & IND_active) ? 0 : 1;
	put_numeric(request, isc_dyn_idx_inactive, value);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for ALTER TABLE action.
//

static void alter_table( gpre_req* request, const act* action)
{
	// add relation name

	const gpre_rel* relation = (gpre_rel*) action->act_object;
	put_symbol(request, isc_dyn_mod_rel, relation->rel_symbol);

	// add field info

	for (const gpre_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		if (field->fld_flags & FLD_delete)
		{
			put_symbol(request, isc_dyn_delete_local_fld, field->fld_symbol);
			request->add_end();
		}
		else if (field->fld_flags & FLD_meta)
		{
			if (field->fld_global)
			{
				put_symbol(request, isc_dyn_def_local_fld, field->fld_symbol);
				put_symbol(request, isc_dyn_fld_source, field->fld_global);
			}
			else
				put_symbol(request, isc_dyn_def_sql_fld, field->fld_symbol);

			put_field_attributes(request, field);

			if (field->fld_default_value)
			{
				put_blr(request, isc_dyn_fld_default_value, field->fld_default_value, CME_expr);
				TEXT* default_source = (TEXT*) MSC_alloc(field->fld_default_source->txt_length + 1);
				CPR_get_text(default_source, field->fld_default_source);
				put_cstring(request, isc_dyn_fld_default_source, default_source);
			}

			request->add_end();

			// Any constraints defined for field being added?

			if (field->fld_constraints)
				create_constraint(request, action, field->fld_constraints);
		}
	}

	// Check for any relation level ADD/DROP of constraints

	if (relation->rel_constraints)
	{
		for (const cnstrt* constraint = relation->rel_constraints;
			 constraint;
			 constraint = constraint->cnstrt_next)
		{
			if (constraint->cnstrt_flags & CNSTRT_delete)
				put_cstring(request, isc_dyn_delete_rel_constraint,
							constraint->cnstrt_name->str_string);
		}
		create_constraint(request, action, relation->rel_constraints);
	}

	request->add_end();
}


//____________________________________________________________
//
//		Generate dyn for creating a CHECK constraint.
//

static void create_check_constraint( gpre_req* request, const act* action,
	cnstrt* constraint)
{
	gpre_trg* trigger = (gpre_trg*) MSC_alloc(TRG_LEN);

	// create the INSERT trigger

	trigger->trg_type = PRE_STORE_TRIGGER;

	// "insert violates CHECK constraint on table"

	trigger->trg_message = NULL;
	trigger->trg_boolean = constraint->cnstrt_boolean;
	trigger->trg_source = (STR) MSC_alloc(constraint->cnstrt_text->txt_length + 1);
	CPR_get_text(trigger->trg_source->str_string, constraint->cnstrt_text);
	create_trigger(request, action, trigger, CME_expr);

	// create the UPDATE trigger

	trigger->trg_type = PRE_MODIFY_TRIGGER;

	// "update violates CHECK constraint on table"

	create_trigger(request, action, trigger, CME_expr);
}


//____________________________________________________________
//
//  Function
//		Generate blr to express: if (old.primary_key != new.primary_key).
//		do a column by column comparison.
//

static void create_trg_firing_cond( gpre_req* request, const cnstrt* constraint)
{
	// count primary key columns
	const gpre_lls* prim_key_fld = constraint->cnstrt_referred_fields;
	const gpre_lls* field = prim_key_fld;

	fb_assert(field != NULL);

	USHORT prim_key_num_flds = 0;
	while (field)
	{
		prim_key_num_flds++;
		field = field->lls_next;
	}

	fb_assert(prim_key_num_flds > 0);

	// generate blr
	request->add_byte(blr_if);
	if (prim_key_num_flds > 1)
		request->add_byte(blr_or);

	USHORT num_flds = 0;
	for (; prim_key_fld; prim_key_fld = prim_key_fld->lls_next)
	{
		request->add_byte(blr_neq);

		const str* prim_key_fld_name = (STR) prim_key_fld->lls_object;

		request->add_byte(blr_field);
		request->add_byte((SSHORT) 0);
		put_cstring(request, 0, prim_key_fld_name->str_string);
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 1);
		put_cstring(request, 0, prim_key_fld_name->str_string);

		++num_flds;

		if (prim_key_num_flds - num_flds >= 2)
			request->add_byte(blr_or);
	}
}


//____________________________________________________________
//
//  Function
//		Generate blr to express: foreign_key == primary_key
//		ie.,  for_key.column_1 = prim_key.column_1 and
//		for_key.column_2 = prim_key.column_2 and ....  so on..
//

static void create_matching_blr(gpre_req* request, const cnstrt* constraint)
{
	// count primary key columns
	const gpre_lls*  prim_key_fld = constraint->cnstrt_referred_fields;
	const gpre_lls* field = prim_key_fld;

	fb_assert(field != NULL);

	USHORT prim_key_num_flds = 0;
	while (field)
	{
		prim_key_num_flds++;
		field = field->lls_next;
	}

	fb_assert(prim_key_num_flds > 0);

	// count of foreign key columns
	const gpre_lls* for_key_fld = constraint->cnstrt_fields;
	field = for_key_fld;

	fb_assert(field != NULL);

	USHORT for_key_num_flds = 0;
	while (field)
	{
		for_key_num_flds++;
		field = field->lls_next;
	}

	fb_assert(for_key_num_flds > 0);
	fb_assert(prim_key_num_flds == for_key_num_flds);

	request->add_byte(blr_boolean);
	if (prim_key_num_flds > 1)
		request->add_byte(blr_and);

	USHORT num_flds = 0;

	do {
		request->add_byte(blr_eql);

		const str* for_key_fld_name = (STR) for_key_fld->lls_object;
		const str* prim_key_fld_name = (STR) prim_key_fld->lls_object;

		request->add_byte(blr_field);
		request->add_byte((SSHORT) 2);
		put_cstring(request, 0, for_key_fld_name->str_string);
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 0);
		put_cstring(request, 0, prim_key_fld_name->str_string);

		++num_flds;

		if (prim_key_num_flds - num_flds >= 2)
			request->add_byte(blr_and);

		for_key_fld = for_key_fld->lls_next;
		prim_key_fld = prim_key_fld->lls_next;
	} while (num_flds < for_key_num_flds);

	request->add_byte(blr_end);
}


//____________________________________________________________
//  Function:
//		The default_blr is passed in default_buffer. It is of the form:
//		blr_version4 blr_literal ..... blr_eoc.
//		strip the blr_version4/blr_version5 and blr_eoc verbs and stuff the remaining
//		blr in the blr stream in the request.
//

static void create_default_blr(gpre_req* request,
							   const TEXT* default_buff, const USHORT buff_size)
{
	int i;

	fb_assert(*default_buff == blr_version4 || *default_buff == blr_version5);

	for (i = 1; ((i < buff_size) && (default_buff[i] != blr_eoc)); i++)
		request->add_byte(default_buff[i]);

	fb_assert(default_buff[i] == blr_eoc);

}


//____________________________________________________________
//
//		Generate dyn for "on update cascade" trigger (for referential
//		integrity) along with the trigger blr.
//

static void create_upd_cascade_trg( gpre_req* request, const act* action,
								   cnstrt* constraint)
{
	gpre_lls* for_key_fld = constraint->cnstrt_fields;
	gpre_lls* prim_key_fld = constraint->cnstrt_referred_fields;
	gpre_rel* relation = (gpre_rel*) action->act_object;

	// no trigger name is generated here. Let the engine make one up
	put_string(request, isc_dyn_def_trigger, "", (USHORT) 0);

	put_numeric(request, isc_dyn_trg_type, (SSHORT) POST_MODIFY_TRIGGER);

	request->add_byte(isc_dyn_sql_object);
	put_numeric(request, isc_dyn_trg_sequence, (SSHORT) 1);
	put_numeric(request, isc_dyn_trg_inactive, (SSHORT) 0);
	put_cstring(request, isc_dyn_rel_name, constraint->cnstrt_referred_rel->str_string);

	// the trigger blr

	request->add_byte(isc_dyn_trg_blr);
	const USHORT offset = request->req_blr - request->req_base;

	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);

	create_trg_firing_cond(request, constraint);

	request->add_byte(blr_begin);
	request->add_byte(blr_begin);

	request->add_byte(blr_for);
	request->add_byte(blr_rse);

	// the new context for the prim. key relation
	request->add_byte(1);

	request->add_byte(blr_relation);
	put_cstring(request, 0, relation->rel_symbol->sym_string);
	// the context for the foreign key relation
	request->add_byte(2);

	// generate the blr for: foreign_key == primary_key
	create_matching_blr(request, constraint);

	request->add_byte(blr_modify);
	request->add_byte((SSHORT) 2);
	request->add_byte((SSHORT) 2);
	request->add_byte(blr_begin);

	for (; for_key_fld && prim_key_fld;
		 for_key_fld = for_key_fld->lls_next, prim_key_fld = prim_key_fld->lls_next)
	{
		const str* for_key_fld_name = (STR) for_key_fld->lls_object;
		const str* prim_key_fld_name = (STR) prim_key_fld->lls_object;

		request->add_byte(blr_assignment);
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 1);
		put_cstring(request, 0, prim_key_fld_name->str_string);
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 2);
		put_cstring(request, 0, for_key_fld_name->str_string);
	}

	request->add_byte(blr_end);
	request->add_byte(blr_end);
	request->add_byte(blr_end);
	request->add_byte(blr_end);

	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
	// end of the blr

	// no trg_source and no trg_description
	request->add_byte(isc_dyn_end);
}


//____________________________________________________________
//
//		Generate dyn for "on delete cascade" trigger (for referential
//		integrity) along with the trigger blr.
//

static void create_del_cascade_trg( gpre_req* request, const act* action,
								   cnstrt* constraint)
{
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	// stuff a trigger_name of size 0. So the dyn-parser will make one up.
	put_string(request, isc_dyn_def_trigger, "", (USHORT) 0);

	put_numeric(request, isc_dyn_trg_type, (SSHORT) POST_ERASE_TRIGGER);

	request->add_byte(isc_dyn_sql_object);
	put_numeric(request, isc_dyn_trg_sequence, (SSHORT) 1);
	put_numeric(request, isc_dyn_trg_inactive, (SSHORT) 0);
	put_cstring(request, isc_dyn_rel_name, constraint->cnstrt_referred_rel->str_string);

	// the trigger blr
	request->add_byte(isc_dyn_trg_blr);
	const USHORT offset = request->req_blr - request->req_base;

	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);

	request->add_byte(blr_for);
	request->add_byte(blr_rse);

	// the context for the prim. key relation
	request->add_byte(1);

	request->add_byte(blr_relation);
	put_cstring(request, 0, relation->rel_symbol->sym_string);
	// the context for the foreign key relation
	request->add_byte(2);

	create_matching_blr(request, constraint);

	request->add_byte(blr_erase);
	request->add_byte((SSHORT) 2);
	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
	// end of the blr

	// no trg_source and no trg_description
	request->add_byte(isc_dyn_end);
}


//____________________________________________________________
//
//		Generate dyn for "on delete|update set default" trigger (for
//		referential integrity) along with the trigger blr.
//		The non_upd_trg parameter == true is an update trigger.
//

static void create_set_default_trg(gpre_req* request,
								   const act* action,
								   cnstrt* constraint,
								   bool on_upd_trg)
{
	TEXT s[512];
	TEXT default_val[BLOB_BUFFER_SIZE];

	fb_assert(request->req_actions->act_type == ACT_create_table ||
		   request->req_actions->act_type == ACT_alter_table);

	gpre_lls* for_key_fld = constraint->cnstrt_fields;
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	// no trigger name. It is generated by the engine
	put_string(request, isc_dyn_def_trigger, "", (USHORT) 0);

	put_numeric(request, isc_dyn_trg_type,
				(SSHORT) (on_upd_trg ? POST_MODIFY_TRIGGER : POST_ERASE_TRIGGER));

	request->add_byte(isc_dyn_sql_object);
	put_numeric(request, isc_dyn_trg_sequence, (SSHORT) 1);
	put_numeric(request, isc_dyn_trg_inactive, (SSHORT) 0);
	put_cstring(request, isc_dyn_rel_name, constraint->cnstrt_referred_rel->str_string);

	// the trigger blr

	request->add_byte(isc_dyn_trg_blr);
	const USHORT offset = request->req_blr - request->req_base;

	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);

	// for ON UPDATE TRIGGER only: generate the trigger firing condition:
	// if prim_key.old_value != prim_key.new value.
	// Note that the key could consist of multiple columns

	if (on_upd_trg)
	{
		create_trg_firing_cond(request, constraint);
		request->add_byte(blr_begin);
		request->add_byte(blr_begin);
	}

	request->add_byte(blr_for);
	request->add_byte(blr_rse);

	// the context for the prim. key relation
	request->add_byte(1);

	request->add_byte(blr_relation);
	put_cstring(request, 0, relation->rel_symbol->sym_string);
	// the context for the foreign key relation
	request->add_byte(2);

	create_matching_blr(request, constraint);

	request->add_byte(blr_modify);
	request->add_byte((SSHORT) 2);
	request->add_byte((SSHORT) 2);
	request->add_byte(blr_begin);

	for (; for_key_fld; for_key_fld = for_key_fld->lls_next)
	{
		// for every column in the foreign key ....
		const str* for_key_fld_name = (STR) for_key_fld->lls_object;

		request->add_byte(blr_assignment);

		// here stuff the default value as blr_literal .... or blr_null
		// if this column does not have an applicable default

		// the default is determined in many cases:
		// (1) The default-info for the column is in memory. (This is because
		// the column is being created in this application)
		// (1-a) the table has a column level default. We get this by
		// searching both current ddl statement and requests
		// linked list.
		// (1-b) the table does not have a column level default, but
		// has a domain default. We get this by searching
		// requests linked list.
		// (2) The default-info for the column is not in memory.
		// We get the column and/or domain default value from the system
		// tables by calling MET_get_column_default/MET_get_domain_default

		bool search_for_default = true;
		bool search_for_column = false;
		const TEXT* search_for_domain = NULL;

		// Is the column being created in this ddl statement?
		gpre_fld* field;
		for (field = relation->rel_fields; field; field = field->fld_next)
		{
			if (strcmp(field->fld_symbol->sym_string, for_key_fld_name->str_string) == 0)
			{
				break;
			}
		}

		if (field)
		{
			// Yes. The column is being created in this ddl statement
			if (field->fld_default_value) {
				// (1-a)
				CME_expr(field->fld_default_value, request);
				search_for_default = false;
			}
			else
			{
				// check for domain default
				if (field->fld_global)
				{
					// could be either (1-b) or (2)
					search_for_domain = field->fld_global->sym_string;
				}
				else
				{
					request->add_byte(blr_null);
					search_for_default = false;
				}
			}
		}
		else
		{
			// Nop. The column is not being created in this ddl statement
			if (request->req_actions->act_type == ACT_create_table)
			{
				sprintf(s, "field \"%s\" does not exist in relation \"%s\"",
						for_key_fld_name->str_string, relation->rel_symbol->sym_string);
				CPR_error(s);
			}

			// Thus we have an ALTER TABLE statement.

			// If somebody is 'clever' enough to create table and then to alter it
			// within the same application ...
			gpre_req* req = gpreGlob.requests;
			for (; req; req = req->req_next)
			{
				act* request_action;
				gpre_rel* rel;
				if (req->req_type == REQ_ddl && (request_action = req->req_actions) &&
					(request_action->act_type == ACT_create_table ||
						request_action->act_type == ACT_alter_table) &&
					(rel = (gpre_rel*) request_action->act_object) &&
					strcmp(rel->rel_symbol->sym_string, relation->rel_symbol->sym_string) == 0)
				{
					// ... then try to check for the default in memory
					gpre_fld* fld;
					for (fld = (gpre_fld*) rel->rel_fields; fld; fld = fld->fld_next)
					{
						if (strcmp(fld->fld_symbol->sym_string, for_key_fld_name->str_string) != 0)
							continue;
						if (fld->fld_default_value)
						{
							// case (1-a):
							CME_expr(fld->fld_default_value, request);
							search_for_default = false;
						}
						else
						{
							if (fld->fld_global) {
								search_for_domain = fld->fld_global->sym_string;
							}
							else
							{
								// default not found
								request->add_byte(blr_null);
								search_for_default = false;
							}
						}
						break;
					}
					if (fld)
						break;
				}
			}
			if (!req)
				search_for_column = true;
		}

		if (search_for_default && search_for_domain != NULL)
		{
			// search for domain level default
			fb_assert(search_for_column == false);
			// search for domain in memory
			for (gpre_req* req = gpreGlob.requests; req; req = req->req_next)
			{
				act* request_action;
				gpre_fld* domain;
				if (req->req_type == REQ_ddl &&
					(request_action = req->req_actions) &&
					(request_action->act_type == ACT_create_domain ||
						request_action->act_type == ACT_alter_domain) &&
					(domain = (gpre_fld*) request_action->act_object) &&
					strcmp(search_for_domain, domain->fld_symbol->sym_string) == 0 &&
					domain->fld_default_value->nod_type != nod_erase)
				{
					// domain found in memory
					if (domain->fld_default_value)
					{
						// case (1-b)
						CME_expr(domain->fld_default_value, request);
					}
					else
					{
						// default not found
						request->add_byte(blr_null);
					}
					search_for_default = false;
					break;
				}
			}

			if (search_for_default)
			{
				// search for domain in db system tables
				if (MET_get_domain_default(relation->rel_database, search_for_domain,
										   default_val, sizeof(default_val)))
				{
					create_default_blr(request, default_val, sizeof(default_val));
				}
				else {
					request->add_byte(blr_null);
				}
				search_for_default = false;
			}
		}						// end of search for domain level default

		if (search_for_default && search_for_column)
		{
			// nothing is found in memory, try to check db system tables
			fb_assert(search_for_domain == NULL);
			if (MET_get_column_default(relation, for_key_fld_name->str_string,
									   default_val, sizeof(default_val)))
			{
				create_default_blr(request, default_val, sizeof(default_val));
			}
			else {
				request->add_byte(blr_null);
			}
		}

		// the context for the foreign key relation
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 2);
		put_cstring(request, 0, for_key_fld_name->str_string);

	}
	request->add_byte(blr_end);

	if (on_upd_trg)
	{
		request->add_byte(blr_end);
		request->add_byte(blr_end);
		request->add_byte(blr_end);
	}

	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
	// end of the blr

	// no trg_source and no trg_description
	request->add_byte(isc_dyn_end);

}


//____________________________________________________________
//
//		Generate dyn for "on delete|update set null" trigger (for
//		referential integrity). The trigger blr is the same for
//		both the delete and update cases. Only differences is its
//		TRIGGER_TYPE (ON DELETE or ON UPDATE).
//		The non_upd_trg parameter == true is an update trigger.
//

static void create_set_null_trg(gpre_req* request,
								const act* action,
								cnstrt* constraint,
								bool on_upd_trg)
{
	gpre_lls* for_key_fld = constraint->cnstrt_fields;
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	// no trigger name. It is generated by the engine
	put_string(request, isc_dyn_def_trigger, "", (USHORT) 0);

	put_numeric(request, isc_dyn_trg_type,
				(SSHORT) (on_upd_trg ? POST_MODIFY_TRIGGER : POST_ERASE_TRIGGER));

	request->add_byte(isc_dyn_sql_object);
	put_numeric(request, isc_dyn_trg_sequence, (SSHORT) 1);
	put_numeric(request, isc_dyn_trg_inactive, (SSHORT) 0);
	put_cstring(request, isc_dyn_rel_name, constraint->cnstrt_referred_rel->str_string);

	// the trigger blr

	request->add_byte(isc_dyn_trg_blr);
	const USHORT offset = request->req_blr - request->req_base;

	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);

	// for ON UPDATE TRIGGER only: generate the trigger firing condition:
	// if prim_key.old_value != prim_key.new value.
	// Note that the key could consist of multiple columns

	if (on_upd_trg)
	{
		create_trg_firing_cond(request, constraint);
		request->add_byte(blr_begin);
		request->add_byte(blr_begin);
	}

	request->add_byte(blr_for);
	request->add_byte(blr_rse);

	// the context for the prim. key relation
	request->add_byte(1);

	request->add_byte(blr_relation);
	put_cstring(request, 0, relation->rel_symbol->sym_string);
	// the context for the foreign key relation
	request->add_byte(2);

	create_matching_blr(request, constraint);

	request->add_byte(blr_modify);
	request->add_byte((SSHORT) 2);
	request->add_byte((SSHORT) 2);
	request->add_byte(blr_begin);

	for (; for_key_fld; for_key_fld = for_key_fld->lls_next)
	{
		const str* for_key_fld_name = (STR) for_key_fld->lls_object;

		request->add_byte(blr_assignment);
		request->add_byte(blr_null);
		request->add_byte(blr_field);
		request->add_byte((SSHORT) 2);
		put_cstring(request, 0, for_key_fld_name->str_string);
	}
	request->add_byte(blr_end);

	if (on_upd_trg)
	{
		request->add_byte(blr_end);
		request->add_byte(blr_end);
		request->add_byte(blr_end);
	}

	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
	// end of the blr

	// no trg_source and no trg_description
	request->add_byte(isc_dyn_end);
}


//____________________________________________________________
//
//		Get referred fields from memory/system tables
//

static void get_referred_fields(const act* action, cnstrt* constraint)
{
	TEXT s[512];
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	for (gpre_req* req = gpreGlob.requests; req; req = req->req_next)
	{
		const act* request_action;
		const gpre_rel* rel;
		if (req->req_type == REQ_ddl && (request_action = req->req_actions) &&
			(request_action->act_type == ACT_create_table ||
				request_action->act_type == ACT_alter_table) &&
			(rel = (gpre_rel*) request_action->act_object) &&
			strcmp(rel->rel_symbol->sym_string, constraint->cnstrt_referred_rel->str_string) == 0)
		{
			for (const cnstrt* cns = rel->rel_constraints; cns; cns = cns->cnstrt_next)
			{
				if (cns->cnstrt_type == CNSTRT_PRIMARY_KEY)
				{
					constraint->cnstrt_referred_fields = cns->cnstrt_fields;
					break;
				}
			}
			if (!constraint->cnstrt_referred_fields && rel->rel_fields)
				for (const cnstrt* cns = rel->rel_fields->fld_constraints; cns; cns = cns->cnstrt_next)
				{
					if (cns->cnstrt_type == CNSTRT_PRIMARY_KEY)
					{
						constraint->cnstrt_referred_fields = cns->cnstrt_fields;
						break;
					}
				}
			if (constraint->cnstrt_referred_fields)
				break;
		}
	}

	if (constraint->cnstrt_referred_fields == NULL)
		// Nothing is in memory. Try to find in system tables
		constraint->cnstrt_referred_fields =
			MET_get_primary_key(relation->rel_database, constraint->cnstrt_referred_rel->str_string);

	if (constraint->cnstrt_referred_fields == NULL)
	{
		// Nothing is in system tables.
		sprintf(s,
				"\"REFERENCES %s\" without \"(column list)\" requires PRIMARY KEY on referenced table",
				constraint->cnstrt_referred_rel->str_string);
		CPR_error(s);
	}
	else
	{
		// count both primary key and foreign key columns
		USHORT prim_key_num_flds = 0, for_key_num_flds = 0;
		const gpre_lls* field = constraint->cnstrt_referred_fields;
		while (field)
		{
			prim_key_num_flds++;
			field = field->lls_next;
		}
		field = constraint->cnstrt_fields;
		while (field)
		{
			for_key_num_flds++;
			field = field->lls_next;
		}
		if (prim_key_num_flds != for_key_num_flds)
		{
			sprintf(s,
					"PRIMARY KEY column count in relation \"%s\" does not match FOREIGN KEY in relation \"%s\"",
					constraint->cnstrt_referred_rel->str_string,
					relation->rel_symbol->sym_string);
			CPR_error(s);
		}
	}
}


//____________________________________________________________
//
//		Generate dyn for creating a constraint.
//

static void create_constraint( gpre_req* request, const act* action, cnstrt* constraint)
{
	for (; constraint; constraint = constraint->cnstrt_next)
	{
		if (constraint->cnstrt_flags & CNSTRT_delete)
			continue;
		put_cstring(request, isc_dyn_rel_constraint, constraint->cnstrt_name->str_string);

        switch (constraint->cnstrt_type)
        {
		case CNSTRT_PRIMARY_KEY:
			request->add_byte(isc_dyn_def_primary_key);
			break;
		case CNSTRT_UNIQUE:
			request->add_byte(isc_dyn_def_unique);
			break;
		case CNSTRT_FOREIGN_KEY:
			request->add_byte(isc_dyn_def_foreign_key);
			break;
		case CNSTRT_NOT_NULL:
			request->add_byte(isc_dyn_fld_not_null);
			request->add_end();
			continue;
		case CNSTRT_CHECK:
			create_check_constraint(request, action, constraint);
			request->add_end();
			continue;
		default:
		    fb_assert(false);
		}

		// stuff a zero-length name, indicating that an index
		// name should be generated

		request->add_word(0);

		if (constraint->cnstrt_type == CNSTRT_FOREIGN_KEY)
		{
			// If <referenced column list> is not specified try to catch
			// them right here
			if (constraint->cnstrt_referred_fields == NULL)
				get_referred_fields(action, constraint);

			if (constraint->cnstrt_fkey_def_type & REF_UPDATE_ACTION)
			{
				request->add_byte(isc_dyn_foreign_key_update);
				switch (constraint->cnstrt_fkey_def_type & REF_UPDATE_MASK)
				{
				case REF_UPD_NONE:
					request->add_byte(isc_dyn_foreign_key_none);
					break;
				case REF_UPD_CASCADE:
					request->add_byte(isc_dyn_foreign_key_cascade);
					create_upd_cascade_trg(request, action, constraint);
					break;
				case REF_UPD_SET_DEFAULT:
					request->add_byte(isc_dyn_foreign_key_default);
					create_set_default_trg(request, action, constraint, true);
					break;
				case REF_UPD_SET_NULL:
					request->add_byte(isc_dyn_foreign_key_null);
					create_set_null_trg(request, action, constraint, true);
					break;
				default:
					// just in case
					fb_assert(0);
					request->add_byte(isc_dyn_foreign_key_none);
					break;
				}
			}
			if (constraint->cnstrt_fkey_def_type & REF_DELETE_ACTION)
			{
				request->add_byte(isc_dyn_foreign_key_delete);
				switch (constraint->cnstrt_fkey_def_type & REF_DELETE_MASK)
				{
				case REF_DEL_NONE:
					request->add_byte(isc_dyn_foreign_key_none);
					break;
				case REF_DEL_CASCADE:
					request->add_byte(isc_dyn_foreign_key_cascade);
					create_del_cascade_trg(request, action, constraint);
					break;
				case REF_DEL_SET_DEFAULT:
					request->add_byte(isc_dyn_foreign_key_default);
					create_set_default_trg(request, action, constraint, false);
					break;
				case REF_DEL_SET_NULL:
					request->add_byte(isc_dyn_foreign_key_null);
					create_set_null_trg(request, action, constraint, false);
					break;
				default:
					// just in case
					fb_assert(0);
					request->add_byte(isc_dyn_foreign_key_none);
					break;
				}
			}
		}
		else
			put_numeric(request, isc_dyn_idx_unique, 1);

		for (const gpre_lls* field = constraint->cnstrt_fields; field; field = field->lls_next)
		{
			const str* string = (STR) field->lls_object;
			put_cstring(request, isc_dyn_fld_name, string->str_string);
		}
		if (constraint->cnstrt_type == CNSTRT_FOREIGN_KEY)
		{
			put_cstring(request, isc_dyn_idx_foreign_key, constraint->cnstrt_referred_rel->str_string);
			for (const gpre_lls* field = constraint->cnstrt_referred_fields; field;
				field = field->lls_next)
			{
				const str* string = (STR) field->lls_object;
				put_cstring(request, isc_dyn_idx_ref_column, string->str_string);
			}
		}
		request->add_end();
	}
}


//____________________________________________________________
//
//		Generate parameter buffer for CREATE DATABASE action.
//

static void create_database( gpre_req* request, const act* action)
{
	const gpre_dbb* db = ((mdbb*) action->act_object)->mdbb_database;
	request->add_byte(isc_dpb_version1);
	request->add_byte(isc_dpb_overwrite);
	request->add_byte(1);
	request->add_byte(0);

	request->add_byte(isc_dpb_sql_dialect);
	request->add_byte(sizeof(int));
	int def_db_dial = 3;
	if (gpreGlob.dialect_specified && (gpreGlob.sw_sql_dialect == 1 || gpreGlob.sw_sql_dialect == 3))
		def_db_dial = gpreGlob.sw_sql_dialect;
	request->add_long(def_db_dial);

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

	SSHORT l;

	if (db->dbb_c_user && !db->dbb_r_user)
	{
		request->add_byte(isc_dpb_user_name);
		l = strlen(db->dbb_c_user);
		request->add_byte(l);
		const char* ch = db->dbb_c_user;
		while (*ch)
			request->add_byte(*ch++);
	}

	if (db->dbb_c_password && !db->dbb_r_password)
	{
		request->add_byte(isc_dpb_password);
		l = strlen(db->dbb_c_password);
		request->add_byte(l);
		const char* ch = db->dbb_c_password;
		while (*ch)
			request->add_byte(*ch++);
	}

	*request->req_blr = 0;
	request->req_length = request->req_blr - request->req_base;
	if (request->req_length == 1)
		request->req_length = 0;
	request->req_blr = request->req_base;
}


//____________________________________________________________
//
//		Generate dynamic DDL for second stage of create database
//

static void create_database_modify_dyn( gpre_req* request, act* action)
{
	gpre_dbb* db = ((mdbb*) action->act_object)->mdbb_database;

	request->add_byte(isc_dyn_mod_database);

	// Reverse the order of files (parser left them backwards)

	gpre_file* next;
	gpre_file* files = NULL;
	for (gpre_file* file = db->dbb_files; file; file = next)
	{
		next = file->fil_next;
		file->fil_next = files;
		files = file;
	}

	SLONG start = db->dbb_length;

	for (const gpre_file* file = files; file != NULL; file = file->fil_next)
	{
		put_cstring(request, isc_dyn_def_file, file->fil_name);
		request->add_byte(isc_dyn_file_start);
		request->add_word(4);
		start = MAX(start, file->fil_start);
		request->add_long(start);
		request->add_byte(isc_dyn_file_length);
		request->add_word(4);
		request->add_long(file->fil_length);
		request->add_end();
		start += file->fil_length;
	}

/*
	if (db->dbb_cache_file)
		add_cache(request, action, db);
*/

	if (db->dbb_def_charset)
		put_cstring(request, isc_dyn_fld_character_set_name, db->dbb_def_charset);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for CREATE DOMAIN action.
//

static void create_domain( gpre_req* request, const act* action)
{
	const gpre_fld* field = (gpre_fld*) action->act_object;

	// add field info

	put_symbol(request, isc_dyn_def_global_fld, field->fld_symbol);
	put_field_attributes(request, field);

	if (field->fld_default_value)
	{
		put_blr(request, isc_dyn_fld_default_value, field->fld_default_value, CME_expr);
		TEXT* default_source = (TEXT*) MSC_alloc(field->fld_default_source->txt_length + 1);
		CPR_get_text(default_source, field->fld_default_source);
		put_cstring(request, isc_dyn_fld_default_source, default_source);
	}

	if (field->fld_constraints)
		create_domain_constraint(request, field->fld_constraints);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dyn for creating a constraints for domains.
//

static void create_domain_constraint(gpre_req* request, const cnstrt* constraint)
{
	for (; constraint; constraint = constraint->cnstrt_next)
	{
		if (constraint->cnstrt_flags & CNSTRT_delete)
			continue;

		// this will be used later
		// put_cstring (request, isc_dyn_rel_constraint, constraint->cnstrt_name->str_string);

		if (constraint->cnstrt_type == CNSTRT_CHECK)
		{
			TEXT* source = (TEXT*) MSC_alloc(constraint->cnstrt_text->txt_length + 1);
			CPR_get_text(source, constraint->cnstrt_text);
			if (source != NULL)
				put_cstring(request, isc_dyn_fld_validation_source, source);
			put_blr(request, isc_dyn_fld_validation_blr, constraint->cnstrt_boolean, CME_expr);
		}
	}
}


//____________________________________________________________
//
//		Generate dynamic DDL for creating a generator.
//

static void create_generator( gpre_req* request, const act* action)
{
	const TEXT* generator_name = (TEXT*) action->act_object;
	put_cstring(request, isc_dyn_def_generator, generator_name);
	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for CREATE INDEX action.
//

static void create_index( gpre_req* request, const gpre_index* index)
{
	if (index->ind_symbol)
		put_symbol(request, isc_dyn_def_idx, index->ind_symbol);
	else
	{
		// An index created because of the UNIQUE constraint on this field.
		put_cstring(request, isc_dyn_def_idx, "");
	}

	put_symbol(request, isc_dyn_rel_name, index->ind_relation->rel_symbol);

	if (index->ind_flags & IND_dup_flag)
		put_numeric(request, isc_dyn_idx_unique, 1);

	if (index->ind_flags & IND_descend)
		put_numeric(request, isc_dyn_idx_type, 1);

	if (index->ind_symbol)
	{
		for (const gpre_fld* field = index->ind_fields; field; field = field->fld_next)
		{
			put_symbol(request, isc_dyn_fld_name, field->fld_symbol);
		}
	}
	else
	{
		// An index created on this one field because of the UNIQUE constraint on this field.
		put_symbol(request, isc_dyn_fld_name, index->ind_fields->fld_symbol);
	}

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for creating a shadow
//

static void create_shadow( gpre_req* request, act* action)
{
	gpre_file* files = (gpre_file*) action->act_object;
	gpre_file* file;

	// Reverse the order of files (parser left them backwards)
	gpre_file* next;
	for (file = files, files = NULL; file; file = next)
	{
		next = file->fil_next;
		file->fil_next = files;
		files = file;
	}
	put_numeric(request, isc_dyn_def_shadow, (SSHORT) files->fil_shadow_number);

	for (file = files; file != NULL; file = file->fil_next)
	{
		put_cstring(request, isc_dyn_def_file, file->fil_name);
		request->add_byte(isc_dyn_file_start);
		request->add_word(4);
		const SLONG start = file->fil_start;
		request->add_long(start);
		request->add_byte(isc_dyn_file_length);
		request->add_word(4);
		request->add_long(file->fil_length);
		if (file->fil_flags & FIL_manual)
			put_numeric(request, isc_dyn_shadow_man_auto, 1);
		if (file->fil_flags & FIL_conditional)
			put_numeric(request, isc_dyn_shadow_conditional, 1);
		request->add_end();
	}
	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for CREATE TABLE action.
//

static void create_table( gpre_req* request, const act* action)
{
	// add relation name

	const gpre_rel* relation = (gpre_rel*) action->act_object;
	put_symbol(request, isc_dyn_def_rel, relation->rel_symbol);

	// If the relation is defined as an external file, add dyn for that

	if (relation->rel_ext_file)
		put_cstring(request, isc_dyn_rel_ext_file, relation->rel_ext_file);

	put_numeric(request, isc_dyn_rel_sql_protection, 1);

	// add field info
	USHORT position = 0;
	for (const gpre_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		if (field->fld_global)
		{
			put_symbol(request, isc_dyn_def_local_fld, field->fld_symbol);
			put_symbol(request, isc_dyn_fld_source, field->fld_global);
		}
		else
			put_symbol(request, isc_dyn_def_sql_fld, field->fld_symbol);

		put_field_attributes(request, field);
		put_numeric(request, isc_dyn_fld_position, position++);

		if (field->fld_default_value)
		{
			put_blr(request, isc_dyn_fld_default_value, field->fld_default_value, CME_expr);
			TEXT* default_source = (TEXT*) MSC_alloc(field->fld_default_source->txt_length + 1);
			CPR_get_text(default_source, field->fld_default_source);
			put_cstring(request, isc_dyn_fld_default_source, default_source);
		}

		request->add_end();
		if (field->fld_constraints)
			create_constraint(request, action, field->fld_constraints);
	}


	if (relation->rel_constraints)
		create_constraint(request, action, relation->rel_constraints);

	request->add_end();

	// Need to create an index for any fields (columns) declared with the UNIQUE constraint.

	for (const gpre_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		if (field->fld_index)
			create_index(request, field->fld_index);
	}
}


//____________________________________________________________
//
//		Generate dynamic DDL for a trigger.
//

static void create_trigger(gpre_req* request,
						   const act* action,
						   gpre_trg* trigger, pfn_local_trigger_cb routine)
{
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	// Name of trigger to be generated

	put_cstring(request, isc_dyn_def_trigger, "");

	put_symbol(request, isc_dyn_rel_name, relation->rel_symbol);
	put_numeric(request, isc_dyn_trg_type, trigger->trg_type);
	put_numeric(request, isc_dyn_trg_sequence, 0);
	put_numeric(request, isc_dyn_trg_inactive, 0);
	request->add_byte(isc_dyn_sql_object);

	if (trigger->trg_source != NULL)
		put_cstring(request, isc_dyn_trg_source, trigger->trg_source->str_string);

	if (trigger->trg_message != NULL)
	{
		put_numeric(request, isc_dyn_def_trigger_msg, 1);
		put_cstring(request, isc_dyn_trg_msg, trigger->trg_message->str_string);
		request->add_byte(isc_dyn_end);
	}

	// Generate the BLR for firing the trigger

	put_trigger_blr(request, isc_dyn_trg_blr, trigger->trg_boolean, routine);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for CREATE VIEW action.
//

static bool create_view(gpre_req* request, act* action)
{
	// add relation name

	gpre_rel* relation = (gpre_rel*) action->act_object;
	put_symbol(request, isc_dyn_def_view, relation->rel_symbol);
	put_numeric(request, isc_dyn_rel_sql_protection, 1);

	// write out blr

	put_blr(request, isc_dyn_view_blr, (gpre_nod*) relation->rel_view_rse, CME_rse);

	// write out view source

	TEXT* view_source = (TEXT *) MSC_alloc(relation->rel_view_text->txt_length + 1);
	CPR_get_text(view_source, relation->rel_view_text);
	put_cstring(request, isc_dyn_view_source, view_source);

	// Write out view context info
	gpre_rel* sub_relation = 0;
	for (gpre_ctx* context = request->req_contexts; context; context = context->ctx_next)
	{
		sub_relation = context->ctx_relation;
		if (!sub_relation)
			continue;
		put_symbol(request, isc_dyn_view_relation, sub_relation->rel_symbol);
		put_numeric(request, isc_dyn_view_context, context->ctx_internal);
		if (context->ctx_symbol)
			put_symbol(request, isc_dyn_view_context_name, context->ctx_symbol);
		//if (context->ctx_type)
		//	put_numeric(request, isc_dyn_view_context_type, context->ctx_type);
		//if (context->ctx_package)
		//	put_symbol(request, isc_dyn_pkg_name, context->ctx_package);
		request->add_end();
	}

	// add the mapping from the rse to the view fields

	gpre_fld* field = relation->rel_fields;
	gpre_nod* fields = relation->rel_view_rse->rse_fields;
	SSHORT position = 0;
	bool non_updateable = false;

	gpre_nod** ptr;
	const gpre_nod* const* end;

	for (ptr = fields->nod_arg, end = ptr + fields->nod_count; ptr < end;
		 ptr++, (field = field ? field->fld_next : NULL))
	{
		const gpre_fld* fld = NULL;
		const gpre_ctx* context = NULL;
		gpre_nod* value = *ptr;
		if (value->nod_type == nod_field)
		{
			const ref* reference = (ref*) value->nod_arg[0];
			fld = reference->ref_field;
			const gpre_req* slice_req;
			const slc* slice;
			if ((value->nod_count >= 2) && (slice_req = (gpre_req*) value->nod_arg[2]) &&
				(slice = slice_req->req_slice))
			{
					CPR_error("Array slices not supported in views");
			}
			context = reference->ref_context;
		}
		const gpre_sym* symbol;
		if (field)
			symbol = field->fld_symbol;
		else if (fld)
			symbol = fld->fld_symbol;
		else
		{
			request->req_length = 0;
			CPR_error("view expression requires field name");
			return false;
		}
		if (fld)
		{
			put_symbol(request, isc_dyn_def_local_fld, symbol);
			put_symbol(request, isc_dyn_fld_base_fld, fld->fld_symbol);
			put_numeric(request, isc_dyn_view_context, context->ctx_internal);
			// ??? context_type, package???
		}
		else
		{
			non_updateable = true;
			put_symbol(request, isc_dyn_def_sql_fld, symbol);
			put_blr(request, isc_dyn_fld_computed_blr, value, CME_expr);
			gpre_fld tmp_field;
			init_field_struct(&tmp_field);
			CME_get_dtype(value, &tmp_field);
			put_dtype(request, &tmp_field);
		}
		put_numeric(request, isc_dyn_fld_position, position++);
		request->add_end();
	}

	if (relation->rel_flags & REL_view_check)
	{
		// VIEW WITH CHECK OPTION
		// Make sure VIEW is updateable
		gpre_rse* select = relation->rel_view_rse;
		if (select->rse_aggregate || non_updateable || select->rse_count != 1)
		{
			CPR_error("Invalid view WITH CHECK OPTION - non-updateable view");
			return false;
		}

		if (!select->rse_boolean)
		{
			CPR_error("Invalid view WITH CHECK OPTION - no WHERE clause");
			return false;
		}

		// For the triggers, the OLD, NEW contexts are reserved

		request->req_internal = 0;
		request->req_contexts = 0;

		gpre_ctx* contexts[3];

		// Make the OLD context for the trigger
		gpre_ctx* context;

		contexts[0] = request->req_contexts = context = MSC_context(request);
		context->ctx_relation = relation;

		// Make the NEW context for the trigger

		contexts[1] = request->req_contexts = context = MSC_context(request);
		context->ctx_relation = relation;

		// Make the context for the  base relation

		contexts[2] = select->rse_context[0] = request->req_contexts =
			context = MSC_context(request);
		context->ctx_relation = sub_relation;

		// Make lists to assign from NEW fields to fields in the base relation.
		// Also make sure rows in base relation correspond to rows in VIEW by
		// making sure values in fields inherited by the VIEW are same as
		// values in base relation.

		field = relation->rel_fields;
		fields = relation->rel_view_rse->rse_fields;
		gpre_nod* and_nod = 0;
		gpre_nod* or_node = 0;
		SSHORT count = 0;
		gpre_lls* stack = NULL;
		for (ptr = fields->nod_arg, end = ptr + fields->nod_count; ptr < end;
			 ptr++, (field = field ? field->fld_next : NULL))
		{
			gpre_fld* fld = NULL;
			gpre_nod* value = *ptr;
			if (value->nod_type == nod_field)
			{
				ref* reference = (ref*) value->nod_arg[0];
				reference->ref_context = contexts[2];
				fld = reference->ref_field;
			}
			ref* view_ref = MSC_reference(0);
			view_ref->ref_context = contexts[0];
			ref* new_view_ref = MSC_reference(0);
			new_view_ref->ref_context = contexts[1];
			new_view_ref->ref_field = view_ref->ref_field = field ? field : fld;
			gpre_nod* view_field = MSC_unary(nod_field, (gpre_nod*) view_ref);
			gpre_nod* new_view_field = MSC_unary(nod_field, (gpre_nod*) new_view_ref);

			gpre_nod* anull_node = MSC_unary(nod_missing, view_field);
			gpre_nod* bnull_node = MSC_unary(nod_missing, value);
			gpre_nod* iand_node = MSC_binary(nod_and, anull_node, bnull_node);

			gpre_nod* eq_nod = MSC_binary(nod_eq, view_field, value);

			or_node = MSC_binary(nod_or, eq_nod, iand_node);

			gpre_nod* set_item = MSC_node(nod_assignment, 2);
			set_item->nod_arg[1] = value;
			set_item->nod_arg[0] = new_view_field;
			MSC_push(set_item, &stack);
			count++;

			if (!or_node)
				or_node = MSC_binary(nod_or, eq_nod, iand_node);
			else if (!and_nod)
				and_nod = MSC_binary(nod_and, or_node, MSC_binary(nod_or, eq_nod, iand_node));
			else
				and_nod = MSC_binary(nod_and, and_nod, MSC_binary(nod_or, eq_nod, iand_node));
		}

		gpre_nod* set_list = MSC_node(nod_list, count);
		ptr = set_list->nod_arg + count;
		while (stack)
			*--ptr = (gpre_nod*) MSC_pop(&stack);

		// Modify the context of fields in boolean to be that of the
		// sub-relation.

		replace_field_names(select->rse_boolean, 0, 0, false, contexts);

		gpre_nod* view_boolean = select->rse_boolean;
		select->rse_boolean = MSC_binary(nod_and, and_nod ? and_nod : or_node, select->rse_boolean);

		// create the UPDATE trigger

		gpre_trg* trigger = (gpre_trg*) MSC_alloc(TRG_LEN);
		trigger->trg_type = PRE_MODIFY_TRIGGER;

		// "update violates CHECK constraint on view"

		trigger->trg_message = NULL;
		trigger->trg_boolean = (gpre_nod*) select;
		create_view_trigger(request, action, trigger, view_boolean, contexts);

		// create the Pre-store trigger

		trigger->trg_type = PRE_STORE_TRIGGER;

		// "insert violates CHECK constraint on view"

		create_view_trigger(request, action, trigger, view_boolean, contexts);
	}

	request->add_end();

	return true;
}


//____________________________________________________________
//
//		Generate dynamic DDL for a trigger.
//

static void create_view_trigger(gpre_req* request,
								const act* action,
								gpre_trg* trigger,
								gpre_nod* view_boolean,
								gpre_ctx** contexts)
{
	const gpre_rel* relation = (gpre_rel*) action->act_object;

	// Name of trigger to be generated

	put_cstring(request, isc_dyn_def_trigger, "");

	put_symbol(request, isc_dyn_rel_name, relation->rel_symbol);
	put_numeric(request, isc_dyn_trg_type, trigger->trg_type);
	put_numeric(request, isc_dyn_trg_sequence, 0);
	request->add_byte(isc_dyn_sql_object);

	if (trigger->trg_source != NULL)
		put_cstring(request, isc_dyn_trg_source, trigger->trg_source->str_string);

	if (trigger->trg_message != NULL)
	{
		put_numeric(request, isc_dyn_def_trigger_msg, 1);
		put_cstring(request, isc_dyn_trg_msg, trigger->trg_message->str_string);
		request->add_byte(isc_dyn_end);
	}

	// Generate the BLR for firing the trigger

	put_view_trigger_blr(request, relation, isc_dyn_trg_blr, trigger, view_boolean, contexts);

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for DECLARE FILTER action.
//

static void declare_filter( gpre_req* request, const act* action)
{
	const gpre_filter* filter = (gpre_filter*) action->act_object;
	put_cstring(request, isc_dyn_def_filter, filter->fltr_name);
	put_numeric(request, isc_dyn_filter_in_subtype, filter->fltr_input_type);
	put_numeric(request, isc_dyn_filter_out_subtype,
				filter->fltr_output_type);
	put_cstring(request, isc_dyn_func_entry_point, filter->fltr_entry_point);

	put_cstring(request, isc_dyn_func_module_name, filter->fltr_module_name);
	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for DECLARE EXTERNAL
//

static void declare_udf( gpre_req* request, const act* action)
{
	const decl_udf* udf_declaration = (decl_udf*) action->act_object;
	const TEXT* udf_name = udf_declaration->decl_udf_name;
	put_cstring(request, isc_dyn_def_function, udf_name);
	put_cstring(request, isc_dyn_func_entry_point, udf_declaration->decl_udf_entry_point);
	put_cstring(request, isc_dyn_func_module_name, udf_declaration->decl_udf_module_name);

	// Reverse the order of arguments which parse left backwords.

	//for (field = udf_declaration->decl_udf_arg_list, udf_declaration->decl_udf_arg_list = NULL; field; field = next)
	//{
	//	next = field->fld_next;
	//	field->fld_next = udf_declaration->decl_udf_arg_list;
	//	udf_declaration->decl_udf_arg_list = field;
	//}

	SSHORT position, blob_position = 0;
	const gpre_fld* field = udf_declaration->decl_udf_return_type;
	if (field)
	{
		// Function returns a value

		// Some data types can not be returned as value
		if (udf_declaration->decl_udf_return_mode == FUN_value)
		{
			switch (field->fld_dtype)
			{
			case dtype_text:
			case dtype_varying:
			case dtype_cstring:
			case dtype_blob:
			case dtype_timestamp:
				CPR_error("return mode by value not allowed for this data type");
			}
		}

		// For functions returning a blob, coerce return argument position to
		// be the last parameter.

		if (field->fld_dtype == dtype_blob)
		{
			blob_position = 1;
			for (const gpre_fld* next = udf_declaration->decl_udf_arg_list; next; next = next->fld_next)
			{
				++blob_position;
			}
			put_numeric(request, isc_dyn_func_return_argument, blob_position);
		}
		else
			put_numeric(request, isc_dyn_func_return_argument, 0);

		position = 0;
	}
	else {
		position = 1;

		// Function modifies an argument whose value is the function return value

		put_numeric(request, isc_dyn_func_return_argument, udf_declaration->decl_udf_return_parameter);
	}

	// Now define all the arguments

	if (!position)
	{
		if (field->fld_dtype == dtype_blob)
		{
			put_numeric(request, isc_dyn_def_function_arg, blob_position);
			put_numeric(request, isc_dyn_func_mechanism, FUN_blob_struct);
		}
		else
		{
			put_numeric(request, isc_dyn_def_function_arg, 0);
			put_numeric(request, isc_dyn_func_mechanism, udf_declaration->decl_udf_return_mode);
		}

		put_cstring(request, isc_dyn_function_name, udf_name);
		put_dtype(request, field);
		request->add_byte(isc_dyn_end);
		position = 1;
	}

	for (field = udf_declaration->decl_udf_arg_list; field; field = field->fld_next)
	{
		if (position > 10)
			CPR_error("External functions can not have more than 10 parameters");
		put_numeric(request, isc_dyn_def_function_arg, position++);

		if (field->fld_dtype == dtype_blob)
			put_numeric(request, isc_dyn_func_mechanism, FUN_blob_struct);
		else
			put_numeric(request, isc_dyn_func_mechanism, FUN_reference);

		put_cstring(request, isc_dyn_function_name, udf_name);
		put_dtype(request, field);
		request->add_byte(isc_dyn_end);
	}

	request->add_end();
}


//____________________________________________________________
//
//		Generate dynamic DDL for GRANT or
//		REVOKE privileges action.
//

static void grant_revoke_privileges( gpre_req* request, const act* action)
{
	TEXT privileges[7];
	TEXT* p = privileges;
	const prv* priv_block = (PRV) action->act_object;

	if (priv_block->prv_privileges & PRV_select)
		*p++ = 'S';

	if (priv_block->prv_privileges & PRV_insert)
		*p++ = 'I';

	if (priv_block->prv_privileges & PRV_delete)
		*p++ = 'D';

	if (priv_block->prv_privileges & PRV_execute)
		*p++ = 'X';

	if (priv_block->prv_privileges & PRV_all)
		*p++ = 'A';

	if (priv_block->prv_privileges & PRV_references)
		*p++ = 'R';

	*p = 0;

	// If there are any select, insert, or delete privileges to be granted
	// or revoked output the necessary DYN strings

	if (p != privileges)
	{
		for (priv_block = (PRV) action->act_object; priv_block; priv_block = priv_block->prv_next)
		{
			if (action->act_type == ACT_dyn_grant)
				put_cstring(request, isc_dyn_grant, privileges);
			else				// action = ACT_dyn_revoke
				put_cstring(request, isc_dyn_revoke, privileges);

			put_cstring(request, priv_block->prv_object_dyn, priv_block->prv_relation->str_string);
			put_cstring(request, priv_block->prv_user_dyn, priv_block->prv_username);

			if (priv_block->prv_privileges & PRV_grant_option)
			{
				put_numeric(request, isc_dyn_grant_options, 1);
			}

			request->add_end();
		}
	}

	// If there are no UPDATE privileges to be granted or revoked, we've finished

	if (!(((PRV) action->act_object)->prv_privileges & PRV_update))
		return;

	p = privileges;
	*p++ = 'U';
	*p = 0;

	for (priv_block = (PRV) action->act_object; priv_block; priv_block = priv_block->prv_next)
	{
		if (priv_block->prv_fields)
		{
			for (const gpre_lls* field = priv_block->prv_fields; field; field = field->lls_next)
			{
				if (action->act_type == ACT_dyn_grant)
					put_cstring(request, isc_dyn_grant, privileges);
				else			// action->act_type == ACT_dyn_revoke
					put_cstring(request, isc_dyn_revoke, privileges);

				put_cstring(request, priv_block->prv_object_dyn, priv_block->prv_relation->str_string);
				const str* string = (STR) field->lls_object;
				put_cstring(request, isc_dyn_fld_name, string->str_string);
				put_cstring(request, priv_block->prv_user_dyn, priv_block->prv_username);

				if (priv_block->prv_privileges & PRV_grant_option)
				{
					put_numeric(request, isc_dyn_grant_options, 1);
				}

				request->add_end();
			}
		}
		else
		{
			// No specific fields mentioned;
			// UPDATE privilege granted or revoked on all fields
			if (action->act_type == ACT_dyn_grant)
				put_cstring(request, isc_dyn_grant, privileges);
			else				// action->act_type == ACT_dyn_revoke
				put_cstring(request, isc_dyn_revoke, privileges);

			put_cstring(request, priv_block->prv_object_dyn, priv_block->prv_relation->str_string);
			put_cstring(request, priv_block->prv_user_dyn, priv_block->prv_username);

			if (priv_block->prv_privileges & PRV_grant_option)
			{
				put_numeric(request, isc_dyn_grant_options, 1);
			}

			request->add_end();
		}
	}
}


//____________________________________________________________
//
//
//

static void init_field_struct( gpre_fld* field)
{
	field->fld_dtype = 0;
	field->fld_length = 0;
	field->fld_scale = 0;
	field->fld_id = 0;
	field->fld_flags = 0;
	field->fld_seg_length = 0;
	field->fld_position = 0;
	field->fld_sub_type = 0;
	field->fld_next = 0;
	field->fld_array = 0;
	field->fld_relation = 0;
	field->fld_procedure = 0;
	field->fld_symbol = 0;
	field->fld_global = 0;
	field->fld_array_info = 0;
	field->fld_default_value = 0;
	field->fld_default_source = 0;
	field->fld_index = 0;
	field->fld_constraints = 0;
	field->fld_character_set = 0;
	field->fld_collate = 0;
	field->fld_computed = 0;
	field->fld_char_length = 0;
	field->fld_charset_id = 0;
	field->fld_collate_id = 0;
}


//____________________________________________________________
//
//		Put dimensions for the array field.
//

static void put_array_info( gpre_req* request, const gpre_fld* field)
{
	const ary* array_info = field->fld_array_info;
	const SSHORT dims = (SSHORT) array_info->ary_dimension_count;
	put_numeric(request, isc_dyn_fld_dimensions, dims);
	for (SSHORT i = 0; i < dims; ++i)
	{
		put_numeric(request, isc_dyn_def_dimension, i);
		request->add_byte(isc_dyn_dim_lower);
		const SLONG lrange = (SLONG) (array_info->ary_rpt[i].ary_lower);
		request->add_word(4);
		request->add_long(lrange);
		request->add_byte(isc_dyn_dim_upper);
		const SLONG urange = (SLONG) (array_info->ary_rpt[i].ary_upper);
		request->add_word(4);
		request->add_long(urange);
		request->add_byte(isc_dyn_end);
	}
}


//____________________________________________________________
//
//		Put an expression expressed in BLR.
//

static void put_blr(gpre_req* request,
					USHORT blr_operator, gpre_nod* node, pfn_local_trigger_cb routine)
{
	request->add_byte(blr_operator);
	const USHORT offset = request->req_blr - request->req_base;
	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);
	(*routine) (node, request);
	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
}


//____________________________________________________________
//
//		Generate dynamic DDL for a computed field.
//

static void put_computed_blr( gpre_req* request, const gpre_fld* field)
{
	//const act* action = request->req_actions;
	//const gpre_rel* relation = (gpre_rel*) action->act_object;

	// Computed field context has to be 0 - so force it

	gpre_ctx* context = request->req_contexts;
	const USHORT save_ctx_int = context->ctx_internal;
	context->ctx_internal = 0;
	request->add_byte(isc_dyn_fld_computed_blr);

	const USHORT offset = request->req_blr - request->req_base;
	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);
	CME_expr(field->fld_computed->cmpf_boolean, request);
	request->add_byte(blr_eoc);

	context->ctx_internal = save_ctx_int;

	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
}


//____________________________________________________________
//
//		Generate dynamic DDL for a computed field.
//

static void put_computed_source( gpre_req* request, const gpre_fld* field)
{
	//const act* action = request->req_actions;
	//const gpre_rel* relation = (gpre_rel*) action->act_object;

	if (field->fld_computed->cmpf_text != NULL)
	{
		TEXT* computed_source = (TEXT*) MSC_alloc(field->fld_computed->cmpf_text->txt_length + 1);
		CPR_get_text(computed_source, field->fld_computed->cmpf_text);
		put_cstring(request, isc_dyn_fld_computed_source, computed_source);
	}
}


//____________________________________________________________
//
//		Put a null-terminated string valued attributed to the output string.
//

static void put_cstring(gpre_req* request, USHORT ddl_operator,
						const TEXT* string)
{
	USHORT length = 0;
	if (string != NULL)
		length = strlen(string);

	put_string(request, ddl_operator, string, length);
}


//____________________________________________________________
//
//		Generate dynamic DDL to create a field for CREATE
//		or ALTER TABLE action.
//

static void put_dtype( gpre_req* request, const gpre_fld* field)
{
	USHORT dtype = 0;

	USHORT length = field->fld_length;
	//const USHORT precision = field->fld_precision;
	//const USHORT sub_type = field->fld_sub_type;
	switch (field->fld_dtype)
	{
	case dtype_cstring:

		// If the user is defining a field as cstring then generate
		// blr_cstring. Currently being used only for defining udf's

		if (field->fld_flags & FLD_meta_cstring)
			dtype = blr_cstring;
		else
		{

			// Correct the length of C string for meta data operations

			if (gpreGlob.sw_cstring && field->fld_sub_type != dsc_text_type_fixed)
				length--;
			dtype = blr_text;
		}

	case dtype_text:
		if (field->fld_dtype == dtype_text)
			dtype = blr_text;
		// Fall into

	case dtype_varying:
		fb_assert(length);
		if (field->fld_dtype == dtype_varying)
			dtype = blr_varying;

		put_numeric(request, isc_dyn_fld_type, dtype);
		put_numeric(request, isc_dyn_fld_length, length);
		put_numeric(request, isc_dyn_fld_scale, 0);

		if (field->fld_sub_type)
			put_numeric(request, isc_dyn_fld_sub_type, field->fld_sub_type);

		if (field->fld_char_length)
			put_numeric(request, isc_dyn_fld_char_length, field->fld_char_length);
		if (field->fld_collate_id)
			put_numeric(request, isc_dyn_fld_collation, field->fld_collate_id);

		if (field->fld_charset_id)
			put_numeric(request, isc_dyn_fld_character_set, field->fld_charset_id);
		return;

	case dtype_short:
		dtype = blr_short;
		length = sizeof(SSHORT);
		if (gpreGlob.sw_server_version >= 6)
		{
			put_numeric(request, isc_dyn_fld_type, dtype);
			put_numeric(request, isc_dyn_fld_length, length);
			put_numeric(request, isc_dyn_fld_scale, field->fld_scale);
			put_numeric(request, isc_dyn_fld_precision, field->fld_precision);
			put_numeric(request, isc_dyn_fld_sub_type, field->fld_sub_type);
			return;
		}
		break;

	case dtype_long:
		dtype = blr_long;
		length = sizeof(SLONG);
		if (gpreGlob.sw_server_version >= 6)
		{
			put_numeric(request, isc_dyn_fld_type, dtype);
			put_numeric(request, isc_dyn_fld_length, length);
			put_numeric(request, isc_dyn_fld_scale, field->fld_scale);
			put_numeric(request, isc_dyn_fld_precision, field->fld_precision);
			put_numeric(request, isc_dyn_fld_sub_type, field->fld_sub_type);
			return;
		}
		break;

	case dtype_blob:
		dtype = blr_blob;
		length = 8;
		put_numeric(request, isc_dyn_fld_type, dtype);
		put_numeric(request, isc_dyn_fld_length, length);
		put_numeric(request, isc_dyn_fld_scale, 0);
		put_numeric(request, isc_dyn_fld_sub_type, field->fld_sub_type);
		put_numeric(request, isc_dyn_fld_segment_length, field->fld_seg_length);
		if (field->fld_sub_type == isc_blob_text && field->fld_charset_id)
			put_numeric(request, isc_dyn_fld_character_set, field->fld_charset_id);
		return;

	case dtype_quad:
		dtype = blr_quad;
		length = 8;
		break;

	case dtype_real:
		dtype = blr_float;
		length = sizeof(float);
		break;

	case dtype_double:
		dtype = blr_double;
		length = sizeof(double);
		break;

	// Begin sql date/time/timestamp
	case dtype_sql_date:
		dtype = blr_sql_date;
		length = sizeof(ISC_DATE);
		break;

	case dtype_sql_time:
		dtype = blr_sql_time;
		length = sizeof(ISC_TIME);
		break;

	case dtype_timestamp:
		dtype = blr_timestamp;
		length = sizeof(ISC_TIMESTAMP);
		break;
	// End sql date/time/timestamp

	case dtype_int64:
		dtype = blr_int64;
		length = sizeof(ISC_INT64);
		put_numeric(request, isc_dyn_fld_type, dtype);
		put_numeric(request, isc_dyn_fld_length, length);
		put_numeric(request, isc_dyn_fld_scale, field->fld_scale);
		put_numeric(request, isc_dyn_fld_precision, field->fld_precision);
		put_numeric(request, isc_dyn_fld_sub_type, field->fld_sub_type);
		return;

	default:
		CPR_bugcheck(" *** Unknown datatype in put_dtype *** ");
		break;
	}

	put_numeric(request, isc_dyn_fld_type, dtype);
	put_numeric(request, isc_dyn_fld_length, length);
	put_numeric(request, isc_dyn_fld_scale, field->fld_scale);

}


//____________________________________________________________
//
//		Generate dynamic DDL to create a field for CREATE
//		or ALTER TABLE action.
//
//		Emit the DDL that is appopriate for a local instance
//		of a field.  eg: that can vary from the local fields
//		global field (DOMAIN in SQL).
//

static void put_field_attributes( gpre_req* request, const gpre_fld* field)
{

	if (field->fld_flags & FLD_computed)
		put_computed_blr(request, field);

	if (!field->fld_global)
		put_dtype(request, field);

	// Put the DDL for local field instances

	if (field->fld_array_info)
		put_array_info(request, field);

	if (field->fld_collate && field->fld_global) {
		put_numeric(request, isc_dyn_fld_collation, field->fld_collate->intlsym_collate_id);
	}

	if (field->fld_flags & FLD_not_null) {
		request->add_byte(isc_dyn_fld_not_null);
	}

	if (field->fld_flags & FLD_computed)
		put_computed_source(request, field);
}


//____________________________________________________________
//
//		Put a numeric valued attributed to the output string.
//

static void put_numeric( gpre_req* request, USHORT blr_operator, SSHORT number)
{

	request->add_byte(blr_operator);
	request->add_word(2);
	request->add_word(number);
}


//____________________________________________________________
//
//		Put a counted string valued attributed to the output string.
//		Count value is BYTE instead of WORD like put_cstring & put_string
//

static void put_short_cstring(gpre_req* request, USHORT ddl_operator, const TEXT* string)
{
	SSHORT length = 0;
	if (string != NULL)
		length = strlen(string);

	STUFF_CHECK(request, length);

	request->add_byte(ddl_operator);
	request->add_byte(length);

	if (string != NULL)
	{
		while (length--)
			request->add_byte(*string++);
	}
}


//____________________________________________________________
//
//		Put a counted string valued attributed to the output string.
//

static void put_string(gpre_req* request, USHORT ddl_operator, const TEXT* string, USHORT length)
{

	STUFF_CHECK(request, length);

	if (ddl_operator)
	{
		request->add_byte(ddl_operator);
		request->add_word(length);
	}
	else
		request->add_byte(length);


	if (string != NULL)
	{
		while (length--)
			request->add_byte(*string++);
	}
}


//____________________________________________________________
//
//		Put a symbol valued attribute.
//

static void put_symbol(gpre_req* request, int ddl_operator, const gpre_sym* symbol)
{
	put_cstring(request, (USHORT) ddl_operator, symbol->sym_string);
}


//____________________________________________________________
//
//		Generate BLR for a trigger whose action is to abort.
//		Abort with a gds_error code error.
//

static void put_trigger_blr(gpre_req* request,
							USHORT blr_operator,
							gpre_nod* node, pfn_local_trigger_cb routine)
{
	request->add_byte(blr_operator);
	const USHORT offset = request->req_blr - request->req_base;
	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);
	request->add_byte(blr_begin);
	request->add_byte(blr_if);
	(*routine) (node, request);
	request->add_byte(blr_begin);
	request->add_byte(blr_end);

	// Generate the action for the trigger to be abort

	request->add_byte(blr_abort);
	put_short_cstring(request, blr_gds_code, "check_constraint");
	request->add_byte(blr_end);				// for if
	request->add_byte(blr_eoc);
	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
}


//____________________________________________________________
//
//		Generate BLR for a trigger for a VIEW WITH CHECK OPTION.
//		This is messy, the gpre_rse passed in is mutilated by the end.
//		Fields in the where clause and in the VIEW definition, are replaced
//		by the VIEW fields.
//		For fields in the where clause but not in the VIEW definition,
//		the fields are set to NULL node for the PRE STORE trigger.
//
//

static void put_view_trigger_blr(gpre_req* request,
								 const gpre_rel* relation,
								 USHORT blr_operator,
								 gpre_trg* trigger,
								 gpre_nod* view_boolean, gpre_ctx** contexts)
{
	gpre_rse* node = (gpre_rse*) trigger->trg_boolean;
	request->add_byte(blr_operator);
	const USHORT offset = request->req_blr - request->req_base;
	request->add_word(0);
	if (request->req_flags & REQ_blr_version4)
		request->add_byte(blr_version4);
	else
		request->add_byte(blr_version5);
	request->add_byte(blr_begin);

	if (trigger->trg_type == PRE_MODIFY_TRIGGER)
	{
		request->add_byte(blr_for);
		CME_rse(node, request);

		// For the boolean, replace all fields in the rse and the view, with the
		// equivalent view field_name, for remaining fields in rse, leave them
		// alone.

		replace_field_names(view_boolean, node->rse_fields, relation->rel_fields, false, contexts);

		request->add_byte(blr_begin);
		request->add_byte(blr_if);
		CME_expr(view_boolean, request);

		request->add_byte(blr_begin);
		request->add_byte(blr_end);

		// Generate the action for the trigger to be abort

		request->add_byte(blr_abort);
		put_short_cstring(request, blr_gds_code, "check_constraint");
		request->add_byte(blr_end);
		request->add_byte(blr_end);
		request->add_byte(blr_eoc);
	}							// end of PRE_MODIFY_TRIGGER trigger

	if (trigger->trg_type == PRE_STORE_TRIGGER)
	{
		replace_field_names(view_boolean, node->rse_fields, relation->rel_fields, true, contexts);

		request->add_byte(blr_if);
		CME_expr(view_boolean, request);

		request->add_byte(blr_begin);
		request->add_byte(blr_end);

		// Generate the action for the trigger to be abort

		request->add_byte(blr_abort);
		put_short_cstring(request, blr_gds_code, "check_constraint");
		request->add_byte(blr_end);
		request->add_byte(blr_eoc);
	}							// end of PRE_STORE_TRIGGER trigger

	const USHORT length = request->req_blr - request->req_base - offset - 2;
	request->req_base[offset] = (UCHAR) length;
	request->req_base[offset + 1] = (UCHAR) (length >> 8);
}


//____________________________________________________________
//
//		Replace fields in given rse by fields referenced in VIEW.
//		if fields in gpre_rse are not part of VIEW definition, then they
//		are not changed.
//		If search list is not specified, then only the context of the fields
//		in the rse is chaged to contexts[2].
//		If null_them is true, then fields in rse but not in VIEW definition
//		are converted to null nodes, this is used for PRE STORE trigger
//		verification.
//

static void replace_field_names(gpre_nod* const input,
								const gpre_nod* const search_list,
								gpre_fld* const replace_with,
								bool null_them,
								gpre_ctx** contexts)
{
	if (!input)
		return;

	if ((input->nod_type == nod_via || input->nod_type == nod_any || input->nod_type == nod_unique) &&
		search_list == 0 && replace_with == 0 && input->nod_count == 0)
	{
		CPR_error("Invalid view WITH CHECK OPTION - no subqueries permitted");
		return;
	}

	gpre_nod** ptr = input->nod_arg;
	for (const gpre_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		if ((*ptr)->nod_type == nod_field)
		{
			ref* reference = (ref*) (*ptr)->nod_arg[0];
			gpre_fld* rse_field = reference->ref_field;
			if (null_them)
			{
				if (reference->ref_context == contexts[2]) {
					*ptr = MSC_node(nod_null, 0);
				}
				continue;
			}
			reference->ref_context = contexts[2];
			if (!search_list)
				continue;
			gpre_fld* view_field = replace_with;
			gpre_nod* const* ptrs = search_list->nod_arg;
			for (gpre_nod* const* const ends = ptrs + search_list->nod_count;
				ptrs < ends;
				ptrs++, view_field = view_field ? view_field->fld_next : NULL)
			{
				const ref* references = (ref*) (*ptrs)->nod_arg[0];
				gpre_fld* select_field = references->ref_field;
				if (rse_field == select_field)
				{
					reference->ref_field = view_field ? view_field : select_field;
					reference->ref_context = contexts[1];
					break;
				}
			}
		}
		else
			replace_field_names(*ptr, search_list, replace_with, null_them, contexts);
	}
}


//____________________________________________________________
//
//		Generate dynamic DDL for a set statistics
//

static void set_statistics( gpre_req* request, const act* action)
{
	const sts* stats = (STS) action->act_object;
	if (stats)
		if (stats->sts_flags & STS_index)
		{
			put_cstring(request, isc_dyn_mod_idx, stats->sts_name->str_string);
			request->add_byte(isc_dyn_idx_statistic);
		}

	request->add_end();
}

