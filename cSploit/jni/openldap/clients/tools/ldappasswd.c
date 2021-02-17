/* ldappasswd -- a tool for change LDAP passwords */
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
 * The original ldappasswd(1) tool was developed by Dave Storey (F5
 * Network), based on other OpenLDAP client tools (which are, of
 * course, based on U-MICH LDAP).  This version was rewritten
 * by Kurt D. Zeilenga (based on other OpenLDAP client tools).
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


static struct berval newpw = { 0, NULL };
static struct berval oldpw = { 0, NULL };

static int   want_newpw = 0;
static int   want_oldpw = 0;

static char *oldpwfile = NULL;
static char *newpwfile = NULL;

void
usage( void )
{
	fprintf( stderr, _("Change password of an LDAP user\n\n"));
	fprintf( stderr,_("usage: %s [options] [user]\n"), prog);
	fprintf( stderr, _("  user: the authentication identity, commonly a DN\n"));
	fprintf( stderr, _("Password change options:\n"));
	fprintf( stderr, _("  -a secret  old password\n"));
	fprintf( stderr, _("  -A         prompt for old password\n"));
	fprintf( stderr, _("  -t file    read file for old password\n"));
	fprintf( stderr, _("  -s secret  new password\n"));
	fprintf( stderr, _("  -S         prompt for new password\n"));
	fprintf( stderr, _("  -T file    read file for new password\n"));
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "a:As:St:T:"
	"d:D:e:h:H:InNO:o:p:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
#if 0
	case 'E': /* passwd extensions */ {
		int		crit;
		char	*control, *cvalue;
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
		fprintf( stderr, _("Invalid passwd extension name: %s\n"), control );
		usage();
		}
#endif

	case 'a':	/* old password (secret) */
		oldpw.bv_val = strdup( optarg );
		{
			char* p;
			for( p = optarg; *p != '\0'; p++ ) {
				*p = '\0';
			}
		}
		oldpw.bv_len = strlen( oldpw.bv_val );
		break;

	case 'A':	/* prompt for old password */
		want_oldpw++;
		break;

	case 's':	/* new password (secret) */
		newpw.bv_val = strdup (optarg);
		{
			char* p;
			for( p = optarg; *p != '\0'; p++ ) {
				*p = '\0';
			}
		}
		newpw.bv_len = strlen( newpw.bv_val );
		break;

	case 'S':	/* prompt for user password */
		want_newpw++;
		break;

	case 't':
		oldpwfile = optarg;
		break;

	case 'T':
		newpwfile = optarg;
		break;

	default:
		return 0;
	}
	return 1;
}


int
main( int argc, char *argv[] )
{
	int rc;
	char	*user = NULL;

	LDAP	       *ld = NULL;
	struct berval bv = {0, NULL};
	BerElement  *ber = NULL;

	int id, code = LDAP_OTHER;
	LDAPMessage *res;
	char *matcheddn = NULL, *text = NULL, **refs = NULL;
	char	*retoid = NULL;
	struct berval *retdata = NULL;
	LDAPControl **ctrls = NULL;

    tool_init( TOOL_PASSWD );
	prog = lutil_progname( "ldappasswd", argc, argv );

	/* LDAPv3 only */
	protocol = LDAP_VERSION3;

	tool_args( argc, argv );

	if( argc - optind > 1 ) {
		usage();
	} else if ( argc - optind == 1 ) {
		user = strdup( argv[optind] );
	} else {
		user = NULL;
	}

	if( oldpwfile ) {
		rc = lutil_get_filed_password( oldpwfile, &oldpw );
		if( rc ) {
			rc = EXIT_FAILURE;
			goto done;
		}
	}

	if( want_oldpw && oldpw.bv_val == NULL ) {
		/* prompt for old password */
		char *ckoldpw;
		oldpw.bv_val = strdup(getpassphrase(_("Old password: ")));
		ckoldpw = getpassphrase(_("Re-enter old password: "));

		if( oldpw.bv_val == NULL || ckoldpw == NULL ||
			strcmp( oldpw.bv_val, ckoldpw ))
		{
			fprintf( stderr, _("passwords do not match\n") );
			rc = EXIT_FAILURE;
			goto done;
		}

		oldpw.bv_len = strlen( oldpw.bv_val );
	}

	if( newpwfile ) {
		rc = lutil_get_filed_password( newpwfile, &newpw );
		if( rc ) {
			rc = EXIT_FAILURE;
			goto done;
		}
	}

	if( want_newpw && newpw.bv_val == NULL ) {
		/* prompt for new password */
		char *cknewpw;
		newpw.bv_val = strdup(getpassphrase(_("New password: ")));
		cknewpw = getpassphrase(_("Re-enter new password: "));

		if( newpw.bv_val == NULL || cknewpw == NULL ||
			strcmp( newpw.bv_val, cknewpw ))
		{
			fprintf( stderr, _("passwords do not match\n") );
			rc = EXIT_FAILURE;
			goto done;
		}

		newpw.bv_len = strlen( newpw.bv_val );
	}

	ld = tool_conn_setup( 0, 0 );

	tool_bind( ld );

	if( user != NULL || oldpw.bv_val != NULL || newpw.bv_val != NULL ) {
		/* build the password modify request data */
		ber = ber_alloc_t( LBER_USE_DER );

		if( ber == NULL ) {
			perror( "ber_alloc_t" );
			rc = EXIT_FAILURE;
			goto done;
		}

		ber_printf( ber, "{" /*}*/ );

		if( user != NULL ) {
			ber_printf( ber, "ts",
				LDAP_TAG_EXOP_MODIFY_PASSWD_ID, user );
			free(user);
		}

		if( oldpw.bv_val != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_OLD, &oldpw );
			free(oldpw.bv_val);
		}

		if( newpw.bv_val != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_NEW, &newpw );
			free(newpw.bv_val);
		}

		ber_printf( ber, /*{*/ "N}" );

		rc = ber_flatten2( ber, &bv, 0 );

		if( rc < 0 ) {
			perror( "ber_flatten2" );
			rc = EXIT_FAILURE;
			goto done;
		}
	}

	if ( dont ) {
		rc = LDAP_SUCCESS;
		goto done;
	}

	tool_server_controls( ld, NULL, 0);

	rc = ldap_extended_operation( ld,
		LDAP_EXOP_MODIFY_PASSWD, bv.bv_val ? &bv : NULL, 
		NULL, NULL, &id );

	ber_free( ber, 1 );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_extended_operation", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto done;
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
	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_result", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto done;
	}

	rc = ldap_parse_extended_result( ld, res, &retoid, &retdata, 1 );
	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_extended_result", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto done;
	}

	if( retdata != NULL ) {
		ber_tag_t tag;
		char *s;
		ber = ber_init( retdata );

		if( ber == NULL ) {
			perror( "ber_init" );
			rc = EXIT_FAILURE;
			goto done;
		}

		/* we should check the tag */
		tag = ber_scanf( ber, "{a}", &s);

		if( tag == LBER_ERROR ) {
			perror( "ber_scanf" );
		} else {
			printf(_("New password: %s\n"), s);
			ber_memfree( s );
		}

		ber_free( ber, 1 );

	} else if ( code == LDAP_SUCCESS && newpw.bv_val == NULL ) {
		tool_perror( "ldap_parse_extended_result", LDAP_DECODING_ERROR,
			" new password expected", NULL, NULL, NULL );
	}

	if( verbose || code != LDAP_SUCCESS ||
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

		if( ctrls ) {
			tool_print_ctrls( ld, ctrls );
			ldap_controls_free( ctrls );
		}
	}

	ber_memfree( text );
	ber_memfree( matcheddn );
	ber_memvfree( (void **) refs );
	ber_memfree( retoid );
	ber_bvfree( retdata );

	rc = ( code == LDAP_SUCCESS ) ? EXIT_SUCCESS : EXIT_FAILURE;

done:
	/* disconnect from server */
	tool_exit( ld, rc ); 
}
