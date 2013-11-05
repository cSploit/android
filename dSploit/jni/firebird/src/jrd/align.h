/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		align.h
 *	DESCRIPTION:	Maximum alignments for corresponding datatype
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
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 */

#ifndef JRD_ALIGN_H
#define JRD_ALIGN_H

/*
Maximum alignments for corresponding data types are defined in dsc.h
*/

#include "../jrd/blr.h"

/*  The following macro must be defined as the highest-numericly-valued
 *  blr which describes a datatype: arrays are sized based on this value.
 *  if a new blr is defined to represent a datatype in blr.h, and the new
 *  value is greater than blr_blob_id, be sure to change the next define,
 *  and also add the required entries to all of the arrays below.
 */
const unsigned char DTYPE_BLR_MAX	= blr_blob_id;

/*
 the blr types are defined in blr.h

No need to worry about blr_blob or ?blr_blob_id

*/

#include "../jrd/dsc.h"
#include "../jrd/RecordNumber.h"

static const USHORT gds_cvt_blr_dtype[DTYPE_BLR_MAX + 1] =
{
	0, 0, 0, 0, 0, 0, 0,
	dtype_short,				/* blr_short == 7 */
	dtype_long,					/* blr_long == 8 */
	dtype_quad,					/* blr_quad == 9 */
	dtype_real,					/* blr_float == 10 */
	dtype_d_float,				/* blr_d_float == 11 */
	dtype_sql_date,				/* blr_sql_date == 12 */
	dtype_sql_time,				/* blr_sql_time == 13 */
	dtype_text,					/* blr_text == 14 */
	dtype_text,					/* blr_text2 == 15 */
	dtype_int64,				/* blr_int64 == 16 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	dtype_double,				/* blr_double == 27 */
	0, 0, 0, 0, 0, 0, 0,
	dtype_timestamp,			/* blr_timestamp == 35 */
	0,
	dtype_varying,				/* blr_varying == 37 */
	dtype_varying,				/* blr_varying2 == 38 */
	0,
	dtype_cstring,				/* blr_cstring == 40 */
	dtype_cstring,				/* blr_cstring == 41 */
	0, 0, 0, 0
};

static const USHORT type_alignments[DTYPE_TYPE_MAX] =
{
	0,
	0,							/* dtype_text */
	0,							/* dtype_cstring */
	sizeof(SSHORT),				/* dtype_varying */
	0,							/* unused */
	0,							/* unused */
	sizeof(SCHAR),				/* dtype_packed */
	sizeof(SCHAR),				/* dtype_byte */
	sizeof(SSHORT),				/* dtype_short */
	sizeof(SLONG),				/* dtype_long */
#ifndef NATIVE_QUAD
	sizeof(SLONG),				/* dtype_quad */
#else
	sizeof(SQUAD),				/* dtype_quad */
#endif
	sizeof(float),				/* dtype_real */
	FB_DOUBLE_ALIGN,			/* dtype_double */
	FB_DOUBLE_ALIGN,			/* dtype_d_float */
	sizeof(GDS_DATE),			/* dtype_sql_date */
	sizeof(GDS_TIME),			/* dtype_sql_time */
	sizeof(GDS_DATE),			/* dtype_timestamp */
	sizeof(SLONG),				/* dtype_blob */
	sizeof(SLONG),				/* dtype_array */
	sizeof(SINT64),				/* dtype_int64 */
	sizeof(ULONG)				/* dtype_dbkey */
};

static const USHORT type_lengths[DTYPE_TYPE_MAX] =
{
	0,
	0,							/* dtype_text */
	0,							/* dtype_cstring */
	0,							/* dtype_varying */
	0,							/* unused */
	0,							/* unused */
	0,							/* dtype_packed */
	sizeof(SCHAR),				/* dtype_byte */
	sizeof(SSHORT),				/* dtype_short */
	sizeof(SLONG),				/* dtype_long */
	sizeof(ISC_QUAD),			/* dtype_quad */
	sizeof(float),				/* dtype_real */
	sizeof(double),				/* dtype_double */
	sizeof(double),				/* dtype_d_float */
	sizeof(GDS_DATE),			/* dtype_sql_date */
	sizeof(GDS_TIME),			/* dtype_sql_time */
	sizeof(GDS_TIMESTAMP),		/* dtype_timestamp */
	sizeof(ISC_QUAD),			/* dtype_blob */
	sizeof(ISC_QUAD),			/* dtype_array */
	sizeof(SINT64),				/* dtype_int64 */
	sizeof(RecordNumber::Packed) /*dtype_dbkey */
};


// This table is only used by gpre's cme.cpp.
// float, double are numbers from IEEE floating-point standard (IEEE 754)
static const USHORT type_significant_bits[DTYPE_TYPE_MAX] =
{
	0,
	0,							/* dtype_text */
	0,							/* dtype_cstring */
	0,							/* dtype_varying */
	0,							/* unused */
	0,							/* unused */
	0,							/* dtype_packed */
	sizeof(SCHAR) * 8,			/* dtype_byte */
	sizeof(SSHORT) * 8,			/* dtype_short */
	sizeof(SLONG) * 8,			/* dtype_long */
	sizeof(ISC_QUAD) * 8,		/* dtype_quad */
	23,							/* dtype_real,  23 sign. bits = 7 sign. digits */
	52,							/* dtype_double,  52 sign. bits = 15 sign. digits */
	52,							/* dtype_d_float,  52 sign. bits = 15 sign. digits */
	sizeof(GDS_DATE) * 8,		/* dtype_sql_date */
	sizeof(GDS_TIME) * 8,		/* dtype_sql_time */
	sizeof(GDS_TIMESTAMP) * 8,	/* dtype_timestamp */
	sizeof(ISC_QUAD) * 8,		/* dtype_blob */
	sizeof(ISC_QUAD) * 8,		/* dtype_array */
	sizeof(SINT64) * 8,			/* dtype_int64 */
	0							// dbkey
};

#endif /* JRD_ALIGN_H */
