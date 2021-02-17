/* delete.c - mdb backend delete routine */
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

#include "lutil.h"
#include "back-mdb.h"

int
mdb_delete( Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	struct berval	pdn = {0, NULL};
	Entry	*e = NULL;
	Entry	*p = NULL;
	int		manageDSAit = get_manageDSAit( op );
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	MDB_txn		*txn = NULL;
	MDB_cursor	*mc;
	mdb_op_info opinfo = {{{ 0 }}}, *moi = &opinfo;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int	parent_is_glue = 0;
	int parent_is_leaf = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(mdb_delete) ": %s\n",
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

	ctrls[num_ctrls] = 0;

	/* begin transaction */
	rs->sr_err = mdb_opinfo_get( op, mdb, 0, &moi );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_delete) ": txn_begin failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	txn = moi->moi_txn;

	/* allocate CSN */
	if ( BER_BVISNULL( &op->o_csn ) ) {
		struct berval csn;
		char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof(csnbuf);
		slap_get_csn( op, &csn, 1 );
	}

	if ( !be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		dnParent( &op->o_req_ndn, &pdn );
	}

	rs->sr_err = mdb_cursor_open( txn, mdb->mi_dn2id, &mc );
	if ( rs->sr_err ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	/* get parent */
	rs->sr_err = mdb_dn2entry( op, txn, mc, &pdn, &p, NULL, 1 );
	switch( rs->sr_err ) {
	case 0:
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
	if ( rs->sr_err == MDB_NOTFOUND ) {
		Debug( LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(mdb_delete) ": no such object %s\n",
			op->o_req_dn.bv_val, 0, 0);

		if ( p && !BER_BVISEMPTY( &p->e_name )) {
			rs->sr_matched = ch_strdup( p->e_name.bv_val );
			if ( is_entry_referral( p )) {
				BerVarray ref = get_entry_referrals( op, p );
				rs->sr_ref = referral_rewrite( ref, &p->e_name,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
				ber_bvarray_free( ref );
			} else {
				rs->sr_ref = NULL;
			}
		} else {
			rs->sr_ref = referral_rewrite( default_referral, NULL,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
		}
		if ( p ) {
			mdb_entry_return( op, p );
			p = NULL;
		}

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	/* get entry */
	rs->sr_err = mdb_dn2entry( op, txn, mc, &op->o_req_ndn, &e, NULL, 0 );
	switch( rs->sr_err ) {
	case MDB_NOTFOUND:
		e = p;
		p = NULL;
	case 0:
		break;
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto return_results;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* FIXME : dn2entry() should return non-glue entry */
	if ( rs->sr_err == MDB_NOTFOUND || ( !manageDSAit && is_entry_glue( e ))) {
		Debug( LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(mdb_delete) ": no such object %s\n",
			op->o_req_dn.bv_val, 0, 0);

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

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	if ( pdn.bv_len != 0 ) {
		/* check parent for "children" acl */
		rs->sr_err = access_allowed( op, p,
			children, NULL, ACL_WDEL, NULL );

		if ( !rs->sr_err  ) {
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_delete) ": no write "
				"access to parent\n", 0, 0, 0 );
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			rs->sr_text = "no write access to parent";
			goto return_results;
		}

	} else {
		/* no parent, must be root to delete */
		if( ! be_isroot( op ) ) {
			if ( be_issuffix( op->o_bd, (struct berval *)&slap_empty_bv )
				|| be_shadow_update( op ) ) {
				p = (Entry *)&slap_entry_root;

				/* check parent for "children" acl */
				rs->sr_err = access_allowed( op, p,
					children, NULL, ACL_WDEL, NULL );

				p = NULL;

				if ( !rs->sr_err  ) {
					Debug( LDAP_DEBUG_TRACE,
						"<=- " LDAP_XSTRING(mdb_delete)
						": no access to parent\n",
						0, 0, 0 );
					rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					rs->sr_text = "no write access to parent";
					goto return_results;
				}

			} else {
				Debug( LDAP_DEBUG_TRACE,
					"<=- " LDAP_XSTRING(mdb_delete)
					": no parent and not root\n", 0, 0, 0 );
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				goto return_results;
			}
		}
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	rs->sr_err = access_allowed( op, e,
		entry, NULL, ACL_WDEL, NULL );

	if ( !rs->sr_err  ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_delete) ": no write access "
			"to entry\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow delete */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_delete) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = ch_strdup( e->e_name.bv_val );
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	/* pre-read */
	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_delete) ": pre-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	rs->sr_text = NULL;

	/* Can't do it if we have kids */
	rs->sr_err = mdb_dn2id_children( op, txn, e );
	if( rs->sr_err != MDB_NOTFOUND ) {
		switch( rs->sr_err ) {
		case 0:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(mdb_delete)
				": non-leaf %s\n",
				op->o_req_dn.bv_val, 0, 0);
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			rs->sr_text = "subordinate objects must be deleted first";
			break;
		default:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(mdb_delete)
				": has_children failed: %s (%d)\n",
				mdb_strerror(rs->sr_err), rs->sr_err, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
		}
		goto return_results;
	}

	/* delete from dn2id */
	rs->sr_err = mdb_dn2id_delete( op, mc, e->e_id, 1 );
	mdb_cursor_close( mc );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_delete) ": dn2id failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_text = "DN index delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	/* delete indices for old attributes */
	rs->sr_err = mdb_index_entry_del( op, txn, e );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_delete) ": index failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_text = "entry index delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	/* fixup delete CSN */
	if ( !SLAP_SHADOW( op->o_bd )) {
		struct berval vals[2];

		assert( !BER_BVISNULL( &op->o_csn ) );
		vals[0] = op->o_csn;
		BER_BVZERO( &vals[1] );
		rs->sr_err = mdb_index_values( op, txn, slap_schema.si_ad_entryCSN,
			vals, 0, SLAP_INDEX_ADD_OP );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_text = "entryCSN index update failed";
			rs->sr_err = LDAP_OTHER;
			goto return_results;
		}
	}

	/* delete from id2entry */
	rs->sr_err = mdb_id2entry_delete( op->o_bd, txn, e );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_delete) ": id2entry failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_text = "entry delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	if ( pdn.bv_len != 0 ) {
		parent_is_glue = is_entry_glue(p);
		rs->sr_err = mdb_dn2id_children( op, txn, p );
		if ( rs->sr_err != MDB_NOTFOUND ) {
			switch( rs->sr_err ) {
			case 0:
				break;
			default:
				Debug(LDAP_DEBUG_ARGS,
					"<=- " LDAP_XSTRING(mdb_delete)
					": has_children failed: %s (%d)\n",
					mdb_strerror(rs->sr_err), rs->sr_err, 0 );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";
				goto return_results;
			}
			parent_is_leaf = 1;
		}
		mdb_entry_return( op, p );
		p = NULL;
	}

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
		}
		txn = NULL;
	}

	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_delete) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			mdb_strerror(rs->sr_err), rs->sr_err );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";

		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_delete) ": deleted%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		e->e_id, op->o_req_dn.bv_val );
	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

	if ( p != NULL ) {
		mdb_entry_return( op, p );
	}

	/* free entry */
	if( e != NULL ) {
		mdb_entry_return( op, e );
	}

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

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}

#if 0
	if( rs->sr_err == LDAP_SUCCESS && mdb->bi_txn_cp_kbyte ) {
		TXN_CHECKPOINT( mdb->bi_dbenv,
			mdb->bi_txn_cp_kbyte, mdb->bi_txn_cp_min, 0 );
	}
#endif
	return rs->sr_err;
}
