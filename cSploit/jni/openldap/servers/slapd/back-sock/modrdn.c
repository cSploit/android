/* modrdn.c - sock backend modrdn function */
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

int
sock_back_modrdn(
    Operation	*op,
    SlapReply	*rs )
{
	struct sockinfo	*si = (struct sockinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	Entry e;
	FILE			*fp;

	e.e_id = NOID;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	e.e_attrs = NULL;
	e.e_ocflags = 0;
	e.e_bv.bv_len = 0;
	e.e_bv.bv_val = NULL;
	e.e_private = NULL;

	if ( ! access_allowed( op, &e, entry, NULL,
			op->oq_modrdn.rs_newSup ? ACL_WDEL : ACL_WRITE,
			NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( (fp = opensock( si->si_sockpath )) == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not open socket" );
		return( -1 );
	}

	/* write out the request to the modrdn process */
	fprintf( fp, "MODRDN\n" );
	fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
	sock_print_conn( fp, op->o_conn, si );
	sock_print_suffixes( fp, op->o_bd );
	fprintf( fp, "dn: %s\n", op->o_req_dn.bv_val );
	fprintf( fp, "newrdn: %s\n", op->oq_modrdn.rs_newrdn.bv_val );
	fprintf( fp, "deleteoldrdn: %d\n", op->oq_modrdn.rs_deleteoldrdn ? 1 : 0 );
	if ( op->oq_modrdn.rs_newSup != NULL ) {
		fprintf( fp, "newSuperior: %s\n", op->oq_modrdn.rs_newSup->bv_val );
	}
	fprintf( fp, "\n" );

	/* read in the results and send them along */
	sock_read_and_send_results( op, rs, fp );
	fclose( fp );
	return( 0 );
}
