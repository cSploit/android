/*
 *    Copyright (C) 2001 Nikos Mavroyanopoulos
 *
 *    This library is free software; you can redistribute it and/or modify it 
 *    under the terms of the GNU Library General Public License as published 
 *    by the Free Software Foundation; either version 2 of the License, or 
 *    (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this library; if not, write to the
 *    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *    Boston, MA 02111-1307, USA.
 */


/* 
 * The algorithm is due to Ron Rivest.  This code is based on code
 * written by Colin Plumb in 1993.
 */


/*
 * This code implements the MD4 message-digest algorithm.
 */

/*
 * File from mhash library (mhash.sourceforge.net)
 */
#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include "md4.h"

TDS_RCSID(var, "$Id: md4.c,v 1.9 2011-05-16 08:51:40 freddy77 Exp $");

#undef word32
#define word32 TDS_UINT

#ifndef WORDS_BIGENDIAN
#define byteReverse(buf, len)	/* Nothing */
#else
static void byteReverse(unsigned char *buf, unsigned longs);


/*
 * Note: this code is harmless on little-endian machines.
 */
static void
byteReverse(unsigned char *buf, unsigned longs)
{
	word32 t;

	do {
		t = (word32) ((unsigned) buf[3] << 8 | buf[2]) << 16 | ((unsigned) buf[1] << 8 | buf[0]);
		*(word32 *) buf = t;
		buf += 4;
	} while (--longs);
}
#endif

#define rotl32(x,n)   (((x) << ((word32)(n))) | ((x) >> (32 - (word32)(n))))

/*
 * Start MD4 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void
MD4Init(struct MD4Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
MD4Update(struct MD4Context *ctx, unsigned char const *buf, size_t len)
{
	register word32 t;

	/* Update bitcount */

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((word32) len << 3)) < t)
		ctx->bits[1]++;	/* Carry from low to high */
	ctx->bits[1] += (TDS_UINT)len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* Handle any leading odd-sized chunks */

	if (t) {
		unsigned char *p = (unsigned char *) ctx->in + t;

		t = 64 - t;
		if (len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		byteReverse(ctx->in, 16);
		MD4Transform(ctx->buf, (word32 *) ctx->in);
		buf += t;
		len -= t;
	}
	/* Process data in 64-byte chunks */

	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteReverse(ctx->in, 16);
		MD4Transform(ctx->buf, (word32 *) ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */

	memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void
MD4Final(struct MD4Context *ctx, unsigned char *digest)
{
	unsigned int count;
	unsigned char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/* Set the first char of padding to 0x80.  This is safe since there is
	 * always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset(p, 0, count);
		byteReverse(ctx->in, 16);
		MD4Transform(ctx->buf, (word32 *) ctx->in);

		/* Now fill the next block with 56 bytes */
		memset(ctx->in, 0, 56);
	} else {
		/* Pad block to 56 bytes */
		memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* Append length in bits and transform */
	((word32 *) ctx->in)[14] = ctx->bits[0];
	((word32 *) ctx->in)[15] = ctx->bits[1];

	MD4Transform(ctx->buf, (word32 *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);

	if (digest != NULL)
		memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(*ctx));	/* In case it's sensitive */
}

/* The three core functions */

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

#define FF(a, b, c, d, x, s) { \
    (a) += F ((b), (c), (d)) + (x); \
    (a) = rotl32 ((a), (s)); \
  }
#define GG(a, b, c, d, x, s) { \
    (a) += G ((b), (c), (d)) + (x) + (word32)0x5a827999; \
    (a) = rotl32 ((a), (s)); \
  }
#define HH(a, b, c, d, x, s) { \
    (a) += H ((b), (c), (d)) + (x) + (word32)0x6ed9eba1; \
    (a) = rotl32 ((a), (s)); \
  }


/*
 * The core of the MD4 algorithm
 */
void
MD4Transform(word32 buf[4], word32 const in[16])
{
	register word32 a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	FF(a, b, c, d, in[0], 3);	/* 1 */
	FF(d, a, b, c, in[1], 7);	/* 2 */
	FF(c, d, a, b, in[2], 11);	/* 3 */
	FF(b, c, d, a, in[3], 19);	/* 4 */
	FF(a, b, c, d, in[4], 3);	/* 5 */
	FF(d, a, b, c, in[5], 7);	/* 6 */
	FF(c, d, a, b, in[6], 11);	/* 7 */
	FF(b, c, d, a, in[7], 19);	/* 8 */
	FF(a, b, c, d, in[8], 3);	/* 9 */
	FF(d, a, b, c, in[9], 7);	/* 10 */
	FF(c, d, a, b, in[10], 11);	/* 11 */
	FF(b, c, d, a, in[11], 19);	/* 12 */
	FF(a, b, c, d, in[12], 3);	/* 13 */
	FF(d, a, b, c, in[13], 7);	/* 14 */
	FF(c, d, a, b, in[14], 11);	/* 15 */
	FF(b, c, d, a, in[15], 19);	/* 16 */

	GG(a, b, c, d, in[0], 3);	/* 17 */
	GG(d, a, b, c, in[4], 5);	/* 18 */
	GG(c, d, a, b, in[8], 9);	/* 19 */
	GG(b, c, d, a, in[12], 13);	/* 20 */
	GG(a, b, c, d, in[1], 3);	/* 21 */
	GG(d, a, b, c, in[5], 5);	/* 22 */
	GG(c, d, a, b, in[9], 9);	/* 23 */
	GG(b, c, d, a, in[13], 13);	/* 24 */
	GG(a, b, c, d, in[2], 3);	/* 25 */
	GG(d, a, b, c, in[6], 5);	/* 26 */
	GG(c, d, a, b, in[10], 9);	/* 27 */
	GG(b, c, d, a, in[14], 13);	/* 28 */
	GG(a, b, c, d, in[3], 3);	/* 29 */
	GG(d, a, b, c, in[7], 5);	/* 30 */
	GG(c, d, a, b, in[11], 9);	/* 31 */
	GG(b, c, d, a, in[15], 13);	/* 32 */

	HH(a, b, c, d, in[0], 3);	/* 33 */
	HH(d, a, b, c, in[8], 9);	/* 34 */
	HH(c, d, a, b, in[4], 11);	/* 35 */
	HH(b, c, d, a, in[12], 15);	/* 36 */
	HH(a, b, c, d, in[2], 3);	/* 37 */
	HH(d, a, b, c, in[10], 9);	/* 38 */
	HH(c, d, a, b, in[6], 11);	/* 39 */
	HH(b, c, d, a, in[14], 15);	/* 40 */
	HH(a, b, c, d, in[1], 3);	/* 41 */
	HH(d, a, b, c, in[9], 9);	/* 42 */
	HH(c, d, a, b, in[5], 11);	/* 43 */
	HH(b, c, d, a, in[13], 15);	/* 44 */
	HH(a, b, c, d, in[3], 3);	/* 45 */
	HH(d, a, b, c, in[11], 9);	/* 46 */
	HH(c, d, a, b, in[7], 11);	/* 47 */
	HH(b, c, d, a, in[15], 15);	/* 48 */


	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}
