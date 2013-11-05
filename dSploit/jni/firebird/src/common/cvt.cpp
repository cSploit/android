/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         cvt.cpp
 *      DESCRIPTION:    Data mover and converter and comparator, etc.
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - DELTA and IMP
 *
 * 2001.6.16 Claudio Valderrama: Wiped out the leading space in
 * cast(float_expr as char(n)) in dialect 1, reported in SF.
 * 2001.11.19 Claudio Valderrama: integer_to_text() should use the
 * source descriptor "from" to call conversion_error.
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2008.07.09 Alex Peshkoff - moved part of CVT code to common part of engine
 *							  this avoids ugly export of CVT_move
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gen/iberror.h"

#include "../jrd/gdsassert.h"
#include "../common/classes/timestamp.h"
#include "../common/cvt.h"
#include "../jrd/intl.h"
#include "../jrd/quad.h"
#include "../jrd/val.h"
#include "../common/classes/VaryStr.h"
#include "../common/classes/FpeControl.h"
#include "../jrd/dsc_proto.h"


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_FLOAT_H
#include <float.h>
#else
#define DBL_MAX_10_EXP          308
#endif


using namespace Firebird;

/* normally the following two definitions are part of limits.h
   but due to a compiler bug on Apollo casting LONG_MIN to be a
   double, these have to be defined as double numbers...

   Also, since SunOS4.0 doesn't want to play with the rest of
   the ANSI world, these definitions have to be included explicitly.
   So, instead of some including <limits.h> and others using these
   definitions, just always use these definitions (huh?) */

#define LONG_MIN_real   -2147483648.	// min decimal value of an "SLONG"
#define LONG_MAX_real   2147483647.	// max decimal value of an "SLONG"
#define LONG_MIN_int    -2147483648	// min integer value of an "SLONG"
#define LONG_MAX_int    2147483647	// max integer value of an "SLONG"

// It turns out to be tricky to write the INT64 versions of those constant in
// a way that will do the right thing on all platforms.  Here we go.

#define LONG_MAX_int64 ((SINT64) 2147483647)	// max int64 value of an SLONG
#define LONG_MIN_int64 (-LONG_MAX_int64 - 1)	// min int64 value of an SLONG

#define QUAD_MIN_real   -9223372036854775808.	// min decimal value of quad
#define QUAD_MAX_real   9223372036854775807.	// max decimal value of quad

#define QUAD_MIN_int    quad_min_int	// min integer value of quad
#define QUAD_MAX_int    quad_max_int	// max integer value of quad

#ifdef FLT_MAX
#define FLOAT_MAX FLT_MAX // Approx. 3.4e38 max float (32 bit) value
#else
#define FLOAT_MAX 3.402823466E+38F // max float (32 bit) value
#endif

#define LETTER7(c)      ((c) >= 'A' && (c) <= 'Z')
#define DIGIT(c)        ((c) >= '0' && (c) <= '9')
#define ABSOLUT(x)      ((x) < 0 ? -(x) : (x))

/* The expressions for SHORT_LIMIT, LONG_LIMIT, INT64_LIMIT and
 * QUAD_LIMIT return the largest value that permit you to multiply by
 * 10 without getting an overflow.  The right operand of the << is two
 * less than the number of bits in the type: one bit is for the sign,
 * and the other is because we divide by 5, rather than 10.  */

#define SHORT_LIMIT     ((1 << 14) / 5)
#define LONG_LIMIT      ((1L << 30) / 5)

// NOTE: The syntax for the below line may need modification to ensure
// the result of 1 << 62 is a quad

#define QUAD_LIMIT      ((((SINT64) 1) << 62) / 5)
#define INT64_LIMIT     ((((SINT64) 1) << 62) / 5)

#define TODAY           "TODAY"
#define NOW             "NOW"
#define TOMORROW        "TOMORROW"
#define YESTERDAY       "YESTERDAY"

#define CVT_COPY_BUFF(from, to, len) \
{if (len) {memcpy(to, from, len); from += len; to += len; len = 0;} }

enum EXPECT_DATETIME
{
	expect_timestamp,
	expect_sql_date,
	expect_sql_time
};

static void datetime_to_text(const dsc*, dsc*, Callbacks*);
static void float_to_text(const dsc*, dsc*, Callbacks*);
static void integer_to_text(const dsc*, dsc*, Callbacks*);
static void string_to_datetime(const dsc*, GDS_TIMESTAMP*, const EXPECT_DATETIME, ErrorFunction);
static SINT64 hex_to_value(const char*& string, const char* end);
static void localError(const Firebird::Arg::StatusVector&);

class DummyException {};


#ifndef NATIVE_QUAD
#ifndef WORDS_BIGENDIAN
static const SQUAD quad_min_int = { 0, SLONG_MIN };
static const SQUAD quad_max_int = { -1, SLONG_MAX };
#else
static const SQUAD quad_min_int = { SLONG_MIN, 0 };
static const SQUAD quad_max_int = { SLONG_MAX, -1 };
#endif
#endif


#if !defined (NATIVE_QUAD)
#include "../jrd/quad.cpp"
#endif

static const double eps_double = 1e-14;
static const double eps_float  = 1e-5;


static void float_to_text(const dsc* from, dsc* to, Callbacks* cb)
{
/**************************************
 *
 *      f l o a t _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *      Convert an arbitrary floating type number to text.
 *      To avoid messiness, convert first into a temp (fixed) then
 *      move to the destination.
 *      Never print more digits than the source type can actually
 *      provide: instead pad the output with blanks on the right if
 *      needed.
 *
 **************************************/
	double d;
	char temp[] = "-1.234567890123456E-300";

	const int to_len = DSC_string_length(to); // length of destination
	const int width = MIN(to_len, (int) sizeof(temp) - 1); // minimum width to print

	int precision;
	if (dtype_double == from->dsc_dtype)
	{
		precision = 16;			// minimum significant digits in a double
		d = *(double*) from->dsc_address;
	}
	else
	{
		fb_assert(dtype_real == from->dsc_dtype);
		precision = 8;			// minimum significant digits in a float
		d = (double) *(float*) from->dsc_address;
	}

	// If this is a double with non-zero scale, then it is an old-style
	// NUMERIC(15, -scale): print it in fixed format with -scale digits
	// to the right of the ".".

	// CVC: Here sprintf was given an extra space in the two formatting
	// masks used below, "%- #*.*f" and "%- #*.*g" but certainly with positive
	// quantities and CAST it yields an annoying leading space.
	// However, by getting rid of the space you get in dialect 1:
	// cast(17/13 as char(5))  => 1.308
	// cast(-17/13 as char(5)) => -1.31
	// Since this is inconsistent with dialect 3, see workaround at the tail
	// of this function.

	int chars_printed;			// number of characters printed
	if ((dtype_double == from->dsc_dtype) && (from->dsc_scale < 0))
		chars_printed = sprintf(temp, "%- #*.*f", width, -from->dsc_scale, d);
	else
		chars_printed = LONG_MAX_int;	// sure to be greater than to_len

	// If it's not an old-style numeric, or the f-format was too long for the
	// destination, try g-format with the maximum precision which makes sense
	// for the input type: if it fits, we're done.

	if (chars_printed > width)
	{
		const char num_format[] = "%- #*.*g";
		chars_printed = sprintf(temp, num_format, width, precision, d);

		// If the full-precision result is too wide for the destination,
		// reduce the precision and try again.

		if (chars_printed > width)
		{
			precision -= (chars_printed - width);

			// If we cannot print at least two digits, one on each side of the
			// ".", report an overflow exception.
			if (precision < 2)
				cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));

			chars_printed = sprintf(temp, num_format, width, precision, d);

			// It's possible that reducing the precision caused sprintf to switch
			// from f-format to e-format, and that the output is still too long
			// for the destination.  If so, reduce the precision once more.
			// This is certain to give a short-enough result.

			if (chars_printed > width)
			{
				precision -= (chars_printed - width);
				if (precision < 2)
					cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			    chars_printed = sprintf(temp, num_format, width, precision, d);
			}
		}
	}
	fb_assert(chars_printed <= width);

	// trim trailing spaces
	const char* p = strchr(temp + 1, ' ');
	if (p)
		chars_printed = p - temp;

	// Now move the result to the destination array.

	dsc intermediate;
	intermediate.dsc_dtype = dtype_text;
	intermediate.dsc_ttype() = ttype_ascii;
	// CVC: If you think this is dangerous, replace the "else" with a call to
	// MEMMOVE(temp, temp + 1, chars_printed) or something cleverer.
	// Paranoid assumption:
	// UCHAR is unsigned char as seen on jrd\common.h => same size.
	if (d < 0)
	{
		intermediate.dsc_address = reinterpret_cast<UCHAR*>(temp);
		intermediate.dsc_length = chars_printed;
	}
	else
	{
		if (!temp[0])
			temp[1] = 0;
		intermediate.dsc_address = reinterpret_cast<UCHAR*>(temp) + 1;
		intermediate.dsc_length = chars_printed - 1;
	}

	CVT_move_common(&intermediate, to, cb);
}


static void integer_to_text(const dsc* from, dsc* to, Callbacks* cb)
{
/**************************************
 *
 *      i n t e g e r _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *      Convert your basic binary number to
 *      nice, formatted text.
 *
 **************************************/
#ifndef NATIVE_QUAD
	// For now, this routine does not handle quadwords unless this is
	// supported by the platform as a native datatype.

	if (from->dsc_dtype == dtype_quad)
	{
		fb_assert(false);
		cb->err(Arg::Gds(isc_badblk));	// internal error
	}
#endif

	SSHORT pad_count = 0, decimal = 0, neg = 0;

	// Save (or compute) scale of source.  Then convert source to ordinary
	// longword or int64.

	SCHAR scale = from->dsc_scale;

	if (scale > 0)
		pad_count = scale;
	else if (scale < 0)
		decimal = 1;

	SINT64 n;
	dsc intermediate;
	MOVE_CLEAR(&intermediate, sizeof(intermediate));
	intermediate.dsc_dtype = dtype_int64;
	intermediate.dsc_length = sizeof(n);
	intermediate.dsc_scale = scale;
	intermediate.dsc_address = (UCHAR*) &n;

	CVT_move_common(from, &intermediate, cb);

	// Check for negation, then convert the number to a string of digits

	FB_UINT64 u;
	if (n >= 0)
		u = n;
	else
	{
		neg = 1;
		u = -n;
	}

    UCHAR temp[32];
    UCHAR* p = temp;

	do {
		*p++ = (UCHAR) (u % 10) + '0';
		u /= 10;
	} while (u);

	SSHORT l = p - temp;

	// if scale < 0, we need at least abs(scale)+1 digits, so add
	// any leading zeroes required.
	while (l + scale <= 0)
	{
		*p++ = '0';
		l++;
	}
	// postassertion: l+scale > 0
	fb_assert(l + scale > 0);

	// CVC: also, we'll check for buffer overflow directly.
	fb_assert(temp + sizeof(temp) >= p);

	// Compute the total length of the field formatted.  Make sure it
	// fits.  Keep in mind that routine handles both string and varying
	// string fields.

	const USHORT length = l + neg + decimal + pad_count;

	if ((to->dsc_dtype == dtype_text && length > to->dsc_length) ||
		(to->dsc_dtype == dtype_cstring && length >= to->dsc_length) ||
		(to->dsc_dtype == dtype_varying && length > (to->dsc_length - sizeof(USHORT))))
	{
	    CVT_conversion_error(from, cb->err);
	}

	UCHAR* q = (to->dsc_dtype == dtype_varying) ? to->dsc_address + sizeof(USHORT) : to->dsc_address;
	const UCHAR* start = q;

	// If negative, put in minus sign

	if (neg)
		*q++ = '-';

	// If a decimal point is required, do the formatting. Otherwise just copy number.

	if (scale >= 0)
	{
		do {
			*q++ = *--p;
		} while (--l);
	}
	else
	{
		l += scale;	// l > 0 (see postassertion: l + scale > 0 above)
		do {
			*q++ = *--p;
		} while (--l);
		*q++ = '.';
		do {
			*q++ = *--p;
		} while (++scale);
	}

	cb->validateLength(cb->getToCharset(to->getCharSet()), length, start, TEXT_LEN(to));

	// If padding is required, do it now.

	if (pad_count)
	{
		do {
			*q++ = '0';
		} while (--pad_count);
	}

	// Finish up by padding (if fixed) or computing the actual length (varying string).

	if (to->dsc_dtype == dtype_text)
	{
		ULONG trailing = ULONG(to->dsc_length) - length;
		if (trailing > 0)
		{
			CHARSET_ID chid = cb->getChid(to); // : DSC_GET_CHARSET(to);

			const char pad = chid == ttype_binary ? '\0' : ' ';
			memset(q, pad, trailing);
		}
		return;
	}

	if (to->dsc_dtype == dtype_cstring)
	{
		*q = 0;
		return;
	}

	// dtype_varying
	*(USHORT*) (to->dsc_address) = q - to->dsc_address - sizeof(SSHORT);
}


static void string_to_datetime(const dsc* desc,
							   GDS_TIMESTAMP* date,
							   const EXPECT_DATETIME expect_type, ErrorFunction err)
{
/**************************************
 *
 *      s t r i n g _ t o _ d a t e t i m e
 *
 **************************************
 *
 * Functional description
 *      Convert an arbitrary string to a date and/or time.
 *
 *      String must be formed using ASCII characters only.
 *      Conversion routine can handle the following input formats
 *      "now"           current date & time
 *      "today"         Today's date       0:0:0.0 time
 *      "tomorrow"      Tomorrow's date    0:0:0.0 time
 *      "yesterday"     Yesterday's date   0:0:0.0 time
 *      YYYY-MM-DD [HH:[Min:[SS.[Thou]]]]]
 *      MM:DD[:YY [HH:[Min:[SS.[Thou]]]]]
 *      DD.MM[:YY [HH:[Min:[SS.[Thou]]]]]
 *      Where:
 *        DD = 1  .. 31    (Day of month)
 *        YY = 00 .. 99    2-digit years are converted to the nearest year
 *		           in a 50 year range.  Eg: if this is 1996
 *                              96 ==> 1996
 *                              97 ==> 1997
 *                              ...
 *                              00 ==> 2000
 *                              01 ==> 2001
 *                              ...
 *                              44 ==> 2044
 *                              45 ==> 2045
 *                              46 ==> 1946
 *                              47 ==> 1947
 *                              ...
 *                              95 ==> 1995
 *                         If the current year is 1997, then 46 is converted
 *                         to 2046 (etc).
 *           = 100.. 5200  (Year)
 *        MM = 1  .. 12    (Month of year)
 *           = "JANUARY"... (etc)
 *        HH = 0  .. 23    (Hour of day)
 *       Min = 0  .. 59    (Minute of hour)
 *        SS = 0  .. 59    (Second of minute - LEAP second not supported)
 *      Thou = 0  .. 9999  (Fraction of second)
 *      HH, Min, SS, Thou default to 0 if missing.
 *      YY defaults to current year if missing.
 *      Note: ANY punctuation can be used instead of : (eg: / - etc)
 *            Using . (period) in either of the first two separation
 *            points will cause the date to be parsed in European DMY
 *            format.
 *            Arbitrary whitespace (space or TAB) can occur between
 *            components.
 *
 **************************************/

	// Values inside of description
	// > 0 is number of digits
	//   0 means missing
	// ENGLISH_MONTH for the presence of an English month name
	// SPECIAL       for a special date verb
	const int ENGLISH_MONTH	= -1;
	const int SPECIAL		= -2; // CVC: I see it set, but never tested.

	unsigned int position_year = 0;
	unsigned int position_month = 1;
	unsigned int position_day = 2;
	bool have_english_month = false;
	bool dot_separator_seen = false;
	VaryStr<100> buffer;			// arbitrarily large

	const char* p = NULL;
	const USHORT length = CVT_make_string(desc, ttype_ascii, &p, &buffer, sizeof(buffer), err);

	const char* const end = p + length;

	USHORT n, components[7];
	int description[7];
	memset(components, 0, sizeof(components));
	memset(description, 0, sizeof(description));

	// Parse components
	// The 7 components are Year, Month, Day, Hours, Minutes, Seconds, Thou
	// The first 3 can be in any order

	const int start_component = (expect_type == expect_sql_time) ? 3 : 0;
	int i;
	for (i = start_component; i < 7; i++)
	{

		// Skip leading blanks.  If we run out of characters, we're done
		// with parse.

		while (p < end && (*p == ' ' || *p == '\t'))
			p++;
		if (p == end)
			break;

		// Handle digit or character strings

		TEXT c = UPPER7(*p);
		if (DIGIT(c))
		{
			USHORT precision = 0;
			n = 0;
			while (p < end && DIGIT(*p))
			{
				n = n * 10 + *p++ - '0';
				precision++;
			}
			description[i] = precision;
		}
		else if (LETTER7(c) && !have_english_month)
		{
			TEXT temp[sizeof(YESTERDAY) + 1];

			TEXT* t = temp;
			while ((p < end) && (t < &temp[sizeof(temp) - 1]))
			{
				c = UPPER7(*p);
				if (!LETTER7(c))
					break;
				*t++ = c;
				p++;
			}
			*t = 0;

			// Insist on at least 3 characters for month names
			if (t - temp < 3)
			{
				CVT_conversion_error(desc, err);
				return;
			}

			const TEXT* const* month_ptr = FB_LONG_MONTHS_UPPER;
			while (true)
			{
				// Month names are only allowed in first 2 positions
				if (*month_ptr && i < 2)
				{
					t = temp;
					const TEXT* m = *month_ptr++;
					while (*t && *t == *m)
					{
						++t;
						++m;
					}
					if (!*t)
						break;
				}
				else
				{
					// it's not a month name, so it's either a magic word or
					// a non-date string.  If there are more characters, it's bad

					description[i] = SPECIAL;

					while (++p < end)
						if (*p != ' ' && *p != '\t' && *p != 0)
							CVT_conversion_error(desc, err);

					// fetch the current datetime
					*date = Firebird::TimeStamp::getCurrentTimeStamp().value();

					if (strcmp(temp, NOW) == 0)
						return;
					if (expect_type == expect_sql_time)
					{
						CVT_conversion_error(desc, err);
						return;
					}
					date->timestamp_time = 0;
					if (strcmp(temp, TODAY) == 0)
						return;
					if (strcmp(temp, TOMORROW) == 0)
					{
						date->timestamp_date++;
						return;
					}
					if (strcmp(temp, YESTERDAY) == 0)
					{
						date->timestamp_date--;
						return;
					}
					CVT_conversion_error(desc, err);
					return;
				}
			}
			n = month_ptr - FB_LONG_MONTHS_UPPER;
			position_month = i;
			description[i] = ENGLISH_MONTH;
			have_english_month = true;
		}
		else
		{
			// Not a digit and not a letter - must be punctuation
			CVT_conversion_error(desc, err);
			return;
		}

		components[i] = n;

		// Grab whitespace following the number
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;

		if (p == end)
			break;

		// Grab a separator character
		if (*p == '/' || *p == '-' || *p == ',' || *p == ':')
		{
			p++;
			continue;
		}
		if (*p == '.')
		{
			if (i <= 1)
				dot_separator_seen = true;
			p++;
			//continue;
		}
	}

	// User must provide at least 2 components
	if (i - start_component < 1)
	{
		CVT_conversion_error(desc, err);
		return;
	}

	// Dates cannot have a Time portion
	if (expect_type == expect_sql_date && i > 2)
	{
		CVT_conversion_error(desc, err);
		return;
	}

	// We won't allow random trash after the recognized string
	while (p < end)
	{
		if (*p != ' ' && *p != '\t' && p != 0)
		{
			CVT_conversion_error(desc, err);
			return;
		}
		++p;
	}

	tm times;
	memset(&times, 0, sizeof(times));

	if (expect_type != expect_sql_time)
	{
		// Figure out what format the user typed the date in

		if (description[0] >= 3)
		{
			// A 4 digit number to start implies YYYY-MM-DD
			position_year = 0;
			position_month = 1;
			position_day = 2;
		}
		else if (description[0] == ENGLISH_MONTH)
		{
			// An English month to start implies MM-DD-YYYY
			position_year = 2;
			position_month = 0;
			position_day = 1;
		}
		else if (description[1] == ENGLISH_MONTH)
		{
			// An English month in the middle implies DD-MM-YYYY
			position_year = 2;
			position_month = 1;
			position_day = 0;
		}
		else if (dot_separator_seen)
		{
			// A period as a separator implies DD.MM.YYYY
			position_year = 2;
			position_month = 1;
			position_day = 0;
		}
		else
		{
			// Otherwise assume MM-DD-YYYY
			position_year = 2;
			position_month = 0;
			position_day = 1;
		}

		// Forbid years with more than 4 digits
		// Forbid months or days with more than 2 digits
		// Forbid months or days being missing
		if (description[position_year] > 4 ||
			description[position_month] > 2 || description[position_month] == 0 ||
			description[position_day] > 2 || description[position_day] <= 0)
		{
			CVT_conversion_error(desc, err);
			return;
		}

		// Slide things into day, month, year form

		times.tm_year = components[position_year];
		times.tm_mon = components[position_month];
		times.tm_mday = components[position_day];

		// Fetch current date/time
		tm times2;
		Firebird::TimeStamp::getCurrentTimeStamp().decode(&times2);

		// Handle defaulting of year

		if (description[position_year] == 0) {
			times.tm_year = times2.tm_year + 1900;
		}
		else if (description[position_year] <= 2)
		{
			// Handle conversion of 2-digit years
			if (times.tm_year < (times2.tm_year - 50) % 100)
				times.tm_year += 2000;
			else
				times.tm_year += 1900;
		}

		times.tm_year -= 1900;
		times.tm_mon -= 1;
	}
	else
	{
		// The date portion isn't needed for time - but to
		// keep the conversion in/out of isc_time clean lets
		// initialize it properly anyway
		times.tm_year = 0;
		times.tm_mon = 0;
		times.tm_mday = 1;
	}

	// Handle time values out of range - note possibility of 60 for
	// seconds value due to LEAP second (Not supported in V4.0).
	if (i > 2 &&
		(((times.tm_hour = components[3]) > 23) ||
			((times.tm_min = components[4]) > 59) ||
			((times.tm_sec = components[5]) > 59) ||
			description[3] > 2 || description[3] == 0 ||
			description[4] > 2 || description[4] == 0 ||
			description[5] > 2 ||
			description[6] > -ISC_TIME_SECONDS_PRECISION_SCALE))
	{
		CVT_conversion_error(desc, err);
	}

	// convert day/month/year to Julian and validate result
	// This catches things like 29-Feb-1995 (not a leap year)

	Firebird::TimeStamp ts(times);

	if (!ts.isValid())
	{
		switch (expect_type)
		{
			case expect_sql_date:
				err(Arg::Gds(isc_date_range_exceeded));
				break;
			case expect_sql_time:
				err(Arg::Gds(isc_time_range_exceeded));
				break;
			case expect_timestamp:
				err(Arg::Gds(isc_datetime_range_exceeded));
				break;
			default: // this should never happen!
				CVT_conversion_error(desc, err);
				break;
		}
	}

	if (expect_type != expect_sql_time)
	{
		tm times2;
		ts.decode(&times2);

		if (times.tm_year != times2.tm_year ||
			times.tm_mon != times2.tm_mon ||
			times.tm_mday != times2.tm_mday ||
			times.tm_hour != times2.tm_hour ||
			times.tm_min != times2.tm_min ||
			times.tm_sec != times2.tm_sec)
		{
			CVT_conversion_error(desc, err);
		}
	}

	*date = ts.value();

	// Convert fraction of seconds
	while (description[6]++ < -ISC_TIME_SECONDS_PRECISION_SCALE)
		components[6] *= 10;

	date->timestamp_time += components[6];
}


SLONG CVT_get_long(const dsc* desc, SSHORT scale, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ g e t _ l o n g
 *
 **************************************
 *
 * Functional description
 *      Convert something arbitrary to a long (32 bit) integer of given
 *      scale.
 *
 **************************************/
	SLONG value, high;

	double d, eps;
	SINT64 val64;
	VaryStr<50> buffer;			// long enough to represent largest long in ASCII

	// adjust exact numeric values to same scaling

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *((SSHORT *) p);
		break;

	case dtype_long:
		value = *((SLONG *) p);
		break;

	case dtype_int64:
		val64 = *((SINT64 *) p);

		// adjust for scale first, *before* range-checking the value.
		if (scale > 0)
		{
			SLONG fraction = 0;
			do {
				if (scale == 1)
					fraction = SLONG(val64 % 10);
				val64 /= 10;
			} while (--scale);
			if (fraction > 4)
				val64++;
			// The following 2 lines are correct for platforms where
			// ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
			// a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
			// we'll have to change this depending on the platform.
			else if (fraction < -4)
				val64--;
		}
		else if (scale < 0)
		{
			do {
				if ((val64 > INT64_LIMIT) || (val64 < -INT64_LIMIT))
					err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
				val64 *= 10;
			} while (++scale);
		}

		if ((val64 > LONG_MAX_int64) || (val64 < LONG_MIN_int64))
			err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		return (SLONG) val64;

	case dtype_quad:
		value = ((SLONG *) p)[LOW_WORD];
		high = ((SLONG *) p)[HIGH_WORD];
		if ((value >= 0 && !high) || (value < 0 && high == -1))
			break;
		err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		break;

	case dtype_real:
	case dtype_double:
		if (desc->dsc_dtype == dtype_real)
		{
			d = *((float*) p);
			eps = eps_float;
		}
		else if (desc->dsc_dtype == DEFAULT_DOUBLE)
		{
			d = *((double*) p);
			eps = eps_double;
		}

		if (scale > 0)
			d /= CVT_power_of_ten(scale);
		else if (scale < 0)
			d *= CVT_power_of_ten(-scale);

		if (d > 0)
			d += 0.5 + eps;
		else
			d -= 0.5 + eps;

		// make sure the cast will succeed - different machines
		// do different things if the value is larger than a long can hold
		// If rounding would yield a legitimate value, permit it

		if (d < (double) LONG_MIN_real)
		{
			if (d > (double) LONG_MIN_real - 1.)
				return SLONG_MIN;
			err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		}
		if (d > (double) LONG_MAX_real)
		{
			if (d < (double) LONG_MAX_real + 1.)
				return LONG_MAX_int;
			err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		}
		return (SLONG) d;

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		{
			USHORT length =
				CVT_make_string(desc, ttype_ascii, &p, &buffer, sizeof(buffer), err);
			scale -= CVT_decompose(p, length, dtype_long, &value, err);
		}
		break;

	case dtype_blob:
	case dtype_sql_date:
	case dtype_sql_time:
	case dtype_timestamp:
	case dtype_array:
	case dtype_dbkey:
		CVT_conversion_error(desc, err);
		break;

	default:
		fb_assert(false);
		err(Arg::Gds(isc_badblk));	// internal error
		break;
	}

	// Last, but not least, adjust for scale

	if (scale > 0)
	{
		SLONG fraction = 0;
		do {
			if (scale == 1)
				fraction = value % 10;
			value /= 10;
		} while (--scale);

		if (fraction > 4)
			value++;
		// The following 2 lines are correct for platforms where
		// ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
		// a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
		// we'll have to change this depending on the platform.
		else if (fraction < -4)
			value--;
	}
	else if (scale < 0)
	{
		do {
			if (value > LONG_LIMIT || value < -LONG_LIMIT)
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			value *= 10;
		} while (++scale);
	}

	return value;
}


double CVT_get_double(const dsc* desc, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ g e t _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *      Convert something arbitrary to a double precision number
 *
 **************************************/
	double value;

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *((SSHORT *) desc->dsc_address);
		break;

	case dtype_long:
		value = *((SLONG *) desc->dsc_address);
		break;

	case dtype_quad:
#ifdef NATIVE_QUAD
		value = *((SQUAD *) desc->dsc_address);
#else
		value = ((SLONG *) desc->dsc_address)[HIGH_WORD];
		value *= -((double) LONG_MIN_real);
		if (value < 0)
			value -= ((ULONG *) desc->dsc_address)[LOW_WORD];
		else
			value += ((ULONG *) desc->dsc_address)[LOW_WORD];
#endif
		break;

	case dtype_int64:
		value = (double) *((SINT64 *) desc->dsc_address);
		break;

	case dtype_real:
		return *((float*) desc->dsc_address);

	case DEFAULT_DOUBLE:
		// memcpy is done in case dsc_address is on a platform dependant
		// invalid alignment address for doubles
		memcpy(&value, desc->dsc_address, sizeof(double));
		return value;

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		{
			VaryStr<50> buffer;	// must hold ascii of largest double
			const char* p;

			const USHORT length =
				CVT_make_string(desc, ttype_ascii, &p, &buffer, sizeof(buffer), err);
			value = 0.0;
			int scale = 0;
			SSHORT sign = 0;
			bool digit_seen = false, past_sign = false, fraction = false;
			const char* const end = p + length;

			// skip initial spaces
			while (p < end && *p == ' ')
				++p;

			for (; p < end; p++)
			{
				if (DIGIT(*p))
				{
					digit_seen = true;
					past_sign = true;
					if (fraction)
						scale++;
					value = value * 10. + (*p - '0');
				}
				else if (*p == '.')
				{
					past_sign = true;
					if (fraction)
						CVT_conversion_error(desc, err);
					else
						fraction = true;
				}
				else if (!past_sign && *p == '-')
				{
					sign = -1;
					past_sign = true;
				}
				else if (!past_sign && *p == '+')
				{
					sign = 1;
					past_sign = true;
				}
				else if (*p == 'e' || *p == 'E')
					break;
				else if (*p == ' ')
				{
					// skip spaces
					while (p < end && *p == ' ')
						++p;

					// throw if there is something after the spaces
					if (p < end)
						CVT_conversion_error(desc, err);
				}
				else
					CVT_conversion_error(desc, err);
			}

			// If we didn't see a digit then must be a funny string like "    ".
			if (!digit_seen)
				CVT_conversion_error(desc, err);

			if (sign == -1)
				value = -value;

			// If there's still something left, there must be an explicit exponent

			if (p < end)
			{
				digit_seen = false;
				sign = 0;
				SSHORT exp = 0;
				for (p++; p < end; p++)
				{
					if (DIGIT(*p))
					{
						digit_seen = true;
						exp = exp * 10 + *p - '0';

						// The following is a 'safe' test to prevent overflow of
						// exp here and of scale below. A more precise test occurs
						// later in this routine.

						if (exp >= SHORT_LIMIT)
							err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
					}
					else if (*p == '-' && !digit_seen && !sign)
						sign = -1;
					else if (*p == '+' && !digit_seen && !sign)
						sign = 1;
					else if (*p == ' ')
					{
						// skip spaces
						while (p < end && *p == ' ')
							++p;

						// throw if there is something after the spaces
						if (p < end)
							CVT_conversion_error(desc, err);
					}
					else
						CVT_conversion_error(desc, err);
				}
				if (!digit_seen)
					CVT_conversion_error(desc, err);

				if (sign == -1)
					scale += exp;
				else
					scale -= exp;
			}

			// if the scale is greater than the power of 10 representable
			// in a double number, then something has gone wrong... let
			// the user know...

			if (ABSOLUT(scale) > DBL_MAX_10_EXP)
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));


			//  Repeated division is a good way to mung the least significant bits
			//  of your value, so we have replaced this iterative multiplication/division
			//  by a single multiplication or division, depending on sign(scale).
			//if (scale > 0)
		 	//	do value /= 10.; while (--scale);
		 	//else if (scale)
		 	//	do value *= 10.; while (++scale);
			if (scale > 0)
				value /= CVT_power_of_ten(scale);
			else if (scale < 0)
				value *= CVT_power_of_ten(-scale);

			if (isinf(value))
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		}
		return value;

	case dtype_timestamp:
	case dtype_sql_date:
	case dtype_sql_time:
	case dtype_blob:
	case dtype_array:
	case dtype_dbkey:
		CVT_conversion_error(desc, err);
		break;

	default:
		fb_assert(false);
		err(Arg::Gds(isc_badblk));	// internal error
		break;
	}

	// Last, but not least, adjust for scale

	const int dscale = desc->dsc_scale;
	if (dscale == 0)
		return value;

	// if the scale is greater than the power of 10 representable
	// in a double number, then something has gone wrong... let
	// the user know...

	if (ABSOLUT(dscale) > DBL_MAX_10_EXP)
		err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));

	if (dscale > 0)
		value *= CVT_power_of_ten(dscale);
	else if (dscale < 0)
		value /= CVT_power_of_ten(-dscale);

	return value;
}


void CVT_move_common(const dsc* from, dsc* to, Callbacks* cb)
{
/**************************************
 *
 *      C V T _ m o v e
 *
 **************************************
 *
 * Functional description
 *      Move (and possible convert) something to something else.
 *
 **************************************/
	ULONG l;
	ULONG length = from->dsc_length;
	UCHAR* p = to->dsc_address;
	const UCHAR* q = from->dsc_address;

	// If the datatypes and lengths are identical, just move the
	// stuff byte by byte.  Although this may seem slower than
	// optimal, it would cost more to find the fast move than the
	// fast move would gain.

	if (DSC_EQUIV(from, to, false))
	{
		if (length) {
			memcpy(p, q, length);
		}
		return;
	}

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
			{
				GDS_TIMESTAMP date;
				string_to_datetime(from, &date, expect_timestamp, cb->err);
				*((GDS_TIMESTAMP *) to->dsc_address) = date;
			}
			return;

		case dtype_sql_date:
			((GDS_TIMESTAMP *) (to->dsc_address))->timestamp_date = *(GDS_DATE *) from->dsc_address;
			((GDS_TIMESTAMP *) (to->dsc_address))->timestamp_time = 0;
			return;

		case dtype_sql_time:
			((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_date = 0;
			((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_time = *(GDS_TIME*) from->dsc_address;
			((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_date = cb->getCurDate();
			return;

		default:
			fb_assert(false);		// Fall into ...
		case dtype_short:
		case dtype_long:
		case dtype_int64:
		case dtype_dbkey:
		case dtype_quad:
		case dtype_real:
		case dtype_double:
			CVT_conversion_error(from, cb->err);
			break;
		}
		break;

	case dtype_sql_date:
		switch (from->dsc_dtype)
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
				GDS_TIMESTAMP date;
				string_to_datetime(from, &date, expect_sql_date, cb->err);
				*((GDS_DATE *) to->dsc_address) = date.timestamp_date;
			}
			return;

		case dtype_timestamp:
			*((GDS_DATE *) to->dsc_address) = ((GDS_TIMESTAMP *) from->dsc_address)->timestamp_date;
			return;

		default:
			fb_assert(false);		// Fall into ...
		case dtype_sql_time:
		case dtype_short:
		case dtype_long:
		case dtype_int64:
		case dtype_dbkey:
		case dtype_quad:
		case dtype_real:
		case dtype_double:
			CVT_conversion_error(from, cb->err);
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
				GDS_TIMESTAMP date;
				string_to_datetime(from, &date, expect_sql_time, cb->err);
				*((GDS_TIME *) to->dsc_address) = date.timestamp_time;
			}
			return;

		case dtype_timestamp:
			*((GDS_TIME *) to->dsc_address) = ((GDS_TIMESTAMP *) from->dsc_address)->timestamp_time;
			return;

		default:
			fb_assert(false);		// Fall into ...
		case dtype_sql_date:
		case dtype_short:
		case dtype_long:
		case dtype_int64:
		case dtype_dbkey:
		case dtype_quad:
		case dtype_real:
		case dtype_double:
			CVT_conversion_error(from, cb->err);
			break;
		}
		break;

	case dtype_varying:
		MOVE_CLEAR(to->dsc_address, to->dsc_length);
		// fall through ...
	case dtype_text:
	case dtype_cstring:
		switch (from->dsc_dtype)
		{
		case dtype_dbkey:
			{
				UCHAR* ptr = NULL;
				USHORT l = 0;

				switch(to->dsc_dtype)
				{
				case dtype_text:
					ptr = to->dsc_address;
					l = to->dsc_length;
					break;
				case dtype_cstring:
					ptr = to->dsc_address;
					l = to->dsc_length - 1;
					break;
				case dtype_varying:
					ptr = to->dsc_address + sizeof(USHORT);
					l = to->dsc_length - sizeof(USHORT);
					break;
				default:
					fb_assert(false);
					break;
				}

				if (l < from->dsc_length)
					cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_string_truncation));

				Jrd::CharSet* charSet = cb->getToCharset(to->getCharSet());
				cb->validateData(charSet, from->dsc_length, from->dsc_address);

				memcpy(ptr, from->dsc_address, from->dsc_length);
				l -= from->dsc_length;
				ptr += from->dsc_length;

				switch (to->dsc_dtype)
				{
				case dtype_text:
					if (l > 0)
					{
						memset(ptr, 0, l);	// Always PAD with nulls, not spaces
					}
					break;

				case dtype_cstring:
					// Note: Following is only correct for narrow and
					// multibyte character sets which use a zero
					// byte to represent end-of-string
					*ptr = 0;
					break;

				case dtype_varying:
					((vary*) (to->dsc_address))->vary_length = from->dsc_length;
					break;
				}
			}
			return;

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
		{
			/* If we are within the engine, INTL_convert_string
			 * will convert the string between character sets
			 * (or die trying).
			 * This module, however, can be called from outside
			 * the engine (for instance, moving values around for
			 * DSQL).
			 * In that event, we'll move the values if we think
			 * they are compatible text types, otherwise fail.
			 * eg: Simple cases can be handled here (no
			 * character set conversion).
			 *
			 * a charset type binary is compatible with all other types.
			 * if a charset involved is ttype_dynamic, we must look up
			 *    the charset of the attachment (only if we are in the
			 *    engine). If we are outside the engine, the
			 *    assume that the engine has converted the values
			 *    previously in the request.
			 *
			 * Even within the engine, not calling INTL_convert_string
			 * unless really required is a good optimization.
			 */

			CHARSET_ID charset2;
			if (cb->transliterate(from, to, charset2))
				return;

			{ // scope
				USHORT strtype_unused;
				UCHAR *ptr;
				length = l = CVT_get_string_ptr_common(from, &strtype_unused, &ptr, NULL, 0, cb);
				q = ptr;
			} // end scope

			const USHORT to_size = TEXT_LEN(to);
			const UCHAR* start = to->dsc_address;
			UCHAR fill_char = ASCII_SPACE;
			Jrd::CharSet* toCharset = cb->getToCharset(charset2);
			ULONG toLength;
			ULONG fill;

			if (charset2 == ttype_binary)
				fill_char = 0x00;

			switch (to->dsc_dtype)
			{
			case dtype_text:
				length = MIN(length, to->dsc_length);
				cb->validateData(toCharset, length, q);
				toLength = length;

				l -= length;
				fill = ULONG(to->dsc_length) - length;

				CVT_COPY_BUFF(q, p, length);
				if (fill > 0)
				{
					memset(p, fill_char, fill);
					p += fill;
					// Note: above is correct only for narrow
					// and multi-byte character sets which
					// use ASCII for the SPACE character.
				}
				break;

			case dtype_cstring:
				// Note: Following is only correct for narrow and
				// multibyte character sets which use a zero
				// byte to represent end-of-string

				fb_assert(to->dsc_length > 0)
				length = MIN(length, ULONG(to->dsc_length - 1));
				cb->validateData(toCharset, length, q);
				toLength = length;

				l -= length;
				CVT_COPY_BUFF(q, p, length);
				*p = 0;
				break;

			case dtype_varying:
				length = MIN(length, (ULONG(to->dsc_length) - sizeof(USHORT)));
				cb->validateData(toCharset, length, q);
				toLength = length;

				l -= length;
				// TMN: Here we should really have the following fb_assert
				// fb_assert(length <= MAX_USHORT);
				((vary*) p)->vary_length = (USHORT) length;
				start = p = reinterpret_cast<UCHAR*>(((vary*) p)->vary_string);
				CVT_COPY_BUFF(q, p, length);
				break;
			}

			cb->validateLength(toCharset, toLength, start, to_size);

			if (l)
			{
				// Scan the truncated string to ensure only spaces lost
				// Warning: it is correct only for narrow and multi-byte
				// character sets which use ASCII or NULL for the SPACE character

				do {
					if (*q++ != fill_char)
					{
						cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_string_truncation));
					}
				} while (--l);
			}
			return;
		}

		case dtype_short:
		case dtype_long:
		case dtype_int64:
		case dtype_quad:
			integer_to_text(from, to, cb);
			return;

		case dtype_real:
		case dtype_double:
			float_to_text(from, to, cb);
			return;

		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_timestamp:
			datetime_to_text(from, to, cb);
			return;

		default:
			fb_assert(false);		// Fall into ...
		case dtype_blob:
			CVT_conversion_error(from, cb->err);
			return;
		}
		break;

	case dtype_blob:
	case dtype_array:
		if (from->dsc_dtype == dtype_quad)
		{
			((SLONG *) p)[0] = ((SLONG *) q)[0];
			((SLONG *) p)[1] = ((SLONG *) q)[1];
			return;
		}

		if (to->dsc_dtype != from->dsc_dtype)
			cb->err(Arg::Gds(isc_wish_list) << Arg::Gds(isc_blobnotsup) << "move");

		// Note: DSC_EQUIV failed above as the blob sub_types were different,
		// or their character sets were different.  In V4 we aren't trying
		// to provide blob type integrity, so we just assign the blob id

		// Move blob_id byte-by-byte as that's the way it was done before
		CVT_COPY_BUFF(q, p, length);
		return;

	case dtype_short:
		l = CVT_get_long(from, (SSHORT) to->dsc_scale, cb->err);
		// TMN: Here we should really have the following fb_assert
		// fb_assert(l <= MAX_SSHORT);
		*(SSHORT *) p = (SSHORT) l;
		if (*(SSHORT *) p != l)
			cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		return;

	case dtype_long:
		*(SLONG *) p = CVT_get_long(from, (SSHORT) to->dsc_scale, cb->err);
		return;

	case dtype_int64:
		*(SINT64 *) p = CVT_get_int64(from, (SSHORT) to->dsc_scale, cb->err);
		return;

	case dtype_quad:
		if (from->dsc_dtype == dtype_blob || from->dsc_dtype == dtype_array)
		{
			((SLONG *) p)[0] = ((SLONG *) q)[0];
			((SLONG *) p)[1] = ((SLONG *) q)[1];
			return;
		}
		*(SQUAD *) p = CVT_get_quad(from, (SSHORT) to->dsc_scale, cb->err);
		return;

	case dtype_real:
		{
			double d_value = CVT_get_double(from, cb->err);
			if (ABSOLUT(d_value) > FLOAT_MAX)
				cb->err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			*(float*) p = (float) d_value;
		}
		return;

	case dtype_dbkey:
		if (from->dsc_dtype <= dtype_any_text)
		{
			USHORT strtype_unused;
			UCHAR* ptr;
			USHORT l = CVT_get_string_ptr_common(from, &strtype_unused, &ptr, NULL, 0, cb);

			if (l == to->dsc_length)
			{
				memcpy(to->dsc_address, ptr, l);
				return;
			}
		}

		CVT_conversion_error(from, cb->err);
		break;

	case DEFAULT_DOUBLE:
#ifdef HPUX
		const double d_value = CVT_get_double(from, cb->err);
		memcpy(p, &d_value, sizeof(double));
#else
		*(double*) p = CVT_get_double(from, cb->err);
#endif
		return;
	}

	if (from->dsc_dtype == dtype_array || from->dsc_dtype == dtype_blob)
	{
		cb->err(Arg::Gds(isc_wish_list) << Arg::Gds(isc_blobnotsup) << "move");
	}

	fb_assert(false);
	cb->err(Arg::Gds(isc_badblk));	// internal error
}


void CVT_conversion_error(const dsc* desc, ErrorFunction err)
{
/**************************************
 *
 *      c o n v e r s i o n _ e r r o r
 *
 **************************************
 *
 * Functional description
 *      A data conversion error occurred.  Complain.
 *
 **************************************/
	Firebird::string message;

	if (desc->dsc_dtype == dtype_blob)
		message = "BLOB";
	else if (desc->dsc_dtype == dtype_array)
		message = "ARRAY";
	else
	{
		// CVC: I don't have access here to JRD_get_thread_data())->tdbb_status_vector
		// to be able to clear it (I don't want errors appended, but replacing
		// the existing, misleading one). Since localError doesn't fill the thread's
		// status vector as ERR_post would do, our new error is the first and only one.
		// We discard errors from CVT_make_string because we are interested in reporting
		// isc_convert_error, not isc_arith_exception if CVT_make_string fails.
		// If someone finds a better way, these hacks can be deleted.
		// Initially I was trapping here status_exception and testing
		// e.value()[1] == isc_arith_except.

		try
	    {
			const char* p;
			VaryStr<41> s;
			const USHORT length =
				CVT_make_string(desc, ttype_ascii, &p, &s, sizeof(s) - 1, localError);
			message.assign(p, length);
		}
		/*
		catch (status_exception& e)
		{
			if (e.value()[1] == isc_arith_except)
			{
				// Apparently we can't do this line:
				JRD_get_thread_data())->tdbb_status_vector[0] = 0;

			    p = "<Too long string or can't be translated>";
			}
			else
				throw;
		}
		*/
		catch (DummyException&)
		{
			message = "<Too long string or can't be translated>";
		}
	}

	err(Arg::Gds(isc_convert_error) << message);
}


static void datetime_to_text(const dsc* from, dsc* to, Callbacks* cb)
{
/**************************************
 *
 *       d a t e t i m e _ t o _ t e x t
 *
 **************************************
 *
 * Functional description
 *      Convert a timestamp, date or time value to text.
 *
 **************************************/
	bool version4 = true;

	fb_assert(DTYPE_IS_TEXT(to->dsc_dtype));

	// Convert a date or time value into a timestamp for manipulation

	tm times;
	memset(&times, 0, sizeof(struct tm));

	int	fractions = 0;

	switch (from->dsc_dtype)
	{
	case dtype_sql_time:
		Firebird::TimeStamp::decode_time(*(GDS_TIME *) from->dsc_address,
										 &times.tm_hour, &times.tm_min, &times.tm_sec,
										 &fractions);
		break;

	case dtype_sql_date:
		Firebird::TimeStamp::decode_date(*(GDS_DATE *) from->dsc_address, &times);
		break;

	case dtype_timestamp:
		cb->isVersion4(version4);
		Firebird::TimeStamp::decode_timestamp(*(GDS_TIMESTAMP *) from->dsc_address,
											  &times, &fractions);
		break;

	default:
		fb_assert(false);
		cb->err(Arg::Gds(isc_badblk));	// internal error
		break;
	}

	// Decode the timestamp into human readable terms

	TEXT temp[30];			// yyyy-mm-dd hh:mm:ss.tttt OR dd-MMM-yyyy hh:mm:ss.tttt
	TEXT* p = temp;

	// Make a textual date for data types that include it

	if (from->dsc_dtype != dtype_sql_time)
	{
		if (from->dsc_dtype == dtype_sql_date || !version4)
		{
			sprintf(p, "%4.4d-%2.2d-%2.2d",
					times.tm_year + 1900, times.tm_mon + 1, times.tm_mday);
		}
		else
		{
			// Prior to BLR version 5 timestamps were converted to text in the dd-MMM-yyyy format
			sprintf(p, "%d-%.3s-%d",
					times.tm_mday,
					FB_LONG_MONTHS_UPPER[times.tm_mon], times.tm_year + 1900);
		}
		while (*p)
			p++;
	}

	// Put in a space to separate date & time components

	if ((from->dsc_dtype == dtype_timestamp) && (!version4))
		*p++ = ' ';

	// Add the time part for data types that include it

	if (from->dsc_dtype != dtype_sql_date)
	{
		if (from->dsc_dtype == dtype_sql_time || !version4)
		{
			sprintf(p, "%2.2d:%2.2d:%2.2d.%4.4d",
					times.tm_hour, times.tm_min, times.tm_sec, fractions);
		}
		else if (times.tm_hour || times.tm_min || times.tm_sec || fractions)
		{
			// Timestamp formating prior to BLR Version 5 is slightly different
			sprintf(p, " %d:%.2d:%.2d.%.4d",
					times.tm_hour, times.tm_min, times.tm_sec, fractions);
		}
		while (*p)
			p++;
	}

	// Move the text version of the date/time value into the destination

	dsc desc;
	MOVE_CLEAR(&desc, sizeof(desc));
	desc.dsc_address = (UCHAR *) temp;
	desc.dsc_dtype = dtype_text;
	desc.dsc_ttype() = ttype_ascii;
	desc.dsc_length = (p - temp);
	if (from->dsc_dtype == dtype_timestamp && version4)
	{
		// Prior to BLR Version5, when a timestamp is converted to a string it
		// is silently truncated if the destination string is not large enough

		fb_assert(to->dsc_dtype <= dtype_any_text);

		const USHORT l = (to->dsc_dtype == dtype_cstring) ? 1 :
			(to->dsc_dtype == dtype_varying) ? sizeof(USHORT) : 0;
		desc.dsc_length = MIN(desc.dsc_length, (to->dsc_length - l));
	}

	CVT_move_common(&desc, to, cb);
}


USHORT CVT_make_string(const dsc*          desc,
					   USHORT        to_interp,
					   const char**  address,
					   vary*         temp,
					   USHORT        length,
					   ErrorFunction    err)
{
/**************************************
 *
 *      C V T _ m a k e _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *     Convert the data from the desc to a string in the specified interp.
 *     The pointer to this string is returned in address.
 *
 **************************************/
	fb_assert(desc != NULL);
	fb_assert(address != NULL);
	fb_assert(err != NULL);
	fb_assert(((temp != NULL && length > 0) ||
			(INTL_TTYPE(desc) <= dtype_any_text && INTL_TTYPE(desc) == to_interp)));

	if (desc->dsc_dtype <= dtype_any_text && INTL_TTYPE(desc) == to_interp)
	{
		*address = reinterpret_cast<char*>(desc->dsc_address);
		const USHORT from_len = desc->dsc_length;

		if (desc->dsc_dtype == dtype_text)
			return from_len;

		if (desc->dsc_dtype == dtype_cstring)
			return MIN((USHORT) strlen((char *) desc->dsc_address), from_len - 1);

		if (desc->dsc_dtype == dtype_varying)
		{
			vary* varying = (vary*) desc->dsc_address;
			*address = varying->vary_string;
			return MIN(varying->vary_length, (USHORT) (from_len - sizeof(USHORT)));
		}
	}

	// Not string data, then  -- convert value to varying string.

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR *) temp;
	INTL_ASSIGN_TTYPE(&temp_desc, to_interp);
	temp_desc.dsc_dtype = dtype_varying;
	CVT_move(desc, &temp_desc, err);
	*address = temp->vary_string;

	return temp->vary_length;
}


double CVT_power_of_ten(const int scale)
{
/*************************************
 *
 *      p o w e r _ o f _ t e n
 *
 *************************************
 *
 * Functional description
 *      return 10.0 raised to the scale power for 0 <= scale < 320.
 *
 *************************************/

	// Note that we could speed things up slightly by making the auxiliary
	// arrays global to this source module and replacing this function with
	// a macro, but the old code did up to 308 multiplies to our 1, and
	// that seems enough of a speed-up for now.

	static const double upper_part[] =
	{
		1.e000, 1.e032, 1.e064, 1.e096, 1.e128,
		1.e160, 1.e192, 1.e224, 1.e256, 1.e288
	};

	static const double lower_part[] =
	{
		1.e00, 1.e01, 1.e02, 1.e03, 1.e04, 1.e05,
		1.e06, 1.e07, 1.e08, 1.e09, 1.e10, 1.e11,
		1.e12, 1.e13, 1.e14, 1.e15, 1.e16, 1.e17,
		1.e18, 1.e19, 1.e20, 1.e21, 1.e22, 1.e23,
		1.e24, 1.e25, 1.e26, 1.e27, 1.e28, 1.e29,
		1.e30, 1.e31
	};

	// The sole caller of this function checks for scale <= 308 before calling,
	// but we just fb_assert the weakest precondition which lets the code work.
	// If the size of the exponent field, and thus the scaling, of doubles
	// gets bigger, increase the size of the upper_part array.

	fb_assert((scale >= 0) && (scale < 320));

	// Note that "scale >> 5" is another way of writing "scale / 32",
	// while "scale & 0x1f" is another way of writing "scale % 32".
	// We split the scale into the lower 5 bits and everything else,
	// then use the "everything else" to index into the upper_part array,
	// whose contents increase in steps of 1e32.

	return upper_part[scale >> 5] * lower_part[scale & 0x1f];
}


SSHORT CVT_decompose(const char* string,
					 USHORT      length,
					 SSHORT      dtype,
					 SLONG*      return_value,
					 ErrorFunction err)
{
/**************************************
 *
 *      d e c o m p o s e
 *
 **************************************
 *
 * Functional description
 *      Decompose a numeric string in mantissa and exponent,
 *      or if it is in hexadecimal notation.
 *
 **************************************/
#ifndef NATIVE_QUAD
	// For now, this routine does not handle quadwords unless this is
	// supported by the platform as a native datatype.

	if (dtype == dtype_quad)
	{
		fb_assert(false);
		err(Arg::Gds(isc_badblk));	// internal error
	}
#endif

	dsc errd;
	MOVE_CLEAR(&errd, sizeof(errd));
	errd.dsc_dtype = dtype_text;
	errd.dsc_ttype() = ttype_ascii;
	errd.dsc_length = length;
	errd.dsc_address = reinterpret_cast<UCHAR*>(const_cast<char*>(string));

	SINT64 value = 0;
	SSHORT scale = 0;
	int sign = 0;
	bool digit_seen = false, fraction = false;
	const SINT64 lower_limit = (dtype == dtype_long) ? MIN_SLONG : MIN_SINT64;
	const SINT64 upper_limit = (dtype == dtype_long) ? MAX_SLONG : MAX_SINT64;

	const SINT64 limit_by_10 = upper_limit / 10;	// used to check for overflow

	const char* p = string;
	const char* end = p + length;

	// skip initial spaces
	while (p < end && *p == ' ')
		++p;

	// Check if this is a numeric hex string. Must start with 0x or 0X, and be
	// no longer than 16 hex digits + 2 (the length of the 0x prefix) = 18.
	if (p + 2 < end && p[0] == '0' && UPPER(p[1]) == 'X')
	{
		p += 2; // skip over 0x part

		// skip non spaces
		const char* q = p;
		while (q < end && *q && *q != ' ')
			++q;

		const char* digits_end = q;

		// skip trailing spaces
		while (q < end && *q == ' ')
			q++;

		if (q != end || end - p == 0 || end - p > 16)
			CVT_conversion_error(&errd, err);

		q = p;
		value = hex_to_value(q, digits_end);

		if (q != digits_end)
			CVT_conversion_error(&errd, err);

		// 0xFFFFFFFF = -1; 0x0FFFFFFFF = 4294967295
		if (digits_end - p <= 8)
			value = (SLONG) value;

		if (dtype == dtype_long)
		{
			if (value < LONG_MIN_int64 || value > LONG_MAX_int64)
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));

			*return_value = (SLONG) value;
		}
		else
			*((SINT64*) return_value) = value;

		return 0; // 0 scale for hex literals
	}

	for (; p < end; p++)
	{
		if (DIGIT(*p))
		{
			digit_seen = true;

			// Before computing the next value, make sure there will be
			// no overflow. Trying to detect overflow after the fact is
			// tricky: the value doesn't always become negative after an
			// overflow!

			if (value >= limit_by_10)
			{
				// possibility of an overflow
				if (value > limit_by_10)
				{
					err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
				}
				else if ((*p > '8' && sign == -1) || (*p > '7' && sign != -1))
				{
					err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
				}
			}

			value = value * 10 + *p - '0';
			if (fraction)
				--scale;
		}
		else if (*p == '.')
		{
			if (fraction)
				CVT_conversion_error(&errd, err);
			else
				fraction = true;
		}
		else if (*p == '-' && !digit_seen && !sign && !fraction)
			sign = -1;
		else if (*p == '+' && !digit_seen && !sign && !fraction)
			sign = 1;
		else if (*p == 'e' || *p == 'E')
			break;
		else if (*p == ' ')
		{
			// skip spaces
			while (p < end && *p == ' ')
				++p;

			// throw if there is something after the spaces
			if (p < end)
				CVT_conversion_error(&errd, err);
		}
		else
			CVT_conversion_error(&errd, err);
	}

	if (!digit_seen)
		CVT_conversion_error(&errd, err);

	if ((sign == -1) && value != lower_limit)
		value = -value;

	// If there's still something left, there must be an explicit exponent
	if (p < end)
	{
		sign = 0;
		SSHORT exp = 0;
		digit_seen = false;
		for (p++; p < end; p++)
		{
			if (DIGIT(*p))
			{
				digit_seen = true;
				exp = exp * 10 + *p - '0';

				// The following is a 'safe' test to prevent overflow of
				// exp here and of scale below. A more precise test will
				// occur in the calling routine when the scale/exp is
				// applied to the value.

				if (exp >= SHORT_LIMIT)
					err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			}
			else if (*p == '-' && !digit_seen && !sign)
				sign = -1;
			else if (*p == '+' && !digit_seen && !sign)
				sign = 1;
			else if (*p == ' ')
			{
				// skip spaces
				while (p < end && *p == ' ')
					++p;

				// throw if there is something after the spaces
				if (p < end)
					CVT_conversion_error(&errd, err);
			}
			else
				CVT_conversion_error(&errd, err);
		}
		if (sign == -1)
			scale -= exp;
		else
			scale += exp;

		if (!digit_seen)
			CVT_conversion_error(&errd, err);

	}

	if (dtype == dtype_long)
		*return_value = (SLONG) value;
	else
		*((SINT64 *) return_value) = value;

	return scale;
}


USHORT CVT_get_string_ptr_common(const dsc* desc, USHORT* ttype, UCHAR** address,
								 vary* temp, USHORT length, Callbacks* cb)
{
/**************************************
 *
 *      C V T _ g e t _ s t r i n g _ p t r
 *
 **************************************
 *
 * Functional description
 *      Get address and length of string, converting the value to
 *      string, if necessary.  The caller must provide a sufficiently
 *      large temporary.  The address of the resultant string is returned
 *      by reference.  Get_string returns the length of the string (in bytes).
 *
 *      The text-type of the string is returned in ttype.
 *
 *      Note: If the descriptor is known to be a string type, the fourth
 *      argument (temp buffer) may be NULL.
 *
 *      If input (desc) is already a string, the output pointers
 *      point to the data of the same text_type.  If (desc) is not
 *      already a string, output pointers point to ttype_ascii.
 *
 **************************************/
	fb_assert(desc != NULL);
	fb_assert(ttype != NULL);
	fb_assert(address != NULL);
	fb_assert(cb != NULL);
	fb_assert((temp != NULL && length > 0) ||
			desc->dsc_dtype == dtype_text ||
			desc->dsc_dtype == dtype_cstring || desc->dsc_dtype == dtype_varying);

	// If the value is already a string (fixed or varying), just return
	// the address and length.

	if (desc->dsc_dtype <= dtype_any_text)
	{
		*address = desc->dsc_address;
		*ttype = INTL_TTYPE(desc);
		if (desc->dsc_dtype == dtype_text)
			return desc->dsc_length;
		if (desc->dsc_dtype == dtype_cstring)
			return MIN((USHORT) strlen((char *) desc->dsc_address),
					   desc->dsc_length - 1);
		if (desc->dsc_dtype == dtype_varying)
		{
			vary* varying = (vary*) desc->dsc_address;
			*address = reinterpret_cast<UCHAR*>(varying->vary_string);
			return MIN(varying->vary_length,
					   (USHORT) (desc->dsc_length - sizeof(USHORT)));
		}
	}

	// Also trivial case - DB_KEY

	if (desc->dsc_dtype == dtype_dbkey)
	{
		*address = desc->dsc_address;
		*ttype = ttype_binary;
		return desc->dsc_length;
	}

	// No luck -- convert value to varying string.

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR *) temp;
	INTL_ASSIGN_TTYPE(&temp_desc, ttype_ascii);
	temp_desc.dsc_dtype = dtype_varying;
	CVT_move_common(desc, &temp_desc, cb);
	*address = reinterpret_cast<UCHAR*>(temp->vary_string);
	*ttype = INTL_TTYPE(&temp_desc);

	return temp->vary_length;
}


SQUAD CVT_get_quad(const dsc* desc, SSHORT scale, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ g e t _ q u a d
 *
 **************************************
 *
 * Functional description
 *      Convert something arbitrary to a quad (64 bit) integer of given
 *      scale.
 *
 **************************************/
	SQUAD value;
	double d;
	VaryStr<50> buffer;			// long enough to represent largest quad in ASCII

	// adjust exact numeric values to same scaling

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		{
			const SSHORT input = *(SSHORT*) p;
			((SLONG*) &value)[LOW_WORD] = input;
			((SLONG*) &value)[HIGH_WORD] = (input < 0) ? -1 : 0;
		}
		break;

	case dtype_long:
		{
			const SLONG input = *(SLONG*) p;
			((SLONG*) &value)[LOW_WORD] = input;
			((SLONG*) &value)[HIGH_WORD] = (input < 0) ? -1 : 0;
		}
		break;

	case dtype_quad:
		value = *((SQUAD*) p);
		break;

	case dtype_int64:
		{
			const SINT64 input = *(SINT64*) p;
			((SLONG*) &value)[LOW_WORD] = (SLONG) (input & 0xffffffff);
			((SLONG*) &value)[HIGH_WORD] = (SLONG) (input >> 32);
		}
		break;

	case dtype_real:
	case dtype_double:
		if (desc->dsc_dtype == dtype_real)
			d = *((float*) p);
		else if (desc->dsc_dtype == DEFAULT_DOUBLE)
			d = *((double*) p);

		if (scale > 0)
		{
			do {
				d /= 10.;
			} while (--scale);
		}
		else if (scale < 0)
		{
			do {
				d *= 10.;
			} while (++scale);
		}

		if (d > 0)
			d += 0.5;
		else
			d -= 0.5;

		// make sure the cast will succeed - different machines
		// do different things if the value is larger than a quad
		// can hold

		if (d < (double) QUAD_MIN_real || (double) QUAD_MAX_real < d)
		{
			// If rounding would yield a legitimate value, permit it

			if (d > (double) QUAD_MIN_real - 1.)
				return QUAD_MIN_int;

			if (d < (double) QUAD_MAX_real + 1.)
				return QUAD_MAX_int;

			err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
		}
		return QUAD_FROM_DOUBLE(d, err);

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		{
			USHORT length =
				CVT_make_string(desc, ttype_ascii, &p, &buffer, sizeof(buffer), err);
			scale -= CVT_decompose(p, length, dtype_quad, &value.high, err);
		}
		break;

	case dtype_blob:
	case dtype_sql_date:
	case dtype_sql_time:
	case dtype_timestamp:
	case dtype_array:
	case dtype_dbkey:
		CVT_conversion_error(desc, err);
		break;

	default:
		fb_assert(false);
		err(Arg::Gds(isc_badblk));	// internal error
		break;
	}

	// Last, but not least, adjust for scale

	if (scale == 0)
		return value;

#ifndef NATIVE_QUAD
	fb_assert(false);
	err(Arg::Gds(isc_badblk));	// internal error
#else
	if (scale > 0)
	{
		SLONG fraction = 0;
		do {
			if (scale == 1)
				fraction = value % 10;
			value /= 10;
		} while (--scale);
		if (fraction > 4)
			value++;
		// The following 2 lines are correct for platforms where
		// ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
		// a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
		// we'll have to change this depending on the platform.
		else if (fraction < -4)
			value--;
	}
	else
	{
		do {
			if (value > QUAD_LIMIT || value < -QUAD_LIMIT)
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			value *= 10;
		} while (++scale);
	}
#endif

	return value;
}


SINT64 CVT_get_int64(const dsc* desc, SSHORT scale, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ g e t _ i n t 6 4
 *
 **************************************
 *
 * Functional description
 *      Convert something arbitrary to an SINT64 (64 bit integer)
 *      of given scale.
 *
 **************************************/
	SINT64 value;
	double d, eps;
	VaryStr<50> buffer;			// long enough to represent largest SINT64 in ASCII

	// adjust exact numeric values to same scaling

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype)
	{
	case dtype_short:
		value = *((SSHORT*) p);
		break;

	case dtype_long:
		value = *((SLONG*) p);
		break;

	case dtype_int64:
		value = *((SINT64*) p);
		break;

	case dtype_quad:
		value = (((SINT64) ((SLONG*) p)[HIGH_WORD]) << 32) + (((ULONG*) p)[LOW_WORD]);
		break;

	case dtype_real:
	case dtype_double:
		if (desc->dsc_dtype == dtype_real)
		{
			d = *((float*) p);
			eps = eps_float;
		}
		else if (desc->dsc_dtype == DEFAULT_DOUBLE)
		{
			d = *((double*) p);
			eps = eps_double;
		}

		if (scale > 0)
			d /= CVT_power_of_ten(scale);
		else if (scale < 0)
			d *= CVT_power_of_ten(-scale);

		if (d > 0)
			d += 0.5 + eps;
		else
			d -= 0.5 + eps;

		/* make sure the cast will succeed - different machines
		   do different things if the value is larger than a quad
		   can hold.

		   Note that adding or subtracting 0.5, as we do in CVT_get_long,
		   will never allow the rounded value to fit into an int64,
		   because when the double value is too large in magnitude
		   to fit, 0.5 is less than half of the least significant bit
		   of the significant (sometimes miscalled "mantissa") of the
		   double, and thus will have no effect on the sum. */

		if (d < (double) QUAD_MIN_real || (double) QUAD_MAX_real < d)
			err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));

		return (SINT64) d;

	case dtype_varying:
	case dtype_cstring:
	case dtype_text:
		{
			USHORT length =
				CVT_make_string(desc, ttype_ascii, &p, &buffer, sizeof(buffer), err);
			scale -= CVT_decompose(p, length, dtype_int64, (SLONG *) & value, err);
		}
		break;

	case dtype_blob:
	case dtype_sql_date:
	case dtype_sql_time:
	case dtype_timestamp:
	case dtype_array:
	case dtype_dbkey:
		CVT_conversion_error(desc, err);
		break;

	default:
		fb_assert(false);
		err(Arg::Gds(isc_badblk));	// internal error
		break;
	}

	// Last, but not least, adjust for scale

	if (scale > 0)
	{
		SLONG fraction = 0;
		do {
			if (scale == 1)
				fraction = (SLONG) (value % 10);
			value /= 10;
		} while (--scale);
		if (fraction > 4)
			value++;
		// The following 2 lines are correct for platforms where
		// (-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
		// a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
		// we'll have to change this depending on the platform.
		else if (fraction < -4)
			value--;
	}
	else if (scale < 0)
	{
		do {
			if (value > INT64_LIMIT || value < -INT64_LIMIT)
				err(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			value *= 10;
		} while (++scale);
	}

	return value;
}


static SINT64 hex_to_value(const char*& string, const char* end)
/*************************************
 *
 *      hex_to_value
 *
 *************************************
 *
 * Functional description
 *      Convert a hex string to a numeric value. This code only
 *      converts a hex string into a numeric value, and the
 *      biggest hex string supported must fit into a BIGINT.
 *
 *************************************/
{
	// we already know this is a hex string, and there is no prefix.
	// So, string is something like DEADBEEF.

	SINT64 value = 0;
	UCHAR byte = 0;
	int nibble = ((end - string) & 1);
	char ch;

	while ((DIGIT((ch = UPPER(*string)))) || ((ch >= 'A') && (ch <= 'F')))
	{
		// Now convert the character to a nibble
		SSHORT c;

		if (ch >= 'A')
			c = (ch - 'A') + 10;
		else
			c = (ch - '0');

		if (nibble)
		{
			byte = (byte << 4) + (UCHAR) c;
			nibble = 0;
			value = (value << 8) + byte;
		}
		else
		{
			byte = c;
			nibble = 1;
		}

		++string;
	}

	fb_assert(string <= end);

	return value;
}


static void localError(const Firebird::Arg::StatusVector&)
{
	throw DummyException();
}


namespace
{
	class CommonCallbacks : public Callbacks
	{
	public:
		explicit CommonCallbacks(ErrorFunction aErr)
			: Callbacks(aErr)
		{
		}

	public:
		virtual bool transliterate(const dsc* from, dsc* to, CHARSET_ID&);
		virtual CHARSET_ID getChid(const dsc* d);
		virtual Jrd::CharSet* getToCharset(CHARSET_ID charset2);
		virtual void validateData(Jrd::CharSet* toCharset, SLONG length, const UCHAR* q);
		virtual void validateLength(Jrd::CharSet* toCharset, SLONG toLength, const UCHAR* start,
			const USHORT to_size);
		virtual SLONG getCurDate();
		virtual void isVersion4(bool& v4);
	};

	bool CommonCallbacks::transliterate(const dsc*, dsc* to, CHARSET_ID& charset2)
	{
		charset2 = INTL_TTYPE(to);
		return false;
	}

	Jrd::CharSet* CommonCallbacks::getToCharset(CHARSET_ID)
	{
		return NULL;
	}

	void CommonCallbacks::validateData(Jrd::CharSet*, SLONG, const UCHAR*)
	{
	}

	void CommonCallbacks::validateLength(Jrd::CharSet*, SLONG, const UCHAR*, const USHORT)
	{
	}

	CHARSET_ID CommonCallbacks::getChid(const dsc* d)
	{
		return INTL_TTYPE(d);
	}

	SLONG CommonCallbacks::getCurDate()
	{
		return TimeStamp::getCurrentTimeStamp().value().timestamp_date;
	}

	void CommonCallbacks::isVersion4(bool& /*v4*/)
	{
	}
}	// namespace


USHORT CVT_get_string_ptr(const dsc* desc, USHORT* ttype, UCHAR** address,
						  vary* temp, USHORT length, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ g e t _ s t r i n g _ p t r
 *
 **************************************
 *
 * Functional description
 *      Get address and length of string, converting the value to
 *      string, if necessary.  The caller must provide a sufficiently
 *      large temporary.  The address of the resultant string is returned
 *      by reference.  Get_string returns the length of the string (in bytes).
 *
 *      The text-type of the string is returned in ttype.
 *
 *      Note: If the descriptor is known to be a string type, the fourth
 *      argument (temp buffer) may be NULL.
 *
 *      If input (desc) is already a string, the output pointers
 *      point to the data of the same text_type.  If (desc) is not
 *      already a string, output pointers point to ttype_ascii.
 *
 **************************************/
	fb_assert(err != NULL); 

	CommonCallbacks callbacks(err);
	return CVT_get_string_ptr_common(desc, ttype, address, temp, length, &callbacks);
}


void CVT_move(const dsc* from, dsc* to, ErrorFunction err)
{
/**************************************
 *
 *      C V T _ m o v e
 *
 **************************************
 *
 * Functional description
 *      Move (and possible convert) something to something else.
 *
 **************************************/
	CommonCallbacks callbacks(err);
	CVT_move_common(from, to, &callbacks);
}
