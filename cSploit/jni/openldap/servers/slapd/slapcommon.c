/* slapcommon.c - common routine for the slap tools */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 2003 IBM Corporation.
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
 * This work was initially developed by Kurt Zeilenga for inclusion
 * in OpenLDAP Software.  Additional signficant contributors include
 *    Jong Hyuk Choi
 *    Hallvard B. Furuseth
 *    Howard Chu
 *    Pierangelo Masarati
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slapcommon.h"
#include "lutil.h"
#include "ldif.h"

tool_vars tool_globals;

#ifdef CSRIMALLOC
static char *leakfilename;
static FILE *leakfile;
#endif

static LDIFFP dummy;

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
int start_syslog;
static char **syslog_unknowns;
#ifdef LOG_LOCAL4
static int syslogUser = SLAP_DEFAULT_SYSLOG_USER;
#endif /* LOG_LOCAL4 */
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

static void
usage( int tool, const char *progname )
{
	char *options = NULL;
	fprintf( stderr,
		"usage: %s [-v] [-d debuglevel] [-f configfile] [-F configdir] [-o <name>[=<value>]]",
		progname );

	switch( tool ) {
	case SLAPACL:
		options = "\n\t[-U authcID | -D authcDN] [-X authzID | -o authzDN=<DN>]"
			"\n\t-b DN [-u] [attr[/access][:value]] [...]\n";
		break;

	case SLAPADD:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix]\n"
			"\t[-l ldiffile] [-j linenumber] [-q] [-u] [-s] [-w]\n";
		break;

	case SLAPAUTH:
		options = "\n\t[-U authcID] [-X authzID] [-R realm] [-M mech] ID [...]\n";
		break;

	case SLAPCAT:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix]"
			" [-l ldiffile] [-a filter] [-s subtree] [-H url]\n";
		break;

	case SLAPDN:
		options = "\n\t[-N | -P] DN [...]\n";
		break;

	case SLAPINDEX:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix] [attr ...] [-q] [-t]\n";
		break;

	case SLAPTEST:
		options = " [-n databasenumber] [-u] [-Q]\n";
		break;

	case SLAPSCHEMA:
		options = " [-c]\n\t[-g] [-n databasenumber | -b suffix]"
			" [-l errorfile] [-a filter] [-s subtree] [-H url]\n";
		break;
	}

	if ( options != NULL ) {
		fputs( options, stderr );
	}
	exit( EXIT_FAILURE );
}

static int
parse_slapopt( int tool, int *mode )
{
	size_t	len = 0;
	char	*p;

	p = strchr( optarg, '=' );
	if ( p != NULL ) {
		len = p - optarg;
		p++;
	}

	if ( strncasecmp( optarg, "sockurl", len ) == 0 ) {
		if ( !BER_BVISNULL( &listener_url ) ) {
			ber_memfree( listener_url.bv_val );
		}
		ber_str2bv( p, 0, 1, &listener_url );

	} else if ( strncasecmp( optarg, "domain", len ) == 0 ) {
		if ( !BER_BVISNULL( &peer_domain ) ) {
			ber_memfree( peer_domain.bv_val );
		}
		ber_str2bv( p, 0, 1, &peer_domain );

	} else if ( strncasecmp( optarg, "peername", len ) == 0 ) {
		if ( !BER_BVISNULL( &peer_name ) ) {
			ber_memfree( peer_name.bv_val );
		}
		ber_str2bv( p, 0, 1, &peer_name );

	} else if ( strncasecmp( optarg, "sockname", len ) == 0 ) {
		if ( !BER_BVISNULL( &sock_name ) ) {
			ber_memfree( sock_name.bv_val );
		}
		ber_str2bv( p, 0, 1, &sock_name );

	} else if ( strncasecmp( optarg, "ssf", len ) == 0 ) {
		if ( lutil_atou( &ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "transport_ssf", len ) == 0 ) {
		if ( lutil_atou( &transport_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse transport_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "tls_ssf", len ) == 0 ) {
		if ( lutil_atou( &tls_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse tls_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "sasl_ssf", len ) == 0 ) {
		if ( lutil_atou( &sasl_ssf, p ) ) {
			Debug( LDAP_DEBUG_ANY, "unable to parse sasl_ssf=\"%s\".\n", p, 0, 0 );
			return -1;
		}

	} else if ( strncasecmp( optarg, "authzDN", len ) == 0 ) {
		ber_str2bv( p, 0, 1, &authzDN );

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	} else if ( strncasecmp( optarg, "syslog", len ) == 0 ) {
		if ( parse_debug_level( p, &ldap_syslog, &syslog_unknowns ) ) {
			return -1;
		}
		start_syslog = 1;

	} else if ( strncasecmp( optarg, "syslog-level", len ) == 0 ) {
		if ( parse_syslog_level( p, &ldap_syslog_level ) ) {
			return -1;
		}
		start_syslog = 1;

#ifdef LOG_LOCAL4
	} else if ( strncasecmp( optarg, "syslog-user", len ) == 0 ) {
		if ( parse_syslog_user( p, &syslogUser ) ) {
			return -1;
		}
		start_syslog = 1;
#endif /* LOG_LOCAL4 */
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

	} else if ( strncasecmp( optarg, "schema-check", len ) == 0 ) {
		switch ( tool ) {
		case SLAPADD:
			if ( strcasecmp( p, "yes" ) == 0 ) {
				*mode &= ~SLAP_TOOL_NO_SCHEMA_CHECK;
			} else if ( strcasecmp( p, "no" ) == 0 ) {
				*mode |= SLAP_TOOL_NO_SCHEMA_CHECK;
			} else {
				Debug( LDAP_DEBUG_ANY, "unable to parse schema-check=\"%s\".\n", p, 0, 0 );
				return -1;
			}
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "schema-check meaningless for tool.\n", 0, 0, 0 );
			break;
		}

	} else if ( strncasecmp( optarg, "value-check", len ) == 0 ) {
		switch ( tool ) {
		case SLAPADD:
			if ( strcasecmp( p, "yes" ) == 0 ) {
				*mode |= SLAP_TOOL_VALUE_CHECK;
			} else if ( strcasecmp( p, "no" ) == 0 ) {
				*mode &= ~SLAP_TOOL_VALUE_CHECK;
			} else {
				Debug( LDAP_DEBUG_ANY, "unable to parse value-check=\"%s\".\n", p, 0, 0 );
				return -1;
			}
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "value-check meaningless for tool.\n", 0, 0, 0 );
			break;
		}

	} else if ( strncasecmp( optarg, "ldif-wrap", len ) == 0 ) {
		switch ( tool ) {
		case SLAPCAT:
			if ( strcasecmp( p, "no" ) == 0 ) {
				ldif_wrap = LDIF_LINE_WIDTH_MAX;

			} else {
				unsigned int u;
				if ( lutil_atou( &u, p ) ) {
					Debug( LDAP_DEBUG_ANY, "unable to parse ldif-wrap=\"%s\".\n", p, 0, 0 );
					return -1;
				}
				ldif_wrap = (ber_len_t)u;
			}
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "value-check meaningless for tool.\n", 0, 0, 0 );
			break;
		}

	} else {
		return -1;
	}

	return 0;
}

/*
 * slap_tool_init - initialize slap utility, handle program options.
 * arguments:
 *	name		program name
 *	tool		tool code
 *	argc, argv	command line arguments
 */

static int need_shutdown;

void
slap_tool_init(
	const char* progname,
	int tool,
	int argc, char **argv )
{
	char *options;
	char *conffile = NULL;
	char *confdir = NULL;
	struct berval base = BER_BVNULL;
	char *filterstr = NULL;
	char *subtree = NULL;
	char *ldiffile	= NULL;
	char **debug_unknowns = NULL;
	int rc, i;
	int mode = SLAP_TOOL_MODE;
	int truncatemode = 0;
	int use_glue = 1;
	int writer;

#ifdef LDAP_DEBUG
	/* tools default to "none", so that at least LDAP_DEBUG_ANY 
	 * messages show up; use -d 0 to reset */
	slap_debug = LDAP_DEBUG_NONE;
	ldif_debug = slap_debug;
#endif
	ldap_syslog = 0;

#ifdef CSRIMALLOC
	leakfilename = malloc( strlen( progname ) + STRLENOF( ".leak" ) + 1 );
	sprintf( leakfilename, "%s.leak", progname );
	if( ( leakfile = fopen( leakfilename, "w" )) == NULL ) {
		leakfile = stderr;
	}
	free( leakfilename );
	leakfilename = NULL;
#endif

	ldif_wrap = LDIF_LINE_WIDTH;

	scope = LDAP_SCOPE_DEFAULT;

	switch( tool ) {
	case SLAPADD:
		options = "b:cd:f:F:gj:l:n:o:qsS:uvw";
		break;

	case SLAPCAT:
		options = "a:b:cd:f:F:gH:l:n:o:s:v";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPDN:
		options = "d:f:F:No:Pv";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPMODIFY:
		options = "b:cd:f:F:gj:l:n:o:qsS:uvw";
		break;

	case SLAPSCHEMA:
		options = "a:b:cd:f:F:gH:l:n:o:s:v";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPTEST:
		options = "d:f:F:n:o:Quv";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPAUTH:
		options = "d:f:F:M:o:R:U:vX:";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	case SLAPINDEX:
		options = "b:cd:f:F:gn:o:qtv";
		mode |= SLAP_TOOL_READMAIN;
		break;

	case SLAPACL:
		options = "b:D:d:f:F:o:uU:vX:";
		mode |= SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY;
		break;

	default:
		fprintf( stderr, "%s: unknown tool mode (%d)\n", progname, tool );
		exit( EXIT_FAILURE );
	}

	dbnum = -1;
	while ( (i = getopt( argc, argv, options )) != EOF ) {
		switch ( i ) {
		case 'a':
			filterstr = ch_strdup( optarg );
			break;

		case 'b':
			ber_str2bv( optarg, 0, 1, &base );
			break;

		case 'c':	/* enable continue mode */
			continuemode++;
			break;

		case 'd': {	/* turn on debugging */
			int	level = 0;

			if ( parse_debug_level( optarg, &level, &debug_unknowns ) ) {
				usage( tool, progname );
			}
#ifdef LDAP_DEBUG
			if ( level == 0 ) {
				/* allow to reset log level */
				slap_debug = 0;

			} else {
				slap_debug |= level;
			}
#else
			if ( level != 0 )
				fputs( "must compile with LDAP_DEBUG for debugging\n",
				       stderr );
#endif
			} break;

		case 'D':
			ber_str2bv( optarg, 0, 1, &authcDN );
			break;

		case 'f':	/* specify a conf file */
			conffile = ch_strdup( optarg );
			break;

		case 'F':	/* specify a conf dir */
			confdir = ch_strdup( optarg );
			break;

		case 'g':	/* disable subordinate glue */
			use_glue = 0;
			break;

		case 'H': {
			LDAPURLDesc *ludp;
			int rc;

			rc = ldap_url_parse_ext( optarg, &ludp,
				LDAP_PVT_URL_PARSE_NOEMPTY_HOST | LDAP_PVT_URL_PARSE_NOEMPTY_DN );
			if ( rc != LDAP_URL_SUCCESS ) {
				usage( tool, progname );
			}

			/* don't accept host, port, attrs, extensions */
			if ( ldap_pvt_url_scheme2proto( ludp->lud_scheme ) != LDAP_PROTO_TCP ) {
				usage( tool, progname );
			}

			if ( ludp->lud_host != NULL ) {
				usage( tool, progname );
			}

			if ( ludp->lud_port != 0 ) {
				usage( tool, progname );
			}

			if ( ludp->lud_attrs != NULL ) {
				usage( tool, progname );
			}

			if ( ludp->lud_exts != NULL ) {
				usage( tool, progname );
			}

			if ( ludp->lud_dn != NULL && ludp->lud_dn[0] != '\0' ) {
				subtree = ludp->lud_dn;
				ludp->lud_dn = NULL;
			}

			if ( ludp->lud_filter != NULL && ludp->lud_filter[0] != '\0' ) {
				filterstr = ludp->lud_filter;
				ludp->lud_filter = NULL;
			}

			scope = ludp->lud_scope;

			ldap_free_urldesc( ludp );
			} break;

		case 'j':	/* jump to linenumber */
			if ( lutil_atoul( &jumpline, optarg ) ) {
				usage( tool, progname );
			}
			break;

		case 'l':	/* LDIF file */
			ldiffile = ch_strdup( optarg );
			break;

		case 'M':
			ber_str2bv( optarg, 0, 0, &mech );
			break;

		case 'N':
			if ( dn_mode && dn_mode != SLAP_TOOL_LDAPDN_NORMAL ) {
				usage( tool, progname );
			}
			dn_mode = SLAP_TOOL_LDAPDN_NORMAL;
			break;

		case 'n':	/* which config file db to index */
			if ( lutil_atoi( &dbnum, optarg ) || dbnum < 0 ) {
				usage( tool, progname );
			}
			break;

		case 'o':
			if ( parse_slapopt( tool, &mode ) ) {
				usage( tool, progname );
			}
			break;

		case 'P':
			if ( dn_mode && dn_mode != SLAP_TOOL_LDAPDN_PRETTY ) {
				usage( tool, progname );
			}
			dn_mode = SLAP_TOOL_LDAPDN_PRETTY;
			break;

		case 'Q':
			quiet++;
			slap_debug = 0;
			break;

		case 'q':	/* turn on quick */
			mode |= SLAP_TOOL_QUICK;
			break;

		case 'R':
			realm = optarg;
			break;

		case 'S':
			if ( lutil_atou( &csnsid, optarg )
				|| csnsid > SLAP_SYNC_SID_MAX )
			{
				usage( tool, progname );
			}
			break;

		case 's':
			switch ( tool ) {
			case SLAPADD:
			case SLAPMODIFY:
				/* no schema check */
				mode |= SLAP_TOOL_NO_SCHEMA_CHECK;
				break;

			case SLAPCAT:
			case SLAPSCHEMA:
				/* dump subtree */
				subtree = ch_strdup( optarg );
				break;
			}
			break;

		case 't':	/* turn on truncate */
			truncatemode++;
			mode |= SLAP_TRUNCATE_MODE;
			break;

		case 'U':
			ber_str2bv( optarg, 0, 0, &authcID );
			break;

		case 'u':	/* dry run */
			dryrun++;
			break;

		case 'v':	/* turn on verbose */
			verbose++;
			break;

		case 'w':	/* write context csn at the end */
			update_ctxcsn++;
			break;

		case 'X':
			ber_str2bv( optarg, 0, 0, &authzID );
			break;

		default:
			usage( tool, progname );
			break;
		}
	}

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	if ( start_syslog ) {
		char *logName;
#ifdef HAVE_EBCDIC
		logName = ch_strdup( progname );
		__atoe( logName );
#else
		logName = (char *)progname;
#endif

#ifdef LOG_LOCAL4
		openlog( logName, OPENLOG_OPTIONS, syslogUser );
#elif defined LOG_DEBUG
		openlog( logName, OPENLOG_OPTIONS );
#endif
#ifdef HAVE_EBCDIC
		free( logName );
		logName = NULL;
#endif
	}
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

	switch ( tool ) {
	case SLAPCAT:
	case SLAPSCHEMA:
		writer = 1;
		break;

	default:
		writer = 0;
		break;
	}

	switch ( tool ) {
	case SLAPADD:
	case SLAPCAT:
	case SLAPMODIFY:
	case SLAPSCHEMA:
		if ( ( argc != optind ) || (dbnum >= 0 && base.bv_val != NULL ) ) {
			usage( tool, progname );
		}

		break;

	case SLAPINDEX:
		if ( dbnum >= 0 && base.bv_val != NULL ) {
			usage( tool, progname );
		}

		break;

	case SLAPDN:
		if ( argc == optind ) {
			usage( tool, progname );
		}
		break;

	case SLAPAUTH:
		if ( argc == optind && BER_BVISNULL( &authcID ) ) {
			usage( tool, progname );
		}
		break;

	case SLAPTEST:
		if ( argc != optind ) {
			usage( tool, progname );
		}
		break;

	case SLAPACL:
		if ( !BER_BVISNULL( &authcDN ) && !BER_BVISNULL( &authcID ) ) {
			usage( tool, progname );
		}
		if ( BER_BVISNULL( &base ) ) {
			usage( tool, progname );
		}
		ber_dupbv( &baseDN, &base );
		break;

	default:
		break;
	}

	if ( ldiffile == NULL ) {
		dummy.fp = writer ? stdout : stdin;
		ldiffp = &dummy;

	} else if ((ldiffp = ldif_open( ldiffile, writer ? "w" : "r" ))
		== NULL )
	{
		perror( ldiffile );
		exit( EXIT_FAILURE );
	}

	/*
	 * initialize stuff and figure out which backend we're dealing with
	 */

	rc = slap_init( mode, progname );
	if ( rc != 0 ) {
		fprintf( stderr, "%s: slap_init failed!\n", progname );
		exit( EXIT_FAILURE );
	}

	rc = read_config( conffile, confdir );

	if ( rc != 0 ) {
		fprintf( stderr, "%s: bad configuration %s!\n",
			progname, confdir ? "directory" : "file" );
		exit( EXIT_FAILURE );
	}

	if ( debug_unknowns ) {
		rc = parse_debug_unknowns( debug_unknowns, &slap_debug );
		ldap_charray_free( debug_unknowns );
		debug_unknowns = NULL;
		if ( rc )
			exit( EXIT_FAILURE );
	}

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
	if ( syslog_unknowns ) {
		rc = parse_debug_unknowns( syslog_unknowns, &ldap_syslog );
		ldap_charray_free( syslog_unknowns );
		syslog_unknowns = NULL;
		if ( rc )
			exit( EXIT_FAILURE );
	}
#endif

	at_oc_cache = 1;

	switch ( tool ) {
	case SLAPADD:
	case SLAPCAT:
	case SLAPINDEX:
	case SLAPMODIFY:
	case SLAPSCHEMA:
		if ( !nbackends ) {
			fprintf( stderr, "No databases found "
					"in config file\n" );
			exit( EXIT_FAILURE );
		}
		break;

	default:
		break;
	}

	if ( use_glue ) {
		rc = glue_sub_attach( 0 );

		if ( rc != 0 ) {
			fprintf( stderr,
				"%s: subordinate configuration error\n", progname );
			exit( EXIT_FAILURE );
		}
	}

	rc = slap_schema_check();

	if ( rc != 0 ) {
		fprintf( stderr, "%s: slap_schema_prep failed!\n", progname );
		exit( EXIT_FAILURE );
	}

	switch ( tool ) {
	case SLAPTEST:
		if ( dbnum >= 0 )
			goto get_db;
		/* FALLTHRU */
	case SLAPDN:
	case SLAPAUTH:
		be = NULL;
		goto startup;

	default:
		break;
	}

	if( filterstr ) {
		filter = str2filter( filterstr );

		if( filter == NULL ) {
			fprintf( stderr, "Invalid filter '%s'\n", filterstr );
			exit( EXIT_FAILURE );
		}

		ch_free( filterstr );
		filterstr = NULL;
	}

	if( subtree ) {
		struct berval val;
		ber_str2bv( subtree, 0, 0, &val );
		rc = dnNormalize( 0, NULL, NULL, &val, &sub_ndn, NULL );
		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "Invalid subtree DN '%s'\n", subtree );
			exit( EXIT_FAILURE );
		}

		if ( BER_BVISNULL( &base ) && dbnum == -1 ) {
			base = val;
		} else {
			free( subtree );
			subtree = NULL;
		}
	}

	if( base.bv_val != NULL ) {
		struct berval nbase;

		rc = dnNormalize( 0, NULL, NULL, &base, &nbase, NULL );
		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: slap_init invalid suffix (\"%s\")\n",
				progname, base.bv_val );
			exit( EXIT_FAILURE );
		}

		be = select_backend( &nbase, 0 );
		ber_memfree( nbase.bv_val );
		BER_BVZERO( &nbase );

		if( be == NULL ) {
			fprintf( stderr, "%s: slap_init no backend for \"%s\"\n",
				progname, base.bv_val );
			exit( EXIT_FAILURE );
		}
		switch ( tool ) {
		case SLAPACL:
			goto startup;

		default:
			break;
		}

		/* If the named base is a glue master, operate on the
		 * entire context
		 */
		if ( SLAP_GLUE_INSTANCE( be ) ) {
			nosubordinates = 1;
		}

		ch_free( base.bv_val );
		BER_BVZERO( &base );

	} else if ( dbnum == -1 ) {
		/* no suffix and no dbnum specified, just default to
		 * the first available database
		 */
		if ( nbackends <= 0 ) {
			fprintf( stderr, "No available databases\n" );
			exit( EXIT_FAILURE );
		}
		LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
			dbnum++;

			/* db #0 is cn=config, don't select it as a default */
			if ( dbnum < 1 ) continue;
		
		 	if ( SLAP_MONITOR(be))
				continue;

		/* If just doing the first by default and it is a
		 * glue subordinate, find the master.
		 */
			if ( SLAP_GLUE_SUBORDINATE(be) ) {
				nosubordinates = 1;
				continue;
			}
			break;
		}

		if ( !be ) {
			fprintf( stderr, "Available database(s) "
					"do not allow %s\n", progname );
			exit( EXIT_FAILURE );
		}
		
		if ( nosubordinates == 0 && dbnum > 1 ) {
			Debug( LDAP_DEBUG_ANY,
				"The first database does not allow %s;"
				" using the first available one (%d)\n",
				progname, dbnum, 0 );
		}

	} else if ( dbnum >= nbackends ) {
		fprintf( stderr,
			"Database number selected via -n is out of range\n"
			"Must be in the range 0 to %d"
			" (the number of configured databases)\n",
			nbackends - 1 );
		exit( EXIT_FAILURE );

	} else {
get_db:
		LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
			if ( dbnum == 0 ) break;
			dbnum--;
		}
	}

	if ( scope != LDAP_SCOPE_DEFAULT && BER_BVISNULL( &sub_ndn ) ) {
		if ( be && be->be_nsuffix ) {
			ber_dupbv( &sub_ndn, be->be_nsuffix );

		} else {
			fprintf( stderr,
				"<scope> needs a DN or a valid database\n" );
			exit( EXIT_FAILURE );
		}
	}

startup:;
	if ( be ) {
		BackendDB *bdtmp;

		dbnum = 0;
		LDAP_STAILQ_FOREACH( bdtmp, &backendDB, be_next ) {
			if ( bdtmp == be ) break;
			dbnum++;
		}
	}

#ifdef CSRIMALLOC
	mal_leaktrace(1);
#endif

	if ( conffile != NULL ) {
		ch_free( conffile );
		conffile = NULL;
	}

	if ( confdir != NULL ) {
		ch_free( confdir );
		confdir = NULL;
	}

	if ( ldiffile != NULL ) {
		ch_free( ldiffile );
		ldiffile = NULL;
	}

	/* slapdn doesn't specify a backend to startup */
	if ( !dryrun && tool != SLAPDN ) {
		need_shutdown = 1;

		if ( slap_startup( be ) ) {
			switch ( tool ) {
			case SLAPTEST:
				fprintf( stderr, "slap_startup failed "
						"(test would succeed using "
						"the -u switch)\n" );
				break;

			default:
				fprintf( stderr, "slap_startup failed\n" );
				break;
			}

			exit( EXIT_FAILURE );
		}
	}
}

int slap_tool_destroy( void )
{
	int rc = 0;
	if ( !dryrun ) {
		if ( need_shutdown ) {
			if ( slap_shutdown( be ))
				rc = EXIT_FAILURE;
		}
		if ( slap_destroy())
			rc = EXIT_FAILURE;
	}
#ifdef SLAPD_MODULES
	if ( slapMode == SLAP_SERVER_MODE ) {
	/* always false. just pulls in necessary symbol references. */
		lutil_uuidstr(NULL, 0);
	}
	module_kill();
#endif
	schema_destroy();
#ifdef HAVE_TLS
	ldap_pvt_tls_destroy();
#endif
	config_destroy();

#ifdef CSRIMALLOC
	mal_dumpleaktrace( leakfile );
#endif

	if ( !BER_BVISNULL( &authcDN ) ) {
		ch_free( authcDN.bv_val );
		BER_BVZERO( &authcDN );
	}

	if ( ldiffp && ldiffp != &dummy ) {
		ldif_close( ldiffp );
	}
	return rc;
}

int
slap_tool_update_ctxcsn(
	const char *progname,
	unsigned long sid,
	struct berval *bvtext )
{
	struct berval ctxdn;
	ID ctxcsn_id;
	Entry *ctxcsn_e;
	int rc = EXIT_SUCCESS;

	if ( !(update_ctxcsn && !dryrun && sid != SLAP_SYNC_SID_MAX + 1) ) {
		return rc;
	}

	if ( SLAP_SYNC_SUBENTRY( be )) {
		build_new_dn( &ctxdn, &be->be_nsuffix[0],
			(struct berval *)&slap_ldapsync_cn_bv, NULL );
	} else {
		ctxdn = be->be_nsuffix[0];
	}
	ctxcsn_id = be->be_dn2id_get( be, &ctxdn );
	if ( ctxcsn_id == NOID ) {
		if ( SLAP_SYNC_SUBENTRY( be )) {
			ctxcsn_e = slap_create_context_csn_entry( be, NULL );
			for ( sid = 0; sid <= SLAP_SYNC_SID_MAX; sid++ ) {
				if ( maxcsn[ sid ].bv_len ) {
					attr_merge_one( ctxcsn_e, slap_schema.si_ad_contextCSN,
						&maxcsn[ sid ], NULL );
				}
			}
			ctxcsn_id = be->be_entry_put( be, ctxcsn_e, bvtext );
			if ( ctxcsn_id == NOID ) {
				fprintf( stderr, "%s: couldn't create context entry\n", progname );
				rc = EXIT_FAILURE;
			}
		} else {
			fprintf( stderr, "%s: context entry is missing\n", progname );
			rc = EXIT_FAILURE;
		}
	} else {
		ctxcsn_e = be->be_entry_get( be, ctxcsn_id );
		if ( ctxcsn_e != NULL ) {
			Entry *e = entry_dup( ctxcsn_e );
			int change;
			Attribute *attr = attr_find( e->e_attrs, slap_schema.si_ad_contextCSN );
			if ( attr ) {
				int		i;

				change = 0;

				for ( i = 0; !BER_BVISNULL( &attr->a_nvals[ i ] ); i++ ) {
					int rc_sid;
					int match;
					const char *text = NULL;

					rc_sid = slap_parse_csn_sid( &attr->a_nvals[ i ] );
					if ( rc_sid < 0 ) {
						Debug( LDAP_DEBUG_ANY,
							"%s: unable to extract SID "
							"from #%d contextCSN=%s\n",
							progname, i,
							attr->a_nvals[ i ].bv_val );
						continue;
					}

					assert( rc_sid <= SLAP_SYNC_SID_MAX );

					sid = (unsigned)rc_sid;

					if ( maxcsn[ sid ].bv_len == 0 ) {
						match = -1;

					} else {
						value_match( &match, slap_schema.si_ad_entryCSN,
							slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							&maxcsn[ sid ], &attr->a_nvals[i], &text );
					}

					if ( match > 0 ) {
						change = 1;
					} else {
						AC_MEMCPY( maxcsn[ sid ].bv_val,
							attr->a_nvals[ i ].bv_val,
							attr->a_nvals[ i ].bv_len );
						maxcsn[ sid ].bv_val[ attr->a_nvals[ i ].bv_len ] = '\0';
						maxcsn[ sid ].bv_len = attr->a_nvals[ i ].bv_len;
					}
				}

				if ( change ) {
					if ( attr->a_nvals != attr->a_vals ) {
						ber_bvarray_free( attr->a_nvals );
					}
					attr->a_nvals = NULL;
					ber_bvarray_free( attr->a_vals );
					attr->a_vals = NULL;
					attr->a_numvals = 0;
				}
			} else {
				change = 1;
			}

			if ( change ) {
				for ( sid = 0; sid <= SLAP_SYNC_SID_MAX; sid++ ) {
					if ( maxcsn[ sid ].bv_len ) {
						attr_merge_one( e, slap_schema.si_ad_contextCSN,
							&maxcsn[ sid], NULL );
					}
				}

				ctxcsn_id = be->be_entry_modify( be, e, bvtext );
				if( ctxcsn_id == NOID ) {
					fprintf( stderr, "%s: could not modify ctxcsn (%s)\n",
						progname, bvtext->bv_val ? bvtext->bv_val : "" );
					rc = EXIT_FAILURE;
				} else if ( verbose ) {
					fprintf( stderr, "modified: \"%s\" (%08lx)\n",
						e->e_dn, (long) ctxcsn_id );
				}
			}
			entry_free( e );
		}
	} 

	return rc;
}

/*
 * return value:
 *	-1:			update_ctxcsn == 0
 *	SLAP_SYNC_SID_MAX + 1:	unable to extract SID
 *	0 <= SLAP_SYNC_SID_MAX:	the SID
 */
unsigned long
slap_tool_update_ctxcsn_check(
	const char *progname,
	Entry *e )
{
	if ( update_ctxcsn ) {
		unsigned long sid = SLAP_SYNC_SID_MAX + 1;
		int rc_sid;
		Attribute *attr;

		attr = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
		assert( attr != NULL );

		rc_sid = slap_parse_csn_sid( &attr->a_nvals[ 0 ] );
		if ( rc_sid < 0 ) {
			Debug( LDAP_DEBUG_ANY, "%s: could not "
				"extract SID from entryCSN=%s, entry dn=\"%s\"\n",
				progname, attr->a_nvals[ 0 ].bv_val, e->e_name.bv_val );
			return (unsigned long)(-1);

		} else {
			int match;
			const char *text = NULL;

			assert( rc_sid <= SLAP_SYNC_SID_MAX );

			sid = (unsigned)rc_sid;
			if ( maxcsn[ sid ].bv_len != 0 ) {
				match = 0;
				value_match( &match, slap_schema.si_ad_entryCSN,
					slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					&maxcsn[ sid ], &attr->a_nvals[0], &text );
			} else {
				match = -1;
			}
			if ( match < 0 ) {
				strcpy( maxcsn[ sid ].bv_val, attr->a_nvals[0].bv_val );
				maxcsn[ sid ].bv_len = attr->a_nvals[0].bv_len;
			}
		}
	}

	return (unsigned long)(-1);
}

int
slap_tool_update_ctxcsn_init(void)
{
	if ( update_ctxcsn ) {
		unsigned long sid;
		maxcsn[ 0 ].bv_val = maxcsnbuf;
		for ( sid = 1; sid <= SLAP_SYNC_SID_MAX; sid++ ) {
			maxcsn[ sid ].bv_val = maxcsn[ sid - 1 ].bv_val + LDAP_PVT_CSNSTR_BUFSIZE;
			maxcsn[ sid ].bv_len = 0;
		}
	}

	return 0;
}

int
slap_tool_entry_check(
	const char *progname,
	Operation *op,
	Entry *e,
	int lineno,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	/* NOTE: we may want to conditionally enable manage */
	int manage = 0;

	Attribute *oc = attr_find( e->e_attrs,
		slap_schema.si_ad_objectClass );

	if( oc == NULL ) {
		fprintf( stderr, "%s: dn=\"%s\" (line=%d): %s\n",
			progname, e->e_dn, lineno,
			"no objectClass attribute");
		return LDAP_NO_SUCH_ATTRIBUTE;
	}

	/* check schema */
	op->o_bd = be;

	if ( (slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) == 0) {
		int rc = entry_schema_check( op, e, NULL, manage, 1, NULL,
			text, textbuf, textlen );

		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
				progname, e->e_dn, lineno, rc, *text );
			return rc;
		}
		textbuf[ 0 ] = '\0';
	}

	if ( (slapMode & SLAP_TOOL_VALUE_CHECK) != 0) {
		Modifications *ml = NULL;

		int rc = slap_entry2mods( e, &ml, text, textbuf, textlen );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
				progname, e->e_dn, lineno, rc, *text );
			return rc;
		}
		textbuf[ 0 ] = '\0';

		rc = slap_mods_check( op, ml, text, textbuf, textlen, NULL );
		slap_mods_free( ml, 1 );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
				progname, e->e_dn, lineno, rc, *text );
			return rc;
		}
		textbuf[ 0 ] = '\0';
	}

	return LDAP_SUCCESS;
}

