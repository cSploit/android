/* modify.cpp - ndb backend modify routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software. This work was sponsored by MySQL.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/time.h>

#include "back-ndb.h"

/* This is a copy from slapd/mods.c, but with compaction tweaked
 * to swap values from the tail into deleted slots, to reduce the
 * overall update traffic.
 */
static int
ndb_modify_delete(
	Entry	*e,
	Modification	*mod,
	int	permissive,
	const char	**text,
	char *textbuf, size_t textlen,
	int *idx )
{
	Attribute	*a;
	MatchingRule 	*mr = mod->sm_desc->ad_type->sat_equality;
	struct berval *cvals;
	int		*id2 = NULL;
	int		i, j, rc = 0, num;
	unsigned flags;
	char		dummy = '\0';

	/* For ordered vals, we have no choice but to preserve order */
	if ( mod->sm_desc->ad_type->sat_flags & SLAP_AT_ORDERED_VAL )
		return modify_delete_vindex( e, mod, permissive, text,
			textbuf, textlen, idx );

	/*
	 * If permissive is set, then the non-existence of an 
	 * attribute is not treated as an error.
	 */

	/* delete the entire attribute */
	if ( mod->sm_values == NULL ) {
		rc = attr_delete( &e->e_attrs, mod->sm_desc );

		if( permissive ) {
			rc = LDAP_SUCCESS;
		} else if( rc != LDAP_SUCCESS ) {
			*text = textbuf;
			snprintf( textbuf, textlen,
				"modify/delete: %s: no such attribute",
				mod->sm_desc->ad_cname.bv_val );
			rc = LDAP_NO_SUCH_ATTRIBUTE;
		}
		return rc;
	}

	/* FIXME: Catch old code that doesn't set sm_numvals.
	 */
	if ( !BER_BVISNULL( &mod->sm_values[mod->sm_numvals] )) {
		for ( i = 0; !BER_BVISNULL( &mod->sm_values[i] ); i++ );
		assert( mod->sm_numvals == i );
	}
	if ( !idx ) {
		id2 = (int *)ch_malloc( mod->sm_numvals * sizeof( int ));
		idx = id2;
	}

	if( mr == NULL || !mr->smr_match ) {
		/* disallow specific attributes from being deleted if
			no equality rule */
		*text = textbuf;
		snprintf( textbuf, textlen,
			"modify/delete: %s: no equality matching rule",
			mod->sm_desc->ad_cname.bv_val );
		rc = LDAP_INAPPROPRIATE_MATCHING;
		goto return_result;
	}

	/* delete specific values - find the attribute first */
	if ( (a = attr_find( e->e_attrs, mod->sm_desc )) == NULL ) {
		if( permissive ) {
			rc = LDAP_SUCCESS;
			goto return_result;
		}
		*text = textbuf;
		snprintf( textbuf, textlen,
			"modify/delete: %s: no such attribute",
			mod->sm_desc->ad_cname.bv_val );
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		goto return_result;
	}

	if ( mod->sm_nvalues ) {
		flags = SLAP_MR_EQUALITY | SLAP_MR_VALUE_OF_ASSERTION_SYNTAX
			| SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH
			| SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH;
		cvals = mod->sm_nvalues;
	} else {
		flags = SLAP_MR_EQUALITY | SLAP_MR_VALUE_OF_ASSERTION_SYNTAX;
		cvals = mod->sm_values;
	}

	/* Locate values to delete */
	for ( i = 0; !BER_BVISNULL( &mod->sm_values[i] ); i++ ) {
		unsigned sort;
		rc = attr_valfind( a, flags, &cvals[i], &sort, NULL );
		if ( rc == LDAP_SUCCESS ) {
			idx[i] = sort;
		} else if ( rc == LDAP_NO_SUCH_ATTRIBUTE ) {
			if ( permissive ) {
				idx[i] = -1;
				continue;
			}
			*text = textbuf;
			snprintf( textbuf, textlen,
				"modify/delete: %s: no such value",
				mod->sm_desc->ad_cname.bv_val );
			goto return_result;
		} else {
			*text = textbuf;
			snprintf( textbuf, textlen,
				"modify/delete: %s: matching rule failed",
				mod->sm_desc->ad_cname.bv_val );
			goto return_result;
		}
	}

	num = a->a_numvals;

	/* Delete the values */
	for ( i = 0; i < mod->sm_numvals; i++ ) {
		/* Skip permissive values that weren't found */
		if ( idx[i] < 0 )
			continue;
		/* Skip duplicate delete specs */
		if ( a->a_vals[idx[i]].bv_val == &dummy )
			continue;
		/* delete value and mark it as gone */
		free( a->a_vals[idx[i]].bv_val );
		a->a_vals[idx[i]].bv_val = &dummy;
		if( a->a_nvals != a->a_vals ) {
			free( a->a_nvals[idx[i]].bv_val );
			a->a_nvals[idx[i]].bv_val = &dummy;
		}
		a->a_numvals--;
	}

	/* compact array */
	for ( i=0; i<num; i++ ) {
		if ( a->a_vals[i].bv_val != &dummy )
			continue;
		for ( --num; num > i && a->a_vals[num].bv_val == &dummy; num-- )
			;
		a->a_vals[i] = a->a_vals[num];
		if ( a->a_nvals != a->a_vals )
			a->a_nvals[i] = a->a_nvals[num];
	}

	BER_BVZERO( &a->a_vals[num] );
	if (a->a_nvals != a->a_vals) {
		BER_BVZERO( &a->a_nvals[num] );
	}

	/* if no values remain, delete the entire attribute */
	if ( !a->a_numvals ) {
		if ( attr_delete( &e->e_attrs, mod->sm_desc ) ) {
			/* Can never happen */
			*text = textbuf;
			snprintf( textbuf, textlen,
				"modify/delete: %s: no such attribute",
				mod->sm_desc->ad_cname.bv_val );
			rc = LDAP_NO_SUCH_ATTRIBUTE;
		}
	}
return_result:
	if ( id2 )
		ch_free( id2 );
	return rc;
}

int ndb_modify_internal(
	Operation *op,
	NdbArgs *NA,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Modification	*mod;
	Modifications	*ml;
	Modifications	*modlist = op->orm_modlist;
	NdbAttrInfo **modai, *atmp;
	const NdbDictionary::Dictionary *myDict;
	const NdbDictionary::Table *myTable;
	int got_oc = 0, nmods = 0, nai = 0, i, j;
	int rc, indexed = 0;
	Attribute *old = NULL;

	Debug( LDAP_DEBUG_TRACE, "ndb_modify_internal: 0x%08lx: %s\n",
		NA->e->e_id, NA->e->e_dn, 0);

	if ( !acl_check_modlist( op, NA->e, modlist )) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	old = attrs_dup( NA->e->e_attrs );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		mod = &ml->sml_mod;
		nmods++;

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: add %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			rc = modify_add_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
			break;

		case LDAP_MOD_DELETE:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: delete %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			rc = ndb_modify_delete( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen, NULL );
			assert( rc != LDAP_TYPE_OR_VALUE_EXISTS );
			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
			break;

		case LDAP_MOD_REPLACE:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: replace %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			rc = modify_replace_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
			break;

		case LDAP_MOD_INCREMENT:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: increment %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			rc = modify_increment_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );
			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS,
					"ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
			break;

		case SLAP_MOD_SOFTADD:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: softadd %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
 			mod->sm_op = LDAP_MOD_ADD;

			rc = modify_add_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_SOFTADD;

 			if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
 				rc = LDAP_SUCCESS;
 			}

			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
 			break;

		case SLAP_MOD_SOFTDEL:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: softdel %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
 			mod->sm_op = LDAP_MOD_DELETE;

			rc = modify_delete_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_SOFTDEL;

 			if ( rc == LDAP_NO_SUCH_ATTRIBUTE) {
 				rc = LDAP_SUCCESS;
 			}

			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
 			break;

		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			Debug(LDAP_DEBUG_ARGS,
				"ndb_modify_internal: add_if_not_present %s\n",
				mod->sm_desc->ad_cname.bv_val, 0, 0);
			if ( attr_find( NA->e->e_attrs, mod->sm_desc ) ) {
				rc = LDAP_SUCCESS;
				break;
			}

 			mod->sm_op = LDAP_MOD_ADD;

			rc = modify_add_values( NA->e, mod, get_permissiveModify(op),
				text, textbuf, textlen );

 			mod->sm_op = SLAP_MOD_ADD_IF_NOT_PRESENT;

			if( rc != LDAP_SUCCESS ) {
				Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
					rc, *text, 0);
			}
 			break;

		default:
			Debug(LDAP_DEBUG_ANY, "ndb_modify_internal: invalid op %d\n",
				mod->sm_op, 0, 0);
			*text = "Invalid modify operation";
			rc = LDAP_OTHER;
			Debug(LDAP_DEBUG_ARGS, "ndb_modify_internal: %d %s\n",
				rc, *text, 0);
		}

		if ( rc != LDAP_SUCCESS ) {
			attrs_free( old );
			return rc; 
		}

		/* If objectClass was modified, reset the flags */
		if ( mod->sm_desc == slap_schema.si_ad_objectClass ) {
			NA->e->e_ocflags = 0;
			got_oc = 1;
		}
	}

	/* check that the entry still obeys the schema */
	rc = entry_schema_check( op, NA->e, NULL, get_relax(op), 0, NULL,
		text, textbuf, textlen );
	if ( rc != LDAP_SUCCESS || op->o_noop ) {
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"entry failed schema check: %s\n",
				*text, 0, 0 );
		}
		attrs_free( old );
		return rc;
	}

	if ( got_oc ) {
		rc = ndb_entry_put_info( op->o_bd, NA, 1 );
		if ( rc ) {
			attrs_free( old );
			return rc;
		}
	}

	/* apply modifications to DB */
	modai = (NdbAttrInfo **)op->o_tmpalloc( nmods * sizeof(NdbAttrInfo*), op->o_tmpmemctx );

	/* Get the unique list of modified attributes */
	ldap_pvt_thread_rdwr_rlock( &ni->ni_ai_rwlock );
	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		/* Already took care of objectclass */
		if ( ml->sml_desc == slap_schema.si_ad_objectClass )
			continue;
		for ( i=0; i<nai; i++ ) {
			if ( ml->sml_desc->ad_type == modai[i]->na_attr )
				break;
		}
		/* This attr was already updated */
		if ( i < nai )
			continue;
		modai[nai] = ndb_ai_find( ni, ml->sml_desc->ad_type );
		if ( modai[nai]->na_flag & NDB_INFO_INDEX )
			indexed++;
		nai++;
	}
	ldap_pvt_thread_rdwr_runlock( &ni->ni_ai_rwlock );

	/* If got_oc, this was already done above */
	if ( indexed && !got_oc) {
		rc = ndb_entry_put_info( op->o_bd, NA, 1 );
		if ( rc ) {
			attrs_free( old );
			return rc;
		}
	}

	myDict = NA->ndb->getDictionary();

	/* sort modai so that OcInfo's are contiguous */
	{
		int j, k;
		for ( i=0; i<nai; i++ ) {
			for ( j=i+1; j<nai; j++ ) {
				if ( modai[i]->na_oi == modai[j]->na_oi )
					continue;
				for ( k=j+1; k<nai; k++ ) {
					if ( modai[i]->na_oi == modai[k]->na_oi ) {
						atmp = modai[j];
						modai[j] = modai[k];
						modai[k] = atmp;
						break;
					}
				}
				/* there are no more na_oi's that match modai[i] */
				if ( k == nai ) {
					i = j;
				}
			}
		}
	}

	/* One call per table... */
	for ( i=0; i<nai; i += j ) {
		atmp = modai[i];
		for ( j=i+1; j<nai; j++ )
			if ( atmp->na_oi != modai[j]->na_oi )
				break;
		j -= i;
		myTable = myDict->getTable( atmp->na_oi->no_table.bv_val );
		if ( !myTable )
			continue;
		rc = ndb_oc_attrs( NA->txn, myTable, NA->e, atmp->na_oi, &modai[i], j, old );
		if ( rc ) break;
	}
	attrs_free( old );
	return rc;
}


int
ndb_back_modify( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Entry		e = {0};
	int		manageDSAit = get_manageDSAit( op );
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;

	int		num_retries = 0;

	NdbArgs NA;
	NdbRdns rdns;
	struct berval matched;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	Debug( LDAP_DEBUG_ARGS, LDAP_XSTRING(ndb_back_modify) ": %s\n",
		op->o_req_dn.bv_val, 0, 0 );

	ctrls[num_ctrls] = NULL;

	slap_mods_opattrs( op, &op->orm_modlist, 1 );

	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );
	rdns.nr_num = 0;
	NA.rdns = &rdns;
	NA.e = &e;

	if( 0 ) {
retry:	/* transaction retry */
		NA.txn->close();
		NA.txn = NULL;
		if( e.e_attrs ) {
			attrs_free( e.e_attrs );
			e.e_attrs = NULL;
		}
		Debug(LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modify) ": retrying...\n", 0, 0, 0);
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto return_results;
		}
		if ( NA.ocs ) {
			ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		}
		ndb_trans_backoff( ++num_retries );
	}
	NA.ocs = NULL;

	/* begin transaction */
	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modify) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* get entry or ancestor */
	rs->sr_err = ndb_entry_get_info( op, &NA, 0, &matched );
	switch( rs->sr_err ) {
	case 0:
		break;
	case LDAP_NO_SUCH_OBJECT:
		Debug( LDAP_DEBUG_ARGS,
			"<=- ndb_back_modify: no such object %s\n",
			op->o_req_dn.bv_val, 0, 0 );
		rs->sr_matched = matched.bv_val;
		if (NA.ocs )
			ndb_check_referral( op, rs, &NA );
		goto return_results;
#if 0
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto retry;
#endif
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto return_results;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* acquire and lock entry */
	rs->sr_err = ndb_entry_get_data( op, &NA, 1 );

	if ( !manageDSAit && is_entry_referral( &e ) ) {
		/* entry is a referral, don't allow modify */
		rs->sr_ref = get_entry_referrals( op, &e );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modify) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = e.e_name.bv_val;
		rs->sr_flags = REP_REF_MUSTBEFREED;
		goto return_results;
	}

	if ( get_assert( op ) &&
		( test_filter( op, &e, (Filter*)get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, &e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(ndb_back_modify) ": pre-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* Modify the entry */
	rs->sr_err = ndb_modify_internal( op, &NA, &rs->sr_text, textbuf, textlen );

	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modify) ": modify failed (%d)\n",
			rs->sr_err, 0, 0 );
#if 0
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
#endif
		goto return_results;
	}

	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &e,
			&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(ndb_back_modify)
				": post-read failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	if( op->o_noop ) {
		if (( rs->sr_err=NA.txn->execute( NdbTransaction::Rollback,
			NdbOperation::AbortOnError, 1 )) != 0 ) {
			rs->sr_text = "txn_abort (no-op) failed";
		} else {
			rs->sr_err = LDAP_X_NO_OPERATION;
		}
	} else {
		if (( rs->sr_err=NA.txn->execute( NdbTransaction::Commit,
			NdbOperation::AbortOnError, 1 )) != 0 ) {
			rs->sr_text = "txn_commit failed";
		} else {
			rs->sr_err = LDAP_SUCCESS;
		}
	}

	if( rs->sr_err != LDAP_SUCCESS && rs->sr_err != LDAP_X_NO_OPERATION ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modify) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code );
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}
	NA.txn->close();
	NA.txn = NULL;

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_modify) ": updated%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		e.e_id, op->o_req_dn.bv_val );

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( NA.ocs ) {
		ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		NA.ocs = NULL;
	}

	if ( e.e_attrs != NULL ) {
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
	}

	if( NA.txn != NULL ) {
		NA.txn->execute( Rollback );
		NA.txn->close();
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

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
