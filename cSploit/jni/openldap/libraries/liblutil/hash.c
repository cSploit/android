/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* This implements the Fowler / Noll / Vo (FNV-1) hash algorithm.
 * A summary of the algorithm can be found at:
 *   http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */

#include "portable.h"

#include <lutil_hash.h>

/* offset and prime for 32-bit FNV-1 */
#define HASH_OFFSET	0x811c9dc5U
#define HASH_PRIME	16777619


/*
 * Initialize context
 */
void
lutil_HASHInit( lutil_HASH_CTX *ctx )
{
	ctx->hash = HASH_OFFSET;
}

/*
 * Update hash
 */
void
lutil_HASHUpdate(
    lutil_HASH_CTX	*ctx,
    const unsigned char		*buf,
    ber_len_t		len )
{
	const unsigned char *p, *e;
	ber_uint_t h;

	p = buf;
	e = &buf[len];

	h = ctx->hash;

	while( p < e ) {
		h *= HASH_PRIME;
		h ^= *p++;
	}

	ctx->hash = h;
}

/*
 * Save hash
 */
void
lutil_HASHFinal( unsigned char *digest, lutil_HASH_CTX *ctx )
{
	ber_uint_t h = ctx->hash;

	digest[0] = h & 0xffU;
	digest[1] = (h>>8) & 0xffU;
	digest[2] = (h>>16) & 0xffU;
	digest[3] = (h>>24) & 0xffU;
}

#ifdef HAVE_LONG_LONG

/* 64 bit Fowler/Noll/Vo-O FNV-1a hash code */

#define HASH64_OFFSET	0xcbf29ce484222325ULL

/*
 * Initialize context
 */
void
lutil_HASH64Init( lutil_HASH_CTX *ctx )
{
	ctx->hash64 = HASH64_OFFSET;
}

/*
 * Update hash
 */
void
lutil_HASH64Update(
    lutil_HASH_CTX	*ctx,
    const unsigned char		*buf,
    ber_len_t		len )
{
	const unsigned char *p, *e;
	unsigned long long h;

	p = buf;
	e = &buf[len];

	h = ctx->hash64;

	while( p < e ) {
		/* xor the bottom with the current octet */
		h ^= *p++;

		/* multiply by the 64 bit FNV magic prime mod 2^64 */
		h += (h << 1) + (h << 4) + (h << 5) +
			(h << 7) + (h << 8) + (h << 40);

	}

	ctx->hash64 = h;
}

/*
 * Save hash
 */
void
lutil_HASH64Final( unsigned char *digest, lutil_HASH_CTX *ctx )
{
	unsigned long long h = ctx->hash;

	digest[0] = h & 0xffU;
	digest[1] = (h>>8) & 0xffU;
	digest[2] = (h>>16) & 0xffU;
	digest[3] = (h>>24) & 0xffU;
	digest[4] = (h>>32) & 0xffU;
	digest[5] = (h>>40) & 0xffU;
	digest[6] = (h>>48) & 0xffU;
	digest[7] = (h>>56) & 0xffU;
}
#endif /* HAVE_LONG_LONG */
