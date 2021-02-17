/* ldapvc.c -- a tool for verifying credentials */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2010 Kurt D. Zeilenga.
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

static int req_authzid = 0;
static int req_pp = 0;

#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
#define LDAP_SASL_NONE (~0U)
static unsigned vc_sasl = LDAP_SASL_NONE;
static char *vc_sasl_realm = NULL;
static char *vc_sasl_authcid = NULL;
static char *vc_sasl_authzid = NULL;
static char *vc_sasl_mech = NULL;
static char *vc_sasl_secprops = NULL;
#endif
static char * dn = NULL;
static struct berval cred = {0, NULL};

void
usage( void )
{
	fprintf( stderr, _("Issue LDAP Verify Credentials operation to verify a user's credentials\n\n"));
	fprintf( stderr, _("usage: %s [options] [DN [cred]])\n"), prog);
	fprintf( stderr, _("where:\n"));
	fprintf( stderr, _("    DN\tDistinguished Name\n"));
	fprintf( stderr, _("    cred\tCredentials (prompt if not present)\n"));
	fprintf( stderr, _("options:\n"));
	fprintf( stderr, _("    -a\tRequest AuthzId\n"));
	fprintf( stderr, _("    -b\tRequest Password Policy Information\n"));
	fprintf( stderr, _("    -E sasl=(a[utomatic]|i[nteractive]|q[uiet]>\tSASL mode (defaults to automatic if any other -E option provided, otherwise none))\n"));
	fprintf( stderr, _("    -E mech=<mech>\tSASL mechanism (default "" e.g. Simple)\n"));
	fprintf( stderr, _("    -E realm=<realm>\tSASL Realm (defaults to none)\n"));
	fprintf( stderr, _("    -E authcid=<authcid>\tSASL Authenication Identity (defaults to USER)\n"));
	fprintf( stderr, _("    -E authzid=<authzid>\tSASL Authorization Identity (defaults to none)\n"));
	fprintf( stderr, _("    -E secprops=<secprops>\tSASL Security Properties (defaults to none)\n"));
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "abE:"
	"d:D:e:h:H:InNO:o:p:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
		char	*control, *cvalue;
		int		crit;
	case 'E': /* vc extension */
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

		if (strcasecmp(control, "sasl") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl != LDAP_SASL_NONE) {
				fprintf(stderr,
				    _("SASL option previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing mode in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			switch (*cvalue) {
			case 'a':
			case 'A':
				vc_sasl = LDAP_SASL_AUTOMATIC;
				break;
			case 'i':
			case 'I':
				vc_sasl = LDAP_SASL_INTERACTIVE;
				break;
			case 'q':
			case 'Q':
				vc_sasl = LDAP_SASL_QUIET;
				break;
			default:
				fprintf(stderr,
					_("unknown mode %s in SASL option\n"), cvalue);
				exit(EXIT_FAILURE);
			}
#else
			fprintf(stderr,
				_("%s: not compiled with SASL support\n"), prog);
			exit(EXIT_FAILURE);
#endif

		} else if (strcasecmp(control, "mech") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl_mech) {
				fprintf(stderr,
				    _("SASL mech previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing mech in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			vc_sasl_mech = ber_strdup(cvalue);
#else
#endif

		} else if (strcasecmp(control, "realm") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl_realm) {
				fprintf(stderr,
				    _("SASL realm previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing realm in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			vc_sasl_realm = ber_strdup(cvalue);
#else
			fprintf(stderr,
				_("%s: not compiled with SASL support\n"), prog);
			exit(EXIT_FAILURE);
#endif

		} else if (strcasecmp(control, "authcid") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl_authcid) {
				fprintf(stderr,
				    _("SASL authcid previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing authcid in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			vc_sasl_authcid = ber_strdup(cvalue);
#else
			fprintf(stderr,
				_("%s: not compiled with SASL support\n"), prog);
			exit(EXIT_FAILURE);
#endif

		} else if (strcasecmp(control, "authzid") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl_authzid) {
				fprintf(stderr,
				    _("SASL authzid previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing authzid in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			vc_sasl_authzid = ber_strdup(cvalue);
#else
			fprintf(stderr,
				_("%s: not compiled with SASL support\n"), prog);
			exit(EXIT_FAILURE);
#endif

		} else if (strcasecmp(control, "secprops") == 0) {
#if defined(LDAP_API_FEATURES_VERIFY_CREDENTIALS_INTERACTIVE) && defined(HAVE_CYRUS_SASL)
			if (vc_sasl_secprops) {
				fprintf(stderr,
				    _("SASL secprops previously specified\n"));
				exit(EXIT_FAILURE);
			}
			if (cvalue == NULL) {
				fprintf(stderr,
					_("missing secprops in SASL option\n"));
				exit(EXIT_FAILURE);
			}

			vc_sasl_secprops = ber_strdup(cvalue);
#else
			fprintf(stderr,
				_("%s: not compiled with SASL support\n"), prog);
			exit(EXIT_FAILURE);
#endif

		} else {
		    fprintf( stderr, _("Invalid Verify Credentials extension name: %s\n"), control );
		    usage();
		}

	case 'a':  /* request authzid */
		req_authzid++;
		break;

	case 'b':  /* request authzid */
		req_pp++;
		break;

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
	int rcode;
	char * diag = NULL;
	struct berval	*scookie = NULL;
	struct berval	*scred = NULL;
	int		id, code = 0;
	LDAPMessage	*res;
	LDAPControl	**ctrls = NULL;
	LDAPControl	**vcctrls = NULL;
	int nvcctrls = 0;

	tool_init( TOOL_VC );
	prog = lutil_progname( "ldapvc", argc, argv );

	/* LDAPv3 only */
	protocol = LDAP_VERSION3;

	tool_args( argc, argv );

	if (argc - optind > 0) {
		dn = argv[optind++];
	}
	if (argc - optind > 0) {
		cred.bv_val = strdup(argv[optind++]);
		cred.bv_len = strlen(cred.bv_val);
	}
	if (argc - optind > 0) {
		usage();
	}
	if (dn 
#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS_INTERACTIVE
           && !vc_sasl_mech 
#endif
           && !cred.bv_val)
	{
		cred.bv_val = strdup(getpassphrase(_("User's password: ")));
	    cred.bv_len = strlen(cred.bv_val);
	}

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS_INTERACTIVE
    if (vc_sasl_mech && (vc_sasl == LDAP_SASL_NONE)) {
		vc_sasl = LDAP_SASL_AUTOMATIC;
	}
#endif

	ld = tool_conn_setup( 0, 0 );

	tool_bind( ld );

	if ( dont ) {
		rc = LDAP_SUCCESS;
		goto skip;
	}

	tool_server_controls( ld, NULL, 0 );

    if (req_authzid) {
		vcctrls = (LDAPControl **) malloc(3*sizeof(LDAPControl *));
		vcctrls[nvcctrls] = (LDAPControl *) malloc(sizeof(LDAPControl));
		vcctrls[nvcctrls]->ldctl_oid = ldap_strdup(LDAP_CONTROL_AUTHZID_REQUEST);
		vcctrls[nvcctrls]->ldctl_iscritical = 0;
		vcctrls[nvcctrls]->ldctl_value.bv_val = NULL;
		vcctrls[nvcctrls]->ldctl_value.bv_len = 0;
		vcctrls[++nvcctrls] = NULL;
    }

    if (req_pp) {
		if (!vcctrls) vcctrls = (LDAPControl **) malloc(3*sizeof(LDAPControl *));
		vcctrls[nvcctrls] = (LDAPControl *) malloc(sizeof(LDAPControl));
		vcctrls[nvcctrls]->ldctl_oid = ldap_strdup(LDAP_CONTROL_PASSWORDPOLICYREQUEST);
		vcctrls[nvcctrls]->ldctl_iscritical = 0;
		vcctrls[nvcctrls]->ldctl_value.bv_val = NULL;
		vcctrls[nvcctrls]->ldctl_value.bv_len = 0;
		vcctrls[++nvcctrls] = NULL;
    }

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS_INTERACTIVE
#ifdef HAVE_CYRUS_SASL
    if (vc_sasl_mech) {
		int msgid;
		void * defaults;
		void * context = NULL;
		const char *rmech = NULL;

		defaults = lutil_sasl_defaults(ld,
			vc_sasl_mech,
			vc_sasl_realm,
			vc_sasl_authcid,
			cred.bv_val,
			sasl_authz_id);

		do {
			rc = ldap_verify_credentials_interactive(ld, dn, vc_sasl_mech,
				vcctrls, NULL, NULL,
				vc_sasl, lutil_sasl_interact, defaults, context,
				res, &rmech, &msgid); 

			if (rc != LDAP_SASL_BIND_IN_PROGRESS) break;

			ldap_msgfree(res);

			if (ldap_result(ld, msgid, LDAP_MSG_ALL, NULL, &res) == -1 || !res) {
				ldap_get_option(ld, LDAP_OPT_RESULT_CODE, (void*) &rc);
				ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*) &text);
				tool_perror( "ldap_verify_credentials_interactive", rc, NULL, NULL, text, NULL);
				ldap_memfree(text);
				tool_exit(ld, rc);
			}
		} while (rc == LDAP_SASL_BIND_IN_PROGRESS);

	    lutil_sasl_freedefs(defaults);

	    if( rc != LDAP_SUCCESS ) {
			ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*) &text);
		    tool_perror( "ldap_verify_credentials", rc, NULL, NULL, text, NULL );
		    rc = EXIT_FAILURE;
		    goto skip;
	    }

	} else
#endif
#endif
    {
	    rc = ldap_verify_credentials( ld,
		    NULL,
		    dn, NULL, cred.bv_val ? &cred: NULL, vcctrls,
		    NULL, NULL, &id ); 
    
	    if( rc != LDAP_SUCCESS ) {
			ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*) &text);
		    tool_perror( "ldap_verify_credentials", rc, NULL, NULL, text, NULL );
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
	}

	ldap_controls_free(vcctrls);
	vcctrls = NULL;

	rc = ldap_parse_result( ld, res,
		&code, &matcheddn, &text, &refs, &ctrls, 0 );

	if (rc == LDAP_SUCCESS) rc = code;

	if (rc != LDAP_SUCCESS) {
		tool_perror( "ldap_parse_result", rc, NULL, matcheddn, text, refs );
		rc = EXIT_FAILURE;
		goto skip;
	}

	rc = ldap_parse_verify_credentials( ld, res, &rcode, &diag, &scookie, &scred, &vcctrls );
	ldap_msgfree(res);

	if (rc != LDAP_SUCCESS) {
		tool_perror( "ldap_parse_verify_credentials", rc, NULL, NULL, NULL, NULL );
		rc = EXIT_FAILURE;
		goto skip;
	}

	if (rcode != LDAP_SUCCESS) {
		printf(_("Failed: %s (%d)\n"), ldap_err2string(rcode), rcode);
	}

	if (diag && *diag) {
	    printf(_("Diagnostic: %s\n"), diag);
	}

	if (vcctrls) {
		tool_print_ctrls( ld, vcctrls );
	}

skip:
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
	ber_bvfree( scookie );
	ber_bvfree( scred );
	ber_memfree( diag );
	free( cred.bv_val );

	/* disconnect from server */
	tool_exit( ld, code == LDAP_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE );
}
