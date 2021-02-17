/* phonetic.c - routines to do phonetic matching */
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
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/time.h>

#include "slap.h"

#if !defined(SLAPD_METAPHONE) && !defined(SLAPD_PHONETIC)
#define SLAPD_METAPHONE
#endif

#define iswordbreak(x)  (!isascii(x) || isspace((unsigned char) (x)) || \
			 ispunct((unsigned char) (x)) || \
			 isdigit((unsigned char) (x)) || (x) == '\0')

#if 0
static char *
first_word( char *s )
{
	if ( s == NULL ) {
		return( NULL );
	}

	while ( iswordbreak( *s ) ) {
		if ( *s == '\0' ) {
			return( NULL );
		} else {
			s++;
		}
	}

	return( s );
}

static char *
next_word( char *s )
{
	if ( s == NULL ) {
		return( NULL );
	}

	while ( ! iswordbreak( *s ) ) {
		s++;
	}

	while ( iswordbreak( *s ) ) {
		if ( *s == '\0' ) {
			return( NULL );
		} else {
			s++;
		}
	}

	return( s );
}

static char *
word_dup( char *w )
{
	char	*s, *ret;
	char	save;

	for ( s = w; !iswordbreak( *s ); s++ )
		;	/* NULL */
	save = *s;
	*s = '\0';
	ret = ch_strdup( w );
	*s = save;

	return( ret );
}
#endif /* 0 */

#ifndef MAXPHONEMELEN
#define MAXPHONEMELEN	4
#endif

#if defined(SLAPD_PHONETIC)

/* lifted from isode-8.0 */
char *
phonetic( char *s )
{
        char	code, adjacent, ch;
	char	*p;
        int	i;
	char	phoneme[MAXPHONEMELEN + 1];

        p = s;
        if (  p == NULL || *p == '\0' ) {
                return( NULL );
        }

        adjacent = '0';
	phoneme[0] = TOUPPER((unsigned char)*p);

	phoneme[1]  = '\0';
        for ( i = 0; i < 99 && (! iswordbreak(*p)); p++ ) {
		ch = TOUPPER ((unsigned char)*p);

                code = '0';

                switch (ch) {
                case 'B':
                case 'F':
		case 'P':
                case 'V':
                        code = (adjacent != '1') ? '1' : '0';
                        break;
                case 'S':
                case 'C':
                case 'G':
                case 'J':
                case 'K':
                case 'Q':
                case 'X':
                case 'Z':
                        code = (adjacent != '2') ? '2' : '0';
                        break;
                case 'D':
                case 'T':
                        code = (adjacent != '3') ? '3' : '0';
                        break;
                case 'L':
                        code = (adjacent != '4') ? '4' : '0';
                        break;
                case 'M':
                case 'N':
                        code = (adjacent != '5') ? '5' : '0';
                        break;
                case 'R':
                        code = (adjacent != '6') ? '6' : '0';
                        break;
                default:
                        adjacent = '0';
                }

                if ( i == 0 ) {
			adjacent = code;
			i++;
		} else if ( code != '0' ) {
			if ( i == MAXPHONEMELEN )
				break;
                        adjacent = phoneme[i] = code;
                        i++;
                }
        }

	if ( i > 0 )
		phoneme[i] = '\0';

        return( ch_strdup( phoneme ) );
}

#elif defined(SLAPD_METAPHONE)

/*
 * Metaphone was originally developed by Lawrence Philips and
 * published in the "Computer Language" magazine in 1990.
 */
/*
 * Metaphone copied from C Gazette, June/July 1991, pp 56-57,
 * author Gary A. Parker, with changes by Bernard Tiffany of the
 * University of Michigan, and more changes by Tim Howes of the
 * University of Michigan.
 */

/* Character coding array */
static const char  vsvfn[26] = {
	   1, 16, 4, 16, 9, 2, 4, 16, 9, 2, 0, 2, 2,
	/* A   B  C   D  E  F  G   H  I  J  K  L  M  */
	   2, 1, 4, 0, 2, 4, 4, 1, 0, 0, 0, 8, 0};
	/* N  O  P  Q  R  S  T  U  V  W  X  Y  Z  */

/* Macros to access character coding array */
#define vowel(x)        ((x) != '\0' && vsvfn[(x) - 'A'] & 1)	/* AEIOU */
#define same(x)         ((x) != '\0' && vsvfn[(x) - 'A'] & 2)	/* FJLMNR */
#define varson(x)       ((x) != '\0' && vsvfn[(x) - 'A'] & 4)	/* CGPST */
#define frontv(x)       ((x) != '\0' && vsvfn[(x) - 'A'] & 8)	/* EIY */
#define noghf(x)        ((x) != '\0' && vsvfn[(x) - 'A'] & 16)	/* BDH */

char *
phonetic( char *Word )
{
	char           *n, *n_start, *n_end;	/* pointers to string */
	char           *metaph_end;	/* pointers to metaph */
	char            ntrans[40];	/* word with uppercase letters */
	int             KSflag;	/* state flag for X -> KS */
	char		buf[MAXPHONEMELEN + 2];
	char		*Metaph;

	/*
	 * Copy Word to internal buffer, dropping non-alphabetic characters
	 * and converting to upper case
	 */

	for (n = ntrans + 4, n_end = ntrans + 35; !iswordbreak( *Word ) &&
	    n < n_end; Word++) {
		if (isalpha((unsigned char)*Word))
			*n++ = TOUPPER((unsigned char)*Word);
	}
	Metaph = buf;
	*Metaph = '\0';
	if (n == ntrans + 4) {
		return( ch_strdup( buf ) );		/* Return if null */
	}
	n_end = n;		/* Set n_end to end of string */

	/* ntrans[0] will always be == 0 */
	ntrans[0] = '\0';
	ntrans[1] = '\0';
	ntrans[2] = '\0';
	ntrans[3] = '\0';
	*n++ = 0;
	*n++ = 0;
	*n++ = 0;
	*n = 0;			/* Pad with nulls */
	n = ntrans + 4;		/* Assign pointer to start */

	/* Check for PN, KN, GN, AE, WR, WH, and X at start */
	switch (*n) {
	case 'P':
	case 'K':
	case 'G':
		/* 'PN', 'KN', 'GN' becomes 'N' */
		if (*(n + 1) == 'N')
			*n++ = 0;
		break;
	case 'A':
		/* 'AE' becomes 'E' */
		if (*(n + 1) == 'E')
			*n++ = 0;
		break;
	case 'W':
		/* 'WR' becomes 'R', and 'WH' to 'H' */
		if (*(n + 1) == 'R')
			*n++ = 0;
		else if (*(n + 1) == 'H') {
			*(n + 1) = *n;
			*n++ = 0;
		}
		break;
	case 'X':
		/* 'X' becomes 'S' */
		*n = 'S';
		break;
	}

	/*
	 * Now, loop step through string, stopping at end of string or when
	 * the computed 'metaph' is MAXPHONEMELEN characters long
	 */

	KSflag = 0;		/* state flag for KS translation */
	for (metaph_end = Metaph + MAXPHONEMELEN, n_start = n;
	     n <= n_end && Metaph < metaph_end; n++) {
		if (KSflag) {
			KSflag = 0;
			*Metaph++ = 'S';
		} else {
			/* Drop duplicates except for CC */
			if (*(n - 1) == *n && *n != 'C')
				continue;
			/* Check for F J L M N R or first letter vowel */
			if (same(*n) || (n == n_start && vowel(*n)))
				*Metaph++ = *n;
			else
				switch (*n) {
				case 'B':

					/*
					 * B unless in -MB
					 */
					if (n == (n_end - 1) && *(n - 1) != 'M')
						*Metaph++ = *n;
					break;
				case 'C':

					/*
					 * X if in -CIA-, -CH- else S if in
					 * -CI-, -CE-, -CY- else dropped if
					 * in -SCI-, -SCE-, -SCY- else K
					 */
					if (*(n - 1) != 'S' || !frontv(*(n + 1))) {
						if (*(n + 1) == 'I' && *(n + 2) == 'A')
							*Metaph++ = 'X';
						else if (frontv(*(n + 1)))
							*Metaph++ = 'S';
						else if (*(n + 1) == 'H')
							*Metaph++ = ((n == n_start && !vowel(*(n + 2)))
							 || *(n - 1) == 'S')
							    ? (char) 'K' : (char) 'X';
						else
							*Metaph++ = 'K';
					}
					break;
				case 'D':

					/*
					 * J if in DGE or DGI or DGY else T
					 */
					*Metaph++ = (*(n + 1) == 'G' && frontv(*(n + 2)))
					    ? (char) 'J' : (char) 'T';
					break;
				case 'G':

					/*
					 * F if in -GH and not B--GH, D--GH,
					 * -H--GH, -H---GH else dropped if
					 * -GNED, -GN, -DGE-, -DGI-, -DGY-
					 * else J if in -GE-, -GI-, -GY- and
					 * not GG else K
					 */
					if ((*(n + 1) != 'J' || vowel(*(n + 2))) &&
					    (*(n + 1) != 'N' || ((n + 1) < n_end &&
								 (*(n + 2) != 'E' || *(n + 3) != 'D'))) &&
					    (*(n - 1) != 'D' || !frontv(*(n + 1))))
						*Metaph++ = (frontv(*(n + 1)) &&
							     *(n + 2) != 'G') ? (char) 'G' : (char) 'K';
					else if (*(n + 1) == 'H' && !noghf(*(n - 3)) &&
						 *(n - 4) != 'H')
						*Metaph++ = 'F';
					break;
				case 'H':

					/*
					 * H if before a vowel and not after
					 * C, G, P, S, T else dropped
					 */
					if (!varson(*(n - 1)) && (!vowel(*(n - 1)) ||
							   vowel(*(n + 1))))
						*Metaph++ = 'H';
					break;
				case 'K':

					/*
					 * dropped if after C else K
					 */
					if (*(n - 1) != 'C')
						*Metaph++ = 'K';
					break;
				case 'P':

					/*
					 * F if before H, else P
					 */
					*Metaph++ = *(n + 1) == 'H' ?
					    (char) 'F' : (char) 'P';
					break;
				case 'Q':

					/*
					 * K
					 */
					*Metaph++ = 'K';
					break;
				case 'S':

					/*
					 * X in -SH-, -SIO- or -SIA- else S
					 */
					*Metaph++ = (*(n + 1) == 'H' ||
						     (*(n + 1) == 'I' && (*(n + 2) == 'O' ||
							  *(n + 2) == 'A')))
					    ? (char) 'X' : (char) 'S';
					break;
				case 'T':

					/*
					 * X in -TIA- or -TIO- else 0 (zero)
					 * before H else dropped if in -TCH-
					 * else T
					 */
					if (*(n + 1) == 'I' && (*(n + 2) == 'O' ||
							   *(n + 2) == 'A'))
						*Metaph++ = 'X';
					else if (*(n + 1) == 'H')
						*Metaph++ = '0';
					else if (*(n + 1) != 'C' || *(n + 2) != 'H')
						*Metaph++ = 'T';
					break;
				case 'V':

					/*
					 * F
					 */
					*Metaph++ = 'F';
					break;
				case 'W':

					/*
					 * W after a vowel, else dropped
					 */
				case 'Y':

					/*
					 * Y unless followed by a vowel
					 */
					if (vowel(*(n + 1)))
						*Metaph++ = *n;
					break;
				case 'X':

					/*
					 * KS
					 */
					if (n == n_start)
						*Metaph++ = 'S';
					else {
						*Metaph++ = 'K';	/* Insert K, then S */
						KSflag = 1;
					}
					break;
				case 'Z':

					/*
					 * S
					 */
					*Metaph++ = 'S';
					break;
				}
		}
	}

	*Metaph = 0;		/* Null terminate */
	return( ch_strdup( buf ) );
}

#endif /* SLAPD_METAPHONE */
