/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
 * Portions Copyright 2004 Mark Adamson.
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
 * Pierangelo Masarati and Mark Adamson.

 */

#include "portable.h"

#include <stdio.h>
#include <sys/types.h>
#include "ac/string.h"

#include "slap.h"
#include "proto-sql.h"

#ifdef BACKSQL_SYNCPROV
#include <lutil.h>
#endif /* BACKSQL_SYNCPROV */

/*
 * Skip:
 * - null values (e.g. delete modification)
 * - single occurrence of objectClass, because it is already used
 *   to determine how to build the SQL entry
 * - operational attributes
 * - empty attributes
 */
#define backsql_opattr_skip(ad) \
	(is_at_operational( (ad)->ad_type ) && (ad) != slap_schema.si_ad_ref )
#define	backsql_attr_skip(ad, vals) \
	( \
		( (ad) == slap_schema.si_ad_objectClass \
				&& (vals) && BER_BVISNULL( &((vals)[ 1 ]) ) ) \
		|| backsql_opattr_skip( (ad) ) \
		|| ( (vals) && BER_BVISNULL( &((vals)[ 0 ]) ) ) \
	)

int
backsql_modify_delete_all_values(
	Operation 		*op,
	SlapReply		*rs,
	SQLHDBC			dbh, 
	backsql_entryID		*e_id,
	backsql_at_map_rec	*at )
{
	backsql_info	*bi = (backsql_info *)op->o_bd->be_private;
	RETCODE		rc;
	SQLHSTMT	asth = SQL_NULL_HSTMT;
	BACKSQL_ROW_NTS	row;

	assert( at != NULL );
	if ( at->bam_delete_proc == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modify_delete_all_values(): "
			"missing attribute value delete procedure "
			"for attr \"%s\"\n",
			at->bam_ad->ad_cname.bv_val, 0, 0 );
		if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
			rs->sr_text = "SQL-backend error";
			return rs->sr_err = LDAP_OTHER;
		}

		return LDAP_SUCCESS;
	}

	rc = backsql_Prepare( dbh, &asth, at->bam_query, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modify_delete_all_values(): "
			"error preparing attribute value select query "
			"\"%s\"\n",
			at->bam_query, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
				asth, rc );

		rs->sr_text = "SQL-backend error";
		return rs->sr_err = LDAP_OTHER;
	}

	rc = backsql_BindParamID( asth, 1, SQL_PARAM_INPUT, &e_id->eid_keyval );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modify_delete_all_values(): "
			"error binding key value parameter "
			"to attribute value select query\n",
			0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
				asth, rc );
		SQLFreeStmt( asth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		return rs->sr_err = LDAP_OTHER;
	}
			
	rc = SQLExecute( asth );
	if ( !BACKSQL_SUCCESS( rc ) ) {
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_modify_delete_all_values(): "
			"error executing attribute value select query\n",
			0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
				asth, rc );
		SQLFreeStmt( asth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		return rs->sr_err = LDAP_OTHER;
	}

	backsql_BindRowAsStrings_x( asth, &row, op->o_tmpmemctx );
	for ( rc = SQLFetch( asth );
			BACKSQL_SUCCESS( rc );
			rc = SQLFetch( asth ) )
	{
		int		i;
		/* first parameter no, parameter order */
		SQLUSMALLINT	pno = 0,
				po = 0;
		/* procedure return code */
		int		prc = LDAP_SUCCESS;
		
		for ( i = 0; i < row.ncols; i++ ) {
			SQLHSTMT	sth = SQL_NULL_HSTMT;
			ber_len_t	col_len;
			
			rc = backsql_Prepare( dbh, &sth, at->bam_delete_proc, 0 );
			if ( rc != SQL_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_delete_all_values(): "
					"error preparing attribute value "
					"delete procedure "
					"\"%s\"\n",
					at->bam_delete_proc, 0, 0 );
				backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );

				rs->sr_text = "SQL-backend error";
				rs->sr_err = LDAP_OTHER;
				goto done;
			}

	   		if ( BACKSQL_IS_DEL( at->bam_expect_return ) ) {
				pno = 1;
				rc = backsql_BindParamInt( sth, 1,
						SQL_PARAM_OUTPUT, &prc );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_delete_all_values(): "
						"error binding output parameter for %s[%d]\n",
						at->bam_ad->ad_cname.bv_val, i, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );
					SQLFreeStmt( sth, SQL_DROP );

					rs->sr_text = "SQL-backend error";
					rs->sr_err = LDAP_OTHER;
					goto done;
				}
			}
			po = ( BACKSQL_IS_DEL( at->bam_param_order ) ) > 0;
			rc = backsql_BindParamID( sth, pno + 1 + po,
				SQL_PARAM_INPUT, &e_id->eid_keyval );
			if ( rc != SQL_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_delete_all_values(): "
					"error binding keyval parameter for %s[%d]\n",
					at->bam_ad->ad_cname.bv_val, i, 0 );
				backsql_PrintErrors( bi->sql_db_env, dbh, 
					sth, rc );
				SQLFreeStmt( sth, SQL_DROP );

				rs->sr_text = "SQL-backend error";
				rs->sr_err = LDAP_OTHER;
				goto done;
			}

			Debug( LDAP_DEBUG_TRACE,
				"   backsql_modify_delete_all_values() "
				"arg(%d)=" BACKSQL_IDFMT "\n",
				pno + 1 + po,
				BACKSQL_IDARG(e_id->eid_keyval), 0 );

			/*
			 * check for syntax needed here 
			 * maybe need binary bind?
			 */
			col_len = strlen( row.cols[ i ] );
			rc = backsql_BindParamStr( sth, pno + 2 - po,
				SQL_PARAM_INPUT, row.cols[ i ], col_len );
			if ( rc != SQL_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_delete_all_values(): "
					"error binding value parameter for %s[%d]\n",
					at->bam_ad->ad_cname.bv_val, i, 0 );
				backsql_PrintErrors( bi->sql_db_env, dbh, 
					sth, rc );
				SQLFreeStmt( sth, SQL_DROP );

				rs->sr_text = "SQL-backend error";
				rs->sr_err = LDAP_OTHER;
				goto done;
			}
	 
			Debug( LDAP_DEBUG_TRACE, 
				"   backsql_modify_delete_all_values(): "
				"arg(%d)=%s; executing \"%s\"\n",
				pno + 2 - po, row.cols[ i ],
				at->bam_delete_proc );
			rc = SQLExecute( sth );
			if ( rc == SQL_SUCCESS && prc == LDAP_SUCCESS ) {
				rs->sr_err = LDAP_SUCCESS;

			} else {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_delete_all_values(): "
					"delete_proc "
					"execution failed (rc=%d, prc=%d)\n",
					rc, prc, 0 );
				if ( prc != LDAP_SUCCESS ) {
					/* SQL procedure executed fine 
					 * but returned an error */
					rs->sr_err = BACKSQL_SANITIZE_ERROR( prc );

				} else {
					backsql_PrintErrors( bi->sql_db_env, dbh,
							sth, rc );
					rs->sr_err = LDAP_OTHER;
				}
				rs->sr_text = op->o_req_dn.bv_val;
				SQLFreeStmt( sth, SQL_DROP );
				goto done;
			}
			SQLFreeStmt( sth, SQL_DROP );
		}
	}

	rs->sr_err = LDAP_SUCCESS;

done:;
	backsql_FreeRow_x( &row, op->o_tmpmemctx );
	SQLFreeStmt( asth, SQL_DROP );

	return rs->sr_err;
}

int
backsql_modify_internal(
	Operation 		*op,
	SlapReply		*rs,
	SQLHDBC			dbh, 
	backsql_oc_map_rec	*oc,
	backsql_entryID		*e_id,
	Modifications		*modlist )
{
	backsql_info	*bi = (backsql_info *)op->o_bd->be_private;
	RETCODE		rc;
	Modifications	*ml;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_modify_internal(): "
		"traversing modifications list\n", 0, 0, 0 );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		AttributeDescription	*ad;
		int			sm_op;
		static char		*sm_ops[] = { "add", "delete", "replace", "increment", NULL };

		BerVarray		sm_values;
#if 0
		/* NOTE: some day we'll have to pass 
		 * the normalized values as well */
		BerVarray		sm_nvalues;
#endif
		backsql_at_map_rec	*at = NULL;
		struct berval		*at_val;
		int			i;
		
		ad = ml->sml_mod.sm_desc;
		sm_op = ( ml->sml_mod.sm_op & LDAP_MOD_OP );
		sm_values = ml->sml_mod.sm_values;
#if 0
		sm_nvalues = ml->sml_mod.sm_nvalues;
#endif

		Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
			"modifying attribute \"%s\" (%s) according to "
			"mappings for objectClass \"%s\"\n",
			ad->ad_cname.bv_val, sm_ops[ sm_op ], BACKSQL_OC_NAME( oc ) );

		if ( backsql_attr_skip( ad, sm_values ) ) {
			continue;
		}

  		at = backsql_ad2at( oc, ad );
		if ( at == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
				"attribute \"%s\" is not registered "
				"in objectClass \"%s\"\n",
				ad->ad_cname.bv_val, BACKSQL_OC_NAME( oc ), 0 );

			if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "operation not permitted "
					"within namingContext";
				goto done;
			}

			continue;
		}
  
		switch ( sm_op ) {
		case LDAP_MOD_REPLACE: {
			Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
				"replacing values for attribute \"%s\"\n",
				at->bam_ad->ad_cname.bv_val, 0, 0 );

			if ( at->bam_add_proc == NULL ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"add procedure is not defined "
					"for attribute \"%s\" "
					"- unable to perform replacements\n",
					at->bam_ad->ad_cname.bv_val, 0, 0 );

				if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "operation not permitted "
						"within namingContext";
					goto done;
				}

				break;
			}

			if ( at->bam_delete_proc == NULL ) {
				if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"delete procedure is not defined "
						"for attribute \"%s\"\n",
						at->bam_ad->ad_cname.bv_val, 0, 0 );

					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "operation not permitted "
						"within namingContext";
					goto done;
				}

				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"delete procedure is not defined "
					"for attribute \"%s\" "
					"- adding only\n",
					at->bam_ad->ad_cname.bv_val, 0, 0 );

				goto add_only;
			}

del_all:
			rs->sr_err = backsql_modify_delete_all_values( op, rs, dbh, e_id, at );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				goto done;
			}

			/* LDAP_MOD_DELETE gets here if all values must be deleted */
			if ( sm_op == LDAP_MOD_DELETE ) {
				break;
			}
	       	}

		/*
		 * PASSTHROUGH - to add new attributes -- do NOT add break
		 */
		case LDAP_MOD_ADD:
		/* case SLAP_MOD_SOFTADD: */
		/* case SLAP_MOD_ADD_IF_NOT_PRESENT: */
add_only:;
			if ( at->bam_add_proc == NULL ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"add procedure is not defined "
					"for attribute \"%s\"\n",
					at->bam_ad->ad_cname.bv_val, 0, 0 );

				if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "operation not permitted "
						"within namingContext";
					goto done;
				}

				break;
			}
			
			Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
				"adding new values for attribute \"%s\"\n",
				at->bam_ad->ad_cname.bv_val, 0, 0 );

			/* can't add a NULL val array */
			assert( sm_values != NULL );
			
			for ( i = 0, at_val = sm_values;
					!BER_BVISNULL( at_val ); 
					i++, at_val++ )
			{
				SQLHSTMT	sth = SQL_NULL_HSTMT;
				/* first parameter position, parameter order */
				SQLUSMALLINT	pno = 0,
						po;
				/* procedure return code */
				int		prc = LDAP_SUCCESS;

				rc = backsql_Prepare( dbh, &sth, at->bam_add_proc, 0 );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error preparing add query\n", 
						0, 0, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );

					rs->sr_err = LDAP_OTHER;
					rs->sr_text = "SQL-backend error";
					goto done;
				}

				if ( BACKSQL_IS_ADD( at->bam_expect_return ) ) {
					pno = 1;
	      				rc = backsql_BindParamInt( sth, 1,
						SQL_PARAM_OUTPUT, &prc );
					if ( rc != SQL_SUCCESS ) {
						Debug( LDAP_DEBUG_TRACE,
							"   backsql_modify_internal(): "
							"error binding output parameter for %s[%d]\n",
							at->bam_ad->ad_cname.bv_val, i, 0 );
						backsql_PrintErrors( bi->sql_db_env, dbh, 
							sth, rc );
						SQLFreeStmt( sth, SQL_DROP );

						rs->sr_text = "SQL-backend error";
						rs->sr_err = LDAP_OTHER;
						goto done;
					}
				}
				po = ( BACKSQL_IS_ADD( at->bam_param_order ) ) > 0;
				rc = backsql_BindParamID( sth, pno + 1 + po,
					SQL_PARAM_INPUT, &e_id->eid_keyval );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error binding keyval parameter for %s[%d]\n",
						at->bam_ad->ad_cname.bv_val, i, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );
					SQLFreeStmt( sth, SQL_DROP );

					rs->sr_text = "SQL-backend error";
					rs->sr_err = LDAP_OTHER;
					goto done;
				}

				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"arg(%d)=" BACKSQL_IDFMT "\n", 
					pno + 1 + po,
					BACKSQL_IDARG(e_id->eid_keyval), 0 );

				/*
				 * check for syntax needed here
				 * maybe need binary bind?
				 */
				rc = backsql_BindParamBerVal( sth, pno + 2 - po,
					SQL_PARAM_INPUT, at_val );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error binding value parameter for %s[%d]\n",
						at->bam_ad->ad_cname.bv_val, i, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );
					SQLFreeStmt( sth, SQL_DROP );

					rs->sr_text = "SQL-backend error";
					rs->sr_err = LDAP_OTHER;
					goto done;
				}
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"arg(%d)=\"%s\"; executing \"%s\"\n", 
					pno + 2 - po, at_val->bv_val,
					at->bam_add_proc );

				rc = SQLExecute( sth );
				if ( rc == SQL_SUCCESS && prc == LDAP_SUCCESS ) {
					rs->sr_err = LDAP_SUCCESS;

				} else {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"add_proc execution failed "
						"(rc=%d, prc=%d)\n",
						rc, prc, 0 );
					if ( prc != LDAP_SUCCESS ) {
						/* SQL procedure executed fine 
						 * but returned an error */
						SQLFreeStmt( sth, SQL_DROP );

						rs->sr_err = BACKSQL_SANITIZE_ERROR( prc );
						rs->sr_text = at->bam_ad->ad_cname.bv_val;
						return rs->sr_err;
					
					} else {
						backsql_PrintErrors( bi->sql_db_env, dbh,
								sth, rc );
						if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) 
						{
							SQLFreeStmt( sth, SQL_DROP );

							rs->sr_err = LDAP_OTHER;
							rs->sr_text = "SQL-backend error";
							goto done;
						}
					}
				}
				SQLFreeStmt( sth, SQL_DROP );
			}
			break;
			
	      	case LDAP_MOD_DELETE:
		/* case SLAP_MOD_SOFTDEL: */
			if ( at->bam_delete_proc == NULL ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"delete procedure is not defined "
					"for attribute \"%s\"\n",
					at->bam_ad->ad_cname.bv_val, 0, 0 );

				if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "operation not permitted "
						"within namingContext";
					goto done;
				}

				break;
			}

			if ( sm_values == NULL ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"no values given to delete "
					"for attribute \"%s\" "
					"-- deleting all values\n",
					at->bam_ad->ad_cname.bv_val, 0, 0 );
				goto del_all;
			}

			Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
				"deleting values for attribute \"%s\"\n",
				at->bam_ad->ad_cname.bv_val, 0, 0 );

			for ( i = 0, at_val = sm_values;
					!BER_BVISNULL( at_val );
					i++, at_val++ )
			{
				SQLHSTMT	sth = SQL_NULL_HSTMT;
				/* first parameter position, parameter order */
				SQLUSMALLINT	pno = 0,
						po;
				/* procedure return code */
				int		prc = LDAP_SUCCESS;

				rc = backsql_Prepare( dbh, &sth, at->bam_delete_proc, 0 );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error preparing delete query\n", 
						0, 0, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );

					rs->sr_err = LDAP_OTHER;
					rs->sr_text = "SQL-backend error";
					goto done;
				}

				if ( BACKSQL_IS_DEL( at->bam_expect_return ) ) {
					pno = 1;
					rc = backsql_BindParamInt( sth, 1,
						SQL_PARAM_OUTPUT, &prc );
					if ( rc != SQL_SUCCESS ) {
						Debug( LDAP_DEBUG_TRACE,
							"   backsql_modify_internal(): "
							"error binding output parameter for %s[%d]\n",
							at->bam_ad->ad_cname.bv_val, i, 0 );
						backsql_PrintErrors( bi->sql_db_env, dbh, 
							sth, rc );
						SQLFreeStmt( sth, SQL_DROP );

						rs->sr_text = "SQL-backend error";
						rs->sr_err = LDAP_OTHER;
						goto done;
					}
				}
				po = ( BACKSQL_IS_DEL( at->bam_param_order ) ) > 0;
				rc = backsql_BindParamID( sth, pno + 1 + po,
					SQL_PARAM_INPUT, &e_id->eid_keyval );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error binding keyval parameter for %s[%d]\n",
						at->bam_ad->ad_cname.bv_val, i, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );
					SQLFreeStmt( sth, SQL_DROP );

					rs->sr_text = "SQL-backend error";
					rs->sr_err = LDAP_OTHER;
					goto done;
				}

				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"arg(%d)=" BACKSQL_IDFMT "\n", 
					pno + 1 + po,
					BACKSQL_IDARG(e_id->eid_keyval), 0 );

				/*
				 * check for syntax needed here 
				 * maybe need binary bind?
				 */
				rc = backsql_BindParamBerVal( sth, pno + 2 - po,
					SQL_PARAM_INPUT, at_val );
				if ( rc != SQL_SUCCESS ) {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"error binding value parameter for %s[%d]\n",
						at->bam_ad->ad_cname.bv_val, i, 0 );
					backsql_PrintErrors( bi->sql_db_env, dbh, 
						sth, rc );
					SQLFreeStmt( sth, SQL_DROP );

					rs->sr_text = "SQL-backend error";
					rs->sr_err = LDAP_OTHER;
					goto done;
				}

				Debug( LDAP_DEBUG_TRACE,
					"   backsql_modify_internal(): "
					"executing \"%s\"\n", 
					at->bam_delete_proc, 0, 0 );
				rc = SQLExecute( sth );
				if ( rc == SQL_SUCCESS && prc == LDAP_SUCCESS )
				{
					rs->sr_err = LDAP_SUCCESS;
					
				} else {
					Debug( LDAP_DEBUG_TRACE,
						"   backsql_modify_internal(): "
						"delete_proc execution "
						"failed (rc=%d, prc=%d)\n",
						rc, prc, 0 );

					if ( prc != LDAP_SUCCESS ) {
						/* SQL procedure executed fine
						 * but returned an error */
						rs->sr_err = BACKSQL_SANITIZE_ERROR( prc );
						rs->sr_text = at->bam_ad->ad_cname.bv_val;
						goto done;
						
					} else {
						backsql_PrintErrors( bi->sql_db_env,
								dbh, sth, rc );
						SQLFreeStmt( sth, SQL_DROP );
						rs->sr_err = LDAP_OTHER;
						rs->sr_text = at->bam_ad->ad_cname.bv_val;
						goto done;
					}
				}
				SQLFreeStmt( sth, SQL_DROP );
			}
			break;

	      	case LDAP_MOD_INCREMENT:
			Debug( LDAP_DEBUG_TRACE, "   backsql_modify_internal(): "
				"increment not supported yet\n", 0, 0, 0 );
			if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "SQL-backend error";
				goto done;
			}
			break;
		}
	}

done:;
	Debug( LDAP_DEBUG_TRACE, "<==backsql_modify_internal(): %d%s%s\n",
		rs->sr_err,
		rs->sr_text ? ": " : "",
		rs->sr_text ? rs->sr_text : "" );

	/*
	 * FIXME: should fail in case one change fails?
	 */
	return rs->sr_err;
}

static int
backsql_add_attr(
	Operation		*op,
	SlapReply		*rs,
	SQLHDBC 		dbh,
	backsql_oc_map_rec 	*oc,
	Attribute		*at,
	backsql_key_t		new_keyval )
{
	backsql_info		*bi = (backsql_info*)op->o_bd->be_private;
	backsql_at_map_rec	*at_rec = NULL;
	struct berval		*at_val;
	unsigned long		i;
	RETCODE			rc;
	SQLUSMALLINT		currpos;
	SQLHSTMT 		sth = SQL_NULL_HSTMT;

	at_rec = backsql_ad2at( oc, at->a_desc ); 
  
	if ( at_rec == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add_attr(\"%s\"): "
			"attribute \"%s\" is not registered "
			"in objectclass \"%s\"\n",
			op->ora_e->e_name.bv_val,
			at->a_desc->ad_cname.bv_val,
			BACKSQL_OC_NAME( oc ) );

		if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
			rs->sr_text = "operation not permitted "
				"within namingContext";
			return rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		}

		return LDAP_SUCCESS;
	}
	
	if ( at_rec->bam_add_proc == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add_attr(\"%s\"): "
			"add procedure is not defined "
			"for attribute \"%s\" "
			"of structuralObjectClass \"%s\"\n",
			op->ora_e->e_name.bv_val,
			at->a_desc->ad_cname.bv_val,
			BACKSQL_OC_NAME( oc ) );

		if ( BACKSQL_FAIL_IF_NO_MAPPING( bi ) ) {
			rs->sr_text = "operation not permitted "
				"within namingContext";
			return rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		}

		return LDAP_SUCCESS;
	}

	for ( i = 0, at_val = &at->a_vals[ i ];
		       	!BER_BVISNULL( at_val );
			i++, at_val = &at->a_vals[ i ] )
	{
		/* procedure return code */
		int		prc = LDAP_SUCCESS;
		/* first parameter #, parameter order */
		SQLUSMALLINT	pno, po;
		char		logbuf[ STRLENOF("val[], id=") + 2*LDAP_PVT_INTTYPE_CHARS(unsigned long)];
		
		/*
		 * Do not deal with the objectClass that is used
		 * to build the entry
		 */
		if ( at->a_desc == slap_schema.si_ad_objectClass ) {
			if ( dn_match( at_val, &oc->bom_oc->soc_cname ) )
			{
				continue;
			}
		}

		rc = backsql_Prepare( dbh, &sth, at_rec->bam_add_proc, 0 );
		if ( rc != SQL_SUCCESS ) {
			rs->sr_text = "SQL-backend error";
			return rs->sr_err = LDAP_OTHER;
		}

		if ( BACKSQL_IS_ADD( at_rec->bam_expect_return ) ) {
			pno = 1;
			rc = backsql_BindParamInt( sth, 1, SQL_PARAM_OUTPUT, &prc );
			if ( rc != SQL_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_add_attr(): "
					"error binding output parameter for %s[%lu]\n",
					at_rec->bam_ad->ad_cname.bv_val, i, 0 );
				backsql_PrintErrors( bi->sql_db_env, dbh, 
					sth, rc );
				SQLFreeStmt( sth, SQL_DROP );

				rs->sr_text = "SQL-backend error";
				return rs->sr_err = LDAP_OTHER;
			}

		} else {
			pno = 0;
		}

		po = ( BACKSQL_IS_ADD( at_rec->bam_param_order ) ) > 0;
		currpos = pno + 1 + po;
		rc = backsql_BindParamNumID( sth, currpos,
				SQL_PARAM_INPUT, &new_keyval );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"   backsql_add_attr(): "
				"error binding keyval parameter for %s[%lu]\n",
				at_rec->bam_ad->ad_cname.bv_val, i, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, 
				sth, rc );
			SQLFreeStmt( sth, SQL_DROP );

			rs->sr_text = "SQL-backend error";
			return rs->sr_err = LDAP_OTHER;
		}

		currpos = pno + 2 - po;

		/*
		 * check for syntax needed here 
		 * maybe need binary bind?
		 */

		rc = backsql_BindParamBerVal( sth, currpos, SQL_PARAM_INPUT, at_val );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"   backsql_add_attr(): "
				"error binding value parameter for %s[%lu]\n",
				at_rec->bam_ad->ad_cname.bv_val, i, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, 
				sth, rc );
			SQLFreeStmt( sth, SQL_DROP );

			rs->sr_text = "SQL-backend error";
			return rs->sr_err = LDAP_OTHER;
		}

#ifdef LDAP_DEBUG
		if ( LogTest( LDAP_DEBUG_TRACE ) ) {
			snprintf( logbuf, sizeof( logbuf ), "val[%lu], id=" BACKSQL_IDNUMFMT,
					i, new_keyval );
			Debug( LDAP_DEBUG_TRACE, "   backsql_add_attr(\"%s\"): "
				"executing \"%s\" %s\n", 
				op->ora_e->e_name.bv_val,
				at_rec->bam_add_proc, logbuf );
		}
#endif
		rc = SQLExecute( sth );
		if ( rc == SQL_SUCCESS && prc == LDAP_SUCCESS ) {
			rs->sr_err = LDAP_SUCCESS;

		} else {
			Debug( LDAP_DEBUG_TRACE,
				"   backsql_add_attr(\"%s\"): "
				"add_proc execution failed (rc=%d, prc=%d)\n", 
				op->ora_e->e_name.bv_val, rc, prc );
			if ( prc != LDAP_SUCCESS ) {
				/* SQL procedure executed fine
				 * but returned an error */
				rs->sr_err = BACKSQL_SANITIZE_ERROR( prc );
				rs->sr_text = op->ora_e->e_name.bv_val;
				SQLFreeStmt( sth, SQL_DROP );
				return rs->sr_err;

			} else {
				backsql_PrintErrors( bi->sql_db_env, dbh,
						sth, rc );
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = op->ora_e->e_name.bv_val;
				SQLFreeStmt( sth, SQL_DROP );
				return rs->sr_err;
			}
		}
		SQLFreeStmt( sth, SQL_DROP );
	}

	return LDAP_SUCCESS;
}

int
backsql_add( Operation *op, SlapReply *rs )
{
	backsql_info		*bi = (backsql_info*)op->o_bd->be_private;
	SQLHDBC 		dbh = SQL_NULL_HDBC;
	SQLHSTMT 		sth = SQL_NULL_HSTMT;
	backsql_key_t		new_keyval = 0;
	RETCODE			rc;
	backsql_oc_map_rec 	*oc = NULL;
	backsql_srch_info	bsi = { 0 };
	Entry			p = { 0 }, *e = NULL;
	Attribute		*at,
				*at_objectClass = NULL;
	ObjectClass		*soc = NULL;
	struct berval		scname = BER_BVNULL;
	struct berval		pdn;
	struct berval		realdn = BER_BVNULL;
	int			colnum;
	slap_mask_t		mask;

	char			textbuf[ SLAP_TEXT_BUFLEN ];
	size_t			textlen = sizeof( textbuf );

#ifdef BACKSQL_SYNCPROV
	/*
	 * NOTE: fake successful result to force contextCSN to be bumped up
	 */
	if ( op->o_sync ) {
		char		buf[ LDAP_PVT_CSNSTR_BUFSIZE ];
		struct berval	csn;

		csn.bv_val = buf;
		csn.bv_len = sizeof( buf );
		slap_get_csn( op, &csn, 1 );

		rs->sr_err = LDAP_SUCCESS;
		send_ldap_result( op, rs );

		slap_graduate_commit_csn( op );

		return 0;
	}
#endif /* BACKSQL_SYNCPROV */

	Debug( LDAP_DEBUG_TRACE, "==>backsql_add(\"%s\")\n",
			op->ora_e->e_name.bv_val, 0, 0 );

	/* check schema */
	if ( BACKSQL_CHECK_SCHEMA( bi ) ) {
		char		textbuf[ SLAP_TEXT_BUFLEN ] = { '\0' };

		rs->sr_err = entry_schema_check( op, op->ora_e, NULL, 0, 1, NULL,
			&rs->sr_text, textbuf, sizeof( textbuf ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"entry failed schema check -- aborting\n",
				op->ora_e->e_name.bv_val, 0, 0 );
			e = NULL;
			goto done;
		}
	}

	slap_add_opattrs( op, &rs->sr_text, textbuf, textlen, 1 );

	if ( get_assert( op ) &&
		( test_filter( op, op->ora_e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"assertion control failed -- aborting\n",
			op->ora_e->e_name.bv_val, 0, 0 );
		e = NULL;
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto done;
	}

	/* search structuralObjectClass */
	for ( at = op->ora_e->e_attrs; at != NULL; at = at->a_next ) {
		if ( at->a_desc == slap_schema.si_ad_structuralObjectClass ) {
			break;
		}
	}

	/* there must exist */
	if ( at == NULL ) {
		char		buf[ SLAP_TEXT_BUFLEN ];
		const char	*text;

		/* search structuralObjectClass */
		for ( at = op->ora_e->e_attrs; at != NULL; at = at->a_next ) {
			if ( at->a_desc == slap_schema.si_ad_objectClass ) {
				break;
			}
		}

		if ( at == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"no objectClass\n",
				op->ora_e->e_name.bv_val, 0, 0 );
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			e = NULL;
			goto done;
		}

		rs->sr_err = structural_class( at->a_vals, &soc, NULL,
				&text, buf, sizeof( buf ), op->o_tmpmemctx );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"%s (%d)\n",
				op->ora_e->e_name.bv_val, text, rs->sr_err );
			e = NULL;
			goto done;
		}
		scname = soc->soc_cname;

	} else {
		scname = at->a_vals[0];
	}

	/* I guess we should play with sub/supertypes to find a suitable oc */
	oc = backsql_name2oc( bi, &scname );

	if ( oc == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"cannot map structuralObjectClass \"%s\" -- aborting\n",
			op->ora_e->e_name.bv_val,
			scname.bv_val, 0 );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "operation not permitted within namingContext";
		e = NULL;
		goto done;
	}

	if ( oc->bom_create_proc == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"create procedure is not defined "
			"for structuralObjectClass \"%s\" - aborting\n",
			op->ora_e->e_name.bv_val,
			scname.bv_val, 0 );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "operation not permitted within namingContext";
		e = NULL;
		goto done;

	} else if ( BACKSQL_CREATE_NEEDS_SELECT( bi )
			&& oc->bom_create_keyval == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"create procedure needs select procedure, "
			"but none is defined for structuralObjectClass \"%s\" "
			"- aborting\n",
			op->ora_e->e_name.bv_val,
			scname.bv_val, 0 );
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "operation not permitted within namingContext";
		e = NULL;
		goto done;
	}

	/* check write access */
	if ( !access_allowed_mask( op, op->ora_e,
				slap_schema.si_ad_entry,
				NULL, ACL_WADD, NULL, &mask ) )
	{
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		e = op->ora_e;
		goto done;
	}

	rs->sr_err = backsql_get_db_conn( op, &dbh );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"could not get connection handle - exiting\n", 
			op->ora_e->e_name.bv_val, 0, 0 );
		rs->sr_text = ( rs->sr_err == LDAP_OTHER )
			?  "SQL-backend error" : NULL;
		e = NULL;
		goto done;
	}

	/*
	 * Check if entry exists
	 *
	 * NOTE: backsql_api_dn2odbc() is called explicitly because
	 * we need the mucked DN to pass it to the create procedure.
	 */
	realdn = op->ora_e->e_name;
	if ( backsql_api_dn2odbc( op, rs, &realdn ) ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"backsql_api_dn2odbc(\"%s\") failed\n", 
			op->ora_e->e_name.bv_val, realdn.bv_val, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}

	rs->sr_err = backsql_dn2id( op, rs, dbh, &realdn, NULL, 0, 0 );
	if ( rs->sr_err == LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"entry exists\n",
			op->ora_e->e_name.bv_val, 0, 0 );
		rs->sr_err = LDAP_ALREADY_EXISTS;
		e = op->ora_e;
		goto done;
	}

	/*
	 * Get the parent dn and see if the corresponding entry exists.
	 */
	if ( be_issuffix( op->o_bd, &op->ora_e->e_nname ) ) {
		pdn = slap_empty_bv;

	} else {
		dnParent( &op->ora_e->e_nname, &pdn );

		/*
		 * Get the parent
		 */
		bsi.bsi_e = &p;
		rs->sr_err = backsql_init_search( &bsi, &pdn,
				LDAP_SCOPE_BASE, 
				(time_t)(-1), NULL, dbh, op, rs, slap_anlist_no_attrs,
				( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY ) );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_add(): "
				"could not retrieve addDN parent "
				"\"%s\" ID - %s matched=\"%s\"\n", 
				pdn.bv_val,
				rs->sr_err == LDAP_REFERRAL ? "referral" : "no such entry",
				rs->sr_matched ? rs->sr_matched : "(null)" );
			e = &p;
			goto done;
		}

		/* check "children" pseudo-attribute access to parent */
		if ( !access_allowed( op, &p, slap_schema.si_ad_children,
					NULL, ACL_WADD, NULL ) )
		{
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			e = &p;
			goto done;
		}
	}

	/*
	 * create_proc is executed; if expect_return is set, then
	 * an output parameter is bound, which should contain 
	 * the id of the added row; otherwise the procedure
	 * is expected to return the id as the first column of a select
	 */
	rc = backsql_Prepare( dbh, &sth, oc->bom_create_proc, 0 );
	if ( rc != SQL_SUCCESS ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}

	colnum = 1;
	if ( BACKSQL_IS_ADD( oc->bom_expect_return ) ) {
		rc = backsql_BindParamNumID( sth, 1, SQL_PARAM_OUTPUT, &new_keyval );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"error binding keyval parameter "
				"for objectClass %s\n",
				op->ora_e->e_name.bv_val,
				oc->bom_oc->soc_cname.bv_val, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, 
				sth, rc );
			SQLFreeStmt( sth, SQL_DROP );

			rs->sr_text = "SQL-backend error";
			rs->sr_err = LDAP_OTHER;
			e = NULL;
			goto done;
		}
		colnum++;
	}

	if ( oc->bom_create_hint ) {
		at = attr_find( op->ora_e->e_attrs, oc->bom_create_hint );
		if ( at && at->a_vals ) {
			backsql_BindParamStr( sth, colnum, SQL_PARAM_INPUT,
					at->a_vals[0].bv_val,
					at->a_vals[0].bv_len );
			Debug( LDAP_DEBUG_TRACE, "backsql_add(): "
					"create_proc hint: param = '%s'\n",
					at->a_vals[0].bv_val, 0, 0 );

		} else {
			backsql_BindParamStr( sth, colnum, SQL_PARAM_INPUT,
					"", 0 );
			Debug( LDAP_DEBUG_TRACE, "backsql_add(): "
					"create_proc hint (%s) not avalable\n",
					oc->bom_create_hint->ad_cname.bv_val,
					0, 0 );
		}
		colnum++;
	}

	Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): executing \"%s\"\n",
		op->ora_e->e_name.bv_val, oc->bom_create_proc, 0 );
	rc = SQLExecute( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"create_proc execution failed\n",
			op->ora_e->e_name.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc);
		SQLFreeStmt( sth, SQL_DROP );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}

	/* FIXME: after SQLExecute(), the row is already inserted
	 * (at least with PostgreSQL and unixODBC); needs investigation */

	if ( !BACKSQL_IS_ADD( oc->bom_expect_return ) ) {
		SWORD		ncols;
		SQLLEN		value_len;

		if ( BACKSQL_CREATE_NEEDS_SELECT( bi ) ) {
			SQLFreeStmt( sth, SQL_DROP );

			rc = backsql_Prepare( dbh, &sth, oc->bom_create_keyval, 0 );
			if ( rc != SQL_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "SQL-backend error";
				e = NULL;
				goto done;
			}

			rc = SQLExecute( sth );
			if ( rc != SQL_SUCCESS ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "SQL-backend error";
				e = NULL;
				goto done;
			}
		}

		/*
		 * the query to know the id of the inserted entry
		 * must be embedded in the create procedure
		 */
		rc = SQLNumResultCols( sth, &ncols );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"create_proc result evaluation failed\n",
				op->ora_e->e_name.bv_val, 0, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc);
			SQLFreeStmt( sth, SQL_DROP );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "SQL-backend error";
			e = NULL;
			goto done;

		} else if ( ncols != 1 ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"create_proc result is bogus (ncols=%d)\n",
				op->ora_e->e_name.bv_val, ncols, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc);
			SQLFreeStmt( sth, SQL_DROP );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "SQL-backend error";
			e = NULL;
			goto done;
		}

#if 0
		{
			SQLCHAR		colname[ 64 ];
			SQLSMALLINT	name_len, col_type, col_scale, col_null;
			UDWORD		col_prec;

			/*
			 * FIXME: check whether col_type is compatible,
			 * if it can be null and so on ...
			 */
			rc = SQLDescribeCol( sth, (SQLUSMALLINT)1, 
					&colname[ 0 ], 
					(SQLUINTEGER)( sizeof( colname ) - 1 ),
					&name_len, &col_type,
					&col_prec, &col_scale, &col_null );
		}
#endif

		rc = SQLBindCol( sth, (SQLUSMALLINT)1, SQL_C_ULONG,
				(SQLPOINTER)&new_keyval, 
				(SQLINTEGER)sizeof( new_keyval ), 
				&value_len );

		rc = SQLFetch( sth );

		if ( value_len <= 0 ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
				"create_proc result is empty?\n",
				op->ora_e->e_name.bv_val, 0, 0 );
			backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc);
			SQLFreeStmt( sth, SQL_DROP );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "SQL-backend error";
			e = NULL;
			goto done;
		}
	}

	SQLFreeStmt( sth, SQL_DROP );

	Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
		"create_proc returned keyval=" BACKSQL_IDNUMFMT "\n",
		op->ora_e->e_name.bv_val, new_keyval, 0 );

	rc = backsql_Prepare( dbh, &sth, bi->sql_insentry_stmt, 0 );
	if ( rc != SQL_SUCCESS ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}
	
	rc = backsql_BindParamBerVal( sth, 1, SQL_PARAM_INPUT, &realdn );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"error binding DN parameter for objectClass %s\n",
			op->ora_e->e_name.bv_val,
			oc->bom_oc->soc_cname.bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamNumID( sth, 2, SQL_PARAM_INPUT, &oc->bom_id );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"error binding objectClass ID parameter "
			"for objectClass %s\n",
			op->ora_e->e_name.bv_val,
			oc->bom_oc->soc_cname.bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamID( sth, 3, SQL_PARAM_INPUT, &bsi.bsi_base_id.eid_id );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"error binding parent ID parameter "
			"for objectClass %s\n",
			op->ora_e->e_name.bv_val,
			oc->bom_oc->soc_cname.bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	rc = backsql_BindParamNumID( sth, 4, SQL_PARAM_INPUT, &new_keyval );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"error binding entry ID parameter "
			"for objectClass %s\n",
			op->ora_e->e_name.bv_val,
			oc->bom_oc->soc_cname.bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, 
			sth, rc );
		SQLFreeStmt( sth, SQL_DROP );

		rs->sr_text = "SQL-backend error";
		rs->sr_err = LDAP_OTHER;
		e = NULL;
		goto done;
	}

	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		char buf[ SLAP_TEXT_BUFLEN ];

		snprintf( buf, sizeof(buf),
			"executing \"%s\" for dn=\"%s\"  oc_map_id=" BACKSQL_IDNUMFMT " p_id=" BACKSQL_IDFMT " keyval=" BACKSQL_IDNUMFMT,
			bi->sql_insentry_stmt, op->ora_e->e_name.bv_val,
			oc->bom_id, BACKSQL_IDARG(bsi.bsi_base_id.eid_id),
			new_keyval );
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(): %s\n", buf, 0, 0 );
	}

	rc = SQLExecute( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(\"%s\"): "
			"could not insert ldap_entries record\n",
			op->ora_e->e_name.bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		
		/*
		 * execute delete_proc to delete data added !!!
		 */
		SQLFreeStmt( sth, SQL_DROP );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "SQL-backend error";
		e = NULL;
		goto done;
	}

	SQLFreeStmt( sth, SQL_DROP );

	for ( at = op->ora_e->e_attrs; at != NULL; at = at->a_next ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_add(): "
			"adding attribute \"%s\"\n", 
			at->a_desc->ad_cname.bv_val, 0, 0 );

		/*
		 * Skip:
		 * - the first occurrence of objectClass, which is used
		 *   to determine how to build the SQL entry (FIXME ?!?)
		 * - operational attributes
		 * - empty attributes (FIXME ?!?)
		 */
		if ( backsql_attr_skip( at->a_desc, at->a_vals ) ) {
			continue;
		}

		if ( at->a_desc == slap_schema.si_ad_objectClass ) {
			at_objectClass = at;
			continue;
		}

		rs->sr_err = backsql_add_attr( op, rs, dbh, oc, at, new_keyval );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			e = op->ora_e;
			goto done;
		}
	}

	if ( at_objectClass ) {
		rs->sr_err = backsql_add_attr( op, rs, dbh, oc,
				at_objectClass, new_keyval );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			e = op->ora_e;
			goto done;
		}
	}

done:;
	/*
	 * Commit only if all operations succeed
	 */
	if ( sth != SQL_NULL_HSTMT ) {
		SQLUSMALLINT	CompletionType = SQL_ROLLBACK;

		if ( rs->sr_err == LDAP_SUCCESS && !op->o_noop ) {
			assert( e == NULL );
			CompletionType = SQL_COMMIT;
		}

		SQLTransact( SQL_NULL_HENV, dbh, CompletionType );
	}

	/*
	 * FIXME: NOOP does not work for add -- it works for all 
	 * the other operations, and I don't get the reason :(
	 * 
	 * hint: there might be some autocommit in Postgres
	 * so that when the unique id of the key table is
	 * automatically increased, there's no rollback.
	 * We might implement a "rollback" procedure consisting
	 * in deleting that row.
	 */

	if ( e != NULL ) {
		int	disclose = 1;

		if ( e == op->ora_e && !ACL_GRANT( mask, ACL_DISCLOSE ) ) {
			/* mask already collected */
			disclose = 0;

		} else if ( e == &p && !access_allowed( op, &p,
					slap_schema.si_ad_entry, NULL,
					ACL_DISCLOSE, NULL ) )
		{
			disclose = 0;
		}

		if ( disclose == 0 ) {
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

	if ( !BER_BVISNULL( &realdn )
			&& realdn.bv_val != op->ora_e->e_name.bv_val )
	{
		ch_free( realdn.bv_val );
	}

	if ( !BER_BVISNULL( &bsi.bsi_base_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &p.e_nname ) ) {
		backsql_entry_clean( op, &p );
	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_add(\"%s\"): %d \"%s\"\n",
			op->ora_e->e_name.bv_val,
			rs->sr_err,
			rs->sr_text ? rs->sr_text : "" );

	rs->sr_text = NULL;
	rs->sr_matched = NULL;
	if ( rs->sr_ref ) {
		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
	}

	return rs->sr_err;
}

