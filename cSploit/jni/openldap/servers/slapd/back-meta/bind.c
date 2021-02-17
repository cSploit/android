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

#include "lutil_ldap.h"

static int
meta_back_proxy_authz_bind(
	metaconn_t		*mc,
	int			candidate,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	int			dolock );

static int
meta_back_single_bind(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		*mc,
	int			candidate );

int
meta_back_bind( Operation *op, SlapReply *rs )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metaconn_t	*mc = NULL;

	int		rc = LDAP_OTHER,
			i,
			gotit = 0,
			isroot = 0;

	SlapReply	*candidates;

	rs->sr_err = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_ARGS, "%s meta_back_bind: dn=\"%s\".\n",
		op->o_log_prefix, op->o_req_dn.bv_val, 0 );

	/* the test on the bind method should be superfluous */
	switch ( be_rootdn_bind( op, rs ) ) {
	case LDAP_SUCCESS:
		if ( META_BACK_DEFER_ROOTDN_BIND( mi ) ) {
			/* frontend will return success */
			return rs->sr_err;
		}

		isroot = 1;
		/* fallthru */

	case SLAP_CB_CONTINUE:
		break;

	default:
		/* be_rootdn_bind() sent result */
		return rs->sr_err;
	}

	/* we need meta_back_getconn() not send result even on error,
	 * because we want to intercept the error and make it
	 * invalidCredentials */
	mc = meta_back_getconn( op, rs, NULL, LDAP_BACK_BIND_DONTSEND );
	if ( !mc ) {
		if ( LogTest( LDAP_DEBUG_ANY ) ) {
			char	buf[ SLAP_TEXT_BUFLEN ];

			snprintf( buf, sizeof( buf ),
				"meta_back_bind: no target "
				"for dn \"%s\" (%d%s%s).",
				op->o_req_dn.bv_val, rs->sr_err,
				rs->sr_text ? ". " : "",
				rs->sr_text ? rs->sr_text : "" );
			Debug( LDAP_DEBUG_ANY,
				"%s %s\n",
				op->o_log_prefix, buf, 0 );
		}

		/* FIXME: there might be cases where we don't want
		 * to map the error onto invalidCredentials */
		switch ( rs->sr_err ) {
		case LDAP_NO_SUCH_OBJECT:
		case LDAP_UNWILLING_TO_PERFORM:
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			rs->sr_text = NULL;
			break;
		}
		send_ldap_result( op, rs );
		return rs->sr_err;
	}

	candidates = meta_back_candidates_get( op );

	/*
	 * Each target is scanned ...
	 */
	mc->mc_authz_target = META_BOUND_NONE;
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		metatarget_t	*mt = mi->mi_targets[ i ];
		int		lerr;

		/*
		 * Skip non-candidates
		 */
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		if ( gotit == 0 ) {
			/* set rc to LDAP_SUCCESS only if at least
			 * one candidate has been tried */
			rc = LDAP_SUCCESS;
			gotit = 1;

		} else if ( !isroot ) {
			/*
			 * A bind operation is expected to have
			 * ONE CANDIDATE ONLY!
			 */
			Debug( LDAP_DEBUG_ANY,
				"### %s meta_back_bind: more than one"
				" candidate selected...\n",
				op->o_log_prefix, 0, 0 );
		}

		if ( isroot ) {
			if ( mt->mt_idassert_authmethod == LDAP_AUTH_NONE
				|| BER_BVISNULL( &mt->mt_idassert_authcDN ) )
			{
				metasingleconn_t	*msc = &mc->mc_conns[ i ];

				/* skip the target if no pseudorootdn is provided */
				if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
					ch_free( msc->msc_bound_ndn.bv_val );
					BER_BVZERO( &msc->msc_bound_ndn );
				}

				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					/* destroy sensitive data */
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
					ch_free( msc->msc_cred.bv_val );
					BER_BVZERO( &msc->msc_cred );
				}

				continue;
			}

			
			(void)meta_back_proxy_authz_bind( mc, i, op, rs, LDAP_BACK_DONTSEND, 1 );
			lerr = rs->sr_err;

		} else {
			lerr = meta_back_single_bind( op, rs, mc, i );
		}

		if ( lerr != LDAP_SUCCESS ) {
			rc = rs->sr_err = lerr;

			/* FIXME: in some cases (e.g. unavailable)
			 * do not assume it's not candidate; rather
			 * mark this as an error to be eventually
			 * reported to client */
			META_CANDIDATE_CLEAR( &candidates[ i ] );
			break;
		}
	}

	/* must re-insert if local DN changed as result of bind */
	if ( rc == LDAP_SUCCESS ) {
		if ( isroot ) {
			mc->mc_authz_target = META_BOUND_ALL;
		}

		if ( !LDAP_BACK_PCONN_ISPRIV( mc )
			&& !dn_match( &op->o_req_ndn, &mc->mc_local_ndn ) )
		{
			int		lerr;

			/* wait for all other ops to release the connection */
			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
			assert( mc->mc_refcnt == 1 );
#if META_BACK_PRINT_CONNTREE > 0
			meta_back_print_conntree( mi, ">>> meta_back_bind" );
#endif /* META_BACK_PRINT_CONNTREE */

			/* delete all cached connections with the current connection */
			if ( LDAP_BACK_SINGLECONN( mi ) ) {
				metaconn_t	*tmpmc;

				while ( ( tmpmc = avl_delete( &mi->mi_conninfo.lai_tree, (caddr_t)mc, meta_back_conn_cmp ) ) != NULL )
				{
					assert( !LDAP_BACK_PCONN_ISPRIV( mc ) );
					Debug( LDAP_DEBUG_TRACE,
						"=>meta_back_bind: destroying conn %lu (refcnt=%u)\n",
						mc->mc_conn->c_connid, mc->mc_refcnt, 0 );

					if ( tmpmc->mc_refcnt != 0 ) {
						/* taint it */
						LDAP_BACK_CONN_TAINTED_SET( tmpmc );

					} else {
						/*
						 * Needs a test because the handler may be corrupted,
						 * and calling ldap_unbind on a corrupted header results
						 * in a segmentation fault
						 */
						meta_back_conn_free( tmpmc );
					}
				}
			}

			ber_bvreplace( &mc->mc_local_ndn, &op->o_req_ndn );
			lerr = avl_insert( &mi->mi_conninfo.lai_tree, (caddr_t)mc,
				meta_back_conndn_cmp, meta_back_conndn_dup );
#if META_BACK_PRINT_CONNTREE > 0
			meta_back_print_conntree( mi, "<<< meta_back_bind" );
#endif /* META_BACK_PRINT_CONNTREE */
			if ( lerr == 0 ) {
#if 0
				/* NOTE: a connection cannot be privileged
				 * and be in the avl tree at the same time
				 */
				if ( isroot ) {
					LDAP_BACK_CONN_ISPRIV_SET( mc );
					LDAP_BACK_PCONN_SET( mc, op );
				}
#endif
				LDAP_BACK_CONN_CACHED_SET( mc );

			} else {
				LDAP_BACK_CONN_CACHED_CLEAR( mc );
			}
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		}
	}

	if ( mc != NULL ) {
		meta_back_release_conn( mi, mc );
	}

	/*
	 * rc is LDAP_SUCCESS if at least one bind succeeded,
	 * err is the last error that occurred during a bind;
	 * if at least (and at most?) one bind succeeds, fine.
	 */
	if ( rc != LDAP_SUCCESS ) {
		
		/*
		 * deal with bind failure ...
		 */

		/*
		 * no target was found within the naming context, 
		 * so bind must fail with invalid credentials
		 */
		if ( rs->sr_err == LDAP_SUCCESS && gotit == 0 ) {
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
		} else {
			rs->sr_err = slap_map_api2result( rs );
		}
		send_ldap_result( op, rs );
		return rs->sr_err;

	}

	return LDAP_SUCCESS;
}

static int
meta_back_bind_op_result(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		*mc,
	int			candidate,
	int			msgid,
	ldap_back_send_t	sendok,
	int			dolock )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	LDAPMessage		*res;
	struct timeval		tv;
	int			rc;
	int			nretries = mt->mt_nretries;
	char			buf[ SLAP_TEXT_BUFLEN ];

	Debug( LDAP_DEBUG_TRACE,
		">>> %s meta_back_bind_op_result[%d]\n",
		op->o_log_prefix, candidate, 0 );

	/* make sure this is clean */
	assert( rs->sr_ctrls == NULL );

	if ( rs->sr_err == LDAP_SUCCESS ) {
		time_t		stoptime = (time_t)(-1),
				timeout;
		int		timeout_err = op->o_protocol >= LDAP_VERSION3 ?
				LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
		const char	*timeout_text = "Operation timed out";
		slap_op_t	opidx = slap_req2op( op->o_tag );

		/* since timeout is not specified, compute and use
		 * the one specific to the ongoing operation */
		if ( opidx == LDAP_REQ_SEARCH ) {
			if ( op->ors_tlimit <= 0 ) {
				timeout = 0;

			} else {
				timeout = op->ors_tlimit;
				timeout_err = LDAP_TIMELIMIT_EXCEEDED;
				timeout_text = NULL;
			}

		} else {
			timeout = mt->mt_timeout[ opidx ];
		}

		/* better than nothing :) */
		if ( timeout == 0 ) {
			if ( mi->mi_idle_timeout ) {
				timeout = mi->mi_idle_timeout;

			} else if ( mi->mi_conn_ttl ) {
				timeout = mi->mi_conn_ttl;
			}
		}

		if ( timeout ) {
			stoptime = op->o_time + timeout;
		}

		LDAP_BACK_TV_SET( &tv );

		/*
		 * handle response!!!
		 */
retry:;
		rc = ldap_result( msc->msc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
		switch ( rc ) {
		case 0:
			if ( nretries != META_RETRY_NEVER 
				|| ( timeout && slap_get_time() <= stoptime ) )
			{
				ldap_pvt_thread_yield();
				if ( nretries > 0 ) {
					nretries--;
				}
				tv = mt->mt_bind_timeout;
				goto retry;
			}

			/* don't let anyone else use this handler,
			 * because there's a pending bind that will not
			 * be acknowledged */
			if ( dolock) {
				ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
			}
			assert( LDAP_BACK_CONN_BINDING( msc ) );

#ifdef DEBUG_205
			Debug( LDAP_DEBUG_ANY, "### %s meta_back_bind_op_result ldap_unbind_ext[%d] ld=%p\n",
				op->o_log_prefix, candidate, (void *)msc->msc_ld );
#endif /* DEBUG_205 */

			meta_clear_one_candidate( op, mc, candidate );
			if ( dolock ) {
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			}

			rs->sr_err = timeout_err;
			rs->sr_text = timeout_text;
			break;

		case -1:
			ldap_get_option( msc->msc_ld, LDAP_OPT_ERROR_NUMBER,
				&rs->sr_err );

			snprintf( buf, sizeof( buf ),
				"err=%d (%s) nretries=%d",
				rs->sr_err, ldap_err2string( rs->sr_err ), nretries );
			Debug( LDAP_DEBUG_ANY,
				"### %s meta_back_bind_op_result[%d]: %s.\n",
				op->o_log_prefix, candidate, buf );
			break;

		default:
			/* only touch when activity actually took place... */
			if ( mi->mi_idle_timeout != 0 && msc->msc_time < op->o_time ) {
				msc->msc_time = op->o_time;
			}

			/* FIXME: matched? referrals? response controls? */
			rc = ldap_parse_result( msc->msc_ld, res, &rs->sr_err,
					NULL, NULL, NULL, NULL, 1 );
			if ( rc != LDAP_SUCCESS ) {
				rs->sr_err = rc;
			}
			rs->sr_err = slap_map_api2result( rs );
			break;
		}
	}

	rs->sr_err = slap_map_api2result( rs );

	Debug( LDAP_DEBUG_TRACE,
		"<<< %s meta_back_bind_op_result[%d] err=%d\n",
		op->o_log_prefix, candidate, rs->sr_err );

	return rs->sr_err;
}

/*
 * meta_back_single_bind
 *
 * attempts to perform a bind with creds
 */
static int
meta_back_single_bind(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		*mc,
	int			candidate )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	struct berval		mdn = BER_BVNULL;
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	int			msgid;
	dncookie		dc;
	struct berval		save_o_dn;
	int			save_o_do_not_cache;
	LDAPControl		**ctrls = NULL;
	
	if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
		ch_free( msc->msc_bound_ndn.bv_val );
		BER_BVZERO( &msc->msc_bound_ndn );
	}

	if ( !BER_BVISNULL( &msc->msc_cred ) ) {
		/* destroy sensitive data */
		memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
		ch_free( msc->msc_cred.bv_val );
		BER_BVZERO( &msc->msc_cred );
	}

	/*
	 * Rewrite the bind dn if needed
	 */
	dc.target = mt;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "bindDN";

	if ( ldap_back_dn_massage( &dc, &op->o_req_dn, &mdn ) ) {
		rs->sr_text = "DN rewrite error";
		rs->sr_err = LDAP_OTHER;
		return rs->sr_err;
	}

	/* don't add proxyAuthz; set the bindDN */
	save_o_dn = op->o_dn;
	save_o_do_not_cache = op->o_do_not_cache;
	op->o_do_not_cache = 1;
	op->o_dn = op->o_req_dn;

	ctrls = op->o_ctrls;
	rs->sr_err = meta_back_controls_add( op, rs, mc, candidate, &ctrls );
	op->o_dn = save_o_dn;
	op->o_do_not_cache = save_o_do_not_cache;
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto return_results;
	}

	/* FIXME: this fixes the bind problem right now; we need
	 * to use the asynchronous version to get the "matched"
	 * and more in case of failure ... */
	/* FIXME: should we check if at least some of the op->o_ctrls
	 * can/should be passed? */
	for (;;) {
		rs->sr_err = ldap_sasl_bind( msc->msc_ld, mdn.bv_val,
			LDAP_SASL_SIMPLE, &op->orb_cred,
			ctrls, NULL, &msgid );
		if ( rs->sr_err != LDAP_X_CONNECTING ) {
			break;
		}
		ldap_pvt_thread_yield();
	}

	mi->mi_ldap_extra->controls_free( op, rs, &ctrls );

	meta_back_bind_op_result( op, rs, mc, candidate, msgid, LDAP_BACK_DONTSEND, 1 );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto return_results;
	}

	/* If defined, proxyAuthz will be used also when
	 * back-ldap is the authorizing backend; for this
	 * purpose, a successful bind is followed by a
	 * bind with the configured identity assertion */
	/* NOTE: use with care */
	if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) {
		meta_back_proxy_authz_bind( mc, candidate, op, rs, LDAP_BACK_SENDERR, 1 );
		if ( !LDAP_BACK_CONN_ISBOUND( msc ) ) {
			goto return_results;
		}
		goto cache_refresh;
	}

	ber_bvreplace( &msc->msc_bound_ndn, &op->o_req_ndn );
	LDAP_BACK_CONN_ISBOUND_SET( msc );
	mc->mc_authz_target = candidate;

	if ( META_BACK_TGT_SAVECRED( mt ) ) {
		if ( !BER_BVISNULL( &msc->msc_cred ) ) {
			memset( msc->msc_cred.bv_val, 0,
				msc->msc_cred.bv_len );
		}
		ber_bvreplace( &msc->msc_cred, &op->orb_cred );
		ldap_set_rebind_proc( msc->msc_ld, mt->mt_rebind_f, msc );
	}

cache_refresh:;
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED
			&& !BER_BVISEMPTY( &op->o_req_ndn ) )
	{
		( void )meta_dncache_update_entry( &mi->mi_cache,
				&op->o_req_ndn, candidate );
	}

return_results:;
	if ( mdn.bv_val != op->o_req_dn.bv_val ) {
		free( mdn.bv_val );
	}

	if ( META_BACK_TGT_QUARANTINE( mt ) ) {
		meta_back_quarantine( op, rs, candidate );
	}

	return rs->sr_err;
}

/*
 * meta_back_single_dobind
 */
int
meta_back_single_dobind(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		**mcp,
	int			candidate,
	ldap_back_send_t	sendok,
	int			nretries,
	int			dolock )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metaconn_t		*mc = *mcp;
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	int			msgid;

	assert( !LDAP_BACK_CONN_ISBOUND( msc ) );

	/* NOTE: this obsoletes pseudorootdn */
	if ( op->o_conn != NULL &&
		!op->o_do_not_cache &&
		( BER_BVISNULL( &msc->msc_bound_ndn ) ||
			BER_BVISEMPTY( &msc->msc_bound_ndn ) ||
			( LDAP_BACK_CONN_ISPRIV( mc ) && dn_match( &msc->msc_bound_ndn, &mt->mt_idassert_authcDN ) ) ||
			( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) ) )
	{
		(void)meta_back_proxy_authz_bind( mc, candidate, op, rs, sendok, dolock );

	} else {
		char *binddn = "";
		struct berval cred = BER_BVC( "" );

		/* use credentials if available */
		if ( !BER_BVISNULL( &msc->msc_bound_ndn )
			&& !BER_BVISNULL( &msc->msc_cred ) )
		{
			binddn = msc->msc_bound_ndn.bv_val;
			cred = msc->msc_cred;
		}

		/* FIXME: should we check if at least some of the op->o_ctrls
		 * can/should be passed? */
		if(!dolock) {
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		}

		for (;;) {
			rs->sr_err = ldap_sasl_bind( msc->msc_ld,
				binddn, LDAP_SASL_SIMPLE, &cred,
				NULL, NULL, &msgid );
			if ( rs->sr_err != LDAP_X_CONNECTING ) {
				break;
			}
			ldap_pvt_thread_yield();
		}

		if(!dolock) {
			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		}

		rs->sr_err = meta_back_bind_op_result( op, rs, mc, candidate, msgid, sendok, dolock );

		/* if bind succeeded, but anonymous, clear msc_bound_ndn */
		if ( rs->sr_err != LDAP_SUCCESS || binddn[0] == '\0' ) {
			if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
				ber_memfree( msc->msc_bound_ndn.bv_val );
				BER_BVZERO( &msc->msc_bound_ndn );
			}

			if ( !BER_BVISNULL( &msc->msc_cred ) ) {
				memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
				ber_memfree( msc->msc_cred.bv_val );
				BER_BVZERO( &msc->msc_cred );
			}
		}
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		if ( dolock ) {
			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		}
		LDAP_BACK_CONN_BINDING_CLEAR( msc );
		if ( META_BACK_ONERR_STOP( mi ) ) {
			LDAP_BACK_CONN_TAINTED_SET( mc );
			meta_back_release_conn_lock( mi, mc, 0 );
			*mcp = NULL;
		}
		if ( dolock ) {
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		}
	}

	if ( META_BACK_TGT_QUARANTINE( mt ) ) {
		meta_back_quarantine( op, rs, candidate );
	}

	return rs->sr_err;
}

/*
 * meta_back_dobind
 */
int
meta_back_dobind(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		*mc,
	ldap_back_send_t	sendok )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;

	int			bound = 0,
				i,
				isroot = 0;

	SlapReply		*candidates;

	if ( be_isroot( op ) ) {
		isroot = 1;
	}

	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		char buf[STRLENOF("4294967295U") + 1] = { 0 };
		mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

		Debug( LDAP_DEBUG_TRACE,
			"%s meta_back_dobind: conn=%s%s\n",
			op->o_log_prefix, buf,
			isroot ? " (isroot)" : "" );
	}

	/*
	 * all the targets are bound as pseudoroot
	 */
	if ( mc->mc_authz_target == META_BOUND_ALL ) {
		bound = 1;
		goto done;
	}

	candidates = meta_back_candidates_get( op );

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		metatarget_t		*mt = mi->mi_targets[ i ];
		metasingleconn_t	*msc = &mc->mc_conns[ i ];
		int			rc;

		/*
		 * Not a candidate
		 */
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		assert( msc->msc_ld != NULL );

		/*
		 * If the target is already bound it is skipped
		 */

retry_binding:;
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		if ( LDAP_BACK_CONN_ISBOUND( msc )
			|| ( LDAP_BACK_CONN_ISANON( msc )
				&& mt->mt_idassert_authmethod == LDAP_AUTH_NONE ) )
		{
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			++bound;
			continue;

		} else if ( META_BACK_CONN_CREATING( msc ) || LDAP_BACK_CONN_BINDING( msc ) )
		{
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			ldap_pvt_thread_yield();
			goto retry_binding;

		}

		LDAP_BACK_CONN_BINDING_SET( msc );
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

		rc = meta_back_single_dobind( op, rs, &mc, i,
			LDAP_BACK_DONTSEND, mt->mt_nretries, 1 );
		/*
		 * NOTE: meta_back_single_dobind() already retries;
		 * in case of failure, it resets mc...
		 */
		if ( rc != LDAP_SUCCESS ) {
			char		buf[ SLAP_TEXT_BUFLEN ];

			if ( mc == NULL ) {
				/* meta_back_single_dobind() already sent 
				 * response and released connection */
				goto send_err;
			}


			if ( rc == LDAP_UNAVAILABLE ) {
				/* FIXME: meta_back_retry() already re-calls
				 * meta_back_single_dobind() */
				if ( meta_back_retry( op, rs, &mc, i, sendok ) ) {
					goto retry_ok;
				}

				if ( mc != NULL ) {
					ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
					LDAP_BACK_CONN_BINDING_CLEAR( msc );
					meta_back_release_conn_lock( mi, mc, 0 );
					ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
				}

				return 0;
			}

			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
			LDAP_BACK_CONN_BINDING_CLEAR( msc );
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

			snprintf( buf, sizeof( buf ),
				"meta_back_dobind[%d]: (%s) err=%d (%s).",
				i, isroot ? op->o_bd->be_rootdn.bv_val : "anonymous",
				rc, ldap_err2string( rc ) );
			Debug( LDAP_DEBUG_ANY,
				"%s %s\n",
				op->o_log_prefix, buf, 0 );

			/*
			 * null cred bind should always succeed
			 * as anonymous, so a failure means
			 * the target is no longer candidate possibly
			 * due to technical reasons (remote host down?)
			 * so better clear the handle
			 */
			/* leave the target candidate, but record the error for later use */
			candidates[ i ].sr_err = rc;
			if ( META_BACK_ONERR_STOP( mi ) ) {
				bound = 0;
				goto done;
			}

			continue;
		} /* else */

retry_ok:;
		Debug( LDAP_DEBUG_TRACE,
			"%s meta_back_dobind[%d]: "
			"(%s)\n",
			op->o_log_prefix, i,
			isroot ? op->o_bd->be_rootdn.bv_val : "anonymous" );

		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		LDAP_BACK_CONN_BINDING_CLEAR( msc );
		if ( isroot ) {
			LDAP_BACK_CONN_ISBOUND_SET( msc );
		} else {
			LDAP_BACK_CONN_ISANON_SET( msc );
		}
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		++bound;
	}

done:;
	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		char buf[STRLENOF("4294967295U") + 1] = { 0 };
		mi->mi_ldap_extra->connid2str( &mc->mc_base, buf, sizeof(buf) );

		Debug( LDAP_DEBUG_TRACE,
			"%s meta_back_dobind: conn=%s bound=%d\n",
			op->o_log_prefix, buf, bound );
	}

	if ( bound == 0 ) {
		meta_back_release_conn( mi, mc );

send_err:;
		if ( sendok & LDAP_BACK_SENDERR ) {
			if ( rs->sr_err == LDAP_SUCCESS ) {
				rs->sr_err = LDAP_BUSY;
			}
			send_ldap_result( op, rs );
		}

		return 0;
	}

	return ( bound > 0 );
}

/*
 * meta_back_default_rebind
 *
 * This is a callback used for chasing referrals using the same
 * credentials as the original user on this session.
 */
int 
meta_back_default_rebind(
	LDAP			*ld,
	LDAP_CONST char		*url,
	ber_tag_t		request,
	ber_int_t		msgid,
	void			*params )
{
	metasingleconn_t	*msc = ( metasingleconn_t * )params;

	return ldap_sasl_bind_s( ld, msc->msc_bound_ndn.bv_val,
			LDAP_SASL_SIMPLE, &msc->msc_cred,
			NULL, NULL, NULL );
}

/*
 * meta_back_default_urllist
 *
 * This is a callback used for mucking with the urllist
 */
int 
meta_back_default_urllist(
	LDAP		*ld,
	LDAPURLDesc	**urllist,
	LDAPURLDesc	**url,
	void		*params )
{
	metatarget_t	*mt = (metatarget_t *)params;
	LDAPURLDesc	**urltail;

	if ( urllist == url ) {
		return LDAP_SUCCESS;
	}

	for ( urltail = &(*url)->lud_next; *urltail; urltail = &(*urltail)->lud_next )
		/* count */ ;

	*urltail = *urllist;
	*urllist = *url;
	*url = NULL;

	ldap_pvt_thread_mutex_lock( &mt->mt_uri_mutex );
	if ( mt->mt_uri ) {
		ch_free( mt->mt_uri );
	}

	ldap_get_option( ld, LDAP_OPT_URI, (void *)&mt->mt_uri );
	ldap_pvt_thread_mutex_unlock( &mt->mt_uri_mutex );

	return LDAP_SUCCESS;
}

int
meta_back_cancel(
	metaconn_t		*mc,
	Operation		*op,
	SlapReply		*rs,
	ber_int_t		msgid,
	int			candidate,
	ldap_back_send_t	sendok )
{
	metainfo_t		*mi = (metainfo_t *)op->o_bd->be_private;

	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	int			rc = LDAP_OTHER;

	Debug( LDAP_DEBUG_TRACE, ">>> %s meta_back_cancel[%d] msgid=%d\n",
		op->o_log_prefix, candidate, msgid );

	/* default behavior */
	if ( META_BACK_TGT_ABANDON( mt ) ) {
		rc = ldap_abandon_ext( msc->msc_ld, msgid, NULL, NULL );

	} else if ( META_BACK_TGT_IGNORE( mt ) ) {
		rc = ldap_pvt_discard( msc->msc_ld, msgid );

	} else if ( META_BACK_TGT_CANCEL( mt ) ) {
		rc = ldap_cancel_s( msc->msc_ld, msgid, NULL, NULL );

	} else {
		assert( 0 );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< %s meta_back_cancel[%d] err=%d\n",
		op->o_log_prefix, candidate, rc );

	return rc;
}



/*
 * FIXME: error return must be handled in a cleaner way ...
 */
int
meta_back_op_result(
	metaconn_t		*mc,
	Operation		*op,
	SlapReply		*rs,
	int			candidate,
	ber_int_t		msgid,
	time_t			timeout,
	ldap_back_send_t	sendok )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;

	const char	*save_text = rs->sr_text,
			*save_matched = rs->sr_matched;
	BerVarray	save_ref = rs->sr_ref;
	LDAPControl	**save_ctrls = rs->sr_ctrls;
	void		*matched_ctx = NULL;

	char		*matched = NULL;
	char		*text = NULL;
	char		**refs = NULL;
	LDAPControl	**ctrls = NULL;

	assert( mc != NULL );

	rs->sr_text = NULL;
	rs->sr_matched = NULL;
	rs->sr_ref = NULL;
	rs->sr_ctrls = NULL;

	if ( candidate != META_TARGET_NONE ) {
		metatarget_t		*mt = mi->mi_targets[ candidate ];
		metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

		if ( LDAP_ERR_OK( rs->sr_err ) ) {
			int		rc;
			struct timeval	tv;
			LDAPMessage	*res = NULL;
			time_t		stoptime = (time_t)(-1);
			int		timeout_err = op->o_protocol >= LDAP_VERSION3 ?
						LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
			const char	*timeout_text = "Operation timed out";

			/* if timeout is not specified, compute and use
			 * the one specific to the ongoing operation */
			if ( timeout == (time_t)(-1) ) {
				slap_op_t	opidx = slap_req2op( op->o_tag );

				if ( opidx == SLAP_OP_SEARCH ) {
					if ( op->ors_tlimit <= 0 ) {
						timeout = 0;

					} else {
						timeout = op->ors_tlimit;
						timeout_err = LDAP_TIMELIMIT_EXCEEDED;
						timeout_text = NULL;
					}

				} else {
					timeout = mt->mt_timeout[ opidx ];
				}
			}

			/* better than nothing :) */
			if ( timeout == 0 ) {
				if ( mi->mi_idle_timeout ) {
					timeout = mi->mi_idle_timeout;

				} else if ( mi->mi_conn_ttl ) {
					timeout = mi->mi_conn_ttl;
				}
			}

			if ( timeout ) {
				stoptime = op->o_time + timeout;
			}

			LDAP_BACK_TV_SET( &tv );

retry:;
			rc = ldap_result( msc->msc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
			switch ( rc ) {
			case 0:
				if ( timeout && slap_get_time() > stoptime ) {
					(void)meta_back_cancel( mc, op, rs, msgid, candidate, sendok );
					rs->sr_err = timeout_err;
					rs->sr_text = timeout_text;
					break;
				}

				LDAP_BACK_TV_SET( &tv );
				ldap_pvt_thread_yield();
				goto retry;

			case -1:
				ldap_get_option( msc->msc_ld, LDAP_OPT_RESULT_CODE,
						&rs->sr_err );
				break;


			/* otherwise get the result; if it is not
			 * LDAP_SUCCESS, record it in the reply
			 * structure (this includes 
			 * LDAP_COMPARE_{TRUE|FALSE}) */
			default:
				/* only touch when activity actually took place... */
				if ( mi->mi_idle_timeout != 0 && msc->msc_time < op->o_time ) {
					msc->msc_time = op->o_time;
				}

				rc = ldap_parse_result( msc->msc_ld, res, &rs->sr_err,
						&matched, &text, &refs, &ctrls, 1 );
				res = NULL;
				if ( rc == LDAP_SUCCESS ) {
					rs->sr_text = text;
				} else {
					rs->sr_err = rc;
				}
				rs->sr_err = slap_map_api2result( rs );

				/* RFC 4511: referrals can only appear
				 * if result code is LDAP_REFERRAL */
				if ( refs != NULL
					&& refs[ 0 ] != NULL
					&& refs[ 0 ][ 0 ] != '\0' )
				{
					if ( rs->sr_err != LDAP_REFERRAL ) {
						Debug( LDAP_DEBUG_ANY,
							"%s meta_back_op_result[%d]: "
							"got referrals with err=%d\n",
							op->o_log_prefix,
							candidate, rs->sr_err );

					} else {
						int	i;
	
						for ( i = 0; refs[ i ] != NULL; i++ )
							/* count */ ;
						rs->sr_ref = op->o_tmpalloc( sizeof( struct berval ) * ( i + 1 ),
							op->o_tmpmemctx );
						for ( i = 0; refs[ i ] != NULL; i++ ) {
							ber_str2bv( refs[ i ], 0, 0, &rs->sr_ref[ i ] );
						}
						BER_BVZERO( &rs->sr_ref[ i ] );
					}

				} else if ( rs->sr_err == LDAP_REFERRAL ) {
					Debug( LDAP_DEBUG_ANY,
						"%s meta_back_op_result[%d]: "
						"got err=%d with null "
						"or empty referrals\n",
						op->o_log_prefix,
						candidate, rs->sr_err );

					rs->sr_err = LDAP_NO_SUCH_OBJECT;
				}

				if ( ctrls != NULL ) {
					rs->sr_ctrls = ctrls;
				}
			}

			assert( res == NULL );
		}

		/* if the error in the reply structure is not
		 * LDAP_SUCCESS, try to map it from client 
		 * to server error */
		if ( !LDAP_ERR_OK( rs->sr_err ) ) {
			rs->sr_err = slap_map_api2result( rs );

			/* internal ops ( op->o_conn == NULL ) 
			 * must not reply to client */
			if ( op->o_conn && !op->o_do_not_cache && matched ) {

				/* record the (massaged) matched
				 * DN into the reply structure */
				rs->sr_matched = matched;
			}
		}

		if ( META_BACK_TGT_QUARANTINE( mt ) ) {
			meta_back_quarantine( op, rs, candidate );
		}

	} else {
		int	i,
			err = rs->sr_err;

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			metasingleconn_t	*msc = &mc->mc_conns[ i ];
			char			*xtext = NULL;
			char			*xmatched = NULL;

			if ( msc->msc_ld == NULL ) {
				continue;
			}

			rs->sr_err = LDAP_SUCCESS;

			ldap_get_option( msc->msc_ld, LDAP_OPT_RESULT_CODE, &rs->sr_err );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				/*
				 * better check the type of error. In some cases
				 * (search ?) it might be better to return a
				 * success if at least one of the targets gave
				 * positive result ...
				 */
				ldap_get_option( msc->msc_ld,
						LDAP_OPT_DIAGNOSTIC_MESSAGE, &xtext );
				if ( xtext != NULL && xtext [ 0 ] == '\0' ) {
					ldap_memfree( xtext );
					xtext = NULL;
				}

				ldap_get_option( msc->msc_ld,
						LDAP_OPT_MATCHED_DN, &xmatched );
				if ( xmatched != NULL && xmatched[ 0 ] == '\0' ) {
					ldap_memfree( xmatched );
					xmatched = NULL;
				}

				rs->sr_err = slap_map_api2result( rs );
	
				if ( LogTest( LDAP_DEBUG_ANY ) ) {
					char	buf[ SLAP_TEXT_BUFLEN ];

					snprintf( buf, sizeof( buf ),
						"meta_back_op_result[%d] "
						"err=%d text=\"%s\" matched=\"%s\"", 
						i, rs->sr_err,
						( xtext ? xtext : "" ),
						( xmatched ? xmatched : "" ) );
					Debug( LDAP_DEBUG_ANY, "%s %s.\n",
						op->o_log_prefix, buf, 0 );
				}

				/*
				 * FIXME: need to rewrite "match" (need rwinfo)
				 */
				switch ( rs->sr_err ) {
				default:
					err = rs->sr_err;
					if ( xtext != NULL ) {
						if ( text ) {
							ldap_memfree( text );
						}
						text = xtext;
						xtext = NULL;
					}
					if ( xmatched != NULL ) {
						if ( matched ) {
							ldap_memfree( matched );
						}
						matched = xmatched;
						xmatched = NULL;
					}
					break;
				}

				if ( xtext ) {
					ldap_memfree( xtext );
				}
	
				if ( xmatched ) {
					ldap_memfree( xmatched );
				}
			}

			if ( META_BACK_TGT_QUARANTINE( mi->mi_targets[ i ] ) ) {
				meta_back_quarantine( op, rs, i );
			}
		}

		if ( err != LDAP_SUCCESS ) {
			rs->sr_err = err;
		}
	}

	if ( matched != NULL ) {
		struct berval	dn, pdn;

		ber_str2bv( matched, 0, 0, &dn );
		if ( dnPretty( NULL, &dn, &pdn, op->o_tmpmemctx ) == LDAP_SUCCESS ) {
			ldap_memfree( matched );
			matched_ctx = op->o_tmpmemctx;
			matched = pdn.bv_val;
		}
		rs->sr_matched = matched;
	}

	if ( rs->sr_err == LDAP_UNAVAILABLE ) {
		if ( !( sendok & LDAP_BACK_RETRYING ) ) {
			if ( op->o_conn && ( sendok & LDAP_BACK_SENDERR ) ) {
				if ( rs->sr_text == NULL ) rs->sr_text = "Proxy operation retry failed";
				send_ldap_result( op, rs );
			}
		}

	} else if ( op->o_conn &&
		( ( ( sendok & LDAP_BACK_SENDOK ) && LDAP_ERR_OK( rs->sr_err ) )
			|| ( ( sendok & LDAP_BACK_SENDERR ) && !LDAP_ERR_OK( rs->sr_err ) ) ) )
	{
		send_ldap_result( op, rs );
	}
	if ( matched ) {
		op->o_tmpfree( (char *)rs->sr_matched, matched_ctx );
	}
	if ( text ) {
		ldap_memfree( text );
	}
	if ( rs->sr_ref ) {
		op->o_tmpfree( rs->sr_ref, op->o_tmpmemctx );
		rs->sr_ref = NULL;
	}
	if ( refs ) {
		ber_memvfree( (void **)refs );
	}
	if ( ctrls ) {
		assert( rs->sr_ctrls != NULL );
		ldap_controls_free( ctrls );
	}

	rs->sr_text = save_text;
	rs->sr_matched = save_matched;
	rs->sr_ref = save_ref;
	rs->sr_ctrls = save_ctrls;

	return( LDAP_ERR_OK( rs->sr_err ) ? LDAP_SUCCESS : rs->sr_err );
}

/*
 * meta_back_proxy_authz_cred()
 *
 * prepares credentials & method for meta_back_proxy_authz_bind();
 * or, if method is SASL, performs the SASL bind directly.
 */
int
meta_back_proxy_authz_cred(
	metaconn_t		*mc,
	int			candidate,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	struct berval		*binddn,
	struct berval		*bindcred,
	int			*method )
{
	metainfo_t		*mi = (metainfo_t *)op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	struct berval		ndn;
	int			dobind = 0;

	/* don't proxyAuthz if protocol is not LDAPv3 */
	switch ( mt->mt_version ) {
	case LDAP_VERSION3:
		break;

	case 0:
		if ( op->o_protocol == 0 || op->o_protocol == LDAP_VERSION3 ) {
			break;
		}
		/* fall thru */

	default:
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
		LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
		goto done;
	}

	if ( op->o_tag == LDAP_REQ_BIND ) {
		ndn = op->o_req_ndn;

	} else if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;

	} else {
		ndn = op->o_ndn;
	}
	rs->sr_err = LDAP_SUCCESS;

	/*
	 * FIXME: we need to let clients use proxyAuthz
	 * otherwise we cannot do symmetric pools of servers;
	 * we have to live with the fact that a user can
	 * authorize itself as any ID that is allowed
	 * by the authzTo directive of the "proxyauthzdn".
	 */
	/*
	 * NOTE: current Proxy Authorization specification
	 * and implementation do not allow proxy authorization
	 * control to be provided with Bind requests
	 */
	/*
	 * if no bind took place yet, but the connection is bound
	 * and the "proxyauthzdn" is set, then bind as 
	 * "proxyauthzdn" and explicitly add the proxyAuthz 
	 * control to every operation with the dn bound 
	 * to the connection as control value.
	 */

	/* bind as proxyauthzdn only if no idassert mode
	 * is requested, or if the client's identity
	 * is authorized */
	switch ( mt->mt_idassert_mode ) {
	case LDAP_BACK_IDASSERT_LEGACY:
		if ( !BER_BVISNULL( &ndn ) && !BER_BVISEMPTY( &ndn ) ) {
			if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) && !BER_BVISEMPTY( &mt->mt_idassert_authcDN ) )
			{
				*binddn = mt->mt_idassert_authcDN;
				*bindcred = mt->mt_idassert_passwd;
				dobind = 1;
			}
		}
		break;

	default:
		/* NOTE: rootdn can always idassert */
		if ( BER_BVISNULL( &ndn )
			&& mt->mt_idassert_authz == NULL
			&& !( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
		{
			if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
				rs->sr_err = LDAP_INAPPROPRIATE_AUTH;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
				goto done;

			}

			rs->sr_err = LDAP_SUCCESS;
			*binddn = slap_empty_bv;
			*bindcred = slap_empty_bv;
			break;

		} else if ( mt->mt_idassert_authz && !be_isroot( op ) ) {
			struct berval authcDN;

			if ( BER_BVISNULL( &ndn ) ) {
				authcDN = slap_empty_bv;

			} else {
				authcDN = ndn;
			}	
			rs->sr_err = slap_sasl_matches( op, mt->mt_idassert_authz,
					&authcDN, &authcDN );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
					if ( sendok & LDAP_BACK_SENDERR ) {
						send_ldap_result( op, rs );
					}
					LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
					goto done;
				}

				rs->sr_err = LDAP_SUCCESS;
				*binddn = slap_empty_bv;
				*bindcred = slap_empty_bv;
				break;
			}
		}

		*binddn = mt->mt_idassert_authcDN;
		*bindcred = mt->mt_idassert_passwd;
		dobind = 1;
		break;
	}

	if ( dobind && mt->mt_idassert_authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void		*defaults = NULL;
		struct berval	authzID = BER_BVNULL;
		int		freeauthz = 0;

		/* if SASL supports native authz, prepare for it */
		if ( ( !op->o_do_not_cache || !op->o_is_auth_check ) &&
				( mt->mt_idassert_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) )
		{
			switch ( mt->mt_idassert_mode ) {
			case LDAP_BACK_IDASSERT_OTHERID:
			case LDAP_BACK_IDASSERT_OTHERDN:
				authzID = mt->mt_idassert_authzID;
				break;

			case LDAP_BACK_IDASSERT_ANONYMOUS:
				BER_BVSTR( &authzID, "dn:" );
				break;

			case LDAP_BACK_IDASSERT_SELF:
				if ( BER_BVISNULL( &ndn ) ) {
					/* connection is not authc'd, so don't idassert */
					BER_BVSTR( &authzID, "dn:" );
					break;
				}
				authzID.bv_len = STRLENOF( "dn:" ) + ndn.bv_len;
				authzID.bv_val = slap_sl_malloc( authzID.bv_len + 1, op->o_tmpmemctx );
				AC_MEMCPY( authzID.bv_val, "dn:", STRLENOF( "dn:" ) );
				AC_MEMCPY( authzID.bv_val + STRLENOF( "dn:" ),
						ndn.bv_val, ndn.bv_len + 1 );
				freeauthz = 1;
				break;

			default:
				break;
			}
		}

		if ( mt->mt_idassert_secprops != NULL ) {
			rs->sr_err = ldap_set_option( msc->msc_ld,
				LDAP_OPT_X_SASL_SECPROPS,
				(void *)mt->mt_idassert_secprops );

			if ( rs->sr_err != LDAP_OPT_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
				goto done;
			}
		}

		defaults = lutil_sasl_defaults( msc->msc_ld,
				mt->mt_idassert_sasl_mech.bv_val,
				mt->mt_idassert_sasl_realm.bv_val,
				mt->mt_idassert_authcID.bv_val,
				mt->mt_idassert_passwd.bv_val,
				authzID.bv_val );
		if ( defaults == NULL ) {
			rs->sr_err = LDAP_OTHER;
			LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			goto done;
		}

		rs->sr_err = ldap_sasl_interactive_bind_s( msc->msc_ld, binddn->bv_val,
				mt->mt_idassert_sasl_mech.bv_val, NULL, NULL,
				LDAP_SASL_QUIET, lutil_sasl_interact,
				defaults );

		rs->sr_err = slap_map_api2result( rs );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}

		} else {
			LDAP_BACK_CONN_ISBOUND_SET( msc );
		}

		lutil_sasl_freedefs( defaults );
		if ( freeauthz ) {
			slap_sl_free( authzID.bv_val, op->o_tmpmemctx );
		}

		goto done;
#endif /* HAVE_CYRUS_SASL */
	}

	*method = mt->mt_idassert_authmethod;
	switch ( mt->mt_idassert_authmethod ) {
	case LDAP_AUTH_NONE:
		BER_BVSTR( binddn, "" );
		BER_BVSTR( bindcred, "" );
		/* fallthru */

	case LDAP_AUTH_SIMPLE:
		break;

	default:
		/* unsupported! */
		LDAP_BACK_CONN_ISBOUND_CLEAR( msc );
		rs->sr_err = LDAP_AUTH_METHOD_NOT_SUPPORTED;
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
		break;
	}

done:;

	if ( !BER_BVISEMPTY( binddn ) ) {
		LDAP_BACK_CONN_ISIDASSERT_SET( msc );
	}

	return rs->sr_err;
}

static int
meta_back_proxy_authz_bind(
	metaconn_t *mc,
	int candidate,
	Operation *op,
	SlapReply *rs,
	ldap_back_send_t sendok,
	int dolock )
{
	metainfo_t		*mi = (metainfo_t *)op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];
	struct berval		binddn = BER_BVC( "" ),
				cred = BER_BVC( "" );
	int			method = LDAP_AUTH_NONE,
				rc;

	rc = meta_back_proxy_authz_cred( mc, candidate, op, rs, sendok, &binddn, &cred, &method );
	if ( rc == LDAP_SUCCESS && !LDAP_BACK_CONN_ISBOUND( msc ) ) {
		int	msgid;

		switch ( method ) {
		case LDAP_AUTH_NONE:
		case LDAP_AUTH_SIMPLE:

			if(!dolock) {
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			}

			for (;;) {
				rs->sr_err = ldap_sasl_bind( msc->msc_ld,
					binddn.bv_val, LDAP_SASL_SIMPLE,
					&cred, NULL, NULL, &msgid );
				if ( rs->sr_err != LDAP_X_CONNECTING ) {
					break;
				}
				ldap_pvt_thread_yield();
			}

			if(!dolock) {
				ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
			}

			rc = meta_back_bind_op_result( op, rs, mc, candidate, msgid, sendok, dolock );
			if ( rc == LDAP_SUCCESS ) {
				/* set rebind stuff in case of successful proxyAuthz bind,
				 * so that referral chasing is attempted using the right
				 * identity */
				LDAP_BACK_CONN_ISBOUND_SET( msc );
				ber_bvreplace( &msc->msc_bound_ndn, &binddn );

				if ( META_BACK_TGT_SAVECRED( mt ) ) {
					if ( !BER_BVISNULL( &msc->msc_cred ) ) {
						memset( msc->msc_cred.bv_val, 0,
							msc->msc_cred.bv_len );
					}
					ber_bvreplace( &msc->msc_cred, &cred );
					ldap_set_rebind_proc( msc->msc_ld, mt->mt_rebind_f, msc );
				}
			}
			break;

		default:
			assert( 0 );
			break;
		}
	}

	return LDAP_BACK_CONN_ISBOUND( msc );
}

/*
 * Add controls;
 *
 * if any needs to be added, it is prepended to existing ones,
 * in a newly allocated array.  The companion function
 * mi->mi_ldap_extra->controls_free() must be used to restore the original
 * status of op->o_ctrls.
 */
int
meta_back_controls_add(
		Operation	*op,
		SlapReply	*rs,
		metaconn_t	*mc,
		int		candidate,
		LDAPControl	***pctrls )
{
	metainfo_t		*mi = (metainfo_t *)op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	LDAPControl		**ctrls = NULL;
	/* set to the maximum number of controls this backend can add */
	LDAPControl		c[ 2 ] = {{ 0 }};
	int			n = 0, i, j1 = 0, j2 = 0;

	*pctrls = NULL;

	rs->sr_err = LDAP_SUCCESS;

	/* don't add controls if protocol is not LDAPv3 */
	switch ( mt->mt_version ) {
	case LDAP_VERSION3:
		break;

	case 0:
		if ( op->o_protocol == 0 || op->o_protocol == LDAP_VERSION3 ) {
			break;
		}
		/* fall thru */

	default:
		goto done;
	}

	/* put controls that go __before__ existing ones here */

	/* proxyAuthz for identity assertion */
	switch ( mi->mi_ldap_extra->proxy_authz_ctrl( op, rs, &msc->msc_bound_ndn,
		mt->mt_version, &mt->mt_idassert, &c[ j1 ] ) )
	{
	case SLAP_CB_CONTINUE:
		break;

	case LDAP_SUCCESS:
		j1++;
		break;

	default:
		goto done;
	}

	/* put controls that go __after__ existing ones here */

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	/* session tracking */
	if ( META_BACK_TGT_ST_REQUEST( mt ) ) {
		switch ( slap_ctrl_session_tracking_request_add( op, rs, &c[ j1 + j2 ] ) ) {
		case SLAP_CB_CONTINUE:
			break;

		case LDAP_SUCCESS:
			j2++;
			break;

		default:
			goto done;
		}
	}
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

	if ( rs->sr_err == SLAP_CB_CONTINUE ) {
		rs->sr_err = LDAP_SUCCESS;
	}

	/* if nothing to do, just bail out */
	if ( j1 == 0 && j2 == 0 ) {
		goto done;
	}

	assert( j1 + j2 <= (int) (sizeof( c )/sizeof( c[0] )) );

	if ( op->o_ctrls ) {
		for ( n = 0; op->o_ctrls[ n ]; n++ )
			/* just count ctrls */ ;
	}

	ctrls = op->o_tmpalloc( (n + j1 + j2 + 1) * sizeof( LDAPControl * ) + ( j1 + j2 ) * sizeof( LDAPControl ),
			op->o_tmpmemctx );
	if ( j1 ) {
		ctrls[ 0 ] = (LDAPControl *)&ctrls[ n + j1 + j2 + 1 ];
		*ctrls[ 0 ] = c[ 0 ];
		for ( i = 1; i < j1; i++ ) {
			ctrls[ i ] = &ctrls[ 0 ][ i ];
			*ctrls[ i ] = c[ i ];
		}
	}

	i = 0;
	if ( op->o_ctrls ) {
		for ( i = 0; op->o_ctrls[ i ]; i++ ) {
			ctrls[ i + j1 ] = op->o_ctrls[ i ];
		}
	}

	n += j1;
	if ( j2 ) {
		ctrls[ n ] = (LDAPControl *)&ctrls[ n + j2 + 1 ] + j1;
		*ctrls[ n ] = c[ j1 ];
		for ( i = 1; i < j2; i++ ) {
			ctrls[ n + i ] = &ctrls[ n ][ i ];
			*ctrls[ n + i ] = c[ i ];
		}
	}

	ctrls[ n + j2 ] = NULL;

done:;
	if ( ctrls == NULL ) {
		ctrls = op->o_ctrls;
	}

	*pctrls = ctrls;
	
	return rs->sr_err;
}

