/* vc.c - LDAP Verify Credentials extop (no spec yet) */
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
 * LDAP Verify Credentials: suggested by Kurt Zeilenga
 * no spec yet
 */

#include "portable.h"

#include "slap.h"
#include "ac/string.h"

typedef struct vc_conn_t {
	struct vc_conn_t *conn;
	Connection connbuf;
	OperationBuffer opbuf;
	Operation *op;
	int refcnt;
} vc_conn_t;

static const struct berval vc_exop_oid_bv = BER_BVC(LDAP_EXOP_VERIFY_CREDENTIALS);
static ldap_pvt_thread_mutex_t vc_mutex;
static Avlnode *vc_tree;

static int
vc_conn_cmp( const void *c1, const void *c2 )
{
	const vc_conn_t *vc1 = (const vc_conn_t *)c1;
	const vc_conn_t *vc2 = (const vc_conn_t *)c2;

	return SLAP_PTRCMP( vc1->conn, vc2->conn );
}

static int
vc_conn_dup( void *c1, void *c2 )
{
	vc_conn_t *vc1 = (vc_conn_t *)c1;
	vc_conn_t *vc2 = (vc_conn_t *)c2;

	if ( vc1->conn == vc2->conn ) {
		return -1;
	}

	return 0;
}

static int
vc_create_response(
	void *conn,
	int resultCode,
	const char *diagnosticMessage,
	struct berval *servercred,
	struct berval *authzid,
	LDAPControl **ctrls,
	struct berval **val )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	struct berval bv;
	int rc;

	assert( val != NULL );

	*val = NULL;

	ber_init2( ber, NULL, LBER_USE_DER );

	(void)ber_printf( ber, "{is" /*}*/ , resultCode, diagnosticMessage ? diagnosticMessage : "" );

	if ( conn ) {
		struct berval cookie;

		cookie.bv_len = sizeof( conn );
		cookie.bv_val = (char *)&conn;
		(void)ber_printf( ber, "tO", 0, LDAP_TAG_EXOP_VERIFY_CREDENTIALS_COOKIE, &cookie ); 
	}

	if ( servercred ) {
		ber_printf( ber, "tO", LDAP_TAG_EXOP_VERIFY_CREDENTIALS_SCREDS, servercred ); 
	}

#if 0
	if ( authzid ) {
		ber_printf( ber, "tO", LDAP_TAG_EXOP_VERIFY_CREDENTIALS_AUTHZID, authzid ); 
	}
#endif

	if ( ctrls ) {
		int c;

		rc = ber_printf( ber, "t{"/*}*/, LDAP_TAG_EXOP_VERIFY_CREDENTIALS_CONTROLS );
		if ( rc == -1 ) goto done;

		for ( c = 0; ctrls[c] != NULL; c++ ) {
			rc = ber_printf( ber, "{s" /*}*/, ctrls[c]->ldctl_oid );

			if ( ctrls[c]->ldctl_iscritical ) {
				rc = ber_printf( ber, "b", (ber_int_t)ctrls[c]->ldctl_iscritical ) ;
				if ( rc == -1 ) goto done;
			}

			if ( ctrls[c]->ldctl_value.bv_val != NULL ) {
				rc = ber_printf( ber, "O", &ctrls[c]->ldctl_value ); 
				if( rc == -1 ) goto done;
			}

			rc = ber_printf( ber, /*{*/"N}" );
			if ( rc == -1 ) goto done;
		}

		rc = ber_printf( ber, /*{*/"N}" );
		if ( rc == -1 ) goto done;
	}

	rc = ber_printf( ber, /*{*/ "}" );
	if ( rc == -1 ) goto done;

	rc = ber_flatten2( ber, &bv, 0 );
	if ( rc == 0 ) {
		*val = ber_bvdup( &bv );
	}

done:;
	ber_free_buf( ber );

	return rc;
}

typedef struct vc_cb_t {
	struct berval sasldata;
	LDAPControl **ctrls;
} vc_cb_t;

static int
vc_cb(
	Operation	*op,
	SlapReply	*rs )
{
	vc_cb_t *vc = (vc_cb_t *)op->o_callback->sc_private;

	if ( rs->sr_tag == LDAP_RES_BIND ) {
		if ( rs->sr_sasldata != NULL ) {
			ber_dupbv( &vc->sasldata, rs->sr_sasldata );
		}

		if ( rs->sr_ctrls != NULL ) {
			vc->ctrls = ldap_controls_dup( rs->sr_ctrls );
		}
	}

	return 0;
}

static int
vc_exop(
	Operation	*op,
	SlapReply	*rs )
{
	int rc = LDAP_SUCCESS;
	ber_tag_t tag;
	ber_len_t len = -1;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	struct berval reqdata = BER_BVNULL;

	struct berval cookie = BER_BVNULL;
	struct berval bdn = BER_BVNULL;
	ber_tag_t authtag;
	struct berval cred = BER_BVNULL;
	struct berval ndn = BER_BVNULL;
	struct berval mechanism = BER_BVNULL;

	vc_conn_t *conn = NULL;
	vc_cb_t vc = { 0 };
	slap_callback sc = { 0 };
	SlapReply rs2 = { 0 };

	if ( op->ore_reqdata == NULL || op->ore_reqdata->bv_len == 0 ) {
		rs->sr_text = "empty request data field in VerifyCredentials exop";
		return LDAP_PROTOCOL_ERROR;
	}

	/* optimistic */
	rs->sr_err = LDAP_SUCCESS;

	ber_dupbv_x( &reqdata, op->ore_reqdata, op->o_tmpmemctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );
	if ( tag != LBER_SEQUENCE ) {
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_VERIFY_CREDENTIALS_COOKIE ) {
		/*
		 * cookie: the pointer to the connection
		 * of this operation
		 */

		ber_scanf( ber, "m", &cookie );
		if ( cookie.bv_len != sizeof(Connection *) ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}
	}

	/* DN, authtag */
	tag = ber_scanf( ber, "mt", &bdn, &authtag );
	if ( tag == LBER_ERROR ) {
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	rc = dnNormalize( 0, NULL, NULL, &bdn, &ndn, op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	switch ( authtag ) {
	case LDAP_AUTH_SIMPLE:
		/* cookie only makes sense for SASL bind (so far) */
		if ( !BER_BVISNULL( &cookie ) ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		tag = ber_scanf( ber, "m", &cred );
		if ( tag == LBER_ERROR ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}
		break;

	case LDAP_AUTH_SASL:
		tag = ber_scanf( ber, "{s" /*}*/ , &mechanism );
		if ( tag == LBER_ERROR || 
			BER_BVISNULL( &mechanism ) || BER_BVISEMPTY( &mechanism ) )
		{
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		tag = ber_peek_tag( ber, &len );
		if ( tag == LBER_OCTETSTRING ) {
			ber_scanf( ber, "m", &cred );
		}

		tag = ber_scanf( ber, /*{*/ "}" );
		break;

	default:
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	if ( !BER_BVISNULL( &cookie ) ) {
		vc_conn_t tmp = { 0 };

		AC_MEMCPY( (char *)&tmp.conn, (const char *)cookie.bv_val, cookie.bv_len );
		ldap_pvt_thread_mutex_lock( &vc_mutex );
		conn = (vc_conn_t *)avl_find( vc_tree, (caddr_t)&tmp, vc_conn_cmp );
		if ( conn == NULL || ( conn != NULL && conn->refcnt != 0 ) ) {
			conn = NULL;
			ldap_pvt_thread_mutex_unlock( &vc_mutex );
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}
		conn->refcnt++;
		ldap_pvt_thread_mutex_unlock( &vc_mutex );

	} else {
		void *thrctx;

		conn = (vc_conn_t *)SLAP_CALLOC( 1, sizeof( vc_conn_t ) );
		conn->refcnt = 1;

		thrctx = ldap_pvt_thread_pool_context();
		connection_fake_init2( &conn->connbuf, &conn->opbuf, thrctx, 0 );
		conn->op = &conn->opbuf.ob_op;
		snprintf( conn->op->o_log_prefix, sizeof( conn->op->o_log_prefix ),
			"%s VERIFYCREDENTIALS", op->o_log_prefix );
	}

	conn->op->o_tag = LDAP_REQ_BIND;
	memset( &conn->op->oq_bind, 0, sizeof( conn->op->oq_bind ) );
	conn->op->o_req_dn = ndn;
	conn->op->o_req_ndn = ndn;
	conn->op->o_protocol = LDAP_VERSION3;
	conn->op->orb_method = authtag;
	conn->op->o_callback = &sc;

	/* TODO: controls */
	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_VERIFY_CREDENTIALS_CONTROLS ) {
		conn->op->o_ber = ber;
		rc = get_ctrls2( conn->op, &rs2, 0, LDAP_TAG_EXOP_VERIFY_CREDENTIALS_CONTROLS );
		if ( rc != LDAP_SUCCESS ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}
	}

	tag = ber_skip_tag( ber, &len );
	if ( len || tag != LBER_DEFAULT ) {
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	switch ( authtag ) {
	case LDAP_AUTH_SIMPLE:
		break;

	case LDAP_AUTH_SASL:
		conn->op->orb_mech = mechanism;
		break;
	}

	conn->op->orb_cred = cred;
	sc.sc_response = vc_cb;
	sc.sc_private = &vc;

	conn->op->o_bd = frontendDB;
	rs->sr_err = frontendDB->be_bind( conn->op, &rs2 );

	if ( conn->op->o_conn->c_sasl_bind_in_progress ) {
		rc = vc_create_response( conn, rs2.sr_err, rs2.sr_text,
			!BER_BVISEMPTY( &vc.sasldata ) ? &vc.sasldata : NULL,
			NULL,
			vc.ctrls, &rs->sr_rspdata );

	} else {
		rc = vc_create_response( NULL, rs2.sr_err, rs2.sr_text,
			NULL,
			&conn->op->o_conn->c_dn,
			vc.ctrls, &rs->sr_rspdata );
	}

	if ( rc != 0 ) {
		rs->sr_err = LDAP_OTHER;
		goto done;
	}

	if ( !BER_BVISNULL( &conn->op->o_conn->c_dn ) &&
		conn->op->o_conn->c_dn.bv_val != conn->op->o_conn->c_ndn.bv_val )
		ber_memfree( conn->op->o_conn->c_dn.bv_val );
	if ( !BER_BVISNULL( &conn->op->o_conn->c_ndn ) )
		ber_memfree( conn->op->o_conn->c_ndn.bv_val );

done:;
	if ( conn ) {
		if ( conn->op->o_conn->c_sasl_bind_in_progress ) {
			if ( conn->conn == NULL ) {
				conn->conn = conn;
				conn->refcnt--;
				ldap_pvt_thread_mutex_lock( &vc_mutex );
				rc = avl_insert( &vc_tree, (caddr_t)conn,
					vc_conn_cmp, vc_conn_dup );
				ldap_pvt_thread_mutex_unlock( &vc_mutex );
				assert( rc == 0 );

			} else {
				ldap_pvt_thread_mutex_lock( &vc_mutex );
				conn->refcnt--;
				ldap_pvt_thread_mutex_unlock( &vc_mutex );
			}

		} else {
			if ( conn->conn != NULL ) {
				vc_conn_t *tmp;

				ldap_pvt_thread_mutex_lock( &vc_mutex );
				tmp = avl_delete( &vc_tree, (caddr_t)conn, vc_conn_cmp );
				ldap_pvt_thread_mutex_unlock( &vc_mutex );
			}
			SLAP_FREE( conn );
		}
	}

	if ( vc.ctrls ) {
		ldap_controls_free( vc.ctrls );
		vc.ctrls = NULL;
	}

	if ( !BER_BVISNULL( &ndn ) ) {
		op->o_tmpfree( ndn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &ndn );
	}

	op->o_tmpfree( reqdata.bv_val, op->o_tmpmemctx );
	BER_BVZERO( &reqdata );

        return rs->sr_err;
}

static int
vc_initialize( void )
{
	int rc;

	rc = load_extop2( (struct berval *)&vc_exop_oid_bv,
		SLAP_EXOP_HIDE, vc_exop, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"vc_initialize: unable to register VerifyCredentials exop: %d.\n",
			rc, 0, 0 );
	}

	ldap_pvt_thread_mutex_init( &vc_mutex );

	return rc;
}

int
init_module( int argc, char *argv[] )
{
	return vc_initialize();
}

