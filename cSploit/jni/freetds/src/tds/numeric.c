/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 * Copyright (C) 2005-2010  Frediano Ziglio
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

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include <freetds/convert.h>
#include <freetds/bytes.h>
#include <stdlib.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: numeric.c,v 1.50 2011-06-11 07:42:26 freddy77 Exp $");

/**
 * tds_numeric_bytes_per_prec is indexed by precision and will
 * tell us the number of bytes required to store the specified
 * precision (with the sign).
 * Support precision up to 77 digits
 */
const int tds_numeric_bytes_per_prec[] = {
	/*
	 * precision can't be 0 but using a value > 0 assure no
	 * core if for some bug it's 0...
	 */
	1, 
	2,  2,  3,  3,  4,  4,  4,  5,  5,
	6,  6,  6,  7,  7,  8,  8,  9,  9,  9,
	10, 10, 11, 11, 11, 12, 12, 13, 13, 14,
	14, 14, 15, 15, 16, 16, 16, 17, 17, 18,
	18, 19, 19, 19, 20, 20, 21, 21, 21, 22,
	22, 23, 23, 24, 24, 24, 25, 25, 26, 26,
	26, 27, 27, 28, 28, 28, 29, 29, 30, 30,
	31, 31, 31, 32, 32, 33, 33, 33
};

TDS_COMPILE_CHECK(maxprecision,
	MAXPRECISION < TDS_VECTOR_SIZE(tds_numeric_bytes_per_prec) );

/*
 * money is a special case of numeric really...that why its here
 */
char *
tds_money_to_string(const TDS_MONEY * money, char *s)
{
	int frac;
	TDS_INT8 mymoney;
	TDS_UINT8 n;
	char *p;

	/* sometimes money it's only 4-byte aligned so always compute 64-bit */
	/* FIXME align money/double/bigint in row to 64-bit */
	mymoney = (((TDS_INT8)(((TDS_INT*)money)[0])) << 32) | ((TDS_UINT*) money)[1];

	p = s;
	if (mymoney < 0) {
		*p++ = '-';
		/* we use unsigned cause this cause arithmetic problem for -2^63*/
		n = -mymoney;
	} else {
		n = mymoney;
	}
	n = (n + 50) / 100;
	frac = (int) (n % 100);
	n /= 100;
	/* if machine is 64 bit you do not need to split n */
	sprintf(p, "%" PRId64 ".%02d", n, frac);
	return s;
}

/**
 * @return <0 if error
 */
TDS_INT
tds_numeric_to_string(const TDS_NUMERIC * numeric, char *s)
{
	const unsigned char *number;

	unsigned int packet[sizeof(numeric->array) / 2];
	unsigned int *pnum, *packet_start;
	unsigned int *const packet_end = packet + TDS_VECTOR_SIZE(packet);

	unsigned int packet10k[(MAXPRECISION + 3) / 4];
	unsigned int *p;

	int num_bytes;
	unsigned int remainder, n, i, m;

	/* a bit of debug */
#if ENABLE_EXTRA_CHECKS
	memset(packet, 0x55, sizeof(packet));
	memset(packet10k, 0x55, sizeof(packet10k));
#endif

	if (numeric->precision < 1 || numeric->precision > MAXPRECISION || numeric->scale > numeric->precision)
		return TDS_CONVERT_FAIL;

	/* set sign */
	if (numeric->array[0] == 1)
		*s++ = '-';

	/* put number in a 16bit array */
	number = numeric->array;
	num_bytes = tds_numeric_bytes_per_prec[numeric->precision];

	n = num_bytes - 1;
	pnum = packet_end;
	for (; n > 1; n -= 2)
		*--pnum = TDS_GET_UA2BE(&number[n - 1]);
	if (n == 1)
		*--pnum = number[n];
	while (!*pnum) {
		++pnum;
		if (pnum == packet_end) {
			*s++ = '0';
			if (numeric->scale) {
				*s++ = '.';
				i = numeric->scale;
				do {
					*s++ = '0';
				} while (--i);
			}
			*s = 0;
			return 1;
		}
	}
	packet_start = pnum;

	/* transform 2^16 base number in 10^4 base number */
	for (p = packet10k + TDS_VECTOR_SIZE(packet10k); packet_start != packet_end;) {
		pnum = packet_start;
		n = *pnum;
		remainder = n % 10000u;
		if (!(*pnum++ = (n / 10000u)))
			packet_start = pnum;
		for (; pnum != packet_end; ++pnum) {
			n = remainder * (256u * 256u) + *pnum;
			remainder = n % 10000u;
			*pnum = n / 10000u;
		}
		*--p = remainder;
	}

	/* transform to 10 base number and output */
	i = 4 * (unsigned int)((packet10k + TDS_VECTOR_SIZE(packet10k)) - p);	/* current digit */
	/* skip leading zeroes */
	n = 1000;
	remainder = *p;
	while (remainder < n)
		n /= 10, --i;
	if (i <= numeric->scale) {
		*s++ = '0';
		*s++ = '.';
		m = i;
		while (m < numeric->scale)
			*s++ = '0', ++m;
	}
	for (;;) {
		*s++ = (remainder / n) + '0';
		--i;
		remainder %= n;
		n /= 10;
		if (!n) {
			n = 1000;
			if (++p == packet10k + TDS_VECTOR_SIZE(packet10k))
				break;
			remainder = *p;
		}
		if (i == numeric->scale)
			*s++ = '.';
	}
	*s = 0;

	return 1;
}

#define TDS_WORD  TDS_UINT
#define TDS_DWORD TDS_UINT8
#define TDS_WORD_DDIGIT 9

/* include to check limits */

#include "num_limits.h"

static int
tds_packet_check_overflow(TDS_WORD *packet, unsigned int packet_len, unsigned int prec)
{
	unsigned int i, len, stop;
	const TDS_WORD *limit = &limits[limit_indexes[prec] + LIMIT_INDEXES_ADJUST * prec];
	len = limit_indexes[prec+1] - limit_indexes[prec] + LIMIT_INDEXES_ADJUST;
	stop = prec / (sizeof(TDS_WORD) * 8);
	/*
	 * Now a number is
	 * ... P[3] P[2] P[1] P[0]
	 * while upper limit + 1 is
 	 * zeroes limit[0 .. len-1] 0[0 .. stop-1]
	 * we must assure that number < upper limit + 1
	 */
	if (packet_len >= len + stop) {
		/* higher packets must be zero */
		for (i = packet_len; --i >= len + stop; )
			if (packet[i] > 0)
				return TDS_CONVERT_OVERFLOW;
		/* test limit */
		for (;; --i, ++limit) {
			if (i <= stop) {
				/* last must be >= not > */
				if (packet[i] >= *limit)
					return TDS_CONVERT_OVERFLOW;
				break;
			}
			if (packet[i] > *limit)
				return TDS_CONVERT_OVERFLOW;
			if (packet[i] < *limit)
				break;
		}
	}
	return 0;
}

TDS_INT
tds_numeric_change_prec_scale(TDS_NUMERIC * numeric, unsigned char new_prec, unsigned char new_scale)
{
	static const TDS_WORD factors[] = {
		1, 10, 100, 1000, 10000,
		100000, 1000000, 10000000, 100000000, 1000000000
	};

	TDS_WORD packet[(sizeof(numeric->array) - 1) / sizeof(TDS_WORD)];

	unsigned int i, packet_len;
	int scale_diff, bytes;

	if (numeric->precision < 1 || numeric->precision > 77 || numeric->scale > numeric->precision)
		return TDS_CONVERT_FAIL;

	if (new_prec < 1 || new_prec > 77 || new_scale > new_prec)
		return TDS_CONVERT_FAIL;

	scale_diff = new_scale - numeric->scale;
	if (scale_diff == 0 && new_prec >= numeric->precision) {
		i = tds_numeric_bytes_per_prec[new_prec] - tds_numeric_bytes_per_prec[numeric->precision];
		if (i > 0) {
			memmove(numeric->array + 1 + i, numeric->array + 1, sizeof(numeric->array) - 1 - i);
			memset(numeric->array + 1, 0, i);
		}
		numeric->precision = new_prec;
		return sizeof(TDS_NUMERIC);
	}

	/* package number */
	bytes = tds_numeric_bytes_per_prec[numeric->precision] - 1;
	i = 0;
	do {
		/*
		 * note that if bytes are smaller we have a small buffer
		 * overflow in numeric->array however is not a problem
		 * cause overflow occurs in numeric and number is fixed below
		 */
		packet[i] = TDS_GET_UA4BE(&numeric->array[bytes-3]);
		++i;
	} while ( (bytes -= sizeof(TDS_WORD)) > 0);
	/* fix last packet */
	if (bytes < 0)
		packet[i-1] &= 0xffffffffu >> (8 * -bytes);
	while (i > 1 && packet[i-1] == 0)
		--i;
	packet_len = i;

	if (scale_diff >= 0) {
		/* check overflow before multiply */
		if (tds_packet_check_overflow(packet, packet_len, new_prec - scale_diff))
			return TDS_CONVERT_OVERFLOW;

		if (scale_diff == 0) {
			i = tds_numeric_bytes_per_prec[numeric->precision] - tds_numeric_bytes_per_prec[new_prec];
			if (i > 0)
				memmove(numeric->array + 1, numeric->array + 1 + i, sizeof(numeric->array) - 1 - i);
			numeric->precision = new_prec;
			return sizeof(TDS_NUMERIC);
		}

		/* multiply */
		do {
			/* multiply by at maximun TDS_WORD_DDIGIT */
			unsigned int n = scale_diff > TDS_WORD_DDIGIT ? TDS_WORD_DDIGIT : scale_diff;
			TDS_WORD factor = factors[n];
			TDS_WORD carry = 0;
			scale_diff -= n; 
			for (i = 0; i < packet_len; ++i) {
				TDS_DWORD n = packet[i] * ((TDS_DWORD) factor) + carry;
				packet[i] = (TDS_WORD) n;
				carry = n >> (8 * sizeof(TDS_WORD));
			}
			/* here we can expand number safely cause we know that it can't overflow */
			if (carry)
				packet[packet_len++] = carry;
		} while (scale_diff > 0);
	} else {
		/* check overflow */
		if (new_prec - scale_diff < numeric->precision)
			if (tds_packet_check_overflow(packet, packet_len, new_prec - scale_diff))
				return TDS_CONVERT_OVERFLOW;

		/* divide */
		scale_diff = -scale_diff;
		do {
			unsigned int n = scale_diff > TDS_WORD_DDIGIT ? TDS_WORD_DDIGIT : scale_diff;
			TDS_WORD factor = factors[n];
			TDS_WORD borrow = 0;
			scale_diff -= n;
			for (i = packet_len; i > 0; ) {
#if defined(__GNUC__) && __GNUC__ >= 3 && defined(__i386__)
				--i;
				__asm__ __volatile__ ("divl %4": "=a"(packet[i]), "=d"(borrow): "0"(packet[i]), "1"(borrow), "r"(factor));
#elif defined(__WATCOMC__) && defined(DOS32X)
				TDS_WORD Int64div32(TDS_WORD* low,TDS_WORD high,TDS_WORD factor);
				#pragma aux Int64div32 = "mov eax, dword ptr[esi]" \
					"div ecx" \
					"mov dword ptr[esi], eax" \
					parm [ESI] [EDX] [ECX] value [EDX] modify [EAX EDX];
				borrow = Int64div32(&packet[i], borrow, factor);
#else
				TDS_DWORD n = (((TDS_DWORD) borrow) << (8 * sizeof(TDS_WORD))) + packet[--i];
				packet[i] = n / factor;
				borrow = n % factor;
#endif
			}
		} while (scale_diff > 0);
	}

	/* back to our format */
	numeric->precision = new_prec;
	numeric->scale = new_scale;
	bytes = tds_numeric_bytes_per_prec[numeric->precision] - 1;
	for (i = bytes / sizeof(TDS_WORD); i >= packet_len; --i)
		packet[i] = 0;
	for (i = 0; bytes >= sizeof(TDS_WORD); bytes -= sizeof(TDS_WORD), ++i) {
		TDS_PUT_UA4BE(&numeric->array[bytes-3], packet[i]);
	}

	if (bytes) {
		TDS_WORD remainder = packet[i];
		do {
			numeric->array[bytes] = (TDS_UCHAR) remainder;
			remainder >>= 8;
		} while (--bytes);
	}

	return sizeof(TDS_NUMERIC);
}

