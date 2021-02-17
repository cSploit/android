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

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

int
meta_back_delete( Operation *op, SlapReply *rs )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t	*mt;
	metaconn_t	*mc = NULL;
	int		candidate = -1;
	struct berval	mdn = BER_BVNULL;
	dncookie	dc;
	int		msgid;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	LDAPControl	**ctrls = NULL;

	mc = meta_back_getconn( op, rs, &candidate, LDAP_BACK_SENDERR );
	if ( !mc || !meta_back_dobind( op, rs, mc, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	assert( mc->mc_conns[ candidate ].msc_ld != NULL );

	/*
	 * Rewrite the compare dn, if needed
	 */
	mt = mi->mi_targets[ candidate ];
	dc.target = mt;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "deleteDN";

	if ( ldap_back_dn_massage( &dc, &op->o_req_dn, &mdn ) ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

retry:;
	ctrls = op->o_ctrls;
	if ( meta_back_controls_add( op, rs, mc, candidate, &ctrls ) != LDAP_SUCCESS )
	{
		send_ldap_result( op, rs );
		goto cleanup;
	}

	rs->sr_err = ldap_delete_ext( mc->mc_conns[ candidate ].msc_ld,
			mdn.bv_val, ctrls, NULL, &msgid );
	rs->sr_err = meta_back_op_result( mc, op, rs, candidate, msgid,
		mt->mt_timeout[ SLAP_OP_DELETE ], ( LDAP_BACK_SENDRESULT | retrying ) );
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
	
	if ( mc ) {
		meta_back_release_conn( mi, mc );
	}

	return rs->sr_err;
}

