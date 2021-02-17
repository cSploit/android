//____________________________________________________________
//
//		PROGRAM:	C preprocess
//		MODULE:		int_cxx.cpp
//		DESCRIPTION:	Code generate for internal JRD modules
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
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings in generated code
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
#include "../gpre/gpre_proto.h"
#include "../gpre/lang_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h"

static void align(const int);
static void asgn_from(ref*, int);
#ifdef NOT_USED_OR_REPLACED
static void asgn_to(ref*);
#endif
static void gen_at_end(const act*, int);
static void gen_blr(void*, SSHORT, const char*);
static void gen_compile(const gpre_req*, int);
static void gen_database();
static void gen_emodify(const act*, int);
static void gen_estore(const act*, int, bool);
static void gen_endfor(const act*, int);
static void gen_erase(const act*, int);
static void gen_for(const act*, int);
static char* gen_name(char* const, const ref*);
static void gen_raw(const gpre_req*);
static void gen_receive(const gpre_req*, const gpre_port*);
static void gen_request(const gpre_req*);
static void gen_routine(const act*, int);
static void gen_s_end(const act*, int);
static void gen_s_fetch(const act*, int);
static void gen_s_start(const act*, int);
static void gen_send(const gpre_req*, const gpre_port*, int, bool);
static void gen_start(const gpre_req*, const gpre_port*, int, bool);
static void gen_type(const act*, int);
static void gen_variable(const act*, int);
static void make_port(const gpre_port*, int);
static void printa(const int, const TEXT*, ...);

static bool global_first_flag = false;

const int INDENT = 3;

static const char* const GDS_VTOV = "gds__vtov";
static const char* const JRD_VTOF = "jrd_vtof";
static const char* const VTO_CALL = "%s ((const char*) %s, (char*) %s, %d);";

static inline void begin(const int column)
{
	printa(column, "{");
}

static inline void endp(const int column)
{
	printa(column, "}");
}

//____________________________________________________________
//
//

void INT_CXX_action( const act* action, int column)
{

	// Put leading braces where required

	switch (action->act_type)
	{
	case ACT_for:
	case ACT_insert:
	case ACT_modify:
	case ACT_store:
	case ACT_s_fetch:
	case ACT_s_start:
		begin(column);
		align(column);
	}

	switch (action->act_type)
	{
	case ACT_at_end:
		gen_at_end(action, column);
		return;
	case ACT_b_declare:
	case ACT_database:
		gen_database();
		return;
	case ACT_endfor:
		gen_endfor(action, column);
		break;
	case ACT_endmodify:
		gen_emodify(action, column);
		break;
	case ACT_endstore:
		gen_estore(action, column, false);
		break;
	case ACT_endstore_special:
		gen_estore(action, column, true);
		break;
	case ACT_erase:
		gen_erase(action, column);
		return;
	case ACT_for:
		gen_for(action, column);
		return;
	case ACT_hctef:
		break;
	case ACT_insert:
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
	case ACT_type_number:
		gen_type(action, column);
		return;
	case ACT_variable:
		gen_variable(action, column);
		return;
	default:
		return;
	}

	// Put in a trailing brace for those actions still with us

	endp(column);
}


//____________________________________________________________
//
//		Align output to a specific column for output.
//

static void align(const int column)
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

static void asgn_from( ref* reference, int column)
{
	TEXT variable[MAX_REF_SIZE];
	TEXT temp[MAX_REF_SIZE];

	for (; reference; reference = reference->ref_next)
	{
		const gpre_fld* field = reference->ref_field;
		align(column);
		gen_name(variable, reference);
		const TEXT* value;
		if (reference->ref_source) {
			value = gen_name(temp, reference->ref_source);
		}
		else {
			value = reference->ref_value;
		}

		// To avoid chopping off a double byte kanji character in between
		// the two bytes, generate calls to gds__ftof2 gds$_vtof2,
		// gds$_vtov2 and jrd_vtof2 wherever necessary

		if (!field || field->fld_dtype == dtype_text)
			fprintf(gpreGlob.out_file, VTO_CALL, JRD_VTOF, value, variable,
					   field ? field->fld_length : 0);
		else if (!field || field->fld_dtype == dtype_cstring)
			fprintf(gpreGlob.out_file, VTO_CALL, GDS_VTOV, value, variable,
					   field ? field->fld_length : 0);
		else
			fprintf(gpreGlob.out_file, "%s = %s;", variable, value);
	}
}


//____________________________________________________________
//
//		Build an assignment to a host language variable from
//		a port variable.
//
#ifdef NOT_USED_OR_REPLACED
static void asgn_to( ref* reference)
{
	TEXT s[MAX_REF_SIZE];

	ref* source = reference->ref_friend;
	gpre_fld* field = source->ref_field;
	gen_name(s, source);

	// Repeated later down in function gen_emodify, but then
	// emitting jrd_ftof call.

	if (!field || field->fld_dtype == dtype_text)
		fprintf(gpreGlob.out_file, "gds__ftov (%s, %d, %s, sizeof(%s));",
				   s, field ? field->fld_length : 0, reference->ref_value, reference->ref_value);
	else if (!field || field->fld_dtype == dtype_cstring)
		fprintf(gpreGlob.out_file, "gds__vtov((const char*) %s, (char*) %s, sizeof(%s));",
				   s, reference->ref_value, reference->ref_value);
	else
		fprintf(gpreGlob.out_file, "%s = %s;", reference->ref_value, s);
}
#endif

//____________________________________________________________
//
//		Generate code for AT END clause of FETCH.
//

static void gen_at_end( const act* action, int column)
{
	TEXT s[MAX_REF_SIZE];

	const gpre_req* request = action->act_request;
	printa(column, "if (!%s) ", gen_name(s, request->req_eof));
}


//____________________________________________________________
//
//		Callback routine for BLR pretty printer.
//

static void gen_blr(void* /*user_arg*/, SSHORT /*offset*/, const char* string)
{
	fprintf(gpreGlob.out_file, "%s\n", string);
}


//____________________________________________________________
//
//		Generate text to compile a request.
//

static void gen_compile( const gpre_req* request, int column)
{
	column += INDENT;
	//const gpre_dbb* db = request->req_database;
	//const gpre_sym* symbol = db->dbb_name;
	//align(column);
	fprintf(gpreGlob.out_file,
		"%s.compile(tdbb, (UCHAR*) jrd_%"ULONGFORMAT", sizeof(jrd_%"ULONGFORMAT"));",
			   request->req_handle, request->req_ident, request->req_ident);
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

	align(0);
	for (const gpre_req* request = gpreGlob.requests; request; request = request->req_next)
		gen_request(request);
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
		if (!source) {
			continue;
		}
		const gpre_fld* field = reference->ref_field;
		align(column);

		switch (field->fld_dtype)
		{
		case dtype_text:
			fprintf(gpreGlob.out_file, "jrd_ftof (%s, %d, %s, %d);",
					gen_name(s1, source), field->fld_length, gen_name(s2, reference), field->fld_length);
			break;
		case dtype_cstring:
			fprintf(gpreGlob.out_file, "gds__vtov((const char*) %s, (char*) %s, %d);",
					gen_name(s1, source), gen_name(s2, reference), field->fld_length);
			break;
		default:
			fprintf(gpreGlob.out_file, "%s = %s;", gen_name(s1, reference), gen_name(s2, source));
		}
	}

	gen_send(action->act_request, modify->upd_port, column, false);
}


//____________________________________________________________
//
//		Generate substitution text for END_STORE.
//

static void gen_estore( const act* action, int column, bool special)
{
	const gpre_req* request = action->act_request;
	align(column);
	gen_compile(request, column);
	gen_start(request, request->req_primary, column, special);
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
		gen_send(request, request->req_sync, column, false);

	endp(column);
}


//____________________________________________________________
//
//		Generate substitution text for ERASE.
//

static void gen_erase( const act* action, int column)
{
	upd* erase = (upd*) action->act_object;
	gen_send(erase->upd_request, erase->upd_port, column, false);
}


//____________________________________________________________
//
//		Generate substitution text for FOR statement.
//

static void gen_for( const act* action, int column)
{
	TEXT s[MAX_REF_SIZE];

	gen_s_start(action, column);

	align(column);
	const gpre_req* request = action->act_request;
	fprintf(gpreGlob.out_file, "while (1)");
	column += INDENT;
	begin(column);
	align(column);
	gen_receive(action->act_request, request->req_primary);
	align(column);
	fprintf(gpreGlob.out_file, "if (!%s) break;", gen_name(s, request->req_eof));
}


//____________________________________________________________
//
//		Generate a name for a reference.  Name is constructed from
//		port and parameter idents.
//

static char* gen_name(char* const string, const ref* reference)
{
	fb_utils::snprintf(string, MAX_REF_SIZE, "jrd_%d.jrd_%d",
					   reference->ref_port->por_ident, reference->ref_ident);

	return string;
}


//____________________________________________________________
//
//		Generate BLR in raw, numeric form.  Ugly but dense.
//

static void gen_raw( const gpre_req* request)
{
	TEXT buffer[80];

	const UCHAR* blr = request->req_blr;
	int blr_length = request->req_length;
	TEXT* p = buffer;
	align(0);

	while (--blr_length)
	{
		const UCHAR c = *blr++;
		if ((c >= 'A' && c <= 'Z') || c == '$' || c == '_')
			sprintf(p, "'%c',", c);
		else
			sprintf(p, "%d,", c);
		while (*p)
			p++;
		if (p - buffer > 60)
		{
			fprintf(gpreGlob.out_file, "%s\n", buffer);
			p = buffer;
			*p = 0;
		}
	}

	fprintf(gpreGlob.out_file, "%s%d", buffer, blr_eoc);
}


//____________________________________________________________
//
//		Generate a send or receive call for a port.
//

static void gen_receive( const gpre_req* request, const gpre_port* port)
{

	fprintf(gpreGlob.out_file,
			   "EXE_receive (tdbb, %s, %d, %d, (UCHAR*) &jrd_%"ULONGFORMAT");",
			   request->req_handle, port->por_msg_number, port->por_length,
			   port->por_ident);
}


//____________________________________________________________
//
//		Generate definitions associated with a single request.
//

static void gen_request( const gpre_req* request)
{

	if (!(request->req_flags & REQ_exp_hand))
		fprintf(gpreGlob.out_file, "static void\t*%s;\t// request handle \n", request->req_handle);

	fprintf(gpreGlob.out_file, "static const UCHAR\tjrd_%"ULONGFORMAT" [%d] =",
			   request->req_ident, request->req_length);
	align(INDENT);
	fprintf(gpreGlob.out_file, "{\t// blr string \n");

	if (gpreGlob.sw_raw)
		gen_raw(request);
	else
		fb_print_blr(request->req_blr, request->req_length, gen_blr, 0, 0);

	printa(INDENT, "};\t// end of blr string \n");
}


//____________________________________________________________
//
//		Process routine head.  If there are gpreGlob.requests in the
//		routine, insert local definitions.
//

static void gen_routine( const act* action, int column)
{
	for (const gpre_req* request = (gpre_req*) action->act_object; request;
		request = request->req_routine)
	{
		for (const gpre_port* port = request->req_ports; port; port = port->por_next)
			make_port(port, column + INDENT);
	}
}


//____________________________________________________________
//
//		Generate substitution text for END_STREAM.
//

static void gen_s_end( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	printa(column, "EXE_unwind (tdbb, %s);", request->req_handle);
}


//____________________________________________________________
//
//		Generate substitution text for FETCH.
//

static void gen_s_fetch( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	if (request->req_sync)
		gen_send(request, request->req_sync, column, false);

	gen_receive(action->act_request, request->req_primary);
}


//____________________________________________________________
//
//		Generate text to compile and start a stream.  This is
//		used both by START_STREAM and FOR
//

static void gen_s_start( const act* action, int column)
{
	const gpre_req* request = action->act_request;
	gen_compile(request, column);

    const gpre_port* port = request->req_vport;
	if (port)
		asgn_from(port->por_references, column);

	gen_start(request, port, column, false);
}


//____________________________________________________________
//
//		Generate a send or receive call for a port.
//

static void gen_send( const gpre_req* request, const gpre_port* port, int column, bool special)
{
	if (special)
	{
		align(column);
		fprintf(gpreGlob.out_file, "if (ignore_perm)");
		align(column);
		fprintf(gpreGlob.out_file, "\trequest->getStatement()->flags |= JrdStatement::FLAG_IGNORE_PERM;");
	}
	align(column);

	fprintf(gpreGlob.out_file, "EXE_send (tdbb, %s, %d, %d, (UCHAR*) &jrd_%"ULONGFORMAT");",
			   request->req_handle, port->por_msg_number, port->por_length, port->por_ident);
}


//____________________________________________________________
//
//		Generate a START.
//

static void gen_start( const gpre_req* request, const gpre_port* port, int column, bool special)
{
	align(column);

	fprintf(gpreGlob.out_file, "EXE_start (tdbb, %s, %s);", request->req_handle, request->req_trans);

	if (port)
		gen_send(request, port, column, special);
}


//____________________________________________________________
//
//		Substitute for a variable reference.
//

static void gen_type( const act* action, int column)
{

	printa(column, "%ld", action->act_object);
}


//____________________________________________________________
//
//		Substitute for a variable reference.
//

static void gen_variable( const act* action, int column)
{
	char s[MAX_REF_SIZE];

	align(column);
	fprintf(gpreGlob.out_file, "%s", gen_name(s, action->act_object));

}


//____________________________________________________________
//
//		Insert a port record description in output.
//

static void make_port( const gpre_port* port, int column)
{
	printa(column, "struct {");

	for (const ref* reference = port->por_references; reference; reference = reference->ref_next)
	{
		align(column + INDENT);
		const gpre_fld* field = reference->ref_field;
		const gpre_sym* symbol = field->fld_symbol;
		const TEXT* name = symbol->sym_string;
		const char* fmtstr = NULL;

		switch (field->fld_dtype)
		{
		case dtype_short:
			fmtstr = "    SSHORT jrd_%d;\t// %s ";
			break;

		case dtype_long:
			fmtstr = "    SLONG  jrd_%d;\t// %s ";
			break;

		// Begin sql date/time/timestamp
		case dtype_sql_date:
			fmtstr = "    ISC_DATE  jrd_%d;\t// %s ";
			break;

		case dtype_sql_time:
			fmtstr = "    ISC_TIME  jrd_%d;\t// %s ";
			break;

		case dtype_timestamp:
			fmtstr = "    ISC_TIMESTAMP  jrd_%d;\t// %s ";
			break;
		// End sql date/time/timestamp

		case dtype_int64:
			fmtstr = "    ISC_INT64  jrd_%d;\t// %s ";
			break;

		case dtype_quad:
			fmtstr = "    ISC_QUAD  jrd_%d;\t// %s ";
			break;

		case dtype_blob:
			fmtstr = "    bid  jrd_%d;\t// %s ";
			break;

		case dtype_cstring:
		case dtype_text:
			fprintf(gpreGlob.out_file, "    TEXT  jrd_%d [%d];\t// %s ",
					reference->ref_ident, field->fld_length, name);
			break;

		case dtype_real:
			fmtstr = "    float  jrd_%d;\t// %s ";
			break;

		case dtype_double:
			fmtstr = "    double  jrd_%d;\t// %s ";
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
		if (fmtstr)
			fprintf(gpreGlob.out_file, fmtstr, reference->ref_ident, name);
	}
	align(column);
	fprintf(gpreGlob.out_file, "} jrd_%"ULONGFORMAT";", port->por_ident);
}


//____________________________________________________________
//
//		Print a fixed string at a particular column.
//

static void printa(const int column, const TEXT* string, ...)
{
	va_list ptr;

	va_start(ptr, string);
	align(column);
	vfprintf(gpreGlob.out_file, string, ptr);
	va_end(ptr);
}

