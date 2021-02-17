/* operation.c - deal with operation subsystem */
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
#include "lber_pvt.h"

struct monitor_ops_t {
	struct berval	rdn;
	struct berval	nrdn;
} monitor_op[] = {
	{ BER_BVC( "cn=Bind" ),		BER_BVNULL },
	{ BER_BVC( "cn=Unbind" ),	BER_BVNULL },
	{ BER_BVC( "cn=Search" ),	BER_BVNULL },
	{ BER_BVC( "cn=Compare" ),	BER_BVNULL },
	{ BER_BVC( "cn=Modify" ),	BER_BVNULL },
	{ BER_BVC( "cn=Modrdn" ),	BER_BVNULL },
	{ BER_BVC( "cn=Add" ),		BER_BVNULL },
	{ BER_BVC( "cn=Delete" ),	BER_BVNULL },
	{ BER_BVC( "cn=Abandon" ),	BER_BVNULL },
	{ BER_BVC( "cn=Extended" ),	BER_BVNULL },
	{ BER_BVNULL,			BER_BVNULL }
};

static int
monitor_subsys_ops_destroy(
	BackendDB		*be,
	monitor_subsys_t	*ms );

static int
monitor_subsys_ops_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e );

int
monitor_subsys_ops_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	monitor_info_t	*mi;
	
	Entry		*e_op, **ep;
	monitor_entry_t	*mp;
	int 		i;
	struct berval	bv_zero = BER_BVC( "0" );

	assert( be != NULL );

	ms->mss_destroy = monitor_subsys_ops_destroy;
	ms->mss_update = monitor_subsys_ops_update;

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi,
			&ms->mss_ndn, &e_op ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_ops_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val, 
			0, 0 );
		return( -1 );
	}

	attr_merge_one( e_op, mi->mi_ad_monitorOpInitiated, &bv_zero, NULL );
	attr_merge_one( e_op, mi->mi_ad_monitorOpCompleted, &bv_zero, NULL );

	mp = ( monitor_entry_t * )e_op->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	for ( i = 0; i < SLAP_OP_LAST; i++ ) {
		struct berval	rdn;
		Entry		*e;
		struct berval bv;

		/*
		 * Initiated ops
		 */
		e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &monitor_op[i].rdn,
			mi->mi_oc_monitorOperation, NULL, NULL );

		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_ops_init: "
				"unable to create entry \"%s,%s\"\n",
				monitor_op[ i ].rdn.bv_val,
				ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		BER_BVSTR( &bv, "0" );
		attr_merge_one( e, mi->mi_ad_monitorOpInitiated, &bv, NULL );
		attr_merge_one( e, mi->mi_ad_monitorOpCompleted, &bv, NULL );

		/* steal normalized RDN */
		dnRdn( &e->e_nname, &rdn );
		ber_dupbv( &monitor_op[ i ].nrdn, &rdn );
	
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
				"monitor_subsys_ops_init: "
				"unable to add entry \"%s,%s\"\n",
				monitor_op[ i ].rdn.bv_val,
				ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		*ep = e;
		ep = &mp->mp_next;
	}

	monitor_cache_release( mi, e_op );

	return( 0 );
}

static int
monitor_subsys_ops_destroy(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	int		i;

	for ( i = 0; i < SLAP_OP_LAST; i++ ) {
		if ( !BER_BVISNULL( &monitor_op[ i ].nrdn ) ) {
			ch_free( monitor_op[ i ].nrdn.bv_val );
		}
	}

	return 0;
}

static int
monitor_subsys_ops_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e )
{
	monitor_info_t		*mi = ( monitor_info_t * )op->o_bd->be_private;

	ldap_pvt_mp_t		nInitiated = LDAP_PVT_MP_INIT,
				nCompleted = LDAP_PVT_MP_INIT;
	struct berval		rdn;
	int 			i;
	Attribute		*a;
	slap_counters_t *sc;
	static struct berval	bv_ops = BER_BVC( "cn=operations" );

	assert( mi != NULL );
	assert( e != NULL );

	dnRdn( &e->e_nname, &rdn );

	if ( dn_match( &rdn, &bv_ops ) ) {
		ldap_pvt_mp_init( nInitiated );
		ldap_pvt_mp_init( nCompleted );

		ldap_pvt_thread_mutex_lock( &slap_counters.sc_mutex );
		for ( i = 0; i < SLAP_OP_LAST; i++ ) {
			ldap_pvt_mp_add( nInitiated, slap_counters.sc_ops_initiated_[ i ] );
			ldap_pvt_mp_add( nCompleted, slap_counters.sc_ops_completed_[ i ] );
		}
		for ( sc = slap_counters.sc_next; sc; sc = sc->sc_next ) {
			ldap_pvt_thread_mutex_lock( &sc->sc_mutex );
			for ( i = 0; i < SLAP_OP_LAST; i++ ) {
				ldap_pvt_mp_add( nInitiated, sc->sc_ops_initiated_[ i ] );
				ldap_pvt_mp_add( nCompleted, sc->sc_ops_completed_[ i ] );
			}
			ldap_pvt_thread_mutex_unlock( &sc->sc_mutex );
		}
		ldap_pvt_thread_mutex_unlock( &slap_counters.sc_mutex );
		
	} else {
		for ( i = 0; i < SLAP_OP_LAST; i++ ) {
			if ( dn_match( &rdn, &monitor_op[ i ].nrdn ) )
			{
				ldap_pvt_thread_mutex_lock( &slap_counters.sc_mutex );
				ldap_pvt_mp_init_set( nInitiated, slap_counters.sc_ops_initiated_[ i ] );
				ldap_pvt_mp_init_set( nCompleted, slap_counters.sc_ops_completed_[ i ] );
				for ( sc = slap_counters.sc_next; sc; sc = sc->sc_next ) {
					ldap_pvt_thread_mutex_lock( &sc->sc_mutex );
					ldap_pvt_mp_add( nInitiated, sc->sc_ops_initiated_[ i ] );
					ldap_pvt_mp_add( nCompleted, sc->sc_ops_completed_[ i ] );
					ldap_pvt_thread_mutex_unlock( &sc->sc_mutex );
				}
				ldap_pvt_thread_mutex_unlock( &slap_counters.sc_mutex );
				break;
			}
		}

		if ( i == SLAP_OP_LAST ) {
			/* not found ... */
			return( 0 );
		}
	}

	a = attr_find( e->e_attrs, mi->mi_ad_monitorOpInitiated );
	assert ( a != NULL );

	/* NOTE: no minus sign is allowed in the counters... */
	UI2BV( &a->a_vals[ 0 ], nInitiated );
	ldap_pvt_mp_clear( nInitiated );
	
	a = attr_find( e->e_attrs, mi->mi_ad_monitorOpCompleted );
	assert ( a != NULL );

	/* NOTE: no minus sign is allowed in the counters... */
	UI2BV( &a->a_vals[ 0 ], nCompleted );
	ldap_pvt_mp_clear( nCompleted );

	/* FIXME: touch modifyTimestamp? */

	return SLAP_CB_CONTINUE;
}

