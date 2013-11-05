/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */
/* This code is in the public domain */

// Adopted and added to firebird cvs tree - A.Peshkov, 2004

#ifndef SHA_H
#define SHA_H

#include <stdlib.h>
#include <stdio.h>
#include "../jrd/sha.h"
#include "../common/classes/array.h"
#include "../jrd/os/guid.h"

namespace
{

/* Useful defines & typedefs */
typedef unsigned char BYTE;	/* 8-bit quantity */
typedef unsigned long LONG;	/* 32-or-more-bit quantity */

#define SHA_BLOCKSIZE		64
#define SHA_DIGESTSIZE		20

struct SHA_INFO
{
	LONG digest[5];			/* message digest */
	LONG count_lo, count_hi;	/* 64-bit bit count */
	BYTE data[SHA_BLOCKSIZE];	/* SHA data buffer */
	int local;			/* unprocessed amount in data */
};

void sha_init(SHA_INFO *);
void sha_update(SHA_INFO *, const BYTE *, int);
void sha_final(unsigned char [SHA_DIGESTSIZE], SHA_INFO *);

#define SHA_VERSION 1

#include "firebird.h"

#ifdef WORDS_BIGENDIAN
#  if SIZEOF_LONG == 4
#    define SHA_BYTE_ORDER  4321
#  elif SIZEOF_LONG == 8
#    define SHA_BYTE_ORDER  87654321
#  endif
#else
#  if SIZEOF_LONG == 4
#    define SHA_BYTE_ORDER  1234
#  elif SIZEOF_LONG == 8
#    define SHA_BYTE_ORDER  12345678
#  endif
#endif

#endif /* SHA_H */



/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain
 * for more info.
 *
 * NIST Secure Hash Algorithm
 * heavily modified by Uwe Hollerbach <uh@alumni.caltech edu>
 * from Peter C. Gutmann's implementation as found in
 * Applied Cryptography by Bruce Schneier
 * Further modifications to include the "UNRAVEL" stuff, below
 *
 * This code is in the public domain
 *
 */

/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#define UNRAVEL
/* #define UNROLL_LOOPS */

/* SHA f()-functions */

#define f1(x, y, z)	((x & y) | (~x & z))
#define f2(x, y, z)	(x ^ y ^ z)
#define f3(x, y, z)	((x & y) | (x & z) | (y & z))
#define f4(x, y, z)	(x ^ y ^ z)


/* SHA constants */

#define CONST1		0x5a827999L
#define CONST2		0x6ed9eba1L
#define CONST3		0x8f1bbcdcL
#define CONST4		0xca62c1d6L

/* truncate to 32 bits -- should be a null op on 32-bit machines */

#define T32(x)	((x) & 0xffffffffL)

/* 32-bit rotate */

#define R32(x, n)	T32(((x << n) | (x >> (32 - n))))

/* the generic case, for when the overall rotation is not unraveled */

#define FG(n)	\
	T = T32(R32(A, 5) + f##n(B, C, D) + E + *WP++ + CONST##n);	\
	E = D; D = C; C = R32(B, 30); B = A; A = T

/* specific cases, for when the overall rotation is unraveled */

#define FA(n)	\
	T = T32(R32(A, 5) + f##n(B, C, D) + E + *WP++ + CONST##n); B = R32(B, 30)

#define FB(n)	\
	E = T32(R32(T, 5) + f##n(A, B, C) + D + *WP++ + CONST##n); A = R32(A, 30)

#define FC(n)	\
	D = T32(R32(E, 5) + f##n(T, A, B) + C + *WP++ + CONST##n); T = R32(T, 30)

#define FD(n)	\
	C = T32(R32(D, 5) + f##n(E, T, A) + B + *WP++ + CONST##n); E = R32(E, 30)

#define FE(n)	\
	B = T32(R32(C, 5) + f##n(D, E, T) + A + *WP++ + CONST##n); D = R32(D, 30)

#define FT(n)	\
	A = T32(R32(B, 5) + f##n(C, D, E) + T + *WP++ + CONST##n); C = R32(C, 30)


/* do SHA transformation */

static void sha_transform(SHA_INFO *sha_info)
{
	int i;
	LONG W[80];

	const BYTE* dp = sha_info->data;

/*
the following makes sure that at least one code block below is
traversed or an error is reported, without the necessity for nested
preprocessor if/else/endif blocks, which are a great pain in the
nether regions of the anatomy...
*/
#undef SWAP_DONE

#if (SHA_BYTE_ORDER == 1234)
#define SWAP_DONE
	for (i = 0; i < 16; ++i)
	{
		const LONG T = *((LONG *) dp);
		dp += 4;
		W[i] =  ((T << 24) & 0xff000000) | ((T <<  8) & 0x00ff0000) |
			((T >>  8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
	}
#endif /* SHA_BYTE_ORDER == 1234 */

#if (SHA_BYTE_ORDER == 4321)
#define SWAP_DONE
	for (i = 0; i < 16; ++i)
	{
		const LONG T = *((LONG *) dp);
		dp += 4;
		W[i] = T32(T);
	}
#endif /* SHA_BYTE_ORDER == 4321 */

#if (SHA_BYTE_ORDER == 12345678)
#define SWAP_DONE
	for (i = 0; i < 16; i += 2)
	{
		LONG T = *((LONG *) dp);
		dp += 8;
		W[i] =  ((T << 24) & 0xff000000) | ((T <<  8) & 0x00ff0000) |
			((T >>  8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
		T >>= 32;
		W[i + 1] = ((T << 24) & 0xff000000) | ((T <<  8) & 0x00ff0000) |
			((T >>  8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
	}
#endif /* SHA_BYTE_ORDER == 12345678 */

#if (SHA_BYTE_ORDER == 87654321)
#define SWAP_DONE
	for (i = 0; i < 16; i += 2)
	{
		const LONG T = *((LONG *) dp);
		dp += 8;
		W[i] = T32(T >> 32);
		W[i + 1] = T32(T);
	}
#endif /* SHA_BYTE_ORDER == 87654321 */

#ifndef SWAP_DONE
#error Unknown byte order -- you need to add code here
#endif /* SWAP_DONE */

	for (i = 16; i < 80; ++i)
	{
		W[i] = W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16];
#if (SHA_VERSION == 1)
		W[i] = R32(W[i], 1);
#endif /* SHA_VERSION */
	}
	LONG A = sha_info->digest[0];
	LONG B = sha_info->digest[1];
	LONG C = sha_info->digest[2];
	LONG D = sha_info->digest[3];
	LONG E = sha_info->digest[4];
	const LONG* WP = W;
	LONG T;
#ifdef UNRAVEL
	FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
	FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
	FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
	FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
	FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
	FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
	FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
	FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
	sha_info->digest[0] = T32(sha_info->digest[0] + E);
	sha_info->digest[1] = T32(sha_info->digest[1] + T);
	sha_info->digest[2] = T32(sha_info->digest[2] + A);
	sha_info->digest[3] = T32(sha_info->digest[3] + B);
	sha_info->digest[4] = T32(sha_info->digest[4] + C);
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
	FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
	FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
	FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
	FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
	FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
	FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
	FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
	FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
#else /* !UNROLL_LOOPS */
	for (i =  0; i < 20; ++i) { FG(1); }
	for (i = 20; i < 40; ++i) { FG(2); }
	for (i = 40; i < 60; ++i) { FG(3); }
	for (i = 60; i < 80; ++i) { FG(4); }
#endif /* !UNROLL_LOOPS */
	sha_info->digest[0] = T32(sha_info->digest[0] + A);
	sha_info->digest[1] = T32(sha_info->digest[1] + B);
	sha_info->digest[2] = T32(sha_info->digest[2] + C);
	sha_info->digest[3] = T32(sha_info->digest[3] + D);
	sha_info->digest[4] = T32(sha_info->digest[4] + E);
#endif /* !UNRAVEL */
}

/* initialize the SHA digest */

void sha_init(SHA_INFO *sha_info)
{
	sha_info->digest[0] = 0x67452301L;
	sha_info->digest[1] = 0xefcdab89L;
	sha_info->digest[2] = 0x98badcfeL;
	sha_info->digest[3] = 0x10325476L;
	sha_info->digest[4] = 0xc3d2e1f0L;
	sha_info->count_lo = 0L;
	sha_info->count_hi = 0L;
	sha_info->local = 0;
}

/* update the SHA digest */

void sha_update(SHA_INFO *sha_info, const BYTE *buffer, int count)
{
	const LONG clo = T32(sha_info->count_lo + ((LONG) count << 3));
	if (clo < sha_info->count_lo) {
		++sha_info->count_hi;
	}
	sha_info->count_lo = clo;
	sha_info->count_hi += (LONG) count >> 29;
	if (sha_info->local)
	{
		int i = SHA_BLOCKSIZE - sha_info->local;
		if (i > count) {
			i = count;
		}
		memcpy(sha_info->data + sha_info->local, buffer, i);
		count -= i;
		buffer += i;
		sha_info->local += i;
		if (sha_info->local == SHA_BLOCKSIZE) {
			sha_transform(sha_info);
		}
		else {
			return;
		}
	}
	while (count >= SHA_BLOCKSIZE)
	{
		memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
		buffer += SHA_BLOCKSIZE;
		count -= SHA_BLOCKSIZE;
		sha_transform(sha_info);
	}
	memcpy(sha_info->data, buffer, count);
	sha_info->local = count;
}

/* finish computing the SHA digest */

void sha_final(unsigned char digest[SHA_DIGESTSIZE], SHA_INFO *sha_info)
{
	const LONG lo_bit_count = sha_info->count_lo;
	const LONG hi_bit_count = sha_info->count_hi;
	int count = (int) ((lo_bit_count >> 3) & 0x3f);
	sha_info->data[count++] = 0x80;
	if (count > SHA_BLOCKSIZE - 8)
	{
		memset(sha_info->data + count, 0, SHA_BLOCKSIZE - count);
		sha_transform(sha_info);
		memset(sha_info->data, 0, SHA_BLOCKSIZE - 8);
	}
	else {
		memset(sha_info->data + count, 0, SHA_BLOCKSIZE - 8 - count);
	}
	sha_info->data[56] = (unsigned char) ((hi_bit_count >> 24) & 0xff);
	sha_info->data[57] = (unsigned char) ((hi_bit_count >> 16) & 0xff);
		sha_info->data[58] = (unsigned char) ((hi_bit_count >>  8) & 0xff);
	sha_info->data[59] = (unsigned char) ((hi_bit_count >>  0) & 0xff);
	sha_info->data[60] = (unsigned char) ((lo_bit_count >> 24) & 0xff);
	sha_info->data[61] = (unsigned char) ((lo_bit_count >> 16) & 0xff);
	sha_info->data[62] = (unsigned char) ((lo_bit_count >>  8) & 0xff);
	sha_info->data[63] = (unsigned char) ((lo_bit_count >>  0) & 0xff);
	sha_transform(sha_info);
	digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
	digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
	digest[ 2] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
	digest[ 3] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
	digest[ 4] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
	digest[ 5] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
	digest[ 6] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
	digest[ 7] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
	digest[ 8] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
	digest[ 9] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
	digest[10] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
	digest[11] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
	digest[12] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
	digest[13] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
	digest[14] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
	digest[15] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
	digest[16] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
	digest[17] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
	digest[18] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
	digest[19] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
}

inline char conv_bin2ascii(ULONG l)
{
	return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[l & 0x3f];
}

typedef Firebird::HalfStaticArray<unsigned char, SHA_DIGESTSIZE> BinHash;

void base64(Firebird::string& b64, const BinHash& bin)
{
	b64.erase();
	const unsigned char* f = bin.begin();
	for (int i = bin.getCount(); i > 0; i -= 3, f += 3)
	{
		if (i >= 3)
		{
			const ULONG l = (ULONG(f[0]) << 16) | (ULONG(f[1]) <<  8) | f[2];
			b64 += conv_bin2ascii(l >> 18);
			b64 += conv_bin2ascii(l >> 12);
			b64 += conv_bin2ascii(l >> 6);
			b64 += conv_bin2ascii(l);
		}
		else
		{
			ULONG l = ULONG(f[0]) << 16;
			if (i == 2)
				l |= (ULONG(f[1]) << 8);
			b64 += conv_bin2ascii(l >> 18);
			b64 += conv_bin2ascii(l >> 12);
			b64 += (i == 1 ? '=' : conv_bin2ascii(l >> 6));
			b64 += '=';
		}
	}
}

}	// anon namespace

void Jrd::CryptSupport::hash(Firebird::string& hashValue, const Firebird::string& data)
{
	SHA_INFO si;
	sha_init(&si);
	sha_update(&si, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
	BinHash bh;
	sha_final(bh.getBuffer(SHA_DIGESTSIZE), &si);
	base64(hashValue, bh);
}

void Jrd::CryptSupport::random(Firebird::string& randomValue, size_t length)
{
	BinHash binRand;
	GenerateRandomBytes(binRand.getBuffer(length), length);
	base64(randomValue, binRand);
	randomValue.resize(length, '$');
}

