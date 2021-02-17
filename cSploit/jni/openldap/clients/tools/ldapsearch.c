/* ldapsearch -- a tool for searching LDAP directories */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 1998-2001 Net Boolean Incorporated.
 * Portions Copyright 2001-2003 IBM Corporation.
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
 *   Jong Hyuk Choi
 *   Lynn Moss
 *   Mikhail Sahalaev
 *   Kurt D. Zeilenga
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/errno.h>
#include <ac/time.h>

#include <sys/stat.h>

#include <ac/signal.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <ldap.h>

#include "ldif.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"
#include "ldap_pvt.h"

#include "common.h"

#if !LDAP_DEPRECATED
/*
 * NOTE: we use this deprecated function only because
 * we want ldapsearch to provide some client-side sorting 
 * capability.
 */
/* from ldap.h */
typedef int (LDAP_SORT_AD_CMP_PROC) LDAP_P(( /* deprecated */
	LDAP_CONST char *left,
	LDAP_CONST char *right ));

LDAP_F( int )	/* deprecated */
ldap_sort_entries LDAP_P(( LDAP *ld,
	LDAPMessage **chain,
	LDAP_CONST char *attr,
	LDAP_SORT_AD_CMP_PROC *cmp ));
#endif

static int scope = LDAP_SCOPE_SUBTREE;
static int deref = -1;
static int attrsonly;
static int timelimit = -1;
static int sizelimit = -1;

static char *control;

static char *def_tmpdir;
static char *def_urlpre;

#if defined(__CYGWIN__) || defined(__MINGW32__)
/* Turn off commandline globbing, otherwise you cannot search for
 * attribute '*'
 */
int _CRT_glob = 0;
#endif

void
usage( void )
{
	fprintf( stderr, _("usage: %s [options] [filter [attributes...]]\nwhere:\n"), prog);
	fprintf( stderr, _("  filter\tRFC 4515 compliant LDAP search filter\n"));
	fprintf( stderr, _("  attributes\twhitespace-separated list of attribute descriptions\n"));
	fprintf( stderr, _("    which may include:\n"));
	fprintf( stderr, _("      1.1   no attributes\n"));
	fprintf( stderr, _("      *     all user attributes\n"));
	fprintf( stderr, _("      +     all operational attributes\n"));


	fprintf( stderr, _("Search options:\n"));
	fprintf( stderr, _("  -a deref   one of never (default), always, search, or find\n"));
	fprintf( stderr, _("  -A         retrieve attribute names only (no values)\n"));
	fprintf( stderr, _("  -b basedn  base dn for search\n"));
	fprintf( stderr, _("  -c         continuous operation mode (do not stop on errors)\n"));
	fprintf( stderr, _("  -E [!]<ext>[=<extparam>] search extensions (! indicates criticality)\n"));
	fprintf( stderr, _("             [!]domainScope              (domain scope)\n"));
	fprintf( stderr, _("             !dontUseCopy                (Don't Use Copy)\n"));
	fprintf( stderr, _("             [!]mv=<filter>              (RFC 3876 matched values filter)\n"));
	fprintf( stderr, _("             [!]pr=<size>[/prompt|noprompt] (RFC 2696 paged results/prompt)\n"));
	fprintf( stderr, _("             [!]sss=[-]<attr[:OID]>[/[-]<attr[:OID]>...]\n"));
	fprintf( stderr, _("                                         (RFC 2891 server side sorting)\n"));
	fprintf( stderr, _("             [!]subentries[=true|false]  (RFC 3672 subentries)\n"));
	fprintf( stderr, _("             [!]sync=ro[/<cookie>]       (RFC 4533 LDAP Sync refreshOnly)\n"));
	fprintf( stderr, _("                     rp[/<cookie>][/<slimit>] (refreshAndPersist)\n"));
	fprintf( stderr, _("             [!]vlv=<before>/<after>(/<offset>/<count>|:<value>)\n"));
	fprintf( stderr, _("                                         (ldapv3-vlv-09 virtual list views)\n"));
#ifdef LDAP_CONTROL_X_DEREF
	fprintf( stderr, _("             [!]deref=derefAttr:attr[,...][;derefAttr:attr[,...][;...]]\n"));
#endif
	fprintf( stderr, _("             [!]<oid>[=:<b64value>] (generic control; no response handling)\n"));
	fprintf( stderr, _("  -f file    read operations from `file'\n"));
	fprintf( stderr, _("  -F prefix  URL prefix for files (default: %s)\n"), def_urlpre);
	fprintf( stderr, _("  -l limit   time limit (in seconds, or \"none\" or \"max\") for search\n"));
	fprintf( stderr, _("  -L         print responses in LDIFv1 format\n"));
	fprintf( stderr, _("  -LL        print responses in LDIF format without comments\n"));
	fprintf( stderr, _("  -LLL       print responses in LDIF format without comments\n"));
	fprintf( stderr, _("             and version\n"));
	fprintf( stderr, _("  -M         enable Manage DSA IT control (-MM to make critical)\n"));
	fprintf( stderr, _("  -P version protocol version (default: 3)\n"));
	fprintf( stderr, _("  -s scope   one of base, one, sub or children (search scope)\n"));
	fprintf( stderr, _("  -S attr    sort the results by attribute `attr'\n"));
	fprintf( stderr, _("  -t         write binary values to files in temporary directory\n"));
	fprintf( stderr, _("  -tt        write all values to files in temporary directory\n"));
	fprintf( stderr, _("  -T path    write files to directory specified by path (default: %s)\n"), def_tmpdir);
	fprintf( stderr, _("  -u         include User Friendly entry names in the output\n"));
	fprintf( stderr, _("  -z limit   size limit (in entries, or \"none\" or \"max\") for search\n"));
	tool_common_usage();
	exit( EXIT_FAILURE );
}

static void print_entry LDAP_P((
	LDAP	*ld,
	LDAPMessage	*entry,
	int		attrsonly));

static void print_reference(
	LDAP *ld,
	LDAPMessage *reference );

static void print_extended(
	LDAP *ld,
	LDAPMessage *extended );

static void print_partial(
	LDAP *ld,
	LDAPMessage *partial );

static int print_result(
	LDAP *ld,
	LDAPMessage *result,
	int search );

static int dosearch LDAP_P((
	LDAP	*ld,
	char	*base,
	int		scope,
	char	*filtpatt,
	char	*value,
	char	**attrs,
	int		attrsonly,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	struct timeval *timeout,
	int	sizelimit ));

static char *tmpdir = NULL;
static char *urlpre = NULL;
static char	*base = NULL;
static char	*sortattr = NULL;
static int  includeufn, vals2tmp = 0;

static int subentries = 0, valuesReturnFilter = 0;
static char	*vrFilter = NULL;

#ifdef LDAP_CONTROL_DONTUSECOPY
static int dontUseCopy = 0;
#endif

static int domainScope = 0;

static int sss = 0;
static LDAPSortKey **sss_keys = NULL;

static int vlv = 0;
static LDAPVLVInfo vlvInfo;
static struct berval vlvValue;

static int ldapsync = 0;
static struct berval sync_cookie = { 0, NULL };
static int sync_slimit = -1;

/* cookie and morePagedResults moved to common.c */
static int pagedResults = 0;
static int pagePrompt = 1;
static ber_int_t pageSize = 0;
static ber_int_t entriesLeft = 0;
static int npagedresponses;
static int npagedentries;
static int npagedreferences;
static int npagedextended;
static int npagedpartial;

static LDAPControl *c = NULL;
static int nctrls = 0;
static int save_nctrls = 0;

#ifdef LDAP_CONTROL_X_DEREF
static int derefcrit;
static LDAPDerefSpec *ds;
static struct berval derefval;
#endif

static int
ctrl_add( void )
{
	LDAPControl	*tmpc;

	nctrls++;
	tmpc = realloc( c, sizeof( LDAPControl ) * nctrls );
	if ( tmpc == NULL ) {
		nctrls--;
		fprintf( stderr,
			_("unable to make room for control; out of memory?\n"));
		return -1;
	}
	c = tmpc;

	return 0;
}

static void
urlize(char *url)
{
	char *p;

	if (*LDAP_DIRSEP != '/') {
		for (p = url; *p; p++) {
			if (*p == *LDAP_DIRSEP)
				*p = '/';
		}
	}
}

static int
parse_vlv(char *cvalue)
{
	char *keyp, *key2;
	int num1, num2;

	keyp = cvalue;
	if ( sscanf( keyp, "%d/%d", &num1, &num2 ) != 2 ) {
		fprintf( stderr,
			_("VLV control value \"%s\" invalid\n"),
			cvalue );
		return -1;
	}
	vlvInfo.ldvlv_before_count = num1;
	vlvInfo.ldvlv_after_count = num2;
	keyp = strchr( keyp, '/' ) + 1;
	key2 = strchr( keyp, '/' );
	if ( key2 ) {
		keyp = key2 + 1;
		if ( sscanf( keyp, "%d/%d", &num1, &num2 ) != 2 ) {
			fprintf( stderr,
				_("VLV control value \"%s\" invalid\n"),
				cvalue );
			return -1;
		}
		vlvInfo.ldvlv_offset = num1;
		vlvInfo.ldvlv_count = num2;
		vlvInfo.ldvlv_attrvalue = NULL;
	} else {
		key2 = strchr( keyp, ':' );
		if ( !key2 ) {
			fprintf( stderr,
				_("VLV control value \"%s\" invalid\n"),
				cvalue );
			return -1;
		}
		ber_str2bv( key2+1, 0, 0, &vlvValue );
		vlvInfo.ldvlv_attrvalue = &vlvValue;
	}
	return 0;
}

const char options[] = "a:Ab:cE:F:l:Ls:S:tT:uz:"
	"Cd:D:e:f:h:H:IMnNO:o:p:P:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	int crit, ival;
	char *cvalue, *next;
	switch ( i ) {
	case 'a':	/* set alias deref option */
		if ( strcasecmp( optarg, "never" ) == 0 ) {
			deref = LDAP_DEREF_NEVER;
		} else if ( strncasecmp( optarg, "search", sizeof("search")-1 ) == 0 ) {
			deref = LDAP_DEREF_SEARCHING;
		} else if ( strncasecmp( optarg, "find", sizeof("find")-1 ) == 0 ) {
			deref = LDAP_DEREF_FINDING;
		} else if ( strcasecmp( optarg, "always" ) == 0 ) {
			deref = LDAP_DEREF_ALWAYS;
		} else {
			fprintf( stderr,
				_("alias deref should be never, search, find, or always\n") );
			usage();
		}
		break;
	case 'A':	/* retrieve attribute names only -- no values */
		++attrsonly;
		break;
	case 'b': /* search base */
		base = ber_strdup( optarg );
		break;
	case 'E': /* search extensions */
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
		while ( optarg[0] == '!' ) {
			crit++;
			optarg++;
		}

		control = ber_strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}

		if ( strcasecmp( control, "mv" ) == 0 ) {
			/* ValuesReturnFilter control */
			if( valuesReturnFilter ) {
				fprintf( stderr,
					_("ValuesReturnFilter previously specified\n"));
				exit( EXIT_FAILURE );
			}
			valuesReturnFilter= 1 + crit;

			if ( cvalue == NULL ) {
				fprintf( stderr,
					_("missing filter in ValuesReturnFilter control\n"));
				exit( EXIT_FAILURE );
			}

			vrFilter = cvalue;
			protocol = LDAP_VERSION3;

		} else if ( strcasecmp( control, "pr" ) == 0 ) {
			int num, tmp;
			/* PagedResults control */
			if ( pagedResults != 0 ) {
				fprintf( stderr,
					_("PagedResultsControl previously specified\n") );
				exit( EXIT_FAILURE );
			}
			if ( vlv != 0 ) {
				fprintf( stderr,
					_("PagedResultsControl incompatible with VLV\n") );
				exit( EXIT_FAILURE );
			}

			if( cvalue != NULL ) {
				char *promptp;

				promptp = strchr( cvalue, '/' );
				if ( promptp != NULL ) {
					*promptp++ = '\0';
					if ( strcasecmp( promptp, "prompt" ) == 0 ) {
						pagePrompt = 1;
					} else if ( strcasecmp( promptp, "noprompt" ) == 0) {
						pagePrompt = 0;
					} else {
						fprintf( stderr,
							_("Invalid value for PagedResultsControl,"
							" %s/%s.\n"), cvalue, promptp );
						exit( EXIT_FAILURE );
					}
				}
				num = sscanf( cvalue, "%d", &tmp );
				if ( num != 1 ) {
					fprintf( stderr,
						_("Invalid value for PagedResultsControl, %s.\n"),
						cvalue );
					exit( EXIT_FAILURE );
				}
			} else {
				fprintf(stderr, _("Invalid value for PagedResultsControl.\n"));
				exit( EXIT_FAILURE );
			}
			pageSize = (ber_int_t) tmp;
			pagedResults = 1 + crit;

#ifdef LDAP_CONTROL_DONTUSECOPY
		} else if ( strcasecmp( control, "dontUseCopy" ) == 0 ) {
			if( dontUseCopy ) {
				fprintf( stderr,
					_("dontUseCopy control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue != NULL ) {
				fprintf( stderr,
			         _("dontUseCopy: no control value expected\n") );
				usage();
			}
			if( !crit ) {
				fprintf( stderr,
			         _("dontUseCopy: critical flag required\n") );
				usage();
			}

			dontUseCopy = 1 + crit;
#endif
		} else if ( strcasecmp( control, "domainScope" ) == 0 ) {
			if( domainScope ) {
				fprintf( stderr,
					_("domainScope control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue != NULL ) {
				fprintf( stderr,
			         _("domainScope: no control value expected\n") );
				usage();
			}

			domainScope = 1 + crit;

		} else if ( strcasecmp( control, "sss" ) == 0 ) {
			char *keyp;
			if( sss ) {
				fprintf( stderr,
					_("server side sorting control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue == NULL ) {
				fprintf( stderr,
			         _("missing specification of sss control\n") );
				exit( EXIT_FAILURE );
			}
			keyp = cvalue;
			while ( ( keyp = strchr(keyp, '/') ) != NULL ) {
				*keyp++ = ' ';
			}
			if ( ldap_create_sort_keylist( &sss_keys, cvalue )) {
				fprintf( stderr,
					_("server side sorting control value \"%s\" invalid\n"),
					cvalue );
				exit( EXIT_FAILURE );
			}

			sss = 1 + crit;

		} else if ( strcasecmp( control, "subentries" ) == 0 ) {
			if( subentries ) {
				fprintf( stderr,
					_("subentries control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue == NULL || strcasecmp( cvalue, "true") == 0 ) {
				subentries = 2;
			} else if ( strcasecmp( cvalue, "false") == 0 ) {
				subentries = 1;
			} else {
				fprintf( stderr,
					_("subentries control value \"%s\" invalid\n"),
					cvalue );
				exit( EXIT_FAILURE );
			}
			if( crit ) subentries *= -1;

		} else if ( strcasecmp( control, "sync" ) == 0 ) {
			char *cookiep;
			char *slimitp;
			if ( ldapsync ) {
				fprintf( stderr, _("sync control previously specified\n") );
				exit( EXIT_FAILURE );
			}
			if ( cvalue == NULL ) {
				fprintf( stderr, _("missing specification of sync control\n"));
				exit( EXIT_FAILURE );
			}
			if ( strncasecmp( cvalue, "ro", 2 ) == 0 ) {
				ldapsync = LDAP_SYNC_REFRESH_ONLY;
				cookiep = strchr( cvalue, '/' );
				if ( cookiep != NULL ) {
					cookiep++;
					if ( *cookiep != '\0' ) {
						ber_str2bv( cookiep, 0, 0, &sync_cookie );
					}
				}
			} else if ( strncasecmp( cvalue, "rp", 2 ) == 0 ) {
				ldapsync = LDAP_SYNC_REFRESH_AND_PERSIST;
				cookiep = strchr( cvalue, '/' );
				if ( cookiep != NULL ) {
					*cookiep++ = '\0';	
					cvalue = cookiep;
				}
				slimitp = strchr( cvalue, '/' );
				if ( slimitp != NULL ) {
					*slimitp++ = '\0';
				}
				if ( cookiep != NULL && *cookiep != '\0' )
					ber_str2bv( cookiep, 0, 0, &sync_cookie );
				if ( slimitp != NULL && *slimitp != '\0' ) {
					ival = strtol( slimitp, &next, 10 );
					if ( next == NULL || next[0] != '\0' ) {
						fprintf( stderr, _("Unable to parse sync control value \"%s\"\n"), slimitp );
						exit( EXIT_FAILURE );
					}
					sync_slimit = ival;
				}
			} else {
				fprintf( stderr, _("sync control value \"%s\" invalid\n"),
					cvalue );
				exit( EXIT_FAILURE );
			}
			if ( crit ) ldapsync *= -1;

		} else if ( strcasecmp( control, "vlv" ) == 0 ) {
			if( vlv ) {
				fprintf( stderr,
					_("virtual list view control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if ( pagedResults != 0 ) {
				fprintf( stderr,
					_("PagedResultsControl incompatible with VLV\n") );
				exit( EXIT_FAILURE );
			}
			if( cvalue == NULL ) {
				fprintf( stderr,
			         _("missing specification of vlv control\n") );
				exit( EXIT_FAILURE );
			}
			if ( parse_vlv( cvalue ))
				exit( EXIT_FAILURE );

			vlv = 1 + crit;

#ifdef LDAP_CONTROL_X_DEREF
		} else if ( strcasecmp( control, "deref" ) == 0 ) {
			int ispecs;
			char **specs;

			/* cvalue is something like
			 *
			 * derefAttr:attr[,attr[...]][;derefAttr:attr[,attr[...]]]"
			 */

			specs = ldap_str2charray( cvalue, ";" );
			if ( specs == NULL ) {
				fprintf( stderr, _("deref specs \"%s\" invalid\n"),
					cvalue );
				exit( EXIT_FAILURE );
			}
			for ( ispecs = 0; specs[ ispecs ] != NULL; ispecs++ )
				/* count'em */ ;

			ds = ldap_memcalloc( ispecs + 1, sizeof( LDAPDerefSpec ) );
			if ( ds == NULL ) {
				perror( "malloc" );
				exit( EXIT_FAILURE );
			}

			for ( ispecs = 0; specs[ ispecs ] != NULL; ispecs++ ) {
				char *ptr;

				ptr = strchr( specs[ ispecs ], ':' );
				if ( ptr == NULL ) {
					fprintf( stderr, _("deref specs \"%s\" invalid\n"),
						cvalue );
					exit( EXIT_FAILURE );
				}

				ds[ ispecs ].derefAttr = specs[ ispecs ];
				*ptr++ = '\0';
				ds[ ispecs ].attributes = ldap_str2charray( ptr, "," );
			}

			derefcrit = 1 + crit;

			ldap_memfree( specs );
#endif /* LDAP_CONTROL_X_DEREF */

		} else if ( tool_is_oid( control ) ) {
			if ( c != NULL ) {
				int i;
				for ( i = 0; i < nctrls; i++ ) {
					if ( strcmp( control, c[ i ].ldctl_oid ) == 0 ) {
						fprintf( stderr, "%s control previously specified\n", control );
						exit( EXIT_FAILURE );
					}
				}
			}

			if ( ctrl_add() ) {
				exit( EXIT_FAILURE );
			}

			/* OID */
			c[ nctrls - 1 ].ldctl_oid = control;

			/* value */
			if ( cvalue == NULL ) {
				c[ nctrls - 1 ].ldctl_value.bv_val = NULL;
				c[ nctrls - 1 ].ldctl_value.bv_len = 0;

			} else if ( cvalue[ 0 ] == ':' ) {
				struct berval type;
				struct berval value;
				int freeval;
				char save_c;

				cvalue++;

				/* dummy type "x"
				 * to use ldif_parse_line2() */
				save_c = cvalue[ -2 ];
				cvalue[ -2 ] = 'x';
				ldif_parse_line2( &cvalue[ -2 ], &type,
					&value, &freeval );
				cvalue[ -2 ] = save_c;

				if ( freeval ) {
					c[ nctrls - 1 ].ldctl_value = value;

				} else {
					ber_dupbv( &c[ nctrls - 1 ].ldctl_value, &value );
				}

			} else {
				fprintf( stderr, "unable to parse %s control value\n", control );
				exit( EXIT_FAILURE );
				
			}

			/* criticality */
			c[ nctrls - 1 ].ldctl_iscritical = crit;

		} else {
			fprintf( stderr, _("Invalid search extension name: %s\n"),
				control );
			usage();
		}
		break;
	case 'F':	/* uri prefix */
		if( urlpre ) free( urlpre );
		urlpre = strdup( optarg );
		break;
	case 'l':	/* time limit */
		if ( strcasecmp( optarg, "none" ) == 0 ) {
			timelimit = 0;

		} else if ( strcasecmp( optarg, "max" ) == 0 ) {
			timelimit = LDAP_MAXINT;

		} else {
			ival = strtol( optarg, &next, 10 );
			if ( next == NULL || next[0] != '\0' ) {
				fprintf( stderr,
					_("Unable to parse time limit \"%s\"\n"), optarg );
				exit( EXIT_FAILURE );
			}
			timelimit = ival;
		}
		if( timelimit < 0 || timelimit > LDAP_MAXINT ) {
			fprintf( stderr, _("%s: invalid timelimit (%d) specified\n"),
				prog, timelimit );
			exit( EXIT_FAILURE );
		}
		break;
	case 'L':	/* print entries in LDIF format */
		++ldif;
		break;
	case 's':	/* search scope */
		if ( strncasecmp( optarg, "base", sizeof("base")-1 ) == 0 ) {
			scope = LDAP_SCOPE_BASE;
		} else if ( strncasecmp( optarg, "one", sizeof("one")-1 ) == 0 ) {
			scope = LDAP_SCOPE_ONELEVEL;
		} else if (( strcasecmp( optarg, "subordinate" ) == 0 )
			|| ( strcasecmp( optarg, "children" ) == 0 ))
		{
			scope = LDAP_SCOPE_SUBORDINATE;
		} else if ( strncasecmp( optarg, "sub", sizeof("sub")-1 ) == 0 ) {
			scope = LDAP_SCOPE_SUBTREE;
		} else {
			fprintf( stderr, _("scope should be base, one, or sub\n") );
			usage();
		}
		break;
	case 'S':	/* sort attribute */
		sortattr = strdup( optarg );
		break;
	case 't':	/* write attribute values to TMPDIR files */
		++vals2tmp;
		break;
	case 'T':	/* tmpdir */
		if( tmpdir ) free( tmpdir );
		tmpdir = strdup( optarg );
		break;
	case 'u':	/* include UFN */
		++includeufn;
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
	if (deref != -1 &&
		ldap_set_option( ld, LDAP_OPT_DEREF, (void *) &deref )
			!= LDAP_OPT_SUCCESS )
	{
		fprintf( stderr, _("Could not set LDAP_OPT_DEREF %d\n"), deref );
		tool_exit( ld, EXIT_FAILURE );
	}
}

int
main( int argc, char **argv )
{
	char		*filtpattern, **attrs = NULL, line[BUFSIZ];
	FILE		*fp = NULL;
	int			rc, rc1, i, first;
	LDAP		*ld = NULL;
	BerElement	*seber = NULL, *vrber = NULL;

	BerElement      *syncber = NULL;
	struct berval   *syncbvalp = NULL;
	int		err;

	tool_init( TOOL_SEARCH );

	npagedresponses = npagedentries = npagedreferences =
		npagedextended = npagedpartial = 0;

	prog = lutil_progname( "ldapsearch", argc, argv );

	if((def_tmpdir = getenv("TMPDIR")) == NULL &&
	   (def_tmpdir = getenv("TMP")) == NULL &&
	   (def_tmpdir = getenv("TEMP")) == NULL )
	{
		def_tmpdir = LDAP_TMPDIR;
	}

	if ( !*def_tmpdir )
		def_tmpdir = LDAP_TMPDIR;

	def_urlpre = malloc( sizeof("file:////") + strlen(def_tmpdir) );

	if( def_urlpre == NULL ) {
		perror( "malloc" );
		return EXIT_FAILURE;
	}

	sprintf( def_urlpre, "file:///%s/",
		def_tmpdir[0] == *LDAP_DIRSEP ? &def_tmpdir[1] : def_tmpdir );

	urlize( def_urlpre );

	tool_args( argc, argv );

	if ( vlv && !sss ) {
		fprintf( stderr,
			_("VLV control requires server side sort control\n" ));
		return EXIT_FAILURE;
	}

	if (( argc - optind < 1 ) ||
		( *argv[optind] != '(' /*')'*/ &&
		( strchr( argv[optind], '=' ) == NULL ) ) )
	{
		filtpattern = "(objectclass=*)";
	} else {
		filtpattern = argv[optind++];
	}

	if ( argv[optind] != NULL ) {
		attrs = &argv[optind];
	}

	if ( infile != NULL ) {
		int percent = 0;
	
		if ( infile[0] == '-' && infile[1] == '\0' ) {
			fp = stdin;
		} else if (( fp = fopen( infile, "r" )) == NULL ) {
			perror( infile );
			return EXIT_FAILURE;
		}

		for( i=0 ; filtpattern[i] ; i++ ) {
			if( filtpattern[i] == '%' ) {
				if( percent ) {
					fprintf( stderr, _("Bad filter pattern \"%s\"\n"),
						filtpattern );
					return EXIT_FAILURE;
				}

				percent++;

				if( filtpattern[i+1] != 's' ) {
					fprintf( stderr, _("Bad filter pattern \"%s\"\n"),
						filtpattern );
					return EXIT_FAILURE;
				}
			}
		}
	}

	if ( tmpdir == NULL ) {
		tmpdir = def_tmpdir;

		if ( urlpre == NULL )
			urlpre = def_urlpre;
	}

	if( urlpre == NULL ) {
		urlpre = malloc( sizeof("file:////") + strlen(tmpdir) );

		if( urlpre == NULL ) {
			perror( "malloc" );
			return EXIT_FAILURE;
		}

		sprintf( urlpre, "file:///%s/",
			tmpdir[0] == *LDAP_DIRSEP ? &tmpdir[1] : tmpdir );

		urlize( urlpre );
	}

	if ( debug )
		ldif_debug = debug;

	ld = tool_conn_setup( 0, &private_conn_setup );

	tool_bind( ld );

getNextPage:
	/* fp may have been closed, need to reopen if code jumps
	 * back here to getNextPage.
	 */
	if ( !fp && infile ) {
		if (( fp = fopen( infile, "r" )) == NULL ) {
			perror( infile );
			tool_exit( ld, EXIT_FAILURE );
		}
	}
	save_nctrls = nctrls;
	i = nctrls;
	if ( nctrls > 0
#ifdef LDAP_CONTROL_DONTUSECOPY
		|| dontUseCopy
#endif
#ifdef LDAP_CONTROL_X_DEREF
		|| derefcrit
#endif
		|| domainScope
		|| pagedResults
		|| ldapsync
		|| sss
		|| subentries
		|| valuesReturnFilter
		|| vlv )
	{

#ifdef LDAP_CONTROL_DONTUSECOPY
		if ( dontUseCopy ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_DONTUSECOPY;
			c[i].ldctl_value.bv_val = NULL;
			c[i].ldctl_value.bv_len = 0;
			c[i].ldctl_iscritical = dontUseCopy == 2;
			i++;
		}
#endif

		if ( domainScope ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_X_DOMAIN_SCOPE;
			c[i].ldctl_value.bv_val = NULL;
			c[i].ldctl_value.bv_len = 0;
			c[i].ldctl_iscritical = domainScope > 1;
			i++;
		}

		if ( subentries ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if (( seber = ber_alloc_t(LBER_USE_DER)) == NULL ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			err = ber_printf( seber, "b", abs(subentries) == 1 ? 0 : 1 );
			if ( err == -1 ) {
				ber_free( seber, 1 );
				fprintf( stderr, _("Subentries control encoding error!\n") );
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ber_flatten2( seber, &c[i].ldctl_value, 0 ) == -1 ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_SUBENTRIES;
			c[i].ldctl_iscritical = subentries < 1;
			i++;
		}

		if ( ldapsync ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if (( syncber = ber_alloc_t(LBER_USE_DER)) == NULL ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( sync_cookie.bv_len == 0 ) {
				err = ber_printf( syncber, "{e}", abs(ldapsync) );
			} else {
				err = ber_printf( syncber, "{eO}", abs(ldapsync),
							&sync_cookie );
			}

			if ( err == -1 ) {
				ber_free( syncber, 1 );
				fprintf( stderr, _("ldap sync control encoding error!\n") );
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ber_flatten( syncber, &syncbvalp ) == -1 ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_SYNC;
			c[i].ldctl_value = (*syncbvalp);
			c[i].ldctl_iscritical = ldapsync < 0;
			i++;
		}

		if ( valuesReturnFilter ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if (( vrber = ber_alloc_t(LBER_USE_DER)) == NULL ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ( err = ldap_put_vrFilter( vrber, vrFilter ) ) == -1 ) {
				ber_free( vrber, 1 );
				fprintf( stderr, _("Bad ValuesReturnFilter: %s\n"), vrFilter );
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ber_flatten2( vrber, &c[i].ldctl_value, 0 ) == -1 ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_VALUESRETURNFILTER;
			c[i].ldctl_iscritical = valuesReturnFilter > 1;
			i++;
		}

		if ( pagedResults ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ldap_create_page_control_value( ld,
				pageSize, &pr_cookie, &c[i].ldctl_value ) )
			{
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( pr_cookie.bv_val != NULL ) {
				ber_memfree( pr_cookie.bv_val );
				pr_cookie.bv_val = NULL;
				pr_cookie.bv_len = 0;
			}
			
			c[i].ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
			c[i].ldctl_iscritical = pagedResults > 1;
			i++;
		}

		if ( sss ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ldap_create_sort_control_value( ld,
				sss_keys, &c[i].ldctl_value ) )
			{
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_SORTREQUEST;
			c[i].ldctl_iscritical = sss > 1;
			i++;
		}

		if ( vlv ) {
			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			if ( ldap_create_vlv_control_value( ld,
				&vlvInfo, &c[i].ldctl_value ) )
			{
				tool_exit( ld, EXIT_FAILURE );
			}

			c[i].ldctl_oid = LDAP_CONTROL_VLVREQUEST;
			c[i].ldctl_iscritical = sss > 1;
			i++;
		}
#ifdef LDAP_CONTROL_X_DEREF
		if ( derefcrit ) {
			if ( derefval.bv_val == NULL ) {
				int i;

				assert( ds != NULL );

				if ( ldap_create_deref_control_value( ld, ds, &derefval ) != LDAP_SUCCESS ) {
					tool_exit( ld, EXIT_FAILURE );
				}

				for ( i = 0; ds[ i ].derefAttr != NULL; i++ ) {
					ldap_memfree( ds[ i ].derefAttr );
					ldap_charray_free( ds[ i ].attributes );
				}
				ldap_memfree( ds );
				ds = NULL;
			}

			if ( ctrl_add() ) {
				tool_exit( ld, EXIT_FAILURE );
			}

			c[ i ].ldctl_iscritical = derefcrit > 1;
			c[ i ].ldctl_oid = LDAP_CONTROL_X_DEREF;
			c[ i ].ldctl_value = derefval;
			i++;
		}
#endif /* LDAP_CONTROL_X_DEREF */
	}

	tool_server_controls( ld, c, i );

	if ( seber ) ber_free( seber, 1 );
	if ( vrber ) ber_free( vrber, 1 );

	/* step back to the original number of controls, so that 
	 * those set while parsing args are preserved */
	nctrls = save_nctrls;

	if ( verbose ) {
		fprintf( stderr, _("filter%s: %s\nrequesting: "),
			infile != NULL ? _(" pattern") : "",
			filtpattern );

		if ( attrs == NULL ) {
			fprintf( stderr, _("All userApplication attributes") );
		} else {
			for ( i = 0; attrs[ i ] != NULL; ++i ) {
				fprintf( stderr, "%s ", attrs[ i ] );
			}
		}
		fprintf( stderr, "\n" );
	}

	if ( ldif == 0 ) {
		printf( _("# extended LDIF\n") );
	} else if ( ldif < 3 ) {
		printf( _("version: %d\n\n"), 1 );
	}

	if (ldif < 2 ) {
		char	*realbase = base;

		if ( realbase == NULL ) {
			ldap_get_option( ld, LDAP_OPT_DEFBASE, (void **)(char *)&realbase );
		}
		
		printf( "#\n" );
		printf(_("# LDAPv%d\n"), protocol);
		printf(_("# base <%s>%s with scope %s\n"),
			realbase ? realbase : "",
			( realbase == NULL || realbase != base ) ? " (default)" : "",
			((scope == LDAP_SCOPE_BASE) ? "baseObject"
				: ((scope == LDAP_SCOPE_ONELEVEL) ? "oneLevel"
				: ((scope == LDAP_SCOPE_SUBORDINATE) ? "children"
				: "subtree" ))));
		printf(_("# filter%s: %s\n"), infile != NULL ? _(" pattern") : "",
		       filtpattern);
		printf(_("# requesting: "));

		if ( attrs == NULL ) {
			printf( _("ALL") );
		} else {
			for ( i = 0; attrs[ i ] != NULL; ++i ) {
				printf( "%s ", attrs[ i ] );
			}
		}

		if ( manageDSAit ) {
			printf(_("\n# with manageDSAit %scontrol"),
				manageDSAit > 1 ? _("critical ") : "" );
		}
		if ( noop ) {
			printf(_("\n# with noop %scontrol"),
				noop > 1 ? _("critical ") : "" );
		}
		if ( subentries ) {
			printf(_("\n# with subentries %scontrol: %s"),
				subentries < 0 ? _("critical ") : "",
				abs(subentries) == 1 ? "false" : "true" );
		}
		if ( valuesReturnFilter ) {
			printf(_("\n# with valuesReturnFilter %scontrol: %s"),
				valuesReturnFilter > 1 ? _("critical ") : "", vrFilter );
		}
		if ( pagedResults ) {
			printf(_("\n# with pagedResults %scontrol: size=%d"),
				(pagedResults > 1) ? _("critical ") : "", 
				pageSize );
		}
		if ( sss ) {
			printf(_("\n# with server side sorting %scontrol"),
				sss > 1 ? _("critical ") : "" );
		}
		if ( vlv ) {
			printf(_("\n# with virtual list view %scontrol: %d/%d"),
				vlv > 1 ? _("critical ") : "",
				vlvInfo.ldvlv_before_count, vlvInfo.ldvlv_after_count);
			if ( vlvInfo.ldvlv_attrvalue )
				printf(":%s", vlvInfo.ldvlv_attrvalue->bv_val );
			else
				printf("/%d/%d", vlvInfo.ldvlv_offset, vlvInfo.ldvlv_count );
		}
#ifdef LDAP_CONTROL_X_DEREF
		if ( derefcrit ) {
			printf(_("\n# with dereference %scontrol"),
				derefcrit > 1 ? _("critical ") : "" );
		}
#endif

		printf( _("\n#\n\n") );

		if ( realbase && realbase != base ) {
			ldap_memfree( realbase );
		}
	}

	if ( infile == NULL ) {
		rc = dosearch( ld, base, scope, NULL, filtpattern,
			attrs, attrsonly, NULL, NULL, NULL, sizelimit );

	} else {
		rc = 0;
		first = 1;
		while ( fgets( line, sizeof( line ), fp ) != NULL ) { 
			line[ strlen( line ) - 1 ] = '\0';
			if ( !first ) {
				putchar( '\n' );
			} else {
				first = 0;
			}
			rc1 = dosearch( ld, base, scope, filtpattern, line,
				attrs, attrsonly, NULL, NULL, NULL, sizelimit );

			if ( rc1 != 0 ) {
				rc = rc1;
				if ( !contoper )
					break;
			}
		}
		if ( fp != stdin ) {
			fclose( fp );
			fp = NULL;
		}
	}

	if (( rc == LDAP_SUCCESS ) && pageSize && pr_morePagedResults ) {
		char	buf[12];
		int	i, moreEntries, tmpSize;

		/* Loop to get the next pages when 
		 * enter is pressed on the terminal.
		 */
		if ( pagePrompt != 0 ) {
			if ( entriesLeft > 0 ) {
				printf( _("Estimate entries: %d\n"), entriesLeft );
			}
			printf( _("Press [size] Enter for the next {%d|size} entries.\n"),
				(int)pageSize );
			i = 0;
			moreEntries = getchar();
			while ( moreEntries != EOF && moreEntries != '\n' ) { 
				if ( i < (int)sizeof(buf) - 1 ) {
					buf[i] = moreEntries;
					i++;
				}
				moreEntries = getchar();
			}
			buf[i] = '\0';

			if ( i > 0 && isdigit( (unsigned char)buf[0] ) ) {
				int num = sscanf( buf, "%d", &tmpSize );
				if ( num != 1 ) {
					fprintf( stderr,
						_("Invalid value for PagedResultsControl, %s.\n"), buf);
					tool_exit( ld, EXIT_FAILURE );
	
				}
				pageSize = (ber_int_t)tmpSize;
			}
		}

		goto getNextPage;
	}

	if (( rc == LDAP_SUCCESS ) && vlv ) {
		char	buf[BUFSIZ];
		int	i, moreEntries;

		/* Loop to get the next window when 
		 * enter is pressed on the terminal.
		 */
		printf( _("Press [before/after(/offset/count|:value)] Enter for the next window.\n"));
		i = 0;
		moreEntries = getchar();
		while ( moreEntries != EOF && moreEntries != '\n' ) { 
			if ( i < (int)sizeof(buf) - 1 ) {
				buf[i] = moreEntries;
				i++;
			}
			moreEntries = getchar();
		}
		buf[i] = '\0';
		if ( buf[0] ) {
			i = parse_vlv( strdup( buf ));
			if ( i )
				tool_exit( ld, EXIT_FAILURE );
		} else {
			vlvInfo.ldvlv_attrvalue = NULL;
			vlvInfo.ldvlv_count = vlvCount;
			vlvInfo.ldvlv_offset += vlvInfo.ldvlv_after_count;
		}

		if ( vlvInfo.ldvlv_context )
			ber_bvfree( vlvInfo.ldvlv_context );
		vlvInfo.ldvlv_context = vlvContext;

		goto getNextPage;
	}

	if ( base != NULL ) {
		ber_memfree( base );
	}
	if ( control != NULL ) {
		ber_memfree( control );
	}
	if ( sss_keys != NULL ) {
		ldap_free_sort_keylist( sss_keys );
	}
	if ( derefval.bv_val != NULL ) {
		ldap_memfree( derefval.bv_val );
	}
	if ( urlpre != NULL ) {
		if ( def_urlpre != urlpre )
			free( def_urlpre );
		free( urlpre );
	}

	if ( c ) {
		for ( ; save_nctrls-- > 0; ) {
			ber_memfree( c[ save_nctrls ].ldctl_value.bv_val );
		}
		free( c );
		c = NULL;
	}

	tool_exit( ld, rc );
}


static int dosearch(
	LDAP	*ld,
	char	*base,
	int		scope,
	char	*filtpatt,
	char	*value,
	char	**attrs,
	int		attrsonly,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	struct timeval *timeout,
	int sizelimit )
{
	char			*filter;
	int			rc, rc2 = LDAP_OTHER;
	int			nresponses;
	int			nentries;
	int			nreferences;
	int			nextended;
	int			npartial;
	LDAPMessage		*res, *msg;
	ber_int_t		msgid;
	char			*retoid = NULL;
	struct berval		*retdata = NULL;
	int			nresponses_psearch = -1;
	int			cancel_msgid = -1;
	struct timeval tv, *tvp = NULL;
	struct timeval tv_timelimit, *tv_timelimitp = NULL;

	if( filtpatt != NULL ) {
		size_t max_fsize = strlen( filtpatt ) + strlen( value ) + 1, outlen;
		filter = malloc( max_fsize );
		if( filter == NULL ) {
			perror( "malloc" );
			return EXIT_FAILURE;
		}

		outlen = snprintf( filter, max_fsize, filtpatt, value );
		if( outlen >= max_fsize ) {
			fprintf( stderr, "Bad filter pattern: \"%s\"\n", filtpatt );
			free( filter );
			return EXIT_FAILURE;
		}

		if ( verbose ) {
			fprintf( stderr, _("filter: %s\n"), filter );
		}

		if( ldif < 2 ) {
			printf( _("#\n# filter: %s\n#\n"), filter );
		}

	} else {
		filter = value;
	}

	if ( dont ) {
		if ( filtpatt != NULL ) {
			free( filter );
		}
		return LDAP_SUCCESS;
	}

	if ( timelimit > 0 ) {
		tv_timelimit.tv_sec = timelimit;
		tv_timelimit.tv_usec = 0;
		tv_timelimitp = &tv_timelimit;
	}

	rc = ldap_search_ext( ld, base, scope, filter, attrs, attrsonly,
		sctrls, cctrls, tv_timelimitp, sizelimit, &msgid );

	if ( filtpatt != NULL ) {
		free( filter );
	}

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_search_ext", rc, NULL, NULL, NULL, NULL );
		return( rc );
	}

	nresponses = nentries = nreferences = nextended = npartial = 0;

	res = NULL;

	if ( timelimit > 0 ) {
		/* disable timeout */
		tv.tv_sec = -1;
		tv.tv_usec = 0;
		tvp = &tv;
	}

	while ((rc = ldap_result( ld, LDAP_RES_ANY,
		sortattr ? LDAP_MSG_ALL : LDAP_MSG_ONE,
		tvp, &res )) > 0 )
	{
		if ( tool_check_abandon( ld, msgid ) ) {
			return -1;
		}

		if( sortattr ) {
			(void) ldap_sort_entries( ld, &res,
				( *sortattr == '\0' ) ? NULL : sortattr, strcasecmp );
		}

		for ( msg = ldap_first_message( ld, res );
			msg != NULL;
			msg = ldap_next_message( ld, msg ) )
		{
			if ( nresponses++ ) putchar('\n');
			if ( nresponses_psearch >= 0 ) 
				nresponses_psearch++;

			switch( ldap_msgtype( msg ) ) {
			case LDAP_RES_SEARCH_ENTRY:
				nentries++;
				print_entry( ld, msg, attrsonly );
				break;

			case LDAP_RES_SEARCH_REFERENCE:
				nreferences++;
				print_reference( ld, msg );
				break;

			case LDAP_RES_EXTENDED:
				nextended++;
				print_extended( ld, msg );

				if ( ldap_msgid( msg ) == 0 ) {
					/* unsolicited extended operation */
					goto done;
				}

				if ( cancel_msgid != -1 &&
						cancel_msgid == ldap_msgid( msg ) ) {
					printf(_("Cancelled \n"));
					printf(_("cancel_msgid = %d\n"), cancel_msgid);
					goto done;
				}
				break;

			case LDAP_RES_SEARCH_RESULT:
				/* pagedResults stuff is dealt with
				 * in tool_print_ctrls(), called by
				 * print_results(). */
				rc2 = print_result( ld, msg, 1 );
				if ( ldapsync == LDAP_SYNC_REFRESH_AND_PERSIST ) {
					break;
				}

				goto done;

			case LDAP_RES_INTERMEDIATE:
				npartial++;
				ldap_parse_intermediate( ld, msg,
					&retoid, &retdata, NULL, 0 );

				nresponses_psearch = 0;

				if ( strcmp( retoid, LDAP_SYNC_INFO ) == 0 ) {
					printf(_("SyncInfo Received\n"));
					ldap_memfree( retoid );
					ber_bvfree( retdata );
					break;
				}

				print_partial( ld, msg );
				ldap_memfree( retoid );
				ber_bvfree( retdata );
				goto done;
			}

			if ( ldapsync && sync_slimit != -1 &&
					nresponses_psearch >= sync_slimit ) {
				BerElement *msgidber = NULL;
				struct berval *msgidvalp = NULL;
				msgidber = ber_alloc_t(LBER_USE_DER);
				ber_printf(msgidber, "{i}", msgid);
				ber_flatten(msgidber, &msgidvalp);
				ldap_extended_operation(ld, LDAP_EXOP_CANCEL,
					msgidvalp, NULL, NULL, &cancel_msgid);
				nresponses_psearch = -1;
			}
		}

		ldap_msgfree( res );
	}

done:
	if ( tvp == NULL && rc != LDAP_RES_SEARCH_RESULT ) {
		ldap_get_option( ld, LDAP_OPT_RESULT_CODE, (void *)&rc2 );
	}

	ldap_msgfree( res );

	if ( pagedResults ) { 
		npagedresponses += nresponses;
		npagedentries += nentries;
		npagedextended += nextended;
		npagedpartial += npartial;
		npagedreferences += nreferences;
		if ( ( pr_morePagedResults == 0 ) && ( ldif < 2 ) ) {
			printf( _("\n# numResponses: %d\n"), npagedresponses );
			if( npagedentries ) {
				printf( _("# numEntries: %d\n"), npagedentries );
			}
			if( npagedextended ) {
				printf( _("# numExtended: %d\n"), npagedextended );
			}
			if( npagedpartial ) {
				printf( _("# numPartial: %d\n"), npagedpartial );
			}
			if( npagedreferences ) {
				printf( _("# numReferences: %d\n"), npagedreferences );
			}
		}
	} else if ( ldif < 2 ) {
		printf( _("\n# numResponses: %d\n"), nresponses );
		if( nentries ) printf( _("# numEntries: %d\n"), nentries );
		if( nextended ) printf( _("# numExtended: %d\n"), nextended );
		if( npartial ) printf( _("# numPartial: %d\n"), npartial );
		if( nreferences ) printf( _("# numReferences: %d\n"), nreferences );
	}

	if ( rc != LDAP_RES_SEARCH_RESULT ) {
		tool_perror( "ldap_result", rc2, NULL, NULL, NULL, NULL );
	}

	return( rc2 );
}

/* This is the proposed new way of doing things.
 * It is more efficient, but the API is non-standard.
 */
static void
print_entry(
	LDAP	*ld,
	LDAPMessage	*entry,
	int		attrsonly)
{
	char		*ufn = NULL;
	char	tmpfname[ 256 ];
	char	url[ 256 ];
	int			i, rc;
	BerElement		*ber = NULL;
	struct berval		bv, *bvals, **bvp = &bvals;
	LDAPControl **ctrls = NULL;
	FILE		*tmpfp;

	rc = ldap_get_dn_ber( ld, entry, &ber, &bv );

	if ( ldif < 2 ) {
		ufn = ldap_dn2ufn( bv.bv_val );
		tool_write_ldif( LDIF_PUT_COMMENT, NULL, ufn, ufn ? strlen( ufn ) : 0 );
	}
	tool_write_ldif( LDIF_PUT_VALUE, "dn", bv.bv_val, bv.bv_len );

	rc = ldap_get_entry_controls( ld, entry, &ctrls );
	if( rc != LDAP_SUCCESS ) {
		fprintf(stderr, _("print_entry: %d\n"), rc );
		tool_perror( "ldap_get_entry_controls", rc, NULL, NULL, NULL, NULL );
		tool_exit( ld, EXIT_FAILURE );
	}

	if( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	if ( includeufn ) {
		if( ufn == NULL ) {
			ufn = ldap_dn2ufn( bv.bv_val );
		}
		tool_write_ldif( LDIF_PUT_VALUE, "ufn", ufn, ufn ? strlen( ufn ) : 0 );
	}

	if( ufn != NULL ) ldap_memfree( ufn );

	if ( attrsonly ) bvp = NULL;

	for ( rc = ldap_get_attribute_ber( ld, entry, ber, &bv, bvp );
		rc == LDAP_SUCCESS;
		rc = ldap_get_attribute_ber( ld, entry, ber, &bv, bvp ) )
	{
		if (bv.bv_val == NULL) break;

		if ( attrsonly ) {
			tool_write_ldif( LDIF_PUT_NOVALUE, bv.bv_val, NULL, 0 );

		} else if ( bvals ) {
			for ( i = 0; bvals[i].bv_val != NULL; i++ ) {
				if ( vals2tmp > 1 || ( vals2tmp &&
					ldif_is_not_printable( bvals[i].bv_val, bvals[i].bv_len )))
				{
					int tmpfd;
					/* write value to file */
					snprintf( tmpfname, sizeof tmpfname,
						"%s" LDAP_DIRSEP "ldapsearch-%s-XXXXXX",
						tmpdir, bv.bv_val );
					tmpfp = NULL;

					tmpfd = mkstemp( tmpfname );

					if ( tmpfd < 0  ) {
						perror( tmpfname );
						continue;
					}

					if (( tmpfp = fdopen( tmpfd, "w")) == NULL ) {
						perror( tmpfname );
						continue;
					}

					if ( fwrite( bvals[ i ].bv_val,
						bvals[ i ].bv_len, 1, tmpfp ) == 0 )
					{
						perror( tmpfname );
						fclose( tmpfp );
						continue;
					}

					fclose( tmpfp );

					snprintf( url, sizeof url, "%s%s", urlpre,
						&tmpfname[strlen(tmpdir) + sizeof(LDAP_DIRSEP) - 1] );

					urlize( url );
					tool_write_ldif( LDIF_PUT_URL, bv.bv_val, url, strlen( url ));

				} else {
					tool_write_ldif( LDIF_PUT_VALUE, bv.bv_val,
						bvals[ i ].bv_val, bvals[ i ].bv_len );
				}
			}
			ber_memfree( bvals );
		}
	}

	if( ber != NULL ) {
		ber_free( ber, 0 );
	}
}

static void print_reference(
	LDAP *ld,
	LDAPMessage *reference )
{
	int rc;
	char **refs = NULL;
	LDAPControl **ctrls;

	if( ldif < 2 ) {
		printf(_("# search reference\n"));
	}

	rc = ldap_parse_reference( ld, reference, &refs, &ctrls, 0 );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_reference", rc, NULL, NULL, NULL, NULL );
		tool_exit( ld, EXIT_FAILURE );
	}

	if( refs ) {
		int i;
		for( i=0; refs[i] != NULL; i++ ) {
			tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
				"ref", refs[i], strlen(refs[i]) );
		}
		ber_memvfree( (void **) refs );
	}

	if( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}
}

static void print_extended(
	LDAP *ld,
	LDAPMessage *extended )
{
	int rc;
	char *retoid = NULL;
	struct berval *retdata = NULL;

	if( ldif < 2 ) {
		printf(_("# extended result response\n"));
	}

	rc = ldap_parse_extended_result( ld, extended,
		&retoid, &retdata, 0 );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_extended_result", rc, NULL, NULL, NULL, NULL );
		tool_exit( ld, EXIT_FAILURE );
	}

	if ( ldif < 2 ) {
		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			"extended", retoid, retoid ? strlen(retoid) : 0 );
	}
	ber_memfree( retoid );

	if(retdata) {
		if ( ldif < 2 ) {
			tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_BINARY,
				"data", retdata->bv_val, retdata->bv_len );
		}
		ber_bvfree( retdata );
	}

	print_result( ld, extended, 0 );
}

static void print_partial(
	LDAP *ld,
	LDAPMessage *partial )
{
	int rc;
	char *retoid = NULL;
	struct berval *retdata = NULL;
	LDAPControl **ctrls = NULL;

	if( ldif < 2 ) {
		printf(_("# extended partial response\n"));
	}

	rc = ldap_parse_intermediate( ld, partial,
		&retoid, &retdata, &ctrls, 0 );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_intermediate", rc, NULL, NULL, NULL, NULL );
		tool_exit( ld, EXIT_FAILURE );
	}

	if ( ldif < 2 ) {
		tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_VALUE,
			"partial", retoid, retoid ? strlen(retoid) : 0 );
	}

	ber_memfree( retoid );

	if( retdata ) {
		if ( ldif < 2 ) {
			tool_write_ldif( ldif ? LDIF_PUT_COMMENT : LDIF_PUT_BINARY,
				"data", retdata->bv_val, retdata->bv_len );
		}

		ber_bvfree( retdata );
	}

	if( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}
}

static int print_result(
	LDAP *ld,
	LDAPMessage *result, int search )
{
	int rc;
	int err;
	char *matcheddn = NULL;
	char *text = NULL;
	char **refs = NULL;
	LDAPControl **ctrls = NULL;

	if( search ) {
		if ( ldif < 2 ) {
			printf(_("# search result\n"));
		}
		if ( ldif < 1 ) {
			printf("%s: %d\n", _("search"), ldap_msgid(result) );
		}
	}

	rc = ldap_parse_result( ld, result,
		&err, &matcheddn, &text, &refs, &ctrls, 0 );

	if( rc != LDAP_SUCCESS ) {
		tool_perror( "ldap_parse_result", rc, NULL, NULL, NULL, NULL );
		tool_exit( ld, EXIT_FAILURE );
	}


	if( !ldif ) {
		printf( _("result: %d %s\n"), err, ldap_err2string(err) );

	} else if ( err != LDAP_SUCCESS ) {
		fprintf( stderr, "%s (%d)\n", ldap_err2string(err), err );
	}

	if( matcheddn ) {
		if( *matcheddn ) {
		if( !ldif ) {
			tool_write_ldif( LDIF_PUT_VALUE,
				"matchedDN", matcheddn, strlen(matcheddn) );
		} else {
			fprintf( stderr, _("Matched DN: %s\n"), matcheddn );
		}
		}

		ber_memfree( matcheddn );
	}

	if( text ) {
		if( *text ) {
			if( !ldif ) {
				if ( err == LDAP_PARTIAL_RESULTS ) {
					char	*line;

					for ( line = text; line != NULL; ) {
						char	*next = strchr( line, '\n' );

						tool_write_ldif( LDIF_PUT_TEXT,
							"text", line,
							next ? (size_t) (next - line) : strlen( line ));

						line = next ? next + 1 : NULL;
					}

				} else {
					tool_write_ldif( LDIF_PUT_TEXT, "text",
						text, strlen(text) );
				}
			} else {
				fprintf( stderr, _("Additional information: %s\n"), text );
			}
		}

		ber_memfree( text );
	}

	if( refs ) {
		int i;
		for( i=0; refs[i] != NULL; i++ ) {
			if( !ldif ) {
				tool_write_ldif( LDIF_PUT_VALUE, "ref", refs[i], strlen(refs[i]) );
			} else {
				fprintf( stderr, _("Referral: %s\n"), refs[i] );
			}
		}

		ber_memvfree( (void **) refs );
	}

	pr_morePagedResults = 0;

	if( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	return err;
}
