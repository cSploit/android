/* add.c - ldap mdb back-end add routine */
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

int
mdb_add(Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	struct berval	pdn;
	Entry		*p = NULL, *oe = op->ora_e;
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	MDB_txn		*txn = NULL;
	MDB_cursor	*mc = NULL;
	MDB_cursor	*mcd;
	ID eid, pid = 0;
	mdb_op_info opinfo = {{{ 0 }}}, *moi = &opinfo;
	int subentry;

	int		success;

	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug(LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(mdb_add) ": %s\n",
		op->ora_e->e_name.bv_val, 0, 0);

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

	/* check entry's schema */
	rs->sr_err = entry_schema_check( op, op->ora_e, NULL,
		get_relax(op), 1, NULL, &rs->sr_text, textbuf, textlen );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": entry failed schema check: "
			"%s (%d)\n", rs->sr_text, rs->sr_err, 0 );
		goto return_results;
	}

	/* begin transaction */
	rs->sr_err = mdb_opinfo_get( op, mdb, 0, &moi );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": txn_begin failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	txn = moi->moi_txn;

	/* add opattrs to shadow as well, only missing attrs will actually
	 * be added; helps compatibility with older OL versions */
	rs->sr_err = slap_add_opattrs( op, &rs->sr_text, textbuf, textlen, 1 );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": entry failed op attrs add: "
			"%s (%d)\n", rs->sr_text, rs->sr_err, 0 );
		goto return_results;
	}

	if ( get_assert( op ) &&
		( test_filter( op, op->ora_e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	subentry = is_entry_subentry( op->ora_e );

	/*
	 * Get the parent dn and see if the corresponding entry exists.
	 */
	if ( be_issuffix( op->o_bd, &op->ora_e->e_nname ) ) {
		pdn = slap_empty_bv;
	} else {
		dnParent( &op->ora_e->e_nname, &pdn );
	}

	rs->sr_err = mdb_cursor_open( txn, mdb->mi_dn2id, &mcd );
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": mdb_cursor_open failed (%d)\n",
			rs->sr_err, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* get entry or parent */
	rs->sr_err = mdb_dn2entry( op, txn, mcd, &op->ora_e->e_nname, &p, NULL, 1 );
	switch( rs->sr_err ) {
	case 0:
		rs->sr_err = LDAP_ALREADY_EXISTS;
		mdb_entry_return( op, p );
		p = NULL;
		goto return_results;
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

	if ( !p )
		p = (Entry *)&slap_entry_root;

	if ( !bvmatch( &pdn, &p->e_nname ) ) {
		rs->sr_matched = ber_strdup_x( p->e_name.bv_val,
			op->o_tmpmemctx );
		if ( p != (Entry *)&slap_entry_root && is_entry_referral( p )) {
			BerVarray ref = get_entry_referrals( op, p );
			rs->sr_ref = referral_rewrite( ref, &p->e_name,
				&op->o_req_dn, LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );
		} else {
			rs->sr_ref = NULL;
		}
		if ( p != (Entry *)&slap_entry_root )
			mdb_entry_return( op, p );
		p = NULL;
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": parent "
			"does not exist\n", 0, 0, 0 );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		goto return_results;
	}

	rs->sr_err = access_allowed( op, p,
		children, NULL, ACL_WADD, NULL );

	if ( ! rs->sr_err ) {
		if ( p != (Entry *)&slap_entry_root )
			mdb_entry_return( op, p );
		p = NULL;

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": no write access to parent\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to parent";
		goto return_results;;
	}

	if ( p != (Entry *)&slap_entry_root ) {
		if ( is_entry_subentry( p ) ) {
			mdb_entry_return( op, p );
			p = NULL;
			/* parent is a subentry, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_add) ": parent is subentry\n",
				0, 0, 0 );
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			rs->sr_text = "parent is a subentry";
			goto return_results;;
		}

		if ( is_entry_alias( p ) ) {
			mdb_entry_return( op, p );
			p = NULL;
			/* parent is an alias, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_add) ": parent is alias\n",
				0, 0, 0 );
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "parent is an alias";
			goto return_results;;
		}

		if ( is_entry_referral( p ) ) {
			BerVarray ref = get_entry_referrals( op, p );
			/* parent is a referral, don't allow add */
			rs->sr_matched = ber_strdup_x( p->e_name.bv_val,
				op->o_tmpmemctx );
			rs->sr_ref = referral_rewrite( ref, &p->e_name,
				&op->o_req_dn, LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );
			mdb_entry_return( op, p );
			p = NULL;
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_add) ": parent is referral\n",
				0, 0, 0 );

			rs->sr_err = LDAP_REFERRAL;
			rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
			goto return_results;
		}

	}

	if ( subentry ) {
		/* FIXME: */
		/* parent must be an administrative point of the required kind */
	}

	/* free parent */
	if ( p != (Entry *)&slap_entry_root ) {
		pid = p->e_id;
		if ( p->e_nname.bv_len ) {
			struct berval ppdn;

			/* ITS#5326: use parent's DN if differs from provided one */
			dnParent( &op->ora_e->e_name, &ppdn );
			if ( !dn_match( &p->e_name, &ppdn ) ) {
				struct berval rdn;
				struct berval newdn;

				dnRdn( &op->ora_e->e_name, &rdn );

				build_new_dn( &newdn, &p->e_name, &rdn, NULL ); 
				if ( op->ora_e->e_name.bv_val != op->o_req_dn.bv_val )
					ber_memfree( op->ora_e->e_name.bv_val );
				op->ora_e->e_name = newdn;

				/* FIXME: should check whether
				 * dnNormalize(newdn) == e->e_nname ... */
			}
		}

		mdb_entry_return( op, p );
	}
	p = NULL;

	rs->sr_err = access_allowed( op, op->ora_e,
		entry, NULL, ACL_WADD, NULL );

	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": no write access to entry\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;;
	}

	/* 
	 * Check ACL for attribute write access
	 */
	if (!acl_check_modlist(op, oe, op->ora_modlist)) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": no write access to attribute\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to attribute";
		goto return_results;;
	}

	rs->sr_err = mdb_cursor_open( txn, mdb->mi_id2entry, &mc );
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": mdb_cursor_open failed (%d)\n",
			rs->sr_err, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	rs->sr_err = mdb_next_id( op->o_bd, mc, &eid );
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": next_id failed (%d)\n",
			rs->sr_err, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	op->ora_e->e_id = eid;

	/* dn2id index */
	rs->sr_err = mdb_dn2id_add( op, mcd, mcd, pid, 1, 1, op->ora_e );
	mdb_cursor_close( mcd );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": dn2id_add failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );

		switch( rs->sr_err ) {
		case MDB_KEYEXIST:
			rs->sr_err = LDAP_ALREADY_EXISTS;
			break;
		default:
			rs->sr_err = LDAP_OTHER;
		}
		goto return_results;
	}

	/* attribute indexes */
	rs->sr_err = mdb_index_entry_add( op, txn, op->ora_e );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": index_entry_add failed\n",
			0, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "index generation failed";
		goto return_results;
	}

	/* id2entry index */
	rs->sr_err = mdb_id2entry_add( op, txn, mc, op->ora_e );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_add) ": id2entry_add failed\n",
			0, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "entry store failed";
		goto return_results;
	}

	/* post-read */
	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, op->ora_e,
			&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_add) ": post-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	if ( moi == &opinfo ) {
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.moi_oe, OpExtra, oe_next );
		opinfo.moi_oe.oe_key = NULL;
		if ( op->o_noop ) {
			mdb_txn_abort( txn );
			rs->sr_err = LDAP_X_NO_OPERATION;
			txn = NULL;
			goto return_results;
		}

		rs->sr_err = mdb_txn_commit( txn );
		txn = NULL;
		if ( rs->sr_err != 0 ) {
			rs->sr_text = "txn_commit failed";
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(mdb_add) ": %s : %s (%d)\n",
				rs->sr_text, mdb_strerror(rs->sr_err), rs->sr_err );
			rs->sr_err = LDAP_OTHER;
			goto return_results;
		}
	}

	Debug(LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_add) ": added%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		op->ora_e->e_id, op->ora_e->e_dn );

	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	success = rs->sr_err;
	send_ldap_result( op, rs );

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

	if( success == LDAP_SUCCESS ) {
#if 0
		if ( mdb->bi_txn_cp_kbyte ) {
			TXN_CHECKPOINT( mdb->bi_dbenv,
				mdb->bi_txn_cp_kbyte, mdb->bi_txn_cp_min, 0 );
		}
#endif
	}

	slap_graduate_commit_csn( op );

	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}
	return rs->sr_err;
}
