/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2010, 2011  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if TIME_WITH_SYS_TIME
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <assert.h>
#include <ctype.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#include <freetds/tds.h>
#include <freetds/convert.h>
#include <freetds/bytes.h>
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: convert.c,v 1.217 2011-08-11 12:50:26 freddy77 Exp $");

typedef unsigned short utf16_t;

struct tds_time
{
	int tm_year; /**< year (0=1900) */
	int tm_mon;  /**< month (0-11) */
	int tm_mday; /**< month day (1-31) */
	int tm_hour; /**< hours (0-23) */
	int tm_min;  /**< minutes (0-59) */
	int tm_sec;  /**< seconds (0-59) */
	int tm_ns;   /**< nanoseconds (0-999999999) */
};

static TDS_INT tds_convert_int(TDS_INT num, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_int1(const TDS_TINYINT * src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_int2(const TDS_SMALLINT * src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_uint2(const TDS_USMALLINT * src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_int4(const TDS_INT* src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_uint4(const TDS_UINT * src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_int8(const TDS_INT8 * src, int desttype, CONV_RESULT * cr);
static TDS_INT tds_convert_uint8(const TDS_UINT8 * src, int desttype, CONV_RESULT * cr);
static int string_to_datetime(const char *datestr, int desttype, CONV_RESULT * cr);
static int is_dd_mon_yyyy(char *t);
static int store_dd_mon_yyy_date(char *datestr, struct tds_time *t);

#define test_alloc(x) {if ((x)==NULL) return TDS_CONVERT_NOMEM;}

#define IS_TINYINT(x) ( 0 <= (x) && (x) <= 0xff )
#define IS_SMALLINT(x) ( -32768 <= (x) && (x) <= 32767 )
#define IS_USMALLINT(x) ( 0 <= (x) && (x) <= 65535 )
/*
 * f77: I don't write -2147483648, some compiler seem to have some problem 
 * with this constant although is a valid 32bit value
 */
#define TDS_INT_MIN (-2147483647l-1l)
#define TDS_INT_MAX 2147483647l
#define IS_INT(x) (TDS_INT_MIN <= (x) && (x) <= TDS_INT_MAX)

#define TDS_UINT_MAX 4294967295lu
#define IS_UINT(x) (0 <= (x) && (x) <= TDS_UINT_MAX)

#define TDS_INT8_MAX ((((TDS_INT8) 0x7fffffffl) << 32) + (TDS_INT8) 0xfffffffflu)
#define TDS_INT8_MIN  (((TDS_INT8) (-0x7fffffffl-1)) << 32)
#define IS_INT8(x) (TDS_INT8_MIN <= (x) && (x) <= TDS_INT8_MAX)

#define TDS_UINT8_MAX ((((TDS_UINT8) 0xfffffffflu) << 32) + 0xfffffffflu)
#define IS_UINT8(x) (0 <= (x) && (x) <= TDS_UINT8_MAX)

#define TDS_ISDIGIT(c) ((c) >= '0' && (c) <= '9')

/**
 * \ingroup libtds
 * \defgroup convert Conversion
 * Conversions between datatypes.  Supports, for example, dbconvert().  
 */

/**
 * \addtogroup convert
 * @{ 
 */

/**
 * convert a number in string to a TDSNUMERIC
 * @return sizeof(TDS_NUMERIC) on success, TDS_CONVERT_* failure code on failure 
 */
static int string_to_numeric(const char *instr, const char *pend, CONV_RESULT * cr);

/**
 * convert a zero terminated string to NUMERIC
 * @return sizeof(TDS_NUMERIC) on success, TDS_CONVERT_* failure code on failure 
 */
static int stringz_to_numeric(const char *instr, CONV_RESULT * cr);

static TDS_INT string_to_int(const char *buf, const char *pend, TDS_INT * res);
static TDS_INT string_to_int8(const char *buf, const char *pend, TDS_INT8 * res);
static TDS_INT string_to_uint8(const char *buf, const char *pend, TDS_UINT8 * res);

static int store_hour(const char *, const char *, struct tds_time *);
static int store_time(const char *, struct tds_time *);
static int store_yymmdd_date(const char *, struct tds_time *);
static int store_monthname(const char *, struct tds_time *);
static int store_numeric_date(const char *, struct tds_time *);
static int store_mday(const char *, struct tds_time *);
static int store_year(int, struct tds_time *);

/* static int days_this_year (int years); */
static int is_timeformat(const char *);
static int is_numeric(const char *);
static int is_alphabetic(const char *);
static int is_ampm(const char *);
#define is_monthname(s) (store_monthname(s, NULL) >= 0)
static int is_numeric_dateformat(const char *);

#if 0
static TDS_UINT utf16len(const utf16_t * s);
static const char *tds_prtype(int token);
#endif

const char tds_hex_digits[16] = "0123456789abcdef";

/**
 * Copy a terminated string to result and return len or TDS_CONVERT_NOMEM
 */
static TDS_INT
string_to_result(int desttype, const char *s, CONV_RESULT * cr)
{
	size_t len = strlen(s);

	if (desttype != TDS_CONVERT_CHAR) {
		cr->c = (TDS_CHAR *) malloc(len + 1);
		test_alloc(cr->c);
		memcpy(cr->c, s, len + 1);
	} else {
		memcpy(cr->cc.c, s, len < cr->cc.len ? len : cr->cc.len);
	}
	return (TDS_INT)len;
}

#define string_to_result(s, cr) \
	string_to_result(desttype, s, cr)

/**
 * Copy binary data to to result and return len or TDS_CONVERT_NOMEM
 */
static TDS_INT
binary_to_result(int desttype, const void *data, size_t len, CONV_RESULT * cr)
{
	if (desttype != TDS_CONVERT_BINARY) {
		cr->ib = (TDS_CHAR *) malloc(len);
		test_alloc(cr->ib);
		memcpy(cr->ib, data, len);
	} else {
		memcpy(cr->cb.ib, data, len < cr->cb.len ? len : cr->cb.len);
	}
	return (TDS_INT)len;
}

#define binary_to_result(data, len, cr) \
	binary_to_result(desttype, data, len, cr)

#define CASE_ALL_CHAR \
	SYBCHAR: case SYBVARCHAR: case SYBTEXT: case XSYBCHAR: case XSYBVARCHAR
#define CASE_ALL_BINARY \
	SYBBINARY: case SYBVARBINARY: case SYBIMAGE: case XSYBBINARY: case XSYBVARBINARY: case TDS_CONVERT_BINARY

/* TODO implement me */
/*
static TDS_INT 
tds_convert_ntext(int srctype,TDS_CHAR *src,TDS_UINT srclen,
      int desttype, CONV_RESULT *cr)
{
      return TDS_CONVERT_NOAVAIL;
}
*/

static TDS_INT
tds_convert_binary(const TDS_UCHAR * src, TDS_INT srclen, int desttype, CONV_RESULT * cr)
{
	int cplen;
	int s;
	char *c;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
		cplen = srclen * 2;
		if ((TDS_UINT)cplen > cr->cc.len)
			cplen = cr->cc.len;

		c = cr->cc.c;
		for (s = 0; cplen >= 2; ++s, cplen -= 2) {
			*c++ = tds_hex_digits[src[s]>>4];
			*c++ = tds_hex_digits[src[s]&0xF];
		}
		if (cplen)
			*c = tds_hex_digits[src[s]>>4];
		return srclen * 2;

	case CASE_ALL_CHAR:

		/*
		 * NOTE: Do not prepend 0x to string.  
		 * The libraries all expect a bare string, without a 0x prefix. 
		 * Applications such as isql and query analyzer provide the "0x" prefix.
		 */

		/* 2 * source length + 1 for terminator */

		cr->c = (TDS_CHAR *) malloc((srclen * 2) + 1);
		test_alloc(cr->c);

		c = cr->c;

		for (s = 0; s < srclen; s++) {
			*c++ = tds_hex_digits[src[s]>>4];
			*c++ = tds_hex_digits[src[s]&0xF];
		}

		*c = '\0';
		return (srclen * 2);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, srclen, cr);
		break;
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBUINT4:
	case SYBINT8:
	case SYBUINT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBREAL:
	case SYBFLT8:
		cplen = tds_get_size_by_type(desttype);
		if (srclen >= cplen)
			srclen = cplen;
		memcpy(cr, src, srclen);
		memset(((char*) cr) + srclen, 0, cplen - srclen);
		return cplen;
		break;

		/* conversions not allowed */
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:

		/* TODO should we do some test for these types or work as ints ?? */
	case SYBDECIMAL:
	case SYBNUMERIC:
	case SYBBIT:
	case SYBBITN:

	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

TDS_INT
tds_char2hex(TDS_CHAR *dest, TDS_UINT destlen, const TDS_CHAR * src, TDS_UINT srclen)
{
	unsigned int i;
	unsigned char hex1, c = 0;

	/* if srclen if odd we must add a "0" before ... */
	i = 0;		/* number where to start converting */
	if (srclen & 1) {
		++srclen;
		i = 1;
		--src;
	}
	for (; i < srclen; ++i) {
		hex1 = src[i];

		if ('0' <= hex1 && hex1 <= '9')
			hex1 &= 0x0f;
		else {
			hex1 &= 0x20 ^ 0xff;	/* mask off 0x20 to ensure upper case */
			if ('A' <= hex1 && hex1 <= 'F') {
				hex1 -= ('A' - 10);
			} else {
				tdsdump_log(TDS_DBG_INFO1,
					    "error_handler:  attempt to convert data stopped by syntax error in source field \n");
				return TDS_CONVERT_SYNTAX;
			}
		}
		assert(hex1 < 0x10);

		if ((i/2u) >= destlen)
			continue;

		if (i & 1)
			dest[i / 2u] = c | hex1;
		else
			c = hex1 << 4;
	}
	return srclen / 2u;
}

static TDS_INT
tds_convert_char(const TDS_CHAR * src, TDS_UINT srclen, int desttype, CONV_RESULT * cr)
{
	unsigned int i;

	TDS_INT8 mymoney;
	char mynumber[28];

	const char *ptr, *pend;
	int point_found;
	unsigned int places;
	TDS_INT tds_i;
	TDS_INT8 tds_i8;
	TDS_UINT8 tds_ui8;
	TDS_INT rc;
	TDS_CHAR *ib;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
		memcpy(cr->cc.c, src, srclen < cr->cc.len ? srclen : cr->cc.len);
		return srclen;

	case CASE_ALL_CHAR:
		cr->c = (TDS_CHAR *) malloc(srclen + 1);
		test_alloc(cr->c);
		memcpy(cr->c, src, srclen);
		cr->c[srclen] = 0;
		return srclen;
		break;

	case CASE_ALL_BINARY:

		/* skip leading "0x" or "0X" */

		if (srclen >= 2 && src[0] == '0' && (src[1] == 'x' || src[1] == 'X')) {
			src += 2;
			srclen -= 2;
		}

		/* ignore trailing blanks and nulls */
		/* FIXME is good to ignore null ?? */
		while (srclen > 0 && (src[srclen - 1] == ' ' || src[srclen - 1] == '\0'))
			--srclen;

		/* a binary string output will be half the length of */
		/* the string which represents it in hexadecimal     */

		ib = cr->cb.ib;
		if (desttype != TDS_CONVERT_BINARY) {
			cr->ib = (TDS_CHAR *) malloc((srclen + 1u) / 2u);
			test_alloc(cr->ib);
			ib = cr->ib;
		}
		return tds_char2hex(ib, desttype == TDS_CONVERT_BINARY ? cr->cb.len : 0xffffffffu, src, srclen);
		break;
	case SYBINT1:
	case SYBUINT1:
		if ((rc = string_to_int(src, src + srclen, &tds_i)) < 0)
			return rc;
		if (!IS_TINYINT(tds_i))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = tds_i;
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		if ((rc = string_to_int(src, src + srclen, &tds_i)) < 0)
			return rc;
		if (!IS_SMALLINT(tds_i))
			return TDS_CONVERT_OVERFLOW;
		cr->si = tds_i;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		if ((rc = string_to_int(src, src + srclen, &tds_i)) < 0)
			return rc;
		if (!IS_USMALLINT(tds_i))
			return TDS_CONVERT_OVERFLOW;
		cr->usi = tds_i;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		if ((rc = string_to_int(src, src + srclen, &tds_i)) < 0)
			return rc;
		cr->i = tds_i;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		if ((rc = string_to_int8(src, src + srclen, &tds_i8)) < 0)
			return rc;
		if (!IS_UINT(tds_i8))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = tds_i8;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		if ((rc = string_to_int8(src, src + srclen, &tds_i8)) < 0)
			return rc;
		cr->bi = tds_i8;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		if ((rc = string_to_uint8(src, src + srclen, &tds_ui8)) < 0)
			return rc;
		cr->ubi = tds_ui8;
		return sizeof(TDS_UINT8);
		break;
	case SYBFLT8:
		/* FIXME not null terminated */
		/* TODO check syntax and overflow */
		cr->f = atof(src);
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		/* FIXME not null terminated */
		/* TODO check syntax and overflow */
		cr->r = (TDS_REAL)atof(src);
		return sizeof(TDS_REAL);
		break;
	case SYBBIT:
	case SYBBITN:
		if ((rc = string_to_int(src, src + srclen, &tds_i)) < 0)
			return rc;
		cr->ti = tds_i ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBMONEY:
	case SYBMONEY4:

		/* TODO code similar to string_to_numeric... */
		i = 0;
		places = 0;
		point_found = 0;
		pend = src + srclen;

		/* skip leading blanks */
		for (ptr = src; ptr != pend && *ptr == ' '; ++ptr)
			continue;

		/* handle sign */
		switch (ptr != pend ? *ptr : 0) {
		case '-':
			mynumber[i++] = '-';
			/* fall through */
		case '+':
			while (++ptr != pend && *ptr == ' ')
				continue;
			break;
		}

		/* handle numbers which start with a lot of '0' */
		while (ptr != pend && *ptr == '0')
			++ptr;

		for (; ptr != pend; ptr++) {	/* deal with the rest */
			if (TDS_ISDIGIT(*ptr)) {	/* it's a number */
				/* no more than 4 decimal digits */
				if (places < 4)
					mynumber[i++] = *ptr;
				/* assure not buffer overflow */
				if (i > 22)
					return TDS_CONVERT_OVERFLOW;
				if (point_found) {	/* if we passed a decimal point */
					/* count digits after that point  */
					++places;
				}
			} else if (*ptr == '.') {	/* found a decimal point */
				if (point_found)	/* already had one. error */
					return TDS_CONVERT_SYNTAX;
				point_found = 1;
			} else	/* first invalid character */
				return TDS_CONVERT_SYNTAX;	/* lose the rest.          */
		}
		for (; places < 4; ++places)
			mynumber[i++] = '0';

		/* convert number and check for overflow */
		if (string_to_int8(mynumber, mynumber + i, &mymoney) < 0)
			return TDS_CONVERT_OVERFLOW;

		if (desttype == SYBMONEY) {
			cr->m.mny = mymoney;
			return sizeof(TDS_MONEY);
		} else {
			if (!IS_INT(mymoney))
				return TDS_CONVERT_OVERFLOW;
			cr->m4.mny4 = (TDS_INT) mymoney;
			return sizeof(TDS_MONEY4);
		}
		break;
	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBMSTIME:
	case SYBMSDATE:
	case SYBMSDATETIME2:
	case SYBMSDATETIMEOFFSET:
		/* FIXME not null terminated */
		return string_to_datetime(src, desttype, cr);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		return string_to_numeric(src, src + srclen, cr);
		break;
	case SYBUNIQUE:{
			unsigned n = 0;
			char c;

			/* 
			 * format:	 XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX 
			 * or 		{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
			 * or 		 XXXXXXXX-XXXX-XXXX-XXXXXXXXXXXXXXXX  
			 * or 		{XXXXXXXX-XXXX-XXXX-XXXXXXXXXXXXXXXX} 
			 * SQL seem to ignore the additional braces.
			 */
			if (srclen < (32 + 3))
				return TDS_CONVERT_SYNTAX;

			if (src[0] == '{') {
				TDS_UINT last = (src[8+1 + 4+1 + 4+1 + 4 + 1] == '-') ? 32+4+1 : 32+3+1;
				if (srclen <= last || src[last] != '}')
					return TDS_CONVERT_SYNTAX;
				++src;
			}

			/* 
			 * Test each character and get value.  
			 * sscanf works if the number terminates with less digits. 
			 */
			for (i = 0; i < 32 + 3; ++i) {
				c = src[i];
				switch (i) {
				case 8:
					if (c != '-')
						return TDS_CONVERT_SYNTAX;
					cr->u.Data1 = n;
					n = 0;
					break;
				case 8+1 + 4:
					if (c != '-')
						return TDS_CONVERT_SYNTAX;
					cr->u.Data2 = n;
					n = 0;
					break;
				case 8+1 + 4+1 + 4:
					if (c != '-')
						return TDS_CONVERT_SYNTAX;
					cr->u.Data3 = n;
					n = 0;
					break;
				case 8+1 + 4+1 + 4+1 + 4:
					/* skip last (optional) dash */
					if (c == '-') {
						if (--srclen < 32 + 3)
							return TDS_CONVERT_SYNTAX;
						c = (++src)[i];
					}
					/* fall through */
				default:
					n = n << 4;
					if (c >= '0' && c <= '9')
						n += c - '0';
					else {
						c &= 0x20 ^ 0xff;
						if (c >= 'A' && c <= 'F')
							n += c - ('A' - 10);
						else
							return TDS_CONVERT_SYNTAX;
					}
					if (i > (16 + 2) && !(i & 1)) {
						cr->u.Data4[(i >> 1) - 10] = n;
						n = 0;
					}
				}
			}
		}
		return sizeof(TDS_UNIQUE);
	default:
		return TDS_CONVERT_NOAVAIL;
		break;
	}			/* end switch */
}				/* tds_convert_char */

static TDS_INT
tds_convert_bit(const TDS_CHAR * src, int desttype, CONV_RESULT * cr)
{
	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 1, cr);
	}
	return tds_convert_int(src[0] ? 1 : 0, desttype, cr);
}

static TDS_INT
tds_convert_int1(const TDS_TINYINT * src, int desttype, CONV_RESULT * cr)
{
	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 1, cr);
	}
	return tds_convert_int(*src, desttype, cr);
}

static TDS_INT
tds_convert_int2(const TDS_SMALLINT * src, int desttype, CONV_RESULT * cr)
{
	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 2, cr);
	}
	return tds_convert_int(*src, desttype, cr);
}

static TDS_INT
tds_convert_uint2(const TDS_USMALLINT * src, int desttype, CONV_RESULT * cr)
{
	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 2, cr);
	}
	return tds_convert_int(*src, desttype, cr);
}

static TDS_INT
tds_convert_int4(const TDS_INT * src, int desttype, CONV_RESULT * cr)
{
	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 4, cr);
	}
	return tds_convert_int(*src, desttype, cr);
}

static TDS_INT
tds_convert_uint4(const TDS_UINT * src, int desttype, CONV_RESULT * cr)
{
	TDS_UINT8 num;

	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(src, 4, cr);
	}

	num = *src;
	return tds_convert_uint8(&num, desttype, cr);
}

static TDS_INT
tds_convert_int_numeric(unsigned char scale,
	unsigned char sign, TDS_UINT num, CONV_RESULT * cr)
{
	unsigned char orig_prec = cr->n.precision, orig_scale = cr->n.scale;
	cr->n.precision = 10;
	cr->n.scale = scale;
	cr->n.array[0] = sign;
	cr->n.array[1] = 0;
	TDS_PUT_UA4BE(&(cr->n.array[2]), num);
	return tds_numeric_change_prec_scale(&(cr->n), orig_prec, orig_scale);
}

static TDS_INT
tds_convert_int8_numeric(unsigned char scale,
	unsigned char sign, TDS_UINT8 num, CONV_RESULT * cr)
{
	unsigned char orig_prec = cr->n.precision, orig_scale = cr->n.scale;
	cr->n.precision = 20;
	cr->n.scale = scale;
	cr->n.array[0] = sign;
	cr->n.array[1] = 0;
	TDS_PUT_UA4BE(&(cr->n.array[2]), (TDS_UINT) (num >> 32));
	TDS_PUT_UA4BE(&(cr->n.array[6]), (TDS_UINT) num);
	return tds_numeric_change_prec_scale(&(cr->n), orig_prec, orig_scale);
}

static TDS_INT
tds_convert_int(TDS_INT num, int desttype, CONV_RESULT * cr)
{
	TDS_CHAR tmp_str[16];

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(tmp_str, "%d", num);
		return string_to_result(tmp_str, cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		if (!IS_TINYINT(num))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = (TDS_TINYINT) num;
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		if (!IS_SMALLINT(num))
			return TDS_CONVERT_OVERFLOW;
		cr->si = num;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		if (!IS_USMALLINT(num))
			return TDS_CONVERT_OVERFLOW;
		cr->usi = (TDS_USMALLINT) num;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		cr->i = num;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		if (num < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) num;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		cr->bi = num;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		if (num < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = (TDS_UINT8) num;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = num ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBFLT8:
		cr->f = num;
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		cr->r = (TDS_REAL) num;
		return sizeof(TDS_REAL);
		break;
	case SYBMONEY4:
		if (num > 214748 || num < -214748)
			return TDS_CONVERT_OVERFLOW;
		cr->m4.mny4 = num * 10000;
		return sizeof(TDS_MONEY4);
		break;
	case SYBMONEY:
		cr->m.mny = (TDS_INT8) num *10000;

		return sizeof(TDS_MONEY);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		if (num < 0)
			return tds_convert_int_numeric(0, 1, (TDS_UINT) -num, cr);
		return tds_convert_int_numeric(0, 0, (TDS_UINT) num, cr);
		break;
		/* handled by upper layer */
	case CASE_ALL_BINARY:
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_int8(const TDS_INT8 *src, int desttype, CONV_RESULT * cr)
{
	TDS_INT8 buf;
	TDS_CHAR tmp_str[24];

	memcpy(&buf, src, sizeof(buf));
	if (IS_INT(buf))
		return tds_convert_int((TDS_INT) buf, desttype, cr);

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(tmp_str, "%" PRId64, buf);
		return string_to_result(tmp_str, cr);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_INT8), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBMONEY4:
		return TDS_CONVERT_OVERFLOW;
		break;
	case SYBUINT4:
		if (!IS_UINT(buf))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) buf;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		cr->bi = buf;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		if (buf < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = (TDS_UINT8) buf;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = buf ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBFLT8:
		cr->f = (TDS_FLOAT) buf;
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		cr->r = (TDS_REAL) buf;
		return sizeof(TDS_REAL);
		break;
	case SYBMONEY:
		if (buf > (TDS_INT8_MAX / 10000) || buf < (TDS_INT8_MIN / 10000))
			return TDS_CONVERT_OVERFLOW;
		cr->m.mny = buf * 10000;
		return sizeof(TDS_MONEY);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		if (buf < 0)
			return tds_convert_int8_numeric(0, 1, -buf, cr);
		return tds_convert_int8_numeric(0, 0, buf, cr);
		break;
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_uint8(const TDS_UINT8 *src, int desttype, CONV_RESULT * cr)
{
	TDS_UINT8 buf;
	TDS_CHAR tmp_str[24];

	memcpy(&buf, src, sizeof(buf));
	/* IS_INT does not work here due to unsigned/signed conversions */
	if (buf <= (TDS_UINT8) TDS_INT_MAX)
		return tds_convert_int((TDS_INT) buf, desttype, cr);

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(tmp_str, "%" PRIu64, buf);
		return string_to_result(tmp_str, cr);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_INT8), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBMONEY4:
		return TDS_CONVERT_OVERFLOW;
		break;
	case SYBUINT4:
		if (!IS_UINT(buf))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) buf;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		if (buf > (TDS_UINT8) TDS_INT8_MAX)
			return TDS_CONVERT_OVERFLOW;
		cr->bi = (TDS_INT8) buf;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		cr->ubi = buf;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = buf ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBFLT8:
		cr->f = (TDS_FLOAT) buf;
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		cr->r = (TDS_REAL) buf;
		return sizeof(TDS_REAL);
		break;
	case SYBMONEY:
		if (buf > (TDS_INT8_MAX / 10000))
			return TDS_CONVERT_OVERFLOW;
		cr->m.mny = buf * 10000;
		return sizeof(TDS_MONEY);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		return tds_convert_int8_numeric(0, 0, buf, cr);
		break;
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_numeric(const TDS_NUMERIC * src, TDS_INT srclen, int desttype, CONV_RESULT * cr)
{
	char tmpstr[MAXPRECISION];
	TDS_INT i, ret;
	TDS_INT8 bi;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		if (tds_numeric_to_string(src, tmpstr) < 0)
			return TDS_CONVERT_FAIL;
		return string_to_result(tmpstr, cr);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_NUMERIC), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 3, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[1] || (cr->n.array[0] && cr->n.array[2]))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = cr->n.array[2];
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 5, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		i = TDS_GET_UA2BE(&(cr->n.array[2]));
		if (cr->n.array[0])
			i = -i;
		if (((i >> 15) ^ cr->n.array[0]) & 1)
			return TDS_CONVERT_OVERFLOW;
		cr->si = (TDS_SMALLINT) i;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 5, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[0] || cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		i = TDS_GET_UA2BE(&(cr->n.array[2]));
		cr->usi = (TDS_USMALLINT) i;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 10, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		i = TDS_GET_UA4BE(&(cr->n.array[2]));
		if (cr->n.array[0])
			i = -i;
		if (((i >> 31) ^ cr->n.array[0]) & 1)
			return TDS_CONVERT_OVERFLOW;
		cr->i = i;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 10, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[0] || cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		i = TDS_GET_UA4BE(&(cr->n.array[2]));
		cr->ui = i;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 20, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		bi = TDS_GET_UA4BE(&(cr->n.array[2]));
		bi = (bi << 32) + TDS_GET_UA4BE(&(cr->n.array[6]));
		if (cr->n.array[0])
			bi = -bi;
		if (((bi >> 63) ^ cr->n.array[0]) & 1)
			return TDS_CONVERT_OVERFLOW;
		cr->bi = bi;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 20, 0);
		if (ret < 0)
			return ret;
		if (cr->n.array[0] || cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		bi = TDS_GET_UA4BE(&(cr->n.array[2]));
		bi = (bi << 32) + TDS_GET_UA4BE(&(cr->n.array[6]));
		cr->ubi = bi;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = 0;
		for (i = tds_numeric_bytes_per_prec[src->precision]; --i > 0;)
			if (src->array[i] != 0) {
				cr->ti = 1;
				break;
			}
		return sizeof(TDS_TINYINT);
		break;
	case SYBMONEY4:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 10, 4);
		if (ret < 0)
			return ret;
		if (cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		i = TDS_GET_UA4BE(&(cr->n.array[2]));
		if (cr->n.array[0])
			i = -i;
		if (((i >> 31) ^ cr->n.array[0]) & 1)
			return TDS_CONVERT_OVERFLOW;
		cr->m4.mny4 = i;
		return sizeof(TDS_MONEY4);
		break;
	case SYBMONEY:
		cr->n = *src;
		ret = tds_numeric_change_prec_scale(&(cr->n), 20, 4);
		if (ret < 0)
			return ret;
		if (cr->n.array[1])
			return TDS_CONVERT_OVERFLOW;
		bi = TDS_GET_UA4BE(&(cr->n.array[2]));
		bi = (bi << 32) + TDS_GET_UA4BE(&(cr->n.array[6]));
		if (cr->n.array[0])
			bi = -bi;
		if (((bi >> 63) ^ cr->n.array[0]) & 1)
			return TDS_CONVERT_OVERFLOW;
		cr->m.mny = bi;
		return sizeof(TDS_MONEY);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		{
			unsigned char prec = cr->n.precision, scale = cr->n.scale;
			cr->n = *src;
			return tds_numeric_change_prec_scale(&(cr->n), prec, scale);
		}
		break;
	case SYBFLT8:
		if (tds_numeric_to_string(src, tmpstr) < 0)
			return TDS_CONVERT_FAIL;
		cr->f = atof(tmpstr);
		return 8;
		break;
	case SYBREAL:
		if (tds_numeric_to_string(src, tmpstr) < 0)
			return TDS_CONVERT_FAIL;
		cr->r = (TDS_REAL) atof(tmpstr);
		return 4;
		break;
		/* TODO conversions to money */
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_money4(const TDS_MONEY4 * src, int srclen, int desttype, CONV_RESULT * cr)
{
	TDS_MONEY4 mny;
	long dollars;
	char tmp_str[33];
	char *p;

	mny = *src;
	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		/*
		 * round to 2 decimal digits
		 * rounding with dollars = (mny.mny4 + 5000) /10000
		 * can give arithmetic overflow so I use
		 * dollars = (mny.mny4/50 + 1)/2 
		 */
		/* TODO round also all conversions to int and from money ?? */
		p = tmp_str;
		if (mny.mny4 < 0) {
			*p++ = '-';
			/* here (-mny.mny4 / 50 + 1 ) / 2 can cause overflow in -mny.mny4 */
			dollars = -(mny.mny4 / 50 - 1 ) / 2;
		} else {
			dollars = (mny.mny4 / 50 + 1 ) / 2;
		}
		/* print only 2 decimal digits as server does */
		sprintf(p, "%ld.%02lu", dollars / 100, dollars % 100);
		return string_to_result(tmp_str, cr);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_MONEY4), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		dollars = mny.mny4 / 10000;
		if (!IS_TINYINT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = (TDS_TINYINT) dollars;
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		dollars = mny.mny4 / 10000;
		if (!IS_SMALLINT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->si = (TDS_SMALLINT) dollars;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		dollars = mny.mny4 / 10000;
		if (!IS_USMALLINT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->usi = (TDS_USMALLINT) dollars;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		cr->i = mny.mny4 / 10000;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		dollars = mny.mny4 / 10000;
		if (!IS_UINT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = dollars;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		cr->bi = mny.mny4 / 10000;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		dollars = mny.mny4 / 10000;
		if (!IS_UINT8(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = dollars;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = mny.mny4 ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBFLT8:
		cr->f = ((TDS_FLOAT) mny.mny4) / 10000.0;
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		cr->r = (TDS_REAL) (mny.mny4 / 10000.0);
		return sizeof(TDS_REAL);
		break;
	case SYBMONEY:
		cr->m.mny = (TDS_INT8) mny.mny4;
		return sizeof(TDS_MONEY);
		break;
	case SYBMONEY4:
		cr->m4 = mny;
		return sizeof(TDS_MONEY4);
		break;
	case SYBDECIMAL:
	case SYBNUMERIC:
		if (mny.mny4 < 0)
			return tds_convert_int_numeric(4, 1, (TDS_UINT) -mny.mny4, cr);
		return tds_convert_int_numeric(4, 0, (TDS_UINT) mny.mny4, cr);
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		return TDS_CONVERT_NOAVAIL;
		break;
	}
	return TDS_CONVERT_FAIL;
}

static TDS_INT
tds_convert_money(const TDS_MONEY * src, int desttype, CONV_RESULT * cr)
{
	char *s;

	TDS_INT8 mymoney, dollars;
	char tmpstr[64];

	tdsdump_log(TDS_DBG_FUNC, "tds_convert_money()\n");
	mymoney = ((TDS_INT8) src->tdsoldmoney.mnyhigh << 32) | src->tdsoldmoney.mnylow;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		s = tds_money_to_string((const TDS_MONEY *) src, tmpstr);
		return string_to_result(s, cr);
		break;

	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_MONEY), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		if (mymoney <= -10000 || mymoney >= 256 * 10000)
			return TDS_CONVERT_OVERFLOW;
		/* TODO: round ?? */
		cr->ti = (TDS_TINYINT) (((TDS_INT) mymoney) / 10000);
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		if (mymoney <= -32769 * 10000 || mymoney >= 32768 * 10000)
			return TDS_CONVERT_OVERFLOW;
		cr->si = (TDS_SMALLINT) (((TDS_INT) mymoney) / 10000);
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		if (mymoney <= -1 * 10000 || mymoney >= 65536 * 10000)
			return TDS_CONVERT_OVERFLOW;
		cr->usi = (TDS_USMALLINT) (((TDS_INT) mymoney) / 10000);
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		dollars = mymoney / 10000;
		if (!IS_INT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->i = (TDS_INT) dollars;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		dollars = mymoney / 10000;
		if (!IS_UINT(dollars))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) dollars;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		cr->bi = mymoney / 10000;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		dollars = mymoney / 10000;
		if (dollars < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = (TDS_UINT8) dollars;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = mymoney ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;
	case SYBFLT8:
		cr->f = ((TDS_FLOAT) mymoney) / 10000.0;
		return sizeof(TDS_FLOAT);
		break;
	case SYBREAL:
		cr->r = (TDS_REAL) (mymoney / 10000.0);
		return sizeof(TDS_REAL);
		break;
	case SYBMONEY4:
		if (!IS_INT(mymoney))
			return TDS_CONVERT_OVERFLOW;
		cr->m4.mny4 = (TDS_INT) mymoney;
		return sizeof(TDS_MONEY4);
		break;
	case SYBMONEY:
		cr->m.mny = mymoney;
		return sizeof(TDS_MONEY);
		break;
	case SYBDECIMAL:
	case SYBNUMERIC:
		if (mymoney < 0)
			return tds_convert_int8_numeric(4, 1, -mymoney, cr);
		return tds_convert_int8_numeric(4, 0, mymoney, cr);
		break;
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_datetimeall(const TDSCONTEXT * tds_ctx, int srctype, const TDS_DATETIMEALL * dta, int desttype, CONV_RESULT * cr)
{
	char whole_date_string[64];
	TDSDATEREC when;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		tds_datecrack(srctype, dta, &when);
		tds_strftime(whole_date_string, sizeof(whole_date_string), tds_ctx->locale->date_fmt, &when, 
		             dta->time_prec);

		return string_to_result(whole_date_string, cr);
	case CASE_ALL_BINARY:
		/* TODO */
		break;
	case SYBDATETIME:
		/* TODO check overflow */
		cr->dt.dtdays = dta->date;
		cr->dt.dttime = (dta->time * 3u + 50000u) / 100000u;
		return sizeof(TDS_DATETIME);
	case SYBDATETIME4:
		/* TODO check overflow */
		cr->dt4.days = dta->date;
		cr->dt4.minutes = (dta->time + 30u * 10000000u) / (60u * 10000000u);
		return sizeof(TDS_DATETIME4);
	case SYBMSDATETIMEOFFSET:
	case SYBMSDATE:
	case SYBMSTIME:
	case SYBMSDATETIME2:
		cr->dta = *dta;
		return sizeof(TDS_DATETIMEALL);
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBBIT:
	case SYBBITN:
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBUINT4:
	case SYBINT8:
	case SYBUINT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBNUMERIC:
	case SYBDECIMAL:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_datetime(const TDSCONTEXT * tds_ctx, const TDS_DATETIME * dt, int desttype, CONV_RESULT * cr)
{
	char whole_date_string[64];
	TDSDATEREC when;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		tds_datecrack(SYBDATETIME, dt, &when);
		tds_strftime(whole_date_string, sizeof(whole_date_string), tds_ctx->locale->date_fmt, &when, 3);

		return string_to_result(whole_date_string, cr);
	case CASE_ALL_BINARY:
		return binary_to_result(dt, sizeof(TDS_DATETIME), cr);
	case SYBDATETIME:
		cr->dt = *dt;
		return sizeof(TDS_DATETIME);
	case SYBDATETIME4:
		cr->dt4.days = dt->dtdays;
		cr->dt4.minutes = (dt->dttime / 300) / 60;
		return sizeof(TDS_DATETIME4);
	case SYBMSDATETIMEOFFSET:
	case SYBMSDATE:
	case SYBMSTIME:
	case SYBMSDATETIME2:
		memset(&cr->dta, 0, sizeof(cr->dta));
		/* FIXME must be 0 if came from DATETIME4 */
		cr->dta.time_prec = 3;
		if (desttype == SYBMSDATETIMEOFFSET)
			cr->dta.has_offset = 1;
		if (desttype != SYBMSDATE) {
			cr->dta.has_time = 1;
			cr->dta.time_prec = 3;
			cr->dta.time = ((TDS_UINT8) dt->dttime) * 100000u / 3u;
		}
		if (desttype != SYBMSTIME) {
			cr->dta.has_date = 1;
			cr->dta.date = dt->dtdays;
		}
		return sizeof(TDS_DATETIMEALL);
		/* conversions not allowed */
	case SYBUNIQUE:
	case SYBBIT:
	case SYBBITN:
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBUINT4:
	case SYBINT8:
	case SYBUINT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBNUMERIC:
	case SYBDECIMAL:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_datetime4(const TDSCONTEXT * tds_ctx, const TDS_DATETIME4 * dt4, int desttype, CONV_RESULT * cr)
{
	TDS_DATETIME dt;

	switch (desttype) {
	case CASE_ALL_BINARY:
		return binary_to_result(dt4, sizeof(TDS_DATETIME4), cr);
	case SYBDATETIME4:
		cr->dt4 = *dt4;
		return sizeof(TDS_DATETIME4);
	default:
		break;
	}

	/* convert to DATETIME and use tds_convert_datetime */
	dt.dtdays = dt4->days;
	dt.dttime = dt4->minutes * (60u * 300u);
	return tds_convert_datetime(tds_ctx, &dt, desttype, cr);
}

static TDS_INT
tds_convert_real(const TDS_REAL* src, int desttype, CONV_RESULT * cr)
{
	TDS_REAL the_value;

/* FIXME how many big should be this buffer ?? */
	char tmp_str[128];
	TDS_INT mymoney4;
	TDS_INT8 mymoney;

	the_value = *src;

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(tmp_str, "%.7g", the_value);
		return string_to_result(tmp_str, cr);
		break;

	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_REAL), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		if (!IS_TINYINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = (TDS_TINYINT) the_value;
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		if (!IS_SMALLINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->si = (TDS_SMALLINT) the_value;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		if (!IS_USMALLINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->usi = (TDS_USMALLINT) the_value;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		if (!IS_INT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->i = (TDS_INT) the_value;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		if (!IS_UINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) the_value;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		if (the_value > (TDS_REAL) TDS_INT8_MAX || the_value < (TDS_REAL) TDS_INT8_MIN)
			return TDS_CONVERT_OVERFLOW;
		cr->bi = (TDS_INT8) the_value;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		if (the_value > (TDS_REAL) TDS_UINT8_MAX || the_value < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = (TDS_UINT8) the_value;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = the_value ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;

	case SYBFLT8:
		cr->f = the_value;
		return sizeof(TDS_FLOAT);
		break;

	case SYBREAL:
		cr->r = the_value;
		return sizeof(TDS_REAL);
		break;

	case SYBMONEY:
		if (the_value > (TDS_REAL) (TDS_INT8_MAX / 10000) || the_value < (TDS_REAL) (TDS_INT8_MIN / 10000))
			return TDS_CONVERT_OVERFLOW;
		mymoney = (TDS_INT8) (the_value * 10000);
		cr->m.mny = mymoney;
		return sizeof(TDS_MONEY);
		break;

	case SYBMONEY4:
		if (the_value > (TDS_REAL) (TDS_INT_MAX / 10000) || the_value < (TDS_REAL) (TDS_INT_MIN / 10000))
			return TDS_CONVERT_OVERFLOW;
		mymoney4 = (TDS_INT) (the_value * 10000);
		cr->m4.mny4 = mymoney4;
		return sizeof(TDS_MONEY4);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		sprintf(tmp_str, "%.*f", cr->n.scale, the_value);
		return stringz_to_numeric(tmp_str, cr);
		break;
		/* not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

/*
 * TODO: emit SYBECLPR errors: "Data conversion resulted in loss of precision".  
 * There are many places where this would be correct to do, but the test is tedious 
 * (convert e.g. 1.5 -> SYBINT and test if output == input) and we don't have a good, 
 * API-independent alternative to tds_client_msg().  Postponed until then.  
 */
static TDS_INT
tds_convert_flt8(const TDS_FLOAT* src, int desttype, CONV_RESULT * cr)
{
	TDS_FLOAT the_value;
	char tmp_str[25];

	memcpy(&the_value, src, 8);
	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(tmp_str, "%.16g", the_value);
		return string_to_result(tmp_str, cr);
		break;

	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_FLOAT), cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		if (!IS_TINYINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->ti = (TDS_TINYINT) the_value;
		return sizeof(TDS_TINYINT);
		break;
	case SYBINT2:
		if (!IS_SMALLINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->si = (TDS_SMALLINT) the_value;
		return sizeof(TDS_SMALLINT);
		break;
	case SYBUINT2:
		if (!IS_USMALLINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->usi = (TDS_USMALLINT) the_value;
		return sizeof(TDS_USMALLINT);
		break;
	case SYBINT4:
		if (!IS_INT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->i = (TDS_INT) the_value;
		return sizeof(TDS_INT);
		break;
	case SYBUINT4:
		if (!IS_UINT(the_value))
			return TDS_CONVERT_OVERFLOW;
		cr->ui = (TDS_UINT) the_value;
		return sizeof(TDS_UINT);
		break;
	case SYBINT8:
		if (the_value > (TDS_FLOAT) TDS_INT8_MAX || the_value < (TDS_FLOAT) TDS_INT8_MIN)
			return TDS_CONVERT_OVERFLOW;
		cr->bi = (TDS_INT8) the_value;
		return sizeof(TDS_INT8);
		break;
	case SYBUINT8:
		if (the_value > (TDS_FLOAT) TDS_UINT8_MAX || the_value < 0)
			return TDS_CONVERT_OVERFLOW;
		cr->ubi = (TDS_UINT8) the_value;
		return sizeof(TDS_UINT8);
		break;
	case SYBBIT:
	case SYBBITN:
		cr->ti = the_value ? 1 : 0;
		return sizeof(TDS_TINYINT);
		break;

	case SYBMONEY:
		if (the_value > (TDS_FLOAT) (TDS_INT8_MAX / 10000) || the_value < (TDS_FLOAT) (TDS_INT8_MIN / 10000))
			return TDS_CONVERT_OVERFLOW;
		cr->m.mny = (TDS_INT8) (the_value * 10000);

		return sizeof(TDS_MONEY);
		break;
	case SYBMONEY4:
		if (the_value > (TDS_FLOAT) (TDS_INT_MAX / 10000) || the_value < (TDS_FLOAT) (TDS_INT_MIN / 10000))
			return TDS_CONVERT_OVERFLOW;
		cr->m4.mny4 = (TDS_INT) (the_value * 10000);
		return sizeof(TDS_MONEY4);
		break;
	case SYBREAL:
		/* TODO check overflow */
		cr->r = (TDS_REAL)the_value;
		return sizeof(TDS_REAL);
		break;
	case SYBFLT8:
		cr->f = the_value;
		return sizeof(TDS_FLOAT);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		sprintf(tmp_str, "%.16g", the_value);
		return stringz_to_numeric(tmp_str, cr);
		break;
		/* not allowed */
	case SYBUNIQUE:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

static TDS_INT
tds_convert_unique(const TDS_CHAR * src, TDS_INT srclen, int desttype, CONV_RESULT * cr)
{
	/*
	 * raw data is equivalent to structure and always aligned, 
	 * so this cast is portable
	 */
	const TDS_UNIQUE *u = (const TDS_UNIQUE *) src;
	char buf[37];

	switch (desttype) {
	case TDS_CONVERT_CHAR:
	case CASE_ALL_CHAR:
		sprintf(buf, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			(int) u->Data1, (int) u->Data2, (int) u->Data3,
			u->Data4[0], u->Data4[1], u->Data4[2], u->Data4[3], u->Data4[4], u->Data4[5], u->Data4[6], u->Data4[7]);
		return string_to_result(buf, cr);
		break;
	case CASE_ALL_BINARY:
		return binary_to_result(src, sizeof(TDS_UNIQUE), cr);
		break;
	case SYBUNIQUE:
		/*
		 * Here we can copy raw to structure because we adjust
		 * byte order in tds_swap_datatype
		 */
		memcpy(&(cr->u), src, sizeof(TDS_UNIQUE));
		return sizeof(TDS_UNIQUE);
		break;
		/* no not warning for not convertible types */
	case SYBBIT:
	case SYBBITN:
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBUINT2:
	case SYBINT4:
	case SYBUINT4:
	case SYBINT8:
	case SYBUINT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBDATETIME:
	case SYBDATETIMN:
	case SYBREAL:
	case SYBFLT8:
	default:
		break;
	}
	return TDS_CONVERT_NOAVAIL;
}

/**
 * tds_convert
 * convert a type to another.
 * If you convert to SYBDECIMAL/SYBNUMERIC you MUST initialize precision 
 * and scale of cr.
 * Do not expect strings to be zero terminated. Databases support zero inside
 * string. Using strlen may result on data loss or even a segmentation fault.
 * Instead, use memcpy to copy destination using length returned.
 * This function does not handle NULL, srclen should be >0.  Client libraries handle NULLs each in their own way. 
 * @param tds_ctx  context (used in conversion to data and to return messages)
 * @param srctype  type of source
 * @param src      pointer to source data to convert
 * @param srclen   length in bytes of source (not counting terminator or strings)
 * @param desttype type of destination
 * @param cr       structure to hold result
 * @return length of result or TDS_CONVERT_* failure code on failure. All TDS_CONVERT_* constants are <0.
 */
TDS_INT
tds_convert(const TDSCONTEXT * tds_ctx, int srctype, const TDS_CHAR * src, TDS_UINT srclen, int desttype, CONV_RESULT * cr)
{
	TDS_INT length = 0;

	assert(srclen >= 0 && srclen <= 2147483647u);

	if (srctype == SYBVARIANT) {
		TDSVARIANT *v = (TDSVARIANT*) src;
		srctype = v->type;
		src = v->data;
		srclen = v->data_len;
	}

	switch (srctype) {
	case CASE_ALL_CHAR:
		length = tds_convert_char(src, srclen, desttype, cr);
		break;
	case SYBMONEY4:
		length = tds_convert_money4((const TDS_MONEY4 *) src, srclen, desttype, cr);
		break;
	case SYBMONEY:
		length = tds_convert_money((const TDS_MONEY *) src, desttype, cr);
		break;
	case SYBNUMERIC:
	case SYBDECIMAL:
		length = tds_convert_numeric((const TDS_NUMERIC *) src, srclen, desttype, cr);
		break;
	case SYBBIT:
	case SYBBITN:
		length = tds_convert_bit(src, desttype, cr);
		break;
	case SYBINT1:
	case SYBUINT1:
		length = tds_convert_int1((const TDS_TINYINT *) src, desttype, cr);
		break;
	case SYBINT2:
		length = tds_convert_int2((const TDS_SMALLINT *) src, desttype, cr);
		break;
	case SYBUINT2:
		length = tds_convert_uint2((const TDS_USMALLINT *) src, desttype, cr);
		break;
	case SYBINT4:
		length = tds_convert_int4((const TDS_INT *) src, desttype, cr);
		break;
	case SYBUINT4:
		length = tds_convert_uint4((const TDS_UINT *) src, desttype, cr);
		break;
	case SYBINT8:
		length = tds_convert_int8((const TDS_INT8 *) src, desttype, cr);
		break;
	case SYBUINT8:
		length = tds_convert_uint8((const TDS_UINT8 *) src, desttype, cr);
		break;
	case SYBREAL:
		length = tds_convert_real((const TDS_REAL *) src, desttype, cr);
		break;
	case SYBFLT8:
		length = tds_convert_flt8((const TDS_FLOAT *) src, desttype, cr);
		break;
	case SYBMSTIME:
	case SYBMSDATE:
	case SYBMSDATETIME2:
	case SYBMSDATETIMEOFFSET:
		length = tds_convert_datetimeall(tds_ctx, srctype, (const TDS_DATETIMEALL *) src, desttype, cr);
		break;
	case SYBDATETIME:
		length = tds_convert_datetime(tds_ctx, (const TDS_DATETIME* ) src, desttype, cr);
		break;
	case SYBDATETIME4:
		length = tds_convert_datetime4(tds_ctx, (const TDS_DATETIME4* ) src, desttype, cr);
		break;
	case SYBLONGBINARY:
	case CASE_ALL_BINARY:
		length = tds_convert_binary((const TDS_UCHAR *) src, srclen, desttype, cr);
		break;
	case SYBUNIQUE:
		length = tds_convert_unique(src, srclen, desttype, cr);
		break;
	case SYBNVARCHAR:
	case SYBNTEXT:
	default:
		return TDS_CONVERT_NOAVAIL;
		break;
	}

/* fix MONEY case */
#if !defined(WORDS_BIGENDIAN)
	if (length > 0 && desttype == SYBMONEY) {
		cr->m.mny = ((TDS_UINT8) cr->m.mny) >> 32 | (cr->m.mny << 32);
	}
#endif
	return length;
}

static int
string_to_datetime(const char *instr, int desttype, CONV_RESULT * cr)
{
	enum states
	{ GOING_IN_BLIND,
		PUT_NUMERIC_IN_CONTEXT,
		DOING_ALPHABETIC_DATE,
		STRING_GARBLED
	};

	char *in;
	char *tok;
	char *lasts;
	char last_token[32];
	int monthdone = 0;
	int yeardone = 0;
	int mdaydone = 0;

	struct tds_time t;

	unsigned int dt_time;
	TDS_INT dt_days;
	int i;

	int current_state;

	memset(&t, '\0', sizeof(t));
	t.tm_mday = 1;

	in = (char *) malloc(strlen(instr) + 1);
	test_alloc(in);
	strcpy(in, instr);

	tok = strtok_r(in, " ,", &lasts);

	current_state = GOING_IN_BLIND;

	while (tok != NULL) {

		tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: current_state = %d\n", current_state);
		switch (current_state) {
		case GOING_IN_BLIND:
			/* If we have no idea of current context, then if we have */
			/* encountered a purely alphabetic string, it MUST be an  */
			/* alphabetic month name or prefix...                     */

			if (is_alphabetic(tok)) {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: is_alphabetic\n");
				if (store_monthname(tok, &t) >= 0) {
					monthdone++;
					current_state = DOING_ALPHABETIC_DATE;
				} else {
					current_state = STRING_GARBLED;
				}
			}

			/* ...whereas if it is numeric, it could be a number of   */
			/* things...                                              */

			else if (is_numeric(tok)) {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: is_numeric\n");
				switch (strlen(tok)) {
					/* in this context a 4 character numeric can   */
					/* ONLY be the year part of an alphabetic date */

				case 4:
					store_year(atoi(tok), &t);
					yeardone++;
					current_state = DOING_ALPHABETIC_DATE;
					break;

					/* whereas these could be the hour part of a   */
					/* time specification ( 4 PM ) or the leading  */
					/* day part of an alphabetic date ( 15 Jan )   */

				case 2:
				case 1:
					strcpy(last_token, tok);
					current_state = PUT_NUMERIC_IN_CONTEXT;
					break;

					/* this must be a [YY]YYMMDD date             */

				case 6:
				case 8:
					if (store_yymmdd_date(tok, &t))
						current_state = GOING_IN_BLIND;
					else
						current_state = STRING_GARBLED;
					break;

					/* anything else is nonsense...               */

				default:
					current_state = STRING_GARBLED;
					break;
				}
			}

			/* it could be [M]M/[D]D/[YY]YY format              */

			else if (is_numeric_dateformat(tok)) {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: is_numeric_dateformat\n");
				store_numeric_date(tok, &t);
				current_state = GOING_IN_BLIND;
			} else if (is_dd_mon_yyyy(tok)) {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: is_dd_mon_yyyy\n");
				store_dd_mon_yyy_date(tok, &t);
				current_state = GOING_IN_BLIND;
			} else if (is_timeformat(tok)) {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: is_timeformat\n");
				store_time(tok, &t);
				current_state = GOING_IN_BLIND;
			} else {
				tdsdump_log(TDS_DBG_INFO1, "string_to_datetime: string_garbled\n");
				current_state = STRING_GARBLED;
			}

			break;	/* end of GOING_IN_BLIND */

		case DOING_ALPHABETIC_DATE:

			if (is_alphabetic(tok)) {
				if (!monthdone && store_monthname(tok, &t) >= 0) {
					monthdone++;
					if (monthdone && yeardone && mdaydone)
						current_state = GOING_IN_BLIND;
					else
						current_state = DOING_ALPHABETIC_DATE;
				} else {
					current_state = STRING_GARBLED;
				}
			} else if (is_numeric(tok)) {
				if (mdaydone && yeardone)
					current_state = STRING_GARBLED;
				else
					switch (strlen(tok)) {
					case 4:
						store_year(atoi(tok), &t);
						yeardone++;
						if (monthdone && yeardone && mdaydone)
							current_state = GOING_IN_BLIND;
						else
							current_state = DOING_ALPHABETIC_DATE;
						break;

					case 2:
					case 1:
						if (!mdaydone) {
							store_mday(tok, &t);

							mdaydone++;
							if (monthdone && yeardone && mdaydone)
								current_state = GOING_IN_BLIND;
							else
								current_state = DOING_ALPHABETIC_DATE;
						} else {
							store_year(atoi(tok), &t);
							yeardone++;
							if (monthdone && yeardone && mdaydone)
								current_state = GOING_IN_BLIND;
							else
								current_state = DOING_ALPHABETIC_DATE;
						}
						break;

					default:
						current_state = STRING_GARBLED;
					}
			} else {
				current_state = STRING_GARBLED;
			}

			break;	/* end of DOING_ALPHABETIC_DATE */

		case PUT_NUMERIC_IN_CONTEXT:

			if (is_alphabetic(tok)) {
				if (store_monthname(tok, &t) >= 0) {
					store_mday(last_token, &t);
					mdaydone++;
					monthdone++;
					if (monthdone && yeardone && mdaydone)
						current_state = GOING_IN_BLIND;
					else
						current_state = DOING_ALPHABETIC_DATE;
				} else if (is_ampm(tok)) {
					store_hour(last_token, tok, &t);
					current_state = GOING_IN_BLIND;
				} else {
					current_state = STRING_GARBLED;
				}
			} else if (is_numeric(tok)) {
				switch (strlen(tok)) {
				case 4:
				case 2:
					store_mday(last_token, &t);
					mdaydone++;
					store_year(atoi(tok), &t);
					yeardone++;

					if (monthdone && yeardone && mdaydone)
						current_state = GOING_IN_BLIND;
					else
						current_state = DOING_ALPHABETIC_DATE;
					break;

				default:
					current_state = STRING_GARBLED;
				}
			} else {
				current_state = STRING_GARBLED;
			}

			break;	/* end of PUT_NUMERIC_IN_CONTEXT */

		case STRING_GARBLED:

			tdsdump_log(TDS_DBG_INFO1,
				    "error_handler:  Attempt to convert data stopped by syntax error in source field \n");
			free(in);
			return TDS_CONVERT_SYNTAX;
		}

		tok = strtok_r(NULL, " ,", &lasts);
	}

	i = (t.tm_mon - 13) / 12;
	dt_days = 1461 * (t.tm_year + 1900 + i) / 4 +
		(367 * (t.tm_mon - 1 - 12 * i)) / 12 - (3 * ((t.tm_year + 2000 + i) / 100)) / 4 + t.tm_mday - 693932;

	free(in);

	/* TODO check for overflow */
	if (desttype == SYBDATETIME) {
		cr->dt.dtdays = dt_days;
		dt_time = (t.tm_hour * 60 + t.tm_min) * 60 + t.tm_sec;
		cr->dt.dttime = dt_time * 300 + (t.tm_ns / 1000000u * 300 + 150) / 1000;
		return sizeof(TDS_DATETIME);
	}
	if (desttype == SYBDATETIME4) {
		cr->dt4.days = dt_days;
		cr->dt4.minutes = t.tm_hour * 60 + t.tm_min;
		return sizeof(TDS_DATETIME4);
	}

	cr->dta.has_offset = 0;
	cr->dta.offset = 0;
	cr->dta.has_date = 1;
	cr->dta.date = dt_days;
	cr->dta.has_time = 1;
	cr->dta.time_prec = 7; /* TODO correct value */
	dt_time = (t.tm_hour * 60 + t.tm_min) * 60 + t.tm_sec;
	cr->dta.time = dt_time * 10000000u + t.tm_ns / 100u;
	return sizeof(TDS_DATETIMEALL);
}

static int
stringz_to_numeric(const char *instr, CONV_RESULT * cr)
{
	return string_to_numeric(instr, instr + strlen(instr), cr);
}

static int
string_to_numeric(const char *instr, const char *pend, CONV_RESULT * cr)
{
	char mynumber[(MAXPRECISION + 7) / 8 * 8 + 8];

	/* num packaged 8 digit, see below for detail */
	TDS_UINT packed_num[(MAXPRECISION + 7) / 8];

	char *ptr;
	const char *pstr;
	int old_digits_left, digits_left, digit_found = 0;

	int i = 0;
	int j = 0;
	int bytes, places;
	unsigned char sign;

	/* FIXME: application can pass invalid value for precision and scale ?? */
	if (cr->n.precision > 77)
		return TDS_CONVERT_FAIL;

	if (cr->n.precision == 0)
		cr->n.precision = 77;	/* assume max precision */

	if (cr->n.scale > cr->n.precision)
		return TDS_CONVERT_FAIL;


	/* skip leading blanks */
	for (pstr = instr;; ++pstr) {
		if (pstr == pend)
			return TDS_CONVERT_SYNTAX;
		if (*pstr != ' ')
			break;
	}

	sign = 0;
	if (*pstr == '-' || *pstr == '+') {	/* deal with a leading sign */
		if (*pstr == '-')
			sign = 1;
		pstr++;
	}
	cr->n.array[0] = sign;

	/* 
	 * skip leading zeroes 
	 * Not skipping them cause numbers like "000000000000" to 
	 * appear like overflow
	 */
	for (; pstr != pend && *pstr == '0'; ++pstr)
		digit_found = 1;

	/* 
	 * Having disposed of any sign and leading blanks, 
	 * vet the digit string, counting places before and after 
	 * the decimal point.  Dispense with trailing blanks, if any.  
	 */

	ptr = mynumber;
	for (i = 0; i < 8; ++i)
		*ptr++ = '0';

	places = 0;
	old_digits_left = 0;
	digits_left = cr->n.precision - cr->n.scale;

	for (; pstr != pend; ++pstr) {			/* deal with the rest */
		if (*pstr >= '0' && *pstr <= '9') {	/* it's a number */
			/* copy digit to destination */
			if (--digits_left >= 0)
				*ptr++ = *pstr;
			digit_found = 1;
		} else if (*pstr == '.') {			/* found a decimal point */
			if (places)				/* found a decimal point previously: return error */
				return TDS_CONVERT_SYNTAX;
			old_digits_left = digits_left;
			digits_left = cr->n.scale;
			places = 1;
		} else if (*pstr == ' ') {
			for (; pstr != pend && *pstr == ' '; ++pstr) ; /* skip contiguous blanks */
			if (pstr == pend)
				break;				/* success: found only trailing blanks */
			return TDS_CONVERT_SYNTAX;		/* bzzt: found something after the blank(s) */
		} else {         				/* first invalid character */
			return TDS_CONVERT_SYNTAX;
		}
	}
	/* no digits? no number! */
	if (!digit_found)
		return TDS_CONVERT_SYNTAX;

	if (!places) {
		old_digits_left = digits_left;
		digits_left = cr->n.scale;
	}

	/* too many digits, error */
	if (old_digits_left < 0)
		return TDS_CONVERT_OVERFLOW;

	/* fill up decimal digits */
	while (--digits_left >= 0)
		*ptr++ = '0';

	/*
	 * Packaged number explanation: 
	 * We package 8 decimal digits in one number.  
	 * Because 10^8 = 5^8 * 2^8 = 5^8 * 256, dividing 10^8 by 256 leaves no remainder.
	 * We can thus split it into bytes in an optimized way.
	 */

	/* transform to packaged one */
	j = -1;
	ptr -= 8;
	do {
		TDS_UINT n = *ptr++;

		for (i = 1; i < 8; ++i)
			n = n * 10 + *ptr++;
		/* fix packet number and store */
		packed_num[++j] = n - ((TDS_UINT) '0' * 11111111lu);
		ptr -= 16;
	} while (ptr > mynumber);

	memset(cr->n.array + 1, 0, sizeof(cr->n.array) - 1);
	bytes = tds_numeric_bytes_per_prec[cr->n.precision];
	while (j > 0 && !packed_num[j])
		--j;

	for (;;) {
		char is_zero = 1;
		TDS_UINT carry = 0;
		i = j;
		if (!packed_num[j])
			--j;
		do {
			TDS_UINT tmp = packed_num[i];
			if (tmp)
				is_zero = 0;

			/* divide for 256 for find another byte */
			/*
			 * carry * (25u*25u*25u*25u) = carry * 10^8 / 256u
			 * using unsigned number is just an optimization
			 * compiler can translate division to a shift and remainder 
			 * to a binary AND
			 */
			packed_num[i] = carry * (25u * 25u * 25u * 25u) + tmp / 256u;
			carry = tmp % 256u;
		} while(--i >= 0);
		if (is_zero)
			break;
		/*
		 * source number is limited to 38 decimal digit
		 * 10^39-1 < 2^128 (16 byte) so this cannot make an overflow
		 */
		cr->n.array[--bytes] = carry;
	}
	return sizeof(TDS_NUMERIC);
}

static int
is_numeric_dateformat(const char *t)
{
	const char *instr;
	int ret = 1;
	int slashes = 0;
	int hyphens = 0;
	int periods = 0;
	int digits = 0;

	for (instr = t; *instr; instr++) {
		if (!isdigit((unsigned char) *instr) && *instr != '/' && *instr != '-' && *instr != '.') {
			ret = 0;
			break;
		}
		if (*instr == '/')
			slashes++;
		else if (*instr == '-')
			hyphens++;
		else if (*instr == '.')
			periods++;
		else
			digits++;

	}
	if (hyphens + slashes + periods != 2)
		ret = 0;
	if (hyphens == 1 || slashes == 1 || periods == 1)
		ret = 0;

	if (digits < 4 || digits > 8)
		ret = 0;

	return (ret);

}

/* This function will check if an alphanumeric string */
/* holds a date in any of the following formats :     */
/*    DD-MON-YYYY */
/*    DD-MON-YY   */
/*    DDMONYY     */
/*    DDMONYYYY   */
static int
is_dd_mon_yyyy(char *t)
{
	char *instr;
	char month[4];

	instr = t;

	if (!isdigit((unsigned char) *instr))
		return (0);

	instr++;

	if (!isdigit((unsigned char) *instr))
		return (0);

	instr++;

	if (*instr == '-') {
		instr++;

		strncpy(month, instr, 3);
		month[3] = '\0';

		if (!is_monthname(month))
			return (0);

		instr += 3;

		if (*instr != '-')
			return (0);

		instr++;
		if (!isdigit((unsigned char) *instr))
			return (0);

		instr++;
		if (!isdigit((unsigned char) *instr))
			return (0);

		instr++;

		if (*instr) {
			if (!isdigit((unsigned char) *instr))
				return (0);

			instr++;
			if (!isdigit((unsigned char) *instr))
				return (0);
		}

	} else {

		strncpy(month, instr, 3);
		month[3] = '\0';

		if (!is_monthname(month))
			return (0);

		instr += 3;

		if (!isdigit((unsigned char) *instr))
			return (0);

		instr++;
		if (!isdigit((unsigned char) *instr))
			return (0);

		instr++;
		if (*instr) {
			if (!isdigit((unsigned char) *instr))
				return (0);

			instr++;
			if (!isdigit((unsigned char) *instr))
				return (0);
		}
	}

	return (1);

}

static int
is_ampm(const char *datestr)
{
	int ret = 0;

	if (strcasecmp(datestr, "am") == 0)
		ret = 1;
	else if (strcasecmp(datestr, "pm") == 0)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int
is_alphabetic(const char *datestr)
{
	const char *s;
	int ret = 1;

	for (s = datestr; *s; s++) {
		if (!isalpha((unsigned char) *s))
			ret = 0;
	}
	return (ret);
}

static int
is_numeric(const char *datestr)
{
	const char *s;
	int ret = 1;

	for (s = datestr; *s; s++) {
		if (!isdigit((unsigned char) *s))
			ret = 0;
	}
	return (ret);
}

static int
is_timeformat(const char *datestr)
{
	const char *s;
	int ret = 1;

	for (s = datestr; *s; s++) {
		if (!isdigit((unsigned char) *s) && *s != ':' && *s != '.')
			break;
	}
	if (*s) {
		if (strcasecmp(s, "am") != 0 && strcasecmp(s, "pm") != 0)
			ret = 0;
	}


	return (ret);
}

static int
store_year(int year, struct tds_time *t)
{

	if (year < 0)
		return 0;

	if (year < 100) {
		if (year > 49)
			t->tm_year = year;
		else
			t->tm_year = 100 + year;
		return (1);
	}

	if (year < 1753)
		return (0);

	if (year <= 9999) {
		t->tm_year = year - 1900;
		return (1);
	}

	return (0);

}

static int
store_mday(const char *datestr, struct tds_time *t)
{
	int mday = atoi(datestr);

	if (mday > 0 && mday < 32) {
		t->tm_mday = mday;
		return 1;
	}
	return 0;
}

static int
store_numeric_date(const char *datestr, struct tds_time *t)
{
	int TDS_MONTH = 0;
	int TDS_DAY = 0;
	int TDS_YEAR = 0;

	int state;
	char last_char = 0;
	const char *s;
	int month = 0, year = 0, mday = 0;

	/* Its YYYY-MM-DD format */

	if (strlen(datestr) == 10 && *(datestr + 4) == '-' && *(datestr + 7) == '-') {

		TDS_YEAR = 0;
		TDS_MONTH = 1;
		TDS_DAY = 2;
		state = TDS_YEAR;

	}
	/* else we assume MDY */
	else {
		TDS_MONTH = 0;
		TDS_DAY = 1;
		TDS_YEAR = 2;
		state = TDS_MONTH;
	}
	for (s = datestr; *s; s++) {
		if (!isdigit((unsigned char) *s) && isdigit((unsigned char) last_char)) {
			state++;
		} else {
			if (state == TDS_MONTH)
				month = (month * 10) + (*s - '0');
			if (state == TDS_DAY)
				mday = (mday * 10) + (*s - '0');
			if (state == TDS_YEAR)
				year = (year * 10) + (*s - '0');
		}
		last_char = *s;
	}

	if (month > 0 && month < 13)
		t->tm_mon = month - 1;
	else
		return 0;
	if (mday > 0 && mday < 32)
		t->tm_mday = mday;
	else
		return 0;

	return store_year(year, t);

}

static int
store_dd_mon_yyy_date(char *datestr, struct tds_time *t)
{

	char dd[3];
	int mday;
	char mon[4];
	char yyyy[5];
	int year;

	tdsdump_log(TDS_DBG_INFO1, "store_dd_mon_yyy_date: %s\n", datestr);
	strncpy(dd, datestr, 2);
	dd[2] = '\0';
	mday = atoi(dd);

	if (mday > 0 && mday < 32)
		t->tm_mday = mday;
	else
		return 0;

	if (datestr[2] == '-') {
		strncpy(mon, &datestr[3], 3);
		mon[3] = '\0';

		if (store_monthname(mon, t) < 0) {
			tdsdump_log(TDS_DBG_INFO1, "store_dd_mon_yyy_date: store_monthname failed\n");
			return 0;
		}

		strcpy(yyyy, &datestr[7]);
		year = atoi(yyyy);
		tdsdump_log(TDS_DBG_INFO1, "store_dd_mon_yyy_date: year %d\n", year);

		return store_year(year, t);
	} else {
		strncpy(mon, &datestr[2], 3);
		mon[3] = '\0';

		if (store_monthname(mon, t) < 0) {
			tdsdump_log(TDS_DBG_INFO1, "store_dd_mon_yyy_date: store_monthname failed\n");
			return 0;
		}

		strcpy(yyyy, &datestr[5]);
		year = atoi(yyyy);
		tdsdump_log(TDS_DBG_INFO1, "store_dd_mon_yyy_date: year %d\n", year);

		return store_year(year, t);
	}

}

/**
 * Test if a string is a month name and store correct month number
 * @return month number (0-11) or -1 if not match
 * @param datestr string to check
 * @param t       where to store month (if NULL no store is done)
 */
static int
store_monthname(const char *datestr, struct tds_time *t)
{
	int ret;

	tdsdump_log(TDS_DBG_INFO1, "store_monthname: %ld %s\n", (long) strlen(datestr), datestr);
	if (strlen(datestr) == 3) {
		if (strcasecmp(datestr, "jan") == 0)
			ret = 0;
		else if (strcasecmp(datestr, "feb") == 0)
			ret = 1;
		else if (strcasecmp(datestr, "mar") == 0)
			ret = 2;
		else if (strcasecmp(datestr, "apr") == 0)
			ret = 3;
		else if (strcasecmp(datestr, "may") == 0)
			ret = 4;
		else if (strcasecmp(datestr, "jun") == 0)
			ret = 5;
		else if (strcasecmp(datestr, "jul") == 0)
			ret = 6;
		else if (strcasecmp(datestr, "aug") == 0)
			ret = 7;
		else if (strcasecmp(datestr, "sep") == 0)
			ret = 8;
		else if (strcasecmp(datestr, "oct") == 0)
			ret = 9;
		else if (strcasecmp(datestr, "nov") == 0)
			ret = 10;
		else if (strcasecmp(datestr, "dec") == 0)
			ret = 11;
		else
			return -1;
	} else {
		if (strcasecmp(datestr, "january") == 0)
			ret = 0;
		else if (strcasecmp(datestr, "february") == 0)
			ret = 1;
		else if (strcasecmp(datestr, "march") == 0)
			ret = 2;
		else if (strcasecmp(datestr, "april") == 0)
			ret = 3;
		else if (strcasecmp(datestr, "june") == 0)
			ret = 5;
		else if (strcasecmp(datestr, "july") == 0)
			ret = 6;
		else if (strcasecmp(datestr, "august") == 0)
			ret = 7;
		else if (strcasecmp(datestr, "september") == 0)
			ret = 8;
		else if (strcasecmp(datestr, "october") == 0)
			ret = 9;
		else if (strcasecmp(datestr, "november") == 0)
			ret = 10;
		else if (strcasecmp(datestr, "december") == 0)
			ret = 11;
		else
			return -1;
	}
	if (t)
		t->tm_mon = ret;
	return ret;
}

static int
store_yymmdd_date(const char *datestr, struct tds_time *t)
{
	int month = 0, year = 0, mday = 0;

	int wholedate;

	wholedate = atoi(datestr);

	year = wholedate / 10000;
	month = (wholedate - (year * 10000)) / 100;
	mday = (wholedate - (year * 10000) - (month * 100));

	if (month > 0 && month < 13)
		t->tm_mon = month - 1;
	else
		return 0;
	if (mday > 0 && mday < 32)
		t->tm_mday = mday;
	else
		return 0;

	return (store_year(year, t));

}

static int
store_time(const char *datestr, struct tds_time *t)
{
	enum
	{ TDS_HOURS,
		TDS_MINUTES,
		TDS_SECONDS,
		TDS_FRACTIONS
	};

	int state = TDS_HOURS;
	char last_sep = '\0';
	const char *s;
	int hours = 0, minutes = 0, seconds = 0, nanosecs = 0;
	int ret = 1;
	unsigned ns_div = 1;

	for (s = datestr; *s && strchr("apmAPM", (int) *s) == NULL; s++) {
		if (*s == ':' || *s == '.') {
			last_sep = *s;
			state++;
		} else {
			if (*s < '0' || *s > '9')
				ret = 0;
			switch (state) {
			case TDS_HOURS:
				hours = (hours * 10) + (*s - '0');
				break;
			case TDS_MINUTES:
				minutes = (minutes * 10) + (*s - '0');
				break;
			case TDS_SECONDS:
				seconds = (seconds * 10) + (*s - '0');
				break;
			case TDS_FRACTIONS:
				if (ns_div < 1000000000u) {
					nanosecs = (nanosecs * 10) + (*s - '0');
					ns_div *= 10;
				}
				break;
			}
		}
	}
	if (*s) {
		if (strcasecmp(s, "am") == 0) {
			if (hours == 12)
				hours = 0;

			t->tm_hour = hours;
		}
		if (strcasecmp(s, "pm") == 0) {
			if (hours == 0)
				ret = 0;
			if (hours > 0 && hours < 12)
				t->tm_hour = hours + 12;
			else
				t->tm_hour = hours;
		}
	} else {
		if (hours >= 0 && hours < 24)
			t->tm_hour = hours;
		else
			ret = 0;
	}
	if (minutes >= 0 && minutes < 60)
		t->tm_min = minutes;
	else
		ret = 0;
	if (seconds >= 0 && seconds < 60)
		t->tm_sec = seconds;
	else
		ret = 0;
	tdsdump_log(TDS_DBG_FUNC, "store_time() nanosecs = %d\n", nanosecs);
	if (nanosecs) {
		if (nanosecs >= 0 && nanosecs < ns_div && last_sep == '.')
			t->tm_ns = nanosecs * (1000000000u / ns_div);
		else if (nanosecs >= 0 && nanosecs < 1000u)
			t->tm_ns = nanosecs * 1000000u;
		else
			ret = 0;
	}

	return (ret);
}

static int
store_hour(const char *hour, const char *ampm, struct tds_time *t)
{
	int ret = 1;
	int hours;

	hours = atoi(hour);

	if (hours >= 0 && hours < 24) {
		if (strcasecmp(ampm, "am") == 0) {
			if (hours == 12)
				hours = 0;

			t->tm_hour = hours;
		}
		if (strcasecmp(ampm, "pm") == 0) {
			if (hours == 0)
				ret = 0;
			if (hours > 0 && hours < 12)
				t->tm_hour = hours + 12;
			else
				t->tm_hour = hours;
		}
	}
	return ret;
}

/**
 * Get same type but nullable
 * @param srctype type requires
 * @return nullable type
 */
TDS_INT
tds_get_null_type(int srctype)
{

	switch (srctype) {
	case SYBCHAR:
		return SYBVARCHAR;
		break;
	case SYBINT1:
	case SYBUINT1:
	case SYBINT2:
	case SYBINT4:
	case SYBINT8:
	/* TODO sure ?? */
	case SYBUINT2:
	case SYBUINT4:
	case SYBUINT8:
		return SYBINTN;
		break;
	case SYBREAL:
	case SYBFLT8:
		return SYBFLTN;
		break;
	case SYBDATETIME:
	case SYBDATETIME4:
		return SYBDATETIMN;
		break;
	case SYBBIT:
		return SYBBITN;
		break;
	case SYBMONEY:
	case SYBMONEY4:
		return SYBMONEYN;
		break;
	default:
		break;
	}
	return srctype;
}

/**
 * format a date string according to an "extended" strftime(3) formatting definition.
 * @param buf     output buffer
 * @param maxsize size of buffer in bytes (space include terminator)
 * @param format  format string passed to strftime(3), except that %z represents milliseconds
 * @param dr      date to convert
 * @return length of string returned, 0 for error
 */
size_t
tds_strftime(char *buf, size_t maxsize, const char *format, const TDSDATEREC * dr, int prec)
{
	struct tm tm;

	size_t length;
	char *our_format;
	char *pz = NULL;
	
	assert(buf);
	assert(format);
	assert(dr);
	assert(0 <= dr->decimicrosecond && dr->decimicrosecond < 10000000);
	if (prec < 0 || prec > 7)
		prec = 3;

	tm.tm_sec = dr->second;
	tm.tm_min = dr->minute;
	tm.tm_hour = dr->hour;
	tm.tm_mday = dr->day;
	tm.tm_mon = dr->month;
	tm.tm_year = dr->year - 1900;
	tm.tm_wday = dr->weekday;
	tm.tm_yday = dr->dayofyear;
	tm.tm_isdst = 0;
#ifdef HAVE_STRUCT_TM_TM_ZONE
	tm.tm_zone = NULL;
#elif defined(HAVE_STRUCT_TM___TM_ZONE)
	tm.__tm_zone = NULL;
#endif

	/* more characters are required because we replace %z with up to 7 digits */
	our_format = (char*) malloc(strlen(format) + 1 + 5);
	if (!our_format)
		return 0;

	strcpy(our_format, format);

	/*
	 * Look for "%z" in the format string.  If found, replace it with dr->milliseconds.
	 * For example, if milliseconds is 124, the format string
	 * "%b %d %Y %H:%M:%S.%z" would become
	 * "%b %d %Y %H:%M:%S.124".
	 */
	for (pz = our_format; (pz = strstr(pz, "%z")) != NULL; pz++) {
		/* Skip any escaped cases (%%z) */
		if (pz > our_format && *(pz - 1) != '%')
			break;
	}

	if (pz) {
		char buf[12];
		sprintf(buf, "%07d", dr->decimicrosecond);
		memcpy(pz, buf, prec);
		strcpy(pz + prec, format + (pz - our_format) + 2);
	}

	length = strftime(buf, maxsize, our_format, &tm);

	free(our_format);

	return length;
}

#if 0
static TDS_UINT
utf16len(const utf16_t * s)
{
	const utf16_t *p = s;

	while (*p++);
	return p - s;
}
#endif

/**
 * Test if a conversion is possible
 * @param srctype  source type
 * @param desttype destination type
 * @return 0 if not convertible
 */
unsigned char
tds_willconvert(int srctype, int desttype)
{
	typedef struct
	{
		int srctype;
		int desttype;
		int yn;
	}
	ANSWER;
	static const ANSWER answers[] = {
#	include "tds_willconvert.h"
	};
	unsigned int i;
	const ANSWER *p = NULL;

	tdsdump_log(TDS_DBG_FUNC, "tds_willconvert(%d, %d)\n", srctype, desttype);

	for (i = 0; i < sizeof(answers) / sizeof(ANSWER); i++) {
		if (srctype == answers[i].srctype && desttype == answers[i].desttype) {
			tdsdump_log(TDS_DBG_FUNC, "tds_willconvert(%d, %d) returns %s\n", answers[i].srctype, answers[i].desttype,
				    answers[i].yn? "yes":"no");
			p =  &answers[i];
			break;
		}
	}

	if (!p)
		return 0;
		
	if (is_fixed_type(p->desttype) || !p->yn)
		return p->yn;
	/*
	 * Return the number of bytes needed to represent the srctype as a string.  
	 * This allows an application to use "tds_willconvert(SYBINT4, SYBCHAR)" to 
	 * discover he'll need an 11-byte buffer. 
	 * 
	 * Sizes exclude null terminators but allow for a '-' sign for negative numbers. 
	 * If the srctype is also variable, there is no per-type answer; it depends 
	 * on the declared length of the input.  We just return 0xFF.  
	 */
	switch (p->srctype) {
	case SYBBIT:
			return 1;
	case SYBSINT1:
	case SYBUINT1:
	case SYBINT1:
			return  3;
	case SYBINT2:
			return  6;
	case SYBUINT2:
			return  5;
	case SYBINT4:
			return 11;
	case SYBUINT4:
			return 10;
	case SYB5INT8:
	case SYBINT8:
			return 21;
	case SYBUINT8:
			return 20;
	case SYBREAL:
	case SYBFLT8:
			return 11;
	case SYBDECIMAL:
	case SYBNUMERIC:
			return 46;
	case SYBDATETIME:
	case SYBDATETIME4:
			return 26;
	case SYBMONEY:
	case SYBMONEY4:
			return 12;
	/* TODO SYBBLOB has the same value */
	case SYBUNIQUE:
			return 36;
	/* non-fixed types have variable data sizes, just return 0xff */
	case SYBCHAR:
	case SYBBINARY:
	case SYBIMAGE:
	case SYBLONGBINARY:
	case SYBLONGCHAR:
	case SYBTEXT:
	case SYBVARBINARY:
	case SYBVARCHAR:
	case SYBNTEXT:
	case SYBNVARCHAR:
			return 0xFF;
	default:
		assert(0 == p->srctype);
	}
	return 0;
#if 0
	/* Other, unmentionable types */
	case SYBBOUNDARY:
	case SYBDATE:
	case SYBINTERVAL:
	case SYBSENSITIVITY:
	case SYBTIME:
	case SYBUNITEXT:
	case SYBVARIANT:
	case SYBVOID:
	case SYBXML:
	case XSYBBINARY:
	case XSYBCHAR:
	case XSYBNCHAR:
	case XSYBNVARCHAR:
	case XSYBVARBINARY:
	case XSYBVARCHAR:
#endif
}

#if 0
select day(d) as day, datepart(dw, d) as weekday, datepart(week, d) as week, d as 'date' from #t order by d
day         weekday     week        date                                                   
----------- ----------- ----------- ----------
          1           5           1 2009-01-01 Thursday
          2           6           1 2009-01-02 Friday
          3           7           1 2009-01-03 Saturday
          4           1           2 2009-01-04 Sunday
          5           2           2 2009-01-05
          6           3           2 2009-01-06
          7           4           2 2009-01-07
          8           5           2 2009-01-08
          9           6           2 2009-01-09
         10           7           2 2009-01-10
         11           1           3 2009-01-11
         12           2           3 2009-01-12
         13           3           3 2009-01-13
         14           4           3 2009-01-14
         15           5           3 2009-01-15
         16           6           3 2009-01-16
         17           7           3 2009-01-17
         18           1           4 2009-01-18
         19           2           4 2009-01-19
         20           3           4 2009-01-20
         21           4           4 2009-01-21
         22           5           4 2009-01-22
         23           6           4 2009-01-23
         24           7           4 2009-01-24
         25           1           5 2009-01-25
         26           2           5 2009-01-26
         27           3           5 2009-01-27
         28           4           5 2009-01-28
         29           5           5 2009-01-29
         30           6           5 2009-01-30
         31           7           5 2009-01-31
#endif
/**
 * Convert from db date format to a structured date format
 * @param datetype source date type. SYBDATETIME or SYBDATETIME4
 * @param di       source date
 * @param dr       destination date
 * @return TDS_FAIL or TDS_SUCCESS
 */
TDSRET
tds_datecrack(TDS_INT datetype, const void *di, TDSDATEREC * dr)
{
	int dt_days;
	unsigned int dt_time;

	int years, months, days, ydays, wday, hours, mins, secs, dms;
	int l, n, i, j;

	memset(dr, 0, sizeof(*dr));

	if (datetype == SYBMSDATE || datetype == SYBMSTIME 
	    || datetype == SYBMSDATETIME2 || datetype == SYBMSDATETIMEOFFSET) {
		const TDS_DATETIMEALL *dta = (const TDS_DATETIMEALL *) di;
		dt_days = (datetype == SYBMSTIME) ? 0 : dta->date;
		if (datetype == SYBMSDATE) {
			dms = 0;
			secs = 0;
			dt_time = 0;
		} else {
			dms = dta->time % 10000000u;
			dt_time = dta->time / 10000000u;
			secs = dt_time % 60;
			dt_time = dt_time / 60;
		}
		if (datetype == SYBMSDATETIMEOFFSET) {
			--dt_days;
			dt_time = dt_time + 86400 + dta->offset;
			dt_days += dt_time / 86400;
			dt_time %= 86400;
		}
	} else if (datetype == SYBDATETIME) {
		const TDS_DATETIME *dt = (const TDS_DATETIME *) di;
		dt_time = dt->dttime;
		dms = ((dt_time % 300) * 1000 + 150) / 300 * 10000u;
		dt_time = dt_time / 300;
		secs = dt_time % 60;
		dt_time = dt_time / 60;
		dt_days = dt->dtdays;
	} else if (datetype == SYBDATETIME4) {
		const TDS_DATETIME4 *dt4 = (const TDS_DATETIME4 *) di;
		secs = 0;
		dms = 0;
		dt_days = dt4->days;
		dt_time = dt4->minutes;
	} else
		return TDS_FAIL;

	/*
	 * -53690 is minimun  (1753-1-1) (Gregorian calendar start in 1732) 
	 * 2958463 is maximun (9999-12-31)
	 */
	l = dt_days + (146038 + 146097*4);
	wday = (l + 4) % 7;
	n = (4 * l) / 146097;	/* n century */
	l = l - (146097 * n + 3) / 4;	/* days from xx00-02-28 (y-m-d) */
	i = (4000 * (l + 1)) / 1461001;	/* years from xx00-02-28 */
	l = l - (1461 * i) / 4;	/* year days from xx00-02-28 */
	ydays = l >= 306 ? l - 305 : l + 60;
	l += 31;
	j = (80 * l) / 2447;
	days = l - (2447 * j) / 80;
	l = j / 11;
	months = j + 1 - 12 * l;
	years = 100 * (n - 1) + i + l;
	if (l == 0 && (years & 3) == 0 && (years % 100 != 0 || years % 400 == 0))
		++ydays;

	hours = dt_time / 60;
	mins = dt_time % 60;

	dr->year = years;
	dr->month = months;
	dr->quarter = months / 3;
	dr->day = days;
	dr->dayofyear = ydays;
	dr->week = -1;
	dr->weekday = wday;
	dr->hour = hours;
	dr->minute = mins;
	dr->second = secs;
	dr->decimicrosecond = dms;
	return TDS_SUCCESS;
}

/**
 * \brief convert a number in string to TDS_INT
 *
 * \return TDS_CONVERT_* or failure code on error
 * \remarks Sybase's char->int conversion tolerates embedded blanks, 
 * such that "convert( int, ' - 13 ' )" works.  
 * If we find blanks, we copy the string to a temporary buffer, 
 * skipping the blanks.  
 * We return the results of atoi() with a clean string.  
 * 
 * n.b. it is possible to embed all sorts of non-printable characters, but we
 * only check for spaces.  at this time, no one on the project has tested anything else.  
 */
static TDS_INT
string_to_int(const char *buf, const char *pend, TDS_INT * res)
{
	enum
	{ blank = ' ' };
	const char *p;
	int sign;
	unsigned int num;	/* we use unsigned here for best overflow check */

	p = buf;

	/* ignore leading spaces */
	while (p != pend && *p == blank)
		++p;
	if (p == pend) {
		*res = 0;
		return sizeof(TDS_INT);
	}

	/* check for sign */
	sign = 0;
	switch (*p) {
	case '-':
		sign = 1;
		/* fall thru */
	case '+':
		/* skip spaces between sign and number */
		++p;
		while (p != pend && *p == blank)
			++p;
		break;
	}

	/* a digit must be present */
	if (p == pend)
		return TDS_CONVERT_SYNTAX;

	num = 0;
	for (; p != pend; ++p) {
		/* check for trailing spaces */
		if (*p == blank) {
			while (++p != pend && *p == blank);
			if (p != pend)
				return TDS_CONVERT_SYNTAX;
			break;
		}

		/* must be a digit */
		if (!isdigit((unsigned char) *p))
			return TDS_CONVERT_SYNTAX;

		/* add a digit to number and check for overflow */
		/* NOTE I didn't forget a digit, I check overflow before multiply to prevent overflow */
		if (num > 214748364u)
			return TDS_CONVERT_OVERFLOW;
		num = num * 10u + (*p - '0');
	}

	/* check for overflow and convert unsigned to signed */
	if (sign) {
		if (num > 2147483648u)
			return TDS_CONVERT_OVERFLOW;
		*res = 0 - num;
	} else {
		if (num >= 2147483648u)
			return TDS_CONVERT_OVERFLOW;
		*res = num;
	}

	return sizeof(TDS_INT);
}

/**
 * \brief convert a number in string to TDS_INT8
 *
 * \return TDS_CONVERT_* or failure code on error
 */
static TDS_INT	/* copied from string_ti_int and modified */
parse_int8(const char *buf, const char *pend, TDS_UINT8 * res, int * p_sign)
{
	enum
	{ blank = ' ' };
	const char *p;
	TDS_UINT8 num, prev;

	p = buf;

	/* ignore leading spaces */
	while (p != pend && *p == blank)
		++p;
	if (p == pend) {
		*res = 0;
		return sizeof(TDS_INT8);
	}

	/* check for sign */
	switch (*p) {
	case '-':
		*p_sign = 1;
		/* fall thru */
	case '+':
		/* skip spaces between sign and number */
		++p;
		while (p != pend && *p == blank)
			++p;
		break;
	}

	/* a digit must be present */
	if (p == pend)
		return TDS_CONVERT_SYNTAX;

	num = 0;
	for (; p != pend; ++p) {
		/* check for trailing spaces */
		if (*p == blank) {
			while (p != pend && *++p == blank);
			if (p != pend)
				return TDS_CONVERT_SYNTAX;
			break;
		}

		/* must be a digit */
		if (!isdigit((unsigned char) *p))
			return TDS_CONVERT_SYNTAX;

		/* add a digit to number and check for overflow */
		if (num > ((((TDS_UINT8) 1) << 63) / ((TDS_UINT8) 5)))
			return TDS_CONVERT_OVERFLOW;
		prev = num;
		num = num * 10u + (*p - '0');
		if (num < prev)
			return TDS_CONVERT_OVERFLOW;
	}

	*res = num;
	return sizeof(TDS_INT8);
}

/**
 * \brief convert a number in string to TDS_INT8
 *
 * \return TDS_CONVERT_* or failure code on error
 */
static TDS_INT	/* copied from string_ti_int and modified */
string_to_int8(const char *buf, const char *pend, TDS_INT8 * res)
{
	TDS_UINT8 num;
	TDS_INT parse_res;
	int sign = 0;

	parse_res = parse_int8(buf, pend, &num, &sign);
	if (parse_res < 0)
		return parse_res;

	/* check for overflow and convert unsigned to signed */
	if (sign) {
		if (num > (((TDS_UINT8) 1) << 63))
			return TDS_CONVERT_OVERFLOW;
		*res = 0 - num;
	} else {
		if (num >= (((TDS_UINT8) 1) << 63))
			return TDS_CONVERT_OVERFLOW;
		*res = num;
	}
	return sizeof(TDS_INT8);
}

/**
 * \brief convert a number in string to TDS_UINT8
 *
 * \return TDS_CONVERT_* or failure code on error
 */
static TDS_INT	/* copied from string_to_int8 and modified */
string_to_uint8(const char *buf, const char *pend, TDS_UINT8 * res)
{
	TDS_UINT8 num;
	TDS_INT parse_res;
	int sign = 0;

	parse_res = parse_int8(buf, pend, &num, &sign);
	if (parse_res < 0)
		return parse_res;

	/* check for overflow */
	if (sign && num)
		return TDS_CONVERT_OVERFLOW;

	*res = num;
	return sizeof(TDS_UINT8);
}

/** @} */
