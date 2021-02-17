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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#include "lutil.h"

int
do_delete(
    Operation	*op,
    SlapReply	*rs )
{
	struct berval dn = BER_BVNULL;

	Debug( LDAP_DEBUG_TRACE, "%s do_delete\n",
		op->o_log_prefix, 0, 0 );
	/*
	 * Parse the delete request.  It looks like this:
	 *
	 *	DelRequest := DistinguishedName
	 */

	if ( ber_scanf( op->o_ber, "m", &dn ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_delete: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	if( get_ctrls( op, rs, 1 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_delete: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		goto cleanup;
	} 

	rs->sr_err = dnPrettyNormal( NULL, &dn, &op->o_req_dn, &op->o_req_ndn,
		op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_delete: invalid dn (%s)\n",
			op->o_log_prefix, dn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid DN" );
		goto cleanup;
	}

	Statslog( LDAP_DEBUG_STATS, "%s DEL dn=\"%s\"\n",
		op->o_log_prefix, op->o_req_dn.bv_val, 0, 0, 0 );

	if( op->o_req_ndn.bv_len == 0 ) {
		Debug( LDAP_DEBUG_ANY, "%s do_delete: root dse!\n",
			op->o_log_prefix, 0, 0 );
		/* protocolError would likely be a more appropriate error */
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"cannot delete the root DSE" );
		goto cleanup;

	} else if ( bvmatch( &op->o_req_ndn, &frontendDB->be_schemandn ) ) {
		Debug( LDAP_DEBUG_ANY, "%s do_delete: subschema subentry!\n",
			op->o_log_prefix, 0, 0 );
		/* protocolError would likely be a more appropriate error */
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"cannot delete the root DSE" );
		goto cleanup;
	}

	op->o_bd = frontendDB;
	rs->sr_err = frontendDB->be_delete( op, rs );

#ifdef LDAP_X_TXN
	if( rs->sr_err == LDAP_X_TXN_SPECIFY_OKAY ) {
		/* skip cleanup */
		return rs->sr_err;
	}
#endif

cleanup:;
	op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
	return rs->sr_err;
}

int
fe_op_delete( Operation *op, SlapReply *rs )
{
	struct berval	pdn = BER_BVNULL;
	BackendDB	*op_be, *bd = op->o_bd;
	
	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	op->o_bd = select_backend( &op->o_req_ndn, 1 );
	if ( op->o_bd == NULL ) {
		op->o_bd = bd;
		rs->sr_ref = referral_rewrite( default_referral,
			NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );

		if (!rs->sr_ref) rs->sr_ref = default_referral;
		if ( rs->sr_ref != NULL ) {
			rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );

			if (rs->sr_ref != default_referral) ber_bvarray_free( rs->sr_ref );
		} else {
			send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
				"no global superior knowledge" );
		}
		goto cleanup;
	}

	/* If we've got a glued backend, check the real backend */
	op_be = op->o_bd;
	if ( SLAP_GLUE_INSTANCE( op->o_bd )) {
		op->o_bd = select_backend( &op->o_req_ndn, 0 );
	}

	/* check restrictions */
	if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* check for referrals */
	if( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		goto cleanup;
	}

	/*
	 * do the delete if 1 && (2 || 3)
	 * 1) there is a delete function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the update_ndn.
	 */
	if ( op->o_bd->be_delete ) {
		/* do the update here */
		int repl_user = be_isupdate( op );
		if ( !SLAP_SINGLE_SHADOW(op->o_bd) || repl_user ) {
			struct berval	org_req_dn = BER_BVNULL;
			struct berval	org_req_ndn = BER_BVNULL;
			struct berval	org_dn = BER_BVNULL;
			struct berval	org_ndn = BER_BVNULL;
			int		org_managedsait;

			op->o_bd = op_be;
			op->o_bd->be_delete( op, rs );

			org_req_dn = op->o_req_dn;
			org_req_ndn = op->o_req_ndn;
			org_dn = op->o_dn;
			org_ndn = op->o_ndn;
			org_managedsait = get_manageDSAit( op );
			op->o_dn = op->o_bd->be_rootdn;
			op->o_ndn = op->o_bd->be_rootndn;
			op->o_managedsait = SLAP_CONTROL_NONCRITICAL;

			while ( rs->sr_err == LDAP_SUCCESS &&
				op->o_delete_glue_parent )
			{
				op->o_delete_glue_parent = 0;
				if ( !be_issuffix( op->o_bd, &op->o_req_ndn )) {
					slap_callback cb = { NULL, NULL, NULL, NULL };
					cb.sc_response = slap_null_cb;
					dnParent( &op->o_req_ndn, &pdn );
					op->o_req_dn = pdn;
					op->o_req_ndn = pdn;
					op->o_callback = &cb;
					op->o_bd->be_delete( op, rs );
				} else {
					break;
				}
			}

			op->o_managedsait = org_managedsait;
			op->o_dn = org_dn;
			op->o_ndn = org_ndn;
			op->o_req_dn = org_req_dn;
			op->o_req_ndn = org_req_ndn;
			op->o_delete_glue_parent = 0;

		} else {
			BerVarray defref = op->o_bd->be_update_refs
				? op->o_bd->be_update_refs : default_referral;

			if ( defref != NULL ) {
				rs->sr_ref = referral_rewrite( defref,
					NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
				if (!rs->sr_ref) rs->sr_ref = defref;
				rs->sr_err = LDAP_REFERRAL;
				send_ldap_result( op, rs );

				if (rs->sr_ref != defref) ber_bvarray_free( rs->sr_ref );

			} else {
				send_ldap_error( op, rs,
					LDAP_UNWILLING_TO_PERFORM,
					"shadow context; no update referral" );
			}
		}

	} else {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"operation not supported within namingContext" );
	}

cleanup:;
	op->o_bd = bd;
	return rs->sr_err;
}
