/* ldapdelete.c - simple program to delete an entry using LDAP */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
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
/* Portions Copyright (c) 1992-1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.  This
 * software is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).  Additional significant contributors
 * include:
 *   Kurt D. Zeilenga
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/socket.h>
#include <ac/time.h>

#include <ldap.h>
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

#include "common.h"


static int	prune = 0;
static int sizelimit = -1;


static int dodelete LDAP_P((
    LDAP *ld,
    const char *dn));

static int deletechildren LDAP_P((
	LDAP *ld,
	const char *dn,
	int subentries ));

void
usage( void )
{
	fprintf( stderr, _("Delete entries from an LDAP server\n\n"));
	fprintf( stderr, _("usage: %s [options] [dn]...\n"), prog);
	fprintf( stderr, _("	dn: list of DNs to delete. If not given, it will be readed from stdin\n"));
	fprintf( stderr, _("	    or from the file specified with \"-f file\".\n"));
	fprintf( stderr, _("Delete Options:\n"));
	fprintf( stderr, _("  -c         continuous operation mode (do not stop on errors)\n"));
	fprintf( stderr, _("  -f file    read operations from `file'\n"));
	fprintf( stderr, _("  -M         enable Manage DSA IT control (-MM to make critical)\n"));
	fprintf( stderr, _("  -P version protocol version (default: 3)\n"));
	fprintf( stderr, _("  -r         delete recursively\n"));
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "r"
	"cd:D:e:f:h:H:IMnNO:o:p:P:QR:U:vVw:WxX:y:Y:z:Z";

int
handle_private_option( int i )
{
	int ival;
	char *next;
	switch ( i ) {
#if 0
		int crit;
		char *control, *cvalue;
	case 'E': /* delete extensions */
		if( protocol == LDAP_VERSION2 ) {
			fprintf( stderr, _("%s: -E incompatible with LDAPv%d\n"),
				prog, protocol );
			exit( EXIT_FAILURE );
		}

		/* should be extended to support comma separated list of
		 *	[!]key[=value] parameters, e.g.  -E !foo,bar=567
		 */

		crit = 0;
		cvalue = NULL;
		if( optarg[0] == '!' ) {
			crit = 1;
			optarg++;
		}

		control = strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}
		fprintf( stderr, _("Invalid delete extension name: %s\n"), control );
		usage();
#endif

	case 'r':
		prune = 1;
		break;

	case 'z':	/* size limit */
		if ( strcasecmp( optarg, "none" ) == 0 ) {
			sizelimit = 0;

		} else if ( strcasecmp( optarg, "max" ) == 0 ) {
			sizelimit = LDAP_MAXINT;

		} else {
			ival = strtol( optarg, &next, 10 );
			if ( next == NULL || next[0] != '\0' ) {
				fprintf( stderr,
					_("Unable to parse size limit \"%s\"\n"), optarg );
				exit( EXIT_FAILURE );
			}
			sizelimit = ival;
		}
		if( sizelimit < 0 || sizelimit > LDAP_MAXINT ) {
			fprintf( stderr, _("%s: invalid sizelimit (%d) specified\n"),
				prog, sizelimit );
			exit( EXIT_FAILURE );
		}
		break;

	default:
		return 0;
	}
	return 1;
}


static void
private_conn_setup( LDAP *ld )
{
	/* this seems prudent for searches below */
	int deref = LDAP_DEREF_NEVER;
	ldap_set_option( ld, LDAP_OPT_DEREF, &deref );
}


int
main( int argc, char **argv )
{
	char		buf[ 4096 ];
	FILE		*fp = NULL;
	LDAP		*ld;
	int		rc, retval;

	tool_init( TOOL_DELETE );
    prog = lutil_progname( "ldapdelete", argc, argv );

	tool_args( argc, argv );

	if ( infile != NULL ) {
		if (( fp = fopen( infile, "r" )) == NULL ) {
			perror( optarg );
			exit( EXIT_FAILURE );
	    }
	} else {
		if ( optind >= argc ) {
			fp = stdin;
		}
	}

	ld = tool_conn_setup( 0, &private_conn_setup );

	tool_bind( ld );

	tool_server_controls( ld, NULL, 0 );

	retval = rc = 0;

	if ( fp == NULL ) {
		for ( ; optind < argc; ++optind ) {
			rc = dodelete( ld, argv[ optind ] );

			/* Stop on error and no -c option */
			if( rc != 0 ) {
				retval = rc;
				if( contoper == 0 ) break;
			}
		}
	} else {
		while ((rc == 0 || contoper) && fgets(buf, sizeof(buf), fp) != NULL) {
			buf[ strlen( buf ) - 1 ] = '\0'; /* remove trailing newline */

			if ( *buf != '\0' ) {
				rc = dodelete( ld, buf );
				if ( rc != 0 )
					retval = rc;
			}
		}
		if ( fp != stdin )
			fclose( fp );
	}

	tool_exit( ld, retval );
}


static int dodelete(
    LDAP	*ld,
    const char	*dn)
{
	int id;
	int	rc, code;
	char *matcheddn = NULL, *text = NULL, **refs = NULL;
	LDAPControl **ctrls = NULL;
	LDAPMessage *res;
	int subentries = 0;

	if ( verbose ) {
		printf( _("%sdeleting entry \"%s\"\n"),
			(dont ? "!" : ""), dn );
	}

	if ( dont ) {
		return LDAP_SUCCESS;
	}

	/* If prune is on, remove a whole subtree.  Delete the children of the
	 * DN recursively, then the DN requested.
	 */
	if ( prune ) {
retry:;
		deletechildren( ld, dn, subentries );
	}

	rc = ldap_delete_ext( ld, dn, NULL, NULL, &id );
	if ( rc != LDAP_SUCCESS ) {
		fprintf( stderr, "%s: ldap_delete_ext: %s (%d)\n",
			prog, ldap_err2string( rc ), rc );
		return rc;
	}

	for ( ; ; ) {
		struct timeval tv;

		if ( tool_check_abandon( ld, id ) ) {
			return LDAP_CANCELLED;
		}

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		rc = ldap_result( ld, LDAP_RES_ANY, LDAP_MSG_ALL, &tv, &res );
		if ( rc < 0 ) {
			tool_perror( "ldap_result", rc, NULL, NULL, NULL, NULL );
			return rc;
		}

		if ( rc != 0 ) {
			break;
		}
	}

	rc = ldap_parse_result( ld, res, &code, &matcheddn, &text, &refs, &ctrls, 1 );

	switch ( rc ) {
	case LDAP_SUCCESS:
		break;

	case LDAP_NOT_ALLOWED_ON_NONLEAF:
		if ( prune && !subentries ) {
			subentries = 1;
			goto retry;
		}
		/* fallthru */

	default:
		fprintf( stderr, "%s: ldap_parse_result: %s (%d)\n",
			prog, ldap_err2string( rc ), rc );
		return rc;
	}

	if( code != LDAP_SUCCESS ) {
		tool_perror( "ldap_delete", code, NULL, matcheddn, text, refs );
	} else if ( verbose && 
		((matcheddn && *matcheddn) || (text && *text) || (refs && *refs) ))
	{
		printf( _("Delete Result: %s (%d)\n"),
			ldap_err2string( code ), code );

		if( text && *text ) {
			printf( _("Additional info: %s\n"), text );
		}

		if( matcheddn && *matcheddn ) {
			printf( _("Matched DN: %s\n"), matcheddn );
		}

		if( refs ) {
			int i;
			for( i=0; refs[i]; i++ ) {
				printf(_("Referral: %s\n"), refs[i] );
			}
		}
	}

	if (ctrls) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	ber_memfree( text );
	ber_memfree( matcheddn );
	ber_memvfree( (void **) refs );

	return code;
}

/*
 * Delete all the children of an entry recursively until leaf nodes are reached.
 */
static int deletechildren(
	LDAP *ld,
	const char *base,
	int subentries )
{
	LDAPMessage *res, *e;
	int entries;
	int rc = LDAP_SUCCESS, srch_rc;
	static char *attrs[] = { LDAP_NO_ATTRS, NULL };
	LDAPControl c, *ctrls[2], **ctrlsp = NULL;
	BerElement *ber = NULL;

	if ( verbose ) printf ( _("deleting children of: %s\n"), base );

	if ( subentries ) {
		/*
		 * Do a one level search at base for subentry children.
		 */

		if ((ber = ber_alloc_t(LBER_USE_DER)) == NULL) {
			return EXIT_FAILURE;
		}
		rc = ber_printf( ber, "b", 1 );
		if ( rc == -1 ) {
			ber_free( ber, 1 );
			fprintf( stderr, _("Subentries control encoding error!\n"));
			return EXIT_FAILURE;
		}
		if ( ber_flatten2( ber, &c.ldctl_value, 0 ) == -1 ) {
			return EXIT_FAILURE;
		}
		c.ldctl_oid = LDAP_CONTROL_SUBENTRIES;
		c.ldctl_iscritical = 1;
		ctrls[0] = &c;
		ctrls[1] = NULL;
		ctrlsp = ctrls;
	}

	/*
	 * Do a one level search at base for children.  For each, delete its children.
	 */
more:;
	srch_rc = ldap_search_ext_s( ld, base, LDAP_SCOPE_ONELEVEL, NULL, attrs, 1,
		ctrlsp, NULL, NULL, sizelimit, &res );
	switch ( srch_rc ) {
	case LDAP_SUCCESS:
	case LDAP_SIZELIMIT_EXCEEDED:
		break;
	default:
		tool_perror( "ldap_search", srch_rc, NULL, NULL, NULL, NULL );
		return( srch_rc );
	}

	entries = ldap_count_entries( ld, res );

	if ( entries > 0 ) {
		int i;

		for (e = ldap_first_entry( ld, res ), i = 0; e != NULL;
			e = ldap_next_entry( ld, e ), i++ )
		{
			char *dn = ldap_get_dn( ld, e );

			if( dn == NULL ) {
				ldap_get_option( ld, LDAP_OPT_RESULT_CODE, &rc );
				tool_perror( "ldap_prune", rc, NULL, NULL, NULL, NULL );
				ber_memfree( dn );
				return rc;
			}

			rc = deletechildren( ld, dn, 0 );
			if ( rc != LDAP_SUCCESS ) {
				tool_perror( "ldap_prune", rc, NULL, NULL, NULL, NULL );
				ber_memfree( dn );
				return rc;
			}

			if ( verbose ) {
				printf( _("\tremoving %s\n"), dn );
			}

			rc = ldap_delete_ext_s( ld, dn, NULL, NULL );
			if ( rc != LDAP_SUCCESS ) {
				tool_perror( "ldap_delete", rc, NULL, NULL, NULL, NULL );
				ber_memfree( dn );
				return rc;

			}
			
			if ( verbose ) {
				printf( _("\t%s removed\n"), dn );
			}

			ber_memfree( dn );
		}
	}

	ldap_msgfree( res );

	if ( srch_rc == LDAP_SIZELIMIT_EXCEEDED ) {
		goto more;
	}

	return rc;
}
