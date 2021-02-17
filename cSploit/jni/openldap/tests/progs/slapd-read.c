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
 * This work was initially developed by Kurt Spanier for inclusion
 * in OpenLDAP Software.
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

#include "ldap_pvt.h"

#include "slapd-common.h"

#define LOOPS	100
#define RETRIES	0

static void
do_read( char *uri, char *manager, struct berval *passwd,
	char *entry, LDAP **ld,
	char **attrs, int noattrs, int nobind, int maxloop,
	int maxretries, int delay, int force, int chaserefs );

static void
do_random( char *uri, char *manager, struct berval *passwd,
	char *sbase, char *filter, char **attrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs );

static void
usage( char *name )
{
        fprintf( stderr,
		"usage: %s "
		"-H <uri> | ([-h <host>] -p <port>) "
		"-D <manager> "
		"-w <passwd> "
		"-e <entry> "
		"[-A] "
		"[-C] "
		"[-F] "
		"[-N] "
		"[-f filter] "
		"[-i <ignore>] "
		"[-l <loops>] "
		"[-L <outerloops>] "
		"[-r <maxretries>] "
		"[-t <delay>] "
		"[-T <attrs>] "
		"[<attrs>] "
		"\n",
		name );
	exit( EXIT_FAILURE );
}

/* -S: just send requests without reading responses
 * -SS: send all requests asynchronous and immediately start reading responses
 * -SSS: send all requests asynchronous; then read responses
 */
static int swamp;

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
	char		*filter  = NULL;
	int		loops = LOOPS;
	int		outerloops = 1;
	int		retries = RETRIES;
	int		delay = 0;
	int		force = 0;
	int		chaserefs = 0;
	char		*srchattrs[] = { "1.1", NULL };
	char		**attrs = srchattrs;
	int		noattrs = 0;
	int		nobind = 0;

	tester_init( "slapd-read", TESTER_READ );

	/* by default, tolerate referrals and no such object */
	tester_ignore_str2errlist( "REFERRAL,NO_SUCH_OBJECT" );

	while ( (i = getopt( argc, argv, "ACD:e:Ff:H:h:i:L:l:p:r:St:T:w:" )) != EOF ) {
		switch ( i ) {
		case 'A':
			noattrs++;
			break;

		case 'C':
			chaserefs++;
			break;

		case 'H':		/* the server uri */
			uri = strdup( optarg );
			break;

		case 'h':		/* the servers host */
			host = strdup( optarg );
			break;

		case 'i':
			tester_ignore_str2errlist( optarg );
			break;

		case 'N':
			nobind++;
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

		case 'e':		/* DN to search for */
			entry = strdup( optarg );
			break;

		case 'f':		/* the search request */
			filter = strdup( optarg );
			break;

		case 'F':
			force++;
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

		case 'S':
			swamp++;
			break;

		case 't':		/* delay in seconds */
			if ( lutil_atoi( &delay, optarg ) != 0 ) {
				usage( argv[0] );
			}
			break;

		case 'T':
			attrs = ldap_str2charray( optarg, "," );
			if ( attrs == NULL ) {
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

	if ( argv[optind] != NULL ) {
		attrs = &argv[optind];
	}

	uri = tester_uri( uri, host, port );

	for ( i = 0; i < outerloops; i++ ) {
		if ( filter != NULL ) {
			do_random( uri, manager, &passwd, entry, filter, attrs,
				noattrs, nobind, loops, retries, delay, force,
				chaserefs );

		} else {
			do_read( uri, manager, &passwd, entry, NULL, attrs,
				noattrs, nobind, loops, retries, delay, force,
				chaserefs );
		}
	}

	exit( EXIT_SUCCESS );
}

static void
do_random( char *uri, char *manager, struct berval *passwd,
	char *sbase, char *filter, char **srchattrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs )
{
	LDAP	*ld = NULL;
	int  	i = 0, do_retry = maxretries;
	char	*attrs[ 2 ];
	int     rc = LDAP_SUCCESS;
	int	version = LDAP_VERSION3;
	int	nvalues = 0;
	char	**values = NULL;
	LDAPMessage *res = NULL, *e = NULL;

	attrs[ 0 ] = LDAP_NO_ATTRS;
	attrs[ 1 ] = NULL;

	ldap_initialize( &ld, uri );
	if ( ld == NULL ) {
		tester_perror( "ldap_initialize", NULL );
		exit( EXIT_FAILURE );
	}

	(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &version ); 
	(void) ldap_set_option( ld, LDAP_OPT_REFERRALS,
		chaserefs ? LDAP_OPT_ON : LDAP_OPT_OFF );

	if ( do_retry == maxretries ) {
		fprintf( stderr, "PID=%ld - Read(%d): base=\"%s\", filter=\"%s\".\n",
				(long) pid, innerloop, sbase, filter );
	}

	if ( nobind == 0 ) {
		rc = ldap_sasl_bind_s( ld, manager, LDAP_SASL_SIMPLE, passwd, NULL, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_sasl_bind_s", NULL );
			switch ( rc ) {
			case LDAP_BUSY:
			case LDAP_UNAVAILABLE:
			/* fallthru */
			default:
				break;
			}
			exit( EXIT_FAILURE );
		}
	}

	rc = ldap_search_ext_s( ld, sbase, LDAP_SCOPE_SUBTREE,
		filter, attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &res );
	switch ( rc ) {
	case LDAP_SIZELIMIT_EXCEEDED:
	case LDAP_TIMELIMIT_EXCEEDED:
	case LDAP_SUCCESS:
		nvalues = ldap_count_entries( ld, res );
		if ( nvalues == 0 ) {
			if ( rc ) {
				tester_ldap_error( ld, "ldap_search_ext_s", NULL );
			}
			break;
		}

		values = malloc( ( nvalues + 1 ) * sizeof( char * ) );
		for ( i = 0, e = ldap_first_entry( ld, res ); e != NULL; i++, e = ldap_next_entry( ld, e ) )
		{
			values[ i ] = ldap_get_dn( ld, e );
		}
		values[ i ] = NULL;

		ldap_msgfree( res );

		if ( do_retry == maxretries ) {
			fprintf( stderr, "  PID=%ld - Read base=\"%s\" filter=\"%s\" got %d values.\n",
				(long) pid, sbase, filter, nvalues );
		}

		for ( i = 0; i < innerloop; i++ ) {
#if 0	/* use high-order bits for better randomness (Numerical Recipes in "C") */
			int	r = rand() % nvalues;
#endif
			int	r = ((double)nvalues)*rand()/(RAND_MAX + 1.0);

			do_read( uri, manager, passwd, values[ r ], &ld,
				srchattrs, noattrs, nobind, 1, maxretries,
				delay, force, chaserefs );
		}
		free( values );
		break;

	default:
		tester_ldap_error( ld, "ldap_search_ext_s", NULL );
		break;
	}

	fprintf( stderr, "  PID=%ld - Read done (%d).\n", (long) pid, rc );

	if ( ld != NULL ) {
		ldap_unbind_ext( ld, NULL, NULL );
	}
}

static void
do_read( char *uri, char *manager, struct berval *passwd, char *entry,
	LDAP **ldp, char **attrs, int noattrs, int nobind, int maxloop,
	int maxretries, int delay, int force, int chaserefs )
{
	LDAP	*ld = ldp ? *ldp : NULL;
	int  	i = 0, do_retry = maxretries;
	int     rc = LDAP_SUCCESS;
	int		version = LDAP_VERSION3;
	int		*msgids = NULL, active = 0;

	/* make room for msgid */
	if ( swamp > 1 ) {
		msgids = (int *)calloc( sizeof(int), maxloop );
	}

retry:;
	if ( ld == NULL ) {
		ldap_initialize( &ld, uri );
		if ( ld == NULL ) {
			tester_perror( "ldap_initialize", NULL );
			exit( EXIT_FAILURE );
		}

		(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &version ); 
		(void) ldap_set_option( ld, LDAP_OPT_REFERRALS,
			chaserefs ? LDAP_OPT_ON : LDAP_OPT_OFF );

		if ( do_retry == maxretries ) {
			fprintf( stderr, "PID=%ld - Read(%d): entry=\"%s\".\n",
				(long) pid, maxloop, entry );
		}

		if ( nobind == 0 ) {
			rc = ldap_sasl_bind_s( ld, manager, LDAP_SASL_SIMPLE, passwd, NULL, NULL, NULL );
			if ( rc != LDAP_SUCCESS ) {
				tester_ldap_error( ld, "ldap_sasl_bind_s", NULL );
				switch ( rc ) {
				case LDAP_BUSY:
				case LDAP_UNAVAILABLE:
					if ( do_retry > 0 ) {
						ldap_unbind_ext( ld, NULL, NULL );
						ld = NULL;
						do_retry--;
						if ( delay != 0 ) {
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
		}
	}

	if ( swamp > 1 ) {
		do {
			LDAPMessage *res = NULL;
			int j, msgid;

			if ( i < maxloop ) {
				rc = ldap_search_ext( ld, entry, LDAP_SCOPE_BASE,
						NULL, attrs, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgids[i] );

				active++;
#if 0
				fprintf( stderr,
					">>> PID=%ld - Read maxloop=%d cnt=%d active=%d msgid=%d: "
					"entry=\"%s\"\n",
					(long) pid, maxloop, i, active, msgids[i],
					entry );
#endif
				i++;

				if ( rc ) {
					char buf[BUFSIZ];
					int first = tester_ignore_err( rc );
					/* if ignore.. */
					if ( first ) {
						/* only log if first occurrence */
						if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
							tester_ldap_error( ld, "ldap_search_ext", NULL );
						}
						continue;
					}
		
					/* busy needs special handling */
					snprintf( buf, sizeof( buf ), "entry=\"%s\"\n", entry );
					tester_ldap_error( ld, "ldap_search_ext", buf );
					if ( rc == LDAP_BUSY && do_retry > 0 ) {
						ldap_unbind_ext( ld, NULL, NULL );
						ld = NULL;
						do_retry--;
						goto retry;
					}
					break;
				}

				if ( swamp > 2 ) {
					continue;
				}
			}

			rc = ldap_result( ld, LDAP_RES_ANY, 0, NULL, &res );
			switch ( rc ) {
			case -1:
				/* gone really bad */
#if 0
				fprintf( stderr,
					">>> PID=%ld - Read maxloop=%d cnt=%d active=%d: "
					"entry=\"%s\" ldap_result()=%d\n",
					(long) pid, maxloop, i, active, entry, rc );
#endif
				goto cleanup;
	
			case 0:
				/* timeout (impossible) */
				break;
	
			case LDAP_RES_SEARCH_ENTRY:
			case LDAP_RES_SEARCH_REFERENCE:
				/* ignore */
				break;
	
			case LDAP_RES_SEARCH_RESULT:
				/* just remove, no error checking (TODO?) */
				msgid = ldap_msgid( res );
				ldap_parse_result( ld, res, &rc, NULL, NULL, NULL, NULL, 1 );
				res = NULL;

				/* linear search, bah */
				for ( j = 0; j < i; j++ ) {
					if ( msgids[ j ] == msgid ) {
						msgids[ j ] = -1;
						active--;
#if 0
						fprintf( stderr,
							"<<< PID=%ld - ReadDone maxloop=%d cnt=%d active=%d msgid=%d: "
							"entry=\"%s\"\n",
							(long) pid, maxloop, j, active, msgid, entry );
#endif
						break;
					}
				}
				break;

			default:
				/* other messages unexpected */
				fprintf( stderr,
					"### PID=%ld - Read(%d): "
					"entry=\"%s\" attrs=%s%s. unexpected response tag=%d\n",
					(long) pid, maxloop,
					entry, attrs[0], attrs[1] ? " (more...)" : "", rc );
				break;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		} while ( i < maxloop || active > 0 );

	} else {
		for ( ; i < maxloop; i++ ) {
			LDAPMessage *res = NULL;

			if (swamp) {
				int msgid;
				rc = ldap_search_ext( ld, entry, LDAP_SCOPE_BASE,
						NULL, attrs, noattrs, NULL, NULL,
						NULL, LDAP_NO_LIMIT, &msgid );
				if ( rc == LDAP_SUCCESS ) continue;
				else break;
			}
	
			rc = ldap_search_ext_s( ld, entry, LDAP_SCOPE_BASE,
					NULL, attrs, noattrs, NULL, NULL, NULL,
					LDAP_NO_LIMIT, &res );
			if ( res != NULL ) {
				ldap_msgfree( res );
			}

			if ( rc ) {
				int		first = tester_ignore_err( rc );
				char		buf[ BUFSIZ ];
	
				snprintf( buf, sizeof( buf ), "ldap_search_ext_s(%s)", entry );
	
				/* if ignore.. */
				if ( first ) {
					/* only log if first occurrence */
					if ( ( force < 2 && first > 0 ) || abs(first) == 1 ) {
						tester_ldap_error( ld, buf, NULL );
					}
					continue;
				}
	
				/* busy needs special handling */
				tester_ldap_error( ld, buf, NULL );
				if ( rc == LDAP_BUSY && do_retry > 0 ) {
					ldap_unbind_ext( ld, NULL, NULL );
					ld = NULL;
					do_retry--;
					goto retry;
				}
				break;
			}
		}
	}

cleanup:;
	if ( msgids != NULL ) {
		free( msgids );
	}

	if ( ldp != NULL ) {
		*ldp = ld;

	} else {
		fprintf( stderr, "  PID=%ld - Read done (%d).\n", (long) pid, rc );

		if ( ld != NULL ) {
			ldap_unbind_ext( ld, NULL, NULL );
		}
	}
}

