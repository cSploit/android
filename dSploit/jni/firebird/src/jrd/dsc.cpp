/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dsc.cpp
 *	DESCRIPTION:	Utility routines for working with data descriptors
 *
 *	Note: This module is used by both engine & utility functions.
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
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include "../jrd/common.h"
#include "../jrd/dsc.h"
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../jrd/dsc_proto.h"


/* When converting non-text values to text, how many bytes to allocate
   for holding the text image of the value.  */

static const USHORT _DSC_convert_to_text_length[DTYPE_TYPE_MAX] =
{
	0,							/* dtype_unknown */
	0,							/* dtype_text */
	0,							/* dtype_cstring */
	0,							/* dtype_varying */
	0,
	0,
	0,							/* dtype_packed */
	0,							/* dtype_byte */
	6,							/* dtype_short      -32768 */
	11,							/* dtype_long       -2147483648 */
	20,							/* dtype_quad       -9223372036854775808 */
	15,							/* dtype_real       -1.23456789e+12 */
	24,							/* dtype_double     -1.2345678901234567e+123 */
	24,							/* dtype_d_float (believed to have this range)  -1.2345678901234567e+123 */
	10,							/* dtype_sql_date   YYYY-MM-DD */
	13,							/* dtype_sql_time   HH:MM:SS.MMMM */
	25,							/* dtype_timestamp  YYYY-MM-DD HH:MM:SS.MMMM */
	/*  -- in BLR_version4  DD-Mon-YYYY HH:MM:SS.MMMM */
	9,							/* dtype_blob       FFFF:FFFF */
	9,							/* dtype_array      FFFF:FFFF */
	20,							/* dtype_int64      -9223372036854775808 */
	0							// dtype_dbkey
};

/* blr to dsc type conversions */
static const USHORT DSC_blr_type_mapping[] =
{
	blr_null,
	blr_text,
	blr_cstring,
	blr_varying,
	blr_null,
	blr_null,
	blr_null,
	blr_null,
	blr_short,
	blr_long,
	blr_double,
	blr_d_float,
	blr_sql_date,
	blr_sql_time,
	blr_timestamp,
	blr_blob,
	blr_blob,
	blr_int64,
	blr_null
};

/* Unimplemented names are in lowercase & <brackets> */
/* Datatypes that represent a range of SQL datatypes are in lowercase */
static const TEXT* const DSC_dtype_names[] =
{
	"<dtype_unknown>",
	"CHAR",
	"CSTRING",
	"VARCHAR",
	"<dtype_4>",
	"<dtype_5>",
	"<packed>",
	"<byte>",
	"SMALLINT",					/* Also DECIMAL & NUMERIC */
	"INTEGER",					/* Also DECIMAL & NUMERIC */
	"<quad>",
	"FLOAT",
	"DOUBLE PRECISION",
	"<d_float>",
	"DATE",
	"TIME",
	"TIMESTAMP",
	"BLOB",
	"ARRAY",
	"BIGINT",
	"DB_KEY"
};


/* The result of adding two datatypes in blr_version5 semantics
   Note: DTYPE_CANNOT as the result means that the operation cannot be done.
   dtype_unknown as the result means that we do not yet know the type of one of
   the operands, so we cannot decide the type of the result. */

const BYTE DSC_add_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX] =
{

/*
	dtype_unknown	dtype_text	dtype_cstring	dtype_varying
	4 (unused)	5 (unused)	dtype_packed	dtype_byte
	dtype_short	dtype_long	dtype_quad	dtype_real
	dtype_double	dtype_d_float	dtype_sql_date	dtype_sql_time
	dtype_timestamp	dtype_blob	dtype_array	dtype_int64
	dtype_dbkey
*/

	/* dtype_unknown */
	{dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, dtype_unknown,
	 dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, dtype_unknown,
	 DTYPE_CANNOT},

	/* dtype_text */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_cstring */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_varying */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* 4 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* 5 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_packed */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_byte */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_short */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_long */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_quad */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_real */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_double */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_d_float -- VMS deprecated */
	{dtype_unknown, dtype_d_float, dtype_d_float, dtype_d_float,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, dtype_d_float,
	 dtype_d_float, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_d_float,
	 DTYPE_CANNOT},

	/* dtype_sql_date */
	{dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_sql_date, dtype_sql_date, DTYPE_CANNOT, dtype_sql_date,
	 dtype_sql_date, dtype_sql_date, DTYPE_CANNOT, dtype_timestamp,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_sql_date,
	 DTYPE_CANNOT},

	/* dtype_sql_time */
	{dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_sql_time, dtype_sql_time, DTYPE_CANNOT, dtype_sql_time,
	 dtype_sql_time, dtype_sql_time, dtype_timestamp, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_sql_time,
	 DTYPE_CANNOT},

	/* dtype_timestamp */
	{dtype_unknown, dtype_timestamp, dtype_timestamp, dtype_timestamp,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, dtype_timestamp, DTYPE_CANNOT, dtype_timestamp,
	 dtype_timestamp, dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_timestamp,
	 DTYPE_CANNOT},

	/* dtype_blob */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_array */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_int64 */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, dtype_sql_date, dtype_sql_time,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_dbkey */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT}

};

/* The result of subtracting two datatypes in blr_version5 semantics
   Note: DTYPE_CANNOT as the result means that the operation cannot be done.
   dtype_unknown as the result means that we do not yet know the type of one of
   the operands, so we cannot decide the type of the result. */

const BYTE DSC_sub_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX] =
{

/*
	dtype_unknown	dtype_text	dtype_cstring	dtype_varying
	4 (unused)	5 (unused)	dtype_packed	dtype_byte
	dtype_short	dtype_long	dtype_quad	dtype_real
	dtype_double	dtype_d_float	dtype_sql_date	dtype_sql_time
	dtype_timestamp	dtype_blob	dtype_array	dtype_int64
	dtype_dbkey
*/

	/* dtype_unknown */
	{dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, dtype_unknown,
	 dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, dtype_unknown,
	 DTYPE_CANNOT},

	/* dtype_text */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_cstring */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_varying */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* 4 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* 5 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_packed */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_byte */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_short */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_long */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_quad */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_real */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_double */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_d_float -- VMS deprecated */
	{dtype_unknown, dtype_d_float, dtype_d_float, dtype_d_float,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, dtype_d_float,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_d_float,
	 DTYPE_CANNOT},

	/* dtype_sql_date */
	{dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_sql_date, dtype_sql_date, DTYPE_CANNOT, dtype_sql_date,
	 dtype_sql_date, dtype_sql_date, dtype_long, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_sql_date,
	 DTYPE_CANNOT},

	/* dtype_sql_time */
	{dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_sql_time, dtype_sql_time, DTYPE_CANNOT, dtype_sql_time,
	 dtype_sql_time, dtype_sql_time, DTYPE_CANNOT, dtype_long,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_sql_time,
	 DTYPE_CANNOT},

	/* dtype_timestamp */
	{dtype_unknown, dtype_timestamp, dtype_timestamp, dtype_timestamp,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_timestamp, dtype_timestamp, DTYPE_CANNOT, dtype_timestamp,
	 dtype_timestamp, dtype_timestamp, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, DTYPE_CANNOT, DTYPE_CANNOT, dtype_timestamp,
	 DTYPE_CANNOT},

	/* dtype_blob */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_array */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_int64 */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_dbkey */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT}
};


/* The result of multiplying or dividing two datatypes in blr_version5 semantics
   Note: DTYPE_CANNOT as the result means that the operation cannot be done.
   dtype_unknown as the result means that we do not yet know the type of one of
   the operands, so we cannot decide the type of the result. */

const BYTE DSC_multiply_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX] =
{

/*
	dtype_unknown	dtype_text	dtype_cstring	dtype_varying
	4 (unused)	5 (unused)	dtype_packed	dtype_byte
	dtype_short	dtype_long	dtype_quad	dtype_real
	dtype_double	dtype_d_float	dtype_sql_date	dtype_sql_time
	dtype_timestamp	dtype_blob	dtype_array	dtype_int64
	dtype_dbkey
*/

	/* dtype_unknown */
	{dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, dtype_unknown,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_unknown,
	 DTYPE_CANNOT},

	/* dtype_text */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_cstring */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_varying */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* 4 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* 5 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_packed */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_byte */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_short */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_long */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_quad */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_real */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_double */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_d_float -- VMS deprecated */
	{dtype_unknown, dtype_d_float, dtype_d_float, dtype_d_float,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, dtype_d_float,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_d_float,
	 DTYPE_CANNOT},

	/* dtype_sql_date */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_sql_time */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_timestamp */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_blob */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_array */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_int64 */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_int64, dtype_int64, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_int64,
	 DTYPE_CANNOT},

	/* dtype_dbkey */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT}

};

/* The result of multiplying two datatypes in blr_version4 semantics.
   Note: DTYPE_CANNOT as the result means that the operation cannot be done.
   dtype_unknown as the result means that we do not yet know the type of one of
   the operands, so we cannot decide the type of the result. */

const BYTE DSC_multiply_blr4_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX] =
{

/*
	dtype_unknown	dtype_text	dtype_cstring	dtype_varying
	4 (unused)	5 (unused)	dtype_packed	dtype_byte
	dtype_short	dtype_long	dtype_quad	dtype_real
	dtype_double	dtype_d_float	dtype_sql_date	dtype_sql_time
	dtype_timestamp	dtype_blob	dtype_array	dtype_int64
	dtype_dbkey
*/

	/* dtype_unknown */
	{dtype_unknown, dtype_unknown, dtype_unknown, dtype_unknown,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, dtype_unknown,
	 dtype_unknown, dtype_unknown, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_unknown,
	 DTYPE_CANNOT},

	/* dtype_text */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_cstring */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_varying */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* 4 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* 5 (unused) */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_packed */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_byte */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_short */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_long, dtype_long, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_long */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_long, dtype_long, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_quad */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_real */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_double */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_d_float -- VMS deprecated */
	{dtype_unknown, dtype_d_float, dtype_d_float, dtype_d_float,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, dtype_d_float,
	 dtype_d_float, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_d_float,
	 DTYPE_CANNOT},

	/* dtype_sql_date */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_sql_time */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_timestamp */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_blob */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_array */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT},

	/* dtype_int64 */
	{dtype_unknown, dtype_double, dtype_double, dtype_double,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 dtype_double, dtype_double, DTYPE_CANNOT, dtype_double,
	 dtype_double, dtype_d_float, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, dtype_double,
	 DTYPE_CANNOT},

	/* dtype_dbkey */
	{DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT, DTYPE_CANNOT,
	 DTYPE_CANNOT}

};

#ifdef DEV_BUILD
static bool validate_dsc_tables();
#endif



int dsc::getStringLength() const
{
	return DSC_string_length(this);
}


USHORT DSC_convert_to_text_length(USHORT dsc_type)
{
	if (dsc_type < (sizeof(_DSC_convert_to_text_length) / sizeof(_DSC_convert_to_text_length[0])))
		return _DSC_convert_to_text_length[dsc_type] + (dsc_type == dtype_int64 ? 1 : 0);
	fb_assert(FALSE);
	return 0;
}


bool DSC_make_descriptor(DSC* desc,
						USHORT blr_type,
						SSHORT scale,
						USHORT length,
						SSHORT sub_type,
						SSHORT charset,
						SSHORT collation)
{
/**************************************
 *
 *	D S C _ m a k e _ d e s c r i p t o r
 *
 **************************************
 *
 * Functional description
 *	Make an internal descriptor from external parameters.
 *
 **************************************/

#ifdef DEV_BUILD
	{
/* Execute this validation code once per server startup only */
		static bool been_here = false;
		if (!been_here)
		{
			been_here = true;
			fb_assert(validate_dsc_tables());
		}
	}
#endif

	desc->dsc_flags = 0;
	desc->dsc_address = NULL;
	desc->dsc_length = length;
	desc->dsc_scale = (SCHAR) scale;
	desc->dsc_sub_type = sub_type;

	switch (blr_type)
	{
	case blr_text:
		desc->dsc_dtype = dtype_text;
		INTL_ASSIGN_DSC(desc, charset, collation);
		break;

	case blr_varying:
		desc->dsc_dtype = dtype_varying;
		desc->dsc_length += sizeof(USHORT);
		INTL_ASSIGN_DSC(desc, charset, collation);
		break;

	case blr_short:
		desc->dsc_length = sizeof(SSHORT);
		desc->dsc_dtype = dtype_short;
		break;

	case blr_long:
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_dtype = dtype_long;
		break;

	case blr_int64:
		desc->dsc_length = sizeof(SINT64);
		desc->dsc_dtype = dtype_int64;
		break;

	case blr_quad:
		desc->dsc_length = 2 * sizeof(SLONG);
		desc->dsc_dtype = dtype_quad;
		break;

	case blr_float:
		desc->dsc_length = sizeof(float);
		desc->dsc_dtype = dtype_real;
		break;

	case blr_double:
	case blr_d_float:
		desc->dsc_length = sizeof(double);
		desc->dsc_dtype = dtype_double;
		break;

	case blr_timestamp:
		desc->dsc_length = 2 * sizeof(SLONG);
		desc->dsc_dtype = dtype_timestamp;
		break;

	case blr_sql_date:
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_dtype = dtype_sql_date;
		break;

	case blr_sql_time:
		desc->dsc_length = sizeof(ULONG);
		desc->dsc_dtype = dtype_sql_time;
		break;

	case blr_blob:
		desc->dsc_length = 2 * sizeof(SLONG);
		desc->dsc_dtype = dtype_blob;
		if (sub_type == isc_blob_text)
		{
			fb_assert(charset <= MAX_SCHAR);
			desc->dsc_scale = (SCHAR) charset;
			desc->dsc_flags = collation << 8;	// collation of blob
		}
		break;

	case blr_cstring:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_DSC(desc, charset, collation);
		break;

	default:
		fb_assert(FALSE);
		desc->dsc_dtype = dtype_unknown;
		return false;
		break;
	}
	return true;
}


int DSC_string_length(const dsc* desc)
{
/**************************************
 *
 *	D S C _ s t r i n g _ l e n g t h
 *
 **************************************
 *
 * Functional description
 *	Estimate length of string (in bytes) based on descriptor.
 *	Estimated length assumes representing string in
 *	narrow-char ASCII format.
 *
 *	Note that this strips off the short at the
 *	start of dtype_varying, and the NULL for dtype_cstring.
 *
 **************************************/

	switch (desc->dsc_dtype)
	{
	case dtype_text:
		return desc->dsc_length;
	case dtype_cstring:
		return desc->dsc_length - 1;
	case dtype_varying:
		return desc->dsc_length - sizeof(USHORT);
	default:
 		if (!DTYPE_IS_EXACT(desc->dsc_dtype) || desc->dsc_scale == 0)
 			return (int) _DSC_convert_to_text_length[desc->dsc_dtype];
 		if (desc->dsc_scale < 0)
			return (int) _DSC_convert_to_text_length[desc->dsc_dtype] + 1;
		return (int) _DSC_convert_to_text_length[desc->dsc_dtype] + desc->dsc_scale;
	}
}


const TEXT *DSC_dtype_tostring(UCHAR dtype)
{
/**************************************
 *
 *	D S C _ d t y p e _ t o s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Convert a datatype to its textual representation
 *
 **************************************/
	if (dtype < FB_NELEM(DSC_dtype_names))
		return DSC_dtype_names[dtype];

	return "<unknown>";
}


void DSC_get_dtype_name(const dsc* desc, TEXT * buffer, USHORT len)
{
/**************************************
 *
 *	D S C _ g e t _ d t y p e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Convert a datatype to its textual representation
 *
 **************************************/
	// This function didn't put a string terminator even though
	// it's calling strncpy that doesn't put it if source > target.
	strncpy(buffer, DSC_dtype_tostring(desc->dsc_dtype), len);
	buffer[len - 1] = 0;
}


#ifdef DEV_BUILD
void dsc::address32bit() const
{
/**************************************
 *
 *	a d d r e s s 3 2 b i t
 *
 **************************************
 *
 * Functional description
 *	Validates dsc_address member to fit into 4-byte integer.
 *
 **************************************/
#if SIZEOF_VOID_P > 4
	FB_UINT64 addr = (FB_UINT64)(IPTR) dsc_address;
	fb_assert(addr == (addr & 0xFFFFFFFF));
#endif
}


static bool validate_dsc_tables()
{
/**************************************
 *
 *	v a l i d a t e _ d s c _ t a b l e s
 *
 **************************************
 *
 * Functional description
 *	Validate tables that are used to describe
 *	operations on descriptors.  Note that this function
 *	is called as part of a DEV_BUILD assertion check
 *	only once per server.
 *
 * WARNING: the fprintf's are commented out because trying to print
 *          to stderr when the server is running detached will trash
 *          server memory.  If you uncomment the printf's and build a
 *          kit, make sure that you run that server with the -d flag
 *          so it won't detach from its controlling terminal.
 *
 **************************************/
	for (BYTE op1 = dtype_unknown; op1 < DTYPE_TYPE_MAX; op1++)
	{
		for (BYTE op2 = dtype_unknown; op2 < DTYPE_TYPE_MAX; op2++)
		{

			if ((DSC_add_result[op1][op2] >= DTYPE_TYPE_MAX) &&
				(DSC_add_result[op1][op2] != DTYPE_CANNOT))
			{
/*
	fprintf (stderr, "DSC_add_result [%d][%d] is %d, invalid.\n",
		 op1, op2, DSC_add_result [op1][op2]);
*/
				return false;
			}

			/* Addition operator must be communitive */
			if (DSC_add_result[op1][op2] != DSC_add_result[op2][op1])
			{
/*
	fprintf (stderr,
"Not commutative: DSC_add_result[%d][%d] = %d, ... [%d][%d] = %d\n",
		 op1, op2, DSC_add_result [op1][op2],
		 op2, op1, DSC_add_result [op2][op1] );
*/
				return false;
			}

			/* Difficult to validate Subtraction */

			if ((DSC_sub_result[op1][op2] >= DTYPE_TYPE_MAX) &&
				(DSC_sub_result[op1][op2] != DTYPE_CANNOT))
			{
/*
	fprintf (stderr, "DSC_sub_result [%d][%d] is %d, invalid.\n",
		 op1, op2, DSC_sub_result [op1][op2]);
*/
				return false;
			}

			/* Multiplication operator must be commutative */
			if (DSC_multiply_result[op1][op2] != DSC_multiply_result[op2][op1])
			{
/*
	fprintf (stderr,
"Not commutative: DSC_multiply_result [%d][%d] = %d,\n\t... [%d][%d] = %d\n",
		 op1, op2, DSC_multiply_result [op1][op2],
		 op2, op1, DSC_multiply_result [op2][op1]);
*/
				return false;
			}

			/* Multiplication operator must be communitive */
			if (DSC_multiply_blr4_result[op1][op2] != DSC_multiply_blr4_result[op2][op1])
			{
/*
	fprintf (stderr,
"Not commutative: DSC_multiply_blr4_result [%d][%d] = %d\n\
\t... [%d][%d] = %d\n",
	    op1, op2, DSC_multiply_blr4_result [op1][op2],
	    op2, op1, DSC_multiply_blr4_result [op2][op1] );
*/
				return false;
			}
		}
	}
	return true;
}

#endif /* DEV_BUILD */


