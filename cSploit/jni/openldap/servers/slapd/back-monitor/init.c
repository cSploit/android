/* init.c - initialize monitor backend */
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

#include <lutil.h>
#include "slap.h"
#include "config.h"
#include "lber_pvt.h"
#include "back-monitor.h"

#include "config.h"

#undef INTEGRATE_CORE_SCHEMA

/*
 * used by many functions to add description to entries
 *
 * WARNING: be_monitor may change as new databases are added,
 * so it should not be used outside monitor_back_db_init()
 * until monitor_back_db_open is called.
 */
BackendDB			*be_monitor;

static struct monitor_subsys_t	**monitor_subsys;
static int			monitor_subsys_opened;
static monitor_info_t		monitor_info;
static const monitor_extra_t monitor_extra = {
	monitor_back_is_configured,
	monitor_back_get_subsys,
	monitor_back_get_subsys_by_dn,

	monitor_back_register_subsys,
	monitor_back_register_backend,
	monitor_back_register_database,
	monitor_back_register_overlay_info,
	monitor_back_register_overlay,
	monitor_back_register_entry,
	monitor_back_register_entry_parent,
	monitor_back_register_entry_attrs,
	monitor_back_register_entry_callback,

	monitor_back_unregister_entry,
	monitor_back_unregister_entry_parent,
	monitor_back_unregister_entry_attrs,
	monitor_back_unregister_entry_callback,

	monitor_back_entry_stub,
	monitor_back_entrypriv_create,
	monitor_back_register_subsys_late
};
	

/*
 * subsystem data
 *
 * the known subsystems are added to the subsystems
 * array at backend initialization; other subsystems
 * may be added by calling monitor_back_register_subsys()
 * before the database is opened (e.g. by other backends
 * or by overlays or modules).
 */
static struct monitor_subsys_t known_monitor_subsys[] = {
	{ 
		SLAPD_MONITOR_BACKEND_NAME, 
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about available backends." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_backend_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_CONN_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about connections." ),
			BER_BVNULL },
		MONITOR_F_VOLATILE_CH,
		monitor_subsys_conn_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_DATABASE_NAME, 	
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about configured databases." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_database_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_LISTENER_NAME, 	
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about active listeners." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_listener_init,
		NULL,	/* destroy */
		NULL,	/* update */
		NULL,	/* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_LOG_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about logging." ),
		  	BER_BVC( "Set the attribute \"managedInfo\" to the desired log levels." ),
			BER_BVNULL },
		MONITOR_F_NONE,
		monitor_subsys_log_init,
		NULL,	/* destroy */
		NULL,	/* update */
		NULL,   /* create */
		NULL,	/* modify */
       	}, { 
		SLAPD_MONITOR_OPS_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about performed operations." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_ops_init,
		NULL,	/* destroy */
		NULL,	/* update */
		NULL,   /* create */
		NULL,	/* modify */
       	}, { 
		SLAPD_MONITOR_OVERLAY_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about available overlays." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_overlay_init,
		NULL,	/* destroy */
		NULL,	/* update */
		NULL,   /* create */
		NULL,	/* modify */
	}, { 
		SLAPD_MONITOR_SASL_NAME, 	
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about SASL." ),
			BER_BVNULL },
		MONITOR_F_NONE,
		NULL,   /* init */
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_SENT_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains statistics." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_sent_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL,	/* modify */
       	}, { 
		SLAPD_MONITOR_THREAD_NAME, 	
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about threads." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_thread_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_TIME_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about time." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_time_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL,	/* modify */
       	}, { 
		SLAPD_MONITOR_TLS_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about TLS." ),
			BER_BVNULL },
		MONITOR_F_NONE,
		NULL,   /* init */
		NULL,	/* destroy */
		NULL,   /* update */
		NULL,   /* create */
		NULL	/* modify */
       	}, { 
		SLAPD_MONITOR_RWW_NAME,
		BER_BVNULL, BER_BVNULL, BER_BVNULL,
		{ BER_BVC( "This subsystem contains information about read/write waiters." ),
			BER_BVNULL },
		MONITOR_F_PERSISTENT_CH,
		monitor_subsys_rww_init,
		NULL,	/* destroy */
		NULL,   /* update */
		NULL, 	/* create */
		NULL	/* modify */
       	}, { NULL }
};

int
monitor_subsys_is_opened( void )
{
	return monitor_subsys_opened;
}

int
monitor_back_register_subsys(
	monitor_subsys_t	*ms )
{
	int	i = 0;

	if ( monitor_subsys ) {
		for ( ; monitor_subsys[ i ] != NULL; i++ )
			/* just count'em */ ;
	}

	monitor_subsys = ch_realloc( monitor_subsys,
			( 2 + i ) * sizeof( monitor_subsys_t * ) );

	if ( monitor_subsys == NULL ) {
		return -1;
	}

	monitor_subsys[ i ] = ms;
	monitor_subsys[ i + 1 ] = NULL;

	/* if a subsystem is registered __AFTER__ subsystem 
	 * initialization (depending on the sequence the databases
	 * are listed in slapd.conf), init it */
	if ( monitor_subsys_is_opened() ) {

		/* FIXME: this should only be possible
		 * if be_monitor is already initialized */
		assert( be_monitor != NULL );

		if ( ms->mss_open && ( *ms->mss_open )( be_monitor, ms ) ) {
			return -1;
		}

		ms->mss_flags |= MONITOR_F_OPENED;
	}

	return 0;
}

enum {
	LIMBO_ENTRY,
	LIMBO_ENTRY_PARENT,
	LIMBO_ATTRS,
	LIMBO_CB,
	LIMBO_BACKEND,
	LIMBO_DATABASE,
	LIMBO_OVERLAY_INFO,
	LIMBO_OVERLAY,
	LIMBO_SUBSYS,

	LIMBO_LAST
};

typedef struct entry_limbo_t {
	int			el_type;
	BackendInfo		*el_bi;
	BackendDB		*el_be;
	slap_overinst		*el_on;
	Entry			*el_e;
	Attribute		*el_a;
	struct berval		*el_ndn;
	struct berval		el_nbase;
	int			el_scope;
	struct berval		el_filter;
	monitor_callback_t	*el_cb;
	monitor_subsys_t	*el_mss;
	unsigned long		el_flags;
	struct entry_limbo_t	*el_next;
} entry_limbo_t;

int
monitor_back_is_configured( void )
{
	return be_monitor != NULL;
}

int
monitor_back_register_subsys_late(
	monitor_subsys_t	*ms )
{
	entry_limbo_t	**elpp, el = { 0 };
	monitor_info_t 	*mi;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_subsys_late: "
			"monitor database not configured.\n",
			0, 0, 0 );
		return -1;
	}

	/* everyting is ready, can register already */
	if ( monitor_subsys_is_opened() ) {
		return monitor_back_register_subsys( ms );
	}

	mi = ( monitor_info_t * )be_monitor->be_private;


	el.el_type = LIMBO_SUBSYS;

	el.el_mss = ms;

	for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		/* go to last */;

	*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );

	el.el_next = NULL;
	**elpp = el;

	return 0;
}

int
monitor_back_register_backend(
	BackendInfo		*bi )
{
	return -1;
}

int
monitor_back_register_overlay_info(
	slap_overinst		*on )
{
	return -1;
}

int
monitor_back_register_backend_limbo(
	BackendInfo		*bi )
{
	return -1;
}

int
monitor_back_register_database_limbo(
	BackendDB		*be,
	struct berval		*ndn_out )
{
	entry_limbo_t	**elpp, el = { 0 };
	monitor_info_t 	*mi;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_database_limbo: "
			"monitor database not configured.\n",
			0, 0, 0 );
		return -1;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;


	el.el_type = LIMBO_DATABASE;

	el.el_be = be->bd_self;
	el.el_ndn = ndn_out;
	
	for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		/* go to last */;

	*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );

	el.el_next = NULL;
	**elpp = el;

	return 0;
}

int
monitor_back_register_overlay_info_limbo(
	slap_overinst		*on )
{
	return -1;
}

int
monitor_back_register_overlay_limbo(
	BackendDB		*be,
	struct slap_overinst	*on,
	struct berval		*ndn_out )
{
	entry_limbo_t	**elpp, el = { 0 };
	monitor_info_t 	*mi;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_overlay_limbo: "
			"monitor database not configured.\n",
			0, 0, 0 );
		return -1;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;


	el.el_type = LIMBO_OVERLAY;

	el.el_be = be->bd_self;
	el.el_on = on;
	el.el_ndn = ndn_out;
	
	for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		/* go to last */;

	*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );

	el.el_next = NULL;
	**elpp = el;

	return 0;
}

int
monitor_back_register_entry(
	Entry			*e,
	monitor_callback_t	*cb,
	monitor_subsys_t	*mss,
	unsigned long		flags )
{
	monitor_info_t 	*mi;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_entry(\"%s\"): "
			"monitor database not configured.\n",
			e->e_name.bv_val, 0, 0 );
		return -1;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );
	assert( e != NULL );
	assert( e->e_private == NULL );
	
	if ( monitor_subsys_is_opened() ) {
		Entry		*e_parent = NULL,
				*e_new = NULL,
				**ep = NULL;
		struct berval	pdn = BER_BVNULL;
		monitor_entry_t *mp = NULL,
				*mp_parent = NULL;
		int		rc = 0;

		if ( monitor_cache_get( mi, &e->e_nname, &e_parent ) == 0 ) {
			/* entry exists */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"entry exists\n",
				e->e_name.bv_val, 0, 0 );
			monitor_cache_release( mi, e_parent );
			return -1;
		}

		dnParent( &e->e_nname, &pdn );
		if ( monitor_cache_get( mi, &pdn, &e_parent ) != 0 ) {
			/* parent does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"parent \"%s\" not found\n",
				e->e_name.bv_val, pdn.bv_val, 0 );
			return -1;
		}

		assert( e_parent->e_private != NULL );
		mp_parent = ( monitor_entry_t * )e_parent->e_private;

		if ( mp_parent->mp_flags & MONITOR_F_VOLATILE ) {
			/* entry is volatile; cannot append children */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"parent \"%s\" is volatile\n",
				e->e_name.bv_val, e_parent->e_name.bv_val, 0 );
			rc = -1;
			goto done;
		}

		mp = monitor_entrypriv_create();
		if ( mp == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"monitor_entrypriv_create() failed\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

		e_new = entry_dup( e );
		if ( e_new == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"entry_dup() failed\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}
		
		e_new->e_private = ( void * )mp;
		if ( mss != NULL ) {
			mp->mp_info = mss;
			mp->mp_flags = flags;

		} else {
			mp->mp_info = mp_parent->mp_info;
			mp->mp_flags = mp_parent->mp_flags | MONITOR_F_SUB;
		}
		mp->mp_cb = cb;

		ep = &mp_parent->mp_children;
		for ( ; *ep; ) {
			mp_parent = ( monitor_entry_t * )(*ep)->e_private;
			ep = &mp_parent->mp_next;
		}
		*ep = e_new;

		if ( monitor_cache_add( mi, e_new ) ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"unable to add entry\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

done:;
		if ( rc ) {
			if ( mp ) {
				ch_free( mp );
			}
			if ( e_new ) {
				e_new->e_private = NULL;
				entry_free( e_new );
			}
		}

		if ( e_parent ) {
			monitor_cache_release( mi, e_parent );
		}

	} else {
		entry_limbo_t	**elpp, el = { 0 };

		el.el_type = LIMBO_ENTRY;

		el.el_e = entry_dup( e );
		if ( el.el_e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"entry_dup() failed\n",
				e->e_name.bv_val, 0, 0 );
			return -1;
		}
		
		el.el_cb = cb;
		el.el_mss = mss;
		el.el_flags = flags;

		for ( elpp = &mi->mi_entry_limbo;
				*elpp;
				elpp = &(*elpp)->el_next )
			/* go to last */;

		*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );
		if ( *elpp == NULL ) {
			el.el_e->e_private = NULL;
			entry_free( el.el_e );
			return -1;
		}

		el.el_next = NULL;
		**elpp = el;
	}

	return 0;
}

int
monitor_back_register_entry_parent(
	Entry			*e,
	monitor_callback_t	*cb,
	monitor_subsys_t	*mss,
	unsigned long		flags,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	monitor_info_t 	*mi;
	struct berval	ndn = BER_BVNULL;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_entry_parent(base=\"%s\" scope=%s filter=\"%s\"): "
			"monitor database not configured.\n",
			BER_BVISNULL( nbase ) ? "" : nbase->bv_val,
			ldap_pvt_scope2str( scope ),
			BER_BVISNULL( filter ) ? "" : filter->bv_val );
		return -1;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );
	assert( e != NULL );
	assert( e->e_private == NULL );

	if ( BER_BVISNULL( filter ) ) {
		/* need a filter */
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_entry_parent(\"\"): "
			"need a valid filter\n",
			0, 0, 0 );
		return -1;
	}

	if ( monitor_subsys_is_opened() ) {
		Entry		*e_parent = NULL,
				*e_new = NULL,
				**ep = NULL;
		struct berval	e_name = BER_BVNULL,
				e_nname = BER_BVNULL;
		monitor_entry_t *mp = NULL,
				*mp_parent = NULL;
		int		rc = 0;

		if ( monitor_search2ndn( nbase, scope, filter, &ndn ) ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_parent(\"\"): "
				"base=\"%s\" scope=%s filter=\"%s\": "
				"unable to find entry\n",
				nbase->bv_val ? nbase->bv_val : "\"\"",
				ldap_pvt_scope2str( scope ),
				filter->bv_val );
			return -1;
		}

		if ( monitor_cache_get( mi, &ndn, &e_parent ) != 0 ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_parent(\"%s\"): "
				"parent entry does not exist\n",
				ndn.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

		assert( e_parent->e_private != NULL );
		mp_parent = ( monitor_entry_t * )e_parent->e_private;

		if ( mp_parent->mp_flags & MONITOR_F_VOLATILE ) {
			/* entry is volatile; cannot append callback */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_parent(\"%s\"): "
				"entry is volatile\n",
				e_parent->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

		build_new_dn( &e_name, &e_parent->e_name, &e->e_name, NULL );
		build_new_dn( &e_nname, &e_parent->e_nname, &e->e_nname, NULL );

		if ( monitor_cache_get( mi, &e_nname, &e_new ) == 0 ) {
			/* entry already exists */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_parent(\"%s\"): "
				"entry already exists\n",
				e_name.bv_val, 0, 0 );
			monitor_cache_release( mi, e_new );
			e_new = NULL;
			rc = -1;
			goto done;
		}

		mp = monitor_entrypriv_create();
		if ( mp == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_parent(\"%s\"): "
				"monitor_entrypriv_create() failed\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

		e_new = entry_dup( e );
		if ( e_new == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"entry_dup() failed\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}
		ch_free( e_new->e_name.bv_val );
		ch_free( e_new->e_nname.bv_val );
		e_new->e_name = e_name;
		e_new->e_nname = e_nname;
		
		e_new->e_private = ( void * )mp;
		if ( mss != NULL ) {
			mp->mp_info = mss;
			mp->mp_flags = flags;

		} else {
			mp->mp_info = mp_parent->mp_info;
			mp->mp_flags = mp_parent->mp_flags | MONITOR_F_SUB;
		}
		mp->mp_cb = cb;

		ep = &mp_parent->mp_children;
		for ( ; *ep; ) {
			mp_parent = ( monitor_entry_t * )(*ep)->e_private;
			ep = &mp_parent->mp_next;
		}
		*ep = e_new;

		if ( monitor_cache_add( mi, e_new ) ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"unable to add entry\n",
				e->e_name.bv_val, 0, 0 );
			rc = -1;
			goto done;
		}

done:;
		if ( !BER_BVISNULL( &ndn ) ) {
			ch_free( ndn.bv_val );
		}

		if ( rc ) {
			if ( mp ) {
				ch_free( mp );
			}
			if ( e_new ) {
				e_new->e_private = NULL;
				entry_free( e_new );
			}
		}

		if ( e_parent ) {
			monitor_cache_release( mi, e_parent );
		}

	} else {
		entry_limbo_t	**elpp = NULL, el = { 0 };

		el.el_type = LIMBO_ENTRY_PARENT;

		el.el_e = entry_dup( e );
		if ( el.el_e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry(\"%s\"): "
				"entry_dup() failed\n",
				e->e_name.bv_val, 0, 0 );
			goto done_limbo;
		}
		
		if ( !BER_BVISNULL( nbase ) ) {
			ber_dupbv( &el.el_nbase, nbase );
		}

		el.el_scope = scope;
		if ( !BER_BVISNULL( filter ) ) {
			ber_dupbv( &el.el_filter, filter  );
		}

		el.el_cb = cb;
		el.el_mss = mss;
		el.el_flags = flags;

		for ( elpp = &mi->mi_entry_limbo;
				*elpp;
				elpp = &(*elpp)->el_next )
			/* go to last */;

		*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );
		if ( *elpp == NULL ) {
			goto done_limbo;
		}

done_limbo:;
		if ( *elpp != NULL ) {
			el.el_next = NULL;
			**elpp = el;

		} else {
			if ( !BER_BVISNULL( &el.el_filter ) ) {
				ch_free( el.el_filter.bv_val );
			}
			if ( !BER_BVISNULL( &el.el_nbase ) ) {
				ch_free( el.el_nbase.bv_val );
			}
			entry_free( el.el_e );
			return -1;
		}
	}

	return 0;
}

static int
monitor_search2ndn_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		struct berval	*ndn = op->o_callback->sc_private;

		if ( !BER_BVISNULL( ndn ) ) {
			rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
			ch_free( ndn->bv_val );
			BER_BVZERO( ndn );
			return rs->sr_err;
		}
		
		ber_dupbv( ndn, &rs->sr_entry->e_nname );
	}

	return 0;
}

int
monitor_search2ndn(
	struct berval	*nbase,
	int		scope,
	struct berval	*filter,
	struct berval	*ndn )
{
	Connection	conn = { 0 };
	OperationBuffer	opbuf;
	Operation	*op;
	void	*thrctx;
	SlapReply	rs = { REP_RESULT };
	slap_callback	cb = { NULL, monitor_search2ndn_cb, NULL, NULL };
	int		rc;

	BER_BVZERO( ndn );

	if ( be_monitor == NULL ) {
		return -1;
	}

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;

	/* use global malloc for now */
	if ( op->o_tmpmemctx ) {
		op->o_tmpmemctx = NULL;
	}
	op->o_tmpmfuncs = &ch_mfuncs;

	op->o_bd = be_monitor;
	if ( nbase == NULL || BER_BVISNULL( nbase ) ) {
		ber_dupbv_x( &op->o_req_dn, &op->o_bd->be_suffix[ 0 ],
				op->o_tmpmemctx );
		ber_dupbv_x( &op->o_req_ndn, &op->o_bd->be_nsuffix[ 0 ],
				op->o_tmpmemctx );

	} else {
		if ( dnPrettyNormal( NULL, nbase, &op->o_req_dn, &op->o_req_ndn,
					op->o_tmpmemctx ) ) {
			return -1;
		}
	}

	op->o_callback = &cb;
	cb.sc_private = (void *)ndn;

	op->ors_scope = scope;
	op->ors_filter = str2filter_x( op, filter->bv_val );
	if ( op->ors_filter == NULL ) {
		rc = LDAP_OTHER;
		goto cleanup;
	}
	ber_dupbv_x( &op->ors_filterstr, filter, op->o_tmpmemctx );
	op->ors_attrs = slap_anlist_no_attrs;
	op->ors_attrsonly = 0;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = 1;
	op->ors_limit = NULL;
	op->ors_deref = LDAP_DEREF_NEVER;

	op->o_nocaching = 1;
	op->o_managedsait = SLAP_CONTROL_NONCRITICAL;

	op->o_dn = be_monitor->be_rootdn;
	op->o_ndn = be_monitor->be_rootndn;

	rc = op->o_bd->be_search( op, &rs );

cleanup:;
	if ( op->ors_filter != NULL ) {
		filter_free_x( op, op->ors_filter, 1 );
	}
	if ( !BER_BVISNULL( &op->ors_filterstr ) ) {
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &op->o_req_dn ) ) {
		op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &op->o_req_ndn ) ) {
		op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
	}

	if ( rc != 0 ) {
		return rc;
	}

	switch ( rs.sr_err ) {
	case LDAP_SUCCESS:
		if ( BER_BVISNULL( ndn ) ) {
			rc = -1;
		}
		break;
			
	case LDAP_SIZELIMIT_EXCEEDED:
	default:
		if ( !BER_BVISNULL( ndn ) ) {
			ber_memfree( ndn->bv_val );
			BER_BVZERO( ndn );
		}
		rc = -1;
		break;
	}

	return rc;
}

int
monitor_back_register_entry_attrs(
	struct berval		*ndn_in,
	Attribute		*a,
	monitor_callback_t	*cb,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	monitor_info_t 	*mi;
	struct berval	ndn = BER_BVNULL;
	char		*fname = ( a == NULL ? "callback" : "attrs" );
	struct berval	empty_bv = BER_BVC("");

	if ( nbase == NULL ) nbase = &empty_bv;
	if ( filter == NULL ) filter = &empty_bv;

	if ( be_monitor == NULL ) {
		char		buf[ SLAP_TEXT_BUFLEN ];

		snprintf( buf, sizeof( buf ),
			"monitor_back_register_entry_%s(base=\"%s\" scope=%s filter=\"%s\"): "
			"monitor database not configured.\n",
			fname,
			BER_BVISNULL( nbase ) ? "" : nbase->bv_val,
			ldap_pvt_scope2str( scope ),
			BER_BVISNULL( filter ) ? "" : filter->bv_val );
		Debug( LDAP_DEBUG_ANY, "%s\n", buf, 0, 0 );

		return -1;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );

	if ( ndn_in != NULL ) {
		ndn = *ndn_in;
	}

	if ( a == NULL && cb == NULL ) {
		/* nothing to do */
		return -1;
	}

	if ( ( ndn_in == NULL || BER_BVISNULL( &ndn ) )
			&& BER_BVISNULL( filter ) )
	{
		/* need a filter */
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_register_entry_%s(\"\"): "
			"need a valid filter\n",
			fname, 0, 0 );
		return -1;
	}

	if ( monitor_subsys_is_opened() ) {
		Entry			*e = NULL;
		Attribute		**atp = NULL;
		monitor_entry_t 	*mp = NULL;
		monitor_callback_t	**mcp = NULL;
		int			rc = 0;
		int			freeit = 0;

		if ( BER_BVISNULL( &ndn ) ) {
			if ( monitor_search2ndn( nbase, scope, filter, &ndn ) ) {
				char		buf[ SLAP_TEXT_BUFLEN ];

				snprintf( buf, sizeof( buf ),
					"monitor_back_register_entry_%s(\"\"): "
					"base=\"%s\" scope=%s filter=\"%s\": "
					"unable to find entry\n",
					fname,
					nbase->bv_val ? nbase->bv_val : "\"\"",
					ldap_pvt_scope2str( scope ),
					filter->bv_val );

				/* entry does not exist */
				Debug( LDAP_DEBUG_ANY, "%s\n", buf, 0, 0 );
				return -1;
			}

			freeit = 1;
		}

		if ( monitor_cache_get( mi, &ndn, &e ) != 0 ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_%s(\"%s\"): "
				"entry does not exist\n",
				fname, ndn.bv_val, 0 );
			rc = -1;
			goto done;
		}

		assert( e->e_private != NULL );
		mp = ( monitor_entry_t * )e->e_private;

		if ( mp->mp_flags & MONITOR_F_VOLATILE ) {
			/* entry is volatile; cannot append callback */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_register_entry_%s(\"%s\"): "
				"entry is volatile\n",
				fname, e->e_name.bv_val, 0 );
			rc = -1;
			goto done;
		}

		if ( a ) {
			for ( atp = &e->e_attrs; *atp; atp = &(*atp)->a_next )
				/* just get to last */ ;

			for ( ; a != NULL; a = a->a_next ) {
				assert( a->a_desc != NULL );
				assert( a->a_vals != NULL );

				if ( attr_find( e->e_attrs, a->a_desc ) ) {
					attr_merge( e, a->a_desc, a->a_vals,
						a->a_nvals == a->a_vals ? NULL : a->a_nvals );

				} else {
					*atp = attr_dup( a );
					if ( *atp == NULL ) {
						Debug( LDAP_DEBUG_ANY,
							"monitor_back_register_entry_%s(\"%s\"): "
							"attr_dup() failed\n",
							fname, e->e_name.bv_val, 0 );
						rc = -1;
						goto done;
					}
					atp = &(*atp)->a_next;
				}
			}
		}

		if ( cb ) {
			for ( mcp = &mp->mp_cb; *mcp; mcp = &(*mcp)->mc_next )
				/* go to tail */ ;
		
			/* NOTE: we do not clear cb->mc_next, so this function
			 * can be used to append a list of callbacks */
			(*mcp) = cb;
		}

done:;
		if ( rc ) {
			if ( atp && *atp ) {
				attrs_free( *atp );
				*atp = NULL;
			}
		}

		if ( freeit ) {
			ber_memfree( ndn.bv_val );
		}

		if ( e ) {
			monitor_cache_release( mi, e );
		}

	} else {
		entry_limbo_t	**elpp, el = { 0 };

		el.el_type = LIMBO_ATTRS;
		el.el_ndn = ndn_in;
		if ( !BER_BVISNULL( nbase ) ) {
			ber_dupbv( &el.el_nbase, nbase);
		}
		el.el_scope = scope;
		if ( !BER_BVISNULL( filter ) ) {
			ber_dupbv( &el.el_filter, filter  );
		}

		el.el_a = attrs_dup( a );
		el.el_cb = cb;

		for ( elpp = &mi->mi_entry_limbo;
				*elpp;
				elpp = &(*elpp)->el_next )
			/* go to last */;

		*elpp = (entry_limbo_t *)ch_malloc( sizeof( entry_limbo_t ) );
		if ( *elpp == NULL ) {
			if ( !BER_BVISNULL( &el.el_filter ) ) {
				ch_free( el.el_filter.bv_val );
			}
			if ( el.el_a != NULL ) {
				attrs_free( el.el_a );
			}
			if ( !BER_BVISNULL( &el.el_nbase ) ) {
				ch_free( &el.el_nbase.bv_val );
			}
			return -1;
		}

		el.el_next = NULL;
		**elpp = el;
	}

	return 0;
}

int
monitor_back_register_entry_callback(
	struct berval		*ndn,
	monitor_callback_t	*cb,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	return monitor_back_register_entry_attrs( ndn, NULL, cb,
			nbase, scope, filter );
}

/*
 * TODO: add corresponding calls to remove installed callbacks, entries
 * and so, in case the entity that installed them is removed (e.g. a 
 * database, via back-config)
 */
int
monitor_back_unregister_entry(
	struct berval	*ndn )
{
	monitor_info_t 	*mi;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_unregister_entry(\"%s\"): "
			"monitor database not configured.\n",
			ndn->bv_val, 0, 0 );

		return -1;
	}

	/* entry will be regularly freed, and resources released
	 * according to callbacks */
	if ( slapd_shutdown ) {
		return 0;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );

	if ( monitor_subsys_is_opened() ) {
		Entry			*e = NULL;
		monitor_entry_t 	*mp = NULL;
		monitor_callback_t	*cb = NULL;

		if ( monitor_cache_remove( mi, ndn, &e ) != 0 ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_unregister_entry(\"%s\"): "
				"entry removal failed.\n",
				ndn->bv_val, 0, 0 );
			return -1;
		}

		mp = (monitor_entry_t *)e->e_private;
		assert( mp != NULL );

		for ( cb = mp->mp_cb; cb != NULL; ) {
			monitor_callback_t	*next = cb->mc_next;

			if ( cb->mc_free ) {
				(void)cb->mc_free( e, &cb->mc_private );
			}
			ch_free( cb );

			cb = next;
		}

		ch_free( mp );
		e->e_private = NULL;
		entry_free( e );

	} else {
		entry_limbo_t	**elpp;

		for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		{
			entry_limbo_t	*elp = *elpp;

			if ( elp->el_type == LIMBO_ENTRY
				&& dn_match( ndn, &elp->el_e->e_nname ) )
			{
				monitor_callback_t	*cb, *next;

				for ( cb = elp->el_cb; cb; cb = next ) {
					/* FIXME: call callbacks? */
					next = cb->mc_next;
					if ( cb->mc_dispose ) {
						cb->mc_dispose( &cb->mc_private );
					}
					ch_free( cb );
				}
				assert( elp->el_e != NULL );
				elp->el_e->e_private = NULL;
				entry_free( elp->el_e );
				*elpp = elp->el_next;
				ch_free( elp );
				elpp = NULL;
				break;
			}
		}

		if ( elpp != NULL ) {
			/* not found!  where did it go? */
			return 1;
		}
	}

	return 0;
}

int
monitor_back_unregister_entry_parent(
	struct berval		*nrdn,
	monitor_callback_t	*target_cb,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	monitor_info_t 	*mi;
	struct berval	ndn = BER_BVNULL;

	if ( be_monitor == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_unregister_entry_parent(base=\"%s\" scope=%s filter=\"%s\"): "
			"monitor database not configured.\n",
			BER_BVISNULL( nbase ) ? "" : nbase->bv_val,
			ldap_pvt_scope2str( scope ),
			BER_BVISNULL( filter ) ? "" : filter->bv_val );

		return -1;
	}

	/* entry will be regularly freed, and resources released
	 * according to callbacks */
	if ( slapd_shutdown ) {
		return 0;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );

	if ( ( nrdn == NULL || BER_BVISNULL( nrdn ) )
			&& BER_BVISNULL( filter ) )
	{
		/* need a filter */
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_unregister_entry_parent(\"\"): "
			"need a valid filter\n",
			0, 0, 0 );
		return -1;
	}

	if ( monitor_subsys_is_opened() ) {
		Entry			*e = NULL;
		monitor_entry_t 	*mp = NULL;

		if ( monitor_search2ndn( nbase, scope, filter, &ndn ) ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_unregister_entry_parent(\"\"): "
				"base=\"%s\" scope=%s filter=\"%s\": "
				"unable to find entry\n",
				nbase->bv_val ? nbase->bv_val : "\"\"",
				ldap_pvt_scope2str( scope ),
				filter->bv_val );
			return -1;
		}

		if ( monitor_cache_remove( mi, &ndn, &e ) != 0 ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_unregister_entry(\"%s\"): "
				"entry removal failed.\n",
				ndn.bv_val, 0, 0 );
			ber_memfree( ndn.bv_val );
			return -1;
		}
		ber_memfree( ndn.bv_val );

		mp = (monitor_entry_t *)e->e_private;
		assert( mp != NULL );

		if ( target_cb != NULL ) {
			monitor_callback_t	**cbp;

			for ( cbp = &mp->mp_cb; *cbp != NULL; cbp = &(*cbp)->mc_next ) {
				if ( *cbp == target_cb ) {
					if ( (*cbp)->mc_free ) {
						(void)(*cbp)->mc_free( e, &(*cbp)->mc_private );
					}
					*cbp = (*cbp)->mc_next;
					ch_free( target_cb );
					break;
				}
			}
		}


		ch_free( mp );
		e->e_private = NULL;
		entry_free( e );

	} else {
		entry_limbo_t	**elpp;

		for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		{
			entry_limbo_t	*elp = *elpp;

			if ( elp->el_type == LIMBO_ENTRY_PARENT
				&& dn_match( nrdn, &elp->el_e->e_nname )
				&& dn_match( nbase, &elp->el_nbase )
				&& scope == elp->el_scope
				&& bvmatch( filter, &elp->el_filter ) )
			{
				monitor_callback_t	*cb, *next;

				for ( cb = elp->el_cb; cb; cb = next ) {
					/* FIXME: call callbacks? */
					next = cb->mc_next;
					if ( cb->mc_dispose ) {
						cb->mc_dispose( &cb->mc_private );
					}
					ch_free( cb );
				}
				assert( elp->el_e != NULL );
				elp->el_e->e_private = NULL;
				entry_free( elp->el_e );
				if ( !BER_BVISNULL( &elp->el_nbase ) ) {
					ch_free( elp->el_nbase.bv_val );
				}
				if ( !BER_BVISNULL( &elp->el_filter ) ) {
					ch_free( elp->el_filter.bv_val );
				}
				*elpp = elp->el_next;
				ch_free( elp );
				elpp = NULL;
				break;
			}
		}

		if ( elpp != NULL ) {
			/* not found!  where did it go? */
			return 1;
		}
	}

	return 0;
}

int
monitor_back_unregister_entry_attrs(
	struct berval		*ndn_in,
	Attribute		*target_a,
	monitor_callback_t	*target_cb,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	monitor_info_t 	*mi;
	struct berval	ndn = BER_BVNULL;
	char		*fname = ( target_a == NULL ? "callback" : "attrs" );

	if ( be_monitor == NULL ) {
		char		buf[ SLAP_TEXT_BUFLEN ];

		snprintf( buf, sizeof( buf ),
			"monitor_back_unregister_entry_%s(base=\"%s\" scope=%s filter=\"%s\"): "
			"monitor database not configured.\n",
			fname,
			BER_BVISNULL( nbase ) ? "" : nbase->bv_val,
			ldap_pvt_scope2str( scope ),
			BER_BVISNULL( filter ) ? "" : filter->bv_val );
		Debug( LDAP_DEBUG_ANY, "%s\n", buf, 0, 0 );

		return -1;
	}

	/* entry will be regularly freed, and resources released
	 * according to callbacks */
	if ( slapd_shutdown ) {
		return 0;
	}

	mi = ( monitor_info_t * )be_monitor->be_private;

	assert( mi != NULL );

	if ( ndn_in != NULL ) {
		ndn = *ndn_in;
	}

	if ( target_a == NULL && target_cb == NULL ) {
		/* nothing to do */
		return -1;
	}

	if ( ( ndn_in == NULL || BER_BVISNULL( &ndn ) )
			&& BER_BVISNULL( filter ) )
	{
		/* need a filter */
		Debug( LDAP_DEBUG_ANY,
			"monitor_back_unregister_entry_%s(\"\"): "
			"need a valid filter\n",
			fname, 0, 0 );
		return -1;
	}

	if ( monitor_subsys_is_opened() ) {
		Entry			*e = NULL;
		monitor_entry_t 	*mp = NULL;
		int			freeit = 0;

		if ( BER_BVISNULL( &ndn ) ) {
			if ( monitor_search2ndn( nbase, scope, filter, &ndn ) ) {
				char		buf[ SLAP_TEXT_BUFLEN ];

				snprintf( buf, sizeof( buf ),
					"monitor_back_unregister_entry_%s(\"\"): "
					"base=\"%s\" scope=%d filter=\"%s\": "
					"unable to find entry\n",
					fname,
					nbase->bv_val ? nbase->bv_val : "\"\"",
					scope, filter->bv_val );

				/* entry does not exist */
				Debug( LDAP_DEBUG_ANY, "%s\n", buf, 0, 0 );
				return -1;
			}

			freeit = 1;
		}

		if ( monitor_cache_get( mi, &ndn, &e ) != 0 ) {
			/* entry does not exist */
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_unregister_entry(\"%s\"): "
				"entry removal failed.\n",
				ndn.bv_val, 0, 0 );
			return -1;
		}

		mp = (monitor_entry_t *)e->e_private;
		assert( mp != NULL );

		if ( target_cb != NULL ) {
			monitor_callback_t	**cbp;

			for ( cbp = &mp->mp_cb; *cbp != NULL; cbp = &(*cbp)->mc_next ) {
				if ( *cbp == target_cb ) {
					if ( (*cbp)->mc_free ) {
						(void)(*cbp)->mc_free( e, &(*cbp)->mc_private );
					}
					*cbp = (*cbp)->mc_next;
					ch_free( target_cb );
					break;
				}
			}
		}

		if ( target_a != NULL ) {
			Attribute	*a;

			for ( a = target_a; a != NULL; a = a->a_next ) {
				Modification	mod = { 0 };
				const char	*text;
				char		textbuf[ SLAP_TEXT_BUFLEN ];

				mod.sm_op = LDAP_MOD_DELETE;
				mod.sm_desc = a->a_desc;
				mod.sm_values = a->a_vals;
				mod.sm_nvalues = a->a_nvals;

				(void)modify_delete_values( e, &mod, 1,
					&text, textbuf, sizeof( textbuf ) );
			}
		}

		if ( freeit ) {
			ber_memfree( ndn.bv_val );
		}

		monitor_cache_release( mi, e );

	} else {
		entry_limbo_t	**elpp;

		for ( elpp = &mi->mi_entry_limbo;
			*elpp;
			elpp = &(*elpp)->el_next )
		{
			entry_limbo_t	*elp = *elpp;

			if ( elp->el_type == LIMBO_ATTRS
				&& dn_match( nbase, &elp->el_nbase )
				&& scope == elp->el_scope
				&& bvmatch( filter, &elp->el_filter ) )
			{
				monitor_callback_t	*cb, *next;

				for ( cb = elp->el_cb; cb; cb = next ) {
					/* FIXME: call callbacks? */
					next = cb->mc_next;
					if ( cb->mc_dispose ) {
						cb->mc_dispose( &cb->mc_private );
					}
					ch_free( cb );
				}
				assert( elp->el_e == NULL );
				if ( elp->el_a != NULL ) {
					attrs_free( elp->el_a );
				}
				if ( !BER_BVISNULL( &elp->el_nbase ) ) {
					ch_free( elp->el_nbase.bv_val );
				}
				if ( !BER_BVISNULL( &elp->el_filter ) ) {
					ch_free( elp->el_filter.bv_val );
				}
				*elpp = elp->el_next;
				ch_free( elp );
				elpp = NULL;
				break;
			}
		}

		if ( elpp != NULL ) {
			/* not found!  where did it go? */
			return 1;
		}
	}

	return 0;
}

int
monitor_back_unregister_entry_callback(
	struct berval		*ndn,
	monitor_callback_t	*cb,
	struct berval		*nbase,
	int			scope,
	struct berval		*filter )
{
	/* TODO: lookup entry (by ndn, if not NULL, and/or by callback);
	 * unregister the callback; if a is not null, unregister the
	 * given attrs.  In any case, call cb->cb_free */
	return monitor_back_unregister_entry_attrs( ndn,
		NULL, cb, nbase, scope, filter );
}

monitor_subsys_t *
monitor_back_get_subsys( const char *name )
{
	if ( monitor_subsys != NULL ) {
		int	i;
		
		for ( i = 0; monitor_subsys[ i ] != NULL; i++ ) {
			if ( strcasecmp( monitor_subsys[ i ]->mss_name, name ) == 0 ) {
				return monitor_subsys[ i ];
			}
		}
	}

	return NULL;
}

monitor_subsys_t *
monitor_back_get_subsys_by_dn(
	struct berval	*ndn,
	int		sub )
{
	if ( monitor_subsys != NULL ) {
		int	i;

		if ( sub ) {
			for ( i = 0; monitor_subsys[ i ] != NULL; i++ ) {
				if ( dnIsSuffix( ndn, &monitor_subsys[ i ]->mss_ndn ) ) {
					return monitor_subsys[ i ];
				}
			}

		} else {
			for ( i = 0; monitor_subsys[ i ] != NULL; i++ ) {
				if ( dn_match( ndn, &monitor_subsys[ i ]->mss_ndn ) ) {
					return monitor_subsys[ i ];
				}
			}
		}
	}

	return NULL;
}

int
monitor_back_initialize(
	BackendInfo	*bi )
{
	static char		*controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		NULL
	};

	static ConfigTable monitorcfg[] = {
		{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
			NULL, NULL, NULL, NULL }
	};

	static ConfigOCs monitorocs[] = {
		{ "( OLcfgDbOc:4.1 "
			"NAME 'olcMonitorConfig' "
			"DESC 'Monitor backend configuration' "
			"SUP olcDatabaseConfig "
			")",
			 	Cft_Database, monitorcfg },
		{ NULL, 0, NULL }
	};

	struct m_s {
		char	*schema;
		slap_mask_t flags;
		int	offset;
	} moc[] = {
		{ "( 1.3.6.1.4.1.4203.666.3.16.1 "
			"NAME 'monitor' "
			"DESC 'OpenLDAP system monitoring' "
			"SUP top STRUCTURAL "
			"MUST cn "
			"MAY ( "
				"description "
				"$ seeAlso "
				"$ labeledURI "
				"$ monitoredInfo "
				"$ managedInfo "
				"$ monitorOverlay "
			") )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitor) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.2 "
			"NAME 'monitorServer' "
			"DESC 'Server monitoring root entry' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitorServer) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.3 "
			"NAME 'monitorContainer' "
			"DESC 'monitor container class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitorContainer) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.4 "
			"NAME 'monitorCounterObject' "
			"DESC 'monitor counter class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitorCounterObject) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.5 "
			"NAME 'monitorOperation' "
			"DESC 'monitor operation class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitorOperation) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.6 "
			"NAME 'monitorConnection' "
			"DESC 'monitor connection class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitorConnection) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.7 "
			"NAME 'managedObject' "
			"DESC 'monitor managed entity class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_managedObject) },
		{ "( 1.3.6.1.4.1.4203.666.3.16.8 "
			"NAME 'monitoredObject' "
			"DESC 'monitor monitored entity class' "
			"SUP monitor STRUCTURAL )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
			offsetof(monitor_info_t, mi_oc_monitoredObject) },
		{ NULL, 0, -1 }
	}, mat[] = {
		{ "( 1.3.6.1.4.1.4203.666.1.55.1 "
			"NAME 'monitoredInfo' "
			"DESC 'monitored info' "
			/* "SUP name " */
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{32768} "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitoredInfo) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.2 "
			"NAME 'managedInfo' "
			"DESC 'monitor managed info' "
			"SUP name )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_managedInfo) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.3 "
			"NAME 'monitorCounter' "
			"DESC 'monitor counter' "
			"EQUALITY integerMatch "
			"ORDERING integerOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorCounter) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.4 "
			"NAME 'monitorOpCompleted' "
			"DESC 'monitor completed operations' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorOpCompleted) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.5 "
			"NAME 'monitorOpInitiated' "
			"DESC 'monitor initiated operations' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorOpInitiated) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.6 "
			"NAME 'monitorConnectionNumber' "
			"DESC 'monitor connection number' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionNumber) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.7 "
			"NAME 'monitorConnectionAuthzDN' "
			"DESC 'monitor connection authorization DN' "
			/* "SUP distinguishedName " */
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionAuthzDN) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.8 "
			"NAME 'monitorConnectionLocalAddress' "
			"DESC 'monitor connection local address' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionLocalAddress) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.9 "
			"NAME 'monitorConnectionPeerAddress' "
			"DESC 'monitor connection peer address' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionPeerAddress) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.10 "
			"NAME 'monitorTimestamp' "
			"DESC 'monitor timestamp' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorTimestamp) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.11 "
			"NAME 'monitorOverlay' "
			"DESC 'name of overlays defined for a given database' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorOverlay) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.12 "
			"NAME 'readOnly' "
			"DESC 'read/write status of a given database' "
			"EQUALITY booleanMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_readOnly) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.13 "
			"NAME 'restrictedOperation' "
			"DESC 'name of restricted operation for a given database' "
			"SUP managedInfo )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_restrictedOperation ) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.14 "
			"NAME 'monitorConnectionProtocol' "
			"DESC 'monitor connection protocol' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionProtocol) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.15 "
			"NAME 'monitorConnectionOpsReceived' "
			"DESC 'monitor number of operations received by the connection' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionOpsReceived) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.16 "
			"NAME 'monitorConnectionOpsExecuting' "
			"DESC 'monitor number of operations in execution within the connection' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionOpsExecuting) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.17 "
			"NAME 'monitorConnectionOpsPending' "
			"DESC 'monitor number of pending operations within the connection' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionOpsPending) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.18 "
			"NAME 'monitorConnectionOpsCompleted' "
			"DESC 'monitor number of operations completed within the connection' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionOpsCompleted) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.19 "
			"NAME 'monitorConnectionGet' "
			"DESC 'number of times connection_get() was called so far' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionGet) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.20 "
			"NAME 'monitorConnectionRead' "
			"DESC 'number of times connection_read() was called so far' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionRead) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.21 "
			"NAME 'monitorConnectionWrite' "
			"DESC 'number of times connection_write() was called so far' "
			"SUP monitorCounter "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionWrite) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.22 "
			"NAME 'monitorConnectionMask' "
			"DESC 'monitor connection mask' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionMask) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.23 "
			"NAME 'monitorConnectionListener' "
			"DESC 'monitor connection listener' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionListener) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.24 "
			"NAME 'monitorConnectionPeerDomain' "
			"DESC 'monitor connection peer domain' "
			"SUP monitoredInfo "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionPeerDomain) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.25 "
			"NAME 'monitorConnectionStartTime' "
			"DESC 'monitor connection start time' "
			"SUP monitorTimestamp "
			"SINGLE-VALUE "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionStartTime) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.26 "
			"NAME 'monitorConnectionActivityTime' "
			"DESC 'monitor connection activity time' "
			"SUP monitorTimestamp "
			"SINGLE-VALUE "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorConnectionActivityTime) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.27 "
			"NAME 'monitorIsShadow' "
			"DESC 'TRUE if the database is shadow' "
			"EQUALITY booleanMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorIsShadow) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.28 "
			"NAME 'monitorUpdateRef' "
			"DESC 'update referral for shadow databases' "
			"SUP monitoredInfo "
			"SINGLE-VALUE "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorUpdateRef) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.29 "
			"NAME 'monitorRuntimeConfig' "
			"DESC 'TRUE if component allows runtime configuration' "
			"EQUALITY booleanMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE "
			"USAGE dSAOperation )", SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorRuntimeConfig) },
		{ "( 1.3.6.1.4.1.4203.666.1.55.30 "
			"NAME 'monitorSuperiorDN' "
			"DESC 'monitor superior DN' "
			/* "SUP distinguishedName " */
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"NO-USER-MODIFICATION "
			"USAGE dSAOperation )", SLAP_AT_FINAL|SLAP_AT_HIDE,
			offsetof(monitor_info_t, mi_ad_monitorSuperiorDN) },
		{ NULL, 0, -1 }
	};

	static struct {
		char			*name;
		char			*oid;
	}		s_oid[] = {
		{ "olmAttributes",			"1.3.6.1.4.1.4203.666.1.55" },
		{ "olmSubSystemAttributes",		"olmAttributes:0" },
		{ "olmGenericAttributes",		"olmSubSystemAttributes:0" },
		{ "olmDatabaseAttributes",		"olmSubSystemAttributes:1" },

		/* for example, back-bdb specific attrs
		 * are in "olmDatabaseAttributes:1"
		 *
		 * NOTE: developers, please record here OID assignments
		 * for other modules */

		{ "olmObjectClasses",			"1.3.6.1.4.1.4203.666.3.16" },
		{ "olmSubSystemObjectClasses",		"olmObjectClasses:0" },
		{ "olmGenericObjectClasses",		"olmSubSystemObjectClasses:0" },
		{ "olmDatabaseObjectClasses",		"olmSubSystemObjectClasses:1" },

		/* for example, back-bdb specific objectClasses
		 * are in "olmDatabaseObjectClasses:1"
		 *
		 * NOTE: developers, please record here OID assignments
		 * for other modules */

		{ NULL }
	};

	int			i, rc;
	monitor_info_t		*mi = &monitor_info;
	ConfigArgs c;
	char	*argv[ 3 ];

	argv[ 0 ] = "monitor";
	c.argv = argv;
	c.argc = 3;
	c.fname = argv[0];

	for ( i = 0; s_oid[ i ].name; i++ ) {
		argv[ 1 ] = s_oid[ i ].name;
		argv[ 2 ] = s_oid[ i ].oid;

		if ( parse_oidm( &c, 0, NULL ) != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_initialize: unable to add "
				"objectIdentifier \"%s=%s\"\n",
				s_oid[ i ].name, s_oid[ i ].oid, 0 );
			return 1;
		}
	}

	/* schema integration */
	for ( i = 0; mat[ i ].schema; i++ ) {
		int			code;
		AttributeDescription **ad =
			((AttributeDescription **)&(((char *)mi)[ mat[ i ].offset ]));

		*ad = NULL;
		code = register_at( mat[ i ].schema, ad, 0 );

		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_db_init: register_at failed\n", 0, 0, 0 );
			return -1;
		}
		(*ad)->ad_type->sat_flags |= mat[ i ].flags;
	}

	for ( i = 0; moc[ i ].schema; i++ ) {
		int			code;
		ObjectClass		**Oc =
			((ObjectClass **)&(((char *)mi)[ moc[ i ].offset ]));

		code = register_oc( moc[ i ].schema, Oc, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_back_db_init: register_oc failed\n", 0, 0, 0 );
			return -1;
		}
		(*Oc)->soc_flags |= moc[ i ].flags;
	}

	bi->bi_controls = controls;

	bi->bi_init = 0;
	bi->bi_open = 0;
	bi->bi_config = monitor_back_config;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = monitor_back_db_init;
#if 0
	bi->bi_db_config = monitor_back_db_config;
#endif
	bi->bi_db_open = monitor_back_db_open;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = monitor_back_db_destroy;

	bi->bi_op_bind = monitor_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = monitor_back_search;
	bi->bi_op_compare = monitor_back_compare;
	bi->bi_op_modify = monitor_back_modify;
	bi->bi_op_modrdn = 0;
	bi->bi_op_add = 0;
	bi->bi_op_delete = 0;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_entry_release_rw = monitor_back_release;
	bi->bi_chk_referrals = 0;
	bi->bi_operational = monitor_back_operational;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = 0;
	bi->bi_tool_entry_close = 0;
	bi->bi_tool_entry_first = 0;
	bi->bi_tool_entry_first_x = 0;
	bi->bi_tool_entry_next = 0;
	bi->bi_tool_entry_get = 0;
	bi->bi_tool_entry_put = 0;
	bi->bi_tool_entry_reindex = 0;
	bi->bi_tool_sync = 0;
	bi->bi_tool_dn2id_get = 0;
	bi->bi_tool_entry_modify = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_extra = (void *)&monitor_extra;

	/*
	 * configuration objectClasses (fake)
	 */
	bi->bi_cf_ocs = monitorocs;

	rc = config_register_schema( monitorcfg, monitorocs );
	if ( rc ) {
		return rc;
	}

	return 0;
}

int
monitor_back_db_init(
	BackendDB	*be,
	ConfigReply	*c)
{
	int			rc;
	struct berval		dn = BER_BVC( SLAPD_MONITOR_DN ),
				pdn,
				ndn;
	BackendDB		*be2;

	monitor_subsys_t	*ms;

	/*
	 * database monitor can be defined once only
	 */
	if ( be_monitor != NULL ) {
		if (c) {
			snprintf(c->msg, sizeof(c->msg),"only one monitor database allowed");
		}
		return( -1 );
	}
	be_monitor = be;

	/*
	 * register subsys
	 */
	for ( ms = known_monitor_subsys; ms->mss_name != NULL; ms++ ) {
		if ( monitor_back_register_subsys( ms ) ) {
			return -1;
		}
	}

	/* indicate system schema supported */
	SLAP_BFLAGS(be) |= SLAP_BFLAG_MONITOR;

	rc = dnPrettyNormal( NULL, &dn, &pdn, &ndn, NULL );
	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"unable to normalize/pretty monitor DN \"%s\" (%d)\n",
			dn.bv_val, rc, 0 );
		return -1;
	}

	ber_bvarray_add( &be->be_suffix, &pdn );
	ber_bvarray_add( &be->be_nsuffix, &ndn );

	/* NOTE: only one monitor database is allowed,
	 * so we use static storage */
	ldap_pvt_thread_mutex_init( &monitor_info.mi_cache_mutex );

	be->be_private = &monitor_info;

	be2 = select_backend( &ndn, 0 );
	if ( be2 != be ) {
		char	*type = be2->bd_info->bi_type;

		if ( overlay_is_over( be2 ) ) {
			slap_overinfo	*oi = (slap_overinfo *)be2->bd_info->bi_private;
			type = oi->oi_orig->bi_type;
		}

		if (c) {
			snprintf(c->msg, sizeof(c->msg),
					"\"monitor\" database serving namingContext \"%s\" "
					"is hidden by \"%s\" database serving namingContext \"%s\".\n",
					pdn.bv_val, type, be2->be_nsuffix[ 0 ].bv_val );
		}
		return -1;
	}

	return 0;
}

static void
monitor_back_destroy_limbo_entry(
	entry_limbo_t	*el,
	int		dispose )
{
	if ( el->el_e ) {
		entry_free( el->el_e );
	}
	if ( el->el_a ) {
		attrs_free( el->el_a );
	}
	if ( !BER_BVISNULL( &el->el_nbase ) ) {
		ber_memfree( el->el_nbase.bv_val );
	}
	if ( !BER_BVISNULL( &el->el_filter ) ) {
		ber_memfree( el->el_filter.bv_val );
	}

	/* NOTE: callbacks are not copied; so only free them
	 * if disposing of */
	if ( el->el_cb && dispose != 0 ) {
		monitor_callback_t *next;

		for ( ; el->el_cb; el->el_cb = next ) {
			next = el->el_cb->mc_next;
			if ( el->el_cb->mc_dispose ) {
				el->el_cb->mc_dispose( &el->el_cb->mc_private );
			}
			ch_free( el->el_cb );
		}
	}

	ch_free( el );
}

int
monitor_back_db_open(
	BackendDB	*be,
	ConfigReply	*cr)
{
	monitor_info_t 		*mi = (monitor_info_t *)be->be_private;
	struct monitor_subsys_t	**ms;
	Entry 			*e, **ep, *root;
	monitor_entry_t		*mp;
	int			i;
	struct berval		bv, rdn = BER_BVC(SLAPD_MONITOR_DN);
	struct tm		tms;
	static char		tmbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	struct berval	desc[] = {
		BER_BVC("This subtree contains monitoring/managing objects."),
		BER_BVC("This object contains information about this server."),
		BER_BVC("Most of the information is held in operational"
		" attributes, which must be explicitly requested."),
		BER_BVNULL };

	int			retcode = 0;

	assert( be_monitor != NULL );
	if ( be != be_monitor ) {
		be_monitor = be;
	}

	/*
	 * Start
	 */
	ldap_pvt_gmtime( &starttime, &tms );
	lutil_gentime( tmbuf, sizeof(tmbuf), &tms );

	mi->mi_startTime.bv_val = tmbuf;
	mi->mi_startTime.bv_len = strlen( tmbuf );

	if ( BER_BVISEMPTY( &be->be_rootdn ) ) {
		BER_BVSTR( &mi->mi_creatorsName, SLAPD_ANONYMOUS );
		BER_BVSTR( &mi->mi_ncreatorsName, SLAPD_ANONYMOUS );
	} else {
		mi->mi_creatorsName = be->be_rootdn;
		mi->mi_ncreatorsName = be->be_rootndn;
	}

	/*
	 * creates the "cn=Monitor" entry 
	 */
	e = monitor_entry_stub( NULL, NULL, &rdn, mi->mi_oc_monitorServer,
		NULL, NULL );

	if ( e == NULL) {
		Debug( LDAP_DEBUG_ANY,
			"unable to create \"%s\" entry\n",
			SLAPD_MONITOR_DN, 0, 0 );
		return( -1 );
	}

	attr_merge_normalize( e, slap_schema.si_ad_description, desc, NULL );

	bv.bv_val = strchr( (char *) Versionstr, '$' );
	if ( bv.bv_val != NULL ) {
		char	*end;

		bv.bv_val++;
		for ( ; bv.bv_val[ 0 ] == ' '; bv.bv_val++ )
			;

		end = strchr( bv.bv_val, '$' );
		if ( end != NULL ) {
			end--;

			for ( ; end > bv.bv_val && end[ 0 ] == ' '; end-- )
				;

			end++;

			bv.bv_len = end - bv.bv_val;

		} else {
			bv.bv_len = strlen( bv.bv_val );
		}

		if ( attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo,
					&bv, NULL ) ) {
			Debug( LDAP_DEBUG_ANY,
				"unable to add monitoredInfo to \"%s\" entry\n",
				SLAPD_MONITOR_DN, 0, 0 );
			return( -1 );
		}
	}

	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		return -1;
	}
	e->e_private = ( void * )mp;
	ep = &mp->mp_children;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"unable to add entry \"%s\" to cache\n",
			SLAPD_MONITOR_DN, 0, 0 );
		return -1;
	}
	root = e;

	/*	
	 * Create all the subsystem specific entries
	 */
	for ( i = 0; monitor_subsys[ i ] != NULL; i++ ) {
		int 		len = strlen( monitor_subsys[ i ]->mss_name );
		struct berval	dn;
		int		rc;

		dn.bv_len = len + sizeof( "cn=" ) - 1;
		dn.bv_val = ch_calloc( sizeof( char ), dn.bv_len + 1 );
		strcpy( dn.bv_val, "cn=" );
		strcat( dn.bv_val, monitor_subsys[ i ]->mss_name );
		rc = dnPretty( NULL, &dn, &monitor_subsys[ i ]->mss_rdn, NULL );
		free( dn.bv_val );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor RDN \"%s\" is invalid\n", 
				dn.bv_val, 0, 0 );
			return( -1 );
		}

		e = monitor_entry_stub( &root->e_name, &root->e_nname,
			&monitor_subsys[ i ]->mss_rdn, mi->mi_oc_monitorContainer,
			NULL, NULL );

		if ( e == NULL) {
			Debug( LDAP_DEBUG_ANY,
				"unable to create \"%s\" entry\n", 
				monitor_subsys[ i ]->mss_dn.bv_val, 0, 0 );
			return( -1 );
		}
		monitor_subsys[i]->mss_dn = e->e_name;
		monitor_subsys[i]->mss_ndn = e->e_nname;

		if ( !BER_BVISNULL( &monitor_subsys[ i ]->mss_desc[ 0 ] ) ) {
			attr_merge_normalize( e, slap_schema.si_ad_description,
					monitor_subsys[ i ]->mss_desc, NULL );
		}

		mp = monitor_entrypriv_create();
		if ( mp == NULL ) {
			return -1;
		}
		e->e_private = ( void * )mp;
		mp->mp_info = monitor_subsys[ i ];
		mp->mp_flags = monitor_subsys[ i ]->mss_flags;

		if ( monitor_cache_add( mi, e ) ) {
			Debug( LDAP_DEBUG_ANY,
				"unable to add entry \"%s\" to cache\n",
				monitor_subsys[ i ]->mss_dn.bv_val, 0, 0 );
			return -1;
		}

		*ep = e;
		ep = &mp->mp_next;
	}

	assert( be != NULL );

	be->be_private = mi;
	
	/*
	 * opens the monitor backend subsystems
	 */
	for ( ms = monitor_subsys; ms[ 0 ] != NULL; ms++ ) {
		if ( ms[ 0 ]->mss_open && ms[ 0 ]->mss_open( be, ms[ 0 ] ) ) {
			return( -1 );
		}
		ms[ 0 ]->mss_flags |= MONITOR_F_OPENED;
	}

	monitor_subsys_opened = 1;

	if ( mi->mi_entry_limbo ) {
		entry_limbo_t	*el = mi->mi_entry_limbo;

		for ( ; el; ) {
			entry_limbo_t	*tmp;
			int		rc;

			switch ( el->el_type ) {
			case LIMBO_ENTRY:
				rc = monitor_back_register_entry(
						el->el_e,
						el->el_cb,
						el->el_mss,
						el->el_flags );
				break;

			case LIMBO_ENTRY_PARENT:
				rc = monitor_back_register_entry_parent(
						el->el_e,
						el->el_cb,
						el->el_mss,
						el->el_flags,
						&el->el_nbase,
						el->el_scope,
						&el->el_filter );
				break;
				

			case LIMBO_ATTRS:
				rc = monitor_back_register_entry_attrs(
						el->el_ndn,
						el->el_a,
						el->el_cb,
						&el->el_nbase,
						el->el_scope,
						&el->el_filter );
				break;

			case LIMBO_CB:
				rc = monitor_back_register_entry_callback(
						el->el_ndn,
						el->el_cb,
						&el->el_nbase,
						el->el_scope,
						&el->el_filter );
				break;

			case LIMBO_BACKEND:
				rc = monitor_back_register_backend( el->el_bi );
				break;

			case LIMBO_DATABASE:
				rc = monitor_back_register_database( el->el_be, el->el_ndn );
				break;

			case LIMBO_OVERLAY_INFO:
				rc = monitor_back_register_overlay_info( el->el_on );
				break;

			case LIMBO_OVERLAY:
				rc = monitor_back_register_overlay( el->el_be, el->el_on, el->el_ndn );
				break;

			case LIMBO_SUBSYS:
				rc = monitor_back_register_subsys( el->el_mss );
				break;

			default:
				assert( 0 );
			}

			tmp = el;
			el = el->el_next;
			monitor_back_destroy_limbo_entry( tmp, rc );

			if ( rc != 0 ) {
				/* try all, but report error at end */
				retcode = 1;
			}
		}

		mi->mi_entry_limbo = NULL;
	}

	return retcode;
}

int
monitor_back_config(
	BackendInfo	*bi,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	/*
	 * eventually, will hold backend specific configuration parameters
	 */
	return SLAP_CONF_UNKNOWN;
}

#if 0
int
monitor_back_db_config(
	Backend     *be,
	const char  *fname,
	int         lineno,
	int         argc,
	char        **argv )
{
	monitor_info_t	*mi = ( monitor_info_t * )be->be_private;

	/*
	 * eventually, will hold database specific configuration parameters
	 */
	return SLAP_CONF_UNKNOWN;
}
#endif

int
monitor_back_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr)
{
	monitor_info_t	*mi = ( monitor_info_t * )be->be_private;

	if ( mi == NULL ) {
		return -1;
	}

	/*
	 * FIXME: destroys all the data
	 */
	/* NOTE: mi points to static storage; don't free it */
	
	(void)monitor_cache_destroy( mi );

	if ( monitor_subsys ) {
		int	i;

		for ( i = 0; monitor_subsys[ i ] != NULL; i++ ) {
			if ( monitor_subsys[ i ]->mss_destroy ) {
				monitor_subsys[ i ]->mss_destroy( be, monitor_subsys[ i ] );
			}

			if ( !BER_BVISNULL( &monitor_subsys[ i ]->mss_rdn ) ) {
				ch_free( monitor_subsys[ i ]->mss_rdn.bv_val );
			}
		}

		ch_free( monitor_subsys );
	}

	if ( mi->mi_entry_limbo ) {
		entry_limbo_t	*el = mi->mi_entry_limbo;

		for ( ; el; ) {
			entry_limbo_t *tmp = el;
			el = el->el_next;
			monitor_back_destroy_limbo_entry( tmp, 1 );
		}
	}
	
	ldap_pvt_thread_mutex_destroy( &monitor_info.mi_cache_mutex );

	be->be_private = NULL;

	return 0;
}

#if SLAPD_MONITOR == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( monitor )

#endif /* SLAPD_MONITOR == SLAPD_MOD_DYNAMIC */

