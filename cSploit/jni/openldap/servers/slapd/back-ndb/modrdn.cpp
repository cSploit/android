/* modrdn.cpp - ndb backend modrdn routine */
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

#include "back-ndb.h"

int
ndb_back_modrdn( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	struct berval	new_dn = BER_BVNULL, new_ndn = BER_BVNULL;
	Entry		e = {0};
	Entry		e2 = {0};
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;

	struct berval	*np_dn = NULL;			/* newSuperior dn */
	struct berval	*np_ndn = NULL;			/* newSuperior ndn */

	int		manageDSAit = get_manageDSAit( op );
	int		num_retries = 0;

	NdbArgs NA, NA2;
	NdbRdns rdns, rdn2;
	struct berval matched;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	int	rc;

	Debug( LDAP_DEBUG_ARGS, "==>" LDAP_XSTRING(ndb_back_modrdn) "(%s,%s,%s)\n",
		op->o_req_dn.bv_val,op->oq_modrdn.rs_newrdn.bv_val,
		op->oq_modrdn.rs_newSup ? op->oq_modrdn.rs_newSup->bv_val : "NULL" );

	ctrls[num_ctrls] = NULL;

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );
	rdns.nr_num = 0;
	NA.rdns = &rdns;
	NA.e = &e;
	NA2.ndb = NA.ndb;
	NA2.e = &e2;
	NA2.rdns = &rdn2;

	if( 0 ) {
retry:	/* transaction retry */
		NA.txn->close();
		NA.txn = NULL;
		if ( e.e_attrs ) {
			attrs_free( e.e_attrs );
			e.e_attrs = NULL;
		}
		Debug( LDAP_DEBUG_TRACE, "==>" LDAP_XSTRING(ndb_back_modrdn)
				": retrying...\n", 0, 0, 0 );
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto return_results;
		}
		if ( NA2.ocs ) {
			ber_bvarray_free_x( NA2.ocs, op->o_tmpmemctx );
		}
		if ( NA.ocs ) {
			ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		}
		ndb_trans_backoff( ++num_retries );
	}
	NA.ocs = NULL;
	NA2.ocs = NULL;

	/* begin transaction */
	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modrdn) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	NA2.txn = NA.txn;

	/* get entry */
	rs->sr_err = ndb_entry_get_info( op, &NA, 1, &matched );
	switch( rs->sr_err ) {
	case 0:
		break;
	case LDAP_NO_SUCH_OBJECT:
		Debug( LDAP_DEBUG_ARGS,
			"<=- ndb_back_modrdn: no such object %s\n",
			op->o_req_dn.bv_val, 0, 0 );
		rs->sr_matched = matched.bv_val;
		if ( NA.ocs )
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
	if ( rs->sr_err )
		goto return_results;

	if ( !manageDSAit && is_entry_glue( &e )) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto return_results;
	}
	
	if ( get_assert( op ) &&
		( test_filter( op, &e, (Filter *)get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	/* check write on old entry */
	rs->sr_err = access_allowed( op, &e, entry, NULL, ACL_WRITE, NULL );
	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE, "no access to entry\n", 0,
			0, 0 );
		rs->sr_text = "no write access to old entry";
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

	/* Can't do it if we have kids */
	rs->sr_err = ndb_has_children( &NA, &rc );
	if ( rs->sr_err ) {
		Debug(LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(ndb_back_modrdn)
			": has_children failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	if ( rc == LDAP_COMPARE_TRUE ) {
		Debug(LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(ndb_back_modrdn)
			": non-leaf %s\n",
			op->o_req_dn.bv_val, 0, 0);
		rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
		rs->sr_text = "subtree rename not supported";
		goto return_results;
	}

	if (!manageDSAit && is_entry_referral( &e ) ) {
		/* entry is a referral, don't allow modrdn */
		rs->sr_ref = get_entry_referrals( op, &e );

		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(ndb_back_modrdn)
			": entry %s is referral\n", e.e_dn, 0, 0 );

		rs->sr_err = LDAP_REFERRAL,
		rs->sr_matched = op->o_req_dn.bv_val;
		rs->sr_flags = REP_REF_MUSTBEFREED;
		goto return_results;
	}

	if ( be_issuffix( op->o_bd, &e.e_nname ) ) {
		/* There can only be one suffix entry */
		rs->sr_err = LDAP_NAMING_VIOLATION;
		rs->sr_text = "cannot rename suffix entry";
		goto return_results;
	} else {
		dnParent( &e.e_nname, &e2.e_nname );
		dnParent( &e.e_name, &e2.e_name );
	}

	/* check parent for "children" acl */
	rs->sr_err = access_allowed( op, &e2,
		children, NULL,
		op->oq_modrdn.rs_newSup == NULL ?
			ACL_WRITE : ACL_WDEL,
		NULL );

	if ( ! rs->sr_err ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		Debug( LDAP_DEBUG_TRACE, "no access to parent\n", 0,
			0, 0 );
		rs->sr_text = "no write access to old parent's children";
		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_modrdn) ": wr to children "
		"of entry %s OK\n", e2.e_name.bv_val, 0, 0 );
	
	if ( op->oq_modrdn.rs_newSup != NULL ) {
		Debug( LDAP_DEBUG_TRACE, 
			LDAP_XSTRING(ndb_back_modrdn)
			": new parent \"%s\" requested...\n",
			op->oq_modrdn.rs_newSup->bv_val, 0, 0 );

		/*  newSuperior == oldParent? */
		if( dn_match( &e2.e_nname, op->oq_modrdn.rs_nnewSup ) ) {
			Debug( LDAP_DEBUG_TRACE, "bdb_back_modrdn: "
				"new parent \"%s\" same as the old parent \"%s\"\n",
				op->oq_modrdn.rs_newSup->bv_val, e2.e_name.bv_val, 0 );
			op->oq_modrdn.rs_newSup = NULL; /* ignore newSuperior */
		}
	}

	if ( op->oq_modrdn.rs_newSup != NULL ) {
		if ( op->oq_modrdn.rs_newSup->bv_len ) {
			rdn2.nr_num = 0;
			np_dn = op->oq_modrdn.rs_newSup;
			np_ndn = op->oq_modrdn.rs_nnewSup;

			/* newSuperior == oldParent? - checked above */
			/* newSuperior == entry being moved?, if so ==> ERROR */
			if ( dnIsSuffix( np_ndn, &e.e_nname )) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = "new superior not found";
				goto return_results;
			}
			/* Get Entry with dn=newSuperior. Does newSuperior exist? */

			e2.e_name = *np_dn;
			e2.e_nname = *np_ndn;
			rs->sr_err = ndb_entry_get_info( op, &NA2, 1, NULL );
			switch( rs->sr_err ) {
			case 0:
				break;
			case LDAP_NO_SUCH_OBJECT:
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(ndb_back_modrdn)
					": newSup(ndn=%s) not here!\n",
					np_ndn->bv_val, 0, 0);
				rs->sr_text = "new superior not found";
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
			if ( NA2.ocs ) {
				Attribute a;
				int i;

				for ( i=0; !BER_BVISNULL( &NA2.ocs[i] ); i++);
				a.a_numvals = i;
				a.a_desc = slap_schema.si_ad_objectClass;
				a.a_vals = NA2.ocs;
				a.a_nvals = NA2.ocs;
				a.a_next = NULL;
				e2.e_attrs = &a;

				if ( is_entry_alias( &e2 )) {
					/* parent is an alias, don't allow move */
					Debug( LDAP_DEBUG_TRACE,
						LDAP_XSTRING(ndb_back_modrdn)
						": entry is alias\n",
						0, 0, 0 );
					rs->sr_text = "new superior is an alias";
					rs->sr_err = LDAP_ALIAS_PROBLEM;
					goto return_results;
				}

				if ( is_entry_referral( &e2 ) ) {
					/* parent is a referral, don't allow move */
					Debug( LDAP_DEBUG_TRACE,
						LDAP_XSTRING(ndb_back_modrdn)
						": entry is referral\n",
						0, 0, 0 );
					rs->sr_text = "new superior is a referral";
					rs->sr_err = LDAP_OTHER;
					goto return_results;
				}
			}
		}

		/* check newSuperior for "children" acl */
		rs->sr_err = access_allowed( op, &e2, children,
			NULL, ACL_WADD, NULL );
		if( ! rs->sr_err ) {
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(ndb_back_modrdn)
				": no wr to newSup children\n",
				0, 0, 0 );
			rs->sr_text = "no write access to new superior's children";
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			goto return_results;
		}

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_modrdn)
			": wr to new parent OK id=%ld\n",
			(long) e2.e_id, 0, 0 );
	}

	/* Build target dn and make sure target entry doesn't exist already. */
	if (!new_dn.bv_val) {
		build_new_dn( &new_dn, &e2.e_name, &op->oq_modrdn.rs_newrdn, NULL ); 
	}

	if (!new_ndn.bv_val) {
		build_new_dn( &new_ndn, &e2.e_nname, &op->oq_modrdn.rs_nnewrdn, NULL ); 
	}

	Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(ndb_back_modrdn) ": new ndn=%s\n",
		new_ndn.bv_val, 0, 0 );

	/* Allow rename to same DN */
	if ( !bvmatch ( &new_ndn, &e.e_nname )) {
		rdn2.nr_num = 0;
		e2.e_name = new_dn;
		e2.e_nname = new_ndn;
		NA2.ocs = &matched;
		rs->sr_err = ndb_entry_get_info( op, &NA2, 1, NULL );
		NA2.ocs = NULL;
		switch( rs->sr_err ) {
#if 0
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
#endif
		case LDAP_NO_SUCH_OBJECT:
			break;
		case 0:
			rs->sr_err = LDAP_ALREADY_EXISTS;
			goto return_results;
		default:
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
			goto return_results;
		}
	}

	assert( op->orr_modlist != NULL );

	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,        
				"<=- " LDAP_XSTRING(ndb_back_modrdn)
				": pre-read failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}                   
	}

	/* delete old DN */
	rs->sr_err = ndb_entry_del_info( op->o_bd, &NA );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_modrdn)
			": dn2id del failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
#if 0
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
#endif
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN index delete fail";
		goto return_results;
	}

	/* copy entry fields */
	e2.e_attrs = e.e_attrs;
	e2.e_id = e.e_id;

	/* add new DN */
	rs->sr_err = ndb_entry_put_info( op->o_bd, &NA2, 0 );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_modrdn)
			": dn2id add failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
#if 0
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
#endif
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "DN index add failed";
		goto return_results;
	}

	/* modify entry */
	rs->sr_err = ndb_modify_internal( op, &NA2,
		&rs->sr_text, textbuf, textlen );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_modrdn)
			": modify failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
#if 0
		switch( rs->sr_err ) {
		case DB_LOCK_DEADLOCK:
		case DB_LOCK_NOTGRANTED:
			goto retry;
		}
#endif
		goto return_results;
	}

	e.e_attrs = e2.e_attrs;

	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &e2,
			&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,        
				"<=- " LDAP_XSTRING(ndb_back_modrdn)
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
			LDAP_XSTRING(ndb_back_modrdn) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code );
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}
	NA.txn->close();
	NA.txn = NULL;

	Debug(LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_modrdn)
		": rdn modified%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		e.e_id, op->o_req_dn.bv_val );

	rs->sr_err = LDAP_SUCCESS;
	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	if ( NA2.ocs ) {
		ber_bvarray_free_x( NA2.ocs, op->o_tmpmemctx );
		NA2.ocs = NULL;
	}

	if ( NA.ocs ) {
		ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		NA.ocs = NULL;
	}

	if ( e.e_attrs ) {
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
	}

	if( NA.txn != NULL ) {
		NA.txn->execute( Rollback );
		NA.txn->close();
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if( new_dn.bv_val != NULL ) free( new_dn.bv_val );
	if( new_ndn.bv_val != NULL ) free( new_ndn.bv_val );

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
