/* overlay.c - deals with overlay subsystem */
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

#include "slap.h"
#include "back-monitor.h"

/*
 * initializes overlay subentries
 */
int
monitor_subsys_overlay_init(
	BackendDB		*be,
	monitor_subsys_t	*ms
)
{
	monitor_info_t		*mi;
	Entry			*e_overlay, **ep;
	int			i;
	monitor_entry_t		*mp;
	slap_overinst		*on;
	monitor_subsys_t	*ms_database;

	mi = ( monitor_info_t * )be->be_private;

	ms_database = monitor_back_get_subsys( SLAPD_MONITOR_DATABASE_NAME );
	if ( ms_database == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_backend_init: "
			"unable to get "
			"\"" SLAPD_MONITOR_DATABASE_NAME "\" "
			"subsystem\n",
			0, 0, 0 );
		return -1;
	}

	if ( monitor_cache_get( mi, &ms->mss_ndn, &e_overlay ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_overlay_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_overlay->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	for ( on = overlay_next( NULL ), i = 0; on; on = overlay_next( on ), i++ ) {
		char 		buf[ BACKMONITOR_BUFSIZE ];
		struct berval 	bv;
		int		j;
		Entry		*e;
		BackendDB	*be;

		bv.bv_len = snprintf( buf, sizeof( buf ), "cn=Overlay %d", i );
		bv.bv_val = buf;
		e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
			mi->mi_oc_monitoredObject, NULL, NULL );
		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_overlay_init: "
				"unable to create entry \"cn=Overlay %d,%s\"\n",
				i, ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}
		ber_str2bv( on->on_bi.bi_type, 0, 0, &bv );
		attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo, &bv, NULL );
		attr_merge_normalize_one( e, mi->mi_ad_monitorRuntimeConfig,
			on->on_bi.bi_cf_ocs ? (struct berval *)&slap_true_bv :
				(struct berval *)&slap_false_bv, NULL );
		
		attr_merge_normalize_one( e_overlay, mi->mi_ad_monitoredInfo,
				&bv, NULL );

		j = -1;
		LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
			char		buf[ SLAP_LDAPDN_MAXLEN ];
			struct berval	dn;

			j++;
			if ( !overlay_is_inst( be, on->on_bi.bi_type ) ) {
				continue;
			}

			snprintf( buf, sizeof( buf ), "cn=Database %d,%s",
					j, ms_database->mss_dn.bv_val );

			ber_str2bv( buf, 0, 0, &dn );
			attr_merge_normalize_one( e, slap_schema.si_ad_seeAlso,
					&dn, NULL );
		}
		
		mp = monitor_entrypriv_create();
		if ( mp == NULL ) {
			return -1;
		}
		e->e_private = ( void * )mp;
		mp->mp_info = ms;
		mp->mp_flags = ms->mss_flags
			| MONITOR_F_SUB;

		if ( monitor_cache_add( mi, e ) ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_overlay_init: "
				"unable to add entry \"cn=Overlay %d,%s\"\n",
				i, ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		*ep = e;
		ep = &mp->mp_next;
	}
	
	monitor_cache_release( mi, e_overlay );

	return( 0 );
}

