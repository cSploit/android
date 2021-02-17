/* modrdn.c - ldap backend modrdn function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-ldap.h"

int
ldap_back_modrdn(
		Operation	*op,
 		SlapReply	*rs )
{
	ldapinfo_t		*li = (ldapinfo_t *)op->o_bd->be_private;

	ldapconn_t		*lc = NULL;
	ber_int_t		msgid;
	LDAPControl		**ctrls = NULL;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	int			rc = LDAP_SUCCESS;
	char			*newSup = NULL;
	struct berval		newrdn = BER_BVNULL;

	if ( !ldap_back_dobind( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	if ( op->orr_newSup ) {
		/* needs LDAPv3 */
		switch ( li->li_version ) {
		case LDAP_VERSION3:
			break;

		case 0:
			if ( op->o_protocol == 0 || op->o_protocol == LDAP_VERSION3 ) {
				break;
			}
			/* fall thru */

		default:
			/* op->o_protocol cannot be anything but LDAPv3,
			 * otherwise wouldn't be here */
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			send_ldap_result( op, rs );
			goto cleanup;
		}
		
		newSup = op->orr_newSup->bv_val;
	}

	/* NOTE: we need to copy the newRDN in case it was formed
	 * from a DN by simply changing the length (ITS#5397) */
	newrdn = op->orr_newrdn;
	if ( newrdn.bv_val[ newrdn.bv_len ] != '\0' ) {
		ber_dupbv_x( &newrdn, &op->orr_newrdn, op->o_tmpmemctx );
	}

retry:
	ctrls = op->o_ctrls;
	rc = ldap_back_controls_add( op, rs, lc, &ctrls );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		rc = -1;
		goto cleanup;
	}

	rs->sr_err = ldap_rename( lc->lc_ld, op->o_req_dn.bv_val,
			newrdn.bv_val, newSup,
			op->orr_deleteoldrdn, ctrls, NULL, &msgid );
	rc = ldap_back_op_result( lc, op, rs, msgid,
		li->li_timeout[ SLAP_OP_MODRDN ],
		( LDAP_BACK_SENDRESULT | retrying ) );
	if ( rs->sr_err == LDAP_UNAVAILABLE && retrying ) {
		retrying &= ~LDAP_BACK_RETRYING;
		if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
			/* if the identity changed, there might be need to re-authz */
			(void)ldap_back_controls_free( op, rs, &ctrls );
			goto retry;
		}
	}

	ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
	ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_MODRDN ], 1 );
	ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

cleanup:
	(void)ldap_back_controls_free( op, rs, &ctrls );

	if ( newrdn.bv_val != op->orr_newrdn.bv_val ) {
		op->o_tmpfree( newrdn.bv_val, op->o_tmpmemctx );
	}

	if ( lc != NULL ) {
		ldap_back_release_conn( li, lc );
	}

	return rc;
}

