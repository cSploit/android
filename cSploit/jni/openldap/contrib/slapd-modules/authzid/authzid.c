/* authzid.c - RFC 3829 Authzid Control */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2010-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

/*
 * RFC 3829 Authzid
 *
 * must be instantiated as a global overlay
 */

#include "portable.h"

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "ac/string.h"

typedef struct authzid_conn_t {
	Connection *conn;
	int refcnt;
	char authzid_flag;
} authzid_conn_t;

static ldap_pvt_thread_mutex_t authzid_mutex;
static Avlnode *authzid_tree;

static int
authzid_conn_cmp( const void *c1, const void *c2 )
{
	const authzid_conn_t *ac1 = (const authzid_conn_t *)c1;
	const authzid_conn_t *ac2 = (const authzid_conn_t *)c2;

	return SLAP_PTRCMP( ac1->conn, ac2->conn );
}

static int
authzid_conn_dup( void *c1, void *c2 )
{
	authzid_conn_t *ac1 = (authzid_conn_t *)c1;
	authzid_conn_t *ac2 = (authzid_conn_t *)c2;

	if ( ac1->conn == ac2->conn ) {
		return -1;
	}

	return 0;
}

static int authzid_cid;
static slap_overinst authzid;

static authzid_conn_t *
authzid_conn_find( Connection *c )
{
	authzid_conn_t *ac = NULL, tmp = { 0 };

	tmp.conn = c;
	ac = (authzid_conn_t *)avl_find( authzid_tree, (caddr_t)&tmp, authzid_conn_cmp );
	if ( ac == NULL || ( ac != NULL && ac->refcnt != 0 ) ) {
		ac = NULL;
	}
	if ( ac ) {
		ac->refcnt++;
	}

	return ac;
}

static authzid_conn_t *
authzid_conn_get( Connection *c )
{
	authzid_conn_t *ac = NULL;

	ldap_pvt_thread_mutex_lock( &authzid_mutex );
	ac = authzid_conn_find( c );
	if ( ac && ac->refcnt ) ac = NULL;
	if ( ac ) ac->refcnt++;
	ldap_pvt_thread_mutex_unlock( &authzid_mutex );

	return ac;
}

static void
authzid_conn_release( authzid_conn_t *ac )
{
	ldap_pvt_thread_mutex_lock( &authzid_mutex );
	ac->refcnt--;
	ldap_pvt_thread_mutex_unlock( &authzid_mutex );
}

static int
authzid_conn_insert( Connection *c, char flag )
{
	authzid_conn_t *ac;
	int rc;

	ldap_pvt_thread_mutex_lock( &authzid_mutex );
	ac = authzid_conn_find( c );
	if ( ac ) {
		ldap_pvt_thread_mutex_unlock( &authzid_mutex );
		return -1;
	}

	ac = SLAP_MALLOC( sizeof( authzid_conn_t ) );
	ac->conn = c;
	ac->refcnt = 0;
	ac->authzid_flag = flag;
	rc = avl_insert( &authzid_tree, (caddr_t)ac,
		authzid_conn_cmp, authzid_conn_dup );
	ldap_pvt_thread_mutex_unlock( &authzid_mutex );

	return rc;
}

static int
authzid_conn_remove( Connection *c )
{
	authzid_conn_t *ac, *tmp;

	ldap_pvt_thread_mutex_lock( &authzid_mutex );
	ac = authzid_conn_find( c );
	if ( !ac ) {
		ldap_pvt_thread_mutex_unlock( &authzid_mutex );
		return -1;
	}
	tmp = avl_delete( &authzid_tree, (caddr_t)ac, authzid_conn_cmp );
	ldap_pvt_thread_mutex_unlock( &authzid_mutex );

	assert( tmp == ac );
	SLAP_FREE( ac );

	return 0;
}

static int
authzid_response(
	Operation *op,
	SlapReply *rs )
{
	LDAPControl **ctrls;
	struct berval edn = BER_BVNULL;
	ber_len_t len = 0;
	int n = 0;

	assert( rs->sr_tag = LDAP_RES_BIND );

	if ( rs->sr_err == LDAP_SASL_BIND_IN_PROGRESS ) {
		authzid_conn_t *ac = op->o_controls[ authzid_cid ];
		if ( ac ) {
			authzid_conn_release( ac );
		} else {
			(void)authzid_conn_insert( op->o_conn, op->o_ctrlflag[ authzid_cid ] );
		}
		return SLAP_CB_CONTINUE;
	}

	(void)authzid_conn_remove( op->o_conn );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	if ( !BER_BVISEMPTY( &op->orb_edn ) ) {
		edn = op->orb_edn;

	} else if ( !BER_BVISEMPTY( &op->o_conn->c_dn ) ) {
		edn = op->o_conn->c_dn;
	}

	if ( !BER_BVISEMPTY( &edn ) ) {
		ber_tag_t save_tag = op->o_tag;
		struct berval save_dn = op->o_dn;
		struct berval save_ndn = op->o_ndn;
		int rc;

		/* pretend it's an extop without data,
		 * so it is treated as a generic write
		 */
		op->o_tag = LDAP_REQ_EXTENDED;
		op->o_dn = edn;
		op->o_ndn = edn;
		rc = backend_check_restrictions( op, rs, NULL );
		op->o_tag = save_tag;
		op->o_dn = save_dn;
		op->o_ndn = save_ndn;
		if ( rc != LDAP_SUCCESS ) {
			rs->sr_err = LDAP_CONFIDENTIALITY_REQUIRED;
			return SLAP_CB_CONTINUE;
		}

		len = STRLENOF("dn:") + edn.bv_len;
	}

	/* save original controls in sc_private;
	 * will be restored by sc_cleanup
	 */
	if ( rs->sr_ctrls != NULL ) {
		op->o_callback->sc_private = rs->sr_ctrls;
		for ( ; rs->sr_ctrls[n] != NULL; n++ )
			;
	}

	ctrls = op->o_tmpalloc( sizeof( LDAPControl * )*( n + 2 ), op->o_tmpmemctx );
	n = 0;
	if ( rs->sr_ctrls ) {
		for ( ; rs->sr_ctrls[n] != NULL; n++ ) {
			ctrls[n] = rs->sr_ctrls[n];
		}
	}

	/* anonymous: "", otherwise "dn:<dn>" */
	ctrls[n] = op->o_tmpalloc( sizeof( LDAPControl ) + len + 1, op->o_tmpmemctx );
	ctrls[n]->ldctl_oid = LDAP_CONTROL_AUTHZID_RESPONSE;
	ctrls[n]->ldctl_iscritical = 0;
	ctrls[n]->ldctl_value.bv_len = len;
	ctrls[n]->ldctl_value.bv_val = (char *)&ctrls[n][1];
	if ( len ) {
		char *ptr;

		ptr = lutil_strcopy( ctrls[n]->ldctl_value.bv_val, "dn:" );
		ptr = lutil_strncopy( ptr, edn.bv_val, edn.bv_len );
	}
	ctrls[n]->ldctl_value.bv_val[len] = '\0';
	ctrls[n + 1] = NULL;

	rs->sr_ctrls = ctrls;

	return SLAP_CB_CONTINUE;
}

static int
authzid_cleanup(
	Operation *op,
	SlapReply *rs )
{
	if ( rs->sr_ctrls ) {
		LDAPControl *ctrl;

		/* if ours, cleanup */
		ctrl = ldap_control_find( LDAP_CONTROL_AUTHZID_RESPONSE, rs->sr_ctrls, NULL );
		if ( ctrl ) {
			op->o_tmpfree( rs->sr_ctrls, op->o_tmpmemctx );
			rs->sr_ctrls = NULL;
		}

		if ( op->o_callback->sc_private != NULL ) {
			rs->sr_ctrls = (LDAPControl **)op->o_callback->sc_private;
			op->o_callback->sc_private = NULL;
		}
	}

	op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
	op->o_callback = NULL;

	return SLAP_CB_CONTINUE;
}

static int
authzid_op_bind(
	Operation *op,
	SlapReply *rs )
{
	slap_callback *sc;

	if ( op->o_ctrlflag[ authzid_cid ] <= SLAP_CONTROL_IGNORED ) {
		authzid_conn_t *ac = authzid_conn_get( op->o_conn );
		if ( ac ) {
			op->o_ctrlflag[ authzid_cid ] = ac->authzid_flag;
			op->o_controls[ authzid_cid] = ac;
		}
	}

	if ( op->o_ctrlflag[ authzid_cid ] > SLAP_CONTROL_IGNORED ) {
		sc = op->o_callback;
		op->o_callback = op->o_tmpalloc( sizeof( slap_callback ), op->o_tmpmemctx );
		op->o_callback->sc_response = authzid_response;
		op->o_callback->sc_cleanup = authzid_cleanup;
		op->o_callback->sc_private = NULL;
		op->o_callback->sc_next = sc;
	}

	return SLAP_CB_CONTINUE;
}

static int
parse_authzid_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	*ctrl )
{
	if ( op->o_ctrlflag[ authzid_cid ] != SLAP_CONTROL_NONE ) {
		rs->sr_text = "authzid control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "authzid control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	/* drop ongoing requests */
	(void)authzid_conn_remove( op->o_conn );

	op->o_ctrlflag[ authzid_cid ] = ctrl->ldctl_iscritical ?  SLAP_CONTROL_CRITICAL : SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int
authzid_db_init( BackendDB *be, ConfigReply *cr )
{
	if ( !SLAP_ISGLOBALOVERLAY( be ) ) {
		/* do not allow slapo-ppolicy to be global by now (ITS#5858) */
		if ( cr ) {
			snprintf( cr->msg, sizeof(cr->msg), 
				"slapo-authzid must be global" );
			Debug( LDAP_DEBUG_ANY, "%s\n", cr->msg, 0, 0 );
		}
		return 1;
	}
		
	int rc;

	rc = register_supported_control( LDAP_CONTROL_AUTHZID_REQUEST,
		SLAP_CTRL_GLOBAL|SLAP_CTRL_BIND|SLAP_CTRL_HIDE, NULL,
		parse_authzid_ctrl, &authzid_cid );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"authzid_initialize: Failed to register control '%s' (%d)\n",
			LDAP_CONTROL_AUTHZID_REQUEST, rc, 0 );
		return rc;
	}

	return LDAP_SUCCESS;
}

/*
 * Almost pointless, by now, since this overlay needs to be global,
 * and global overlays deletion is not supported yet.
 */
static int
authzid_db_destroy( BackendDB *be, ConfigReply *cr )
{
#ifdef SLAP_CONFIG_DELETE
	overlay_unregister_control( be, LDAP_CONTROL_AUTHZID_REQUEST );
#endif /* SLAP_CONFIG_DELETE */

	unregister_supported_control( LDAP_CONTROL_AUTHZID_REQUEST );

	return 0;
}

static int
authzid_initialize( void )
{
	ldap_pvt_thread_mutex_init( &authzid_mutex );

	authzid.on_bi.bi_type = "authzid";

	authzid.on_bi.bi_db_init = authzid_db_init;
	authzid.on_bi.bi_db_destroy = authzid_db_destroy;
	authzid.on_bi.bi_op_bind = authzid_op_bind;

	return overlay_register( &authzid );
}

int
init_module( int argc, char *argv[] )
{
	return authzid_initialize();
}

