/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
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

#ifndef _tdsstring_h_
#define _tdsstring_h_

/* $Id: tdsstring.h,v 1.21 2010-01-25 23:05:59 freddy77 Exp $ */

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility push(hidden)
#endif

extern const struct tds_dstr tds_str_empty;

/* TODO do some function and use inline if available */

/** \addtogroup dstring
 * @{ 
 */

#if ENABLE_EXTRA_CHECKS
void tds_dstr_init(DSTR * s);
int tds_dstr_isempty(DSTR * s);
char *tds_dstr_buf(DSTR * s);
size_t tds_dstr_len(DSTR * s);
#else
/** init a string with empty */
#define tds_dstr_init(s) \
	do { *(s) = (struct tds_dstr*) &tds_str_empty; } while(0)

/** test if string is empty */
#define tds_dstr_isempty(s) \
	((*(s))->dstr_size == 0)
#define tds_dstr_buf(s) \
	((*(s))->dstr_s)
#define tds_dstr_len(s) \
	((*(s))->dstr_size)
#endif

#define tds_dstr_cstr(s) \
	((const char* ) tds_dstr_buf(s))

void tds_dstr_zero(DSTR * s);
void tds_dstr_free(DSTR * s);

DSTR* tds_dstr_dup(DSTR * s, const DSTR * src);
DSTR* tds_dstr_copy(DSTR * s, const char *src);
DSTR* tds_dstr_copyn(DSTR * s, const char *src, size_t length);
DSTR* tds_dstr_set(DSTR * s, char *src);

/** limit length of string, MUST be <= current length */
DSTR* tds_dstr_setlen(DSTR *s, size_t length);
/** allocate space for length char */
DSTR* tds_dstr_alloc(DSTR *s, size_t length);

/** @} */

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility pop
#endif

#endif /* _tdsstring_h_ */
