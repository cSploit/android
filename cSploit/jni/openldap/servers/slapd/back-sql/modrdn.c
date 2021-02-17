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
backsql_modrdn( Operation *op, SlapReply *rs )
{
	backsql_info		*bi = (backsql_info*)op->o_bd->be_private;
	SQLHDBC			dbh = SQL_NULL_HDBC;
	SQLHSTMT		sth = SQL_NULL_HSTMT;
	RETCODE			rc;
	backsql_entryID		e_id = BACKSQL_ENTRYID_INIT,
				n_id = BACKSQL_ENTRYID_INIT;
	backsql_srch_info	bsi = { 0 };
	backsql_oc_map_rec	*oc = NULL;
	struct berval		pdn = BER_BVNULL, pndn = BER_BVNULL,
				*new_pdn = NULL, *new_npdn = NULL,
				new_dn = BER_BVNULL, new_ndn = BER_BVNULL,
				realnew_dn = BER_BVNULL;
	Entry			r = { 0 },
				p = { 0 },
				n = { 0 },
				*e = NULL;
	int			manageDSAit = get_manageDSAit( op );
	struct berval		*newSuperior = op->oq_modrdn.rs_newSup;
 
	Debug( LDAP_DEBUG_TRACE, "==>backsql_modrdn() renaming entry \"%s\", "
			"newrdn=\"%s\", newSuperior=\"%s\"\n",
			op->o_req_dn.bv_val, op->oq_modrdn.rs_newrdn.bv_val, 
			newSuperior ? newSuperior->bv_val : "(NULL)" );

	rs->sr_err = backsql_get_db_conn( op, &dbh );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"could not get connection handle - exiting\n", 
			0, 0, 0 );
		rs->sr_text = ( rs->sr_err == LDAP_OTHER )
			?  "SQL-backend error" : NULL;
		e = NULL;
		goto done;
	}

	bsi.bsi_e = &r;
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
		e = &r;
		/* fallthru */

	default:
		Debug( LDAP_DEBUG_TRACE, "backsql_modrdn(): "
			"could not retrieve modrdnDN ID - no such entry\n", 
			0, 0, 0 );
		if ( !BER_BVISNULL( &r.e_nname ) ) {
			/* FIXME: should always be true! */
			e = &r;

		} else {
			e = NULL;
		}
		goto done;
	}

	Debug( LDAP_DEBUG_TRACE,
		"   backsql_modrdn(): entry id=" BACKSQL_IDFMT "\n",
		BACKSQL_IDARG(e_id.eid_id), 0, 0 );

	if ( get_assert( op ) &&
			( test_filter( op, &r, get_assertion( op ) )
			  != LDAP_COMPARE_TRUE ) )
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		e = &r;
		goto done;
	}

	if ( backsql_has_children( op, dbh, &op->o_req_ndn ) == LDAP_COMPARE_TRUE ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"entry \"%s\" has children\n",
			op->o_req_dn.bv_val, 0, 0 );
		rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
		rs->sr_text = "subtree rename not supported";
		e = &r;
		goto done;
	}

	/*
	 * Check for entry access to target
	 */
	if ( !access_allowed( op, &r, slap_schema.si_ad_entry, 
				NULL, ACL_WRITE, NULL ) ) {
		Debug( LDAP_DEBUG_TRACE, "   no access to entry\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto done;
	}

	dnParent( &op->o_req_dn, &pdn );
	dnParent( &op->o_req_ndn, &pndn );

	/*
	 * namingContext "" is not supported
	 */
	if ( BER_BVISEMPTY( &pdn ) ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"parent is \"\" - aborting\n", 0, 0, 0 );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "not allowed within namingContext";
		e = NULL;
		goto done;
	}

	/*
	 * Check for children access to parent
	 */
	bsi.bsi_e = &p;
	e_id = bsi.bsi_base_id;
	memset( &bsi.bsi_base_id, 0, sizeof( bsi.bsi_base_id ) );
	rs->sr_err = backsql_init_search( &bsi, &pndn,
			LDAP_SCOPE_BASE, 
			(time_t)(-1), NULL, dbh, op, rs,
			slap_anlist_all_attributes,
			BACKSQL_ISF_GET_ENTRY );

	Debug( LDAP_DEBUG_TRACE,
		"   backsql_modrdn(): old parent entry id is " BACKSQL_IDFMT "\n",
		BACKSQL_IDARG(bsi.bsi_base_id.eid_id), 0, 0 );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_modrdn(): "
			"could not retrieve renameDN ID - no such entry\n", 
			0, 0, 0 );
		e = &p;
		goto done;
	}

	if ( !access_allowed( op, &p, slap_schema.si_ad_children, NULL,
			newSuperior ? ACL_WDEL : ACL_WRITE, NULL ) )
	{
		Debug( LDAP_DEBUG_TRACE, "   no access to parent\n", 0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto done;
	}

	if ( newSuperior ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
		
		/*
		 * namingContext "" is not supported
		 */
		if ( BER_BVISEMPTY( newSuperior ) ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
				"newSuperior is \"\" - aborting\n", 0, 0, 0 );
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "not allowed within namingContext";
			e = NULL;
			goto done;
		}

		new_pdn = newSuperior;
		new_npdn = op->oq_modrdn.rs_nnewSup;

		/*
		 * Check for children access to new parent
		 */
		bsi.bsi_e = &n;
		rs->sr_err = backsql_init_search( &bsi, new_npdn,
				LDAP_SCOPE_BASE, 
				(time_t)(-1), NULL, dbh, op, rs,
				slap_anlist_all_attributes,
				( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_modrdn(): "
				"could not retrieve renameDN ID - no such entry\n", 
				0, 0, 0 );
			e = &n;
			goto done;
		}

		n_id = bsi.bsi_base_id;

		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): new parent entry id=" BACKSQL_IDFMT "\n",
			BACKSQL_IDARG(n_id.eid_id), 0, 0 );

		if ( !access_allowed( op, &n, slap_schema.si_ad_children, 
					NULL, ACL_WADD, NULL ) ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
					"no access to new parent \"%s\"\n", 
					new_pdn->bv_val, 0, 0 );
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			e = &n;
			goto done;
		}

	} else {
		n_id = bsi.bsi_base_id;
		new_pdn = &pdn;
		new_npdn = &pndn;
	}

	memset( &bsi.bsi_base_id, 0, sizeof( bsi.bsi_base_id ) );

	if ( newSuperior && dn_match( &pndn, new_npdn ) ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"newSuperior is equal to old parent - ignored\n",
			0, 0, 0 );
		newSuperior = NULL;
	}

	if ( newSuperior && dn_match( &op->o_req_ndn, new_npdn ) ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"newSuperior is equal to entry being moved "
			"- aborting\n", 0, 0, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "newSuperior is equal to old DN";
		e = &r;
		goto done;
	}

	build_new_dn( &new_dn, new_pdn, &op->oq_modrdn.rs_newrdn,
			op->o_tmpmemctx );
	build_new_dn( &new_ndn, new_npdn, &op->oq_modrdn.rs_nnewrdn,
			op->o_tmpmemctx );
	
	Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): new entry dn is \"%s\"\n",
			new_dn.bv_val, 0, 0 );

	realnew_dn = new_dn;
	if ( backsql_api_dn2odbc( op, rs, &realnew_dn ) ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(\"%s\"): "
			"backsql_api_dn2odbc(\"%s\") failed\n", 
			op->o_req_dn.bv_val, realnew_dn.bv_val, 0 );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
		"executing renentry_stmt\n", 0, 0, 0 );

	rc = backsql_Prepare( dbh, &sth, bi->sql_renentry_stmt, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): "
			"error preparing renentry_stmt\n", 0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
				sth, rc );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamBerVal( sth, 1, SQL_PARAM_INPUT, &realnew_dn );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): "
			"error binding DN parameter for objectClass %s\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamID( sth, 2, SQL_PARAM_INPUT, &n_id.eid_id );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): "
			"error binding parent ID parameter for objectClass %s\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamID( sth, 3, SQL_PARAM_INPUT, &e_id.eid_keyval );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): "
			"error binding entry ID parameter for objectClass %s\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamID( sth, 4, SQL_PARAM_INPUT, &e_id.eid_id );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modrdn(): "
			"error binding ID parameter for objectClass %s\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = SQLExecute( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(): "
			"could not rename ldap_entries record\n", 0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}
	SQLFreeStmt( sth, SQL_DROP );

	assert( op->orr_modlist != NULL );

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	assert( e_id.eid_oc != NULL );
	oc = e_id.eid_oc;
	rs->sr_err = backsql_modify_internal( op, rs, dbh, oc, &e_id, op->orr_modlist );
	slap_graduate_commit_csn( op );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		e = &r;
		goto done;
	}

	if ( BACKSQL_CHECK_SCHEMA( bi ) ) {
		char		textbuf[ SLAP_TEXT_BUFLEN ] = { '\0' };

		backsql_entry_clean( op, &r );
		(void)backsql_free_entryID( &e_id, 0, op->o_tmpmemctx );

		bsi.bsi_e = &r;
		rs->sr_err = backsql_init_search( &bsi, &new_ndn,
				LDAP_SCOPE_BASE, 
				(time_t)(-1), NULL, dbh, op, rs,
				slap_anlist_all_attributes,
				( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY ) );
		switch ( rs->sr_err ) {
		case LDAP_SUCCESS:
			break;

		case LDAP_REFERRAL:
			if ( manageDSAit && !BER_BVISNULL( &bsi.bsi_e->e_nname ) &&
					dn_match( &new_ndn, &bsi.bsi_e->e_nname ) )
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
			e = &r;
			/* fallthru */

		default:
			Debug( LDAP_DEBUG_TRACE, "backsql_modrdn(): "
				"could not retrieve modrdnDN ID - no such entry\n", 
				0, 0, 0 );
			if ( !BER_BVISNULL( &r.e_nname ) ) {
				/* FIXME: should always be true! */
				e = &r;

			} else {
				e = NULL;
			}
			goto done;
		}

		e_id = bsi.bsi_base_id;

		rs->sr_err = entry_schema_check( op, &r, NULL, 0, 0, NULL,
			&rs->sr_text, textbuf, sizeof( textbuf ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modrdn(\"%s\"): "
				"entry failed schema check -- aborting\n",
				r.e_name.bv_val, 0, 0 );
			e = NULL;
			goto done;
		}
	}

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

	/*
	 * Commit only if all operations succeed
	 */
	if ( sth != SQL_NULL_HSTMT ) {
		SQLUSMALLINT	CompletionType = SQL_ROLLBACK;
	
		if ( rs->sr_err == LDAP_SUCCESS && !op->o_noop ) {
			CompletionType = SQL_COMMIT;
		}

		SQLTransact( SQL_NULL_HENV, dbh, CompletionType );
	}

	if ( op->o_noop && rs->sr_err == LDAP_SUCCESS ) {
		rs->sr_err = LDAP_X_NO_OPERATION;
	}

	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );

	if ( !BER_BVISNULL( &realnew_dn ) && realnew_dn.bv_val != new_dn.bv_val ) {
		ch_free( realnew_dn.bv_val );
	}

	if ( !BER_BVISNULL( &new_dn ) ) {
		slap_sl_free( new_dn.bv_val, op->o_tmpmemctx );
	}
	
	if ( !BER_BVISNULL( &new_ndn ) ) {
		slap_sl_free( new_ndn.bv_val, op->o_tmpmemctx );
	}
	
	if ( !BER_BVISNULL( &e_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &e_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &n_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &n_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &r.e_nname ) ) {
		backsql_entry_clean( op, &r );
	}

	if ( !BER_BVISNULL( &p.e_nname ) ) {
		backsql_entry_clean( op, &p );
	}

	if ( !BER_BVISNULL( &n.e_nname ) ) {
		backsql_entry_clean( op, &n );
	}

	if ( rs->sr_ref ) {
		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_modrdn()\n", 0, 0, 0 );

	return rs->sr_err;
}

