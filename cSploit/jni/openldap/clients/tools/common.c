/* common.c - common routines for the ldap client tools */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 Kurt D. Zeilenga.
 * Portions Copyright 2003 IBM Corporation.
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
 * This file was initially created by Hallvard B. Furuseth based (in
 * part) upon argument parsing code for individual tools located in
 * this directory.   Additional contributors include:
 *   Kurt D. Zeilenga (additional common argument and control support)
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/signal.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/unistd.h>
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/socket.h>

#ifdef HAVE_CYRUS_SASL
#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#else
#include <sasl.h>
#endif
#endif

#include <ldap.h>

#include "ldif.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"
#include "ldap_pvt.h"
#include "lber_pvt.h"

#include "common.h"

/* input-related vars */

/* misc. parameters */
tool_type_t	tool_type;
int		contoper = 0;
int		debug = 0;
char		*infile = NULL;
int		dont = 0;
int		nocanon = 0;
int		referrals = 0;
int		verbose = 0;
int		ldif = 0;
ber_len_t	ldif_wrap = LDIF_LINE_WIDTH;
char		*prog = NULL;

/* connection */
char		*ldapuri = NULL;
char		*ldaphost = NULL;
int  		ldapport = 0;
int		use_tls = 0;
int		protocol = -1;
int		version = 0;

/* authc/authz */
int		authmethod = -1;
char		*binddn = NULL;
int		want_bindpw = 0;
struct berval	passwd = { 0, NULL };
char		*pw_file = NULL;
#ifdef HAVE_CYRUS_SASL
unsigned	sasl_flags = LDAP_SASL_AUTOMATIC;
char		*sasl_realm = NULL;
char		*sasl_authc_id = NULL;
char		*sasl_authz_id = NULL;
char		*sasl_mech = NULL;
char		*sasl_secprops = NULL;
#endif

/* controls */
int		assertctl;
char		*assertion = NULL;
struct berval	assertionvalue = BER_BVNULL;
char		*authzid = NULL;
int		authzcrit = 1;
/* support deprecated early version of proxyAuthz */
#define LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ	"2.16.840.1.113730.3.4.12"
#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
char		*proxydn = NULL;
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */
int		manageDIT = 0;
int		manageDSAit = 0;
int		noop = 0;
int		ppolicy = 0;
int		preread = 0;
static char	*preread_attrs = NULL;
int		postread = 0;
static char	*postread_attrs = NULL;
ber_int_t	pr_morePagedResults = 1;
struct berval	pr_cookie = { 0, NULL };
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
int		chaining = 0;
static int	chainingResolve = -1;
static int	chainingContinuation = -1;
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */
#ifdef LDAP_CONTROL_X_SESSION_TRACKING
static int	sessionTracking = 0;
static char	*sessionTrackingName;
struct berval	stValue;
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */
ber_int_t vlvPos;
ber_int_t vlvCount;
struct berval *vlvContext;
static int	bauthzid;

LDAPControl	*unknown_ctrls = NULL;
int		unknown_ctrls_num = 0;

/* options */
struct timeval	nettimeout = { -1 , 0 };

typedef int (*print_ctrl_fn)( LDAP *ld, LDAPControl *ctrl );

static int print_preread( LDAP *ld, LDAPControl *ctrl );
static int print_postread( LDAP *ld, LDAPControl *ctrl );
static int print_paged_results( LDAP *ld, LDAPControl *ctrl );
#ifdef LDAP_CONTROL_AUTHZID_RESPONSE
static int print_authzid( LDAP *ld, LDAPControl *ctrl );
#endif
#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
static int print_ppolicy( LDAP *ld, LDAPControl *ctrl );
#endif
static int print_sss( LDAP *ld, LDAPControl *ctrl );
static int print_vlv( LDAP *ld, LDAPControl *ctrl );
#ifdef LDAP_CONTROL_X_DEREF
static int print_deref( LDAP *ld, LDAPControl *ctrl );
#endif
#ifdef LDAP_CONTROL_X_WHATFAILED
static int print_whatfailed( LDAP *ld, LDAPControl *ctrl );
#endif

static struct tool_ctrls_t {
	const char	*oid;
	unsigned	mask;
	print_ctrl_fn	func;
} tool_ctrl_response[] = {
	{ LDAP_CONTROL_PRE_READ,			TOOL_ALL,	print_preread },
	{ LDAP_CONTROL_POST_READ,			TOOL_ALL,	print_postread },
	{ LDAP_CONTROL_PAGEDRESULTS,			TOOL_SEARCH,	print_paged_results },
#ifdef LDAP_CONTROL_AUTHZID_RESPONSE
	/* this is generally deprecated in favor of LDAP WhoAmI? operation, hence only supported as a VC inner control */
	{ LDAP_CONTROL_AUTHZID_RESPONSE,		TOOL_VC,	print_authzid },
#endif
#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
	{ LDAP_CONTROL_PASSWORDPOLICYRESPONSE,		TOOL_ALL,	print_ppolicy },
#endif
	{ LDAP_CONTROL_SORTRESPONSE,	TOOL_SEARCH,	print_sss },
	{ LDAP_CONTROL_VLVRESPONSE,		TOOL_SEARCH,	print_vlv },
#ifdef LDAP_CONTROL_X_DEREF
	{ LDAP_CONTROL_X_DEREF,				TOOL_SEARCH,	print_deref },
#endif
#ifdef LDAP_CONTROL_X_WHATFAILED
	{ LDAP_CONTROL_X_WHATFAILED,			TOOL_ALL,	print_whatfailed },
#endif
	{ NULL,						0,		NULL }
};

/* "features" */
enum { Intr_None = 0, Intr_Abandon, Intr_Cancel, Intr_Ignore }; 
static volatile sig_atomic_t	gotintr, abcan;


#ifdef LDAP_CONTROL_X_SESSION_TRACKING
static int
st_value( LDAP *ld, struct berval *value )
{
	char		*ip = NULL, *name = NULL;
	struct berval	id = { 0 };
	char		namebuf[ MAXHOSTNAMELEN ];

	if ( gethostname( namebuf, sizeof( namebuf ) ) == 0 ) {
		struct hostent	*h;
		struct in_addr	addr;

		name = namebuf;

		h = gethostbyname( name );
		if ( h != NULL ) {
			AC_MEMCPY( &addr, h->h_addr, sizeof( addr ) );
			ip = inet_ntoa( addr );
		}
	}

	if ( sessionTrackingName != NULL ) {
		ber_str2bv( sessionTrackingName , 0, 0, &id );
	} else
#ifdef HAVE_CYRUS_SASL
	if ( sasl_authz_id != NULL ) {
		ber_str2bv( sasl_authz_id, 0, 0, &id );

	} else if ( sasl_authc_id != NULL ) {
		ber_str2bv( sasl_authc_id, 0, 0, &id );

	} else 
#endif /* HAVE_CYRUS_SASL */
	if ( binddn != NULL ) {
		ber_str2bv( binddn, 0, 0, &id );
	}

	if ( ldap_create_session_tracking_value( ld,
		ip, name, LDAP_CONTROL_X_SESSION_TRACKING_USERNAME,
		&id, &stValue ) )
	{
		fprintf( stderr, _("Session tracking control encoding error!\n") );
		return -1;
	}

	return 0;
}
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */

RETSIGTYPE
do_sig( int sig )
{
	gotintr = abcan;
}

void
tool_init( tool_type_t type )
{
	tool_type = type;
	ldap_pvt_setlocale(LC_MESSAGES, "");
	ldap_pvt_bindtextdomain(OPENLDAP_PACKAGE, LDAP_LOCALEDIR);
	ldap_pvt_textdomain(OPENLDAP_PACKAGE);
}

void
tool_destroy( void )
{
	static int destroyed;
	if ( destroyed++ )
		return;

#ifdef HAVE_CYRUS_SASL
	sasl_done();
#endif
#ifdef HAVE_TLS
	ldap_pvt_tls_destroy();
#endif

	if ( ldapuri != NULL ) {
		ber_memfree( ldapuri );
		ldapuri = NULL;
	}

	if ( pr_cookie.bv_val != NULL ) {
		ber_memfree( pr_cookie.bv_val );
		BER_BVZERO( &pr_cookie );
	}

	if ( binddn != NULL ) {
		ber_memfree( binddn );
		binddn = NULL;
	}

	if ( passwd.bv_val != NULL ) {
		ber_memfree( passwd.bv_val );
		BER_BVZERO( &passwd );
	}

#ifdef HAVE_CYRUS_SASL
	if ( sasl_mech != NULL ) {
		ber_memfree( sasl_mech );
		sasl_mech = NULL;
	}
#endif /* HAVE_CYRUS_SASL */

	if ( infile != NULL ) {
		ber_memfree( infile );
		infile = NULL;
	}

	if ( assertion ) {
		ber_memfree( assertion );
		assertion = NULL;
	}

	if ( authzid ) {
		ber_memfree( authzid );
		authzid = NULL;
	}

	if ( proxydn ) {
		ber_memfree( proxydn );
		proxydn = NULL;
	}

	if ( preread_attrs ) {
		ber_memfree( preread_attrs );
		preread_attrs = NULL;
	}

	if ( postread_attrs ) {
		ber_memfree( postread_attrs );
		postread_attrs = NULL;
	}

#ifdef LDAP_CONTROL_X_SESSION_TRACKING
	if ( !BER_BVISNULL( &stValue ) ) {
		ber_memfree( stValue.bv_val );
		BER_BVZERO( &stValue );
	}

	if ( sessionTrackingName ) {
		ber_memfree( sessionTrackingName );
		sessionTrackingName = NULL;
	}
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */
}

void
tool_common_usage( void )
{
	static const char *const descriptions[] = {
N_("  -d level   set LDAP debugging level to `level'\n"),
N_("  -D binddn  bind DN\n"),
N_("  -e [!]<ext>[=<extparam>] general extensions (! indicates criticality)\n")
N_("             [!]assert=<filter>     (RFC 4528; a RFC 4515 Filter string)\n")
N_("             [!]authzid=<authzid>   (RFC 4370; \"dn:<dn>\" or \"u:<user>\")\n")
N_("             [!]bauthzid            (RFC 3829)\n")
#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
#if 0
                 /* non-advertized support for proxyDN */
N_("             [!]proxydn=<dn>        (a RFC 4514 DN string)\n")
#endif
#endif
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
N_("             [!]chaining[=<resolveBehavior>[/<continuationBehavior>]]\n")
N_("                     one of \"chainingPreferred\", \"chainingRequired\",\n")
N_("                     \"referralsPreferred\", \"referralsRequired\"\n")
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */
N_("             [!]manageDSAit         (RFC 3296)\n")
N_("             [!]noop\n")
#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
N_("             ppolicy\n")
#endif
N_("             [!]postread[=<attrs>]  (RFC 4527; comma-separated attr list)\n")
N_("             [!]preread[=<attrs>]   (RFC 4527; comma-separated attr list)\n")
N_("             [!]relax\n")
#ifdef LDAP_CONTROL_X_SESSION_TRACKING
N_("             [!]sessiontracking[=<username>]\n")
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */
N_("             abandon, cancel, ignore (SIGINT sends abandon/cancel,\n"
   "             or ignores response; if critical, doesn't wait for SIGINT.\n"
   "             not really controls)\n")
N_("  -h host    LDAP server\n"),
N_("  -H URI     LDAP Uniform Resource Identifier(s)\n"),
N_("  -I         use SASL Interactive mode\n"),
N_("  -n         show what would be done but don't actually do it\n"),
N_("  -N         do not use reverse DNS to canonicalize SASL host name\n"),
N_("  -O props   SASL security properties\n"),
N_("  -o <opt>[=<optparam>] general options\n"),
N_("             nettimeout=<timeout> (in seconds, or \"none\" or \"max\")\n"),
N_("             ldif-wrap=<width> (in columns, or \"no\" for no wrapping)\n"),
N_("  -p port    port on LDAP server\n"),
N_("  -Q         use SASL Quiet mode\n"),
N_("  -R realm   SASL realm\n"),
N_("  -U authcid SASL authentication identity\n"),
N_("  -v         run in verbose mode (diagnostics to standard output)\n"),
N_("  -V         print version info (-VV only)\n"),
N_("  -w passwd  bind password (for simple authentication)\n"),
N_("  -W         prompt for bind password\n"),
N_("  -x         Simple authentication\n"),
N_("  -X authzid SASL authorization identity (\"dn:<dn>\" or \"u:<user>\")\n"),
N_("  -y file    Read password from file\n"),
N_("  -Y mech    SASL mechanism\n"),
N_("  -Z         Start TLS request (-ZZ to require successful response)\n"),
NULL
	};
	const char *const *cpp;

	fputs( _("Common options:\n"), stderr );
	for( cpp = descriptions; *cpp != NULL; cpp++ ) {
		if( strchr( options, (*cpp)[3] ) || (*cpp)[3] == ' ' ) {
			fputs( _(*cpp), stderr );
		}
	}

	tool_destroy();
}

void tool_perror(
	const char *func,
	int err,
	const char *extra,
	const char *matched,
	const char *info,
	char **refs )
{
	fprintf( stderr, "%s: %s (%d)%s\n",
		func, ldap_err2string( err ), err, extra ? extra : "" );

	if ( matched && *matched ) {
		fprintf( stderr, _("\tmatched DN: %s\n"), matched );
	}

	if ( info && *info ) {
		fprintf( stderr, _("\tadditional info: %s\n"), info );
	}

	if ( refs && *refs ) {
		int i;
		fprintf( stderr, _("\treferrals:\n") );
		for( i=0; refs[i]; i++ ) {
			fprintf( stderr, "\t\t%s\n", refs[i] );
		}
	}
}


void
tool_args( int argc, char **argv )
{
	int i;

	while (( i = getopt( argc, argv, options )) != EOF ) {
		int crit, ival;
		char *control, *cvalue, *next;
		switch( i ) {
		case 'c':	/* continuous operation mode */
			contoper++;
			break;
		case 'C':	/* referrals: obsolete */
			referrals++;
			break;
		case 'd':
			ival = strtol( optarg, &next, 10 );
			if (next == NULL || next[0] != '\0') {
				fprintf( stderr, "%s: unable to parse debug value \"%s\"\n", prog, optarg);
				exit(EXIT_FAILURE);
			}
			debug |= ival;
			break;
		case 'D':	/* bind DN */
			if( binddn != NULL ) {
				fprintf( stderr, "%s: -D previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			binddn = ber_strdup( optarg );
			break;
		case 'e':	/* general extensions (controls and such) */
			/* should be extended to support comma separated list of
			 *	[!]key[=value] parameters, e.g.  -e !foo,bar=567
			 */

			crit = 0;
			cvalue = NULL;
			while ( optarg[0] == '!' ) {
				crit++;
				optarg++;
			}

			control = ber_strdup( optarg );
			if ( (cvalue = strchr( control, '=' )) != NULL ) {
				*cvalue++ = '\0';
			}

			if ( strcasecmp( control, "assert" ) == 0 ) {
				if( assertctl ) {
					fprintf( stderr, "assert control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue == NULL ) {
					fprintf( stderr, "assert: control value expected\n" );
					usage();
				}

				assertctl = 1 + crit;

				assert( assertion == NULL );
				assertion = ber_strdup( cvalue );

			} else if ( strcasecmp( control, "authzid" ) == 0 ) {
				if( authzid != NULL ) {
					fprintf( stderr, "authzid control previously specified\n");
					exit( EXIT_FAILURE );
				}
#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
				if( proxydn != NULL ) {
					fprintf( stderr, "authzid control incompatible with proxydn\n");
					exit( EXIT_FAILURE );
				}
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */
				if( cvalue == NULL ) {
					fprintf( stderr, "authzid: control value expected\n" );
					usage();
				}
				if( !crit ) {
					fprintf( stderr, "authzid: must be marked critical\n" );
					usage();
				} else if ( crit > 1 ) {
					/* purposely flag proxied authorization
					 * as non-critical, to test DSA */
					authzcrit = 0;
				}

				assert( authzid == NULL );
				authzid = ber_strdup( cvalue );

#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
			} else if ( strcasecmp( control, "proxydn" ) == 0 ) {
				if( proxydn != NULL ) {
					fprintf( stderr, "proxydn control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( authzid != NULL ) {
					fprintf( stderr, "proxydn control incompatible with authzid\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue == NULL ) {
					fprintf( stderr, "proxydn: control value expected\n" );
					usage();
				}
				if( !crit ) {
					fprintf( stderr, "proxydn: must be marked critical\n" );
					usage();
				} else if ( crit > 1 ) {
					/* purposely flag proxied authorization
					 * as non-critical, to test DSA */
					authzcrit = 0;
				}

				assert( proxydn == NULL );
				proxydn = ber_strdup( cvalue );
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */

			} else if ( strcasecmp( control, "bauthzid" ) == 0 ) {
				if( bauthzid ) {
					fprintf( stderr, "bauthzid control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr, "bauthzid: no control value expected\n" );
					usage();
				}
				bauthzid = 1 + crit;

			} else if ( ( strcasecmp( control, "relax" ) == 0 ) ||
				( strcasecmp( control, "manageDIT" ) == 0 ) )
			{
				if( manageDIT ) {
					fprintf( stderr,
						"relax control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr,
						"relax: no control value expected\n" );
					usage();
				}

				manageDIT = 1 + crit;

			} else if ( strcasecmp( control, "manageDSAit" ) == 0 ) {
				if( manageDSAit ) {
					fprintf( stderr,
						"manageDSAit control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr,
						"manageDSAit: no control value expected\n" );
					usage();
				}

				manageDSAit = 1 + crit;

			} else if ( strcasecmp( control, "noop" ) == 0 ) {
				if( noop ) {
					fprintf( stderr, "noop control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr, "noop: no control value expected\n" );
					usage();
				}

				noop = 1 + crit;

#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
			} else if ( strcasecmp( control, "ppolicy" ) == 0 ) {
				if( ppolicy ) {
					fprintf( stderr, "ppolicy control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr, "ppolicy: no control value expected\n" );
					usage();
				}
				if( crit ) {
					fprintf( stderr, "ppolicy: critical flag not allowed\n" );
					usage();
				}

				ppolicy = 1;
#endif

			} else if ( strcasecmp( control, "preread" ) == 0 ) {
				if( preread ) {
					fprintf( stderr, "preread control previously specified\n");
					exit( EXIT_FAILURE );
				}

				preread = 1 + crit;
				preread_attrs = ber_strdup( cvalue );

			} else if ( strcasecmp( control, "postread" ) == 0 ) {
				if( postread ) {
					fprintf( stderr, "postread control previously specified\n");
					exit( EXIT_FAILURE );
				}

				postread = 1 + crit;
				postread_attrs = ber_strdup( cvalue );

#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
			} else if ( strcasecmp( control, "chaining" ) == 0 ) {
				if ( chaining ) {
					fprintf( stderr, "chaining control previously specified\n");
					exit( EXIT_FAILURE );
				}

				chaining = 1 + crit;

				if ( cvalue != NULL ) {
					char	*continuation;

					continuation = strchr( cvalue, '/' );
					if ( continuation ) {
						/* FIXME: this makes sense only in searches */
						*continuation++ = '\0';
						if ( strcasecmp( continuation, "chainingPreferred" ) == 0 ) {
							chainingContinuation = LDAP_CHAINING_PREFERRED;
						} else if ( strcasecmp( continuation, "chainingRequired" ) == 0 ) {
							chainingContinuation = LDAP_CHAINING_REQUIRED;
						} else if ( strcasecmp( continuation, "referralsPreferred" ) == 0 ) {
							chainingContinuation = LDAP_REFERRALS_PREFERRED;
						} else if ( strcasecmp( continuation, "referralsRequired" ) == 0 ) {
							chainingContinuation = LDAP_REFERRALS_REQUIRED;
						} else {
							fprintf( stderr,
								"chaining behavior control "
								"continuation value \"%s\" invalid\n",
								continuation );
							exit( EXIT_FAILURE );
						}
					}
	
					if ( strcasecmp( cvalue, "chainingPreferred" ) == 0 ) {
						chainingResolve = LDAP_CHAINING_PREFERRED;
					} else if ( strcasecmp( cvalue, "chainingRequired" ) == 0 ) {
						chainingResolve = LDAP_CHAINING_REQUIRED;
					} else if ( strcasecmp( cvalue, "referralsPreferred" ) == 0 ) {
						chainingResolve = LDAP_REFERRALS_PREFERRED;
					} else if ( strcasecmp( cvalue, "referralsRequired" ) == 0 ) {
						chainingResolve = LDAP_REFERRALS_REQUIRED;
					} else {
						fprintf( stderr,
							"chaining behavior control "
							"resolve value \"%s\" invalid\n",
							cvalue);
						exit( EXIT_FAILURE );
					}
				}
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */

#ifdef LDAP_CONTROL_X_SESSION_TRACKING
			} else if ( strcasecmp( control, "sessiontracking" ) == 0 ) {
				if ( sessionTracking ) {
					fprintf( stderr, "%s: session tracking can be only specified once\n", prog );
					exit( EXIT_FAILURE );
				}
				sessionTracking = 1;
				if ( crit ) {
					fprintf( stderr, "sessiontracking: critical flag not allowed\n" );
					usage();
				}
				if ( cvalue ) {
					sessionTrackingName = ber_strdup( cvalue );
				}
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */

			/* this shouldn't go here, really; but it's a feature... */
			} else if ( strcasecmp( control, "abandon" ) == 0 ) {
				abcan = Intr_Abandon;
				if ( crit ) {
					gotintr = abcan;
				}

			} else if ( strcasecmp( control, "cancel" ) == 0 ) {
				abcan = Intr_Cancel;
				if ( crit ) {
					gotintr = abcan;
				}

			} else if ( strcasecmp( control, "ignore" ) == 0 ) {
				abcan = Intr_Ignore;
				if ( crit ) {
					gotintr = abcan;
				}

			} else if ( tool_is_oid( control ) ) {
				LDAPControl	*tmpctrls, ctrl;

				if ( unknown_ctrls != NULL ) {
					int i;
					for ( i = 0; unknown_ctrls[ i ].ldctl_oid != NULL; i++ ) {
						if ( strcmp( control, unknown_ctrls[ i ].ldctl_oid ) == 0 ) {
							fprintf( stderr, "%s control previously specified\n", control );
							exit( EXIT_FAILURE );
						}
					}
				}

				tmpctrls = (LDAPControl *)ber_memrealloc( unknown_ctrls,
					(unknown_ctrls_num + 1)*sizeof( LDAPControl ) );
				if ( tmpctrls == NULL ) {
					fprintf( stderr, "%s: no memory?\n", prog );
					exit( EXIT_FAILURE );
				}
				unknown_ctrls = tmpctrls;
				ctrl.ldctl_oid = control;
				/* don't free it */
				control = NULL;
				ctrl.ldctl_value.bv_val = NULL;
				ctrl.ldctl_value.bv_len = 0;
				ctrl.ldctl_iscritical = crit;

				if ( cvalue != NULL ) {
					struct berval	bv;
					size_t		len = strlen( cvalue );
					int		retcode;

					bv.bv_len = LUTIL_BASE64_DECODE_LEN( len );
					bv.bv_val = ber_memalloc( bv.bv_len + 1 );

					retcode = lutil_b64_pton( cvalue,
						(unsigned char *)bv.bv_val,
						bv.bv_len );

					if ( retcode == -1 || (unsigned) retcode > bv.bv_len ) {
						fprintf( stderr, "Unable to parse value of general control %s\n",
							control );
						usage();
					}

					bv.bv_len = retcode;
					ctrl.ldctl_value = bv;
				}

				unknown_ctrls[ unknown_ctrls_num ] = ctrl;
				unknown_ctrls_num++;

			} else {
				fprintf( stderr, "Invalid general control name: %s\n",
					control );
				usage();
			}
			if ( control ) {
				ber_memfree( control );	 
				control = NULL;
			}
			break;
		case 'f':	/* read from file */
			if( infile != NULL ) {
				fprintf( stderr, "%s: -f previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			infile = ber_strdup( optarg );
			break;
		case 'h':	/* ldap host */
			if( ldaphost != NULL ) {
				fprintf( stderr, "%s: -h previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ldaphost = ber_strdup( optarg );
			break;
		case 'H':	/* ldap URI */
			if( ldapuri != NULL ) {
				fprintf( stderr, "%s: -H previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ldapuri = ber_strdup( optarg );
			break;
		case 'I':
#ifdef HAVE_CYRUS_SASL
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
					"authentication choice\n",
					prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_flags = LDAP_SASL_INTERACTIVE;
			break;
#else
			fprintf( stderr, "%s: was not compiled with SASL support\n",
				prog );
			exit( EXIT_FAILURE );
#endif
		case 'M':
			/* enable Manage DSA IT */
			manageDSAit++;
			break;
		case 'n':	/* print operations, don't actually do them */
			dont++;
			break;
		case 'N':
			nocanon++;
			break;
		case 'o':
			control = ber_strdup( optarg );
			if ( (cvalue = strchr( control, '=' )) != NULL ) {
				*cvalue++ = '\0';
			}

			if ( strcasecmp( control, "nettimeout" ) == 0 ) {
				if( nettimeout.tv_sec != -1 ) {
					fprintf( stderr, "nettimeout option previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue == NULL || cvalue[0] == '\0' ) {
					fprintf( stderr, "nettimeout: option value expected\n" );
					usage();
				}
		 		if ( strcasecmp( cvalue, "none" ) == 0 ) {
		 			nettimeout.tv_sec = 0;
		 		} else if ( strcasecmp( cvalue, "max" ) == 0 ) {
		 			nettimeout.tv_sec = LDAP_MAXINT;
		 		} else {
		 			ival = strtol( cvalue, &next, 10 );
		 			if ( next == NULL || next[0] != '\0' ) {
		 				fprintf( stderr,
		 					_("Unable to parse network timeout \"%s\"\n"), cvalue );
		 				exit( EXIT_FAILURE );
		 			}
		 			nettimeout.tv_sec = ival;
		 		}
		 		if( nettimeout.tv_sec < 0 || nettimeout.tv_sec > LDAP_MAXINT ) {
		 			fprintf( stderr, _("%s: invalid network timeout (%ld) specified\n"),
		 				prog, (long)nettimeout.tv_sec );
	 				exit( EXIT_FAILURE );
 				}

			} else if ( strcasecmp( control, "ldif-wrap" ) == 0 ) {
				if ( cvalue == 0 ) {
					ldif_wrap = LDIF_LINE_WIDTH;

				} else if ( strcasecmp( cvalue, "no" ) == 0 ) {
					ldif_wrap = LDIF_LINE_WIDTH_MAX;

				} else {
					unsigned int u;
					if ( lutil_atou( &u, cvalue ) ) {
						fprintf( stderr,
							_("Unable to parse ldif-wrap=\"%s\"\n"), cvalue );
		 				exit( EXIT_FAILURE );
					}
					ldif_wrap = (ber_len_t)u;
				}

			} else {
				fprintf( stderr, "Invalid general option name: %s\n",
					control );
				usage();
			}
			ber_memfree(control);
			break;
		case 'O':
#ifdef HAVE_CYRUS_SASL
			if( sasl_secprops != NULL ) {
				fprintf( stderr, "%s: -O previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
					"authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_secprops = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'p':
			if( ldapport ) {
				fprintf( stderr, "%s: -p previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ival = strtol( optarg, &next, 10 );
			if ( next == NULL || next[0] != '\0' ) {
				fprintf( stderr, "%s: unable to parse port number \"%s\"\n", prog, optarg );
				exit( EXIT_FAILURE );
			}
			ldapport = ival;
			break;
		case 'P':
			ival = strtol( optarg, &next, 10 );
			if ( next == NULL || next[0] != '\0' ) {
				fprintf( stderr, "%s: unable to parse protocol version \"%s\"\n", prog, optarg );
				exit( EXIT_FAILURE );
			}
			switch( ival ) {
			case 2:
				if( protocol == LDAP_VERSION3 ) {
					fprintf( stderr, "%s: -P 2 incompatible with version %d\n",
						prog, protocol );
					exit( EXIT_FAILURE );
				}
				protocol = LDAP_VERSION2;
				break;
			case 3:
				if( protocol == LDAP_VERSION2 ) {
					fprintf( stderr, "%s: -P 2 incompatible with version %d\n",
						prog, protocol );
					exit( EXIT_FAILURE );
				}
				protocol = LDAP_VERSION3;
				break;
			default:
				fprintf( stderr, "%s: protocol version should be 2 or 3\n",
					prog );
				usage();
			}
			break;
		case 'Q':
#ifdef HAVE_CYRUS_SASL
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
					"authentication choice\n",
					prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_flags = LDAP_SASL_QUIET;
			break;
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
				prog );
			exit( EXIT_FAILURE );
#endif
		case 'R':
#ifdef HAVE_CYRUS_SASL
			if( sasl_realm != NULL ) {
				fprintf( stderr, "%s: -R previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
					"authentication choice\n",
					prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_realm = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
				prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'U':
#ifdef HAVE_CYRUS_SASL
			if( sasl_authc_id != NULL ) {
				fprintf( stderr, "%s: -U previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
					"authentication choice\n",
					prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_authc_id = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
				prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'v':	/* verbose mode */
			verbose++;
			break;
		case 'V':	/* version */
			version++;
			break;
		case 'w':	/* password */
			passwd.bv_val = ber_strdup( optarg );
			{
				char* p;

				for( p = optarg; *p != '\0'; p++ ) {
					*p = '\0';
				}
			}
			passwd.bv_len = strlen( passwd.bv_val );
			break;
		case 'W':
			want_bindpw++;
			break;
		case 'y':
			pw_file = optarg;
			break;
		case 'Y':
#ifdef HAVE_CYRUS_SASL
			if( sasl_mech != NULL ) {
				fprintf( stderr, "%s: -Y previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr,
					"%s: incompatible with authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_mech = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'x':
			if( authmethod != -1 && authmethod != LDAP_AUTH_SIMPLE ) {
				fprintf( stderr, "%s: incompatible with previous "
					"authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SIMPLE;
			break;
		case 'X':
#ifdef HAVE_CYRUS_SASL
			if( sasl_authz_id != NULL ) {
				fprintf( stderr, "%s: -X previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: -X incompatible with "
					"authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_authz_id = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'Z':
#ifdef HAVE_TLS
			use_tls++;
#else
			fprintf( stderr, "%s: not compiled with TLS support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		default:
			if( handle_private_option( i ) ) break;
			fprintf( stderr, "%s: unrecognized option -%c\n",
				prog, optopt );
			usage();
		}
	}

	{
		/* prevent bad linking */
		LDAPAPIInfo api;
		api.ldapai_info_version = LDAP_API_INFO_VERSION;

		if ( ldap_get_option(NULL, LDAP_OPT_API_INFO, &api)
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "%s: ldap_get_option(API_INFO) failed\n", prog );
			exit( EXIT_FAILURE );
		}

		if (api.ldapai_info_version != LDAP_API_INFO_VERSION) {
			fprintf( stderr, "LDAP APIInfo version mismatch: "
				"library %d, header %d\n",
				api.ldapai_info_version, LDAP_API_INFO_VERSION );
			exit( EXIT_FAILURE );
		}

		if( api.ldapai_api_version != LDAP_API_VERSION ) {
			fprintf( stderr, "LDAP API version mismatch: "
				"library %d, header %d\n",
				api.ldapai_api_version, LDAP_API_VERSION );
			exit( EXIT_FAILURE );
		}

		if( strcmp(api.ldapai_vendor_name, LDAP_VENDOR_NAME ) != 0 ) {
			fprintf( stderr, "LDAP vendor name mismatch: "
				"library %s, header %s\n",
				api.ldapai_vendor_name, LDAP_VENDOR_NAME );
			exit( EXIT_FAILURE );
		}

		if( api.ldapai_vendor_version != LDAP_VENDOR_VERSION ) {
			fprintf( stderr, "LDAP vendor version mismatch: "
				"library %d, header %d\n",
				api.ldapai_vendor_version, LDAP_VENDOR_VERSION );
			exit( EXIT_FAILURE );
		}

		if (version) {
			fprintf( stderr, "%s: %s\t(LDAP library: %s %d)\n",
				prog, __Version,
				LDAP_VENDOR_NAME, LDAP_VENDOR_VERSION );
			if (version > 1) exit( EXIT_SUCCESS );
		}

		ldap_memfree( api.ldapai_vendor_name );
		ber_memvfree( (void **)api.ldapai_extensions );
	}

	if (protocol == -1)
		protocol = LDAP_VERSION3;

	if (authmethod == -1 && protocol > LDAP_VERSION2) {
#ifdef HAVE_CYRUS_SASL
		if ( binddn != NULL ) {
			authmethod = LDAP_AUTH_SIMPLE;
		} else {
			authmethod = LDAP_AUTH_SASL;
		}
#else
		authmethod = LDAP_AUTH_SIMPLE;
#endif
	}

	if( ldapuri == NULL ) {
		if( ldapport && ( ldaphost == NULL )) {
			fprintf( stderr, "%s: -p without -h is invalid.\n", prog );
			exit( EXIT_FAILURE );
		}
	} else {
		if( ldaphost != NULL ) {
			fprintf( stderr, "%s: -H incompatible with -h\n", prog );
			exit( EXIT_FAILURE );
		}
		if( ldapport ) {
			fprintf( stderr, "%s: -H incompatible with -p\n", prog );
			exit( EXIT_FAILURE );
		}
	}

	if( protocol == LDAP_VERSION2 ) {
		if( assertctl || authzid || manageDIT || manageDSAit ||
#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
			proxydn ||
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
			chaining ||
#endif
#ifdef LDAP_CONTROL_X_SESSION_TRACKING
			sessionTracking ||
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */
			noop || ppolicy || preread || postread )
		{
			fprintf( stderr, "%s: -e/-M incompatible with LDAPv2\n", prog );
			exit( EXIT_FAILURE );
		}
#ifdef HAVE_TLS
		if( use_tls ) {
			fprintf( stderr, "%s: -Z incompatible with LDAPv2\n", prog );
			exit( EXIT_FAILURE );
		}
#endif
#ifdef HAVE_CYRUS_SASL
		if( authmethod == LDAP_AUTH_SASL ) {
			fprintf( stderr, "%s: -[IOQRUXY] incompatible with LDAPv2\n",
				prog );
			exit( EXIT_FAILURE );
		}
#endif
	}

	if ( ( pw_file || want_bindpw ) && !BER_BVISNULL( &passwd ) ) {
		fprintf( stderr, "%s: -%c incompatible with -w\n",
			prog, ( pw_file ? 'y' : 'W' ) );
		exit( EXIT_FAILURE );
	}
}


LDAP *
tool_conn_setup( int dont, void (*private_setup)( LDAP * ) )
{
	LDAP *ld = NULL;

	if ( debug ) {
		if( ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &debug )
			!= LBER_OPT_SUCCESS )
		{
			fprintf( stderr,
				"Could not set LBER_OPT_DEBUG_LEVEL %d\n", debug );
		}
		if( ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &debug )
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr,
				"Could not set LDAP_OPT_DEBUG_LEVEL %d\n", debug );
		}
	}

#ifdef SIGPIPE
	(void) SIGNAL( SIGPIPE, SIG_IGN );
#endif

	if ( abcan ) {
		SIGNAL( SIGINT, do_sig );
	}

	if ( !dont ) {
		int rc;

		if( ( ldaphost != NULL || ldapport ) && ( ldapuri == NULL ) ) {
			/* construct URL */
			LDAPURLDesc url;
			memset( &url, 0, sizeof(url));

			url.lud_scheme = "ldap";
			url.lud_host = ldaphost;
			url.lud_port = ldapport;
			url.lud_scope = LDAP_SCOPE_DEFAULT;

			ldapuri = ldap_url_desc2str( &url );

		} else if ( ldapuri != NULL ) {
			LDAPURLDesc	*ludlist, **ludp;
			char		**urls = NULL;
			int		nurls = 0;

			rc = ldap_url_parselist( &ludlist, ldapuri );
			if ( rc != LDAP_URL_SUCCESS ) {
				fprintf( stderr,
					"Could not parse LDAP URI(s)=%s (%d)\n",
					ldapuri, rc );
				exit( EXIT_FAILURE );
			}

			for ( ludp = &ludlist; *ludp != NULL; ) {
				LDAPURLDesc	*lud = *ludp;
				char		**tmp;

				if ( lud->lud_dn != NULL && lud->lud_dn[ 0 ] != '\0' &&
					( lud->lud_host == NULL || lud->lud_host[0] == '\0' ) )
				{
					/* if no host but a DN is provided,
					 * use DNS SRV to gather the host list
					 * and turn it into a list of URIs
					 * using the scheme provided */
					char	*domain = NULL,
						*hostlist = NULL,
						**hosts = NULL;
					int	i,
						len_proto = strlen( lud->lud_scheme );

					if ( ldap_dn2domain( lud->lud_dn, &domain )
						|| domain == NULL )
					{
						fprintf( stderr,
							"DNS SRV: Could not turn "
							"DN=\"%s\" into a domain\n",
							lud->lud_dn );
						goto dnssrv_free;
					}
					
					rc = ldap_domain2hostlist( domain, &hostlist );
					if ( rc ) {
						fprintf( stderr,
							"DNS SRV: Could not turn "
							"domain=%s into a hostlist\n",
							domain );
						goto dnssrv_free;
					}

					hosts = ldap_str2charray( hostlist, " " );
					if ( hosts == NULL ) {
						fprintf( stderr,
							"DNS SRV: Could not parse "
							"hostlist=\"%s\"\n",
							hostlist );
						goto dnssrv_free;
					}

					for ( i = 0; hosts[ i ] != NULL; i++ )
						/* count'em */ ;

					tmp = (char **)ber_memrealloc( urls, sizeof( char * ) * ( nurls + i + 1 ) );
					if ( tmp == NULL ) {
						fprintf( stderr,
							"DNS SRV: out of memory?\n" );
						goto dnssrv_free;
					}
					urls = tmp;
					urls[ nurls ] = NULL;

					for ( i = 0; hosts[ i ] != NULL; i++ ) {
						size_t	len = len_proto
							+ STRLENOF( "://" )
							+ strlen( hosts[ i ] )
							+ 1;

						urls[ nurls + i + 1 ] = NULL;
						urls[ nurls + i ] = (char *)malloc( sizeof( char ) * len );
						if ( urls[ nurls + i ] == NULL ) {
							fprintf( stderr,
								"DNS SRV: out of memory?\n" );
							goto dnssrv_free;
						}

						snprintf( urls[ nurls + i ], len, "%s://%s",
							lud->lud_scheme, hosts[ i ] );
					}
					nurls += i;

dnssrv_free:;
					ber_memvfree( (void **)hosts );
					ber_memfree( hostlist );
					ber_memfree( domain );

				} else {
					tmp = (char **)ber_memrealloc( urls, sizeof( char * ) * ( nurls + 2 ) );
					if ( tmp == NULL ) {
						fprintf( stderr,
							"DNS SRV: out of memory?\n" );
						break;
					}
					urls = tmp;
					urls[ nurls + 1 ] = NULL;

					urls[ nurls ] = ldap_url_desc2str( lud );
					if ( urls[ nurls ] == NULL ) {
						fprintf( stderr,
							"DNS SRV: out of memory?\n" );
						break;
					}
					nurls++;
				}

				*ludp = lud->lud_next;

				lud->lud_next = NULL;
				ldap_free_urldesc( lud );
			}

			if ( ludlist != NULL ) {
				ldap_free_urllist( ludlist );
				exit( EXIT_FAILURE );

			} else if ( urls == NULL ) {
				exit( EXIT_FAILURE );
			}

			ldap_memfree( ldapuri );
			ldapuri = ldap_charray2str( urls, " " );
			ber_memvfree( (void **)urls );
		}

		if ( verbose ) {
			fprintf( stderr, "ldap_initialize( %s )\n",
				ldapuri != NULL ? ldapuri : "<DEFAULT>" );
		}
		rc = ldap_initialize( &ld, ldapuri );
		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr,
				"Could not create LDAP session handle for URI=%s (%d): %s\n",
				ldapuri, rc, ldap_err2string(rc) );
			exit( EXIT_FAILURE );
		}

		if( private_setup ) private_setup( ld );

		/* referrals: obsolete */
		if( ldap_set_option( ld, LDAP_OPT_REFERRALS,
			referrals ? LDAP_OPT_ON : LDAP_OPT_OFF ) != LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LDAP_OPT_REFERRALS %s\n",
				referrals ? "on" : "off" );
			tool_exit( ld, EXIT_FAILURE );
		}

#ifdef HAVE_CYRUS_SASL
		/* canon */
		if( nocanon ) {
			if( ldap_set_option( ld, LDAP_OPT_X_SASL_NOCANON,
				LDAP_OPT_ON ) != LDAP_OPT_SUCCESS )
			{
				fprintf( stderr, "Could not set LDAP_OPT_X_SASL_NOCANON on\n" );
				tool_exit( ld, EXIT_FAILURE );
			}
		}
#endif
		if( ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &protocol )
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LDAP_OPT_PROTOCOL_VERSION %d\n",
				protocol );
			tool_exit( ld, EXIT_FAILURE );
		}

		if ( use_tls ) {
			rc = ldap_start_tls_s( ld, NULL, NULL );
			if ( rc != LDAP_SUCCESS ) {
				char *msg=NULL;
				ldap_get_option( ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&msg);
				tool_perror( "ldap_start_tls", rc, NULL, NULL, msg, NULL );
				ldap_memfree(msg);
				if ( use_tls > 1 ) {
					tool_exit( ld, EXIT_FAILURE );
				}
			}
		}

		if ( nettimeout.tv_sec > 0 ) {
	 		if ( ldap_set_option( ld, LDAP_OPT_NETWORK_TIMEOUT, (void *) &nettimeout )
				!= LDAP_OPT_SUCCESS )
			{
		 		fprintf( stderr, "Could not set LDAP_OPT_NETWORK_TIMEOUT %ld\n",
					(long)nettimeout.tv_sec );
	 			tool_exit( ld, EXIT_FAILURE );
			}
		}
	}

	return ld;
}


void
tool_bind( LDAP *ld )
{
	LDAPControl	**sctrlsp = NULL;
	LDAPControl	*sctrls[4];
	LDAPControl	sctrl[3];
	int		nsctrls = 0;

	int rc, msgid;
	LDAPMessage *result = NULL;

	int err;
	char *matched = NULL;
	char *info = NULL;
	char **refs = NULL;
	LDAPControl **ctrls = NULL;
	char msgbuf[256];

	msgbuf[0] = 0;

#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
	if ( ppolicy ) {
		LDAPControl c;
		c.ldctl_oid = LDAP_CONTROL_PASSWORDPOLICYREQUEST;
		c.ldctl_value.bv_val = NULL;
		c.ldctl_value.bv_len = 0;
		c.ldctl_iscritical = 0;
		sctrl[nsctrls] = c;
		sctrls[nsctrls] = &sctrl[nsctrls];
		sctrls[++nsctrls] = NULL;
	}
#endif

	if ( bauthzid ) {
		LDAPControl c;

		c.ldctl_oid = LDAP_CONTROL_AUTHZID_REQUEST;
		c.ldctl_iscritical = bauthzid > 1;
		BER_BVZERO( &c.ldctl_value );

		sctrl[nsctrls] = c;
		sctrls[nsctrls] = &sctrl[nsctrls];
		sctrls[++nsctrls] = NULL;
	}

#ifdef LDAP_CONTROL_X_SESSION_TRACKING
	if ( sessionTracking ) {
		LDAPControl c;

		if ( BER_BVISNULL( &stValue) && st_value( ld, &stValue ) ) {
			tool_exit( ld, EXIT_FAILURE );
		}

		c.ldctl_oid = LDAP_CONTROL_X_SESSION_TRACKING;
		c.ldctl_iscritical = 0;
		c.ldctl_value = stValue;

		sctrl[nsctrls] = c;
		sctrls[nsctrls] = &sctrl[nsctrls];
		sctrls[++nsctrls] = NULL;
	}
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */

	if ( nsctrls ) {
		sctrlsp = sctrls;
	}

	assert( nsctrls < (int) (sizeof(sctrls)/sizeof(sctrls[0])) );

	if ( pw_file || want_bindpw ) {
		assert( passwd.bv_val == NULL && passwd.bv_len == 0 );

		if ( pw_file ) {
			if ( lutil_get_filed_password( pw_file, &passwd ) ) {
				tool_exit( ld, EXIT_FAILURE );
			}

		} else {
			char *pw = getpassphrase( _("Enter LDAP Password: ") );
			if ( pw ) {
				passwd.bv_val = ber_strdup( pw );
				passwd.bv_len = strlen( passwd.bv_val );
			}
		}
	}

	if ( authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void *defaults;
		const char *rmech = NULL;

		if( sasl_secprops != NULL ) {
			rc = ldap_set_option( ld, LDAP_OPT_X_SASL_SECPROPS,
				(void *) sasl_secprops );

			if( rc != LDAP_OPT_SUCCESS ) {
				fprintf( stderr,
					"Could not set LDAP_OPT_X_SASL_SECPROPS: %s\n",
					sasl_secprops );
				tool_exit( ld, LDAP_LOCAL_ERROR );
			}
		}

		defaults = lutil_sasl_defaults( ld,
			sasl_mech,
			sasl_realm,
			sasl_authc_id,
			passwd.bv_val,
			sasl_authz_id );

		do {
			rc = ldap_sasl_interactive_bind( ld, binddn, sasl_mech,
				sctrlsp, NULL, sasl_flags, lutil_sasl_interact, defaults,
				result, &rmech, &msgid );

			if ( rc != LDAP_SASL_BIND_IN_PROGRESS )
				break;

			ldap_msgfree( result );

			if ( ldap_result( ld, msgid, LDAP_MSG_ALL, NULL, &result ) == -1 || !result ) {
				ldap_get_option( ld, LDAP_OPT_RESULT_CODE, (void*)&err );
				ldap_get_option( ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&info );
				tool_perror( "ldap_sasl_interactive_bind",
					err, NULL, NULL, info, NULL );
				ldap_memfree( info );
				tool_exit( ld, err );
			}
		} while ( rc == LDAP_SASL_BIND_IN_PROGRESS );

		lutil_sasl_freedefs( defaults );

		if ( rc != LDAP_SUCCESS ) {
			ldap_get_option( ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&info );
			tool_perror( "ldap_sasl_interactive_bind",
				rc, NULL, NULL, info, NULL );
			ldap_memfree( info );
			tool_exit( ld, rc );
		}
#else
		fprintf( stderr, "%s: not compiled with SASL support\n", prog );
		tool_exit( ld, LDAP_NOT_SUPPORTED );
#endif
	} else {
		/* simple bind */
		rc = ldap_sasl_bind( ld, binddn, LDAP_SASL_SIMPLE, &passwd,
			sctrlsp, NULL, &msgid );
		if ( msgid == -1 ) {
			tool_perror( "ldap_sasl_bind(SIMPLE)", rc,
				NULL, NULL, NULL, NULL );
			tool_exit( ld, rc );
		}

		rc = ldap_result( ld, msgid, LDAP_MSG_ALL, NULL, &result );
		if ( rc == -1 ) {
			tool_perror( "ldap_result", -1, NULL, NULL, NULL, NULL );
			tool_exit( ld, LDAP_LOCAL_ERROR );
		}

		if ( rc == 0 ) {
			tool_perror( "ldap_result", LDAP_TIMEOUT, NULL, NULL, NULL, NULL );
			tool_exit( ld, LDAP_LOCAL_ERROR );
		}
	}

	if ( result ) {
		rc = ldap_parse_result( ld, result, &err, &matched, &info, &refs,
		                        &ctrls, 1 );
		if ( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_bind parse result", rc, NULL, matched, info, refs );
			tool_exit( ld, LDAP_LOCAL_ERROR );
		}
	}

#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
	if ( ctrls && ppolicy ) {
		LDAPControl *ctrl;
		int expire, grace, len = 0;
		LDAPPasswordPolicyError pErr = -1;
		
		ctrl = ldap_control_find( LDAP_CONTROL_PASSWORDPOLICYRESPONSE,
			ctrls, NULL );

		if ( ctrl && ldap_parse_passwordpolicy_control( ld, ctrl,
			&expire, &grace, &pErr ) == LDAP_SUCCESS )
		{
			if ( pErr != PP_noError ){
				msgbuf[0] = ';';
				msgbuf[1] = ' ';
				strcpy( msgbuf+2, ldap_passwordpolicy_err2txt( pErr ));
				len = strlen( msgbuf );
			}
			if ( expire >= 0 ) {
				sprintf( msgbuf+len,
					" (Password expires in %d seconds)",
					expire );
			} else if ( grace >= 0 ) {
				sprintf( msgbuf+len,
					" (Password expired, %d grace logins remain)",
					grace );
			}
		}
	}
#endif

	if ( ctrls && bauthzid ) {
		LDAPControl *ctrl;
		
		ctrl = ldap_control_find( LDAP_CONTROL_AUTHZID_RESPONSE,
			ctrls, NULL );
		if ( ctrl ) {
			LDAPControl *ctmp[2];
			ctmp[0] = ctrl;
			ctmp[1] = NULL;
			tool_print_ctrls( ld, ctmp );
		}
	}

	if ( ctrls ) {
		ldap_controls_free( ctrls );
	}

	if ( err != LDAP_SUCCESS
		|| msgbuf[0]
		|| ( matched && matched[ 0 ] )
		|| ( info && info[ 0 ] )
		|| refs )
	{
		tool_perror( "ldap_bind", err, msgbuf, matched, info, refs );

		if( matched ) ber_memfree( matched );
		if( info ) ber_memfree( info );
		if( refs ) ber_memvfree( (void **)refs );

		if ( err != LDAP_SUCCESS ) tool_exit( ld, err );
	}
}

void
tool_unbind( LDAP *ld )
{
	int err = ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, NULL );

	if ( err != LDAP_OPT_SUCCESS ) {
		fprintf( stderr, "Could not unset controls\n");
	}

	(void) ldap_unbind_ext( ld, NULL, NULL );
}

void
tool_exit( LDAP *ld, int status )
{
	if ( ld != NULL ) {
		tool_unbind( ld );
	}
	tool_destroy();
	exit( status );
}


/* Set server controls.  Add controls extra_c[0..count-1], if set. */
void
tool_server_controls( LDAP *ld, LDAPControl *extra_c, int count )
{
	int i = 0, j, crit = 0, err;
	LDAPControl c[16], **ctrls;

	if ( ! ( assertctl
		|| authzid
#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
		|| proxydn
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */
		|| manageDIT
		|| manageDSAit
		|| noop
#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
		|| ppolicy
#endif
		|| preread
		|| postread
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
		|| chaining
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */
#ifdef LDAP_CONTROL_X_SESSION_TRACKING
		|| sessionTracking
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */
		|| count
		|| unknown_ctrls_num ) )
	{
		return;
	}

	ctrls = (LDAPControl**) malloc(sizeof(c) + (count + unknown_ctrls_num + 1)*sizeof(LDAPControl*));
	if ( ctrls == NULL ) {
		fprintf( stderr, "No memory\n" );
		tool_exit( ld, EXIT_FAILURE );
	}

	if ( assertctl ) {
		if ( BER_BVISNULL( &assertionvalue ) ) {
			err = ldap_create_assertion_control_value( ld,
				assertion, &assertionvalue );
			if ( err ) {
				fprintf( stderr,
					"Unable to create assertion value "
					"\"%s\" (%d)\n", assertion, err );
			}
		}

		c[i].ldctl_oid = LDAP_CONTROL_ASSERT;
		c[i].ldctl_value = assertionvalue;
		c[i].ldctl_iscritical = assertctl > 1;
		ctrls[i] = &c[i];
		i++;
	}

	if ( authzid ) {
		c[i].ldctl_value.bv_val = authzid;
		c[i].ldctl_value.bv_len = strlen( authzid );
		c[i].ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
		c[i].ldctl_iscritical = authzcrit;
		ctrls[i] = &c[i];
		i++;
	}

#ifdef LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ
	/* NOTE: doesn't need an extra count because it's incompatible
	 * with authzid */
	if ( proxydn ) {
		BerElementBuffer berbuf;
		BerElement *ber = (BerElement *)&berbuf;
		
		ber_init2( ber, NULL, LBER_USE_DER );

		if ( ber_printf( ber, "s", proxydn ) == -1 ) {
			tool_exit( ld, EXIT_FAILURE );
		}

		if ( ber_flatten2( ber, &c[i].ldctl_value, 0 ) == -1 ) {
			tool_exit( ld, EXIT_FAILURE );
		}

		c[i].ldctl_oid = LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ;
		c[i].ldctl_iscritical = authzcrit;
		ctrls[i] = &c[i];
		i++;
	}
#endif /* LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ */

	if ( manageDIT ) {
		c[i].ldctl_oid = LDAP_CONTROL_MANAGEDIT;
		BER_BVZERO( &c[i].ldctl_value );
		c[i].ldctl_iscritical = manageDIT > 1;
		ctrls[i] = &c[i];
		i++;
	}

	if ( manageDSAit ) {
		c[i].ldctl_oid = LDAP_CONTROL_MANAGEDSAIT;
		BER_BVZERO( &c[i].ldctl_value );
		c[i].ldctl_iscritical = manageDSAit > 1;
		ctrls[i] = &c[i];
		i++;
	}

	if ( noop ) {
		c[i].ldctl_oid = LDAP_CONTROL_NOOP;
		BER_BVZERO( &c[i].ldctl_value );
		c[i].ldctl_iscritical = noop > 1;
		ctrls[i] = &c[i];
		i++;
	}

#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
	if ( ppolicy ) {
		c[i].ldctl_oid = LDAP_CONTROL_PASSWORDPOLICYREQUEST;
		BER_BVZERO( &c[i].ldctl_value );
		c[i].ldctl_iscritical = 0;
		ctrls[i] = &c[i];
		i++;
	}
#endif

	if ( preread ) {
		BerElementBuffer berbuf;
		BerElement *ber = (BerElement *)&berbuf;
		char **attrs = NULL;

		if( preread_attrs ) {
			attrs = ldap_str2charray( preread_attrs, "," );
		}

		ber_init2( ber, NULL, LBER_USE_DER );

		if( ber_printf( ber, "{v}", attrs ) == -1 ) {
			fprintf( stderr, "preread attrs encode failed.\n" );
			tool_exit( ld, EXIT_FAILURE );
		}

		err = ber_flatten2( ber, &c[i].ldctl_value, 0 );
		if( err < 0 ) {
			fprintf( stderr, "preread flatten failed (%d)\n", err );
			tool_exit( ld, EXIT_FAILURE );
		}

		c[i].ldctl_oid = LDAP_CONTROL_PRE_READ;
		c[i].ldctl_iscritical = preread > 1;
		ctrls[i] = &c[i];
		i++;

		if( attrs ) ldap_charray_free( attrs );
	}

	if ( postread ) {
		BerElementBuffer berbuf;
		BerElement *ber = (BerElement *)&berbuf;
		char **attrs = NULL;

		if( postread_attrs ) {
			attrs = ldap_str2charray( postread_attrs, "," );
		}

		ber_init2( ber, NULL, LBER_USE_DER );

		if( ber_printf( ber, "{v}", attrs ) == -1 ) {
			fprintf( stderr, "postread attrs encode failed.\n" );
			tool_exit( ld, EXIT_FAILURE );
		}

		err = ber_flatten2( ber, &c[i].ldctl_value, 0 );
		if( err < 0 ) {
			fprintf( stderr, "postread flatten failed (%d)\n", err );
			tool_exit( ld, EXIT_FAILURE );
		}

		c[i].ldctl_oid = LDAP_CONTROL_POST_READ;
		c[i].ldctl_iscritical = postread > 1;
		ctrls[i] = &c[i];
		i++;

		if( attrs ) ldap_charray_free( attrs );
	}

#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
	if ( chaining ) {
		if ( chainingResolve > -1 ) {
			BerElementBuffer berbuf;
			BerElement *ber = (BerElement *)&berbuf;

			ber_init2( ber, NULL, LBER_USE_DER );

			err = ber_printf( ber, "{e" /* } */, chainingResolve );
		    	if ( err == -1 ) {
				ber_free( ber, 1 );
				fprintf( stderr, _("Chaining behavior control encoding error!\n") );
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( chainingContinuation > -1 ) {
				err = ber_printf( ber, "e", chainingContinuation );
		    		if ( err == -1 ) {
					ber_free( ber, 1 );
					fprintf( stderr, _("Chaining behavior control encoding error!\n") );
					tool_exit( ld, EXIT_FAILURE );
				}
			}

			err = ber_printf( ber, /* { */ "N}" );
		    	if ( err == -1 ) {
				ber_free( ber, 1 );
				fprintf( stderr, _("Chaining behavior control encoding error!\n") );
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ber_flatten2( ber, &c[i].ldctl_value, 0 ) == -1 ) {
				tool_exit( ld, EXIT_FAILURE );
			}

		} else {
			BER_BVZERO( &c[i].ldctl_value );
		}

		c[i].ldctl_oid = LDAP_CONTROL_X_CHAINING_BEHAVIOR;
		c[i].ldctl_iscritical = chaining > 1;
		ctrls[i] = &c[i];
		i++;
	}
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */

#ifdef LDAP_CONTROL_X_SESSION_TRACKING
	if ( sessionTracking ) {
		if ( BER_BVISNULL( &stValue ) && st_value( ld, &stValue ) ) {
			tool_exit( ld, EXIT_FAILURE );
		}

		c[i].ldctl_oid = LDAP_CONTROL_X_SESSION_TRACKING;
		c[i].ldctl_iscritical = 0;
		c[i].ldctl_value = stValue;

		ctrls[i] = &c[i];
		i++;
	}
#endif /* LDAP_CONTROL_X_SESSION_TRACKING */

	while ( count-- ) {
		ctrls[i++] = extra_c++;
	}
	for ( count = 0; count < unknown_ctrls_num; count++ ) {
		ctrls[i++] = &unknown_ctrls[count];
	}
	ctrls[i] = NULL;

	err = ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, ctrls );

	if ( err != LDAP_OPT_SUCCESS ) {
		for ( j = 0; j < i; j++ ) {
			if ( ctrls[j]->ldctl_iscritical ) crit = 1;
		}
		fprintf( stderr, "Could not set %scontrols\n",
			crit ? "critical " : "" );
	}

 	free( ctrls );
	if ( crit ) {
		tool_exit( ld, EXIT_FAILURE );
	}
}

int
tool_check_abandon( LDAP *ld, int msgid )
{
	int	rc;

	switch ( gotintr ) {
	case Intr_Cancel:
		rc = ldap_cancel_s( ld, msgid, NULL, NULL );
		fprintf( stderr, "got interrupt, cancel got %d: %s\n",
				rc, ldap_err2string( rc ) );
		return -1;

	case Intr_Abandon:
		rc = ldap_abandon_ext( ld, msgid, NULL, NULL );
		fprintf( stderr, "got interrupt, abandon got %d: %s\n",
				rc, ldap_err2string( rc ) );
		return -1;

	case Intr_Ignore:
		/* just unbind, ignoring the request */
		return -1;
	}

	return 0;
}

static int
print_prepostread( LDAP *ld, LDAPControl *ctrl, struct berval *what)
{
	BerElement	*ber;
	struct berval	bv;

	tool_write_ldif( LDIF_PUT_COMMENT, "==> ",
		what->bv_val, what->bv_len );
	ber = ber_init( &ctrl->ldctl_value );
	if ( ber == NULL ) {
		/* error? */
		return 1;

	} else if ( ber_scanf( ber, "{m{" /*}}*/, &bv ) == LBER_ERROR ) {
		/* error? */
		return 1;

	} else {
		tool_write_ldif( LDIF_PUT_VALUE, "dn", bv.bv_val, bv.bv_len );

		while ( ber_scanf( ber, "{m" /*}*/, &bv ) != LBER_ERROR ) {
			int		i;
			BerVarray	vals = NULL;
			char		*str = NULL;

			if ( ber_scanf( ber, "[W]", &vals ) == LBER_ERROR ||
				vals == NULL )
			{
				/* error? */
				return 1;
			}

			if ( ldif ) {
				char *ptr;

				str = malloc( bv.bv_len + STRLENOF(": ") + 1 );

				ptr = str;
				ptr = lutil_strncopy( ptr, bv.bv_val, bv.bv_len );
				ptr = lutil_strcopy( ptr, ": " );
			}
		
			for ( i = 0; vals[ i ].bv_val != NULL; i++ ) {
				tool_write_ldif(
					ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
					ldif ? str : bv.bv_val, vals[ i ].bv_val, vals[ i ].bv_len );
			}

			ber_bvarray_free( vals );
			if ( str ) free( str );
		}
	}

	if ( ber != NULL ) {
		ber_free( ber, 1 );
	}

	tool_write_ldif( LDIF_PUT_COMMENT, "<== ",
		what->bv_val, what->bv_len );

	return 0;
}

static int
print_preread( LDAP *ld, LDAPControl *ctrl )
{
	static struct berval what = BER_BVC( "preread" );

	return print_prepostread( ld, ctrl, &what );
}

static int
print_postread( LDAP *ld, LDAPControl *ctrl )
{
	static struct berval what = BER_BVC( "postread" );

	return print_prepostread( ld, ctrl, &what );
}

static int
print_paged_results( LDAP *ld, LDAPControl *ctrl )
{
	ber_int_t estimate;

	/* note: pr_cookie is being malloced; it's freed
	 * the next time the control is sent, but the last
	 * time it's not; we don't care too much, because
	 * the last time an empty value is returned... */
	if ( ldap_parse_pageresponse_control( ld, ctrl, &estimate, &pr_cookie )
		!= LDAP_SUCCESS )
	{
		/* error? */
		return 1;

	} else {
		/* FIXME: check buffer overflow */
		char	buf[ BUFSIZ ], *ptr = buf;

		if ( estimate > 0 ) {
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"estimate=%d", estimate );
		}

		if ( pr_cookie.bv_len > 0 ) {
			struct berval	bv;

			bv.bv_len = LUTIL_BASE64_ENCODE_LEN(
				pr_cookie.bv_len ) + 1;
			bv.bv_val = ber_memalloc( bv.bv_len + 1 );

			bv.bv_len = lutil_b64_ntop(
				(unsigned char *) pr_cookie.bv_val,
				pr_cookie.bv_len,
				bv.bv_val, bv.bv_len );

			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"%scookie=%s", ptr == buf ? "" : " ",
				bv.bv_val );

			ber_memfree( bv.bv_val );

			pr_morePagedResults = 1;

		} else {
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"%scookie=", ptr == buf ? "" : " " );
		}

		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "pagedresults: " : "pagedresults",
			buf, ptr - buf );
	}

	return 0;
}

static int
print_sss( LDAP *ld, LDAPControl *ctrl )
{
	int rc;
	ber_int_t err;
	char *attr;

	rc = ldap_parse_sortresponse_control( ld, ctrl, &err, &attr );
	if ( rc == LDAP_SUCCESS ) {
		char buf[ BUFSIZ ];
		rc = snprintf( buf, sizeof(buf), "(%d) %s%s%s",
			err, ldap_err2string(err), attr ? " " : "", attr ? attr : "" );

		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "sortResult: " : "sortResult", buf, rc );
	}

	return rc;
}

static int
print_vlv( LDAP *ld, LDAPControl *ctrl )
{
	int rc;
	ber_int_t err;
	struct berval bv;

	rc = ldap_parse_vlvresponse_control( ld, ctrl, &vlvPos, &vlvCount,
		&vlvContext, &err );
	if ( rc == LDAP_SUCCESS ) {
		char buf[ BUFSIZ ];

		if ( vlvContext && vlvContext->bv_len > 0 ) {
			bv.bv_len = LUTIL_BASE64_ENCODE_LEN(
				vlvContext->bv_len ) + 1;
			bv.bv_val = ber_memalloc( bv.bv_len + 1 );

			bv.bv_len = lutil_b64_ntop(
				(unsigned char *) vlvContext->bv_val,
				vlvContext->bv_len,
				bv.bv_val, bv.bv_len );
		} else {
			bv.bv_val = "";
			bv.bv_len = 0;
		}

		rc = snprintf( buf, sizeof(buf), "pos=%d count=%d context=%s (%d) %s",
			vlvPos, vlvCount, bv.bv_val,
			err, ldap_err2string(err));

		if ( bv.bv_len )
			ber_memfree( bv.bv_val );

		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "vlvResult" : "vlvResult", buf, rc );
	}

	return rc;
}

#ifdef LDAP_CONTROL_X_DEREF
static int
print_deref( LDAP *ld, LDAPControl *ctrl )
{
	LDAPDerefRes    *drhead = NULL, *dr;
	int		rc;

	rc = ldap_parse_derefresponse_control( ld, ctrl, &drhead );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	for ( dr = drhead; dr != NULL; dr = dr->next ) {
		LDAPDerefVal	*dv;
		ber_len_t	len;
		char		*buf, *ptr;

		len = strlen( dr->derefAttr ) + STRLENOF(": ");

		for ( dv = dr->attrVals; dv != NULL; dv = dv->next ) {
			if ( dv->vals != NULL ) {
				int j;
				ber_len_t tlen = strlen(dv->type);

				for ( j = 0; dv->vals[ j ].bv_val != NULL; j++ ) {
					len += STRLENOF("<:=>;") + tlen + 4*((dv->vals[ j ].bv_len - 1)/3 + 1);
				}
			}
		}
		len += dr->derefVal.bv_len + STRLENOF("\n");
		buf = ldap_memalloc( len + 1 );
		if ( buf == NULL ) {
			rc = LDAP_NO_MEMORY;
			goto done;
		}

		ptr = buf;
		ptr = lutil_strcopy( ptr, dr->derefAttr );
		*ptr++ = ':';
		*ptr++ = ' ';
		for ( dv = dr->attrVals; dv != NULL; dv = dv->next ) {
			if ( dv->vals != NULL ) {
				int j;
				for ( j = 0; dv->vals[ j ].bv_val != NULL; j++ ) {
					int k = ldif_is_not_printable( dv->vals[ j ].bv_val, dv->vals[ j ].bv_len );

					*ptr++ = '<';
					ptr = lutil_strcopy( ptr, dv->type );
					if ( k ) {
						*ptr++ = ':';
					}
					*ptr++ = '=';
					if ( k ) {
						k = lutil_b64_ntop(
							(unsigned char *) dv->vals[ j ].bv_val,
							dv->vals[ j ].bv_len,
							ptr, buf + len - ptr );
						assert( k >= 0 );
						ptr += k;
						
					} else {
						ptr = lutil_memcopy( ptr, dv->vals[ j ].bv_val, dv->vals[ j ].bv_len );
					}
					*ptr++ = '>';
					*ptr++ = ';';
				}
			}
		}
		ptr = lutil_strncopy( ptr, dr->derefVal.bv_val, dr->derefVal.bv_len );
		*ptr++ = '\n';
		*ptr = '\0';
		assert( ptr <= buf + len );

		tool_write_ldif( LDIF_PUT_COMMENT, NULL, buf, ptr - buf);

		ldap_memfree( buf );
	}

	rc = LDAP_SUCCESS;

done:;
	ldap_derefresponse_free( drhead );

	return rc;
}
#endif

#ifdef LDAP_CONTROL_X_WHATFAILED
static int
print_whatfailed( LDAP *ld, LDAPControl *ctrl )
{
	BerElement *ber;
	ber_tag_t tag;
	ber_len_t siz;
	BerVarray bva = NULL;

	/* Create a BerElement from the berval returned in the control. */
	ber = ber_init( &ctrl->ldctl_value );

	if ( ber == NULL ) {
		return LDAP_NO_MEMORY;
	}

	siz = sizeof(struct berval);
	tag = ber_scanf( ber, "[M]", &bva, &siz, 0 );
	if ( tag != LBER_ERROR ) {
		int i;

		tool_write_ldif( LDIF_PUT_COMMENT, " what failed:", NULL, 0 );

		for ( i = 0; bva[i].bv_val != NULL; i++ ) {
			tool_write_ldif( LDIF_PUT_COMMENT, NULL, bva[i].bv_val, bva[i].bv_len );
		}

		ldap_memfree( bva );
	}

        ber_free( ber, 1 );


	return 0;
}
#endif

#ifdef LDAP_CONTROL_AUTHZID_RESPONSE
static int
print_authzid( LDAP *ld, LDAPControl *ctrl )
{
	if ( ctrl->ldctl_value.bv_len ) {
		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "authzid: " : "authzid",
		ctrl->ldctl_value.bv_val, ctrl->ldctl_value.bv_len );
	} else {
		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "authzid: " : "authzid",
			"anonymous",  STRLENOF("anonymous") );
	}

	return 0;
}
#endif

#ifdef LDAP_CONTROL_PASSWORDPOLICYREQUEST
static int
print_ppolicy( LDAP *ld, LDAPControl *ctrl )
{
	int expire = 0, grace = 0, rc;
	LDAPPasswordPolicyError	pperr;

	rc = ldap_parse_passwordpolicy_control( ld, ctrl,
		&expire, &grace, &pperr );
	if ( rc == LDAP_SUCCESS ) {
		char	buf[ BUFSIZ ], *ptr = buf;

		if ( expire != -1 ) {
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"expire=%d", expire );
		}

		if ( grace != -1 ) {
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"%sgrace=%d", ptr == buf ? "" : " ", grace );
		}

		if ( pperr != PP_noError ) {
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ),
				"%serror=%d (%s)", ptr == buf ? "" : " ",
				pperr,
				ldap_passwordpolicy_err2txt( pperr ) );
		}

		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			ldif ? "ppolicy: " : "ppolicy", buf, ptr - buf );
	}

	return rc;
}
#endif

void tool_print_ctrls(
	LDAP		*ld,
	LDAPControl	**ctrls )
{
	int	i;
	char	*ptr;

	for ( i = 0; ctrls[i] != NULL; i++ ) {
		/* control: OID criticality base64value */
		struct berval b64 = BER_BVNULL;
		ber_len_t len;
		char *str;
		int j;

		/* FIXME: there might be cases where a control has NULL OID;
		 * this makes little sense, especially when returned by the
		 * server, but libldap happily allows it */
		if ( ctrls[i]->ldctl_oid == NULL ) {
			continue;
		}

		len = ldif ? 2 : 0;
		len += strlen( ctrls[i]->ldctl_oid );

		/* add enough for space after OID and the critical value itself */
		len += ctrls[i]->ldctl_iscritical
			? sizeof("true") : sizeof("false");

		/* convert to base64 */
		if ( !BER_BVISNULL( &ctrls[i]->ldctl_value ) ) {
			b64.bv_len = LUTIL_BASE64_ENCODE_LEN(
				ctrls[i]->ldctl_value.bv_len ) + 1;
			b64.bv_val = ber_memalloc( b64.bv_len + 1 );

			b64.bv_len = lutil_b64_ntop(
				(unsigned char *) ctrls[i]->ldctl_value.bv_val,
				ctrls[i]->ldctl_value.bv_len,
				b64.bv_val, b64.bv_len );
		}

		if ( b64.bv_len ) {
			len += 1 + b64.bv_len;
		}

		ptr = str = malloc( len + 1 );
		if ( ldif ) {
			ptr = lutil_strcopy( ptr, ": " );
		}
		ptr = lutil_strcopy( ptr, ctrls[i]->ldctl_oid );
		ptr = lutil_strcopy( ptr, ctrls[i]->ldctl_iscritical
			? " true" : " false" );

		if ( b64.bv_len ) {
			ptr = lutil_strcopy( ptr, " " );
			ptr = lutil_strcopy( ptr, b64.bv_val );
		}

		if ( ldif < 2 ) {
			tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
				"control", str, len );
		}

		free( str );
		if ( b64.bv_len ) {
			ber_memfree( b64.bv_val );
		}

		/* known controls */
		for ( j = 0; tool_ctrl_response[j].oid != NULL; j++ ) {
			if ( strcmp( tool_ctrl_response[j].oid, ctrls[i]->ldctl_oid ) == 0 ) {
				if ( !tool_ctrl_response[j].mask & tool_type ) {
					/* this control should not appear
					 * with this tool; warning? */
				}
				break;
			}
		}

		if ( tool_ctrl_response[j].oid != NULL && tool_ctrl_response[j].func ) {
			(void)tool_ctrl_response[j].func( ld, ctrls[i] );
		}
	}
}

int
tool_write_ldif( int type, char *name, char *value, ber_len_t vallen )
{
	char	*ldif;

	if (( ldif = ldif_put_wrap( type, name, value, vallen, ldif_wrap )) == NULL ) {
		return( -1 );
	}

	fputs( ldif, stdout );
	ber_memfree( ldif );

	return( 0 );
}

int
tool_is_oid( const char *s )
{
	int		first = 1;

	if ( !isdigit( (unsigned char) s[ 0 ] ) ) {
		return 0;
	}

	for ( ; s[ 0 ]; s++ ) {
		if ( s[ 0 ] == '.' ) {
			if ( s[ 1 ] == '\0' ) {
				return 0;
			}
			first = 1;
			continue;
		}

		if ( !isdigit( (unsigned char) s[ 0 ] ) ) {
			return 0;
		}

		if ( first == 1 && s[ 0 ] == '0' && s[ 1 ] != '.' ) {
			return 0;
		}
		first = 0;
	}

	return 1;
}
