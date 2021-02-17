/* ldapmodify.c - generic program to modify or add entries using LDAP */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2006 Howard Chu.
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
 *   Kurt D. Zeilenga
 *   Norbert Klasen
 *   Howard Chu
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/socket.h>
#include <ac/time.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <ldap.h>

#include "lutil.h"
#include "lutil_ldap.h"
#include "ldif.h"
#include "ldap_defaults.h"
#include "ldap_pvt.h"
#include "lber_pvt.h"

#include "common.h"

static int	ldapadd;
static char *rejfile = NULL;
static LDAP	*ld = NULL;

static int process_ldif_rec LDAP_P(( char *rbuf, unsigned long lineno ));
static int domodify LDAP_P((
	const struct berval *dn,
	LDAPMod **pmods,
	LDAPControl **pctrls,
	int newentry ));
static int dodelete LDAP_P((
	const struct berval *dn,
	LDAPControl **pctrls ));
static int dorename LDAP_P((
	const struct berval *dn,
	const struct berval *newrdn,
	const struct berval *newsup,
	int deleteoldrdn,
	LDAPControl **pctrls ));
static int process_response(
	LDAP *ld,
	int msgid,
	int res,
	const struct berval *dn );

#ifdef LDAP_X_TXN
static int txn = 0;
static int txnabort = 0;
struct berval *txn_id = NULL;
#endif

void
usage( void )
{
	fprintf( stderr, _("Add or modify entries from an LDAP server\n\n"));
	fprintf( stderr, _("usage: %s [options]\n"), prog);
	fprintf( stderr, _("	The list of desired operations are read from stdin"
		" or from the file\n"));
	fprintf( stderr, _("	specified by \"-f file\".\n"));
	fprintf( stderr, _("Add or modify options:\n"));
	fprintf( stderr, _("  -a         add values (%s)\n"),
		(ldapadd ? _("default") : _("default is to replace")));
	fprintf( stderr, _("  -c         continuous operation mode (do not stop on errors)\n"));
	fprintf( stderr, _("  -E [!]ext=extparam	modify extensions"
		" (! indicate s criticality)\n"));
	fprintf( stderr, _("  -f file    read operations from `file'\n"));
	fprintf( stderr, _("  -M         enable Manage DSA IT control (-MM to make critical)\n"));
	fprintf( stderr, _("  -P version protocol version (default: 3)\n"));
#ifdef LDAP_X_TXN
 	fprintf( stderr,
		_("             [!]txn=<commit|abort>         (transaction)\n"));
#endif
	fprintf( stderr, _("  -S file    write skipped modifications to `file'\n"));

	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "aE:rS:"
	"cd:D:e:f:h:H:IMnNO:o:p:P:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	char	*control, *cvalue;
	int		crit;

	switch ( i ) {
	case 'E': /* modify extensions */
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

		control = ber_strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}

#ifdef LDAP_X_TXN
		if( strcasecmp( control, "txn" ) == 0 ) {
			/* Transaction */
			if( txn ) {
				fprintf( stderr,
					_("txn control previously specified\n"));
				exit( EXIT_FAILURE );
			}
			if( cvalue != NULL ) {
				if( strcasecmp( cvalue, "abort" ) == 0 ) {
					txnabort=1;
				} else if( strcasecmp( cvalue, "commit" ) != 0 ) {
					fprintf( stderr, _("Invalid value for txn control, %s\n"),
						cvalue );
					exit( EXIT_FAILURE );
				}
			}

			txn = 1 + crit;
		} else
#endif
		{
			fprintf( stderr, _("Invalid modify extension name: %s\n"),
				control );
			usage();
		}
		break;

	case 'a':	/* add */
		ldapadd = 1;
		break;

	case 'r':	/* replace (obsolete) */
		break;

	case 'S':	/* skipped modifications to file */
		if( rejfile != NULL ) {
			fprintf( stderr, _("%s: -S previously specified\n"), prog );
			exit( EXIT_FAILURE );
		}
		rejfile = ber_strdup( optarg );
		break;

	default:
		return 0;
	}
	return 1;
}


int
main( int argc, char **argv )
{
	char		*rbuf = NULL, *rejbuf = NULL;
	FILE		*rejfp;
	struct LDIFFP *ldiffp = NULL, ldifdummy = {0};
	char		*matched_msg, *error_msg;
	int		rc, retval, ldifrc;
	int		len;
	int		i = 0, lmax = 0;
	unsigned long	lineno, nextline = 0;
	LDAPControl	c[1];

	prog = lutil_progname( "ldapmodify", argc, argv );

	/* strncmp instead of strcmp since NT binaries carry .exe extension */
	ldapadd = ( strncasecmp( prog, "ldapadd", sizeof("ldapadd")-1 ) == 0 );

	tool_init( ldapadd ? TOOL_ADD : TOOL_MODIFY );

	tool_args( argc, argv );

	if ( argc != optind ) usage();

	if ( rejfile != NULL ) {
		if (( rejfp = fopen( rejfile, "w" )) == NULL ) {
			perror( rejfile );
			retval = EXIT_FAILURE;
			goto fail;
		}
	} else {
		rejfp = NULL;
	}

	if ( infile != NULL ) {
		if (( ldiffp = ldif_open( infile, "r" )) == NULL ) {
			perror( infile );
			retval = EXIT_FAILURE;
			goto fail;
		}
	} else {
		ldifdummy.fp = stdin;
		ldiffp = &ldifdummy;
	}

	if ( debug ) ldif_debug = debug;

	ld = tool_conn_setup( dont, 0 );

	if ( !dont ) {
		tool_bind( ld );
	}

#ifdef LDAP_X_TXN
	if( txn ) {
		/* start transaction */
		rc = ldap_txn_start_s( ld, NULL, NULL, &txn_id );
		if( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_txn_start_s", rc, NULL, NULL, NULL, NULL );
			if( txn > 1 ) {
				retval = EXIT_FAILURE;
				goto fail;
			}
			txn = 0;
		}
	}
#endif

	if ( 0
#ifdef LDAP_X_TXN
		|| txn
#endif
		)
	{
#ifdef LDAP_X_TXN
		if( txn ) {
			c[i].ldctl_oid = LDAP_CONTROL_X_TXN_SPEC;
			c[i].ldctl_value = *txn_id;
			c[i].ldctl_iscritical = 1;
			i++;
		}
#endif
	}

	tool_server_controls( ld, c, i );

	rc = 0;
	retval = 0;
	lineno = 1;
	while (( rc == 0 || contoper ) && ( ldifrc = ldif_read_record( ldiffp, &nextline,
		&rbuf, &lmax )) > 0 )
	{
		if ( rejfp ) {
			len = strlen( rbuf );
			if (( rejbuf = (char *)ber_memalloc( len+1 )) == NULL ) {
				perror( "malloc" );
				retval = EXIT_FAILURE;
				goto fail;
			}
			memcpy( rejbuf, rbuf, len+1 );
		}

		rc = process_ldif_rec( rbuf, lineno );
		lineno = nextline+1;

		if ( rc ) retval = rc;
		if ( rc && rejfp ) {
			fprintf(rejfp, _("# Error: %s (%d)"), ldap_err2string(rc), rc);

			matched_msg = NULL;
			ldap_get_option(ld, LDAP_OPT_MATCHED_DN, &matched_msg);
			if ( matched_msg != NULL ) {
				if ( *matched_msg != '\0' ) {
					fprintf( rejfp, _(", matched DN: %s"), matched_msg );
				}
				ldap_memfree( matched_msg );
			}

			error_msg = NULL;
			ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, &error_msg);
			if ( error_msg != NULL ) {
				if ( *error_msg != '\0' ) {
					fprintf( rejfp, _(", additional info: %s"), error_msg );
				}
				ldap_memfree( error_msg );
			}
			fprintf( rejfp, "\n%s\n", rejbuf );
		}

		if (rejfp) ber_memfree( rejbuf );
	}
	ber_memfree( rbuf );

	if ( ldifrc < 0 )
		retval = LDAP_OTHER;

#ifdef LDAP_X_TXN
	if( retval == 0 && txn ) {
		rc = ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, NULL );
		if ( rc != LDAP_OPT_SUCCESS ) {
			fprintf( stderr, "Could not unset controls for ldap_txn_end\n");
		}

		/* create transaction */
		rc = ldap_txn_end_s( ld, !txnabort, txn_id, NULL, NULL, NULL );
		if( rc != LDAP_SUCCESS ) {
			tool_perror( "ldap_txn_end_s", rc, NULL, NULL, NULL, NULL );
			retval = rc;
		}
	}
#endif

fail:;
	if ( rejfp != NULL ) {
		fclose( rejfp );
	}

	if ( ldiffp != NULL && ldiffp != &ldifdummy ) {
		ldif_close( ldiffp );
	}

	tool_exit( ld, retval );
}


static int
process_ldif_rec( char *rbuf, unsigned long linenum )
{
	LDIFRecord lr;
	int lrflags = ldapadd ? LDIF_DEFAULT_ADD : 0;
	int rc;
	struct berval rbuf_bv;

#ifdef TEST_LDIF_API
	if ( getenv( "LDIF_ENTRIES_ONLY" ) ) {
		lrflags |= LDIF_ENTRIES_ONLY;
	}
	if ( getenv( "LDIF_NO_CONTROLS" ) ) {
		lrflags |= LDIF_NO_CONTROLS;
	}
#endif /* TEST_LDIF_API */

	rbuf_bv.bv_val = rbuf;
	rbuf_bv.bv_len = 0; /* not used */
	rc = ldap_parse_ldif_record( &rbuf_bv, linenum, &lr, prog, lrflags );

	/* If default controls are set (as with -M option) and controls are
	   specified in the LDIF file, we must add the default controls to
	   the list of controls sent with the ldap operation.
	*/
	if ( rc == 0 ) {
		if (lr.lr_ctrls) {
			LDAPControl **defctrls = NULL;   /* Default server controls */
			LDAPControl **newctrls = NULL;
			ldap_get_option(ld, LDAP_OPT_SERVER_CONTROLS, &defctrls);
			if (defctrls) {
				int npc=0;                       /* Num of LDIF controls */
				int ndefc=0;                     /* Num of default controls */
				while (lr.lr_ctrls[npc]) npc++;       /* Count LDIF controls */
				while (defctrls[ndefc]) ndefc++; /* Count default controls */
				newctrls = ber_memrealloc(lr.lr_ctrls,
					(npc+ndefc+1)*sizeof(LDAPControl*));

				if (newctrls == NULL) {
					rc = LDAP_NO_MEMORY;
				} else {
					int i;
					lr.lr_ctrls = newctrls;
					for (i=npc; i<npc+ndefc; i++) {
						lr.lr_ctrls[i] = ldap_control_dup(defctrls[i-npc]);
						if (lr.lr_ctrls[i] == NULL) {
							rc = LDAP_NO_MEMORY;
							break;
						}
					}
					lr.lr_ctrls[npc+ndefc] = NULL;
				}
				ldap_controls_free(defctrls);  /* Must be freed by library */
			}
		}
	}

	if ( rc == 0 ) {
		if ( LDAP_REQ_DELETE == lr.lr_op ) {
			rc = dodelete( &lr.lr_dn, lr.lr_ctrls );
		} else if ( LDAP_REQ_RENAME == lr.lr_op ) {
			rc = dorename( &lr.lr_dn, &lr.lrop_newrdn, &lr.lrop_newsup, lr.lrop_delold, lr.lr_ctrls );
		} else if ( ( LDAP_REQ_ADD == lr.lr_op ) || ( LDAP_REQ_MODIFY == lr.lr_op ) ) {
			rc = domodify( &lr.lr_dn, lr.lrop_mods, lr.lr_ctrls, LDAP_REQ_ADD == lr.lr_op );
		} else {
			/* record skipped e.g. version: or comment or something we don't handle yet */
		}

		if ( rc == LDAP_SUCCESS ) {
			rc = 0;
		}
	}

	ldap_ldif_record_done( &lr );

	return( rc );
}


static int
domodify(
	const struct berval *dn,
	LDAPMod **pmods,
	LDAPControl **pctrls,
	int newentry )
{
	int			rc, i, j, k, notascii, op;
	struct berval	*bvp;

	if ( ( dn == NULL ) || ( dn->bv_val == NULL ) ) {
		fprintf( stderr, _("%s: no DN specified\n"), prog );
		return( LDAP_PARAM_ERROR );
	}

	if ( pmods == NULL ) {
		/* implement "touch" (empty sequence)
		 * modify operation (note that there
		 * is no symmetry with the UNIX command,
		 * since \"touch\" on a non-existent entry
		 * will fail)*/
		printf( "warning: no attributes to %sadd (entry=\"%s\")\n",
			newentry ? "" : "change or ", dn->bv_val );

	} else {
		for ( i = 0; pmods[ i ] != NULL; ++i ) {
			op = pmods[ i ]->mod_op & ~LDAP_MOD_BVALUES;
			if( op == LDAP_MOD_ADD && ( pmods[i]->mod_bvalues == NULL )) {
				fprintf( stderr,
					_("%s: attribute \"%s\" has no values (entry=\"%s\")\n"),
					prog, pmods[i]->mod_type, dn->bv_val );
				return LDAP_PARAM_ERROR;
			}
		}

		if ( verbose ) {
			for ( i = 0; pmods[ i ] != NULL; ++i ) {
				op = pmods[ i ]->mod_op & ~LDAP_MOD_BVALUES;
				printf( "%s %s:\n",
					op == LDAP_MOD_REPLACE ? _("replace") :
						op == LDAP_MOD_ADD ?  _("add") :
							op == LDAP_MOD_INCREMENT ?  _("increment") :
								op == LDAP_MOD_DELETE ?  _("delete") :
									_("unknown"),
					pmods[ i ]->mod_type );
	
				if ( pmods[ i ]->mod_bvalues != NULL ) {
					for ( j = 0; pmods[ i ]->mod_bvalues[ j ] != NULL; ++j ) {
						bvp = pmods[ i ]->mod_bvalues[ j ];
						notascii = 0;
						for ( k = 0; (unsigned long) k < bvp->bv_len; ++k ) {
							if ( !isascii( bvp->bv_val[ k ] )) {
								notascii = 1;
								break;
							}
						}
						if ( notascii ) {
							printf( _("\tNOT ASCII (%ld bytes)\n"), bvp->bv_len );
						} else {
							printf( "\t%s\n", bvp->bv_val );
						}
					}
				}
			}
		}
	}

	if ( newentry ) {
		printf( "%sadding new entry \"%s\"\n", dont ? "!" : "", dn->bv_val );
	} else {
		printf( "%smodifying entry \"%s\"\n", dont ? "!" : "", dn->bv_val );
	}

	if ( !dont ) {
		int	msgid;
		if ( newentry ) {
			rc = ldap_add_ext( ld, dn->bv_val, pmods, pctrls, NULL, &msgid );
		} else {
			rc = ldap_modify_ext( ld, dn->bv_val, pmods, pctrls, NULL, &msgid );
		}

		if ( rc != LDAP_SUCCESS ) {
			/* print error message about failed update including DN */
			fprintf( stderr, _("%s: update failed: %s\n"), prog, dn->bv_val );
			tool_perror( newentry ? "ldap_add" : "ldap_modify",
				rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid,
			newentry ? LDAP_RES_ADD : LDAP_RES_MODIFY, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("modify complete\n") );
		}

	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return rc;
}


static int
dodelete(
	const struct berval *dn,
	LDAPControl **pctrls )
{
	int	rc;
	int msgid;

	assert( dn != NULL );
	assert( dn->bv_val != NULL );
	printf( _("%sdeleting entry \"%s\"\n"), dont ? "!" : "", dn->bv_val );
	if ( !dont ) {
		rc = ldap_delete_ext( ld, dn->bv_val, pctrls, NULL, &msgid );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, _("%s: delete failed: %s\n"), prog, dn->bv_val );
			tool_perror( "ldap_delete", rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid, LDAP_RES_DELETE, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("delete complete\n") );
		}
	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return( rc );
}


static int
dorename(
	const struct berval *dn,
	const struct berval *newrdn,
	const struct berval *newsup,
	int deleteoldrdn,
	LDAPControl **pctrls )
{
	int	rc;
	int msgid;

	assert( dn != NULL );
	assert( dn->bv_val != NULL );
	assert( newrdn != NULL );
	assert( newrdn->bv_val != NULL );
	printf( _("%smodifying rdn of entry \"%s\"\n"), dont ? "!" : "", dn->bv_val );
	if ( verbose ) {
		printf( _("\tnew RDN: \"%s\" (%skeep existing values)\n"),
			newrdn->bv_val, deleteoldrdn ? _("do not ") : "" );
	}
	if ( !dont ) {
		rc = ldap_rename( ld, dn->bv_val, newrdn->bv_val,
						  ( newsup && newsup->bv_val ) ? newsup->bv_val : NULL,
						  deleteoldrdn, pctrls, NULL, &msgid );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, _("%s: rename failed: %s\n"), prog, dn->bv_val );
			tool_perror( "ldap_rename", rc, NULL, NULL, NULL, NULL );
			goto done;
		}
		rc = process_response( ld, msgid, LDAP_RES_RENAME, dn );

		if ( verbose && rc == LDAP_SUCCESS ) {
			printf( _("rename complete\n") );
		}
	} else {
		rc = LDAP_SUCCESS;
	}

done:
	putchar( '\n' );
	return( rc );
}

static const char *
res2str( int res ) {
	switch ( res ) {
	case LDAP_RES_ADD:
		return "ldap_add";
	case LDAP_RES_DELETE:
		return "ldap_delete";
	case LDAP_RES_MODIFY:
		return "ldap_modify";
	case LDAP_RES_MODRDN:
		return "ldap_rename";
	default:
		assert( 0 );
	}

	return "ldap_unknown";
}

static int process_response(
	LDAP *ld,
	int msgid,
	int op,
	const struct berval *dn )
{
	LDAPMessage	*res;
	int		rc = LDAP_OTHER, msgtype;
	struct timeval	tv = { 0, 0 };
	int		err;
	char		*text = NULL, *matched = NULL, **refs = NULL;
	LDAPControl	**ctrls = NULL;

	assert( dn != NULL );
	for ( ; ; ) {
		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		rc = ldap_result( ld, msgid, LDAP_MSG_ALL, &tv, &res );
		if ( tool_check_abandon( ld, msgid ) ) {
			return LDAP_CANCELLED;
		}

		if ( rc == -1 ) {
			ldap_get_option( ld, LDAP_OPT_RESULT_CODE, &rc );
			tool_perror( "ldap_result", rc, NULL, NULL, NULL, NULL );
			return rc;
		}

		if ( rc != 0 ) {
			break;
		}
	}

	msgtype = ldap_msgtype( res );

	rc = ldap_parse_result( ld, res, &err, &matched, &text, &refs, &ctrls, 1 );
	if ( rc == LDAP_SUCCESS ) rc = err;

#ifdef LDAP_X_TXN
	if ( rc == LDAP_X_TXN_SPECIFY_OKAY ) {
		rc = LDAP_SUCCESS;
	} else
#endif
	if ( rc != LDAP_SUCCESS ) {
		tool_perror( res2str( op ), rc, NULL, matched, text, refs );
	} else if ( msgtype != op ) {
		fprintf( stderr, "%s: msgtype: expected %d got %d\n",
			res2str( op ), op, msgtype );
		rc = LDAP_OTHER;
	}

	if ( text ) ldap_memfree( text );
	if ( matched ) ldap_memfree( matched );
	if ( refs ) ber_memvfree( (void **)refs );

	if ( ctrls ) {
		tool_print_ctrls( ld, ctrls );
		ldap_controls_free( ctrls );
	}

	return rc;
}
