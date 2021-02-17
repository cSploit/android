/* delete.cpp - ndb backend delete routine */
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

#include "lutil.h"
#include "back-ndb.h"

static struct berval glue_bv = BER_BVC("glue");

int
ndb_back_delete( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Entry	e = {0};
	Entry	p = {0};
	int		manageDSAit = get_manageDSAit( op );
	AttributeDescription *children = slap_schema.si_ad_children;
	AttributeDescription *entry = slap_schema.si_ad_entry;

	NdbArgs NA;
	NdbRdns rdns;
	struct berval matched;

	int	num_retries = 0;

	int     rc;

	LDAPControl **preread_ctrl = NULL;
	LDAPControl *ctrls[SLAP_MAX_RESPONSE_CONTROLS];
	int num_ctrls = 0;

	Debug( LDAP_DEBUG_ARGS, "==> " LDAP_XSTRING(ndb_back_delete) ": %s\n",
		op->o_req_dn.bv_val, 0, 0 );

	ctrls[num_ctrls] = 0;

	/* allocate CSN */
	if ( BER_BVISNULL( &op->o_csn ) ) {
		struct berval csn;
		char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof(csnbuf);
		slap_get_csn( op, &csn, 1 );
	}

	if ( !be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		dnParent( &op->o_req_dn, &p.e_name );
		dnParent( &op->o_req_ndn, &p.e_nname );
	}

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );
	rdns.nr_num = 0;
	NA.rdns = &rdns;
	NA.ocs = NULL;
	NA.e = &e;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;

	if( 0 ) {
retry:	/* transaction retry */
		NA.txn->close();
		NA.txn = NULL;
		Debug( LDAP_DEBUG_TRACE,
			"==> " LDAP_XSTRING(ndb_back_delete) ": retrying...\n",
			0, 0, 0 );
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto return_results;
		}
		if ( NA.ocs ) {
			ber_bvarray_free( NA.ocs );
			NA.ocs = NULL;
		}
		ndb_trans_backoff( ++num_retries );
	}

	/* begin transaction */
	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_delete) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	/* get entry */
	rs->sr_err = ndb_entry_get_info( op, &NA, 1, &matched );
	switch( rs->sr_err ) {
	case 0:
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

	if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ||
		( !manageDSAit && bvmatch( NA.ocs, &glue_bv ))) {
		Debug( LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(ndb_back_delete) ": no such object %s\n",
			op->o_req_dn.bv_val, 0, 0);

		if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
			rs->sr_matched = matched.bv_val;
			if ( NA.ocs )
				ndb_check_referral( op, rs, &NA );
		} else {
			rs->sr_matched = p.e_name.bv_val;
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		}
		goto return_results;
	}

	/* check parent for "children" acl */
	rs->sr_err = access_allowed( op, &p,
		children, NULL, ACL_WDEL, NULL );

	if ( !rs->sr_err  ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_delete) ": no write "
			"access to parent\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to parent";
		goto return_results;
	}

	rs->sr_err = ndb_entry_get_data( op, &NA, 1 );

	rs->sr_err = access_allowed( op, &e,
		entry, NULL, ACL_WDEL, NULL );

	if ( !rs->sr_err  ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_delete) ": no write access "
			"to entry\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to entry";
		goto return_results;
	}

	if ( !manageDSAit && is_entry_referral( &e ) ) {
		/* entry is a referral, don't allow delete */
		rs->sr_ref = get_entry_referrals( op, &e );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_delete) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_err = LDAP_REFERRAL;
		rs->sr_matched = e.e_name.bv_val;
		rs->sr_flags = REP_REF_MUSTBEFREED;
		goto return_results;
	}

	if ( get_assert( op ) &&
		( test_filter( op, &e, (Filter *)get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	/* pre-read */
	if( op->o_preread ) {
		if( preread_ctrl == NULL ) {
			preread_ctrl = &ctrls[num_ctrls++];
			ctrls[num_ctrls] = NULL;
		}
		if( slap_read_controls( op, rs, &e,
			&slap_pre_read_bv, preread_ctrl ) )
		{
			Debug( LDAP_DEBUG_TRACE,
				"<=- " LDAP_XSTRING(ndb_back_delete) ": pre-read "
				"failed!\n", 0, 0, 0 );
			if ( op->o_preread & SLAP_CONTROL_CRITICAL ) {
				/* FIXME: is it correct to abort
				 * operation if control fails? */
				goto return_results;
			}
		}
	}

	/* Can't do it if we have kids */
	rs->sr_err = ndb_has_children( &NA, &rc );
	if ( rs->sr_err ) {
		Debug(LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(ndb_back_delete)
			": has_children failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}
	if ( rc == LDAP_COMPARE_TRUE ) {
		Debug(LDAP_DEBUG_ARGS,
			"<=- " LDAP_XSTRING(ndb_back_delete)
			": non-leaf %s\n",
			op->o_req_dn.bv_val, 0, 0);
		rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
		rs->sr_text = "subordinate objects must be deleted first";
		goto return_results;
	}

	/* delete info */
	rs->sr_err = ndb_entry_del_info( op->o_bd, &NA );
	if ( rs->sr_err != 0 ) {
		Debug(LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_delete) ": del_info failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
		rs->sr_text = "DN index delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	/* delete data */
	rs->sr_err = ndb_entry_del_data( op->o_bd, &NA );
	if ( rs->sr_err != 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"<=- " LDAP_XSTRING(ndb_back_delete) ": del_data failed: %s (%d)\n",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code, 0 );
		rs->sr_text = "entry delete failed";
		rs->sr_err = LDAP_OTHER;
		goto return_results;
	}

	if( op->o_noop ) {
		if (( rs->sr_err=NA.txn->execute( NdbTransaction::Rollback,
			NdbOperation::AbortOnError, 1 )) != 0 ) {
			rs->sr_text = "txn (no-op) failed";
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
			LDAP_XSTRING(ndb_back_delete) ": txn_%s failed: %s (%d)\n",
			op->o_noop ? "abort (no-op)" : "commit",
			NA.txn->getNdbError().message, NA.txn->getNdbError().code );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "commit failed";

		goto return_results;
	}
	NA.txn->close();
	NA.txn = NULL;

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_delete) ": deleted%s id=%08lx dn=\"%s\"\n",
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

	/* free entry */
	if( e.e_attrs != NULL ) {
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
	return rs->sr_err;
}
