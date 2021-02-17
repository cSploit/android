/* ldapurl -- a tool for generating LDAP URLs */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 Pierangelo Masarati, SysNet
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
/* Portions Copyright (c) 1992-1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.  This
 * software is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by Pierangelo Masarati
 * for inclusion in OpenLDAP software.
 */

#include "portable.h"

#include <ac/stdlib.h>
#include <stdio.h>
#include <ac/unistd.h>

#include "ldap.h"
#include "ldap_pvt.h"
#include "lutil.h"

static int
usage(void)
{
	fprintf( stderr, _("usage: %s [options]\n\n"), "ldapurl" );
	fprintf( stderr, _("generates RFC 4516 LDAP URL with extensions\n\n" ) );
	fprintf( stderr, _("URL options:\n"));
	fprintf( stderr, _("  -a attrs   comma separated list of attributes\n" ) );
	fprintf( stderr, _("  -b base    (RFC 4514 LDAP DN)\n" ) );
	fprintf( stderr, _("  -E ext     (format: \"ext=value\"; multiple occurrences allowed)\n" ) );
	fprintf( stderr, _("  -f filter  (RFC 4515 LDAP filter)\n" ) );
	fprintf( stderr, _("  -h host    \n" ) );
	fprintf( stderr, _("  -p port    (default: 389 for ldap, 636 for ldaps)\n" ) );
	fprintf( stderr, _("  -s scope   (RFC 4511 searchScope and extensions)\n" ) );
	fprintf( stderr, _("  -S scheme  (RFC 4516 LDAP URL scheme and extensions)\n" ) );
	exit( EXIT_FAILURE );
}

static int
do_uri_create( LDAPURLDesc *lud )
{
	char	*uri;

	if ( lud->lud_scheme == NULL ) {
		lud->lud_scheme = "ldap";
	}

	if ( lud->lud_port == -1 ) {
		if ( strcasecmp( lud->lud_scheme, "ldap" ) == 0 ) {
			lud->lud_port = LDAP_PORT;

		} else if ( strcasecmp( lud->lud_scheme, "ldaps" ) == 0 ) {
			lud->lud_port = LDAPS_PORT;

		} else if ( strcasecmp( lud->lud_scheme, "ldapi" ) == 0 ) {
			lud->lud_port = 0;

		} else {
			/* forgiving... */
			lud->lud_port = 0;
		}
	}

	if ( lud->lud_scope == -1 ) {
		lud->lud_scope = LDAP_SCOPE_DEFAULT;
	}

	uri = ldap_url_desc2str( lud );

	if ( lud->lud_attrs != NULL ) {
		ldap_charray_free( lud->lud_attrs );
		lud->lud_attrs = NULL;
	}

	if ( lud->lud_exts != NULL ) {
		free( lud->lud_exts );
		lud->lud_exts = NULL;
	}

	if ( uri == NULL ) {
		fprintf( stderr, "unable to generate URI\n" );
		exit( EXIT_FAILURE );
	}

	printf( "%s\n", uri );
	free( uri );

	return 0;
}

static int
do_uri_explode( const char *uri )
{
	LDAPURLDesc	*lud;
	int		rc;

	rc = ldap_url_parse( uri, &lud );
	if ( rc != LDAP_URL_SUCCESS ) {
		fprintf( stderr, "unable to parse URI \"%s\"\n", uri );
		return 1;
	}

	if ( lud->lud_scheme != NULL && lud->lud_scheme[0] != '\0' ) {
		printf( "scheme: %s\n", lud->lud_scheme );
	}

	if ( lud->lud_host != NULL && lud->lud_host[0] != '\0' ) {
		printf( "host: %s\n", lud->lud_host );
	}

	if ( lud->lud_port != 0 ) {
		printf( "port: %d\n", lud->lud_port );
	}

	if ( lud->lud_dn != NULL && lud->lud_dn[0] != '\0' ) {
		printf( "dn: %s\n", lud->lud_dn );
	}

	if ( lud->lud_attrs != NULL ) {
		int	i;

		for ( i = 0; lud->lud_attrs[i] != NULL; i++ ) {
			printf( "selector: %s\n", lud->lud_attrs[i] );
		}
	}

	if ( lud->lud_scope != LDAP_SCOPE_DEFAULT ) {
		printf( "scope: %s\n", ldap_pvt_scope2str( lud->lud_scope ) );
	}

	if ( lud->lud_filter != NULL && lud->lud_filter[0] != '\0' ) {
		printf( "filter: %s\n", lud->lud_filter );
	}

	if ( lud->lud_exts != NULL ) {
		int	i;

		for ( i = 0; lud->lud_exts[i] != NULL; i++ ) {
			printf( "extension: %s\n", lud->lud_exts[i] );
		}
	}

	return 0;
}

int
main( int argc, char *argv[])
{
	LDAPURLDesc	lud = { 0 };
	char		*uri = NULL;
	int		gotlud = 0;
	int		nexts = 0;

	lud.lud_port = -1;
	lud.lud_scope = -1;

	while ( 1 ) {
		int opt = getopt( argc, argv, "S:h:p:b:a:s:f:E:H:" );

		if ( opt == EOF ) {
			break;
		}

		if ( opt == 'H' ) {
			if ( gotlud ) {
				fprintf( stderr, "option -H incompatible with previous options\n" );
				usage();
			}

			if ( uri != NULL ) {
				fprintf( stderr, "URI already provided\n" );
				usage();
			}

			uri = optarg;
			continue;
		}

		switch ( opt ) {
		case 'S':
		case 'h':
		case 'p':
		case 'b':
		case 'a':
		case 's':
		case 'f':
		case 'E':
			if ( uri != NULL ) {
				fprintf( stderr, "option -%c incompatible with -H\n", opt );
				usage();
			}
			gotlud++;
		}

		switch ( opt ) {
		case 'S':
			if ( lud.lud_scheme != NULL ) {
				fprintf( stderr, "scheme already provided\n" );
				usage();
			}
			lud.lud_scheme = optarg;
			break;

		case 'h':
			if ( lud.lud_host != NULL ) {
				fprintf( stderr, "host already provided\n" );
				usage();
			}
			lud.lud_host = optarg;
			break;

		case 'p':
			if ( lud.lud_port != -1 ) {
				fprintf( stderr, "port already provided\n" );
				usage();
			}

			if ( lutil_atoi( &lud.lud_port, optarg ) ) {
				fprintf( stderr, "unable to parse port \"%s\"\n", optarg );
				usage();
			}
			break;

		case 'b':
			if ( lud.lud_dn != NULL ) {
				fprintf( stderr, "base already provided\n" );
				usage();
			}
			lud.lud_dn = optarg;
			break;

		case 'a':
			if ( lud.lud_attrs != NULL ) {
				fprintf( stderr, "attrs already provided\n" );
				usage();
			}
			lud.lud_attrs = ldap_str2charray( optarg, "," );
			if ( lud.lud_attrs == NULL ) {
				fprintf( stderr, "unable to parse attrs list \"%s\"\n", optarg );
				usage();
			}
			break;

		case 's':
			if ( lud.lud_scope != -1 ) {
				fprintf( stderr, "scope already provided\n" );
				usage();
			}

			lud.lud_scope = ldap_pvt_str2scope( optarg );
			if ( lud.lud_scope == -1 ) {
				fprintf( stderr, "unable to parse scope \"%s\"\n", optarg );
				usage();
			}
			break;

		case 'f':
			if ( lud.lud_filter != NULL ) {
				fprintf( stderr, "filter already provided\n" );
				usage();
			}
			lud.lud_filter = optarg;
			break;

		case 'E':
			lud.lud_exts = (char **)realloc( lud.lud_exts,
				sizeof( char * ) * ( nexts + 2 ) );
			lud.lud_exts[ nexts++ ] = optarg;
			lud.lud_exts[ nexts ] = NULL;
			break;

		default:
			assert( opt != 'H' );
			usage();
		}
	}

	if ( uri != NULL ) {
		return do_uri_explode( uri );

	}

	return do_uri_create( &lud );
}
