/* pguid.c - Parent GUID value overlay */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 Pierangelo Masarati.
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
 * This work was initially developed by Pierangelo Masarati
 * for inclusion in OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_PGUID

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "slap.h"
#include "config.h"

#include "lutil.h"

/*
 * Maintain an attribute (parentUUID) that contains the value
 * of the entryUUID of the parent entry (used by Samba4)
 */

static AttributeDescription	*ad_parentUUID;

static slap_overinst 		pguid;

static int
pguid_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;

	struct berval pdn, pndn;
	Entry *e = NULL;
	Attribute *a;
	int rc;

	/* don't care about suffix entry */
	if ( dn_match( &op->o_req_ndn, &op->o_bd->be_nsuffix[0] ) ) {
		return SLAP_CB_CONTINUE;
	}

	dnParent( &op->o_req_dn, &pdn );
	dnParent( &op->o_req_ndn, &pndn );

	rc = overlay_entry_get_ov( op, &pndn, NULL, slap_schema.si_ad_entryUUID, 0, &e, on );
	if ( rc != LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_op_add: unable to get parent entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, pdn.bv_val, rc );
		return SLAP_CB_CONTINUE;
	}

	a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
	if ( a == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_op_add: unable to find entryUUID of parent entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, pdn.bv_val, rc );
		
	} else {
		assert( a->a_numvals == 1 );

		if ( op->ora_e != NULL ) {
			attr_merge_one( op->ora_e, ad_parentUUID, &a->a_vals[0], a->a_nvals == a->a_vals ? NULL : &a->a_nvals[0] );
		
		} else {
			Modifications *ml;
			Modifications *mod;

			assert( op->ora_modlist != NULL );

			for ( ml = op->ora_modlist; ml != NULL; ml = ml->sml_next ) {
				if ( ml->sml_mod.sm_desc == slap_schema.si_ad_entryUUID ) {
					break;
				}
			}

			if ( ml == NULL ) {
				ml = op->ora_modlist;
			}

			mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
			mod->sml_flags = SLAP_MOD_INTERNAL;
			mod->sml_op = LDAP_MOD_ADD;
			mod->sml_desc = ad_parentUUID;
			mod->sml_type = ad_parentUUID->ad_cname;
			mod->sml_values = ch_malloc( sizeof( struct berval ) * 2 );
			mod->sml_nvalues = NULL;
			mod->sml_numvals = 1;

			ber_dupbv( &mod->sml_values[0], &a->a_vals[0] );
			BER_BVZERO( &mod->sml_values[1] );

			mod->sml_next = ml->sml_next;
			ml->sml_next = mod;
		}
	}

	if ( e != NULL ) {
		(void)overlay_entry_release_ov( op, e, 0, on );
	}

	return SLAP_CB_CONTINUE;
}

static int
pguid_op_rename( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;

	Entry *e = NULL;
	Attribute *a;
	int rc;

	if ( op->orr_nnewSup == NULL ) {
		return SLAP_CB_CONTINUE;
	}

	rc = overlay_entry_get_ov( op, op->orr_nnewSup, NULL, slap_schema.si_ad_entryUUID, 0, &e, on );
	if ( rc != LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_op_rename: unable to get newSuperior entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, op->orr_newSup->bv_val, rc );
		return SLAP_CB_CONTINUE;
	}

	a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
	if ( a == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_op_rename: unable to find entryUUID of newSuperior entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, op->orr_newSup->bv_val, rc );
		
	} else {
		Modifications *mod;

		assert( a->a_numvals == 1 );

		mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
		mod->sml_flags = SLAP_MOD_INTERNAL;
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_desc = ad_parentUUID;
		mod->sml_type = ad_parentUUID->ad_cname;
		mod->sml_values = ch_malloc( sizeof( struct berval ) * 2 );
		mod->sml_nvalues = NULL;
		mod->sml_numvals = 1;

		ber_dupbv( &mod->sml_values[0], &a->a_vals[0] );
		BER_BVZERO( &mod->sml_values[1] );

		mod->sml_next = op->orr_modlist;
		op->orr_modlist = mod;
	}

	if ( e != NULL ) {
		(void)overlay_entry_release_ov( op, e, 0, on );
	}

	return SLAP_CB_CONTINUE;
}

static int
pguid_db_init(
	BackendDB	*be,
	ConfigReply	*cr)
{
	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"pguid_db_init: pguid cannot be used as global overlay.\n" );
		return 1;
	}

	if ( be->be_nsuffix == NULL ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"pguid_db_init: database must have suffix\n" );
		return 1;
	}

	if ( BER_BVISNULL( &be->be_rootndn ) || BER_BVISEMPTY( &be->be_rootndn ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"pguid_db_init: missing rootdn for database DN=\"%s\", YMMV\n",
			be->be_suffix[ 0 ].bv_val );
	}

	return 0;
}

typedef struct pguid_mod_t {
	struct berval ndn;
	struct berval pguid;
	struct pguid_mod_t *next;
} pguid_mod_t;

typedef struct {
	slap_overinst *on;
	pguid_mod_t *mods;
} pguid_repair_cb_t;

static int
pguid_repair_cb( Operation *op, SlapReply *rs )
{
	int rc;
	pguid_repair_cb_t *pcb = op->o_callback->sc_private;
	Entry *e = NULL;
	Attribute *a;
	struct berval pdn, pndn;

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		break;

	case REP_SEARCHREF:
	case REP_RESULT:
		return rs->sr_err;

	default:
		assert( 0 );
	}

	assert( rs->sr_entry != NULL );

	dnParent( &rs->sr_entry->e_name, &pdn );
	dnParent( &rs->sr_entry->e_nname, &pndn );

	rc = overlay_entry_get_ov( op, &pndn, NULL, slap_schema.si_ad_entryUUID, 0, &e, pcb->on );
	if ( rc != LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_repair_cb: unable to get parent entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, pdn.bv_val, rc );
		return 0;
	}

	a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
	if ( a == NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: pguid_repair_cb: unable to find entryUUID of parent entry DN=\"%s\" (%d)\n",
			op->o_log_prefix, pdn.bv_val, rc );
		
	} else {
		ber_len_t len;
		pguid_mod_t *mod;

		assert( a->a_numvals == 1 );

		len = sizeof( pguid_mod_t ) + rs->sr_entry->e_nname.bv_len + 1 + a->a_vals[0].bv_len + 1;
		mod = op->o_tmpalloc( len, op->o_tmpmemctx );
		mod->ndn.bv_len = rs->sr_entry->e_nname.bv_len;
		mod->ndn.bv_val = (char *)&mod[1];
		mod->pguid.bv_len = a->a_vals[0].bv_len;
		mod->pguid.bv_val = (char *)&mod->ndn.bv_val[mod->ndn.bv_len + 1];
		lutil_strncopy( mod->ndn.bv_val, rs->sr_entry->e_nname.bv_val, rs->sr_entry->e_nname.bv_len );
		lutil_strncopy( mod->pguid.bv_val, a->a_vals[0].bv_val, a->a_vals[0].bv_len );

		mod->next = pcb->mods;
		pcb->mods = mod;

		Debug( LDAP_DEBUG_TRACE, "%s: pguid_repair_cb: scheduling entry DN=\"%s\" for repair\n",
			op->o_log_prefix, rs->sr_entry->e_name.bv_val, 0 );
	}

	if ( e != NULL ) {
		(void)overlay_entry_release_ov( op, e, 0, pcb->on );
	}

	return 0;
}

static int
pguid_repair( BackendDB *be )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	void *ctx = ldap_pvt_thread_pool_context();
	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	slap_callback sc = { 0 };
	pguid_repair_cb_t pcb = { 0 };
	SlapReply rs = { REP_RESULT };
	pguid_mod_t *pmod;
	int nrepaired = 0;

	connection_fake_init2( &conn, &opbuf, ctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;
	memset( &op->oq_search, 0, sizeof( op->oq_search ) );

	op->o_bd = select_backend( &be->be_nsuffix[ 0 ], 0 );

	op->o_req_dn = op->o_bd->be_suffix[ 0 ];
	op->o_req_ndn = op->o_bd->be_nsuffix[ 0 ];

	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	op->ors_scope = LDAP_SCOPE_SUBORDINATE;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_attrs = slap_anlist_no_attrs;

	op->ors_filterstr.bv_len = STRLENOF( "(!(=*))" ) + ad_parentUUID->ad_cname.bv_len;
	op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	snprintf( op->ors_filterstr.bv_val, op->ors_filterstr.bv_len + 1,
		"(!(%s=*))", ad_parentUUID->ad_cname.bv_val );

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( op->ors_filter == NULL ) {
		rs.sr_err = LDAP_OTHER;
		goto done_search;
	}
	
	op->o_callback = &sc;
	sc.sc_response = pguid_repair_cb;
	sc.sc_private = &pcb;
	pcb.on = on;

	(void)op->o_bd->bd_info->bi_op_search( op, &rs );

	op->o_tag = LDAP_REQ_MODIFY;
	sc.sc_response = slap_null_cb;
	sc.sc_private = NULL;
	memset( &op->oq_modify, 0, sizeof( req_modify_s ) );

	for ( pmod = pcb.mods; pmod != NULL; ) {
		pguid_mod_t *pnext;

		Modifications *mod;
		SlapReply rs2 = { REP_RESULT };

		mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
		mod->sml_flags = SLAP_MOD_INTERNAL;
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_desc = ad_parentUUID;
		mod->sml_type = ad_parentUUID->ad_cname;
		mod->sml_values = ch_malloc( sizeof( struct berval ) * 2 );
		mod->sml_nvalues = NULL;
		mod->sml_numvals = 1;
		mod->sml_next = NULL;

		ber_dupbv( &mod->sml_values[0], &pmod->pguid );
		BER_BVZERO( &mod->sml_values[1] );

		op->o_req_dn = pmod->ndn;
		op->o_req_ndn = pmod->ndn;

		op->orm_modlist = mod;
		op->o_bd->be_modify( op, &rs2 );
		slap_mods_free( op->orm_modlist, 1 );
		if ( rs2.sr_err == LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "%s: pguid_repair: entry DN=\"%s\" repaired\n",
				op->o_log_prefix, pmod->ndn.bv_val, 0 );
			nrepaired++;

		} else {
			Debug( LDAP_DEBUG_ANY, "%s: pguid_repair: entry DN=\"%s\" repair failed (%d)\n",
				op->o_log_prefix, pmod->ndn.bv_val, rs2.sr_err );
		}

		pnext = pmod->next;
		op->o_tmpfree( pmod, op->o_tmpmemctx );
		pmod = pnext;
	}

done_search:;
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	filter_free_x( op, op->ors_filter, 1 );

	Log1( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
		"pguid: repaired=%d\n", nrepaired );

	return rs.sr_err;
}

/* search all entries without parentUUID; "repair" them */
static int
pguid_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	if ( SLAP_SINGLE_SHADOW( be ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"pguid incompatible with shadow database \"%s\".\n",
			be->be_suffix[ 0 ].bv_val );
		return 1;
	}

	pguid_repair( be );

	return 0;
}

static struct {
	char	*desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.3.6.1.4.1.4203.666.1.59 "
		"NAME 'parentUUID' "
		"DESC 'the value of the entryUUID of the parent' "
		"EQUALITY UUIDMatch "
		"ORDERING UUIDOrderingMatch "
		"SYNTAX 1.3.6.1.1.16.1 "
		"USAGE dSAOperation "
		"SINGLE-VALUE "
		"NO-USER-MODIFICATION " 
		")",
		&ad_parentUUID },
	{ NULL }
};

int
pguid_initialize(void)
{
	int code, i;

	for ( i = 0; as[ i ].desc != NULL; i++ ) {
		code = register_at( as[ i ].desc, as[ i ].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"pguid_initialize: register_at #%d failed\n",
				i, 0, 0 );
			return code;
		}

		/* Allow Manager to set these as needed */
		if ( is_at_no_user_mod( (*as[ i ].adp)->ad_type ) ) {
			(*as[ i ].adp)->ad_type->sat_flags |=
				SLAP_AT_MANAGEABLE;
		}
	}

	pguid.on_bi.bi_type = "pguid";

	pguid.on_bi.bi_op_add = pguid_op_add;
	pguid.on_bi.bi_op_modrdn = pguid_op_rename;

	pguid.on_bi.bi_db_init = pguid_db_init;
	pguid.on_bi.bi_db_open = pguid_db_open;

	return overlay_register( &pguid );
}

#if SLAPD_OVER_PGUID == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return pguid_initialize();
}
#endif /* SLAPD_OVER_PGUID == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_PGUID */
