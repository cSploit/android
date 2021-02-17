/* search.c - monitor backend search function */
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

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-monitor.h"
#include "proto-back-monitor.h"

static void
monitor_find_children(
	Operation *op,
	SlapReply *rs,
	Entry *e_parent,
	Entry **nonv,
	Entry **vol
)
{
	monitor_entry_t *mp;

	mp = ( monitor_entry_t * )e_parent->e_private;
	*nonv = mp->mp_children;

	if ( MONITOR_HAS_VOLATILE_CH( mp ) ) {
		monitor_entry_create( op, rs, NULL, e_parent, vol );
	}
}

static int
monitor_send_children(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e_nonvolatile,
	Entry		*e_ch,
	int		sub )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	Entry 			*e,
				*e_tmp;
	monitor_entry_t *mp;
	int			rc,
				nonvolatile = 0;

	e = e_nonvolatile;

	/* no volatile entries? */
	if ( e_ch == NULL ) {
		/* no persistent entries? return */
		if ( e == NULL ) {
			return LDAP_SUCCESS;
		}

	/* volatile entries */
	} else {
		/* if no persistent, return only volatile */
		if ( e == NULL ) {
			e = e_ch;

		/* else append persistent to volatile */
		} else {
			e_tmp = e_ch;
			do {
				mp = ( monitor_entry_t * )e_tmp->e_private;
				e_tmp = mp->mp_next;
	
				if ( e_tmp == NULL ) {
					mp->mp_next = e;
					break;
				}
			} while ( e_tmp );
			e = e_ch;
		}
	}

	/* return entries */
	for ( ; e != NULL; e = e_tmp ) {
		Entry *sub_nv = NULL, *sub_ch = NULL;

		monitor_cache_lock( e );
		monitor_entry_update( op, rs, e );

		if ( e == e_nonvolatile )
			nonvolatile = 1;

		mp = ( monitor_entry_t * )e->e_private;
		e_tmp = mp->mp_next;

		if ( op->o_abandon ) {
			monitor_cache_release( mi, e );
			rc = SLAPD_ABANDON;
			goto freeout;
		}

		if ( sub )
			monitor_find_children( op, rs, e, &sub_nv, &sub_ch );

		rc = test_filter( op, e, op->oq_search.rs_filter );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;
			rc = send_search_entry( op, rs );
			if ( rc ) {
				for ( e = sub_ch; e != NULL; e = sub_nv ) {
					mp = ( monitor_entry_t * )e->e_private;
					sub_nv = mp->mp_next;
					monitor_cache_lock( e );
					monitor_cache_release( mi, e );
				}
				goto freeout;
			}
		} else {
			monitor_cache_release( mi, e );
		}

		if ( sub ) {
			rc = monitor_send_children( op, rs, sub_nv, sub_ch, sub );
			if ( rc ) {
freeout:
				if ( nonvolatile == 0 ) {
					for ( ; e_tmp != NULL; ) {
						mp = ( monitor_entry_t * )e_tmp->e_private;
						e = e_tmp;
						e_tmp = mp->mp_next;
						monitor_cache_lock( e );
						monitor_cache_release( mi, e );
	
						if ( e_tmp == e_nonvolatile ) {
							break;
						}
					}
				}

				return( rc );
			}
		}
	}
	
	return LDAP_SUCCESS;
}

int
monitor_back_search( Operation *op, SlapReply *rs )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	int		rc = LDAP_SUCCESS;
	Entry		*e = NULL, *matched = NULL;
	Entry		*e_nv = NULL, *e_ch = NULL;
	slap_mask_t	mask;

	Debug( LDAP_DEBUG_TRACE, "=> monitor_back_search\n", 0, 0, 0 );


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

	/* NOTE: __NEW__ "search" access is required
	 * on searchBase object */
	if ( !access_allowed_mask( op, e, slap_schema.si_ad_entry,
				NULL, ACL_SEARCH, NULL, &mask ) )
	{
		monitor_cache_release( mi, e );

		if ( !ACL_GRANT( mask, ACL_DISCLOSE ) ) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		}

		send_ldap_result( op, rs );

		return rs->sr_err;
	}

	rs->sr_attrs = op->oq_search.rs_attrs;
	switch ( op->oq_search.rs_scope ) {
	case LDAP_SCOPE_BASE:
		monitor_entry_update( op, rs, e );
		rc = test_filter( op, e, op->oq_search.rs_filter );
 		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
		} else {
			monitor_cache_release( mi, e );
		}
		rc = LDAP_SUCCESS;
		break;

	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBORDINATE:
		monitor_find_children( op, rs, e, &e_nv, &e_ch );
		monitor_cache_release( mi, e );
		rc = monitor_send_children( op, rs, e_nv, e_ch,
			op->oq_search.rs_scope == LDAP_SCOPE_SUBORDINATE );
		break;

	case LDAP_SCOPE_SUBTREE:
		monitor_entry_update( op, rs, e );
		monitor_find_children( op, rs, e, &e_nv, &e_ch );
		rc = test_filter( op, e, op->oq_search.rs_filter );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
		} else {
			monitor_cache_release( mi, e );
		}

		rc = monitor_send_children( op, rs, e_nv, e_ch, 1 );
		break;

	default:
		rc = LDAP_UNWILLING_TO_PERFORM;
		monitor_cache_release( mi, e );
	}

	rs->sr_attrs = NULL;
	rs->sr_err = rc;
	if ( rs->sr_err != SLAPD_ABANDON ) {
		send_ldap_result( op, rs );
	}

	return rs->sr_err;
}

