/* modify.c - mdb backend modify routine */
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
#include <ac/time.h>

#include "back-mdb.h"

static struct berval scbva[] = {
	BER_BVC("glue"),
	BER_BVNULL
};

static void
mdb_modify_idxflags(
	Operation *op,
	AttributeDescription *desc,
	int got_delete,
	Attribute *newattrs,
	Attribute *oldattrs )
{
	struct berval	ix_at;
	AttrInfo	*ai;

	/* check if modified attribute was indexed
	 * but not in case of NOOP... */
	ai = mdb_index_mask( op->o_bd, desc, &ix_at );
	if ( ai ) {
		if ( got_delete ) {
			Attribute 	*ap;
			struct berval	ix2;

			ap = attr_find( oldattrs, desc );
			if ( ap ) ap->a_flags |= SLAP_ATTR_IXDEL;

			/* Find all other attrs that index to same slot */
			for ( ap = newattrs; ap; ap = ap->a_next ) {
				ai = mdb_index_mask( op->o_bd, ap->a_desc, &ix2 );
				if ( ai && ix2.bv_val == ix_at.bv_val )
					ap->a_flags |= SLAP_ATTR_IXADD;
			}

		} else {
			Attribute 	*ap;

			ap = attr_find( newattrs, desc );
			if ( ap ) ap->a_flags |= SLAP_ATTR_IXADD;
		}
	}
}

int mdb_modify_internal(
	Operation *op,
	MDB_txn *tid,
	Modifications *modlist,
	Entry *e,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	int rc, err;
	Modification	*mod;
	Modifications	*ml;
	Attribute	*save_attrs;
	Attribute 	*ap;
	int			glue_attr_delete = 0;
	int			got_delete;

	Debug( LDAP_DEBUG_TRACE, "mdb_modify_internal: 0x%08lx: %s\n",
		e->e_id, e->e_dn, 0);

	if ( !acl_check_modlist( op, e, modlist )) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	/* save_attrs will be disposed of by caller */
	save_attrs = e->e_attrs;
	e->e_attrs = attrs_dup( e->e_attrs );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		int match;
		mod = &ml->sml_mod;
		switch( mod->sm_op ) {
		case LDAP_MOD_ADD:
		case LDAP_MOD_REPLACE:
			if ( mod->sm_desc == slap_schema.si_ad_structuralObjectClass ) {
				value_match( &match, slap_schema.si_ad_structuralObjectClass,
					slap_schema.si_ad_structuralObjectClass->
						ad_type->sat_equality,
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					&mod->sm_values[0], &scbva[0], text );
				if ( !match ) glue_attr_delete = 1;
			}
		}
		if ( glue_attr_delete )
			break;
	}

	if ( glue_attr_delete ) {
		Attribute	**app = &e->e_attrs;
		while ( *app != NULL ) {
			if ( !is_at_operational( (*app)->a_desc->ad_type )) {
				Attribute *save = *app;
				*app = (*app)->a_next;
				attr_free( save );
				continue;
			}
			app = &(*app)->a_next;
		}
	}

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		mod = &ml->sml_mod;
		got_delete = 0;

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: add %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			err = modify_add_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			}
			break;

		case LDAP_MOD_DELETE:
			if ( glue_attr_delete ) {
				err = LDAP_SUCCESS;
				break;
			}

			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: delete %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			err = modify_delete_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			} else {
				got_delete = 1;
			}
			break;

		case LDAP_MOD_REPLACE:
			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: replace %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			err = modify_replace_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			} else {
				got_delete = 1;
			}
			break;

		case LDAP_MOD_INCREMENT:
			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: increment %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			err = modify_increment_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS,
					"mdb_modify_internal: %d %s\n",
					err, *text, 0);
			} else {
				got_delete = 1;
			}
			break;

		case SLAP_MOD_SOFTADD:
			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: softadd %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
 			/* Avoid problems in index_add_mods()
 			 * We need to add index if necessary.
 			 */
 			mod->sm_op = LDAP_MOD_ADD;

			err = modify_add_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_SOFTADD;

 			if ( err == LDAP_TYPE_OR_VALUE_EXISTS ) {
 				err = LDAP_SUCCESS;
 			}

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			}
 			break;

		case SLAP_MOD_SOFTDEL:
			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: softdel %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
 			/* Avoid problems in index_delete_mods()
 			 * We need to add index if necessary.
 			 */
 			mod->sm_op = LDAP_MOD_DELETE;

			err = modify_delete_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_SOFTDEL;

			if ( err == LDAP_SUCCESS ) {
				got_delete = 1;
			} else if ( err == LDAP_NO_SUCH_ATTRIBUTE ) {
 				err = LDAP_SUCCESS;
 			}

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			}
 			break;

		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			if ( attr_find( e->e_attrs, mod->sm_desc ) != NULL ) {
				/* skip */
				err = LDAP_SUCCESS;
				break;
			}

			Debug(LDAP_DEBUG_ARGS,
				"mdb_modify_internal: add_if_not_present %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
 			/* Avoid problems in index_add_mods()
 			 * We need to add index if necessary.
 			 */
 			mod->sm_op = LDAP_MOD_ADD;

			err = modify_add_values( e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_ADD_IF_NOT_PRESENT;

			if( err != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
					err, *text, 0);
			}
 			break;

		default:
			Debug(LDAP_DEBUG_ANY, "mdb_modify_internal: invalid op %d\n",
				mod->sm_op, 0, 0);
			*text = "Invalid modify operation";
			err = LDAP_OTHER;
			Debug(LDAP_DEBUG_ARGS, "mdb_modify_internal: %d %s\n",
				err, *text, 0);
		}

		if ( err != LDAP_SUCCESS ) {
			attrs_free( e->e_attrs );
			e->e_attrs = save_attrs;
			/* unlock entry, delete from cache */
			return err; 
		}

		/* If objectClass was modified, reset the flags */
		if ( mod->sm_desc == slap_schema.si_ad_objectClass ) {
			e->e_ocflags = 0;
		}

		if ( glue_attr_delete ) e->e_ocflags = 0;


		/* check if modified attribute was indexed
		 * but not in case of NOOP... */
		if ( !op->o_noop ) {
			mdb_modify_idxflags( op, mod->sm_desc, got_delete, e->e_attrs, save_attrs );
		}
	}

	/* check that the entry still obeys the schema */
	ap = NULL;
	rc = entry_schema_check( op, e, save_attrs, get_relax(op), 0, &ap,
		text, textbuf, textlen );
	if ( rc != LDAP_SUCCESS || op->o_noop ) {
		attrs_free( e->e_attrs );
		/* clear the indexing flags */
		for ( ap = save_attrs; ap != NULL; ap = ap->a_next ) {
			ap->a_flags &= ~(SLAP_ATTR_IXADD|SLAP_ATTR_IXDEL);
		}
		e->e_attrs = save_attrs;

		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"entry failed schema check: %s\n",
				*text, 0, 0 );
		}

		/* if NOOP then silently revert to saved attrs */
		return rc;
	}

	/* structuralObjectClass modified! */
	if ( ap ) {
		assert( ap->a_desc == slap_schema.si_ad_structuralObjectClass );
		if ( !op->o_noop ) {
			mdb_modify_idxflags( op, slap_schema.si_ad_structuralObjectClass,
				1, e->e_attrs, save_attrs );
		}
	}

	/* update the indices of the modified attributes */

	/* start with deleting the old index entries */
	for ( ap = save_attrs; ap != NULL; ap = ap->a_next ) {
		if ( ap->a_flags & SLAP_ATTR_IXDEL ) {
			struct berval *vals;
			Attribute *a2;
			ap->a_flags &= ~SLAP_ATTR_IXDEL;
			a2 = attr_find( e->e_attrs, ap->a_desc );
			if ( a2 ) {
				/* need to detect which values were deleted */
				int i, j;
				/* let add know there were deletes */
				if ( a2->a_flags & SLAP_ATTR_IXADD )
					a2->a_flags |= SLAP_ATTR_IXDEL;
				vals = op->o_tmpalloc( (ap->a_numvals + 1) *
					sizeof(struct berval), op->o_tmpmemctx );
				j = 0;
				for ( i=0; i < ap->a_numvals; i++ ) {
					rc = attr_valfind( a2, SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
						&ap->a_nvals[i], NULL, op->o_tmpmemctx );
					/* Save deleted values */
					if ( rc == LDAP_NO_SUCH_ATTRIBUTE )
						vals[j++] = ap->a_nvals[i];
				}
				BER_BVZERO(vals+j);
			} else {
				/* attribute was completely deleted */
				vals = ap->a_nvals;
			}
			rc = 0;
			if ( !BER_BVISNULL( vals )) {
				rc = mdb_index_values( op, tid, ap->a_desc,
					vals, e->e_id, SLAP_INDEX_DELETE_OP );
				if ( rc != LDAP_SUCCESS ) {
					Debug( LDAP_DEBUG_ANY,
						"%s: attribute \"%s\" index delete failure\n",
						op->o_log_prefix, ap->a_desc->ad_cname.bv_val, 0 );
					attrs_free( e->e_attrs );
					e->e_attrs = save_attrs;
				}
			}
			if ( vals != ap->a_nvals )
				op->o_tmpfree( vals, op->o_tmpmemctx );
			if ( rc ) return rc;
		}
	}

	/* add the new index entries */
	for ( ap = e->e_attrs; ap != NULL; ap = ap->a_next ) {
		if (ap->a_flags & SLAP_ATTR_IXADD) {
			ap->a_flags &= ~SLAP_ATTR_IXADD;
			if ( ap->a_flags & SLAP_ATTR_IXDEL ) {
				/* if any values were deleted, we must readd index
				 * for all remaining values.
				 */
				ap->a_flags &= ~SLAP_ATTR_IXDEL;
				rc = mdb_index_values( op, tid, ap->a_desc,
					ap->a_nvals,
					e->e_id, SLAP_INDEX_ADD_OP );
			} else {
				int found = 0;
				/* if this was only an add, we only need to index
				 * the added values.
				 */
				for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
					struct berval *vals;
					if ( ml->sml_desc != ap->a_desc || !ml->sml_numvals )
						continue;
					found = 1;
					switch( ml->sml_op ) {
					case LDAP_MOD_ADD:
					case LDAP_MOD_REPLACE:
					case LDAP_MOD_INCREMENT:
					case SLAP_MOD_SOFTADD:
					case SLAP_MOD_ADD_IF_NOT_PRESENT:
						if ( ml->sml_op == LDAP_MOD_INCREMENT )
							vals = ap->a_nvals;
						else if ( ml->sml_nvalues )
							vals = ml->sml_nvalues;
						else
							vals = ml->sml_values;
						rc = mdb_index_values( op, tid, ap->a_desc,
							vals, e->e_id, SLAP_INDEX_ADD_OP );
						break;
					}
					if ( rc )
						break;
				}
				/* This attr was affected by a modify of a subtype, so
				 * there was no direct match in the modlist. Just readd
				 * all of its values.
				 */
				if ( !found ) {
					rc = mdb_index_values( op, tid, ap->a_desc,
						ap->a_nvals,
						e->e_id, SLAP_INDEX_ADD_OP );
				}
			}
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
				       "%s: attribute \"%s\" index add failure\n",
					op->o_log_prefix, ap->a_desc->ad_cname.bv_val, 0 );
				attrs_free( e->e_attrs );
				e->e_attrs = save_attrs;
				return rc;
			}
		}
	}

	return rc;
}


int
mdb_modify( Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	Entry		*e = NULL;
	int		manageDSAit = get_manageDSAit( op );
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	MDB_txn	*txn = NULL;
	mdb_op_info opinfo = {{{ 0 }}}, *moi = &opinfo;
	Entry		dummy = {0};

	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug( LDAP_DEBUG_ARGS, LDAP_XSTRING(mdb_modify) ": %s\n",
		op->o_req_dn.bv_val, 0, 0 );

#ifdef LDAP_X_TXN
	if( op->o_txnSpec ) {
		/* acquire connection lock */
		ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
		if( op->o_conn->c_txn == CONN_TXN_INACTIVE ) {
			rs->sr_text = "invalid transaction identifier";
			rs->sr_err = LDAP_X_TXN_ID_INVALID;
			goto txnReturn;
		} else if( op->o_conn->c_txn == CONN_TXN_SETTLE ) {
			settle=1;
			goto txnReturn;
		}

		if( op->o_conn->c_txn_backend == NULL ) {
			op->o_conn->c_txn_backend = op->o_bd;

		} else if( op->o_conn->c_txn_backend != op->o_bd ) {
			rs->sr_text = "transaction cannot span multiple database contexts";
			rs->sr_err = LDAP_AFFECTS_MULTIPLE_DSAS;
			goto txnReturn;
		}

		/* insert operation into transaction */

		rs->sr_text = "transaction specified";
		rs->sr_err = LDAP_X_TXN_SPECIFY_OKAY;

txnReturn:
		/* release connection lock */
		ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

		if( !settle ) {
			send_ldap_result( op, rs );
			return rs->sr_err;
		}
	}
#endif

	ctrls[num_ctrls] = NULL;

	/* begin transaction */
	rs->sr_err = mdb_opinfo_get( op, mdb, 0, &moi );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modify) ": txn_begin failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	txn = moi->moi_txn;

	/* Don't touch the opattrs, if this is a contextCSN update
	 * initiated from updatedn */
	if ( !be_isupdate(op) || !op->orm_modlist || op->orm_modlist->sml_next ||
		 op->orm_modlist->sml_desc != slap_schema.si_ad_contextCSN ) {

		slap_mods_opattrs( op, &op->orm_modlist, 1 );
	}

	/* get entry or ancestor */
	rs->sr_err = mdb_dn2entry( op, txn, NULL, &op->o_req_ndn, &e, NULL, 1 );

	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modify) ": dn2entry failed (%d)\n",
			rs->sr_err, 0, 0 );
		switch( rs->sr_err ) {
		case MDB_NOTFOUND:
			break;
		case LDAP_BUSY:
			rs->sr_text = "ldap server busy";
			goto return_results;
		default:
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
			goto return_results;
		}
	}

	/* acquire and lock entry */
	/* FIXME: dn2entry() should return non-glue entry */
	if (( rs->sr_err == MDB_NOTFOUND ) ||
		( !manageDSAit && e && is_entry_glue( e )))
	{
		if ( e != NULL ) {
			rs->sr_matched = ch_strdup( e->e_dn );
			if ( is_entry_referral( e )) {
				BerVarray ref = get_entry_referrals( op, e );
				rs->sr_ref = referral_rewrite( ref, &e->e_name,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
				ber_bvarray_free( ref );
			} else {
				rs->sr_ref = NULL;
			}
			mdb_entry_return( op, e );
			e = NULL;

		} else {
			rs->sr_ref = referral_rewrite( default_referral, NULL,
				&op->o_req_dn, LDAP_SCOPE_DEFAULT );
		}

		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		rs->sr_err = LDAP_REFERRAL;
		send_ldap_result( op, rs );
		goto done;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow modify */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modify) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = e->e_name.bv_val;
		rs->sr_flags = REP_REF_MUSTBEFREED;
		send_ldap_result( op, rs );
		rs->sr_matched = NULL;
		goto done;
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_modify) ": pre-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* Modify the entry */
	dummy = *e;
	rs->sr_err = mdb_modify_internal( op, txn, op->orm_modlist,
		&dummy, &rs->sr_text, textbuf, textlen );

	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modify) ": modify failed (%d)\n",
			rs->sr_err, 0, 0 );
		/* Only free attrs if they were dup'd.  */
		if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
		goto return_results;
	}

	/* change the entry itself */
	rs->sr_err = mdb_id2entry_update( op, txn, NULL, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modify) ": id2entry update failed " "(%d)\n",
			rs->sr_err, 0, 0 );
		rs->sr_text = "entry update failed";
		goto return_results;
	}

	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &dummy,
			&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_modify)
				": post-read failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* Only free attrs if they were dup'd.  */
	if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
	if( moi == &opinfo ) {
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.moi_oe, OpExtra, oe_next );
		opinfo.moi_oe.oe_key = NULL;
		if( op->o_noop ) {
			mdb_txn_abort( txn );
			rs->sr_err = LDAP_X_NO_OPERATION;
			txn = NULL;
			goto return_results;
		} else {
			rs->sr_err = mdb_txn_commit( txn );
			txn = NULL;
		}
	}

	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_modify) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			mdb_strerror(rs->sr_err), rs->sr_err );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";

		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_modify) ": updated%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		dummy.e_id, op->o_req_dn.bv_val );

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if( dummy.e_attrs ) {
		attrs_free( dummy.e_attrs );
	}
	send_ldap_result( op, rs );

#if 0
	if( rs->sr_err == LDAP_SUCCESS && mdb->bi_txn_cp_kbyte ) {
		TXN_CHECKPOINT( mdb->bi_dbenv,
			mdb->bi_txn_cp_kbyte, mdb->bi_txn_cp_min, 0 );
	}
#endif

done:
	slap_graduate_commit_csn( op );

	if( moi == &opinfo ) {
		if( txn != NULL ) {
			mdb_txn_abort( txn );
		}
		if ( opinfo.moi_oe.oe_key ) {
			LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.moi_oe, OpExtra, oe_next );
		}
	} else {
		moi->moi_ref--;
	}

	if( e != NULL ) {
		mdb_entry_return( op, e );
	}

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}
	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}

	rs->sr_text = NULL;

	return rs->sr_err;
}
