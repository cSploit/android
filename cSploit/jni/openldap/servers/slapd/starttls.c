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

#include "slap.h"
#include "lber_pvt.h"

const struct berval slap_EXOP_START_TLS = BER_BVC(LDAP_EXOP_START_TLS);

#ifdef HAVE_TLS
int
starttls_extop ( Operation *op, SlapReply *rs )
{
	int rc;

	Statslog( LDAP_DEBUG_STATS, "%s STARTTLS\n",
	    op->o_log_prefix, 0, 0, 0, 0 );

	if ( op->ore_reqdata != NULL ) {
		/* no request data should be provided */
		rs->sr_text = "no request data expected";
		return LDAP_PROTOCOL_ERROR;
	}

	/* acquire connection lock */
	ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

	/* can't start TLS if it is already started */
	if (op->o_conn->c_is_tls != 0) {
		rs->sr_text = "TLS already started";
		rc = LDAP_OPERATIONS_ERROR;
		goto done;
	}

	/* can't start TLS if there are other op's around */
	if (( !LDAP_STAILQ_EMPTY(&op->o_conn->c_ops) &&
			(LDAP_STAILQ_FIRST(&op->o_conn->c_ops) != op ||
			LDAP_STAILQ_NEXT(op, o_next) != NULL)) ||
		( !LDAP_STAILQ_EMPTY(&op->o_conn->c_pending_ops) ))
	{
		rs->sr_text = "cannot start TLS when operations are outstanding";
		rc = LDAP_OPERATIONS_ERROR;
		goto done;
	}

	if ( !( global_disallows & SLAP_DISALLOW_TLS_2_ANON ) &&
		( op->o_conn->c_dn.bv_len != 0 ) )
	{
		Statslog( LDAP_DEBUG_STATS,
			"%s AUTHZ anonymous mech=starttls ssf=0\n",
			op->o_log_prefix, 0, 0, 0, 0 );

		/* force to anonymous */
		connection2anonymous( op->o_conn );
	}

	if ( ( global_disallows & SLAP_DISALLOW_TLS_AUTHC ) &&
		( op->o_conn->c_dn.bv_len != 0 ) )
	{
		rs->sr_text = "cannot start TLS after authentication";
		rc = LDAP_OPERATIONS_ERROR;
		goto done;
	}

	/* fail if TLS could not be initialized */
	if ( slap_tls_ctx == NULL ) {
		if (default_referral != NULL) {
			/* caller will put the referral in the result */
			rc = LDAP_REFERRAL;
			goto done;
		}

		rs->sr_text = "Could not initialize TLS";
		rc = LDAP_UNAVAILABLE;
		goto done;
	}

    op->o_conn->c_is_tls = 1;
    op->o_conn->c_needs_tls_accept = 1;

    rc = LDAP_SUCCESS;

done:
	/* give up connection lock */
	ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

	/* FIXME: RACE CONDITION! we give up lock before sending result
	 * Should be resolved by reworking connection state, not
	 * by moving send here (so as to ensure proper TLS sequencing)
	 */

	return rc;
}

#endif	/* HAVE_TLS */
