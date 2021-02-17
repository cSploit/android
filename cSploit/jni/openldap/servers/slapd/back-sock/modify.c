/* modify.c - sock backend modify function */
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
#include "ldif.h"

int
sock_back_modify(
    Operation	*op,
    SlapReply	*rs )
{
	Modification *mod;
	struct sockinfo	*si = (struct sockinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	Modifications *ml  = op->orm_modlist;
	Entry e;
	FILE			*fp;
	int			i;

	e.e_id = NOID;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	e.e_attrs = NULL;
	e.e_ocflags = 0;
	e.e_bv.bv_len = 0;
	e.e_bv.bv_val = NULL;
	e.e_private = NULL;

	if ( ! access_allowed( op, &e,
		entry, NULL, ACL_WRITE, NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( (fp = opensock( si->si_sockpath )) == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not open socket" );
		return( -1 );
	}

	/* write out the request to the modify process */
	fprintf( fp, "MODIFY\n" );
	fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
	sock_print_conn( fp, op->o_conn, si );
	sock_print_suffixes( fp, op->o_bd );
	fprintf( fp, "dn: %s\n", op->o_req_dn.bv_val );
	for ( ; ml != NULL; ml = ml->sml_next ) {
		mod = &ml->sml_mod;

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			fprintf( fp, "add: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;

		case LDAP_MOD_DELETE:
			fprintf( fp, "delete: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;

		case LDAP_MOD_REPLACE:
			fprintf( fp, "replace: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;
		}

		if( mod->sm_values != NULL ) {
			for ( i = 0; mod->sm_values[i].bv_val != NULL; i++ ) {
				char *text = ldif_put_wrap( LDIF_PUT_VALUE,
					mod->sm_desc->ad_cname.bv_val,
					mod->sm_values[i].bv_val,
					mod->sm_values[i].bv_len, LDIF_LINE_WIDTH_MAX );
				if ( text ) {
					fprintf( fp, "%s", text );
					ber_memfree( text );
				} else {
					break;
				}
			}
		}

		fprintf( fp, "-\n" );
	}
	fprintf( fp, "\n" );

	/* read in the results and send them along */
	sock_read_and_send_results( op, rs, fp );
	fclose( fp );
	return( 0 );
}
