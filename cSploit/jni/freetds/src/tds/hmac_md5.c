/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2008  Frediano Ziglio
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

/* $Id: hmac_md5.c,v 1.3 2011-05-16 08:51:40 freddy77 Exp $ */

#include <config.h>

#include <freetds/tds.h>
#include "md5.h"
#include "hmac_md5.h"
#include <memory.h>

/**
 * Calculates the HMAC-MD5 hash of the given data using the specified
 * hashing key.
 *
 * @param key      The hashing key.
 * @param data     The data for which the hash will be calculated. 
 * @param data_len data length. 
 * @param digest   The HMAC-MD5 hash of the given data.
 */
void hmac_md5(const unsigned char key[16],
              const unsigned char* data, size_t data_len,
              unsigned char* digest)
{
	int i;
	MD5_CTX ctx;
	unsigned char k_ipad[64];
	unsigned char k_opad[64];

	/* compute ipad and opad */
	memset(k_ipad, 0x36, sizeof(k_ipad));
	memset(k_opad, 0x5c, sizeof(k_opad));
	for (i=0; i<16; ++i) {
		k_ipad[i] ^= key[i];
		k_opad[i] ^= key[i];
	}

	MD5Init(&ctx);
	MD5Update(&ctx, k_ipad, 64);
	if (data_len != 0)
		MD5Update(&ctx, data, data_len);
	MD5Final(&ctx, digest);

	MD5Init(&ctx);
	MD5Update(&ctx, k_opad, 64);
	MD5Update(&ctx, digest, 16);
	MD5Final(&ctx, digest);
}

