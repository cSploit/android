/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		make.cpp
 *	DESCRIPTION:	Routines to make various blocks.
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 * 2001.11.21 Claudio Valderrama: Finally solved the mystery of DSQL
 * not recognizing when a UDF returns NULL. This fixes SF bug #484399.
 * See case nod_udf in MAKE_desc().
 * 2001.02.23 Claudio Valderrama: Fix SF bug #518350 with substring()
 * and text blobs containing charsets other than ASCII/NONE/BINARY.
 * 2002.07.30 Arno Brinkman:
 *   COALESCE, CASE support added
 *   procedure MAKE_desc_from_list added
 * 2003.01.25 Dmitry Yemanov: Fixed problem with concatenation which
 *   trashed RDB$FIELD_LENGTH in the system tables. This change may
 *   potentially interfere with the one made by Claudio one year ago.
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <ctype.h>
#include <string.h>
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../jrd/constants.h"
#include "../jrd/align.h"
#include "../dsql/errd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/misc_func.h"
#include "../dsql/utld_proto.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/ini.h"
#include "../jrd/dsc_proto.h"
#include "../common/cvt.h"
#include "../jrd/thread_proto.h"
#include "../jrd/why_proto.h"
#include "../common/config/config.h"
#include "../common/StatusArg.h"

using namespace Jrd;
using namespace Dsql;
using namespace Firebird;

// Firebird provides transparent conversion from string to date in
// contexts where it makes sense.  This macro checks a descriptor to
// see if it is something that *could* represent a date value
static inline bool could_be_date(const dsc& d)
{
	return DTYPE_IS_DATE(d.dsc_dtype) || (d.dsc_dtype <= dtype_any_text);
}


// One of d1, d2 is time, the other is date
static inline bool is_date_and_time(const dsc& d1, const dsc& d2)
{
	return ((d1.dsc_dtype == dtype_sql_time) && (d2.dsc_dtype == dtype_sql_date)) ||
	((d2.dsc_dtype == dtype_sql_time) && (d1.dsc_dtype == dtype_sql_date));
}

static void make_null(dsc* const desc);
static void make_parameter_names(dsql_par*, const dsql_nod*);


static const char* DB_KEY_NAME = "DB_KEY";


dsql_nod* MAKE_const_slong(SLONG value)
{
	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(), 1) dsql_nod;
	node->nod_type = nod_constant;
	node->nod_flags = 0;
	node->nod_desc.dsc_dtype = dtype_long;
	node->nod_desc.dsc_length = sizeof(SLONG);
	node->nod_desc.dsc_scale = 0;
	node->nod_desc.dsc_sub_type = 0;
	node->nod_desc.dsc_address = (UCHAR*) node->nod_arg;

	*((SLONG *) (node->nod_desc.dsc_address)) = value;
	//printf("make.cpp %p %d\n", node->nod_arg[0], value);

	return node;
}


/**

 	MAKE_constant

    @brief	Make a constant node.


    @param constant
    @param numeric_flag

 **/
dsql_nod* MAKE_constant(dsql_str* constant, dsql_constant_type numeric_flag)
{
	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(),
						(numeric_flag == CONSTANT_TIMESTAMP ||
						  numeric_flag == CONSTANT_SINT64) ? 2 : 1) dsql_nod;
	node->nod_type = nod_constant;

	switch (numeric_flag)
	{
/*	case CONSTANT_SLONG:
		node->nod_desc.dsc_dtype = dtype_long;
		node->nod_desc.dsc_length = sizeof(SLONG);
		node->nod_desc.dsc_scale = 0;
		node->nod_desc.dsc_sub_type = 0;
		node->nod_desc.dsc_address = (UCHAR*) node->nod_arg;
		*((SLONG *) (node->nod_desc.dsc_address)) = (SLONG) (IPTR) constant;
		break;
*/
	case CONSTANT_DOUBLE:
		DEV_BLKCHK(constant, dsql_type_str);

		// This is a numeric value which is transported to the engine as
		// a string.  The engine will convert it. Use dtype_double so that
		// the engine can distinguish it from an actual string.
		// Note: Due to the size of dsc_scale we are limited to numeric
		// constants of less than 256 bytes.

		node->nod_desc.dsc_dtype = dtype_double;
		// Scale has no use for double
		node->nod_desc.dsc_scale = static_cast<signed char>(constant->str_length);
		node->nod_desc.dsc_sub_type = 0;
		node->nod_desc.dsc_length = sizeof(double);
		node->nod_desc.dsc_address = (UCHAR*) constant->str_data;
		node->nod_desc.dsc_ttype() = ttype_ascii;
		node->nod_arg[0] = (dsql_nod*) constant;
		break;

	case CONSTANT_SINT64:
		{
			// We convert the string to an int64.  We treat the two adjacent
			// 32-bit words node->nod_arg[0] and node->nod_arg[1] as a
			// 64-bit integer: if we ever port to a platform which requires
			// 8-byte alignment of int64 data, we will have to force 8-byte
			// alignment of node->nod_arg, which is now only guaranteed
			// 4-byte alignment.    -- ChrisJ 1999-02-20

			node->nod_desc.dsc_dtype = dtype_int64;
			node->nod_desc.dsc_length = sizeof(SINT64);
			node->nod_desc.dsc_scale = 0;
			node->nod_desc.dsc_sub_type = 0;
			node->nod_desc.dsc_address = (UCHAR*) node->nod_arg;

			// Now convert the string to an int64.  We can omit testing for
			// overflow, because we would never have gotten here if yylex
			// hadn't recognized the string as a valid 64-bit integer value.
			// We *might* have "9223372936854775808", which works an an int64
			// only if preceded by a '-', but that issue is handled in GEN_expr,
			// and need not be addressed here.

			// Recent change to support hex numeric constants means the input
			// string now can be X8000000000000000, for example.
			// Hex constants coming through this code are guaranteed to be
			// valid - they start with X and contains only 0-9, A-F.
			// And, they will fit in a SINT64 without overflow.

			SINT64 value = 0;
			const UCHAR* p = reinterpret_cast<const UCHAR*>(constant->str_data);

			if (*p == 'X')
			{
				// oh no, a hex string!
				*p++; // skip the 'X' part.
				UCHAR byte = 0;
				bool nibble = ((strlen(constant->str_data) - 1) & 1);
				SSHORT c;

				// hex string is already upper-cased
				while (isdigit(*p) || ((*p >= 'A') && (*p <= 'F')))
				{
					// Now convert the character to a nibble
					if (*p >= 'A')
						c = (*p - 'A') + 10;
					else
						c = (*p - '0');

					if (nibble)
					{
						byte = (byte << 4) + (UCHAR) c;
						nibble = false;
						value = (value << 8) + byte;
					}
					else
					{
						byte = c;
						nibble = true;
					}

					*p++;
				}

				// if value is negative, then GEN_constant (from dsql/gen.cpp)
				// is going to want 2 nodes: nod_negate (to hold the minus)
				// and nod_constant as a child to hold the value.
				if (value < 0)
				{
					value = -value;
					*(SINT64*) node->nod_desc.dsc_address = value;
					dsql_nod* sub = node;
					node = MAKE_node(nod_negate, 1);
					node->nod_arg[0] = sub;
				}
				else
					*(SINT64*) node->nod_desc.dsc_address = value;
			} // hex constant
			else
			{
				// good old-fashioned base-10 number from SQLParse.cpp

				// We convert the string to an int64.  We treat the two adjacent
				// 32-bit words node->nod_arg[0] and node->nod_arg[1] as a
				// 64-bit integer: if we ever port to a platform which requires
				// 8-byte alignment of int64 data, we will have to force 8-byte
				// alignment of node->nod_arg, which is now only guaranteed
				// 4-byte alignment.    -- ChrisJ 1999-02-20

				while (isdigit(*p))
					value = 10 * value + (*(p++) - '0');

				if (*p++ == '.')
				{
					while (isdigit(*p))
					{
						value = 10 * value + (*p++ - '0');
						node->nod_desc.dsc_scale--;
					}
				}

				*(FB_UINT64*) (node->nod_desc.dsc_address) = value;
			}
		}
		break;

	case CONSTANT_DATE:
	case CONSTANT_TIME:
	case CONSTANT_TIMESTAMP:
		{
			// Setup the constant's descriptor

			switch (numeric_flag)
			{
			case CONSTANT_DATE:
				node->nod_desc.dsc_dtype = dtype_sql_date;
				break;
			case CONSTANT_TIME:
				node->nod_desc.dsc_dtype = dtype_sql_time;
				break;
			case CONSTANT_TIMESTAMP:
				node->nod_desc.dsc_dtype = dtype_timestamp;
				break;
			}
			node->nod_desc.dsc_sub_type = 0;
			node->nod_desc.dsc_scale = 0;
			node->nod_desc.dsc_length = type_lengths[node->nod_desc.dsc_dtype];
			node->nod_desc.dsc_address = (UCHAR*) node->nod_arg;

			// Set up a descriptor to point to the string

			dsc tmp;
			tmp.dsc_dtype = dtype_text;
			tmp.dsc_scale = 0;
			tmp.dsc_flags = 0;
			tmp.dsc_ttype() = ttype_ascii;
			tmp.dsc_length = static_cast<USHORT>(constant->str_length);
			tmp.dsc_address = (UCHAR*) constant->str_data;

			// Now invoke the string_to_date/time/timestamp routines

			CVT_move(&tmp, &node->nod_desc, ERRD_post);
			break;
		}

	default:
		fb_assert(numeric_flag == CONSTANT_STRING);
		DEV_BLKCHK(constant, dsql_type_str);

		node->nod_desc.dsc_dtype = dtype_text;
		node->nod_desc.dsc_sub_type = 0;
		node->nod_desc.dsc_scale = 0;
		node->nod_desc.dsc_length = static_cast<USHORT>(constant->str_length);
		node->nod_desc.dsc_address = (UCHAR*) constant->str_data;
		node->nod_desc.dsc_ttype() = ttype_dynamic;
		// carry a pointer to the constant to resolve character set in pass1
		node->nod_arg[0] = (dsql_nod*) constant;
		break;
	}

	return node;
}


/**

 	MAKE_str_constant

    @brief	Make a constant node when the
       character set ID is already known.


    @param constant
    @param character_set

 **/
dsql_nod* MAKE_str_constant(dsql_str* constant, SSHORT character_set)
{
	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(), 1) dsql_nod;
	node->nod_type = nod_constant;

	DEV_BLKCHK(constant, dsql_type_str);

	node->nod_desc.dsc_dtype = dtype_text;
	node->nod_desc.dsc_sub_type = 0;
	node->nod_desc.dsc_scale = 0;
	node->nod_desc.dsc_length = static_cast<USHORT>(constant->str_length);
	node->nod_desc.dsc_address = (UCHAR*) constant->str_data;
	node->nod_desc.dsc_ttype() = character_set;
	// carry a pointer to the constant to resolve character set in pass1
	node->nod_arg[0] = (dsql_nod*) constant;

	return node;
}


/**

 	MAKE_cstring

    @brief	Make a string node for a string whose
 	length is not known, but is null-terminated.


    @param str

 **/
dsql_str* MAKE_cstring(const char* str)
{

	return MAKE_string(str, strlen(str));
}


/**

 	MAKE_desc

    @brief	Make a descriptor from input node.
	This function can modify node->nod_flags to add NOD_COMP_DIALECT

    @param desc
    @param node
	@param null_replacement

 **/
void MAKE_desc(CompiledStatement* statement, dsc* desc, dsql_nod* node, dsql_nod* null_replacement)
{
	dsc desc1, desc2, desc3;
	USHORT dtype, dtype1, dtype2;
	dsql_map* map;
	dsql_ctx* context;
	dsql_rel* relation;
	dsql_fld* field;

	DEV_BLKCHK(node, dsql_type_nod);

	// If we already know the datatype, don't worry about anything.
	//
	// dimitr: But let's re-evaluate descriptors for expression arguments
	//		   (when a NULL replacement node is provided) to always
	//		   choose the correct resulting datatype. Example:
	//		   NULLIF(NULL, 0) => CHAR(1), but
	//		   1 + NULLIF(NULL, 0) => INT
	//		   This is required because of MAKE_desc() being called
	//		   from custom pass1 handlers for some node types and thus
	//		   causing an incorrect datatype (determined without
	//		   context) to be cached in nod_desc.

	if (node->nod_desc.dsc_dtype && !null_replacement)
	{
		*desc = node->nod_desc;
		return;
	}

	switch (node->nod_type)
	{
	case nod_constant:
	case nod_variable:
		*desc = node->nod_desc;
		return;

	case nod_agg_count:
// count2
//    case nod_agg_distinct:

		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		return;

	case nod_map:
		map = (dsql_map*) node->nod_arg[e_map_map];
		context = (dsql_ctx*) node->nod_arg[e_map_context];
		MAKE_desc(statement, desc, map->map_node, null_replacement);

		// ASF: We should mark nod_agg_count as nullable when it's in an outer join - CORE-2660.
		if (context->ctx_flags & CTX_outer_join)
			desc->dsc_flags |= DSC_nullable;

		return;

	case nod_agg_min:
	case nod_agg_max:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		desc->dsc_flags = DSC_nullable;
		return;

	case nod_agg_average:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		desc->dsc_flags = DSC_nullable;
		if (!DTYPE_IS_NUMERIC(desc->dsc_dtype) && !DTYPE_IS_TEXT(desc->dsc_dtype))
		{
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_agg_wrongarg) << Arg::Str("AVG"));
		}
		else if (DTYPE_IS_TEXT(desc->dsc_dtype)) {
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		return;

	case nod_agg_average2:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		desc->dsc_flags = DSC_nullable;
		dtype = desc->dsc_dtype;
		if (!DTYPE_IS_NUMERIC(dtype)) {
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_agg2_wrongarg) << Arg::Str("AVG"));
		}
		else if (DTYPE_IS_EXACT(dtype)) {
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof(SINT64);
			node->nod_flags |= NOD_COMP_DIALECT;
		}
		else {
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		return;

	case nod_agg_total:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		if (!DTYPE_IS_NUMERIC(desc->dsc_dtype) && !DTYPE_IS_TEXT(desc->dsc_dtype))
		{
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_agg_wrongarg) << Arg::Str("SUM"));
		}
		else if (desc->dsc_dtype == dtype_short) {
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof(SLONG);
		}
		else if (desc->dsc_dtype == dtype_int64) {
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		else if (DTYPE_IS_TEXT(desc->dsc_dtype)) {
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		desc->dsc_flags = DSC_nullable;
		return;

	case nod_agg_total2:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		dtype = desc->dsc_dtype;
		if (!DTYPE_IS_NUMERIC(dtype)) {
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_agg2_wrongarg) << Arg::Str("SUM"));
		}
		else if (DTYPE_IS_EXACT(dtype)) {
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof(SINT64);
			node->nod_flags |= NOD_COMP_DIALECT;
		}
		else {
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		desc->dsc_flags = DSC_nullable;
		return;

	case nod_agg_list:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);
		desc->makeBlob(desc->getBlobSubType(), desc->getTextType());
		desc->setNullable(true);
		return;

	case nod_concatenate:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);
		DSqlDataTypeUtil(statement).makeConcatenate(desc, &desc1, &desc2);
		return;

	case nod_derived_field:
		MAKE_desc(statement, desc, node->nod_arg[e_derived_field_value], null_replacement);
		return;

 	case nod_upcase:
 	case nod_lowcase:
 		MAKE_desc(statement, &desc1, node->nod_arg[0], null_replacement);
		if (desc1.dsc_dtype <= dtype_any_text || desc1.dsc_dtype == dtype_blob)
 		{
 			*desc = desc1;
 			return;
 		}

 		desc->dsc_dtype = dtype_varying;
 		desc->dsc_scale = 0;
		desc->dsc_ttype() = ttype_ascii;
		desc->dsc_length = sizeof(USHORT) + DSC_string_length(&desc1);
		desc->dsc_flags = desc1.dsc_flags & DSC_nullable;
		return;

	case nod_substr:
		MAKE_desc(statement, &desc1, node->nod_arg[0], null_replacement);
		MAKE_desc(statement, &desc2, node->nod_arg[1], null_replacement);
		MAKE_desc(statement, &desc3, node->nod_arg[2], null_replacement);
 		DSqlDataTypeUtil(statement).makeSubstr(desc, &desc1, &desc2, &desc3);
  		return;

    case nod_trim:
		MAKE_desc(statement, &desc1, node->nod_arg[e_trim_value], null_replacement);
		if (node->nod_arg[e_trim_characters])
			MAKE_desc(statement, &desc2, node->nod_arg[e_trim_characters], null_replacement);
		else
			desc2.dsc_flags = 0;

		if (desc1.dsc_dtype == dtype_blob)
		{
			*desc = desc1;
			desc->dsc_flags |= (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;
		}
		else if (desc1.dsc_dtype <= dtype_any_text)
		{
			*desc = desc1;
			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = sizeof(USHORT) + DSC_string_length(&desc1);
			desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;
		}
		else
		{
			desc->dsc_dtype = dtype_varying;
			desc->dsc_scale = 0;
			desc->dsc_ttype() = ttype_ascii;
			desc->dsc_length = sizeof(USHORT) + DSC_string_length(&desc1);
			desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;
		}

		return;

	case nod_cast:
		field = (dsql_fld*) node->nod_arg[e_cast_target];
		MAKE_desc_from_field(desc, field);
		MAKE_desc(statement, &desc1, node->nod_arg[e_cast_source], NULL);
		desc->dsc_flags = desc1.dsc_flags & DSC_nullable;
		return;

	case nod_simple_case:
		MAKE_desc_from_list(statement, &desc1, node->nod_arg[e_simple_case_results],
							null_replacement, "CASE");
		*desc = desc1;
		return;

	case nod_searched_case:
		MAKE_desc_from_list(statement, &desc1, node->nod_arg[e_searched_case_results],
							null_replacement, "CASE");
		*desc = desc1;
		return;

	case nod_coalesce:
		MAKE_desc_from_list(statement, &desc1, node->nod_arg[0],
							null_replacement, "COALESCE");
		*desc = desc1;
		return;

#ifdef DEV_BUILD
	case nod_collate:
		ERRD_bugcheck("Not expecting nod_collate in dsql/MAKE_desc");
		return;
#endif

	case nod_add:
	case nod_subtract:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL + NULL = NULL of INT
			make_null(desc);
			return;
		}

		dtype1 = desc1.dsc_dtype;
		if (dtype_int64 == dtype1 || DTYPE_IS_TEXT(dtype1))
			dtype1 = dtype_double;

		dtype2 = desc2.dsc_dtype;
		if (dtype_int64 == dtype2 || DTYPE_IS_TEXT(dtype2))
			dtype2 = dtype_double;

		dtype = MAX(dtype1, dtype2);

		if (DTYPE_IS_BLOB(dtype)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;

		switch (dtype)
		{
		case dtype_sql_time:
		case dtype_sql_date:
			// CVC: I don't see how this case can happen since dialect 1 doesn't accept DATE or TIME
			// Forbid <date/time> +- <string>
			if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
			{
				ERRD_post(Arg::Gds(isc_expression_eval_err) <<
							Arg::Gds(isc_dsql_nodateortime_pm_string));
			}

		case dtype_timestamp:

			// Allow <timestamp> +- <string> (historical)
			if (could_be_date(desc1) && could_be_date(desc2))
			{
				if (node->nod_type == nod_subtract)
				{
					// <any date> - <any date>

					// Legal permutations are:
					// <timestamp> - <timestamp>
					// <timestamp> - <date>
					// <date> - <date>
					// <date> - <timestamp>
					// <time> - <time>
					// <timestamp> - <string>
					// <string> - <timestamp>
					// <string> - <string>

					if (DTYPE_IS_TEXT(desc1.dsc_dtype))
						dtype = dtype_timestamp;
					else if (DTYPE_IS_TEXT(desc2.dsc_dtype))
						dtype = dtype_timestamp;
					else if (desc1.dsc_dtype == desc2.dsc_dtype)
						dtype = desc1.dsc_dtype;
					else if ((desc1.dsc_dtype == dtype_timestamp) && (desc2.dsc_dtype == dtype_sql_date))
					{
						dtype = dtype_timestamp;
					}
					else if ((desc2.dsc_dtype == dtype_timestamp) && (desc1.dsc_dtype == dtype_sql_date))
					{
						dtype = dtype_timestamp;
					}
					else {
						ERRD_post(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_dsql_invalid_datetime_subtract));
					}

					if (dtype == dtype_sql_date) {
						desc->dsc_dtype = dtype_long;
						desc->dsc_length = sizeof(SLONG);
						desc->dsc_scale = 0;
					}
					else if (dtype == dtype_sql_time) {
						desc->dsc_dtype = dtype_long;
						desc->dsc_length = sizeof(SLONG);
						desc->dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
						desc->dsc_sub_type = dsc_num_type_numeric;
					}
					else {
						fb_assert(dtype == dtype_timestamp);
						desc->dsc_dtype = dtype_double;
						desc->dsc_length = sizeof(double);
						desc->dsc_scale = 0;
					}
				}
				else if (is_date_and_time(desc1, desc2)) {
					// <date> + <time>
					// <time> + <date>
					desc->dsc_dtype = dtype_timestamp;
					desc->dsc_length = type_lengths[dtype_timestamp];
					desc->dsc_scale = 0;
				}
				else {
					// <date> + <date>
					// <time> + <time>
					// CVC: Hard to see it, since we are in dialect 1.
					ERRD_post(Arg::Gds(isc_expression_eval_err) <<
								Arg::Gds(isc_dsql_invalid_dateortime_add));
				}
			}
			else if (DTYPE_IS_DATE(desc1.dsc_dtype) || (node->nod_type == nod_add))
			{
				// <date> +/- <non-date>
				// <non-date> + <date>
				desc->dsc_dtype = desc1.dsc_dtype;
				if (!DTYPE_IS_DATE(desc->dsc_dtype))
					desc->dsc_dtype = desc2.dsc_dtype;
				fb_assert(DTYPE_IS_DATE(desc->dsc_dtype));
				desc->dsc_length = type_lengths[desc->dsc_dtype];
				desc->dsc_scale = 0;
			}
			else {
				// <non-date> - <date>
				fb_assert(node->nod_type == nod_subtract);
				ERRD_post(Arg::Gds(isc_expression_eval_err) <<
							Arg::Gds(isc_dsql_invalid_type_minus_date));
			}
			return;

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
		case dtype_double:
		case dtype_real:
			desc->dsc_dtype = dtype_double;
			desc->dsc_sub_type = 0;
			desc->dsc_scale = 0;
			desc->dsc_length = sizeof(double);
			return;

		default:
			desc->dsc_dtype = dtype_long;
			desc->dsc_sub_type = 0;
			desc->dsc_length = sizeof(SLONG);
			desc->dsc_scale = MIN(NUMERIC_SCALE(desc1), NUMERIC_SCALE(desc2));
			break;
		}
		return;

	case nod_add2:
	case nod_subtract2:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL + NULL = NULL of INT
			make_null(desc);
			return;
		}

		dtype1 = desc1.dsc_dtype;
		dtype2 = desc2.dsc_dtype;

		// Arrays and blobs can never partipate in addition/subtraction
		if (DTYPE_IS_BLOB(dtype1) || DTYPE_IS_BLOB(dtype2)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		// In Dialect 2 or 3, strings can never partipate in addition / sub
		// (use a specific cast instead)
		if (DTYPE_IS_TEXT(dtype1) || DTYPE_IS_TEXT(dtype2))
		{
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_nostring_addsub_dial3));
		}

		// Determine the TYPE of arithmetic to perform, store it
		// in dtype.  Note:  this is different from the result of
		// the operation, as <timestamp>-<timestamp> uses
		// <timestamp> arithmetic, but returns a <double>
		if (DTYPE_IS_EXACT(dtype1) && DTYPE_IS_EXACT(dtype2))
		{
			dtype = dtype_int64;
		}
		else if (DTYPE_IS_NUMERIC(dtype1) && DTYPE_IS_NUMERIC(dtype2))
		{
			fb_assert(DTYPE_IS_APPROX(dtype1) || DTYPE_IS_APPROX(dtype2));
			dtype = dtype_double;
		}
		else {
			// mixed numeric and non-numeric:

			// The MAX(dtype) rule doesn't apply with dtype_int64

			if (dtype_int64 == dtype1)
				dtype1 = dtype_double;
			if (dtype_int64 == dtype2)
				dtype2 = dtype_double;

			dtype = MAX(dtype1, dtype2);
		}

		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;

		switch (dtype)
		{
		case dtype_sql_time:
		case dtype_sql_date:
		case dtype_timestamp:

			if ((DTYPE_IS_DATE(dtype1) || (dtype1 == dtype_unknown)) &&
				(DTYPE_IS_DATE(dtype2) || (dtype2 == dtype_unknown)))
			{
				if (node->nod_type == nod_subtract2)
				{
					// <any date> - <any date>
					// Legal permutations are:
					// <timestamp> - <timestamp>
					// <timestamp> - <date>
					// <date> - <date>
					// <date> - <timestamp>
					// <time> - <time>

					if (dtype1 == dtype2)
						dtype = dtype1;
					else if (dtype1 == dtype_timestamp && dtype2 == dtype_sql_date)
						dtype = dtype_timestamp;
					else if (dtype2 == dtype_timestamp && dtype1 == dtype_sql_date)
						dtype = dtype_timestamp;
					else
						ERRD_post(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_dsql_invalid_datetime_subtract));

					if (dtype == dtype_sql_date)
					{
						desc->dsc_dtype = dtype_long;
						desc->dsc_length = sizeof(SLONG);
						desc->dsc_scale = 0;
					}
					else if (dtype == dtype_sql_time)
					{
						desc->dsc_dtype = dtype_long;
						desc->dsc_length = sizeof(SLONG);
						desc->dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
						desc->dsc_sub_type = dsc_num_type_numeric;
					}
					else
					{
						fb_assert(dtype == dtype_timestamp);
						desc->dsc_dtype = dtype_int64;
						desc->dsc_length = sizeof(SINT64);
						desc->dsc_scale = -9;
						desc->dsc_sub_type = dsc_num_type_numeric;
					}
				}
				else if (is_date_and_time(desc1, desc2))
				{
					// <date> + <time>
					// <time> + <date>
					desc->dsc_dtype = dtype_timestamp;
					desc->dsc_length = type_lengths[dtype_timestamp];
					desc->dsc_scale = 0;
				}
				else
				{
					// <date> + <date>
					// <time> + <time>
					ERRD_post(Arg::Gds(isc_expression_eval_err) <<
								Arg::Gds(isc_dsql_invalid_dateortime_add));
				}
			}
			else if (DTYPE_IS_DATE(desc1.dsc_dtype) || (node->nod_type == nod_add2))
			{
				// <date> +/- <non-date>
				// <non-date> + <date>
				desc->dsc_dtype = desc1.dsc_dtype;
				if (!DTYPE_IS_DATE(desc->dsc_dtype))
					desc->dsc_dtype = desc2.dsc_dtype;
				fb_assert(DTYPE_IS_DATE(desc->dsc_dtype));
				desc->dsc_length = type_lengths[desc->dsc_dtype];
				desc->dsc_scale = 0;
			}
			else
			{
				// <non-date> - <date>
				fb_assert(node->nod_type == nod_subtract2);
				ERRD_post(Arg::Gds(isc_expression_eval_err) <<
							Arg::Gds(isc_dsql_invalid_type_minus_date));
			}
			return;

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
		case dtype_double:
		case dtype_real:
			desc->dsc_dtype = dtype_double;
			desc->dsc_sub_type = 0;
			desc->dsc_scale = 0;
			desc->dsc_length = sizeof(double);
			return;

		case dtype_short:
		case dtype_long:
		case dtype_int64:
			desc->dsc_dtype = dtype_int64;
			desc->dsc_sub_type = 0;
			desc->dsc_length = sizeof(SINT64);

			// The result type is int64 because both operands are
			// exact numeric: hence we don't need the NUMERIC_SCALE
			// macro here.
			fb_assert(desc1.dsc_dtype == dtype_unknown || DTYPE_IS_EXACT(desc1.dsc_dtype));
			fb_assert(desc2.dsc_dtype == dtype_unknown || DTYPE_IS_EXACT(desc2.dsc_dtype));

			desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
			node->nod_flags |= NOD_COMP_DIALECT;
			break;

		default:
			// a type which cannot participate in an add or subtract
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_invalid_type_addsub_dial3));
		}
		return;

	case nod_multiply:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL * NULL = NULL of INT
			make_null(desc);
			return;
		}

		// Arrays and blobs can never partipate in multiplication
		if (DTYPE_IS_BLOB(desc1.dsc_dtype) || DTYPE_IS_BLOB(desc2.dsc_dtype)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		dtype = DSC_multiply_blr4_result[desc1.dsc_dtype][desc2.dsc_dtype];
		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;

		switch (dtype)
		{
		case dtype_double:
			desc->dsc_dtype = dtype_double;
			desc->dsc_sub_type = 0;
			desc->dsc_scale = 0;
			desc->dsc_length = sizeof(double);
			break;

		case dtype_long:
			desc->dsc_dtype = dtype_long;
			desc->dsc_sub_type = 0;
			desc->dsc_length = sizeof(SLONG);
			desc->dsc_scale = NUMERIC_SCALE(desc1) + NUMERIC_SCALE(desc2);
			break;

		default:
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_invalid_type_multip_dial1));
		}
		return;

	case nod_multiply2:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL * NULL = NULL of INT
			make_null(desc);
			return;
		}

		// In Dialect 2 or 3, strings can never partipate in multiplication
		// (use a specific cast instead)
		if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
		{
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_nostring_multip_dial3));
		}

		// Arrays and blobs can never partipate in multiplication
		if (DTYPE_IS_BLOB(desc1.dsc_dtype) || DTYPE_IS_BLOB(desc2.dsc_dtype)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		dtype = DSC_multiply_result[desc1.dsc_dtype][desc2.dsc_dtype];
		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;

		switch (dtype)
		{
		case dtype_double:
			desc->dsc_dtype = dtype_double;
			desc->dsc_sub_type = 0;
			desc->dsc_scale = 0;
			desc->dsc_length = sizeof(double);
			break;

		case dtype_int64:
			desc->dsc_dtype = dtype_int64;
			desc->dsc_sub_type = 0;
			desc->dsc_length = sizeof(SINT64);
			desc->dsc_scale = NUMERIC_SCALE(desc1) + NUMERIC_SCALE(desc2);
			node->nod_flags |= NOD_COMP_DIALECT;
			break;

		default:
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_invalid_type_multip_dial3));
		}
		return;

	/*
	case nod_count:
		desc->dsc_dtype = dtype_long;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_flags = 0;
		return;
	*/

	case nod_divide:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL / NULL = NULL of INT
			make_null(desc);
			return;
		}

		// Arrays and blobs can never partipate in division
		if (DTYPE_IS_BLOB(desc1.dsc_dtype) || DTYPE_IS_BLOB(desc2.dsc_dtype)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		dtype1 = desc1.dsc_dtype;
		if (dtype_int64 == dtype1 || DTYPE_IS_TEXT(dtype1))
			dtype1 = dtype_double;

		dtype2 = desc2.dsc_dtype;
		if (dtype_int64 == dtype2 || DTYPE_IS_TEXT(dtype2))
			dtype2 = dtype_double;

		dtype = MAX(dtype1, dtype2);

		if (!DTYPE_IS_NUMERIC(dtype)) {
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_mustuse_numeric_div_dial1));
		}

		desc->dsc_dtype = dtype_double;
		desc->dsc_length = sizeof(double);
		desc->dsc_scale = 0;
		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;
		return;

	case nod_divide2:
		MAKE_desc(statement, &desc1, node->nod_arg[0], node->nod_arg[1]);
		MAKE_desc(statement, &desc2, node->nod_arg[1], node->nod_arg[0]);

		if (node->nod_arg[0]->nod_type == nod_null && node->nod_arg[1]->nod_type == nod_null)
		{
			// NULL / NULL = NULL of INT
			make_null(desc);
			return;
		}

		// In Dialect 2 or 3, strings can never partipate in division
		// (use a specific cast instead)
		if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
		{
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_nostring_div_dial3));
		}

		// Arrays and blobs can never partipate in division
		if (DTYPE_IS_BLOB(desc1.dsc_dtype) || DTYPE_IS_BLOB(desc2.dsc_dtype)) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}

		dtype = DSC_multiply_result[desc1.dsc_dtype][desc2.dsc_dtype];
		desc->dsc_dtype = static_cast<UCHAR>(dtype);
		desc->dsc_flags = (desc1.dsc_flags | desc2.dsc_flags) & DSC_nullable;

		switch (dtype)
		{
		case dtype_int64:
			desc->dsc_length = sizeof(SINT64);
			desc->dsc_scale = NUMERIC_SCALE(desc1) + NUMERIC_SCALE(desc2);
			node->nod_flags |= NOD_COMP_DIALECT;
			break;

		case dtype_double:
			desc->dsc_length = sizeof(double);
			desc->dsc_scale = 0;
			break;

		default:
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_invalid_type_div_dial3));
		}

		return;

	case nod_negate:
		MAKE_desc(statement, desc, node->nod_arg[0], null_replacement);

		if (node->nod_arg[0]->nod_type == nod_null)
		{
			// -NULL = NULL of INT
			make_null(desc);
			return;
		}

		// In Dialect 2 or 3, a string can never partipate in negation
		// (use a specific cast instead)
		if (DTYPE_IS_TEXT(desc->dsc_dtype)) {
			if (statement->req_client_dialect >= SQL_DIALECT_V6_TRANSITION) {
				ERRD_post(Arg::Gds(isc_expression_eval_err) <<
							Arg::Gds(isc_dsql_nostring_neg_dial3));
			}
			desc->dsc_dtype = dtype_double;
			desc->dsc_length = sizeof(double);
		}
		// Forbid blobs and arrays
		else if (DTYPE_IS_BLOB(desc->dsc_dtype))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_no_blob_array));
		}
		// Forbid other not numeric datatypes
		else if (!DTYPE_IS_NUMERIC(desc->dsc_dtype)) {
			ERRD_post(Arg::Gds(isc_expression_eval_err) <<
						Arg::Gds(isc_dsql_invalid_type_neg));
		}
		return;

	case nod_alias:
		MAKE_desc(statement, desc, node->nod_arg[e_alias_value], null_replacement);
		return;

	case nod_dbkey:
		// Fix for bug 10072 check that the target is a relation
		context = (dsql_ctx*) node->nod_arg[0]->nod_arg[0];
		relation = context->ctx_relation;
		if (relation != 0)
		{
			desc->dsc_dtype = dtype_text;
			if (relation->rel_flags & REL_creating)
				desc->dsc_length = 8;
			else
				desc->dsc_length = relation->rel_dbkey_length;
			desc->dsc_flags = DSC_nullable;
			desc->dsc_ttype() = ttype_binary;
		}
		else {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_dbkey_from_non_table));
		}
		return;

	case nod_udf:
		{
			const dsql_udf* userFunc = (dsql_udf*) node->nod_arg[0];
			desc->dsc_dtype = static_cast<UCHAR>(userFunc->udf_dtype);
			desc->dsc_length = userFunc->udf_length;
			desc->dsc_scale = static_cast<SCHAR>(userFunc->udf_scale);
			// CVC: Setting flags to zero obviously impeded DSQL to acknowledge
			// the fact that any UDF can return NULL simply returning a NULL
			// pointer.
			desc->dsc_flags = DSC_nullable;

			if (desc->dsc_dtype <= dtype_any_text) {
				desc->dsc_ttype() = userFunc->udf_character_set_id;
			}
			else {
				desc->dsc_ttype() = userFunc->udf_sub_type;
			}
			return;
		}

	case nod_sys_function:
		{
			dsql_nod* nodeArgs = node->nod_arg[e_sysfunc_args];
			Firebird::Array<const dsc*> args;

			if (nodeArgs)
			{
				fb_assert(nodeArgs->nod_type == nod_list);

				for (dsql_nod** p = nodeArgs->nod_arg;
					p < nodeArgs->nod_arg + nodeArgs->nod_count; ++p)
				{
					MAKE_desc(statement, &(*p)->nod_desc, *p, NULL);
					args.add(&(*p)->nod_desc);
				}
			}

			const dsql_str* name = (dsql_str*) node->nod_arg[e_sysfunc_name];
			DSqlDataTypeUtil(statement).makeSysFunction(desc, name->str_data, args.getCount(), args.begin());

			return;
		}

	case nod_gen_id:
		MAKE_desc(statement, &desc1, node->nod_arg[e_gen_id_value], NULL);
		desc->dsc_dtype = dtype_long;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_flags = (desc1.dsc_flags & DSC_nullable);
		return;

	case nod_gen_id2:
		MAKE_desc(statement, &desc1, node->nod_arg[e_gen_id_value], NULL);
		desc->dsc_dtype = dtype_int64;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_length = sizeof(SINT64);
		desc->dsc_flags = (desc1.dsc_flags & DSC_nullable);
		node->nod_flags |= NOD_COMP_DIALECT;
		return;

	case nod_limit:
	case nod_rows:
		if (statement->req_client_dialect <= SQL_DIALECT_V5) {
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof (SLONG);
		}
		else {
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof (SINT64);
		}
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0; // Can first/skip accept NULL in the future?
		return;

	case nod_field:
		if (node->nod_desc.dsc_dtype)
		{
			*desc = node->nod_desc;
		}
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-203) <<
					  Arg::Gds(isc_dsql_field_ref));
		}
		return;

	case nod_user_name:
	case nod_current_role:
		desc->dsc_dtype = dtype_varying;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		desc->dsc_ttype() = ttype_metadata;
		desc->dsc_length =
			USERNAME_LENGTH * METD_get_charset_bpc(statement, ttype_metadata) + sizeof(USHORT);
		return;

	case nod_internal_info:
		{
			const internal_info_id id =
				*reinterpret_cast<internal_info_id*>(node->nod_arg[0]->nod_desc.dsc_address);

			if (id == internal_sqlstate)
			{
				desc->makeText(FB_SQLSTATE_LENGTH, ttype_ascii);
			}
			else
			{
				desc->makeLong(0);
			}
		}
		return;

	case nod_current_time:
		desc->dsc_dtype = dtype_sql_time;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		return;

	case nod_current_date:
		desc->dsc_dtype = dtype_sql_date;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		return;

	case nod_current_timestamp:
		desc->dsc_dtype = dtype_timestamp;
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		return;

	case nod_extract:
		MAKE_desc(statement, &desc1, node->nod_arg[e_extract_value], NULL);

		switch (node->nod_arg[e_extract_part]->getSlong())
		{
			case blr_extract_second:
				// QUADDATE - maybe this should be DECIMAL(6,4)
				desc->makeLong(ISC_TIME_SECONDS_PRECISION_SCALE);
				break;

			case blr_extract_millisecond:
				desc->makeLong(ISC_TIME_SECONDS_PRECISION_SCALE + 3);
				break;

			default:
				desc->makeShort(0);
				break;
		}

		desc->setNullable(desc1.isNullable());
		return;

	case nod_strlen:
		MAKE_desc(statement, &desc1, node->nod_arg[e_strlen_value], NULL);
		desc->dsc_sub_type = 0;
		desc->dsc_scale = 0;
		desc->dsc_flags = (desc1.dsc_flags & DSC_nullable);
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(ULONG);
		return;

	case nod_parameter:
		// We don't actually know the datatype of a parameter -
		// we have to guess it based on the context that the
		// parameter appears in. (This is done is pass1.c::set_parameter_type())
		// However, a parameter can appear as part of an expression.
		// As MAKE_desc is used for both determination of parameter
		// types and for expression type checking, we just continue.

		if (node->nod_desc.dsc_dtype)
		{
			*desc = node->nod_desc;
		}
		return;

	case nod_null:
		// This occurs when SQL statement specifies a literal NULL, eg:
		//  SELECT NULL FROM TABLE1;
		// As we don't have a <dtype_null, SQL_NULL> datatype pairing,
		// we don't know how to map this NULL to a host-language
		// datatype.  Therefore we now describe it as a
		// CHAR(1) CHARACTER SET NONE type.
		// No value will ever be sent back, as the value of the select
		// will be NULL - this is only for purposes of DESCRIBING
		// the statement.  Note that this mapping could be done in dsql.cpp
		// as part of the DESCRIBE statement - but I suspect other areas
		// of the code would break if this is declared dtype_unknown.

		if (null_replacement)
		{
			MAKE_desc(statement, desc, null_replacement, NULL);
			desc->dsc_flags |= (DSC_nullable | DSC_null);
		}
		else
			desc->makeNullString();
		return;

	case nod_via:
		MAKE_desc(statement, desc, node->nod_arg[e_via_value_1], null_replacement);
	    // Set the descriptor flag as nullable. The
	    // select expression may or may not return
	    // this row based on the WHERE clause. Setting this
	    // flag warns the client to expect null values.
	    // (bug 10379)

		desc->dsc_flags |= DSC_nullable;
		return;

	case nod_hidden_var:
		MAKE_desc(statement, desc, node->nod_arg[e_hidden_var_expr], null_replacement);
		return;

	default:
		fb_assert(false);			// unexpected dsql_nod type

	case nod_dom_value:		// computed value not used
		// By the time we get here, any nod_dom_value node should have had
		// its descriptor set to the type of the domain being created, or
		// to the type of the existing domain to which a CHECK constraint
		// is being added.

		fb_assert(node->nod_desc.dsc_dtype != dtype_unknown);
		if (desc != &node->nod_desc)
			*desc = node->nod_desc;
		return;
	}
}


/**

 	MAKE_desc_from_field

    @brief	Compute a DSC from a field's description information.


    @param desc
    @param field

 **/
void MAKE_desc_from_field(dsc* desc, const dsql_fld* field)
{

	DEV_BLKCHK(field, dsql_type_fld);

	desc->dsc_dtype = static_cast<UCHAR>(field->fld_dtype);
	desc->dsc_scale = static_cast<SCHAR>(field->fld_scale);
	desc->dsc_sub_type = field->fld_sub_type;
	desc->dsc_length = field->fld_length;
	desc->dsc_flags = (field->fld_flags & FLD_nullable) ? DSC_nullable : 0;
	if (desc->dsc_dtype <= dtype_any_text) {
		INTL_ASSIGN_DSC(desc, field->fld_character_set_id, field->fld_collation_id);
	}
	else if (desc->dsc_dtype == dtype_blob)
	{
		desc->dsc_scale = static_cast<SCHAR>(field->fld_character_set_id);
		desc->dsc_flags |= field->fld_collation_id << 8;
	}
}


/**

 	MAKE_desc_from_list

    @brief	Make a descriptor from a list of values
    according to the sql-standard.


    @param desc
    @param node
	@param expression_name

 **/
void MAKE_desc_from_list(CompiledStatement* statement, dsc* desc, dsql_nod* node,
						 dsql_nod* null_replacement,
						 const TEXT* expression_name)
{
	Firebird::Array<const dsc*> args;

	fb_assert(node->nod_type == nod_list);

	for (dsql_nod** p = node->nod_arg; p < node->nod_arg + node->nod_count; ++p)
	{
		MAKE_desc(statement, &(*p)->nod_desc, *p, NULL);
		args.add(&(*p)->nod_desc);
	}

	DSqlDataTypeUtil(statement).makeFromList(desc, expression_name, args.getCount(), args.begin());

	// If we have literal NULLs only, let the result be either
	// CHAR(1) CHARACTER SET NONE or the context-provided datatype
	if (desc->isNull() && null_replacement)
	{
		MAKE_desc(statement, desc, null_replacement, NULL);
		desc->dsc_flags |= DSC_null | DSC_nullable;
		return;
	}
}


/**

 	MAKE_field

    @brief	Make up a field node.


    @param context
    @param field
    @param indices

 **/
dsql_nod* MAKE_field(dsql_ctx* context, dsql_fld* field, dsql_nod* indices)
{
	DEV_BLKCHK(context, dsql_type_ctx);
	DEV_BLKCHK(field, dsql_type_fld);
	DEV_BLKCHK(indices, dsql_type_nod);

	dsql_nod* node = MAKE_node(nod_field, e_fld_count);
	node->nod_arg[e_fld_context] = (dsql_nod*) context;
	node->nod_arg[e_fld_field] = (dsql_nod*) field;
	if (field->fld_dimensions)
	{
		if (indices)
		{
			node->nod_arg[e_fld_indices] = indices;
			MAKE_desc_from_field(&node->nod_desc, field);
			node->nod_desc.dsc_dtype = static_cast<UCHAR>(field->fld_element_dtype);
			node->nod_desc.dsc_length = field->fld_element_length;

			// node->nod_desc.dsc_scale = field->fld_scale;
			// node->nod_desc.dsc_sub_type = field->fld_sub_type;

		}
		else
		{
			node->nod_desc.dsc_dtype = dtype_array;
			node->nod_desc.dsc_length = sizeof(ISC_QUAD);
			node->nod_desc.dsc_scale = static_cast<SCHAR>(field->fld_scale);
			node->nod_desc.dsc_sub_type = field->fld_sub_type;
		}
	}
	else
	{
		if (indices)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					  Arg::Gds(isc_dsql_only_can_subscript_array) << Arg::Str(field->fld_name));
		}

		MAKE_desc_from_field(&node->nod_desc, field);
	}

	if ((field->fld_flags & FLD_nullable) || (context->ctx_flags & CTX_outer_join))
	{
		node->nod_desc.dsc_flags |= DSC_nullable;
	}

	// check if the field is a system domain and the type is CHAR/VARCHAR CHARACTER SET UNICODE_FSS
	if ((field->fld_flags & FLD_system) && node->nod_desc.dsc_dtype <= dtype_varying &&
		INTL_GET_CHARSET(&node->nod_desc) == CS_METADATA)
	{
		USHORT adjust = 0;

		if (node->nod_desc.dsc_dtype == dtype_varying)
			adjust = sizeof(USHORT);
		else if (node->nod_desc.dsc_dtype == dtype_cstring)
			adjust = 1;

		node->nod_desc.dsc_length -= adjust;
		node->nod_desc.dsc_length *= 3;
		node->nod_desc.dsc_length += adjust;
	}

	return node;
}


/**

 	MAKE_field_name

    @brief	Make up a field name node.


    @param field_name

 **/
dsql_nod* MAKE_field_name(const char* field_name)
{
    dsql_nod* const field_node = MAKE_node(nod_field_name, (int) e_fln_count);
    field_node->nod_arg[e_fln_name] = (dsql_nod*) MAKE_cstring(field_name);
	return field_node;
}


/**

 	MAKE_list

    @brief	Make a list node from a linked list stack of things.


    @param stack

 **/
dsql_nod* MAKE_list(DsqlNodStack& stack)
{
	USHORT count = stack.getCount();
	dsql_nod* node = MAKE_node(nod_list, count);
	dsql_nod** ptr = node->nod_arg + count;

	while (stack.hasData())
	{
		*--ptr = stack.pop();
	}

	return node;
}


/**

 	MAKE_node

    @brief	Make a node of given type.


    @param type
    @param count

 **/
dsql_nod* MAKE_node(NOD_TYPE type, int count)
{
	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(), count) dsql_nod;
	node->nod_type = type;
	node->nod_count = count;

	return node;
}


/**

 	MAKE_parameter

    @brief	Generate a parameter block for a message.  If requested,
 	set up for a null flag as well.


    @param message
    @param sqlda_flag
    @param null_flag
    @param sqlda_index
	@param node

 **/
dsql_par* MAKE_parameter(dsql_msg* message, bool sqlda_flag, bool null_flag,
	USHORT sqlda_index, const dsql_nod* node)
{
	if (!message) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_badmsgnum));
	}

	DEV_BLKCHK(message, dsql_type_msg);

	if (sqlda_flag && sqlda_index && sqlda_index <= message->msg_index)
	{
		// This parameter possibly already here. Look for it
		for (dsql_par* temp = message->msg_parameters; temp; temp = temp->par_next) {
			if (temp->par_index == sqlda_index)
				return temp;
		}
	}

	thread_db* tdbb = JRD_get_thread_data();

	dsql_par* parameter = FB_NEW(*tdbb->getDefaultPool()) dsql_par;
	parameter->par_message = message;
	parameter->par_next = message->msg_parameters;
	message->msg_parameters = parameter;
	parameter->par_parameter = message->msg_parameter++;

	parameter->par_rel_name = NULL;
	parameter->par_owner_name = NULL;
	parameter->par_rel_alias = NULL;

	if (node) {
		make_parameter_names(parameter, node);
	}

	// If the parameter is used declared, set SQLDA index
	if (sqlda_flag)
	{
		if (sqlda_index) {
			parameter->par_index = sqlda_index;
			if (message->msg_index < sqlda_index)
				message->msg_index = sqlda_index;
		}
		else {
			parameter->par_index = ++message->msg_index;
		}
	}

	// If a null handing has been requested, set up a null flag

	if (null_flag) {
		dsql_par* null = MAKE_parameter(message, false, false, 0, NULL);
		parameter->par_null = null;
		null->par_desc.dsc_dtype = dtype_short;
		null->par_desc.dsc_scale = 0;
		null->par_desc.dsc_length = sizeof(SSHORT);
	}

	return parameter;
}

/**

 	MAKE_string

    @brief	Generalized routine for making a string block.


    @param str
    @param length

 **/
dsql_str* MAKE_string(const char* str, int length)
{
	fb_assert(length >= 0);
	return MAKE_tagged_string(str, length, NULL);
}


/**

 	MAKE_symbol

    @brief	Make a symbol for an object and insert symbol into hash table.


    @param database
    @param name
    @param length
    @param type
    @param object

 **/
dsql_sym* MAKE_symbol(dsql_dbb* database,
				const TEXT* name, USHORT length, SYM_TYPE type, dsql_req* object)
{
	DEV_BLKCHK(database, dsql_type_dbb);
	DEV_BLKCHK(object, dsql_type_req);
	fb_assert(name);
	fb_assert(length > 0);

	thread_db* tdbb = JRD_get_thread_data();

	dsql_sym* symbol = FB_NEW_RPT(*tdbb->getDefaultPool(), length) dsql_sym;
	symbol->sym_type = type;
	symbol->sym_object = object;
	symbol->sym_dbb = database;
	symbol->sym_length = length;
	TEXT* p = symbol->sym_name;
	symbol->sym_string = p;

	if (length)
		memcpy(p, name, length);

	HSHD_insert(symbol);

	return symbol;
}


/**

 	MAKE_tagged_string

    @brief	Generalized routine for making a string block.
 	Which is tagged with a character set descriptor.


    @param str_
    @param length
    @param charset

 **/
dsql_str* MAKE_tagged_string(const char* strvar, size_t length, const char* charset)
{
	thread_db* tdbb = JRD_get_thread_data();

	dsql_str* string = FB_NEW_RPT(*tdbb->getDefaultPool(), length) dsql_str;
	string->str_charset = charset;
	string->str_length  = length;
	memcpy(string->str_data, strvar, length);

	return string;
}


/**

 	MAKE_trigger_type

    @brief	Make a trigger type


    @param prefix_node
    @param suffix_node

 **/
dsql_nod* MAKE_trigger_type(dsql_nod* prefix_node, dsql_nod* suffix_node)
{
	const SLONG prefix = prefix_node->getSlong();
	const SLONG suffix = suffix_node->getSlong();
	delete prefix_node;
	delete suffix_node;
	return MAKE_const_slong(prefix + suffix - 1);
}


/**

 	MAKE_variable

    @brief	Make up a field node.


    @param field
    @param name
    @param type
    @param msg_number
    @param item_number
    @param local_number

 **/
dsql_nod* MAKE_variable(dsql_fld* field, const TEXT* name, const dsql_var_type type,
				 		USHORT msg_number, USHORT item_number, USHORT local_number)
{
	DEV_BLKCHK(field, dsql_type_fld);

	thread_db* tdbb = JRD_get_thread_data();

	dsql_var* variable = FB_NEW_RPT(*tdbb->getDefaultPool(), strlen(name)) dsql_var;
	dsql_nod* node = MAKE_node(nod_variable, e_var_count);
	node->nod_arg[e_var_variable] = (dsql_nod*) variable;
	variable->var_msg_number = msg_number;
	variable->var_msg_item = item_number;
	variable->var_variable_number = local_number;
	variable->var_field = field;
	strcpy(variable->var_name, name);
	//variable->var_flags = 0;
	variable->var_type = type;
	if (field)
		MAKE_desc_from_field(&node->nod_desc, field);

	return node;
}


/**

	make_null

	@brief  Prepare a descriptor to signal SQL NULL

	@param desc

**/
static void make_null(dsc* const desc)
{
	desc->dsc_dtype = dtype_long;
	desc->dsc_sub_type = 0;
	desc->dsc_length = sizeof(SLONG);
	desc->dsc_scale = 0;
	desc->dsc_flags |= DSC_nullable;
}


/**

	make_parameter_names

	@brief  Determine relation/column/alias names (if appropriate)
			and store them in the given parameter.

	@param parameter
	@param item

**/
static void make_parameter_names(dsql_par* parameter, const dsql_nod* item)
{
	const dsql_fld* field;
	const dsql_ctx* context = NULL;
	const dsql_str* string;
	const dsql_nod* alias;

	fb_assert(parameter && item);

	const char* name_alias = NULL;

	switch (item->nod_type)
	{
	case nod_field:
		field = (dsql_fld*) item->nod_arg[e_fld_field];
		name_alias = field->fld_name.c_str();
		context = (dsql_ctx*) item->nod_arg[e_fld_context];
		break;
	case nod_dbkey:
		parameter->par_name = parameter->par_alias = DB_KEY_NAME;
		context = (dsql_ctx*) item->nod_arg[0]->nod_arg[0];
		break;
	case nod_alias:
		string = (dsql_str*) item->nod_arg[e_alias_alias];
		parameter->par_alias = string->str_data;
		alias = item->nod_arg[e_alias_value];
		if (alias->nod_type == nod_field) {
			field = (dsql_fld*) alias->nod_arg[e_fld_field];
			parameter->par_name = field->fld_name.c_str();
			context = (dsql_ctx*) alias->nod_arg[e_fld_context];
		}
		else if (alias->nod_type == nod_dbkey) {
			parameter->par_name = DB_KEY_NAME;
			context = (dsql_ctx*) alias->nod_arg[0]->nod_arg[0];
		}
		break;
	case nod_via:
		// subquery, aka sub-select
		make_parameter_names(parameter, item->nod_arg[e_via_value_1]);
		break;
	case nod_derived_field:
		string = (dsql_str*) item->nod_arg[e_derived_field_name];
		parameter->par_alias = string->str_data;
		alias = item->nod_arg[e_derived_field_value];
		if (alias->nod_type == nod_field) {
			field = (dsql_fld*) alias->nod_arg[e_fld_field];
			parameter->par_name = field->fld_name.c_str();
			context = (dsql_ctx*) alias->nod_arg[e_fld_context];
		}
		else if (alias->nod_type == nod_dbkey) {
			parameter->par_name = DB_KEY_NAME;
			context = (dsql_ctx*) alias->nod_arg[0]->nod_arg[0];
		}
		break;
	case nod_map:
		{
			const dsql_map* map = (dsql_map*) item->nod_arg[e_map_map];
			const dsql_nod* map_node = map->map_node;
			while (map_node->nod_type == nod_map) {
				// skip all the nod_map nodes
				map = (dsql_map*) map_node->nod_arg[e_map_map];
				map_node = map->map_node;
			}
			switch (map_node->nod_type)
			{
			case nod_field:
				field = (dsql_fld*) map_node->nod_arg[e_fld_field];
				name_alias = field->fld_name.c_str();
				context = (dsql_ctx*) map_node->nod_arg[e_fld_context];
				break;
			case nod_alias:
				string = (dsql_str*) map_node->nod_arg[e_alias_alias];
				parameter->par_alias = string->str_data;
				alias = map_node->nod_arg[e_alias_value];
				if (alias->nod_type == nod_field) {
					field = (dsql_fld*) alias->nod_arg[e_fld_field];
					parameter->par_name = field->fld_name.c_str();
					context = (dsql_ctx*) alias->nod_arg[e_fld_context];
				}
				break;
			case nod_derived_field:
				string = (dsql_str*) map_node->nod_arg[e_derived_field_name];
				parameter->par_alias = string->str_data;
				alias = map_node->nod_arg[e_derived_field_value];
				if (alias->nod_type == nod_field) {
					field = (dsql_fld*) alias->nod_arg[e_fld_field];
					parameter->par_name = field->fld_name.c_str();
					context = (dsql_ctx*) alias->nod_arg[e_fld_context];
				}
				break;

			case nod_agg_count:
				name_alias = "COUNT";
				break;
			case nod_agg_total:
			case nod_agg_total2:
				name_alias = "SUM";
				break;
			case nod_agg_average:
			case nod_agg_average2:
				name_alias = "AVG";
				break;
			case nod_agg_min:
				name_alias = "MIN";
				break;
			case nod_agg_max:
				name_alias = "MAX";
				break;
			case nod_agg_list:
				name_alias = "LIST";
				break;
			case nod_constant:
				name_alias = "CONSTANT";
				break;
			case nod_dbkey:
				name_alias = DB_KEY_NAME;
				break;
			} // switch(map_node->nod_type)
			break;
		} // case nod_map
	case nod_variable:
		{
			dsql_var* variable = (dsql_var*) item->nod_arg[e_var_variable];
			if (variable->var_field)
				name_alias = variable->var_field->fld_name.c_str();
			break;
		}
	case nod_udf:
		{
			dsql_udf* userFunc = (dsql_udf*) item->nod_arg[0];
			name_alias = userFunc->udf_name.c_str();
			break;
		}
	case nod_sys_function:
		name_alias = ((dsql_str*) item->nod_arg[e_sysfunc_name])->str_data;
		break;
	case nod_gen_id:
	case nod_gen_id2:
		name_alias	= "GEN_ID";
		break;
	case nod_user_name:
		name_alias	= "USER";
		break;
	case nod_current_role:
		name_alias	= "ROLE";
		break;
	case nod_internal_info:
		{
			const internal_info_id id =
				*reinterpret_cast<internal_info_id*>(item->nod_arg[0]->nod_desc.dsc_address);
			name_alias = InternalInfo::getAlias(id);
		}
		break;
	case nod_concatenate:
		if ( !Config::getOldColumnNaming() )
			name_alias = "CONCATENATION";
		break;
	case nod_constant:
	case nod_null:
		name_alias = "CONSTANT";
		break;
	case nod_negate:
		{
			// CVC: For this to be a thorough check, we need to recurse over all nodes.
			// This means we should separate the code that sets aliases from
			// the rest of the functionality here in make_parameter_names().
			// Otherwise, we need to test here for most of the other node types.
			// However, we need to be recursive only if we agree things like -gen_id()
			// should be given the GEN_ID alias, too.
			int level = 0;
			const dsql_nod* node = item->nod_arg[0];
			while (node->nod_type == nod_negate)
			{
				node = node->nod_arg[0];
				++level;
			}

			switch (node->nod_type)
			{
			case nod_constant:
			case nod_null:
				name_alias = "CONSTANT";
				break;
			/*
			case nod_add:
			case nod_add2:
				name_alias = "ADD";
				break;
			case nod_subtract:
			case nod_subtract2:
				name_alias = "SUBTRACT";
				break;
			*/
			case nod_multiply:
			case nod_multiply2:
				if (!level)
					name_alias = "MULTIPLY";
				break;
			case nod_divide:
			case nod_divide2:
				if (!level)
					name_alias = "DIVIDE";
				break;
			}
		}
		break;
	case nod_add:
	case nod_add2:
		name_alias = "ADD";
		break;
	case nod_subtract:
	case nod_subtract2:
		name_alias = "SUBTRACT";
		break;
	case nod_multiply:
	case nod_multiply2:
		name_alias = "MULTIPLY";
		break;
	case nod_divide:
	case nod_divide2:
		name_alias = "DIVIDE";
		break;
	case nod_substr:
		name_alias = "SUBSTRING";
		break;
	case nod_trim:
		name_alias = "TRIM";
		break;
	case nod_cast:
		if ( !Config::getOldColumnNaming() )
			name_alias	= "CAST";
		break;
	case nod_upcase:
		if ( !Config::getOldColumnNaming() )
            name_alias	= "UPPER";
		break;
    case nod_lowcase:
        name_alias	= "LOWER";
		break;
	case nod_current_date:
		if ( !Config::getOldColumnNaming() )
			name_alias = "CURRENT_DATE";
		break;
	case nod_current_time:
		if ( !Config::getOldColumnNaming() )
			name_alias = "CURRENT_TIME";
		break;
	case nod_current_timestamp:
		if ( !Config::getOldColumnNaming() )
			name_alias = "CURRENT_TIMESTAMP";
		break;
	case nod_extract:
		if ( !Config::getOldColumnNaming() )
            name_alias = "EXTRACT";
		break;
	case nod_strlen:
		{
			const ULONG length_type = item->nod_arg[e_strlen_type]->getSlong();

			switch (length_type)
			{
				case blr_strlen_bit:
					name_alias = "BIT_LENGTH";
					break;

				case blr_strlen_char:
					name_alias = "CHAR_LENGTH";
					break;

				case blr_strlen_octet:
					name_alias = "OCTET_LENGTH";
					break;

				default:
					name_alias = "LENGTH";
					fb_assert(false);
					break;
			}
		}
		break;
	case nod_searched_case:
	case nod_simple_case:
		name_alias = "CASE";
		break;
	case nod_coalesce:
		name_alias = "COALESCE";
		break;
	}

	if (name_alias) {
		parameter->par_name = parameter->par_alias = name_alias;
	}

	if (context)
	{
		if (context->ctx_relation)
		{
			parameter->par_rel_name = context->ctx_relation->rel_name.c_str();
			parameter->par_owner_name = context->ctx_relation->rel_owner.c_str();
		}
		else if (context->ctx_procedure)
		{
			parameter->par_rel_name = context->ctx_procedure->prc_name.c_str();
			parameter->par_owner_name = context->ctx_procedure->prc_owner.c_str();
		}

		parameter->par_rel_alias = context->ctx_alias;
	}
}
