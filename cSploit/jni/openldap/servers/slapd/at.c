/* at.c - routines for dealing with attribute types */
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

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"


const char *
at_syntax(
	AttributeType	*at )
{
	for ( ; at != NULL; at = at->sat_sup ) {
		if ( at->sat_syntax_oid ) {
			return at->sat_syntax_oid;
		}
	}

	assert( 0 );

	return NULL;
}

int
is_at_syntax(
	AttributeType	*at,
	const char	*oid )
{
	const char *syn_oid = at_syntax( at );

	if ( syn_oid ) {
		return strcmp( syn_oid, oid ) == 0;
	}

	return 0;
}

int is_at_subtype(
	AttributeType *sub,
	AttributeType *sup )
{
	for( ; sub != NULL; sub = sub->sat_sup ) {
		if( sub == sup ) return 1;
	}

	return 0;
}

struct aindexrec {
	struct berval	air_name;
	AttributeType	*air_at;
};

static Avlnode	*attr_index = NULL;
static Avlnode	*attr_cache = NULL;
static LDAP_STAILQ_HEAD(ATList, AttributeType) attr_list
	= LDAP_STAILQ_HEAD_INITIALIZER(attr_list);

/* Last hardcoded attribute registered */
AttributeType *at_sys_tail;

int at_oc_cache;

static int
attr_index_cmp(
    const void	*v_air1,
    const void	*v_air2 )
{
	const struct aindexrec	*air1 = v_air1;
	const struct aindexrec	*air2 = v_air2;
	int i = air1->air_name.bv_len - air2->air_name.bv_len;
	if (i) return i;
	return (strcasecmp( air1->air_name.bv_val, air2->air_name.bv_val ));
}

static int
attr_index_name_cmp(
    const void	*v_type,
    const void	*v_air )
{
    const struct berval    *type = v_type;
    const struct aindexrec *air  = v_air;
	int i = type->bv_len - air->air_name.bv_len;
	if (i) return i;
	return (strncasecmp( type->bv_val, air->air_name.bv_val, type->bv_len ));
}

AttributeType *
at_find( const char *name )
{
	struct berval bv;

	bv.bv_val = (char *)name;
	bv.bv_len = strlen( name );

	return at_bvfind( &bv );
}

AttributeType *
at_bvfind( struct berval *name )
{
	struct aindexrec *air;

	if ( attr_cache ) {
		air = avl_find( attr_cache, name, attr_index_name_cmp );
		if ( air ) return air->air_at;
	}

	air = avl_find( attr_index, name, attr_index_name_cmp );

	if ( air ) {
		if ( air->air_at->sat_flags & SLAP_AT_DELETED ) {
			air = NULL;
		} else if (( slapMode & SLAP_TOOL_MODE ) && at_oc_cache ) {
			avl_insert( &attr_cache, (caddr_t) air,
				attr_index_cmp, avl_dup_error );
		}
	}

	return air != NULL ? air->air_at : NULL;
}

int
at_append_to_list(
    AttributeType	*sat,
    AttributeType	***listp )
{
	AttributeType	**list;
	AttributeType	**list1;
	int		size;

	list = *listp;
	if ( !list ) {
		size = 2;
		list = ch_calloc(size, sizeof(AttributeType *));
		if ( !list ) {
			return -1;
		}
	} else {
		size = 0;
		list1 = *listp;
		while ( *list1 ) {
			size++;
			list1++;
		}
		size += 2;
		list1 = ch_realloc(list, size*sizeof(AttributeType *));
		if ( !list1 ) {
			return -1;
		}
		list = list1;
	}
	list[size-2] = sat;
	list[size-1] = NULL;
	*listp = list;
	return 0;
}

int
at_delete_from_list(
    int			pos,
    AttributeType	***listp )
{
	AttributeType	**list;
	AttributeType	**list1;
	int		i;
	int		j;

	if ( pos < 0 ) {
		return -2;
	}
	list = *listp;
	for ( i=0; list[i]; i++ )
		;
	if ( pos >= i ) {
		return -2;
	}
	for ( i=pos, j=pos+1; list[j]; i++, j++ ) {
		list[i] = list[j];
	}
	list[i] = NULL;
	/* Tell the runtime this can be shrinked */
	list1 = ch_realloc(list, (i+1)*sizeof(AttributeType **));
	if ( !list1 ) {
		return -1;
	}
	*listp = list1;
	return 0;
}

int
at_find_in_list(
    AttributeType	*sat,
    AttributeType	**list )
{
	int	i;

	if ( !list ) {
		return -1;
	}
	for ( i=0; list[i]; i++ ) {
		if ( sat == list[i] ) {
			return i;
		}
	}
	return -1;
}

static void
at_delete_names( AttributeType *at )
{
	char			**names = at->sat_names;

	if (!names) return;

	while (*names) {
		struct aindexrec	tmpair, *air;

		ber_str2bv( *names, 0, 0, &tmpair.air_name );
		tmpair.air_at = at;
		air = (struct aindexrec *)avl_delete( &attr_index,
			(caddr_t)&tmpair, attr_index_cmp );
		assert( air != NULL );
		ldap_memfree( air );
		names++;
	}
}

/* Mark the attribute as deleted, remove from list, and remove all its
 * names from the AVL tree. Leave the OID in the tree.
 */
void
at_delete( AttributeType *at )
{
	at->sat_flags |= SLAP_AT_DELETED;

	LDAP_STAILQ_REMOVE(&attr_list, at, AttributeType, sat_next);

	at_delete_names( at );
}

static void
at_clean( AttributeType *a )
{
	if ( a->sat_equality ) {
		MatchingRule	*mr;

		mr = mr_find( a->sat_equality->smr_oid );
		assert( mr != NULL );
		if ( mr != a->sat_equality ) {
			ch_free( a->sat_equality );
			a->sat_equality = NULL;
		}
	}

	assert( a->sat_syntax != NULL );
	if ( a->sat_syntax != NULL ) {
		Syntax		*syn;

		syn = syn_find( a->sat_syntax->ssyn_oid );
		assert( syn != NULL );
		if ( syn != a->sat_syntax ) {
			ch_free( a->sat_syntax );
			a->sat_syntax = NULL;
		}
	}

	if ( a->sat_oidmacro ) {
		ldap_memfree( a->sat_oidmacro );
		a->sat_oidmacro = NULL;
	}
	if ( a->sat_soidmacro ) {
		ldap_memfree( a->sat_soidmacro );
		a->sat_soidmacro = NULL;
	}
	if ( a->sat_subtypes ) {
		ldap_memfree( a->sat_subtypes );
		a->sat_subtypes = NULL;
	}
}

static void
at_destroy_one( void *v )
{
	struct aindexrec *air = v;
	AttributeType *a = air->air_at;

	at_clean( a );
	ad_destroy(a->sat_ad);
	ldap_pvt_thread_mutex_destroy(&a->sat_ad_mutex);
	ldap_attributetype_free((LDAPAttributeType *)a);
	ldap_memfree(air);
}

void
at_destroy( void )
{
	AttributeType *a;

	while( !LDAP_STAILQ_EMPTY(&attr_list) ) {
		a = LDAP_STAILQ_FIRST(&attr_list);
		LDAP_STAILQ_REMOVE_HEAD(&attr_list, sat_next);

		at_delete_names( a );
	}

	avl_free(attr_index, at_destroy_one);

	if ( slap_schema.si_at_undefined ) {
		ad_destroy(slap_schema.si_at_undefined->sat_ad);
	}

	if ( slap_schema.si_at_proxied ) {
		ad_destroy(slap_schema.si_at_proxied->sat_ad);
	}
}

int
at_start( AttributeType **at )
{
	assert( at != NULL );

	*at = LDAP_STAILQ_FIRST(&attr_list);

	return (*at != NULL);
}

int
at_next( AttributeType **at )
{
	assert( at != NULL );

#if 0	/* pedantic check: don't use this */
	{
		AttributeType *tmp = NULL;

		LDAP_STAILQ_FOREACH(tmp,&attr_list,sat_next) {
			if ( tmp == *at ) {
				break;
			}
		}

		assert( tmp != NULL );
	}
#endif

	*at = LDAP_STAILQ_NEXT(*at,sat_next);

	return (*at != NULL);
}

/*
 * check whether the two attributeTypes actually __are__ identical,
 * or rather inconsistent
 */
static int
at_check_dup(
	AttributeType		*sat,
	AttributeType		*new_sat )
{
	if ( new_sat->sat_oid != NULL ) {
		if ( sat->sat_oid == NULL ) {
			return SLAP_SCHERR_ATTR_INCONSISTENT;
		}

		if ( strcmp( sat->sat_oid, new_sat->sat_oid ) != 0 ) {
			return SLAP_SCHERR_ATTR_INCONSISTENT;
		}

	} else {
		if ( sat->sat_oid != NULL ) {
			return SLAP_SCHERR_ATTR_INCONSISTENT;
		}
	}

	if ( new_sat->sat_names ) {
		int	i;

		if ( sat->sat_names == NULL ) {
			return SLAP_SCHERR_ATTR_INCONSISTENT;
		}

		for ( i = 0; new_sat->sat_names[ i ]; i++ ) {
			if ( sat->sat_names[ i ] == NULL ) {
				return SLAP_SCHERR_ATTR_INCONSISTENT;
			}
			
			if ( strcasecmp( sat->sat_names[ i ],
					new_sat->sat_names[ i ] ) != 0 )
			{
				return SLAP_SCHERR_ATTR_INCONSISTENT;
			}
		}
	} else {
		if ( sat->sat_names != NULL ) {
			return SLAP_SCHERR_ATTR_INCONSISTENT;
		}
	}

	return SLAP_SCHERR_ATTR_DUP;
}

static struct aindexrec *air_old;

static int
at_dup_error( void *left, void *right )
{
	air_old = left;
	return -1;
}

static int
at_insert(
    AttributeType	**rat,
	AttributeType	*prev,
    const char		**err )
{
	struct aindexrec	*air;
	char			**names = NULL;
	AttributeType	*sat = *rat;

	if ( sat->sat_oid ) {
		air = (struct aindexrec *)
			ch_calloc( 1, sizeof(struct aindexrec) );
		ber_str2bv( sat->sat_oid, 0, 0, &air->air_name );
		air->air_at = sat;
		air_old = NULL;

		if ( avl_insert( &attr_index, (caddr_t) air,
		                 attr_index_cmp, at_dup_error ) )
		{
			AttributeType	*old_sat;
			int		rc;

			*err = sat->sat_oid;

			assert( air_old != NULL );
			old_sat = air_old->air_at;

			/* replacing a deleted definition? */
			if ( old_sat->sat_flags & SLAP_AT_DELETED ) {
				AttributeType tmp;
				AttributeDescription *ad;
				
				/* Keep old oid, free new oid;
				 * Keep old ads, free new ads;
				 * Keep old ad_mutex, free new ad_mutex;
				 * Keep new everything else, free old
				 */
				tmp = *old_sat;
				*old_sat = *sat;
				old_sat->sat_oid = tmp.sat_oid;
				tmp.sat_oid = sat->sat_oid;
				old_sat->sat_ad = tmp.sat_ad;
				tmp.sat_ad = sat->sat_ad;
				old_sat->sat_ad_mutex = tmp.sat_ad_mutex;
				tmp.sat_ad_mutex = sat->sat_ad_mutex;
				*sat = tmp;

				/* Check for basic ad pointing at old cname */
				for ( ad = old_sat->sat_ad; ad; ad=ad->ad_next ) {
					if ( ad->ad_cname.bv_val == sat->sat_cname.bv_val ) {
						ad->ad_cname = old_sat->sat_cname;
						break;
					}
				}

				at_clean( sat );
				at_destroy_one( air );

				air = air_old;
				sat = old_sat;
				*rat = sat;
			} else {
				ldap_memfree( air );

				rc = at_check_dup( old_sat, sat );

				return rc;
			}
		}
		/* FIX: temporal consistency check */
		at_bvfind( &air->air_name );
	}

	names = sat->sat_names;
	if ( names ) {
		while ( *names ) {
			air = (struct aindexrec *)
				ch_calloc( 1, sizeof(struct aindexrec) );
			ber_str2bv( *names, 0, 0, &air->air_name );
			air->air_at = sat;
			if ( avl_insert( &attr_index, (caddr_t) air,
			                 attr_index_cmp, avl_dup_error ) )
			{
				AttributeType	*old_sat;
				int		rc;

				*err = *names;

				old_sat = at_bvfind( &air->air_name );
				assert( old_sat != NULL );
				rc = at_check_dup( old_sat, sat );

				ldap_memfree(air);

				while ( names > sat->sat_names ) {
					struct aindexrec	tmpair;

					names--;
					ber_str2bv( *names, 0, 0, &tmpair.air_name );
					tmpair.air_at = sat;
					air = (struct aindexrec *)avl_delete( &attr_index,
						(caddr_t)&tmpair, attr_index_cmp );
					assert( air != NULL );
					ldap_memfree( air );
				}

				if ( sat->sat_oid ) {
					struct aindexrec	tmpair;

					ber_str2bv( sat->sat_oid, 0, 0, &tmpair.air_name );
					tmpair.air_at = sat;
					air = (struct aindexrec *)avl_delete( &attr_index,
						(caddr_t)&tmpair, attr_index_cmp );
					assert( air != NULL );
					ldap_memfree( air );
				}

				return rc;
			}
			/* FIX: temporal consistency check */
			at_bvfind(&air->air_name);
			names++;
		}
	}

	if ( sat->sat_oid ) {
		slap_ad_undef_promote( sat->sat_oid, sat );
	}

	names = sat->sat_names;
	if ( names ) {
		while ( *names ) {
			slap_ad_undef_promote( *names, sat );
			names++;
		}
	}

	if ( sat->sat_flags & SLAP_AT_HARDCODE ) {
		prev = at_sys_tail;
		at_sys_tail = sat;
	}
	if ( prev ) {
		LDAP_STAILQ_INSERT_AFTER( &attr_list, prev, sat, sat_next );
	} else {
		LDAP_STAILQ_INSERT_TAIL( &attr_list, sat, sat_next );
	}

	return 0;
}

int
at_add(
	LDAPAttributeType	*at,
	int			user,
	AttributeType		**rsat,
	AttributeType	*prev,
	const char		**err )
{
	AttributeType	*sat = NULL;
	MatchingRule	*mr = NULL;
	Syntax		*syn = NULL;
	int		i;
	int		code = LDAP_SUCCESS;
	char		*cname = NULL;
	char		*oidm = NULL;
	char		*soidm = NULL;

	if ( !at->at_oid ) {
		*err = "";
		return SLAP_SCHERR_ATTR_INCOMPLETE;
	}

	if ( !OID_LEADCHAR( at->at_oid[0] )) {
		char	*oid;

		/* Expand OID macros */
		oid = oidm_find( at->at_oid );
		if ( !oid ) {
			*err = at->at_oid;
			return SLAP_SCHERR_OIDM;
		}
		if ( oid != at->at_oid ) {
			oidm = at->at_oid;
			at->at_oid = oid;
		}
	}

	if ( at->at_syntax_oid && !OID_LEADCHAR( at->at_syntax_oid[0] )) {
		char	*oid;

		/* Expand OID macros */
		oid = oidm_find( at->at_syntax_oid );
		if ( !oid ) {
			*err = at->at_syntax_oid;
			code = SLAP_SCHERR_OIDM;
			goto error_return;
		}
		if ( oid != at->at_syntax_oid ) {
			soidm = at->at_syntax_oid;
			at->at_syntax_oid = oid;
		}
	}

	if ( at->at_names && at->at_names[0] ) {
		int i;

		for( i=0; at->at_names[i]; i++ ) {
			if( !slap_valid_descr( at->at_names[i] ) ) {
				*err = at->at_names[i];
				code = SLAP_SCHERR_BAD_DESCR;
				goto error_return;
			}
		}

		cname = at->at_names[0];

	} else {
		cname = at->at_oid;

	}

	*err = cname;

	if ( !at->at_usage && at->at_no_user_mod ) {
		/* user attribute must be modifable */
		code = SLAP_SCHERR_ATTR_BAD_USAGE;
		goto error_return;
	}

	if ( at->at_collective ) {
		if( at->at_usage ) {
			/* collective attributes cannot be operational */
			code = SLAP_SCHERR_ATTR_BAD_USAGE;
			goto error_return;
		}

		if( at->at_single_value ) {
			/* collective attributes cannot be single-valued */
			code = SLAP_SCHERR_ATTR_BAD_USAGE;
			goto error_return;
		}
	}

	sat = (AttributeType *) ch_calloc( 1, sizeof(AttributeType) );
	AC_MEMCPY( &sat->sat_atype, at, sizeof(LDAPAttributeType));

	sat->sat_cname.bv_val = cname;
	sat->sat_cname.bv_len = strlen( cname );
	sat->sat_oidmacro = oidm;
	sat->sat_soidmacro = soidm;
	ldap_pvt_thread_mutex_init(&sat->sat_ad_mutex);

	if ( at->at_sup_oid ) {
		AttributeType *supsat = at_find(at->at_sup_oid);

		if ( supsat == NULL ) {
			*err = at->at_sup_oid;
			code = SLAP_SCHERR_ATTR_NOT_FOUND;
			goto error_return;
		}

		sat->sat_sup = supsat;

		if ( at_append_to_list(sat, &supsat->sat_subtypes) ) {
			code = SLAP_SCHERR_OUTOFMEM;
			goto error_return;
		}

		if ( sat->sat_usage != supsat->sat_usage ) {
			/* subtypes must have same usage as their SUP */
			code = SLAP_SCHERR_ATTR_BAD_USAGE;
			goto error_return;
		}

		if ( supsat->sat_obsolete && !sat->sat_obsolete ) {
			/* subtypes must be obsolete if super is */
			code = SLAP_SCHERR_ATTR_BAD_SUP;
			goto error_return;
		}

		if ( sat->sat_flags & SLAP_AT_FINAL ) {
			/* cannot subtype a "final" attribute type */
			code = SLAP_SCHERR_ATTR_BAD_SUP;
			goto error_return;
		}
	}

	/*
	 * Inherit definitions from superiors.  We only check the
	 * direct superior since that one has already inherited from
	 * its own superiorss
	 */
	if ( sat->sat_sup ) {
		Syntax *syn = syn_find(sat->sat_sup->sat_syntax->ssyn_oid);
		if ( syn != sat->sat_sup->sat_syntax ) {
			sat->sat_syntax = ch_malloc( sizeof( Syntax ));
			*sat->sat_syntax = *sat->sat_sup->sat_syntax;
		} else {
			sat->sat_syntax = sat->sat_sup->sat_syntax;
		}
		if ( sat->sat_sup->sat_equality ) {
			MatchingRule *mr = mr_find( sat->sat_sup->sat_equality->smr_oid );
			if ( mr != sat->sat_sup->sat_equality ) {
				sat->sat_equality = ch_malloc( sizeof( MatchingRule ));
				*sat->sat_equality = *sat->sat_sup->sat_equality;
			} else {
				sat->sat_equality = sat->sat_sup->sat_equality;
			}
		}
		sat->sat_approx = sat->sat_sup->sat_approx;
		sat->sat_ordering = sat->sat_sup->sat_ordering;
		sat->sat_substr = sat->sat_sup->sat_substr;
	}

	/*
	 * check for X-ORDERED attributes
	 */
	if ( sat->sat_extensions ) {
		for (i=0; sat->sat_extensions[i]; i++) {
			if (!strcasecmp( sat->sat_extensions[i]->lsei_name,
				"X-ORDERED" ) && sat->sat_extensions[i]->lsei_values ) {
				if ( !strcasecmp( sat->sat_extensions[i]->lsei_values[0],
					"VALUES" )) {
					sat->sat_flags |= SLAP_AT_ORDERED_VAL;
					break;
				} else if ( !strcasecmp( sat->sat_extensions[i]->lsei_values[0],
					"SIBLINGS" )) {
					sat->sat_flags |= SLAP_AT_ORDERED_SIB;
					break;
				}
			}
		}
	}

	if ( !user )
		sat->sat_flags |= SLAP_AT_HARDCODE;

	if ( at->at_syntax_oid ) {
		syn = syn_find(sat->sat_syntax_oid);
		if ( syn == NULL ) {
			*err = sat->sat_syntax_oid;
			code = SLAP_SCHERR_SYN_NOT_FOUND;
			goto error_return;
		}

		if ( sat->sat_syntax != NULL && sat->sat_syntax != syn ) {
			/* BEWARE: no loop detection! */
			if ( syn_is_sup( sat->sat_syntax, syn ) ) {
				code = SLAP_SCHERR_ATTR_BAD_SUP;
				goto error_return;
			}
		}

		sat->sat_syntax = syn;

	} else if ( sat->sat_syntax == NULL ) {
		code = SLAP_SCHERR_ATTR_INCOMPLETE;
		goto error_return;
	}

	if ( sat->sat_equality_oid ) {
		mr = mr_find(sat->sat_equality_oid);

		if( mr == NULL ) {
			*err = sat->sat_equality_oid;
			code = SLAP_SCHERR_MR_NOT_FOUND;
			goto error_return;
		}

		if(( mr->smr_usage & SLAP_MR_EQUALITY ) != SLAP_MR_EQUALITY ) {
			*err = sat->sat_equality_oid;
			code = SLAP_SCHERR_ATTR_BAD_MR;
			goto error_return;
		}

		if( sat->sat_syntax != mr->smr_syntax ) {
			if( mr->smr_compat_syntaxes == NULL ) {
				*err = sat->sat_equality_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}

			for(i=0; mr->smr_compat_syntaxes[i]; i++) {
				if( sat->sat_syntax == mr->smr_compat_syntaxes[i] ) {
					i = -1;
					break;
				}
			}

			if( i >= 0 ) {
				*err = sat->sat_equality_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}
		}

		sat->sat_equality = mr;
		sat->sat_approx = mr->smr_associated;
	}

	if ( sat->sat_ordering_oid ) {
		if( !sat->sat_equality ) {
			*err = sat->sat_ordering_oid;
			code = SLAP_SCHERR_ATTR_BAD_MR;
			goto error_return;
		}

		mr = mr_find(sat->sat_ordering_oid);

		if( mr == NULL ) {
			*err = sat->sat_ordering_oid;
			code = SLAP_SCHERR_MR_NOT_FOUND;
			goto error_return;
		}

		if(( mr->smr_usage & SLAP_MR_ORDERING ) != SLAP_MR_ORDERING ) {
			*err = sat->sat_ordering_oid;
			code = SLAP_SCHERR_ATTR_BAD_MR;
			goto error_return;
		}

		if( sat->sat_syntax != mr->smr_syntax ) {
			if( mr->smr_compat_syntaxes == NULL ) {
				*err = sat->sat_ordering_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}

			for(i=0; mr->smr_compat_syntaxes[i]; i++) {
				if( sat->sat_syntax == mr->smr_compat_syntaxes[i] ) {
					i = -1;
					break;
				}
			}

			if( i >= 0 ) {
				*err = sat->sat_ordering_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}
		}

		sat->sat_ordering = mr;
	}

	if ( sat->sat_substr_oid ) {
		if( !sat->sat_equality ) {
			*err = sat->sat_substr_oid;
			code = SLAP_SCHERR_ATTR_BAD_MR;
			goto error_return;
		}

		mr = mr_find(sat->sat_substr_oid);

		if( mr == NULL ) {
			*err = sat->sat_substr_oid;
			code = SLAP_SCHERR_MR_NOT_FOUND;
			goto error_return;
		}

		if(( mr->smr_usage & SLAP_MR_SUBSTR ) != SLAP_MR_SUBSTR ) {
			*err = sat->sat_substr_oid;
			code = SLAP_SCHERR_ATTR_BAD_MR;
			goto error_return;
		}

		/* due to funky LDAP builtin substring rules,
		 * we check against the equality rule assertion
		 * syntax and compat syntaxes instead of those
		 * associated with the substrings rule.
		 */
		if( sat->sat_syntax != sat->sat_equality->smr_syntax ) {
			if( sat->sat_equality->smr_compat_syntaxes == NULL ) {
				*err = sat->sat_substr_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}

			for(i=0; sat->sat_equality->smr_compat_syntaxes[i]; i++) {
				if( sat->sat_syntax ==
					sat->sat_equality->smr_compat_syntaxes[i] )
				{
					i = -1;
					break;
				}
			}

			if( i >= 0 ) {
				*err = sat->sat_substr_oid;
				code = SLAP_SCHERR_ATTR_BAD_MR;
				goto error_return;
			}
		}

		sat->sat_substr = mr;
	}

	code = at_insert( &sat, prev, err );
	if ( code != 0 ) {
error_return:;
		if ( sat ) {
			ldap_pvt_thread_mutex_destroy( &sat->sat_ad_mutex );
			ch_free( sat );
		}

		if ( oidm ) {
			SLAP_FREE( at->at_oid );
			at->at_oid = oidm;
		}

		if ( soidm ) {
			SLAP_FREE( at->at_syntax_oid );
			at->at_syntax_oid = soidm;
		}

	} else if ( rsat ) {
		*rsat = sat;
	}

	return code;
}

#ifdef LDAP_DEBUG
#ifdef SLAPD_UNUSED
static int
at_index_printnode( void *v_air, void *ignore )
{
	struct aindexrec *air = v_air;
	printf("%s = %s\n",
		air->air_name.bv_val,
		ldap_attributetype2str(&air->air_at->sat_atype) );
	return( 0 );
}

static void
at_index_print( void )
{
	printf("Printing attribute type index:\n");
	(void) avl_apply( attr_index, at_index_printnode, 0, -1, AVL_INORDER );
}
#endif
#endif

void
at_unparse( BerVarray *res, AttributeType *start, AttributeType *end, int sys )
{
	AttributeType *at;
	int i, num;
	struct berval bv, *bva = NULL, idx;
	char ibuf[32];

	if ( !start )
		start = LDAP_STAILQ_FIRST( &attr_list );

	/* count the result size */
	i = 0;
	for ( at=start; at; at=LDAP_STAILQ_NEXT(at, sat_next)) {
		if ( sys && !(at->sat_flags & SLAP_AT_HARDCODE)) break;
		i++;
		if ( at == end ) break;
	}
	if (!i) return;

	num = i;
	bva = ch_malloc( (num+1) * sizeof(struct berval) );
	BER_BVZERO( bva );
	idx.bv_val = ibuf;
	if ( sys ) {
		idx.bv_len = 0;
		ibuf[0] = '\0';
	}
	i = 0;
	for ( at=start; at; at=LDAP_STAILQ_NEXT(at, sat_next)) {
		LDAPAttributeType lat, *latp;
		if ( sys && !(at->sat_flags & SLAP_AT_HARDCODE)) break;
		if ( at->sat_oidmacro || at->sat_soidmacro ) {
			lat = at->sat_atype;
			if ( at->sat_oidmacro )
				lat.at_oid = at->sat_oidmacro;
			if ( at->sat_soidmacro )
				lat.at_syntax_oid = at->sat_soidmacro;
			latp = &lat;
		} else {
			latp = &at->sat_atype;
		}
		if ( ldap_attributetype2bv( latp, &bv ) == NULL ) {
			ber_bvarray_free( bva );
		}
		if ( !sys ) {
			idx.bv_len = sprintf(idx.bv_val, "{%d}", i);
		}
		bva[i].bv_len = idx.bv_len + bv.bv_len;
		bva[i].bv_val = ch_malloc( bva[i].bv_len + 1 );
		strcpy( bva[i].bv_val, ibuf );
		strcpy( bva[i].bv_val + idx.bv_len, bv.bv_val );
		i++;
		bva[i].bv_val = NULL;
		ldap_memfree( bv.bv_val );
		if ( at == end ) break;
	}
	*res = bva;
}

int
at_schema_info( Entry *e )
{
	AttributeDescription *ad_attributeTypes = slap_schema.si_ad_attributeTypes;
	AttributeType	*at;
	struct berval	val;
	struct berval	nval;

	LDAP_STAILQ_FOREACH(at,&attr_list,sat_next) {
		if( at->sat_flags & SLAP_AT_HIDE ) continue;

		if ( ldap_attributetype2bv( &at->sat_atype, &val ) == NULL ) {
			return -1;
		}

		ber_str2bv( at->sat_oid, 0, 0, &nval );

		if( attr_merge_one( e, ad_attributeTypes, &val, &nval ) )
		{
			return -1;
		}
		ldap_memfree( val.bv_val );
	}
	return 0;
}

int
register_at( const char *def, AttributeDescription **rad, int dupok )
{
	LDAPAttributeType *at;
	int code, freeit = 0;
	const char *err;
	AttributeDescription *ad = NULL;

	at = ldap_str2attributetype( def, &code, &err, LDAP_SCHEMA_ALLOW_ALL );
	if ( !at ) {
		Debug( LDAP_DEBUG_ANY,
			"register_at: AttributeType \"%s\": %s, %s\n",
				def, ldap_scherr2str(code), err );
		return code;
	}

	code = at_add( at, 0, NULL, NULL, &err );
	if ( code ) {
		if ( code == SLAP_SCHERR_ATTR_DUP && dupok ) {
			freeit = 1;

		} else {
			Debug( LDAP_DEBUG_ANY,
				"register_at: AttributeType \"%s\": %s, %s\n",
				def, scherr2str(code), err );
			ldap_attributetype_free( at );
			return code;
		}
	}
	code = slap_str2ad( at->at_names[0], &ad, &err );
	if ( freeit || code ) {
		ldap_attributetype_free( at );
	} else {
		ldap_memfree( at );
	}
	if ( code ) {
		Debug( LDAP_DEBUG_ANY, "register_at: AttributeType \"%s\": %s\n",
			def, err, 0 );
	}
	if ( rad ) *rad = ad;
	return code;
}
