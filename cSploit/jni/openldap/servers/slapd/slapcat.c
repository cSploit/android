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
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slapcommon.h"
#include "ldif.h"

static volatile sig_atomic_t gotsig;

static RETSIGTYPE
slapcat_sig( int sig )
{
	gotsig=1;
}

int
slapcat( int argc, char **argv )
{
	ID id;
	int rc = EXIT_SUCCESS;
	Operation op = {0};
	const char *progname = "slapcat";
	int requestBSF;
	int doBSF = 0;

	slap_tool_init( progname, SLAPCAT, argc, argv );

	requestBSF = ( sub_ndn.bv_len || filter );

#ifdef SIGPIPE
	(void) SIGNAL( SIGPIPE, slapcat_sig );
#endif
#ifdef SIGHUP
	(void) SIGNAL( SIGHUP, slapcat_sig );
#endif
	(void) SIGNAL( SIGINT, slapcat_sig );
	(void) SIGNAL( SIGTERM, slapcat_sig );

	if( !be->be_entry_open ||
		!be->be_entry_close ||
		!( be->be_entry_first_x || be->be_entry_first ) ||
		!be->be_entry_next ||
		!be->be_entry_get )
	{
		fprintf( stderr, "%s: database doesn't support necessary operations.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	if( be->be_entry_open( be, 0 ) != 0 ) {
		fprintf( stderr, "%s: could not open database.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	op.o_bd = be;
	if ( !requestBSF && be->be_entry_first ) {
		id = be->be_entry_first( be );

	} else {
		if ( be->be_entry_first_x ) {
			id = be->be_entry_first_x( be,
				sub_ndn.bv_len ? &sub_ndn : NULL, scope, filter );

		} else {
			assert( be->be_entry_first != NULL );
			doBSF = 1;
			id = be->be_entry_first( be );
		}
	}

	for ( ; id != NOID; id = be->be_entry_next( be ) )
	{
		char *data;
		int len;
		Entry* e;

		if ( gotsig )
			break;

		e = be->be_entry_get( be, id );
		if ( e == NULL ) {
			printf("# no data for entry id=%08lx\n\n", (long) id );
			rc = EXIT_FAILURE;
			if ( continuemode == 0 ) {
				break;

			} else if ( continuemode == 1 ) {
				continue;
			}

			/* this is a last resort: linearly scan all ids
			 * trying to recover as much as possible (ITS#6482) */
			while ( ++id != NOID ) {
				e = be->be_entry_get( be, id );
				if ( e != NULL ) break;
				printf("# no data for entry id=%08lx\n\n", (long) id );
			}

			if ( e == NULL ) break;
		}

		if ( doBSF ) {
			if ( sub_ndn.bv_len && !dnIsSuffixScope( &e->e_nname, &sub_ndn, scope ) )
			{
				be_entry_release_r( &op, e );
				continue;
			}


			if ( filter != NULL ) {
				int rc = test_filter( NULL, e, filter );
				if ( rc != LDAP_COMPARE_TRUE ) {
					be_entry_release_r( &op, e );
					continue;
				}
			}
		}

		if ( verbose ) {
			printf( "# id=%08lx\n", (long) id );
		}

		data = entry2str_wrap( e, &len, ldif_wrap );
		be_entry_release_r( &op, e );

		if ( data == NULL ) {
			printf("# bad data for entry id=%08lx\n\n", (long) id );
			rc = EXIT_FAILURE;
			if( continuemode ) continue;
			break;
		}

		if ( fputs( data, ldiffp->fp ) == EOF ||
			fputs( "\n", ldiffp->fp ) == EOF ) {
			fprintf(stderr, "%s: error writing output.\n",
				progname);
			rc = EXIT_FAILURE;
			break;
		}
	}

	be->be_entry_close( be );

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;
	return rc;
}
