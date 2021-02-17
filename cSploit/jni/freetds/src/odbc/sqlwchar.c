/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2008  Ziglio Frediano
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

#include <freetds/odbc.h>

TDS_RCSID(var, "$Id: sqlwchar.c,v 1.4 2012-03-09 21:51:21 freddy77 Exp $");

#if SIZEOF_SQLWCHAR != SIZEOF_WCHAR_T
size_t sqlwcslen(const SQLWCHAR * s)
{
	const SQLWCHAR *p = s;

	while (*p)
		++p;
	return p - s;
}

#ifdef ENABLE_ODBC_WIDE
const wchar_t *sqlwstr(const SQLWCHAR *str)
{
	/*
	 * TODO not thread safe but at least does not use same static buffer
	 * used only for debugging purpose
	 */
	static wchar_t buf[16][256];
	static unsigned next = 0;

	wchar_t *dst_start, *dst, *dst_end;
	const SQLWCHAR *src = str;

	dst = dst_start = buf[next];
	next = (next+1) % TDS_VECTOR_SIZE(buf);
	dst_end = dst + (TDS_VECTOR_SIZE(buf[0]) - 1);
	*dst_end = L'\0';

	for (; *src && dst < dst_end; *dst++ = *src++)
		continue;
	*dst = L'\0';

	return dst_start;
}
#endif
#endif

