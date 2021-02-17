/* delete.c - bdb backend delete routine */
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
#include "back-bdb.h"

int
bdb_delete( Operation *op, SlapReply *rs )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	Entry	*matched = NULL;
	struct berval	pdn = {0, NULL};
	Entry	*e = NULL;
	Entry	*p = NULL;
	EntryInfo	*ei = NULL, *eip = NULL;
	int		manageDSAit = get_manageDSAit( op );
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	DB_TXN		*ltid = NULL, *lt2;
	struct bdb_op_info opinfo = {{{ 0 }}};
	ID	eid;

	DB_LOCK		lock, plock;

	int		num_retries = 0;

	int     rc;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int	parent_is_glue = 0;
	int parent_is_leaf = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(bdb_delete) ": %s\n",
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

	/* allocate CSN */
	if ( BER_BVISNULL( &op->o_csn ) ) {
		struct berval csn;
		char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof(csnbuf);
		slap_get_csn( op, &csn, 1 );
	}

	if( 0 ) {
retry:	/* transaction retry */
		if( e != NULL ) {
			bdb_unlocked_cache_return_entry_w(&bdb->bi_cache, e);
			e = NULL;
		}
		if( p != NULL ) {
			bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, p);
			p = NULL;
		}
		Debug( LDAP_DEBUG_TRACE,
			"==> " LDAP_XSTRING(bdb_delete) ": retrying...\n",
			0, 0, 0 );
		rs->sr_err = TXN_ABORT( ltid );
		ltid = NULL;
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.boi_oe, OpExtra, oe_next );
		opinfo.boi_oe.oe_key = NULL;
		op->o_do_not_cache = opinfo.boi_acl_cache;
		if( rs->sr_err != 0 ) {
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
			goto return_results;
		}
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto return_results;
		}
		parent_is_glue = 0;
		parent_is_leaf = 0;
		bdb_trans_backoff( ++num_retries );
	}

	/* begin transaction */
	rs->sr_err = TXN_BEGIN( bdb->bi_dbenv, NULL, &ltid, 
		bdb->bi_db_opflags );
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_delete) ": txn1 id: %x\n",
		ltid->id(ltid), 0, 0 );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_delete) ": txn_begin failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	opinfo.boi_oe.oe_key = bdb;
	opinfo.boi_txn = ltid;
	opinfo.boi_err = 0;
	opinfo.boi_acl_cache = op->o_do_not_cache;
	LDAP_SLIST_INSERT_HEAD( &op->o_extra, &opinfo.boi_oe, oe_next );

	if ( !be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		dnParent( &op->o_req_ndn, &pdn );
	}

	/* get entry */
	rs->sr_err = bdb_dn2entry( op, ltid, &op->o_req_ndn, &ei, 1,
		&lock );

	switch( rs->sr_err ) {
	case 0:
	case DB_NOTFOUND:
		break;
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto retry;
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto return_results;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	if ( rs->sr_err == 0 ) {
		e = ei->bei_e;
		eip = ei->bei_parent;
	} else {
		matched = ei->bei_e;
	}

	/* FIXME : dn2entry() should return non-glue entry */
	if ( e == NULL || ( !manageDSAit && is_entry_glue( e ))) {
		Debug( LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(bdb_delete) ": no such object %s\n",
			op->o_req_dn.bv_val, 0, 0);

		if ( matched != NULL ) {
			rs->sr_matched = ch_strdup( matched->e_dn );
			rs->sr_ref = is_entry_referral( matched )
				? get_entry_referrals( op, matched )
				: NULL;
			bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, matched);
			matched = NULL;

		} else {
			rs->sr_ref = referral_rewrite( default_referral, NULL,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
		}

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	rc = bdb_cache_find_id( op, ltid, eip->bei_id, &eip, 0, &plock );
	switch( rc ) {
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto retry;
	case 0:
	case DB_NOTFOUND:
		break;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	if ( eip ) p = eip->bei_e;

	if ( pdn.bv_len != 0 ) {
		if( p == NULL || !bvmatch( &pdn, &p->e_nname )) {
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(bdb_delete) ": parent "
				"does not exist\n", 0, 0, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "could not locate parent of entry";
			goto return_results;
		}

		/* check parent for "children" acl */
		rs->sr_err = access_allowed( op, p,
			children, NULL, ACL_WDEL, NULL );

		if ( !rs->sr_err  ) {
			switch( opinfo.boi_err ) {
			case DB_LOCK_DEADLOCK:
			case DB_LOCK_NOTGRANTED:
				goto retry;
			}

			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(bdb_delete) ": no write "
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
					switch( opinfo.boi_err ) {
					case DB_LOCK_DEADLOCK:
					case DB_LOCK_NOTGRANTED:
						goto retry;
					}

					Debug( LDAP_DEBUG_TRACE,
						"<=- " LDAP_XSTRING(bdb_delete)
						": no access to parent\n",
						0, 0, 0 );
					rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					rs->sr_text = "no write access to parent";
					goto return_results;
				}

			} else {
				Debug( LDAP_DEBUG_TRACE,
					"<=- " LDAP_XSTRING(bdb_delete)
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
		switch( opinfo.boi_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}

		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_delete) ": no write access "
			"to entry\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow delete */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_delete) ": entry is referral\n",
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
				"<=- " LDAP_XSTRING(bdb_delete) ": pre-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* nested transaction */
	rs->sr_err = TXN_BEGIN( bdb->bi_dbenv, ltid, &lt2, 
		bdb->bi_db_opflags );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_delete) ": txn_begin(2) failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_delete) ": txn2 id: %x\n",
		lt2->id(lt2), 0, 0 );

	BDB_LOG_PRINTF( bdb->bi_dbenv, lt2, "slapd Starting delete %s(%d)",
		e->e_nname.bv_val, e->e_id );

	/* Can't do it if we have kids */
	rs->sr_err = bdb_cache_children( op, lt2, e );
	if( rs->sr_err != DB_NOTFOUND ) {
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		case 0:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(bdb_delete)
				": non-leaf %s\n",
				op->o_req_dn.bv_val, 0, 0);
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			rs->sr_text = "subordinate objects must be deleted first";
			break;
		default:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(bdb_delete)
				": has_children failed: %s (%d)\n",
				db_strerror(rs->sr_err), rs->sr_err, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
		}
		goto return_results;
	}

	/* delete from dn2id */
	rs->sr_err = bdb_dn2id_delete( op, lt2, eip, e );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_delete) ": dn2id failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		rs->sr_text = "DN index delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	/* delete indices for old attributes */
	rs->sr_err = bdb_index_entry_del( op, lt2, e );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_delete) ": index failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
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
		rs->sr_err = bdb_index_values( op, lt2, slap_schema.si_ad_entryCSN,
			vals, 0, SLAP_INDEX_ADD_OP );
	if ( rs->sr_err != LDAP_SUCCESS ) {
			switch( rs->sr_err ) {
			case DB_LOCK_DEADLOCK:
			case DB_LOCK_NOTGRANTED:
				goto retry;
			}
			rs->sr_text = "entryCSN index update failed";
			rs->sr_err = LDAP_OTHER;
			goto return_results;
		}
	}

	/* delete from id2entry */
	rs->sr_err = bdb_id2entry_delete( op->o_bd, lt2, e );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_delete) ": id2entry failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		rs->sr_text = "entry delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	if ( pdn.bv_len != 0 ) {
		parent_is_glue = is_entry_glue(p);
		rs->sr_err = bdb_cache_children( op, lt2, p );
		if ( rs->sr_err != DB_NOTFOUND ) {
			switch( rs->sr_err ) {
			case DB_LOCK_DEADLOCK:
			case DB_LOCK_NOTGRANTED:
				goto retry;
			case 0:
				break;
			default:
				Debug(LDAP_DEBUG_ARGS,
					"<=- " LDAP_XSTRING(bdb_delete)
					": has_children failed: %s (%d)\n",
					db_strerror(rs->sr_err), rs->sr_err, 0 );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";
				goto return_results;
			}
			parent_is_leaf = 1;
		}
		bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, p);
		p = NULL;
	}

	BDB_LOG_PRINTF( bdb->bi_dbenv, lt2, "slapd Commit1 delete %s(%d)",
		e->e_nname.bv_val, e->e_id );

	if ( TXN_COMMIT( lt2, 0 ) != 0 ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "txn_commit(2) failed";
		goto return_results;
	}

	eid = e->e_id;

#if 0	/* Do we want to reclaim deleted IDs? */
	ldap_pvt_thread_mutex_lock( &bdb->bi_lastid_mutex );
	if ( e->e_id == bdb->bi_lastid ) {
		bdb_last_id( op->o_bd, ltid );
	}
	ldap_pvt_thread_mutex_unlock( &bdb->bi_lastid_mutex );
#endif

	if( op->o_noop ) {
		if ( ( rs->sr_err = TXN_ABORT( ltid ) ) != 0 ) {
			rs->sr_text = "txn_abort (no-op) failed";
		} else {
			rs->sr_err = LDAP_X_NO_OPERATION;
			ltid = NULL;
			goto return_results;
		}
	} else {

		BDB_LOG_PRINTF( bdb->bi_dbenv, ltid, "slapd Cache delete %s(%d)",
			e->e_nname.bv_val, e->e_id );

		rc = bdb_cache_delete( bdb, e, ltid, &lock );
		switch( rc ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}

		rs->sr_err = TXN_COMMIT( ltid, 0 );
	}
	ltid = NULL;
	LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.boi_oe, OpExtra, oe_next );
	opinfo.boi_oe.oe_key = NULL;

	BDB_LOG_PRINTF( bdb->bi_dbenv, NULL, "slapd Committed delete %s(%d)",
		e->e_nname.bv_val, e->e_id );

	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_delete) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			db_strerror(rs->sr_err), rs->sr_err );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";

		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_delete) ": deleted%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		eid, op->o_req_dn.bv_val );
	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

	if ( p )
		bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, p);

	/* free entry */
	if( e != NULL ) {
		if ( rs->sr_err == LDAP_SUCCESS ) {
			/* Free the EntryInfo and the Entry */
			bdb_cache_entryinfo_lock( BEI(e) );
			bdb_cache_delete_cleanup( &bdb->bi_cache, BEI(e) );
		} else {
			bdb_unlocked_cache_return_entry_w(&bdb->bi_cache, e);
		}
	}

	if( ltid != NULL ) {
		TXN_ABORT( ltid );
	}
	if ( opinfo.boi_oe.oe_key ) {
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.boi_oe, OpExtra, oe_next );
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}

	if( rs->sr_err == LDAP_SUCCESS && bdb->bi_txn_cp_kbyte ) {
		TXN_CHECKPOINT( bdb->bi_dbenv,
			bdb->bi_txn_cp_kbyte, bdb->bi_txn_cp_min, 0 );
	}
	return rs->sr_err;
}
