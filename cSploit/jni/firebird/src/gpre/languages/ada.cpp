//____________________________________________________________
//
//		PROGRAM:	ADA preprocesser
//		MODULE:		ada.cpp
//		DESCRIPTION:	Inserted text generator for ADA
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

#include "firebird.h"

#include <stdio.h>
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
#include "../gpre/pat.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/lang_proto.h"
#include "../gpre/pat_proto.h"
#include "../common/prett_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

static void	align (int);
static void	asgn_from (const act*, ref*, int);
static void	asgn_to (const act*, ref*, int);
static void	asgn_to_proc (ref*, int);
static void	gen_any (const act*, int);
static void	gen_at_end (const act*, int);
static void	gen_based (const act*, int);
static void	gen_blob_close (const act*, USHORT);
static void	gen_blob_end (const act*, USHORT);
static void	gen_blob_for (const act*, USHORT);
static void	gen_blob_open (const act*, USHORT);
static void	gen_blr (void*, SSHORT, const char*);
static void	gen_clear_handles(int);
static void	gen_compile (const act*, int);
static void	gen_create_database (const act*, int);
static int	gen_cursor_close(int);
static void	gen_cursor_init (const act*, int);
static void	gen_database(int);
static void	gen_ddl (const act*, int);
static void	gen_drop_database (const act*, int);
static void	gen_dyn_close (const act*, int);
static void	gen_dyn_declare (const act*, int);
static void	gen_dyn_describe(const act*, int, bool);
static void	gen_dyn_execute (const act*, int);
static void	gen_dyn_fetch (const act*, int);
static void	gen_dyn_immediate (const act*, int);
static void	gen_dyn_insert (const act*, int);
static void	gen_dyn_open (const act*, int);
static void	gen_dyn_prepare (const act*, int);
static void	gen_emodify (const act*, int);
static void	gen_estore (const act*, int);
static void	gen_endfor (const act*, int);
static void	gen_erase (const act*, int);
static SSHORT	gen_event_block (const act*);
static void	gen_event_init (const act*, int);
static void	gen_event_wait (const act*, int);
static void	gen_fetch (const act*, int);
static void	gen_finish (const act*, int);
static void	gen_for (const act*, int);
static void	gen_function (const act*, int);
static void	gen_get_or_put_slice(const act*, ref*, bool, int);
static void	gen_get_segment (const act*, int);
static void	gen_loop (const act*, int);
static TEXT* gen_name(TEXT* const, const ref*, bool);
static void	gen_on_error (const act*, USHORT);
static void	gen_procedure (const act*, int);
static void	gen_put_segment (const act*, int);
static void	gen_raw (const UCHAR*, const int);
static void	gen_ready (const act*, int);
static void	gen_receive (const act*, int, gpre_port*);
static void	gen_release (const act*, int);
static void	gen_request (gpre_req*, int);
static void	gen_return_value (const act*, int);
static void	gen_routine (const act*, int);
static void	gen_s_end (const act*, int);
static void	gen_s_fetch (const act*, int);
static void	gen_s_start (const act*, int);
static void	gen_segment (const act*, int);
static void	gen_select (const act*, int);
static void	gen_send (const act*, gpre_port*, int);
static void	gen_slice (const act*, int);
static void	gen_start (const act*, gpre_port*, int);
static void	gen_store (const act*, int);
static void	gen_t_start (const act*, int);
static void	gen_tpb (const tpb*, int);
static void	gen_trans (const act*, int);
static void	gen_type (const act*, int);
static void	gen_update (const act*, int);
static void	gen_variable (const act*, int);
static void	gen_whenever (const swe*, int);
static void	make_array_declaration (ref*, int);
static void	make_cursor_open_test (act_t, gpre_req*, int);
static TEXT* make_name (TEXT* const, const gpre_sym*);
static void	make_ok_test (const act*, gpre_req*, int);
static void	make_port (const gpre_port*, int);
static void	make_ready (const gpre_dbb*, const TEXT*, const TEXT*, USHORT, gpre_req*);
static void	printa (int, const TEXT*, ...);
static const TEXT* request_trans (const act*, const gpre_req*);
static const TEXT* status_vector (const act*);
static void	t_start_auto (const act*, const gpre_req*, const TEXT*, int, bool);

static TEXT output_buffer[512];
static bool first_flag = false;

static const char* const COMMENT = "--- ";
const int INDENT	= 3;

static const char* const BYTE_DCL			= "firebird.isc_byte";
static const char* const BYTE_VECTOR_DCL	= "firebird.isc_vector_byte";
static const char* const SHORT_DCL		= "firebird.isc_short";
static const char* const USHORT_DCL		= "firebird.isc_ushort";
static const char* const LONG_DCL			= "firebird.isc_long";
static const char* const LONG_VECTOR_DCL	= "firebird.isc_vector_long";
static const char* const EVENT_LIST_DCL	= "firebird.event_list";
static const char* const REAL_DCL			= "firebird.isc_float";
static const char* const DOUBLE_DCL		= "firebird.isc_double";

static inline void endif(const int column)
{
	printa(column, "end if;");
}

static inline void begin(const int column)
{
	printa(column, "begin");
}

static inline void endp(const int column)
{
	printa(column, "end");
}

static inline void set_sqlcode(const act* action, const int column)
{
	if (action->act_flags & ACT_sql)
		printa(column, "SQLCODE := firebird.sqlcode(isc_status);");
}

//____________________________________________________________
//
//		Code generator for ADA.  Not to be confused with
//
//

void ADA_action( const act* action, int column)
{

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
		gen_ddl(action, column);
		break;
	case ACT_any:
		gen_any(action, column);
		return;
	case ACT_at_end:
		gen_at_end(action, column);
		return;
	case ACT_commit:
		gen_trans(action, column);
		break;
	case ACT_commit_retain_context:
		gen_trans(action, column);
		break;
	case ACT_b_declare:
		gen_database(column);
		gen_routine(action, column);
		return;
	case ACT_basedon:
		gen_based(action, column);
		return;
	case ACT_blob_cancel:
		gen_blob_close(action, column);
		return;
	case ACT_blob_close:
		gen_blob_close(action, column);
		return;
	case ACT_blob_create:
		gen_blob_open(action, column);
		return;
	case ACT_blob_for:
		gen_blob_for(action, column);
		return;
	case ACT_blob_handle:
		gen_segment(action, column);
		return;
	case ACT_blob_open:
		gen_blob_open(action, column);
		return;
	case ACT_clear_handles:
		gen_clear_handles(column);
		break;
	case ACT_close:
		gen_s_end(action, column);
		break;
	case ACT_create_database:
		gen_create_database(action, column);
		break;
	case ACT_cursor:
		gen_cursor_init(action, column);
		return;
	case ACT_database:
		gen_database(column);
		return;
	case ACT_disconnect:
		gen_finish(action, column);
		break;
	case ACT_drop_database:
		gen_drop_database(action, column);
		break;
	case ACT_dyn_close:
		gen_dyn_close(action, column);
		break;
	case ACT_dyn_cursor:
		gen_dyn_declare(action, column);
		break;
	case ACT_dyn_describe:
		gen_dyn_describe(action, column, false);
		break;
	case ACT_dyn_describe_input:
		gen_dyn_describe(action, column, true);
		break;
	case ACT_dyn_execute:
		gen_dyn_execute(action, column);
		break;
	case ACT_dyn_fetch:
		gen_dyn_fetch(action, column);
		break;
	case ACT_dyn_grant:
		gen_ddl(action, column);
		break;
	case ACT_dyn_immediate:
		gen_dyn_immediate(action, column);
		break;
	case ACT_dyn_insert:
		gen_dyn_insert(action, column);
		break;
	case ACT_dyn_open:
		gen_dyn_open(action, column);
		break;
	case ACT_dyn_prepare:
		gen_dyn_prepare(action, column);
		break;
	case ACT_procedure:
		gen_procedure(action, column);
		break;
	case ACT_dyn_revoke:
		gen_ddl(action, column);
		break;
	case ACT_endblob:
		gen_blob_end(action, column);
		return;
	case ACT_enderror:
		endif(column);
		break;
	case ACT_endfor:
		gen_endfor(action, column);
		break;
	case ACT_endmodify:
		gen_emodify(action, column);
		break;
	case ACT_endstore:
		gen_estore(action, column);
		break;
	case ACT_erase:
		gen_erase(action, column);
		return;
	case ACT_event_init:
		gen_event_init(action, column);
		break;
	case ACT_event_wait:
		gen_event_wait(action, column);
		break;
	case ACT_fetch:
		gen_fetch(action, column);
		break;
	case ACT_finish:
		gen_finish(action, column);
		break;
	case ACT_for:
		gen_for(action, column);
		return;
	case ACT_function:
		gen_function(action, column);
		return;
	case ACT_get_segment:
		gen_get_segment(action, column);
		return;
	case ACT_get_slice:
		gen_slice(action, column);
		return;
	case ACT_hctef:
		endif(column);
		break;
	case ACT_insert:
		gen_s_start(action, column);
		break;
	case ACT_loop:
		gen_loop(action, column);
		break;
	case ACT_open:
		gen_s_start(action, column);
		break;
	case ACT_on_error:
		gen_on_error(action, column);
		return;
	case ACT_ready:
		gen_ready(action, column);
		break;
	case ACT_prepare:
		gen_trans(action, column);
		break;
	case ACT_put_segment:
		gen_put_segment(action, column);
		return;
	case ACT_put_slice:
		gen_slice(action, column);
		return;
	case ACT_release:
		gen_release(action, column);
		break;
	case ACT_rfinish:
		gen_finish(action, column);
		break;
	case ACT_rollback:
		gen_trans(action, column);
		break;
	case ACT_rollback_retain_context:
		gen_trans(action, column);
		break;
	case ACT_routine:
		gen_routine(action, column);
		return;
	case ACT_s_end:
		gen_s_end(action, column);
		return;
	case ACT_s_fetch:
		gen_s_fetch(action, column);
		return;
	case ACT_s_start:
		gen_s_start(action, column);
		break;
	case ACT_select:
		gen_select(action, column);
		break;
	case ACT_segment_length:
		gen_segment(action, column);
		return;
	case ACT_segment:
		gen_segment(action, column);
		return;
	case ACT_sql_dialect:
		gpreGlob.sw_sql_dialect = ((set_dialect*) action->act_object)->sdt_dialect;
		return;
	case ACT_start:
		gen_t_start(action, column);
		break;
	case ACT_store:
		gen_store(action, column);
		return;
	case ACT_store2:
		gen_return_value(action, column);
		return;
	case ACT_type_number:
		gen_type(action, column);
		return;
	case ACT_update:
		gen_update(action, column);
		break;
	case ACT_variable:
		gen_variable(action, column);
		return;
	default:
		return;
	}

	// Put in a trailing brace for those actions still with us

	if (action->act_flags & ACT_sql)
		gen_whenever(action->act_whenever, column);
}


//____________________________________________________________
//
//		Print a statment, breaking it into
//       reasonable 120 character hunks.
//

void ADA_print_buffer( TEXT* output_bufferL, const int column)
{
	TEXT s[121];
	bool multiline = false;

	TEXT* p = s;

	for (const TEXT* q = output_bufferL; *q; q++)
	{
		*p++ = *q;
		if ((p - s + column) > 119)
		{
			for (p--; (*p != ',') && (*p != ' '); p--)
				q--;
			*++p = 0;

			if (multiline)
			{
				for (int i = column / 8; i; --i)
					putc('\t', gpreGlob.out_file);

				for (int i = column % 8; i; --i)
					putc(' ', gpreGlob.out_file);
			}
			fprintf(gpreGlob.out_file, "%s\n", s);
			s[0] = 0;
			p = s;
			multiline = true;
		}
	}
	*p = 0;
	if (multiline)
	{
		for (int i = column / 8; i; --i)
			putc('\t', gpreGlob.out_file);
		for (int i = column % 8; i; --i)
			putc(' ', gpreGlob.out_file);
	}
	fprintf(gpreGlob.out_file, "%s", s);
	output_bufferL[0] = 0;
}


//____________________________________________________________
//
//		Align output to a specific column for output.
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


//____________________________________________________________
//
//		Build an assignment from a host language variable to
//		a port variable.
//

static void asgn_from( const act* action, ref* reference, int column)
{
	TEXT name[MAX_REF_SIZE], variable[MAX_REF_SIZE], temp[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_array_info)
			if (!(reference->ref_flags & REF_array_elem))
			{
				printa(column, "%s := firebird.null_blob;", gen_name(name, reference, true));
				gen_get_or_put_slice(action, reference, false, column);
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

		if (!reference->ref_master)
			printa(column, "%s := %s;", variable, value);
		else
		{
			printa(column, "if %s < 0 then", value);
			printa(column + INDENT, "%s := -1;", variable);
			printa(column, "else");
			printa(column + INDENT, "%s := 0;", variable);
			endif(column);
		}
	}
}

//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to( const act* action, ref* reference, int column)
{
	ref* source = reference->ref_friend;
	const gpre_fld* field = source->ref_field;

	if (field->fld_array_info)
	{
		source->ref_value = reference->ref_value;
		gen_get_or_put_slice(action, source, true, column);
		return;
	}

	//if (field && (field->fld_flags & FLD_text))
	//	printa (column, "firebird.isc_ftof (%s, %d, %s, sizeof (%s));",
	// gen_name (s, source, true),
	// field->fld_length,
	// reference->ref_value,
	// reference->ref_value);
	//else
	//
	TEXT s[MAX_REF_SIZE];
	printa(column, "%s := %s;", reference->ref_value, gen_name(s, source, true));

	// Pick up NULL value if one is there
	if (reference = reference->ref_null)
		printa(column, "%s := %s;", reference->ref_value, gen_name(s, reference, true));
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to_proc( ref* reference, int column)
{
	SCHAR s[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		if (!reference->ref_value)
			continue;
		printa(column, "%s := %s;", reference->ref_value, gen_name(s, reference, true));
	}
}


//____________________________________________________________
//
//       Generate a function call for free standing ANY.  Somebody else
//       will need to generate the actual function.
//

static void gen_any( const act* action, int column)
{
	align(column);
	const gpre_req* request = action->act_request;

	fprintf(gpreGlob.out_file, "%s_r (&%s, &%s",
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
//       Generate code for AT END clause of FETCH.
//

static void gen_at_end( const act* action, int column)
{
	SCHAR s[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	printa(column, "if %s = 0 then", gen_name(s, request->req_eof, true));
}


//____________________________________________________________
//
//		Substitute for a BASED ON <field name> clause.
//

static void gen_based( const act* action, int column)
{
	SSHORT datatype;
	SCHAR s[512], s2[128];

	bas* based_on = (bas*) action->act_object;
	gpre_fld* field = based_on->bas_field;
	TEXT* q = s;
	const TEXT* p;

	if (field->fld_array_info)
	{
		datatype = field->fld_array->fld_dtype;
		sprintf(s, "array (");
		for (q = s; *q; q++)
			;

		// Print out the dimension part of the declaration
		const dim* dimension = field->fld_array_info->ary_dimension;
		for (SSHORT i = 1; i < field->fld_array_info->ary_dimension_count;
			 dimension = dimension->dim_next, i++)
		{
			sprintf(s2, "%s range %"SLONGFORMAT"..%"SLONGFORMAT",\n                            ",
					LONG_DCL, dimension->dim_lower, dimension->dim_upper);
			for (p = s2; *p; p++, q++)
				*q = *p;
		}

		sprintf(s2, "%s range %"SLONGFORMAT"..%"SLONGFORMAT") of ",
				LONG_DCL, dimension->dim_lower, dimension->dim_upper);
		for (p = s2; *p; p++, q++)
			*q = *p;
	}
	else
		datatype = field->fld_dtype;

	SSHORT length;

	switch (datatype)
	{
	case dtype_short:
		sprintf(s2, "%s", SHORT_DCL);
		break;

	case dtype_long:
		sprintf(s2, "%s", LONG_DCL);
		break;

	case dtype_date:
	case dtype_blob:
	case dtype_quad:
		sprintf(s2, "firebird.quad");
		break;

	case dtype_text:
		length = field->fld_array_info ? field->fld_array->fld_length : field->fld_length;
		if (field->fld_sub_type == 1)
		{
			if (length == 1)
				sprintf(s2, "%s", BYTE_DCL);
			else
				sprintf(s2, "%s (1..%d)", BYTE_VECTOR_DCL, length);
		}
		else if (length == 1)
			sprintf(s2, "firebird.isc_character");
		else
			sprintf(s2, "string (1..%d)", length);
		break;

	case dtype_real:
		sprintf(s2, "%s", REAL_DCL);
		break;

	case dtype_double:
		sprintf(s2, "%s", DOUBLE_DCL);
		break;

	default:
		sprintf(s2, "datatype %d unknown\n", field->fld_dtype);
		return;
	}

	for (p = s2; *p; p++, q++)
		*q = *p;
	if (!strcmp(based_on->bas_terminator, ";"))
		*q++ = ';';
	*q = 0;
	printa(column, s);
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_close( const act* action, USHORT column)
{
	blb* blob;

	if (action->act_flags & ACT_sql)
	{
		column = gen_cursor_close(column);
		blob = (blb*) action->act_request->req_blobs;
	}
	else
		blob = (blb*) action->act_object;

	const TEXT* command = (action->act_type == ACT_blob_cancel) ? "CANCEL" : "CLOSE";
	printa(column, "firebird.%s_BLOB (%s isc_%d);", command, status_vector(action), blob->blb_ident);

	if (action->act_flags & ACT_sql)
	{
		endif(column);
		column -= INDENT;
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		End a blob FOR loop.
//

static void gen_blob_end( const act* action, USHORT column)
{
	const blb* blob = (blb*) action->act_object;
	printa(column, "end loop;");

	if (action->act_error)
		printa(column, "firebird.CLOSE_BLOB (isc_status2, isc_%d);", blob->blb_ident);
	else
		printa(column, "firebird.CLOSE_BLOB (%s isc_%d);", status_vector(0), blob->blb_ident);
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_for( const act* action, USHORT column)
{

	gen_blob_open(action, column);
	if (action->act_error)
		printa(column, "if (isc_status(1) = 0) then");
	printa(column, "loop");
	gen_get_segment(action, column + INDENT);
	printa(column + INDENT,
		   "exit when (isc_status(1) /= 0 and isc_status(1) /= firebird.isc_segment);");
}


#ifdef NOT_USED_OR_REPLACED
// This is the V4.0 version of the function prior to 18-January-95.
// for unknown reasons it contains a core dump.  The V3.3 version of
// this function appears to work fine, so we are using it instead.
// Ravi Kumar
//
//____________________________________________________________
//
//		Generate the call to open (or create) a blob.
//

static gen_blob_open( const act* action, USHORT column)
{
	TEXT s[MAX_REF_SIZE];
	const TEXT* pattern1 =
		"firebird.%IFCREATE%ELOPEN%EN_BLOB2 (%V1 %RF%DH, %RF%RT, %RF%BH, %RF%FR, %N1, %I1);";
	const TEXT* pattern2 =
		"firebird.%IFCREATE%ELOPEN%EN_BLOB2 (%V1 %RF%DH, %RF%RT, %RF%BH, %RF%FR, 0, isc_null_bpb);";

	ref* reference = 0;
	blb* blob;
	if (action->act_flags & ACT_sql)
	{
		column = gen_cursor_open(action, action->act_request, column);
		blob = (blb*) action->act_request->req_blobs;
		reference = ((open_cursor*) action->act_object)->opn_using;
		gen_name(s, reference, true);
	}
	else
		blob = (blb*) action->act_object;

	PAT args;
	args.pat_condition = (action->act_type == ACT_blob_create);	// open or create blob
	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = blob->blb_request->req_database;	// database handle
	args.pat_request = blob->blb_request;	// transaction handle
	args.pat_blob = blob;		// blob handle
	args.pat_reference = reference;	// blob identifier
	args.pat_ident1 = blob->blb_bpb_ident;

	if ((action->act_flags & ACT_sql) && action->act_type == ACT_blob_open)
		printa(column, "%s := %s;", s, reference->ref_value);

	if (args.pat_value1 = blob->blb_bpb_length)
		PATTERN_expand(column, pattern1, &args);
	else
		PATTERN_expand(column, pattern2, &args);

	if (action->act_flags & ACT_sql)
	{
		endif(column);
		column -= INDENT;
		endif(column);
		column -= INDENT;
		set_sqlcode(action, column);
		if (action->act_type == ACT_blob_create)
		{
			printa(column, "if SQLCODE = 0 then");
			printa(column + INDENT, "%s := %s;", reference->ref_value, s);
			endif(column);
		}
		endif(column);
	}
}
#else
// This is the version 3.3 of this routine - the original V4.0 version
// has a core drop.
// Ravi Kumar
// CVC: no surprise, since "reference" was left uninitialized in one execution path!
//
//____________________________________________________________
//
//       Generate the call to open (or create) a blob.
//

static void gen_blob_open( const act* action, USHORT column)
{
	const TEXT* pattern1 =
		"firebird.%IFCREATE%ELOPEN%EN_BLOB2 (%V1 %RF%DH, %RF%RT, %RF%BH, %RF%FR, %N1, %I1);";
	const TEXT* pattern2 =
		"firebird.%IFCREATE%ELOPEN%EN_BLOB2 (%V1 %RF%DH, %RF%RT, %RF%BH, %RF%FR, 0, isc_null_bpb);";

	blb* blob = (blb*) action->act_object;
	PAT args;
	args.pat_condition = (action->act_type == ACT_blob_create);	// open or create blob
	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = blob->blb_request->req_database;	// database handle
	args.pat_request = blob->blb_request;	// transaction handle
	args.pat_blob = blob;		// blob handle
	args.pat_reference = blob->blb_reference;	// blob identifier
	args.pat_ident1 = blob->blb_bpb_ident;

	if (args.pat_value1 = blob->blb_bpb_length)
		PATTERN_expand(column, pattern1, &args);
	else
		PATTERN_expand(column, pattern2, &args);
}
#endif


//____________________________________________________________
//
//		Callback routine for BLR pretty printer.
//

static void gen_blr(void* /*user_arg*/, SSHORT /*offset*/, const char* string)
{
	const int c_len = strlen(COMMENT);
	const int len = strlen(string);
	int from = 0;
	int to = 120 - c_len;

	while (from < len)
	{
		if (to < len)
		{
			char buffer[121];
			strncpy(buffer, string + from, 120 - c_len);
			buffer[120 - c_len] = 0;
			fprintf(gpreGlob.out_file, "%s%s\n", COMMENT, buffer);
		}
		else
			fprintf(gpreGlob.out_file, "%s%s\n", COMMENT, string + from);

		from = to;
		to = to + 120 - c_len;
	}
}


//____________________________________________________________
//
//       Zap all know handles.
//

static void gen_clear_handles(int column)
{
	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
	{
		if (!(request->req_flags & REQ_exp_hand))
			printa(column, "%s = 0;", request->req_handle);
	}
}


#ifdef NOT_USED_OR_REPLACED
//____________________________________________________________
//
//       Generate a symbol to ease compatibility with V3.
//

static void gen_compatibility_symbol(const TEXT* symbol,
									 const TEXT* v4_prefix, const TEXT* trailer)
{
	const TEXT* v3_prefix = isLangCpp(gpreGlob.sw_language) ? "gds_" : "gds__";

	fprintf(gpreGlob.out_file, "#define %s%s\t%s%s%s\n", v3_prefix, symbol,
			v4_prefix, symbol, trailer);
}
#endif


//____________________________________________________________
//
//		Generate text to compile a request.
//

static void gen_compile( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	column += INDENT;
	const gpre_dbb* db = request->req_database;
	const gpre_sym* symbol = db->dbb_name;

	if (gpreGlob.sw_auto)
		t_start_auto(action, request, status_vector(action), column, true);

	if ((action->act_error || (action->act_flags & ACT_sql)) && gpreGlob.sw_auto)
		printa(column, "if (%s = 0) and (%s%s /= 0) then", request->req_handle,
				gpreGlob.ada_package, request_trans(action, request));
	else
		printa(column, "if %s = 0 then", request->req_handle);

	column += INDENT;
#ifdef NOT_USED_OR_REPLACED
	printa(column, "firebird.COMPILE_REQUEST%s (%s %s%s, %s%s, %d, isc_%d);",
		   (request->req_flags & REQ_exp_hand) ? "" : "2",
		   status_vector(action),
		   gpreGlob.ada_package, symbol->sym_string,
		   request_trans(action, request),
		   (request->req_flags & REQ_exp_hand) ? "" : "'address",
		   request->req_length, request->req_ident);
#endif
	printa(column, "firebird.COMPILE_REQUEST%s (%s %s%s, %s%s, %d, isc_%d);",
		   (request->req_flags & REQ_exp_hand) ? "" : "2",
		   status_vector(action),
		   gpreGlob.ada_package, symbol->sym_string,
		   request->req_handle,
		   (request->req_flags & REQ_exp_hand) ? "" : "'address",
		   request->req_length, request->req_ident);

	set_sqlcode(action, column);
	column -= INDENT;
	endif(column);

	for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		printa(column - INDENT, "isc_%d := 0;", blob->blb_ident);
}


//____________________________________________________________
//
//		Generate a call to create a database.
//

static void gen_create_database( const act* action, int column)
{
	TEXT s1[32], s2[32];

	gpre_req* request = ((mdbb*) action->act_object)->mdbb_dpb_request;
	const gpre_dbb* db = (gpre_dbb*) request->req_database;

	sprintf(s1, "isc_%dl", request->req_ident);

	if (request->req_flags & REQ_extend_dpb)
	{
		sprintf(s2, "isc_%dp", request->req_ident);
		if (request->req_length)
			printa(column, "%s = isc_to_dpb_ptr (isc_%d'address);", s2, request->req_ident);
		if (db->dbb_r_user)
			printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_user_name, %s, %d);\n",
				   s2, s1, db->dbb_r_user, (strlen(db->dbb_r_user) - 2));
		if (db->dbb_r_password)
			printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_password, %s, %d);\n",
				   s2, s1, db->dbb_r_password,
				   (strlen(db->dbb_r_password) - 2));

		// Process Role Name
		if (db->dbb_r_sql_role)
			printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_sql_role_name, %s, %d);\n",
				   s2, s1, db->dbb_r_sql_role, (strlen(db->dbb_r_sql_role) - 2));

		if (db->dbb_r_lc_messages)
			printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_lc_messages, %s, %d);\n",
				   s2, s1, db->dbb_r_lc_messages, (strlen(db->dbb_r_lc_messages) - 2));
		if (db->dbb_r_lc_ctype)
			printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_lc_ctype, %s, %d);\n",
				   s2, s1, db->dbb_r_lc_ctype, (strlen(db->dbb_r_lc_ctype) - 2));

	}
	else
		sprintf(s2, "isc_to_dpb_ptr (isc_%d'address)", request->req_ident);

	if (request->req_length)
		printa(column, "firebird.CREATE_DATABASE (%s %d, \"%s\", %s%s, %s, %s, 0);",
			   status_vector(action), strlen(db->dbb_filename), db->dbb_filename,
			   gpreGlob.ada_package, db->dbb_name->sym_string, s1, s2);
	else
		printa(column, "firebird.CREATE_DATABASE (%s %d, \"%s\", %s%s, 0, firebird.null_dpb, 0);",
			   status_vector(action), strlen(db->dbb_filename), db->dbb_filename,
			   gpreGlob.ada_package, db->dbb_name->sym_string);

	if (request && request->req_flags & REQ_extend_dpb)
	{
		if (request->req_length)
			printa(column, "if (%s != isc_%d)", s2, request->req_ident);
		printa(column + (request->req_length ? 4 : 0), "firebird.FREE (%s);", s2);

		// reset the length of the dpb
		if (request->req_length)
			printa(column, "%s = %d;", s1, request->req_length);
	}

	printa(column, "if %sisc_status(1) = 0 then", gpreGlob.ada_package);
	column += INDENT;
	const bool save_sw_auto = gpreGlob.sw_auto;
	gpreGlob.sw_auto = true;
	gen_ddl(action, column);
	gpreGlob.sw_auto = save_sw_auto;
	column -= INDENT;
	printa(column, "else");
	column += INDENT;
	set_sqlcode(action, column);
	column -= INDENT;
	endif(column);
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static int gen_cursor_close(int column)
{
	return column;
}


//____________________________________________________________
//
//		Generate text to initialize a cursor.
//

static void gen_cursor_init( const act* action, int column)
{
	// If blobs are present, zero out all of the blob handles.  After this
	// int, the handles are the user's responsibility

	if (action->act_request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
	{
		printa(column, "gds_%d := 0;", action->act_request->req_blobs->blb_ident);
	}
}

#ifdef NOT_USED_OR_REPLACED
//____________________________________________________________
//
//		Generate text to open an embedded SQL cursor.
//

static int gen_cursor_open( const act* action, gpre_req* request, int column)
{
	return column;
}
#endif

//____________________________________________________________
//
//		Generate insertion text for the database statement,
//		including the definitions of all gpreGlob.requests, and blob
//		and port declarations for gpreGlob.requests in the main routine.
//

static void gen_database(int column)
{
	if (first_flag)
		return;
	first_flag = true;

	fprintf(gpreGlob.out_file, "\n----- GPRE Preprocessor Definitions -----\n");

	gpre_req* request;
	for (request = gpreGlob.requests; request; request = request->req_next)
	{
		if (request->req_flags & REQ_local)
			continue;
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			make_port(port, column);
	}
	fprintf(gpreGlob.out_file, "\n");
	for (request = gpreGlob.requests; request; request = request->req_routine)
	{
		if (request->req_flags & REQ_local)
			continue;
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			printa(column, "isc_%d\t: isc_%dt;\t\t\t-- message --", port->por_ident, port->por_ident);

		for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			printa(column, "isc_%d\t: firebird.blob_handle;\t-- blob handle --", blob->blb_ident);
			SSHORT blob_subtype = blob->blb_const_to_type;
			if (!blob_subtype)
			{
				const ref* reference = blob->blb_reference;
				if (reference)
				{
					const gpre_fld* field = reference->ref_field;
					if (field)
						blob_subtype = field->fld_sub_type;
				}
			}
			if (blob_subtype != 0 && blob_subtype != 1)
				printa(column, "isc_%d\t: %s (1 .. %d);\t\t-- blob segment --",
					   blob->blb_buff_ident, BYTE_VECTOR_DCL, blob->blb_seg_length);
			else
				printa(column, "isc_%d\t: string (1 .. %d);\t\t-- blob segment --",
					   blob->blb_buff_ident, blob->blb_seg_length);
			printa(column, "isc_%d\t: %s;\t\t-- segment length --", blob->blb_len_ident, USHORT_DCL);
		}
	}

	// generate event parameter block for each event in module

	USHORT max_count = 0;
	for (gpre_lls* stack_ptr = gpreGlob.events; stack_ptr; stack_ptr = stack_ptr->lls_next)
	{
		const USHORT event_count = gen_event_block((const act*) stack_ptr->lls_object);
		max_count = MAX(event_count, max_count);
	}

	if (max_count)
		printa(column, "isc_events: %s(1..%d);\t-- event vector --", EVENT_LIST_DCL, max_count);

	bool array_flag = false;
	for (request = gpreGlob.requests; request; request = request->req_next)
	{
		gen_request(request, column);

		// Array declarations
		const gpre_port* port = request->req_primary;
		if (port)
			for (ref* reference = port->por_references; reference; reference = reference->ref_next)
			{
				if (reference->ref_flags & REF_fetch_array)
				{
					make_array_declaration(reference, column);
					array_flag = true;
				}
			}
	}
	if (array_flag)
	{
		printa(column, "isc_array_length\t: %s;\t-- slice return value --", LONG_DCL);
		printa(column, "isc_null_vector_l\t: %s (1..1);\t-- null long vector --", LONG_VECTOR_DCL);
	}

	printa(column, "isc_null_bpb\t: firebird.blr (1..1);\t-- null blob parameter block --");

	if (!gpreGlob.ada_package[0])
		printa(column, "gds_trans\t: firebird.transaction_handle := 0;\t-- default transaction --");
	printa(column, "isc_status\t: firebird.status_vector;\t-- status vector --");
	printa(column, "isc_status2\t: firebird.status_vector;\t-- status vector --");
	printa(column, "SQLCODE\t: %s := 0;\t\t\t-- SQL status code --", LONG_DCL);
	if (!gpreGlob.ada_package[0])
	{
		for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
			printa(column, "%s\t\t: firebird.database_handle%s;-- database handle --",
				   db->dbb_name->sym_string, (db->dbb_scope == DBB_EXTERN) ? "" : " := 0");
	}

	printa(column, " ");

	USHORT count = 0;
	for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		++count;
		for (const tpb* tpb_val = db->dbb_tpbs; tpb_val; tpb_val = tpb_val->tpb_dbb_next)
			gen_tpb(tpb_val, column + INDENT);
	}

	printa(column, "isc_teb\t: array (1..%d) of firebird.isc_teb_t;\t-- transaction vector ", count);

	printa(column, "----- end of GPRE definitions ");
}


//____________________________________________________________
//
//		Generate a call to update metadata.
//

static void gen_ddl( const act* action, int column)
{
	// Set up command type for call to RDB$DDL

	const gpre_req* request = action->act_request;

	// Generate a call to RDB$DDL to perform action

	if (gpreGlob.sw_auto)
	{
		t_start_auto(action, 0, status_vector(action), column, true);
		printa(column, "if %sgds_trans /= 0 then", gpreGlob.ada_package);
		column += INDENT;
	}

	printa(column, "firebird.DDL (%s %s%s, %s%s, %d, isc_%d'address);",
		   status_vector(action),
		   gpreGlob.ada_package, request->req_database->dbb_name->sym_string,
		   gpreGlob.ada_package, request->req_trans,
		   request->req_length, request->req_ident);

	if (gpreGlob.sw_auto)
	{
		printa(column, "if %sisc_status(1) = 0 then", gpreGlob.ada_package);
		printa(column + INDENT, "firebird.commit_TRANSACTION (%s %sgds_trans);",
			   status_vector(action), gpreGlob.ada_package);
		printa(column, "else", gpreGlob.ada_package);
		printa(column + INDENT, "firebird.rollback_TRANSACTION (%sgds_trans);", gpreGlob.ada_package);
		endif(column);
		column -= INDENT;
		endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a call to create a database.
//

static void gen_drop_database( const act* action, int column)
{
	const gpre_dbb* db = (gpre_dbb*) action->act_object;
	//gpre_req* request = action->act_request;

	printa(column, "firebird.DROP_DATABASE (%s %d, \"%s\", RDBK_DB_TYPE_GDS);",
		   status_vector(action), strlen(db->dbb_filename), db->dbb_filename);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_close( const act* action, int column)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	printa(column, "firebird.embed_dsql_close (%s %s);", status_vector(action),
		   make_name(s, statement->dyn_cursor_name));
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_declare( const act* action, int column)
{
	TEXT s1[MAX_CURSOR_SIZE], s2[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	printa(column, "firebird.embed_dsql_declare (%s %s, %s);", status_vector(action),
		   make_name(s1, statement->dyn_statement_name), make_name(s2, statement->dyn_cursor_name));
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_describe(const act* action,
							 int column,
							 bool input_flag)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	printa(column, "firebird.embed_dsql_describe%s (%s %s, %d, %s'address);",
		   input_flag ? "_bind" : "", status_vector(action),
		   make_name(s, statement->dyn_statement_name), gpreGlob.sw_sql_dialect, statement->dyn_sqlda);
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_execute( const act* action, int column)
{
	gpre_req* request;
	gpre_req req_const;

	dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds_trans";
		request = NULL;
	}

	if (gpreGlob.sw_auto)
	{
		t_start_auto(action, request, status_vector(action), column, true);
		printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, transaction);
		column += INDENT;
	}

	TEXT s[MAX_CURSOR_SIZE];
	if (statement->dyn_sqlda2)
		printa(column, "firebird.embed_dsql_execute2 (isc_status, %s%s, %s, %d, %s'address);",
			   gpreGlob.ada_package, transaction, make_name(s, statement->dyn_statement_name),
			   gpreGlob.sw_sql_dialect,
			   statement->dyn_sqlda ? gpreGlob.ada_null_address : statement->dyn_sqlda,
			   statement->dyn_sqlda2);
	else if (statement->dyn_sqlda)
		printa(column, "firebird.embed_dsql_execute (isc_status, %s%s, %s, %d, %s'address);",
			   gpreGlob.ada_package, transaction, make_name(s, statement->dyn_statement_name),
			   gpreGlob.sw_sql_dialect, statement->dyn_sqlda);
	else
		printa(column, "firebird.embed_dsql_execute (isc_status, %s%s, %s, %d);",
			   gpreGlob.ada_package, transaction, make_name(s, statement->dyn_statement_name),
			   gpreGlob.sw_sql_dialect);

	if (gpreGlob.sw_auto)
	{
		column -= INDENT;
		endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_fetch( const act* action, int column)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	if (statement->dyn_sqlda)
		printa(column, "firebird.embed_dsql_fetch (isc_status, SQLCODE, %s, %d, %s'address);",
			   make_name(s, statement->dyn_cursor_name), gpreGlob.sw_sql_dialect,
			   statement->dyn_sqlda);
	else
		printa(column, "firebird.embed_dsql_fetch (isc_status, SQLCODE, %s, %d);",
			   make_name(s, statement->dyn_cursor_name), gpreGlob.sw_sql_dialect);

	printa(column, "if SQLCODE /= 100 then");
	printa(column + INDENT, "SQLCODE := firebird.sqlcode (isc_status);");
	endif(column);
}


//____________________________________________________________
//
//		Generate code for an EXECUTE IMMEDIATE dynamic SQL statement.
//

static void gen_dyn_immediate( const act* action, int column)
{
	gpre_req* request;
	gpre_req req_const;

	dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds_trans";
		request = NULL;
	}

	const gpre_dbb* database = statement->dyn_database;

	if (gpreGlob.sw_auto)
	{
		t_start_auto(action, request, status_vector(action), column, true);
		printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, transaction);
		column += INDENT;
	}

	printa(column, "firebird.embed_dsql_execute_immed (isc_status, %s%s, %s%s, %s'length(1), %s, %d);",
		   gpreGlob.ada_package, database->dbb_name->sym_string, gpreGlob.ada_package, transaction,
		   statement->dyn_string, statement->dyn_string, gpreGlob.sw_sql_dialect);

	if (gpreGlob.sw_auto)
	{
		column -= INDENT;
		endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_insert( const act* action, int column)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	if (statement->dyn_sqlda)
		printa(column, "firebird.embed_dsql_insert (isc_status, %s, %d, %s'address);",
			   make_name(s, statement->dyn_cursor_name),
			   gpreGlob.sw_sql_dialect, statement->dyn_sqlda);
	else
		printa(column, "firebird.embed_dsql_insert (isc_status, %s, %d);",
			   make_name(s, statement->dyn_cursor_name), gpreGlob.sw_sql_dialect);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_open( const act* action, int column)
{
	gpre_req* request;
	gpre_req req_const;

	dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds_trans";
		request = NULL;
	}

	TEXT s[MAX_CURSOR_SIZE];
	make_name(s, statement->dyn_cursor_name);

	if (gpreGlob.sw_auto)
	{
		t_start_auto(action, request, status_vector(action), column, true);
		printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, transaction);
		column += INDENT;
	}

	if (statement->dyn_sqlda)
		printa(column, "firebird.embed_dsql_open (isc_status, %s%s, %s, %d, %s'address);",
			   gpreGlob.ada_package, transaction, s, gpreGlob.sw_sql_dialect, statement->dyn_sqlda);
	else
		printa(column, "firebird.embed_dsql_open (isc_status, %s%s, %s, %d);",
			   gpreGlob.ada_package, transaction, s, gpreGlob.sw_sql_dialect);

	if (gpreGlob.sw_auto)
	{
		column -= INDENT;
		endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_prepare( const act* action, int column)
{
	gpre_req* request;
	gpre_req req_const;

	dyn* statement = (dyn*) action->act_object;
	const gpre_dbb* database = statement->dyn_database;

	const TEXT* transaction;
	if (statement->dyn_trans)
	{
		transaction = statement->dyn_trans;
		request = &req_const;
		request->req_trans = transaction;
	}
	else
	{
		transaction = "gds_trans";
		request = NULL;
	}

	if (gpreGlob.sw_auto)
	{
		t_start_auto(action, request, status_vector(action), column, true);
		printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, transaction);
		column += INDENT;
	}

	TEXT s[MAX_CURSOR_SIZE];
	if (statement->dyn_sqlda)
		printa(column,
			   "firebird.embed_dsql_prepare (isc_status, %s%s, %s%s, %s, %s'length(1), %s, %d, %s'address);",
			   gpreGlob.ada_package, database->dbb_name->sym_string, gpreGlob.ada_package,
			   transaction, make_name(s, statement->dyn_statement_name),
			   statement->dyn_string, statement->dyn_string, gpreGlob.sw_sql_dialect,
			   statement->dyn_sqlda);
	else
		printa(column,
			   "firebird.embed_dsql_prepare (isc_status, %s%s, %s%s, %s, %s'length(1), %s, %d);",
			   gpreGlob.ada_package, database->dbb_name->sym_string, gpreGlob.ada_package,
			   transaction, make_name(s, statement->dyn_statement_name),
			   statement->dyn_string, statement->dyn_string, gpreGlob.sw_sql_dialect);

	if (gpreGlob.sw_auto)
	{
		column -= INDENT;
		endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for END_MODIFY.
//

static void gen_emodify( const act* action, int column)
{
	TEXT s1[MAX_REF_SIZE], s2[MAX_REF_SIZE];

	upd* modify = (upd*) action->act_object;

	for (ref* reference = modify->upd_port->por_references; reference; reference = reference->ref_next)
	{
		ref* source = reference->ref_source;
		if (!source)
			continue;
		const gpre_fld* field = reference->ref_field;
		printa(column, "%s := %s;", gen_name(s1, reference, true), gen_name(s2, source, true));
		if (field->fld_array_info)
			gen_get_or_put_slice(action, reference, false, column);
	}

	gen_send(action, modify->upd_port, column);
}


//____________________________________________________________
//
//		Generate substitution text for END_STORE.
//

static void gen_estore( const act* action, int column)
{
	gpre_req* request = action->act_request;

	// if we did a store ... returning_values aka store2
	// just wrap up any dangling error handling
	if (request->req_type == REQ_store2)
	{
		if (action->act_error)
			endif(column);
		return;
	}

	gen_start(action, request->req_primary, column);
	if (action->act_error)
		endif(column);
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_endfor( const act* action, int column)
{
	gpre_req* request = action->act_request;

	if (request->req_sync)
		gen_send(action, request->req_sync, column + INDENT);

	printa(column, "end loop;");

	if (action->act_error || (action->act_flags & ACT_sql))
		endif(column);
}


//____________________________________________________________
//
//		Generate substitution text for ERASE.
//

static void gen_erase( const act* action, int column)
{
	upd* erase = (upd*) action->act_object;
	gen_send(action, erase->upd_port, column);
}


//____________________________________________________________
//
//		Generate event parameter blocks for use
//		with a particular call to isc_event_wait.
//		For languages too dim to deal with variable
//		arg lists, set up a vector for the event names.
//

static SSHORT gen_event_block( const act* action)
{
	gpre_nod* init = (gpre_nod*) action->act_object;
	//gpre_sym* event_name = (gpre_sym*) init->nod_arg[0];

	const int ident = CMP_next_ident();
	init->nod_arg[2] = (gpre_nod*)(IPTR) ident;
	const gpre_nod* list = init->nod_arg[1];

	printa(0, "isc_%da\t\t: system.address;\t\t-- event parameter block --\n", ident);
	printa(0, "isc_%db\t\t: system.address;\t\t-- result parameter block --\n", ident);
	printa(0, "isc_%dc\t\t: %s;\t\t-- count of events --\n", ident, USHORT_DCL);
	printa(0, "isc_%dl\t\t: %s;\t\t-- length of event parameter block --\n", ident, USHORT_DCL);
	printa(0, "isc_%dv\t\t: firebird.isc_el_t (1..%d);\t\t-- vector for initialization --\n",
		   ident, list->nod_count);

	return list->nod_count;
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_INIT.
//

static void gen_event_init( const act* action, int column)
{
	const TEXT* pattern1 =
		"isc_%N1l := firebird.event_block (%RFisc_%N1a'address, %RFisc_%N1b'address, isc_%N1c, %RFisc_%N1v);";
	const TEXT *pattern2 =
		"firebird.event_wait (%V1 %RF%DH, isc_%N1l, isc_%N1a, isc_%N1b);";
	const TEXT* pattern3 =
		"firebird.event_counts (isc_events, isc_%N1l, isc_%N1a, isc_%N1b);";

	if (action->act_error)
		begin(column);
	begin(column);

	gpre_nod* init = (gpre_nod*) action->act_object;
	gpre_nod* event_list = init->nod_arg[1];

	PAT args;
	args.pat_database = (gpre_dbb*) init->nod_arg[3];
	args.pat_vector1 = status_vector(action);
	args.pat_value1 = (int)(IPTR) init->nod_arg[2];

	// generate call to dynamically generate event blocks

	TEXT variable[MAX_REF_SIZE];
	USHORT count = 0;
	gpre_nod** ptr = event_list->nod_arg;
	for (gpre_nod** const end = ptr + event_list->nod_count; ptr < end; ++ptr)
	{
		count++;
		const gpre_nod* node = *ptr;
		if (node->nod_type == nod_field)
		{
			const ref* reference = (ref*) node->nod_arg[0];
			gen_name(variable, reference, true);
			printa(column, "isc_%dv(%d) := %s'address;", args.pat_value1, count, variable);
		}
		else
			printa(column, "isc_%dv(%d) := %s'address;", args.pat_value1, count, node->nod_arg[0]);
	}

	printa(column, "isc_%dc := %d;", args.pat_value1, (int) event_list->nod_count);
	PATTERN_expand(column, pattern1, &args);

	// generate actual call to event_wait

	PATTERN_expand(column, pattern2, &args);

	// get change in event counts, copying event parameter block for reuse

	PATTERN_expand(column, pattern3, &args);

	endp(column);
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_WAIT.
//

static void gen_event_wait( const act* action, int column)
{
	const TEXT* pattern1 = "firebird.event_wait (%V1 %RF%DH, isc_%N1l, isc_%N1a, isc_%N1b);";
	const TEXT* pattern2 = "firebird.event_counts (isc_events, isc_%N1l, isc_%N1a, isc_%N1b);";

	if (action->act_error)
		begin(column);
	begin(column);

	gpre_sym* event_name = (gpre_sym*) action->act_object;

	// go through the stack of gpreGlob.events, checking to see if the
	// event has been initialized and getting the event identifier

	int ident = -1;
	const gpre_dbb* database = NULL;
	for (gpre_lls* stack_ptr = gpreGlob.events; stack_ptr; stack_ptr = stack_ptr->lls_next)
	{
		const act* event_action = (const act*) stack_ptr->lls_object;
		gpre_nod* event_init = (gpre_nod*) event_action->act_object;
		gpre_sym* stack_name = (gpre_sym*) event_init->nod_arg[0];
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
	args.pat_vector1 = status_vector(action);
	args.pat_value1 = ident;

	// generate calls to wait on the event and to fill out the gpreGlob.events array

	PATTERN_expand(column, pattern1, &args);
	PATTERN_expand(column, pattern2, &args);

	endp(column);
	set_sqlcode(action, column);
}



//____________________________________________________________
//
//		Generate replacement text for the SQL FETCH statement.  The
//		epilog FETCH statement is handled by GEN_S_FETCH (generate
//		stream fetch).
//

static void gen_fetch( const act* action, int column)
{
	gpre_req* request = action->act_request;

	if (request->req_sync)
	{
		gen_send(action, request->req_sync, column);
		printa(column, "if SQLCODE = 0 then");
		column += INDENT;
	}

	gen_receive(action, column, request->req_primary);
	printa(column, "if SQLCODE = 0 then");
	column += INDENT;

	SCHAR s[20];
	printa(column, "if %s /= 0 then", gen_name(s, request->req_eof, true));
	column += INDENT;

	gpre_nod* var_list = (gpre_nod*) action->act_object;
	if (var_list)
	{
		for (int i = 0; i < var_list->nod_count; i++)
			asgn_to(action, (ref*) var_list->nod_arg[i], column);
	}
	else
	{
		// No WITH clause on the FETCH, so no assignments generated;  fix
		// for bug#940.  mao 6/20/89
		printa(column, "NULL;");
	}

	column -= INDENT;
	printa(column, "else");
	printa(column + INDENT, "SQLCODE := 100;");
	endif(column);
	column -= INDENT;
	endif(column);
	column -= INDENT;

	if (request->req_sync)
		endif(column);
}


//____________________________________________________________
//
//		Generate substitution text for FINISH.
//

static void gen_finish( const act* action, int column)
{
	const gpre_dbb* db = NULL;

	if (gpreGlob.sw_auto || ((action->act_flags & ACT_sql) && (action->act_type != ACT_disconnect)))
	{
		printa(column, "if %sgds_trans /= 0 then", gpreGlob.ada_package);
		printa(column + INDENT, "firebird.%s_TRANSACTION (%s %sgds_trans);",
			   (action->act_type != ACT_rfinish) ? "COMMIT" : "ROLLBACK",
			   status_vector(action), gpreGlob.ada_package);
		endif(column);
	}

	// the user supplied one or more db_handles

	for (const rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
	{
		db = ready->rdy_database;
		if (action->act_error || (action->act_flags & ACT_sql))
			printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, db->dbb_name->sym_string);
		printa(column, "firebird.DETACH_DATABASE (%s %s%s);",
			   status_vector(action), gpreGlob.ada_package, db->dbb_name->sym_string);
		if (action->act_error || (action->act_flags & ACT_sql))
			endif(column);
	}

	// no hanbdles, so we finish all known databases

	if (!db)
	{
		if (action->act_error || (action->act_flags & ACT_sql))
			printa(column, "if %sgds_trans = 0 then", gpreGlob.ada_package);
		for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			if ((action->act_error || (action->act_flags & ACT_sql)) && (db != gpreGlob.isc_databases))
			{
				printa(column, "if %s%s /= 0 and isc_status(1) = 0 then",
						 gpreGlob.ada_package, db->dbb_name->sym_string);
			}
			else
				printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, db->dbb_name->sym_string);
			column += INDENT;
			printa(column, "firebird.DETACH_DATABASE (%s %s%s);", status_vector(action),
				   gpreGlob.ada_package, db->dbb_name->sym_string);
			for (gpre_req* request = gpreGlob.requests; request; request = request->req_next)
			{
				if (!(request->req_flags & REQ_exp_hand) && request->req_type != REQ_slice &&
					request->req_type != REQ_procedure && request->req_database == db)
				{
					printa(column, "%s := 0;", request->req_handle);
				}
			}
			column -= INDENT;
			endif(column);
		}
		if (action->act_error || (action->act_flags & ACT_sql))
			endif(column);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for FOR statement.
//

static void gen_for( const act* action, int column)
{
	gen_s_start(action, column);
	gpre_req* request = action->act_request;

	if (action->act_error || (action->act_flags & ACT_sql))
		printa(column, "if isc_status(1) = 0 then");

	printa(column, "loop");
	column += INDENT;
	gen_receive(action, column, request->req_primary);

	TEXT s[MAX_REF_SIZE];
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(column, "exit when (%s = 0) or (isc_status(1) /= 0);",
			   gen_name(s, request->req_eof, true));
	else
		printa(column, "exit when %s = 0;", gen_name(s, request->req_eof, true));

	gpre_port* port = action->act_request->req_primary;
	if (port)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field->fld_array_info)
				gen_get_or_put_slice(action, reference, true, column);
		}
	}
}


//____________________________________________________________
//
//       Generate a function for free standing ANY or statistical.
//

static void gen_function( const act* function, int column)
{
	const act* action = (const act*) function->act_object;

	if (action->act_type != ACT_any)
	{
		CPR_error("can't generate function");
		return;
	}

	gpre_req* request = action->act_request;

	fprintf(gpreGlob.out_file, "static %s_r (request, transaction", request->req_handle);

	TEXT s[MAX_REF_SIZE];
	const gpre_port* port = request->req_vport;
	if (port)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			fprintf(gpreGlob.out_file, ", %s", gen_name(s, reference->ref_source, true));
		}
	}
	fprintf(gpreGlob.out_file, ")\n    isc_req_handle\trequest;\n    isc_tr_handle\ttransaction;\n");

	if (port)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			const gpre_fld* field = reference->ref_field;
			const TEXT* dtype;

			switch (field->fld_dtype)
			{
			case dtype_short:
				dtype = "short";
				break;

			case dtype_long:
				dtype = "long";
				break;

			case dtype_cstring:
			case dtype_text:
				dtype = "char*";
				break;

			case dtype_quad:
				dtype = "long";
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
		make_port(port, column);

	fprintf(gpreGlob.out_file, "\n\n");
	gen_s_start(action, 0);
	gen_receive(action, column, request->req_primary);

	for (port = request->req_ports; port; port = port->por_next)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field->fld_array_info)
				gen_get_or_put_slice(action, reference, true, column);
		}
	}

	port = request->req_primary;
	fprintf(gpreGlob.out_file, "\nreturn %s;\n}\n", gen_name(s, port->por_references, true));
}

//____________________________________________________________
//
//		Generate a call to isc_get_slice
//       or isc_put_slice for an array.
//

static void gen_get_or_put_slice(const act* action,
								 ref* reference,
								 bool get,
								 int column)
{
	const TEXT* pattern1 =
		"firebird.GET_SLICE (%V1 %RF%DH%RE, %RF%S1%RE, %S2, %N1, %S3, %N2, %S4, %L1, %S5, %S6);";
	const TEXT* pattern2 =
		"firebird.PUT_SLICE (%V1 %RF%DH%RE, %RF%S1%RE, %S2, %N1, %S3, %N2, %S4, %L1, %S5);";

	if (!(reference->ref_flags & REF_fetch_array))
		return;

	PAT args;
	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = action->act_request->req_database;	// database handle
	args.pat_string1 = action->act_request->req_trans;	// transaction handle

	TEXT s1[MAX_REF_SIZE];
	gen_name(s1, reference, true);	// blob handle
	args.pat_string2 = s1;

	args.pat_value1 = reference->ref_sdl_length;	// slice descr. length

	TEXT s2[25];
	sprintf(s2, "isc_%d", reference->ref_sdl_ident);	// slice description
	args.pat_string3 = s2;

	args.pat_value2 = 0;		// parameter length

	TEXT s3[25];
	sprintf(s3, "isc_null_vector_l");	// parameter block init
	args.pat_string4 = s3;

	args.pat_long1 = reference->ref_field->fld_array_info->ary_size;
	// slice size
	TEXT s4[25];
	if (action->act_flags & ACT_sql) {
		sprintf(s4, "%s'address", reference->ref_value);
	}
	else {
		sprintf(s4, "isc_%d'address", reference->ref_field->fld_array_info->ary_ident);
	}
	args.pat_string5 = s4;		// array name

	args.pat_string6 = "isc_array_length";	// return length

	if (get)
		PATTERN_expand(column, pattern1, &args);
	else
		PATTERN_expand(column, pattern2, &args);
}


//____________________________________________________________
//
//		Generate the code to do a get segment.
//

static void gen_get_segment( const act* action, int column)
{
	blb* blob;
	if (action->act_flags & ACT_sql)
		blob = (blb*) action->act_request->req_blobs;
	else
		blob = (blb*) action->act_object;

	if (action->act_error || (action->act_flags & ACT_sql))
		printa(column, "firebird.ISC_GET_SEGMENT (isc_status, isc_%d, isc_%d, %d, isc_%d'address);",
			   blob->blb_ident,
			   blob->blb_len_ident,
			   blob->blb_seg_length, blob->blb_buff_ident);
	else
		printa(column, "firebird.ISC_GET_SEGMENT (isc_status(1), isc_%d, isc_%d, %d, isc_%d'address);",
			   blob->blb_ident,
			   blob->blb_len_ident,
			   blob->blb_seg_length, blob->blb_buff_ident);

	if (action->act_flags & ACT_sql)
	{
		const ref* into = action->act_object;
		set_sqlcode(action, column);
		printa(column, "if (SQLCODE = 0) or (SQLCODE = 101) then");
		printa(column + INDENT, "%s := isc_%d;", into->ref_value, blob->blb_buff_ident);
		if (into->ref_null_value)
			printa(column + INDENT, "%s := isc_%d;", into->ref_null_value, blob->blb_len_ident);
		endif(column);
	}
}


//____________________________________________________________
//
//		Generate code to drive a mass update.  Just start a request
//		and get the result.
//

static void gen_loop( const act* action, int column)
{
	gen_s_start(action, column);
	gpre_req* request = action->act_request;
	gpre_port* port = request->req_primary;
	printa(column, "if SQLCODE = 0 then");
	column += INDENT;
	gen_receive(action, column, port);

	TEXT name[MAX_REF_SIZE];
	gen_name(name, port->por_references, true);
	printa(column, "if (SQLCODE = 0) and (%s = 0) then", name);
	printa(column + INDENT, "SQLCODE := 100;");
	endif(column);
	column -= INDENT;
	endif(column);
}


//____________________________________________________________
//
//		Generate a name for a reference.  Name is constructed from
//		port and parameter idents.
//

static TEXT* gen_name(TEXT* const string, const ref* reference, bool as_blob)
{
	if (reference->ref_field->fld_array_info && !as_blob)
		fb_utils::snprintf(string, MAX_REF_SIZE, "isc_%d",
						   reference->ref_field->fld_array_info->ary_ident);
	else
		fb_utils::snprintf(string, MAX_REF_SIZE, "isc_%d.isc_%d",
						   reference->ref_port->por_ident, reference->ref_ident);

	return string;
}


//____________________________________________________________
//
//		Generate a block to handle errors.
//

static void gen_on_error( const act* action, USHORT column)
{
	const act* err_action = (const act*) action->act_object;
	switch (err_action->act_type)
	{
	case ACT_get_segment:
	case ACT_put_segment:
	case ACT_endblob:
		printa(column,
			   "if (isc_status (1) /= 0) and (isc_status(1) /= firebird.isc_segment) and (isc_status(1) /= firebird.isc_segstr_eof) then");
		break;
	default:
		printa(column, "if (isc_status (1) /= 0) then");
	}
}


//____________________________________________________________
//
//		Generate code for an EXECUTE PROCEDURE.
//

static void gen_procedure( const act* action, int column)
{
	gpre_req* request = action->act_request;
	gpre_port* in_port = request->req_vport;
	gpre_port* out_port = request->req_primary;

	const gpre_dbb* database = request->req_database;
	PAT args;
	args.pat_database = database;
	args.pat_request = action->act_request;
	args.pat_vector1 = status_vector(action);
	args.pat_string2 = gpreGlob.ada_null_address;
	args.pat_request = request;
	args.pat_port = in_port;
	args.pat_port2 = out_port;

	const TEXT* pattern;
	if (in_port && in_port->por_length)
		pattern =
			"firebird.gds_transact_request (%V1 %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RF%RI%RE, %VF%PL%VE, %PI'address, %VF%QL%VE, %QI'address);\n";
	else
		pattern =
			"firebird.gds_transact_request (%V1 %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RF%RI%RE, %VF0%VE, %S2, %VF%QL%VE, %QI'address);\n";

	// Get database attach and transaction started

	if (gpreGlob.sw_auto)
		t_start_auto(action, 0, status_vector(action), column, true);

	// Move in input values

	asgn_from(action, request->req_values, column);

	// Execute the procedure

	PATTERN_expand(column, pattern, &args);

	set_sqlcode(action, column);

	printa(column, "if SQLCODE = 0 then");

	// Move out output values

	asgn_to_proc(request->req_references, column);
	endif(column);
}


//____________________________________________________________
//
//		Generate the code to do a put segment.
//

static void gen_put_segment( const act* action, int column)
{
	const blb* blob;
	if (action->act_flags & ACT_sql)
	{
		blob = (blb*) action->act_request->req_blobs;
		const ref* from = action->act_object;
		printa(column, "isc_%d := %s;", blob->blb_len_ident, from->ref_null_value);
		printa(column, "isc_%d := %s;", blob->blb_buff_ident, from->ref_value);
	}
	else
		blob = (blb*) action->act_object;

	printa(column, "firebird.ISC_PUT_SEGMENT (%s isc_%d, isc_%d, isc_%d'address);",
		   status_vector(action), blob->blb_ident, blob->blb_len_ident, blob->blb_buff_ident);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate BLR in raw, numeric form.  Ugly but dense.
//

static void gen_raw(const UCHAR* blr, const int request_length)
{
	TEXT buffer[80];

	TEXT* p = buffer;
	const TEXT* const limit = buffer + 60;

	for (const UCHAR* const end = blr + request_length - 1; blr <= end; ++blr)
	{
		const UCHAR c = *blr;
		sprintf(p, "%d", c);
		while (*p)
			p++;
		if (blr != end)
			*p++ = ',';
		if (p < limit)
			continue;
		*p = 0;
		printa(INDENT, buffer);
		p = buffer;
	}

	*p = 0;
	printa(INDENT, buffer);
}


//____________________________________________________________
//
//		Generate substitution text for READY
//

static void gen_ready( const act* action, int column)
{
	const TEXT* vector = status_vector(action);

	for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
	{
		const gpre_dbb* db = ready->rdy_database;
		const TEXT* filename = ready->rdy_filename;
		if (!filename)
			filename = db->dbb_runtime;
		if ((action->act_error || (action->act_flags & ACT_sql)) && ready != (rdy*) action->act_object)
		{
			printa(column, "if isc_status(1) = 0 then");
		}
		make_ready(db, filename, vector, column, ready->rdy_request);
		if ((action->act_error || (action->act_flags & ACT_sql)) && ready != (rdy*) action->act_object)
		{
			endif(column);
		}
	}
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate receive call for a port.
//

static void gen_receive( const act* action, int column, gpre_port* port)
{
	const gpre_req* request = action->act_request;
	printa(column, "firebird.RECEIVE (%s %s, %d, %d, isc_%d'address, %s);", status_vector(action),
		   request->req_handle, port->por_msg_number,
		   port->por_length, port->por_ident, request->req_request_level);

	set_sqlcode(action, column);
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

static void gen_release( const act* action, int column)
{
	const gpre_dbb* exp_db = (gpre_dbb*) action->act_object;

	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
	{
		const gpre_dbb* db = request->req_database;
		if (exp_db && db != exp_db)
			continue;
		if (!(request->req_flags & REQ_exp_hand))
		{
			printa(column, "if %s%s /= 0 then", gpreGlob.ada_package, db->dbb_name->sym_string);
			printa(column + INDENT, "firebird.release_request (isc_status, %s);", request->req_handle);
			endif(column);
			printa(column, "%s := 0;", request->req_handle);
		}
	}
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_request( gpre_req* request, int column)
{
	// generate request handle, blob handles, and ports

	if (!(request->req_flags & (REQ_exp_hand  | REQ_sql_blob_open | REQ_sql_blob_create)) &&
		request->req_type != REQ_slice && request->req_type != REQ_procedure)
	{
		printa(column, "%s\t: firebird.request_handle := 0;-- request handle --", request->req_handle);
	}

	if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
		printa(column, "isc_%do\t: %s;\t\t-- SQL CURSOR FLAG --", request->req_ident, SHORT_DCL);

	// check the case where we need to extend the dpb dynamically at runtime,
	// in which case we need dpb length and a pointer to be defined even if
	// there is no static dpb defined

	if (request->req_flags & REQ_extend_dpb)
	{
		printa(column, "isc_%dp\t: firebird.dpb_ptr := 0;\t-- db parameter block --",
			   request->req_ident);
		if (!request->req_length)
			printa(column, "isc_%dl\t: firebird.isc_ushort := 0;\t-- db parameter block --",
				   request->req_ident);
	}

	// generate actual BLR string

	if (request->req_length)
	{
		const SSHORT length = request->req_length;
		switch (request->req_type)
		{
		case REQ_create_database:
			if (gpreGlob.ada_flags & gpreGlob.ADA_create_database)
			{
				printa(column,
					   "function isc_to_dpb_ptr is new unchecked_conversion (system.address, firebird.dpb_ptr);");
				gpreGlob.ada_flags &= ~gpreGlob.ADA_create_database;
			}
			// fall into ...
		case REQ_ready:
			printa(column, "isc_%dl\t: firebird.isc_ushort := %d;\t-- db parameter block --",
				   request->req_ident, length);
			printa(column, "isc_%d\t: CONSTANT firebird.dpb (1..%d) := (", request->req_ident, length);
			break;

		default:
			if (request->req_flags & REQ_sql_cursor)
				printa(column, "isc_%do\t: %s;\t\t-- SQL CURSOR FLAG --",
					   request->req_ident, SHORT_DCL);
			printa(column, "isc_%d\t: CONSTANT firebird.blr (1..%d) := (", request->req_ident, length);
		}
		gen_raw(request->req_blr, request->req_length);
		printa(column, ");\n");
		const TEXT* string_type;
		if (!gpreGlob.sw_raw)
		{
			printa(column, "---");
			printa(column, "--- FORMATTED REQUEST BLR FOR GDS_%d = \n", request->req_ident);

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
		printa(column, "\t-- end of %s string for request isc_%d --\n",
			   string_type, request->req_ident, request->req_length);
	}

	// Print out slice description language if there are arrays associated with request
	for (const gpre_port* port = request->req_ports; port; port = port->por_next)
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_sdl)
			{
				printa(column, "isc_%d\t: CONSTANT firebird.blr (1..%d) := (",
					   reference->ref_sdl_ident, reference->ref_sdl_length);
				gen_raw(reference->ref_sdl, reference->ref_sdl_length);
				printa(column, "); \t--- end of SDL string for isc_%d\n", reference->ref_sdl_ident);
				if (!gpreGlob.sw_raw)
				{
					printa(column, "---");
					printa(column, "--- FORMATTED REQUEST SDL FOR GDS_%d = \n",
						   reference->ref_sdl_ident);
					if (PRETTY_print_sdl(reference->ref_sdl, gen_blr, 0, 1))
						CPR_error("internal error during SDL generation");
				}
			}
		}

	// Print out any blob parameter blocks required

	for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		if (blob->blb_bpb_length)
		{
			printa(column, "isc_%d\t: CONSTANT firebird.blr (1..%d) := (",
				   blob->blb_bpb_ident, blob->blb_bpb_length);
			gen_raw(blob->blb_bpb, blob->blb_bpb_length);
			printa(INDENT, ");\n");
		}

	// If this is a GET_SLICE/PUT_slice, allocate some variables

	if (request->req_type == REQ_slice)
	{
		printa(INDENT, "isc_%ds\t: %s;\t\t", request->req_ident, LONG_DCL);
		printa(INDENT, "isc_%dv: %s (1..%d);", request->req_ident,
			   LONG_VECTOR_DCL, MAX(request->req_slice->slc_parameters, 1));
	}
}


//____________________________________________________________
//
//		Generate receive call for a port
//		in a store2 statement.
//

static void gen_return_value( const act* action, int column)
{
	gpre_req* request = action->act_request;
	if (action->act_pair->act_error)
		column += INDENT;
	align(column);

	gen_start(action, request->req_primary, column);
	upd* update = (upd*) action->act_object;
	ref* reference = update->upd_references;
	gen_receive(action, column, reference->ref_port);
}


//____________________________________________________________
//
//		Process routine head.  If there are gpreGlob.requests in the
//		routine, insert local definitions.
//

static void gen_routine( const act* action, int column)
{
	column += INDENT;

	for (gpre_req* request = (gpre_req*) action->act_object; request; request = request->req_routine)
	{
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			make_port(port, column);
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			printa(column, "isc_%d\t: isc_%dt;\t\t\t-- message --", port->por_ident, port->por_ident);

		for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			printa(column, "isc_%d\t: firebird.blob_handle;\t-- blob handle --", blob->blb_ident);
			SSHORT blob_subtype = blob->blb_const_to_type;
			if (!blob_subtype)
			{
				const ref* reference = blob->blb_reference;
				if (reference)
				{
					const gpre_fld* field = reference->ref_field;
					if (field)
						blob_subtype = field->fld_sub_type;
				}
			}
			if (blob_subtype != 0 && blob_subtype != 1)
				printa(column, "isc_%d\t: %s (1..%d);\t-- blob segment --",
					   blob->blb_buff_ident, BYTE_VECTOR_DCL, blob->blb_seg_length);
			else
				printa(column, "isc_%d\t: string (1..%d);\t-- blob segment --",
					   blob->blb_buff_ident, blob->blb_seg_length);
			printa(column, "isc_%d\t: %s;\t\t\t-- segment length --", blob->blb_len_ident, USHORT_DCL);
		}
	}
	column -= INDENT;
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static void gen_s_end( const act* action, int column)
{
	gpre_req* request = action->act_request;

	if (action->act_type == ACT_close)
	{
		make_cursor_open_test(ACT_close, request, column);
		column += INDENT;
	}

	printa(column, "firebird.UNWIND_REQUEST (%s %s, %s);", status_vector(action),
		   request->req_handle, request->req_request_level);

	set_sqlcode(action, column);

	if (action->act_type == ACT_close)
	{
		printa(column, "if (SQLCODE = 0) then");
		printa(column + INDENT, "isc_%do := 0;", request->req_ident);
		endif(column);
		column -= INDENT;
		endif(column);
	}
}


//____________________________________________________________
//
//		Generate substitution text for FETCH.
//

static void gen_s_fetch( const act* action, int column)
{
	gpre_req* request = action->act_request;
	if (request->req_sync)
		gen_send(action, request->req_sync, column);

	gen_receive(action, column, request->req_primary);
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_s_start( const act* action, int column)
{
	gpre_req* request = action->act_request;

	if ((action->act_type == ACT_open))
	{
		make_cursor_open_test(ACT_open, request, column);
		column += INDENT;
	}

	gen_compile(action, column);

	gpre_port* port = request->req_vport;
	if (port)
		asgn_from(action, port->por_references, column);

	if (action->act_error || (action->act_flags & ACT_sql))
	{
		make_ok_test(action, request, column);
		gen_start(action, port, column + INDENT);
		endif(column);
	}
	else
		gen_start(action, port, column);

	if (action->act_type == ACT_open)
	{
		printa(column, "if (SQLCODE = 0) then");
		printa(column + INDENT, "isc_%do := 1;", request->req_ident);
		endif(column);
		column -= INDENT;
		endif(column);
	}
}


//____________________________________________________________
//
//		Substitute for a segment, segment length, or blob handle.
//

static void gen_segment( const act* action, int column)
{
	const blb* blob = (blb*) action->act_object;

	printa(column, "isc_%d",
			(action->act_type == ACT_segment) ? blob->blb_buff_ident :
				(action->act_type == ACT_segment_length) ? blob->blb_len_ident : blob->blb_ident);
}


//____________________________________________________________
//
//		Generate code for standalone SELECT statement.
//

static void gen_select( const act* action, int column)
{
	gpre_req* request = action->act_request;
	gpre_port* port = request->req_primary;
	TEXT name[MAX_REF_SIZE];
	gen_name(name, request->req_eof, true);

	gen_s_start(action, column);
	gen_receive(action, column, port);
	printa(column, "if SQLCODE = 0 then", name);
	column += INDENT;
	printa(column, "if %s /= 0 then", name);
	column += INDENT;

	gpre_nod* var_list = (gpre_nod*) action->act_object;
	if (var_list)
	{
		for (int i = 0; i < var_list->nod_count; i++)
			asgn_to(action, (ref*) var_list->nod_arg[i], column);
	}

	printa(column - INDENT, "else");
	printa(column, "SQLCODE := 100;");
	column -= INDENT;
	endif(column);
	column -= INDENT;
	endif(column);
}


//____________________________________________________________
//
//		Generate a send or receive call for a port.
//

static void gen_send( const act* action, gpre_port* port, int column)
{
	const gpre_req* request = action->act_request;
	printa(column, "firebird.SEND (%s %s, %d, %d, isc_%d'address, %s);", status_vector(action),
		   request->req_handle, port->por_msg_number,
		   port->por_length, port->por_ident, request->req_request_level);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate support for get/put slice statement.
//

static void gen_slice( const act* action, int column)
{
	const TEXT* pattern1 =
		"firebird.GET_SLICE (%V1 %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5'address%RE, %RF%S6%RE);";
	const TEXT* pattern2 =
		"firebird.PUT_SLICE (%V1 %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5'address%RE);";

	const gpre_req* request = action->act_request;
	const slc* slice = (slc*) action->act_object;
	const gpre_req* parent_request = slice->slc_parent_request;

	// Compute array size

	printa(column, "isc_%ds := %d", request->req_ident, slice->slc_field->fld_array->fld_length);

	const slc::slc_repeat* tail = slice->slc_rpt;
	for (const slc::slc_repeat* const end = tail + slice->slc_dimensions; tail < end; ++tail)
	{
		if (tail->slc_upper != tail->slc_lower)
		{
			const ref* lower = (ref*) tail->slc_lower->nod_arg[0];
			const ref* upper = (ref*) tail->slc_upper->nod_arg[0];
			if (lower->ref_value)
				fprintf(gpreGlob.out_file, " * ( %s - %s + 1)", upper->ref_value, lower->ref_value);
			else
				fprintf(gpreGlob.out_file, " * ( %s + 1)", upper->ref_value);
		}
	}

	fprintf(gpreGlob.out_file, ";");

	// Make assignments to variable vector

	for (const ref* reference = request->req_values; reference; reference = reference->ref_next)
	{
		printa(column, "isc_%dv (%d) := %s;", request->req_ident,
			   reference->ref_id + 1, reference->ref_value);
	}

	PAT args;
	args.pat_reference = slice->slc_field_ref;
	args.pat_request = parent_request;	// blob id request
	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = parent_request->req_database;	// database handle
	args.pat_value1 = request->req_length;	// slice descr. length
	args.pat_ident1 = request->req_ident;	// request name
	args.pat_value2 = slice->slc_parameters * sizeof(SLONG);	// parameter length

	const ref* reference = (ref*) slice->slc_array->nod_arg[0];
	args.pat_string5 = reference->ref_value;	// array name
	args.pat_string6 = "isc_array_length";

	PATTERN_expand(column, (action->act_type == ACT_get_slice) ? pattern1 : pattern2, &args);
}


//____________________________________________________________
//
//		Generate either a START or START_AND_SEND depending
//		on whether or a not a port is present.
//

static void gen_start( const act* action, gpre_port* port, int column)
{
	gpre_req* request = action->act_request;
	const TEXT* vector = status_vector(action);
	column += INDENT;

	if (port)
	{
		for (ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field->fld_array_info)
				gen_get_or_put_slice(action, reference, false, column);
		}

		printa(column, "firebird.START_AND_SEND (%s %s, %s%s, %d, %d, isc_%d'address, %s);",
			   vector, request->req_handle, gpreGlob.ada_package, request_trans(action, request),
			   port->por_msg_number, port->por_length, port->por_ident, request->req_request_level);
	}
	else
		printa(column, "firebird.START_REQUEST (%s %s, %s%s, %s);", vector,
			   request->req_handle, gpreGlob.ada_package, request_trans(action, request),
			   request->req_request_level);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate text for STORE statement.  This includes the compile
//		call and any variable initialization required.
//

static void gen_store( const act* action, int column)
{
	gpre_req* request = action->act_request;
	gen_compile(action, column);
	if (action->act_error || (action->act_flags & ACT_sql))
		make_ok_test(action, request, column);

	// Initialize any blob fields
	TEXT name[MAX_REF_SIZE];
	const gpre_port* port = request->req_primary;
	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_flags & FLD_blob)
			printa(column, "%s := firebird.null_blob;", gen_name(name, reference, true));
	}
}


//____________________________________________________________
//
//		Generate substitution text for START_TRANSACTION.
//

static void gen_t_start( const act* action, int column)
{
	// for automatically generated transactions, and transactions that are
	// explicitly started, but don't have any arguments so don't get a TPB,
	// generate something plausible.

	const gpre_tra* trans;
	if (!action || !(trans = (gpre_tra*) action->act_object))
	{
		t_start_auto(action, 0, status_vector(action), column, false);
		return;
	}

	// build a complete statement, including tpb's.
	// first generate any appropriate ready statements,
	// and fill in the tpb vector (aka TEB).

	int count = 0;
	for (const tpb* tpb_iterator = trans->tra_tpb; tpb_iterator;
		tpb_iterator = tpb_iterator->tpb_tra_next)
	{
		count++;
		const gpre_dbb* db = tpb_iterator->tpb_database;
		if (gpreGlob.sw_auto)
		{
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				printa(column, "if (%s%s = 0) then", gpreGlob.ada_package, db->dbb_name->sym_string);
				align(column + INDENT);
				make_ready(db, filename, status_vector(action), column + INDENT, 0);
				endif(column);
			}
		}

		printa(column, "isc_teb(%d).tpb_len := %d;", count, tpb_iterator->tpb_length);
		printa(column, "isc_teb(%d).tpb_ptr := isc_tpb_%d'address;", count, tpb_iterator->tpb_ident);
		printa(column, "isc_teb(%d).dbb_ptr := %s%s'address;",
			   count, gpreGlob.ada_package, db->dbb_name->sym_string);
	}

	printa(column, "firebird.start_multiple (%s %s%s, %d, isc_teb'address);", status_vector(action),
		   gpreGlob.ada_package, trans->tra_handle ? (const char*) trans->tra_handle : "gds_trans",
		   trans->tra_db_count);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a TPB in the output file
//

static void gen_tpb(const tpb* tpb_buffer, int column)
{
	printa(column, "isc_tpb_%d\t: CONSTANT firebird.tpb (1..%d) := (",
		   tpb_buffer->tpb_ident, tpb_buffer->tpb_length);

	int length = tpb_buffer->tpb_length;
	const TEXT* text = (TEXT*) tpb_buffer->tpb_string;
	TEXT buffer[80];
	TEXT* p = buffer;

	while (--length)
	{
		const TEXT c = *text++;
		sprintf(p, "%d, ", c);
		while (*p)
			p++;
		if (p - buffer > 60)
		{
			printa(column + INDENT, " %s", buffer);
			p = buffer;
			*p = 0;
		}
	}

	// handle the last character

	const TEXT c = *text++;
	sprintf(p, "%d", c);
	printa(column + INDENT, "%s", buffer);
	printa(column, ");");
}


//____________________________________________________________
//
//		Generate substitution text for COMMIT, ROLLBACK, PREPARE, and SAVE
//

static void gen_trans( const act* action, int column)
{
	const char* tranText = action->act_object ? (const TEXT*) action->act_object : "gds_trans";

	switch (action->act_type)
	{
	case ACT_commit_retain_context:
		printa(column, "firebird.COMMIT_RETAINING (%s %s%s);", status_vector(action),
			   gpreGlob.ada_package, tranText);
		break;
	case ACT_rollback_retain_context:
		printa(column, "firebird.ROLLBACK_RETAINING (%s %s%s);", status_vector(action),
			   gpreGlob.ada_package, tranText);
		break;
	default:
		printa(column, "firebird.%s_TRANSACTION (%s %s%s);",
			   (action->act_type == ACT_commit) ?
					"COMMIT" : (action->act_type == ACT_rollback) ? "ROLLBACK" : "PREPARE",
			   status_vector(action), gpreGlob.ada_package, tranText);
	}

	set_sqlcode(action, column);
}



//____________________________________________________________
//
//       Substitute for a variable reference.
//

static void gen_type( const act* action, int column)
{
	printa(column, "%ld", action->act_object);
}



//____________________________________________________________
//
//		Generate substitution text for UPDATE ... WHERE CURRENT OF ...
//

static void gen_update( const act* action, int column)
{
	upd* modify = (upd*) action->act_object;
	gpre_port* port = modify->upd_port;
	asgn_from(action, port->por_references, column);
	gen_send(action, port, column);
}


//____________________________________________________________
//
//		Substitute for a variable reference.
//

static void gen_variable( const act* action, int column)
{
	TEXT s[MAX_REF_SIZE];

	printa(column, gen_name(s, (ref*) action->act_object, false));
}


//____________________________________________________________
//
//		Generate tests for any WHENEVER clauses that may have been declared.
//

static void gen_whenever(const swe* label, int column)
{
	while (label)
	{
		const TEXT* condition = NULL;

		switch (label->swe_condition)
		{
		case SWE_error:
			condition = "SQLCODE < 0";
			break;

		case SWE_warning:
			condition = "(SQLCODE > 0) AND (SQLCODE /= 100)";
			break;

		case SWE_not_found:
			condition = "SQLCODE = 100";
			break;

		default:
			// condition undefined
			fb_assert(false);
			return;
		}
		printa(column, "if %s then goto %s; end if;", condition, label->swe_label);
		label = label->swe_next;
	}
}


//____________________________________________________________
//
//		Generate a declaration of an array in the
//       output file.
//

static void make_array_declaration( ref* reference, int column)
{
	gpre_fld* field = reference->ref_field;
	const TEXT* name = field->fld_symbol->sym_string;

	// Don't generate multiple declarations for the array.  V3 Bug 569.

	if (field->fld_array_info->ary_declared)
		return;

	field->fld_array_info->ary_declared = true;

	if ((field->fld_array->fld_dtype <= dtype_varying) && (field->fld_array->fld_length != 1))
	{
		if (field->fld_array->fld_sub_type == 1)
			fprintf(gpreGlob.out_file, "subtype isc_%d_byte is %s(1..%d);\n",
					   field->fld_array_info->ary_ident, BYTE_VECTOR_DCL,
					   field->fld_array->fld_length);
		else
			fprintf(gpreGlob.out_file, "subtype isc_%d_string is string(1..%d);\n",
					   field->fld_array_info->ary_ident, field->fld_array->fld_length);
	}

	fprintf(gpreGlob.out_file, "type isc_%dt is array (", field->fld_array_info->ary_ident);

	// Print out the dimension part of the declaration
	const dim* dimension = field->fld_array_info->ary_dimension;
	for (int i = 1;
		 i < field->fld_array_info->ary_dimension_count;
		 dimension = dimension->dim_next, ++i)
	{
		fprintf(gpreGlob.out_file, "%s range %"SLONGFORMAT"..%"SLONGFORMAT",\n                        ",
				LONG_DCL, dimension->dim_lower, dimension->dim_upper);
	}

	fprintf(gpreGlob.out_file, "%s range %"SLONGFORMAT"..%"SLONGFORMAT") of ",
			   LONG_DCL, dimension->dim_lower, dimension->dim_upper);

	switch (field->fld_array_info->ary_dtype)
	{
	case dtype_short:
		fprintf(gpreGlob.out_file, "%s", SHORT_DCL);
		break;

	case dtype_long:
		fprintf(gpreGlob.out_file, "%s", LONG_DCL);
		break;

	case dtype_cstring:
	case dtype_text:
	case dtype_varying:
		if (field->fld_array->fld_length == 1)
		{
			if (field->fld_array->fld_sub_type == 1)
				fprintf(gpreGlob.out_file, BYTE_DCL);
			else
				fprintf(gpreGlob.out_file, "firebird.isc_character");
		}
		else
		{
			if (field->fld_array->fld_sub_type == 1)
				fprintf(gpreGlob.out_file, "isc_%d_byte", field->fld_array_info->ary_ident);
			else
				fprintf(gpreGlob.out_file, "isc_%d_string", field->fld_array_info->ary_ident);
		}
		break;

	case dtype_date:
	case dtype_quad:
		fprintf(gpreGlob.out_file, "firebird.quad");
		break;

	case dtype_real:
		fprintf(gpreGlob.out_file, "%s", REAL_DCL);
		break;

	case dtype_double:
		fprintf(gpreGlob.out_file, "%s", DOUBLE_DCL);
		break;

	default:
		printa(column, "datatype %d unknown for field %s", field->fld_array_info->ary_dtype, name);
		return;
	}

	fprintf(gpreGlob.out_file, ";\n");

	// Print out the database field

	fprintf(gpreGlob.out_file, "isc_%d : isc_%dt;\t--- %s\n\n",
			field->fld_array_info->ary_ident, field->fld_array_info->ary_ident, name);
}


//____________________________________________________________
//
//     Generate code to test existence
//     of open cursor and do the right thing:
//     if type == ACT_open && isc_nl, error
//     if type == ACT_close && !isc_nl, error
//

static void make_cursor_open_test( act_t type, gpre_req* request, int column)
{
	if (type == ACT_open)
	{
		printa(column, "if (isc_%do = 1) then", request->req_ident);
		printa(column + INDENT, "SQLCODE := -502;");
	}
	else if (type == ACT_close)
	{
		printa(column, "if (isc_%do = 0) then", request->req_ident);
		printa(column + INDENT, "SQLCODE := -501;");
	}

	printa(column, "else");
}


//____________________________________________________________
//
//		Turn a symbol into a varying string.
//

static TEXT* make_name(TEXT* const string, const gpre_sym* symbol)
{
	fb_utils::snprintf(string, MAX_CURSOR_SIZE, "\"%s \"", symbol->sym_string);

	return string;
}


//____________________________________________________________
//
//		Generate code to test existence of
//		compiled request with active transaction.
//

static void make_ok_test( const act* action, gpre_req* request, int column)
{
	if (gpreGlob.sw_auto)
		printa(column, "if (%s%s /= 0) and (%s /= 0) then",
			   gpreGlob.ada_package, request_trans(action, request), request->req_handle);
	else
		printa(column, "if (%s /= 0) then", request->req_handle);
}


//____________________________________________________________
//
//		Insert a port record description in output.
//

static void make_port( const gpre_port* port, int column)
{
	printa(column, "type isc_%dt is record", port->por_ident);

	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		const TEXT* name;
		const gpre_sym* symbol = field->fld_symbol;
		if (symbol)
			name = symbol->sym_string;
		else
			name = "<expression>";
		if (reference->ref_value && (reference->ref_flags & REF_array_elem))
			field = field->fld_array;

		switch (field->fld_dtype)
		{
		case dtype_real:
			printa(column + INDENT, "isc_%d\t: %s;\t-- %s --", reference->ref_ident, REAL_DCL, name);
			break;

		case dtype_double:
			printa(column + INDENT, "isc_%d\t: %s;\t-- %s --", reference->ref_ident, DOUBLE_DCL, name);
			break;

		case dtype_short:
			printa(column + INDENT, "isc_%d\t: %s;\t-- %s --", reference->ref_ident, SHORT_DCL, name);
			break;

		case dtype_long:
			printa(column + INDENT, "isc_%d\t: %s;\t-- %s --", reference->ref_ident, LONG_DCL, name);
			break;

		case dtype_date:
		case dtype_quad:
		case dtype_blob:
			printa(column + INDENT, "isc_%d\t: firebird.quad;\t-- %s --", reference->ref_ident, name);
			break;

		case dtype_text:
			if (strcmp(name, "isc_slack"))
			{
				if (field->fld_sub_type == 1)
				{
					if (field->fld_length == 1)
						printa(column + INDENT, "isc_%d\t: %s;\t-- %s --",
							   reference->ref_ident, BYTE_DCL, name);
					else
						printa(column + INDENT, "isc_%d\t: %s (1..%d);\t-- %s --",
							   reference->ref_ident, BYTE_VECTOR_DCL, field->fld_length, name);
				}
				else
				{
					if (field->fld_length == 1)
						printa(column + INDENT, "isc_%d\t: firebird.isc_character;\t-- %s --",
							   reference->ref_ident, name);
					else
						printa(column + INDENT, "isc_%d\t: string (1..%d);\t-- %s --",
							   reference->ref_ident, field->fld_length, name);
				}
			}
			break;

		default:
			{
				TEXT s[ERROR_LENGTH];
				fb_utils::snprintf(s, sizeof(s), "datatype %d unknown for field %s, msg %d",
								   field->fld_dtype, name, port->por_msg_number);
				CPR_error(s);
				return;
			}
		}
	}

	printa(column, "end record;");

	printa(column, "for isc_%dt use record at mod 4;", port->por_ident);

	int pos = 0;
	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (reference->ref_value && field->fld_array_info)
			field = field->fld_array;
		printa(column + INDENT, "isc_%d at %d range 0..%d;",
			   reference->ref_ident, pos, 8 * field->fld_length - 1);
		pos += field->fld_length;
	}

	printa(column, "end record;");
}


//____________________________________________________________
//
//		Generate the actual insertion text for a
//       ready;
//

static void make_ready(const gpre_dbb* db,
					   const TEXT* filename,
					   const TEXT* vector,
					   USHORT column,
					   gpre_req* request)
{
	TEXT s1[32], s2[32];

	if (request)
	{
		sprintf(s1, "isc_%dl", request->req_ident);
		sprintf(s2, "isc_%d", request->req_ident);

		// if the dpb needs to be extended at runtime to include items
		// in host variables, do so here; this assumes that there is
		// always a request generated for runtime variables

		if (request->req_flags & REQ_extend_dpb)
		{
			sprintf(s2, "isc_%dp", request->req_ident);
			if (request->req_length)
				printa(column, "%s = isc_%d;", s2, request->req_ident);
			if (db->dbb_r_user)
				printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_user_name, %s, %d);\n",
					   s2, s1, db->dbb_r_user, (strlen(db->dbb_r_user) - 2));
			if (db->dbb_r_password)
				printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_password, %s, %d);\n",
					   s2, s1, db->dbb_r_password, (strlen(db->dbb_r_password) - 2));

			// Process Role Name
			if (db->dbb_r_sql_role)
				printa(column,
					   "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_sql_role_name, %s, %d);\n",
					   s2, s1, db->dbb_r_sql_role, (strlen(db->dbb_r_sql_role) - 2));

			if (db->dbb_r_lc_messages)
				printa(column,
					   "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_lc_messages, %s, %d);\n",
					   s2, s1, db->dbb_r_lc_messages, (strlen(db->dbb_r_lc_messages) - 2));
			if (db->dbb_r_lc_ctype)
				printa(column, "firebird.MODIFY_DPB (%s, %s, firebird.isc_dpb_lc_ctype, %s, %d);\n",
					   s2, s1, db->dbb_r_lc_ctype, (strlen(db->dbb_r_lc_ctype) - 2));
		}
	}

	if (filename)
	{
		if (*filename == '"')
			printa(column, "firebird.ATTACH_DATABASE (%s %d, %s, %s%s, %s, %s);",
				   vector, strlen(filename) - 2, filename, gpreGlob.ada_package,
				   db->dbb_name->sym_string, (request ? s1 : "0"),
				   (request ? s2 : "firebird.null_dpb"));
		else
			printa(column, "firebird.ATTACH_DATABASE (%s %s'length(1), %s, %s%s, %s, %s);",
				   vector, filename, filename, gpreGlob.ada_package,
				   db->dbb_name->sym_string, (request ? s1 : "0"),
				   (request ? s2 : "firebird.null_dpb"));
	}
	else
		printa(column, "firebird.ATTACH_DATABASE (%s %d, \"%s\", %s%s, %s, %s);",
			   vector, strlen(db->dbb_filename), db->dbb_filename, gpreGlob.ada_package,
			   db->dbb_name->sym_string, (request ? s1 : "0"),
			   (request ? s2 : "firebird.null_dpb"));

	if (request && request->req_flags & REQ_extend_dpb)
	{
		if (request->req_length)
			printa(column, "if (%s != isc_%d)", s2, request->req_ident);
		printa(column + (request->req_length ? 4 : 0), "firebird.FREE (%s);", s2);

		// reset the length of the dpb
		if (request->req_length)
			printa(column, "%s = %d;", s1, request->req_length);
	}
}


//____________________________________________________________
//
//		Print a fixed string at a particular column.
//

static void printa( int column, const TEXT* string, ...)
{
	va_list ptr;

	va_start(ptr, string);
	align(column);
	vsprintf(output_buffer, string, ptr);
	va_end(ptr);
	ADA_print_buffer(output_buffer, column);
}


//____________________________________________________________
//
//		Generate the appropriate transaction handle.
//

static const TEXT* request_trans( const act* action, const gpre_req* request)
{
	if (action->act_type == ACT_open)
	{
		const TEXT* trname = ((open_cursor*) action->act_object)->opn_trans;
		if (!trname)
			trname = "gds_trans";
		return trname;
	}

	return request ? request->req_trans : "gds_trans";
}


//____________________________________________________________
//
//		Generate the appropriate status vector parameter for a gds
//		call depending on where or not the action has an error clause.
//

static const TEXT* status_vector( const act* action)
{
	if (action && (action->act_error || (action->act_flags & ACT_sql)))
		return "isc_status,";

	return "";
}


//____________________________________________________________
//
//		Generate substitution text for START_TRANSACTION.
//

static void t_start_auto(const act* action,
						 const gpre_req* request,
						 const TEXT* vector,
						 int column,
						 bool test)
{
	const TEXT* trname = request_trans(action, request);
	const int stat = !strcmp(vector, "isc_status");

	begin(column);

	if (gpreGlob.sw_auto)
	{
		TEXT buffer[256], temp[40];
		buffer[0] = 0;
		for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				align(column);
				fprintf(gpreGlob.out_file, "if (%s%s = 0", gpreGlob.ada_package,
						db->dbb_name->sym_string);
				if (stat && buffer[0])
					fprintf(gpreGlob.out_file, "and %s(1) = 0", vector);
				fprintf(gpreGlob.out_file, ") then");
				make_ready(db, filename, vector, column + INDENT, 0);
				endif(column);
				if (buffer[0])
					strcat(buffer, ") and (");
				sprintf(temp, "%s%s /= 0", gpreGlob.ada_package, db->dbb_name->sym_string);
				strcat(buffer, temp);
			}
		}
		if (!buffer[0])
			strcpy(buffer, "True");
		if (test)
			printa(column, "if (%s) and (%s%s = 0) then", buffer, gpreGlob.ada_package, trname);
		else
			printa(column, "if (%s) then", buffer);
		column += INDENT;
	}

	int count = 0;
	for (const gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		count++;
		printa(column, "isc_teb(%d).tpb_len:= 0;", count);
		printa(column, "isc_teb(%d).tpb_ptr := firebird.null_tpb'address;", count);
		printa(column, "isc_teb(%d).dbb_ptr := %s%s'address;", count,
			   gpreGlob.ada_package, db->dbb_name->sym_string);
	}

	printa(column, "firebird.start_multiple (%s %s%s, %d, isc_teb'address);",
		   vector, gpreGlob.ada_package, trname, count);

	if (gpreGlob.sw_auto)
	{
		endif(column);
		column -= INDENT;
	}

	endp(column);
}

