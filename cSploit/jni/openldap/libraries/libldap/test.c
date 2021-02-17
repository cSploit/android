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

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <sys/stat.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <fcntl.h>

/* including the "internal" defs is legit and nec. since this test routine has 
 * a-priori knowledge of libldap internal workings.
 * hodges@stanford.edu 5-Feb-96
 */
#include "ldap-int.h"

/* local functions */
static char *get_line LDAP_P(( char *line, int len, FILE *fp, const char *prompt ));
static char **get_list LDAP_P(( const char *prompt ));
static int file_read LDAP_P(( const char *path, struct berval *bv ));
static LDAPMod **get_modlist LDAP_P(( const char *prompt1,
	const char *prompt2, const char *prompt3 ));
static void handle_result LDAP_P(( LDAP *ld, LDAPMessage *lm ));
static void print_ldap_result LDAP_P(( LDAP *ld, LDAPMessage *lm,
	const char *s ));
static void print_search_entry LDAP_P(( LDAP *ld, LDAPMessage *res ));
static void free_list LDAP_P(( char **list ));

static char *dnsuffix;

static char *
get_line( char *line, int len, FILE *fp, const char *prompt )
{
	fputs(prompt, stdout);

	if ( fgets( line, len, fp ) == NULL )
		return( NULL );

	line[ strlen( line ) - 1 ] = '\0';

	return( line );
}

static char **
get_list( const char *prompt )
{
	static char	buf[256];
	int		num;
	char		**result;

	num = 0;
	result = (char **) 0;
	while ( 1 ) {
		get_line( buf, sizeof(buf), stdin, prompt );

		if ( *buf == '\0' )
			break;

		if ( result == (char **) 0 )
			result = (char **) malloc( sizeof(char *) );
		else
			result = (char **) realloc( result,
			    sizeof(char *) * (num + 1) );

		result[num++] = (char *) strdup( buf );
	}
	if ( result == (char **) 0 )
		return( NULL );
	result = (char **) realloc( result, sizeof(char *) * (num + 1) );
	result[num] = NULL;

	return( result );
}


static void
free_list( char **list )
{
	int	i;

	if ( list != NULL ) {
		for ( i = 0; list[ i ] != NULL; ++i ) {
			free( list[ i ] );
		}
		free( (char *)list );
	}
}


static int
file_read( const char *path, struct berval *bv )
{
	FILE		*fp;
	ber_slen_t	rlen;
	int		eof;

	if (( fp = fopen( path, "r" )) == NULL ) {
	    	perror( path );
		return( -1 );
	}

	if ( fseek( fp, 0L, SEEK_END ) != 0 ) {
		perror( path );
		fclose( fp );
		return( -1 );
	}

	bv->bv_len = ftell( fp );

	if (( bv->bv_val = (char *)malloc( bv->bv_len )) == NULL ) {
		perror( "malloc" );
		fclose( fp );
		return( -1 );
	}

	if ( fseek( fp, 0L, SEEK_SET ) != 0 ) {
		perror( path );
		fclose( fp );
		return( -1 );
	}

	rlen = fread( bv->bv_val, 1, bv->bv_len, fp );
	eof = feof( fp );
	fclose( fp );

	if ( (ber_len_t) rlen != bv->bv_len ) {
		perror( path );
		free( bv->bv_val );
		return( -1 );
	}

	return( bv->bv_len );
}


static LDAPMod **
get_modlist(
	const char *prompt1,
	const char *prompt2,
	const char *prompt3 )
{
	static char	buf[256];
	int		num;
	LDAPMod		tmp = { 0 };
	LDAPMod		**result;
	struct berval	**bvals;

	num = 0;
	result = NULL;
	while ( 1 ) {
		if ( prompt1 ) {
			get_line( buf, sizeof(buf), stdin, prompt1 );
			tmp.mod_op = atoi( buf );

			if ( tmp.mod_op == -1 || buf[0] == '\0' )
				break;
		}

		get_line( buf, sizeof(buf), stdin, prompt2 );
		if ( buf[0] == '\0' )
			break;
		tmp.mod_type = strdup( buf );

		tmp.mod_values = get_list( prompt3 );

		if ( tmp.mod_values != NULL ) {
			int	i;

			for ( i = 0; tmp.mod_values[i] != NULL; ++i )
				;
			bvals = (struct berval **)calloc( i + 1,
			    sizeof( struct berval *));
			for ( i = 0; tmp.mod_values[i] != NULL; ++i ) {
				bvals[i] = (struct berval *)malloc(
				    sizeof( struct berval ));
				if ( strncmp( tmp.mod_values[i], "{FILE}",
				    6 ) == 0 ) {
					if ( file_read( tmp.mod_values[i] + 6,
					    bvals[i] ) < 0 ) {
						free( bvals );
						for ( i = 0; i<num; i++ )
							free( result[ i ] );
						free( result );
						return( NULL );
					}
				} else {
					bvals[i]->bv_val = tmp.mod_values[i];
					bvals[i]->bv_len =
					    strlen( tmp.mod_values[i] );
				}
			}
			tmp.mod_bvalues = bvals;
			tmp.mod_op |= LDAP_MOD_BVALUES;
		}

		if ( result == NULL )
			result = (LDAPMod **) malloc( sizeof(LDAPMod *) );
		else
			result = (LDAPMod **) realloc( result,
			    sizeof(LDAPMod *) * (num + 1) );

		result[num] = (LDAPMod *) malloc( sizeof(LDAPMod) );
		*(result[num]) = tmp;	/* struct copy */
		num++;
	}
	if ( result == NULL )
		return( NULL );
	result = (LDAPMod **) realloc( result, sizeof(LDAPMod *) * (num + 1) );
	result[num] = NULL;

	return( result );
}


static int
bind_prompt( LDAP *ld,
	LDAP_CONST char *url,
	ber_tag_t request, ber_int_t msgid,
	void *params )
{
	static char	dn[256], passwd[256];
	int	authmethod;

	printf("rebind for request=%ld msgid=%ld url=%s\n",
		request, (long) msgid, url );

	authmethod = LDAP_AUTH_SIMPLE;

		get_line( dn, sizeof(dn), stdin, "re-bind dn? " );
		strcat( dn, dnsuffix );

	if ( authmethod == LDAP_AUTH_SIMPLE && dn[0] != '\0' ) {
			get_line( passwd, sizeof(passwd), stdin,
			    "re-bind password? " );
		} else {
			passwd[0] = '\0';
		}

	return ldap_bind_s( ld, dn, passwd, authmethod);
}


int
main( int argc, char **argv )
{
	LDAP		*ld = NULL;
	int		i, c, port, errflg, method, id, msgtype;
	char		line[256], command1, command2, command3;
	char		passwd[64], dn[256], rdn[64], attr[64], value[256];
	char		filter[256], *host, **types;
	char		**exdn;
	static const char usage[] =
		"usage: %s [-u] [-h host] [-d level] [-s dnsuffix] [-p port] [-t file] [-T file]\n";
	int		bound, all, scope, attrsonly;
	LDAPMessage	*res;
	LDAPMod		**mods, **attrs;
	struct timeval	timeout;
	char		*copyfname = NULL;
	int		copyoptions = 0;
	LDAPURLDesc	*ludp;

	host = NULL;
	port = LDAP_PORT;
	dnsuffix = "";
	errflg = 0;

	while (( c = getopt( argc, argv, "h:d:s:p:t:T:" )) != -1 ) {
		switch( c ) {
		case 'd':
#ifdef LDAP_DEBUG
			ldap_debug = atoi( optarg );
#ifdef LBER_DEBUG
			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
				ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &ldap_debug );
			}
#endif
#else
			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
#endif
			break;

		case 'h':
			host = optarg;
			break;

		case 's':
			dnsuffix = optarg;
			break;

		case 'p':
			port = atoi( optarg );
			break;

		case 't':	/* copy ber's to given file */
			copyfname = strdup( optarg );
/*			copyoptions = LBER_TO_FILE; */
			break;

		case 'T':	/* only output ber's to given file */
			copyfname = strdup( optarg );
/*			copyoptions = (LBER_TO_FILE | LBER_TO_FILE_ONLY); */
			break;

		default:
		    ++errflg;
		}
	}

	if ( host == NULL && optind == argc - 1 ) {
		host = argv[ optind ];
		++optind;
	}

	if ( errflg || optind < argc - 1 ) {
		fprintf( stderr, usage, argv[ 0 ] );
		exit( EXIT_FAILURE );
	}
	
	printf( "ldap_init( %s, %d )\n",
		host == NULL ? "(null)" : host, port );

	ld = ldap_init( host, port );

	if ( ld == NULL ) {
		perror( "ldap_init" );
		exit( EXIT_FAILURE );
	}

	if ( copyfname != NULL ) {
		if ( ( ld->ld_sb->sb_fd = open( copyfname, O_WRONLY|O_CREAT|O_EXCL,
		    0600 ))  == -1 ) {
			perror( copyfname );
			exit ( EXIT_FAILURE );
		}
		ld->ld_sb->sb_options = copyoptions;
	}

	bound = 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	(void) memset( line, '\0', sizeof(line) );
	while ( get_line( line, sizeof(line), stdin, "\ncommand? " ) != NULL ) {
		command1 = line[0];
		command2 = line[1];
		command3 = line[2];

		switch ( command1 ) {
		case 'a':	/* add or abandon */
			switch ( command2 ) {
			case 'd':	/* add */
				get_line( dn, sizeof(dn), stdin, "dn? " );
				strcat( dn, dnsuffix );
				if ( (attrs = get_modlist( NULL, "attr? ",
				    "value? " )) == NULL )
					break;
				if ( (id = ldap_add( ld, dn, attrs )) == -1 )
					ldap_perror( ld, "ldap_add" );
				else
					printf( "Add initiated with id %d\n",
					    id );
				break;

			case 'b':	/* abandon */
				get_line( line, sizeof(line), stdin, "msgid? " );
				id = atoi( line );
				if ( ldap_abandon( ld, id ) != 0 )
					ldap_perror( ld, "ldap_abandon" );
				else
					printf( "Abandon successful\n" );
				break;
			default:
				printf( "Possibilities: [ad]d, [ab]ort\n" );
			}
			break;

		case 'b':	/* asynch bind */
			method = LDAP_AUTH_SIMPLE;
			get_line( dn, sizeof(dn), stdin, "dn? " );
			strcat( dn, dnsuffix );

			if ( method == LDAP_AUTH_SIMPLE && dn[0] != '\0' )
				get_line( passwd, sizeof(passwd), stdin,
				    "password? " );
			else
				passwd[0] = '\0';

			if ( ldap_bind( ld, dn, passwd, method ) == -1 ) {
				fprintf( stderr, "ldap_bind failed\n" );
				ldap_perror( ld, "ldap_bind" );
			} else {
				printf( "Bind initiated\n" );
				bound = 1;
			}
			break;

		case 'B':	/* synch bind */
			method = LDAP_AUTH_SIMPLE;
			get_line( dn, sizeof(dn), stdin, "dn? " );
			strcat( dn, dnsuffix );

			if ( dn[0] != '\0' )
				get_line( passwd, sizeof(passwd), stdin,
				    "password? " );
			else
				passwd[0] = '\0';

			if ( ldap_bind_s( ld, dn, passwd, method ) !=
			    LDAP_SUCCESS ) {
				fprintf( stderr, "ldap_bind_s failed\n" );
				ldap_perror( ld, "ldap_bind_s" );
			} else {
				printf( "Bind successful\n" );
				bound = 1;
			}
			break;

		case 'c':	/* compare */
			get_line( dn, sizeof(dn), stdin, "dn? " );
			strcat( dn, dnsuffix );
			get_line( attr, sizeof(attr), stdin, "attr? " );
			get_line( value, sizeof(value), stdin, "value? " );

			if ( (id = ldap_compare( ld, dn, attr, value )) == -1 )
				ldap_perror( ld, "ldap_compare" );
			else
				printf( "Compare initiated with id %d\n", id );
			break;

		case 'd':	/* turn on debugging */
#ifdef LDAP_DEBUG
			get_line( line, sizeof(line), stdin, "debug level? " );
			ldap_debug = atoi( line );
#ifdef LBER_DEBUG
			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
				ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &ldap_debug );
			}
#endif
#else
			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
#endif
			break;

		case 'E':	/* explode a dn */
			get_line( line, sizeof(line), stdin, "dn? " );
			exdn = ldap_explode_dn( line, 0 );
			for ( i = 0; exdn != NULL && exdn[i] != NULL; i++ ) {
				printf( "\t%s\n", exdn[i] );
			}
			break;

		case 'g':	/* set next msgid */
			get_line( line, sizeof(line), stdin, "msgid? " );
			ld->ld_msgid = atoi( line );
			break;

		case 'v':	/* set version number */
			get_line( line, sizeof(line), stdin, "version? " );
			ld->ld_version = atoi( line );
			break;

		case 'm':	/* modify or modifyrdn */
			if ( strncmp( line, "modify", 4 ) == 0 ) {
				get_line( dn, sizeof(dn), stdin, "dn? " );
				strcat( dn, dnsuffix );
				if ( (mods = get_modlist(
				    "mod (0=>add, 1=>delete, 2=>replace -1=>done)? ",
				    "attribute type? ", "attribute value? " ))
				    == NULL )
					break;
				if ( (id = ldap_modify( ld, dn, mods )) == -1 )
					ldap_perror( ld, "ldap_modify" );
				else
					printf( "Modify initiated with id %d\n",
					    id );
			} else if ( strncmp( line, "modrdn", 4 ) == 0 ) {
				get_line( dn, sizeof(dn), stdin, "dn? " );
				strcat( dn, dnsuffix );
				get_line( rdn, sizeof(rdn), stdin, "newrdn? " );
				if ( (id = ldap_modrdn( ld, dn, rdn )) == -1 )
					ldap_perror( ld, "ldap_modrdn" );
				else
					printf( "Modrdn initiated with id %d\n",
					    id );
			} else {
				printf( "Possibilities: [modi]fy, [modr]dn\n" );
			}
			break;

		case 'q':	/* quit */
			ldap_unbind( ld );
			exit( EXIT_SUCCESS );
			break;

		case 'r':	/* result or remove */
			switch ( command3 ) {
			case 's':	/* result */
				get_line( line, sizeof(line), stdin,
				    "msgid (-1=>any)? " );
				if ( line[0] == '\0' )
					id = -1;
				else
					id = atoi( line );
				get_line( line, sizeof(line), stdin,
				    "all (0=>any, 1=>all)? " );
				if ( line[0] == '\0' )
					all = 1;
				else
					all = atoi( line );
				if (( msgtype = ldap_result( ld, id, all,
				    &timeout, &res )) < 1 ) {
					ldap_perror( ld, "ldap_result" );
					break;
				}
				printf( "\nresult: msgtype %d msgid %d\n",
				    msgtype, res->lm_msgid );
				handle_result( ld, res );
				res = NULL;
				break;

			case 'm':	/* remove */
				get_line( dn, sizeof(dn), stdin, "dn? " );
				strcat( dn, dnsuffix );
				if ( (id = ldap_delete( ld, dn )) == -1 )
					ldap_perror( ld, "ldap_delete" );
				else
					printf( "Remove initiated with id %d\n",
					    id );
				break;

			default:
				printf( "Possibilities: [rem]ove, [res]ult\n" );
				break;
			}
			break;

		case 's':	/* search */
			get_line( dn, sizeof(dn), stdin, "searchbase? " );
			strcat( dn, dnsuffix );
			get_line( line, sizeof(line), stdin,
			    "scope (0=baseObject, 1=oneLevel, 2=subtree, 3=children)? " );
			scope = atoi( line );
			get_line( filter, sizeof(filter), stdin,
			    "search filter (e.g. sn=jones)? " );
			types = get_list( "attrs to return? " );
			get_line( line, sizeof(line), stdin,
			    "attrsonly (0=attrs&values, 1=attrs only)? " );
			attrsonly = atoi( line );

			    if (( id = ldap_search( ld, dn, scope, filter,
				    types, attrsonly  )) == -1 ) {
				ldap_perror( ld, "ldap_search" );
			    } else {
				printf( "Search initiated with id %d\n", id );
			    }
			free_list( types );
			break;

		case 't':	/* set timeout value */
			get_line( line, sizeof(line), stdin, "timeout? " );
			timeout.tv_sec = atoi( line );
			break;

		case 'p':	/* parse LDAP URL */
			get_line( line, sizeof(line), stdin, "LDAP URL? " );
			if (( i = ldap_url_parse( line, &ludp )) != 0 ) {
			    fprintf( stderr, "ldap_url_parse: error %d\n", i );
			} else {
			    printf( "\t  host: " );
			    if ( ludp->lud_host == NULL ) {
				printf( "DEFAULT\n" );
			    } else {
				printf( "<%s>\n", ludp->lud_host );
			    }
			    printf( "\t  port: " );
			    if ( ludp->lud_port == 0 ) {
				printf( "DEFAULT\n" );
			    } else {
				printf( "%d\n", ludp->lud_port );
			    }
			    printf( "\t    dn: <%s>\n", ludp->lud_dn );
			    printf( "\t attrs:" );
			    if ( ludp->lud_attrs == NULL ) {
				printf( " ALL" );
			    } else {
				for ( i = 0; ludp->lud_attrs[ i ] != NULL; ++i ) {
				    printf( " <%s>", ludp->lud_attrs[ i ] );
				}
			    }
			    printf( "\n\t scope: %s\n",
					ludp->lud_scope == LDAP_SCOPE_BASE ? "baseObject"
					: ludp->lud_scope == LDAP_SCOPE_ONELEVEL ? "oneLevel"
					: ludp->lud_scope == LDAP_SCOPE_SUBTREE ? "subtree"
#ifdef LDAP_SCOPE_SUBORDINATE
					: ludp->lud_scope == LDAP_SCOPE_SUBORDINATE ? "children"
#endif
					: "**invalid**" );
			    printf( "\tfilter: <%s>\n", ludp->lud_filter );
			    ldap_free_urldesc( ludp );
			}
			    break;

		case 'n':	/* set dn suffix, for convenience */
			get_line( line, sizeof(line), stdin, "DN suffix? " );
			strcpy( dnsuffix, line );
			break;

		case 'o':	/* set ldap options */
			get_line( line, sizeof(line), stdin, "alias deref (0=never, 1=searching, 2=finding, 3=always)?" );
			ld->ld_deref = atoi( line );
			get_line( line, sizeof(line), stdin, "timelimit?" );
			ld->ld_timelimit = atoi( line );
			get_line( line, sizeof(line), stdin, "sizelimit?" );
			ld->ld_sizelimit = atoi( line );

			LDAP_BOOL_ZERO(&ld->ld_options);

			get_line( line, sizeof(line), stdin,
				"Recognize and chase referrals (0=no, 1=yes)?" );
			if ( atoi( line ) != 0 ) {
				LDAP_BOOL_SET(&ld->ld_options, LDAP_BOOL_REFERRALS);
				get_line( line, sizeof(line), stdin,
					"Prompt for bind credentials when chasing referrals (0=no, 1=yes)?" );
				if ( atoi( line ) != 0 ) {
					ldap_set_rebind_proc( ld, bind_prompt, NULL );
				}
			}
			break;

		case '?':	/* help */
			printf(
"Commands: [ad]d         [ab]andon         [b]ind\n"
"          [B]ind async  [c]ompare\n"
"          [modi]fy      [modr]dn          [rem]ove\n"
"          [res]ult      [s]earch          [q]uit/unbind\n\n"
"          [d]ebug       set ms[g]id\n"
"          d[n]suffix    [t]imeout         [v]ersion\n"
"          [?]help       [o]ptions"
"          [E]xplode dn  [p]arse LDAP URL\n" );
			break;

		default:
			printf( "Invalid command.  Type ? for help.\n" );
			break;
		}

		(void) memset( line, '\0', sizeof(line) );
	}

	return( 0 );
}

static void
handle_result( LDAP *ld, LDAPMessage *lm )
{
	switch ( lm->lm_msgtype ) {
	case LDAP_RES_COMPARE:
		printf( "Compare result\n" );
		print_ldap_result( ld, lm, "compare" );
		break;

	case LDAP_RES_SEARCH_RESULT:
		printf( "Search result\n" );
		print_ldap_result( ld, lm, "search" );
		break;

	case LDAP_RES_SEARCH_ENTRY:
		printf( "Search entry\n" );
		print_search_entry( ld, lm );
		break;

	case LDAP_RES_ADD:
		printf( "Add result\n" );
		print_ldap_result( ld, lm, "add" );
		break;

	case LDAP_RES_DELETE:
		printf( "Delete result\n" );
		print_ldap_result( ld, lm, "delete" );
		break;

	case LDAP_RES_MODRDN:
		printf( "ModRDN result\n" );
		print_ldap_result( ld, lm, "modrdn" );
		break;

	case LDAP_RES_BIND:
		printf( "Bind result\n" );
		print_ldap_result( ld, lm, "bind" );
		break;

	default:
		printf( "Unknown result type 0x%lx\n",
		        (unsigned long) lm->lm_msgtype );
		print_ldap_result( ld, lm, "unknown" );
	}
}

static void
print_ldap_result( LDAP *ld, LDAPMessage *lm, const char *s )
{
	ldap_result2error( ld, lm, 1 );
	ldap_perror( ld, s );
/*
	if ( ld->ld_error != NULL && *ld->ld_error != '\0' )
		fprintf( stderr, "Additional info: %s\n", ld->ld_error );
	if ( LDAP_NAME_ERROR( ld->ld_errno ) && ld->ld_matched != NULL )
		fprintf( stderr, "Matched DN: %s\n", ld->ld_matched );
*/
}

static void
print_search_entry( LDAP *ld, LDAPMessage *res )
{
	LDAPMessage	*e;

	for ( e = ldap_first_entry( ld, res ); e != NULL;
	    e = ldap_next_entry( ld, e ) )
	{
		BerElement	*ber = NULL;
		char *a, *dn, *ufn;

		if ( e->lm_msgtype == LDAP_RES_SEARCH_RESULT )
			break;

		dn = ldap_get_dn( ld, e );
		printf( "\tDN: %s\n", dn );

		ufn = ldap_dn2ufn( dn );
		printf( "\tUFN: %s\n", ufn );

		free( dn );
		free( ufn );

		for ( a = ldap_first_attribute( ld, e, &ber ); a != NULL;
		    a = ldap_next_attribute( ld, e, ber ) )
		{
			struct berval	**vals;

			printf( "\t\tATTR: %s\n", a );
			if ( (vals = ldap_get_values_len( ld, e, a ))
			    == NULL ) {
				printf( "\t\t\t(no values)\n" );
			} else {
				int i;
				for ( i = 0; vals[i] != NULL; i++ ) {
					int	j, nonascii;

					nonascii = 0;
					for ( j = 0; (ber_len_t) j < vals[i]->bv_len; j++ )
						if ( !isascii( vals[i]->bv_val[j] ) ) {
							nonascii = 1;
							break;
						}

					if ( nonascii ) {
						printf( "\t\t\tlength (%ld) (not ascii)\n", vals[i]->bv_len );
#ifdef BPRINT_NONASCII
						ber_bprint( vals[i]->bv_val,
						    vals[i]->bv_len );
#endif /* BPRINT_NONASCII */
						continue;
					}
					printf( "\t\t\tlength (%ld) %s\n",
					    vals[i]->bv_len, vals[i]->bv_val );
				}
				ber_bvecfree( vals );
			}
		}

		if(ber != NULL) {
			ber_free( ber, 0 );
		}
	}

	if ( res->lm_msgtype == LDAP_RES_SEARCH_RESULT
	    || res->lm_chain != NULL )
		print_ldap_result( ld, res, "search" );
}
