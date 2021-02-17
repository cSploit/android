/* bind.c - shell backend bind function */
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
shell_back_bind(
    Operation		*op,
    SlapReply		*rs )
{
	struct shellinfo	*si = (struct shellinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	Entry e;
	FILE			*rfp, *wfp;
	int			rc;

	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		return rs->sr_err;
	}

	if ( si->si_bind == NULL ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		    "bind not implemented" );
		return( -1 );
	}

	e.e_id = NOID;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	e.e_attrs = NULL;
	e.e_ocflags = 0;
	e.e_bv.bv_len = 0;
	e.e_bv.bv_val = NULL;
	e.e_private = NULL;

	if ( ! access_allowed( op, &e,
		entry, NULL, ACL_AUTH, NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( forkandexec( si->si_bind, &rfp, &wfp ) == (pid_t)-1 ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not fork/exec" );
		return( -1 );
	}

	/* write out the request to the bind process */
	fprintf( wfp, "BIND\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, op->o_bd );
	fprintf( wfp, "dn: %s\n", op->o_req_dn.bv_val );
	fprintf( wfp, "method: %d\n", op->oq_bind.rb_method );
	fprintf( wfp, "credlen: %lu\n", op->oq_bind.rb_cred.bv_len );
	fprintf( wfp, "cred: %s\n", op->oq_bind.rb_cred.bv_val ); /* XXX */
	fclose( wfp );

	/* read in the results and send them along */
	rc = read_and_send_results( op, rs, rfp );
	fclose( rfp );

	return( rc );
}
