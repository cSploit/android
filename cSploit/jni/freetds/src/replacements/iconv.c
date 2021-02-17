/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2003, 2004  James K. Lowden, based on original work by Brian Bruns
 * Copyright (C) 2011 Frediano Ziglio
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

/**
 * \file
 * This file implements a very simple iconv.  
 * Its purpose is to allow ASCII clients to communicate with Microsoft servers
 * that encode their metadata in Unicode (UTF-16).
 *
 * It supports ISO-8859-1, ASCII, UTF-16, UCS-4 and UTF-8
 */

#include <config.h>

#if ! HAVE_ICONV

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include <assert.h>
#include <ctype.h>

#include <freetds/tds.h>
#include <freetds/bytes.h>
#include <freetds/iconv.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: iconv.c,v 1.22 2011-05-16 08:51:40 freddy77 Exp $");

/**
 * \addtogroup conv
 * @{ 
 */

enum ICONV_CD_VALUE
{
	Like_to_Like = 0x100
};

typedef TDS_UINT ICONV_CHAR;

static const unsigned char utf8_lengths[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0,
};

static const unsigned char utf8_masks[7] = {
	0, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

static int
get_utf8(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	ICONV_CHAR uc;
	size_t l;

	l = utf8_lengths[p[0]];
	if (TDS_UNLIKELY(l == 0))
		return -EILSEQ;
	if (TDS_UNLIKELY(len < l))
		return -EINVAL;

	len = l;
	uc = *p++ & utf8_masks[l];
	while(--l)
		uc = (uc << 6) | (*p++ & 0x3f);
	*out = uc;
	return len;
}

static int
put_utf8(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
#define MASK(n) ((0xffffffffu << (n)) & 0xffffffffu)
	size_t o_len;
	unsigned mask;

	if ((c & MASK(7)) == 0) {
		if (buf_len < 1)
			return -E2BIG;
		*buf = (unsigned char) c;
		return 1;
	}

	o_len = 2;
	for (;;) {
		if ((c & MASK(11)) == 0)
			break;
		++o_len;
		if ((c & MASK(16)) == 0)
			break;
		++o_len;
		if ((c & MASK(21)) == 0)
			break;
		++o_len;
		if ((c & MASK(26)) == 0)
			break;
		++o_len;
		if ((c & MASK(31)) != 0)
			return -EINVAL;
	}

	if (buf_len < o_len)
		return -E2BIG;
	buf += o_len;
	mask = 0xff80;
	for (;;) {
		*--buf = 0x80 | (c & 0x3f);
		c >>= 6;
		mask >>= 1;
		if (c < 0x40) {
			*--buf = mask | c;
			break;
		}
	}
	return o_len;
}

static int
get_ucs4le(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	if (len < 4)
		return -EINVAL;
	*out = TDS_GET_A4LE(p);
	return 4;
}

static int
put_ucs4le(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (buf_len < 4)
		return -E2BIG;
	TDS_PUT_A4LE(buf, c);
	return 4;
}

static int
get_ucs4be(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	if (len < 4)
		return -EINVAL;
	*out = TDS_GET_A4BE(p);
	return 4;
}

static int
put_ucs4be(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (buf_len < 4)
		return -E2BIG;
	TDS_PUT_A4BE(buf, c);
	return 4;
}

static int
get_utf16le(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	ICONV_CHAR c, c2;

	if (len < 2)
		return -EINVAL;
	c = TDS_GET_A2LE(p);
	if ((c & 0xfc00) == 0xd800) {
		if (len < 4)
			return -EINVAL;
		c2 = TDS_GET_A2LE(p+2);
		if ((c2 & 0xfc00) != 0xdc00)
			return -EILSEQ;
		*out = (c << 10) + c2 - ((0xd800 << 10) + 0xdc00 - 0x10000);
		return 4;
	}
	*out = c;
	return 2;
}

static int
put_utf16le(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (c >= 0x110000u)
		return -EILSEQ;
	if (c < 0x10000u) {
		if (buf_len < 2)
			return -E2BIG;
		TDS_PUT_A2LE(buf, c);
		return 2;
	}
	if (buf_len < 4)
		return -E2BIG;
	c -= 0x10000u;
	TDS_PUT_A2LE(buf,   0xd800 + (c >> 10));
	TDS_PUT_A2LE(buf+2, 0xdc00 + (c & 0x3ffu));
	return 4;
}

static int
get_utf16be(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	ICONV_CHAR c, c2;

	if (len < 2)
		return -EINVAL;
	c = TDS_GET_A2BE(p);
	if ((c & 0xfc00) == 0xd800) {
		if (len < 4)
			return -EINVAL;
		c2 = TDS_GET_A2BE(p+2);
		if ((c2 & 0xfc00) != 0xdc00)
			return -EILSEQ;
		*out = (c << 10) + c2 - ((0xd800 << 10) + 0xdc00 - 0x10000);
		return 4;
	}
	*out = c;
	return 2;
}

static int
put_utf16be(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (c >= 0x110000u)
		return -EILSEQ;
	if (c < 0x10000u) {
		if (buf_len < 2)
			return -E2BIG;
		TDS_PUT_A2BE(buf, c);
		return 2;
	}
	if (buf_len < 4)
		return -E2BIG;
	c -= 0x10000u;
	TDS_PUT_A2BE(buf,   0xd800 + (c >> 10));
	TDS_PUT_A2BE(buf+2, 0xdc00 + (c & 0x3ffu));
	return 4;
}

static int
get_iso1(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	*out = p[0];
	return 1;
}

static int
put_iso1(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (c >= 0x100u)
		return -EILSEQ;
	if (buf_len < 1)
		return -E2BIG;
	buf[0] = (unsigned char) c;
	return 1;
}

static int
get_ascii(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	if (p[0] >= 0x80)
		return -EILSEQ;
	*out = p[0];
	return 1;
}

static int
put_ascii(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	if (c >= 0x80u)
		return -EILSEQ;
	if (buf_len < 1)
		return -E2BIG;
	buf[0] = (unsigned char) c;
	return 1;
}

static int
get_err(const unsigned char *p, size_t len, ICONV_CHAR *out)
{
	return -EILSEQ;
}

static int
put_err(unsigned char *buf, size_t buf_len, ICONV_CHAR c)
{
	return -EILSEQ;
}

typedef int (*iconv_get_t)(const unsigned char *p, size_t len,     ICONV_CHAR *out);
typedef int (*iconv_put_t)(unsigned char *buf,     size_t buf_len, ICONV_CHAR c);

static const iconv_get_t iconv_gets[8] = {
	get_iso1, get_ascii, get_utf16le, get_utf16be, get_ucs4le, get_ucs4be, get_utf8, get_err
};
static const iconv_put_t iconv_puts[8] = {
	put_iso1, put_ascii, put_utf16le, put_utf16be, put_ucs4le, put_ucs4be, put_utf8, put_err
};

/** 
 * Inputs are FreeTDS canonical names, no other. No alias list is consulted.  
 */
iconv_t 
tds_sys_iconv_open (const char* tocode, const char* fromcode)
{
	int i;
	unsigned int fromto;
	const char *enc_name;
	unsigned char encodings[2];

	static char first_time = 1;

	if (first_time) {
		first_time = 0;
		tdsdump_log(TDS_DBG_INFO1, "Using trivial iconv\n");
	}
	
	/* match both inputs to our canonical names */
	enc_name = fromcode;
	for (i=0; i < 2; ++i) {
		unsigned char encoding;

		if (strcmp(enc_name, "ISO-8859-1") == 0)
			encoding = 0;
		else if (strcmp(enc_name, "US-ASCII") == 0)
			encoding = 1;
		else if (strcmp(enc_name, "UCS-2LE") == 0 || strcmp(enc_name, "UTF-16LE") == 0)
			encoding = 2;
		else if (strcmp(enc_name, "UCS-2BE") == 0 || strcmp(enc_name, "UTF-16BE") == 0)
			encoding = 3;
		else if (strcmp(enc_name, "UCS-4LE") == 0)
			encoding = 4;
		else if (strcmp(enc_name, "UCS-4BE") == 0)
			encoding = 5;
		else if (strcmp(enc_name, "UTF-8") == 0)
			encoding = 6;
		else {
			errno = EINVAL;
			return (iconv_t)(-1);
		}
		encodings[i] = encoding;

		enc_name = tocode;
	}

	fromto = (encodings[0] << 4) | (encodings[1] & 0x0F);

	/* like to like */
	if (encodings[0] == encodings[1]) {
		fromto = Like_to_Like;
	}

	return (iconv_t) (TDS_INTPTR) fromto;
} 

int 
tds_sys_iconv_close (iconv_t cd)
{
	return 0;
}

size_t 
tds_sys_iconv (iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft)
{
	const unsigned char *ib;
	unsigned char *ob;
	size_t il, ol;
	int local_errno;

#undef CD
#define CD ((int) (TDS_INTPTR) cd)

	/* iconv defines valid semantics for NULL inputs, but we don't support them. */
	if (!inbuf || !*inbuf || !inbytesleft || !outbuf || !*outbuf || !outbytesleft)
		return 0;
	
	/* 
	 * some optimizations
	 * - do not use errno directly only assign a time
	 *   (some platform define errno as a complex macro)
	 * - some processors have few registers, deference and copy input variable
	 *   (this make also compiler optimize more due to removed aliasing)
	 *   also we use unsigned to remove required unsigned casts
	 */
	local_errno = 0;
	il = *inbytesleft;
	ol = *outbytesleft;
	ib = (const unsigned char*) *inbuf;
	ob = (unsigned char*) *outbuf;

	if (CD == Like_to_Like) {
		size_t copybytes = (il < ol)? il : ol;

		memcpy(ob, ib, copybytes);
		ob += copybytes;
		ol -= copybytes;
		ib += copybytes;
		il -= copybytes;
	} else if (CD & ~0x77) {
		local_errno = EINVAL;
	} else {
		iconv_get_t get_func = iconv_gets[(CD>>4) & 7];
		iconv_put_t put_func = iconv_puts[ CD     & 7];

		while (il) {
			ICONV_CHAR out_c;
			int readed = get_func(ib, il, &out_c), written;

			if (TDS_UNLIKELY(readed < 0)) {
				local_errno = -readed;
				break;
			}

			written = put_func(ob, ol, out_c);
			if (TDS_UNLIKELY(written < 0)) {
				local_errno = -written;
				break;
			}
			il -= readed;
			ib += readed;
			ol -= written;
			ob += written;
		}
	}

	/* back to source */
	*inbytesleft = il;
	*outbytesleft = ol;
	*inbuf = (const char*) ib;
	*outbuf = (char*) ob;

	if (il && !local_errno)
		local_errno = E2BIG;
	
	if (local_errno) {
		errno = local_errno;
		return (size_t)(-1);
	}
	
	return 0;
}


/** @} */

#endif
