/* vernum.c - RDN value overlay */
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

#ifdef SLAPD_OVER_VERNUM

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "slap.h"
#include "config.h"

#include "lutil.h"

/*
 * Maintain an attribute (e.g. msDS-KeyVersionNumber) that consists
 * in a counter of modifications of another attribute (e.g. unicodePwd).
 */

typedef struct vernum_t {
	AttributeDescription	*vn_attr;
	AttributeDescription	*vn_vernum;
} vernum_t;

static AttributeDescription	*ad_msDS_KeyVersionNumber;

static struct berval		val_init = BER_BVC( "0" );
static slap_overinst 		vernum;

static int
vernum_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *) op->o_bd->bd_info;
	vernum_t	*vn = (vernum_t *)on->on_bi.bi_private;

	Attribute *a, **ap;
	int rc;

	/* NOTE: should we accept an entry still in mods format? */
	assert( op->ora_e != NULL );

	if ( BER_BVISEMPTY( &op->ora_e->e_nname ) ) {
		return SLAP_CB_CONTINUE;
	}

	a = attr_find( op->ora_e->e_attrs, vn->vn_attr );
	if ( a == NULL ) {
		return SLAP_CB_CONTINUE;
	}

	if ( attr_find( op->ora_e->e_attrs, vn->vn_vernum ) != NULL ) {
		/* already present - leave it alone */
		return SLAP_CB_CONTINUE;
	}

	a = attr_alloc( vn->vn_vernum );

	value_add_one( &a->a_vals, &val_init );
	a->a_nvals = a->a_vals;
	a->a_numvals = 1;

	for ( ap = &op->ora_e->e_attrs; *ap != NULL; ap = &(*ap)->a_next )
		/* goto tail */ ;

	*ap = a;

	return SLAP_CB_CONTINUE;
}

static int
vernum_op_modify( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *) op->o_bd->bd_info;
	vernum_t	*vn = (vernum_t *)on->on_bi.bi_private;

	Modifications *ml, **mlp;
	struct berval val = BER_BVC( "1" );
	int rc;
	unsigned got = 0;

	for ( ml = op->orm_modlist; ml != NULL; ml = ml->sml_next ) {
		if ( ml->sml_desc == vn->vn_vernum ) {
			/* already present - leave it alone
			 * (or should we increment it anyway?) */
			return SLAP_CB_CONTINUE;
		}

		if ( ml->sml_desc == vn->vn_attr ) {
			got = 1;
		}
	}

	if ( !got ) {
		return SLAP_CB_CONTINUE;
	}

	for ( mlp = &op->orm_modlist; *mlp != NULL; mlp = &(*mlp)->sml_next )
		/* goto tail */ ;

	/* ITS#6561 */
#ifdef SLAP_MOD_ADD_IF_NOT_PRESENT
	/* the initial value is only added if the vernum attr is not present */
	ml = SLAP_CALLOC( sizeof( Modifications ), 1 );
	ml->sml_values = SLAP_CALLOC( sizeof( struct berval ) , 2 );
	value_add_one( &ml->sml_values, &val_init );
	ml->sml_nvalues = NULL;
	ml->sml_numvals = 1;
	ml->sml_op = SLAP_MOD_ADD_IF_NOT_PRESENT;
	ml->sml_flags = SLAP_MOD_INTERNAL;
	ml->sml_desc = vn->vn_vernum;
	ml->sml_type = vn->vn_vernum->ad_cname;

	*mlp = ml;
	mlp = &ml->sml_next;
#endif /* SLAP_MOD_ADD_IF_NOT_PRESENT */

	/* this increments by 1 the vernum attr */
	ml = SLAP_CALLOC( sizeof( Modifications ), 1 );
	ml->sml_values = SLAP_CALLOC( sizeof( struct berval ) , 2 );
	value_add_one( &ml->sml_values, &val );
	ml->sml_nvalues = NULL;
	ml->sml_numvals = 1;
	ml->sml_op = LDAP_MOD_INCREMENT;
	ml->sml_flags = SLAP_MOD_INTERNAL;
	ml->sml_desc = vn->vn_vernum;
	ml->sml_type = vn->vn_vernum->ad_cname;

	*mlp = ml;

	return SLAP_CB_CONTINUE;
}

static int
vernum_db_init(
	BackendDB	*be,
	ConfigReply	*cr)
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;
	vernum_t	*vn = NULL;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"vernum_db_init: vernum cannot be used as global overlay.\n" );
		return 1;
	}

	if ( be->be_nsuffix == NULL ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"vernum_db_init: database must have suffix\n" );
		return 1;
	}

	if ( BER_BVISNULL( &be->be_rootndn ) || BER_BVISEMPTY( &be->be_rootndn ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"vernum_db_init: missing rootdn for database DN=\"%s\", YMMV\n",
			be->be_suffix[ 0 ].bv_val );
	}

	vn = (vernum_t *)ch_calloc( 1, sizeof( vernum_t ) );

	on->on_bi.bi_private = (void *)vn;

	return 0;
}

typedef struct vernum_mod_t {
	struct berval ndn;
	struct vernum_mod_t *next;
} vernum_mod_t;

typedef struct {
	BackendDB *bd;
	vernum_mod_t *mods;
} vernum_repair_cb_t;

static int
vernum_repair_cb( Operation *op, SlapReply *rs )
{
	int rc;
	vernum_repair_cb_t *rcb = op->o_callback->sc_private;
	vernum_mod_t *mod;
	ber_len_t len;
	BackendDB *save_bd = op->o_bd;

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

	len = sizeof( vernum_mod_t ) + rs->sr_entry->e_nname.bv_len + 1;
	mod = op->o_tmpalloc( len, op->o_tmpmemctx );
	mod->ndn.bv_len = rs->sr_entry->e_nname.bv_len;
	mod->ndn.bv_val = (char *)&mod[1];
	lutil_strncopy( mod->ndn.bv_val, rs->sr_entry->e_nname.bv_val, rs->sr_entry->e_nname.bv_len );

	mod->next = rcb->mods;
	rcb->mods = mod;

	Debug( LDAP_DEBUG_TRACE, "%s: vernum_repair_cb: scheduling entry DN=\"%s\" for repair\n",
		op->o_log_prefix, rs->sr_entry->e_name.bv_val, 0 );

	return 0;
}

static int
vernum_repair( BackendDB *be )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	vernum_t *vn = (vernum_t *)on->on_bi.bi_private;
	void *ctx = ldap_pvt_thread_pool_context();
	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	BackendDB db;
	slap_callback sc = { 0 };
	vernum_repair_cb_t rcb = { 0 };
	SlapReply rs = { REP_RESULT };
	vernum_mod_t *rmod;
	int nrepaired = 0;

	connection_fake_init2( &conn, &opbuf, ctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;
	memset( &op->oq_search, 0, sizeof( op->oq_search ) );

	assert( !BER_BVISNULL( &be->be_nsuffix[ 0 ] ) );

	op->o_bd = select_backend( &be->be_nsuffix[ 0 ], 0 );
	assert( op->o_bd != NULL );
	assert( op->o_bd->be_nsuffix != NULL );

	op->o_req_dn = op->o_bd->be_suffix[ 0 ];
	op->o_req_ndn = op->o_bd->be_nsuffix[ 0 ];

	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_attrs = slap_anlist_no_attrs;

	op->ors_filterstr.bv_len = STRLENOF( "(&(=*)(!(=*)))" )
		+ vn->vn_attr->ad_cname.bv_len
		+ vn->vn_vernum->ad_cname.bv_len;
	op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	snprintf( op->ors_filterstr.bv_val, op->ors_filterstr.bv_len + 1,
		"(&(%s=*)(!(%s=*)))",
		vn->vn_attr->ad_cname.bv_val,
		vn->vn_vernum->ad_cname.bv_val );

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( op->ors_filter == NULL ) {
		rs.sr_err = LDAP_OTHER;
		goto done_search;
	}
	
	op->o_callback = &sc;
	sc.sc_response = vernum_repair_cb;
	sc.sc_private = &rcb;
	rcb.bd = &db;
	db = *be;
	db.bd_info = (BackendInfo *)on;

	(void)op->o_bd->bd_info->bi_op_search( op, &rs );

	op->o_tag = LDAP_REQ_MODIFY;
	sc.sc_response = slap_null_cb;
	sc.sc_private = NULL;
	memset( &op->oq_modify, 0, sizeof( req_modify_s ) );

	for ( rmod = rcb.mods; rmod != NULL; ) {
		vernum_mod_t *rnext;
		Modifications mod;
		struct berval vals[2] = { BER_BVNULL };
		SlapReply rs2 = { REP_RESULT };

		mod.sml_flags = SLAP_MOD_INTERNAL;
		mod.sml_op = LDAP_MOD_REPLACE;
		mod.sml_desc = vn->vn_vernum;
		mod.sml_type = vn->vn_vernum->ad_cname;
		mod.sml_values = vals;
		mod.sml_values[0] = val_init;
		mod.sml_nvalues = NULL;
		mod.sml_numvals = 1;
		mod.sml_next = NULL;

		op->o_req_dn = rmod->ndn;
		op->o_req_ndn = rmod->ndn;

		op->orm_modlist = &mod;

		op->o_bd->be_modify( op, &rs2 );

		slap_mods_free( op->orm_modlist->sml_next, 1 );
		if ( rs2.sr_err == LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "%s: vernum_repair: entry DN=\"%s\" repaired\n",
				op->o_log_prefix, rmod->ndn.bv_val, 0 );
			nrepaired++;

		} else {
			Debug( LDAP_DEBUG_ANY, "%s: vernum_repair: entry DN=\"%s\" repair failed (%d)\n",
				op->o_log_prefix, rmod->ndn.bv_val, rs2.sr_err );
		}

		rnext = rmod->next;
		op->o_tmpfree( rmod, op->o_tmpmemctx );
		rmod = rnext;
	}

done_search:;
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	filter_free_x( op, op->ors_filter, 1 );

	Log1( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
		"vernum: repaired=%d\n", nrepaired );

	return 0;
}

static int
vernum_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	vernum_t *vn = (vernum_t *)on->on_bi.bi_private;

	if ( SLAP_SINGLE_SHADOW( be ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"vernum incompatible with shadow database \"%s\".\n",
			be->be_suffix[ 0 ].bv_val );
		return 1;
	}

	/* default: unicodePwd & msDS-KeyVersionNumber */
	if ( vn->vn_attr == NULL ) {
		const char *text = NULL;
		int rc;

		rc = slap_str2ad( "unicodePwd", &vn->vn_attr, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "vernum: unable to find attribute 'unicodePwd' (%d: %s)\n",
				rc, text, 0 );
			return 1;
		}

		vn->vn_vernum = ad_msDS_KeyVersionNumber;
	}

	return vernum_repair( be );
}

static int
vernum_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	vernum_t *vn = (vernum_t *)on->on_bi.bi_private;

	if ( vn ) {
		ch_free( vn );
		on->on_bi.bi_private = NULL;
	}

	return 0;
}

static struct {
	char	*desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.2.840.113556.1.4.1782 "
		"NAME 'msDS-KeyVersionNumber' "
		"DESC 'in the original specification the syntax is 2.5.5.9' "
		"SYNTAX '1.3.6.1.4.1.1466.115.121.1.27' "
		"EQUALITY integerMatch "
		"SINGLE-VALUE "
		"USAGE dSAOperation "
		"NO-USER-MODIFICATION " 
		")",
		&ad_msDS_KeyVersionNumber },
	{ NULL }
};

int
vernum_initialize(void)
{
	int code, i;

	for ( i = 0; as[ i ].desc != NULL; i++ ) {
		code = register_at( as[ i ].desc, as[ i ].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"vernum_initialize: register_at #%d failed\n",
				i, 0, 0 );
			return code;
		}

		/* Allow Manager to set these as needed */
		if ( is_at_no_user_mod( (*as[ i ].adp)->ad_type ) ) {
			(*as[ i ].adp)->ad_type->sat_flags |=
				SLAP_AT_MANAGEABLE;
		}
	}

	vernum.on_bi.bi_type = "vernum";

	vernum.on_bi.bi_op_add = vernum_op_add;
	vernum.on_bi.bi_op_modify = vernum_op_modify;

	vernum.on_bi.bi_db_init = vernum_db_init;
	vernum.on_bi.bi_db_open = vernum_db_open;
	vernum.on_bi.bi_db_destroy = vernum_db_destroy;

	return overlay_register( &vernum );
}

#if SLAPD_OVER_VERNUM == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return vernum_initialize();
}
#endif /* SLAPD_OVER_VERNUM == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_VERNUM */
