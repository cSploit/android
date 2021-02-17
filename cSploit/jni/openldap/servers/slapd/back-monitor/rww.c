/* readw.c - deal with read waiters subsystem */
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
#include "lutil.h"
#include "back-monitor.h"

static int
monitor_subsys_rww_destroy(
	BackendDB		*be,
	monitor_subsys_t	*ms );

static int
monitor_subsys_rww_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e );

enum {
	MONITOR_RWW_READ = 0,
	MONITOR_RWW_WRITE,

	MONITOR_RWW_LAST
};

static struct monitor_rww_t {
	struct berval	rdn;
	struct berval	nrdn;
} monitor_rww[] = {
	{ BER_BVC("cn=Read"),		BER_BVNULL },
	{ BER_BVC("cn=Write"),		BER_BVNULL },
	{ BER_BVNULL,			BER_BVNULL }
};

int
monitor_subsys_rww_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	monitor_info_t	*mi;
	
	Entry		**ep, *e_conn;
	monitor_entry_t	*mp;
	int			i;

	assert( be != NULL );

	ms->mss_destroy = monitor_subsys_rww_destroy;
	ms->mss_update = monitor_subsys_rww_update;

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi, &ms->mss_ndn, &e_conn ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_rww_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_conn->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	for ( i = 0; i < MONITOR_RWW_LAST; i++ ) {
		struct berval		nrdn, bv;
		Entry			*e;
		
		e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &monitor_rww[i].rdn,
			mi->mi_oc_monitorCounterObject, NULL, NULL );
		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_rww_init: "
				"unable to create entry \"cn=Read,%s\"\n",
				ms->mss_ndn.bv_val, 0, 0 );
			return( -1 );
		}

		/* steal normalized RDN */
		dnRdn( &e->e_nname, &nrdn );
		ber_dupbv( &monitor_rww[ i ].nrdn, &nrdn );
	
		BER_BVSTR( &bv, "0" );
		attr_merge_one( e, mi->mi_ad_monitorCounter, &bv, NULL );
	
		mp = monitor_entrypriv_create();
		if ( mp == NULL ) {
			return -1;
		}
		e->e_private = ( void * )mp;
		mp->mp_info = ms;
		mp->mp_flags = ms->mss_flags \
			| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

		if ( monitor_cache_add( mi, e ) ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_rww_init: "
				"unable to add entry \"%s,%s\"\n",
				monitor_rww[ i ].rdn.bv_val,
				ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}
	
		*ep = e;
		ep = &mp->mp_next;
	}

	monitor_cache_release( mi, e_conn );

	return( 0 );
}

static int
monitor_subsys_rww_destroy(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	int		i;

	for ( i = 0; i < MONITOR_RWW_LAST; i++ ) {
		ber_memfree_x( monitor_rww[ i ].nrdn.bv_val, NULL );
	}

	return 0;
}

static int
monitor_subsys_rww_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e )
{
	monitor_info_t *mi = (monitor_info_t *)op->o_bd->be_private;
	Connection	*c;
	ber_socket_t	connindex;
	long		nconns, nwritewaiters, nreadwaiters;

	int		i;
	struct berval	nrdn;

	Attribute	*a;
	char 		buf[LDAP_PVT_INTTYPE_CHARS(long)];
	long		num = 0;
	ber_len_t	len;

	assert( mi != NULL );
	assert( e != NULL );

	dnRdn( &e->e_nname, &nrdn );

	for ( i = 0; !BER_BVISNULL( &monitor_rww[ i ].nrdn ); i++ ) {
		if ( dn_match( &nrdn, &monitor_rww[ i ].nrdn ) ) {
			break;
		}
	}

	if ( i == MONITOR_RWW_LAST ) {
		return SLAP_CB_CONTINUE;
	}

	nconns = nwritewaiters = nreadwaiters = 0;
	for ( c = connection_first( &connindex );
			c != NULL;
			c = connection_next( c, &connindex ), nconns++ )
	{
		if ( c->c_writewaiter ) {
			nwritewaiters++;
		}

		/* FIXME: ?!? */
		if ( c->c_currentber != NULL ) {
			nreadwaiters++;
		}
	}
	connection_done(c);

	switch ( i ) {
	case MONITOR_RWW_READ:
		num = nreadwaiters;
		break;

	case MONITOR_RWW_WRITE:
		num = nwritewaiters;
		break;

	default:
		assert( 0 );
	}

	snprintf( buf, sizeof( buf ), "%ld", num );

	a = attr_find( e->e_attrs, mi->mi_ad_monitorCounter );
	assert( a != NULL );
	len = strlen( buf );
	if ( len > a->a_vals[ 0 ].bv_len ) {
		a->a_vals[ 0 ].bv_val = ber_memrealloc( a->a_vals[ 0 ].bv_val, len + 1 );
		if ( BER_BVISNULL( &a->a_vals[ 0 ] ) ) {
			BER_BVZERO( &a->a_vals[ 0 ] );
			return SLAP_CB_CONTINUE;
		}
	}
	AC_MEMCPY( a->a_vals[ 0 ].bv_val, buf, len + 1 );
	a->a_vals[ 0 ].bv_len = len;

	/* FIXME: touch modifyTimestamp? */

	return SLAP_CB_CONTINUE;
}

