/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2005 Ziglio Frediano
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

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/sysdep_private.h>
#include "replacements.h"

#if ! HAVE_BASENAME

TDS_RCSID(var, "$Id: basename.c,v 1.6 2011-05-16 08:51:40 freddy77 Exp $");

#ifdef _WIN32
#define TDS_ISDIR_SEPARATOR(c) ((c) == '/' || (c) == '\\')
#else
#define TDS_ISDIR_SEPARATOR(c) ((c) == '/')
#endif

char *tds_basename(char *path)
{
	char *p;

	if (path == NULL)
		return path;

	/* remove trailing directories separators */
	for (p = path + strlen(path); --p > path && TDS_ISDIR_SEPARATOR(*p);)
		*p = '\0';

	p = strrchr(path, '/');
	if (p)
		path = p + 1;
#ifdef _WIN32
	p = strrchr(path, '\\');
	if (p)
		path = p + 1;
#endif
	return path;
}
#endif

