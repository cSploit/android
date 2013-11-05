/*
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
 * Changes made by Claudio Valderrama for the Firebird project
 *   changes to substr and added substrlen
 * 2004.9.1 Claudio Valderrama, change some UDF's to be able to detect NULL.
 * 2004.12.5 Slavomir Skopalik contributed IB_UDF_frac.
 *
 */

#include "firebird.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "fb_types.h"
#include "ib_util.h"
#include "ib_udf.h"

extern "C"
{

#ifndef __ICC
#ifndef SOLARIS
#ifdef WIN_NT
#define exception_type _exception
#else
#define exception_type __exception
#endif
int MATHERR(struct exception_type*)
{
	return 1;
}
#undef exception_type
#endif /* SOLARIS */
#endif //__ICC

typedef char* pChar;

double EXPORT IB_UDF_abs( double *a)
{
	return (*a < 0.0) ? -*a : *a;
}

double EXPORT IB_UDF_acos( double *a)
{
	return (acos(*a));
}

pChar EXPORT IB_UDF_ascii_char( ISC_LONG *a)
{
	if (!a)
		return 0;

	char* b = (char *) ib_util_malloc(2);
	*b = (char) (*a);
	/* let us not forget to NULL terminate */
	b[1] = '\0';
	return (b);
}

ISC_LONG EXPORT IB_UDF_ascii_val( const char *a)
{
	// NULL is treated as ASCII(0).
	return ((ISC_LONG) (*a));
}

double EXPORT IB_UDF_asin( double *a)
{
	return (asin(*a));
}

double EXPORT IB_UDF_atan( double *a)
{
	return (atan(*a));
}

double EXPORT IB_UDF_atan2( double *a, double *b)
{
	return (atan2(*a, *b));
}

ISC_LONG EXPORT IB_UDF_bin_and( ISC_LONG *a, ISC_LONG *b)
{
	return (*a & *b);
}

ISC_LONG EXPORT IB_UDF_bin_or( ISC_LONG *a, ISC_LONG *b)
{
	return (*a | *b);
}

ISC_LONG EXPORT IB_UDF_bin_xor( ISC_LONG *a, ISC_LONG *b)
{
	return (*a ^ *b);
}

double EXPORT IB_UDF_ceiling( double *a)
{
	return (ceil(*a));
}

double EXPORT IB_UDF_cos( double *a)
{
	return (cos(*a));
}

double EXPORT IB_UDF_cosh( double *a)
{
	return (cosh(*a));
}

double EXPORT IB_UDF_cot( double *a)
{
	return (1.0 / tan(*a));
}

double EXPORT IB_UDF_div( ISC_LONG *a, ISC_LONG *b)
{
	if (*b != 0)
	{
		// VS8.0 has two implementations of div().
		// Let's explicitly use the int-aware one.
		div_t div_result = div((int) *a, (int) *b);
		return (div_result.quot);
	}

	// This is a Kludge!  We need to return INF,
	// but this seems to be the only way to do
	// it since there seens to be no constant for it.
#ifdef HAVE_INFINITY
	return INFINITY;
#else
	return 1 / tan(0.0);
#endif
}

double EXPORT IB_UDF_floor( double *a)
{
	return (floor(*a));
}

double EXPORT IB_UDF_frac(const double* x)
{
	if (*x > 0)
		return *x - floor(*x);
	if (*x < 0)
		return *x - ceil(*x);
	return 0;
}

double EXPORT IB_UDF_ln( double *a)
{
	return (log(*a));
}

double EXPORT IB_UDF_log( double *a, double *b)
{
	return (log(*b) / log(*a));
}

double EXPORT IB_UDF_log10( double *a)
{
	return (log10(*a));
}

pChar EXPORT IB_UDF_lower(const char *s)
{
	if (!s)
		return 0;

	char* buf = (char *) ib_util_malloc(strlen(s) + 1);
	char* p = buf;
	while (*s)
	{
		if (*s >= 'A' && *s <= 'Z') {
			*p++ = *s++ - 'A' + 'a';
		}
		else
			*p++ = *s++;
	}
	*p = '\0';

	return buf;
}

pChar EXPORT IB_UDF_lpad( const char *s, ISC_LONG *a, const char *c)
{
	if (!s || !c)
		return 0;

	const long avalue = *a;

	if (avalue >= 0)
	{
		long current = 0;
		const long length = strlen(s);
		const long padlength = strlen(c);
		const long stop = avalue < length ? avalue : length;
		char* buf = (char*) ib_util_malloc(avalue + 1);

		if (padlength)
		{
			while (current + length < avalue)
			{
				memcpy(&buf[current], c, padlength);
				current += padlength;
			}
			memcpy(&buf[(avalue - length < 0) ? 0 : avalue - length], s, stop);
			buf[avalue] = '\0';
		}
		else
		{
			memcpy(buf, s, stop);
			buf[stop] = '\0';
		}
		return buf;
	}

	return NULL;
}

pChar EXPORT IB_UDF_ltrim( const char *s)
{
	if (!s)
		return 0;

	while (*s == ' ')		/* skip leading blanks */
		s++;

	const long length = strlen(s);
	char* buf = (char *) ib_util_malloc(length + 1);
	memcpy(buf, s, length);
	buf[length] = '\0';

	return buf;
}

double EXPORT IB_UDF_mod( ISC_LONG *a, ISC_LONG *b)
{
	if (*b != 0)
	{
		// VS8.0 has two implementations of div().
		// Let's explicitly use the int-aware one.
		div_t div_result = div((int) *a, (int) *b);
		return (div_result.rem);
	}

	// This is a Kludge!  We need to return INF,
	// but this seems to be the only way to do
	// it since there seens to be no constant for it.
#ifdef HAVE_INFINITY
	return INFINITY;
#else
	return 1 / tan(0.0);
#endif
}

double EXPORT IB_UDF_pi()
{
	return (IB_PI);
}

double EXPORT IB_UDF_srand()
{
	srand((unsigned) time(NULL));
	return ((float) rand() / (float) RAND_MAX);
}

double EXPORT IB_UDF_rand()
{
	return ((float) rand() / (float) RAND_MAX);
}

pChar EXPORT IB_UDF_rpad( const char *s, ISC_LONG *a, const char *c)
{
	if (!s || !c)
		return 0;

	const long avalue = *a;

	if (avalue >= 0)
	{
		const long length = strlen(s);
		long current = (avalue - length) < 0 ? avalue : length;
		const long padlength = strlen(c);
		char* buf = (char*) ib_util_malloc (avalue + 1);
		memcpy(buf, s, current);

		if (padlength)
		{
			while (current + padlength < avalue)
			{
				memcpy(&buf[current], c, padlength);
				current += padlength;
			}
			memcpy(&buf[current], c, avalue - current);
			buf[avalue] = '\0';
		}
		else
			buf[current] = 0;

		return buf;
	}

	return NULL;
}

pChar EXPORT IB_UDF_rtrim( const char *s)
{
	if (!s)
		return 0;

	const char* p = s + strlen(s);
	while (--p >= s && *p == ' ')
		; // empty loop body

	const long length = p - s + 1;
	char* buf = (char *) ib_util_malloc(length + 1);
	memcpy(buf, s, length);
	buf[length] = '\0';

	return buf;
}

ISC_LONG EXPORT IB_UDF_sign( double *a)
{
	if (*a > 0)
		return 1;
	if (*a < 0)
		return -1;
	/* If neither is true then it equals 0 */
	return 0;
}

double EXPORT IB_UDF_sin( double *a)
{
	return (sin(*a));
}

double EXPORT IB_UDF_sinh( double *a)
{
	return (sinh(*a));
}

double EXPORT IB_UDF_sqrt( double *a)
{
	return (sqrt(*a));
}

pChar EXPORT IB_UDF_substr(const char* s, ISC_SHORT* m, ISC_SHORT* n)
{
	if (!s) {
		return 0;
	}

	char* buf;
	long length = strlen(s);
	if (!length ||
		*m > *n ||
		*m < 1  ||
		*n < 1  ||
		*m > length)
	{
		buf = (char*)ib_util_malloc(1);
		buf[0] = '\0';
	}
	else
	{
		/* we want from the mth char to the
		   nth char inclusive, so add one to
		   the length. */
		/* CVC: We need to compensate for n if it's longer than s's length */
		if (*n > length) {
			length -= *m - 1;
		}
		else {
			length = *n - *m + 1;
		}
		buf = (char*)ib_util_malloc (length + 1);
		memcpy(buf, s + *m - 1, length);
		buf[length] = '\0';
	}
	return buf;
}

pChar EXPORT IB_UDF_substrlen(const char* s, ISC_SHORT* m, ISC_SHORT* n)
{
	/* Created by Claudio Valderrama for the Firebird project,
		2001.04.17 We don't want to return NULL when params are wrong
		and we'll return the remaining characters if the final position
		is greater than the string's length, unless NULL is provided.
	*/
	if (!s) {
		return NULL;
	}

	char* buf;
	long length = strlen(s);
	if (!length ||
		*m < 1  ||
		*n < 1  ||
		*m > length)
	{
		buf = (char*)ib_util_malloc(1);
		buf[0] = '\0';
	}
	else
	{
		/* we want from the mth char to the (m+n)th char inclusive,
		 * so add one to the length.
		 */
		/* CVC: We need to compensate for n if it's longer than s's length */
		if (*m + *n - 1 > length) {
			length -= *m - 1;
		}
		else {
			length = *n;
		}
		buf = (char*) ib_util_malloc (length + 1);
		memcpy(buf, s + *m - 1, length);
		buf[length] = '\0';
	}
	return buf;
}

ISC_LONG EXPORT IB_UDF_strlen( const char *a)
{
	return (strlen(a));
}

double EXPORT IB_UDF_tan( double *a)
{
	return (tan(*a));
}

double EXPORT IB_UDF_tanh( double *a)
{
	return (tanh(*a));
}

} // extern "C"

