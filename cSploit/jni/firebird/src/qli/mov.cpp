/*
 *	PROGRAM:	QLI Access Method
 *	MODULE:		mov.cpp
 *	DESCRIPTION:	Data mover and converter and comparator, etc.
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
#include <stdio.h>
#include <string.h>
#include "../qli/dtr.h"
#include "../common/classes/timestamp.h"
#include "../jrd/intl.h"
#include "../qli/err_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/gdsassert.h"
#include "../qli/mov_proto.h"
#include "../common/utils_proto.h"

using MsgFormat::SafeArg;


static void date_error(const TEXT*, const USHORT);
static double double_from_text(const dsc* desc);
static void sql_date_to_text(const SLONG[1], DSC*);
static void sql_time_to_text(const ULONG[1], DSC*);
static void timestamp_to_text(const SLONG[2], DSC*);
static void mover_error(int, USHORT, USHORT);
static void now_to_date(const tm*, SLONG[2]);
static void numeric_to_text(const dsc*, dsc*);
static void string_to_date(const TEXT*, USHORT, SLONG[2]);
static void string_to_time(const TEXT*, USHORT, SLONG[2]);
static const TEXT* type_name(USHORT);



#define LETTER(c)	(c >= 'A' && c <= 'Z')
#define DIGIT(c)	(c >= '0' && c <= '9')
const char* const TODAY = "TODAY";
const char* const NOW = "NOW";
const char* const TOMORROW = "TOMORROW";
const char* const YESTERDAY = "YESTERDAY";

const int PRECISION		= 10000;

struct dtypes_t
{
	USHORT type;
	const TEXT* description;
};

static const dtypes_t dtypes_table[] =
{
	{ dtype_unknown, "NULL" },
	{ dtype_text, "character string" },
	{ dtype_cstring, "character string" },
	{ dtype_varying, "varying string" },
	{ dtype_packed, "packed decimal" },
	{ dtype_byte, "byte integer" },
	{ dtype_short, "short integer" },
	{ dtype_long, "long integer" },
	{ dtype_quad, "quadword integer" },
	{ dtype_real, "single precision floating" },
	{ dtype_double, "double precision floating" },
	{ dtype_d_float, "double precision floating" },
	{ dtype_timestamp, "date" },
	{ dtype_sql_date, "SQL date" },
	{ dtype_sql_time, "SQL time" },
	{ dtype_blob, "blob" },
	{ dtype_int64, "big integer" },
	{ 0, 0 }
};


int MOVQ_compare(const dsc* arg1, const dsc* arg2)
{
/**************************************
 *
 *	M O V Q _ c o m p a r e
 *
 **************************************
 *
 * Functional description
 *	Compare two descriptors.  Return (-1, 0, 1) if a<b, a=b, or a>b.
 *
 **************************************/

	// Handle the simple (matched) ones first

	if (arg1->dsc_dtype == arg2->dsc_dtype && arg1->dsc_scale == arg2->dsc_scale)
	{
		const UCHAR* p1 = arg1->dsc_address;
		const UCHAR* p2 = arg2->dsc_address;

		switch (arg1->dsc_dtype)
		{
		case dtype_short:
			if (*(SSHORT*) p1 == *(SSHORT*) p2)
				return 0;
			if (*(SSHORT*) p1 > *(SSHORT*) p2)
				return 1;
			return -1;

		case dtype_long:
		case dtype_sql_date:
		case dtype_sql_time:
			if (*(SLONG*) p1 == *(SLONG*) p2)
				return 0;
			if (*(SLONG*) p1 > *(SLONG*) p2)
				return 1;
			return -1;
		case dtype_int64:
       	case dtype_timestamp:
		case dtype_quad:
			if (((SLONG*) p1)[0] < ((SLONG*) p2)[0])
				return -1;
			if (((SLONG*) p1)[0] > ((SLONG*) p2)[0])
				return 1;
			if (((ULONG*) p1)[1] < ((ULONG*) p2)[1])
				return -1;
			if (((ULONG*) p1)[1] > ((ULONG*) p2)[1])
				return 1;
			return 0;

		case dtype_real:
			if (*(float*) p1 == *(float*) p2)
				return 0;
			if (*(float*) p1 > *(float*) p2)
				return 1;
			return -1;

		case dtype_double:
			if (*(double*) p1 == *(double*) p2)
				return 0;
			if (*(double*) p1 > *(double*) p2)
				return 1;
			return -1;

		case dtype_text:
			{
			    SSHORT length;
				if (arg1->dsc_length >= arg2->dsc_length)
				{
					if (length = arg2->dsc_length)
						do {
							if (*p1++ != *p2++)
								return (p1[-1] > p2[-1]) ? 1 : -1;
						} while (--length);
					if (length = arg1->dsc_length - arg2->dsc_length)
						do {
							if (*p1++ != ' ')
								return (p1[-1] > ' ') ? 1 : -1;
						} while (--length);
					return 0;
				}
				if (length = arg1->dsc_length)
					do {
						if (*p1++ != *p2++)
							return (p1[-1] > p2[-1]) ? 1 : -1;
					} while (--length);
				length = arg2->dsc_length - arg1->dsc_length;
				do {
					if (*p2++ != ' ')
						return (' ' > p2[-1]) ? 1 : -1;
				} while (--length);
				return 0;
			}
		}
	}

	// Handle mixed string comparisons

	if (arg1->dsc_dtype <= dtype_varying && arg2->dsc_dtype <= dtype_varying)
	{
	    const TEXT* p1;
	    const TEXT* p2;
		SSHORT length = MOVQ_get_string(arg1, &p1, 0, 0);
		SSHORT length2 = MOVQ_get_string(arg2, &p2, 0, 0);
		SSHORT fill = length - length2;
		if (length >= length2)
		{
			if (length2)
				do {
					if (*p1++ != *p2++)
						return (p1[-1] > p2[-1]) ? 1 : -1;
				} while (--length2);
			if (fill > 0)
				do {
					if (*p1++ != ' ')
						return (p1[-1] > ' ') ? 1 : -1;
				} while (--fill);
			return 0;
		}
		if (length)
		{
			do {
				if (*p1++ != *p2++)
					return (p1[-1] > p2[-1]) ? 1 : -1;
			} while (--length);
		}
		do {
			if (*p2++ != ' ')
				return (' ' > p2[-1]) ? 1 : -1;
		} while (++fill);
		return 0;
	}

	// Handle hetergeneous compares

	if (arg1->dsc_dtype < arg2->dsc_dtype)
		return (-MOVQ_compare(arg2, arg1));

	dsc desc;
	SLONG date[2];

	switch (arg1->dsc_dtype)
	{
	case dtype_timestamp:
		desc.dsc_dtype = dtype_timestamp;
		desc.dsc_length = sizeof(date);
		desc.dsc_scale = 0;
		desc.dsc_address = (UCHAR*) date;
		desc.dsc_sub_type = 0;
		MOVQ_move(arg2, &desc);
		return MOVQ_compare(arg1, &desc);

	case dtype_sql_time:
		desc.dsc_dtype = dtype_sql_time;
		desc.dsc_length = sizeof(date[0]);
		desc.dsc_scale = 0;
		desc.dsc_address = (UCHAR*) date;
		desc.dsc_sub_type = 0;
		MOVQ_move(arg2, &desc);
		return MOVQ_compare(arg1, &desc);

	case dtype_sql_date:
		desc.dsc_dtype = dtype_sql_date;
		desc.dsc_length = sizeof(date[0]);
		desc.dsc_scale = 0;
		desc.dsc_address = (UCHAR*) date;
		desc.dsc_sub_type = 0;
		MOVQ_move(arg2, &desc);
		return MOVQ_compare(arg1, &desc);

	case dtype_short:
	case dtype_long:
		{
			const SSHORT scale = MIN(arg1->dsc_scale, arg2->dsc_scale);

			const SLONG temp1 = MOVQ_get_long(arg1, scale);
			const SLONG temp2 = MOVQ_get_long(arg2, scale);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_real:
		{
			const float temp1 = MOVQ_get_double(arg1);
			const float temp2 = MOVQ_get_double(arg2);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_double:
		{
			const double temp1 = MOVQ_get_double(arg1);
			const double temp2 = MOVQ_get_double(arg2);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_blob:
		IBERROR(48);			// Msg 48 Blob conversion is not supported

	default:
		mover_error(410, arg1->dsc_dtype, arg2->dsc_dtype);

	}
	return -1;
}


double MOVQ_date_to_double(const dsc* desc)
{
/**************************************
 *
 *	M O V Q _ d a t e _ t o _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *    Convert a date to double precision for
 *    date arithmetic routines.
 *
 **************************************/
	SLONG temp[2], *date;

	// If the input descriptor is not in date form, convert it.

	if (desc->dsc_dtype == dtype_timestamp)
		date = (SLONG *) desc->dsc_address;
	else
	{
	    dsc temp_desc;
		temp_desc.dsc_dtype = dtype_timestamp;
		temp_desc.dsc_length = sizeof(temp);
		temp_desc.dsc_scale = 0;
		temp_desc.dsc_sub_type = 0;
		date = temp;
		temp_desc.dsc_address = (UCHAR*) date;
		QLI_validate_desc(temp_desc);
		MOVQ_move(desc, &temp_desc);
	}

	return date[0] + (double) date[1] / (24. * 60. * 60. * PRECISION);
}


int MOVQ_decompose(const TEXT* string, USHORT length, SLONG* return_value)
{
/**************************************
 *
 *	M O V Q _ d e c o m p o s e
 *
 **************************************
 *
 * Functional description
 *	Decompose a numeric string in mantissa and exponent.
 *
 **************************************/
	SSHORT scale = 0;
	SLONG value = 0;
	SCHAR temp[128];

	bool sign = false;
	bool fraction = false;

	const TEXT* p = string;
	const TEXT* const end = p + length;
	for (; p < end; p++)
	{
		if (*p == ',')
			continue;

		if (DIGIT(*p))
		{
			value = value * 10 + *p - '0';
			if (fraction)
				--scale;
		}
		else if (*p == '.')
		{
			if (fraction)
			{
				MOVQ_terminate(string, temp, length, sizeof(temp));
				ERRQ_error(411, temp);
			}
			else
				fraction = true;
		}
		else if (*p == '-' && !value && !sign)
			sign = true;
		else if (*p == '+' && !value && !sign)
			sign = false;
		else if (*p == 'e' || *p == 'E')
			break;
		else if (*p != ' ')
		{
			MOVQ_terminate(string, temp, length, sizeof(temp));
			ERRQ_error(411, temp);
		}
	}

	if (sign)
		value = -value;

	// If there's still something left, there must be an explicit exponent

	if (p < end)
	{
		SSHORT exp = 0;
		sign = false;
		for (p++; p < end; p++)
		{
			if (DIGIT(*p))
				exp = exp * 10 + *p - '0';
			else if (*p == '-' && !exp)
				sign = true;
			else if (*p == '+' && !exp && !sign)
				continue;
			else if (*p != ' ')
			{
				MOVQ_terminate(string, temp, length, sizeof(temp));
				ERRQ_error(411, temp);
			}
		}
		if (sign)
			scale -= exp;
		else
			scale += exp;
	}

	*return_value = value;

	return scale;
}


void MOVQ_double_to_date( double real, SLONG fixed[2])
{
/**************************************
 *
 *	M O V Q _ d o u b l e _ t o _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a double precision representation of a date
 *	to a fixed point representation.   Double is used for
 *      date arithmetic.
 *
 **************************************/

	fixed[0] = static_cast<SLONG>(real);
	fixed[1] = static_cast<SLONG>((real - fixed[0]) * 24. * 60. * 60. * PRECISION);
}


double MOVQ_get_double(const dsc* desc)
{
/**************************************
 *
 *	M O V Q _ g e t _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a
 *	double_precision representation.
 *
 **************************************/
	double value = 0;

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *((SSHORT*) desc->dsc_address);
		break;

	case dtype_long:
		value = *((SLONG*) desc->dsc_address);
		break;

	case dtype_int64:
		value = *((SINT64*) desc->dsc_address);
		break;

	case dtype_real:
		return *((float*) desc->dsc_address);

	case dtype_double:
		return *((double*) desc->dsc_address);

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		return double_from_text(desc);

	default:
		mover_error(410, desc->dsc_dtype, dtype_double);
	}

	// Last, but not least, adjust for scale

	int scale = desc->dsc_scale;
	if (scale == 0)
		return value;

	if (scale > 0)
		do {
			value *= 10.;
		} while (--scale);
	else
		do {
			value /= 10.;
		} while (++scale);

	return value;
}


SLONG MOVQ_get_long(const dsc* desc, SSHORT scale)
{
/**************************************
 *
 *	M O V Q _ g e t _ l o n g
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a long (32 bit) integer of given
 *	scale.
 *
 **************************************/
	SLONG value;
	double d;

	scale -= (SSHORT) desc->dsc_scale;

	const TEXT* p = (TEXT*) desc->dsc_address;
	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *((SSHORT*) p);
		break;

	case dtype_int64:
		value = *((SINT64*) p);
		break;

	case dtype_long:
		value = *((SLONG*) p);
		break;

	case dtype_real:
		d = *((float*) p);
		if (scale > 0)
			do {
				d /= 10.;
			} while (--scale);
		else if (scale < 0)
			do {
				d *= 10.;
			} while (++scale);
		if (d > 0)
			d += 0.5;
		else
			d -= 0.5;
		return (SLONG) d;

	case dtype_double:
		d = *((double*) p);
		if (scale > 0)
			do {
				d /= 10.;
			} while (--scale);
		else if (scale < 0)
			do {
				d *= 10.;
			} while (++scale);
		if (d > 0)
			d += 0.5;
		else
			d -= 0.5;
		return (SLONG) d;

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		{
			const SSHORT length = MOVQ_get_string(desc, &p, 0, 0);
			scale -= MOVQ_decompose(p, length, &value);
			break;
		}

	default:
		mover_error(410, desc->dsc_dtype, dtype_long);
	}

	// Last, but not least, adjust for scale

	if (scale == 0)
		return value;

	if (scale > 0)
	{
		if ((desc->dsc_dtype == dtype_short) || (desc->dsc_dtype == dtype_long))
		{
			int fraction = 0;
			do {
				if (scale == 1)
					fraction = value % 10;
				value /= 10;
			} while (--scale);
			if (fraction > 4)
				value++;
		}
		else
			do {
				value /= 10;
			} while (--scale);
	}
	else
		do {
			value *= 10;
		} while (++scale);

	return value;
}


int MOVQ_get_string(const dsc* desc, const TEXT** address, vary* temp, USHORT length)
{
/**************************************
 *
 *	M O V Q _  g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Get address and length of string, converting the value to
 *	string, if necessary.  The caller must provide a sufficiently
 *	large temporary.  The address of the resultant string is returned
 *	by reference.  Get_string returns the length of the string.
 *
 *	Note: If the descriptor is known to be a string type, the third
 *	argument (temp buffer) may be omitted.
 *
 **************************************/

	// If the value is already a string (fixed or varying), just return
	// the address and length.

	if (desc->dsc_dtype == dtype_text)
	{
		*address = (TEXT*) desc->dsc_address;
		return desc->dsc_length;
	}

	// Perhaps it a "C" type string?

	if (desc->dsc_dtype == dtype_cstring)
	{
		*address = (TEXT*) desc->dsc_address;
		return MIN(static_cast<int>(strlen((char*)desc->dsc_address)), desc->dsc_length - 1);
	}

	// No luck -- convert value to varying string.

	if (desc->dsc_dtype == dtype_varying)
	{
		vary* varying = (vary*) desc->dsc_address;
		*address = varying->vary_string;
		return varying->vary_length;
	}

	dsc temp_desc;
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR*) temp;
	temp_desc.dsc_scale = 0;
	temp_desc.dsc_dtype = dtype_varying;
	temp_desc.dsc_sub_type = ttype_ascii;
	QLI_validate_desc(temp_desc);
	MOVQ_move(desc, &temp_desc);
	*address = temp->vary_string;

	return temp->vary_length;
}


void MOVQ_move(const dsc* from, dsc* to)
{
/**************************************
 *
 *	M O V Q _ m o v e
 *
 **************************************
 *
 * Functional description
 *	Move (and possible convert) something to something else.
 *
 **************************************/
	USHORT length = from->dsc_length;
	UCHAR* p = to->dsc_address;
	const UCHAR* q = from->dsc_address;

	// If the datatypes and lengths are identical, just move the
	// stuff byte by byte.  Although this may seem slower than
	// optimal, it would cost more to find the fast move than the
	// fast move would gain.

	// if (((ALT_DSC*) from)->dsc_combined_type == ((ALT_DSC*) to)->dsc_combined_type)
   	if (DSC_EQUIV(from, to, false))
	{
		if (length)
			memcpy(p, q, length);

		return;
	}

	const TEXT* ptr;

	// Do data type by data type conversions.  Not all are supported,
	// and some will drop out for additional handling.

	switch (to->dsc_dtype)
	{
	case dtype_timestamp:
		switch (from->dsc_dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			length = MOVQ_get_string(from, &ptr, 0, 0);
			string_to_date((TEXT*) ptr, length, (SLONG*) to->dsc_address);
			return;
		case dtype_sql_date:
			((SLONG*) to->dsc_address)[0] = *(SLONG*) from->dsc_address;
			((SLONG*) to->dsc_address)[1] = 0;
			return;
		case dtype_sql_time:
			((SLONG*) to->dsc_address)[0] = 0;
			((SLONG*) to->dsc_address)[1] = *(SLONG*) from->dsc_address;
			return;
		}
		break;

	case dtype_sql_date:
		switch (from->dsc_dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
				SLONG date[2];
				length = MOVQ_get_string(from, &ptr, 0, 0);
				string_to_date((TEXT*) ptr, length, (SLONG*) date);
				((SLONG*) to->dsc_address)[0] = date[0];
			}
			return;
		case dtype_timestamp:
			((SLONG*) to->dsc_address)[0] = ((SLONG*) from->dsc_address)[0];
			return;
		case dtype_sql_time:
			// Error situation
			break;
		}
		break;

	case dtype_sql_time:
		switch (from->dsc_dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
				SLONG date[2];
				length = MOVQ_get_string(from, &ptr, 0, 0);
				string_to_time((TEXT*) ptr, length, (SLONG*) date);
				((SLONG*) to->dsc_address)[0] = date[1];
			}
			return;
		case dtype_timestamp:
			((SLONG*) to->dsc_address)[0] = ((SLONG*) from->dsc_address)[1];
			return;
		case dtype_sql_date:
			// Error situation
			break;
		}
		break;

	case dtype_text:
	case dtype_cstring:
	case dtype_varying:
		switch (from->dsc_dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
				length = MOVQ_get_string(from, &ptr, 0, 0);
				const TEXT* s = ptr;

				switch (to->dsc_dtype)
				{
				case dtype_text:
					{
						length = MIN(length, to->dsc_length);
						SSHORT fill = to->dsc_length - length;
						if (length)
							memcpy(p, s, length);
						if (fill > 0)
							memset(p + length, ' ', fill);
						return;
					}

				case dtype_cstring:
					length = MIN(length, to->dsc_length - 1);
					if (length)
						memcpy(p, s, length);
					p[length] = 0;
					return;

				case dtype_varying:
					{
						length = MIN(length, to->dsc_length - sizeof(SSHORT));
						vary* avary = reinterpret_cast<vary*>(p);
						avary->vary_length = length;
						if (length)
							memcpy(avary->vary_string, s, length);
					}
					return;
				}
			}

		case dtype_int64:
		case dtype_short:
		case dtype_long:
		case dtype_real:
		case dtype_double:
			numeric_to_text(from, to);
			return;

		case dtype_sql_date:
			sql_date_to_text((SLONG*) from->dsc_address, to);
			return;
		case dtype_sql_time:
			sql_time_to_text((ULONG*) from->dsc_address, to);
			return;
		case dtype_timestamp:
			timestamp_to_text((SLONG*) from->dsc_address, to);
			return;
		}
		break;

	case dtype_blob:
		if (from->dsc_dtype == dtype_quad)
		{
			((SLONG*) p)[0] = ((SLONG*) q)[0];
			((SLONG*) p)[1] = ((SLONG*) q)[1];
			return;
		}
		break;

	case dtype_quad:
		if (from->dsc_dtype == dtype_blob)
		{
			((SLONG*) p)[0] = ((SLONG*) q)[0];
			((SLONG*) p)[1] = ((SLONG*) q)[1];
			return;
		}
		break;

	case dtype_short:
		{
			const SLONG l = MOVQ_get_long(from, to->dsc_scale);
			*(SSHORT*) p = l;
			if (*(SSHORT*) p != l)
				IBERROR(14);		// Msg14 integer overflow
			return;
		}

	case dtype_long:
		*(SLONG*) p = MOVQ_get_long(from, to->dsc_scale);
		return;

	case dtype_int64:
		*(SINT64*) p = MOVQ_get_long(from, to->dsc_scale);
		return;

	case dtype_real:
		*(float*) p = MOVQ_get_double(from);
		return;

	case dtype_double:
		*(double*) p = MOVQ_get_double(from);
		return;
	}

	if (to->dsc_dtype == dtype_blob || from->dsc_dtype == dtype_blob)
		IBERROR(55);			// Msg 55 Blob conversion is not supported

	mover_error(410, from->dsc_dtype, to->dsc_dtype);
}


void MOVQ_terminate(const SCHAR* from, SCHAR* to, USHORT length, USHORT max_length)
{
/**************************************
 *
 *	M O V Q _ t e r m i n a t e
 *
 **************************************
 *
 * Functional description
 *	Null-terminate a possibly non-
 *	null-terminated string with max
 *	buffer room.
 *
 **************************************/

	fb_assert(max_length != 0);
	if (length)
	{
		length = MIN(length, max_length - 1);
		memcpy(to, from, length);
		to[length] = '\0';
	}
	else
		fb_utils::copy_terminate(to, from, max_length);
}


static void date_error(const TEXT* string, const USHORT length)
{
/**************************************
 *
 *	d a t e _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	A date conversion error occurred.  Complain.
 *
 **************************************/
	SCHAR temp[128];

	MOVQ_terminate(string, temp, length, sizeof(temp));
	ERRQ_error(56, temp);
	// Msg 56 Error converting string \"%s\" to date
}


static double double_from_text(const dsc* desc)
{
	const TEXT* p;
	const SSHORT length = MOVQ_get_string(desc, &p, 0, 0);
	int scale = 0;
	bool fraction = false, sign = false;
	double value = 0;
	const TEXT* const end = p + length;
	for (; p < end; p++)
	{
		if (*p == ',')
			continue;

		if (DIGIT(*p))
		{
			value = value * 10. + (*p - '0');
			if (fraction)
				scale++;
		}
		else if (*p == '.')
		{
			if (fraction)
				IBERROR(52);	// Msg 52 conversion error
			else
				fraction = true;
		}
		else if (!value && *p == '-')
			sign = true;
		else if (!value && *p == '+')
			continue;
		else if (*p == 'e' || *p == 'E')
			break;
		else if (*p != ' ')
			IBERROR(53);	// Msg 53 conversion error
	}

	if (sign)
		value = -value;

	// If there's still something left, there must be an explicit exponent

	if (p < end)
	{
		sign = false;
		int exp = 0;
		for (p++; p < end; p++)
		{
			if (DIGIT(*p))
				exp = exp * 10 + *p - '0';
			else if (*p == '-' && !exp)
				sign = true;
			else if (*p == '+' && !exp)
				;
			else if (*p != ' ')
				IBERROR(54);	// Msg 54 conversion error
		}
		if (sign)
			scale += exp;
		else
			scale -= exp;
	}

	if (scale > 0)
		do {
			value /= 10.;
		} while (--scale);
	else if (scale)
		do {
			value *= 10.;
		} while (++scale);

	return value;
}


static void sql_date_to_text( const SLONG date[1], DSC* to)
{
/**************************************
 *
 *	s q l _ d a t e _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *	Convert date to text.
 *
 **************************************/
	tm times;
	SLONG date2[2];

	date2[0] = date[0];
	date2[1] = 0;
	isc_decode_date((const ISC_QUAD*) date2, &times);

	TEXT temp[35];
	sprintf(temp, "%2d-%.3s-%04d", times.tm_mday,
			FB_LONG_MONTHS_UPPER[times.tm_mon], times.tm_year + 1900);

	const TEXT* p = temp;
	while (*p)
		++p;

	dsc desc;
	desc.dsc_length = p - temp;
	desc.dsc_address = (UCHAR*) temp;
	desc.dsc_dtype = dtype_text;
	desc.dsc_scale = 0;
	desc.dsc_sub_type = ttype_ascii;
	QLI_validate_desc(desc);

	MOVQ_move(&desc, to);
}


static void sql_time_to_text( const ULONG date[1], DSC* to)
{
/**************************************
 *
 *	s q l _ t i m e _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *	Convert sql time to text.
 *
 **************************************/
	tm times;
	SLONG date2[2];

	date2[0] = 0;
	date2[1] = date[0];

	isc_decode_date((const ISC_QUAD*) date2, &times);

	TEXT temp[35];
	sprintf(temp, " %2d:%.2d:%.2d.%.4"SLONGFORMAT, times.tm_hour, times.tm_min,
			times.tm_sec, date2[1] % PRECISION);

	const TEXT* p = temp;
	while (*p)
		++p;

	dsc desc;
	desc.dsc_length = p - temp;
	desc.dsc_address = (UCHAR*) temp;
	desc.dsc_dtype = dtype_text;
	desc.dsc_scale = 0;
	desc.dsc_sub_type = ttype_ascii;
	QLI_validate_desc(desc);

	MOVQ_move(&desc, to);
}


static void timestamp_to_text( const SLONG date[2], DSC* to)
{
/**************************************
 *
 *	t i m e s t a m p _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *	Convert date to text.
 *
 **************************************/
	tm times;
	isc_decode_date((const ISC_QUAD*) date, &times);

    TEXT temp[35];
	sprintf(temp, "%2d-%.3s-%04d", times.tm_mday,
			FB_LONG_MONTHS_UPPER[times.tm_mon], times.tm_year + 1900);

	if (times.tm_hour || times.tm_min || times.tm_sec || date[1])
	{
		TEXT time[15];
		sprintf(time, " %2d:%.2d:%.2d.%.4"SLONGFORMAT, times.tm_hour, times.tm_min,
				times.tm_sec, date[1] % PRECISION);
		strcat(temp, time);
	}

	const TEXT* p = temp;
	while (*p)
		++p;

	dsc desc;
	desc.dsc_length = p - temp;
	desc.dsc_address = (UCHAR*) temp;
	desc.dsc_dtype = dtype_text;
	desc.dsc_scale = 0;
	desc.dsc_sub_type = ttype_ascii;
	QLI_validate_desc(desc);

	MOVQ_move(&desc, to);
}


static void mover_error( int pattern, USHORT in_type, USHORT out_type)
{
/**************************************
 *
 *	m o v e r _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Return a reasonable error for
 *	unreasonable conversions and
 *	comparisons.
 *
 **************************************/
	TEXT in_name[25], out_name[25], msg_unknown[40];

	ERRQ_msg_get(504, msg_unknown, sizeof(msg_unknown));	// Msg504 unknown datatype %d

	const TEXT* in = type_name(in_type);
	if (!in)
	{
		in = in_name;
		sprintf(in_name, msg_unknown, in_type);
	}

	const TEXT* out = type_name(out_type);
	if (!out)
	{
		out = out_name;
		sprintf(out_name, msg_unknown, out_type);
	}

	ERRQ_error(pattern, SafeArg() << in << out);
}


static void now_to_date(const tm* time, SLONG date[2])
{
/**************************************
 *
 *	n o w _ t o _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert "now" (or best guess) to
 *	a date attempting to get millisecond
 *	precision.  This unfortunately means
 *	that we use one routine for VAX, one
 *	for Apollo, and a third for Unix.
 *
 **************************************/

	isc_encode_date(time, (ISC_QUAD*)date);
}


static void numeric_to_text(const dsc* from, dsc* to)
{
/**************************************
 *
 *	n u m e r i c _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *	Convert your basic number to nice, formatted text.
 *
 **************************************/

	// Save (or compute) scale of source.  Then convert source to ordinary longword.

	SSHORT scale = from->dsc_scale;

	SSHORT pad = 0, decimal = 0;
	if (scale > 0)
		pad = scale;
	else if (scale < 0)
		decimal = 1;

	SLONG n;
	dsc intermediate;
	intermediate.dsc_dtype = dtype_long;
	intermediate.dsc_length = sizeof(SLONG);
	intermediate.dsc_scale = scale;
	intermediate.dsc_sub_type = 0;
	intermediate.dsc_address = (UCHAR*) &n;
	QLI_validate_desc(intermediate);

	MOVQ_move(from, &intermediate);

	// Check for negation, then convert the number to a string of digits

	SSHORT neg = 0;
	if (n < 0)
	{
		neg = 1;
		n = -n;
	}

	TEXT temp[32];
	TEXT* p = temp;

	do {
		*p++ = n % 10 + '0';
		n /= 10;
	} while (n);

	// Compute the total length off the field formatted.  Make sure it
	// fits.  Keep in mind that routine handles both string and varying
	// string fields.

	SSHORT l = p - temp;
	const size_t length = l + neg + decimal + pad;

	if ((to->dsc_dtype == dtype_text && length > to->dsc_length) ||
		(to->dsc_dtype == dtype_cstring && length >= to->dsc_length) ||
		(to->dsc_dtype == dtype_varying && length > to->dsc_length - sizeof(SSHORT)))
	{
		IBERROR(57);			// Msg 57 overflow during conversion
	}

	// Hopefully a cstring never reached this point.
	TEXT* q = (TEXT*) ((to->dsc_dtype == dtype_text) ?
		to->dsc_address : to->dsc_address + sizeof(SSHORT));

	// If negative, put in minus sign

	if (neg)
		*q++ = '-';

	// If a decimal point is required, do the formatting.  Otherwise just
	// copy number

	if (scale < 0)
	{
		if ((l += scale) > 0)
			do {
				*q++ = *--p;
			} while (--l);
		*q++ = '.';
		do {
			*q++ = *--p;
		} while (++scale);
	}
	else
		do {
			*q++ = *--p;
		} while (--l);

	// If padding is required, do it now.

	if (pad)
		do {
			*q++ = '0';
		} while (--pad);

	// Finish up by padding (if fixed) or computing the actual length
	// (varying string)

	if (to->dsc_dtype == dtype_text)
	{
		if (l = to->dsc_length - length)
			do {
				*q++ = ' ';
			} while (--l);
		return;
	}

	if (to->dsc_dtype == dtype_cstring)
	{
		*q = 0;
		return;
	}

	*(SSHORT*) (to->dsc_address) = (UCHAR*) q - to->dsc_address - sizeof(SSHORT);
}


static void string_to_date(const TEXT* string, USHORT length, SLONG date[2])
{
/**************************************
 *
 *	s t r i n g _ t o _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert an arbitrary string to a date.
 *
 **************************************/
	if (!length)
	{
		date[0] = date[1] = 0;
		return;
	}

	const TEXT* p = string;
	const TEXT* const end = p + length;

	USHORT month_position = 0;

	const time_t clock = time(0);
	tm* today = localtime(&clock);

	USHORT components[7];
	for (int i = 0; i < 7; i++)
		components[i] = 0;

	// Parse components

	TEXT temp[15];
	USHORT n, precision;
	bool year = false;

	for (int i = 0; i < 7; i++)
	{

		// Skip leading blanks.  If we run out of characters, we're done with parse.

		while (p < end && *p == ' ')
			p++;
		if (p == end)
			break;

		// Handle digit or character strings

		TEXT c = UPPER(*p);
		if (DIGIT(c))
		{
			precision = n = 0;
			while (p < end && DIGIT(*p))
			{
				n = n * 10 + *p++ - '0';
				precision++;
			}
			if (i == 2)
				year = true;
		}
		else if (LETTER(c))
		{
			TEXT* t = temp;
			while (p < end && LETTER(c))
			{
				c = UPPER(*p);
				if (!LETTER(c))
					break;
				*t++ = c;
				p++;
			}
			*t = 0;
			const TEXT* const* month_ptr = FB_LONG_MONTHS_UPPER;
			while (true)
			{
				if (!*month_ptr)
				{
					while (++p < end)
						if (*p != ' ' && *p != '\t' && *p != 0)
							date_error(string, length);

					if (strcmp(temp, NOW) == 0)
					{
						now_to_date(today, date);
						return;
					}
					today->tm_hour = today->tm_min = today->tm_sec = 0;
					isc_encode_date(today, (ISC_QUAD*)date);
					if (strcmp(temp, TODAY) == 0)
						return;
					if (strcmp(temp, TOMORROW) == 0)
					{
						++date[0];
						return;
					}
					if (strcmp(temp, YESTERDAY) == 0)
					{
						--date[0];
						return;
					}
					date_error(string, length);
				}
				t = temp;
				for (const TEXT* m = *month_ptr++; *t && *t == *m; t++, m++);
				if (!*t)
					break;
			}
			n = month_ptr - FB_LONG_MONTHS_UPPER;
			month_position = i;
		}
		else
		{
			date_error(string, length);
			return;
		}
		components[i] = n;
		while (p < end && *p == ' ')
			p++;

		switch (*p)
		{
		case '.':
			if (!month_position && i < 2)
				month_position = 1;
			// fall into
		case '/':
		case '-':
		case ',':
		case ':':
			p++;
			continue;
		}
	}

	// Slide things into day, month, year form

	tm times;
	if (month_position)
	{
		times.tm_mon = components[1];
		times.tm_mday = components[0];
	}
	else
	{
		times.tm_mon = components[0];
		times.tm_mday = components[1];
	}

	// Handle defaulting of year

	if (((times.tm_year = components[2]) == 0) && !year)
		times.tm_year = today->tm_year + 1900;
	else if (times.tm_year < 100)
	{
		if (times.tm_year < (today->tm_year - 50) % 100)
			times.tm_year += 2000;
		else
			times.tm_year += 1900;
	}

	times.tm_year -= 1900;
	times.tm_mon -= 1;
	times.tm_hour = components[3];
	times.tm_min = components[4];
	times.tm_sec = components[5];

	// convert day/month/year to Julian and validate result

	isc_encode_date(&times, (ISC_QUAD*)date);
	tm times2;
	isc_decode_date((ISC_QUAD*)date, &times2);

	if (times.tm_year != times2.tm_year ||
		times.tm_mon != times2.tm_mon || times.tm_mday != times2.tm_mday)
	{
		date_error(string, length);
	}

	while (precision++ < 4)
		components[6] *= 10;

	date[1] += components[6];

}


static void string_to_time(const TEXT* string, USHORT length, SLONG date[2])
{
/**************************************
 *
 *	s t r i n g _ t o _ t i m e
 *
 **************************************
 *
 * Functional description
 *	Convert an arbitrary string to a time.
 *
 **************************************/
	if (!length)
	{
		date[0] = date[1] = 0;
		return;
	}

	const TEXT* p = string;
	const TEXT* const end = p + length;

	const time_t clock = time(0);
	const tm* today = localtime(&clock);

	int i;
	USHORT components[7];
	for (i = 0; i < 7; i++)
		components[i] = 0;

	// Parse components

	TEXT temp[15];
	USHORT n, precision;

	for (i = 3; i < 7; i++)
	{

		// Skip leading blanks.  If we run out of characters, we're done with parse.

		while (p < end && *p == ' ')
			p++;
		if (p == end)
			break;

		// Handle digit or character strings

		TEXT c = UPPER(*p);
		if (DIGIT(c))
		{
			precision = n = 0;
			while (p < end && DIGIT(*p))
			{
				n = n * 10 + *p++ - '0';
				precision++;
			}
		}
		else if (LETTER(c))
		{
			TEXT* t = temp;
			while (p < end && LETTER(c))
			{
				c = UPPER(*p);
				if (!LETTER(c))
					break;
				*t++ = c;
				p++;
			}
			*t = 0;
			while (++p < end)
			{
				if (*p != ' ' && *p != '\t' && *p != 0)
					date_error(string, length);
			}

			if (strcmp(temp, NOW) == 0)
			{
				now_to_date(today, date);
				return;
			}
			date_error(string, length);
		}
		else
		{
			date_error(string, length);
			return;
		}
		components[i] = n;
		while (p < end && *p == ' ')
			p++;

		if (*p == '/' || *p == '-' || *p == ',' || *p == ':' || *p == '.')
		{
			p++;
			continue;
		}

	}

	tm times;
	times.tm_hour = components[3];
	times.tm_min = components[4];
	times.tm_sec = components[5];

	// convert day/month/year to Julian and validate result

	isc_encode_date(&times, (ISC_QUAD*)date);

	while (precision++ < 4)
		components[6] *= 10;

	date[1] += components[6];

}


static const TEXT* type_name( USHORT dtype)
{
/**************************************
 *
 *	t y p e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Return the name of a data type.
 *
 **************************************/
	for (const dtypes_t* names = dtypes_table; names->description; names++)
	{
		if (names->type == dtype)
			return names->description;
	}
	return NULL;
}
