/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2010 Frediano Ziglio
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
#undef NDEBUG
#include "common.h"

#include <ctype.h>
#include <assert.h>

int utf8_max_len = 0;

int
get_unichar(const char **psrc)
{
	const char *src = *psrc;
	int n;

	if (!*src) return -1;

	if (src[0] == '&' && src[1] == '#') {
		char *end;
		int radix = 10;

		if (toupper(src[2]) == 'X') {
			radix = 16;
			++src;
		}
		n = strtol(src+2, &end, radix);
		assert(*end == ';' && n > 0 && n < 0x10000);
		src = end + 1;
	} else {
		n = (unsigned char) *src++;
	}
	*psrc = src;
	return n;
}

char *
to_utf8(const char *src, char *dest)
{
	unsigned char *p = (unsigned char *) dest;
	int len = 0, n;

	while ((n=get_unichar(&src)) > 0) {
		if (n >= 0x2000) {
			*p++ = 0xe0 | (n >> 12);
			*p++ = 0x80 | ((n >> 6) & 0x3f);
			*p++ = 0x80 | (n & 0x3f);
		} else if (n >= 0x80) {
			*p++ = 0xc0 | (n >> 6);
			*p++ = 0x80 | (n & 0x3f);
		} else {
			*p++ = (unsigned char) n;
		}
		++len;
	}
	if (len > utf8_max_len)
		utf8_max_len = len;
	*p = 0;
	return dest;
}

