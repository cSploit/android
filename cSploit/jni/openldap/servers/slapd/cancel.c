/* cancel.c - LDAP cancel extended operation */
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

const struct berval slap_EXOP_CANCEL = BER_BVC(LDAP_EXOP_CANCEL);

int cancel_extop( Operation *op, SlapReply *rs )
{
	Operation *o;
	int rc;
	int opid;
	BerElement *ber;

	assert( ber_bvcmp( &slap_EXOP_CANCEL, &op->ore_reqoid ) == 0 );

	if ( op->ore_reqdata == NULL ) {
		rs->sr_text = "no message ID supplied";
		return LDAP_PROTOCOL_ERROR;
	}

	ber = ber_init( op->ore_reqdata );
	if ( ber == NULL ) {
		rs->sr_text = "internal error";
		return LDAP_OTHER;
	}

	if ( ber_scanf( ber, "{i}", &opid ) == LBER_ERROR ) {
		rs->sr_text = "message ID parse failed";
		return LDAP_PROTOCOL_ERROR;
	}

	(void) ber_free( ber, 1 );

	Statslog( LDAP_DEBUG_STATS, "%s CANCEL msg=%d\n",
		op->o_log_prefix, opid, 0, 0, 0 );

	if ( opid < 0 ) {
		rs->sr_text = "message ID invalid";
		return LDAP_PROTOCOL_ERROR;
	}

	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	if ( op->o_abandon ) {
		/* FIXME: Should instead reject the cancel/abandon of this op, but
		 * it seems unsafe to reset op->o_abandon once it is set. ITS#6138.
		 */
		rc = LDAP_OPERATIONS_ERROR;
		rs->sr_text = "tried to abandon or cancel this operation";
		goto out;
	}

	LDAP_STAILQ_FOREACH( o, &op->o_conn->c_pending_ops, o_next ) {
		if ( o->o_msgid == opid ) {
			/* TODO: We could instead remove the cancelled operation
			 * from c_pending_ops like Abandon does, and send its
			 * response here.  Not if it is pending because of a
			 * congested connection though.
			 */
			rc = LDAP_CANNOT_CANCEL;
			rs->sr_text = "too busy for Cancel, try Abandon instead";
			goto out;
		}
	}

	LDAP_STAILQ_FOREACH( o, &op->o_conn->c_ops, o_next ) {
		if ( o->o_msgid == opid ) {
			break;
		}
	}

	if ( o == NULL ) {
	 	rc = LDAP_NO_SUCH_OPERATION;
		rs->sr_text = "message ID not found";

	} else if ( o->o_tag == LDAP_REQ_BIND
			|| o->o_tag == LDAP_REQ_UNBIND
			|| o->o_tag == LDAP_REQ_ABANDON ) {
		rc = LDAP_CANNOT_CANCEL;

	} else if ( o->o_cancel != SLAP_CANCEL_NONE ) {
		rc = LDAP_OPERATIONS_ERROR;
		rs->sr_text = "message ID already being cancelled";

#if 0
	} else if ( o->o_abandon ) {
		/* TODO: Would this break something when
		 * o_abandon="suppress response"? (ITS#6138)
		 */
		rc = LDAP_TOO_LATE;
#endif

	} else {
		rc = LDAP_SUCCESS;
		o->o_cancel = SLAP_CANCEL_REQ;
		o->o_abandon = 1;
	}

 out:
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	if ( rc == LDAP_SUCCESS ) {
		LDAP_STAILQ_FOREACH( op->o_bd, &backendDB, be_next ) {
			if( !op->o_bd->be_cancel ) continue;

			op->oq_cancel.rs_msgid = opid;
			if ( op->o_bd->be_cancel( op, rs ) == LDAP_SUCCESS ) {
				return LDAP_SUCCESS;
			}
		}

		do {
			/* Fake a cond_wait with thread_yield, then
			 * verify the result properly mutex-protected.
			 */
			while ( o->o_cancel == SLAP_CANCEL_REQ )
				ldap_pvt_thread_yield();
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			rc = o->o_cancel;
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
		} while ( rc == SLAP_CANCEL_REQ );

		if ( rc == SLAP_CANCEL_ACK ) {
			rc = LDAP_SUCCESS;
		}

		o->o_cancel = SLAP_CANCEL_DONE;
	}

	return rc;
}
