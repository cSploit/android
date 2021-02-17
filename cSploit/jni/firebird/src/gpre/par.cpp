//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		par.cpp
//		DESCRIPTION:	Parser
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

//  Revision 1.2  2000/11/27 09:26:13  fsg
//  Fixed bugs in gpre to handle PYXIS forms
//  and allow edit.e and fred.e to go through
//  gpre without errors (and correct result).
//
//  This is a partial fix until all
//  PYXIS datatypes are adjusted in frm_trn.c
//
//  removed some compiler warnings too
//
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings
//  TMN (Mike Nordell) APR-MAY.2001 - Conversion to C++
//  SWB (Stepen Boyd) 2007/03/21 - Supressed parsing of QLI keywords if -noqli
//                                 switch given on the command line.
//
//
//____________________________________________________________
//
//

#include "firebird.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/exp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "../gpre/par_proto.h"
#include "../gpre/sql_proto.h"
#include "../common/utils_proto.h"

#ifdef FTN_BLK_DATA
static void		block_data_list(const gpre_dbb*);
#endif
static bool		match_parentheses();
static act*		par_any();
static act*		par_array_element();
static act*		par_at();
static act*		par_based();
static act*		par_begin();
static blb*		par_blob();
static act*		par_blob_action(act_t);
static act*		par_blob_field();
static act*		par_case();
static act*		par_clear_handles();
static act*		par_derived_from();
static act*		par_end_block();
static act*		par_end_error();
static act*		par_end_fetch();
static act*		par_end_for();
static act*		par_end_modify();
static act*		par_end_stream();
static act*		par_end_store(bool);
static act*		par_erase();
static act*		par_fetch();
static act*		par_finish();
static act*		par_for();
static act*		par_function();
static act*		par_left_brace();
static act*		par_modify();
static act*		par_on();
static act*		par_on_error();
static act*		par_open_blob(act_t, gpre_sym*);
static bool		par_options(gpre_req*, bool);
static act*		par_procedure();
static act*		par_ready();
static act*		par_returning_values();
static act*		par_right_brace();
static act*		par_release();
static act*		par_slice(act_t);
static act*		par_store();
static act*		par_start_stream();
static act*		par_start_transaction();
static act*		par_subroutine();
static act*		par_trans(act_t);
static act*		par_type();
static act*		par_variable();
static act*		scan_routine_header();
static void		set_external_flag();
static bool		terminator();

static int		brace_count;
static int		namespace_count;
static bool		routine_decl;
static act*		cur_statement;
static act*		cur_item;
static gpre_lls*	cur_for;
static gpre_lls*	cur_store;
static gpre_lls*	cur_fetch;
static gpre_lls*	cur_modify;
static gpre_lls*	cur_error;
static gpre_lls*	routine_stack;
static gpre_fld*	flag_field;


//____________________________________________________________
//
//		We have a token with a symbolic meaning.  If appropriate,
//		parse an action segment.  If not, return NULL.
//

act* PAR_action(const TEXT* base_dir)
{
	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;

	if (!symbol)
	{
		cur_statement = NULL;
		return NULL;
	}

	if (gpreGlob.token_global.tok_keyword >= KW_start_actions &&
		gpreGlob.token_global.tok_keyword <= KW_end_actions)
	{
		const kwwords_t keyword = gpreGlob.token_global.tok_keyword;
		if (! gpreGlob.sw_no_qli)
		{
			switch (keyword)
			{
			case KW_READY:
			case KW_START_TRANSACTION:
			case KW_FINISH:
			case KW_COMMIT:
			case KW_PREPARE:
			case KW_RELEASE_REQUESTS:
			case KW_ROLLBACK:
			case KW_FUNCTION:
			case KW_SAVE:
			case KW_SUB:
			case KW_SUBROUTINE:
				CPR_eol_token();
				break;

			case KW_EXTERNAL:
				set_external_flag();
				return NULL;

			case KW_FOR:
				// Get the next token as it is without upcasing
				gpreGlob.override_case = true;
				CPR_token();
				break;

			default:
				CPR_token();
			}
		}
		else
			CPR_token();

		try {

			if (! gpreGlob.sw_no_qli)
			{
				switch (keyword)
				{
				case KW_INT:
				case KW_LONG:
				case KW_SHORT:
				case KW_CHAR:
				case KW_FLOAT:
				case KW_DOUBLE:
					//par_var_c(keyword);
					return NULL;

				case KW_ANY:
					return par_any();
				case KW_AT:
					return par_at();
				case KW_BASED:
					return par_based();
				case KW_CLEAR_HANDLES:
					return par_clear_handles();
				case KW_DATABASE:
					return PAR_database(false, base_dir);
				case KW_DERIVED_FROM:
					return par_derived_from();
				case KW_END_ERROR:
					return par_end_error();
				case KW_END_FOR:
					return cur_statement = par_end_for();
				case KW_END_MODIFY:
					return cur_statement = par_end_modify();
				case KW_END_STREAM:
					return cur_statement = par_end_stream();
				case KW_END_STORE:
					return cur_statement = par_end_store(false);
				case KW_END_STORE_SPECIAL:
					return cur_statement = par_end_store(true);
				case KW_ELEMENT:
					return par_array_element();
				case KW_ERASE:
					return cur_statement = par_erase();
				case KW_EVENT_INIT:
					return cur_statement = PAR_event_init(false);
				case KW_EVENT_WAIT:
					return cur_statement = PAR_event_wait(false);
				case KW_FETCH:
					return cur_statement = par_fetch();
				case KW_FINISH:
					return cur_statement = par_finish();
				case KW_FOR:
					return par_for();
				case KW_END_FETCH:
					return cur_statement = par_end_fetch();
				case KW_MODIFY:
					return par_modify();
				case KW_ON:
					return par_on();
				case KW_ON_ERROR:
					return par_on_error();
				case KW_READY:
					return cur_statement = par_ready();
				case KW_RELEASE_REQUESTS:
					return cur_statement = par_release();
				case KW_RETURNING:
					return par_returning_values();
				case KW_START_STREAM:
					return cur_statement = par_start_stream();
				case KW_STORE:
					return par_store();
				case KW_START_TRANSACTION:
					return cur_statement = par_start_transaction();
				case KW_FUNCTION:
					return par_function();
				case KW_PROCEDURE:
					return par_procedure();

				case KW_PROC:
					break;

				case KW_SUBROUTINE:
					return par_subroutine();
				case KW_SUB:
					break;

				case KW_OPEN_BLOB:
					return cur_statement = par_open_blob(ACT_blob_open, 0);
				case KW_CREATE_BLOB:
					return cur_statement = par_open_blob(ACT_blob_create, 0);
				case KW_GET_SLICE:
					return cur_statement = par_slice(ACT_get_slice);
				case KW_PUT_SLICE:
					return cur_statement = par_slice(ACT_put_slice);
				case KW_GET_SEGMENT:
					return cur_statement = par_blob_action(ACT_get_segment);
				case KW_PUT_SEGMENT:
					return cur_statement = par_blob_action(ACT_put_segment);
				case KW_CLOSE_BLOB:
					return cur_statement = par_blob_action(ACT_blob_close);
				case KW_CANCEL_BLOB:
					return cur_statement = par_blob_action(ACT_blob_cancel);

				case KW_COMMIT:
					return cur_statement = par_trans(ACT_commit);
				case KW_SAVE:
					return cur_statement = par_trans(ACT_commit_retain_context);
				case KW_ROLLBACK:
					return cur_statement = par_trans(ACT_rollback);
				case KW_PREPARE:
					return cur_statement = par_trans(ACT_prepare);

				case KW_NAMESPACE:
					if (gpreGlob.sw_language == lang_internal || gpreGlob.sw_language == lang_cxx ||
						gpreGlob.sw_language == lang_cplusplus)
					{
						if (CPR_token())
						{
							if (gpreGlob.token_global.tok_keyword == KW_L_BRACE)
							{
								CPR_token();
								++namespace_count;
								++brace_count;
								return NULL;
							}
						}
					}
					break;

				case KW_L_BRACE:
					return par_left_brace();
				case KW_R_BRACE:
					return par_right_brace();
				case KW_END:
					return par_end_block();
				case KW_BEGIN:
					return par_begin();
				case KW_CASE:
					return par_case();

				case KW_EXEC:
					{
						if (!MSC_match(KW_SQL))
							break;
						gpreGlob.sw_sql = true;
						act* action = SQL_action(base_dir);
						gpreGlob.sw_sql = false;
						return action;
					}

				default:
					break;
				}
			}
			else
			{
				switch (keyword)
				{
				case KW_BASED:
					return par_based();

				case KW_EXEC:
					{
						if (!MSC_match(KW_SQL))
							break;
						gpreGlob.sw_sql = true;
						act* action = SQL_action(base_dir);
						gpreGlob.sw_sql = false;
						return action;
					}

				default:
					break;
				}
			}

		}	// try
		catch (const gpre_exception&) {
			throw;
		}
		catch (const Firebird::fatal_exception&)
		{
			// CVC: a fatal exception should be propagated.
			// For example, a failure in our runtime.
			throw;
		}
		catch (const Firebird::Exception&)
		{
			gpreGlob.sw_sql = false;
			// This is to force GPRE to get the next symbol. Fix for bug #274. DROOT
			gpreGlob.token_global.tok_symbol = NULL;
			return NULL;
		}

		cur_statement = NULL;
		return NULL;
	}

	if (! gpreGlob.sw_no_qli)
	{
		for (; symbol; symbol = symbol->sym_homonym)
		{
			switch (symbol->sym_type)
			{
			case SYM_context:
				try {
					cur_statement = NULL;
					return par_variable();
				}
				catch (const gpre_exception&) {
					throw;
				}
				catch (const Firebird::fatal_exception&)
				{
					// CVC: a fatal exception should be propagated.
					throw;
				}
				catch (const Firebird::Exception&) {
					return 0;
				}
			case SYM_blob:
				try {
					cur_statement = NULL;
					return par_blob_field();
				}
				catch (const gpre_exception&) {
					throw;
				}
				catch (const Firebird::fatal_exception&)
				{
					// CVC: a fatal exception should be propagated.
					throw;
				}
				catch (const Firebird::Exception&) {
					return 0;
				}
			case SYM_relation:
				try {
					cur_statement = NULL;
					return par_type();
				}
				catch (const gpre_exception&) {
					throw;
				}
				catch (const Firebird::fatal_exception&)
				{
					// CVC: a fatal exception should be propagated.
					throw;
				}
				catch (const Firebird::Exception&) {
					return 0;
				}
			default:
				break;
			}
		}
	}

	cur_statement = NULL;
	CPR_token();

	return NULL;
}


//____________________________________________________________
//
//		Parse a blob subtype -- either a signed number or a symbolic name.
//

SSHORT PAR_blob_subtype(gpre_dbb* db)
{
	// Check for symbol type name

	if (gpreGlob.token_global.tok_type == tok_ident)
	{
		gpre_fld* field = NULL;
		gpre_rel* relation = MET_get_relation(db, "RDB$FIELDS", "");
		if (!relation || !(field = MET_field(relation, "RDB$FIELD_SUB_TYPE")))
		{
			PAR_error("error during BLOB SUB_TYPE lookup");
		}

		SSHORT const_subtype;
		if (!MET_type(field, gpreGlob.token_global.tok_string, &const_subtype))
			CPR_s_error("blob sub_type");
		PAR_get_token();
		return const_subtype;
	}

	return EXP_SSHORT_ordinal(true);
}


//____________________________________________________________
//
//		Parse a DATABASE declaration.  If successful, return
//		an action block.
//

act* PAR_database(bool sql, const TEXT* base_directory)
{
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Database alias too long");

	TEXT s[MAXPATHLEN << 1];

	act* action = MSC_action(0, ACT_database);
	gpre_dbb* db = (gpre_dbb*) MSC_alloc(DBB_LEN);

	// Get handle name token, make symbol for handle, and
	// insert symbol into hash table

	gpre_sym* symbol = PAR_symbol(SYM_dummy);
	db->dbb_name = symbol;
	symbol->sym_type = SYM_database;
	symbol->sym_object = (gpre_ctx*) db;

	if (!MSC_match(KW_EQUALS))
		CPR_s_error("\"=\" in database declaration");

	if (MSC_match(KW_STATIC))
		db->dbb_scope = DBB_STATIC;
	else if (MSC_match(KW_EXTERN))
		db->dbb_scope = DBB_EXTERN;

	MSC_match(KW_COMPILETIME);

	// parse the compiletime options
	TEXT* string;

	for (;;)
	{
		if (MSC_match(KW_FILENAME) && !isQuoted(gpreGlob.token_global.tok_type))
			CPR_s_error("quoted file name");

		tok& token = gpreGlob.token_global;

		if (isQuoted(token.tok_type))
		{
			if (base_directory)
			{
				db->dbb_filename = string =
					(TEXT*) MSC_alloc(token.tok_length + strlen(base_directory) + 1);
				MSC_copy_cat(base_directory, strlen(base_directory),
						 token.tok_string, token.tok_length, string);
			}
			else
			{
				db->dbb_filename = string = (TEXT *) MSC_alloc(token.tok_length + 1);
				MSC_copy(token.tok_string, token.tok_length, string);
			}
			token.tok_length += 2;
		}
		else if (MSC_match(KW_PASSWORD))
		{
			if (!isQuoted(token.tok_type))
				CPR_s_error("quoted password");
			db->dbb_c_password = string = (TEXT *) MSC_alloc(token.tok_length + 1);
			MSC_copy(token.tok_string, token.tok_length, string);
		}
		else if (MSC_match(KW_USER))
		{
			if (!isQuoted(token.tok_type))
				CPR_s_error("quoted user name");
			db->dbb_c_user = string = (TEXT *) MSC_alloc(token.tok_length + 1);
			MSC_copy(token.tok_string, token.tok_length, string);
		}
		else if (MSC_match(KW_LC_MESSAGES))
		{
			if (!isQuoted(token.tok_type))
				CPR_s_error("quoted language name");
			db->dbb_c_lc_messages = string = (TEXT *) MSC_alloc(token.tok_length + 1);
			MSC_copy(token.tok_string, token.tok_length, string);
		}
		else if (!sql && MSC_match(KW_LC_CTYPE))
		{
			if (!isQuoted(token.tok_type))
				CPR_s_error("quoted character set name");
			db->dbb_c_lc_ctype = string = (TEXT *) MSC_alloc(token.tok_length + 1);
			MSC_copy(token.tok_string, token.tok_length, string);
		}
		else
			break;

		PAR_get_token();
	}

	if ((gpreGlob.sw_auto) && (db->dbb_c_password || db->dbb_c_user || db->dbb_c_lc_ctype ||
		db->dbb_c_lc_messages))
	{
			CPR_warn("PASSWORD, USER and NAMES options require -manual switch to gpre.");
	}

	if (!db->dbb_filename)
		CPR_s_error("quoted file name");

	if (gpreGlob.default_user && !db->dbb_c_user)
		db->dbb_c_user = gpreGlob.default_user;

	if (gpreGlob.default_password && !db->dbb_c_password)
		db->dbb_c_password = gpreGlob.default_password;

	if (gpreGlob.module_lc_ctype && !db->dbb_c_lc_ctype)
		db->dbb_c_lc_ctype = gpreGlob.module_lc_ctype;

	if (gpreGlob.default_lc_ctype && !db->dbb_c_lc_ctype)
		db->dbb_c_lc_ctype = gpreGlob.default_lc_ctype;

	// parse the runtime options

	if (MSC_match(KW_RUNTIME))
	{
		if (MSC_match(KW_FILENAME))
			db->dbb_runtime = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
		else if (MSC_match(KW_PASSWORD))
			db->dbb_r_password = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
		else if (MSC_match(KW_USER))
			db->dbb_r_user = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
		else if (MSC_match(KW_LC_MESSAGES))
			db->dbb_r_lc_messages = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
		else if (!sql && MSC_match(KW_LC_CTYPE))
			db->dbb_r_lc_ctype = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
		else
			db->dbb_runtime = sql ? SQL_var_or_string(false) : PAR_native_value(false, false);
	}

#ifdef GPRE_ADA
	if ((gpreGlob.sw_language == lang_ada) && (gpreGlob.token_global.tok_keyword == KW_HANDLES))
	{
		PAR_get_token();
		if (isQuoted(gpreGlob.token_global.tok_type))
			CPR_s_error("quoted file name");
		int len = gpreGlob.token_global.tok_length;
		if (len > MAXPATHLEN - 2)
		    len = MAXPATHLEN - 2;
		MSC_copy(gpreGlob.token_global.tok_string, len, s);
		s[len] = 0;
		strcat(s, ".");
		if (!gpreGlob.ada_package[0] || !strcmp(gpreGlob.ada_package, s))
		{
			strncpy(gpreGlob.ada_package, s, MAXPATHLEN);
			gpreGlob.ada_package[MAXPATHLEN - 1] = 0;
		}
		else
		{
			fb_utils::snprintf(s, sizeof(s),
					"Ada handle package \"%s\" already in use, ignoring package %s",
					gpreGlob.ada_package, gpreGlob.token_global.tok_string);
			CPR_warn(s);
		}
		PAR_get_token();
	}
#endif

	if (gpreGlob.sw_language != lang_fortran)
		MSC_match(KW_SEMI_COLON);

	bool found_error = false;
	try {
		if (!MET_database(db, true))
		    found_error = true;
	}
	// CVC: It avoids countless errors if the db can't be loaded.
	catch (const Firebird::Exception& exc)
	{
		found_error = true;
		// CVC: Print the low level error. The lack of this caused me a lot of problems.
		// Granted, status_exception doesn't help, but fatal_exception carries a
		// meaningful message.
		CPR_error(exc.what());
	}

	if (found_error)
	{
		fb_utils::snprintf(s, sizeof(s), "Couldn't access database %s = '%s'",
				db->dbb_name->sym_string, db->dbb_filename);
		CPR_error(s);
		CPR_abort();
	}

	db->dbb_next = gpreGlob.isc_databases;
	gpreGlob.isc_databases = db;
	HSH_insert(symbol);

	// Load up the symbol (hash) table with relation names from this databases.
	MET_load_hash_table(db);

#ifdef FTN_BLK_DATA
	if ((gpreGlob.sw_language == lang_fortran) && (db->dbb_scope != DBB_EXTERN))
		block_data_list(db);
#endif

	// Since we have a real DATABASE statement, get rid of any artificial
	// databases that were created because of an INCLUDE SQLCA statement.

	for (gpre_dbb** db_ptr = &gpreGlob.isc_databases; *db_ptr;)
	{
		if ((*db_ptr)->dbb_flags & DBB_sqlca)
			*db_ptr = (*db_ptr)->dbb_next;
		else
			db_ptr = &(*db_ptr)->dbb_next;
	}

	return action;
}


//____________________________________________________________
//
//		Parse end of statement.  All languages except ADA leave
//		the trailing semi-colon dangling.   ADA, however, must
//		eat the semi-colon as part of the statement.  In any case,
//		return true is a semi-colon is/was there, otherwise return false.
//

bool PAR_end()
{
	if ((gpreGlob.sw_language == lang_ada) || (gpreGlob.sw_language == lang_c) ||
		(isLangCpp(gpreGlob.sw_language))
#ifdef GPRE_COBOL
		 ||
		(gpreGlob.sw_language == lang_cobol && isAnsiCobol(gpreGlob.sw_cob_dialect))
#endif
		)
	{
		return (MSC_match(KW_SEMI_COLON));
	}

	return (gpreGlob.token_global.tok_keyword == KW_SEMI_COLON);
}


//____________________________________________________________
//
//		Report an error during parse and unwind.
//

void PAR_error(const TEXT* string)
{
	CPR_error(string);
	PAR_unwind();
}


//____________________________________________________________
//
//		Parse an event init statement, preparing
//		to wait on a number of named gpreGlob.events.
//

act* PAR_event_init(bool sql)
{
	//char req_name[128];

	// make up statement node

	SQL_resolve_identifier("<identifier>", NULL, MAX_EVENT_SIZE);
	//SQL_resolve_identifier("<identifier>", req_name, sizeof(req_name));
	//strcpy(gpreGlob.token_global.tok_string, req_name); Why? It's already done.
	gpre_nod* init = MSC_node(nod_event_init, 4);
	init->nod_arg[0] = (gpre_nod*) PAR_symbol(SYM_dummy);
	init->nod_arg[3] = (gpre_nod*) gpreGlob.isc_databases;

	act* action = MSC_action(0, ACT_event_init);
	action->act_object = (ref*) init;

	// parse optional database handle

	if (!MSC_match(KW_LEFT_PAREN))
	{
		gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
		if (symbol && (symbol->sym_type == SYM_database))
			init->nod_arg[3] = (gpre_nod*) symbol->sym_object;
		else
			CPR_s_error("left parenthesis or database handle");

		PAR_get_token();
		if (!MSC_match(KW_LEFT_PAREN))
			CPR_s_error("left parenthesis");
	}

	// eat any number of event strings until a right paren is found,
	// pushing the gpreGlob.events onto a stack

	gpre_nod* node;
	gpre_lls* stack = NULL;
	int count = 0;

	while (true)
	{
		if (MSC_match(KW_RIGHT_PAREN))
			break;

		const gpre_sym* symbol;
		if (!sql && (symbol = gpreGlob.token_global.tok_symbol) && symbol->sym_type == SYM_context)
		{
			gpre_ctx* context;
			gpre_fld* field = EXP_field(&context);
			ref* reference = EXP_post_field(field, context, false);
			node = MSC_node(nod_field, 1);
			node->nod_arg[0] = (gpre_nod*) reference;
		}
		else
		{
			node = MSC_node(nod_null, 1);
			if (sql)
				node->nod_arg[0] = (gpre_nod*) SQL_var_or_string(false);
			else
				node->nod_arg[0] = (gpre_nod*) PAR_native_value(false, false);
		}
		MSC_push(node, &stack);
		count++;

		MSC_match(KW_COMMA);
	}

	// pop the event strings off the stack

	gpre_nod* event_list = init->nod_arg[1] = MSC_node(nod_list, (SSHORT) count);
	gpre_nod** ptr = event_list->nod_arg + count;
	while (stack)
		*--ptr = (gpre_nod*) MSC_pop(&stack);

	MSC_push((gpre_nod*) action, &gpreGlob.events);

	if (!sql)
		PAR_end();
	return action;
}


//____________________________________________________________
//
//		Parse an event wait statement, preparing
//		to wait on a number of named gpreGlob.events.
//

act* PAR_event_wait(bool sql)
{
	//char req_name[132];

	// this is a simple statement, just add a handle

	act* action = MSC_action(0, ACT_event_wait);
	SQL_resolve_identifier("<identifier>", NULL, MAX_EVENT_SIZE);
	//SQL_resolve_identifier("<identifier>", req_name, sizeof(req_name));
	//strcpy(gpreGlob.token_global.tok_string, req_name); redundant
	action->act_object = (ref*) PAR_symbol(SYM_dummy);
	if (!sql)
		PAR_end();

	return action;
}


//____________________________________________________________
//
//		Perform any last minute stuff necessary at the end of pass1.
//

void PAR_fini()
{
	if (cur_for)
		CPR_error("unterminated FOR statement");

	if (cur_modify)
		CPR_error("unterminated MODIFY statement");

	if (cur_store)
		CPR_error("unterminated STORE statement");

	if (cur_error)
		CPR_error("unterminated ON_ERROR clause");

	if (cur_item)
		CPR_error("unterminated ITEM statement");
}


//____________________________________________________________
//
//		Get a token or unwind the parse
//		if we hit end of file
//

void PAR_get_token()
{
	if (CPR_token() == NULL)
	{
		CPR_error("unexpected EOF");
		PAR_unwind();
	}
	//return NULL;
}


//____________________________________________________________
//
//		Do any initialization necessary.
//		For one thing, set all current indicators
//		to null, since nothing is current.  Also,
//		set up a block to hold the current routine,
//
//		(The 'routine' indicator tells the code
//		generator where to put ports to support
//		recursive routines and Fortran's strange idea
//		of separate sub-modules.  For PASCAL only, we
//		keep a stack of routines, and pay special attention
//		to the main routine.)
//

void PAR_init()
{
	SQL_init();

	cur_error = cur_fetch = cur_for = cur_modify = cur_store = NULL;
	cur_statement = cur_item = NULL;

	gpreGlob.cur_routine = MSC_action(0, ACT_routine);
	gpreGlob.cur_routine->act_flags |= ACT_main;
	MSC_push((gpre_nod*) gpreGlob.cur_routine, &routine_stack);
	routine_decl = true;

	flag_field = NULL;
	brace_count = 0;
	namespace_count = 0;
}

static inline void gobble(SCHAR*& string)
{
	const SCHAR* s1 = gpreGlob.token_global.tok_string;
	while (*s1)
		*string++ = *s1++;
	PAR_get_token();
}

//____________________________________________________________
//
//		Parse a native expression as a string.
//

TEXT* PAR_native_value(bool array_ref, bool handle_ref)
{
	SCHAR buffer[512];

	SCHAR* string = buffer;

	while (true)
	{
		// PAR_native_values copies the string constants. These are
		// passed to api calls. Make sure to enclose these with
		// double quotes.

		if (gpreGlob.sw_sql_dialect == 1)
		{
			if (isQuoted(gpreGlob.token_global.tok_type))
			{
				//const tok_t typ = gpreGlob.token_global.tok_type;
				gpreGlob.token_global.tok_length += 2;
				*string++ = '\"';
				gobble(string);
				*string++ = '\"';
				break;
			}
		}
		else if (gpreGlob.sw_sql_dialect == 2)
		{
			if (gpreGlob.token_global.tok_type == tok_dblquoted)
				PAR_error("Ambiguous use of double quotes in dialect 2");
			else if (gpreGlob.token_global.tok_type == tok_sglquoted)
			{
				gpreGlob.token_global.tok_length += 2;
				*string++ = '\"';
				gobble(string);
				*string++ = '\"';
				break;
			}
		}
		else if (gpreGlob.sw_sql_dialect == 3)
		{
			if (gpreGlob.token_global.tok_type == tok_sglquoted)
			{
				gpreGlob.token_global.tok_length += 2;
				*string++ = '\"';
				gobble(string);
				*string++ = '\"';
				break;
			}
		}

		if (gpreGlob.token_global.tok_keyword == KW_AMPERSAND || gpreGlob.token_global.tok_keyword == KW_ASTERISK)
			gobble(string);
		if (gpreGlob.token_global.tok_type != tok_ident)
			CPR_s_error("identifier");
		gobble(string);

		// For ADA, gobble '<attribute>

		if ((gpreGlob.sw_language == lang_ada) && (gpreGlob.token_global.tok_string[0] == '\'')) {
			gobble(string);
		}
		kwwords_t keyword = gpreGlob.token_global.tok_keyword;
		if (keyword == KW_LEFT_PAREN)
		{
			int parens = 1;
			while (parens)
			{
				const tok_t typ = gpreGlob.token_global.tok_type;
				if (isQuoted(typ))
					*string++ = (typ == tok_sglquoted) ? '\'' : '\"';
				gobble(string);
				if (isQuoted(typ))
					*string++ = (typ == tok_sglquoted) ? '\'' : '\"';
				keyword = gpreGlob.token_global.tok_keyword;
				if (keyword == KW_RIGHT_PAREN)
					parens--;
				else if (keyword == KW_LEFT_PAREN)
					parens++;
			}
			gobble(string);
			keyword = gpreGlob.token_global.tok_keyword;
		}
		while (keyword == KW_L_BRCKET)
		{
			int brackets = 1;
			while (brackets)
			{
				gobble(string);
				keyword = gpreGlob.token_global.tok_keyword;
				if (keyword == KW_R_BRCKET)
					brackets--;
				else if (keyword == KW_L_BRCKET)
					brackets++;
			}
			gobble(string);
			keyword = gpreGlob.token_global.tok_keyword;
		}

#ifdef GPRE_PASCAL
		while ((keyword == KW_CARAT) && (gpreGlob.sw_language == lang_pascal))
		{
			gobble(string);
			keyword = gpreGlob.token_global.tok_keyword;
		}
#endif

		if ((keyword == KW_DOT &&
			(!handle_ref || gpreGlob.sw_language == lang_c || gpreGlob.sw_language == lang_ada)) ||
			keyword == KW_POINTS || (keyword == KW_COLON && !gpreGlob.sw_sql && !array_ref))
		{
			gobble(string);
		}
		else
			break;
	}

	const unsigned int length = string - buffer;
	fb_assert(length < sizeof(buffer));
	string = (SCHAR*) MSC_alloc(length + 1);

	if (length)
		memcpy(string, buffer, length); // MSC_alloc filled the string with zeroes.

	return string;
}


//____________________________________________________________
//
//		Find a pseudo-field for null.  If there isn't one,
//		make one.
//

gpre_fld* PAR_null_field()
{

	if (flag_field)
		return flag_field;

	flag_field = MET_make_field("gds__null_flag", dtype_short, sizeof(SSHORT), false);

	return flag_field;
}


//____________________________________________________________
//
//		Parse the RESERVING clause of the start_transaction & set transaction
//  	statements, creating a partial TPB in the process.  The
//  	TPB just hangs off the end of the transaction block.
//

void PAR_reserving( USHORT flags, bool parse_sql)
{
	gpre_dbb* database;

	while (true)
	{
		// find a relation name, or maybe a list of them

		if (!parse_sql && terminator())
			break;

		do {
			gpre_rel* relation = EXP_relation();
			if (!relation)
				CPR_s_error("relation name");
			database = relation->rel_database;
			rrl* lock_block = (rrl*) MSC_alloc(RRL_LEN);
			lock_block->rrl_next = database->dbb_rrls;
			lock_block->rrl_relation = relation;
			database->dbb_rrls = lock_block;
		} while (MSC_match(KW_COMMA));

		// get the lock level and mode and apply them to all the
		// relations in the list

		MSC_match(KW_FOR);
		USHORT lock_level = (flags & TRA_con) ? isc_tpb_protected : isc_tpb_shared;
		USHORT lock_mode = isc_tpb_lock_read;

		if (MSC_match(KW_PROTECTED))
			lock_level = isc_tpb_protected;
		else if (MSC_match(KW_EXCLUSIVE))
			lock_level = isc_tpb_exclusive;
		else if (MSC_match(KW_SHARED))
			lock_level = isc_tpb_shared;

		if (MSC_match(KW_WRITE))
		{
			if (flags & TRA_ro)
				CPR_error("write lock requested for a read_only transaction");
			lock_mode = isc_tpb_lock_write;
		}
		else
			MSC_match(KW_READ);

		for (database = gpreGlob.isc_databases; database; database = database->dbb_next)
		{
			for (rrl* lock_block = database->dbb_rrls; lock_block; lock_block = lock_block->rrl_next)
			{
				if (!lock_block->rrl_lock_level)
				{
					fb_assert(lock_level <= MAX_UCHAR);
					fb_assert(lock_mode <= MAX_UCHAR);
					lock_block->rrl_lock_level = (UCHAR) lock_level;
					lock_block->rrl_lock_mode = (UCHAR) lock_mode;
				}
			}
		}
		if (!MSC_match(KW_COMMA))
			break;
	}
}


//____________________________________________________________
//
//		Initialize the request and the ready.
//

gpre_req* PAR_set_up_dpb_info(rdy* ready, act* action, USHORT buffercount)
{
	ready->rdy_database->dbb_buffercount = buffercount;
	gpre_req* request = MSC_request(REQ_ready);
	request->req_database = ready->rdy_database;
	request->req_actions = action;
	ready->rdy_request = request;

	return request;
}


//____________________________________________________________
//
//		Make a symbol from the current token, and advance
//		to the next token.  If a symbol type other than
//		SYM_dummy, the symbol can be overloaded, but not
//		redefined.
//

gpre_sym* PAR_symbol(sym_t type)
{
	gpre_sym* symbol;

	for (symbol = gpreGlob.token_global.tok_symbol; symbol; symbol = symbol->sym_homonym)
		if (type == SYM_dummy || symbol->sym_type == type)
		{
			TEXT s[ERROR_LENGTH];
			fb_utils::snprintf(s, sizeof(s),
				"symbol %s is already in use", gpreGlob.token_global.tok_string);
			PAR_error(s);
		}

	symbol = MSC_symbol(SYM_cursor, gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, 0);
	PAR_get_token();

	return symbol;
}


//____________________________________________________________
//
//		There's been a parse error, so unwind out.
//

void PAR_unwind()
{
	throw Firebird::LongJump();
}


//____________________________________________________________
//
//		mark databases specified in start_transaction and set transaction
//		statements.
//

void PAR_using_db()
{
	while (true)
	{
		gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_database);
		if (symbol)
		{
			gpre_dbb* db = (gpre_dbb*) symbol->sym_object;
			db->dbb_flags |= DBB_in_trans;
		}
		else
			CPR_s_error("database handle");
		PAR_get_token();
		if (!MSC_match(KW_COMMA))
			break;
	}
}


#ifdef FTN_BLK_DATA
//____________________________________________________________
//
//
//		Damn fortran sometimes only allows global
//		initializations in block data.  This collects
//		names of dbs to be so handled.
//

static void block_data_list( const gpre_dbb* db)
{
	if (db->dbb_scope == DBB_EXTERN)
		return;

	const TEXT* name = db->dbb_name->sym_string;
	const dbd* list = gpreGlob.global_db_list;

	if (gpreGlob.global_db_count)
		if (gpreGlob.global_db_count > MAX_DATABASES) {
			PAR_error("Database limit exceeded: 32 databases per source file.");
		}
		else
		{
			for (const dbd* const end = gpreGlob.global_db_list + gpreGlob.global_db_count;
				list < end; list++)
			{
				if (!strcmp(name, list->dbd_name))
					return;
			}
		}

	if (gpreGlob.global_db_count >= MAX_DATABASES)
		return;

	strcpy(list->dbd_name, name); // safe while dbd_name is defined bigger than dbb_name->sym_string
	gpreGlob.global_db_count++;
}
#endif


//____________________________________________________________
//
//
//		For reasons best left unnamed, we need
//		to skip the contents of a parenthesized
//		list
//

static bool match_parentheses()
{
	USHORT paren_count = 0;

	if (MSC_match(KW_LEFT_PAREN))
	{
		paren_count++;
		while (paren_count)
		{
			if (MSC_match(KW_RIGHT_PAREN))
				paren_count--;
			else if (MSC_match(KW_LEFT_PAREN))
				paren_count++;
			else
				PAR_get_token();
		}
		return true;
	}

	return false;
}


//____________________________________________________________
//
//		Parse a free standing ANY expression.
//

static act* par_any()
{
	// For time being flag as an error

	PAR_error("Free standing any not supported");
	return NULL; // make the compiler happy.

	/*
	gpre_sym* symbol = NULL;

	// Make up request block.  Since this might not be a database statement,
	// stay ready to back out if necessay.

	gpre_req* request = MSC_request(REQ_any);

	par_options(request, true);
	gpre_rse* rec_expr = EXP_rse(request, symbol);
	EXP_rse_cleanup(rec_expr);
	act* action = MSC_action(request, ACT_any);

	request->req_rse = rec_expr;
	gpre_ctx* context = rec_expr->rse_context[0];
	gpre_rel* relation = context->ctx_relation;
	request->req_database = relation->rel_database;

	act* function = MSC_action(0, ACT_function);
	function->act_object = (ref*) action;
	function->act_next = gpreGlob.global_functions;
	gpreGlob.global_functions = function;

	return action;
	*/
}


//____________________________________________________________
//
//		Parse a free reference to a database field in general
//		program context.  If the next keyword isn't a context
//		varying, this isn't an array element reference.
//

static act* par_array_element()
{
	if (!MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_context))
		return NULL;

	gpre_ctx* context;
	gpre_fld* field = EXP_field(&context);
	gpre_req* request = context->ctx_request;
	gpre_nod* node = EXP_array(request, field, false, false);
	ref* reference = MSC_reference(&request->req_references);
	reference->ref_expr = node;
	gpre_fld* element = field->fld_array;
	reference->ref_field = element;
	element->fld_symbol = field->fld_symbol;
	reference->ref_context = context;
	node->nod_arg[0] = (gpre_nod*) reference;

	act* action = MSC_action(request, ACT_variable);
	action->act_object = reference;

	return action;
}


//____________________________________________________________
//
//		Parse an AT END clause.
//

static act* par_at()
{
	if (!MSC_match(KW_END) || !cur_fetch)
		return NULL;

	act* action = (act*) cur_fetch->lls_object;

	return MSC_action(action->act_request, ACT_at_end);
}


//____________________________________________________________
//
//		Parse a BASED ON clause.  If this
//		is fortran and we don't have a database
//		declared yet, don't parse it completely.
//		If this is PLI look for a left paren or
//		a semi colon to avoid stomping a
//		DECLARE i FIXED BIN BASED (X);
//		or DECLARE LIST (10) FIXED BINARY BASED;
//

static act* par_based()
{
	bool notSegment = false;	// a COBOL specific patch

	MSC_match(KW_ON);
	act* action = MSC_action(0, ACT_basedon);
	bas* based_on = (bas*) MSC_alloc(BAS_LEN);
	action->act_object = (ref*) based_on;

	if ((gpreGlob.sw_language != lang_fortran) || gpreGlob.isc_databases)
	{
		TEXT s[ERROR_LENGTH];
		gpre_rel* relation = EXP_relation();
		if (!MSC_match(KW_DOT))
			CPR_s_error("dot in qualified field reference");
		SQL_resolve_identifier("<fieldname>", NULL, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Field length too long");

		gpre_fld* field = MET_field(relation, gpreGlob.token_global.tok_string);
		if (!field)
		{
			fb_utils::snprintf(s, sizeof(s), "undefined field %s", gpreGlob.token_global.tok_string);
			PAR_error(s);
		}
		if (SQL_DIALECT_V5 == gpreGlob.sw_sql_dialect)
		{
			switch (field->fld_dtype)
			{
			case dtype_sql_date:
			case dtype_sql_time:
			case dtype_int64:
				PAR_error("BASED ON impermissible datatype for a dialect-1 program");
			}
		}
		PAR_get_token();
		char tmpChar[2];			// a COBOL specific patch
		if (gpreGlob.sw_language == lang_cobol && gpreGlob.token_global.tok_keyword == KW_DOT) {
			strcpy(tmpChar, gpreGlob.token_global.tok_string);
		}
		if (MSC_match(KW_DOT))
		{
			if (!MSC_match(KW_SEGMENT))
			{
				if (gpreGlob.sw_language != lang_cobol)
					PAR_error("only .SEGMENT allowed after qualified field name");
				else
				{
					strcpy(based_on->bas_terminator, tmpChar);
					notSegment = true;
				}
			}
			else if (!(field->fld_flags & FLD_blob))
			{
				fb_utils::snprintf(s, sizeof(s), "field %s is not a blob", field->fld_symbol->sym_string);
				PAR_error(s);
			}
			// this flag is to solve KW_DOT problem in COBOL. Should be false for all other lang
			if (!notSegment)
				based_on->bas_flags |= BAS_segment;
		}
		based_on->bas_field = field;
	}
	else
	{
		based_on->bas_rel_name = (STR) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length,
			based_on->bas_rel_name->str_string);
		PAR_get_token();
		if (!MSC_match(KW_DOT))
			PAR_error("expected qualified field name");
		else
		{
			based_on->bas_fld_name = (STR) MSC_alloc(gpreGlob.token_global.tok_length + 1);
			MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length,
				based_on->bas_fld_name->str_string);
			bool ambiguous_flag = false;
			PAR_get_token();
			if (MSC_match(KW_DOT))
			{
				based_on->bas_db_name = based_on->bas_rel_name;
				based_on->bas_rel_name = based_on->bas_fld_name;
				based_on->bas_fld_name = (STR) MSC_alloc(gpreGlob.token_global.tok_length + 1);
				MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length,
					based_on->bas_fld_name->str_string);
				if (gpreGlob.token_global.tok_keyword == KW_SEGMENT)
					ambiguous_flag = true;
				PAR_get_token();
				if (MSC_match(KW_DOT))
				{
					if (!MSC_match(KW_SEGMENT))
						PAR_error("too many qualifiers on field name");
					based_on->bas_flags |= BAS_segment;
					ambiguous_flag = false;
				}
			}
			if (ambiguous_flag)
				based_on->bas_flags |= BAS_ambiguous;
		}
	}

	switch (gpreGlob.sw_language)
	{
	case lang_internal:
	case lang_fortran:
	//case lang_epascal:
	case lang_c:
	case lang_cxx:
		do {
			MSC_push((gpre_nod*) PAR_native_value(false, false),
				 &based_on->bas_variables);
		} while (MSC_match(KW_COMMA));
		// bug_4031.  based_on->bas_variables are now in reverse order.
		// we must reverse the order so we can output them to the .c
		// file correctly.
		if (based_on->bas_variables->lls_next)
		{
			gpre_lls* t1 = based_on->bas_variables;	// last one in the old list
			gpre_lls* t2 = NULL;			// last one in the new list
			gpre_lls* hold = t2;			// beginning of new list

			// while we still have a next one, keep going thru
			while (t1->lls_next)
			{
				// now find the last one in the list
				while (t1->lls_next->lls_next)
					t1 = t1->lls_next;

				// if this is the first time thru, set hold
				if (hold == NULL)
				{
					hold = t1->lls_next;
					t2 = hold;
				}
				else
				{
					// not first time thru, add this one to the end
					// of the new list

					t2->lls_next = t1->lls_next;
					t2 = t2->lls_next;
				}
				// now null out the last one, and start again
				t1->lls_next = NULL;
				t1 = based_on->bas_variables;
			}
			// ok, we're done, tack the original lls onto the very
			// end of the new list.

			t2->lls_next = t1;
			if (hold)
				based_on->bas_variables = hold;
		}
	default:
		break;
	}

	if (notSegment)
		return action;
	if (gpreGlob.token_global.tok_keyword == KW_SEMI_COLON ||
		(gpreGlob.sw_language == lang_cobol && gpreGlob.token_global.tok_keyword == KW_DOT))
	{
		strcpy(based_on->bas_terminator, gpreGlob.token_global.tok_string);
		PAR_get_token();
	}

	return action;
}


//____________________________________________________________
//
//		If this is a PASCAL program, and we're
//  	in a code block, then increment the
//		brace count.  If we're in a routine
//		declaration, then we've reached the start
//		of the code block and should mark it as
//		a new routine.
//

static act* par_begin()
{

	if (gpreGlob.sw_language == lang_pascal)
	{
		routine_decl = false;
		gpreGlob.cur_routine->act_count++;
	}
	return NULL;
}


//____________________________________________________________
//
//		Parse a blob handle and return the blob.
//

static blb* par_blob()
{
	gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_blob);

	if (!symbol)
		CPR_s_error("blob handle");

	PAR_get_token();

	return (blb*) symbol->sym_object;
}


//____________________________________________________________
//
//		Parse a GET_SEGMENT, PUT_SEGMENT, CLOSE_BLOB or CANCEL_BLOB.
//

static act* par_blob_action( act_t type)
{
	blb* blob = par_blob();
	act* action = MSC_action(blob->blb_request, type);
	action->act_object = (ref*) blob;

	// Need to eat the semicolon if present

	if (gpreGlob.sw_language == lang_c)
		MSC_match(KW_SEMI_COLON);
	else
		PAR_end();

	return action;
}


//____________________________________________________________
//
//		Parse a blob segment or blob field reference.
//

static act* par_blob_field()
{
	const bool first = gpreGlob.token_global.tok_first;

	blb* blob = par_blob();

	act_t type = ACT_blob_handle;
	if (MSC_match(KW_DOT))
	{
		if (MSC_match(KW_SEGMENT))
			type = ACT_segment;
		else if (MSC_match(KW_LENGTH))
			type = ACT_segment_length;
		else
			CPR_s_error("SEGMENT or LENGTH");
	}

	act* action = MSC_action(blob->blb_request, type);

	if (first)
		action->act_flags |= ACT_first;

	action->act_object = (ref*) blob;

	return action;
}


//____________________________________________________________
//
//		If this is a PASCAL program, and we're
//  	in a code block, then a case statement
//		will end with an END, so it adds to the
//		begin count.
//

static act* par_case()
{

	if (gpreGlob.sw_language == lang_pascal && !routine_decl)
		gpreGlob.cur_routine->act_count++;

	return NULL;
}


//____________________________________________________________
//
//		Parse degenerate CLEAR_HANDLES command.
//

static act* par_clear_handles()
{

	return MSC_action(0, ACT_clear_handles);
}


//____________________________________________________________
//
//		Parse a DERIVED_FROM clause.  Like
//		BASED ON but for C/C++ prototypes.
//

static act* par_derived_from()
{
	if (gpreGlob.sw_language != lang_c && !isLangCpp(gpreGlob.sw_language)) {
		return (NULL);
    }

	act* action = MSC_action(0, ACT_basedon);
	bas* based_on = (bas*) MSC_alloc(BAS_LEN);
	action->act_object = (ref*) based_on;

	gpre_rel* relation = EXP_relation();
	if (!MSC_match(KW_DOT))
		CPR_s_error("dot in qualified field reference");

	SQL_resolve_identifier("<Field Name>", NULL, NAME_SIZE);

	gpre_fld* field = MET_field(relation, gpreGlob.token_global.tok_string);
	if (!field)
	{
		TEXT s[ERROR_LENGTH];
		fb_utils::snprintf(s, sizeof(s), "undefined field %s", gpreGlob.token_global.tok_string);
		PAR_error(s);
	}
	PAR_get_token();
	based_on->bas_field = field;

	based_on->bas_variables = (gpre_lls*) MSC_alloc(LLS_LEN);
	based_on->bas_variables->lls_next = NULL;
	based_on->bas_variables->lls_object = (gpre_nod*) PAR_native_value(false, false);

	strcpy(based_on->bas_terminator, gpreGlob.token_global.tok_string);
	PAR_get_token();

	return action;
}


//____________________________________________________________
//
//
//     If the language is PASCAL, and if we're
//     the body of a routine,  every END counts
//     against the number of BEGIN's and CASE's
//     and when the count comes to zero, we SHOULD
//     be at the end of the current routine, so
//     pop it off the routine stack.
//

static act* par_end_block()
{
	if (gpreGlob.sw_language == lang_pascal &&
		!routine_decl && --gpreGlob.cur_routine->act_count == 0 && routine_stack)
	{
		gpreGlob.cur_routine = (act*) MSC_pop(&routine_stack);
	}

	return NULL;
}


//____________________________________________________________
//
//		Parse an END_ERROR statement.  Piece of cake.
//

static act* par_end_error()
{

	// avoid parsing an ada exception end_error -
	// check for a semicolon

	if (!PAR_end() && gpreGlob.sw_language == lang_ada)
		return NULL;

	if (!cur_error)
		PAR_error("END_ERROR used out of context");

	if (!((act*) MSC_pop(&cur_error)))
		return NULL;

	// Need to eat the semicolon for c if present

	if (gpreGlob.sw_language == lang_c)
		MSC_match(KW_SEMI_COLON);

	return MSC_action(0, ACT_enderror);
}


//____________________________________________________________
//
//		Parse END_FETCH statement (clause?).
//

static act* par_end_fetch()
{
	if (!cur_fetch)
		PAR_error("END_FETCH used out of context");

	act* begin_action = (act*) MSC_pop(&cur_fetch);

	act* action = MSC_action(begin_action->act_request, ACT_hctef);
	begin_action->act_pair = action;
	action->act_pair = begin_action;

	PAR_end();
	return action;
}


//____________________________________________________________
//
//		Parse a FOR loop terminator.
//

static act* par_end_for()
{
	if (!cur_for)
		PAR_error("unmatched END_FOR");

	act* begin_action = (act*) MSC_pop(&cur_for);
	if (!begin_action)
		return NULL;

	PAR_end();
	gpre_req* request = begin_action->act_request;

	// If the action is a blob for, make up a blob end.

	if (begin_action->act_type == ACT_blob_for)
	{
		blb* blob = (blb*) begin_action->act_object;
		act* action = MSC_action(request, ACT_endblob);
		action->act_object = (ref*) blob;
		begin_action->act_pair = action;
		action->act_pair = begin_action;
		HSH_remove(blob->blb_symbol);
		blob->blb_flags |= BLB_symbol_released;
		return action;
	}

	// If there isn't a database assigned, the FOR statement itself
	// failed.  Since an error has been given, just return quietly.

	if (!request->req_database)
		return NULL;

	act* action = MSC_action(request, ACT_endfor);
	begin_action->act_pair = action;
	action->act_pair = begin_action;
	EXP_rse_cleanup(request->req_rse);

	for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
	{
		if (!(blob->blb_flags & BLB_symbol_released))
			HSH_remove(blob->blb_symbol);
	}

	return action;
}


//____________________________________________________________
//
//		Parse and process END_MODIFY.  The processing mostly includes
//		copying field references to proper context at proper level.
//

static act* par_end_modify()
{
	if (!cur_modify)
		PAR_error("unmatched END_MODIFY");

	PAR_end();
	upd* modify = (upd*) MSC_pop(&cur_modify);

	if (gpreGlob.errors_global)
		return NULL;

	gpre_req* request = modify->upd_request;

	// used at the end of this function
	act* begin_action = request->req_actions;
	while ((upd*) begin_action->act_object != modify)
		begin_action = begin_action->act_next;

	// Build assignments for all fields and null flags referenced

	gpre_lls* stack = NULL;
	int count = 0;

	for (ref* reference = request->req_references; reference; reference = reference->ref_next)
	{
		if (reference->ref_context == modify->upd_source &&
			reference->ref_level >= modify->upd_level &&
			!reference->ref_master)
		{
			ref* change = MSC_reference(&modify->upd_references);
			change->ref_context = modify->upd_update;
			change->ref_field = reference->ref_field;
			change->ref_source = reference;
			change->ref_flags = reference->ref_flags;

			gpre_nod* item = MSC_node(nod_assignment, 2);
			item->nod_arg[0] = MSC_unary(nod_value, (gpre_nod*) change);
			item->nod_arg[1] = MSC_unary(nod_field, (gpre_nod*) change);
			MSC_push((gpre_nod*) item, &stack);
			count++;

			if (reference->ref_null)
			{
				ref* flag = MSC_reference(&modify->upd_references);
				flag->ref_context = change->ref_context;
				flag->ref_field = flag_field;
				flag->ref_master = change;
				flag->ref_source = reference->ref_null;
				change->ref_null = flag;

				item = MSC_node(nod_assignment, 2);
				item->nod_arg[0] = MSC_unary(nod_value, (gpre_nod*) flag);
				item->nod_arg[1] = MSC_unary(nod_field, (gpre_nod*) flag);
				MSC_push((gpre_nod*) item, &stack);
				count++;
			}
		}
	}

	// Build a list node of the assignments

	gpre_nod* assignments = MSC_node(nod_list, (SSHORT) count);
	modify->upd_assignments = assignments;
	gpre_nod** ptr = assignments->nod_arg + count;

	while (stack)
		*--ptr = (gpre_nod*) MSC_pop(&stack);

	act* action = MSC_action(request, ACT_endmodify);
	action->act_object = (ref*) modify;
	begin_action->act_pair = action;
	action->act_pair = begin_action;

	return action;
}


//____________________________________________________________
//
//		Parse a stream END statement.
//

static act* par_end_stream()
{
	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol || symbol->sym_type != SYM_stream)
		CPR_s_error("stream cursor");

	gpre_req* request = (gpre_req*) symbol->sym_object;
	HSH_remove(symbol);

	EXP_rse_cleanup(request->req_rse);
	PAR_get_token();
	PAR_end();

	return MSC_action(request, ACT_s_end);
}


//____________________________________________________________
//
//		Process an END_STORE.
//

static act* par_end_store(bool special)
{
	if (!cur_store)
		PAR_error("unmatched END_STORE");

	PAR_end();

	act* begin_action = (act*) MSC_pop(&cur_store);
	gpre_req* request = begin_action->act_request;

	if (request->req_type == REQ_store)
	{
		if (gpreGlob.errors_global)
			return NULL;

		// Make up an assignment list for all field references

		int count = 0;
		for (ref* reference = request->req_references; reference; reference = reference->ref_next)
		{
			if (!reference->ref_master)
				count++;
		}

		gpre_nod* const assignments = MSC_node(nod_list, (SSHORT) count);
		request->req_node = assignments;
		count = 0;

		for (ref* reference = request->req_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_master)
				continue;
			gpre_nod* item = MSC_node(nod_assignment, 2);
			item->nod_arg[0] = MSC_unary(nod_value, (gpre_nod*) reference);
			item->nod_arg[1] = MSC_unary(nod_field, (gpre_nod*) reference);
			assignments->nod_arg[count++] = item;
		}
	}
	else
	{
		// if the request type is store2, we have store ...returning_values.
		// The next action on the cur_store stack points to a upd structure
		// which will give us the assignments for this one.

		act* action2 = (act*) MSC_pop(&cur_store);
		upd* return_values = (upd*) action2->act_object;

		// Build assignments for all fields and null flags referenced

		gpre_lls* stack = NULL;
		int count = 0;

		for (ref* reference = request->req_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_context == return_values->upd_update &&
				reference->ref_level >= return_values->upd_level &&
				!reference->ref_master)
			{
				ref* change = MSC_reference(&return_values->upd_references);
				change->ref_context = return_values->upd_update;
				change->ref_field = reference->ref_field;
				change->ref_source = reference;
				change->ref_flags = reference->ref_flags;

				gpre_nod* item = MSC_node(nod_assignment, 2);
				item->nod_arg[0] = MSC_unary(nod_field, (gpre_nod*) change);
				item->nod_arg[1] = MSC_unary(nod_value, (gpre_nod*) change);
				MSC_push((gpre_nod*) item, &stack);
				count++;
			}
		}

		// Build a list node of the assignments

		gpre_nod* const assignments = MSC_node(nod_list, (SSHORT) count);
		return_values->upd_assignments = assignments;
		gpre_nod** ptr = assignments->nod_arg + count;

		while (stack)
			*--ptr = (gpre_nod*) MSC_pop(&stack);
	}

	gpre_ctx* context = request->req_contexts;
	if (context)
		HSH_remove(context->ctx_symbol);

	act* action;
	if (special)
		action = MSC_action(request, ACT_endstore_special);
	else
		action = MSC_action(request, ACT_endstore);
	begin_action->act_pair = action;
	action->act_pair = begin_action;
	return action;
}


//____________________________________________________________
//
//		Parse a ERASE statement.
//

static act* par_erase()
{
	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol || symbol->sym_type != SYM_context)
		CPR_s_error("context variable");

	gpre_ctx* source = symbol->sym_object;
	gpre_req* request = source->ctx_request;
	if (request->req_type != REQ_for && request->req_type != REQ_cursor)
		PAR_error("invalid context for modify");

	PAR_get_token();
	PAR_end();

	// Make an update block to hold everything known about the modify

	upd* erase = (upd*) MSC_alloc(UPD_LEN);
	erase->upd_request = request;
	erase->upd_source = source;

	act* action = MSC_action(request, ACT_erase);
	action->act_object = (ref*) erase;

	return action;
}


//____________________________________________________________
//
//		Parse a stream FETCH statement.
//

static act* par_fetch()
{
	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol || symbol->sym_type != SYM_stream)
		return NULL;

	gpre_req* request = (gpre_req*) symbol->sym_object;
	PAR_get_token();
	PAR_end();

	act* action = MSC_action(request, ACT_s_fetch);
	MSC_push((gpre_nod*) action, &cur_fetch);

	return action;
}


//____________________________________________________________
//
//		Parse a FINISH statement.
//

static act* par_finish()
{
	act* action = MSC_action(0, ACT_finish);

	if (!terminator())
		while (true)
		{
			gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
			if (symbol && (symbol->sym_type == SYM_database))
			{
				rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
				ready->rdy_next = (rdy*) action->act_object;
				action->act_object = (ref*) ready;
				ready->rdy_database = (gpre_dbb*) symbol->sym_object;
				CPR_eol_token();
			}
			else
				CPR_s_error("database handle");
			if (terminator())
				break;
			if (!MSC_match(KW_COMMA))
				break;
		}

	if (gpreGlob.sw_language == lang_ada)
		MSC_match(KW_SEMI_COLON);
	return action;
}


//____________________________________________________________
//
//		Parse a FOR clause, returning an action.
//		We don't know where we are a host language FOR, a record looping
//		FOR, or a blob FOR.  Parse a little ahead and try to find out.
//		Avoid stepping on user routines that use GDML keywords
//

static act* par_for()
{
	TEXT s[ERROR_LENGTH];
	gpre_sym* symbol = NULL;
	bool dup_symbol = false;

	tok& token = gpreGlob.token_global;

	if (token.tok_keyword != KW_FIRST && token.tok_keyword != KW_LEFT_PAREN)
	{
		if (token.tok_symbol)
			dup_symbol = true;

		symbol = MSC_symbol(SYM_cursor, token.tok_string, token.tok_length, 0);
		PAR_get_token();

		if (!MSC_match(KW_IN))
		{
			MSC_free(symbol);
			return NULL;
		}
		if (dup_symbol)
		{
			fb_utils::snprintf(s, sizeof(s), "symbol %s is already in use", token.tok_string);
			PAR_error(s);
		}

		const gpre_sym* temp = token.tok_symbol;
		if (temp && temp->sym_type == SYM_context)
			return par_open_blob(ACT_blob_for, symbol);
	}

	// Make up request block.  Since this might not be a database statement,
	// stay ready to back out if necessay.

	gpre_req* request = MSC_request(REQ_for);

	gpre_rse* rec_expr;
	if (!par_options(request, true) || !(rec_expr = EXP_rse(request, symbol)))
	{
		MSC_free_request(request);
		return NULL;
	}

	act* action = MSC_action(request, ACT_for);
	MSC_push((gpre_nod*) action, &cur_for);

	request->req_rse = rec_expr;
	gpre_ctx* context = rec_expr->rse_context[0];
	gpre_rel* relation = context->ctx_relation;
	request->req_database = relation->rel_database;

	gpre_ctx** ptr = rec_expr->rse_context;
	for (const gpre_ctx* const* const end = ptr + rec_expr->rse_count; ptr < end; ptr++)
	{
		context = *ptr;
		context->ctx_next = request->req_contexts;
		request->req_contexts = context;
	}

	return action;
}

//____________________________________________________________
//
//  	A function declaration is interesting in
//		FORTRAN because it starts a new sub-module
//		and we have to begin everything all over.
//		In PASCAL it's interesting because it may
//		indicate a good place to put message declarations.
//		Unfortunately that requires a loose parse of the
//		routine header, but what the hell...
//

static act* par_function()
{

	if (gpreGlob.sw_language == lang_fortran)
		return par_subroutine();

	if (gpreGlob.sw_language == lang_pascal)
		return par_procedure();

	return NULL;
}


//____________________________________________________________
//
//		Check a left brace (or whatever) for start of a new
//		routine.
//

static act* par_left_brace()
{
	if (brace_count++ - namespace_count > 0)
		return NULL;

	act* action = MSC_action(0, ACT_routine);
	gpreGlob.cur_routine = action;
	action->act_flags |= ACT_mark;

	return action;
}


//____________________________________________________________
//
//		Parse a MODIFY statement.
//

static act* par_modify()
{

	// Set up modify and action blocks.  This is done here to leave the
	// structure in place to cleanly handle END_MODIFY under error conditions.

	upd* modify = (upd*) MSC_alloc(UPD_LEN);
	MSC_push((gpre_nod*) modify, &cur_modify);

	// If the next token isn't a context variable, we can't continue

	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol || symbol->sym_type != SYM_context)
	{
		SCHAR s[ERROR_LENGTH];
		sprintf(s, "%s is not a valid context variable", gpreGlob.token_global.tok_string);
		PAR_error(s);
	}

	gpre_ctx* source = symbol->sym_object;
	gpre_req* request = source->ctx_request;
	if (request->req_type != REQ_for && request->req_type != REQ_cursor)
		PAR_error("invalid context for modify");

	act* action = MSC_action(request, ACT_modify);
	action->act_object = (ref*) modify;

	PAR_get_token();
	MSC_match(KW_USING);

	// Make an update context by cloning the source context

	gpre_ctx* update = MSC_context(request);
	update->ctx_symbol = source->ctx_symbol;
	update->ctx_relation = source->ctx_relation;

	// Make an update block to hold everything known about the modify

	modify->upd_request = request;
	modify->upd_source = source;
	modify->upd_update = update;
	modify->upd_level = ++request->req_level;

	return action;
}


//____________________________________________________________
//
//		This rather degenerate routine exists to allow both:
//
//			ON_ERROR
//			ON ERROR
//
//		so the more dim of our users avoid mistakes.
//

static act* par_on()
{
	if (!MSC_match(KW_ERROR))
		return NULL;

	return par_on_error();
}


//____________________________________________________________
//

//		Parse a trailing ON_ERROR clause.
//

static act* par_on_error()
{
	if (!cur_statement)
		PAR_error("ON_ERROR used out of context");

	PAR_end();
	act* action = MSC_action(0, ACT_on_error);
	cur_statement->act_error = action;
	action->act_object = (ref*) cur_statement;

	MSC_push((gpre_nod*) action, &cur_error);

	if (cur_statement->act_pair)
		cur_statement->act_pair->act_error = action;

	return action;
}


//____________________________________________________________
//
//		Parse an "open blob" type statement.  These include OPEN_BLOB,
//		CREATE_BLOB, and blob FOR.
//

static act* par_open_blob( act_t act_op, gpre_sym* symbol)
{
	// If somebody hasn't already parsed up a symbol for us, parse the
	// symbol and the mandatory IN now.

	if (!symbol)
	{
		symbol = PAR_symbol(SYM_dummy);
		if (!MSC_match(KW_IN))
			CPR_s_error("IN");
	}

	// The next thing we should find is a field reference.  Get it.
	gpre_ctx* context;
	gpre_fld* field = EXP_field(&context);
	if (!field)
		return NULL;

	TEXT s[ERROR_LENGTH];
	if (!(field->fld_flags & FLD_blob))
	{
		fb_utils::snprintf(s, sizeof(s), "Field %s is not a blob", field->fld_symbol->sym_string);
		PAR_error(s);
	}

	if (field->fld_array_info)
	{
		fb_utils::snprintf(s, sizeof(s), "Field %s is an array and can not be opened as a blob",
				field->fld_symbol->sym_string);
		PAR_error(s);
	}

	gpre_req* request = context->ctx_request;
	ref* reference = EXP_post_field(field, context, false);

	blb* blob = (blb*) MSC_alloc(BLB_LEN);
	blob->blb_symbol = symbol;
	blob->blb_reference = reference;

	// See if we need a blob filter (do we have a subtype to subtype clause?)

	for (;;)
	{
		if (MSC_match(KW_FILTER))
		{
			blob->blb_const_from_type = (MSC_match(KW_FROM)) ?
				PAR_blob_subtype(request->req_database) : field->fld_sub_type;
			if (!MSC_match(KW_TO))
				CPR_s_error("TO");
			blob->blb_const_to_type = PAR_blob_subtype(request->req_database);
		}
		else if (MSC_match(KW_STREAM))
			blob->blb_type = isc_bpb_type_stream;
		else
			break;
	}

	if (!(blob->blb_seg_length = field->fld_seg_length))
		blob->blb_seg_length = 512;

	blob->blb_request = request;
	blob->blb_next = request->req_blobs;
	request->req_blobs = blob;

	symbol->sym_type = SYM_blob;
	symbol->sym_object = (gpre_ctx*) blob;
	HSH_insert(symbol);
	// You just inserted the context variable into the hash table.
	// The current token however might be the same context variable.
	// If so, get the symbol for it.

	if (gpreGlob.token_global.tok_keyword == KW_none)
		gpreGlob.token_global.tok_symbol = HSH_lookup(gpreGlob.token_global.tok_string);

	act* action = MSC_action(request, act_op);
	action->act_object = (ref*) blob;

	if (act_op == ACT_blob_for)
		MSC_push((gpre_nod*) action, &cur_for);

	// Need to eat the semicolon if present

	if (gpreGlob.sw_language == lang_c)
		MSC_match(KW_SEMI_COLON);
	else
		PAR_end();

	return action;
}


//____________________________________________________________
//
//		Parse request options.  Return true if successful, otherwise
//		false.  If a flag is set, don't give an error on false.
//

static bool par_options(gpre_req* request, bool flag)
{
	if (!MSC_match(KW_LEFT_PAREN))
		return true;

	while (true)
	{
		if (MSC_match(KW_RIGHT_PAREN))
			return true;
		if (MSC_match(KW_REQUEST_HANDLE))
		{
			request->req_handle = PAR_native_value(false, true);
			request->req_flags |= REQ_exp_hand;
		}
		else if (MSC_match(KW_TRANSACTION_HANDLE))
			request->req_trans = PAR_native_value(false, true);
		else if (MSC_match(KW_LEVEL))
			request->req_request_level = PAR_native_value(false, false);
		else
		{
			if (!flag)
				CPR_s_error("request option");
			return false;
		}
		MSC_match(KW_COMMA);
	}
}


//____________________________________________________________
//
//		If this is PLI, then we've got a new procedure.
//
//		If this is PASCAL, then we've come upon
//		a program, module, function, or procedure header.
//		Alas and alack, we have to decide if this is
//		a real header or a forward/external declaration.
//
//		In either case, we make a mark-only action block,
//		because that's real cheap.  If it's a real routine,
//		we make the action the current routine.
//

static act* par_procedure()
{
	act* action;

	if (gpreGlob.sw_language == lang_pascal)
	{
		routine_decl = true;
		action = scan_routine_header();
		if (!(action->act_flags & ACT_decl))
		{
			MSC_push((gpre_nod*) gpreGlob.cur_routine, &routine_stack);
			gpreGlob.cur_routine = action;
		}
	}
	else
		action = NULL;
	return action;
}


//____________________________________________________________
//
//		Parse a READY statement.
//

static act* par_ready()
{
	gpre_req* request;
	gpre_sym* symbol;
	gpre_dbb* db;
	bool need_handle = false;
	USHORT default_buffers = 0;

	act* action = MSC_action(0, ACT_ready);

	if (gpreGlob.token_global.tok_keyword == KW_CACHE)
		CPR_s_error("database name or handle");

	while (!terminator())
	{
		// this default mechanism is left here for backwards
		// compatibility, but it is no longer documented and
		// is not something we should maintain for all ready
		// options since it needlessly complicates the ready
		// statement without providing any extra functionality

		if (MSC_match(KW_DEFAULT))
		{
			if (!MSC_match(KW_CACHE))
				CPR_s_error("database name or handle");
			default_buffers = atoi(gpreGlob.token_global.tok_string);
			CPR_eol_token();
			MSC_match(KW_BUFFERS);
			continue;
		}

		rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
		ready->rdy_next = (rdy*) action->act_object;
		action->act_object = (ref*) ready;

		if (!(symbol = gpreGlob.token_global.tok_symbol) || symbol->sym_type != SYM_database)
		{
			ready->rdy_filename = PAR_native_value(false, false);
			if (MSC_match(KW_AS))
				need_handle = true;
		}

		if (!(symbol = gpreGlob.token_global.tok_symbol) || symbol->sym_type != SYM_database)
		{
			if (!gpreGlob.isc_databases || gpreGlob.isc_databases->dbb_next || need_handle)
			{
				need_handle = false;
				CPR_s_error("database handle");
			}
			ready->rdy_database = gpreGlob.isc_databases;
		}

		need_handle = false;
		if (!ready->rdy_database)
			ready->rdy_database = (gpre_dbb*) symbol->sym_object;
		if (terminator())
			break;
		CPR_eol_token();

		// pick up the possible parameters, in any order

		USHORT buffers = 0;
		db = ready->rdy_database;
		for (;;)
		{
			if (MSC_match(KW_CACHE))
			{
				buffers = atoi(gpreGlob.token_global.tok_string);
				CPR_eol_token();
				MSC_match(KW_BUFFERS);
			}
			else if (MSC_match(KW_USER))
				db->dbb_r_user = PAR_native_value(false, false);
			else if (MSC_match(KW_PASSWORD))
				db->dbb_r_password = PAR_native_value(false, false);
			else if (MSC_match(KW_LC_MESSAGES))
				db->dbb_r_lc_messages = PAR_native_value(false, false);
			else if (MSC_match(KW_LC_CTYPE))
			{
				db->dbb_r_lc_ctype = PAR_native_value(false, false);
				db->dbb_know_subtype = 2;
			}
			else
				break;
		}

		request = NULL;
		if (buffers)
			request = PAR_set_up_dpb_info(ready, action, buffers);

		// if there are any options that take host variables as arguments,
		// make sure that we generate variables for the request so that the
		// dpb can be extended at runtime

		if (db->dbb_r_user || db->dbb_r_password || db->dbb_r_lc_messages || db->dbb_r_lc_ctype)
		{
			if (!request)
				request = PAR_set_up_dpb_info(ready, action, default_buffers);
			request->req_flags |= REQ_extend_dpb;
		}

		// ... and if there are compile time user or password specified,
		// make sure there will be a dpb generated for them

		if (!request && (db->dbb_c_user || db->dbb_c_password ||
						 db->dbb_c_lc_messages || db->dbb_c_lc_ctype))
		{
			request = PAR_set_up_dpb_info(ready, action, default_buffers);
		}

		MSC_match(KW_COMMA);
	}

	PAR_end();

	if (action->act_object)
	{
		if (default_buffers)
			for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
			{
				if (!ready->rdy_request)
					request = PAR_set_up_dpb_info(ready, action, default_buffers);
			}
		return action;
	}

	// No explicit databases -- pick up all known

	for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
		if (db->dbb_runtime || !(db->dbb_flags & DBB_sqlca))
		{
			rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
			ready->rdy_next = (rdy*) action->act_object;
			action->act_object = (ref*) ready;
			ready->rdy_database = db;
		}

	if (!action->act_object)
		PAR_error("no database available to READY");
	else
		for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
		{
			request = ready->rdy_request;
			if (default_buffers && !ready->rdy_request)
				request = PAR_set_up_dpb_info(ready, action, default_buffers);

			// if there are any options that take host variables as arguments,
			// make sure that we generate variables for the request so that the
			// dpb can be extended at runtime

			db = ready->rdy_database;
			if (db->dbb_r_user || db->dbb_r_password || db->dbb_r_lc_messages || db->dbb_r_lc_ctype)
			{
				if (!request)
					request = PAR_set_up_dpb_info(ready, action, default_buffers);
				request->req_flags |= REQ_extend_dpb;
			}

			// ... and if there are compile time user or password specified,
			// make sure there will be a dpb generated for them

			if (!request && (db->dbb_c_user || db->dbb_c_password ||
							 db->dbb_c_lc_messages || db->dbb_c_lc_ctype))
			{
				request = PAR_set_up_dpb_info(ready, action, default_buffers);
			}
		}

	return action;
}


//____________________________________________________________
//
//		Parse a returning values clause in a STORE
//		returning an action.
//		Act as if we were at end_store, then set up
//		for a further set of fields for returned values.
//

static act* par_returning_values()
{
	if (!cur_store)
		PAR_error("STORE must precede RETURNING_VALUES");

	act* begin_action = (act*) MSC_pop(&cur_store);
	gpre_req* request = begin_action->act_request;

	// First take care of the impending store:
	// Make up an assignment list for all field references  and
	// clone the references while we are at it

	int count = 0;
	for (ref* reference = request->req_references; reference; reference = reference->ref_next)
	{
		if (!reference->ref_master)
			count++;
	}

	gpre_nod* assignments = MSC_node(nod_list, (SSHORT) count);
	request->req_node = assignments;
	count = 0;

	for (ref* reference = request->req_references; reference; reference = reference->ref_next)
	{
		ref* save_ref = MSC_reference(&begin_action->act_object);
		save_ref->ref_context = reference->ref_context;
		save_ref->ref_field = reference->ref_field;
		save_ref->ref_source = reference;
		save_ref->ref_flags = reference->ref_flags;

		if (reference->ref_master)
			continue;
		gpre_nod* item = MSC_node(nod_assignment, 2);
		item->nod_arg[0] = MSC_unary(nod_value, (gpre_nod*) save_ref);
		item->nod_arg[1] = MSC_unary(nod_field, (gpre_nod*) save_ref);
		assignments->nod_arg[count++] = item;
	}

	// Next make an updated context for post_store actions

	upd* new_values = (upd*) MSC_alloc(UPD_LEN);
	gpre_ctx* source = request->req_contexts;
	request->req_type = REQ_store2;

	gpre_ctx* new_ctx = MSC_context(request);
	new_ctx->ctx_symbol = source->ctx_symbol;
	new_ctx->ctx_relation = source->ctx_relation;
	new_ctx->ctx_symbol->sym_object = new_ctx; // pointing to itself?

	// make an update block to hold everything known about referenced fields

	act* action = MSC_action(request, ACT_store2);
	action->act_object = (ref*) new_values;

	new_values->upd_request = request;
	new_values->upd_source = source;
	new_values->upd_update = new_ctx;
	new_values->upd_level = ++request->req_level;

	// both actions go on the cur_store stack, the store topmost

	MSC_push((gpre_nod*) action, &cur_store);
	MSC_push((gpre_nod*) begin_action, &cur_store);
	return action;
}


//____________________________________________________________
//
//		Do something about a right brace.
//

static act* par_right_brace()
{
	if (--brace_count < 0)
		brace_count = 0;

	if (brace_count <= 0)
	{
		if (--namespace_count < 0)
			namespace_count = 0;
	}

	return NULL;
}


//____________________________________________________________
//
//		Parse a RELEASE_REQUEST statement.
//

static act* par_release()
{
	act* action = MSC_action(0, ACT_release);

	MSC_match(KW_FOR);

	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (symbol && (symbol->sym_type == SYM_database))
	{
		action->act_object = (ref*) symbol->sym_object;
		PAR_get_token();
	}

	PAR_end();

	return action;
}


//____________________________________________________________
//
//		Handle a GET_SLICE or PUT_SLICE statement.
//

static act* par_slice( act_t type)
{
	gpre_ctx* context;
	gpre_fld* field = EXP_field(&context);

	ary* info = field->fld_array_info;
	if (!info)
		CPR_s_error("array field");

	gpre_req* request = MSC_request(REQ_slice);
	slc* slice = (slc*) MSC_alloc(SLC_LEN(info->ary_dimension_count));
	request->req_slice = slice;
	slice->slc_dimensions = info->ary_dimension_count;
	slice->slc_field = field;
	slice->slc_field_ref = EXP_post_field(field, context, false);
	slice->slc_parent_request = context->ctx_request;

	if (!MSC_match(KW_L_BRCKET))
		CPR_s_error("left bracket");

	USHORT n = 0;
	for (slc::slc_repeat* tail = slice->slc_rpt; ++n <= slice->slc_dimensions; ++tail)
	{
		tail->slc_lower = tail->slc_upper = EXP_subscript(request);
		if (MSC_match(KW_COLON))
			tail->slc_upper = EXP_subscript(request);
		if (!MSC_match(KW_COMMA))
			break;
	}

	if (n != slice->slc_dimensions)
		PAR_error("subscript count mismatch");

	if (!MSC_match(KW_R_BRCKET))
		CPR_s_error("right bracket");

	if (type == ACT_get_slice)
	{
		if (!MSC_match(KW_INTO))
			CPR_s_error("INTO");
	}
	else if (!MSC_match(KW_FROM))
		CPR_s_error("FROM");

	slice->slc_array = EXP_subscript(0);

	act* action = MSC_action(request, type);
	action->act_object = (ref*) slice;

	if (gpreGlob.sw_language == lang_c)
		MSC_match(KW_SEMI_COLON);
	else
		PAR_end();

	return action;
}


//____________________________________________________________
//
//		Parse a STORE clause, returning an action.
//

static act* par_store()
{
	gpre_req* request = MSC_request(REQ_store);
	par_options(request, false);
	act* action = MSC_action(request, ACT_store);
	MSC_push((gpre_nod*) action, &cur_store);

	gpre_ctx* context = EXP_context(request, 0);
	gpre_rel* relation = context->ctx_relation;
	request->req_database = relation->rel_database;
	HSH_insert(context->ctx_symbol);
	// You just inserted the context variable into the hash table.
	// The current token however might be the same context variable.
	// If so, get the symbol for it.

	if (gpreGlob.token_global.tok_keyword == KW_none)
		gpreGlob.token_global.tok_symbol = HSH_lookup(gpreGlob.token_global.tok_string);
	MSC_match(KW_USING);

	return action;
}


//____________________________________________________________
//
//		Parse a start stream statement.
//

static act* par_start_stream()
{
	gpre_req* request = MSC_request(REQ_cursor);
	par_options(request, false);
	act* action = MSC_action(request, ACT_s_start);

	gpre_sym* cursor = PAR_symbol(SYM_dummy);
	cursor->sym_type = SYM_stream;
	cursor->sym_object = (gpre_ctx*) request;

	MSC_match(KW_USING);
	gpre_rse* rec_expr = EXP_rse(request, 0);
	request->req_rse = rec_expr;
	gpre_ctx* context = rec_expr->rse_context[0];
	gpre_rel* relation = context->ctx_relation;
	request->req_database = relation->rel_database;

	gpre_ctx** ptr = rec_expr->rse_context;
	for (const gpre_ctx* const* const end = ptr + rec_expr->rse_count; ptr < end; ptr++)
	{
		context = *ptr;
		context->ctx_next = request->req_contexts;
		request->req_contexts = context;
	}

	HSH_insert(cursor);
	PAR_end();

	return action;
}


//____________________________________________________________
//
//		Parse a START_TRANSACTION statement, including
//  	transaction handle, transaction options, and
//		reserving list.
//

static act* par_start_transaction()
{
	act* action = MSC_action(0, ACT_start);

	if (terminator())
	{
		PAR_end();
		return action;
	}

	gpre_tra* trans = (gpre_tra*) MSC_alloc(TRA_LEN);

	// get the transaction handle

	if (!gpreGlob.token_global.tok_symbol)
		trans->tra_handle = PAR_native_value(false, true);

	// loop reading the various transaction options

	while (gpreGlob.token_global.tok_keyword != KW_RESERVING &&
		gpreGlob.token_global.tok_keyword != KW_USING && !terminator())
	{
		if (MSC_match(KW_READ_ONLY))
		{
			trans->tra_flags |= TRA_ro;
			continue;
		}
		if (MSC_match(KW_READ_WRITE))
			continue;

		if (MSC_match(KW_CONSISTENCY))
		{
			trans->tra_flags |= TRA_con;
			continue;
		}

		//if (MSC_match (KW_READ_COMMITTED))
		//{
		//	trans->tra_flags |= TRA_read_committed;
		//	continue;
		//}

		if (MSC_match(KW_CONCURRENCY))
			continue;

		if (MSC_match(KW_NO_WAIT))
		{
			trans->tra_flags |= TRA_nw;
			continue;
		}

		if (MSC_match(KW_WAIT))
			continue;

		if (MSC_match(KW_AUTOCOMMIT))
		{
			trans->tra_flags |= TRA_autocommit;
			continue;
		}

		if (gpreGlob.sw_language == lang_cobol || gpreGlob.sw_language == lang_fortran)
			break;

		CPR_s_error("transaction keyword");
	}

	// send out for the list of reserved relations

	if (MSC_match(KW_RESERVING))
	{
		trans->tra_flags |= TRA_rrl;
		PAR_reserving(trans->tra_flags, false);
	}
	else if (MSC_match(KW_USING))
	{
		trans->tra_flags |= TRA_inc;
		PAR_using_db();
	}

	PAR_end();
	CMP_t_start(trans);
	action->act_object = (ref*) trans;

	return action;
}


//____________________________________________________________
//
//		We have hit either a function or subroutine declaration.
//		If the language is fortran, make the position with a break.
//

static act* par_subroutine()
{
	if (gpreGlob.sw_language != lang_fortran)
		return NULL;

	act* action = MSC_action(0, ACT_routine);
	action->act_flags |= ACT_mark | ACT_break;
	gpreGlob.cur_routine = action;
	return action;
}


//____________________________________________________________
//
//		Parse a transaction termination statement: commit,
//		prepare, rollback, or save (commit retaining context).
//

static act* par_trans( act_t act_op)
{
	act* action = MSC_action(0, act_op);

	if (!terminator())
	{
		const bool parens = MSC_match(KW_LEFT_PAREN);
		if ((gpreGlob.sw_language == lang_fortran) && (act_op == ACT_commit_retain_context))
		{
			if (!MSC_match(KW_TRANSACTION_HANDLE))
				return NULL;
		}
		else
			MSC_match(KW_TRANSACTION_HANDLE);
		action->act_object = (ref*) PAR_native_value(false, true);
		if (parens)
			EXP_match_paren();
	}

	if ((gpreGlob.sw_language != lang_fortran) && (gpreGlob.sw_language != lang_pascal))
		MSC_match(KW_SEMI_COLON);

	return action;
}


//____________________________________________________________
//
//		Parse something of the form:
//
//			<relation> . <field> . <something>
//
//		where <something> is currently an enumerated type.
//

static act* par_type()
{
	// Pick up relation

	//gpre_sym*	symbol;
	//symbol = gpreGlob.token_global.tok_symbol;
	//relation = (gpre_rel*) symbol->sym_object;
	//PAR_get_token();

	gpre_rel* relation = EXP_relation();

	// No dot and we give up

	if (!MSC_match(KW_DOT))
		return NULL;

	// Look for field name.  No field name, punt

	SQL_resolve_identifier("<Field Name>", NULL, NAME_SIZE);
	gpre_fld* field = MET_field(relation, gpreGlob.token_global.tok_string);
	if (!field)
		return NULL;

	PAR_get_token();

	if (!MSC_match(KW_DOT))
		CPR_s_error("period");

	// Lookup type.  If we can't find it, complain bitterly

	SSHORT type;
	if (!MET_type(field, gpreGlob.token_global.tok_string, &type))
	{
		TEXT s[ERROR_LENGTH];
		fb_utils::snprintf(s, sizeof(s), "undefined type %s", gpreGlob.token_global.tok_string);
		PAR_error(s);
	}

	PAR_get_token();
	act* action = MSC_action(0, ACT_type_number);
	action->act_object = (ref*) (IPTR) type;

	return action;
}

//____________________________________________________________
//
//		Parse a free reference to a database field in general
//		program context.
//

static act* par_variable()
{
	// Since fortran is fussy about continuations and the like,
	// see if this variable token is the first thing in a statement.

	const bool first = gpreGlob.token_global.tok_first;
	gpre_ctx* context;
	gpre_fld* field = EXP_field(&context);

	gpre_fld* cast;
	bool dot = MSC_match(KW_DOT);
	if (dot && (cast = EXP_cast(field)))
	{
		field = cast;
		dot = MSC_match(KW_DOT);
	}

	bool is_null = false;
	if (dot && MSC_match(KW_NULL))
	{
		is_null = true;
		dot = false;
	}

	gpre_req* request = context->ctx_request;
	ref* reference = EXP_post_field(field, context, is_null);

	if (field->fld_array)
		EXP_post_array(reference);

	act* action = MSC_action(request, ACT_variable);

	if (first)
		action->act_flags |= ACT_first;
	if (dot)
		action->act_flags |= ACT_back_token;

	action->act_object = reference;

	if (!is_null)
		return action;

	// We've got a explicit null flag reference rather than a field
	// reference.  If there's already a null reference for the field,
	// use it; otherwise make one up.

	if (reference->ref_null)
	{
		action->act_object = reference->ref_null;
		return action;
	}

	// Check to see if the flag field has been allocated.  If not, sigh, allocate it

	ref* flag = MSC_reference(&request->req_references);
	flag->ref_context = reference->ref_context;
	flag->ref_field = PAR_null_field();
	flag->ref_level = request->req_level;

	flag->ref_master = reference;
	reference->ref_null = flag;

	action->act_object = flag;

	return action;
}


//____________________________________________________________
//
//		This is PASCAL, and we've got a function, or procedure header.
//		Alas and alack, we have to decide if this is a real header or
//		a forward/external declaration.
//
//		Basically we scan the thing, skipping parenthesized bits,
//		looking for a semi-colon.  We look at the next token, which may
//		be OPTIONS followed by a parenthesized list of options, or it
//		may be just some options, or it may be nothing.  If the options
//		are EXTERN or FORWARD, we've got a reference, otherwise its a real
//		routine (or possibly program or module).
//
//		Fortunately all of these are of the form:
//			<keyword> <name> [( blah, blah )] [: type] ; [<options>;]
//
//

static act* scan_routine_header()
{
	act* action = MSC_action(0, ACT_routine);
	action->act_flags |= ACT_mark;

	while (!MSC_match(KW_SEMI_COLON))
	{
		if (!match_parentheses())
			PAR_get_token();
	}

	if (MSC_match(KW_OPTIONS) && MSC_match(KW_LEFT_PAREN))
	{
		while (!MSC_match(KW_RIGHT_PAREN))
		{
			if (MSC_match(KW_EXTERN) || MSC_match(KW_FORWARD))
				action->act_flags |= ACT_decl;
			else
				PAR_get_token();
		}
		MSC_match(KW_SEMI_COLON);
	}
	else
	{
		for (;;)
		{
			if (MSC_match(KW_EXTERN) || MSC_match(KW_FORWARD))
			{
				action->act_flags |= ACT_decl;
				MSC_match(KW_SEMI_COLON);
			}
			else if (MSC_match(KW_INTERNAL) || MSC_match(KW_ABNORMAL) ||
					 MSC_match(KW_VARIABLE) || MSC_match(KW_VAL_PARAM))
			{
				MSC_match(KW_SEMI_COLON);
			}
			else
				break;
		}
	}

	return action;
}


//____________________________________________________________
//
//		If this is a external declaration in
//		a BASIC program, set a flag to indicate
//		the situation.
//

static void set_external_flag()
{
	CPR_token();
}


//____________________________________________________________
//
//		Check the current token for a logical terminator.  Terminators
//		are semi-colon, ELSE, or ON_ERROR.
//

static bool terminator()
{

	// For C, changed keyword (KW_SEMICOLON) to MSC_match (KW_SEMICOLON) to eat a
	// semicolon if it is present so as to allow it to be there or not be there.
	// Bug#833.  mao 6/21/89

	// For C, right brace ("}") must also be a terminator.

	// Used reference here because MSC_match() can change tok_leyword.
	kwwords_t& key = gpreGlob.token_global.tok_keyword;

	switch (gpreGlob.sw_language)
	{
	case lang_c:
		if (MSC_match(KW_SEMI_COLON) || key == KW_ELSE || key == KW_ON_ERROR || key == KW_R_BRACE)
		{
			return true;
		}
		break;
	case lang_ada:
		if (MSC_match(KW_SEMI_COLON) || key == KW_ELSE || key == KW_ON_ERROR)
		{
			return true;
		}
		break;
	default:
		if (key == KW_SEMI_COLON || key == KW_ELSE || key == KW_ON_ERROR ||
			(gpreGlob.sw_language == lang_cobol && key == KW_DOT))
		{
			return true;
		}
	}

	return false;
}

