/* filterindex.c - generate the list of candidate entries from a filter */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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

#include "back-mdb.h"
#include "idl.h"
#ifdef LDAP_COMP_MATCH
#include <component.h>
#endif

static int presence_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeDescription *desc,
	ID *ids );

static int equality_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp );
static int inequality_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp,
	int gtorlt );
static int approx_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp );
static int substring_candidates(
	Operation *op,
	MDB_txn *rtxn,
	SubstringsAssertion *sub,
	ID *ids,
	ID *tmp );

static int list_candidates(
	Operation *op,
	MDB_txn *rtxn,
	Filter *flist,
	int ftype,
	ID *ids,
	ID *tmp,
	ID *stack );

static int
ext_candidates(
        Operation *op,
		MDB_txn *rtxn,
        MatchingRuleAssertion *mra,
        ID *ids,
        ID *tmp,
        ID *stack);

#ifdef LDAP_COMP_MATCH
static int
comp_candidates (
	Operation *op,
	MDB_txn *rtxn,
	MatchingRuleAssertion *mra,
	ComponentFilter *f,
	ID *ids,
	ID *tmp,
	ID *stack);

static int
ava_comp_candidates (
		Operation *op,
		MDB_txn *rtxn,
		AttributeAssertion *ava,
		AttributeAliasing *aa,
		ID *ids,
		ID *tmp,
		ID *stack);
#endif

int
mdb_filter_candidates(
	Operation *op,
	MDB_txn *rtxn,
	Filter	*f,
	ID *ids,
	ID *tmp,
	ID *stack )
{
	int rc = 0;
#ifdef LDAP_COMP_MATCH
	AttributeAliasing *aa;
#endif
	Debug( LDAP_DEBUG_FILTER, "=> mdb_filter_candidates\n", 0, 0, 0 );

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		MDB_IDL_ZERO( ids );
		goto out;
	}

	switch ( f->f_choice ) {
	case SLAPD_FILTER_COMPUTED:
		switch( f->f_result ) {
		case SLAPD_COMPARE_UNDEFINED:
		/* This technically is not the same as FALSE, but it
		 * certainly will produce no matches.
		 */
		/* FALL THRU */
		case LDAP_COMPARE_FALSE:
			MDB_IDL_ZERO( ids );
			break;
		case LDAP_COMPARE_TRUE:
			MDB_IDL_ALL( ids );
			break;
		case LDAP_SUCCESS:
			/* this is a pre-computed scope, leave it alone */
			break;
		}
		break;
	case LDAP_FILTER_PRESENT:
		Debug( LDAP_DEBUG_FILTER, "\tPRESENT\n", 0, 0, 0 );
		rc = presence_candidates( op, rtxn, f->f_desc, ids );
		break;

	case LDAP_FILTER_EQUALITY:
		Debug( LDAP_DEBUG_FILTER, "\tEQUALITY\n", 0, 0, 0 );
#ifdef LDAP_COMP_MATCH
		if ( is_aliased_attribute && ( aa = is_aliased_attribute ( f->f_ava->aa_desc ) ) ) {
			rc = ava_comp_candidates ( op, rtxn, f->f_ava, aa, ids, tmp, stack );
		}
		else
#endif
		{
			rc = equality_candidates( op, rtxn, f->f_ava, ids, tmp );
		}
		break;

	case LDAP_FILTER_APPROX:
		Debug( LDAP_DEBUG_FILTER, "\tAPPROX\n", 0, 0, 0 );
		rc = approx_candidates( op, rtxn, f->f_ava, ids, tmp );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		Debug( LDAP_DEBUG_FILTER, "\tSUBSTRINGS\n", 0, 0, 0 );
		rc = substring_candidates( op, rtxn, f->f_sub, ids, tmp );
		break;

	case LDAP_FILTER_GE:
		/* if no GE index, use pres */
		Debug( LDAP_DEBUG_FILTER, "\tGE\n", 0, 0, 0 );
		if( f->f_ava->aa_desc->ad_type->sat_ordering &&
			( f->f_ava->aa_desc->ad_type->sat_ordering->smr_usage & SLAP_MR_ORDERED_INDEX ) )
			rc = inequality_candidates( op, rtxn, f->f_ava, ids, tmp, LDAP_FILTER_GE );
		else
			rc = presence_candidates( op, rtxn, f->f_ava->aa_desc, ids );
		break;

	case LDAP_FILTER_LE:
		/* if no LE index, use pres */
		Debug( LDAP_DEBUG_FILTER, "\tLE\n", 0, 0, 0 );
		if( f->f_ava->aa_desc->ad_type->sat_ordering &&
			( f->f_ava->aa_desc->ad_type->sat_ordering->smr_usage & SLAP_MR_ORDERED_INDEX ) )
			rc = inequality_candidates( op, rtxn, f->f_ava, ids, tmp, LDAP_FILTER_LE );
		else
			rc = presence_candidates( op, rtxn, f->f_ava->aa_desc, ids );
		break;

	case LDAP_FILTER_NOT:
		/* no indexing to support NOT filters */
		Debug( LDAP_DEBUG_FILTER, "\tNOT\n", 0, 0, 0 );
		MDB_IDL_ALL( ids );
		break;

	case LDAP_FILTER_AND:
		Debug( LDAP_DEBUG_FILTER, "\tAND\n", 0, 0, 0 );
		rc = list_candidates( op, rtxn, 
			f->f_and, LDAP_FILTER_AND, ids, tmp, stack );
		break;

	case LDAP_FILTER_OR:
		Debug( LDAP_DEBUG_FILTER, "\tOR\n", 0, 0, 0 );
		rc = list_candidates( op, rtxn,
			f->f_or, LDAP_FILTER_OR, ids, tmp, stack );
		break;
	case LDAP_FILTER_EXT:
                Debug( LDAP_DEBUG_FILTER, "\tEXT\n", 0, 0, 0 );
                rc = ext_candidates( op, rtxn, f->f_mra, ids, tmp, stack );
                break;
	default:
		Debug( LDAP_DEBUG_FILTER, "\tUNKNOWN %lu\n",
			(unsigned long) f->f_choice, 0, 0 );
		/* Must not return NULL, otherwise extended filters break */
		MDB_IDL_ALL( ids );
	}

out:
	Debug( LDAP_DEBUG_FILTER,
		"<= mdb_filter_candidates: id=%ld first=%ld last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST( ids ),
		(long) MDB_IDL_LAST( ids ) );

	return rc;
}

#ifdef LDAP_COMP_MATCH
static int
comp_list_candidates(
	Operation *op,
	MDB_txn *rtxn,
	MatchingRuleAssertion* mra,
	ComponentFilter	*flist,
	int	ftype,
	ID *ids,
	ID *tmp,
	ID *save )
{
	int rc = 0;
	ComponentFilter	*f;

	Debug( LDAP_DEBUG_FILTER, "=> comp_list_candidates 0x%x\n", ftype, 0, 0 );
	for ( f = flist; f != NULL; f = f->cf_next ) {
		/* ignore precomputed scopes */
		if ( f->cf_choice == SLAPD_FILTER_COMPUTED &&
		     f->cf_result == LDAP_SUCCESS ) {
			continue;
		}
		MDB_IDL_ZERO( save );
		rc = comp_candidates( op, rtxn, mra, f, save, tmp, save+MDB_IDL_UM_SIZE );

		if ( rc != 0 ) {
			if ( ftype == LDAP_COMP_FILTER_AND ) {
				rc = 0;
				continue;
			}
			break;
		}
		
		if ( ftype == LDAP_COMP_FILTER_AND ) {
			if ( f == flist ) {
				MDB_IDL_CPY( ids, save );
			} else {
				mdb_idl_intersection( ids, save );
			}
			if( MDB_IDL_IS_ZERO( ids ) )
				break;
		} else {
			if ( f == flist ) {
				MDB_IDL_CPY( ids, save );
			} else {
				mdb_idl_union( ids, save );
			}
		}
	}

	if( rc == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_FILTER,
			"<= comp_list_candidates: id=%ld first=%ld last=%ld\n",
			(long) ids[0],
			(long) MDB_IDL_FIRST(ids),
			(long) MDB_IDL_LAST(ids) );

	} else {
		Debug( LDAP_DEBUG_FILTER,
			"<= comp_list_candidates: undefined rc=%d\n",
			rc, 0, 0 );
	}

	return rc;
}

static int
comp_equality_candidates (
	Operation *op,
	MDB_txn *rtxn,
	MatchingRuleAssertion *mra,
	ComponentAssertion *ca,
	ID *ids,
	ID *tmp,
	ID *stack)
{
	MDB_dbi	  dbi;
	int i;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr = mra->ma_rule;
	Syntax *sat_syntax;
	ComponentReference* cr_list, *cr;
	AttrInfo *ai;

	MDB_IDL_ALL( ids );

	if ( !ca->ca_comp_ref )
		return 0;

	ai = mdb_attr_mask( op->o_bd->be_private, mra->ma_desc );
	if( ai ) {
		cr_list = ai->ai_cr;
	}
	else {
		return 0;
	}
	/* find a component reference to be indexed */
	sat_syntax = ca->ca_ma_rule->smr_syntax;
	for ( cr = cr_list ; cr ; cr = cr->cr_next ) {
		if ( cr->cr_string.bv_len == ca->ca_comp_ref->cr_string.bv_len &&
			strncmp( cr->cr_string.bv_val, ca->ca_comp_ref->cr_string.bv_val,cr->cr_string.bv_len ) == 0 )
			break;
	}
	
	if ( !cr )
		return 0;

	rc = mdb_index_param( op->o_bd, mra->ma_desc, LDAP_FILTER_EQUALITY,
			&dbi, &mask, &prefix );

	if( rc != LDAP_SUCCESS ) {
		return 0;
	}

	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (ca->ca_ma_rule->smr_filter)(
				LDAP_FILTER_EQUALITY,
				cr->cr_indexmask,
				sat_syntax,
				ca->ca_ma_rule,
				&prefix,
				&ca->ca_ma_value,
				&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		return 0;
	}

	if( keys == NULL ) {
		return 0;
	}
	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = mdb_key_read( op->o_bd, rtxn, dbi, &keys[i], tmp, NULL, 0 );

		if( rc == MDB_NOTFOUND ) {
			MDB_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			break;
		}

		if( MDB_IDL_IS_ZERO( tmp ) ) {
			MDB_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			MDB_IDL_CPY( ids, tmp );
		} else {
			mdb_idl_intersection( ids, tmp );
		}

		if( MDB_IDL_IS_ZERO( ids ) )
			break;
	}
	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE,
			"<= comp_equality_candidates: id=%ld, first=%ld, last=%ld\n",
			(long) ids[0],
			(long) MDB_IDL_FIRST(ids),
			(long) MDB_IDL_LAST(ids) );
	return( rc );
}

static int
ava_comp_candidates (
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	AttributeAliasing *aa,
	ID *ids,
	ID *tmp,
	ID *stack )
{
	MatchingRuleAssertion mra;
	
	mra.ma_rule = ava->aa_desc->ad_type->sat_equality;
	if ( !mra.ma_rule ) {
		MDB_IDL_ALL( ids );
		return 0;
	}
	mra.ma_desc = aa->aa_aliased_ad;
	mra.ma_rule = ava->aa_desc->ad_type->sat_equality;
	
	return comp_candidates ( op, rtxn, &mra, ava->aa_cf, ids, tmp, stack );
}

static int
comp_candidates (
	Operation *op,
	MDB_txn *rtxn,
	MatchingRuleAssertion *mra,
	ComponentFilter *f,
	ID *ids,
	ID *tmp,
	ID *stack)
{
	int	rc;

	if ( !f ) return LDAP_PROTOCOL_ERROR;

	Debug( LDAP_DEBUG_FILTER, "comp_candidates\n", 0, 0, 0 );
	switch ( f->cf_choice ) {
	case SLAPD_FILTER_COMPUTED:
		rc = f->cf_result;
		break;
	case LDAP_COMP_FILTER_AND:
		rc = comp_list_candidates( op, rtxn, mra, f->cf_and, LDAP_COMP_FILTER_AND, ids, tmp, stack );
		break;
	case LDAP_COMP_FILTER_OR:
		rc = comp_list_candidates( op, rtxn, mra, f->cf_or, LDAP_COMP_FILTER_OR, ids, tmp, stack );
		break;
	case LDAP_COMP_FILTER_NOT:
		/* No component indexing supported for NOT filter */
		Debug( LDAP_DEBUG_FILTER, "\tComponent NOT\n", 0, 0, 0 );
		MDB_IDL_ALL( ids );
		rc = LDAP_PROTOCOL_ERROR;
		break;
	case LDAP_COMP_FILTER_ITEM:
		rc = comp_equality_candidates( op, rtxn, mra, f->cf_ca, ids, tmp, stack );
		break;
	default:
		MDB_IDL_ALL( ids );
		rc = LDAP_PROTOCOL_ERROR;
	}

	return( rc );
}
#endif

static int
ext_candidates(
        Operation *op,
		MDB_txn *rtxn,
        MatchingRuleAssertion *mra,
        ID *ids,
        ID *tmp,
        ID *stack)
{
#ifdef LDAP_COMP_MATCH
	/*
	 * Currently Only Component Indexing for componentFilterMatch is supported
	 * Indexing for an extensible filter is not supported yet
	 */
	if ( mra->ma_cf ) {
		return comp_candidates ( op, rtxn, mra, mra->ma_cf, ids, tmp, stack);
	}
#endif
	if ( mra->ma_desc == slap_schema.si_ad_entryDN ) {
		int rc;
		ID id;

		MDB_IDL_ZERO( ids );
		if ( mra->ma_rule == slap_schema.si_mr_distinguishedNameMatch ) {
base:
			rc = mdb_dn2id( op, rtxn, NULL, &mra->ma_value, &id, NULL, NULL, NULL );
			if ( rc == MDB_SUCCESS ) {
				mdb_idl_insert( ids, id );
			}
			return 0;
		} else if ( mra->ma_rule && mra->ma_rule->smr_match ==
			dnRelativeMatch && dnIsSuffix( &mra->ma_value,
				op->o_bd->be_nsuffix )) {
			int scope;
			if ( mra->ma_rule == slap_schema.si_mr_dnSuperiorMatch ) {
				mdb_dn2sups( op, rtxn, &mra->ma_value, ids );
				return 0;
			}
			if ( mra->ma_rule == slap_schema.si_mr_dnSubtreeMatch )
				scope = LDAP_SCOPE_SUBTREE;
			else if ( mra->ma_rule == slap_schema.si_mr_dnOneLevelMatch )
				scope = LDAP_SCOPE_ONELEVEL;
			else if ( mra->ma_rule == slap_schema.si_mr_dnSubordinateMatch )
				scope = LDAP_SCOPE_SUBORDINATE;
			else
				goto base;	/* scope = LDAP_SCOPE_BASE; */
#if 0
			if ( scope > LDAP_SCOPE_BASE ) {
				ei = NULL;
				rc = mdb_cache_find_ndn( op, rtxn, &mra->ma_value, &ei );
				if ( ei )
					mdb_cache_entryinfo_unlock( ei );
				if ( rc == LDAP_SUCCESS ) {
					int sc = op->ors_scope;
					op->ors_scope = scope;
					rc = mdb_dn2idl( op, rtxn, &mra->ma_value, ei, ids,
						stack );
					op->ors_scope = sc;
				}
				return 0;
			}
#endif
		}
	}

	MDB_IDL_ALL( ids );
	return 0;
}

static int
list_candidates(
	Operation *op,
	MDB_txn *rtxn,
	Filter	*flist,
	int		ftype,
	ID *ids,
	ID *tmp,
	ID *save )
{
	int rc = 0;
	Filter	*f;

	Debug( LDAP_DEBUG_FILTER, "=> mdb_list_candidates 0x%x\n", ftype, 0, 0 );
	for ( f = flist; f != NULL; f = f->f_next ) {
		/* ignore precomputed scopes */
		if ( f->f_choice == SLAPD_FILTER_COMPUTED &&
		     f->f_result == LDAP_SUCCESS ) {
			continue;
		}
		MDB_IDL_ZERO( save );
		rc = mdb_filter_candidates( op, rtxn, f, save, tmp,
			save+MDB_IDL_UM_SIZE );

		if ( rc != 0 ) {
			if ( ftype == LDAP_FILTER_AND ) {
				rc = 0;
				continue;
			}
			break;
		}

		
		if ( ftype == LDAP_FILTER_AND ) {
			if ( f == flist ) {
				MDB_IDL_CPY( ids, save );
			} else {
				mdb_idl_intersection( ids, save );
			}
			if( MDB_IDL_IS_ZERO( ids ) )
				break;
		} else {
			if ( f == flist ) {
				MDB_IDL_CPY( ids, save );
			} else {
				mdb_idl_union( ids, save );
			}
		}
	}

	if( rc == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_FILTER,
			"<= mdb_list_candidates: id=%ld first=%ld last=%ld\n",
			(long) ids[0],
			(long) MDB_IDL_FIRST(ids),
			(long) MDB_IDL_LAST(ids) );

	} else {
		Debug( LDAP_DEBUG_FILTER,
			"<= mdb_list_candidates: undefined rc=%d\n",
			rc, 0, 0 );
	}

	return rc;
}

static int
presence_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeDescription *desc,
	ID *ids )
{
	MDB_dbi dbi;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};

	Debug( LDAP_DEBUG_TRACE, "=> mdb_presence_candidates (%s)\n",
			desc->ad_cname.bv_val, 0, 0 );

	MDB_IDL_ALL( ids );

	if( desc == slap_schema.si_ad_objectClass ) {
		return 0;
	}

	rc = mdb_index_param( op->o_bd, desc, LDAP_FILTER_PRESENT,
		&dbi, &mask, &prefix );

	if( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		/* not indexed */
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_presence_candidates: (%s) not indexed\n",
			desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_presence_candidates: (%s) index_param "
			"returned=%d\n",
			desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	if( prefix.bv_val == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_presence_candidates: (%s) no prefix\n",
			desc->ad_cname.bv_val, 0, 0 );
		return -1;
	}

	rc = mdb_key_read( op->o_bd, rtxn, dbi, &prefix, ids, NULL, 0 );

	if( rc == MDB_NOTFOUND ) {
		MDB_IDL_ZERO( ids );
		rc = 0;
	} else if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_presense_candidates: (%s) "
			"key read failed (%d)\n",
			desc->ad_cname.bv_val, rc, 0 );
		goto done;
	}

	Debug(LDAP_DEBUG_TRACE,
		"<= mdb_presence_candidates: id=%ld first=%ld last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST(ids),
		(long) MDB_IDL_LAST(ids) );

done:
	return rc;
}

static int
equality_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp )
{
	MDB_dbi	dbi;
	int i;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_equality_candidates (%s)\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );

	if ( ava->aa_desc == slap_schema.si_ad_entryDN ) {
		ID id;
		rc = mdb_dn2id( op, rtxn, NULL, &ava->aa_value, &id, NULL, NULL, NULL );
		if ( rc == LDAP_SUCCESS ) {
			/* exactly one ID can match */
			ids[0] = 1;
			ids[1] = id;
		}
		if ( rc == MDB_NOTFOUND ) {
			MDB_IDL_ZERO( ids );
			rc = 0;
		}
		return rc;
	}

	MDB_IDL_ALL( ids );

	rc = mdb_index_param( op->o_bd, ava->aa_desc, LDAP_FILTER_EQUALITY,
		&dbi, &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_equality_candidates: (%s) not indexed\n", 
			ava->aa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_equality_candidates: (%s) "
			"index_param failed (%d)\n",
			ava->aa_desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	mr = ava->aa_desc->ad_type->sat_equality;
	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_EQUALITY,
		mask,
		ava->aa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		&ava->aa_value,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_equality_candidates: (%s, %s) "
			"MR filter failed (%d)\n",
			prefix.bv_val, ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_equality_candidates: (%s) no keys\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = mdb_key_read( op->o_bd, rtxn, dbi, &keys[i], tmp, NULL, 0 );

		if( rc == MDB_NOTFOUND ) {
			MDB_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_equality_candidates: (%s) "
				"key read failed (%d)\n",
				ava->aa_desc->ad_cname.bv_val, rc, 0 );
			break;
		}

		if( MDB_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_equality_candidates: (%s) NULL\n", 
				ava->aa_desc->ad_cname.bv_val, 0, 0 );
			MDB_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			MDB_IDL_CPY( ids, tmp );
		} else {
			mdb_idl_intersection( ids, tmp );
		}

		if( MDB_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE,
		"<= mdb_equality_candidates: id=%ld, first=%ld, last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST(ids),
		(long) MDB_IDL_LAST(ids) );
	return( rc );
}


static int
approx_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp )
{
	MDB_dbi	dbi;
	int i;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_approx_candidates (%s)\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );

	MDB_IDL_ALL( ids );

	rc = mdb_index_param( op->o_bd, ava->aa_desc, LDAP_FILTER_APPROX,
		&dbi, &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_approx_candidates: (%s) not indexed\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_approx_candidates: (%s) "
			"index_param failed (%d)\n",
			ava->aa_desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	mr = ava->aa_desc->ad_type->sat_approx;
	if( !mr ) {
		/* no approx matching rule, try equality matching rule */
		mr = ava->aa_desc->ad_type->sat_equality;
	}

	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_APPROX,
		mask,
		ava->aa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		&ava->aa_value,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_approx_candidates: (%s, %s) "
			"MR filter failed (%d)\n",
			prefix.bv_val, ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_approx_candidates: (%s) no keys (%s)\n",
			prefix.bv_val, ava->aa_desc->ad_cname.bv_val, 0 );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = mdb_key_read( op->o_bd, rtxn, dbi, &keys[i], tmp, NULL, 0 );

		if( rc == MDB_NOTFOUND ) {
			MDB_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_approx_candidates: (%s) "
				"key read failed (%d)\n",
				ava->aa_desc->ad_cname.bv_val, rc, 0 );
			break;
		}

		if( MDB_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_approx_candidates: (%s) NULL\n",
				ava->aa_desc->ad_cname.bv_val, 0, 0 );
			MDB_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			MDB_IDL_CPY( ids, tmp );
		} else {
			mdb_idl_intersection( ids, tmp );
		}

		if( MDB_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE, "<= mdb_approx_candidates %ld, first=%ld, last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST(ids),
		(long) MDB_IDL_LAST(ids) );
	return( rc );
}

static int
substring_candidates(
	Operation *op,
	MDB_txn *rtxn,
	SubstringsAssertion	*sub,
	ID *ids,
	ID *tmp )
{
	MDB_dbi	dbi;
	int i;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_substring_candidates (%s)\n",
			sub->sa_desc->ad_cname.bv_val, 0, 0 );

	MDB_IDL_ALL( ids );

	rc = mdb_index_param( op->o_bd, sub->sa_desc, LDAP_FILTER_SUBSTRINGS,
		&dbi, &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_substring_candidates: (%s) not indexed\n",
			sub->sa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_substring_candidates: (%s) "
			"index_param failed (%d)\n",
			sub->sa_desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	mr = sub->sa_desc->ad_type->sat_substr;

	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_SUBSTRINGS,
		mask,
		sub->sa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		sub,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_substring_candidates: (%s) "
			"MR filter failed (%d)\n",
			sub->sa_desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_substring_candidates: (0x%04lx) no keys (%s)\n",
			mask, sub->sa_desc->ad_cname.bv_val, 0 );
		return 0;
	}

	for ( i= 0; keys[i].bv_val != NULL; i++ ) {
		rc = mdb_key_read( op->o_bd, rtxn, dbi, &keys[i], tmp, NULL, 0 );

		if( rc == MDB_NOTFOUND ) {
			MDB_IDL_ZERO( ids );
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_substring_candidates: (%s) "
				"key read failed (%d)\n",
				sub->sa_desc->ad_cname.bv_val, rc, 0 );
			break;
		}

		if( MDB_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= mdb_substring_candidates: (%s) NULL\n",
				sub->sa_desc->ad_cname.bv_val, 0, 0 );
			MDB_IDL_ZERO( ids );
			break;
		}

		if ( i == 0 ) {
			MDB_IDL_CPY( ids, tmp );
		} else {
			mdb_idl_intersection( ids, tmp );
		}

		if( MDB_IDL_IS_ZERO( ids ) )
			break;
	}

	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE, "<= mdb_substring_candidates: %ld, first=%ld, last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST(ids),
		(long) MDB_IDL_LAST(ids) );
	return( rc );
}

static int
inequality_candidates(
	Operation *op,
	MDB_txn *rtxn,
	AttributeAssertion *ava,
	ID *ids,
	ID *tmp,
	int gtorlt )
{
	MDB_dbi	dbi;
	int rc;
	slap_mask_t mask;
	struct berval prefix = {0, NULL};
	struct berval *keys = NULL;
	MatchingRule *mr;
	MDB_cursor *cursor = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_inequality_candidates (%s)\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );

	MDB_IDL_ALL( ids );

	rc = mdb_index_param( op->o_bd, ava->aa_desc, LDAP_FILTER_EQUALITY,
		&dbi, &mask, &prefix );

	if ( rc == LDAP_INAPPROPRIATE_MATCHING ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_inequality_candidates: (%s) not indexed\n", 
			ava->aa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"<= mdb_inequality_candidates: (%s) "
			"index_param failed (%d)\n",
			ava->aa_desc->ad_cname.bv_val, rc, 0 );
		return 0;
	}

	mr = ava->aa_desc->ad_type->sat_equality;
	if( !mr ) {
		return 0;
	}

	if( !mr->smr_filter ) {
		return 0;
	}

	rc = (mr->smr_filter)(
		LDAP_FILTER_EQUALITY,
		mask,
		ava->aa_desc->ad_type->sat_syntax,
		mr,
		&prefix,
		&ava->aa_value,
		&keys, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_inequality_candidates: (%s, %s) "
			"MR filter failed (%d)\n",
			prefix.bv_val, ava->aa_desc->ad_cname.bv_val, rc );
		return 0;
	}

	if( keys == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mdb_inequality_candidates: (%s) no keys\n",
			ava->aa_desc->ad_cname.bv_val, 0, 0 );
		return 0;
	}

	MDB_IDL_ZERO( ids );
	while(1) {
		rc = mdb_key_read( op->o_bd, rtxn, dbi, &keys[0], tmp, &cursor, gtorlt );

		if( rc == MDB_NOTFOUND ) {
			rc = 0;
			break;
		} else if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
			       "<= mdb_inequality_candidates: (%s) "
			       "key read failed (%d)\n",
			       ava->aa_desc->ad_cname.bv_val, rc, 0 );
			break;
		}

		if( MDB_IDL_IS_ZERO( tmp ) ) {
			Debug( LDAP_DEBUG_TRACE,
			       "<= mdb_inequality_candidates: (%s) NULL\n", 
			       ava->aa_desc->ad_cname.bv_val, 0, 0 );
			break;
		}

		mdb_idl_union( ids, tmp );

		if( op->ors_limit && op->ors_limit->lms_s_unchecked != -1 &&
			MDB_IDL_N( ids ) >= (unsigned) op->ors_limit->lms_s_unchecked ) {
			mdb_cursor_close( cursor );
			break;
		}
	}
	ber_bvarray_free_x( keys, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE,
		"<= mdb_inequality_candidates: id=%ld, first=%ld, last=%ld\n",
		(long) ids[0],
		(long) MDB_IDL_FIRST(ids),
		(long) MDB_IDL_LAST(ids) );
	return( rc );
}
