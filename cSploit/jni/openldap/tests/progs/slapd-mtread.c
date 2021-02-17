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

/*
 * This tool is a MT reader.  It behaves like slapd-read however
 * with one or more threads simultaneously using the same connection.
 * If -M is enabled, then M threads will also perform write operations.
 */

#include "portable.h"

#include <stdio.h>
#include "ldap_pvt_thread.h"

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

#define MAXCONN	512
#define LOOPS	100
#define RETRIES	0
#define DEFAULT_BASE	"ou=people,dc=example,dc=com"

static void
do_conn( char *uri, char *manager, struct berval *passwd,
	LDAP **ld, int nobind, int maxretries, int conn_num );

static void
do_read( LDAP *ld, char *entry,
	char **attrs, int noattrs, int nobind, int maxloop,
	int maxretries, int delay, int force, int chaserefs, int idx );

static void
do_random( LDAP *ld,
	char *sbase, char *filter, char **attrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs,
	int idx );

static void
do_random2( LDAP *ld,
	char *sbase, char *filter, char **attrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs,
	int idx );

static void *
do_onethread( void *arg );

static void *
do_onerwthread( void *arg );

#define MAX_THREAD	1024
/* Use same array for readers and writers, offset writers by MAX_THREAD */
int	rt_pass[MAX_THREAD*2];
int	rt_fail[MAX_THREAD*2];
int	*rwt_pass = rt_pass + MAX_THREAD;
int	*rwt_fail = rt_fail + MAX_THREAD;
ldap_pvt_thread_t	rtid[MAX_THREAD*2], *rwtid = rtid + MAX_THREAD;

/*
 * Shared globals (command line args)
 */
LDAP		*ld = NULL;
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
int		threads = 1;
int		rwthreads = 0;
int		verbose = 0;

int		noconns = 1;
LDAP		**lds = NULL;

static void
thread_error(int idx, char *string)
{
	char		thrstr[BUFSIZ];

	snprintf(thrstr, BUFSIZ, "error on tidx: %d: %s", idx, string);
	tester_error( thrstr );
}

static void
thread_output(int idx, char *string)
{
	char		thrstr[BUFSIZ];

	snprintf(thrstr, BUFSIZ, "tidx: %d says: %s", idx, string);
	tester_error( thrstr );
}

static void
thread_verbose(int idx, char *string)
{
	char		thrstr[BUFSIZ];

	if (!verbose)
		return;
	snprintf(thrstr, BUFSIZ, "tidx: %d says: %s", idx, string);
	tester_error( thrstr );
}

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
		"[-v] "
		"[-c connections] "
		"[-f filter] "
		"[-i <ignore>] "
		"[-l <loops>] "
		"[-L <outerloops>] "
		"[-m threads] "
		"[-M threads] "
		"[-r <maxretries>] "
		"[-t <delay>] "
		"[-T <attrs>] "
		"[<attrs>] "
		"\n",
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
	char		outstr[BUFSIZ];
	int		ptpass;
	int		testfail = 0;


	tester_init( "slapd-mtread", TESTER_READ );

	/* by default, tolerate referrals and no such object */
	tester_ignore_str2errlist( "REFERRAL,NO_SUCH_OBJECT" );

	while ( (i = getopt( argc, argv, "ACc:D:e:Ff:H:h:i:L:l:M:m:p:r:t:T:w:v" )) != EOF ) {
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

		case 'i':
			tester_ignore_str2errlist( optarg );
			break;

		case 'N':
			nobind++;
			break;

		case 'v':
			verbose++;
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

		case 'c':		/* the number of connections */
			if ( lutil_atoi( &noconns, optarg ) != 0 ) {
				usage( argv[0] );
			}
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

		case 'M':		/* the number of R/W threads */
			if ( lutil_atoi( &rwthreads, optarg ) != 0 ) {
				usage( argv[0] );
			}
			if (rwthreads > MAX_THREAD)
				rwthreads = MAX_THREAD;
			break;

		case 'm':		/* the number of threads */
			if ( lutil_atoi( &threads, optarg ) != 0 ) {
				usage( argv[0] );
			}
			if (threads > MAX_THREAD)
				threads = MAX_THREAD;
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

	if (noconns < 1)
		noconns = 1;
	if (noconns > MAXCONN)
		noconns = MAXCONN;
	lds = (LDAP **) calloc( sizeof(LDAP *), noconns);
	if (lds == NULL) {
		fprintf( stderr, "%s: Memory error: calloc noconns.\n",
				argv[0] );
		exit( EXIT_FAILURE );
	}

	uri = tester_uri( uri, host, port );
	/* One connection and one connection only */
	do_conn( uri, manager, &passwd, &ld, nobind, retries, 0 );
	lds[0] = ld;
	for(i = 1; i < noconns; i++) {
		do_conn( uri, manager, &passwd, &lds[i], nobind, retries, i );
	}

	ldap_pvt_thread_initialize();

	snprintf(outstr, BUFSIZ, "MT Test Start: conns: %d (%s)", noconns, uri);
	tester_error(outstr);
	snprintf(outstr, BUFSIZ, "Threads: RO: %d RW: %d", threads, rwthreads);
	tester_error(outstr);

	/* Set up read only threads */
	for ( i = 0; i < threads; i++ ) {
		ldap_pvt_thread_create( &rtid[i], 0, do_onethread, &rtid[i]);
		snprintf(outstr, BUFSIZ, "Created RO thread %d", i);
		thread_verbose(-1, outstr);
	}
	/* Set up read/write threads */
	for ( i = 0; i < rwthreads; i++ ) {
		ldap_pvt_thread_create( &rwtid[i], 0, do_onerwthread, &rwtid[i]);
		snprintf(outstr, BUFSIZ, "Created RW thread %d", i + MAX_THREAD);
		thread_verbose(-1, outstr);
	}

	ptpass =  outerloops * loops;

	/* wait for read only threads to complete */
	for ( i = 0; i < threads; i++ )
		ldap_pvt_thread_join(rtid[i], NULL);
	/* wait for read/write threads to complete */
	for ( i = 0; i < rwthreads; i++ )
		ldap_pvt_thread_join(rwtid[i], NULL);

	for(i = 0; i < noconns; i++) {
		if ( lds[i] != NULL ) {
			ldap_unbind_ext( lds[i], NULL, NULL );
		}
	}
	free( lds );

	for ( i = 0; i < threads; i++ ) {
		snprintf(outstr, BUFSIZ, "RO thread %d pass=%d fail=%d", i,
			rt_pass[i], rt_fail[i]);
		tester_error(outstr);
		if (rt_fail[i] != 0 || rt_pass[i] != ptpass) {
			snprintf(outstr, BUFSIZ, "FAIL RO thread %d", i);
			tester_error(outstr);
			testfail++;
		}
	}
	for ( i = 0; i < rwthreads; i++ ) {
		snprintf(outstr, BUFSIZ, "RW thread %d pass=%d fail=%d", i + MAX_THREAD,
			rwt_pass[i], rwt_fail[i]);
		tester_error(outstr);
		if (rwt_fail[i] != 0 || rwt_pass[i] != ptpass) {
			snprintf(outstr, BUFSIZ, "FAIL RW thread %d", i);
			tester_error(outstr);
			testfail++;
		}
	}
	snprintf(outstr, BUFSIZ, "MT Test complete" );
	tester_error(outstr);

	if (testfail)
		exit( EXIT_FAILURE );
	exit( EXIT_SUCCESS );
}

static void *
do_onethread( void *arg )
{
	int		i, j, thisconn;
	LDAP		**mlds;
	char		thrstr[BUFSIZ];
	int		rc, refcnt = 0;
	int		idx = (ldap_pvt_thread_t *)arg - rtid;

	mlds = (LDAP **) calloc( sizeof(LDAP *), noconns);
	if (mlds == NULL) {
		thread_error( idx, "Memory error: thread calloc for noconns" );
		exit( EXIT_FAILURE );
	}

	for ( j = 0; j < outerloops; j++ ) {
		for(i = 0; i < noconns; i++) {
			mlds[i] = ldap_dup(lds[i]);
			if (mlds[i] == NULL) {
				thread_error( idx, "ldap_dup error" );
			}
		}
		rc = ldap_get_option(mlds[0], LDAP_OPT_SESSION_REFCNT, &refcnt);
		snprintf(thrstr, BUFSIZ,
			"RO Thread conns: %d refcnt: %d (rc = %d)",
			noconns, refcnt, rc);
		thread_verbose(idx, thrstr);

		thisconn = (idx + j) % noconns;
		if (thisconn < 0 || thisconn >= noconns)
			thisconn = 0;
		if (mlds[thisconn] == NULL) {
			thread_error( idx, "(failed to dup)");
			tester_perror( "ldap_dup", "(failed to dup)" );
			exit( EXIT_FAILURE );
		}
		snprintf(thrstr, BUFSIZ, "Using conn %d", thisconn);
		thread_verbose(idx, thrstr);
		if ( filter != NULL ) {
			if (strchr(filter, '['))
				do_random2( mlds[thisconn], entry, filter, attrs,
					noattrs, nobind, loops, retries, delay, force,
					chaserefs, idx );
			else
				do_random( mlds[thisconn], entry, filter, attrs,
					noattrs, nobind, loops, retries, delay, force,
					chaserefs, idx );

		} else {
			do_read( mlds[thisconn], entry, attrs,
				noattrs, nobind, loops, retries, delay, force,
				chaserefs, idx );
		}
		for(i = 0; i < noconns; i++) {
			(void) ldap_destroy(mlds[i]);
			mlds[i] = NULL;
		}
	}
	free( mlds );
	return( NULL );
}

static void *
do_onerwthread( void *arg )
{
	int		i, j, thisconn;
	LDAP		**mlds, *ld;
	char		thrstr[BUFSIZ];
	char		dn[256], uids[32], cns[32], *base;
	LDAPMod		*attrp[5], attrs[4];
	char		*oc_vals[] = { "top", "OpenLDAPperson", NULL };
	char		*cn_vals[] = { NULL, NULL };
	char		*sn_vals[] = { NULL, NULL };
	char		*uid_vals[] = { NULL, NULL };
	int		ret;
	int		adds = 0;
	int		dels = 0;
	int		rc, refcnt = 0;
	int		idx = (ldap_pvt_thread_t *)arg - rtid;

	mlds = (LDAP **) calloc( sizeof(LDAP *), noconns);
	if (mlds == NULL) {
		thread_error( idx, "Memory error: thread calloc for noconns" );
		exit( EXIT_FAILURE );
	}

	snprintf(uids, sizeof(uids), "rwtest%04d", idx);
	snprintf(cns, sizeof(cns), "rwtest%04d", idx);
	/* add setup */
	for (i = 0; i < 4; i++) {
		attrp[i] = &attrs[i];
		attrs[i].mod_op = 0;
	}
	attrp[4] = NULL;
	attrs[0].mod_type = "objectClass";
	attrs[0].mod_values = oc_vals;
	attrs[1].mod_type = "cn";
	attrs[1].mod_values = cn_vals;
	cn_vals[0] = &cns[0];
	attrs[2].mod_type = "sn";
	attrs[2].mod_values = sn_vals;
	sn_vals[0] = &cns[0];
	attrs[3].mod_type = "uid";
	attrs[3].mod_values = uid_vals;
	uid_vals[0] = &uids[0];

	for ( j = 0; j < outerloops; j++ ) {
		for(i = 0; i < noconns; i++) {
			mlds[i] = ldap_dup(lds[i]);
			if (mlds[i] == NULL) {
				thread_error( idx, "ldap_dup error" );
			}
		}
		rc = ldap_get_option(mlds[0], LDAP_OPT_SESSION_REFCNT, &refcnt);
		snprintf(thrstr, BUFSIZ,
			"RW Thread conns: %d refcnt: %d (rc = %d)",
			noconns, refcnt, rc);
		thread_verbose(idx, thrstr);

		thisconn = (idx + j) % noconns;
		if (thisconn < 0 || thisconn >= noconns)
			thisconn = 0;
		if (mlds[thisconn] == NULL) {
			thread_error( idx, "(failed to dup)");
			tester_perror( "ldap_dup", "(failed to dup)" );
			exit( EXIT_FAILURE );
		}
		snprintf(thrstr, BUFSIZ, "START RW Thread using conn %d", thisconn);
		thread_verbose(idx, thrstr);

		ld = mlds[thisconn];
		if (entry != NULL)
			base = entry;
		else
			base = DEFAULT_BASE;
		snprintf(dn, 256, "cn=%s,%s", cns, base);

		adds = 0;
		dels = 0;
		for (i = 0; i < loops; i++) {
			ret = ldap_add_ext_s(ld, dn, &attrp[0], NULL, NULL);
			if (ret == LDAP_SUCCESS) {
				adds++;
				ret = ldap_delete_ext_s(ld, dn, NULL, NULL);
				if (ret == LDAP_SUCCESS) {
					dels++;
					rt_pass[idx]++;
				} else {
					thread_output(idx, ldap_err2string(ret));
					rt_fail[idx]++;
				}
			} else {
				thread_output(idx, ldap_err2string(ret));
				rt_fail[idx]++;
			}
		}

		snprintf(thrstr, BUFSIZ,
			"INNER STOP RW Thread using conn %d (%d/%d)",
			thisconn, adds, dels);
		thread_verbose(idx, thrstr);

		for(i = 0; i < noconns; i++) {
			(void) ldap_destroy(mlds[i]);
			mlds[i] = NULL;
		}
	}

	free( mlds );
	return( NULL );
}

static void
do_conn( char *uri, char *manager, struct berval *passwd,
	LDAP **ldp, int nobind, int maxretries, int conn_num )
{
	LDAP	*ld = NULL;
	int	version = LDAP_VERSION3;
	int  	i = 0, do_retry = maxretries;
	int     rc = LDAP_SUCCESS;
	char	thrstr[BUFSIZ];

retry:;
	ldap_initialize( &ld, uri );
	if ( ld == NULL ) {
		snprintf( thrstr, BUFSIZ, "connection: %d", conn_num );
		tester_error( thrstr );
		tester_perror( "ldap_initialize", NULL );
		exit( EXIT_FAILURE );
	}

	(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &version ); 
	(void) ldap_set_option( ld, LDAP_OPT_REFERRALS,
		chaserefs ? LDAP_OPT_ON : LDAP_OPT_OFF );

	if ( do_retry == maxretries ) {
		snprintf( thrstr, BUFSIZ, "do_conn #%d\n", conn_num );
		thread_verbose( -1, thrstr );
	}

	if ( nobind == 0 ) {
		rc = ldap_sasl_bind_s( ld, manager, LDAP_SASL_SIMPLE, passwd, NULL, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( thrstr, BUFSIZ, "connection: %d", conn_num );
			tester_error( thrstr );
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
	*ldp = ld;
}

static void
do_random( LDAP *ld,
	char *sbase, char *filter, char **srchattrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs,
	int idx )
{
	int  	i = 0, do_retry = maxretries;
	char	*attrs[ 2 ];
	int     rc = LDAP_SUCCESS;
	int	nvalues = 0;
	char	**values = NULL;
	LDAPMessage *res = NULL, *e = NULL;
	char	thrstr[BUFSIZ];

	attrs[ 0 ] = LDAP_NO_ATTRS;
	attrs[ 1 ] = NULL;

	snprintf( thrstr, BUFSIZ,
			"Read(%d): base=\"%s\", filter=\"%s\".\n",
			innerloop, sbase, filter );
	thread_verbose( idx, thrstr );

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
			snprintf( thrstr, BUFSIZ,
				"Read base=\"%s\" filter=\"%s\" got %d values.\n",
				sbase, filter, nvalues );
			thread_verbose( idx, thrstr );
		}

		for ( i = 0; i < innerloop; i++ ) {
			int	r = ((double)nvalues)*rand()/(RAND_MAX + 1.0);

			do_read( ld, values[ r ],
				srchattrs, noattrs, nobind, 1, maxretries,
				delay, force, chaserefs, idx );
		}
		for( i = 0; i < nvalues; i++) {
			if (values[i] != NULL)
				ldap_memfree( values[i] );
		}
		free( values );
		break;

	default:
		tester_ldap_error( ld, "ldap_search_ext_s", NULL );
		break;
	}

	snprintf( thrstr, BUFSIZ, "Search done (%d).\n", rc );
	thread_verbose( idx, thrstr );
}

/* substitute a generated int into the filter */
static void
do_random2( LDAP *ld,
	char *sbase, char *filter, char **srchattrs, int noattrs, int nobind,
	int innerloop, int maxretries, int delay, int force, int chaserefs,
	int idx )
{
	int  	i = 0, do_retry = maxretries;
	int     rc = LDAP_SUCCESS;
	int		lo, hi, range;
	int	flen;
	LDAPMessage *res = NULL, *e = NULL;
	char	*ptr, *ftail;
	char	thrstr[BUFSIZ];
	char	fbuf[BUFSIZ];

	snprintf( thrstr, BUFSIZ,
			"Read(%d): base=\"%s\", filter=\"%s\".\n",
			innerloop, sbase, filter );
	thread_verbose( idx, thrstr );

	ptr = strchr(filter, '[');
	if (!ptr)
		return;
	ftail = strchr(filter, ']');
	if (!ftail || ftail < ptr)
		return;

	sscanf(ptr, "[%d-%d]", &lo, &hi);
	range = hi - lo + 1;

	flen = ptr - filter;
	ftail++;

	for ( i = 0; i < innerloop; i++ ) {
		int	r = ((double)range)*rand()/(RAND_MAX + 1.0);
		sprintf(fbuf, "%.*s%d%s", flen, filter, r, ftail);

		rc = ldap_search_ext_s( ld, sbase, LDAP_SCOPE_SUBTREE,
				fbuf, srchattrs, noattrs, NULL, NULL, NULL,
				LDAP_NO_LIMIT, &res );
		if ( res != NULL ) {
			ldap_msgfree( res );
		}
		if ( rc == 0 ) {
			rt_pass[idx]++;
		} else {
			int		first = tester_ignore_err( rc );
			char		buf[ BUFSIZ ];

			rt_fail[idx]++;
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
				do_retry--;
				continue;
			}
			break;
		}
	}

	snprintf( thrstr, BUFSIZ, "Search done (%d).\n", rc );
	thread_verbose( idx, thrstr );
}

static void
do_read( LDAP *ld, char *entry,
	char **attrs, int noattrs, int nobind, int maxloop,
	int maxretries, int delay, int force, int chaserefs, int idx )
{
	int  	i = 0, do_retry = maxretries;
	int     rc = LDAP_SUCCESS;
	char	thrstr[BUFSIZ];

retry:;
	if ( do_retry == maxretries ) {
		snprintf( thrstr, BUFSIZ, "Read(%d): entry=\"%s\".\n",
			maxloop, entry );
		thread_verbose( idx, thrstr );
	}

	snprintf(thrstr, BUFSIZ, "LD %p cnt: %d (retried %d) (%s)", \
		 (void *) ld, maxloop, (do_retry - maxretries), entry);
	thread_verbose( idx, thrstr );

	for ( ; i < maxloop; i++ ) {
		LDAPMessage *res = NULL;

		rc = ldap_search_ext_s( ld, entry, LDAP_SCOPE_BASE,
				NULL, attrs, noattrs, NULL, NULL, NULL,
				LDAP_NO_LIMIT, &res );
		if ( res != NULL ) {
			ldap_msgfree( res );
		}

		if ( rc == 0 ) {
			rt_pass[idx]++;
		} else {
			int		first = tester_ignore_err( rc );
			char		buf[ BUFSIZ ];

			rt_fail[idx]++;
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
				do_retry--;
				goto retry;
			}
			break;
		}
	}
}
