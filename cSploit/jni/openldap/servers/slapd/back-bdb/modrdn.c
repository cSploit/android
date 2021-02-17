/* modrdn.c - bdb backend modrdn routine */
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

#include "back-bdb.h"

int
bdb_modrdn( Operation	*op, SlapReply *rs )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	struct berval	p_dn, p_ndn;
	struct berval	new_dn = {0, NULL}, new_ndn = {0, NULL};
	Entry		*e = NULL;
	Entry		*p = NULL;
	EntryInfo	*ei = NULL, *eip = NULL, *nei = NULL, *neip = NULL;
	/* LDAP v2 supporting correct attribute handling. */
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	DB_TXN		*ltid = NULL, *lt2;
	struct bdb_op_info opinfo = {{{ 0 }}};
	Entry dummy = {0};

	Entry		*np = NULL;			/* newSuperior Entry */
	struct berval	*np_dn = NULL;			/* newSuperior dn */
	struct berval	*np_ndn = NULL;			/* newSuperior ndn */
	struct berval	*new_parent_dn = NULL;	/* np_dn, p_dn, or NULL */

	int		manageDSAit = get_manageDSAit( op );

	DB_LOCK		lock, plock, nplock;

	int		num_retries = 0;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int	rc;

	int parent_is_glue = 0;
	int parent_is_leaf = 0;

#ifdef LDAP_X_TXN
	int settle = 0;
#endif

	Debug( LDAP_DEBUG_TRACE, "==>" LDAP_XSTRING(bdb_modrdn) "(%s,%s,%s)\n",
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

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	if( 0 ) {
retry:	/* transaction retry */
		if ( dummy.e_attrs ) {
			attrs_free( dummy.e_attrs );
			dummy.e_attrs = NULL;
		}
		if (e != NULL) {
			bdb_unlocked_cache_return_entry_w(&bdb->bi_cache, e);
			e = NULL;
		}
		if (p != NULL) {
			bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, p);
			p = NULL;
		}
		if (np != NULL) {
			bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, np);
			np = NULL;
		}
		Debug( LDAP_DEBUG_TRACE, "==>" LDAP_XSTRING(bdb_modrdn)
				": retrying...\n", 0, 0, 0 );

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
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_modrdn) ": txn_begin failed: "
			"%s (%d)\n", db_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_modrdn) ": txn1 id: %x\n",
		ltid->id(ltid), 0, 0 );

	opinfo.boi_oe.oe_key = bdb;
	opinfo.boi_txn = ltid;
	opinfo.boi_err = 0;
	opinfo.boi_acl_cache = op->o_do_not_cache;
	LDAP_SLIST_INSERT_HEAD( &op->o_extra, &opinfo.boi_oe, oe_next );

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

	e = ei->bei_e;
	/* FIXME: dn2entry() should return non-glue entry */
	if (( rs->sr_err == DB_NOTFOUND ) ||
		( !manageDSAit && e && is_entry_glue( e )))
	{
		if( e != NULL ) {
			rs->sr_matched = ch_strdup( e->e_dn );
			rs->sr_ref = is_entry_referral( e )
				? get_entry_referrals( op, e )
				: NULL;
			bdb_unlocked_cache_return_entry_r( &bdb->bi_cache, e);
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
		switch( opinfo.boi_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}

		Debug( LDAP_DEBUG_TRACE, "no access to entry\n", 0,
			0, 0 );
		rs->sr_text = "no write access to old entry";
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

#ifndef BDB_HIER
	rs->sr_err = bdb_cache_children( op, ltid, e );
	if ( rs->sr_err != DB_NOTFOUND ) {
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		case 0:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(bdb_modrdn)
				": non-leaf %s\n",
				op->o_req_dn.bv_val, 0, 0);
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			rs->sr_text = "subtree rename not supported";
			break;
		default:
			Debug(LDAP_DEBUG_ARGS,
				"<=- " LDAP_XSTRING(bdb_modrdn)
				": has_children failed: %s (%d)\n",
				db_strerror(rs->sr_err), rs->sr_err, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
		}
		goto return_results;
	}
	ei->bei_state |= CACHE_ENTRY_NO_KIDS;
#endif

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* parent is a referral, don't allow add */
		rs->sr_ref = get_entry_referrals( op, e );

		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_modrdn)
			": entry %s is referral\n", e->e_dn, 0, 0 );

		rs->sr_err = LDAP_REFERRAL,
		rs->sr_matched = e->e_name.bv_val;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;
		goto done;
	}

	if ( be_issuffix( op->o_bd, &e->e_nname ) ) {
#ifdef BDB_MULTIPLE_SUFFIXES
		/* Allow renaming one suffix entry to another */
		p_ndn = slap_empty_bv;
#else
		/* There can only be one suffix entry */
		rs->sr_err = LDAP_NAMING_VIOLATION;
		rs->sr_text = "cannot rename suffix entry";
		goto return_results;
#endif
	} else {
		dnParent( &e->e_nname, &p_ndn );
	}
	np_ndn = &p_ndn;
	eip = ei->bei_parent;
	if ( eip && eip->bei_id ) {
		/* Make sure parent entry exist and we can write its 
		 * children.
		 */
		rs->sr_err = bdb_cache_find_id( op, ltid,
			eip->bei_id, &eip, 0, &plock );

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

		p = eip->bei_e;
		if( p == NULL) {
			Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_modrdn)
				": parent does not exist\n", 0, 0, 0);
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "old entry's parent does not exist";
			goto return_results;
		}
	} else {
		p = (Entry *)&slap_entry_root;
	}

	/* check parent for "children" acl */
	rs->sr_err = access_allowed( op, p,
		children, NULL,
		op->oq_modrdn.rs_newSup == NULL ?
			ACL_WRITE : ACL_WDEL,
		NULL );

	if ( !p_ndn.bv_len )
		p = NULL;

	if ( ! rs->sr_err ) {
		switch( opinfo.boi_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}

		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		Debug( LDAP_DEBUG_TRACE, "no access to parent\n", 0,
			0, 0 );
		rs->sr_text = "no write access to old parent's children";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_modrdn) ": wr to children "
		"of entry %s OK\n", p_ndn.bv_val, 0, 0 );
	
	if ( p_ndn.bv_val == slap_empty_bv.bv_val ) {
		p_dn = slap_empty_bv;
	} else {
		dnParent( &e->e_name, &p_dn );
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_modrdn) ": parent dn=%s\n",
		p_dn.bv_val, 0, 0 );

	new_parent_dn = &p_dn;	/* New Parent unless newSuperior given */

	if ( op->oq_modrdn.rs_newSup != NULL ) {
		Debug( LDAP_DEBUG_TRACE, 
			LDAP_XSTRING(bdb_modrdn)
			": new parent \"%s\" requested...\n",
			op->oq_modrdn.rs_newSup->bv_val, 0, 0 );

		/*  newSuperior == oldParent? */
		if( dn_match( &p_ndn, op->oq_modrdn.rs_nnewSup ) ) {
			Debug( LDAP_DEBUG_TRACE, "bdb_back_modrdn: "
				"new parent \"%s\" same as the old parent \"%s\"\n",
				op->oq_modrdn.rs_newSup->bv_val, p_dn.bv_val, 0 );
			op->oq_modrdn.rs_newSup = NULL; /* ignore newSuperior */
		}
	}

	/* There's a BDB_MULTIPLE_SUFFIXES case here that this code doesn't
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

			rs->sr_err = bdb_dn2entry( op, ltid, np_ndn,
				&neip, 0, &nplock );

			switch( rs->sr_err ) {
			case 0: np = neip->bei_e;
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

			if( np == NULL) {
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(bdb_modrdn)
					": newSup(ndn=%s) not here!\n",
					np_ndn->bv_val, 0, 0);
				rs->sr_text = "new superior not found";
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				goto return_results;
			}

			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(bdb_modrdn)
				": wr to new parent OK np=%p, id=%ld\n",
				(void *) np, (long) np->e_id, 0 );

			/* check newSuperior for "children" acl */
			rs->sr_err = access_allowed( op, np, children,
				NULL, ACL_WADD, NULL );

			if( ! rs->sr_err ) {
				switch( opinfo.boi_err ) {
				case DB_LOCK_DEADLOCK:
				case DB_LOCK_NOTGRANTED:
					goto retry;
				}

				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(bdb_modrdn)
					": no wr to newSup children\n",
					0, 0, 0 );
				rs->sr_text = "no write access to new superior's children";
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				goto return_results;
			}

			if ( is_entry_alias( np ) ) {
				/* parent is an alias, don't allow add */
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(bdb_modrdn)
					": entry is alias\n",
					0, 0, 0 );
				rs->sr_text = "new superior is an alias";
				rs->sr_err = LDAP_ALIAS_PROBLEM;
				goto return_results;
			}

			if ( is_entry_referral( np ) ) {
				/* parent is a referral, don't allow add */
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(bdb_modrdn)
					": entry is referral\n",
					0, 0, 0 );
				rs->sr_text = "new superior is a referral";
				rs->sr_err = LDAP_OTHER;
				goto return_results;
			}

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
					switch( opinfo.boi_err ) {
					case DB_LOCK_DEADLOCK:
					case DB_LOCK_NOTGRANTED:
						goto retry;
					}

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
			LDAP_XSTRING(bdb_modrdn)
			": wr to new parent's children OK\n",
			0, 0, 0 );

		new_parent_dn = np_dn;
	}

	/* Build target dn and make sure target entry doesn't exist already. */
	if (!new_dn.bv_val) {
		build_new_dn( &new_dn, new_parent_dn, &op->oq_modrdn.rs_newrdn, NULL ); 
	}

	if (!new_ndn.bv_val) {
		struct berval bv = {0, NULL};
		dnNormalize( 0, NULL, NULL, &new_dn, &bv, op->o_tmpmemctx );
		ber_dupbv( &new_ndn, &bv );
		/* FIXME: why not call dnNormalize() w/o ctx? */
		op->o_tmpfree( bv.bv_val, op->o_tmpmemctx );
	}

	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_modrdn) ": new ndn=%s\n",
		new_ndn.bv_val, 0, 0 );

	/* Shortcut the search */
	nei = neip ? neip : eip;
	rs->sr_err = bdb_cache_find_ndn ( op, ltid, &new_ndn, &nei );
	if ( nei ) bdb_cache_entryinfo_unlock( nei );
	switch( rs->sr_err ) {
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto retry;
	case DB_NOTFOUND:
		break;
	case 0:
		/* Allow rename to same DN */
		if ( nei == ei )
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
				"<=- " LDAP_XSTRING(bdb_modrdn)
				": pre-read failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}                   
	}

	/* nested transaction */
	rs->sr_err = TXN_BEGIN( bdb->bi_dbenv, ltid, &lt2, bdb->bi_db_opflags );
	rs->sr_text = NULL;
	if( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_modrdn)
			": txn_begin(2) failed: %s (%d)\n",
			db_strerror(rs->sr_err), rs->sr_err, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_modrdn) ": txn2 id: %x\n",
		lt2->id(lt2), 0, 0 );

	/* delete old DN */
	rs->sr_err = bdb_dn2id_delete( op, lt2, eip, e );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_modrdn)
			": dn2id del failed: %s (%d)\n",
			db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
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
	rs->sr_err = bdb_dn2id_add( op, lt2, neip ? neip : eip, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_modrdn)
			": dn2id add failed: %s (%d)\n",
			db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN index add failed";
		goto return_results;
	}

	dummy.e_attrs = e->e_attrs;

	/* modify entry */
	rs->sr_err = bdb_modify_internal( op, lt2, op->orr_modlist, &dummy,
		&rs->sr_text, textbuf, textlen );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_modrdn)
			": modify failed: %s (%d)\n",
			db_strerror(rs->sr_err), rs->sr_err, 0 );
		if ( ( rs->sr_err == LDAP_INSUFFICIENT_ACCESS ) && opinfo.boi_err ) {
			rs->sr_err = opinfo.boi_err;
		}
		if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		goto return_results;
	}

	/* id2entry index */
	rs->sr_err = bdb_id2entry_update( op->o_bd, lt2, &dummy );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(bdb_modrdn)
			": id2entry failed: %s (%d)\n",
			db_strerror(rs->sr_err), rs->sr_err, 0 );
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "entry update failed";
		goto return_results;
	}

	if ( p_ndn.bv_len != 0 ) {
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
					"<=- " LDAP_XSTRING(bdb_modrdn)
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

	if ( TXN_COMMIT( lt2, 0 ) != 0 ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "txn_commit(2) failed";
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
				"<=- " LDAP_XSTRING(bdb_modrdn)
				": post-read failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}                   
	}

	if( op->o_noop ) {
		if(( rs->sr_err=TXN_ABORT( ltid )) != 0 ) {
			rs->sr_text = "txn_abort (no-op) failed";
		} else {
			rs->sr_err = LDAP_X_NO_OPERATION;
			ltid = NULL;
			/* Only free attrs if they were dup'd.  */
			if ( dummy.e_attrs == e->e_attrs ) dummy.e_attrs = NULL;
			goto return_results;
		}

	} else {
		rc = bdb_cache_modrdn( bdb, e, &op->orr_nnewrdn, &dummy, neip,
			ltid, &lock );
		switch( rc ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
		dummy.e_attrs = NULL;
		new_dn.bv_val = NULL;
		new_ndn.bv_val = NULL;

		if(( rs->sr_err=TXN_COMMIT( ltid, 0 )) != 0 ) {
			rs->sr_text = "txn_commit failed";
		} else {
			rs->sr_err = LDAP_SUCCESS;
		}
	}
 
	ltid = NULL;
	LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.boi_oe, OpExtra, oe_next );
	opinfo.boi_oe.oe_key = NULL;
 
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_modrdn) ": %s : %s (%d)\n",
			rs->sr_text, db_strerror(rs->sr_err), rs->sr_err );
		rs->sr_err = LDAP_OTHER;

		goto return_results;
	}

	Debug(LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_modrdn)
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

	if( rs->sr_err == LDAP_SUCCESS && bdb->bi_txn_cp_kbyte ) {
		TXN_CHECKPOINT( bdb->bi_dbenv,
			bdb->bi_txn_cp_kbyte, bdb->bi_txn_cp_min, 0 );
	}
	
	if ( rs->sr_err == LDAP_SUCCESS && parent_is_glue && parent_is_leaf ) {
		op->o_delete_glue_parent = 1;
	}

done:
	slap_graduate_commit_csn( op );

	if( new_dn.bv_val != NULL ) free( new_dn.bv_val );
	if( new_ndn.bv_val != NULL ) free( new_ndn.bv_val );

	/* LDAP v3 Support */
	if( np != NULL ) {
		/* free new parent and reader lock */
		bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, np);
	}

	if( p != NULL ) {
		/* free parent and reader lock */
		bdb_unlocked_cache_return_entry_r(&bdb->bi_cache, p);
	}

	/* free entry */
	if( e != NULL ) {
		bdb_unlocked_cache_return_entry_w( &bdb->bi_cache, e);
	}

	if( ltid != NULL ) {
		TXN_ABORT( ltid );
	}
	if ( opinfo.boi_oe.oe_key ) {
		LDAP_SLIST_REMOVE( &op->o_extra, &opinfo.boi_oe, OpExtra, oe_next );
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
