/* add.cpp - ldap NDB back-end add routine */
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

extern "C" int
ndb_back_add(Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Entry		p = {0};
	Attribute	poc;
	char textbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof textbuf;
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	NdbArgs NA;
	NdbRdns rdns;
	struct berval matched;
	struct berval pdn, pndn;

	int		num_retries = 0;
	int		success;

	LDAPControl **postread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	Debug(LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(ndb_back_add) ": %s\n",
		op->oq_add.rs_e->e_name.bv_val, 0, 0);

	ctrls[num_ctrls] = 0;
	NA.txn = NULL;

	/* check entry's schema */
	rs->sr_err = entry_schema_check( op, op->oq_add.rs_e, NULL,
		get_relax(op), 1, NULL, &rs->sr_text, textbuf, textlen );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": entry failed schema check: "
			"%s (%d)\n", rs->sr_text, rs->sr_err, 0 );
		goto return_results;
	}

	/* add opattrs to shadow as well, only missing attrs will actually
	 * be added; helps compatibility with older OL versions */
	rs->sr_err = slap_add_opattrs( op, &rs->sr_text, textbuf, textlen, 1 );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": entry failed op attrs add: "
			"%s (%d)\n", rs->sr_text, rs->sr_err, 0 );
		goto return_results;
	}

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );

	/*
	 * Get the parent dn and see if the corresponding entry exists.
	 */
	if ( be_issuffix( op->o_bd, &op->oq_add.rs_e->e_nname ) ) {
		pdn = slap_empty_bv;
		pndn = slap_empty_bv;
	} else {
		dnParent( &op->ora_e->e_name, &pdn );
		dnParent( &op->ora_e->e_nname, &pndn );
	}
	p.e_name = op->ora_e->e_name;
	p.e_nname = op->ora_e->e_nname;

	op->ora_e->e_id = NOID;
	rdns.nr_num = 0;
	NA.rdns = &rdns;

	if( 0 ) {
retry:	/* transaction retry */
		NA.txn->close();
		NA.txn = NULL;
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto return_results;
		}
		ndb_trans_backoff( ++num_retries );
	}

	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* get entry or parent */
	NA.e = &p;
	NA.ocs = NULL;
	rs->sr_err = ndb_entry_get_info( op, &NA, 0, &matched );
	switch( rs->sr_err ) {
	case 0:
		rs->sr_err = LDAP_ALREADY_EXISTS;
		goto return_results;
	case LDAP_NO_SUCH_OBJECT:
		break;
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

	if ( NA.ocs ) {
		int i;
		for ( i=0; !BER_BVISNULL( &NA.ocs[i] ); i++ );
		poc.a_numvals = i;
		poc.a_desc = slap_schema.si_ad_objectClass;
		poc.a_vals = NA.ocs;
		poc.a_nvals = poc.a_vals;
		poc.a_next = NULL;
		p.e_attrs = &poc;
	}

	if ( ber_bvstrcasecmp( &pndn, &matched ) ) {
		rs->sr_matched = matched.bv_val;
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": parent "
			"does not exist\n", 0, 0, 0 );

		rs->sr_text = "parent does not exist";
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		if ( p.e_attrs && is_entry_referral( &p )) {
is_ref:			p.e_attrs = NULL;
			ndb_entry_get_data( op, &NA, 0 );
			rs->sr_ref = get_entry_referrals( op, &p );
			rs->sr_err = LDAP_REFERRAL;
			rs->sr_flags = REP_REF_MUSTBEFREED;
			attrs_free( p.e_attrs );
			p.e_attrs = NULL;
		}
		goto return_results;
	}

	p.e_name = pdn;
	p.e_nname = pndn;
	rs->sr_err = access_allowed( op, &p,
		children, NULL, ACL_WADD, NULL );

	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": no write access to parent\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to parent";
		goto return_results;
	}

	if ( NA.ocs ) {
		if ( is_entry_subentry( &p )) {
			/* parent is a subentry, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(ndb_back_add) ": parent is subentry\n",
				0, 0, 0 );
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			rs->sr_text = "parent is a subentry";
			goto return_results;
		}

		if ( is_entry_alias( &p ) ) {
			/* parent is an alias, don't allow add */
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(ndb_back_add) ": parent is alias\n",
				0, 0, 0 );
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "parent is an alias";
			goto return_results;
		}

		if ( is_entry_referral( &p ) ) {
			/* parent is a referral, don't allow add */
			rs->sr_matched = p.e_name.bv_val;
			goto is_ref;
		}
	}

	rs->sr_err = access_allowed( op, op->ora_e,
		entry, NULL, ACL_WADD, NULL );

	if ( ! rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": no write access to entry\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;;
	}

	/* 
	 * Check ACL for attribute write access
	 */
	if (!acl_check_modlist(op, op->ora_e, op->ora_modlist)) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_add) ": no write access to attribute\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to attribute";
		goto return_results;;
	}


	/* acquire entry ID */
	if ( op->ora_e->e_id == NOID ) {
		rs->sr_err = ndb_next_id( op->o_bd, NA.ndb, &op->ora_e->e_id );
		if( rs->sr_err != 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(ndb_back_add) ": next_id failed (%d)\n",
				rs->sr_err, 0, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "internal error";
			goto return_results;
		}
	}

	if ( matched.bv_val )
		rdns.nr_num++;
	NA.e = op->ora_e;
	/* dn2id index */
	rs->sr_err = ndb_entry_put_info( op->o_bd, &NA, 0 );
	if ( rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": ndb_entry_put_info failed (%d)\n",
			rs->sr_err, 0, 0 );
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* id2entry index */
	rs->sr_err = ndb_entry_put_data( op->o_bd, &NA );
	if ( rs->sr_err ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": ndb_entry_put_data failed (%d) %s(%d)\n",
			rs->sr_err, NA.txn->getNdbError().message, NA.txn->getNdbError().code );
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* post-read */
	if( op->o_postread ) {
		if( postread_ctrl == NULL ) {
			postread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if ( slap_read_controls( op, rs, op->oq_add.rs_e,
			&slap_post_read_bv, postread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(ndb_back_add) ": post-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_postread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	if ( op->o_noop ) {
		if (( rs->sr_err=NA.txn->execute( NdbTransaction::Rollback,
			NdbOperation::AbortOnError, 1 )) != 0 ) {
			rs->sr_text = "txn (no-op) failed";
		} else {
			rs->sr_err = LDAP_X_NO_OPERATION;
		}

	} else {
		if(( rs->sr_err=NA.txn->execute( NdbTransaction::Commit,
			NdbOperation::AbortOnError, 1 )) != 0 ) {
			rs->sr_text = "txn_commit failed";
		} else {
			rs->sr_err = LDAP_SUCCESS;
		}
	}

	if ( rs->sr_err != LDAP_SUCCESS && rs->sr_err != LDAP_X_NO_OPERATION ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_add) ": %s : %s (%d)\n",
			rs->sr_text, NA.txn->getNdbError().message, NA.txn->getNdbError().code );
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}
	NA.txn->close();
	NA.txn = NULL;

	Debug(LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_add) ": added%s id=%08lx dn=\"%s\"\n",
		op->o_noop ? " (no-op)" : "",
		op->oq_add.rs_e->e_id, op->oq_add.rs_e->e_dn );

	rs->sr_text = NULL;
	if( num_ctrls ) rs->sr_ctrls = ctrls;

return_results:
	success = rs->sr_err;
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if( NA.txn != NULL ) {
		NA.txn->execute( Rollback );
		NA.txn->close();
	}

	if( postread_ctrl != NULL && (*postread_ctrl) != NULL ) {
		slap_sl_free( (*postread_ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
		slap_sl_free( *postread_ctrl, op->o_tmpmemctx );
	}

	return rs->sr_err;
}
