/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>


#define AVL_INTERNAL
#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

/*
 * meta_back_conndn_cmp
 *
 * compares two struct metaconn based on the value of the conn pointer
 * and of the local DN; used by avl stuff
 */
int
meta_back_conndn_cmp(
	const void *c1,
	const void *c2 )
{
	metaconn_t	*mc1 = ( metaconn_t * )c1;
        metaconn_t	*mc2 = ( metaconn_t * )c2;
	int		rc;
	
	/* If local DNs don't match, it is definitely not a match */
	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	rc = SLAP_PTRCMP( mc1->mc_conn, mc2->mc_conn );
	if ( rc == 0 ) {
		rc = ber_bvcmp( &mc1->mc_local_ndn, &mc2->mc_local_ndn );
	}

	return rc;
}

/*
 * meta_back_conndnmc_cmp
 *
 * compares two struct metaconn based on the value of the conn pointer,
 * the local DN and the struct pointer; used by avl stuff
 */
static int
meta_back_conndnmc_cmp(
	const void *c1,
	const void *c2 )
{
	metaconn_t	*mc1 = ( metaconn_t * )c1;
        metaconn_t	*mc2 = ( metaconn_t * )c2;
	int		rc;
	
	/* If local DNs don't match, it is definitely not a match */
	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	rc = SLAP_PTRCMP( mc1->mc_conn, mc2->mc_conn );
	if ( rc == 0 ) {
		rc = ber_bvcmp( &mc1->mc_local_ndn, &mc2->mc_local_ndn );
		if ( rc == 0 ) {
			rc = SLAP_PTRCMP( mc1, mc2 );
		}
	}

	return rc;
}

/*
 * meta_back_conn_cmp
 *
 * compares two struct metaconn based on the value of the conn pointer;
 * used by avl stuff
 */
int
meta_back_conn_cmp(
	const void *c1,
	const void *c2 )
{
	metaconn_t	*mc1 = ( metaconn_t * )c1;
        metaconn_t	*mc2 = ( metaconn_t * )c2;
	
	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	return SLAP_PTRCMP( mc1->mc_conn, mc2->mc_conn );
}

/*
 * meta_back_conndn_dup
 *
 * returns -1 in case a duplicate struct metaconn has been inserted;
 * used by avl stuff
 */
int
meta_back_conndn_dup(
	void *c1,
	void *c2 )
{
	metaconn_t	*mc1 = ( metaconn_t * )c1;
	metaconn_t	*mc2 = ( metaconn_t * )c2;

	/* Cannot have more than one shared session with same DN */
	if ( mc1->mc_conn == mc2->mc_conn &&
		dn_match( &mc1->mc_local_ndn, &mc2->mc_local_ndn ) )
	{
		return -1;
	}
		
	return 0;
}

/*
 * Debug stuff (got it from libavl)
 */
#if META_BACK_PRINT_CONNTREE > 0
static void
meta_back_print( metaconn_t *mc, char *avlstr )
{
	int	i;

	fputs( "targets=[", stderr );
	for ( i = 0; i < mc->mc_info->mi_ntargets; i++ ) {
		fputc( mc->mc_conns[ i ].msc_ld ? '*' : 'o', stderr);
	}
	fputc( ']', stderr );

	fprintf( stderr, " mc=%p local=\"%s\" conn=%p refcnt=%d%s %s\n",
		(void *)mc,
		mc->mc_local_ndn.bv_val ? mc->mc_local_ndn.bv_val : "",
		(void *)mc->mc_conn,
		mc->mc_refcnt,
		LDAP_BACK_CONN_TAINTED( mc ) ? " tainted" : "",
		avlstr );
}

static void
meta_back_ravl_print( Avlnode *root, int depth )
{
	int     	i;

	if ( root == 0 ) {
		return;
	}
	
	meta_back_ravl_print( root->avl_right, depth + 1 );
	
	for ( i = 0; i < depth; i++ ) {
		fprintf( stderr, "-" );
	}
	fputc( ' ', stderr );

	meta_back_print( (metaconn_t *)root->avl_data,
		avl_bf2str( root->avl_bf ) );

	meta_back_ravl_print( root->avl_left, depth + 1 );
}

/* NOTE: duplicate from back-ldap/bind.c */
static char* priv2str[] = {
	"privileged",
	"privileged/TLS",
	"anonymous",
	"anonymous/TLS",
	"bind",
	"bind/TLS",
	NULL
};

void
meta_back_print_conntree( metainfo_t *mi, char *msg )
{
	int	c;

	fprintf( stderr, "========> %s\n", msg );
	
	for ( c = LDAP_BACK_PCONN_FIRST; c < LDAP_BACK_PCONN_LAST; c++ ) {
		int		i = 0;
		metaconn_t	*mc;

		fprintf( stderr, "  %s[%d]\n", priv2str[ c ], mi->mi_conn_priv[ c ].mic_num );

		LDAP_TAILQ_FOREACH( mc, &mi->mi_conn_priv[ c ].mic_priv, mc_q )
		{
			fprintf( stderr, "    [%d] ", i );
			meta_back_print( mc, "" );
			i++;
		}
	}
	
	if ( mi->mi_conninfo.lai_tree == NULL ) {
		fprintf( stderr, "\t(empty)\n" );

	} else {
		meta_back_ravl_print( mi->mi_conninfo.lai_tree, 0 );
	}
	
	fprintf( stderr, "<======== %s\n", msg );
}
#endif /* META_BACK_PRINT_CONNTREE */
/*
 * End of debug stuff
 */

/*
 * metaconn_alloc
 * 
 * Allocates a connection structure, making room for all the referenced targets
 */
static metaconn_t *
metaconn_alloc(
       	Operation 		*op )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metaconn_t	*mc;
	int		ntargets = mi->mi_ntargets;

	assert( ntargets > 0 );

	/* malloc all in one */
	mc = ( metaconn_t * )ch_calloc( 1, sizeof( metaconn_t )
		+ sizeof( metasingleconn_t ) * ( ntargets - 1 ) );
	if ( mc == NULL ) {
		return NULL;
	}

	mc->mc_info = mi;

	mc->mc_authz_target = META_BOUND_NONE;
	mc->mc_refcnt = 1;

	return mc;
}

/*
 * meta_back_init_one_conn
 * 
 * Initializes one connection
 */
int
meta_back_init_one_conn(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		*mc,
	int			candidate,
	int			ispriv,
	ldap_back_send_t	sendok,
	int			dolock )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	int			version;
	dncookie		dc;
	int			isauthz = ( candidate == mc->mc_authz_target );
	int			do_return = 0;
#ifdef HAVE_TLS
	int			is_ldaps = 0;
	int			do_start_tls = 0;
#endif /* HAVE_TLS */

	/* if the server is quarantined, and
	 * - the current interval did not expire yet, or
	 * - no more retries should occur,
	 * don't return the connection */
	if ( mt->mt_isquarantined ) {
		slap_retry_info_t	*ri = &mt->mt_quarantine;
		int			dont_retry = 0;

		if ( mt->mt_quarantine.ri_interval ) {
			ldap_pvt_thread_mutex_lock( &mt->mt_quarantine_mutex );
			dont_retry = ( mt->mt_isquarantined > LDAP_BACK_FQ_NO );
			if ( dont_retry ) {
				dont_retry = ( ri->ri_num[ ri->ri_idx ] == SLAP_RETRYNUM_TAIL
					|| slap_get_time() < ri->ri_last + ri->ri_interval[ ri->ri_idx ] );
				if ( !dont_retry ) {
					if ( LogTest( LDAP_DEBUG_ANY ) ) {
						char	buf[ SLAP_TEXT_BUFLEN ];

						snprintf( buf, sizeof( buf ),
							"meta_back_init_one_conn[%d]: quarantine "
							"retry block #%d try #%d",
							candidate, ri->ri_idx, ri->ri_count );
						Debug( LDAP_DEBUG_ANY, "%s %s.\n",
							op->o_log_prefix, buf, 0 );
					}

					mt->mt_isquarantined = LDAP_BACK_FQ_RETRYING;
				}

			}
			ldap_pvt_thread_mutex_unlock( &mt->mt_quarantine_mutex );
		}

		if ( dont_retry ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			if ( op->o_conn && ( sendok & LDAP_BACK_SENDERR ) ) {
				rs->sr_text = "Target is quarantined";
				send_ldap_result( op, rs );
			}
			return rs->sr_err;
		}
	}

retry_lock:;
	if ( dolock ) {
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
	}

	/*
	 * Already init'ed
	 */
	if ( LDAP_BACK_CONN_ISBOUND( msc )
		|| LDAP_BACK_CONN_ISANON( msc ) )
	{
		assert( msc->msc_ld != NULL );
		rs->sr_err = LDAP_SUCCESS;
		do_return = 1;

	} else if ( META_BACK_CONN_CREATING( msc )
		|| LDAP_BACK_CONN_BINDING( msc ) )
	{
		if ( !LDAP_BACK_USE_TEMPORARIES( mi ) ) {
			if ( dolock ) {
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			}

			ldap_pvt_thread_yield();
			goto retry_lock;
		}

		/* sounds more appropriate */
		rs->sr_err = LDAP_BUSY;
		rs->sr_text = "No connections to target are available";
		do_return = 1;

	} else if ( META_BACK_CONN_INITED( msc ) ) {
		assert( msc->msc_ld != NULL );
		rs->sr_err = LDAP_SUCCESS;
		do_return = 1;

	} else {
		/*
		 * creating...
		 */
		META_BACK_CONN_CREATING_SET( msc );
	}

	if ( dolock ) {
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
	}

	if ( do_return ) {
		if ( rs->sr_err != LDAP_SUCCESS
			&& op->o_conn
			&& ( sendok & LDAP_BACK_SENDERR ) )
		{
			send_ldap_result( op, rs );
		}

		return rs->sr_err;
	}

	assert( msc->msc_ld == NULL );
       
	/*
	 * Attempts to initialize the connection to the target ds
	 */
	ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );
	rs->sr_err = ldap_initialize( &msc->msc_ld, mt->mt_uri );
#ifdef HAVE_TLS
	is_ldaps = ldap_is_ldaps_url( mt->mt_uri );
#endif /* HAVE_TLS */
	ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto error_return;
	}

	/*
	 * Set LDAP version. This will always succeed: If the client
	 * bound with a particular version, then so can we.
	 */
	if ( mt->mt_version != 0 ) {
		version = mt->mt_version;

	} else if ( op->o_conn->c_protocol != 0 ) {
		version = op->o_conn->c_protocol;

	} else {
		version = LDAP_VERSION3;
	}
	ldap_set_option( msc->msc_ld, LDAP_OPT_PROTOCOL_VERSION, &version );
	ldap_set_urllist_proc( msc->msc_ld, mt->mt_urllist_f, mt->mt_urllist_p );

	/* automatically chase referrals ("chase-referrals [{yes|no}]" statement) */
	ldap_set_option( msc->msc_ld, LDAP_OPT_REFERRALS,
		META_BACK_TGT_CHASE_REFERRALS( mt ) ? LDAP_OPT_ON : LDAP_OPT_OFF );

	slap_client_keepalive(msc->msc_ld, &mt->mt_tls.sb_keepalive);

#ifdef HAVE_TLS
	if ( !is_ldaps ) {
		slap_bindconf *sb = NULL;

		if ( ispriv ) {
			sb = &mt->mt_idassert.si_bc;
		} else {
			sb = &mt->mt_tls;
		}

		if ( sb->sb_tls_do_init ) {
			bindconf_tls_set( sb, msc->msc_ld );
		} else if ( sb->sb_tls_ctx ) {
			ldap_set_option( msc->msc_ld, LDAP_OPT_X_TLS_CTX, sb->sb_tls_ctx );
		}

		if ( sb == &mt->mt_idassert.si_bc && sb->sb_tls_ctx ) {
			do_start_tls = 1;

		} else if ( META_BACK_TGT_USE_TLS( mt )
			|| ( op->o_conn->c_is_tls && META_BACK_TGT_PROPAGATE_TLS( mt ) ) )
		{
			do_start_tls = 1;
		}
	}

	/* start TLS ("tls [try-]{start|propagate}" statement) */
	if ( do_start_tls ) {
#ifdef SLAP_STARTTLS_ASYNCHRONOUS
		/*
		 * use asynchronous StartTLS; in case, chase referral
		 * FIXME: OpenLDAP does not return referral on StartTLS yet
		 */
		int		msgid;

		rs->sr_err = ldap_start_tls( msc->msc_ld, NULL, NULL, &msgid );
		if ( rs->sr_err == LDAP_SUCCESS ) {
			LDAPMessage	*res = NULL;
			int		rc, nretries = mt->mt_nretries;
			struct timeval	tv;

			LDAP_BACK_TV_SET( &tv );

retry:;
			rc = ldap_result( msc->msc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
			switch ( rc ) {
			case -1:
				rs->sr_err = LDAP_OTHER;
				break;

			case 0:
				if ( nretries != 0 ) {
					if ( nretries > 0 ) {
						nretries--;
					}
					LDAP_BACK_TV_SET( &tv );
					goto retry;
				}
				rs->sr_err = LDAP_OTHER;
				break;

			default:
				/* only touch when activity actually took place... */
				if ( mi->mi_idle_timeout != 0 && msc->msc_time < op->o_time ) {
					msc->msc_time = op->o_time;
				}
				break;
			}

			if ( rc == LDAP_RES_EXTENDED ) {
				struct berval	*data = NULL;

				/* NOTE: right now, data is unused, so don't get it */
				rs->sr_err = ldap_parse_extended_result( msc->msc_ld,
					res, NULL, NULL /* &data */ , 0 );
				if ( rs->sr_err == LDAP_SUCCESS ) {
					int		err;

					/* FIXME: matched? referrals? response controls? */
					rs->sr_err = ldap_parse_result( msc->msc_ld,
						res, &err, NULL, NULL, NULL, NULL, 1 );
					res = NULL;

					if ( rs->sr_err == LDAP_SUCCESS ) {
						rs->sr_err = err;
					}
					rs->sr_err = slap_map_api2result( rs );
					
					/* FIXME: in case a referral 
					 * is returned, should we try
					 * using it instead of the 
					 * configured URI? */
					if ( rs->sr_err == LDAP_SUCCESS ) {
						ldap_install_tls( msc->msc_ld );

					} else if ( rs->sr_err == LDAP_REFERRAL ) {
						/* FIXME: LDAP_OPERATIONS_ERROR? */
						rs->sr_err = LDAP_OTHER;
						rs->sr_text = "Unwilling to chase referral "
							"returned by Start TLS exop";
					}

					if ( data ) {
						ber_bvfree( data );
					}
				}

			} else {
				rs->sr_err = LDAP_OTHER;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		}
#else /* ! SLAP_STARTTLS_ASYNCHRONOUS */
		/*
		 * use synchronous StartTLS
		 */
		rs->sr_err = ldap_start_tls_s( msc->msc_ld, NULL, NULL );
#endif /* ! SLAP_STARTTLS_ASYNCHRONOUS */

		/* if StartTLS is requested, only attempt it if the URL
		 * is not "ldaps://"; this may occur not only in case
		 * of misconfiguration, but also when used in the chain 
		 * overlay, where the "uri" can be parsed out of a referral */
		if ( rs->sr_err == LDAP_SERVER_DOWN
			|| ( rs->sr_err != LDAP_SUCCESS
				&& META_BACK_TGT_TLS_CRITICAL( mt ) ) )
		{

#ifdef DEBUG_205
			Debug( LDAP_DEBUG_ANY,
				"### %s meta_back_init_one_conn(TLS) "
				"ldap_unbind_ext[%d] ld=%p\n",
				op->o_log_prefix, candidate,
				(void *)msc->msc_ld );
#endif /* DEBUG_205 */

			/* need to trash a failed Start TLS */
			meta_clear_one_candidate( op, mc, candidate );
			goto error_return;
		}
	}
#endif /* HAVE_TLS */

	/*
	 * Set the network timeout if set
	 */
	if ( mt->mt_network_timeout != 0 ) {
		struct timeval	network_timeout;

		network_timeout.tv_usec = 0;
		network_timeout.tv_sec = mt->mt_network_timeout;

		ldap_set_option( msc->msc_ld, LDAP_OPT_NETWORK_TIMEOUT,
				(void *)&network_timeout );
	}

	/*
	 * If the connection DN is not null, an attempt to rewrite it is made
	 */

	if ( ispriv ) {
		if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) ) {
			ber_bvreplace( &msc->msc_bound_ndn, &mt->mt_idassert_authcDN );
			if ( !BER_BVISNULL( &mt->mt_idassert_passwd ) ) {
				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
				}
				ber_bvreplace( &msc->msc_cred, &mt->mt_idassert_passwd );
			}
			LDAP_BACK_CONN_ISIDASSERT_SET( msc );

		} else {
			ber_bvreplace( &msc->msc_bound_ndn, &slap_empty_bv );
		}

	} else {
		if ( !BER_BVISNULL( &msc->msc_cred ) ) {
			memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
			ber_memfree_x( msc->msc_cred.bv_val, NULL );
			BER_BVZERO( &msc->msc_cred );
		}
		if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
			ber_memfree_x( msc->msc_bound_ndn.bv_val, NULL );
			BER_BVZERO( &msc->msc_bound_ndn );
		}
		if ( !BER_BVISEMPTY( &op->o_ndn )
			&& SLAP_IS_AUTHZ_BACKEND( op )
			&& isauthz )
		{
			dc.target = mt;
			dc.conn = op->o_conn;
			dc.rs = rs;
			dc.ctx = "bindDN";
		
			/*
			 * Rewrite the bind dn if needed
			 */
			if ( ldap_back_dn_massage( &dc, &op->o_conn->c_dn,
						&msc->msc_bound_ndn ) )
			{

#ifdef DEBUG_205
				Debug( LDAP_DEBUG_ANY,
					"### %s meta_back_init_one_conn(rewrite) "
					"ldap_unbind_ext[%d] ld=%p\n",
					op->o_log_prefix, candidate,
					(void *)msc->msc_ld );
#endif /* DEBUG_205 */

				/* need to trash a connection not fully established */
				meta_clear_one_candidate( op, mc, candidate );
				goto error_return;
			}
			
			/* copy the DN if needed */
			if ( msc->msc_bound_ndn.bv_val == op->o_conn->c_dn.bv_val ) {
				ber_dupbv( &msc->msc_bound_ndn, &op->o_conn->c_dn );
			}

			assert( !BER_BVISNULL( &msc->msc_bound_ndn ) );

		} else {
			ber_dupbv( &msc->msc_bound_ndn, (struct berval *)&slap_empty_bv );
		}
	}

	assert( !BER_BVISNULL( &msc->msc_bound_ndn ) );

error_return:;
	if ( dolock ) {
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
	}
	META_BACK_CONN_CREATING_CLEAR( msc );
	if ( rs->sr_err == LDAP_SUCCESS ) {
		/*
		 * Sets a cookie for the rewrite session
		 */
		( void )rewrite_session_init( mt->mt_rwmap.rwm_rw, op->o_conn );
		META_BACK_CONN_INITED_SET( msc );
	}
	if ( dolock ) {
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		rs->sr_err = slap_map_api2result( rs );
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
	}

	return rs->sr_err;
}

/*
 * meta_back_retry
 * 
 * Retries one connection
 */
int
meta_back_retry(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		**mcp,
	int			candidate,
	ldap_back_send_t	sendok )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metaconn_t		*mc = *mcp;
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	int			rc = LDAP_UNAVAILABLE,
				binding,
				quarantine = 1;

	ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );

	assert( !META_BACK_CONN_CREATING( msc ) );
	binding = LDAP_BACK_CONN_BINDING( msc );
	LDAP_BACK_CONN_BINDING_CLEAR( msc );

	assert( mc->mc_refcnt > 0 );
	if ( mc->mc_refcnt == 1 ) {
		struct berval save_cred;

		if ( LogTest( LDAP_DEBUG_ANY ) ) {
			char	buf[ SLAP_TEXT_BUFLEN ];

			/* this lock is required; however,
			 * it's invoked only when logging is on */
			ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );
			snprintf( buf, sizeof( buf ),
				"retrying URI=\"%s\" DN=\"%s\"",
				mt->mt_uri,
				BER_BVISNULL( &msc->msc_bound_ndn ) ?
					"" : msc->msc_bound_ndn.bv_val );
			ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );

			Debug( LDAP_DEBUG_ANY,
				"%s meta_back_retry[%d]: %s.\n",
				op->o_log_prefix, candidate, buf );
		}

		/* save credentials, if any, for later use;
		 * meta_clear_one_candidate() would free them */
		save_cred = msc->msc_cred;
		BER_BVZERO( &msc->msc_cred );

		meta_clear_one_candidate( op, mc, candidate );
		LDAP_BACK_CONN_ISBOUND_CLEAR( msc );

		( void )rewrite_session_delete( mt->mt_rwmap.rwm_rw, op->o_conn );

		/* mc here must be the regular mc, reset and ready for init */
		rc = meta_back_init_one_conn( op, rs, mc, candidate,
			LDAP_BACK_CONN_ISPRIV( mc ), sendok, 0 );

		/* restore credentials, if any and if needed;
		 * meta_back_init_one_conn() restores msc_bound_ndn, if any;
		 * if no msc_bound_ndn is restored, destroy credentials */
		if ( !BER_BVISNULL( &msc->msc_bound_ndn )
			&& BER_BVISNULL( &msc->msc_cred ) )
		{
			msc->msc_cred = save_cred;

		} else if ( !BER_BVISNULL( &save_cred ) ) {
			memset( save_cred.bv_val, 0, save_cred.bv_len );
			ber_memfree_x( save_cred.bv_val, NULL );
		}

		/* restore the "binding" flag, in case */
		if ( binding ) {
			LDAP_BACK_CONN_BINDING_SET( msc );
		}

		if ( rc == LDAP_SUCCESS ) {
			quarantine = 0;
			LDAP_BACK_CONN_BINDING_SET( msc ); binding = 1;
			rc = meta_back_single_dobind( op, rs, mcp, candidate,
				sendok, mt->mt_nretries, 0 );

			Debug( LDAP_DEBUG_ANY,
				"%s meta_back_retry[%d]: "
				"meta_back_single_dobind=%d\n",
				op->o_log_prefix, candidate, rc );
			if ( rc == LDAP_SUCCESS ) {
				if ( !BER_BVISNULL( &msc->msc_bound_ndn ) &&
					!BER_BVISEMPTY( &msc->msc_bound_ndn ) )
				{
					LDAP_BACK_CONN_ISBOUND_SET( msc );

				} else {
					LDAP_BACK_CONN_ISANON_SET( msc );
				}

				/* when bound, dispose of the "binding" flag */
				if ( binding ) {
					LDAP_BACK_CONN_BINDING_CLEAR( msc );
				}
			}
		}

#if 0	/* ITS#7591, following stmt drops needed result msgs */
		/* don't send twice */
		sendok &= ~LDAP_BACK_SENDERR;
#endif
	}

	if ( rc != LDAP_SUCCESS ) {
		SlapReply		*candidates = meta_back_candidates_get( op );

		candidates[ candidate ].sr_err = rc;

		if ( *mcp != NULL ) {
			if ( mc->mc_refcnt == 1 ) {
				if ( binding ) {
					LDAP_BACK_CONN_BINDING_CLEAR( msc );
				}
				(void)meta_clear_one_candidate( op, mc, candidate );
			}

			LDAP_BACK_CONN_TAINTED_SET( mc );
			/* only release if mandatory; otherwise
			 * let the caller do what's best before
			 * releasing */
			if ( META_BACK_ONERR_STOP( mi ) ) {
				meta_back_release_conn_lock( mi, mc, 0 );
				*mcp = NULL;

			} else {
#if META_BACK_PRINT_CONNTREE > 0
				meta_back_print_conntree( mi, ">>> meta_back_retry" );
#endif /* META_BACK_PRINT_CONNTREE */

				/* FIXME: could be done better, reworking meta_back_release_conn_lock() */
				if ( LDAP_BACK_PCONN_ISPRIV( mc ) ) {
					if ( mc->mc_q.tqe_prev != NULL ) {
						assert( LDAP_BACK_CONN_CACHED( mc ) );
						assert( mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num > 0 );
						LDAP_TAILQ_REMOVE( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv,
							mc, mc_q );
						mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num--;
						LDAP_TAILQ_ENTRY_INIT( mc, mc_q );

					} else {
						assert( !LDAP_BACK_CONN_CACHED( mc ) );
					}

				} else {
					/* FIXME: check if in tree, for consistency? */
					(void)avl_delete( &mi->mi_conninfo.lai_tree,
						( caddr_t )mc, meta_back_conndnmc_cmp );
				}
				LDAP_BACK_CONN_CACHED_CLEAR( mc );

#if META_BACK_PRINT_CONNTREE > 0
				meta_back_print_conntree( mi, "<<< meta_back_retry" );
#endif /* META_BACK_PRINT_CONNTREE */
			}
		}

		if ( sendok & LDAP_BACK_SENDERR ) {
			rs->sr_err = rc;
			rs->sr_text = "Unable to retry";
			send_ldap_result( op, rs );
		}
	}

	if ( quarantine && META_BACK_TGT_QUARANTINE( mt ) ) {
		meta_back_quarantine( op, rs, candidate );
	}

	ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

	return rc == LDAP_SUCCESS ? 1 : 0;
}

/*
 * callback for unique candidate selection
 */
static int
meta_back_conn_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		((long *)op->o_callback->sc_private)[0] = (long)op->o_private;
		break;

	case REP_SEARCHREF:
	case REP_RESULT:
		break;

	default:
		return rs->sr_err;
	}

	return 0;
}


static int
meta_back_get_candidate(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*ndn )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	long		candidate;

	/*
	 * tries to get a unique candidate
	 * (takes care of default target)
	 */
	candidate = meta_back_select_unique_candidate( mi, ndn );

	/*
	 * if any is found, inits the connection
	 */
	if ( candidate == META_TARGET_NONE ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "No suitable candidate target found";

	} else if ( candidate == META_TARGET_MULTIPLE ) {
		Operation	op2 = *op;
		SlapReply	rs2 = { REP_RESULT };
		slap_callback	cb2 = { 0 };
		int		rc;

		/* try to get a unique match for the request ndn
		 * among the multiple candidates available */
		op2.o_tag = LDAP_REQ_SEARCH;
		op2.o_req_dn = *ndn;
		op2.o_req_ndn = *ndn;
		op2.ors_scope = LDAP_SCOPE_BASE;
		op2.ors_deref = LDAP_DEREF_NEVER;
		op2.ors_attrs = slap_anlist_no_attrs;
		op2.ors_attrsonly = 0;
		op2.ors_limit = NULL;
		op2.ors_slimit = 1;
		op2.ors_tlimit = SLAP_NO_LIMIT;

		op2.ors_filter = (Filter *)slap_filter_objectClass_pres;
		op2.ors_filterstr = *slap_filterstr_objectClass_pres;

		op2.o_callback = &cb2;
		cb2.sc_response = meta_back_conn_cb;
		cb2.sc_private = (void *)&candidate;

		rc = op->o_bd->be_search( &op2, &rs2 );

		switch ( rs2.sr_err ) {
		case LDAP_SUCCESS:
		default:
			rs->sr_err = rs2.sr_err;
			break;

		case LDAP_SIZELIMIT_EXCEEDED:
			/* if multiple candidates can serve the operation,
			 * and a default target is defined, and it is
			 * a candidate, try using it (FIXME: YMMV) */
			if ( mi->mi_defaulttarget != META_DEFAULT_TARGET_NONE
				&& meta_back_is_candidate( mi->mi_targets[ mi->mi_defaulttarget ],
						ndn, op->o_tag == LDAP_REQ_SEARCH ? op->ors_scope : LDAP_SCOPE_BASE ) )
			{
				candidate = mi->mi_defaulttarget;
				rs->sr_err = LDAP_SUCCESS;
				rs->sr_text = NULL;

			} else {
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "Unable to select unique candidate target";
			}
			break;
		}

	} else {
		rs->sr_err = LDAP_SUCCESS;
	}

	return candidate;
}

static void	*meta_back_candidates_dummy;

static void
meta_back_candidates_keyfree(
	void		*key,
	void		*data )
{
	metacandidates_t	*mc = (metacandidates_t *)data;

	ber_memfree_x( mc->mc_candidates, NULL );
	ber_memfree_x( data, NULL );
}

SlapReply *
meta_back_candidates_get( Operation *op )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metacandidates_t	*mc;

	if ( op->o_threadctx ) {
		void		*data = NULL;

		ldap_pvt_thread_pool_getkey( op->o_threadctx,
				&meta_back_candidates_dummy, &data, NULL );
		mc = (metacandidates_t *)data;

	} else {
		mc = mi->mi_candidates;
	}

	if ( mc == NULL ) {
		mc = ch_calloc( sizeof( metacandidates_t ), 1 );
		mc->mc_ntargets = mi->mi_ntargets;
		mc->mc_candidates = ch_calloc( sizeof( SlapReply ), mc->mc_ntargets );
		if ( op->o_threadctx ) {
			void		*data = NULL;

			data = (void *)mc;
			ldap_pvt_thread_pool_setkey( op->o_threadctx,
					&meta_back_candidates_dummy, data,
					meta_back_candidates_keyfree,
					NULL, NULL );

		} else {
			mi->mi_candidates = mc;
		}

	} else if ( mc->mc_ntargets < mi->mi_ntargets ) {
		/* NOTE: in the future, may want to allow back-config
		 * to add/remove targets from back-meta... */
		mc->mc_candidates = ch_realloc( mc->mc_candidates,
				sizeof( SlapReply ) * mi->mi_ntargets );
		memset( &mc->mc_candidates[ mc->mc_ntargets ], 0,
			sizeof( SlapReply ) * ( mi->mi_ntargets - mc->mc_ntargets ) );
		mc->mc_ntargets = mi->mi_ntargets;
	}

	return mc->mc_candidates;
}

/*
 * meta_back_getconn
 * 
 * Prepares the connection structure
 * 
 * RATIONALE:
 *
 * - determine what DN is being requested:
 *
 *	op	requires candidate	checks
 *
 *	add	unique			parent of o_req_ndn
 *	bind	unique^*[/all]		o_req_ndn [no check]
 *	compare	unique^+		o_req_ndn
 *	delete	unique			o_req_ndn
 *	modify	unique			o_req_ndn
 *	search	any			o_req_ndn
 *	modrdn	unique[, unique]	o_req_ndn[, orr_nnewSup]
 *
 * - for ops that require the candidate to be unique, in case of multiple
 *   occurrences an internal search with sizeLimit=1 is performed
 *   if a unique candidate can actually be determined.  If none is found,
 *   the operation aborts; if multiple are found, the default target
 *   is used if defined and candidate; otherwise the operation aborts.
 *
 * *^note: actually, the bind operation is handled much like a search;
 *   i.e. the bind is broadcast to all candidate targets.
 *
 * +^note: actually, the compare operation is handled much like a search;
 *   i.e. the compare is broadcast to all candidate targets, while checking
 *   that exactly none (noSuchObject) or one (TRUE/FALSE/UNDEFINED) is
 *   returned.
 */
metaconn_t *
meta_back_getconn(
       	Operation 		*op,
	SlapReply		*rs,
	int 			*candidate,
	ldap_back_send_t	sendok )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metaconn_t	*mc = NULL,
			mc_curr = {{ 0 }};
	int		cached = META_TARGET_NONE,
			i = META_TARGET_NONE,
			err = LDAP_SUCCESS,
			new_conn = 0,
			ncandidates = 0;


	meta_op_type	op_type = META_OP_REQUIRE_SINGLE;
	enum		{
		META_DNTYPE_ENTRY,
		META_DNTYPE_PARENT,
		META_DNTYPE_NEWPARENT
	}		dn_type = META_DNTYPE_ENTRY;
	struct berval	ndn = op->o_req_ndn,
			pndn;

	SlapReply	*candidates = meta_back_candidates_get( op );

	/* Internal searches are privileged and shared. So is root. */
	if ( ( !BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_ALWAYS( mi ) )
		|| ( BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_ANON( mi ) )
		|| op->o_do_not_cache || be_isroot( op ) )
	{
		LDAP_BACK_CONN_ISPRIV_SET( &mc_curr );
		mc_curr.mc_local_ndn = op->o_bd->be_rootndn;
		LDAP_BACK_PCONN_ROOTDN_SET( &mc_curr, op );

	} else if ( BER_BVISEMPTY( &op->o_ndn ) && META_BACK_PROXYAUTHZ_NOANON( mi ) )
	{
		LDAP_BACK_CONN_ISANON_SET( &mc_curr );
		BER_BVSTR( &mc_curr.mc_local_ndn, "" );
		LDAP_BACK_PCONN_ANON_SET( &mc_curr, op );

	} else {
		mc_curr.mc_local_ndn = op->o_ndn;

		/* Explicit binds must not be shared */
		if ( !BER_BVISEMPTY( &op->o_ndn )
			|| op->o_tag == LDAP_REQ_BIND
			|| SLAP_IS_AUTHZ_BACKEND( op ) )
		{
			mc_curr.mc_conn = op->o_conn;
	
		} else {
			LDAP_BACK_CONN_ISANON_SET( &mc_curr );
			LDAP_BACK_PCONN_ANON_SET( &mc_curr, op );
		}
	}

	/* Explicit Bind requests always get their own conn */
	if ( sendok & LDAP_BACK_BINDING ) {
		mc_curr.mc_conn = op->o_conn;

	} else {
		/* Searches for a metaconn in the avl tree */
retry_lock:;
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		if ( LDAP_BACK_PCONN_ISPRIV( &mc_curr ) ) {
			/* lookup a conn that's not binding */
			LDAP_TAILQ_FOREACH( mc,
				&mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( &mc_curr ) ].mic_priv,
				mc_q )
			{
				if ( !LDAP_BACK_CONN_BINDING( mc ) && mc->mc_refcnt == 0 ) {
					break;
				}
			}

			if ( mc != NULL ) {
				/* move to tail of queue */
				if ( mc != LDAP_TAILQ_LAST( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv,
					metaconn_t, mc_q ) )
				{
					LDAP_TAILQ_REMOVE( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv,
						mc, mc_q );
					LDAP_TAILQ_ENTRY_INIT( mc, mc_q );
					LDAP_TAILQ_INSERT_TAIL( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv,
						mc, mc_q );
				}

			} else if ( !LDAP_BACK_USE_TEMPORARIES( mi )
				&& mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( &mc_curr ) ].mic_num == mi->mi_conn_priv_max )
			{
				mc = LDAP_TAILQ_FIRST( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( &mc_curr ) ].mic_priv );
			}
			

		} else {
			mc = (metaconn_t *)avl_find( mi->mi_conninfo.lai_tree, 
				(caddr_t)&mc_curr, meta_back_conndn_cmp );
		}

		if ( mc ) {
			/* catch taint errors */
			assert( !LDAP_BACK_CONN_TAINTED( mc ) );

			/* Don't reuse connections while they're still binding
			 * NOTE: only makes sense for binds */
			if ( LDAP_BACK_CONN_BINDING( mc ) ) {
				if ( !LDAP_BACK_USE_TEMPORARIES( mi ) ) {
					ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

					ldap_pvt_thread_yield();
					goto retry_lock;
				}

				/* release conn, and create a temporary */
				mc = NULL;

			} else {
				if ( mc->mc_refcnt == 0 && (( mi->mi_conn_ttl != 0 && op->o_time > mc->mc_create_time + mi->mi_conn_ttl )
					|| ( mi->mi_idle_timeout != 0 && op->o_time > mc->mc_time + mi->mi_idle_timeout )) )
				{
#if META_BACK_PRINT_CONNTREE > 0
					meta_back_print_conntree( mi,
						">>> meta_back_getconn(expired)" );
#endif /* META_BACK_PRINT_CONNTREE */

					/* don't let anyone else use this expired connection */
					if ( LDAP_BACK_PCONN_ISPRIV( mc ) ) {
						if ( mc->mc_q.tqe_prev != NULL ) {
							assert( LDAP_BACK_CONN_CACHED( mc ) );
							assert( mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num > 0 );
							LDAP_TAILQ_REMOVE( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv,
								mc, mc_q );
							mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num--;
							LDAP_TAILQ_ENTRY_INIT( mc, mc_q );

						} else {
							assert( !LDAP_BACK_CONN_CACHED( mc ) );
						}

					} else {
						(void)avl_delete( &mi->mi_conninfo.lai_tree,
							(caddr_t)mc, meta_back_conndnmc_cmp );
					}

#if META_BACK_PRINT_CONNTREE > 0
					meta_back_print_conntree( mi,
						"<<< meta_back_getconn(expired)" );
#endif /* META_BACK_PRINT_CONNTREE */
					LDAP_BACK_CONN_TAINTED_SET( mc );
					LDAP_BACK_CONN_CACHED_CLEAR( mc );

					if ( LogTest( LDAP_DEBUG_TRACE ) ) {
						char buf[STRLENOF("4294967295U") + 1] = { 0 };
						mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

						Debug( LDAP_DEBUG_TRACE,
							"%s meta_back_getconn: mc=%p conn=%s expired (tainted).\n",
							op->o_log_prefix, (void *)mc, buf );
					}
				}

				mc->mc_refcnt++;
			}
		}
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
	}

	switch ( op->o_tag ) {
	case LDAP_REQ_ADD:
		/* if we go to selection, the entry must not exist,
		 * and we must be able to resolve the parent */
		dn_type = META_DNTYPE_PARENT;
		dnParent( &ndn, &pndn );
		break;

	case LDAP_REQ_MODRDN:
		/* if nnewSuperior is not NULL, it must resolve
		 * to the same candidate as the req_ndn */
		if ( op->orr_nnewSup ) {
			dn_type = META_DNTYPE_NEWPARENT;
		}
		break;

	case LDAP_REQ_BIND:
		/* if bound as rootdn, the backend must bind to all targets
		 * with the administrative identity
		 * (unless pseoudoroot-bind-defer is TRUE) */
		if ( op->orb_method == LDAP_AUTH_SIMPLE && be_isroot_pw( op ) ) {
			op_type = META_OP_REQUIRE_ALL;
		}
		break;

	case LDAP_REQ_COMPARE:
	case LDAP_REQ_DELETE:
	case LDAP_REQ_MODIFY:
		/* just a unique candidate */
		break;

	case LDAP_REQ_SEARCH:
		/* allow multiple candidates for the searchBase */
		op_type = META_OP_ALLOW_MULTIPLE;
		break;

	default:
		/* right now, just break (exop?) */
		break;
	}

	/*
	 * require all connections ...
	 */
	if ( op_type == META_OP_REQUIRE_ALL ) {

		/* Looks like we didn't get a bind. Open a new session... */
		if ( mc == NULL ) {
			assert( new_conn == 0 );
			mc = metaconn_alloc( op );
			mc->mc_conn = mc_curr.mc_conn;
			ber_dupbv( &mc->mc_local_ndn, &mc_curr.mc_local_ndn );
			new_conn = 1;
			if ( sendok & LDAP_BACK_BINDING ) {
				LDAP_BACK_CONN_BINDING_SET( mc );
			}
			if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
				LDAP_BACK_CONN_ISPRIV_SET( mc );

			} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
				LDAP_BACK_CONN_ISANON_SET( mc );
			}

		} else if ( 0 ) {
			/* TODO: if any of the connections is binding,
			 * release mc and create a new one */
		}

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			/*
			 * The target is activated; if needed, it is
			 * also init'd
			 */
			candidates[ i ].sr_err = meta_back_init_one_conn( op,
				rs, mc, i, LDAP_BACK_CONN_ISPRIV( &mc_curr ),
				LDAP_BACK_DONTSEND, !new_conn );
			if ( candidates[ i ].sr_err == LDAP_SUCCESS ) {
				if ( new_conn && ( sendok & LDAP_BACK_BINDING ) ) {
					LDAP_BACK_CONN_BINDING_SET( &mc->mc_conns[ i ] );
				}
				META_CANDIDATE_SET( &candidates[ i ] );
				ncandidates++;
	
			} else {
				
				/*
				 * FIXME: in case one target cannot
				 * be init'd, should the other ones
				 * be tried?
				 */
				META_CANDIDATE_RESET( &candidates[ i ] );
				err = candidates[ i ].sr_err;
				continue;
			}
		}

		if ( ncandidates == 0 ) {
			if ( new_conn ) {
				mc->mc_refcnt = 0;
				meta_back_conn_free( mc );

			} else {
				meta_back_release_conn( mi, mc );
			}

			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			rs->sr_text = "Unable to select valid candidates";

			if ( sendok & LDAP_BACK_SENDERR ) {
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					rs->sr_matched = op->o_bd->be_suffix[ 0 ].bv_val;
				}
				send_ldap_result( op, rs );
				rs->sr_matched = NULL;
			}

			return NULL;
		}

		goto done;
	}
	
	/*
	 * looks in cache, if any
	 */
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED ) {
		cached = i = meta_dncache_get_target( &mi->mi_cache, &op->o_req_ndn );
	}

	if ( op_type == META_OP_REQUIRE_SINGLE ) {
		metatarget_t		*mt = NULL;
		metasingleconn_t	*msc = NULL;

		int			j;

		for ( j = 0; j < mi->mi_ntargets; j++ ) {
			META_CANDIDATE_RESET( &candidates[ j ] );
		}

		/*
		 * tries to get a unique candidate
		 * (takes care of default target)
		 */
		if ( i == META_TARGET_NONE ) {
			i = meta_back_get_candidate( op, rs, &ndn );

			if ( rs->sr_err == LDAP_NO_SUCH_OBJECT && dn_type == META_DNTYPE_PARENT ) {
				i = meta_back_get_candidate( op, rs, &pndn );
			}
	
			if ( i < 0 || rs->sr_err != LDAP_SUCCESS ) {
				if ( mc != NULL ) {
					meta_back_release_conn( mi, mc );
				}

				if ( sendok & LDAP_BACK_SENDERR ) {
					if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
						rs->sr_matched = op->o_bd->be_suffix[ 0 ].bv_val;
					}
					send_ldap_result( op, rs );
					rs->sr_matched = NULL;
				}
			
				return NULL;
			}
		}

		if ( dn_type == META_DNTYPE_NEWPARENT && meta_back_get_candidate( op, rs, op->orr_nnewSup ) != i )
		{
			if ( mc != NULL ) {
				meta_back_release_conn( mi, mc );
			}

			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "Cross-target rename not supported";
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}

			return NULL;
		}

		Debug( LDAP_DEBUG_TRACE,
	"==>meta_back_getconn: got target=%d for ndn=\"%s\" from cache\n",
				i, op->o_req_ndn.bv_val, 0 );

		if ( mc == NULL ) {
			/* Retries searching for a metaconn in the avl tree
			 * the reason is that the connection might have been
			 * created by meta_back_get_candidate() */
			if ( !( sendok & LDAP_BACK_BINDING ) ) {
retry_lock2:;
				ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
				mc = (metaconn_t *)avl_find( mi->mi_conninfo.lai_tree, 
					(caddr_t)&mc_curr, meta_back_conndn_cmp );
				if ( mc != NULL ) {
					/* catch taint errors */
					assert( !LDAP_BACK_CONN_TAINTED( mc ) );

					/* Don't reuse connections while they're still binding */
					if ( META_BACK_CONN_CREATING( &mc->mc_conns[ i ] )
						|| LDAP_BACK_CONN_BINDING( &mc->mc_conns[ i ] ) )
					{
						if ( !LDAP_BACK_USE_TEMPORARIES( mi ) ) {
							ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
							ldap_pvt_thread_yield();
							goto retry_lock2;
						}

						mc = NULL;

					} else {
						mc->mc_refcnt++;
					}
				}
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			}

			/* Looks like we didn't get a bind. Open a new session... */
			if ( mc == NULL ) {
				assert( new_conn == 0 );
				mc = metaconn_alloc( op );
				mc->mc_conn = mc_curr.mc_conn;
				ber_dupbv( &mc->mc_local_ndn, &mc_curr.mc_local_ndn );
				new_conn = 1;
				if ( sendok & LDAP_BACK_BINDING ) {
					LDAP_BACK_CONN_BINDING_SET( mc );
				}
				if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
					LDAP_BACK_CONN_ISPRIV_SET( mc );

				} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
					LDAP_BACK_CONN_ISANON_SET( mc );
				}
			}
		}

		/*
		 * Clear all other candidates
		 */
		( void )meta_clear_unused_candidates( op, i );

		mt = mi->mi_targets[ i ];
		msc = &mc->mc_conns[ i ];

		/*
		 * The target is activated; if needed, it is
		 * also init'd. In case of error, meta_back_init_one_conn
		 * sends the appropriate result.
		 */
		err = meta_back_init_one_conn( op, rs, mc, i,
			LDAP_BACK_CONN_ISPRIV( &mc_curr ), sendok, !new_conn );
		if ( err != LDAP_SUCCESS ) {
			/*
			 * FIXME: in case one target cannot
			 * be init'd, should the other ones
			 * be tried?
			 */
			META_CANDIDATE_RESET( &candidates[ i ] );
 			if ( new_conn ) {
				mc->mc_refcnt = 0;
				meta_back_conn_free( mc );

			} else {
				meta_back_release_conn( mi, mc );
			}
			return NULL;
		}

		if ( new_conn && ( sendok & LDAP_BACK_BINDING ) ) {
			LDAP_BACK_CONN_BINDING_SET( &mc->mc_conns[ i ] );
		}

		candidates[ i ].sr_err = LDAP_SUCCESS;
		META_CANDIDATE_SET( &candidates[ i ] );
		ncandidates++;

		if ( candidate ) {
			*candidate = i;
		}

	/*
	 * if no unique candidate ...
	 */
	} else {

		/* Looks like we didn't get a bind. Open a new session... */
		if ( mc == NULL ) {
			assert( new_conn == 0 );
			mc = metaconn_alloc( op );
			mc->mc_conn = mc_curr.mc_conn;
			ber_dupbv( &mc->mc_local_ndn, &mc_curr.mc_local_ndn );
			new_conn = 1;
			if ( LDAP_BACK_CONN_ISPRIV( &mc_curr ) ) {
				LDAP_BACK_CONN_ISPRIV_SET( mc );

			} else if ( LDAP_BACK_CONN_ISANON( &mc_curr ) ) {
				LDAP_BACK_CONN_ISANON_SET( mc );
			}
		}

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			metatarget_t		*mt = mi->mi_targets[ i ];

			META_CANDIDATE_RESET( &candidates[ i ] );

			if ( i == cached 
				|| meta_back_is_candidate( mt, &op->o_req_ndn,
					op->o_tag == LDAP_REQ_SEARCH ? op->ors_scope : LDAP_SCOPE_SUBTREE ) )
			{

				/*
				 * The target is activated; if needed, it is
				 * also init'd
				 */
				int lerr = meta_back_init_one_conn( op, rs, mc, i,
					LDAP_BACK_CONN_ISPRIV( &mc_curr ),
					LDAP_BACK_DONTSEND, !new_conn );
				candidates[ i ].sr_err = lerr;
				if ( lerr == LDAP_SUCCESS ) {
					META_CANDIDATE_SET( &candidates[ i ] );
					ncandidates++;

					Debug( LDAP_DEBUG_TRACE, "%s: meta_back_getconn[%d]\n",
						op->o_log_prefix, i, 0 );

				} else if ( lerr == LDAP_UNAVAILABLE && !META_BACK_ONERR_STOP( mi ) ) {
					META_CANDIDATE_SET( &candidates[ i ] );

					Debug( LDAP_DEBUG_TRACE, "%s: meta_back_getconn[%d] %s\n",
						op->o_log_prefix, i,
						mt->mt_isquarantined != LDAP_BACK_FQ_NO ? "quarantined" : "unavailable" );

				} else {
				
					/*
					 * FIXME: in case one target cannot
					 * be init'd, should the other ones
					 * be tried?
					 */
					if ( new_conn ) {
						( void )meta_clear_one_candidate( op, mc, i );
					}
					/* leave the target candidate, but record the error for later use */
					err = lerr;

					if ( lerr == LDAP_UNAVAILABLE && mt->mt_isquarantined != LDAP_BACK_FQ_NO ) {
						Debug( LDAP_DEBUG_TRACE, "%s: meta_back_getconn[%d] quarantined err=%d\n",
							op->o_log_prefix, i, lerr );

					} else {
						Debug( LDAP_DEBUG_ANY, "%s: meta_back_getconn[%d] failed err=%d\n",
							op->o_log_prefix, i, lerr );
					}

					if ( META_BACK_ONERR_STOP( mi ) ) {
						if ( sendok & LDAP_BACK_SENDERR ) {
							send_ldap_result( op, rs );
						}
						if ( new_conn ) {
							mc->mc_refcnt = 0;
							meta_back_conn_free( mc );

						} else {
							meta_back_release_conn( mi, mc );
						}

						return NULL;
					}

					continue;
				}

			} else {
				if ( new_conn ) {
					( void )meta_clear_one_candidate( op, mc, i );
				}
			}
		}

		if ( ncandidates == 0 ) {
			if ( new_conn ) {
				mc->mc_refcnt = 0;
				meta_back_conn_free( mc );

			} else {
				meta_back_release_conn( mi, mc );
			}

			if ( rs->sr_err == LDAP_SUCCESS ) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "Unable to select valid candidates";
			}

			if ( sendok & LDAP_BACK_SENDERR ) {
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					rs->sr_matched = op->o_bd->be_suffix[ 0 ].bv_val;
				}
				send_ldap_result( op, rs );
				rs->sr_matched = NULL;
			}

			return NULL;
		}
	}

done:;
	/* clear out meta_back_init_one_conn non-fatal errors */
	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;

	/* touch the timestamp */
	if ( mi->mi_idle_timeout != 0 ) {
		mc->mc_time = op->o_time;
	}

	if ( new_conn ) {
		if ( mi->mi_conn_ttl ) {
			mc->mc_create_time = op->o_time;
		}

		/*
		 * Inserts the newly created metaconn in the avl tree
		 */
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
#if META_BACK_PRINT_CONNTREE > 0
		meta_back_print_conntree( mi, ">>> meta_back_getconn" );
#endif /* META_BACK_PRINT_CONNTREE */

		err = 0;
		if ( LDAP_BACK_PCONN_ISPRIV( mc ) ) {
			if ( mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num < mi->mi_conn_priv_max ) {
				LDAP_TAILQ_INSERT_TAIL( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv, mc, mc_q );
				mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num++;
				LDAP_BACK_CONN_CACHED_SET( mc );

			} else {
				LDAP_BACK_CONN_TAINTED_SET( mc );
			}
			rs->sr_err = 0;

		} else if ( !( sendok & LDAP_BACK_BINDING ) ) {
			err = avl_insert( &mi->mi_conninfo.lai_tree, ( caddr_t )mc,
			       	meta_back_conndn_cmp, meta_back_conndn_dup );
			LDAP_BACK_CONN_CACHED_SET( mc );
		}

#if META_BACK_PRINT_CONNTREE > 0
		meta_back_print_conntree( mi, "<<< meta_back_getconn" );
#endif /* META_BACK_PRINT_CONNTREE */
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

		if ( !LDAP_BACK_PCONN_ISPRIV( mc ) ) {
			/*
			 * Err could be -1 in case a duplicate metaconn is inserted
			 */
			switch ( err ) {
			case 0:
				break;

			case -1:
				LDAP_BACK_CONN_CACHED_CLEAR( mc );
				/* duplicate: free and try to get the newly created one */
				if ( !( sendok & LDAP_BACK_BINDING ) && !LDAP_BACK_USE_TEMPORARIES( mi ) ) {
					mc->mc_refcnt = 0;	
					meta_back_conn_free( mc );
	
					new_conn = 0;
					goto retry_lock;
				}

				LDAP_BACK_CONN_TAINTED_SET( mc );
				break;

			default:
				LDAP_BACK_CONN_CACHED_CLEAR( mc );
				if ( LogTest( LDAP_DEBUG_ANY ) ) {
					char buf[STRLENOF("4294967295U") + 1] = { 0 };
					mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

					Debug( LDAP_DEBUG_ANY,
						"%s meta_back_getconn: candidates=%d conn=%s insert failed\n",
						op->o_log_prefix, ncandidates, buf );
				}
	
				mc->mc_refcnt = 0;	
				meta_back_conn_free( mc );

				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "Proxy bind collision";
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				return NULL;
			}
		}

		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char buf[STRLENOF("4294967295U") + 1] = { 0 };
			mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

			Debug( LDAP_DEBUG_TRACE,
				"%s meta_back_getconn: candidates=%d conn=%s inserted\n",
				op->o_log_prefix, ncandidates, buf );
		}

	} else {
		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char buf[STRLENOF("4294967295U") + 1] = { 0 };
			mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

			Debug( LDAP_DEBUG_TRACE,
				"%s meta_back_getconn: candidates=%d conn=%s fetched\n",
				op->o_log_prefix, ncandidates, buf );
		}
	}

	return mc;
}

void
meta_back_release_conn_lock(
       	metainfo_t		*mi,
	metaconn_t		*mc,
	int			dolock )
{
	assert( mc != NULL );

	if ( dolock ) {
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
	}
	assert( mc->mc_refcnt > 0 );
	mc->mc_refcnt--;
	/* NOTE: the connection is removed if either it is tainted
	 * or if it is shared and no one else is using it.  This needs
	 * to occur because for intrinsic reasons cached connections
	 * that are not privileged would live forever and pollute
	 * the connection space (and eat up resources).  Maybe this
	 * should be configurable... */
	if ( LDAP_BACK_CONN_TAINTED( mc ) || !LDAP_BACK_CONN_CACHED( mc ) ) {
#if META_BACK_PRINT_CONNTREE > 0
		meta_back_print_conntree( mi, ">>> meta_back_release_conn" );
#endif /* META_BACK_PRINT_CONNTREE */

		if ( LDAP_BACK_PCONN_ISPRIV( mc ) ) {
			if ( mc->mc_q.tqe_prev != NULL ) {
				assert( LDAP_BACK_CONN_CACHED( mc ) );
				assert( mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num > 0 );
				LDAP_TAILQ_REMOVE( &mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_priv, mc, mc_q );
				mi->mi_conn_priv[ LDAP_BACK_CONN2PRIV( mc ) ].mic_num--;
				LDAP_TAILQ_ENTRY_INIT( mc, mc_q );

			} else {
				assert( !LDAP_BACK_CONN_CACHED( mc ) );
			}

		} else if ( LDAP_BACK_CONN_CACHED( mc ) ) {
			metaconn_t	*tmpmc;

			tmpmc = avl_delete( &mi->mi_conninfo.lai_tree,
				( caddr_t )mc, meta_back_conndnmc_cmp );

			/* Overparanoid, but useful... */
			assert( tmpmc == NULL || tmpmc == mc );
		}

		LDAP_BACK_CONN_CACHED_CLEAR( mc );

#if META_BACK_PRINT_CONNTREE > 0
		meta_back_print_conntree( mi, "<<< meta_back_release_conn" );
#endif /* META_BACK_PRINT_CONNTREE */

		if ( mc->mc_refcnt == 0 ) {
			meta_back_conn_free( mc );
			mc = NULL;
		}
	}

	if ( mc != NULL && LDAP_BACK_CONN_BINDING( mc ) ) {
		LDAP_BACK_CONN_BINDING_CLEAR( mc );
	}

	if ( dolock ) {
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
	}
}

void
meta_back_quarantine(
	Operation	*op,
	SlapReply	*rs,
	int		candidate )
{
	metainfo_t		*mi = (metainfo_t *)op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];

	slap_retry_info_t	*ri = &mt->mt_quarantine;

	ldap_pvt_thread_mutex_lock( &mt->mt_quarantine_mutex );

	if ( rs->sr_err == LDAP_UNAVAILABLE ) {
		time_t	new_last = slap_get_time();

		switch ( mt->mt_isquarantined ) {
		case LDAP_BACK_FQ_NO:
			if ( ri->ri_last == new_last ) {
				goto done;
			}

			Debug( LDAP_DEBUG_ANY,
				"%s meta_back_quarantine[%d]: enter.\n",
				op->o_log_prefix, candidate, 0 );

			ri->ri_idx = 0;
			ri->ri_count = 0;
			break;

		case LDAP_BACK_FQ_RETRYING:
			if ( LogTest( LDAP_DEBUG_ANY ) ) {
				char	buf[ SLAP_TEXT_BUFLEN ];

				snprintf( buf, sizeof( buf ),
					"meta_back_quarantine[%d]: block #%d try #%d failed",
					candidate, ri->ri_idx, ri->ri_count );
				Debug( LDAP_DEBUG_ANY, "%s %s.\n",
					op->o_log_prefix, buf, 0 );
			}

			++ri->ri_count;
			if ( ri->ri_num[ ri->ri_idx ] != SLAP_RETRYNUM_FOREVER
				&& ri->ri_count == ri->ri_num[ ri->ri_idx ] )
			{
				ri->ri_count = 0;
				++ri->ri_idx;
			}
			break;

		default:
			break;
		}

		mt->mt_isquarantined = LDAP_BACK_FQ_YES;
		ri->ri_last = new_last;

	} else if ( mt->mt_isquarantined == LDAP_BACK_FQ_RETRYING ) {
		Debug( LDAP_DEBUG_ANY,
			"%s meta_back_quarantine[%d]: exit.\n",
			op->o_log_prefix, candidate, 0 );

		if ( mi->mi_quarantine_f ) {
			(void)mi->mi_quarantine_f( mi, candidate,
				mi->mi_quarantine_p );
		}

		ri->ri_count = 0;
		ri->ri_idx = 0;
		mt->mt_isquarantined = LDAP_BACK_FQ_NO;
	}

done:;
	ldap_pvt_thread_mutex_unlock( &mt->mt_quarantine_mutex );
}
