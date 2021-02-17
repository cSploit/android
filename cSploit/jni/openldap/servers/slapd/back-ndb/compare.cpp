/* compare.cpp - ndb backend compare routine */
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
ndb_back_compare( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Entry		e = {0};
	Attribute	*a;
	int		manageDSAit = get_manageDSAit( op );

	NdbArgs NA;
	NdbRdns rdns;
	struct berval matched;

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );

	rdns.nr_num = 0;
	NA.rdns = &rdns;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	NA.e = &e;

dn2entry_retry:
	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_compare) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	NA.ocs = NULL;
	/* get entry */
	rs->sr_err = ndb_entry_get_info( op, &NA, 0, &matched );
	switch( rs->sr_err ) {
	case 0:
		break;
	case LDAP_NO_SUCH_OBJECT:
		rs->sr_matched = matched.bv_val;
		if ( NA.ocs )
			ndb_check_referral( op, rs, &NA );
		goto return_results;
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto return_results;
#if 0
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto dn2entry_retry;
#endif
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	rs->sr_err = ndb_entry_get_data( op, &NA, 0 );
	ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
	if (!manageDSAit && is_entry_referral( &e ) ) {
		/* return referral only if "disclose" is granted on the object */
		if ( !access_allowed( op, &e, slap_schema.si_ad_entry,
			NULL, ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			/* entry is a referral, don't allow compare */
			rs->sr_ref = get_entry_referrals( op, &e );
			rs->sr_err = LDAP_REFERRAL;
			rs->sr_matched = e.e_name.bv_val;
			rs->sr_flags |= REP_REF_MUSTBEFREED;
		}

		Debug( LDAP_DEBUG_TRACE, "entry is referral\n", 0, 0, 0 );
		goto return_results;
	}

	if ( get_assert( op ) &&
		( test_filter( op, &e, (Filter *)get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		if ( !access_allowed( op, &e, slap_schema.si_ad_entry,
			NULL, ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_ASSERTION_FAILED;
		}
		goto return_results;
	}

	if ( !access_allowed( op, &e, op->oq_compare.rs_ava->aa_desc,
		&op->oq_compare.rs_ava->aa_value, ACL_COMPARE, NULL ) )
	{
		/* return error only if "disclose"
		 * is granted on the object */
		if ( !access_allowed( op, &e, slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		}
		goto return_results;
	}

	rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;

	for ( a = attrs_find( e.e_attrs, op->oq_compare.rs_ava->aa_desc );
		a != NULL;
		a = attrs_find( a->a_next, op->oq_compare.rs_ava->aa_desc ) )
	{
		rs->sr_err = LDAP_COMPARE_FALSE;

		if ( attr_valfind( a,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
			&op->oq_compare.rs_ava->aa_value, NULL,
			op->o_tmpmemctx ) == 0 )
		{
			rs->sr_err = LDAP_COMPARE_TRUE;
			break;
		}
	}

return_results:
	NA.txn->close();
	if ( e.e_attrs ) {
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
	}
	send_ldap_result( op, rs );

	switch ( rs->sr_err ) {
	case LDAP_COMPARE_FALSE:
	case LDAP_COMPARE_TRUE:
		rs->sr_err = LDAP_SUCCESS;
		break;
	}

	return rs->sr_err;
}
