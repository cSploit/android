/* search.c - sock backend search function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-sock.h"

/*
 * FIXME: add a filterSearchResults option like back-perl has
 */

int
sock_back_search(
    Operation	*op,
    SlapReply	*rs )
{
	struct sockinfo	*si = (struct sockinfo *) op->o_bd->be_private;
	FILE			*fp;
	AttributeName		*an;

	if ( (fp = opensock( si->si_sockpath )) == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not open socket" );
		return( -1 );
	}

	/* write out the request to the search process */
	fprintf( fp, "SEARCH\n" );
	fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
	sock_print_conn( fp, op->o_conn, si );
	sock_print_suffixes( fp, op->o_bd );
	fprintf( fp, "base: %s\n", op->o_req_dn.bv_val );
	fprintf( fp, "scope: %d\n", op->oq_search.rs_scope );
	fprintf( fp, "deref: %d\n", op->oq_search.rs_deref );
	fprintf( fp, "sizelimit: %d\n", op->oq_search.rs_slimit );
	fprintf( fp, "timelimit: %d\n", op->oq_search.rs_tlimit );
	fprintf( fp, "filter: %s\n", op->oq_search.rs_filterstr.bv_val );
	fprintf( fp, "attrsonly: %d\n", op->oq_search.rs_attrsonly ? 1 : 0 );
	fprintf( fp, "attrs:%s", op->oq_search.rs_attrs == NULL ? " all" : "" );
	for ( an = op->oq_search.rs_attrs; an && an->an_name.bv_val; an++ ) {
		fprintf( fp, " %s", an->an_name.bv_val );
	}
	fprintf( fp, "\n\n" );  /* end of attr line plus blank line */

	/* read in the results and send them along */
	rs->sr_attrs = op->oq_search.rs_attrs;
	sock_read_and_send_results( op, rs, fp );

	fclose( fp );
	return( 0 );
}
