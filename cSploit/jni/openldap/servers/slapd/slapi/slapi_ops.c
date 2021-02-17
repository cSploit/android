/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002-2003 IBM Corporation.
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
 * This work was initially developed by IBM Corporation for use in
 * IBM products and subsequently ported to OpenLDAP Software by
 * Steve Omrani.  Additional significant contributors include:
 *   Luke Howard
 */

#include "portable.h"

#include <ac/string.h>
#include <ac/stdarg.h>
#include <ac/ctype.h>
#include <ac/unistd.h>

#include <slap.h>
#include <lber_pvt.h>
#include <slapi.h>

#ifdef LDAP_SLAPI

static struct Listener slapi_listener = {
	BER_BVC("slapi://"),
	BER_BVC("slapi://")
};

static LDAPControl **
slapi_int_dup_controls( LDAPControl **controls )
{
	LDAPControl **c;
	size_t i;

	if ( controls == NULL )
		return NULL;

	for ( i = 0; controls[i] != NULL; i++ )
		;

	c = (LDAPControl **) slapi_ch_calloc( i + 1, sizeof(LDAPControl *) );

	for ( i = 0; controls[i] != NULL; i++ ) {
		c[i] = slapi_dup_control( controls[i] );
	}

	return c;
}

static int
slapi_int_result(
	Operation	*op, 
	SlapReply	*rs )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	plugin_result_callback	prc = NULL;
	void			*callback_data = NULL;
	LDAPControl		**ctrls = NULL;

	assert( pb != NULL );	

	slapi_pblock_get( pb, SLAPI_X_INTOP_RESULT_CALLBACK, (void **)&prc );
	slapi_pblock_get( pb, SLAPI_X_INTOP_CALLBACK_DATA,   &callback_data );

	/* we need to duplicate controls because they might go out of scope */
	ctrls = slapi_int_dup_controls( rs->sr_ctrls );
	slapi_pblock_set( pb, SLAPI_RESCONTROLS, ctrls );

	if ( prc != NULL ) {
		(*prc)( rs->sr_err, callback_data );
	}

	return rs->sr_err;
}

static int
slapi_int_search_entry(
	Operation	*op,
	SlapReply	*rs )
{
	Slapi_PBlock			*pb = SLAPI_OPERATION_PBLOCK( op );
	plugin_search_entry_callback	psec = NULL;
	void				*callback_data = NULL;
	int				rc = LDAP_SUCCESS;

	assert( pb != NULL );

	slapi_pblock_get( pb, SLAPI_X_INTOP_SEARCH_ENTRY_CALLBACK, (void **)&psec );
	slapi_pblock_get( pb, SLAPI_X_INTOP_CALLBACK_DATA,         &callback_data );

	if ( psec != NULL ) {
		rc = (*psec)( rs->sr_entry, callback_data );
	}

	return rc;
}

static int
slapi_int_search_reference(
	Operation	*op,	
	SlapReply	*rs )
{
	int				i, rc = LDAP_SUCCESS;
	plugin_referral_entry_callback	prec = NULL;
	void				*callback_data = NULL;
	Slapi_PBlock			*pb = SLAPI_OPERATION_PBLOCK( op );

	assert( pb != NULL );

	slapi_pblock_get( pb, SLAPI_X_INTOP_REFERRAL_ENTRY_CALLBACK, (void **)&prec );
	slapi_pblock_get( pb, SLAPI_X_INTOP_CALLBACK_DATA,           &callback_data );

	if ( prec != NULL ) {
		for ( i = 0; rs->sr_ref[i].bv_val != NULL; i++ ) {
			rc = (*prec)( rs->sr_ref[i].bv_val, callback_data );
			if ( rc != LDAP_SUCCESS ) {
				break;
			}
		}
	}

	return rc;
}

int
slapi_int_response( Slapi_Operation *op, SlapReply *rs )
{
	int				rc;

	switch ( rs->sr_type ) {
	case REP_RESULT:
		rc = slapi_int_result( op, rs );
		break;
	case REP_SEARCH:
		rc = slapi_int_search_entry( op, rs );
		break;
	case REP_SEARCHREF:
		rc = slapi_int_search_reference( op, rs );
		break;
	default:
		rc = LDAP_OTHER;
		break;
	}

	assert( rc != SLAP_CB_CONTINUE ); /* never try to send a wire response */

	return rc;
}

static int
slapi_int_get_ctrls( Slapi_PBlock *pb )
{
	LDAPControl		**c;
	int			rc = LDAP_SUCCESS;

	if ( pb->pb_op->o_ctrls != NULL ) {
		for ( c = pb->pb_op->o_ctrls; *c != NULL; c++ ) {
			rc = slap_parse_ctrl( pb->pb_op, pb->pb_rs, *c, &pb->pb_rs->sr_text );
			if ( rc != LDAP_SUCCESS )
				break;
		}
	}

	return rc;
}

void
slapi_int_connection_init_pb( Slapi_PBlock *pb, ber_tag_t tag )
{
	Connection		*conn;
	Operation		*op;
	ber_len_t		max = sockbuf_max_incoming;

	conn = (Connection *) slapi_ch_calloc( 1, sizeof(Connection) );

	LDAP_STAILQ_INIT( &conn->c_pending_ops );

	op = (Operation *) slapi_ch_calloc( 1, sizeof(OperationBuffer) );
	op->o_hdr = &((OperationBuffer *) op)->ob_hdr;
	op->o_controls = ((OperationBuffer *) op)->ob_controls;

	op->o_callback = (slap_callback *) slapi_ch_calloc( 1, sizeof(slap_callback) );
	op->o_callback->sc_response = slapi_int_response;
	op->o_callback->sc_cleanup = NULL;
	op->o_callback->sc_private = pb;
	op->o_callback->sc_next = NULL;

	conn->c_pending_ops.stqh_first = op;

	/* connection object authorization information */
	conn->c_authtype = LDAP_AUTH_NONE;
	BER_BVZERO( &conn->c_authmech );
	BER_BVZERO( &conn->c_dn );
	BER_BVZERO( &conn->c_ndn );

	conn->c_listener = &slapi_listener;
	ber_dupbv( &conn->c_peer_domain, (struct berval *)&slap_unknown_bv );
	ber_dupbv( &conn->c_peer_name, (struct berval *)&slap_unknown_bv );

	LDAP_STAILQ_INIT( &conn->c_ops );

	BER_BVZERO( &conn->c_sasl_bind_mech );
	conn->c_sasl_authctx = NULL;
	conn->c_sasl_sockctx = NULL;
	conn->c_sasl_extra = NULL;

	conn->c_sb = ber_sockbuf_alloc();

	ber_sockbuf_ctrl( conn->c_sb, LBER_SB_OPT_SET_MAX_INCOMING, &max );

	conn->c_currentber = NULL;

	/* should check status of thread calls */
	ldap_pvt_thread_mutex_init( &conn->c_mutex );
	ldap_pvt_thread_mutex_init( &conn->c_write1_mutex );
	ldap_pvt_thread_mutex_init( &conn->c_write2_mutex );
	ldap_pvt_thread_cond_init( &conn->c_write1_cv );
	ldap_pvt_thread_cond_init( &conn->c_write2_cv );

	ldap_pvt_thread_mutex_lock( &conn->c_mutex );

	conn->c_n_ops_received = 0;
	conn->c_n_ops_executing = 0;
	conn->c_n_ops_pending = 0;
	conn->c_n_ops_completed = 0;

	conn->c_n_get = 0;
	conn->c_n_read = 0;
	conn->c_n_write = 0;

	conn->c_protocol = LDAP_VERSION3; 

	conn->c_activitytime = conn->c_starttime = slap_get_time();

	/*
	 * A real connection ID is required, because syncrepl associates
	 * pending CSNs with unique ( connection, operation ) tuples.
	 * Setting a fake connection ID will cause slap_get_commit_csn()
	 * to return a stale value.
	 */
	connection_assign_nextid( conn );

	conn->c_conn_state  = 0x01;	/* SLAP_C_ACTIVE */
	conn->c_struct_state = 0x02;	/* SLAP_C_USED */

	conn->c_ssf = conn->c_transport_ssf = local_ssf;
	conn->c_tls_ssf = 0;

	backend_connection_init( conn );

	conn->c_send_ldap_result = slap_send_ldap_result;
	conn->c_send_search_entry = slap_send_search_entry;
	conn->c_send_ldap_extended = slap_send_ldap_extended;
	conn->c_send_search_reference = slap_send_search_reference;

	/* operation object */
	op->o_tag = tag;
	op->o_protocol = LDAP_VERSION3; 
	BER_BVZERO( &op->o_authmech );
	op->o_time = slap_get_time();
	op->o_do_not_cache = 1;
	op->o_threadctx = ldap_pvt_thread_pool_context();
	op->o_tmpmemctx = NULL;
	op->o_tmpmfuncs = &ch_mfuncs;
	op->o_conn = conn;
	op->o_connid = conn->c_connid;
	op->o_bd = frontendDB;

	/* extensions */
	slapi_int_create_object_extensions( SLAPI_X_EXT_OPERATION, op );
	slapi_int_create_object_extensions( SLAPI_X_EXT_CONNECTION, conn );

	pb->pb_rs = (SlapReply *)slapi_ch_calloc( 1, sizeof(SlapReply) );
	pb->pb_op = op;
	pb->pb_conn = conn;
	pb->pb_intop = 1;

	ldap_pvt_thread_mutex_unlock( &conn->c_mutex );
}

static void
slapi_int_set_operation_dn( Slapi_PBlock *pb )
{
	Backend			*be;
	Operation		*op = pb->pb_op;

	if ( BER_BVISNULL( &op->o_ndn ) ) {
		/* set to root DN */
		be = select_backend( &op->o_req_ndn, 1 );
		if ( be != NULL ) {
			ber_dupbv( &op->o_dn, &be->be_rootdn );
			ber_dupbv( &op->o_ndn, &be->be_rootndn );
		}
	}
}

void
slapi_int_connection_done_pb( Slapi_PBlock *pb )
{
	Connection		*conn;
	Operation		*op;

	PBLOCK_ASSERT_INTOP( pb, 0 );

	conn = pb->pb_conn;
	op = pb->pb_op;

	/* free allocated DNs */
	if ( !BER_BVISNULL( &op->o_dn ) )
		op->o_tmpfree( op->o_dn.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &op->o_ndn ) )
		op->o_tmpfree( op->o_ndn.bv_val, op->o_tmpmemctx );

	if ( !BER_BVISNULL( &op->o_req_dn ) )
		op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &op->o_req_ndn ) )
		op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );

	switch ( op->o_tag ) {
	case LDAP_REQ_MODRDN:
		if ( !BER_BVISNULL( &op->orr_newrdn ))
			op->o_tmpfree( op->orr_newrdn.bv_val, op->o_tmpmemctx );
		if ( !BER_BVISNULL( &op->orr_nnewrdn ))
			op->o_tmpfree( op->orr_nnewrdn.bv_val, op->o_tmpmemctx );
		if ( op->orr_newSup != NULL ) {
			assert( !BER_BVISNULL( op->orr_newSup ) );
			op->o_tmpfree( op->orr_newSup->bv_val, op->o_tmpmemctx );
			op->o_tmpfree( op->orr_newSup, op->o_tmpmemctx );
		}
		if ( op->orr_nnewSup != NULL ) {
			assert( !BER_BVISNULL( op->orr_nnewSup ) );
			op->o_tmpfree( op->orr_nnewSup->bv_val, op->o_tmpmemctx );
			op->o_tmpfree( op->orr_nnewSup, op->o_tmpmemctx );
		}
		slap_mods_free( op->orr_modlist, 1 );
		break;
	case LDAP_REQ_ADD:
		slap_mods_free( op->ora_modlist, 0 );
		break;
	case LDAP_REQ_MODIFY:
		slap_mods_free( op->orm_modlist, 1 );
		break;
	case LDAP_REQ_SEARCH:
		if ( op->ors_attrs != NULL ) {
			op->o_tmpfree( op->ors_attrs, op->o_tmpmemctx );
			op->ors_attrs = NULL;
		}
		break;
	default:
		break;
	}

	slapi_ch_free_string( &conn->c_authmech.bv_val );
	slapi_ch_free_string( &conn->c_dn.bv_val );
	slapi_ch_free_string( &conn->c_ndn.bv_val );
	slapi_ch_free_string( &conn->c_peer_domain.bv_val );
	slapi_ch_free_string( &conn->c_peer_name.bv_val );

	if ( conn->c_sb != NULL ) {
		ber_sockbuf_free( conn->c_sb );
	}

	slapi_int_free_object_extensions( SLAPI_X_EXT_OPERATION, op );
	slapi_int_free_object_extensions( SLAPI_X_EXT_CONNECTION, conn );

	slapi_ch_free( (void **)&pb->pb_op->o_callback );
	slapi_ch_free( (void **)&pb->pb_op );
	slapi_ch_free( (void **)&pb->pb_conn );
	slapi_ch_free( (void **)&pb->pb_rs );
}

static int
slapi_int_func_internal_pb( Slapi_PBlock *pb, slap_operation_t which )
{
	SlapReply		*rs = pb->pb_rs;
	int			rc;

	PBLOCK_ASSERT_INTOP( pb, 0 );

	rc = slapi_int_get_ctrls( pb );
	if ( rc != LDAP_SUCCESS ) {
		rs->sr_err = rc;
		return rc;
	}

	pb->pb_op->o_bd = frontendDB;
	return (&frontendDB->be_bind)[which]( pb->pb_op, pb->pb_rs );
}

int
slapi_delete_internal_pb( Slapi_PBlock *pb )
{
	if ( pb == NULL ) {
		return -1;
	}

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_DELETE );

	slapi_int_func_internal_pb( pb, op_delete );

	return 0;
}

int
slapi_add_internal_pb( Slapi_PBlock *pb )
{
	SlapReply		*rs;
	Slapi_Entry		*entry_orig = NULL;
	OpExtraDB oex;
	int rc;

	if ( pb == NULL ) {
		return -1;
	}

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_ADD );

	rs = pb->pb_rs;

	entry_orig = pb->pb_op->ora_e;
	pb->pb_op->ora_e = NULL;

	/*
	 * The caller can specify a new entry, or a target DN and set
	 * of modifications, but not both.
	 */
	if ( entry_orig != NULL ) {
		if ( pb->pb_op->ora_modlist != NULL || !BER_BVISNULL( &pb->pb_op->o_req_ndn )) {
			rs->sr_err = LDAP_PARAM_ERROR;
			goto cleanup;
		}

		assert( BER_BVISNULL( &pb->pb_op->o_req_dn ) ); /* shouldn't get set */
		ber_dupbv( &pb->pb_op->o_req_dn, &entry_orig->e_name );
		ber_dupbv( &pb->pb_op->o_req_ndn, &entry_orig->e_nname );
	} else if ( pb->pb_op->ora_modlist == NULL || BER_BVISNULL( &pb->pb_op->o_req_ndn )) {
		rs->sr_err = LDAP_PARAM_ERROR;
		goto cleanup;
	}

	pb->pb_op->ora_e = (Entry *)slapi_ch_calloc( 1, sizeof(Entry) );
	ber_dupbv( &pb->pb_op->ora_e->e_name,  &pb->pb_op->o_req_dn );
	ber_dupbv( &pb->pb_op->ora_e->e_nname, &pb->pb_op->o_req_ndn );

	if ( entry_orig != NULL ) {
		assert( pb->pb_op->ora_modlist == NULL );

		rs->sr_err = slap_entry2mods( entry_orig, &pb->pb_op->ora_modlist,
			&rs->sr_text, pb->pb_textbuf, sizeof( pb->pb_textbuf ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto cleanup;
		}
	} else {
		assert( pb->pb_op->ora_modlist != NULL );
	}

	rs->sr_err = slap_mods_check( pb->pb_op, pb->pb_op->ora_modlist, &rs->sr_text,
		pb->pb_textbuf, sizeof( pb->pb_textbuf ), NULL );
	if ( rs->sr_err != LDAP_SUCCESS ) {
                goto cleanup;
        }

	/* Duplicate the values, because we may call slapi_entry_free() */
	rs->sr_err = slap_mods2entry( pb->pb_op->ora_modlist, &pb->pb_op->ora_e,
		1, 0, &rs->sr_text, pb->pb_textbuf, sizeof( pb->pb_textbuf ) );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto cleanup;
	}

	oex.oe.oe_key = (void *)do_add;
	oex.oe_db = NULL;
	LDAP_SLIST_INSERT_HEAD(&pb->pb_op->o_extra, &oex.oe, oe_next);
	rc = slapi_int_func_internal_pb( pb, op_add );
	LDAP_SLIST_REMOVE(&pb->pb_op->o_extra, &oex.oe, OpExtra, oe_next);

	if ( !rc ) {
		if ( pb->pb_op->ora_e != NULL && oex.oe_db != NULL ) {
			BackendDB	*bd = pb->pb_op->o_bd;

			pb->pb_op->o_bd = oex.oe_db;
			be_entry_release_w( pb->pb_op, pb->pb_op->ora_e );
			pb->pb_op->ora_e = NULL;
			pb->pb_op->o_bd = bd;
		}
	}

cleanup:

	if ( pb->pb_op->ora_e != NULL ) {
		slapi_entry_free( pb->pb_op->ora_e );
		pb->pb_op->ora_e = NULL;
	}
	if ( entry_orig != NULL ) {
		pb->pb_op->ora_e = entry_orig;
		slap_mods_free( pb->pb_op->ora_modlist, 1 );
		pb->pb_op->ora_modlist = NULL;
	}

	return 0;
}

int
slapi_modrdn_internal_pb( Slapi_PBlock *pb )
{
	if ( pb == NULL ) {
		return -1;
	}

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_MODRDN );

	if ( BER_BVISEMPTY( &pb->pb_op->o_req_ndn ) ) {
		pb->pb_rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto cleanup;
	}

	slapi_int_func_internal_pb( pb, op_modrdn );

cleanup:

	return 0;
}

int
slapi_modify_internal_pb( Slapi_PBlock *pb )
{
	SlapReply		*rs;

	if ( pb == NULL ) {
		return -1;
	}

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_MODIFY );

	rs = pb->pb_rs;

	if ( pb->pb_op->orm_modlist == NULL ) {
		rs->sr_err = LDAP_PARAM_ERROR;
		goto cleanup;
	}

	if ( BER_BVISEMPTY( &pb->pb_op->o_req_ndn ) ) {
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto cleanup;
	}

	rs->sr_err = slap_mods_check( pb->pb_op, pb->pb_op->orm_modlist,
		&rs->sr_text, pb->pb_textbuf, sizeof( pb->pb_textbuf ), NULL );
	if ( rs->sr_err != LDAP_SUCCESS ) {
                goto cleanup;
        }

	slapi_int_func_internal_pb( pb, op_modify );

cleanup:

	return 0;
}

static int
slapi_int_search_entry_callback( Slapi_Entry *entry, void *callback_data )
{
	int		nentries = 0, i = 0;
	Slapi_Entry	**head = NULL, **tp;
	Slapi_PBlock	*pb = (Slapi_PBlock *)callback_data;

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_SEARCH );

	entry = slapi_entry_dup( entry );
	if ( entry == NULL ) {
		return LDAP_NO_MEMORY;
	}

	slapi_pblock_get( pb, SLAPI_NENTRIES, &nentries );
	slapi_pblock_get( pb, SLAPI_PLUGIN_INTOP_SEARCH_ENTRIES, &head );
	
	i = nentries + 1;
	if ( nentries == 0 ) {
		tp = (Slapi_Entry **)slapi_ch_malloc( 2 * sizeof(Slapi_Entry *) );
		if ( tp == NULL ) {
			slapi_entry_free( entry );
			return LDAP_NO_MEMORY;
		}

		tp[0] = entry;
	} else {
		tp = (Slapi_Entry **)slapi_ch_realloc( (char *)head,
				sizeof(Slapi_Entry *) * ( i + 1 ) );
		if ( tp == NULL ) {
			slapi_entry_free( entry );
			return LDAP_NO_MEMORY;
		}
		tp[i - 1] = entry;
	}
	tp[i] = NULL;
	          
	slapi_pblock_set( pb, SLAPI_PLUGIN_INTOP_SEARCH_ENTRIES, (void *)tp );
	slapi_pblock_set( pb, SLAPI_NENTRIES, (void *)&i );

	return LDAP_SUCCESS;
}

int
slapi_search_internal_pb( Slapi_PBlock *pb )
{
	return slapi_search_internal_callback_pb( pb,
		(void *)pb,
		NULL,
		slapi_int_search_entry_callback,
		NULL );
}

int
slapi_search_internal_callback_pb( Slapi_PBlock *pb,
	void *callback_data,
	plugin_result_callback prc,
	plugin_search_entry_callback psec,
	plugin_referral_entry_callback prec )
{
	int			free_filter = 0;
	SlapReply		*rs;

	if ( pb == NULL ) {
		return -1;
	}

	PBLOCK_ASSERT_INTOP( pb, LDAP_REQ_SEARCH );

	rs = pb->pb_rs;

	/* search callback and arguments */
	slapi_pblock_set( pb, SLAPI_X_INTOP_RESULT_CALLBACK,         (void *)prc );
	slapi_pblock_set( pb, SLAPI_X_INTOP_SEARCH_ENTRY_CALLBACK,   (void *)psec );
	slapi_pblock_set( pb, SLAPI_X_INTOP_REFERRAL_ENTRY_CALLBACK, (void *)prec );
	slapi_pblock_set( pb, SLAPI_X_INTOP_CALLBACK_DATA,           (void *)callback_data );

	if ( BER_BVISEMPTY( &pb->pb_op->ors_filterstr )) {
		rs->sr_err = LDAP_PARAM_ERROR;
		goto cleanup;
	}

	if ( pb->pb_op->ors_filter == NULL ) {
		pb->pb_op->ors_filter = slapi_str2filter( pb->pb_op->ors_filterstr.bv_val );
		if ( pb->pb_op->ors_filter == NULL ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto cleanup;
		}

		free_filter = 1;
	}

	slapi_int_func_internal_pb( pb, op_search );

cleanup:
	if ( free_filter ) {
		slapi_filter_free( pb->pb_op->ors_filter, 1 );
		pb->pb_op->ors_filter = NULL;
	}

	slapi_pblock_delete_param( pb, SLAPI_X_INTOP_RESULT_CALLBACK );
	slapi_pblock_delete_param( pb, SLAPI_X_INTOP_SEARCH_ENTRY_CALLBACK );
	slapi_pblock_delete_param( pb, SLAPI_X_INTOP_REFERRAL_ENTRY_CALLBACK );
	slapi_pblock_delete_param( pb, SLAPI_X_INTOP_CALLBACK_DATA );

	return 0;
}

/* Wrappers for old API */

void
slapi_search_internal_set_pb( Slapi_PBlock *pb,
	const char *base,
	int scope,
	const char *filter,
	char **attrs,
	int attrsonly,
	LDAPControl **controls,
	const char *uniqueid,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	int no_limit = SLAP_NO_LIMIT;
	int deref = LDAP_DEREF_NEVER;

	slapi_int_connection_init_pb( pb, LDAP_REQ_SEARCH );
	slapi_pblock_set( pb, SLAPI_SEARCH_TARGET,    (void *)base );
	slapi_pblock_set( pb, SLAPI_SEARCH_SCOPE,     (void *)&scope );
	slapi_pblock_set( pb, SLAPI_SEARCH_FILTER,    (void *)0 );
	slapi_pblock_set( pb, SLAPI_SEARCH_STRFILTER, (void *)filter );
	slapi_pblock_set( pb, SLAPI_SEARCH_ATTRS,     (void *)attrs );
	slapi_pblock_set( pb, SLAPI_SEARCH_ATTRSONLY, (void *)&attrsonly );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,      (void *)controls );
	slapi_pblock_set( pb, SLAPI_TARGET_UNIQUEID,  (void *)uniqueid );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY,  (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,    (void *)&operation_flags );
	slapi_pblock_set( pb, SLAPI_SEARCH_DEREF,     (void *)&deref );
	slapi_pblock_set( pb, SLAPI_SEARCH_SIZELIMIT, (void *)&no_limit );
	slapi_pblock_set( pb, SLAPI_SEARCH_TIMELIMIT, (void *)&no_limit );

	slapi_int_set_operation_dn( pb );
}

Slapi_PBlock *
slapi_search_internal(
	char *ldn, 
	int scope, 
	char *filStr, 
	LDAPControl **controls, 
	char **attrs, 
	int attrsonly ) 
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new();

	slapi_search_internal_set_pb( pb, ldn, scope, filStr,
		attrs, attrsonly,
		controls, NULL, NULL, 0 );

	slapi_search_internal_pb( pb );

	return pb;
}

void
slapi_modify_internal_set_pb( Slapi_PBlock *pb,
	const char *dn,
	LDAPMod **mods,
	LDAPControl **controls,
	const char *uniqueid,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	slapi_int_connection_init_pb( pb, LDAP_REQ_MODIFY );
	slapi_pblock_set( pb, SLAPI_MODIFY_TARGET,   (void *)dn );
	slapi_pblock_set( pb, SLAPI_MODIFY_MODS,     (void *)mods );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,     (void *)controls );
	slapi_pblock_set( pb, SLAPI_TARGET_UNIQUEID, (void *)uniqueid );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY, (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,   (void *)&operation_flags );
	slapi_int_set_operation_dn( pb );
}

/* Function : slapi_modify_internal
 *
 * Description:	Plugin functions call this routine to modify an entry 
 *				in the backend directly
 * Return values : LDAP_SUCCESS
 *                 LDAP_PARAM_ERROR
 *                 LDAP_NO_MEMORY
 *                 LDAP_OTHER
 *                 LDAP_UNWILLING_TO_PERFORM
*/
Slapi_PBlock *
slapi_modify_internal(
	char *ldn, 	
	LDAPMod **mods, 
	LDAPControl **controls, 
	int log_change )
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new();

	slapi_modify_internal_set_pb( pb, ldn, mods, controls, NULL, NULL, 0 );
	slapi_pblock_set( pb, SLAPI_LOG_OPERATION, (void *)&log_change );
	slapi_modify_internal_pb( pb );

	return pb;
}

int
slapi_add_internal_set_pb( Slapi_PBlock *pb,
	const char *dn,
	LDAPMod **attrs,
	LDAPControl **controls,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	slapi_int_connection_init_pb( pb, LDAP_REQ_ADD );
	slapi_pblock_set( pb, SLAPI_ADD_TARGET,      (void *)dn );
	slapi_pblock_set( pb, SLAPI_MODIFY_MODS,     (void *)attrs );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,     (void *)controls );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY, (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,   (void *)&operation_flags );
	slapi_int_set_operation_dn( pb );

	return 0;
}

Slapi_PBlock *
slapi_add_internal(
	char * dn,
	LDAPMod **attrs,
	LDAPControl **controls,
	int log_change )
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new();

	slapi_add_internal_set_pb( pb, dn, attrs, controls, NULL, 0);
	slapi_pblock_set( pb, SLAPI_LOG_OPERATION, (void *)&log_change );
	slapi_add_internal_pb( pb );

	return pb;
}

void
slapi_add_entry_internal_set_pb( Slapi_PBlock *pb,
	Slapi_Entry *e,
	LDAPControl **controls,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	slapi_int_connection_init_pb( pb, LDAP_REQ_ADD );
	slapi_pblock_set( pb, SLAPI_ADD_ENTRY,       (void *)e );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,     (void *)controls );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY, (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,   (void *)&operation_flags );
	slapi_int_set_operation_dn( pb );
}

Slapi_PBlock * 
slapi_add_entry_internal(
	Slapi_Entry *e, 
	LDAPControl **controls, 
	int log_change )
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new();

	slapi_add_entry_internal_set_pb( pb, e, controls, NULL, 0 );
	slapi_pblock_set( pb, SLAPI_LOG_OPERATION, (void *)&log_change );
	slapi_add_internal_pb( pb );

	return pb;
}

void
slapi_rename_internal_set_pb( Slapi_PBlock *pb,
	const char *olddn,
	const char *newrdn,
	const char *newsuperior,
	int deloldrdn,
	LDAPControl **controls,
	const char *uniqueid,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	slapi_int_connection_init_pb( pb, LDAP_REQ_MODRDN );
	slapi_pblock_set( pb, SLAPI_MODRDN_TARGET,      (void *)olddn );
	slapi_pblock_set( pb, SLAPI_MODRDN_NEWRDN,      (void *)newrdn );
	slapi_pblock_set( pb, SLAPI_MODRDN_NEWSUPERIOR, (void *)newsuperior );
	slapi_pblock_set( pb, SLAPI_MODRDN_DELOLDRDN,   (void *)&deloldrdn );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,        (void *)controls );
	slapi_pblock_set( pb, SLAPI_TARGET_UNIQUEID,    (void *)uniqueid );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY,    (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,      (void *)&operation_flags );
	slap_modrdn2mods( pb->pb_op, pb->pb_rs );
	slapi_int_set_operation_dn( pb );
}

/* Function : slapi_modrdn_internal
 *
 * Description : Plugin functions call this routine to modify the rdn 
 *				 of an entry in the backend directly
 * Return values : LDAP_SUCCESS
 *                 LDAP_PARAM_ERROR
 *                 LDAP_NO_MEMORY
 *                 LDAP_OTHER
 *                 LDAP_UNWILLING_TO_PERFORM
 *
 * NOTE: This function does not support the "newSuperior" option from LDAP V3.
 */
Slapi_PBlock *
slapi_modrdn_internal(
	char *olddn, 
	char *lnewrdn, 
	int deloldrdn, 
	LDAPControl **controls, 
	int log_change )
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new ();

	slapi_rename_internal_set_pb( pb, olddn, lnewrdn, NULL,
		deloldrdn, controls, NULL, NULL, 0 );
	slapi_pblock_set( pb, SLAPI_LOG_OPERATION, (void *)&log_change );
	slapi_modrdn_internal_pb( pb );

	return pb;
}

void
slapi_delete_internal_set_pb( Slapi_PBlock *pb,
	const char *dn,
	LDAPControl **controls,
	const char *uniqueid,
	Slapi_ComponentId *plugin_identity,
	int operation_flags )
{
	slapi_int_connection_init_pb( pb, LDAP_REQ_DELETE );
	slapi_pblock_set( pb, SLAPI_TARGET_DN,       (void *)dn );
	slapi_pblock_set( pb, SLAPI_REQCONTROLS,     (void *)controls );
	slapi_pblock_set( pb, SLAPI_TARGET_UNIQUEID, (void *)uniqueid );
	slapi_pblock_set( pb, SLAPI_PLUGIN_IDENTITY, (void *)plugin_identity );
	slapi_pblock_set( pb, SLAPI_X_INTOP_FLAGS,   (void *)&operation_flags );
	slapi_int_set_operation_dn( pb );
}

/* Function : slapi_delete_internal
 *
 * Description : Plugin functions call this routine to delete an entry 
 *               in the backend directly
 * Return values : LDAP_SUCCESS
 *                 LDAP_PARAM_ERROR
 *                 LDAP_NO_MEMORY
 *                 LDAP_OTHER
 *                 LDAP_UNWILLING_TO_PERFORM
*/
Slapi_PBlock *
slapi_delete_internal(
	char *ldn, 
	LDAPControl **controls, 
	int log_change )
{
	Slapi_PBlock *pb;

	pb = slapi_pblock_new();

	slapi_delete_internal_set_pb( pb, ldn, controls, NULL, NULL, 0 );
	slapi_pblock_set( pb, SLAPI_LOG_OPERATION, (void *)&log_change );
	slapi_delete_internal_pb( pb );

	return pb;
}

#endif /* LDAP_SLAPI */
