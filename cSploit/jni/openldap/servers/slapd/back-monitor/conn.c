/* conn.c - deal with connection subsystem */
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
monitor_subsys_conn_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e );

static int 
monitor_subsys_conn_create( 
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry 			*e_parent,
	Entry			**ep );

int
monitor_subsys_conn_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	monitor_info_t	*mi;
	Entry		*e, **ep, *e_conn;
	monitor_entry_t	*mp;
	char		buf[ BACKMONITOR_BUFSIZE ];
	struct berval	bv;

	assert( be != NULL );

	ms->mss_update = monitor_subsys_conn_update;
	ms->mss_create = monitor_subsys_conn_create;

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi, &ms->mss_ndn, &e_conn ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_conn->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;

	/*
	 * Max file descriptors
	 */
	BER_BVSTR( &bv, "cn=Max File Descriptors" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitorCounterObject, NULL, NULL );
	
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to create entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val, 0 );
		return( -1 );
	}

	if ( dtblsize ) {
		bv.bv_val = buf;
		bv.bv_len = snprintf( buf, sizeof( buf ), "%d", dtblsize );

	} else {
		BER_BVSTR( &bv, "0" );
	}
	attr_merge_one( e, mi->mi_ad_monitorCounter, &bv, NULL );
	
	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		return -1;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = ms->mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;
	mp->mp_flags &= ~MONITOR_F_VOLATILE_CH;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to add entry \"cn=Total,%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	*ep = e;
	ep = &mp->mp_next;
	
	/*
	 * Total conns
	 */
	BER_BVSTR( &bv, "cn=Total" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitorCounterObject, NULL, NULL );
	
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to create entry \"cn=Total,%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}
	
	BER_BVSTR( &bv, "-1" );
	attr_merge_one( e, mi->mi_ad_monitorCounter, &bv, NULL );
	
	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		return -1;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = ms->mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;
	mp->mp_flags &= ~MONITOR_F_VOLATILE_CH;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to add entry \"cn=Total,%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}

	*ep = e;
	ep = &mp->mp_next;
	
	/*
	 * Current conns
	 */
	BER_BVSTR( &bv, "cn=Current" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitorCounterObject, NULL, NULL );

	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to create entry \"cn=Current,%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}
	
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
	mp->mp_flags &= ~MONITOR_F_VOLATILE_CH;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_init: "
			"unable to add entry \"cn=Current,%s\"\n",
			ms->mss_ndn.bv_val, 0, 0 );
		return( -1 );
	}
	
	*ep = e;
	ep = &mp->mp_next;

	monitor_cache_release( mi, e_conn );

	return( 0 );
}

static int
monitor_subsys_conn_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;

	long 			n = -1;
	static struct berval	total_bv = BER_BVC( "cn=total" ),
				current_bv = BER_BVC( "cn=current" );
	struct berval		rdn;

	assert( mi != NULL );
	assert( e != NULL );

	dnRdn( &e->e_nname, &rdn );
	
	if ( dn_match( &rdn, &total_bv ) ) {
		n = connections_nextid();

	} else if ( dn_match( &rdn, &current_bv ) ) {
		Connection	*c;
		ber_socket_t	connindex;

		for ( n = 0, c = connection_first( &connindex );
				c != NULL;
				n++, c = connection_next( c, &connindex ) )
		{
			/* No Op */ ;
		}
		connection_done( c );
	}

	if ( n != -1 ) {
		Attribute	*a;
		char		buf[LDAP_PVT_INTTYPE_CHARS(long)];
		ber_len_t	len;

		a = attr_find( e->e_attrs, mi->mi_ad_monitorCounter );
		if ( a == NULL ) {
			return( -1 );
		}

		snprintf( buf, sizeof( buf ), "%ld", n );
		len = strlen( buf );
		if ( len > a->a_vals[ 0 ].bv_len ) {
			a->a_vals[ 0 ].bv_val = ber_memrealloc( a->a_vals[ 0 ].bv_val, len + 1 );
		}
		a->a_vals[ 0 ].bv_len = len;
		AC_MEMCPY( a->a_vals[ 0 ].bv_val, buf, len + 1 );

		/* FIXME: touch modifyTimestamp? */
	}

	return SLAP_CB_CONTINUE;
}

static int
conn_create(
	monitor_info_t		*mi,
	Connection		*c,
	Entry			**ep,
	monitor_subsys_t	*ms )
{
	monitor_entry_t *mp;
	struct tm	tm;
	char		buf[ BACKMONITOR_BUFSIZE ];
	char		buf2[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	char		buf3[ LDAP_LUTIL_GENTIME_BUFSIZE ];

	struct berval bv, ctmbv, mtmbv;
	struct berval bv_unknown= BER_BVC("unknown");

	Entry		*e;

	assert( c != NULL );
	assert( ep != NULL );

	ldap_pvt_gmtime( &c->c_starttime, &tm );

	ctmbv.bv_len = lutil_gentime( buf2, sizeof( buf2 ), &tm );
	ctmbv.bv_val = buf2;

	ldap_pvt_gmtime( &c->c_activitytime, &tm );
	mtmbv.bv_len = lutil_gentime( buf3, sizeof( buf3 ), &tm );
	mtmbv.bv_val = buf3;

	bv.bv_len = snprintf( buf, sizeof( buf ),
		"cn=Connection %ld", c->c_connid );
	bv.bv_val = buf;
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv, 
		mi->mi_oc_monitorConnection, &ctmbv, &mtmbv );

	if ( e == NULL) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_conn_create: "
			"unable to create entry "
			"\"cn=Connection %ld,%s\"\n",
			c->c_connid, 
			ms->mss_dn.bv_val, 0 );
		return( -1 );
	}

#ifdef MONITOR_LEGACY_CONN
	/* NOTE: this will disappear, as the exploded data
	 * has been moved to dedicated attributes */
	bv.bv_len = snprintf( buf, sizeof( buf ),
			"%ld "
			": %ld "
			": %ld/%ld/%ld/%ld "
			": %ld/%ld/%ld "
			": %s%s%s%s%s%s "
			": %s "
			": %s "
			": %s "
			": %s "
			": %s "
			": %s "
			": %s",
			c->c_connid,
			(long) c->c_protocol,
			c->c_n_ops_received, c->c_n_ops_executing,
				c->c_n_ops_pending, c->c_n_ops_completed,
			
			/* add low-level counters here */
			c->c_n_get, c->c_n_read, c->c_n_write,
			
			c->c_currentber ? "r" : "",
			c->c_writewaiter ? "w" : "",
			LDAP_STAILQ_EMPTY( &c->c_ops ) ? "" : "x",
			LDAP_STAILQ_EMPTY( &c->c_pending_ops ) ? "" : "p",
			connection_state2str( c->c_conn_state ),
			c->c_sasl_bind_in_progress ? "S" : "",
			
			c->c_dn.bv_len ? c->c_dn.bv_val : SLAPD_ANONYMOUS,
			
			c->c_listener_url.bv_val,
			BER_BVISNULL( &c->c_peer_domain )
				? "" : c->c_peer_domain.bv_val,
			BER_BVISNULL( &c->c_peer_name )
				? "" : c->c_peer_name.bv_val,
			c->c_sock_name.bv_val,
			
			buf2,
			buf3 );
	attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo, &bv, NULL );
#endif /* MONITOR_LEGACY_CONN */

	bv.bv_len = snprintf( buf, sizeof( buf ), "%lu", c->c_connid );
	attr_merge_one( e, mi->mi_ad_monitorConnectionNumber, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", (long) c->c_protocol );
	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionProtocol, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_ops_received );
	attr_merge_one( e, mi->mi_ad_monitorConnectionOpsReceived, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_ops_executing );
	attr_merge_one( e, mi->mi_ad_monitorConnectionOpsExecuting, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_ops_pending );
	attr_merge_one( e, mi->mi_ad_monitorConnectionOpsPending, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_ops_completed );
	attr_merge_one( e, mi->mi_ad_monitorConnectionOpsCompleted, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_get );
	attr_merge_one( e, mi->mi_ad_monitorConnectionGet, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_read );
	attr_merge_one( e, mi->mi_ad_monitorConnectionRead, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%ld", c->c_n_write );
	attr_merge_one( e, mi->mi_ad_monitorConnectionWrite, &bv, NULL );

	bv.bv_len = snprintf( buf, sizeof( buf ), "%s%s%s%s%s%s",
			c->c_currentber ? "r" : "",
			c->c_writewaiter ? "w" : "",
			LDAP_STAILQ_EMPTY( &c->c_ops ) ? "" : "x",
			LDAP_STAILQ_EMPTY( &c->c_pending_ops ) ? "" : "p",
			connection_state2str( c->c_conn_state ),
			c->c_sasl_bind_in_progress ? "S" : "" );
	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionMask, &bv, NULL );

	attr_merge_one( e, mi->mi_ad_monitorConnectionAuthzDN,
		&c->c_dn, &c->c_ndn );

	/* NOTE: client connections leave the c_peer_* fields NULL */
	assert( !BER_BVISNULL( &c->c_listener_url ) );
	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionListener,
		&c->c_listener_url, NULL );

	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionPeerDomain,
		BER_BVISNULL( &c->c_peer_domain ) ? &bv_unknown : &c->c_peer_domain,
		NULL );

	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionPeerAddress,
		BER_BVISNULL( &c->c_peer_name ) ? &bv_unknown : &c->c_peer_name,
		NULL );

	assert( !BER_BVISNULL( &c->c_sock_name ) );
	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionLocalAddress,
		&c->c_sock_name, NULL );

	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionStartTime, &ctmbv, NULL );

	attr_merge_normalize_one( e, mi->mi_ad_monitorConnectionActivityTime, &mtmbv, NULL );

	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		return LDAP_OTHER;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = MONITOR_F_SUB | MONITOR_F_VOLATILE;

	*ep = e;

	return SLAP_CB_CONTINUE;
}

static int 
monitor_subsys_conn_create( 
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry 			*e_parent,
	Entry			**ep )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;

	int			rc = SLAP_CB_CONTINUE;
	monitor_subsys_t	*ms;

	assert( mi != NULL );
	assert( e_parent != NULL );
	assert( ep != NULL );

	ms = (( monitor_entry_t *)e_parent->e_private)->mp_info;

	*ep = NULL;

	if ( ndn == NULL ) {
		Connection	*c;
		ber_socket_t	connindex;
		Entry		*e = NULL,
				*e_tmp = NULL;

		/* create all the children of e_parent */
		for ( c = connection_first( &connindex );
				c != NULL;
				c = connection_next( c, &connindex ) )
		{
			monitor_entry_t 	*mp;

			if ( conn_create( mi, c, &e, ms ) != SLAP_CB_CONTINUE
					|| e == NULL )
			{
				for ( ; e_tmp != NULL; ) {
					mp = ( monitor_entry_t * )e_tmp->e_private;
					e = mp->mp_next;

					ch_free( mp );
					e_tmp->e_private = NULL;
					entry_free( e_tmp );

					e_tmp = e;
				}
				rc = rs->sr_err = LDAP_OTHER;
				break;
			}
			mp = ( monitor_entry_t * )e->e_private;
			mp->mp_next = e_tmp;
			e_tmp = e;
		}
		connection_done( c );
		*ep = e;

	} else {
		Connection		*c;
		ber_socket_t		connindex;
		unsigned long 		connid;
		char			*next = NULL;
		static struct berval	nconn_bv = BER_BVC( "cn=connection " );

		rc = LDAP_NO_SUCH_OBJECT;
	       
		/* create exactly the required entry;
		 * the normalized DN must start with "cn=connection ",
		 * followed by the connection id, followed by
		 * the RDN separator "," */
		if ( ndn->bv_len <= nconn_bv.bv_len
				|| strncmp( ndn->bv_val, nconn_bv.bv_val, nconn_bv.bv_len ) != 0 )
		{
			return -1;
		}
		
		connid = strtol( &ndn->bv_val[ nconn_bv.bv_len ], &next, 10 );
		if ( next[ 0 ] != ',' ) {
			return ( rs->sr_err = LDAP_OTHER );
		}

		for ( c = connection_first( &connindex );
				c != NULL;
				c = connection_next( c, &connindex ) )
		{
			if ( c->c_connid == connid ) {
				rc = conn_create( mi, c, ep, ms );
				if ( rc != SLAP_CB_CONTINUE ) {
					rs->sr_err = rc;

				} else if ( *ep == NULL ) {
					rc = rs->sr_err = LDAP_OTHER;
				}

				break;
			}
		}
		
		connection_done( c );
	}

	return rc;
}

