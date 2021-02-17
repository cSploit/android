/* schema_check.c - routines to enforce schema definitions */
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

static char * oc_check_required(
	Entry *e,
	ObjectClass *oc,
	struct berval *ocname );

static int entry_naming_check(
	Entry *e,
	int manage,
	int add_naming,
	const char** text,
	char *textbuf, size_t textlen );
/*
 * entry_schema_check - check that entry e conforms to the schema required
 * by its object class(es).
 *
 * returns 0 if so, non-zero otherwise.
 */

int
entry_schema_check( 
	Operation *op,
	Entry *e,
	Attribute *oldattrs,
	int manage,
	int add,
	Attribute **socp,
	const char** text,
	char *textbuf, size_t textlen )
{
	Attribute	*a, *asc = NULL, *aoc = NULL;
	ObjectClass *sc, *oc, **socs = NULL;
	AttributeType *at;
	ContentRule *cr;
	int	rc, i;
	AttributeDescription *ad_structuralObjectClass
		= slap_schema.si_ad_structuralObjectClass;
	AttributeDescription *ad_objectClass
		= slap_schema.si_ad_objectClass;
	int extensible = 0;
	int subentry = is_entry_subentry( e );
	int collectiveSubentry = 0;

	if ( SLAP_NO_SCHEMA_CHECK( op->o_bd )) {
		return LDAP_SUCCESS;
	}

	if ( get_no_schema_check( op ) ) {
		return LDAP_SUCCESS;
	}

	if( subentry ) {
		collectiveSubentry = is_entry_collectiveAttributeSubentry( e );
	}

	*text = textbuf;

	/* misc attribute checks */
	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
		const char *type = a->a_desc->ad_cname.bv_val;

		/* there should be at least one value */
		assert( a->a_vals != NULL );
		assert( a->a_vals[0].bv_val != NULL ); 

		if( a->a_desc->ad_type->sat_check ) {
			rc = (a->a_desc->ad_type->sat_check)(
				op->o_bd, e, a, text, textbuf, textlen );
			if( rc != LDAP_SUCCESS ) {
				return rc;
			}
		}

		if( a->a_desc == ad_structuralObjectClass )
			asc = a;
		else if ( a->a_desc == ad_objectClass )
			aoc = a;

		if( !collectiveSubentry && is_at_collective( a->a_desc->ad_type ) ) {
			snprintf( textbuf, textlen,
				"'%s' can only appear in collectiveAttributeSubentry",
				type );
			return LDAP_OBJECT_CLASS_VIOLATION;
		}

		/* if single value type, check for multiple values */
		if( is_at_single_value( a->a_desc->ad_type ) &&
			a->a_vals[1].bv_val != NULL )
		{
			snprintf( textbuf, textlen, 
				"attribute '%s' cannot have multiple values",
				type );

			Debug( LDAP_DEBUG_ANY,
			    "Entry (%s), %s\n",
			    e->e_dn, textbuf, 0 );

			return LDAP_CONSTRAINT_VIOLATION;
		}
	}

	/* check the object class attribute */
	if ( aoc == NULL ) {
		Debug( LDAP_DEBUG_ANY, "No objectClass for entry (%s)\n",
		    e->e_dn, 0, 0 );

		*text = "no objectClass attribute";
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	assert( aoc->a_vals != NULL );
	assert( aoc->a_vals[0].bv_val != NULL );

	/* check the structural object class attribute */
	if ( asc == NULL && !add ) {
		Debug( LDAP_DEBUG_ANY,
			"No structuralObjectClass for entry (%s)\n",
		    e->e_dn, 0, 0 );

		*text = "no structuralObjectClass operational attribute";
		return LDAP_OTHER;
	}

	rc = structural_class( aoc->a_vals, &oc, &socs, text, textbuf, textlen,
		op->o_tmpmemctx );
	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( asc == NULL && add ) {
		attr_merge_one( e, ad_structuralObjectClass, &oc->soc_cname, NULL );
		asc = attr_find( e->e_attrs, ad_structuralObjectClass );
		sc = oc;
		goto got_soc;
	}

	assert( asc->a_vals != NULL );
	assert( asc->a_vals[0].bv_val != NULL );
	assert( asc->a_vals[1].bv_val == NULL );

	sc = oc_bvfind( &asc->a_vals[0] );
	if( sc == NULL ) {
		snprintf( textbuf, textlen, 
			"unrecognized structuralObjectClass '%s'",
			asc->a_vals[0].bv_val );

		Debug( LDAP_DEBUG_ANY,
			"entry_check_schema(%s): %s\n",
			e->e_dn, textbuf, 0 );

		rc = LDAP_OBJECT_CLASS_VIOLATION;
		goto done;
	}

	if( sc->soc_kind != LDAP_SCHEMA_STRUCTURAL ) {
		snprintf( textbuf, textlen, 
			"structuralObjectClass '%s' is not STRUCTURAL",
			asc->a_vals[0].bv_val );

		Debug( LDAP_DEBUG_ANY,
			"entry_check_schema(%s): %s\n",
			e->e_dn, textbuf, 0 );

		rc = LDAP_OTHER;
		goto done;
	}

got_soc:
	if( !manage && sc->soc_obsolete ) {
		snprintf( textbuf, textlen, 
			"structuralObjectClass '%s' is OBSOLETE",
			asc->a_vals[0].bv_val );

		Debug( LDAP_DEBUG_ANY,
			"entry_check_schema(%s): %s\n",
			e->e_dn, textbuf, 0 );

		rc = LDAP_OBJECT_CLASS_VIOLATION;
		goto done;
	}

	*text = textbuf;

	if ( oc == NULL ) {
		snprintf( textbuf, textlen, 
			"unrecognized objectClass '%s'",
			aoc->a_vals[0].bv_val );
		rc = LDAP_OBJECT_CLASS_VIOLATION;
		goto done;

	} else if ( sc != oc ) {
		if ( !manage && sc != slap_schema.si_oc_glue ) {
			snprintf( textbuf, textlen, 
				"structural object class modification "
				"from '%s' to '%s' not allowed",
				asc->a_vals[0].bv_val, oc->soc_cname.bv_val );
			rc = LDAP_NO_OBJECT_CLASS_MODS;
			goto done;
		}

		assert( asc->a_vals != NULL );
		assert( !BER_BVISNULL( &asc->a_vals[0] ) );
		assert( BER_BVISNULL( &asc->a_vals[1] ) );
		assert( asc->a_nvals == asc->a_vals );

		/* draft-zeilenga-ldap-relax: automatically modify
		 * structuralObjectClass if changed with relax */
		sc = oc;
		ber_bvreplace( &asc->a_vals[ 0 ], &sc->soc_cname );
		if ( socp ) {
			*socp = asc;
		}
	}

	/* naming check */
	if ( !is_entry_glue ( e ) ) {
		rc = entry_naming_check( e, manage, add, text, textbuf, textlen );
		if( rc != LDAP_SUCCESS ) {
			goto done;
		}
	} else {
		/* Glue Entry */
	}

	/* find the content rule for the structural class */
	cr = cr_find( sc->soc_oid );

	/* the cr must be same as the structural class */
	assert( !cr || !strcmp( cr->scr_oid, sc->soc_oid ) );

	/* check that the entry has required attrs of the content rule */
	if( cr ) {
		if( !manage && cr->scr_obsolete ) {
			snprintf( textbuf, textlen, 
				"content rule '%s' is obsolete",
				ldap_contentrule2name( &cr->scr_crule ));

			Debug( LDAP_DEBUG_ANY,
				"Entry (%s): %s\n",
				e->e_dn, textbuf, 0 );

			rc = LDAP_OBJECT_CLASS_VIOLATION;
			goto done;
		}

		if( cr->scr_required ) for( i=0; cr->scr_required[i]; i++ ) {
			at = cr->scr_required[i];

			for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
				if( a->a_desc->ad_type == at ) {
					break;
				}
			}

			/* not there => schema violation */
			if ( a == NULL ) {
				snprintf( textbuf, textlen, 
					"content rule '%s' requires attribute '%s'",
					ldap_contentrule2name( &cr->scr_crule ),
					at->sat_cname.bv_val );

				Debug( LDAP_DEBUG_ANY,
					"Entry (%s): %s\n",
					e->e_dn, textbuf, 0 );

				rc = LDAP_OBJECT_CLASS_VIOLATION;
				goto done;
			}
		}

		if( cr->scr_precluded ) for( i=0; cr->scr_precluded[i]; i++ ) {
			at = cr->scr_precluded[i];

			for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
				if( a->a_desc->ad_type == at ) {
					break;
				}
			}

			/* there => schema violation */
			if ( a != NULL ) {
				snprintf( textbuf, textlen, 
					"content rule '%s' precluded attribute '%s'",
					ldap_contentrule2name( &cr->scr_crule ),
					at->sat_cname.bv_val );

				Debug( LDAP_DEBUG_ANY,
					"Entry (%s): %s\n",
					e->e_dn, textbuf, 0 );

				rc = LDAP_OBJECT_CLASS_VIOLATION;
				goto done;
			}
		}
	}

	/* check that the entry has required attrs for each oc */
	for ( i = 0; socs[i]; i++ ) {
		oc = socs[i];
		if ( !manage && oc->soc_obsolete ) {
			/* disallow obsolete classes */
			snprintf( textbuf, textlen, 
				"objectClass '%s' is OBSOLETE",
				aoc->a_vals[i].bv_val );

			Debug( LDAP_DEBUG_ANY,
				"entry_check_schema(%s): %s\n",
				e->e_dn, textbuf, 0 );

			rc = LDAP_OBJECT_CLASS_VIOLATION;
			goto done;
		}

		if ( oc->soc_check ) {
			rc = (oc->soc_check)( op->o_bd, e, oc,
				text, textbuf, textlen );
			if( rc != LDAP_SUCCESS ) {
				goto done;
			}
		}

		if ( oc->soc_kind == LDAP_SCHEMA_ABSTRACT ) {
			/* object class is abstract */
			if ( oc != slap_schema.si_oc_top &&
				!is_object_subclass( oc, sc ))
			{
				int j;
				ObjectClass *xc = NULL;
				for( j=0; socs[j]; j++ ) {
					if( i != j ) {
						xc = socs[j];

						/* since we previous check against the
						 * structural object of this entry, the
						 * abstract class must be a (direct or indirect)
						 * superclass of one of the auxiliary classes of
						 * the entry.
						 */
						if ( xc->soc_kind == LDAP_SCHEMA_AUXILIARY &&
							is_object_subclass( oc, xc ) )
						{
							xc = NULL;
							break;
						}
					}
				}

				if( xc != NULL ) {
					snprintf( textbuf, textlen, "instantiation of "
						"abstract objectClass '%s' not allowed",
						aoc->a_vals[i].bv_val );

					Debug( LDAP_DEBUG_ANY,
						"entry_check_schema(%s): %s\n",
						e->e_dn, textbuf, 0 );

					rc = LDAP_OBJECT_CLASS_VIOLATION;
					goto done;
				}
			}

		} else if ( oc->soc_kind != LDAP_SCHEMA_STRUCTURAL || oc == sc ) {
			char *s;

			if( oc->soc_kind == LDAP_SCHEMA_AUXILIARY ) {
				int k;

				if( cr ) {
					int j;

					k = -1;
					if( cr->scr_auxiliaries ) {
						for( j = 0; cr->scr_auxiliaries[j]; j++ ) {
							if( cr->scr_auxiliaries[j] == oc ) {
								k = 0;
								break;
							}
						}
					}
					if ( k ) {
						snprintf( textbuf, textlen, 
							"class '%s' not allowed by content rule '%s'",
							oc->soc_cname.bv_val,
							ldap_contentrule2name( &cr->scr_crule ) );
					}
				} else if ( global_disallows & SLAP_DISALLOW_AUX_WO_CR ) {
					k = -1;
					snprintf( textbuf, textlen, 
						"class '%s' not allowed by any content rule",
						oc->soc_cname.bv_val );
				} else {
					k = 0;	
				}

				if( k == -1 ) {
					Debug( LDAP_DEBUG_ANY,
						"Entry (%s): %s\n",
						e->e_dn, textbuf, 0 );

					rc = LDAP_OBJECT_CLASS_VIOLATION;
					goto done;
				}
			}

			s = oc_check_required( e, oc, &aoc->a_vals[i] );
			if (s != NULL) {
				snprintf( textbuf, textlen, 
					"object class '%s' requires attribute '%s'",
					aoc->a_vals[i].bv_val, s );

				Debug( LDAP_DEBUG_ANY,
					"Entry (%s): %s\n",
					e->e_dn, textbuf, 0 );

				rc = LDAP_OBJECT_CLASS_VIOLATION;
				goto done;
			}

			if( oc == slap_schema.si_oc_extensibleObject ) {
				extensible=1;
			}
		}
	}

	if( extensible ) {
		*text = NULL;
		rc = LDAP_SUCCESS;
		goto done;
	}

	/* check that each attr in the entry is allowed by some oc */
	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
 		rc = LDAP_OBJECT_CLASS_VIOLATION;

		if( cr && cr->scr_required ) {
			for( i=0; cr->scr_required[i]; i++ ) {
				if( cr->scr_required[i] == a->a_desc->ad_type ) {
					rc = LDAP_SUCCESS;
					break;
				}
			}
		}

		if( rc != LDAP_SUCCESS && cr && cr->scr_allowed ) {
			for( i=0; cr->scr_allowed[i]; i++ ) {
				if( cr->scr_allowed[i] == a->a_desc->ad_type ) {
					rc = LDAP_SUCCESS;
					break;
				}
			}
		}

		if( rc != LDAP_SUCCESS ) 
		{
			rc = oc_check_allowed( a->a_desc->ad_type, socs, sc );
		}

		if ( rc != LDAP_SUCCESS ) {
			char *type = a->a_desc->ad_cname.bv_val;

			snprintf( textbuf, textlen, 
				"attribute '%s' not allowed",
				type );

			Debug( LDAP_DEBUG_ANY,
			    "Entry (%s), %s\n",
			    e->e_dn, textbuf, 0 );

			goto done;
		}
	}

	*text = NULL;
done:
	slap_sl_free( socs, op->o_tmpmemctx );
	return rc;
}

static char *
oc_check_required(
	Entry *e,
	ObjectClass *oc,
	struct berval *ocname )
{
	AttributeType	*at;
	int		i;
	Attribute	*a;

	Debug( LDAP_DEBUG_TRACE,
		"oc_check_required entry (%s), objectClass \"%s\"\n",
		e->e_dn, ocname->bv_val, 0 );


	/* check for empty oc_required */
	if(oc->soc_required == NULL) {
		return NULL;
	}

	/* for each required attribute */
	for ( i = 0; oc->soc_required[i] != NULL; i++ ) {
		at = oc->soc_required[i];
		/* see if it's in the entry */
		for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
			if( a->a_desc->ad_type == at ) {
				break;
			}
		}
		/* not there => schema violation */
		if ( a == NULL ) {
			return at->sat_cname.bv_val;
		}
	}

	return( NULL );
}

int oc_check_allowed(
	AttributeType *at,
	ObjectClass **socs,
	ObjectClass *sc )
{
	int		i, j;

	Debug( LDAP_DEBUG_TRACE,
		"oc_check_allowed type \"%s\"\n",
		at->sat_cname.bv_val, 0, 0 );

	/* always allow objectClass attribute */
	if ( strcasecmp( at->sat_cname.bv_val, "objectClass" ) == 0 ) {
		return LDAP_SUCCESS;
	}

	/*
	 * All operational attributions are allowed by schema rules.
	 */
	if( is_at_operational(at) ) {
		return LDAP_SUCCESS;
	}

	/* check to see if its allowed by the structuralObjectClass */
	if( sc ) {
		/* does it require the type? */
		for ( j = 0; sc->soc_required != NULL && 
			sc->soc_required[j] != NULL; j++ )
		{
			if( at == sc->soc_required[j] ) {
				return LDAP_SUCCESS;
			}
		}

		/* does it allow the type? */
		for ( j = 0; sc->soc_allowed != NULL && 
			sc->soc_allowed[j] != NULL; j++ )
		{
			if( at == sc->soc_allowed[j] ) {
				return LDAP_SUCCESS;
			}
		}
	}

	/* check that the type appears as req or opt in at least one oc */
	for ( i = 0; socs[i]; i++ ) {
		/* if we know about the oc */
		ObjectClass	*oc = socs[i];
		/* extensibleObject allows all */
		if ( oc == slap_schema.si_oc_extensibleObject ) {
			return LDAP_SUCCESS;
		}
		if ( oc != NULL && oc->soc_kind != LDAP_SCHEMA_ABSTRACT &&
			( sc == NULL || oc->soc_kind == LDAP_SCHEMA_AUXILIARY ))
		{
			/* does it require the type? */
			for ( j = 0; oc->soc_required != NULL && 
				oc->soc_required[j] != NULL; j++ )
			{
				if( at == oc->soc_required[j] ) {
					return LDAP_SUCCESS;
				}
			}
			/* does it allow the type? */
			for ( j = 0; oc->soc_allowed != NULL && 
				oc->soc_allowed[j] != NULL; j++ )
			{
				if( at == oc->soc_allowed[j] ) {
					return LDAP_SUCCESS;
				}
			}
		}
	}

	/* not allowed by any oc */
	return LDAP_OBJECT_CLASS_VIOLATION;
}

/*
 * Determine the structural object class from a set of OIDs
 */
int structural_class(
	BerVarray ocs,
	ObjectClass **scp,
	ObjectClass ***socsp,
	const char **text,
	char *textbuf, size_t textlen,
	void *ctx )
{
	int i, nocs;
	ObjectClass *oc, **socs;
	ObjectClass *sc = NULL;
	int scn = -1;

	*text = "structural_class: internal error";

	/* count them */
	for( i=0; ocs[i].bv_val; i++ ) ;
	nocs = i;
	
	socs = slap_sl_malloc( (nocs+1) * sizeof(ObjectClass *), ctx );

	for( i=0; ocs[i].bv_val; i++ ) {
		socs[i] = oc_bvfind( &ocs[i] );

		if( socs[i] == NULL ) {
			snprintf( textbuf, textlen,
				"unrecognized objectClass '%s'",
				ocs[i].bv_val );
			*text = textbuf;
			goto fail;
		}
	}
	socs[i] = NULL;

	for( i=0; ocs[i].bv_val; i++ ) {
		oc = socs[i];
		if( oc->soc_kind == LDAP_SCHEMA_STRUCTURAL ) {
			if( sc == NULL || is_object_subclass( sc, oc ) ) {
				sc = oc;
				scn = i;

			} else if ( !is_object_subclass( oc, sc ) ) {
				int j;
				ObjectClass *xc = NULL;

				/* find common superior */
				for( j=i+1; ocs[j].bv_val; j++ ) {
					xc = socs[j];

					if( xc == NULL ) {
						snprintf( textbuf, textlen,
							"unrecognized objectClass '%s'",
							ocs[j].bv_val );
						*text = textbuf;
						goto fail;
					}

					if( xc->soc_kind != LDAP_SCHEMA_STRUCTURAL ) {
						xc = NULL;
						continue;
					}

					if( is_object_subclass( sc, xc ) &&
						is_object_subclass( oc, xc ) )
					{
						/* found common subclass */
						break;
					}

					xc = NULL;
				}

				if( xc == NULL ) {
					/* no common subclass */
					snprintf( textbuf, textlen,
						"invalid structural object class chain (%s/%s)",
						ocs[scn].bv_val, ocs[i].bv_val );
					*text = textbuf;
					goto fail;
				}
			}
		}
	}

	if( scp ) {
		*scp = sc;
	}

	if( sc == NULL ) {
		*text = "no structural object class provided";
		goto fail;
	}

	if( scn < 0 ) {
		*text = "invalid structural object class";
		goto fail;
	}

	if ( socsp ) {
		*socsp = socs;
	} else {
		slap_sl_free( socs, ctx );
	}
	*text = NULL;

	return LDAP_SUCCESS;

fail:
	slap_sl_free( socs, ctx );
	return LDAP_OBJECT_CLASS_VIOLATION;
}

/*
 * Return structural object class from list of modifications
 */
int mods_structural_class(
	Modifications *mods,
	struct berval *sc,
	const char **text,
	char *textbuf, size_t textlen, void *ctx )
{
	Modifications *ocmod = NULL;
	ObjectClass *ssc;
	int rc;

	for( ; mods != NULL; mods = mods->sml_next ) {
		if( mods->sml_desc == slap_schema.si_ad_objectClass ) {
			if( ocmod != NULL ) {
				*text = "entry has multiple objectClass attributes";
				return LDAP_OBJECT_CLASS_VIOLATION;
			}
			ocmod = mods;
		}
	}

	if( ocmod == NULL ) {
		*text = "entry has no objectClass attribute";
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( ocmod->sml_values == NULL || ocmod->sml_values[0].bv_val == NULL ) {
		*text = "objectClass attribute has no values";
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	rc = structural_class( ocmod->sml_values, &ssc, NULL,
		text, textbuf, textlen, ctx );
	if ( rc == LDAP_SUCCESS )
		*sc = ssc->soc_cname;
	return rc;
}


static int
entry_naming_check(
	Entry *e,
	int manage,
	int add_naming,
	const char** text,
	char *textbuf, size_t textlen )
{
	/* naming check */
	LDAPRDN		rdn = NULL;
	const char	*p = NULL;
	ber_len_t	cnt;
	int		rc = LDAP_SUCCESS;

	if ( BER_BVISEMPTY( &e->e_name )) {
		return LDAP_SUCCESS;
	}

	/*
	 * Get attribute type(s) and attribute value(s) of our RDN
	 */
	if ( ldap_bv2rdn( &e->e_name, &rdn, (char **)&p,
		LDAP_DN_FORMAT_LDAP ) )
	{
		*text = "unrecognized attribute type(s) in RDN";
		return LDAP_INVALID_DN_SYNTAX;
	}

	/* Check that each AVA of the RDN is present in the entry */
	/* FIXME: Should also check that each AVA lists a distinct type */
	for ( cnt = 0; rdn[cnt]; cnt++ ) {
		LDAPAVA *ava = rdn[cnt];
		AttributeDescription *desc = NULL;
		Attribute *attr;
		const char *errtext;
		int add = 0;

		if( ava->la_flags & LDAP_AVA_BINARY ) {
			snprintf( textbuf, textlen, 
				"value of naming attribute '%s' in unsupported BER form",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
		}

		rc = slap_bv2ad( &ava->la_attr, &desc, &errtext );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( textbuf, textlen, "%s (in RDN)", errtext );
			break;
		}

		if( desc->ad_type->sat_usage ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' is operational",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
			break;
		}
 
		if( desc->ad_type->sat_collective ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' is collective",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
			break;
		}

		if( !manage && desc->ad_type->sat_obsolete ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' is obsolete",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
			break;
		}

		if( !desc->ad_type->sat_equality ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' has no equality matching rule",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
			break;
		}

		if( !desc->ad_type->sat_equality->smr_match ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' has unsupported equality matching rule",
				ava->la_attr.bv_val );
			rc = LDAP_NAMING_VIOLATION;
			break;
		}

		/* find the naming attribute */
		attr = attr_find( e->e_attrs, desc );
		if ( attr == NULL ) {
			snprintf( textbuf, textlen, 
				"naming attribute '%s' is not present in entry",
				ava->la_attr.bv_val );
			if ( add_naming ) {
				add = 1;

			} else {
				rc = LDAP_NAMING_VIOLATION;
			}

		} else {
			rc = attr_valfind( attr, SLAP_MR_VALUE_OF_ASSERTION_SYNTAX|
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH,
				&ava->la_value, NULL, NULL );

			if ( rc != 0 ) {
				switch( rc ) {
				case LDAP_INAPPROPRIATE_MATCHING:
					snprintf( textbuf, textlen, 
						"inappropriate matching for naming attribute '%s'",
						ava->la_attr.bv_val );
					break;
				case LDAP_INVALID_SYNTAX:
					snprintf( textbuf, textlen, 
						"value of naming attribute '%s' is invalid",
						ava->la_attr.bv_val );
					break;
				case LDAP_NO_SUCH_ATTRIBUTE:
					if ( add_naming ) {
						if ( is_at_single_value( desc->ad_type ) ) {
							snprintf( textbuf, textlen, 
								"value of single-valued naming attribute '%s' conflicts with value present in entry",
								ava->la_attr.bv_val );

						} else {
							add = 1;
							rc = LDAP_SUCCESS;
						}

					} else {
						snprintf( textbuf, textlen, 
							"value of naming attribute '%s' is not present in entry",
							ava->la_attr.bv_val );
					}
					break;
				default:
					snprintf( textbuf, textlen, 
						"naming attribute '%s' is inappropriate",
						ava->la_attr.bv_val );
				}

				if ( !add ) {
					rc = LDAP_NAMING_VIOLATION;
				}
			}
		}

		if ( add ) {
			attr_merge_normalize_one( e, desc, &ava->la_value, NULL );

		} else if ( rc != LDAP_SUCCESS ) {
			break;
		}
	}

	ldap_rdnfree( rdn );
	return rc;
}

