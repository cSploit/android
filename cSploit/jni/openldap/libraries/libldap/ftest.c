/* ftest.c -- OpenLDAP Filter API Test */
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

#include "portable.h"

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include <stdio.h>

#include <ldap.h>

#include "ldap_pvt.h"
#include "lber_pvt.h"

#include "ldif.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

static int filter2ber( char *filter );

int usage()
{
	fprintf( stderr, "usage:\n"
		"  ftest [-d n] filter\n"
		"    filter - RFC 4515 string representation of an "
			"LDAP search filter\n" );
	return EXIT_FAILURE;
}

int
main( int argc, char *argv[] )
{
	int c;
	int debug=0;

    while( (c = getopt( argc, argv, "d:" )) != EOF ) {
		switch ( c ) {
		case 'd':
			debug = atoi( optarg );
			break;
		default:
			fprintf( stderr, "ftest: unrecognized option -%c\n",
				optopt );
			return usage();
		}
	}

	if ( debug ) {
		if ( ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &debug )
			!= LBER_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LBER_OPT_DEBUG_LEVEL %d\n",
				debug );
		}
		if ( ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &debug )
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LDAP_OPT_DEBUG_LEVEL %d\n",
				debug );
		}
	}

	if ( argc - optind != 1 ) {
		return usage();
	}

	return filter2ber( strdup( argv[optind] ) );
}

static int filter2ber( char *filter )
{
	int rc;
	struct berval bv = BER_BVNULL;
	BerElement *ber;

	printf( "Filter: %s\n", filter );

	ber = ber_alloc_t( LBER_USE_DER );
	if( ber == NULL ) {
		perror( "ber_alloc_t" );
		return EXIT_FAILURE;
	}

	rc = ldap_pvt_put_filter( ber, filter );
	if( rc < 0 ) {
		fprintf( stderr, "Filter error!\n");
		return EXIT_FAILURE;
	}

	rc = ber_flatten2( ber, &bv, 0 );
	if( rc < 0 ) {
		perror( "ber_flatten2" );
		return EXIT_FAILURE;
	}

	printf( "BER encoding (len=%ld):\n", (long) bv.bv_len );
	ber_bprint( bv.bv_val, bv.bv_len );

	ber_free( ber, 1 );

	return EXIT_SUCCESS;
}

