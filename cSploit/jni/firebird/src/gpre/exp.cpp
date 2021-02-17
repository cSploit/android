//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		exp.cpp
//		DESCRIPTION:	Expression parser
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
#include "../jrd/intl.h"
#include "../gpre/cmp_proto.h"
#include "../gpre/exp_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "../gpre/par_proto.h"
#include "../gpre/sqe_proto.h"
#include "../gpre/sql_proto.h"

const int ZERO_BASED	= 0;
const int ONE_BASED		= 1;

static bool check_relation();
static gpre_nod* lookup_field(gpre_ctx*);
static gpre_nod* make_and(gpre_nod*, gpre_nod*);
static gpre_nod* make_list(gpre_lls*);
static gpre_nod* normalize_index(dim*, gpre_nod*, USHORT);
static gpre_nod* par_and(gpre_req*);
static gpre_nod* par_array(gpre_req*, gpre_fld*, bool, bool);
static gpre_nod* par_boolean(gpre_req*);
static gpre_nod* par_field(gpre_req*);
static gpre_nod* par_multiply(gpre_req*, gpre_fld*);
static gpre_nod* par_native_value(gpre_req*, gpre_fld*);
static gpre_nod* par_not(gpre_req*);
static gpre_nod* par_over(gpre_ctx*);
static gpre_nod* par_primitive_value(gpre_req*, gpre_fld*);
static gpre_nod* par_relational(gpre_req*);
static gpre_nod* par_udf(gpre_req*, USHORT, gpre_fld*);
static gpre_nod* par_value(gpre_req*, gpre_fld*);

static gpre_fld* global_count_field;
static gpre_fld* global_subscript_field;

struct rel_ops
{
	nod_t rel_op;
	kwwords_t rel_kw;
	SSHORT rel_args;
};

static const rel_ops relops[] =
{
	{ nod_eq, KW_EQ, 2 },
	{ nod_eq, KW_EQUALS, 2 },
	{ nod_equiv, KW_EQUIV, 2 },
	{ nod_ne, KW_NE, 2 },
	{ nod_gt, KW_GT, 2 },
	{ nod_ge, KW_GE, 2 },
	{ nod_le, KW_LE, 2 },
	{ nod_lt, KW_LT, 2 },
	{ nod_containing, KW_CONTAINING, 2 },
	{ nod_matches, KW_MATCHES, 2 },
	{ nod_like, KW_LIKE, 2 },
	{ nod_starting, KW_STARTING, 2 },
	{ nod_missing, KW_MISSING, 1 },
	{ nod_between, KW_BETWEEN, 3},
	{ nod_any, KW_none, 0}
};

struct dtypes
{
	kwwords_t dtype_keyword;
	USHORT dtype_dtype;
};

static const dtypes data_types[] =
{
	{ KW_CHAR, dtype_text },
	{ KW_VARYING, dtype_varying },
	{ KW_STRING, dtype_cstring },
	{ KW_SHORT, dtype_short },
	{ KW_LONG, dtype_long },
	{ KW_QUAD, dtype_quad },
	{ KW_FLOAT, dtype_real },
	{ KW_DOUBLE, dtype_double },
	{ KW_DATE, dtype_date },
	{ KW_none, 0}
};


//____________________________________________________________
//
//		Parse array subscript.
//

gpre_nod* EXP_array(gpre_req* request, gpre_fld* field, bool subscript_flag, bool sql_flag)
{
	return par_array(request, field, subscript_flag, sql_flag);
}


//____________________________________________________________
//
//		Parse a datatype cast (sans leading period).
//

gpre_fld* EXP_cast(gpre_fld* field)
{
	const dtypes* dtype = data_types;
	while (true)
	{
		if (dtype->dtype_keyword == KW_none)
			return NULL;
		if (MSC_match(dtype->dtype_keyword))
			break;
		++dtype;
	}

	gpre_fld* cast = (gpre_fld*) MSC_alloc(FLD_LEN);
	cast->fld_symbol = field->fld_symbol;

	switch (cast->fld_dtype = dtype->dtype_dtype)
	{
	case dtype_varying:
		cast->fld_length++;
		// fall back

	case dtype_cstring:
		cast->fld_length++;
		// fall back

	case dtype_text:
		if (gpreGlob.sw_cstring && !(cast->fld_dtype == dtype_cstring))
		{
			cast->fld_length++;
			cast->fld_dtype = dtype_cstring;
		}
		if (!MSC_match(KW_L_BRCKET) && !MSC_match(KW_LT))
			CPR_s_error("left bracket or <");
		cast->fld_length += EXP_pos_USHORT_ordinal(true);
		if (!MSC_match(KW_R_BRCKET) && !MSC_match(KW_GT))
			CPR_s_error("right bracket or >");
		break;

	case dtype_quad:
		cast->fld_length = 4;
		break;

	// Begin date/time/timestamp
	case dtype_sql_date:
		cast->fld_length = sizeof(ISC_DATE);
		break;

	case dtype_sql_time:
		cast->fld_length = sizeof(ISC_TIME);
		break;

	case dtype_timestamp:
		cast->fld_length = sizeof(ISC_TIMESTAMP);
		break;
	// End date/time/timestamp

	case dtype_int64:
		cast->fld_length = sizeof(ISC_INT64);
		break;

	case dtype_long:
		cast->fld_length = sizeof(SLONG);
		if (MSC_match(KW_SCALE))
			cast->fld_scale = EXP_SSHORT_ordinal(true);
		break;

	case dtype_short:
		cast->fld_length = sizeof(SSHORT);
		if (MSC_match(KW_SCALE))
			cast->fld_scale = EXP_SSHORT_ordinal(true);
		break;

	case dtype_real:
		cast->fld_length = 4;
		break;

	case dtype_double:
		cast->fld_length = 8;
		break;

	}

	return cast;
}


//____________________________________________________________
//
//		Parse a clause of the form "<context> IN <relation>".
//		If the error flag is true, and the parse fails, quietly
//		return NULL.  Otherwise issue error messages where appropriate,
//		and return a CONTEXT block as value.
//

gpre_ctx* EXP_context(gpre_req* request, gpre_sym* initial_symbol)
{
	// Use the token (context name) to make up a symbol
	// block.  Then check for the keyword IN.  If it's
	// missing, either complain or punt, depending on the
	// error flag.  In either case, be sure to get rid of
	// the symbol.  If things look kosher, continue.

	gpre_sym* symbol = initial_symbol;
	if (!symbol)
	{
		symbol = PAR_symbol(SYM_context);
		if (!MSC_match(KW_IN))
		{
			MSC_free(symbol);
			CPR_s_error("IN");
		}
	}

	symbol->sym_type = SYM_context;

	gpre_rel* relation = EXP_relation();
	gpre_ctx* context = MSC_context(request);
	context->ctx_symbol = symbol;
	context->ctx_relation = relation;
	symbol->sym_object = context;

	return context;
}


//____________________________________________________________
//
//		Parse a qualified field clause.  If recognized,
//		return both the field block (as value) and the
//		context block (by reference).
//

gpre_fld* EXP_field(gpre_ctx** rcontext)
{
	gpre_sym* symbol;
	for (symbol = gpreGlob.token_global.tok_symbol; symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_context)
			break;
	}

	if (!symbol)
		CPR_s_error("context variable");

	gpre_ctx* context = symbol->sym_object;
	gpre_rel* relation = context->ctx_relation;
	PAR_get_token();

	if (!MSC_match(KW_DOT))
		CPR_s_error("dot after context variable");

	SQL_resolve_identifier("<Field Name>", NULL, NAME_SIZE);
	gpre_fld* field = MET_field(relation, gpreGlob.token_global.tok_string);
	if (!field)
	{
		TEXT s[ERROR_LENGTH];
		sprintf(s, "field \"%s\" is not defined in relation %s",
				gpreGlob.token_global.tok_string, relation->rel_symbol->sym_string);
		PAR_error(s);
	}

	PAR_get_token();
	*rcontext = context;

	return field;
}


//____________________________________________________________
//
//		Eat a left parenthesis, complain if not there.
//

void EXP_left_paren(const TEXT* string)
{
	if (!MSC_match(KW_LEFT_PAREN))
		CPR_s_error(string ? string : "left parenthesis");
}


//____________________________________________________________
//
//		Parse a native literal constant value.
//

gpre_nod* EXP_literal()
{
	switch (gpreGlob.sw_sql_dialect)
	{
	case 1:
		if (!(gpreGlob.token_global.tok_type == tok_number || isQuoted(gpreGlob.token_global.tok_type)))
			return NULL;
		break;
	default:
		if (!(gpreGlob.token_global.tok_type == tok_number || gpreGlob.token_global.tok_type == tok_sglquoted))
			return NULL;
	}

	ref* reference = (ref*) MSC_alloc(REF_LEN);
	gpre_nod* node = MSC_unary(nod_literal, (gpre_nod*) reference);
	if (isQuoted(gpreGlob.token_global.tok_type))
	{
	    TEXT* string = (TEXT *) MSC_alloc(gpreGlob.token_global.tok_length + 3);
		reference->ref_value = string;
		strcat(string, "\'");
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string + 1);
		strcat((string + gpreGlob.token_global.tok_length + 1), "\'");
		// What kind of hack is this? The token has not been enlarged nor modified.
		gpreGlob.token_global.tok_length += 2;
	}
	else
	{
		TEXT* string = (TEXT *) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		reference->ref_value = string;
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
	}

	// Begin date/time/timestamp
	switch (gpreGlob.token_global.tok_keyword)
	{
	case KW_DATE:
		reference->ref_flags |= REF_sql_date;
		break;

	case KW_TIME:
		reference->ref_flags |= REF_sql_time;
		break;

	case KW_TIMESTAMP:
		reference->ref_flags |= REF_timestamp;
		break;
		// Do not put a default here
	}
	// End date/time/timestamp

	if ((gpreGlob.token_global.tok_type == tok_sglquoted && gpreGlob.token_global.tok_charset) ||
		((isQuoted(gpreGlob.token_global.tok_type) && gpreGlob.sw_sql_dialect == 1) &&
		gpreGlob.token_global.tok_charset))
	{
		reference->ref_flags |= REF_ttype;
		gpre_sym* symbol = gpreGlob.token_global.tok_charset;
		reference->ref_ttype = ((intlsym*) (symbol->sym_object))->intlsym_ttype;
	}
	else if (gpreGlob.sw_language == lang_internal)
	{
		// literals referenced in an Internal request are always correct charset
		reference->ref_flags |= REF_ttype;
		reference->ref_ttype = ttype_metadata;
	}
	PAR_get_token();
	return node;
}


//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//		Restrict to LONG range.
//

SINT64 EXP_SINT64_ordinal(bool advance_flag)
{
	const bool negate = (MSC_match(KW_MINUS));

	if (gpreGlob.token_global.tok_type != tok_number)
		CPR_s_error("<number>");

	const char format[8] = "%"SQUADFORMAT;
	SINT64 n;
	sscanf(gpreGlob.token_global.tok_string, format, &n);

	char buffer[64];
	sprintf(buffer, format, n);
	if (strcmp(buffer, gpreGlob.token_global.tok_string) != 0)
		PAR_error("Numeric value out of range");

	if (advance_flag)
		PAR_get_token();

	return negate ? -n : n;
}

//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//		Restrict to LONG range.
//

SLONG EXP_SLONG_ordinal(bool advance_flag)
{
	const bool negate = (MSC_match(KW_MINUS));

	if (gpreGlob.token_global.tok_type != tok_number)
		CPR_s_error("<number>");

	const SLONG n = atoi(gpreGlob.token_global.tok_string);
	char buffer[32];
	sprintf(buffer, "%"SLONGFORMAT, n);
	if (strcmp(buffer, gpreGlob.token_global.tok_string) != 0)
		PAR_error("Numeric value out of range");

	if (advance_flag)
		PAR_get_token();

	return negate ? -n : n;
}


//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//		A SSHORT is desired.
//

SSHORT EXP_SSHORT_ordinal(bool advance_flag)
{
	const bool negate = (MSC_match(KW_MINUS));

	if (gpreGlob.token_global.tok_type != tok_number)
		CPR_s_error("<number>");

	const SLONG n = atoi(gpreGlob.token_global.tok_string);
	if (negate && n > -1L * MIN_SSHORT)
		PAR_error("Numeric value out of range");
	else if (!negate && n > (SLONG) MAX_SSHORT)
		PAR_error("Numeric value out of range");

	if (advance_flag)
		PAR_get_token();

	return (SSHORT) (negate ? -n : n);
}


//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//		Restrict to LONG range.
//

ULONG EXP_ULONG_ordinal(bool advance_flag)
{
	if (gpreGlob.token_global.tok_type != tok_number)
		CPR_s_error("<unsigned number>");

	const ULONG n = atoi(gpreGlob.token_global.tok_string);
	char buffer[32];
	sprintf(buffer, "%"ULONGFORMAT, n);
	if (strcmp(buffer, gpreGlob.token_global.tok_string) != 0)
		PAR_error("Numeric value out of range");

	if (advance_flag)
		PAR_get_token();

	return n;
}


//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//

USHORT EXP_USHORT_ordinal(bool advance_flag)
{
	if (gpreGlob.token_global.tok_type != tok_number)
		CPR_s_error("<unsigned number>");

	const ULONG n = atoi(gpreGlob.token_global.tok_string);
	if (n > MAX_USHORT)
		PAR_error("Numeric value out of range");

	if (advance_flag)
		PAR_get_token();

	return (USHORT) n;
}


//____________________________________________________________
//
//		Parse and convert to binary a numeric token.
//		Which must be non-zero.
//

USHORT EXP_pos_USHORT_ordinal(bool advance_flag)
{
	const USHORT n = EXP_USHORT_ordinal(advance_flag);
	if (n == 0)
		PAR_error("Expected positive value");

	return n;
}


//____________________________________________________________
//
//		We have a free reference to array.  Make sure the whole damn thing
//		gets sucked up.
//

void EXP_post_array( ref* reference)
{
	gpre_fld* field = reference->ref_field;

	if (!field->fld_array_info)
		return;

	reference->ref_flags |= REF_fetch_array;

	gpre_ctx* context = reference->ref_context;
	gpre_req* request = context->ctx_request;
	ref* array_reference = MSC_reference(&request->req_array_references);
	array_reference->ref_context = context;
	array_reference->ref_field = field;
	array_reference->ref_level = request->req_level;
	field->fld_array_info->ary_ident = CMP_next_ident();

	blb* blob = (blb*) MSC_alloc(BLB_LEN);
	blob->blb_symbol = field->fld_symbol;
	blob->blb_reference = reference;

	if (!(blob->blb_seg_length = field->fld_seg_length))
		blob->blb_seg_length = 512;

	blob->blb_request = request;
}


//____________________________________________________________
//
//		Post a field reference to a request.  This
//		can be called from either par_variable (free
//		standing field reference) or EXP\par_value
//		(cross request field reference).
//

ref* EXP_post_field(gpre_fld* field, gpre_ctx* context, bool null_flag)
{
	TEXT s[128];

	gpre_req* request = context->ctx_request;

	// If the reference is already posted, return the reference

	ref* reference;
	for (reference = request->req_references; reference; reference = reference->ref_next)
	{
		if (reference->ref_context == context)
		{
			gpre_fld* ref_field = reference->ref_field;
			if (ref_field == field ||
				(ref_field->fld_symbol == field->fld_symbol && ref_field->fld_array == field->fld_array))
			{
				if (!null_flag &&
					(ref_field->fld_dtype != field->fld_dtype ||
						ref_field->fld_length != field->fld_length ||
						ref_field->fld_scale != field->fld_scale))
				{
					if (reference->ref_flags & REF_null)
						reference->ref_field = field;
					else
					{
						sprintf(s, "field %s is inconsistently cast", field->fld_symbol->sym_string);
						PAR_error(s);
					}
				}
				if (request->req_level > reference->ref_level)
					reference->ref_level = request->req_level;

				if (!null_flag)
					reference->ref_flags &= ~REF_null;
				return reference;
			}
		}
	}

	// This is first occurrence of field, make a new reference

	reference = MSC_reference(&request->req_references);
	reference->ref_context = context;
	reference->ref_field = field;
	reference->ref_level = request->req_level;

	if (null_flag)
		reference->ref_flags |= REF_null;

	return reference;
}


//____________________________________________________________
//
//		Match a trailing parenthesis.  If isn't one, generate an error
//		and return false.
//

bool EXP_match_paren()
{
	if (MSC_match(KW_RIGHT_PAREN))
		return true;

	CPR_s_error("right parenthesis");
	return false;	// silence compiler warning
}


//____________________________________________________________
//
//		Parse and look up a qualified relation name.
//

gpre_rel* EXP_relation()
{
	if (!gpreGlob.isc_databases)
		PAR_error("no database for operation");

	// The current token is (i.e. should be) either a relation
	// name or a database name.  If it's a database name, search
	// it for the relation name.  If it's an unqualified relation
	// name, search all databases for the name

	gpre_rel* relation = NULL;

	SQL_resolve_identifier("<identifier>", NULL, NAME_SIZE);
	gpre_sym* symbol = MSC_find_symbol(gpreGlob.token_global.tok_symbol, SYM_database);
	if (symbol)
	{
		gpre_dbb* db = (gpre_dbb*) symbol->sym_object;
		PAR_get_token();
		if (!MSC_match(KW_DOT))
			CPR_s_error("period after database name");
		SQL_resolve_identifier("<Table name>", NULL, NAME_SIZE);
		relation = MET_get_relation(db, gpreGlob.token_global.tok_string, "");
	}
	else
	{
		for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
		{
		    gpre_rel* temp = MET_get_relation(db, gpreGlob.token_global.tok_string, "");
			if (temp)
			{
				if (!relation)
					relation = temp;
				else
				{
					TEXT s[ERROR_LENGTH];
					sprintf(s, "relation %s is ambiguous", gpreGlob.token_global.tok_string);
					PAR_get_token();
					PAR_error(s);
				}
			}
		}
	}

	if (!relation)
		CPR_s_error("relation name");

	PAR_get_token();

	return relation;
}


//____________________________________________________________
//
//		Parse a record selection expression.  If there is an
//		error, return NULL.  This is slightly complicated by
//		the fact that PASCAL and FORTRAN have a native FOR
//		statement, and ADA has a FOR <variable> IN statement.
//
//		If an initial symbol is given, the caller has already
//		parsed the <contect> IN part of the expression.
//

gpre_rse* EXP_rse(gpre_req* request, gpre_sym* initial_symbol)
{
	// parse FIRST n clause, if present

	gpre_nod* first = NULL;
	if (MSC_match(KW_FIRST))
	{
		if (!global_count_field)
			global_count_field = MET_make_field("jrd_count", dtype_long, 4, false);
		first = par_value(request, global_count_field);
	}

	// parse first context clause

	if (initial_symbol && gpreGlob.sw_language == lang_ada && !check_relation())
		return NULL;

	gpre_ctx* context = EXP_context(request, initial_symbol);
	SSHORT count = 1;

	// parse subsequent context clauses if this is a join
	gpre_nod* boolean = NULL;
	while (MSC_match(KW_CROSS))
	{
		context = EXP_context(request, 0);
		count++;
		if (MSC_match(KW_OVER))
			boolean = make_and(boolean, par_over(context));
	}

	// bug_3380 - could have an "over" clause without a "cross" clause
	if (MSC_match(KW_OVER))
		boolean = make_and(boolean, par_over(context));

	// build rse node

	gpre_rse* rec_expr = (gpre_rse*) MSC_alloc(RSE_LEN(count));
	rec_expr->rse_count = count;
	rec_expr->rse_first = first;
	rec_expr->rse_boolean = boolean;

	while (count)
	{
		rec_expr->rse_context[--count] = context;
		HSH_insert(context->ctx_symbol);
		context = context->ctx_next;
	}

	// parse boolean, if any.  If there is an error, ignore the
	// boolean, but keep the rse

	if (MSC_match(KW_WITH))
		boolean = make_and(boolean, par_boolean(request));

	rec_expr->rse_boolean = boolean;

	// Parse SORT clause, if any.

	// CVC: It's not clear whether this var should be initialized at the same
	// level than "direction".
	bool insensitive = false;

	while (true)
	{
		if (MSC_match(KW_SORTED))
		{
			MSC_match(KW_BY);
			gpre_lls* items = NULL;
			gpre_lls* directions = NULL;
			bool direction = false;
			count = 0;
			while (true)
			{
				if (MSC_match(KW_ASCENDING))
				{
					direction = false;
					continue;
				}
				if (MSC_match(KW_DESCENDING))
				{
					direction = true;
					continue;
				}
				if (MSC_match(KW_EXACTCASE))
				{
					insensitive = false;
					continue;
				}
				if (MSC_match(KW_ANYCASE))
				{
					insensitive = true;
					continue;
				}
				gpre_nod* item = par_value(request, 0);
				count++;
				MSC_push((gpre_nod*) (IPTR) (direction ? 1 : 0), &directions);
				if (insensitive)
				{
					gpre_nod* upcase = MSC_node(nod_upcase, 1);
					upcase->nod_arg[0] = item;
					MSC_push(upcase, &items);
				}
				else
					MSC_push(item, &items);
				if (!MSC_match(KW_COMMA))
					break;
			}
			gpre_nod* sort = MSC_node(nod_sort, (SSHORT) (count * 2));
			rec_expr->rse_sort = sort;
			sort->nod_count = count;
			gpre_nod** ptr = sort->nod_arg + count * 2;
			while (--count >= 0)
			{
				*--ptr = (gpre_nod*) MSC_pop(&items);
				*--ptr = (gpre_nod*) MSC_pop(&directions);
			}
		}
		else if (MSC_match(KW_REDUCED))
		{
			// Parse REDUCED clause, if any.
			MSC_match(KW_TO);
			gpre_lls* items = NULL;
			count = 0;
			while (true)
			{
				gpre_nod* item = par_value(request, 0);
				count++;
				MSC_push(item, &items);
				if (!MSC_match(KW_COMMA))
					break;
			}
			gpre_nod* sort = MSC_node(nod_projection, count);
			rec_expr->rse_reduced = sort;
			sort->nod_count = count;
			gpre_nod** ptr = sort->nod_arg + count;
			while (--count >= 0)
				*--ptr = (gpre_nod*) MSC_pop(&items);
		}
		else
			break;
	}

	return rec_expr;
}


//____________________________________________________________
//
//		Remove any context variables from hash table for a record
//		selection expression.
//

void EXP_rse_cleanup( gpre_rse* rs)
{
	// Clean up simple context variables

	const gpre_ctx* const* context = rs->rse_context;
	const gpre_ctx* const* const end = context + rs->rse_count;

	for (; context < end; context++)
	{
		if ((*context)->ctx_symbol)
			HSH_remove((*context)->ctx_symbol);
	}

	// If this is an aggregate, clean up the underlying rse

	if (rs->rse_aggregate)
		EXP_rse_cleanup(rs->rse_aggregate);

	// If this is a union, clean up each of the primitive rse's

	gpre_nod* node = rs->rse_union;
	if (node)
	{
		for (int i = 0; i < node->nod_count; i++)
			EXP_rse_cleanup((gpre_rse*) node->nod_arg[i]);
	}
}


//____________________________________________________________
//
//		Parse a subscript value.  This is called by PAR\par_slice.
//

gpre_nod* EXP_subscript(gpre_req* request)
{
	ref* reference = (ref*) MSC_alloc(REF_LEN);
	gpre_nod* node = MSC_unary(nod_value, (gpre_nod*) reference);

	// Special case literals

	if (gpreGlob.token_global.tok_type == tok_number)
	{
		node->nod_type = nod_literal;
		TEXT* string = (TEXT *) MSC_alloc(gpreGlob.token_global.tok_length + 1);
		reference->ref_value = string;
		MSC_copy(gpreGlob.token_global.tok_string, gpreGlob.token_global.tok_length, string);
		PAR_get_token();
		return node;
	}

	reference->ref_value = PAR_native_value(true, false);

	if (request)
	{
		reference->ref_next = request->req_values;
		request->req_values = reference;
	}

	return node;
}


//____________________________________________________________
//
//		Check current token for either a relation or database name.
//

static bool check_relation()
{
	// The current token is (i.e. should be) either a relation
	// name or a database name.  If it's a database name, search
	// it for the relation name.  If it's an unqualified relation
	// name, search all databases for the name

	gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (symbol && symbol->sym_type == SYM_database)
		return true;

	for (gpre_dbb* db = gpreGlob.isc_databases; db; db = db->dbb_next)
	{
		if (MET_get_relation(db, gpreGlob.token_global.tok_string, ""))
			return true;
	}

	return false;
}


//____________________________________________________________
//
//		Check to see if the current token is a field name corresponding
//		to a given context.  If so, return a field block (with reference
//		block) for field.
//

static gpre_nod* lookup_field(gpre_ctx* context)
{
	SQL_resolve_identifier("<Field Name>", NULL, NAME_SIZE);
	gpre_fld* field = MET_field(context->ctx_relation, gpreGlob.token_global.tok_string);
	if (!field)
		return NULL;

	ref* reference = (ref*) MSC_alloc(REF_LEN);
	reference->ref_field = field;
	reference->ref_context = context;

	return MSC_unary(nod_field, (gpre_nod*) reference);
}


//____________________________________________________________
//
//		Combine two (potention) conjuncts into a single, valid
//		boolean.  Either or both on the conjunctions may be NULL.
//		If both are null, return null.
//

static gpre_nod* make_and( gpre_nod* node1, gpre_nod* node2)
{
	if (!node1)
		return node2;

	if (!node2)
		return NULL;

	return MSC_binary(nod_and, node1, node2);
}


//____________________________________________________________
//
//		Make a generic variable length node from a stack.
//

static gpre_nod* make_list( gpre_lls* stack)
{
	USHORT count = 0;
	for (const gpre_lls* temp = stack; temp; temp = temp->lls_next)
		++count;

	gpre_nod* node = MSC_node(nod_list, count);

	for (gpre_nod** ptr = node->nod_arg + count; stack;)
		*--ptr = MSC_pop(&stack);

	return node;
}


//____________________________________________________________
//
//		"Normalize" the array index so that
//		the index used in the rse refers to
//		the same relative position in the
//		dimension in the database as it is
//		in the user's program.
//

static gpre_nod* normalize_index( dim* dimension, gpre_nod* user_index, USHORT array_base)
{
	TEXT string[33];
	bool negate = false;

	switch (array_base)
	{
	case ZERO_BASED:
		if (dimension->dim_lower < 0)
			negate = true;
		sprintf(string, "%d", abs(dimension->dim_lower));
		break;

	case ONE_BASED:
		if (dimension->dim_lower - 1 < 0)
			negate = true;
		sprintf(string, "%d", abs(dimension->dim_lower - 1));
		break;

	default:
		return user_index;
	}

	ref* reference = (ref*) MSC_alloc(REF_LEN);
	char* tmp = (TEXT *) MSC_alloc(strlen(string));
	reference->ref_value = tmp;
	strcpy(tmp, string);
	gpre_nod* adjustment_node = MSC_unary(nod_literal, (gpre_nod*) reference);

	gpre_nod* negate_node;
	if (negate)
		negate_node = MSC_unary(nod_negate, adjustment_node);
	else
		negate_node = adjustment_node;

	gpre_nod* index_node = MSC_binary(nod_plus, negate_node, user_index);

	return index_node;
}


//____________________________________________________________
//
//		Parse a boolean AND.
//

static gpre_nod* par_and( gpre_req* request)
{
	gpre_nod* expr1 = par_not(request);

	if (!MSC_match(KW_AND))
		return expr1;

	return MSC_binary(nod_and, expr1, par_and(request));
}


//____________________________________________________________
//
//		Parse a array element reference
//		(array name and subscript list)
//		in an gpre_rse.
//

static gpre_nod* par_array(gpre_req* request, gpre_fld* field, bool subscript_flag, bool sql_flag)
{
	bool paren = false;
	bool bracket = false;

	if (MSC_match(KW_LEFT_PAREN))
		paren = true;
	else if (MSC_match(KW_L_BRCKET))
		bracket = true;
	else if (!subscript_flag)
		CPR_s_error("Missing parenthesis or bracket for array reference.");

	gpre_nod* array_node =
		MSC_node(nod_array, (SSHORT) (field->fld_array_info->ary_dimension_count + 1));

	if (sql_flag && ((paren && MSC_match(KW_RIGHT_PAREN)) || (bracket && MSC_match(KW_R_BRCKET))))
	{
		return array_node;
	}

	int fortran_adjustment = array_node->nod_count;
	if (paren || bracket)
	{
		if (!global_subscript_field)
			global_subscript_field = MET_make_field("gds_array_subscript", dtype_long, 4, false);

		// Parse a commalist of subscripts and build a tree of index nodes

		int i = 1;
		for (dim* dimension = field->fld_array_info->ary_dimension;
			 dimension; dimension = dimension->dim_next, i++)
		{
		    gpre_nod* node;
			if (!sql_flag)
				node = par_value(request, global_subscript_field);
			else
			{
				node = SQE_value(request, false, NULL, NULL);
				// For all values referenced, post the subscript field
				SQE_post_field(node, global_subscript_field);
			}

			gpre_nod* index_node = MSC_unary(nod_index, node);

			// Languages which can't handle negative or non-positive bounds need to
			// be accomodated with normalization of the indices.

			switch (gpreGlob.sw_language)
			{
			case lang_c:
			case lang_cxx:
			case lang_internal:
				index_node->nod_arg[0] = normalize_index(dimension, index_node->nod_arg[0], ZERO_BASED);
				break;

			case lang_cobol:
				index_node->nod_arg[0] = normalize_index(dimension, index_node->nod_arg[0], ONE_BASED);
				break;
			}

			// Error checking of constants being out of range will be here in the future.

			// Good ole Fortran's column major order needs to be accomodated.

			if (gpreGlob.sw_language == lang_fortran)
				array_node->nod_arg[fortran_adjustment - i] = index_node;
			else
				array_node->nod_arg[i] = index_node;

			if (dimension->dim_next && !MSC_match(KW_COMMA))
				CPR_s_error("Adequate number of subscripts for this array reference.");
		}

		// Match the parenthesis or bracket

		if (paren && !MSC_match(KW_RIGHT_PAREN))
			CPR_s_error("Missing parenthesis for array reference.");
		else if (bracket && !MSC_match(KW_R_BRCKET))
			CPR_s_error("Missing right bracket for array reference.");
	}

	return array_node;
}


//____________________________________________________________
//
//		Parse a boolean expression.  Actually, just parse
//		an OR node or anything of lower precedence.
//

static gpre_nod* par_boolean( gpre_req* request)
{
	gpre_nod* expr1 = par_and(request);

	if (!MSC_match(KW_OR) && !MSC_match(KW_OR1))
		return expr1;

	return MSC_binary(nod_or, expr1, par_boolean(request));
}


//____________________________________________________________
//
//		Parse a field reference.  Anything else is an error.
//

static gpre_nod* par_field( gpre_req* request)
{
	const gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol)
		CPR_s_error("qualified field reference");

	bool upcase_flag = false;
	gpre_nod* prefix_node = 0;
	if (MSC_match(KW_UPPERCASE))
	{
		prefix_node = MSC_node(nod_upcase, 1);
		upcase_flag = true;
		if (!MSC_match(KW_LEFT_PAREN))
			CPR_s_error("left parenthesis");
		if (!(symbol = gpreGlob.token_global.tok_symbol))
			CPR_s_error("qualified field reference");
	}

	gpre_ctx* context = 0;
	gpre_nod* node = 0;
	gpre_fld* field = 0;
	if (symbol->sym_type == SYM_context)
	{
		field = EXP_field(&context);
		if (field->fld_array_info)
			node = par_array(request, field, false, false);

		if (MSC_match(KW_DOT))
		{
			gpre_fld* cast = EXP_cast(field);
			if (cast)
				field = cast;
		}
	}
	else
		CPR_s_error("qualified field reference");

	// There is a legit field reference.  If the reference is
	// to a field in this request, make up a reference block
	// and a field node, and return.

	if (!field->fld_array_info)
		node = MSC_node(nod_field, 1);

	if (upcase_flag)
		prefix_node->nod_arg[0] = node;

	if (context->ctx_request == request)
	{
		ref* reference = (ref*) MSC_alloc(REF_LEN);
		reference->ref_field = field;
		reference->ref_context = context;
		if (node->nod_type == nod_array)
			reference->ref_flags |= REF_array_elem;
		node->nod_arg[0] = (gpre_nod*) reference;
	}
	else
	{
		// Field wants to straddle two gpreGlob.requests.  We need to do
		// two things.  First, post a reference to the field to
		// the other request.  This is a variance on code found
		// in par_variable in par.cpp

		ref* reference = EXP_post_field(field, context, false);
		// Next, make a value reference for this request

		ref* value_reference = MSC_reference(&request->req_values);
		value_reference->ref_field = reference->ref_field;
		value_reference->ref_source = reference;

		node->nod_type = nod_value;
		node->nod_arg[0] = (gpre_nod*) value_reference;
	}

	if (upcase_flag)
	{
		if (!MSC_match(KW_RIGHT_PAREN))
			CPR_s_error("right parenthesis");
		return prefix_node;
	}

	return node;
}


//____________________________________________________________
//
//		Parse a value expression.  In specific, handle the lowest
//		precedence operator plus/minus.
//

static gpre_nod* par_multiply( gpre_req* request, gpre_fld* field)
{
	gpre_nod* node = par_primitive_value(request, field);

	while (true)
	{
		nod_t nod_type;
		if (MSC_match(KW_ASTERISK))
			nod_type = nod_times;
		else if (MSC_match(KW_SLASH))
			nod_type = nod_divide;
		else
			return node;

		node = MSC_binary(nod_type, node, par_primitive_value(request, field));
	}
}


//____________________________________________________________
//
//		Parse a native C value.
//

static gpre_nod* par_native_value( gpre_req* request, gpre_fld* field)
{
	TEXT s[64];

	// Special case literals

	if (gpreGlob.token_global.tok_type == tok_number ||
		gpreGlob.token_global.tok_type == tok_sglquoted ||
		(gpreGlob.token_global.tok_type == tok_dblquoted && gpreGlob.sw_sql_dialect == 1))
	{
		gpre_nod* anode = EXP_literal();
		return anode;
	}

	ref* reference = (ref*) MSC_alloc(REF_LEN);
	gpre_nod* node = MSC_unary(nod_value, (gpre_nod*) reference);

	// Handle general native value references.  Since these values will need
	// to be exported to the database system, make sure there is a reference
	// field.

	reference->ref_value = PAR_native_value(false, false);

	if (!field)
	{
		sprintf(s, "no reference field for %s", reference->ref_value);
		PAR_error(s);
	}

	reference->ref_next = request->req_values;
	request->req_values = reference;
	reference->ref_field = field;

	return node;
}


//____________________________________________________________
//
//		Parse either a boolean NOT or a boolean parenthetical.
//

static gpre_nod* par_not( gpre_req* request)
{
	if (MSC_match(KW_LEFT_PAREN))
	{
		gpre_nod* anode = par_boolean(request);
		EXP_match_paren();
		return anode;
	}

	gpre_nod* node = par_udf(request, UDF_boolean, 0);
	if (node)
		return node;

	if (!MSC_match(KW_NOT))
		return par_relational(request);

	return MSC_unary(nod_not, par_not(request));
}


//____________________________________________________________
//
//		Parse the substance of an OVER clause (but not the leading keyword).
//

static gpre_nod* par_over( gpre_ctx* context)
{
	TEXT s[64];

	gpre_nod* boolean = NULL;

	do {
		gpre_nod* field1 = lookup_field(context);
		if (!field1)
		{
			sprintf(s, "OVER field %s undefined", gpreGlob.token_global.tok_string);
			PAR_error(s);
		}
		gpre_nod* field2 = NULL;
		for (gpre_ctx* next = context->ctx_next; next; next = next->ctx_next)
		{
			if (field2 = lookup_field(next))
				break;
		}
		if (!field2)
		{
			sprintf(s, "OVER field %s undefined", gpreGlob.token_global.tok_string);
			PAR_error(s);
		}
		boolean = make_and(boolean, MSC_binary(nod_eq, field1, field2));
		PAR_get_token();
	} while (MSC_match(KW_COMMA));

	return boolean;
}


//____________________________________________________________
//
//		Parse a value expression.  In specific, handle the lowest
//		precedence operator plus/minus.
//

static gpre_nod* par_primitive_value( gpre_req* request, gpre_fld* field)
{
	if (MSC_match(KW_MINUS))
		return MSC_unary(nod_negate, par_primitive_value(request, field));

	if (MSC_match(KW_LEFT_PAREN))
	{
		gpre_nod* node = par_value(request, field);
		EXP_match_paren();
		return node;
	}

	if (MSC_match(KW_UPPERCASE))
	{
		gpre_nod* node = MSC_node(nod_upcase, 1);
		gpre_nod* sub = par_primitive_value(request, field);
		node->nod_arg[0] = sub;
		return node;
	}

	if (MSC_match(KW_USER_NAME))
		return MSC_node(nod_user_name, 0);

	if (MSC_match(KW_NULLIF))
	{
		gpre_nod* node = MSC_node(nod_nullif, 2);
		EXP_left_paren(0);
		node->nod_arg[0] = par_value(request, field);
		if (!MSC_match(KW_COMMA))
			CPR_s_error("<comma>");
		node->nod_arg[1] = par_value(request, field);
		EXP_match_paren();
		return node;
	}

	// Check for user defined functions

	gpre_nod* node = par_udf(request, UDF_value, field);
	if (node)
		return node;

	const gpre_sym* symbol = gpreGlob.token_global.tok_symbol;
	if (!symbol || symbol->sym_type != SYM_context)
		return par_native_value(request, field);

	return par_field(request);
}


//____________________________________________________________
//
//		Parse a relational expression.
//

static gpre_nod* par_relational( gpre_req* request)
{
	if (MSC_match(KW_ANY))
	{
		gpre_nod* expr = MSC_node(nod_any, 1);
		expr->nod_count = 0;
		expr->nod_arg[0] = (gpre_nod*) EXP_rse(request, 0);
		EXP_rse_cleanup((gpre_rse*) expr->nod_arg[0]);
		return expr;
	}

	if (MSC_match(KW_UNIQUE))
	{
		gpre_nod* expr = MSC_node(nod_unique, 1);
		expr->nod_count = 0;
		expr->nod_arg[0] = (gpre_nod*) EXP_rse(request, 0);
		EXP_rse_cleanup((gpre_rse*) expr->nod_arg[0]);
		return expr;
	}

	// That's right, three pointer dereferences to get to the reference
	// structure if there's a UDF.  V3 bug#531.  MaryAnn  12/4/89

	gpre_nod* expr1 = par_udf(request, UDF_value, 0);
	ref* reference;
	if (expr1)
		reference = (ref*) (expr1->nod_arg[0]->nod_arg[0]->nod_arg[0]);
	else
	{
		expr1 = par_field(request);
		reference = (ref*) expr1->nod_arg[0];
	}

	gpre_fld* field = reference->ref_field;

	// Check for any of the binary guys

	const bool negation = MSC_match(KW_NOT);

	const rel_ops* relop;
	for (relop = relops;; relop++)
	{
		if (relop->rel_kw == KW_none)
			CPR_s_error("relational operator");
		else if (MSC_match(relop->rel_kw))
			break;
	}

	gpre_nod* expr = NULL;
	gpre_nod* expr2 = NULL;
	switch (relop->rel_kw)
	{
	case KW_STARTING:
		MSC_match(KW_WITH);
		expr = MSC_node(relop->rel_op, relop->rel_args);
		break;
	case KW_MATCHES:
		expr2 = par_value(request, field);
		if (MSC_match(KW_USING))
			expr = MSC_node(nod_sleuth, 3);
		else
			expr = MSC_node(nod_matches, 2);
		break;
	default:
		expr = MSC_node(relop->rel_op, relop->rel_args);
	}

	expr->nod_arg[0] = expr1;

	switch (expr->nod_type)
	{
	case nod_missing:
		break;

	case nod_between:
		expr->nod_arg[1] = expr2 = par_value(request, field);
		MSC_match(KW_AND);
		expr->nod_arg[2] = par_value(request, field);
		break;

	case nod_matches:
		expr->nod_arg[1] = expr2;
		break;

	case nod_sleuth:
		expr->nod_arg[1] = expr2;
		expr->nod_arg[2] = par_value(request, field);
		break;

	default:
		expr->nod_arg[1] = expr2 = par_value(request, field);
	}

	if (expr2)
	{
		ref* ref1 = (ref*) expr1->nod_arg[0];
		ref* ref2 = (ref*) expr2->nod_arg[0];
		if (expr1->nod_type == nod_array && expr2->nod_type == nod_value)
			ref2->ref_flags |= ref1->ref_flags & REF_array_elem;
		else if (expr2->nod_type == nod_array && expr1->nod_type == nod_value)
			ref1->ref_flags |= ref2->ref_flags & REF_array_elem;
	}

	if (!negation)
		return expr;

	return MSC_unary(nod_not, expr);
}


//____________________________________________________________
//
//		Parse a user defined function.  If the current token isn't one,
//		return NULL.  Otherwise try to parse one.  If things go badly,
//		complain bitterly.
//

static gpre_nod* par_udf( gpre_req* request, USHORT type, gpre_fld* field)
{
	if (!request)
		return NULL;

	// Check for user defined functions

	for (gpre_sym* symbol = gpreGlob.token_global.tok_symbol; symbol; symbol = symbol->sym_homonym)
	{
		udf* new_udf;
		if (symbol->sym_type == SYM_udf && (new_udf = (udf*) symbol->sym_object) &&
			// request->req_database == new_udf->udf_database &&
			new_udf->udf_type == type)
		{
			gpre_nod* node = MSC_node(nod_udf, 2);
			node->nod_count = 1;
			node->nod_arg[1] = (gpre_nod*) new_udf;
			PAR_get_token();
			if (!MSC_match(KW_LEFT_PAREN))
				EXP_left_paren(0);
			gpre_lls* stack = NULL;
			for (;;)
			{
				MSC_push(par_value(request, field), &stack);
				if (!MSC_match(KW_COMMA))
					break;
			}
			EXP_match_paren();
			node->nod_arg[0] = make_list(stack);
			return node;
		}
	}

	return NULL;
}


//____________________________________________________________
//
//		Parse a value expression.  In specific, handle the lowest
//		precedence operator plus/minus.
//

static gpre_nod* par_value( gpre_req* request, gpre_fld* field)
{
	gpre_nod* node = par_multiply(request, field);

	while (true)
	{
		nod_t nod_type;
		if (MSC_match(KW_PLUS))
			nod_type = nod_plus;
		else if (MSC_match(KW_MINUS))
			nod_type = nod_minus;
		else
			return node;

		node = MSC_binary(nod_type, node, par_value(request, field));
	}
}

