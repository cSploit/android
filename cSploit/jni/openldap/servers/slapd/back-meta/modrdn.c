/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

int
meta_back_modrdn( Operation *op, SlapReply *rs )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t	*mt;
	metaconn_t	*mc;
	int		candidate = -1;
	struct berval	mdn = BER_BVNULL,
			mnewSuperior = BER_BVNULL;
	dncookie	dc;
	int		msgid;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	LDAPControl	**ctrls = NULL;
	struct berval	newrdn = BER_BVNULL;

	mc = meta_back_getconn( op, rs, &candidate, LDAP_BACK_SENDERR );
	if ( !mc || !meta_back_dobind( op, rs, mc, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	assert( mc->mc_conns[ candidate ].msc_ld != NULL );

	mt = mi->mi_targets[ candidate ];
	dc.target = mt;
	dc.conn = op->o_conn;
	dc.rs = rs;

	if ( op->orr_newSup ) {

		/*
		 * NOTE: the newParent, if defined, must be on the 
		 * same target as the entry to be renamed.  This check
		 * has been anticipated in meta_back_getconn()
		 */
		/*
		 * FIXME: one possibility is to delete the entry
		 * from one target and add it to the other;
		 * unfortunately we'd need write access to both,
		 * which is nearly impossible; for administration
		 * needs, the rootdn of the metadirectory could
		 * be mapped to an administrative account on each
		 * target (the binddn?); we'll see.
		 */
		/*
		 * NOTE: we need to port the identity assertion
		 * feature from back-ldap
		 */

		/* needs LDAPv3 */
		switch ( mt->mt_version ) {
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
		
		/*
		 * Rewrite the new superior, if defined and required
	 	 */
		dc.ctx = "newSuperiorDN";
		if ( ldap_back_dn_massage( &dc, op->orr_newSup, &mnewSuperior ) ) {
			rs->sr_err = LDAP_OTHER;
			send_ldap_result( op, rs );
			goto cleanup;
		}
	}

	/*
	 * Rewrite the modrdn dn, if required
	 */
	dc.ctx = "modrDN";
	if ( ldap_back_dn_massage( &dc, &op->o_req_dn, &mdn ) ) {
		rs->sr_err = LDAP_OTHER;
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* NOTE: we need to copy the newRDN in case it was formed
	 * from a DN by simply changing the length (ITS#5397) */
	newrdn = op->orr_newrdn;
	if ( newrdn.bv_val[ newrdn.bv_len ] != '\0' ) {
		ber_dupbv_x( &newrdn, &op->orr_newrdn, op->o_tmpmemctx );
	}

retry:;
	ctrls = op->o_ctrls;
	if ( meta_back_controls_add( op, rs, mc, candidate, &ctrls ) != LDAP_SUCCESS )
	{
		send_ldap_result( op, rs );
		goto cleanup;
	}

	rs->sr_err = ldap_rename( mc->mc_conns[ candidate ].msc_ld,
			mdn.bv_val, newrdn.bv_val,
			mnewSuperior.bv_val, op->orr_deleteoldrdn,
			ctrls, NULL, &msgid );
	rs->sr_err = meta_back_op_result( mc, op, rs, candidate, msgid,
		mt->mt_timeout[ SLAP_OP_MODRDN ], ( LDAP_BACK_SENDRESULT | retrying ) );
	if ( rs->sr_err == LDAP_UNAVAILABLE && retrying ) {
		retrying &= ~LDAP_BACK_RETRYING;
		if ( meta_back_retry( op, rs, &mc, candidate, LDAP_BACK_SENDERR ) ) {
			/* if the identity changed, there might be need to re-authz */
			(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );
			goto retry;
		}
	}

cleanup:;
	(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );

	if ( mdn.bv_val != op->o_req_dn.bv_val ) {
		free( mdn.bv_val );
		BER_BVZERO( &mdn );
	}
	
	if ( !BER_BVISNULL( &mnewSuperior )
			&& mnewSuperior.bv_val != op->orr_newSup->bv_val )
	{
		free( mnewSuperior.bv_val );
		BER_BVZERO( &mnewSuperior );
	}

	if ( newrdn.bv_val != op->orr_newrdn.bv_val ) {
		op->o_tmpfree( newrdn.bv_val, op->o_tmpmemctx );
	}

	if ( mc ) {
		meta_back_release_conn( mi, mc );
	}

	return rs->sr_err;
}

