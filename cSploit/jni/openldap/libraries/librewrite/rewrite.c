/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#include <portable.h>

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/syslog.h>
#include <ac/regex.h>
#include <ac/socket.h>
#include <ac/unistd.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <stdio.h>

#include <rewrite.h>
#include <lutil.h>
#include <ldap.h>

int ldap_debug;
int ldap_syslog;
int ldap_syslog_level;

static void
apply( 
		FILE *fin, 
		const char *rewriteContext,
		const char *arg
)
{
	struct rewrite_info *info;
	char *string, *sep, *result = NULL;
	int rc;
	void *cookie = &info;

	info = rewrite_info_init( REWRITE_MODE_ERR );

	if ( rewrite_read( fin, info ) != 0 ) {
		exit( EXIT_FAILURE );
	}

	rewrite_param_set( info, "prog", "rewrite" );

	rewrite_session_init( info, cookie );

	string = (char *)arg;
	for ( sep = strchr( rewriteContext, ',' );
			rewriteContext != NULL;
			rewriteContext = sep,
			sep ? sep = strchr( rewriteContext, ',' ) : NULL )
	{
		char	*errmsg = "";

		if ( sep != NULL ) {
			sep[ 0 ] = '\0';
			sep++;
		}
		/* rc = rewrite( info, rewriteContext, string, &result ); */
		rc = rewrite_session( info, rewriteContext, string,
				cookie, &result );
		
		switch ( rc ) {
		case REWRITE_REGEXEC_OK:
			errmsg = "ok";
			break;

		case REWRITE_REGEXEC_ERR:
			errmsg = "error";
			break;

		case REWRITE_REGEXEC_STOP:
			errmsg = "stop";
			break;

		case REWRITE_REGEXEC_UNWILLING:
			errmsg = "unwilling to perform";
			break;

		default:
			if (rc >= REWRITE_REGEXEC_USER) {
				errmsg = "user-defined";
			} else {
				errmsg = "unknown";
			}
			break;
		}
		
		fprintf( stdout, "%s -> %s [%d:%s]\n", string, 
				( result ? result : "(null)" ),
				rc, errmsg );
		if ( result == NULL ) {
			break;
		}
		if ( string != arg && string != result ) {
			free( string );
		}
		string = result;
	}

	if ( result && result != arg ) {
		free( result );
	}

	rewrite_session_delete( info, cookie );

	rewrite_info_delete( &info );
}

int
main( int argc, char *argv[] )
{
	FILE	*fin = NULL;
	char	*rewriteContext = REWRITE_DEFAULT_CONTEXT;
	int	debug = 0;

	while ( 1 ) {
		int opt = getopt( argc, argv, "d:f:hr:" );

		if ( opt == EOF ) {
			break;
		}

		switch ( opt ) {
		case 'd':
			if ( lutil_atoi( &debug, optarg ) != 0 ) {
				fprintf( stderr, "illegal log level '%s'\n",
						optarg );
				exit( EXIT_FAILURE );
			}
			break;

		case 'f':
			fin = fopen( optarg, "r" );
			if ( fin == NULL ) {
				fprintf( stderr, "unable to open file '%s'\n",
						optarg );
				exit( EXIT_FAILURE );
			}
			break;
			
		case 'h':
			fprintf( stderr, 
	"usage: rewrite [options] string\n"
	"\n"
	"\t\t-f file\t\tconfiguration file\n"
	"\t\t-r rule[s]\tlist of comma-separated rules\n"
	"\n"
	"\tsyntax:\n"
	"\t\trewriteEngine\t{on|off}\n"
	"\t\trewriteContext\tcontextName [alias aliasedContextName]\n"
	"\t\trewriteRule\tpattern subst [flags]\n"
	"\n" 
				);
			exit( EXIT_SUCCESS );
			
		case 'r':
			rewriteContext = optarg;
			break;
		}
	}
	
	if ( debug != 0 ) {
		ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &debug);
		ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &debug);
	}
	
	if ( optind >= argc ) {
		return -1;
	}

	apply( ( fin ? fin : stdin ), rewriteContext, argv[ optind ] );

	if ( fin ) {
		fclose( fin );
	}

	return 0;
}

