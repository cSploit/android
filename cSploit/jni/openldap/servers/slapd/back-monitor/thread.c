/* thread.c - deal with thread subsystem */
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

#include <ldap_rq.h>

#ifndef NO_THREADS
typedef enum {
	MT_UNKNOWN,
	MT_RUNQUEUE,
	MT_TASKLIST,

	MT_LAST
} monitor_thread_t;

static struct {
	struct berval			rdn;
	struct berval			desc;
	struct berval			nrdn;
	ldap_pvt_thread_pool_param_t	param;
	monitor_thread_t		mt;
}		mt[] = {
	{ BER_BVC( "cn=Max" ),
		BER_BVC("Maximum number of threads as configured"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_MAX,		MT_UNKNOWN },
	{ BER_BVC( "cn=Max Pending" ),
		BER_BVC("Maximum number of pending threads"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_MAX_PENDING,	MT_UNKNOWN },
	{ BER_BVC( "cn=Open" ),		
		BER_BVC("Number of open threads"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_OPEN,	MT_UNKNOWN },
	{ BER_BVC( "cn=Starting" ),	
		BER_BVC("Number of threads being started"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_STARTING,	MT_UNKNOWN },
	{ BER_BVC( "cn=Active" ),	
		BER_BVC("Number of active threads"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_ACTIVE,	MT_UNKNOWN },
	{ BER_BVC( "cn=Pending" ),	
		BER_BVC("Number of pending threads"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_PENDING,	MT_UNKNOWN },
	{ BER_BVC( "cn=Backload" ),	
		BER_BVC("Number of active plus pending threads"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD,	MT_UNKNOWN },
#if 0	/* not meaningful right now */
	{ BER_BVC( "cn=Active Max" ),
		BER_BVNULL,
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_ACTIVE_MAX,	MT_UNKNOWN },
	{ BER_BVC( "cn=Pending Max" ),
		BER_BVNULL,
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_PENDING_MAX,	MT_UNKNOWN },
	{ BER_BVC( "cn=Backload Max" ),
		BER_BVNULL,
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD_MAX,MT_UNKNOWN },
#endif
	{ BER_BVC( "cn=State" ),
		BER_BVC("Thread pool state"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_STATE,	MT_UNKNOWN },

	{ BER_BVC( "cn=Runqueue" ),
		BER_BVC("Queue of running threads - besides those handling operations"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN,	MT_RUNQUEUE },
	{ BER_BVC( "cn=Tasklist" ),
		BER_BVC("List of running plus standby threads - besides those handling operations"),
		BER_BVNULL,	LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN,	MT_TASKLIST },

	{ BER_BVNULL }
};

static int 
monitor_subsys_thread_update( 
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e );
#endif /* ! NO_THREADS */

/*
 * initializes log subentry
 */
int
monitor_subsys_thread_init(
	BackendDB       	*be,
	monitor_subsys_t	*ms )
{
#ifndef NO_THREADS
	monitor_info_t	*mi;
	monitor_entry_t	*mp;
	Entry		*e, **ep, *e_thread;
	int		i;

	ms->mss_update = monitor_subsys_thread_update;

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi, &ms->mss_ndn, &e_thread ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_thread_init: unable to get entry \"%s\"\n",
			ms->mss_dn.bv_val, 
			0, 0 );
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_thread->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	for ( i = 0; !BER_BVISNULL( &mt[ i ].rdn ); i++ ) {
		static char	buf[ BACKMONITOR_BUFSIZE ];
		int		count = -1;
		char		*state = NULL;
		struct berval	bv = BER_BVNULL;

		/*
		 * Max
		 */
		e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn,
			&mt[ i ].rdn,
			mi->mi_oc_monitoredObject, NULL, NULL );
		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_thread_init: "
				"unable to create entry \"%s,%s\"\n",
				mt[ i ].rdn.bv_val,
				ms->mss_ndn.bv_val, 0 );
			return( -1 );
		}

		/* NOTE: reference to the normalized DN of the entry,
		 * under the assumption it's not modified */
		dnRdn( &e->e_nname, &mt[ i ].nrdn );

		switch ( mt[ i ].param ) {
		case LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN:
			break;

		case LDAP_PVT_THREAD_POOL_PARAM_STATE:
			if ( ldap_pvt_thread_pool_query( &connection_pool,
				mt[ i ].param, (void *)&state ) == 0 )
			{
				ber_str2bv( state, 0, 0, &bv );

			} else {
				BER_BVSTR( &bv, "unknown" );
			}
			break;

		default:
			/* NOTE: in case of error, it'll be set to -1 */
			(void)ldap_pvt_thread_pool_query( &connection_pool,
				mt[ i ].param, (void *)&count );
			bv.bv_val = buf;
			bv.bv_len = snprintf( buf, sizeof( buf ), "%d", count );
			break;
		}

		if ( !BER_BVISNULL( &bv ) ) {
			attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo, &bv, NULL );
		}

		if ( !BER_BVISNULL( &mt[ i ].desc ) ) {
			attr_merge_normalize_one( e,
				slap_schema.si_ad_description,
				&mt[ i ].desc, NULL );
		}
	
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
				"monitor_subsys_thread_init: "
				"unable to add entry \"%s,%s\"\n",
				mt[ i ].rdn.bv_val,
				ms->mss_dn.bv_val, 0 );
			return( -1 );
		}
	
		*ep = e;
		ep = &mp->mp_next;
	}

	monitor_cache_release( mi, e_thread );

#endif /* ! NO_THREADS */
	return( 0 );
}

#ifndef NO_THREADS
static int 
monitor_subsys_thread_update( 
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	Attribute		*a;
	BerVarray		vals = NULL;
	char 			buf[ BACKMONITOR_BUFSIZE ];
	struct berval		rdn, bv;
	int			which, i;
	struct re_s		*re;
	int			count = -1;
	char			*state = NULL;

	assert( mi != NULL );

	dnRdn( &e->e_nname, &rdn );

	for ( i = 0; !BER_BVISNULL( &mt[ i ].nrdn ); i++ ) {
		if ( dn_match( &mt[ i ].nrdn, &rdn ) ) {
			break;
		}
	}

	which = i;
	if ( BER_BVISNULL( &mt[ which ].nrdn ) ) {
		return SLAP_CB_CONTINUE;
	}

	a = attr_find( e->e_attrs, mi->mi_ad_monitoredInfo );

	switch ( mt[ which ].param ) {
	case LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN:
		switch ( mt[ which ].mt ) {
		case MT_RUNQUEUE:
			if ( a != NULL ) {
				if ( a->a_nvals != a->a_vals ) {
					ber_bvarray_free( a->a_nvals );
				}
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}

			i = 0;
			bv.bv_val = buf;
			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			LDAP_STAILQ_FOREACH( re, &slapd_rq.run_list, rnext ) {
				bv.bv_len = snprintf( buf, sizeof( buf ), "{%d}%s(%s)",
					i, re->tname, re->tspec );
				if ( bv.bv_len < sizeof( buf ) ) {
					value_add_one( &vals, &bv );
				}
				i++;
			}
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	
			if ( vals ) {
				attr_merge_normalize( e, mi->mi_ad_monitoredInfo, vals, NULL );
				ber_bvarray_free( vals );

			} else {
				attr_delete( &e->e_attrs, mi->mi_ad_monitoredInfo );
			}
			break;

		case MT_TASKLIST:
			if ( a != NULL ) {
				if ( a->a_nvals != a->a_vals ) {
					ber_bvarray_free( a->a_nvals );
				}
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
	
			i = 0;
			bv.bv_val = buf;
			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			LDAP_STAILQ_FOREACH( re, &slapd_rq.task_list, tnext ) {
				bv.bv_len = snprintf( buf, sizeof( buf ), "{%d}%s(%s)",
					i, re->tname, re->tspec );
				if ( bv.bv_len < sizeof( buf ) ) {
					value_add_one( &vals, &bv );
				}
				i++;
			}
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	
			if ( vals ) {
				attr_merge_normalize( e, mi->mi_ad_monitoredInfo, vals, NULL );
				ber_bvarray_free( vals );

			} else {
				attr_delete( &e->e_attrs, mi->mi_ad_monitoredInfo );
			}
			break;

		default:
			assert( 0 );
		}
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_STATE:
		if ( a == NULL ) {
			return rs->sr_err = LDAP_OTHER;
		}
		if ( ldap_pvt_thread_pool_query( &connection_pool,
			mt[ i ].param, (void *)&state ) == 0 )
		{
			ber_str2bv( state, 0, 0, &bv );
			ber_bvreplace( &a->a_vals[ 0 ], &bv );
		}
		break;

	default:
		if ( a == NULL ) {
			return rs->sr_err = LDAP_OTHER;
		}
		if ( ldap_pvt_thread_pool_query( &connection_pool,
			mt[ i ].param, (void *)&count ) == 0 )
		{
			bv.bv_val = buf;
			bv.bv_len = snprintf( buf, sizeof( buf ), "%d", count );
			if ( bv.bv_len < sizeof( buf ) ) {
				ber_bvreplace( &a->a_vals[ 0 ], &bv );
			}
		}
		break;
	}

	/* FIXME: touch modifyTimestamp? */

	return SLAP_CB_CONTINUE;
}
#endif /* ! NO_THREADS */
