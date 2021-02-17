/* compare.c - bdb backend compare routine */
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
bdb_compare( Operation *op, SlapReply *rs )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	Entry		*e = NULL;
	EntryInfo	*ei;
	int		manageDSAit = get_manageDSAit( op );

	DB_TXN		*rtxn;
	DB_LOCK		lock;

	rs->sr_err = bdb_reader_get(op, bdb->bi_dbenv, &rtxn);
	switch(rs->sr_err) {
	case 0:
		break;
	default:
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

dn2entry_retry:
	/* get entry */
	rs->sr_err = bdb_dn2entry( op, rtxn, &op->o_req_ndn, &ei, 1,
		&lock );

	switch( rs->sr_err ) {
	case DB_NOTFOUND:
	case 0:
		break;
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto return_results;
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto dn2entry_retry;
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto return_results;
	}

	e = ei->bei_e;
	if ( rs->sr_err == DB_NOTFOUND ) {
		if ( e != NULL ) {
			/* return referral only if "disclose" is granted on the object */
			if ( ! access_allowed( op, e, slap_schema.si_ad_entry,
				NULL, ACL_DISCLOSE, NULL ) )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;

			} else {
				rs->sr_matched = ch_strdup( e->e_dn );
				rs->sr_ref = is_entry_referral( e )
					? get_entry_referrals( op, e )
					: NULL;
				rs->sr_err = LDAP_REFERRAL;
			}

			bdb_cache_return_entry_r( bdb, e, &lock );
			e = NULL;

		} else {
			rs->sr_ref = referral_rewrite( default_referral,
				NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
			rs->sr_err = rs->sr_ref ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
		}

		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		free( (char *)rs->sr_matched );
		rs->sr_ref = NULL;
		rs->sr_matched = NULL;

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
	/* free entry */
	if ( e != NULL ) {
		bdb_cache_return_entry_r( bdb, e, &lock );
	}

	return rs->sr_err;
}
