/* rdnval.c - RDN value overlay */
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

#ifdef SLAPD_OVER_RDNVAL

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "slap.h"
#include "config.h"

#include "lutil.h"

/*
 * Maintain an attribute (rdnValue) that contains the values of each AVA
 * that builds up the RDN of an entry.  This is required for interoperation
 * with Samba4.  It mimics the "name" attribute provided by Active Directory.
 * The naming attributes must be directoryString-valued, or compatible.
 * For example, IA5String values are cast into directoryString unless
 * consisting of the empty string ("").
 */

static AttributeDescription	*ad_rdnValue;
static Syntax			*syn_IA5String;

static slap_overinst 		rdnval;

static int
rdnval_is_valid( AttributeDescription *desc, struct berval *value )
{
	if ( desc->ad_type->sat_syntax == slap_schema.si_syn_directoryString ) {
		return 1;
	}

	if ( desc->ad_type->sat_syntax == syn_IA5String
		&& !BER_BVISEMPTY( value ) )
	{
		return 1;
	}

	return 0;
}

static int
rdnval_unique_check_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		int *p = (int *)op->o_callback->sc_private;
		(*p)++;
	}

	return 0;
}

static int
rdnval_unique_check( Operation *op, BerVarray vals )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;

	BackendDB db = *op->o_bd;
	Operation op2 = *op;
	SlapReply rs2 = { 0 };
	int i;
	BerVarray fvals;
	char *ptr;
	int gotit = 0;
	slap_callback cb = { 0 };

	/* short-circuit attempts to add suffix entry */
	if ( op->o_tag == LDAP_REQ_ADD
		&& be_issuffix( op->o_bd, &op->o_req_ndn ) )
	{
		return LDAP_SUCCESS;
	}

	op2.o_bd = &db;
	op2.o_bd->bd_info = (BackendInfo *)on->on_info;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_dn = op->o_bd->be_rootdn;
	op2.o_ndn = op->o_bd->be_rootndn;
	op2.o_callback = &cb;
	cb.sc_response = rdnval_unique_check_cb;
	cb.sc_private = (void *)&gotit;

	dnParent( &op->o_req_ndn, &op2.o_req_dn );
	op2.o_req_ndn = op2.o_req_dn;

	op2.ors_limit = NULL;
	op2.ors_slimit = 1;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_attrs = slap_anlist_no_attrs;
	op2.ors_attrsonly = 1;
	op2.ors_deref = LDAP_DEREF_NEVER;
	op2.ors_scope = LDAP_SCOPE_ONELEVEL;

	for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ )
		/* just count */ ;

	fvals = op->o_tmpcalloc( sizeof( struct berval ), i + 1,
		op->o_tmpmemctx );

	op2.ors_filterstr.bv_len = 0;
	if ( i > 1 ) {
		op2.ors_filterstr.bv_len = STRLENOF( "(&)" );
	}

	for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
		ldap_bv2escaped_filter_value_x( &vals[ i ], &fvals[ i ],
			1, op->o_tmpmemctx );
		op2.ors_filterstr.bv_len += ad_rdnValue->ad_cname.bv_len
			+ fvals[ i ].bv_len + STRLENOF( "(=)" );
	}

	op2.ors_filterstr.bv_val = op->o_tmpalloc( op2.ors_filterstr.bv_len + 1, op->o_tmpmemctx );

	ptr = op2.ors_filterstr.bv_val;
	if ( i > 1 ) {
		ptr = lutil_strcopy( ptr, "(&" );
	}
	for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
		*ptr++ = '(';
		ptr = lutil_strncopy( ptr, ad_rdnValue->ad_cname.bv_val, ad_rdnValue->ad_cname.bv_len );
		*ptr++ = '=';
		ptr = lutil_strncopy( ptr, fvals[ i ].bv_val, fvals[ i ].bv_len );
		*ptr++ = ')';
	}

	if ( i > 1 ) {
		*ptr++ = ')';
	}
	*ptr = '\0';

	assert( ptr == op2.ors_filterstr.bv_val + op2.ors_filterstr.bv_len );
	op2.ors_filter = str2filter_x( op, op2.ors_filterstr.bv_val );
	assert( op2.ors_filter != NULL );

	(void)op2.o_bd->be_search( &op2, &rs2 );

	filter_free_x( op, op2.ors_filter, 1 );
	op->o_tmpfree( op2.ors_filterstr.bv_val, op->o_tmpmemctx );
	for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
		if ( vals[ i ].bv_val != fvals[ i ].bv_val ) {
			op->o_tmpfree( fvals[ i ].bv_val, op->o_tmpmemctx );
		}
	}
	op->o_tmpfree( fvals, op->o_tmpmemctx );

	if ( rs2.sr_err != LDAP_SUCCESS || gotit > 0 ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int
rdnval_rdn2vals(
	Operation *op,
	SlapReply *rs,
	struct berval *dn,
	struct berval *ndn,
	BerVarray *valsp,
	BerVarray *nvalsp,
	int *numvalsp )
{
	LDAPRDN rdn = NULL, nrdn = NULL;
	int nAVA, i;

	assert( *valsp == NULL );
	assert( *nvalsp == NULL );

	*numvalsp = 0;

	if ( ldap_bv2rdn_x( dn, &rdn, (char **)&rs->sr_text,
		LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx ) )
	{
		Debug( LDAP_DEBUG_TRACE,
			"%s rdnval: can't figure out "
			"type(s)/value(s) of rdn DN=\"%s\"\n",
			op->o_log_prefix, dn->bv_val, 0 );
		rs->sr_err = LDAP_INVALID_DN_SYNTAX;
		rs->sr_text = "unknown type(s) used in RDN";

		goto done;
	}

	if ( ldap_bv2rdn_x( ndn, &nrdn,
		(char **)&rs->sr_text, LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx ) )
	{
		Debug( LDAP_DEBUG_TRACE,
			"%s rdnval: can't figure out "
			"type(s)/value(s) of normalized rdn DN=\"%s\"\n",
			op->o_log_prefix, ndn->bv_val, 0 );
		rs->sr_err = LDAP_INVALID_DN_SYNTAX;
		rs->sr_text = "unknown type(s) used in RDN";

		goto done;
	}

	for ( nAVA = 0; rdn[ nAVA ]; nAVA++ )
		/* count'em */ ;

	/* NOTE: we assume rdn and nrdn contain the same AVAs! */

	*valsp = SLAP_CALLOC( sizeof( struct berval ), nAVA + 1 );
	*nvalsp = SLAP_CALLOC( sizeof( struct berval ), nAVA + 1 );

	/* Add new attribute values to the entry */
	for ( i = 0; rdn[ i ]; i++ ) {
		AttributeDescription	*desc = NULL;

		rs->sr_err = slap_bv2ad( &rdn[ i ]->la_attr,
			&desc, &rs->sr_text );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s rdnval: %s: %s\n",
				op->o_log_prefix,
				rs->sr_text, 
				rdn[ i ]->la_attr.bv_val );
			goto done;
		}

		if ( !rdnval_is_valid( desc, &rdn[ i ]->la_value ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s rdnval: syntax of naming attribute '%s' "
				"not compatible with directoryString",
				op->o_log_prefix, rdn[ i ]->la_attr.bv_val, 0 );
			continue;
		}

		if ( value_find_ex( desc,
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				*nvalsp,
				&nrdn[ i ]->la_value,
				op->o_tmpmemctx )
			== LDAP_NO_SUCH_ATTRIBUTE )
		{
			ber_dupbv( &(*valsp)[ *numvalsp ], &rdn[ i ]->la_value );
			ber_dupbv( &(*nvalsp)[ *numvalsp ], &nrdn[ i ]->la_value );

			(*numvalsp)++;
		}
	}

	if ( rdnval_unique_check( op, *valsp ) != LDAP_SUCCESS ) {
		rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
		rs->sr_text = "rdnValue not unique within siblings";
		goto done;
	}

done:;
	if ( rdn != NULL ) {
		ldap_rdnfree_x( rdn, op->o_tmpmemctx );
	}

	if ( nrdn != NULL ) {
		ldap_rdnfree_x( nrdn, op->o_tmpmemctx );
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		if ( *valsp != NULL ) {
			ber_bvarray_free( *valsp );
			ber_bvarray_free( *nvalsp );
			*valsp = NULL;
			*nvalsp = NULL;
			*numvalsp = 0;
		}
	}

	return rs->sr_err;
}

static int
rdnval_op_add( Operation *op, SlapReply *rs )
{
	Attribute *a, **ap;
	int numvals = 0;
	BerVarray vals = NULL, nvals = NULL;
	int rc;

	/* NOTE: should we accept an entry still in mods format? */
	assert( op->ora_e != NULL );

	if ( BER_BVISEMPTY( &op->ora_e->e_nname ) ) {
		return SLAP_CB_CONTINUE;
	}

	a = attr_find( op->ora_e->e_attrs, ad_rdnValue );
	if ( a != NULL ) {
		/* TODO: check consistency? */
		return SLAP_CB_CONTINUE;
	}

	rc = rdnval_rdn2vals( op, rs, &op->ora_e->e_name, &op->ora_e->e_nname,
		&vals, &nvals, &numvals );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
	}

	a = attr_alloc( ad_rdnValue );

	a->a_vals = vals;
	a->a_nvals = nvals;
	a->a_numvals = numvals;

	for ( ap = &op->ora_e->e_attrs; *ap != NULL; ap = &(*ap)->a_next )
		/* goto tail */ ;

	*ap = a;

	return SLAP_CB_CONTINUE;
}

static int
rdnval_op_rename( Operation *op, SlapReply *rs )
{
	Modifications *ml, **mlp;
	int numvals = 0;
	BerVarray vals = NULL, nvals = NULL;
	struct berval old;
	int rc;

	dnRdn( &op->o_req_ndn, &old );
	if ( dn_match( &old, &op->orr_nnewrdn ) ) {
		return SLAP_CB_CONTINUE;
	}

	rc = rdnval_rdn2vals( op, rs, &op->orr_newrdn, &op->orr_nnewrdn,
		&vals, &nvals, &numvals );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
	}

	ml = SLAP_CALLOC( sizeof( Modifications ), 1 );
	ml->sml_values = vals;
	ml->sml_nvalues = nvals;

	ml->sml_numvals = numvals;

	ml->sml_op = LDAP_MOD_REPLACE;
	ml->sml_flags = SLAP_MOD_INTERNAL;
	ml->sml_desc = ad_rdnValue;
	ml->sml_type = ad_rdnValue->ad_cname;

	for ( mlp = &op->orr_modlist; *mlp != NULL; mlp = &(*mlp)->sml_next )
		/* goto tail */ ;

	*mlp = ml;

	return SLAP_CB_CONTINUE;
}

static int
rdnval_db_init(
	BackendDB	*be,
	ConfigReply	*cr)
{
	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"rdnval_db_init: rdnval cannot be used as global overlay.\n" );
		return 1;
	}

	if ( be->be_nsuffix == NULL ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"rdnval_db_init: database must have suffix\n" );
		return 1;
	}

	if ( BER_BVISNULL( &be->be_rootndn ) || BER_BVISEMPTY( &be->be_rootndn ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"rdnval_db_init: missing rootdn for database DN=\"%s\", YMMV\n",
			be->be_suffix[ 0 ].bv_val );
	}

	return 0;
}

typedef struct rdnval_mod_t {
	struct berval ndn;
	BerVarray vals;
	BerVarray nvals;
	int numvals;
	struct rdnval_mod_t *next;
} rdnval_mod_t;

typedef struct {
	BackendDB *bd;
	rdnval_mod_t *mods;
} rdnval_repair_cb_t;

static int
rdnval_repair_cb( Operation *op, SlapReply *rs )
{
	int rc;
	rdnval_repair_cb_t *rcb = op->o_callback->sc_private;
	rdnval_mod_t *mod;
	BerVarray vals = NULL, nvals = NULL;
	int numvals = 0;
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

	op->o_bd = rcb->bd;
	rc = rdnval_rdn2vals( op, rs, &rs->sr_entry->e_name, &rs->sr_entry->e_nname,
		&vals, &nvals, &numvals );
	op->o_bd = save_bd;
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	len = sizeof( rdnval_mod_t ) + rs->sr_entry->e_nname.bv_len + 1;
	mod = op->o_tmpalloc( len, op->o_tmpmemctx );
	mod->ndn.bv_len = rs->sr_entry->e_nname.bv_len;
	mod->ndn.bv_val = (char *)&mod[1];
	lutil_strncopy( mod->ndn.bv_val, rs->sr_entry->e_nname.bv_val, rs->sr_entry->e_nname.bv_len );
	mod->vals = vals;
	mod->nvals = nvals;
	mod->numvals = numvals;

	mod->next = rcb->mods;
	rcb->mods = mod;

	Debug( LDAP_DEBUG_TRACE, "%s: rdnval_repair_cb: scheduling entry DN=\"%s\" for repair\n",
		op->o_log_prefix, rs->sr_entry->e_name.bv_val, 0 );

	return 0;
}

static int
rdnval_repair( BackendDB *be )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	void *ctx = ldap_pvt_thread_pool_context();
	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	BackendDB db;
	slap_callback sc = { 0 };
	rdnval_repair_cb_t rcb = { 0 };
	SlapReply rs = { REP_RESULT };
	rdnval_mod_t *rmod;
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

	op->ors_filterstr.bv_len = STRLENOF( "(!(=*))" ) + ad_rdnValue->ad_cname.bv_len;
	op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	snprintf( op->ors_filterstr.bv_val, op->ors_filterstr.bv_len + 1,
		"(!(%s=*))", ad_rdnValue->ad_cname.bv_val );

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( op->ors_filter == NULL ) {
		rs.sr_err = LDAP_OTHER;
		goto done_search;
	}
	
	op->o_callback = &sc;
	sc.sc_response = rdnval_repair_cb;
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
		rdnval_mod_t *rnext;

		Modifications *mod;
		SlapReply rs2 = { REP_RESULT };

		mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
		mod->sml_flags = SLAP_MOD_INTERNAL;
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_desc = ad_rdnValue;
		mod->sml_type = ad_rdnValue->ad_cname;
		mod->sml_values = rmod->vals;
		mod->sml_nvalues = rmod->nvals;
		mod->sml_numvals = rmod->numvals;
		mod->sml_next = NULL;

		op->o_req_dn = rmod->ndn;
		op->o_req_ndn = rmod->ndn;

		op->orm_modlist = mod;

		op->o_bd->be_modify( op, &rs2 );

		slap_mods_free( op->orm_modlist, 1 );
		if ( rs2.sr_err == LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "%s: rdnval_repair: entry DN=\"%s\" repaired\n",
				op->o_log_prefix, rmod->ndn.bv_val, 0 );
			nrepaired++;

		} else {
			Debug( LDAP_DEBUG_ANY, "%s: rdnval_repair: entry DN=\"%s\" repair failed (%d)\n",
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
		"rdnval: repaired=%d\n", nrepaired );

	return 0;
}

/* search all entries without parentUUID; "repair" them */
static int
rdnval_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	if ( SLAP_SINGLE_SHADOW( be ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"rdnval incompatible with shadow database \"%s\".\n",
			be->be_suffix[ 0 ].bv_val );
		return 1;
	}

	return rdnval_repair( be );
}

static struct {
	char	*desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.3.6.1.4.1.4203.666.1.58 "
		"NAME 'rdnValue' "
		"DESC 'the value of the naming attributes' "
		"SYNTAX '1.3.6.1.4.1.1466.115.121.1.15' "
		"EQUALITY caseIgnoreMatch "
		"USAGE dSAOperation "
		"NO-USER-MODIFICATION " 
		")",
		&ad_rdnValue },
	{ NULL }
};

int
rdnval_initialize(void)
{
	int code, i;

	for ( i = 0; as[ i ].desc != NULL; i++ ) {
		code = register_at( as[ i ].desc, as[ i ].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"rdnval_initialize: register_at #%d failed\n",
				i, 0, 0 );
			return code;
		}

		/* Allow Manager to set these as needed */
		if ( is_at_no_user_mod( (*as[ i ].adp)->ad_type ) ) {
			(*as[ i ].adp)->ad_type->sat_flags |=
				SLAP_AT_MANAGEABLE;
		}
	}

	syn_IA5String = syn_find( "1.3.6.1.4.1.1466.115.121.1.26" );
	if ( syn_IA5String == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"rdnval_initialize: unable to find syntax '1.3.6.1.4.1.1466.115.121.1.26' (IA5String)\n",
			0, 0, 0 );
		return LDAP_OTHER;
	}

	rdnval.on_bi.bi_type = "rdnval";

	rdnval.on_bi.bi_op_add = rdnval_op_add;
	rdnval.on_bi.bi_op_modrdn = rdnval_op_rename;

	rdnval.on_bi.bi_db_init = rdnval_db_init;
	rdnval.on_bi.bi_db_open = rdnval_db_open;

	return overlay_register( &rdnval );
}

#if SLAPD_OVER_RDNVAL == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return rdnval_initialize();
}
#endif /* SLAPD_OVER_RDNVAL == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_RDNVAL */
