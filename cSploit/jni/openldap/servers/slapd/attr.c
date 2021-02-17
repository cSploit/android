/* attr.c - routines for dealing with attributes */
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"

/*
 * Allocate in chunks, minimum of 1000 at a time.
 */
#define	CHUNK_SIZE	1000
typedef struct slap_list {
	struct slap_list *next;
} slap_list;
static slap_list *attr_chunks;
static Attribute *attr_list;
static ldap_pvt_thread_mutex_t attr_mutex;

int
attr_prealloc( int num )
{
	Attribute *a;
	slap_list *s;

	if (!num) return 0;

	s = ch_calloc( 1, sizeof(slap_list) + num * sizeof(Attribute));
	s->next = attr_chunks;
	attr_chunks = s;

	a = (Attribute *)(s+1);
	for ( ;num>1; num--) {
		a->a_next = a+1;
		a++;
	}
	a->a_next = attr_list;
	attr_list = (Attribute *)(s+1);

	return 0;
}

Attribute *
attr_alloc( AttributeDescription *ad )
{
	Attribute *a;

	ldap_pvt_thread_mutex_lock( &attr_mutex );
	if ( !attr_list )
		attr_prealloc( CHUNK_SIZE );
	a = attr_list;
	attr_list = a->a_next;
	a->a_next = NULL;
	ldap_pvt_thread_mutex_unlock( &attr_mutex );
	
	a->a_desc = ad;
	if ( ad && ( ad->ad_type->sat_flags & SLAP_AT_SORTED_VAL ))
		a->a_flags |= SLAP_ATTR_SORTED_VALS;

	return a;
}

/* Return a list of num attrs */
Attribute *
attrs_alloc( int num )
{
	Attribute *head = NULL;
	Attribute **a;

	ldap_pvt_thread_mutex_lock( &attr_mutex );
	for ( a = &attr_list; *a && num > 0; a = &(*a)->a_next ) {
		if ( !head )
			head = *a;
		num--;
	}
	attr_list = *a;
	if ( num > 0 ) {
		attr_prealloc( num > CHUNK_SIZE ? num : CHUNK_SIZE );
		*a = attr_list;
		for ( ; *a && num > 0; a = &(*a)->a_next ) {
			if ( !head )
				head = *a;
			num--;
		}
		attr_list = *a;
	}
	*a = NULL;
	ldap_pvt_thread_mutex_unlock( &attr_mutex );

	return head;
}


void
attr_clean( Attribute *a )
{
	if ( a->a_nvals && a->a_nvals != a->a_vals &&
		!( a->a_flags & SLAP_ATTR_DONT_FREE_VALS )) {
		if ( a->a_flags & SLAP_ATTR_DONT_FREE_DATA ) {
			free( a->a_nvals );
		} else {
			ber_bvarray_free( a->a_nvals );
		}
	}
	/* a_vals may be equal to slap_dummy_bv, a static empty berval;
	 * this is used as a placeholder for attributes that do not carry
	 * values, e.g. when proxying search entries with the "attrsonly"
	 * bit set. */
	if ( a->a_vals != &slap_dummy_bv &&
		!( a->a_flags & SLAP_ATTR_DONT_FREE_VALS )) {
		if ( a->a_flags & SLAP_ATTR_DONT_FREE_DATA ) {
			free( a->a_vals );
		} else {
			ber_bvarray_free( a->a_vals );
		}
	}
	a->a_desc = NULL;
	a->a_vals = NULL;
	a->a_nvals = NULL;
#ifdef LDAP_COMP_MATCH
	a->a_comp_data = NULL;
#endif
	a->a_flags = 0;
	a->a_numvals = 0;
}

void
attr_free( Attribute *a )
{
	attr_clean( a );
	ldap_pvt_thread_mutex_lock( &attr_mutex );
	a->a_next = attr_list;
	attr_list = a;
	ldap_pvt_thread_mutex_unlock( &attr_mutex );
}

#ifdef LDAP_COMP_MATCH
void
comp_tree_free( Attribute *a )
{
	Attribute *next;

	for( ; a != NULL ; a = next ) {
		next = a->a_next;
		if ( component_destructor && a->a_comp_data ) {
			if ( a->a_comp_data->cd_mem_op )
				component_destructor( a->a_comp_data->cd_mem_op );
			free ( a->a_comp_data );
		}
	}
}
#endif

void
attrs_free( Attribute *a )
{
	if ( a ) {
		Attribute *b = (Attribute *)0xBAD, *tail, *next;

		/* save tail */
		tail = a;
		do {
			next = a->a_next;
			attr_clean( a );
			a->a_next = b;
			b = a;
			a = next;
		} while ( next );

		ldap_pvt_thread_mutex_lock( &attr_mutex );
		/* replace NULL with current attr list and let attr list
		 * start from last attribute returned to list */
		tail->a_next = attr_list;
		attr_list = b;
		ldap_pvt_thread_mutex_unlock( &attr_mutex );
	}
}

static void
attr_dup2( Attribute *tmp, Attribute *a )
{
	tmp->a_flags = a->a_flags & SLAP_ATTR_PERSISTENT_FLAGS;
	if ( a->a_vals != NULL ) {
		unsigned	i, j;

		tmp->a_numvals = a->a_numvals;
		tmp->a_vals = ch_malloc( (tmp->a_numvals + 1) * sizeof(struct berval) );
		for ( i = 0; i < tmp->a_numvals; i++ ) {
			ber_dupbv( &tmp->a_vals[i], &a->a_vals[i] );
			if ( BER_BVISNULL( &tmp->a_vals[i] ) ) break;
			/* FIXME: error? */
		}
		BER_BVZERO( &tmp->a_vals[i] );

		/* a_nvals must be non null; it may be equal to a_vals */
		assert( a->a_nvals != NULL );

		if ( a->a_nvals != a->a_vals ) {

			tmp->a_nvals = ch_malloc( (tmp->a_numvals + 1) * sizeof(struct berval) );
			j = 0;
			if ( i ) {
				for ( ; !BER_BVISNULL( &a->a_nvals[j] ); j++ ) {
					assert( j < i );
					ber_dupbv( &tmp->a_nvals[j], &a->a_nvals[j] );
					if ( BER_BVISNULL( &tmp->a_nvals[j] ) ) break;
					/* FIXME: error? */
				}
				assert( j == i );
			}
			BER_BVZERO( &tmp->a_nvals[j] );

		} else {
			tmp->a_nvals = tmp->a_vals;
		}
	}
}

Attribute *
attr_dup( Attribute *a )
{
	Attribute *tmp;

	if ( a == NULL) return NULL;

	tmp = attr_alloc( a->a_desc );
	attr_dup2( tmp, a );
	return tmp;
}

Attribute *
attrs_dup( Attribute *a )
{
	int i;
	Attribute *tmp, *anew;

	if( a == NULL ) return NULL;

	/* count them */
	for( tmp=a,i=0; tmp; tmp=tmp->a_next ) {
		i++;
	}

	anew = attrs_alloc( i );

	for( tmp=anew; a; a=a->a_next ) {
		tmp->a_desc = a->a_desc;
		attr_dup2( tmp, a );
		tmp=tmp->a_next;
	}

	return anew;
}

int
attr_valfind(
	Attribute *a,
	unsigned flags,
	struct berval *val,
	unsigned *slot,
	void *ctx )
{
	struct berval nval = BER_BVNULL, *cval;
	MatchingRule *mr;
	const char *text;
	int match = -1, rc;
	unsigned i, n;

	if ( flags & SLAP_MR_ORDERING )
		mr = a->a_desc->ad_type->sat_ordering;
	else
		mr = a->a_desc->ad_type->sat_equality;

	if( !SLAP_IS_MR_ASSERTED_VALUE_NORMALIZED_MATCH( flags ) &&
		mr->smr_normalize )
	{
		rc = (mr->smr_normalize)(
			flags & (SLAP_MR_TYPE_MASK|SLAP_MR_SUBTYPE_MASK|SLAP_MR_VALUE_OF_SYNTAX),
			a->a_desc->ad_type->sat_syntax,
			mr, val, &nval, ctx );

		if( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
		cval = &nval;
	} else {
		cval = val;
	}

	n = a->a_numvals;
	if ( (a->a_flags & SLAP_ATTR_SORTED_VALS) && n ) {
		/* Binary search */
		unsigned base = 0;

		do {
			unsigned pivot = n >> 1;
			i = base + pivot;
			rc = value_match( &match, a->a_desc, mr, flags,
				&a->a_nvals[i], cval, &text );
			if ( rc == LDAP_SUCCESS && match == 0 )
				break;
			if ( match < 0 ) {
				base = i+1;
				n -= pivot+1;
			} else {
				n = pivot;
			}
		} while ( n );
		if ( match < 0 )
			i++;
	} else {
	/* Linear search */
		for ( i = 0; i < n; i++ ) {
			const char *text;

			rc = ordered_value_match( &match, a->a_desc, mr, flags,
				&a->a_nvals[i], cval, &text );
			if ( rc == LDAP_SUCCESS && match == 0 )
				break;
		}
	}
	if ( match )
		rc = LDAP_NO_SUCH_ATTRIBUTE;
	if ( slot )
		*slot = i;
	if ( nval.bv_val )
		slap_sl_free( nval.bv_val, ctx );

	return rc;
}

int
attr_valadd(
	Attribute *a,
	BerVarray vals,
	BerVarray nvals,
	int nn )
{
	int		i;
	BerVarray	v2;

	v2 = (BerVarray) SLAP_REALLOC( (char *) a->a_vals,
		    (a->a_numvals + nn + 1) * sizeof(struct berval) );
	if( v2 == NULL ) {
		Debug(LDAP_DEBUG_TRACE,
		  "attr_valadd: SLAP_REALLOC failed.\n", 0, 0, 0 );
		return LBER_ERROR_MEMORY;
	}
	a->a_vals = v2;
	if ( nvals ) {
		v2 = (BerVarray) SLAP_REALLOC( (char *) a->a_nvals,
				(a->a_numvals + nn + 1) * sizeof(struct berval) );
		if( v2 == NULL ) {
			Debug(LDAP_DEBUG_TRACE,
			  "attr_valadd: SLAP_REALLOC failed.\n", 0, 0, 0 );
			return LBER_ERROR_MEMORY;
		}
		a->a_nvals = v2;
	} else {
		a->a_nvals = a->a_vals;
	}

	/* If sorted and old vals exist, must insert */
	if (( a->a_flags & SLAP_ATTR_SORTED_VALS ) && a->a_numvals ) {
		unsigned slot;
		int j, rc;
		v2 = nvals ? nvals : vals;
		for ( i = 0; i < nn; i++ ) {
			rc = attr_valfind( a, SLAP_MR_EQUALITY | SLAP_MR_VALUE_OF_ASSERTION_SYNTAX |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH | SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH,
				&v2[i], &slot, NULL );
			if ( rc != LDAP_NO_SUCH_ATTRIBUTE ) {
				/* should never happen */
				if ( rc == LDAP_SUCCESS )
					rc = LDAP_TYPE_OR_VALUE_EXISTS;
				return rc;
			}
			for ( j = a->a_numvals; j >= (int)slot; j-- ) {
				a->a_vals[j+1] = a->a_vals[j];
				if ( nvals )
					a->a_nvals[j+1] = a->a_nvals[j];
			}
			ber_dupbv( &a->a_nvals[slot], &v2[i] );
			if ( nvals )
				ber_dupbv( &a->a_vals[slot], &vals[i] );
			a->a_numvals++;
		}
		BER_BVZERO( &a->a_vals[a->a_numvals] );
		if ( a->a_vals != a->a_nvals )
			BER_BVZERO( &a->a_nvals[a->a_numvals] );
	} else {
		v2 = &a->a_vals[a->a_numvals];
		for ( i = 0 ; i < nn; i++ ) {
			ber_dupbv( &v2[i], &vals[i] );
			if ( BER_BVISNULL( &v2[i] ) ) break;
		}
		BER_BVZERO( &v2[i] );

		if ( nvals ) {
			v2 = &a->a_nvals[a->a_numvals];
			for ( i = 0 ; i < nn; i++ ) {
				ber_dupbv( &v2[i], &nvals[i] );
				if ( BER_BVISNULL( &v2[i] ) ) break;
			}
			BER_BVZERO( &v2[i] );
		}
		a->a_numvals += i;
	}
	return 0;
}

/*
 * attr_merge - merge the given type and value with the list of
 * attributes in attrs.
 *
 * nvals must be NULL if the attribute has no normalizer.
 * In this case, a->a_nvals will be set equal to a->a_vals.
 *
 * returns	0	everything went ok
 *		-1	trouble
 */

int
attr_merge(
	Entry		*e,
	AttributeDescription *desc,
	BerVarray	vals,
	BerVarray	nvals )
{
	int i = 0;

	Attribute	**a;

	for ( a = &e->e_attrs; *a != NULL; a = &(*a)->a_next ) {
		if (  (*a)->a_desc == desc ) {
			break;
		}
	}

	if ( *a == NULL ) {
		*a = attr_alloc( desc );
	} else {
		/*
		 * FIXME: if the attribute already exists, the presence
		 * of nvals and the value of (*a)->a_nvals must be consistent
		 */
		assert( ( nvals == NULL && (*a)->a_nvals == (*a)->a_vals )
				|| ( nvals != NULL && (
					( (*a)->a_vals == NULL && (*a)->a_nvals == NULL )
					|| ( (*a)->a_nvals != (*a)->a_vals ) ) ) );
	}

	if ( vals != NULL ) {
		for ( ; !BER_BVISNULL( &vals[i] ); i++ ) ;
	}
	return attr_valadd( *a, vals, nvals, i );
}

/*
 * if a normalization function is defined for the equality matchingRule
 * of desc, the value is normalized and stored in nval; otherwise nval 
 * is NULL
 */
int
attr_normalize(
	AttributeDescription	*desc,
	BerVarray		vals,
	BerVarray		*nvalsp,
	void	 		*memctx )
{
	int		rc = LDAP_SUCCESS;
	BerVarray	nvals = NULL;

	*nvalsp = NULL;

	if ( desc->ad_type->sat_equality &&
		desc->ad_type->sat_equality->smr_normalize )
	{
		int	i;
		
		for ( i = 0; !BER_BVISNULL( &vals[i] ); i++ );

		nvals = slap_sl_calloc( sizeof(struct berval), i + 1, memctx );
		for ( i = 0; !BER_BVISNULL( &vals[i] ); i++ ) {
			rc = desc->ad_type->sat_equality->smr_normalize(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					desc->ad_type->sat_syntax,
					desc->ad_type->sat_equality,
					&vals[i], &nvals[i], memctx );

			if ( rc != LDAP_SUCCESS ) {
				BER_BVZERO( &nvals[i + 1] );
				break;
			}
		}
		BER_BVZERO( &nvals[i] );
		*nvalsp = nvals;
	}

	if ( rc != LDAP_SUCCESS && nvals != NULL ) {
		ber_bvarray_free_x( nvals, memctx );
	}

	return rc;
}

int
attr_merge_normalize(
	Entry			*e,
	AttributeDescription	*desc,
	BerVarray		vals,
	void	 		*memctx )
{
	BerVarray	nvals = NULL;
	int		rc;

	rc = attr_normalize( desc, vals, &nvals, memctx );
	if ( rc == LDAP_SUCCESS ) {
		rc = attr_merge( e, desc, vals, nvals );
		if ( nvals != NULL ) {
			ber_bvarray_free_x( nvals, memctx );
		}
	}

	return rc;
}

int
attr_merge_one(
	Entry		*e,
	AttributeDescription *desc,
	struct berval	*val,
	struct berval	*nval )
{
	Attribute	**a;

	for ( a = &e->e_attrs; *a != NULL; a = &(*a)->a_next ) {
		if ( (*a)->a_desc == desc ) {
			break;
		}
	}

	if ( *a == NULL ) {
		*a = attr_alloc( desc );
	}

	return attr_valadd( *a, val, nval, 1 );
}

/*
 * if a normalization function is defined for the equality matchingRule
 * of desc, the value is normalized and stored in nval; otherwise nval 
 * is NULL
 */
int
attr_normalize_one(
	AttributeDescription *desc,
	struct berval	*val,
	struct berval	*nval,
	void		*memctx )
{
	int		rc = LDAP_SUCCESS;

	BER_BVZERO( nval );

	if ( desc->ad_type->sat_equality &&
		desc->ad_type->sat_equality->smr_normalize )
	{
		rc = desc->ad_type->sat_equality->smr_normalize(
				SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
				desc->ad_type->sat_syntax,
				desc->ad_type->sat_equality,
				val, nval, memctx );

		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	return rc;
}

int
attr_merge_normalize_one(
	Entry		*e,
	AttributeDescription *desc,
	struct berval	*val,
	void		*memctx )
{
	struct berval	nval = BER_BVNULL;
	struct berval	*nvalp = NULL;
	int		rc;

	rc = attr_normalize_one( desc, val, &nval, memctx );
	if ( rc == LDAP_SUCCESS && !BER_BVISNULL( &nval ) ) {
		nvalp = &nval;
	}

	rc = attr_merge_one( e, desc, val, nvalp );
	if ( nvalp != NULL ) {
		slap_sl_free( nval.bv_val, memctx );
	}
	return rc;
}

/*
 * attrs_find - find attribute(s) by AttributeDescription
 * returns next attribute which is subtype of provided description.
 */

Attribute *
attrs_find(
    Attribute	*a,
	AttributeDescription *desc )
{
	for ( ; a != NULL; a = a->a_next ) {
		if ( is_ad_subtype( a->a_desc, desc ) ) {
			return( a );
		}
	}

	return( NULL );
}

/*
 * attr_find - find attribute by type
 */

Attribute *
attr_find(
    Attribute	*a,
	AttributeDescription *desc )
{
	for ( ; a != NULL; a = a->a_next ) {
		if ( a->a_desc == desc ) {
			return( a );
		}
	}

	return( NULL );
}

/*
 * attr_delete - delete the attribute type in list pointed to by attrs
 * return	0	deleted ok
 * 		1	not found in list a
 * 		-1	something bad happened
 */

int
attr_delete(
    Attribute	**attrs,
	AttributeDescription *desc )
{
	Attribute	**a;

	for ( a = attrs; *a != NULL; a = &(*a)->a_next ) {
		if ( (*a)->a_desc == desc ) {
			Attribute	*save = *a;
			*a = (*a)->a_next;
			attr_free( save );

			return LDAP_SUCCESS;
		}
	}

	return LDAP_NO_SUCH_ATTRIBUTE;
}

int
attr_init( void )
{
	ldap_pvt_thread_mutex_init( &attr_mutex );
	return 0;
}

int
attr_destroy( void )
{
	slap_list *a;

	for ( a=attr_chunks; a; a=attr_chunks ) {
		attr_chunks = a->next;
		free( a );
	}
	ldap_pvt_thread_mutex_destroy( &attr_mutex );
	return 0;
}
