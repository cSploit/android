/* compare.c - monitor backend compare routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <slap.h>
#include "back-monitor.h"

int
monitor_back_compare( Operation *op, SlapReply *rs )
{
	monitor_info_t	*mi = ( monitor_info_t * ) op->o_bd->be_private;
	Entry           *e, *matched = NULL;
	int		rc;

	/* get entry with reader lock */
	monitor_cache_dn2entry( op, rs, &op->o_req_ndn, &e, &matched );
	if ( e == NULL ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		if ( matched ) {
			if ( !access_allowed_mask( op, matched,
					slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL, NULL ) )
			{
				/* do nothing */ ;
			} else {
				rs->sr_matched = matched->e_dn;
			}
		}
		send_ldap_result( op, rs );
		if ( matched ) {
			monitor_cache_release( mi, matched );
			rs->sr_matched = NULL;
		}

		return rs->sr_err;
	}

	monitor_entry_update( op, rs, e );
	rs->sr_err = slap_compare_entry( op, e, op->orc_ava );
	rc = rs->sr_err;
	switch ( rc ) {
	case LDAP_COMPARE_FALSE:
	case LDAP_COMPARE_TRUE:
		rc = LDAP_SUCCESS;
		break;
	}
		
	send_ldap_result( op, rs );
	rs->sr_err = rc;

	monitor_cache_release( mi, e );

	return rs->sr_err;
}

