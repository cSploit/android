//____________________________________________________________
//
//		PROGRAM:	C Preprocessor
//		MODULE:		sql.cpp
//		DESCRIPTION:	SQL parser
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
#include "../gpre/gpre.h"
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../jrd/constants.h"
#include "../gpre/cme_proto.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/exp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "../gpre/par_proto.h"
#include "../gpre/sqe_proto.h"
#include "../gpre/sql_proto.h"
#include "../common/utils_proto.h"


const int DEFAULT_BLOB_SEGMENT_LENGTH	= 80;	// bytes

static act* act_alter();
static act* act_alter_database();
static act* act_alter_domain();
static act* act_alter_index();
static act* act_alter_table();
static act* act_comment();
static act* act_connect();
static act* act_create();
static act* act_create_database();
static act* act_create_domain();
static act* act_create_generator();
static act* act_create_index(bool, bool);
static act* act_create_shadow();
static act* act_create_table();
static act* act_create_view();
static act* act_d_section(act_t);
static act* act_declare();
static act* act_declare_filter();
static act* act_declare_table(gpre_sym*, gpre_dbb*);
static act* act_declare_udf();
static act* act_delete();
static act* act_describe();
static act* act_disconnect();
static act* act_drop();
static act* act_event();
static act* act_execute();
static act* act_fetch();
static act* act_grant_revoke(act_t);
static act* act_include();
static act* act_insert();
static act* act_insert_blob();
static act* act_lock();
static act* act_openclose(act_t);
static act* act_open_blob(act_t, gpre_sym*);
static act* act_prepare();
static act* act_procedure();
static act* act_release();
static act* act_select();
static act* act_set(const TEXT*);
static act* act_set_dialect();
static act* act_set_generator();
static act* act_set_names();
static act* act_set_statistics();
static act* act_set_transaction();
static act* act_transaction(act_t);
static act* act_update();
static act* act_whenever();

static bool			check_filename(const TEXT *);
static void			connect_opts(const TEXT**, const TEXT**, const TEXT**, const TEXT**, USHORT*);
static gpre_file*	define_file();
static gpre_file*	define_log_file();
static gpre_dbb*	dup_dbb(const gpre_dbb*);
static void			error(const TEXT *, const TEXT *);
static TEXT*		extract_string(bool);
static swe*			gen_whenever();
static void			into(gpre_req*, gpre_nod*, gpre_nod*);
static gpre_fld*	make_field(gpre_rel*);
static gpre_index*	make_index(gpre_req*, const TEXT*);
static gpre_rel*	make_relation(gpre_req*, const TEXT *);
static void			pair(gpre_nod*, gpre_nod*);
static void			par_array(gpre_fld*);
static SSHORT		par_char_set();
static void			par_computed(gpre_req*, gpre_fld*);
static gpre_req*	par_cursor(gpre_sym**);
static dyn*			par_dynamic_cursor();
static gpre_fld*	par_field(gpre_req*, gpre_rel*);
static cnstrt*		par_field_constraint(gpre_req*, gpre_fld*);
static void			par_fkey_extension(cnstrt*);
static bool			par_into(dyn*);
static void			par_options(const TEXT**);
static int			par_page_size();
static gpre_rel*	par_relation(gpre_req*);
static dyn*			par_statement();
static cnstrt*		par_table_constraint(gpre_req*);
static bool			par_transaction_modes(gpre_tra*, bool);
static bool			par_using(dyn*);
static USHORT		resolve_dtypes(kwwords_t, bool);
static bool			tail_database(act_t, gpre_dbb*);
static void			to_upcase(const TEXT*, TEXT*, int);

static swe* global_whenever[SWE_max];
static swe* global_whenever_list;

static inline bool end_of_command()
{
	return
		(gpreGlob.sw_language != lang_cobol && gpreGlob.token_global.tok_keyword == KW_SEMI_COLON) ||
		(gpreGlob.sw_language == lang_cobol && gpreGlob.token_global.tok_keyword == KW_END_EXEC);
}

static inline bool range_short_integer(const SLONG x)
{
	return (x < 32768 && x >= -32768);
}

static inline bool range_positive_short_integer(const SLONG x)
{
	return (x < 32768 && x >= 0);
}

//____________________________________________________________
//
//		Parse and return a sequel action.
//

act* SQL_action(const TEXT* base_directory)
{
	act* action = NULL;
	const kwwords_t keyword = gpreGlob.token_global.tok_keyword;

	switch (keyword)
	{
	case KW_ALTER:
	case KW_COMMENT:
	case KW_CONNECT:
	case KW_CREATE:
	case KW_DROP:
	case KW_EVENT:
	case KW_GRANT:
	case KW_REVOKE:
	case KW_BEGIN:
	case KW_CLOSE:
	case KW_COMMIT:
	case KW_DECLARE:
	case KW_DELETE:
	case KW_DESCRIBE:
	case KW_DISCONNECT:
	case KW_END:
	case KW_EXECUTE:
	case KW_FETCH:
	case KW_INCLUDE:
	case KW_INSERT:
	case KW_LOCK:
	case KW_OPEN:
	case KW_PREPARE:
	case KW_RELEASE_REQUESTS:
	case KW_ROLLBACK:
	case KW_SELECT:
	case KW_SET:
	case KW_UPDATE:
	case KW_WHENEVER:
	case KW_DATABASE:
		PAR_get_token();
		break;

	default:
		CPR_s_error("SQL operation");
	}

	switch (keyword)
	{
	case KW_ALTER:
		action = act_alter();
		break;

	case KW_BEGIN:
		action = act_d_section(ACT_b_declare);
		break;

	case KW_CLOSE:
		action = act_openclose(ACT_close);
		break;

	case KW_CONNECT:
		action = act_connect();
		break;

	case KW_COMMENT:
		action = act_comment();
		break;

	case KW_COMMIT:
		action = act_transaction(ACT_commit);
		break;

	case KW_CREATE:
		action = act_create();
		break;

	case KW_DATABASE:
		action = PAR_database(true, base_directory);
		break;

	case KW_DROP:
		action = act_drop();
		break;

	case KW_DECLARE:
		action = act_declare();
		break;

	case KW_DELETE:
		action = act_delete();
		break;

	case KW_DESCRIBE:
		action = act_describe();
		break;

	case KW_DISCONNECT:
		action = act_disconnect();
		break;

	case KW_EVENT:
		action = act_event();
		break;

	case KW_END:
		action = act_d_section(ACT_e_declare);
		break;

	case KW_EXECUTE:
		action = act_execute();
		break;

	case KW_FETCH:
		action = act_fetch();
		break;

	case KW_GRANT:
		action = act_grant_revoke(ACT_dyn_grant);
		break;

	case KW_INCLUDE:
		action = act_include();
		break;

	case KW_INSERT:
		action = act_insert();
		break;

	case KW_LOCK:
		action = act_lock();
		break;

	case KW_OPEN:
		action = act_openclose(ACT_open);
		break;

	case KW_PREPARE:
		action = act_prepare();
		break;

	case KW_RELEASE_REQUESTS:
		action = act_release();
		break;

	case KW_REVOKE:
		action = act_grant_revoke(ACT_dyn_revoke);
		break;

	case KW_ROLLBACK:
		action = act_transaction(ACT_rollback);
		break;

	case KW_SELECT:
		action = act_select();
		break;

	case KW_SET:
		action = act_set(base_directory);
		break;

	case KW_UPDATE:
		action = act_update();
		break;

	case KW_WHENEVER:
		action = act_whenever();
		break;
	}

	MSC_match(KW_END_EXEC);
	PAR_end();
	action->act_flags |= ACT_sql;

	return action;
}


//____________________________________________________________
//
//		Given a field datatype, remap it as needed to
//		a user datatype, and set the length field.
//

void SQL_adjust_field_dtype( gpre_fld* field)
{
	if (field->fld_dtype <= dtype_any_text)
	{
		ULONG field_length;
		// Adjust the string data types and their lengths
		if (field->fld_collate)
		{
			if (field->fld_char_length)
			{
				field_length =
					(ULONG) field->fld_char_length * field->fld_collate->intlsym_bytes_per_char;
			}
			else
				field_length = field->fld_length;
			field->fld_collate_id = field->fld_collate->intlsym_collate_id;
			field->fld_charset_id = field->fld_collate->intlsym_charset_id;
			field->fld_ttype = field->fld_collate->intlsym_ttype;
		}
		else if (field->fld_character_set)
		{
			if (field->fld_char_length)
			{
				field_length =
					(ULONG) field->fld_char_length * field->fld_character_set->intlsym_bytes_per_char;
			}
			else
				field_length = field->fld_length;
			field->fld_collate_id = field->fld_character_set->intlsym_collate_id;
			field->fld_charset_id = field->fld_character_set->intlsym_charset_id;
			field->fld_ttype = field->fld_character_set->intlsym_ttype;
		}
		else
		{
			if (field->fld_char_length)
				field_length = (ULONG) field->fld_char_length * 1;
			else
				field_length = field->fld_length;
			field->fld_collate_id = 0;
			field->fld_charset_id = 0;
			field->fld_ttype = 0;
		}

		if (!(field->fld_flags & FLD_meta))
		{
			// field for meta operation?
			// Field isn't for meta-data operation, so adjust it's
			// type definition for local use
			if (field->fld_dtype != dtype_cstring)
			{
				field->fld_dtype =
					(gpreGlob.sw_cstring && field->fld_sub_type != dsc_text_type_fixed) ?
						dtype_cstring : dtype_text;
			}
			if (field->fld_dtype == dtype_cstring)
				field_length++;
			field->fld_length = (USHORT) field_length;
		}
		else
		{
			field->fld_length = (USHORT) field_length;
			if (field->fld_dtype == dtype_varying)
				field_length += sizeof(USHORT);

			if (field_length > MAX_COLUMN_SIZE)
				error("Size of column %s exceeds implementation limit", field->fld_symbol->sym_string);
		}
	}
	else
		switch (field->fld_dtype)
		{
		case dtype_short:
			field->fld_length = sizeof(SSHORT);
			break;

		case dtype_long:
			field->fld_length = sizeof(SLONG);
			break;

		case dtype_real:
			field->fld_length = sizeof(float);
			break;

		case dtype_double:
			field->fld_length = sizeof(double);
			break;

		// Begin sql/date/time/timestamp
		case dtype_sql_date:
			field->fld_length = sizeof(ISC_DATE);
			break;

		case dtype_sql_time:
			field->fld_length = sizeof(ISC_TIME);
			break;

		case dtype_timestamp:
			field->fld_length = sizeof(ISC_TIMESTAMP);
			break;
		// End sql/date/time/timestamp

		case dtype_int64:
			field->fld_length = sizeof(ISC_INT64);
			break;

		case dtype_blob:
			field->fld_length = sizeof(ISC_QUAD);
			field->fld_flags |= FLD_blob;
			if (field->fld_character_set)
			{
				field->fld_charset_id =
					field->fld_character_set->intlsym_charset_id;
				field->fld_ttype = field->fld_character_set->intlsym_ttype;
			}
			break;

		default:
			CPR_bugcheck("datatype not recognized");
			break;
		}
}


//____________________________________________________________
//
//		Initialize (or re-initialize) to process a module.
//

void SQL_init()
{
	global_whenever_list = NULL;

	for (int i = 0; i < SWE_max; i++)
		global_whenever[i] = NULL;

	gpreGlob.module_lc_ctype = gpreGlob.default_lc_ctype;
}


//____________________________________________________________
//
//

void SQL_par_field_collate(gpre_req*, //request,
						   gpre_fld* field)
{
	if (MSC_match(KW_COLLATE))
	{

		if ((field->fld_dtype != dtype_text) &&
			(field->fld_dtype != dtype_cstring) &&
			(field->fld_dtype != dtype_varying))
		{
			PAR_error("COLLATE applies only to character columns");
		}
		if (gpreGlob.token_global.tok_type != tok_ident)
			CPR_s_error("<collation name>");
		gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_collate);
		if (!symbol)
			PAR_error("The named COLLATION was not found");
		field->fld_collate = (intlsym*) symbol->sym_object;

		// Is the collation valid for declared character set?
		// The character set is either declared (fld_character_set) or inferered
		// from the global domain (fld_global & fld_charset_id)


		if ((field->fld_character_set &&
				field->fld_character_set->intlsym_charset_id != field->fld_collate->intlsym_charset_id)
			||
			(field->fld_global && field->fld_charset_id != field->fld_collate->intlsym_charset_id))
		{
			PAR_error("Specified COLLATION is incompatible with column CHARACTER SET");
		}
		PAR_get_token();
	}
}


//____________________________________________________________
//
//		Handle an SQL field datatype definition for
//		field CREATE, DECLARE or ALTER TABLE statement.
//		Also for CAST statement
//

void SQL_par_field_dtype(gpre_req* request, gpre_fld* field, bool is_udf)
{
	bool sql_date = false;

	kwwords_t keyword = gpreGlob.token_global.tok_keyword;
	switch (keyword)
	{
	case KW_SMALLINT:
	case KW_INT:
	case KW_INTEGER:
	case KW_FLOAT:
	case KW_REAL:
	case KW_DOUBLE:
	case KW_LONG:
	// Begin sql/date/time/timestamp
	case KW_TIMESTAMP:
	// End sql/date/time/timestamp
	case KW_CHAR:
	case KW_NCHAR:
	case KW_VARCHAR:
	case KW_DECIMAL:
	case KW_NUMERIC:
	case KW_BLOB:
	case KW_NATIONAL:
		PAR_get_token();
		break;

	case KW_DATE:
		if (gpreGlob.sw_sql_dialect == 2)
			PAR_error("DATE is ambiguous in dialect 2 use SQL DATE or TIMESTAMP");
		PAR_get_token();
		break;

	case KW_TIME:
		if (gpreGlob.sw_sql_dialect == 1)
			CPR_s_error("<data type>");
		PAR_get_token();
		break;

	case KW_SQL:
		if (gpreGlob.sw_sql_dialect == 1)
			CPR_s_error("<data type>");
		PAR_get_token();
		if (gpreGlob.token_global.tok_keyword == KW_DATE)
			PAR_get_token();
		else
			CPR_s_error("<data type>");
		keyword = KW_DATE;
		sql_date = true;
		break;

	case KW_COMPUTED:
		if (is_udf)
			CPR_s_error("<data type>");
		// just return - actual parse is done later
		return;

	default:
		if (is_udf)
		{
			if (keyword == KW_CSTRING)
				PAR_get_token();
			else
				CPR_s_error("<data type>");
		}
		else
		{
			char s[NAME_SIZE];
			SQL_resolve_identifier("<domain name>", s, NAME_SIZE);
			gpre_sym* symbol = MSC_symbol(SYM_field, s, (USHORT) strlen(s), (gpre_ctx*) field);
			field->fld_global = symbol;
			if (!MET_domain_lookup(request, field, s))
				PAR_error("Specified DOMAIN or source column not found");
			PAR_get_token();
			return;
		}
	}

	switch (keyword)
	{
	case KW_SMALLINT:
		field->fld_dtype = dtype_short;
		break;

	case KW_INT:
	case KW_INTEGER:
		field->fld_dtype = dtype_long;
		break;

	case KW_REAL:
		field->fld_dtype = dtype_real;
		break;

	case KW_FLOAT:
		if (MSC_match(KW_LEFT_PAREN))
		{
			const int l = EXP_USHORT_ordinal(true);
			EXP_match_paren();
			if (l < 17)
				field->fld_dtype = dtype_real;
			else
				field->fld_dtype = dtype_double;
		}
		else
			field->fld_dtype = dtype_real;
		break;

	case KW_LONG:
		if (!(gpreGlob.token_global.tok_keyword == KW_FLOAT))
			CPR_s_error("FLOAT");
		PAR_get_token();
		field->fld_dtype = dtype_double;
		break;

	case KW_DOUBLE:
		if (!(gpreGlob.token_global.tok_keyword == KW_PRECISION))
			CPR_s_error("PRECISION");
		PAR_get_token();
		field->fld_dtype = dtype_double;
		break;

	// Begin sql/date/time/timestamp
	case KW_DATE:
	case KW_TIME:
	case KW_TIMESTAMP:
		field->fld_dtype = resolve_dtypes(keyword, sql_date);
		break;
	// End sql/date/time/timestamp

	case KW_NCHAR:
		field->fld_flags |= FLD_national;
		field->fld_dtype = dtype_text;
		if (MSC_match(KW_LEFT_PAREN))
		{
			field->fld_char_length = EXP_pos_USHORT_ordinal(true);
			EXP_match_paren();
		}
		else
			field->fld_char_length = 1;
		break;

	case KW_NATIONAL:
		if (!(gpreGlob.token_global.tok_keyword == KW_CHAR))
			CPR_s_error("CHARACTER");
		PAR_get_token();
		field->fld_flags |= FLD_national;
		// Fall into KW_CHAR
	case KW_CHAR:
		if (MSC_match(KW_VARYING))
		{
			field->fld_dtype = dtype_varying;
			EXP_left_paren(0);
			field->fld_char_length = EXP_pos_USHORT_ordinal(true);
			EXP_match_paren();
			break;
		}
	case KW_CSTRING:
		if (keyword == KW_CSTRING)
		{
			field->fld_dtype = dtype_cstring;
			field->fld_flags |= FLD_meta_cstring;
		}
		else
			field->fld_dtype = dtype_text;
		if (MSC_match(KW_LEFT_PAREN))
		{
			field->fld_char_length = EXP_pos_USHORT_ordinal(true);
			EXP_match_paren();
		}
		else
			field->fld_char_length = 1;
		break;

	case KW_VARCHAR:
		field->fld_dtype = dtype_varying;
		EXP_left_paren(0);
		field->fld_char_length = EXP_pos_USHORT_ordinal(true);
		EXP_match_paren();
		break;


	case KW_NUMERIC:
	case KW_DECIMAL:
		field->fld_dtype = dtype_long;
		field->fld_scale = 0;
		field->fld_precision = 9;
		field->fld_sub_type = (keyword == KW_NUMERIC) ? 1 : 2;
		if (MSC_match(KW_LEFT_PAREN))
		{
			const int p = EXP_USHORT_ordinal(true);
			if ((p <= 0) || (p > 18))
				PAR_error("Precision must be from 1 to 18");

			if ((keyword == KW_NUMERIC) && (p < 5))
				field->fld_dtype = dtype_short;
			else if (p > 9)
			{
				if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5)
					field->fld_dtype = dtype_double;
				else
					field->fld_dtype = dtype_int64;
			}

			if (MSC_match(KW_COMMA))
			{
				const int q = EXP_USHORT_ordinal(true);

				if (q > p)
					PAR_error("Scale can not be greater than precision");
				field->fld_scale = -q;
			}
			field->fld_precision = p;
			EXP_match_paren();
		}
		break;

	case KW_BLOB:
		field->fld_dtype = dtype_blob;
		field->fld_seg_length = DEFAULT_BLOB_SEGMENT_LENGTH;
		if (MSC_match(KW_LEFT_PAREN))
		{
			if (MSC_match(KW_COMMA))
				field->fld_sub_type = EXP_SSHORT_ordinal(true);
			else
			{
				field->fld_seg_length = EXP_USHORT_ordinal(true);
				if (MSC_match(KW_COMMA))
					field->fld_sub_type = EXP_SSHORT_ordinal(true);
			}
			EXP_match_paren();
		}
		else
		{
			if (MSC_match(KW_SUB_TYPE)) {
				field->fld_sub_type = PAR_blob_subtype(request->req_database);
			}
			if (MSC_match(KW_SEGMENT))
			{
				MSC_match(KW_SIZE);
				field->fld_seg_length = EXP_USHORT_ordinal(true);
			}
		}
		break;

	default:
		CPR_s_error("<data type>");
	}

	// Check for array declaration

	if ((keyword != KW_BLOB) && !is_udf && (MSC_match(KW_L_BRCKET)))
	{
		field->fld_array_info = (ary*) MSC_alloc(sizeof(ary));
		par_array(field);
	}

	if (MSC_match(KW_CHAR))
	{
		if ((field->fld_dtype != dtype_text) &&
			(field->fld_dtype != dtype_cstring) &&
			(field->fld_dtype != dtype_varying) &&
			(field->fld_dtype != dtype_blob))
		{
			PAR_error("CHARACTER SET applies only to character columns");
		}

		if (field->fld_dtype == dtype_blob && field->fld_sub_type == isc_blob_untyped)
			field->fld_sub_type = isc_blob_text;

		if (field->fld_dtype == dtype_blob && field->fld_sub_type != isc_blob_text)
			PAR_error("CHARACTER SET applies only to character columns");

		if (field->fld_flags & FLD_national)
			PAR_error("cannot specify CHARACTER SET with NATIONAL");

		if (!MSC_match(KW_SET))
			CPR_s_error("SET");
		if (gpreGlob.token_global.tok_type != tok_ident)
			CPR_s_error("<character set name>");

		gpre_sym* symbol2 = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_charset);
		if (!symbol2)
			PAR_error("The named CHARACTER SET was not found");
		field->fld_character_set = (intlsym*) symbol2->sym_object;
		PAR_get_token();
	}

	if (field->fld_flags & FLD_national)
	{
        gpre_sym* symbol = MSC_find_symbol(HSH_lookup(DEFAULT_CHARACTER_SET_NAME), SYM_charset);
		if (!symbol)
		{
			PAR_error("NATIONAL character set missing");
		}
		field->fld_character_set = (intlsym*) symbol->sym_object;
	}
	else if ((field->fld_dtype <= dtype_any_text ||
		(field->fld_dtype == dtype_blob && field->fld_sub_type == isc_blob_text)) &&
		!field->fld_character_set && !field->fld_collate && request &&
		request->req_database && request->req_database->dbb_def_charset)
	{
		// Use database default character set
		gpre_sym* symbol =
			MSC_find_symbol(HSH_lookup(request->req_database->dbb_def_charset), SYM_charset);
		if (symbol)
		{
			field->fld_character_set = (intlsym*) symbol->sym_object;
		}
		else
			PAR_error("Could not find database default character set");
	}
}


//____________________________________________________________
//
//		Find procedure for request.  If request already has a database,
//		find the procedure in that database only.
//

gpre_prc* SQL_procedure(gpre_req* request,
					   const TEXT* prc_string,
					   const TEXT* db_string,
					   const TEXT* owner_string,
					   bool err_flag)
{
	SCHAR s[ERROR_LENGTH];

	if (db_string && db_string[0])
	{
		// a database was specified for the procedure
		// search the known symbols for the database name

		gpre_sym* symbol = MSC_find_symbol(HSH_lookup(db_string), SYM_database);
		if (!symbol)
			PAR_error("Unknown database specifier.");
		if (request->req_database)
		{
			if ((gpre_dbb*) symbol->sym_object != request->req_database)
				PAR_error("Inconsistent database specifier");
		}
		else
			request->req_database = (gpre_dbb*) symbol->sym_object;
	}

	gpre_prc* procedure = NULL;

	if (request->req_database)
		procedure = MET_get_procedure(request->req_database, prc_string, owner_string);
	else
	{
		// no database was specified, check the metadata for all the databases
		// for the existence of the procedure

		procedure = NULL; // redundant
		for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			gpre_prc* tmp_procedure = MET_get_procedure(db, prc_string, owner_string);
			if (tmp_procedure)
			{
				if (procedure)
				{
					// relation was found in more than one database

					sprintf(s, "PROCEDURE %s is ambiguous", prc_string);
					PAR_error(s);
				}
				else
				{
					procedure = tmp_procedure;
					request->req_database = db;
				}
			}
		}
	}

	if (!procedure)
	{
		if (!err_flag)
			return NULL;
		if (owner_string[0])
			sprintf(s, "PROCEDURE %s.%s not defined", owner_string, prc_string);
		else
			sprintf(s, "PROCEDURE %s not defined", prc_string);
		PAR_error(s);
	}

	return procedure;
}


//____________________________________________________________
//
//		Find relation for request.  If request already has a database,
//		find the relation in that database only.
//

gpre_rel* SQL_relation(gpre_req* request,
					  const TEXT* rel_string,
					  const TEXT* db_string,
					  const TEXT* owner_string,
					  bool err_flag)
{
	if (db_string && db_string[0])
	{
		// a database was specified for the relation,
		// search the known symbols for the database name

		gpre_sym* symbol = MSC_find_symbol(HSH_lookup(db_string), SYM_database);
		if (!symbol)
			PAR_error("Unknown database specifier.");
		if (request->req_database)
		{
			if ((gpre_dbb*) symbol->sym_object != request->req_database)
				PAR_error("Inconsistent database specifier");
		}
		else
			request->req_database = (gpre_dbb*) symbol->sym_object;
	}

	SCHAR s[ERROR_LENGTH];
	gpre_rel* relation = NULL;

	if (request->req_database)
		relation = MET_get_relation(request->req_database, rel_string, owner_string);
	else
	{
		// no database was specified, check the metadata for all the databases
		// for the existence of the relation

		relation = NULL; // redundant
		for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			gpre_rel* tmp_relation = MET_get_relation(db, rel_string, owner_string);
			if (tmp_relation)
			{
				if (relation)
				{
					// relation was found in more than one database

					sprintf(s, "TABLE %s is ambiguous", rel_string);
					PAR_error(s);
				}
				else
				{
					relation = tmp_relation;
					request->req_database = db;
				}
			}
		}
	}

	if (!relation)
	{
		if (!err_flag)
			return (NULL);
		if (owner_string[0])
			sprintf(s, "TABLE %s.%s not defined", owner_string, rel_string);
		else
			sprintf(s, "TABLE %s not defined", rel_string);
		PAR_error(s);
	}

	return relation;
}


//____________________________________________________________
//
//		Get a relation name (checking for database specifier)
//

void SQL_relation_name(TEXT* r_name, TEXT* db_name, TEXT* owner_name)
{
	db_name[0] = 0;
	owner_name[0] = 0;

	SQL_resolve_identifier("<Database name>", NULL, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Database alias too long");

	gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_database);
	if (symbol)
	{
		if (strlen(symbol->sym_name) >= unsigned(NAME_SIZE))
			PAR_error("Database alias too long");

		strcpy(db_name, symbol->sym_name); // this is the alias, not the path
		PAR_get_token();
		if (!MSC_match(KW_DOT))
			CPR_s_error(". (period)");
	}

	SQL_resolve_identifier("<Table name>", NULL, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Table or owner name too long");

	strcpy(r_name, gpreGlob.token_global.tok_string);
	PAR_get_token();

	if (MSC_match(KW_DOT))
	{
		// the table name was really a owner specifier
		strcpy(owner_name, r_name);
		SQL_resolve_identifier("<Table name>", NULL, NAME_SIZE);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("TABLE name too long");

		strcpy(r_name, gpreGlob.token_global.tok_string);
		PAR_get_token();
	}
}


//____________________________________________________________
//
//		Extract SQL var
//

TEXT* SQL_var_or_string(bool string_only)
{

	if ((gpreGlob.token_global.tok_type != tok_sglquoted && gpreGlob.sw_sql_dialect == 3) ||
		(!isQuoted(gpreGlob.token_global.tok_type) && gpreGlob.sw_sql_dialect == 1))
	{
		if (string_only)
			CPR_s_error("<quoted string>");
		if (!MSC_match(KW_COLON))
			CPR_s_error("<colon> or <quoted string>");
	}
	return PAR_native_value(false, false);
}


//____________________________________________________________
//
//		Handle an SQL alter statement.
//

static act* act_alter()
{

	switch (gpreGlob.token_global.tok_keyword)
	{

	case KW_DATABASE:
	case KW_SCHEMA:
		return act_alter_database();

	case KW_DOMAIN:
		return act_alter_domain();

	case KW_INDEX:
		PAR_get_token();
		return act_alter_index();

	case KW_STOGROUP:
		PAR_error("ALTER STOGROUP not supported");

	case KW_TABLE:
		PAR_get_token();
		return act_alter_table();

	case KW_TABLESPACE:
		PAR_error("ALTER TABLESPACE not supported");

	default:
		PAR_error("Invalid ALTER request");
	}
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Handle an SQL alter database statement
//

static act* act_alter_database()
{
	gpre_req* request = MSC_request(REQ_ddl);
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only alter database in context of single database");

	PAR_get_token();

	// create action block
	act* action = MSC_action(request, ACT_alter_database);
	action->act_whenever = gen_whenever();
	gpre_dbb* database = (gpre_dbb*) MSC_alloc(DBB_LEN);
	action->act_object = (ref*) database;

	bool logdefined = false;	// this var was undefined

	while (true)
	{
		if (MSC_match(KW_DROP))
		{
			if (MSC_match(KW_LOG_FILE))
				; // ignore DROP LOG database->dbb_flags |= DBB_drop_log;
			else if (MSC_match(KW_CASCADE))
				; // ignore DROP CASCADE database->dbb_flags |= DBB_cascade;
			else
				PAR_error("only log file can be dropped");
		}
		else if (MSC_match(KW_ADD))
		{
			if (MSC_match(KW_FILE))
			{
				do {
					gpre_file* file = define_file();
					file->fil_next = database->dbb_files;
					database->dbb_files = file;
				} while (MSC_match(KW_FILE));
			}
			else if (MSC_match(KW_LOG_FILE))
			{
				if (logdefined)
					PAR_error("log redefinition");
				logdefined = true;
				if (MSC_match(KW_LEFT_PAREN))
				{
					while (true)
					{
						gpre_file* logfile = define_log_file();
						logfile->fil_next = database->dbb_logfiles;
						database->dbb_logfiles = logfile;
						if (!MSC_match(KW_COMMA))
						{
							EXP_match_paren();
							break;
						}
					}

					if (MSC_match(KW_OVERFLOW))
						define_log_file();
					else
						PAR_error("Overflow log specification required for this configuration");

				}
				else if (MSC_match(KW_BASE_NAME))
				{
					database->dbb_flags |= DBB_log_serial;
					database->dbb_logfiles = define_log_file();
				}
			}
		}
		else if (MSC_match(KW_SET))
		{
			while (true)
			{
				if (MSC_match(KW_CHECK_POINT_LEN))
				{
					MSC_match(KW_EQUALS);
					EXP_ULONG_ordinal(true); // skip
					MSC_match(KW_COMMA);
				}
				else if (MSC_match(KW_NUM_LOG_BUFS))
				{
					MSC_match(KW_EQUALS);
					EXP_USHORT_ordinal(true); // skip
					MSC_match(KW_COMMA);
				}
				else if (MSC_match(KW_LOG_BUF_SIZE))
				{
					MSC_match(KW_EQUALS);
					EXP_USHORT_ordinal(true); // skip
					MSC_match(KW_COMMA);
				}
				else if (MSC_match(KW_GROUP_COMMIT_WAIT))
				{
					MSC_match(KW_EQUALS);
					EXP_ULONG_ordinal(true); // skip
					MSC_match(KW_COMMA);
				}
				else
					break;
			}
		}
		else
			break;
	}
	return action;
}


//____________________________________________________________
//
//		Handle altering of a domain (global field).
//

static act* act_alter_domain()
{
	// create request block

	gpre_req* request = MSC_request(REQ_ddl);
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only ALTER a domain in context of single database");

	// create action block

	act* action = MSC_action(request, ACT_alter_domain);

	PAR_get_token();
	gpre_fld* field = make_field(0);
	cnstrt** cnstrt_ptr = &field->fld_constraints;

	// Check if default value was specified

	while (!end_of_command())
	{
		if (MSC_match(KW_SET))
		{
			if (gpreGlob.token_global.tok_keyword == KW_DEFAULT)
			{
				field->fld_default_source = CPR_start_text();
				PAR_get_token();
			}
			else
				CPR_s_error("DEFAULT");

			if (MSC_match(KW_USER))
				field->fld_default_value = MSC_node(nod_user_name, 0);
			else if (MSC_match(KW_NULL))
				field->fld_default_value = MSC_node(nod_null, 0);
			else
			{
				if (MSC_match(KW_MINUS))
				{
					if (gpreGlob.token_global.tok_type != tok_number)
						CPR_s_error("<number>");

					gpre_nod* literal_node = EXP_literal();
					field->fld_default_value = MSC_unary(nod_negate, literal_node);
				}
				else if ((field->fld_default_value = EXP_literal()) == NULL)
					CPR_s_error("<constant>");
			}
			CPR_end_text(field->fld_default_source);
		}
		else if (MSC_match(KW_ADD))
		{
			MSC_match(KW_CONSTRAINT);
			if (gpreGlob.token_global.tok_keyword == KW_CHECK)
			{
				cnstrt* cnstrt_str = par_field_constraint(request, field);
				*cnstrt_ptr = cnstrt_str;
				cnstrt_ptr = &cnstrt_str->cnstrt_next;
			}
			else
				PAR_error("Invalid constraint.");
		}
		else if (MSC_match(KW_DROP))
		{
			if (MSC_match(KW_CONSTRAINT))
			{
				cnstrt* cnstrt_str = (cnstrt*) MSC_alloc(CNSTRT_LEN);
				cnstrt_str->cnstrt_flags |= CNSTRT_delete;
				*cnstrt_ptr = cnstrt_str;
				cnstrt_ptr = &cnstrt_str->cnstrt_next;
			}
			else if (MSC_match(KW_DEFAULT)) {
				field->fld_default_value = MSC_node(nod_erase, 0);
			}
			else
				PAR_error("Invalid attribute for DROP");
		}
		else
			CPR_s_error("SET, ADD, or DROP");

	}

	action->act_whenever = gen_whenever();
	action->act_object = (ref*) field;

	return action;
}


//____________________________________________________________
//
//		Handle an SQL alter index statement.
//

static act* act_alter_index()
{
	// create request block

	gpre_req* request = MSC_request(REQ_ddl);

	char i_name[NAME_SIZE + 1];
	SQL_resolve_identifier("<index name>", i_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Index name too long");

	PAR_get_token();

	gpre_index* index = make_index(request, i_name);

	if (MSC_match(KW_ACTIVE))
		index->ind_flags |= IND_active;
	else if (MSC_match(KW_INACTIVE))
		index->ind_flags |= IND_inactive;
	else
		PAR_error("Unsupported ALTER INDEX option");

	act* action = MSC_action(request, ACT_alter_index);
	action->act_object = (ref*) index;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Handle an SQL alter table statement.
//

static act* act_alter_table()
{
	// create request block

	gpre_req* request = MSC_request(REQ_ddl);

	// get table name and create relation block

	gpre_rel* relation = par_relation(request);

	// CHECK Constraints require the context to be set to the
	// current relation

	gpre_ctx* context = MSC_context(request);
	request->req_contexts = context;
	context->ctx_relation = relation;

	// Reserve context 1 for relation on which constraint is
	// being defined

	context->ctx_internal++;
	request->req_internal++;

	// create action block

	act* action = MSC_action(request, ACT_alter_table);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) relation;

	// parse action list and create corresponding field blocks

	gpre_fld** ptr = &relation->rel_fields;
	cnstrt** cnstrt_ptr = &relation->rel_constraints;
	cnstrt* cnstrt_str;
	gpre_fld* field;

	while (!end_of_command())
	{
		if (MSC_match(KW_ADD))
		{
			switch (gpreGlob.token_global.tok_keyword)
			{
			case KW_CONSTRAINT:
			case KW_PRIMARY:
			case KW_UNIQUE:
			case KW_FOREIGN:
			case KW_CHECK:
				cnstrt_str = par_table_constraint(request);
				*cnstrt_ptr = cnstrt_str;
				cnstrt_ptr = &cnstrt_str->cnstrt_next;
				break;

			default:
				field = par_field(request, relation);
				*ptr = field;
				ptr = &field->fld_next;
			}
		}
		else if (MSC_match(KW_DROP))
		{
			if (gpreGlob.token_global.tok_keyword == KW_CONSTRAINT)
			{
				PAR_get_token();
				cnstrt_str = (cnstrt*) MSC_alloc(CNSTRT_LEN);
				cnstrt_str->cnstrt_flags |= CNSTRT_delete;
				cnstrt_str->cnstrt_name = (STR) MSC_alloc(NAME_SIZE + 1);
				SQL_resolve_identifier("<constraint name>",
									   cnstrt_str->cnstrt_name->str_string, NAME_SIZE + 1);
				if (gpreGlob.token_global.tok_length >= NAME_SIZE)
					PAR_error("Constraint name too long");

				*cnstrt_ptr = cnstrt_str;
				cnstrt_ptr = &cnstrt_str->cnstrt_next;
				PAR_get_token();
			}
			else
			{
				field = make_field(relation);
				field->fld_flags |= FLD_delete;
				*ptr = field;
				ptr = &field->fld_next;

				// Fix for bug 8054:
				// [CASCADE | RESTRICT] syntax is available in IB4.5, but not
				// required until v5.0.
				// Option CASCADE causes an error :
				// unsupported construct
				// Option RESTRICT is default behaviour.
				if (gpreGlob.token_global.tok_keyword == KW_CASCADE)
				{
					PAR_error("Unsupported construct CASCADE");
					PAR_get_token();
				}
				else if (gpreGlob.token_global.tok_keyword == KW_RESTRICT) {
					PAR_get_token();
				}
			}
		}
		else
			CPR_s_error("ADD or DROP");
		if (!MSC_match(KW_COMMA))
			break;
	}

	return action;
}


//____________________________________________________________
//
//		Handle an SQL comment statement.
//			Reject
//

static act* act_comment()
{
	PAR_error("SQL COMMENT ON request not allowed");
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Parse a CONNECT statement.
//

static act* act_connect()
{
	act* action = MSC_action(0, ACT_ready);
	action->act_whenever = gen_whenever();
	bool need_handle = false;
	const USHORT default_buffers = 0; // useless?

	MSC_match(KW_TO);

	if (!MSC_match(KW_ALL) && !MSC_match(KW_DEFAULT))
	{
		while (true)
		{
			rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
			ready->rdy_next = (rdy*) action->act_object;
			action->act_object = (ref*) ready;

			gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
			if (!symbol || symbol->sym_type != SYM_database)
			{
				ready->rdy_filename = SQL_var_or_string(false);
				if (MSC_match(KW_AS))
					need_handle = true;
			}

			symbol = gpreGlob.token_global.tok_symbol;
			if (!symbol || symbol->sym_type != SYM_database)
			{
				if (!gpreGlob.isc_databases || gpreGlob.isc_databases->dbb_next || need_handle)
				{
					need_handle = false;
					CPR_s_error("<database handle>");
				}
				ready->rdy_database = dup_dbb(gpreGlob.isc_databases);
			}

			need_handle = false;
			if (!ready->rdy_database)
			{
				ready->rdy_database = dup_dbb((gpre_dbb*) symbol->sym_object);
				PAR_get_token();
			}

			// pick up the possible parameters, in any order

			USHORT buffers = 0;
			gpre_dbb* db = ready->rdy_database;
			connect_opts(&db->dbb_r_user, &db->dbb_r_password,
						 &db->dbb_r_sql_role, &db->dbb_r_lc_messages, &buffers);

			gpre_req* request = NULL;
			if (buffers)
				request = PAR_set_up_dpb_info(ready, action, buffers);

			// if there are any options that take host variables as arguments,
			// make sure that we generate variables for the request so that the
			// dpb can be extended at runtime

			if (db->dbb_r_user || db->dbb_r_password || db->dbb_r_sql_role ||
				db->dbb_r_lc_messages || db->dbb_r_lc_ctype)
			{
				if (!request)
					request = PAR_set_up_dpb_info(ready, action, default_buffers);
				request->req_flags |= REQ_extend_dpb;
			}

			// ...and if there are compile time user or password specified,
			// make sure there will be a dpb generated for them

			if (!request && (db->dbb_c_user || db->dbb_c_password || db->dbb_c_sql_role ||
							 db->dbb_c_lc_messages || db->dbb_c_lc_ctype))
			{
				request = PAR_set_up_dpb_info(ready, action, default_buffers);
			}

			if (!MSC_match(KW_COMMA))
				break;
		}
	}

	if (action->act_object)
		return action;

	// No explicit databases -- pick up all known

	const TEXT* lc_messages = NULL;
	const TEXT *user = NULL, *password = NULL, *sql_role = NULL;
	// The original logic suggests that "buffers" should be shared between the
	// two sections of code. However, if the loop above executes, action->act_object
	// is not null and therefore, the function returns before this point.
	// Furthermore, connect_opts() below refills the values.
	USHORT buffers = 0;

	connect_opts(&user, &password, &sql_role, &lc_messages, &buffers);

	for (const gpre_dbb* db_iter = gpreGlob.isc_databases; db_iter; db_iter = db_iter->dbb_next)
		if (db_iter->dbb_runtime || !(db_iter->dbb_flags & DBB_sqlca))
		{
			rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
			ready->rdy_next = (rdy*) action->act_object;
			action->act_object = (ref*) ready;
			ready->rdy_database = dup_dbb(db_iter);
		}

	if (!action->act_object)
		PAR_error("no database available to CONNECT");
	else
		for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
		{
		    gpre_req* request;
			if (buffers)
				request = PAR_set_up_dpb_info(ready, action, buffers);
			else
				request = ready->rdy_request;

			// if there are any options that take host variables as arguments,
			// make sure that we generate variables for the request so that the
			// dpb can be extended at runtime

			gpre_dbb* db = ready->rdy_database;
			if (user || password || lc_messages || db->dbb_r_lc_ctype)
			{
				db->dbb_r_user = user;
				db->dbb_r_password = password;
				db->dbb_r_lc_messages = lc_messages;
				if (!request)
					request = PAR_set_up_dpb_info(ready, action, default_buffers);
				request->req_flags |= REQ_extend_dpb;
			}

			// ...and if there are compile time user or password specified,
			// make sure there will be a dpb generated for them

			if (!request && (db->dbb_c_user || db->dbb_c_password || db->dbb_c_sql_role ||
							 db->dbb_c_lc_ctype || db->dbb_c_lc_messages))
			{
				request = PAR_set_up_dpb_info(ready, action, default_buffers);
			}
		}

	return action;
}


//____________________________________________________________
//
//		Handle an SQL create statement.
//

static act* act_create()
{
	if (MSC_match(KW_DATABASE) || MSC_match(KW_SCHEMA))
		return act_create_database();

	if (MSC_match(KW_DOMAIN))
		return act_create_domain();

	if (MSC_match(KW_GENERATOR))
		return act_create_generator();

	if (MSC_match(KW_SHADOW))
		return act_create_shadow();

	if (MSC_match(KW_STOGROUP))
		PAR_error("CREATE STOGROUP not supported");

	if (MSC_match(KW_SYNONYM))
		PAR_error("CREATE SYNONYM not supported");

	if (MSC_match(KW_TABLE))
		return act_create_table();

	if (MSC_match(KW_TABLESPACE))
		PAR_error("CREATE TABLESPACE not supported");

	if (MSC_match(KW_VIEW))
		return (act_create_view());

	const kwwords_t tok_kw = gpreGlob.token_global.tok_keyword;
	if (tok_kw == KW_UNIQUE || tok_kw == KW_ASCENDING || tok_kw == KW_DESCENDING || tok_kw == KW_INDEX)
	{
		bool descending = false;
		bool unique = false;
		while (true)
		{
			if (MSC_match(KW_UNIQUE))
				unique = true;
			else if (MSC_match(KW_ASCENDING))
				descending = false;
			else if (MSC_match(KW_DESCENDING))
				descending = true;
			else if (MSC_match(KW_INDEX))
				return act_create_index(unique, descending);
			else
				break;
		}
	}

	PAR_error("Invalid CREATE request");
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Handle an SQLish create database statement.
//

static act* act_create_database()
{
	bool dummy;

	if (gpreGlob.isc_databases && gpreGlob.isc_databases->dbb_next)
		PAR_error("CREATE DATABASE only allowed in context of a single database");

	if (!gpreGlob.isc_databases)
	{
		// generate a dummy db

		dummy = true;
		gpreGlob.isc_databases = (gpre_dbb*) MSC_alloc_permanent(DBB_LEN);

		// allocate symbol block

		gpre_sym* symbol = (gpre_sym*) MSC_alloc_permanent(SYM_LEN);

		// make it the default database

		symbol->sym_type = SYM_database;
		symbol->sym_object = (gpre_ctx*) gpreGlob.isc_databases;
		symbol->sym_string = gpreGlob.database_name;

		// database block points to the symbol block

		gpreGlob.isc_databases->dbb_name = symbol;
		gpreGlob.isc_databases->dbb_filename = NULL;
		gpreGlob.isc_databases->dbb_c_lc_ctype = gpreGlob.module_lc_ctype;
	}
	else
		dummy = false;

	// get database name

	gpre_dbb* db = NULL;
	if (isQuoted(gpreGlob.token_global.tok_type))
	{
		db = dup_dbb(gpreGlob.isc_databases);
		TEXT* string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		db->dbb_filename = string;
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
		if (!gpreGlob.isc_databases->dbb_filename)
			gpreGlob.isc_databases->dbb_filename = string;
		PAR_get_token();
	}
	else
		CPR_s_error("<quoted database name>");

	// Create a request for generating DYN to add files to database.

	//gpre_req* request = MSC_request (REQ_create_database);
	gpre_req* request = MSC_request(REQ_ddl);
	request->req_flags |= REQ_sql_database_dyn;
	request->req_database = db;

	// create action block

	act* action = MSC_action(request, ACT_create_database);

	mdbb* mdb = (mdbb*) MSC_alloc(sizeof(mdbb));
	mdb->mdbb_database = db;
	action->act_object = (ref*) mdb;
	action->act_whenever = gen_whenever();

	if (dummy)
	{
		// Create a ACT_database action

		action->act_rest = MSC_action(0, ACT_database);
		action->act_rest->act_flags |= ACT_mark;
	}

	// Get optional specifications

	const bool extend_dpb = tail_database(ACT_create_database, db);

	// Create a request to generate dpb
#ifdef GPRE_ADA
	gpreGlob.ada_flags |= gpreGlob.ADA_create_database;
#endif

	request = MSC_request(REQ_create_database);
	request->req_actions = action;
	mdb->mdbb_dpb_request = request;
	request->req_database = db;
	if (extend_dpb)
		request->req_flags |= REQ_extend_dpb;

	return action;
}


//____________________________________________________________
//
//		Handle creation of a domain (global field).
//

static act* act_create_domain()
{
	// create request block

	gpre_req* request = MSC_request(REQ_ddl);
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only CREATE DOMAIN in context of single database");

	// create action block

	act* action = MSC_action(request, ACT_create_domain);

	gpre_fld* field = make_field(0);
	MSC_match(KW_AS);
	SQL_par_field_dtype(request, field, false);

	// Check if default value was specified

	if (gpreGlob.token_global.tok_keyword == KW_DEFAULT)
	{
		field->fld_default_source = CPR_start_text();
		PAR_get_token();

		if (MSC_match(KW_USER))
			field->fld_default_value = MSC_node(nod_user_name, 0);
		else if (MSC_match(KW_NULL))
			field->fld_default_value = MSC_node(nod_null, 0);
		else
		{
			if (MSC_match(KW_MINUS))
			{
				if (gpreGlob.token_global.tok_type != tok_number)
					CPR_s_error("<number>");

				gpre_nod* literal_node = EXP_literal();
				field->fld_default_value = MSC_unary(nod_negate, literal_node);
			}
			else if ((field->fld_default_value = EXP_literal()) == NULL)
				CPR_s_error("<constant>");
		}
		CPR_end_text(field->fld_default_source);
	}


	// Check for any column level constraints

	cnstrt** cnstrt_ptr = &field->fld_constraints;
	bool in_constraints = true;

	while (in_constraints)
	{
		switch (gpreGlob.token_global.tok_keyword)
		{
		case KW_CONSTRAINT:
		case KW_CHECK:
		case KW_NOT:
			*cnstrt_ptr = par_field_constraint(request, field);
			cnstrt_ptr = &(*cnstrt_ptr)->cnstrt_next;
			break;

		default:
			in_constraints = false;
			break;
		}
	}

	SQL_par_field_collate(request, field);
	SQL_adjust_field_dtype(field);

	action->act_whenever = gen_whenever();
	action->act_object = (ref*) field;

	return action;
}


//____________________________________________________________
//
//		Handle an SQL create generator statement
//

static act* act_create_generator()
{
	TEXT* generator_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
	SQL_resolve_identifier("<generator>", generator_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Generator name too long");

	gpre_req* request = MSC_request(REQ_ddl);
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only CREATE GENERATOR in context of single database");

	// create action block
	act* action = MSC_action(request, ACT_create_generator);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) generator_name;

	PAR_get_token();
	return action;
}


//____________________________________________________________
//
//		Handle an SQL create index statement.
//

static act* act_create_index(bool dups, bool descending)
{
	// create request block

	gpre_req* request = MSC_request(REQ_ddl);

	// get index and table names and create index and relation blocks

	SCHAR i_name[NAME_SIZE + 1];
	SQL_resolve_identifier("<index name>", i_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Index name too long");

	PAR_get_token();

	if (!MSC_match(KW_ON))
		CPR_s_error("ON");

	gpre_rel* relation = par_relation(request);

	gpre_index* index = make_index(request, i_name);
	index->ind_relation = relation;
	index->ind_flags |= dups ? IND_dup_flag : 0;
	index->ind_flags |= descending ? IND_descend : 0;

	// create action block

	act* action = MSC_action(request, ACT_create_index);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) index;

	// parse field list and create corresponding field blocks

	EXP_left_paren(0);
	gpre_fld** ptr = &index->ind_fields;

	for (;;)
	{
		*ptr = make_field(relation);
		ptr = &(*ptr)->fld_next;
		if (!MSC_match(KW_COMMA))
			break;
	}

	EXP_match_paren();

	return action;
}


//____________________________________________________________
//
//		Handle an SQL create shadow statement
//

static act* act_create_shadow()
{
	gpre_req* request = MSC_request(REQ_ddl);
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only CREATE SHADOW in context of single database");

	act* action = MSC_action(request, ACT_create_shadow);
	action->act_whenever = gen_whenever();
	const SLONG shadow_number = EXP_USHORT_ordinal(true);

	USHORT file_flags = 0;

	if (!range_positive_short_integer(shadow_number))
		PAR_error("Shadow number out of range");

	if (MSC_match(KW_MANUAL))
		file_flags |= FIL_manual;
	else
		MSC_match(KW_AUTO);

	if (MSC_match(KW_CONDITIONAL))
		file_flags |= FIL_conditional;

	gpre_file* file = define_file();
	gpre_file* file_list = file;
	if (file->fil_start)
		PAR_error("Can not specify file start for first file");

	SLONG length = file->fil_length;
	file->fil_flags = file_flags;
	file->fil_shadow_number = (SSHORT) shadow_number;

	while (MSC_match(KW_FILE))
	{
		file = define_file();
		if (!length && !file->fil_start)
		{
			TEXT err_string[1024];
			sprintf(err_string,
					"Preceding file did not specify length, so %s must include starting page number",
					file->fil_name);
			PAR_error(err_string);
		}
		length = file->fil_length;
		file->fil_flags = file_flags;
		file->fil_next = file_list;
		file_list = file;
	}
	action->act_object = (ref*) file_list;
	return action;
}


//____________________________________________________________
//
//		Handle an SQL create table statement.
//

static act* act_create_table()
{
	gpre_req* request = MSC_request(REQ_ddl);
	gpre_rel* relation = par_relation(request);

	if (MSC_match(KW_EXTERNAL))
	{
		TEXT* string = NULL;
		MSC_match(KW_FILE);
		if (isQuoted(gpreGlob.token_global.tok_type))
		{
			string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
			relation->rel_ext_file = string;
			MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
			PAR_get_token();
		}
		else
			CPR_s_error("<quoted filename>");

		if (!check_filename(string))
			PAR_error("node name not permitted");	// a node name is not permitted in external file name
	}

	// CHECK Constraints require the context to be set to the
	// current relation

	gpre_ctx* context = MSC_context(request);
	request->req_contexts = context;
	context->ctx_relation = relation;

	// Reserve context 1 for relation on which constraint is
	// being defined
	context->ctx_internal++;
	request->req_internal++;

	// create action block

	act* action = MSC_action(request, ACT_create_table);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) relation;

	EXP_left_paren(0);
	gpre_fld** ptr = &relation->rel_fields;
	cnstrt** cnstrt_ptr = &relation->rel_constraints;

	for (;;)
	{
		switch (gpreGlob.token_global.tok_keyword)
		{
		case KW_CONSTRAINT:
		case KW_PRIMARY:
		case KW_UNIQUE:
		case KW_FOREIGN:
		case KW_CHECK:
			*cnstrt_ptr = par_table_constraint(request);
			cnstrt_ptr = &(*cnstrt_ptr)->cnstrt_next;
			break;

		default:
			// parse field list and create corresponding field blocks

			*ptr = par_field(request, relation);
			ptr = &(*ptr)->fld_next;
		}
		if (!MSC_match(KW_COMMA))
			break;
	}
	EXP_match_paren();

	return action;
}


//____________________________________________________________
//
//		Handle an SQL create view statement.
//

static act* act_create_view()
{
	gpre_req* request = MSC_request(REQ_ddl);
	gpre_rel* relation = par_relation(request);

	// create action block

	act* action = MSC_action(request, ACT_create_view);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) relation;

	// if field list is present parse it and create corresponding field blocks

	if (MSC_match(KW_LEFT_PAREN))
	{
		gpre_fld** ptr = &relation->rel_fields;
		for (;;)
		{
			*ptr = make_field(relation);
			ptr = &(*ptr)->fld_next;
			if (MSC_match(KW_RIGHT_PAREN))
				break;
			MSC_match(KW_COMMA);
		}
	}

	// skip 'AS SELECT'

	if (!MSC_match(KW_AS))
		CPR_s_error("AS");

	relation->rel_view_text = CPR_start_text();
	if (!MSC_match(KW_SELECT))
		CPR_s_error("SELECT");

	// reserve context variable 0 for view

	request->req_internal++;

	// parse the view SELECT

	gpre_rse* select = SQE_select(request, true);
	relation->rel_view_rse = select;
	EXP_rse_cleanup(select);

	if (MSC_match(KW_WITH))
	{
		if (!MSC_match(KW_CHECK))
			CPR_s_error("CHECK");
		if (!MSC_match(KW_OPTION))
			CPR_s_error("OPTION");
		relation->rel_flags |= REL_view_check;
	}

	CPR_end_text(relation->rel_view_text);

	return action;
}


//____________________________________________________________
//
//		Recognize BEGIN/END DECLARE SECTION,
//		and mark it as a good place to put miscellaneous
//		global routine stuff.
//

static act* act_d_section(act_t type)
{
	if (!MSC_match(KW_DECLARE))
		CPR_s_error("DECLARE SECTION");

	if (!MSC_match(KW_SECTION))
		CPR_s_error("SECTION");

	MSC_match(KW_SEMI_COLON);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = type;

	if (type == ACT_b_declare)
		gpreGlob.cur_routine = action; // Hmm, global var.

	if (!gpreGlob.isc_databases)
	{
		// allocate database block and link to db chain

		gpreGlob.isc_databases = (gpre_dbb*) MSC_alloc_permanent(DBB_LEN);

		// allocate symbol block

		gpre_sym* symbol = (gpre_sym*) MSC_alloc_permanent(SYM_LEN);

		// make it a database, specifically this one

		symbol->sym_type = SYM_database;
		symbol->sym_object = (gpre_ctx*) gpreGlob.isc_databases;
		symbol->sym_string = gpreGlob.database_name;

		// database block points to the symbol block

		gpreGlob.isc_databases->dbb_name = symbol;
		gpreGlob.isc_databases->dbb_filename = NULL;
		gpreGlob.isc_databases->dbb_flags = DBB_sqlca;
		gpreGlob.isc_databases->dbb_c_lc_ctype = gpreGlob.module_lc_ctype;
		if (gpreGlob.sw_external)
			gpreGlob.isc_databases->dbb_scope = DBB_EXTERN;
	}
	else
	{
		// Load up the symbol (hash) table with relation names from this database

		MET_load_hash_table(gpreGlob.isc_databases);
	}

	HSH_insert(gpreGlob.isc_databases->dbb_name);

	if (MSC_match(KW_SQL))
	{
		if (!MSC_match(KW_NAMES))
			CPR_s_error("NAMES ARE");
		if (!MSC_match(KW_ARE))
			CPR_s_error("NAMES ARE");
		gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_charset);
		if (!symbol)
			PAR_error("The named CHARACTER SET was not found");
		if (gpreGlob.module_lc_ctype && !strcmp(gpreGlob.module_lc_ctype, symbol->sym_string))
			PAR_error("Duplicate specification of module CHARACTER SET.");
		gpreGlob.module_lc_ctype = symbol->sym_string;
		gpreGlob.isc_databases->dbb_c_lc_ctype = symbol->sym_string;
		CPR_token();
	}

	return action;
}


//____________________________________________________________
//
//		Parse the SQL cursor declaration.
//

static act* act_declare()
{
	gpre_dbb* db = NULL;

	if (gpreGlob.token_global.tok_symbol && (gpreGlob.token_global.tok_symbol->sym_type == SYM_database))
	{
		// must be a database specifier in a DECLARE TABLE statement

		db = (gpre_dbb*) gpreGlob.token_global.tok_symbol->sym_object;
		PAR_get_token();
		if (!MSC_match(KW_DOT))
			CPR_s_error(". (period)");
		gpre_sym* symbol = PAR_symbol(SYM_dummy);
		if (gpreGlob.token_global.tok_keyword != KW_TABLE)
			CPR_s_error("TABLE");
		return (act_declare_table(symbol, db));
	}

	switch (gpreGlob.token_global.tok_keyword)
	{
	case KW_FILTER:
		return (act_declare_filter());
		break;

	case KW_EXTERNAL:
		PAR_get_token();
		if (MSC_match(KW_FUNCTION))
			return (act_declare_udf());

		CPR_s_error("FUNCTION");
		break;
	}

	bool delimited = false;

	{ // Scope
		TEXT t_str[MAX_CURSOR_SIZE];
		SQL_resolve_identifier("<Cursor Name>", t_str, MAX_CURSOR_SIZE);
		gpre_sym* symb = NULL;
		if (gpreGlob.token_global.tok_type == tok_dblquoted)
		{
			delimited = true;
			symb = HSH_lookup(t_str);
		}
		else {
			symb = HSH_lookup2(t_str);
		}
		if (symb &&
			(symb->sym_type == SYM_cursor || symb->sym_type == SYM_delimited_cursor))
		{
			char s[ERROR_LENGTH];
			fb_utils::snprintf(s, sizeof(s), "symbol %s is already in use", t_str);
			PAR_error(s);
		}
	} // end scope

	act* action = NULL;
	gpre_sym* symbol = PAR_symbol(SYM_cursor);

	switch (gpreGlob.token_global.tok_keyword)
	{
	case KW_TABLE:
		return (act_declare_table(symbol, 0));

	case KW_CURSOR:
		PAR_get_token();
		if (!MSC_match(KW_FOR))
			CPR_s_error("FOR");
		if (MSC_match(KW_SELECT))
		{
			gpre_req* request = MSC_request(REQ_cursor);
			request->req_flags |= REQ_sql_cursor | REQ_sql_declare_cursor;
			symbol->sym_object = (gpre_ctx*) request;
			action = MSC_action(request, ACT_cursor);
			action->act_object = (ref*) symbol;
			symbol->sym_type = delimited ? SYM_delimited_cursor : SYM_cursor;
			request->req_rse = SQE_select(request, false);
			EXP_rse_cleanup(request->req_rse);
		}
		else if (MSC_match(KW_READ))
		{
			action = act_open_blob(ACT_blob_open, symbol);
			symbol->sym_type = delimited ? SYM_delimited_cursor : SYM_cursor;
		}
		else if (MSC_match(KW_INSERT))
		{
			action = act_open_blob(ACT_blob_create, symbol);
			symbol->sym_type = delimited ? SYM_delimited_cursor : SYM_cursor;
		}
		else
		{
			dyn* statement = par_statement();
			symbol->sym_object = (gpre_ctx*) statement;
			if (MSC_match(KW_FOR))
			{
				if (!MSC_match(KW_UPDATE))
					CPR_s_error("UPDATE");
				if (!MSC_match(KW_OF))
					CPR_s_error("OF");

				do {
					CPR_token();
				} while (MSC_match(KW_COMMA));
			}
			symbol->sym_type = SYM_dyn_cursor;
			statement->dyn_cursor_name = symbol;
			action = (act*) MSC_alloc(ACT_LEN);
			action->act_type = ACT_dyn_cursor;
			action->act_object = (ref*) statement;
			action->act_whenever = gen_whenever();
		}
		HSH_insert(symbol);
		return action;

	default:
		while (MSC_match(KW_COMMA))
			; // empty loop body
		if (MSC_match(KW_STATEMENT))
		{
			action = (act*) MSC_alloc(ACT_LEN);
			action->act_type = ACT_dyn_statement;
			return action;
		}
		CPR_s_error("CURSOR, STATEMENT or TABLE");
	}
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Handle an SQL declare filter statement
//

static act* act_declare_filter()
{
	gpre_req* request = MSC_request(REQ_ddl);

	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only DECLARE FILTER in context of single database");

	PAR_get_token();
	gpre_filter* filter = (gpre_filter*) MSC_alloc(FLTR_LEN);
	filter->fltr_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
	SQL_resolve_identifier("<identifier>", filter->fltr_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Filter name too long");

	PAR_get_token();

	// create action block
	act* action = MSC_action(request, ACT_declare_filter);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) filter;

	SLONG input_type = 0;
	if (MSC_match(KW_INPUT_TYPE))
		input_type = EXP_SSHORT_ordinal(true);
	else
		CPR_s_error("INPUT_TYPE");

	SLONG output_type = 0;
	if (MSC_match(KW_OUTPUT_TYPE))
		output_type = EXP_SSHORT_ordinal(true);
	else
		CPR_s_error("OUTPUT_TYPE");

	if (!range_short_integer(input_type) || !range_short_integer(output_type))
		PAR_error("Blob sub_type out of range");

	filter->fltr_input_type = (SSHORT) input_type;
	filter->fltr_output_type = (SSHORT) output_type;

	if (MSC_match(KW_ENTRY_POINT))
		filter->fltr_entry_point = extract_string(true);
	else
		CPR_s_error("ENTRY_POINT");

	if (MSC_match(KW_MODULE_NAME))
		filter->fltr_module_name = extract_string(true);
	else
		CPR_s_error("MODULE_NAME");

	return action;
}


//____________________________________________________________
//
//		Handle an SQL declare table statement.
//

static act* act_declare_table( gpre_sym* symbol, gpre_dbb* db)
{
	// create a local request block

	gpre_req* request = (gpre_req*) MSC_alloc(REQ_LEN);
	request->req_type = REQ_ddl;

	// create relation block

	gpre_rel* relation = make_relation(0, symbol->sym_string);

	if (!db)
		db = relation->rel_database;

	request->req_database = db;

	relation->rel_next = db->dbb_relations;
	db->dbb_relations = relation;
	gpre_fld* dbkey = MET_make_field("rdb$db_key", dtype_text, 8, false);
	relation->rel_dbkey = dbkey;
	dbkey->fld_flags |= FLD_dbkey | FLD_text | FLD_charset;
	dbkey->fld_ttype = ttype_binary;

	// if relation name already in incore metadata, remove it & its fields

	gpre_sym* old_symbol = HSH_lookup(relation->rel_symbol->sym_string);

	while (old_symbol)
	{
		if (old_symbol->sym_type == SYM_relation)
		{
			gpre_rel* tmp_relation = (gpre_rel*) old_symbol->sym_object;
			if (tmp_relation->rel_meta)
				PAR_error("Multiple DECLARE TABLE statements for table");
			gpre_sym* tmp_symbol = old_symbol->sym_homonym;
			HSH_remove(old_symbol);
			for (gpre_fld* field = tmp_relation->rel_fields; field; field = field->fld_next)
			{
				 HSH_remove(field->fld_symbol);
			}
			old_symbol = tmp_symbol;
		}
		else
			old_symbol = old_symbol->sym_homonym;
	}

	// add new symbol to incore metadata

	HSH_insert(relation->rel_symbol);

	// create action block

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_noop;
	action->act_object = (ref*) relation;

	// parse field list and create corresponding field blocks
	// include size information for message length calculations

	PAR_get_token();
	EXP_left_paren(0);
	USHORT count = 0;
	gpre_fld** ptr = &relation->rel_fields;

	for (;;)
	{
		gpre_fld* field = par_field(request, relation);
		*ptr = field;
		ptr = &field->fld_next;
		HSH_insert(field->fld_symbol);
		field->fld_position = count++;
		field->fld_flags &= ~FLD_meta;
		SQL_adjust_field_dtype(field);

		if (!MSC_match(KW_COMMA))
			break;
	}

	EXP_match_paren();

	return action;
}


//____________________________________________________________
//
//		Handle an SQL declare external statement
//

static act* act_declare_udf()
{
	gpre_req* request = MSC_request(REQ_ddl);

	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can only DECLARE EXTERNAL FUNCTION in context of single database");

	decl_udf* udf_declaration = (decl_udf*) MSC_alloc(DECL_UDF_LEN);
	TEXT* udf_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
	SQL_resolve_identifier("<identifier>", udf_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("External function name too long");

	udf_declaration->decl_udf_name = udf_name;
	PAR_get_token();

	// create action block
	act* action = MSC_action(request, ACT_declare_udf);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) udf_declaration;

	gpre_fld** ptr = &udf_declaration->decl_udf_arg_list;
	while (true)
	{
		if (MSC_match(KW_RETURNS))
		{
			if (MSC_match(KW_PARAMETER))
			{
				const SLONG return_parameter = EXP_pos_USHORT_ordinal(true);
				if (return_parameter > 10)
					PAR_error("return parameter not in range");
				fb_assert(return_parameter <= MAX_SSHORT);
				udf_declaration->decl_udf_return_parameter = (SSHORT) return_parameter;
			}
			else
			{
				gpre_fld* field = (gpre_fld*) MSC_alloc(FLD_LEN);
				field->fld_flags |= (FLD_meta | FLD_meta_cstring);
				SQL_par_field_dtype(request, field, true);
				SQL_adjust_field_dtype(field);
				udf_declaration->decl_udf_return_type = field;
				MSC_match(KW_BY);
				if (MSC_match(KW_VALUE))
					udf_declaration->decl_udf_return_mode = FUN_value;
				else
					udf_declaration->decl_udf_return_mode = FUN_reference;
			}
			break;
		}

		gpre_fld* field = (gpre_fld*) MSC_alloc(FLD_LEN);
		field->fld_flags |= (FLD_meta | FLD_meta_cstring);
		SQL_par_field_dtype(request, field, true);
		SQL_adjust_field_dtype(field);
		*ptr = field;
		ptr = &(field->fld_next);
		MSC_match(KW_COMMA);
	}

	if (MSC_match(KW_ENTRY_POINT))
		udf_declaration->decl_udf_entry_point = extract_string(true);
	else
		CPR_s_error("ENTRY_POINT");

	if (MSC_match(KW_MODULE_NAME))
		udf_declaration->decl_udf_module_name = extract_string(true);
	else
		CPR_s_error("MODULE_NAME");

	return action;
}


//____________________________________________________________
//
//		Parse an update action.  This is a little more complicated
//		because SQL confuses the update of a cursor with a mass update.
//		The syntax, and therefor the code, I fear, is a mess.
//

static act* act_delete()
{
	const TEXT* transaction;

	par_options(&transaction);

	if (!MSC_match(KW_FROM))
		CPR_s_error("FROM");

	// First comes the relation.  Unfortunately, there is no way to identify
	// its database until the cursor is known.  Sigh.  Save the token.

	TEXT r_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];
	SQL_relation_name(r_name, db_name, owner_name);

	// Parse the optional alias (context variable)

	gpre_sym* alias = gpreGlob.token_global.tok_symbol ? NULL : PAR_symbol(SYM_dummy);

	// Now the moment of truth.  If the next few tokens are WHERE CURRENT OF
	// then this is a sub-action of an existing request.  If not, then it is
	// a free standing request

	gpre_req* request = MSC_request(REQ_mass_update);
	upd* update = (upd*) MSC_alloc(UPD_LEN);

	const bool where = MSC_match(KW_WITH);
	if (where && MSC_match(KW_CURRENT))
	{
		if (!MSC_match(KW_OF))
			CPR_s_error("OF <cursor>");
		gpreGlob.requests = request->req_next;
		gpreGlob.cur_routine->act_object = (ref*) request->req_routine; // Beware global var
		MSC_free(request);
		request = par_cursor(NULL);
		if ((transaction || request->req_trans) &&
			(!transaction || !request->req_trans || strcmp(transaction, request->req_trans)))
		{
			if (transaction)
				PAR_error("different transaction for select and delete");
			else
			{
				// does not specify transaction clause in
				// "delete ... where current of cursor" stmt
				const size_t trans_nm_len = strlen(request->req_trans);
				char* str_2 = (char*) MSC_alloc(trans_nm_len + 1);
				memcpy(str_2, request->req_trans, trans_nm_len);
				transaction = str_2;
			}
		}
		request->req_trans = transaction;
		gpre_rel* relation = SQL_relation(request, r_name, db_name, owner_name, true);
		gpre_rse* selection = request->req_rse;
		gpre_ctx* context = NULL;
		SSHORT i;
		for (i = 0; i < selection->rse_count; i++)
		{
			context = selection->rse_context[i];
			if (context->ctx_relation == relation)
				break;
		}
		if (i == selection->rse_count)
			PAR_error("table not in request");
		update->upd_request = request;
		update->upd_source = context;

		act* action = MSC_action(request, ACT_erase);
		action->act_object = (ref*) update;
		action->act_whenever = gen_whenever();
		return action;
	}

	request->req_trans = transaction;

	// How amusing.  After all that work, it wasn't a sub-action at all.
	// Neat.  Take the pieces and build a complete request.  Start by
	// figuring out what database is involved.

	gpre_rel* relation = SQL_relation(request, r_name, db_name, owner_name, true);

	gpre_rse* selection = (gpre_rse*) MSC_alloc(RSE_LEN(1));
	request->req_rse = selection;
	selection->rse_count = 1;
	gpre_ctx* context = MSC_context(request);
	selection->rse_context[0] = context;
	context->ctx_relation = relation;

	bool hsh_rm = false;
	if (alias && !gpreGlob.token_global.tok_symbol)
	{
		alias->sym_type = SYM_context;
		alias->sym_object = context;
		context->ctx_symbol = alias;
		gpreGlob.token_global.tok_symbol = alias;
		HSH_insert(alias);
		hsh_rm = true;
	}

	if (where)
		selection->rse_boolean = SQE_boolean(request, 0);

	request->req_node = MSC_node(nod_erase, 0);

	act* action = MSC_action(request, ACT_loop);
	action->act_whenever = gen_whenever();

	if (hsh_rm)
		HSH_remove(alias);

	return action;
}


//____________________________________________________________
//
//		Handle an SQL describe statement.
//			Reject
//

static act* act_describe()
{
	bool in_sqlda;

	if (MSC_match(KW_INPUT))
		in_sqlda = true;
	else
	{
		MSC_match(KW_OUTPUT);
		in_sqlda = false;
	}

	dyn* statement = par_statement();

	if (!MSC_match(KW_INTO))
	{
		// check for SQL2 syntax
		// "USING SQL DESCRIPTOR sqlda"

		if (!MSC_match(KW_USING) || !MSC_match(KW_SQL) || !MSC_match(KW_DESCRIPTOR))
			CPR_s_error("INTO or USING SQL DESCRIPTOR sqlda");
	}
	else if (MSC_match(KW_SQL) && !MSC_match(KW_DESCRIPTOR))
		CPR_s_error("INTO SQL DESCRIPTOR sqlda");

	statement->dyn_sqlda = PAR_native_value(false, false);
	act* action = (act*) MSC_alloc(ACT_LEN);
	if (in_sqlda)
		action->act_type = ACT_dyn_describe_input;
	else
		action->act_type = ACT_dyn_describe;
	action->act_object = (ref*) statement;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Parse a FINISH statement.
//

static act* act_disconnect()
{
	act* action = MSC_action(0, ACT_disconnect);
	action->act_whenever = gen_whenever();
	bool all = MSC_match(KW_ALL) || MSC_match(KW_DEFAULT);

	if (!all)
	{
		if (MSC_match(KW_CURRENT))
			PAR_error("DISCONNECT CURRENT not supported");
		gpre_sym* test_symbol = gpreGlob.token_global.tok_symbol;
		if (!(test_symbol && test_symbol->sym_type == SYM_database))
		{
			CPR_s_error("ALL, DEFAULT, or <database handle>");
		}
		while (true)
		{
			gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
			if (symbol && symbol->sym_type == SYM_database)
			{
				rdy* ready = (rdy*) MSC_alloc(RDY_LEN);
				ready->rdy_next = (rdy*) action->act_object;
				action->act_object = (ref*) ready;
				ready->rdy_database = (gpre_dbb*) symbol->sym_object;
				PAR_get_token();
			}
			else
				CPR_s_error("<database handle>");
			if (!MSC_match(KW_COMMA))
				break;
		}
	}

	if (gpreGlob.sw_language == lang_ada)
		MSC_match(KW_SEMI_COLON);
	return action;
}


//____________________________________________________________
//
//		Handle an SQL drop statement.
//

static act* act_drop()
{
	act* action = NULL;
	gpre_dbb* db = NULL;
	gpre_req* request = NULL;
	gpre_rel* relation = NULL;
	TEXT* identifier_name;

	switch (gpreGlob.token_global.tok_keyword)
	{
	case KW_DATABASE:
		{
			PAR_error("DROP DATABASE not supported");

			request = MSC_request(REQ_ddl);
			PAR_get_token();
			if (!isQuoted(gpreGlob.token_global.tok_type))
				CPR_s_error("<quoted database name>");
			db = (gpre_dbb*) MSC_alloc(DBB_LEN);
			SCHAR* db_string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
			db->dbb_filename = db_string;
			MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, db_string);
			gpre_sym* symbol = PAR_symbol(SYM_dummy);
			db->dbb_name = symbol;
			symbol->sym_type = SYM_database;
			symbol->sym_object = (gpre_ctx*) db;
			action = MSC_action(request, ACT_drop_database);
			action->act_whenever = gen_whenever();
			action->act_object = (ref*) db;
			PAR_get_token();
		}
		return action;

	case KW_DOMAIN:
		request = MSC_request(REQ_ddl);
		if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
			request->req_database = gpreGlob.isc_databases;
		else
			PAR_error("Can only DROP DOMAIN in context of single database");
		PAR_get_token();
		identifier_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<identifier>", identifier_name, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Domain name too long");

		action = MSC_action(request, ACT_drop_domain);
		action->act_whenever = gen_whenever();
		action->act_object = (ref*) identifier_name;
		PAR_get_token();
		return action;

	case KW_FILTER:
		request = MSC_request(REQ_ddl);
		if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
			request->req_database = gpreGlob.isc_databases;
		else
			PAR_error("Can only DROP FILTER in context of single database");
		PAR_get_token();
		identifier_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<identifier>", identifier_name, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Filter name too long");

		action = MSC_action(request, ACT_drop_filter);
		action->act_whenever = gen_whenever();
		action->act_object = (ref*) identifier_name;
		PAR_get_token();
		return action;

	case KW_EXTERNAL:

		PAR_get_token();
		if (!MSC_match(KW_FUNCTION))
			CPR_s_error("FUNCTION");
		request = MSC_request(REQ_ddl);
		if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
			request->req_database = gpreGlob.isc_databases;
		else
			PAR_error("Can only DROP EXTERNAL FUNCTION in context of a single database");

		identifier_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<identifier>", identifier_name, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("External function name too long");

		action = MSC_action(request, ACT_drop_udf);
		action->act_whenever = gen_whenever();
		action->act_object = (ref*) identifier_name;
		PAR_get_token();
		return action;

	case KW_INDEX:
		{
			request = MSC_request(REQ_ddl);
			PAR_get_token();
			SQL_resolve_identifier("<index name>", NULL, NAME_SIZE + 1);
			if (gpreGlob.token_global.tok_length >= NAME_SIZE)
				PAR_error("Index name too long");

			gpre_index* index = make_index(request, gpreGlob.token_global.tok_string);
			action = MSC_action(request, ACT_drop_index);
			action->act_whenever = gen_whenever();
			action->act_object = (ref*) index;
			PAR_get_token();
		}
		return action;

	case KW_STOGROUP:
		PAR_error("DROP STOGROUP not supported");

	case KW_TABLESPACE:
		PAR_error("DROP TABLESPACE not supported");

	case KW_SYNONYM:
		PAR_error("DROP SYNONYM request not allowed");

	case KW_SHADOW:
		{
			request = MSC_request(REQ_ddl);
			if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
				request->req_database = gpreGlob.isc_databases;
			else
				PAR_error("Can only DROP SHADOW in context of a single database");
			PAR_get_token();
			action = MSC_action(request, ACT_drop_shadow);
			action->act_whenever = gen_whenever();
			SLONG num = EXP_USHORT_ordinal(true);
			if (!range_positive_short_integer(num))
				PAR_error("Shadow number out of range");
			action->act_object = (ref*)(IPTR) num;
		}
		return action;

	case KW_TABLE:
		request = MSC_request(REQ_ddl);
		PAR_get_token();
		relation = par_relation(request);
		action = MSC_action(request, ACT_drop_table);
		action->act_whenever = gen_whenever();
		action->act_object = (ref*) relation;
		return action;

	case KW_VIEW:
		request = MSC_request(REQ_ddl);
		PAR_get_token();
		relation = par_relation(request);
		action = MSC_action(request, ACT_drop_view);
		action->act_whenever = gen_whenever();
		action->act_object = (ref*) relation;
		return action;

	default:
		PAR_error("Invalid DROP request");
	}
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Handle an SQL event statement
//

static act* act_event()
{
	act* action = NULL;

	if (MSC_match(KW_INIT))
		action = PAR_event_init(true);
	else if (MSC_match(KW_WAIT))
		action = PAR_event_wait(true);
	else
		CPR_s_error("INIT or WAIT");
	action->act_whenever = gen_whenever();
	return action;
}


//____________________________________________________________
//
//		Handle an SQL execute statement.
//			Reject
//

static act* act_execute()
{
	if (MSC_match(KW_PROCEDURE))
		return act_procedure();

	// EXECUTE IMMEDIATE is a different sort of duck

	if (MSC_match(KW_IMMEDIATE))
	{
		if (gpreGlob.isc_databases && gpreGlob.isc_databases->dbb_next)
		{
			TEXT s[ERROR_LENGTH];
			sprintf(s, "Executing dynamic SQL statement in context of database %s",
					gpreGlob.isc_databases->dbb_name->sym_string);
			CPR_warn(s);
		}
		dyn* statement = (dyn*) MSC_alloc(DYN_LEN);
		par_options(&statement->dyn_trans);

		switch (gpreGlob.sw_sql_dialect)
		{
		case 1:
			if (!isQuoted(gpreGlob.token_global.tok_type) && !MSC_match(KW_COLON))
				CPR_s_error(": <string expression>");
			break;

		default:
			if (gpreGlob.token_global.tok_type != tok_sglquoted && !MSC_match(KW_COLON))
				CPR_s_error(": <string expression>");
			break;
		}
		statement->dyn_string = PAR_native_value(false, false);
		par_using(statement);
		if (statement->dyn_using)
			PAR_error("Using host-variable list not supported.");
		par_into(statement);
		statement->dyn_database = gpreGlob.isc_databases;

		act* action = (act*) MSC_alloc(ACT_LEN);
		action->act_type = ACT_dyn_immediate;
		action->act_object = (ref*) statement;
		action->act_whenever = gen_whenever();
		return action;
	}

	// Ordinary form of EXECUTE

	const TEXT* transaction;
	par_options(&transaction);
	dyn* statement = par_statement();
	statement->dyn_trans = transaction;

	par_using(statement);
	if (statement->dyn_using)
		PAR_error("Using host-variable list not supported.");
	par_into(statement);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_dyn_execute;
	action->act_object = (ref*) statement;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Parse the SQL fetch statement.
//

static act* act_fetch()
{
	// Handle dynamic SQL statement, if appropriate

	dyn* cursor = par_dynamic_cursor();
	if (cursor)
	{
		dyn* statement = (dyn*) MSC_alloc(DYN_LEN);
		statement->dyn_statement_name = cursor->dyn_statement_name;
		statement->dyn_cursor_name = cursor->dyn_cursor_name;
		if (MSC_match(KW_USING) || MSC_match(KW_INTO))
		{
			MSC_match(KW_SQL);		// optional for backward compatibility

			if (MSC_match(KW_DESCRIPTOR))
				statement->dyn_sqlda = PAR_native_value(false, false);
			else
				statement->dyn_using = (gpre_nod*) SQE_list(SQE_variable, NULL, false);
			if (statement->dyn_using)
				PAR_error("Using host-variable list not supported.");
		}
		act* action = (act*) MSC_alloc(ACT_LEN);
		action->act_type = ACT_dyn_fetch;
		action->act_object = (ref*) statement;
		action->act_whenever = gen_whenever();
		return action;
	}

	// Statement is static SQL

	gpre_req* request = par_cursor(NULL);

	if (request->req_flags & REQ_sql_blob_create)
		PAR_error("Fetching from a blob being created is not allowed.");

	act* action = MSC_action(request, ACT_fetch);
	action->act_whenever = gen_whenever();

	if (request->req_flags & REQ_sql_blob_open)
	{
		if (!MSC_match(KW_INTO))
			CPR_s_error("INTO");
		action->act_object = (ref*) SQE_variable(NULL, false, NULL, NULL);
		action->act_type = ACT_get_segment;
	}
	else if (MSC_match(KW_INTO))
	{
		action->act_object = (ref*) SQE_list(SQE_variable, request, false);
		gpre_rse* select = request->req_rse;
		into(request, select->rse_fields, (gpre_nod*) action->act_object);
	}

	return action;
}


//____________________________________________________________
//
//		Parse an SQL grant or revoke statement.  Set up grant/revoke
//		blocks, fill in all of the privilege information, and
//		attach them to an action block of type GRANT or REVOKE.
//

static act* act_grant_revoke(act_t type)
{
	gpre_req* request = MSC_request(REQ_ddl);
	PRV priv_block = MSC_privilege_block();

	// if it is revoke action, parse the optional grant option for

	if (type == ACT_dyn_revoke)
	{
		if (MSC_match(KW_GRANT))
		{
			if (!MSC_match(KW_OPTION))
				CPR_s_error("OPTION");
			if (!MSC_match(KW_FOR))
				CPR_s_error("FOR");
			priv_block->prv_privileges |= PRV_grant_option;
		}
	}

	bool execute_priv = false;

	if (MSC_match(KW_ALL))
	{
		MSC_match(KW_PRIVILEGES);	// Keyword 'privileges' is optional
		priv_block->prv_privileges = PRV_all;
	}
	else if (MSC_match(KW_EXECUTE))
	{
		priv_block->prv_privileges |= PRV_execute;
		execute_priv = true;
	}
	else
	{
		gpre_lls** fields = &priv_block->prv_fields;
		while (true)
		{
			if (MSC_match(KW_SELECT))
				priv_block->prv_privileges |= PRV_select;
			else if (MSC_match(KW_INSERT))
				priv_block->prv_privileges |= PRV_insert;
			else if (MSC_match(KW_DELETE))
				priv_block->prv_privileges |= PRV_delete;
			else if (MSC_match(KW_UPDATE))
			{
				priv_block->prv_privileges |= PRV_update;
				if (MSC_match(KW_LEFT_PAREN))
				{
					SCHAR col_name[NAME_SIZE + 1];
					do {
						SQL_resolve_identifier("<column name>", col_name, NAME_SIZE + 1);
						if (gpreGlob.token_global.tok_length >= NAME_SIZE)
							PAR_error("Field name too long");

						STR field_name = (STR) MSC_string(col_name);
						MSC_push((gpre_nod*) field_name, fields);
						fields = &(*fields)->lls_next;
						CPR_token();
					} while (MSC_match(KW_COMMA));

					if (!MSC_match(KW_RIGHT_PAREN))
						CPR_s_error("<right parenthesis>");
				}
			}

			if (!MSC_match(KW_COMMA))
				break;
		}
	}

	if (!MSC_match(KW_ON))
		CPR_s_error("ON");

	STR relation_name = NULL;
	SCHAR r_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];

	if (execute_priv)
	{
		if (!MSC_match(KW_PROCEDURE))
			CPR_s_error("PROCEDURE");
		SQL_relation_name(r_name, db_name, owner_name);
		SQL_procedure(request, r_name, db_name, owner_name, true);
		relation_name = (STR) MSC_string(r_name);
		priv_block->prv_relation = relation_name;
		priv_block->prv_object_dyn = isc_dyn_prc_name;
	}
	else
	{
		MSC_match(KW_TABLE);		// filler word
		SQL_relation_name(r_name, db_name, owner_name);
		SQL_relation(request, r_name, db_name, owner_name, true);
		relation_name = (STR) MSC_string(r_name);
		priv_block->prv_relation = relation_name;
		priv_block->prv_object_dyn = isc_dyn_rel_name;
	}

	if (type == ACT_dyn_grant)
	{
		if (!MSC_match(KW_TO))
			CPR_s_error("TO");
	}
	else
	{
		//  type == ACT_dyn_revoke
		if (!MSC_match(KW_FROM))
			CPR_s_error("FROM");
	}

	bool grant_option_legal = true;
	gpre_usn* usernames = 0;
	gpre_usn* user = 0;
	USHORT user_dyn = 0;
	SCHAR s[ERROR_LENGTH];

	while (true)
	{
		if (MSC_match(KW_PROCEDURE))
		{
			SQL_relation_name(r_name, db_name, owner_name);
			SQL_procedure(request, r_name, db_name, owner_name, true);
			user_dyn = isc_dyn_grant_proc;
			grant_option_legal = false;
		}
		else if (MSC_match(KW_TRIGGER))
		{
			SQL_relation_name(r_name, db_name, owner_name);
			if (!MET_trigger_exists(request->req_database, r_name))
			{
				sprintf(s, "TRIGGER %s not defined", r_name);
				PAR_error(s);
			}
			user_dyn = isc_dyn_grant_trig;
			grant_option_legal = false;
		}
		else if (MSC_match(KW_VIEW))
		{
			SQL_relation_name(r_name, db_name, owner_name);
			if (!MET_get_view_relation(request, r_name, relation_name->str_string, 0))
			{
				sprintf(s, "VIEW %s not defined on table %s", r_name, relation_name->str_string);
				PAR_error(s);
			}
			user_dyn = isc_dyn_grant_view;
			grant_option_legal = false;
		}
		else if (MSC_match(KW_PUBLIC))
		{
			strcpy(r_name, "PUBLIC");
			user_dyn = isc_dyn_grant_user;
		}
		else
		{
			MSC_match(KW_USER);
			if (gpreGlob.token_global.tok_type != tok_ident)
				CPR_s_error("<user name identifier>");
			else
				to_upcase(gpreGlob.token_global.tok_string, r_name, sizeof(r_name));
			user_dyn = isc_dyn_grant_user;
			CPR_token();
		}

		if (!usernames)
		{
			usernames = MSC_username(r_name, user_dyn);
			user = usernames;
		}
		else
		{
			user->usn_next = MSC_username(r_name, user_dyn);
			user = user->usn_next;
		}

		if (!MSC_match(KW_COMMA))
			break;
	}

	// If this is a grant, do we have the optional WITH GRANT OPTION specification?

	if ((type == ACT_dyn_grant) && grant_option_legal)
	{
		if (MSC_match(KW_WITH))
		{
			if (!MSC_match(KW_GRANT))
				CPR_s_error("GRANT");
			if (!MSC_match(KW_OPTION))
				CPR_s_error("OPTION");
			priv_block->prv_privileges |= PRV_grant_option;
		}
	}

	// create action block

	act* action = MSC_action(request, type);
	action->act_next = 0;

	PRV last_priv_block = priv_block;
	bool first = true;

	for (user = usernames; user; user = user->usn_next)
	{
		// create and fill privilege block

		priv_block = MSC_privilege_block();
		priv_block->prv_username = user->usn_name;
		priv_block->prv_user_dyn = user->usn_dyn;
		priv_block->prv_privileges = last_priv_block->prv_privileges;
		priv_block->prv_relation = last_priv_block->prv_relation;
		priv_block->prv_object_dyn = last_priv_block->prv_object_dyn;
		priv_block->prv_fields = last_priv_block->prv_fields;
		if (first)
		{
			action->act_object = (ref*) priv_block;
			first = false;
		}
		else
			last_priv_block->prv_next = priv_block;
		last_priv_block = priv_block;
	}
	action->act_request = request;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//

static act* act_include()
{
	PAR_get_token();
	MSC_match(KW_SEMI_COLON);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_b_declare;
	gpreGlob.cur_routine = action; // Hmm, global var

	if (!gpreGlob.isc_databases)
	{
		// allocate database block and link to db chain

		gpreGlob.isc_databases = (gpre_dbb*) MSC_alloc_permanent(DBB_LEN);

		// allocate symbol block

		gpre_sym* symbol = (gpre_sym*) MSC_alloc_permanent(SYM_LEN);

		// make it a database, specifically this one

		symbol->sym_type = SYM_database;
		symbol->sym_object = (gpre_ctx*) gpreGlob.isc_databases;
		symbol->sym_string = gpreGlob.database_name;

		// database block points to the symbol block

		gpreGlob.isc_databases->dbb_name = symbol;
		gpreGlob.isc_databases->dbb_filename = NULL;
		gpreGlob.isc_databases->dbb_flags = DBB_sqlca;
		gpreGlob.isc_databases->dbb_c_lc_ctype = gpreGlob.module_lc_ctype;
		if (gpreGlob.sw_external)
			gpreGlob.isc_databases->dbb_scope = DBB_EXTERN;
	}
	else
		// Load the symbol (hash) table with relation names from this database.
		MET_load_hash_table(gpreGlob.isc_databases);

	HSH_insert(gpreGlob.isc_databases->dbb_name);

	return action;
}


//____________________________________________________________
//
//		Process SQL INSERT statement.
//

static act* act_insert()
{
	const TEXT* transaction = NULL;

	par_options(&transaction);

	if (MSC_match(KW_CURSOR))
		return act_insert_blob();

	if (!MSC_match(KW_INTO))
		CPR_s_error("INTO");

	gpre_req* request = MSC_request(REQ_insert);
	request->req_trans = transaction;
	gpre_ctx* context = SQE_context(request);

	int count = 0, count2 = 0;
	gpre_lls* fields = NULL;

	// Pick up a field list

	if (!MSC_match(KW_LEFT_PAREN))
	{
		gpre_nod* list = MET_fields(context);
		count = list->nod_count;
		for (int i = 0; i < count; i++)
			MSC_push(list->nod_arg[i], &fields);
	}
	else
	{
		do {
			gpre_nod* node = SQE_field(request, false);
			if (node->nod_type == nod_array)
			{
				node->nod_type = nod_field;

				// Make sure no subscripts are specified

				if (node->nod_arg[1]) {
					PAR_error("Partial insert of arrays not permitted");
				}
			}

			// Dialect 1 program may not insert new datatypes
			if ((SQL_DIALECT_V5 == gpreGlob.sw_sql_dialect) && nod_field == node->nod_type)
			{
				const USHORT field_dtype = ((ref*) (node->nod_arg[0]))->ref_field->fld_dtype;
				if (dtype_sql_date == field_dtype || dtype_sql_time == field_dtype ||
					dtype_int64 == field_dtype)
				{
					SQL_dialect1_bad_type(field_dtype);
				}
			}

			MSC_push(node, &fields);
			count++;
		} while (MSC_match(KW_COMMA));

		EXP_match_paren();
	}

	gpre_lls* values = NULL;
	if (MSC_match(KW_VALUES))
	{
		// Now pick up a value list

		EXP_left_paren(0);
		for (;;)
		{
			if (MSC_match(KW_NULL))
				MSC_push(MSC_node(nod_null, 0), &values);
			else
				MSC_push(SQE_value(request, false, NULL, NULL), &values);
			count2++;
			if (!MSC_match(KW_COMMA))
				break;
		}
		EXP_match_paren();

		// Make an assignment list

		if (count != count2)
			PAR_error("count of values doesn't match count of columns");

		gpre_nod* vlist = MSC_node(nod_list, (SSHORT) count);
		request->req_node = vlist;
		gpre_nod** ptr = &vlist->nod_arg[count];

		while (values)
		{
			gpre_nod* assignment = MSC_node(nod_assignment, 2);
			assignment->nod_arg[0] = (gpre_nod*) MSC_pop(&values);
			assignment->nod_arg[1] = (gpre_nod*) MSC_pop(&fields);
			pair(assignment->nod_arg[0], assignment->nod_arg[1]);
			*--ptr = assignment;
		}

		if (context->ctx_symbol)
			HSH_remove(context->ctx_symbol);
		act* action = MSC_action(request, ACT_insert);
		action->act_whenever = gen_whenever();
		return action;
	}

	if (!MSC_match(KW_SELECT))
		CPR_s_error("VALUES or SELECT");

	// OK, we've got a mass insert on our hands.  Start by picking
	// up the select statement.  First, however, remove the INSERT
	// context to avoid resolving SELECT fields to the insert relation.

	request->req_type = REQ_mass_update;
	request->req_contexts = NULL;
	request->req_update = context;
	gpre_rse* select = SQE_select(request, false);
	request->req_rse = select;
	context->ctx_next = request->req_contexts;
	request->req_contexts = context;

	// Build an assignment list from select expressions into target list

	gpre_nod* select_list = select->rse_fields;

	if (count != select_list->nod_count)
		PAR_error("count of values doesn't match count of columns");

	gpre_nod* alist = MSC_node(nod_list, (SSHORT) count);
	request->req_node = alist;
	gpre_nod** ptr = &alist->nod_arg[count];
	gpre_nod** ptr2 = &select_list->nod_arg[count];

	while (fields)
	{
		gpre_nod* assignment = MSC_node(nod_assignment, 2);
		assignment->nod_arg[0] = *--ptr2;
		assignment->nod_arg[1] = (gpre_nod*) MSC_pop(&fields);
		pair(assignment->nod_arg[0], assignment->nod_arg[1]);
		*--ptr = assignment;
	}

	gpre_nod* store = MSC_binary(nod_store, (gpre_nod*) context, alist);
	request->req_node = store;
	EXP_rse_cleanup(select);
	if (context->ctx_symbol)
		HSH_remove(context->ctx_symbol);

	act* action = MSC_action(request, ACT_loop);
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Process SQL INSERT statement.
//

static act* act_insert_blob()
{
	// Handle dynamic SQL statement, if appropriate

	dyn* cursor = par_dynamic_cursor();
	if (cursor)
	{
		dyn* statement = (dyn*) MSC_alloc(DYN_LEN);
		statement->dyn_statement_name = cursor->dyn_statement_name;
		statement->dyn_cursor_name = cursor->dyn_cursor_name;
		par_using(statement);
		if (statement->dyn_using)
			PAR_error("Using host-variable list not supported.");

		act* action = (act*) MSC_alloc(ACT_LEN);
		action->act_type = ACT_dyn_insert;
		action->act_object = (ref*) statement;
		action->act_whenever = gen_whenever();
		return action;
	}

	// Statement is static SQL

	gpre_req* request = par_cursor(NULL);
	if (request->req_flags & REQ_sql_blob_open)
		PAR_error("Inserting into a blob being opened is not allowed.");

	act* action = MSC_action(request, ACT_put_segment);
	action->act_whenever = gen_whenever();

	if (!MSC_match(KW_VALUES))
		CPR_s_error("VALUES");

	EXP_left_paren(0);
	action->act_object = (ref*) SQE_variable(NULL, false, NULL, NULL);
	if (!action->act_object->ref_null_value)
		PAR_error("A segment length is required.");
	EXP_match_paren();

	return action;
}


//____________________________________________________________
//
//		Handle an SQL lock statement.
//			Reject
//

static act* act_lock()
{
	PAR_error("SQL LOCK TABLE request not allowed");
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Handle the SQL actions OPEN and CLOSE cursors.
//

static act* act_openclose(act_t type)
{
	const TEXT* transaction = 0;

	if (type == ACT_open)
		par_options(&transaction);

	// Handle dynamic SQL statement, if appropriate

	dyn* cursor = par_dynamic_cursor();
	if (cursor)
	{
		dyn* statement = (dyn*) MSC_alloc(DYN_LEN);
		statement->dyn_statement_name = cursor->dyn_statement_name;
		statement->dyn_cursor_name = cursor->dyn_cursor_name;
		act* action = (act*) MSC_alloc(ACT_LEN);
		action->act_object = (ref*) statement;
		action->act_whenever = gen_whenever();
		if (type == ACT_open)
		{
			action->act_type = ACT_dyn_open;
			statement->dyn_trans = transaction;
			par_using(statement);
			if (statement->dyn_using)
				PAR_error("Using host-variable list not supported.");
			par_into(statement);
		}
		else
			action->act_type = ACT_dyn_close;
		return action;
	}

	// Statement is static SQL

	gpre_sym* symbol = NULL;
	gpre_req* request = par_cursor(&symbol);

	act* action = MSC_action(request, type);
	if (type == ACT_open)
	{
		open_cursor* open = (open_cursor*) MSC_alloc(OPN_LEN);
		open->opn_trans = transaction;
		open->opn_cursor = symbol;
		action->act_object = (ref*) open;
		if (transaction != NULL)
			request->req_trans = transaction;
		if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
		{
			if (request->req_flags & REQ_sql_blob_open)
			{
				if (!MSC_match(KW_USING))
					CPR_s_error("USING");
			}
			else if (!MSC_match(KW_INTO))
				CPR_s_error("INTO");
			ref* opn_using = (ref*) SQE_variable(NULL, false, NULL, NULL);
			open->opn_using = opn_using;
			opn_using->ref_next = request->req_blobs->blb_reference;
			request->req_blobs->blb_reference = opn_using;
			opn_using->ref_context = request->req_contexts;
			opn_using->ref_field = request->req_references->ref_field;
		}
	}
	else
		action->act_object = (ref*) symbol;
	action->act_whenever = gen_whenever();

	if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
	{
		action->act_type = (type == ACT_close) ?
			ACT_blob_close : (request->req_flags & REQ_sql_blob_open) ?
				ACT_blob_open : ACT_blob_create;
	}

	return action;
}


//____________________________________________________________
//
//		Parse an "open blob" type statement.
//		These include READ BLOB and INSERT BLOB.
//

static act* act_open_blob( act_t act_op, gpre_sym* symbol)
{
	if (!MSC_match(KW_BLOB))
		CPR_s_error("BLOB");

	// if the token isn't an identifier, complain

	tok* f_token = (tok*) MSC_alloc(TOK_LEN);
	f_token->tok_length = gpreGlob.token_global.tok_length;

	// Funny, as if we can have relation names up to MAX_SYM_SIZE.
	SQL_resolve_identifier("<column_name>", f_token->tok_string, f_token->tok_length + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Field name too long");

	CPR_token();

	if (act_op == ACT_blob_open)
	{
		if (!MSC_match(KW_FROM))
			CPR_s_error("FROM");
	}
	else if (!MSC_match(KW_INTO))
		CPR_s_error("INTO");

	gpre_req* request = MSC_request(REQ_cursor);
	request->req_flags = (act_op == ACT_blob_open) ? REQ_sql_blob_open : REQ_sql_blob_create;

	SCHAR r_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];
	SQL_relation_name(r_name, db_name, owner_name);


	SCHAR s[ERROR_LENGTH];
	gpre_rel* relation = SQL_relation(request, r_name, db_name, owner_name, true);
	gpre_fld* field = MET_field(relation, f_token->tok_string);
	if (!field)
	{
		fb_utils::snprintf(s, sizeof(s), "column \"%s\" not in context", f_token->tok_string);
		PAR_error(s);
	}

	if (!(field->fld_flags & FLD_blob))
	{
		fb_utils::snprintf(s, sizeof(s), "column %s is not a BLOB", field->fld_symbol->sym_string);
		PAR_error(s);
	}

	if (field->fld_array_info)
	{
		fb_utils::snprintf(s, sizeof(s), "column %s is an array and can not be opened as a BLOB",
			field->fld_symbol->sym_string);
		PAR_error(s);
	}

	ref* reference = MSC_reference(0);
	reference->ref_field = field;

	gpre_ctx* context = MSC_context(request);
	context->ctx_relation = relation;

	blb* blob = (blb*) MSC_alloc(BLB_LEN);
	blob->blb_symbol = symbol;
	blob->blb_request = request;
	blob->blb_next = request->req_blobs;
	symbol->sym_object = (gpre_ctx*) request;
	symbol->sym_type = SYM_cursor;

	request->req_references = reference;
	request->req_blobs = blob;

	if (MSC_match(KW_FILTER))
	{
		if (MSC_match(KW_FROM))
		{
			blob->blb_const_from_type = PAR_blob_subtype(request->req_database);
			if (gpreGlob.token_global.tok_keyword == KW_CHAR)
				if (blob->blb_const_from_type == isc_blob_text)
				{
					blob->blb_from_charset = par_char_set();
					if (act_op == ACT_blob_open && blob->blb_from_charset != field->fld_charset_id)
					{
						PAR_error("Specified CHARACTER SET does not match BLOB column declaration.");
					}
				}
				else
					PAR_error("Only text BLOBS can specify CHARACTER SET");
			else if (blob->blb_const_from_type == isc_blob_text)
				if (act_op == ACT_blob_create)
					blob->blb_from_charset = CS_dynamic;
				else
					blob->blb_from_charset = field->fld_charset_id;
		}
		else
		{
			blob->blb_const_from_type = field->fld_sub_type;
			if (blob->blb_const_from_type == isc_blob_text)
				if (act_op == ACT_blob_create)
					blob->blb_from_charset = CS_dynamic;
				else
					blob->blb_from_charset = field->fld_charset_id;
		}
		if (!MSC_match(KW_TO))
			CPR_s_error("TO");
		blob->blb_const_to_type = PAR_blob_subtype(request->req_database);
		if (gpreGlob.token_global.tok_keyword == KW_CHAR)
			if (blob->blb_const_to_type == isc_blob_text)
			{
				blob->blb_to_charset = par_char_set();
				if (act_op == ACT_blob_create && blob->blb_to_charset != field->fld_charset_id)
				{
					PAR_error("Specified CHARACTER SET does not match BLOB column declaration.");
				}
			}
			else
				PAR_error("Only text BLOBS can specify CHARACTER SET");
		else if (blob->blb_const_to_type == isc_blob_text)
			if (act_op == ACT_blob_create)
				blob->blb_to_charset = field->fld_charset_id;
			else
				blob->blb_to_charset = CS_dynamic;
	}
	else
	{
		// No FILTER keyword seen

		// Even if no FILTER was specified, we set one up for the special
		// case of Firebird TEXT blobs, in order to do character set
		// transliteration from the column-declared character set of the
		// blob to the process character set (CS_dynamic).
		//
		// It is necessary to pass this information in the bpb as blob
		// operations are done using blob_ids, and we cannot determine
		// this information from the blob_id within the engine.

		if (field->fld_sub_type == isc_blob_text && (field->fld_charset_id != CS_NONE))
		{
			blob->blb_const_from_type = isc_blob_text;
			blob->blb_const_to_type = isc_blob_text;
			if (act_op == ACT_blob_create)
			{
				blob->blb_from_charset = CS_dynamic;
				blob->blb_to_charset = field->fld_charset_id;
			}
			else
			{
				blob->blb_from_charset = field->fld_charset_id;
				blob->blb_to_charset = CS_dynamic;
			}
		}
	}

	if (MSC_match(KW_MAX_SEGMENT))
		blob->blb_seg_length = EXP_USHORT_ordinal(true);
	else
		blob->blb_seg_length = field->fld_seg_length;

	if (!blob->blb_seg_length)
		blob->blb_seg_length = 512;

	act* action = MSC_action(request, ACT_cursor);
	action->act_object = (ref*) blob;

	return action;
}


//____________________________________________________________
//
//		Handle an SQL prepare statement.
//

static act* act_prepare()
{
	if (gpreGlob.isc_databases && gpreGlob.isc_databases->dbb_next)
	{
		TEXT s[ERROR_LENGTH];
		fb_utils::snprintf(s, sizeof(s), "Executing dynamic SQL statement in context of database %s",
				gpreGlob.isc_databases->dbb_name->sym_string);
		CPR_warn(s);
	}

	const TEXT* transaction = NULL;
	par_options(&transaction);

	dyn* statement = par_statement();
	statement->dyn_database = gpreGlob.isc_databases;
	statement->dyn_trans = transaction;

	if (MSC_match(KW_INTO))
	{
		if (MSC_match(KW_SQL) && !MSC_match(KW_DESCRIPTOR))
			CPR_s_error("INTO SQL DESCRIPTOR sqlda");
		statement->dyn_sqlda = PAR_native_value(false, false);
	}

	if (!MSC_match(KW_FROM))
		CPR_s_error("FROM");

	switch (gpreGlob.sw_sql_dialect)
	{
	case 1:
		if (!isQuoted(gpreGlob.token_global.tok_type) && !MSC_match(KW_COLON))
			CPR_s_error(": <string expression>");
		break;

	default:
		if (gpreGlob.token_global.tok_type != tok_sglquoted && !MSC_match(KW_COLON))
			CPR_s_error(": <string expression>");
		break;
	}

	statement->dyn_string = PAR_native_value(false, false);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_dyn_prepare;
	action->act_object = (ref*) statement;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Handle the EXECUTE PROCEDURE statement.
//

static act* act_procedure()
{
	gpre_req* request = MSC_request(REQ_procedure);
	par_options(&request->req_trans);

	SCHAR p_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];

	SQL_relation_name(p_name, db_name, owner_name);
	gpre_prc* procedure = SQL_procedure(request, p_name, db_name, owner_name, true);

	gpre_lls* values = NULL;

	SSHORT inputs = 0;
	if (gpreGlob.token_global.tok_keyword != KW_RETURNING &&
		gpreGlob.token_global.tok_keyword != KW_SEMI_COLON)
	{
		// parse input references

		const bool paren = MSC_match(KW_LEFT_PAREN);
		gpre_fld* field = procedure->prc_inputs;
		ref** ref_ptr = &request->req_values;
		do {
			if (MSC_match(KW_NULL))
				MSC_push(MSC_node(nod_null, 0), &values);
			else
			{
				ref* reference = SQE_parameter(request);
				*ref_ptr = reference;
				reference->ref_field = field;
				MSC_push(MSC_unary(nod_value, (gpre_nod*) reference), &values);
				ref_ptr = &reference->ref_next;
			}
			if (field)
				field = field->fld_next;
			inputs++;
		} while (MSC_match(KW_COMMA));
		if (paren)
			EXP_match_paren();
	}

	SSHORT outputs = 0;
	if (MSC_match(KW_RETURNING))
	{
		// parse output references

		const bool paren = MSC_match(KW_LEFT_PAREN);
		gpre_fld* field = procedure->prc_outputs;
		ref** ref_ptr = &request->req_references;
		do {
			ref* reference = (ref*) SQE_variable(request, false, NULL, NULL);
			*ref_ptr = reference;
			if (reference->ref_field = field)
				field = field->fld_next;
			ref_ptr = &reference->ref_next;
			outputs++;
		} while (MSC_match(KW_COMMA));
		if (paren)
			EXP_match_paren();
	}

	if (procedure->prc_in_count != inputs)
		PAR_error("count of input values doesn't match count of parameters");
	if (procedure->prc_out_count != outputs)
		PAR_error("count of output values doesn't match count of parameters");

	gpre_nod* list = MSC_node(nod_list, inputs);
	request->req_node = list;
	gpre_nod** ptr = &list->nod_arg[inputs];
	while (values)
		*--ptr = (gpre_nod*) MSC_pop(&values);

	act* action = MSC_action(request, ACT_procedure);
	action->act_object = (ref*) procedure;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Parse a RELEASE_REQUESTS statement
//
static act* act_release()
{
	act* action = MSC_action(0, ACT_release);

	MSC_match(KW_FOR);

	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (symbol && (symbol->sym_type == SYM_database))
	{
		action->act_object = (ref*) symbol->sym_object;
		PAR_get_token();
	}

	return action;
}


//____________________________________________________________
//
//		Handle the stand alone SQL select statement.
//

static act* act_select()
{
	gpre_req* request = MSC_request(REQ_for);
	par_options(&request->req_trans);
	gpre_rse* select = SQE_select(request, false);
	request->req_rse = select;

	if (!MSC_match(KW_SEMI_COLON))
	{
		TEXT s[ERROR_LENGTH];
		fb_utils::snprintf(s, sizeof(s), "Expected ';', got %s.", gpreGlob.token_global.tok_string);
		CPR_warn(s);
	}

	if (select->rse_into)
		into(request, select->rse_fields, select->rse_into);

	act* action = MSC_action(request, ACT_select);
	action->act_object = (ref*) select->rse_into;
	action->act_whenever = gen_whenever();
	EXP_rse_cleanup(select);
	return action;
}


//____________________________________________________________
//
//		Parse a SET <something>
//

static act* act_set(const TEXT* base_directory)
{

	if (MSC_match(KW_TRANSACTION))
		return act_set_transaction();

	if (MSC_match(KW_NAMES))
		return act_set_names();

	if (MSC_match(KW_STATISTICS))
		return act_set_statistics();

	if (MSC_match(KW_SCHEMA) || MSC_match(KW_DATABASE))
		return PAR_database(true, base_directory);

	if (MSC_match(KW_GENERATOR))
		return act_set_generator();

	if (MSC_match(KW_SQL))
	{
		if (MSC_match(KW_DIALECT))
			return act_set_dialect();
	}

	CPR_s_error("TRANSACTION, NAMES, SCHEMA, DATABASE, GENERATOR, DIALECT or STATISTICS");
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		Parse a SET SQL DIALECT
//

static act* act_set_dialect()
{
	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_sql_dialect;

	USHORT dialect = EXP_USHORT_ordinal(false);
	if (dialect < 1 || dialect > 3)
		CPR_s_error("SQL DIALECT 1,2 or 3");

	if (gpreGlob.isc_databases && dialect != gpreGlob.compiletime_db_dialect &&
		gpreGlob.sw_ods_version < 10)
	{
		char warn_mesg[100];
		sprintf(warn_mesg, "Pre 6.0 database. Cannot use dialect %d, Resetting to %d\n",
				dialect, SQL_DIALECT_V5);
		dialect = SQL_DIALECT_V5;
		CPR_warn(warn_mesg);
	}
	else if (gpreGlob.isc_databases && dialect != gpreGlob.compiletime_db_dialect)
	{
		char warn_mesg[100];
		sprintf(warn_mesg, "Client dialect set to %d. Compiletime database dialect is %d\n",
				dialect, gpreGlob.compiletime_db_dialect);
		CPR_warn(warn_mesg);
	}

	action->act_object = (ref*) MSC_alloc(SDT_LEN);
	((set_dialect*) action->act_object)->sdt_dialect = dialect;

	// Needed because subsequent parsing pass1 looks at sw_Sql_dialect value
	gpreGlob.sw_sql_dialect = dialect;
	gpreGlob.dialect_specified = true;

	PAR_get_token();
	return action;
}


//____________________________________________________________
//
//		Parse a SET generator
//

static act* act_set_generator()
{
	gpre_req* request = MSC_request(REQ_set_generator);

	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can SET GENERATOR in context of single database only");

	set_gen* setgen = (set_gen*) MSC_alloc(SGEN_LEN);
	setgen->sgen_name = (TEXT*) MSC_alloc(NAME_SIZE + 1);
	SQL_resolve_identifier("<identifier>", setgen->sgen_name, NAME_SIZE + 1);
	if (gpreGlob.token_global.tok_length >= NAME_SIZE)
		PAR_error("Generator name too long");

	if (!MET_generator(setgen->sgen_name, request->req_database))
	{
		SCHAR s[ERROR_LENGTH];
		fb_utils::snprintf(s, sizeof(s),
			"generator %s not found", gpreGlob.token_global.tok_string);
		PAR_error(s);
	}
	PAR_get_token();
	MSC_match(KW_TO);
	if ((gpreGlob.sw_sql_dialect == SQL_DIALECT_V5) || (gpreGlob.sw_server_version < 6))
	{
		setgen->sgen_value = EXP_SLONG_ordinal(true);
		setgen->sgen_dialect = SQL_DIALECT_V5;
	}
	else
	{
		// dialect is > 1. Server can handle int64

		setgen->sgen_int64value = EXP_SINT64_ordinal(true);
		setgen->sgen_dialect = SQL_DIALECT_V5 + 1;
	}

	act* action = (act*) MSC_action(request, ACT_s_start);
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) setgen;
	return action;
}


//____________________________________________________________
//
//		Parse a SET NAMES <charset>;
//

static act* act_set_names()
{
	if (gpreGlob.sw_auto)
		CPR_warn("SET NAMES requires -manual switch to gpre.");

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_noop;

	if (MSC_match(KW_COLON))
	{
		// User is specifying a host variable or string as
		// the character set.  Make this the run-time set.

		TEXT* value = PAR_native_value(false, false);
		for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			if (db->dbb_r_lc_ctype)
			{
				char buffer[ERROR_LENGTH];
				fb_utils::snprintf(buffer, sizeof(buffer),
						"Supersedes runtime character set for database %s",
						db->dbb_filename ? db->dbb_filename : db->dbb_name->sym_string);
				CPR_warn(buffer);
			}
			db->dbb_r_lc_ctype = value;
		}
	}
	else if (gpreGlob.token_global.tok_type == tok_ident)
	{
		// User is specifying the name of a character set
		// Make this the compile time character set

		TEXT* value = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, value);
		value[gpreGlob.token_global.tok_length] = '\0';

		// Due to the ambiguities involved in having modules expressed
		// in multiple compile-time character sets, we disallow it.

		if (gpreGlob.module_lc_ctype && strcmp(gpreGlob.module_lc_ctype, value) != 0)
			PAR_error("Duplicate declaration of module CHARACTER SET");

		gpreGlob.module_lc_ctype = value;
		for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			if (db->dbb_c_lc_ctype)
			{
				char buffer[ERROR_LENGTH];
				fb_utils::snprintf(buffer, sizeof(buffer),
					"Supersedes character set for database %s",
					db->dbb_filename ? db->dbb_filename : db->dbb_name->sym_string);
				CPR_warn(buffer);
			}

			db->dbb_c_lc_ctype = value;
			if (!(db->dbb_flags & DBB_sqlca))
			{
				// Verify that character set name is valid,
				// this requires a database to be previously declared
				// so we can resolve against it.
				// So what if we go through this code once for each database...

				if (!MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_charset))
					PAR_error("The named CHARACTER SET was not found");
			}
		}
		CPR_token();
	}
	else
		CPR_s_error("<character set name> or :<hostvar>");

	return action;
}


//____________________________________________________________
//
//		Parse a SET statistics
//

static act* act_set_statistics()
{
	gpre_req* request = MSC_request(REQ_ddl);
	STS stats = (STS) MSC_alloc(STS_LEN);

	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
		request->req_database = gpreGlob.isc_databases;
	else
		PAR_error("Can SET STATISTICS in context of single database only");

	if (MSC_match(KW_INDEX))
	{
		stats->sts_flags = STS_index;
		stats->sts_name = (STR) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<index name>", stats->sts_name->str_string, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Index name too long");

		PAR_get_token();
	}
	else
		CPR_s_error("INDEX");

	act* action = (act*) MSC_action(request, ACT_statistics);
	action->act_object = (ref*) stats;
	return action;
}


//____________________________________________________________
//
//		Generate a set transaction
//

static act* act_set_transaction()
{
	gpre_tra* trans = (gpre_tra*) MSC_alloc(TRA_LEN);

	if (MSC_match(KW_NAME))
		trans->tra_handle = PAR_native_value(false, true);

	// Get all the transaction parameters

	while (true)
	{
		if (MSC_match(KW_ISOLATION))
		{
			MSC_match(KW_LEVEL);
			if (!par_transaction_modes(trans, true))
				CPR_s_error("SNAPSHOT");

			continue;
		}

		if (par_transaction_modes(trans, false))
			continue;

		if (MSC_match(KW_NO))
		{
			if (MSC_match(KW_WAIT))
			{
				trans->tra_flags |= TRA_nw;
				continue;
			}
			CPR_s_error("WAIT");
		}

		if (MSC_match(KW_WAIT))
			continue;

#ifdef DEV_BUILD
		if (MSC_match(KW_AUTOCOMMIT))
		{
			trans->tra_flags |= TRA_autocommit;
			continue;
		}
#endif

		if (MSC_match(KW_NO_AUTO_UNDO))
		{
			trans->tra_flags |= TRA_no_auto_undo;
			continue;
		}

		break;
	}

	// send out for the list of reserved relations

	if (MSC_match(KW_RESERVING))
	{
		trans->tra_flags |= TRA_rrl;
		PAR_reserving(trans->tra_flags, true);
	}
	else if (MSC_match(KW_USING))
	{
		trans->tra_flags |= TRA_inc;
		PAR_using_db();
	}

	CMP_t_start(trans);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_start;
	action->act_object = (ref*) trans;
	action->act_whenever = gen_whenever();

	return action;
}


//____________________________________________________________
//
//		Generate a COMMIT, FINISH, or ROLLBACK.
//

static act* act_transaction(act_t type)
{
	const TEXT* transaction = NULL;

	par_options(&transaction);
	MSC_match(KW_WORK);

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = type;
	action->act_whenever = gen_whenever();
	action->act_object = (ref*) transaction;

	if (MSC_match(KW_RELEASE))
	{
		type = (type == ACT_commit) ? ACT_finish : ACT_rfinish;
		if (transaction)
		{
			action->act_rest = (act*) MSC_alloc(ACT_LEN);
			action->act_rest->act_type = type;
		}
		else
			action->act_type = type;
	}
	else if ((type == ACT_commit) && (MSC_match(KW_RETAIN)))
	{
		MSC_match(KW_SNAPSHOT);
		action->act_type = type = ACT_commit_retain_context;
	}
	else if ((type == ACT_rollback) && (MSC_match(KW_RETAIN)))
	{
		MSC_match(KW_SNAPSHOT);
		action->act_type = type = ACT_rollback_retain_context;
	}

	return action;
}


//____________________________________________________________
//
//		Parse an update action.  This is a little more complicated
//		because SQL confuses the update of a cursor with a mass update.
//		The syntax, and therefor the code, I fear, is a mess.
//

static act* act_update()
{
	const TEXT* transaction = NULL;

	par_options(&transaction);

	// First comes the relation.  Unfortunately, there is no way to identify
	// its database until the cursor is known.  Sigh.  Save the token.

	SCHAR r_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];
	SQL_relation_name(r_name, db_name, owner_name);

	// Parse the optional alias (context variable)

	gpre_sym* alias = gpreGlob.token_global.tok_symbol ? NULL : PAR_symbol(SYM_dummy);

	// Now we need the SET list list.  Do this thru a linked list stack

	if (!MSC_match(KW_SET))
		CPR_s_error("SET");

	gpre_req* request = MSC_request(REQ_mass_update);
	gpre_rel* relation = SQL_relation(request, r_name, db_name, owner_name, true);
	gpre_ctx* input_context = MSC_context(request);
	input_context->ctx_relation = relation;

	if (alias)
	{
		alias->sym_type = SYM_context;
		alias->sym_object = input_context;
		HSH_insert(alias);
		gpreGlob.token_global.tok_symbol = HSH_lookup(gpreGlob.token_global.tok_string);
	}

	gpre_lls* stack = NULL;
	SSHORT count = 0;

	do {
		gpre_nod* set_item = MSC_node(nod_assignment, 2);
		set_item->nod_arg[1] = SQE_field(NULL, false);
		if (!MSC_match(KW_EQUALS))
			CPR_s_error("assignment operator");
		if (MSC_match(KW_NULL))
			set_item->nod_arg[0] = MSC_node(nod_null, 0);
		else
			set_item->nod_arg[0] = SQE_value(request, false, NULL, NULL);
		MSC_push(set_item, &stack);
		count++;
	} while (MSC_match(KW_COMMA));

	gpre_nod* set_list = MSC_node(nod_list, count);
	gpre_nod** const end_list = set_list->nod_arg + count;
	gpre_nod** ptr = end_list;

	while (stack)
		*--ptr = (gpre_nod*) MSC_pop(&stack);

	// Now the moment of truth.  If the next few tokens are WHERE CURRENT OF
	// then this is a sub-action of an existing request.  If not, then it is
	// a free standing request

	const bool where = MSC_match(KW_WITH);
	if (where && MSC_match(KW_CURRENT))
	{
		if (!MSC_match(KW_OF))
			CPR_s_error("OF cursor");

		// Allocate update block, flush old request block, then
		// find the right request block, and the target relation

		upd* update = (upd*) MSC_alloc(UPD_LEN);
		update->upd_references = request->req_values;
		MSC_free_request(request);

		request = par_cursor(NULL);
		if (request->req_flags == REQ_sql_blob_create)
			PAR_error("expected a TABLE cursor, got a BLOB cursor");
		if ((transaction || request->req_trans) &&
			(!transaction || !request->req_trans || strcmp(transaction, request->req_trans)))
		{
			if (transaction)
				PAR_error("different transaction for select and update");
			else
			{
				// does not specify transaction clause in
				// "update ... where cuurent of cursor" stmt
				const USHORT trans_nm_len = strlen(request->req_trans);
				char* newtrans = (SCHAR *) MSC_alloc(trans_nm_len + 1);
				transaction = newtrans;
				memcpy(newtrans, request->req_trans, trans_nm_len);
			}
		}
		request->req_trans = transaction;
		relation = SQL_relation(request, r_name, db_name, owner_name, true);

		// Given the target relation, find the input context for the modify

		gpre_rse* select = request->req_rse;
		SSHORT i;
		for (i = 0; i < select->rse_count; i++)
		{
			input_context = select->rse_context[i];
			if (input_context->ctx_relation == relation)
				break;
		}

		if (i == select->rse_count)
			PAR_error("table not in request");

		// Resolve input fields first

		if (alias)
		{
			alias->sym_type = SYM_context;
			alias->sym_object = input_context;
			HSH_insert(alias);
		}

		for (ptr = set_list->nod_arg; ptr < end_list; ptr++)
		{
			gpre_nod* set_item = *ptr;
			SQE_resolve(set_item->nod_arg[0], request, 0);
			pair(set_item->nod_arg[0], set_item->nod_arg[1]);
		}

		update->upd_request = request;
		update->upd_source = input_context;
		gpre_ctx* update_context = MSC_context(request);
		update->upd_update = update_context;
		update_context->ctx_relation = relation;
		update->upd_assignments = set_list;

		act* action = MSC_action(request, ACT_update);

		// Resolve update fields next

		if (alias)
			alias->sym_object = update_context;

		for (ptr = set_list->nod_arg; ptr < end_list; ptr++)
		{
			gpre_nod* set_item = *ptr;
			SQE_resolve(set_item->nod_arg[1], request, 0);
			ref* field_ref = (ref*) ((set_item->nod_arg[1])->nod_arg[0]);

			slc* slice = NULL;
			act* slice_action = (act*) field_ref->ref_slice;
			if (slice_action && (slice = (slc*) slice_action->act_object))
			{
				// These gpreGlob.requests got lost in freeing the main request

				gpre_req* slice_request = slice_action->act_request;
				slice_request->req_next = gpreGlob.requests;
				gpreGlob.requests = slice_request;
				slice->slc_field_ref = field_ref;
				slice->slc_array = (gpre_nod*) set_item->nod_arg[0];
				slice->slc_parent_request = request;
				slice_action->act_type = ACT_put_slice;

				// If slice ref is not yet posted, post it.
				// This is required to receive the handle for the
				// array being updated

				bool found = false;
				for (ref* req_ref = request->req_references; req_ref; req_ref = req_ref->ref_next)
				{
					if (req_ref == field_ref)
					{
						set_item->nod_arg[1]->nod_arg[0] = (gpre_nod*) req_ref;
						found = true;
						break;
					}
					if ((req_ref->ref_field == field_ref->ref_field) &&
						(req_ref->ref_context == field_ref->ref_context))
					{
						PAR_error("Can't update multiple slices of same column");
					}
				}
				if (!found)
				{
					field_ref->ref_next = request->req_references;
					request->req_references = field_ref;
				}
			}
			pair(set_item->nod_arg[0], set_item->nod_arg[1]);
		}

		action->act_whenever = gen_whenever();
		action->act_object = (ref*) update;
		if (alias)
			HSH_remove(alias);
		return action;
	}

	request->req_trans = transaction;

	// How amusing.  After all that work, it wasn't a sub-action at all.
	// Neat.  Take the pieces and build a complete request.  Start by
	// figuring out what database is involved.

	// Generate record select expression, then resolve input values

	gpre_rse* select = (gpre_rse*) MSC_alloc(RSE_LEN(1));
	request->req_rse = select;
	select->rse_count = 1;
	select->rse_context[0] = input_context;

	if (!alias && !gpreGlob.token_global.tok_symbol)
		// may have a relation name put parser didn't know it when it parsed it
		gpreGlob.token_global.tok_symbol = HSH_lookup(gpreGlob.token_global.tok_string);

	for (ptr = set_list->nod_arg; ptr < end_list; ptr++)
	{
		gpre_nod* set_item = *ptr;
		SQE_resolve(set_item->nod_arg[0], request, select);
	}

	// Process boolean, if any

	if (where)
		select->rse_boolean = SQE_boolean(request, 0);

	// Resolve update fields to update context

	gpre_ctx* update_context = MSC_context(request);
	request->req_update = update_context;
	update_context->ctx_relation = relation;

	if (alias)
		alias->sym_object = update_context;

	for (ptr = set_list->nod_arg; ptr < end_list; ptr++)
	{
		gpre_nod* set_item = *ptr;
		SQE_resolve(set_item->nod_arg[1], request, 0);
		ref* field_ref = (ref*) ((set_item->nod_arg[1])->nod_arg[0]);

		act* slice_action = (act*) field_ref->ref_slice;
		if (slice_action)
		{
			// Slices not allowed in searched updates

			PAR_error("Updates of slices not allowed in searched updates");
		}

		// In dialect 1, neither the value being assigned (nod_arg[0])
		// nor the field to which it is being assigned (nod_arg[1]) may
		// be of a data type added in V6.
		if (SQL_DIALECT_V5 == gpreGlob.sw_sql_dialect)
		{
			for (int arg_num = 0; arg_num <= 1; arg_num++)
				if (nod_field == set_item->nod_arg[arg_num]->nod_type)
				{
					const USHORT field_dtype =
						((ref*) set_item->nod_arg[arg_num]->nod_arg[0])->ref_field->fld_dtype;
					if (dtype_sql_date == field_dtype || dtype_sql_time == field_dtype ||
						dtype_int64 == field_dtype)
					{
						SQL_dialect1_bad_type(field_dtype);
					}
				}
		}

		pair(set_item->nod_arg[0], set_item->nod_arg[1]);
	}

	gpre_nod* modify = MSC_node(nod_modify, 1);
	request->req_node = modify;
	modify->nod_arg[0] = set_list;

	act* action = MSC_action(request, ACT_loop);
	action->act_whenever = gen_whenever();

	if (alias)
		HSH_remove(alias);

	return action;
}


//____________________________________________________________
//
//		Handle an SQL whenever statement.  This is declaratory,
//		rather than a significant action.
//

static act* act_whenever()
{
	global_whenever_list = NULL; // global var

	// Pick up condition

	swe_condition_vals condition = SWE_max;
	if (MSC_match(KW_SQLERROR))
		condition = SWE_error;
	else if (MSC_match(KW_SQLWARNING))
		condition = SWE_warning;
	else if (MSC_match(KW_NOT) && MSC_match(KW_FOUND))
		condition = SWE_not_found;
	else
		CPR_s_error("WHENEVER condition");

	// Pick up action

	swe* label = NULL;
	if (MSC_match(KW_CONTINUE))
		label = NULL;
	else if ((MSC_match(KW_GO) && MSC_match(KW_TO)) || MSC_match(KW_GOTO))
	{
		MSC_match(KW_COLON);
		const USHORT len = gpreGlob.token_global.tok_length;
		label = (swe*) MSC_alloc(sizeof(swe) + len);
		label->swe_condition = condition;
		label->swe_length = len;
		if (len)
			memcpy(label->swe_label, gpreGlob.token_global.tok_string, len);

		PAR_get_token();
		label->swe_condition = condition;
	}
	else
		CPR_s_error("GO TO or CONTINUE");

	// Set up condition vector

	global_whenever[condition] = label;

	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = ACT_noop;

	MSC_match(KW_SEMI_COLON);
	return action;
}


//____________________________________________________________
//
//		Make sure that a file path doesn't contain a
//		Decnet node name.
//

static bool check_filename(const TEXT* name)
{
	const USHORT l = strlen(name);
	if (!l)
		return true;

	for (const TEXT* p = name; *p; p++)
	{
		if (p[0] == ':' && p[1] == ':')
			return false;
	}
	return true;
}


//____________________________________________________________
//
//		Parse connect options
//

static void connect_opts(const TEXT** user,
						 const TEXT** password,
						 const TEXT** sql_role,
						 const TEXT** lc_messages,
						 USHORT* buffers)
{
	for (;;)
	{
		if (MSC_match(KW_CACHE))
		{
			*buffers = atoi(gpreGlob.token_global.tok_string);
			PAR_get_token();
			MSC_match(KW_BUFFERS);
		}
		else if (MSC_match(KW_USER))
			*user = SQL_var_or_string(false);
		else if (MSC_match(KW_PASSWORD))
			*password = SQL_var_or_string(false);
		else if (MSC_match(KW_ROLE))
		{
			if (gpreGlob.token_global.tok_type == tok_ident)
			{
				// reserve extra bytes for quotes and NULL
				//TEXT* s = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 3);
				TEXT* s = (TEXT*) MSC_alloc(NAME_SIZE + 2);

				SQL_resolve_identifier("<Role Name>", s + 1, NAME_SIZE);
				s[0] = '\"';
				strcat(s, "\"");
				*sql_role = s;
			}
			else
				*sql_role = SQL_var_or_string(false);
		}
		else if (MSC_match(KW_LC_MESSAGES))
			*lc_messages = SQL_var_or_string(false);
		else
			break;
	}

}
//____________________________________________________________
//
//		Add a new file to an existing database.
//

static gpre_file* define_file()
{
	gpre_file* file = (gpre_file*) MSC_alloc(FIL_LEN);
	if (isQuoted(gpreGlob.token_global.tok_type))
	{
		TEXT* string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		file->fil_name = string;
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
		gpreGlob.token_global.tok_length += 2;
		PAR_get_token();
	}
	else
		CPR_s_error("<quoted filename>");

	if (!check_filename(file->fil_name))
		PAR_error("node name not permitted");	// A node name is not permitted in a shadow or secondary file name

	while (true)
	{
		if (MSC_match(KW_LENGTH))
		{
			MSC_match(KW_EQUALS);
			file->fil_length = EXP_ULONG_ordinal(true);
			MSC_match(KW_PAGES);
			MSC_match(KW_PAGE);
		}
		else if (MSC_match(KW_STARTS) || MSC_match(KW_STARTING))
		{
			MSC_match(KW_AT);
			MSC_match(KW_PAGE);
			file->fil_start = EXP_ULONG_ordinal(true);
		}
		else
			break;
	}

	return file;
}


//____________________________________________________________
//
//		define a log file
//

static gpre_file* define_log_file()
{
	gpre_file* file = (gpre_file*) MSC_alloc(FIL_LEN);
	if (isQuoted(gpreGlob.token_global.tok_type))
	{
		TEXT* string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		file->fil_name = string;
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
		PAR_get_token();
	}
	else
		CPR_s_error("<quoted filename>");

	if (!check_filename(file->fil_name))
		PAR_error("node name not permitted");	// A node name is not permitted in a shadow or secondary file name

	while (true)
	{
		if (MSC_match(KW_SIZE))
		{
			MSC_match(KW_EQUALS);
			file->fil_length = EXP_ULONG_ordinal(true);
		}
		else
			break;
	}

	return file;
}


static gpre_dbb* dup_dbb(const gpre_dbb* db)
{

// ****************************************
//
//		d u p _ d b b
//
// ****************************************
//
//		dirty duplication of a gpre_dbb.
//		just memcpy as no memory
//		is freed in gpre.
//
// *************************************
	if (!db)
		return NULL;
	gpre_dbb* newdb = (gpre_dbb*) MSC_alloc(DBB_LEN);
	// CVC: the casts here should be tested and removed.
	memcpy((SCHAR*) newdb, (const SCHAR*) db, DBB_LEN);

	return newdb;
}


//____________________________________________________________
//
//		Report an error with parameter
//

static void error(const TEXT* format, const TEXT* string2)
{
	TEXT buffer[ERROR_LENGTH];

	fb_utils::snprintf(buffer, sizeof(buffer), format, string2);
	PAR_error(buffer);
}


//____________________________________________________________
//
//		Extract string from "string" in
//		token.
//

static TEXT* extract_string(bool advance_token)
{
	switch (gpreGlob.sw_sql_dialect)
	{
	case 1:
		if (!isQuoted(gpreGlob.token_global.tok_type))
			CPR_s_error("<string>");
		break;

	default:
		if (gpreGlob.token_global.tok_type != tok_sglquoted)
			CPR_s_error("<string>");
		break;
	}

	TEXT* string = (TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
	MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
	if (advance_token)
		PAR_get_token();
	return string;
}



//____________________________________________________________
//
//		Generate a linked list of SQL WHENEVER items for error
//		handling.
//

static swe* gen_whenever()
{
	if (global_whenever_list)
		return global_whenever_list;

	global_whenever_list = NULL; // redundant
	swe* label = NULL;

	for (int i = 0; i < SWE_max; i++)
	{
	    const swe* proto = global_whenever[i];
		if (proto)
		{
			swe* prior = label;
			const USHORT len = proto->swe_length;
			label = (swe*) MSC_alloc(sizeof(swe) + len);
			label->swe_next = prior;
			label->swe_condition = proto->swe_condition;
			if (len)
				memcpy(label->swe_label, proto->swe_label, len);
		}
	}

	return label;
}


//____________________________________________________________
//
//		Correlate the into list with the select expression list
//		to form full references (post same against request).
//

static void into( gpre_req* request, gpre_nod* field_list, gpre_nod* var_list)
{
	if (!var_list)
		PAR_error("INTO list is required");

	if (!field_list || field_list->nod_count != var_list->nod_count)
		PAR_error("column count and number of INTO list host-variables unequal");

	gpre_nod** var_ptr = var_list->nod_arg;
	gpre_nod** fld_ptr = field_list->nod_arg;

	for (gpre_nod** const end = fld_ptr + var_list->nod_count; fld_ptr < end;fld_ptr++, var_ptr++)
	{
		ref* var_ref = (ref*) *var_ptr;
		ref* field_ref = NULL;
		gpre_req* slice_req = NULL;
		if (((*fld_ptr)->nod_type == nod_field) || ((*fld_ptr)->nod_type == nod_array))
		{
			field_ref = (ref*) (*fld_ptr)->nod_arg[0];
			if ((*fld_ptr)->nod_count > 2)
				slice_req = (gpre_req*) (*fld_ptr)->nod_arg[2];
		}

		ref* reference = NULL;
		gpre_fld* field;
		if (field_ref && slice_req && (field = field_ref->ref_field) && field->fld_array)
		{
			EXP_post_array(field_ref);

			// If field ref not posted yet, post it

			bool found = false;
			for (reference = request->req_references; reference; reference = reference->ref_next)
			{
				if (reference == field_ref)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				field_ref->ref_next = request->req_references;
				request->req_references = field_ref;
			}
			reference = field_ref;
		}
		else
			reference = SQE_post_reference(request, 0, 0, *fld_ptr);
		var_ref->ref_friend = reference;
		reference->ref_null_value = var_ref->ref_null_value;
	}
}


//____________________________________________________________
//
//		Create field in a relation for a metadata request.
//

static gpre_fld* make_field( gpre_rel* relation)
{
	char s[NAME_SIZE];

	SQL_resolve_identifier("<column name>", s, NAME_SIZE);
	gpre_fld* field = MET_make_field(s, 0, 0, true);
	field->fld_relation = relation;
	field->fld_flags |= FLD_meta;
	PAR_get_token();

	return field;
}


//____________________________________________________________
//
//		Create index for metadata request.
//

static gpre_index* make_index( gpre_req* request, const TEXT* string)
{
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
	{
		// CVC: I've kept this silly code. What's the idea of the copy here?
		// If we are trying to limit the index name, the correct length is NAME_SIZE.
		TEXT s[ERROR_LENGTH];
		strncpy(s, string, sizeof(s));
		s[sizeof(s) - 1] = 0;
		gpre_index* index = MET_make_index(s);
		if (request)
			request->req_database = gpreGlob.isc_databases;
		//index->ind_flags |= IND_meta; // nobody uses this info
		return index;
	}

	PAR_error("Can only reference INDEX in context of single database");
	return NULL; // silence compiler warning
}


//____________________________________________________________
//
//		Create relation for a metadata request.
//

static gpre_rel* make_relation( gpre_req* request, const TEXT* relation_name)
{
	if (gpreGlob.isc_databases && !gpreGlob.isc_databases->dbb_next)
	{
		TEXT r[ERROR_LENGTH];
		strncpy(r, relation_name, sizeof(r));
		r[sizeof(r) - 1] = 0;

		gpre_rel* relation = MET_make_relation(r);
		relation->rel_database = gpreGlob.isc_databases;
		relation->rel_meta = true;

		if (request)
			request->req_database = gpreGlob.isc_databases;
		return relation;
	}

	PAR_error("Can only reference TABLE in context of single database");
	return NULL; // silence compiler warning
}


//____________________________________________________________
//
//		Given two value expressions associated in a relational
//		expression, see if one is a field reference and the other
//		is a host language variable..  If so, match the field to the
//		host language variable.
//		In other words, here we are guessing what the datatype is
//		of a host language variable.
//

static void pair( gpre_nod* expr, gpre_nod* field_expr)
{
	if (field_expr->nod_type != nod_field)
		return;

	switch (expr->nod_type)
	{
	case nod_value:
		{
			ref* ref1 = (ref*) field_expr->nod_arg[0];
			ref* ref2 = (ref*) expr->nod_arg[0];

			// We're done if we've already determined the type of this reference
			// (if, for instance, it's a parameter to a udf or PROCEDURE)

			if (ref2->ref_field)
				return;

			ref2->ref_field = ref1->ref_field;
			if (ref1->ref_field->fld_array)
			{
				ref2->ref_context = ref1->ref_context;
				ref2->ref_friend = ref1;
				EXP_post_array(ref1);
				EXP_post_array(ref2);
			}
		}
		return;

	case nod_map_ref:
		{
			mel* element = (mel*) expr->nod_arg[0];
			pair(element->mel_expr, field_expr);
		}
		return;

	case nod_field:
	case nod_literal:
		return;
	}

	gpre_nod** ptr = expr->nod_arg;
	const gpre_nod* const* const end_ptr = ptr + expr->nod_count;

	for (; ptr < end_ptr; ptr++)
		pair(*ptr, field_expr);
}


//____________________________________________________________
//
//		Parse the multi-dimensional array specification.
//

static void par_array( gpre_fld* field)
{
	USHORT i = 0;

	// Pick up ranges

	ary* array_info = field->fld_array_info;

	while (true)
	{
		SLONG rangel = EXP_SSHORT_ordinal(true);
		SLONG rangeh;
		if (MSC_match(KW_COLON))
			rangeh = EXP_SSHORT_ordinal(true);
		else
		{
			if (rangel < 1)
				rangeh = 1;
			else
			{
				rangeh = rangel;
				rangel = 1;
			}
		}

		if (rangel >= rangeh)
			PAR_error("Start of array range must be less than end of range");

		if (i > MAX_ARRAY_DIMENSIONS)
			PAR_error("Array has too many dimensions");

		array_info->ary_rpt[i].ary_lower = rangel;
		array_info->ary_rpt[i++].ary_upper = rangeh;

		if (MSC_match(KW_R_BRCKET))
			break;

		if (!MSC_match(KW_COMMA))
			CPR_s_error(", (comma)");
	}

	array_info->ary_dimension_count = i;
}


//____________________________________________________________
//
//
//		Read in the specified character set on
//		a READ BLOB or an INSERT BLOB.
//

static SSHORT par_char_set()
{
	if (!MSC_match(KW_CHAR))
		CPR_s_error("CHARACTER SET");

	if (!MSC_match(KW_SET))
		CPR_s_error("CHARACTER SET");

	if (gpreGlob.token_global.tok_type != tok_ident)
		CPR_s_error("<character set name>");

	gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_charset);
	if (!symbol)
		PAR_error("The named CHARACTER SET was not found");

	PAR_get_token();

	return (((intlsym*) symbol->sym_object)->intlsym_charset_id);
}


//____________________________________________________________
//
//		Create a computed field
//

static void par_computed( gpre_req* request, gpre_fld* field)
{
	MSC_match(KW_BY);

	// If field size has been specified, save it.  Then NULL it
	// till the new size is calculated from the specified expression.
	// This will catch self references.
	//
	// The user specified values will be restored to override calculated
	// values.
	//
	// Don't move this initialization below.
	const gpre_fld save_fld = *field;

	CMPF cmp = (CMPF) MSC_alloc(CMPF_LEN);
	cmp->cmpf_text = CPR_start_text();
	cmp->cmpf_boolean = SQE_value(request, false, NULL, NULL);
	CPR_end_text(cmp->cmpf_text);

	field->fld_computed = cmp;
	field->fld_flags |= FLD_computed;

	CME_get_dtype(field->fld_computed->cmpf_boolean, field);

	// If there was user specified data type/size, restore it

	if (save_fld.fld_dtype)
	{
		field->fld_dtype = save_fld.fld_dtype;
		field->fld_length = save_fld.fld_length;
		field->fld_scale = save_fld.fld_scale;
		field->fld_sub_type = save_fld.fld_sub_type;
		field->fld_ttype = save_fld.fld_ttype;
		field->fld_charset_id = save_fld.fld_charset_id;
		field->fld_collate_id = save_fld.fld_collate_id;
		field->fld_char_length = save_fld.fld_char_length;
	}
}


//____________________________________________________________
//
//		Parse the next token as a cursor name.
//		If it is, return the request associated with the cursor.  If
//		not, produce an error and return NULL.
//

static gpre_req* par_cursor( gpre_sym** symbol_ptr)
{
	// par_cursor() is called to use a previously declared cursor.
	// tok_symbol == NULL means one of the two things.
	// a) The name does not belong to a cursor. OR
	// b) get_token() function in gpre.cpp was not able to find the cursor
	//    in hash table.
	//
	// case a) is an error condition.
	// case b) Could have resulted because the cursor name was upcased and
	//  inserted into hash table since it was not quoted and it is
	//  now being refered as it was declared.
	//
	// Hence, Try and lookup the cursor name after resolving it once more. If
	// it still cannot be located, Its an error

	SQL_resolve_identifier("<cursor name>", NULL, MAX_CURSOR_SIZE);
	gpre_sym* symbol = HSH_lookup(gpreGlob.token_global.tok_string);
	gpreGlob.token_global.tok_symbol = symbol;
	if (symbol && symbol->sym_type == SYM_keyword)
		gpreGlob.token_global.tok_keyword = (kwwords_t) symbol->sym_keyword;
	else
		gpreGlob.token_global.tok_keyword = KW_none;

	symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_cursor);

	if (!symbol)
		symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_delimited_cursor);

	if (symbol)
	{
		PAR_get_token();
		if (symbol_ptr)
			*symbol_ptr = symbol;
		return (gpre_req*) symbol->sym_object;
	}

	symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_dyn_cursor);
	if (symbol)
		PAR_error("DSQL cursors require DSQL update & delete statements");

	CPR_s_error("<cursor name>");
	return NULL;				// silence compiler
}


//____________________________________________________________
//
//		If this is a dynamic curser, return dynamic statement block.
//

static dyn* par_dynamic_cursor()
{
	gpre_sym* symbol = NULL;

	if (gpreGlob.token_global.tok_symbol == NULL)
	{
		SQL_resolve_identifier("<cursor name>", NULL, MAX_CURSOR_SIZE);
		gpreGlob.token_global.tok_symbol = symbol = HSH_lookup(gpreGlob.token_global.tok_string);
		if (symbol && symbol->sym_type == SYM_keyword)
			gpreGlob.token_global.tok_keyword = (kwwords_t) symbol->sym_keyword;
		else
			gpreGlob.token_global.tok_keyword = KW_none;
	}
	if (symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_dyn_cursor))
	{
		PAR_get_token();
		return (dyn*) symbol->sym_object;
	}

	return NULL;
}


//____________________________________________________________
//
//		Handle an SQL field definition in CREATE, DECLARE or
//		ALTER TABLE statement.
//

static gpre_fld* par_field( gpre_req* request, gpre_rel* relation)
{
	gpre_fld* field = make_field(relation);

	SQL_par_field_dtype(request, field, false);

	if (MSC_match(KW_COMPUTED))
	{
		if (field->fld_global)
			PAR_error("Cannot use domains to override computed column size");
		if (field->fld_array_info)
			PAR_error("Computed columns cannot be arrays");

		par_computed(request, field);
	}

	// Check if default value was specified

	if (gpreGlob.token_global.tok_keyword == KW_DEFAULT)
	{
		field->fld_default_source = CPR_start_text();
		PAR_get_token();

		if (MSC_match(KW_USER))
			field->fld_default_value = MSC_node(nod_user_name, 0);
		// Begin sql/date/time/timestamp
		else if (MSC_match(KW_CURRENT_DATE))
			field->fld_default_value = MSC_node(nod_current_date, 0);
		else if (MSC_match(KW_CURRENT_TIME))
			field->fld_default_value = MSC_node(nod_current_time, 0);
		else if (MSC_match(KW_CURRENT_TIMESTAMP))
			field->fld_default_value = MSC_node(nod_current_timestamp, 0);
		// End sql/date/time/timestamp
		else if (MSC_match(KW_NULL))
			field->fld_default_value = MSC_node(nod_null, 0);
		else
		{
			if (MSC_match(KW_MINUS))
			{
				if (gpreGlob.token_global.tok_type != tok_number)
					CPR_s_error("<number>");

				gpre_nod* literal_node = EXP_literal();
				field->fld_default_value = MSC_unary(nod_negate, literal_node);
			}
			else if ((field->fld_default_value = EXP_literal()) == NULL)
				CPR_s_error("<constant>");
		}
		CPR_end_text(field->fld_default_source);
	}


	// Check for any column level constraints

	cnstrt** constraint_ref = &field->fld_constraints;
	bool in_constraints = true;

	while (in_constraints)
	{
		switch (gpreGlob.token_global.tok_keyword)
		{
		case KW_CONSTRAINT:
		case KW_PRIMARY:
		case KW_UNIQUE:
		case KW_REFERENCES:
		case KW_CHECK:
		case KW_NOT:
			*constraint_ref = par_field_constraint(request, field);
			constraint_ref = &(*constraint_ref)->cnstrt_next;
			break;

		default:
			in_constraints = false;
			break;
		}
	}
	//if (MSC_match (KW_NOT))
	//if (MSC_match (KW_NULL))
	//{
	//	field->fld_flags |= FLD_not_null;
	//	if (MSC_match (KW_UNIQUE))
	//	{
	//		index = (gpre_index*) MSC_alloc (IND_LEN);
	//		index->ind_relation = relation;
	//		index->ind_fields = field;
	//		index->ind_flags |= IND_dup_flag | IND_meta;
	//		index->ind_flags &= ~IND_descend;
	//		field->fld_index = index;
	//	}
	//}
	//else
	//	CPR_s_error ("NULL");

	SQL_par_field_collate(request, field);
	SQL_adjust_field_dtype(field);

	return field;
}


//____________________________________________________________
//
//		Create a field level constraint as part of CREATE TABLE or
//		ALTER TABLE statement. Constraint maybe table or column level.
//

static cnstrt* par_field_constraint( gpre_req* request, gpre_fld* for_field)
{
	cnstrt* new_constraint = (cnstrt*) MSC_alloc(CNSTRT_LEN);

	if (gpreGlob.token_global.tok_keyword == KW_CONSTRAINT)
	{
		PAR_get_token();
		new_constraint->cnstrt_name = (STR) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<constraint name>",
							   new_constraint->cnstrt_name->str_string, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Constraint name too long");

		PAR_get_token();
	}

	STR field_name;
	const kwwords_t keyword = gpreGlob.token_global.tok_keyword;
	switch (keyword)
	{
	case KW_NOT:
		PAR_get_token();
		if (!MSC_match(KW_NULL))
			CPR_s_error("NULL");
		new_constraint->cnstrt_type = CNSTRT_NOT_NULL;
		for_field->fld_flags |= FLD_not_null;
		break;

	case KW_PRIMARY:
	case KW_UNIQUE:
	case KW_REFERENCES:
		PAR_get_token();
		switch (keyword)
		{
		case KW_PRIMARY:
			if (!MSC_match(KW_KEY))
				CPR_s_error("KEY");
			new_constraint->cnstrt_type = CNSTRT_PRIMARY_KEY;
			break;
		case KW_REFERENCES:
			new_constraint->cnstrt_type = CNSTRT_FOREIGN_KEY;
			break;
		default:
			new_constraint->cnstrt_type = CNSTRT_UNIQUE;
		}

		// Set field for PRIMARY KEY or FOREIGN KEY or UNIQUE constraint

		field_name = (STR) MSC_alloc(NAME_SIZE + 1);
		strcpy(field_name->str_string, for_field->fld_symbol->sym_string);
		MSC_push((gpre_nod*) field_name, &new_constraint->cnstrt_fields);

		if (keyword == KW_REFERENCES)
		{
			// Relation name for foreign key

			new_constraint->cnstrt_referred_rel = (STR) MSC_alloc(NAME_SIZE + 1);
			SQL_resolve_identifier("referred <table name>",
								   new_constraint->cnstrt_referred_rel->str_string,
								   NAME_SIZE + 1);
			if (gpreGlob.token_global.tok_length >= NAME_SIZE)
				PAR_error("Referred table name too long");

			PAR_get_token();

			if (MSC_match(KW_LEFT_PAREN))
			{
				// Field specified for referred relation

				field_name = (STR) MSC_alloc(NAME_SIZE + 1);
				SQL_resolve_identifier("<column name>", field_name->str_string, NAME_SIZE + 1);
				if (gpreGlob.token_global.tok_length >= NAME_SIZE)
					PAR_error("Referred field name too long");

				MSC_push((gpre_nod*) field_name, &new_constraint->cnstrt_referred_fields);
				CPR_token();
				EXP_match_paren();
			}

			if (gpreGlob.token_global.tok_keyword == KW_ON)
			{
				par_fkey_extension(new_constraint);
				PAR_get_token();
				if (gpreGlob.token_global.tok_keyword == KW_ON)
				{
					par_fkey_extension(new_constraint);
					PAR_get_token();
				}
			}
		}
		break;

	case KW_CHECK:
		PAR_get_token();
		new_constraint->cnstrt_type = CNSTRT_CHECK;
		new_constraint->cnstrt_text = CPR_start_text();
		new_constraint->cnstrt_boolean = SQE_boolean(request, 0);
		CPR_end_text(new_constraint->cnstrt_text);
		break;

	default:
		PAR_error("Invalid constraint type");
	}

	return new_constraint;
}


//____________________________________________________________
//
//		Parse the INTO clause for a dynamic SQL statement.
//		Nobody uses its returned value currently.

static bool par_into( dyn* statement)
{

	if (!MSC_match(KW_INTO))
		return false;

	MSC_match(KW_SQL);
	// "SQL" keyword is optional for backward compatibility

	if (!MSC_match(KW_DESCRIPTOR))
		CPR_s_error("DESCRIPTOR");

	statement->dyn_sqlda2 = PAR_native_value(false, false);

	return true;
}


//____________________________________________________________
//
//		Parse request options.
//

static void par_options(const TEXT** transaction)
{
	*transaction = NULL;

	if (!MSC_match(KW_TRANSACTION))
		return;

	*transaction = PAR_native_value(false, true);
}


//____________________________________________________________
//
//		parse the page_size clause of a
//		create database statement
//

static int par_page_size()
{
	MSC_match(KW_EQUALS);
	const int n1 = EXP_USHORT_ordinal(false);
	int n2 = n1;

	if (n1 <= 1024)
		n2 = 1024;
	else if (n1 <= 2048)
		n2 = 2048;
	else if (n1 <= 4096)
		n2 = 4096;
	else if (n1 <= 8192)
		n2 = 8192;
	else if (n1 <= 16384)
	    n2 = 16384;
	else
		CPR_s_error("<page size> in range 1024 to 16384");
	PAR_get_token();

	return n2;
}


//____________________________________________________________
//
//		Parse the next thing as a relation.
//

static gpre_rel* par_relation( gpre_req* request)
{
	SCHAR r_name[NAME_SIZE], db_name[NAME_SIZE], owner_name[NAME_SIZE];

	SQL_relation_name(r_name, db_name, owner_name);

	return make_relation(request, r_name);
}


//____________________________________________________________
//
//		Parse a dynamic statement name returning a dynamic
//		statement block.
//

static dyn* par_statement()
{
	dyn* statement = (dyn*) MSC_alloc(DYN_LEN);
	statement->dyn_statement_name = PAR_symbol(SYM_dummy);
	statement->dyn_database = gpreGlob.isc_databases;

	return statement;
}


//____________________________________________________________
//
//		Handle an extended foreign key definition as part of CREATE TABLE
//		or ALTER TABLE statements.
//

static void par_fkey_extension(cnstrt* cnstrt_val)
{
	// Extended foreign key definition could be as follows :
	//
	// [ON DELETE { NO ACTION | CASCADE | SET DEFAULT | SET NULL } ]
	// [ON UPDATE { NO ACTION | CASCADE | SET DEFAULT | SET NULL } ]

	fb_assert(gpreGlob.token_global.tok_keyword == KW_ON);
	fb_assert(cnstrt_val != NULL);

	PAR_get_token();

	const kwwords_t keyword = gpreGlob.token_global.tok_keyword;
	switch (keyword)
	{
	case KW_DELETE:
		// NOTE: action must be defined only once
		if (cnstrt_val->cnstrt_fkey_def_type & REF_DELETE_ACTION)
			CPR_s_error("UPDATE");
		else
			cnstrt_val->cnstrt_fkey_def_type |= REF_DELETE_ACTION;
		break;
	case KW_UPDATE:
		// NOTE: action must be defined only once
		if (cnstrt_val->cnstrt_fkey_def_type & REF_UPDATE_ACTION)
			CPR_s_error("DELETE");
		else
			cnstrt_val->cnstrt_fkey_def_type |= REF_UPDATE_ACTION;
		break;
	default:
		// unexpected keyword
		CPR_s_error("UPDATE or DELETE");
		break;
	}

	PAR_get_token();

	switch (gpreGlob.token_global.tok_keyword)
	{
	case KW_NO:
		PAR_get_token();
		if (gpreGlob.token_global.tok_keyword != KW_ACTION)
			CPR_s_error("ACTION");
		else if (keyword == KW_DELETE)
			cnstrt_val->cnstrt_fkey_def_type |= REF_DEL_NONE;
		else
			cnstrt_val->cnstrt_fkey_def_type |= REF_UPD_NONE;
		break;
	case KW_CASCADE:
		if (keyword == KW_DELETE)
			cnstrt_val->cnstrt_fkey_def_type |= REF_DEL_CASCADE;
		else
			cnstrt_val->cnstrt_fkey_def_type |= REF_UPD_CASCADE;
		break;
	case KW_SET:
		PAR_get_token();
		switch (gpreGlob.token_global.tok_keyword)
		{
		case KW_DEFAULT:
			if (keyword == KW_DELETE)
				cnstrt_val->cnstrt_fkey_def_type |= REF_DEL_SET_DEFAULT;
			else
				cnstrt_val->cnstrt_fkey_def_type |= REF_UPD_SET_DEFAULT;
			break;
		case KW_NULL:
			if (keyword == KW_DELETE)
				cnstrt_val->cnstrt_fkey_def_type |= REF_DEL_SET_NULL;
			else
				cnstrt_val->cnstrt_fkey_def_type |= REF_UPD_SET_NULL;
			break;
		default:
			// unexpected keyword
			CPR_s_error("NULL or DEFAULT");
			break;
		}
		break;
	default:
		// unexpected keyword
		CPR_s_error("NO ACTION or CASCADE or SET DEFAULT or SET NULL");
		break;
	}
}


//____________________________________________________________
//
//		Handle a create constraint verb as part of CREATE TABLE or
//		ALTER TABLE statement. Constraint maybe table or column level.
//

static cnstrt* par_table_constraint( gpre_req* request)
{
	cnstrt* constraint = (cnstrt*) MSC_alloc(CNSTRT_LEN);

	if (gpreGlob.token_global.tok_keyword == KW_CONSTRAINT)
	{
		PAR_get_token();
		constraint->cnstrt_name = (STR) MSC_alloc(NAME_SIZE + 1);
		SQL_resolve_identifier("<constraint name>",
							   constraint->cnstrt_name->str_string, NAME_SIZE + 1);
		if (gpreGlob.token_global.tok_length >= NAME_SIZE)
			PAR_error("Constraint name too long");

		PAR_get_token();
	}

	gpre_lls** fields;
	USHORT num_for_key_flds = 0, num_prim_key_flds = 0;

	const kwwords_t keyword = gpreGlob.token_global.tok_keyword;
	switch (keyword)
	{
	case KW_PRIMARY:
	case KW_UNIQUE:
	case KW_FOREIGN:
		PAR_get_token();
		switch (keyword)
		{
		case KW_PRIMARY:
			if (!MSC_match(KW_KEY))
				CPR_s_error("KEY");
			constraint->cnstrt_type = CNSTRT_PRIMARY_KEY;
			break;
		case KW_FOREIGN:
			if (!MSC_match(KW_KEY))
				CPR_s_error("KEY");
			constraint->cnstrt_type = CNSTRT_FOREIGN_KEY;
			break;
		default:
			constraint->cnstrt_type = CNSTRT_UNIQUE;
		}

		EXP_left_paren(0);

		// Get list of fields for PRIMARY KEY or FOREIGN KEY or UNIQUE constraint

		fields = &constraint->cnstrt_fields;
		do {
			STR field_name = (STR) MSC_alloc(NAME_SIZE + 1);
			SQL_resolve_identifier("<column name>", field_name->str_string, NAME_SIZE + 1);
			if (gpreGlob.token_global.tok_length >= NAME_SIZE)
				PAR_error("Field name too long");

			MSC_push((gpre_nod*) field_name, fields);
			fields = &(*fields)->lls_next;
			++num_for_key_flds;
			CPR_token();
		} while (MSC_match(KW_COMMA));

		if (keyword != KW_FOREIGN)
			num_for_key_flds = 0;

		EXP_match_paren();
		if (keyword == KW_FOREIGN)
		{
			if (!MSC_match(KW_REFERENCES))
				CPR_s_error("REFERENCES");

			// Relation name for foreign key

			constraint->cnstrt_referred_rel = (STR) MSC_alloc(NAME_SIZE + 1);
			SQL_resolve_identifier("referred <table name>",
								   constraint->cnstrt_referred_rel->str_string,
								   NAME_SIZE + 1);
			if (gpreGlob.token_global.tok_length >= NAME_SIZE)
				PAR_error("Referred table name too long");

			PAR_get_token();

			constraint->cnstrt_referred_fields = NULL;

			if (MSC_match(KW_LEFT_PAREN))
			{
				// Fields specified for referred relation

				fields = &constraint->cnstrt_referred_fields;
				do {
					STR field_name = (STR) MSC_alloc(NAME_SIZE + 1);
					SQL_resolve_identifier("<column name>", field_name->str_string, NAME_SIZE + 1);
					if (gpreGlob.token_global.tok_length >= NAME_SIZE)
						PAR_error("Referred field name too long");

					MSC_push((gpre_nod*) field_name, fields);
					fields = &(*fields)->lls_next;
					++num_prim_key_flds;
					CPR_token();
				} while (MSC_match(KW_COMMA));

				EXP_match_paren();
			}

			// Don't print error message in case if <referenced column list>
			// is not specified. Try to catch them in cmd.c[create_constraint]
			// later on
			if (constraint->cnstrt_referred_fields != NULL && num_prim_key_flds != num_for_key_flds)
			{
				PAR_error("FOREIGN KEY column count does not match PRIMARY KEY");
			}
			if (gpreGlob.token_global.tok_keyword == KW_ON)
			{
				par_fkey_extension(constraint);
				PAR_get_token();
				if (gpreGlob.token_global.tok_keyword == KW_ON)
				{
					par_fkey_extension(constraint);
					PAR_get_token();
				}
			}
		}						// if KW_FOREIGN
		break;

	case KW_CHECK:
		PAR_get_token();
		constraint->cnstrt_type = CNSTRT_CHECK;
		constraint->cnstrt_text = CPR_start_text();
		constraint->cnstrt_boolean = SQE_boolean(request, 0);
		CPR_end_text(constraint->cnstrt_text);
		break;

	default:
		PAR_error("Invalid constraint type");
	}

	return constraint;
}


//____________________________________________________________
//
//		Parse the isolation level part.
//		If expect_iso is true, an isolation level is required.
//		Returns true if found a match else false.
//

static bool par_transaction_modes(gpre_tra* trans, bool expect_iso)
{

	if (MSC_match(KW_READ))
	{
		if (MSC_match(KW_ONLY))
		{
			if (expect_iso)
				CPR_s_error("SNAPSHOT");

			trans->tra_flags |= TRA_ro;
			return true;
		}

		if (MSC_match(KW_WRITE))
		{
			if (expect_iso)
				CPR_s_error("SNAPSHOT");

			return true;
		}

		if (!(MSC_match(KW_COMMITTED) || MSC_match(KW_UNCOMMITTED)))
			CPR_s_error("COMMITTED");

		trans->tra_flags |= TRA_read_committed;

		if (MSC_match(KW_NO))
		{
			if (MSC_match(KW_VERSION))
				return true;
			if (MSC_match(KW_WAIT))
			{
				trans->tra_flags |= TRA_nw;
				return true;
			}
			CPR_s_error("WAIT or VERSION");
		}

		if (MSC_match(KW_VERSION))
			trans->tra_flags |= TRA_rec_version;

		return true;
	}

	if (MSC_match(KW_SNAPSHOT))
	{
		if (MSC_match(KW_TABLE))
		{
			trans->tra_flags |= TRA_con;

			MSC_match(KW_STABILITY);
		}
		return true;
	}

	return false;
}


//____________________________________________________________
//
//		Parse the USING clause for a dynamic SQL statement.
//

static bool par_using( dyn* statement)
{

	if (!MSC_match(KW_USING))
		return false;

	MSC_match(KW_SQL);
	// keyword "SQL" is optional for backward compatibility

	if (MSC_match(KW_DESCRIPTOR))
		statement->dyn_sqlda = PAR_native_value(false, false);
	else
		statement->dyn_using = (gpre_nod*) SQE_list(SQE_variable, NULL, false);

	return true;
}


//____________________________________________________________
//
//		Figure out the correct dtypes
//

static USHORT resolve_dtypes(kwwords_t typ, bool sql_date)
{
	TEXT err_mesg[ERROR_LENGTH];

	switch (typ)
	{
	case KW_DATE:
		if ((gpreGlob.sw_ods_version < 10) || (gpreGlob.sw_server_version < 6))
			switch (gpreGlob.sw_sql_dialect)
			{
			case 1:
				return dtype_timestamp;
			case 2:
				sprintf(err_mesg, "Encountered column type DATE which is ambiguous in dialect %d\n",
						gpreGlob.sw_sql_dialect);
				PAR_error(err_mesg);
				return dtype_unknown;	// TMN: FIX FIX
				// return;
			default:
				sprintf(err_mesg,
						"Encountered column type DATE which is not supported in ods version %d\n",
						gpreGlob.sw_ods_version);
				PAR_error(err_mesg);
			}
		else
		{
			// column definition SQL DATE is unambiguous in any dialect
			if (sql_date)
				return dtype_sql_date;

			switch (gpreGlob.sw_sql_dialect)
			{
			case 1:
				return dtype_timestamp;
			case 2:
				sprintf(err_mesg, "Encountered column type DATE which is ambiguous in dialect %d\n",
						gpreGlob.sw_sql_dialect);
				PAR_error(err_mesg);
				return dtype_unknown;	// TMN: FIX FIX
				// return;
			default:
				return dtype_sql_date;
			}
		}
		break;

	case KW_TIME:
		if ((gpreGlob.sw_ods_version < 10) || (gpreGlob.sw_server_version < 6))
		{
			sprintf(err_mesg,
					"Encountered column type TIME which is not supported by pre 6.0 Servers\n");
			PAR_error(err_mesg);
			return dtype_unknown;	// TMN: FIX FIX
			// return;
		}
		return dtype_sql_time;

	case KW_TIMESTAMP:
		return dtype_timestamp;

	default:
		sprintf(err_mesg, "resolve_dtypes(): Unknown dtype %d\n", typ);
		PAR_error(err_mesg);
		break;
	}

	// TMN: FIX FIX Added "return dtype_unknown;" to silence compiler, but
	// this is really a logic error we have to fix.

	return dtype_unknown;
}


//____________________________________________________________
//
//		Parse the tail of a CREATE DATABASE statement.
//

static bool tail_database(act_t action_type, gpre_dbb* database)
{
	TEXT* string = NULL;

	// parse options for the database parameter block

	bool extend_dpb = false;

	while (true)
	{
		if (MSC_match(KW_LENGTH))
		{
			database->dbb_length = EXP_ULONG_ordinal(true);
			MSC_match(KW_PAGES);
		}
		else if (MSC_match(KW_USER))
		{
			if (isQuoted(gpreGlob.token_global.tok_type))
			{
				database->dbb_c_user = string =
					(TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
				MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);

				// - 2 here ???
				string[gpreGlob.token_global.tok_length - 2] = '\0';
				PAR_get_token();
			}
			else
			{
				// Some non-C languages (such as Fortran) have problems
				// with extending the static DPB to add the runtime options
				// needed for a runtime user name or password.
				// 11 April 1995

				if (gpreGlob.sw_language == lang_c)
				{
					if (!MSC_match(KW_COLON))
						CPR_s_error("<colon> or <quoted string>");
					// Should I bother about dialects here ??
					if (isQuoted(gpreGlob.token_global.tok_type))
						CPR_s_error("<host variable>");
					database->dbb_r_user = PAR_native_value(false, false);
					extend_dpb = true;
				}
				else
					CPR_s_error("<quoted string>");
			}

		}
		else if (MSC_match(KW_PASSWORD))
		{
			if (isQuoted(gpreGlob.token_global.tok_type))
			{
				database->dbb_c_password = string =
					(TEXT*) MSC_alloc(gpreGlob.token_global.tok_length + 1);
				MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
				string[gpreGlob.token_global.tok_length] = '\0';
				PAR_get_token();
			}
			else
			{
				// Some non-C languages (such as Fortran) have problems
				// with extending the static DPB to add the runtime options
				// needed for a runtime user name or password.
				// 11 April 1995

				if (gpreGlob.sw_language == lang_c)
				{
					if (!MSC_match(KW_COLON))
						CPR_s_error("<colon> or <quoted string>");
					if (isQuoted(gpreGlob.token_global.tok_type))
						CPR_s_error("<host variable>");
					database->dbb_r_password = PAR_native_value(false, false);
					extend_dpb = true;
				}
				else
					CPR_s_error("<quoted string>");
			}
		}
		else if (MSC_match(KW_DEFAULT))
		{
			if (!MSC_match(KW_CHAR) || !MSC_match(KW_SET))
				CPR_s_error("CHARACTER SET");
			database->dbb_def_charset = PAR_native_value(false, false);
		}
		else
			break;
	}

	if (action_type == ACT_drop_database)
	{
		MSC_match(KW_CASCADE); // Commented the original code to avoid a warning
		//if (MSC_match(KW_CASCADE))
		//	; // ignore DROP CASCADE database->dbb_flags |= DBB_cascade;
		// TMN: ERROR ERROR we cant return _nothing* from a function returning
		// a bool. I changed this to false to flag an error, but we have to
		// look into this.
		return false;
		// return;
	}

	// parse add/drop items
	bool logdefined = false;
	while (true)
	{
		MSC_match(KW_ADD);
		if (MSC_match(KW_DROP))
		{
			if (MSC_match(KW_LOG_FILE))
				; // Ignore DROP LOG database->dbb_flags |= DBB_drop_log;
			else if (MSC_match(KW_CASCADE))
				; // ignore DROP CASCADE database->dbb_flags |= DBB_cascade;
			else
				PAR_error("only log files can be dropped");	// msg 121 only SECURITY_CLASS, DESCRIPTION and CACHE can be dropped

			//else if (MSC_match (KW_DESCRIP))
			//	database->dbb_flags	|= DBB_null_description;
			//else if (MSC_match (KW_SECURITY_CLASS))
			//	database->dbb_flags |= DBB_null_security_class;
		}

		//else if (gpreGlob.token_global.tok_keyword == KW_DESCRIPTION)
		//	database->dbb_description = parse_description();
		//else if (MSC_match (KW_SECURITY_CLASS))
		//	database->dbb_security_class = PARSE_symbol (tok_ident);

		else if (MSC_match(KW_FILE))
		{
			gpre_file* file = define_file();
			file->fil_next = database->dbb_files;
			database->dbb_files = file;
		}
		else if (MSC_match(KW_PAGE_SIZE) || MSC_match(KW_PAGESIZE))
		{
			if (action_type == ACT_alter_database)
				PAR_error("PAGE SIZE can not be modified");
			database->dbb_pagesize = par_page_size();
		}
		else if (MSC_match(KW_CHECK_POINT_LEN))
		{
			MSC_match(KW_EQUALS);
			EXP_ULONG_ordinal(true); // skip
		}
		else if (MSC_match(KW_NUM_LOG_BUFS))
		{
			MSC_match(KW_EQUALS);
			EXP_USHORT_ordinal(true); // skip
		}
		else if (MSC_match(KW_LOG_BUF_SIZE))
		{
			MSC_match(KW_EQUALS);
			EXP_USHORT_ordinal(true); // skip
		}
		else if (MSC_match(KW_GROUP_COMMIT_WAIT))
		{
			MSC_match(KW_EQUALS);
			EXP_ULONG_ordinal(true); // skip
		}
		else if (MSC_match(KW_LOG_FILE))
		{
			if (logdefined)
				PAR_error("log redefinition");
			logdefined = true;
			if (MSC_match(KW_LEFT_PAREN))
			{
				while (true)
				{
					gpre_file* logfile = define_log_file();
					logfile->fil_next = database->dbb_logfiles;
					database->dbb_logfiles = logfile;
					if (!MSC_match(KW_COMMA))
					{
						EXP_match_paren();
						break;
					}
				}

				if (MSC_match(KW_OVERFLOW))
					define_log_file();
				else
					PAR_error("Overflow log specification required for this configuration");
			}
			else if (MSC_match(KW_BASE_NAME))
			{
				database->dbb_flags |= DBB_log_serial;
				database->dbb_logfiles = define_log_file();
			}
		}
		else
			break;
	}

	return extend_dpb;
}


//____________________________________________________________
//
//		Upcase a string into another string.
//

static void to_upcase(const TEXT* p, TEXT* q, int target_size)
{
	UCHAR c;
	USHORT l = 0;

	while ((c = *p++) && (++l < target_size)) {
		*q++ = UPPER(c);
	}
	*q = 0;
}


//____________________________________________________________
//
// To do: move these to the correct position in the file.
// Idea: if we don't need a result in a variable, we don't pass it, since the
// internal buffer will be used instead (in that case, bigger size cannot surpass
// the size of a cursor). We can provide a size smaller than MAX_CURSOR size
// with the internal buffer. We should provide the correct size if we provide
// the output variable to put the result in it.

void SQL_resolve_identifier( const TEXT* err_mesg, TEXT* str_in, int in_size)
{
	static TEXT internal_buffer[MAX_CURSOR_SIZE];
	TEXT* str;
	int len;
	if (str_in)
	{
		str = str_in;
		len = in_size - 1;
	}
	else
	{
		str = internal_buffer;
		len = sizeof(internal_buffer) - 1;
		if (in_size > 0 && in_size <= len)
		    len = in_size - 1;
		else if (in_size > len + 1)
		    PAR_error("Provide your own buffer for sizes bigger than 64.");
	}

	TEXT* const tk_string = gpreGlob.token_global.tok_string;

	switch (gpreGlob.sw_sql_dialect)
	{
	case 2:
		if (gpreGlob.token_global.tok_type == tok_dblquoted)
			PAR_error("Ambiguous use of double quotes in dialect 2");
			// fall into
	case 1:
		if (gpreGlob.token_global.tok_type != tok_ident)
			CPR_s_error(err_mesg);
		else
			to_upcase(tk_string, str, len + 1);
		break;
	case 3:
		switch (gpreGlob.token_global.tok_type)
		{
		case tok_dblquoted:
			// strip_quotes is too dumb to handle C escape sequences
			// or SQL escape sequences in quoted identifiers.
			if (tk_string[0] == '\"')
				strip_quotes(gpreGlob.token_global);
			strncpy(str, tk_string, len);
			str[len] = 0;
			break;
		case tok_ident:
			to_upcase(tk_string, str, len + 1);
			break;
		default:
			CPR_s_error(err_mesg);
		}

		break;
	}
	strcpy(tk_string, str);
}


void SQL_dialect1_bad_type(USHORT field_dtype)
{
	char buffer[200];
	const char* s = "unknown";

	switch (field_dtype)
	{
	case dtype_sql_date:
		s = "SQL DATE";
		break;
	case dtype_sql_time:
		s = "SQL TIME";
		break;
	case dtype_int64:
		s = "64-bit numeric";
		break;
	}
	sprintf(buffer, "Client SQL dialect 1 does not support reference to the %s datatype", s);
	PAR_error(buffer);
}
