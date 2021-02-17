/* etest.c - lber encoding test program */
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
/* Portions Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#ifdef HAVE_CONSOLE_H
#include <console.h>
#endif /* HAVE_CONSOLE_H */

#include "lber.h"

static void usage( const char *name )
{
	fprintf( stderr, "usage: %s fmtstring\n", name );
}

static char* getbuf( void ) {
	char *p;
	static char buf[1024];

	if ( fgets( buf, sizeof(buf), stdin ) == NULL ) return NULL;

	if ( (p = strchr( buf, '\n' )) != NULL ) *p = '\0';

	return buf;
}

int
main( int argc, char **argv )
{
	char	*s;
	int tag;

	int			fd, rc;
	BerElement	*ber;
	Sockbuf		*sb;

	/* enable debugging */
	int ival = -1;
	ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &ival );

	if ( argc < 2 ) {
		usage( argv[0] );
		return( EXIT_FAILURE );
	}

#ifdef HAVE_CONSOLE_H
	ccommand( &argv );
	cshow( stdout );

	if (( fd = open( "lber-test", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY ))
		< 0 ) {
	    perror( "open" );
	    return( EXIT_FAILURE );
	}

#else
	fd = fileno(stdout);
#endif

	sb = ber_sockbuf_alloc();
	ber_sockbuf_add_io( sb, &ber_sockbuf_io_fd, LBER_SBIOD_LEVEL_PROVIDER,
		(void *)&fd );

	if( sb == NULL ) {
		perror( "ber_sockbuf_alloc_fd" );
		return( EXIT_FAILURE );
	}

	if ( (ber = ber_alloc_t( LBER_USE_DER )) == NULL ) {
		perror( "ber_alloc" );
		return( EXIT_FAILURE );
	}

	fprintf(stderr, "encode: start\n" );
	if( ber_printf( ber, "{" /*}*/ ) ) {
		perror( "ber_printf {" /*}*/ );
		return( EXIT_FAILURE );
	}

	for ( s = argv[1]; *s; s++ ) {
		char *buf;
		char fmt[2];

		fmt[0] = *s;
		fmt[1] = '\0';

		fprintf(stderr, "encode: %s\n", fmt );
		switch ( *s ) {
		case 'i':	/* int */
		case 'b':	/* boolean */
		case 'e':	/* enumeration */
			buf = getbuf();
			rc = ber_printf( ber, fmt, atoi(buf) );
			break;

		case 'n':	/* null */
		case '{':	/* begin sequence */
		case '}':	/* end sequence */
		case '[':	/* begin set */
		case ']':	/* end set */
			rc = ber_printf( ber, fmt );
			break;

		case 'o':	/* octet string (non-null terminated) */
		case 'B':	/* bit string */
			buf = getbuf();
			rc = ber_printf( ber, fmt, buf, strlen(buf) );
			break;

		case 's':	/* string */
			buf = getbuf();
			rc = ber_printf( ber, fmt, buf );
			break;
		case 't':	/* tag for the next element */
			buf = getbuf();
			tag = atoi(buf);
			rc = ber_printf( ber, fmt, tag );
			break;

		default:
			fprintf( stderr, "encode: unknown fmt %c\n", *fmt );
			rc = -1;
			break;
		}

		if( rc == -1 ) {
			perror( "ber_printf" );
			return( EXIT_FAILURE );
		}
	}

	fprintf(stderr, "encode: end\n" );
	if( ber_printf( ber, /*{*/ "N}" ) == -1 ) {
		perror( /*{*/ "ber_printf }" );
		return( EXIT_FAILURE );
	}

	if ( ber_flush2( sb, ber, LBER_FLUSH_FREE_ALWAYS ) == -1 ) {
		perror( "ber_flush2" );
		return( EXIT_FAILURE );
	}

	ber_sockbuf_free( sb );
	return( EXIT_SUCCESS );
}
