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

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "lutil.h"
#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"
#include "../../../libraries/liblber/lber-int.h"

/* IGNORE means that target does not (no longer) participate
 * in the search;
 * NOTREADY means the search on that target has not been initialized yet
 */
#define	META_MSGID_IGNORE	(-1)
#define	META_MSGID_NEED_BIND	(-2)
#define	META_MSGID_CONNECTING	(-3)

static int
meta_send_entry(
	Operation 	*op,
	SlapReply	*rs,
	metaconn_t	*mc,
	int 		i,
	LDAPMessage 	*e );

typedef enum meta_search_candidate_t {
	META_SEARCH_UNDEFINED = -2,
	META_SEARCH_ERR = -1,
	META_SEARCH_NOT_CANDIDATE,
	META_SEARCH_CANDIDATE,
	META_SEARCH_BINDING,
	META_SEARCH_NEED_BIND,
	META_SEARCH_CONNECTING
} meta_search_candidate_t;

/*
 * meta_search_dobind_init()
 *
 * initiates bind for a candidate target of a search.
 */
static meta_search_candidate_t
meta_search_dobind_init(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		**mcp,
	int			candidate,
	SlapReply		*candidates )
{
	metaconn_t		*mc = *mcp;
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	struct berval		binddn = msc->msc_bound_ndn,
				cred = msc->msc_cred;
	int			method;

	int			rc;

	meta_search_candidate_t	retcode;

	Debug( LDAP_DEBUG_TRACE, "%s >>> meta_search_dobind_init[%d]\n",
		op->o_log_prefix, candidate, 0 );

	/*
	 * all the targets are already bound as pseudoroot
	 */
	if ( mc->mc_authz_target == META_BOUND_ALL ) {
		return META_SEARCH_CANDIDATE;
	}

	retcode = META_SEARCH_BINDING;
	ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
	if ( LDAP_BACK_CONN_ISBOUND( msc ) || LDAP_BACK_CONN_ISANON( msc ) ) {
		/* already bound (or anonymous) */

#ifdef DEBUG_205
		char	buf[ SLAP_TEXT_BUFLEN ] = { '\0' };
		int	bound = 0;

		if ( LDAP_BACK_CONN_ISBOUND( msc ) ) {
			bound = 1;
		}

		snprintf( buf, sizeof( buf ), " mc=%p ld=%p%s DN=\"%s\"",
			(void *)mc, (void *)msc->msc_ld,
			bound ? " bound" : " anonymous",
			bound == 0 ? "" : msc->msc_bound_ndn.bv_val );
		Debug( LDAP_DEBUG_ANY, "### %s meta_search_dobind_init[%d]%s\n",
			op->o_log_prefix, candidate, buf );
#endif /* DEBUG_205 */

		retcode = META_SEARCH_CANDIDATE;

	} else if ( META_BACK_CONN_CREATING( msc ) || LDAP_BACK_CONN_BINDING( msc ) ) {
		/* another thread is binding the target for this conn; wait */

#ifdef DEBUG_205
		char	buf[ SLAP_TEXT_BUFLEN ] = { '\0' };

		snprintf( buf, sizeof( buf ), " mc=%p ld=%p needbind",
			(void *)mc, (void *)msc->msc_ld );
		Debug( LDAP_DEBUG_ANY, "### %s meta_search_dobind_init[%d]%s\n",
			op->o_log_prefix, candidate, buf );
#endif /* DEBUG_205 */

		candidates[ candidate ].sr_msgid = META_MSGID_NEED_BIND;
		retcode = META_SEARCH_NEED_BIND;

	} else {
		/* we'll need to bind the target for this conn */

#ifdef DEBUG_205
		char buf[ SLAP_TEXT_BUFLEN ];

		snprintf( buf, sizeof( buf ), " mc=%p ld=%p binding",
			(void *)mc, (void *)msc->msc_ld );
		Debug( LDAP_DEBUG_ANY, "### %s meta_search_dobind_init[%d]%s\n",
			op->o_log_prefix, candidate, buf );
#endif /* DEBUG_205 */

		if ( msc->msc_ld == NULL ) {
			/* for some reason (e.g. because formerly in "binding"
			 * state, with eventual connection expiration or invalidation)
			 * it was not initialized as expected */

			Debug( LDAP_DEBUG_ANY, "%s meta_search_dobind_init[%d] mc=%p ld=NULL\n",
				op->o_log_prefix, candidate, (void *)mc );

			rc = meta_back_init_one_conn( op, rs, *mcp, candidate,
				LDAP_BACK_CONN_ISPRIV( *mcp ), LDAP_BACK_DONTSEND, 0 );
			switch ( rc ) {
			case LDAP_SUCCESS:
				assert( msc->msc_ld != NULL );
				break;

			case LDAP_SERVER_DOWN:
			case LDAP_UNAVAILABLE:
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
				goto down;
	
			default:
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
				goto other;
			}
		}

		LDAP_BACK_CONN_BINDING_SET( msc );
	}

	ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

	if ( retcode != META_SEARCH_BINDING ) {
		return retcode;
	}

	/* NOTE: this obsoletes pseudorootdn */
	if ( op->o_conn != NULL &&
		!op->o_do_not_cache &&
		( BER_BVISNULL( &msc->msc_bound_ndn ) ||
			BER_BVISEMPTY( &msc->msc_bound_ndn ) ||
			( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) ) )
	{
		rc = meta_back_proxy_authz_cred( mc, candidate, op, rs, LDAP_BACK_DONTSEND, &binddn, &cred, &method );
		switch ( rc ) {
		case LDAP_SUCCESS:
			break;
		case LDAP_UNAVAILABLE:
			goto down;
		default:
			goto other;
		}

		/* NOTE: we copy things here, even if bind didn't succeed yet,
		 * because the connection is not shared until bind is over */
		if ( !BER_BVISNULL( &binddn ) ) {
			ber_bvreplace( &msc->msc_bound_ndn, &binddn );
			if ( META_BACK_TGT_SAVECRED( mt ) && !BER_BVISNULL( &cred ) ) {
				if ( !BER_BVISNULL( &msc->msc_cred ) ) {
					memset( msc->msc_cred.bv_val, 0,
						msc->msc_cred.bv_len );
				}
				ber_bvreplace( &msc->msc_cred, &cred );
			}
		}

		if ( LDAP_BACK_CONN_ISBOUND( msc ) ) {
			/* apparently, idassert was configured with SASL bind,
			 * so bind occurred inside meta_back_proxy_authz_cred() */
			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
			LDAP_BACK_CONN_BINDING_CLEAR( msc );
			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
			return META_SEARCH_CANDIDATE;
		}

		/* paranoid */
		switch ( method ) {
		case LDAP_AUTH_NONE:
		case LDAP_AUTH_SIMPLE:
			/* do a simple bind with binddn, cred */
			break;

		default:
			assert( 0 );
			break;
		}
	}

	assert( msc->msc_ld != NULL );

	/* connect must be async only the first time... */
	ldap_set_option( msc->msc_ld, LDAP_OPT_CONNECT_ASYNC, LDAP_OPT_ON );

retry:;
	if ( !BER_BVISEMPTY( &binddn ) && BER_BVISEMPTY( &cred ) ) {
		/* bind anonymously? */
		Debug( LDAP_DEBUG_ANY, "%s meta_search_dobind_init[%d] mc=%p: "
			"non-empty dn with empty cred; binding anonymously\n",
			op->o_log_prefix, candidate, (void *)mc );
		cred = slap_empty_bv;
		
	} else if ( BER_BVISEMPTY( &binddn ) && !BER_BVISEMPTY( &cred ) ) {
		/* error */
		Debug( LDAP_DEBUG_ANY, "%s meta_search_dobind_init[%d] mc=%p: "
			"empty dn with non-empty cred: error\n",
			op->o_log_prefix, candidate, (void *)mc );
		goto other;
	}

	rc = ldap_sasl_bind( msc->msc_ld, binddn.bv_val, LDAP_SASL_SIMPLE, &cred,
			NULL, NULL, &candidates[ candidate ].sr_msgid );

#ifdef DEBUG_205
	{
		char buf[ SLAP_TEXT_BUFLEN ];

		snprintf( buf, sizeof( buf ), "meta_search_dobind_init[%d] mc=%p ld=%p rc=%d",
			candidate, (void *)mc, (void *)mc->mc_conns[ candidate ].msc_ld, rc );
		Debug( LDAP_DEBUG_ANY, "### %s %s\n",
			op->o_log_prefix, buf, 0 );
	}
#endif /* DEBUG_205 */

	switch ( rc ) {
	case LDAP_SUCCESS:
		assert( candidates[ candidate ].sr_msgid >= 0 );
		META_BINDING_SET( &candidates[ candidate ] );
		return META_SEARCH_BINDING;

	case LDAP_X_CONNECTING:
		/* must retry, same conn */
		candidates[ candidate ].sr_msgid = META_MSGID_CONNECTING;
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		LDAP_BACK_CONN_BINDING_CLEAR( msc );
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		return META_SEARCH_CONNECTING;

	case LDAP_SERVER_DOWN:
down:;
		/* This is the worst thing that could happen:
		 * the search will wait until the retry is over. */
		if ( !META_IS_RETRYING( &candidates[ candidate ] ) ) {
			META_RETRYING_SET( &candidates[ candidate ] );

			ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );

			assert( mc->mc_refcnt > 0 );
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
					"%s meta_search_dobind_init[%d]: %s.\n",
					op->o_log_prefix, candidate, buf );
			}

			meta_clear_one_candidate( op, mc, candidate );
			LDAP_BACK_CONN_ISBOUND_CLEAR( msc );

			( void )rewrite_session_delete( mt->mt_rwmap.rwm_rw, op->o_conn );

			/* mc here must be the regular mc, reset and ready for init */
			rc = meta_back_init_one_conn( op, rs, mc, candidate,
				LDAP_BACK_CONN_ISPRIV( mc ), LDAP_BACK_DONTSEND, 0 );

			if ( rc == LDAP_SUCCESS ) {
				LDAP_BACK_CONN_BINDING_SET( msc );
			}

			ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

			if ( rc == LDAP_SUCCESS ) {
				candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
				binddn = msc->msc_bound_ndn;
				cred = msc->msc_cred;
				goto retry;
			}
		}

		if ( *mcp == NULL ) {
			retcode = META_SEARCH_ERR;
			rs->sr_err = LDAP_UNAVAILABLE;
			candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
			break;
		}
		/* fall thru */

	default:
other:;
		rs->sr_err = rc;
		rc = slap_map_api2result( rs );

		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		meta_clear_one_candidate( op, mc, candidate );
		candidates[ candidate ].sr_err = rc;
		if ( META_BACK_ONERR_STOP( mi ) ) {
			LDAP_BACK_CONN_TAINTED_SET( mc );
			meta_back_release_conn_lock( mi, mc, 0 );
			*mcp = NULL;
			rs->sr_err = rc;

			retcode = META_SEARCH_ERR;

		} else {
			retcode = META_SEARCH_NOT_CANDIDATE;
		}
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		break;
	}

	return retcode;
}

static meta_search_candidate_t
meta_search_dobind_result(
	Operation		*op,
	SlapReply		*rs,
	metaconn_t		**mcp,
	int			candidate,
	SlapReply		*candidates,
	LDAPMessage		*res )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metaconn_t		*mc = *mcp;
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	meta_search_candidate_t	retcode = META_SEARCH_NOT_CANDIDATE;
	int			rc;

	assert( msc->msc_ld != NULL );

	/* FIXME: matched? referrals? response controls? */
	rc = ldap_parse_result( msc->msc_ld, res,
		&candidates[ candidate ].sr_err,
		NULL, NULL, NULL, NULL, 0 );
	if ( rc != LDAP_SUCCESS ) {
		candidates[ candidate ].sr_err = rc;
	}
	rc = slap_map_api2result( &candidates[ candidate ] );

	ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
	LDAP_BACK_CONN_BINDING_CLEAR( msc );
	if ( rc != LDAP_SUCCESS ) {
		meta_clear_one_candidate( op, mc, candidate );
		candidates[ candidate ].sr_err = rc;
		if ( META_BACK_ONERR_STOP( mi ) ) {
	        	LDAP_BACK_CONN_TAINTED_SET( mc );
			meta_back_release_conn_lock( mi, mc, 0 );
			*mcp = NULL;
			retcode = META_SEARCH_ERR;
			rs->sr_err = rc;
		}

	} else {
		/* FIXME: check if bound as idassert authcDN! */
		if ( BER_BVISNULL( &msc->msc_bound_ndn )
			|| BER_BVISEMPTY( &msc->msc_bound_ndn ) )
		{
			LDAP_BACK_CONN_ISANON_SET( msc );

		} else {
			if ( META_BACK_TGT_SAVECRED( mt ) &&
				!BER_BVISNULL( &msc->msc_cred ) &&
				!BER_BVISEMPTY( &msc->msc_cred ) )
			{
				ldap_set_rebind_proc( msc->msc_ld, mt->mt_rebind_f, msc );
			}
			LDAP_BACK_CONN_ISBOUND_SET( msc );
		}
		retcode = META_SEARCH_CANDIDATE;

		/* connect must be async */
		ldap_set_option( msc->msc_ld, LDAP_OPT_CONNECT_ASYNC, LDAP_OPT_OFF );
	}

	candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
	META_BINDING_CLEAR( &candidates[ candidate ] );

	ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

	return retcode;
}

static meta_search_candidate_t
meta_back_search_start(
	Operation		*op,
	SlapReply		*rs,
	dncookie		*dc,
	metaconn_t		**mcp,
	int			candidate,
	SlapReply		*candidates,
	struct berval		*prcookie,
	ber_int_t		prsize )
{
	metainfo_t		*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t		*mt = mi->mi_targets[ candidate ];
	metasingleconn_t	*msc = &(*mcp)->mc_conns[ candidate ];
	struct berval		realbase = op->o_req_dn;
	int			realscope = op->ors_scope;
	struct berval		mbase = BER_BVNULL; 
	struct berval		mfilter = BER_BVNULL;
	char			**mapped_attrs = NULL;
	int			rc;
	meta_search_candidate_t	retcode;
	struct timeval		tv, *tvp = NULL;
	int			nretries = 1;
	LDAPControl		**ctrls = NULL;
#ifdef SLAPD_META_CLIENT_PR
	LDAPControl		**save_ctrls = NULL;
#endif /* SLAPD_META_CLIENT_PR */

	/* this should not happen; just in case... */
	if ( msc->msc_ld == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"%s: meta_back_search_start candidate=%d ld=NULL%s.\n",
			op->o_log_prefix, candidate,
			META_BACK_ONERR_STOP( mi ) ? "" : " (ignored)" );
		candidates[ candidate ].sr_err = LDAP_OTHER;
		if ( META_BACK_ONERR_STOP( mi ) ) {
			return META_SEARCH_ERR;
		}
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		return META_SEARCH_NOT_CANDIDATE;
	}

	Debug( LDAP_DEBUG_TRACE, "%s >>> meta_back_search_start[%d]\n", op->o_log_prefix, candidate, 0 );

	/*
	 * modifies the base according to the scope, if required
	 */
	if ( mt->mt_nsuffix.bv_len > op->o_req_ndn.bv_len ) {
		switch ( op->ors_scope ) {
		case LDAP_SCOPE_SUBTREE:
			/*
			 * make the target suffix the new base
			 * FIXME: this is very forgiving, because
			 * "illegal" searchBases may be turned
			 * into the suffix of the target; however,
			 * the requested searchBase already passed
			 * thru the candidate analyzer...
			 */
			if ( dnIsSuffix( &mt->mt_nsuffix, &op->o_req_ndn ) ) {
				realbase = mt->mt_nsuffix;
				if ( mt->mt_scope == LDAP_SCOPE_SUBORDINATE ) {
					realscope = LDAP_SCOPE_SUBORDINATE;
				}

			} else {
				/*
				 * this target is no longer candidate
				 */
				retcode = META_SEARCH_NOT_CANDIDATE;
				goto doreturn;
			}
			break;

		case LDAP_SCOPE_SUBORDINATE:
		case LDAP_SCOPE_ONELEVEL:
		{
			struct berval	rdn = mt->mt_nsuffix;
			rdn.bv_len -= op->o_req_ndn.bv_len + STRLENOF( "," );
			if ( dnIsOneLevelRDN( &rdn )
					&& dnIsSuffix( &mt->mt_nsuffix, &op->o_req_ndn ) )
			{
				/*
				 * if there is exactly one level,
				 * make the target suffix the new
				 * base, and make scope "base"
				 */
				realbase = mt->mt_nsuffix;
				if ( op->ors_scope == LDAP_SCOPE_SUBORDINATE ) {
					if ( mt->mt_scope == LDAP_SCOPE_SUBORDINATE ) {
						realscope = LDAP_SCOPE_SUBORDINATE;
					} else {
						realscope = LDAP_SCOPE_SUBTREE;
					}
				} else {
					realscope = LDAP_SCOPE_BASE;
				}
				break;
			} /* else continue with the next case */
		}

		case LDAP_SCOPE_BASE:
			/*
			 * this target is no longer candidate
			 */
			retcode = META_SEARCH_NOT_CANDIDATE;
			goto doreturn;
		}
	}

	/* check filter expression */
	if ( mt->mt_filter ) {
		metafilter_t *mf;
		for ( mf = mt->mt_filter; mf; mf = mf->mf_next ) {
			if ( regexec( &mf->mf_regex, op->ors_filterstr.bv_val, 0, NULL, 0 ) == 0 )
				break;
		}
		/* nothing matched, this target is no longer a candidate */
		if ( !mf ) {
			retcode = META_SEARCH_NOT_CANDIDATE;
			goto doreturn;
		}
	}

	/* initiate dobind */
	retcode = meta_search_dobind_init( op, rs, mcp, candidate, candidates );

	Debug( LDAP_DEBUG_TRACE, "%s <<< meta_search_dobind_init[%d]=%d\n", op->o_log_prefix, candidate, retcode );

	if ( retcode != META_SEARCH_CANDIDATE ) {
		goto doreturn;
	}

	/*
	 * Rewrite the search base, if required
	 */
	dc->target = mt;
	dc->ctx = "searchBase";
	switch ( ldap_back_dn_massage( dc, &realbase, &mbase ) ) {
	case LDAP_SUCCESS:
		break;

	case LDAP_UNWILLING_TO_PERFORM:
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "Operation not allowed";
		send_ldap_result( op, rs );
		retcode = META_SEARCH_ERR;
		goto doreturn;

	default:

		/*
		 * this target is no longer candidate
		 */
		retcode = META_SEARCH_NOT_CANDIDATE;
		goto doreturn;
	}

	/*
	 * Maps filter
	 */
	rc = ldap_back_filter_map_rewrite( dc, op->ors_filter,
			&mfilter, BACKLDAP_MAP, op->o_tmpmemctx );
	switch ( rc ) {
	case LDAP_SUCCESS:
		break;

	case LDAP_COMPARE_FALSE:
	default:
		/*
		 * this target is no longer candidate
		 */
		retcode = META_SEARCH_NOT_CANDIDATE;
		goto done;
	}

	/*
	 * Maps required attributes
	 */
	rc = ldap_back_map_attrs( op, &mt->mt_rwmap.rwm_at,
			op->ors_attrs, BACKLDAP_MAP, &mapped_attrs );
	if ( rc != LDAP_SUCCESS ) {
		/*
		 * this target is no longer candidate
		 */
		retcode = META_SEARCH_NOT_CANDIDATE;
		goto done;
	}

	if ( op->ors_tlimit != SLAP_NO_LIMIT ) {
		tv.tv_sec = op->ors_tlimit > 0 ? op->ors_tlimit : 1;
		tv.tv_usec = 0;
		tvp = &tv;
	}

#ifdef SLAPD_META_CLIENT_PR
	save_ctrls = op->o_ctrls;
	{
		LDAPControl *pr_c = NULL;
		int i = 0, nc = 0;

		if ( save_ctrls ) {
			for ( ; save_ctrls[i] != NULL; i++ );
			nc = i;
			pr_c = ldap_control_find( LDAP_CONTROL_PAGEDRESULTS, save_ctrls, NULL );
		}

		if ( pr_c != NULL ) nc--;
		if ( mt->mt_ps > 0 || prcookie != NULL ) nc++;

		if ( mt->mt_ps > 0 || prcookie != NULL || pr_c != NULL ) {
			int src = 0, dst = 0;
			BerElementBuffer berbuf;
			BerElement *ber = (BerElement *)&berbuf;
			struct berval val = BER_BVNULL;
			ber_len_t len;

			len = sizeof( LDAPControl * )*( nc + 1 ) + sizeof( LDAPControl );

			if ( mt->mt_ps > 0 || prcookie != NULL ) {
				struct berval nullcookie = BER_BVNULL;
				ber_tag_t tag;

				if ( prsize == 0 && mt->mt_ps > 0 ) prsize = mt->mt_ps;
				if ( prcookie == NULL ) prcookie = &nullcookie;

				ber_init2( ber, NULL, LBER_USE_DER );
				tag = ber_printf( ber, "{iO}", prsize, prcookie ); 
				if ( tag == LBER_ERROR ) {
					/* error */
					(void) ber_free_buf( ber );
					goto done_pr;
				}

				tag = ber_flatten2( ber, &val, 0 );
				if ( tag == LBER_ERROR ) {
					/* error */
					(void) ber_free_buf( ber );
					goto done_pr;
				}

				len += val.bv_len + 1;
			}

			op->o_ctrls = op->o_tmpalloc( len, op->o_tmpmemctx );
			if ( save_ctrls ) {
				for ( ; save_ctrls[ src ] != NULL; src++ ) {
					if ( save_ctrls[ src ] != pr_c ) {
						op->o_ctrls[ dst ] = save_ctrls[ src ];
						dst++;
					}
				}
			}

			if ( mt->mt_ps > 0 || prcookie != NULL ) {
				op->o_ctrls[ dst ] = (LDAPControl *)&op->o_ctrls[ nc + 1 ];

				op->o_ctrls[ dst ]->ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
				op->o_ctrls[ dst ]->ldctl_iscritical = 1;

				op->o_ctrls[ dst ]->ldctl_value.bv_val = (char *)&op->o_ctrls[ dst ][ 1 ];
				AC_MEMCPY( op->o_ctrls[ dst ]->ldctl_value.bv_val, val.bv_val, val.bv_len + 1 );
				op->o_ctrls[ dst ]->ldctl_value.bv_len = val.bv_len;
				dst++;

				(void)ber_free_buf( ber );
			}

			op->o_ctrls[ dst ] = NULL;
		}
done_pr:;
	}
#endif /* SLAPD_META_CLIENT_PR */

retry:;
	ctrls = op->o_ctrls;
	if ( meta_back_controls_add( op, rs, *mcp, candidate, &ctrls )
		!= LDAP_SUCCESS )
	{
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		retcode = META_SEARCH_NOT_CANDIDATE;
		goto done;
	}

	/*
	 * Starts the search
	 */
	assert( msc->msc_ld != NULL );
	rc = ldap_pvt_search( msc->msc_ld,
			mbase.bv_val, realscope, mfilter.bv_val,
			mapped_attrs, op->ors_attrsonly,
			ctrls, NULL, tvp, op->ors_slimit, op->ors_deref,
			&candidates[ candidate ].sr_msgid ); 
	switch ( rc ) {
	case LDAP_SUCCESS:
		retcode = META_SEARCH_CANDIDATE;
		break;
	
	case LDAP_SERVER_DOWN:
		if ( nretries && meta_back_retry( op, rs, mcp, candidate, LDAP_BACK_DONTSEND ) ) {
			nretries = 0;
			/* if the identity changed, there might be need to re-authz */
			(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );
			goto retry;
		}

		if ( *mcp == NULL ) {
			retcode = META_SEARCH_ERR;
			candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
			break;
		}
		/* fall thru */

	default:
		candidates[ candidate ].sr_msgid = META_MSGID_IGNORE;
		retcode = META_SEARCH_NOT_CANDIDATE;
	}

done:;
	(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );
#ifdef SLAPD_META_CLIENT_PR
	if ( save_ctrls != op->o_ctrls ) {
		op->o_tmpfree( op->o_ctrls, op->o_tmpmemctx );
		op->o_ctrls = save_ctrls;
	}
#endif /* SLAPD_META_CLIENT_PR */

	if ( mapped_attrs ) {
		ber_memfree_x( mapped_attrs, op->o_tmpmemctx );
	}
	if ( mfilter.bv_val != op->ors_filterstr.bv_val ) {
		ber_memfree_x( mfilter.bv_val, op->o_tmpmemctx );
	}
	if ( mbase.bv_val != realbase.bv_val ) {
		free( mbase.bv_val );
	}

doreturn:;
	Debug( LDAP_DEBUG_TRACE, "%s <<< meta_back_search_start[%d]=%d\n", op->o_log_prefix, candidate, retcode );

	return retcode;
}

int
meta_back_search( Operation *op, SlapReply *rs )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metaconn_t	*mc;
	struct timeval	save_tv = { 0, 0 },
			tv;
	time_t		stoptime = (time_t)(-1),
			lastres_time = slap_get_time(),
			timeout = 0;
	int		rc = 0, sres = LDAP_SUCCESS;
	char		*matched = NULL;
	int		last = 0, ncandidates = 0,
			initial_candidates = 0, candidate_match = 0,
			needbind = 0;
	ldap_back_send_t	sendok = LDAP_BACK_SENDERR;
	long		i;
	dncookie	dc;
	int		is_ok = 0;
	void		*savepriv;
	SlapReply	*candidates = NULL;
	int		do_taint = 0;

	rs_assert_ready( rs );
	rs->sr_flags &= ~REP_ENTRY_MASK; /* paranoia, we can set rs = non-entry */

	/*
	 * controls are set in ldap_back_dobind()
	 * 
	 * FIXME: in case of values return filter, we might want
	 * to map attrs and maybe rewrite value
	 */
getconn:;
	mc = meta_back_getconn( op, rs, NULL, sendok );
	if ( !mc ) {
		return rs->sr_err;
	}

	dc.conn = op->o_conn;
	dc.rs = rs;

	if ( candidates == NULL ) candidates = meta_back_candidates_get( op );
	/*
	 * Inits searches
	 */
	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		/* reset sr_msgid; it is used in most loops
		 * to check if that target is still to be considered */
		candidates[ i ].sr_msgid = META_MSGID_IGNORE;

		/* a target is marked as candidate by meta_back_getconn();
		 * if for any reason (an error, it's over or so) it is
		 * no longer active, sr_msgid is set to META_MSGID_IGNORE
		 * but it remains candidate, which means it has been active
		 * at some point during the operation.  This allows to 
		 * use its response code and more to compute the final
		 * response */
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		candidates[ i ].sr_matched = NULL;
		candidates[ i ].sr_text = NULL;
		candidates[ i ].sr_ref = NULL;
		candidates[ i ].sr_ctrls = NULL;
		candidates[ i ].sr_nentries = 0;

		/* get largest timeout among candidates */
		if ( mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ]
			&& mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ] > timeout )
		{
			timeout = mi->mi_targets[ i ]->mt_timeout[ SLAP_OP_SEARCH ];
		}
	}

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		if ( !META_IS_CANDIDATE( &candidates[ i ] )
			|| candidates[ i ].sr_err != LDAP_SUCCESS )
		{
			continue;
		}

		switch ( meta_back_search_start( op, rs, &dc, &mc, i, candidates, NULL, 0 ) )
		{
		case META_SEARCH_NOT_CANDIDATE:
			candidates[ i ].sr_msgid = META_MSGID_IGNORE;
			break;

		case META_SEARCH_NEED_BIND:
			++needbind;
			/* fallthru */

		case META_SEARCH_CONNECTING:
		case META_SEARCH_CANDIDATE:
		case META_SEARCH_BINDING:
			candidates[ i ].sr_type = REP_INTERMEDIATE;
			++ncandidates;
			break;

		case META_SEARCH_ERR:
			savepriv = op->o_private;
			op->o_private = (void *)i;
			send_ldap_result( op, rs );
			op->o_private = savepriv;
			rc = -1;
			goto finish;

		default:
			assert( 0 );
			break;
		}
	}

	if ( ncandidates > 0 && needbind == ncandidates ) {
		/*
		 * give up the second time...
		 *
		 * NOTE: this should not occur the second time, since a fresh
		 * connection has ben created; however, targets may also
		 * need bind because the bind timed out or so.
		 */
		if ( sendok & LDAP_BACK_BINDING ) {
			Debug( LDAP_DEBUG_ANY,
				"%s meta_back_search: unable to initialize conn\n",
				op->o_log_prefix, 0, 0 );
			rs->sr_err = LDAP_UNAVAILABLE;
			rs->sr_text = "unable to initialize connection to remote targets";
			send_ldap_result( op, rs );
			rc = -1;
			goto finish;
		}

		/* FIXME: better create a separate connection? */
		sendok |= LDAP_BACK_BINDING;

#ifdef DEBUG_205
		Debug( LDAP_DEBUG_ANY, "*** %s drop mc=%p create new connection\n",
			op->o_log_prefix, (void *)mc, 0 );
#endif /* DEBUG_205 */

		meta_back_release_conn( mi, mc );
		mc = NULL;

		needbind = 0;
		ncandidates = 0;

		goto getconn;
	}

	initial_candidates = ncandidates;

	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		char	cnd[ SLAP_TEXT_BUFLEN ];
		int	c;

		for ( c = 0; c < mi->mi_ntargets; c++ ) {
			if ( META_IS_CANDIDATE( &candidates[ c ] ) ) {
				cnd[ c ] = '*';
			} else {
				cnd[ c ] = ' ';
			}
		}
		cnd[ c ] = '\0';

		Debug( LDAP_DEBUG_TRACE, "%s meta_back_search: ncandidates=%d "
			"cnd=\"%s\"\n", op->o_log_prefix, ncandidates, cnd );
	}

	if ( initial_candidates == 0 ) {
		/* NOTE: here we are not sending any matchedDN;
		 * this is intended, because if the back-meta
		 * is serving this search request, but no valid
		 * candidate could be looked up, it means that
		 * there is a hole in the mapping of the targets
		 * and thus no knowledge of any remote superior
		 * is available */
		Debug( LDAP_DEBUG_ANY, "%s meta_back_search: "
			"base=\"%s\" scope=%d: "
			"no candidate could be selected\n",
			op->o_log_prefix, op->o_req_dn.bv_val,
			op->ors_scope );

		/* FIXME: we're sending the first error we encounter;
		 * maybe we should pick the worst... */
		rc = LDAP_NO_SUCH_OBJECT;
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			if ( META_IS_CANDIDATE( &candidates[ i ] )
				&& candidates[ i ].sr_err != LDAP_SUCCESS )
			{
				rc = candidates[ i ].sr_err;
				break;
			}
		}

		send_ldap_error( op, rs, rc, NULL );

		goto finish;
	}

	/* We pull apart the ber result, stuff it into a slapd entry, and
	 * let send_search_entry stuff it back into ber format. Slow & ugly,
	 * but this is necessary for version matching, and for ACL processing.
	 */

	if ( op->ors_tlimit != SLAP_NO_LIMIT ) {
		stoptime = op->o_time + op->ors_tlimit;
	}

	/*
	 * In case there are no candidates, no cycle takes place...
	 *
	 * FIXME: we might use a queue, to better balance the load 
	 * among the candidates
	 */
	for ( rc = 0; ncandidates > 0; ) {
		int	gotit = 0,
			doabandon = 0,
			alreadybound = ncandidates;

		/* check timeout */
		if ( timeout && lastres_time > 0
			&& ( slap_get_time() - lastres_time ) > timeout )
		{
			doabandon = 1;
			rs->sr_text = "Operation timed out";
			rc = rs->sr_err = op->o_protocol >= LDAP_VERSION3 ?
				LDAP_ADMINLIMIT_EXCEEDED : LDAP_OTHER;
			savepriv = op->o_private;
			op->o_private = (void *)i;
			send_ldap_result( op, rs );
			op->o_private = savepriv;
			goto finish;
		}

		/* check time limit */
		if ( op->ors_tlimit != SLAP_NO_LIMIT
				&& slap_get_time() > stoptime )
		{
			doabandon = 1;
			rc = rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
			savepriv = op->o_private;
			op->o_private = (void *)i;
			send_ldap_result( op, rs );
			op->o_private = savepriv;
			goto finish;
		}

		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			meta_search_candidate_t	retcode = META_SEARCH_UNDEFINED;
			metasingleconn_t	*msc = &mc->mc_conns[ i ];
			LDAPMessage		*res = NULL, *msg;

			/* if msgid is invalid, don't ldap_result() */
			if ( candidates[ i ].sr_msgid == META_MSGID_IGNORE ) {
				continue;
			}

			/* if target still needs bind, retry */
			if ( candidates[ i ].sr_msgid == META_MSGID_NEED_BIND
				|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
			{
				/* initiate dobind */
				retcode = meta_search_dobind_init( op, rs, &mc, i, candidates );

				Debug( LDAP_DEBUG_TRACE, "%s <<< meta_search_dobind_init[%ld]=%d\n",
					op->o_log_prefix, i, retcode );

				switch ( retcode ) {
				case META_SEARCH_NEED_BIND:
					alreadybound--;
					/* fallthru */

				case META_SEARCH_CONNECTING:
				case META_SEARCH_BINDING:
					break;

				case META_SEARCH_ERR:
					candidates[ i ].sr_err = rs->sr_err;
					if ( META_BACK_ONERR_STOP( mi ) ) {
						savepriv = op->o_private;
						op->o_private = (void *)i;
						send_ldap_result( op, rs );
						op->o_private = savepriv;
						goto finish;
					}
					/* fallthru */

				case META_SEARCH_NOT_CANDIDATE:
					/*
					 * When no candidates are left,
					 * the outer cycle finishes
					 */
					candidates[ i ].sr_msgid = META_MSGID_IGNORE;
					assert( ncandidates > 0 );
					--ncandidates;
					break;

				case META_SEARCH_CANDIDATE:
					candidates[ i ].sr_msgid = META_MSGID_IGNORE;
					switch ( meta_back_search_start( op, rs, &dc, &mc, i, candidates, NULL, 0 ) )
					{
					case META_SEARCH_CANDIDATE:
						assert( candidates[ i ].sr_msgid >= 0 );
						break;

					case META_SEARCH_ERR:
						candidates[ i ].sr_err = rs->sr_err;
						if ( META_BACK_ONERR_STOP( mi ) ) {
							savepriv = op->o_private;
							op->o_private = (void *)i;
							send_ldap_result( op, rs );
							op->o_private = savepriv;
							goto finish;
						}
						/* fallthru */

					case META_SEARCH_NOT_CANDIDATE:
						/* means that meta_back_search_start()
						 * failed but onerr == continue */
						candidates[ i ].sr_msgid = META_MSGID_IGNORE;
						assert( ncandidates > 0 );
						--ncandidates;
						break;

					default:
						/* impossible */
						assert( 0 );
						break;
					}
					break;

				default:
					/* impossible */
					assert( 0 );
					break;
				}
				continue;
			}

			/* check for abandon */
			if ( op->o_abandon || LDAP_BACK_CONN_ABANDON( mc ) ) {
				break;
			}

#ifdef DEBUG_205
			if ( msc->msc_ld == NULL ) {
				char	buf[ SLAP_TEXT_BUFLEN ];

				ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
				snprintf( buf, sizeof( buf ),
					"%s meta_back_search[%ld] mc=%p msgid=%d%s%s%s\n",
					op->o_log_prefix, (long)i, (void *)mc,
					candidates[ i ].sr_msgid,
					META_IS_BINDING( &candidates[ i ] ) ? " binding" : "",
					LDAP_BACK_CONN_BINDING( &mc->mc_conns[ i ] ) ? " connbinding" : "",
					META_BACK_CONN_CREATING( &mc->mc_conns[ i ] ) ? " conncreating" : "" );
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
					
				Debug( LDAP_DEBUG_ANY, "!!! %s\n", buf, 0, 0 );
			}
#endif /* DEBUG_205 */
			
			/*
			 * FIXME: handle time limit as well?
			 * Note that target servers are likely 
			 * to handle it, so at some time we'll
			 * get a LDAP_TIMELIMIT_EXCEEDED from
			 * one of them ...
			 */
			tv = save_tv;
			rc = ldap_result( msc->msc_ld, candidates[ i ].sr_msgid,
					LDAP_MSG_RECEIVED, &tv, &res );
			switch ( rc ) {
			case 0:
				/* FIXME: res should not need to be freed */
				assert( res == NULL );
				continue;

			case -1:
really_bad:;
				/* something REALLY bad happened! */
				if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
					candidates[ i ].sr_type = REP_RESULT;

					if ( meta_back_retry( op, rs, &mc, i, LDAP_BACK_DONTSEND ) ) {
						candidates[ i ].sr_msgid = META_MSGID_IGNORE;
						switch ( meta_back_search_start( op, rs, &dc, &mc, i, candidates, NULL, 0 ) )
						{
							/* means that failed but onerr == continue */
						case META_SEARCH_NOT_CANDIDATE:
							candidates[ i ].sr_msgid = META_MSGID_IGNORE;

							assert( ncandidates > 0 );
							--ncandidates;

							candidates[ i ].sr_err = rs->sr_err;
							if ( META_BACK_ONERR_STOP( mi ) ) {
								savepriv = op->o_private;
								op->o_private = (void *)i;
								send_ldap_result( op, rs );
								op->o_private = savepriv;
								goto finish;
							}
							/* fall thru */

						case META_SEARCH_CANDIDATE:
							/* get back into business... */
							continue;

						case META_SEARCH_BINDING:
						case META_SEARCH_CONNECTING:
						case META_SEARCH_NEED_BIND:
						case META_SEARCH_UNDEFINED:
							assert( 0 );

						default:
							/* unrecoverable error */
							candidates[ i ].sr_msgid = META_MSGID_IGNORE;
							rc = rs->sr_err = LDAP_OTHER;
							goto finish;
						}
					}

					candidates[ i ].sr_err = rs->sr_err;
					if ( META_BACK_ONERR_STOP( mi ) ) {
						savepriv = op->o_private;
						op->o_private = (void *)i;
						send_ldap_result( op, rs );
						op->o_private = savepriv;
						goto finish;
					}
				}

				/*
				 * When no candidates are left,
				 * the outer cycle finishes
				 */
				candidates[ i ].sr_msgid = META_MSGID_IGNORE;
				assert( ncandidates > 0 );
				--ncandidates;
				rs->sr_err = candidates[ i ].sr_err;
				continue;

			default:
				lastres_time = slap_get_time();

				/* only touch when activity actually took place... */
				if ( mi->mi_idle_timeout != 0 && msc->msc_time < lastres_time ) {
					msc->msc_time = lastres_time;
				}
				break;
			}

			for ( msg = ldap_first_message( msc->msc_ld, res );
				msg != NULL;
				msg = ldap_next_message( msc->msc_ld, msg ) )
			{
				rc = ldap_msgtype( msg );
				if ( rc == LDAP_RES_SEARCH_ENTRY ) {
					LDAPMessage	*e;

					if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
						/* don't retry any more... */
						candidates[ i ].sr_type = REP_RESULT;
					}

					/* count entries returned by target */
					candidates[ i ].sr_nentries++;

					is_ok++;

					e = ldap_first_entry( msc->msc_ld, msg );
					savepriv = op->o_private;
					op->o_private = (void *)i;
					rs->sr_err = meta_send_entry( op, rs, mc, i, e );

					switch ( rs->sr_err ) {
					case LDAP_SIZELIMIT_EXCEEDED:
						savepriv = op->o_private;
						op->o_private = (void *)i;
						send_ldap_result( op, rs );
						op->o_private = savepriv;
						rs->sr_err = LDAP_SUCCESS;
						ldap_msgfree( res );
						res = NULL;
						goto finish;

					case LDAP_UNAVAILABLE:
						rs->sr_err = LDAP_OTHER;
						ldap_msgfree( res );
						res = NULL;
						goto finish;
					}
					op->o_private = savepriv;

					/* don't wait any longer... */
					gotit = 1;
					save_tv.tv_sec = 0;
					save_tv.tv_usec = 0;

				} else if ( rc == LDAP_RES_SEARCH_REFERENCE ) {
					char		**references = NULL;
					int		cnt;

					if ( META_BACK_TGT_NOREFS( mi->mi_targets[ i ] ) ) {
						continue;
					}

					if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
						/* don't retry any more... */
						candidates[ i ].sr_type = REP_RESULT;
					}
	
					is_ok++;
	
					rc = ldap_parse_reference( msc->msc_ld, msg,
							&references, &rs->sr_ctrls, 0 );
	
					if ( rc != LDAP_SUCCESS ) {
						continue;
					}
	
					if ( references == NULL ) {
						continue;
					}

#ifdef ENABLE_REWRITE
					dc.ctx = "referralDN";
#else /* ! ENABLE_REWRITE */
					dc.tofrom = 0;
					dc.normalized = 0;
#endif /* ! ENABLE_REWRITE */

					/* FIXME: merge all and return at the end */
	
					for ( cnt = 0; references[ cnt ]; cnt++ )
						;
	
					rs->sr_ref = ber_memalloc_x( sizeof( struct berval ) * ( cnt + 1 ),
						op->o_tmpmemctx );
	
					for ( cnt = 0; references[ cnt ]; cnt++ ) {
						ber_str2bv_x( references[ cnt ], 0, 1, &rs->sr_ref[ cnt ],
						op->o_tmpmemctx );
					}
					BER_BVZERO( &rs->sr_ref[ cnt ] );
	
					( void )ldap_back_referral_result_rewrite( &dc, rs->sr_ref,
						op->o_tmpmemctx );

					if ( rs->sr_ref != NULL && !BER_BVISNULL( &rs->sr_ref[ 0 ] ) ) {
						/* ignore return value by now */
						savepriv = op->o_private;
						op->o_private = (void *)i;
						( void )send_search_reference( op, rs );
						op->o_private = savepriv;
	
						ber_bvarray_free_x( rs->sr_ref, op->o_tmpmemctx );
						rs->sr_ref = NULL;
					}

					/* cleanup */
					if ( references ) {
						ber_memvfree( (void **)references );
					}

					if ( rs->sr_ctrls ) {
						ldap_controls_free( rs->sr_ctrls );
						rs->sr_ctrls = NULL;
					}

				} else if ( rc == LDAP_RES_INTERMEDIATE ) {
					if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
						/* don't retry any more... */
						candidates[ i ].sr_type = REP_RESULT;
					}
	
					/* FIXME: response controls
					 * are passed without checks */
					rs->sr_err = ldap_parse_intermediate( msc->msc_ld,
						msg,
						(char **)&rs->sr_rspoid,
						&rs->sr_rspdata,
						&rs->sr_ctrls,
						0 );
					if ( rs->sr_err != LDAP_SUCCESS ) {
						candidates[ i ].sr_type = REP_RESULT;
						ldap_msgfree( res );
						res = NULL;
						goto really_bad;
					}

					slap_send_ldap_intermediate( op, rs );

					if ( rs->sr_rspoid != NULL ) {
						ber_memfree( (char *)rs->sr_rspoid );
						rs->sr_rspoid = NULL;
					}

					if ( rs->sr_rspdata != NULL ) {
						ber_bvfree( rs->sr_rspdata );
						rs->sr_rspdata = NULL;
					}

					if ( rs->sr_ctrls != NULL ) {
						ldap_controls_free( rs->sr_ctrls );
						rs->sr_ctrls = NULL;
					}

				} else if ( rc == LDAP_RES_SEARCH_RESULT ) {
					char		buf[ SLAP_TEXT_BUFLEN ];
					char		**references = NULL;
					LDAPControl	**ctrls = NULL;

					if ( candidates[ i ].sr_type == REP_INTERMEDIATE ) {
						/* don't retry any more... */
						candidates[ i ].sr_type = REP_RESULT;
					}
	
					candidates[ i ].sr_msgid = META_MSGID_IGNORE;

					/* NOTE: ignores response controls
					 * (and intermediate response controls
					 * as well, except for those with search
					 * references); this may not be correct,
					 * but if they're not ignored then
					 * back-meta would need to merge them
					 * consistently (think of pagedResults...)
					 */
					/* FIXME: response controls? */
					rs->sr_err = ldap_parse_result( msc->msc_ld,
						msg,
						&candidates[ i ].sr_err,
						(char **)&candidates[ i ].sr_matched,
						(char **)&candidates[ i ].sr_text,
						&references,
						&ctrls /* &candidates[ i ].sr_ctrls (unused) */ ,
						0 );
					if ( rs->sr_err != LDAP_SUCCESS ) {
						candidates[ i ].sr_err = rs->sr_err;
						sres = slap_map_api2result( &candidates[ i ] );
						candidates[ i ].sr_type = REP_RESULT;
						ldap_msgfree( res );
						res = NULL;
						goto really_bad;
					}

					rs->sr_err = candidates[ i ].sr_err;

					/* massage matchedDN if need be */
					if ( candidates[ i ].sr_matched != NULL ) {
						struct berval	match, mmatch;

						ber_str2bv( candidates[ i ].sr_matched,
							0, 0, &match );
						candidates[ i ].sr_matched = NULL;

						dc.ctx = "matchedDN";
						dc.target = mi->mi_targets[ i ];
						if ( !ldap_back_dn_massage( &dc, &match, &mmatch ) ) {
							if ( mmatch.bv_val == match.bv_val ) {
								candidates[ i ].sr_matched
									= ch_strdup( mmatch.bv_val );

							} else {
								candidates[ i ].sr_matched = mmatch.bv_val;
							}

							candidate_match++;
						} 
						ldap_memfree( match.bv_val );
					}

					/* add references to array */
					/* RFC 4511: referrals can only appear
					 * if result code is LDAP_REFERRAL */
					if ( references != NULL
						&& references[ 0 ] != NULL
						&& references[ 0 ][ 0 ] != '\0' )
					{
						if ( rs->sr_err != LDAP_REFERRAL ) {
							Debug( LDAP_DEBUG_ANY,
								"%s meta_back_search[%ld]: "
								"got referrals with err=%d\n",
								op->o_log_prefix,
								i, rs->sr_err );

						} else {
							BerVarray	sr_ref;
							int		cnt;
	
							for ( cnt = 0; references[ cnt ]; cnt++ )
								;
	
							sr_ref = ber_memalloc_x( sizeof( struct berval ) * ( cnt + 1 ),
								op->o_tmpmemctx );
	
							for ( cnt = 0; references[ cnt ]; cnt++ ) {
								ber_str2bv_x( references[ cnt ], 0, 1, &sr_ref[ cnt ],
									op->o_tmpmemctx );
							}
							BER_BVZERO( &sr_ref[ cnt ] );
	
							( void )ldap_back_referral_result_rewrite( &dc, sr_ref,
								op->o_tmpmemctx );
					
							if ( rs->sr_v2ref == NULL ) {
								rs->sr_v2ref = sr_ref;

							} else {
								for ( cnt = 0; !BER_BVISNULL( &sr_ref[ cnt ] ); cnt++ ) {
									ber_bvarray_add_x( &rs->sr_v2ref, &sr_ref[ cnt ],
										op->o_tmpmemctx );
								}
								ber_memfree_x( sr_ref, op->o_tmpmemctx );
							}
						}

					} else if ( rs->sr_err == LDAP_REFERRAL ) {
						Debug( LDAP_DEBUG_ANY,
							"%s meta_back_search[%ld]: "
							"got err=%d with null "
							"or empty referrals\n",
							op->o_log_prefix,
							i, rs->sr_err );

						rs->sr_err = LDAP_NO_SUCH_OBJECT;
					}

					/* cleanup */
					ber_memvfree( (void **)references );

					sres = slap_map_api2result( rs );
	
					if ( LogTest( LDAP_DEBUG_TRACE | LDAP_DEBUG_ANY ) ) {
						snprintf( buf, sizeof( buf ),
							"%s meta_back_search[%ld] "
							"match=\"%s\" err=%ld",
							op->o_log_prefix, i,
							candidates[ i ].sr_matched ? candidates[ i ].sr_matched : "",
							(long) candidates[ i ].sr_err );
						if ( candidates[ i ].sr_err == LDAP_SUCCESS ) {
							Debug( LDAP_DEBUG_TRACE, "%s.\n", buf, 0, 0 );
	
						} else {
							Debug( LDAP_DEBUG_ANY, "%s (%s).\n",
								buf, ldap_err2string( candidates[ i ].sr_err ), 0 );
						}
					}
	
					switch ( sres ) {
					case LDAP_NO_SUCH_OBJECT:
						/* is_ok is touched any time a valid
						 * (even intermediate) result is
						 * returned; as a consequence, if
						 * a candidate returns noSuchObject
						 * it is ignored and the candidate
						 * is simply demoted. */
						if ( is_ok ) {
							sres = LDAP_SUCCESS;
						}
						break;
	
					case LDAP_SUCCESS:
						if ( ctrls != NULL && ctrls[0] != NULL ) {
#ifdef SLAPD_META_CLIENT_PR
							LDAPControl *pr_c;

							pr_c = ldap_control_find( LDAP_CONTROL_PAGEDRESULTS, ctrls, NULL );
							if ( pr_c != NULL ) {
								BerElementBuffer berbuf;
								BerElement *ber = (BerElement *)&berbuf;
								ber_tag_t tag;
								ber_int_t prsize;
								struct berval prcookie;

								/* unsolicited, do not accept */
								if ( mi->mi_targets[i]->mt_ps == 0 ) {
									rs->sr_err = LDAP_OTHER;
									goto err_pr;
								}

								ber_init2( ber, &pr_c->ldctl_value, LBER_USE_DER );

								tag = ber_scanf( ber, "{im}", &prsize, &prcookie );
								if ( tag == LBER_ERROR ) {
									rs->sr_err = LDAP_OTHER;
									goto err_pr;
								}

								/* more pages? new search request */
								if ( !BER_BVISNULL( &prcookie ) && !BER_BVISEMPTY( &prcookie ) ) {
									if ( mi->mi_targets[i]->mt_ps > 0 ) {
										/* ignore size if specified */
										prsize = 0;

									} else if ( prsize == 0 ) {
										/* guess the page size from the entries returned so far */
										prsize = candidates[ i ].sr_nentries;
									}

									candidates[ i ].sr_nentries = 0;
									candidates[ i ].sr_msgid = META_MSGID_IGNORE;
									candidates[ i ].sr_type = REP_INTERMEDIATE;
								
									assert( candidates[ i ].sr_matched == NULL );
									assert( candidates[ i ].sr_text == NULL );
									assert( candidates[ i ].sr_ref == NULL );

									switch ( meta_back_search_start( op, rs, &dc, &mc, i, candidates, &prcookie, prsize ) )
									{
									case META_SEARCH_CANDIDATE:
										assert( candidates[ i ].sr_msgid >= 0 );
										ldap_controls_free( ctrls );
										goto free_message;

									case META_SEARCH_ERR:
err_pr:;
										candidates[ i ].sr_err = rs->sr_err;
										if ( META_BACK_ONERR_STOP( mi ) ) {
											savepriv = op->o_private;
											op->o_private = (void *)i;
											send_ldap_result( op, rs );
											op->o_private = savepriv;
											ldap_controls_free( ctrls );
											goto finish;
										}
										/* fallthru */

									case META_SEARCH_NOT_CANDIDATE:
										/* means that meta_back_search_start()
										 * failed but onerr == continue */
										candidates[ i ].sr_msgid = META_MSGID_IGNORE;
										assert( ncandidates > 0 );
										--ncandidates;
										break;

									default:
										/* impossible */
										assert( 0 );
										break;
									}
									break;
								}
							}
#endif /* SLAPD_META_CLIENT_PR */

							ldap_controls_free( ctrls );
						}
						/* fallthru */

					case LDAP_REFERRAL:
						is_ok++;
						break;
	
					case LDAP_SIZELIMIT_EXCEEDED:
						/* if a target returned sizelimitExceeded
						 * and the entry count is equal to the
						 * proxy's limit, the target would have
						 * returned more, and the error must be
						 * propagated to the client; otherwise,
						 * the target enforced a limit lower
						 * than what requested by the proxy;
						 * ignore it */
						candidates[ i ].sr_err = rs->sr_err;
						if ( rs->sr_nentries == op->ors_slimit
							|| META_BACK_ONERR_STOP( mi ) )
						{
							const char *save_text = rs->sr_text;
							savepriv = op->o_private;
							op->o_private = (void *)i;
							rs->sr_text = candidates[ i ].sr_text;
							send_ldap_result( op, rs );
							rs->sr_text = save_text;
							op->o_private = savepriv;
							ldap_msgfree( res );
							res = NULL;
							goto finish;
						}
						break;
	
					default:
						candidates[ i ].sr_err = rs->sr_err;
						if ( META_BACK_ONERR_STOP( mi ) ) {
							const char *save_text = rs->sr_text;
							savepriv = op->o_private;
							op->o_private = (void *)i;
							rs->sr_text = candidates[ i ].sr_text;
							send_ldap_result( op, rs );
							rs->sr_text = save_text;
							op->o_private = savepriv;
							ldap_msgfree( res );
							res = NULL;
							goto finish;
						}
						break;
					}
	
					last = i;
					rc = 0;
	
					/*
					 * When no candidates are left,
					 * the outer cycle finishes
					 */
					assert( ncandidates > 0 );
					--ncandidates;

				} else if ( rc == LDAP_RES_BIND ) {
					meta_search_candidate_t	retcode;
	
					retcode = meta_search_dobind_result( op, rs, &mc, i, candidates, msg );
					if ( retcode == META_SEARCH_CANDIDATE ) {
						candidates[ i ].sr_msgid = META_MSGID_IGNORE;
						retcode = meta_back_search_start( op, rs, &dc, &mc, i, candidates, NULL, 0 );
					}
	
					switch ( retcode ) {
					case META_SEARCH_CANDIDATE:
						break;
	
						/* means that failed but onerr == continue */
					case META_SEARCH_NOT_CANDIDATE:
					case META_SEARCH_ERR:
						candidates[ i ].sr_msgid = META_MSGID_IGNORE;
						assert( ncandidates > 0 );
						--ncandidates;
	
						candidates[ i ].sr_err = rs->sr_err;
						if ( META_BACK_ONERR_STOP( mi ) ) {
							savepriv = op->o_private;
							op->o_private = (void *)i;
							send_ldap_result( op, rs );
							op->o_private = savepriv;
							ldap_msgfree( res );
							res = NULL;
							goto finish;
						}
						goto free_message;
	
					default:
						assert( 0 );
						break;
					}
	
				} else {
					Debug( LDAP_DEBUG_ANY,
						"%s meta_back_search[%ld]: "
						"unrecognized response message tag=%d\n",
						op->o_log_prefix,
						i, rc );
				
					ldap_msgfree( res );
					res = NULL;
					goto really_bad;
				}
			}

free_message:;
			ldap_msgfree( res );
			res = NULL;
		}

		/* check for abandon */
		if ( op->o_abandon || LDAP_BACK_CONN_ABANDON( mc ) ) {
			for ( i = 0; i < mi->mi_ntargets; i++ ) {
				if ( candidates[ i ].sr_msgid >= 0
					|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
				{
					if ( META_IS_BINDING( &candidates[ i ] )
						|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
					{
						ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
						if ( LDAP_BACK_CONN_BINDING( &mc->mc_conns[ i ] )
							|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
						{
							/* if still binding, destroy */

#ifdef DEBUG_205
							char buf[ SLAP_TEXT_BUFLEN ];

							snprintf( buf, sizeof( buf), "%s meta_back_search(abandon) "
								"ldap_unbind_ext[%ld] mc=%p ld=%p",
								op->o_log_prefix, i, (void *)mc,
								(void *)mc->mc_conns[i].msc_ld );

							Debug( LDAP_DEBUG_ANY, "### %s\n", buf, 0, 0 );
#endif /* DEBUG_205 */

							meta_clear_one_candidate( op, mc, i );
						}
						ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
						META_BINDING_CLEAR( &candidates[ i ] );
						
					} else {
						(void)meta_back_cancel( mc, op, rs,
							candidates[ i ].sr_msgid, i,
							LDAP_BACK_DONTSEND );
					}

					candidates[ i ].sr_msgid = META_MSGID_IGNORE;
					assert( ncandidates > 0 );
					--ncandidates;
				}
			}

			if ( op->o_abandon ) {
				rc = SLAPD_ABANDON;
			}

			/* let send_ldap_result play cleanup handlers (ITS#4645) */
			break;
		}

		/* if no entry was found during this loop,
		 * set a minimal timeout */
		if ( ncandidates > 0 && gotit == 0 ) {
			if ( save_tv.tv_sec == 0 && save_tv.tv_usec == 0 ) {
				save_tv.tv_usec = LDAP_BACK_RESULT_UTIMEOUT/initial_candidates;

				/* arbitrarily limit to something between 1 and 2 minutes */
			} else if ( ( stoptime == -1 && save_tv.tv_sec < 60 )
				|| save_tv.tv_sec < ( stoptime - slap_get_time() ) / ( 2 * ncandidates ) )
			{
				/* double the timeout */
				lutil_timermul( &save_tv, 2, &save_tv );
			}

			if ( alreadybound == 0 ) {
				tv = save_tv;
				(void)select( 0, NULL, NULL, NULL, &tv );

			} else {
				ldap_pvt_thread_yield();
			}
		}
	}

	if ( rc == -1 ) {
		/*
		 * FIXME: need a better strategy to handle errors
		 */
		if ( mc ) {
			rc = meta_back_op_result( mc, op, rs, META_TARGET_NONE,
				-1, stoptime != -1 ? (stoptime - slap_get_time()) : 0,
				LDAP_BACK_SENDERR );
		} else {
			rc = rs->sr_err;
		}
		goto finish;
	}

	/*
	 * Rewrite the matched portion of the search base, if required
	 * 
	 * FIXME: only the last one gets caught!
	 */
	savepriv = op->o_private;
	op->o_private = (void *)(long)mi->mi_ntargets;
	if ( candidate_match > 0 ) {
		struct berval	pmatched = BER_BVNULL;

		/* we use the first one */
		for ( i = 0; i < mi->mi_ntargets; i++ ) {
			if ( META_IS_CANDIDATE( &candidates[ i ] )
					&& candidates[ i ].sr_matched != NULL )
			{
				struct berval	bv, pbv;
				int		rc;

				/* if we got success, and this target
				 * returned noSuchObject, and its suffix
				 * is a superior of the searchBase,
				 * ignore the matchedDN */
				if ( sres == LDAP_SUCCESS
					&& candidates[ i ].sr_err == LDAP_NO_SUCH_OBJECT
					&& op->o_req_ndn.bv_len > mi->mi_targets[ i ]->mt_nsuffix.bv_len )
				{
					free( (char *)candidates[ i ].sr_matched );
					candidates[ i ].sr_matched = NULL;
					continue;
				}

				ber_str2bv( candidates[ i ].sr_matched, 0, 0, &bv );
				rc = dnPretty( NULL, &bv, &pbv, op->o_tmpmemctx );

				if ( rc == LDAP_SUCCESS ) {

					/* NOTE: if they all are superiors
					 * of the baseDN, the shorter is also 
					 * superior of the longer... */
					if ( pbv.bv_len > pmatched.bv_len ) {
						if ( !BER_BVISNULL( &pmatched ) ) {
							op->o_tmpfree( pmatched.bv_val, op->o_tmpmemctx );
						}
						pmatched = pbv;
						op->o_private = (void *)i;

					} else {
						op->o_tmpfree( pbv.bv_val, op->o_tmpmemctx );
					}
				}

				if ( candidates[ i ].sr_matched != NULL ) {
					free( (char *)candidates[ i ].sr_matched );
					candidates[ i ].sr_matched = NULL;
				}
			}
		}

		if ( !BER_BVISNULL( &pmatched ) ) {
			matched = pmatched.bv_val;
		}

	} else if ( sres == LDAP_NO_SUCH_OBJECT ) {
		matched = op->o_bd->be_suffix[ 0 ].bv_val;
	}

	/*
	 * In case we returned at least one entry, we return LDAP_SUCCESS
	 * otherwise, the latter error code we got
	 */

	if ( sres == LDAP_SUCCESS ) {
		if ( rs->sr_v2ref ) {
			sres = LDAP_REFERRAL;
		}

		if ( META_BACK_ONERR_REPORT( mi ) ) {
			/*
			 * Report errors, if any
			 *
			 * FIXME: we should handle error codes and return the more 
			 * important/reasonable
			 */
			for ( i = 0; i < mi->mi_ntargets; i++ ) {
				if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
					continue;
				}

				if ( candidates[ i ].sr_err != LDAP_SUCCESS
					&& candidates[ i ].sr_err != LDAP_NO_SUCH_OBJECT )
				{
					sres = candidates[ i ].sr_err;
					break;
				}
			}
		}
	}

	rs->sr_err = sres;
	rs->sr_matched = ( sres == LDAP_SUCCESS ? NULL : matched );
	rs->sr_ref = ( sres == LDAP_REFERRAL ? rs->sr_v2ref : NULL );
	send_ldap_result( op, rs );
	op->o_private = savepriv;
	rs->sr_matched = NULL;
	rs->sr_ref = NULL;

finish:;
	if ( matched && matched != op->o_bd->be_suffix[ 0 ].bv_val ) {
		op->o_tmpfree( matched, op->o_tmpmemctx );
	}

	if ( rs->sr_v2ref ) {
		ber_bvarray_free_x( rs->sr_v2ref, op->o_tmpmemctx );
	}

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		if ( !META_IS_CANDIDATE( &candidates[ i ] ) ) {
			continue;
		}

		if ( mc ) {
			if ( META_IS_BINDING( &candidates[ i ] )
				|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
			{
				ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
				if ( LDAP_BACK_CONN_BINDING( &mc->mc_conns[ i ] )
					|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING )
				{
					assert( candidates[ i ].sr_msgid >= 0
						|| candidates[ i ].sr_msgid == META_MSGID_CONNECTING );
					assert( mc->mc_conns[ i ].msc_ld != NULL );

#ifdef DEBUG_205
					Debug( LDAP_DEBUG_ANY, "### %s meta_back_search(cleanup) "
						"ldap_unbind_ext[%ld] ld=%p\n",
						op->o_log_prefix, i, (void *)mc->mc_conns[i].msc_ld );
#endif /* DEBUG_205 */

					/* if still binding, destroy */
					meta_clear_one_candidate( op, mc, i );
				}
				ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
				META_BINDING_CLEAR( &candidates[ i ] );

			} else if ( candidates[ i ].sr_msgid >= 0 ) {
				(void)meta_back_cancel( mc, op, rs,
					candidates[ i ].sr_msgid, i,
					LDAP_BACK_DONTSEND );
			}
		}

		if ( candidates[ i ].sr_matched ) {
			free( (char *)candidates[ i ].sr_matched );
			candidates[ i ].sr_matched = NULL;
		}

		if ( candidates[ i ].sr_text ) {
			ldap_memfree( (char *)candidates[ i ].sr_text );
			candidates[ i ].sr_text = NULL;
		}

		if ( candidates[ i ].sr_ref ) {
			ber_bvarray_free( candidates[ i ].sr_ref );
			candidates[ i ].sr_ref = NULL;
		}

		if ( candidates[ i ].sr_ctrls ) {
			ldap_controls_free( candidates[ i ].sr_ctrls );
			candidates[ i ].sr_ctrls = NULL;
		}

		if ( META_BACK_TGT_QUARANTINE( mi->mi_targets[ i ] ) ) {
			meta_back_quarantine( op, &candidates[ i ], i );
		}

		/* only in case of timelimit exceeded, if the timelimit exceeded because
		 * one contacted target never responded, invalidate the connection
		 * NOTE: should we quarantine the target as well?  right now, the connection
		 * is invalidated; the next time it will be recreated and the target
		 * will be quarantined if it cannot be contacted */
		if ( mi->mi_idle_timeout != 0
			&& rs->sr_err == LDAP_TIMELIMIT_EXCEEDED
			&& op->o_time > mc->mc_conns[ i ].msc_time )
		{
			/* don't let anyone else use this expired connection */
			do_taint++;
		}
	}

	if ( mc ) {
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
		if ( do_taint ) {
			LDAP_BACK_CONN_TAINTED_SET( mc );
		}
		meta_back_release_conn_lock( mi, mc, 0 );
		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
	}

	return rs->sr_err;
}

static int
meta_send_entry(
	Operation 	*op,
	SlapReply	*rs,
	metaconn_t	*mc,
	int 		target,
	LDAPMessage 	*e )
{
	metainfo_t 		*mi = ( metainfo_t * )op->o_bd->be_private;
	struct berval		a, mapped;
	int			check_duplicate_attrs = 0;
	int			check_sorted_attrs = 0;
	Entry 			ent = { 0 };
	BerElement 		ber = *ldap_get_message_ber( e );
	Attribute 		*attr, **attrp;
	struct berval 		bdn,
				dn = BER_BVNULL;
	const char 		*text;
	dncookie		dc;
	ber_len_t		len;
	int			rc;

	if ( ber_scanf( &ber, "l{", &len ) == LBER_ERROR ) {
		return LDAP_DECODING_ERROR;
	}

	if ( ber_set_option( &ber, LBER_OPT_REMAINING_BYTES, &len ) != LBER_OPT_SUCCESS ) {
		return LDAP_OTHER;
	}

	if ( ber_scanf( &ber, "m{", &bdn ) == LBER_ERROR ) {
		return LDAP_DECODING_ERROR;
	}

	/*
	 * Rewrite the dn of the result, if needed
	 */
	dc.target = mi->mi_targets[ target ];
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "searchResult";

	rs->sr_err = ldap_back_dn_massage( &dc, &bdn, &dn );
	if ( rs->sr_err != LDAP_SUCCESS) {
		return rs->sr_err;
	}

	/*
	 * Note: this may fail if the target host(s) schema differs
	 * from the one known to the meta, and a DN with unknown
	 * attributes is returned.
	 * 
	 * FIXME: should we log anything, or delegate to dnNormalize?
	 */
	rc = dnPrettyNormal( NULL, &dn, &ent.e_name, &ent.e_nname,
		op->o_tmpmemctx );
	if ( dn.bv_val != bdn.bv_val ) {
		free( dn.bv_val );
	}
	BER_BVZERO( &dn );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"%s meta_send_entry(\"%s\"): "
			"invalid DN syntax\n",
			op->o_log_prefix, ent.e_name.bv_val, 0 );
		rc = LDAP_INVALID_DN_SYNTAX;
		goto done;
	}

	/*
	 * cache dn
	 */
	if ( mi->mi_cache.ttl != META_DNCACHE_DISABLED ) {
		( void )meta_dncache_update_entry( &mi->mi_cache,
				&ent.e_nname, target );
	}

	attrp = &ent.e_attrs;

	dc.ctx = "searchAttrDN";
	while ( ber_scanf( &ber, "{m", &a ) != LBER_ERROR ) {
		int				last = 0;
		slap_syntax_validate_func	*validate;
		slap_syntax_transform_func	*pretty;

		if ( ber_pvt_ber_remaining( &ber ) < 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"%s meta_send_entry(\"%s\"): "
				"unable to parse attr \"%s\".\n",
				op->o_log_prefix, ent.e_name.bv_val, a.bv_val );
				
			rc = LDAP_OTHER;
			goto done;
		}

		if ( ber_pvt_ber_remaining( &ber ) == 0 ) {
			break;
		}

		ldap_back_map( &mi->mi_targets[ target ]->mt_rwmap.rwm_at, 
				&a, &mapped, BACKLDAP_REMAP );
		if ( BER_BVISNULL( &mapped ) || mapped.bv_val[0] == '\0' ) {
			( void )ber_scanf( &ber, "x" /* [W] */ );
			continue;
		}
		if ( mapped.bv_val != a.bv_val ) {
			/* will need to check for duplicate attrs */
			check_duplicate_attrs++;
		}
		attr = attr_alloc( NULL );
		if ( attr == NULL ) {
			rc = LDAP_OTHER;
			goto done;
		}
		if ( slap_bv2ad( &mapped, &attr->a_desc, &text )
				!= LDAP_SUCCESS) {
			if ( slap_bv2undef_ad( &mapped, &attr->a_desc, &text,
				SLAP_AD_PROXIED ) != LDAP_SUCCESS )
			{
				char	buf[ SLAP_TEXT_BUFLEN ];

				snprintf( buf, sizeof( buf ),
					"%s meta_send_entry(\"%s\"): "
					"slap_bv2undef_ad(%s): %s\n",
					op->o_log_prefix, ent.e_name.bv_val,
					mapped.bv_val, text );

				Debug( LDAP_DEBUG_ANY, "%s", buf, 0, 0 );
				( void )ber_scanf( &ber, "x" /* [W] */ );
				attr_free( attr );
				continue;
			}
		}

		if ( attr->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL )
			check_sorted_attrs = 1;

		/* no subschemaSubentry */
		if ( attr->a_desc == slap_schema.si_ad_subschemaSubentry
			|| attr->a_desc == slap_schema.si_ad_entryDN )
		{

			/* 
			 * We eat target's subschemaSubentry because
			 * a search for this value is likely not
			 * to resolve to the appropriate backend;
			 * later, the local subschemaSubentry is
			 * added.
			 *
			 * We also eat entryDN because the frontend
			 * will reattach it without checking if already
			 * present...
			 */
			( void )ber_scanf( &ber, "x" /* [W] */ );
			attr_free(attr);
			continue;
		}

		if ( ber_scanf( &ber, "[W]", &attr->a_vals ) == LBER_ERROR 
				|| attr->a_vals == NULL )
		{
			attr->a_vals = (struct berval *)&slap_dummy_bv;

		} else {
			for ( last = 0; !BER_BVISNULL( &attr->a_vals[ last ] ); ++last )
				;
		}
		attr->a_numvals = last;

		validate = attr->a_desc->ad_type->sat_syntax->ssyn_validate;
		pretty = attr->a_desc->ad_type->sat_syntax->ssyn_pretty;

		if ( !validate && !pretty ) {
			attr_free( attr );
			goto next_attr;
		}

		if ( attr->a_desc == slap_schema.si_ad_objectClass
				|| attr->a_desc == slap_schema.si_ad_structuralObjectClass )
		{
			struct berval 	*bv;

			for ( bv = attr->a_vals; !BER_BVISNULL( bv ); bv++ ) {
				ObjectClass *oc;

				ldap_back_map( &mi->mi_targets[ target ]->mt_rwmap.rwm_oc,
						bv, &mapped, BACKLDAP_REMAP );
				if ( BER_BVISNULL( &mapped ) || mapped.bv_val[0] == '\0') {
remove_oc:;
					free( bv->bv_val );
					BER_BVZERO( bv );
					if ( --last < 0 ) {
						break;
					}
					*bv = attr->a_vals[ last ];
					BER_BVZERO( &attr->a_vals[ last ] );
					bv--;

				} else if ( mapped.bv_val != bv->bv_val ) {
					int	i;

					for ( i = 0; !BER_BVISNULL( &attr->a_vals[ i ] ); i++ ) {
						if ( &attr->a_vals[ i ] == bv ) {
							continue;
						}

						if ( ber_bvstrcasecmp( &mapped, &attr->a_vals[ i ] ) == 0 ) {
							break;
						}
					}

					if ( !BER_BVISNULL( &attr->a_vals[ i ] ) ) {
						goto remove_oc;
					}

					ber_bvreplace( bv, &mapped );

				} else if ( ( oc = oc_bvfind_undef( bv ) ) == NULL ) {
					goto remove_oc;

				} else {
					ber_bvreplace( bv, &oc->soc_cname );
				}
			}
		/*
		 * It is necessary to try to rewrite attributes with
		 * dn syntax because they might be used in ACLs as
		 * members of groups; since ACLs are applied to the
		 * rewritten stuff, no dn-based subecj clause could
		 * be used at the ldap backend side (see
		 * http://www.OpenLDAP.org/faq/data/cache/452.html)
		 * The problem can be overcome by moving the dn-based
		 * ACLs to the target directory server, and letting
		 * everything pass thru the ldap backend.
		 */
		} else {
			int	i;

			if ( attr->a_desc->ad_type->sat_syntax ==
				slap_schema.si_syn_distinguishedName )
			{
				ldap_dnattr_result_rewrite( &dc, attr->a_vals );

			} else if ( attr->a_desc == slap_schema.si_ad_ref ) {
				ldap_back_referral_result_rewrite( &dc, attr->a_vals, NULL );

			}

			for ( i = 0; i < last; i++ ) {
				struct berval	pval;
				int		rc;

				if ( pretty ) {
					rc = ordered_value_pretty( attr->a_desc,
						&attr->a_vals[i], &pval, NULL );

				} else {
					rc = ordered_value_validate( attr->a_desc,
						&attr->a_vals[i], 0 );
				}

				if ( rc ) {
					ber_memfree( attr->a_vals[i].bv_val );
					if ( --last == i ) {
						BER_BVZERO( &attr->a_vals[ i ] );
						break;
					}
					attr->a_vals[i] = attr->a_vals[last];
					BER_BVZERO( &attr->a_vals[last] );
					i--;
					continue;
				}

				if ( pretty ) {
					ber_memfree( attr->a_vals[i].bv_val );
					attr->a_vals[i] = pval;
				}
			}

			if ( last == 0 && attr->a_vals != &slap_dummy_bv ) {
				attr_free( attr );
				goto next_attr;
			}
		}

		if ( last && attr->a_desc->ad_type->sat_equality &&
			attr->a_desc->ad_type->sat_equality->smr_normalize )
		{
			int i;

			attr->a_nvals = ch_malloc( ( last + 1 ) * sizeof( struct berval ) );
			for ( i = 0; i<last; i++ ) {
				/* if normalizer fails, drop this value */
				if ( ordered_value_normalize(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					attr->a_desc,
					attr->a_desc->ad_type->sat_equality,
					&attr->a_vals[i], &attr->a_nvals[i],
					NULL )) {
					ber_memfree( attr->a_vals[i].bv_val );
					if ( --last == i ) {
						BER_BVZERO( &attr->a_vals[ i ] );
						break;
					}
					attr->a_vals[i] = attr->a_vals[last];
					BER_BVZERO( &attr->a_vals[last] );
					i--;
				}
			}
			BER_BVZERO( &attr->a_nvals[i] );
			if ( last == 0 ) {
				attr_free( attr );
				goto next_attr;
			}

		} else {
			attr->a_nvals = attr->a_vals;
		}

		attr->a_numvals = last;
		*attrp = attr;
		attrp = &attr->a_next;
next_attr:;
	}

	/* only check if some mapping occurred */
	if ( check_duplicate_attrs ) {
		Attribute	**ap;

		for ( ap = &ent.e_attrs; *ap != NULL; ap = &(*ap)->a_next ) {
			Attribute	**tap;

			for ( tap = &(*ap)->a_next; *tap != NULL; ) {
				if ( (*tap)->a_desc == (*ap)->a_desc ) {
					Entry		e = { 0 };
					Modification	mod = { 0 };
					const char	*text = NULL;
					char		textbuf[ SLAP_TEXT_BUFLEN ];
					Attribute	*next = (*tap)->a_next;

					BER_BVSTR( &e.e_name, "" );
					BER_BVSTR( &e.e_nname, "" );
					e.e_attrs = *ap;
					mod.sm_op = LDAP_MOD_ADD;
					mod.sm_desc = (*ap)->a_desc;
					mod.sm_type = mod.sm_desc->ad_cname;
					mod.sm_numvals = (*ap)->a_numvals;
					mod.sm_values = (*tap)->a_vals;
					if ( (*tap)->a_nvals != (*tap)->a_vals ) {
						mod.sm_nvalues = (*tap)->a_nvals;
					}

					(void)modify_add_values( &e, &mod,
						/* permissive */ 1,
						&text, textbuf, sizeof( textbuf ) );

					/* should not insert new attrs! */
					assert( e.e_attrs == *ap );

					attr_free( *tap );
					*tap = next;

				} else {
					tap = &(*tap)->a_next;
				}
			}
		}
	}

	/* Check for sorted attributes */
	if ( check_sorted_attrs ) {
		for ( attr = ent.e_attrs; attr; attr = attr->a_next ) {
			if ( attr->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL ) {
				while ( attr->a_numvals > 1 ) {
					int i;
					int rc = slap_sort_vals( (Modifications *)attr, &text, &i, op->o_tmpmemctx );
					if ( rc != LDAP_TYPE_OR_VALUE_EXISTS )
						break;

					/* Strip duplicate values */
					if ( attr->a_nvals != attr->a_vals )
						ber_memfree( attr->a_nvals[i].bv_val );
					ber_memfree( attr->a_vals[i].bv_val );
					attr->a_numvals--;
					if ( (unsigned)i < attr->a_numvals ) {
						attr->a_vals[i] = attr->a_vals[attr->a_numvals];
						if ( attr->a_nvals != attr->a_vals )
							attr->a_nvals[i] = attr->a_nvals[attr->a_numvals];
					}
					BER_BVZERO(&attr->a_vals[attr->a_numvals]);
					if ( attr->a_nvals != attr->a_vals )
						BER_BVZERO(&attr->a_nvals[attr->a_numvals]);
				}
				attr->a_flags |= SLAP_ATTR_SORTED_VALS;
			}
		}
	}

	ldap_get_entry_controls( mc->mc_conns[target].msc_ld,
		e, &rs->sr_ctrls );
	rs->sr_entry = &ent;
	rs->sr_attrs = op->ors_attrs;
	rs->sr_operational_attrs = NULL;
	rs->sr_flags = mi->mi_targets[ target ]->mt_rep_flags;
	rs->sr_err = LDAP_SUCCESS;
	rc = send_search_entry( op, rs );
	switch ( rc ) {
	case LDAP_UNAVAILABLE:
		rc = LDAP_OTHER;
		break;
	}

done:;
	rs->sr_entry = NULL;
	rs->sr_attrs = NULL;
	if ( rs->sr_ctrls != NULL ) {
		ldap_controls_free( rs->sr_ctrls );
		rs->sr_ctrls = NULL;
	}
	if ( !BER_BVISNULL( &ent.e_name ) ) {
		free( ent.e_name.bv_val );
		BER_BVZERO( &ent.e_name );
	}
	if ( !BER_BVISNULL( &ent.e_nname ) ) {
		free( ent.e_nname.bv_val );
		BER_BVZERO( &ent.e_nname );
	}
	entry_clean( &ent );

	return rc;
}

