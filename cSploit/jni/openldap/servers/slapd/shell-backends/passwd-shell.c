/* passwd-shell.c - passwd(5) shell-based backend for slapd(8) */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */


#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/string.h>
#include <ac/unistd.h>

#include <pwd.h>

#include <lber.h>
#include <ldap.h>

#include "shellutil.h"

static void pwdfile_search LDAP_P(( struct ldop *op, FILE *ofp ));
static struct ldentry *pw2entry LDAP_P(( struct ldop *op, struct passwd *pw ));

static char	tmpbuf[ MAXLINELEN * 2 ];


int
main( int argc, char **argv )
{
    int			c, errflg;
    struct ldop		op;

    if (( progname = strrchr( argv[ 0 ], '/' )) == NULL ) {
	progname = estrdup( argv[ 0 ] );
    } else {
	progname = estrdup( progname + 1 );
    }

    errflg = debugflg = 0;

    while (( c = getopt( argc, argv, "d" )) != EOF ) {
	switch( c ) {
	case 'd':
#ifdef LDAP_DEBUG
	    ++debugflg;
#else /* LDAP_DEBUG */
	    fprintf( stderr, "%s: compile with -DLDAP_DEBUG for debugging\n",
		    progname );
#endif /* LDAP_DEBUG */
	    break;
	default:
	    ++errflg;
	}
    }

    if ( errflg || optind < argc ) {
	fprintf( stderr, "usage: %s [-d]\n", progname );
	exit( EXIT_FAILURE );
    }

    debug_printf( "started\n" );

    (void) memset( (char *)&op, '\0', sizeof( op ));

    if ( parse_input( stdin, stdout, &op ) < 0 ) {
	exit( EXIT_SUCCESS );
    }

    if ( op.ldop_op != LDOP_SEARCH ) {
	write_result( stdout, LDAP_UNWILLING_TO_PERFORM, NULL,
		"Command Not Implemented" );
	exit( EXIT_SUCCESS );
    }

#ifdef LDAP_DEBUG
    dump_ldop( &op );
#endif /* LDAP_DEBUG */

    pwdfile_search( &op, stdout );

    exit( EXIT_SUCCESS );
}


static void
pwdfile_search( struct ldop *op, FILE *ofp )
{
    struct passwd	*pw;
    struct ldentry	*entry;
    int			oneentry;

    oneentry = ( strchr( op->ldop_dn, '@' ) != NULL );

    for ( pw = getpwent(); pw != NULL; pw = getpwent()) {
	if (( entry = pw2entry( op, pw )) != NULL ) {
	    if ( oneentry ) {
		if ( strcasecmp( op->ldop_dn, entry->lde_dn ) == 0 ) {
		    write_entry( op, entry, ofp );
		    break;
		}
	    } else if ( test_filter( op, entry ) == LDAP_COMPARE_TRUE ) {
			write_entry( op, entry, ofp );
	    }
	    free_entry( entry );
	}
    }
    endpwent();

    write_result( ofp, LDAP_SUCCESS, NULL, NULL );
}


static struct ldentry *
pw2entry( struct ldop *op, struct passwd *pw )
{
    struct ldentry	*entry;
    struct ldattr	*attr;
    int			i;

    /* 
     * construct the DN from pw_name
     */
    if ( strchr( op->ldop_suffixes[ 0 ], '=' ) != NULL ) {
	/*
	 * X.500 style DN
	 */
	i = snprintf( tmpbuf, sizeof( tmpbuf ), "cn=%s, %s", pw->pw_name, op->ldop_suffixes[ 0 ] );
    } else {
	/*
	 * RFC-822 style DN
	 */
	i = snprintf( tmpbuf, sizeof( tmpbuf ), "%s@%s", pw->pw_name, op->ldop_suffixes[ 0 ] );
    }

    if ( i < 0 || i >= sizeof( tmpbuf ) ) {
        return NULL;
    }

    entry = (struct ldentry *) ecalloc( 1, sizeof( struct ldentry ));
    entry->lde_dn = estrdup( tmpbuf );

    /*
     * for now, we simply derive the LDAP attribute values as follows:
     *  objectClass = person
     *  uid = pw_name
     *  sn = pw_name
     *  cn = pw_name
     *  cn = pw_gecos	(second common name)
     */
    entry->lde_attrs = (struct ldattr **)ecalloc( 5, sizeof( struct ldattr * ));
    i = 0;
    attr = (struct ldattr *)ecalloc( 1, sizeof( struct ldattr ));
    attr->lda_name = estrdup( "objectClass" );
    attr->lda_values = (char **)ecalloc( 2, sizeof( char * ));
    attr->lda_values[ 0 ] = estrdup( "person" );
    entry->lde_attrs[ i++ ] = attr;

    attr = (struct ldattr *)ecalloc( 1, sizeof( struct ldattr ));
    attr->lda_name = estrdup( "uid" );
    attr->lda_values = (char **)ecalloc( 2, sizeof( char * ));
    attr->lda_values[ 0 ] = estrdup( pw->pw_name );
    entry->lde_attrs[ i++ ] = attr;

    attr = (struct ldattr *)ecalloc( 1, sizeof( struct ldattr ));
    attr->lda_name = estrdup( "sn" );
    attr->lda_values = (char **)ecalloc( 2, sizeof( char * ));
    attr->lda_values[ 0 ] = estrdup( pw->pw_name );
    entry->lde_attrs[ i++ ] = attr;

    attr = (struct ldattr *)ecalloc( 1, sizeof( struct ldattr ));
    attr->lda_name = estrdup( "cn" );
    attr->lda_values = (char **)ecalloc( 3, sizeof( char * ));
    attr->lda_values[ 0 ] = estrdup( pw->pw_name );
    if ( pw->pw_gecos != NULL && *pw->pw_gecos != '\0' ) {
	attr->lda_values[ 1 ] = estrdup( pw->pw_gecos );
    }
    entry->lde_attrs[ i++ ] = attr;

    return( entry );
}
