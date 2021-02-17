/* filterentry.c - apply a filter to an entry */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"

#ifdef LDAP_COMP_MATCH
#include "component.h"
#endif

static int	test_filter_and( Operation *op, Entry *e, Filter *flist );
static int	test_filter_or( Operation *op, Entry *e, Filter *flist );
static int	test_substrings_filter( Operation *op, Entry *e, Filter *f);
static int	test_ava_filter( Operation *op,
	Entry *e, AttributeAssertion *ava, int type );
static int	test_mra_filter( Operation *op,
	Entry *e, MatchingRuleAssertion *mra );
static int	test_presence_filter( Operation *op,
	Entry *e, AttributeDescription *desc );


/*
 * test_filter - test a filter against a single entry.
 * returns:
 *		LDAP_COMPARE_TRUE		filter matched
 *		LDAP_COMPARE_FALSE		filter did not match
 *		SLAPD_COMPARE_UNDEFINED	filter is undefined
 *	or an ldap result code indicating error
 */

int
test_filter(
    Operation	*op,
    Entry	*e,
    Filter	*f )
{
	int	rc;
	Debug( LDAP_DEBUG_FILTER, "=> test_filter\n", 0, 0, 0 );

	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		Debug( LDAP_DEBUG_FILTER, "    UNDEFINED\n", 0, 0, 0 );
		rc = SLAPD_COMPARE_UNDEFINED;
		goto out;
	}

	switch ( f->f_choice ) {
	case SLAPD_FILTER_COMPUTED:
		Debug( LDAP_DEBUG_FILTER, "    COMPUTED %s (%d)\n",
			f->f_result == LDAP_COMPARE_FALSE ? "false" :
			f->f_result == LDAP_COMPARE_TRUE ? "true" :
			f->f_result == SLAPD_COMPARE_UNDEFINED ? "undefined" : "error",
			f->f_result, 0 );

		rc = f->f_result;
		break;

	case LDAP_FILTER_EQUALITY:
		Debug( LDAP_DEBUG_FILTER, "    EQUALITY\n", 0, 0, 0 );
		rc = test_ava_filter( op, e, f->f_ava, LDAP_FILTER_EQUALITY );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		Debug( LDAP_DEBUG_FILTER, "    SUBSTRINGS\n", 0, 0, 0 );
		rc = test_substrings_filter( op, e, f );
		break;

	case LDAP_FILTER_GE:
		Debug( LDAP_DEBUG_FILTER, "    GE\n", 0, 0, 0 );
		rc = test_ava_filter( op, e, f->f_ava, LDAP_FILTER_GE );
		break;

	case LDAP_FILTER_LE:
		Debug( LDAP_DEBUG_FILTER, "    LE\n", 0, 0, 0 );
		rc = test_ava_filter( op, e, f->f_ava, LDAP_FILTER_LE );
		break;

	case LDAP_FILTER_PRESENT:
		Debug( LDAP_DEBUG_FILTER, "    PRESENT\n", 0, 0, 0 );
		rc = test_presence_filter( op, e, f->f_desc );
		break;

	case LDAP_FILTER_APPROX:
		Debug( LDAP_DEBUG_FILTER, "    APPROX\n", 0, 0, 0 );
		rc = test_ava_filter( op, e, f->f_ava, LDAP_FILTER_APPROX );
		break;

	case LDAP_FILTER_AND:
		Debug( LDAP_DEBUG_FILTER, "    AND\n", 0, 0, 0 );
		rc = test_filter_and( op, e, f->f_and );
		break;

	case LDAP_FILTER_OR:
		Debug( LDAP_DEBUG_FILTER, "    OR\n", 0, 0, 0 );
		rc = test_filter_or( op, e, f->f_or );
		break;

	case LDAP_FILTER_NOT:
		Debug( LDAP_DEBUG_FILTER, "    NOT\n", 0, 0, 0 );
		rc = test_filter( op, e, f->f_not );

		/* Flip true to false and false to true
		 * but leave Undefined alone.
		 */
		switch( rc ) {
		case LDAP_COMPARE_TRUE:
			rc = LDAP_COMPARE_FALSE;
			break;
		case LDAP_COMPARE_FALSE:
			rc = LDAP_COMPARE_TRUE;
			break;
		}
		break;

	case LDAP_FILTER_EXT:
		Debug( LDAP_DEBUG_FILTER, "    EXT\n", 0, 0, 0 );
		rc = test_mra_filter( op, e, f->f_mra );
		break;

	default:
		Debug( LDAP_DEBUG_ANY, "    unknown filter type %lu\n",
		    f->f_choice, 0, 0 );
		rc = LDAP_PROTOCOL_ERROR;
	}
out:
	Debug( LDAP_DEBUG_FILTER, "<= test_filter %d\n", rc, 0, 0 );
	return( rc );
}

static int test_mra_filter(
	Operation *op,
	Entry *e,
	MatchingRuleAssertion *mra )
{
	Attribute	*a;
	void		*memctx;
	BER_MEMFREE_FN	*memfree;
#ifdef LDAP_COMP_MATCH
	int i, num_attr_vals = 0;
#endif

	if ( op == NULL ) {
		memctx = NULL;
		memfree = slap_sl_mfuncs.bmf_free;
	} else {
		memctx = op->o_tmpmemctx;
		memfree = op->o_tmpfree;
	}

	if ( mra->ma_desc ) {
		/*
		 * if ma_desc is available, then we're filtering for
		 * one attribute, and SEARCH permissions can be checked
		 * directly.
		 */
		if ( !access_allowed( op, e,
			mra->ma_desc, &mra->ma_value, ACL_SEARCH, NULL ) )
		{
			return LDAP_INSUFFICIENT_ACCESS;
		}

		if ( mra->ma_desc == slap_schema.si_ad_entryDN ) {
			int ret, rc;
			const char *text;

			rc = value_match( &ret, slap_schema.si_ad_entryDN, mra->ma_rule,
				SLAP_MR_EXT, &e->e_nname, &mra->ma_value, &text );
	
	
			if( rc != LDAP_SUCCESS ) return rc;
			if ( ret == 0 ) return LDAP_COMPARE_TRUE;
			return LDAP_COMPARE_FALSE;
		}

		for ( a = attrs_find( e->e_attrs, mra->ma_desc );
			a != NULL;
			a = attrs_find( a->a_next, mra->ma_desc ) )
		{
			struct berval	*bv;
			int		normalize_attribute = 0;

#ifdef LDAP_COMP_MATCH
			/* Component Matching */
			if ( mra->ma_cf && mra->ma_rule->smr_usage & SLAP_MR_COMPONENT ) {
				num_attr_vals = 0;
				if ( !a->a_comp_data ) {
					num_attr_vals = a->a_numvals;
					if ( num_attr_vals <= 0 ) {
						/* no attribute value */
						return LDAP_INAPPROPRIATE_MATCHING;
					}
					num_attr_vals++;

					/* following malloced will be freed by comp_tree_free () */
					a->a_comp_data = SLAP_MALLOC( sizeof( ComponentData ) +
						sizeof( ComponentSyntaxInfo* )*num_attr_vals );

					if ( !a->a_comp_data ) return LDAP_NO_MEMORY;
					a->a_comp_data->cd_tree = (ComponentSyntaxInfo**)
						((char*)a->a_comp_data + sizeof(ComponentData));
					a->a_comp_data->cd_tree[num_attr_vals - 1] =
						(ComponentSyntaxInfo*) NULL;
					a->a_comp_data->cd_mem_op =
						nibble_mem_allocator( 1024*16, 1024 );
				}
			}
#endif

			/* If ma_rule is not the same as the attribute's
			 * normal rule, then we can't use the a_nvals.
			 */
			if ( mra->ma_rule == a->a_desc->ad_type->sat_equality ) {
				bv = a->a_nvals;

			} else {
				bv = a->a_vals;
				normalize_attribute = 1;
			}
#ifdef LDAP_COMP_MATCH
			i = 0;
#endif
			for ( ; !BER_BVISNULL( bv ); bv++ ) {
				int ret;
				int rc;
				const char *text;
	
#ifdef LDAP_COMP_MATCH
				if ( mra->ma_cf &&
					mra->ma_rule->smr_usage & SLAP_MR_COMPONENT )
				{
					/* Check if decoded component trees are already linked */
					if ( num_attr_vals ) {
						a->a_comp_data->cd_tree[i] = attr_converter(
							a, a->a_desc->ad_type->sat_syntax, bv );
					}
					/* decoding error */
					if ( !a->a_comp_data->cd_tree[i] ) {
						return LDAP_OPERATIONS_ERROR;
					}
					rc = value_match( &ret, a->a_desc, mra->ma_rule,
						SLAP_MR_COMPONENT,
						(struct berval*)a->a_comp_data->cd_tree[i++],
						(void*)mra, &text );
				} else 
#endif
				{
					struct berval	nbv = BER_BVNULL;

					if ( normalize_attribute && mra->ma_rule->smr_normalize ) {
						/*
				
				Document: RFC 4511

				    4.5.1. Search Request 
				        ...
				    If the type field is present and the matchingRule is present, 
			            the matchValue is compared against entry attributes of the 
			            specified type. In this case, the matchingRule MUST be one 
				    suitable for use with the specified type (see [RFC4517]), 
				    otherwise the filter item is Undefined.  


				In this case, since the matchingRule requires the assertion
				value to be normalized, we normalize the attribute value
				according to the syntax of the matchingRule.

				This should likely be done inside value_match(), by passing
				the appropriate flags, but this is not done at present.
				See ITS#3406.
						 */
						if ( mra->ma_rule->smr_normalize(
								SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
								mra->ma_rule->smr_syntax,
								mra->ma_rule,
								bv, &nbv, memctx ) != LDAP_SUCCESS )
						{
							/* FIXME: stop processing? */
							continue;
						}

					} else {
						nbv = *bv;
					}

					rc = value_match( &ret, a->a_desc, mra->ma_rule,
						SLAP_MR_EXT, &nbv, &mra->ma_value, &text );

					if ( nbv.bv_val != bv->bv_val ) {
						memfree( nbv.bv_val, memctx );
					}
				}

				if ( rc != LDAP_SUCCESS ) return rc;
				if ( ret == 0 ) return LDAP_COMPARE_TRUE;
			}
		}

	} else {
		/*
		 * No attribute description: test all
		 */
		for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
			struct berval	*bv, value;
			const char	*text = NULL;
			int		rc;
			int		normalize_attribute = 0;

			/* check if matching is appropriate */
			if ( !mr_usable_with_at( mra->ma_rule, a->a_desc->ad_type ) ) {
				continue;
			}

			/* normalize for equality */
			rc = asserted_value_validate_normalize( a->a_desc, mra->ma_rule,
				SLAP_MR_EXT|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
				&mra->ma_value, &value, &text, memctx );
			if ( rc != LDAP_SUCCESS ) continue;

			/* check search access */
			if ( !access_allowed( op, e,
				a->a_desc, &value, ACL_SEARCH, NULL ) )
			{
				memfree( value.bv_val, memctx );
				continue;
			}
#ifdef LDAP_COMP_MATCH
			/* Component Matching */
			if ( mra->ma_cf &&
				mra->ma_rule->smr_usage & SLAP_MR_COMPONENT )
			{
				int ret;

				rc = value_match( &ret, a->a_desc, mra->ma_rule,
					SLAP_MR_COMPONENT,
					(struct berval*)a, (void*)mra, &text );
				if ( rc != LDAP_SUCCESS ) break;
	
				if ( ret == 0 ) {
					rc = LDAP_COMPARE_TRUE;
					break;
				}

			}
#endif

			/* check match */
			if ( mra->ma_rule == a->a_desc->ad_type->sat_equality ) {
				bv = a->a_nvals;

			} else {
				bv = a->a_vals;
				normalize_attribute = 1;
			}

			for ( ; !BER_BVISNULL( bv ); bv++ ) {
				int		ret;
				struct berval	nbv = BER_BVNULL;

				if ( normalize_attribute && mra->ma_rule->smr_normalize ) {
					/* see comment above */
					if ( mra->ma_rule->smr_normalize(
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							mra->ma_rule->smr_syntax,
							mra->ma_rule,
							bv, &nbv, memctx ) != LDAP_SUCCESS )
					{
						/* FIXME: stop processing? */
						continue;
					}

				} else {
					nbv = *bv;
				}

				rc = value_match( &ret, a->a_desc, mra->ma_rule,
					SLAP_MR_EXT, &nbv, &value, &text );

				if ( nbv.bv_val != bv->bv_val ) {
					memfree( nbv.bv_val, memctx );
				}

				if ( rc != LDAP_SUCCESS ) break;
	
				if ( ret == 0 ) {
					rc = LDAP_COMPARE_TRUE;
					break;
				}
			}
			memfree( value.bv_val, memctx );
			if ( rc != LDAP_SUCCESS ) return rc;
		}
	}

	/* check attrs in DN AVAs if required */
	if ( mra->ma_dnattrs && !BER_BVISEMPTY( &e->e_nname ) ) {
		LDAPDN		dn = NULL;
		int		iRDN, iAVA;
		int		rc;

		/* parse and pretty the dn */
		rc = dnPrettyDN( NULL, &e->e_name, &dn, memctx );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		/* for each AVA of each RDN ... */
		for ( iRDN = 0; dn[ iRDN ]; iRDN++ ) {
			LDAPRDN		rdn = dn[ iRDN ];

			for ( iAVA = 0; rdn[ iAVA ]; iAVA++ ) {
				LDAPAVA		*ava = rdn[ iAVA ];
				struct berval	*bv = &ava->la_value,
						value = BER_BVNULL,
						nbv = BER_BVNULL;
				AttributeDescription *ad =
					(AttributeDescription *)ava->la_private;
				int		ret;
				const char	*text;

				assert( ad != NULL );

				if ( mra->ma_desc ) {
					/* have a mra type? check for subtype */
					if ( !is_ad_subtype( ad, mra->ma_desc ) ) {
						continue;
					}
					value = mra->ma_value;

				} else {
					const char	*text = NULL;

					/* check if matching is appropriate */
					if ( !mr_usable_with_at( mra->ma_rule, ad->ad_type ) ) {
						continue;
					}

					/* normalize for equality */
					rc = asserted_value_validate_normalize( ad,
						mra->ma_rule,
						SLAP_MR_EXT|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
						&mra->ma_value, &value, &text, memctx );
					if ( rc != LDAP_SUCCESS ) continue;

					/* check search access */
					if ( !access_allowed( op, e,
						ad, &value, ACL_SEARCH, NULL ) )
					{
						memfree( value.bv_val, memctx );
						continue;
					}
				}

				if ( mra->ma_rule->smr_normalize ) {
					/* see comment above */
					if ( mra->ma_rule->smr_normalize(
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							mra->ma_rule->smr_syntax,
							mra->ma_rule,
							bv, &nbv, memctx ) != LDAP_SUCCESS )
					{
						/* FIXME: stop processing? */
						rc = LDAP_SUCCESS;
						ret = -1;
						goto cleanup;
					}

				} else {
					nbv = *bv;
				}

				/* check match */
				rc = value_match( &ret, ad, mra->ma_rule, SLAP_MR_EXT,
					&nbv, &value, &text );

cleanup:;
				if ( !BER_BVISNULL( &value ) && value.bv_val != mra->ma_value.bv_val ) {
					memfree( value.bv_val, memctx );
				}

				if ( !BER_BVISNULL( &nbv ) && nbv.bv_val != bv->bv_val ) {
					memfree( nbv.bv_val, memctx );
				}

				if ( rc == LDAP_SUCCESS && ret == 0 ) rc = LDAP_COMPARE_TRUE;

				if ( rc != LDAP_SUCCESS ) {
					ldap_dnfree_x( dn, memctx );
					return rc;
				}
			}
		}
		ldap_dnfree_x( dn, memctx );
	}

	return LDAP_COMPARE_FALSE;
}

static int
test_ava_filter(
	Operation	*op,
	Entry		*e,
	AttributeAssertion *ava,
	int		type )
{
	int rc;
	Attribute	*a;
#ifdef LDAP_COMP_MATCH
	int i, num_attr_vals = 0;
	AttributeAliasing *a_alias = NULL;
#endif

	if ( !access_allowed( op, e,
		ava->aa_desc, &ava->aa_value, ACL_SEARCH, NULL ) )
	{
		return LDAP_INSUFFICIENT_ACCESS;
	}

	if ( ava->aa_desc == slap_schema.si_ad_hasSubordinates 
		&& op && op->o_bd && op->o_bd->be_has_subordinates )
	{
		int	hasSubordinates = 0;
		struct berval hs;

		if( type != LDAP_FILTER_EQUALITY &&
			type != LDAP_FILTER_APPROX )
		{
			/* No other match is allowed */
			return LDAP_INAPPROPRIATE_MATCHING;
		}
		
		if ( op->o_bd->be_has_subordinates( op, e, &hasSubordinates ) !=
			LDAP_SUCCESS )
		{
			return LDAP_OTHER;
		}

		if ( hasSubordinates == LDAP_COMPARE_TRUE ) {
			hs = slap_true_bv;

		} else if ( hasSubordinates == LDAP_COMPARE_FALSE ) {
			hs = slap_false_bv;

		} else {
			return LDAP_OTHER;
		}

		if ( bvmatch( &ava->aa_value, &hs ) ) return LDAP_COMPARE_TRUE;
		return LDAP_COMPARE_FALSE;
	}

	if ( ava->aa_desc == slap_schema.si_ad_entryDN ) {
		MatchingRule *mr;
		int match;
		const char *text;

		if( type != LDAP_FILTER_EQUALITY &&
			type != LDAP_FILTER_APPROX )
		{
			/* No other match is allowed */
			return LDAP_INAPPROPRIATE_MATCHING;
		}

		mr = slap_schema.si_ad_entryDN->ad_type->sat_equality;
		assert( mr != NULL );

		rc = value_match( &match, slap_schema.si_ad_entryDN, mr,
			SLAP_MR_EXT, &e->e_nname, &ava->aa_value, &text );

		if( rc != LDAP_SUCCESS ) return rc;
		if( match == 0 ) return LDAP_COMPARE_TRUE;
		return LDAP_COMPARE_FALSE;
	}

	rc = LDAP_COMPARE_FALSE;

#ifdef LDAP_COMP_MATCH
	if ( is_aliased_attribute && ava->aa_cf )
	{
		a_alias = is_aliased_attribute ( ava->aa_desc );
		if ( a_alias )
			ava->aa_desc = a_alias->aa_aliased_ad;
		else
			ava->aa_cf = NULL;
	}
#endif

	for(a = attrs_find( e->e_attrs, ava->aa_desc );
		a != NULL;
		a = attrs_find( a->a_next, ava->aa_desc ) )
	{
		int use;
		MatchingRule *mr;
		struct berval *bv;

		if (( ava->aa_desc != a->a_desc ) && !access_allowed( op,
			e, a->a_desc, &ava->aa_value, ACL_SEARCH, NULL ))
		{
			rc = LDAP_INSUFFICIENT_ACCESS;
			continue;
		}

		use = SLAP_MR_EQUALITY;

		switch ( type ) {
		case LDAP_FILTER_APPROX:
			use = SLAP_MR_EQUALITY_APPROX;
			mr = a->a_desc->ad_type->sat_approx;
			if( mr != NULL ) break;

			/* fallthru: use EQUALITY matching rule if no APPROX rule */

		case LDAP_FILTER_EQUALITY:
			/* use variable set above so fall thru use is not clobbered */
			mr = a->a_desc->ad_type->sat_equality;
			break;

		case LDAP_FILTER_GE:
		case LDAP_FILTER_LE:
			use = SLAP_MR_ORDERING;
			mr = a->a_desc->ad_type->sat_ordering;
			break;

		default:
			mr = NULL;
		}

		if( mr == NULL ) {
			rc = LDAP_INAPPROPRIATE_MATCHING;
			continue;
		}

		/* We have no Sort optimization for Approx matches */
		if (( a->a_flags & SLAP_ATTR_SORTED_VALS ) && type != LDAP_FILTER_APPROX ) {
			unsigned slot;
			int ret;

			/* For Ordering matches, we just need to do one comparison with
			 * either the first (least) or last (greatest) value.
			 */
			if ( use == SLAP_MR_ORDERING ) {
				const char *text;
				int match, which;
				which = (type == LDAP_FILTER_LE) ? 0 : a->a_numvals-1;
				ret = value_match( &match, a->a_desc, mr, use,
					&a->a_nvals[which], &ava->aa_value, &text );
				if ( ret != LDAP_SUCCESS ) return ret;
				if (( type == LDAP_FILTER_LE && match <= 0 ) ||
					( type == LDAP_FILTER_GE && match >= 0 ))
					return LDAP_COMPARE_TRUE;
				continue;
			}
			/* Only Equality will get here */
			ret = attr_valfind( a, use | SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH, 
				&ava->aa_value, &slot, NULL );
			if ( ret == LDAP_SUCCESS )
				return LDAP_COMPARE_TRUE;
			else if ( ret != LDAP_NO_SUCH_ATTRIBUTE )
				return ret;
#if 0
			/* The following is useful if we want to know which values
			 * matched an ordering test. But here we don't care, we just
			 * want to know if any value did, and that is checked above.
			 */
			if ( ret == LDAP_NO_SUCH_ATTRIBUTE ) {
				/* If insertion point is not the end of the list, there was
				 * at least one value greater than the assertion.
				 */
				if ( type == LDAP_FILTER_GE && slot < a->a_numvals )
					return LDAP_COMPARE_TRUE;
				/* Likewise, if insertion point is not the head of the list,
				 * there was at least one value less than the assertion.
				 */
				if ( type == LDAP_FILTER_LE && slot > 0 )
					return LDAP_COMPARE_TRUE;
				return LDAP_COMPARE_FALSE;
			}
#endif
			continue;
		}

#ifdef LDAP_COMP_MATCH
		if ( nibble_mem_allocator && ava->aa_cf && !a->a_comp_data ) {
			/* Component Matching */
			for ( num_attr_vals = 0; a->a_vals[num_attr_vals].bv_val != NULL; num_attr_vals++ );
			if ( num_attr_vals <= 0 )/* no attribute value */
				return LDAP_INAPPROPRIATE_MATCHING;
			num_attr_vals++;/* for NULL termination */

			/* following malloced will be freed by comp_tree_free () */
			a->a_comp_data = SLAP_MALLOC( sizeof( ComponentData ) + sizeof( ComponentSyntaxInfo* )*num_attr_vals );

			if ( !a->a_comp_data ) {
				return LDAP_NO_MEMORY;
			}

			a->a_comp_data->cd_tree = (ComponentSyntaxInfo**)((char*)a->a_comp_data + sizeof(ComponentData));
			i = num_attr_vals;
			for ( ; i ; i-- ) {
				a->a_comp_data->cd_tree[ i-1 ] = (ComponentSyntaxInfo*)NULL;
			}

			a->a_comp_data->cd_mem_op = nibble_mem_allocator ( 1024*10*(num_attr_vals-1), 1024 );
			if ( a->a_comp_data->cd_mem_op == NULL ) {
				free ( a->a_comp_data );
				a->a_comp_data = NULL;
				return LDAP_OPERATIONS_ERROR;
			}
		}

		i = 0;
#endif

		for ( bv = a->a_nvals; !BER_BVISNULL( bv ); bv++ ) {
			int ret, match;
			const char *text;

#ifdef LDAP_COMP_MATCH
			if( attr_converter && ava->aa_cf && a->a_comp_data ) {
				/* Check if decoded component trees are already linked */
				struct berval cf_bv = { 20, "componentFilterMatch" };
				MatchingRule* cf_mr = mr_bvfind( &cf_bv );
				MatchingRuleAssertion mra;
				mra.ma_cf = ava->aa_cf;

				if ( a->a_comp_data->cd_tree[i] == NULL )
					a->a_comp_data->cd_tree[i] = attr_converter (a, a->a_desc->ad_type->sat_syntax, (a->a_vals + i));
				/* decoding error */
				if ( !a->a_comp_data->cd_tree[i] ) {
					free_ComponentData ( a );
					return LDAP_OPERATIONS_ERROR;
				}

				ret = value_match( &match, a->a_desc, cf_mr,
					SLAP_MR_COMPONENT,
					(struct berval*)a->a_comp_data->cd_tree[i++],
					(void*)&mra, &text );
				if ( ret == LDAP_INAPPROPRIATE_MATCHING ) {
					/* cached component tree is broken, just remove it */
					free_ComponentData ( a );
					return ret;
				}
				if ( a_alias )
					ava->aa_desc = a_alias->aa_aliasing_ad;

			} else 
#endif
			{
				ret = ordered_value_match( &match, a->a_desc, mr, use,
					bv, &ava->aa_value, &text );
			}

			if( ret != LDAP_SUCCESS ) {
				rc = ret;
				break;
			}

			switch ( type ) {
			case LDAP_FILTER_EQUALITY:
			case LDAP_FILTER_APPROX:
				if ( match == 0 ) return LDAP_COMPARE_TRUE;
				break;

			case LDAP_FILTER_GE:
				if ( match >= 0 ) return LDAP_COMPARE_TRUE;
				break;

			case LDAP_FILTER_LE:
				if ( match <= 0 ) return LDAP_COMPARE_TRUE;
				break;
			}
		}
	}

#ifdef LDAP_COMP_MATCH
	if ( a_alias )
		ava->aa_desc = a_alias->aa_aliasing_ad;
#endif

	return rc;
}


static int
test_presence_filter(
	Operation	*op,
	Entry		*e,
	AttributeDescription *desc )
{
	Attribute	*a;
	int rc;

	if ( !access_allowed( op, e, desc, NULL, ACL_SEARCH, NULL ) ) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	if ( desc == slap_schema.si_ad_hasSubordinates ) {
		/*
		 * XXX: fairly optimistic: if the function is defined,
		 * then PRESENCE must succeed, because hasSubordinate
		 * is boolean-valued; I think we may live with this 
		 * simplification by now.
		 */
		if ( op && op->o_bd && op->o_bd->be_has_subordinates ) {
			return LDAP_COMPARE_TRUE;
		}

		return LDAP_COMPARE_FALSE;
	}

	if ( desc == slap_schema.si_ad_entryDN ||
		desc == slap_schema.si_ad_subschemaSubentry )
	{
		/* entryDN and subschemaSubentry are always present */
		return LDAP_COMPARE_TRUE;
	}

	rc = LDAP_COMPARE_FALSE;

	for(a = attrs_find( e->e_attrs, desc );
		a != NULL;
		a = attrs_find( a->a_next, desc ) )
	{
		if (( desc != a->a_desc ) && !access_allowed( op,
			e, a->a_desc, NULL, ACL_SEARCH, NULL ))
		{
			rc = LDAP_INSUFFICIENT_ACCESS;
			continue;
		}

		rc = LDAP_COMPARE_TRUE;
		break;
	}

	return rc;
}


static int
test_filter_and(
	Operation	*op,
	Entry	*e,
	Filter	*flist )
{
	Filter	*f;
	int rtn = LDAP_COMPARE_TRUE; /* True if empty */

	Debug( LDAP_DEBUG_FILTER, "=> test_filter_and\n", 0, 0, 0 );

	for ( f = flist; f != NULL; f = f->f_next ) {
		int rc = test_filter( op, e, f );

		if ( rc == LDAP_COMPARE_FALSE ) {
			/* filter is False */
			rtn = rc;
			break;
		}

		if ( rc != LDAP_COMPARE_TRUE ) {
			/* filter is Undefined unless later elements are False */
			rtn = rc;
		}
	}

	Debug( LDAP_DEBUG_FILTER, "<= test_filter_and %d\n", rtn, 0, 0 );

	return rtn;
}

static int
test_filter_or(
	Operation	*op,
	Entry	*e,
	Filter	*flist )
{
	Filter	*f;
	int rtn = LDAP_COMPARE_FALSE; /* False if empty */

	Debug( LDAP_DEBUG_FILTER, "=> test_filter_or\n", 0, 0, 0 );

	for ( f = flist; f != NULL; f = f->f_next ) {
		int rc = test_filter( op, e, f );

		if ( rc == LDAP_COMPARE_TRUE ) {
			/* filter is True */
			rtn = rc;
			break;
		}

		if ( rc != LDAP_COMPARE_FALSE ) {
			/* filter is Undefined unless later elements are True */
			rtn = rc;
		}
	}

	Debug( LDAP_DEBUG_FILTER, "<= test_filter_or %d\n", rtn, 0, 0 );
	return rtn;
}


static int
test_substrings_filter(
	Operation	*op,
	Entry	*e,
	Filter	*f )
{
	Attribute	*a;
	int rc;

	Debug( LDAP_DEBUG_FILTER, "begin test_substrings_filter\n", 0, 0, 0 );

	if ( !access_allowed( op, e,
		f->f_sub_desc, NULL, ACL_SEARCH, NULL ) )
	{
		return LDAP_INSUFFICIENT_ACCESS;
	}

	rc = LDAP_COMPARE_FALSE;

	for(a = attrs_find( e->e_attrs, f->f_sub_desc );
		a != NULL;
		a = attrs_find( a->a_next, f->f_sub_desc ) )
	{
		MatchingRule *mr;
		struct berval *bv;

		if (( f->f_sub_desc != a->a_desc ) && !access_allowed( op,
			e, a->a_desc, NULL, ACL_SEARCH, NULL ))
		{
			rc = LDAP_INSUFFICIENT_ACCESS;
			continue;
		}

		mr = a->a_desc->ad_type->sat_substr;
		if( mr == NULL ) {
			rc = LDAP_INAPPROPRIATE_MATCHING;
			continue;
		}

		for ( bv = a->a_nvals; !BER_BVISNULL( bv ); bv++ ) {
			int ret, match;
			const char *text;

			ret = value_match( &match, a->a_desc, mr, SLAP_MR_SUBSTR,
				bv, f->f_sub, &text );

			if( ret != LDAP_SUCCESS ) {
				rc = ret;
				break;
			}
			if ( match == 0 ) return LDAP_COMPARE_TRUE;
		}
	}

	Debug( LDAP_DEBUG_FILTER, "end test_substrings_filter %d\n",
		rc, 0, 0 );
	return rc;
}
