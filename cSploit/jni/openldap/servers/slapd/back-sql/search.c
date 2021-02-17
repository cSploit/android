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
#include "ac/ctype.h"

#include "lutil.h"
#include "slap.h"
#include "proto-sql.h"

static int backsql_process_filter( backsql_srch_info *bsi, Filter *f );
static int backsql_process_filter_eq( backsql_srch_info *bsi, 
		backsql_at_map_rec *at,
		int casefold, struct berval *filter_value );
static int backsql_process_filter_like( backsql_srch_info *bsi, 
		backsql_at_map_rec *at,
		int casefold, struct berval *filter_value );
static int backsql_process_filter_attr( backsql_srch_info *bsi, Filter *f, 
		backsql_at_map_rec *at );

/* For LDAP_CONTROL_PAGEDRESULTS, a 32 bit cookie is available to keep track of
   the state of paged results. The ldap_entries.id and oc_map_id values of the
   last entry returned are used as the cookie, so 6 bits are used for the OC id
   and the other 26 for ldap_entries ID number. If your max(oc_map_id) is more
   than 63, you will need to steal more bits from ldap_entries ID number and
   put them into the OC ID part of the cookie. */

/* NOTE: not supported when BACKSQL_ARBITRARY_KEY is defined */
#ifndef BACKSQL_ARBITRARY_KEY
#define SQL_TO_PAGECOOKIE(id, oc) (((id) << 6 ) | ((oc) & 0x3F))
#define PAGECOOKIE_TO_SQL_ID(pc) ((pc) >> 6)
#define PAGECOOKIE_TO_SQL_OC(pc) ((pc) & 0x3F)

static int parse_paged_cookie( Operation *op, SlapReply *rs );

static void send_paged_response( 
	Operation *op,
	SlapReply *rs,
	ID  *lastid );
#endif /* ! BACKSQL_ARBITRARY_KEY */

static int
backsql_attrlist_add( backsql_srch_info *bsi, AttributeDescription *ad )
{
	int 		n_attrs = 0;
	AttributeName	*an = NULL;

	if ( bsi->bsi_attrs == NULL ) {
		return 1;
	}

	/*
	 * clear the list (retrieve all attrs)
	 */
	if ( ad == NULL ) {
		bsi->bsi_op->o_tmpfree( bsi->bsi_attrs, bsi->bsi_op->o_tmpmemctx );
		bsi->bsi_attrs = NULL;
		bsi->bsi_flags |= BSQL_SF_ALL_ATTRS;
		return 1;
	}

	/* strip ';binary' */
	if ( slap_ad_is_binary( ad ) ) {
		ad = ad->ad_type->sat_ad;
	}

	for ( ; !BER_BVISNULL( &bsi->bsi_attrs[ n_attrs ].an_name ); n_attrs++ ) {
		an = &bsi->bsi_attrs[ n_attrs ];
		
		Debug( LDAP_DEBUG_TRACE, "==>backsql_attrlist_add(): "
			"attribute \"%s\" is in list\n", 
			an->an_name.bv_val, 0, 0 );
		/*
		 * We can live with strcmp because the attribute 
		 * list has been normalized before calling be_search
		 */
		if ( !BACKSQL_NCMP( &an->an_name, &ad->ad_cname ) ) {
			return 1;
		}
	}
	
	Debug( LDAP_DEBUG_TRACE, "==>backsql_attrlist_add(): "
		"adding \"%s\" to list\n", ad->ad_cname.bv_val, 0, 0 );

	an = (AttributeName *)bsi->bsi_op->o_tmprealloc( bsi->bsi_attrs,
			sizeof( AttributeName ) * ( n_attrs + 2 ),
			bsi->bsi_op->o_tmpmemctx );
	if ( an == NULL ) {
		return -1;
	}

	an[ n_attrs ].an_name = ad->ad_cname;
	an[ n_attrs ].an_desc = ad;
	BER_BVZERO( &an[ n_attrs + 1 ].an_name );

	bsi->bsi_attrs = an;
	
	return 1;
}

/*
 * Initializes the search structure.
 * 
 * If get_base_id != 0, the field bsi_base_id is filled 
 * with the entryID of bsi_base_ndn; it must be freed
 * by backsql_free_entryID() when no longer required.
 *
 * NOTE: base must be normalized
 */
int
backsql_init_search(
	backsql_srch_info 	*bsi, 
	struct berval		*nbase, 
	int 			scope, 
	time_t 			stoptime, 
	Filter 			*filter, 
	SQLHDBC 		dbh,
	Operation 		*op,
	SlapReply		*rs,
	AttributeName 		*attrs,
	unsigned		flags )
{
	backsql_info		*bi = (backsql_info *)op->o_bd->be_private;
	int			rc = LDAP_SUCCESS;

	bsi->bsi_base_ndn = nbase;
	bsi->bsi_use_subtree_shortcut = 0;
	BER_BVZERO( &bsi->bsi_base_id.eid_dn );
	BER_BVZERO( &bsi->bsi_base_id.eid_ndn );
	bsi->bsi_scope = scope;
	bsi->bsi_filter = filter;
	bsi->bsi_dbh = dbh;
	bsi->bsi_op = op;
	bsi->bsi_rs = rs;
	bsi->bsi_flags = BSQL_SF_NONE;

	bsi->bsi_attrs = NULL;

	if ( BACKSQL_FETCH_ALL_ATTRS( bi ) ) {
		/*
		 * if requested, simply try to fetch all attributes
		 */
		bsi->bsi_flags |= BSQL_SF_ALL_ATTRS;

	} else {
		if ( BACKSQL_FETCH_ALL_USERATTRS( bi ) ) {
			bsi->bsi_flags |= BSQL_SF_ALL_USER;

		} else if ( BACKSQL_FETCH_ALL_OPATTRS( bi ) ) {
			bsi->bsi_flags |= BSQL_SF_ALL_OPER;
		}

		if ( attrs == NULL ) {
			/* NULL means all user attributes */
			bsi->bsi_flags |= BSQL_SF_ALL_USER;

		} else {
			AttributeName	*p;
			int		got_oc = 0;

			bsi->bsi_attrs = (AttributeName *)bsi->bsi_op->o_tmpalloc(
					sizeof( AttributeName ),
					bsi->bsi_op->o_tmpmemctx );
			BER_BVZERO( &bsi->bsi_attrs[ 0 ].an_name );
	
			for ( p = attrs; !BER_BVISNULL( &p->an_name ); p++ ) {
				if ( BACKSQL_NCMP( &p->an_name, slap_bv_all_user_attrs ) == 0 ) {
					/* handle "*" */
					bsi->bsi_flags |= BSQL_SF_ALL_USER;

					/* if all attrs are requested, there's
					 * no need to continue */
					if ( BSQL_ISF_ALL_ATTRS( bsi ) ) {
						bsi->bsi_op->o_tmpfree( bsi->bsi_attrs,
								bsi->bsi_op->o_tmpmemctx );
						bsi->bsi_attrs = NULL;
						break;
					}
					continue;

				} else if ( BACKSQL_NCMP( &p->an_name, slap_bv_all_operational_attrs ) == 0 ) {
					/* handle "+" */
					bsi->bsi_flags |= BSQL_SF_ALL_OPER;

					/* if all attrs are requested, there's
					 * no need to continue */
					if ( BSQL_ISF_ALL_ATTRS( bsi ) ) {
						bsi->bsi_op->o_tmpfree( bsi->bsi_attrs,
								bsi->bsi_op->o_tmpmemctx );
						bsi->bsi_attrs = NULL;
						break;
					}
					continue;

				} else if ( BACKSQL_NCMP( &p->an_name, slap_bv_no_attrs ) == 0 ) {
					/* ignore "1.1" */
					continue;

				} else if ( p->an_desc == slap_schema.si_ad_objectClass ) {
					got_oc = 1;
				}

				backsql_attrlist_add( bsi, p->an_desc );
			}

			if ( got_oc == 0 && !( bsi->bsi_flags & BSQL_SF_ALL_USER ) ) {
				/* add objectClass if not present,
				 * because it is required to understand
				 * if an entry is a referral, an alias 
				 * or so... */
				backsql_attrlist_add( bsi, slap_schema.si_ad_objectClass );
			}
		}

		if ( !BSQL_ISF_ALL_ATTRS( bsi ) && bi->sql_anlist ) {
			AttributeName	*p;
			
			/* use hints if available */
			for ( p = bi->sql_anlist; !BER_BVISNULL( &p->an_name ); p++ ) {
				if ( BACKSQL_NCMP( &p->an_name, slap_bv_all_user_attrs ) == 0 ) {
					/* handle "*" */
					bsi->bsi_flags |= BSQL_SF_ALL_USER;

					/* if all attrs are requested, there's
					 * no need to continue */
					if ( BSQL_ISF_ALL_ATTRS( bsi ) ) {
						bsi->bsi_op->o_tmpfree( bsi->bsi_attrs,
								bsi->bsi_op->o_tmpmemctx );
						bsi->bsi_attrs = NULL;
						break;
					}
					continue;

				} else if ( BACKSQL_NCMP( &p->an_name, slap_bv_all_operational_attrs ) == 0 ) {
					/* handle "+" */
					bsi->bsi_flags |= BSQL_SF_ALL_OPER;

					/* if all attrs are requested, there's
					 * no need to continue */
					if ( BSQL_ISF_ALL_ATTRS( bsi ) ) {
						bsi->bsi_op->o_tmpfree( bsi->bsi_attrs,
								bsi->bsi_op->o_tmpmemctx );
						bsi->bsi_attrs = NULL;
						break;
					}
					continue;
				}

				backsql_attrlist_add( bsi, p->an_desc );
			}

		}
	}

	bsi->bsi_id_list = NULL;
	bsi->bsi_id_listtail = &bsi->bsi_id_list;
	bsi->bsi_n_candidates = 0;
	bsi->bsi_stoptime = stoptime;
	BER_BVZERO( &bsi->bsi_sel.bb_val );
	bsi->bsi_sel.bb_len = 0;
	BER_BVZERO( &bsi->bsi_from.bb_val );
	bsi->bsi_from.bb_len = 0;
	BER_BVZERO( &bsi->bsi_join_where.bb_val );
	bsi->bsi_join_where.bb_len = 0;
	BER_BVZERO( &bsi->bsi_flt_where.bb_val );
	bsi->bsi_flt_where.bb_len = 0;
	bsi->bsi_filter_oc = NULL;

	if ( BACKSQL_IS_GET_ID( flags ) ) {
		int	matched = BACKSQL_IS_MATCHED( flags );
		int	getentry = BACKSQL_IS_GET_ENTRY( flags );
		int	gotit = 0;

		assert( op->o_bd->be_private != NULL );

		rc = backsql_dn2id( op, rs, dbh, nbase, &bsi->bsi_base_id,
				matched, 1 );

		/* the entry is collected either if requested for by getentry
		 * or if get noSuchObject and requested to climb the tree,
		 * so that a matchedDN or a referral can be returned */
		if ( ( rc == LDAP_NO_SUCH_OBJECT && matched ) || getentry ) {
			if ( !BER_BVISNULL( &bsi->bsi_base_id.eid_ndn ) ) {
				assert( bsi->bsi_e != NULL );
				
				if ( dn_match( nbase, &bsi->bsi_base_id.eid_ndn ) )
				{
					gotit = 1;
				}
			
				/*
				 * let's see if it is a referral and, in case, get it
				 */
				backsql_attrlist_add( bsi, slap_schema.si_ad_ref );
				rc = backsql_id2entry( bsi, &bsi->bsi_base_id );
				if ( rc == LDAP_SUCCESS ) {
					if ( is_entry_referral( bsi->bsi_e ) )
					{
						BerVarray erefs = get_entry_referrals( op, bsi->bsi_e );
						if ( erefs ) {
							rc = rs->sr_err = LDAP_REFERRAL;
							rs->sr_ref = referral_rewrite( erefs,
									&bsi->bsi_e->e_nname,
									&op->o_req_dn,
									scope );
							ber_bvarray_free( erefs );
	
						} else {
							rc = rs->sr_err = LDAP_OTHER;
							rs->sr_text = "bad referral object";
						}

					} else if ( !gotit ) {
						rc = rs->sr_err = LDAP_NO_SUCH_OBJECT;
					}
				}

			} else {
				rs->sr_err = rc;
			}
		}

		if ( gotit && BACKSQL_IS_GET_OC( flags ) ) {
			bsi->bsi_base_id.eid_oc = backsql_id2oc( bi,
				bsi->bsi_base_id.eid_oc_id );
			if ( bsi->bsi_base_id.eid_oc == NULL ) {
				/* error? */
				backsql_free_entryID( &bsi->bsi_base_id, 1,
					op->o_tmpmemctx );
				rc = rs->sr_err = LDAP_OTHER;
			}
		}
	}

	bsi->bsi_status = rc;

	switch ( rc ) {
	case LDAP_SUCCESS:
	case LDAP_REFERRAL:
		break;

	default:
		bsi->bsi_op->o_tmpfree( bsi->bsi_attrs,
				bsi->bsi_op->o_tmpmemctx );
		break;
	}

	return rc;
}

static int
backsql_process_filter_list( backsql_srch_info *bsi, Filter *f, int op )
{
	int		res;

	if ( !f ) {
		return 0;
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx, "c", '(' /* ) */  );

	while ( 1 ) {
		res = backsql_process_filter( bsi, f );
		if ( res < 0 ) {
			/*
			 * TimesTen : If the query has no answers,
			 * don't bother to run the query.
			 */
			return -1;
		}
 
		f = f->f_next;
		if ( f == NULL ) {
			break;
		}

		switch ( op ) {
		case LDAP_FILTER_AND:
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx, "l",
					(ber_len_t)STRLENOF( " AND " ), 
						" AND " );
			break;

		case LDAP_FILTER_OR:
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx, "l",
					(ber_len_t)STRLENOF( " OR " ),
						" OR " );
			break;
		}
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx, "c", /* ( */ ')' );

	return 1;
}

static int
backsql_process_sub_filter( backsql_srch_info *bsi, Filter *f,
	backsql_at_map_rec *at )
{
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	int			i;
	int			casefold = 0;

	if ( !f ) {
		return 0;
	}

	/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
	if ( f->f_sub_desc->ad_type->sat_substr &&
			SLAP_MR_ASSOCIATED( f->f_sub_desc->ad_type->sat_substr,
				bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
	{
		casefold = 1;
	}

	if ( f->f_sub_desc->ad_type->sat_substr &&
			SLAP_MR_ASSOCIATED( f->f_sub_desc->ad_type->sat_substr,
				bi->sql_telephoneNumberMatch ) )
	{

		struct berval	bv;
		ber_len_t	i, s, a;

		/*
		 * to check for matching telephone numbers
		 * with intermixed chars, e.g. val='1234'
		 * use
		 * 
		 * val LIKE '%1%2%3%4%'
		 */

		BER_BVZERO( &bv );
		if ( f->f_sub_initial.bv_val ) {
			bv.bv_len += f->f_sub_initial.bv_len;
		}
		if ( f->f_sub_any != NULL ) {
			for ( a = 0; f->f_sub_any[ a ].bv_val != NULL; a++ ) {
				bv.bv_len += f->f_sub_any[ a ].bv_len;
			}
		}
		if ( f->f_sub_final.bv_val ) {
			bv.bv_len += f->f_sub_final.bv_len;
		}
		bv.bv_len = 2 * bv.bv_len - 1;
		bv.bv_val = ch_malloc( bv.bv_len + 1 );

		s = 0;
		if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
			bv.bv_val[ s ] = f->f_sub_initial.bv_val[ 0 ];
			for ( i = 1; i < f->f_sub_initial.bv_len; i++ ) {
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				bv.bv_val[ s + 2 * i ] = f->f_sub_initial.bv_val[ i ];
			}
			bv.bv_val[ s + 2 * i - 1 ] = '%';
			s += 2 * i;
		}

		if ( f->f_sub_any != NULL ) {
			for ( a = 0; !BER_BVISNULL( &f->f_sub_any[ a ] ); a++ ) {
				bv.bv_val[ s ] = f->f_sub_any[ a ].bv_val[ 0 ];
				for ( i = 1; i < f->f_sub_any[ a ].bv_len; i++ ) {
					bv.bv_val[ s + 2 * i - 1 ] = '%';
					bv.bv_val[ s + 2 * i ] = f->f_sub_any[ a ].bv_val[ i ];
				}
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				s += 2 * i;
			}
		}

		if ( !BER_BVISNULL( &f->f_sub_final ) ) {
			bv.bv_val[ s ] = f->f_sub_final.bv_val[ 0 ];
			for ( i = 1; i < f->f_sub_final.bv_len; i++ ) {
				bv.bv_val[ s + 2 * i - 1 ] = '%';
				bv.bv_val[ s + 2 * i ] = f->f_sub_final.bv_val[ i ];
			}
				bv.bv_val[ s + 2 * i - 1 ] = '%';
			s += 2 * i;
		}

		bv.bv_val[ s - 1 ] = '\0';

		(void)backsql_process_filter_like( bsi, at, casefold, &bv );
		ch_free( bv.bv_val );

		return 1;
	}

	/*
	 * When dealing with case-sensitive strings 
	 * we may omit normalization; however, normalized
	 * SQL filters are more liberal.
	 */

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx, "c", '(' /* ) */  );

	/* TimesTen */
	Debug( LDAP_DEBUG_TRACE, "backsql_process_sub_filter(%s):\n",
		at->bam_ad->ad_cname.bv_val, 0, 0 );
	Debug(LDAP_DEBUG_TRACE, "   expr: '%s%s%s'\n", at->bam_sel_expr.bv_val,
		at->bam_sel_expr_u.bv_val ? "' '" : "",
		at->bam_sel_expr_u.bv_val ? at->bam_sel_expr_u.bv_val : "" );
	if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
		/*
		 * If a pre-upper-cased version of the column 
		 * or a precompiled upper function exists, use it
		 */
		backsql_strfcat_x( &bsi->bsi_flt_where, 
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				&at->bam_sel_expr_u,
				(ber_len_t)STRLENOF( " LIKE '" ),
					" LIKE '" );

	} else {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				&at->bam_sel_expr,
				(ber_len_t)STRLENOF( " LIKE '" ), " LIKE '" );
	}
 
	if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
		ber_len_t	start;

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, 
			"==>backsql_process_sub_filter(%s): "
			"sub_initial=\"%s\"\n", at->bam_ad->ad_cname.bv_val,
			f->f_sub_initial.bv_val, 0 );
#endif /* BACKSQL_TRACE */

		start = bsi->bsi_flt_where.bb_val.bv_len;
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"b",
				&f->f_sub_initial );
		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		}
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"c", '%' );

	if ( f->f_sub_any != NULL ) {
		for ( i = 0; !BER_BVISNULL( &f->f_sub_any[ i ] ); i++ ) {
			ber_len_t	start;

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, 
				"==>backsql_process_sub_filter(%s): "
				"sub_any[%d]=\"%s\"\n", at->bam_ad->ad_cname.bv_val, 
				i, f->f_sub_any[ i ].bv_val );
#endif /* BACKSQL_TRACE */

			start = bsi->bsi_flt_where.bb_val.bv_len;
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bc",
					&f->f_sub_any[ i ],
					'%' );
			if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
				/*
				 * Note: toupper('%') = '%'
				 */
				ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
			}
		}
	}

	if ( !BER_BVISNULL( &f->f_sub_final ) ) {
		ber_len_t	start;

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, 
			"==>backsql_process_sub_filter(%s): "
			"sub_final=\"%s\"\n", at->bam_ad->ad_cname.bv_val,
			f->f_sub_final.bv_val, 0 );
#endif /* BACKSQL_TRACE */

		start = bsi->bsi_flt_where.bb_val.bv_len;
    		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"b",
				&f->f_sub_final );
  		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		}
	}

	backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"l", 
			(ber_len_t)STRLENOF( /* (' */ "')" ), /* (' */ "')" );
 
	return 1;
}

static int
backsql_merge_from_tbls( backsql_srch_info *bsi, struct berval *from_tbls )
{
	if ( BER_BVISNULL( from_tbls ) ) {
		return LDAP_SUCCESS;
	}

	if ( !BER_BVISNULL( &bsi->bsi_from.bb_val ) ) {
		char		*start, *end;
		struct berval	tmp;

		ber_dupbv_x( &tmp, from_tbls, bsi->bsi_op->o_tmpmemctx );

		for ( start = tmp.bv_val, end = strchr( start, ',' ); start; ) {
			if ( end ) {
				end[0] = '\0';
			}

			if ( strstr( bsi->bsi_from.bb_val.bv_val, start) == NULL )
			{
				backsql_strfcat_x( &bsi->bsi_from,
						bsi->bsi_op->o_tmpmemctx,
						"cs", ',', start );
			}

			if ( end ) {
				/* in case there are spaces after the comma... */
				for ( start = &end[1]; isspace( start[0] ); start++ );
				if ( start[0] ) {
					end = strchr( start, ',' );
				} else {
					start = NULL;
				}
			} else {
				start = NULL;
			}
		}

		bsi->bsi_op->o_tmpfree( tmp.bv_val, bsi->bsi_op->o_tmpmemctx );

	} else {
		backsql_strfcat_x( &bsi->bsi_from,
				bsi->bsi_op->o_tmpmemctx,
				"b", from_tbls );
	}

	return LDAP_SUCCESS;
}

static int
backsql_process_filter( backsql_srch_info *bsi, Filter *f )
{
	backsql_at_map_rec	**vat = NULL;
	AttributeDescription	*ad = NULL;
	unsigned		i;
	int 			done = 0;
	int			rc = 0;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_process_filter()\n", 0, 0, 0 );
	if ( f->f_choice == SLAPD_FILTER_COMPUTED ) {
		struct berval	flt;
		char		*msg = NULL;

		switch ( f->f_result ) {
		case LDAP_COMPARE_TRUE:
			BER_BVSTR( &flt, "10=10" );
			msg = "TRUE";
			break;

		case LDAP_COMPARE_FALSE:
			BER_BVSTR( &flt, "11=0" );
			msg = "FALSE";
			break;

		case SLAPD_COMPARE_UNDEFINED:
			BER_BVSTR( &flt, "12=0" );
			msg = "UNDEFINED";
			break;

		default:
			rc = -1;
			goto done;
		}

		Debug( LDAP_DEBUG_TRACE, "backsql_process_filter(): "
			"filter computed (%s)\n", msg, 0, 0 );
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx, "b", &flt );
		rc = 1;
		goto done;
	}

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"l",
			(ber_len_t)STRLENOF( "1=0" ), "1=0" );
		done = 1;
		rc = 1;
		goto done;
	}

	switch( f->f_choice ) {
	case LDAP_FILTER_OR:
		rc = backsql_process_filter_list( bsi, f->f_or, 
				LDAP_FILTER_OR );
		done = 1;
		break;
		
	case LDAP_FILTER_AND:
		rc = backsql_process_filter_list( bsi, f->f_and,
				LDAP_FILTER_AND );
		done = 1;
		break;

	case LDAP_FILTER_NOT:
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "NOT (" /* ) */ ),
					"NOT (" /* ) */ );
		rc = backsql_process_filter( bsi, f->f_not );
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"c", /* ( */ ')' );
		done = 1;
		break;

	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		break;
		
	case LDAP_FILTER_EXT:
		ad = f->f_mra->ma_desc;
		if ( f->f_mr_dnattrs ) {
			/*
			 * if dn attrs filtering is requested, better return 
			 * success and let test_filter() deal with candidate
			 * selection; otherwise we'd need to set conditions
			 * on the contents of the DN, e.g. "SELECT ... FROM
			 * ldap_entries AS attributeName WHERE attributeName.dn
			 * like '%attributeName=value%'"
			 */
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "1=1" ), "1=1" );
			bsi->bsi_status = LDAP_SUCCESS;
			rc = 1;
			goto done;
		}
		break;
		
	default:
		ad = f->f_av_desc;
		break;
	}

	if ( rc == -1 ) {
		goto done;
	}
 
	if ( done ) {
		rc = 1;
		goto done;
	}

	/*
	 * Turn structuralObjectClass into objectClass
	 */
	if ( ad == slap_schema.si_ad_objectClass 
			|| ad == slap_schema.si_ad_structuralObjectClass )
	{
		/*
		 * If the filter is LDAP_FILTER_PRESENT, then it's done;
		 * otherwise, let's see if we are lucky: filtering
		 * for "structural" objectclass or ancestor...
		 */
		switch ( f->f_choice ) {
		case LDAP_FILTER_EQUALITY:
		{
			ObjectClass	*oc = oc_bvfind( &f->f_av_value );

			if ( oc == NULL ) {
				Debug( LDAP_DEBUG_TRACE,
						"backsql_process_filter(): "
						"unknown objectClass \"%s\" "
						"in filter\n",
						f->f_av_value.bv_val, 0, 0 );
				bsi->bsi_status = LDAP_OTHER;
				rc = -1;
				goto done;
			}

			/*
			 * "structural" objectClass inheritance:
			 * - a search for "person" will also return 
			 *   "inetOrgPerson"
			 * - a search for "top" will return everything
			 */
			if ( is_object_subclass( oc, bsi->bsi_oc->bom_oc ) ) {
				static struct berval ldap_entry_objclasses = BER_BVC( "ldap_entry_objclasses" );

				backsql_merge_from_tbls( bsi, &ldap_entry_objclasses );

				backsql_strfcat_x( &bsi->bsi_flt_where,
						bsi->bsi_op->o_tmpmemctx,
						"lbl",
						(ber_len_t)STRLENOF( "(2=2 OR (ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ')) */ ),
							"(2=2 OR (ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ')) */,
						&bsi->bsi_oc->bom_oc->soc_cname,
						(ber_len_t)STRLENOF( /* ((' */ "'))" ),
							/* ((' */ "'))" );
				bsi->bsi_status = LDAP_SUCCESS;
				rc = 1;
				goto done;
			}

			break;
		}

		case LDAP_FILTER_PRESENT:
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "3=3" ), "3=3" );
			bsi->bsi_status = LDAP_SUCCESS;
			rc = 1;
			goto done;

			/* FIXME: LDAP_FILTER_EXT? */
			
		default:
			Debug( LDAP_DEBUG_TRACE,
					"backsql_process_filter(): "
					"illegal/unhandled filter "
					"on objectClass attribute",
					0, 0, 0 );
			bsi->bsi_status = LDAP_OTHER;
			rc = -1;
			goto done;
		}

	} else if ( ad == slap_schema.si_ad_entryUUID ) {
		unsigned long	oc_id;
#ifdef BACKSQL_ARBITRARY_KEY
		struct berval	keyval;
#else /* ! BACKSQL_ARBITRARY_KEY */
		unsigned long	keyval;
		char		keyvalbuf[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
#endif /* ! BACKSQL_ARBITRARY_KEY */

		switch ( f->f_choice ) {
		case LDAP_FILTER_EQUALITY:
			backsql_entryUUID_decode( &f->f_av_value, &oc_id, &keyval );

			if ( oc_id != bsi->bsi_oc->bom_id ) {
				bsi->bsi_status = LDAP_SUCCESS;
				rc = -1;
				goto done;
			}

#ifdef BACKSQL_ARBITRARY_KEY
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bcblbc",
					&bsi->bsi_oc->bom_keytbl, '.',
					&bsi->bsi_oc->bom_keycol,
					STRLENOF( " LIKE '" ), " LIKE '",
					&keyval, '\'' );
#else /* ! BACKSQL_ARBITRARY_KEY */
			snprintf( keyvalbuf, sizeof( keyvalbuf ), "%lu", keyval );
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bcbcs",
					&bsi->bsi_oc->bom_keytbl, '.',
					&bsi->bsi_oc->bom_keycol, '=', keyvalbuf );
#endif /* ! BACKSQL_ARBITRARY_KEY */
			break;

		case LDAP_FILTER_PRESENT:
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "4=4" ), "4=4" );
			break;

		default:
			rc = -1;
			goto done;
		}

		bsi->bsi_flags |= BSQL_SF_FILTER_ENTRYUUID;
		rc = 1;
		goto done;

#ifdef BACKSQL_SYNCPROV
	} else if ( ad == slap_schema.si_ad_entryCSN ) {
		/*
		 * support for syncrepl as provider...
		 */
#if 0
		if ( !bsi->bsi_op->o_sync ) {
			/* unsupported at present... */
			bsi->bsi_status = LDAP_OTHER;
			rc = -1;
			goto done;
		}
#endif

		bsi->bsi_flags |= ( BSQL_SF_FILTER_ENTRYCSN | BSQL_SF_RETURN_ENTRYUUID);

		/* if doing a syncrepl, try to return as much as possible,
		 * and always match the filter */
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "5=5" ), "5=5" );

		/* save for later use in operational attributes */
		/* FIXME: saves only the first occurrence, because 
		 * the filter during updates is written as
		 * "(&(entryCSN<={contextCSN})(entryCSN>={oldContextCSN})({filter}))"
		 * so we want our fake entryCSN to match the greatest
		 * value
		 */
		if ( bsi->bsi_op->o_private == NULL ) {
			bsi->bsi_op->o_private = &f->f_av_value;
		}
		bsi->bsi_status = LDAP_SUCCESS;

		rc = 1;
		goto done;
#endif /* BACKSQL_SYNCPROV */

	} else if ( ad == slap_schema.si_ad_hasSubordinates || ad == NULL ) {
		/*
		 * FIXME: this is not robust; e.g. a filter
		 * '(!(hasSubordinates=TRUE))' fails because
		 * in SQL it would read 'NOT (1=1)' instead 
		 * of no condition.  
		 * Note however that hasSubordinates is boolean, 
		 * so a more appropriate filter would be 
		 * '(hasSubordinates=FALSE)'
		 *
		 * A more robust search for hasSubordinates
		 * would * require joining the ldap_entries table
		 * selecting if there are descendants of the
		 * candidate.
		 */
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "6=6" ), "6=6" );
		if ( ad == slap_schema.si_ad_hasSubordinates ) {
			/*
			 * instruct candidate selection algorithm
			 * and attribute list to try to detect
			 * if an entry has subordinates
			 */
			bsi->bsi_flags |= BSQL_SF_FILTER_HASSUBORDINATE;

		} else {
			/*
			 * clear attributes to fetch, to require ALL
			 * and try extended match on all attributes
			 */
			backsql_attrlist_add( bsi, NULL );
		}
		rc = 1;
		goto done;
	}

	/*
	 * attribute inheritance:
	 */
	if ( backsql_supad2at( bsi->bsi_oc, ad, &vat ) ) {
		bsi->bsi_status = LDAP_OTHER;
		rc = -1;
		goto done;
	}

	if ( vat == NULL ) {
		/* search anyway; other parts of the filter
		 * may succeeed */
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "7=7" ), "7=7" );
		bsi->bsi_status = LDAP_SUCCESS;
		rc = 1;
		goto done;
	}

	/* if required, open extra level of parens */
	done = 0;
	if ( vat[0]->bam_next || vat[1] ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"c", '(' );
		done = 1;
	}

	i = 0;
next:;
	/* apply attr */
	if ( backsql_process_filter_attr( bsi, f, vat[i] ) == -1 ) {
		return -1;
	}

	/* if more definitions of the same attr, apply */
	if ( vat[i]->bam_next ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
			STRLENOF( " OR " ), " OR " );
		vat[i] = vat[i]->bam_next;
		goto next;
	}

	/* if more descendants of the same attr, apply */
	i++;
	if ( vat[i] ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
			STRLENOF( " OR " ), " OR " );
		goto next;
	}

	/* if needed, close extra level of parens */
	if ( done ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"c", ')' );
	}

	rc = 1;

done:;
	if ( vat ) {
		ch_free( vat );
	}

	Debug( LDAP_DEBUG_TRACE,
			"<==backsql_process_filter() %s\n",
			rc == 1 ? "succeeded" : "failed", 0, 0);

	return rc;
}

static int
backsql_process_filter_eq( backsql_srch_info *bsi, backsql_at_map_rec *at,
		int casefold, struct berval *filter_value )
{
	/*
	 * maybe we should check type of at->sel_expr here somehow,
	 * to know whether upper_func is applicable, but for now
	 * upper_func stuff is made for Oracle, where UPPER is
	 * safely applicable to NUMBER etc.
	 */
	if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
		ber_len_t	start;

		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"cbl",
				'(', /* ) */
				&at->bam_sel_expr_u, 
				(ber_len_t)STRLENOF( "='" ),
					"='" );

		start = bsi->bsi_flt_where.bb_val.bv_len;

		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				filter_value, 
				(ber_len_t)STRLENOF( /* (' */ "')" ),
					/* (' */ "')" );

		ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );

	} else {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"cblbl",
				'(', /* ) */
				&at->bam_sel_expr,
				(ber_len_t)STRLENOF( "='" ), "='",
				filter_value,
				(ber_len_t)STRLENOF( /* (' */ "')" ),
					/* (' */ "')" );
	}

	return 1;
}
	
static int
backsql_process_filter_like( backsql_srch_info *bsi, backsql_at_map_rec *at,
		int casefold, struct berval *filter_value )
{
	/*
	 * maybe we should check type of at->sel_expr here somehow,
	 * to know whether upper_func is applicable, but for now
	 * upper_func stuff is made for Oracle, where UPPER is
	 * safely applicable to NUMBER etc.
	 */
	if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
		ber_len_t	start;

		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"cbl",
				'(', /* ) */
				&at->bam_sel_expr_u, 
				(ber_len_t)STRLENOF( " LIKE '%" ),
					" LIKE '%" );

		start = bsi->bsi_flt_where.bb_val.bv_len;

		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"bl",
				filter_value, 
				(ber_len_t)STRLENOF( /* (' */ "%')" ),
					/* (' */ "%')" );

		ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );

	} else {
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"cblbl",
				'(', /* ) */
				&at->bam_sel_expr,
				(ber_len_t)STRLENOF( " LIKE '%" ),
					" LIKE '%",
				filter_value,
				(ber_len_t)STRLENOF( /* (' */ "%')" ),
					/* (' */ "%')" );
	}

	return 1;
}

static int
backsql_process_filter_attr( backsql_srch_info *bsi, Filter *f, backsql_at_map_rec *at )
{
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	int			casefold = 0;
	struct berval		*filter_value = NULL;
	MatchingRule		*matching_rule = NULL;
	struct berval		ordering = BER_BVC("<=");

	Debug( LDAP_DEBUG_TRACE, "==>backsql_process_filter_attr(%s)\n",
		at->bam_ad->ad_cname.bv_val, 0, 0 );

	/*
	 * need to add this attribute to list of attrs to load,
	 * so that we can do test_filter() later
	 */
	backsql_attrlist_add( bsi, at->bam_ad );

	backsql_merge_from_tbls( bsi, &at->bam_from_tbls );

	if ( !BER_BVISNULL( &at->bam_join_where )
			&& strstr( bsi->bsi_join_where.bb_val.bv_val,
				at->bam_join_where.bv_val ) == NULL )
	{
	       	backsql_strfcat_x( &bsi->bsi_join_where,
				bsi->bsi_op->o_tmpmemctx,
				"lb",
				(ber_len_t)STRLENOF( " AND " ), " AND ",
				&at->bam_join_where );
	}

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		backsql_strfcat_x( &bsi->bsi_flt_where,
			bsi->bsi_op->o_tmpmemctx,
			"l",
			(ber_len_t)STRLENOF( "1=0" ), "1=0" );
		return 1;
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		filter_value = &f->f_av_value;
		matching_rule = at->bam_ad->ad_type->sat_equality;

		goto equality_match;

		/* fail over into next case */
		
	case LDAP_FILTER_EXT:
		filter_value = &f->f_mra->ma_value;
		matching_rule = f->f_mr_rule;

equality_match:;
		/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
		if ( SLAP_MR_ASSOCIATED( matching_rule,
					bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
		{
			casefold = 1;
		}

		/* FIXME: directoryString filtering should use a similar
		 * approach to deal with non-prettified values like
		 * " A  non    prettified   value  ", by using a LIKE
		 * filter with all whitespaces collapsed to a single '%' */
		if ( SLAP_MR_ASSOCIATED( matching_rule,
					bi->sql_telephoneNumberMatch ) )
		{
			struct berval	bv;
			ber_len_t	i;

			/*
			 * to check for matching telephone numbers
			 * with intermized chars, e.g. val='1234'
			 * use
			 * 
			 * val LIKE '%1%2%3%4%'
			 */

			bv.bv_len = 2 * filter_value->bv_len - 1;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );

			bv.bv_val[ 0 ] = filter_value->bv_val[ 0 ];
			for ( i = 1; i < filter_value->bv_len; i++ ) {
				bv.bv_val[ 2 * i - 1 ] = '%';
				bv.bv_val[ 2 * i ] = filter_value->bv_val[ i ];
			}
			bv.bv_val[ 2 * i - 1 ] = '\0';

			(void)backsql_process_filter_like( bsi, at, casefold, &bv );
			ch_free( bv.bv_val );

			break;
		}

		/* NOTE: this is required by objectClass inheritance 
		 * and auxiliary objectClass use in filters for slightly
		 * more efficient candidate selection. */
		/* FIXME: a bit too many specializations to deal with
		 * very specific cases... */
		if ( at->bam_ad == slap_schema.si_ad_objectClass
				|| at->bam_ad == slap_schema.si_ad_structuralObjectClass )
		{
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"lbl",
					(ber_len_t)STRLENOF( "(ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ') */ ),
						"(ldap_entries.id=ldap_entry_objclasses.entry_id AND ldap_entry_objclasses.oc_name='" /* ') */,
					filter_value,
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* (' */ "')" );
			break;
		}

		/*
		 * maybe we should check type of at->sel_expr here somehow,
		 * to know whether upper_func is applicable, but for now
		 * upper_func stuff is made for Oracle, where UPPER is
		 * safely applicable to NUMBER etc.
		 */
		(void)backsql_process_filter_eq( bsi, at, casefold, filter_value );
		break;

	case LDAP_FILTER_GE:
		ordering.bv_val = ">=";

		/* fall thru to next case */
		
	case LDAP_FILTER_LE:
		filter_value = &f->f_av_value;
		
		/* always uppercase strings by now */
#ifdef BACKSQL_UPPERCASE_FILTER
		if ( at->bam_ad->ad_type->sat_ordering &&
				SLAP_MR_ASSOCIATED( at->bam_ad->ad_type->sat_ordering,
					bi->sql_caseIgnoreMatch ) )
#endif /* BACKSQL_UPPERCASE_FILTER */
		{
			casefold = 1;
		}

		/*
		 * FIXME: should we uppercase the operands?
		 */
		if ( casefold && BACKSQL_AT_CANUPPERCASE( at ) ) {
			ber_len_t	start;

			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"cbbc",
					'(', /* ) */
					&at->bam_sel_expr_u, 
					&ordering,
					'\'' );

			start = bsi->bsi_flt_where.bb_val.bv_len;

			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"bl",
					filter_value, 
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* (' */ "')" );

			ldap_pvt_str2upper( &bsi->bsi_flt_where.bb_val.bv_val[ start ] );
		
		} else {
			backsql_strfcat_x( &bsi->bsi_flt_where,
					bsi->bsi_op->o_tmpmemctx,
					"cbbcbl",
					'(' /* ) */ ,
					&at->bam_sel_expr,
					&ordering,
					'\'',
					&f->f_av_value,
					(ber_len_t)STRLENOF( /* (' */ "')" ),
						/* ( */ "')" );
		}
		break;

	case LDAP_FILTER_PRESENT:
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"lbl",
				(ber_len_t)STRLENOF( "NOT (" /* ) */),
					"NOT (", /* ) */
				&at->bam_sel_expr, 
				(ber_len_t)STRLENOF( /* ( */ " IS NULL)" ),
					/* ( */ " IS NULL)" );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		backsql_process_sub_filter( bsi, f, at );
		break;

	case LDAP_FILTER_APPROX:
		/* we do our best */

		/*
		 * maybe we should check type of at->sel_expr here somehow,
		 * to know whether upper_func is applicable, but for now
		 * upper_func stuff is made for Oracle, where UPPER is
		 * safely applicable to NUMBER etc.
		 */
		(void)backsql_process_filter_like( bsi, at, 1, &f->f_av_value );
		break;

	default:
		/* unhandled filter type; should not happen */
		assert( 0 );
		backsql_strfcat_x( &bsi->bsi_flt_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "8=8" ), "8=8" );
		break;

	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_process_filter_attr(%s)\n",
		at->bam_ad->ad_cname.bv_val, 0, 0 );

	return 1;
}

static int
backsql_srch_query( backsql_srch_info *bsi, struct berval *query )
{
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	int			rc;

	assert( query != NULL );
	BER_BVZERO( query );

	bsi->bsi_use_subtree_shortcut = 0;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_srch_query()\n", 0, 0, 0 );
	BER_BVZERO( &bsi->bsi_sel.bb_val );
	BER_BVZERO( &bsi->bsi_sel.bb_val );
	bsi->bsi_sel.bb_len = 0;
	BER_BVZERO( &bsi->bsi_from.bb_val );
	bsi->bsi_from.bb_len = 0;
	BER_BVZERO( &bsi->bsi_join_where.bb_val );
	bsi->bsi_join_where.bb_len = 0;
	BER_BVZERO( &bsi->bsi_flt_where.bb_val );
	bsi->bsi_flt_where.bb_len = 0;

	backsql_strfcat_x( &bsi->bsi_sel,
			bsi->bsi_op->o_tmpmemctx,
			"lbcbc",
			(ber_len_t)STRLENOF( "SELECT DISTINCT ldap_entries.id," ),
				"SELECT DISTINCT ldap_entries.id,", 
			&bsi->bsi_oc->bom_keytbl, 
			'.', 
			&bsi->bsi_oc->bom_keycol, 
			',' );

	if ( !BER_BVISNULL( &bi->sql_strcast_func ) ) {
		backsql_strfcat_x( &bsi->bsi_sel,
				bsi->bsi_op->o_tmpmemctx,
				"blbl",
				&bi->sql_strcast_func, 
				(ber_len_t)STRLENOF( "('" /* ') */ ),
					"('" /* ') */ ,
				&bsi->bsi_oc->bom_oc->soc_cname,
				(ber_len_t)STRLENOF( /* (' */ "')" ),
					/* (' */ "')" );
	} else {
		backsql_strfcat_x( &bsi->bsi_sel,
				bsi->bsi_op->o_tmpmemctx,
				"cbc",
				'\'',
				&bsi->bsi_oc->bom_oc->soc_cname,
				'\'' );
	}

	backsql_strfcat_x( &bsi->bsi_sel,
			bsi->bsi_op->o_tmpmemctx,
			"b",
			&bi->sql_dn_oc_aliasing );
	backsql_strfcat_x( &bsi->bsi_from,
			bsi->bsi_op->o_tmpmemctx,
			"lb",
			(ber_len_t)STRLENOF( " FROM ldap_entries," ),
				" FROM ldap_entries,",
			&bsi->bsi_oc->bom_keytbl );

	backsql_strfcat_x( &bsi->bsi_join_where,
			bsi->bsi_op->o_tmpmemctx,
			"lbcbl",
			(ber_len_t)STRLENOF( " WHERE " ), " WHERE ",
			&bsi->bsi_oc->bom_keytbl,
			'.',
			&bsi->bsi_oc->bom_keycol,
			(ber_len_t)STRLENOF( "=ldap_entries.keyval AND ldap_entries.oc_map_id=? AND " ),
				"=ldap_entries.keyval AND ldap_entries.oc_map_id=? AND " );

	switch ( bsi->bsi_scope ) {
	case LDAP_SCOPE_BASE:
		if ( BACKSQL_CANUPPERCASE( bi ) ) {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx, 
					"bl",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn)=?" ),
						"(ldap_entries.dn)=?" );
		} else {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "ldap_entries.dn=?" ),
						"ldap_entries.dn=?" );
		}
		break;
		
	case BACKSQL_SCOPE_BASE_LIKE:
		if ( BACKSQL_CANUPPERCASE( bi ) ) {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"bl",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn) LIKE ?" ),
						"(ldap_entries.dn) LIKE ?" );
		} else {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "ldap_entries.dn LIKE ?" ),
						"ldap_entries.dn LIKE ?" );
		}
		break;
		
	case LDAP_SCOPE_ONELEVEL:
		backsql_strfcat_x( &bsi->bsi_join_where,
				bsi->bsi_op->o_tmpmemctx,
				"l",
				(ber_len_t)STRLENOF( "ldap_entries.parent=?" ),
					"ldap_entries.parent=?" );
		break;

	case LDAP_SCOPE_SUBORDINATE:
	case LDAP_SCOPE_SUBTREE:
		if ( BACKSQL_USE_SUBTREE_SHORTCUT( bi ) ) {
			int		i;
			BackendDB	*bd = bsi->bsi_op->o_bd;

			assert( bd->be_nsuffix != NULL );

			for ( i = 0; !BER_BVISNULL( &bd->be_nsuffix[ i ] ); i++ )
			{
				if ( dn_match( &bd->be_nsuffix[ i ],
							bsi->bsi_base_ndn ) )
				{
					/* pass this to the candidate selection
					 * routine so that the DN is not bound
					 * to the select statement */
					bsi->bsi_use_subtree_shortcut = 1;
					break;
				}
			}
		}

		if ( bsi->bsi_use_subtree_shortcut ) {
			/* Skip the base DN filter, as every entry will match it */
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "9=9"), "9=9");

		} else if ( !BER_BVISNULL( &bi->sql_subtree_cond ) ) {
			/* This should always be true... */
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"b",
					&bi->sql_subtree_cond );

		} else if ( BACKSQL_CANUPPERCASE( bi ) ) {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"bl",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn) LIKE ?" ),
						"(ldap_entries.dn) LIKE ?"  );

		} else {
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)STRLENOF( "ldap_entries.dn LIKE ?" ),
						"ldap_entries.dn LIKE ?" );
		}

		break;

	default:
		assert( 0 );
	}

#ifndef BACKSQL_ARBITRARY_KEY
	/* If paged results are in effect, ignore low ldap_entries.id numbers */
	if ( get_pagedresults(bsi->bsi_op) > SLAP_CONTROL_IGNORED ) {
		unsigned long lowid = 0;

		/* Pick up the previous ldap_entries.id if the previous page ended in this objectClass */
		if ( bsi->bsi_oc->bom_id == PAGECOOKIE_TO_SQL_OC( ((PagedResultsState *)bsi->bsi_op->o_pagedresults_state)->ps_cookie ) )
		{
			lowid = PAGECOOKIE_TO_SQL_ID( ((PagedResultsState *)bsi->bsi_op->o_pagedresults_state)->ps_cookie );
		}

		if ( lowid ) {
			char lowidstring[48];
			int  lowidlen;

			lowidlen = snprintf( lowidstring, sizeof( lowidstring ),
				" AND ldap_entries.id>%lu", lowid );
			backsql_strfcat_x( &bsi->bsi_join_where,
					bsi->bsi_op->o_tmpmemctx,
					"l",
					(ber_len_t)lowidlen,
					lowidstring );
		}
	}
#endif /* ! BACKSQL_ARBITRARY_KEY */

	rc = backsql_process_filter( bsi, bsi->bsi_filter );
	if ( rc > 0 ) {
		struct berbuf	bb = BB_NULL;

		backsql_strfcat_x( &bb,
				bsi->bsi_op->o_tmpmemctx,
				"bbblb",
				&bsi->bsi_sel.bb_val,
				&bsi->bsi_from.bb_val, 
				&bsi->bsi_join_where.bb_val,
				(ber_len_t)STRLENOF( " AND " ), " AND ",
				&bsi->bsi_flt_where.bb_val );

		*query = bb.bb_val;

	} else if ( rc < 0 ) {
		/* 
		 * Indicates that there's no possible way the filter matches
		 * anything.  No need to issue the query
		 */
		free( query->bv_val );
		BER_BVZERO( query );
	}
 
	bsi->bsi_op->o_tmpfree( bsi->bsi_sel.bb_val.bv_val, bsi->bsi_op->o_tmpmemctx );
	BER_BVZERO( &bsi->bsi_sel.bb_val );
	bsi->bsi_sel.bb_len = 0;
	bsi->bsi_op->o_tmpfree( bsi->bsi_from.bb_val.bv_val, bsi->bsi_op->o_tmpmemctx );
	BER_BVZERO( &bsi->bsi_from.bb_val );
	bsi->bsi_from.bb_len = 0;
	bsi->bsi_op->o_tmpfree( bsi->bsi_join_where.bb_val.bv_val, bsi->bsi_op->o_tmpmemctx );
	BER_BVZERO( &bsi->bsi_join_where.bb_val );
	bsi->bsi_join_where.bb_len = 0;
	bsi->bsi_op->o_tmpfree( bsi->bsi_flt_where.bb_val.bv_val, bsi->bsi_op->o_tmpmemctx );
	BER_BVZERO( &bsi->bsi_flt_where.bb_val );
	bsi->bsi_flt_where.bb_len = 0;
	
	Debug( LDAP_DEBUG_TRACE, "<==backsql_srch_query() returns %s\n",
		query->bv_val ? query->bv_val : "NULL", 0, 0 );
	
	return ( rc <= 0 ? 1 : 0 );
}

static int
backsql_oc_get_candidates( void *v_oc, void *v_bsi )
{
	backsql_oc_map_rec	*oc = v_oc;
	backsql_srch_info	*bsi = v_bsi;
	Operation		*op = bsi->bsi_op;
	backsql_info		*bi = (backsql_info *)bsi->bsi_op->o_bd->be_private;
	struct berval		query;
	SQLHSTMT		sth = SQL_NULL_HSTMT;
	RETCODE			rc;
	int			res;
	BACKSQL_ROW_NTS		row;
	int			i;
	int			j;
	int			n_candidates = bsi->bsi_n_candidates;

	/* 
	 * + 1 because we need room for '%';
	 * + 1 because we need room for ',' for LDAP_SCOPE_SUBORDINATE;
	 * this makes a subtree
	 * search for a DN BACKSQL_MAX_DN_LEN long legal 
	 * if it returns that DN only
	 */
	char			tmp_base_ndn[ BACKSQL_MAX_DN_LEN + 1 + 1 ];

	bsi->bsi_status = LDAP_SUCCESS;
 
	Debug( LDAP_DEBUG_TRACE, "==>backsql_oc_get_candidates(): oc=\"%s\"\n",
			BACKSQL_OC_NAME( oc ), 0, 0 );

	/* check for abandon */
	if ( op->o_abandon ) {
		bsi->bsi_status = SLAPD_ABANDON;
		return BACKSQL_AVL_STOP;
	}

#ifndef BACKSQL_ARBITRARY_KEY
	/* If paged results have already completed this objectClass, skip it */
	if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
		if ( oc->bom_id < PAGECOOKIE_TO_SQL_OC( ((PagedResultsState *)op->o_pagedresults_state)->ps_cookie ) )
		{
			return BACKSQL_AVL_CONTINUE;
		}
	}
#endif /* ! BACKSQL_ARBITRARY_KEY */

	if ( bsi->bsi_n_candidates == -1 ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"unchecked limit has been overcome\n", 0, 0, 0 );
		/* should never get here */
		assert( 0 );
		bsi->bsi_status = LDAP_ADMINLIMIT_EXCEEDED;
		return BACKSQL_AVL_STOP;
	}
	
	bsi->bsi_oc = oc;
	res = backsql_srch_query( bsi, &query );
	if ( res ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"error while constructing query for objectclass \"%s\"\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		/*
		 * FIXME: need to separate errors from legally
		 * impossible filters
		 */
		switch ( bsi->bsi_status ) {
		case LDAP_SUCCESS:
		case LDAP_UNDEFINED_TYPE:
		case LDAP_NO_SUCH_OBJECT:
			/* we are conservative... */
		default:
			bsi->bsi_status = LDAP_SUCCESS;
			/* try next */
			return BACKSQL_AVL_CONTINUE;

		case LDAP_ADMINLIMIT_EXCEEDED:
		case LDAP_OTHER:
			/* don't try any more */
			return BACKSQL_AVL_STOP;
		}
	}

	if ( BER_BVISNULL( &query ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"could not construct query for objectclass \"%s\"\n",
			oc->bom_oc->soc_cname.bv_val, 0, 0 );
		bsi->bsi_status = LDAP_SUCCESS;
		return BACKSQL_AVL_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "Constructed query: %s\n", 
			query.bv_val, 0, 0 );

	rc = backsql_Prepare( bsi->bsi_dbh, &sth, query.bv_val, 0 );
	bsi->bsi_op->o_tmpfree( query.bv_val, bsi->bsi_op->o_tmpmemctx );
	BER_BVZERO( &query );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"error preparing query\n", 0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		bsi->bsi_status = LDAP_OTHER;
		return BACKSQL_AVL_CONTINUE;
	}
	
	Debug( LDAP_DEBUG_TRACE, "id: '" BACKSQL_IDNUMFMT "'\n",
		bsi->bsi_oc->bom_id, 0, 0 );

	rc = backsql_BindParamNumID( sth, 1, SQL_PARAM_INPUT,
			&bsi->bsi_oc->bom_id );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"error binding objectclass id parameter\n", 0, 0, 0 );
		bsi->bsi_status = LDAP_OTHER;
		return BACKSQL_AVL_CONTINUE;
	}

	switch ( bsi->bsi_scope ) {
	case LDAP_SCOPE_BASE:
	case BACKSQL_SCOPE_BASE_LIKE:
		/*
		 * We do not accept DNs longer than BACKSQL_MAX_DN_LEN;
		 * however this should be handled earlier
		 */
		if ( bsi->bsi_base_ndn->bv_len > BACKSQL_MAX_DN_LEN ) {
			bsi->bsi_status = LDAP_OTHER;
			return BACKSQL_AVL_CONTINUE;
		}

		AC_MEMCPY( tmp_base_ndn, bsi->bsi_base_ndn->bv_val,
				bsi->bsi_base_ndn->bv_len + 1 );

		/* uppercase DN only if the stored DN can be uppercased
		 * for comparison */
		if ( BACKSQL_CANUPPERCASE( bi ) ) {
			ldap_pvt_str2upper( tmp_base_ndn );
		}

		Debug( LDAP_DEBUG_TRACE, "(base)dn: \"%s\"\n",
				tmp_base_ndn, 0, 0 );

		rc = backsql_BindParamStr( sth, 2, SQL_PARAM_INPUT,
				tmp_base_ndn, BACKSQL_MAX_DN_LEN );
		if ( rc != SQL_SUCCESS ) {
         		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
				"error binding base_ndn parameter\n", 0, 0, 0 );
			backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, 
					sth, rc );
			bsi->bsi_status = LDAP_OTHER;
			return BACKSQL_AVL_CONTINUE;
		}
		break;

	case LDAP_SCOPE_SUBORDINATE:
	case LDAP_SCOPE_SUBTREE:
	{
		/* if short-cutting the search base,
		 * don't bind any parameter */
		if ( bsi->bsi_use_subtree_shortcut ) {
			break;
		}
		
		/*
		 * We do not accept DNs longer than BACKSQL_MAX_DN_LEN;
		 * however this should be handled earlier
		 */
		if ( bsi->bsi_base_ndn->bv_len > BACKSQL_MAX_DN_LEN ) {
			bsi->bsi_status = LDAP_OTHER;
			return BACKSQL_AVL_CONTINUE;
		}

		/* 
		 * Sets the parameters for the SQL built earlier
		 * NOTE that all the databases could actually use 
		 * the TimesTen version, which would be cleaner 
		 * and would also eliminate the need for the
		 * subtree_cond line in the configuration file.  
		 * For now, I'm leaving it the way it is, 
		 * so non-TimesTen databases use the original code.
		 * But at some point this should get cleaned up.
		 *
		 * If "dn" is being used, do a suffix search.
		 * If "dn_ru" is being used, do a prefix search.
		 */
		if ( BACKSQL_HAS_LDAPINFO_DN_RU( bi ) ) {
			tmp_base_ndn[ 0 ] = '\0';

			for ( i = 0, j = bsi->bsi_base_ndn->bv_len - 1;
					j >= 0; i++, j--) {
				tmp_base_ndn[ i ] = bsi->bsi_base_ndn->bv_val[ j ];
			}

			if ( bsi->bsi_scope == LDAP_SCOPE_SUBORDINATE ) {
				tmp_base_ndn[ i++ ] = ',';
			}

			tmp_base_ndn[ i ] = '%';
			tmp_base_ndn[ i + 1 ] = '\0';

		} else {
			i = 0;

			tmp_base_ndn[ i++ ] = '%';

			if ( bsi->bsi_scope == LDAP_SCOPE_SUBORDINATE ) {
				tmp_base_ndn[ i++ ] = ',';
			}

			AC_MEMCPY( &tmp_base_ndn[ i ], bsi->bsi_base_ndn->bv_val,
				bsi->bsi_base_ndn->bv_len + 1 );
		}

		/* uppercase DN only if the stored DN can be uppercased
		 * for comparison */
		if ( BACKSQL_CANUPPERCASE( bi ) ) {
			ldap_pvt_str2upper( tmp_base_ndn );
		}

		if ( bsi->bsi_scope == LDAP_SCOPE_SUBORDINATE ) {
			Debug( LDAP_DEBUG_TRACE, "(children)dn: \"%s\"\n",
				tmp_base_ndn, 0, 0 );
		} else {
			Debug( LDAP_DEBUG_TRACE, "(sub)dn: \"%s\"\n",
				tmp_base_ndn, 0, 0 );
		}

		rc = backsql_BindParamStr( sth, 2, SQL_PARAM_INPUT,
				tmp_base_ndn, BACKSQL_MAX_DN_LEN );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
				"error binding base_ndn parameter (2)\n",
				0, 0, 0 );
			backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, 
					sth, rc );
			bsi->bsi_status = LDAP_OTHER;
			return BACKSQL_AVL_CONTINUE;
		}
		break;
	}

 	case LDAP_SCOPE_ONELEVEL:
		assert( !BER_BVISNULL( &bsi->bsi_base_id.eid_ndn ) );

		Debug( LDAP_DEBUG_TRACE, "(one)id=" BACKSQL_IDFMT "\n",
			BACKSQL_IDARG(bsi->bsi_base_id.eid_id), 0, 0 );
		rc = backsql_BindParamID( sth, 2, SQL_PARAM_INPUT,
				&bsi->bsi_base_id.eid_id );
		if ( rc != SQL_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
				"error binding base id parameter\n", 0, 0, 0 );
			bsi->bsi_status = LDAP_OTHER;
			return BACKSQL_AVL_CONTINUE;
		}
		break;
	}
	
	rc = SQLExecute( sth );
	if ( !BACKSQL_SUCCESS( rc ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"error executing query\n", 0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, bsi->bsi_dbh, sth, rc );
		SQLFreeStmt( sth, SQL_DROP );
		bsi->bsi_status = LDAP_OTHER;
		return BACKSQL_AVL_CONTINUE;
	}

	backsql_BindRowAsStrings_x( sth, &row, bsi->bsi_op->o_tmpmemctx );
	rc = SQLFetch( sth );
	for ( ; BACKSQL_SUCCESS( rc ); rc = SQLFetch( sth ) ) {
		struct berval		dn, pdn, ndn;
		backsql_entryID		*c_id = NULL;
		int			ret;

		ber_str2bv( row.cols[ 3 ], 0, 0, &dn );

		if ( backsql_api_odbc2dn( bsi->bsi_op, bsi->bsi_rs, &dn ) ) {
			continue;
		}

		ret = dnPrettyNormal( NULL, &dn, &pdn, &ndn, op->o_tmpmemctx );
		if ( dn.bv_val != row.cols[ 3 ] ) {
			free( dn.bv_val );
		}

		if ( ret != LDAP_SUCCESS ) {
			continue;
		}

		if ( bi->sql_baseObject && dn_match( &ndn, &bi->sql_baseObject->e_nname ) ) {
			goto cleanup;
		}

		c_id = (backsql_entryID *)op->o_tmpcalloc( 1, 
				sizeof( backsql_entryID ), op->o_tmpmemctx );
#ifdef BACKSQL_ARBITRARY_KEY
		ber_str2bv_x( row.cols[ 0 ], 0, 1, &c_id->eid_id,
				op->o_tmpmemctx );
		ber_str2bv_x( row.cols[ 1 ], 0, 1, &c_id->eid_keyval,
				op->o_tmpmemctx );
#else /* ! BACKSQL_ARBITRARY_KEY */
		if ( BACKSQL_STR2ID( &c_id->eid_id, row.cols[ 0 ], 0 ) != 0 ) {
			goto cleanup;
		}
		if ( BACKSQL_STR2ID( &c_id->eid_keyval, row.cols[ 1 ], 0 ) != 0 ) {
			goto cleanup;
		}
#endif /* ! BACKSQL_ARBITRARY_KEY */
		c_id->eid_oc = bsi->bsi_oc;
		c_id->eid_oc_id = bsi->bsi_oc->bom_id;

		c_id->eid_dn = pdn;
		c_id->eid_ndn = ndn;

		/* append at end of list ... */
		c_id->eid_next = NULL;
		*bsi->bsi_id_listtail = c_id;
		bsi->bsi_id_listtail = &c_id->eid_next;

		Debug( LDAP_DEBUG_TRACE, "backsql_oc_get_candidates(): "
			"added entry id=" BACKSQL_IDFMT " keyval=" BACKSQL_IDFMT " dn=\"%s\"\n",
			BACKSQL_IDARG(c_id->eid_id),
			BACKSQL_IDARG(c_id->eid_keyval),
			row.cols[ 3 ] );

		/* count candidates, for unchecked limit */
		bsi->bsi_n_candidates--;
		if ( bsi->bsi_n_candidates == -1 ) {
			break;
		}
		continue;

cleanup:;
		if ( !BER_BVISNULL( &pdn ) ) {
			op->o_tmpfree( pdn.bv_val, op->o_tmpmemctx );
		}
		if ( !BER_BVISNULL( &ndn ) ) {
			op->o_tmpfree( ndn.bv_val, op->o_tmpmemctx );
		}
		if ( c_id != NULL ) {
			ch_free( c_id );
		}
	}
	backsql_FreeRow_x( &row, bsi->bsi_op->o_tmpmemctx );
	SQLFreeStmt( sth, SQL_DROP );

	Debug( LDAP_DEBUG_TRACE, "<==backsql_oc_get_candidates(): %d\n",
			n_candidates - bsi->bsi_n_candidates, 0, 0 );

	return ( bsi->bsi_n_candidates == -1 ? BACKSQL_AVL_STOP : BACKSQL_AVL_CONTINUE );
}

int
backsql_search( Operation *op, SlapReply *rs )
{
	backsql_info		*bi = (backsql_info *)op->o_bd->be_private;
	SQLHDBC			dbh = SQL_NULL_HDBC;
	int			sres;
	Entry			user_entry = { 0 },
				base_entry = { 0 };
	int			manageDSAit = get_manageDSAit( op );
	time_t			stoptime = 0;
	backsql_srch_info	bsi = { 0 };
	backsql_entryID		*eid = NULL;
	struct berval		nbase = BER_BVNULL;
#ifndef BACKSQL_ARBITRARY_KEY
	ID			lastid = 0;
#endif /* ! BACKSQL_ARBITRARY_KEY */

	Debug( LDAP_DEBUG_TRACE, "==>backsql_search(): "
		"base=\"%s\", filter=\"%s\", scope=%d,", 
		op->o_req_ndn.bv_val,
		op->ors_filterstr.bv_val,
		op->ors_scope );
	Debug( LDAP_DEBUG_TRACE, " deref=%d, attrsonly=%d, "
		"attributes to load: %s\n",
		op->ors_deref,
		op->ors_attrsonly,
		op->ors_attrs == NULL ? "all" : "custom list" );

	if ( op->o_req_ndn.bv_len > BACKSQL_MAX_DN_LEN ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_search(): "
			"search base length (%ld) exceeds max length (%d)\n", 
			op->o_req_ndn.bv_len, BACKSQL_MAX_DN_LEN, 0 );
		/*
		 * FIXME: a LDAP_NO_SUCH_OBJECT could be appropriate
		 * since it is impossible that such a long DN exists
		 * in the backend
		 */
		rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
		send_ldap_result( op, rs );
		return 1;
	}

	sres = backsql_get_db_conn( op, &dbh );
	if ( sres != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_search(): "
			"could not get connection handle - exiting\n", 
			0, 0, 0 );
		rs->sr_err = sres;
		rs->sr_text = sres == LDAP_OTHER ?  "SQL-backend error" : NULL;
		send_ldap_result( op, rs );
		return 1;
	}

	/* compute it anyway; root does not use it */
	stoptime = op->o_time + op->ors_tlimit;

	/* init search */
	bsi.bsi_e = &base_entry;
	rs->sr_err = backsql_init_search( &bsi, &op->o_req_ndn,
			op->ors_scope,
			stoptime, op->ors_filter,
			dbh, op, rs, op->ors_attrs,
			( BACKSQL_ISF_MATCHED | BACKSQL_ISF_GET_ENTRY ) );
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

		/* an entry was created; free it */
		entry_clean( bsi.bsi_e );

		/* fall thru */

	default:
		if ( !BER_BVISNULL( &base_entry.e_nname )
				&& !access_allowed( op, &base_entry,
					slap_schema.si_ad_entry, NULL,
					ACL_DISCLOSE, NULL ) )
		{
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			if ( rs->sr_ref ) {
				ber_bvarray_free( rs->sr_ref );
				rs->sr_ref = NULL;
			}
			rs->sr_matched = NULL;
			rs->sr_text = NULL;
		}

		send_ldap_result( op, rs );

		if ( rs->sr_ref ) {
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
		}

		if ( !BER_BVISNULL( &base_entry.e_nname ) ) {
			entry_clean( &base_entry );
		}

		goto done;
	}
	/* NOTE: __NEW__ "search" access is required
	 * on searchBase object */
	{
		slap_mask_t	mask;
		
		if ( get_assert( op ) &&
				( test_filter( op, &base_entry, get_assertion( op ) )
				  != LDAP_COMPARE_TRUE ) )
		{
			rs->sr_err = LDAP_ASSERTION_FAILED;
			
		}
		if ( ! access_allowed_mask( op, &base_entry,
					slap_schema.si_ad_entry,
					NULL, ACL_SEARCH, NULL, &mask ) )
		{
			if ( rs->sr_err == LDAP_SUCCESS ) {
				rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			}
		}

		if ( rs->sr_err != LDAP_SUCCESS ) {
			if ( !ACL_GRANT( mask, ACL_DISCLOSE ) ) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				rs->sr_text = NULL;
			}
			send_ldap_result( op, rs );
			goto done;
		}
	}

	bsi.bsi_e = NULL;

	bsi.bsi_n_candidates =
		( op->ors_limit == NULL	/* isroot == TRUE */ ? -2 : 
		( op->ors_limit->lms_s_unchecked == -1 ? -2 :
		( op->ors_limit->lms_s_unchecked ) ) );

#ifndef BACKSQL_ARBITRARY_KEY
	/* If paged results are in effect, check the paging cookie */
	if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
		rs->sr_err = parse_paged_cookie( op, rs );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			goto done;
		}
	}
#endif /* ! BACKSQL_ARBITRARY_KEY */

	switch ( bsi.bsi_scope ) {
	case LDAP_SCOPE_BASE:
	case BACKSQL_SCOPE_BASE_LIKE:
		/*
		 * probably already found...
		 */
		bsi.bsi_id_list = &bsi.bsi_base_id;
		bsi.bsi_id_listtail = &bsi.bsi_base_id.eid_next;
		break;

	case LDAP_SCOPE_SUBTREE:
		/*
		 * if baseObject is defined, and if it is the root 
		 * of the search, add it to the candidate list
		 */
		if ( bi->sql_baseObject && BACKSQL_IS_BASEOBJECT_ID( &bsi.bsi_base_id.eid_id ) )
		{
			bsi.bsi_id_list = &bsi.bsi_base_id;
			bsi.bsi_id_listtail = &bsi.bsi_base_id.eid_next;
		}

		/* FALLTHRU */
	default:

		/*
		 * for each objectclass we try to construct query which gets IDs
		 * of entries matching LDAP query filter and scope (or at least 
		 * candidates), and get the IDs. Do this in ID order for paging.
		 */
		avl_apply( bi->sql_oc_by_id, backsql_oc_get_candidates,
				&bsi, BACKSQL_AVL_STOP, AVL_INORDER );

		/* check for abandon */
		if ( op->o_abandon ) {
			eid = bsi.bsi_id_list;
			rs->sr_err = SLAPD_ABANDON;
			goto send_results;
		}
	}

	if ( op->ors_limit != NULL	/* isroot == FALSE */
			&& op->ors_limit->lms_s_unchecked != -1
			&& bsi.bsi_n_candidates == -1 )
	{
		rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
		send_ldap_result( op, rs );
		goto done;
	}

	/*
	 * now we load candidate entries (only those attributes 
	 * mentioned in attrs and filter), test it against full filter 
	 * and then send to client; don't free entry_id if baseObject...
	 */
	for ( eid = bsi.bsi_id_list;
		eid != NULL; 
		eid = backsql_free_entryID( 
			eid, eid == &bsi.bsi_base_id ? 0 : 1, op->o_tmpmemctx ) )
	{
		int		rc;
		Attribute	*a_hasSubordinate = NULL,
				*a_entryUUID = NULL,
				*a_entryCSN = NULL,
				**ap = NULL;
		Entry		*e = NULL;

		/* check for abandon */
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto send_results;
		}

		/* check time limit */
		if ( op->ors_tlimit != SLAP_NO_LIMIT
				&& slap_get_time() > stoptime )
		{
			rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
			rs->sr_ctrls = NULL;
			rs->sr_ref = rs->sr_v2ref;
			goto send_results;
		}

		Debug(LDAP_DEBUG_TRACE, "backsql_search(): loading data "
			"for entry id=" BACKSQL_IDFMT " oc_id=" BACKSQL_IDNUMFMT ", keyval=" BACKSQL_IDFMT "\n",
			BACKSQL_IDARG(eid->eid_id),
			eid->eid_oc_id,
			BACKSQL_IDARG(eid->eid_keyval) );

		/* check scope */
		switch ( op->ors_scope ) {
		case LDAP_SCOPE_BASE:
		case BACKSQL_SCOPE_BASE_LIKE:
			if ( !dn_match( &eid->eid_ndn, &op->o_req_ndn ) ) {
				goto next_entry2;
			}
			break;

		case LDAP_SCOPE_ONE:
		{
			struct berval	rdn = eid->eid_ndn;

			rdn.bv_len -= op->o_req_ndn.bv_len + STRLENOF( "," );
			if ( !dnIsOneLevelRDN( &rdn ) ) {
				goto next_entry2;
			}
			/* fall thru */
		}

		case LDAP_SCOPE_SUBORDINATE:
			/* discard the baseObject entry */
			if ( dn_match( &eid->eid_ndn, &op->o_req_ndn ) ) {
				goto next_entry2;
			}
			/* FALLTHRU */
		case LDAP_SCOPE_SUBTREE:
			/* FIXME: this should never fail... */
			if ( !dnIsSuffix( &eid->eid_ndn, &op->o_req_ndn ) ) {
				goto next_entry2;
			}
			break;
		}

		if ( BACKSQL_IS_BASEOBJECT_ID( &eid->eid_id ) ) {
			/* don't recollect baseObject... */
			e = bi->sql_baseObject;

		} else if ( eid == &bsi.bsi_base_id ) {
			/* don't recollect searchBase object... */
			e = &base_entry;

		} else {
			bsi.bsi_e = &user_entry;
			rc = backsql_id2entry( &bsi, eid );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE, "backsql_search(): "
					"error %d in backsql_id2entry() "
					"- skipping\n", rc, 0, 0 );
				continue;
			}
			e = &user_entry;
		}

		if ( !manageDSAit &&
				op->ors_scope != LDAP_SCOPE_BASE &&
				op->ors_scope != BACKSQL_SCOPE_BASE_LIKE &&
				is_entry_referral( e ) )
		{
			BerVarray refs;

			refs = get_entry_referrals( op, e );
			if ( !refs ) {
				backsql_srch_info	bsi2 = { 0 };
				Entry			user_entry2 = { 0 };

				/* retry with the full entry... */
				bsi2.bsi_e = &user_entry2;
				rc = backsql_init_search( &bsi2,
						&e->e_nname,
						LDAP_SCOPE_BASE, 
						(time_t)(-1), NULL,
						dbh, op, rs, NULL,
						BACKSQL_ISF_GET_ENTRY );
				if ( rc == LDAP_SUCCESS ) {
					if ( is_entry_referral( &user_entry2 ) )
					{
						refs = get_entry_referrals( op,
								&user_entry2 );
					} else {
						rs->sr_err = LDAP_OTHER;
					}
					backsql_entry_clean( op, &user_entry2 );
				}
				if ( bsi2.bsi_attrs != NULL ) {
					op->o_tmpfree( bsi2.bsi_attrs,
							op->o_tmpmemctx );
				}
			}

			if ( refs ) {
				rs->sr_ref = referral_rewrite( refs,
						&e->e_name,
						&op->o_req_dn,
						op->ors_scope );
				ber_bvarray_free( refs );
			}

			if ( rs->sr_ref ) {
				rs->sr_err = LDAP_REFERRAL;

			} else {
				rs->sr_text = "bad referral object";
			}

			rs->sr_entry = e;
			rs->sr_matched = user_entry.e_name.bv_val;
			send_search_reference( op, rs );

			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
			rs->sr_matched = NULL;
			rs->sr_entry = NULL;
			if ( rs->sr_err == LDAP_REFERRAL ) {
				rs->sr_err = LDAP_SUCCESS;
			}

			goto next_entry;
		}

		/*
		 * We use this flag since we need to parse the filter
		 * anyway; we should have used the frontend API function
		 * filter_has_subordinates()
		 */
		if ( bsi.bsi_flags & BSQL_SF_FILTER_HASSUBORDINATE ) {
			rc = backsql_has_children( op, dbh, &e->e_nname );

			switch ( rc ) {
			case LDAP_COMPARE_TRUE:
			case LDAP_COMPARE_FALSE:
				a_hasSubordinate = slap_operational_hasSubordinate( rc == LDAP_COMPARE_TRUE );
				if ( a_hasSubordinate != NULL ) {
					for ( ap = &user_entry.e_attrs; 
							*ap; 
							ap = &(*ap)->a_next );

					*ap = a_hasSubordinate;
				}
				rc = 0;
				break;

			default:
				Debug(LDAP_DEBUG_TRACE, 
					"backsql_search(): "
					"has_children failed( %d)\n", 
					rc, 0, 0 );
				rc = 1;
				goto next_entry;
			}
		}

		if ( bsi.bsi_flags & BSQL_SF_FILTER_ENTRYUUID ) {
			a_entryUUID = backsql_operational_entryUUID( bi, eid );
			if ( a_entryUUID != NULL ) {
				if ( ap == NULL ) {
					ap = &user_entry.e_attrs;
				}

				for ( ; *ap; ap = &(*ap)->a_next );

				*ap = a_entryUUID;
			}
		}

#ifdef BACKSQL_SYNCPROV
		if ( bsi.bsi_flags & BSQL_SF_FILTER_ENTRYCSN ) {
			a_entryCSN = backsql_operational_entryCSN( op );
			if ( a_entryCSN != NULL ) {
				if ( ap == NULL ) {
					ap = &user_entry.e_attrs;
				}

				for ( ; *ap; ap = &(*ap)->a_next );

				*ap = a_entryCSN;
			}
		}
#endif /* BACKSQL_SYNCPROV */

		if ( test_filter( op, e, op->ors_filter ) == LDAP_COMPARE_TRUE )
		{
#ifndef BACKSQL_ARBITRARY_KEY
			/* If paged results are in effect, see if the page limit was exceeded */
			if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
				if ( rs->sr_nentries >= ((PagedResultsState *)op->o_pagedresults_state)->ps_size )
				{
					e = NULL;
					send_paged_response( op, rs, &lastid );
					goto done;
				}
				lastid = SQL_TO_PAGECOOKIE( eid->eid_id, eid->eid_oc_id );
			}
#endif /* ! BACKSQL_ARBITRARY_KEY */
			rs->sr_attrs = op->ors_attrs;
			rs->sr_operational_attrs = NULL;
			rs->sr_entry = e;
			e->e_private = (void *)eid;
			rs->sr_flags = ( e == &user_entry ) ? REP_ENTRY_MODIFIABLE : 0;
			/* FIXME: need the whole entry (ITS#3480) */
			rs->sr_err = send_search_entry( op, rs );
			e->e_private = NULL;
			rs->sr_entry = NULL;
			rs->sr_attrs = NULL;
			rs->sr_operational_attrs = NULL;

			switch ( rs->sr_err ) {
			case LDAP_UNAVAILABLE:
				/*
				 * FIXME: send_search_entry failed;
				 * better stop
				 */
				Debug( LDAP_DEBUG_TRACE, "backsql_search(): "
					"connection lost\n", 0, 0, 0 );
				goto end_of_search;

			case LDAP_SIZELIMIT_EXCEEDED:
			case LDAP_BUSY:
				goto send_results;
			}
		}

next_entry:;
		if ( e == &user_entry ) {
			backsql_entry_clean( op, &user_entry );
		}

next_entry2:;
	}

end_of_search:;
	if ( rs->sr_nentries > 0 ) {
		rs->sr_ref = rs->sr_v2ref;
		rs->sr_err = (rs->sr_v2ref == NULL) ? LDAP_SUCCESS
			: LDAP_REFERRAL;

	} else {
		rs->sr_err = bsi.bsi_status;
	}

send_results:;
	if ( rs->sr_err != SLAPD_ABANDON ) {
#ifndef BACKSQL_ARBITRARY_KEY
		if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
			send_paged_response( op, rs, NULL );
		} else
#endif /* ! BACKSQL_ARBITRARY_KEY */
		{
			send_ldap_result( op, rs );
		}
	}

	/* cleanup in case of abandon */
	for ( ; eid != NULL; 
		eid = backsql_free_entryID(
			eid, eid == &bsi.bsi_base_id ? 0 : 1, op->o_tmpmemctx ) )
		;

	backsql_entry_clean( op, &base_entry );

	/* in case we got here accidentally */
	backsql_entry_clean( op, &user_entry );

	if ( rs->sr_v2ref ) {
		ber_bvarray_free( rs->sr_v2ref );
		rs->sr_v2ref = NULL;
	}

#ifdef BACKSQL_SYNCPROV
	if ( op->o_sync ) {
		Operation	op2 = *op;
		SlapReply	rs2 = { REP_RESULT };
		Entry		*e = entry_alloc();
		slap_callback	cb = { 0 };

		op2.o_tag = LDAP_REQ_ADD;
		op2.o_bd = select_backend( &op->o_bd->be_nsuffix[0], 0 );
		op2.ora_e = e;
		op2.o_callback = &cb;

		ber_dupbv( &e->e_name, op->o_bd->be_suffix );
		ber_dupbv( &e->e_nname, op->o_bd->be_nsuffix );

		cb.sc_response = slap_null_cb;

		op2.o_bd->be_add( &op2, &rs2 );

		if ( op2.ora_e == e )
			entry_free( e );
	}
#endif /* BACKSQL_SYNCPROV */

done:;
	(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );

	if ( bsi.bsi_attrs != NULL ) {
		op->o_tmpfree( bsi.bsi_attrs, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &nbase )
			&& nbase.bv_val != op->o_req_ndn.bv_val )
	{
		ch_free( nbase.bv_val );
	}

	/* restore scope ... FIXME: this should be done before ANY
	 * frontend call that uses op */
	if ( op->ors_scope == BACKSQL_SCOPE_BASE_LIKE ) {
		op->ors_scope = LDAP_SCOPE_BASE;
	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_search()\n", 0, 0, 0 );

	return rs->sr_err;
}

/* return LDAP_SUCCESS IFF we can retrieve the specified entry.
 */
int
backsql_entry_get(
		Operation		*op,
		struct berval		*ndn,
		ObjectClass		*oc,
		AttributeDescription	*at,
		int			rw,
		Entry			**ent )
{
	backsql_srch_info	bsi = { 0 };
	SQLHDBC			dbh = SQL_NULL_HDBC;
	int			rc;
	SlapReply		rs = { 0 };
	AttributeName		anlist[ 2 ];

	*ent = NULL;

	rc = backsql_get_db_conn( op, &dbh );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( at ) {
		anlist[ 0 ].an_name = at->ad_cname;
		anlist[ 0 ].an_desc = at;
		BER_BVZERO( &anlist[ 1 ].an_name );
	}

	bsi.bsi_e = entry_alloc();
	rc = backsql_init_search( &bsi,
			ndn,
			LDAP_SCOPE_BASE, 
			(time_t)(-1), NULL,
			dbh, op, &rs, at ? anlist : NULL,
			BACKSQL_ISF_GET_ENTRY );

	if ( !BER_BVISNULL( &bsi.bsi_base_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
	}

	if ( rc == LDAP_SUCCESS ) {

#if 0 /* not supported at present */
		/* find attribute values */
		if ( is_entry_alias( bsi.bsi_e ) ) {
			Debug( LDAP_DEBUG_ACL,
				"<= backsql_entry_get: entry is an alias\n",
				0, 0, 0 );
			rc = LDAP_ALIAS_PROBLEM;
			goto return_results;
		}
#endif

		if ( is_entry_referral( bsi.bsi_e ) ) {
			Debug( LDAP_DEBUG_ACL,
				"<= backsql_entry_get: entry is a referral\n",
				0, 0, 0 );
			rc = LDAP_REFERRAL;
			goto return_results;
		}

		if ( oc && !is_entry_objectclass( bsi.bsi_e, oc, 0 ) ) {
			Debug( LDAP_DEBUG_ACL,
					"<= backsql_entry_get: "
					"failed to find objectClass\n",
					0, 0, 0 ); 
			rc = LDAP_NO_SUCH_ATTRIBUTE;
			goto return_results;
		}

		*ent = bsi.bsi_e;
	}

return_results:;
	if ( bsi.bsi_attrs != NULL ) {
		op->o_tmpfree( bsi.bsi_attrs, op->o_tmpmemctx );
	}

	if ( rc != LDAP_SUCCESS ) {
		if ( bsi.bsi_e ) {
			entry_free( bsi.bsi_e );
		}
	}

	return rc;
}

void
backsql_entry_clean(
		Operation	*op,
		Entry		*e )
{
	void *ctx;

	ctx = ldap_pvt_thread_pool_context();

	if ( ctx == NULL || ctx != op->o_tmpmemctx ) {
		if ( !BER_BVISNULL( &e->e_name ) ) {
			op->o_tmpfree( e->e_name.bv_val, op->o_tmpmemctx );
			BER_BVZERO( &e->e_name );
		}

		if ( !BER_BVISNULL( &e->e_nname ) ) {
			op->o_tmpfree( e->e_nname.bv_val, op->o_tmpmemctx );
			BER_BVZERO( &e->e_nname );
		}
	}

	entry_clean( e );
}

int
backsql_entry_release(
		Operation	*op,
		Entry		*e,
		int		rw )
{
	backsql_entry_clean( op, e );

	entry_free( e );

	return 0;
}

#ifndef BACKSQL_ARBITRARY_KEY
/* This function is copied verbatim from back-bdb/search.c */
static int
parse_paged_cookie( Operation *op, SlapReply *rs )
{
	int		rc = LDAP_SUCCESS;
	PagedResultsState *ps = op->o_pagedresults_state;

	/* this function must be invoked only if the pagedResults
	 * control has been detected, parsed and partially checked
	 * by the frontend */
	assert( get_pagedresults( op ) > SLAP_CONTROL_IGNORED );

	/* cookie decoding/checks deferred to backend... */
	if ( ps->ps_cookieval.bv_len ) {
		PagedResultsCookie reqcookie;
		if( ps->ps_cookieval.bv_len != sizeof( reqcookie ) ) {
			/* bad cookie */
			rs->sr_text = "paged results cookie is invalid";
			rc = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		AC_MEMCPY( &reqcookie, ps->ps_cookieval.bv_val, sizeof( reqcookie ));

		if ( reqcookie > ps->ps_cookie ) {
			/* bad cookie */
			rs->sr_text = "paged results cookie is invalid";
			rc = LDAP_PROTOCOL_ERROR;
			goto done;

		} else if ( reqcookie < ps->ps_cookie ) {
			rs->sr_text = "paged results cookie is invalid or old";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

	} else {
		/* Initial request.  Initialize state. */
		ps->ps_cookie = 0;
		ps->ps_count = 0;
	}

done:;

	return rc;
}

/* This function is copied nearly verbatim from back-bdb/search.c */
static void
send_paged_response( 
	Operation	*op,
	SlapReply	*rs,
	ID		*lastid )
{
	LDAPControl	ctrl, *ctrls[2];
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;
	PagedResultsCookie respcookie;
	struct berval cookie;

	Debug(LDAP_DEBUG_ARGS,
		"send_paged_response: lastid=0x%08lx nentries=%d\n", 
		lastid ? *lastid : 0, rs->sr_nentries, NULL );

	BER_BVZERO( &ctrl.ldctl_value );
	ctrls[0] = &ctrl;
	ctrls[1] = NULL;

	ber_init2( ber, NULL, LBER_USE_DER );

	if ( lastid ) {
		respcookie = ( PagedResultsCookie )(*lastid);
		cookie.bv_len = sizeof( respcookie );
		cookie.bv_val = (char *)&respcookie;

	} else {
		respcookie = ( PagedResultsCookie )0;
		BER_BVSTR( &cookie, "" );
	}

	op->o_conn->c_pagedresults_state.ps_cookie = respcookie;
	op->o_conn->c_pagedresults_state.ps_count =
		((PagedResultsState *)op->o_pagedresults_state)->ps_count +
		rs->sr_nentries;

	/* return size of 0 -- no estimate */
	ber_printf( ber, "{iO}", 0, &cookie ); 

	if ( ber_flatten2( ber, &ctrls[0]->ldctl_value, 0 ) == -1 ) {
		goto done;
	}

	ctrls[0]->ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
	ctrls[0]->ldctl_iscritical = 0;

	rs->sr_ctrls = ctrls;
	rs->sr_err = LDAP_SUCCESS;
	send_ldap_result( op, rs );
	rs->sr_ctrls = NULL;

done:
	(void) ber_free_buf( ber );
}
#endif /* ! BACKSQL_ARBITRARY_KEY */
