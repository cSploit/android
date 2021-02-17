/* op.c - relay backend operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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

#include <stdio.h>

#include "slap.h"
#include "back-relay.h"

/* Results when no real database (.rf_bd) or operation handler (.rf_op) */
static const struct relay_fail_modes_s {
	slap_mask_t	rf_bd, rf_op;
#define RB_ERR_MASK	0x0000FFFFU /* bitmask for default return value */
#define RB_BDERR	0x80000000U /* use .rf_bd's default return value */
#define RB_OPERR	0x40000000U /* set rs->sr_err = .rf_op return value */
#define RB_REF		0x20000000U /* use default_referral if available */
#define RB_SEND		0x10000000U /* send result; RB_??ERR is also set */
#define RB_SENDREF	0/*unused*/ /* like RB_SEND when referral found */
#define RB_NO_BIND	(RB_OPERR | LDAP_INVALID_CREDENTIALS)
#define RB_NOT_SUPP	(RB_OPERR | LDAP_UNWILLING_TO_PERFORM)
#define RB_NO_OBJ	(RB_REF | LDAP_NO_SUCH_OBJECT)
#define RB_CHK_REF	(RB_REF | RB_SENDREF | LDAP_SUCCESS)
} relay_fail_modes[relay_op_last] = {
	/* .rf_bd is unused when zero, otherwise both fields have RB_BDERR */
#	define RB_OP(b, o)	{ (b) | RB_BD2ERR(b), (o) | RB_BD2ERR(b) }
#	define RB_BD2ERR(b)	((b) ? RB_BDERR : 0)
	/* indexed by slap_operation_t: */
	RB_OP(RB_NO_BIND|RB_SEND, RB_NO_BIND  |RB_SEND), /* Bind           */
	RB_OP(0,                  LDAP_SUCCESS),         /* Unbind: unused */
	RB_OP(RB_NO_OBJ |RB_SEND, RB_NOT_SUPP |RB_SEND), /* Search         */
	RB_OP(RB_NO_OBJ |RB_SEND, SLAP_CB_CONTINUE),     /* Compare        */
	RB_OP(RB_NO_OBJ |RB_SEND, RB_NOT_SUPP |RB_SEND), /* Modify         */
	RB_OP(RB_NO_OBJ |RB_SEND, RB_NOT_SUPP |RB_SEND), /* Modrdn         */
	RB_OP(RB_NO_OBJ |RB_SEND, RB_NOT_SUPP |RB_SEND), /* Add            */
	RB_OP(RB_NO_OBJ |RB_SEND, RB_NOT_SUPP |RB_SEND), /* Delete         */
	RB_OP(0,                  LDAP_SUCCESS),         /* Abandon:unused */
	RB_OP(RB_NO_OBJ,          RB_NOT_SUPP),          /* Extended       */
	RB_OP(0,                  SLAP_CB_CONTINUE),     /* Cancel: unused */
	RB_OP(0,                  LDAP_SUCCESS),    /* operational         */
	RB_OP(RB_CHK_REF,         LDAP_SUCCESS),    /* chk_referrals:unused*/
	RB_OP(0,                  SLAP_CB_CONTINUE),/* chk_controls:unused */
	/* additional relay_operation_t indexes from back-relay.h: */
	RB_OP(0,                  0/*unused*/),     /* entry_get = op_last */
	RB_OP(0,                  0/*unused*/),     /* entry_release       */
	RB_OP(0,                  0/*unused*/),     /* has_subordinates    */
};

/*
 * Callbacks: Caller changed op->o_bd from Relay to underlying
 * BackendDB.  sc_response sets it to Relay BackendDB, sc_cleanup puts
 * back underlying BackendDB.  Caller will restore Relay BackendDB.
 */

typedef struct relay_callback {
	slap_callback rcb_sc;
	BackendDB *rcb_bd;
} relay_callback;

static int
relay_back_cleanup_cb( Operation *op, SlapReply *rs )
{
	op->o_bd = ((relay_callback *) op->o_callback)->rcb_bd;
	return SLAP_CB_CONTINUE;
}

static int
relay_back_response_cb( Operation *op, SlapReply *rs )
{
	relay_callback	*rcb = (relay_callback *) op->o_callback;

	rcb->rcb_sc.sc_cleanup = relay_back_cleanup_cb;
	rcb->rcb_bd = op->o_bd;
	op->o_bd = op->o_callback->sc_private;
	return SLAP_CB_CONTINUE;
}

#define relay_back_add_cb( rcb, op ) {				\
		(rcb)->rcb_sc.sc_next = (op)->o_callback;	\
		(rcb)->rcb_sc.sc_response = relay_back_response_cb; \
		(rcb)->rcb_sc.sc_cleanup = 0;			\
		(rcb)->rcb_sc.sc_private = (op)->o_bd;		\
		(op)->o_callback = (slap_callback *) (rcb);	\
}

#define relay_back_remove_cb( rcb, op ) {			\
		slap_callback	**sc = &(op)->o_callback;	\
		for ( ;; sc = &(*sc)->sc_next )			\
			if ( *sc == (slap_callback *) (rcb) ) {	\
				*sc = (*sc)->sc_next; break;	\
			} else if ( *sc == NULL ) break;	\
}

/*
 * Select the backend database with the operation's DN.  On failure,
 * set/send results depending on operation type <which>'s fail_modes.
 */
static BackendDB *
relay_back_select_backend( Operation *op, SlapReply *rs, int which )
{
	OpExtra		*oex;
	char		*key = (char *) op->o_bd->be_private;
	BackendDB	*bd  = ((relay_back_info *) key)->ri_bd;
	slap_mask_t	fail_mode = relay_fail_modes[which].rf_bd;
	int		useDN = 0, rc = ( fail_mode & RB_ERR_MASK );

	if ( bd == NULL && !BER_BVISNULL( &op->o_req_ndn ) ) {
		useDN = 1;
		bd = select_backend( &op->o_req_ndn, 1 );
	}

	if ( bd != NULL ) {
		key += which; /* <relay, op type> key from RELAY_WRAP_OP() */
		LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
			if ( oex->oe_key == key )
				break;
		}
		if ( oex == NULL ) {
			return bd;
		}

		Debug( LDAP_DEBUG_ANY,
			"%s: back-relay for DN=\"%s\" would call self.\n",
			op->o_log_prefix, op->o_req_dn.bv_val, 0 );

	} else if ( useDN && ( fail_mode & RB_REF ) && default_referral ) {
		rc = LDAP_REFERRAL;

		/* if we set sr_err to LDAP_REFERRAL, we must provide one */
		rs->sr_ref = referral_rewrite(
			default_referral, NULL, &op->o_req_dn,
			op->o_tag == LDAP_REQ_SEARCH ?
			op->ors_scope : LDAP_SCOPE_DEFAULT );
		if ( rs->sr_ref != NULL ) {
			rs->sr_flags |= REP_REF_MUSTBEFREED;
		} else {
			rs->sr_ref = default_referral;
		}

		if ( fail_mode & RB_SENDREF )
			fail_mode = (RB_BDERR | RB_SEND);
	}

	if ( fail_mode & RB_BDERR ) {
		rs->sr_err = rc;
		if ( fail_mode & RB_SEND ) {
			send_ldap_result( op, rs );
		}
	}

	return NULL;
}

/*
 * Forward <act> on <op> to database <bd>, with <relay, op type>-specific
 * key in op->o_extra so relay_back_select_backend() can catch recursion.
 */
#define RELAY_WRAP_OP( op, bd, which, act ) { \
	OpExtraDB wrap_oex; \
	BackendDB *const wrap_bd = (op)->o_bd; \
	wrap_oex.oe_db = wrap_bd; \
	wrap_oex.oe.oe_key = (char *) wrap_bd->be_private + (which); \
	LDAP_SLIST_INSERT_HEAD( &(op)->o_extra, &wrap_oex.oe, oe_next ); \
	(op)->o_bd = (bd); \
	act; \
	(op)->o_bd = wrap_bd; \
	LDAP_SLIST_REMOVE( &(op)->o_extra, &wrap_oex.oe, OpExtra, oe_next ); \
}

/*
 * Forward backend function #<which> on <op> to operation DN's database
 * like RELAY_WRAP_OP, after setting up callbacks. If no database or no
 * backend function, set/send results depending on <which>'s fail_modes.
 */
static int
relay_back_op( Operation *op, SlapReply *rs, int which )
{
	BackendDB	*bd;
	BackendInfo	*bi;
	slap_mask_t	fail_mode = relay_fail_modes[which].rf_op;
	int		rc = ( fail_mode & RB_ERR_MASK );

	bd = relay_back_select_backend( op, rs, which );
	if ( bd == NULL ) {
		if ( fail_mode & RB_BDERR )
			return rs->sr_err;	/* sr_err was set above */

	} else if ( (&( bi = bd->bd_info )->bi_op_bind)[which] ) {
		relay_callback	rcb;

		relay_back_add_cb( &rcb, op );
		RELAY_WRAP_OP( op, bd, which, {
			rc = (&bi->bi_op_bind)[which]( op, rs );
		});
		relay_back_remove_cb( &rcb, op );

	} else if ( fail_mode & RB_OPERR ) {
		rs->sr_err = rc;
		if ( rc == LDAP_UNWILLING_TO_PERFORM ) {
			rs->sr_text = "operation not supported within naming context";
		}

		if ( fail_mode & RB_SEND ) {
			send_ldap_result( op, rs );
		}
	}

	return rc;
}


int
relay_back_op_bind( Operation *op, SlapReply *rs )
{
	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		return rs->sr_err;
	}

	return relay_back_op( op, rs, op_bind );
}

#define RELAY_DEFOP(func, which) \
	int func( Operation *op, SlapReply *rs ) \
	{ return relay_back_op( op, rs, which ); }

RELAY_DEFOP( relay_back_op_search,		op_search )
RELAY_DEFOP( relay_back_op_compare,		op_compare )
RELAY_DEFOP( relay_back_op_modify,		op_modify )
RELAY_DEFOP( relay_back_op_modrdn,		op_modrdn )
RELAY_DEFOP( relay_back_op_add,			op_add )
RELAY_DEFOP( relay_back_op_delete,		op_delete )
RELAY_DEFOP( relay_back_op_extended,	op_extended )
RELAY_DEFOP( relay_back_operational,	op_aux_operational )

/* Abandon, Cancel, Unbind and some DN-less calls like be_connection_init
 * need no extra handling:  slapd already calls them for all databases.
 */


int
relay_back_entry_release_rw( Operation *op, Entry *e, int rw )
{
	BackendDB		*bd;
	int			rc = LDAP_UNWILLING_TO_PERFORM;

	bd = relay_back_select_backend( op, NULL, relay_op_entry_release );
	if ( bd && bd->be_release ) {
		RELAY_WRAP_OP( op, bd, relay_op_entry_release, {
			rc = bd->be_release( op, e, rw );
		});
	} else if ( e->e_private == NULL ) {
		entry_free( e );
		rc = LDAP_SUCCESS;
	}

	return rc;
}

int
relay_back_entry_get_rw( Operation *op, struct berval *ndn,
	ObjectClass *oc, AttributeDescription *at, int rw, Entry **e )
{
	BackendDB		*bd;
	int			rc = LDAP_NO_SUCH_OBJECT;

	bd = relay_back_select_backend( op, NULL, relay_op_entry_get );
	if ( bd && bd->be_fetch ) {
		RELAY_WRAP_OP( op, bd, relay_op_entry_get, {
			rc = bd->be_fetch( op, ndn, oc, at, rw, e );
		});
	}

	return rc;
}

#if 0 /* Give the RB_SENDREF flag a nonzero value if implementing this */
/*
 * NOTE: even the existence of this function is questionable: we cannot
 * pass the bi_chk_referrals() call thru the rwm overlay because there
 * is no way to rewrite the req_dn back; but then relay_back_chk_referrals()
 * is passing the target database a DN that likely does not belong to its
 * naming context... mmmh.
 */
RELAY_DEFOP( relay_back_chk_referrals, op_aux_chk_referrals )
#endif /*0*/

int
relay_back_has_subordinates( Operation *op, Entry *e, int *hasSubs )
{
	BackendDB		*bd;
	int			rc = LDAP_OTHER;

	bd = relay_back_select_backend( op, NULL, relay_op_has_subordinates );
	if ( bd && bd->be_has_subordinates ) {
		RELAY_WRAP_OP( op, bd, relay_op_has_subordinates, {
			rc = bd->be_has_subordinates( op, e, hasSubs );
		});
	}

	return rc;
}


/*
 * FIXME: must implement tools as well
 */
