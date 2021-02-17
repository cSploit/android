/* rwmmap.c - rewrite/mapping routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#ifdef SLAPD_OVER_RWM

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "rwm.h"

#undef ldap_debug	/* silence a warning in ldap-int.h */
#include "../../../libraries/libldap/ldap-int.h"

int
rwm_mapping_cmp( const void *c1, const void *c2 )
{
	struct ldapmapping *map1 = (struct ldapmapping *)c1;
	struct ldapmapping *map2 = (struct ldapmapping *)c2;
	int rc = map1->m_src.bv_len - map2->m_src.bv_len;
	
	if ( rc ) {
		return rc;
	}

	return strcasecmp( map1->m_src.bv_val, map2->m_src.bv_val );
}

int
rwm_mapping_dup( void *c1, void *c2 )
{
	struct ldapmapping *map1 = (struct ldapmapping *)c1;
	struct ldapmapping *map2 = (struct ldapmapping *)c2;
	int rc = map1->m_src.bv_len - map2->m_src.bv_len;

	if ( rc ) {
		return 0;
	}

	return ( ( strcasecmp( map1->m_src.bv_val, map2->m_src.bv_val ) == 0 ) ? -1 : 0 );
}

int
rwm_map_init( struct ldapmap *lm, struct ldapmapping **m )
{
	struct ldapmapping	*mapping;
	const char		*text;
	int			rc;

	assert( m != NULL );

	*m = NULL;
	
	mapping = (struct ldapmapping *)ch_calloc( 2, 
			sizeof( struct ldapmapping ) );
	if ( mapping == NULL ) {
		return LDAP_NO_MEMORY;
	}

	/* NOTE: this is needed to make sure that
	 *	rwm-map attribute *
	 * does not  filter out all attributes including objectClass */
	rc = slap_str2ad( "objectClass", &mapping[0].m_src_ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		ch_free( mapping );
		return rc;
	}

	mapping[0].m_dst_ad = mapping[0].m_src_ad;
	ber_dupbv( &mapping[0].m_src, &mapping[0].m_src_ad->ad_cname );
	ber_dupbv( &mapping[0].m_dst, &mapping[0].m_src );

	mapping[1].m_src = mapping[0].m_src;
	mapping[1].m_dst = mapping[0].m_dst;
	mapping[1].m_src_ad = mapping[0].m_src_ad;
	mapping[1].m_dst_ad = mapping[1].m_src_ad;

	avl_insert( &lm->map, (caddr_t)&mapping[0], 
			rwm_mapping_cmp, rwm_mapping_dup );
	avl_insert( &lm->remap, (caddr_t)&mapping[1], 
			rwm_mapping_cmp, rwm_mapping_dup );

	*m = mapping;

	return rc;
}

int
rwm_mapping( struct ldapmap *map, struct berval *s, struct ldapmapping **m, int remap )
{
	Avlnode *tree;
	struct ldapmapping fmapping;

	if ( map == NULL ) {
		return 0;
	}

	assert( m != NULL );

	/* let special attrnames slip through (ITS#5760) */
	if ( bvmatch( s, slap_bv_no_attrs )
		|| bvmatch( s, slap_bv_all_user_attrs )
		|| bvmatch( s, slap_bv_all_operational_attrs ) )
	{
		*m = NULL;
		return 0;
	}

	if ( remap == RWM_REMAP ) {
		tree = map->remap;

	} else {
		tree = map->map;
	}

	fmapping.m_src = *s;
	*m = (struct ldapmapping *)avl_find( tree, (caddr_t)&fmapping,
			rwm_mapping_cmp );

	if ( *m == NULL ) {
		return map->drop_missing;
	}

	return 0;
}

void
rwm_map( struct ldapmap *map, struct berval *s, struct berval *bv, int remap )
{
	struct ldapmapping *mapping;

	/* map->map may be NULL when mapping is configured,
	 * but map->remap can't */
	if ( map->remap == NULL ) {
		*bv = *s;
		return;
	}

	BER_BVZERO( bv );
	( void )rwm_mapping( map, s, &mapping, remap );
	if ( mapping != NULL ) {
		if ( !BER_BVISNULL( &mapping->m_dst ) ) {
			*bv = mapping->m_dst;
		}
		return;
	}

	if ( !map->drop_missing ) {
		*bv = *s;
	}
}

/*
 * Map attribute names in place
 */
int
rwm_map_attrnames(
	Operation	*op,
	struct ldapmap	*at_map,
	struct ldapmap	*oc_map,
	AttributeName	*an,
	AttributeName	**anp,
	int		remap )
{
	int		i, j, x;

	assert( anp != NULL );

	*anp = NULL;

	if ( an == NULL && op->o_bd->be_extra_anlist == NULL ) {
		return LDAP_SUCCESS;
	}

	i = 0;
	if ( an != NULL ) {
		for ( i = 0; !BER_BVISNULL( &an[i].an_name ); i++ )
			/* just count */ ;
	}

	x = 0;
	if ( op->o_bd->be_extra_anlist ) {
		for ( ; !BER_BVISNULL( &op->o_bd->be_extra_anlist[x].an_name ); x++ )
			/* just count */ ;
	}

	assert( i > 0 || x > 0 );
	*anp = op->o_tmpcalloc( ( i + x + 1 ), sizeof( AttributeName ),
		op->o_tmpmemctx );
	if ( *anp == NULL ) {
		return LDAP_NO_MEMORY;
	}

	for ( i = 0, j = 0; !BER_BVISNULL( &an[i].an_name ); i++ ) {
		struct ldapmapping	*m;
		int			at_drop_missing = 0,
					oc_drop_missing = 0;

		if ( an[i].an_desc ) {
			if ( !at_map ) {
				/* FIXME: better leave as is? */
				continue;
			}
				
			at_drop_missing = rwm_mapping( at_map, &an[i].an_name, &m, remap );
			if ( at_drop_missing || ( m && BER_BVISNULL( &m->m_dst ) ) ) {
				continue;
			}

			if ( !m ) {
				(*anp)[j] = an[i];
				j++;
				continue;
			}

			(*anp)[j] = an[i];
			if ( remap == RWM_MAP ) {
				(*anp)[j].an_name = m->m_dst;
				(*anp)[j].an_desc = m->m_dst_ad;
			} else {
				(*anp)[j].an_name = m->m_src;
				(*anp)[j].an_desc = m->m_src_ad;

			}

			j++;
			continue;

		} else if ( an[i].an_oc ) {
			if ( !oc_map ) {
				/* FIXME: better leave as is? */
				continue;
			}

			oc_drop_missing = rwm_mapping( oc_map, &an[i].an_name, &m, remap );

			if ( oc_drop_missing || ( m && BER_BVISNULL( &m->m_dst ) ) ) {
				continue;
			}

			if ( !m ) {
				(*anp)[j] = an[i];
				j++;
				continue;
			}

			(*anp)[j] = an[i];
			if ( remap == RWM_MAP ) {
				(*anp)[j].an_name = m->m_dst;
				(*anp)[j].an_oc = m->m_dst_oc;
			} else {
				(*anp)[j].an_name = m->m_src;
				(*anp)[j].an_oc = m->m_src_oc;
			}

		} else {
			at_drop_missing = rwm_mapping( at_map, &an[i].an_name, &m, remap );
		
			if ( at_drop_missing || !m ) {
				oc_drop_missing = rwm_mapping( oc_map, &an[i].an_name, &m, remap );

				/* if both at_map and oc_map required to drop missing,
				 * then do it */
				if ( oc_drop_missing && at_drop_missing ) {
					continue;
				}

				/* if no oc_map mapping was found and at_map required
				 * to drop missing, then do it; otherwise, at_map wins
				 * and an is considered an attr and is left unchanged */
				if ( !m ) {
					if ( at_drop_missing ) {
						continue;
					}
					(*anp)[j] = an[i];
					j++;
					continue;
				}
	
				if ( BER_BVISNULL( &m->m_dst ) ) {
					continue;
				}

				(*anp)[j] = an[i];
				if ( remap == RWM_MAP ) {
					(*anp)[j].an_name = m->m_dst;
					(*anp)[j].an_oc = m->m_dst_oc;
				} else {
					(*anp)[j].an_name = m->m_src;
					(*anp)[j].an_oc = m->m_src_oc;
				}
				j++;
				continue;
			}

			if ( !BER_BVISNULL( &m->m_dst ) ) {
				(*anp)[j] = an[i];
				if ( remap == RWM_MAP ) {
					(*anp)[j].an_name = m->m_dst;
					(*anp)[j].an_desc = m->m_dst_ad;
				} else {
					(*anp)[j].an_name = m->m_src;
					(*anp)[j].an_desc = m->m_src_ad;
				}
				j++;
				continue;
			}
		}
	}

	if ( op->o_bd->be_extra_anlist != NULL ) {
		/* we assume be_extra_anlist are already mapped */
		for ( x = 0; !BER_BVISNULL( &op->o_bd->be_extra_anlist[x].an_name ); x++ ) {
			BER_BVZERO( &(*anp)[j].an_name );
			if ( op->o_bd->be_extra_anlist[x].an_desc &&
				ad_inlist( op->o_bd->be_extra_anlist[x].an_desc, *anp ) )
			{
				continue;
			}

			(*anp)[j] = op->o_bd->be_extra_anlist[x];
			j++;
		}
	}

	if ( j == 0 && ( i != 0 || x != 0 ) ) {
		memset( &(*anp)[0], 0, sizeof( AttributeName ) );
		(*anp)[0].an_name = *slap_bv_no_attrs;
		j = 1;
	}
	memset( &(*anp)[j], 0, sizeof( AttributeName ) );

	return LDAP_SUCCESS;
}

#if 0 /* unused! */
int
rwm_map_attrs(
	struct ldapmap	*at_map,
	AttributeName	*an,
	int		remap,
	char		***mapped_attrs )
{
	int i, j;
	char **na;

	if ( an == NULL ) {
		*mapped_attrs = NULL;
		return LDAP_SUCCESS;
	}

	for ( i = 0; !BER_BVISNULL( &an[ i ].an_name ); i++ )
		/* count'em */ ;

	na = (char **)ch_calloc( i + 1, sizeof( char * ) );
	if ( na == NULL ) {
		*mapped_attrs = NULL;
		return LDAP_NO_MEMORY;
	}

	for ( i = j = 0; !BER_BVISNULL( &an[i].an_name ); i++ ) {
		struct ldapmapping	*mapping;
		
		if ( rwm_mapping( at_map, &an[i].an_name, &mapping, remap ) ) {
			continue;
		}

		if ( !mapping ) {
			na[ j++ ] = an[ i ].an_name.bv_val;
			
		} else if ( !BER_BVISNULL( &mapping->m_dst ) ) {
			na[ j++ ] = mapping->m_dst.bv_val;
		}
	}

	if ( j == 0 && i != 0 ) {
		na[ j++ ] = LDAP_NO_ATTRS;
	}

	na[ j ] = NULL;

	*mapped_attrs = na;

	return LDAP_SUCCESS;
}
#endif

static int
map_attr_value(
	dncookie		*dc,
	AttributeDescription 	**adp,
	struct berval		*mapped_attr,
	struct berval		*value,
	struct berval		*mapped_value,
	int			remap,
	void			*memctx )
{
	struct berval		vtmp = BER_BVNULL;
	int			freeval = 0;
	AttributeDescription	*ad = *adp;
	struct ldapmapping	*mapping = NULL;

	rwm_mapping( &dc->rwmap->rwm_at, &ad->ad_cname, &mapping, remap );
	if ( mapping == NULL ) {
		if ( dc->rwmap->rwm_at.drop_missing ) {
			return -1;
		}

		*mapped_attr = ad->ad_cname;

	} else {
		*mapped_attr = mapping->m_dst;
	}

	if ( value != NULL ) {
		assert( mapped_value != NULL );

		if ( ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName
				|| ( mapping != NULL && mapping->m_dst_ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) )
		{
			dncookie 	fdc = *dc;
			int		rc;

			fdc.ctx = "searchFilterAttrDN";

			vtmp = *value;
			rc = rwm_dn_massage_normalize( &fdc, value, &vtmp );
			switch ( rc ) {
			case LDAP_SUCCESS:
				if ( vtmp.bv_val != value->bv_val ) {
					freeval = 1;
				}
				break;
		
			case LDAP_UNWILLING_TO_PERFORM:
			case LDAP_OTHER:
			default:
				return -1;
			}

		} else if ( ad->ad_type->sat_equality &&
			( ad->ad_type->sat_equality->smr_usage & SLAP_MR_MUTATION_NORMALIZER ) )
		{
			if ( ad->ad_type->sat_equality->smr_normalize(
				(SLAP_MR_DENORMALIZE|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX),
				NULL, NULL, value, &vtmp, memctx ) )
			{
				return -1;
			}
			freeval = 2;

		} else if ( ad == slap_schema.si_ad_objectClass
				|| ad == slap_schema.si_ad_structuralObjectClass )
		{
			rwm_map( &dc->rwmap->rwm_oc, value, &vtmp, remap );
			if ( BER_BVISNULL( &vtmp ) || BER_BVISEMPTY( &vtmp ) ) {
				vtmp = *value;
			}
		
		} else {
			vtmp = *value;
		}

		filter_escape_value_x( &vtmp, mapped_value, memctx );

		switch ( freeval ) {
		case 1:
			ch_free( vtmp.bv_val );
			break;

		case 2:
			ber_memfree_x( vtmp.bv_val, memctx );
			break;
		}
	}
	
	if ( mapping != NULL ) {
		assert( mapping->m_dst_ad != NULL );
		*adp = mapping->m_dst_ad;
	}

	return 0;
}

static int
rwm_int_filter_map_rewrite(
	Operation		*op,
	dncookie		*dc,
	Filter			*f,
	struct berval		*fstr )
{
	int		i;
	Filter		*p;
	AttributeDescription *ad;
	struct berval	atmp,
			vtmp,
			*tmp;
	static struct berval
			/* better than nothing... */
			ber_bvfalse = BER_BVC( "(!(objectClass=*))" ),
			ber_bvtf_false = BER_BVC( "(|)" ),
			/* better than nothing... */
			ber_bvtrue = BER_BVC( "(objectClass=*)" ),
			ber_bvtf_true = BER_BVC( "(&)" ),
#if 0
			/* no longer needed; preserved for completeness */
			ber_bvundefined = BER_BVC( "(?=undefined)" ),
#endif
			ber_bverror = BER_BVC( "(?=error)" ),
			ber_bvunknown = BER_BVC( "(?=unknown)" ),
			ber_bvnone = BER_BVC( "(?=none)" );
	ber_len_t	len;

	assert( fstr != NULL );
	BER_BVZERO( fstr );

	if ( f == NULL ) {
		ber_dupbv_x( fstr, &ber_bvnone, op->o_tmpmemctx );
		return LDAP_OTHER;
	}

#if 0
	/* ITS#6814: give the caller a chance to use undefined filters */
	if ( f->f_choice & SLAPD_FILTER_UNDEFINED ) {
		goto computed;
	}
#endif

	switch ( f->f_choice & SLAPD_FILTER_MASK ) {
	case LDAP_FILTER_EQUALITY:
		ad = f->f_av_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			&f->f_av_value, &vtmp, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		fstr->bv_len = atmp.bv_len + vtmp.bv_len + STRLENOF( "(=)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=%s)",
			atmp.bv_val, vtmp.bv_len ? vtmp.bv_val : "" );

		op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		break;

	case LDAP_FILTER_GE:
		ad = f->f_av_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			&f->f_av_value, &vtmp, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		fstr->bv_len = atmp.bv_len + vtmp.bv_len + STRLENOF( "(>=)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s>=%s)",
			atmp.bv_val, vtmp.bv_len ? vtmp.bv_val : "" );

		op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		break;

	case LDAP_FILTER_LE:
		ad = f->f_av_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			&f->f_av_value, &vtmp, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		fstr->bv_len = atmp.bv_len + vtmp.bv_len + STRLENOF( "(<=)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s<=%s)",
			atmp.bv_val, vtmp.bv_len ? vtmp.bv_val : "" );

		op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		break;

	case LDAP_FILTER_APPROX:
		ad = f->f_av_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			&f->f_av_value, &vtmp, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		fstr->bv_len = atmp.bv_len + vtmp.bv_len + STRLENOF( "(~=)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s~=%s)",
			atmp.bv_val, vtmp.bv_len ? vtmp.bv_val : "" );

		op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		ad = f->f_sub_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			NULL, NULL, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		/* cannot be a DN ... */

		fstr->bv_len = atmp.bv_len + STRLENOF( "(=*)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 128, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=*)",
			atmp.bv_val );

		if ( !BER_BVISNULL( &f->f_sub_initial ) ) {
			len = fstr->bv_len;

			filter_escape_value_x( &f->f_sub_initial, &vtmp, op->o_tmpmemctx );

			fstr->bv_len += vtmp.bv_len;
			fstr->bv_val = op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1,
				op->o_tmpmemctx );

			snprintf( &fstr->bv_val[len - 2], vtmp.bv_len + 3,
				/* "(attr=" */ "%s*)",
				vtmp.bv_len ? vtmp.bv_val : "" );

			op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		}

		if ( f->f_sub_any != NULL ) {
			for ( i = 0; !BER_BVISNULL( &f->f_sub_any[i] ); i++ ) {
				len = fstr->bv_len;
				filter_escape_value_x( &f->f_sub_any[i], &vtmp,
					op->o_tmpmemctx );

				fstr->bv_len += vtmp.bv_len + 1;
				fstr->bv_val = op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1,
					op->o_tmpmemctx );

				snprintf( &fstr->bv_val[len - 1], vtmp.bv_len + 3,
					/* "(attr=[init]*[any*]" */ "%s*)",
					vtmp.bv_len ? vtmp.bv_val : "" );
				op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
			}
		}

		if ( !BER_BVISNULL( &f->f_sub_final ) ) {
			len = fstr->bv_len;

			filter_escape_value_x( &f->f_sub_final, &vtmp, op->o_tmpmemctx );

			fstr->bv_len += vtmp.bv_len;
			fstr->bv_val = op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1,
				op->o_tmpmemctx );

			snprintf( &fstr->bv_val[len - 1], vtmp.bv_len + 3,
				/* "(attr=[init*][any*]" */ "%s)",
				vtmp.bv_len ? vtmp.bv_val : "" );

			op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		}

		break;

	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		if ( map_attr_value( dc, &ad, &atmp,
			NULL, NULL, RWM_MAP, op->o_tmpmemctx ) )
		{
			goto computed;
		}

		fstr->bv_len = atmp.bv_len + STRLENOF( "(=*)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s=*)",
			atmp.bv_val );
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		fstr->bv_len = STRLENOF( "(%)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 128, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%c)",
			f->f_choice == LDAP_FILTER_AND ? '&' :
			f->f_choice == LDAP_FILTER_OR ? '|' : '!' );

		for ( p = f->f_list; p != NULL; p = p->f_next ) {
			int	rc;

			len = fstr->bv_len;

			rc = rwm_int_filter_map_rewrite( op, dc, p, &vtmp );
			if ( rc != LDAP_SUCCESS ) {
				return rc;
			}
			
			fstr->bv_len += vtmp.bv_len;
			fstr->bv_val = op->o_tmprealloc( fstr->bv_val, fstr->bv_len + 1,
				op->o_tmpmemctx );

			snprintf( &fstr->bv_val[len-1], vtmp.bv_len + 2, 
				/*"("*/ "%s)", vtmp.bv_len ? vtmp.bv_val : "" );

			op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		}

		break;

	case LDAP_FILTER_EXT: {
		if ( f->f_mr_desc ) {
			ad = f->f_mr_desc;
			if ( map_attr_value( dc, &ad, &atmp,
				&f->f_mr_value, &vtmp, RWM_MAP, op->o_tmpmemctx ) )
			{
				goto computed;
			}

		} else {
			BER_BVSTR( &atmp, "" );
			filter_escape_value_x( &f->f_mr_value, &vtmp, op->o_tmpmemctx );
		}
			

		fstr->bv_len = atmp.bv_len +
			( f->f_mr_dnattrs ? STRLENOF( ":dn" ) : 0 ) +
			( f->f_mr_rule_text.bv_len ? f->f_mr_rule_text.bv_len + 1 : 0 ) +
			vtmp.bv_len + STRLENOF( "(:=)" );
		fstr->bv_val = op->o_tmpalloc( fstr->bv_len + 1, op->o_tmpmemctx );

		snprintf( fstr->bv_val, fstr->bv_len + 1, "(%s%s%s%s:=%s)",
			atmp.bv_val,
			f->f_mr_dnattrs ? ":dn" : "",
			!BER_BVISEMPTY( &f->f_mr_rule_text ) ? ":" : "",
			!BER_BVISEMPTY( &f->f_mr_rule_text ) ? f->f_mr_rule_text.bv_val : "",
			vtmp.bv_len ? vtmp.bv_val : "" );
		op->o_tmpfree( vtmp.bv_val, op->o_tmpmemctx );
		break;
	}

	case -1:
computed:;
		filter_free_x( op, f, 0 );
		f->f_choice = SLAPD_FILTER_COMPUTED;
		f->f_result = SLAPD_COMPARE_UNDEFINED;
		/* fallthru */

	case SLAPD_FILTER_COMPUTED:
		switch ( f->f_result ) {
		case LDAP_COMPARE_FALSE:
		/* FIXME: treat UNDEFINED as FALSE */
		case SLAPD_COMPARE_UNDEFINED:
			if ( dc->rwmap->rwm_flags & RWM_F_SUPPORT_T_F ) {
				tmp = &ber_bvtf_false;
				break;
			}
			tmp = &ber_bvfalse;
			break;

		case LDAP_COMPARE_TRUE:
			if ( dc->rwmap->rwm_flags & RWM_F_SUPPORT_T_F ) {
				tmp = &ber_bvtf_true;
				break;
			}
			tmp = &ber_bvtrue;
			break;
			
		default:
			tmp = &ber_bverror;
			break;
		}

		ber_dupbv_x( fstr, tmp, op->o_tmpmemctx );
		break;
		
	default:
		ber_dupbv_x( fstr, &ber_bvunknown, op->o_tmpmemctx );
		break;
	}

	return LDAP_SUCCESS;
}

int
rwm_filter_map_rewrite(
	Operation		*op,
	dncookie		*dc,
	Filter			*f,
	struct berval		*fstr )
{
	int		rc;
	dncookie 	fdc;
	struct berval	ftmp;

	rc = rwm_int_filter_map_rewrite( op, dc, f, fstr );

	if ( rc != 0 ) {
		return rc;
	}

	fdc = *dc;
	ftmp = *fstr;

	fdc.ctx = "searchFilter";

	switch ( rewrite_session( fdc.rwmap->rwm_rw, fdc.ctx, 
				( !BER_BVISEMPTY( &ftmp ) ? ftmp.bv_val : "" ), 
				fdc.conn, &fstr->bv_val ) )
	{
	case REWRITE_REGEXEC_OK:
		if ( !BER_BVISNULL( fstr ) ) {
			fstr->bv_len = strlen( fstr->bv_val );

		} else {
			*fstr = ftmp;
		}

		Debug( LDAP_DEBUG_ARGS,
			"[rw] %s: \"%s\" -> \"%s\"\n",
			fdc.ctx, ftmp.bv_val, fstr->bv_val );		
		if ( fstr->bv_val != ftmp.bv_val ) {
			ber_bvreplace_x( &ftmp, fstr, op->o_tmpmemctx );
			ch_free( fstr->bv_val );
			*fstr = ftmp;
		}
		rc = LDAP_SUCCESS;
		break;
 		
 	case REWRITE_REGEXEC_UNWILLING:
		if ( fdc.rs ) {
			fdc.rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			fdc.rs->sr_text = "Operation not allowed";
		}
		op->o_tmpfree( ftmp.bv_val, op->o_tmpmemctx );
		rc = LDAP_UNWILLING_TO_PERFORM;
		break;
	       	
	case REWRITE_REGEXEC_ERR:
		if ( fdc.rs ) {
			fdc.rs->sr_err = LDAP_OTHER;
			fdc.rs->sr_text = "Rewrite error";
		}
		op->o_tmpfree( ftmp.bv_val, op->o_tmpmemctx );
		rc = LDAP_OTHER;
		break;
	}

	return rc;
}

/*
 * I don't like this much, but we need two different
 * functions because different heap managers may be
 * in use in back-ldap/meta to reduce the amount of
 * calls to malloc routines, and some of the free()
 * routines may be macros with args
 */
int
rwm_referral_rewrite(
	Operation		*op,
	SlapReply		*rs,
	void			*cookie,
	BerVarray		a_vals,
	BerVarray		*pa_nvals )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			i, last;

	dncookie		dc;
	struct berval		dn = BER_BVNULL,
				ndn = BER_BVNULL;

	assert( a_vals != NULL );

	/*
	 * Rewrite the dn if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = (char *)cookie;

	for ( last = 0; !BER_BVISNULL( &a_vals[last] ); last++ )
		;
	last--;
	
	if ( pa_nvals != NULL ) {
		if ( *pa_nvals == NULL ) {
			*pa_nvals = ch_malloc( ( last + 2 ) * sizeof(struct berval) );
			memset( *pa_nvals, 0, ( last + 2 ) * sizeof(struct berval) );
		}
	}

	for ( i = 0; !BER_BVISNULL( &a_vals[i] ); i++ ) {
		struct berval	olddn = BER_BVNULL,
				oldval;
		int		rc;
		LDAPURLDesc	*ludp;

		oldval = a_vals[i];
		rc = ldap_url_parse( oldval.bv_val, &ludp );
		if ( rc != LDAP_URL_SUCCESS ) {
			/* leave attr untouched if massage failed */
			if ( pa_nvals && BER_BVISNULL( &(*pa_nvals)[i] ) ) {
				ber_dupbv( &(*pa_nvals)[i], &oldval );
			}
			continue;
		}

		/* FIXME: URLs like "ldap:///dc=suffix" if passed
		 * thru ldap_url_parse() and ldap_url_desc2str() 
		 * get rewritten as "ldap:///dc=suffix??base";
		 * we don't want this to occur... */
		if ( ludp->lud_scope == LDAP_SCOPE_BASE ) {
			ludp->lud_scope = LDAP_SCOPE_DEFAULT;
		}

		ber_str2bv( ludp->lud_dn, 0, 0, &olddn );

		dn = olddn;
		if ( pa_nvals ) {
			ndn = olddn;
			rc = rwm_dn_massage_pretty_normalize( &dc, &olddn,
					&dn, &ndn );
		} else {
			rc = rwm_dn_massage_pretty( &dc, &olddn, &dn );
		}

		switch ( rc ) {
		case LDAP_UNWILLING_TO_PERFORM:
			/*
			 * FIXME: need to check if it may be considered 
			 * legal to trim values when adding/modifying;
			 * it should be when searching (e.g. ACLs).
			 */
			ch_free( a_vals[i].bv_val );
			if (last > i ) {
				a_vals[i] = a_vals[last];
				if ( pa_nvals ) {
					(*pa_nvals)[i] = (*pa_nvals)[last];
				}
			}
			BER_BVZERO( &a_vals[last] );
			if ( pa_nvals ) {
				BER_BVZERO( &(*pa_nvals)[last] );
			}
			last--;
			break;
		
		case LDAP_SUCCESS:
			if ( !BER_BVISNULL( &dn ) && dn.bv_val != olddn.bv_val ) {
				char	*newurl;

				ludp->lud_dn = dn.bv_val;
				newurl = ldap_url_desc2str( ludp );
				ludp->lud_dn = olddn.bv_val;
				ch_free( dn.bv_val );
				if ( newurl == NULL ) {
					/* FIXME: leave attr untouched
					 * even if ldap_url_desc2str failed...
					 */
					break;
				}

				ber_str2bv( newurl, 0, 1, &a_vals[i] );
				ber_memfree( newurl );

				if ( pa_nvals ) {
					ludp->lud_dn = ndn.bv_val;
					newurl = ldap_url_desc2str( ludp );
					ludp->lud_dn = olddn.bv_val;
					ch_free( ndn.bv_val );
					if ( newurl == NULL ) {
						/* FIXME: leave attr untouched
						 * even if ldap_url_desc2str failed...
						 */
						ch_free( a_vals[i].bv_val );
						a_vals[i] = oldval;
						break;
					}

					if ( !BER_BVISNULL( &(*pa_nvals)[i] ) ) {
						ch_free( (*pa_nvals)[i].bv_val );
					}
					ber_str2bv( newurl, 0, 1, &(*pa_nvals)[i] );
					ber_memfree( newurl );
				}

				ch_free( oldval.bv_val );
				ludp->lud_dn = olddn.bv_val;
			}
			break;

		default:
			/* leave attr untouched if massage failed */
			if ( pa_nvals && BER_BVISNULL( &(*pa_nvals)[i] ) ) {
				ber_dupbv( &(*pa_nvals)[i], &a_vals[i] );
			}
			break;
		}
		ldap_free_urldesc( ludp );
	}
	
	return 0;
}

/*
 * I don't like this much, but we need two different
 * functions because different heap managers may be
 * in use in back-ldap/meta to reduce the amount of
 * calls to malloc routines, and some of the free()
 * routines may be macros with args
 */
int
rwm_dnattr_rewrite(
	Operation		*op,
	SlapReply		*rs,
	void			*cookie,
	BerVarray		a_vals,
	BerVarray		*pa_nvals )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			i, last;

	dncookie		dc;
	struct berval		dn = BER_BVNULL,
				ndn = BER_BVNULL;
	BerVarray		in;

	if ( a_vals ) {
		in = a_vals;

	} else {
		if ( pa_nvals == NULL || *pa_nvals == NULL ) {
			return LDAP_OTHER;
		}
		in = *pa_nvals;
	}

	/*
	 * Rewrite the dn if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = (char *)cookie;

	for ( last = 0; !BER_BVISNULL( &in[last] ); last++ );
	last--;
	if ( pa_nvals != NULL ) {
		if ( *pa_nvals == NULL ) {
			*pa_nvals = ch_malloc( ( last + 2 ) * sizeof(struct berval) );
			memset( *pa_nvals, 0, ( last + 2 ) * sizeof(struct berval) );
		}
	}

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ ) {
		int		rc;

		if ( a_vals ) {
			dn = in[i];
			if ( pa_nvals ) {
				ndn = (*pa_nvals)[i];
				rc = rwm_dn_massage_pretty_normalize( &dc, &in[i], &dn, &ndn );
			} else {
				rc = rwm_dn_massage_pretty( &dc, &in[i], &dn );
			}
		} else {
			ndn = in[i];
			rc = rwm_dn_massage_normalize( &dc, &in[i], &ndn );
		}

		switch ( rc ) {
		case LDAP_UNWILLING_TO_PERFORM:
			/*
			 * FIXME: need to check if it may be considered 
			 * legal to trim values when adding/modifying;
			 * it should be when searching (e.g. ACLs).
			 */
			ch_free( in[i].bv_val );
			if (last > i ) {
				in[i] = in[last];
				if ( a_vals && pa_nvals ) {
					(*pa_nvals)[i] = (*pa_nvals)[last];
				}
			}
			BER_BVZERO( &in[last] );
			if ( a_vals && pa_nvals ) {
				BER_BVZERO( &(*pa_nvals)[last] );
			}
			last--;
			break;
		
		case LDAP_SUCCESS:
			if ( a_vals ) {
				if ( !BER_BVISNULL( &dn ) && dn.bv_val != a_vals[i].bv_val ) {
					ch_free( a_vals[i].bv_val );
					a_vals[i] = dn;

					if ( pa_nvals ) {
						if ( !BER_BVISNULL( &(*pa_nvals)[i] ) ) {
							ch_free( (*pa_nvals)[i].bv_val );
						}
						(*pa_nvals)[i] = ndn;
					}
				}
				
			} else {
				if ( !BER_BVISNULL( &ndn ) && ndn.bv_val != (*pa_nvals)[i].bv_val ) {
					ch_free( (*pa_nvals)[i].bv_val );
					(*pa_nvals)[i] = ndn;
				}
			}
			break;

		default:
			/* leave attr untouched if massage failed */
			if ( a_vals && pa_nvals && BER_BVISNULL( &(*pa_nvals)[i] ) ) {
				dnNormalize( 0, NULL, NULL, &a_vals[i], &(*pa_nvals)[i], NULL );
			}
			break;
		}
	}
	
	return 0;
}

int
rwm_referral_result_rewrite(
	dncookie		*dc,
	BerVarray		a_vals )
{
	int		i, last;

	for ( last = 0; !BER_BVISNULL( &a_vals[last] ); last++ );
	last--;

	for ( i = 0; !BER_BVISNULL( &a_vals[i] ); i++ ) {
		struct berval	dn,
				olddn = BER_BVNULL;
		int		rc;
		LDAPURLDesc	*ludp;

		rc = ldap_url_parse( a_vals[i].bv_val, &ludp );
		if ( rc != LDAP_URL_SUCCESS ) {
			/* leave attr untouched if massage failed */
			continue;
		}

		/* FIXME: URLs like "ldap:///dc=suffix" if passed
		 * thru ldap_url_parse() and ldap_url_desc2str()
		 * get rewritten as "ldap:///dc=suffix??base";
		 * we don't want this to occur... */
		if ( ludp->lud_scope == LDAP_SCOPE_BASE ) {
			ludp->lud_scope = LDAP_SCOPE_DEFAULT;
		}

		ber_str2bv( ludp->lud_dn, 0, 0, &olddn );

		dn = olddn;
		rc = rwm_dn_massage_pretty( dc, &olddn, &dn );
		switch ( rc ) {
		case LDAP_UNWILLING_TO_PERFORM:
			/*
			 * FIXME: need to check if it may be considered 
			 * legal to trim values when adding/modifying;
			 * it should be when searching (e.g. ACLs).
			 */
			ch_free( a_vals[i].bv_val );
			if ( last > i ) {
				a_vals[i] = a_vals[last];
			}
			BER_BVZERO( &a_vals[last] );
			last--;
			i--;
			break;

		default:
			/* leave attr untouched if massage failed */
			if ( !BER_BVISNULL( &dn ) && olddn.bv_val != dn.bv_val ) {
				char	*newurl;

				ludp->lud_dn = dn.bv_val;
				newurl = ldap_url_desc2str( ludp );
				if ( newurl == NULL ) {
					/* FIXME: leave attr untouched
					 * even if ldap_url_desc2str failed...
					 */
					break;
				}

				ch_free( a_vals[i].bv_val );
				ber_str2bv( newurl, 0, 1, &a_vals[i] );
				ber_memfree( newurl );
				ludp->lud_dn = olddn.bv_val;
			}
			break;
		}

		ldap_free_urldesc( ludp );
	}

	return 0;
}

int
rwm_dnattr_result_rewrite(
	dncookie		*dc,
	BerVarray		a_vals,
	BerVarray		a_nvals )
{
	int		i, last;

	for ( last = 0; !BER_BVISNULL( &a_vals[last] ); last++ );
	last--;

	for ( i = 0; !BER_BVISNULL( &a_vals[i] ); i++ ) {
		struct berval	pdn, ndn = BER_BVNULL;
		int		rc;
		
		pdn = a_vals[i];
		rc = rwm_dn_massage_pretty_normalize( dc, &a_vals[i], &pdn, &ndn );
		switch ( rc ) {
		case LDAP_UNWILLING_TO_PERFORM:
			/*
			 * FIXME: need to check if it may be considered 
			 * legal to trim values when adding/modifying;
			 * it should be when searching (e.g. ACLs).
			 */
			assert( a_vals[i].bv_val != a_nvals[i].bv_val );
			ch_free( a_vals[i].bv_val );
			ch_free( a_nvals[i].bv_val );
			if ( last > i ) {
				a_vals[i] = a_vals[last];
				a_nvals[i] = a_nvals[last];
			}
			BER_BVZERO( &a_vals[last] );
			BER_BVZERO( &a_nvals[last] );
			last--;
			break;

		default:
			/* leave attr untouched if massage failed */
			if ( !BER_BVISNULL( &pdn ) && a_vals[i].bv_val != pdn.bv_val ) {
				ch_free( a_vals[i].bv_val );
				a_vals[i] = pdn;
			}
			if ( !BER_BVISNULL( &ndn ) && a_nvals[i].bv_val != ndn.bv_val ) {
				ch_free( a_nvals[i].bv_val );
				a_nvals[i] = ndn;
			}
			break;
		}
	}

	return 0;
}

void
rwm_mapping_dst_free( void *v_mapping )
{
	struct ldapmapping *mapping = v_mapping;

	if ( BER_BVISEMPTY( &mapping[0].m_dst ) ) {
		rwm_mapping_free( &mapping[ -1 ] );
	}
}

void
rwm_mapping_free( void *v_mapping )
{
	struct ldapmapping *mapping = v_mapping;

	if ( !BER_BVISNULL( &mapping[0].m_src ) ) {
		ch_free( mapping[0].m_src.bv_val );
	}

	if ( mapping[0].m_flags & RWMMAP_F_FREE_SRC ) {
		if ( mapping[0].m_flags & RWMMAP_F_IS_OC ) {
			if ( mapping[0].m_src_oc ) {
				ch_free( mapping[0].m_src_oc );
			}

		} else {
			if ( mapping[0].m_src_ad ) {
				ch_free( mapping[0].m_src_ad );
			}
		}
	}

	if ( !BER_BVISNULL( &mapping[0].m_dst ) ) {
		ch_free( mapping[0].m_dst.bv_val );
	}

	if ( mapping[0].m_flags & RWMMAP_F_FREE_DST ) {
		if ( mapping[0].m_flags & RWMMAP_F_IS_OC ) {
			if ( mapping[0].m_dst_oc ) {
				ch_free( mapping[0].m_dst_oc );
			}

		} else {
			if ( mapping[0].m_dst_ad ) {
				ch_free( mapping[0].m_dst_ad );
			}
		}
	}

	ch_free( mapping );

}

#endif /* SLAPD_OVER_RWM */
