/* index.c - routines for dealing with attribute indexes */
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
#include <ac/socket.h>

#include "slap.h"
#include "back-bdb.h"
#include "lutil_hash.h"

static char presence_keyval[] = {0,0};
static struct berval presence_key = BER_BVC(presence_keyval);

AttrInfo *bdb_index_mask(
	Backend *be,
	AttributeDescription *desc,
	struct berval *atname )
{
	AttributeType *at;
	AttrInfo *ai = bdb_attr_mask( be->be_private, desc );

	if( ai ) {
		*atname = desc->ad_cname;
		return ai;
	}

	/* If there is a tagging option, did we ever index the base
	 * type? If so, check for mask, otherwise it's not there.
	 */
	if( slap_ad_is_tagged( desc ) && desc != desc->ad_type->sat_ad ) {
		/* has tagging option */
		ai = bdb_attr_mask( be->be_private, desc->ad_type->sat_ad );

		if ( ai && !( ai->ai_indexmask & SLAP_INDEX_NOTAGS ) ) {
			*atname = desc->ad_type->sat_cname;
			return ai;
		}
	}

	/* see if supertype defined mask for its subtypes */
	for( at = desc->ad_type; at != NULL ; at = at->sat_sup ) {
		/* If no AD, we've never indexed this type */
		if ( !at->sat_ad ) continue;

		ai = bdb_attr_mask( be->be_private, at->sat_ad );

		if ( ai && !( ai->ai_indexmask & SLAP_INDEX_NOSUBTYPES ) ) {
			*atname = at->sat_cname;
			return ai;
		}
	}

	return 0;
}

/* This function is only called when evaluating search filters.
 */
int bdb_index_param(
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	DB **dbp,
	slap_mask_t *maskp,
	struct berval *prefixp )
{
	AttrInfo *ai;
	int rc;
	slap_mask_t mask, type = 0;
	DB *db;

	ai = bdb_index_mask( be, desc, prefixp );

	if ( !ai ) {
#ifdef BDB_MONITOR_IDX
		switch ( ftype ) {
		case LDAP_FILTER_PRESENT:
			type = SLAP_INDEX_PRESENT;
			break;
		case LDAP_FILTER_APPROX:
			type = SLAP_INDEX_APPROX;
			break;
		case LDAP_FILTER_EQUALITY:
			type = SLAP_INDEX_EQUALITY;
			break;
		case LDAP_FILTER_SUBSTRINGS:
			type = SLAP_INDEX_SUBSTR;
			break;
		default:
			return LDAP_INAPPROPRIATE_MATCHING;
		}
		bdb_monitor_idx_add( be->be_private, desc, type );
#endif /* BDB_MONITOR_IDX */

		return LDAP_INAPPROPRIATE_MATCHING;
	}
	mask = ai->ai_indexmask;

	rc = bdb_db_cache( be, prefixp, &db );

	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	switch( ftype ) {
	case LDAP_FILTER_PRESENT:
		type = SLAP_INDEX_PRESENT;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
			*prefixp = presence_key;
			goto done;
		}
		break;

	case LDAP_FILTER_APPROX:
		type = SLAP_INDEX_APPROX;
		if ( desc->ad_type->sat_approx ) {
			if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
				goto done;
			}
			break;
		}

		/* Use EQUALITY rule and index for approximate match */
		/* fall thru */

	case LDAP_FILTER_EQUALITY:
		type = SLAP_INDEX_EQUALITY;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
			goto done;
		}
		break;

	case LDAP_FILTER_SUBSTRINGS:
		type = SLAP_INDEX_SUBSTR;
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
			goto done;
		}
		break;

	default:
		return LDAP_OTHER;
	}

#ifdef BDB_MONITOR_IDX
	bdb_monitor_idx_add( be->be_private, desc, type );
#endif /* BDB_MONITOR_IDX */

	return LDAP_INAPPROPRIATE_MATCHING;

done:
	*dbp = db;
	*maskp = mask;
	return LDAP_SUCCESS;
}

static int indexer(
	Operation *op,
	DB_TXN *txn,
	AttributeDescription *ad,
	struct berval *atname,
	BerVarray vals,
	ID id,
	int opid,
	slap_mask_t mask )
{
	int rc, i;
	DB *db;
	struct berval *keys;

	assert( mask != 0 );

	rc = bdb_db_cache( op->o_bd, atname, &db );
	
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_index_read: Could not open DB %s\n",
			atname->bv_val, 0, 0 );
		return LDAP_OTHER;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
		rc = bdb_key_change( op->o_bd, db, txn, &presence_key, id, opid );
		if( rc ) {
			goto done;
		}
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
		rc = ad->ad_type->sat_equality->smr_indexer(
			LDAP_FILTER_EQUALITY,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_equality,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = bdb_key_change( op->o_bd, db, txn, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}
		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
		rc = ad->ad_type->sat_approx->smr_indexer(
			LDAP_FILTER_APPROX,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_approx,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = bdb_key_change( op->o_bd, db, txn, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}

		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
		rc = ad->ad_type->sat_substr->smr_indexer(
			LDAP_FILTER_SUBSTRINGS,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_substr,
			atname, vals, &keys, op->o_tmpmemctx );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = bdb_key_change( op->o_bd, db, txn, &keys[i], id, opid );
				if( rc ) {
					ber_bvarray_free_x( keys, op->o_tmpmemctx );
					goto done;
				}
			}
			ber_bvarray_free_x( keys, op->o_tmpmemctx );
		}

		rc = LDAP_SUCCESS;
	}

done:
	switch( rc ) {
	/* The callers all know how to deal with these results */
	case 0:
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		break;
	/* Anything else is bad news */
	default:
		rc = LDAP_OTHER;
	}
	return rc;
}

static int index_at_values(
	Operation *op,
	DB_TXN *txn,
	AttributeDescription *ad,
	AttributeType *type,
	struct berval *tags,
	BerVarray vals,
	ID id,
	int opid )
{
	int rc;
	slap_mask_t mask = 0;
	int ixop = opid;
	AttrInfo *ai = NULL;

	if ( opid == BDB_INDEX_UPDATE_OP )
		ixop = SLAP_INDEX_ADD_OP;

	if( type->sat_sup ) {
		/* recurse */
		rc = index_at_values( op, txn, NULL,
			type->sat_sup, tags,
			vals, id, opid );

		if( rc ) return rc;
	}

	/* If this type has no AD, we've never used it before */
	if( type->sat_ad ) {
		ai = bdb_attr_mask( op->o_bd->be_private, type->sat_ad );
		if ( ai ) {
#ifdef LDAP_COMP_MATCH
			/* component indexing */
			if ( ai->ai_cr ) {
				ComponentReference *cr;
				for( cr = ai->ai_cr ; cr ; cr = cr->cr_next ) {
					rc = indexer( op, txn, cr->cr_ad, &type->sat_cname,
						cr->cr_nvals, id, ixop,
						cr->cr_indexmask );
				}
			}
#endif
			ad = type->sat_ad;
			/* If we're updating the index, just set the new bits that aren't
			 * already in the old mask.
			 */
			if ( opid == BDB_INDEX_UPDATE_OP )
				mask = ai->ai_newmask & ~ai->ai_indexmask;
			else
			/* For regular updates, if there is a newmask use it. Otherwise
			 * just use the old mask.
			 */
				mask = ai->ai_newmask ? ai->ai_newmask : ai->ai_indexmask;
			if( mask ) {
				rc = indexer( op, txn, ad, &type->sat_cname,
					vals, id, ixop, mask );

				if( rc ) return rc;
			}
		}
	}

	if( tags->bv_len ) {
		AttributeDescription *desc;

		desc = ad_find_tags( type, tags );
		if( desc ) {
			ai = bdb_attr_mask( op->o_bd->be_private, desc );

			if( ai ) {
				if ( opid == BDB_INDEX_UPDATE_OP )
					mask = ai->ai_newmask & ~ai->ai_indexmask;
				else
					mask = ai->ai_newmask ? ai->ai_newmask : ai->ai_indexmask;
				if ( mask ) {
					rc = indexer( op, txn, desc, &desc->ad_cname,
						vals, id, ixop, mask );

					if( rc ) {
						return rc;
					}
				}
			}
		}
	}

	return LDAP_SUCCESS;
}

int bdb_index_values(
	Operation *op,
	DB_TXN *txn,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int opid )
{
	int rc;

	/* Never index ID 0 */
	if ( id == 0 )
		return 0;

	rc = index_at_values( op, txn, desc,
		desc->ad_type, &desc->ad_tags,
		vals, id, opid );

	return rc;
}

/* Get the list of which indices apply to this attr */
int
bdb_index_recset(
	struct bdb_info *bdb,
	Attribute *a,
	AttributeType *type,
	struct berval *tags,
	IndexRec *ir )
{
	int rc, slot;
	AttrList *al;

	if( type->sat_sup ) {
		/* recurse */
		rc = bdb_index_recset( bdb, a, type->sat_sup, tags, ir );
		if( rc ) return rc;
	}
	/* If this type has no AD, we've never used it before */
	if( type->sat_ad ) {
		slot = bdb_attr_slot( bdb, type->sat_ad, NULL );
		if ( slot >= 0 ) {
			ir[slot].ai = bdb->bi_attrs[slot];
			al = ch_malloc( sizeof( AttrList ));
			al->attr = a;
			al->next = ir[slot].attrs;
			ir[slot].attrs = al;
		}
	}
	if( tags->bv_len ) {
		AttributeDescription *desc;

		desc = ad_find_tags( type, tags );
		if( desc ) {
			slot = bdb_attr_slot( bdb, desc, NULL );
			if ( slot >= 0 ) {
				ir[slot].ai = bdb->bi_attrs[slot];
				al = ch_malloc( sizeof( AttrList ));
				al->attr = a;
				al->next = ir[slot].attrs;
				ir[slot].attrs = al;
			}
		}
	}
	return LDAP_SUCCESS;
}

/* Apply the indices for the recset */
int bdb_index_recrun(
	Operation *op,
	struct bdb_info *bdb,
	IndexRec *ir0,
	ID id,
	int base )
{
	IndexRec *ir;
	AttrList *al;
	int i, rc = 0;

	/* Never index ID 0 */
	if ( id == 0 )
		return 0;

	for (i=base; i<bdb->bi_nattrs; i+=slap_tool_thread_max-1) {
		ir = ir0 + i;
		if ( !ir->ai ) continue;
		while (( al = ir->attrs )) {
			ir->attrs = al->next;
			rc = indexer( op, NULL, ir->ai->ai_desc,
				&ir->ai->ai_desc->ad_type->sat_cname,
				al->attr->a_nvals, id, SLAP_INDEX_ADD_OP,
				ir->ai->ai_indexmask );
			free( al );
			if ( rc ) break;
		}
	}
	return rc;
}

int
bdb_index_entry(
	Operation *op,
	DB_TXN *txn,
	int opid,
	Entry	*e )
{
	int rc;
	Attribute *ap = e->e_attrs;
#if 0 /* ifdef LDAP_COMP_MATCH */
	ComponentReference *cr_list = NULL;
	ComponentReference *cr = NULL, *dupped_cr = NULL;
	void* decoded_comp;
	ComponentSyntaxInfo* csi_attr;
	Syntax* syn;
	AttributeType* at;
	int i, num_attr;
	void* mem_op;
	struct berval value = {0};
#endif

	/* Never index ID 0 */
	if ( e->e_id == 0 )
		return 0;

	Debug( LDAP_DEBUG_TRACE, "=> index_entry_%s( %ld, \"%s\" )\n",
		opid == SLAP_INDEX_DELETE_OP ? "del" : "add",
		(long) e->e_id, e->e_dn );

	/* add each attribute to the indexes */
	for ( ; ap != NULL; ap = ap->a_next ) {
#if 0 /* ifdef LDAP_COMP_MATCH */
		AttrInfo *ai;
		/* see if attribute has components to be indexed */
		ai = bdb_attr_mask( op->o_bd->be_private, ap->a_desc->ad_type->sat_ad );
		if ( !ai ) continue;
		cr_list = ai->ai_cr;
		if ( attr_converter && cr_list ) {
			syn = ap->a_desc->ad_type->sat_syntax;
			ap->a_comp_data = op->o_tmpalloc( sizeof( ComponentData ), op->o_tmpmemctx );
                	/* Memory chunk(nibble) pre-allocation for decoders */
                	mem_op = nibble_mem_allocator ( 1024*16, 1024*4 );
			ap->a_comp_data->cd_mem_op = mem_op;
			for( cr = cr_list ; cr ; cr = cr->cr_next ) {
				/* count how many values in an attribute */
				for( num_attr=0; ap->a_vals[num_attr].bv_val != NULL; num_attr++ );
				num_attr++;
				cr->cr_nvals = (BerVarray)op->o_tmpalloc( sizeof( struct berval )*num_attr, op->o_tmpmemctx );
				for( i=0; ap->a_vals[i].bv_val != NULL; i++ ) {
					/* decoding attribute value */
					decoded_comp = attr_converter ( ap, syn, &ap->a_vals[i] );
					if ( !decoded_comp )
						return LDAP_DECODING_ERROR;
					/* extracting the referenced component */
					dupped_cr = dup_comp_ref( op, cr );
					csi_attr = ((ComponentSyntaxInfo*)decoded_comp)->csi_comp_desc->cd_extract_i( mem_op, dupped_cr, decoded_comp );
					if ( !csi_attr )
						return LDAP_DECODING_ERROR;
					cr->cr_asn_type_id = csi_attr->csi_comp_desc->cd_type_id;
					cr->cr_ad = (AttributeDescription*)get_component_description ( cr->cr_asn_type_id );
					if ( !cr->cr_ad )
						return LDAP_INVALID_SYNTAX;
					at = cr->cr_ad->ad_type;
					/* encoding the value of component in GSER */
					rc = component_encoder( mem_op, csi_attr, &value );
					if ( rc != LDAP_SUCCESS )
						return LDAP_ENCODING_ERROR;
					/* Normalize the encoded component values */
					if ( at->sat_equality && at->sat_equality->smr_normalize ) {
						rc = at->sat_equality->smr_normalize (
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							at->sat_syntax, at->sat_equality,
							&value, &cr->cr_nvals[i], op->o_tmpmemctx );
					} else {
						cr->cr_nvals[i] = value;
					}
				}
				/* The end of BerVarray */
				cr->cr_nvals[num_attr-1].bv_val = NULL;
				cr->cr_nvals[num_attr-1].bv_len = 0;
			}
			op->o_tmpfree( ap->a_comp_data, op->o_tmpmemctx );
			nibble_mem_free ( mem_op );
			ap->a_comp_data = NULL;
		}
#endif
		rc = bdb_index_values( op, txn, ap->a_desc,
			ap->a_nvals, e->e_id, opid );

		if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= index_entry_%s( %ld, \"%s\" ) failure\n",
				opid == SLAP_INDEX_ADD_OP ? "add" : "del",
				(long) e->e_id, e->e_dn );
			return rc;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<= index_entry_%s( %ld, \"%s\" ) success\n",
		opid == SLAP_INDEX_DELETE_OP ? "del" : "add",
		(long) e->e_id, e->e_dn );

	return LDAP_SUCCESS;
}
