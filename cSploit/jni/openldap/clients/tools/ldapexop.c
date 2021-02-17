/* ldapexop.c -- a tool for performing well-known extended operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software based, in part, on other client tools.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <ldap.h>
#include "ldif.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

#include "common.h"

void
usage( void )
{
	fprintf( stderr, _("Issue LDAP extended operations\n\n"));
	fprintf( stderr, _("usage: %s [options] <oid|oid:data|oid::b64data>\n"), prog);
	fprintf( stderr, _("       %s [options] whoami\n"), prog);
	fprintf( stderr, _("       %s [options] cancel <id>\n"), prog);
	fprintf( stderr, _("       %s [options] refresh <DN> [<ttl>]\n"), prog);
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = ""
	"d:D:e:h:H:InNO:o:p:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
	default:
		return 0;
	}
	return 1;
}


int
main( int argc, char *argv[] )
{
	int		rc;

	LDAP		*ld = NULL;

	char		*matcheddn = NULL, *text = NULL, **refs = NULL;
	LDAPControl **ctrls = NULL;
	int		id, code;
	LDAPMessage	*res = NULL;

	tool_init( TOOL_EXOP );
	prog = lutil_progname( "ldapexop", argc, argv );

	/* LDAPv3 only */
	protocol = LDAP_VERSION3;

	tool_args( argc, argv );

	if ( argc - optind < 1 ) {
		usage();
	}

	ld = tool_conn_setup( 0, 0 );

	tool_bind( ld );

	argv += optind;
	argc -= optind;

	if ( strcasecmp( argv[ 0 ], "whoami" ) == 0 ) {
		tool_server_controls( ld, NULL, 0 );

		rc = ldap_whoami( ld, NULL, NULL, &id ); 
		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_extended_operation", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

	} else if ( strcasecmp( argv[ 0 ], "cancel" ) == 0 ) {
		int		cancelid;

		switch ( argc ) {
		case 2:
			 if ( lutil_atoi( &cancelid, argv[ 1 ] ) != 0 || cancelid < 0 ) {
				fprintf( stderr, "invalid cancelid=%s\n\n", argv[ 1 ] );
				usage();
			}
			break;

		default:
			fprintf( stderr, "need cancelid\n\n" );
			usage();
		}

		rc = ldap_cancel( ld, cancelid, NULL, NULL, &id );
		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_cancel", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

	} else if ( strcasecmp( argv[ 0 ], "passwd" ) == 0 ) {
		fprintf( stderr, "use ldappasswd(1) instead.\n\n", argv[ 0 ] );
		usage();
		/* TODO? */

	} else if ( strcasecmp( argv[ 0 ], "refresh" ) == 0 ) {
		int		ttl = 3600;
		struct berval	dn;

		switch ( argc ) {
		case 3:
			ttl = atoi( argv[ 2 ] );

		case 2:
			dn.bv_val = argv[ 1 ];
			dn.bv_len = strlen( dn.bv_val );
			break;

		default:
			fprintf( stderr, _("need DN [ttl]\n\n") );
			usage();
		}
		
		tool_server_controls( ld, NULL, 0 );

		rc = ldap_refresh( ld, &dn, ttl, NULL, NULL, &id ); 
		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_extended_operation", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

	} else {
		char *p;

		if ( argc != 1 ) {
			usage();
		}

		p = strchr( argv[ 0 ], ':' );
		if ( p == argv[ 0 ] ) {
			usage();
		}

		if ( p != NULL )
			*p++ = '\0';

		if ( tool_is_oid( argv[ 0 ] ) ) {
			struct berval	reqdata;
			struct berval	type;
			struct berval	value;
			int		freeval;

			if ( p != NULL ) {
				p[ -1 ] = ':';
				ldif_parse_line2( argv[ 0 ], &type, &value, &freeval );
				p[ -1 ] = '\0';

				if ( freeval ) {
					reqdata = value;
				} else {
					ber_dupbv( &reqdata, &value );
				}
			}


			tool_server_controls( ld, NULL, 0 );

			rc = ldap_extended_operation( ld, argv[ 0 ], p ? &reqdata : NULL, NULL, NULL, &id );
			if ( rc != LDAP_SUCCESS ) {
				tool_perror( "ldap_extended_operation", rc, NULL, NULL, NULL, NULL );
				rc = EXIT_FAILURE;
				goto skip;
			}
		} else {
			fprintf( stderr, "unknown exop \"%s\"\n\n", argv[ 0 ] );
			usage();
		}
	}

	for ( ; ; ) {
		struct timeval	tv;

		if ( tool_check_abandon( ld, id ) ) {
			tool_exit( ld, LDAP_CANCELLED );
		}

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		rc = ldap_result( ld, LDAP_RES_ANY, LDAP_MSG_ALL, &tv, &res );
		if ( rc < 0 ) {
			tool_perror( "ldap_result", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

		if ( rc != 0 ) {
			break;
		}
	}

	rc = ldap_parse_result( ld, res,
		&code, &matcheddn, &text, &refs, &ctrls, 0 );
	if ( rc == LDAP_SUCCESS ) {
		rc = code;
	}

	if ( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_result", rc, NULL, matcheddn, text, refs );
		rc = EXIT_FAILURE;
		goto skip;
	}

	if ( strcasecmp( argv[ 0 ], "whoami" ) == 0 ) {
		char		*retoid = NULL;
		struct berval	*retdata = NULL;

		rc = ldap_parse_extended_result( ld, res, &retoid, &retdata, 0 );

		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_parse_extended_result", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

		if ( retdata != NULL ) {
			if ( retdata->bv_len == 0 ) {
				printf(_("anonymous\n") );
			} else {
				printf("%s\n", retdata->bv_val );
			}
		}

		ber_memfree( retoid );
		ber_bvfree( retdata );

	} else if ( strcasecmp( argv[ 0 ], "cancel" ) == 0 ) {
		/* no extended response; returns specific errors */
		assert( 0 );

	} else if ( strcasecmp( argv[ 0 ], "passwd" ) == 0 ) {
		/* TODO */

	} else if ( strcasecmp( argv[ 0 ], "refresh" ) == 0 ) {
		int	newttl;

		rc = ldap_parse_refresh( ld, res, &newttl );

		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_parse_refresh", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

		printf( "newttl=%d\n", newttl );

	} else if ( tool_is_oid( argv[ 0 ] ) ) {
		char		*retoid = NULL;
		struct berval	*retdata = NULL;

		if( ldif < 2 ) {
			printf(_("# extended operation response\n"));
		}

		rc = ldap_parse_extended_result( ld, res, &retoid, &retdata, 0 );
		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_parse_extended_result", rc, NULL, NULL, NULL, NULL );
			rc = EXIT_FAILURE;
			goto skip;
		}

		if ( ldif < 2 && retoid != NULL ) {
			tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
				"oid", retoid, strlen(retoid) );
		}

		ber_memfree( retoid );

		if( retdata != NULL ) {
			if ( ldif < 2 ) {
				tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_BINARY,
					"data", retdata->bv_val, retdata->bv_len );
			}

			ber_bvfree( retdata );
		}
	}

	if( verbose || code != LDAP_SUCCESS ||
		( matcheddn && *matcheddn ) || ( text && *text ) || refs ) {
		printf( _("Result: %s (%d)\n"), ldap_err2string( code ), code );

		if( text && *text ) {
			printf( _("Additional info: %s\n"), text );
		}

		if( matcheddn && *matcheddn ) {
			printf( _("Matched DN: %s\n"), matcheddn );
		}

		if( refs ) {
			int i;
			for( i=0; refs[i]; i++ ) {
				printf(_("Referral: %s\n"), refs[i] );
			}
		}
	}

    if (ctrls) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	ber_memfree( text );
	ber_memfree( matcheddn );
	ber_memvfree( (void **) refs );

skip:
	/* disconnect from server */
	if ( res )
		ldap_msgfree( res );
	tool_exit( ld, code == LDAP_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE );
}
