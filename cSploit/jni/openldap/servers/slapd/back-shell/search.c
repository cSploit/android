/* search.c - shell backend search function */
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

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "shell.h"

int
shell_back_search(
    Operation	*op,
    SlapReply	*rs )
{
	struct shellinfo	*si = (struct shellinfo *) op->o_bd->be_private;
	FILE			*rfp, *wfp;
	AttributeName		*an;

	if ( si->si_search == NULL ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		    "search not implemented" );
		return( -1 );
	}

	if ( forkandexec( si->si_search, &rfp, &wfp ) == (pid_t)-1 ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not fork/exec" );
		return( -1 );
	}

	/* write out the request to the search process */
	fprintf( wfp, "SEARCH\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, op->o_bd );
	fprintf( wfp, "base: %s\n", op->o_req_dn.bv_val );
	fprintf( wfp, "scope: %d\n", op->oq_search.rs_scope );
	fprintf( wfp, "deref: %d\n", op->oq_search.rs_deref );
	fprintf( wfp, "sizelimit: %d\n", op->oq_search.rs_slimit );
	fprintf( wfp, "timelimit: %d\n", op->oq_search.rs_tlimit );
	fprintf( wfp, "filter: %s\n", op->oq_search.rs_filterstr.bv_val );
	fprintf( wfp, "attrsonly: %d\n", op->oq_search.rs_attrsonly ? 1 : 0 );
	fprintf( wfp, "attrs:%s", op->oq_search.rs_attrs == NULL ? " all" : "" );
	for ( an = op->oq_search.rs_attrs; an && an->an_name.bv_val; an++ ) {
		fprintf( wfp, " %s", an->an_name.bv_val );
	}
	fprintf( wfp, "\n" );
	fclose( wfp );

	/* read in the results and send them along */
	rs->sr_attrs = op->oq_search.rs_attrs;
	read_and_send_results( op, rs, rfp );

	fclose( rfp );
	return( 0 );
}
