/* slapi_overlay.c - SLAPI overlay */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Luke Howard for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "slapi.h"
#include "config.h"

#ifdef LDAP_SLAPI

static slap_overinst slapi;
static int slapi_over_initialized = 0;

static int slapi_over_response( Operation *op, SlapReply *rs );
static int slapi_over_cleanup( Operation *op, SlapReply *rs );

static Slapi_PBlock *
slapi_over_pblock_new( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb;

	pb = slapi_pblock_new();
	pb->pb_op = op;
	pb->pb_conn = op->o_conn;
	pb->pb_rs = rs;
	pb->pb_intop = 0;

	PBLOCK_ASSERT_OP( pb, op->o_tag );

	return pb;
}

static int
slapi_op_internal_p( Operation *op, SlapReply *rs, slap_callback *cb )
{
	int			internal_op = 0;
	Slapi_PBlock		*pb = NULL;
	slap_callback		*pcb;

	/*
	 * Abstraction violating check for SLAPI internal operations
	 * allows pblock to remain consistent when invoking internal
	 * op plugins
	 */
	for ( pcb = op->o_callback; pcb != NULL; pcb = pcb->sc_next ) {
		if ( pcb->sc_response == slapi_int_response ) {
			pb = (Slapi_PBlock *)pcb->sc_private;
			PBLOCK_ASSERT_INTOP( pb, 0 );
			internal_op = 1;
			break;
		}
	}

	if ( cb != NULL ) {
		if ( pb == NULL ) {
			pb = slapi_over_pblock_new( op, rs );
		}

		cb->sc_response = slapi_over_response;
		cb->sc_cleanup = slapi_over_cleanup;
		cb->sc_private = pb;
		cb->sc_next = op->o_callback;
		op->o_callback = cb;
	}

	return internal_op;
}

static int
slapi_over_compute_output(
	computed_attr_context *c,
	Slapi_Attr *attribute,
	Slapi_Entry *entry
)
{
	Attribute		**a;
	AttributeDescription	*desc;
	SlapReply		*rs;

	if ( c == NULL || attribute == NULL || entry == NULL ) {
		return 0;
	}

	rs = (SlapReply *)c->cac_private;

	assert( rs->sr_entry == entry );

	desc = attribute->a_desc;

	if ( rs->sr_attrs == NULL ) {
		/* All attrs request, skip operational attributes */
		if ( is_at_operational( desc->ad_type ) ) {
			return 0;
		}
	} else {
		/* Specific attributes requested */
		if ( is_at_operational( desc->ad_type ) ) {
			if ( !SLAP_OPATTRS( rs->sr_attr_flags ) &&
			     !ad_inlist( desc, rs->sr_attrs ) ) {
				return 0;
			}
		} else {
			if ( !SLAP_USERATTRS( rs->sr_attr_flags ) &&
			     !ad_inlist( desc, rs->sr_attrs ) ) {
				return 0;
			}
		}
	}

	/* XXX perhaps we should check for existing attributes and merge */
	for ( a = &rs->sr_operational_attrs; *a != NULL; a = &(*a)->a_next )
		;

	*a = slapi_attr_dup( attribute );

	return 0;
}

static int
slapi_over_aux_operational( Operation *op, SlapReply *rs )
{
	/* Support for computed attribute plugins */
	computed_attr_context    ctx;
	AttributeName		*anp;

	if ( slapi_op_internal_p( op, rs, NULL ) ) {
		return SLAP_CB_CONTINUE;
	}

	ctx.cac_pb = slapi_over_pblock_new( op, rs );
	ctx.cac_op = op;
	ctx.cac_private = rs;

	if ( rs->sr_entry != NULL ) {
		/*
		 * For each client requested attribute, call the plugins.
		 */
		if ( rs->sr_attrs != NULL ) {
			for ( anp = rs->sr_attrs; anp->an_name.bv_val != NULL; anp++ ) {
				if ( compute_evaluator( &ctx, anp->an_name.bv_val,
					rs->sr_entry, slapi_over_compute_output ) == 1 ) {
					break;
				}
			}
		} else {
			/*
			 * Technically we shouldn't be returning operational attributes
			 * when the user requested only user attributes. We'll let the
			 * plugin decide whether to be naughty or not.
			 */
			compute_evaluator( &ctx, "*", rs->sr_entry, slapi_over_compute_output );
		}
	}

	slapi_pblock_destroy( ctx.cac_pb );

	return SLAP_CB_CONTINUE;
}

/*
 * We need this function to call frontendDB (global) plugins before
 * database plugins, if we are invoked by a slap_callback.
 */
static int
slapi_over_call_plugins( Slapi_PBlock *pb, int type )
{
	int 			rc = 1; /* means no plugins called */
	Operation		*op;

	PBLOCK_ASSERT_OP( pb, 0 );
	op = pb->pb_op;

	if ( !be_match( op->o_bd, frontendDB ) ) {
		rc = slapi_int_call_plugins( frontendDB, type, pb );
	}
	if ( rc >= 0 ) {
		rc = slapi_int_call_plugins( op->o_bd, type, pb );
	}

	return rc;
}

static int
slapi_over_search( Operation *op, SlapReply *rs, int type )
{
	int			rc;
	Slapi_PBlock		*pb;

	assert( rs->sr_type == REP_SEARCH || rs->sr_type == REP_SEARCHREF );

	/* create a new pblock to not trample on result controls */
	pb = slapi_over_pblock_new( op, rs );

	rc = slapi_over_call_plugins( pb, type );
	if ( rc >= 0 ) /* 1 means no plugins called */
		rc = SLAP_CB_CONTINUE;
	else
		rc = LDAP_SUCCESS; /* confusing: don't abort, but don't send */

	slapi_pblock_destroy(pb);

	return rc;
}

/*
 * Call pre- and post-result plugins
 */
static int
slapi_over_result( Operation *op, SlapReply *rs, int type )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );

	assert( rs->sr_type == REP_RESULT || rs->sr_type == REP_SASL || rs->sr_type == REP_EXTENDED );

	slapi_over_call_plugins( pb, type );

	return SLAP_CB_CONTINUE;
}


static int
slapi_op_bind_callback( Operation *op, SlapReply *rs, int prc )
{
	switch ( prc ) {
	case SLAPI_BIND_SUCCESS:
		/* Continue with backend processing */
		break;
	case SLAPI_BIND_FAIL:
		/* Failure, frontend (that's us) sends result */
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		send_ldap_result( op, rs );
		return rs->sr_err;
		break;
	case SLAPI_BIND_ANONYMOUS: /* undocumented */
	default: /* plugin sent result or no plugins called */
		BER_BVZERO( &op->orb_edn );

		if ( rs->sr_err == LDAP_SUCCESS ) {
			/*
			 * Plugin will have called slapi_pblock_set(LDAP_CONN_DN) which
			 * will have set conn->c_dn and conn->c_ndn
			 */
			if ( BER_BVISNULL( &op->o_conn->c_ndn ) && prc == 1 ) {
				/* No plugins were called; continue processing */
				return LDAP_SUCCESS;
			}
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			if ( !BER_BVISEMPTY( &op->o_conn->c_ndn ) ) {
				ber_len_t max = sockbuf_max_incoming_auth;
				ber_sockbuf_ctrl( op->o_conn->c_sb,
					LBER_SB_OPT_SET_MAX_INCOMING, &max );
			}
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

			/* log authorization identity */
			Statslog( LDAP_DEBUG_STATS,
				"%s BIND dn=\"%s\" mech=%s (SLAPI) ssf=0\n",
				op->o_log_prefix,
				BER_BVISNULL( &op->o_conn->c_dn )
					? "<empty>" : op->o_conn->c_dn.bv_val,
				BER_BVISNULL( &op->orb_mech )
					? "<empty>" : op->orb_mech.bv_val, 0, 0 );

			return -1;
		}
		break;
	}

	return rs->sr_err;
}

static int
slapi_op_search_callback( Operation *op, SlapReply *rs, int prc )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	Filter *f = op->ors_filter;

	/* check preoperation result code */
	if ( prc < 0 ) {
		return rs->sr_err;
	}

	rs->sr_err = LDAP_SUCCESS;

	if ( pb->pb_intop == 0 && 
	     slapi_int_call_plugins( op->o_bd, SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN, pb ) == 0 ) {
		/*
		 * The plugin can set the SLAPI_SEARCH_FILTER.
		 * SLAPI_SEARCH_STRFILER is not normative.
		 */
		if (f != op->ors_filter) {
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
			filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
		}
	}

	return LDAP_SUCCESS;
}

struct slapi_op_info {
	int soi_preop;			/* preoperation plugin parameter */
	int soi_postop;			/* postoperation plugin parameter */
	int soi_internal_preop;		/* internal preoperation plugin parameter */
	int soi_internal_postop;	/* internal postoperation plugin parameter */
	int (*soi_callback)(Operation *, SlapReply *, int); /* preoperation result handler */
} slapi_op_dispatch_table[] = {
	{
		SLAPI_PLUGIN_PRE_BIND_FN,
		SLAPI_PLUGIN_POST_BIND_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_BIND_FN,
		SLAPI_PLUGIN_INTERNAL_POST_BIND_FN,
		slapi_op_bind_callback
	},
	{
		SLAPI_PLUGIN_PRE_UNBIND_FN,
		SLAPI_PLUGIN_POST_UNBIND_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_UNBIND_FN,
		SLAPI_PLUGIN_INTERNAL_POST_UNBIND_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_SEARCH_FN,
		SLAPI_PLUGIN_POST_SEARCH_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_SEARCH_FN,
		SLAPI_PLUGIN_INTERNAL_POST_SEARCH_FN,
		slapi_op_search_callback
	},
	{
		SLAPI_PLUGIN_PRE_COMPARE_FN,
		SLAPI_PLUGIN_POST_COMPARE_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_COMPARE_FN,
		SLAPI_PLUGIN_INTERNAL_POST_COMPARE_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_MODIFY_FN,
		SLAPI_PLUGIN_POST_MODIFY_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_MODIFY_FN,
		SLAPI_PLUGIN_INTERNAL_POST_MODIFY_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_MODRDN_FN,
		SLAPI_PLUGIN_POST_MODRDN_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_MODRDN_FN,
		SLAPI_PLUGIN_INTERNAL_POST_MODRDN_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_ADD_FN,
		SLAPI_PLUGIN_POST_ADD_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_ADD_FN,
		SLAPI_PLUGIN_INTERNAL_POST_ADD_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_DELETE_FN,
		SLAPI_PLUGIN_POST_DELETE_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_DELETE_FN,
		SLAPI_PLUGIN_INTERNAL_POST_DELETE_FN,
		NULL
	},
	{
		SLAPI_PLUGIN_PRE_ABANDON_FN,
		SLAPI_PLUGIN_POST_ABANDON_FN,
		SLAPI_PLUGIN_INTERNAL_PRE_ABANDON_FN,
		SLAPI_PLUGIN_INTERNAL_POST_ABANDON_FN,
		NULL
	},
	{
		0,
		0,
		0,
		0,
		NULL
	}
};

slap_operation_t
slapi_tag2op( ber_tag_t tag )
{
	slap_operation_t op;

	switch ( tag ) {
	case LDAP_REQ_BIND:
		op = op_bind;
		break;
	case LDAP_REQ_ADD:
		op = op_add;
		break;
	case LDAP_REQ_DELETE:
		op = op_delete;
		break;
	case LDAP_REQ_MODRDN:
		op = op_modrdn;
		break;
	case LDAP_REQ_MODIFY:
		op = op_modify;
		break;
	case LDAP_REQ_COMPARE:
		op = op_compare;
		break;
	case LDAP_REQ_SEARCH:
		op = op_search;
		break;
	case LDAP_REQ_UNBIND:
		op = op_unbind;
		break;
	default:
		op = op_last;
		break;
	}

	return op;
}

/* Add SLAPI_RESCONTROLS to rs->sr_ctrls, with care, because
 * rs->sr_ctrls could be allocated on the stack */
static int
slapi_over_merge_controls( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	LDAPControl		**ctrls = NULL;
	LDAPControl		**slapi_ctrls = NULL;
	size_t			n_slapi_ctrls = 0;
	size_t			n_rs_ctrls = 0;
	size_t			i;

	slapi_pblock_get( pb, SLAPI_RESCONTROLS, (void **)&slapi_ctrls );

	n_slapi_ctrls = slapi_int_count_controls( slapi_ctrls );
	n_rs_ctrls = slapi_int_count_controls( rs->sr_ctrls );

	if ( n_slapi_ctrls == 0 )
		return LDAP_SUCCESS; /* no SLAPI controls */

	slapi_pblock_set( pb, SLAPI_X_OLD_RESCONTROLS, (void *)rs->sr_ctrls );

	ctrls = (LDAPControl **) op->o_tmpalloc(
		( n_slapi_ctrls + n_rs_ctrls + 1 ) * sizeof(LDAPControl *),
		op->o_tmpmemctx );

	for ( i = 0; i < n_slapi_ctrls; i++ ) {
		ctrls[i] = slapi_ctrls[i];
	}
	if ( rs->sr_ctrls != NULL ) {
		for ( i = 0; i < n_rs_ctrls; i++ ) {
			ctrls[n_slapi_ctrls + i] = rs->sr_ctrls[i];
		}
	}
	ctrls[n_slapi_ctrls + n_rs_ctrls] = NULL;

	rs->sr_ctrls = ctrls;

	return LDAP_SUCCESS;
}

static int
slapi_over_unmerge_controls( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	LDAPControl		**rs_ctrls = NULL;

	slapi_pblock_get( pb, SLAPI_X_OLD_RESCONTROLS, (void **)&rs_ctrls );

	if ( rs_ctrls == NULL || rs->sr_ctrls == rs_ctrls ) {
		/* no copying done */
		return LDAP_SUCCESS;
	}

	op->o_tmpfree( rs->sr_ctrls, op->o_tmpmemctx );
	rs->sr_ctrls = rs_ctrls;

	return LDAP_SUCCESS;
}

static int
slapi_over_response( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	int			rc = SLAP_CB_CONTINUE;

	if ( pb->pb_intop == 0 ) {
		switch ( rs->sr_type ) {
		case REP_RESULT:
		case REP_SASL:
		case REP_EXTENDED:
			rc = slapi_over_result( op, rs, SLAPI_PLUGIN_PRE_RESULT_FN );
			break;
		case REP_SEARCH:
			rc = slapi_over_search( op, rs, SLAPI_PLUGIN_PRE_ENTRY_FN );
			break;
		case REP_SEARCHREF:
			rc = slapi_over_search( op, rs, SLAPI_PLUGIN_PRE_REFERRAL_FN );
			break;
		default:
			break;
		}
	}

	slapi_over_merge_controls( op, rs );

	return rc;
}

static int
slapi_over_cleanup( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb = SLAPI_OPERATION_PBLOCK( op );
	int			rc = SLAP_CB_CONTINUE;

	slapi_over_unmerge_controls( op, rs );

	if ( pb->pb_intop == 0 ) {
		switch ( rs->sr_type ) {
		case REP_RESULT:
		case REP_SASL:
		case REP_EXTENDED:
			rc = slapi_over_result( op, rs, SLAPI_PLUGIN_POST_RESULT_FN );
			break;
		case REP_SEARCH:
			rc = slapi_over_search( op, rs, SLAPI_PLUGIN_POST_ENTRY_FN );
			break;
		case REP_SEARCHREF:
			rc = slapi_over_search( op, rs, SLAPI_PLUGIN_POST_REFERRAL_FN );
			break;
		default:
			break;
		}
	}

	return rc;
}

static int
slapi_op_func( Operation *op, SlapReply *rs )
{
	Slapi_PBlock		*pb;
	slap_operation_t	which;
	struct slapi_op_info	*opinfo;
	int			rc;
	slap_overinfo		*oi;
	slap_overinst		*on;
	slap_callback		cb;
	int			internal_op;
	int			preop_type, postop_type;
	BackendDB		*be;

	if ( !slapi_plugins_used )
		return SLAP_CB_CONTINUE;

	/*
	 * Find the SLAPI operation information for this LDAP
	 * operation; this will contain the preop and postop
	 * plugin types, as well as optional callbacks for
	 * setting up the SLAPI environment.
	 */
	which = slapi_tag2op( op->o_tag );
	if ( which >= op_last ) {
		/* invalid operation, but let someone else deal with it */
		return SLAP_CB_CONTINUE;
	}

	opinfo = &slapi_op_dispatch_table[which];
	if ( opinfo == NULL ) {
		/* no SLAPI plugin types for this operation */
		return SLAP_CB_CONTINUE;
	}

	internal_op = slapi_op_internal_p( op, rs, &cb );

	if ( internal_op ) {
		preop_type = opinfo->soi_internal_preop;
		postop_type = opinfo->soi_internal_postop;
	} else {
		preop_type = opinfo->soi_preop;
		postop_type = opinfo->soi_postop;
	}

	if ( preop_type == 0 ) {
		/* no SLAPI plugin types for this operation */
		pb = NULL;
		rc = SLAP_CB_CONTINUE;
		goto cleanup;
	}

	pb = SLAPI_OPERATION_PBLOCK( op );

	/* cache backend so we call correct postop plugins */
	be = pb->pb_op->o_bd;

	rc = slapi_int_call_plugins( be, preop_type, pb );

	/*
	 * soi_callback is responsible for examining the result code
	 * of the preoperation plugin and determining whether to
	 * abort. This is needed because of special SLAPI behaviour
	 e with bind preoperation plugins.
	 *
	 * The soi_callback function is also used to reset any values
	 * returned from the preoperation plugin before calling the
	 * backend (for the success case).
	 */
	if ( opinfo->soi_callback == NULL ) {
		/* default behaviour is preop plugin can abort operation */
		if ( rc < 0 ) {
			rc = rs->sr_err;
			goto cleanup;
		}
	} else {
		rc = (opinfo->soi_callback)( op, rs, rc );
		if ( rc )
			goto cleanup;
	}

	/*
	 * Call actual backend (or next overlay in stack). We need to
	 * do this rather than returning SLAP_CB_CONTINUE and calling
	 * postoperation plugins in a response handler to match the
	 * behaviour of SLAPI in OpenLDAP 2.2, where postoperation
	 * plugins are called after the backend has completely
	 * finished processing the operation.
	 */
	on = (slap_overinst *)op->o_bd->bd_info;
	oi = on->on_info;

	rc = overlay_op_walk( op, rs, which, oi, on->on_next );

	/*
	 * Call postoperation plugins
	 */
	slapi_int_call_plugins( be, postop_type, pb );

cleanup:
	if ( !internal_op ) {
		slapi_pblock_destroy(pb);
		cb.sc_private = NULL;
	}

	op->o_callback = cb.sc_next;

	return rc;
}

static int
slapi_over_extended( Operation *op, SlapReply *rs )
{
	Slapi_PBlock	*pb;
	SLAPI_FUNC	callback;
	int		rc;
	int		internal_op;
	slap_callback	cb;

	slapi_int_get_extop_plugin( &op->ore_reqoid, &callback );
	if ( callback == NULL ) {
		return SLAP_CB_CONTINUE;
	}

	internal_op = slapi_op_internal_p( op, rs, &cb );
	if ( internal_op ) {
		return SLAP_CB_CONTINUE;
	}

	pb = SLAPI_OPERATION_PBLOCK( op );

	rc = (*callback)( pb );
	if ( rc == SLAPI_PLUGIN_EXTENDED_SENT_RESULT ) {
		goto cleanup;
	} else if ( rc == SLAPI_PLUGIN_EXTENDED_NOT_HANDLED ) {
		rc = SLAP_CB_CONTINUE;
		goto cleanup;
	}

	assert( rs->sr_rspoid != NULL );

	send_ldap_extended( op, rs );

#if 0
	slapi_ch_free_string( (char **)&rs->sr_rspoid );
#endif

	if ( rs->sr_rspdata != NULL )
		ber_bvfree( rs->sr_rspdata );

	rc = rs->sr_err;

cleanup:
	slapi_pblock_destroy( pb );
	op->o_callback = cb.sc_next;

	return rc;
}

static int
slapi_over_access_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	int			rc;
	Slapi_PBlock		*pb;
	slap_callback		cb;
	int			internal_op;
	SlapReply		rs = { REP_RESULT };

	internal_op = slapi_op_internal_p( op, &rs, &cb );

	cb.sc_response = NULL;
	cb.sc_cleanup = NULL;

	pb = SLAPI_OPERATION_PBLOCK( op );

	rc = slapi_int_access_allowed( op, e, desc, val, access, state );
	if ( rc ) {
		rc = SLAP_CB_CONTINUE;
	}

	if ( !internal_op ) {
		slapi_pblock_destroy( pb );
	}

	op->o_callback = cb.sc_next;

	return rc;
}

static int
slapi_over_acl_group(
	Operation		*op,
	Entry			*target,
	struct berval		*gr_ndn,
	struct berval		*op_ndn,
	ObjectClass		*group_oc,
	AttributeDescription	*group_at )
{
	Slapi_Entry		*e;
	int			rc;
	Slapi_PBlock		*pb;
	BackendDB		*be = op->o_bd;
	GroupAssertion		*g;
	SlapReply		rs = { REP_RESULT };

	op->o_bd = select_backend( gr_ndn, 0 );

	for ( g = op->o_groups; g; g = g->ga_next ) {
		if ( g->ga_be != op->o_bd || g->ga_oc != group_oc ||
			g->ga_at != group_at || g->ga_len != gr_ndn->bv_len )
		{
			continue;
		}
		if ( strcmp( g->ga_ndn, gr_ndn->bv_val ) == 0 ) {
			break;
		}
	}
	if ( g != NULL ) {
		rc = g->ga_res;
		goto done;
	}

	if ( target != NULL && dn_match( &target->e_nname, gr_ndn ) ) {
		e = target;
		rc = 0;
	} else {
		rc = be_entry_get_rw( op, gr_ndn, group_oc, group_at, 0, &e );
	}
	if ( e != NULL ) {
		int			internal_op;
		slap_callback		cb;

		internal_op = slapi_op_internal_p( op, &rs, &cb );

		cb.sc_response = NULL;
		cb.sc_cleanup = NULL;

		pb = SLAPI_OPERATION_PBLOCK( op );

		slapi_pblock_set( pb, SLAPI_X_GROUP_ENTRY,        (void *)e );
		slapi_pblock_set( pb, SLAPI_X_GROUP_OPERATION_DN, (void *)op_ndn->bv_val );
		slapi_pblock_set( pb, SLAPI_X_GROUP_ATTRIBUTE,    (void *)group_at->ad_cname.bv_val );
		slapi_pblock_set( pb, SLAPI_X_GROUP_TARGET_ENTRY, (void *)target );

		rc = slapi_over_call_plugins( pb, SLAPI_X_PLUGIN_PRE_GROUP_FN );
		if ( rc >= 0 ) /* 1 means no plugins called */
			rc = SLAP_CB_CONTINUE;
		else
			rc = pb->pb_rs->sr_err;

		slapi_pblock_delete_param( pb, SLAPI_X_GROUP_ENTRY );
		slapi_pblock_delete_param( pb, SLAPI_X_GROUP_OPERATION_DN );
		slapi_pblock_delete_param( pb, SLAPI_X_GROUP_ATTRIBUTE );
		slapi_pblock_delete_param( pb, SLAPI_X_GROUP_TARGET_ENTRY );

		if ( !internal_op )
			slapi_pblock_destroy( pb );

		if ( e != target ) {
			be_entry_release_r( op, e );
		}

		op->o_callback = cb.sc_next;
	} else {
		rc = LDAP_NO_SUCH_OBJECT; /* return SLAP_CB_CONTINUE for correctness? */
	}

	if ( op->o_tag != LDAP_REQ_BIND && !op->o_do_not_cache &&
	     rc != SLAP_CB_CONTINUE ) {
		g = op->o_tmpalloc( sizeof( GroupAssertion ) + gr_ndn->bv_len,
			op->o_tmpmemctx );
		g->ga_be = op->o_bd;
		g->ga_oc = group_oc;
		g->ga_at = group_at;
		g->ga_res = rc;
		g->ga_len = gr_ndn->bv_len;
		strcpy( g->ga_ndn, gr_ndn->bv_val );
		g->ga_next = op->o_groups;
		op->o_groups = g;
	}
	/*
	 * XXX don't call POST_GROUP_FN, I have no idea what the point of
	 * that plugin function was anyway
	 */
done:
	op->o_bd = be;
	return rc;
}

static int
slapi_over_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	Slapi_PBlock		*pb;
	int			rc;

	pb = slapi_pblock_new();

	rc = slapi_int_call_plugins( be, SLAPI_PLUGIN_START_FN, pb );

	slapi_pblock_destroy( pb );

	return rc;
}

static int
slapi_over_db_close(
	BackendDB	*be,
	ConfigReply	*cr )
{
	Slapi_PBlock		*pb;
	int			rc;

	pb = slapi_pblock_new();

	rc = slapi_int_call_plugins( be, SLAPI_PLUGIN_CLOSE_FN, pb );

	slapi_pblock_destroy( pb );

	return rc;
}

static int
slapi_over_init()
{
	memset( &slapi, 0, sizeof(slapi) );

	slapi.on_bi.bi_type = SLAPI_OVERLAY_NAME;

	slapi.on_bi.bi_op_bind 		= slapi_op_func;
	slapi.on_bi.bi_op_unbind	= slapi_op_func;
	slapi.on_bi.bi_op_search	= slapi_op_func;
	slapi.on_bi.bi_op_compare	= slapi_op_func;
	slapi.on_bi.bi_op_modify	= slapi_op_func;
	slapi.on_bi.bi_op_modrdn	= slapi_op_func;
	slapi.on_bi.bi_op_add		= slapi_op_func;
	slapi.on_bi.bi_op_delete	= slapi_op_func;
	slapi.on_bi.bi_op_abandon	= slapi_op_func;
	slapi.on_bi.bi_op_cancel	= slapi_op_func;

	slapi.on_bi.bi_db_open		= slapi_over_db_open;
	slapi.on_bi.bi_db_close		= slapi_over_db_close;

	slapi.on_bi.bi_extended		= slapi_over_extended;
	slapi.on_bi.bi_access_allowed	= slapi_over_access_allowed;
	slapi.on_bi.bi_operational	= slapi_over_aux_operational;
	slapi.on_bi.bi_acl_group	= slapi_over_acl_group;

	return overlay_register( &slapi );
}

int slapi_over_is_inst( BackendDB *be )
{
	return overlay_is_inst( be, SLAPI_OVERLAY_NAME );
}

int slapi_over_config( BackendDB *be, ConfigReply *cr )
{
	if ( slapi_over_initialized == 0 ) {
		int rc;

		/* do global initializaiton */
		ldap_pvt_thread_mutex_init( &slapi_hn_mutex );
		ldap_pvt_thread_mutex_init( &slapi_time_mutex );
		ldap_pvt_thread_mutex_init( &slapi_printmessage_mutex );

		if ( slapi_log_file == NULL )
			slapi_log_file = slapi_ch_strdup( LDAP_RUNDIR LDAP_DIRSEP "errors" );

		rc = slapi_int_init_object_extensions();
		if ( rc != 0 )
			return rc;

		rc = slapi_over_init();
		if ( rc != 0 )
			return rc;

		slapi_over_initialized = 1;
	}

	return overlay_config( be, SLAPI_OVERLAY_NAME, -1, NULL, cr );
}

#endif /* LDAP_SLAPI */
