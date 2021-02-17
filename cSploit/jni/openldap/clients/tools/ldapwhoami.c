/* ldapwhoami.c -- a tool for asking the directory "Who Am I?" */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 1998-2001 Net Boolean Incorporated.
 * Portions Copyright 2001-2003 IBM Corporation.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
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
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

#include "common.h"


void
usage( void )
{
	fprintf( stderr, _("Issue LDAP Who am I? operation to request user's authzid\n\n"));
	fprintf( stderr, _("usage: %s [options]\n"), prog);
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = ""
	"d:D:e:h:H:InNO:o:p:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
#if 0
		char	*control, *cvalue;
		int		crit;
	case 'E': /* whoami extension */
		if( protocol == LDAP_VERSION2 ) {
			fprintf( stderr, _("%s: -E incompatible with LDAPv%d\n"),
				prog, protocol );
			exit( EXIT_FAILURE );
		}

		/* should be extended to support comma separated list of
		 *	[!]key[=value] parameters, e.g.  -E !foo,bar=567
		 */

		crit = 0;
		cvalue = NULL;
		if( optarg[0] == '!' ) {
			crit = 1;
			optarg++;
		}

		control = strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}

		fprintf( stderr, _("Invalid whoami extension name: %s\n"), control );
		usage();
#endif

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
	struct berval	*authzid = NULL;
	int		id, code = 0;
	LDAPMessage	*res = NULL;
	LDAPControl	**ctrls = NULL;

	tool_init( TOOL_WHOAMI );
	prog = lutil_progname( "ldapwhoami", argc, argv );

	/* LDAPv3 only */
	protocol = LDAP_VERSION3;

	tool_args( argc, argv );

	if( argc - optind > 0 ) {
		usage();
	}

	ld = tool_conn_setup( 0, 0 );

	tool_bind( ld );

	if ( dont ) {
		rc = LDAP_SUCCESS;
		goto skip;
	}

	tool_server_controls( ld, NULL, 0 );

	rc = ldap_whoami( ld, NULL, NULL, &id ); 

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_whoami", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto skip;
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
			tool_exit( ld, rc );
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

	rc = ldap_parse_whoami( ld, res, &authzid );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_whoami", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto skip;
	}

	if( authzid != NULL ) {
		if( authzid->bv_len == 0 ) {
			printf(_("anonymous\n") );
		} else {
			printf("%s\n", authzid->bv_val );
		}
	}

skip:
	ldap_msgfree(res);
	if ( verbose || code != LDAP_SUCCESS ||
		( matcheddn && *matcheddn ) || ( text && *text ) || refs || ctrls )
	{
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

		if (ctrls) {
			tool_print_ctrls( ld, ctrls );
			ldap_controls_free( ctrls );
		}
	}

	ber_memfree( text );
	ber_memfree( matcheddn );
	ber_memvfree( (void **) refs );
	ber_bvfree( authzid );

	/* disconnect from server */
	tool_exit( ld, code == LDAP_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE );
}
