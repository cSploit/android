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

/* $Id: strlcpy.c,v 1.2 2011-05-16 08:51:40 freddy77 Exp $ */

size_t 
tds_strlcpy(char *dest, const char *src, size_t len)
{
	size_t l = strlen(src);

	--len;
	if (l > len) {
		memcpy(dest, src, len);
		dest[len] = 0;
	} else {
		memcpy(dest, src, l + 1);
	}
	return l;
}

