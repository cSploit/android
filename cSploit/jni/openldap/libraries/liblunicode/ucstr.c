/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <ac/bytes.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/stdlib.h>

#include <lber_pvt.h>

#include <ldap_utf8.h>
#include <ldap_pvt_uc.h>

#define	malloc(x)	ber_memalloc_x(x,ctx)
#define	realloc(x,y)	ber_memrealloc_x(x,y,ctx)
#define	free(x)		ber_memfree_x(x,ctx)

int ucstrncmp(
	const ldap_unicode_t *u1,
	const ldap_unicode_t *u2,
	ber_len_t n )
{
	for(; 0 < n; ++u1, ++u2, --n ) {
		if( *u1 != *u2 ) {
			return *u1 < *u2 ? -1 : +1;
		}
		if ( *u1 == 0 ) {
			return 0;
		}
	}
	return 0;
}

int ucstrncasecmp(
	const ldap_unicode_t *u1,
	const ldap_unicode_t *u2,
	ber_len_t n )
{
	for(; 0 < n; ++u1, ++u2, --n ) {
		ldap_unicode_t uu1 = uctolower( *u1 );
		ldap_unicode_t uu2 = uctolower( *u2 );

		if( uu1 != uu2 ) {
			return uu1 < uu2 ? -1 : +1;
		}
		if ( uu1 == 0 ) {
			return 0;
		}
	}
	return 0;
}

ldap_unicode_t * ucstrnchr(
	const ldap_unicode_t *u,
	ber_len_t n,
	ldap_unicode_t c )
{
	for(; 0 < n; ++u, --n ) {
		if( *u == c ) {
			return (ldap_unicode_t *) u;
		}
	}

	return NULL;
}

ldap_unicode_t * ucstrncasechr(
	const ldap_unicode_t *u,
	ber_len_t n,
	ldap_unicode_t c )
{
	c = uctolower( c );
	for(; 0 < n; ++u, --n ) {
		if( uctolower( *u ) == c ) {
			return (ldap_unicode_t *) u;
		}
	}

	return NULL;
}

void ucstr2upper(
	ldap_unicode_t *u,
	ber_len_t n )
{
	for(; 0 < n; ++u, --n ) {
		*u = uctoupper( *u );
	}
}

struct berval * UTF8bvnormalize(
	struct berval *bv,
	struct berval *newbv,
	unsigned flags,
	void *ctx )
{
	int i, j, len, clen, outpos, ucsoutlen, outsize, last;
	char *out, *outtmp, *s;
	ac_uint4 *ucs, *p, *ucsout;

	static unsigned char mask[] = {
		0, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

	unsigned casefold = flags & LDAP_UTF8_CASEFOLD;
	unsigned approx = flags & LDAP_UTF8_APPROX;

	if ( bv == NULL ) {
		return NULL;
	}

	s = bv->bv_val;
	len = bv->bv_len;

	if ( len == 0 ) {
		return ber_dupbv_x( newbv, bv, ctx );
	}

	if ( !newbv ) {
		newbv = ber_memalloc_x( sizeof(struct berval), ctx );
		if ( !newbv ) return NULL;
	}

	/* Should first check to see if string is already in proper
	 * normalized form. This is almost as time consuming as
	 * the normalization though.
	 */

	/* finish off everything up to character before first non-ascii */
	if ( LDAP_UTF8_ISASCII( s ) ) {
		if ( casefold ) {
			outsize = len + 7;
			out = (char *) ber_memalloc_x( outsize, ctx );
			if ( out == NULL ) {
				return NULL;
			}
			outpos = 0;

			for ( i = 1; (i < len) && LDAP_UTF8_ISASCII(s + i); i++ ) {
				out[outpos++] = TOLOWER( s[i-1] );
			}
			if ( i == len ) {
				out[outpos++] = TOLOWER( s[len-1] );
				out[outpos] = '\0';
				newbv->bv_val = out;
				newbv->bv_len = outpos;
				return newbv;
			}
		} else {
			for ( i = 1; (i < len) && LDAP_UTF8_ISASCII(s + i); i++ ) {
				/* empty */
			}

			if ( i == len ) {
				return ber_str2bv_x( s, len, 1, newbv, ctx );
			}
				
			outsize = len + 7;
			out = (char *) ber_memalloc_x( outsize, ctx );
			if ( out == NULL ) {
				return NULL;
			}
			outpos = i - 1;
			memcpy(out, s, outpos);
		}
	} else {
		outsize = len + 7;
		out = (char *) ber_memalloc_x( outsize, ctx );
		if ( out == NULL ) {
			return NULL;
		}
		outpos = 0;
		i = 0;
	}

	p = ucs = ber_memalloc_x( len * sizeof(*ucs), ctx );
	if ( ucs == NULL ) {
		ber_memfree_x(out, ctx);
		return NULL;
	}

	/* convert character before first non-ascii to ucs-4 */
	if ( i > 0 ) {
		*p = casefold ? TOLOWER( s[i-1] ) : s[i-1];
		p++;
	}

	/* s[i] is now first non-ascii character */
	for (;;) {
		/* s[i] is non-ascii */
		/* convert everything up to next ascii to ucs-4 */
		while ( i < len ) {
			clen = LDAP_UTF8_CHARLEN2( s + i, clen );
			if ( clen == 0 ) {
				ber_memfree_x( ucs, ctx );
				ber_memfree_x( out, ctx );
				return NULL;
			}
			if ( clen == 1 ) {
				/* ascii */
				break;
			}
			*p = s[i] & mask[clen];
			i++;
			for( j = 1; j < clen; j++ ) {
				if ( (s[i] & 0xc0) != 0x80 ) {
					ber_memfree_x( ucs, ctx );
					ber_memfree_x( out, ctx );
					return NULL;
				}
				*p <<= 6;
				*p |= s[i] & 0x3f;
				i++;
			}
			if ( casefold ) {
				*p = uctolower( *p );
			}
			p++;
		}
		/* normalize ucs of length p - ucs */
		uccompatdecomp( ucs, p - ucs, &ucsout, &ucsoutlen, ctx );
		if ( approx ) {
			for ( j = 0; j < ucsoutlen; j++ ) {
				if ( ucsout[j] < 0x80 ) {
					out[outpos++] = ucsout[j];
				}
			}
		} else {
			ucsoutlen = uccanoncomp( ucsout, ucsoutlen );
			/* convert ucs to utf-8 and store in out */
			for ( j = 0; j < ucsoutlen; j++ ) {
				/* allocate more space if not enough room for
				   6 bytes and terminator */
				if ( outsize - outpos < 7 ) {
					outsize = ucsoutlen - j + outpos + 6;
					outtmp = (char *) ber_memrealloc_x( out, outsize, ctx );
					if ( outtmp == NULL ) {
						ber_memfree_x( ucsout, ctx );
						ber_memfree_x( ucs, ctx );
						ber_memfree_x( out, ctx );
						return NULL;
					}
					out = outtmp;
				}
				outpos += ldap_x_ucs4_to_utf8( ucsout[j], &out[outpos] );
			}
		}

		ber_memfree_x( ucsout, ctx );
		ucsout = NULL;
		
		if ( i == len ) {
			break;
		}

		last = i;

		/* Allocate more space in out if necessary */
		if (len - i >= outsize - outpos) {
			outsize += 1 + ((len - i) - (outsize - outpos));
			outtmp = (char *) ber_memrealloc_x(out, outsize, ctx);
			if (outtmp == NULL) {
				ber_memfree_x( ucs, ctx );
				ber_memfree_x( out, ctx );
				return NULL;
			}
			out = outtmp;
		}

		/* s[i] is ascii */
		/* finish off everything up to char before next non-ascii */
		for ( i++; (i < len) && LDAP_UTF8_ISASCII(s + i); i++ ) {
			out[outpos++] = casefold ? TOLOWER( s[i-1] ) : s[i-1];
		}
		if ( i == len ) {
			out[outpos++] = casefold ? TOLOWER( s[len-1] ) : s[len-1];
			break;
		}

		/* convert character before next non-ascii to ucs-4 */
		*ucs = casefold ? TOLOWER( s[i-1] ) : s[i-1];
		p = ucs + 1;
	}

	ber_memfree_x( ucs, ctx );
	out[outpos] = '\0';
	newbv->bv_val = out;
	newbv->bv_len = outpos;
	return newbv;
}

/* compare UTF8-strings, optionally ignore casing */
/* slow, should be optimized */
int UTF8bvnormcmp(
	struct berval *bv1,
	struct berval *bv2,
	unsigned flags,
	void *ctx )
{
	int i, l1, l2, len, ulen, res = 0;
	char *s1, *s2, *done;
	ac_uint4 *ucs, *ucsout1, *ucsout2;

	unsigned casefold = flags & LDAP_UTF8_CASEFOLD;
	unsigned norm1 = flags & LDAP_UTF8_ARG1NFC;
	unsigned norm2 = flags & LDAP_UTF8_ARG2NFC;

	if (bv1 == NULL) {
		return bv2 == NULL ? 0 : -1;

	} else if (bv2 == NULL) {
		return 1;
	}

	l1 = bv1->bv_len;
	l2 = bv2->bv_len;

	len = (l1 < l2) ? l1 : l2;
	if (len == 0) {
		return l1 == 0 ? (l2 == 0 ? 0 : -1) : 1;
	}

	s1 = bv1->bv_val;
	s2 = bv2->bv_val;
	done = s1 + len;

	while ( (s1 < done) && LDAP_UTF8_ISASCII(s1) && LDAP_UTF8_ISASCII(s2) ) {
		if (casefold) {
			char c1 = TOLOWER(*s1);
			char c2 = TOLOWER(*s2);
			res = c1 - c2;
		} else {
			res = *s1 - *s2;
		}			
		s1++;
		s2++;
		if (res) {
			/* done unless next character in s1 or s2 is non-ascii */
			if (s1 < done) {
				if (!LDAP_UTF8_ISASCII(s1) || !LDAP_UTF8_ISASCII(s2)) {
					break;
				}
			} else if (((len < l1) && !LDAP_UTF8_ISASCII(s1)) ||
				((len < l2) && !LDAP_UTF8_ISASCII(s2)))
			{
				break;
			}
			return res;
		}
	}

	/* We have encountered non-ascii or strings equal up to len */

	/* set i to number of iterations */
	i = s1 - done + len;
	/* passed through loop at least once? */
	if (i > 0) {
		if (!res && (s1 == done) &&
		    ((len == l1) || LDAP_UTF8_ISASCII(s1)) &&
		    ((len == l2) || LDAP_UTF8_ISASCII(s2))) {
			/* all ascii and equal up to len */
			return l1 - l2;
		}

		/* rewind one char, and do normalized compare from there */
		s1--;
		s2--;
		l1 -= i - 1;
		l2 -= i - 1;
	}
			
	/* Should first check to see if strings are already in
	 * proper normalized form.
	 */
	ucs = malloc( ( ( norm1 || l1 > l2 ) ? l1 : l2 ) * sizeof(*ucs) );
	if ( ucs == NULL ) {
		return l1 > l2 ? 1 : -1; /* what to do??? */
	}
	
	/*
	 * XXYYZ: we convert to ucs4 even though -llunicode
	 * expects ucs2 in an ac_uint4
	 */
	
	/* convert and normalize 1st string */
	for ( i = 0, ulen = 0; i < l1; i += len, ulen++ ) {
		ucs[ulen] = ldap_x_utf8_to_ucs4( s1 + i );
		if ( ucs[ulen] == LDAP_UCS4_INVALID ) {
			free( ucs );
			return -1; /* what to do??? */
		}
		len = LDAP_UTF8_CHARLEN( s1 + i );
	}

	if ( norm1 ) {
		ucsout1 = ucs;
		l1 = ulen;
		ucs = malloc( l2 * sizeof(*ucs) );
		if ( ucs == NULL ) {
			free( ucsout1 );
			return l1 > l2 ? 1 : -1; /* what to do??? */
		}
	} else {
		uccompatdecomp( ucs, ulen, &ucsout1, &l1, ctx );
		l1 = uccanoncomp( ucsout1, l1 );
	}

	/* convert and normalize 2nd string */
	for ( i = 0, ulen = 0; i < l2; i += len, ulen++ ) {
		ucs[ulen] = ldap_x_utf8_to_ucs4( s2 + i );
		if ( ucs[ulen] == LDAP_UCS4_INVALID ) {
			free( ucsout1 );
			free( ucs );
			return 1; /* what to do??? */
		}
		len = LDAP_UTF8_CHARLEN( s2 + i );
	}

	if ( norm2 ) {
		ucsout2 = ucs;
		l2 = ulen;
	} else {
		uccompatdecomp( ucs, ulen, &ucsout2, &l2, ctx );
		l2 = uccanoncomp( ucsout2, l2 );
		free( ucs );
	}
	
	res = casefold
		? ucstrncasecmp( ucsout1, ucsout2, l1 < l2 ? l1 : l2 )
		: ucstrncmp( ucsout1, ucsout2, l1 < l2 ? l1 : l2 );
	free( ucsout1 );
	free( ucsout2 );

	if ( res != 0 ) {
		return res;
	}
	if ( l1 == l2 ) {
		return 0;
	}
	return l1 > l2 ? 1 : -1;
}
