/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
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
#include <ac/socket.h>

#include "slap.h"

static int
test_mra_vrFilter(
	Operation	*op,
	Attribute	*a,
	MatchingRuleAssertion *mra,
	char 		***e_flags
);

static int
test_substrings_vrFilter(
	Operation	*op,
	Attribute	*a,
	ValuesReturnFilter *f,
	char		***e_flags
);

static int
test_presence_vrFilter(
	Operation	*op,
	Attribute	*a,
	AttributeDescription *desc,
	char 		***e_flags
);

static int
test_ava_vrFilter(
	Operation	*op,
	Attribute	*a,
	AttributeAssertion *ava,
	int		type,
	char 		***e_flags
);


int
filter_matched_values( 
	Operation	*op,
	Attribute	*a,
	char		***e_flags )
{
	ValuesReturnFilter *vrf;
	int		rc = LDAP_SUCCESS;

	Debug( LDAP_DEBUG_FILTER, "=> filter_matched_values\n", 0, 0, 0 );

	for ( vrf = op->o_vrFilter; vrf != NULL; vrf = vrf->vrf_next ) {
		switch ( vrf->vrf_choice ) {
		case SLAPD_FILTER_COMPUTED:
			Debug( LDAP_DEBUG_FILTER, "	COMPUTED %s (%d)\n",
				vrf->vrf_result == LDAP_COMPARE_FALSE ? "false"
				: vrf->vrf_result == LDAP_COMPARE_TRUE ? "true"
				: vrf->vrf_result == SLAPD_COMPARE_UNDEFINED ? "undefined"
				: "error",
				vrf->vrf_result, 0 );
			/*This type of filter does not affect the result */
			rc = LDAP_SUCCESS;
		break;

		case LDAP_FILTER_EQUALITY:
			Debug( LDAP_DEBUG_FILTER, "	EQUALITY\n", 0, 0, 0 );
			rc = test_ava_vrFilter( op, a, vrf->vrf_ava,
				LDAP_FILTER_EQUALITY, e_flags );
			if( rc == -1 ) return rc;
			break;

		case LDAP_FILTER_SUBSTRINGS:
			Debug( LDAP_DEBUG_FILTER, "	SUBSTRINGS\n", 0, 0, 0 );
			rc = test_substrings_vrFilter( op, a,
				vrf, e_flags );
			if( rc == -1 ) return rc;
			break;

		case LDAP_FILTER_PRESENT:
			Debug( LDAP_DEBUG_FILTER, "	PRESENT\n", 0, 0, 0 );
			rc = test_presence_vrFilter( op, a,
				vrf->vrf_desc, e_flags );
			if( rc == -1 ) return rc;
			break;

		case LDAP_FILTER_GE:
			rc = test_ava_vrFilter( op, a, vrf->vrf_ava,
				LDAP_FILTER_GE, e_flags );
			if( rc == -1 ) return rc;
			break;

		case LDAP_FILTER_LE:
			rc = test_ava_vrFilter( op, a, vrf->vrf_ava,
				LDAP_FILTER_LE, e_flags );
			if( rc == -1 ) return rc;
			break;

		case LDAP_FILTER_EXT:
			Debug( LDAP_DEBUG_FILTER, "	EXT\n", 0, 0, 0 );
			rc = test_mra_vrFilter( op, a,
				vrf->vrf_mra, e_flags );
			if( rc == -1 ) return rc;
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "	unknown filter type %lu\n",
				vrf->vrf_choice, 0, 0 );
			rc = LDAP_PROTOCOL_ERROR;
		}
	}

	Debug( LDAP_DEBUG_FILTER, "<= filter_matched_values %d\n", rc, 0, 0 );
	return( rc );
}

static int
test_ava_vrFilter(
	Operation	*op,
	Attribute	*a,
	AttributeAssertion *ava,
	int		type,
	char 		***e_flags )
{
	int 		i, j;

	for ( i=0; a != NULL; a = a->a_next, i++ ) {
		MatchingRule *mr;
		struct berval *bv;
	
		if ( !is_ad_subtype( a->a_desc, ava->aa_desc ) ) {
			continue;
		}

		switch ( type ) {
		case LDAP_FILTER_APPROX:
			mr = a->a_desc->ad_type->sat_approx;
			if( mr != NULL ) break;
			/* use EQUALITY matching rule if no APPROX rule */

		case LDAP_FILTER_EQUALITY:
			mr = a->a_desc->ad_type->sat_equality;
			break;
		
		case LDAP_FILTER_GE:
		case LDAP_FILTER_LE:
			mr = a->a_desc->ad_type->sat_ordering;
			break;

		default:
			mr = NULL;
		}

		if( mr == NULL ) continue;

		bv = a->a_nvals;
		for ( j=0; !BER_BVISNULL( bv ); bv++, j++ ) {
			int rc, match;
			const char *text;

			rc = value_match( &match, a->a_desc, mr, 0,
				bv, &ava->aa_value, &text );
			if( rc != LDAP_SUCCESS ) return rc;

			switch ( type ) {
			case LDAP_FILTER_EQUALITY:
			case LDAP_FILTER_APPROX:
				if ( match == 0 ) {
					(*e_flags)[i][j] = 1;
				}
				break;
	
			case LDAP_FILTER_GE:
				if ( match >= 0 ) {
					(*e_flags)[i][j] = 1;
				}
				break;
	
			case LDAP_FILTER_LE:
				if ( match <= 0 ) {
					(*e_flags)[i][j] = 1;
				}
				break;
			}
		}
	}
	return LDAP_SUCCESS;
}

static int
test_presence_vrFilter(
	Operation	*op,
	Attribute	*a,
	AttributeDescription *desc,
	char 		***e_flags )
{
	int i, j;

	for ( i=0; a != NULL; a = a->a_next, i++ ) {
		struct berval *bv;

		if ( !is_ad_subtype( a->a_desc, desc ) ) continue;

		for ( bv = a->a_vals, j = 0; !BER_BVISNULL( bv ); bv++, j++ );
		memset( (*e_flags)[i], 1, j);
	}

	return( LDAP_SUCCESS );
}

static int
test_substrings_vrFilter(
	Operation	*op,
	Attribute	*a,
	ValuesReturnFilter *vrf,
	char		***e_flags )
{
	int i, j;

	for ( i=0; a != NULL; a = a->a_next, i++ ) {
		MatchingRule *mr = a->a_desc->ad_type->sat_substr;
		struct berval *bv;

		if ( !is_ad_subtype( a->a_desc, vrf->vrf_sub_desc ) ) {
			continue;
		}

		if( mr == NULL ) continue;

		bv = a->a_nvals;
		for ( j = 0; !BER_BVISNULL( bv ); bv++, j++ ) {
			int rc, match;
			const char *text;

			rc = value_match( &match, a->a_desc, mr, 0,
				bv, vrf->vrf_sub, &text );

			if( rc != LDAP_SUCCESS ) return rc;

			if ( match == 0 ) {
				(*e_flags)[i][j] = 1;
			}
		}
	}

	return LDAP_SUCCESS;
}

static int
test_mra_vrFilter(
	Operation	*op,
	Attribute	*a,
	MatchingRuleAssertion *mra,
	char 		***e_flags )
{
	int	i, j;

	for ( i = 0; a != NULL; a = a->a_next, i++ ) {
		struct berval	*bv, assertedValue;
		int		normalize_attribute = 0;

		if ( mra->ma_desc ) {
			if ( !is_ad_subtype( a->a_desc, mra->ma_desc ) ) {
				continue;
			}
			assertedValue = mra->ma_value;

		} else {
			int rc;
			const char	*text = NULL;

			/* check if matching is appropriate */
			if ( !mr_usable_with_at( mra->ma_rule, a->a_desc->ad_type ) ) {
				continue;
			}

			rc = asserted_value_validate_normalize( a->a_desc, mra->ma_rule,
				SLAP_MR_EXT|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
				&mra->ma_value, &assertedValue, &text, op->o_tmpmemctx );

			if ( rc != LDAP_SUCCESS ) continue;
		}

		/* check match */
		if ( mra->ma_rule == a->a_desc->ad_type->sat_equality ) {
			bv = a->a_nvals;

		} else {
			bv = a->a_vals;
			normalize_attribute = 1;
		}
					
		for ( j = 0; !BER_BVISNULL( bv ); bv++, j++ ) {
			int		rc, match;
			const char	*text;
			struct berval	nbv = BER_BVNULL;

			if ( normalize_attribute && mra->ma_rule->smr_normalize ) {
				/* see comment in filterentry.c */
				if ( mra->ma_rule->smr_normalize(
						SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
						mra->ma_rule->smr_syntax,
						mra->ma_rule,
						bv, &nbv, op->o_tmpmemctx ) != LDAP_SUCCESS )
				{
					/* FIXME: stop processing? */
					continue;
				}

			} else {
				nbv = *bv;
			}

			rc = value_match( &match, a->a_desc, mra->ma_rule, 0,
				&nbv, &assertedValue, &text );

			if ( nbv.bv_val != bv->bv_val ) {
				op->o_tmpfree( nbv.bv_val, op->o_tmpmemctx );
			}

			if ( rc != LDAP_SUCCESS ) return rc;

			if ( match == 0 ) {
				(*e_flags)[i][j] = 1;
			}
		}
	}

	return LDAP_SUCCESS;
}

