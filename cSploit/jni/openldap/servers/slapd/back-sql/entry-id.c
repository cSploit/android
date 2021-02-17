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

#include "lutil.h"
#include "slap.h"
#include "proto-sql.h"

#ifdef BACKSQL_ARBITRARY_KEY
struct berval backsql_baseObject_bv = BER_BVC( BACKSQL_BASEOBJECT_IDSTR );
#endif /* BACKSQL_ARBITRARY_KEY */

backsql_entryID *
backsql_entryID_dup( backsql_entryID *src, void *ctx )
{
	backsql_entryID	*dst;

	if ( src == NULL ) return NULL;

	dst = slap_sl_calloc( 1, sizeof( backsql_entryID ), ctx );
	ber_dupbv_x( &dst->eid_ndn, &src->eid_ndn, ctx );
	if ( src->eid_dn.bv_val == src->eid_ndn.bv_val ) {
		dst->eid_dn = dst->eid_ndn;
	} else {
		ber_dupbv_x( &dst->eid_dn, &src->eid_dn, ctx );
	}

#ifdef BACKSQL_ARBITRARY_KEY
	ber_dupbv_x( &dst->eid_id, &src->eid_id, ctx );
	ber_dupbv_x( &dst->eid_keyval, &src->eid_keyval, ctx );
#else /* ! BACKSQL_ARBITRARY_KEY */
	dst->eid_id = src->eid_id;
	dst->eid_keyval = src->eid_keyval;
#endif /* ! BACKSQL_ARBITRARY_KEY */

	dst->eid_oc = src->eid_oc;
	dst->eid_oc_id = src->eid_oc_id;

	return dst;
}

backsql_entryID *
backsql_free_entryID( backsql_entryID *id, int freeit, void *ctx )
{
	backsql_entryID 	*next;

	assert( id != NULL );

	next = id->eid_next;

	if ( !BER_BVISNULL( &id->eid_ndn ) ) {
		if ( !BER_BVISNULL( &id->eid_dn )
				&& id->eid_dn.bv_val != id->eid_ndn.bv_val )
		{
			slap_sl_free( id->eid_dn.bv_val, ctx );
			BER_BVZERO( &id->eid_dn );
		}

		slap_sl_free( id->eid_ndn.bv_val, ctx );
		BER_BVZERO( &id->eid_ndn );
	}

#ifdef BACKSQL_ARBITRARY_KEY
	if ( !BER_BVISNULL( &id->eid_id ) ) {
		slap_sl_free( id->eid_id.bv_val, ctx );
		BER_BVZERO( &id->eid_id );
	}

	if ( !BER_BVISNULL( &id->eid_keyval ) ) {
		slap_sl_free( id->eid_keyval.bv_val, ctx );
		BER_BVZERO( &id->eid_keyval );
	}
#endif /* BACKSQL_ARBITRARY_KEY */

	if ( freeit ) {
		slap_sl_free( id, ctx );
	}

	return next;
}

/*
 * NOTE: the dn must be normalized
 */
int
backsql_dn2id(
	Operation		*op,
	SlapReply		*rs,
	SQLHDBC			dbh,
	struct berval		*ndn,
	backsql_entryID		*id,
	int			matched,
	int			muck )
{
	backsql_info		*bi = op->o_bd->be_private;
	SQLHSTMT		sth = SQL_NULL_HSTMT; 
	BACKSQL_ROW_NTS		row = { 0 };
	RETCODE 		rc;
	int			res;
	struct berval		realndn = BER_BVNULL;

	/* TimesTen */
	char			upperdn[ BACKSQL_MAX_DN_LEN + 1 ];
	struct berval		tbbDN;
	int			i, j;

	/*
	 * NOTE: id can be NULL; in this case, the function
	 * simply checks whether the DN can be successfully 
	 * turned into an ID, returning LDAP_SUCCESS for
	 * positive cases, or the most appropriate error
	 */

	Debug( LDAP_DEBUG_TRACE, "==>backsql_dn2id(\"%s\")%s%s\n", 
			ndn->bv_val, id == NULL ? " (no ID expected)" : "",
			matched ? " matched expected" : "" );

	if ( id ) {
		/* NOTE: trap inconsistencies */
		assert( BER_BVISNULL( &id->eid_ndn ) );
	}

	if ( ndn->bv_len > BACKSQL_MAX_DN_LEN ) {
		Debug( LDAP_DEBUG_TRACE, 
			"   backsql_dn2id(\"%s\"): DN length=%ld "
			"exceeds max DN length %d:\n",
			ndn->bv_val, ndn->bv_len, BACKSQL_MAX_DN_LEN );
		return LDAP_OTHER;
	}

	/* return baseObject if available and matches */
	/* FIXME: if ndn is already mucked, we cannot check this */
	if ( bi->sql_baseObject != NULL &&
			dn_match( ndn, &bi->sql_baseObject->e_nname ) )
	{
		if ( id != NULL ) {
#ifdef BACKSQL_ARBITRARY_KEY
			ber_dupbv_x( &id->eid_id, &backsql_baseObject_bv,
					op->o_tmpmemctx );
			ber_dupbv_x( &id->eid_keyval, &backsql_baseObject_bv,
					op->o_tmpmemctx );
#else /* ! BACKSQL_ARBITRARY_KEY */
			id->eid_id = BACKSQL_BASEOBJECT_ID;
			id->eid_keyval = BACKSQL_BASEOBJECT_KEYVAL;
#endif /* ! BACKSQL_ARBITRARY_KEY */
			id->eid_oc_id = BACKSQL_BASEOBJECT_OC;

			ber_dupbv_x( &id->eid_ndn, &bi->sql_baseObject->e_nname,
					op->o_tmpmemctx );
			ber_dupbv_x( &id->eid_dn, &bi->sql_baseObject->e_name,
					op->o_tmpmemctx );

			id->eid_next = NULL;
		}

		return LDAP_SUCCESS;
	}
	
	/* begin TimesTen */
	assert( bi->sql_id_query != NULL );
	Debug( LDAP_DEBUG_TRACE, "   backsql_dn2id(\"%s\"): id_query \"%s\"\n",
			ndn->bv_val, bi->sql_id_query, 0 );
 	rc = backsql_Prepare( dbh, &sth, bi->sql_id_query, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, 
			"   backsql_dn2id(\"%s\"): "
			"error preparing SQL:\n   %s", 
			ndn->bv_val, bi->sql_id_query, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		res = LDAP_OTHER;
		goto done;
	}

	realndn = *ndn;
	if ( muck ) {
		if ( backsql_api_dn2odbc( op, rs, &realndn ) ) {
			Debug( LDAP_DEBUG_TRACE, "   backsql_dn2id(\"%s\"): "
				"backsql_api_dn2odbc(\"%s\") failed\n", 
				ndn->bv_val, realndn.bv_val, 0 );
			res = LDAP_OTHER;
			goto done;
		}
	}

	if ( BACKSQL_HAS_LDAPINFO_DN_RU( bi ) ) {
		/*
		 * Prepare an upper cased, byte reversed version 
		 * that can be searched using indexes
		 */

		for ( i = 0, j = realndn.bv_len - 1; realndn.bv_val[ i ]; i++, j--)
		{
			upperdn[ i ] = realndn.bv_val[ j ];
		}
		upperdn[ i ] = '\0';
		ldap_pvt_str2upper( upperdn );

		Debug( LDAP_DEBUG_TRACE, "   backsql_dn2id(\"%s\"): "
				"upperdn=\"%s\"\n",
				ndn->bv_val, upperdn, 0 );
		ber_str2bv( upperdn, 0, 0, &tbbDN );

	} else {
		if ( BACKSQL_USE_REVERSE_DN( bi ) ) {
			AC_MEMCPY( upperdn, realndn.bv_val, realndn.bv_len + 1 );
			ldap_pvt_str2upper( upperdn );
			Debug( LDAP_DEBUG_TRACE,
				"   backsql_dn2id(\"%s\"): "
				"upperdn=\"%s\"\n",
				ndn->bv_val, upperdn, 0 );
			ber_str2bv( upperdn, 0, 0, &tbbDN );

		} else {
			tbbDN = realndn;
		}
	}

	rc = backsql_BindParamBerVal( sth, 1, SQL_PARAM_INPUT, &tbbDN );
	if ( rc != SQL_SUCCESS) {
		/* end TimesTen */ 
		Debug( LDAP_DEBUG_TRACE, "   backsql_dn2id(\"%s\"): "
			"error binding dn=\"%s\" parameter:\n", 
			ndn->bv_val, tbbDN.bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		res = LDAP_OTHER;
		goto done;
	}

	rc = SQLExecute( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "   backsql_dn2id(\"%s\"): "
			"error executing query (\"%s\", \"%s\"):\n", 
			ndn->bv_val, bi->sql_id_query, tbbDN.bv_val );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		res = LDAP_OTHER;
		goto done;
	}

	backsql_BindRowAsStrings_x( sth, &row, op->o_tmpmemctx );
	rc = SQLFetch( sth );
	if ( BACKSQL_SUCCESS( rc ) ) {
		char	buf[ SLAP_TEXT_BUFLEN ];

#ifdef LDAP_DEBUG
		snprintf( buf, sizeof(buf),
			"id=%s keyval=%s oc_id=%s dn=%s",
			row.cols[ 0 ], row.cols[ 1 ],
			row.cols[ 2 ], row.cols[ 3 ] );
		Debug( LDAP_DEBUG_TRACE,
			"   backsql_dn2id(\"%s\"): %s\n",
			ndn->bv_val, buf, 0 );
#endif /* LDAP_DEBUG */

		res = LDAP_SUCCESS;
		if ( id != NULL ) {
			struct berval	dn;

			id->eid_next = NULL;

#ifdef BACKSQL_ARBITRARY_KEY
			ber_str2bv_x( row.cols[ 0 ], 0, 1, &id->eid_id,
					op->o_tmpmemctx );
			ber_str2bv_x( row.cols[ 1 ], 0, 1, &id->eid_keyval,
					op->o_tmpmemctx );
#else /* ! BACKSQL_ARBITRARY_KEY */
			if ( BACKSQL_STR2ID( &id->eid_id, row.cols[ 0 ], 0 ) != 0 ) {
				res = LDAP_OTHER;
				goto done;
			}
			if ( BACKSQL_STR2ID( &id->eid_keyval, row.cols[ 1 ], 0 ) != 0 ) {
				res = LDAP_OTHER;
				goto done;
			}
#endif /* ! BACKSQL_ARBITRARY_KEY */
			if ( BACKSQL_STR2ID( &id->eid_oc_id, row.cols[ 2 ], 0 ) != 0 ) {
				res = LDAP_OTHER;
				goto done;
			}

			ber_str2bv( row.cols[ 3 ], 0, 0, &dn );

			if ( backsql_api_odbc2dn( op, rs, &dn ) ) {
				res = LDAP_OTHER;
				goto done;
			}
			
			res = dnPrettyNormal( NULL, &dn,
					&id->eid_dn, &id->eid_ndn,
					op->o_tmpmemctx );
			if ( res != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"   backsql_dn2id(\"%s\"): "
					"dnPrettyNormal failed (%d: %s)\n",
					realndn.bv_val, res,
					ldap_err2string( res ) );

				/* cleanup... */
				(void)backsql_free_entryID( id, 0, op->o_tmpmemctx );
			}

			if ( dn.bv_val != row.cols[ 3 ] ) {
				free( dn.bv_val );
			}
		}

	} else {
		res = LDAP_NO_SUCH_OBJECT;
		if ( matched ) {
			struct berval	pdn = *ndn;

			/*
			 * Look for matched
			 */
			rs->sr_matched = NULL;
			while ( !be_issuffix( op->o_bd, &pdn ) ) {
				char		*matchedDN = NULL;
	
				dnParent( &pdn, &pdn );
	
				/*
				 * Empty DN ("") defaults to LDAP_SUCCESS
				 */
				rs->sr_err = backsql_dn2id( op, rs, dbh, &pdn, id, 0, 1 );
				switch ( rs->sr_err ) {
				case LDAP_NO_SUCH_OBJECT:
					/* try another one */
					break;
					
				case LDAP_SUCCESS:
					matchedDN = pdn.bv_val;
					/* fail over to next case */
	
				default:
					rs->sr_err = LDAP_NO_SUCH_OBJECT;
					rs->sr_matched = matchedDN;
					goto done;
				} 
			}
		}
	}

done:;
	backsql_FreeRow_x( &row, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE,
		"<==backsql_dn2id(\"%s\"): err=%d\n",
		ndn->bv_val, res, 0 );
	if ( sth != SQL_NULL_HSTMT ) {
		SQLFreeStmt( sth, SQL_DROP );
	}

	if ( !BER_BVISNULL( &realndn ) && realndn.bv_val != ndn->bv_val ) {
		ch_free( realndn.bv_val );
	}

	return res;
}

int
backsql_count_children(
	Operation		*op,
	SQLHDBC			dbh,
	struct berval		*dn,
	unsigned long		*nchildren )
{
	backsql_info 		*bi = (backsql_info *)op->o_bd->be_private;
	SQLHSTMT		sth = SQL_NULL_HSTMT;
	BACKSQL_ROW_NTS		row;
	RETCODE 		rc;
	int			res = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_count_children(): dn=\"%s\"\n", 
			dn->bv_val, 0, 0 );

	if ( dn->bv_len > BACKSQL_MAX_DN_LEN ) {
		Debug( LDAP_DEBUG_TRACE, 
			"backsql_count_children(): DN \"%s\" (%ld bytes) "
			"exceeds max DN length (%d):\n",
			dn->bv_val, dn->bv_len, BACKSQL_MAX_DN_LEN );
		return LDAP_OTHER;
	}
	
	/* begin TimesTen */
	assert( bi->sql_has_children_query != NULL );
	Debug(LDAP_DEBUG_TRACE, "children id query \"%s\"\n", 
			bi->sql_has_children_query, 0, 0);
 	rc = backsql_Prepare( dbh, &sth, bi->sql_has_children_query, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, 
			"backsql_count_children(): error preparing SQL:\n%s", 
			bi->sql_has_children_query, 0, 0);
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		return LDAP_OTHER;
	}

	rc = backsql_BindParamBerVal( sth, 1, SQL_PARAM_INPUT, dn );
	if ( rc != SQL_SUCCESS) {
		/* end TimesTen */ 
		Debug( LDAP_DEBUG_TRACE, "backsql_count_children(): "
			"error binding dn=\"%s\" parameter:\n", 
			dn->bv_val, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		return LDAP_OTHER;
	}

	rc = SQLExecute( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_count_children(): "
			"error executing query (\"%s\", \"%s\"):\n", 
			bi->sql_has_children_query, dn->bv_val, 0 );
		backsql_PrintErrors( bi->sql_db_env, dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		return LDAP_OTHER;
	}

	backsql_BindRowAsStrings_x( sth, &row, op->o_tmpmemctx );
	
	rc = SQLFetch( sth );
	if ( BACKSQL_SUCCESS( rc ) ) {
		char *end;

		*nchildren = strtol( row.cols[ 0 ], &end, 0 );
		if ( end == row.cols[ 0 ] ) {
			res = LDAP_OTHER;

		} else {
			switch ( end[ 0 ] ) {
			case '\0':
				break;

			case '.': {
				unsigned long	ul;

				/* FIXME: braindead RDBMSes return
				 * a fractional number from COUNT!
				 */
				if ( lutil_atoul( &ul, end + 1 ) != 0 || ul != 0 ) {
					res = LDAP_OTHER;
				}
				} break;

			default:
				res = LDAP_OTHER;
			}
		}

	} else {
		res = LDAP_OTHER;
	}
	backsql_FreeRow_x( &row, op->o_tmpmemctx );

	SQLFreeStmt( sth, SQL_DROP );

	Debug( LDAP_DEBUG_TRACE, "<==backsql_count_children(): %lu\n",
			*nchildren, 0, 0 );

	return res;
}

int
backsql_has_children(
	Operation		*op,
	SQLHDBC			dbh,
	struct berval		*dn )
{
	unsigned long	nchildren;
	int		rc;

	rc = backsql_count_children( op, dbh, dn, &nchildren );

	if ( rc == LDAP_SUCCESS ) {
		return nchildren > 0 ? LDAP_COMPARE_TRUE : LDAP_COMPARE_FALSE;
	}

	return rc;
}

static int
backsql_get_attr_vals( void *v_at, void *v_bsi )
{
	backsql_at_map_rec	*at = v_at;
	backsql_srch_info	*bsi = v_bsi;
	backsql_info		*bi;
	RETCODE			rc;
	SQLHSTMT		sth = SQL_NULL_HSTMT;
	BACKSQL_ROW_NTS		row;
	unsigned long		i,
				k = 0,
				oldcount = 0,
				res = 0;
#ifdef BACKSQL_COUNTQUERY
	unsigned 		count,
				j,
				append = 0;
	SQLLEN			countsize = sizeof( count );
	Attribute		*attr = NULL;

	slap_mr_normalize_func		*normfunc = NULL;
#endif /* BACKSQL_COUNTQUERY */
#ifdef BACKSQL_PRETTY_VALIDATE
	slap_syntax_validate_func	*validate = NULL;
	slap_syntax_transform_func	*pretty = NULL;
#endif /* BACKSQL_PRETTY_VALIDATE */

	assert( at != NULL );
	assert( bsi != NULL );
	Debug( LDAP_DEBUG_TRACE, "==>backsql_get_attr_vals(): "
		"oc=\"%s\" attr=\"%s\" keyval=" BACKSQL_IDFMT "\n",
		BACKSQL_OC_NAME( bsi->bsi_oc ), at->bam_ad->ad_cname.bv_val, 
		BACKSQL_IDARG(bsi->bsi_c_eid->eid_keyval) );

	bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;

#ifdef BACKSQL_PRETTY_VALIDATE
	validate = at->bam_true_ad->ad_type->sat_syntax->ssyn_validate;
	pretty =  at->bam_true_ad->ad_type->sat_syntax->ssyn_pretty;

	if ( validate == NULL && pretty == NULL ) {
		return 1;
	}
#endif /* BACKSQL_PRETTY_VALIDATE */

#ifdef BACKSQL_COUNTQUERY
	if ( at->bam_true_ad->ad_type->sat_equality ) {
		normfunc = at->bam_true_ad->ad_type->sat_equality->smr_normalize;
	}

	/* Count how many rows will be returned. This avoids memory 
	 * fragmentation that can result from loading the values in 
	 * one by one and using realloc() 
	 */
	rc = backsql_Prepare( bsi->bsi_dbh, &sth, at->bam_countquery, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error preparing count query: %s\n",
			at->bam_countquery, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		return 1;
	}

	rc = backsql_BindParamID( sth, 1, SQL_PARAM_INPUT,
			&bsi->bsi_c_eid->eid_keyval );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error binding key value parameter\n", 0, 0, 0 );
		SQLFreeStmt( sth, SQL_DROP );
		return 1;
	}

	rc = SQLExecute( sth );
	if ( ! BACKSQL_SUCCESS( rc ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error executing attribute count query '%s'\n",
			at->bam_countquery, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		return 1;
	}

	SQLBindCol( sth, (SQLUSMALLINT)1, SQL_C_LONG,
			(SQLPOINTER)&count,
			(SQLINTEGER)sizeof( count ),
			&countsize );

	rc = SQLFetch( sth );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error fetch results of count query: %s\n",
			at->bam_countquery, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		return 1;
	}

	Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
		"number of values in query: %u\n", count, 0, 0 );
	SQLFreeStmt( sth, SQL_DROP );
	if ( count == 0 ) {
		return 1;
	}

	attr = attr_find( bsi->bsi_e->e_attrs, at->bam_true_ad );
	if ( attr != NULL ) {
		BerVarray	tmp;

		if ( attr->a_vals != NULL ) {
			oldcount = attr->a_numvals;
		}

		tmp = ch_realloc( attr->a_vals, ( oldcount + count + 1 ) * sizeof( struct berval ) );
		if ( tmp == NULL ) {
			return 1;
		}
		attr->a_vals = tmp;
		memset( &attr->a_vals[ oldcount ], 0, ( count + 1 ) * sizeof( struct berval ) );

		if ( normfunc ) {
			tmp = ch_realloc( attr->a_nvals, ( oldcount + count + 1 ) * sizeof( struct berval ) );
			if ( tmp == NULL ) {
				return 1;
			}
			attr->a_nvals = tmp;
			memset( &attr->a_nvals[ oldcount ], 0, ( count + 1 ) * sizeof( struct berval ) );

		} else {
			attr->a_nvals = attr->a_vals;
		}
		attr->a_numvals += count;

	} else {
		append = 1;

		/* Make space for the array of values */
		attr = attr_alloc( at->bam_true_ad );
		attr->a_numvals = count;
		attr->a_vals = ch_calloc( count + 1, sizeof( struct berval ) );
		if ( attr->a_vals == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "Out of memory!\n", 0,0,0 );
			ch_free( attr );
			return 1;
		}
		if ( normfunc ) {
			attr->a_nvals = ch_calloc( count + 1, sizeof( struct berval ) );
			if ( attr->a_nvals == NULL ) {
				ch_free( attr->a_vals );
				ch_free( attr );
				return 1;

			}

		} else {
			attr->a_nvals = attr->a_vals;
		}
	}
#endif /* BACKSQL_COUNTQUERY */

	rc = backsql_Prepare( bsi->bsi_dbh, &sth, at->bam_query, 0 );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error preparing query: %s\n", at->bam_query, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
#ifdef BACKSQL_COUNTQUERY
		if ( append ) {
			attr_free( attr );
		}
#endif /* BACKSQL_COUNTQUERY */
		return 1;
	}

	rc = backsql_BindParamID( sth, 1, SQL_PARAM_INPUT,
			&bsi->bsi_c_eid->eid_keyval );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error binding key value parameter\n", 0, 0, 0 );
#ifdef BACKSQL_COUNTQUERY
		if ( append ) {
			attr_free( attr );
		}
#endif /* BACKSQL_COUNTQUERY */
		return 1;
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
		"query=\"%s\" keyval=" BACKSQL_IDFMT "\n", at->bam_query,
		BACKSQL_IDARG(bsi->bsi_c_eid->eid_keyval), 0 );
#endif /* BACKSQL_TRACE */

	rc = SQLExecute( sth );
	if ( ! BACKSQL_SUCCESS( rc ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_get_attr_vals(): "
			"error executing attribute query \"%s\"\n",
			at->bam_query, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
#ifdef BACKSQL_COUNTQUERY
		if ( append ) {
			attr_free( attr );
		}
#endif /* BACKSQL_COUNTQUERY */
		return 1;
	}

	backsql_BindRowAsStrings_x( sth, &row, bsi->bsi_op->o_tmpmemctx );
#ifdef BACKSQL_COUNTQUERY
	j = oldcount;
#endif /* BACKSQL_COUNTQUERY */
	for ( rc = SQLFetch( sth ), k = 0;
			BACKSQL_SUCCESS( rc );
			rc = SQLFetch( sth ), k++ )
	{
		for ( i = 0; i < (unsigned long)row.ncols; i++ ) {

			if ( row.value_len[ i ] > 0 ) {
				struct berval		bv;
				int			retval;
#ifdef BACKSQL_TRACE
				AttributeDescription	*ad = NULL;
				const char		*text;

				retval = slap_bv2ad( &row.col_names[ i ], &ad, &text );
				if ( retval != LDAP_SUCCESS ) {
					Debug( LDAP_DEBUG_ANY,
						"==>backsql_get_attr_vals(\"%s\"): "
						"unable to find AttributeDescription %s "
						"in schema (%d)\n",
						bsi->bsi_e->e_name.bv_val,
						row.col_names[ i ].bv_val, retval );
					res = 1;
					goto done;
				}

				if ( ad != at->bam_ad ) {
					Debug( LDAP_DEBUG_ANY,
						"==>backsql_get_attr_vals(\"%s\"): "
						"column name %s differs from "
						"AttributeDescription %s\n",
						bsi->bsi_e->e_name.bv_val,
						ad->ad_cname.bv_val,
						at->bam_ad->ad_cname.bv_val );
					res = 1;
					goto done;
				}
#endif /* BACKSQL_TRACE */

				/* ITS#3386, ITS#3113 - 20070308
				 * If a binary is fetched?
				 * must use the actual size read
				 * from the database.
				 */
				if ( BACKSQL_IS_BINARY( row.col_type[ i ] ) ) {
#ifdef BACKSQL_TRACE
					Debug( LDAP_DEBUG_ANY,
						"==>backsql_get_attr_vals(\"%s\"): "
						"column name %s: data is binary; "
						"using database size %ld\n",
						bsi->bsi_e->e_name.bv_val,
						ad->ad_cname.bv_val,
						row.value_len[ i ] );
#endif /* BACKSQL_TRACE */
					bv.bv_val = row.cols[ i ];
					bv.bv_len = row.value_len[ i ];

				} else {
					ber_str2bv( row.cols[ i ], 0, 0, &bv );
				}

#ifdef BACKSQL_PRETTY_VALIDATE
				if ( pretty ) {
					struct berval	pbv;

					retval = pretty( at->bam_true_ad->ad_type->sat_syntax,
						&bv, &pbv, bsi->bsi_op->o_tmpmemctx );
					bv = pbv;

				} else {
					retval = validate( at->bam_true_ad->ad_type->sat_syntax,
						&bv );
				}

				if ( retval != LDAP_SUCCESS ) {
					char	buf[ SLAP_TEXT_BUFLEN ];

					/* FIXME: we're ignoring invalid values,
					 * but we're accepting the attributes;
					 * should we fail at all? */
					snprintf( buf, sizeof( buf ),
							"unable to %s value #%lu "
							"of AttributeDescription %s",
							pretty ? "prettify" : "validate",
							k - oldcount,
							at->bam_ad->ad_cname.bv_val );
					Debug( LDAP_DEBUG_TRACE,
						"==>backsql_get_attr_vals(\"%s\"): "
						"%s (%d)\n",
						bsi->bsi_e->e_name.bv_val, buf, retval );
					continue;
				}
#endif /* BACKSQL_PRETTY_VALIDATE */

#ifndef BACKSQL_COUNTQUERY
				(void)backsql_entry_addattr( bsi->bsi_e, 
						at->bam_true_ad, &bv,
						bsi->bsi_op->o_tmpmemctx );

#else /* BACKSQL_COUNTQUERY */
				if ( normfunc ) {
					struct berval	nbv;

					retval = (*normfunc)( SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
						at->bam_true_ad->ad_type->sat_syntax,
						at->bam_true_ad->ad_type->sat_equality,
						&bv, &nbv,
						bsi->bsi_op->o_tmpmemctx );

					if ( retval != LDAP_SUCCESS ) {
						char	buf[ SLAP_TEXT_BUFLEN ];

						/* FIXME: we're ignoring invalid values,
						 * but we're accepting the attributes;
						 * should we fail at all? */
						snprintf( buf, sizeof( buf ),
							"unable to normalize value #%lu "
							"of AttributeDescription %s",
							k - oldcount,
							at->bam_ad->ad_cname.bv_val );
						Debug( LDAP_DEBUG_TRACE,
							"==>backsql_get_attr_vals(\"%s\"): "
							"%s (%d)\n",
							bsi->bsi_e->e_name.bv_val, buf, retval );

#ifdef BACKSQL_PRETTY_VALIDATE
						if ( pretty ) {
							bsi->bsi_op->o_tmpfree( bv.bv_val,
									bsi->bsi_op->o_tmpmemctx );
						}
#endif /* BACKSQL_PRETTY_VALIDATE */

						continue;
					}
					ber_dupbv( &attr->a_nvals[ j ], &nbv );
					bsi->bsi_op->o_tmpfree( nbv.bv_val,
							bsi->bsi_op->o_tmpmemctx );
				}

				ber_dupbv( &attr->a_vals[ j ], &bv );

				assert( j < oldcount + count );
				j++;
#endif /* BACKSQL_COUNTQUERY */

#ifdef BACKSQL_PRETTY_VALIDATE
				if ( pretty ) {
					bsi->bsi_op->o_tmpfree( bv.bv_val,
							bsi->bsi_op->o_tmpmemctx );
				}
#endif /* BACKSQL_PRETTY_VALIDATE */

#ifdef BACKSQL_TRACE
				Debug( LDAP_DEBUG_TRACE, "prec=%d\n",
					(int)row.col_prec[ i ], 0, 0 );

			} else {
      				Debug( LDAP_DEBUG_TRACE, "NULL value "
					"in this row for attribute \"%s\"\n",
					row.col_names[ i ].bv_val, 0, 0 );
#endif /* BACKSQL_TRACE */
			}
		}
	}

#ifdef BACKSQL_COUNTQUERY
	if ( BER_BVISNULL( &attr->a_vals[ 0 ] ) ) {
		/* don't leave around attributes with no values */
		attr_free( attr );

	} else if ( append ) {
		Attribute	**ap;

		for ( ap = &bsi->bsi_e->e_attrs; (*ap) != NULL; ap = &(*ap)->a_next )
			/* goto last */ ;
		*ap =  attr;
	}
#endif /* BACKSQL_COUNTQUERY */

	SQLFreeStmt( sth, SQL_DROP );
	Debug( LDAP_DEBUG_TRACE, "<==backsql_get_attr_vals()\n", 0, 0, 0 );

	if ( at->bam_next ) {
		res = backsql_get_attr_vals( at->bam_next, v_bsi );
	} else {
		res = 1;
	}

#ifdef BACKSQL_TRACE
done:;
#endif /* BACKSQL_TRACE */
	backsql_FreeRow_x( &row, bsi->bsi_op->o_tmpmemctx );

	return res;
}

int
backsql_id2entry( backsql_srch_info *bsi, backsql_entryID *eid )
{
	Operation		*op = bsi->bsi_op;
	backsql_info		*bi = (backsql_info *)op->o_bd->be_private;
	int			i;
	int			rc;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_id2entry()\n", 0, 0, 0 );

	assert( bsi->bsi_e != NULL );

	memset( bsi->bsi_e, 0, sizeof( Entry ) );

	if ( bi->sql_baseObject && BACKSQL_IS_BASEOBJECT_ID( &eid->eid_id ) ) {
		(void)entry_dup2( bsi->bsi_e, bi->sql_baseObject );
		goto done;
	}

	bsi->bsi_e->e_attrs = NULL;
	bsi->bsi_e->e_private = NULL;

	if ( eid->eid_oc == NULL ) {
		eid->eid_oc = backsql_id2oc( bsi->bsi_op->o_bd->be_private,
			eid->eid_oc_id );
		if ( eid->eid_oc == NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				"backsql_id2entry(): unable to fetch objectClass with id=" BACKSQL_IDNUMFMT " for entry id=" BACKSQL_IDFMT " dn=\"%s\"\n",
				eid->eid_oc_id, BACKSQL_IDARG(eid->eid_id),
				eid->eid_dn.bv_val );
			return LDAP_OTHER;
		}
	}
	bsi->bsi_oc = eid->eid_oc;
	bsi->bsi_c_eid = eid;

	ber_dupbv_x( &bsi->bsi_e->e_name, &eid->eid_dn, op->o_tmpmemctx );
	ber_dupbv_x( &bsi->bsi_e->e_nname, &eid->eid_ndn, op->o_tmpmemctx );

#ifndef BACKSQL_ARBITRARY_KEY	
	/* FIXME: unused */
	bsi->bsi_e->e_id = eid->eid_id;
#endif /* ! BACKSQL_ARBITRARY_KEY */
 
	rc = attr_merge_normalize_one( bsi->bsi_e,
			slap_schema.si_ad_objectClass,
			&bsi->bsi_oc->bom_oc->soc_cname,
			bsi->bsi_op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		backsql_entry_clean( op, bsi->bsi_e );
		return rc;
	}

	if ( bsi->bsi_attrs == NULL || ( bsi->bsi_flags & BSQL_SF_ALL_USER ) )
	{
		Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(): "
			"retrieving all attributes\n", 0, 0, 0 );
		avl_apply( bsi->bsi_oc->bom_attrs, backsql_get_attr_vals,
				bsi, 0, AVL_INORDER );

	} else {
		Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(): "
			"custom attribute list\n", 0, 0, 0 );
		for ( i = 0; !BER_BVISNULL( &bsi->bsi_attrs[ i ].an_name ); i++ ) {
			backsql_at_map_rec	**vat;
			AttributeName		*an = &bsi->bsi_attrs[ i ];
			int			j;

			/* if one of the attributes listed here is
			 * a subtype of another, it must be ignored,
			 * because subtypes are already dealt with
			 * by backsql_supad2at()
			 */
			for ( j = 0; !BER_BVISNULL( &bsi->bsi_attrs[ j ].an_name ); j++ ) {
				/* skip self */
				if ( j == i ) {
					continue;
				}

				/* skip subtypes */
				if ( is_at_subtype( an->an_desc->ad_type,
							bsi->bsi_attrs[ j ].an_desc->ad_type ) )
				{
					goto next;
				}
			}

			rc = backsql_supad2at( bsi->bsi_oc, an->an_desc, &vat );
			if ( rc != 0 || vat == NULL ) {
				Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(): "
						"attribute \"%s\" is not defined "
						"for objectlass \"%s\"\n",
						an->an_name.bv_val, 
						BACKSQL_OC_NAME( bsi->bsi_oc ), 0 );
				continue;
			}

			for ( j = 0; vat[j]; j++ ) {
    				backsql_get_attr_vals( vat[j], bsi );
			}

			ch_free( vat );

next:;
		}
	}

	if ( bsi->bsi_flags & BSQL_SF_RETURN_ENTRYUUID ) {
		Attribute	*a_entryUUID,
				**ap;

		a_entryUUID = backsql_operational_entryUUID( bi, eid );
		if ( a_entryUUID != NULL ) {
			for ( ap = &bsi->bsi_e->e_attrs; 
					*ap; 
					ap = &(*ap)->a_next );

			*ap = a_entryUUID;
		}
	}

	if ( ( bsi->bsi_flags & BSQL_SF_ALL_OPER )
			|| an_find( bsi->bsi_attrs, slap_bv_all_operational_attrs )
			|| an_find( bsi->bsi_attrs, &slap_schema.si_ad_structuralObjectClass->ad_cname ) )
	{
		ObjectClass	*soc = NULL;

		if ( BACKSQL_CHECK_SCHEMA( bi ) ) {
			Attribute	*a;
			const char	*text = NULL;
			char		textbuf[ 1024 ];
			size_t		textlen = sizeof( textbuf );
			struct berval	bv[ 2 ],
					*nvals;
			int		rc = LDAP_SUCCESS;

			a = attr_find( bsi->bsi_e->e_attrs,
					slap_schema.si_ad_objectClass );
			if ( a != NULL ) {
				nvals = a->a_nvals;

			} else {
				bv[ 0 ] = bsi->bsi_oc->bom_oc->soc_cname;
				BER_BVZERO( &bv[ 1 ] );
				nvals = bv;
			}

			rc = structural_class( nvals, &soc, NULL, 
					&text, textbuf, textlen, op->o_tmpmemctx );
			if ( rc != LDAP_SUCCESS ) {
      				Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(%s): "
					"structural_class() failed %d (%s)\n",
					bsi->bsi_e->e_name.bv_val,
					rc, text ? text : "" );
				backsql_entry_clean( op, bsi->bsi_e );
				return rc;
			}

			if ( !bvmatch( &soc->soc_cname, &bsi->bsi_oc->bom_oc->soc_cname ) ) {
				if ( !is_object_subclass( bsi->bsi_oc->bom_oc, soc ) ) {
      					Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(%s): "
						"computed structuralObjectClass %s "
						"does not match objectClass %s associated "
						"to entry\n",
						bsi->bsi_e->e_name.bv_val, soc->soc_cname.bv_val,
						bsi->bsi_oc->bom_oc->soc_cname.bv_val );
					backsql_entry_clean( op, bsi->bsi_e );
					return rc;
				}

      				Debug( LDAP_DEBUG_TRACE, "backsql_id2entry(%s): "
					"computed structuralObjectClass %s "
					"is subclass of objectClass %s associated "
					"to entry\n",
					bsi->bsi_e->e_name.bv_val, soc->soc_cname.bv_val,
					bsi->bsi_oc->bom_oc->soc_cname.bv_val );
			}

		} else {
			soc = bsi->bsi_oc->bom_oc;
		}

		rc = attr_merge_normalize_one( bsi->bsi_e,
				slap_schema.si_ad_structuralObjectClass,
				&soc->soc_cname,
				bsi->bsi_op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			backsql_entry_clean( op, bsi->bsi_e );
			return rc;
		}
	}

done:;
	Debug( LDAP_DEBUG_TRACE, "<==backsql_id2entry()\n", 0, 0, 0 );

	return LDAP_SUCCESS;
}

