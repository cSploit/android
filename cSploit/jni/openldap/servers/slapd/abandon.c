/* abandon.c - decode and handle an ldap abandon operation */
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
#include <ac/socket.h>

#include "slap.h"

int
do_abandon( Operation *op, SlapReply *rs )
{
	ber_int_t	id;
	Operation	*o;
	const char	*msg;

	Debug( LDAP_DEBUG_TRACE, "%s do_abandon\n",
		op->o_log_prefix, 0, 0 );

	/*
	 * Parse the abandon request.  It looks like this:
	 *
	 *	AbandonRequest := MessageID
	 */

	if ( ber_scanf( op->o_ber, "i", &id ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_abandon: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	Statslog( LDAP_DEBUG_STATS, "%s ABANDON msg=%ld\n",
		op->o_log_prefix, (long) id, 0, 0, 0 );

	if( get_ctrls( op, rs, 0 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_abandon: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		return rs->sr_err;
	} 

	Debug( LDAP_DEBUG_ARGS, "%s do_abandon: id=%ld\n",
		op->o_log_prefix, (long) id, 0 );

	if( id <= 0 ) {
		Debug( LDAP_DEBUG_ANY, "%s do_abandon: bad msgid %ld\n",
			op->o_log_prefix, (long) id, 0 );
		return LDAP_SUCCESS;
	}

	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	/* Find the operation being abandoned. */
	LDAP_STAILQ_FOREACH( o, &op->o_conn->c_ops, o_next ) {
		if ( o->o_msgid == id ) {
			break;
		}
	}

	if ( o == NULL ) {
		msg = "not found";
		/* The operation is not active. Just discard it if found.  */
		LDAP_STAILQ_FOREACH( o, &op->o_conn->c_pending_ops, o_next ) {
			if ( o->o_msgid == id ) {
				msg = "discarded";
				/* FIXME: This traverses c_pending_ops yet again. */
				LDAP_STAILQ_REMOVE( &op->o_conn->c_pending_ops,
					o, Operation, o_next );
				LDAP_STAILQ_NEXT(o, o_next) = NULL;
				op->o_conn->c_n_ops_pending--;
				slap_op_free( o, NULL );
				break;
			}
		}

	} else if ( o->o_tag == LDAP_REQ_BIND
			|| o->o_tag == LDAP_REQ_UNBIND
			|| o->o_tag == LDAP_REQ_ABANDON ) {
		msg = "cannot be abandoned";

#if 0 /* Would break o_abandon used as "suppress response" flag, ITS#6138 */
	} else if ( o->o_abandon ) {
		msg = "already being abandoned";
#endif

	} else {
		msg = "found";
		/* Set the o_abandon flag in the to-be-abandoned operation.
		 * The backend can periodically check this flag and abort the
		 * operation at a convenient time.  However it should "send"
		 * the response anyway, with result code SLAPD_ABANDON.
		 * The functions in result.c will intercept the message.
		 */
		o->o_abandon = 1;
		op->orn_msgid = id;
		op->o_bd = frontendDB;
		rs->sr_err = frontendDB->be_abandon( op, rs );
	}

	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	Debug( LDAP_DEBUG_TRACE, "%s do_abandon: op=%ld %s\n",
		op->o_log_prefix, (long) id, msg );
	return rs->sr_err;
}

int
fe_op_abandon( Operation *op, SlapReply *rs )
{
	LDAP_STAILQ_FOREACH( op->o_bd, &backendDB, be_next ) {
		if ( op->o_bd->be_abandon ) {
			(void)op->o_bd->be_abandon( op, rs );
		}
	}

	return LDAP_SUCCESS;
}
