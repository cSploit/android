/*
    wcs.c - Wide character handling functions
    Copyright (C) 1999, 2000  Petr Vandrovec

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Revision history:

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Created from nwnet.c

	1.01  2000, February 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added wcsrev().

	1.02  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include stdlib.h (needed for wcsdup).

 */

#include "config.h"

#include "nwnet_i.h"

#include <stdlib.h>
#include <string.h>

#ifndef HAVE_WCSCPY
/* Do it yourself... Move into specific file?! */
/* we can pass NULL into this procedure */
wchar_t* wcscpy(wchar_t* dst, const wchar_t* src) {
	wchar_t* tmp = dst;
	while ((*dst++ = *src++) != 0) ;
	return tmp;
}
#endif	/* !HAVE_WCSCPY */

#ifndef HAVE_WCSCMP
int wcscmp(const wchar_t* s1, const wchar_t* s2) {
	wint_t c1;
	wint_t c2;
	do {
		c1 = *s1++;
		c2 = *s2++;
		if (!c1)
			break;
	} while (c1 == c2);
	if (c1 < c2)
		return -1;	/* negative */
	return c1 - c2;		/* zero or positive */
}
#endif	/* !HAVE_WCSCMP */

#ifndef HAVE_WCSNCMP
int wcsncmp(const wchar_t* s1, const wchar_t* s2, size_t n) {
	wint_t c1;
	wint_t c2;
	if (!n)
		return 0;
	do {
		int diff;
		
		c1 = *s1++;
		c2 = *s2++;
		if (!c1)
			break;
	} while ((c1 == c2) && --n);
	if (c1 < c2)
		return -1;
	return c1 - c2;
}
#endif	/* !HAVE_WCSNCMP */

#ifndef HAVE_WCSCASECMP
int wcscasecmp(const wchar_t* s1, const wchar_t* s2) {
	wint_t c1;
	wint_t c2;
	do {
		int diff;
		
		c1 = *s1++;
		c2 = *s2++;
		if (!c1)
			break;
		diff = c1 - c2;
		if (diff) {
			if ((c1 >= L'A') && (c1 <= L'Z'))
				c1 += 0x20;
			if ((c2 >= L'A') && (c2 <= L'Z'))
				c2 += 0x20;
			if (c1 != c2) {
				break;
			}
		}
	} while (1);
	if (c1 < c2)
		return -1;
	return c1 - c2;
}
#endif	/* !HAVE_WCSCASECMP */

#ifndef HAVE_WCSNCASECMP
int wcsncasecmp(const wchar_t* s1, const wchar_t* s2, size_t n) {
	wint_t c1;
	wint_t c2;
	if (!n)
		return 0;
	do {
		int diff;
		
		c1 = *s1++;
		c2 = *s2++;
		if (!c1)
			break;
		diff = c1 - c2;
		if (diff) {
			if ((c1 >= L'A') && (c1 <= L'Z'))
				c1 += 0x20;
			if ((c2 >= L'A') && (c2 <= L'Z'))
				c2 += 0x20;
			if (c1 != c2) {
				break;
			}
		}
	} while (--n);
	if (c1 < c2)
		return -1;
	return c1 - c2;
}
#endif	/* !HAVE_WCSNCASECMP */

#ifndef HAVE_WCSLEN
size_t wcslen(const wchar_t* x) {
	const wchar_t* start = x;
	if (x)
		while (*x) x++;
	return x - start;
}
#endif	/* !HAVE_WCSLEN */

#ifndef HAVE_WCSDUP
/* NULL on input -> NULL on output */
wchar_t* wcsdup(const wchar_t* x) {
	size_t l;
	wchar_t* ret;
	
	if (!x) return NULL;
	l = (wcslen(x) + 1) * sizeof(wchar_t);
	ret = (wchar_t*)malloc(l);
	if (ret)
		memcpy(ret, x, l);
	return ret;
}
#endif	/* !HAVE_WCSDUP */

#ifndef HAVE_WCSREV
wchar_t* wcsrev(wchar_t* x) {
	wchar_t* bottom;
	wchar_t* top;
	
	bottom = x;
	top = x + wcslen(x) - 1;
	while (bottom < top) {
		wchar_t tmp;
		
		tmp = *bottom;
		*bottom++ = *top;
		*top-- = tmp;
	}
	return x;
}
#endif
