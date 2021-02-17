/* listener.c - deals with listener subsystem */
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

#include "slap.h"
#include "back-monitor.h"

int
monitor_subsys_listener_init(
	BackendDB		*be,
	monitor_subsys_t	*ms
)
{
	monitor_info_t	*mi;
	Entry		*e_listener, **ep;
	int		i;
	monitor_entry_t	*mp;
	Listener	**l;

	assert( be != NULL );

	if ( ( l = slapd_get_listeners() ) == NULL ) {
		if ( slapMode & SLAP_TOOL_MODE ) {
			return 0;
		}

		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_listener_init: "
			"unable to get listeners\n", 0, 0, 0 );
		return( -1 );
	}

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi, &ms->mss_ndn, &e_listener ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_listener_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_listener->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	for ( i = 0; l[ i ]; i++ ) {
		char 		buf[ BACKMONITOR_BUFSIZE ];
		Entry		*e;
		struct berval bv;

		bv.bv_len = snprintf( buf, sizeof( buf ),
				"cn=Listener %d", i );
		bv.bv_val = buf;
		e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
			mi->mi_oc_monitoredObject, NULL, NULL );

		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_listener_init: "
				"unable to create entry \"cn=Listener %d,%s\"\n",
				i, ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionLocalAddress,
				&l[ i ]->sl_name, NULL );

		attr_merge_normalize_one( e, slap_schema.si_ad_labeledURI,
				&l[ i ]->sl_url, NULL );

#ifdef HAVE_TLS
		if ( l[ i ]->sl_is_tls ) {
			struct berval bv;

			BER_BVSTR( &bv, "TLS" );
			attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo,
					&bv, NULL );
		}
#endif /* HAVE_TLS */
#ifdef LDAP_CONNECTIONLESS
		if ( l[ i ]->sl_is_udp ) {
			struct berval bv;

			BER_BVSTR( &bv, "UDP" );
			attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo,
					&bv, NULL );
		}
#endif /* HAVE_TLS */

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
				"monitor_subsys_listener_init: "
				"unable to add entry \"cn=Listener %d,%s\"\n",
				i, ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		*ep = e;
		ep = &mp->mp_next;
	}
	
	monitor_cache_release( mi, e_listener );

	return( 0 );
}

