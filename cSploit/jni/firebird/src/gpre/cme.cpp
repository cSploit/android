//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		cme.cpp
//		DESCRIPTION:	Request expression compiler
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
//  Mike Nordell		- Reduce compiler warnings, buffer ptr bug
//  Stephen W. Boyd		- Add support for new features
//____________________________________________________________
//

#include "firebird.h"
#include <stdlib.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
#include "../jrd/intl.h"
#include "../intl/charsets.h"
#include "../gpre/cme_proto.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/movg_proto.h"
#include "../gpre/par_proto.h"
#include "../common/prett_proto.h"
#include "../common/dsc_proto.h"
#include "../gpre/msc_proto.h"
#include "../jrd/align.h"

static void cmp_array(gpre_nod*, gpre_req*);
static void cmp_array_element(gpre_nod*, gpre_req*);
static void cmp_cast(gpre_nod*, gpre_req*);
static void cmp_field(const gpre_nod*, gpre_req*);
static void cmp_literal(const gpre_nod*, gpre_req*);
static void cmp_map(map*, gpre_req*);
static void cmp_plan(const gpre_nod*, gpre_req*);
static void cmp_sdl_dtype(const gpre_fld*, ref*);
static void cmp_udf(gpre_nod*, gpre_req*);
static void cmp_value(const gpre_nod*, gpre_req*);
static void get_dtype_of_case(const gpre_nod*, gpre_fld*);
static void get_dtype_of_list(const gpre_nod*, gpre_fld*);
static USHORT get_string_len(const gpre_fld*);
static void stuff_sdl_dimension(const dim*, ref*, SSHORT);
static void stuff_sdl_element(ref*, const gpre_fld*);
static void stuff_sdl_loops(ref*, const gpre_fld*);
static void stuff_sdl_number(const SLONG, ref*);

//#define STUFF(blr)		*request->req_blr++ = (UCHAR) (blr)
//#define STUFF_WORD(blr)		STUFF (blr); STUFF (blr >> 8)
//#define STUFF_CSTRING(blr)	stuff_cstring (request, blr)

//#define STUFF_SDL(sdl)		*reference->ref_sdl++ = (UCHAR) (sdl)
//#define STUFF_SDL_WORD(sdl)	STUFF_SDL (sdl); STUFF_SDL (sdl >> 8);
//#define STUFF_SDL_LONG(sdl)	STUFF_SDL (sdl); STUFF_SDL (sdl >> 8); STUFF_SDL (sdl >>16); STUFF_SDL (sdl >> 24);

static bool debug_on;

struct op_table
{
	nod_t op_type;
	UCHAR op_blr;
};

const op_table operators[] =
{
	{ nod_eq			, blr_eql },
	{ nod_equiv			, blr_equiv },
	{ nod_ge			, blr_geq },
	{ nod_gt			, blr_gtr },
	{ nod_le			, blr_leq },
	{ nod_lt			, blr_lss },
	{ nod_ne			, blr_neq },
	{ nod_missing		, blr_missing },
	{ nod_between		, blr_between },
	{ nod_and			, blr_and },
	{ nod_or			, blr_or },
	{ nod_not			, blr_not },
	{ nod_matches		, blr_matching },
	{ nod_starting	, blr_starting },
	{ nod_containing	, blr_containing },
	{ nod_plus		, blr_add },
	{ nod_from		, blr_from },
	{ nod_via			, blr_via },
	{ nod_minus		, blr_subtract },
	{ nod_times		, blr_multiply },
	{ nod_divide		, blr_divide },
	{ nod_negate		, blr_negate },
	{ nod_null		, blr_null },
	{ nod_user_name	, blr_user_name },
	//{ count2 }
	//{ nod_count, blr_count2 },
	{ nod_count		, blr_count },
	{ nod_max			, blr_maximum },
	{ nod_min			, blr_minimum },
	{ nod_average		, blr_average },
	{ nod_total		, blr_total },
	{ nod_any			, blr_any },
	{ nod_unique		, blr_unique },
	{ nod_agg_count	, blr_agg_count2 },
	{ nod_agg_count	, blr_agg_count },
	{ nod_agg_max		, blr_agg_max },
	{ nod_agg_min		, blr_agg_min },
	{ nod_agg_total	, blr_agg_total },
	{ nod_agg_average	, blr_agg_average },
	{ nod_upcase		, blr_upcase },
	{ nod_lowcase		, blr_lowcase },
	{ nod_sleuth		, blr_matching2 },
	{ nod_concatenate	, blr_concatenate },
	{ nod_cast		, blr_cast },
	{ nod_ansi_any	, blr_ansi_any },
	{ nod_gen_id		, blr_gen_id },
	{ nod_ansi_all	, blr_ansi_all },
	{ nod_current_date, blr_current_date },
	{ nod_current_time, blr_current_time },
	{ nod_current_timestamp, blr_current_timestamp },
	{ nod_current_role, blr_current_role },
	{ nod_any, 0 }
};


static inline void assign_dtype(gpre_fld* f, const gpre_fld* field)
{
	f->fld_dtype = field->fld_dtype;
	f->fld_length = field->fld_length;
	f->fld_scale = field->fld_scale;
	f->fld_precision = field->fld_precision;
	f->fld_sub_type = field->fld_sub_type;
	f->fld_charset_id = field->fld_charset_id;
	f->fld_collate_id = field->fld_collate_id;
	f->fld_ttype = field->fld_ttype;
}

// One of d1, d2 is time, the other is date
static inline bool is_date_and_time(const USHORT d1, const USHORT d2)
{
	return (d1 == dtype_sql_time && d2 == dtype_sql_date) ||
		   (d2 == dtype_sql_time && d1 == dtype_sql_date);
}

//____________________________________________________________
//
//		Compile a random expression.
//

void CME_expr(gpre_nod* node, gpre_req* request)
{
	gpre_ctx* context;
	gpre_fld field;
	const ref* reference;
	TEXT s[128];

	switch (node->nod_type)
	{
	case nod_field:
		if (!(reference = (ref*) node->nod_arg[0]))
		{
			CPR_error("CME_expr: reference missing");
			return;
		}
		cmp_field(node, request);
		if (reference->ref_flags & REF_fetch_array)
			cmp_array(node, request);
		return;

	case nod_array:
		cmp_array_element(node, request);
		return;

	case nod_index:
		CME_expr(node->nod_arg[0], request);
		return;

	case nod_value:
		cmp_value(node, request);
		if ((reference = (ref*) node->nod_arg[0]) && (reference->ref_flags & REF_fetch_array))
		{
			cmp_array(node, request);
		}
		return;

	case nod_negate:
		if (node->nod_arg[0]->nod_type != nod_literal)
			break;
	case nod_literal:
		cmp_literal(node, request);
		return;

	case nod_like:
		{
			request->add_byte((node->nod_count == 2) ? blr_like : blr_ansi_like);
			gpre_nod** ptr = node->nod_arg;
			for (const gpre_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
			{
				CME_expr(*ptr, request);
			}
			return;
		}

	case nod_udf:
		cmp_udf(node, request);
		return;

	case nod_gen_id:
		{
			request->add_byte(blr_gen_id);
			const TEXT* p = (TEXT *) (node->nod_arg[1]);

			// check if this generator really exists

			if (!MET_generator(p, request->req_database))
			{
				sprintf(s, "generator %s not found", p);
				CPR_error(s);
			}
			request->add_byte(strlen(p));
			while (*p)
				request->add_byte(*p++);
			CME_expr(node->nod_arg[0], request);
			return;
		}

	case nod_cast:
		cmp_cast(node, request);
		return;

	case nod_nullif:
		request->add_byte(blr_value_if);
		request->add_byte(blr_eql);
		CME_expr(node->nod_arg[0], request);
		CME_expr(node->nod_arg[1], request);
		request->add_byte(blr_null);
		CME_expr(node->nod_arg[0], request);
		return;

	case nod_agg_count:
		if (node->nod_arg[0])
		{
			if (node->nod_arg[1])
				request->add_byte(blr_agg_count_distinct);
			else
				request->add_byte(blr_agg_count2);
			CME_expr(node->nod_arg[0], request);
		}
		else
			request->add_byte(blr_agg_count);
		return;

	// Begin date/time/timestamp support
	case nod_extract:
		request->add_byte(blr_extract);
		switch ((kwwords_t) (IPTR) node->nod_arg[0])
		{
		case KW_YEAR:
			request->add_byte(blr_extract_year);
			break;
		case KW_MONTH:
			request->add_byte(blr_extract_month);
			break;
		case KW_DAY:
			request->add_byte(blr_extract_day);
			break;
		case KW_HOUR:
			request->add_byte(blr_extract_hour);
			break;
		case KW_MINUTE:
			request->add_byte(blr_extract_minute);
			break;
		case KW_SECOND:
			request->add_byte(blr_extract_second);
			break;
		case KW_WEEKDAY:
			request->add_byte(blr_extract_weekday);
			break;
		case KW_YEARDAY:
			request->add_byte(blr_extract_yearday);
			break;
		default:
			CPR_error("CME_expr: Invalid extract part");
		}
		CME_expr(node->nod_arg[1], request);
		return;
	// End date/time/timestamp support

	// count2
	//case nod_count:
	//	if (node->nod_arg [1])
	//		break;
	//STUFF (blr_count);
	//CME_rse (node->nod_arg [0], request);
	//return;

	case nod_agg_total:
		if (node->nod_arg[1])
		{
			request->add_byte(blr_agg_total_distinct);
		}
		else
			request->add_byte(blr_agg_total);
		CME_expr(node->nod_arg[0], request);
		return;

	case nod_agg_average:
		if (node->nod_arg[1])
		{
			request->add_byte(blr_agg_average_distinct);
		}
		else
			request->add_byte(blr_agg_average);
		CME_expr(node->nod_arg[0], request);
		return;

	case nod_dom_value:
		request->add_byte(blr_fid);
		request->add_byte(0);			// Context
		request->add_word(0);			// Field id
		return;

	case nod_map_ref:
		{
			const mel* element = (mel*) node->nod_arg[0];
			context = element->mel_context;
			request->add_byte(blr_fid);
			request->add_byte(context->ctx_internal);
			request->add_word(element->mel_position);
			return;
		}

	case nod_current_connection:
		request->add_byte(blr_internal_info);
		request->add_byte(blr_literal);
		request->add_byte(blr_long);
		request->add_byte(0);
		request->add_long(INFO_TYPE_CONNECTION_ID);
		return;

	case nod_current_transaction:
		request->add_byte(blr_internal_info);
		request->add_byte(blr_literal);
		request->add_byte(blr_long);
		request->add_byte(0);
		request->add_long(INFO_TYPE_TRANSACTION_ID);
		return;

	case nod_coalesce:
		// Begin by casting the result of coalesce to the proper data type
		request->add_byte(blr_cast);
		get_dtype_of_list(node->nod_arg[0], &field);
		CMP_external_field(request, &field);
		// Now add the 'if <expr1> is null then[[ if <expr2> is null then] ...]' stuff
		for (int i = 0; i < node->nod_arg[0]->nod_count; i++)
		{
			request->add_byte(blr_value_if);
			request->add_byte(blr_missing);
			CME_expr(node->nod_arg[0]->nod_arg[i], request);
		}
		// Add blr_null to return something if all expressions evaluate to null
		request->add_byte(blr_null);
		// Now add the 'else <exprn>[[ else <exprn - 1>] ...]' stuff
		for (int j = node->nod_arg[0]->nod_count - 1; j >= 0; j--)
		{
			CME_expr(node->nod_arg[0]->nod_arg[j], request);
		}
		return;

	case nod_case:
		// Begin by casting the result of case to the proper data type
		request->add_byte(blr_cast);
		get_dtype_of_case(node, &field);
		CMP_external_field(request, &field);
		// Now add the WHEN ... THEN ... clauses
		for (int i = 0; i < (node->nod_count - 1); i += 2)
		{
			request->add_byte(blr_value_if);
			CME_expr(node->nod_arg[i], request);
			CME_expr(node->nod_arg[i + 1], request);
		}
		// Now add the ELSE clause
		if ((node->nod_count % 2) == 1)
		{
			CME_expr(node->nod_arg[node->nod_count - 1], request);
		}
		else
		{
			request->add_byte(blr_null);
		}
		return;

	case nod_case1:
		// Begin by casting the result of case to the proper data type
		request->add_byte(blr_cast);
		get_dtype_of_case(node, &field);
		CMP_external_field(request, &field);
		// Now add the WHEN ... THEN ... clauses
		for (int i = 1; i < (node->nod_count - 1); i += 2)
		{
			request->add_byte(blr_value_if);
			request->add_byte(blr_eql);
			CME_expr(node->nod_arg[0], request);
			CME_expr(node->nod_arg[i], request);
			CME_expr(node->nod_arg[i + 1], request);
		}
		// Now add the ELSE clause
		if ((node->nod_count % 2) == 0)
		{
			CME_expr(node->nod_arg[node->nod_count - 1], request);
		}
		else
		{
			request->add_byte(blr_null);
		}
		return;

	case nod_substring:
		request->add_byte(blr_substring);
		CME_expr(node->nod_arg[0], request);
		// We need to subtract 1 from the FROM value since it is 1 relative
		// but blr_substring requires that it be 0 relative.
		request->add_byte(blr_subtract);
		CME_expr(node->nod_arg[1], request);
		request->add_byte(blr_literal);
		request->add_byte(blr_long);
		request->add_byte(0);
		request->add_long(1);
		CME_expr(node->nod_arg[2], request);
		return;
	}

	const op_table* nod2blr_operator;
	for (nod2blr_operator = operators; nod2blr_operator->op_type != node->nod_type; ++nod2blr_operator)
	{
		if (!nod2blr_operator->op_blr)
		{
			CPR_bugcheck("node type not implemented");
			return;
		}
	}

	request->add_byte(nod2blr_operator->op_blr);
	gpre_nod** ptr = node->nod_arg;

	for (const gpre_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
		CME_expr(*ptr, request);

	switch (node->nod_type)
	{
	case nod_any:
	case nod_ansi_any:
	case nod_ansi_all:
	case nod_unique:
	// count2 next line would be deleted
	case nod_count:
		CME_rse(node->nod_arg[0], request);
		break;

	case nod_max:
	case nod_min:
	case nod_average:
	case nod_total:
	case nod_from:
//
//	case nod_count:
//
		CME_rse(node->nod_arg[0], request);
		CME_expr(node->nod_arg[1], request);
		break;

	case nod_via:
		CME_rse(node->nod_arg[0], request);
		CME_expr(node->nod_arg[1], request);
		CME_expr(node->nod_arg[2], request);
	}
}


//____________________________________________________________
//
//		Compute datatype, length, and scale of an expression.
//

void CME_get_dtype(const gpre_nod* node, gpre_fld* f)
{
	gpre_fld field1, field2;
	SSHORT dtype_max;
	const TEXT* string;
	const ref* reference;
	const gpre_fld* tmp_field;
	const udf* a_udf;

	f->fld_dtype = 0;
	f->fld_length = 0;
	f->fld_scale = 0;
	f->fld_sub_type = 0;
	f->fld_charset_id = 0;
	f->fld_collate_id = 0;
	f->fld_ttype = 0;

	switch (node->nod_type)
	{
	case nod_null:
		// This occurs when SQL statement specifies a literal NULL, eg:
		// SELECT NULL FROM TABLE1;
		// As we don't have a <dtype_null, HOSTTYPE> datatype pairing,
		// we don't know how to map this NULL to a host-language
		// datatype.  Therefore we now describe it as a
		// CHAR(1) CHARACTER SET NONE type.
		// No value will ever be sent back, as the value of the select
		// will be NULL - this is only for purposes of allocating
		// values in the message DESCRIBING
		// the statement.
		// Other parts of gpre aren't too happy with a dtype_unknown datatype

		f->fld_dtype = dtype_text;
		f->fld_length = 1;
		f->fld_ttype = ttype_none;
		f->fld_charset_id = CS_NONE;
		return;

	case nod_map_ref:
		{
			const mel* element = (mel*) node->nod_arg[0];
			CME_get_dtype(element->mel_expr, f);
			return;
		}

	case nod_value:
	case nod_field:
	case nod_array:
		reference = (ref*) node->nod_arg[0];
		if (!(tmp_field = reference->ref_field))
			CPR_error("CME_get_dtype: node type not supported");
		if (!tmp_field->fld_dtype || !tmp_field->fld_length)
			PAR_error("Inappropriate self-reference of field");

		assign_dtype(f, tmp_field);
		return;

	case nod_agg_count:
	case nod_count:
		f->fld_dtype = dtype_long;
		f->fld_length = sizeof(SLONG);
		return;

	case nod_gen_id:
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5 || gpreGlob.sw_server_version < 6)
		{
			f->fld_dtype = dtype_long;
			f->fld_length = sizeof(SLONG);
		}
		else
		{
			f->fld_dtype = dtype_int64;
			f->fld_length = sizeof(ISC_INT64);
		}
		return;

	case nod_max:
	case nod_min:
	case nod_from:
		CME_get_dtype(node->nod_arg[1], f);
		return;

	case nod_agg_max:
	case nod_agg_min:
	case nod_negate:
		CME_get_dtype(node->nod_arg[0], f);
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5 && f->fld_dtype == dtype_int64)
		{
			f->fld_precision = 0;
			f->fld_scale = 0;
			f->fld_dtype = dtype_double;
			f->fld_length = sizeof(double);
		}
		return;

	// Begin date/time/timestamp support
	case nod_extract:
		{
			const kwwords_t kw_word = (kwwords_t) (IPTR) node->nod_arg[0];
			CME_get_dtype(node->nod_arg[1], f);
			switch (f->fld_dtype)
			{
			case dtype_timestamp:
				break;

			case dtype_sql_date:
				if (kw_word == KW_HOUR || kw_word == KW_MINUTE || kw_word == KW_SECOND)
				{
					CPR_error("Invalid extract part for SQL DATE type");
				}
				break;

			case dtype_sql_time:
				if (kw_word != KW_HOUR && kw_word != KW_MINUTE && kw_word != KW_SECOND)
				{
					CPR_error("Invalid extract part for SQL TIME type");
				}
				break;

			default:
				CPR_error("Invalid use of EXTRACT function");
			}
			switch (kw_word)
			{
			case KW_YEAR:
			case KW_MONTH:
			case KW_DAY:
			case KW_WEEKDAY:
			case KW_YEARDAY:
			case KW_HOUR:
			case KW_MINUTE:
				f->fld_dtype = dtype_short;
				f->fld_length = sizeof(short);
				break;
			case KW_SECOND:
				f->fld_dtype = dtype_long;
				f->fld_length = sizeof(long);
				f->fld_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
				break;
			default:
				CPR_error("Invalid EXTRACT part");
			}
			return;
		}

	case nod_current_date:
		f->fld_dtype = dtype_sql_date;
		f->fld_length = sizeof(ISC_DATE);
		return;

	case nod_current_time:
		f->fld_dtype = dtype_sql_time;
		f->fld_length = sizeof(ISC_TIME);
		return;

	case nod_current_timestamp:
		f->fld_dtype = dtype_timestamp;
		f->fld_length = sizeof(ISC_TIMESTAMP);
		return;

	// End date/time/timestamp support

	case nod_times:
		CME_get_dtype(node->nod_arg[0], &field1);
		CME_get_dtype(node->nod_arg[1], &field2);
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5)
		{
			if (field1.fld_dtype == dtype_int64)
			{
				field1.fld_dtype = dtype_double;
				field1.fld_scale = 0;
				field1.fld_length = sizeof(double);
			}
			if (field2.fld_dtype == dtype_int64)
			{
				field2.fld_dtype = dtype_double;
				field2.fld_scale = 0;
				field2.fld_length = sizeof(double);
			}
			dtype_max = MAX(field1.fld_dtype, field2.fld_dtype);
			if (DTYPE_IS_DATE(dtype_max) || DTYPE_IS_BLOB(dtype_max))
			{
				CPR_error("Invalid use of date/blob/array value");
			}
		}
		else
		{
			dtype_max =	DSC_multiply_result[field1.fld_dtype][field2.fld_dtype];
			if (dtype_max == dtype_unknown)
			{
				CPR_error("Invalid operand used in multiplication");
			}
			else if (dtype_max == DTYPE_CANNOT)
			{
				CPR_error("expression evaluation not supported");
			}
		}

		switch (dtype_max)
		{
		case dtype_short:
		case dtype_long:
			f->fld_dtype = dtype_long;
			f->fld_scale = field1.fld_scale + field2.fld_scale;
			f->fld_length = sizeof(SLONG);
			break;
		case dtype_int64:
			f->fld_dtype = dtype_int64;
			f->fld_scale = field1.fld_scale + field2.fld_scale;
			f->fld_length = sizeof(ISC_INT64);
			break;
		default:
			f->fld_dtype = dtype_double;
			f->fld_scale = 0;
			f->fld_length = sizeof(double);
			break;
		}
		return;

	case nod_concatenate:
		CME_get_dtype(node->nod_arg[0], &field1);
		CME_get_dtype(node->nod_arg[1], &field2);
		dtype_max = MAX(field1.fld_dtype, field2.fld_dtype);
		if (field1.fld_dtype > dtype_any_text)
		{
			f->fld_dtype		= dtype_cstring;
			f->fld_char_length	= get_string_len(&field1) + get_string_len(&field2);
			f->fld_length		= f->fld_char_length + sizeof(USHORT);
			f->fld_charset_id	= CS_ASCII;
			f->fld_ttype		= ttype_ascii;
			return;
		}
		assign_dtype(f, &field1);
		f->fld_length = get_string_len(&field1) + get_string_len(&field2);
		if (f->fld_dtype == dtype_cstring)
			f->fld_length += 1;
		else if (f->fld_dtype == dtype_varying)
			f->fld_length += sizeof(USHORT);
		return;

	case nod_plus:
	case nod_minus:
		CME_get_dtype(node->nod_arg[0], &field1);
		CME_get_dtype(node->nod_arg[1], &field2);
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5)
		{
			if (field1.fld_dtype == dtype_int64)
			{
				field1.fld_dtype = dtype_double;
				field1.fld_scale = 0;
				field1.fld_length = sizeof(double);
			}
			if (field2.fld_dtype == dtype_int64)
			{
				field2.fld_dtype = dtype_double;
				field2.fld_scale = 0;
				field2.fld_length = sizeof(double);
			}

			if (DTYPE_IS_DATE(field1.fld_dtype) && DTYPE_IS_DATE(field2.fld_dtype) &&
				!(node->nod_type == nod_minus || is_date_and_time(field1.fld_dtype, field2.fld_dtype)))
			{
				CPR_error("Invalid use of timestamp/date/time value");
				return; // silence non initialized warning
			}

			dtype_max = MAX(field1.fld_dtype, field2.fld_dtype);
			if (DTYPE_IS_BLOB(dtype_max))
			{
				CPR_error("Invalid use of blob/array value");
				return; // silence non initialized warning
			}
		}
		else
		{
			// Dialect is > 1

			if (node->nod_type == nod_plus)
				dtype_max = DSC_add_result[field1.fld_dtype][field2.fld_dtype];
			else
				dtype_max = DSC_sub_result[field1.fld_dtype][field2.fld_dtype];

			if (dtype_max == dtype_unknown)
			{
				CPR_error("Illegal operands used in addition");
				return; // silence non initialized warning
			}

			if (dtype_max == DTYPE_CANNOT)
			{
				CPR_error("expression evaluation not supported");
				return; // silence non initialized warning
			}
		}

        switch (dtype_max)
        {
		case dtype_short:
		case dtype_long:
			f->fld_dtype = dtype_long;
			f->fld_scale = MIN(field1.fld_scale, field2.fld_scale);
			f->fld_length = sizeof(SLONG);
			break;
		// Begin date/time/timestamp support
		case dtype_sql_date:
			f->fld_dtype = dtype_sql_date;
			f->fld_scale = 0;
			f->fld_length = sizeof(ISC_DATE);
			break;
		case dtype_sql_time:
			f->fld_dtype = dtype_sql_time;
			f->fld_scale = 0;
			f->fld_length = sizeof(ISC_TIME);
			break;
		case dtype_timestamp:
			f->fld_dtype = dtype_timestamp;
			f->fld_scale = 0;
			f->fld_length = sizeof(ISC_TIMESTAMP);
			break;
		// End date/time/timestamp support

		case dtype_int64:
			f->fld_dtype = dtype_int64;
			f->fld_scale = MIN(field1.fld_scale, field2.fld_scale);
			f->fld_length = sizeof(ISC_INT64);
			break;
		default:
			f->fld_dtype = dtype_double;
			f->fld_scale = 0;
			f->fld_length = sizeof(double);
			break;
		}
		return;

	case nod_via:
		CME_get_dtype(node->nod_arg[1], &field1);
		CME_get_dtype(node->nod_arg[2], &field2);
		if (field1.fld_dtype >= field2.fld_dtype)
		{
			assign_dtype(f, &field1);
		}
		else
		{
			assign_dtype(f, &field2);
		}
		return;

	case nod_divide:
		CME_get_dtype(node->nod_arg[0], &field1);
		CME_get_dtype(node->nod_arg[1], &field2);
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5)
		{
			if (field1.fld_dtype == dtype_int64)
			{
				field1.fld_dtype = dtype_double;
				field1.fld_scale = 0;
				field1.fld_length = sizeof(double);
			}
			if (field2.fld_dtype == dtype_int64)
			{
				field2.fld_dtype = dtype_double;
				field2.fld_scale = 0;
				field2.fld_length = sizeof(double);
			}
			dtype_max = MAX(field1.fld_dtype, field2.fld_dtype);
			if (DTYPE_IS_DATE(dtype_max) || DTYPE_IS_BLOB(dtype_max))
				CPR_error("Invalid use of date/blob/array value");
			f->fld_dtype = dtype_double;
			f->fld_length = sizeof(double);
			return;
		}

		dtype_max = DSC_multiply_result[field1.fld_dtype][field2.fld_dtype];
		if (dtype_max == dtype_unknown)
			CPR_error("Illegal operands used in division");
		else if (dtype_max == DTYPE_CANNOT)
			CPR_error("expression evaluation not supported");

		if (dtype_max == dtype_int64)
		{
			f->fld_dtype = dtype_int64;
			f->fld_scale = field1.fld_scale + field2.fld_scale;
			f->fld_length = sizeof(ISC_INT64);
		}
		else
		{
			f->fld_dtype = dtype_double;
			f->fld_scale = 0;
			f->fld_length = sizeof(double);
		}
		return;

	case nod_average:
	case nod_agg_average:
		if (node->nod_type == nod_average)
			CME_get_dtype(node->nod_arg[1], f);
		else
			CME_get_dtype(node->nod_arg[0], f);
		if (!DTYPE_IS_NUMERIC(f->fld_dtype))
			CPR_error("expression evaluation not supported");
		if (gpreGlob.sw_sql_dialect != SQL_DIALECT_V5)
		{
			if (DTYPE_IS_EXACT(f->fld_dtype))
			{
				f->fld_dtype = dtype_int64;
				f->fld_length = sizeof(SINT64);
			}
			else
			{
				f->fld_dtype = dtype_double;
				f->fld_length = sizeof(double);
			}
		}
		return;

	case nod_agg_total:
	case nod_total:
		if (node->nod_type == nod_total)
			CME_get_dtype(node->nod_arg[1], f);
		else
			CME_get_dtype(node->nod_arg[0], f);
		if (gpreGlob.sw_sql_dialect == SQL_DIALECT_V5)
		{
			if ((f->fld_dtype == dtype_short) || (f->fld_dtype == dtype_long))
			{
				f->fld_dtype = dtype_long;
				f->fld_length = sizeof(SLONG);
			}
			else
			{
				f->fld_precision = 0;
				f->fld_scale = 0;
				f->fld_dtype = dtype_double;
				f->fld_length = sizeof(double);
			}
		}
		else
		{
			// Dialect is 2 or 3

			if (DTYPE_IS_EXACT(f->fld_dtype))
			{
				f->fld_dtype = dtype_int64;
				f->fld_length = sizeof(SINT64);
			}
			else
			{
				f->fld_dtype = dtype_double;
				f->fld_length = sizeof(double);
			}
		}
		return;

	case nod_literal:
		reference = (ref*) node->nod_arg[0];
		string = reference->ref_value;
		if (*string != '"' && *string != '\'')
		{
			// Value didn't start with a quotemark - must be a numeric
			// that we stuffed away as a string during the parse
			if (strpbrk(string, "Ee"))
			{
				f->fld_dtype = dtype_double;
				f->fld_length = sizeof(double);
			}
			else
			{
				int scale = 0;

				const char* s_ptr = string;

				// Get the scale
				const char* ptr = strpbrk(string, ".");
				if (ptr)
				{
					scale = (string + (strlen(string) - 1)) - ptr;
					scale = -scale;
				}

				// Get rid of the decimal point
				FB_UINT64 uint64_val = 0;
				while (*s_ptr)
				{
					if (*s_ptr != '.')
						uint64_val = (uint64_val * 10) + (*s_ptr - '0');
					s_ptr++;
				}

				if (uint64_val <= MAX_SLONG)
				{
					f->fld_dtype = dtype_long;
					f->fld_scale = scale;
					f->fld_length = sizeof(SLONG);
				}
				else
				{
					f->fld_dtype = dtype_int64;
					f->fld_scale = scale;
					f->fld_length = sizeof(SINT64);
				}
			}
		}
		else
		{
			// Did the reference include a character set specification?

			if (reference->ref_flags & REF_ttype)
				f->fld_ttype = reference->ref_ttype;

			if (reference->ref_flags & REF_sql_date)
			{
				f->fld_dtype = dtype_sql_date;
				f->fld_length = sizeof(ISC_DATE);
			}
			else if (reference->ref_flags & REF_sql_time)
			{
				f->fld_dtype = dtype_sql_time;
				f->fld_length = sizeof(ISC_TIME);
			}
			else if (reference->ref_flags & REF_timestamp)
			{
				f->fld_dtype = dtype_timestamp;
				f->fld_length = sizeof(ISC_TIMESTAMP);
			}
			else
			{
				// subtract 2 for starting & terminating quote

				f->fld_length = strlen(string) - 2;
				if (gpreGlob.sw_cstring)
				{
					// add 1 back for the NULL byte

					f->fld_length += 1;
					f->fld_dtype = dtype_cstring;
				}
				else
					f->fld_dtype = dtype_text;
			}
		}
		return;

	case nod_user_name:
		f->fld_dtype = dtype_text;
		f->fld_length = MAX_SQL_IDENTIFIER_SIZE; // why 32?
		f->fld_ttype = ttype_ascii;
		f->fld_charset_id = CS_ASCII;
		return;

	case nod_udf:
		a_udf = (udf*) node->nod_arg[1];
		f->fld_dtype = a_udf->udf_dtype;
		f->fld_length = a_udf->udf_length;
		f->fld_scale = a_udf->udf_scale;
		f->fld_sub_type = a_udf->udf_sub_type;
		f->fld_ttype = a_udf->udf_ttype;
		f->fld_charset_id = a_udf->udf_charset_id;
		return;

	case nod_cast:
		CME_get_dtype(node->nod_arg[0], &field1);
		tmp_field = (gpre_fld*) node->nod_arg[1];
		assign_dtype(f, tmp_field);
		if (f->fld_length == 0)
			f->fld_length = field1.fld_length;
		return;

	case nod_upcase:
	case nod_lowcase:
	case nod_substring:
		CME_get_dtype(node->nod_arg[0], f);
		if (f->fld_dtype <= dtype_any_text)
			return;

		// User has specified UPPER(5) - while silly, we'll cast
		// the value into a string, and upcase it anyway

		f->fld_length = get_string_len(f) + sizeof(USHORT);
		f->fld_dtype = dtype_cstring;
		f->fld_ttype = ttype_ascii;
		f->fld_charset_id = CS_ASCII;
		return;

	case nod_current_connection:
	case nod_current_transaction:
		f->fld_dtype = dtype_long;
		f->fld_length = sizeof(SLONG);
		return;

	case nod_current_role:
		f->fld_dtype = dtype_text;
		f->fld_ttype = ttype_ascii;
		f->fld_charset_id = CS_ASCII;
		f->fld_length = MAX_SQL_IDENTIFIER_SIZE; // why 32?
		return;

	case nod_coalesce:
		get_dtype_of_list(node->nod_arg[0], f);
		return;

	case nod_case:
	case nod_case1:
		get_dtype_of_case(node, f);
		return;

	default:
		CPR_error("CME_get_dtype: node type not supported");
	}
}


//____________________________________________________________
//
//		Generate a relation reference.
//

void CME_relation(gpre_ctx* context, gpre_req* request)
{
	CMP_check(request, 0);

	gpre_rse* rs_stream = context->ctx_stream;
	if (rs_stream)
	{
		CME_rse(rs_stream, request);
		return;
	}

	gpre_prc* procedure;
	gpre_rel* relation = context->ctx_relation;
	if (relation)
	{
		if (gpreGlob.sw_ids)
		{
			if (context->ctx_alias)
			{
				request->add_byte(blr_rid2);
			}
			else
			{
				request->add_byte(blr_rid);
			}
			request->add_word(relation->rel_id);
		}
		else
		{
			if (context->ctx_alias)
			{
				request->add_byte(blr_relation2);
			}
			else
				request->add_byte(blr_relation);
			CMP_stuff_symbol(request, relation->rel_symbol);
		}

		if (context->ctx_alias)
		{
			request->add_cstring(context->ctx_alias);
		}
		request->add_byte(context->ctx_internal);
	}
	else if (procedure = context->ctx_procedure)
	{
		if (gpreGlob.sw_ids)
		{
			request->add_byte(blr_pid);
			request->add_word(procedure->prc_id);
		}
		else
		{
			request->add_byte(blr_procedure);
			CMP_stuff_symbol(request, procedure->prc_symbol);
		}
		request->add_byte(context->ctx_internal);
		request->add_word(procedure->prc_in_count);
		gpre_nod* inputs = context->ctx_prc_inputs;
		if (inputs)
		{
			gpre_nod** ptr = inputs->nod_arg;
			for (const gpre_nod* const* const end = ptr + inputs->nod_count; ptr < end; ptr++)
			{
				CME_expr(*ptr, request);
			}
		}
	}
}


//____________________________________________________________
//
//		Generate blr for an rse node.
//

void CME_rse(gpre_rse* selection, gpre_req* request)
{
	if (selection->rse_join_type == nod_nothing)
	{
		if (selection->rse_flags & RSE_singleton)
		{
			request->add_byte(blr_singular);
		}
		request->add_byte(blr_rse);
	}
	else
		request->add_byte(blr_rs_stream);

	// Process unions, if any, otherwise process relations

	gpre_rse* sub_rse = 0;
	gpre_nod* union_node = selection->rse_union;
	if (union_node)
	{
		request->add_byte(1);
		request->add_byte(blr_union);
		request->add_byte(selection->rse_context[0]->ctx_internal);
		request->add_byte(union_node->nod_count);
		gpre_nod** ptr = union_node->nod_arg;
		for (const gpre_nod* const* const end = ptr + union_node->nod_count; ptr < end; ptr++)
		{
			sub_rse = (gpre_rse*) *ptr;
			CME_rse(sub_rse, request);
			cmp_map(sub_rse->rse_map, request);
		}
	}
	else if (sub_rse = selection->rse_aggregate)
	{
		request->add_byte(1);
		request->add_byte(blr_aggregate);
		request->add_byte(sub_rse->rse_map->map_context->ctx_internal);
		CME_rse(sub_rse, request);
		request->add_byte(blr_group_by);
		gpre_nod* list = sub_rse->rse_group_by;
		if (list)
		{
			request->add_byte(list->nod_count);
			gpre_nod** ptr = list->nod_arg;
			for (const gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
			{
				CME_expr(*ptr, request);
			}
		}
		else
			request->add_byte(0);
		cmp_map(sub_rse->rse_map, request);
	}
	else
	{
		request->add_byte(selection->rse_count);
		for (USHORT i = 0; i < selection->rse_count; i++)
			CME_relation(selection->rse_context[i], request);
		if (selection->rse_flags & RSE_with_lock)
			request->add_byte(blr_writelock);
	}

	// Process the clauses present

	if (selection->rse_first)
	{
		request->add_byte(blr_first);
		CME_expr(selection->rse_first, request);
	}

	if (selection->rse_sqlfirst)
	{
		request->add_byte(blr_first);
		CME_expr(selection->rse_sqlfirst->nod_arg[0], request);
	}

	if (selection->rse_sqlskip)
	{
		request->add_byte(blr_skip);
		CME_expr(selection->rse_sqlskip->nod_arg[0], request);
	}

	if (selection->rse_boolean)
	{
		request->add_byte(blr_boolean);
		CME_expr(selection->rse_boolean, request);
	}

	gpre_nod* temp = selection->rse_sort;
	if (temp)
	{
		request->add_byte(blr_sort);
		request->add_byte(temp->nod_count);
		gpre_nod** ptr = temp->nod_arg;
		for (USHORT i = 0; i < temp->nod_count; i++)
		{
			request->add_byte((*ptr++) ? blr_descending : blr_ascending);
			CME_expr(*ptr++, request);
		}
	}

	if (temp = selection->rse_reduced)
	{
		request->add_byte(blr_project);
		request->add_byte(temp->nod_count);
		gpre_nod** ptr = temp->nod_arg;
		for (USHORT i = 0; i < temp->nod_count; i++)
			CME_expr(*ptr++, request);
	}

	if (temp = selection->rse_plan)
	{
		request->add_byte(blr_plan);
		cmp_plan(temp, request);
	}

	if (selection->rse_join_type != nod_nothing && selection->rse_join_type != nod_join_inner)
	{
		request->add_byte(blr_join_type);
		switch (selection->rse_join_type)
		{
		case nod_join_left:
			request->add_byte(blr_left);
			break;
		case nod_join_right:
			request->add_byte(blr_right);
			break;
		default:
			request->add_byte(blr_full);
		}
	}

	// Finish up by making a BLR_END

	request->add_byte(blr_end);
}


//____________________________________________________________
//
//		Compile up an array reference putting
//       out sdl (slice description language)
//

static void cmp_array( gpre_nod* node, gpre_req* request)
{
	CMP_check(request, 0);

	ref* reference = (ref*) node->nod_arg[0];

	if (!reference->ref_context)
	{
		CPR_error("cmp_array: context missing");
		return; //NULL;
	}

	const gpre_fld* field = reference->ref_field;
	if (!field)
	{
		CPR_error("cmp_array: field missing");
		return; // NULL;
	}

	// Header stuff

	reference->ref_sdl = reference->ref_sdl_base = MSC_alloc(500);
	reference->ref_sdl_length = 500;
	reference->ref_sdl_ident = CMP_next_ident();
	reference->add_byte(isc_sdl_version1);
	reference->add_byte(isc_sdl_struct);
	reference->add_byte(1);

	// The datatype of the array elements

	cmp_sdl_dtype(field->fld_array, reference);

	// The relation and field identifiers or strings

	if (gpreGlob.sw_ids)
	{
		reference->add_byte(isc_sdl_rid);
		reference->add_byte(reference->ref_id);
		reference->add_byte(isc_sdl_fid);
		reference->add_byte(field->fld_id);
	}
	else
	{
		reference->add_byte(isc_sdl_relation);
		reference->add_byte(strlen(field->fld_relation->rel_symbol->sym_string));
		for (const TEXT* p = field->fld_relation->rel_symbol->sym_string; *p; p++)
			reference->add_byte(*p);
		reference->add_byte(isc_sdl_field);
		reference->add_byte(strlen(field->fld_symbol->sym_string));
		for (const TEXT* p = field->fld_symbol->sym_string; *p; p++)
			reference->add_byte(*p);
	}

	// The loops for the dimensions

	stuff_sdl_loops(reference, field);

	// The array element and its "subscripts"

	stuff_sdl_element(reference, field);

	reference->add_byte(isc_sdl_eoc);

	reference->ref_sdl_length = reference->ref_sdl - reference->ref_sdl_base;
	reference->ref_sdl = reference->ref_sdl_base;

	if (debug_on)
		PRETTY_print_sdl(reference->ref_sdl, 0, 0, 0);

	//return node;
}


//____________________________________________________________
//
//		Compile up a subscripted array reference
//       from an gpre_rse and output blr for this reference
//

static void cmp_array_element( gpre_nod* node, gpre_req* request)
{
	request->add_byte(blr_index);

	cmp_field(node, request);

	request->add_byte(node->nod_count - 1);

	for (USHORT index_count = 1; index_count < node->nod_count; index_count++)
		CME_expr(node->nod_arg[index_count], request);

	// return node;
}


//____________________________________________________________
//
//

static void cmp_cast( gpre_nod* node, gpre_req* request)
{

	request->add_byte(blr_cast);
	CMP_external_field(request, (const gpre_fld*) node->nod_arg[1]);
	CME_expr(node->nod_arg[0], request);
}


//____________________________________________________________
//
//		Compile up a field reference.
//

static void cmp_field( const gpre_nod* node, gpre_req* request)
{
	CMP_check(request, 0);

	const ref* reference = (ref*) node->nod_arg[0];
	if (!reference)
	{
		CPR_error("cmp_field: reference missing");
		return; // NULL;
	}

	const gpre_ctx* context = reference->ref_context;
	if (!context)
	{
		CPR_error("cmp_field: context missing");
		return; // NULL;
	}

	const gpre_fld* field = reference->ref_field;
	if (!field)
	{
		CPR_error("cmp_field: field missing");
		return; // NULL;
	}

	if (!field)
		puts("cmp_field: symbol missing");

	if (field->fld_flags & FLD_dbkey)
	{
		request->add_byte(blr_dbkey);
		request->add_byte(context->ctx_internal);
	}
	/* This code cannot run because REF_union is never activated, parser bug?
	else if (reference->ref_flags & REF_union)
	{
		request->add_byte(blr_fid);
		request->add_byte(context->ctx_internal);
		request->add_word(reference->ref_id);
	}
	*/
	else if (gpreGlob.sw_ids)
	{
		request->add_byte(blr_fid);
		request->add_byte(context->ctx_internal);
		request->add_word(field->fld_id);
	}
	else
	{
		request->add_byte(blr_field);
		request->add_byte(context->ctx_internal);
		CMP_stuff_symbol(request, field->fld_symbol);
	}

	// return node;
}


//____________________________________________________________
//
//		Handle a literal expression.
//

static void cmp_literal( const gpre_nod* node, gpre_req* request)
{
	bool negate = false;

	if (node->nod_type == nod_negate)
	{
		node = node->nod_arg[0];
		negate = true;
	}

	request->add_byte(blr_literal);
	const ref* reference = (ref*) node->nod_arg[0];
	const char* string = reference->ref_value;

	if (*string != '"' && *string != '\'')
	{
		// If the numeric string contains an 'E' or 'e' in it
		// then the datatype is double.

		if (strpbrk(string, "Ee"))
		{
			string = reference->ref_value;

			request->add_byte(blr_double);
			request->add_word(strlen(string));
			while (*string)
				request->add_byte(*string++);
		}
		else
		{
			// The numeric string doesn't contain 'E' or 'e' in it.
			// Then this must be a scaled int.  Figure out if there
			// is a '.' in it and calculate its scale.

			const char* s_ptr = string;

			// Get the scale
			int scale = 0;
			const char* ptr = strpbrk(string, ".");
			if (ptr)
			{
				// Aha!, there is a '.'. find the scale
				scale = (string + (strlen(string) - 1)) - ptr;
				scale = -scale;
			}

			FB_UINT64 uint64_val = 0;
			while (*s_ptr)
			{
				if (*s_ptr != '.')
					uint64_val = (uint64_val * 10) + (*s_ptr - '0');
				s_ptr++;
			}

			// see if we can fit the value in a long or INT64.
			if ((uint64_val <= MAX_SLONG) || ((uint64_val == (MAX_SLONG + (FB_UINT64) 1)) && negate))
			{
				long long_val;
				if (negate)
					long_val = -((long) uint64_val);
				else
					long_val = (long) uint64_val;
				request->add_byte(blr_long);
				request->add_byte(scale);	// scale factor
				request->add_word(long_val);
				request->add_word(long_val >> 16);
			}
			else if ((uint64_val <= MAX_SINT64) ||
				((uint64_val == ((FB_UINT64) MAX_SINT64 + 1)) && negate))
			{
				SINT64 sint64_val;
				if (negate)
					sint64_val = -((SINT64) uint64_val);
				else
					sint64_val = (SINT64) uint64_val;
				request->add_byte(blr_int64);
				request->add_byte(scale);	// scale factor
				request->add_word(sint64_val);
				request->add_word(sint64_val >> 16);
				request->add_word(sint64_val >> 32);
				request->add_word(sint64_val >> 48);
			}
			else
				CPR_error("cmp_literal: Numeric Value too big");

		}
	}
	else
	{
		// Remove surrounding quotes from string, etc.
		char buffer[MAX_SYM_SIZE];
		char* p = buffer;

		// Skip introducing quote mark
		if (*string)
			string++;

		while (*string)
			*p++ = *string++;

		// Zap out terminating quote mark
		*--p = 0;
		const SSHORT length = p - buffer;

		dsc from;
		from.dsc_sub_type = ttype_ascii;
		from.dsc_flags = 0;
		from.dsc_dtype = dtype_text;
		from.dsc_length = length;
		from.dsc_address = (UCHAR*) buffer;
		dsc to;
		to.dsc_sub_type = 0;
		to.dsc_flags = 0;

		if (reference->ref_flags & REF_sql_date)
		{
			ISC_DATE dt;
			request->add_byte(blr_sql_date);
			to.dsc_dtype = dtype_sql_date;
			to.dsc_length = sizeof(ISC_DATE);
			to.dsc_address = (UCHAR*) & dt;
			MOVG_move(&from, &to);
			request->add_word(dt);
			request->add_word(dt >> 16);
			return; // node;
		}
		if (reference->ref_flags & REF_timestamp)
		{
			ISC_TIMESTAMP ts;
			request->add_byte(blr_timestamp);
			to.dsc_dtype = dtype_timestamp;
			to.dsc_length = sizeof(ISC_TIMESTAMP);
			to.dsc_address = (UCHAR*) & ts;
			MOVG_move(&from, &to);
			request->add_word(ts.timestamp_date);
			request->add_word(ts.timestamp_date >> 16);
			request->add_word(ts.timestamp_time);
			request->add_word(ts.timestamp_time >> 16);
			return; // node;
		}
		if (reference->ref_flags & REF_sql_time)
		{
			ISC_TIME itim;
			request->add_byte(blr_sql_time);
			to.dsc_dtype = dtype_sql_time;
			to.dsc_length = sizeof(ISC_DATE);
			to.dsc_address = (UCHAR*) & itim;
			MOVG_move(&from, &to);
			request->add_word(itim);
			request->add_word(itim >> 16);
			return; // node;
		}
		if (!(reference->ref_flags & REF_ttype))
			request->add_byte(blr_text);
		else
		{
			request->add_byte(blr_text2);
			request->add_word(reference->ref_ttype);
		}

		request->add_word(length);
		for (string = buffer; *string;)
			request->add_byte(*string++);
	}

	// return node;
}


//____________________________________________________________
//
//		Generate a map for a union or aggregate rse.
//

static void cmp_map(map* a_map, gpre_req* request)
{
	request->add_byte(blr_map);
	request->add_word(a_map->map_count);

	for (mel* element = a_map->map_elements; element; element = element->mel_next)
	{
		request->add_word(element->mel_position);
		CME_expr(element->mel_expr, request);
	}
}


//____________________________________________________________
//
//		Generate an access plan for a query.
//

static void cmp_plan(const gpre_nod* plan_expression, gpre_req* request)
{
	// stuff the join type

	const gpre_nod* list = plan_expression->nod_arg[1];
	if (list->nod_count > 1)
	{
		const gpre_nod* node = plan_expression->nod_arg[0];
		if (node)
			request->add_byte(blr_merge);
		else
			request->add_byte(blr_join);
		request->add_byte(list->nod_count);
	}

	// stuff one or more plan items

	gpre_nod* const* ptr = list->nod_arg;
	for (gpre_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		gpre_nod* node = *ptr;
		if (node->nod_type == nod_plan_expr)
		{
			cmp_plan(node, request);
			continue;
		}

		// if we're here, it must be a nod_plan_item

		request->add_byte(blr_retrieve);

		// stuff the relation--the relation id itself is redundant except
		// when there is a need to differentiate the base tables of views

		CME_relation((gpre_ctx*) node->nod_arg[2], request);

		// now stuff the access method for this stream

		const gpre_nod* arg = node->nod_arg[1];
		switch (arg->nod_type)
		{
		case nod_natural:
			request->add_byte(blr_sequential);
			break;

		case nod_index_order:
			request->add_byte(blr_navigational);
			request->add_cstring((TEXT*) arg->nod_arg[0]);
			break;

		case nod_index:
			{
				request->add_byte(blr_indices);
				arg = arg->nod_arg[0];
				request->add_byte(arg->nod_count);
				const gpre_nod* const* ptr2 = arg->nod_arg;
				for (const gpre_nod* const* const end2 = ptr2 + arg->nod_count;  ptr2 < end2; ptr2++)
				{
					request->add_cstring((TEXT*) *ptr2);
				}
				break;
			}
		}
	}
}


//____________________________________________________________
//
//		Print out the correct blr for
//       this datatype.
//

static void cmp_sdl_dtype( const gpre_fld* field, ref* reference)
{
	switch (field->fld_dtype)
	{
	case dtype_cstring:
		// 3.2j has new, tagged blr intruction for cstring

		if (gpreGlob.sw_know_interp)
		{
			reference->add_byte(blr_cstring2);
			reference->add_word(gpreGlob.sw_interp);
			reference->add_word(field->fld_length);
		}
		else
		{
			reference->add_byte(blr_cstring);
			reference->add_word(field->fld_length);
		}
		break;

	case dtype_text:
		// 3.2j has new, tagged blr intruction for text too

		if (gpreGlob.sw_know_interp)
		{
			reference->add_byte(blr_text2);
			reference->add_word(gpreGlob.sw_interp);
			reference->add_word(field->fld_length);
		}
		else
		{
			reference->add_byte(blr_text);
			reference->add_word(field->fld_length);
		}
		break;

	case dtype_varying:
		// 3.2j has new, tagged blr intruction for varying also

		if (gpreGlob.sw_know_interp)
		{
			reference->add_byte(blr_varying2);
			reference->add_word(gpreGlob.sw_interp);
			reference->add_word(field->fld_length);
		}
		else
		{
			reference->add_byte(blr_varying);
			reference->add_word(field->fld_length);
		}
		break;

	case dtype_short:
		reference->add_byte(blr_short);
		reference->add_byte(field->fld_scale);
		break;

	case dtype_long:
		reference->add_byte(blr_long);
		reference->add_byte(field->fld_scale);
		break;

	case dtype_quad:
		reference->add_byte(blr_quad);
		reference->add_byte(field->fld_scale);
		break;

	// Begin date/time/timestamp support
	case dtype_sql_date:
		reference->add_byte(blr_sql_date);
		break;
	case dtype_sql_time:
		reference->add_byte(blr_sql_time);
		break;

	case dtype_timestamp:
		reference->add_byte(blr_timestamp);
		break;
	// End date/time/timestamp support

	case dtype_int64:
		reference->add_byte(blr_int64);
		break;

	case dtype_real:
		reference->add_byte(blr_float);
		break;

	case dtype_double:
		if (gpreGlob.sw_d_float)
			reference->add_byte(blr_d_float);
		else
			reference->add_byte(blr_double);
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
//		Compile a reference to a user defined function.
//

static void cmp_udf( gpre_nod* node, gpre_req* request)
{
	const udf* an_udf = (udf*) node->nod_arg[1];
	request->add_byte(blr_function);
	const TEXT* p = an_udf->udf_function;
	request->add_byte(strlen(p));

	while (*p)
		request->add_byte(*p++);

	gpre_nod* list = node->nod_arg[0];
	if (list)
	{
		request->add_byte(list->nod_count);

		gpre_nod** ptr = list->nod_arg;
		for (gpre_nod** const end = ptr + list->nod_count; ptr < end; ++ptr)
		{
			CME_expr(*ptr, request);
		}
	}
	else
		request->add_byte(0);

	// return node;
}


//____________________________________________________________
//
//		Process a random value expression.
//

static void cmp_value( const gpre_nod* node, gpre_req* request)
{
	const ref* reference = (ref*) node->nod_arg[0];

	if (!reference)
		puts("cmp_value: missing reference");

	if (!reference->ref_port)
		puts("cmp_value: port missing");

	const ref* flag = reference->ref_null;
	if (flag)
	{
		request->add_byte(blr_parameter2);
		request->add_byte(reference->ref_port->por_msg_number);
		request->add_word(reference->ref_parameter);
		request->add_word(flag->ref_parameter);
	}
	else
	{
		request->add_byte(blr_parameter);
		request->add_byte(reference->ref_port->por_msg_number);
		request->add_word(reference->ref_parameter);
	}

	// return node;
}


//____________________________________________________________
//
//		Figure out a text length from a datatype and a length
//

static USHORT get_string_len( const gpre_fld* field)
{
	fb_assert(field->fld_dtype <= MAX_UCHAR);

	dsc tmp_dsc;
	tmp_dsc.dsc_dtype = (UCHAR) field->fld_dtype;
	tmp_dsc.dsc_length = field->fld_length;
	tmp_dsc.dsc_scale = 0;
	tmp_dsc.dsc_sub_type = 0;
	tmp_dsc.dsc_flags = 0;
	tmp_dsc.dsc_address = NULL;

	return DSC_string_length(&tmp_dsc);
}

//____________________________________________________________
//
//		Write to the sdl string, the do
//       loop for a particular dimension.
//

static void stuff_sdl_dimension(const dim* dimension, ref* reference, SSHORT dimension_count)
{

	// In the future, when we support slices, new code to handle the
	// user-defined slice ranges will be here.

	if (dimension->dim_lower == 1)
	{
		reference->add_byte(isc_sdl_do1);
		reference->add_byte(dimension_count);
		stuff_sdl_number(dimension->dim_upper, reference);
	}
	else
	{
		reference->add_byte(isc_sdl_do2);
		reference->add_byte(dimension_count);
		stuff_sdl_number(dimension->dim_lower, reference);
		stuff_sdl_number(dimension->dim_upper, reference);
	}
}


//____________________________________________________________
//
//		Write the element information
//       (including the subscripts) to
//       the SDL string for the array.
//

static void stuff_sdl_element(ref* reference, const gpre_fld* field)
{
	reference->add_byte(isc_sdl_element);
	reference->add_byte(1);
	reference->add_byte(isc_sdl_scalar);
	reference->add_byte(0);

	reference->add_byte(field->fld_array_info->ary_dimension_count);

	// Fortran needs the array in column-major order

	if (gpreGlob.sw_language == lang_fortran)
	{
		for (SSHORT i = field->fld_array_info->ary_dimension_count - 1; i >= 0; i--)
		{
			reference->add_byte(isc_sdl_variable);
			reference->add_byte(i);
		}
	}
	else
	{
		for (SSHORT i = 0; i < field->fld_array_info->ary_dimension_count; i++)
		{
			reference->add_byte(isc_sdl_variable);
			reference->add_byte(i);
		}
	}
}


//____________________________________________________________
//
//		Write loop information to the SDL
//       string for the array dimensions.
//

static void stuff_sdl_loops(ref* reference, const gpre_fld* field)
{
	// Fortran needs the array in column-major order

	if (gpreGlob.sw_language == lang_fortran)
	{
		const dim* dimension = field->fld_array_info->ary_dimension;
		while (dimension->dim_next)
			dimension = dimension->dim_next;

		for (SSHORT i = 0; i < field->fld_array_info->ary_dimension_count;
			 i++, dimension = dimension->dim_previous)
		{
			stuff_sdl_dimension(dimension, reference, i);
		}
	}
	else
	{
		SSHORT i = 0;
		for (const dim* dimension = field->fld_array_info->ary_dimension;
			 i < field->fld_array_info->ary_dimension_count;
			 i++, dimension = dimension->dim_next)
		{
			stuff_sdl_dimension(dimension, reference, i);
		}
	}
}


//____________________________________________________________
//
//		Write the number in the 'smallest'
//       form possible to the SDL string.
//

static void stuff_sdl_number(const SLONG number, ref* reference)
{
	if ((number > -16) && (number < 15))
	{
		reference->add_byte(isc_sdl_tiny_integer);
		reference->add_byte(number);
	}
	else if ((number > -32768) && (number < 32767))
	{
		reference->add_byte(isc_sdl_short_integer);
		reference->add_word(number);
	}
	else
	{
		reference->add_byte(isc_sdl_long_integer);
		reference->add_long(number);
	}
}

// Set the dtype, etc. of the given CASE node from the THEN and ELSE values
static void get_dtype_of_case(const gpre_nod* node, gpre_fld* f)
{
	gpre_nod* args;
	int j;
	int arg_count;

	// Set default values
	f->fld_dtype = dtype_unknown;
	f->fld_length = 0;
	f->fld_ttype = ttype_none;
	f->fld_charset_id = CS_NONE;

	switch (node->nod_type)
	{
	case nod_case:
		// In this case the return values are the odd numbered nodes and the ELSE
		// value is in the last node (maybe).
		arg_count = (node->nod_count / 2) + (node->nod_count % 2);
		args = MSC_node(nod_list, arg_count);
		// Get the THEN values
		j = 0;
		for (int i = 1; i < node->nod_count; i += 2, j++)
		{
			args->nod_arg[j] = node->nod_arg[i];
		}
		// Get the ELSE value
		if ((node->nod_count % 2) == 1)
		{
			args->nod_arg[j] = node->nod_arg[node->nod_count - 1];
		}
		get_dtype_of_list(args, f);
		MSC_free(args);
		break;

	case nod_case1:
		// In this case the return values are in the even numbered nodes (starting with 2)
		// and the ELSE value is in the last node (maybe).
		arg_count = (node->nod_count / 2);
		args = MSC_node(nod_list, arg_count);
		// Get the then values
		j = 0;
		for (int i = 2; i < node->nod_count; i += 2, j++)
		{
			args->nod_arg[j] = node->nod_arg[i];
		}
		// Get the ELSE value
		if ((node->nod_count % 2) == 0)
		{
			args->nod_arg[j] = node->nod_arg[node->nod_count - 1];
		}
		get_dtype_of_list(args, f);
		MSC_free(args);
		break;
	}
}

// Set the dtype, etc. of the given node from the list of expressions contained in that node
// using the same algorithm used in DataTypeUtilBase::makeFromList.

// If any datatype has a character type then :
// - the output will always be a character type except unconvertable types.
//   (dtype_text, dtype_cstring, dtype_varying, dtype_blob sub_type TEXT)
// !!  Currently engine cannot convert string to BLOB therefor BLOB isn't allowed. !!
// - first character-set and collation are used as output descriptor.
// - if all types have datatype CHAR then output should be CHAR else
//   VARCHAR and with the maximum length used from the given list.
//
// If all of the datatypes are EXACT numeric then the output descriptor
// shall be EXACT numeric with the maximum scale and the maximum precision
// used. (dtype_byte, dtype_short, dtype_long, dtype_int64)
//
// If any of the datatypes is APPROXIMATE numeric then each datatype in the
// list shall be numeric else a error is thrown and the output descriptor
// shall be APPROXIMATE numeric. (dtype_real, dtype_double, dtype_d_float)
//
// If any of the datatypes is a datetime type then each datatype in the
// list shall be the same datetime type else a error is thrown.
// numeric. (dtype_sql_date, dtype_sql_time, dtype_timestamp)
//
// If any of the datatypes is a BLOB datatype then :
// - all types should be a BLOB else throw error.
// - all types should have the same sub_type else throw error.
// - when TEXT type then use first character-set and collation as output
//   descriptor.
// (dtype_blob)

static void get_dtype_of_list(const gpre_nod* node, gpre_fld* f)
{
	// Initialize values.
	UCHAR max_dtype = 0;
	SCHAR max_scale = 0;
	USHORT max_length = 0, max_dtype_length = 0, maxtextlength = 0, max_significant_digits = 0;
	SSHORT max_sub_type = 0, first_sub_type = -1, ttype = ttype_ascii; // default type if all nodes are nod_null.
	SSHORT max_numeric_sub_type = 0;
	bool firstarg = true, all_same_sub_type = true, all_equal = true; //, all_nulls = true;
	bool all_numeric = true, any_numeric = false, any_approx = false, any_float = false;
	bool all_text = true, any_text = false, any_varying = false;
	bool all_date = true, all_time = true, all_timestamp = true, any_datetime = false;
	bool all_blob = true, any_text_blob = false;
	//bool nullable = false;
	//bool err = false;
	gpre_fld field_aux;

	// Set default values
	f->fld_dtype = dtype_unknown;
	f->fld_length = 0;
	f->fld_ttype = ttype_none;
	f->fld_charset_id = CS_NONE;

	// If not a list node, exit
	if (node->nod_type != nod_list)
		return;

	// Process all elements of list
	for (int i = 0; i < node->nod_count; i++)
	{
		CME_get_dtype(node->nod_arg[i], &field_aux);
		const gpre_fld& field = field_aux; // Trick to avoid more assignment mistakes.

		// Initialize some values if this is the first time
		if (firstarg)
		{
			max_scale = field.fld_scale;
			max_length = max_dtype_length = field.fld_length;
			max_sub_type = first_sub_type = field.fld_sub_type;
			max_dtype = field.fld_dtype;
			firstarg = false;
		}
		else
		{
			if (all_equal)
			{
				all_equal = (max_dtype == field.fld_dtype) &&
							(max_scale == field.fld_scale) &&
							(max_length == field.fld_length) &&
							(max_sub_type == field.fld_sub_type);
			}
		}

		// Numeric data types
		if (DTYPE_IS_NUMERIC(field.fld_dtype))
		{
			any_numeric = true;
			if (DTYPE_IS_APPROX(field.fld_dtype))
			{
				any_approx = true;
				// Dialect 1 NUMERIC and DECIMAL are stroed as sub-types
				// 1 and 2 from float types dtype_real and dtype_double
				if (! any_float)
					any_float = (field.fld_sub_type == 0);
			}
			if (field.fld_sub_type > max_numeric_sub_type)
				max_numeric_sub_type = field.fld_sub_type;
		}
		else
			all_numeric = false;

		// Get the max scale and length (precision)
		// scale is negative!!
		if (field.fld_scale < max_scale)
			max_scale = field.fld_scale;
		if (field.fld_length > max_length)
			max_length = field.fld_length;

		// Get max significant bits
		if (type_significant_bits[field.fld_dtype] > max_significant_digits)
			max_significant_digits = type_significant_bits[field.fld_dtype];

		// Get max dtype and sub_type
		if (field.fld_dtype > max_dtype)
		{
			max_dtype = field.fld_dtype;
			max_dtype_length = field.fld_length;
		}
		if (field.fld_sub_type > max_sub_type)
			max_sub_type = field.fld_sub_type;
		if (field.fld_sub_type != first_sub_type)
			all_same_sub_type = false;

		// Text fields
		if (DTYPE_IS_TEXT(field.fld_dtype))
		{
			if (field.fld_length > maxtextlength)
				maxtextlength = field.fld_length;
			if ((field.fld_dtype == dtype_varying) || (field.fld_dtype == dtype_cstring))
				any_varying = true;
			// Pick the first charset from the list
			if (! any_text)
				ttype = field.fld_ttype;
			else
			{
				if ((ttype == ttype_none) || (ttype == ttype_ascii))
					ttype = field.fld_ttype;
			}
			any_text = true;
		}
		else
		{
			// Get max needed length for non-text types such as int64, timestamp, etc.
			const USHORT cnvlength = DSC_convert_to_text_length(field.fld_dtype);
			if (cnvlength > maxtextlength)
				maxtextlength = cnvlength;
			all_text = false;
		}

		// Date fields
		if (DTYPE_IS_DATE(field.fld_dtype))
		{
			any_datetime = true;
			switch (field.fld_dtype)
			{
			case dtype_sql_date:
				all_time = false;
				all_timestamp = false;
				break;
			case dtype_sql_time:
				all_date = false;
				all_timestamp = false;
				break;
			case dtype_timestamp:
				all_date = false;
				all_time = false;
				break;
			}
		}
		else
		{
			all_date = false;
			all_time = false;
			all_timestamp = false;
		}

		// Blob fields
		if (field.fld_dtype == dtype_blob)
		{
			// When there was already an other data type, raise immediate error
			if (!all_blob || !all_same_sub_type)
			{
				CPR_error("Incompatible data types");
				return;
			}

			if (field.fld_sub_type == 1)
			{
				// Text sub type
				if (! any_text_blob)
					ttype = field.fld_ttype;
				any_text_blob = true;
			}
		}
		else
			all_blob = false;
	}

	// If all the data types that we have seen are the same, then we are done
	if (all_equal)
	{
		f->fld_dtype = max_dtype;
		f->fld_length = max_length;
		f->fld_scale = max_scale;
		f->fld_sub_type = max_sub_type;
		return;
	}

	// If all of the expressions are of type text then use a text type.
	// Since Firebird allows most anything to be coverted to text, we
	// allow mixing numeric and dates/times with text.
	if (all_text || (any_text && (any_numeric || any_datetime)))
	{
		if (any_varying || (any_text && (any_numeric || any_datetime)))
			f->fld_dtype = dtype_cstring;
		else
			f->fld_dtype = dtype_text;

		f->fld_ttype = ttype;
		f->fld_length = maxtextlength;
		f->fld_scale = 0;
		if (gpreGlob.sw_cstring && (f->fld_dtype == dtype_cstring))
			f->fld_length++;
		else
			f->fld_dtype = dtype_text;
		return;
	}

	if (all_numeric)
	{
		if (any_approx)
		{
			if (max_significant_digits <= type_significant_bits[dtype_real])
			{
				f->fld_dtype = dtype_real;
				f->fld_length = type_lengths[f->fld_dtype];
			}
			else
			{
				f->fld_dtype = dtype_double;
				f->fld_length = type_lengths[f->fld_dtype];
			}
			if (any_float)
			{
				f->fld_scale = 0;
				f->fld_sub_type = 0;
			}
			else
			{
				f->fld_scale = max_scale;
				f->fld_sub_type = max_numeric_sub_type;
			}
		}
		else
		{
			f->fld_dtype = max_dtype;
			f->fld_length = max_dtype_length;
			f->fld_sub_type = max_numeric_sub_type;
			f->fld_scale = max_scale;
		}
		return;
	}

	if (all_date || all_time || all_timestamp)
	{
		f->fld_dtype = max_dtype;
		f->fld_length = max_dtype_length;
		f->fld_scale = 0;
		f->fld_sub_type = 0;
		return;
	}

	if (all_blob && all_same_sub_type)
	{
		f->fld_dtype = max_dtype;
		f->fld_sub_type = max_sub_type;
		if (max_sub_type == isc_blob_text)
			f->fld_scale = ttype;
		else
			f->fld_scale = max_scale;
		f->fld_length = max_length;
		return;
	}

	// We couldn't come up with a data type because the data types are incompatible.
	CPR_error("Incompatible data types");
	return;
}
