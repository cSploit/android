//____________________________________________________________
//
//		PROGRAM:	PASCAL preprocesser
//		MODULE:		pas.cpp
//		DESCRIPTION:	Inserted text generator for Domain Pascal
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
#include <string.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
#include "../gpre/pat.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/lang_proto.h"
#include "../gpre/pat_proto.h"
#include "../gpre/gpre_proto.h"
#include "../common/prett_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h"

#pragma FB_COMPILER_MESSAGE("This file is not fit for public consumption")
// #error compiler halted, 'rogue' not found...
// TMN: Upon converting this file to C++, it has been noted
// that this code would never have worked because of (SEGV) bug(s),
// why I rather than trying to use it currently remove it from compilation.


static void align(const int);
static void asgn_from(const act*, const ref*, int);
static void asgn_sqlda_from(const ref*, int, TEXT*, int);
static void asgn_to(const act*, const ref*, int);
static void asgn_to_proc(const ref*, int);
static void gen_at_end(const act*, int);
static void gen_based(const act*, int);
static void gen_blob_close(const act*, USHORT);
static void gen_blob_end(const act*, USHORT);
static void gen_blob_for(const act*, USHORT);
static void gen_blob_open(const act*, USHORT);
static void gen_blr(void*, SSHORT, const char*);
static void gen_compile(const act*, int);
static void gen_create_database(const act*, int);
static int gen_cursor_close(const act*, const gpre_req*, int);
static void gen_cursor_init(const act*, int);
static int gen_cursor_open(const act*, const gpre_req*, int);
static void gen_database(/*const act*,*/ int);
static void gen_ddl(const act*, int);
static void gen_drop_database(const act*, int);
static void gen_dyn_close(const act*, int);
static void gen_dyn_declare(const act*, int);
static void gen_dyn_describe(const act*, int, bool);
static void gen_dyn_execute(const act*, int);
static void gen_dyn_fetch(const act*, int);
static void gen_dyn_immediate(const act*, int);
static void gen_dyn_insert(const act*, int);
static void gen_dyn_open(const act*, int);
static void gen_dyn_prepare(const act*, int);
static void gen_emodify(const act*, int);
static void gen_estore(const act*, int);
static void gen_endfor(const act*, int);
static void gen_erase(const act*, int);
static SSHORT gen_event_block(const act*);
static void gen_event_init(const act*, int);
static void gen_event_wait(const act*, int);
static void gen_fetch(const act*, int);
static void gen_finish(const act*, int);
static void gen_for(const act*, int);
static void gen_get_or_put_slice(const act*, const ref*, bool, int);
static void gen_get_segment(const act*, int);
static void gen_loop(const act*, int);
static TEXT*	gen_name(TEXT* const, const ref*, bool);
static void gen_on_error(const act*, USHORT);
static void gen_procedure(const act*, int);
static void gen_put_segment(const act*, int);
static void gen_raw(const UCHAR*, int); //, int);
static void gen_ready(const act*, int);
static void gen_receive(const act*, int, const gpre_port*);
static void gen_release(const act*, int);
static void gen_request(const gpre_req*, int);
static void gen_return_value(const act*, int);
static void gen_routine(const act*, int);
static void gen_s_end(const act*, int);
static void gen_s_fetch(const act*, int);
static void gen_s_start(const act*, int);
static void gen_segment(const act*, int);
static void gen_select(const act*, int);
static void gen_send(const act*, const gpre_port*, int);
static void gen_slice(const act*, int);
static void gen_start(const act*, const gpre_port*, int);
static void gen_store(const act*, int);
static void gen_t_start(const act*, int);
static void gen_tpb(const tpb*, int);
static void gen_trans(const act*, int);
static void gen_update(const act*, int);
static void gen_variable(const act*, int);
static void gen_whenever(const swe*, int);
static void make_array_declaration(const ref*);
static TEXT* make_name(TEXT* const, const gpre_sym*);
static void make_ok_test(const act*, const gpre_req*, int);
static void make_port(const gpre_port*, int);
static void make_ready(const gpre_dbb*, const TEXT*, const TEXT*, USHORT, const gpre_req*);
static void printa(int, const char*, ...);
static const TEXT* request_trans(const act*, const gpre_req*);
static const TEXT* status_vector(const act*);
static void t_start_auto(const act*, const gpre_req*, const TEXT*, int);

static bool global_first_flag = false;

const int INDENT = 3;

const char* const SHORT_DCL		= "integer16";
const char* const LONG_DCL		= "integer32";
const char* const POINTER_DCL		= "UNIV_PTR";
const char* const PACKED_ARRAY	= "array";
const char* const OPEN_BRACKET	= "[";
const char* const CLOSE_BRACKET	= "]";
const char* const REF_PAR			= "";
const char* const SIZEOF			= "sizeof";
const char* const STATIC_STRING	= "STATIC";
const char* const ISC_BADDRESS	= "ADDR";

const char* const FB_DP_VOLATILE		= "";
const char* const GDS_EVENT_COUNTS	= "GDS__EVENT_COUNTS";
const char* const GDS_EVENT_WAIT		= "GDS__EVENT_WAIT";

static inline void begin(const int column)
{
	printa(column, "begin");
}

static inline void endp(const int column)
{
	printa(column, "end");
}

static inline void ends(const int column)
{
	printa(column, "end;");
}

static inline void set_sqlcode(const act* action, const int column)
{
	if (action->act_flags & ACT_sql)
		printa(column, "SQLCODE := gds__sqlcode (gds__status);");
}

//____________________________________________________________
//
//		Code generator for Domain Pascal.  Not to be confused with
//		the language "Pascal".
//

void PAS_action(const act* action, int column)
{

	// Put leading braces where required

	switch (action->act_type)
	{
	case ACT_alter_database:
	case ACT_alter_domain:
	case ACT_alter_table:
	case ACT_alter_index:
	case ACT_blob_close:
	case ACT_blob_create:
	case ACT_blob_for:
	case ACT_blob_open:
	case ACT_close:
	case ACT_commit:
	case ACT_commit_retain_context:
	case ACT_create_database:
	case ACT_create_domain:
	case ACT_create_generator:
	case ACT_create_index:
	case ACT_create_shadow:
	case ACT_create_table:
	case ACT_create_view:
	case ACT_declare_filter:
	case ACT_declare_udf:
	case ACT_disconnect:
	case ACT_drop_database:
	case ACT_drop_domain:
	case ACT_drop_filter:
	case ACT_drop_index:
	case ACT_drop_shadow:
	case ACT_drop_table:
	case ACT_drop_udf:
	case ACT_drop_view:
	case ACT_dyn_close:
	case ACT_dyn_cursor:
	case ACT_dyn_describe:
	case ACT_dyn_describe_input:
	case ACT_dyn_execute:
	case ACT_dyn_fetch:
	case ACT_dyn_grant:
	case ACT_dyn_immediate:
	case ACT_dyn_insert:
	case ACT_dyn_open:
	case ACT_dyn_prepare:
	case ACT_dyn_revoke:
	case ACT_fetch:
	case ACT_finish:
	case ACT_for:
	case ACT_get_segment:
	case ACT_get_slice:
	case ACT_insert:
	case ACT_loop:
	case ACT_modify:
	case ACT_open:
	case ACT_prepare:
	case ACT_procedure:
	case ACT_put_slice:
	case ACT_ready:
	case ACT_release:
	case ACT_rfinish:
	case ACT_rollback:
	case ACT_rollback_retain_context:
	case ACT_s_fetch:
	case ACT_s_start:
	case ACT_select:
	case ACT_start:
	case ACT_store:
	case ACT_update:
	case ACT_statistics:
		begin(column);
	}

	switch (action->act_type)
	{
	case ACT_alter_domain:
	case ACT_create_domain:
	case ACT_create_generator:
	case ACT_create_shadow:
	case ACT_declare_filter:
	case ACT_declare_udf:
	case ACT_drop_domain:
	case ACT_drop_filter:
	case ACT_drop_shadow:
	case ACT_drop_udf:
	case ACT_statistics:
	case ACT_alter_index:
	case ACT_alter_table:
		gen_ddl(action, column);
		break;
	case ACT_at_end:
		gen_at_end(action, column);
		return;
	case ACT_b_declare:
		gen_database(/*action,*/ column);
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
		break;
	case ACT_blob_create:
		gen_blob_open(action, column);
		break;
	case ACT_blob_for:
		gen_blob_for(action, column);
		return;
	case ACT_blob_handle:
		gen_segment(action, column);
		return;
	case ACT_blob_open:
		gen_blob_open(action, column);
		break;
	case ACT_close:
		gen_s_end(action, column);
		break;
	case ACT_commit:
		gen_trans(action, column);
		break;
	case ACT_commit_retain_context:
		gen_trans(action, column);
		break;
	case ACT_create_database:
		gen_create_database(action, column);
		break;
	case ACT_create_index:
		gen_ddl(action, column);
		break;
	case ACT_create_table:
		gen_ddl(action, column);
		break;
	case ACT_create_view:
		gen_ddl(action, column);
		break;
	case ACT_cursor:
		gen_cursor_init(action, column);
		return;
	case ACT_database:
		gen_database(/*action,*/ column);
		return;
	case ACT_disconnect:
		gen_finish(action, column);
		break;
	case ACT_drop_database:
		gen_drop_database(action, column);
		break;
	case ACT_drop_index:
		gen_ddl(action, column);
		break;
	case ACT_drop_table:
		gen_ddl(action, column);
		break;
	case ACT_drop_view:
		gen_ddl(action, column);
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
	case ACT_dyn_revoke:
		gen_ddl(action, column);
		break;
	case ACT_endblob:
		gen_blob_end(action, column);
		return;
	case ACT_enderror:
		column += INDENT;
		endp(column);
		column -= INDENT;
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
	case ACT_get_segment:
		gen_get_segment(action, column);
		break;
	case ACT_get_slice:
		gen_slice(action, column);
		break;
	case ACT_hctef:
		ends(column);
		break;
	case ACT_insert:
		gen_s_start(action, column);
		break;
	case ACT_loop:
		gen_loop(action, column);
		break;
	case ACT_on_error:
		gen_on_error(action, column);
		return;
	case ACT_open:
		gen_s_start(action, column);
		break;
	case ACT_ready:
		gen_ready(action, column);
		break;
	case ACT_put_segment:
		gen_put_segment(action, column);
		break;
	case ACT_put_slice:
		gen_slice(action, column);
		break;
	case ACT_prepare:
		gen_trans(action, column);
		break;
	case ACT_procedure:
		gen_procedure(action, column);
		break;
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
	case ACT_segment:
		gen_segment(action, column);
		return;
	case ACT_segment_length:
		gen_segment(action, column);
		return;
	case ACT_sql_dialect:
		gpreGlob.sw_sql_dialect = ((set_dialect*) action->act_object)->sdt_dialect;
		return;
	case ACT_select:
		gen_select(action, column);
		break;
	case ACT_start:
		gen_t_start(action, column);
		break;
	case ACT_store:
		gen_store(action, column);
		return;
	case ACT_store2:
		gen_return_value(action, column);
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

	if (action->act_error)
		fprintf(gpreGlob.out_file, ";");
	else
		endp(column);
}


//____________________________________________________________
//
//		Align output to a specific column for output.
//

static void align(const int column)
{
	putc('\n', gpreGlob.out_file);

	if (column < 0)
		return;

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

static void asgn_from( const act* action, const ref* reference, int column)
{
	TEXT name[MAX_REF_SIZE], variable[MAX_REF_SIZE], temp[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_array_info)
			if (!(reference->ref_flags & REF_array_elem))
			{
				printa(column, "%s := gds__blob_null;\n", gen_name(name, reference, true));
				gen_get_or_put_slice(action, reference, false, column);
				continue;
			}

		if (!reference->ref_source && !reference->ref_value)
			continue;
		align(column);
		gen_name(variable, reference, true);
		const TEXT* value;
		if (reference->ref_source)
			value = gen_name(temp, reference->ref_source, true);
		else
			value = reference->ref_value;
		if (reference->ref_value && (reference->ref_flags & REF_array_elem))
			field = field->fld_array;
		if (!field || (field->fld_flags & FLD_text))
			fprintf(gpreGlob.out_file, "gds__ftof (%s%s, %s (%s), %s%s, %d);",
					   REF_PAR, value, SIZEOF, value, REF_PAR, variable, field->fld_length);
		else if (!reference->ref_master || (reference->ref_flags & REF_literal))
			fprintf(gpreGlob.out_file, "%s := %s;", variable, value);
		else
		{
			fprintf(gpreGlob.out_file, "if %s < 0 then", value);
			align(column + 4);
			fprintf(gpreGlob.out_file, "%s := -1", variable);
			align(column);
			fprintf(gpreGlob.out_file, "else");
			align(column + 4);
			fprintf(gpreGlob.out_file, "%s := 0;", variable);
		}
	}
}


//____________________________________________________________
//
//		Build an assignment from a host language variable to
//		a sqlda variable.
//

static void asgn_sqlda_from( const ref* reference, int number, TEXT* string, int column)
{
	TEXT temp[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		align(column);
		const TEXT* value;
		if (reference->ref_source)
			value = gen_name(temp, reference->ref_source, true);
		else
			value = reference->ref_value;
		fprintf(gpreGlob.out_file, "gds__to_sqlda (gds__sqlda, %d, %s, %s(%s), %s);",
				number, SIZEOF, value, value, string);
	}
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to(const act* action, const ref* reference, int column)
{
	TEXT s[MAX_REF_SIZE];

	ref* source = reference->ref_friend;
	const gpre_fld* field = source->ref_field;

	if (field && field->fld_array_info)
	{
		source->ref_value = reference->ref_value;
		gen_get_or_put_slice(action, source, true, column);
		return;
	}

	if (field && (field->fld_flags & FLD_text))
		fprintf(gpreGlob.out_file, "gds__ftof (%s%s, %d, %s%s, %s (%s));",
				   REF_PAR, gen_name(s, source, true), field->fld_length,
				   REF_PAR, reference->ref_value, SIZEOF, reference->ref_value);
	else
		fprintf(gpreGlob.out_file, "%s := %s;", reference->ref_value, gen_name(s, source, true));

	// Pick up NULL value if one is there

	if (reference = reference->ref_null)
		fprintf(gpreGlob.out_file, "%s := %s;", reference->ref_value, gen_name(s, reference, true));
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//

static void asgn_to_proc(const ref* reference, int column)
{
	TEXT s[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		if (!reference->ref_value)
			continue;
		const gpre_fld* field = reference->ref_field;
		gen_name(s, reference, true);
		align(column);
		if (field && (field->fld_flags & FLD_text))
			fprintf(gpreGlob.out_file, "gds__ftof (%s%s, %d, %s%s, %s (%s));",
					   REF_PAR, gen_name(s, reference, true), field->fld_length,
					   REF_PAR, reference->ref_value, SIZEOF, reference->ref_value);
		else
			fprintf(gpreGlob.out_file, "%s := %s;", reference->ref_value, gen_name(s, reference, true));

	}
}


//____________________________________________________________
//
//		Generate code for AT END clause of FETCH.
//

static void gen_at_end( const act* action, int column)
{
	TEXT s[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	printa(column, "if %s = 0 then begin", gen_name(s, request->req_eof, true));
}


//____________________________________________________________
//
//		Substitute for a BASED ON <field name> clause.
//

static void gen_based( const act* action, int column)
{
	SSHORT datatype;

	align(column);
	bas* based_on = (bas*) action->act_object;
	gpre_fld* field = based_on->bas_field;

	if (based_on->bas_flags & BAS_segment)
	{
		datatype = dtype_text;
		SLONG length = field->fld_seg_length;
		if (!length)
			length = 256;
		fprintf(gpreGlob.out_file, "%s [1..%"SLONGFORMAT"] of ", PACKED_ARRAY, length);
	}
	else if (field->fld_array_info)
	{
		datatype = field->fld_array_info->ary_dtype;
		if (datatype <= dtype_varying)
			fprintf(gpreGlob.out_file, "%s [", PACKED_ARRAY);
		else
			fprintf(gpreGlob.out_file, "array [");

		for (dim* dimension = field->fld_array_info->ary_dimension; dimension;
			dimension = dimension->dim_next)
		{
			fprintf(gpreGlob.out_file, "%"SLONGFORMAT"..%"SLONGFORMAT, dimension->dim_lower,
					   dimension->dim_upper);
			if (dimension->dim_next)
				fprintf(gpreGlob.out_file, ", ");
		}
		if (datatype <= dtype_varying)
			fprintf(gpreGlob.out_file, ", 1..%d", field->fld_array->fld_length);

		fprintf(gpreGlob.out_file, "] of ");
	}
	else
	{
		datatype = field->fld_dtype;
		if (datatype <= dtype_varying)
			fprintf(gpreGlob.out_file, "%s [1..%d] of ", PACKED_ARRAY, field->fld_length);
	}

	TEXT s[64];

	switch (datatype)
	{
	case dtype_short:
		fprintf(gpreGlob.out_file, "%s;", SHORT_DCL);
		break;

	case dtype_long:
		fprintf(gpreGlob.out_file, "%s;", LONG_DCL);
		break;

	case dtype_date:
	case dtype_blob:
	case dtype_quad:
		fprintf(gpreGlob.out_file, "gds__quad;");
		break;

	case dtype_text:
		fprintf(gpreGlob.out_file, "char;");
		break;

	case dtype_real:
		fprintf(gpreGlob.out_file, "real;");
		break;

	case dtype_double:
		fprintf(gpreGlob.out_file, "double;");
		break;

	default:
		sprintf(s, "datatype %d unknown\n", field->fld_dtype);
		CPR_error(s);
		return;
	}
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_close( const act* action, USHORT column)
{
	if (action->act_error || (action->act_flags & ACT_sql))
		begin(column);

	const blb* blob;
	if (action->act_flags & ACT_sql)
	{
		column = gen_cursor_close(action, action->act_request, column);
		blob = (blb*) action->act_request->req_blobs;
	}
	else
		blob = (blb*) action->act_object;

	const TEXT* command = (action->act_type == ACT_blob_cancel) ? "CANCEL" : "CLOSE";
	printa(column, "GDS__%s_BLOB (%s, gds__%d);", command, status_vector(action), blob->blb_ident);

	if (action->act_flags & ACT_sql)
	{
		endp(column);
		column -= INDENT;
		ends(column);
		column -= INDENT;
		set_sqlcode(action, column);
		endp(column);
	}
}


//____________________________________________________________
//
//		End a blob FOR loop.
//

static void gen_blob_end(const act* action, USHORT column)
{
	const blb* blob = (blb*) action->act_object;
	printa(column + INDENT, "end;");
	if (action->act_error)
		printa(column, "GDS__CLOSE_BLOB (gds__status2, gds__%d);", blob->blb_ident);
	else
		printa(column, "GDS__CLOSE_BLOB (%s, gds__%d);", status_vector(0), blob->blb_ident);
	if (action->act_error)
		ends(column);
	else
		endp(column);
}


//____________________________________________________________
//
//		Make a blob FOR loop.
//

static void gen_blob_for( const act* action, USHORT column)
{

	gen_blob_open(action, column);
	if (action->act_error)
		printa(column, "if (gds__status[2] = 0) then begin");
	printa(column, "while (true) do begin");
	gen_get_segment(action, column + INDENT);
	printa(column + INDENT, "if ((gds__status[2] <> 0) and (gds__status[2] <> gds__segment)) then");
	printa(column + 2 * INDENT, "exit;");
}


//____________________________________________________________
//
//		Generate the call to open (or create) a blob.
//

static void gen_blob_open( const act* action, USHORT column)
{
	const TEXT* pattern1 =
		"GDS__%IFCREATE%ELOPEN%EN_BLOB2 (%V1, %RF%DH, %RF%RT, %RF%BH, %RF%FR, %N1, %RF%I1);";
	const TEXT* pattern2 =
		"GDS__%IFCREATE%ELOPEN%EN_BLOB2 (%V1, %RF%DH, %RF%RT, %RF%BH, %RF%FR, 0, 0);";

	if (gpreGlob.sw_auto && (action->act_flags & ACT_sql))
	{
		t_start_auto(action, action->act_request, status_vector(action), column);
		printa(column, "if %s <> nil then", request_trans(action, action->act_request));
		column += INDENT;
	}

	if ((action->act_error && (action->act_type != ACT_blob_for)) || action->act_flags & ACT_sql)
	{
		begin(column);
	}

	TEXT s[MAX_REF_SIZE];
	const blb* blob;
	const ref* reference;
	if (action->act_flags & ACT_sql)
	{
		column = gen_cursor_open(action, action->act_request, column);
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
	args.pat_condition = (action->act_type == ACT_blob_create); // open or create blob
	args.pat_vector1 = status_vector(action); // status vector
	args.pat_database = blob->blb_request->req_database; // database handle
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
		endp(column);
		column -= INDENT;
		endp(column);
		column -= INDENT;
		ends(column);
		column -= INDENT;
		if (gpreGlob.sw_auto)
		{
			endp(column);
			column -= INDENT;
		}
		set_sqlcode(action, column);
		if (action->act_type == ACT_blob_create)
		{
			printa(column, "if SQLCODE = 0 then");
			align(column + INDENT);
			fprintf(gpreGlob.out_file, "%s := %s;", reference->ref_value, s);
		}
	}
}


//____________________________________________________________
//
//		Callback routine for BLR pretty printer.
//

static void gen_blr(void* /*user_arg*/, SSHORT /*offset*/, const char* string)
{
	bool first_line = true;

	int indent = 2 * INDENT;
	const char* p = string;
	while (*p == ' ')
	{
		p++;
		indent++;
	}

	// Limit indentation to 192 characters

	indent = MIN(indent, 192);

	int length = strlen(p);
	while (length + indent > 255)
	{
		// if we did not find somewhere to break between the 200th and 256th character
		// just print out 256 characters

		const char* q = p;
		for (bool open_quote = false; (q - p + indent) < 255; q++)
		{
			if ((q - p + indent) > 220 && *q == ',' && !open_quote)
				break;
			if (*q == '\'' && *(q - 1) != '\\')
				open_quote = !open_quote;
		}
		q++;
		char buffer[256];
		strncpy(buffer, p, q - p);
		buffer[q - p] = 0;
		printa(indent, buffer);
		length = length - (q - p);
		p = q;
		if (first_line)
		{
			indent = MIN(indent + INDENT, 192);
			first_line = false;
		}
	}
	printa(indent, p);
}


//____________________________________________________________
//
//		Generate text to compile a request.
//

static void gen_compile( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	const gpre_dbb* db = request->req_database;
	const gpre_sym* symbol = db->dbb_name;

	if (gpreGlob.sw_auto)
	{
		const TEXT* filename = db->dbb_runtime;
		if (filename || !(db->dbb_flags & DBB_sqlca))
		{
			printa(column, "if %s = nil then", symbol->sym_string);
			make_ready(db, filename, status_vector(action), column + INDENT, 0);
		}
		if (action->act_error || (action->act_flags & ACT_sql))
			printa(column, "if (%s = nil) and (%s <> nil) then",
				   request_trans(action, request), symbol->sym_string);
		else
			printa(column, "if %s = nil then", request_trans(action, request));
		t_start_auto(action, request, status_vector(action), column + INDENT);
	}

	if ((action->act_error || (action->act_flags & ACT_sql)) && gpreGlob.sw_auto)
		printa(column, "if (%s = nil) and (%s <> nil) then",
			   request->req_handle, request_trans(action, request));
	else
		printa(column, "if %s = nil then", request->req_handle);

	align(column + INDENT);
	fprintf(gpreGlob.out_file, "GDS__COMPILE_REQUEST%s (%s, %s, %s, %d, gds__%d);\n",
			   (request->req_flags & REQ_exp_hand) ? "" : "2",
			   status_vector(action), symbol->sym_string, request->req_handle,
			   request->req_length, request->req_ident);

	// If blobs are present, zero out all of the blob handles.  After this
	// point, the handles are the user's responsibility

	for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		printa(column - INDENT, "gds__%d := nil;", blob->blb_ident);
}


//____________________________________________________________
//
//		Generate a call to create a database.
//

static void gen_create_database( const act* action, int column)
{
	const gpre_req* request = ((mdbb*) action->act_object)->mdbb_dpb_request;
	const gpre_dbb* db = (gpre_dbb*) request->req_database;
	align(column);

	if (request->req_length)
		fprintf(gpreGlob.out_file, "GDS__CREATE_DATABASE (%s, %" SIZEFORMAT ", '%s', %s, %d, gds__%d, 0);",
				   status_vector(action), strlen(db->dbb_filename),
				   db->dbb_filename, db->dbb_name->sym_string,
				   request->req_length, request->req_ident);
	else
		fprintf(gpreGlob.out_file, "GDS__CREATE_DATABASE (%s, %" SIZEFORMAT ", '%s', %s, 0, 0, 0);",
				   status_vector(action), strlen(db->dbb_filename),
				   db->dbb_filename, db->dbb_name->sym_string);

	const bool save_sw_auto = gpreGlob.sw_auto;
	gpreGlob.sw_auto = true;
	printa(column, "if (gds__status [2] = 0) then");
	begin(column);
	gen_ddl(action, column);
	ends(column);
	gpreGlob.sw_auto = save_sw_auto;
	column -= INDENT;
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static int gen_cursor_close( const act* action, const gpre_req* request, int column)
{
	const TEXT* pattern1 = "if %RIs <> nil then";
	const TEXT* pattern2 = "isc_dsql_free_statement (%V1, %RF%RIs, %N1);";

	PAT args;
	args.pat_request = request;
	args.pat_vector1 = status_vector(action);
	args.pat_value1 = 1;

	PATTERN_expand(column, pattern1, &args);
	column += INDENT;
	begin(column);
	PATTERN_expand(column, pattern2, &args);
	printa(column, "if (gds__status[2] = 0) then");
	column += INDENT;
	begin(column);

	return column;
}


//____________________________________________________________
//
//		Generate text to initialize a cursor.
//

static void gen_cursor_init( const act* action, int column)
{

	// If blobs are present, zero out all of the blob handles.  After this
	// point, the handles are the user's responsibility

	if (action->act_request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
		printa(column, "gds__%d := nil;", action->act_request->req_blobs->blb_ident);
}


//____________________________________________________________
//
//		Generate text to open an embedded SQL cursor.
//

static int gen_cursor_open( const act* action, const gpre_req* request, int column)
{
	TEXT s[MAX_CURSOR_SIZE];
	const TEXT *pattern1 =
		"if (%RIs = nil) and (%RH <> nil)%IF and (%DH <> nil)%EN then",
		*pattern2 = "if (%RIs = nil)%IF and (%DH <> nil)%EN then",
		*pattern3 = "isc_dsql_alloc_statement2 (%V1, %RF%DH, %RF%RIs);",
		*pattern4 = "if (%RIs <> nil)%IF and (%S3 <> nil)%EN then",
		*pattern5 = "isc_dsql_set_cursor_name (%V1, %RF%RIs, %S1, 0);",
		*pattern6 = "isc_dsql_execute_m (%V1, %RF%S3, %RF%RIs, 0, gds__null, %N2, 0, gds__null);";

	PAT args;
	args.pat_request = request;
	args.pat_database = request->req_database;
	args.pat_vector1 = status_vector(action);
	args.pat_condition = gpreGlob.sw_auto;
	args.pat_string1 = make_name(s, ((open_cursor*) action->act_object)->opn_cursor);
	args.pat_string3 = request_trans(action, request);
	args.pat_value2 = -1;

	PATTERN_expand(column, (action->act_type == ACT_open) ? pattern1 : pattern2, &args);
	PATTERN_expand(column + INDENT, pattern3, &args);
	PATTERN_expand(column, pattern4, &args);
	column += INDENT;
	begin(column);
	PATTERN_expand(column, pattern5, &args);
	printa(column, "if (gds__status[2] = 0) then");
	column += INDENT;
	begin(column);
	PATTERN_expand(column, pattern6, &args);
	printa(column, "if (gds__status[2] = 0) then");
	column += INDENT;
	begin(column);

	return column;
}


//____________________________________________________________
//
//		Generate insertion text for the database statement,
//		including the definitions of all gpreGlob.requests, and blob
//		ans port declarations for gpreGlob.requests in the main routine.
//

static void gen_database(/* const act* action,*/ int column)
{
	const gpre_req* request;

	if (global_first_flag)
		return;
	global_first_flag = true;

	fprintf(gpreGlob.out_file, "\n(**** GPRE Preprocessor Definitions ****)\n");
	fprintf(gpreGlob.out_file, "%%include '/firebird/include/gds.ins.pas';\n");

	int indent = column + INDENT;
	bool flag = true;

	for (request = gpreGlob.requests; request; request = request->req_next)
	{
		if (request->req_flags & REQ_local)
			continue;
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
		{
			if (flag)
			{
				flag = false;
				fprintf(gpreGlob.out_file, "type");
			}
			make_port(port, indent);
		}
	}
	fprintf(gpreGlob.out_file, "\nvar");
	for (request = gpreGlob.requests; request; request = request->req_routine)
	{
		if (request->req_flags & REQ_local)
			continue;
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			printa(indent, "gds__%d\t: gds__%dt;\t\t\t(* message *)",
				   port->por_ident, port->por_ident);

		for (blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			printa(indent, "gds__%d\t: gds__handle;\t\t\t(* blob handle *)", blob->blb_ident);
			printa(indent, "gds__%d\t: %s [1 .. %d] of char;\t(* blob segment *)",
				   blob->blb_buff_ident, PACKED_ARRAY, blob->blb_seg_length);
			printa(indent, "gds__%d\t: %s;\t\t\t(* segment length *)", blob->blb_len_ident, SHORT_DCL);
		}
	}

	const gpre_dbb* db;
	bool all_static = true;
	bool all_extern = true;

	for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		all_static = all_static && (db->dbb_scope == DBB_STATIC);
		all_extern = all_extern && (db->dbb_scope == DBB_EXTERN);
		if (db->dbb_scope == DBB_STATIC)
			printa(indent, "%s\t: %s gds__handle\t:= nil;	(* database handle *)",
				   db->dbb_name->sym_string, STATIC_STRING);
	}

	USHORT count = 0;
	for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		count++;
		for (const tpb* tpb_val = db->dbb_tpbs; tpb_val; tpb_val = tpb_val->tpb_dbb_next)
			gen_tpb(tpb_val, indent);
	}

	printa(indent, "gds__teb\t: array [1..%d] of gds__teb_t;\t(* transaction vector *)", count);

	// generate event parameter block for each event in module

	SSHORT max_count = 0;
	for (gpre_lls* stack_ptr = gpreGlob.events; stack_ptr; stack_ptr = stack_ptr->lls_next)
	{
		SSHORT event_count = gen_event_block(reinterpret_cast<act*>(stack_ptr->lls_object));
		max_count = MAX(event_count, max_count);
	}

	if (max_count)
	{
		printa(indent, "gds__events\t\t: %s array [1..%d] of %s;\t\t(* event vector *)",
			   STATIC_STRING, max_count, LONG_DCL);
		printa(indent, "gds__event_names\t\t: %s array [1..%d] of %s;\t\t(* event buffer *)",
			   STATIC_STRING, max_count, POINTER_DCL);
		printa(indent,
			   "gds__event_names2\t\t: %s %s [1..%d, 1..31] of char;\t\t(* event string buffer *)",
			   STATIC_STRING, PACKED_ARRAY, max_count);
	}

	bool array_flag = false;
	for (request = gpreGlob.requests; request; request = request->req_next)
	{
		gen_request(request, indent);
		// Array declarations
		if (request->req_type == REQ_slice)
			array_flag = true;

		const gpre_port* port = request->req_primary;
		if (port)
			for (const ref* reference = port->por_references; reference;
				reference = reference->ref_next)
			{
				if (reference->ref_flags & REF_fetch_array)
				{
					make_array_declaration(reference);
					array_flag = true;
				}
			}
	}

	if (array_flag)
		printa(indent, "gds__array_length\t: integer32;\t\t\t(* slice return value *)");

	printa(indent, "gds__null\t\t: ^gds__status_vector := nil;\t(* null status vector *)");
	printa(indent, "gds__blob_null\t: gds__quad := %s0,0%s;\t\t(* null blob id *)",
		   OPEN_BRACKET, CLOSE_BRACKET);

	if (all_static)
	{
		printa(indent, "gds__trans\t\t: %s gds__handle := nil;\t\t(* default transaction *)",
			   STATIC_STRING);
		printa(indent, "gds__status\t\t: %s gds__status_vector;\t\t(* status vector *)", STATIC_STRING);
		printa(indent, "gds__status2\t\t: %s gds__status_vector;\t\t(* status vector *)", STATIC_STRING);
		printa(indent, "SQLCODE\t: %s integer := 0;\t\t\t(* SQL status code *)", STATIC_STRING);
	}
	else
	{
		printa(column, "\nvar (gds__trans)");
		printa(indent, "gds__trans\t\t: gds__handle%s;\t\t(* default transaction *)",
			   all_extern ? "" : "\t:= nil");
		printa(column, "\nvar (gds__status)");
		printa(indent, "gds__status\t\t: gds__status_vector;\t\t(* status vector *)");
		printa(column, "\nvar (gds__status2)");
		printa(indent, "gds__status2\t\t: gds__status_vector;\t\t(* status vector *)");
		printa(column, "\nvar (SQLCODE)");
		printa(indent, "SQLCODE\t: integer%s;\t\t\t(* SQL status code *)",
			   all_extern ? "" : "\t:= 0");
	}
	printa(column, "\nvar (gds__window)");
	printa(indent, "gds__window\t\t:  gds__handle := nil;\t\t(* window handle *)");
	printa(column, "\nvar (gds__width)");
	printa(indent, "gds__width\t\t: %s := 80;\t(* window width *)", SHORT_DCL);
	printa(column, "\nvar (gds__height)");
	printa(indent, "gds__height\t\t: %s := 24;\t(* window height *)", SHORT_DCL);

	for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		if (db->dbb_scope != DBB_STATIC)
		{
			printa(column, "\nvar (%s)", db->dbb_name->sym_string);
			printa(indent, "%s\t: gds__handle%s;	(* database handle *)",
				   db->dbb_name->sym_string, (db->dbb_scope == DBB_EXTERN) ? "" : "\t:= nil");
		}
	}

	printa(column, " ");
	printa(column, "(**** end of GPRE definitions ****)");
}


//____________________________________________________________
//
//		Generate a call to update metadata.
//

static void gen_ddl( const act* action, int column)
{
	if (gpreGlob.sw_auto)
	{
		printa(column, "if (gds__trans = nil) then");
		column += INDENT;
		t_start_auto(action, 0, status_vector(action), column + INDENT);
		column -= INDENT;
	}

	// Set up command type for call to RDB$DDL

	const gpre_req* request = action->act_request;

	if (gpreGlob.sw_auto)
	{
		printa(column, "if (gds__trans <> nil) then");
		column += INDENT;
	}

	align(column);
	fprintf(gpreGlob.out_file, "GDS__DDL (%s, %s, gds__trans, %d, gds__%d);",
			   status_vector(action),
			   request->req_database->dbb_name->sym_string,
			   request->req_length, request->req_ident);

	if (gpreGlob.sw_auto)
	{
		column -= INDENT;
		printa(column, "if (gds__status [2] = 0) then");
		printa(column + INDENT, "GDS__COMMIT_TRANSACTION (%s, gds__trans);", status_vector(action));
		printa(column, "if (gds__status [2] <> 0) then");
		printa(column + INDENT, "GDS__ROLLBACK_TRANSACTION (gds__null^ , gds__trans);");
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
	align(column);

	fprintf(gpreGlob.out_file, "GDS__DROP_DATABASE (%s, %" SIZEFORMAT ", '%s', RDB$K_DB_TYPE_GDS);",
			   status_vector(action),
			   strlen(db->dbb_filename), db->dbb_filename);
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
	printa(column, "isc_embed_dsql_close (gds__status, %s);", make_name(s, statement->dyn_cursor_name));
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
	printa(column, "isc_embed_dsql_declare (gds__status, %s, %s);",
		   make_name(s1, statement->dyn_statement_name),
		   make_name(s2, statement->dyn_cursor_name));
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a dynamic SQL statement.
//

static void gen_dyn_describe(const act* action, int column, bool bind_flag)
{
	TEXT s[MAX_CURSOR_SIZE];

	const dyn* statement = (dyn*) action->act_object;
	printa(column, "isc_embed_dsql_describe%s (gds__status, %s, %d, %s %s);",
		   bind_flag ? "_bind" : "",
		   make_name(s, statement->dyn_statement_name),
		   gpreGlob.sw_sql_dialect, REF_PAR, statement->dyn_sqlda);
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

	const dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
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
		printa(column, "if (%s = nil) then", transaction);
		column += INDENT;
		t_start_auto(action, request, status_vector(action), column + INDENT);
		column -= INDENT;
	}

	TEXT s[MAX_CURSOR_SIZE];
	make_name(s, statement->dyn_cursor_name);

	gpre_nod* var_list;
	if (var_list = statement->dyn_using)
	{
		printa(column, "gds__sqlda.sqln = %s;", gpreGlob.sw_dyn_using);
		printa(column, "gds__sqlda.sqld = %s;", gpreGlob.sw_dyn_using);
		for (int i = 0; i < var_list->nod_count; i++)
			asgn_sqlda_from(reinterpret_cast<ref*>(var_list->nod_arg[i]), i, s, column);
	}

	printa(column,
		   statement->dyn_sqlda2 ?
				"isc_embed_dsql_execute2 (gds__status, %s, %s, %d, %s %s, %s %s);" :
				"isc_embed_dsql_execute (gds__status, %s, %s, %d, %s %s);",
		   transaction, make_name(s, statement->dyn_statement_name),
		   gpreGlob.sw_sql_dialect, REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^",
		   REF_PAR,
		   statement->dyn_sqlda2 ? statement->dyn_sqlda2 : "gds__null^");
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
	printa(column, "SQLCODE := isc_embed_dsql_fetch (gds__status, %s, %d, %s %s);",
		   make_name(s, statement->dyn_cursor_name),
		   gpreGlob.sw_sql_dialect,
		   REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^");
	printa(column,
		   "if sqlcode <> 100 then sqlcode := gds__sqlcode (gds__status);");
}


//____________________________________________________________
//
//		Generate code for an EXECUTE IMMEDIATE dynamic SQL statement.
//

static void gen_dyn_immediate( const act* action, int column)
{
	gpre_req* request;
	gpre_req req_const;

	const dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
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
		printa(column, "if (%s = nil) then", transaction);
		column += INDENT;
		t_start_auto(action, request, status_vector(action), column + INDENT);
		column -= INDENT;
	}

	const gpre_dbb* database = statement->dyn_database;
	printa(column,
		   statement->dyn_sqlda2 ?
				"isc_embed_dsql_execute_immed2 (gds__status, %s, %s, %s(%s), %s, %d, %s %s, %s %s);" :
				"isc_embed_dsql_execute_immed (gds__status, %s, %s, %s(%s), %s, %d, %s %s);",
		   database->dbb_name->sym_string, transaction, SIZEOF,
		   statement->dyn_string, statement->dyn_string, gpreGlob.sw_sql_dialect,
		   REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^",
		   REF_PAR,
		   statement->dyn_sqlda2 ? statement->dyn_sqlda2 : "gds__null^");
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
	printa(column, "isc_embed_dsql_insert (gds__status, %s, %d, %s %s);",
		   make_name(s, statement->dyn_cursor_name),
		   gpreGlob.sw_sql_dialect,
		   REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^");

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

	const dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
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
		printa(column, "if (%s = nil) then", transaction);
		column += INDENT;
		t_start_auto(action, request, status_vector(action), column + INDENT);
		column -= INDENT;
	}

	TEXT s[MAX_CURSOR_SIZE];
	make_name(s, statement->dyn_cursor_name);

	if (const gpre_nod* var_list = statement->dyn_using)
	{
		printa(column, "gds__sqlda.sqln = %d;", gpreGlob.sw_dyn_using);
		printa(column, "gds__sqlda.sqld = %d;", gpreGlob.sw_dyn_using);
		for (int i = 0; i < var_list->nod_count; i++)
			asgn_sqlda_from(reinterpret_cast<const ref*>(var_list->nod_arg[i]), i, s, column);
	}

	printa(column,
		   statement->dyn_sqlda2 ?
				"isc_embed_dsql_open2 (gds__status, %s, %s, %d, %s %s, %s %s);" :
				"isc_embed_dsql_open (gds__status, %s, %s, %d, %s %s);",
		   s,
		   transaction,
		   gpreGlob.sw_sql_dialect,
		   REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^",
		   REF_PAR,
		   statement->dyn_sqlda2 ? statement->dyn_sqlda2 : "gds__null^");
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

    const dyn* statement = (dyn*) action->act_object;
	const TEXT* transaction;
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
		printa(column, "if (%s = nil) then", transaction);
		column += INDENT;
		t_start_auto(action, request, status_vector(action), column + INDENT);
		column -= INDENT;
	}

	const gpre_dbb* database = statement->dyn_database;
	TEXT s[MAX_CURSOR_SIZE];
	printa(column,
		   "isc_embed_dsql_prepare (gds__status, %s, transaction, %s, %s(%s), %s, %d, %s %s);",
		   database->dbb_name->sym_string, transaction,
		   make_name(s, statement->dyn_statement_name),
		   SIZEOF, statement->dyn_string, statement->dyn_string,
		   gpreGlob.sw_sql_dialect, REF_PAR,
		   statement->dyn_sqlda ? statement->dyn_sqlda : "gds__null^");
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for END_MODIFY.
//

static void gen_emodify( const act* action, int column)
{
	TEXT s1[MAX_REF_SIZE], s2[MAX_REF_SIZE];

	const upd* modify = (upd*) action->act_object;

	for (const ref* reference = modify->upd_port->por_references; reference;
		 reference = reference->ref_next)
	{
		const ref* source = reference->ref_source;
		if (!source)
			continue;
		gpre_fld* field = reference->ref_field;
		align(column);
		fprintf(gpreGlob.out_file, "%s := %s;",
				   gen_name(s1, reference, true), gen_name(s2, source, true));
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
	const gpre_req* request = action->act_request;
	// if we did a store ... returning_values aka store2
	// just wrap up pending error
	if (request->req_type == REQ_store2)
	{
		if (action->act_error || (action->act_flags & ACT_sql))
			endp(column);
		return;
	}

	if (action->act_error)
		column += INDENT;
	gen_start(action, request->req_primary, column);
	if (action->act_error || (action->act_flags & ACT_sql))
		endp(column);
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_endfor( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	column += INDENT;

	if (request->req_sync)
		gen_send(action, request->req_sync, column);

	gen_receive(action, column, request->req_primary);

	endp(column);
	if (action->act_error || (action->act_flags & ACT_sql))
		endp(column);
}


//____________________________________________________________
//
//		Generate substitution text for ERASE.
//

static void gen_erase( const act* action, int column)
{
	if (action->act_error || (action->act_flags & ACT_sql))
		begin(column);

	const upd* erase = (upd*) action->act_object;
	gen_send(action, erase->upd_port, column);

	if (action->act_flags & ACT_sql)
		endp(column);
}


//____________________________________________________________
//
//		Generate event parameter blocks for use
//		with a particular call to gds__event_wait.
//

static SSHORT gen_event_block( const act* action)
{
	gpre_nod* init = (gpre_nod*) action->act_object;

	int ident = CMP_next_ident();
	init->nod_arg[2] = (gpre_nod*)(IPTR) ident;

	printa(INDENT, "gds__%da\t\t: ^char;\t\t(* event parameter block *)", ident);
	printa(INDENT, "gds__%db\t\t: ^char;\t\t(* result parameter block *)", ident);
	printa(INDENT, "gds__%dl\t\t: %s;\t\t(* length of event parameter block *)", ident, SHORT_DCL);

	gpre_nod* list = init->nod_arg[1];
	return list->nod_count;
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_INIT.
//

static void gen_event_init( const act* action, int column)
{
	const TEXT* pattern1 =
		"gds__%N1l := GDS__EVENT_BLOCK_A (%RFgds__%N1a, %RFgds__%N1b, %N2, %RFgds__event_names%RE);";
	const TEXT* pattern2 = "%S1 (%V1, %RF%DH, gds__%N1l, gds__%N1a, gds__%N1b);";
	const TEXT* pattern3 = "%S2 (gds__events, gds__%N1l, gds__%N1a, gds__%N1b);";

	if (action->act_error)
		begin(column);
	begin(column);

	gpre_nod* init = (gpre_nod*) action->act_object;
	gpre_nod* event_list = init->nod_arg[1];

	PAT args;
	args.pat_database = (gpre_dbb*) init->nod_arg[3];
	args.pat_vector1 = status_vector(action);
	args.pat_value1 = (int)(IPTR) init->nod_arg[2];
	args.pat_value2 = (int) event_list->nod_count;
	args.pat_string1 = GDS_EVENT_WAIT;
	args.pat_string2 = GDS_EVENT_COUNTS;

	// generate call to dynamically generate event blocks

	TEXT variable[MAX_REF_SIZE];
	gpre_nod** ptr;
	gpre_nod** end;
	SSHORT count;
	for (ptr = event_list->nod_arg, count = 0, end = ptr + event_list->nod_count; ptr < end; ptr++)
	{
		count++;
		const gpre_nod* node = *ptr;
		if (node->nod_type == nod_field)
		{
			const ref* reference = (const ref*) node->nod_arg[0];
			gen_name(variable, reference, true);
			printa(column, "gds__event_names2[%d] := %s;", count, variable);
		}
		else
			printa(column, "gds__event_names2[%d] := %s;", count, node->nod_arg[0]);

		printa(column, "gds__event_names[%d] := %s (gds__event_names2[%d]);",
			   count, ISC_BADDRESS, count);
	}

	PATTERN_expand(column, pattern1, &args);

	// generate actual call to event_wait

	PATTERN_expand(column, pattern2, &args);

	// get change in event counts, copying event parameter block for reuse

	PATTERN_expand(column, pattern3, &args);

	if (action->act_error)
		endp(column);
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for EVENT_WAIT.
//

static void gen_event_wait( const act* action, int column)
{
	const TEXT* pattern1 = "%S1 (%V1, %RF%DH, gds__%N1l, gds__%N1a, gds__%N1b);";
	const TEXT* pattern2 = "%S2 (gds__events, gds__%N1l, gds__%N1a, gds__%N1b);";

	if (action->act_error)
		begin(column);
	begin(column);

	gpre_sym* event_name = (gpre_sym*) action->act_object;

	// go through the stack of gpreGlob.events, checking to see if the
	// event has been initialized and getting the event identifier

	const gpre_dbb* database = NULL;
	int ident = -1;
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
	args.pat_string1 = GDS_EVENT_WAIT;
	args.pat_string2 = GDS_EVENT_COUNTS;

	// generate calls to wait on the event and to fill out the gpreGlob.events array

	PATTERN_expand(column, pattern1, &args);
	PATTERN_expand(column, pattern2, &args);

	if (action->act_error)
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
	const gpre_req* request = action->act_request;

	if (request->req_sync)
	{
		gen_send(action, request->req_sync, column);
		printa(column, "if SQLCODE = 0 then");
		column += INDENT;
		begin(column);
	}

	TEXT s[MAX_REF_SIZE];
	gen_receive(action, column, request->req_primary);
	printa(column, "if SQLCODE = 0 then");
	column += INDENT;
	printa(column, "if %s <> 0 then", gen_name(s, request->req_eof, true));
	column += INDENT;
	begin(column);

	if (gpre_nod* var_list = (gpre_nod*) action->act_object)
		for (int i = 0; i < var_list->nod_count; i++)
		{
			align(column);
			asgn_to(action, reinterpret_cast<ref*>(var_list->nod_arg[i]), column);
		}

	endp(column);
	printa(column - INDENT, "else");
	printa(column, "SQLCODE := 100;");
	if (request->req_sync)
	{
		column -= INDENT;
		endp(column);
	}
}


//____________________________________________________________
//
//		Generate substitution text for FINISH
//

static void gen_finish( const act* action, int column)
{
	const gpre_dbb* db = NULL;

	if (gpreGlob.sw_auto || ((action->act_flags & ACT_sql) && (action->act_type != ACT_disconnect)))
	{
		printa(column, "if gds__trans <> nil");
		printa(column + 4, "then GDS__%s_TRANSACTION (%s, gds__trans);",
			   (action->act_type != ACT_rfinish) ? "COMMIT" : "ROLLBACK",
			   status_vector(action));
	}

	// Got rid of tests of gds__trans <> nil which were causing the skipping
	// of trying to detach the databases.  Related to bug#935.  mao 6/22/89

	for (rdy* ready = (rdy*) action->act_object; ready; ready = ready->rdy_next)
	{
		db = ready->rdy_database;
		printa(column, "GDS__DETACH_DATABASE (%s, %s);",
			   status_vector(action), db->dbb_name->sym_string);
	}

	if (!db)
	{
		for (db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
			if ((action->act_error || (action->act_flags & ACT_sql)) &&
				(db != gpreGlob.isc_databases))
			{
				printa(column, "if (%s <> nil) and (gds__status[2] = 0) then", db->dbb_name->sym_string);
			}
			else
				printa(column, "if %s <> nil then", db->dbb_name->sym_string);
			printa(column + INDENT, "GDS__DETACH_DATABASE (%s, %s);",
				   status_vector(action), db->dbb_name->sym_string);
		}
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for FOR statement.
//

static void gen_for( const act* action, int column)
{
	gpre_port* port;
	const gpre_req* request;
	const ref* reference;

	gen_s_start(action, column);
	request = action->act_request;

	if (action->act_error || (action->act_flags & ACT_sql))
		printa(column, "if gds__status[2] = 0 then begin");

	TEXT s[MAX_REF_SIZE];
	gen_receive(action, column, request->req_primary);
	if (action->act_error || (action->act_flags & ACT_sql))
		printa(column, "while (%s <> 0) and (gds__status[2] = 0) do",
			   gen_name(s, request->req_eof, true));
	else
		printa(column, "while %s <> 0 do",
			   gen_name(s, request->req_eof, true));
	column += INDENT;
	begin(column);

	if (port = action->act_request->req_primary)
		for (reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field->fld_array_info)
				gen_get_or_put_slice(action, reference, true, column);
		}
}


//____________________________________________________________
//
//		Generate a call to gds__get_slice
//		or gds__put_slice for an array.
//

static void gen_get_or_put_slice(const act* action,
								 const ref* reference,
								 bool get,
								 int column)
{
	PAT args;
	const TEXT *pattern1 =
		"GDS__GET_SLICE (%V1, %RF%DH%RE, %RF%S1%RE, %S2, %N1, %S3, %N2, %S4, %L1, %S5, %S6);\n";
	const TEXT *pattern2 =
		"GDS__PUT_SLICE (%V1, %RF%DH%RE, %RF%S1%RE, %S2, %N1, %S3, %N2, %S4, %L1, %S5);\n";

	if (!(reference->ref_flags & REF_fetch_array))
		return;

	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = action->act_request->req_database;	// database handle
	args.pat_string1 = action->act_request->req_trans;	// transaction handle

	TEXT s1[MAX_REF_SIZE], s2[MAX_REF_SIZE], s3[MAX_REF_SIZE], s4[MAX_REF_SIZE];
	gen_name(s1, reference, true);	// blob handle
	args.pat_string2 = s1;

	args.pat_value1 = reference->ref_sdl_length;	// slice descr. length

	sprintf(s2, "gds__%d", reference->ref_sdl_ident);	// slice description
	args.pat_string3 = s2;

	args.pat_value2 = 0;		// parameter length

	sprintf(s3, "0");			// parameter
	args.pat_string4 = s3;

	args.pat_long1 = reference->ref_field->fld_array_info->ary_size;
	// slice size

	if (action->act_flags & ACT_sql) {
		args.pat_string5 = reference->ref_value;
	}
	else
	{
		sprintf(s4, "gds__%d", reference->ref_field->fld_array_info->ary_ident);
		args.pat_string5 = s4;	// array name
	}

	args.pat_string6 = "gds__array_length";	// return length

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
	PAT args;
	const ref* into;
	const TEXT *pattern1 =
		"%IFgds__status[2] := %ENGDS__GET_SEGMENT (%V1, %BH, %I1, %S1 (%I2), %RF%I2);";

	if (action->act_error && (action->act_type != ACT_blob_for))
		begin(column);

	if (action->act_flags & ACT_sql)
		blob = (blb*) action->act_request->req_blobs;
	else
		blob = (blb*) action->act_object;

	args.pat_blob = blob;
	args.pat_vector1 = status_vector(action);
	args.pat_condition = true;
	args.pat_ident1 = blob->blb_len_ident;
	args.pat_ident2 = blob->blb_buff_ident;
	args.pat_string1 = SIZEOF;
	PATTERN_expand(column, pattern1, &args);

	if (action->act_flags & ACT_sql)
	{
		into = action->act_object;
		set_sqlcode(action, column);
		printa(column, "if (SQLCODE = 0) or (SQLCODE = 101) then");
		column += INDENT;
		begin(column);
		align(column);
		fprintf(gpreGlob.out_file, "gds__ftof (gds__%d, gds__%d, %s, gds__%d);",
				   blob->blb_buff_ident, blob->blb_len_ident,
				   into->ref_value, blob->blb_len_ident);
		if (into->ref_null_value)
		{
			align(column);
			fprintf(gpreGlob.out_file, "%s := gds__%d;", into->ref_null_value, blob->blb_len_ident);
		}
		endp(column);
		column -= INDENT;
	}
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_loop( const act* action, int column)
{
	const gpre_req* request;
	gpre_port* port;
	TEXT name[MAX_REF_SIZE];

	gen_s_start(action, column);
	request = action->act_request;
	port = request->req_primary;
	printa(column, "if SQLCODE = 0 then");
	column += INDENT;
	begin(column);
	gen_receive(action, column, port);
	gen_name(name, port->por_references, true);
	printa(column, "if (SQLCODE = 0) and (%s = 0)", name);
	printa(column + INDENT, "then SQLCODE := 100;");
	endp(column);
	column -= INDENT;
}


//____________________________________________________________
//
//		Generate a name for a reference.  Name is constructed from
//		port and parameter idents.
//

static TEXT *gen_name(TEXT* const string, const ref* reference, bool as_blob)
{
	if (reference->ref_field->fld_array_info && !as_blob)
		fb_utils::snprintf(string, MAX_REF_SIZE, "gds__%d",
				reference->ref_field->fld_array_info->ary_ident);
	else
		fb_utils::snprintf(string, MAX_REF_SIZE, "gds__%d.gds__%d",
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
	if ((err_action->act_type == ACT_get_segment) ||
		(err_action->act_type == ACT_put_segment) ||
		(err_action->act_type == ACT_endblob))
			printa(column,
				   "if (gds__status [2] <> 0) and (gds__status[2] <> gds__segment) and (gds__status[2] <> gds__segstr_eof) then");
	else
		printa(column, "if (gds__status [2] <> 0) then");
	column += INDENT;
	begin(column);
}


//____________________________________________________________
//
//		Generate code for an EXECUTE PROCEDURE.
//

static void gen_procedure( const act* action, int column)
{
	column += INDENT;
	const gpre_req* request = action->act_request;
	gpre_port* in_port = request->req_vport;
	gpre_port* out_port = request->req_primary;

	PAT args;
	args.pat_database = request->req_database;
	args.pat_request = action->act_request;
	args.pat_vector1 = status_vector(action);
	args.pat_request = request;
	args.pat_port = in_port;
	args.pat_port2 = out_port;

	const TEXT* pattern;
	if (in_port && in_port->por_length)
		pattern =
			"isc_transact_request (%V1, %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RI, %VF%PL%VE, %RF%PI%RE, %VF%QL%VE, %RF%QI%RE);";
	else
		pattern =
			"isc_transact_request (%V1, %RF%DH%RE, %RF%RT%RE, %VF%RS%VE, %RI, %VF0%VE, 0, %VF%QL%VE, %RF%QI%RE);";


	// Get database attach and transaction started

	if (gpreGlob.sw_auto)
		t_start_auto(action, 0, status_vector(action), column);

	// Move in input values

	asgn_from(action, request->req_values, column);

	// Execute the procedure

	PATTERN_expand(column, pattern, &args);

	set_sqlcode(action, column);

	printa(column, "if SQLCODE = 0 then");
	column += INDENT;
	begin(column);

	// Move out output values

	asgn_to_proc(request->req_references, column);
	endp(column);
}


//____________________________________________________________
//
//		Generate the code to do a put segment.
//

static void gen_put_segment( const act* action, int column)
{
	blb* blob;
	PAT args;
	const ref* from;
	const TEXT *pattern1 =
		"%IFgds__status[2] := %ENGDS__PUT_SEGMENT (%V1, %BH, %I1, %I2);";

	if (!action->act_error)
		begin(column);
	if (action->act_error || (action->act_flags & ACT_sql))
		begin(column);

	if (action->act_flags & ACT_sql)
	{
		blob = (blb*) action->act_request->req_blobs;
		from = action->act_object;
		align(column);
		fprintf(gpreGlob.out_file, "gds__%d := %s;",
				   blob->blb_len_ident, from->ref_null_value);
		align(column);
		fprintf(gpreGlob.out_file, "gds__ftof (%s, gds__%d, gds__%d, gds__%d);",
				   from->ref_value, blob->blb_len_ident,
				   blob->blb_buff_ident, blob->blb_len_ident);
	}
	else
		blob = (blb*) action->act_object;

	args.pat_blob = blob;
	args.pat_vector1 = status_vector(action);
	args.pat_condition = true;
	args.pat_ident1 = blob->blb_len_ident;
	args.pat_ident2 = blob->blb_buff_ident;
	PATTERN_expand(column, pattern1, &args);

	set_sqlcode(action, column);

	if (action->act_flags & ACT_sql)
		endp(column);
}


//____________________________________________________________
//
//		Generate BLR in raw, numeric form.  Ughly but dense.
//

static void gen_raw(const UCHAR* blr, int request_length) //, int column)
{
	TEXT buffer[80];

	TEXT* p = buffer;
	const TEXT* const limit = buffer + 60;

	for (const UCHAR* const end = blr + request_length - 1; blr <= end; blr++)
	{
		const UCHAR c = *blr;
		if ((c >= 'A' && c <= 'Z') || c == '$' || c == '_')
			sprintf(p, "'%c'", c);
		else
			sprintf(p, "chr(%d)", c);
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
			printa(column, "if (gds__status[2] = 0) then begin");
		}
		make_ready(db, filename, vector, column, ready->rdy_request);
		if ((action->act_error || (action->act_flags & ACT_sql)) &&
			ready != (rdy*) action->act_object)
		{
			endp(column);
		}
	}
	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate receive call for a port.
//

static void gen_receive( const act* action, int column, const gpre_port* port)
{
	align(column);

	const gpre_req* request = action->act_request;
	fprintf(gpreGlob.out_file, "GDS__RECEIVE (%s, %s, %d, %d, %sgds__%d, %s);",
			   status_vector(action),
			   request->req_handle,
			   port->por_msg_number,
			   port->por_length,
			   REF_PAR, port->por_ident, request->req_request_level);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for RELEASE_REQUESTS
//		For active databases, call gds__release_request.
//		for all others, just zero the handle.  For the
//		release request calls, ignore error returns, which
//		are likely if the request was compiled on a database
//		which has been released and re-readied.  If there is
//		a serious error, it will be caught on the next statement.
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
			printa(column, "if %s <> NIL then", db->dbb_name->sym_string);
			printa(column + INDENT, "gds__release_request (gds__status, %s);",
				   request->req_handle);
			printa(column, "%s := NIL;", request->req_handle);
		}
	}
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_request( const gpre_req* request, int column)
{
	// generate request handle, blob handles, and ports

	const TEXT* sw_volatile = FB_DP_VOLATILE;
	printa(column, " ");

	if (!(request->req_flags & (REQ_exp_hand | REQ_sql_blob_open | REQ_sql_blob_create)) &&
		request->req_type != REQ_slice && request->req_type != REQ_procedure)
	{
		printa(column, "%s\t: %s gds__handle := nil;\t\t(* request handle *)",
			   request->req_handle, sw_volatile);
	}

	if (request->req_flags & (REQ_sql_blob_open | REQ_sql_blob_create))
		printa(column, "gds__%ds\t: %s gds__handle := nil;\t\t(* SQL statement handle *)",
			   request->req_ident, sw_volatile);

	// generate actual BLR string

	if (request->req_length)
	{
		printa(column, " ");
		if (request->req_flags & REQ_sql_cursor)
			printa(column, "gds__%ds\t: %s gds__handle := nil;\t\t(* SQL statement handle *)",
				   request->req_ident, sw_volatile);
		printa(column, "gds__%dl\t: %s := %d;\t\t(* request length *)",
			   request->req_ident, SHORT_DCL, request->req_length);
		printa(column, "gds__%d\t: %s [1..%d] of char := %s",
			   request->req_ident, PACKED_ARRAY, request->req_length, OPEN_BRACKET);
		const TEXT* string_type = "BLR";
		if (gpreGlob.sw_raw)
		{
			gen_raw(request->req_blr, request->req_length); //, column);
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
				break;
			}
		}
		else
			switch (request->req_type)
			{
			case REQ_create_database:
			case REQ_ready:
				string_type = "DPB";
				if (PRETTY_print_cdb(request->req_blr, gen_blr, 0, 1))
					CPR_error("internal error during parameter generation");
				break;

			case REQ_ddl:
				string_type = "DYN";
				if (PRETTY_print_dyn(request->req_blr, gen_blr, 0, 1))
					CPR_error("internal error during dynamic DDL generation");
				break;
			case REQ_slice:
				string_type = "SDL";
				if (PRETTY_print_sdl(request->req_blr, gen_blr, 0, 1))
					CPR_error("internal error during SDL generation");
				break;

			default:
				string_type = "BLR";
				if (fb_print_blr(request->req_blr, request->req_length, gen_blr, 0, 1))
					CPR_error("internal error during BLR generation");
				break;
			}
		printa(column, "%s;\t(* end of %s string for request gds__%d *)\n",
			   CLOSE_BRACKET, string_type, request->req_ident);
	}

	// Print out slice description language if there are arrays associated with request

	for (const gpre_port* port = request->req_ports; port; port = port->por_next)
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_sdl)
			{
				printa(column, "gds__%d\t: %s [1..%d] of char := %s",
					   reference->ref_sdl_ident, PACKED_ARRAY,
					   reference->ref_sdl_length, OPEN_BRACKET);
				if (gpreGlob.sw_raw)
					gen_raw(reference->ref_sdl, reference->ref_sdl_length); //, column);
				else if (PRETTY_print_sdl(reference->ref_sdl, gen_blr, 0, 1))
					CPR_error("internal error during SDL generation");
				printa(column, "%s; \t(* end of SDL string for gds__%d *)\n",
					   CLOSE_BRACKET, reference->ref_sdl_ident);
			}
		}

	// Print out any blob parameter blocks required

	for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		if (blob->blb_bpb_length)
		{
			printa(column, "gds__%d\t: %s [1..%d] of char := %s",
				   blob->blb_bpb_ident, PACKED_ARRAY, blob->blb_bpb_length, OPEN_BRACKET);
			gen_raw(blob->blb_bpb, blob->blb_bpb_length); //, column);
			printa(column, "%s;\n", CLOSE_BRACKET);
		}
	// If this is GET_SLICE/PUT_SLICE, allocate some variables

	if (request->req_type == REQ_slice)
	{
		printa(column, "gds__%dv\t: array [1..%d] of %s;",
			   request->req_ident, MAX(request->req_slice->slc_parameters, 1), LONG_DCL);
		printa(column, "gds__%ds\t: %s;", request->req_ident, LONG_DCL);
	}
}


//____________________________________________________________
//
//		Generate receive call for a port
//		in a store2 statement.
//

static void gen_return_value( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	if (action->act_pair->act_error)
		column += INDENT;
	gen_start(action, request->req_primary, column);
	const upd* update = (upd*) action->act_object;
	const ref* reference = update->upd_references;
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

	for (const gpre_req* request = (const gpre_req*) action->act_object; request;
		 request = request->req_routine)
	{
		gpre_port* port;

		for (port = request->req_ports; port; port = port->por_next)
		{
			printa(column - INDENT, "type");
			make_port(port, column);
		}

		// Only write a var reserved word if there are variables which will be
		// associated with a var section.  Fix for bug#809.  mao  03/22/89

		if (request->req_ports) {
			printa(column - INDENT, "var");
		}

		for (port = request->req_ports; port; port = port->por_next)
			printa(column, "gds__%d\t: gds__%dt;\t\t\t(* message *)", port->por_ident, port->por_ident);

		for (const blb* blob = request->req_blobs; blob; blob = blob->blb_next)
		{
			printa(column, "gds__%d\t: gds__handle;\t\t\t(* blob handle *)", blob->blb_ident);
			printa(column, "gds__%d\t: %s [1 .. %d] of char;\t(* blob segment *)",
				   blob->blb_buff_ident, PACKED_ARRAY, blob->blb_seg_length);
			printa(column, "gds__%d\t: %s;\t\t\t(* segment length *)", blob->blb_len_ident, SHORT_DCL);
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
	if (action->act_error)
		begin(column);

	const gpre_req* request = action->act_request;

	if (action->act_type == ACT_close)
		column = gen_cursor_close(action, request, column);

	printa(column, "GDS__UNWIND_REQUEST (%s, %s, %s);",
		   status_vector(action), request->req_handle, request->req_request_level);

	if (action->act_type == ACT_close)
	{
		endp(column);
		column -= INDENT;
		ends(column);
		column -= INDENT;
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for FETCH.
//

static void gen_s_fetch( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	if (request->req_sync)
		gen_send(action, request->req_sync, column);

	gen_receive(action, column, request->req_primary);
	if (!action->act_pair && !action->act_error)
		endp(column);
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_s_start( const act* action, int column)
{
	const gpre_req* request = action->act_request;

	gen_compile(action, column);

	if (action->act_type == ACT_open)
		column = gen_cursor_open(action, request, column);

	const gpre_port* port = request->req_vport;
	if (port)
		asgn_from(action, port->por_references, column);

	if (action->act_error || (action->act_flags & ACT_sql))
	{
		make_ok_test(action, request, column);
		column += INDENT;
	}

	gen_start(action, port, column);

	if (action->act_error || (action->act_flags & ACT_sql))
		column -= INDENT;

	if (action->act_type == ACT_open)
	{
		endp(column);
		column -= INDENT;
		endp(column);
		column -= INDENT;
		ends(column);
		column -= INDENT;
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Substitute for a segment, segment length, or blob handle.
//

static void gen_segment( const act* action, int column)
{
	const blb* blob = (blb*) action->act_object;

	printa(column, "gds__%d",
		   (action->act_type == ACT_segment) ?
		   		blob->blb_buff_ident : (action->act_type == ACT_segment_length) ?
		   			blob->blb_len_ident : blob->blb_ident);
}


//____________________________________________________________
//
//

static void gen_select( const act* action, int column)
{
	TEXT name[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	const gpre_port* port = request->req_primary;
	gen_name(name, request->req_eof, true);

	gen_s_start(action, column);
	begin(column);
	gen_receive(action, column, port);
	printa(column, "if SQLCODE = 0 then", name);
	column += INDENT;
	printa(column, "if %s <> 0 then", name);
	column += INDENT;

	begin(column);
	const gpre_nod* var_list = (gpre_nod*) action->act_object;
	if (var_list)
	{
		for (int i = 0; i < var_list->nod_count; i++)
		{
			align(column);
			asgn_to(action, reinterpret_cast<const ref*>(var_list->nod_arg[i]), column);
		}
	}

	printa(column - INDENT, "else");
	printa(column, "SQLCODE := 100;");
	column -= INDENT;
	ends(column);
}


//____________________________________________________________
//
//		Generate a send or receive call for a port.
//

static void gen_send( const act* action, const gpre_port* port, int column)
{
	const gpre_req* request = action->act_request;
	align(column);

	fprintf(gpreGlob.out_file, "GDS__SEND (%s, %s, %d, %d, gds__%d, %s);",
			   status_vector(action),
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
		"GDS__GET_SLICE (%V1, %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5%RE, %RF%S6%RE);";
	const TEXT* pattern2 =
		"GDS__PUT_SLICE (%V1, %RF%DH%RE, %RF%RT%RE, %RF%FR%RE, %N1, \
%I1, %N2, %I1v, %I1s, %RF%S5%RE);";

	const gpre_req* request = action->act_request;
	const slc* slice = (slc*) action->act_object;
	const gpre_req* parent_request = slice->slc_parent_request;

	// Compute array size

	printa(column, "gds__%ds := %d", request->req_ident,
		   slice->slc_field->fld_array->fld_length);

	const slc::slc_repeat *tail, *end;
	for (tail = slice->slc_rpt, end = tail + slice->slc_dimensions; tail < end; ++tail)
	{
		if (tail->slc_upper != tail->slc_lower)
		{
			const ref* lower = (const ref*) tail->slc_lower->nod_arg[0];
			const ref* upper = (const ref*) tail->slc_upper->nod_arg[0];
			if (lower->ref_value)
				fprintf(gpreGlob.out_file, " * ( %s - %s + 1)", upper->ref_value, lower->ref_value);
			else
				fprintf(gpreGlob.out_file, " * ( %s + 1)", upper->ref_value);
		}
	}

	fprintf(gpreGlob.out_file, ";");

	// Make assignments to variable vector
	const ref* reference;
	for (reference = request->req_values; reference; reference = reference->ref_next)
	{
		printa(column, "gds__%dv [%d] := %s;",
				request->req_ident, reference->ref_id, reference->ref_value);
	}

	PAT args;
	args.pat_reference = slice->slc_field_ref;
	args.pat_request = parent_request;	// blob id request
	args.pat_vector1 = status_vector(action);	// status vector
	args.pat_database = parent_request->req_database;	// database handle
	args.pat_string1 = action->act_request->req_trans;	// transaction handle
	args.pat_value1 = request->req_length;	// slice descr. length
	args.pat_ident1 = request->req_ident;	// request name
	args.pat_value2 = slice->slc_parameters * sizeof(SLONG);	// parameter length

	reference = (const ref*) slice->slc_array->nod_arg[0];
	args.pat_string5 = reference->ref_value;	// array name
	args.pat_string6 = "gds__array_length";

	PATTERN_expand(column, (action->act_type == ACT_get_slice) ? pattern1 : pattern2, &args);
}


//____________________________________________________________
//
//		Generate either a START or START_AND_SEND depending
//		on whether or a not a port is present.
//

static void gen_start( const act* action, const gpre_port* port, int column)
{
	const gpre_req* request = action->act_request;
	const TEXT* vector = status_vector(action);

	align(column);

	if (port)
	{
		for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
		{
			if (reference->ref_field->fld_array_info)
				gen_get_or_put_slice(action, reference, false, column);
		}

		fprintf(gpreGlob.out_file, "GDS__START_AND_SEND (%s, %s, %s, %d, %d, gds__%d, %s);",
				   vector, request->req_handle, request_trans(action, request),
				   port->por_msg_number, port->por_length, port->por_ident,
				   request->req_request_level);
	}
	else
		fprintf(gpreGlob.out_file, "GDS__START_REQUEST (%s, %s, %s, %s);",
				   vector, request->req_handle, request_trans(action, request),
				   request->req_request_level);
}


//____________________________________________________________
//
//		Generate text for STORE statement.  This includes the compile
//		call and any variable initialization required.
//

static void gen_store( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	gen_compile(action, column);
	if (action->act_error || (action->act_flags & ACT_sql))
	{
		make_ok_test(action, request, column);
		column += INDENT;
		if (action->act_error)
			begin(column);
	}

	// Initialize any blob fields

	TEXT name[MAX_REF_SIZE];
	const gpre_port* port = request->req_primary;
	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		if (field->fld_flags & FLD_blob)
			printa(column, "%s := gds__blob_null;\n", gen_name(name, reference, true));
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

	gpre_tra* trans;
	if (!action || !(trans = (gpre_tra*) action->act_object))
	{
		t_start_auto(action, 0, status_vector(action), column);
		return;
	}

	// build a complete statement, including tpb's.
	// first generate any appropriate ready statements,
	// On non-VMS machines, fill in the tpb vector (aka TEB).

	int count = 0;
	for (const tpb* tpb_val = trans->tra_tpb; tpb_val; tpb_val = tpb_val->tpb_tra_next)
	{
		count++;
		const gpre_dbb* db = tpb_val->tpb_database;
		if (gpreGlob.sw_auto)
		{
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				printa(column, "if (%s = nil) then", db->dbb_name->sym_string);
				make_ready(db, filename, status_vector(action), column + INDENT, 0);
			}
		}

		printa(column, "gds__teb[%d].tpb_len := %d;", count, tpb_val->tpb_length);
		printa(column, "gds__teb[%d].tpb_ptr := ADDR(gds__tpb_%d);", count, tpb_val->tpb_ident);
		printa(column, "gds__teb[%d].dbb_ptr := ADDR(%s);", count, db->dbb_name->sym_string);
	}

	printa(column, "GDS__START_MULTIPLE (%s, %s, %d, gds__teb);",
		   status_vector(action), trans->tra_handle ? trans->tra_handle : "gds__trans",
		   trans->tra_db_count);

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate a TPB in the output file
//

static void gen_tpb( const tpb* tpb_val, int column)
{
	TEXT buffer[80];

	printa(column, "gds__tpb_%d\t: %s [1..%d] of char := %s",
		   tpb_val->tpb_ident, PACKED_ARRAY, tpb_val->tpb_length, OPEN_BRACKET);

	int length = tpb_val->tpb_length;
	const TEXT* text = (TEXT *) tpb_val->tpb_string;
	TEXT* p = buffer;

	while (--length)
	{
		const TEXT c = *text++;
		if ((c >= 'A' && c <= 'Z') || c == '$' || c == '_')
			sprintf(p, "'%c', ", c);
		else
			sprintf(p, "chr(%d), ", c);
		while (*p)
			p++;
		if (p - buffer > 60)
		{
			align(column + INDENT);
			fprintf(gpreGlob.out_file, " %s", buffer);
			p = buffer;
			*p = 0;
		}
	}

	// handle the last character
	TEXT c = *text++;

	if ((c >= 'A' && c <= 'Z') || c == '$' || c == '_')
		sprintf(p, "'%c',", c);
	else
		sprintf(p, "chr(%d)", c);

	align(column + INDENT);
	fprintf(gpreGlob.out_file, "%s", buffer);

	printa(column, "%s;\n", CLOSE_BRACKET);
}


//____________________________________________________________
//
//		Generate substitution text for COMMIT, ROLLBACK, PREPARE, and SAVE
//

static void gen_trans( const act* action, int column)
{
	align(column);

	const char* tranText = action->act_object ? (const TEXT*) action->act_object : "gds__trans";

	switch (action->act_type)
	{
	case ACT_commit_retain_context:
		fprintf(gpreGlob.out_file, "GDS__COMMIT_RETAINING (%s, %s);", status_vector(action), tranText);
		break;
	case ACT_rollback_retain_context:
		fprintf(gpreGlob.out_file, "GDS__ROLLBACK_RETAINING (%s, %s);", status_vector(action), tranText);
		break;
	default:
		fprintf(gpreGlob.out_file, "GDS__%s_TRANSACTION (%s, %s);",
				(action->act_type == ACT_commit) ?
					"COMMIT" : (action->act_type == ACT_rollback) ? "ROLLBACK" : "PREPARE",
				status_vector(action), tranText);
	}

	set_sqlcode(action, column);
}


//____________________________________________________________
//
//		Generate substitution text for UPDATE ... WHERE CURRENT OF ...
//

static void gen_update( const act* action, int column)
{
	const upd* modify = (upd*) action->act_object;
	const gpre_port* port = modify->upd_port;
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

	printa(column, gen_name(s, action->act_object, false));
}


//____________________________________________________________
//
//		Generate tests for any WHENEVER clauses that may have been declared.
//

static void gen_whenever( const swe* label, int column)
{
	const TEXT* condition = NULL;

	if (label)
		fprintf(gpreGlob.out_file, ";");

	while (label)
	{
		switch (label->swe_condition)
		{
		case SWE_error:
			condition = "SQLCODE < 0";
			break;

		case SWE_warning:
			condition = "(SQLCODE > 0) AND (SQLCODE <> 100)";
			break;

		case SWE_not_found:
			condition = "SQLCODE = 100";
			break;

		default:
			// condition undefined
			fb_assert(false);
			return;
		}
		align(column);
		fprintf(gpreGlob.out_file, "if %s then goto %s;", condition, label->swe_label);
		label = label->swe_next;
	}
}

//____________________________________________________________
//
//		Generate a declaration of an array in the
//		output file.
//

static void make_array_declaration( const ref* reference)
{
	const gpre_fld* field = reference->ref_field;
	const TEXT* name = field->fld_symbol->sym_string;

	// Don't generate multiple declarations for the array.  V3 Bug 569.

	if (field->fld_array_info->ary_declared)
		return;

	field->fld_array_info->ary_declared = true;

	if (field->fld_array_info->ary_dtype <= dtype_varying)
		fprintf(gpreGlob.out_file, "gds__%d : %s [", field->fld_array_info->ary_ident, PACKED_ARRAY);
	else
		fprintf(gpreGlob.out_file, "gds__%d : array [", field->fld_array_info->ary_ident);

	// Print out the dimension part of the declaration
	for (const dim* dimension = field->fld_array_info->ary_dimension; dimension;
		dimension = dimension->dim_next)
	{
		fprintf(gpreGlob.out_file, "%"SLONGFORMAT"..%"SLONGFORMAT, dimension->dim_lower,
				   dimension->dim_upper);
		if (dimension->dim_next)
			fprintf(gpreGlob.out_file, ", ");
	}

	if (field->fld_array_info->ary_dtype <= dtype_varying)
		fprintf(gpreGlob.out_file, ", 1..%d", field->fld_array->fld_length);

	fprintf(gpreGlob.out_file, "] of ");

	switch (field->fld_array_info->ary_dtype)
	{
	case dtype_short:
		fprintf(gpreGlob.out_file, SHORT_DCL);
		break;

	case dtype_long:
		fprintf(gpreGlob.out_file, LONG_DCL);
		break;

	case dtype_cstring:
	case dtype_text:
	case dtype_varying:
		fprintf(gpreGlob.out_file, "char");
		break;

	case dtype_date:
	case dtype_quad:
		fprintf(gpreGlob.out_file, "ISC_QUAD");
		break;

	case dtype_real:
		fprintf(gpreGlob.out_file, "real");
		break;

	case dtype_double:
		fprintf(gpreGlob.out_file, "double");
		break;

	default:
		{
			TEXT s[ERROR_LENGTH];
			fb_utils::snprintf(s, sizeof(s), "datatype %d unknown for field %s",
					field->fld_array_info->ary_dtype, name);
			CPR_error(s);
			return;
		}
	}

	// Print out the database field

	fprintf(gpreGlob.out_file, ";\t(* %s *)\n", name);
}


//____________________________________________________________
//
//		Turn a symbol into a varying string.
//

static TEXT* make_name( TEXT* const string, const gpre_sym* symbol)
{
	fb_utils::snprintf(string, MAX_CURSOR_SIZE, "'%s '", symbol->sym_string);

	return string;
}


//____________________________________________________________
//
//		Generate code to test existence of compiled request with
//		active transaction
//

static void make_ok_test( const act* action, const gpre_req* request, int column)
{

	if (gpreGlob.sw_auto)
		printa(column, "if (%s <> nil) and (%s <> nil) then",
			   request_trans(action, request), request->req_handle);
	else
		printa(column, "if (%s <> nil) then", request->req_handle);
}


//____________________________________________________________
//
//		Insert a port record description in output.
//

static void make_port( const gpre_port* port, int column)
{
	bool flag = false;

	printa(column, "gds__%dt = record", port->por_ident);

	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		if (flag)
			fprintf(gpreGlob.out_file, ";");
		flag = true;
		align(column + INDENT);
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
			fprintf(gpreGlob.out_file, "gds__%d\t: real\t(* %s *)", reference->ref_ident, name);
			break;

		case dtype_double:
			fprintf(gpreGlob.out_file, "gds__%d\t: double\t(* %s *)", reference->ref_ident, name);
			break;

		case dtype_short:
			fprintf(gpreGlob.out_file, "gds__%d\t: %s\t(* %s *)", reference->ref_ident, SHORT_DCL, name);
			break;

		case dtype_long:
			fprintf(gpreGlob.out_file, "gds__%d\t: %s\t(* %s *)", reference->ref_ident, LONG_DCL, name);
			break;

		case dtype_date:
		case dtype_quad:
		case dtype_blob:
			fprintf(gpreGlob.out_file, "gds__%d\t: gds__quad\t(* %s *)", reference->ref_ident, name);
			break;

		case dtype_text:
			fprintf(gpreGlob.out_file, "gds__%d\t: %s [1..%d] of char\t(* %s *)",
					   reference->ref_ident, PACKED_ARRAY, field->fld_length, name);
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

	printa(column, "end;\n");
}


//____________________________________________________________
//
//		Generate the actual insertion text for a
//		ready;
//

static void make_ready(const gpre_dbb* db,
				  	   const TEXT* filename, const TEXT* vector, USHORT column,
				  	   const gpre_req* request)
{
	TEXT s1[32], s2[32];

	if (request)
	{
		sprintf(s1, "gds__%dL", request->req_ident);
		sprintf(s2, "gds__%d", request->req_ident);
	}

	align(column);
	if (filename)
	{
		fprintf(gpreGlob.out_file, "GDS__ATTACH_DATABASE (%s, %s (%s), %s, %s, %s, %s);",
				   vector, SIZEOF, filename, filename,
				   db->dbb_name->sym_string, (request ? s1 : "0"),
				   (request ? s2 : "0"));
	}
	else
	{
		fprintf(gpreGlob.out_file, "GDS__ATTACH_DATABASE (%s, %" SIZEFORMAT ", '%s', %s, %s, %s);",
				   vector, strlen(db->dbb_filename), db->dbb_filename,
				   db->dbb_name->sym_string, (request ? s1 : "0"),
				   (request ? s2 : "0"));
	}
}


//____________________________________________________________
//
//		Print a fixed string at a particular column.
//

static void printa( int column, const char* string, ...)
{
	va_list ptr;

	va_start(ptr, string);
	align(column);
	vfprintf(gpreGlob.out_file, string, ptr);
	va_end(ptr);
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
			trname = "gds__trans";
		return trname;
	}

	return request ? request->req_trans : "gds__trans";
}


//____________________________________________________________
//
//		Generate the appropriate status vector parameter for a gds
//		call depending on where or not the action has an error clause.
//

static const TEXT* status_vector( const act* action)
{
	if (action && (action->act_error || (action->act_flags & ACT_sql)))
		return "gds__status";

	return "gds__null^";
}


//____________________________________________________________
//
//		Generate substitution text for START_TRANSACTION.
//		The complications include the fact that all databases
//		must be readied, and that everything should stop if
//		any thing fails so we don't trash the status vector.
//

static void t_start_auto( const act* action, const gpre_req* request,
						 const TEXT* vector, int column)
{
	TEXT buffer[256];

	buffer[0] = 0;

	// find out whether we're using a status vector or not

	const bool stat = !strcmp(vector, "gds__status");

	// this is a default transaction, make sure all databases are ready

	begin(column);

	int count, and_count;
	const gpre_dbb* db;
	for (db = gpreGlob.isc_databases, count = and_count = 0; db; db = db->dbb_next)
	{
		if (gpreGlob.sw_auto)
		{
			const TEXT* filename = db->dbb_runtime;
			if (filename || !(db->dbb_flags & DBB_sqlca))
			{
				align(column);
				fprintf(gpreGlob.out_file, "if (%s = nil", db->dbb_name->sym_string);
				if (stat && buffer[0])
					fprintf(gpreGlob.out_file, ") and (%s[2] = 0", vector);
				fprintf(gpreGlob.out_file, ") then");
				make_ready(db, filename, vector, column + INDENT, 0);
				if (buffer[0])
					if (and_count % 4)
						strcat(buffer, ") and (");
					else
						strcat(buffer, ") and\n\t(");
				and_count++;
				TEXT temp[40];
				sprintf(temp, "%s <> nil", db->dbb_name->sym_string);
				strcat(buffer, temp);
				printa(column, "if (%s) then", buffer);
				align(column + INDENT);
			}
		}

		count++;
		printa(column, "gds__teb[%d].tpb_len:= 0;", count);
		printa(column, "gds__teb[%d].tpb_ptr := ADDR(gds__null);", count);
		printa(column, "gds__teb[%d].dbb_ptr := ADDR(%s);", count, db->dbb_name->sym_string);
	}

	printa(column, "GDS__START_MULTIPLE (%s, %s, %d, gds__teb);",
		   vector, request_trans(action, request), count);

	if (gpreGlob.sw_auto && request)
		column -= INDENT;

	set_sqlcode(action, column);
	ends(column);
}
