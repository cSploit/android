/* noopsrch.c - LDAP Control that counts entries a search would return */
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

#include "portable.h"

/* define SLAPD_OVER_NOOPSRCH=2 to build as run-time loadable module */
#ifdef SLAPD_OVER_NOOPSRCH

/*
 * Control OID
 */
#define	LDAP_CONTROL_X_NOOPSRCH		"1.3.6.1.4.1.4203.666.5.18"

#include "slap.h"
#include "ac/string.h"

#define o_noopsrch			o_ctrlflag[noopsrch_cid]
#define o_ctrlnoopsrch		o_controls[noopsrch_cid]

static int noopsrch_cid;
static slap_overinst noopsrch;

static int
noopsrch_parseCtrl (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_noopsrch != SLAP_CONTROL_NONE ) {
		rs->sr_text = "No-op Search control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "No-op Search control value is present";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_ctrlnoopsrch = (void *)NULL;

	op->o_noopsrch = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	rs->sr_err = LDAP_SUCCESS;

	return rs->sr_err;
}

int dummy;

typedef struct noopsrch_cb_t {
	slap_overinst	*nc_on;
	ber_int_t		nc_nentries;
	ber_int_t		nc_nsearchref;
	AttributeName	*nc_save_attrs;
	int				*nc_pdummy;
	int				nc_save_slimit;
} noopsrch_cb_t;

static int
noopsrch_response( Operation *op, SlapReply *rs )
{
	noopsrch_cb_t		*nc = (noopsrch_cb_t *)op->o_callback->sc_private;

	/* if the control is global, limits are not computed yet  */
	if ( nc->nc_pdummy == &dummy ) {	
		nc->nc_save_slimit = op->ors_slimit;
		op->ors_slimit = SLAP_NO_LIMIT;
		nc->nc_pdummy = NULL;
	}

	if ( rs->sr_type == REP_SEARCH ) {
		nc->nc_nentries++;
#ifdef NOOPSRCH_DEBUG
		Debug( LDAP_DEBUG_TRACE, "noopsrch_response(REP_SEARCH): nentries=%d\n", nc->nc_nentries, 0, 0 );
#endif
		return 0;

	} else if ( rs->sr_type == REP_SEARCHREF ) {
		nc->nc_nsearchref++;
		return 0;

	} else if ( rs->sr_type == REP_RESULT ) {
		BerElementBuffer	berbuf;
		BerElement			*ber = (BerElement *) &berbuf;
		struct berval		ctrlval;
		LDAPControl			*ctrl, *ctrlsp[2];
		int					rc = rs->sr_err;

		if ( nc->nc_save_slimit >= 0 && nc->nc_nentries >= nc->nc_save_slimit ) {
			rc = LDAP_SIZELIMIT_EXCEEDED;
		}

#ifdef NOOPSRCH_DEBUG
		Debug( LDAP_DEBUG_TRACE, "noopsrch_response(REP_RESULT): err=%d nentries=%d nref=%d\n", rc, nc->nc_nentries, nc->nc_nsearchref );
#endif

		ber_init2( ber, NULL, LBER_USE_DER );

		ber_printf( ber, "{iii}", rc, nc->nc_nentries, nc->nc_nsearchref );
		if ( ber_flatten2( ber, &ctrlval, 0 ) == -1 ) {
			ber_free_buf( ber );
			if ( op->o_noopsrch == SLAP_CONTROL_CRITICAL ) {
				return LDAP_CONSTRAINT_VIOLATION;
			}
			return SLAP_CB_CONTINUE;
		}

		ctrl = op->o_tmpcalloc( 1,
			sizeof( LDAPControl ) + ctrlval.bv_len + 1,
			op->o_tmpmemctx );
		ctrl->ldctl_value.bv_val = (char *)&ctrl[ 1 ];
		ctrl->ldctl_oid = LDAP_CONTROL_X_NOOPSRCH;
		ctrl->ldctl_iscritical = 0;
		ctrl->ldctl_value.bv_len = ctrlval.bv_len;
		AC_MEMCPY( ctrl->ldctl_value.bv_val, ctrlval.bv_val, ctrlval.bv_len );
		ctrl->ldctl_value.bv_val[ ctrl->ldctl_value.bv_len ] = '\0';

		ber_free_buf( ber );

		ctrlsp[0] = ctrl;
		ctrlsp[1] = NULL;
		slap_add_ctrls( op, rs, ctrlsp );

		return SLAP_CB_CONTINUE;
	}
}

static int
noopsrch_cleanup( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_RESULT || rs->sr_err == SLAPD_ABANDON ) {
		noopsrch_cb_t		*nc = (noopsrch_cb_t *)op->o_callback->sc_private;
		op->ors_attrs = nc->nc_save_attrs;
		if ( nc->nc_pdummy == NULL ) {
			op->ors_slimit = nc->nc_save_slimit;
		}

		op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
		op->o_callback = NULL;
	}

	return SLAP_CB_CONTINUE;
}

static int
noopsrch_op_search( Operation *op, SlapReply *rs )
{
	if ( op->o_noopsrch != SLAP_CONTROL_NONE ) {
		slap_callback *sc;
		noopsrch_cb_t *nc;

		sc = op->o_tmpcalloc( 1, sizeof( slap_callback ) + sizeof( noopsrch_cb_t ), op->o_tmpmemctx );

		nc = (noopsrch_cb_t *)&sc[ 1 ];
		nc->nc_on = (slap_overinst *)op->o_bd->bd_info;
		nc->nc_nentries = 0;
		nc->nc_nsearchref = 0;
		nc->nc_save_attrs = op->ors_attrs;
		nc->nc_pdummy = &dummy;

		sc->sc_response = noopsrch_response;
		sc->sc_cleanup = noopsrch_cleanup;
		sc->sc_private = (void *)nc;

		op->ors_attrs = slap_anlist_no_attrs;

		sc->sc_next = op->o_callback->sc_next;
                op->o_callback->sc_next = sc;
	}
	
	return SLAP_CB_CONTINUE;
}

static int noopsrch_cnt;

static int
noopsrch_db_init( BackendDB *be, ConfigReply *cr)
{
	if ( noopsrch_cnt++ == 0 ) {
		int rc;

		rc = register_supported_control( LDAP_CONTROL_X_NOOPSRCH,
			SLAP_CTRL_SEARCH | SLAP_CTRL_GLOBAL_SEARCH, NULL,
			noopsrch_parseCtrl, &noopsrch_cid );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"noopsrch_initialize: Failed to register control '%s' (%d)\n",
				LDAP_CONTROL_X_NOOPSRCH, rc, 0 );
			return rc;
		}
	}

	return LDAP_SUCCESS;
}

static int
noopsrch_db_destroy( BackendDB *be, ConfigReply *cr )
{
	assert( noopsrch_cnt > 0 );

#ifdef SLAP_CONFIG_DELETE
	overlay_unregister_control( be, LDAP_CONTROL_X_NOOPSRCH );
	if ( --noopsrch_cnt == 0 ) {
		unregister_supported_control( LDAP_CONTROL_X_NOOPSRCH );
	}

#endif /* SLAP_CONFIG_DELETE */

	return 0;
}

#if SLAPD_OVER_NOOPSRCH == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_NOOPSRCH == SLAPD_MOD_DYNAMIC */
int
noopsrch_initialize( void )
{

	noopsrch.on_bi.bi_type = "noopsrch";

	noopsrch.on_bi.bi_db_init = noopsrch_db_init;
	noopsrch.on_bi.bi_db_destroy = noopsrch_db_destroy;
	noopsrch.on_bi.bi_op_search = noopsrch_op_search;

	return overlay_register( &noopsrch );
}

#if SLAPD_OVER_NOOPSRCH == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return noopsrch_initialize();
}
#endif /* SLAPD_OVER_NOOPSRCH == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_NOOPSRCH */
