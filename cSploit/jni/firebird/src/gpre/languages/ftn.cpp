//____________________________________________________________
//
//		PROGRAM:	General preprocessor
//		MODULE:		ftn.cpp
//		DESCRIPTION:	Fortran text generator
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
//
//
//____________________________________________________________
//
//
// 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
// 2002.10.28 Sean Leyne - Completed removal of obsolete "SGI" port
//
//

#include "firebird.h"
#include <stdio.h>
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../yvalve/gds_proto.h"
#include "../gpre/gpre.h"
#include "../gpre/pat.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/lang_proto.h"
#include "../gpre/pat_proto.h"
#include "../common/prett_proto.h"
#include "../gpre/msc_proto.h"
#include "../common/utils_proto.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef NOT_USED_OR_REPLACED
static void align (int column);
#endif
static void	asgn_from (const act*, const ref*);
static void	asgn_to (const act*, const ref*);
static void	asgn_to_proc (const ref*);
static void	gen_any (const act*);
static void	gen_at_end (const act*);
static void	gen_based (const act*);
static void	gen_blob_close (const act*);
static void	gen_blob_end (const act*);
static void	gen_blob_for (const act*);
static void	gen_blob_open (const act*);
static void	gen_blr (void*, SSHORT, const char*);
static void	gen_clear_handles(); //(const act*);
#ifdef NOT_USED_OR_REPLACED
static void	gen_compatibility_symbol(const TEXT*, const TEXT*, const TEXT*);
#endif
static void	gen_compile (const act*);
static void	gen_create_database (const act*);
static void	gen_cursor_close(const gpre_req*);
static void	gen_cursor_init (const act*);
static void	gen_cursor_open (const act*, const gpre_req*);
static void	gen_database();
static void	gen_database_data(); // (const act*);
static void	gen_database_decls(); // (const act*);
static void	gen_ddl (const act*);
static void	gen_drop_database (const act*);
static void	gen_dyn_close (const act*);
static void	gen_dyn_declare (const act*);
static void	gen_dyn_describe(const act*, bool);
static void	gen_dyn_execute (const act*);
static void	gen_dyn_fetch (const act*);
static void	gen_dyn_immediate (const act*);
static void	gen_dyn_insert (const act*);
static void	gen_dyn_open (const act*);
static void	gen_dyn_prepare (const act*);
static void	gen_emodify (const act*);
static void	gen_estore (const act*);
static void	gen_end_fetch ();
static void	gen_endfor (const act*);
static void	gen_erase (const act*);
static SSHORT	gen_event_block (const act*);
static void	gen_event_init (const act*);
static void	gen_event_wait (const act*);
static void	gen_fetch (const act*);
static void	gen_finish (const act*);
static void	gen_for (const act*);
static void	gen_function(const act*);
static void	gen_get_or_put_slice(const act*, const ref*, bool);
static void	gen_get_segment (const act*);
static void	gen_loop (const act*);
static TEXT*	gen_name (SCHAR* const, const ref*, bool);
static void	gen_on_error (const act*);
static void	gen_procedure (const act*);
static void	gen_put_segment (const act*);


// RRK_?: the following prototype is different from C stuff
static void gen_raw(const UCHAR*, req_t, int, int, int);

static void	gen_ready (const act*);
static void	gen_receive (const act*, const gpre_port*);
static void	gen_release (const act*);
#ifdef NOT_USED_OR_REPLACED
static void	gen_request (const gpre_req*);
#endif
static void	gen_request_data (const gpre_req*);
static void	gen_request_decls (const gpre_req*);
static void	gen_return_value (const act*);
static void	gen_routine (const act*);
static void	gen_s_end (const act*);
static void	gen_s_fetch (const act*);
static void	gen_s_start (const act*);
static void	gen_segment (const act*);
static void	gen_select (const act*);
static void	gen_send (const act*, const gpre_port*);
static void	gen_slice (const act*);
static void	gen_start (const act*, const gpre_port*);
static void	gen_store (const act*);
static void	gen_t_start (const act*);
static void	gen_tpb_data(const tpb*);
static void	gen_tpb_decls(const tpb*);
static void	gen_trans (const act*);
static void	gen_type (const act*);
static void	gen_update (const act*);
static void	gen_variable (const act*);
static void	gen_whenever (const swe*);
static void	make_array_declaration (const ref*);
static TEXT* make_name (TEXT* const, const gpre_sym*);
static void	make_ok_test (const act*, const gpre_req*);
static void	make_port (const gpre_port*);
static void	make_ready (const gpre_dbb*, const TEXT*, const TEXT*, const gpre_req*);
static USHORT	next_label ();
static void	printa(const TEXT*, const TEXT*, ...);
#ifdef NOT_USED_OR_REPLACED
static void	printb(const TEXT*,  ...);
#endif
static const TEXT* request_trans (const act*, const gpre_req*);
static void	status_and_stop (const act*);
static const TEXT* status_vector(); // (const act*);
static void	t_start_auto (const gpre_req*, const TEXT*, const act*, bool);


static TEXT output_buffer[512];
static bool global_first_flag = false;
static adl* array_decl_list;

#if (defined AIX || defined AIX_PPC)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  \'%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "%VAL(";
const char* const I2CONST_2			= ")";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "%VAL(";
const char* const VAL_2				= ")";
const char* const REF_1				= "%REF(";
const char* const REF_2				= ")";
const char* const I4CONST_1			= "%VAL(";
const char* const I4CONST_2			= ")";
const char* const COMMENT			= "C     ";
const char* const INLINE_COMMENT	= "!";
const char* const COMMA				= ",";
#elif defined(__sun)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  \'%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "";
const char* const I2CONST_2			= "";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "\n*                ";
const char* const COMMA				= ",";
#elif defined(LINUX)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  \'%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "";
const char* const I2CONST_2			= "";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "\n*                ";
const char* const COMMA				= ",";
#elif defined(WIN_NT)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  \'%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "";
const char* const I2CONST_2			= "";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "\n*                ";
const char* const COMMA				= ",";
#elif (defined FREEBSD || defined NETBSD)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  \'%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "";
const char* const I2CONST_2			= "";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "\n*                ";
const char* const COMMA				= ",";
#elif defined(DARWIN)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  '/Library/Frameworks/Firebird.framework/Headers/gds.f\' \n\n";
const char* const INCLUDE_FTN_FILE	= "Firebird/gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "";
const char* const I2CONST_2			= "";
const char* const I2_1				= "";
const char* const I2_2				= "";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "\n*                ";
const char* const COMMA				= ",";
#elif defined(HPUX)
const char* const INCLUDE_ISC_FTN	= "       INCLUDE  '%s\' \n\n";
const char* const INCLUDE_FTN_FILE	= "gds.f";
const char* const DOUBLE_DCL		= "DOUBLE PRECISION";
const char* const I2CONST_1			= "ISC_INT2(";
const char* const I2CONST_2			= ")";
const char* const I2_1				= "ISC_INT2(";
const char* const I2_2				= ")";
const char* const VAL_1				= "";
const char* const VAL_2				= "";
const char* const REF_1				= "";
const char* const REF_2				= "";
const char* const I4CONST_1			= "";
const char* const I4CONST_2			= "";
const char* const COMMENT			= "*     ";
const char* const INLINE_COMMENT	= "!";
const char* const COMMA				= ",";
#endif

const char* const COLUMN			= "      ";
const char* const INDENT			= "   ";
const char* const CONTINUE			= "     +   ";
const char* const COLUMN_INDENT		= "          ";

const char* const ISC_EMBED_DSQL_CLOSE			= "isc_embed_dsql_close";
const char* const ISC_EMBED_DSQL_DECLARE		= "isc_embed_dsql_declare";
const char* const ISC_EMBED_DSQL_DESCRIBE		= "isc_embed_dsql_describe";
const char* const ISC_EMBED_DSQL_DESCRIBE_BIND	= "isc_embed_dsql_describe_bind";
const char* const ISC_EMBED_DSQL_EXECUTE 		= "isc_embed_dsql_execute";
const char* const ISC_EMBED_DSQL_EXECUTE2		= "isc_embed_dsql_execute2";
const char* const ISC_EMBED_DSQL_EXECUTE_IMMEDIATE	= "isc_embed_dsql_execute_immed";
const char* const ISC_EMBED_DSQL_EXECUTE_IMMEDIATE2	= "isc_embed_dsql_execute_immed2";
const char* const ISC_EMBED_DSQL_FETCH		= "isc_embed_dsql_fetch";
const char* const ISC_EMBED_DSQL_INSERT		= "isc_embed_dsql_insert";
const char* const ISC_EMBED_DSQL_OPEN		= "isc_embed_dsql_open";
const char* const ISC_EMBED_DSQL_OPEN2		= "isc_embed_dsql_open2";
const char* const ISC_EMBED_DSQL_PREPARE	= "isc_embed_dsql_prepare";
const char* const ISC_DSQL_ALLOCATE			= "isc_dsql_alloc_statement2";
const char* const ISC_DSQL_EXECUTE			= "isc_dsql_execute_m";
const char* const ISC_DSQL_FREE				= "isc_dsql_free_statement";
const char* const ISC_DSQL_SET_CURSOR		= "isc_dsql_set_cursor_name";

const char* const ISC_EVENT_WAIT			= "ISC_EVENT_WAIT";
const char* const ISC_EVENT_COUNTS			= "ISC_EVENT_COUNTS";

const char* const DSQL_I2CONST_1			= I2CONST_1;
const char* const DSQL_I2CONST_2			= I2CONST_2;

const char* const NULL_SQLDA	= "0";


//____________________________________________________________
//
//

void FTN_action(const act* action, int /*column*/)
{
	if (action->act_flags & ACT_break)
		global_first_flag = false;

	switch (action->act_type)
	{
	case ACT_alter_database:
	case ACT_alter_domain:
	case ACT_alter_table:
	case ACT_alter_index:
	case ACT_create_domain:
	case ACT_create_generator:
	case ACT_create_index:
	case ACT_create_shadow:
	case ACT_create_table:
	case ACT_create_view:
	case ACT_declare_filter:
	case ACT_declare_udf:
	case ACT_drop_domain:
	case ACT_drop_filter:
	case ACT_drop_index:
	case ACT_drop_shadow:
	case ACT_drop_table:
	case ACT_drop_udf:
	case ACT_statistics:
	case ACT_drop_view:
		gen_ddl(action);
		break;
	case ACT_any:
		gen_any(action);
		return;
	case ACT_at_end:
		gen_at_end(action);
		return;
	case ACT_commit:
		gen_trans(action);
		break;
	case ACT_commit_retain_context:
		gen_trans(action);
		break;
	case ACT_basedon:
		gen_based(action);
		return;
	case ACT_b_declare:
		gen_database();
		return;
	case ACT_blob_cancel:
		gen_blob_close(action);
		return;
	case ACT_blob_close:
		gen_blob_close(action);
		break;
	case ACT_blob_create:
		gen_blob_open(action);
		break;
	case ACT_blob_for:
		gen_blob_for(action);
		return;
	case ACT_blob_handle:
		gen_segment(action);
		return;
	case ACT_blob_open:
		gen_blob_open(action);
		break;
	case ACT_clear_handles:
		gen_clear_handles(); //(action);
		break;
	case ACT_create_database:
		gen_create_database(action);
		break;
	case ACT_cursor:
		gen_cursor_init(action);
		return;
	case ACT_database:
		gen_database();
		return;
	case ACT_disconnect:
		gen_finish(action);
		break;
	case ACT_drop_database:
		gen_drop_database(action);
		break;
	case ACT_dyn_close:
		gen_dyn_close(action);
		break;
	case ACT_dyn_cursor:
		gen_dyn_declare(action);
		break;
	case ACT_dyn_describe:
		gen_dyn_describe(action, false);
		break;
	case ACT_dyn_describe_input:
		gen_dyn_describe(action, true);
		break;
	case ACT_dyn_execute:
		gen_dyn_execute(action);
		break;
	case ACT_dyn_fetch:
		gen_dyn_fetch(action);
		break;
	case ACT_dyn_grant:
		gen_ddl(action);
		break;
	case ACT_dyn_immediate:
		gen_dyn_immediate(action);
		break;
	case ACT_dyn_insert:
		gen_dyn_insert(action);
		break;
	case ACT_dyn_open:
		gen_dyn_open(action);
		break;
	case ACT_dyn_prepare:
		gen_dyn_prepare(action);
		break;
	case ACT_dyn_revoke:
		gen_ddl(action);
		break;
	case ACT_close:
		gen_s_end(action);
		break;
	case ACT_endblob:
		gen_blob_end(action);
		return;
	case ACT_enderror:
		printa(COLUMN, "END IF");
		return;
	case ACT_endfor:
		gen_endfor(action);
		break;
	case ACT_endmodify:
		gen_emodify(action);
		break;
	case ACT_endstore:
		gen_estore(action);
		break;
	case ACT_erase:
		gen_erase(action);
		return;
	case ACT_event_init:
		gen_event_init(action);
		break;
	case ACT_event_wait:
		gen_event_wait(action);
		break;
	case ACT_fetch:
		gen_fetch(action);
		break;
	case ACT_finish:
		gen_finish(action);
		break;
	case ACT_for:
		gen_for(action);
		return;
	case ACT_function:
		gen_function(action);
		return;
	case ACT_get_segment:
		gen_get_segment(action);
		break;
	case ACT_get_slice:
		gen_slice(action);
		break;
	case ACT_hctef:
		gen_end_fetch();
		return;
	case ACT_insert:
		gen_s_start(action);
		break;
	case ACT_loop:
		gen_loop(action);
		break;
	case ACT_open:
		gen_s_start(action);
		break;
	case ACT_on_error:
		gen_on_error(action);
		return;
	case ACT_procedure:
		gen_procedure(action);
		return;
	case ACT_put_segment:
		gen_put_segment(action);
		break;
	case ACT_put_slice:
		gen_slice(action);
		break;
	case ACT_ready:
		gen_ready(action);
		return;
	case ACT_release:
		gen_release(action);
		break;
	case ACT_rfinish:
		gen_finish(action);
		return;
	case ACT_prepare:
		gen_trans(action);
		break;
	case ACT_rollback:
		gen_trans(action);
		break;
	case ACT_rollback_retain_context:
		gen_trans(action);
		break;
	case ACT_routine:
		gen_routine(action);
		return;
	case ACT_s_end:
		gen_s_end(action);
		return;
	case ACT_s_fetch:
		gen_s_fetch(action);
		break;
	case ACT_s_start:
		gen_s_start(action);
		break;
	case ACT_select:
		gen_select(action);
		break;
	case ACT_segment_length:
		gen_segment(action);
		return;
	case ACT_segment:
		gen_segment(action);
		return;
	case ACT_sql_dialect:
		gpreGlob.sw_sql_dialect = ((set_dialect*) action->act_object)->sdt_dialect;
		return;
	case ACT_start:
		gen_t_start(action);
		break;
	case ACT_store:
		gen_store(action);
		break;
	case ACT_store2:
		gen_return_value(action);
		break;
	case ACT_type_number:
		gen_type(action);
		return;
	case ACT_update:
		gen_update(action);
		break;
	case ACT_variable:
		gen_variable(action);
		return;
	default:
		return;
	}

	if (action->act_flags & ACT_sql)
		gen_whenever(action->act_whenever);
}


//____________________________________________________________
//
//       Create a block data module at the
//       head of a preprocessed fortran file
//       containing the initializations for
//       all databases not declared as extern
//

void FTN_fini()
{
	if (!gpreGlob.global_db_count)
		return;

	fprintf(gpreGlob.out_file, "\n");
	printa(COLUMN, "BLOCK DATA");

	const dbd* db_list = gpreGlob.global_db_list;
	for (const dbd* const end = gpreGlob.global_db_list + gpreGlob.global_db_count;
		 db_list < end; ++db_list)
	{
		const TEXT* name = db_list->dbd_name;
		fprintf(gpreGlob.out_file, "%sINTEGER*4  %s                %s{ database handle }\n",
				   COLUMN, name, INLINE_COMMENT);
		fprintf(gpreGlob.out_file, "%sCOMMON /%s/ %s\n", COLUMN, name, name);
		fprintf(gpreGlob.out_file, "%sDATA %s /0/             %s{ init database handle }\n",
				   COLUMN, name, INLINE_COMMENT);
	}
	printa(COLUMN, "END");
}


//____________________________________________________________
//
//		Print a statment, breaking it into
//       reasonable 72 character hunks.
//

void FTN_print_buffer( TEXT* output_bufferL)
{
	TEXT s[73];
	TEXT* p = s;

	for (const TEXT* q = output_bufferL; *q; q++)
	{
		*p++ = *q;
#ifdef __sun
		if (q[0] == '\n' && q[1] == '\0')
		{
			*p = 0;
			fprintf(gpreGlob.out_file, "%s", s);
			p = s;
		}
#endif
		if ((p - s) > 71)
		{
			for (p--; (*p != ',') && (*p != ' '); p--)
			{
				if ((p - s) < 10)
				{
					p += 51;
					q += 50;
					*p-- = 0;
					TEXT err[128];
					sprintf(err, "Output line overflow: %s", s);
					CPR_error(err);
					break;
				}
				q--;
			}

			*++p = 0;
			fprintf(gpreGlob.out_file, "%s\n", s);
			strcpy(s, CONTINUE);
			for (p = s; *p; p++);
		}
	}
	*p = 0;
	fprintf(gpreGlob.out_file, "%s", s);
	output_bufferL[0] = 0;
}


// RRK_?: copy align from c_cxx


//____________________________________________________________
//
//		Build an assignment from a host language variable to
//		a port variable.
//

static void asgn_from(const act* action, const ref* reference)
{
	SCHAR name[MAX_REF_SIZE], variable[MAX_REF_SIZE], temp[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_array_info)
			if (!(reference->ref_flags & REF_array_elem))
			{
				printa(COLUMN, "CALL isc_qtoq (isc_blob_null, %s)", gen_name(name, reference, true));
				gen_get_or_put_slice(action, reference, false);
				continue;
			}

		if (!reference->ref_source && !reference->ref_value)
			continue;
		gen_name(variable, reference, true);

		const TEXT* value;
		if (reference->ref_source)
			value = gen_name(temp, reference->ref_source, true);
		else
			value = reference->ref_value;
		if (reference->ref_value && (reference->ref_flags & REF_array_elem))
			field = field->fld_array;
		if (field->fld_dtype == dtype_blob || field->fld_dtype == dtype_quad ||
			field->fld_dtype == dtype_date)
		{
			sprintf(output_buffer, "%sCALL isc_qtoq (%s, %s)\n", COLUMN, value, variable);
		}
		else if (!reference->ref_master || (reference->ref_flags & REF_literal))
		{
			sprintf(output_buffer, "%s%s = %s\n", COLUMN, variable, value);
		}
		else
		{
			sprintf(output_buffer, "%sIF (%s .LT. 0) THEN\n", COLUMN, value);
			FTN_print_buffer(output_buffer);
			sprintf(output_buffer, "%s%s = -1\n", COLUMN_INDENT, variable);
			FTN_print_buffer(output_buffer);
			sprintf(output_buffer, "%sELSE\n", COLUMN);
			FTN_print_buffer(output_buffer);
			sprintf(output_buffer, "%s%s = 0\n", COLUMN_INDENT, variable);
			FTN_print_buffer(output_buffer);
			sprintf(output_buffer, "%sEND IF\n", COLUMN);
		}
		FTN_print_buffer(output_buffer);
	}
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to(const act* action, const ref* reference)
{
	ref* source = reference->ref_friend;
	const gpre_fld* field = source->ref_field;
	if (field->fld_array_info)
	{
		source->ref_value = reference->ref_value;
		gen_get_or_put_slice(action, source, true);
		return;
	}

    SCHAR s[MAX_REF_SIZE];
	gen_name(s, source, true);

	if (field->fld_dtype == dtype_blob || field->fld_dtype == dtype_quad ||
		field->fld_dtype == dtype_date)
	{
		sprintf(output_buffer, "%sCALL isc_qtoq (%s, %s)\n", COLUMN, s, reference->ref_value);
	}
	else
		sprintf(output_buffer, "%s%s = %s\n", COLUMN, reference->ref_value, s);
	FTN_print_buffer(output_buffer);

	// Pick up NULL value if one is there

	if (reference = reference->ref_null)
	{
		sprintf(output_buffer, "%s%s = %s\n",
				COLUMN, reference->ref_value, gen_name(s, reference, true));
		FTN_print_buffer(output_buffer);
	}
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to_proc( const ref* reference)
{
	SCHAR s[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		if (!reference->ref_value)
			continue;
		const gpre_fld* field = reference->ref_field;
		gen_name(s, reference, true);
		if (field->fld_dtype == dtype_blob || field->fld_dtype == dtype_quad ||
			field->fld_dtype == dtype_date)
		{
			sprintf(output_buffer, "%sCALL isc_qtoq (%s, %s)\n", COLUMN, s, reference->ref_value);
		}
		else
			sprintf(output_buffer, "%s%s = %s\n", COLUMN, reference->ref_value, s);
		FTN_print_buffer(output_buffer);
	}
}


//____________________________________________________________
//
//		Generate code for AT END clause of FETCH.
//

static void gen_at_end(const act* action)
{
	SCHAR s[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	printa(COLUMN, "IF (%s .EQ. 0) THEN", gen_name(s, request->req_eof, true));
	fprintf(gpreGlob.out_file, COLUMN);
}


//____________________________________________________________
//
//		Substitute for a BASED ON <field name> clause.
//

static void gen_based(const act* action)
{
	USHORT datatype;
	SLONG length = 0;

	bas* based_on = (bas*) action->act_object;
	const gpre_fld* field = based_on->bas_field;

	if (based_on->bas_flags & BAS_segment)
	{
		datatype = dtype_text;
		if (!(length = field->fld_seg_length))
			length = 256;
	}
	else if (field->fld_array_info)
		datatype = field->fld_array_info->ary_dtype;
	else
		datatype = field->fld_dtype;

	switch (datatype)
	{
	case dtype_short:
		fprintf(gpreGlob.out_file, "%sINTEGER*2%s", COLUMN, COLUMN);
		break;

	case dtype_long:
		fprintf(gpreGlob.out_file, "%sINTEGER*4%s", COLUMN, COLUMN);
		break;

	case dtype_date:
	case dtype_blob:
	case dtype_quad:
		fprintf(gpreGlob.out_file, "%sINTEGER*4%s", COLUMN, COLUMN);
		break;

	case dtype_text:
		fprintf(gpreGlob.out_file, "%sCHARACTER*%"SLONGFORMAT"%s", COLUMN,
				(based_on->bas_flags & BAS_segment) ? length : ((field->fld_array_info) ?
					field->fld_array->fld_length : field->fld_length),
				COLUMN);
		break;

	case dtype_real:
		fprintf(gpreGlob.out_file, "%sREAL%s", COLUMN, COLUMN);
		break;

	case dtype_double:
		fprintf(gpreGlob.out_file, "%s%s%s", COLUMN, DOUBLE_DCL, COLUMN);
		break;

	default:
	    {
	    	TEXT s[64];
			sprintf(s, "datatype %d unknown\n", field->fld_dtype);
			CPR_error(s);
			return;
		}
	}

	// print the first variable, then precede the rest with commas

	bool first = true;

	while (based_on->bas_variables)
	{
		const TEXT* variable = (const TEXT*) MSC_pop(&based_on->bas_variables);
		if (!first)
			fprintf(gpreGlob.out_file, ",\n%s", CONTINUE);
		fprintf(gpreGlob.out_file, "%s", variable);
		first = false;
		if (field->fld_array_info && !(based_on->bas_flags & BAS_segment))
		{
			// Print out the dimension part of the declaration
			fprintf(gpreGlob.out_file, "(");

			for (dim* dimension = field->fld_array_info->ary_dimension; dimension;
				 dimension = dimension->dim_next)
			{
				if (dimension->dim_lower != 1)
					fprintf(gpreGlob.out_file, "%"SLONGFORMAT":", dimension->dim_lower);

				fprintf(gpreGlob.out_file, "%"SLONGFORMAT, dimension->dim_upper);
				if (dimension->dim_next)
					fprintf(gpreGlob.out_file, ", ");
			}

			if (field->fld_dtype == dtype_quad || field->fld_dtype == dtype_date)
			{
				fprintf(gpreGlob.out_file, ",2");
			}

			fprintf(gpreGlob.out_file, ")");
		}

		else if (field->fld_dtype == dtype_blob || field->fld_dtype == dtype_quad ||
			field->fld_dtype == dtype_date)
		{
			fprintf(gpreGlob.out_file, "(2)");
		}
	}

	fprintf(gpreGlob.out_file, "\n");
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_close(const act* action)
{
	blb* blob;

	if (action->act_flags & ACT_sql)
	{
		gen_cursor_close(action->act_request);
		blob = (blb*) action->act_request->req_blobs;
	}
	else
		blob = (blb*) action->act_object;

	const TEXT* command = (action->act_type == ACT_blob_cancel) ? "CANCEL" : "CLOSE";
	printa(COLUMN, "CALL ISC_%s_BLOB (%s, isc_%d)", command, status_vector(), blob->blb_ident);

	if (action->act_flags & ACT_sql)
	{
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
	}

	status_and_stop(action);
}


//____________________________________________________________
//
//		End a blob FOR loop.
//

static void gen_blob_end(const act* action)
{
	blb* blob = (blb*) action->act_object;
	printa(COLUMN, "%sGOTO %d", INDENT, blob->blb_top_label);
	printa("", "%-6dCONTINUE", blob->blb_btm_label);
	if (action->act_error)
		printa(COLUMN, "CALL ISC_CANCEL_BLOB (ISC_STATUS2, isc_%d)", blob->blb_ident);
	else
		printa(COLUMN, "CALL ISC_CANCEL_BLOB (%s, isc_%d)", status_vector(), blob->blb_ident);
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_for(const act* action)
{
	blb* blob = (blb*) action->act_object;
	blob->blb_top_label = next_label();
	blob->blb_btm_label = next_label();
	gen_blob_open(action);
	if (action->act_error)
		printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0) GOTO %d\n", blob->blb_btm_label);
	printa("", "%-6dCONTINUE", blob->blb_top_label);
	gen_get_segment(action);
	printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0 .AND. ISC_STATUS(2) .NE. ISC_SEGMENT) THEN");
	printa(COLUMN, "%s GOTO %d", INDENT, blob->blb_btm_label);
	printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate the call to open (or create) a blob.
//

static void gen_blob_open(const act* action)
{
	TEXT s[MAX_REF_SIZE];
	const TEXT* pattern1 =
		"CALL ISC_%IFCREATE%ELOPEN%EN_BLOB2 (%V1, %RF%DH%RE, %RF%RT%RE, %RF%BH%RE, %RF%FR%RE, %N1, %I1)\n";
	const TEXT* pattern2 =
		"CALL ISC_%IFCREATE%ELOPEN%EN_BLOB2 (%V1, %RF%DH%RE, %RF%RT%RE, %RF%BH%RE, %RF%FR%RE, 0, 0)\n";

	blb* blob;
	const ref* reference;
	if (action->act_flags & ACT_sql)
	{
		if (gpreGlob.sw_auto)
		{
			t_start_auto(action->act_request, status_vector(), action, true);
			printa(COLUMN, "if (%s .ne. 0) then", request_trans(action, action->act_request));
		}

		gen_cursor_open(action, action->act_request);
		blob = (blb*) action->act_request->req_blobs;
		reference = ((open_cursor*) action->act_object)->opn_using;
		gen_name(s, reference, true);
	}
	else
	{
		blob = (blb*) action->act_object;
		reference = blob->blb_reference;
	}

	PAT args;
	args.pat_condition = (action->act_type == ACT_blob_create);	// open or create blob
	args.pat_vector1 = status_vector();
	args.pat_database = blob->blb_request->req_database;	// database handle
	args.pat_request = blob->blb_request;	// transaction handle
	args.pat_blob = blob;		// blob handle
	args.pat_reference = reference;	// blob identifier
	args.pat_ident1 = blob->blb_bpb_ident;

	if ((action->act_flags & ACT_sql) && action->act_type == ACT_blob_open) {
		printa(COLUMN, "CALL isc_qtoq (%s, %s)", reference->ref_value, s);
	}

    const USHORT column = 6;

	if (args.pat_value1 = blob->blb_bpb_length)
		PATTERN_expand(column, pattern1, &args);
	else
		PATTERN_expand(column, pattern2, &args);

	if (action->act_flags & ACT_sql)
	{
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
		if (gpreGlob.sw_auto)
			printa(COLUMN, "END IF");
		status_and_stop(action);
		if (action->act_type == ACT_blob_create)
		{
			printa(COLUMN, "IF (SQLCODE .EQ. 0) THEN");
			printa(COLUMN_INDENT, "CALL isc_qtoq (%s, %s)", s, reference->ref_value);
			printa(COLUMN, "ENDIF");
		}
	}
	else
		status_and_stop(action);
}


//____________________________________________________________
//
//		Callback routine for BLR pretty printer.
//

static void gen_blr(void* /*user_arg*/, SSHORT /*offset*/, const char* string)
{
	const int c_len = strlen(COMMENT);
	const int len = strlen(string);
	int from = 0;
	int to = 80 - c_len;

	while (from < len)
	{
		if (to < len)
		{
			char buffer[81];
			strncpy(buffer, string + from, 80 - c_len);
			buffer[80 - c_len] = 0;
			fprintf(gpreGlob.out_file, "%s%s\n", COMMENT, buffer);
		}
		else
			fprintf(gpreGlob.out_file, "%s%s\n", COMMENT, string + from);
		from = to;
		to = to + 80 - c_len;
	}
}


//____________________________________________________________
//
//		Generate text to compile a request.
//

static void gen_compile(const act* action)
{
	const gpre_req* request = action->act_request;
	const gpre_dbb* db = request->req_database;
	const gpre_sym* symbol = db->dbb_name;

	// generate automatic ready if appropriate

	if (gpreGlob.sw_auto)
		t_start_auto(request, status_vector(), action, true);

	// always generate a compile, a test for the success of the compile,
	// and an end to the 'if not compiled test

	// generate an 'if not compiled'

	if (gpreGlob.sw_auto && (action->act_error || (action->act_flags & ACT_sql)))
		printa(COLUMN, "IF (%s .EQ. 0 .AND. %s .NE. 0) THEN",
			   request->req_handle, request_trans(action, request));
	else
		printa(COLUMN, "IF (%s .EQ. 0) THEN", request->req_handle);

	sprintf(output_buffer, "%sCALL ISC_COMPILE_REQUEST%s (%s, %s, %s, %s%d%s, %sisc_%d%s)\n",
			COLUMN, (request->req_flags & REQ_exp_hand) ? "" : "2",
			status_vector(), symbol->sym_string, request->req_handle,
			I2CONST_1, request->req_length, I2CONST_2, REF_1,
			request->req_ident, REF_2);
	FTN_print_buffer(output_buffer);
	status_and_stop(action);
	printa(COLUMN, "END IF");

	// If blobs are present, zero out all of the blob handles.  After this
	// point, the handles are the user's responsibility

	for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
	{
		sprintf(output_buffer, "%sisc_%d = 0\n", COLUMN, blob->blb_ident);
		FTN_print_buffer(output_buffer);
	}
}




//____________________________________________________________
//
//		Generate a call to create a database.
//

static void gen_create_database(const act* action)
{
	TEXT s1[32], s2[32];

	const gpre_req* request = ((mdbb*) action->act_object)->mdbb_dpb_request;
	const gpre_dbb* db = (gpre_dbb*) request->req_database;
	sprintf(s1, "isc_%dl", request->req_ident);

	if (request->req_flags & REQ_extend_dpb)
	{
		sprintf(s2, "isc_%dp", request->req_ident);
		if (request->req_length)
		{
			sprintf(output_buffer, "%s%s = isc_%d\n", COLUMN, s2, request->req_ident);
			FTN_print_buffer(output_buffer);
		}
		if (db->dbb_r_user)
		{
			sprintf(output_buffer,
					"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_user_name, %s, %sLEN(%s)%s)\n",
					COLUMN,
					s2, s1, db->dbb_r_user,
					I2CONST_1, db->dbb_r_user, I2CONST_2);
			FTN_print_buffer(output_buffer);
		}
		if (db->dbb_r_password)
		{
			sprintf(output_buffer,
					"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_password, %s, %sLEN(%s)%s)\n",
					COLUMN,
					s2, s1, db->dbb_r_password,
					I2CONST_1, db->dbb_r_password, I2CONST_2);
			FTN_print_buffer(output_buffer);
		}

		// SQL Role supports GPRE/Fortran
		if (db->dbb_r_sql_role)
		{
			sprintf(output_buffer,
					"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_sql_role_name, %s, %sLEN(%s)%s)\n",
					COLUMN,
					s2, s1, db->dbb_r_sql_role,
					I2CONST_1, db->dbb_r_sql_role, I2CONST_2);
			FTN_print_buffer(output_buffer);
		}

		if (db->dbb_r_lc_messages)
		{
			sprintf(output_buffer,
					"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_lc_messages, %s, %sLEN(%s)%s)\n",
					COLUMN,
					s2, s1, db->dbb_r_lc_messages,
					I2CONST_1, db->dbb_r_lc_messages, I2CONST_2);
			FTN_print_buffer(output_buffer);
		}
		if (db->dbb_r_lc_ctype)
		{
			sprintf(output_buffer,
					"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_lc_type, %s, %sLEN(%s)%s)\n",
					COLUMN,
					s2, s1, db->dbb_r_lc_ctype,
					I2CONST_1, db->dbb_r_lc_ctype, I2CONST_2);
			FTN_print_buffer(output_buffer);
		}
	}
	else
		sprintf(s2, "isc_%d", request->req_ident);

	if (request->req_length || request->req_flags & REQ_extend_dpb)
		sprintf(output_buffer,
				"%sCALL ISC_CREATE_DATABASE (%s, %s%"SIZEFORMAT"%s, %s'%s'%s, %s, %s%s%s, %s, 0)\n",
				COLUMN,
				status_vector(),
				I2CONST_1, strlen(db->dbb_filename), I2CONST_2,
				REF_1, db->dbb_filename, REF_2,
				db->dbb_name->sym_string, I2CONST_1, s1, I2CONST_2, s2);
	else
		sprintf(output_buffer, "%sCALL ISC_CREATE_DATABASE (%s, %s%"SIZEFORMAT"%s, %s'%s'%s, %s, %s0%s, 0, 0)\n",
				COLUMN,
				status_vector(),
				I2CONST_1, strlen(db->dbb_filename), I2CONST_2,
				REF_1, db->dbb_filename, REF_2,
				db->dbb_name->sym_string, I2CONST_1, I2CONST_2);
	FTN_print_buffer(output_buffer);
	if (request && request->req_flags & REQ_extend_dpb)
	{
		if (request->req_length)
		{
			sprintf(output_buffer, "%sif (%s != isc_%d)\n", COLUMN, s2, request->req_ident);
			FTN_print_buffer(output_buffer);
		}
		sprintf(output_buffer, "%sCALL ISC_FREE (%s)\n", COLUMN, s2);
		FTN_print_buffer(output_buffer);

		// reset the length of the dpb

		sprintf(output_buffer, "%s%s = %d\n", COLUMN, s1, request->req_length);
		FTN_print_buffer(output_buffer);
	}

	const bool save_sw_auto = gpreGlob.sw_auto;
	gpreGlob.sw_auto = true;
	printa(COLUMN, "IF (isc_status(2) .eq. 0) THEN");
	gen_ddl(action);
	printa(COLUMN, "END IF");
	gpreGlob.sw_auto = save_sw_auto;
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static void gen_cursor_close(const gpre_req* request)
{
	printa(COLUMN, "IF (isc_%ds .NE. 0) THEN", request->req_ident);
	printa(COLUMN, "CALL %s (%s, isc_%ds, %s1%s)",
		   ISC_DSQL_FREE,
		   status_vector(),
		   request->req_ident, DSQL_I2CONST_1, DSQL_I2CONST_2);
	printa(COLUMN, "IF (isc_status(2) .EQ. 0) THEN");
}


//____________________________________________________________
//
//		Generate text to initialize a cursor.
//

static void gen_cursor_init(const act* action)
{
	// If blobs are present, zero out all of the blob handles.  After this
	// point, the handles are the user's responsibility

	if (action->act_request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
	{
		printa(COLUMN, "isc_%d = 0", action->act_request->req_blobs->blb_ident);
	}
}


//____________________________________________________________
//
//		Generate text to open an embedded SQL cursor.
//

static void gen_cursor_open(const act* action, const gpre_req* request)
{
	TEXT s[MAX_CURSOR_SIZE];

	if (action->act_type != ACT_open)
	{
		if (gpreGlob.sw_auto)
			printa(COLUMN, "IF (isc_%ds .EQ. 0 .AND. %s .NE. 0) THEN",
				   request->req_ident,
				   request->req_database->dbb_name->sym_string);
		else
			printa(COLUMN, "IF (isc_%ds .EQ. 0) THEN", request->req_ident);
	}
	else
	{
		if (gpreGlob.sw_auto)
			printa(COLUMN, "IF (isc_%ds .EQ. 0 .AND. %s .NE. 0 .AND. %s .NE. 0) THEN",
				   request->req_ident, request->req_handle,
				   request->req_database->dbb_name->sym_string);
		else
			printa(COLUMN, "IF (isc_%ds .EQ. 0 .AND. %s .NE. 0) THEN",
				   request->req_ident, request->req_handle);
	}

	printa(COLUMN, "CALL %s (%s, %s, isc_%ds)",
		   ISC_DSQL_ALLOCATE,
		   status_vector(),
		   request->req_database->dbb_name->sym_string, request->req_ident);
	printa(COLUMN, "END IF");

	if (gpreGlob.sw_auto)
		printa(COLUMN, "IF (isc_%ds .NE. 0 .AND. %s .NE. 0) THEN",
			   request->req_ident, request_trans(action, request));
	else
		printa(COLUMN, "IF (isc_%ds .NE. 0) THEN", request->req_ident);
	printa(COLUMN, "CALL %s (%s, isc_%ds, %s, %s0%s)",
		   ISC_DSQL_SET_CURSOR,
		   status_vector(),
		   request->req_ident,
		   make_name(s, ((open_cursor*) action->act_object)->opn_cursor),
		   DSQL_I2CONST_1, DSQL_I2CONST_2);
	printa(COLUMN, "IF (isc_status(2) .EQ. 0) THEN");
	printa(COLUMN, "CALL %s (%s, %s, isc_%ds, %s0%s, 0, %s-1%s, %s0%s, 0)",
		   ISC_DSQL_EXECUTE,
		   status_vector(),
		   request_trans(action, request),
		   request->req_ident,
		   DSQL_I2CONST_1, DSQL_I2CONST_2,
		   DSQL_I2CONST_1, DSQL_I2CONST_2, DSQL_I2CONST_1, DSQL_I2CONST_2);
	printa(COLUMN, "IF (isc_status(2) .EQ. 0) THEN");
}


//____________________________________________________________
//
//		Generate insertion text for the database statement.
//

static void gen_database()
{
	if (global_first_flag)
		return;

	global_first_flag = true;

	sprintf(output_buffer, "\n%s      **** GDS Preprocessor Definitions ****\n\n", COMMENT);
	FTN_print_buffer(output_buffer);

	gen_database_decls(); //(action);
	gen_database_data(); //(action);

	printa(COMMENT, "**** end of GPRE definitions ****\n");
}


//____________________________________________________________
//
//		Generate insertion text for global DATA statements.
//

static void gen_database_data() //(const act* action)
{
	Firebird::PathName include_buffer;

	include_buffer = fb_utils::getPrefix(Firebird::DirType::FB_DIR_INC, INCLUDE_FTN_FILE);
	sprintf(output_buffer, INCLUDE_ISC_FTN, include_buffer.c_str());

	FTN_print_buffer(output_buffer);

	bool any_extern = false;
	for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
#ifndef FTN_BLK_DATA
		if (db->dbb_scope != DBB_EXTERN)
			fprintf(gpreGlob.out_file, "%sDATA %s /0/               %s{ init database handle }\n",
					COLUMN, db->dbb_name->sym_string, INLINE_COMMENT);
		else
			any_extern = true;
#endif
		for (const tpb* tpb_iterator = db->dbb_tpbs;
			tpb_iterator;
			tpb_iterator = tpb_iterator->tpb_dbb_next)
		{
			gen_tpb_data(tpb_iterator);
		}
	}

	fprintf(gpreGlob.out_file, "%sDATA ISC_NULL /0/            %s{ init null vector }\n",
			   COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sDATA ISC_BLOB_NULL /0,0/     %s{ init null blob }\n",
			   COLUMN, INLINE_COMMENT);
#ifndef FTN_BLK_DATA
	if (!any_extern)
		fprintf(gpreGlob.out_file, "%sDATA GDS__TRANS /0/           %s{ init trans handle }\n",
				   COLUMN, INLINE_COMMENT);
#endif

	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
		gen_request_data(request);
}


//____________________________________________________________
//
//		Generate insertion text for global
//       data declarations.
//

static void gen_database_decls() //const act* action)
{
	fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_BLOB_NULL(2)  %s{ null blob handle }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  GDS__TRANS         %s{ default transaction handle }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_STATUS(20)    %s{ status vector }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_STATUS2(20)   %s{ status vector }\n",
			COLUMN, INLINE_COMMENT);

	// added for 3.3 compatibility
	fprintf(gpreGlob.out_file, "%sINTEGER*4  GDS__STATUS(20)    %s{ status vector }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  GDS__STATUS2(20)   %s{ status vector }\n",
			COLUMN, INLINE_COMMENT);

	printa(COLUMN, "EQUIVALENCE    (ISC_STATUS(20), GDS__STATUS(20)) ");
	printa(COLUMN, "EQUIVALENCE    (ISC_STATUS2(20), GDS__STATUS2(20)) ");
	// end of code added for 3.3 compatibility

	fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_NULL          %s{ dummy status vector }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  SQLCODE            %s{ SQL status code }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_SQLCODE       %s{ SQL status code translator }\n",
			COLUMN, INLINE_COMMENT);
	fprintf(gpreGlob.out_file,
			"%sINTEGER*4  ISC_ARRAY_LENGTH  %s{ array return size }\n",
			COLUMN, INLINE_COMMENT);

	bool all_static = true;
	bool dcl_ndx_var = false;

	SSHORT count = 0;
	for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		all_static = all_static && (db->dbb_scope == DBB_STATIC);
		const TEXT* name = db->dbb_name->sym_string;
		fprintf(gpreGlob.out_file, "%sINTEGER*4  %s                %s{ database handle }\n",
				COLUMN, name, INLINE_COMMENT);

		fprintf(gpreGlob.out_file, "%sCHARACTER*256 ISC_%s        %s{ database file name }\n",
				COLUMN, name, INLINE_COMMENT);

		for (const tpb* tpb_iterator = db->dbb_tpbs;
			 tpb_iterator;
			 tpb_iterator = tpb_iterator->tpb_dbb_next)
		{
			gen_tpb_decls(tpb_iterator);
			dcl_ndx_var = true;
		}

#ifdef HPUX
		// build fields to handle start_multiple

		count++;
		fprintf(gpreGlob.out_file, "%sINTEGER*4      ISC_TEB%d_DBB   %s( vector db handle )\n",
				COLUMN, count, INLINE_COMMENT);
		fprintf(gpreGlob.out_file, "%sINTEGER*4      ISC_TEB%d_LEN   %s( vector tpb length )\n",
				COLUMN, count, INLINE_COMMENT);
		fprintf(gpreGlob.out_file, "%sINTEGER*4      ISC_TEB%d_TPB   %s( vector tpb handle )\n",
				COLUMN, count, INLINE_COMMENT);
#endif

		printa(COLUMN, "COMMON /%s/ %s", name, name);
	}

#ifdef HPUX
	// declare array and set up equivalence for start_multiple vector

	const SSHORT length = 12;
	fprintf(gpreGlob.out_file, "%sCHARACTER      ISC_TEB(%d)  %s( transaction vector )\n",
			COLUMN, length * count, INLINE_COMMENT);
	for (SSHORT i = 0; i < count;)
	{
		const SSHORT index = i++ * length + 1;
		printa(COLUMN, "EQUIVALENCE    (ISC_TEB(%d), ISC_TEB%d_DBB )", index, i);
		printa(COLUMN, "EQUIVALENCE    (ISC_TEB(%d), ISC_TEB%d_LEN )", index + 4, i);
		printa(COLUMN, "EQUIVALENCE    (ISC_TEB(%d), ISC_TEB%d_TPB )", index + 8, i);
	}
#endif

	if (!all_static)
	{
		printa(COLUMN, "COMMON /GDS__TRANS/GDS__TRANS");
		printa(COLUMN, "COMMON /ISC_STATUS/ISC_STATUS");
		printa(COLUMN, "COMMON /ISC_STATUS2/ISC_STATUS2");
		printa(COLUMN, "COMMON /SQLCODE/SQLCODE");
	}

	array_decl_list = NULL;
	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
	{
		gen_request_decls(request);
		const gpre_port* port;
		for (port = request->req_ports; port; port = port->por_next)
			make_port(port);
		for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			fprintf(gpreGlob.out_file, "%sINTEGER*4 isc_%d         %s{ blob handle }\n",
					COLUMN, blob->blb_ident, INLINE_COMMENT);
			fprintf(gpreGlob.out_file, "%sCHARACTER*%d isc_%d      %s{ blob segment }\n",
					COLUMN, blob->blb_seg_length, blob->blb_buff_ident, INLINE_COMMENT);
			fprintf(gpreGlob.out_file, "%sINTEGER*2 isc_%d         %s{ segment length }\n",
					COLUMN, blob->blb_len_ident, INLINE_COMMENT);
		}

		// Array declarations

		if (port = request->req_primary)
			for (const ref* reference = port->por_references; reference;
				reference = reference->ref_next)
			{
				if (reference->ref_field->fld_array_info)
					make_array_declaration(reference);
			}
	}

	// Declare DATA statement index variable

	if (dcl_ndx_var || gpreGlob.requests)
		printa(COLUMN, "INTEGER ISC_I");

	// generate event parameter block for each event in module

	SSHORT max_count = 0;
	for (gpre_lls* stack_ptr = gpreGlob.events; stack_ptr; stack_ptr = stack_ptr->lls_next)
	{
		count = gen_event_block((const act*) stack_ptr->lls_object);
		max_count = MAX(count, max_count);
	}

	if (max_count)
	{
		fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_EVENTS(%d)         %s{ event vector }\n",
				COLUMN, max_count, INLINE_COMMENT);
		fprintf(gpreGlob.out_file, "%sINTEGER*4  ISC_EVENT_NAMES(%d)    %s{ event buffer }\n",
				COLUMN, max_count, INLINE_COMMENT);
		fprintf(gpreGlob.out_file, "%sCHARACTER*31 ISC_EVENT_NAMES2(%d) %s{ event string buffer }\n",
				COLUMN, max_count, INLINE_COMMENT);
	}
}


//____________________________________________________________
//
//		Generate a call to update metadata.
//

static void gen_ddl(const act* action)
{
	if (gpreGlob.sw_auto)
	{
		t_start_auto(0, status_vector(), action, true);
		printa(COLUMN, "if (gds__trans .ne. 0) then");
	}

	// Set up command type for call to RDB_DDL

	const gpre_req* request = action->act_request;

	sprintf(output_buffer, "%sCALL isc_ddl (%s, %s, gds__trans, %s%d%s, isc_%d)\n", COLUMN,
			status_vector(),
			request->req_database->dbb_name->sym_string, I2CONST_1,
			request->req_length, I2CONST_2, request->req_ident);

	FTN_print_buffer(output_buffer);

	if (gpreGlob.sw_auto)
	{
		printa(COLUMN, "END IF");
		printa(COLUMN, "if (isc_status(2) .eq. 0)");
		printa(CONTINUE, "CALL isc_commit_transaction (%s, gds__trans)", status_vector());
		printa(COLUMN, "if (isc_status(2) .ne. 0)");
		printa(CONTINUE, "CALL isc_rollback_transaction (isc_status2, gds__trans)");
	}

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a call to create a database.
//

static void gen_drop_database(const act* action)
{
	const gpre_dbb* db = (gpre_dbb*) action->act_object;

	sprintf(output_buffer, "%s CALL ISC_DROP_DATABASE (%s, %s%"SIZEFORMAT"%s, %s\'%s\'%s, RDB_K_DB_TYPE_GDS)\n",
			COLUMN,
			status_vector(),
			I2_1, strlen(db->dbb_filename), I2_2,
			REF_1, db->dbb_filename, REF_2);
	FTN_print_buffer(output_buffer);
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_close(const act* action)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (const dyn*) action->act_object;
	printa(COLUMN, "CALL %s (isc_status, %s)",
		   ISC_EMBED_DSQL_CLOSE, make_name(s, statement->dyn_cursor_name));
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_declare(const act* action)
{
	TEXT s1[MAX_CURSOR_SIZE], s2[MAX_CURSOR_SIZE];

	const dyn* statement = (const dyn*) action->act_object;
	printa(COLUMN, "CALL %s (isc_status, %s, %s)",
		   ISC_EMBED_DSQL_DECLARE,
		   make_name(s1, statement->dyn_statement_name),
		   make_name(s2, statement->dyn_cursor_name));
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_describe(const act* action, bool bind_flag)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (const dyn*) action->act_object;
	printa(COLUMN, "CALL %s (isc_status, %s, %s%d%s, %s)",
		   bind_flag ? ISC_EMBED_DSQL_DESCRIBE_BIND : ISC_EMBED_DSQL_DESCRIBE,
		   make_name(s, statement->dyn_statement_name),
		   DSQL_I2CONST_1, gpreGlob.sw_sql_dialect, DSQL_I2CONST_2,
		   statement->dyn_sqlda);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_execute(const act* action)
{
	const TEXT* transaction;
	gpre_req* request;
	gpre_req req_const;

	const dyn* statement = (const dyn*) action->act_object;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds__trans";
		request = NULL;
	}

	if (gpreGlob.sw_auto)
	{
		t_start_auto(request, status_vector(), action, true);
		printa(COLUMN, "if (%s .ne. 0) then", transaction);
	}

	const TEXT* sqlda = statement->dyn_sqlda;
	const TEXT* sqlda2 = statement->dyn_sqlda2;
#ifdef HPUX
	TEXT s2[64], s3[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
	if (sqlda2)
	{
		sprintf(s3, "isc_baddress (%s)", sqlda2);
		sqlda2 = s3;
	}
#endif

	TEXT s1[MAX_CURSOR_SIZE];
	printa(COLUMN,
		   sqlda2 ?
				"CALL %s (isc_status, %s, %s, %s%d%s, %s, %s)" :
				"CALL %s (isc_status, %s, %s, %s%d%s, %s)",
		   sqlda2 ? ISC_EMBED_DSQL_EXECUTE2 : ISC_EMBED_DSQL_EXECUTE,
		   transaction,
		   make_name(s1, statement->dyn_statement_name),
		   DSQL_I2CONST_1, gpreGlob.sw_sql_dialect, DSQL_I2CONST_2,
		   sqlda ? sqlda : NULL_SQLDA, sqlda2 ? sqlda2 : NULL_SQLDA);

	if (gpreGlob.sw_auto)
		printa(COLUMN, "END IF");

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_fetch(const act* action)
{
	const dyn* statement = (const dyn*) action->act_object;

	const TEXT* sqlda = statement->dyn_sqlda;
#ifdef HPUX
	TEXT s2[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
#endif

	TEXT s1[MAX_CURSOR_SIZE];
	printa(COLUMN, "SQLCODE = %s (isc_status, %s, %s%d%s, %s)",
		   ISC_EMBED_DSQL_FETCH,
		   make_name(s1, statement->dyn_cursor_name),
		   DSQL_I2CONST_1, gpreGlob.sw_sql_dialect, DSQL_I2CONST_2,
		   sqlda ? sqlda : NULL_SQLDA);

	printa(COLUMN, "IF (SQLCODE .NE. 100) SQLCODE = ISC_SQLCODE (isc_status)");
}


//____________________________________________________________
//
//		Generate code for an EXECUTE IMMEDIATE dynamic SQL statement.
//

static void gen_dyn_immediate(const act* action)
{
	const TEXT* transaction;
	gpre_req* request;
	gpre_req req_const;

	const dyn* statement = (const dyn*) action->act_object;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds__trans";
		request = NULL;
	}

	const gpre_dbb* database = statement->dyn_database;

	if (gpreGlob.sw_auto)
	{
		t_start_auto(request, status_vector(), action, true);
		printa(COLUMN, "if (%s .ne. 0) then", transaction);
	}

	const TEXT* sqlda = statement->dyn_sqlda;
	const TEXT* sqlda2 = statement->dyn_sqlda2;
#ifdef HPUX
	TEXT s2[64], s3[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
	if (sqlda2)
	{
		sprintf(s3, "isc_baddress (%s)", sqlda2);
		sqlda2 = s3;
	}
#endif

	printa(COLUMN,
		   sqlda2 ?
				"CALL %s (isc_status, %s, %s, %sLEN(%s)%s, %s%s%s, %s%d%s, %s, %s)" :
				"CALL %s (isc_status, %s, %s, %sLEN(%s)%s, %s%s%s, %s%d%s, %s)",
		   sqlda2 ? ISC_EMBED_DSQL_EXECUTE_IMMEDIATE2 : ISC_EMBED_DSQL_EXECUTE_IMMEDIATE,
		   transaction,
		   database->dbb_name->sym_string, DSQL_I2CONST_1,
		   statement->dyn_string, DSQL_I2CONST_2, REF_1,
		   statement->dyn_string, REF_2, DSQL_I2CONST_1, gpreGlob.sw_sql_dialect,
		   DSQL_I2CONST_2, sqlda ? sqlda : NULL_SQLDA,
		   sqlda2 ? sqlda2 : NULL_SQLDA);

	if (gpreGlob.sw_auto)
		printa(COLUMN, "END IF");

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_insert(const act* action)
{
	const dyn* statement = (const dyn*) action->act_object;

	const TEXT* sqlda = statement->dyn_sqlda;
#ifdef HPUX
	TEXT s2[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
#endif

	TEXT s1[MAX_CURSOR_SIZE];
	printa(COLUMN, "%s (isc_status, %s, %s%d%s, %s)",
		   ISC_EMBED_DSQL_INSERT,
		   make_name(s1, statement->dyn_cursor_name),
		   DSQL_I2CONST_1, gpreGlob.sw_sql_dialect, DSQL_I2CONST_2,
		   sqlda ? sqlda : NULL_SQLDA);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_open(const act* action)
{
	const TEXT* transaction;
	gpre_req* request;
	gpre_req req_const;

	const dyn* statement = (const dyn*) action->act_object;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds__trans";
		request = NULL;
	}

	if (gpreGlob.sw_auto)
	{
		t_start_auto(request, status_vector(), action, true);
		printa(COLUMN, "if (%s .ne. 0) then", transaction);
	}

	const TEXT* sqlda = statement->dyn_sqlda;
	const TEXT* sqlda2 = statement->dyn_sqlda2;
#ifdef HPUX
	TEXT s2[64], s3[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
	if (sqlda2)
	{
		sprintf(s3, "isc_baddress (%s)", sqlda2);
		sqlda2 = s3;
	}
#endif

	TEXT s1[MAX_CURSOR_SIZE];
	printa(COLUMN,
		   sqlda2 ?
				"CALL %s (isc_status, %s, %s, %s%d%s, %s, %s)" :
				"CALL %s (isc_status, %s, %s, %s%d%s, %s)",
		   sqlda2 ? ISC_EMBED_DSQL_OPEN2 : ISC_EMBED_DSQL_OPEN,
		   transaction,
		   make_name(s1, statement->dyn_cursor_name),
		   DSQL_I2CONST_1, gpreGlob.sw_sql_dialect, DSQL_I2CONST_2,
		   sqlda ? sqlda : NULL_SQLDA, sqlda2 ? sqlda2 : NULL_SQLDA);

	if (gpreGlob.sw_auto)
		printa(COLUMN, "END IF");

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_prepare(const act* action)
{
	const TEXT* transaction;
	gpre_req* request;
	gpre_req req_const;

	const dyn* statement = (const dyn*) action->act_object;
	const gpre_dbb* database = statement->dyn_database;

	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds__trans";
		request = NULL;
	}

	if (gpreGlob.sw_auto)
	{
		t_start_auto(request, status_vector(), action, true);
		printa(COLUMN, "if (%s .ne. 0) then", transaction);
	}

	const TEXT* sqlda = statement->dyn_sqlda;
#ifdef HPUX
	TEXT s2[64];
	if (sqlda)
	{
		sprintf(s2, "isc_baddress (%s)", sqlda);
		sqlda = s2;
	}
#endif

	TEXT s1[MAX_CURSOR_SIZE];
	printa(COLUMN, "CALL %s (isc_status, %s, %s, %s, %sLEN(%s)%s, %s%s%s, %s%d%s, %s)",
		   ISC_EMBED_DSQL_PREPARE, database->dbb_name->sym_string,
		   transaction, make_name(s1, statement->dyn_statement_name),
		   DSQL_I2CONST_1, statement->dyn_string, DSQL_I2CONST_2, REF_1,
		   statement->dyn_string, REF_2, DSQL_I2CONST_1, gpreGlob.sw_sql_dialect,
		   DSQL_I2CONST_2, sqlda ? sqlda : NULL_SQLDA);

	if (gpreGlob.sw_auto)
		printa(COLUMN, "END IF");

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate substitution text for END_MODIFY.
//

static void gen_emodify(const act* action)
{
	SCHAR s1[MAX_REF_SIZE], s2[MAX_REF_SIZE];

	const upd* modify = (const upd*) action->act_object;

	for (const ref* reference = modify->upd_port->por_references; reference;
		reference = reference->ref_next)
	{
		const ref* source = reference->ref_source;
		if (!source)
			continue;
		const gpre_fld* field = reference->ref_field;
		gen_name(s1, source, true);
		gen_name(s2, reference, true);
		if (field->fld_dtype == dtype_blob || field->fld_dtype == dtype_quad ||
			field->fld_dtype == dtype_date)
		{
			sprintf(output_buffer, "%sCALL isc_qtoq (%s, %s)\n", COLUMN, s1, s2);
		}
		else
			sprintf(output_buffer, "%s%s = %s\n", COLUMN, s2, s1);
		FTN_print_buffer(output_buffer);
		if (field->fld_array_info)
			gen_get_or_put_slice(action, reference, false);
	}

	gen_send(action, modify->upd_port);
}


//____________________________________________________________
//
//		Generate substitution text for END_STORE.
//

static void gen_estore(const act* action)
{
	const gpre_req* request = action->act_request;

	// if this is a store...returning_values (aka store2)
	// we already executed the store, so go home quietly

	if (request->req_type == REQ_store2)
		return;

	gen_start(action, request->req_primary);
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate an END IF for the IF generated for
//		the AT_END clause.
//

static void gen_end_fetch()
{
	printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_endfor(const act* action)
{
	const gpre_req* request = action->act_request;

	if (request->req_sync)
		gen_send(action, request->req_sync);

	printa(COLUMN, "GOTO %d", request->req_top_label);
	printa("", "%-6dCONTINUE", request->req_btm_label);
}


//____________________________________________________________
//
//		Generate substitution text for ERASE.
//

static void gen_erase(const act* action)
{
	const upd* erase = (const upd*) action->act_object;
	gen_send(action, erase->upd_port);
}


//____________________________________________________________
//
//		Generate event parameter blocks for use
//		with a particular call to isc_event_wait.
//

static SSHORT gen_event_block(const act* action)
{
	gpre_nod* init = (gpre_nod*) action->act_object;

	int ident = CMP_next_ident();
	init->nod_arg[2] = (gpre_nod*)(IPTR) ident;

	printa(COLUMN, "INTEGER*4      isc_%dA", ident);
	printa(COLUMN, "INTEGER*4      isc_%dB", ident);
	printa(COLUMN, "INTEGER*2      isc_%dL", ident);

	gpre_nod* list = init->nod_arg[1];

	return list->nod_count;
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_INIT.
//

static void gen_event_init(const act* action)
{
#if (!defined AIX && !defined AIX_PPC)
	const TEXT* pattern1 =
		"ISC_%N1L = ISC_EVENT_BLOCK_A (%RFISC_%N1A%RE, %RFISC_%N1B%RE, %VF%S3%N2%S4%VE, %RFISC_EVENT_NAMES%RE)";
#else
	const TEXT* pattern1 =
		"CALL ISC_EVENT_BLOCK_S (%RFISC_%N1A%RE, %RFISC_%N1B%RE, %VF%S3%N2%S4%VE, %RFISC_EVENT_NAMES%RE, %RFISC_%N1L%RE)";
#endif
	const TEXT* pattern2 =
		"CALL %S1 (%V1, %RF%DH%RE, %VFISC_%N1L%VE, %VFISC_%N1A%VE, %VFISC_%N1B%VE)";
	const TEXT* pattern3 =
		"CALL %S2 (ISC_EVENTS, %VFISC_%N1L%VE, %VFISC_%N1A%VE, %VFISC_%N1B%VE)";

	gpre_nod* init = (gpre_nod*) action->act_object;
	gpre_nod* event_list = init->nod_arg[1];

	PAT args;
	args.pat_database = (gpre_dbb*) init->nod_arg[3];
	args.pat_vector1 = status_vector();
	args.pat_value1 = (int)(IPTR) init->nod_arg[2];
	args.pat_value2 = (int) event_list->nod_count;
	args.pat_string1 = ISC_EVENT_WAIT;
	args.pat_string2 = ISC_EVENT_COUNTS;
	args.pat_string3 = I2_1;
	args.pat_string4 = I2_2;

	// generate call to dynamically generate event blocks

	TEXT variable[MAX_REF_SIZE];
	SSHORT count = 0;
	gpre_nod** ptr = event_list->nod_arg;
	for (gpre_nod** const end = ptr + event_list->nod_count; ptr < end; ptr++)
	{
		count++;
		gpre_nod* node = *ptr;
		if (node->nod_type == nod_field)
		{
			const ref* reference = (const ref*) node->nod_arg[0];
			gen_name(variable, reference, true);
			printa(COLUMN, "ISC_EVENT_NAMES2(%d) = %s", count, variable);
		}
		else
			printa(COLUMN, "ISC_EVENT_NAMES2(%d) = %s", count, node->nod_arg[0]);

#if (!defined AIX && !defined AIX_PPC)
		printa(COLUMN, "ISC_EVENT_NAMES(%d) = ISC_BADDRESS (%sISC_EVENT_NAMES2(%d)%s)",
			   count, REF_1, count, REF_2);
#else
		printa(COLUMN, "CALL ISC_BADDRESS (%sISC_EVENT_NAMES2(%d)%s, ISC_EVENT_NAMES(%d))",
			   REF_1, count, REF_2, count);
#endif
	}

	const SSHORT column = 6;

	PATTERN_expand(column, pattern1, &args);

	// generate actual call to event_wait

	PATTERN_expand(column, pattern2, &args);

	// get change in event counts, copying event parameter block for reuse

	PATTERN_expand(column, pattern3, &args);
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_WAIT.
//

static void gen_event_wait(const act* action)
{
	const TEXT* pattern1 =
		"CALL %S1 (%V1, %RF%DH%RE, %VFISC_%N1L%VE, %VFISC_%N1A%VE, %VFISC_%N1B%VE)";
	const TEXT* pattern2 =
		"CALL %S2 (ISC_EVENTS, %VFISC_%N1L%VE, %VFISC_%N1A%VE, %VFISC_%N1B%VE)";

	const gpre_sym* event_name = (const gpre_sym*) action->act_object;

	// go through the stack of gpreGlob.events, checking to see if the
	// event has been initialized and getting the event identifier

	int ident = -1;
	const gpre_dbb* database =  NULL;
	for (gpre_lls* stack_ptr = gpreGlob.events; stack_ptr; stack_ptr = stack_ptr->lls_next)
	{
		const act* event_action = (const act*) stack_ptr->lls_object;
		const gpre_nod* event_init = (gpre_nod*) event_action->act_object;
		const gpre_sym* stack_name = (const gpre_sym*) event_init->nod_arg[0];
		if (!strcmp(event_name->sym_string, stack_name->sym_string))
		{
			ident = (int)(IPTR) event_init->nod_arg[2];
			database = (gpre_dbb*) event_init->nod_arg[3];
		}
	}

	if (ident < 0)
	{
		TEXT s[64];
		sprintf(s, "event handle \"%s\" not found", event_name->sym_string);
		CPR_error(s);
        return;
	}

	PAT args;
	args.pat_database = database;
	args.pat_vector1 = status_vector();
	args.pat_value1 = ident;
	args.pat_string1 = ISC_EVENT_WAIT;
	args.pat_string2 = ISC_EVENT_COUNTS;

	// generate calls to wait on the event and to fill out the gpreGlob.events array

	const SSHORT column = 6;

	PATTERN_expand(column, pattern1, &args);
	PATTERN_expand(column, pattern2, &args);
	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate replacement text for the SQL FETCH statement.  The
//		epilog FETCH statement is handled by GEN_S_FETCH (generate
//		stream fetch).
//

static void gen_fetch(const act* action)
{
	SCHAR s[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	if (request->req_sync)
	{
		gen_send(action, request->req_sync);
		printa(COLUMN, "IF (SQLCODE .EQ. 0) THEN");
	}

	gen_receive(action, request->req_primary);
	printa(COLUMN, "IF (%s .NE. 0) THEN", gen_name(s, request->req_eof, true));
	printa(COLUMN, "SQLCODE = 0");

	gpre_nod* var_list = (gpre_nod*) action->act_object;
	if (var_list)
	{
		for (int i = 0; i < var_list->nod_count; i++) {
			asgn_to(action, (const ref*) var_list->nod_arg[i]);
		}
	}
	printa(COLUMN, "ELSE");
	printa(COLUMN, "SQLCODE = 100");
	printa(COLUMN, "END IF");

	if (request->req_sync)
		printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate substitution text for FINISH
//

static void gen_finish(const act* action)
{
	const gpre_dbb* db = NULL;

	if (gpreGlob.sw_auto || ((action->act_flags & ACT_sql) && action->act_type != ACT_disconnect))
	{
		printa(COLUMN, "IF (GDS__TRANS .NE. 0) THEN");
		printa(COLUMN, "    CALL ISC_%s_TRANSACTION (%s, GDS__TRANS)",
			   (action->act_type != ACT_rfinish) ? "COMMIT" : "ROLLBACK",
			   status_vector());
		status_and_stop(action);
		printa(COLUMN, "END IF");
	}

	// the user supplied one or more db_handles

	for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
	{
		db = ready->rdy_database;
		printa(COLUMN, "IF (%s .NE. 0) THEN", db->dbb_name->sym_string);
		printa(COLUMN, "CALL ISC_DETACH_DATABASE (%s, %s)",
			   status_vector(), db->dbb_name->sym_string);
		status_and_stop(action);
		printa(COLUMN, "END IF");
	}

	if (!db)
		for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			if ((action->act_error || (action->act_flags & ACT_sql)) && db != gpreGlob.isc_databases)
			{
				printa(COLUMN, "IF (%s .NE. 0 .AND. ISC_STATUS(2) .EQ. 0) THEN",
					   db->dbb_name->sym_string);
			}
			else
				printa(COLUMN, "IF (%s .NE. 0) THEN", db->dbb_name->sym_string);
			printa(COLUMN, "CALL ISC_DETACH_DATABASE (%s, %s)",
				   status_vector(), db->dbb_name->sym_string);
			status_and_stop(action);
			printa(COLUMN, "END IF");
		}
}


//____________________________________________________________
//
//		Generate substitution text for FOR statement.
//

static void gen_for(const act* action)
{
	gen_s_start(action);
	gpre_req* request = action->act_request;
	request->req_top_label = next_label();
	request->req_btm_label = next_label();
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0) GOTO %d\n", request->req_btm_label);

	printa("", "%-6dCONTINUE", request->req_top_label);

	SCHAR s[MAX_REF_SIZE];
	gen_receive(action, request->req_primary);
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(COLUMN, "IF (%s .EQ. 0 .OR. ISC_STATUS(2) .NE. 0) GOTO %d\n",
			   gen_name(s, request->req_eof, true), request->req_btm_label);
	else
		printa(COLUMN, "IF (%s .EQ. 0) GOTO %d\n",
			   gen_name(s, request->req_eof, true), request->req_btm_label);

	const gpre_port* port = action->act_request->req_primary;
	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_flags & REF_fetch_array)
				gen_get_or_put_slice(action, reference, true);
		}
	}
}


//____________________________________________________________
//
//		Generate a call to isc_get_slice
//       or isc_put_slice for an array.
//

static void gen_get_or_put_slice(const act* action, const ref* reference, bool get)
{
	if (!(reference->ref_flags & REF_fetch_array))
		return;

	TEXT s[MAX_REF_SIZE];
	if (get)
	{
		if (action->act_flags & ACT_sql)
		{
			sprintf(output_buffer,
					"%sCALL ISC_GET_SLICE (%s, %s, %s, %s, %s%d%s, isc_%d, %s0%s, %s0%s, %s%"
					SLONGFORMAT"%s, %s, ISC_ARRAY_LENGTH)\n",
					COLUMN,
					status_vector(),
					action->act_request->req_database->dbb_name->sym_string,
					action->act_request->req_trans,
					gen_name(s, reference, true),
					I2CONST_1, reference->ref_sdl_length, I2CONST_2,
					reference->ref_sdl_ident,
					I2CONST_1, I2CONST_2,
					I2CONST_1, I2CONST_2,
					I4CONST_1, reference->ref_field->fld_array_info->ary_size,
					I4CONST_2, reference->ref_value);
		}
		else
		{
			sprintf(output_buffer,
					"%sCALL ISC_GET_SLICE (%s, %s, %s, %s, %s%d%s, isc_%d, %s0%s, %s0%s, %s%"
					SLONGFORMAT"%s, isc_%d, ISC_ARRAY_LENGTH)\n",
					COLUMN,
					status_vector(),
					action->act_request->req_database->dbb_name->sym_string,
					action->act_request->req_trans,
					gen_name(s, reference, true),
					I2CONST_1, reference->ref_sdl_length, I2CONST_2,
					reference->ref_sdl_ident,
					I2CONST_1, I2CONST_2,
					I2CONST_1, I2CONST_2,
					I4CONST_1, reference->ref_field->fld_array_info->ary_size,
					I4CONST_2,
					reference->ref_field->fld_array_info->ary_ident);
		}
	}
	else
	{
		if (action->act_flags & ACT_sql)
		{
			sprintf(output_buffer,
					"%sCALL ISC_PUT_SLICE (%s, %s, %s, %s, %s%d%s, isc_%d, %s0%s, %s0%s, %s%"
					SLONGFORMAT"%s, %s)\n",
					COLUMN,
					status_vector(),
					action->act_request->req_database->dbb_name->sym_string,
					action->act_request->req_trans,
					gen_name(s, reference, true),
					I2CONST_1, reference->ref_sdl_length, I2CONST_2,
					reference->ref_sdl_ident,
					I2CONST_1, I2CONST_2,
					I2CONST_1, I2CONST_2,
					I4CONST_1, reference->ref_field->fld_array_info->ary_size,
					I4CONST_2, reference->ref_value);
		}
		else
		{
			sprintf(output_buffer,
					"%sCALL ISC_PUT_SLICE (%s, %s, %s, %s, %s%d%s, isc_%d, %s0%s, %s0%s, %s%"
					SLONGFORMAT"%s, isc_%d)\n",
					COLUMN,
					status_vector(),
					action->act_request->req_database->dbb_name->sym_string,
					action->act_request->req_trans,
					gen_name(s, reference, true),
					I2CONST_1, reference->ref_sdl_length, I2CONST_2,
					reference->ref_sdl_ident,
					I2CONST_1, I2CONST_2,
					I2CONST_1, I2CONST_2,
					I4CONST_1, reference->ref_field->fld_array_info->ary_size,
					I4CONST_2,
					reference->ref_field->fld_array_info->ary_ident);
		}
	}

	FTN_print_buffer(output_buffer);
}


//____________________________________________________________
//
//		Generate the code to do a get segment.
//

static void gen_get_segment(const act* action)
{
	blb* blob;

	if (action->act_flags & ACT_sql)
		blob = (blb*) action->act_request->req_blobs;
	else
		blob = (blb*) action->act_object;

	sprintf(output_buffer,
			"%sISC_STATUS(2) = ISC_GET_SEGMENT (%s, isc_%d, isc_%d, %sLEN(isc_%d)%s, %sisc_%d%s)\n",
			COLUMN,
			status_vector(),
			blob->blb_ident,
			blob->blb_len_ident,
			I2CONST_1, blob->blb_buff_ident, I2CONST_2,
			REF_1, blob->blb_buff_ident, REF_2);

	FTN_print_buffer(output_buffer);

	if (action->act_flags & ACT_sql)
	{
		status_and_stop(action);
		printa(COLUMN, "IF (SQLCODE .EQ. 0 .OR. SQLCODE .EQ. 101) THEN");
		const ref* into = action->act_object;
		printa(COLUMN_INDENT, "%s = isc_%d", into->ref_value, blob->blb_buff_ident);
		if (into->ref_null_value)
			printa(COLUMN_INDENT, "%s = isc_%d", into->ref_null_value, blob->blb_len_ident);
		printa(COLUMN, "ENDIF");
	}
	else if (!action->act_error)
	{
		printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0 .AND. ISC_STATUS(2) .NE. ISC_SEGMENT");
		printa(CONTINUE, ".AND. ISC_STATUS(2) .NE. ISC_SEGSTR_EOF) THEN");
		printa(COLUMN, "    CALL ISC_PRINT_STATUS (ISC_STATUS)");
		printa(COLUMN, "    STOP");
		printa(COLUMN, "END IF");
	}
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_loop(const act* action)
{
	TEXT name[MAX_REF_SIZE];

	gen_s_start(action);
	const gpre_req* request = action->act_request;
	const gpre_port* port = request->req_primary;
	printa(COLUMN, "IF (SQLCODE .EQ. 0) THEN");
	gen_receive(action, port);
	gen_name(name, port->por_references, true);
	printa(COLUMN, "IF (SQLCODE .EQ. 0 .AND. %s .EQ. 0) ", name);
	printa(CONTINUE, "SQLCODE = 100");
	printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate a name for a reference.  Name is constructed from
//		port and parameter idents.
//

static TEXT* gen_name(SCHAR* const string, const ref* reference, bool as_blob)
{
	if (reference->ref_field->fld_array_info && !as_blob)
		fb_utils::snprintf(string, MAX_REF_SIZE, "isc_%d",
				reference->ref_field->fld_array_info->ary_ident);
	else
		fb_utils::snprintf(string, MAX_REF_SIZE, "isc_%d", reference->ref_ident);

	return string;
}


//____________________________________________________________
//
//		Generate a block to handle errors.
//

static void gen_on_error(const act* action)
{
	const act* err_action = (const act*) action->act_object;
	switch (err_action->act_type)
	{
	case ACT_get_segment:
	case ACT_put_segment:
	case ACT_endblob:
		printa(COLUMN,
				"IF (ISC_STATUS(2) .NE. 0 .AND. ISC_STATUS(2) .NE. ISC_SEGMENT .AND. ISC_STATUS(2) .NE. ISC_SEGSTR_EOF) THEN");
		break;
	default:
		printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0) THEN");
	}
}


//____________________________________________________________
//
//		Generate code for an EXECUTE PROCEDURE.
//

static void gen_procedure(const act* action)
{
	const gpre_req* request = action->act_request;
	const gpre_port* in_port = request->req_vport;
	const gpre_port* out_port = request->req_primary;

	gpre_dbb* database = request->req_database;
	PAT args;
	args.pat_database = database;
	args.pat_request = action->act_request;
	args.pat_vector1 = status_vector();
	args.pat_request = request;
	args.pat_port = in_port;
	args.pat_port2 = out_port;

	const TEXT* pattern;
	if (in_port && in_port->por_length)
		pattern =
			"CALL ISC_TRANSACT_REQUEST (%V1, %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RF%RI%RE, %VF%PL%VE, %RF%PI%RE, %VF%QL%VE, %RF%QI%RE)\n";
	else
		pattern =
			"CALL ISC_TRANSACT_REQUEST (%V1, %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RI, %VF0%VE, 0, %VF%QL%VE, %RF%QI%RE)\n";

	// Get database attach and transaction started

	if (gpreGlob.sw_auto)
		t_start_auto(0, status_vector(), action, true);

	// Move in input values

	asgn_from(action, request->req_values);

	// Execute the procedure

	const USHORT column = 6;

	PATTERN_expand(column, pattern, &args);

	status_and_stop(action);

	printa(COLUMN, "IF (SQLCODE .EQ. 0) THEN");

	// Move out output values

	asgn_to_proc(request->req_references);
	printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate the code to do a put segment.
//

static void gen_put_segment(const act* action)
{
	blb* blob;

	if (action->act_flags & ACT_sql)
	{
		blob = (blb*) action->act_request->req_blobs;
		const ref* from = action->act_object;
		printa(COLUMN, "isc_%d = %s", blob->blb_len_ident, from->ref_null_value);
		printa(COLUMN, "isc_%d = %s", blob->blb_buff_ident, from->ref_value);
	}
	else
		blob = (blb*) action->act_object;

	sprintf(output_buffer, "%sISC_STATUS(2) = ISC_PUT_SEGMENT (%s, isc_%d, %sisc_%d%s, %sisc_%d%s)\n",
			COLUMN,
			status_vector(),
			blob->blb_ident,
			VAL_1, blob->blb_len_ident, VAL_2,
			REF_1, blob->blb_buff_ident, REF_2);
	FTN_print_buffer(output_buffer);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate BLR in raw, numeric form.  Ughly but dense.
//

static void gen_raw(const UCHAR* blr, req_t request_type, int request_length, int begin_c, int end_c)
{
	union {
		UCHAR bytewise_blr[4];
		SLONG longword_blr;
	} blr_hunk;

	blr = blr + begin_c;
	int blr_length = end_c - begin_c + 1;

	TEXT buffer[80];
	TEXT* p = buffer;

	while (blr_length)
	{
	    blr_hunk.longword_blr = 0;
		for (UCHAR* c = blr_hunk.bytewise_blr; c < blr_hunk.bytewise_blr + sizeof(SLONG); c++)
		{
			if (--blr_length)
				*c = *blr++;
			else
			{
				if (request_type == REQ_slice)
					*c = isc_sdl_eoc;
				else if (request_type == REQ_ddl || request_type == REQ_create_database ||
					(request_length != end_c + 1))
				{
					*c = *blr++;
				}
				else
					*c = blr_eoc;
				break;
			}
		}
		if (blr_length)
			sprintf(p, "%"SLONGFORMAT",", blr_hunk.longword_blr);
		else
			sprintf(p, "%"SLONGFORMAT, blr_hunk.longword_blr);
		while (*p)
			p++;
		if (p - buffer > 50)
		{
			fprintf(gpreGlob.out_file, "%s%s\n", CONTINUE, buffer);
			p = buffer;
			*p = 0;
		}
	}

	fprintf(gpreGlob.out_file, "%s%s/\n", CONTINUE, buffer);
}


//____________________________________________________________
//
//		Generate substitution text for READY
//

static void gen_ready(const act* action)
{
	const TEXT* vector = status_vector();

	for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
	{
		const gpre_dbb* db = ready->rdy_database;
		const TEXT* filename = ready->rdy_filename;
		if (!filename)
			filename = db->dbb_runtime;
		if (action->act_error && (ready != (rdy*) action->act_object))
			printa(COLUMN, "IF (ISC_STATUS(2) .EQ. 0) THEN");
		make_ready(db, filename, vector, ready->rdy_request);
		status_and_stop(action);
		if (action->act_error && (ready != (rdy*) action->act_object))
			printa(COLUMN, "END IF");
	}
}


//____________________________________________________________
//
//		Generate a send or receive call for a port.
//

static void gen_receive(const act* action, const gpre_port* port)
{
	const gpre_req* request = action->act_request;

	sprintf(output_buffer, "%sCALL ISC_RECEIVE (%s, %s, %s%d%s, %s%d%s, %sisc_%d%s, %s%s%s)\n",
			COLUMN, status_vector(), request->req_handle, I2CONST_1,
			port->por_msg_number, I2CONST_2, I2CONST_1, port->por_length,
			I2CONST_2, REF_1, port->por_ident, REF_2, VAL_1,
			request->req_request_level, VAL_2);

	FTN_print_buffer(output_buffer);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate substitution text for RELEASE_REQUESTS
//       For active databases, call isc_release_request.
//       for all others, just zero the handle.  For the
//       release request calls, ignore error returns, which
//       are likely if the request was compiled on a database
//       which has been released and re-readied.  If there is
//       a serious error, it will be caught on the next statement.
//

static void gen_release(const act* action)
{
	const gpre_dbb* exp_db = (gpre_dbb*) action->act_object;

	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
	{
		gpre_dbb* db = request->req_database;
		if (exp_db && db != exp_db)
			continue;
		if (!(request->req_flags & REQ_exp_hand))
		{
			printa(COLUMN, "IF %s", db->dbb_name->sym_string);
			printa(CONTINUE, "CALL ISC_RELEASE_REQUEST (ISC_STATUS, %S)", request->req_handle);
			printa(COLUMN, "%s = 0", request->req_handle);
		}
	}
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_request_data( const gpre_req* request)
{
	// gpreGlob.requests are generated as raw BLR in longword chunks
	// because FORTRAN is a miserable excuse for a language
	// and doesn't allow byte value assignments to character
	// fields.

	if (!(request->req_flags & (REQ_exp_hand | REQ_sql_blob_open | REQ_sql_blob_create)) &&
		request->req_type != REQ_slice && request->req_type != REQ_procedure)
	{
		fprintf(gpreGlob.out_file, "%sDATA %s /0/               %s{ init request handle }\n\n",
				COLUMN, request->req_handle, INLINE_COMMENT);
	}

	if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
		fprintf(gpreGlob.out_file, "%sDATA isc_%dS /0/             %s{ init SQL statement handle }\n\n",
				COLUMN, request->req_ident, INLINE_COMMENT);

	if (request->req_flags & REQ_sql_cursor)
		fprintf(gpreGlob.out_file, "%sDATA isc_%dS /0/             %s{ init SQL statement handle }\n\n",
				COLUMN, request->req_ident, INLINE_COMMENT);

	// Changed termination test in for-loop from <= to < to fix bug#840.
	// We were generating data statements with bad bounds on the last data
	// statement if the data size was divisible by 75.  mao 4/3/89

	if ((request->req_type == REQ_ready) || (request->req_type == REQ_create_database))
	{
		if (request->req_length || request->req_flags & REQ_extend_dpb)
		{
			fprintf(gpreGlob.out_file, "%sDATA isc_%dl /%d/               %s{ request length }\n\n",
					COLUMN, request->req_ident, request->req_length, INLINE_COMMENT);
		}
	}

	if (request->req_length)
	{
		for (int begin_i = 0; begin_i < request->req_length; begin_i = begin_i + (75 * sizeof(SLONG)))
		{
			const int end_i = MIN(request->req_length - 1, begin_i + (SLONG)(75 * sizeof(SLONG)) - 1);
			printa(COLUMN, "DATA (isc_%d(ISC_I)%s ISC_I=%d,%d)  /",
				   request->req_ident, COMMA, (begin_i / sizeof(SLONG)) + 1,
				   (end_i / sizeof(SLONG)) + 1);
			gen_raw(request->req_blr, request->req_type, request->req_length, begin_i, end_i);
		}

		const TEXT* string_type;
		if (!gpreGlob.sw_raw)
		{
			printa(COMMENT, " ");
			printa(COMMENT, "FORMATTED REQUEST BLR FOR isc_%d = ", request->req_ident);
			switch (request->req_type)
			{
			case REQ_create_database:
			case REQ_ready:
				string_type = "DPB";
				if (PRETTY_print_cdb(request->req_blr, gen_blr, 0, 0))
					CPR_error("internal error during parameter generation");
				break;

			case REQ_ddl:
				string_type = "DYN";
				if (PRETTY_print_dyn(request->req_blr, gen_blr, 0, 0))
					CPR_error("internal error during dynamic DDL generation");
				break;
			case REQ_slice:
				string_type = "SDL";
				if (PRETTY_print_sdl(request->req_blr, gen_blr, 0, 0))
					CPR_error("internal error during SDL generation");
				break;

			default:
				string_type = "BLR";
				if (fb_print_blr(request->req_blr, request->req_length, gen_blr, 0, 0))
					CPR_error("internal error during BLR generation");
			}
		}
		else
		{
			switch (request->req_type)
			{
			case REQ_create_database:
			case REQ_ready:
				string_type = "DPB";
				break;

			case REQ_ddl:
				string_type = "DYN";
				break;
			case REQ_slice:
				string_type = "SDL";
				break;

			default:
				string_type = "BLR";
			}
		}
		printa(COMMENT, " ");
		printa(COMMENT, "END OF %s STRING FOR REQUEST isc_%d\n", string_type, request->req_ident);
	}

	// Print out slice description language if there are arrays associated with request

	for (const gpre_port* port = request->req_ports; port; port = port->por_next)
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_sdl)
			{
				for (int begin_i = 0; begin_i < reference->ref_sdl_length;
					begin_i = begin_i + (75 * sizeof(SLONG)))
				{
					const int end_i = MIN(reference->ref_sdl_length - 1,
								begin_i + (SLONG)(75 * sizeof(SLONG)) - 1);
					printa(COLUMN, "DATA (isc_%d(ISC_I)%s ISC_I=%d,%d)  /",
						   reference->ref_sdl_ident, COMMA,
						   (begin_i / sizeof(SLONG)) + 1,
						   (end_i / sizeof(SLONG)) + 1);
					gen_raw(reference->ref_sdl, REQ_slice, reference->ref_sdl_length, begin_i, end_i);
				}
				if (!gpreGlob.sw_raw)
				{
					printa(COMMENT, " ");
					if (PRETTY_print_sdl(reference->ref_sdl, gen_blr, 0, 0))
						CPR_error("internal error during SDL generation");
					printa(COMMENT, " ");
					printa(COMMENT, "END OF SDL STRING FOR REQUEST isc_%d\n", reference->ref_sdl_ident);
				}
			}
		}

	// Print out any blob parameter blocks required

	for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		if (blob->blb_bpb_length)
		{
			for (int begin_i = 0; begin_i < blob->blb_bpb_length; begin_i = begin_i + (75 * sizeof(SLONG)))
			{
				const int end_i =
					MIN(blob->blb_bpb_length - 1, begin_i + (SLONG)(75 * sizeof(SLONG)) - 1);
				printa(COLUMN, "DATA (isc_%d(ISC_I)%s ISC_I=%d,%d)  /",
					   blob->blb_bpb_ident, COMMA,
					   (begin_i / sizeof(SLONG)) + 1,
					   (end_i / sizeof(SLONG)) + 1);
				gen_raw(blob->blb_bpb, REQ_for, blob->blb_bpb_length, begin_i, end_i);
				printa(COMMENT, " ");
			}
		}
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_request_decls( const gpre_req* request)
{
	if (!(request->req_flags & (REQ_exp_hand | REQ_sql_blob_open | REQ_sql_blob_create)) &&
		request->req_type != REQ_slice && request->req_type != REQ_procedure)
	{
		fprintf(gpreGlob.out_file, "%sINTEGER*4  %s             %s{ request handle }\n\n",
				COLUMN, request->req_handle, INLINE_COMMENT);
	}
	// generate the request as BLR long words

	const int rlength = (request->req_length + (sizeof(SLONG) - 1)) / sizeof(SLONG);
	if (rlength)
	{
		fprintf(gpreGlob.out_file, "%sINTEGER*4      isc_%d(%d)    %s{ request BLR }\n",
				COLUMN, request->req_ident, rlength, INLINE_COMMENT);
	}

	// Generate declarations for the slice description language

	for (const gpre_port* port = request->req_ports; port; port = port->por_next)
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_sdl)
			{
				const int slength = (reference->ref_sdl_length + (sizeof(SLONG) - 1)) / sizeof(SLONG);
				fprintf(gpreGlob.out_file, "%sINTEGER*4      isc_%d(%d)     %s{ request SDL }\n",
						COLUMN, reference->ref_sdl_ident, slength, INLINE_COMMENT);
			}
		}

	// Print out any blob parameter block variable declarations required
	for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		if (blob->blb_const_from_type)
		{
			const int blength = (blob->blb_bpb_length + (sizeof(SLONG) - 1)) / sizeof(SLONG);
			fprintf(gpreGlob.out_file, "%sINTEGER*4       isc_%d(%d)      %s{ blob parameter block }\n",
					COLUMN, blob->blb_bpb_ident, blength, INLINE_COMMENT);
		}

	if (request->req_flags & REQ_sql_cursor)
		fprintf(gpreGlob.out_file, "%sINTEGER*4  isc_%dS             %s{ SQL statement handle }\n\n",
				COLUMN, request->req_ident, INLINE_COMMENT);

	if (request->req_type == REQ_ready || request->req_type == REQ_create_database)
	{
		printa(COLUMN, "INTEGER*2  isc_%dl", request->req_ident);
		if (request->req_flags & REQ_extend_dpb)
			printa(COLUMN, "INTEGER*4  isc_%dp", request->req_ident);
	}


	// If this is a GET_SLICE/PUT_slice, allocate some variables

	if (request->req_type == REQ_slice)
	{
		printa(COLUMN, "INTEGER*4 isc_%dv (%d)", request->req_ident,
			   MAX(request->req_slice->slc_parameters, 1));
		printa(COLUMN, "INTEGER*4  isc_%ds", request->req_ident);
	}
}


//____________________________________________________________
//
//		Generate receive call for a port
//		in a store2 statement.
//

static void gen_return_value(const act* action)
{
	const gpre_req* request = action->act_request;

	gen_start(action, request->req_primary);
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(COLUMN, "END IF");

	const upd* update = (const upd*) action->act_object;
	const ref* reference = update->upd_references;
	gen_receive(action, reference->ref_port);
}


//____________________________________________________________
//
//		Process routine head.  If there are gpreGlob.requests in the
//		routine, insert local definitions.
//

static void gen_routine(const act* action)
{
	for (const gpre_req* request = (const gpre_req*) action->act_object; request;
		 request = request->req_routine)
	{
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			make_port(port);
		for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			fprintf(gpreGlob.out_file, "%sINTEGER*4 isc_%d         %s{ blob handle }\n",
					   COLUMN, blob->blb_ident, INLINE_COMMENT);
			fprintf(gpreGlob.out_file, "%sCHARACTER*%d isc_%d      %s{ blob segment }\n",
					   COLUMN, blob->blb_seg_length, blob->blb_buff_ident, INLINE_COMMENT);
			fprintf(gpreGlob.out_file, "%sINTEGER*2 isc_%d         %s{ segment length }\n",
					   COLUMN, blob->blb_len_ident, INLINE_COMMENT);
		}
	}
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static void gen_s_end(const act* action)
{
	const gpre_req* request = action->act_request;

	if (action->act_type == ACT_close)
		gen_cursor_close(request);

	printa(COLUMN, "CALL ISC_UNWIND_REQUEST (%s, %s, %s%s%s)",
		   status_vector(),
		   request->req_handle, VAL_1, request->req_request_level, VAL_2);

	if (action->act_type == ACT_close)
	{
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
	}

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate substitution text for FETCH.
//

static void gen_s_fetch(const act* action)
{
	const gpre_req* request = action->act_request;
	if (request->req_sync)
		gen_send(action, request->req_sync);

	gen_receive(action, request->req_primary);
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_s_start(const act* action)
{
	const gpre_req* request = action->act_request;

	gen_compile(action);

	const gpre_port* port = request->req_vport;
	if (port)
		asgn_from(action, port->por_references);

	if (action->act_type == ACT_open)
		gen_cursor_open(action, request);

	if (action->act_error || (action->act_flags & ACT_sql))
		make_ok_test(action, request);

	gen_start(action, port);

	if (action->act_error || (action->act_flags & ACT_sql))
		printa(COLUMN, "END IF");

	if (action->act_type == ACT_open)
	{
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
		printa(COLUMN, "END IF");
		status_and_stop(action);
	}
}


//____________________________________________________________
//
//		Substitute for a segment, segment length, or blob handle.
//

static void gen_segment(const act* action)
{
	blb* blob = (blb*) action->act_object;

	printa("", "%sisc_%d",
		   (action->act_flags & ACT_first) ? COLUMN : CONTINUE,
		   (action->act_type == ACT_segment) ? blob->blb_buff_ident :
		   (action->act_type == ACT_segment_length) ? blob->blb_len_ident : blob->blb_ident);
	fputs(CONTINUE, gpreGlob.out_file);
}


//____________________________________________________________
//
//

static void gen_select(const act* action)
{
	SCHAR name[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	const gpre_port* port = request->req_primary;
	gen_name(name, request->req_eof, true);

	gen_s_start(action);
	printa(COLUMN, "IF (SQLCODE .EQ. 0) THEN");
	gen_receive(action, port);
	printa(COLUMN, "IF (%s .NE. 0) THEN", name);
    gpre_nod* var_list = (gpre_nod*) action->act_object;
	if (var_list)
	{
		for (int i = 0; i < var_list->nod_count; ++i)
			asgn_to(action, (const ref*) var_list->nod_arg[i]);
	}

	printa(COLUMN, "ELSE");
	printa(COLUMN, "SQLCODE = 100");
	printa(COLUMN, "END IF");
	printa(COLUMN, "END IF");
}


//____________________________________________________________
//
//		Generate a send call for a port.
//

static void gen_send(const act* action, const gpre_port* port)
{
	const gpre_req* request = action->act_request;
	sprintf(output_buffer, "%s CALL ISC_SEND (%s, %s, %s%d%s, %s%d%s, %sisc_%d%s, %s%s%s)\n",
			COLUMN, status_vector(), request->req_handle, I2CONST_1,
			port->por_msg_number, I2CONST_2, I2CONST_1, port->por_length,
			I2CONST_2, REF_1, port->por_ident, REF_2, VAL_1,
			request->req_request_level, VAL_2);

	FTN_print_buffer(output_buffer);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate support for get/put slice statement.
//

static void gen_slice(const act* action)
{
	TEXT buffer[256], temp[64];
	const TEXT* pattern1 =
		"CALL ISC_GET_SLICE (%V1, %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5%RE, %RF%S6%RE)";
	const TEXT* pattern2 =
		"CALL ISC_PUT_SLICE (%V1, %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5%RE)";

	const gpre_req* request = action->act_request;
	const slc* slice = (slc*) action->act_object;
	const gpre_req* parent_request = slice->slc_parent_request;

	// Compute array size

	sprintf(buffer, "isc_%ds = %d", request->req_ident, slice->slc_field->fld_array->fld_length);

	const slc::slc_repeat* tail = slice->slc_rpt;
	for (const slc::slc_repeat* const end = tail + slice->slc_dimensions; tail < end; ++tail)
	{
		if (tail->slc_upper != tail->slc_lower)
		{
			const ref* lower = (const ref*) tail->slc_lower->nod_arg[0];
			const ref* upper = (const ref*) tail->slc_upper->nod_arg[0];
			if (lower->ref_value)
				sprintf(temp, " * ( %s - %s + 1)", upper->ref_value, lower->ref_value);
			else
				sprintf(temp, " * ( %s + 1)", upper->ref_value);
			strcat(buffer, temp);
		}
	}
	printa(COLUMN, buffer);

	// Make assignments to variable vector

	const ref* reference;
	for (reference = request->req_values; reference; reference = reference->ref_next)
	{
		printa(COLUMN, "isc_%dv [%d] = %s;", request->req_ident, reference->ref_id,
			   reference->ref_value);
	}

	PAT args;
	args.pat_reference = slice->slc_field_ref;
	args.pat_request = parent_request;	// blob id request
	args.pat_vector1 = status_vector();	// status vector
	args.pat_database = parent_request->req_database;	// database handle
	args.pat_string1 = action->act_request->req_trans;	// transaction handle
	args.pat_value1 = request->req_length;	// slice descr. length
	args.pat_ident1 = request->req_ident;	// request name
	args.pat_value2 = slice->slc_parameters * sizeof(SLONG);	// parameter length

	reference = (const ref*) slice->slc_array->nod_arg[0];
	args.pat_string5 = reference->ref_value;	// array name
	args.pat_string6 = "isc_array_length";

	const SSHORT column = 6;

	PATTERN_expand(column, (action->act_type == ACT_get_slice) ? pattern1 : pattern2, &args);
}


//____________________________________________________________
//
//		Generate either a START or START_AND_SEND depending
//		on whether or a not a port is present.
//

static void gen_start(const act* action, const gpre_port* port)
{
	const gpre_req* request = action->act_request;
	const TEXT* vector = status_vector();

	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field-> fld_array_info)
				gen_get_or_put_slice(action, reference, false);
		}

		sprintf(output_buffer,
				"%sCALL ISC_START_AND_SEND (%s, %s, %s, %s%d%s, %s%d%s, %sisc_%d%s, %s%s%s)\n",
				COLUMN, vector, request->req_handle,
				request_trans(action, request),
				I2CONST_1, port->por_msg_number, I2CONST_2,
				I2CONST_1, port->por_length, I2CONST_2,
				REF_1, port->por_ident, REF_2,
				I2CONST_1, request->req_request_level, I2CONST_2);
	}
	else
		sprintf(output_buffer, "%sCALL ISC_START_REQUEST (%s, %s, %s, %s%s%s)\n",
				COLUMN, vector, request->req_handle, request_trans(action, request),
				I2CONST_1, request->req_request_level, I2CONST_2);

	FTN_print_buffer(output_buffer);

	status_and_stop(action);
}


//____________________________________________________________
//
//		Generate text for STORE statement.  This includes the compile
//		call and any variable initialization required.
//

static void gen_store(const act* action)
{
	TEXT name[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	gen_compile(action);
	if (action->act_error || (action->act_flags & ACT_sql))
		make_ok_test(action, request);

	// Initialize any blob fields

	const gpre_port* port = request->req_primary;
	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_flags & FLD_blob)
			printa(COLUMN, "CALL isc_qtoq (isc_blob_null, %s)", gen_name(name, reference, true));
	}
}


//____________________________________________________________
//
//		Generate substitution text for START_TRANSACTION.
//

static void gen_t_start(const act* action)
{
	// if this is a purely default transaction, just let it through
	gpre_tra* trans;
	if (!action || !(trans = (gpre_tra*) action->act_object))
	{
		t_start_auto(0, status_vector(), action, false);
		return;
	}

	// build a complete statement, including tpb's.  Ready db's as req.

	const tpb* tpb_iterator;
	if (gpreGlob.sw_auto)
		for (tpb_iterator = trans->tra_tpb; tpb_iterator; tpb_iterator = tpb_iterator->tpb_tra_next)
		{
			const gpre_dbb* db = tpb_iterator->tpb_database;
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				printa(COLUMN, "IF (%s .EQ. 0) THEN", db->dbb_name->sym_string);
				make_ready(db, filename, status_vector(), 0);
				status_and_stop(action);
				printa(COLUMN, "END IF");
			}
		}

#ifdef HPUX
	// If this is HPUX we should be building a teb vector here
	// with the tpb address and length specified

	int count = 0;
	for (tpb_iterator = trans->tra_tpb; tpb_iterator; tpb_iterator = tpb_iterator->tpb_tra_next)
	{
		count++;
		const gpre_dbb* db = tpb_iterator->tpb_database;
		printa(COLUMN, "ISC_TEB%d_LEN = %d", count, tpb_iterator->tpb_length);
		printa(COLUMN, "ISC_TEB%d_TPB = ISC_BADDRESS (ISC_TPB_%d)",  count, tpb_iterator->tpb_ident);
		printa(COLUMN, "ISC_TEB%d_DBB = ISC_BADDRESS (%s)", count, db->dbb_name->sym_string);
	}

	printa(COLUMN, "CALL ISC_START_MULTIPLE (%s, %s, %d, ISC_TEB)",
		   status_vector(),
		   trans->tra_handle ? trans->tra_handle : "gds__trans",
		   trans->tra_db_count);

#else

	printa(COLUMN, "CALL ISC_START_TRANSACTION (%s, %s, %s%d%s",
		   status_vector(),
		   trans->tra_handle ? trans->tra_handle : "GDS__TRANS",
		   I2CONST_1, trans->tra_db_count, I2CONST_2);

	for (tpb_iterator = trans->tra_tpb; tpb_iterator; tpb_iterator = tpb_iterator->tpb_tra_next)
	{
		printa(CONTINUE, ", %s, %s%d%s, isc_tpb_%d",
			   tpb_iterator->tpb_database->dbb_name->sym_string,
			   I2CONST_1, tpb_iterator->tpb_length, I2CONST_2,
			   tpb_iterator->tpb_ident);
	}

	printa(CONTINUE, ")");
#endif

	status_and_stop(action);
}


//____________________________________________________________
//
//		Initialize a TPB in the output file
//

static void gen_tpb_data(const tpb* tpb_buffer)
{
	union {
		UCHAR bytewise_tpb[4];
		SLONG longword_tpb;
	} tpb_hunk;

	// TPBs are generated as raw BLR in longword chunks
	// because FORTRAN is a miserable excuse for a language
	// and doesn't allow byte value assignments to character
	// fields.

	int length = (tpb_buffer->tpb_length + (sizeof(SLONG) - 1)) / sizeof(SLONG);

	printa(COLUMN, "DATA ISC_TPB_%d  /", tpb_buffer->tpb_ident, COMMA, length);

	const UCHAR* text = tpb_buffer->tpb_string;
	length = tpb_buffer->tpb_length;
	strcpy(output_buffer, CONTINUE);

	TEXT* p;
	for (p = output_buffer; *p; p++);

	while (length)
	{
		for (UCHAR* c = tpb_hunk.bytewise_tpb; c < tpb_hunk.bytewise_tpb + sizeof(SLONG); c++)
		{
			*c = *text++;
			if (!--length)
				break;
		}
		if (length)
			sprintf(p, "%"SLONGFORMAT",", tpb_hunk.longword_tpb);
		else
			sprintf(p, "%"SLONGFORMAT"/\n", tpb_hunk.longword_tpb);
		p += 12; // ???
	}

	FTN_print_buffer(output_buffer);
	sprintf(output_buffer, "%sEnd of data for ISC_TPB_%d\n", COMMENT, tpb_buffer->tpb_ident);
	FTN_print_buffer(output_buffer);
}


//____________________________________________________________
//
//		Generate the declaration for a
//       TPB in the output file
//

static void gen_tpb_decls(const tpb* tpb_buffer)
{
	const int length = (tpb_buffer->tpb_length + (sizeof(SLONG) - 1)) / sizeof(SLONG);
	fprintf(gpreGlob.out_file, "%sINTEGER*4      ISC_TPB_%d(%d)    %s{ transaction parameters }\n",
			   COLUMN, tpb_buffer->tpb_ident, length, INLINE_COMMENT);
}


//____________________________________________________________
//
//		Generate substitution text for COMMIT, ROLLBACK, PREPARE, and SAVE
//

static void gen_trans(const act* action)
{
	const char* tranText = action->act_object ? (const TEXT*) action->act_object : "GDS__TRANS";

	switch (action->act_type)
	{
	case ACT_commit_retain_context:
		printa(COLUMN, "CALL ISC_COMMIT_RETAINING (%s, %s)", status_vector(), tranText);
		break;
	case ACT_rollback_retain_context:
		printa(COLUMN, "CALL ISC_ROLLBACK_RETAINING (%s, %s)", status_vector(), tranText);
		break;
	default:
		printa(COLUMN, "CALL ISC_%s_TRANSACTION (%s, %s)",
			   (action->act_type == ACT_commit) ?
					"COMMIT" : (action->act_type == ACT_rollback) ? "ROLLBACK" : "PREPARE",
				status_vector(), tranText);
	}

	status_and_stop(action);

}


//____________________________________________________________
//
//		Generate substitution text for UPDATE ... WHERE CURRENT OF ...
//

static void gen_update(const act* action)
{
	const upd* modify = (const upd*) action->act_object;
	const gpre_port* port = modify->upd_port;
	asgn_from(action, port->por_references);
	gen_send(action, port);
}


//____________________________________________________________
//
//		Substitute for a variable reference.
//

static void gen_variable(const act* action)
{
	SCHAR s[MAX_REF_SIZE];
	const ref* reference = (const ref*) action->act_object;
	printa("", "%s%s", (action->act_flags & ACT_first) ? COLUMN : CONTINUE,
		   gen_name(s, reference, false));

	fputs(CONTINUE, gpreGlob.out_file);
}


//____________________________________________________________
//
//		Generate tests for any WHENEVER clauses that may have been declared.
//

static void gen_whenever(const swe* label)
{
	const TEXT* condition = NULL;

	while (label)
	{
		switch (label->swe_condition)
		{
		case SWE_error:
			condition = "SQLCODE .LT. 0";
			break;

		case SWE_warning:
			condition = "SQLCODE .EQ. 0 .AND. SQLCODE .NE. 100";
			break;

		case SWE_not_found:
			condition = "SQLCODE .EQ. 100";
			break;

		default:
			// condition undefined
			fb_assert(false);
			return;
		}
		printa(COLUMN, "if (%s) goto %s", condition, label->swe_label);
		label = label->swe_next;
	}
}

//____________________________________________________________
//
//		Generate a declaration of an array in the
//       output file.
//

static void make_array_declaration( const ref* reference)
{
	const gpre_fld* field = reference->ref_field;
	const SCHAR* const name = field->fld_symbol->sym_string;

	// Check to see if the array already has been
	// declared in this routine or subroutine
	if (array_decl_list)
	{
		for (adl* loop_array = array_decl_list; loop_array; loop_array = loop_array->adl_next)
		{
			if (field->fld_array_info->ary_ident == loop_array->adl_gds_ident)
			{
				return;
			}
        }
    }

	// If not, add it to the "declared" list and declare it
	adl* this_array = (adl*) MSC_alloc(ADL_LEN);
	this_array->adl_gds_ident = field->fld_array_info->ary_ident;
	if (array_decl_list)
		this_array->adl_next = array_decl_list;
	else
		this_array->adl_next = NULL;
	array_decl_list = this_array;

	switch (field->fld_array_info->ary_dtype)
	{
	case dtype_short:
		fprintf(gpreGlob.out_file, "%sINTEGER*2%s", COLUMN, COLUMN);
		break;

	case dtype_long:
		fprintf(gpreGlob.out_file, "%sINTEGER*4%s", COLUMN, COLUMN);
		break;

	case dtype_date:
	case dtype_blob:
	case dtype_quad:
		fprintf(gpreGlob.out_file, "%sINTEGER*4%s", COLUMN, COLUMN);
		break;

	case dtype_text:
		fprintf(gpreGlob.out_file, "%sCHARACTER*%d%s", COLUMN, field->fld_array->fld_length, COLUMN);
		break;

	case dtype_real:
		fprintf(gpreGlob.out_file, "%sREAL%s", COLUMN, COLUMN);
		break;

	case dtype_double:
		fprintf(gpreGlob.out_file, "%s%s%s", COLUMN, DOUBLE_DCL, COLUMN);
		break;

	default:
	    {
			TEXT s[64];
			sprintf(s, "datatype %d unknown\n", field->fld_dtype);
			CPR_error(s);
			return;
		}
	}

	// Print out the dimension part of the declaration
	fprintf(gpreGlob.out_file, "isc_%d", field->fld_array_info->ary_ident);
	fprintf(gpreGlob.out_file, "(");

	for (dim* dimension = field->fld_array_info->ary_dimension; dimension;
		dimension = dimension->dim_next)
	{
		if (dimension->dim_lower != 1)
			fprintf(gpreGlob.out_file, "%"SLONGFORMAT":", dimension->dim_lower);

		fprintf(gpreGlob.out_file, "%"SLONGFORMAT, dimension->dim_upper);
		if (dimension->dim_next)
			fprintf(gpreGlob.out_file, ", ");
	}

	if (field->fld_dtype == dtype_quad || field->fld_dtype == dtype_date)
		fprintf(gpreGlob.out_file, ",2");

	// Print out the database field

	fprintf(gpreGlob.out_file, ")        %s{ %s }\n", INLINE_COMMENT, name);
}


//____________________________________________________________
//
//		Turn a symbol into a varying string.
//

static TEXT* make_name( TEXT* const string, const gpre_sym* symbol)
{
	fb_utils::snprintf(string, MAX_CURSOR_SIZE, "%s'%s '%s", REF_1, symbol->sym_string, REF_2);

	return string;
}


//____________________________________________________________
//
//		Generate code to test existence of compiled request with
//		active transaction
//

static void make_ok_test(const act* action, const gpre_req* request)
{
	if (gpreGlob.sw_auto)
		printa(COLUMN, "IF (%s .NE. 0 .AND. %s .NE. 0) THEN",
			   request_trans(action, request), request->req_handle);
	else
		printa(COLUMN, "IF (%s .NE. 0) THEN", request->req_handle);
}


//____________________________________________________________
//
//		Insert a port record description in output.
//

static void make_port( const gpre_port* port)
{
	const USHORT length = (port->por_length + 3) & ~3;
	printa(COLUMN, "CHARACTER      isc_%d(%d)", port->por_ident, length);

	const ref* reference;
	for (reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		const gpre_sym* symbol = field->fld_symbol;
		const SCHAR* name;
		if (symbol)
			name = symbol->sym_string;
		else
			name = "<expression>";
		if (reference->ref_value && (reference->ref_flags & REF_array_elem))
			field = field->fld_array;

		switch (field->fld_dtype)
		{
		case dtype_short:
			fprintf(gpreGlob.out_file, "%sINTEGER*2      isc_%d      %s{ %s }\n",
					   COLUMN, reference->ref_ident, INLINE_COMMENT, name);
			break;

		case dtype_long:
			fprintf(gpreGlob.out_file, "%sINTEGER*4      isc_%d      %s{ %s }\n",
					   COLUMN, reference->ref_ident, INLINE_COMMENT, name);
			break;

		case dtype_cstring:
		case dtype_text:
			fprintf(gpreGlob.out_file, "%sCHARACTER*%d   isc_%d      %s{ %s }\n",
					   COLUMN, field->fld_length, reference->ref_ident, INLINE_COMMENT, name);
			break;

		case dtype_date:
		case dtype_quad:
		case dtype_blob:
			fprintf(gpreGlob.out_file, "%sINTEGER*4      isc_%d(2)   %s{ %s }\n",
					   COLUMN, reference->ref_ident, INLINE_COMMENT, name);
			break;

		case dtype_real:
			fprintf(gpreGlob.out_file, "%sREAL          isc_%d      %s{ %s }\n",
					   COLUMN, reference->ref_ident, INLINE_COMMENT, name);
			break;

		case dtype_double:
			fprintf(gpreGlob.out_file, "%s%s         isc_%d      %s{ %s }\n",
					   COLUMN, DOUBLE_DCL, reference->ref_ident, INLINE_COMMENT, name);
			break;

		default:
		    {
			    SCHAR s[ERROR_LENGTH];
				fb_utils::snprintf(s, sizeof(s), "datatype %d unknown for field %s, msg %d",
						field->fld_dtype, name, port->por_msg_number);
				CPR_error(s);
				return;
			}
		}
	}

	for (reference = port->por_references; reference; reference = reference->ref_next)
	{
		printa(COLUMN, "EQUIVALENCE    (isc_%d(%d), isc_%d)",
			   port->por_ident, reference->ref_offset + 1,
			   reference->ref_ident);
	}

	printa(COLUMN, " ");
}


//____________________________________________________________
//
//		Generate the actual ready call.
//

static void make_ready(const gpre_dbb* db, const TEXT* filename, const TEXT* vector, const gpre_req* request)
{
	TEXT s1[32], s2[32];

	if (request)
	{
		sprintf(s1, "isc_%dl", request->req_ident);

		if (request->req_flags & REQ_extend_dpb)
			sprintf(s2, "isc_%dp", request->req_ident);
		else
			sprintf(s2, "isc_%d", request->req_ident);
		// if the dpb needs to be extended at runtime to include items
		// in host variables, do so here; this assumes that there is
		// always a request generated for runtime variables

		if (request->req_flags & REQ_extend_dpb)
		{
			if (request->req_length)
			{
				sprintf(output_buffer, "%s%s = isc_%d\n", COLUMN, s2, request->req_ident);
				FTN_print_buffer(output_buffer);
			}
			// MMM
			else
			{
				sprintf(output_buffer, "%s%s = 0\n", COLUMN, s2);
				FTN_print_buffer(output_buffer);
			}

			if (db->dbb_r_user)
			{
				sprintf(output_buffer,
						"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_user_name, %s, %sLEN(%s)%s)\n",
						COLUMN, s2, s1, db->dbb_r_user,
						I2CONST_1, db->dbb_r_user, I2CONST_2);
				FTN_print_buffer(output_buffer);
			}
			if (db->dbb_r_password)
			{
				sprintf(output_buffer,
						"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_password, %s, %sLEN(%s)%s)\n",
						COLUMN, s2, s1, db->dbb_r_password,
						I2CONST_1, db->dbb_r_password, I2CONST_2);
				FTN_print_buffer(output_buffer);
			}

			// SQL Role supports GPRE/Fortran
			if (db->dbb_r_sql_role)
			{
				sprintf(output_buffer,
						"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_sql_role_name, %s, %sLEN(%s)%s)\n",
						COLUMN, s2, s1, db->dbb_r_sql_role,
						I2CONST_1, db->dbb_r_sql_role, I2CONST_2);
				FTN_print_buffer(output_buffer);
			}

			if (db->dbb_r_lc_messages)
			{
				sprintf(output_buffer,
						"%sCALL ISC_MODIFY_DPB(%s, %s, isc_dpb_lc_messages, %s, %sLEN(%s)%s)\n",
						COLUMN, s2, s1, db->dbb_r_lc_messages,
						I2CONST_1, db->dbb_r_lc_messages, I2CONST_2);
				FTN_print_buffer(output_buffer);
			}
			if (db->dbb_r_lc_ctype)
			{
				sprintf(output_buffer,
						"%sCALL ISC_MODIFY_DPB (%s, %s, isc_dpb_lc_type, %s, %sLEN(%s)%s)\n",
						COLUMN, s2, s1, db->dbb_r_lc_ctype,
						I2CONST_1, db->dbb_r_lc_ctype, I2CONST_2);
				FTN_print_buffer(output_buffer);
			}
		}
	}

	if (filename)
	{
		sprintf(output_buffer, "%sISC_%s = %s\n", COLUMN, db->dbb_name->sym_string, filename);
		FTN_print_buffer(output_buffer);

		sprintf(output_buffer,
				"%sCALL ISC_ATTACH_DATABASE (%s, %sLEN(%s)%s, %sISC_%s%s, %s, %s%s%s, %s)\n",
				COLUMN, vector, I2CONST_1, filename, I2CONST_2,
				REF_1, db->dbb_name->sym_string, REF_2,
				db->dbb_name->sym_string, I2CONST_1,
				(request ? s1 : "0"), I2CONST_2, (request ? s2 : "0"));
		FTN_print_buffer(output_buffer);
	}
	else
	{
		sprintf(output_buffer, "%sISC_%s = '%s'\n", COLUMN, db->dbb_name->sym_string, db->dbb_filename);
		FTN_print_buffer(output_buffer);

		sprintf(output_buffer,
				"%sCALL ISC_ATTACH_DATABASE (%s, %sLEN('%s')%s, %sISC_%s%s, %s, %s%s%s, %s)\n",
				COLUMN, vector, I2CONST_1, db->dbb_filename, I2CONST_2,
				REF_1, db->dbb_name->sym_string, REF_2,
				db->dbb_name->sym_string, I2CONST_1,
				(request ? s1 : "0"), I2CONST_2, (request ? s2 : "0"));
		FTN_print_buffer(output_buffer);
	}
	if (request && request->req_flags & REQ_extend_dpb)
	{
		if (request->req_length)
		{
			sprintf(output_buffer, "%sif (%s != isc_%d)\n", COLUMN, s2, request->req_ident);
			FTN_print_buffer(output_buffer);
		}
		sprintf(output_buffer, "%sCALL ISC_FREE (%s)\n", COLUMN, s2);
		FTN_print_buffer(output_buffer);

		// reset the length of the dpb

		sprintf(output_buffer, "%s%s = %d\n", COLUMN, s1, request->req_length);
		FTN_print_buffer(output_buffer);
	}
}


//____________________________________________________________
//
//		Looks at the label bitmap and allocates
//       an unused label.  Marks the current
//       label as used.
//

static USHORT next_label()
{
	UCHAR* byte = gpreGlob.fortran_labels;
	while (*byte == 255)
		++byte;

	USHORT label = ((byte - gpreGlob.fortran_labels) << 3);

	for (UCHAR target_byte = *byte; target_byte & 1; target_byte >>= 1)
		label++;

	*byte |= 1 << (label & 7);

	return label;
}


//____________________________________________________________
//
//		Print a fixed string at a particular COLUMN.
//

static void printa(const TEXT* column, const TEXT* string, ...)
{
	va_list ptr;
	SCHAR s[256];

	va_start(ptr, string);
	strcpy(s, column);
	strcat(s, string);
	strcat(s, "\n");
	vsprintf(output_buffer, s, ptr);
	va_end(ptr);
	FTN_print_buffer(output_buffer);
}


//____________________________________________________________
//
//		Generate the appropriate transaction handle.
//

static const TEXT* request_trans(const act* action, const gpre_req* request)
{
	if (action->act_type == ACT_open)
	{
		const TEXT* trname = ((open_cursor*) action->act_object)->opn_trans;
		if (!trname)
			trname = "GDS__TRANS";
		return trname;
	}

	return request ? request->req_trans : "GDS__TRANS";
}


//____________________________________________________________
//
//		Do the error handling ourselves
//       until we figure out how to use the
//       ISC_NULL from FORTRAN
//

static void status_and_stop(const act* action)
{
	if (action && (action->act_flags & ACT_sql))
		printa(COLUMN, "SQLCODE = ISC_SQLCODE (ISC_STATUS)");
	else if (!action || !action->act_error)
	{
		printa(COLUMN, "IF (ISC_STATUS(2) .NE. 0) THEN");
		printa(COLUMN, "    CALL ISC_PRINT_STATUS (ISC_STATUS)");
		printa(COLUMN, "    STOP");
		printa(COLUMN, "END IF");
	}
}


//____________________________________________________________
//
//		Generate the appropriate status vector parameter for a gds
//		call depending on where or not the action has an error clause.
//

static const TEXT* status_vector() //(const act* action)
{
	return "ISC_STATUS";
//	return (!action || !action->act_error) ? "ISC_NULL" : "ISC_STATUS";
}


//____________________________________________________________
//
//		Generate substitution text for START_TRANSACTION,
//       when it's being generated automatically by a compile
//       call or one of the DDL commands.  Be careful not to
//		continue after errors as that destroys evidence.
//

static void t_start_auto(const gpre_req* request, const TEXT* vector, const act* action, bool test)
{
	TEXT buffer[256], temp[40];
	buffer[0] = 0;
	const TEXT* trname = request_trans(action, request);

	// this is a default transaction, make sure all databases are ready

	int count = 0;
	for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		if (gpreGlob.sw_auto)
		{
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				if (buffer[0])
					printa(COLUMN, "IF (%s .EQ. 0 .AND. %s(2) .EQ. 0) THEN",
						   db->dbb_name->sym_string, vector);
				else
					printa(COLUMN, "IF (%s .EQ. 0) THEN", db->dbb_name->sym_string);
				make_ready(db, filename, vector, 0);
				printa(COLUMN, "END IF");
				if (buffer[0])
					strcat(buffer, " .AND. ");
				sprintf(temp, "%s .NE. 0", db->dbb_name->sym_string);
				strcat(buffer, temp);
			}
		}

		count++;
#ifdef HPUX
		printa(COLUMN, "ISC_TEB%d_LEN = 0", count);
		printa(COLUMN, "ISC_TEB%d_TPB = ISC_NULL", count);
		printa(COLUMN, "ISC_TEB%d_DBB = ISC_BADDRESS (%s)", count, db->dbb_name->sym_string);
#endif
	}

	if (gpreGlob.sw_auto)
	{
		if (!buffer[0])
			strcpy(buffer, ".TRUE.");
		if (test)
			printa(COLUMN, "IF ((%s) .AND. (%s .EQ. 0)) THEN", buffer, trname);
		else
			printa(COLUMN, "IF (%s) THEN", buffer);
	}

#ifdef HPUX
	printa(COLUMN_INDENT, "CALL ISC_START_MULTIPLE (%s, %s, %s%d%s, ISC_TEB)",
		   vector, trname, I2CONST_1, count, I2CONST_2);
#else
	printa(COLUMN_INDENT, "CALL ISC_START_TRANSACTION (%s, %s, %s%d%s",
		   vector, trname, I2CONST_1, count, I2CONST_2);

	for (const gpre_dbb* db2 = gpreGlob.isc_databases; db2; db2 = db2->dbb_next)
	{
		printa(CONTINUE, ", %s, %s0%s, 0", db2->dbb_name->sym_string, I2CONST_1, I2CONST_2);
	}
	printa(CONTINUE, ")");
#endif

	if (gpreGlob.sw_auto)
		printa(COLUMN, "END IF");

	status_and_stop(action);
}

#ifdef NOT_USED_OR_REPLACED
// RRK_?: this column stuff was not used in 3.3
// may be should not bother with it now
//____________________________________________________________
//
//		Align output to a specific column for output.  If the
//		column is negative, don't do anything.
//

static void align( int column)
{
	if (column < 0)
		return;

	putc('\n', gpreGlob.out_file);

	for (int i = column / 8; i; --i)
		putc('\t', gpreGlob.out_file);

	for (int i = column % 8; i; --i)
		putc(' ', gpreGlob.out_file);
}
#endif // RRK_?: end of comment out

//____________________________________________________________
//
//		Generate a function call for free standing ANY.  Somebody else
//		will need to generate the actual function.
//

static void gen_any(const act* action)
{
	const gpre_req* request = action->act_request;

	fprintf(gpreGlob.out_file, "%s%s_r (&%s, &%s", COLUMN,
			   request->req_handle, request->req_handle, request->req_trans);

	const gpre_port* port = request->req_vport;
	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			fprintf(gpreGlob.out_file, ", %s", reference->ref_value);
		}
	}

	fprintf(gpreGlob.out_file, ")");
}

//____________________________________________________________
//
//		Zap all know handles.
//

static void gen_clear_handles() //const act* action)
{
	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
	{
		if (!(request->req_flags & REQ_exp_hand))
			printa("%s%s = 0;", COLUMN, request->req_handle);
	}
}

#ifdef NOT_USED_OR_REPLACED
//____________________________________________________________
//
//		Generate a symbol to ease compatibility with V3.
//

static void gen_compatibility_symbol(const TEXT* symbol, const TEXT* v4_prefix, const TEXT* trailer)
{
	// CVC: always the same prefix? Function not used, so no problem. :-)
	const char* v3_prefix = isLangCpp(gpreGlob.sw_language) ? "isc_" : "isc_";

	fprintf(gpreGlob.out_file, "#define %s%s\t%s%s%s\n", v3_prefix, symbol, v4_prefix, symbol, trailer);
}
#endif

//____________________________________________________________
//
//		Generate a function for free standing ANY or statistical.
//

static void gen_function(const act* function)
{
	const act* action = (const act*) function->act_object;

	if (action->act_type != ACT_any)
	{
		CPR_error("can't generate function");
		return;
	}

	const gpre_req* request = action->act_request;

	fprintf(gpreGlob.out_file, "static %s_r (request, transaction", request->req_handle);

	TEXT s[MAX_REF_SIZE];
    const gpre_port* port = request->req_vport;
	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			fprintf(gpreGlob.out_file, ", %s", gen_name(s, reference->ref_source, true));
		}
	}

	fprintf(gpreGlob.out_file, ")\n    isc_req_handle\trequest;\n    isc_tr_handle\ttransaction;\n");

	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			const gpre_fld* field = reference->ref_field;
			const TEXT* dtype = NULL;

			switch (field->fld_dtype)
			{
			case dtype_short:
				dtype = "short";
				break;

			case dtype_long:
				// RRK_?:		dtype = DCL_LONG;
				break;

			case dtype_cstring:
			case dtype_text:
				dtype = "char*";
				break;

			case dtype_quad:
				//dtype = DCL_QUAD;
				break;

			case dtype_date:
			case dtype_blob:
				dtype = "ISC_QUAD";
				break;

			case dtype_real:
				dtype = "float";
				break;

			case dtype_double:
				dtype = "double";
				break;

			default:
				CPR_error("gen_function: unsupported datatype");
				return;
			}
			fprintf(gpreGlob.out_file, "    %s\t%s;\n", dtype,
					gen_name(s, reference->ref_source, true));
		}
	}

	fprintf(gpreGlob.out_file, "{\n");
	for (port = request->req_ports; port; port = port->por_next)
		make_port(port);

	fprintf(gpreGlob.out_file, "\n\n");
	gen_s_start(action);
	gen_receive(action, request->req_primary);

	for (port = request->req_ports; port; port = port->por_next)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field-> fld_array_info)
				gen_get_or_put_slice(action, reference, true);
		}
	}

	port = request->req_primary;
	fprintf(gpreGlob.out_file, "\nreturn %s;\n}\n", gen_name(s, port->por_references, true));
}

//____________________________________________________________
//
//		Substitute for a variable reference.
//

static void gen_type(const act* action)
{
	// CVC: If I'm not mistaken, assumes sizeof(long) == sizeof(ref*)
	printa("%s%ld", COLUMN, action->act_object);
}

#ifdef NOT_USED_OR_REPLACED
//____________________________________________________________
//
//		Print a fixed string at a particular column.
//

static void printb(const TEXT* string, ...)
{
	va_list ptr;

	va_start(ptr, string);
	vfprintf(gpreGlob.out_file, string, ptr);
	va_end(ptr);
}
#endif

