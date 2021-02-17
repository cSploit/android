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

#include "slapd-common.h"

#define LOOPS	100
#define RETRIES	0

static char *
get_add_entry( char *filename, LDAPMod ***mods );

static void
do_addel( char *uri, char *manager, struct berval *passwd,
	char *dn, LDAPMod **attrs, int maxloop, int maxretries, int delay,
	int friendly, int chaserefs );

static void
usage( char *name )
{
        fprintf( stderr,
		"usage: %s "
		"-H <uri> | ([-h <host>] -p <port>) "
		"-D <manager> "
		"-w <passwd> "
		"-f <addfile> "
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
	char		*host = "localhost";
	char		*uri = NULL;
	int		port = -1;
	char		*manager = NULL;
	struct berval	passwd = { 0, NULL };
	char		*filename = NULL;
	char		*entry = NULL;
	int		loops = LOOPS;
	int		outerloops = 1;
	int		retries = RETRIES;
	int		delay = 0;
	int		friendly = 0;
	int		chaserefs = 0;
	LDAPMod		**attrs = NULL;

	tester_init( "slapd-addel", TESTER_ADDEL );

	while ( ( i = getopt( argc, argv, "CD:Ff:H:h:i:L:l:p:r:t:w:" ) ) != EOF )
	{
		switch ( i ) {
		case 'C':
			chaserefs++;
			break;

		case 'F':
			friendly++;
			break;
			
		case 'H':		/* the server's URI */
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

		case 'f':		/* file with entry search request */
			filename = strdup( optarg );
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

		case 'r':		/* number of retries */
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

	if (( filename == NULL ) || ( port == -1 && uri == NULL ) ||
				( manager == NULL ) || ( passwd.bv_val == NULL ))
		usage( argv[0] );

	entry = get_add_entry( filename, &attrs );
	if (( entry == NULL ) || ( *entry == '\0' )) {

		fprintf( stderr, "%s: invalid entry DN in file \"%s\".\n",
				argv[0], filename );
		exit( EXIT_FAILURE );

	}

	if (( attrs == NULL ) || ( *attrs == '\0' )) {

		fprintf( stderr, "%s: invalid attrs in file \"%s\".\n",
				argv[0], filename );
		exit( EXIT_FAILURE );

	}

	uri = tester_uri( uri, host, port );

	for ( i = 0; i < outerloops; i++ ) {
		do_addel( uri, manager, &passwd, entry, attrs,
				loops, retries, delay, friendly, chaserefs );
	}

	exit( EXIT_SUCCESS );
}


static void
addmodifyop( LDAPMod ***pmodsp, int modop, char *attr, char *value, int vlen )
{
    LDAPMod		**pmods;
    int			i, j;
    struct berval	*bvp;

    pmods = *pmodsp;
    modop |= LDAP_MOD_BVALUES;

    i = 0;
    if ( pmods != NULL ) {
		for ( ; pmods[ i ] != NULL; ++i ) {
	    	if ( strcasecmp( pmods[ i ]->mod_type, attr ) == 0 &&
		    	pmods[ i ]->mod_op == modop ) {
				break;
	    	}
		}
    }

    if ( pmods == NULL || pmods[ i ] == NULL ) {
		if (( pmods = (LDAPMod **)realloc( pmods, (i + 2) *
			sizeof( LDAPMod * ))) == NULL ) {
	    		tester_perror( "realloc", NULL );
	    		exit( EXIT_FAILURE );
		}
		*pmodsp = pmods;
		pmods[ i + 1 ] = NULL;
		if (( pmods[ i ] = (LDAPMod *)calloc( 1, sizeof( LDAPMod )))
			== NULL ) {
	    		tester_perror( "calloc", NULL );
	    		exit( EXIT_FAILURE );
		}
		pmods[ i ]->mod_op = modop;
		if (( pmods[ i ]->mod_type = strdup( attr )) == NULL ) {
	    	tester_perror( "strdup", NULL );
	    	exit( EXIT_FAILURE );
		}
    }

    if ( value != NULL ) {
		j = 0;
		if ( pmods[ i ]->mod_bvalues != NULL ) {
	    	for ( ; pmods[ i ]->mod_bvalues[ j ] != NULL; ++j ) {
				;
	    	}
		}
		if (( pmods[ i ]->mod_bvalues =
			(struct berval **)ber_memrealloc( pmods[ i ]->mod_bvalues,
			(j + 2) * sizeof( struct berval * ))) == NULL ) {
	    		tester_perror( "ber_memrealloc", NULL );
	    		exit( EXIT_FAILURE );
		}
		pmods[ i ]->mod_bvalues[ j + 1 ] = NULL;
		if (( bvp = (struct berval *)ber_memalloc( sizeof( struct berval )))
			== NULL ) {
	    		tester_perror( "ber_memalloc", NULL );
	    		exit( EXIT_FAILURE );
		}
		pmods[ i ]->mod_bvalues[ j ] = bvp;

	    bvp->bv_len = vlen;
	    if (( bvp->bv_val = (char *)malloc( vlen + 1 )) == NULL ) {
			tester_perror( "malloc", NULL );
			exit( EXIT_FAILURE );
	    }
	    AC_MEMCPY( bvp->bv_val, value, vlen );
	    bvp->bv_val[ vlen ] = '\0';
    }
}


static char *
get_add_entry( char *filename, LDAPMod ***mods )
{
	FILE    *fp;
	char    *entry = NULL;

	if ( (fp = fopen( filename, "r" )) != NULL ) {
		char  line[BUFSIZ];

		if ( fgets( line, BUFSIZ, fp )) {
			char *nl;

			if (( nl = strchr( line, '\r' )) || ( nl = strchr( line, '\n' )))
				*nl = '\0';
			nl = line;
			if ( !strncasecmp( nl, "dn: ", 4 ))
				nl += 4;
			entry = strdup( nl );

		}

		while ( fgets( line, BUFSIZ, fp )) {
			char	*nl;
			char	*value;

			if (( nl = strchr( line, '\r' )) || ( nl = strchr( line, '\n' )))
				*nl = '\0';

			if ( *line == '\0' ) break;
			if ( !( value = strchr( line, ':' ))) break;

			*value++ = '\0'; 
			while ( *value && isspace( (unsigned char) *value ))
				value++;

			addmodifyop( mods, LDAP_MOD_ADD, line, value, strlen( value ));

		}
		fclose( fp );
	}

	return( entry );
}


static void
do_addel(
	char *uri,
	char *manager,
	struct berval *passwd,
	char *entry,
	LDAPMod **attrs,
	int maxloop,
	int maxretries,
	int delay,
	int friendly,
	int chaserefs )
{
	LDAP	*ld = NULL;
	int  	i = 0, do_retry = maxretries;
	int	rc = LDAP_SUCCESS;
	int	version = LDAP_VERSION3;

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
		fprintf( stderr, "PID=%ld - Add/Delete(%d): entry=\"%s\".\n",
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

	for ( ; i < maxloop; i++ ) {

		/* add the entry */
		rc = ldap_add_ext_s( ld, entry, attrs, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_add_ext_s", NULL );
			switch ( rc ) {
			case LDAP_ALREADY_EXISTS:
				/* NOTE: this likely means
				 * the delete failed
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

#if 0
		/* wait a second for the add to really complete */
		/* This masks some race conditions though. */
		sleep( 1 );
#endif

		/* now delete the entry again */
		rc = ldap_delete_ext_s( ld, entry, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			tester_ldap_error( ld, "ldap_delete_ext_s", NULL );
			switch ( rc ) {
			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: this likely means
				 * the add failed
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
	fprintf( stderr, "  PID=%ld - Add/Delete done (%d).\n", (long) pid, rc );

	ldap_unbind_ext( ld, NULL, NULL );
}


