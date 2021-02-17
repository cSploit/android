/* lastmod.c - returns last modification info */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_LASTMOD

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "lutil.h"

typedef struct lastmod_info_t {
	struct berval		lmi_rdnvalue;
	Entry			*lmi_e;
	ldap_pvt_thread_mutex_t	lmi_entry_mutex;
	int			lmi_enabled;
} lastmod_info_t;

struct lastmod_schema_t {
	ObjectClass		*lms_oc_lastmod;
	AttributeDescription	*lms_ad_lastmodDN;
	AttributeDescription	*lms_ad_lastmodType;
	AttributeDescription	*lms_ad_lastmodEnabled;
} lastmod_schema;

enum lastmodType_e {
	LASTMOD_ADD = 0,
	LASTMOD_DELETE,
	LASTMOD_EXOP,
	LASTMOD_MODIFY,
	LASTMOD_MODRDN,
	LASTMOD_UNKNOWN
};

struct berval lastmodType[] = {
	BER_BVC( "add" ),
	BER_BVC( "delete" ),
	BER_BVC( "exop" ),
	BER_BVC( "modify" ),
	BER_BVC( "modrdn" ),
	BER_BVC( "unknown" ),
	BER_BVNULL
};

static struct m_s {
	char			*schema;
	slap_mask_t 		flags;
	int			offset;
} moc[] = {
	{ "( 1.3.6.1.4.1.4203.666.3.13"
		"NAME 'lastmod' "
		"DESC 'OpenLDAP per-database last modification monitoring' "
		"STRUCTURAL "
		"SUP top "
		"MUST cn "
		"MAY ( "
			"lastmodDN "
			"$ lastmodType "
			"$ description "
			"$ seeAlso "
		") )", SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof( struct lastmod_schema_t, lms_oc_lastmod ) },
	{ NULL }
}, mat[] = {
	{ "( 1.3.6.1.4.1.4203.666.1.28"
		"NAME 'lastmodDN' "
		"DESC 'DN of last modification' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )", SLAP_AT_HIDE,
		offsetof( struct lastmod_schema_t, lms_ad_lastmodDN ) },
	{ "( 1.3.6.1.4.1.4203.666.1.29"
		"NAME 'lastmodType' "
		"DESC 'Type of last modification' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
		"EQUALITY caseIgnoreMatch "
		"SINGLE-VALUE "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )", SLAP_AT_HIDE,
		offsetof( struct lastmod_schema_t, lms_ad_lastmodType ) },
	{ "( 1.3.6.1.4.1.4203.666.1.30"
		"NAME 'lastmodEnabled' "
		"DESC 'Lastmod overlay state' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
		"EQUALITY booleanMatch "
		"SINGLE-VALUE )", 0,
		offsetof( struct lastmod_schema_t, lms_ad_lastmodEnabled ) },
	{ NULL }

	/* FIXME: what about UUID of last modified entry? */
};

static int
lastmod_search( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	int			rc;

	/* if we get here, it must be a success */
	rs->sr_err = LDAP_SUCCESS;

	ldap_pvt_thread_mutex_lock( &lmi->lmi_entry_mutex );

	rc = test_filter( op, lmi->lmi_e, op->oq_search.rs_filter );
	if ( rc == LDAP_COMPARE_TRUE ) {
		rs->sr_attrs = op->ors_attrs;
		rs->sr_flags = 0;
		rs->sr_entry = lmi->lmi_e;
		rs->sr_err = send_search_entry( op, rs );
		rs->sr_entry = NULL;
		rs->sr_flags = 0;
		rs->sr_attrs = NULL;
	}

	ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );

	send_ldap_result( op, rs );

	return 0;
}

static int
lastmod_compare( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	Attribute		*a;

	ldap_pvt_thread_mutex_lock( &lmi->lmi_entry_mutex );

	if ( get_assert( op ) &&
		( test_filter( op, lmi->lmi_e, get_assertion( op ) ) != LDAP_COMPARE_TRUE ) )
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		goto return_results;
	}

	rs->sr_err = access_allowed( op, lmi->lmi_e, op->oq_compare.rs_ava->aa_desc,
		&op->oq_compare.rs_ava->aa_value, ACL_COMPARE, NULL );
	if ( ! rs->sr_err ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

	rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;

	for ( a = attr_find( lmi->lmi_e->e_attrs, op->oq_compare.rs_ava->aa_desc );
		a != NULL;
		a = attr_find( a->a_next, op->oq_compare.rs_ava->aa_desc ) )
	{
		rs->sr_err = LDAP_COMPARE_FALSE;

		if ( value_find_ex( op->oq_compare.rs_ava->aa_desc,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
			a->a_nvals, &op->oq_compare.rs_ava->aa_value, op->o_tmpmemctx ) == 0 )
		{
			rs->sr_err = LDAP_COMPARE_TRUE;
			break;
		}
	}

return_results:;

	ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );

	send_ldap_result( op, rs );

	if( rs->sr_err == LDAP_COMPARE_FALSE || rs->sr_err == LDAP_COMPARE_TRUE ) {
		rs->sr_err = LDAP_SUCCESS;
	}

	return rs->sr_err;
}

static int
lastmod_exop( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;

	/* Temporary */

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
	rs->sr_text = "not allowed within namingContext";
	send_ldap_result( op, rs );
	rs->sr_text = NULL;
	
	return -1;
}

static int
lastmod_modify( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	Modifications		*ml;

	ldap_pvt_thread_mutex_lock( &lmi->lmi_entry_mutex );

	if ( !acl_check_modlist( op, lmi->lmi_e, op->orm_modlist ) ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto cleanup;
	}

	for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		Attribute	*a;

		if ( ml->sml_desc != lastmod_schema.lms_ad_lastmodEnabled ) {
			continue;
		}

		if ( ml->sml_op != LDAP_MOD_REPLACE ) {
			rs->sr_text = "unsupported mod type";
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			goto cleanup;
		}
		
		a = attr_find( lmi->lmi_e->e_attrs, ml->sml_desc );

		if ( a == NULL ) {
			rs->sr_text = "lastmod overlay internal error";
			rs->sr_err = LDAP_OTHER;
			goto cleanup;
		}

		ch_free( a->a_vals[ 0 ].bv_val );
		ber_dupbv( &a->a_vals[ 0 ], &ml->sml_values[ 0 ] );
		if ( a->a_nvals ) {
			ch_free( a->a_nvals[ 0 ].bv_val );
			if ( ml->sml_nvalues && !BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
				ber_dupbv( &a->a_nvals[ 0 ], &ml->sml_nvalues[ 0 ] );
			} else {
				ber_dupbv( &a->a_nvals[ 0 ], &ml->sml_values[ 0 ] );
			}
		}

		if ( strcmp( ml->sml_values[ 0 ].bv_val, "TRUE" ) == 0 ) {
			lmi->lmi_enabled = 1;
		} else if ( strcmp( ml->sml_values[ 0 ].bv_val, "FALSE" ) == 0 ) {
			lmi->lmi_enabled = 0;
		} else {
			assert( 0 );
		}
	}

	rs->sr_err = LDAP_SUCCESS;

cleanup:;
	ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );

	send_ldap_result( op, rs );
	rs->sr_text = NULL;

	return rs->sr_err;
}

static int
lastmod_op_func( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	Modifications		*ml;

	if ( dn_match( &op->o_req_ndn, &lmi->lmi_e->e_nname ) ) {
		switch ( op->o_tag ) {
		case LDAP_REQ_SEARCH:
			if ( op->ors_scope != LDAP_SCOPE_BASE ) {
				goto return_referral;
			}
			/* process */
			return lastmod_search( op, rs );

		case LDAP_REQ_COMPARE:
			return lastmod_compare( op, rs );

		case LDAP_REQ_EXTENDED:
			/* if write, reject; otherwise process */
			if ( exop_is_write( op )) {
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "not allowed within namingContext";
				goto return_error;
			}
			return lastmod_exop( op, rs );

		case LDAP_REQ_MODIFY:
			/* allow only changes to overlay status */
			for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
				if ( ad_cmp( ml->sml_desc, slap_schema.si_ad_modifiersName ) != 0
						&& ad_cmp( ml->sml_desc, slap_schema.si_ad_modifyTimestamp ) != 0
						&& ad_cmp( ml->sml_desc, slap_schema.si_ad_entryCSN ) != 0
						&& ad_cmp( ml->sml_desc, lastmod_schema.lms_ad_lastmodEnabled ) != 0 )
				{
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "not allowed within namingContext";
					goto return_error;
				}
			}
			return lastmod_modify( op, rs );

		default:
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "not allowed within namingContext";
			goto return_error;
		}
	}

	if ( dnIsSuffix( &op->o_req_ndn, &lmi->lmi_e->e_nname ) ) {
		goto return_referral;
	}

	return SLAP_CB_CONTINUE;

return_referral:;
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rs->sr_ref = referral_rewrite( default_referral,
			NULL, &op->o_req_dn, op->ors_scope );

	if ( !rs->sr_ref ) {
		rs->sr_ref = default_referral;
	}
	rs->sr_err = LDAP_REFERRAL;
	send_ldap_result( op, rs );

	if ( rs->sr_ref != default_referral ) {
		ber_bvarray_free( rs->sr_ref );
	}
	rs->sr_ref = NULL;

	return -1;

return_error:;
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	send_ldap_result( op, rs );
	rs->sr_text = NULL;

	return -1;
}

static int
best_guess( Operation *op,
		struct berval *bv_entryCSN, struct berval *bv_nentryCSN,
		struct berval *bv_modifyTimestamp, struct berval *bv_nmodifyTimestamp,
		struct berval *bv_modifiersName, struct berval *bv_nmodifiersName )
{
	if ( bv_entryCSN ) {
		char		csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
		struct berval	entryCSN;
	
		entryCSN.bv_val = csnbuf;
		entryCSN.bv_len = sizeof( csnbuf );
		slap_get_csn( NULL, &entryCSN, 0 );

		ber_dupbv( bv_entryCSN, &entryCSN );
		ber_dupbv( bv_nentryCSN, &entryCSN );
	}

	if ( bv_modifyTimestamp ) {
		char		tmbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		struct berval timestamp;
		time_t		currtime;

		/* best guess */
#if 0
		currtime = slap_get_time();
#endif
		/* maybe we better use the time the operation was initiated */
		currtime = op->o_time;

		timestamp.bv_val = tmbuf;
		timestamp.bv_len = sizeof(tmbuf);
		slap_timestamp( &currtime, &timestamp );

		ber_dupbv( bv_modifyTimestamp, &timestamp );
		ber_dupbv( bv_nmodifyTimestamp, bv_modifyTimestamp );
	}

	if ( bv_modifiersName ) {
		/* best guess */
		ber_dupbv( bv_modifiersName, &op->o_dn );
		ber_dupbv( bv_nmodifiersName, &op->o_ndn );
	}

	return 0;
}

static int
lastmod_update( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	Attribute		*a;
	Modifications		*ml = NULL;
	struct berval		bv_entryCSN = BER_BVNULL,
				bv_nentryCSN = BER_BVNULL,
				bv_modifyTimestamp = BER_BVNULL,
				bv_nmodifyTimestamp = BER_BVNULL,
				bv_modifiersName = BER_BVNULL,
				bv_nmodifiersName = BER_BVNULL,
				bv_name = BER_BVNULL,
				bv_nname = BER_BVNULL;
	enum lastmodType_e	lmt = LASTMOD_UNKNOWN;
	Entry			*e = NULL;
	int			rc = -1;

	/* FIXME: timestamp? modifier? */
	switch ( op->o_tag ) {
	case LDAP_REQ_ADD:
		lmt = LASTMOD_ADD;
		e = op->ora_e;
		a = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
		if ( a != NULL ) {
			ber_dupbv( &bv_entryCSN, &a->a_vals[0] );
			if ( a->a_nvals && !BER_BVISNULL( &a->a_nvals[0] ) ) {
				ber_dupbv( &bv_nentryCSN, &a->a_nvals[0] );
			} else {
				ber_dupbv( &bv_nentryCSN, &a->a_vals[0] );
			}
		}
		a = attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp );
		if ( a != NULL ) {
			ber_dupbv( &bv_modifyTimestamp, &a->a_vals[0] );
			if ( a->a_nvals && !BER_BVISNULL( &a->a_nvals[0] ) ) {
				ber_dupbv( &bv_nmodifyTimestamp, &a->a_nvals[0] );
			} else {
				ber_dupbv( &bv_nmodifyTimestamp, &a->a_vals[0] );
			}
		}
		a = attr_find( e->e_attrs, slap_schema.si_ad_modifiersName );
		if ( a != NULL ) {
			ber_dupbv( &bv_modifiersName, &a->a_vals[0] );
			ber_dupbv( &bv_nmodifiersName, &a->a_nvals[0] );
		}
		ber_dupbv( &bv_name, &e->e_name );
		ber_dupbv( &bv_nname, &e->e_nname );
		break;

	case LDAP_REQ_DELETE:
		lmt = LASTMOD_DELETE;

		best_guess( op, &bv_entryCSN, &bv_nentryCSN,
				&bv_modifyTimestamp, &bv_nmodifyTimestamp,
				&bv_modifiersName, &bv_nmodifiersName );

		ber_dupbv( &bv_name, &op->o_req_dn );
		ber_dupbv( &bv_nname, &op->o_req_ndn );
		break;

	case LDAP_REQ_EXTENDED:
		lmt = LASTMOD_EXOP;

		/* actually, password change is wrapped around a backend 
		 * call to modify, so it never shows up as an exop... */
		best_guess( op, &bv_entryCSN, &bv_nentryCSN,
				&bv_modifyTimestamp, &bv_nmodifyTimestamp,
				&bv_modifiersName, &bv_nmodifiersName );

		ber_dupbv( &bv_name, &op->o_req_dn );
		ber_dupbv( &bv_nname, &op->o_req_ndn );
		break;

	case LDAP_REQ_MODIFY:
		lmt = LASTMOD_MODIFY;
		rc = 3;

		for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
			if ( ad_cmp( ml->sml_desc , slap_schema.si_ad_modifiersName ) == 0 ) {
				ber_dupbv( &bv_modifiersName, &ml->sml_values[0] );
				ber_dupbv( &bv_nmodifiersName, &ml->sml_nvalues[0] );

				rc--;
				if ( !rc ) {
					break;
				}

			} else if ( ad_cmp( ml->sml_desc, slap_schema.si_ad_entryCSN ) == 0 ) {
				ber_dupbv( &bv_entryCSN, &ml->sml_values[0] );
				if ( ml->sml_nvalues && !BER_BVISNULL( &ml->sml_nvalues[0] ) ) {
					ber_dupbv( &bv_nentryCSN, &ml->sml_nvalues[0] );
				} else {
					ber_dupbv( &bv_nentryCSN, &ml->sml_values[0] );
				}

				rc --;
				if ( !rc ) {
					break;
				}

			} else if ( ad_cmp( ml->sml_desc, slap_schema.si_ad_modifyTimestamp ) == 0 ) {
				ber_dupbv( &bv_modifyTimestamp, &ml->sml_values[0] );
				if ( ml->sml_nvalues && !BER_BVISNULL( &ml->sml_nvalues[0] ) ) {
					ber_dupbv( &bv_nmodifyTimestamp, &ml->sml_nvalues[0] );
				} else {
					ber_dupbv( &bv_nmodifyTimestamp, &ml->sml_values[0] );
				}

				rc --;
				if ( !rc ) {
					break;
				}
			}
		}

		/* if rooted at global overlay, opattrs are not yet in place */
		if ( BER_BVISNULL( &bv_modifiersName ) ) {
			best_guess( op, NULL, NULL, NULL, NULL, &bv_modifiersName, &bv_nmodifiersName );
		}

		if ( BER_BVISNULL( &bv_entryCSN ) ) {
			best_guess( op, &bv_entryCSN, &bv_nentryCSN, NULL, NULL, NULL, NULL );
		}

		if ( BER_BVISNULL( &bv_modifyTimestamp ) ) {
			best_guess( op, NULL, NULL, &bv_modifyTimestamp, &bv_nmodifyTimestamp, NULL, NULL );
		}

		ber_dupbv( &bv_name, &op->o_req_dn );
		ber_dupbv( &bv_nname, &op->o_req_ndn );
		break;

	case LDAP_REQ_MODRDN:
		lmt = LASTMOD_MODRDN;
		e = NULL;

		if ( op->orr_newSup && !BER_BVISNULL( op->orr_newSup ) ) {
			build_new_dn( &bv_name, op->orr_newSup, &op->orr_newrdn, NULL );
			build_new_dn( &bv_nname, op->orr_nnewSup, &op->orr_nnewrdn, NULL );

		} else {
			struct berval	pdn;

			dnParent( &op->o_req_dn, &pdn );
			build_new_dn( &bv_name, &pdn, &op->orr_newrdn, NULL );

			dnParent( &op->o_req_ndn, &pdn );
			build_new_dn( &bv_nname, &pdn, &op->orr_nnewrdn, NULL );
		}

		if ( on->on_info->oi_orig->bi_entry_get_rw ) {
			BackendInfo	*bi = op->o_bd->bd_info;
			int		rc;

			op->o_bd->bd_info = (BackendInfo *)on->on_info->oi_orig;
			rc = op->o_bd->bd_info->bi_entry_get_rw( op, &bv_name, NULL, NULL, 0, &e );
			if ( rc == LDAP_SUCCESS ) {
				a = attr_find( e->e_attrs, slap_schema.si_ad_modifiersName );
				if ( a != NULL ) {
					ber_dupbv( &bv_modifiersName, &a->a_vals[0] );
					ber_dupbv( &bv_nmodifiersName, &a->a_nvals[0] );
				}
				a = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
				if ( a != NULL ) {
					ber_dupbv( &bv_entryCSN, &a->a_vals[0] );
					if ( a->a_nvals && !BER_BVISNULL( &a->a_nvals[0] ) ) {
						ber_dupbv( &bv_nentryCSN, &a->a_nvals[0] );
					} else {
						ber_dupbv( &bv_nentryCSN, &a->a_vals[0] );
					}
				}
				a = attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp );
				if ( a != NULL ) {
					ber_dupbv( &bv_modifyTimestamp, &a->a_vals[0] );
					if ( a->a_nvals && !BER_BVISNULL( &a->a_nvals[0] ) ) {
						ber_dupbv( &bv_nmodifyTimestamp, &a->a_nvals[0] );
					} else {
						ber_dupbv( &bv_nmodifyTimestamp, &a->a_vals[0] );
					}
				}

				assert( dn_match( &bv_name, &e->e_name ) );
				assert( dn_match( &bv_nname, &e->e_nname ) );

				op->o_bd->bd_info->bi_entry_release_rw( op, e, 0 );
			}

			op->o_bd->bd_info = bi;

		}

		/* if !bi_entry_get_rw || bi_entry_get_rw failed for any reason... */
		if ( e == NULL ) {
			best_guess( op, &bv_entryCSN, &bv_nentryCSN,
					&bv_modifyTimestamp, &bv_nmodifyTimestamp,
					&bv_modifiersName, &bv_nmodifiersName );
		}

		break;

	default:
		return -1;
	}
	
	ldap_pvt_thread_mutex_lock( &lmi->lmi_entry_mutex );

#if 0
	fprintf( stderr, "### lastmodDN: %s %s\n", bv_name.bv_val, bv_nname.bv_val );
#endif

	a = attr_find( lmi->lmi_e->e_attrs, lastmod_schema.lms_ad_lastmodDN );
	if ( a == NULL ) {
		goto error_return;
	}
	ch_free( a->a_vals[0].bv_val );
	a->a_vals[0] = bv_name;
	ch_free( a->a_nvals[0].bv_val );
	a->a_nvals[0] = bv_nname;

#if 0
	fprintf( stderr, "### lastmodType: %s %s\n", lastmodType[ lmt ].bv_val, lastmodType[ lmt ].bv_val );
#endif

	a = attr_find( lmi->lmi_e->e_attrs, lastmod_schema.lms_ad_lastmodType );
	if ( a == NULL ) {
		goto error_return;
	} 
	ch_free( a->a_vals[0].bv_val );
	ber_dupbv( &a->a_vals[0], &lastmodType[ lmt ] );
	ch_free( a->a_nvals[0].bv_val );
	ber_dupbv( &a->a_nvals[0], &lastmodType[ lmt ] );

#if 0
	fprintf( stderr, "### modifiersName: %s %s\n", bv_modifiersName.bv_val, bv_nmodifiersName.bv_val );
#endif

	a = attr_find( lmi->lmi_e->e_attrs, slap_schema.si_ad_modifiersName );
	if ( a == NULL ) {
		goto error_return;
	} 
	ch_free( a->a_vals[0].bv_val );
	a->a_vals[0] = bv_modifiersName;
	ch_free( a->a_nvals[0].bv_val );
	a->a_nvals[0] = bv_nmodifiersName;

#if 0
	fprintf( stderr, "### modifyTimestamp: %s %s\n", bv_nmodifyTimestamp.bv_val, bv_modifyTimestamp.bv_val );
#endif

	a = attr_find( lmi->lmi_e->e_attrs, slap_schema.si_ad_modifyTimestamp );
	if ( a == NULL ) {
		goto error_return;
	} 
	ch_free( a->a_vals[0].bv_val );
	a->a_vals[0] = bv_modifyTimestamp;
	ch_free( a->a_nvals[0].bv_val );
	a->a_nvals[0] = bv_nmodifyTimestamp;

#if 0
	fprintf( stderr, "### entryCSN: %s %s\n", bv_nentryCSN.bv_val, bv_entryCSN.bv_val );
#endif

	a = attr_find( lmi->lmi_e->e_attrs, slap_schema.si_ad_entryCSN );
	if ( a == NULL ) {
		goto error_return;
	} 
	ch_free( a->a_vals[0].bv_val );
	a->a_vals[0] = bv_entryCSN;
	ch_free( a->a_nvals[0].bv_val );
	a->a_nvals[0] = bv_nentryCSN;

	rc = 0;

error_return:;
	ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );
	
	return rc;
}

static int
lastmod_response( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;

	/* don't record failed operations */
	switch ( rs->sr_err ) {
	case LDAP_SUCCESS:
		/* FIXME: other cases? */
		break;

	default:
		return SLAP_CB_CONTINUE;
	}

	/* record only write operations */
	switch ( op->o_tag ) {
	case LDAP_REQ_ADD:
	case LDAP_REQ_MODIFY:
	case LDAP_REQ_MODRDN:
	case LDAP_REQ_DELETE:
		break;

	case LDAP_REQ_EXTENDED:
		/* if write, process */
		if ( exop_is_write( op ))
			break;

		/* fall thru */
	default:
		return SLAP_CB_CONTINUE;
	}

	/* skip if disabled */
	ldap_pvt_thread_mutex_lock( &lmi->lmi_entry_mutex );
	if ( !lmi->lmi_enabled ) {
		ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );
		return SLAP_CB_CONTINUE;
	}
	ldap_pvt_thread_mutex_unlock( &lmi->lmi_entry_mutex );

	(void)lastmod_update( op, rs );

	return SLAP_CB_CONTINUE;
}

static int
lastmod_db_init(
	BackendDB *be
)
{
	slap_overinst		*on = (slap_overinst *)be->bd_info;
	lastmod_info_t		*lmi;

	if ( lastmod_schema.lms_oc_lastmod == NULL ) {
		int		i;
		const char 	*text;

		/* schema integration */
		for ( i = 0; mat[i].schema; i++ ) {
			int			code;
			AttributeDescription	**ad =
				((AttributeDescription **)&(((char *)&lastmod_schema)[mat[i].offset]));
			ad[0] = NULL;

			code = register_at( mat[i].schema, ad, 0 );
			if ( code ) {
				Debug( LDAP_DEBUG_ANY,
					"lastmod_init: register_at failed\n", 0, 0, 0 );
				return -1;
			}
			(*ad)->ad_type->sat_flags |= mat[i].flags;
		}

		for ( i = 0; moc[i].schema; i++ ) {
			int			code;
			ObjectClass		**Oc =
				((ObjectClass **)&(((char *)&lastmod_schema)[moc[i].offset]));
	
			code = register_oc( moc[i].schema, Oc, 0 );
			if ( code ) {
				Debug( LDAP_DEBUG_ANY,
					"lastmod_init: register_oc failed\n", 0, 0, 0 );
				return -1;
			}
			(*Oc)->soc_flags |= moc[i].flags;
		}
	}

	lmi = (lastmod_info_t *)ch_malloc( sizeof( lastmod_info_t ) );

	memset( lmi, 0, sizeof( lastmod_info_t ) );
	lmi->lmi_enabled = 1;
	
	on->on_bi.bi_private = lmi;

	return 0;
}

static int
lastmod_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char	**argv
)
{
	slap_overinst		*on = (slap_overinst *)be->bd_info;
	lastmod_info_t		*lmi = (lastmod_info_t *)on->on_bi.bi_private;

	if ( strcasecmp( argv[ 0 ], "lastmod-rdnvalue" ) == 0 ) {
		if ( lmi->lmi_rdnvalue.bv_val ) {
			/* already defined! */
			ch_free( lmi->lmi_rdnvalue.bv_val );
		}

		ber_str2bv( argv[ 1 ], 0, 1, &lmi->lmi_rdnvalue );

	} else if ( strcasecmp( argv[ 0 ], "lastmod-enabled" ) == 0 ) {
		if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			lmi->lmi_enabled = 1;

		} else if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			lmi->lmi_enabled = 0;

		} else {
			return -1;
		}

	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}

static int
lastmod_db_open(
	BackendDB *be
)
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;
	lastmod_info_t	*lmi = (lastmod_info_t *)on->on_bi.bi_private;
	char		buf[ 8192 ];
	static char		tmbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];

	char			csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
	struct berval		entryCSN;
	struct berval timestamp;

	if ( !SLAP_LASTMOD( be ) ) {
		fprintf( stderr, "set \"lastmod on\" to make this overlay effective\n" );
		return -1;
	}

	/*
	 * Start
	 */
	timestamp.bv_val = tmbuf;
	timestamp.bv_len = sizeof(tmbuf);
	slap_timestamp( &starttime, &timestamp );

	entryCSN.bv_val = csnbuf;
	entryCSN.bv_len = sizeof( csnbuf );
	slap_get_csn( NULL, &entryCSN, 0 );

	if ( BER_BVISNULL( &lmi->lmi_rdnvalue ) ) {
		ber_str2bv( "Lastmod", 0, 1, &lmi->lmi_rdnvalue );
	}

	snprintf( buf, sizeof( buf ),
			"dn: cn=%s%s%s\n"
			"objectClass: %s\n"
			"structuralObjectClass: %s\n"
			"cn: %s\n"
			"description: This object contains the last modification to this database\n"
			"%s: cn=%s%s%s\n"
			"%s: %s\n"
			"%s: %s\n"
			"createTimestamp: %s\n"
			"creatorsName: %s\n"
			"entryCSN: %s\n"
			"modifyTimestamp: %s\n"
			"modifiersName: %s\n"
			"hasSubordinates: FALSE\n",
			lmi->lmi_rdnvalue.bv_val, BER_BVISEMPTY( &be->be_suffix[ 0 ] ) ? "" : ",", be->be_suffix[ 0 ].bv_val,
			lastmod_schema.lms_oc_lastmod->soc_cname.bv_val,
			lastmod_schema.lms_oc_lastmod->soc_cname.bv_val,
			lmi->lmi_rdnvalue.bv_val,
			lastmod_schema.lms_ad_lastmodDN->ad_cname.bv_val,
				lmi->lmi_rdnvalue.bv_val, BER_BVISEMPTY( &be->be_suffix[ 0 ] ) ? "" : ",", be->be_suffix[ 0 ].bv_val,
			lastmod_schema.lms_ad_lastmodType->ad_cname.bv_val, lastmodType[ LASTMOD_ADD ].bv_val,
			lastmod_schema.lms_ad_lastmodEnabled->ad_cname.bv_val, lmi->lmi_enabled ? "TRUE" : "FALSE",
			tmbuf,
			BER_BVISNULL( &be->be_rootdn ) ? SLAPD_ANONYMOUS : be->be_rootdn.bv_val,
			entryCSN.bv_val,
			tmbuf,
			BER_BVISNULL( &be->be_rootdn ) ? SLAPD_ANONYMOUS : be->be_rootdn.bv_val );

#if 0
	fprintf( stderr, "# entry:\n%s\n", buf );
#endif

	lmi->lmi_e = str2entry( buf );
	if ( lmi->lmi_e == NULL ) {
		return -1;
	}

	ldap_pvt_thread_mutex_init( &lmi->lmi_entry_mutex );

	return 0;
}

static int
lastmod_db_destroy(
	BackendDB *be
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	lastmod_info_t	*lmi = (lastmod_info_t *)on->on_bi.bi_private;

	if ( lmi ) {
		if ( !BER_BVISNULL( &lmi->lmi_rdnvalue ) ) {
			ch_free( lmi->lmi_rdnvalue.bv_val );
		}

		if ( lmi->lmi_e ) {
			entry_free( lmi->lmi_e );

			ldap_pvt_thread_mutex_destroy( &lmi->lmi_entry_mutex );
		}

		ch_free( lmi );
	}

	return 0;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst 		lastmod;

int
lastmod_initialize()
{
	lastmod.on_bi.bi_type = "lastmod";
	lastmod.on_bi.bi_db_init = lastmod_db_init;
	lastmod.on_bi.bi_db_config = lastmod_db_config;
	lastmod.on_bi.bi_db_destroy = lastmod_db_destroy;
	lastmod.on_bi.bi_db_open = lastmod_db_open;

	lastmod.on_bi.bi_op_add = lastmod_op_func;
	lastmod.on_bi.bi_op_compare = lastmod_op_func;
	lastmod.on_bi.bi_op_delete = lastmod_op_func;
	lastmod.on_bi.bi_op_modify = lastmod_op_func;
	lastmod.on_bi.bi_op_modrdn = lastmod_op_func;
	lastmod.on_bi.bi_op_search = lastmod_op_func;
	lastmod.on_bi.bi_extended = lastmod_op_func;

	lastmod.on_response = lastmod_response;

	return overlay_register( &lastmod );
}

#if SLAPD_OVER_LASTMOD == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return lastmod_initialize();
}
#endif /* SLAPD_OVER_LASTMOD == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_LASTMOD) */
