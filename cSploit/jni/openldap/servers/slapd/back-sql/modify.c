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
#include "ac/string.h"

#include "slap.h"
#include "proto-sql.h"

int
backsql_modify( Operation *op, SlapReply *rs )
{
	backsql_info		*bi = (backsql_info*)op->o_bd->be_private;
	SQLHDBC 		dbh = SQL_NULL_HDBC;
	backsql_oc_map_rec	*oc = NULL;
	backsql_srch_info	bsi = { 0 };
	Entry			m = { 0 }, *e = NULL;
	int			manageDSAit = get_manageDSAit( op );
	SQLUSMALLINT		CompletionType = SQL_ROLLBACK;

	/*
	 * FIXME: in case part of the operation cannot be performed
	 * (missing mapping, SQL write fails or so) the entire operation
	 * should be rolled-back
	 */
	Debug( LDAP_DEBUG_TRACE, "==>backsql_modify(): modifying entry \"%s\"\n",
		op->o_req_ndn.bv_val, 0, 0 );

	rs->sr_err = backsql_get_db_conn( op, &dbh );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modify(): "
			"could not get connection handle - exiting\n", 
			0, 0, 0 );
		/*
		 * FIXME: we don't want to send back 
		 * excessively detailed messages
		 */
		rs->sr_text = ( rs->sr_err == LDAP_OTHER )
			? "SQL-backend error" : NULL;
		goto done;
	}

	bsi.bsi_e = &m;
	rs->sr_err = backsql_init_search( &bsi, &op->o_req_ndn,
			LDAP_SCOPE_BASE, 
			(time_t)(-1), NULL, dbh, op, rs,
			slap_anlist_all_attributes,
			( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY | BACKSQL_ISF_GET_OC ) );
	switch ( rs->sr_err ) {
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
		e = &m;
		/* fallthru */

	default:
		Debug( LDAP_DEBUG_TRACE, "backsql_modify(): "
			"could not retrieve modifyDN ID - no such entry\n", 
			0, 0, 0 );
		if ( !BER_BVISNULL( &m.e_nname ) ) {
			/* FIXME: should always be true! */
			e = &m;

		} else {
			e = NULL;
		}
		goto done;
	}

	Debug( LDAP_DEBUG_TRACE, "   backsql_modify(): "
		"modifying entry \"%s\" (id=" BACKSQL_IDFMT ")\n", 
		bsi.bsi_base_id.eid_dn.bv_val,
		BACKSQL_IDARG(bsi.bsi_base_id.eid_id), 0 );

	if ( get_assert( op ) &&
			( test_filter( op, &m, get_assertion( op ) )
			  != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		e = &m;
		goto done;
	}

	slap_mods_opattrs( op, &op->orm_modlist, 1 );

	assert( bsi.bsi_base_id.eid_oc != NULL );
	oc = bsi.bsi_base_id.eid_oc;

	if ( !acl_check_modlist( op, &m, op->orm_modlist ) ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		e = &m;
		goto done;
	}

	rs->sr_err = backsql_modify_internal( op, rs, dbh, oc,
			&bsi.bsi_base_id, op->orm_modlist );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		e = &m;
		goto do_transact;
	}

	if ( BACKSQL_CHECK_SCHEMA( bi ) ) {
		char		textbuf[ SLAP_TEXT_BUFLEN ] = { '\0' };

		backsql_entry_clean( op, &m );

		bsi.bsi_e = &m;
		rs->sr_err = backsql_id2entry( &bsi, &bsi.bsi_base_id );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			e = &m;
			goto do_transact;
		}

		rs->sr_err = entry_schema_check( op, &m, NULL, 0, 0, NULL,
			&rs->sr_text, textbuf, sizeof( textbuf ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modify(\"%s\"): "
				"entry failed schema check -- aborting\n",
				m.e_name.bv_val, 0, 0 );
			e = NULL;
			goto do_transact;
		}
	}

do_transact:;
	/*
	 * Commit only if all operations succeed
	 */
	if ( rs->sr_err == LDAP_SUCCESS && !op->o_noop ) {
		assert( e == NULL );
		CompletionType = SQL_COMMIT;
	}

	SQLTransact( SQL_NULL_HENV, dbh, CompletionType );

done:;
	if ( e != NULL ) {
		if ( !access_allowed( op, e, slap_schema.si_ad_entry, NULL,
					ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			rs->sr_text = NULL;
			rs->sr_matched = NULL;
			if ( rs->sr_ref ) {
				ber_bvarray_free( rs->sr_ref );
				rs->sr_ref = NULL;
			}
		}
	}

	if ( op->o_noop && rs->sr_err == LDAP_SUCCESS ) {
		rs->sr_err = LDAP_X_NO_OPERATION;
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if ( !BER_BVISNULL( &bsi.bsi_base_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &m.e_nname ) ) {
		backsql_entry_clean( op, &m );
	}

	if ( bsi.bsi_attrs != NULL ) {
		op->o_tmpfree( bsi.bsi_attrs, op->o_tmpmemctx );
	}

	if ( rs->sr_ref ) {
		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_modify()\n", 0, 0, 0 );

	return rs->sr_err;
}

