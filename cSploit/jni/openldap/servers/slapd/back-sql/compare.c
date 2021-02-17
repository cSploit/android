/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
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
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include <sys/types.h>

#include "slap.h"
#include "proto-sql.h"

int
backsql_compare( Operation *op, SlapReply *rs )
{
	SQLHDBC			dbh = SQL_NULL_HDBC;
	Entry			e = { 0 };
	Attribute		*a = NULL;
	backsql_srch_info	bsi = { 0 };
	int			rc;
	int			manageDSAit = get_manageDSAit( op );
	AttributeName		anlist[2];

 	Debug( LDAP_DEBUG_TRACE, "==>backsql_compare()\n", 0, 0, 0 );

	rs->sr_err = backsql_get_db_conn( op, &dbh );
	if ( rs->sr_err != LDAP_SUCCESS ) {
     		Debug( LDAP_DEBUG_TRACE, "backsql_compare(): "
			"could not get connection handle - exiting\n",
			0, 0, 0 );

		rs->sr_text = ( rs->sr_err == LDAP_OTHER )
			? "SQL-backend error" : NULL;
		goto return_results;
	}

	anlist[ 0 ].an_name = op->oq_compare.rs_ava->aa_desc->ad_cname;
	anlist[ 0 ].an_desc = op->oq_compare.rs_ava->aa_desc;
	BER_BVZERO( &anlist[ 1 ].an_name );

	/*
	 * Get the entry
	 */
	bsi.bsi_e = &e;
	rc = backsql_init_search( &bsi, &op->o_req_ndn, LDAP_SCOPE_BASE,
			(time_t)(-1), NULL, dbh, op, rs, anlist,
			( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY ) );
	switch ( rc ) {
	case LDAP_SUCCESS:
		break;

	case LDAP_REFERRAL:
		if ( manageDSAit && !BER_BVISNULL( &bsi.bsi_e->e_nname ) &&
				dn_match( &op->o_req_ndn, &bsi.bsi_e->e_nname ) )
		{
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_text = NULL;
			rs->sr_matched = NULL;
			if ( rs->sr_ref ) {
				ber_bvarray_free( rs->sr_ref );
				rs->sr_ref = NULL;
			}
			break;
		}
		/* fallthru */

	default:
		Debug( LDAP_DEBUG_TRACE, "backsql_compare(): "
			"could not retrieve compareDN ID - no such entry\n", 
			0, 0, 0 );
		goto return_results;
	}

	if ( get_assert( op ) &&
			( test_filter( op, &e, get_assertion( op ) )
			  != LDAP_COMPARE_TRUE ) )
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	if ( is_at_operational( op->oq_compare.rs_ava->aa_desc->ad_type ) ) {
		SlapReply	nrs = { REP_SEARCH };
		Attribute	**ap;

		for ( ap = &e.e_attrs; *ap; ap = &(*ap)->a_next )
			;

		nrs.sr_attrs = anlist;
		nrs.sr_entry = &e;
		nrs.sr_attr_flags = SLAP_OPATTRS_NO;
		nrs.sr_operational_attrs = NULL;

		rs->sr_err = backsql_operational( op, &nrs );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto return_results;
		}
		
		*ap = nrs.sr_operational_attrs;
	}

	if ( ! access_allowed( op, &e, op->oq_compare.rs_ava->aa_desc,
				&op->oq_compare.rs_ava->aa_value,
				ACL_COMPARE, NULL ) )
	{
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
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

return_results:;
	switch ( rs->sr_err ) {
	case LDAP_COMPARE_TRUE:
	case LDAP_COMPARE_FALSE:
		break;

	default:
		if ( !BER_BVISNULL( &e.e_nname ) &&
				! access_allowed( op, &e,
					slap_schema.si_ad_entry, NULL,
					ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			rs->sr_text = NULL;
		}
		break;
	}

	send_ldap_result( op, rs );

	if ( rs->sr_matched ) {
		rs->sr_matched = NULL;
	}

	if ( rs->sr_ref ) {
		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
	}

	if ( !BER_BVISNULL( &bsi.bsi_base_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &e.e_nname ) ) {
		backsql_entry_clean( op, &e );
	}

	if ( bsi.bsi_attrs != NULL ) {
		op->o_tmpfree( bsi.bsi_attrs, op->o_tmpmemctx );
	}

	Debug(LDAP_DEBUG_TRACE,"<==backsql_compare()\n",0,0,0);
	switch ( rs->sr_err ) {
	case LDAP_COMPARE_TRUE:
	case LDAP_COMPARE_FALSE:
		return LDAP_SUCCESS;

	default:
		return rs->sr_err;
	}
}
 
