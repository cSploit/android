/* compare.c - mdb backend compare routine */
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
mdb_compare( Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	Entry		*e = NULL;
	int		manageDSAit = get_manageDSAit( op );

	MDB_txn		*rtxn;
	mdb_op_info	opinfo = {{{0}}}, *moi = &opinfo;

	rs->sr_err = mdb_opinfo_get(op, mdb, 1, &moi);
	switch(rs->sr_err) {
	case 0:
		break;
	default:
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

	rtxn = moi->moi_txn;

	/* get entry */
	rs->sr_err = mdb_dn2entry( op, rtxn, NULL, &op->o_req_ndn, &e, NULL, 1 );
	switch( rs->sr_err ) {
	case MDB_NOTFOUND:
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

	if ( rs->sr_err == MDB_NOTFOUND ) {
		if ( e != NULL ) {
			/* return referral only if "disclose" is granted on the object */
			if ( ! access_allowed( op, e, slap_schema.si_ad_entry,
				NULL, ACL_DISCLOSE, NULL ) )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;

			} else {
				rs->sr_matched = ch_strdup( e->e_dn );
				if ( is_entry_referral( e )) {
					BerVarray ref = get_entry_referrals( op, e );
					rs->sr_ref = referral_rewrite( ref, &e->e_name,
						&op->o_req_dn, LDAP_SCOPE_DEFAULT );
					ber_bvarray_free( ref );
				} else {
					rs->sr_ref = NULL;
				}
				rs->sr_err = LDAP_REFERRAL;
			}
			mdb_entry_return( op, e );
			e = NULL;

		} else {
			rs->sr_ref = referral_rewrite( default_referral,
				NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
			rs->sr_err = rs->sr_ref ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
		}

		rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
		send_ldap_result( op, rs );
		goto done;
	}

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* return referral only if "disclose" is granted on the object */
		if ( !access_allowed( op, e, slap_schema.si_ad_entry,
			NULL, ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			/* entry is a referral, don't allow compare */
			rs->sr_ref = get_entry_referrals( op, e );
			rs->sr_err = LDAP_REFERRAL;
			rs->sr_matched = e->e_name.bv_val;
		}

		Debug( LDAP_DEBUG_TRACE, "entry is referral\n", 0, 0, 0 );

		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;
		goto done;
	}

	rs->sr_err = slap_compare_entry( op, e, op->orc_ava );

return_results:
	send_ldap_result( op, rs );

	switch ( rs->sr_err ) {
	case LDAP_COMPARE_FALSE:
	case LDAP_COMPARE_TRUE:
		rs->sr_err = LDAP_SUCCESS;
		break;
	}

done:
	if ( moi == &opinfo ) {
		mdb_txn_reset( moi->moi_txn );
		LDAP_SLIST_REMOVE( &op->o_extra, &moi->moi_oe, OpExtra, oe_next );
	} else {
		moi->moi_ref--;
	}
	/* free entry */
	if ( e != NULL ) {
		mdb_entry_return( op, e );
	}

	return rs->sr_err;
}
