/* null.c - the null backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
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
 * This work was originally developed by Hallvard Furuseth for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"

struct null_info {
	int	ni_bind_allowed;
	ID	ni_nextid;
};

static ConfigDriver null_cf_gen;

static ConfigTable nullcfg[] = {
	{ "bind", "true|FALSE", 1, 2, 0, ARG_ON_OFF|ARG_MAGIC,
		null_cf_gen,
		"( OLcfgDbAt:8.1 NAME 'olcDbBindAllowed' "
		"DESC 'Allow binds to this database' "
		"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs nullocs[] = {
	{ "( OLcfgDbOc:8.1 "
		"NAME 'olcNullConfig' "
		"DESC 'Null backend ocnfiguration' "
		"SUP olcDatabaseConfig "
		"MAY ( olcDbBindAllowed ) )",
		Cft_Database, nullcfg },
	{ NULL, 0, NULL }
};


/* LDAP operations */

static int
null_back_bind( Operation *op, SlapReply *rs )
{
	struct null_info *ni = (struct null_info *) op->o_bd->be_private;

	if ( ni->ni_bind_allowed || be_isroot_pw( op ) ) {
		/* front end will send result on success (0) */
		return LDAP_SUCCESS;
	}

	rs->sr_err = LDAP_INVALID_CREDENTIALS;
	send_ldap_result( op, rs );

	return rs->sr_err;
}


static int
null_back_respond( Operation *op, SlapReply *rs, int rc )
{
	LDAPControl ctrl[SLAP_MAX_RESPONSE_CONTROLS], *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int c = 0;

	BerElementBuffer	ps_berbuf;
	BerElement		*ps_ber = NULL;
	LDAPControl		**preread_ctrl = NULL,
				**postread_ctrl = NULL;

	rs->sr_err = LDAP_OTHER;

	/* this comes first, as in case of assertion failure
	 * any further processing must stop */
	if ( get_assert( op ) ) {
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto respond;
	}

	if ( op->o_preread ) {
		Entry		e = { 0 };

		switch ( op->o_tag ) {
		case LDAP_REQ_MODIFY:
		case LDAP_REQ_RENAME:
		case LDAP_REQ_DELETE:
			e.e_name = op->o_req_dn;
			e.e_nname = op->o_req_ndn;

			preread_ctrl = &ctrls[c];
			*preread_ctrl = NULL;

			if ( slap_read_controls( op, rs, &e,
				&slap_pre_read_bv, preread_ctrl ) )
			{
				preread_ctrl = NULL;

				Debug( LDAP_DEBUG_TRACE,
					"<=- null_back_respond: pre-read "
					"failed!\n", 0, 0, 0 );

				if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
					/* FIXME: is it correct to abort
					 * operation if control fails? */
					goto respond;
				}

			} else {
				c++;
			}
			break;
		}
	}

	if ( op->o_postread ) {
		Entry		e = { 0 };

		switch ( op->o_tag ) {
		case LDAP_REQ_ADD:
		case LDAP_REQ_MODIFY:
		case LDAP_REQ_RENAME:
			if ( op->o_tag == LDAP_REQ_ADD ) {
				e.e_name = op->ora_e->e_name;
				e.e_nname = op->ora_e->e_nname;

			} else {
				e.e_name = op->o_req_dn;
				e.e_nname = op->o_req_ndn;
			}

			postread_ctrl = &ctrls[c];
			*postread_ctrl = NULL;

			if ( slap_read_controls( op, rs, &e,
				&slap_post_read_bv, postread_ctrl ) )
			{
				postread_ctrl = NULL;

				Debug( LDAP_DEBUG_TRACE,
					"<=- null_back_respond: post-read "
					"failed!\n", 0, 0, 0 );

				if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
					/* FIXME: is it correct to abort
					 * operation if control fails? */
					goto respond;
				}

			} else {
				c++;
			}
			break;
		}
	}

	if ( op->o_noop ) {
		switch ( op->o_tag ) {
		case LDAP_REQ_ADD:
		case LDAP_REQ_MODIFY:
		case LDAP_REQ_RENAME:
		case LDAP_REQ_DELETE:
		case LDAP_REQ_EXTENDED:
			rc = LDAP_X_NO_OPERATION;
			break;
		}
	}

	if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
		struct berval		cookie = BER_BVC( "" );

		/* should not be here... */
		assert( op->o_tag == LDAP_REQ_SEARCH );

		ctrl[c].ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
		ctrl[c].ldctl_iscritical = 0;

		ps_ber = (BerElement *)&ps_berbuf;
		ber_init2( ps_ber, NULL, LBER_USE_DER );

		/* return size of 0 -- no estimate */
		ber_printf( ps_ber, "{iO}", 0, &cookie ); 

		if ( ber_flatten2( ps_ber, &ctrl[c].ldctl_value, 0 ) == -1 ) {
			goto done;
		}
		
		ctrls[c] = &ctrl[c];
		c++;
	}

	/* terminate controls array */
	ctrls[c] = NULL;
	rs->sr_ctrls = ctrls;
	rs->sr_err = rc;

respond:;
	send_ldap_result( op, rs );
	rs->sr_ctrls = NULL;

done:;
	if ( ps_ber != NULL ) {
		(void) ber_free_buf( ps_ber );
	}

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}

	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

/* add, delete, modify, modrdn, search */
static int
null_back_success( Operation *op, SlapReply *rs )
{
	return null_back_respond( op, rs, LDAP_SUCCESS );
}

/* compare */
static int
null_back_false( Operation *op, SlapReply *rs )
{
	return null_back_respond( op, rs, LDAP_COMPARE_FALSE );
}


/* for overlays */
static int
null_back_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **ent )
{
	/* don't admit the object isn't there */
	return oc || at ? LDAP_NO_SUCH_ATTRIBUTE : LDAP_BUSY;
}


/* Slap tools */

static int
null_tool_entry_open( BackendDB *be, int mode )
{
	return 0;
}

static int
null_tool_entry_close( BackendDB *be )
{
	assert( be != NULL );
	return 0;
}

static ID
null_tool_entry_first_x( BackendDB *be, struct berval *base, int scope, Filter *f )
{
	return NOID;
}

static ID
null_tool_entry_next( BackendDB *be )
{
	return NOID;
}

static Entry *
null_tool_entry_get( BackendDB *be, ID id )
{
	assert( slapMode & SLAP_TOOL_MODE );
	return NULL;
}

static ID
null_tool_entry_put( BackendDB *be, Entry *e, struct berval *text )
{
	assert( slapMode & SLAP_TOOL_MODE );
	assert( text != NULL );
	assert( text->bv_val != NULL );
	assert( text->bv_val[0] == '\0' );	/* overconservative? */

	e->e_id = ((struct null_info *) be->be_private)->ni_nextid++;
	return e->e_id;
}


/* Setup */

static int
null_cf_gen( ConfigArgs *c )
{
	struct null_info *ni = (struct null_info *) c->be->be_private;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		c->value_int = ni->ni_bind_allowed;
		return LDAP_SUCCESS;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ni->ni_bind_allowed = 0;
		return LDAP_SUCCESS;
	}

	ni->ni_bind_allowed = c->value_int;
	return LDAP_SUCCESS;
}

static int
null_back_db_init( BackendDB *be, ConfigReply *cr )
{
	struct null_info *ni = ch_calloc( 1, sizeof(struct null_info) );
	ni->ni_bind_allowed = 0;
	ni->ni_nextid = 1;
	be->be_private = ni;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;
	return 0;
}

static int
null_back_db_destroy( Backend *be, ConfigReply *cr )
{
	free( be->be_private );
	return 0;
}


int
null_back_initialize( BackendInfo *bi )
{
	static char *controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
		LDAP_CONTROL_PAGEDRESULTS,
		LDAP_CONTROL_SUBENTRIES,
		LDAP_CONTROL_PRE_READ,
		LDAP_CONTROL_POST_READ,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
		NULL
	};

	Debug( LDAP_DEBUG_TRACE,
		"null_back_initialize: initialize null backend\n", 0, 0, 0 );

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_SUBENTRIES |
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = null_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = null_back_db_destroy;

	bi->bi_op_bind = null_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = null_back_success;
	bi->bi_op_compare = null_back_false;
	bi->bi_op_modify = null_back_success;
	bi->bi_op_modrdn = null_back_success;
	bi->bi_op_add = null_back_success;
	bi->bi_op_delete = null_back_success;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_entry_get_rw = null_back_entry_get;

	bi->bi_tool_entry_open = null_tool_entry_open;
	bi->bi_tool_entry_close = null_tool_entry_close;
	bi->bi_tool_entry_first = backend_tool_entry_first;
	bi->bi_tool_entry_first_x = null_tool_entry_first_x;
	bi->bi_tool_entry_next = null_tool_entry_next;
	bi->bi_tool_entry_get = null_tool_entry_get;
	bi->bi_tool_entry_put = null_tool_entry_put;

	bi->bi_cf_ocs = nullocs;
	return config_register_schema( nullcfg, nullocs );
}

#if SLAPD_NULL == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( null )

#endif /* SLAPD_NULL == SLAPD_MOD_DYNAMIC */
