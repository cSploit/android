/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2006-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/param.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/wait.h>
#include <ac/time.h>
#include <ac/signal.h>

#include <ldap.h>
#include <ldap_pvt_thread.h>
#include <lutil.h>

static int
do_time( );

/* This program is a simplified version of SLAMD's WeightedAuthRate jobclass.
 * It doesn't offer as much configurability, but it's a good starting point.
 * When run without the -R option it will behave as a Standard AuthRate job.
 * Eventually this will grow into a set of C-based load generators for the SLAMD
 * framework. This code is anywhere from 2 to 10 times more efficient than the
 * original Java code, allowing servers to be fully loaded without requiring
 * anywhere near as much load-generation hardware.
 */
static void
usage( char *name )
{
	fprintf( stderr, "usage: %s -H <uri> -b <baseDN> -w <passwd> -t <seconds> -r lo:hi\n\t"
		"[-R %:lo:hi] [-f <filter-template>] [-n <threads>] [-D <bindDN>] [-i <seconds>]\n",
			name );
	exit( EXIT_FAILURE );
}

static char *filter = "(uid=user.%d)";

static char hname[1024];
static char *uri = "ldap:///";
static char	*base;
static char	*pass;
static char *binder;

static int tdur, r1per, r1lo, r1hi, r2per, r2lo, r2hi;
static int threads = 1;

static int interval = 30;

static volatile int *r1binds, *r2binds;
static int *r1old, *r2old;
static volatile int finish;

int
main( int argc, char **argv )
{
	int		i;

	while ( (i = getopt( argc, argv, "b:D:H:w:f:n:i:t:r:R:" )) != EOF ) {
		switch( i ) {
			case 'b':		/* base DN of a tree of user DNs */
				base = strdup( optarg );
				break;

			case 'D':
				binder = strdup( optarg );
				break;

			case 'H':		/* the server uri */
				uri = strdup( optarg );
				break;

			case 'w':
				pass = strdup( optarg );
				break;

			case 't':		/* the duration to run */
				if ( lutil_atoi( &tdur, optarg ) != 0 ) {
					usage( argv[0] );
				}
				break;

			case 'i':		/* the time interval */
				if ( lutil_atoi( &interval, optarg ) != 0 ) {
					usage( argv[0] );
				}
				break;

			case 'r':		/* the uid range */
				if ( sscanf(optarg, "%d:%d", &r1lo, &r1hi) != 2 ) {
					usage( argv[0] );
				}
				break;

			case 'R':		/* percentage:2nd uid range */
				if ( sscanf(optarg, "%d:%d:%d", &r2per, &r2lo, &r2hi) != 3 ) {
					usage( argv[0] );
				}
				break;

			case 'f':
				filter = optarg;
				break;

			case 'n':
				if ( lutil_atoi( &threads, optarg ) != 0 || threads < 1 ) {
					usage( argv[0] );
				}
				break;
				
			default:
				usage( argv[0] );
				break;
		}
	}

	if ( tdur == 0 || r1hi <= r1lo )
		usage( argv[0] );

	r1per = 100 - r2per;
	if ( r1per < 1 )
		usage( argv[0] );

	r1binds = calloc( threads*4, sizeof( int ));
	r2binds = r1binds + threads;
	r1old = (int *)r2binds + threads;
	r2old = r1old + threads;

	do_time( );

	exit( EXIT_SUCCESS );
}

static void *
my_task( void *my_num )
{
	LDAP	*ld = NULL, *sld = NULL;
	ber_int_t msgid;
	LDAPMessage *res, *msg;
	char *attrs[] = { "1.1", NULL };
	int     rc = LDAP_SUCCESS;
	int		tid = *(int *)my_num;

	ldap_initialize( &ld, uri );
	if ( ld == NULL ) {
		perror( "ldap_initialize" );
		return NULL;
	}

	{
		int version = LDAP_VERSION3;
		(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION,
			&version ); 
	}
	(void) ldap_set_option( ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF );

	ldap_initialize( &sld, uri );
	if ( sld == NULL ) {
		perror( "ldap_initialize" );
		return NULL;
	}

	{
		int version = LDAP_VERSION3;
		(void) ldap_set_option( sld, LDAP_OPT_PROTOCOL_VERSION,
			&version ); 
	}
	(void) ldap_set_option( sld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF );
	if ( binder ) {
		rc = ldap_bind_s( sld, binder, pass, LDAP_AUTH_SIMPLE );
		if ( rc != LDAP_SUCCESS ) {
			ldap_perror( sld, "ldap_bind" );
		}
	}

	r1binds[tid] = 0;

	for (;;) {
		char dn[BUFSIZ], *ptr, fstr[256];
		int j, isr1;
		
		if ( finish )
			break;

		j = rand() % 100;
		if ( j < r1per ) {
			j = rand() % r1hi;
			isr1 = 1;
		} else {
			j = rand() % (r2hi - r2lo + 1 );
			j += r2lo;
			isr1 = 0;
		}
		sprintf(fstr, filter, j);

		rc = ldap_search_ext( sld, base, LDAP_SCOPE_SUB,
			fstr, attrs, 0, NULL, NULL, 0, 0, &msgid );
		if ( rc != LDAP_SUCCESS ) {
			ldap_perror( sld, "ldap_search_ex" );
			return NULL;
		}

		while (( rc=ldap_result( sld, LDAP_RES_ANY, LDAP_MSG_ONE, NULL, &res )) >0){
			BerElement *ber;
			struct berval bv;
			char *ptr;
			int done = 0;

			for (msg = ldap_first_message( sld, res ); msg;
				msg = ldap_next_message( sld, msg )) {
				switch ( ldap_msgtype( msg )) {
				case LDAP_RES_SEARCH_ENTRY:
					rc = ldap_get_dn_ber( sld, msg, &ber, &bv );
					strcpy(dn, bv.bv_val );
					ber_free( ber, 0 );
					break;
				case LDAP_RES_SEARCH_RESULT:
					done = 1;
					break;
				}
				if ( done )
					break;
			}
			ldap_msgfree( res );
			if ( done ) break;
		}

		rc = ldap_bind_s( ld, dn, pass, LDAP_AUTH_SIMPLE );
		if ( rc != LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_bind" );
		}
		if ( isr1 )
			r1binds[tid]++;
		else
			r2binds[tid]++;
	}

	ldap_unbind( sld );
	ldap_unbind( ld );

	return NULL;
}

static int
do_time( )
{
	struct timeval tv;
	time_t now, prevt, start;

	int r1new, r2new;
	int dt, dr1, dr2, rr1, rr2;
	int dr10, dr20;
	int i;

	gethostname(hname, sizeof(hname));
	printf("%s(tid)\tdeltaT\tauth1\tauth2\trate1\trate2\tRate1+2\n", hname);
	srand(getpid());

	prevt = start = time(0L);

	for ( i = 0; i<threads; i++ ) {
		ldap_pvt_thread_t thr;
		r1binds[i] = i;
		ldap_pvt_thread_create( &thr, 1, my_task, (void *)&r1binds[i] );
	}

	for (;;) {
		tv.tv_sec = interval;
		tv.tv_usec = 0;

		select(0, NULL, NULL, NULL, &tv);

		now = time(0L);

		dt = now - prevt;
		prevt = now;

		dr10 = 0;
		dr20 = 0;

		for ( i = 0; i < threads; i++ ) {
			r1new = r1binds[i];
			r2new = r2binds[i];

			dr1 = r1new - r1old[i];
			dr2 = r2new - r2old[i];
			rr1 = dr1 / dt;
			rr2 = dr2 / dt;

			printf("%s(%d)\t%d\t%d\t%d\t%d\t%d\t%d\n",
				hname, i, dt, dr1, dr2, rr1, rr2, rr1 + rr2);

			dr10 += dr1;
			dr20 += dr2;

			r1old[i] = r1new;
			r2old[i] = r2new;
		}
		if ( i > 1 ) {
			rr1 = dr10 / dt;
			rr2 = dr20 / dt;
			
			printf("%s(sum)\t%d\t%d\t%d\t%d\t%d\t%d\n",
				hname, 0, dr10, dr20, rr1, rr2, rr1 + rr2);
		}

		if ( now - start >= tdur ) {
			finish = 1;
			break;
		}
	}
	return 0;
}
