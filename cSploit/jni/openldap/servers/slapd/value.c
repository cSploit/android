/* value.c - routines for dealing with values */
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
/*
 * Copyright (c) 1995 Regents of the University of Michigan.
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

#include <sys/stat.h>

#include "slap.h"

int
value_add( 
    BerVarray	*vals,
    BerVarray	addvals )
{
	int		n, nn = 0;
	BerVarray	v2;

	if ( addvals != NULL ) {
		for ( ; !BER_BVISNULL( &addvals[nn] ); nn++ )
			;	/* NULL */
	}

	if ( *vals == NULL ) {
		*vals = (BerVarray) SLAP_MALLOC( (nn + 1)
		    * sizeof(struct berval) );
		if( *vals == NULL ) {
			Debug(LDAP_DEBUG_TRACE,
		      "value_add: SLAP_MALLOC failed.\n", 0, 0, 0 );
			return LBER_ERROR_MEMORY;
		}
		n = 0;

	} else {
		for ( n = 0; !BER_BVISNULL( &(*vals)[n] ); n++ ) {
			;	/* Empty */
		}
		*vals = (BerVarray) SLAP_REALLOC( (char *) *vals,
		    (n + nn + 1) * sizeof(struct berval) );
		if( *vals == NULL ) {
			Debug(LDAP_DEBUG_TRACE,
		      "value_add: SLAP_MALLOC failed.\n", 0, 0, 0 );
			return LBER_ERROR_MEMORY;
		}
	}

	v2 = &(*vals)[n];
	for ( n = 0 ; n < nn; v2++, addvals++ ) {
		ber_dupbv( v2, addvals );
		if ( BER_BVISNULL( v2 ) ) break;
	}
	BER_BVZERO( v2 );

	return LDAP_SUCCESS;
}

int
value_add_one( 
    BerVarray		*vals,
    struct berval	*addval )
{
	int		n;
	BerVarray	v2;

	if ( *vals == NULL ) {
		*vals = (BerVarray) SLAP_MALLOC( 2 * sizeof(struct berval) );
		if( *vals == NULL ) {
			Debug(LDAP_DEBUG_TRACE,
		      "value_add_one: SLAP_MALLOC failed.\n", 0, 0, 0 );
			return LBER_ERROR_MEMORY;
		}
		n = 0;

	} else {
		for ( n = 0; !BER_BVISNULL( &(*vals)[n] ); n++ ) {
			;	/* Empty */
		}
		*vals = (BerVarray) SLAP_REALLOC( (char *) *vals,
		    (n + 2) * sizeof(struct berval) );
		if( *vals == NULL ) {
			Debug(LDAP_DEBUG_TRACE,
		      "value_add_one: SLAP_MALLOC failed.\n", 0, 0, 0 );
			return LBER_ERROR_MEMORY;
		}
	}

	v2 = &(*vals)[n];
	ber_dupbv(v2, addval);

	v2++;
	BER_BVZERO( v2 );

	return LDAP_SUCCESS;
}

int asserted_value_validate_normalize( 
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned usage,
	struct berval *in,
	struct berval *out,
	const char ** text,
	void *ctx )
{
	int rc;
	struct berval pval;
	pval.bv_val = NULL;

	/* we expect the value to be in the assertion syntax */
	assert( !SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX(usage) );

	if( mr == NULL ) {
		*text = "inappropriate matching request";
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	if( !mr->smr_match ) {
		*text = "requested matching rule not supported";
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	if( mr->smr_syntax->ssyn_pretty ) {
		rc = (mr->smr_syntax->ssyn_pretty)( mr->smr_syntax, in, &pval, ctx );
		in = &pval;

	} else if ( mr->smr_syntax->ssyn_validate ) {
		rc = (mr->smr_syntax->ssyn_validate)( mr->smr_syntax, in );

	} else {
		*text = "inappropriate matching request";
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	if( rc != LDAP_SUCCESS ) {
		*text = "value does not conform to assertion syntax";
		return LDAP_INVALID_SYNTAX;
	}

	if( mr->smr_normalize ) {
		rc = (mr->smr_normalize)(
			usage|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
			ad ? ad->ad_type->sat_syntax : NULL,
			mr, in, out, ctx );

		if( pval.bv_val ) ber_memfree_x( pval.bv_val, ctx );

		if( rc != LDAP_SUCCESS ) {
			*text = "unable to normalize value for matching";
			return LDAP_INVALID_SYNTAX;
		}

	} else if ( pval.bv_val != NULL ) {
		*out = pval;

	} else {
		ber_dupbv_x( out, in, ctx );
	}

	return LDAP_SUCCESS;
}

int
value_match(
	int *match,
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned flags,
	struct berval *v1, /* stored value */
	void *v2, /* assertion */
	const char ** text )
{
	int rc;

	assert( mr != NULL );

	if( !mr->smr_match ) {
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	rc = (mr->smr_match)( match, flags,
		ad->ad_type->sat_syntax, mr, v1, v2 );
	
	return rc;
}

int value_find_ex(
	AttributeDescription *ad,
	unsigned flags,
	BerVarray vals,
	struct berval *val,
	void *ctx )
{
	int	i;
	int rc;
	struct berval nval = BER_BVNULL;
	MatchingRule *mr = ad->ad_type->sat_equality;

	if( mr == NULL || !mr->smr_match ) {
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	assert( SLAP_IS_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH( flags ) != 0 );

	if( !SLAP_IS_MR_ASSERTED_VALUE_NORMALIZED_MATCH( flags ) &&
		mr->smr_normalize )
	{
		rc = (mr->smr_normalize)(
			flags & (SLAP_MR_TYPE_MASK|SLAP_MR_SUBTYPE_MASK|SLAP_MR_VALUE_OF_SYNTAX),
			ad->ad_type->sat_syntax,
			mr, val, &nval, ctx );

		if( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	for ( i = 0; vals[i].bv_val != NULL; i++ ) {
		int match;
		const char *text;

		rc = value_match( &match, ad, mr, flags,
			&vals[i], nval.bv_val == NULL ? val : &nval, &text );

		if( rc == LDAP_SUCCESS && match == 0 ) {
			slap_sl_free( nval.bv_val, ctx );
			return rc;
		}
	}

	slap_sl_free( nval.bv_val, ctx );
	return LDAP_NO_SUCH_ATTRIBUTE;
}

/* assign new indexes to an attribute's ordered values */
void
ordered_value_renumber( Attribute *a )
{
	char *ptr, ibuf[64];	/* many digits */
	struct berval ibv, tmp, vtmp;
	unsigned i;

	ibv.bv_val = ibuf;

	for (i=0; i<a->a_numvals; i++) {
		ibv.bv_len = sprintf(ibv.bv_val, "{%u}", i);
		vtmp = a->a_vals[i];
		if ( vtmp.bv_val[0] == '{' ) {
			ptr = ber_bvchr(&vtmp, '}');
			assert( ptr != NULL );
			++ptr;
			vtmp.bv_len -= ptr - vtmp.bv_val;
			vtmp.bv_val = ptr;
		}
		tmp.bv_len = ibv.bv_len + vtmp.bv_len;
		tmp.bv_val = ch_malloc( tmp.bv_len + 1 );
		strcpy( tmp.bv_val, ibv.bv_val );
		AC_MEMCPY( tmp.bv_val + ibv.bv_len, vtmp.bv_val, vtmp.bv_len );
		tmp.bv_val[tmp.bv_len] = '\0';
		ch_free( a->a_vals[i].bv_val );
		a->a_vals[i] = tmp;

		if ( a->a_nvals && a->a_nvals != a->a_vals ) {
			vtmp = a->a_nvals[i];
			if ( vtmp.bv_val[0] == '{' ) {
				ptr = ber_bvchr(&vtmp, '}');
				assert( ptr != NULL );
				++ptr;
				vtmp.bv_len -= ptr - vtmp.bv_val;
				vtmp.bv_val = ptr;
			}
			tmp.bv_len = ibv.bv_len + vtmp.bv_len;
			tmp.bv_val = ch_malloc( tmp.bv_len + 1 );
			strcpy( tmp.bv_val, ibv.bv_val );
			AC_MEMCPY( tmp.bv_val + ibv.bv_len, vtmp.bv_val, vtmp.bv_len );
			tmp.bv_val[tmp.bv_len] = '\0';
			ch_free( a->a_nvals[i].bv_val );
			a->a_nvals[i] = tmp;
		}
	}
}

/* Sort the values in an X-ORDERED VALUES attribute.
 * If the values have no index, index them in their given order.
 * If the values have indexes, sort them.
 * If some are indexed and some are not, return Error.
 */
int
ordered_value_sort( Attribute *a, int do_renumber )
{
	int i, vals;
	int index = 0, noindex = 0, renumber = 0, gotnvals = 0;
	struct berval tmp;

	if ( a->a_nvals && a->a_nvals != a->a_vals )
		gotnvals = 1;

	/* count attrs, look for index */
	for (i=0; a->a_vals[i].bv_val; i++) {
		if ( a->a_vals[i].bv_val[0] == '{' ) {
			char *ptr;
			index = 1;
			ptr = ber_bvchr( &a->a_vals[i], '}' );
			if ( !ptr )
				return LDAP_INVALID_SYNTAX;
			if ( noindex )
				return LDAP_INVALID_SYNTAX;
		} else {
			noindex = 1;
			if ( index )
				return LDAP_INVALID_SYNTAX;
		}
	}
	vals = i;

	/* If values have indexes, sort the values */
	if ( index ) {
		int *indexes, j, idx;
		struct berval ntmp;

#if 0
		/* Strip index from normalized values */
		if ( !a->a_nvals || a->a_vals == a->a_nvals ) {
			a->a_nvals = ch_malloc( (vals+1)*sizeof(struct berval));
			BER_BVZERO(a->a_nvals+vals);
			for ( i=0; i<vals; i++ ) {
				char *ptr = ber_bvchr(&a->a_vals[i], '}') + 1;
				a->a_nvals[i].bv_len = a->a_vals[i].bv_len -
					(ptr - a->a_vals[i].bv_val);
				a->a_nvals[i].bv_val = ch_malloc( a->a_nvals[i].bv_len + 1);
				strcpy(a->a_nvals[i].bv_val, ptr );
			}
		} else {
			for ( i=0; i<vals; i++ ) {
				char *ptr = ber_bvchr(&a->a_nvals[i], '}') + 1;
				a->a_nvals[i].bv_len -= ptr - a->a_nvals[i].bv_val;
				strcpy(a->a_nvals[i].bv_val, ptr);
			}
		}
#endif
				
		indexes = ch_malloc( vals * sizeof(int) );
		for ( i=0; i<vals; i++) {
			char *ptr;
			indexes[i] = strtol(a->a_vals[i].bv_val+1, &ptr, 0);
			if ( *ptr != '}' ) {
				ch_free( indexes );
				return LDAP_INVALID_SYNTAX;
			}
		}

		/* Insertion sort */
		for ( i=1; i<vals; i++ ) {
			idx = indexes[i];
			tmp = a->a_vals[i];
			if ( gotnvals ) ntmp = a->a_nvals[i];
			j = i;
			while ((j > 0) && (indexes[j-1] > idx)) {
				indexes[j] = indexes[j-1];
				a->a_vals[j] = a->a_vals[j-1];
				if ( gotnvals ) a->a_nvals[j] = a->a_nvals[j-1];
				j--;
			}
			indexes[j] = idx;
			a->a_vals[j] = tmp;
			if ( gotnvals ) a->a_nvals[j] = ntmp;
		}

		/* If range is not contiguous, must renumber */
		if ( indexes[0] != 0 || indexes[vals-1] != vals-1 ) {
			renumber = 1;
		}
		ch_free( indexes );
	} else {
		renumber = 1;
	}

	if ( do_renumber && renumber )
		ordered_value_renumber( a );

	return 0;
}

/*
 * wrapper for validate function
 * uses the validate function of the syntax after removing
 * the index, if allowed and present
 */
int
ordered_value_validate(
	AttributeDescription *ad,
	struct berval *in,
	int mop )
{
	struct berval	bv = *in;

	assert( ad->ad_type->sat_syntax != NULL );
	assert( ad->ad_type->sat_syntax->ssyn_validate != NULL );

	if ( ad->ad_type->sat_flags & SLAP_AT_ORDERED ) {

		/* Skip past the assertion index */
		if ( bv.bv_val[0] == '{' ) {
			char		*ptr;

			ptr = ber_bvchr( &bv, '}' );
			if ( ptr != NULL ) {
				struct berval	ns;

				ns.bv_val = bv.bv_val + 1;
				ns.bv_len = ptr - ns.bv_val;

				if ( numericStringValidate( NULL, &ns ) == LDAP_SUCCESS ) {
					ptr++;
					bv.bv_len -= ptr - bv.bv_val;
					bv.bv_val = ptr;
					in = &bv;
					/* If deleting by index, just succeed */
					if ( mop == LDAP_MOD_DELETE && BER_BVISEMPTY( &bv ) ) {
						return LDAP_SUCCESS;
					}
				}
			}
		}
	}

	return ad->ad_type->sat_syntax->ssyn_validate( ad->ad_type->sat_syntax, in );
}

/*
 * wrapper for pretty function
 * uses the pretty function of the syntax after removing
 * the index, if allowed and present; in case, it's prepended
 * to the pretty value
 */
int
ordered_value_pretty(
	AttributeDescription *ad,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	struct berval	bv,
			idx = BER_BVNULL;
	int		rc;

	assert( ad->ad_type->sat_syntax != NULL );
	assert( ad->ad_type->sat_syntax->ssyn_pretty != NULL );
	assert( val != NULL );
	assert( out != NULL );

	bv = *val;

	if ( ad->ad_type->sat_flags & SLAP_AT_ORDERED ) {

		/* Skip past the assertion index */
		if ( bv.bv_val[0] == '{' ) {
			char	*ptr;

			ptr = ber_bvchr( &bv, '}' );
			if ( ptr != NULL ) {
				struct berval	ns;

				ns.bv_val = bv.bv_val + 1;
				ns.bv_len = ptr - ns.bv_val;

				if ( numericStringValidate( NULL, &ns ) == LDAP_SUCCESS ) {
					ptr++;

					idx = bv;
					idx.bv_len = ptr - bv.bv_val;

					bv.bv_len -= idx.bv_len;
					bv.bv_val = ptr;

					val = &bv;
				}
			}
		}
	}

	rc = ad->ad_type->sat_syntax->ssyn_pretty( ad->ad_type->sat_syntax, val, out, ctx );

	if ( rc == LDAP_SUCCESS && !BER_BVISNULL( &idx ) ) {
		bv = *out;

		out->bv_len = idx.bv_len + bv.bv_len;
		out->bv_val = ber_memalloc_x( out->bv_len + 1, ctx );
		
		AC_MEMCPY( out->bv_val, idx.bv_val, idx.bv_len );
		AC_MEMCPY( &out->bv_val[ idx.bv_len ], bv.bv_val, bv.bv_len + 1 );

		ber_memfree_x( bv.bv_val, ctx );
	}

	return rc;
}

/*
 * wrapper for normalize function
 * uses the normalize function of the attribute description equality rule
 * after removing the index, if allowed and present; in case, it's
 * prepended to the value
 */
int
ordered_value_normalize(
	slap_mask_t usage,
	AttributeDescription *ad,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval	bv,
			idx = BER_BVNULL;
	int		rc;

	assert( ad->ad_type->sat_equality != NULL );
	assert( ad->ad_type->sat_equality->smr_normalize != NULL );
	assert( val != NULL );
	assert( normalized != NULL );

	bv = *val;

	if ( ad->ad_type->sat_flags & SLAP_AT_ORDERED ) {

		/* Skip past the assertion index */
		if ( bv.bv_val[ 0 ] == '{' ) {
			char	*ptr;

			ptr = ber_bvchr( &bv, '}' );
			if ( ptr != NULL ) {
				struct berval	ns;

				ns.bv_val = bv.bv_val + 1;
				ns.bv_len = ptr - ns.bv_val;

				if ( numericStringValidate( NULL, &ns ) == LDAP_SUCCESS ) {
					ptr++;

					idx = bv;
					idx.bv_len = ptr - bv.bv_val;

					bv.bv_len -= idx.bv_len;
					bv.bv_val = ptr;

					/* validator will already prevent this for Adds */
					if ( BER_BVISEMPTY( &bv )) {
						ber_dupbv_x( normalized, &idx, ctx );
						return LDAP_SUCCESS;
					}
					val = &bv;
				}
			}
		}
	}

	rc = ad->ad_type->sat_equality->smr_normalize( usage,
		ad->ad_type->sat_syntax, mr, val, normalized, ctx );

	if ( rc == LDAP_SUCCESS && !BER_BVISNULL( &idx ) ) {
		bv = *normalized;

		normalized->bv_len = idx.bv_len + bv.bv_len;
		normalized->bv_val = ber_memalloc_x( normalized->bv_len + 1, ctx );
		
		AC_MEMCPY( normalized->bv_val, idx.bv_val, idx.bv_len );
		AC_MEMCPY( &normalized->bv_val[ idx.bv_len ], bv.bv_val, bv.bv_len + 1 );

		ber_memfree_x( bv.bv_val, ctx );
	}

	return rc;
}

/* A wrapper for value match, handles Equality matches for attributes
 * with ordered values.
 */
int
ordered_value_match(
	int *match,
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned flags,
	struct berval *v1, /* stored value */
	struct berval *v2, /* assertion */
	const char ** text )
{
	struct berval bv1, bv2;

	/* X-ORDERED VALUES equality matching:
	 * If (SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX) that means we are
	 * comparing two attribute values. In this case, we want to ignore
	 * the ordering index of both values, we just want to know if their
	 * main values are equal.
	 *
	 * If (SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX) then we are comparing
	 * an assertion against an attribute value.
	 *    If the assertion has no index, the index of the value is ignored. 
	 *    If the assertion has only an index, the remainder of the value is
	 *      ignored.
	 *    If the assertion has index and value, both are compared.
	 */
	if ( ad->ad_type->sat_flags & SLAP_AT_ORDERED ) {
		char *ptr;
		struct berval ns1 = BER_BVNULL, ns2 = BER_BVNULL;

		bv1 = *v1;
		bv2 = *v2;

		/* Skip past the assertion index */
		if ( bv2.bv_val[0] == '{' ) {
			ptr = ber_bvchr( &bv2, '}' );
			if ( ptr != NULL ) {
				ns2.bv_val = bv2.bv_val + 1;
				ns2.bv_len = ptr - ns2.bv_val;

				if ( numericStringValidate( NULL, &ns2 ) == LDAP_SUCCESS ) {
					ptr++;
					bv2.bv_len -= ptr - bv2.bv_val;
					bv2.bv_val = ptr;
					v2 = &bv2;
				}
			}
		}

		/* Skip past the attribute index */
		if ( bv1.bv_val[0] == '{' ) {
			ptr = ber_bvchr( &bv1, '}' );
			if ( ptr != NULL ) {
				ns1.bv_val = bv1.bv_val + 1;
				ns1.bv_len = ptr - ns1.bv_val;

				if ( numericStringValidate( NULL, &ns1 ) == LDAP_SUCCESS ) {
					ptr++;
					bv1.bv_len -= ptr - bv1.bv_val;
					bv1.bv_val = ptr;
					v1 = &bv1;
				}
			}
		}

		if ( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX( flags )) {
			if ( !BER_BVISNULL( &ns2 ) && !BER_BVISNULL( &ns1 ) ) {
				/* compare index values first */
				(void)octetStringOrderingMatch( match, 0, NULL, NULL, &ns1, &ns2 );

				/* If not equal, or we're only comparing the index,
				 * return result now.
				 */
				if ( *match != 0 || BER_BVISEMPTY( &bv2 ) ) {
					return LDAP_SUCCESS;
				}
			}
		}

	}

	if ( !mr || !mr->smr_match ) {
		*match = ber_bvcmp( v1, v2 );
		return LDAP_SUCCESS;
	}

	return value_match( match, ad, mr, flags, v1, v2, text );
}

int
ordered_value_add(
	Entry *e,
	AttributeDescription *ad,
	Attribute *a,
	BerVarray vals,
	BerVarray nvals
)
{
	int i, j, k, anum, vnum;
	BerVarray new, nnew = NULL;

	/* count new vals */
	for (i=0; !BER_BVISNULL( vals+i ); i++) ;
	vnum = i;

	if ( a ) {
		ordered_value_sort( a, 0 );
	} else {
		Attribute **ap;
		for ( ap=&e->e_attrs; *ap; ap = &(*ap)->a_next ) ;
		a = attr_alloc( ad );
		*ap = a;
	}
	anum = a->a_numvals;

	new = ch_malloc( (anum+vnum+1) * sizeof(struct berval));

	/* sanity check: if normalized modifications come in, either
	 * no values are present or normalized existing values differ
	 * from non-normalized; if no normalized modifications come in,
	 * either no values are present or normalized existing values
	 * don't differ from non-normalized */
	if ( nvals != NULL ) {
		assert( nvals != vals );
		assert( a->a_nvals == NULL || a->a_nvals != a->a_vals );

	} else {
		assert( a->a_nvals == NULL || a->a_nvals == a->a_vals );
	}

	if ( ( a->a_nvals && a->a_nvals != a->a_vals ) || nvals != NULL ) {
		nnew = ch_malloc( (anum+vnum+1) * sizeof(struct berval));
		/* Shouldn't happen... */
		if ( !nvals ) nvals = vals;
	}
	if ( anum ) {
		AC_MEMCPY( new, a->a_vals, anum * sizeof(struct berval));
		if ( nnew && a->a_nvals )
			AC_MEMCPY( nnew, a->a_nvals, anum * sizeof(struct berval));
	}

	for (i=0; i<vnum; i++) {
		char	*next;

		k = -1;
		if ( vals[i].bv_val[0] == '{' ) {
			/* FIXME: strtol() could go past end... */
			k = strtol( vals[i].bv_val + 1, &next, 0 );
			if ( next == vals[i].bv_val + 1 ||
				next[ 0 ] != '}' ||
				(ber_len_t) (next - vals[i].bv_val) > vals[i].bv_len )
			{
				ch_free( nnew );
				ch_free( new );
				return -1;
			}
			if ( k > anum ) k = -1;
		}
		/* No index, or index is greater than current number of
		 * values, just tack onto the end
		 */
		if ( k < 0 ) {
			ber_dupbv( new+anum, vals+i );
			if ( nnew ) ber_dupbv( nnew+anum, nvals+i );

		/* Indexed, push everything else down one and insert */
		} else {
			for (j=anum; j>k; j--) {
				new[j] = new[j-1];
				if ( nnew ) nnew[j] = nnew[j-1];
			}
			ber_dupbv( new+k, vals+i );
			if ( nnew ) ber_dupbv( nnew+k, nvals+i );
		}
		anum++;
	}
	BER_BVZERO( new+anum );
	ch_free( a->a_vals );
	a->a_vals = new;
	if ( nnew ) {
		BER_BVZERO( nnew+anum );
		ch_free( a->a_nvals );
		a->a_nvals = nnew;
	} else {
		a->a_nvals = a->a_vals;
	}

	a->a_numvals = anum;
	ordered_value_renumber( a );

	return 0;
}
