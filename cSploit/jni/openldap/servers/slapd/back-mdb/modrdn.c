/* modrdn.c - mdb backend modrdn routine */
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
mdb_modrdn( Operation	*op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	struct berval	p_dn, p_ndn;
	struct berval	new_dn = {0, NULL}, new_ndn = {0, NULL};
	Entry		*e = NULL;
	Entry		*p = NULL;
	/* LDAP v2 supporting correct attribute handling. */
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	MDB_txn		*txn = NULL;
	MDB_cursor	*mc;
	struct mdb_op_info opinfo = {{{ 0 }}}, *moi = &opinfo;
	Entry dummy = {0};

	Entry		*np = NULL;			/* newSuperior Entry */
	struct berval	*np_dn = NULL;			/* newSuperior dn */
	struct berval	*np_ndn = NULL;			/* newSuperior ndn */
	struct berval	*new_parent_dn = NULL;	/* np_dn, p_dn, or NULL */

	int		manageDSAit = get_manageDSAit( op );

	ID nid, nsubs;
	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int parent_is_glue = 0;
	int parent_is_leaf = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug( LDAP_DEBUG_TRACE, "==>" LDAP_XSTRING(mdb_modrdn) "(%s,%s,%s)\n",
		op->o_req_dn.bv_val,op->oq_modrdn.rs_newrdn.bv_val,
		op->oq_modrdn.rs_newSup ? op->oq_modrdn.rs_newSup->bv_val : "NULL" );

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
			LDAP_XSTRING(mdb_modrdn) ": txn_begin failed: "
			"%s (%d)\n", mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	txn = moi->moi_txn;

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	if ( be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
#ifdef MDB_MULTIPLE_SUFFIXES
		/* Allow renaming one suffix entry to another */
		p_ndn = slap_empty_bv;
#else
		/* There can only be one suffix entry */
		rs->sr_err = LDAP_NAMING_VIOLATION;
		rs->sr_text = "cannot rename suffix entry";
		goto return_results;
#endif
	} else {
		dnParent( &op->o_req_ndn, &p_ndn );
	}
	np_ndn = &p_ndn;
	/* Make sure parent entry exist and we can write its
	 * children.
	 */
	rs->sr_err = mdb_cursor_open( txn, mdb->mi_dn2id, &mc );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_modrdn)
			": cursor_open failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN cursor_open failed";
		goto return_results;
	}
	rs->sr_err = mdb_dn2entry( op, txn, mc, &p_ndn, &p, NULL, 0 );
	switch( rs->sr_err ) {
	case MDB_NOTFOUND:
		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(mdb_modrdn)
			": parent does not exist\n", 0, 0, 0);
		rs->sr_ref = referral_rewrite( default_referral, NULL,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
		rs->sr_err = LDAP_REFERRAL;

		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		goto done;
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

	/* check parent for "children" acl */
	rs->sr_err = access_allowed( op, p,
		children, NULL,
		op->oq_modrdn.rs_newSup == NULL ?
			ACL_WRITE : ACL_WDEL,
		NULL );

	if ( ! rs->sr_err ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		Debug( LDAP_DEBUG_TRACE, "no access to parent\n", 0,
			0, 0 );
		rs->sr_text = "no write access to parent's children";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_modrdn) ": wr to children "
		"of entry %s OK\n", p_ndn.bv_val, 0, 0 );

	if ( p_ndn.bv_val == slap_empty_bv.bv_val ) {
		p_dn = slap_empty_bv;
	} else {
		dnParent( &op->o_req_dn, &p_dn );
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_modrdn) ": parent dn=%s\n",
		p_dn.bv_val, 0, 0 );

	/* get entry */
	rs->sr_err = mdb_dn2entry( op, txn, mc, &op->o_req_ndn, &e, &nsubs, 0 );
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

	/* FIXME: dn2entry() should return non-glue entry */
	if (( rs->sr_err == MDB_NOTFOUND ) ||
		( !manageDSAit && e && is_entry_glue( e )))
	{
		if( e != NULL ) {
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

		rs->sr_err = LDAP_REFERRAL;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		free( (char *)rs->sr_matched );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;

		goto done;
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	/* check write on old entry */
	rs->sr_err = access_allowed( op, e, entry, NULL, ACL_WRITE, NULL );
	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE, "no access to entry\n", 0,
			0, 0 );
		rs->sr_text = "no write access to old entry";
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow rename */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(mdb_modrdn)
			": entry %s is referral\n", e->e_dn, 0, 0 );

		rs->sr_err = LDAP_REFERRAL,
		rs->sr_matched = e->e_name.bv_val;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;
		goto done;
	}

	new_parent_dn = &p_dn;	/* New Parent unless newSuperior given */

	if ( op->oq_modrdn.rs_newSup != NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modrdn)
			": new parent \"%s\" requested...\n",
			op->oq_modrdn.rs_newSup->bv_val, 0, 0 );

		/*  newSuperior == oldParent? */
		if( dn_match( &p_ndn, op->oq_modrdn.rs_nnewSup ) ) {
			Debug( LDAP_DEBUG_TRACE, "mdb_back_modrdn: "
				"new parent \"%s\" same as the old parent \"%s\"\n",
				op->oq_modrdn.rs_newSup->bv_val, p_dn.bv_val, 0 );
			op->oq_modrdn.rs_newSup = NULL; /* ignore newSuperior */
		}
	}

	/* There's a MDB_MULTIPLE_SUFFIXES case here that this code doesn't
	 * support. E.g., two suffixes dc=foo,dc=com and dc=bar,dc=net.
	 * We do not allow modDN
	 *   dc=foo,dc=com
	 *    newrdn dc=bar
	 *    newsup dc=net
	 * and we probably should. But since MULTIPLE_SUFFIXES is deprecated
	 * I'm ignoring this problem for now.
	 */
	if ( op->oq_modrdn.rs_newSup != NULL ) {
		if ( op->oq_modrdn.rs_newSup->bv_len ) {
			np_dn = op->oq_modrdn.rs_newSup;
			np_ndn = op->oq_modrdn.rs_nnewSup;

			/* newSuperior == oldParent? - checked above */
			/* newSuperior == entry being moved?, if so ==> ERROR */
			if ( dnIsSuffix( np_ndn, &e->e_nname )) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "new superior not found";
				goto return_results;
			}
			/* Get Entry with dn=newSuperior. Does newSuperior exist? */
			rs->sr_err = mdb_dn2entry( op, txn, NULL, np_ndn, &np, NULL, 0 );

			switch( rs->sr_err ) {
			case 0:
				break;
			case MDB_NOTFOUND:
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(mdb_modrdn)
					": newSup(ndn=%s) not here!\n",
					np_ndn->bv_val, 0, 0);
				rs->sr_text = "new superior not found";
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				goto return_results;
			case LDAP_BUSY:
				rs->sr_text = "ldap server busy";
				goto return_results;
			default:
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";
				goto return_results;
			}

			/* check newSuperior for "children" acl */
			rs->sr_err = access_allowed( op, np, children,
				NULL, ACL_WADD, NULL );

			if( ! rs->sr_err ) {
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(mdb_modrdn)
					": no wr to newSup children\n",
					0, 0, 0 );
				rs->sr_text = "no write access to new superior's children";
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				goto return_results;
			}

			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_modrdn)
				": wr to new parent OK np=%p, id=%ld\n",
				(void *) np, (long) np->e_id, 0 );

			if ( is_entry_alias( np ) ) {
				/* parent is an alias, don't allow add */
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(mdb_modrdn)
					": entry is alias\n",
					0, 0, 0 );
				rs->sr_text = "new superior is an alias";
				rs->sr_err = LDAP_ALIAS_PROBLEM;
				goto return_results;
			}

			if ( is_entry_referral( np ) ) {
				/* parent is a referral, don't allow add */
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(mdb_modrdn)
					": entry is referral\n",
					0, 0, 0 );
				rs->sr_text = "new superior is a referral";
				rs->sr_err = LDAP_OTHER;
				goto return_results;
			}
			new_parent_dn = &np->e_name;

		} else {
			np_dn = NULL;

			/* no parent, modrdn entry directly under root */
			if ( be_issuffix( op->o_bd, (struct berval *)&slap_empty_bv )
				|| be_isupdate( op ) ) {
				np = (Entry *)&slap_entry_root;

				/* check parent for "children" acl */
				rs->sr_err = access_allowed( op, np,
					children, NULL, ACL_WADD, NULL );

				np = NULL;

				if ( ! rs->sr_err ) {
					rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					Debug( LDAP_DEBUG_TRACE,
						"no access to new superior\n",
						0, 0, 0 );
					rs->sr_text =
						"no write access to new superior's children";
					goto return_results;
				}
			}
		}

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_modrdn)
			": wr to new parent's children OK\n",
			0, 0, 0 );

		new_parent_dn = np_dn;
	}

	/* Build target dn and make sure target entry doesn't exist already. */
	if (!new_dn.bv_val) {
		build_new_dn( &new_dn, new_parent_dn, &op->oq_modrdn.rs_newrdn, op->o_tmpmemctx );
	}

	if (!new_ndn.bv_val) {
		dnNormalize( 0, NULL, NULL, &new_dn, &new_ndn, op->o_tmpmemctx );
	}

	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(mdb_modrdn) ": new ndn=%s\n",
		new_ndn.bv_val, 0, 0 );

	/* Shortcut the search */
	rs->sr_err = mdb_dn2id ( op, txn, NULL, &new_ndn, &nid, NULL, NULL, NULL );
	switch( rs->sr_err ) {
	case MDB_NOTFOUND:
		break;
	case 0:
		/* Allow rename to same DN */
		if ( nid == e->e_id )
			break;
		rs->sr_err = LDAP_ALREADY_EXISTS;
		goto return_results;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	assert( op->orr_modlist != NULL );

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(mdb_modrdn)
				": pre-read failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* delete old DN
	 * If moving to a new parent, must delete current subtree count,
	 * otherwise leave it unchanged since we'll be adding it right back.
	 */
	rs->sr_err = mdb_dn2id_delete( op, mc, e->e_id, np ? nsubs : 0 );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_modrdn)
			": dn2id del failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN index delete fail";
		goto return_results;
	}

	/* copy the entry, then override some fields */
	dummy = *e;
	dummy.e_name = new_dn;
	dummy.e_nname = new_ndn;
	dummy.e_attrs = NULL;

	/* add new DN */
	rs->sr_err = mdb_dn2id_add( op, mc, mc, np ? np->e_id : p->e_id,
		nsubs, np != NULL, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_modrdn)
			": dn2id add failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN index add failed";
		goto return_results;
	}

	dummy.e_attrs = e->e_attrs;

	/* modify entry */
	rs->sr_err = mdb_modify_internal( op, txn, op->orr_modlist, &dummy,
		&rs->sr_text, textbuf, textlen );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_modrdn)
			": modify failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
		goto return_results;
	}

	/* id2entry index */
	rs->sr_err = mdb_id2entry_update( op, txn, NULL, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(mdb_modrdn)
			": id2entry failed: %s (%d)\n",
			mdb_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "entry update failed";
		goto return_results;
	}

	if ( p_ndn.bv_len != 0 ) {
		if ((parent_is_glue = is_entry_glue(p))) {
			rs->sr_err = mdb_dn2id_children( op, txn, p );
			if ( rs->sr_err != MDB_NOTFOUND ) {
				switch( rs->sr_err ) {
				case 0:
					break;
				default:
					Debug(LDAP_DEBUG_ARGS,
						"<=- " LDAP_XSTRING(mdb_modrdn)
						": has_children failed: %s (%d)\n",
						mdb_strerror(rs->sr_err), rs->sr_err, 0 );
					rs->sr_err = LDAP_OTHER;
					rs->sr_text = "internal error";
					goto return_results;
				}
			} else {
				parent_is_leaf = 1;
			}
		}
		mdb_entry_return( op, p );
		p = NULL;
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
				"<=- " LDAP_XSTRING(mdb_modrdn)
				": post-read failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	if( moi == &opinfo ) {
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.moi_oe, OpExtra, oe_next );
		opinfo.moi_oe.oe_key = NULL;
		if( op->o_noop ) {
			mdb_txn_abort( txn );
			rs->sr_err = LDAP_X_NO_OPERATION;
			txn = NULL;
			/* Only free attrs if they were dup'd.  */
			if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
			goto return_results;

		} else {
			if(( rs->sr_err=mdb_txn_commit( txn )) != 0 ) {
				rs->sr_text = "txn_commit failed";
			} else {
				rs->sr_err = LDAP_SUCCESS;
			}
			txn = NULL;
		}
	}

	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_modrdn) ": %s : %s (%d)\n",
			rs->sr_text, mdb_strerror(rs->sr_err), rs->sr_err );
		rs->sr_err = LDAP_OTHER;

		goto return_results;
	}

	Debug(LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_modrdn)
		": rdn modified%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		dummy.e_id, op->o_req_dn.bv_val );
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( dummy.e_attrs ) {
		attrs_free( dummy.e_attrs );
	}
	send_ldap_result( op, rs );

#if 0
	if( rs->sr_err == LDAP_SUCCESS && mdb->bi_txn_cp_kbyte ) {
		TXN_CHECKPOINT( mdb->bi_dbenv,
			mdb->bi_txn_cp_kbyte, mdb->bi_txn_cp_min, 0 );
	}
#endif

	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

done:
	slap_graduate_commit_csn( op );

	if( new_ndn.bv_val != NULL ) op->o_tmpfree( new_ndn.bv_val, op->o_tmpmemctx );
	if( new_dn.bv_val != NULL ) op->o_tmpfree( new_dn.bv_val, op->o_tmpmemctx );

	/* LDAP v3 Support */
	if( np != NULL ) {
		/* free new parent */
		mdb_entry_return( op, np );
	}

	if( p != NULL ) {
		/* free parent */
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

	if( preread_ctrl != NULL && (*preread_ctrl) != NULL ) {
		slap_sl_free( (*preread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *preread_ctrl, op->o_tmpmemctx );
	}
	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}
	return rs->sr_err;
}
