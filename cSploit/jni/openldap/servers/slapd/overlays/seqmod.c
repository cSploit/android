/* seqmod.c - sequenced modifies */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_SEQMOD

#include "slap.h"
#include "config.h"

/* This overlay serializes concurrent attempts to modify a single entry */

typedef struct modtarget {
	struct modtarget *mt_next;
	struct modtarget *mt_tail;
	Operation *mt_op;
} modtarget;

typedef struct seqmod_info {
	Avlnode		*sm_mods;	/* entries being modified */
	ldap_pvt_thread_mutex_t	sm_mutex;
} seqmod_info;

static int
sm_avl_cmp( const void *c1, const void *c2 )
{
	const modtarget *m1, *m2;
	int rc;

	m1 = c1; m2 = c2;
	rc = m1->mt_op->o_req_ndn.bv_len - m2->mt_op->o_req_ndn.bv_len;

	if ( rc ) return rc;
	return ber_bvcmp( &m1->mt_op->o_req_ndn, &m2->mt_op->o_req_ndn );
}

static int
seqmod_op_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	seqmod_info *sm = sc->sc_private;
	modtarget *mt, mtdummy;
	Avlnode	 *av;

	mtdummy.mt_op = op;
	/* This op is done, remove it */
	ldap_pvt_thread_mutex_lock( &sm->sm_mutex );
	av = avl_find2( sm->sm_mods, &mtdummy, sm_avl_cmp );
	assert(av != NULL);

	mt = av->avl_data;

	/* If there are more, promote the next one */
	if ( mt->mt_next ) {
		av->avl_data = mt->mt_next;
		mt->mt_next->mt_tail = mt->mt_tail;
	} else {
		avl_delete( &sm->sm_mods, mt, sm_avl_cmp );
	}
	ldap_pvt_thread_mutex_unlock( &sm->sm_mutex );
	op->o_callback = sc->sc_next;
	op->o_tmpfree( sc, op->o_tmpmemctx );

	return 0;
}

static int
seqmod_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	seqmod_info		*sm = on->on_bi.bi_private;
	modtarget	*mt;
	Avlnode	*av;
	slap_callback *cb;

	cb = op->o_tmpcalloc( 1, sizeof(slap_callback) + sizeof(modtarget),
		op->o_tmpmemctx );
	mt = (modtarget *)(cb+1);
	mt->mt_next = NULL;
	mt->mt_tail = mt;
	mt->mt_op = op;

	/* See if we're already modifying this entry - don't allow
	 * near-simultaneous mods of the same entry
	 */
	ldap_pvt_thread_mutex_lock( &sm->sm_mutex );
	av = avl_find2( sm->sm_mods, mt, sm_avl_cmp );
	if ( av ) {
		modtarget *mtp = av->avl_data;
		mtp->mt_tail->mt_next = mt;
		mtp->mt_tail = mt;
		/* Wait for this op to get to head of list */
		while ( mtp != mt ) {
			ldap_pvt_thread_mutex_unlock( &sm->sm_mutex );
			ldap_pvt_thread_yield();
			/* Let it finish - should use a condition
			 * variable here... */
			ldap_pvt_thread_mutex_lock( &sm->sm_mutex );
			mtp = av->avl_data;
		}
	} else {
		/* Record that we're modifying this now */
		avl_insert( &sm->sm_mods, mt, sm_avl_cmp, avl_dup_error );
	}
	ldap_pvt_thread_mutex_unlock( &sm->sm_mutex );

	cb->sc_cleanup = seqmod_op_cleanup;
	cb->sc_private = sm;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

	return SLAP_CB_CONTINUE;
}

static int
seqmod_op_extended(
	Operation *op,
	SlapReply *rs
)
{
	if ( exop_is_write( op )) return seqmod_op_mod( op, rs );
	else return SLAP_CB_CONTINUE;
}

static int
seqmod_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	seqmod_info	*sm;

	sm = ch_calloc(1, sizeof(seqmod_info));
	on->on_bi.bi_private = sm;

	ldap_pvt_thread_mutex_init( &sm->sm_mutex );

	return 0;
}

static int
seqmod_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	seqmod_info	*sm = (seqmod_info *)on->on_bi.bi_private;

	if ( sm ) {
		ldap_pvt_thread_mutex_destroy( &sm->sm_mutex );

		ch_free( sm );
	}

	return 0;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst 		seqmod;

int
seqmod_initialize()
{
	seqmod.on_bi.bi_type = "seqmod";
	seqmod.on_bi.bi_db_open = seqmod_db_open;
	seqmod.on_bi.bi_db_close = seqmod_db_close;

	seqmod.on_bi.bi_op_modify = seqmod_op_mod;
	seqmod.on_bi.bi_op_modrdn = seqmod_op_mod;
	seqmod.on_bi.bi_extended = seqmod_op_extended;

	return overlay_register( &seqmod );
}

#if SLAPD_OVER_SEQMOD == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return seqmod_initialize();
}
#endif /* SLAPD_OVER_SEQMOD == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_SEQMOD) */
