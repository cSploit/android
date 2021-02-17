/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2004-2005  Frediano Ziglio
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
#include <assert.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <freetds/tds.h>
#include <freetds/string.h>

TDS_RCSID(var, "$Id: tdsstring.c,v 1.21 2011-05-16 08:51:40 freddy77 Exp $");


/**
 * \ingroup libtds
 * \defgroup dstring Dynamic string functions
 * Handle dynamic string. In this string are always valid 
 * (you don't have NULL pointer, only empty string)
 */

/* This is in a separate module because we use the pointer to discriminate allocated and not allocated */
const struct tds_dstr tds_str_empty = { 0, "" };
#define EMPTY ((struct tds_dstr*) &tds_str_empty)

/**
 * \addtogroup dstring
 * @{ 
 */

/** clear all string filling with zeroes (mainly for security reason) */
void
tds_dstr_zero(DSTR * s)
{
	memset((*s)->dstr_s, 0, (*s)->dstr_size);
}

/** free string */
void
tds_dstr_free(DSTR * s)
{
	if (*s != EMPTY)
		free(*s);
	*s = EMPTY;
}

/**
 * Set string to a given buffer of characters
 * @param s      dynamic string
 * @param src    source buffer
 * @param length length of source buffer
 * @return string copied or NULL on memory error
 */
DSTR*
tds_dstr_copyn(DSTR * s, const char *src, size_t length)
{
	if (!length) {
		if (*s != EMPTY) {
			free(*s);
			*s = EMPTY;
		}
	} else {
		struct tds_dstr *p = (struct tds_dstr *) malloc(length + TDS_OFFSET(struct tds_dstr, dstr_s) + 1);
		if (TDS_UNLIKELY(!p))
			return NULL;
		memcpy(p->dstr_s, src, length);
		p->dstr_s[length] = 0;
		p->dstr_size = length;
		if (*s != EMPTY)
			free(*s);
		*s = p;
	}
	return s;
}

/**
 * set a string from another buffer. 
 * The string will use the supplied buffer (it not copy the string),
 * so it should be a pointer returned by malloc.
 * @param s      dynamic string
 * @param src    source buffer
 * @return string copied or NULL on memory error
 */
DSTR*
tds_dstr_set(DSTR * s, char *src)
{
	DSTR *res = tds_dstr_copy(s, src);
	if (TDS_LIKELY(res != NULL))
		free(src);
	return res;
}

/**
 * copy a string from another
 * @param s      dynamic string
 * @param src    source buffer
 * @return string copied or NULL on memory error
 */
DSTR*
tds_dstr_copy(DSTR * s, const char *src)
{
	return tds_dstr_copyn(s, src, strlen(src));
}

DSTR*
tds_dstr_dup(DSTR * s, const DSTR * src)
{
	return tds_dstr_copyn(s, (*src)->dstr_s, (*src)->dstr_size);
}

/**
 * limit length of string, MUST be <= current length
 * @param s        dynamic string
 * @param length   new length 
 */
DSTR*
tds_dstr_setlen(DSTR *s, size_t length)
{
#if ENABLE_EXTRA_CHECKS
	assert((*s)->dstr_size >= length);
#endif
	/* test required for empty strings */
	if ((*s)->dstr_size >= length && *s != EMPTY) {
		(*s)->dstr_size = length;
		(*s)->dstr_s[length] = 0;
	}
	return s;
}

/**
 * allocate space for length char
 * @param s        dynamic string
 * @param length   new length 
 * @return string allocated or NULL on memory error
 */
DSTR*
tds_dstr_alloc(DSTR *s, size_t length)
{
	struct tds_dstr *p = (struct tds_dstr *) malloc(length + TDS_OFFSET(struct tds_dstr, dstr_s) + 1);
	if (TDS_UNLIKELY(!p))
		return NULL;

	if (*s != EMPTY)
		free(*s);
	p->dstr_s[0] = 0;
	p->dstr_size = length;
	*s = p;
	return s;
}

#if ENABLE_EXTRA_CHECKS
void
tds_dstr_init(DSTR * s)
{
	*s = EMPTY;
}

int
tds_dstr_isempty(DSTR * s)
{
	return (*s)->dstr_size == 0;
}

char *
tds_dstr_buf(DSTR * s)
{
	return (*s)->dstr_s;
}

size_t
tds_dstr_len(DSTR * s)
{
	return (*s)->dstr_size;
}
#endif

/** @} */
