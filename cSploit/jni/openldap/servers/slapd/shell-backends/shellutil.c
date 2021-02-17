/* shellutil.c - common routines useful when building shell-based backends */
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
#include <ac/stdarg.h>

#include <pwd.h>

#include <ac/ctype.h>
#include <ac/string.h>

#include <lber.h>
#include <ldap.h>
#include "shellutil.h"


int	debugflg;
char	*progname;

static struct inputparams	ips[] = {
    IP_TYPE_SUFFIX,	"suffix",
    IP_TYPE_BASE,	"base",
    IP_TYPE_SCOPE,	"scope",
    IP_TYPE_ALIASDEREF,	"deref",
    IP_TYPE_SIZELIMIT,	"sizelimit",
    IP_TYPE_TIMELIMIT,	"timelimit",
    IP_TYPE_FILTER,	"filter",
    IP_TYPE_ATTRS,	"attrs",
    IP_TYPE_ATTRSONLY,	"attrsonly",
    0,			NULL
};


void
write_result( FILE *fp, int code, char *matched, char *info )
{
    fprintf( fp, "RESULT\ncode: %d\n", code );
    debug_printf( ">> RESULT\n" );
    debug_printf( ">> code: %d\n", code );

    if ( matched != NULL ) {
	fprintf( fp, "matched: %s\n", matched );
	debug_printf( ">> matched: %s\n", matched );
    }

    if ( info != NULL ) {
	fprintf( fp, "info: %s\n", info );
	debug_printf( ">> info: %s\n", info );
    }
}


void
write_entry( struct ldop *op, struct ldentry *entry, FILE *ofp )
{
    struct ldattr	**app;
    char		**valp;

    fprintf( ofp, "dn: %s\n", entry->lde_dn );
    for ( app = entry->lde_attrs; *app != NULL; ++app ) {
	if ( attr_requested( (*app)->lda_name, op )) {
	    for ( valp = (*app)->lda_values; *valp != NULL; ++valp ) {
		fprintf( ofp, "%s: %s\n", (*app)->lda_name, *valp );
	    }
	}
    }
    fputc( '\n', ofp );
}


int
test_filter( struct ldop *op, struct ldentry *entry )
{
    return ((random() & 0x07 ) == 0x07) /* XXX random for now */
		? LDAP_COMPARE_TRUE : LDAP_COMPARE_FALSE;
}


int
attr_requested( char *name, struct ldop *op )
{
    char	**ap;

    if ( op->ldop_srch.ldsp_attrs == NULL ) {	/* special case */
	return( 1 );
    }

    for ( ap = op->ldop_srch.ldsp_attrs; *ap != NULL; ++ap ) {
	if ( strcasecmp( name, *ap ) == 0 ) {
	    return( 1 );
	}
    }

    return( 0 );
}


void
free_entry( struct ldentry *entry )
{
    struct ldattr	**app;
    char		**valp;

    free( entry->lde_dn );

    for ( app = entry->lde_attrs; *app != NULL; ++app ) {
	for ( valp = (*app)->lda_values; *valp != NULL; ++valp ) {
	    free( *valp );
	}
	free( (*app)->lda_values );
	free( (*app)->lda_name );
    }

    free( entry->lde_attrs );
    free( entry );
}


int
parse_input( FILE *ifp, FILE *ofp, struct ldop *op )
{
    char		*p, *args, line[ MAXLINELEN + 1 ];
    struct inputparams	*ip;

    if ( fgets( line, MAXLINELEN, ifp ) == NULL ) {
	write_result( ofp, LDAP_OTHER, NULL, "Empty Input" );
    }
    line[ strlen( line ) - 1 ] = '\0';
    if ( strncasecmp( line, STR_OP_SEARCH, sizeof( STR_OP_SEARCH ) - 1 )
	    != 0 ) {
	write_result( ofp, LDAP_UNWILLING_TO_PERFORM, NULL,
		"Operation Not Supported" );
	return( -1 );
    }

    op->ldop_op = LDOP_SEARCH;

    while ( fgets( line, MAXLINELEN, ifp ) != NULL ) {
	line[ strlen( line ) - 1 ] = '\0';
	debug_printf( "<< %s\n", line );

	args = line;
	if (( ip = find_input_tag( &args )) == NULL ) {
	    debug_printf( "ignoring %s\n", line );
	    continue;
	}

	switch( ip->ip_type ) {
	case IP_TYPE_SUFFIX:
	    add_strval( &op->ldop_suffixes, args );
	    break;
	case IP_TYPE_BASE:
	    op->ldop_dn = estrdup( args );
	    break;
	case IP_TYPE_SCOPE:
	    if ( lutil_atoi( &op->ldop_srch.ldsp_scope, args ) != 0 ||
		( op->ldop_srch.ldsp_scope != LDAP_SCOPE_BASE &&
		    op->ldop_srch.ldsp_scope != LDAP_SCOPE_ONELEVEL &&
		    op->ldop_srch.ldsp_scope != LDAP_SCOPE_SUBTREE ) )
	    {
		write_result( ofp, LDAP_OTHER, NULL, "Bad scope" );
		return( -1 );
	    }
	    break;
	case IP_TYPE_ALIASDEREF:
	    if ( lutil_atoi( &op->ldop_srch.ldsp_aliasderef, args ) != 0 ) {
		write_result( ofp, LDAP_OTHER, NULL, "Bad alias deref" );
		return( -1 );
	    }
	    break;
	case IP_TYPE_SIZELIMIT:
	    if ( lutil_atoi( &op->ldop_srch.ldsp_sizelimit, args ) != 0 ) {
		write_result( ofp, LDAP_OTHER, NULL, "Bad size limit" );
		return( -1 );
	    }
	    break;
	case IP_TYPE_TIMELIMIT:
	    if ( lutil_atoi( &op->ldop_srch.ldsp_timelimit, args ) != 0 ) {
		write_result( ofp, LDAP_OTHER, NULL, "Bad time limit" );
		return( -1 );
	    }
	    break;
	case IP_TYPE_FILTER:
	    op->ldop_srch.ldsp_filter = estrdup( args );
	    break;
	case IP_TYPE_ATTRSONLY:
	    op->ldop_srch.ldsp_attrsonly = ( *args != '0' );
	    break;
	case IP_TYPE_ATTRS:
	    if ( strcmp( args, "all" ) == 0 ) {
		op->ldop_srch.ldsp_attrs = NULL;
	    } else {
		while ( args != NULL ) {
		    if (( p = strchr( args, ' ' )) != NULL ) {
			*p++ = '\0';
			while ( isspace( (unsigned char) *p )) {
			    ++p;
			}
		    }
		    add_strval( &op->ldop_srch.ldsp_attrs, args );
		    args = p;
		}
	    }
	    break;
	}
    }

    if ( op->ldop_suffixes == NULL || op->ldop_dn == NULL ||
		op->ldop_srch.ldsp_filter == NULL ) {
	write_result( ofp, LDAP_OTHER, NULL,
		"Required suffix:, base:, or filter: missing" );
	return( -1 );
    }

    return( 0 );
}


struct inputparams *
find_input_tag( char **linep )	/* linep is set to start of args */
{
    int		i;
    char	*p;

    if (( p = strchr( *linep, ':' )) == NULL || p == *linep ) {
	return( NULL );
    }

    for ( i = 0; ips[ i ].ip_type != 0; ++i ) {
	if ( strncasecmp( *linep, ips[ i ].ip_tag, p - *linep ) == 0 ) {
	    while ( isspace( (unsigned char) *(++p) )) {
		;
	    }
	    *linep = p;
	    return( &ips[ i ] );
	}
    }

    return( NULL );
}


void
add_strval( char ***sp, char *val )
{
    int		i;
    char	**vallist;

    vallist = *sp;

    if ( vallist == NULL ) {
	i = 0;
    } else {
	for ( i = 0; vallist[ i ] != NULL; ++i ) {
	    ;
	}
    }

    vallist = (char **)erealloc( vallist, ( i + 2 ) * sizeof( char * ));
    vallist[ i ] = estrdup( val );
    vallist[ ++i ] = NULL;
    *sp = vallist;
}


char *
estrdup( char *s )
{
    char	*p;

    if (( p = strdup( s )) == NULL ) {
	debug_printf( "strdup failed\n" );
	exit( EXIT_FAILURE );
    }

    return( p );
}


void *
erealloc( void *s, unsigned size )
{
    char	*p;

    if ( s == NULL ) {
	p = malloc( size );
    } else {
	p = realloc( s, size );
    }

    if ( p == NULL ) {
	debug_printf( "realloc( p, %d ) failed\n", size );
	exit( EXIT_FAILURE );
    }

    return( p );
}


char *
ecalloc( unsigned nelem, unsigned elsize )
{
    char	*p;

    if (( p = calloc( nelem, elsize )) == NULL ) {
	debug_printf( "calloc( %d, %d ) failed\n", nelem, elsize );
	exit( EXIT_FAILURE );
    }

    return( p );
}


#ifdef LDAP_DEBUG

/* VARARGS */
void
debug_printf( const char *fmt, ... )
{
    va_list	ap;

	if ( debugflg ) {
		va_start( ap, fmt );
		fprintf( stderr, "%s: ", progname );
		vfprintf( stderr, fmt, ap );
		va_end( ap );
	}
}


void
dump_ldop( struct ldop *op )
{
    if ( !debugflg ) {
	return;
    }

    debug_printf( "SEARCH operation\n" );
    if ( op->ldop_suffixes == NULL ) {
	debug_printf( "    suffix: NONE\n" );
    } else {
	int	i;
	for ( i = 0; op->ldop_suffixes[ i ] != NULL; ++i ) {
	    debug_printf( "    suffix: <%s>\n", op->ldop_suffixes[ i ] );
	}
    }
    debug_printf( "        dn: <%s>\n", op->ldop_dn );
    debug_printf( "     scope: <%d>\n", op->ldop_srch.ldsp_scope );
    debug_printf( "    filter: <%s>\n", op->ldop_srch.ldsp_filter );
    debug_printf( "aliasderef: <%d>\n", op->ldop_srch.ldsp_aliasderef );
    debug_printf( " sizelimit: <%d>\n", op->ldop_srch.ldsp_sizelimit );
    debug_printf( " timelimit: <%d>\n", op->ldop_srch.ldsp_timelimit );
    debug_printf( " attrsonly: <%d>\n", op->ldop_srch.ldsp_attrsonly );
    if ( op->ldop_srch.ldsp_attrs == NULL ) {
	debug_printf( "     attrs: ALL\n" );
    } else {
	int	i;

	for ( i = 0; op->ldop_srch.ldsp_attrs[ i ] != NULL; ++i ) {
	    debug_printf( "  attrs: <%s>\n", op->ldop_srch.ldsp_attrs[ i ] );
	}
    }
}
#endif /* LDAP_DEBUG */
