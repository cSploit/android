/* add.c - sock backend add function */
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

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-sock.h"

int
sock_back_add(
    Operation	*op,
    SlapReply	*rs )
{
	struct sockinfo	*si = (struct sockinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	FILE			*fp;
	int			len;

	if ( ! access_allowed( op, op->oq_add.rs_e,
		entry, NULL, ACL_WADD, NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( (fp = opensock( si->si_sockpath )) == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not open socket" );
		return( -1 );
	}

	/* write out the request to the add process */
	fprintf( fp, "ADD\n" );
	fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
	sock_print_conn( fp, op->o_conn, si );
	sock_print_suffixes( fp, op->o_bd );
	ldap_pvt_thread_mutex_lock( &entry2str_mutex );
	fprintf( fp, "%s", entry2str( op->oq_add.rs_e, &len ) );
	ldap_pvt_thread_mutex_unlock( &entry2str_mutex );
	fprintf (fp, "\n" );

	/* read in the result and send it along */
	sock_read_and_send_results( op, rs, fp );

	fclose( fp );
	return( 0 );
}
