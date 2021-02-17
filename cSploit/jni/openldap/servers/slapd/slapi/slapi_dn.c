/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Luke Howard for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <ac/string.h>
#include <ac/stdarg.h>
#include <ac/ctype.h>
#include <ac/unistd.h>
#include <ldap_pvt.h>

#include <slap.h>
#include <slapi.h>

#ifdef LDAP_SLAPI
#define FLAG_DN 0x1
#define FLAG_NDN 0x2

void slapi_sdn_init( Slapi_DN *sdn )
{
	sdn->flag = 0;
	BER_BVZERO( &sdn->dn );
	BER_BVZERO( &sdn->ndn );
}

Slapi_DN *slapi_sdn_new( void )
{
	Slapi_DN *sdn;

	sdn = (Slapi_DN *)slapi_ch_malloc( sizeof(*sdn ));
	slapi_sdn_init( sdn );

	return sdn;
}

void slapi_sdn_done( Slapi_DN *sdn )
{
	if ( sdn == NULL )
		return;

	if ( sdn->flag & FLAG_DN ) {
		slapi_ch_free_string( &sdn->dn.bv_val );
	}
	if ( sdn->flag & FLAG_NDN ) {
		slapi_ch_free_string( &sdn->ndn.bv_val );
	}

	slapi_sdn_init( sdn );
}

void slapi_sdn_free( Slapi_DN **sdn )
{
	slapi_sdn_done( *sdn );
	slapi_ch_free( (void **)sdn );
}

const char *slapi_sdn_get_dn( const Slapi_DN *sdn )
{
	if ( !BER_BVISNULL( &sdn->dn ) )
		return sdn->dn.bv_val;
	else
		return sdn->ndn.bv_val;
}

const char *slapi_sdn_get_ndn( const Slapi_DN *sdn )
{
	if ( BER_BVISNULL( &sdn->ndn ) ) {
		dnNormalize( 0, NULL, NULL,
			(struct berval *)&sdn->dn, (struct berval *)&sdn->ndn, NULL );
		((Slapi_DN *)sdn)->flag |= FLAG_NDN;
	}

	return sdn->ndn.bv_val;
}

Slapi_DN *slapi_sdn_new_dn_byval( const char *dn )
{
	Slapi_DN *sdn;

	sdn = slapi_sdn_new();
	return slapi_sdn_set_dn_byval( sdn, dn );
}

Slapi_DN *slapi_sdn_new_ndn_byval( const char *ndn )
{
	Slapi_DN *sdn;

	sdn = slapi_sdn_new();
	return slapi_sdn_set_ndn_byval( sdn, ndn );
}

Slapi_DN *slapi_sdn_new_dn_byref( const char *dn )
{
	Slapi_DN *sdn;

	sdn = slapi_sdn_new();
	return slapi_sdn_set_dn_byref( sdn, dn );
}

Slapi_DN *slapi_sdn_new_ndn_byref( const char *ndn )
{
	Slapi_DN *sdn;

	sdn = slapi_sdn_new();
	return slapi_sdn_set_ndn_byref( sdn, ndn );
}

Slapi_DN *slapi_sdn_new_dn_passin( const char *dn )
{
	Slapi_DN *sdn;

	sdn = slapi_sdn_new();
	return slapi_sdn_set_dn_passin( sdn, dn );
}

Slapi_DN *slapi_sdn_set_dn_byval( Slapi_DN *sdn, const char *dn )
{
	if ( sdn == NULL ) {
		return NULL;
	}

	slapi_sdn_done( sdn );
	if ( dn != NULL ) {
		sdn->dn.bv_val = slapi_ch_strdup( dn );
		sdn->dn.bv_len = strlen( dn );
	}
	sdn->flag |= FLAG_DN;

	return sdn;
}

Slapi_DN *slapi_sdn_set_dn_byref( Slapi_DN *sdn, const char *dn )
{
	if ( sdn == NULL )
		return NULL;

	slapi_sdn_done( sdn );
	if ( dn != NULL ) {
		sdn->dn.bv_val = (char *)dn;
		sdn->dn.bv_len = strlen( dn );
	}

	return sdn;
}

Slapi_DN *slapi_sdn_set_dn_passin( Slapi_DN *sdn, const char *dn )
{
	if ( sdn == NULL )
		return NULL;

	slapi_sdn_set_dn_byref( sdn, dn );
	sdn->flag |= FLAG_DN;

	return sdn;
}

Slapi_DN *slapi_sdn_set_ndn_byval( Slapi_DN *sdn, const char *ndn )
{
	if ( sdn == NULL ) {
		return NULL;
	}

	slapi_sdn_done( sdn );
	if ( ndn != NULL ) {
		sdn->ndn.bv_val = slapi_ch_strdup( ndn );
		sdn->ndn.bv_len = strlen( ndn );
	}
	sdn->flag |= FLAG_NDN;

	return sdn;
}

Slapi_DN *slapi_sdn_set_ndn_byref( Slapi_DN *sdn, const char *ndn )
{
	if ( sdn == NULL )
		return NULL;

	slapi_sdn_done( sdn );
	if ( ndn != NULL ) {
		sdn->ndn.bv_val = (char *)ndn;
		sdn->ndn.bv_len = strlen( ndn );
	}

	return sdn;
}

Slapi_DN *slapi_sdn_set_ndn_passin( Slapi_DN *sdn, const char *ndn )
{
	if ( sdn == NULL )
		return NULL;

	slapi_sdn_set_ndn_byref( sdn, ndn );
	sdn->flag |= FLAG_NDN;

	return sdn;
}

void slapi_sdn_get_parent( const Slapi_DN *sdn, Slapi_DN *sdn_parent )
{
	struct berval parent_dn;

	if ( !(sdn->flag & FLAG_DN) ) {
		dnParent( (struct berval *)&sdn->ndn, &parent_dn );
		slapi_sdn_set_ndn_byval( sdn_parent, parent_dn.bv_val );
	} else {
		dnParent( (struct berval *)&sdn->dn, &parent_dn );
		slapi_sdn_set_dn_byval( sdn_parent, parent_dn.bv_val );
	}
}

void slapi_sdn_get_backend_parent( const Slapi_DN *sdn,
	Slapi_DN *sdn_parent,
	const Slapi_Backend *backend )
{
	slapi_sdn_get_ndn( sdn );

	if ( backend == NULL ||
	     be_issuffix( (Slapi_Backend *)backend, (struct berval *)&sdn->ndn ) == 0 ) {
		slapi_sdn_get_parent( sdn, sdn_parent );
	}

}

Slapi_DN * slapi_sdn_dup( const Slapi_DN *sdn )
{
	Slapi_DN *new_sdn;

	new_sdn = slapi_sdn_new();
	slapi_sdn_copy( sdn, new_sdn );

	return new_sdn;
}

void slapi_sdn_copy( const Slapi_DN *from, Slapi_DN *to )
{
	slapi_sdn_set_dn_byval( to, from->dn.bv_val );
}

int slapi_sdn_compare( const Slapi_DN *sdn1, const Slapi_DN *sdn2 )
{
	int match = -1;

	slapi_sdn_get_ndn( sdn1 );
	slapi_sdn_get_ndn( sdn2 );

	dnMatch( &match, 0, slap_schema.si_syn_distinguishedName, NULL,
		(struct berval *)&sdn1->ndn, (void *)&sdn2->ndn );

	return match;
}

int slapi_sdn_isempty( const Slapi_DN *sdn)
{
	return ( BER_BVISEMPTY( &sdn->dn ) && BER_BVISEMPTY( &sdn->ndn ) );
}

int slapi_sdn_issuffix( const Slapi_DN *sdn, const Slapi_DN *suffix_sdn )
{
	slapi_sdn_get_ndn( sdn );
	slapi_sdn_get_ndn( suffix_sdn );

	return dnIsSuffix( &sdn->ndn, &suffix_sdn->ndn );
}

int slapi_sdn_isparent( const Slapi_DN *parent, const Slapi_DN *child )
{
	Slapi_DN child_parent;

	slapi_sdn_get_ndn( child );

	slapi_sdn_init( &child_parent );
	dnParent( (struct berval *)&child->ndn, &child_parent.ndn );

	return ( slapi_sdn_compare( parent, &child_parent ) == 0 );
}

int slapi_sdn_isgrandparent( const Slapi_DN *parent, const Slapi_DN *child )
{
	Slapi_DN child_grandparent;

	slapi_sdn_get_ndn( child );

	slapi_sdn_init( &child_grandparent );
	dnParent( (struct berval *)&child->ndn, &child_grandparent.ndn );
	if ( child_grandparent.ndn.bv_len == 0 ) {
		return 0;
	}

	dnParent( &child_grandparent.ndn, &child_grandparent.ndn );

	return ( slapi_sdn_compare( parent, &child_grandparent ) == 0 );
}

int slapi_sdn_get_ndn_len( const Slapi_DN *sdn )
{
	slapi_sdn_get_ndn( sdn );

	return sdn->ndn.bv_len;
}

int slapi_sdn_scope_test( const Slapi_DN *dn, const Slapi_DN *base, int scope )
{
	int rc;

	switch ( scope ) {
	case LDAP_SCOPE_BASE:
		rc = ( slapi_sdn_compare( dn, base ) == 0 );
		break;
	case LDAP_SCOPE_ONELEVEL:
		rc = slapi_sdn_isparent( base, dn );
		break;
	case LDAP_SCOPE_SUBTREE:
		rc = slapi_sdn_issuffix( dn, base );
		break;
	default:
		rc = 0;
		break;
	}

	return rc;
}

void slapi_rdn_init( Slapi_RDN *rdn )
{
	rdn->flag = 0;
	BER_BVZERO( &rdn->bv );
	rdn->rdn = NULL;
}

Slapi_RDN *slapi_rdn_new( void )
{
	Slapi_RDN *rdn;

	rdn = (Slapi_RDN *)slapi_ch_malloc( sizeof(*rdn ));
	slapi_rdn_init( rdn );

	return rdn;
}

Slapi_RDN *slapi_rdn_new_dn( const char *dn )
{
	Slapi_RDN *rdn;

	rdn = slapi_rdn_new();
	slapi_rdn_init_dn( rdn, dn );
	return rdn;
}

Slapi_RDN *slapi_rdn_new_sdn( const Slapi_DN *sdn )
{
	return slapi_rdn_new_dn( slapi_sdn_get_dn( sdn ) );
}

Slapi_RDN *slapi_rdn_new_rdn( const Slapi_RDN *fromrdn )
{
	return slapi_rdn_new_dn( fromrdn->bv.bv_val );
}

void slapi_rdn_init_dn( Slapi_RDN *rdn, const char *dn )
{
	slapi_rdn_init( rdn );
	slapi_rdn_set_dn( rdn, dn );
}

void slapi_rdn_init_sdn( Slapi_RDN *rdn, const Slapi_DN *sdn )
{
	slapi_rdn_init( rdn );
	slapi_rdn_set_sdn( rdn, sdn );
}

void slapi_rdn_init_rdn( Slapi_RDN *rdn, const Slapi_RDN *fromrdn )
{
	slapi_rdn_init( rdn );
	slapi_rdn_set_rdn( rdn, fromrdn );
}

void slapi_rdn_set_dn( Slapi_RDN *rdn, const char *dn )
{
	struct berval bv;

	slapi_rdn_done( rdn );

	BER_BVZERO( &bv );

	if ( dn != NULL ) {
		bv.bv_val = (char *)dn;
		bv.bv_len = strlen( dn );
	}

	dnExtractRdn( &bv, &rdn->bv, NULL );
	rdn->flag |= FLAG_DN;
}

void slapi_rdn_set_sdn( Slapi_RDN *rdn, const Slapi_DN *sdn )
{
	slapi_rdn_set_dn( rdn, slapi_sdn_get_dn( sdn ) );
}

void slapi_rdn_set_rdn( Slapi_RDN *rdn, const Slapi_RDN *fromrdn )
{
	slapi_rdn_set_dn( rdn, fromrdn->bv.bv_val );
}

void slapi_rdn_free( Slapi_RDN **rdn )
{
	slapi_rdn_done( *rdn );
	slapi_ch_free( (void **)rdn );
}

void slapi_rdn_done( Slapi_RDN *rdn )
{
	if ( rdn->rdn != NULL ) {
		ldap_rdnfree( rdn->rdn );
		rdn->rdn = NULL;
	}
	slapi_ch_free_string( &rdn->bv.bv_val );
	slapi_rdn_init( rdn );
}

const char *slapi_rdn_get_rdn( const Slapi_RDN *rdn )
{
	return rdn->bv.bv_val;
}

static int slapi_int_rdn_explode( Slapi_RDN *rdn )
{
	char *next;

	if ( rdn->rdn != NULL ) {
		return LDAP_SUCCESS;
	}

	return ldap_bv2rdn( &rdn->bv, &rdn->rdn, &next, LDAP_DN_FORMAT_LDAP );
}

static int slapi_int_rdn_implode( Slapi_RDN *rdn )
{
	struct berval bv;
	int rc;

	if ( rdn->rdn == NULL ) {
		return LDAP_SUCCESS;
	}

	rc = ldap_rdn2bv( rdn->rdn, &bv, LDAP_DN_FORMAT_LDAPV3 | LDAP_DN_PRETTY );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	slapi_ch_free_string( &rdn->bv.bv_val );
	rdn->bv = bv;

	return 0;
}

int slapi_rdn_get_num_components( Slapi_RDN *rdn )
{
	int i;

	if ( slapi_int_rdn_explode( rdn ) != LDAP_SUCCESS )
		return 0;

	for ( i = 0; rdn->rdn[i] != NULL; i++ )
		;

	return i;
}

int slapi_rdn_get_first( Slapi_RDN *rdn, char **type, char **value )
{
	return slapi_rdn_get_next( rdn, 0, type, value );
}

int slapi_rdn_get_next( Slapi_RDN *rdn, int index, char **type, char **value )
{
	slapi_int_rdn_explode( rdn );

	if ( rdn->rdn == NULL || rdn->rdn[index] == NULL )
		return -1;

	*type = rdn->rdn[index]->la_attr.bv_val;
	*value = rdn->rdn[index]->la_value.bv_val;

	return index + 1;
}

int slapi_rdn_get_index( Slapi_RDN *rdn, const char *type, const char *value, size_t length )
{
	int i, match;
	struct berval bv;
	AttributeDescription *ad = NULL;
	const char *text;

	slapi_int_rdn_explode( rdn );

	if ( slap_str2ad( type, &ad, &text ) != LDAP_SUCCESS ) {
		return -1;
	}

	bv.bv_val = (char *)value;
	bv.bv_len = length;

	for ( i = 0; rdn->rdn[i] != NULL; i++ ) {
		if ( !slapi_attr_types_equivalent( ad->ad_cname.bv_val, type ))
			continue;

		if ( value_match( &match, ad, ad->ad_type->sat_equality, 0,
			&rdn->rdn[i]->la_value, (void *)&bv, &text ) != LDAP_SUCCESS )
			match = -1;

		if ( match == 0 )
			return i;
	}

	return -1;
}

int slapi_rdn_get_index_attr( Slapi_RDN *rdn, const char *type, char **value )
{
	int i;

	for ( i = 0; rdn->rdn[i] != NULL; i++ ) {
		if ( slapi_attr_types_equivalent( rdn->rdn[i]->la_attr.bv_val, type ) ) {
			*value = rdn->rdn[i]->la_value.bv_val;
			return i;
		}
	}

	return -1;
}

int slapi_rdn_contains( Slapi_RDN *rdn, const char *type, const char *value, size_t length )
{
	return ( slapi_rdn_get_index( rdn, type, value, length ) != -1 );
}

int slapi_rdn_contains_attr( Slapi_RDN *rdn, const char *type, char **value )
{
	return ( slapi_rdn_get_index_attr( rdn, type, value ) != -1 );
}

int slapi_rdn_compare( Slapi_RDN *rdn1, Slapi_RDN *rdn2 )
{
	struct berval nrdn1 = BER_BVNULL;
	struct berval nrdn2 = BER_BVNULL;
	int match;

	rdnNormalize( 0, NULL, NULL, (struct berval *)&rdn1->bv, &nrdn1, NULL );
	rdnNormalize( 0, NULL, NULL, (struct berval *)&rdn2->bv, &nrdn2, NULL );

	if ( rdnMatch( &match, 0, NULL, NULL, &nrdn1, (void *)&nrdn2 ) != LDAP_SUCCESS) {
		match = -1;
	}

	return match;
}

int slapi_rdn_isempty( const Slapi_RDN *rdn )
{
	return ( BER_BVISEMPTY( &rdn->bv ) ); 
}

int slapi_rdn_add( Slapi_RDN *rdn, const char *type, const char *value )
{
	char *s;
	size_t len;

	len = strlen(type) + 1 + strlen( value );
	if ( !BER_BVISEMPTY( &rdn->bv ) ) {
		len += 1 + rdn->bv.bv_len;
	}

	s = slapi_ch_malloc( len + 1 );

	if ( BER_BVISEMPTY( &rdn->bv ) ) {
		snprintf( s, len + 1, "%s=%s", type, value );
	} else {
		snprintf( s, len + 1, "%s=%s+%s", type, value, rdn->bv.bv_val );
	}

	slapi_rdn_done( rdn );

	rdn->bv.bv_len = len;
	rdn->bv.bv_val = s;

	return 1;
}

int slapi_rdn_remove_index( Slapi_RDN *rdn, int atindex )
{
	int count, i;

	count = slapi_rdn_get_num_components( rdn );

	if ( atindex < 0 || atindex >= count )
		return 0;

	if ( rdn->rdn == NULL )
		return 0;

	slapi_ch_free_string( &rdn->rdn[atindex]->la_attr.bv_val );
	slapi_ch_free_string( &rdn->rdn[atindex]->la_value.bv_val );

	for ( i = atindex; i < count; i++ ) {
		rdn->rdn[i] = rdn->rdn[i + 1];
	}

	if ( slapi_int_rdn_implode( rdn ) != LDAP_SUCCESS )
		return 0;

	return 1;
}

int slapi_rdn_remove( Slapi_RDN *rdn, const char *type, const char *value, size_t length )
{
	int index = slapi_rdn_get_index( rdn, type, value, length );

	return slapi_rdn_remove_index( rdn, index );
}

int slapi_rdn_remove_attr( Slapi_RDN *rdn, const char *type )
{
	char *value;
	int index = slapi_rdn_get_index_attr( rdn, type, &value );

	return slapi_rdn_remove_index( rdn, index );
}

Slapi_DN *slapi_sdn_add_rdn( Slapi_DN *sdn, const Slapi_RDN *rdn )
{
	struct berval bv;

	build_new_dn( &bv, &sdn->dn, (struct berval *)&rdn->bv, NULL );

	slapi_sdn_done( sdn );
	sdn->dn = bv;

	return sdn;
}

Slapi_DN *slapi_sdn_set_parent( Slapi_DN *sdn, const Slapi_DN *parentdn )
{
	Slapi_RDN rdn;

	slapi_rdn_init_sdn( &rdn, sdn );
	slapi_sdn_set_dn_byref( sdn, slapi_sdn_get_dn( parentdn ) );
	slapi_sdn_add_rdn( sdn, &rdn );
	slapi_rdn_done( &rdn );

	return sdn;
}

#endif /* LDAP_SLAPI */
