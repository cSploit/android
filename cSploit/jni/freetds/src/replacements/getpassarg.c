/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2006, 2007, 2008, 2009, 2010  Frediano Ziglio
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

#include <stdio.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include "replacements.h"

/*
 * return a copy of the password, reading from stdin if arg is '-'
 * trashing he argument in the process.
 */
char *
getpassarg(char *arg)
{
	char pwd[256], *ptr, *q;

	if (strcmp(arg, "-") == 0) {
		if (fgets(pwd, sizeof(pwd), stdin) == NULL)
			return NULL;
		ptr = strchr(pwd, '\n');
		if (ptr) *ptr = '\0';
		arg = pwd;
	}

	ptr = strdup(arg);
	memset(pwd, 0, sizeof(pwd));

	for (q = arg; *q; *q++ = '*')
		continue;

	return ptr;
}

