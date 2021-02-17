/*
    nwnet.c - NWDS Calls implementation
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

	0.00  1999, June 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release

	0.05  1999, September 5		Philip R. Wilson <prw@home.com>
		Added DCK_CONFIDENCE flag.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added copyright, couple of important functions and so on...

	1.01  1999, November 20 	Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved wide character functions to wcs.c.
		Moved DN handling functions to rdn.c.

	1.02  1999, December 14		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed __NWDSFinishLoginV2, it can return NWE_PASSWORD_EXPIRED on success.
						
	1.03  2000, January 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetObjectHostServerAddress and NWDSDuplicateContextInt calls.
		Fixed NWDSDuplicateContext call to copy DCK_NAME_CONTEXT.
		
	1.04  2000, January 29		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWDXFindConnection to actually work (we always opened
			new connection).

	1.05  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetCountByClassAndName.

	1.06  2000, April 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetServerName.

	1.07  2000, May 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Modified NWDSGetAttrVal and NWDSComputeAttrValSize to
		support DSV_READ_CLASS_DEF buffers.
		
	1.08  2000, May 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWDSModifyObject iteration handle.

	1.09  2000, July 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Modified NWDSGetAttrValModTime to work on buffers returned
		from NWDSReadAttrDef.

	1.10  2000, July 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added handling of alias referral to NWDSResolveName.

	1.11  2000, July 8		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed handling of charset conversions (you must add
		"alias WCHAR_T// INTERNAL" into /usr/lib/gconv/gconv-modules
		for non-hardwired conversions). 
		Local charset is now retrieved from config file instead of 
		hardcoded ISO-8859-1.
		libc5/6 compatibility iconv now handles also UTF-8.

	1.12  2000, August 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added fallback code to NWDSResolveName. Now we try
		other attached servers, and if everything fails,
		we try resolve name with walk tree on all servers.
		It can slow down resolve process when there are
		dead servers.
		Removed DS_RESOLVE_WALK_TREE from all requests. Resolving
		code add this yourself when needed...
		
	1.13  2000, August 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added "UCS-4LE" and "UCS-4BE" to list of known wchar_t
		encodings. But as glibc2.2 knows "WCHAR_T" encoding,
		everything should be happy on glibc2.2 systems.

	1.14  2001, January 5		PP
		Added NWDSWhoAmI, NWDSScanForAvailableTrees....

	1.15  2001, January 16		PP
		Added Syntaxs API's ( #include ds/syntaxe.c)

	1.16  2001, March 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWDSScanConnsForTrees to match NWSDK doc.
		
	1.17  2001, March 10		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added default tree name and default name context retrieval.

	1.18  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use size_t/nuint32 types as needed.

	1.19  2001, June 3		Ben Harris <bjh21@cam.ac.uk>
		Fix connection leak when NWDSResolveName dereferences alias.

	1.20  2001, November 11		Hans Grobler <grobh@sun.ac.za>
		Fix SYN_BOOLEAN NDS encoding.

	1.21  2001, December 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSDuplicateContext. It is obsolete in NWSDK, but handy
		for Perl...

	1.22  2002, January 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Now you can call NWDSFreeContext with NULL context handle.

	1.23  2002, February 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Limit length of string returned by NWDSGetAttrName by
		MAX_SCHEMA_NAME_BYTES.

 */

#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "nwnet_i.h"
#include "ncplib_i.h"
#include "ncpcode.h"
#include "cfgfile.h"
#include "ncp/nwclient.h"

#if defined(ANDROID) || defined(__BIONIC__)
/* wcsdup is exported as global symbol
 * in the BIONIC libc, but not declared
 * in the headers.
 */
wchar_t* wcsdup(const wchar_t*);
#endif

static const char wchar_init[] = "WCHAR_T//";
static const char* wchar_encoding = wchar_init;
static const char* default_encoding = NULL;

static NWDSCCODE __NWCCGetServerAddressPtr(NWCONN_HANDLE conn, 
		NWObjectCount* count, nuint8** data);

/* debug fprintf */
#if 1
#define dfprintf(X...)
#else
#define dfprintf(X...) fprintf(X)
#endif

size_t unilen(const unicode* x) {
	const unicode* start = x;
	if (x)
		while (*x) x++;
	return x - start;
}

#if 0
static unicode* unirev(unicode* str) {
	unicode* end = str + unilen(str) - 1;
	unicode* oldstr = str;
	while (end > str) {
		unicode tmp;
		
		tmp = *str;
		*str++ = *end;
		*end-- = tmp;
	}
	return oldstr;
}
#endif

/* Charset conversion support for libc5 and libc6.0 */
/* libc6.1 contains iconv interface itself (but buggy and non-working) */
static int iconv_88591_to_wchar_t(const char** inp, size_t* inl,
		char** outp, size_t* outl) {
	const char* i;
	size_t il;
	wchar_t *o;
	size_t ol;
	int ret;
	
	ret = 0;
	ol = *outl;
	o = (wchar_t*)*outp;
	il = *inl;
	i = *inp;
	while (il) {
		if (ol < sizeof(*o)) {
			errno = E2BIG;
			ret = -1;
			goto end;
		}
		*o++ = (*i++) & 0xFF;
		il--;
		ol-=sizeof(*o);
		ret++;
	}
end:;
	*inp = i;
	*inl = il;
	*outp = (char*)o;
	*outl = ol;
	return ret;
}

static int iconv_wchar_t_to_88591(const char** inp, size_t* inl,
		char** outp, size_t* outl) {
	const wchar_t* i;
	size_t il;
	char *o;
	size_t ol;
	int ret;
	
	ret = 0;
	ol = *outl;
	o = *outp;
	il = *inl;
	i = (const wchar_t*)*inp;
	while (il >= sizeof(*i)) {
		if (ol < sizeof(*o)) {
			errno = E2BIG;
			ret = -1;
			goto end;
		}
		*o++ = (*i++) & 0xFF;
		il-=sizeof(*i);
		ol-=sizeof(*o);
		ret++;
	}
end:;
	*inp = (const char*)i;
	*inl = il;
	*outp = o;
	*outl = ol;
	return ret;
}

static int iconv_utf8_to_wchar_t(const char** inp, size_t* inl,
		char** outp, size_t* outl) {
	const unsigned char* i;
	size_t il;
	wchar_t *o;
	size_t ol;
	int ret;
	
	ret = 0;
	ol = *outl;
	o = (wchar_t*)*outp;
	il = *inl;
	i = (const unsigned char*)*inp;
	while (il) {
		wchar_t wc;
		size_t l;
		
		wc = *i;
		if (wc < 0x80) {
			l = 1;
		} else if (wc < 0xC0) {
			goto inval;
		} else if (wc < 0xE0) {
			l = 2;
			wc &= 0x1F;
		} else if (wc < 0xF0) {
			l = 3;
			wc &= 0x0F;
		} else if (wc < 0xF8) {
			l = 4;
			wc &= 0x07;
		} else if (wc < 0xFC) {
			l = 5;
			wc &= 0x03;
		} else if (wc < 0xFE) {
			l = 6;
			wc &= 0x01;
		} else if (wc < 0xFF) {
			l = 7;
			wc = 0;
		} else {
inval:;
			errno = EINVAL;
			ret = -1;
			goto end;
		}
		if (il < l)
			break;
		if (l > 1) {
			size_t p;
			
			for(p = 1; p < l; p++) {
				unsigned char c;

				c = i[p];
				if ((c < 0x80) || (c >= 0xC0)) {
					goto inval;
				}
				wc = (wc << 6) | (c & 0x3F);
			}
		}
		if (ol < sizeof(*o)) {
			errno = E2BIG;
			ret = -1;
			goto end;
		}
		*o++ = wc;
		i += l;
		il -= l;
		ol -= sizeof(*o);
		ret++;
	}
end:;
	*inp = (const char*)i;
	*inl = il;
	*outp = (char*)o;
	*outl = ol;
	return ret;
}

static int iconv_wchar_t_to_utf8(const char** inp, size_t* inl,
		char** outp, size_t* outl) {
	const wchar_t* i;
	size_t il;
	char *o;
	size_t ol;
	int ret;
	
	ret = 0;
	ol = *outl;
	o = *outp;
	il = *inl;
	i = (const wchar_t*)*inp;
	while (il >= sizeof(*i)) {
		unsigned long wc = *i;
		size_t l;
		char first;
		
		if (wc < 128) {
			l = 1;
			first = wc;
		} else if (wc < 0x800) {
			l = 2;
			first = (wc >> 6) | 0xC0;
		} else if (wc < 0x10000) {
			l = 3;
			first = (wc >> 12) | 0xE0;
		} else if (wc < 0x200000) {
			l = 4;
			first = (wc >> 18) | 0xF0;
		} else if (wc < 0x4000000) {
			l = 5;
			first = (wc >> 24) | 0xF8;
		} else if (wc < 0x80000000) {
			l = 6;
			first = (wc >> 30) | 0xFC;
		} else {
			l = 7;
			first = 0xFE;
		}
		if (ol < l) {
			errno = E2BIG;
			ret = -1;
			goto end;
		}
		*o++ = first;
		switch (l) {
			case 7:
				*o++ = ((wc >> 30) & 0x3F) | 0x80;
			case 6:
				*o++ = ((wc >> 24) & 0x3F) | 0x80;
			case 5:
				*o++ = ((wc >> 18) & 0x3F) | 0x80;
			case 4:
				*o++ = ((wc >> 12) & 0x3F) | 0x80;
			case 3:
				*o++ = ((wc >> 6) & 0x3F) | 0x80;
			case 2:
				*o++ = (wc & 0x3F) | 0x80;
		}
		i++;
		il -= sizeof(*i);
		ol -= l;
		ret++;
	}
end:;
	*inp = (const char*)i;
	*inl = il;
	*outp = o;
	*outl = ol;
	return ret;
}

static int iconv_wchar_t_to_wchar_t(const char** inp, size_t* inl,
		char** outp, size_t* outl) {
	const wchar_t* i;
	size_t il;
	wchar_t *o;
	size_t ol;
	int ret;
	
	ret = 0;
	ol = *outl;
	o = (wchar_t*)*outp;
	il = *inl;
	i = (const wchar_t*)*inp;
	while (il >= sizeof(*i)) {
		if (ol < sizeof(*o)) {
			errno = E2BIG;
			ret = -1;
			goto end;
		}
		*o++ = *i++;
		il-=sizeof(*i);
		ol-=sizeof(*o);
		ret++;
	}
end:;
	*inp = (const char*)i;
	*inl = il;
	*outp = (char*)o;
	*outl = ol;
	return ret;
}

static my_iconv_t libc_iconv_open(const char* to, const char* from) {
#ifdef HAVE_ICONV
	my_iconv_t ret;
	iconv_t h = iconv_open(to, from);

	if (h == (iconv_t)-1)
		return (my_iconv_t)-1;
	ret = (my_iconv_t)malloc(sizeof(*ret));
	if (!ret) {
		iconv_close(h);
		errno = ENOMEM;
		return (my_iconv_t)-1;
	}
	ret->lowlevel.h = h;
	ret->type = MY_ICONV_LIBC;
	return ret;
#else
	errno = EINVAL;
	return (my_iconv_t)-1;
#endif
}

my_iconv_t my_iconv_open(const char* to, const char* from) {
	int (*p)(const char** inp, size_t* inl,
		char** outp, size_t* outl) = NULL;
	my_iconv_t ret;
		
	if (!strcmp(from, wchar_encoding) || !strcmp(from, "WCHAR_T//")) {
		if (!strcmp(to, wchar_encoding) || !strcmp(to, "WCHAR_T//"))
			p = iconv_wchar_t_to_wchar_t;
		else if (!strcmp(to, "ISO_8859-1//"))
			p = iconv_wchar_t_to_88591;
		else if (!strcmp(to, "UTF-8//"))
			p = iconv_wchar_t_to_utf8;
	} else if (!strcmp(to, wchar_encoding) || !strcmp(to, "WCHAR_T//")) {
		if (!strcmp(from, "ISO_8859-1//"))
			p = iconv_88591_to_wchar_t;
		else if (!strcmp(from, "UTF-8//"))
			p = iconv_utf8_to_wchar_t;
	}
	/* this conversion is not supported */
	if (!p) {
		return libc_iconv_open(to, from);
	}
	ret = (my_iconv_t)malloc(sizeof(*ret));
	if (!ret) {
		errno = ENOMEM;
		return (my_iconv_t)-1;
	}
	ret->lowlevel.proc = p;
	ret->type = MY_ICONV_INTERNAL;
	return ret;
}

static int my_iconv_is_wchar(my_iconv_t filter) {
	return (filter->type == MY_ICONV_INTERNAL) && (filter->lowlevel.proc == iconv_wchar_t_to_wchar_t);
}

int my_iconv_close(my_iconv_t filter) {
	if (filter->type == MY_ICONV_INTERNAL) {
		;
#ifdef HAVE_ICONV
	} else if (filter->type == MY_ICONV_LIBC) {
		iconv_close(filter->lowlevel.h);
#endif
	}
	free(filter);
	return 0;
}

#define REMOVECONST(X) ((ICONV_CONST char**)ncp_const_cast((X)))
int my_iconv(my_iconv_t filter, const char** inbuf, size_t* inbytesleft,
		char** outbuf, size_t* outbytesleft) {
	if (filter->type == MY_ICONV_INTERNAL) {
		if (inbuf && outbuf)
			return (filter->lowlevel.proc)(inbuf, inbytesleft, outbuf, outbytesleft);
		else
			return 0;
#ifdef HAVE_ICONV
	} else if (filter->type == MY_ICONV_LIBC) {
		/* Grrr! glibc2.1 has const char** as second parameter,
		   while glibc2.2 uses char**... What to do with compiler
		   warning? */
		return iconv(filter->lowlevel.h, REMOVECONST(inbuf), inbytesleft,
			outbuf, outbytesleft);
#endif			
	} else {
		errno = EBADF;
		return -1;
	}
}

static int iconv_is_wchar_encoding(const char* to, const char* from) {
	static const wchar_t expected[] = L"Test";
	my_iconv_t h;
	size_t il;
	size_t ol;
	const char* i;
	char* o;
	char obuf[40];
	int q;
	
	h = libc_iconv_open(to, from);
	if (h == (my_iconv_t)-1)
		return -1;
	dfprintf(stderr, "open ok\n");
	i = "Test";
	il = 4;
	o = obuf;
	ol = sizeof(obuf);
	q = my_iconv(h, &i, &il, &o, &ol);
	my_iconv_close(h);
	if (q == -1)
		return -1;
	dfprintf(stderr, "conv ok, q=%d, il=%d, ol=%d\n", q, il, ol);
	if (sizeof(obuf)-ol != 4 * sizeof(wchar_t))
		return -1;
	dfprintf(stderr, "len ok\n");
	if (memcmp(obuf, expected, 4 * sizeof(wchar_t)))
		return -1;
	dfprintf(stderr, "ok ok\n");
	return 0;
}

static const char* iconv_search_wchar_name(const char* from) {
	static const char *(names[]) = {
		"WCHAR_T//",
		"WCHAR//",
		"UCS-4LE//",
		"UCS4LE//",
		"UCS4LITTLE//",
		"UCS-4BE//",
		"UCS4BE//",
		"UCS4BIG//",
		"UCS4//",
		"ISO-10646//",
		"ISO-10646-LE//",
		"ISO-10646-LITTLE//",
		"ISO-10646-BE//",
		"ISO-10646-BIG//",
		"INTERNAL//",
		NULL };
	const char **x;
	
	/* Yes, it is unbelievable...
	   with glibc up to glibc-2.1.1 (2.1.1 is latest at the time of writting)
	   iconv_open("ASD", "FGH");
	   iconv_open("ASD", "FGH");
	   coredumps in second iconv_open... So it is completely UNUSABLE */
	for (x = names; *x; x++) {
		dfprintf(stderr, "Testing: %s\n", *x);
		if (!iconv_is_wchar_encoding(*x, from))
			break;
	}
	return *x;
}
			
static int __NWUUnicodeToInternal(wchar_t* dest, wchar_t* destEnd,
		const unicode* src, const unicode* srcEnd, UNUSED(wchar_t* noMap),
		wchar_t** destPtr, const unicode** srcPtr) {
	if (!srcEnd) {
		srcEnd = src;
		while (*srcEnd++);
	}
	while (src < srcEnd) {
		if (dest < destEnd) {
			wint_t chr = WVAL_LH(src++, 0);
			*dest++ =  chr;
			continue;
		}
		if (srcPtr)
			*srcPtr = src;
		if (destPtr)
			*destPtr = dest;
		return E2BIG;
	}
	if (srcPtr)
		*srcPtr = src;
	if (destPtr)
		*destPtr = dest;
	return 0;
}

static int __NWUInternalToUnicode(unicode* dest, unicode* destEnd,
		const wchar_t* src, const wchar_t* srcEnd, unicode* noMap, 
		unicode** destPtr, const wchar_t** srcPtr) {
	if (!srcEnd) {
		srcEnd = src + wcslen(src) + 1;
	}
	while (src < srcEnd) {
		int err;
		wint_t chr = *src;
		if (chr < 0x10000) {
			if (dest < destEnd) {
				src++;
				WSET_LH(dest++, 0, chr);
				continue;
			}
			err = E2BIG;
		} else {
			/* TODO: utf-16 add here... */
			if (noMap) {
				unicode* p = noMap;
				unicode* tp;
				
				tp = dest;
				while (*p && (dest < destEnd)) {
					*dest++ = *p++;
				}
				if (!*p) {
					src++;
					continue;
				}
				dest = tp;
				err = E2BIG;
			} else {
				err = EILSEQ;
			}
		}
		if (srcPtr)
			*srcPtr = src;
		if (destPtr)
			*destPtr = dest;
		return err;
	}
	if (srcPtr)
		*srcPtr = src;
	if (destPtr)
		*destPtr = dest;
	return 0;
}

static void __NWULocalInit(my_iconv_t h) {
	my_iconv(h, NULL, NULL, NULL, NULL);
}

static int __NWULocalToInternal(my_iconv_t h, wchar_t* dest, wchar_t* destEnd,
		const char* src, const char* srcEnd, wchar_t* noMap, 
		wchar_t** destPtr, const char** srcPtr) {
	int err = 0;
	size_t destLen = (destEnd - dest) * sizeof(*dest);
	size_t srcLen;
	
	if (!srcEnd) {
		if (my_iconv_is_wchar(h))
			srcEnd = src + (wcslen((const wchar_t*)src) + 1) * sizeof(wchar_t);
		else
			srcEnd = src + strlen(src) + 1;
	}
	srcLen = (srcEnd - src) * sizeof(*src);
	
	while (srcLen > 0) {
		size_t n = my_iconv(h, &src, &srcLen, (char**)(char*)&dest, &destLen);
		if (n != (size_t)-1)
			break;
		err = errno;
		if ((err == EILSEQ) && noMap) {
			wchar_t* p = noMap;
			wchar_t* tp;
			size_t tdl;
			
			tp = dest;
			tdl = destLen;
			while (*p && (destLen >= sizeof(*dest))) {
				*dest++ = *p++;
				destLen -= sizeof(*dest);
			}
			if (!*p) {
				src++;
				srcLen -= sizeof(*src);
				continue;
			}
			dest = tp;
			destLen = tdl;
			err = E2BIG;
		}
		break;
	}
	if (srcPtr)
		*srcPtr = src;
	if (destPtr)
		*destPtr = dest;
	return err;
}

static int __NWUInternalToLocal(my_iconv_t h, char* dest, char* destEnd,
		const wchar_t* src, const wchar_t* srcEnd, char* noMap, 
		char** destPtr, const wchar_t** srcPtr) {
	int err = 0;
	size_t destLen = (destEnd - dest) * sizeof(*dest);
	size_t srcLen;
	
	if (!srcEnd) {
		srcEnd = src + wcslen(src) + 1;
	}
	srcLen = (srcEnd - src) * sizeof(*src);
	
	/* GRRRR: why is not INTERNAL available for iconv?! */
	while (srcLen > 0) {
		size_t n = my_iconv(h, (const char**)(const char*)&src, &srcLen, &dest, &destLen);
		if (n != (size_t)-1)
			break;
		err = errno;
		if ((err == EILSEQ) && noMap) {
			char *p = noMap;
			char* tp;
			size_t tdl;
			
			tp = dest;
			tdl = destLen;
			while (*p && (destLen >= sizeof(*dest))) {
				*dest++ = *p++;
				destLen -= sizeof(*dest);
			}
			if (!*p) {
				src++;
				srcLen -= sizeof(*src);
				continue;
			}
			dest = tp;
			destLen = tdl;
			err = E2BIG;
		}
		break;
	}
	if (srcPtr)
		*srcPtr = src;
	if (destLen)
		*destPtr = dest;
	return err;
}

static NWDSCCODE verifyCharset(const char* tmp) {
	my_iconv_t h;

	h = my_iconv_open(tmp, wchar_encoding);
	if (h == (my_iconv_t)-1)
		return -1;
	my_iconv_close(h);
	return 0;
}

NWDSCCODE NWDSInitRequester(void) {
	static int dsinit = 0;

	if (dsinit)
		return 0;
	if (!default_encoding)
		default_encoding = strdup("ISO_8859-1//");
	if (wchar_encoding == wchar_init) {
		const char* tmp;
		
		tmp = iconv_search_wchar_name(default_encoding);
		if (!tmp) {
			tmp = iconv_search_wchar_name("US-ASCII//");
		}
		if (tmp)
			wchar_encoding = tmp;
	}
	dfprintf(stderr, "iconv: %s\n", wchar_encoding);
	dsinit = 1;
	return 0;
}

struct NWDSConnList {
	NWDS_HANDLE	  dsh;
	NWCONN_HANDLE	  lastconn;
	u_int32_t	  conn_state;
	NWDSCCODE	  ecode;
};

static NWDSCCODE __NWDSListConnectionInit(NWDS_HANDLE dsh, struct NWDSConnList* cnl) {
	cnl->dsh = dsh;
	cnl->lastconn = NULL;
	cnl->conn_state = 0;
	cnl->ecode = 0;
	return 0;
}

static NWDSCCODE __NWDSListConnectionNext(struct NWDSConnList* list, NWCONN_HANDLE* pconn) {
	NWDS_HANDLE dsh;
	NWCONN_HANDLE conn;
	struct list_head *stop;
	struct list_head *current;
	
	if (list->ecode)
		return list->ecode;
	dsh = list->dsh;
	stop = &dsh->conns;
	ncpt_mutex_lock(&nds_ring_lock);
	conn = list->lastconn;
	if (conn) {
		list->lastconn = NULL;
		if (conn->state != list->conn_state) {
			ncp_conn_release(conn);
			goto restartLoop;
		}
		current = conn->nds_ring.next;
		ncp_conn_release(conn);
	} else {
restartLoop:;		
		current = stop->next;
	}
	if (current == stop) {
		list->ecode = ESRCH;
		ncpt_mutex_unlock(&nds_ring_lock);
		return ESRCH;
	}
	conn = list_entry(current, struct ncp_conn, nds_ring);
	if (conn->nds_conn != dsh) {
		goto restartLoop;
	}
	ncp_conn_store(conn);
	ncp_conn_use(conn);
	list->lastconn = conn;
	list->conn_state = conn->state;
	ncpt_mutex_unlock(&nds_ring_lock);
	*pconn = conn;
	return 0;
}

static NWDSCCODE __NWDSListConnectionEnd(struct NWDSConnList* list) {
	if (list->lastconn)
		ncp_conn_release(list->lastconn);
	list->ecode = EBADF;
	return 0;
}


static NWDSCCODE NWDXGetConnection(NWDS_HANDLE dsh, NWCONN_HANDLE* result) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	struct list_head* ptr;
	
	err = NWDXIsValid(dsh);
	if (err)
		return err;
	ncpt_mutex_lock(&nds_ring_lock);
	ptr = dsh->conns.next;
	if (list_empty(ptr)) {
		err = ERR_NO_CONNECTION;
	} else {
		conn = list_entry(ptr, struct ncp_conn, nds_ring);
		/* FIXME: mark connection as 'authentication disabled' ? */
		ncp_conn_use(conn);
		*result = conn;
	}
	ncpt_mutex_unlock(&nds_ring_lock);
	return err;
}

static NWDSCCODE NWDSSetLastConnection(NWDSContextHandle ctx, NWCONN_HANDLE conn) {
	NWCONN_HANDLE connold;
	
	connold = ctx->dck.last_connection.conn;
	if (conn != connold) {
		if (ctx->dck.flags & DCV_DISALLOW_REFERRALS)
			return NCPLIB_REFERRAL_NEEDED;
		if (conn) {
			ncp_conn_store(conn);
			ctx->dck.last_connection.conn = conn;
			ctx->dck.last_connection.state = conn->state;
		} else {
			ctx->dck.last_connection.conn = NULL;
		}
		if (connold)
			ncp_conn_release(connold);
	}
	return 0;
}

NWDSCCODE __NWDSGetConnection(NWDSContextHandle ctx, NWCONN_HANDLE* result) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	conn = ctx->dck.last_connection.conn;
	if (conn) {
		if (conn->state == ctx->dck.last_connection.state) {
			ncp_conn_use(conn);
			/* FIXME: authentication disabled */
			*result = conn;
			return 0;
		}
		ncp_conn_release(conn);
		ctx->dck.last_connection.conn = NULL;
	}
	err = NWDXGetConnection(ctx->ds_connection, &conn);
	if (err)
		return err;
	err = NWDSSetLastConnection(ctx, conn);
	if (err) {
		NWCCCloseConn(conn);
		return err;
	}
	*result = conn;
	return 0;
}

static inline NWDSCCODE NWDSConnectionFinished(UNUSED(NWDSContextHandle ctx), NWCONN_HANDLE conn) {
	NWCCCloseConn(conn);
	return 0;
}

static void NWDXAddContext(NWDS_HANDLE dsh, NWDSContextHandle ctx) {
	if (ctx->ds_connection)
		list_del(&ctx->context_ring);
	ctx->ds_connection = dsh;
	list_add(&ctx->context_ring, &dsh->contexts);
}

static NWDSCCODE __NWDSCreateDSConnection(NWDS_HANDLE *dsh) {
	NWDS_HANDLE tmp;
	
	tmp = (NWDS_HANDLE)malloc(sizeof(*tmp));
	if (!tmp) return ERR_NOT_ENOUGH_MEMORY;
	memset(tmp, 0, sizeof(*tmp));
	
	tmp->dck.tree_name = NULL;
	INIT_LIST_HEAD(&tmp->contexts);
	INIT_LIST_HEAD(&tmp->conns);
	*dsh = tmp;
	return 0;
}
	
static NWDSCCODE __NWDSReleaseDSConnection(NWDS_HANDLE dsh) {
	/* TODO: mutex */
	/* FIXME: ?? conns ?? */
	if (list_empty(&dsh->contexts) && list_empty(&dsh->conns)) {
		if (dsh->dck.tree_name)
			free(dsh->dck.tree_name);
		if (dsh->authinfo) {
			size_t tlen = dsh->authinfo->header.total;
			memset(dsh->authinfo, 0, tlen);
			munlock(dsh->authinfo, tlen);
			free(dsh->authinfo);
		}
		free(dsh);
	}
	return 0;
}

static NWDSCCODE NWDXSetTreeNameW(NWDS_HANDLE dsh, const wchar_t* treename) {
	size_t len;
	wchar_t* buf;
	NWDSCCODE err;
	
	if (!treename)
		return ERR_NULL_POINTER;
	err = NWDXIsValid(dsh);
	if (err)
		return err;
	len = wcslen(treename);
	if (len > MAX_TREE_NAME_CHARS)
		return NWE_BUFFER_OVERFLOW;
	buf = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
	if (!buf)
		return ERR_NOT_ENOUGH_MEMORY;
	memcpy(buf, treename, len * sizeof(wchar_t));
	buf[len] = 0;
	/* lock... */
	if (dsh->dck.tree_name)
		free(dsh->dck.tree_name);
	dsh->dck.tree_name = buf;
	/* unlock... */
	return 0;
}

NWDSCCODE NWDSSetTreeNameW(NWDSContextHandle ctx, const wchar_t* treename) {
	return NWDXSetTreeNameW(ctx->ds_connection, treename);
}

int iconv_external_to_wchar_t(const char* inp, wchar_t* outp, size_t maxl) {
	int i;
	size_t inl;
	
	inl = strlen(inp) + sizeof(*inp);
	i = iconv_utf8_to_wchar_t(&inp, &inl, (char**)(char*)&outp, &maxl);
	if (i < 0)
		return errno;
	return 0;
}

int iconv_wchar_t_to_external(const wchar_t* inp, char* outp, size_t maxl) {
	int i;
	size_t inl;
	
	inl = (wcslen(inp) + 1) * sizeof(*inp);
	i = iconv_wchar_t_to_utf8((const char**)(const char*)&inp, &inl, &outp, &maxl);
	if (i < 0)
		return errno;
	return 0;
}

static NWDSCCODE NWDXGetTreeNameFromCfg(NWDS_HANDLE dsh) {
	wchar_t* tmpstr;
	NWCONN_HANDLE conn;
	NWDSCCODE err;
	char* tmpchar;

	err = NWDXGetConnection(dsh, &conn);
	if (!err) {
		wchar_t tname[MAX_TREE_NAME_CHARS + 1];
		
		err = NWIsDSServerW(conn, tname);
		NWCCCloseConn(conn);
		if (err) {
			return NWDXSetTreeNameW(dsh, tname);
		}
	}
	tmpchar = getenv(PREFER_TREE_ENV);
	if (tmpchar) {
		wchar_t tname[MAX_TREE_NAME_CHARS + 1];

		if (!iconv_external_to_wchar_t(tmpchar, tname, sizeof(tname))) {
			return NWDXSetTreeNameW(dsh, tname);
		}
	}
	tmpstr = cfgGetItemW("Requester", "Default Tree Name");
	if (!tmpstr)
		return ERR_NO_CONNECTION;
	err = NWDXSetTreeNameW(dsh, tmpstr);
	free(tmpstr);
	return err;
}

static inline NWDSCCODE NWDXFetchTreeName(NWDS_HANDLE dsh) {
	if (!dsh->dck.tree_name)
		return NWDXGetTreeNameFromCfg(dsh);
	return 0;
}

/* Warning! namectx passed to this function must be allocated by malloc, and must not
   be deallocated by caller if function succeeds! */
static NWDSCCODE NWDSSetNameContextW(NWDSContextHandle ctx, wchar_t* namectx) {
	struct RDNInfo ctxRDN;
				
	if (!wcscasecmp(namectx, L"[Root]")) {
		ctxRDN.end = NULL;
		ctxRDN.depth = 0;
	} else {
		NWDSCCODE err;

		err = __NWDSCreateRDN(&ctxRDN, namectx, NULL);
		if (err) {
			return err;
		}
	}
	__NWDSDestroyRDN(&ctx->dck.rdn);
	if (ctx->dck.namectx)
		free(ctx->dck.namectx);
	memcpy(&ctx->dck.rdn, &ctxRDN, sizeof(ctxRDN));
	ctx->dck.namectx = namectx;
	return 0;
}

static NWDSCCODE NWDSGetNameContextFromCfg(NWDSContextHandle ctx) {
	NWDS_HANDLE dsh;
	NWDSCCODE dserr;
	char sect[20 + MAX_TREE_NAME_BYTES + 1];
	wchar_t* tmpstr;
	size_t l;
	
	dsh = ctx->ds_connection;
	dserr = NWDXFetchTreeName(dsh);
	if (dserr)
		return dserr;
	strcpy(sect, "Tree ");
	l = strlen(sect);
	if (!iconv_wchar_t_to_external(dsh->dck.tree_name, sect + l, sizeof(sect) - l)) {
		char* d;

		d = strchr(sect, 0);
		while (*--d == '_');
		d[1] = 0;
		tmpstr = cfgGetItemW(sect, "Default Name Context");
		if (tmpstr)
			goto have;
	}
	tmpstr = cfgGetItemW("Requester", "Default Name Context");
	if (tmpstr) {
		goto have;
	}
	tmpstr = wcsdup(L"[Root]");
	if (tmpstr) {
		goto have;
	}
	return ERR_NOT_ENOUGH_MEMORY;
have:;	
	dserr = NWDSSetNameContextW(ctx, tmpstr);
	if (dserr)
		free(tmpstr);
	return dserr;
}

static inline NWDSCCODE NWDSFetchNameContext(NWDSContextHandle ctx) {
	if (!ctx->dck.namectx)
		return NWDSGetNameContextFromCfg(ctx);
	return 0;
}

/* NWDSSetContext supports only 2 transports */
NWDSCCODE NWDSSetTransport(NWDSContextHandle ctx, size_t len, const NET_ADDRESS_TYPE* transports) {
	NWDSCCODE err;
	nuint32* ptr;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	if (len > 20) /* some reasonable limit */
		return NWE_PARAM_INVALID;
	if (len) {
		size_t cnt;
		nuint32* ptr2;
		
		ptr2 = ptr = (nuint32*)malloc(len * sizeof(nuint32));
		if (!ptr)
			return ERR_NOT_ENOUGH_MEMORY;
		for (cnt = len; cnt; cnt--)
			DSET_LH(ptr2++, 0, *transports++);
	} else {
		ptr = NULL;
	}
	if (ctx->dck.transport_types)
		free(ctx->dck.transport_types);
	ctx->dck.transport_types = ptr;
	ctx->dck.transports = len;
	return 0;
}

NWDSCCODE NWDSCreateContextHandle(NWDSContextHandle *ctx) {
	NWDSContextHandle tmp;
	NWDSCCODE err;
	NWDS_HANDLE dsh;
	char* tmpstr;

	/* everyone must call NWDSCreateContextHandle before another
	   reasonable NWDS function, so do initialization here... */
	NWDSInitRequester();
	err = __NWDSCreateDSConnection(&dsh);
	if (err)
		return err;	
	tmp = (NWDSContextHandle)malloc(sizeof(*tmp));
	if (!tmp) {
		__NWDSReleaseDSConnection(dsh);
		return ERR_NOT_ENOUGH_MEMORY;
	}
	memset(tmp, 0, sizeof(*tmp));
	INIT_LIST_HEAD(&tmp->context_ring);
	tmp->dck.flags = DCV_DEREF_ALIASES | DCV_XLATE_STRINGS | DCV_CANONICALIZE_NAMES;
	tmp->dck.name_form = 0;
	tmp->dck.last_connection.conn = NULL;
	tmp->dck.local_charset = NULL;
	tmp->dck.confidence = DCV_LOW_CONF;
	tmp->dck.dsi_flags = DSI_ENTRY_FLAGS | DSI_OUTPUT_FIELDS | 
		DSI_SUBORDINATE_COUNT | DSI_MODIFICATION_TIME | 
		DSI_BASE_CLASS | DSI_ENTRY_RDN | DSI_ENTRY_DN;
	tmp->xlate.from = (my_iconv_t)-1;
	tmp->xlate.to = (my_iconv_t)-1;
	ncpt_mutex_init(&tmp->xlate.fromlock);
	ncpt_mutex_init(&tmp->xlate.tolock);
	NWDXAddContext(dsh, tmp);
	tmpstr = cfgGetItem("Requester", "Local Charset");
	if (tmpstr) {
		if (verifyCharset(tmpstr)) {
			free(tmpstr);
			tmpstr = NULL;
		}
	}
	err = NWDSSetContext(tmp, DCK_LOCAL_CHARSET, tmpstr ? tmpstr : default_encoding);
	if (tmpstr)
		free(tmpstr);
	if (err) {
		NWDSFreeContext(tmp);
		return err;
	}
	/* set to something reasonable */
	tmp->dck.rdn.end = NULL;
	tmp->dck.rdn.depth = 0;
	tmp->dck.namectx = NULL;
	{
		static const nuint32 t[] = {
#ifdef NCP_IPX_SUPPORT
			NT_IPX, 
#endif
#ifdef NCP_IN_SUPPORT
			NT_TCP,
			NT_UDP,
#endif
		};

		err = NWDSSetTransport(tmp, sizeof(t)/sizeof(t[0]), t);
	}
	if (err) {
		NWDSFreeContext(tmp);
		return err;
	}
	*ctx = tmp;
	return 0;
}

NWDSCCODE NWDSDuplicateContextHandleInt(NWDSContextHandle srcctx,
		NWDSContextHandle *ctx) {
	NWDSContextHandle tmp;
	NWDSCCODE err;
	
	if (!srcctx)
		return ERR_NULL_POINTER;
	tmp = (NWDSContextHandle)malloc(sizeof(*tmp));
	if (!tmp)
		return ERR_NOT_ENOUGH_MEMORY;

	memset(tmp, 0, sizeof(*tmp));
	INIT_LIST_HEAD(&tmp->context_ring);
	/* return typed absolute names */
	tmp->dck.flags = srcctx->dck.flags & ~(DCV_CANONICALIZE_NAMES | DCV_TYPELESS_NAMES);
	tmp->dck.name_form = srcctx->dck.name_form;
	tmp->dck.last_connection.conn = srcctx->dck.last_connection.conn;
	tmp->dck.last_connection.state = srcctx->dck.last_connection.state;
	if (tmp->dck.last_connection.conn) {
		ncp_conn_store(tmp->dck.last_connection.conn);
	}
	tmp->dck.local_charset = NULL;
	tmp->dck.confidence = srcctx->dck.confidence;
	tmp->dck.dsi_flags = srcctx->dck.dsi_flags;
	tmp->xlate.from = (my_iconv_t)-1;
	tmp->xlate.to = (my_iconv_t)-1;
	ncpt_mutex_init(&tmp->xlate.fromlock);
	ncpt_mutex_init(&tmp->xlate.tolock);
	/* tree_list is not duplicated ! */
	NWDXAddContext(srcctx->ds_connection, tmp);
	/* select wchar_t encoding for NWDS* functions */
	err = NWDSSetContext(tmp, DCK_LOCAL_CHARSET, NULL);
	if (err) {
		NWDSFreeContext(tmp);
		return err;
	}
	if (srcctx->dck.namectx) {
		err = NWDSSetContext(tmp, DCK_NAME_CONTEXT, srcctx->dck.namectx);
		if (err) {
			NWDSFreeContext(tmp);
			return err;
		}
	} else {
		tmp->dck.rdn.end = NULL;
		tmp->dck.rdn.depth = 0;
		tmp->dck.namectx = NULL;
	}
	/* FIXME! Maybe other fields... */
	{
		size_t tlen = srcctx->dck.transports * sizeof(nuint32);
		void* tt = malloc(tlen);
		if (tt) {
			tmp->dck.transport_types = tt;
			tmp->dck.transports = srcctx->dck.transports;
			memcpy(tt, srcctx->dck.transport_types, tlen);
		} else
			err = ERR_NOT_ENOUGH_MEMORY;
	}
	if (err) {
		NWDSFreeContext(tmp);
		return err;
	}
	*ctx = tmp;
	return 0;
}

NWDSCCODE NWDSDuplicateContextHandle(NWDSContextHandle srcctx,
		NWDSContextHandle *ctx) {
	NWDSContextHandle tmp;
	NWDSCCODE err;

	err = NWDSDuplicateContextHandleInt(srcctx, &tmp);	
	if (err)
		return err;

	err = NWDSSetContext(tmp, DCK_LOCAL_CHARSET, srcctx->dck.local_charset);
	if (err) {
		NWDSFreeContext(tmp);
		return err;
	}
	/* DCK_LOCAL_CHARSET modifies dck.flags */
	tmp->dck.flags = srcctx->dck.flags;
	*ctx = tmp;
	return 0;
}

static void __freeTree(struct TreeList*);

static inline void __NWDSDestroyTreeList(struct TreeList** t) {
	if (*t) {
		__freeTree(*t);
		*t = NULL;
	}
}

/* must be able to free partially allocated context handle */
NWDSCCODE NWDSFreeContext(NWDSContextHandle ctx) {
	if (!ctx) {
		return 0;
	}
	if (ctx->ds_connection) {
		list_del(&ctx->context_ring);
		__NWDSReleaseDSConnection(ctx->ds_connection);
	}
	if (ctx->xlate.from != (my_iconv_t)-1)
		my_iconv_close(ctx->xlate.from);
	if (ctx->xlate.to != (my_iconv_t)-1)
		my_iconv_close(ctx->xlate.to);
	ncpt_mutex_destroy(&ctx->xlate.fromlock);
	ncpt_mutex_destroy(&ctx->xlate.tolock);
	__NWDSDestroyTreeList(&ctx->dck.tree_list);
	if (ctx->dck.local_charset)
		free(ctx->dck.local_charset);
	if (ctx->dck.transport_types)
		free(ctx->dck.transport_types);
	__NWDSDestroyRDN(&ctx->dck.rdn);
	if (ctx->dck.namectx)
		free(ctx->dck.namectx);
	if (ctx->dck.last_connection.conn)
		ncp_conn_release(ctx->dck.last_connection.conn);
	free(ctx);
	return 0;
}

NWDSContextHandle NWDSCreateContext(void) {
	NWDSContextHandle ctx;
	NWDSCCODE err;
	
	err = NWDSCreateContextHandle(&ctx);
	if (err)
		return (NWDSContextHandle)ERR_CONTEXT_CREATION;
	return ctx;
}

NWDSContextHandle NWDSDuplicateContext(NWDSContextHandle ctxin) {
	NWDSContextHandle ctx;
	NWDSCCODE err;
	
	err = NWDSDuplicateContextHandle(ctxin, &ctx);
	if (err)
		return (NWDSContextHandle)ERR_CONTEXT_CREATION;
	return ctx;
}

NWDSCCODE NWDSSetContext(NWDSContextHandle ctx, int key, const void* ptr) {
	NWDSCCODE err;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	switch (key) {
		case DCK_FLAGS:
			{
				ctx->dck.flags = *(const nuint32*)ptr;
			}
			return 0;
		case DCK_CONFIDENCE:
			{
				/* confidence can be any value... */
				ctx->dck.confidence = *(const nuint32*)ptr;
			}
			return 0;
		case DCK_NAME_CONTEXT:
			{
				wchar_t *namectx;
				
				namectx = (wchar_t*)malloc(sizeof(wchar_t) * 1024);
				if (!namectx)
					return ERR_NOT_ENOUGH_MEMORY;
				err = NWDSXlateFromCtx(ctx, namectx, sizeof(wchar_t) * 1024, ptr);
				if (err)
					return err;
				err = NWDSSetNameContextW(ctx, namectx);
				if (err)
					free(namectx);
				return err;
			}
		case DCK_DSI_FLAGS:
			{
				ctx->dck.dsi_flags = *(const nuint32*)ptr | DSI_OUTPUT_FIELDS;
			}
			return 0;
		case DCK_NAME_FORM:
			{
				nuint32 v = *(const nuint32*)ptr;
				nuint32 x;
				
				if (v == DCV_NF_PARTIAL_DOT)
					x = 0;
				else if (v == DCV_NF_FULL_DOT)
					x = 4;
				else if (v == DCV_NF_SLASH)
					x = 2;
				else
					x = 0;
	/* 0: CN=TEST.OU=VC.O=CVUT.C=CZ (NF_PARTIAL_DOT)
	 * 1: TEST.VC.CVUT.CZ
	 * 2: \T=CVUT\C=CZ\O=CVUT\OU=VC\CN=TEST (NF_SLASH)
	 * 3: \CVUT\CZ\CVUT\VC\TEST
	 * 4: CN=TEST.OU=VC.O=CVUT.C=CZ.T=CVUT. (NF_FULL_DOT)
	 * 5: TEST.VC.CVUT.CZ.CVUT.
	 * 6 & 7 == 4 & 5
	 * 8..9:  Structured (1), unsupported (?)
	 * 10..15: Mirror of 8..9
	 * 16..17: Structured (2), unsupported (?)
	 * 18..31: Mirror of 16..17
	 * other:  Only low 5 bits are significant on NW5 (DS 7.28)
	 * Structured (1) and (2) are not supported ATM TODO */
				ctx->dck.name_form = x;
			}
			return 0;
		case DCK_LOCAL_CHARSET:
			{
				my_iconv_t f;
				my_iconv_t t;
				const char* name = ptr;
				
				if (!name)
					name = wchar_encoding;

				if (ctx->dck.local_charset && !strcmp(name, ctx->dck.local_charset))
					return 0;
				f = my_iconv_open(wchar_encoding, name);
				if (f == (my_iconv_t)-1)
					return ERR_UNICODE_FILE_NOT_FOUND;
				t = my_iconv_open(name, wchar_encoding);
				if (t == (my_iconv_t)-1) {
					my_iconv_close(f);
					return ERR_UNICODE_FILE_NOT_FOUND;
				}
				if (ctx->xlate.from != (my_iconv_t)-1)
					my_iconv_close(ctx->xlate.from);
				ctx->xlate.from = f;
				if (ctx->xlate.to != (my_iconv_t)-1)
					my_iconv_close(ctx->xlate.to);
				ctx->xlate.to = t;
				if (ctx->dck.local_charset)
					free(ctx->dck.local_charset);
				ctx->dck.local_charset = strdup(name);
				ctx->dck.flags |= DCV_XLATE_STRINGS;
				return 0;
			}
	}
	return ERR_BAD_KEY;
}

NWDSCCODE NWDSGetContext2(NWDSContextHandle ctx, int key, void* ptr, size_t maxlen) {
	NWDSCCODE err;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	switch (key) {
		case DCK_FLAGS:
			if (maxlen < sizeof(u_int32_t))
				return NWE_BUFFER_OVERFLOW;
			*(u_int32_t*)ptr = ctx->dck.flags;
			return 0;
		case DCK_CONFIDENCE:
			if (maxlen < sizeof(u_int32_t))
				return NWE_BUFFER_OVERFLOW;
			*(u_int32_t*)ptr = ctx->dck.confidence;
			return 0;
		case DCK_NAME_CONTEXT:
			err = NWDSFetchNameContext(ctx);
			if (err)
				return err;
			err = NWDSXlateToCtx(ctx, ptr, maxlen, ctx->dck.namectx, NULL);
			return err;
		case DCK_RDN:
			if (maxlen < sizeof(ctx->dck.rdn))
				return NWE_BUFFER_OVERFLOW;
			err = NWDSFetchNameContext(ctx);
			if (err)
				return err;
			memcpy(ptr, &ctx->dck.rdn, sizeof(ctx->dck.rdn));
			return 0;
		case DCK_LAST_CONNECTION:
			if (maxlen < sizeof(NWCONN_HANDLE))
				return NWE_BUFFER_OVERFLOW;
			*(NWCONN_HANDLE*)ptr = ctx->dck.last_connection.conn;
			return 0;
		case DCK_TREE_NAME:
			{
				NWDS_HANDLE dsh = ctx->ds_connection;
				const wchar_t* src;
			
				err = NWDXFetchTreeName(dsh);
				if (err)
					return err;
				src = dsh->dck.tree_name;
				if (!src)
					return ERR_NO_CONNECTION;
				return NWDSXlateToCtx(ctx, ptr, maxlen, src, NULL);
			}
		case DCK_DSI_FLAGS:
			if (maxlen < sizeof(nuint32))
				return NWE_BUFFER_OVERFLOW;
			*(u_int32_t*)ptr = ctx->dck.dsi_flags;
			return 0;
		case DCK_NAME_FORM:
			if (maxlen < sizeof(nuint32))
				return NWE_BUFFER_OVERFLOW;
			{
				nuint32* nf = ptr;

				if (ctx->dck.name_form == 4)
					*nf = DCV_NF_FULL_DOT;
				else if (ctx->dck.name_form == 2)
					*nf = DCV_NF_SLASH;
				else
					*nf = DCV_NF_PARTIAL_DOT;
			}
			return 0;
		case DCK_TREELIST:
			if (maxlen < sizeof(struct TreeList*))
				return NWE_BUFFER_OVERFLOW;
			*(struct TreeList**)ptr = ctx->dck.tree_list;
			return 0;
	}
	return ERR_BAD_KEY;
}
	
NWDSCCODE NWDSGetContext(NWDSContextHandle ctx, int key, void* ptr) {
	static size_t optlen[] =
		{		      0, 
		      sizeof(u_int32_t),	/* 1, DCK_FLAGS */
		      sizeof(u_int32_t),	/* 2, DCK_CONFIDENCE */      
		      	   MAX_DN_BYTES,	/* 3, DCK_NAME_CONTEXT */
				      0,	/* 4, N/A */
				      0,	/* 5, N/A */
		 sizeof(struct RDNInfo),	/* 6, DCK_RDN */		 
		 		      0,	/* 7, N/A */
		  sizeof(NWCONN_HANDLE),	/* 8, DCK_LAST_CONNECTION */
		  		      0,	/* 9, N/A */
		  		      0,	/* 10, N/A */
	  4 * (MAX_TREE_NAME_CHARS + 1),	/* 11, DCK_TREE_NAME */
		      sizeof(u_int32_t),	/* 12, DCK_DSI_FLAGS */
		      sizeof(u_int32_t),	/* 13, DCK_NAME_FORM */
				      0,	/* 14, N/A */
				      0,	/* 15, N/A */
				      0,	/* 16, N/A */
	       sizeof(struct TreeList*),	/* 17, DCK_TREELIST */
	       		};

	if ((key >= DCK_FLAGS) && (key <= DCK_TREELIST))
		return NWDSGetContext2(ctx, key, ptr, optlen[key]);
	else
		return NWDSGetContext2(ctx, key, ptr, ~0);	/* unlimited buffer if size not known */
	
}

void NWDSSetupBuf(Buf_T* buf, void* ptr, size_t len) {
	buf->bufFlags = 0;
	buf->cmdFlags = 0;
	buf->dsiFlags = 0;
	buf->data = buf->curPos = ptr;
	buf->dataend = buf->allocend = (char*)ptr + len;
	buf->operation = 0;
	buf->attrCountPtr = NULL;
	buf->valCountPtr = NULL;
}

NWDSCCODE NWDSCreateBuf(Buf_T** buff, void* ptr, size_t len) {
	Buf_T* buf;
	
	*buff = NULL;
	buf = (Buf_T*)malloc(sizeof(*buf));
	if (!buf)
		return ERR_NOT_ENOUGH_MEMORY;
	NWDSSetupBuf(buf, ptr, len);
	*buff = buf;
	return 0;
}

NWDSCCODE NWDSAllocBuf(size_t len, pBuf_T* buff) {
	NWDSCCODE err;
	nuint8* buffer;
	Buf_T* buf;
	
	*buff = NULL;
	len = ROUNDBUFF(len);
	buffer = (nuint8*)malloc(len);
	if (!buffer)
		return ERR_NOT_ENOUGH_MEMORY;
	/* if (buffer & 3) { panic("Sorry, malloc must return dword aligned value"}; */
	err = NWDSCreateBuf(&buf, buffer, len);
	if (err)
		free(buffer);
	else {
		buf->bufFlags |= NWDSBUFT_ALLOCATED;
		*buff = buf;
	}
	return err;
}

NWDSCCODE NWDSFreeBuf(Buf_T* buf) {
	if (buf) {
		if (buf->bufFlags & NWDSBUFT_ALLOCATED) {
			free(buf->data);
			buf->data = NULL;
		}
		free(buf);
	}
	return 0;	/* easy... */
}

NWDSCCODE NWDSClearFreeBuf(Buf_T* buf) {
	if (buf && buf->data) {
		memset(buf->data, 0, buf->allocend - buf->data);
	}
	return NWDSFreeBuf(buf);
}

#define NWDSFINDCONN_NOCREATE		0x0000
#define NWDSFINDCONN_CREATEALLOWED	0x0001
#define NWDSFINDCONN_DSREADBUF		0x0002
static NWDSCCODE NWDXFindConnection(NWDS_HANDLE dsh, NWCONN_HANDLE* pconn, u_int32_t saddr, Buf_T* addr, int flags) {
	struct list_head* stop = &dsh->conns;
	u_int32_t saddr2;
	void* p;
	NWDSCCODE err = 0;
	
	saddr2 = saddr;
	p = NWDSBufPeekPtr(addr);
	while (saddr--) {
		struct list_head* current;
		nuint32 addrtype;
		nuint32 len;
		void* data;
		
		if (flags & NWDSFINDCONN_DSREADBUF) {
			nuint32 tlen;

			err = NWDSBufGetLE32(addr, &tlen);
			if (err)
				break;
			if (tlen < 8) {
				err = ERR_INVALID_SERVER_RESPONSE;
				break;
			}
			err = NWDSBufGetLE32(addr, &addrtype);
			if (err)
				break;
			err = NWDSBufGetLE32(addr, &len);
			if (err)
				break;
			if (len > tlen - 8) {
				err = ERR_INVALID_SERVER_RESPONSE;
				break;
			}
			data = NWDSBufGetPtr(addr, tlen - 8);
		} else {
			err = NWDSBufGetLE32(addr, &addrtype);
			if (err)
				break;
			err = NWDSBufGetLE32(addr, &len);
			if (err)
				break;
			data = NWDSBufGetPtr(addr, len);
		}
		if (!data) {
			err = ERR_BUFFER_EMPTY;
			break;
		}
		ncpt_mutex_lock(&nds_ring_lock);
restartLoop:;
		for (current = stop->next; current != stop; current = current->next) {
			NWCONN_HANDLE conn = list_entry(current, struct ncp_conn, nds_ring);
			NWObjectCount connaddresses;
			nuint8* conndata;
			NWCCODE err2;
			/* compare addresses */

			ncpt_mutex_unlock(&nds_ring_lock);
			err2 = __NWCCGetServerAddressPtr(conn, &connaddresses, &conndata);
			ncpt_mutex_lock(&nds_ring_lock);
			if (conn->nds_conn != dsh) {
				/* Someone removed connection from the ring under us! 
				 * Restart from the beginning... */
				goto restartLoop;
			}
			if (err2)
				continue;
			while (connaddresses--) {
				if ((DVAL_LH(conndata, 4) == addrtype) &&
				    (DVAL_LH(conndata, 8) == len) &&
				    !memcmp(conndata + 12, data, len)) {
					ncp_conn_use(conn);
					ncpt_mutex_unlock(&nds_ring_lock);
					*pconn = conn;
					return 0;
				}
				conndata += 4 + ROUNDPKT(DVAL_LH(conndata, 0));
			}
			
		}
		ncpt_mutex_unlock(&nds_ring_lock);
	}
	if (flags & NWDSFINDCONN_CREATEALLOWED) {
		saddr = saddr2;
		NWDSBufSeek(addr, p);
		while (saddr--) {
			nuint32 addrtype;
			nuint32 len;
			void* data;
			NWCCTranAddr tran;
		
			if (flags & NWDSFINDCONN_DSREADBUF) {
				nuint32 tlen;

				err = NWDSBufGetLE32(addr, &tlen);
				if (err)
					break;
				if (tlen < 8) {
					err = ERR_INVALID_SERVER_RESPONSE;
					break;
				}
				err = NWDSBufGetLE32(addr, &addrtype);
				if (err)
					break;
				err = NWDSBufGetLE32(addr, &len);
				if (err)
					break;
				if (len > tlen - 8) {
					err = ERR_INVALID_SERVER_RESPONSE;
					break;
				}
				data = NWDSBufGetPtr(addr, tlen - 8);
			} else {
				err = NWDSBufGetLE32(addr, &addrtype);
				if (err)
					break;
				err = NWDSBufGetLE32(addr, &len);
				if (err)
					break;
				data = NWDSBufGetPtr(addr, len);
			}
			if (!data) {
				err = ERR_BUFFER_EMPTY;
				break;
			}
			tran.type = addrtype;
			tran.buffer = data;
			tran.len = len;
			err = NWCCOpenConnByAddr(&tran, 0, NWCC_RESERVED, pconn);
			if (!err)
				return 0;
		}
	}
	if (err)
		return err;
	return ERR_NO_CONNECTION;	
}

static NWDSCCODE NWDSTryAuthenticateConn(NWDSContextHandle ctx, NWCONN_HANDLE conn) {
	/* FIXME: Check for ctx->ds_connection->authinfo... */
	return NWDSAuthenticateConn(ctx, conn);
}

static NWDSCCODE NWDSListConnectionInit(NWDSContextHandle ctx, struct NWDSConnList** lst) {
	NWDSCCODE err;
	struct NWDSConnList *tmp;
	
	tmp = (struct NWDSConnList*)malloc(sizeof(*tmp));
	if (!tmp) {
		*lst = NULL;
		return ENOMEM;
	}
	err = __NWDSListConnectionInit(ctx->ds_connection, tmp);
	if (err) {
		free(tmp);
	} else {
		*lst = tmp;
	}
	return err;
}

static NWDSCCODE NWDSListConnectionNext(NWDSContextHandle ctx,
		struct NWDSConnList* list,
		NWCONN_HANDLE* pconn) {
	NWCONN_HANDLE conn;
	NWDSCCODE err;
	
	if (ctx->ds_connection != list->dsh)
		return EINVAL;
	err = __NWDSListConnectionNext(list, &conn);
	if (err)
		return err;
	/* catch disallow referrals */
	err = NWDSSetLastConnection(ctx, conn);
	if (err) {
		list->ecode = err;
		return err;
	}
	*pconn = conn;
	return 0;
}

static NWDSCCODE NWDSListConnectionEnd(NWDSContextHandle ctx,
		struct NWDSConnList* list) {
	NWDSCCODE err;
	
	err = __NWDSListConnectionEnd(list);
	free(list);
	return err;
	(void)ctx;
}

static NWDSCCODE NWDSFindConnection(NWDSContextHandle ctx, NWCONN_HANDLE* pconn, u_int32_t cnt, Buf_T* addr, int flags) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
		
	err = NWDXFindConnection(ctx->ds_connection, &conn, cnt, addr, flags);
	if (err)
		return err;
	if (!(conn->connState & CONNECTION_AUTHENTICATED) && 
	    !(ctx->priv_flags & DCV_PRIV_AUTHENTICATING))
		NWDSTryAuthenticateConn(ctx, conn);
	err = NWDSSetLastConnection(ctx, conn);
	if (err) {
		NWCCCloseConn(conn);
		return err;
	}
	*pconn = conn;
	return 0;
}

static NWDSCCODE NWDSFindConnection2(NWDSContextHandle ctx, NWCONN_HANDLE* pconn, Buf_T* addr, int flags) {
	u_int32_t cnt;
	NWDSCCODE err;
	
	err = NWDSBufGetLE32(addr, &cnt);
	if (!err)
		err = NWDSFindConnection(ctx, pconn, cnt, addr, flags);
	return err;
}

static inline NWDSCCODE NWDSGetDSIRaw(NWCONN_HANDLE conn, nuint32 flags, 
		nuint32 DNstyle, NWObjectID id, Buf_T* reply) {
	unsigned char x[16];
	unsigned char rpl[4096];
	size_t rpl_len;
	NWDSCCODE err;
	
	DSET_LH(x, 0, 2);	/* version */
	DSET_LH(x, 4, DNstyle);
	DSET_LH(x, 8, flags);
	DSET_HL(x, 12, id);
	err = ncp_send_nds_frag(conn, DSV_READ_ENTRY_INFO, x, 16, rpl,
		sizeof(rpl), &rpl_len);
	if (!err) {
		dfprintf(stderr, "Reply len: %u\n", rpl_len);
		NWDSBufStartPut(reply, DSV_READ_ENTRY_INFO);
		NWDSBufSetDSIFlags(reply, flags);
		err = NWDSBufPut(reply, rpl, rpl_len);
		NWDSBufFinishPut(reply);
	}
	return err;
}

static NWDSCCODE NWDSXlateUniToCtx(NWDSContextHandle ctx, void* data,
		size_t *maxlen, const unicode* src, size_t ln) {
	NWDSCCODE err;
	nuint32 val;
	const unicode* srcEnd = src + ln / sizeof(*src);
	char* dst = data;
	char* dstEnd = dst + *maxlen;

	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_XLATE_STRINGS) {
		NWDSCCODE err2;

		ncpt_mutex_lock(&ctx->xlate.tolock);
		__NWULocalInit(ctx->xlate.to);
		do {
			wchar_t tbuff[128];
			wchar_t* tEnd;

			err2 = __NWUUnicodeToInternal(tbuff, tbuff+128, src, srcEnd, NULL, &tEnd, &src);
			if (data)
				err = __NWUInternalToLocal(ctx->xlate.to, dst, dstEnd, tbuff, tEnd, NULL, &dst, NULL);
			else {
				char tmpChar[1024];
				char* tChar;

				err = __NWUInternalToLocal(ctx->xlate.to, tmpChar, tmpChar+1024, tbuff, tEnd, NULL, &tChar, NULL);
				dst += tChar - tmpChar;
			}
		} while ((!err) && err2 == E2BIG);
		ncpt_mutex_unlock(&ctx->xlate.tolock);
		if (err)
			return err;
		if (err2)
			return err2;
	} else {
		size_t slen = (srcEnd - src) * sizeof(*src);
		if (data) {
			if ((size_t)(dstEnd - dst) < slen)
				return E2BIG;
			memcpy(dst, src, slen);
		}
		dst += slen;
	}
	*maxlen = dst - (char*)data;
	return 0;
}

static NWDSCCODE NWDSXlateCtxToUni(NWDSContextHandle ctx, unicode* data,
		size_t *maxlen, const void* src, size_t ln) {
	NWDSCCODE err;
	nuint32 val;
	unicode* dst = data;
	unicode* dstEnd = dst + *maxlen / sizeof(unicode);

	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_XLATE_STRINGS) {
		NWDSCCODE err2;
		const nuint8* srcEnd;

		ncpt_mutex_lock(&ctx->xlate.fromlock);
		__NWULocalInit(ctx->xlate.from);
		if (!ln) {
			if (my_iconv_is_wchar(ctx->xlate.from))
				ln = (wcslen((const wchar_t*)src) + 1) * sizeof(wchar_t);
			else
				ln = strlen(src) + 1;
		}
		srcEnd = (const nuint8*)src + ln;
		do {
			wchar_t tbuff[128];
			wchar_t* tEnd;

			err2 = __NWULocalToInternal(ctx->xlate.from, tbuff, tbuff+128, src, srcEnd, NULL, &tEnd, (const char**)(const char*)&src);
			err = __NWUInternalToUnicode(dst, dstEnd, tbuff, tEnd, NULL, &dst, NULL);
		} while ((!err) && err2 == E2BIG);
		ncpt_mutex_unlock(&ctx->xlate.fromlock);
		if (err)
			return err;
		if (err2)
			return err2;
	} else {
		if (!ln)
			ln = (unilen(src) + 1) * sizeof(unicode);
		if (*maxlen < ln)
			return E2BIG;
		memcpy(dst, src, ln);
		dst += ln / sizeof(unicode);
	}
	*maxlen = (dst - data) * sizeof(unicode);
	return 0;
}

NWDSCCODE NWDSXlateToCtx(NWDSContextHandle ctx, void* data, 
		size_t maxlen, const wchar_t* src, size_t* ln) {
	NWDSCCODE err;
	nuint32 val;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_XLATE_STRINGS) {
		char* ep;
		ncpt_mutex_lock(&ctx->xlate.tolock);
		__NWULocalInit(ctx->xlate.to);
		err = __NWUInternalToLocal(ctx->xlate.to, data, (char*)data+maxlen, 
				src, NULL, NULL, &ep, NULL);
		ncpt_mutex_unlock(&ctx->xlate.tolock);
		if (err)
			return ERR_DN_TOO_LONG; /* or INVALID MULTIBYTE */
		if (ln)
			*ln = ep - (char*)data;
	} else {
		unicode* ep;
		err = __NWUInternalToUnicode(data, (unicode*)data + maxlen / sizeof(unicode), 
				src, NULL, NULL, &ep, NULL);
		if (ln)
			*ln = (ep - (unicode*)data) * sizeof(*ep);
	}
	return err;
}

NWDSCCODE NWDSXlateFromCtx(NWDSContextHandle ctx, wchar_t* dst,
		size_t maxlen, const void* src) {
	NWDSCCODE err;
	nuint32 val;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_XLATE_STRINGS) {
		ncpt_mutex_lock(&ctx->xlate.fromlock);
		__NWULocalInit(ctx->xlate.from);
		err = __NWULocalToInternal(ctx->xlate.from, dst, dst + maxlen / sizeof(*dst), 
				src, NULL, NULL, NULL, NULL);
		ncpt_mutex_unlock(&ctx->xlate.fromlock);
		if (err)
			return ERR_DN_TOO_LONG;	/* or INVALID MULTIBYTE */
	} else {
		err = __NWUUnicodeToInternal(dst, dst + maxlen / sizeof(*dst), 
				src, NULL, NULL, NULL, NULL);
	}
	return err;
}

static NWDSCCODE NWDSPtrCtxString(NWDSContextHandle ctx,
		const unicode* ptr, size_t len, 
		void* chrs, size_t maxlen, size_t* realLen) {
	NWDSCCODE err;
	static const unicode uz = 0;

	if (!ptr)
		return ERR_BUFFER_EMPTY;
	if (len & 1)
		return ERR_INVALID_OBJECT_NAME;
	if (!len) {
		ptr = &uz;
		len = sizeof(uz);
	}
	if (ptr[(len >> 1) - 1])
		return ERR_INVALID_OBJECT_NAME;
	err = NWDSXlateUniToCtx(ctx, chrs, &maxlen, ptr, len);
	if (realLen)
		*realLen = maxlen;
	return err;
}

static NWDSCCODE NWDSBufUnicodeString(Buf_T* buffer,
		const unicode** unistr, size_t* len) {
	NWDSCCODE err;
	nuint32 len32;
	const unicode* ptr;
	static const unicode uz = 0;

	err = NWDSBufGetLE32(buffer, &len32);
	if (err)
		return err;
	ptr = NWDSBufGetPtr(buffer, len32);
	if (!ptr)
		return ERR_BUFFER_EMPTY;
	if (len32 & 1)
		return ERR_INVALID_OBJECT_NAME;
	if (!len32) {
		ptr = &uz;
		len32 = sizeof(uz);
	}
	if (ptr[(len32 >> 1) - 1])
		return ERR_INVALID_OBJECT_NAME;
	if (unistr)
		*unistr = ptr;
	if (len)
		*len = len32;
	return 0;
}

NWDSCCODE NWDSBufCtxString(NWDSContextHandle ctx, Buf_T* buffer, 
		void* chrs, size_t maxlen, size_t *realLen) {
	NWDSCCODE err;
	nuint32 len32;
	const unicode* ptr;
	
	err = NWDSBufGetLE32(buffer, &len32);
	if (err)
		return err;
	ptr = NWDSBufGetPtr(buffer, len32);
	return NWDSPtrCtxString(ctx, ptr, len32, chrs, maxlen, realLen);
}

static NWDSCCODE NWDSCtxPtrString(NWDSContextHandle ctx, void* ptr, size_t maxlen,
		const NWDSChar* string, size_t* realLen) {
	NWDSCCODE err;
	
	err = NWDSXlateCtxToUni(ctx, ptr, &maxlen, string, 0);
	if (realLen)
		*realLen = maxlen;
	return err;
}
	
NWDSCCODE NWDSCtxBufString(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* string) {
	NWDSCCODE err;
	nuint8* p;
	nuint8* strdata;
	size_t len;
	
	p = NWDSBufPutPtr(buffer, 4);
	if (!p)
		return ERR_BUFFER_FULL;
	strdata = NWDSBufPutPtrLen(buffer, &len);
	if (!strdata)
		return ERR_BUFFER_FULL;
	if (string) {
		err = NWDSCtxPtrString(ctx, strdata, len, string, &len);
		if (err)
			return err;
	}
	DSET_LH(p, 0, len);
	NWDSBufPutSkip(buffer, len);
	return 0;
}

NWDSCCODE NWDSPtrDN(const unicode* ptr, size_t len, 
		wchar_t* data, size_t maxlen) {
	NWDSCCODE err;

	if (!ptr)
		return ERR_BUFFER_EMPTY;
	if (len & 1)
		return ERR_INVALID_OBJECT_NAME;	
	if (!len) {
		if (data)
			*data = 0;
		return 0;
	}
	if (ptr[(len >> 1)-1])
		return ERR_INVALID_OBJECT_NAME;
	if (data) {
		err = __NWUUnicodeToInternal(data, data + maxlen / sizeof(*data), 
				ptr, ptr + len / sizeof(*ptr), NULL, NULL, NULL);
		if (err)
			return ERR_DN_TOO_LONG;
	}
	return 0;
}

NWDSCCODE NWDSBufDN(Buf_T* buffer, wchar_t* data, size_t maxlen) {
	NWDSCCODE err;
	nuint32 len32;
	const unicode* ptr;
	
	err = NWDSBufGetLE32(buffer, &len32);
	if (err)
		return err;
	ptr = NWDSBufGetPtr(buffer, len32);
	return NWDSPtrDN(ptr, len32, data, maxlen);
}

static inline NWDSCCODE NWDSBufSkipCIString(Buf_T* buffer) {
	return NWDSBufDN(buffer, NULL, 0);
}

static NWDSCCODE NWDSPtrCtxDN(NWDSContextHandle ctx,
		const unicode* p, size_t len,
		void* ret, size_t* ln) {
	NWDSCCODE err;
	nuint32 v;
	wchar_t tmpb[MAX_DN_CHARS+1];

	err = NWDSGetContext(ctx, DCK_FLAGS, &v);
	if (err)
		return err;
	err = NWDSPtrDN(p, len, tmpb, sizeof(tmpb));
	if (err)
		return err;
	if ((v & DCV_CANONICALIZE_NAMES) && (ctx->dck.name_form == 0)) {
		wchar_t abbrev[MAX_DN_CHARS+1];
		
		err = NWDSAbbreviateNameW(ctx, tmpb, abbrev);
		if (!err)
			err = NWDSXlateToCtx(ctx, ret, MAX_DN_BYTES, abbrev, ln);
	} else {
		err = NWDSXlateToCtx(ctx, ret, MAX_DN_BYTES, tmpb, ln);
	}
	return err;
}

NWDSCCODE NWDSBufCtxDN(NWDSContextHandle ctx, Buf_T* buffer,
		void* ret, size_t* ln) {
	NWDSCCODE err;
	nuint32 len32;
	const unicode* ptr;
	
	err = NWDSBufGetLE32(buffer, &len32);
	if (err)
		return err;
	ptr = NWDSBufGetPtr(buffer, len32);
	return NWDSPtrCtxDN(ctx, ptr, len32, ret, ln);
}

static NWDSCCODE NWDSBufPutCIStringLen(Buf_T* buffer, size_t len, const wchar_t* string) {
	NWDSCCODE err;
	size_t maxlen;
	void* lenspace;
	unicode* strspace;
	unicode* ep;
	
	lenspace = NWDSBufPutPtr(buffer, 4);
	if (!lenspace)
		return ERR_BUFFER_FULL;
	strspace = NWDSBufPutPtrLen(buffer, &maxlen);
	err = __NWUInternalToUnicode(strspace, strspace + maxlen / sizeof(*strspace), 
		string, string + len, NULL, &ep, NULL);
	if (err) {
		buffer->curPos = lenspace;
		return ERR_BUFFER_FULL;
	}
	maxlen = (ep - strspace) * sizeof(*ep);
	DSET_LH(lenspace, 0, maxlen);
	NWDSBufPutSkip(buffer, maxlen);
	return 0;
}

static inline NWDSCCODE NWDSBufPutCIString(Buf_T* buffer, const wchar_t* string) {
	return NWDSBufPutCIStringLen(buffer, wcslen(string)+1, string);
}

NWDSCCODE NWDSBufSkipBuffer(Buf_T* buffer) {
	NWDSCCODE err;
	nuint32 len;
	
	err = NWDSBufGetLE32(buffer, &len);
	if (err)
		return err;
	NWDSBufGetSkip(buffer, len);
	return 0;
}
	
NWDSCCODE NWDSBufPutBuffer(Buf_T* buffer, const void* data, size_t len) {
	nuint8* lenspace;
	
	lenspace = NWDSBufPutPtr(buffer, 4 + len);
	if (!lenspace)
		return ERR_BUFFER_FULL;
	DSET_LH(lenspace, 0, len);
	memcpy(lenspace + 4, data, len);
	return 0;
}

static inline NWDSCCODE NWDSBufPutUnicodeString(Buf_T* buffer, const unicode* string) {
	return NWDSBufPutBuffer(buffer, string, (unilen(string) + 1) * sizeof(unicode));
}

NWDSCCODE NWDSGetCanonicalizedName(NWDSContextHandle ctx, const NWDSChar* src,
		wchar_t* dst) {
	NWDSCCODE err;

	if (src) {
		nuint32 v;
		
		err = NWDSGetContext(ctx, DCK_FLAGS, &v);
		if (err)
			return err;
		if ((v & DCV_CANONICALIZE_NAMES) && (ctx->dck.name_form == 0)) {
			wchar_t tmp[MAX_DN_CHARS+1];
			
			err = NWDSXlateFromCtx(ctx, tmp, sizeof(tmp), src);
			if (!err)
				err = NWDSCanonicalizeNameW(ctx, tmp, dst);
		} else
			err = NWDSXlateFromCtx(ctx, dst, (MAX_DN_CHARS + 1) * sizeof(wchar_t), src);
	} else {
		*dst = 0;
		err = 0;
	}
	return err;
}

NWDSCCODE NWDSCtxBufDN(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* name) {
	NWDSCCODE err;
		
	if (name) {
		wchar_t tmpb[MAX_DN_CHARS+1];
		
		err = NWDSGetCanonicalizedName(ctx, name, tmpb);
		if (!err)
			err = NWDSBufPutCIString(buffer, tmpb);
	} else
		err = NWDSBufPutLE32(buffer, 0); /* NULL -> zero length... */
	return err;
}

static inline NWDSCCODE NWDSPutAttrVal_DIST_NAME(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* name) {
	return NWDSCtxBufDN(ctx, buffer, name);
}

/* Attribute name */
static inline NWDSCCODE NWDSGetAttrVal_XX_STRING(NWDSContextHandle ctx, Buf_T* buffer,
		NWDSChar* name, size_t maxlen) {
	return NWDSBufCtxString(ctx, buffer, name, maxlen, NULL);
}

/* Object name (not in attribute) */
static inline NWDSCCODE NWDSGetAttrVal_DIST_NAME(NWDSContextHandle ctx, Buf_T* buffer,
		void* ret) {
	return NWDSBufCtxDN(ctx, buffer, ret, NULL);
}

/* Any string... */
static inline NWDSCCODE NWDSGetAttrSize_XX_STRING2(NWDSContextHandle ctx,
		const unicode* ptr, size_t len, size_t* strLen) {
	return NWDSPtrCtxString(ctx, ptr, len, NULL, 0, strLen);
}

static inline NWDSCCODE NWDSGetAttrVal_XX_STRING2(NWDSContextHandle ctx, 
		const unicode* ptr, size_t len, NWDSChar* name) {
	return NWDSPtrCtxString(ctx, ptr, len, name, 9999999, NULL);
}

/* Dist Name */
static inline NWDSCCODE NWDSGetAttrSize_DIST_NAME2(NWDSContextHandle ctx,
		const unicode* p, size_t len, size_t* strLen) {
	char tmp[MAX_DN_BYTES+1];
	return NWDSPtrCtxDN(ctx, p, len, tmp, strLen);
}

static inline NWDSCCODE NWDSGetAttrVal_DIST_NAME2(NWDSContextHandle ctx, 
		const unicode* p, size_t len, void* ret) {
	return NWDSPtrCtxDN(ctx, p, len, ret, NULL);
}

/* CI List */
static NWDSCCODE NWDSGetAttrSize_CI_LIST(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	nuint32 cnt;
	size_t cs = 0;
	
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	while (cnt--) {
		size_t ln;
		
		err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
		if (err)
			return err;
		cs += ROUNDBUFF(ln) + sizeof(CI_List_T);
	}
	if (!cs)
		cs = sizeof(CI_List_T);
	*size = cs;
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_CI_LIST(NWDSContextHandle ctx, Buf_T* buffer,
		CI_List_T* ptr) {
	NWDSCCODE err;
	nuint32 cnt;
	u_int8_t* spareSpace;
	
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	ptr->s = NULL;
	spareSpace = (u_int8_t*)ptr;
	while (cnt--) {
		size_t ln;
		
		ptr->next = (CI_List_T*)spareSpace;
		ptr = (CI_List_T*)spareSpace;
		spareSpace = (u_int8_t*)(ptr + 1);
		ptr->s = (NWDSChar*)spareSpace;
		err = NWDSBufCtxString(ctx, buffer, (NWDSChar*)spareSpace, 999999, &ln);
		if (err)
			return err;
		spareSpace += ROUNDBUFF(ln);
	}
	ptr->next = NULL;
	return 0;
}

static NWDSCCODE NWDSPutAttrVal_CI_LIST(NWDSContextHandle ctx, Buf_T* buffer,
		const CI_List_T* ptr) {
	NWDSCCODE err;
	nuint32 cnt = 0;
	nuint8* tlen;
	
	tlen = NWDSBufPutPtr(buffer, 8);
	if (!tlen)
		return ERR_BUFFER_FULL;
	while (ptr) {
		err = NWDSCtxBufString(ctx, buffer, ptr->s);
		if (err)
			return err;
		cnt++;
		ptr = ptr->next;
	}
	DSET_LH(tlen, 0, buffer->curPos - tlen - 4);
	DSET_LH(tlen, 4, cnt);
	return 0;
}
	
/* Octet List */
static NWDSCCODE NWDSGetAttrSize_OCTET_LIST(UNUSED(NWDSContextHandle ctx), 
		Buf_T* buffer, size_t* size) {
	NWDSCCODE err;
	nuint32 cnt;
	size_t cs = 0;
	
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	while (cnt--) {
		nuint32 ln;
		
		err = NWDSBufGetLE32(buffer, &ln);
		if (err)
			return err;
		if (!NWDSBufGetPtr(buffer, ln))
			return ERR_BUFFER_EMPTY;
		cs += ROUNDBUFF(ln) + sizeof(Octet_List_T);
	}
	if (!cs)
		cs = sizeof(Octet_List_T);
	*size = cs;
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_OCTET_LIST(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		Octet_List_T* ptr) {
	NWDSCCODE err;
	nuint32 cnt;
	u_int8_t* spareSpace;
	
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	ptr->data = NULL;
	spareSpace = (u_int8_t*)ptr;
	while (cnt--) {
		nuint32 ln;
		
		ptr->next = (Octet_List_T*)spareSpace;
		ptr = (Octet_List_T*)spareSpace;
		spareSpace = (u_int8_t*)(ptr + 1);
		ptr->data = spareSpace;
		err = NWDSBufGetLE32(buffer, &ln);
		if (err)
			return err;
		ptr->length = ln;
		err = NWDSBufGet(buffer, spareSpace, ln);
		if (err)
			return err;
		spareSpace += ROUNDBUFF(ln);
	}
	ptr->next = NULL;
	return 0;
}

static NWDSCCODE NWDSPutAttrVal_OCTET_LIST(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		const Octet_List_T* ptr) {
	NWDSCCODE err;
	nuint32 cnt = 0;
	nuint8* tlen;
	
	tlen = NWDSBufPutPtr(buffer, 8);
	if (!tlen)
		return ERR_BUFFER_FULL;
	while (ptr) {
		if (ptr->length && !ptr->data)
			return ERR_NULL_POINTER;
		err = NWDSBufPutBuffer(buffer, ptr->data, ptr->length);
		if (err)
			return err;
		cnt++;
		ptr = ptr->next;
	}
	DSET_LH(tlen, 0, buffer->curPos - tlen - 4);
	DSET_LH(tlen, 4, cnt);
	return 0;
}

/* Replica Pointer */
static NWDSCCODE NWDSGetAttrSize_REPLICA_POINTER(NWDSContextHandle ctx, 
		Buf_T* buffer, size_t* size) {
	NWDSCCODE err;
	char tmpb[MAX_DN_BYTES+1];
	nuint32 le32;
	nuint32 cnt;
	size_t cs;
	
	err = NWDSBufCtxDN(ctx, buffer, tmpb, &cs);
	if (err)
		return err;
	/* DN is at end of buffer, so no rounding here... */
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	cs += sizeof(Net_Address_T) * cnt + 
	      offsetof(Replica_Pointer_T,replicaAddressHint);
	while (cnt--) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		if (!NWDSBufGetPtr(buffer, le32))
			return ERR_BUFFER_EMPTY;
		cs += ROUNDBUFF(le32);
	}
	*size = cs;
	return err;
}

static NWDSCCODE NWDSGetAttrVal_REPLICA_POINTER(NWDSContextHandle ctx, Buf_T* buffer,
		Replica_Pointer_T* ptr) {
	NWDSCCODE err;
	nuint32 v;
	wchar_t tmpb[MAX_DN_CHARS+1];
	nuint32 le32;
	nuint32 cnt;
	Net_Address_T* nat;
	u_int8_t* spareSpace;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &v);
	if (err)
		return err;
	err = NWDSBufDN(buffer, tmpb, sizeof(tmpb));
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	ptr->replicaType = le32;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	ptr->replicaNumber = le32;
	err = NWDSBufGetLE32(buffer, &cnt);
	if (err)
		return err;
	ptr->count = cnt;
	nat = ptr->replicaAddressHint;
	spareSpace = (u_int8_t*)(nat + cnt);
	while (cnt--) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		nat->addressType = le32;
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		nat->addressLength = le32;
		err = NWDSBufGet(buffer, spareSpace, le32);
		if (err)
			return err;
		nat->address = spareSpace;
		spareSpace += ROUNDBUFF(le32);
		nat++;
	}
	ptr->serverName = (NWDSChar*)spareSpace;
	if ((v & DCV_CANONICALIZE_NAMES) && (ctx->dck.name_form == 0)) {
		wchar_t abbrev[MAX_DN_CHARS+1];
		
		err = NWDSAbbreviateNameW(ctx, tmpb, abbrev);
		if (!err)
			err = NWDSXlateToCtx(ctx, spareSpace, MAX_DN_BYTES, abbrev, NULL);
	} else {
		err = NWDSXlateToCtx(ctx, spareSpace, MAX_DN_BYTES, tmpb, NULL);
	}
	return err;
}

static NWDSCCODE NWDSPutAttrVal_REPLICA_POINTER(NWDSContextHandle ctx, Buf_T* buffer,
		const Replica_Pointer_T* ptr) {
	NWDSCCODE err;
	nuint32 cnt;
	const Net_Address_T* nat;
	nuint8* p;
	
	p = NWDSBufPutPtr(buffer, 4);
	if (!p)
		return ERR_BUFFER_FULL;
	err = NWDSCtxBufDN(ctx, buffer, ptr->serverName);
	if (err)
		return err;
	err = NWDSBufPutLE32(buffer, ptr->replicaType);
	if (err)
		return err;
	err = NWDSBufPutLE32(buffer, ptr->replicaNumber);
	if (err)
		return err;
	err = NWDSBufPutLE32(buffer, cnt = ptr->count);
	if (err)
		return err;
	nat = ptr->replicaAddressHint;
	while (cnt--) {
		if (nat->addressLength && !nat->address)
			return ERR_NULL_POINTER;
		err = NWDSBufPutLE32(buffer, nat->addressType);
		if (err)
			return err;
		err = NWDSBufPutBuffer(buffer, nat->address, nat->addressLength);
		if (err)
			return err;
		nat++;
	}
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* Net address... size is easy */
static NWDSCCODE NWDSGetAttrVal_NET_ADDRESS(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		Net_Address_T* nt) {
	nuint32 type, len;
	NWDSCCODE err;
	void* p;
	
	err = NWDSBufGetLE32(buffer, &type);
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &len);
	if (err)
		return err;
	p = NWDSBufGetPtr(buffer, len);
	if (!p)
		return ERR_BUFFER_EMPTY;
	if (nt) {
		nt->addressType = type;
		nt->addressLength = len;
		memcpy(nt->address = (void*)(nt + 1), p, len);
	}
	return err;
}

static NWDSCCODE NWDSPutAttrVal_NET_ADDRESS(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		const Net_Address_T* nt) {
	nuint8* ptr;
	
	if (nt->addressLength && !nt->address)
		return ERR_NULL_POINTER;
	ptr = NWDSBufPutPtr(buffer, 4 + 4 + 4 + nt->addressLength);
	if (!ptr)
		return ERR_BUFFER_FULL;
	DSET_LH(ptr, 0, 4 + 4 + nt->addressLength);
	DSET_LH(ptr, 4, nt->addressType);
	DSET_LH(ptr, 8, nt->addressLength);
	memcpy(ptr + 12, nt->address, nt->addressLength);
	return 0;
}
	
/* Octet string... size is easy */
static inline NWDSCCODE NWDSGetAttrVal_OCTET_STRING(UNUSED(NWDSContextHandle ctx), 
		void* p, size_t len, Octet_String_T* os) {
	os->length = len;
	memcpy(os->data = (void*)(os + 1), p, len);
	return 0;
}

static NWDSCCODE NWDSPutAttrVal_OCTET_STRING(UNUSED(NWDSContextHandle ctx),
		Buf_T* buffer, const Octet_String_T* os) {
	if (os->length && !os->data)
		return ERR_NULL_POINTER;
	return NWDSBufPutBuffer(buffer, os->data, os->length);
}

/* Integer... size is easy */
static NWDSCCODE NWDSBufPutSizedLE32(Buf_T* buffer, nuint32 le) {
	NWDSCCODE err;

	err = NWDSBufPutLE32(buffer, 4);
	if (err)
		return err;
	return NWDSBufPutLE32(buffer, le);
}

static NWDSCCODE NWDSGetAttrVal_INTEGER(UNUSED(NWDSContextHandle ctx), 
		void* p, size_t len, Integer_T* i) {
	if (len != 4)
		return NWE_BUFFER_OVERFLOW;
	*i = DVAL_LH(p, 0);
	return 0;
}

static inline NWDSCCODE NWDSPutAttrVal_INTEGER(UNUSED(NWDSContextHandle ctx),
		Buf_T* buffer, const Integer_T* i) {
	return NWDSBufPutSizedLE32(buffer, *i);
}

/* Boolean... size is easy */
static NWDSCCODE NWDSGetAttrVal_BOOLEAN(UNUSED(NWDSContextHandle ctx),
		void* p, size_t len, Boolean_T* b) {
	if (len != 1)
		return NWE_BUFFER_OVERFLOW;
	*b = BVAL(p, 0);
	return 0;
}

static inline NWDSCCODE NWDSPutAttrVal_BOOLEAN(UNUSED(NWDSContextHandle ctx),
		Buf_T* buffer, const Boolean_T* b) {
	NWDSCCODE err;

	err = NWDSBufPutLE32(buffer, 1);
	if (err)
		return err;
	return NWDSBufPutLE32(buffer, *b);
}

/* Time... size is easy */
static NWDSCCODE NWDSGetAttrVal_TIME(UNUSED(NWDSContextHandle ctx),
		void* p, size_t len, Time_T* t) {
	if (len != 4)
		return NWE_BUFFER_OVERFLOW;
	*t = DVAL_LH(p, 0);
	return 0;
}

static inline NWDSCCODE NWDSPutAttrVal_TIME(UNUSED(NWDSContextHandle ctx),
		Buf_T* buffer, const Time_T* t) {
	return NWDSBufPutSizedLE32(buffer, *t);
}

/* TimeStamp... size is easy */
static NWDSCCODE NWDSGetAttrVal_TIMESTAMP(UNUSED(NWDSContextHandle ctx),
		void* p, size_t len, TimeStamp_T* ts) {
	if (len != 8)
		return NWE_BUFFER_OVERFLOW;
	ts->wholeSeconds = DVAL_LH(p, 0);
	ts->replicaNum = WVAL_LH(p, 4);
	ts->eventID = WVAL_LH(p, 6);
	return 0;
}

NWDSCCODE NWDSPutAttrVal_TIMESTAMP(UNUSED(NWDSContextHandle ctx),
		Buf_T* buffer, const TimeStamp_T* ts) {
	NWDSCCODE err;
	void* p;

	err = NWDSBufPutLE32(buffer, 8);
	if (err)
		return err;
	p = NWDSBufPutPtr(buffer, 8);
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_LH(p, 0, ts->wholeSeconds);
	WSET_LH(p, 4, ts->replicaNum);
	WSET_LH(p, 6, ts->eventID);
	return 0;
}

/* Object ACL */
static NWDSCCODE NWDSGetAttrSize_OBJECT_ACL(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	char tmp[MAX_DN_BYTES+1];
	size_t cs;
	
	err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
	if (err)
		return err;
	cs = ROUNDBUFF(ln) + sizeof(Object_ACL_T);
	
	err = NWDSBufCtxDN(ctx, buffer, tmp, &ln);
	if (err)
		return err;
	*size = cs + ln;
	return err;
}

static NWDSCCODE NWDSGetAttrVal_OBJECT_ACL(NWDSContextHandle ctx, Buf_T* buffer,
		Object_ACL_T* oacl) {
	u_int8_t* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 priv;
	
	spareSpace = (u_int8_t*)(oacl + 1);
	oacl->protectedAttrName = (NWDSChar*)spareSpace;
	
	err = NWDSBufCtxString(ctx, buffer, (NWDSChar*)spareSpace, 999999, &ln);
	if (err)
		return err;
	spareSpace += ROUNDBUFF(ln);
	
	oacl->subjectName = (NWDSChar*)spareSpace;
	err = NWDSBufCtxDN(ctx, buffer, (NWDSChar*)spareSpace, NULL);
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &priv);
	oacl->privileges = priv;
	return err;
}

static NWDSCCODE NWDSPutAttrVal_OBJECT_ACL(NWDSContextHandle ctx, Buf_T* buffer,
		const Object_ACL_T* oacl) {
	NWDSCCODE err;
	nuint8* p = NWDSBufPutPtr(buffer, 4);

	if (!p)
		return ERR_BUFFER_FULL;
	err = NWDSCtxBufString(ctx, buffer, oacl->protectedAttrName);
	if (err)
		return err;
	err = NWDSCtxBufDN(ctx, buffer, oacl->subjectName);
	if (err)
		return err;
	err = NWDSBufPutLE32(buffer, oacl->privileges);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* Path */
static NWDSCCODE NWDSGetAttrSize_PATH(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 nstype;
	size_t cs;
	char tmp[MAX_DN_BYTES+1];
	
	err = NWDSBufGetLE32(buffer, &nstype);
	if (err)
		return err;
	
	err = NWDSBufCtxDN(ctx, buffer, tmp, &ln);
	if (err)
		return err;
	cs = ROUNDBUFF(ln) + sizeof(Path_T);
	
	err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
	if (err)
		return err;
	*size = cs + ln;
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_PATH(NWDSContextHandle ctx, Buf_T* buffer,
		Path_T* p) {
	u_int8_t* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 nstype;
	
	spareSpace = (u_int8_t*)(p + 1);
	err = NWDSBufGetLE32(buffer, &nstype);
	if (err)
		return err;
	p->nameSpaceType = nstype;
	p->volumeName = (NWDSChar*)spareSpace;
	
	err = NWDSBufCtxDN(ctx, buffer, (NWDSChar*)spareSpace, &ln);
	if (err)
		return err;
	spareSpace += ROUNDBUFF(ln);
	
	p->path = (NWDSChar*)spareSpace;
	err = NWDSBufCtxString(ctx, buffer, (NWDSChar*)spareSpace, 999999, &ln);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_PATH(NWDSContextHandle ctx, Buf_T* buffer,
		const Path_T* p) {
	NWDSCCODE err;
	nuint8* bp = NWDSBufPutPtr(buffer, 8); 
	
	DSET_LH(bp, 4, p->nameSpaceType);
	err = NWDSCtxBufDN(ctx, buffer, p->volumeName);
	if (err)
		return err;
	err = NWDSCtxBufString(ctx, buffer, p->path);
	if (err)
		return err;
	DSET_LH(bp, 0, buffer->curPos - bp - 4);
	return 0;
}

/* Typed Name */
static NWDSCCODE NWDSGetAttrSize_TYPED_NAME(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	char tmp[MAX_DN_BYTES+1];
	
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;

	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;

	err = NWDSBufCtxDN(ctx, buffer, tmp, &ln);
	if (err)
		return err;
	*size = ln + sizeof(Typed_Name_T);
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_TYPED_NAME(NWDSContextHandle ctx, Buf_T* buffer,
		Typed_Name_T* p) {
	NWDSChar* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	
	spareSpace = (NWDSChar*)(p + 1);
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	p->level = v;

	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	p->interval = v;

	p->objectName = spareSpace;
	err = NWDSBufCtxDN(ctx, buffer, spareSpace, &ln);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_TYPED_NAME(NWDSContextHandle ctx, Buf_T* buffer,
		const Typed_Name_T* tn) {
	NWDSCCODE err;
	nuint8* p = NWDSBufPutPtr(buffer, 12);
	
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_LH(p, 4, tn->level);
	DSET_LH(p, 8, tn->interval);
	err = NWDSCtxBufDN(ctx, buffer, tn->objectName);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* BackLink */
static NWDSCCODE NWDSGetAttrSize_BACK_LINK(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	NWObjectID id;
	char tmp[MAX_DN_BYTES+1];
	
	err = NWDSBufGetID(buffer, &id);
	if (err)
		return err;

	err = NWDSBufCtxDN(ctx, buffer, tmp, &ln);
	if (err)
		return err;
	*size = ln + sizeof(Back_Link_T);
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_BACK_LINK(NWDSContextHandle ctx, Buf_T* buffer,
		Back_Link_T* p) {
	NWDSChar* spareSpace;
	NWDSCCODE err;
	size_t ln;
	NWObjectID id;
	
	spareSpace = (NWDSChar*)(p + 1);
	err = NWDSBufGetID(buffer, &id);
	if (err)
		return err;
	p->remoteID = id;

	p->objectName = spareSpace;
	err = NWDSBufCtxDN(ctx, buffer, spareSpace, &ln);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_BACK_LINK(NWDSContextHandle ctx, Buf_T* buffer,
		const Back_Link_T* bl) {
	NWDSCCODE err;
	nuint8* p = NWDSBufPutPtr(buffer, 8);
	
	if (!bl->objectName)
		return ERR_NULL_POINTER;
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_HL(p, 4, bl->remoteID);
	err = NWDSCtxBufDN(ctx, buffer, bl->objectName);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* Hold */
static NWDSCCODE NWDSGetAttrSize_HOLD(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 amount;
	char tmp[MAX_DN_BYTES+1];
	
	err = NWDSBufGetLE32(buffer, &amount);
	if (err)
		return err;

	err = NWDSBufCtxDN(ctx, buffer, tmp, &ln);
	if (err)
		return err;
	*size = ln + sizeof(Hold_T);
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_HOLD(NWDSContextHandle ctx, Buf_T* buffer,
		Hold_T* p) {
	NWDSChar* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 amount;
	
	spareSpace = (NWDSChar*)(p + 1);
	err = NWDSBufGetLE32(buffer, &amount);
	if (err)
		return err;
	p->amount = amount;

	p->objectName = spareSpace;
	err = NWDSBufCtxDN(ctx, buffer, spareSpace, &ln);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_HOLD(NWDSContextHandle ctx, Buf_T* buffer,
		const Hold_T* h) {
	NWDSCCODE err;
	nuint8* p = NWDSBufPutPtr(buffer, 8);
	
	if (!h->objectName)
		return ERR_NULL_POINTER;
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_LH(p, 4, h->amount);
	err = NWDSCtxBufDN(ctx, buffer, h->objectName);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* Fax Number */
static NWDSCCODE NWDSGetAttrSize_FAX_NUMBER(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	nuint32 l;
	
	err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
	if (err)
		return err;

	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;

	err = NWDSBufGetLE32(buffer, &l);
	if (err)
		return err;
	if (v > l * 8)
		return ERR_INVALID_SERVER_RESPONSE;
	*size = sizeof(Fax_Number_T) + ROUNDBUFF(ln) + l;
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_FAX_NUMBER(NWDSContextHandle ctx, Buf_T* buffer,
		Fax_Number_T* p) {
	u_int8_t* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	nuint32 l;
	
	spareSpace = (u_int8_t*)(p + 1);
	p->telephoneNumber = (NWDSChar*)spareSpace;
	err = NWDSBufCtxString(ctx, buffer, (NWDSChar*)spareSpace, 999999, &ln);
	if (err)
		return err;
	spareSpace += ROUNDBUFF(ln);

	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	p->parameters.numOfBits = v;

	err = NWDSBufGetLE32(buffer, &l);
	if (err)
		return err;
	if (v > l * 8)
		return ERR_INVALID_SERVER_RESPONSE;
	p->parameters.data = spareSpace;
	err = NWDSBufGet(buffer, spareSpace, l);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_FAX_NUMBER(NWDSContextHandle ctx, Buf_T* buffer,
		const Fax_Number_T* fn) {
	NWDSCCODE err;
	nuint8* p;
	
	if (fn->parameters.numOfBits && !fn->parameters.data)
		return ERR_NULL_POINTER;
	p = NWDSBufPutPtr(buffer, 4);
	if (!p)
		return ERR_BUFFER_FULL;
	err = NWDSCtxBufString(ctx, buffer, fn->telephoneNumber);
	if (err)
		return err;
	err = NWDSBufPutLE32(buffer, fn->parameters.numOfBits);
	if (err)
		return err;
	err = NWDSBufPutBuffer(buffer, fn->parameters.data, (fn->parameters.numOfBits + 7) >> 3);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* Email Address */
static NWDSCCODE NWDSGetAttrSize_EMAIL_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;

	err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
	if (err)
		return err;
	*size = ln + sizeof(EMail_Address_T);
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_EMAIL_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		EMail_Address_T* p) {
	NWDSChar* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	
	spareSpace = (NWDSChar*)(p + 1);
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	p->type = v;

	p->address = spareSpace;
	err = NWDSBufCtxString(ctx, buffer, spareSpace, 999999, &ln);
	return err;
}

static NWDSCCODE NWDSPutAttrVal_EMAIL_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		const EMail_Address_T* ea) {
	NWDSCCODE err;
	nuint8* p = NWDSBufPutPtr(buffer, 8);
	
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_LH(p, 4, ea->type);
	err = NWDSCtxBufString(ctx, buffer, ea->address);
	if (err)
		return err;
	DSET_LH(p, 0, buffer->curPos - p - 4);
	return 0;
}

/* PO Address */
static NWDSCCODE NWDSGetAttrSize_PO_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		size_t* size) {
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	nuint32 cnt;
	size_t cs = sizeof(Postal_Address_T);
	
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	if (v > 6)
		v = 6;
	cnt = 6 - v;
	while (v--) {
		err = NWDSBufCtxString(ctx, buffer, NULL, 0, &ln);
		if (err)
			return err;
		cs += ROUNDBUFF(ln);
	}
	*size = cs;
	return 0;
}

static NWDSCCODE NWDSGetAttrVal_PO_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		Postal_Address_T* p) {
	u_int8_t* spareSpace;
	NWDSCCODE err;
	size_t ln;
	nuint32 v;
	nuint32 cnt;
	NWDSChar** q;
	
	spareSpace = (u_int8_t*)(p + 1);
	err = NWDSBufGetLE32(buffer, &v);
	if (err)
		return err;
	if (v > 6)
		v = 6;
	cnt = 6 - v;
	q = *p;
	while (v--) {
		*q++ = (NWDSChar*)spareSpace;
		err = NWDSBufCtxString(ctx, buffer, (NWDSChar*)spareSpace, 9999999, &ln);
		if (err)
			return err;
		spareSpace += ROUNDBUFF(ln);
	}
	while (cnt--) {
		*q++ = NULL;
	}
	return 0;
}

static NWDSCCODE NWDSPutAttrVal_PO_ADDRESS(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* const* pa) {
	NWDSCCODE err;
	const NWDSChar* const* q;
	nuint32 cnt;
	nuint8* p = NWDSBufPutPtr(buffer, 8);
	
	q = pa;
	for (cnt = 6; cnt > 0; cnt--) {
		if (q[cnt])
			break;
	}
	DSET_LH(p, 4, cnt);
	while (cnt--) {
		err = NWDSCtxBufString(ctx, buffer, *(q++));
		if (err)
			return err;
	}
	DSET_LH(p, 0, buffer->curPos - p -4);
	return 0;
}

/* ... */

static NWDSCCODE NWDSSplitName(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		wchar_t* parentName,
		wchar_t* childName) {
	NWDSCCODE err;
	nuint32 v;
	wchar_t tmpb[MAX_DN_CHARS+1];
	wchar_t* c;
		
	err = NWDSGetContext(ctx, DCK_FLAGS, &v);
	if (err)
		return err;
	if ((v & DCV_CANONICALIZE_NAMES) && (ctx->dck.name_form == 0)) {
		err = NWDSXlateFromCtx(ctx, parentName, MAX_DN_BYTES, objectName);
		if (err)
			return err;
		err = NWDSCanonicalizeNameW(ctx, parentName, tmpb);
	} else {
		err = NWDSXlateFromCtx(ctx, tmpb, sizeof(tmpb), objectName);
	}
	if (err)
		return err;
	c = tmpb;
	if ((tmpb[0] == '\\') && ((tmpb[1] != '.') && (tmpb[1] != '+') && (tmpb[1] != '=') && (tmpb[1] != '\\'))) {
		wchar_t* lastSlash;
		
		lastSlash = c++;
		while (*c) {
			if (*c == '\\')
				lastSlash = c;
			c++;
		}
		*lastSlash++ = 0;
		memcpy(parentName, tmpb, (lastSlash - tmpb) * sizeof(wchar_t));
		memcpy(childName, lastSlash, (c - lastSlash + 1) * sizeof(wchar_t));
	} else {
		while (*c) {
			if (*c == '.')
				break;
			if (*c++ == '\\') {
				if (*c)
					c++;
			}
		}
		wcscpy(parentName, (*c)?c+1:L"Root");
		*c = 0;
		memcpy(childName, tmpb, (c - tmpb + 1) * sizeof(wchar_t));
	}
	return 0;
}

NWDSCCODE NWDSPutAttrVal_XX_STRING(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* name) {
	return NWDSCtxBufString(ctx, buffer, name);
}

NWDSCCODE NWDSComputeAttrValSize(NWDSContextHandle ctx, Buf_T* buffer,
		enum SYNTAX syntaxID, size_t* valsize) {
	NWDSCCODE err = 0;
	nuint32 le32;
	size_t size = 0;
	nuint32 tlen;
	size_t blen;
	void* p;
	Buf_T buf;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ_CLASS_DEF:;
			if (syntaxID != SYN_OBJECT_ACL)
				return ERR_BAD_VERB;
			p = NWDSBufRetrievePtrAndLen(buffer, &blen);
			NWDSSetupBuf(&buf, p, blen);
			err = NWDSGetAttrSize_OBJECT_ACL(ctx, &buf, &size);
			goto quit;
		default:;
			break;
	}
	err = NWDSBufPeekLE32(buffer, 0, &tlen);
	if (err)
		return err;
	p = NWDSBufPeekPtrLen(buffer, 4, tlen);
	if (!p)
		return ERR_BUFFER_EMPTY;
	NWDSSetupBuf(&buf, p, tlen);
	switch (syntaxID) {
		case SYN_DIST_NAME:
			err = NWDSGetAttrSize_DIST_NAME2(ctx, p, tlen, &size);
			break;
		case SYN_CE_STRING:
		case SYN_CI_STRING:
		case SYN_PR_STRING:
		case SYN_NU_STRING:
		case SYN_CLASS_NAME:
		case SYN_TEL_NUMBER:
			err = NWDSGetAttrSize_XX_STRING2(ctx, p, tlen, &size);
			break;
		case SYN_OCTET_STRING:
		case SYN_STREAM:
			err = NWDSBufPeekLE32(buffer, 0, &le32);
			if (!err)
				size = le32 + sizeof(Octet_String_T);
			break;
		case SYN_NET_ADDRESS:
			err = NWDSBufPeekLE32(buffer, 8, &le32);
			if (!err)
				size = le32 + sizeof(Net_Address_T);
			break;
		case SYN_BOOLEAN:
			size = sizeof(Boolean_T);
			break;
		case SYN_COUNTER:
		case SYN_INTEGER:
		case SYN_INTERVAL:
			size = sizeof(Integer_T);
			break;
		case SYN_TIMESTAMP:
			size = sizeof(TimeStamp_T);
			break;
		case SYN_TIME:
			size = sizeof(Time_T);
			break;
		case SYN_REPLICA_POINTER:
			err = NWDSGetAttrSize_REPLICA_POINTER(ctx, &buf, &size);
			break;
		case SYN_OBJECT_ACL:
			err = NWDSGetAttrSize_OBJECT_ACL(ctx, &buf, &size);
			break;
		case SYN_PATH:
			err = NWDSGetAttrSize_PATH(ctx, &buf, &size);
			break;
		case SYN_CI_LIST:
			err = NWDSGetAttrSize_CI_LIST(ctx, &buf, &size);
			break;
		case SYN_TYPED_NAME:
			err = NWDSGetAttrSize_TYPED_NAME(ctx, &buf, &size);
			break;
		case SYN_BACK_LINK:
			err = NWDSGetAttrSize_BACK_LINK(ctx, &buf, &size);
			break;
		case SYN_FAX_NUMBER:
			err = NWDSGetAttrSize_FAX_NUMBER(ctx, &buf, &size);
			break;
		case SYN_EMAIL_ADDRESS:
			err = NWDSGetAttrSize_EMAIL_ADDRESS(ctx, &buf, &size);
			break;
		case SYN_PO_ADDRESS:
			err = NWDSGetAttrSize_PO_ADDRESS(ctx, &buf, &size);
			break;
		case SYN_OCTET_LIST:
			err = NWDSGetAttrSize_OCTET_LIST(ctx, &buf, &size);
			break;
		case SYN_HOLD:
			err = NWDSGetAttrSize_HOLD(ctx, &buf, &size);
			break;
		/* FIXME: other syntaxes... */
		default:
			err = ERR_NO_SUCH_SYNTAX;
			break;
	}
quit:;
	if (!err && valsize)
		*valsize = size;
	return err;
}

NWDSCCODE NWDSGetAttrVal(NWDSContextHandle ctx, Buf_T* buffer,
		enum SYNTAX syntaxID, void* ptr) {
	NWDSCCODE err;
	size_t blen;
	nuint32 tlen;
	void* p;
	Buf_T buf;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ_CLASS_DEF:;
			if (syntaxID != SYN_OBJECT_ACL)
				return ERR_BAD_VERB;
			p = NWDSBufRetrievePtrAndLen(buffer, &blen);
			NWDSSetupBuf(&buf, p, blen);
			err = NWDSGetAttrVal_OBJECT_ACL(ctx, &buf, ptr);
			if (err)
				return err;
			buffer->curPos = buf.curPos;
			return 0;
		default:;
			break;
	}
	err = NWDSBufPeekLE32(buffer, 0, &tlen);
	if (err)
		return err;
	p = NWDSBufPeekPtrLen(buffer, 4, tlen);
	if (!p)
		return ERR_BUFFER_EMPTY;
	if (ptr) {
		NWDSSetupBuf(&buf, p, tlen);
		switch (syntaxID) {
			case SYN_DIST_NAME:
				err = NWDSGetAttrVal_DIST_NAME2(ctx, p, tlen, ptr);
				break;
			case SYN_NET_ADDRESS:
				err = NWDSGetAttrVal_NET_ADDRESS(ctx, &buf, ptr);
				break;
			case SYN_CI_STRING:
			case SYN_CE_STRING:
			case SYN_NU_STRING:
			case SYN_PR_STRING:
			case SYN_CLASS_NAME:
			case SYN_TEL_NUMBER:
				err = NWDSGetAttrVal_XX_STRING2(ctx, p, tlen, ptr);
				break;
			case SYN_OCTET_STRING:
			case SYN_STREAM:
				err = NWDSGetAttrVal_OCTET_STRING(ctx, p, tlen, ptr);
				break;
			case SYN_BOOLEAN:
				err = NWDSGetAttrVal_BOOLEAN(ctx, p, tlen, ptr);
				break;
			case SYN_COUNTER:
			case SYN_INTEGER:
			case SYN_INTERVAL:
				err = NWDSGetAttrVal_INTEGER(ctx, p, tlen, ptr);
				break;
			case SYN_TIMESTAMP:
				err = NWDSGetAttrVal_TIMESTAMP(ctx, p, tlen, ptr);
				break;
			case SYN_REPLICA_POINTER:
				err = NWDSGetAttrVal_REPLICA_POINTER(ctx, &buf, ptr);
				break;
			case SYN_OBJECT_ACL:
				err = NWDSGetAttrVal_OBJECT_ACL(ctx, &buf, ptr);
				break;
			case SYN_PATH:
				err = NWDSGetAttrVal_PATH(ctx, &buf, ptr);
				break;
			case SYN_TIME:
				err = NWDSGetAttrVal_TIME(ctx, p, tlen, ptr);
				break;
			case SYN_CI_LIST:
				err = NWDSGetAttrVal_CI_LIST(ctx, &buf, ptr);
				break;
			case SYN_TYPED_NAME:
				err = NWDSGetAttrVal_TYPED_NAME(ctx, &buf, ptr);
				break;
			case SYN_BACK_LINK:
				err = NWDSGetAttrVal_BACK_LINK(ctx, &buf, ptr);
				break;
			case SYN_FAX_NUMBER:
				err = NWDSGetAttrVal_FAX_NUMBER(ctx, &buf, ptr);
				break;
			case SYN_EMAIL_ADDRESS:
				err = NWDSGetAttrVal_EMAIL_ADDRESS(ctx, &buf, ptr);
				break;
			case SYN_PO_ADDRESS:
				err = NWDSGetAttrVal_PO_ADDRESS(ctx, &buf, ptr);
				break;
			case SYN_OCTET_LIST:
				err = NWDSGetAttrVal_OCTET_LIST(ctx, &buf, ptr);
				break;
			case SYN_HOLD:
				err = NWDSGetAttrVal_HOLD(ctx, &buf, ptr);
				break;
			default:
				err = ERR_NO_SUCH_SYNTAX;
				break;
		}
	}
	if (err)
		return err;
	NWDSBufGetSkip(buffer, 4 + tlen);
	return 0;
}

NWDSCCODE NWDSGetAttrCount(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		NWObjectCount* count) {
	NWDSCCODE err;
	nuint32 le32;

	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	if (count)
		*count = le32;
	return 0;
}

NWDSCCODE NWDSGetObjectCount(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		NWObjectCount* count) {
	NWDSCCODE err;
	nuint32 le32;

	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_LIST:
		case DSV_SEARCH:
			break;
		default:
			return ERR_BAD_VERB;
	}
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	if (count)
		*count = le32;
	return 0;	
}

#define CMDFLAGS_FLAGS		0x01
#define CMDFLAGS_STAMP		0x02
#define CMDFLAGS_VALUELEN	0x04
#define CMDFLAGS_VALUEDATA	0x08
#define CMDFLAGS_SYNTAXID	0x10
#define CMDFLAGS_VALCOUNT	0x20
NWDSCCODE NWDSBufSetInfoType(Buf_T* buffer, nuint32 infoType) {
#define STD	CMDFLAGS_SYNTAXID | CMDFLAGS_VALCOUNT | CMDFLAGS_VALUELEN
	static const nuint32 replytype[] = {	0,
				STD | CMDFLAGS_VALUEDATA,
				STD | CMDFLAGS_VALUEDATA,
				STD | CMDFLAGS_FLAGS | CMDFLAGS_STAMP | CMDFLAGS_VALUEDATA,
				STD | CMDFLAGS_FLAGS | CMDFLAGS_STAMP };
#undef STD

	if (infoType > 4)
		return ERR_INVALID_REQUEST;
	buffer->cmdFlags = replytype[infoType];
	return 0;		
}

NWDSCCODE NWDSGetObjectName(NWDSContextHandle ctx, Buf_T* buffer,
		NWDSChar* objectName, NWObjectCount* attrCount, 
		Object_Info_T* oi) {
	NWDSCCODE err;
	nuint32 le32;
	nuint32 dsiflags;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_LIST:
		case DSV_READ_ENTRY_INFO:
		case DSV_SEARCH:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (oi)
		memset(oi, 0, sizeof(*oi));
	dsiflags = buffer->dsiFlags;
	if (dsiflags & DSI_OUTPUT_FIELDS) {
		err = NWDSBufGetLE32(buffer, &dsiflags);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_ENTRY_FLAGS) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		if (oi)
			oi->objectFlags = le32;
	}
	if (dsiflags & DSI_SUBORDINATE_COUNT) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		if (oi)
			oi->subordinateCount = le32;
	}
	if (dsiflags & DSI_MODIFICATION_TIME) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		if (oi)
			oi->modificationTime = le32;
	}
	if (dsiflags & DSI_MODIFICATION_TIMESTAMP)
		NWDSBufGetSkip(buffer, 8);
	if (dsiflags & DSI_CREATION_TIMESTAMP)
		NWDSBufGetSkip(buffer, 8);
	if (dsiflags & DSI_PARTITION_ROOT_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_PARENT_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REVISION_COUNT)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REPLICA_TYPE)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_BASE_CLASS) {
		err = NWDSBufCtxString(ctx, buffer, oi?oi->baseClass:NULL, MAX_SCHEMA_NAME_BYTES, NULL);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_RDN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_DN) {
		err = NWDSBufCtxDN(ctx, buffer, objectName, NULL);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PARTITION_ROOT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PARENT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PURGE_TIME)
		NWDSBufGetSkip(buffer, 4);
	/* DSI_DEREFERENCE_BASE_CLASSES */
	if (dsiflags & DSI_REPLICA_NUMBER)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REPLICA_STATE)
		NWDSBufGetSkip(buffer, 4);
	if (buffer->operation == DSV_SEARCH) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		/* Set which fields are valid for THIS object.
		   Maybe it can differ from one object to another? */
		err = NWDSBufSetInfoType(buffer, le32);
		if (err)
			return err;
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
	} else
		le32 = 0;
	if (attrCount)
		*attrCount = le32;
	return 0;
}

NWDSCCODE NWDSGetObjectNameAndInfo(
		NWDSContextHandle ctx, 
		Buf_T* buffer,
		NWDSChar* objectName, 
		NWObjectCount* attrCount, 
		char** info) {
	NWDSCCODE err;
	nuint32 dsiflags;
	nuint32 le32;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_LIST:
		case DSV_READ_ENTRY_INFO:
		case DSV_SEARCH:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (info)
		*info = buffer->curPos;
	dsiflags = buffer->dsiFlags;
	if (dsiflags & DSI_OUTPUT_FIELDS) {
		err = NWDSBufGetLE32(buffer, &dsiflags);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_ENTRY_FLAGS)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_SUBORDINATE_COUNT)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_MODIFICATION_TIME)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_MODIFICATION_TIMESTAMP)
		NWDSBufGetSkip(buffer, 8);
	if (dsiflags & DSI_CREATION_TIMESTAMP)
		NWDSBufGetSkip(buffer, 8);
	if (dsiflags & DSI_PARTITION_ROOT_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_PARENT_ID)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REVISION_COUNT)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REPLICA_TYPE)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_BASE_CLASS) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_RDN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_ENTRY_DN) {
		if (objectName)
			err = NWDSBufCtxDN(ctx, buffer, objectName, NULL);
		else
			err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PARTITION_ROOT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PARENT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}
	if (dsiflags & DSI_PURGE_TIME)
		NWDSBufGetSkip(buffer, 4);
	/* DSI_DEREFERENCE_BASE_CLASSES */
	if (dsiflags & DSI_REPLICA_NUMBER)
		NWDSBufGetSkip(buffer, 4);
	if (dsiflags & DSI_REPLICA_STATE)
		NWDSBufGetSkip(buffer, 4);
	if (buffer->operation == DSV_SEARCH) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
		err = NWDSBufSetInfoType(buffer, le32);
		if (err)
			return err;
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
	} else
		le32 = 0;
	if (attrCount)
		*attrCount = le32;
	return 0;
}

NWDSCCODE NWDSGetDSIInfo(
		NWDSContextHandle ctx,
		void* buffer,
		size_t buflen,
		nuint32 dsiget,
		void* dst) {
	Buf_T buff;
	nuint32 le32;
	nuint32 dsiflags;
	NWDSCCODE err;
	
	if (dsiget & (dsiget - 1))
		return ERR_BAD_KEY;
	if (!buffer)
		return ERR_NULL_POINTER;
	NWDSSetupBuf(&buff, buffer, buflen);
	err = NWDSBufGetLE32(&buff, &dsiflags);
	if (err)
		return err;
	if ((dsiflags & dsiget) != dsiget)
		return ERR_BAD_KEY;
	if (!dst)
		return ERR_NULL_POINTER;
	if (dsiget & DSI_OUTPUT_FIELDS) {
		*(nuint32*)dst = dsiflags;
		return 0;
	}
	if (dsiget & DSI_ENTRY_ID) {
		err = NWDSBufGetID(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_ENTRY_ID)
		NWDSBufGetSkip(&buff, 4);
		
	if (dsiget & DSI_ENTRY_FLAGS) {
		err = NWDSBufGetLE32(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_ENTRY_FLAGS)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_SUBORDINATE_COUNT) {
		err = NWDSBufGetLE32(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_SUBORDINATE_COUNT)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_MODIFICATION_TIME) {
		err = NWDSBufGetLE32(&buff, &le32);
		if (err)
			return err;
		*(time_t*)dst = le32;
		return 0;
	}
	if (dsiflags & DSI_MODIFICATION_TIME)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_MODIFICATION_TIMESTAMP) {
		void* p;
		
		p = NWDSBufGetPtr(&buff, 8);
		if (err)
			return err;
		((TimeStamp_T*)dst)->wholeSeconds = DVAL_LH(p, 0);
		((TimeStamp_T*)dst)->replicaNum = WVAL_LH(p, 4);
		((TimeStamp_T*)dst)->eventID = WVAL_LH(p, 6);
		return 0;
	}
	if (dsiflags & DSI_MODIFICATION_TIMESTAMP)
		NWDSBufGetSkip(&buff, 8);

	if (dsiget & DSI_CREATION_TIMESTAMP) {
		void* p;
		
		p = NWDSBufGetPtr(&buff, 8);
		if (err)
			return err;
		((TimeStamp_T*)dst)->wholeSeconds = DVAL_LH(p, 0);
		((TimeStamp_T*)dst)->replicaNum = WVAL_LH(p, 4);
		((TimeStamp_T*)dst)->eventID = WVAL_LH(p, 6);
		return 0;
	}
	if (dsiflags & DSI_CREATION_TIMESTAMP)
		NWDSBufGetSkip(&buff, 8);
	
	if (dsiget & DSI_PARTITION_ROOT_ID) {
		err = NWDSBufGetID(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_PARTITION_ROOT_ID)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_PARENT_ID) {
		err = NWDSBufGetID(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_PARENT_ID)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_REVISION_COUNT) {
		err = NWDSBufGetLE32(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_REVISION_COUNT)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_REPLICA_TYPE) {
		err = NWDSBufGetLE32(&buff, dst);
		return err;
	}
	if (dsiflags & DSI_REPLICA_TYPE)
		NWDSBufGetSkip(&buff, 4);

	if (dsiget & DSI_BASE_CLASS) {
		err = NWDSBufCtxString(ctx, &buff, dst, MAX_SCHEMA_NAME_BYTES, NULL);
		return err;
	}
	if (dsiflags & DSI_BASE_CLASS) {
		err = NWDSBufSkipCIString(&buff);
		if (err)
			return err;
	}

	if (dsiget & DSI_ENTRY_RDN) {
		err = NWDSBufCtxString(ctx, &buff, dst, MAX_RDN_BYTES, NULL);
		return err;
	}
	if (dsiflags & DSI_ENTRY_RDN) {
		err = NWDSBufSkipCIString(&buff);
		if (err)
			return err;
	}
	
	if (dsiget & DSI_ENTRY_DN) {
		err = NWDSBufCtxDN(ctx, &buff, dst, NULL);
		return err;
	}
	if (dsiflags & DSI_ENTRY_DN) {
		err = NWDSBufSkipCIString(&buff);
		if (err)
			return err;
	}

	if (dsiget & DSI_PARTITION_ROOT_DN) {
		err = NWDSBufCtxDN(ctx, &buff, dst, NULL);
		return err;
	}
	if (dsiflags & DSI_PARTITION_ROOT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}

	if (dsiget & DSI_PARENT_DN) {
		err = NWDSBufCtxDN(ctx, &buff, dst, NULL);
		return err;
	}
	if (dsiflags & DSI_PARENT_DN) {
		err = NWDSBufSkipCIString(buffer);
		if (err)
			return err;
	}

	if (dsiget & DSI_PURGE_TIME) {
		err = NWDSBufGetLE32(&buff, &le32);
		if (err)
			return err;
		*(time_t*)dst = le32;
		return 0;
	}
	if (dsiflags & DSI_PURGE_TIME)
		NWDSBufGetSkip(buffer, 4);

	/* DSI_DEREFERENCE_BASE_CLASSES */

	if (dsiget & DSI_REPLICA_NUMBER) {
		err = NWDSBufGetLE32(&buff, &le32);
		return err;
	}
	if (dsiflags & DSI_REPLICA_NUMBER)
		NWDSBufGetSkip(buffer, 4);

	if (dsiget & DSI_REPLICA_STATE) {
		err = NWDSBufGetLE32(&buff, &le32);
		return err;
	}
	if (dsiflags & DSI_REPLICA_STATE)
		NWDSBufGetSkip(buffer, 4);
	return ERR_BUFFER_EMPTY; /* we did not find requested item... */
}

NWDSCCODE NWDSGetAttrName(NWDSContextHandle ctx, Buf_T* buffer,
		NWDSChar* name, NWObjectCount* valcount, 
		enum SYNTAX *syntaxID) {
	NWDSCCODE err;
	nuint32 le32;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ:
		case DSV_SEARCH:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (buffer->cmdFlags & CMDFLAGS_SYNTAXID) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
	} else
		le32 = SYN_UNKNOWN;
	if (syntaxID)
		*syntaxID = le32;
	err = NWDSGetAttrVal_XX_STRING(ctx, buffer, name, MAX_SCHEMA_NAME_BYTES);
	if (err)
		return err;
	if (buffer->cmdFlags & CMDFLAGS_VALCOUNT) {
		err = NWDSBufGetLE32(buffer, &le32);
		if (err)
			return err;
	} else
		le32 = 0;
	if (valcount)
		*valcount = le32;
	return 0;
}

NWDSCCODE NWDSGetAttrValFlags(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		nuint32* flags) {
	NWDSCCODE err;
	nuint32 le32;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ:
		case DSV_SEARCH:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (!(buffer->cmdFlags & CMDFLAGS_FLAGS))
		return ERR_BAD_VERB;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	if (flags)
		*flags = le32;
	return 0;
}

NWDSCCODE NWDSGetAttrValModTime(UNUSED(NWDSContextHandle ctx), Buf_T* buffer,
		TimeStamp_T* stamp) {
	NWDSCCODE err;
	nuint32 le32;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ:
		case DSV_SEARCH:
		case DSV_READ_ATTR_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (!(buffer->cmdFlags & CMDFLAGS_STAMP))
		return ERR_BAD_VERB;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	if (stamp)
		stamp->wholeSeconds = le32;
	err = NWDSBufGetLE32(buffer, &le32);
	if (err)
		return err;
	if (stamp) {
		stamp->replicaNum = le32 & 0xFFFF;
		stamp->eventID = le32 >> 16;
	}
	return 0;
}

NWDSCCODE NWDSGetServerName(NWDSContextHandle ctx, Buf_T* buffer,
		NWDSChar* name, NWObjectCount* partcount) {
	NWDSCCODE err;
	nuint32 le32;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_LIST_PARTITIONS:
			break;
		default:
			return ERR_BAD_VERB;
	}
	err = NWDSBufCtxDN(ctx, buffer, name, NULL);
	if (err)
		return err;
	err = NWDSBufGetLE32(buffer, &le32);
	if (partcount)
		*partcount = le32;
	return 0;
}

static nuint32 tmp32;

NWDSCCODE NWDSInitBuf(UNUSED(NWDSContextHandle ctx), nuint operation,
		Buf_T* buffer) {
	NWDSCCODE err;

	NWDSBufStartPut(buffer, operation);
	buffer->bufFlags |= NWDSBUFT_INPUT;
	buffer->bufFlags &= ~NWDSBUFT_OUTPUT;
	switch (operation) {
		case DSV_READ:
		case DSV_SEARCH:
		case DSV_COMPARE:
		case DSV_ADD_ENTRY:
		case DSV_MODIFY_ENTRY:
		case DSV_READ_ATTR_DEF:
		case DSV_READ_CLASS_DEF:
		case DSV_MODIFY_CLASS_DEF:
		case DSV_READ_SYNTAXES:
			buffer->attrCountPtr = buffer->curPos;
			err = NWDSBufPutLE32(buffer, 0);
			if (err)
				return err;
			break;
		case DSV_SEARCH_FILTER:
			buffer->attrCountPtr = (nuint8*)&tmp32;
			break;
		case DSV_DEFINE_CLASS:
			break;
	}
	return 0;
}

NWDSCCODE NWDSPutAttrName(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* name) {
	NWDSCCODE err;
	void* bpos;

	if (!buffer || !name)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_OUTPUT)
		return ERR_BAD_VERB;
	switch (buffer->operation) {
		case DSV_READ:
		case DSV_SEARCH:
		case DSV_COMPARE:
		case DSV_ADD_ENTRY:
		case DSV_SEARCH_FILTER:
		case DSV_READ_ATTR_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	if (!buffer->attrCountPtr)
		return ERR_BAD_VERB;
	bpos = buffer->curPos;
	err = NWDSPutAttrVal_XX_STRING(ctx, buffer, name);
	if (err)
		return err;
	if ((buffer->operation == DSV_COMPARE) ||
	    (buffer->operation == DSV_ADD_ENTRY)) {
		void *vcp = buffer->curPos;
		err = NWDSBufPutLE32(buffer, 0);
		if (err) {
			buffer->curPos = bpos;
			return err;
		}
		buffer->valCountPtr = vcp;
	} else if (buffer->operation == DSV_SEARCH_FILTER) {
		buffer->valCountPtr = (nuint8*)&tmp32;
	} else
		buffer->valCountPtr = NULL;
	DSET_LH(buffer->attrCountPtr, 0, DVAL_LH(buffer->attrCountPtr, 0) + 1);
	return 0;
}

NWDSCCODE NWDSPutChange(NWDSContextHandle ctx, Buf_T* buffer,
		nuint changeType, const NWDSChar* name) {
	NWDSCCODE err;
	void* bpos;
	
	if (!buffer || !name)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_OUTPUT)
		return ERR_BAD_VERB;
	if ((buffer->operation != DSV_MODIFY_ENTRY))
		return ERR_BAD_VERB;
	if (!buffer->attrCountPtr)
		return ERR_BAD_VERB;
	bpos = buffer->curPos;
	err = NWDSBufPutLE32(buffer, changeType);
	if (err)
		goto errq;
	err = NWDSPutAttrVal_XX_STRING(ctx, buffer, name);
	if (err)
		goto errq;
	if ((changeType != DS_REMOVE_ATTRIBUTE) &&
	    (changeType != DS_CLEAR_ATTRIBUTE)) {
		void* vcp = buffer->curPos;
		err = NWDSBufPutLE32(buffer, 0);
		if (err)
			goto errq;
		buffer->valCountPtr = vcp;
	} else
		buffer->valCountPtr = NULL;
	DSET_LH(buffer->attrCountPtr, 0, DVAL_LH(buffer->attrCountPtr, 0) + 1);
	return 0;
errq:;
	buffer->curPos = bpos;
	return err;
}

NWDSCCODE NWDSPutAttrVal(NWDSContextHandle ctx, Buf_T* buffer,
		enum SYNTAX syntaxID, const void* attrVal) {
	NWDSCCODE err;
	
	if (!buffer || !attrVal)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_OUTPUT)
		return ERR_BAD_VERB;
	if (!buffer->valCountPtr)
		return ERR_BAD_VERB;
	switch (syntaxID) {
		case SYN_DIST_NAME:
			err = NWDSPutAttrVal_DIST_NAME(ctx, buffer, attrVal);
			break;
		case SYN_NET_ADDRESS:
			err = NWDSPutAttrVal_NET_ADDRESS(ctx, buffer, attrVal);
			break;
		case SYN_CI_STRING:
		case SYN_CE_STRING:
		case SYN_NU_STRING:
		case SYN_PR_STRING:
		case SYN_CLASS_NAME:
		case SYN_TEL_NUMBER:
			err = NWDSPutAttrVal_XX_STRING(ctx, buffer, attrVal);
			break;
		case SYN_OCTET_STRING:
		case SYN_STREAM:
			err = NWDSPutAttrVal_OCTET_STRING(ctx, buffer, attrVal);
			break;
		case SYN_BOOLEAN:
			err = NWDSPutAttrVal_BOOLEAN(ctx, buffer, attrVal);
			break;
		case SYN_COUNTER:
		case SYN_INTEGER:
		case SYN_INTERVAL:
			err = NWDSPutAttrVal_INTEGER(ctx, buffer, attrVal);
			break;
		case SYN_TIMESTAMP:
			err = NWDSPutAttrVal_TIMESTAMP(ctx, buffer, attrVal);
			break;
		case SYN_REPLICA_POINTER:
			err = NWDSPutAttrVal_REPLICA_POINTER(ctx, buffer, attrVal);
			break;
		case SYN_OBJECT_ACL:
			err = NWDSPutAttrVal_OBJECT_ACL(ctx, buffer, attrVal);
			break;
		case SYN_PATH:
			err = NWDSPutAttrVal_PATH(ctx, buffer, attrVal);
			break;
		case SYN_TIME:
			err = NWDSPutAttrVal_TIME(ctx, buffer, attrVal);
			break;
		case SYN_CI_LIST:
			err = NWDSPutAttrVal_CI_LIST(ctx, buffer, attrVal);
			break;
		case SYN_TYPED_NAME:
			err = NWDSPutAttrVal_TYPED_NAME(ctx, buffer, attrVal);
			break;
		case SYN_BACK_LINK:
			err = NWDSPutAttrVal_BACK_LINK(ctx, buffer, attrVal);
			break;
		case SYN_FAX_NUMBER:
			err = NWDSPutAttrVal_FAX_NUMBER(ctx, buffer, attrVal);
			break;
		case SYN_EMAIL_ADDRESS:
			err = NWDSPutAttrVal_EMAIL_ADDRESS(ctx, buffer, attrVal);
			break;
		case SYN_PO_ADDRESS:
			err = NWDSPutAttrVal_PO_ADDRESS(ctx, buffer, attrVal);
			break;
		case SYN_OCTET_LIST:
			err = NWDSPutAttrVal_OCTET_LIST(ctx, buffer, attrVal);
			break;
		case SYN_HOLD:
			err = NWDSPutAttrVal_HOLD(ctx, buffer, attrVal);
			break;
		default:
			err = ERR_NO_SUCH_SYNTAX;
			break;
	}
	if (err)
		return err;
	DSET_LH(buffer->valCountPtr, 0, DVAL_LH(buffer->valCountPtr, 0) + 1);
	return 0;
}

NWDSCCODE NWDSPutAttrNameAndVal(NWDSContextHandle ctx, Buf_T* buffer,
		const NWDSChar* name, enum SYNTAX syntaxID, 
		const void* attrVal) {
	NWDSCCODE err;
	void* curpos;
	void* vcp;
	nuint32 ac;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (!buffer->attrCountPtr)
		return ERR_BAD_VERB;
	ac = DVAL_LH(buffer->attrCountPtr, 0);
	curpos = buffer->curPos;
	vcp = buffer->valCountPtr;
	err = NWDSPutAttrName(ctx, buffer, name);
	if (err)
		return err;
	err = NWDSPutAttrVal(ctx, buffer, syntaxID, attrVal);
	if (err) {
		buffer->curPos = curpos;
		buffer->valCountPtr = vcp;
		DSET_LH(buffer->attrCountPtr, 0, ac);
	}
	return err;
}

NWDSCCODE NWDSPutChangeAndVal(NWDSContextHandle ctx, Buf_T* buffer,
		nuint changeType, const NWDSChar* name,
		enum SYNTAX syntaxID, const void* attrVal) {
	NWDSCCODE err;
	void* curpos;
	void* vcp;
	nuint32 ac;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (!buffer->attrCountPtr)
		return ERR_BAD_VERB;
	ac = DVAL_LH(buffer->attrCountPtr, 0);
	curpos = buffer->curPos;
	vcp = buffer->valCountPtr;
	err = NWDSPutChange(ctx, buffer, changeType, name);
	if (err)
		return err;
	err = NWDSPutAttrVal(ctx, buffer, syntaxID, attrVal);
	if (err) {
		buffer->curPos = curpos;
		buffer->valCountPtr = vcp;
		DSET_LH(buffer->attrCountPtr, 0, ac);
	}
	return err;
}

NWDSCCODE NWDSCanonicalizeName(NWDSContextHandle ctx, const NWDSChar* src,
		NWDSChar* dst) {
	wchar_t srcW[MAX_DN_CHARS+1];
	wchar_t dstW[MAX_DN_CHARS+1];
	NWDSCCODE err;
	
	err = NWDSXlateFromCtx(ctx, srcW, sizeof(srcW), src);
	if (err)
		return err;
	err = NWDSCanonicalizeNameW(ctx, srcW, dstW);
	if (err)
		return err;
	return NWDSXlateToCtx(ctx, dst, MAX_DN_BYTES, dstW, NULL);
}

NWDSCCODE NWDSAbbreviateName(NWDSContextHandle ctx, const NWDSChar* src,
		NWDSChar* dst) {
	wchar_t srcW[MAX_DN_CHARS+1];
	wchar_t dstW[MAX_DN_CHARS+1];
	NWDSCCODE err;
	
	err = NWDSXlateFromCtx(ctx, srcW, sizeof(srcW), src);
	if (err)
		return err;
	err = NWDSAbbreviateNameW(ctx, srcW, dstW);
	if (err)
		return err;
	return NWDSXlateToCtx(ctx, dst, MAX_DN_BYTES, dstW, NULL);
}

NWDSCCODE NWDSRemoveAllTypes(NWDSContextHandle ctx, const NWDSChar* src,
		NWDSChar* dst) {
	wchar_t srcW[MAX_DN_CHARS+1];
	wchar_t dstW[MAX_DN_CHARS+1];
	NWDSCCODE err;
	
	err = NWDSXlateFromCtx(ctx, srcW, sizeof(srcW), src);
	if (err)
		return err;
	err = NWDSRemoveAllTypesW(ctx, srcW, dstW);
	if (err)
		return err;
	return NWDSXlateToCtx(ctx, dst, MAX_DN_BYTES, dstW, NULL);
}

static NWDSCCODE __NWDSResolvePrep(
		NWDSContextHandle ctx,
		u_int32_t version,
		u_int32_t flag,
		const NWDSChar* name,
		Buf_T* rq,
		int wchar_name) {
	NWDSCCODE err;
	void* p;
	
	p = NWDSBufPutPtr(rq, 12);
	if (!p)
		return ERR_BUFFER_FULL;
	DSET_LH(p, 0, version);
	DSET_LH(p, 4, flag);
	DSET_LH(p, 8, ctx->dck.confidence);
	switch (wchar_name) {
		case 1:
			err = NWDSBufPutCIString(rq, (const wchar_t*)name);
			break;
		case 2:
			err = NWDSBufPutUnicodeString(rq, (const unicode*)name);
			break;
		default:
			err = NWDSPutAttrVal_DIST_NAME(ctx, rq, name);
			break;
	}
	if (err)
		return err;
	err = NWDSBufPutLE32(rq, ctx->dck.transports);
	if (err)
		return err;
	err = NWDSBufPut(rq, ctx->dck.transport_types, ctx->dck.transports * sizeof(nuint32));
	if (err)
		return err;
	err = NWDSBufPutLE32(rq, ctx->dck.transports);
	if (err)
		return err;
	err = NWDSBufPut(rq, ctx->dck.transport_types, ctx->dck.transports * sizeof(nuint32));
	if (err)
		return err;
	return 0;
}

static inline NWDSCCODE __NWDSResolveNameInt(
		UNUSED(NWDSContextHandle ctx), 
		NWCONN_HANDLE conn, 
		Buf_T* reply, 
		Buf_T* rq) {
	NWDSCCODE err;
	void* p;
	size_t rpl_len;

	NWDSBufStartPut(reply, DSV_RESOLVE_NAME);
	p = NWDSBufPutPtrLen(reply, &rpl_len);
	err = ncp_send_nds_frag(conn, DSV_RESOLVE_NAME, 
			rq->data, rq->curPos-rq->data, 
			p, rpl_len, &rpl_len);
	if (!err) {
		if (rpl_len < 8) {
			err = ERR_INVALID_SERVER_RESPONSE;
		} else {
			NWDSBufPutSkip(reply, rpl_len);
		}
	}
	NWDSBufFinishPut(reply);
	return err;
}

NWDSCCODE NWDSResolveNameInt(
		NWDSContextHandle ctx, 
		NWCONN_HANDLE conn, 
		u_int32_t version, 
		u_int32_t flag, 
		const NWDSChar* name, 
		Buf_T* reply) {
	NWDSCCODE err;
	Buf_T* rq;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &rq);
	if (err)
		return err;
	err = __NWDSResolvePrep(ctx, version, flag, name, rq, 0);	
	if (err)
		return err;
	err = __NWDSResolveNameInt(ctx, conn, reply, rq);
	NWDSFreeBuf(rq);
	return err;
}
		
/* server oriented NDS calls */

NWDSCCODE NWDSMapNameToID(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		const NWDSChar* name, NWObjectID* ID) {
	NWDSCCODE err;
	Buf_T* rp;
	
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &rp);
	if (err)
		return err;
	err = NWDSResolveNameInt(ctx, conn, DS_RESOLVE_V0, DS_RESOLVE_CREATE_ID | DS_RESOLVE_ENTRY_ID, name, rp);
	if (!err) {
		nuint32 replytype;
		
		err = NWDSBufGetLE32(rp, &replytype);
		if (!err) {
			/* we requested local entry */
			if (replytype != DS_RESOLVE_REPLY_LOCAL_ENTRY)
				err = ERR_INVALID_SERVER_RESPONSE;
			else {
				err = NWDSBufGetID(rp, ID);
			}
		}
	}
	NWDSFreeBuf(rp);
	return err;
}

NWDSCCODE NWDSMapIDToName(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWObjectID ID, NWDSChar* name) {
	NWDSCCODE err;
	Buf_T* b;
	nuint32 replyFmt = 0;
	nuint32 dsiFmt = DSI_ENTRY_DN;
	nuint32 val;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_TYPELESS_NAMES)
		replyFmt |= 1;
	if (val & DCV_DEREF_BASE_CLASS)
		dsiFmt |= DSI_DEREFERENCE_BASE_CLASS;
	/* DCV_DEREF_ALIASES does not have DSI_ counterpart */
	/* DCV_DISALLOW_REFERRALS is N/A for MapIDToName (server centric) */
	/* DCV_ASYNC_MODE: TODO (if someone knows...) */
	replyFmt |= ctx->dck.name_form;
	err = NWDSAllocBuf(MAX_DN_BYTES, &b);
	if (err)
		return err;
	err = NWDSGetDSIRaw(conn, dsiFmt, replyFmt, ID, b);
	if (!err) {
		/* Netware DS 6.02, 7.26, 7.28 and 7.30 (all I tested)
		 * has problem that [Self] maps to 0x040000FF, but
		 * 0x040000FF maps to [Self][Inheritance Mask]. Do you
		 * think that I should add hack to map [Self][Inheritance Mask]
		 * here? Or is it meant in another way and I oversight
		 * something? */
		err = NWDSGetAttrVal_DIST_NAME(ctx, b, name);
	}
	NWDSFreeBuf(b);
	return err;
}

static NWDSCCODE NWDSResolveName99(
		NWDSContextHandle ctx, 
		NWCONN_HANDLE conn,
		Buf_T* rq,
		NWCONN_HANDLE* resconn, 
		NWObjectID* ID, 
		Buf_T** rplbuf, 
		void*** array, 
		unsigned int* saddr, 
		int* idepth,
		const unicode** alias) {
	NWDSCCODE err;
	Buf_T* rp;

	*rplbuf = NULL;
	*array = NULL;
	*saddr = 0;
	*resconn = NULL;
	*alias = NULL;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &rp);
	if (err)
		return err;
	err = __NWDSResolveNameInt(ctx, conn, rp, rq);
	if (!err) {
		nuint32 replytype;
		nuint32 dummy;
			
		err = NWDSBufGetLE32(rp, &replytype);
		if (!err) switch (replytype) {
			case DS_RESOLVE_REPLY_LOCAL_ENTRY:
				dfprintf(stderr, "Local Entry\n");
				err = NWDSBufGetID(rp, ID);
				if (err)
					break;
				ncp_conn_use(conn);
				*resconn = conn;
				break;
			case DS_RESOLVE_REPLY_REMOTE_ENTRY:
				dfprintf(stderr, "Remote Entry\n");
				err = NWDSBufGetID(rp, ID);
				if (err)
					break;
				err = NWDSBufGetLE32(rp, &dummy);
				if (err)
					break;
				err = NWDSFindConnection2(ctx, resconn, rp, NWDSFINDCONN_CREATEALLOWED);
				break;
			case DS_RESOLVE_REPLY_ALIAS:
				dfprintf(stderr, "Alias\n");
				err = NWDSBufUnicodeString(rp, alias, NULL);
				if (err)
					break;
				*rplbuf = rp;
				return 0;
			case DS_RESOLVE_REPLY_REFERRAL_AND_ENTRY:
			case DS_RESOLVE_REPLY_REFERRAL:
				{
					u_int32_t depth;
					u_int32_t servercount;
					u_int32_t parsedaddresses;
					void** addresses;
					unsigned int pos;
							
					if (replytype == DS_RESOLVE_REPLY_REFERRAL) {
						dfprintf(stderr, "Referrals\n");
						err = NWDSBufGetLE32(rp, &depth);
						if (err)
							break;
					} else {
						NWObjectID objid;
								
						dfprintf(stderr, "Referrals + ID\n");
						err = NWDSBufGetLE32(rp, &dummy);
						if (err)
							break;
						err = NWDSBufGetID(rp, &objid);
						if (err)
							break;
						if (objid != (NWObjectID)~0) {
							*ID = objid;
							ncp_conn_use(conn);
							*resconn = conn;
							dfprintf(stderr, "Got ID\n");
							break;
						}
						depth = 0xFFFF0000; /* ignore all referrals */
					}
					err = NWDSBufGetLE32(rp, &servercount);
					if (err)
						break;
					if (servercount > 1024) { /* some unreasonable value */
						/* return ERR_TOO_MANY_REFERRALS; ??? */
						servercount = 1024;
					}
					if (!servercount) {
						err = ERR_NO_REFERRALS;
						break;
					}
					dfprintf(stderr, "%d servers\n", servercount);
					addresses = (void**)malloc(sizeof(void*) * servercount);
					if (!addresses) {
						err = ERR_NOT_ENOUGH_MEMORY;
						break;
					}
					parsedaddresses = 0;
					for (pos = 0; pos < servercount; pos++) {
						u_int32_t caddr;
						u_int32_t len;
						void* data;
						u_int32_t i;
									
						addresses[parsedaddresses] = NWDSBufPeekPtr(rp);
						err = NWDSBufGetLE32(rp, &caddr);
						if (err)
							break;
						for (i = 0; i < caddr; i++) {
							err = NWDSBufGetLE32(rp, &dummy);
							if (err)
								goto packetEOF;
							err = NWDSBufGetLE32(rp, &len);
							if (err)
								goto packetEOF;
							data = NWDSBufGetPtr(rp, len);
							if (!data) {
								err = ERR_BUFFER_EMPTY;
								goto packetEOF;
							}
						}
						parsedaddresses++;
					}
				packetEOF:;
					if (err) {
						free(addresses);
						break;
					}
					if (!parsedaddresses) {
						free(addresses);
						err = ERR_NO_REFERRALS;
						break;
					}
					*rplbuf = rp;
					*array = addresses;
					*saddr = parsedaddresses;
					*idepth = depth;
				}
				return 0;
			default:
				err = ERR_INVALID_SERVER_RESPONSE;
				dfprintf(stderr, "Invalid server response\n");
				break;
		}
	}
	NWDSFreeBuf(rp);
	return err;
}

static inline int __NWDSTransportError(NWDSCCODE err) {
	return err == ERR_TRANSPORT_FAILURE ||
	       err == ERR_ALL_REFERRALS_FAILED ||
	       err == ERR_NO_REFERRALS ||
	       err == ERR_REMOTE_FAILURE ||
	       err == ERR_UNREACHABLE_SERVER ||
	       err == EIO;
}

static NWDSCCODE __NWDSResolveName2(
		NWDSContextHandle ctx, 
		Buf_T* rq,
		NWCONN_HANDLE* resconn, 
		NWObjectID* ID) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	Buf_T* rp;
	void** addresses;
	unsigned int servers;
	int depth;
	const unicode* alias;
	int alias_level = 0;
		
	err = __NWDSGetConnection(ctx, &conn);
	if (err)
		return err;
restart:;
	err = NWDSResolveName99(ctx, conn, rq, resconn, ID, &rp, &addresses, &servers, &depth, &alias);
	if (err) {
		if (__NWDSTransportError(err)) {
			struct NWDSConnList* cnl;
			NWCONN_HANDLE cn2;
			NWDSCCODE tmperr;
			u_int32_t type;

			tmperr = NWDSListConnectionInit(ctx, &cnl);
			if (tmperr)
				goto quit;
			while ((tmperr = NWDSListConnectionNext(ctx, cnl, &cn2)) == 0) {
				if (cn2 != conn) {
					err = NWDSResolveName99(ctx, cn2, rq, resconn, ID, &rp, &addresses, &servers, &depth, &alias);
					if (!err) {
						NWCCCloseConn(conn);
						conn = cn2;
						NWDSListConnectionEnd(ctx, cnl);
						goto foundStep1;
					}
					if (!__NWDSTransportError(err)) {
					    	NWCCCloseConn(cn2);
						NWDSListConnectionEnd(ctx, cnl);
						goto quit;
					}
				}
				NWCCCloseConn(cn2);
			}
			NWDSListConnectionEnd(ctx, cnl);
			type = DVAL_LH(rq->data, 4);
			if (!(type & DS_RESOLVE_WALK_TREE)) {
				DSET_LH(rq->data, 4, type | DS_RESOLVE_WALK_TREE);
				goto restart;
			}
		}
	} else {
foundStep1:;
	if (rp) {
		unsigned int pos;
		unsigned int crmode;
		NWDSCCODE dserr = ERR_ALL_REFERRALS_FAILED;

		if (alias) {
alias:;
			NWDSBufStartPut(rq, DSV_RESOLVE_NAME);
			dserr = __NWDSResolvePrep(ctx, DVAL_LH(rq->data, 0),
				DVAL_LH(rq->data, 4), (const NWDSChar*)alias, 
				rq, 2);
			NWDSFreeBuf(rp);
			if (dserr)
				goto quit;
			alias_level++;
			/* Novell code allows only one level aliases... */
			if (alias_level < 64)
				goto restart;
			dserr = ERR_ALIAS_OF_AN_ALIAS;
			goto quit;
		}
again:;
		dfprintf(stderr, "Received referrals\n");
		crmode = NWDSFINDCONN_NOCREATE;
again2:;
		for (pos = 0; pos < servers; pos++) {
			NWCONN_HANDLE conn2;
			Buf_T* buf;
			Buf_T* b2;
			void** addr2;
			unsigned int serv2;
			int depth2;
									
			if (!addresses[pos])
				continue;
			err = NWDSCreateBuf(&buf, addresses[pos], 0x7FFFFFF);
			if (err)
				continue;
			err = NWDSFindConnection2(ctx, &conn2, buf, crmode);
			NWDSFreeBuf(buf);
			if (err)
				continue;
			dfprintf(stderr, "Processing referrals\n");
			addresses[pos] = NULL;
			err = NWDSResolveName99(ctx, conn2, rq, resconn, ID, &b2, &addr2, &serv2, &depth2, &alias);
			if (!err) {
				if (!b2) {
					NWCCCloseConn(conn2);
					goto freeExit;
				}
				if (alias) {
					free(addresses);
					NWDSFreeBuf(rp);
					rp = b2;
					NWCCCloseConn(conn2);
					goto alias;
				}
				if (depth2 < depth) {
					free(addresses);
					addresses = addr2;
					NWDSFreeBuf(rp);
					rp = b2;
					depth = depth2;
					servers = serv2;
					NWCCCloseConn(conn);
					conn = conn2;
					goto again;
				}
				dfprintf(stderr, "Referral ignored; %d >= %d\n", depth2, depth);
				free(addr2);
				NWDSFreeBuf(b2);
				err = dserr;
			} else
				dserr = err;
			NWCCCloseConn(conn2);
		}
		if (crmode == NWDSFINDCONN_NOCREATE) {
			dfprintf(stderr, "Connection not found, making new\n");
			crmode = NWDSFINDCONN_CREATEALLOWED;
			goto again2;
		}
freeExit:;
		free(addresses);
		NWDSFreeBuf(rp);
	}
	}
quit:;
	NWDSConnectionFinished(ctx, conn);
	return err;
}

NWDSCCODE NWDSResolveName2DR(
		NWDSContextHandle ctx,
		const NWDSChar* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
		
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	err = __NWDSResolvePrep(ctx, DS_RESOLVE_V0, flag, name, &rq, 0);
	if (err)
		return err;
	return __NWDSResolveName2(ctx, &rq, resconn, ID);
}

NWDSCCODE __NWDSResolveName2u(
		NWDSContextHandle ctx,
		const unicode* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;

	flag &= ~(DS_RESOLVE_DEREF_ALIASES);	
	if (ctx->dck.flags & DCV_DEREF_ALIASES)
		flag |= DS_RESOLVE_DEREF_ALIASES;
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	err = __NWDSResolvePrep(ctx, DS_RESOLVE_V0, flag, (const NWDSChar*)name, &rq, 2);
	if (err)
		return err;
	return __NWDSResolveName2(ctx, &rq, resconn, ID);
}

NWDSCCODE __NWDSResolveName2w(
		NWDSContextHandle ctx,
		const wchar_t* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID
) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;

	flag &= ~(DS_RESOLVE_DEREF_ALIASES);	
	if (ctx->dck.flags & DCV_DEREF_ALIASES)
		flag |= DS_RESOLVE_DEREF_ALIASES;
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	err = __NWDSResolvePrep(ctx, DS_RESOLVE_V0, flag, (const NWDSChar*)name, &rq, 1);
	if (err)
		return err;
	return __NWDSResolveName2(ctx, &rq, resconn, ID);
}

NWDSCCODE NWDSResolveName2(
		NWDSContextHandle ctx, 
		const NWDSChar* name, 
		u_int32_t flag,
		NWCONN_HANDLE* resconn, 
		NWObjectID* ID) {
	flag &= ~(DS_RESOLVE_DEREF_ALIASES);
	if (ctx->dck.flags & DCV_DEREF_ALIASES)
		flag |= DS_RESOLVE_DEREF_ALIASES;
	return NWDSResolveName2DR(ctx, name, flag, resconn, ID);
}

NWDSCCODE __NWDSResolveName2p(
		NWDSContextHandle ctx,
		const NWDSChar* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID,
		wchar_t* childName) {
	NWDSCCODE err;
	wchar_t parentName[MAX_DN_CHARS+1];
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	
	err = NWDSSplitName(ctx, name, parentName, childName);
	if (err)
		return err;
	if (ctx->dck.flags & DCV_DEREF_ALIASES)
		flag |= DS_RESOLVE_DEREF_ALIASES;
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	err = __NWDSResolvePrep(ctx, DS_RESOLVE_V0, flag, (NWDSChar*)parentName, &rq, 1);
	if (err)
		return err;
	return __NWDSResolveName2(ctx, &rq, resconn, ID);
}

NWDSCCODE NWDSResolveName(NWDSContextHandle ctx, const NWDSChar* name,
		NWCONN_HANDLE* resconn, NWObjectID* ID) {
	return NWDSResolveName2(ctx, name, DS_RESOLVE_WRITEABLE,
		resconn, ID);
}

static NWDSCCODE NWDSGetServerNameAddress(NWCONN_HANDLE conn, u_int32_t version,
	u_int32_t nameform, Buf_T* reply) {
	NWDSCCODE err;
	unsigned char rq[8];
	size_t rpl_len;
	void* rpl;
	
	DSET_LH(rq, 0, version);
	DSET_LH(rq, 4, nameform);
	NWDSBufStartPut(reply, DSV_GET_SERVER_ADDRESS);
	rpl = NWDSBufPutPtrLen(reply, &rpl_len);
	err = ncp_send_nds_frag(conn, DSV_GET_SERVER_ADDRESS, rq, sizeof(rq), rpl,
		rpl_len, &rpl_len);
	if (!err) {
		if (rpl_len < 8) {
			/* at least 0 bytes name and 0 network addresses */
			err = ERR_INVALID_SERVER_RESPONSE;
		} else {
			NWDSBufPutSkip(reply, rpl_len);
			NWDSBufFinishPut(reply);
		}
	}
	return err;
}

/* NWSDK returns abbreviated typeless name of server ... */
NWDSCCODE NWDSGetServerDN(NWDSContextHandle ctx, NWCONN_HANDLE conn, NWDSChar* name) {
	NWDSCCODE err;
	Buf_T* reply;
	u_int32_t replyFmt;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &reply);
	if (err)
		return err;
	replyFmt = ctx->dck.name_form;
	if (ctx->dck.flags & DCV_TYPELESS_NAMES)
		replyFmt |= 1;
	/* COMPATIBILITY
	replyFmt |= 1;
	*/
	err = NWDSGetServerNameAddress(conn, 0, replyFmt, reply);
	if (!err) {
		err = NWDSGetAttrVal_DIST_NAME(ctx, reply, name);
		/* COMPATIBILITY
		if (!err)
			err = NWDSAbbreviateName(ctx, name, name);
		*/
	}
	NWDSFreeBuf(reply);
	return err;
}

static NWDSCCODE NWDSGetServerAddressInt(NWCONN_HANDLE conn, 
		NWObjectCount* addrcnt,
		Buf_T* naddrs) {
	NWDSCCODE err;
	Buf_T* reply;
	
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &reply);
	if (err)
		return err;
	/* version: 0, format: partial dot, typeless */
	err = NWDSGetServerNameAddress(conn, 0, 1, reply);
	if (!err) {
		err = NWDSBufSkipCIString(reply);
		if (!err) {
			nuint32 cnt;
			
			err = NWDSBufGetLE32(reply, &cnt);
			if (!err) {
				if (addrcnt)
					*addrcnt = cnt;
				if (naddrs) {
					NWDSBufStartPut(naddrs, DSV_GET_SERVER_ADDRESS);
					/* copy addresses... maybe we can 
					   copy whole rest of buffer
					   as unstructured data block */
					while (cnt--) {
						nuint32 val;
						nuint32 size;
						void* ptr;

						/* address type */
						err = NWDSBufGetLE32(reply, &val);
						if (err) break;
						/* address length */
						err = NWDSBufGetLE32(reply, &size);
						if (err) break;

						err = NWDSBufPutLE32(naddrs, size + 8);
						if (err) break;
						err = NWDSBufPutLE32(naddrs, val);
						if (err) break;
						err = NWDSBufPutLE32(naddrs, size);
						if (err) break;

						/* address value */
						ptr = NWDSBufGetPtr(reply, size);
						if (!ptr) {
							err = ERR_BUFFER_EMPTY;
							break;
						}
						err = NWDSBufPut(naddrs, ptr, size);
						if (err)
							break;
					}
					NWDSBufFinishPut(naddrs);
				}
			}
		}
	}
	NWDSFreeBuf(reply);
	return err;
}

NWDSCCODE NWDSGetServerAddress(NWDSContextHandle ctx, NWCONN_HANDLE conn, 
		NWObjectCount* addrcnt,
		Buf_T* naddrs) {
	NWDSCCODE err;

	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	return NWDSGetServerAddressInt(conn, addrcnt, naddrs);
}

/* difference? */
NWDSCCODE NWDSGetServerAddress2(NWDSContextHandle ctx, NWCONN_HANDLE conn, 
		NWObjectCount* addrcnt,
		Buf_T* naddrs) {
	return NWDSGetServerAddress(ctx, conn, addrcnt, naddrs);
}

static NWDSCCODE __NWCCGetServerAddressPtr(NWCONN_HANDLE conn, 
		NWObjectCount* count, nuint8** data) {
	if (!conn->serverAddresses.buffer) {
		NWDSCCODE err;
		Buf_T* bb;
		NWObjectCount cnt;
		
		err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &bb);
		if (err)
			return err;
		err = NWDSGetServerAddressInt(conn, &cnt, bb);
		if (err)
			return err;
		/* TODO: lock connection */
		if (conn->serverAddresses.buffer) {
			NWDSFreeBuf(bb);
		} else {
			conn->serverAddresses.buffer = bb;
			conn->serverAddresses.count = cnt;
		}
		/* unlock connection */
	}
	*count = conn->serverAddresses.count;
	*data = NWDSBufPeekPtr(conn->serverAddresses.buffer);
	return 0;
}

NWDSCCODE __NWDSCompare(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		NWObjectID objectID,
		Buf_T* buffer,
		nbool8* matched) {
	NWDSCCODE err;
	unsigned char rq[8192];
	size_t rpl_len;
	unsigned char rpl[4];
	size_t pos;
	nuint32 ctxflags;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->operation != DSV_COMPARE)
		return ERR_BAD_VERB;
	err = NWDSGetContext(ctx, DCK_FLAGS, &ctxflags);
	if (err)
		return err;
	DSET_LH(rq, 0, 0); 		/* version */
	DSET_HL(rq, 4, objectID);
	pos = 8;
	{
		size_t len = buffer->curPos - buffer->data;
		memcpy(rq+pos, buffer->data, len);
		pos += ROUNDPKT(len);
	}
	err = ncp_send_nds_frag(conn, DSV_COMPARE, rq, pos, rpl, sizeof(rpl), &rpl_len);
	if (!err) {
		if (matched) {
			*matched = BVAL(rpl, 0) != 0;
		}
	}
	return err;
}

NWDSCCODE NWDSCompare(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		Buf_T* buffer,
		nbool8* matched) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->operation != DSV_COMPARE)
		return ERR_BAD_VERB;
	err = NWDSResolveName2(ctx, objectName, DS_RESOLVE_READABLE, 
		&conn, &objID);
	if (err)
		return err;
	err = __NWDSCompare(ctx, conn, objID, buffer, matched);
	NWCCCloseConn(conn);
	return err;
}

/* qflags:
   0x00000001 -> return typeless names
   0x00000002 -> return containers only
   0x00000004 \  return name format
   0x00000008 /
   0x00000010
   0x00000020
   0x00000040 -> deref base class
*/
NWDSCCODE __NWDSReadObjectDSIInfo(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		NWObjectID id,
		nuint32 dsiFmt,
		Buf_T* b) {
	NWDSCCODE err;
	nuint32 replyFmt = 0;
	nuint32 val;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &val);
	if (err)
		return err;
	if (val & DCV_TYPELESS_NAMES)
		replyFmt |= 1;
	if (val & DCV_DEREF_BASE_CLASS)
		dsiFmt |= DSI_DEREFERENCE_BASE_CLASS;
	replyFmt |= ctx->dck.name_form;
	return NWDSGetDSIRaw(conn, dsiFmt, replyFmt, id, b);
}

NWDSCCODE __NWDSReadObjectInfo(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		NWObjectID id,
		NWDSChar* distname,
		Object_Info_T* oi) {
	NWDSCCODE err;
	char buffer[8192];
	Buf_T b;
	
	NWDSSetupBuf(&b, buffer, sizeof(buffer));
	err = __NWDSReadObjectDSIInfo(ctx, conn, id, DSI_OUTPUT_FIELDS | 
		DSI_ENTRY_FLAGS | DSI_SUBORDINATE_COUNT | 
		DSI_MODIFICATION_TIME | DSI_BASE_CLASS | DSI_ENTRY_DN, &b);
	if (err)
		return err;
	err = NWDSGetObjectName(ctx, &b, distname, NULL, oi);
	return err;
}

NWDSCCODE NWDSReadObjectInfo(NWDSContextHandle ctx, const NWDSChar* name,
		NWDSChar* distname, Object_Info_T* oi) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	
	err = NWDSResolveName2(ctx, name, DS_RESOLVE_READABLE,
		&conn, &objID);
	if (err)
		return err;
	err = __NWDSReadObjectInfo(ctx, conn, objID, distname, oi);
	NWCCCloseConn(conn);
	return err;
}

NWDSCCODE NWDSReadObjectDSIInfo(NWDSContextHandle ctx, const NWDSChar* name,
		size_t len, void* buffer) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	Buf_T b;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	err = NWDSResolveName2(ctx, name, DS_RESOLVE_READABLE,
		&conn, &objID);
	if (err)
		return err;
	NWDSSetupBuf(&b, buffer, len);
	err = __NWDSReadObjectDSIInfo(ctx, conn, objID, ctx->dck.dsi_flags, &b);
	NWCCCloseConn(conn);
	return err;
}

NWDSCCODE __NWDSCloseIterationV0(
		NWCONN_HANDLE	conn,
		nuint32		iterHandle,
		nuint32		verb) {
	unsigned char rq[12];
	size_t rpl_len;
	unsigned char rpl[512];
	
	DSET_LH(rq, 0, 0);		/* version */
	DSET_LH(rq, 4, iterHandle);	/* iterHandle */
	DSET_LH(rq, 8, verb);		/* verb */
	return ncp_send_nds_frag(conn, DSV_CLOSE_ITERATION, rq, sizeof(rq), rpl, sizeof(rpl), &rpl_len);
}

NWDSCCODE NWDSCloseIteration(NWDSContextHandle ctx, nuint32 iterHandle,
		nuint32 verb) {
	NWCONN_HANDLE conn;
	NWDSCCODE err;
	struct wrappedIterationHandle* ih;
	
	switch (verb) {
		case DSV_ADD_ENTRY:
			err = NWDSGetContext(ctx, DCK_LAST_CONNECTION, &conn);
			if (err)
				return err;
			return __NWDSCloseIterationV0(conn, iterHandle, verb);
	}
	ih = __NWDSIHLookup(iterHandle, verb);
	if (!ih)
		return ERR_INVALID_HANDLE;
	err = __NWDSIHDelete(ih);
	free(ih);
	return err;
}

static NWDSCCODE __NWDSAddObjectV0(
		NWCONN_HANDLE conn,
		nuint32 flags,
		NWObjectID objID,
		const wchar_t* childName,
		Buf_T* buffer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	void* p;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 12);		
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_HL(rq_b, 8, objID);	/* parentID */
	err = NWDSBufPutCIString(&rq, childName);
	if (err)
		return err;
	p = NWDSBufRetrieve(buffer, &l);
	err = NWDSBufPut(&rq, p, l);
	if (err)
		return err;
	err = ncp_send_nds_frag(conn, DSV_ADD_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	return err;
}

static NWDSCCODE __NWDSAddObjectV2(
		NWCONN_HANDLE conn,
		nuint32 flags,
		nuint32* iterHandle,
		NWObjectID objID,
		const wchar_t* childName,
		Buf_T* buffer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	void* p;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 16);		
	DSET_LH(rq_b, 0, 2); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_LH(rq_b, 8, iterHandle?*iterHandle:~0U);
	DSET_HL(rq_b, 12, objID);	/* parentID */
	err = NWDSBufPutCIString(&rq, childName);
	if (err)
		return err;
	p = NWDSBufRetrieve(buffer, &l);
	err = NWDSBufPut(&rq, p, l);
	if (err)
		return err;
	err = ncp_send_nds_frag(conn, DSV_ADD_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	if (err)
		return err;
	if (l < 4) {
		if (iterHandle)
			*iterHandle = NO_MORE_ITERATIONS;
	} else {
		if (iterHandle)
			*iterHandle = DVAL_LH(rp_b, 0);
	}
	return 0;
}

NWDSCCODE NWDSAddObject(
		NWDSContextHandle ctx, 
		const NWDSChar* objectName,
		nuint32* iterHandle, 
		nbool8 moreIter, 
		Buf_T* buffer) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	wchar_t childName[MAX_DN_CHARS+1];
	
	if (moreIter && !iterHandle)
		return ERR_NULL_POINTER;
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_OUTPUT)
		return ERR_BAD_VERB;
	if (buffer->operation != DSV_ADD_ENTRY)
		return ERR_BAD_VERB;
	err = __NWDSResolveName2p(ctx, objectName, DS_RESOLVE_WRITEABLE,
		&conn, &objID, childName);
	if (err)
		return err;
	err = __NWDSAddObjectV2(conn, moreIter?1:0, iterHandle, objID, childName, buffer);
	if ((err == ERR_INVALID_API_VERSION) && !moreIter && (!iterHandle || *iterHandle == NO_MORE_ITERATIONS))
		err = __NWDSAddObjectV0(conn, 0, objID, childName, buffer);
	NWCCCloseConn(conn);
	return err;
}

static NWDSCCODE __NWDSRemoveObjectV0(
		NWCONN_HANDLE conn,
		NWObjectID objID) {
	NWDSCCODE err;
	char rq_b[8];
	size_t l;
	char rp_b[16];
	
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_HL(rq_b, 4, objID);	/* object ID */
	err = ncp_send_nds_frag(conn, DSV_REMOVE_ENTRY, rq_b, sizeof(rq_b),
		rp_b, sizeof(rp_b), &l);
	return err;
}

NWDSCCODE NWDSRemoveObject(
		NWDSContextHandle ctx,
		const NWDSChar* objectName) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	
	err = NWDSResolveName2DR(ctx, objectName, DS_RESOLVE_WRITEABLE,
		&conn, &objID);
	if (err)
		return err;
	err = __NWDSRemoveObjectV0(conn, objID);
	NWCCCloseConn(conn);
	return err;
}

static NWDSCCODE __NWDSModifyObjectV0(
		NWCONN_HANDLE conn,
		nuint32 flags,
		NWObjectID objID,
		Buf_T* buffer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	void* p;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 12);		
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_HL(rq_b, 8, objID);	/* parentID */
	p = NWDSBufRetrieve(buffer, &l);
	err = NWDSBufPut(&rq, p, l);
	if (err)
		return err;
	err = ncp_send_nds_frag(conn, DSV_MODIFY_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	return err;
}

static NWDSCCODE __NWDSModifyObjectV2(
		NWCONN_HANDLE conn,
		nuint32 flags,
		nuint32* iterHandle,
		NWObjectID objID,
		Buf_T* buffer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	void* p;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 16);		
	DSET_LH(rq_b, 0, 2); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_LH(rq_b, 8, iterHandle?*iterHandle:~0U);
	DSET_HL(rq_b, 12, objID);	/* parentID */
	p = NWDSBufRetrieve(buffer, &l);
	err = NWDSBufPut(&rq, p, l);
	if (err)
		return err;
	err = ncp_send_nds_frag(conn, DSV_MODIFY_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	if (err)
		return err;
	if (l < 4) {
		if (iterHandle)
			*iterHandle = NO_MORE_ITERATIONS;
	} else {
		if (iterHandle)
			*iterHandle = DVAL_LH(rp_b, 0);
	}
	return 0;
}

NWDSCCODE NWDSModifyObject(
		NWDSContextHandle ctx, 
		const NWDSChar* objectName,
		nuint32* iterHandle, 
		nbool8 moreIter, 
		Buf_T* buffer) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	nuint32 lh;
	struct wrappedIterationHandle* ih = NULL;

	if (moreIter && !iterHandle)
		return ERR_NULL_POINTER;
	if (!buffer)
		return ERR_NULL_POINTER;
	if (buffer->bufFlags & NWDSBUFT_OUTPUT)
		return ERR_BAD_VERB;
	if (buffer->operation != DSV_MODIFY_ENTRY)
		return ERR_BAD_VERB;
	if (iterHandle && *iterHandle != NO_MORE_ITERATIONS) {
		ih = __NWDSIHLookup(*iterHandle, DSV_MODIFY_ENTRY);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		objID = ih->objectID;
		lh = ih->iterHandle;
	} else {
		err = NWDSResolveName2DR(ctx, objectName, DS_RESOLVE_WRITEABLE,
			&conn, &objID);
		if (err)
			return err;
		lh = NO_MORE_ITERATIONS;
	}
	err = __NWDSModifyObjectV2(conn, moreIter?1:0, &lh, objID, buffer);
	if ((err == ERR_INVALID_API_VERSION) && !moreIter && (!iterHandle || *iterHandle == NO_MORE_ITERATIONS)) {
		lh = NO_MORE_ITERATIONS;
		err = __NWDSModifyObjectV0(conn, 0, objID, buffer);
	}
	if (ih)
		return __NWDSIHUpdate(ih, err, lh, iterHandle);
	return __NWDSIHCreate(err, conn, objID, lh, DSV_MODIFY_ENTRY, iterHandle);
}

static NWDSCCODE __NWDSModifyRDNV0(
		NWCONN_HANDLE conn,
		NWObjectID objID,
		nuint32 deleteOldRDN,
		const wchar_t* newRDN) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 12);		
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_HL(rq_b, 4, objID);	/* object ID */
	DSET_LH(rq_b, 8, deleteOldRDN);	/* delete old RDN */
	err = NWDSBufPutCIString(&rq, newRDN);
	if (err)
		return err;
	err = ncp_send_nds_frag(conn, DSV_MODIFY_RDN, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	return err;
}

NWDSCCODE NWDSModifyRDN(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const NWDSChar* newName,
		nuint deleteOldRDN) {
	wchar_t newParent[MAX_DN_CHARS+1];
	wchar_t newRDN[MAX_DN_CHARS+1];
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	
	if (!objectName || !newName)
		return ERR_NULL_POINTER;
	err = NWDSSplitName(ctx, newName, newParent, newRDN);
	if (err)
		return err;
	err = NWDSResolveName2DR(ctx, objectName, DS_RESOLVE_WRITEABLE,
		&conn, &objID);
	if (err)
		return err;
	err = __NWDSModifyRDNV0(conn, objID, deleteOldRDN, newRDN);
	NWCCCloseConn(conn);
	return err;
}

NWDSCCODE __NWDSGetServerDN(
		NWCONN_HANDLE conn,
		wchar_t* name,
		size_t maxlen) {
	char rp_b[DEFAULT_MESSAGE_LEN];
	Buf_T rp;
	NWDSCCODE err;
	
	NWDSSetupBuf(&rp, rp_b, sizeof(rp_b));
	err = NWDSGetServerNameAddress(conn, 0, 0, &rp);
	if (err)
		return err;
	return NWDSBufDN(&rp, name, maxlen);
}

NWDSCCODE __NWDSGetObjectDN(
		NWCONN_HANDLE conn,
		NWObjectID id,
		wchar_t* name,
		size_t maxlen) {
	char rp_b[DEFAULT_MESSAGE_LEN];
	Buf_T rp;
	NWDSCCODE err;
	
	NWDSSetupBuf(&rp, rp_b, sizeof(rp_b));
	err = NWDSGetDSIRaw(conn, DSI_ENTRY_DN, 0, id, &rp);
	if (err)
		return err;
	return NWDSBufDN(&rp, name, maxlen);
}

NWDSCCODE __NWDSGetObjectDNUnicode(
		NWCONN_HANDLE conn,
		NWObjectID id,
		unicode* name,
		size_t* len) {
	char rp_b[DEFAULT_MESSAGE_LEN];
	Buf_T rp;
	NWDSCCODE err;
	nuint32 rlen;
	
	NWDSSetupBuf(&rp, rp_b, sizeof(rp_b));
	err = NWDSGetDSIRaw(conn, DSI_ENTRY_DN, 0, id, &rp);
	if (err)
		return err;
	err = NWDSBufGetLE32(&rp, &rlen);
	if (err)
		return err;
	if (rlen > *len)
		return NWE_BUFFER_OVERFLOW;
	err = NWDSBufGet(&rp, name, rlen);
	if (err)
		return err;
	*len = rlen;
	return 0;
}

static NWDSCCODE __NWDSBeginMoveEntryV0(
		NWCONN_HANDLE dstConn,
		nuint32 flags,
		NWObjectID dstParentID,
		const wchar_t* newRDN,
		const wchar_t* srcServer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 12);
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_HL(rq_b, 8, dstParentID);	/* dst parent ID */
	err = NWDSBufPutCIString(&rq, newRDN);
	if (err)
		return err;
	err = NWDSBufPutCIString(&rq, srcServer);
	if (err)
		return err;
	err = ncp_send_nds_frag(dstConn, DSV_BEGIN_MOVE_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	return err;
}

static NWDSCCODE __NWDSFinishMoveEntryV0(
		NWCONN_HANDLE srcConn,
		nuint32 flags,
		NWObjectID srcID,
		NWObjectID dstParentID,
		const wchar_t* newRDN,
		const wchar_t* dstServer) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	size_t l;
	char rp_b[16];
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	NWDSBufPutPtr(&rq, 16);
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_LH(rq_b, 4, flags);	/* flags */
	DSET_HL(rq_b, 8, srcID);	/* source ID */
	DSET_HL(rq_b, 12, dstParentID);	/* dst parent ID */
	err = NWDSBufPutCIString(&rq, newRDN);
	if (err)
		return err;
	err = NWDSBufPutCIString(&rq, dstServer);
	if (err)
		return err;
	err = ncp_send_nds_frag(srcConn, DSV_FINISH_MOVE_ENTRY, rq.data, rq.curPos - rq.data,
		rp_b, sizeof(rp_b), &l);
	return err;
}

static const wchar_t* findDelim(const wchar_t* str, wint_t delim) {
	wint_t tmp;
	
	while ((tmp = *str++) != 0) {
		if (tmp == delim)
			return str;
		if (tmp == '\\') {
			if (!*str++)
				return NULL;
		}
	}
	return NULL;		
}

NWDSCCODE NWDSMoveObject(
		NWDSContextHandle ctx,
		const NWDSChar* srcObjectName,
		const NWDSChar* dstParentName,
		const NWDSChar* dstRDN) {
	NWCONN_HANDLE srcConn;
	NWObjectID srcObjID;
	NWCONN_HANDLE dstConn;
	NWObjectID dstObjID;
	NWDSCCODE err;
	wchar_t srcDN[MAX_DN_CHARS+1];
	wchar_t dstDN[MAX_DN_CHARS+1];
	wchar_t wRDN[MAX_RDN_CHARS+1];
	const wchar_t* srcParentDN;
	
	if (!srcObjectName || !dstParentName || !dstRDN)
		return ERR_NULL_POINTER;
	err = NWDSXlateFromCtx(ctx, wRDN, sizeof(wRDN), dstRDN);
	if (err)
		return err;
	err = NWDSResolveName2DR(ctx, srcObjectName, DS_RESOLVE_MASTER,
		&srcConn, &srcObjID);
	if (err)
		return err;
	err = NWDSResolveName2(ctx, dstParentName, DS_RESOLVE_MASTER,
		&dstConn, &dstObjID);
	if (err)
		goto err1;
	err = __NWDSGetObjectDN(srcConn, srcObjID, srcDN, sizeof(srcDN));
	if (err)
		goto error;
	err = __NWDSGetObjectDN(dstConn, dstObjID, dstDN, sizeof(dstDN));
	if (err)
		goto error;
	srcParentDN = findDelim(srcDN, '.');
	if (!srcParentDN)
		srcParentDN = L"[Root]";
	if (!wcscasecmp(srcParentDN, dstDN)) {
		err = ERR_RENAME_NOT_ALLOWED;
		goto error;
	}
	err = __NWDSGetServerDN(srcConn, srcDN, sizeof(srcDN));
	if (err)
		goto error;
	err = __NWDSGetServerDN(dstConn, dstDN, sizeof(dstDN));
	if (err)
		goto error;
	err = __NWDSBeginMoveEntryV0(dstConn, 0, dstObjID, wRDN, srcDN);
	if (err)
		goto error;
	err = __NWDSFinishMoveEntryV0(srcConn, 1, srcObjID, dstObjID, wRDN, dstDN);
error:;
	NWCCCloseConn(dstConn);
err1:;
	NWCCCloseConn(srcConn);
	return err;
}

NWDSCCODE NWDSModifyDN(
		NWDSContextHandle ctx,
		const NWDSChar* srcObjectName,
		const NWDSChar* dstObjectName,
		nuint deleteOldRDN) {
	NWCONN_HANDLE srcConn;
	NWObjectID srcObjID;
	NWCONN_HANDLE dstConn;
	NWObjectID dstObjID;
	NWDSCCODE err;
	wchar_t srcDN[MAX_DN_CHARS+1];
	wchar_t dstDN[MAX_DN_CHARS+1];
	wchar_t wRDN[MAX_DN_CHARS+1];
	const wchar_t* srcParentDN;
	
	
	if (!srcObjectName || !dstObjectName)
		return ERR_NULL_POINTER;
	if (deleteOldRDN)
		deleteOldRDN = 1;
	err = NWDSResolveName2DR(ctx, srcObjectName, DS_RESOLVE_WRITEABLE,
		&srcConn, &srcObjID);
	if (err)
		return err;
	err = __NWDSResolveName2p(ctx, dstObjectName, DS_RESOLVE_WRITEABLE,
		&dstConn, &dstObjID, wRDN);
	if (err)
		goto err1;
	err = __NWDSGetObjectDN(srcConn, srcObjID, srcDN, sizeof(srcDN));
	if (err)
		goto error;
	err = __NWDSGetObjectDN(dstConn, dstObjID, dstDN, sizeof(dstDN));
	if (err)
		goto error;
	srcParentDN = findDelim(srcDN, '.');
	if (!srcParentDN)
		srcParentDN = L"[Root]";
	if (!wcscasecmp(srcParentDN, dstDN)) {
		err = __NWDSModifyRDNV0(srcConn, srcObjID, deleteOldRDN, wRDN);
		goto error;
	}
	err = __NWDSGetServerDN(srcConn, srcDN, sizeof(srcDN));
	if (err)
		goto error;
	err = __NWDSGetServerDN(dstConn, dstDN, sizeof(dstDN));
	if (err)
		goto error;
	err = __NWDSBeginMoveEntryV0(dstConn, 0, dstObjID, wRDN, srcDN);
	if (err)
		goto error;
	err = __NWDSFinishMoveEntryV0(srcConn, deleteOldRDN, srcObjID, dstObjID, wRDN, dstDN);
error:;
	NWCCCloseConn(dstConn);
err1:;
	NWCCCloseConn(srcConn);
	return err;
}

NWDSCCODE __NWDSBeginLoginV0(
		NWCONN_HANDLE conn,
		NWObjectID objID,
		NWObjectID *p1,
		void *p2) {
	NWDSCCODE err;
	char rq_b[8];
	size_t l;
	char rp_b[16];
	
	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_HL(rq_b, 4, objID);	/* object ID */
	err = ncp_send_nds_frag(conn, DSV_BEGIN_LOGIN, rq_b, 8,
		rp_b, sizeof(rp_b), &l);
	if (err)
		return err;
	if (l < 8) {
		dfprintf(stderr, "Short reply (%u) in BeginLoginV0\n", l);
		return ERR_INVALID_SERVER_RESPONSE;
	}
	if (p1)
		*p1 = DVAL_HL(rp_b, 0);
	if (p2)
		memcpy(p2, rp_b+4, 4);
	return 0;
}

NWDSCCODE __NWDSFinishLoginV2(
		NWCONN_HANDLE conn,
		nuint32 flag,
		NWObjectID objID,
		Buf_T* rqb,
		nuint8 p1[8],
		Buf_T* rpb) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	Buf_T rq;
	void* p;
	void* q;
	size_t ln;
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	q = NWDSBufRetrieve(rqb, &ln);
	p = NWDSBufPutPtr(&rq, 16);
	DSET_LH(p, 0, 2);	/* version */
	DSET_LH(p, 4, flag);	/* flags */
	DSET_HL(p, 8, objID);	/* object ID */
	DSET_LH(p, 12, ln);
	err = NWDSBufPut(&rq, q, ln);
	if (err)
		return err;
	NWDSBufStartPut(rpb, DSV_FINISH_LOGIN);
	q = NWDSBufPutPtrLen(rpb, &ln);
	err = ncp_send_nds_frag(conn, DSV_FINISH_LOGIN, rq_b, rq.curPos - rq.data, q, ln, &ln);
	memset(rq_b, 0, sizeof(rq_b));
	if (!err || (err == NWE_PASSWORD_EXPIRED)) {
		NWDSBufPutSkip(rpb, ln);
		NWDSBufFinishPut(rpb);
		err = NWDSBufGet(rpb, p1, 8);
	}
	return err;
}

NWDSCCODE __NWDSBeginAuthenticationV0(
		NWCONN_HANDLE conn,
		NWObjectID objID,
		const nuint8 seed[4],
		nuint8 authid[4],
		Buf_T* rpb) {
	NWDSCCODE err;
	char rq_b[12];
	void* q;
	size_t ln;
	
	DSET_LH(rq_b, 0, 0);		/* version */
	DSET_HL(rq_b, 4, objID);	/* object ID */
	memcpy(rq_b + 8, seed, 4);	/* seed */
	NWDSBufStartPut(rpb, DSV_BEGIN_AUTHENTICATION);
	q = NWDSBufPutPtrLen(rpb, &ln);
	err = ncp_send_nds_frag(conn, DSV_BEGIN_AUTHENTICATION, rq_b, sizeof(rq_b), q, ln, &ln);
	if (!err) {
		NWDSBufPutSkip(rpb, ln);
		NWDSBufFinishPut(rpb);
		err = NWDSBufGet(rpb, authid, 4);
		if (!err) {
			nuint32 le32;
			
			err = NWDSBufGetLE32(rpb, &le32);
			if (!err) {
				if (!NWDSBufPeekPtrLen(rpb, 0, le32)) {
					dfprintf(stderr, "Short BeginAuthentication reply\n");
					err = ERR_INVALID_SERVER_RESPONSE;
				} else
					rpb->dataend = rpb->curPos + le32;
			}
		}
	}
	return err;
}

NWDSCCODE __NWDSFinishAuthenticationV0(
		NWCONN_HANDLE	conn,
		Buf_T*		md5_key,
		const void*	login_identity,
		size_t		login_identity_len,
		Buf_T*		auth_key) {
	NWDSCCODE err;
	char rq_b[DEFAULT_MESSAGE_LEN];
	char rp_b[16];		/* dummy, no data expected */
	Buf_T rq;
	void* p;
	void* q;
	size_t ln;
	
	NWDSSetupBuf(&rq, rq_b, sizeof(rq_b));
	q = NWDSBufRetrieve(md5_key, &ln);
	p = NWDSBufPutPtr(&rq, 8);
	DSET_LH(p, 0, 0);	/* version */
	DSET_LH(p, 4, ln);
	if (ln) {
		err = NWDSBufPut(&rq, q, ln);
		if (err)
			return err;
	}
	err = NWDSBufPutLE32(&rq, login_identity_len);
	if (err)
		return err;
	if (login_identity_len) {
		err = NWDSBufPut(&rq, login_identity, login_identity_len);
		if (err)
			return err;
	}
	q = NWDSBufRetrieve(auth_key, &ln);
	err = NWDSBufPutLE32(&rq, ln);
	if (err)
		return err;
	if (ln) {
		err = NWDSBufPut(&rq, q, ln);
		if (err)
			return err;
	}
	err = ncp_send_nds_frag(conn, DSV_FINISH_AUTHENTICATION, rq_b, rq.curPos - rq.data, rp_b, sizeof(rp_b), &ln);
	memset(rq_b, 0, sizeof(rq_b));
	return err;
}

NWDSCCODE NWDSGetObjectHostServerAddress(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		NWDSChar* serverName,
		Buf_T* netAddresses) {
	NWDSContextHandle tmp;
	NWDSCCODE err;
	Buf_T attrname;
	char attrname_b[DEFAULT_MESSAGE_LEN];
	Buf_T hostname;
	char hostname_b[DEFAULT_MESSAGE_LEN];
	wchar_t rattrname[MAX_DN_CHARS+1];
	nuint32 ih = NO_MORE_ITERATIONS;
	NWObjectCount cnt, valcnt;
	enum SYNTAX synt;
	
	err = NWDSDuplicateContextHandleInt(ctx, &tmp);
	if (err)
		return err;
	NWDSSetupBuf(&attrname, attrname_b, sizeof(attrname_b));
	NWDSSetupBuf(&hostname, hostname_b, sizeof(hostname_b));
	err = NWDSInitBuf(tmp, DSV_READ, &attrname);
	if (err)
		goto freectx;
	err = NWDSPutAttrName(tmp, &attrname, (const NWDSChar*)L"Host Server");
	if (err)
		goto freectx;
	/* use input context... */
	err = NWDSRead(ctx, objectName, DS_ATTRIBUTE_VALUES, 0, &attrname, &ih, &hostname);
	if (err)
		goto freectx;
	if (ih != NO_MORE_ITERATIONS)
		NWDSCloseIteration(ctx, ih, DSV_READ);
	err = NWDSGetAttrCount(ctx, &hostname, &cnt);
	if (err)
		goto freectx;
	if (cnt < 1) {
		err = ERR_BUFFER_EMPTY;
		goto freectx;
	}
	err = NWDSGetAttrName(tmp, &hostname, (NWDSChar*)rattrname, &valcnt, &synt);
	if (err)
		goto freectx;
	if (wcscmp(rattrname, L"Host Server") || (synt != SYN_DIST_NAME) || (valcnt < 1)) {
		err = ERR_SYSTEM_ERROR;
		goto freectx;
	}
	if (serverName) {
		void* cur = NWDSBufPeekPtr(&hostname);
		
		err = NWDSGetAttrVal(ctx, &hostname, synt, serverName);
		if (err)
			goto freectx;
		NWDSBufSeek(&hostname, cur);
	}
	if (netAddresses) {
		err = NWDSGetAttrVal(tmp, &hostname, synt, rattrname);
		if (err)
			goto freectx;
		err = NWDSInitBuf(tmp, DSV_READ, &attrname);
		if (err)
			goto freectx;
		err = NWDSPutAttrName(tmp, &attrname, (const NWDSChar*)L"Network Address");
		if (err)
			goto freectx;
		ih = NO_MORE_ITERATIONS;
		err = NWDSRead(tmp, (NWDSChar*)rattrname, DS_ATTRIBUTE_VALUES, 0, &attrname, &ih, netAddresses);
		if (err)
			goto freectx;
		if (ih != NO_MORE_ITERATIONS) {
			NWDSCloseIteration(ctx, ih, DSV_READ);
			err = ERR_BUFFER_FULL;
			goto freectx;
		}
	}
	err = 0;
freectx:
	NWDSFreeContext(tmp);
	return err;
}

NWDSCCODE NWDSOpenConnToNDSServer(
		NWDSContextHandle ctx,
		const NWDSChar* serverName,
		NWCONN_HANDLE* pconn) {
	NWDSContextHandle tmp;
	NWDSCCODE err;
	Buf_T attrname;
	char attrname_b[DEFAULT_MESSAGE_LEN];
	Buf_T hostaddr;
	char hostaddr_b[DEFAULT_MESSAGE_LEN];
	wchar_t rattrname[MAX_DN_CHARS+1];
	nuint32 ih = NO_MORE_ITERATIONS;
	NWObjectCount cnt, valcnt;
	enum SYNTAX synt;
	
	err = NWDSDuplicateContextHandleInt(ctx, &tmp);
	if (err)
		return err;
	NWDSSetupBuf(&attrname, attrname_b, sizeof(attrname_b));
	NWDSSetupBuf(&hostaddr, hostaddr_b, sizeof(hostaddr_b));
	err = NWDSInitBuf(tmp, DSV_READ, &attrname);
	if (err)
		goto freectx;
	err = NWDSPutAttrName(tmp, &attrname, (const NWDSChar*)L"Network Address");
	if (err)
		goto freectx;
	/* use input context... */
	err = NWDSRead(ctx, serverName, DS_ATTRIBUTE_VALUES, 0, &attrname, &ih, &hostaddr);
	if (err)
		goto freectx;
	if (ih != NO_MORE_ITERATIONS)
		NWDSCloseIteration(ctx, ih, DSV_READ);
	err = NWDSGetAttrCount(ctx, &hostaddr, &cnt);
	if (err)
		goto freectx;
	if (cnt < 1) {
		err = ERR_BUFFER_EMPTY;
		goto freectx;
	}
	err = NWDSGetAttrName(tmp, &hostaddr, (NWDSChar*)rattrname, &valcnt, &synt);
	if (err)
		goto freectx;
	if (wcscmp(rattrname, L"Network Address") || (synt != SYN_NET_ADDRESS) || (valcnt < 1)) {
		err = ERR_SYSTEM_ERROR;
		goto freectx;
	}
	err = NWDSFindConnection(ctx, pconn, valcnt, &hostaddr, NWDSFINDCONN_CREATEALLOWED | NWDSFINDCONN_DSREADBUF);
freectx:
	NWDSFreeContext(tmp);
	return err;
}

/*****************************   begin PP */

// PP V 1.14
// not sure that __NWDSGetConnection returns always an authenticated connection.
// I had problems when I was "ncpmounted" and called this from a program with a -S parameter
// pointing to another server
NWDSCCODE NWDSWhoAmI_Not_So_Good ( NWDSContextHandle ctx, NWDSChar * me );
NWDSCCODE NWDSWhoAmI_Not_So_Good ( NWDSContextHandle ctx, NWDSChar * me ) {
	NWDSCCODE dserr;
	NWCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID myID;

	err = NWDSIsContextValid(ctx);
	if (err)
		return ERR_BAD_CONTEXT;
	dserr = __NWDSGetConnection (ctx, &conn);
	if (dserr)
		return dserr;
	err = NWCCGetConnInfo(conn, NWCC_INFO_USER_ID, sizeof(myID), &myID);
	if (err)
		return err;
	return NWDSMapIDToName(ctx, conn, myID, me);
}

// seems to me a bit better ????
// scan all ds connections for a valid user_id and ask the corresponding server ?
NWDSCCODE NWDSWhoAmI ( NWDSContextHandle ctx, NWDSChar * me ) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	NWDS_HANDLE dsh;
	struct NWDSConnList cnl;

	dserr = NWDSIsContextValid(ctx);
	if (dserr)
		return ERR_BAD_CONTEXT;
	dsh=ctx->ds_connection;
	if (!dsh)
   		return ERR_NOT_LOGGED_IN;

	dserr = __NWDSListConnectionInit(dsh, &cnl);
	if (dserr)
		return dserr;
	while ((dserr = __NWDSListConnectionNext(&cnl, &conn)) == 0) {
		/* permanent connections mounted by ncpmount
		   and used later
		   are not flagged as CONN_AUTHENTICATED , nor CONN_LICENSED
		   they should be isn't it ??? so I cannot test as below .... */
		/* if (conn->connState & CONNECTION_AUTHENTICATED) */
		{
			NWObjectID myID;

			dserr = NWCCGetConnInfo(conn, NWCC_INFO_USER_ID, sizeof(myID), &myID);
			if (!dserr) {
				dserr = NWDSMapIDToName(ctx, conn, myID, me);
				NWCCCloseConn(conn);
				return dserr;
			}
		}
		NWCCCloseConn(conn);
	}
	__NWDSListConnectionEnd(&cnl);
	return ERR_NOT_LOGGED_IN;
}

/****  PP 02 TreeScaning routines ***********************/

struct TreeNode {
	struct TreeNode* left;
	struct TreeNode* right;
	struct TreeNode* next;
	struct TreeNode** pprev;
	size_t cnt;
	wchar_t name[MAX_TREE_NAME_CHARS+1];
};

struct TreeList {
	struct TreeNode* first;
	struct TreeNode* lin;
	struct TreeNode* curr;
	int dups;
	size_t uniqueTrees;
	size_t remainTrees;
};

static struct TreeList* __allocTree(int dups) {
	struct TreeList* t;
	
	t = (struct TreeList*)malloc(sizeof(*t));
	if (t) {
		t->first = NULL;
		t->lin = NULL;
		t->curr = NULL;
		t->dups = dups;
		t->uniqueTrees = 0;
	}
	return t;
}

static void __freeNode(struct TreeNode* n) {
	while (n) {
		struct TreeNode* tmp;

		__freeNode(n->left);
		tmp = n;
		n = n->right;
		free(tmp);
	}
}

static void __freeTree(struct TreeList* t) {
	if (t) {
		struct TreeNode* n = t->first;
		free(t);
		
		__freeNode(n);
	}
}

static NWDSCCODE __allocNode(struct TreeNode** pn, const wchar_t* tn) {
	struct TreeNode* n;
	size_t tnl;
	
	tnl = wcslen(tn);
	if (tnl > MAX_TREE_NAME_CHARS)
		return NWE_BUFFER_OVERFLOW;
	n = (struct TreeNode*)malloc(sizeof(*n));
	if (!n)
		return ERR_NOT_ENOUGH_MEMORY;
	n->left = n->right = NULL;
	n->cnt = 1;
	memcpy(n->name, tn, sizeof(*tn) * (tnl + 1));
	*pn = n;
	return 0;
}	

static NWDSCCODE __insertNode(struct TreeList* t, const wchar_t* tn) {
	struct TreeNode** p;
	struct TreeNode* n;
	NWDSCCODE err;

	p = &t->first;
	while ((n = *p) != NULL) {
		int cmp = wcscmp(tn, n->name);
		if (cmp < 0) {
			p = &n->left;
			if (!*p) {
				err = __allocNode(p, tn);
				if (!err) {
					struct TreeNode* q = *p;
					
					q->next = n;
					q->pprev = n->pprev;
					n->pprev = &q->next;
					*(q->pprev) = q;
					t->uniqueTrees++;
				}
				return err;
			}
		} else if (cmp > 0) {
			p = &n->right;
			if (!*p) {
				err = __allocNode(p, tn);
				if (!err) {
					struct TreeNode* q = *p;
					
					q->next = n->next;
					if (q->next)
						q->next->pprev = &q->next;
					n->next = q;
					q->pprev = &n->next;
					t->uniqueTrees++;
				}
				return err;
			}
		} else {
			if (t->dups)
				n->cnt++;
			return 0;
		}
	}
	err = __allocNode(p, tn);
	if (!err) {
		struct TreeNode* q = *p;
		
		t->lin = q;
		q->next = NULL;
		q->pprev = &t->lin;
		t->uniqueTrees++;
	}
	return err;
}

static NWDSCCODE __fillTree(struct TreeList* t, NWCONN_HANDLE conn, const char* filter) {
	struct ncp_bindery_object obj;
	nuint32 iterHandle = ~0;
	NWDSCCODE err;

	if (!filter || !filter[0])
		filter = "*";		
	while ((err = ncp_scan_bindery_object(conn, iterHandle, OT_TREE_NAME, filter, &obj)) == 0) {
		char* p = obj.object_name;
		if (strlen(p) >= MAX_TREE_NAME_CHARS) {
			wchar_t nm[MAX_TREE_NAME_CHARS + 1];
			wchar_t* dst;
			char* q;
			
			for (q = p + MAX_TREE_NAME_CHARS - 1; (*q == '_') && (q >= p); q--);
			dst = nm;
			while (p <= q) {
				/* ISO-8859-1... No excuse... If someone thinks that we
				   should use ctx's DCK_LOCAL_CHARSET, please tell me
				   why. PV */
				*dst++ = (*p++) & 0xFF;
			}
			*dst = 0;
			err = __insertNode(t, nm);
			if (err)
				break;
		}
		iterHandle = obj.object_id;
	}
	if (err == NWE_SERVER_UNKNOWN)
		err = 0;
	t->curr = t->lin;
	t->remainTrees = t->uniqueTrees;
	return err;
}

static NWDSCCODE __fetchTree(NWDSContextHandle ctx, struct TreeList* t, NWDSChar* tree, const wchar_t* upperBound) {
	struct TreeNode* n;
	NWDSCCODE err;
	
	n = t->curr;
	if (!n)
		return NWE_SERVER_UNKNOWN;
	if (upperBound && (wcscmp(n->name, upperBound) > 0))
		return NWE_SERVER_UNKNOWN;
	err = NWDSXlateToCtx(ctx, tree, MAX_TREE_NAME_BYTES, n->name, NULL);
	if (err)
		return err;
	if (!--n->cnt) {
		t->curr = n->next;
		t->remainTrees--;
	}
	return 0;
}

NWDSCCODE NWDSScanForAvailableTrees(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		const char*		scanFilter,
		nint32*			iterationHandle,
		NWDSChar*		treeName) {
	NWDSCCODE err;
	
	if (!treeName || !iterationHandle)
		return ERR_NULL_POINTER;
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	if (*iterationHandle == -1) {
		__freeTree(ctx->dck.tree_list);
		ctx->dck.tree_list = __allocTree(1);
		if (!ctx->dck.tree_list)
			return ERR_NOT_ENOUGH_MEMORY;
		err = __fillTree(ctx->dck.tree_list, conn, scanFilter);
		if (err) {
			__NWDSDestroyTreeList(&ctx->dck.tree_list);
			return err;
		}
	}
	err = __fetchTree(ctx, ctx->dck.tree_list, treeName, NULL);
	if (err) {
		*iterationHandle = 0;
		__NWDSDestroyTreeList(&ctx->dck.tree_list);
	} else {
		*iterationHandle = 1;
	}
	return err;
}

NWDSCCODE NWDSReturnBlockOfAvailableTrees(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		const char*		scanFilter,
		const void*		lastBlocksString,
		const NWDSChar*		endBoundString,
		nuint32			maxTreeNames,
		NWDSChar**		arrayOfNames,
		nuint32*		numberOfTrees,
		nuint32*		totalUniqueTrees) {
	NWDSCCODE err;
	struct TreeList* t;
	nuint32 ret;
	wchar_t endBoundW[MAX_TREE_NAME_CHARS+1];
	wchar_t* eb = NULL;

	if (maxTreeNames && !arrayOfNames)
		return ERR_NULL_POINTER;
	if (endBoundString) {
		err = NWDSXlateFromCtx(ctx, endBoundW, sizeof(endBoundW), endBoundString);
		if (err)
			return err;
		if (endBoundW[0])
			eb = endBoundW;
	} else {
		err = NWDSIsContextValid(ctx);
		if (err)
			return err;
	}
	t = ctx->dck.tree_list;
	if (lastBlocksString) {
		if (!t) {
			if (numberOfTrees)
				*numberOfTrees = 0;
			if (totalUniqueTrees)
				*totalUniqueTrees = 0;
			return 0;
		}
	} else {
		__freeTree(t);
		ctx->dck.tree_list = __allocTree(0);
		if (!ctx->dck.tree_list)
			return ERR_NOT_ENOUGH_MEMORY;
		err = __fillTree(ctx->dck.tree_list, conn, scanFilter);
		if (err) {
			__NWDSDestroyTreeList(&ctx->dck.tree_list);
			return err;
		}
	}
	if (totalUniqueTrees)
		*totalUniqueTrees = ctx->dck.tree_list->remainTrees;
	ret = 0;
	while (maxTreeNames--) {
		err = __fetchTree(ctx, ctx->dck.tree_list, *arrayOfNames++, eb);
		if (err)
			break;
		ret++;
	}
	if (numberOfTrees)
		*numberOfTrees = ret;	
	if (err) {
		__NWDSDestroyTreeList(&ctx->dck.tree_list);
		if (err == NWE_SERVER_UNKNOWN)
			err = 0;
	}
	return err;
}

static NWDSCCODE __NWDSScanConnsForTrees(
		UNUSED(NWDSContextHandle	ctx),
		struct TreeList*	t,
		NWCONN_HANDLE		conn) {
	wchar_t tname[MAX_TREE_NAME_CHARS+1];
	
	if (NWIsDSServerW(conn, tname)) {
		NWDSCCODE err;
		wchar_t* dst = tname;
			
		for (dst = tname + MAX_TREE_NAME_CHARS - 1; (*dst == L'_') && (dst >= tname); dst--);
		dst[1] = 0;
		err = __insertNode(t, tname);
		if (err)
			return err;
	}
	return 0;
}

NWDSCCODE NWDSScanConnsForTrees (
		NWDSContextHandle	ctx,
		nuint			numOfPtrs,
		nuint*			numOfTrees,
		NWDSChar**		treeBufPtrs){
	NWDSCCODE err;
	NWCONN_HANDLE conns[64];
	int maxEntries=64;
	int curEntries=0;
	int i;
	struct TreeList* t;

	if (numOfPtrs && !treeBufPtrs)
		return ERR_NULL_POINTER;

	t = __allocTree(0);
	if (!t)
		return ERR_NOT_ENOUGH_MEMORY;
	err = NWCXGetPermConnList(conns, maxEntries, &curEntries, getuid());
	if (!err) {
		NWCONN_HANDLE c = NULL;
		NWCONN_HANDLE c2;
		
		while (!ncp_next_conn(c, &c2)) {
			if (c)
				NWCCCloseConn(c);
			c = c2;
			err = __NWDSScanConnsForTrees(ctx, t, c);
			if (err)
				break;
		}
		if (c)
			NWCCCloseConn(c);
		t->curr = t->lin;
		t->remainTrees = t->uniqueTrees;
		if (!err) {
			while (numOfPtrs--) {
				err = __fetchTree(ctx, t, *treeBufPtrs++, NULL);
				if (err)
					break;
			}
			if (err == NWE_SERVER_UNKNOWN)
				err = 0;
			if (numOfTrees) {
				*numOfTrees = t->uniqueTrees;
			}
		}
		for (i = 0; i < curEntries; i++)
			NWCCCloseConn(conns[i]);
	}
	__freeTree(t);
	return err;
}

/***************** end tree scanning routines ********************/

// debugging and internal spying
// caller MUST allocate a BIG buffer for output
NWDSCCODE NWDSSpyConns(NWDSContextHandle ctx, NWDSChar * infos);
NWDSCCODE NWDSSpyConns(NWDSContextHandle ctx, NWDSChar * infos) {
	NWCCODE err;
	NWCONN_HANDLE conn;
	nuint32 myID;
	struct list_head *stop;
	struct list_head *current;
	NWDS_HANDLE dsh;
	char buffer[256],fsn[256],me[256];

	sprintf(infos,"connections:\n");
	err = NWDSIsContextValid(ctx);
	if (err)
		return ERR_BAD_CONTEXT;
	dsh=ctx->ds_connection;
	if (!dsh)
		return ERR_NOT_LOGGED_IN;
	// lets see what we have in the nds_ring....
	ncpt_mutex_lock(&nds_ring_lock);
	stop = &dsh->conns;
	for (current = stop->next; current != stop; current = current->next) {
		conn = list_entry(current, struct ncp_conn, nds_ring);
		NWCCGetConnInfo(conn,  NWCC_INFO_USER_ID ,sizeof(myID), &myID);
		NWCCGetConnInfo(conn,  NWCC_INFO_SERVER_NAME ,sizeof(fsn), fsn);
		NWCCGetConnInfo(conn,  NWCC_INFO_USER_NAME ,sizeof(me), me);
		sprintf (buffer,"state= %x,uid= %x,uid2=%x,serv=%s,usr=%s\t",conn->connState,conn->user_id,myID,fsn,me);
		strcat(infos,buffer);
	}
	ncpt_mutex_unlock(&nds_ring_lock);
	return 0;
}
