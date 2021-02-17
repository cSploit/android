/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2011  Frediano Ziglio
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

/* 
 * Purpose: test challenge code.
 */
#include "common.h"
#include <assert.h>
#include "md4.h"
#include "md5.h"
#include "hmac_md5.h"
#include "des.h"

static char software_version[] = "$Id: challenge.c,v 1.2 2011-08-08 16:56:13 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static char *
bin2ascii(char *dest, const void *data, size_t len)
{
	char *s = dest;
	const unsigned char *src = (const unsigned char *) data;
	for (; len > 0; --len, s += 2)
		sprintf(s, "%02x", *src++);
	*s = 0;
	return dest;
}

static void
md4(const char *src, const char *out)
{
	MD4_CTX ctx;
	unsigned char digest[16];
	char s_digest[34];

	assert(strlen(out) == 32);
	MD4Init(&ctx);
	if (strlen(src) > 12) {
		MD4Update(&ctx, (const unsigned char *) src, 5);
		MD4Update(&ctx, (const unsigned char *) src+5, strlen(src) - 5);
	} else {
		MD4Update(&ctx, (const unsigned char *) src, strlen(src));
	}
	MD4Final(&ctx, digest);
	if (strcasecmp(bin2ascii(s_digest, digest, 16), out) != 0) {
		fprintf(stderr, "Wrong md4(%s) -> %s expected %s\n", src, s_digest, out);
		exit(1);
	}
}

static void
md4tests(void)
{
	md4("", "31d6cfe0d16ae931b73c59d7e0c089c0");
	md4("a", "bde52cb31de33e46245e05fbdbd6fb24");
	md4("abc", "a448017aaf21d8525fc10ae87aa6729d");
	md4("message digest", "d9130a8164549fe818874806e1c7014b");
	md4("abcdefghijklmnopqrstuvwxyz", "d79e1c308aa5bbcdeea8ed63df412da9");
	md4("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "043f8582f241db351ce627e153e7f0e4");
	md4("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "e33b4ddc9c38f2199c3e7b164fcc0536");
}

static void
md5(const char *src, const char *out)
{
	MD5_CTX ctx;
	unsigned char digest[16];
	char s_digest[34];

	assert(strlen(out) == 32);
	MD5Init(&ctx);
	if (strlen(src) > 12) {
		MD5Update(&ctx, (const unsigned char *) src, 5);
		MD5Update(&ctx, (const unsigned char *) src+5, strlen(src) - 5);
	} else {
		MD5Update(&ctx, (const unsigned char *) src, strlen(src));
	}
	MD5Final(&ctx, digest);
	if (strcasecmp(bin2ascii(s_digest, digest, 16), out) != 0) {
		fprintf(stderr, "Wrong md5(%s) -> %s expected %s\n", src, s_digest, out);
		exit(1);
	}
}

static void
md5tests(void)
{
	md5("", "d41d8cd98f00b204e9800998ecf8427e");
	md5("The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6");
	md5("The quick brown fox jumps over the lazy dog.", "e4d909c290d0fb1ca068ffaddf22cbd0");
}

static const char *hmac5_key = NULL;

static void
hmac5(const char *src, const char *out)
{
	unsigned char digest[16];
	char s_digest[34];

	assert(strlen(out) == 32 && hmac5_key);
	hmac_md5((const unsigned char*) hmac5_key, (const unsigned char*) src, strlen(src), digest);
	if (strcasecmp(bin2ascii(s_digest, digest, 16), out) != 0) {
		fprintf(stderr, "Wrong hman md5(%s) -> %s expected %s\n", src, s_digest, out);
		exit(1);
	}
}

static void
hmac5tests(void)
{
	hmac5_key = "01234567890123456";
	hmac5("", "c26c57d7ff1236db11d7419a490ad84d");
	hmac5("message digest", "e4f89edbda3d3b6b7328a0a90f87ece8");
	hmac5("abcdefghijklmnopqrstuvwxyz", "2c4eac474ec340df63ae93b8ffc33571");
}

static void
des(const char *src, const char *out)
{
	static const des_cblock key = { 0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25 };
	DES_KEY ks;
	unsigned char digest[128];
	char s_digest[256];
	size_t out_len;

	memset(digest, 0, sizeof(digest));
	tds_des_set_key(&ks, key, sizeof(key));
	tds_des_ecb_encrypt(src, strlen(src), &ks, digest);

	out_len = strlen(src) & ~7u;
	if (strcasecmp(bin2ascii(s_digest, digest, out_len), out) != 0) {
		fprintf(stderr, "Wrong des(%s) -> %s expected %s\n", src, s_digest, out);
		exit(1);
	}
}

static void
destests(void)
{
	des("", "");
	des("The quick brown fox jumps over the lazy dog",
	    "51551eab3ebab959553caaed64a3dd9c49f595a630c45cb7317332f8ade70308c4e97aeabbdc7f19");
	des("test des encryption",
	    "7ced9849bed3f7efc1686c89759bafa8");
}


int
main(void)
{
	md4tests();
	md5tests();
	hmac5tests();
	destests();
	printf("All tests passed\n");
	return 0;
}

