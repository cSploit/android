/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Howard Chu, based in part
 * on other OpenLDAP test tools, for inclusion in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include "ac/stdlib.h"

#include "ac/ctype.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"

#include "ldap.h"
#include "lutil.h"

#include "slapd-common.h"

#define LOOPS	100
#define RETRIES	0

static void
do_modrdn( char *uri, char *manager, struct berval *passwd,
		char *entry, int maxloop, int maxretries, int delay,
		int friendly, int chaserefs );

static void
usage( char *name )
{
        fprintf( stderr,
		"usage: %s "
		"-H <uri> | ([-h <host>] -p <port>) "
		"-D <manager> "
		"-w <passwd> "
		"-e <entry> "
		"[-i <ignore>] "
		"[-l <loops>] "
		"[-L <outerloops>] "
		"[-r <maxretries>] "
		"[-t <delay>] "
		"[-F] "
		"[-C]\n",
			name );
	exit( EXIT_FAILURE );
}

int
main( int argc, char **argv )
{
	int		i;
	char		*uri = NULL;
	char		*host = "localhost";
	int		port = -1;
	char		*manager = NULL;
	struct berval	passwd = { 0, NULL };
	char		*entry = NULL;
	int		loops = LOOPS;
	int		outerloops = 1;
	int		retries = RETRIES;
	int		delay = 0;
	int		friendly = 0;
	int		chaserefs = 0;

	tester_init( "slapd-modrdn", TESTER_MODRDN );

	while ( ( i = getopt( argc, argv, "CD:e:FH:h:i:L:l:p:r:t:w:" ) ) != EOF )
	{
		switch ( i ) {
		case 'C':
			chaserefs++;
			break;

		case 'F':
			friendly++;
			break;

		case 'H':		/* the server uri */
			uri = strdup( optarg );
			break;

		case 'h':		/* the servers host */
			host = strdup( optarg );
			break;

		case 'i':
			/* ignored (!) by now */
			break;

		case 'p':		/* the servers port */
			if ( lutil_atoi( &port, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		case 'D':		/* the servers manager */
			manager = strdup( optarg );
			break;

		case 'w':		/* the server managers password */
			passwd.bv_val = strdup( optarg );
			passwd.bv_len = strlen( optarg );
			memset( optarg, '*', passwd.bv_len );
			break;

		case 'e':		/* entry to rename */
			entry = strdup( optarg );
			break;

		case 'l':		/* the number of loops */
			if ( lutil_atoi( &loops, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		case 'L':		/* the number of outerloops */
			if ( lutil_atoi( &outerloops, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		case 'r':		/* the number of retries */
			if ( lutil_atoi( &retries, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		case 't':		/* delay in seconds */
			if ( lutil_atoi( &delay, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		default:
			usage( argv[0] );
			break;
		}
	}

	if (( entry == NULL ) || ( port == -1 && uri == NULL ))
		usage( argv[0] );

	if ( *entry == '\0' ) {

		fprintf( stderr, "%s: invalid EMPTY entry DN.\n",
				argv[0] );
		exit( EXIT_FAILURE );

	}

	uri = tester_uri( uri, host, port );

	for ( i = 0; i < outerloops; i++ ) {
		do_modrdn( uri, manager, &passwd, entry,
			loops, retries, delay, friendly, chaserefs );
	}

	exit( EXIT_SUCCESS );
}


static void
do_modrdn( char *uri, char *manager,
	struct berval *passwd, char *entry, int maxloop, int maxretries,
	int delay, int friendly, int chaserefs )
{
	LDAP	*ld = NULL;
	int  	i, do_retry = maxretries;
	char	*DNs[2];
	char	*rdns[2];
	int	rc = LDAP_SUCCESS;
	char	*p1, *p2;
	int	version = LDAP_VERSION3;

	DNs[0] = entry;
	DNs[1] = strdup( entry );

	/* reverse the RDN, make new DN */
	p1 = strchr( entry, '=' ) + 1;
	p2 = strchr( p1, ',' );

	*p2 = '\0';
	rdns[1] = strdup( entry );
	*p2-- = ',';

	for (i = p1 - entry;p2 >= p1;)
		DNs[1][i++] = *p2--;
	
	DNs[1][i] = '\0';
	rdns[0] = strdup( DNs[1] );
	DNs[1][i] = ',';

	i = 0;

retry:;
	ldap_initialize( &ld, uri );
	if ( ld == NULL ) {
		tester_perror( "ldap_initialize", NULL );
		exit( EXIT_FAILURE );
	}

	(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &version ); 
	(void) ldap_set_option( ld, LDAP_OPT_REFERRALS,
		chaserefs ? LDAP_OPT_ON : LDAP_OPT_OFF );

	if ( do_retry == maxretries ) {
		fprintf( stderr, "PID=%ld - Modrdn(%d): entry=\"%s\".\n",
			(long) pid, maxloop, entry );
	}

	rc = ldap_sasl_bind_s( ld, manager, LDAP_SASL_SIMPLE, passwd, NULL, NULL, NULL );
	if ( rc != LDAP_SUCCESS ) {
		tester_ldap_error( ld, "ldap_sasl_bind_s", NULL );
		switch ( rc ) {
		case LDAP_BUSY:
		case LDAP_UNAVAILABLE:
			if ( do_retry > 0 ) {
				do_retry--;
				if ( delay > 0) {
				    sleep( delay );
				}
				goto retry;
			}
		/* fallthru */
		default:
			break;
		}
		exit( EXIT_FAILURE );
	}

	for ( ; i < maxloop; i++ ) {
		rc = ldap_rename_s( ld, DNs[0], rdns[0], NULL, 0, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_rename_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: this likely means
				 * the second modrdn failed
				 * during the previous round... */
				if ( !friendly ) {
					goto done;
				}
				break;

			case LDAP_BUSY:
			case LDAP_UNAVAILABLE:
				if ( do_retry > 0 ) {
					do_retry--;
					goto retry;
				}
				/* fall thru */

			default:
				goto done;
			}
		}
		rc = ldap_rename_s( ld, DNs[1], rdns[1], NULL, 1, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_rename_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: this likely means
				 * the first modrdn failed
				 * during the previous round... */
				if ( !friendly ) {
					goto done;
				}
				break;

			case LDAP_BUSY:
			case LDAP_UNAVAILABLE:
				if ( do_retry > 0 ) {
					do_retry--;
					goto retry;
				}
				/* fall thru */

			default:
				goto done;
			}
		}
	}

done:;
	fprintf( stderr, "  PID=%ld - Modrdn done (%d).\n", (long) pid, rc );

	ldap_unbind_ext( ld, NULL, NULL );

	free( DNs[1] );
	free( rdns[0] );
	free( rdns[1] );
}
