/* bind.c - ldap backend bind function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by Howard Chu for inclusion
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
#include "back-ldap.h"
#include "lutil.h"
#include "lutil_ldap.h"

#define LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ	"2.16.840.1.113730.3.4.12"

#define SLAP_AUTH_DN 1

#if LDAP_BACK_PRINT_CONNTREE > 0

static const struct {
	slap_mask_t	f;
	char		c;
} flagsmap[] = {
	{ LDAP_BACK_FCONN_ISBOUND,	'B' },
	{ LDAP_BACK_FCONN_ISANON,	'A' },
	{ LDAP_BACK_FCONN_ISPRIV,	'P' },
	{ LDAP_BACK_FCONN_ISTLS,	'T' },
	{ LDAP_BACK_FCONN_BINDING,	'X' },
	{ LDAP_BACK_FCONN_TAINTED,	'E' },
	{ LDAP_BACK_FCONN_ABANDON,	'N' },
	{ LDAP_BACK_FCONN_ISIDASR,	'S' },
	{ LDAP_BACK_FCONN_CACHED,	'C' },
	{ 0,				'\0' }
};

static void
ldap_back_conn_print( ldapconn_t *lc, const char *avlstr )
{
	char buf[ SLAP_TEXT_BUFLEN ];
	char fbuf[ sizeof("BAPTIENSC") ];
	int i;

	ldap_back_conn2str( &lc->lc_base, buf, sizeof( buf ) );
	for ( i = 0; flagsmap[ i ].c != '\0'; i++ ) {
		if ( lc->lc_lcflags & flagsmap[i].f ) {
			fbuf[i] = flagsmap[i].c;

		} else {
			fbuf[i] = '.';
		}
	}
	fbuf[i] = '\0';
	
	fprintf( stderr, "lc=%p %s %s flags=0x%08x (%s)\n",
		(void *)lc, buf, avlstr, lc->lc_lcflags, fbuf );
}

static void
ldap_back_ravl_print( Avlnode *root, int depth )
{
	int		i;
	ldapconn_t	*lc;
	
	if ( root == 0 ) {
		return;
	}
	
	ldap_back_ravl_print( root->avl_right, depth+1 );
	
	for ( i = 0; i < depth; i++ ) {
		fprintf( stderr, "-" );
	}

	lc = root->avl_data;
	ldap_back_conn_print( lc, avl_bf2str( root->avl_bf ) );

	ldap_back_ravl_print( root->avl_left, depth + 1 );
}

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
ldap_back_print_conntree( ldapinfo_t *li, char *msg )
{
	int	c;

	fprintf( stderr, "========> %s\n", msg );

	for ( c = LDAP_BACK_PCONN_FIRST; c < LDAP_BACK_PCONN_LAST; c++ ) {
		int		i = 0;
		ldapconn_t	*lc;

		fprintf( stderr, "  %s[%d]\n", priv2str[ c ], li->li_conn_priv[ c ].lic_num );

		LDAP_TAILQ_FOREACH( lc, &li->li_conn_priv[ c ].lic_priv, lc_q )
		{
			fprintf( stderr, "    [%d] ", i );
			ldap_back_conn_print( lc, "" );
			i++;
		}
	}
	
	if ( li->li_conninfo.lai_tree == 0 ) {
		fprintf( stderr, "\t(empty)\n" );

	} else {
		ldap_back_ravl_print( li->li_conninfo.lai_tree, 0 );
	}
	
	fprintf( stderr, "<======== %s\n", msg );
}
#endif /* LDAP_BACK_PRINT_CONNTREE */

static int
ldap_back_freeconn( ldapinfo_t *li, ldapconn_t *lc, int dolock );

static ldapconn_t *
ldap_back_getconn( Operation *op, SlapReply *rs, ldap_back_send_t sendok,
	struct berval *binddn, struct berval *bindcred );

static int
ldap_back_is_proxy_authz( Operation *op, SlapReply *rs, ldap_back_send_t sendok,
	struct berval *binddn, struct berval *bindcred );

static int
ldap_back_proxy_authz_bind( ldapconn_t *lc, Operation *op, SlapReply *rs,
	ldap_back_send_t sendok, struct berval *binddn, struct berval *bindcred );

static int
ldap_back_prepare_conn( ldapconn_t *lc, Operation *op, SlapReply *rs,
	ldap_back_send_t sendok );

static int
ldap_back_conndnlc_cmp( const void *c1, const void *c2 );

ldapconn_t *
ldap_back_conn_delete( ldapinfo_t *li, ldapconn_t *lc )
{
	if ( LDAP_BACK_PCONN_ISPRIV( lc ) ) {
		if ( LDAP_BACK_CONN_CACHED( lc ) ) {
			assert( lc->lc_q.tqe_prev != NULL );
			assert( li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_num > 0 );
			li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_num--;
			LDAP_TAILQ_REMOVE( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_priv, lc, lc_q );
			LDAP_TAILQ_ENTRY_INIT( lc, lc_q );
			LDAP_BACK_CONN_CACHED_CLEAR( lc );

		} else {
			assert( LDAP_BACK_CONN_TAINTED( lc ) );
			assert( lc->lc_q.tqe_prev == NULL );
		}

	} else {
		ldapconn_t	*tmplc = NULL;

		if ( LDAP_BACK_CONN_CACHED( lc ) ) {
			assert( !LDAP_BACK_CONN_TAINTED( lc ) );
			tmplc = avl_delete( &li->li_conninfo.lai_tree, (caddr_t)lc,
				ldap_back_conndnlc_cmp );
			assert( tmplc == lc );
			LDAP_BACK_CONN_CACHED_CLEAR( lc );
		}

		assert( LDAP_BACK_CONN_TAINTED( lc ) || tmplc == lc );
	}

	return lc;
}

int
ldap_back_bind( Operation *op, SlapReply *rs )
{
	ldapinfo_t		*li = (ldapinfo_t *) op->o_bd->be_private;
	ldapconn_t		*lc;

	LDAPControl		**ctrls = NULL;
	struct berval		save_o_dn;
	int			save_o_do_not_cache,
				rc = 0;
	ber_int_t		msgid;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;

	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		return rs->sr_err;
	}

	lc = ldap_back_getconn( op, rs, LDAP_BACK_BIND_SERR, NULL, NULL );
	if ( !lc ) {
		return rs->sr_err;
	}

	/* we can do (almost) whatever we want with this conn,
	 * because either it's temporary, or it's marked as binding */
	if ( !BER_BVISNULL( &lc->lc_bound_ndn ) ) {
		ch_free( lc->lc_bound_ndn.bv_val );
		BER_BVZERO( &lc->lc_bound_ndn );
	}
	if ( !BER_BVISNULL( &lc->lc_cred ) ) {
		memset( lc->lc_cred.bv_val, 0, lc->lc_cred.bv_len );
		ch_free( lc->lc_cred.bv_val );
		BER_BVZERO( &lc->lc_cred );
	}
	LDAP_BACK_CONN_ISBOUND_CLEAR( lc );

	/* don't add proxyAuthz; set the bindDN */
	save_o_dn = op->o_dn;
	save_o_do_not_cache = op->o_do_not_cache;
	op->o_dn = op->o_req_dn;
	op->o_do_not_cache = 1;

	ctrls = op->o_ctrls;
	rc = ldap_back_controls_add( op, rs, lc, &ctrls );
	op->o_dn = save_o_dn;
	op->o_do_not_cache = save_o_do_not_cache;
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		ldap_back_release_conn( li, lc );
		return( rc );
	}

retry:;
	/* method is always LDAP_AUTH_SIMPLE if we got here */
	rs->sr_err = ldap_sasl_bind( lc->lc_ld, op->o_req_dn.bv_val,
			LDAP_SASL_SIMPLE,
			&op->orb_cred, ctrls, NULL, &msgid );
	/* FIXME: should we always retry, or only when piping the bind
	 * in the "override" connection pool? */
	rc = ldap_back_op_result( lc, op, rs, msgid,
		li->li_timeout[ SLAP_OP_BIND ],
		LDAP_BACK_BIND_SERR | retrying );
	if ( rc == LDAP_UNAVAILABLE && retrying ) {
		retrying &= ~LDAP_BACK_RETRYING;
		if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_BIND_SERR ) ) {
			goto retry;
		}
	}

	ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
	ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_BIND ], 1 );
	ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

	ldap_back_controls_free( op, rs, &ctrls );

	if ( rc == LDAP_SUCCESS ) {
		op->o_conn->c_authz_cookie = op->o_bd->be_private;

		/* If defined, proxyAuthz will be used also when
		 * back-ldap is the authorizing backend; for this
		 * purpose, after a successful bind the connection
		 * is left for further binds, and further operations 
		 * on this client connection will use a default
		 * connection with identity assertion */
		/* NOTE: use with care */
		if ( li->li_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) {
			ldap_back_release_conn( li, lc );
			return( rc );
		}

		/* rebind is now done inside ldap_back_proxy_authz_bind()
		 * in case of success */
		LDAP_BACK_CONN_ISBOUND_SET( lc );
		ber_dupbv( &lc->lc_bound_ndn, &op->o_req_ndn );

		if ( !BER_BVISNULL( &lc->lc_cred ) ) {
			memset( lc->lc_cred.bv_val, 0,
					lc->lc_cred.bv_len );
		}

		if ( LDAP_BACK_SAVECRED( li ) ) {
			ber_bvreplace( &lc->lc_cred, &op->orb_cred );
			ldap_set_rebind_proc( lc->lc_ld, li->li_rebind_f, lc );

		} else {
			lc->lc_cred.bv_len = 0;
		}
	}

	/* must re-insert if local DN changed as result of bind */
	if ( !LDAP_BACK_CONN_ISBOUND( lc )
		|| ( !dn_match( &op->o_req_ndn, &lc->lc_local_ndn )
			&& !LDAP_BACK_PCONN_ISPRIV( lc ) ) )
	{
		int		lerr = -1;
		ldapconn_t	*tmplc;

		/* wait for all other ops to release the connection */
retry_lock:;
		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
		if ( lc->lc_refcnt > 1 ) {
			ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
			ldap_pvt_thread_yield();
			goto retry_lock;
		}

#if LDAP_BACK_PRINT_CONNTREE > 0
		ldap_back_print_conntree( li, ">>> ldap_back_bind" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

		assert( lc->lc_refcnt == 1 );
		ldap_back_conn_delete( li, lc );

		/* delete all cached connections with the current connection */
		if ( LDAP_BACK_SINGLECONN( li ) ) {
			while ( ( tmplc = avl_delete( &li->li_conninfo.lai_tree, (caddr_t)lc, ldap_back_conn_cmp ) ) != NULL )
			{
				assert( !LDAP_BACK_PCONN_ISPRIV( lc ) );
				Debug( LDAP_DEBUG_TRACE,
					"=>ldap_back_bind: destroying conn %lu (refcnt=%u)\n",
					lc->lc_conn->c_connid, lc->lc_refcnt, 0 );

				if ( tmplc->lc_refcnt != 0 ) {
					/* taint it */
					LDAP_BACK_CONN_TAINTED_SET( tmplc );
					LDAP_BACK_CONN_CACHED_CLEAR( tmplc );

				} else {
					/*
					 * Needs a test because the handler may be corrupted,
					 * and calling ldap_unbind on a corrupted header results
					 * in a segmentation fault
					 */
					ldap_back_conn_free( tmplc );
				}
			}
		}

		if ( LDAP_BACK_CONN_ISBOUND( lc ) ) {
			ber_bvreplace( &lc->lc_local_ndn, &op->o_req_ndn );
			if ( be_isroot_dn( op->o_bd, &op->o_req_ndn ) ) {
				LDAP_BACK_PCONN_ROOTDN_SET( lc, op );
			}
			lerr = avl_insert( &li->li_conninfo.lai_tree, (caddr_t)lc,
				ldap_back_conndn_cmp, ldap_back_conndn_dup );
		}

#if LDAP_BACK_PRINT_CONNTREE > 0
		ldap_back_print_conntree( li, "<<< ldap_back_bind" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
	
		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
		switch ( lerr ) {
		case 0:
			LDAP_BACK_CONN_CACHED_SET( lc );
			break;

		case -1:
			/* duplicate; someone else successfully bound
			 * on the same connection with the same identity;
			 * we can do this because lc_refcnt == 1 */
			ldap_back_conn_free( lc );
			lc = NULL;
		}
	}

	if ( lc != NULL ) {
		ldap_back_release_conn( li, lc );
	}

	return( rc );
}

/*
 * ldap_back_conndn_cmp
 *
 * compares two ldapconn_t based on the value of the conn pointer
 * and of the local DN; used by avl stuff for insert, lookup
 * and direct delete
 */
int
ldap_back_conndn_cmp( const void *c1, const void *c2 )
{
	const ldapconn_t	*lc1 = (const ldapconn_t *)c1;
	const ldapconn_t	*lc2 = (const ldapconn_t *)c2;
	int rc;

	/* If local DNs don't match, it is definitely not a match */
	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	rc = SLAP_PTRCMP( lc1->lc_conn, lc2->lc_conn );
	if ( rc == 0 ) {
		rc = ber_bvcmp( &lc1->lc_local_ndn, &lc2->lc_local_ndn );
	}

	return rc;
}

/*
 * ldap_back_conndnlc_cmp
 *
 * compares two ldapconn_t based on the value of the conn pointer,
 * the local DN and the lc pointer; used by avl stuff for insert, lookup
 * and direct delete
 */
static int
ldap_back_conndnlc_cmp( const void *c1, const void *c2 )
{
	const ldapconn_t	*lc1 = (const ldapconn_t *)c1;
	const ldapconn_t	*lc2 = (const ldapconn_t *)c2;
	int rc;

	/* If local DNs don't match, it is definitely not a match */
	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	rc = SLAP_PTRCMP( lc1->lc_conn, lc2->lc_conn );
	if ( rc == 0 ) {
		rc = ber_bvcmp( &lc1->lc_local_ndn, &lc2->lc_local_ndn );
		if ( rc == 0 ) {
			rc = SLAP_PTRCMP( lc1, lc2 );
		}
	}

	return rc;
}

/*
 * ldap_back_conn_cmp
 *
 * compares two ldapconn_t based on the value of the conn pointer;
 * used by avl stuff for delete of all conns with the same connid
 */
int
ldap_back_conn_cmp( const void *c1, const void *c2 )
{
	const ldapconn_t	*lc1 = (const ldapconn_t *)c1;
	const ldapconn_t	*lc2 = (const ldapconn_t *)c2;

	/* For shared sessions, conn is NULL. Only explicitly
	 * bound sessions will have non-NULL conn.
	 */
	return SLAP_PTRCMP( lc1->lc_conn, lc2->lc_conn );
}

/*
 * ldap_back_conndn_dup
 *
 * returns -1 in case a duplicate ldapconn_t has been inserted;
 * used by avl stuff
 */
int
ldap_back_conndn_dup( void *c1, void *c2 )
{
	ldapconn_t	*lc1 = (ldapconn_t *)c1;
	ldapconn_t	*lc2 = (ldapconn_t *)c2;

	/* Cannot have more than one shared session with same DN */
	if ( lc1->lc_conn == lc2->lc_conn &&
		dn_match( &lc1->lc_local_ndn, &lc2->lc_local_ndn ) )
	{
		return -1;
	}
		
	return 0;
}

static int
ldap_back_freeconn( ldapinfo_t *li, ldapconn_t *lc, int dolock )
{
	if ( dolock ) {
		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
	}

#if LDAP_BACK_PRINT_CONNTREE > 0
	ldap_back_print_conntree( li, ">>> ldap_back_freeconn" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

	(void)ldap_back_conn_delete( li, lc );

	if ( lc->lc_refcnt == 0 ) {
		ldap_back_conn_free( (void *)lc );
	}

#if LDAP_BACK_PRINT_CONNTREE > 0
	ldap_back_print_conntree( li, "<<< ldap_back_freeconn" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

	if ( dolock ) {
		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
	}

	return 0;
}

#ifdef HAVE_TLS
static int
ldap_back_start_tls(
	LDAP		*ld,
	int		protocol,
	int		*is_tls,
	const char	*url,
	unsigned	flags,
	int		retries,
	const char	**text )
{
	int		rc = LDAP_SUCCESS;

	/* start TLS ("tls-[try-]{start,propagate}" statements) */
	if ( ( LDAP_BACK_USE_TLS_F( flags ) || ( *is_tls && LDAP_BACK_PROPAGATE_TLS_F( flags ) ) )
				&& !ldap_is_ldaps_url( url ) )
	{
#ifdef SLAP_STARTTLS_ASYNCHRONOUS
		/*
		 * use asynchronous StartTLS
		 * in case, chase referral (not implemented yet)
		 */
		int		msgid;

		if ( protocol == 0 ) {
			ldap_get_option( ld, LDAP_OPT_PROTOCOL_VERSION,
					(void *)&protocol );
		}

		if ( protocol < LDAP_VERSION3 ) {
			/* we should rather bail out... */
			rc = LDAP_UNWILLING_TO_PERFORM;
			*text = "invalid protocol version";
		}

		if ( rc == LDAP_SUCCESS ) {
			rc = ldap_start_tls( ld, NULL, NULL, &msgid );
		}

		if ( rc == LDAP_SUCCESS ) {
			LDAPMessage	*res = NULL;
			struct timeval	tv;

			LDAP_BACK_TV_SET( &tv );

retry:;
			rc = ldap_result( ld, msgid, LDAP_MSG_ALL, &tv, &res );
			if ( rc < 0 ) {
				rc = LDAP_UNAVAILABLE;

			} else if ( rc == 0 ) {
				if ( retries != LDAP_BACK_RETRY_NEVER ) {
					ldap_pvt_thread_yield();
					if ( retries > 0 ) {
						retries--;
					}
					LDAP_BACK_TV_SET( &tv );
					goto retry;
				}
				rc = LDAP_UNAVAILABLE;

			} else if ( rc == LDAP_RES_EXTENDED ) {
				struct berval	*data = NULL;

				rc = ldap_parse_extended_result( ld, res,
						NULL, &data, 0 );
				if ( rc == LDAP_SUCCESS ) {
					SlapReply rs;
					rc = ldap_parse_result( ld, res, &rs.sr_err,
						NULL, NULL, NULL, NULL, 1 );
					if ( rc != LDAP_SUCCESS ) {
						rs.sr_err = rc;
					}
					rc = slap_map_api2result( &rs );
					res = NULL;
					
					/* FIXME: in case a referral 
					 * is returned, should we try
					 * using it instead of the 
					 * configured URI? */
					if ( rc == LDAP_SUCCESS ) {
						rc = ldap_install_tls( ld );

					} else if ( rc == LDAP_REFERRAL ) {
						rc = LDAP_UNWILLING_TO_PERFORM;
						*text = "unwilling to chase referral returned by Start TLS exop";
					}

					if ( data ) {
						if ( data->bv_val ) {
							ber_memfree( data->bv_val );
						}
						ber_memfree( data );
					}
				}

			} else {
				rc = LDAP_OTHER;
			}

			if ( res != NULL ) {
				ldap_msgfree( res );
			}
		}
#else /* ! SLAP_STARTTLS_ASYNCHRONOUS */
		/*
		 * use synchronous StartTLS
		 */
		rc = ldap_start_tls_s( ld, NULL, NULL );
#endif /* ! SLAP_STARTTLS_ASYNCHRONOUS */

		/* if StartTLS is requested, only attempt it if the URL
		 * is not "ldaps://"; this may occur not only in case
		 * of misconfiguration, but also when used in the chain 
		 * overlay, where the "uri" can be parsed out of a referral */
		switch ( rc ) {
		case LDAP_SUCCESS:
			*is_tls = 1;
			break;

		case LDAP_SERVER_DOWN:
			break;

		default:
			if ( LDAP_BACK_TLS_CRITICAL_F( flags ) ) {
				*text = "could not start TLS";
				break;
			}

			/* in case Start TLS is not critical */
			*is_tls = 0;
			rc = LDAP_SUCCESS;
			break;
		}

	} else {
		*is_tls = 0;
	}

	return rc;
}
#endif /* HAVE_TLS */

static int
ldap_back_prepare_conn( ldapconn_t *lc, Operation *op, SlapReply *rs, ldap_back_send_t sendok )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;
	int		version;
	LDAP		*ld = NULL;
#ifdef HAVE_TLS
	int		is_tls = op->o_conn->c_is_tls;
	int		flags = li->li_flags;
	time_t		lctime = (time_t)(-1);
	slap_bindconf *sb;
#endif /* HAVE_TLS */

	ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
	rs->sr_err = ldap_initialize( &ld, li->li_uri );
	ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto error_return;
	}

	if ( li->li_urllist_f ) {
		ldap_set_urllist_proc( ld, li->li_urllist_f, li->li_urllist_p );
	}

	/* Set LDAP version. This will always succeed: If the client
	 * bound with a particular version, then so can we.
	 */
	if ( li->li_version != 0 ) {
		version = li->li_version;

	} else if ( op->o_protocol != 0 ) {
		version = op->o_protocol;

	} else {
		/* assume it's an internal op; set to LDAPv3 */
		version = LDAP_VERSION3;
	}
	ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, (const void *)&version );

	/* automatically chase referrals ("chase-referrals [{yes|no}]" statement) */
	ldap_set_option( ld, LDAP_OPT_REFERRALS,
		LDAP_BACK_CHASE_REFERRALS( li ) ? LDAP_OPT_ON : LDAP_OPT_OFF );

	if ( li->li_network_timeout > 0 ) {
		struct timeval		tv;

		tv.tv_sec = li->li_network_timeout;
		tv.tv_usec = 0;
		ldap_set_option( ld, LDAP_OPT_NETWORK_TIMEOUT, (const void *)&tv );
	}

	/* turn on network keepalive, if configured so */
	slap_client_keepalive(ld, &li->li_tls.sb_keepalive);

#ifdef HAVE_TLS
	if ( LDAP_BACK_CONN_ISPRIV( lc ) ) {
		/* See "rationale" comment in ldap_back_getconn() */
		if ( li->li_acl_authmethod == LDAP_AUTH_NONE &&
			 li->li_idassert_authmethod != LDAP_AUTH_NONE )
			sb = &li->li_idassert.si_bc;
		else
			sb = &li->li_acl;

	} else if ( LDAP_BACK_CONN_ISIDASSERT( lc ) ) {
		sb = &li->li_idassert.si_bc;

	} else {
		sb = &li->li_tls;
	}

	if ( sb->sb_tls_do_init ) {
		bindconf_tls_set( sb, ld );
	} else if ( sb->sb_tls_ctx ) {
		ldap_set_option( ld, LDAP_OPT_X_TLS_CTX, sb->sb_tls_ctx );
	}

	/* if required by the bindconf configuration, force TLS */
	if ( ( sb == &li->li_acl || sb == &li->li_idassert.si_bc ) &&
		sb->sb_tls_ctx )
	{
		flags |= LDAP_BACK_F_USE_TLS;
	}

	ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
	assert( li->li_uri_mutex_do_not_lock == 0 );
	li->li_uri_mutex_do_not_lock = 1;
	rs->sr_err = ldap_back_start_tls( ld, op->o_protocol, &is_tls,
			li->li_uri, flags, li->li_nretries, &rs->sr_text );
	li->li_uri_mutex_do_not_lock = 0;
	ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		ldap_unbind_ext( ld, NULL, NULL );
		rs->sr_text = "Start TLS failed";
		goto error_return;

	} else if ( li->li_idle_timeout ) {
		/* only touch when activity actually took place... */
		lctime = op->o_time;
	}
#endif /* HAVE_TLS */

	lc->lc_ld = ld;
	lc->lc_refcnt = 1;
#ifdef HAVE_TLS
	if ( is_tls ) {
		LDAP_BACK_CONN_ISTLS_SET( lc );
	} else {
		LDAP_BACK_CONN_ISTLS_CLEAR( lc );
	}
	if ( lctime != (time_t)(-1) ) {
		lc->lc_time = lctime;
	}
#endif /* HAVE_TLS */

error_return:;
	if ( rs->sr_err != LDAP_SUCCESS ) {
		rs->sr_err = slap_map_api2result( rs );
		if ( sendok & LDAP_BACK_SENDERR ) {
			if ( rs->sr_text == NULL ) {
				rs->sr_text = "Proxy connection initialization failed";
			}
			send_ldap_result( op, rs );
		}

	} else {
		if ( li->li_conn_ttl > 0 ) {
			lc->lc_create_time = op->o_time;
		}
	}

	return rs->sr_err;
}

static ldapconn_t *
ldap_back_getconn(
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	struct berval		*binddn,
	struct berval		*bindcred )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;
	ldapconn_t	*lc = NULL,
			lc_curr = {{ 0 }};
	int		refcnt = 1,
			lookupconn = !( sendok & LDAP_BACK_BINDING );

	/* if the server is quarantined, and
	 * - the current interval did not expire yet, or
	 * - no more retries should occur,
	 * don't return the connection */
	if ( li->li_isquarantined ) {
		slap_retry_info_t	*ri = &li->li_quarantine;
		int			dont_retry = 1;

		if ( li->li_quarantine.ri_interval ) {
			ldap_pvt_thread_mutex_lock( &li->li_quarantine_mutex );
			if ( li->li_isquarantined == LDAP_BACK_FQ_YES ) {
				dont_retry = ( ri->ri_num[ ri->ri_idx ] == SLAP_RETRYNUM_TAIL
					|| slap_get_time() < ri->ri_last + ri->ri_interval[ ri->ri_idx ] );
				if ( !dont_retry ) {
					Debug( LDAP_DEBUG_ANY,
						"%s: ldap_back_getconn quarantine "
						"retry block #%d try #%d.\n",
						op->o_log_prefix, ri->ri_idx, ri->ri_count );
					li->li_isquarantined = LDAP_BACK_FQ_RETRYING;
				}
			}
			ldap_pvt_thread_mutex_unlock( &li->li_quarantine_mutex );
		}

		if ( dont_retry ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			if ( op->o_conn && ( sendok & LDAP_BACK_SENDERR ) ) {
				rs->sr_text = "Target is quarantined";
				send_ldap_result( op, rs );
			}
			return NULL;
		}
	}

	/* Internal searches are privileged and shared. So is root. */
	if ( op->o_do_not_cache || be_isroot( op ) ) {
		LDAP_BACK_CONN_ISPRIV_SET( &lc_curr );
		lc_curr.lc_local_ndn = op->o_bd->be_rootndn;
		LDAP_BACK_PCONN_ROOTDN_SET( &lc_curr, op );

	} else {
		struct berval	tmpbinddn,
				tmpbindcred,
				save_o_dn,
				save_o_ndn;
		int		isproxyauthz;

		/* need cleanup */
		if ( binddn == NULL ) {
			binddn = &tmpbinddn;
		}	
		if ( bindcred == NULL ) {
			bindcred = &tmpbindcred;
		}
		if ( op->o_tag == LDAP_REQ_BIND ) {
			save_o_dn = op->o_dn;
			save_o_ndn = op->o_ndn;
			op->o_dn = op->o_req_dn;
			op->o_ndn = op->o_req_ndn;
		}
		isproxyauthz = ldap_back_is_proxy_authz( op, rs, sendok, binddn, bindcred );
		if ( op->o_tag == LDAP_REQ_BIND ) {
			op->o_dn = save_o_dn;
			op->o_ndn = save_o_ndn;
		}
		if ( isproxyauthz == -1 ) {
			return NULL;
		}

		lc_curr.lc_local_ndn = op->o_ndn;
		/* Explicit binds must not be shared;
		 * however, explicit binds are piped in a special connection
		 * when idassert is to occur with "override" set */
		if ( op->o_tag == LDAP_REQ_BIND && !isproxyauthz ) {
			lc_curr.lc_conn = op->o_conn;

		} else {
			if ( isproxyauthz && !( sendok & LDAP_BACK_BINDING ) ) {
				lc_curr.lc_local_ndn = *binddn;
				LDAP_BACK_PCONN_ROOTDN_SET( &lc_curr, op );
				LDAP_BACK_CONN_ISIDASSERT_SET( &lc_curr );

			} else if ( isproxyauthz && ( li->li_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) ) {
				lc_curr.lc_local_ndn = slap_empty_bv;
				LDAP_BACK_PCONN_BIND_SET( &lc_curr, op );
				LDAP_BACK_CONN_ISIDASSERT_SET( &lc_curr );
				lookupconn = 1;

			} else if ( SLAP_IS_AUTHZ_BACKEND( op ) ) {
				lc_curr.lc_conn = op->o_conn;

			} else {
				LDAP_BACK_PCONN_ANON_SET( &lc_curr, op );
			}
		}
	}

	/* Explicit Bind requests always get their own conn */
	if ( lookupconn ) {
retry_lock:
		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
		if ( LDAP_BACK_PCONN_ISPRIV( &lc_curr ) ) {
			/* lookup a conn that's not binding */
			LDAP_TAILQ_FOREACH( lc,
				&li->li_conn_priv[ LDAP_BACK_CONN2PRIV( &lc_curr ) ].lic_priv,
				lc_q )
			{
				if ( !LDAP_BACK_CONN_BINDING( lc ) && lc->lc_refcnt == 0 ) {
					break;
				}
			}

			if ( lc != NULL ) {
				if ( lc != LDAP_TAILQ_LAST( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_priv,
					ldapconn_t, lc_q ) )
				{
					LDAP_TAILQ_REMOVE( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_priv,
						lc, lc_q );
					LDAP_TAILQ_ENTRY_INIT( lc, lc_q );
					LDAP_TAILQ_INSERT_TAIL( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_priv,
						lc, lc_q );
				}

			} else if ( !LDAP_BACK_USE_TEMPORARIES( li )
				&& li->li_conn_priv[ LDAP_BACK_CONN2PRIV( &lc_curr ) ].lic_num == li->li_conn_priv_max )
			{
				lc = LDAP_TAILQ_FIRST( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( &lc_curr ) ].lic_priv );
			}
			
		} else {

			/* Searches for a ldapconn in the avl tree */
			lc = (ldapconn_t *)avl_find( li->li_conninfo.lai_tree, 
					(caddr_t)&lc_curr, ldap_back_conndn_cmp );
		}

		if ( lc != NULL ) {
			/* Don't reuse connections while they're still binding */
			if ( LDAP_BACK_CONN_BINDING( lc ) ) {
				if ( !LDAP_BACK_USE_TEMPORARIES( li ) ) {
					ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

					ldap_pvt_thread_yield();
					goto retry_lock;
				}
				lc = NULL;
			}

			if ( lc != NULL ) {
				if ( op->o_tag == LDAP_REQ_BIND ) {
					/* right now, this is the only possible case */
					assert( ( li->li_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) );
					LDAP_BACK_CONN_BINDING_SET( lc );
				}

				refcnt = ++lc->lc_refcnt;
			}
		}
		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
	}

	/* Looks like we didn't get a bind. Open a new session... */
	if ( lc == NULL ) {
		lc = (ldapconn_t *)ch_calloc( 1, sizeof( ldapconn_t ) );
		lc->lc_flags = li->li_flags;
		lc->lc_lcflags = lc_curr.lc_lcflags;
		if ( ldap_back_prepare_conn( lc, op, rs, sendok ) != LDAP_SUCCESS ) {
			ch_free( lc );
			return NULL;
		}

		if ( sendok & LDAP_BACK_BINDING ) {
			LDAP_BACK_CONN_BINDING_SET( lc );
		}

		lc->lc_conn = lc_curr.lc_conn;
		ber_dupbv( &lc->lc_local_ndn, &lc_curr.lc_local_ndn );

		/*
		 * the rationale is: connections as the rootdn are privileged,
		 * so li_acl is to be used; however, in some cases
		 * one already configured identity assertion with a highly
		 * privileged idassert_authcDN, so if li_acl is not configured
		 * and idassert is, use idassert instead.
		 *
		 * might change in the future, because it's preferable
		 * to make clear what identity is being used, since
		 * the only drawback is that one risks to configure
		 * the same identity twice...
		 */
		if ( LDAP_BACK_CONN_ISPRIV( &lc_curr ) ) {
			if ( li->li_acl_authmethod == LDAP_AUTH_NONE &&
				 li->li_idassert_authmethod != LDAP_AUTH_NONE ) {
				ber_dupbv( &lc->lc_bound_ndn, &li->li_idassert_authcDN );
				ber_dupbv( &lc->lc_cred, &li->li_idassert_passwd );

			} else {
				ber_dupbv( &lc->lc_bound_ndn, &li->li_acl_authcDN );
				ber_dupbv( &lc->lc_cred, &li->li_acl_passwd );
			}
			LDAP_BACK_CONN_ISPRIV_SET( lc );

		} else if ( LDAP_BACK_CONN_ISIDASSERT( &lc_curr ) ) {
			if ( !LDAP_BACK_PCONN_ISBIND( &lc_curr ) ) {
				ber_dupbv( &lc->lc_bound_ndn, &li->li_idassert_authcDN );
				ber_dupbv( &lc->lc_cred, &li->li_idassert_passwd );
			}
			LDAP_BACK_CONN_ISIDASSERT_SET( lc );

		} else {
			BER_BVZERO( &lc->lc_cred );
			BER_BVZERO( &lc->lc_bound_ndn );
			if ( !BER_BVISEMPTY( &op->o_ndn )
				&& SLAP_IS_AUTHZ_BACKEND( op ) )
			{
				ber_dupbv( &lc->lc_bound_ndn, &op->o_ndn );
			}
		}

#ifdef HAVE_TLS
		/* if start TLS failed but it was not mandatory,
		 * check if the non-TLS connection was already
		 * in cache; in case, destroy the newly created
		 * connection and use the existing one */
		if ( LDAP_BACK_PCONN_ISTLS( lc ) 
				&& !ldap_tls_inplace( lc->lc_ld ) )
		{
			ldapconn_t	*tmplc = NULL;
			int		idx = LDAP_BACK_CONN2PRIV( &lc_curr ) - 1;
			
			ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
			LDAP_TAILQ_FOREACH( tmplc,
				&li->li_conn_priv[ idx ].lic_priv,
				lc_q )
			{
				if ( !LDAP_BACK_CONN_BINDING( tmplc ) ) {
					break;
				}
			}

			if ( tmplc != NULL ) {
				refcnt = ++tmplc->lc_refcnt;
				ldap_back_conn_free( lc );
				lc = tmplc;
			}
			ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

			if ( tmplc != NULL ) {
				goto done;
			}
		}
#endif /* HAVE_TLS */

		/* Inserts the newly created ldapconn in the avl tree */
		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );

		LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
		lc->lc_connid = li->li_conn_nextid++;

		assert( lc->lc_refcnt == 1 );

#if LDAP_BACK_PRINT_CONNTREE > 0
		ldap_back_print_conntree( li, ">>> ldap_back_getconn(insert)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
	
		if ( LDAP_BACK_PCONN_ISPRIV( lc ) ) {
			if ( li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_num < li->li_conn_priv_max ) {
				LDAP_TAILQ_INSERT_TAIL( &li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_priv, lc, lc_q );
				li->li_conn_priv[ LDAP_BACK_CONN2PRIV( lc ) ].lic_num++;
				LDAP_BACK_CONN_CACHED_SET( lc );

			} else {
				LDAP_BACK_CONN_TAINTED_SET( lc );
			}
			rs->sr_err = 0;

		} else {
			rs->sr_err = avl_insert( &li->li_conninfo.lai_tree, (caddr_t)lc,
				ldap_back_conndn_cmp, ldap_back_conndn_dup );
			LDAP_BACK_CONN_CACHED_SET( lc );
		}

#if LDAP_BACK_PRINT_CONNTREE > 0
		ldap_back_print_conntree( li, "<<< ldap_back_getconn(insert)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
	
		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char	buf[ SLAP_TEXT_BUFLEN ];

			snprintf( buf, sizeof( buf ),
				"lc=%p inserted refcnt=%u rc=%d",
				(void *)lc, refcnt, rs->sr_err );
				
			Debug( LDAP_DEBUG_TRACE,
				"=>ldap_back_getconn: %s: %s\n",
				op->o_log_prefix, buf, 0 );
		}
	
		if ( !LDAP_BACK_PCONN_ISPRIV( lc ) ) {
			/* Err could be -1 in case a duplicate ldapconn is inserted */
			switch ( rs->sr_err ) {
			case 0:
				break;

			case -1:
				LDAP_BACK_CONN_CACHED_CLEAR( lc );
				if ( !( sendok & LDAP_BACK_BINDING ) && !LDAP_BACK_USE_TEMPORARIES( li ) ) {
					/* duplicate: free and try to get the newly created one */
					ldap_back_conn_free( lc );
					lc = NULL;
					goto retry_lock;
				}

				/* taint connection, so that it'll be freed when released */
				LDAP_BACK_CONN_TAINTED_SET( lc );
				break;

			default:
				LDAP_BACK_CONN_CACHED_CLEAR( lc );
				ldap_back_conn_free( lc );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "Proxy bind collision";
				if ( op->o_conn && ( sendok & LDAP_BACK_SENDERR ) ) {
					send_ldap_result( op, rs );
				}
				return NULL;
			}
		}

	} else {
		int	expiring = 0;

		if ( ( li->li_idle_timeout != 0 && op->o_time > lc->lc_time + li->li_idle_timeout )
			|| ( li->li_conn_ttl != 0 && op->o_time > lc->lc_create_time + li->li_conn_ttl ) )
		{
			expiring = 1;

			/* let it be used, but taint/delete it so that 
			 * no-one else can look it up any further */
			ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );

#if LDAP_BACK_PRINT_CONNTREE > 0
			ldap_back_print_conntree( li, ">>> ldap_back_getconn(timeout)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

			(void)ldap_back_conn_delete( li, lc );
			LDAP_BACK_CONN_TAINTED_SET( lc );

#if LDAP_BACK_PRINT_CONNTREE > 0
			ldap_back_print_conntree( li, "<<< ldap_back_getconn(timeout)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

			ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
		}

		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			char	buf[ SLAP_TEXT_BUFLEN ];

			snprintf( buf, sizeof( buf ),
				"conn %p fetched refcnt=%u%s",
				(void *)lc, refcnt,
				expiring ? " expiring" : "" );
			Debug( LDAP_DEBUG_TRACE,
				"=>ldap_back_getconn: %s.\n", buf, 0, 0 );
		}
	}

#ifdef HAVE_TLS
done:;
#endif /* HAVE_TLS */

	return lc;
}

void
ldap_back_release_conn_lock(
	ldapinfo_t		*li,
	ldapconn_t		**lcp,
	int			dolock )
{

	ldapconn_t	*lc = *lcp;

	if ( dolock ) {
		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
	}
	assert( lc->lc_refcnt > 0 );
	LDAP_BACK_CONN_BINDING_CLEAR( lc );
	lc->lc_refcnt--;
	if ( LDAP_BACK_CONN_TAINTED( lc ) ) {
		ldap_back_freeconn( li, lc, 0 );
		*lcp = NULL;
	}
	if ( dolock ) {
		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
	}
}

void
ldap_back_quarantine(
	Operation	*op,
	SlapReply	*rs )
{
	ldapinfo_t		*li = (ldapinfo_t *)op->o_bd->be_private;

	slap_retry_info_t	*ri = &li->li_quarantine;

	ldap_pvt_thread_mutex_lock( &li->li_quarantine_mutex );

	if ( rs->sr_err == LDAP_UNAVAILABLE ) {
		time_t		new_last = slap_get_time();

		switch ( li->li_isquarantined ) {
		case LDAP_BACK_FQ_NO:
			if ( ri->ri_last == new_last ) {
				goto done;
			}

			Debug( LDAP_DEBUG_ANY,
				"%s: ldap_back_quarantine enter.\n",
				op->o_log_prefix, 0, 0 );

			ri->ri_idx = 0;
			ri->ri_count = 0;
			break;

		case LDAP_BACK_FQ_RETRYING:
			Debug( LDAP_DEBUG_ANY,
				"%s: ldap_back_quarantine block #%d try #%d failed.\n",
				op->o_log_prefix, ri->ri_idx, ri->ri_count );

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

		li->li_isquarantined = LDAP_BACK_FQ_YES;
		ri->ri_last = new_last;

	} else if ( li->li_isquarantined != LDAP_BACK_FQ_NO ) {
		if ( ri->ri_last == slap_get_time() ) {
			goto done;
		}

		Debug( LDAP_DEBUG_ANY,
			"%s: ldap_back_quarantine exit (%d) err=%d.\n",
			op->o_log_prefix, li->li_isquarantined, rs->sr_err );

		if ( li->li_quarantine_f ) {
			(void)li->li_quarantine_f( li, li->li_quarantine_p );
		}

		ri->ri_count = 0;
		ri->ri_idx = 0;
		li->li_isquarantined = LDAP_BACK_FQ_NO;
	}

done:;
	ldap_pvt_thread_mutex_unlock( &li->li_quarantine_mutex );
}

static int
ldap_back_dobind_cb(
	Operation *op,
	SlapReply *rs
)
{
	ber_tag_t *tptr = op->o_callback->sc_private;
	op->o_tag = *tptr;
	rs->sr_tag = slap_req2res( op->o_tag );

	return SLAP_CB_CONTINUE;
}

/*
 * ldap_back_dobind_int
 *
 * Note: dolock indicates whether li->li_conninfo.lai_mutex must be locked or not
 */
static int
ldap_back_dobind_int(
	ldapconn_t		**lcp,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	int			retries,
	int			dolock )
{	
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;

	ldapconn_t	*lc;
	struct berval	binddn = slap_empty_bv,
			bindcred = slap_empty_bv;

	int		rc = 0,
			isbound,
			binding = 0;
	ber_int_t	msgid;
	ber_tag_t	o_tag = op->o_tag;
	slap_callback cb = {0};
	char		*tmp_dn;

	assert( lcp != NULL );
	assert( retries >= 0 );

	if ( sendok & LDAP_BACK_GETCONN ) {
		assert( *lcp == NULL );

		lc = ldap_back_getconn( op, rs, sendok, &binddn, &bindcred );
		if ( lc == NULL ) {
			return 0;
		}
		*lcp = lc;

	} else {
		lc = *lcp;
	}

	assert( lc != NULL );

retry_lock:;
 	if ( dolock ) {
 		ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
 	}

 	if ( binding == 0 ) {
		/* check if already bound */
		rc = isbound = LDAP_BACK_CONN_ISBOUND( lc );
		if ( isbound ) {
 			if ( dolock ) {
 				ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
 			}
			return rc;
		}

		if ( LDAP_BACK_CONN_BINDING( lc ) ) {
			/* if someone else is about to bind it, give up and retry */
 			if ( dolock ) {
 				ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
 			}
			ldap_pvt_thread_yield();
			goto retry_lock;

		} else {
			/* otherwise this thread will bind it */
 			LDAP_BACK_CONN_BINDING_SET( lc );
			binding = 1;
		}
	}

 	if ( dolock ) {
 		ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
 	}

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
	 * and the "idassert-authcDN" (or other ID) is set, 
	 * then bind as the asserting identity and explicitly 
	 * add the proxyAuthz control to every operation with the
	 * dn bound to the connection as control value.
	 * This is done also if this is the authorizing backend,
	 * but the "override" flag is given to idassert.
	 * It allows to use SASL bind and yet proxyAuthz users
	 */
	op->o_tag = LDAP_REQ_BIND;
	cb.sc_next = op->o_callback;
	cb.sc_private = &o_tag;
	cb.sc_response = ldap_back_dobind_cb;
	op->o_callback = &cb;

	if ( LDAP_BACK_CONN_ISIDASSERT( lc ) ) {
		if ( BER_BVISEMPTY( &binddn ) && BER_BVISEMPTY( &bindcred ) ) {
			/* if we got here, it shouldn't return result */
			rc = ldap_back_is_proxy_authz( op, rs,
				LDAP_BACK_DONTSEND, &binddn, &bindcred );
			if ( rc != 1 ) {
				Debug( LDAP_DEBUG_ANY, "Error: ldap_back_is_proxy_authz "
					"returned %d, misconfigured URI?\n", rc, 0, 0 );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "misconfigured URI?";
				LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				goto done;
			}
		}
		rc = ldap_back_proxy_authz_bind( lc, op, rs, sendok, &binddn, &bindcred );
		goto done;
	}

#ifdef HAVE_CYRUS_SASL
	if ( LDAP_BACK_CONN_ISPRIV( lc )) {
	slap_bindconf *sb;
	if ( li->li_acl_authmethod != LDAP_AUTH_NONE )
		sb = &li->li_acl;
	else
		sb = &li->li_idassert.si_bc;

	if ( sb->sb_method == LDAP_AUTH_SASL ) {
		void		*defaults = NULL;

		if ( sb->sb_secprops != NULL ) {
			rc = ldap_set_option( lc->lc_ld,
				LDAP_OPT_X_SASL_SECPROPS, sb->sb_secprops );

			if ( rc != LDAP_OPT_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "Error: ldap_set_option "
					"(SECPROPS,\"%s\") failed!\n",
					sb->sb_secprops, 0, 0 );
				goto done;
			}
		}

		defaults = lutil_sasl_defaults( lc->lc_ld,
				sb->sb_saslmech.bv_val,
				sb->sb_realm.bv_val,
				sb->sb_authcId.bv_val,
				sb->sb_cred.bv_val,
				NULL );
		if ( defaults == NULL ) {
			rs->sr_err = LDAP_OTHER;
			LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			goto done;
		}

		rs->sr_err = ldap_sasl_interactive_bind_s( lc->lc_ld,
				sb->sb_binddn.bv_val,
				sb->sb_saslmech.bv_val, NULL, NULL,
				LDAP_SASL_QUIET, lutil_sasl_interact,
				defaults );

		ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
		ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_BIND ], 1 );
		ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

		lutil_sasl_freedefs( defaults );

		switch ( rs->sr_err ) {
		case LDAP_SUCCESS:
			LDAP_BACK_CONN_ISBOUND_SET( lc );
			break;

		case LDAP_LOCAL_ERROR:
			/* list client API error codes that require
			 * to taint the connection */
			/* FIXME: should actually retry? */
			LDAP_BACK_CONN_TAINTED_SET( lc );

			/* fallthru */

		default:
			LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
			rs->sr_err = slap_map_api2result( rs );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			break;
		}

		if ( LDAP_BACK_QUARANTINE( li ) ) {
			ldap_back_quarantine( op, rs );
		}

		goto done;
	}
	}
#endif /* HAVE_CYRUS_SASL */

retry:;
	if ( BER_BVISNULL( &lc->lc_cred ) ) {
		tmp_dn = "";
		if ( !BER_BVISNULL( &lc->lc_bound_ndn ) && !BER_BVISEMPTY( &lc->lc_bound_ndn ) ) {
			Debug( LDAP_DEBUG_ANY, "%s ldap_back_dobind_int: DN=\"%s\" without creds, binding anonymously",
				op->o_log_prefix, lc->lc_bound_ndn.bv_val, 0 );
		}

	} else {
		tmp_dn = lc->lc_bound_ndn.bv_val;
	}
	rs->sr_err = ldap_sasl_bind( lc->lc_ld,
			tmp_dn,
			LDAP_SASL_SIMPLE, &lc->lc_cred,
			NULL, NULL, &msgid );

	ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
	ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_BIND ], 1 );
	ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

	if ( rs->sr_err == LDAP_SERVER_DOWN ) {
		if ( retries != LDAP_BACK_RETRY_NEVER ) {
			if ( dolock ) {
				ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
			}

			assert( lc->lc_refcnt > 0 );
			if ( lc->lc_refcnt == 1 ) {
				ldap_unbind_ext( lc->lc_ld, NULL, NULL );
				lc->lc_ld = NULL;

				/* lc here must be the regular lc, reset and ready for init */
				rs->sr_err = ldap_back_prepare_conn( lc, op, rs, sendok );
				if ( rs->sr_err != LDAP_SUCCESS ) {
					sendok &= ~LDAP_BACK_SENDERR;
					lc->lc_refcnt = 0;
				}
			}

			if ( dolock ) {
				ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );
			}

			if ( rs->sr_err == LDAP_SUCCESS ) {
				if ( retries > 0 ) {
					retries--;
				}
				goto retry;
			}
		}

		assert( lc->lc_refcnt == 1 );
		lc->lc_refcnt = 0;
		ldap_back_freeconn( li, lc, dolock );
		*lcp = NULL;
		rs->sr_err = slap_map_api2result( rs );

		if ( LDAP_BACK_QUARANTINE( li ) ) {
			ldap_back_quarantine( op, rs );
		}

		if ( rs->sr_err != LDAP_SUCCESS &&
			( sendok & LDAP_BACK_SENDERR ) )
		{
			if ( op->o_callback == &cb )
				op->o_callback = cb.sc_next;
			op->o_tag = o_tag;
			rs->sr_text = "Proxy can't contact remote server";
			send_ldap_result( op, rs );
		}

		rc = 0;
		goto func_leave;
	}

	rc = ldap_back_op_result( lc, op, rs, msgid,
		-1, ( sendok | LDAP_BACK_BINDING ) );
	if ( rc == LDAP_SUCCESS ) {
		LDAP_BACK_CONN_ISBOUND_SET( lc );
	}

done:;
	LDAP_BACK_CONN_BINDING_CLEAR( lc );
	rc = LDAP_BACK_CONN_ISBOUND( lc );
	if ( !rc ) {
		ldap_back_release_conn_lock( li, lcp, dolock );

	} else if ( LDAP_BACK_SAVECRED( li ) ) {
		ldap_set_rebind_proc( lc->lc_ld, li->li_rebind_f, lc );
	}

func_leave:;
	if ( op->o_callback == &cb )
		op->o_callback = cb.sc_next;
	op->o_tag = o_tag;

	return rc;
}

/*
 * ldap_back_dobind
 *
 * Note: dolock indicates whether li->li_conninfo.lai_mutex must be locked or not
 */
int
ldap_back_dobind( ldapconn_t **lcp, Operation *op, SlapReply *rs, ldap_back_send_t sendok )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;

	return ldap_back_dobind_int( lcp, op, rs,
		( sendok | LDAP_BACK_GETCONN ), li->li_nretries, 1 );
}

/*
 * ldap_back_default_rebind
 *
 * This is a callback used for chasing referrals using the same
 * credentials as the original user on this session.
 */
int 
ldap_back_default_rebind( LDAP *ld, LDAP_CONST char *url, ber_tag_t request,
	ber_int_t msgid, void *params )
{
	ldapconn_t	*lc = (ldapconn_t *)params;

#ifdef HAVE_TLS
	/* ... otherwise we couldn't get here */
	assert( lc != NULL );

	if ( !ldap_tls_inplace( ld ) ) {
		int		is_tls = LDAP_BACK_CONN_ISTLS( lc ),
				rc;
		const char	*text = NULL;

		rc = ldap_back_start_tls( ld, 0, &is_tls, url, lc->lc_flags,
			LDAP_BACK_RETRY_DEFAULT, &text );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}
#endif /* HAVE_TLS */

	/* FIXME: add checks on the URL/identity? */
	/* TODO: would like to count this bind operation for monitoring
	 * too, but where do we get the ldapinfo_t? */

	return ldap_sasl_bind_s( ld,
			BER_BVISNULL( &lc->lc_cred ) ? "" : lc->lc_bound_ndn.bv_val,
			LDAP_SASL_SIMPLE, &lc->lc_cred, NULL, NULL, NULL );
}

/*
 * ldap_back_default_urllist
 */
int 
ldap_back_default_urllist(
	LDAP		*ld,
	LDAPURLDesc	**urllist,
	LDAPURLDesc	**url,
	void		*params )
{
	ldapinfo_t	*li = (ldapinfo_t *)params;
	LDAPURLDesc	**urltail;

	if ( urllist == url ) {
		return LDAP_SUCCESS;
	}

	for ( urltail = &(*url)->lud_next; *urltail; urltail = &(*urltail)->lud_next )
		/* count */ ;

	*urltail = *urllist;
	*urllist = *url;
	*url = NULL;

	if ( !li->li_uri_mutex_do_not_lock ) {
		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
	}

	if ( li->li_uri ) {
		ch_free( li->li_uri );
	}

	ldap_get_option( ld, LDAP_OPT_URI, (void *)&li->li_uri );

	if ( !li->li_uri_mutex_do_not_lock ) {
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );
	}

	return LDAP_SUCCESS;
}

int
ldap_back_cancel(
		ldapconn_t		*lc,
		Operation		*op,
		SlapReply		*rs,
		ber_int_t		msgid,
		ldap_back_send_t	sendok )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;

	/* default behavior */
	if ( LDAP_BACK_ABANDON( li ) ) {
		return ldap_abandon_ext( lc->lc_ld, msgid, NULL, NULL );
	}

	if ( LDAP_BACK_IGNORE( li ) ) {
		return ldap_pvt_discard( lc->lc_ld, msgid );
	}

	if ( LDAP_BACK_CANCEL( li ) ) {
		/* FIXME: asynchronous? */
		return ldap_cancel_s( lc->lc_ld, msgid, NULL, NULL );
	}

	assert( 0 );

	return LDAP_OTHER;
}

int
ldap_back_op_result(
		ldapconn_t		*lc,
		Operation		*op,
		SlapReply		*rs,
		ber_int_t		msgid,
		time_t			timeout,
		ldap_back_send_t	sendok )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;

	char		*match = NULL;
	char		*text = NULL;
	char		**refs = NULL;
	LDAPControl	**ctrls = NULL;

	rs->sr_text = NULL;
	rs->sr_matched = NULL;
	rs->sr_ref = NULL;
	rs->sr_ctrls = NULL;

	/* if the error recorded in the reply corresponds
	 * to a successful state, get the error from the
	 * remote server response */
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
				timeout = li->li_timeout[ opidx ];
			}
		}

		/* better than nothing :) */
		if ( timeout == 0 ) {
			if ( li->li_idle_timeout ) {
				timeout = li->li_idle_timeout;

			} else if ( li->li_conn_ttl ) {
				timeout = li->li_conn_ttl;
			}
		}

		if ( timeout ) {
			stoptime = op->o_time + timeout;
		}

		LDAP_BACK_TV_SET( &tv );

retry:;
		/* if result parsing fails, note the failure reason */
		rc = ldap_result( lc->lc_ld, msgid, LDAP_MSG_ALL, &tv, &res );
		switch ( rc ) {
		case 0:
			if ( timeout && slap_get_time() > stoptime ) {
				if ( sendok & LDAP_BACK_BINDING ) {
					ldap_unbind_ext( lc->lc_ld, NULL, NULL );
					lc->lc_ld = NULL;

					/* let it be used, but taint/delete it so that 
					 * no-one else can look it up any further */
					ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );

#if LDAP_BACK_PRINT_CONNTREE > 0
					ldap_back_print_conntree( li, ">>> ldap_back_getconn(timeout)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */

					(void)ldap_back_conn_delete( li, lc );
					LDAP_BACK_CONN_TAINTED_SET( lc );

#if LDAP_BACK_PRINT_CONNTREE > 0
					ldap_back_print_conntree( li, "<<< ldap_back_getconn(timeout)" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
					ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

				} else {
					(void)ldap_back_cancel( lc, op, rs, msgid, sendok );
				}
				rs->sr_err = timeout_err;
				rs->sr_text = timeout_text;
				break;
			}

			/* timeout == 0 */
			LDAP_BACK_TV_SET( &tv );
			ldap_pvt_thread_yield();
			goto retry;

		case -1:
			ldap_get_option( lc->lc_ld, LDAP_OPT_ERROR_NUMBER,
					&rs->sr_err );
			break;


		/* otherwise get the result; if it is not
		 * LDAP_SUCCESS, record it in the reply
		 * structure (this includes 
		 * LDAP_COMPARE_{TRUE|FALSE}) */
		default:
			/* only touch when activity actually took place... */
			if ( li->li_idle_timeout && lc ) {
				lc->lc_time = op->o_time;
			}

			rc = ldap_parse_result( lc->lc_ld, res, &rs->sr_err,
					&match, &text, &refs, &ctrls, 1 );
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
						"%s ldap_back_op_result: "
						"got referrals with err=%d\n",
						op->o_log_prefix,
						rs->sr_err, 0 );

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
					"%s ldap_back_op_result: "
					"got err=%d with null "
					"or empty referrals\n",
					op->o_log_prefix,
					rs->sr_err, 0 );

				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}

			if ( ctrls != NULL ) {
				rs->sr_ctrls = ctrls;
			}
		}
	}

	/* if the error in the reply structure is not
	 * LDAP_SUCCESS, try to map it from client 
	 * to server error */
	if ( !LDAP_ERR_OK( rs->sr_err ) ) {
		rs->sr_err = slap_map_api2result( rs );

		/* internal ops ( op->o_conn == NULL ) 
		 * must not reply to client */
		if ( op->o_conn && !op->o_do_not_cache && match ) {

			/* record the (massaged) matched
			 * DN into the reply structure */
			rs->sr_matched = match;
		}
	}

	if ( rs->sr_err == LDAP_UNAVAILABLE ) {
		if ( !( sendok & LDAP_BACK_RETRYING ) ) {
			if ( LDAP_BACK_QUARANTINE( li ) ) {
				ldap_back_quarantine( op, rs );
			}
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

	if ( text ) {
		ldap_memfree( text );
	}
	rs->sr_text = NULL;

	/* there can't be refs with a (successful) bind */
	if ( rs->sr_ref ) {
		op->o_tmpfree( rs->sr_ref, op->o_tmpmemctx );
		rs->sr_ref = NULL;
	}

	if ( refs ) {
		ber_memvfree( (void **)refs );
	}

	/* match should not be possible with a successful bind */
	if ( match ) {
		if ( rs->sr_matched != match ) {
			free( (char *)rs->sr_matched );
		}
		rs->sr_matched = NULL;
		ldap_memfree( match );
	}

	if ( ctrls != NULL ) {
		if ( op->o_tag == LDAP_REQ_BIND && rs->sr_err == LDAP_SUCCESS ) {
			int i;

			for ( i = 0; ctrls[i] != NULL; i++ );

			rs->sr_ctrls = op->o_tmpalloc( sizeof( LDAPControl * )*( i + 1 ),
				op->o_tmpmemctx );
			for ( i = 0; ctrls[ i ] != NULL; i++ ) {
				char *ptr;
				ber_len_t oidlen = strlen( ctrls[i]->ldctl_oid );
				ber_len_t size = sizeof( LDAPControl )
					+ oidlen + 1
					+ ctrls[i]->ldctl_value.bv_len + 1;
	
				rs->sr_ctrls[ i ] = op->o_tmpalloc( size, op->o_tmpmemctx );
				rs->sr_ctrls[ i ]->ldctl_oid = (char *)&rs->sr_ctrls[ i ][ 1 ];
				lutil_strcopy( rs->sr_ctrls[ i ]->ldctl_oid, ctrls[i]->ldctl_oid );
				rs->sr_ctrls[ i ]->ldctl_value.bv_val
						= (char *)&rs->sr_ctrls[ i ]->ldctl_oid[oidlen + 1];
				rs->sr_ctrls[ i ]->ldctl_value.bv_len
					= ctrls[i]->ldctl_value.bv_len;
				ptr = lutil_memcopy( rs->sr_ctrls[ i ]->ldctl_value.bv_val,
					ctrls[i]->ldctl_value.bv_val, ctrls[i]->ldctl_value.bv_len );
				*ptr = '\0';
			}
			rs->sr_ctrls[ i ] = NULL;
			rs->sr_flags |= REP_CTRLS_MUSTBEFREED;

		} else {
			assert( rs->sr_ctrls != NULL );
			rs->sr_ctrls = NULL;
		}

		ldap_controls_free( ctrls );
	}

	return( LDAP_ERR_OK( rs->sr_err ) ? LDAP_SUCCESS : rs->sr_err );
}

/* return true if bound, false if failed */
int
ldap_back_retry( ldapconn_t **lcp, Operation *op, SlapReply *rs, ldap_back_send_t sendok )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;
	int		rc = 0;

	assert( lcp != NULL );
	assert( *lcp != NULL );

	ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );

	if ( (*lcp)->lc_refcnt == 1 ) {
		int binding = LDAP_BACK_CONN_BINDING( *lcp );

		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
		Debug( LDAP_DEBUG_ANY,
			"%s ldap_back_retry: retrying URI=\"%s\" DN=\"%s\"\n",
			op->o_log_prefix, li->li_uri,
			BER_BVISNULL( &(*lcp)->lc_bound_ndn ) ?
				"" : (*lcp)->lc_bound_ndn.bv_val );
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );

		ldap_unbind_ext( (*lcp)->lc_ld, NULL, NULL );
		(*lcp)->lc_ld = NULL;
		LDAP_BACK_CONN_ISBOUND_CLEAR( (*lcp) );

		/* lc here must be the regular lc, reset and ready for init */
		rc = ldap_back_prepare_conn( *lcp, op, rs, sendok );
		if ( rc != LDAP_SUCCESS ) {
			/* freeit, because lc_refcnt == 1 */
			(*lcp)->lc_refcnt = 0;
			(void)ldap_back_freeconn( li, *lcp, 0 );
			*lcp = NULL;
			rc = 0;

		} else if ( ( sendok & LDAP_BACK_BINDING ) ) {
			if ( binding ) {
				LDAP_BACK_CONN_BINDING_SET( *lcp );
			}
			rc = 1;

		} else {
			rc = ldap_back_dobind_int( lcp, op, rs, sendok, 0, 0 );
			if ( rc == 0 && *lcp != NULL ) {
				/* freeit, because lc_refcnt == 1 */
				(*lcp)->lc_refcnt = 0;
				LDAP_BACK_CONN_TAINTED_SET( *lcp );
				(void)ldap_back_freeconn( li, *lcp, 0 );
				*lcp = NULL;
			}
		}

	} else {
		Debug( LDAP_DEBUG_TRACE,
			"ldap_back_retry: conn %p refcnt=%u unable to retry.\n",
			(void *)(*lcp), (*lcp)->lc_refcnt, 0 );

		LDAP_BACK_CONN_TAINTED_SET( *lcp );
		ldap_back_release_conn_lock( li, lcp, 0 );
		assert( *lcp == NULL );

		if ( sendok & LDAP_BACK_SENDERR ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			rs->sr_text = "Unable to retry";
			send_ldap_result( op, rs );
		}
	}

	ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

	return rc;
}

static int
ldap_back_is_proxy_authz( Operation *op, SlapReply *rs, ldap_back_send_t sendok,
	struct berval *binddn, struct berval *bindcred )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;
	struct berval	ndn;
	int		dobind = 0;

	if ( op->o_conn == NULL || op->o_do_not_cache ) {
		goto done;
	}

	/* don't proxyAuthz if protocol is not LDAPv3 */
	switch ( li->li_version ) {
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
			dobind = -1;
		}
		goto done;
	}

	/* safe default */
	*binddn = slap_empty_bv;
	*bindcred = slap_empty_bv;

	if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;

	} else {
		ndn = op->o_ndn;
	}

	if ( !( li->li_idassert_flags & LDAP_BACK_AUTH_OVERRIDE )) {
		if ( op->o_tag == LDAP_REQ_BIND ) {
			if ( !BER_BVISEMPTY( &ndn )) {
				dobind = 0;
				goto done;
			}
		} else if ( SLAP_IS_AUTHZ_BACKEND( op )) {
			dobind = 0;
			goto done;
		}
	}

	switch ( li->li_idassert_mode ) {
	case LDAP_BACK_IDASSERT_LEGACY:
		if ( !BER_BVISNULL( &ndn ) && !BER_BVISEMPTY( &ndn ) ) {
			if ( !BER_BVISNULL( &li->li_idassert_authcDN ) && !BER_BVISEMPTY( &li->li_idassert_authcDN ) )
			{
				*binddn = li->li_idassert_authcDN;
				*bindcred = li->li_idassert_passwd;
				dobind = 1;
			}
		}
		break;

	default:
		/* NOTE: rootdn can always idassert */
		if ( BER_BVISNULL( &ndn )
			&& li->li_idassert_authz == NULL
			&& !( li->li_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
		{
			if ( li->li_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
				rs->sr_err = LDAP_INAPPROPRIATE_AUTH;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
					dobind = -1;
				}

			} else {
				rs->sr_err = LDAP_SUCCESS;
				*binddn = slap_empty_bv;
				*bindcred = slap_empty_bv;
				break;
			}

			goto done;

		} else if ( !be_isroot( op ) ) {
			if ( li->li_idassert_passthru ) {
				struct berval authcDN;

				if ( BER_BVISNULL( &ndn ) ) {
					authcDN = slap_empty_bv;

				} else {
					authcDN = ndn;
				}	
				rs->sr_err = slap_sasl_matches( op, li->li_idassert_passthru,
						&authcDN, &authcDN );
				if ( rs->sr_err == LDAP_SUCCESS ) {
					dobind = 0;
					break;
				}
			}

			if ( li->li_idassert_authz ) {
				struct berval authcDN;

				if ( BER_BVISNULL( &ndn ) ) {
					authcDN = slap_empty_bv;

				} else {
					authcDN = ndn;
				}	
				rs->sr_err = slap_sasl_matches( op, li->li_idassert_authz,
						&authcDN, &authcDN );
				if ( rs->sr_err != LDAP_SUCCESS ) {
					if ( li->li_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
						if ( sendok & LDAP_BACK_SENDERR ) {
							send_ldap_result( op, rs );
							dobind = -1;
						}

					} else {
						rs->sr_err = LDAP_SUCCESS;
						*binddn = slap_empty_bv;
						*bindcred = slap_empty_bv;
						break;
					}

					goto done;
				}
			}
		}

		*binddn = li->li_idassert_authcDN;
		*bindcred = li->li_idassert_passwd;
		dobind = 1;
		break;
	}

done:;
	return dobind;
}

static int
ldap_back_proxy_authz_bind(
	ldapconn_t		*lc,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	struct berval		*binddn,
	struct berval		*bindcred )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;
	struct berval	ndn;
	int		msgid;
	int		rc;

	if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;

	} else {
		ndn = op->o_ndn;
	}

	if ( li->li_idassert_authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void		*defaults = NULL;
		struct berval	authzID = BER_BVNULL;
		int		freeauthz = 0;
		LDAPControl **ctrlsp = NULL;
		LDAPMessage *result = NULL;
		const char *rmech = NULL;
		const char *save_text = rs->sr_text;

#ifdef SLAP_AUTH_DN
		LDAPControl ctrl, *ctrls[2];
		int msgid;
#endif /* SLAP_AUTH_DN */

		/* if SASL supports native authz, prepare for it */
		if ( ( !op->o_do_not_cache || !op->o_is_auth_check ) &&
				( li->li_idassert_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) )
		{
			switch ( li->li_idassert_mode ) {
			case LDAP_BACK_IDASSERT_OTHERID:
			case LDAP_BACK_IDASSERT_OTHERDN:
				authzID = li->li_idassert_authzID;
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

		if ( li->li_idassert_secprops != NULL ) {
			rs->sr_err = ldap_set_option( lc->lc_ld,
				LDAP_OPT_X_SASL_SECPROPS,
				(void *)li->li_idassert_secprops );

			if ( rs->sr_err != LDAP_OPT_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				if ( sendok & LDAP_BACK_SENDERR ) {
					send_ldap_result( op, rs );
				}
				LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
				goto done;
			}
		}

		defaults = lutil_sasl_defaults( lc->lc_ld,
				li->li_idassert_sasl_mech.bv_val,
				li->li_idassert_sasl_realm.bv_val,
				li->li_idassert_authcID.bv_val,
				li->li_idassert_passwd.bv_val,
				authzID.bv_val );
		if ( defaults == NULL ) {
			rs->sr_err = LDAP_OTHER;
			LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			goto done;
		}

#ifdef SLAP_AUTH_DN
		if ( li->li_idassert_flags & LDAP_BACK_AUTH_DN_AUTHZID ) {
			assert( BER_BVISNULL( binddn ) );

			ctrl.ldctl_oid = LDAP_CONTROL_AUTHZID_REQUEST;
			ctrl.ldctl_iscritical = 0;
			BER_BVZERO( &ctrl.ldctl_value );
			ctrls[0] = &ctrl;
			ctrls[1] = NULL;
			ctrlsp = ctrls;
		}
#endif /* SLAP_AUTH_DN */

		do {
			rs->sr_err = ldap_sasl_interactive_bind( lc->lc_ld, binddn->bv_val,
				li->li_idassert_sasl_mech.bv_val, 
				ctrlsp, NULL, LDAP_SASL_QUIET, lutil_sasl_interact, defaults,
				result, &rmech, &msgid );

			if ( rs->sr_err != LDAP_SASL_BIND_IN_PROGRESS )
				break;

			ldap_msgfree( result );

			if ( ldap_result( lc->lc_ld, msgid, LDAP_MSG_ALL, NULL, &result ) == -1 || !result ) {
				ldap_get_option( lc->lc_ld, LDAP_OPT_RESULT_CODE, (void*)&rs->sr_err );
				ldap_get_option( lc->lc_ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&rs->sr_text );
				break;
			}
		} while ( rs->sr_err == LDAP_SASL_BIND_IN_PROGRESS );

		ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
		ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_BIND ], 1 );
		ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

		switch ( rs->sr_err ) {
		case LDAP_SUCCESS:
#ifdef SLAP_AUTH_DN
			/* FIXME: right now, the only reason to check
			 * response controls is RFC 3829 authzid */
			if ( li->li_idassert_flags & LDAP_BACK_AUTH_DN_AUTHZID ) {
				ctrlsp = NULL;
				rc = ldap_parse_result( lc->lc_ld, result, NULL, NULL, NULL, NULL,
					&ctrlsp, 0 );
				if ( rc == LDAP_SUCCESS && ctrlsp ) {
					LDAPControl *ctrl;
		
					ctrl = ldap_control_find( LDAP_CONTROL_AUTHZID_RESPONSE,
						ctrlsp, NULL );
					if ( ctrl ) {
						Debug( LDAP_DEBUG_TRACE, "%s: ldap_back_proxy_authz_bind: authzID=\"%s\" (authzid)\n",
							op->o_log_prefix, ctrl->ldctl_value.bv_val, 0 );
						if ( ctrl->ldctl_value.bv_len > STRLENOF("dn:") &&
							strncasecmp( ctrl->ldctl_value.bv_val, "dn:", STRLENOF("dn:") ) == 0 )
						{
							struct berval bv;
							bv.bv_val = &ctrl->ldctl_value.bv_val[STRLENOF("dn:")];
							bv.bv_len = ctrl->ldctl_value.bv_len - STRLENOF("dn:");
							ber_bvreplace( &lc->lc_bound_ndn, &bv );
						}
					}
				}

				ldap_controls_free( ctrlsp );

			} else if ( li->li_idassert_flags & LDAP_BACK_AUTH_DN_WHOAMI ) {
				struct berval *val = NULL;
				rc = ldap_whoami_s( lc->lc_ld, &val, NULL, NULL );
				if ( rc == LDAP_SUCCESS && val != NULL ) {
					Debug( LDAP_DEBUG_TRACE, "%s: ldap_back_proxy_authz_bind: authzID=\"%s\" (whoami)\n",
						op->o_log_prefix, val->bv_val, 0 );
					if ( val->bv_len > STRLENOF("dn:") &&
						strncasecmp( val->bv_val, "dn:", STRLENOF("dn:") ) == 0 )
					{
						struct berval bv;
						bv.bv_val = &val->bv_val[STRLENOF("dn:")];
						bv.bv_len = val->bv_len - STRLENOF("dn:");
						ber_bvreplace( &lc->lc_bound_ndn, &bv );
					}
					ber_bvfree( val );
				}
			}

			if ( ( li->li_idassert_flags & LDAP_BACK_AUTH_DN_MASK ) &&
				BER_BVISNULL( &lc->lc_bound_ndn ) )
			{
				/* all in all, we only need it to be non-null */
				/* FIXME: should this be configurable? */
				static struct berval bv = BER_BVC("cn=authzdn");
				ber_bvreplace( &lc->lc_bound_ndn, &bv );
			}
#endif /* SLAP_AUTH_DN */
			LDAP_BACK_CONN_ISBOUND_SET( lc );
			break;

		case LDAP_LOCAL_ERROR:
			/* list client API error codes that require
			 * to taint the connection */
			/* FIXME: should actually retry? */
			LDAP_BACK_CONN_TAINTED_SET( lc );

			/* fallthru */

		default:
			LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
			rs->sr_err = slap_map_api2result( rs );
			if ( sendok & LDAP_BACK_SENDERR ) {
				send_ldap_result( op, rs );
			}
			break;
		}

		if ( save_text != rs->sr_text ) {
			ldap_memfree( (char *)rs->sr_text );
			rs->sr_text = save_text;
		}

		ldap_msgfree( result );

		lutil_sasl_freedefs( defaults );
		if ( freeauthz ) {
			slap_sl_free( authzID.bv_val, op->o_tmpmemctx );
		}

		goto done;
#endif /* HAVE_CYRUS_SASL */
	}

	switch ( li->li_idassert_authmethod ) {
	case LDAP_AUTH_NONE:
		/* FIXME: do we really need this? */
		BER_BVSTR( binddn, "" );
		BER_BVSTR( bindcred, "" );
		/* fallthru */

	case LDAP_AUTH_SIMPLE:
		rs->sr_err = ldap_sasl_bind( lc->lc_ld,
				binddn->bv_val, LDAP_SASL_SIMPLE,
				bindcred, NULL, NULL, &msgid );
		rc = ldap_back_op_result( lc, op, rs, msgid,
			-1, ( sendok | LDAP_BACK_BINDING ) );

		ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
		ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_BIND ], 1 );
		ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );
		break;

	default:
		/* unsupported! */
		LDAP_BACK_CONN_ISBOUND_CLEAR( lc );
		rs->sr_err = LDAP_AUTH_METHOD_NOT_SUPPORTED;
		if ( sendok & LDAP_BACK_SENDERR ) {
			send_ldap_result( op, rs );
		}
		goto done;
	}

	if ( rc == LDAP_SUCCESS ) {
		/* set rebind stuff in case of successful proxyAuthz bind,
		 * so that referral chasing is attempted using the right
		 * identity */
		LDAP_BACK_CONN_ISBOUND_SET( lc );
		if ( !BER_BVISNULL( binddn ) ) {
			ber_bvreplace( &lc->lc_bound_ndn, binddn );
		}

		if ( !BER_BVISNULL( &lc->lc_cred ) ) {
			memset( lc->lc_cred.bv_val, 0,
					lc->lc_cred.bv_len );
		}

		if ( LDAP_BACK_SAVECRED( li ) ) {
			if ( !BER_BVISNULL( bindcred ) ) {
				ber_bvreplace( &lc->lc_cred, bindcred );
				ldap_set_rebind_proc( lc->lc_ld, li->li_rebind_f, lc );
			}

		} else {
			lc->lc_cred.bv_len = 0;
		}
	}

done:;
	return LDAP_BACK_CONN_ISBOUND( lc );
}

/*
 * ldap_back_proxy_authz_ctrl() prepends a proxyAuthz control
 * to existing server-side controls if required; if not,
 * the existing server-side controls are placed in *pctrls.
 * The caller, after using the controls in client API 
 * operations, if ( *pctrls != op->o_ctrls ), should
 * free( (*pctrls)[ 0 ] ) and free( *pctrls ).
 * The function returns success if the control could
 * be added if required, or if it did nothing; in the future,
 * it might return some error if it failed.
 * 
 * if no bind took place yet, but the connection is bound
 * and the "proxyauthzdn" is set, then bind as "proxyauthzdn" 
 * and explicitly add proxyAuthz the control to every operation
 * with the dn bound to the connection as control value.
 *
 * If no server-side controls are defined for the operation,
 * simply add the proxyAuthz control; otherwise, if the
 * proxyAuthz control is not already set, add it as
 * the first one
 *
 * FIXME: is controls order significant for security?
 * ANSWER: controls ordering and interoperability
 * must be indicated by the specs of each control; if none
 * is specified, the order is irrelevant.
 */
int
ldap_back_proxy_authz_ctrl(
		Operation	*op,
		SlapReply	*rs,
		struct berval	*bound_ndn,
		int		version,
		slap_idassert_t	*si,
		LDAPControl	*ctrl )
{
	slap_idassert_mode_t	mode;
	struct berval		assertedID,
				ndn;
	int			isroot = 0;

	rs->sr_err = SLAP_CB_CONTINUE;

	/* FIXME: SASL/EXTERNAL over ldapi:// doesn't honor the authcID,
	 * but if it is not set this test fails.  We need a different
	 * means to detect if idassert is enabled */
	if ( ( BER_BVISNULL( &si->si_bc.sb_authcId ) || BER_BVISEMPTY( &si->si_bc.sb_authcId ) )
		&& ( BER_BVISNULL( &si->si_bc.sb_binddn ) || BER_BVISEMPTY( &si->si_bc.sb_binddn ) )
		&& BER_BVISNULL( &si->si_bc.sb_saslmech ) )
	{
		goto done;
	}

	if ( !op->o_conn || op->o_do_not_cache || ( isroot = be_isroot( op ) ) ) {
		goto done;
	}

	if ( op->o_tag == LDAP_REQ_BIND ) {
		ndn = op->o_req_ndn;

	} else if ( !BER_BVISNULL( &op->o_conn->c_ndn ) ) {
		ndn = op->o_conn->c_ndn;

	} else {
		ndn = op->o_ndn;
	}

	if ( si->si_mode == LDAP_BACK_IDASSERT_LEGACY ) {
		if ( op->o_proxy_authz ) {
			/*
			 * FIXME: we do not want to perform proxyAuthz
			 * on behalf of the client, because this would
			 * be performed with "proxyauthzdn" privileges.
			 *
			 * This might actually be too strict, since
			 * the "proxyauthzdn" authzTo, and each entry's
			 * authzFrom attributes may be crafted
			 * to avoid unwanted proxyAuthz to take place.
			 */
#if 0
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "proxyAuthz not allowed within namingContext";
#endif
			goto done;
		}

		if ( !BER_BVISNULL( bound_ndn ) ) {
			goto done;
		}

		if ( BER_BVISNULL( &ndn ) ) {
			goto done;
		}

		if ( BER_BVISNULL( &si->si_bc.sb_binddn ) ) {
			goto done;
		}

	} else if ( si->si_bc.sb_method == LDAP_AUTH_SASL ) {
		if ( ( si->si_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) )
		{
			/* already asserted in SASL via native authz */
			goto done;
		}

	} else if ( si->si_authz && !isroot ) {
		int		rc;
		struct berval authcDN;

		if ( BER_BVISNULL( &ndn ) ) {
			authcDN = slap_empty_bv;
		} else {
			authcDN = ndn;
		}
		rc = slap_sasl_matches( op, si->si_authz,
				&authcDN, &authcDN );
		if ( rc != LDAP_SUCCESS ) {
			if ( si->si_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
				/* ndn is not authorized
				 * to use idassert */
				rs->sr_err = rc;
			}
			goto done;
		}
	}

	if ( op->o_proxy_authz ) {
		/*
		 * FIXME: we can:
		 * 1) ignore the already set proxyAuthz control
		 * 2) leave it in place, and don't set ours
		 * 3) add both
		 * 4) reject the operation
		 *
		 * option (4) is very drastic
		 * option (3) will make the remote server reject
		 * the operation, thus being equivalent to (4)
		 * option (2) will likely break the idassert
		 * assumptions, so we cannot accept it;
		 * option (1) means that we are contradicting
		 * the client's reques.
		 *
		 * I think (4) is the only correct choice.
		 */
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "proxyAuthz not allowed within namingContext";
	}

	if ( op->o_is_auth_check ) {
		mode = LDAP_BACK_IDASSERT_NOASSERT;

	} else {
		mode = si->si_mode;
	}

	switch ( mode ) {
	case LDAP_BACK_IDASSERT_LEGACY:
		/* original behavior:
		 * assert the client's identity */
	case LDAP_BACK_IDASSERT_SELF:
		assertedID = ndn;
		break;

	case LDAP_BACK_IDASSERT_ANONYMOUS:
		/* assert "anonymous" */
		assertedID = slap_empty_bv;
		break;

	case LDAP_BACK_IDASSERT_NOASSERT:
		/* don't assert; bind as proxyauthzdn */
		goto done;

	case LDAP_BACK_IDASSERT_OTHERID:
	case LDAP_BACK_IDASSERT_OTHERDN:
		/* assert idassert DN */
		assertedID = si->si_bc.sb_authzId;
		break;

	default:
		assert( 0 );
	}

	/* if we got here, "" is allowed to proxyAuthz */
	if ( BER_BVISNULL( &assertedID ) ) {
		assertedID = slap_empty_bv;
	}

	/* don't idassert the bound DN (ITS#4497) */
	if ( dn_match( &assertedID, bound_ndn ) ) {
		goto done;
	}

	ctrl->ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
	ctrl->ldctl_iscritical = ( ( si->si_flags & LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL ) == LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL );

	switch ( si->si_mode ) {
	/* already in u:ID or dn:DN form */
	case LDAP_BACK_IDASSERT_OTHERID:
	case LDAP_BACK_IDASSERT_OTHERDN:
		ber_dupbv_x( &ctrl->ldctl_value, &assertedID, op->o_tmpmemctx );
		rs->sr_err = LDAP_SUCCESS;
		break;

	/* needs the dn: prefix */
	default:
		ctrl->ldctl_value.bv_len = assertedID.bv_len + STRLENOF( "dn:" );
		ctrl->ldctl_value.bv_val = op->o_tmpalloc( ctrl->ldctl_value.bv_len + 1,
				op->o_tmpmemctx );
		AC_MEMCPY( ctrl->ldctl_value.bv_val, "dn:", STRLENOF( "dn:" ) );
		AC_MEMCPY( &ctrl->ldctl_value.bv_val[ STRLENOF( "dn:" ) ],
				assertedID.bv_val, assertedID.bv_len + 1 );
		rs->sr_err = LDAP_SUCCESS;
		break;
	}

	/* Older versions of <draft-weltman-ldapv3-proxy> required
	 * to encode the value of the authzID (and called it proxyDN);
	 * this hack provides compatibility with those DSAs that
	 * implement it this way */
	if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
		struct berval		authzID = ctrl->ldctl_value;
		BerElementBuffer	berbuf;
		BerElement		*ber = (BerElement *)&berbuf;
		ber_tag_t		tag;

		ber_init2( ber, 0, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

		tag = ber_printf( ber, "O", &authzID );
		if ( tag == LBER_ERROR ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber;
		}

		if ( ber_flatten2( ber, &ctrl->ldctl_value, 1 ) == -1 ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber;
		}

		rs->sr_err = LDAP_SUCCESS;

free_ber:;
		op->o_tmpfree( authzID.bv_val, op->o_tmpmemctx );
		ber_free_buf( ber );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto done;
		}

	} else if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
		struct berval		authzID = ctrl->ldctl_value,
					tmp;
		BerElementBuffer	berbuf;
		BerElement		*ber = (BerElement *)&berbuf;
		ber_tag_t		tag;

		if ( strncasecmp( authzID.bv_val, "dn:", STRLENOF( "dn:" ) ) != 0 ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		tmp = authzID;
		tmp.bv_val += STRLENOF( "dn:" );
		tmp.bv_len -= STRLENOF( "dn:" );

		ber_init2( ber, 0, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

		/* apparently, Mozilla API encodes this
		 * as "SEQUENCE { LDAPDN }" */
		tag = ber_printf( ber, "{O}", &tmp );
		if ( tag == LBER_ERROR ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber2;
		}

		if ( ber_flatten2( ber, &ctrl->ldctl_value, 1 ) == -1 ) {
			rs->sr_err = LDAP_OTHER;
			goto free_ber2;
		}

		ctrl->ldctl_oid = LDAP_CONTROL_OBSOLETE_PROXY_AUTHZ;
		rs->sr_err = LDAP_SUCCESS;

free_ber2:;
		op->o_tmpfree( authzID.bv_val, op->o_tmpmemctx );
		ber_free_buf( ber );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto done;
		}
	}

done:;

	return rs->sr_err;
}

/*
 * Add controls;
 *
 * if any needs to be added, it is prepended to existing ones,
 * in a newly allocated array.  The companion function
 * ldap_back_controls_free() must be used to restore the original
 * status of op->o_ctrls.
 */
int
ldap_back_controls_add(
		Operation	*op,
		SlapReply	*rs,
		ldapconn_t	*lc,
		LDAPControl	***pctrls )
{
	ldapinfo_t	*li = (ldapinfo_t *)op->o_bd->be_private;

	LDAPControl	**ctrls = NULL;
	/* set to the maximum number of controls this backend can add */
	LDAPControl	c[ 2 ] = { { 0 } };
	int		n = 0, i, j1 = 0, j2 = 0;

	*pctrls = NULL;

	rs->sr_err = LDAP_SUCCESS;

	/* don't add controls if protocol is not LDAPv3 */
	switch ( li->li_version ) {
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
	switch ( ldap_back_proxy_authz_ctrl( op, rs, &lc->lc_bound_ndn,
		li->li_version, &li->li_idassert, &c[ j1 ] ) )
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
	/* FIXME: according to <draft-wahl-ldap-session>, 
	 * the server should check if the control can be added
	 * based on the identity of the client and so */

	/* session tracking */
	if ( LDAP_BACK_ST_REQUEST( li ) ) {
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

int
ldap_back_controls_free( Operation *op, SlapReply *rs, LDAPControl ***pctrls )
{
	LDAPControl	**ctrls = *pctrls;

	/* we assume that the controls added by the proxy come first,
	 * so as soon as we find op->o_ctrls[ 0 ] we can stop */
	if ( ctrls && ctrls != op->o_ctrls ) {
		int		i = 0, n = 0, n_added;
		LDAPControl	*lower, *upper;

		assert( ctrls[ 0 ] != NULL );

		for ( n = 0; ctrls[ n ] != NULL; n++ )
			/* count 'em */ ;

		if ( op->o_ctrls ) {
			for ( i = 0; op->o_ctrls[ i ] != NULL; i++ )
				/* count 'em */ ;
		}

		n_added = n - i;
		lower = (LDAPControl *)&ctrls[ n ];
		upper = &lower[ n_added ];

		for ( i = 0; ctrls[ i ] != NULL; i++ ) {
			if ( ctrls[ i ] < lower || ctrls[ i ] >= upper ) {
				/* original; don't touch */
				continue;
			}

			if ( !BER_BVISNULL( &ctrls[ i ]->ldctl_value ) ) {
				op->o_tmpfree( ctrls[ i ]->ldctl_value.bv_val, op->o_tmpmemctx );
			}
		}

		op->o_tmpfree( ctrls, op->o_tmpmemctx );
	} 

	*pctrls = NULL;

	return 0;
}

int
ldap_back_conn2str( const ldapconn_base_t *lc, char *buf, ber_len_t buflen )
{
	char tbuf[ SLAP_TEXT_BUFLEN ];
	char *ptr = buf, *end = buf + buflen;
	int len;

	if ( ptr + sizeof("conn=") > end ) return -1;
	ptr = lutil_strcopy( ptr, "conn=" );

	len = ldap_back_connid2str( lc, ptr, (ber_len_t)(end - ptr) );
	ptr += len;
	if ( ptr >= end ) return -1;

	if ( !BER_BVISNULL( &lc->lcb_local_ndn ) ) {
		if ( ptr + sizeof(" DN=\"\"") + lc->lcb_local_ndn.bv_len > end ) return -1;
		ptr = lutil_strcopy( ptr, " DN=\"" );
		ptr = lutil_strncopy( ptr, lc->lcb_local_ndn.bv_val, lc->lcb_local_ndn.bv_len );
		*ptr++ = '"';
	}

	if ( lc->lcb_create_time != 0 ) {
		len = snprintf( tbuf, sizeof(tbuf), "%ld", lc->lcb_create_time );
		if ( ptr + sizeof(" created=") + len >= end ) return -1;
		ptr = lutil_strcopy( ptr, " created=" );
		ptr = lutil_strcopy( ptr, tbuf );
	}

	if ( lc->lcb_time != 0 ) {
		len = snprintf( tbuf, sizeof(tbuf), "%ld", lc->lcb_time );
		if ( ptr + sizeof(" modified=") + len >= end ) return -1;
		ptr = lutil_strcopy( ptr, " modified=" );
		ptr = lutil_strcopy( ptr, tbuf );
	}

	len = snprintf( tbuf, sizeof(tbuf), "%u", lc->lcb_refcnt );
	if ( ptr + sizeof(" refcnt=") + len >= end ) return -1;
	ptr = lutil_strcopy( ptr, " refcnt=" );
	ptr = lutil_strcopy( ptr, tbuf );

	return ptr - buf;
}

int
ldap_back_connid2str( const ldapconn_base_t *lc, char *buf, ber_len_t buflen )
{
	static struct berval conns[] = {
		BER_BVC("ROOTDN"),
		BER_BVC("ROOTDN-TLS"),
		BER_BVC("ANON"),
		BER_BVC("ANON-TLS"),
		BER_BVC("BIND"),
		BER_BVC("BIND-TLS"),
		BER_BVNULL
	};

	int len = 0;

	if ( LDAP_BACK_PCONN_ISPRIV( (const ldapconn_t *)lc ) ) {
		long cid;
		struct berval *bv;

		cid = (long)lc->lcb_conn;
		assert( cid >= LDAP_BACK_PCONN_FIRST && cid < LDAP_BACK_PCONN_LAST );

		bv = &conns[ cid ];

		if ( bv->bv_len >= buflen ) {
			return bv->bv_len + 1;
		}

		len = bv->bv_len;
		lutil_strncopy( buf, bv->bv_val, bv->bv_len + 1 );

	} else {
		len = snprintf( buf, buflen, "%lu", lc->lcb_conn->c_connid );
	}

	return len;
}
