/* urltest.c -- OpenLDAP URL API Test Program */
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
/* ACKNOWLEDGEMENT:
 * This program was initially developed by Pierangelo Masarati
 * <ando@OpenLDAP.org> for inclusion in OpenLDAP Software.
 */

/*
 * This program is designed to test the ldap_url_* functions
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include <ldap.h>

#include "ldap-int.h"

#include "ldap_defaults.h"

int
main(int argc, char *argv[])
{
	const char	*url,
			*scope = NULL;
	LDAPURLDesc	*lud;
	enum {
		IS_LDAP = 0,
		IS_LDAPS,
		IS_LDAPI
	} type = IS_LDAP;
	int		rc;

	if ( argc != 2 ) {
		fprintf( stderr, "usage: urltest <url>\n" );
		exit( EXIT_FAILURE );
	}

	url = argv[ 1 ];

	if ( ldap_is_ldaps_url( url ) ) {
		fprintf( stdout, "LDAPS url\n" );
		type = IS_LDAPS;

	} else if ( ldap_is_ldapi_url( url ) ) {
		fprintf( stdout, "LDAPI url\n" );
		type = IS_LDAPI;

	} else if ( ldap_is_ldap_url( url ) ) {
		fprintf( stdout, "generic LDAP url\n" );

	} else {
		fprintf( stderr, "Need a valid LDAP url\n" );
		exit( EXIT_FAILURE );
	}

	rc = ldap_url_parse( url, &lud );
	if ( rc != LDAP_URL_SUCCESS ) {
		fprintf( stderr, "ldap_url_parse(%s) failed (%d)\n", url, rc );
		exit( EXIT_FAILURE );
	}

	fprintf( stdout, "PROTO: %s\n", lud->lud_scheme );
	switch ( type ) {
	case IS_LDAPI:
		fprintf( stdout, "PATH: %s\n", lud->lud_host );
		break;

	default:
		fprintf( stdout, "HOST: %s\n", lud->lud_host );
		if ( lud->lud_port != 0 ) {
			fprintf( stdout, "PORT: %d\n", lud->lud_port );
		}
	}

	if ( lud->lud_dn && lud->lud_dn[ 0 ] ) {
		fprintf( stdout, "DN: %s\n", lud->lud_dn );
	}

	if ( lud->lud_attrs ) {
		int	i;

		fprintf( stdout, "ATTRS:\n" );
		for ( i = 0; lud->lud_attrs[ i ]; i++ ) {
			fprintf( stdout, "\t%s\n", lud->lud_attrs[ i ] );
		}
	}

	scope = ldap_pvt_scope2str( lud->lud_scope );
	if ( scope ) {
		fprintf( stdout, "SCOPE: %s\n", scope );
	}

	if ( lud->lud_filter ) {
		fprintf( stdout, "FILTER: %s\n", lud->lud_filter );
	}

	if ( lud->lud_exts ) {
		int	i;

		fprintf( stdout, "EXTS:\n" );
		for ( i = 0; lud->lud_exts[ i ]; i++ ) {
			fprintf( stdout, "\t%s\n", lud->lud_exts[ i ] );
		}
	}

	fprintf( stdout, "URL: %s\n", ldap_url_desc2str( lud ));

	return EXIT_SUCCESS;
}
