/* modify.c - shell backend modify function */
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

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"
#include "ldif.h"

int
shell_back_modify(
    Operation	*op,
    SlapReply	*rs )
{
	Modification *mod;
	struct shellinfo	*si = (struct shellinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	Modifications *ml  = op->orm_modlist;
	Entry e;
	FILE			*rfp, *wfp;
	int			i;

	if ( si->si_modify == NULL ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		    "modify not implemented" );
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
		entry, NULL, ACL_WRITE, NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( forkandexec( si->si_modify, &rfp, &wfp ) == (pid_t)-1 ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not fork/exec" );
		return( -1 );
	}

	/* write out the request to the modify process */
	fprintf( wfp, "MODIFY\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, op->o_bd );
	fprintf( wfp, "dn: %s\n", op->o_req_dn.bv_val );
	for ( ; ml != NULL; ml = ml->sml_next ) {
		mod = &ml->sml_mod;

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			fprintf( wfp, "add: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;

		case LDAP_MOD_DELETE:
			fprintf( wfp, "delete: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;

		case LDAP_MOD_REPLACE:
			fprintf( wfp, "replace: %s\n", mod->sm_desc->ad_cname.bv_val );
			break;
		}

		if( mod->sm_values != NULL ) {
			for ( i = 0; mod->sm_values[i].bv_val != NULL; i++ ) {
				char *out = ldif_put( LDIF_PUT_VALUE,
					mod->sm_desc->ad_cname.bv_val,
					mod->sm_values[i].bv_val,
					mod->sm_values[i].bv_len );
				if ( out ) {
					fprintf( wfp, "%s", out );
					ber_memfree( out );
				}
			}
		}

		fprintf( wfp, "-\n" );
	}
	fclose( wfp );

	/* read in the results and send them along */
	read_and_send_results( op, rs, rfp );
	fclose( rfp );
	return( 0 );
}
