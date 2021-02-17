/* ctxcsn.c -- Context CSN Management Routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

const struct berval slap_ldapsync_bv = BER_BVC("ldapsync");
const struct berval slap_ldapsync_cn_bv = BER_BVC("cn=ldapsync");
int slap_serverID;

/* maxcsn->bv_val must point to a char buf[LDAP_PVT_CSNSTR_BUFSIZE] */
void
slap_get_commit_csn(
	Operation *op,
	struct berval *maxcsn,
	int *foundit
)
{
	struct slap_csn_entry *csne, *committed_csne = NULL;
	BackendDB *be = op->o_bd->bd_self;
	int sid = -1;

	if ( maxcsn ) {
		assert( maxcsn->bv_val != NULL );
		assert( maxcsn->bv_len >= LDAP_PVT_CSNSTR_BUFSIZE );
	}
	if ( foundit ) {
		*foundit = 0;
	}

	if ( !BER_BVISEMPTY( &op->o_csn )) {
		sid = slap_parse_csn_sid( &op->o_csn );
	}

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			csne->ce_state = SLAP_CSN_COMMIT;
			if ( foundit ) *foundit = 1;
			break;
		}
	}

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( sid != -1 && sid == csne->ce_sid ) {
			if ( csne->ce_state == SLAP_CSN_COMMIT ) committed_csne = csne;
			if ( csne->ce_state == SLAP_CSN_PENDING ) break;
		}
	}

	if ( maxcsn ) {
		if ( committed_csne ) {
			if ( committed_csne->ce_csn.bv_len < maxcsn->bv_len )
				maxcsn->bv_len = committed_csne->ce_csn.bv_len;
			AC_MEMCPY( maxcsn->bv_val, committed_csne->ce_csn.bv_val,
				maxcsn->bv_len+1 );
		} else {
			maxcsn->bv_len = 0;
			maxcsn->bv_val[0] = 0;
		}
	}
	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
}

void
slap_rewind_commit_csn( Operation *op )
{
	struct slap_csn_entry *csne;
	BackendDB *be = op->o_bd->bd_self;

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			csne->ce_state = SLAP_CSN_PENDING;
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
}

void
slap_graduate_commit_csn( Operation *op )
{
	struct slap_csn_entry *csne;
	BackendDB *be;

	if ( op == NULL ) return;
	if ( op->o_bd == NULL ) return;
	be = op->o_bd->bd_self;

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			LDAP_TAILQ_REMOVE( be->be_pending_csn_list,
				csne, ce_csn_link );
			Debug( LDAP_DEBUG_SYNC, "slap_graduate_commit_csn: removing %p %s\n",
				csne->ce_csn.bv_val, csne->ce_csn.bv_val, 0 );
			if ( op->o_csn.bv_val == csne->ce_csn.bv_val ) {
				BER_BVZERO( &op->o_csn );
			}
			ch_free( csne->ce_csn.bv_val );
			ch_free( csne );
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );

	return;
}

static struct berval ocbva[] = {
	BER_BVC("top"),
	BER_BVC("subentry"),
	BER_BVC("syncProviderSubentry"),
	BER_BVNULL
};

Entry *
slap_create_context_csn_entry(
	Backend *be,
	struct berval *context_csn )
{
	Entry* e;

	struct berval bv;

	e = entry_alloc();

	attr_merge( e, slap_schema.si_ad_objectClass,
		ocbva, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
		&ocbva[1], NULL );
	attr_merge_one( e, slap_schema.si_ad_cn,
		(struct berval *)&slap_ldapsync_bv, NULL );

	if ( context_csn ) {
		attr_merge_one( e, slap_schema.si_ad_contextCSN,
			context_csn, NULL );
	}

	BER_BVSTR( &bv, "{}" );
	attr_merge_one( e, slap_schema.si_ad_subtreeSpecification, &bv, NULL );

	build_new_dn( &e->e_name, &be->be_nsuffix[0],
		(struct berval *)&slap_ldapsync_cn_bv, NULL );
	ber_dupbv( &e->e_nname, &e->e_name );

	return e;
}

void
slap_queue_csn(
	Operation *op,
	struct berval *csn )
{
	struct slap_csn_entry *pending;
	BackendDB *be = op->o_bd->bd_self;

	pending = (struct slap_csn_entry *) ch_calloc( 1,
			sizeof( struct slap_csn_entry ));

	Debug( LDAP_DEBUG_SYNC, "slap_queue_csn: queing %p %s\n", csn->bv_val, csn->bv_val, 0 );

	ber_dupbv( &pending->ce_csn, csn );
	ber_bvreplace_x( &op->o_csn, &pending->ce_csn, op->o_tmpmemctx );
	pending->ce_sid = slap_parse_csn_sid( csn );
	pending->ce_connid = op->o_connid;
	pending->ce_opid = op->o_opid;
	pending->ce_state = SLAP_CSN_PENDING;

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );
	LDAP_TAILQ_INSERT_TAIL( be->be_pending_csn_list,
		pending, ce_csn_link );
	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
}

int
slap_get_csn(
	Operation *op,
	struct berval *csn,
	int manage_ctxcsn )
{
	if ( csn == NULL ) return LDAP_OTHER;

	csn->bv_len = ldap_pvt_csnstr( csn->bv_val, csn->bv_len, slap_serverID, 0 );
	if ( manage_ctxcsn )
		slap_queue_csn( op, csn );

	return LDAP_SUCCESS;
}
