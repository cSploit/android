/* cr.c - content rule routines */
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
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

struct cindexrec {
	struct berval	cir_name;
	ContentRule	*cir_cr;
};

static Avlnode	*cr_index = NULL;
static LDAP_STAILQ_HEAD(CRList, ContentRule) cr_list
	= LDAP_STAILQ_HEAD_INITIALIZER(cr_list);

static int
cr_index_cmp(
    const void	*v_cir1,
    const void	*v_cir2 )
{
	const struct cindexrec	*cir1 = v_cir1;
	const struct cindexrec	*cir2 = v_cir2;
	int i = cir1->cir_name.bv_len - cir2->cir_name.bv_len;
	if (i) return i;
	return strcasecmp( cir1->cir_name.bv_val, cir2->cir_name.bv_val );
}

static int
cr_index_name_cmp(
    const void	*v_name,
    const void	*v_cir )
{
	const struct berval    *name = v_name;
	const struct cindexrec *cir  = v_cir;
	int i = name->bv_len - cir->cir_name.bv_len;
	if (i) return i;
	return strncasecmp( name->bv_val, cir->cir_name.bv_val, name->bv_len );
}

ContentRule *
cr_find( const char *crname )
{
	struct berval bv;

	bv.bv_val = (char *)crname;
	bv.bv_len = strlen( crname );

	return( cr_bvfind( &bv ) );
}

ContentRule *
cr_bvfind( struct berval *crname )
{
	struct cindexrec	*cir;

	cir = avl_find( cr_index, crname, cr_index_name_cmp );

	if ( cir != NULL ) {
		return( cir->cir_cr );
	}

	return( NULL );
}

static int
cr_destroy_one( ContentRule *c )
{
	assert( c != NULL );

	if (c->scr_auxiliaries) ldap_memfree(c->scr_auxiliaries);
	if (c->scr_required) ldap_memfree(c->scr_required);
	if (c->scr_allowed) ldap_memfree(c->scr_allowed);
	if (c->scr_precluded) ldap_memfree(c->scr_precluded);
	ldap_contentrule_free((LDAPContentRule *)c);

	return 0;
}

void
cr_destroy( void )
{
	ContentRule *c;

	avl_free(cr_index, ldap_memfree);

	while( !LDAP_STAILQ_EMPTY(&cr_list) ) {
		c = LDAP_STAILQ_FIRST(&cr_list);
		LDAP_STAILQ_REMOVE_HEAD(&cr_list, scr_next);

		cr_destroy_one( c );
	}
}

static int
cr_insert(
    ContentRule		*scr,
    const char		**err
)
{
	struct cindexrec	*cir;
	char			**names;

	assert( scr != NULL );

	if ( scr->scr_oid ) {
		cir = (struct cindexrec *)
			ch_calloc( 1, sizeof(struct cindexrec) );
		cir->cir_name.bv_val = scr->scr_oid;
		cir->cir_name.bv_len = strlen( scr->scr_oid );
		cir->cir_cr = scr;

		if ( avl_insert( &cr_index, (caddr_t) cir,
		                 cr_index_cmp, avl_dup_error ) )
		{
			*err = scr->scr_oid;
			ldap_memfree(cir);
			return SLAP_SCHERR_CR_DUP;
		}

		/* FIX: temporal consistency check */
		assert( cr_bvfind(&cir->cir_name) != NULL );
	}

	if ( (names = scr->scr_names) ) {
		while ( *names ) {
			cir = (struct cindexrec *)
				ch_calloc( 1, sizeof(struct cindexrec) );
			cir->cir_name.bv_val = *names;
			cir->cir_name.bv_len = strlen( *names );
			cir->cir_cr = scr;

			if ( avl_insert( &cr_index, (caddr_t) cir,
			                 cr_index_cmp, avl_dup_error ) )
			{
				*err = *names;
				ldap_memfree(cir);
				return SLAP_SCHERR_CR_DUP;
			}

			/* FIX: temporal consistency check */
			assert( cr_bvfind(&cir->cir_name) != NULL );

			names++;
		}
	}

	LDAP_STAILQ_INSERT_TAIL(&cr_list, scr, scr_next);

	return 0;
}

static int
cr_add_auxiliaries(
    ContentRule		*scr,
	int			*op,
    const char		**err )
{
	int naux;

	if( scr->scr_oc_oids_aux == NULL ) return 0;
	
	for( naux=0; scr->scr_oc_oids_aux[naux]; naux++ ) {
		/* count them */ ;
	}

	scr->scr_auxiliaries = ch_calloc( naux+1, sizeof(ObjectClass *));

	for( naux=0; scr->scr_oc_oids_aux[naux]; naux++ ) {
		ObjectClass *soc = scr->scr_auxiliaries[naux]
			= oc_find(scr->scr_oc_oids_aux[naux]);
		if ( !soc ) {
			*err = scr->scr_oc_oids_aux[naux];
			return SLAP_SCHERR_CLASS_NOT_FOUND;
		}

		if( soc->soc_flags & SLAP_OC_OPERATIONAL &&
			soc != slap_schema.si_oc_extensibleObject )
		{
			(*op)++;
		}

		if( soc->soc_kind != LDAP_SCHEMA_AUXILIARY ) {
			*err = scr->scr_oc_oids_aux[naux];
			return SLAP_SCHERR_CR_BAD_AUX;
		}
	}

	scr->scr_auxiliaries[naux] = NULL;
	return 0;
}

static int
cr_create_required(
    ContentRule		*scr,
	int			*op,
    const char		**err )
{
    char		**attrs = scr->scr_at_oids_must;
	char		**attrs1;
	AttributeType	*sat;

	if ( attrs ) {
		attrs1 = attrs;
		while ( *attrs1 ) {
			sat = at_find(*attrs1);
			if ( !sat ) {
				*err = *attrs1;
				return SLAP_SCHERR_ATTR_NOT_FOUND;
			}

			if( is_at_operational( sat )) (*op)++;

			if ( at_find_in_list(sat, scr->scr_required) < 0) {
				if ( at_append_to_list(sat, &scr->scr_required) ) {
					*err = *attrs1;
					return SLAP_SCHERR_OUTOFMEM;
				}
			} else {
				*err = *attrs1;
				return SLAP_SCHERR_CR_BAD_AT;
			}
			attrs1++;
		}
	}
	return 0;
}

static int
cr_create_allowed(
    ContentRule		*scr,
	int			*op,
    const char		**err )
{
    char		**attrs = scr->scr_at_oids_may;
	char		**attrs1;
	AttributeType	*sat;

	if ( attrs ) {
		attrs1 = attrs;
		while ( *attrs1 ) {
			sat = at_find(*attrs1);
			if ( !sat ) {
				*err = *attrs1;
				return SLAP_SCHERR_ATTR_NOT_FOUND;
			}

			if( is_at_operational( sat )) (*op)++;

			if ( at_find_in_list(sat, scr->scr_required) < 0 &&
				at_find_in_list(sat, scr->scr_allowed) < 0 )
			{
				if ( at_append_to_list(sat, &scr->scr_allowed) ) {
					*err = *attrs1;
					return SLAP_SCHERR_OUTOFMEM;
				}
			} else {
				*err = *attrs1;
				return SLAP_SCHERR_CR_BAD_AT;
			}
			attrs1++;
		}
	}
	return 0;
}

static int
cr_create_precluded(
    ContentRule		*scr,
	int			*op,
    const char		**err )
{
    char		**attrs = scr->scr_at_oids_not;
	char		**attrs1;
	AttributeType	*sat;

	if ( attrs ) {
		attrs1 = attrs;
		while ( *attrs1 ) {
			sat = at_find(*attrs1);
			if ( !sat ) {
				*err = *attrs1;
				return SLAP_SCHERR_ATTR_NOT_FOUND;
			}

			if( is_at_operational( sat )) (*op)++;

			/* FIXME: should also make sure attribute type is not
				a required attribute of the structural class or
				any auxiliary class */
			if ( at_find_in_list(sat, scr->scr_required) < 0 &&
				at_find_in_list(sat, scr->scr_allowed) < 0 &&
				at_find_in_list(sat, scr->scr_precluded) < 0 )
			{
				if ( at_append_to_list(sat, &scr->scr_precluded) ) {
					*err = *attrs1;
					return SLAP_SCHERR_OUTOFMEM;
				}
			} else {
				*err = *attrs1;
				return SLAP_SCHERR_CR_BAD_AT;
			}
			attrs1++;
		}
	}
	return 0;
}

int
cr_add(
    LDAPContentRule	*cr,
	int user,
	ContentRule **rscr,
    const char		**err
)
{
	ContentRule	*scr;
	int		code;
	int		op = 0;
	char	*oidm = NULL;

	if ( cr->cr_names != NULL ) {
		int i;

		for( i=0; cr->cr_names[i]; i++ ) {
			if( !slap_valid_descr( cr->cr_names[i] ) ) {
				return SLAP_SCHERR_BAD_DESCR;
			}
		}
	}

	if ( !OID_LEADCHAR( cr->cr_oid[0] )) {
		/* Expand OID macros */
		char *oid = oidm_find( cr->cr_oid );
		if ( !oid ) {
			*err = cr->cr_oid;
			return SLAP_SCHERR_OIDM;
		}
		if ( oid != cr->cr_oid ) {
			oidm = cr->cr_oid;
			cr->cr_oid = oid;
		}
	}

	scr = (ContentRule *) ch_calloc( 1, sizeof(ContentRule) );
	AC_MEMCPY( &scr->scr_crule, cr, sizeof(LDAPContentRule) );

	scr->scr_oidmacro = oidm;
	scr->scr_sclass = oc_find(cr->cr_oid);
	if ( !scr->scr_sclass ) {
		*err = cr->cr_oid;
		code = SLAP_SCHERR_CLASS_NOT_FOUND;
		goto fail;
	}

	/* check object class usage */
	if( scr->scr_sclass->soc_kind != LDAP_SCHEMA_STRUCTURAL )
	{
		*err = cr->cr_oid;
		code = SLAP_SCHERR_CR_BAD_STRUCT;
		goto fail;
	}

	if( scr->scr_sclass->soc_flags & SLAP_OC_OPERATIONAL ) op++;

	code = cr_add_auxiliaries( scr, &op, err );
	if ( code != 0 ) goto fail;

	code = cr_create_required( scr, &op, err );
	if ( code != 0 ) goto fail;

	code = cr_create_allowed( scr, &op, err );
	if ( code != 0 ) goto fail;

	code = cr_create_precluded( scr, &op, err );
	if ( code != 0 ) goto fail;

	if( user && op ) {
		code = SLAP_SCHERR_CR_BAD_AUX;
		goto fail;
	}

	code = cr_insert(scr,err);
	if ( code == 0 && rscr )
		*rscr = scr;
	return code;
fail:
	ch_free( scr );
	return code;
}

void
cr_unparse( BerVarray *res, ContentRule *start, ContentRule *end, int sys )
{
	ContentRule *cr;
	int i, num;
	struct berval bv, *bva = NULL, idx;
	char ibuf[32];

	if ( !start )
		start = LDAP_STAILQ_FIRST( &cr_list );

	/* count the result size */
	i = 0;
	for ( cr=start; cr; cr=LDAP_STAILQ_NEXT(cr, scr_next)) {
		if ( sys && !(cr->scr_flags & SLAP_CR_HARDCODE)) continue;
		i++;
		if ( cr == end ) break;
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
	for ( cr=start; cr; cr=LDAP_STAILQ_NEXT(cr, scr_next)) {
		LDAPContentRule lcr, *lcrp;
		if ( sys && !(cr->scr_flags & SLAP_CR_HARDCODE)) continue;
		if ( cr->scr_oidmacro ) {
			lcr = cr->scr_crule;
			lcr.cr_oid = cr->scr_oidmacro;
			lcrp = &lcr;
		} else {
			lcrp = &cr->scr_crule;
		}
		if ( ldap_contentrule2bv( lcrp, &bv ) == NULL ) {
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
		if ( cr == end ) break;
	}
	*res = bva;
}

int
cr_schema_info( Entry *e )
{
	AttributeDescription *ad_ditContentRules
		= slap_schema.si_ad_ditContentRules;
	ContentRule	*cr;

	struct berval	val;
	struct berval	nval;

	LDAP_STAILQ_FOREACH(cr, &cr_list, scr_next) {
		if ( ldap_contentrule2bv( &cr->scr_crule, &val ) == NULL ) {
			return -1;
		}

#if 0
		if( cr->scr_flags & SLAP_CR_HIDE ) continue;
#endif
#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging cr [%ld] %s\n",
	       (long) val.bv_len, val.bv_val, 0 );
#endif

		nval.bv_val = cr->scr_oid;
		nval.bv_len = strlen(cr->scr_oid);

		if( attr_merge_one( e, ad_ditContentRules, &val, &nval ) )
		{
			return -1;
		}
		ldap_memfree( val.bv_val );
	}
	return 0;
}
