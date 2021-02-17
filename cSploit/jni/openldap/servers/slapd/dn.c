/* dn.c - routines for dealing with distinguished names */
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

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"

/*
 * The DN syntax-related functions take advantage of the dn representation
 * handling functions ldap_str2dn/ldap_dn2str.  The latter are not schema-
 * aware, so the attributes and their values need be validated (and possibly
 * normalized).  In the current implementation the required validation/nor-
 * malization/"pretty"ing are done on newly created DN structural represen-
 * tations; however the idea is to move towards DN handling in structural
 * representation instead of the current string representation.  To this
 * purpose, we need to do only the required operations and keep track of
 * what has been done to minimize their impact on performances.
 *
 * Developers are strongly encouraged to use this feature, to speed-up
 * its stabilization.
 */

#define	AVA_PRIVATE( ava ) ( ( AttributeDescription * )(ava)->la_private )

int slap_DN_strict = SLAP_AD_NOINSERT;

static int
LDAPRDN_validate( LDAPRDN rdn )
{
	int		iAVA;
	int 		rc;

	assert( rdn != NULL );

	for ( iAVA = 0; rdn[ iAVA ]; iAVA++ ) {
		LDAPAVA			*ava = rdn[ iAVA ];
		AttributeDescription	*ad;
		slap_syntax_validate_func *validate = NULL;

		assert( ava != NULL );
		
		if ( ( ad = AVA_PRIVATE( ava ) ) == NULL ) {
			const char	*text = NULL;

			rc = slap_bv2ad( &ava->la_attr, &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				rc = slap_bv2undef_ad( &ava->la_attr,
					&ad, &text,
					SLAP_AD_PROXIED|slap_DN_strict );
				if ( rc != LDAP_SUCCESS ) {
					return LDAP_INVALID_SYNTAX;
				}
			}

			ava->la_private = ( void * )ad;
		}

		/*
		 * Do not allow X-ORDERED 'VALUES' naming attributes
		 */
		if ( ad->ad_type->sat_flags & SLAP_AT_ORDERED_VAL ) {
			return LDAP_INVALID_SYNTAX;
		}

		/* 
		 * Replace attr oid/name with the canonical name
		 */
		ava->la_attr = ad->ad_cname;

		validate = ad->ad_type->sat_syntax->ssyn_validate;

		if ( validate ) {
			/*
		 	 * validate value by validate function
			 */
			rc = ( *validate )( ad->ad_type->sat_syntax,
				&ava->la_value );
			
			if ( rc != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
		}
	}

	return LDAP_SUCCESS;
}

/*
 * In-place, schema-aware validation of the
 * structural representation of a distinguished name.
 */
static int
LDAPDN_validate( LDAPDN dn )
{
	int 		iRDN;
	int 		rc;

	assert( dn != NULL );

	for ( iRDN = 0; dn[ iRDN ]; iRDN++ ) {
		rc = LDAPRDN_validate( dn[ iRDN ] );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	return LDAP_SUCCESS;
}

/*
 * dn validate routine
 */
int
dnValidate(
	Syntax *syntax,
	struct berval *in )
{
	int		rc;
	LDAPDN		dn = NULL;

	assert( in != NULL );

	if ( in->bv_len == 0 ) {
		return LDAP_SUCCESS;

	} else if ( in->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;
	}

	rc = ldap_bv2dn( in, &dn, LDAP_DN_FORMAT_LDAP );
	if ( rc != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	assert( strlen( in->bv_val ) == in->bv_len );

	/*
	 * Schema-aware validate
	 */
	rc = LDAPDN_validate( dn );
	ldap_dnfree( dn );

	if ( rc != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

int
rdnValidate(
	Syntax *syntax,
	struct berval *in )
{
	int		rc;
	LDAPRDN		rdn;
	char*		p;

	assert( in != NULL );
	if ( in->bv_len == 0 ) {
		return LDAP_SUCCESS;

	} else if ( in->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;
	}

	rc = ldap_bv2rdn_x( in , &rdn, (char **) &p,
				LDAP_DN_FORMAT_LDAP, NULL);
	if ( rc != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	assert( strlen( in->bv_val ) == in->bv_len );

	/*
	 * Schema-aware validate
	 */
	rc = LDAPRDN_validate( rdn );
	ldap_rdnfree( rdn );

	if ( rc != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}


/*
 * AVA sorting inside a RDN
 *
 * Rule: sort attributeTypes in alphabetical order.
 *
 * Note: the sorting can be slightly improved by sorting first
 * by attribute type length, then by alphabetical order.
 *
 * uses an insertion sort; should be fine since the number of AVAs in
 * a RDN should be limited.
 */
static int
AVA_Sort( LDAPRDN rdn, int nAVAs )
{
	LDAPAVA	*ava_i;
	int		i;

	assert( rdn != NULL );

	for ( i = 1; i < nAVAs; i++ ) {
		LDAPAVA *ava_j;
		int j;

		ava_i = rdn[ i ];
		for ( j = i-1; j >=0; j-- ) {
			int a;

			ava_j = rdn[ j ];
			a = strcmp( ava_i->la_attr.bv_val, ava_j->la_attr.bv_val );

			/* RFC4512 does not allow multiple AVAs
			 * with the same attribute type in RDN (ITS#5968) */
			if ( a == 0 )
				return LDAP_INVALID_DN_SYNTAX;

			if ( a > 0 )
				break;

			rdn[ j+1 ] = rdn[ j ];
		}
		rdn[ j+1 ] = ava_i;
	}
	return LDAP_SUCCESS;
}

static int
LDAPRDN_rewrite( LDAPRDN rdn, unsigned flags, void *ctx )
{

	int rc, iAVA, do_sort = 0;

	for ( iAVA = 0; rdn[ iAVA ]; iAVA++ ) {
		LDAPAVA			*ava = rdn[ iAVA ];
		AttributeDescription	*ad;
		slap_syntax_validate_func *validf = NULL;
		slap_mr_normalize_func *normf = NULL;
		slap_syntax_transform_func *transf = NULL;
		MatchingRule *mr = NULL;
		struct berval		bv = BER_BVNULL;

		assert( ava != NULL );

		if ( ( ad = AVA_PRIVATE( ava ) ) == NULL ) {
			const char	*text = NULL;

			rc = slap_bv2ad( &ava->la_attr, &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				rc = slap_bv2undef_ad( &ava->la_attr,
					&ad, &text,
					SLAP_AD_PROXIED|slap_DN_strict );
				if ( rc != LDAP_SUCCESS ) {
					return LDAP_INVALID_SYNTAX;
				}
			}
			
			ava->la_private = ( void * )ad;
			do_sort = 1;
		}

		/* 
		 * Replace attr oid/name with the canonical name
		 */
		ava->la_attr = ad->ad_cname;

		if( ava->la_flags & LDAP_AVA_BINARY ) {
			/* AVA is binary encoded, not supported */
			return LDAP_INVALID_SYNTAX;

			/* Do not allow X-ORDERED 'VALUES' naming attributes */
		} else if( ad->ad_type->sat_flags & SLAP_AT_ORDERED_VAL ) {
			return LDAP_INVALID_SYNTAX;

		} else if( flags & SLAP_LDAPDN_PRETTY ) {
			transf = ad->ad_type->sat_syntax->ssyn_pretty;
			if( !transf ) {
				validf = ad->ad_type->sat_syntax->ssyn_validate;
			}
		} else { /* normalization */
			validf = ad->ad_type->sat_syntax->ssyn_validate;
			mr = ad->ad_type->sat_equality;
			if( mr && (!( mr->smr_usage & SLAP_MR_MUTATION_NORMALIZER ))) {
				normf = mr->smr_normalize;
			}
		}

		if ( validf ) {
			/* validate value before normalization */
			rc = ( *validf )( ad->ad_type->sat_syntax,
				ava->la_value.bv_len
					? &ava->la_value
					: (struct berval *) &slap_empty_bv );

			if ( rc != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
		}

		if ( transf ) {
			/*
		 	 * transform value by pretty function
			 *	if value is empty, use empty_bv
			 */
			rc = ( *transf )( ad->ad_type->sat_syntax,
				ava->la_value.bv_len
					? &ava->la_value
					: (struct berval *) &slap_empty_bv,
				&bv, ctx );
		
			if ( rc != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
		}

		if ( normf ) {
			/*
		 	 * normalize value
			 *	if value is empty, use empty_bv
			 */
			rc = ( *normf )(
				SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
				ad->ad_type->sat_syntax,
				mr,
				ava->la_value.bv_len
					? &ava->la_value
					: (struct berval *) &slap_empty_bv,
				&bv, ctx );
		
			if ( rc != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
		}


		if( bv.bv_val ) {
			if ( ava->la_flags & LDAP_AVA_FREE_VALUE )
				ber_memfree_x( ava->la_value.bv_val, ctx );
			ava->la_value = bv;
			ava->la_flags |= LDAP_AVA_FREE_VALUE;
		}
		/* reject empty values */
		if (!ava->la_value.bv_len) {
			return LDAP_INVALID_SYNTAX;
		}
	}
	rc = LDAP_SUCCESS;

	if ( do_sort ) {
		rc = AVA_Sort( rdn, iAVA );
	}

	return rc;
}

/*
 * In-place, schema-aware normalization / "pretty"ing of the
 * structural representation of a distinguished name.
 */
static int
LDAPDN_rewrite( LDAPDN dn, unsigned flags, void *ctx )
{
	int 		iRDN;
	int 		rc;

	assert( dn != NULL );

	for ( iRDN = 0; dn[ iRDN ]; iRDN++ ) {
		rc = LDAPRDN_rewrite( dn[ iRDN ], flags, ctx );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	return LDAP_SUCCESS;
}

int
dnNormalize(
    slap_mask_t use,
    Syntax *syntax,
    MatchingRule *mr,
    struct berval *val,
    struct berval *out,
    void *ctx)
{
	assert( val != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> dnNormalize: <%s>\n", val->bv_val ? val->bv_val : "", 0, 0 );

	if ( val->bv_len != 0 ) {
		LDAPDN		dn = NULL;
		int		rc;

		/*
		 * Go to structural representation
		 */
		rc = ldap_bv2dn_x( val, &dn, LDAP_DN_FORMAT_LDAP, ctx );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPDN_rewrite( dn, 0, ctx ) != LDAP_SUCCESS ) {
			ldap_dnfree_x( dn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		/*
		 * Back to string representation
		 */
		rc = ldap_dn2bv_x( dn, out,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		ldap_dnfree_x( dn, ctx );

		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	} else {
		ber_dupbv_x( out, val, ctx );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dnNormalize: <%s>\n", out->bv_val ? out->bv_val : "", 0, 0 );

	return LDAP_SUCCESS;
}

int
rdnNormalize(
    slap_mask_t use,
    Syntax *syntax,
    MatchingRule *mr,
    struct berval *val,
    struct berval *out,
    void *ctx)
{
	assert( val != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> dnNormalize: <%s>\n", val->bv_val ? val->bv_val : "", 0, 0 );
	if ( val->bv_len != 0 ) {
		LDAPRDN		rdn = NULL;
		int		rc;
		char*		p;

		/*
		 * Go to structural representation
		 */
		rc = ldap_bv2rdn_x( val , &rdn, (char **) &p,
					LDAP_DN_FORMAT_LDAP, ctx);

		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPRDN_rewrite( rdn, 0, ctx ) != LDAP_SUCCESS ) {
			ldap_rdnfree_x( rdn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		/*
		 * Back to string representation
		 */
		rc = ldap_rdn2bv_x( rdn, out,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		ldap_rdnfree_x( rdn, ctx );

		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	} else {
		ber_dupbv_x( out, val, ctx );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dnNormalize: <%s>\n", out->bv_val ? out->bv_val : "", 0, 0 );

	return LDAP_SUCCESS;
}

int
dnPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx)
{
	assert( val != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> dnPretty: <%s>\n", val->bv_val ? val->bv_val : "", 0, 0 );

	if ( val->bv_len == 0 ) {
		ber_dupbv_x( out, val, ctx );

	} else if ( val->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;

	} else {
		LDAPDN		dn = NULL;
		int		rc;

		/* FIXME: should be liberal in what we accept */
		rc = ldap_bv2dn_x( val, &dn, LDAP_DN_FORMAT_LDAP, ctx );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPDN_rewrite( dn, SLAP_LDAPDN_PRETTY, ctx ) != LDAP_SUCCESS ) {
			ldap_dnfree_x( dn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		/* FIXME: not sure why the default isn't pretty */
		/* RE: the default is the form that is used as
		 * an internal representation; the pretty form
		 * is a variant */
		rc = ldap_dn2bv_x( dn, out,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		ldap_dnfree_x( dn, ctx );

		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dnPretty: <%s>\n", out->bv_val ? out->bv_val : "", 0, 0 );

	return LDAP_SUCCESS;
}

int
rdnPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx)
{
	assert( val != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> rdnPretty: <%s>\n", val->bv_val ? val->bv_val : "", 0, 0 );

	if ( val->bv_len == 0 ) {
		ber_dupbv_x( out, val, ctx );

	} else if ( val->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;

	} else {
		LDAPRDN		rdn = NULL;
		int		rc;
		char*		p;

		/* FIXME: should be liberal in what we accept */
		rc = ldap_bv2rdn_x( val , &rdn, (char **) &p,
					LDAP_DN_FORMAT_LDAP, ctx);
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPRDN_rewrite( rdn, SLAP_LDAPDN_PRETTY, ctx ) != LDAP_SUCCESS ) {
			ldap_rdnfree_x( rdn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		/* FIXME: not sure why the default isn't pretty */
		/* RE: the default is the form that is used as
		 * an internal representation; the pretty form
		 * is a variant */
		rc = ldap_rdn2bv_x( rdn, out,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		ldap_rdnfree_x( rdn, ctx );

		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dnPretty: <%s>\n", out->bv_val ? out->bv_val : "", 0, 0 );

	return LDAP_SUCCESS;
}


int
dnPrettyNormalDN(
	Syntax *syntax,
	struct berval *val,
	LDAPDN *dn,
	int flags,
	void *ctx )
{
	assert( val != NULL );
	assert( dn != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> dn%sDN: <%s>\n", 
			flags == SLAP_LDAPDN_PRETTY ? "Pretty" : "Normal", 
			val->bv_val ? val->bv_val : "", 0 );

	if ( val->bv_len == 0 ) {
		return LDAP_SUCCESS;

	} else if ( val->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;

	} else {
		int		rc;

		/* FIXME: should be liberal in what we accept */
		rc = ldap_bv2dn_x( val, dn, LDAP_DN_FORMAT_LDAP, ctx );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPDN_rewrite( *dn, flags, ctx ) != LDAP_SUCCESS ) {
			ldap_dnfree_x( *dn, ctx );
			*dn = NULL;
			return LDAP_INVALID_SYNTAX;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dn%sDN\n", 
			flags == SLAP_LDAPDN_PRETTY ? "Pretty" : "Normal",
			0, 0 );

	return LDAP_SUCCESS;
}

/*
 * Combination of both dnPretty and dnNormalize
 */
int
dnPrettyNormal(
	Syntax *syntax,
	struct berval *val,
	struct berval *pretty,
	struct berval *normal,
	void *ctx)
{
	assert( val != NULL );
	assert( pretty != NULL );
	assert( normal != NULL );
	Debug( LDAP_DEBUG_TRACE, ">>> dnPrettyNormal: <%s>\n", val->bv_val ? val->bv_val : "", 0, 0 );

	if ( val->bv_len == 0 ) {
		ber_dupbv_x( pretty, val, ctx );
		ber_dupbv_x( normal, val, ctx );

	} else if ( val->bv_len > SLAP_LDAPDN_MAXLEN ) {
		/* too big */
		return LDAP_INVALID_SYNTAX;

	} else {
		LDAPDN		dn = NULL;
		int		rc;

		pretty->bv_val = NULL;
		normal->bv_val = NULL;
		pretty->bv_len = 0;
		normal->bv_len = 0;

		/* FIXME: should be liberal in what we accept */
		rc = ldap_bv2dn_x( val, &dn, LDAP_DN_FORMAT_LDAP, ctx );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		assert( strlen( val->bv_val ) == val->bv_len );

		/*
		 * Schema-aware rewrite
		 */
		if ( LDAPDN_rewrite( dn, SLAP_LDAPDN_PRETTY, ctx ) != LDAP_SUCCESS ) {
			ldap_dnfree_x( dn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		rc = ldap_dn2bv_x( dn, pretty,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		if ( rc != LDAP_SUCCESS ) {
			ldap_dnfree_x( dn, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		if ( LDAPDN_rewrite( dn, 0, ctx ) != LDAP_SUCCESS ) {
			ldap_dnfree_x( dn, ctx );
			ber_memfree_x( pretty->bv_val, ctx );
			pretty->bv_val = NULL;
			pretty->bv_len = 0;
			return LDAP_INVALID_SYNTAX;
		}

		rc = ldap_dn2bv_x( dn, normal,
			LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY, ctx );

		ldap_dnfree_x( dn, ctx );
		if ( rc != LDAP_SUCCESS ) {
			ber_memfree_x( pretty->bv_val, ctx );
			pretty->bv_val = NULL;
			pretty->bv_len = 0;
			return LDAP_INVALID_SYNTAX;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<<< dnPrettyNormal: <%s>, <%s>\n",
		pretty->bv_val ? pretty->bv_val : "",
		normal->bv_val ? normal->bv_val : "", 0 );

	return LDAP_SUCCESS;
}

/*
 * dnMatch routine
 */
int
dnMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match;
	struct berval *asserted = (struct berval *) assertedValue;

	assert( matchp != NULL );
	assert( value != NULL );
	assert( assertedValue != NULL );
	assert( !BER_BVISNULL( value ) );
	assert( !BER_BVISNULL( asserted ) );
	
	match = value->bv_len - asserted->bv_len;

	if ( match == 0 ) {
		match = memcmp( value->bv_val, asserted->bv_val, 
				value->bv_len );
	}

	Debug( LDAP_DEBUG_ARGS, "dnMatch %d\n\t\"%s\"\n\t\"%s\"\n",
		match, value->bv_val, asserted->bv_val );

	*matchp = match;
	return LDAP_SUCCESS;
}

/*
 * dnRelativeMatch routine
 */
int
dnRelativeMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match;
	struct berval *asserted = (struct berval *) assertedValue;

	assert( matchp != NULL );
	assert( value != NULL );
	assert( assertedValue != NULL );
	assert( !BER_BVISNULL( value ) );
	assert( !BER_BVISNULL( asserted ) );

	if( mr == slap_schema.si_mr_dnSubtreeMatch ) {
		if( asserted->bv_len > value->bv_len ) {
			match = -1;
		} else if ( asserted->bv_len == value->bv_len ) {
			match = memcmp( value->bv_val, asserted->bv_val, 
				value->bv_len );
		} else {
			if( DN_SEPARATOR(
				value->bv_val[value->bv_len - asserted->bv_len - 1] ))
			{
				match = memcmp(
					&value->bv_val[value->bv_len - asserted->bv_len],
					asserted->bv_val, 
					asserted->bv_len );
			} else {
				match = 1;
			}
		}

		*matchp = match;
		return LDAP_SUCCESS;
	}

	if( mr == slap_schema.si_mr_dnSuperiorMatch ) {
		asserted = value;
		value = (struct berval *) assertedValue;
		mr = slap_schema.si_mr_dnSubordinateMatch;
	}

	if( mr == slap_schema.si_mr_dnSubordinateMatch ) {
		if( asserted->bv_len >= value->bv_len ) {
			match = -1;
		} else {
			if( DN_SEPARATOR(
				value->bv_val[value->bv_len - asserted->bv_len - 1] ))
			{
				match = memcmp(
					&value->bv_val[value->bv_len - asserted->bv_len],
					asserted->bv_val, 
					asserted->bv_len );
			} else {
				match = 1;
			}
		}

		*matchp = match;
		return LDAP_SUCCESS;
	}

	if( mr == slap_schema.si_mr_dnOneLevelMatch ) {
		if( asserted->bv_len >= value->bv_len ) {
			match = -1;
		} else {
			if( DN_SEPARATOR(
				value->bv_val[value->bv_len - asserted->bv_len - 1] ))
			{
				match = memcmp(
					&value->bv_val[value->bv_len - asserted->bv_len],
					asserted->bv_val, 
					asserted->bv_len );

				if( !match ) {
					struct berval rdn;
					rdn.bv_val = value->bv_val;
					rdn.bv_len = value->bv_len - asserted->bv_len - 1;
					match = dnIsOneLevelRDN( &rdn ) ? 0 : 1;
				}
			} else {
				match = 1;
			}
		}

		*matchp = match;
		return LDAP_SUCCESS;
	}

	/* should not be reachable */
	assert( 0 );
	return LDAP_OTHER;
}

int
rdnMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match;
	struct berval *asserted = (struct berval *) assertedValue;

	assert( matchp != NULL );
	assert( value != NULL );
	assert( assertedValue != NULL );
	
	match = value->bv_len - asserted->bv_len;

	if ( match == 0 ) {
		match = memcmp( value->bv_val, asserted->bv_val, 
				value->bv_len );
	}

	Debug( LDAP_DEBUG_ARGS, "rdnMatch %d\n\t\"%s\"\n\t\"%s\"\n",
		match, value->bv_val, asserted->bv_val );

	*matchp = match;
	return LDAP_SUCCESS;
}


/*
 * dnParent - dn's parent, in-place
 * note: the incoming dn is assumed to be normalized/prettyfied,
 * so that escaped rdn/ava separators are in '\'+hexpair form
 *
 * note: "dn" and "pdn" can point to the same berval;
 * beware that, in this case, the pointer to the original buffer
 * will get lost.
 */
void
dnParent( 
	struct berval	*dn, 
	struct berval	*pdn )
{
	char	*p;

	p = ber_bvchr( dn, ',' );

	/* one-level dn */
	if ( p == NULL ) {
		pdn->bv_val = dn->bv_val + dn->bv_len;
		pdn->bv_len = 0;
		return;
	}

	assert( DN_SEPARATOR( p[ 0 ] ) );
	p++;

	assert( ATTR_LEADCHAR( p[ 0 ] ) );
	pdn->bv_len = dn->bv_len - (p - dn->bv_val);
	pdn->bv_val = p;

	return;
}

/*
 * dnRdn - dn's rdn, in-place
 * note: the incoming dn is assumed to be normalized/prettyfied,
 * so that escaped rdn/ava separators are in '\'+hexpair form
 */
void
dnRdn( 
	struct berval	*dn, 
	struct berval	*rdn )
{
	char	*p;

	*rdn = *dn;
	p = ber_bvchr( dn, ',' );

	/* one-level dn */
	if ( p == NULL ) {
		return;
	}

	assert( DN_SEPARATOR( p[ 0 ] ) );
	assert( ATTR_LEADCHAR( p[ 1 ] ) );
	rdn->bv_len = p - dn->bv_val;

	return;
}

int
dnExtractRdn( 
	struct berval	*dn, 
	struct berval 	*rdn,
	void *ctx )
{
	LDAPRDN		tmpRDN;
	const char	*p;
	int		rc;

	assert( dn != NULL );
	assert( rdn != NULL );

	if( dn->bv_len == 0 ) {
		return LDAP_OTHER;
	}

	rc = ldap_bv2rdn_x( dn, &tmpRDN, (char **)&p, LDAP_DN_FORMAT_LDAP, ctx );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = ldap_rdn2bv_x( tmpRDN, rdn, LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY,
		ctx );

	ldap_rdnfree_x( tmpRDN, ctx );
	return rc;
}

/*
 * We can assume the input is a prettied or normalized DN
 */
ber_len_t
dn_rdnlen(
	Backend		*be,
	struct berval	*dn_in )
{
	const char	*p;

	assert( dn_in != NULL );

	if ( dn_in == NULL ) {
		return 0;
	}

	if ( !dn_in->bv_len ) {
		return 0;
	}

	if ( be != NULL && be_issuffix( be, dn_in ) ) {
		return 0;
	}

	p = ber_bvchr( dn_in, ',' );

	return p ? (ber_len_t) (p - dn_in->bv_val) : dn_in->bv_len;
}


/* rdnValidate:
 *
 * LDAP_SUCCESS if rdn is a legal rdn;
 * LDAP_INVALID_SYNTAX otherwise (including a sequence of rdns)
 */
int
rdn_validate( struct berval *rdn )
{
#if 1
	/* Major cheat!
	 * input is a pretty or normalized DN
	 * hence, we can just search for ','
	 */
	if( rdn == NULL || rdn->bv_len == 0 ||
		rdn->bv_len > SLAP_LDAPDN_MAXLEN )
	{
		return LDAP_INVALID_SYNTAX;
	}
	return ber_bvchr( rdn, ',' ) == NULL
		? LDAP_SUCCESS : LDAP_INVALID_SYNTAX;

#else
	LDAPRDN		*RDN, **DN[ 2 ] = { &RDN, NULL };
	const char	*p;
	int		rc;

	/*
	 * must be non-empty
	 */
	if ( rdn == NULL || rdn == '\0' ) {
		return 0;
	}

	/*
	 * must be parsable
	 */
	rc = ldap_bv2rdn( rdn, &RDN, (char **)&p, LDAP_DN_FORMAT_LDAP );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	/*
	 * Must be one-level
	 */
	if ( p[ 0 ] != '\0' ) {
		return 0;
	}

	/*
	 * Schema-aware validate
	 */
	if ( rc == LDAP_SUCCESS ) {
		rc = LDAPDN_validate( DN );
	}
	ldap_rdnfree( RDN );

	/*
	 * Must validate (there's a repeated parsing ...)
	 */
	return ( rc == LDAP_SUCCESS );
#endif
}


/* build_new_dn:
 *
 * Used by back-bdb back_modrdn to create the new dn of entries being
 * renamed.
 *
 * new_dn = parent (p_dn) + separator + rdn (newrdn) + null.
 */

void
build_new_dn( struct berval * new_dn,
	struct berval * parent_dn,
	struct berval * newrdn,
	void *memctx )
{
	char *ptr;

	if ( parent_dn == NULL || parent_dn->bv_len == 0 ) {
		ber_dupbv_x( new_dn, newrdn, memctx );
		return;
	}

	new_dn->bv_len = parent_dn->bv_len + newrdn->bv_len + 1;
	new_dn->bv_val = (char *) slap_sl_malloc( new_dn->bv_len + 1, memctx );

	ptr = lutil_strncopy( new_dn->bv_val, newrdn->bv_val, newrdn->bv_len );
	*ptr++ = ',';
	strcpy( ptr, parent_dn->bv_val );
}


/*
 * dnIsSuffix - tells whether suffix is a suffix of dn.
 * Both dn and suffix must be normalized.
 */
int
dnIsSuffix(
	const struct berval *dn,
	const struct berval *suffix )
{
	int	d;

	assert( dn != NULL );
	assert( suffix != NULL );

	d = dn->bv_len - suffix->bv_len;

	/* empty suffix matches any dn */
	if ( suffix->bv_len == 0 ) {
		return 1;
	}

	/* suffix longer than dn */
	if ( d < 0 ) {
		return 0;
	}

	/* no rdn separator or escaped rdn separator */
	if ( d > 1 && !DN_SEPARATOR( dn->bv_val[ d - 1 ] ) ) {
		return 0;
	}

	/* no possible match or malformed dn */
	if ( d == 1 ) {
		return 0;
	}

	/* compare */
	return( strncmp( dn->bv_val + d, suffix->bv_val, suffix->bv_len ) == 0 );
}

/*
 * In place; assumes:
 * - ndn is normalized
 * - nbase is normalized
 * - dnIsSuffix( ndn, nbase ) == TRUE
 * - LDAP_SCOPE_DEFAULT == LDAP_SCOPE_SUBTREE
 */
int
dnIsWithinScope( struct berval *ndn, struct berval *nbase, int scope )
{
	assert( ndn != NULL );
	assert( nbase != NULL );
	assert( !BER_BVISNULL( ndn ) );
	assert( !BER_BVISNULL( nbase ) );

	switch ( scope ) {
	case LDAP_SCOPE_DEFAULT:
	case LDAP_SCOPE_SUBTREE:
		break;

	case LDAP_SCOPE_BASE:
		if ( ndn->bv_len != nbase->bv_len ) {
			return 0;
		}
		break;

	case LDAP_SCOPE_ONELEVEL: {
		struct berval pndn;
		dnParent( ndn, &pndn );
		if ( pndn.bv_len != nbase->bv_len ) {
			return 0;
		}
		} break;

	case LDAP_SCOPE_SUBORDINATE:
		if ( ndn->bv_len == nbase->bv_len ) {
			return 0;
		}
		break;

	/* unknown scope */
	default:
		return -1;
	}

	return 1;
}

/*
 * In place; assumes:
 * - ndn is normalized
 * - nbase is normalized
 * - LDAP_SCOPE_DEFAULT == LDAP_SCOPE_SUBTREE
 */
int
dnIsSuffixScope( struct berval *ndn, struct berval *nbase, int scope )
{
	if ( !dnIsSuffix( ndn, nbase ) ) {
		return 0;
	}

	return dnIsWithinScope( ndn, nbase, scope );
}

int
dnIsOneLevelRDN( struct berval *rdn )
{
	ber_len_t	len = rdn->bv_len;
	for ( ; len--; ) {
		if ( DN_SEPARATOR( rdn->bv_val[ len ] ) ) {
			return 0;
		}
	}

	return 1;
}

#ifdef HAVE_TLS
static SLAP_CERT_MAP_FN *DNX509PeerNormalizeCertMap = NULL;
#endif

int register_certificate_map_function(SLAP_CERT_MAP_FN *fn)
{
#ifdef HAVE_TLS
	if ( DNX509PeerNormalizeCertMap == NULL ) {
		DNX509PeerNormalizeCertMap = fn;
		return 0;
	}
#endif

	return -1;
}

/*
 * Convert an X.509 DN into a normalized LDAP DN
 */
int
dnX509normalize( void *x509_name, struct berval *out )
{
	/* Invoke the LDAP library's converter with our schema-rewriter */
	int rc = ldap_X509dn2bv( x509_name, out, LDAPDN_rewrite, 0 );

	Debug( LDAP_DEBUG_TRACE,
		"dnX509Normalize: <%s> (%d)\n",
		BER_BVISNULL( out ) ? "(null)" : out->bv_val, rc, 0 );

	return rc;
}

#ifdef HAVE_TLS
/*
 * Get the TLS session's peer's DN into a normalized LDAP DN
 */
int
dnX509peerNormalize( void *ssl, struct berval *dn )
{
	int rc = LDAP_INVALID_CREDENTIALS;

	if ( DNX509PeerNormalizeCertMap != NULL )
		rc = (*DNX509PeerNormalizeCertMap)( ssl, dn );

	if ( rc != LDAP_SUCCESS ) {
		rc = ldap_pvt_tls_get_peer_dn( ssl, dn,
			(LDAPDN_rewrite_dummy *)LDAPDN_rewrite, 0 );
	}

	return rc;
}
#endif
