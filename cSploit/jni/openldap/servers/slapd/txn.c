/* txn.c - LDAP Transactions */
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

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "slap.h"

#include <lber_pvt.h>
#include <lutil.h>

#ifdef LDAP_X_TXN
const struct berval slap_EXOP_TXN_START = BER_BVC(LDAP_EXOP_X_TXN_START);
const struct berval slap_EXOP_TXN_END = BER_BVC(LDAP_EXOP_X_TXN_END);

int txn_start_extop(
	Operation *op, SlapReply *rs )
{
	int rc;
	struct berval *bv;

	Statslog( LDAP_DEBUG_STATS, "%s TXN START\n",
		op->o_log_prefix, 0, 0, 0, 0 );

	if( op->ore_reqdata != NULL ) {
		rs->sr_text = "no request data expected";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_bd = op->o_conn->c_authz_backend;
	if( backend_check_restrictions( op, rs,
		(struct berval *)&slap_EXOP_TXN_START ) != LDAP_SUCCESS )
	{
		return rs->sr_err;
	}

	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	if( op->o_conn->c_txn != CONN_TXN_INACTIVE ) {
		rs->sr_text = "Too many transactions";
		rc = LDAP_BUSY;
		goto done;
	}

	assert( op->o_conn->c_txn_backend == NULL );
	op->o_conn->c_txn = CONN_TXN_SPECIFY;

	bv = (struct berval *) ch_malloc( sizeof (struct berval) );
	bv->bv_len = 0;
	bv->bv_val = NULL;

	rs->sr_rspdata = bv;
	rc = LDAP_SUCCESS;

done:
	/* release connection lock */
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
	return rc;
}

int txn_spec_ctrl(
	Operation *op, SlapReply *rs, LDAPControl *ctrl )
{
	if ( !ctrl->ldctl_iscritical ) {
		rs->sr_text = "txnSpec control must be marked critical";
		return LDAP_PROTOCOL_ERROR;
	}
	if( op->o_txnSpec ) {
		rs->sr_text = "txnSpec control provided multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( ctrl->ldctl_value.bv_val == NULL ) {
		rs->sr_text = "no transaction identifier provided";
		return LDAP_PROTOCOL_ERROR;
	}
	if ( ctrl->ldctl_value.bv_len != 0 ) {
		rs->sr_text = "invalid transaction identifier";
		return LDAP_X_TXN_ID_INVALID;
	}

	if ( op->o_preread ) { /* temporary limitation */
		rs->sr_text = "cannot perform pre-read in transaction";
		return LDAP_UNWILLING_TO_PERFORM;
	} 
	if ( op->o_postread ) { /* temporary limitation */
		rs->sr_text = "cannot perform post-read in transaction";
		return LDAP_UNWILLING_TO_PERFORM;
	}

	op->o_txnSpec = SLAP_CONTROL_CRITICAL;
	return LDAP_SUCCESS;
}

int txn_end_extop(
	Operation *op, SlapReply *rs )
{
	int rc;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t commit=1;
	struct berval txnid;

	Statslog( LDAP_DEBUG_STATS, "%s TXN END\n",
		op->o_log_prefix, 0, 0, 0, 0 );

	if( op->ore_reqdata == NULL ) {
		rs->sr_text = "request data expected";
		return LDAP_PROTOCOL_ERROR;
	}
	if( op->ore_reqdata->bv_len == 0 ) {
		rs->sr_text = "empty request data";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_bd = op->o_conn->c_authz_backend;
	if( backend_check_restrictions( op, rs,
		(struct berval *)&slap_EXOP_TXN_END ) != LDAP_SUCCESS )
	{
		return rs->sr_err;
	}

	ber_init2( ber, op->ore_reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );
	if( tag == LBER_ERROR ) {
		rs->sr_text = "request data decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	tag = ber_peek_tag( ber, &len );
	if( tag == LBER_BOOLEAN ) {
		tag = ber_scanf( ber, "b", &commit );
		if( tag == LBER_ERROR ) {
			rs->sr_text = "request data decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
	}

	tag = ber_scanf( ber, /*{*/ "m}", &txnid );
	if( tag == LBER_ERROR ) {
		rs->sr_text = "request data decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	if( txnid.bv_len ) {
		rs->sr_text = "invalid transaction identifier";
		return LDAP_X_TXN_ID_INVALID;
	}

	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	if( op->o_conn->c_txn != CONN_TXN_SPECIFY ) {
		rs->sr_text = "invalid transaction identifier";
		rc = LDAP_X_TXN_ID_INVALID;
		goto done;
	}
	op->o_conn->c_txn = CONN_TXN_SETTLE;

	if( commit ) {
		if ( op->o_abandon ) {
		}

		if( LDAP_STAILQ_EMPTY(&op->o_conn->c_txn_ops) ) {
			/* no updates to commit */
			rs->sr_text = "no updates to commit";
			rc = LDAP_OPERATIONS_ERROR;
			goto settled;
		}

		rs->sr_text = "not yet implemented";
		rc = LDAP_UNWILLING_TO_PERFORM;

	} else {
		rs->sr_text = "transaction aborted";
		rc = LDAP_SUCCESS;;
	}

drain:
	/* drain txn ops list */

settled:
	assert( LDAP_STAILQ_EMPTY(&op->o_conn->c_txn_ops) );
	assert( op->o_conn->c_txn == CONN_TXN_SETTLE );
	op->o_conn->c_txn = CONN_TXN_INACTIVE;
	op->o_conn->c_txn_backend = NULL;

done:
	/* release connection lock */
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	return rc;
}

#endif /* LDAP_X_TXN */
