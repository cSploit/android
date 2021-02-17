/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		picstr.cpp
 *	DESCRIPTION:	Picture String Handler
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
#include "../qli/exe.h"
#include "../qli/format.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/picst_proto.h"
#include "../qli/mov_proto.h"
#include "../common/classes/timestamp.h"
#include "../common/classes/VaryStr.h"
#include "../yvalve/gds_proto.h"
#include "../common/classes/FpeControl.h"

const int PRECISION	= 10000;

static TEXT* cvt_to_ascii(SLONG, TEXT*, int);
static const TEXT* default_edit_string(const dsc*, TEXT*);
static void edit_alpha(const dsc*, pics*, TEXT**, USHORT);
static void edit_date(const dsc*, pics*, TEXT**);
static void edit_float(const dsc*, pics*, TEXT**);
static void edit_numeric(const dsc*, pics*, TEXT**);
static int generate(pics*);
static void literal(pics*, TEXT, TEXT**);

static const TEXT* alpha_weekdays[] =
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static const TEXT* alpha_months[] =
{
	"January",
	"February",
	"March",
	"April",
	"May",
    "June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};


pics* PIC_analyze(const TEXT* string, const dsc* desc)
{
/**************************************
 *
 *	P I C _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *	Analyze a picture in preparation for formatting.
 *
 **************************************/
	if (!string)
	{
		if (!desc)
			return NULL;

		string = default_edit_string(desc, NULL);
	}

	pics* picture = (pics*) ALLOCD(type_pic);
	picture->pic_string = picture->pic_pointer = string;

	// Make a first pass just counting characters

	bool debit = false;
	TEXT c;
	while ((c = generate(picture)) && c != '?')
	{

		c = UPPER(c);
		switch (c)
		{
		case 'X':
		case 'A':
			++picture->pic_chars;
			break;

		case '9':
		case 'Z':
		case '*':
			// Count all digits;
			// count them as fractions only after a decimal pt and
			// before an E
			++picture->pic_digits;
			if (picture->pic_decimals && !picture->pic_exponents)
				++picture->pic_fractions;
			break;

		case 'H':
			++picture->pic_hex_digits;
			break;

		case '.':
			++picture->pic_decimals;
			break;

		case '-':
		case '+':
			picture->pic_flags |= PIC_signed;
		case '$':
			if (picture->pic_chars || picture->pic_exponents)
				++picture->pic_literals;
			else if (picture->pic_floats)
				++picture->pic_digits;
			else
				++picture->pic_floats;
			break;

		case 'M':
			++picture->pic_months;
			break;

		case 'D':
			// DB is ambiguous, could be Day Blank or DeBit...
			if (UPPER(*picture->pic_pointer) == 'B')
			{
				++picture->pic_pointer;
				++picture->pic_literals;
				debit = true;
			}
			++picture->pic_days;
			break;

		case 'Y':
			++picture->pic_years;
			break;

		case 'J':
			++picture->pic_julians;
			break;

		case 'W':
			++picture->pic_weekdays;
			break;

		case 'N':
			++picture->pic_nmonths;
			break;

		case 'E':
			++picture->pic_exponents;
			break;

		case 'G':
			++picture->pic_float_digits;
			break;

		case '(':
		case ')':
			picture->pic_flags |= PIC_signed;
			++picture->pic_brackets;
			break;

		case '\'':
		case '"':
			picture->pic_flags |= PIC_literal;
			{
				TEXT d;
				// Shouldn't UPPER be used on d before comparing with c?
				while ((d = generate(picture)) && d != c)
					++picture->pic_literals;
			}
			picture->pic_flags &= ~PIC_literal;
			break;

		case '\\':
			++picture->pic_literals;
			picture->pic_flags |= PIC_literal;
			generate(picture);
			picture->pic_flags &= ~PIC_literal;
			break;

		case 'C':
		case 'R':
			picture->pic_flags |= PIC_signed;
			++picture->pic_brackets;
			if (generate(picture))
				++picture->pic_brackets;
			else
				++picture->pic_count;
			break;

		case 'P':
			++picture->pic_meridian;
			break;

		case 'T':
			if (picture->pic_hours < 2)
				++picture->pic_hours;
			else if (picture->pic_minutes < 2)
				++picture->pic_minutes;
			else
				++picture->pic_seconds;
			break;

		case 'B':
		case '%':
		case ',':
		case '/':
		default:
			++picture->pic_literals;
			break;
		}
	}

	if (c == '?')
		picture->pic_missing = PIC_analyze(picture->pic_pointer, 0);

	// if a DB showed up, and the string is numeric, treat the DB as DeBit

	if (debit && (picture->pic_digits || picture->pic_hex_digits))
	{
		--picture->pic_days;
		--picture->pic_literals;
		picture->pic_flags |= PIC_signed;
		++picture->pic_brackets;
		++picture->pic_brackets;
	}

	picture->pic_print_length =
		picture->pic_digits +
		picture->pic_hex_digits +
		picture->pic_chars +
		picture->pic_floats +
		picture->pic_literals +
		picture->pic_decimals +
		picture->pic_months + picture->pic_days + picture->pic_weekdays + picture->pic_years +
		picture->pic_nmonths + picture->pic_julians +
		picture->pic_brackets +
		picture->pic_exponents +
		picture->pic_float_digits +
		picture->pic_hours + picture->pic_minutes + picture->pic_seconds +
		picture->pic_meridian;


	if (picture->pic_missing)
	{
		picture->pic_length = MAX(picture->pic_print_length, picture->pic_missing->pic_print_length);
		picture->pic_missing->pic_length = picture->pic_length;
	}
	else
		picture->pic_length = picture->pic_print_length;

	if (picture->pic_days || picture->pic_weekdays || picture->pic_months || picture->pic_nmonths ||
		picture->pic_years || picture->pic_hours || picture->pic_julians)
	{
		picture->pic_type = pic_date;
	}
	else if (picture->pic_exponents || picture->pic_float_digits)
		picture->pic_type = pic_float;
	else if (picture->pic_digits || picture->pic_hex_digits)
		picture->pic_type = pic_numeric;
	else
		picture->pic_type = pic_alpha;

	return picture;
}


void PIC_edit(const dsc* desc, pics* picture, TEXT** output, USHORT max_length)
{
/**************************************
 *
 *	P I C _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Edit data from a descriptor through an edit string to a running
 *	output pointer.  For text strings, check that we don't overflow
 *	the output buffer.
 *
 **************************************/

	switch (picture->pic_type)
	{
	case pic_alpha:
		edit_alpha(desc, picture, output, max_length);
		return;
	case pic_numeric:
		edit_numeric(desc, picture, output);
		return;
	case pic_date:
		edit_date(desc, picture, output);
		return;
	case pic_float:
		edit_float(desc, picture, output);
		return;
	default:
		ERRQ_bugcheck(68);			// Msg 68 PIC_edit: class not yet implemented
	}
}


void PIC_missing( qli_const* constant, pics* picture)
{
/**************************************
 *
 *	P I C _ m i s s i n g
 *
 **************************************
 *
 * Functional description
 *	Create a literal picture string from
 *	a descriptor for a missing value so
 *	we can print the missing value
 *
 **************************************/

	const dsc* desc = &constant->con_desc;

	const int l = MAX(desc->dsc_length, picture->pic_length);

	qli_str* scratch = (qli_str*) ALLOCDV(type_str, l + 3);
	TEXT* p = scratch->str_data;
	*p++ = '\"';

	PIC_edit(desc, picture, &p, l);
	*p++ = '\"';
	*p = 0;

    pics* missing_picture = PIC_analyze(scratch->str_data, desc);
	picture->pic_missing = missing_picture;
	picture->pic_length = MAX(picture->pic_print_length, missing_picture->pic_print_length);
	missing_picture->pic_length = picture->pic_length;
}


static TEXT* cvt_to_ascii( SLONG number, TEXT* pointer, int length)
{
/**************************************
 *
 *	c v t _ t o _ a s c i i
 *
 **************************************
 *
 * Functional description
 *	Convert a number to a number of ascii digits (plus terminating
 *	null), updating pointer.
 *
 **************************************/
	pointer += length + 1;
	TEXT* p = pointer;
	*--p = 0;

	while (--length >= 0)
	{
		*--p = (number % 10) + '0';
		number /= 10;
	}

	return pointer;
}


static const TEXT* default_edit_string(const dsc* desc, TEXT* buff)
{
/**************************************
 *
 *	d e f a u l t _ e d i t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Given a skeletal descriptor, generate a default edit string.
 *
 **************************************/
	TEXT buffer[32];

	if (!buff)
		buff = buffer;

	const SSHORT scale = desc->dsc_scale;

	switch (desc->dsc_dtype)
	{
	case dtype_text:
		sprintf(buff, "X(%d)", desc->dsc_length);
		break;

	case dtype_cstring:
		sprintf(buff, "X(%d)", desc->dsc_length - 1);
		break;

	case dtype_varying:
		sprintf(buff, "X(%d)", desc->dsc_length - 2);
		break;

	case dtype_short:
		if (!scale)
			return "-(5)9";
		if (scale < 0 && scale > -5)
			sprintf(buff, "-(%d).9(%d)", 6 + scale, -scale);
		else if (scale < 0)
			sprintf(buff, "-.9(%d)", -scale);
		else
			sprintf(buff, "-(%d)9", 5 + scale);
		break;

	case dtype_long:
		if (!scale)
			return "-(10)9";
		if (scale < 0 && scale > -10)
			sprintf(buff, "-(%d).9(%d)", 10 + scale, -scale);
		else if (scale < 0)
			sprintf(buff, "-.9(%d)", -scale);
		else
			sprintf(buff, "-(%d)9", 11 + scale);
		break;

	case dtype_int64:
        // replace 16 with 20 later
		// (as soon as I have sorted out the rounding issues)
		// FSG
		if (!scale)
			return "-(16)9";
		if (scale < 0 && scale > -16)
			sprintf(buff, "-(%d).9(%d)", 16 + scale, -scale);
		else if (scale < 0)
			sprintf(buff, "-.9(%d)", -scale);
		else
			sprintf(buff, "-(%d)9", 17 + scale);
		break;

	case dtype_sql_date:
	case dtype_timestamp:
		return "DD-MMM-YYYY";

	case dtype_sql_time:
		return "TT:TT:TT.TTTT";

	case dtype_real:
		return "G(8)";

	case dtype_double:
		return "G(16)";

	default:
		return "X(11)";
	}

	if (buff == buffer)
	{
		qli_str* string = (qli_str*) ALLOCDV(type_str, strlen(buff));
		strcpy(string->str_data, buff);
		buff = string->str_data;
	}

	return buff;
}


static void edit_alpha(const dsc* desc,
					   pics* picture, TEXT** output, USHORT max_length)
{
/**************************************
 *
 *	e d i t _ a l p h a
 *
 **************************************
 *
 * Functional description
 *	Edit data from a descriptor through an edit string to a running
 *	output pointer.
 *
 **************************************/
	Firebird::VaryStr<512> temp;

	const TEXT* p = NULL;
	const USHORT l = MOVQ_get_string(desc, &p, &temp, sizeof(temp));
	const TEXT* const end = p + l;
	picture->pic_pointer = picture->pic_string;
	picture->pic_count = 0;
	TEXT* out = *output;

	while (p < end)
	{
		if ((out - *output) >= max_length)
			break;
		TEXT c = generate(picture);
		if (!c || c == '?')
			break;

		c = UPPER(c);

		switch (c)
		{
		case 'X':

			*out++ = *p++;
			break;

		case 'A':
			if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
				*out++ = *p++;
			else
				IBERROR(69);	// Msg 69 conversion error
			break;

		case 'B':
			*out++ = ' ';
			break;

		case '"':
		case '\'':
		case '\\':
			literal(picture, c, &out);
			break;

		default:
			*out++ = c;
			break;
		}
	}

	*output = out;
}


static void edit_date( const dsc* desc, pics* picture, TEXT** output)
{
/**************************************
 *
 *	e d i t _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Edit data from a descriptor through an edit string to a running
 *	output pointer.
 *
 **************************************/
	SLONG date[2];
	DSC temp_desc;
	TEXT d, temp[256];

	temp_desc.dsc_dtype = dtype_timestamp;
	temp_desc.dsc_scale = 0;
	temp_desc.dsc_sub_type = 0;
	temp_desc.dsc_length = sizeof(date);
	temp_desc.dsc_address = (UCHAR*) date;
	QLI_validate_desc(temp_desc);
	MOVQ_move(desc, &temp_desc);

    tm times;
	isc_decode_date((ISC_QUAD*) date, &times);
	TEXT* p = temp;

	const TEXT* nmonth = p;
	p = cvt_to_ascii((SLONG) times.tm_mon + 1, p, picture->pic_nmonths);

	const TEXT* day = p;
	p = cvt_to_ascii((SLONG) times.tm_mday, p, picture->pic_days);

	const TEXT* year = p;
	p = cvt_to_ascii((SLONG) times.tm_year + 1900, p, picture->pic_years);

	const TEXT* julians = p;
	p = cvt_to_ascii((SLONG) times.tm_yday + 1, p, picture->pic_julians);

	const TEXT* meridian = "";
	if (picture->pic_meridian)
	{
		if (times.tm_hour >= 12)
		{
			meridian = "PM";
			if (times.tm_hour > 12)
				times.tm_hour -= 12;
		}
		else
			meridian = "AM";
	}

	const SLONG seconds = date[1] % (60 * PRECISION);

	TEXT* hours = p;
	p = cvt_to_ascii((SLONG) times.tm_hour, p, picture->pic_hours);
	p = cvt_to_ascii((SLONG) times.tm_min, --p, picture->pic_minutes);
	p = cvt_to_ascii((SLONG) seconds, --p, 6);

	if (*hours == '0')
		*hours = ' ';

	SLONG rel_day = (date[0] + 3) % 7;
	if (rel_day < 0)
		rel_day += 7;
	const TEXT* weekday = alpha_weekdays[rel_day];
	const TEXT* month = alpha_months[times.tm_mon];

	picture->pic_pointer = picture->pic_string;
	picture->pic_count = 0;
	TEXT* out = *output;

	bool sig_day = false;
	bool blank = true;

	for (;;)
	{
		TEXT c = generate(picture);
		if (!c || c == '?')
			break;
		c = UPPER(c);

		switch (c)
		{
		case 'Y':
			*out++ = *year++;
			break;

		case 'M':
			if (*month)
				*out++ = *month++;
			break;

		case 'N':
			*out++ = *nmonth++;
			break;

		case 'D':
			d = *day++;
			if (!sig_day && d == '0' && blank)
				*out++ = ' ';
			else
			{
				sig_day = true;
				*out++ = d;
			}
			break;

		case 'J':
			if (*julians)
				*out++ = *julians++;
			break;

		case 'W':
			if (*weekday)
				*out++ = *weekday++;
			break;

		case 'B':
			*out++ = ' ';
			break;

		case 'P':
			if (*meridian)
				*out++ = *meridian++;
			break;

		case 'T':
			if (*hours)
				*out++ = *hours++;
			break;

		case '"':
		case '\'':
		case '\\':
			literal(picture, c, &out);
			break;

		default:
			*out++ = c;
			break;
		}
		if (c != 'B')
			blank = false;
	}

	*output = out;
}


static void edit_float( const dsc* desc, pics* picture, TEXT** output)
{
/**************************************
 *
 *	e d i t _ f l o a t
 *
 **************************************
 *
 * Functional description
 *	Edit data from a descriptor through an edit string to a running
 *	output pointer.
 *
 **************************************/
	TEXT temp[512];
	USHORT l, width, decimal_digits, w_digits, f_digits;

	double number = MOVQ_get_double(desc);
	const bool negative = (number < 0);
	if (negative)
		number = -number;

	// If exponents are explicitly requested (E-format edit_string), generate them.
	// Otherwise, the rules are: if the number in f-format will fit into the allotted
	// space, print it in f-format; otherwise print it in e-format.
	// (G-format is untrustworthy.)

	if (isnan(number))
		sprintf(temp, "NaN");
	else if (isinf(number))
		sprintf(temp, "Infinity");
	else if (picture->pic_exponents)
	{
		width = picture->pic_print_length - picture->pic_floats - picture->pic_literals;
		decimal_digits = picture->pic_fractions;
		sprintf(temp, "%*.*e", width, decimal_digits, number);
	}
	else if (number == 0)
		sprintf(temp, "%.0f", number);
	else
	{
		width = picture->pic_float_digits - 1 + picture->pic_floats;
		f_digits = (width > 2) ? width - 2 : 0;
		sprintf(temp, "%.*f", f_digits, number);
		w_digits = strlen(temp);
		if (f_digits)
		{
			TEXT* p = temp + w_digits;	// find the end
			w_digits = w_digits - (f_digits + 1);
			while (*--p == '0')
				--f_digits;
			if (*p != '.')
				++p;
			*p = 0;				// move the end
		}
		if ((w_digits > width) || (!f_digits && w_digits == 1 && temp[0] == '0'))
		{
			// if the number doesn't fit in the default window, revert
			// to exponential notation; displaying the maximum number of
			// mantissa digits.

			if (number < 1e100)
				decimal_digits = (width > 6) ? width - 6 : 0;
			else
				decimal_digits = (width > 7) ? width - 7 : 0;
			sprintf(temp, "%.*e", decimal_digits, number);
		}
	}

	TEXT* p = temp;
	picture->pic_pointer = picture->pic_string;
	picture->pic_count = 0;
	TEXT* out = *output;

	for (l = picture->pic_length - picture->pic_print_length; l > 0; --l)
		*out++ = ' ';

	bool is_signed = false;

	for (;;)
	{
		const TEXT e = generate(picture);
		TEXT c = e;
		if (!c || c == '?')
			break;
		c = UPPER(c);

		switch (c)
		{
		case 'G':
			if (!is_signed)
			{
				if (negative)
					*out++ = '-';
				else
					*out++ = ' ';
				is_signed = true;
			}
			else if (*p)
				*out++ = *p++;
			break;

		case 'B':
			*out++ = ' ';
			break;

		case '"':
		case '\'':
		case '\\':
			literal(picture, c, &out);
			break;

		case '9':
		case 'Z':
			{
				if (!(*p) || *p > '9' || *p < '0')
					break;
				TEXT d = *p++;
				if (c == '9' && d == ' ')
					d = '0';
				else if (c == 'Z' && d == '0')
					d = ' ';
				*out++ = d;
			}
			break;

		case '.':
			*out++ = (*p == c) ? *p++ : c;
			break;

		case 'E':
			if (!*p)
				break;
			*out++ = e;
			if (UPPER(*p) == c)
				++p;
			break;

		case '+':
		case '-':
			if (!*p)
				break;
			if (*p != '+' && *p != '-')
			{
				if (is_signed)
					*out++ = c;
				else if (c == '-' && !negative)
					*out++ = ' ';
				else if (c == '+' && negative)
					*out++ = '-';
				else
					*out++ = c;
				is_signed = true;
			}
			else if (*p == '-' || c == '+')
				*out++ = *p++;
			else
			{
				*out++ = ' ';
				p++;
			}
			break;

		default:
			*out++ = c;
			break;
		}
	}

	*output = out;
}


static void edit_numeric(const dsc* desc, pics* picture, TEXT** output)
{
/**************************************
 *
 *	e d i t _ n u m e r i c
 *
 **************************************
 *
 * Functional description
 *	Edit data from a descriptor through an edit string to a running
 *	output pointer.
 *
 **************************************/
	bool overflow = false;

	TEXT* out = *output;

	double number = MOVQ_get_double(desc);
	const bool negative = (number < 0);
	if (negative)
	{
		number = -number;
		if (!(picture->pic_flags & PIC_signed))
			overflow = true;
	}

    SSHORT scale = picture->pic_fractions;
	if (scale)
	{
		if (scale < 0)
		{
			do {
				number /= 10.;
			} while (++scale);
		}
		else if (scale > 0)
		{
			do {
				number *= 10.;
			} while (--scale);
		}
	}

	TEXT temp[512];
	TEXT* p = temp;
	TEXT* digits = p;

	double check;
	USHORT power;
	if (picture->pic_digits && !overflow)
	{
		for (check = number, power = picture->pic_digits; power; --power)
			check /= 10.0;
		if (check >= 1)
			overflow = true;
		else
		{
			sprintf(digits, "%0*.0f", picture->pic_digits, number);
			p = digits + strlen(digits);
		}
	}

	picture->pic_pointer = picture->pic_string;
	bool hex_overflow = false;
	const TEXT* hex = 0;
	if (picture->pic_hex_digits)
	{
		hex = p;
		p += picture->pic_hex_digits;
		for (check = number, power = picture->pic_hex_digits; power; --power)
			check /= 16.0;
		if (check >= 1)
			hex_overflow = true;
		else
		{
			SLONG nh = static_cast<SLONG>(number);
			while (p-- > hex)
			{
				*p = "0123456789abcdef"[nh & 15];
				nh >>= 4;
			}
		}
	}

	for (USHORT l = picture->pic_length - picture->pic_print_length; l-- > 0;)
		*out++ = ' ';

	const SLONG n = (number + 0.5 < 1) ? 0 : 1;

	TEXT* float_ptr = NULL;
	TEXT float_char;
	TEXT d;
	bool signif = false;

	for (;;)
	{
		TEXT c = generate(picture);
		if (!c || c == '?')
			break;

		c = UPPER(c);
		if (overflow && c != 'H')
		{
			*out++ = '*';
			continue;
		}

		switch (c)
		{
		case '9':
			signif = true;
			*out++ = *digits++;
			break;

		case 'H':
			if (hex_overflow)
			{
				*out++ = '*';
				continue;
			}
		case '*':
		case 'Z':
			d = (c == 'H') ? *hex++ : *digits++;
			if (signif || d != '0')
			{
				*out++ = d;
				signif = true;
			}
			else
				*out++ = (c == '*') ? '*' : ' ';
			break;

		case '+':
		case '-':
		case '$':
			if (c == '+' && negative)
				c = '-';
			else if (c == '-' && !negative)
				c = ' ';
			float_char = c;
			if (!float_ptr)
			{
				float_ptr = out;
				*out++ = n ? c : ' ';
				break;
			}
			d = *digits++;
			if (signif || d != '0')
			{
				*out++ = d;
				signif = true;
				break;
			}
			*float_ptr = ' ';
			float_ptr = out;
			*out++ = n ? c : ' ';
			break;

		case '(':
		case ')':
			*out++ = negative ? c : ' ';
			break;

		case 'C':
		case 'D':
			d = generate(picture);
			if (negative)
			{
				*out++ = c;
				*out++ = UPPER(d);
			}
			else if (d)
			{
				*out++ = ' ';
				*out++ = ' ';
			}
			break;

		case '.':
			signif = true;
			*out++ = c;
			break;

		case 'B':
			*out++ = ' ';
			break;

		case '"':
		case '\'':
		case '\\':
			literal(picture, c, &out);
			break;

		case ',':
			if (signif)
				*out++ = c;
			else if (float_ptr)
			{
				*float_ptr = ' ';
				float_ptr = out;
				*out++ = float_char;
			}
			else
				*out++ = ' ';
			break;

		default:
			*out++ = c;
			break;
		}
		if ((picture->pic_flags & PIC_suppress_blanks) && out[-1] == ' ')
			--out;
	}

	*output = out;
}


static int generate( pics* picture)
{
/**************************************
 *
 *	g e n e r a t e
 *
 **************************************
 *
 * Functional description
 *	Generate the next character from a picture string.
 *
 **************************************/
	TEXT c;

	for (;;)
	{
		// If we're in a repeat, pass it back

		if (picture->pic_count > 0)
		{
			--picture->pic_count;
			return picture->pic_character;
		}

		// Get the next character.  If null, we're done

		c = *picture->pic_pointer++;

		// If we're in literal mode, just return the character

		if (picture->pic_flags & PIC_literal)
			break;

		// If the next character is also a paren, it is a debit indicating
		// bracket.  If so, swallow the second.

		if ((c == ')' || c == '(') && *picture->pic_pointer == c)
		{
			picture->pic_pointer++;
			return (picture->pic_character = c);
		}

		// If the character is null and not a repeat count, we're done

		if (!c || c != '(')
			break;

		// We're got a potential repeat count.  If real, extract it.

		const TEXT* p = picture->pic_pointer;
		while (*p >= '0' && *p <= '9')
			picture->pic_count = picture->pic_count * 10 + *p++ - '0';

		if (p == picture->pic_pointer)
		{
			c = '(';
			break;
		}

		if (*p == ')')
			++p;

		picture->pic_pointer = p;

		// Someone may have done something very stupid, like specify a repeat
		// count of zero.  It's too late, as we've already gen'd one instance
		// of the edit character -- but let's not access violate, shall we?

		if (picture->pic_count)
			--picture->pic_count;
	}

	return (picture->pic_character = c);
}


static void literal( pics* picture, TEXT c, TEXT** ptr)
{
/**************************************
 *
 *	l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Handle a literal string in a picture string.
 *
 **************************************/
	TEXT* p = *ptr;
	picture->pic_flags |= PIC_literal;

	TEXT d;
	if (c == '\\')
		*p++ = generate(picture);
	else
		while ((d = generate(picture)) && d != c)
			*p++ = d;

	*ptr = p;
	picture->pic_flags &= ~PIC_literal;
}


