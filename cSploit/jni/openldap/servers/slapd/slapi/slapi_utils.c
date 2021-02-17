/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002-2003 IBM Corporation.
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
 * This work was initially developed by IBM Corporation for use in
 * IBM products and subsequently ported to OpenLDAP Software by
 * Steve Omrani.  Additional significant contributors include:
 *   Luke Howard
 */

#include "portable.h"

#include <ac/string.h>
#include <ac/stdarg.h>
#include <ac/ctype.h>
#include <ac/unistd.h>
#include <lutil.h>

#include <slap.h>
#include <slapi.h>

#include <netdb.h>

#ifdef LDAP_SLAPI

/*
 * server start time (should we use a struct timeval also in slapd?
 */
static struct			timeval base_time;
ldap_pvt_thread_mutex_t		slapi_hn_mutex;
ldap_pvt_thread_mutex_t		slapi_time_mutex;

struct slapi_mutex {
	ldap_pvt_thread_mutex_t mutex;
};

struct slapi_condvar {
	ldap_pvt_thread_cond_t cond;
	ldap_pvt_thread_mutex_t mutex;
};

static int checkBVString(const struct berval *bv)
{
	ber_len_t i;

	for ( i = 0; i < bv->bv_len; i++ ) {
		if ( bv->bv_val[i] == '\0' )
			return 0;
	}
	if ( bv->bv_val[i] != '\0' )
		return 0;

	return 1;
}

/*
 * This function converts an array of pointers to berval objects to
 * an array of berval objects.
 */

int
bvptr2obj(
	struct berval	**bvptr, 
	BerVarray	*bvobj,
	unsigned *num )
{
	int		rc = LDAP_SUCCESS;
	int		i;
	BerVarray	tmpberval;

	if ( bvptr == NULL || *bvptr == NULL ) {
		return LDAP_OTHER;
	}

	for ( i = 0; bvptr != NULL && bvptr[i] != NULL; i++ ) {
		; /* EMPTY */
	}
	if ( num )
		*num = i;

	tmpberval = (BerVarray)slapi_ch_malloc( (i + 1)*sizeof(struct berval));
	if ( tmpberval == NULL ) {
		return LDAP_NO_MEMORY;
	} 

	for ( i = 0; bvptr[i] != NULL; i++ ) {
		tmpberval[i].bv_val = bvptr[i]->bv_val;
		tmpberval[i].bv_len = bvptr[i]->bv_len;
	}
	tmpberval[i].bv_val = NULL;
	tmpberval[i].bv_len = 0;

	if ( rc == LDAP_SUCCESS ) {
		*bvobj = tmpberval;
	}

	return rc;
}

Slapi_Entry *
slapi_str2entry(
	char		*s, 
	int		flags )
{
	return str2entry( s );
}

char *
slapi_entry2str(
	Slapi_Entry	*e, 
	int		*len ) 
{
	char		*ret = NULL;
	char		*s;

	ldap_pvt_thread_mutex_lock( &entry2str_mutex );
	s = entry2str( e, len );
	if ( s != NULL )
		ret = slapi_ch_strdup( s );
	ldap_pvt_thread_mutex_unlock( &entry2str_mutex );

	return ret;
}

char *
slapi_entry_get_dn( Slapi_Entry *e ) 
{
	return e->e_name.bv_val;
}

int
slapi_x_entry_get_id( Slapi_Entry *e )
{
	return e->e_id;
}

static int
slapi_int_dn_pretty( struct berval *in, struct berval *out )
{
	Syntax		*syntax = slap_schema.si_syn_distinguishedName;

	assert( syntax != NULL );

	return (syntax->ssyn_pretty)( syntax, in, out, NULL );
}

static int
slapi_int_dn_normalize( struct berval *in, struct berval *out )
{
	MatchingRule	*mr = slap_schema.si_mr_distinguishedNameMatch;
	Syntax		*syntax = slap_schema.si_syn_distinguishedName;

	assert( mr != NULL );

	return (mr->smr_normalize)( 0, syntax, mr, in, out, NULL );
}

void 
slapi_entry_set_dn(
	Slapi_Entry	*e, 
	char		*ldn )
{
	struct berval	dn = BER_BVNULL;

	dn.bv_val = ldn;
	dn.bv_len = strlen( ldn );

	slapi_int_dn_pretty( &dn, &e->e_name );
	slapi_int_dn_normalize( &dn, &e->e_nname );
}

Slapi_Entry *
slapi_entry_dup( Slapi_Entry *e ) 
{
	return entry_dup( e );
}

int 
slapi_entry_attr_delete(
	Slapi_Entry	*e, 		
	char		*type ) 
{
	AttributeDescription	*ad = NULL;
	const char		*text;

	if ( slap_str2ad( type, &ad, &text ) != LDAP_SUCCESS ) {
		return 1;	/* LDAP_NO_SUCH_ATTRIBUTE */
	}

	if ( attr_delete( &e->e_attrs, ad ) == LDAP_SUCCESS ) {
		return 0;	/* attribute is deleted */
	} else {
		return -1;	/* something went wrong */
	}
}

Slapi_Entry *
slapi_entry_alloc( void ) 
{
	return (Slapi_Entry *)entry_alloc();
}

void 
slapi_entry_free( Slapi_Entry *e ) 
{
	if ( e != NULL )
		entry_free( e );
}

int 
slapi_entry_attr_merge(
	Slapi_Entry	*e, 
	char		*type, 
	struct berval	**vals ) 
{
	AttributeDescription	*ad = NULL;
	const char		*text;
	BerVarray		bv;
	int			rc;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}
	
	rc = bvptr2obj( vals, &bv, NULL );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}
	
	rc = attr_merge_normalize( e, ad, bv, NULL );
	ch_free( bv );

	return rc;
}

int
slapi_entry_attr_find(
	Slapi_Entry	*e, 
	char		*type, 
	Slapi_Attr	**attr ) 
{
	AttributeDescription	*ad = NULL;
	const char		*text;
	int			rc;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}

	*attr = attr_find( e->e_attrs, ad );
	if ( *attr == NULL ) {
		return -1;
	}

	return 0;
}

char *
slapi_entry_attr_get_charptr( const Slapi_Entry *e, const char *type )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return NULL;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return NULL;
	}

	if ( attr->a_vals != NULL && attr->a_vals[0].bv_len != 0 ) {
		const char *p;

		p = slapi_value_get_string( &attr->a_vals[0] );
		if ( p != NULL ) {
			return slapi_ch_strdup( p );
		}
	}

	return NULL;
}

int
slapi_entry_attr_get_int( const Slapi_Entry *e, const char *type )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return 0;
	}

	return slapi_value_get_int( attr->a_vals );
}

long
slapi_entry_attr_get_long( const Slapi_Entry *e, const char *type )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return 0;
	}

	return slapi_value_get_long( attr->a_vals );
}

unsigned int
slapi_entry_attr_get_uint( const Slapi_Entry *e, const char *type )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return 0;
	}

	return slapi_value_get_uint( attr->a_vals );
}

unsigned long
slapi_entry_attr_get_ulong( const Slapi_Entry *e, const char *type )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return 0;
	}

	return slapi_value_get_ulong( attr->a_vals );
}

int
slapi_entry_attr_hasvalue( Slapi_Entry *e, const char *type, const char *value )
{
	struct berval bv;
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	Attribute *attr;
	
	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr = attr_find( e->e_attrs, ad );
	if ( attr == NULL ) {
		return 0;
	}

	bv.bv_val = (char *)value;
	bv.bv_len = strlen( value );

	return ( slapi_attr_value_find( attr, &bv ) != -1 );
}

void
slapi_entry_attr_set_charptr(Slapi_Entry* e, const char *type, const char *value)
{
	AttributeDescription	*ad = NULL;
	const char		*text;
	int			rc;
	struct berval		bv;
	
	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return;
	}
	
	attr_delete ( &e->e_attrs, ad );
	if ( value != NULL ) {
		bv.bv_val = (char *)value;
		bv.bv_len = strlen(value);
		attr_merge_normalize_one( e, ad, &bv, NULL );
	}
}

void
slapi_entry_attr_set_int( Slapi_Entry* e, const char *type, int l)
{
	char buf[64];

	snprintf( buf, sizeof( buf ), "%d", l );
	slapi_entry_attr_set_charptr( e, type, buf );
}

void
slapi_entry_attr_set_uint( Slapi_Entry* e, const char *type, unsigned int l)
{
	char buf[64];

	snprintf( buf, sizeof( buf ), "%u", l );
	slapi_entry_attr_set_charptr( e, type, buf );
}

void
slapi_entry_attr_set_long(Slapi_Entry* e, const char *type, long l)
{
	char buf[64];

	snprintf( buf, sizeof( buf ), "%ld", l );
	slapi_entry_attr_set_charptr( e, type, buf );
}

void
slapi_entry_attr_set_ulong(Slapi_Entry* e, const char *type, unsigned long l)
{
	char buf[64];

	snprintf( buf, sizeof( buf ), "%lu", l );
	slapi_entry_attr_set_charptr( e, type, buf );
}

int
slapi_is_rootdse( const char *dn )
{
	return ( dn == NULL || dn[0] == '\0' );
}

int
slapi_entry_has_children( const Slapi_Entry *e )
{
	Slapi_PBlock *pb;
	Backend *be = select_backend( (struct berval *)&e->e_nname, 0 );
	int rc, hasSubordinates = 0;

	if ( be == NULL || be->be_has_subordinates == 0 ) {
		return 0;
	}

	pb = slapi_pblock_new();
	if ( pb == NULL ) {
		return 0;
	}
	slapi_int_connection_init_pb( pb, LDAP_REQ_SEARCH );

	rc = slapi_pblock_set( pb, SLAPI_TARGET_DN, slapi_entry_get_dn(
		(Entry *) e ));
	if ( rc == LDAP_SUCCESS ) {
		pb->pb_op->o_bd = be;
		rc = be->be_has_subordinates( pb->pb_op, (Entry *) e,
			&hasSubordinates );
	}

	slapi_pblock_destroy( pb );

	return ( rc == LDAP_SUCCESS && hasSubordinates == LDAP_COMPARE_TRUE );
}

/*
 * Return approximate size of the entry rounded to the nearest
 * 1K. Only the size of the attribute values are counted in the
 * Sun implementation.
 *
 * http://docs.sun.com/source/816-6701-10/funcref.html#1017388
 */
size_t slapi_entry_size(Slapi_Entry *e)
{
	size_t size;
	Attribute *a;
	int i;

	for ( size = 0, a = e->e_attrs; a != NULL; a = a->a_next ) {
		for ( i = 0; a->a_vals[i].bv_val != NULL; i++ ) {
			size += a->a_vals[i].bv_len + 1;
		}
	}

	size += 1023;
	size -= (size % 1024);

	return size;
}

/*
 * Add values to entry.
 *
 * Returns:
 *	LDAP_SUCCESS			Values added to entry
 *	LDAP_TYPE_OR_VALUE_EXISTS	One or more values exist in entry already
 *	LDAP_CONSTRAINT_VIOLATION	Any other error (odd, but it's the spec)
 */
int
slapi_entry_add_values( Slapi_Entry *e, const char *type, struct berval **vals )
{
	Modification		mod;
	const char		*text;
	int			rc;
	char			textbuf[SLAP_TEXT_BUFLEN];

	mod.sm_op = LDAP_MOD_ADD;
	mod.sm_flags = 0;
	mod.sm_desc = NULL;
	mod.sm_type.bv_val = (char *)type;
	mod.sm_type.bv_len = strlen( type );

	rc = slap_str2ad( type, &mod.sm_desc, &text );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( vals == NULL ) {
		/* Apparently vals can be NULL
		 * FIXME: sm_values = NULL ? */
		mod.sm_values = (BerVarray)ch_malloc( sizeof(struct berval) );
		mod.sm_values->bv_val = NULL;
		mod.sm_numvals = 0;

	} else {
		rc = bvptr2obj( vals, &mod.sm_values, &mod.sm_numvals );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_CONSTRAINT_VIOLATION;
		}
	}
	mod.sm_nvalues = NULL;

	rc = modify_add_values( e, &mod, 0, &text, textbuf, sizeof(textbuf) );

	slapi_ch_free( (void **)&mod.sm_values );

	return (rc == LDAP_SUCCESS) ? LDAP_SUCCESS : LDAP_CONSTRAINT_VIOLATION;
}

int
slapi_entry_add_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals )
{
	return slapi_entry_add_values( e, type, vals );
}

int
slapi_entry_add_valueset(Slapi_Entry *e, const char *type, Slapi_ValueSet *vs)
{
	AttributeDescription	*ad = NULL;
	const char		*text;
	int			rc;
	
	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}

	return attr_merge_normalize( e, ad, *vs, NULL );
}

int
slapi_entry_delete_values( Slapi_Entry *e, const char *type, struct berval **vals )
{
	Modification		mod;
	const char		*text;
	int			rc;
	char			textbuf[SLAP_TEXT_BUFLEN];

	mod.sm_op = LDAP_MOD_DELETE;
	mod.sm_flags = 0;
	mod.sm_desc = NULL;
	mod.sm_type.bv_val = (char *)type;
	mod.sm_type.bv_len = strlen( type );

	if ( vals == NULL ) {
		/* If vals is NULL, this is a NOOP. */
		return LDAP_SUCCESS;
	}
	
	rc = slap_str2ad( type, &mod.sm_desc, &text );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( vals[0] == NULL ) {
		/* SLAPI doco says LDApb_opERATIONS_ERROR but LDAP_OTHER is better */
		return attr_delete( &e->e_attrs, mod.sm_desc ) ? LDAP_OTHER : LDAP_SUCCESS;
	}

	rc = bvptr2obj( vals, &mod.sm_values, &mod.sm_numvals );
	if ( rc != LDAP_SUCCESS ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	mod.sm_nvalues = NULL;

	rc = modify_delete_values( e, &mod, 0, &text, textbuf, sizeof(textbuf) );

	slapi_ch_free( (void **)&mod.sm_values );

	return rc;
}

int
slapi_entry_delete_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals )
{
	return slapi_entry_delete_values( e, type, vals );
}

int
slapi_entry_merge_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals )
{
	return slapi_entry_attr_merge( e, (char *)type, vals );
}

int
slapi_entry_add_value(Slapi_Entry *e, const char *type, const Slapi_Value *value)
{
	AttributeDescription	*ad = NULL;
	int 			rc;
	const char		*text;

	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}

	rc = attr_merge_normalize_one( e, ad, (Slapi_Value *)value, NULL );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}

	return 0;
}

int
slapi_entry_add_string(Slapi_Entry *e, const char *type, const char *value)
{
	Slapi_Value val;

	val.bv_val = (char *)value;
	val.bv_len = strlen( value );

	return slapi_entry_add_value( e, type, &val );
}

int
slapi_entry_delete_string(Slapi_Entry *e, const char *type, const char *value)
{
	Slapi_Value *vals[2];
	Slapi_Value val;

	val.bv_val = (char *)value;
	val.bv_len = strlen( value );
	vals[0] = &val;
	vals[1] = NULL;

	return slapi_entry_delete_values_sv( e, type, vals );	
}

int
slapi_entry_attr_merge_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals )
{
	return slapi_entry_attr_merge( e, (char *)type, vals );
}

int
slapi_entry_first_attr( const Slapi_Entry *e, Slapi_Attr **attr )
{
	if ( e == NULL ) {
		return -1;
	}

	*attr = e->e_attrs;

	return ( *attr != NULL ) ? 0 : -1;
}

int
slapi_entry_next_attr( const Slapi_Entry *e, Slapi_Attr *prevattr, Slapi_Attr **attr )
{
	if ( e == NULL ) {
		return -1;
	}

	if ( prevattr == NULL ) {
		return -1;
	}

	*attr = prevattr->a_next;

	return ( *attr != NULL ) ? 0 : -1;
}

int
slapi_entry_attr_replace_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals )
{
	AttributeDescription *ad = NULL;
	const char *text;
	int rc;
	BerVarray bv;
	
	rc = slap_str2ad( type, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	attr_delete( &e->e_attrs, ad );

	rc = bvptr2obj( vals, &bv, NULL );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}
	
	rc = attr_merge_normalize( e, ad, bv, NULL );
	slapi_ch_free( (void **)&bv );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}
	
	return 0;
}

/* 
 * FIXME -- The caller must free the allocated memory. 
 * In Netscape they do not have to.
 */
int 
slapi_attr_get_values(
	Slapi_Attr	*attr, 
	struct berval	***vals ) 
{
	int		i, j;
	struct berval	**bv;

	if ( attr == NULL ) {
		return 1;
	}

	for ( i = 0; attr->a_vals[i].bv_val != NULL; i++ ) {
		; /* EMPTY */
	}

	bv = (struct berval **)ch_malloc( (i + 1) * sizeof(struct berval *) );
	for ( j = 0; j < i; j++ ) {
		bv[j] = ber_dupbv( NULL, &attr->a_vals[j] );
	}
	bv[j] = NULL;
	
	*vals = (struct berval **)bv;

	return 0;
}

char *
slapi_dn_normalize( char *dn ) 
{
	struct berval	bdn;
	struct berval	pdn;

	assert( dn != NULL );
	
	bdn.bv_val = dn;
	bdn.bv_len = strlen( dn );

	if ( slapi_int_dn_pretty( &bdn, &pdn ) != LDAP_SUCCESS ) {
		return NULL;
	}

	return pdn.bv_val;
}

char *
slapi_dn_normalize_case( char *dn ) 
{
	struct berval	bdn;
	struct berval	ndn;

	assert( dn != NULL );
	
	bdn.bv_val = dn;
	bdn.bv_len = strlen( dn );

	if ( slapi_int_dn_normalize( &bdn, &ndn ) != LDAP_SUCCESS ) {
		return NULL;
	}

	return ndn.bv_val;
}

int 
slapi_dn_issuffix(
	char		*dn, 
	char		*suffix )
{
	struct berval	bdn, ndn;
	struct berval	bsuffix, nsuffix;
	int rc;

	assert( dn != NULL );
	assert( suffix != NULL );

	bdn.bv_val = dn;
	bdn.bv_len = strlen( dn );

	bsuffix.bv_val = suffix;
	bsuffix.bv_len = strlen( suffix );

	if ( dnNormalize( 0, NULL, NULL, &bdn, &ndn, NULL ) != LDAP_SUCCESS ) {
		return 0;
	}

	if ( dnNormalize( 0, NULL, NULL, &bsuffix, &nsuffix, NULL )
		!= LDAP_SUCCESS )
	{
		slapi_ch_free( (void **)&ndn.bv_val );
		return 0;
	}

	rc = dnIsSuffix( &ndn, &nsuffix );

	slapi_ch_free( (void **)&ndn.bv_val );
	slapi_ch_free( (void **)&nsuffix.bv_val );

	return rc;
}

int
slapi_dn_isparent(
	const char	*parentdn,
	const char	*childdn )
{
	struct berval	assertedParentDN, normalizedAssertedParentDN;
	struct berval	childDN, normalizedChildDN;
	struct berval	normalizedParentDN;
	int		match;

	assert( parentdn != NULL );
	assert( childdn != NULL );

	assertedParentDN.bv_val = (char *)parentdn;
	assertedParentDN.bv_len = strlen( parentdn );

	if ( dnNormalize( 0, NULL, NULL, &assertedParentDN,
		&normalizedAssertedParentDN, NULL ) != LDAP_SUCCESS )
	{
		return 0;
	}

	childDN.bv_val = (char *)childdn;
	childDN.bv_len = strlen( childdn );

	if ( dnNormalize( 0, NULL, NULL, &childDN,
		&normalizedChildDN, NULL ) != LDAP_SUCCESS )
	{
		slapi_ch_free( (void **)&normalizedAssertedParentDN.bv_val );
		return 0;
	}

	dnParent( &normalizedChildDN, &normalizedParentDN );

	if ( dnMatch( &match, 0, slap_schema.si_syn_distinguishedName, NULL,
		&normalizedParentDN, (void *)&normalizedAssertedParentDN ) != LDAP_SUCCESS )
	{
		match = -1;
	}

	slapi_ch_free( (void **)&normalizedAssertedParentDN.bv_val );
	slapi_ch_free( (void **)&normalizedChildDN.bv_val );

	return ( match == 0 );
}

/*
 * Returns DN of the parent entry, or NULL if the DN is
 * an empty string or NULL, or has no parent.
 */
char *
slapi_dn_parent( const char *_dn )
{
	struct berval	dn, prettyDN;
	struct berval	parentDN;
	char		*ret;

	if ( _dn == NULL ) {
		return NULL;
	}

	dn.bv_val = (char *)_dn;
	dn.bv_len = strlen( _dn );

	if ( dn.bv_len == 0 ) {
		return NULL;
	}

	if ( dnPretty( NULL, &dn, &prettyDN, NULL ) != LDAP_SUCCESS ) {
		return NULL;
	}

	dnParent( &prettyDN, &parentDN ); /* in-place */

	if ( parentDN.bv_len == 0 ) {
		slapi_ch_free_string( &prettyDN.bv_val );
		return NULL;
	}

	ret = slapi_ch_strdup( parentDN.bv_val );
	slapi_ch_free_string( &prettyDN.bv_val );

	return ret;
}

int slapi_dn_isbesuffix( Slapi_PBlock *pb, char *ldn )
{
	struct berval	ndn;
	Backend		*be;

	if ( slapi_is_rootdse( ldn ) ) {
		return 0;
	}

	/* according to spec should already be normalized */
	ndn.bv_len = strlen( ldn );
	ndn.bv_val = ldn;

	be = select_backend( &pb->pb_op->o_req_ndn, 0 );
	if ( be == NULL ) {
		return 0;
	}

	return be_issuffix( be, &ndn );
}

/*
 * Returns DN of the parent entry; or NULL if the DN is
 * an empty string, if the DN has no parent, or if the
 * DN is the suffix of the backend database
 */
char *slapi_dn_beparent( Slapi_PBlock *pb, const char *ldn )
{
	Backend 	*be;
	struct berval	dn, prettyDN;
	struct berval	normalizedDN, parentDN;
	char		*parent = NULL;

	if ( pb == NULL ) {
		return NULL;
	}

	PBLOCK_ASSERT_OP( pb, 0 );

	if ( slapi_is_rootdse( ldn ) ) {
		return NULL;
	}

	dn.bv_val = (char *)ldn;
	dn.bv_len = strlen( ldn );

	if ( dnPrettyNormal( NULL, &dn, &prettyDN, &normalizedDN, NULL ) != LDAP_SUCCESS ) {
		return NULL;
	}

	be = select_backend( &pb->pb_op->o_req_ndn, 0 );

	if ( be == NULL || be_issuffix( be, &normalizedDN ) == 0 ) {
		dnParent( &prettyDN, &parentDN );

		if ( parentDN.bv_len != 0 )
			parent = slapi_ch_strdup( parentDN.bv_val );
	}

	slapi_ch_free_string( &prettyDN.bv_val );
	slapi_ch_free_string( &normalizedDN.bv_val );

	return parent;
}

char *
slapi_dn_ignore_case( char *dn )
{       
	return slapi_dn_normalize_case( dn );
}

char *
slapi_ch_malloc( unsigned long size ) 
{
	return ch_malloc( size );	
}

void 
slapi_ch_free( void **ptr ) 
{
	if ( ptr == NULL || *ptr == NULL )
		return;
	ch_free( *ptr );
	*ptr = NULL;
}

void 
slapi_ch_free_string( char **ptr ) 
{
	slapi_ch_free( (void **)ptr );
}

void
slapi_ch_array_free( char **arrayp )
{
	char **p;

	if ( arrayp != NULL ) {
		for ( p = arrayp; *p != NULL; p++ ) {
			slapi_ch_free( (void **)p );
		}
		slapi_ch_free( (void **)&arrayp );
	}
}

struct berval *
slapi_ch_bvdup(const struct berval *v)
{
	return ber_dupbv(NULL, (struct berval *)v);
}

struct berval **
slapi_ch_bvecdup(const struct berval **v)
{
	int i;
	struct berval **rv;

	if ( v == NULL ) {
		return NULL;
	}

	for ( i = 0; v[i] != NULL; i++ )
		;

	rv = (struct berval **) slapi_ch_malloc( (i + 1) * sizeof(struct berval *) );

	for ( i = 0; v[i] != NULL; i++ ) {
		rv[i] = slapi_ch_bvdup( v[i] );
	}
	rv[i] = NULL;

	return rv;
}

char *
slapi_ch_calloc(
	unsigned long nelem, 
	unsigned long size ) 
{
	return ch_calloc( nelem, size );
}

char *
slapi_ch_realloc(
	char *block, 
	unsigned long size ) 
{
	return ch_realloc( block, size );
}

char *
slapi_ch_strdup( const char *s ) 
{
	return ch_strdup( s );
}

size_t
slapi_ch_stlen( const char *s ) 
{
	return strlen( s );
}

int 
slapi_control_present(
	LDAPControl	**controls, 
	char		*oid, 
	struct berval	**val, 
	int		*iscritical ) 
{
	int		i;
	int		rc = 0;

	if ( val ) {
		*val = NULL;
	}
	
	if ( iscritical ) {
		*iscritical = 0;
	}
	
	for ( i = 0; controls != NULL && controls[i] != NULL; i++ ) {
		if ( strcmp( controls[i]->ldctl_oid, oid ) != 0 ) {
			continue;
		}

		rc = 1;
		if ( controls[i]->ldctl_value.bv_len != 0 ) {
			if ( val ) {
				*val = &controls[i]->ldctl_value;
			}
		}

		if ( iscritical ) {
			*iscritical = controls[i]->ldctl_iscritical;
		}

		break;
	}

	return rc;
}

static void
slapControlMask2SlapiControlOp(slap_mask_t slap_mask,
	unsigned long *slapi_mask)
{
	*slapi_mask = SLAPI_OPERATION_NONE;

	if ( slap_mask & SLAP_CTRL_ABANDON ) 
		*slapi_mask |= SLAPI_OPERATION_ABANDON;

	if ( slap_mask & SLAP_CTRL_ADD )
		*slapi_mask |= SLAPI_OPERATION_ADD;

	if ( slap_mask & SLAP_CTRL_BIND )
		*slapi_mask |= SLAPI_OPERATION_BIND;

	if ( slap_mask & SLAP_CTRL_COMPARE )
		*slapi_mask |= SLAPI_OPERATION_COMPARE;

	if ( slap_mask & SLAP_CTRL_DELETE )
		*slapi_mask |= SLAPI_OPERATION_DELETE;

	if ( slap_mask & SLAP_CTRL_MODIFY )
		*slapi_mask |= SLAPI_OPERATION_MODIFY;

	if ( slap_mask & SLAP_CTRL_RENAME )
		*slapi_mask |= SLAPI_OPERATION_MODDN;

	if ( slap_mask & SLAP_CTRL_SEARCH )
		*slapi_mask |= SLAPI_OPERATION_SEARCH;

	if ( slap_mask & SLAP_CTRL_UNBIND )
		*slapi_mask |= SLAPI_OPERATION_UNBIND;
}

static void
slapiControlOp2SlapControlMask(unsigned long slapi_mask,
	slap_mask_t *slap_mask)
{
	*slap_mask = 0;

	if ( slapi_mask & SLAPI_OPERATION_BIND )
		*slap_mask |= SLAP_CTRL_BIND;

	if ( slapi_mask & SLAPI_OPERATION_UNBIND )
		*slap_mask |= SLAP_CTRL_UNBIND;

	if ( slapi_mask & SLAPI_OPERATION_SEARCH )
		*slap_mask |= SLAP_CTRL_SEARCH;

	if ( slapi_mask & SLAPI_OPERATION_MODIFY )
		*slap_mask |= SLAP_CTRL_MODIFY;

	if ( slapi_mask & SLAPI_OPERATION_ADD )
		*slap_mask |= SLAP_CTRL_ADD;

	if ( slapi_mask & SLAPI_OPERATION_DELETE )
		*slap_mask |= SLAP_CTRL_DELETE;

	if ( slapi_mask & SLAPI_OPERATION_MODDN )
		*slap_mask |= SLAP_CTRL_RENAME;

	if ( slapi_mask & SLAPI_OPERATION_COMPARE )
		*slap_mask |= SLAP_CTRL_COMPARE;

	if ( slapi_mask & SLAPI_OPERATION_ABANDON )
		*slap_mask |= SLAP_CTRL_ABANDON;

	*slap_mask |= SLAP_CTRL_GLOBAL;
}

static int
slapi_int_parse_control(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	/* Plugins must deal with controls themselves. */

	return LDAP_SUCCESS;
}

void 
slapi_register_supported_control(
	char		*controloid, 
	unsigned long	controlops )
{
	slap_mask_t controlmask;

	slapiControlOp2SlapControlMask( controlops, &controlmask );

	register_supported_control( controloid, controlmask, NULL, slapi_int_parse_control, NULL );
}

int 
slapi_get_supported_controls(
	char		***ctrloidsp, 
	unsigned long	**ctrlopsp ) 
{
	int i, rc;

	rc = get_supported_controls( ctrloidsp, (slap_mask_t **)ctrlopsp );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	for ( i = 0; (*ctrloidsp)[i] != NULL; i++ ) {
		/* In place, naughty. */
		slapControlMask2SlapiControlOp( (*ctrlopsp)[i], &((*ctrlopsp)[i]) );
	}

	return LDAP_SUCCESS;
}

LDAPControl *
slapi_dup_control( LDAPControl *ctrl )
{
	LDAPControl *ret;

	ret = (LDAPControl *)slapi_ch_malloc( sizeof(*ret) );
	ret->ldctl_oid = slapi_ch_strdup( ctrl->ldctl_oid );
	ber_dupbv( &ret->ldctl_value, &ctrl->ldctl_value );
	ret->ldctl_iscritical = ctrl->ldctl_iscritical;

	return ret;
}

void 
slapi_register_supported_saslmechanism( char *mechanism )
{
	/* FIXME -- can not add saslmechanism to OpenLDAP dynamically */
	slapi_log_error( SLAPI_LOG_FATAL, "slapi_register_supported_saslmechanism",
			"OpenLDAP does not support dynamic registration of SASL mechanisms\n" );
}

char **
slapi_get_supported_saslmechanisms( void )
{
	/* FIXME -- can not get the saslmechanism without a connection. */
	slapi_log_error( SLAPI_LOG_FATAL, "slapi_get_supported_saslmechanisms",
			"can not get the SASL mechanism list "
			"without a connection\n" );
	return NULL;
}

char **
slapi_get_supported_extended_ops( void )
{
	int		i, j, k;
	char		**ppExtOpOID = NULL;
	int		numExtOps = 0;

	for ( i = 0; get_supported_extop( i ) != NULL; i++ ) {
		;
	}
	
	for ( j = 0; slapi_int_get_supported_extop( j ) != NULL; j++ ) {
		;
	}

	numExtOps = i + j;
	if ( numExtOps == 0 ) {
		return NULL;
	}

	ppExtOpOID = (char **)slapi_ch_malloc( (numExtOps + 1) * sizeof(char *) );
	for ( k = 0; k < i; k++ ) {
		struct berval	*bv;

		bv = get_supported_extop( k );
		assert( bv != NULL );

		ppExtOpOID[ k ] = bv->bv_val;
	}
	
	for ( ; k < j; k++ ) {
		struct berval	*bv;

		bv = slapi_int_get_supported_extop( k );
		assert( bv != NULL );

		ppExtOpOID[ i + k ] = bv->bv_val;
	}
	ppExtOpOID[ i + k ] = NULL;

	return ppExtOpOID;
}

void 
slapi_send_ldap_result(
	Slapi_PBlock	*pb, 
	int		err, 
	char		*matched, 
	char		*text, 
	int		nentries, 
	struct berval	**urls ) 
{
	SlapReply	*rs;

	PBLOCK_ASSERT_OP( pb, 0 );

	rs = pb->pb_rs;

	rs->sr_err = err;
	rs->sr_matched = matched;
	rs->sr_text = text;
	rs->sr_ref = NULL;

	if ( err == LDAP_SASL_BIND_IN_PROGRESS ) {
		send_ldap_sasl( pb->pb_op, rs );
	} else if ( rs->sr_rspoid != NULL ) {
		send_ldap_extended( pb->pb_op, rs );
	} else {
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			rs->sr_nentries = nentries;
		if ( urls != NULL )
			bvptr2obj( urls, &rs->sr_ref, NULL );

		send_ldap_result( pb->pb_op, rs );

		if ( urls != NULL )
			slapi_ch_free( (void **)&rs->sr_ref );
	}
}

int 
slapi_send_ldap_search_entry(
	Slapi_PBlock	*pb, 
	Slapi_Entry	*e, 
	LDAPControl	**ectrls, 
	char		**attrs, 
	int		attrsonly )
{
	SlapReply		rs = { REP_SEARCH };
	int			i = 0, j = 0;
	AttributeName		*an = NULL;
	const char		*text;
	int			rc;

	assert( pb->pb_op != NULL );

	if ( attrs != NULL ) {
		for ( i = 0; attrs[ i ] != NULL; i++ ) {
			; /* empty */
		}
	}

	if ( i ) {
		an = (AttributeName *) slapi_ch_calloc( i + 1, sizeof(AttributeName) );
		for ( i = 0; attrs[i] != NULL; i++ ) {
			an[j].an_name.bv_val = attrs[i];
			an[j].an_name.bv_len = strlen( attrs[i] );
			an[j].an_desc = NULL;
			if ( slap_bv2ad( &an[j].an_name, &an[j].an_desc, &text ) == LDAP_SUCCESS) {
				j++;
			}
		}
		an[j].an_name.bv_len = 0;
		an[j].an_name.bv_val = NULL;
	}

	rs.sr_err = LDAP_SUCCESS;
	rs.sr_matched = NULL;
	rs.sr_text = NULL;
	rs.sr_ref = NULL;
	rs.sr_ctrls = ectrls;
	rs.sr_attrs = an;
	rs.sr_operational_attrs = NULL;
	rs.sr_entry = e;
	rs.sr_v2ref = NULL;
	rs.sr_flags = 0;

	rc = send_search_entry( pb->pb_op, &rs );

	slapi_ch_free( (void **)&an );

	return rc;
}

int 
slapi_send_ldap_search_reference(
	Slapi_PBlock	*pb,
	Slapi_Entry	*e,
	struct berval	**references,
	LDAPControl	**ectrls, 
	struct berval	**v2refs
	)
{
	SlapReply	rs = { REP_SEARCHREF };
	int		rc;

	rs.sr_err = LDAP_SUCCESS;
	rs.sr_matched = NULL;
	rs.sr_text = NULL;

	rc = bvptr2obj( references, &rs.sr_ref, NULL );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rs.sr_ctrls = ectrls;
	rs.sr_attrs = NULL;
	rs.sr_operational_attrs = NULL;
	rs.sr_entry = e;

	if ( v2refs != NULL ) {
		rc = bvptr2obj( v2refs, &rs.sr_v2ref, NULL );
		if ( rc != LDAP_SUCCESS ) {
			slapi_ch_free( (void **)&rs.sr_ref );
			return rc;
		}
	} else {
		rs.sr_v2ref = NULL;
	}

	rc = send_search_reference( pb->pb_op, &rs );

	slapi_ch_free( (void **)&rs.sr_ref );
	slapi_ch_free( (void **)&rs.sr_v2ref );

	return rc;
}

Slapi_Filter *
slapi_str2filter( char *str ) 
{
	return str2filter( str );
}

void 
slapi_filter_free(
	Slapi_Filter	*f, 
	int		recurse ) 
{
	filter_free( f );
}

Slapi_Filter *
slapi_filter_dup( Slapi_Filter *filter )
{
	return filter_dup( filter, NULL );
}

int 
slapi_filter_get_choice( Slapi_Filter *f )
{
	int		rc;

	if ( f != NULL ) {
		rc = f->f_choice;
	} else {
		rc = 0;
	}

	return rc;
}

int 
slapi_filter_get_ava(
	Slapi_Filter	*f, 
	char		**type, 
	struct berval	**bval )
{
	int		ftype;
	int		rc = LDAP_SUCCESS;

	assert( type != NULL );
	assert( bval != NULL );

	*type = NULL;
	*bval = NULL;

	ftype = f->f_choice;
	if ( ftype == LDAP_FILTER_EQUALITY 
			|| ftype ==  LDAP_FILTER_GE 
			|| ftype == LDAP_FILTER_LE 
			|| ftype == LDAP_FILTER_APPROX ) {
		/*
		 * According to the SLAPI Reference Manual these are
		 * not duplicated.
		 */
		*type = f->f_un.f_un_ava->aa_desc->ad_cname.bv_val;
		*bval = &f->f_un.f_un_ava->aa_value;
	} else { /* filter type not supported */
		rc = -1;
	}

	return rc;
}

Slapi_Filter *
slapi_filter_list_first( Slapi_Filter *f )
{
	int		ftype;

	if ( f == NULL ) {
		return NULL;
	}

	ftype = f->f_choice;
	if ( ftype == LDAP_FILTER_AND
			|| ftype == LDAP_FILTER_OR
			|| ftype == LDAP_FILTER_NOT ) {
		return (Slapi_Filter *)f->f_list;
	} else {
		return NULL;
	}
}

Slapi_Filter *
slapi_filter_list_next(
	Slapi_Filter	*f, 
	Slapi_Filter	*fprev )
{
	int		ftype;

	if ( f == NULL ) {
		return NULL;
	}

	ftype = f->f_choice;
	if ( ftype == LDAP_FILTER_AND
			|| ftype == LDAP_FILTER_OR
			|| ftype == LDAP_FILTER_NOT )
	{
		return fprev->f_next;
	}

	return NULL;
}

int
slapi_filter_get_attribute_type( Slapi_Filter *f, char **type )
{
	if ( f == NULL ) {
		return -1;
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_APPROX:
		*type = f->f_av_desc->ad_cname.bv_val;
		break;
	case LDAP_FILTER_SUBSTRINGS:
		*type = f->f_sub_desc->ad_cname.bv_val;
		break;
	case LDAP_FILTER_PRESENT:
		*type = f->f_desc->ad_cname.bv_val;
		break;
	case LDAP_FILTER_EXT:
		*type = f->f_mr_desc->ad_cname.bv_val;
		break;
	default:
		/* Complex filters need not apply. */
		*type = NULL;
		return -1;
	}

	return 0;
}

int
slapi_x_filter_set_attribute_type( Slapi_Filter *f, const char *type )
{
	AttributeDescription **adp, *ad = NULL;
	const char *text;
	int rc;

	if ( f == NULL ) {
		return -1;
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_APPROX:
		adp = &f->f_av_desc;
		break;
	case LDAP_FILTER_SUBSTRINGS:
		adp = &f->f_sub_desc;
		break;
	case LDAP_FILTER_PRESENT:
		adp = &f->f_desc;
		break;
	case LDAP_FILTER_EXT:
		adp = &f->f_mr_desc;
		break;
	default:
		/* Complex filters need not apply. */
		return -1;
	}

	rc = slap_str2ad( type, &ad, &text );
	if ( rc == LDAP_SUCCESS )
		*adp = ad;

	return ( rc == LDAP_SUCCESS ) ? 0 : -1;
}

int
slapi_filter_get_subfilt( Slapi_Filter *f, char **type, char **initial,
	char ***any, char **final )
{
	int i;

	if ( f->f_choice != LDAP_FILTER_SUBSTRINGS ) {
		return -1;
	}

	/*
	 * The caller shouldn't free but we can't return an
	 * array of char *s from an array of bervals without
	 * allocating memory, so we may as well be consistent.
	 * XXX
	 */
	*type = f->f_sub_desc->ad_cname.bv_val;
	*initial = f->f_sub_initial.bv_val ? slapi_ch_strdup(f->f_sub_initial.bv_val) : NULL;
	if ( f->f_sub_any != NULL ) {
		for ( i = 0; f->f_sub_any[i].bv_val != NULL; i++ )
			;
		*any = (char **)slapi_ch_malloc( (i + 1) * sizeof(char *) );
		for ( i = 0; f->f_sub_any[i].bv_val != NULL; i++ ) {
			(*any)[i] = slapi_ch_strdup(f->f_sub_any[i].bv_val);
		}
		(*any)[i] = NULL;
	} else {
		*any = NULL;
	}
	*final = f->f_sub_final.bv_val ? slapi_ch_strdup(f->f_sub_final.bv_val) : NULL;

	return 0;
}

Slapi_Filter *
slapi_filter_join( int ftype, Slapi_Filter *f1, Slapi_Filter *f2 )
{
	Slapi_Filter *f = NULL;

	if ( ftype == LDAP_FILTER_AND ||
	     ftype == LDAP_FILTER_OR ||
	     ftype == LDAP_FILTER_NOT )
	{
		f = (Slapi_Filter *)slapi_ch_malloc( sizeof(*f) );
		f->f_choice = ftype;
		f->f_list = f1;
		f->f_list->f_next = f2;
		f->f_next = NULL;
	}

	return f;
}

int
slapi_x_filter_append( int ftype,
	Slapi_Filter **pContainingFilter, /* NULL on first call */
	Slapi_Filter **pNextFilter,
	Slapi_Filter *filterToAppend )
{
	if ( ftype == LDAP_FILTER_AND ||
	     ftype == LDAP_FILTER_OR ||
	     ftype == LDAP_FILTER_NOT )
	{
		if ( *pContainingFilter == NULL ) {
			*pContainingFilter = (Slapi_Filter *)slapi_ch_malloc( sizeof(Slapi_Filter) );
			(*pContainingFilter)->f_choice = ftype;
			(*pContainingFilter)->f_list = filterToAppend;
			(*pContainingFilter)->f_next = NULL;
		} else {
			if ( (*pContainingFilter)->f_choice != ftype ) {
				/* Sanity check */
				return -1;
			}
			(*pNextFilter)->f_next = filterToAppend;
		}
		*pNextFilter = filterToAppend;

		return 0;
	}
	return -1;
}

int
slapi_filter_test( Slapi_PBlock *pb, Slapi_Entry *e, Slapi_Filter *f,
	int verify_access )
{
	Operation *op;
	int rc;

	if ( f == NULL ) {
		/* spec says return zero if no filter. */
		return 0;
	}

	if ( verify_access ) {
		op = pb->pb_op;
		if ( op == NULL )
			return LDAP_PARAM_ERROR;
	} else {
		op = NULL;
	}

	/*
	 * According to acl.c it is safe to call test_filter() with
	 * NULL arguments...
	 */
	rc = test_filter( op, e, f );
	switch (rc) {
	case LDAP_COMPARE_TRUE:
		rc = 0;
		break;
	case LDAP_COMPARE_FALSE:
		break;
	case SLAPD_COMPARE_UNDEFINED:
		rc = LDAP_OTHER;
		break;
	case LDAP_PROTOCOL_ERROR:
		/* filter type unknown: spec says return -1 */
		rc = -1;
		break;
	}

	return rc;
}

int
slapi_filter_test_simple( Slapi_Entry *e, Slapi_Filter *f)
{
	return slapi_filter_test( NULL, e, f, 0 );
}

int
slapi_filter_apply( Slapi_Filter *f, FILTER_APPLY_FN fn, void *arg, int *error_code )
{
	switch ( f->f_choice ) {
	case LDAP_FILTER_AND:
	case LDAP_FILTER_NOT:
	case LDAP_FILTER_OR: {
		int rc;

		/*
		 * FIXME: altering f; should we use a temporary?
		 */
		for ( f = f->f_list; f != NULL; f = f->f_next ) {
			rc = slapi_filter_apply( f, fn, arg, error_code );
			if ( rc != 0 ) {
				return rc;
			}
			if ( *error_code == SLAPI_FILTER_SCAN_NOMORE ) {
				break;
			}
		}
		break;
	}
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_SUBSTRINGS:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_PRESENT:
	case LDAP_FILTER_APPROX:
	case LDAP_FILTER_EXT:
		*error_code = fn( f, arg );
		break;
	default:
		*error_code = SLAPI_FILTER_UNKNOWN_FILTER_TYPE;
	}

	if ( *error_code == SLAPI_FILTER_SCAN_NOMORE ||
	     *error_code == SLAPI_FILTER_SCAN_CONTINUE ) {
		return 0;
	}

	return -1;
}

int 
slapi_pw_find(
	struct berval	**vals, 
	struct berval	*v ) 
{
	int i;

	if( ( vals == NULL ) || ( v == NULL ) )
		return 1;

	for ( i = 0; vals[i] != NULL; i++ ) {
		if ( !lutil_passwd( vals[i], v, NULL, NULL ) )
			return 0;
	}

	return 1;
}

/* Get connected client IP address.
 *
 * The user must free the returned client IP after its use.
 * Compatible with IBM Tivoli call.
 *
 * Errors:
 * * LDAP_PARAM_ERROR - If the pb parameter is null.
 * * LDAP_OPERATIONS_ERROR - If the API encounters error processing the request.
 * * LDAP_NO_MEMORY - Failed to allocate required memory.
 */
int
slapi_get_client_ip(Slapi_PBlock *pb, char **clientIP)
{
	char *s = NULL;

	if(pb == NULL || pb->pb_conn == NULL) return(LDAP_PARAM_ERROR);
	if((s = (char *) slapi_ch_malloc(pb->pb_conn->c_peer_name.bv_len + 1)) == NULL) {
		return(LDAP_NO_MEMORY);
	}

	memcpy(s, pb->pb_conn->c_peer_name.bv_val, pb->pb_conn->c_peer_name.bv_len);

	s[pb->pb_conn->c_peer_name.bv_len] = 0;

	*clientIP = s;

	return(LDAP_SUCCESS);
}

/* Free previously allocated client IP address. */
void
slapi_free_client_ip(char **clientIP)
{
	slapi_ch_free((void **) clientIP);
}

#define MAX_HOSTNAME 512

char *
slapi_get_hostname( void ) 
{
	char		*hn = NULL;
	static int	been_here = 0;   
	static char	*static_hn = NULL;

	ldap_pvt_thread_mutex_lock( &slapi_hn_mutex );
	if ( !been_here ) {
		static_hn = (char *)slapi_ch_malloc( MAX_HOSTNAME );
		if ( static_hn == NULL) {
			slapi_log_error( SLAPI_LOG_FATAL, "slapi_get_hostname",
					"Cannot allocate memory for hostname\n" );
			static_hn = NULL;
			ldap_pvt_thread_mutex_unlock( &slapi_hn_mutex );

			return hn;
			
		} else { 
			if ( gethostname( static_hn, MAX_HOSTNAME ) != 0 ) {
				slapi_log_error( SLAPI_LOG_FATAL,
						"SLAPI",
						"can't get hostname\n" );
				slapi_ch_free( (void **)&static_hn );
				static_hn = NULL;
				ldap_pvt_thread_mutex_unlock( &slapi_hn_mutex );

				return hn;

			} else {
				been_here = 1;
			}
		}
	}
	ldap_pvt_thread_mutex_unlock( &slapi_hn_mutex );
	
	hn = ch_strdup( static_hn );

	return hn;
}

/*
 * FIXME: this should go in an appropriate header ...
 */
extern int slapi_int_log_error( int level, char *subsystem, char *fmt, va_list arglist );

int 
slapi_log_error(
	int		severity, 
	char		*subsystem, 
	char		*fmt, 
	... ) 
{
	int		rc = LDAP_SUCCESS;
	va_list		arglist;

	va_start( arglist, fmt );
	rc = slapi_int_log_error( severity, subsystem, fmt, arglist );
	va_end( arglist );

	return rc;
}


unsigned long
slapi_timer_current_time( void ) 
{
	static int	first_time = 1;
#if !defined (_WIN32)
	struct timeval	now;
	unsigned long	ret;

	ldap_pvt_thread_mutex_lock( &slapi_time_mutex );
	if (first_time) {
		first_time = 0;
		gettimeofday( &base_time, NULL );
	}
	gettimeofday( &now, NULL );
	ret = ( now.tv_sec  - base_time.tv_sec ) * 1000000 + 
			(now.tv_usec - base_time.tv_usec);
	ldap_pvt_thread_mutex_unlock( &slapi_time_mutex );

	return ret;

	/*
	 * Ain't it better?
	return (slap_get_time() - starttime) * 1000000;
	 */
#else /* _WIN32 */
	LARGE_INTEGER now;

	if ( first_time ) {
		first_time = 0;
		performance_counter_present = QueryPerformanceCounter( &base_time );
		QueryPerformanceFrequency( &performance_freq );
	}

	if ( !performance_counter_present )
	     return 0;

	QueryPerformanceCounter( &now );
	return (1000000*(now.QuadPart-base_time.QuadPart))/performance_freq.QuadPart;
#endif /* _WIN32 */
}

/*
 * FIXME ?
 */
unsigned long
slapi_timer_get_time( char *label ) 
{
	unsigned long start = slapi_timer_current_time();
	printf("%10ld %10d usec %s\n", start, 0, label);
	return start;
}

/*
 * FIXME ?
 */
void
slapi_timer_elapsed_time(
	char *label,
	unsigned long start ) 
{
	unsigned long stop = slapi_timer_current_time();
	printf ("%10ld %10ld usec %s\n", stop, stop - start, label);
}

void
slapi_free_search_results_internal( Slapi_PBlock *pb ) 
{
	Slapi_Entry	**entries;
	int		k = 0, nEnt = 0;

	slapi_pblock_get( pb, SLAPI_NENTRIES, &nEnt );
	slapi_pblock_get( pb, SLAPI_PLUGIN_INTOP_SEARCH_ENTRIES, &entries );
	if ( nEnt == 0 || entries == NULL ) {
		return;
	}

	for ( k = 0; k < nEnt; k++ ) {
		slapi_entry_free( entries[k] );
		entries[k] = NULL;
	}
	
	slapi_ch_free( (void **)&entries );
}

int slapi_is_connection_ssl( Slapi_PBlock *pb, int *isSSL )
{
	if ( pb == NULL )
		return LDAP_PARAM_ERROR;

	if ( pb->pb_conn == NULL )
		return LDAP_PARAM_ERROR;

#ifdef HAVE_TLS
	*isSSL = pb->pb_conn->c_is_tls;
#else
	*isSSL = 0;
#endif

	return LDAP_SUCCESS;
}

/*
 * DS 5.x compatability API follow
 */

int slapi_attr_get_flags( const Slapi_Attr *attr, unsigned long *flags )
{
	AttributeType *at;

	if ( attr == NULL )
		return LDAP_PARAM_ERROR;

	at = attr->a_desc->ad_type;

	*flags = SLAPI_ATTR_FLAG_STD_ATTR;

	if ( is_at_single_value( at ) )
		*flags |= SLAPI_ATTR_FLAG_SINGLE;
	if ( is_at_operational( at ) )
		*flags |= SLAPI_ATTR_FLAG_OPATTR;
	if ( is_at_obsolete( at ) )
		*flags |= SLAPI_ATTR_FLAG_OBSOLETE;
	if ( is_at_collective( at ) )
		*flags |= SLAPI_ATTR_FLAG_COLLECTIVE;
	if ( is_at_no_user_mod( at ) )
		*flags |= SLAPI_ATTR_FLAG_NOUSERMOD;

	return LDAP_SUCCESS;
}

int slapi_attr_flag_is_set( const Slapi_Attr *attr, unsigned long flag )
{
	unsigned long flags;

	if ( slapi_attr_get_flags( attr, &flags ) != 0 )
		return 0;
	return (flags & flag) ? 1 : 0;
}

Slapi_Attr *slapi_attr_new( void )
{
	Attribute *ad;

	ad = (Attribute  *)slapi_ch_calloc( 1, sizeof(*ad) );

	return ad;
}

Slapi_Attr *slapi_attr_init( Slapi_Attr *a, const char *type )
{
	const char *text;
	AttributeDescription *ad = NULL;

	if( slap_str2ad( type, &ad, &text ) != LDAP_SUCCESS ) {
		return NULL;
	}

	a->a_desc = ad;
	a->a_vals = NULL;
	a->a_nvals = NULL;
	a->a_next = NULL;
	a->a_flags = 0;

	return a;
}

void slapi_attr_free( Slapi_Attr **a )
{
	attr_free( *a );
	*a = NULL;
}

Slapi_Attr *slapi_attr_dup( const Slapi_Attr *attr )
{
	return attr_dup( (Slapi_Attr *)attr );
}

int slapi_attr_add_value( Slapi_Attr *a, const Slapi_Value *v )
{
	struct berval nval;
	struct berval *nvalp;
	int rc;
	AttributeDescription *desc = a->a_desc;

	if ( desc->ad_type->sat_equality &&
	     desc->ad_type->sat_equality->smr_normalize ) {
		rc = (*desc->ad_type->sat_equality->smr_normalize)(
			SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
			desc->ad_type->sat_syntax,
			desc->ad_type->sat_equality,
			(Slapi_Value *)v, &nval, NULL );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
		nvalp = &nval;
	} else {
		nvalp = NULL;
	}

	rc = attr_valadd( a, (Slapi_Value *)v, nvalp, 1 );

	if ( nvalp != NULL ) {
		slapi_ch_free_string( &nval.bv_val );
	}

	return rc;
}

int slapi_attr_type2plugin( const char *type, void **pi )
{
	*pi = NULL;

	return LDAP_OTHER;
}

int slapi_attr_get_type( const Slapi_Attr *attr, char **type )
{
	if ( attr == NULL ) {
		return LDAP_PARAM_ERROR;
	}

	*type = attr->a_desc->ad_cname.bv_val;

	return LDAP_SUCCESS;
}

int slapi_attr_get_oid_copy( const Slapi_Attr *attr, char **oidp )
{
	if ( attr == NULL ) {
		return LDAP_PARAM_ERROR;
	}
	*oidp = attr->a_desc->ad_type->sat_oid;

	return LDAP_SUCCESS;
}

int slapi_attr_value_cmp( const Slapi_Attr *a, const struct berval *v1, const struct berval *v2 )
{
	MatchingRule *mr;
	int ret;
	int rc;
	const char *text;

	mr = a->a_desc->ad_type->sat_equality;
	rc = value_match( &ret, a->a_desc, mr,
			SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
		(struct berval *)v1, (void *)v2, &text );
	if ( rc != LDAP_SUCCESS ) 
		return -1;

	return ( ret == 0 ) ? 0 : -1;
}

int slapi_attr_value_find( const Slapi_Attr *a, struct berval *v )
{
	int rc;

	if ( a ->a_vals == NULL ) {
		return -1;
	}
	rc = attr_valfind( (Attribute *)a, SLAP_MR_VALUE_OF_ASSERTION_SYNTAX, v,
		NULL, NULL );
	return rc == 0 ? 0 : -1;
}

int slapi_attr_type_cmp( const char *t1, const char *t2, int opt )
{
	AttributeDescription *a1 = NULL;
	AttributeDescription *a2 = NULL;
	const char *text;
	int ret;

	if ( slap_str2ad( t1, &a1, &text ) != LDAP_SUCCESS ) {
		return -1;
	}

	if ( slap_str2ad( t2, &a2, &text ) != LDAP_SUCCESS ) {
		return 1;
	}

#define ad_base_cmp(l,r) (((l)->ad_type->sat_cname.bv_len < (r)->ad_type->sat_cname.bv_len) \
	? -1 : (((l)->ad_type->sat_cname.bv_len > (r)->ad_type->sat_cname.bv_len) \
		? 1 : strcasecmp((l)->ad_type->sat_cname.bv_val, (r)->ad_type->sat_cname.bv_val )))

	switch ( opt ) {
	case SLAPI_TYPE_CMP_EXACT:
		ret = ad_cmp( a1, a2 );
		break;
	case SLAPI_TYPE_CMP_BASE:
		ret = ad_base_cmp( a1, a2 );
		break;
	case SLAPI_TYPE_CMP_SUBTYPE:
		ret = is_ad_subtype( a2, a2 );
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int slapi_attr_types_equivalent( const char *t1, const char *t2 )
{
	return ( slapi_attr_type_cmp( t1, t2, SLAPI_TYPE_CMP_EXACT ) == 0 );
}

int slapi_attr_first_value( Slapi_Attr *a, Slapi_Value **v )
{
	return slapi_valueset_first_value( &a->a_vals, v );
}

int slapi_attr_next_value( Slapi_Attr *a, int hint, Slapi_Value **v )
{
	return slapi_valueset_next_value( &a->a_vals, hint, v );
}

int slapi_attr_get_numvalues( const Slapi_Attr *a, int *numValues )
{
	*numValues = slapi_valueset_count( &a->a_vals );

	return 0;
}

int slapi_attr_get_valueset( const Slapi_Attr *a, Slapi_ValueSet **vs )
{
	*vs = &((Slapi_Attr *)a)->a_vals;

	return 0;
}

int slapi_attr_get_bervals_copy( Slapi_Attr *a, struct berval ***vals )
{
	return slapi_attr_get_values( a, vals );
}

char *slapi_attr_syntax_normalize( const char *s )
{
	AttributeDescription *ad = NULL;
	const char *text;

	if ( slap_str2ad( s, &ad, &text ) != LDAP_SUCCESS ) {
		return NULL;
	}

	return ad->ad_cname.bv_val;
}

Slapi_Value *slapi_value_new( void )
{
	struct berval *bv;

	bv = (struct berval *)slapi_ch_malloc( sizeof(*bv) );

	return bv;
}

Slapi_Value *slapi_value_new_berval(const struct berval *bval)
{
	return ber_dupbv( NULL, (struct berval *)bval );
}

Slapi_Value *slapi_value_new_value(const Slapi_Value *v)
{
	return slapi_value_new_berval( v );
}

Slapi_Value *slapi_value_new_string(const char *s)
{
	struct berval bv;

	bv.bv_val = (char *)s;
	bv.bv_len = strlen( s );

	return slapi_value_new_berval( &bv );
}

Slapi_Value *slapi_value_init(Slapi_Value *val)
{
	val->bv_val = NULL;
	val->bv_len = 0;

	return val;
}

Slapi_Value *slapi_value_init_berval(Slapi_Value *v, struct berval *bval)
{
	return ber_dupbv( v, bval );
}

Slapi_Value *slapi_value_init_string(Slapi_Value *v, const char *s)
{
	v->bv_val = slapi_ch_strdup( s );
	v->bv_len = strlen( s );

	return v;
}

Slapi_Value *slapi_value_dup(const Slapi_Value *v)
{
	return slapi_value_new_value( v );
}

void slapi_value_free(Slapi_Value **value)
{
	if ( value == NULL ) {
		return;
	}

	if ( (*value) != NULL ) {
		slapi_ch_free( (void **)&(*value)->bv_val );
		slapi_ch_free( (void **)value );
	}
}

const struct berval *slapi_value_get_berval( const Slapi_Value *value )
{
	return value;
}

Slapi_Value *slapi_value_set_berval( Slapi_Value *value, const struct berval *bval )
{
	if ( value == NULL ) {
		return NULL;
	}
	if ( value->bv_val != NULL ) {
		slapi_ch_free( (void **)&value->bv_val );
	}
	slapi_value_init_berval( value, (struct berval *)bval );

	return value;
}

Slapi_Value *slapi_value_set_value( Slapi_Value *value, const Slapi_Value *vfrom)
{
	if ( value == NULL ) {
		return NULL;
	}
	return slapi_value_set_berval( value, vfrom );
}

Slapi_Value *slapi_value_set( Slapi_Value *value, void *val, unsigned long len)
{
	if ( value == NULL ) {
		return NULL;
	}
	if ( value->bv_val != NULL ) {
		slapi_ch_free( (void **)&value->bv_val );
	}
	value->bv_val = slapi_ch_malloc( len );
	value->bv_len = len;
	AC_MEMCPY( value->bv_val, val, len );

	return value;
}

int slapi_value_set_string(Slapi_Value *value, const char *strVal)
{
	if ( value == NULL ) {
		return -1;
	}
	slapi_value_set( value, (void *)strVal, strlen( strVal ) );
	return 0;
}

int slapi_value_set_int(Slapi_Value *value, int intVal)
{
	char buf[64];

	snprintf( buf, sizeof( buf ), "%d", intVal );

	return slapi_value_set_string( value, buf );
}

const char *slapi_value_get_string(const Slapi_Value *value)
{
	if ( value == NULL ) return NULL;
	if ( value->bv_val == NULL ) return NULL;
	if ( !checkBVString( value ) ) return NULL;

	return value->bv_val;
}

int slapi_value_get_int(const Slapi_Value *value)
{
	if ( value == NULL ) return 0;
	if ( value->bv_val == NULL ) return 0;
	if ( !checkBVString( value ) ) return 0;

	return (int)strtol( value->bv_val, NULL, 10 );
}

unsigned int slapi_value_get_uint(const Slapi_Value *value)
{
	if ( value == NULL ) return 0;
	if ( value->bv_val == NULL ) return 0;
	if ( !checkBVString( value ) ) return 0;

	return (unsigned int)strtoul( value->bv_val, NULL, 10 );
}

long slapi_value_get_long(const Slapi_Value *value)
{
	if ( value == NULL ) return 0;
	if ( value->bv_val == NULL ) return 0;
	if ( !checkBVString( value ) ) return 0;

	return strtol( value->bv_val, NULL, 10 );
}

unsigned long slapi_value_get_ulong(const Slapi_Value *value)
{
	if ( value == NULL ) return 0;
	if ( value->bv_val == NULL ) return 0;
	if ( !checkBVString( value ) ) return 0;

	return strtoul( value->bv_val, NULL, 10 );
}

size_t slapi_value_get_length(const Slapi_Value *value)
{
	if ( value == NULL )
		return 0;

	return (size_t) value->bv_len;
}

int slapi_value_compare(const Slapi_Attr *a, const Slapi_Value *v1, const Slapi_Value *v2)
{
	return slapi_attr_value_cmp( a, v1, v2 );
}

/* A ValueSet is a container for a BerVarray. */
Slapi_ValueSet *slapi_valueset_new( void )
{
	Slapi_ValueSet *vs;

	vs = (Slapi_ValueSet *)slapi_ch_malloc( sizeof( *vs ) );
	*vs = NULL;

	return vs;
}

void slapi_valueset_free(Slapi_ValueSet *vs)
{
	if ( vs != NULL ) {
		BerVarray vp = *vs;

		ber_bvarray_free( vp );
		vp = NULL;

		slapi_ch_free( (void **)&vp );
	}
}

void slapi_valueset_init(Slapi_ValueSet *vs)
{
	if ( vs != NULL && *vs == NULL ) {
		*vs = (Slapi_ValueSet)slapi_ch_calloc( 1, sizeof(struct berval) );
		(*vs)->bv_val = NULL;
		(*vs)->bv_len = 0;
	}
}

void slapi_valueset_done(Slapi_ValueSet *vs)
{
	BerVarray vp;

	if ( vs == NULL )
		return;

	for ( vp = *vs; vp->bv_val != NULL; vp++ ) {
		vp->bv_len = 0;
		slapi_ch_free( (void **)&vp->bv_val );
	}
	/* but don't free *vs or vs */
}

void slapi_valueset_add_value(Slapi_ValueSet *vs, const Slapi_Value *addval)
{
	struct berval bv;

	ber_dupbv( &bv, (Slapi_Value *)addval );
	ber_bvarray_add( vs, &bv );
}

int slapi_valueset_first_value( Slapi_ValueSet *vs, Slapi_Value **v )
{
	return slapi_valueset_next_value( vs, 0, v );
}

int slapi_valueset_next_value( Slapi_ValueSet *vs, int index, Slapi_Value **v)
{
	int i;
	BerVarray vp;

	if ( vs == NULL )
		return -1;

	vp = *vs;

	for ( i = 0; vp[i].bv_val != NULL; i++ ) {
		if ( i == index ) {
			*v = &vp[i];
			return index + 1;
		}
	}

	return -1;
}

int slapi_valueset_count( const Slapi_ValueSet *vs )
{
	int i;
	BerVarray vp;

	if ( vs == NULL )
		return 0;

	vp = *vs;

	if ( vp == NULL )
		return 0;

	for ( i = 0; vp[i].bv_val != NULL; i++ )
		;

	return i;

}

void slapi_valueset_set_valueset(Slapi_ValueSet *vs1, const Slapi_ValueSet *vs2)
{
	BerVarray vp;

	for ( vp = *vs2; vp->bv_val != NULL; vp++ ) {
		slapi_valueset_add_value( vs1, vp );
	}
}

int slapi_access_allowed( Slapi_PBlock *pb, Slapi_Entry *e, char *attr,
	struct berval *val, int access )
{
	int rc;
	slap_access_t slap_access;
	AttributeDescription *ad = NULL;
	const char *text;

	rc = slap_str2ad( attr, &ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	/*
	 * Whilst the SLAPI access types are arranged as a bitmask, the
	 * documentation indicates that they are to be used separately.
	 */
	switch ( access & SLAPI_ACL_ALL ) {
	case SLAPI_ACL_COMPARE:
		slap_access = ACL_COMPARE;
		break;
	case SLAPI_ACL_SEARCH:
		slap_access = ACL_SEARCH;
		break;
	case SLAPI_ACL_READ:
		slap_access = ACL_READ;
		break;
	case SLAPI_ACL_WRITE:
		slap_access = ACL_WRITE;
		break;
	case SLAPI_ACL_DELETE:
		slap_access = ACL_WDEL;
		break;
	case SLAPI_ACL_ADD:
		slap_access = ACL_WADD;
		break;
	case SLAPI_ACL_SELF:  /* not documented */
	case SLAPI_ACL_PROXY: /* not documented */
	default:
		return LDAP_INSUFFICIENT_ACCESS;
		break;
	}

	assert( pb->pb_op != NULL );

	if ( access_allowed( pb->pb_op, e, ad, val, slap_access, NULL ) ) {
		return LDAP_SUCCESS;
	}

	return LDAP_INSUFFICIENT_ACCESS;
}

int slapi_acl_check_mods(Slapi_PBlock *pb, Slapi_Entry *e, LDAPMod **mods, char **errbuf)
{
	int rc = LDAP_SUCCESS;
	Modifications *ml;

	if ( pb == NULL || pb->pb_op == NULL )
		return LDAP_PARAM_ERROR;

	ml = slapi_int_ldapmods2modifications( pb->pb_op, mods );
	if ( ml == NULL ) {
		return LDAP_OTHER;
	}

	if ( rc == LDAP_SUCCESS ) {
		rc = acl_check_modlist( pb->pb_op, e, ml ) ? LDAP_SUCCESS : LDAP_INSUFFICIENT_ACCESS;
	}

	slap_mods_free( ml, 1 );

	return rc;
}

/*
 * Synthesise an LDAPMod array from a Modifications list to pass
 * to SLAPI.
 */
LDAPMod **slapi_int_modifications2ldapmods( Modifications *modlist )
{
	Modifications *ml;
	LDAPMod **mods, *modp;
	int i, j;

	for( i = 0, ml = modlist; ml != NULL; i++, ml = ml->sml_next )
		;

	mods = (LDAPMod **)slapi_ch_malloc( (i + 1) * sizeof(LDAPMod *) );

	for( i = 0, ml = modlist; ml != NULL; ml = ml->sml_next ) {
		mods[i] = (LDAPMod *)slapi_ch_malloc( sizeof(LDAPMod) );
		modp = mods[i];
		modp->mod_op = ml->sml_op | LDAP_MOD_BVALUES;
		if ( BER_BVISNULL( &ml->sml_type ) ) {
			/* may happen for internally generated mods */
			assert( ml->sml_desc != NULL );
			modp->mod_type = slapi_ch_strdup( ml->sml_desc->ad_cname.bv_val );
		} else {
			modp->mod_type = slapi_ch_strdup( ml->sml_type.bv_val );
		}

		if ( ml->sml_values != NULL ) {
			for( j = 0; ml->sml_values[j].bv_val != NULL; j++ )
				;
			modp->mod_bvalues = (struct berval **)slapi_ch_malloc( (j + 1) *
				sizeof(struct berval *) );
			for( j = 0; ml->sml_values[j].bv_val != NULL; j++ ) {
				modp->mod_bvalues[j] = (struct berval *)slapi_ch_malloc(
						sizeof(struct berval) );
				ber_dupbv( modp->mod_bvalues[j], &ml->sml_values[j] );
			}
			modp->mod_bvalues[j] = NULL;
		} else {
			modp->mod_bvalues = NULL;
		}
		i++;
	}

	mods[i] = NULL;

	return mods;
}

/*
 * Convert a potentially modified array of LDAPMods back to a
 * Modification list. Unfortunately the values need to be
 * duplicated because slap_mods_check() will try to free them
 * before prettying (and we can't easily get out of calling
 * slap_mods_check() because we need normalized values).
 */
Modifications *slapi_int_ldapmods2modifications ( Operation *op, LDAPMod **mods )
{
	Modifications *modlist = NULL, **modtail;
	LDAPMod **modp;
	char textbuf[SLAP_TEXT_BUFLEN];
	const char *text;

	if ( mods == NULL ) {
		return NULL;
	}

	modtail = &modlist;

	for ( modp = mods; *modp != NULL; modp++ ) {
		Modifications *mod;
		LDAPMod *lmod = *modp;
		int i;
		const char *text;
		AttributeDescription *ad = NULL;

		if ( slap_str2ad( lmod->mod_type, &ad, &text ) != LDAP_SUCCESS ) {
			continue;
		}

		mod = (Modifications *) slapi_ch_malloc( sizeof(Modifications) );
		mod->sml_op = lmod->mod_op & ~(LDAP_MOD_BVALUES);
		mod->sml_flags = 0;
		mod->sml_type = ad->ad_cname;
		mod->sml_desc = ad;
		mod->sml_next = NULL;

		i = 0;
		if ( lmod->mod_op & LDAP_MOD_BVALUES ) {
			if ( lmod->mod_bvalues != NULL ) {
				while ( lmod->mod_bvalues[i] != NULL )
					i++;
			}
		} else {
			if ( lmod->mod_values != NULL ) {
				while ( lmod->mod_values[i] != NULL )
					i++;
			}
		}
		mod->sml_numvals = i;

		if ( i == 0 ) {
			mod->sml_values = NULL;
		} else {
			mod->sml_values = (BerVarray) slapi_ch_malloc( (i + 1) * sizeof(struct berval) );

			/* NB: This implicitly trusts a plugin to return valid modifications. */
			if ( lmod->mod_op & LDAP_MOD_BVALUES ) {
				for ( i = 0; lmod->mod_bvalues[i] != NULL; i++ ) {
					ber_dupbv( &mod->sml_values[i], lmod->mod_bvalues[i] );
				}
			} else {
				for ( i = 0; lmod->mod_values[i] != NULL; i++ ) {
					mod->sml_values[i].bv_val = slapi_ch_strdup( lmod->mod_values[i] );
					mod->sml_values[i].bv_len = strlen( lmod->mod_values[i] );
				}
			}
			mod->sml_values[i].bv_val = NULL;
			mod->sml_values[i].bv_len = 0;
		}
		mod->sml_nvalues = NULL;

		*modtail = mod;
		modtail = &mod->sml_next;
	}

	if ( slap_mods_check( op, modlist, &text, textbuf, sizeof( textbuf ), NULL ) != LDAP_SUCCESS ) {
		slap_mods_free( modlist, 1 );
		modlist = NULL;
	}

	return modlist;
}

/*
 * Sun ONE DS 5.x computed attribute support. Computed attributes
 * allow for dynamically generated operational attributes, a very
 * useful thing indeed.
 */

/*
 * For some reason Sun don't use the normal plugin mechanism
 * registration path to register an "evaluator" function (an
 * "evaluator" is responsible for adding computed attributes;
 * the nomenclature is somewhat confusing).
 *
 * As such slapi_compute_add_evaluator() registers the 
 * function directly.
 */
int slapi_compute_add_evaluator(slapi_compute_callback_t function)
{
	Slapi_PBlock *pPlugin = NULL;
	int rc;
	int type = SLAPI_PLUGIN_OBJECT;

	pPlugin = slapi_pblock_new();
	if ( pPlugin == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto done;
	}

	rc = slapi_pblock_set( pPlugin, SLAPI_PLUGIN_TYPE, (void *)&type );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = slapi_pblock_set( pPlugin, SLAPI_PLUGIN_COMPUTE_EVALUATOR_FN, (void *)function );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = slapi_int_register_plugin( frontendDB, pPlugin );
	if ( rc != 0 ) {
		rc = LDAP_OTHER;
		goto done;
	}

done:
	if ( rc != LDAP_SUCCESS ) {
		if ( pPlugin != NULL ) {
			slapi_pblock_destroy( pPlugin );
		}
		return -1;
	}

	return 0;
}

/*
 * See notes above regarding slapi_compute_add_evaluator().
 */
int slapi_compute_add_search_rewriter(slapi_search_rewrite_callback_t function)
{
	Slapi_PBlock *pPlugin = NULL;
	int rc;
	int type = SLAPI_PLUGIN_OBJECT;

	pPlugin = slapi_pblock_new();
	if ( pPlugin == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto done;
	}

	rc = slapi_pblock_set( pPlugin, SLAPI_PLUGIN_TYPE, (void *)&type );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = slapi_pblock_set( pPlugin, SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN, (void *)function );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = slapi_int_register_plugin( frontendDB, pPlugin );
	if ( rc != 0 ) {
		rc = LDAP_OTHER;
		goto done;
	}

done:
	if ( rc != LDAP_SUCCESS ) {
		if ( pPlugin != NULL ) {
			slapi_pblock_destroy( pPlugin );
		}
		return -1;
	}

	return 0;
}

/*
 * Call compute evaluators
 */
int compute_evaluator(computed_attr_context *c, char *type, Slapi_Entry *e, slapi_compute_output_t outputfn)
{
	int rc = 0;
	slapi_compute_callback_t *pGetPlugin, *tmpPlugin;

	rc = slapi_int_get_plugins( frontendDB, SLAPI_PLUGIN_COMPUTE_EVALUATOR_FN, (SLAPI_FUNC **)&tmpPlugin );
	if ( rc != LDAP_SUCCESS || tmpPlugin == NULL ) {
		/* Nothing to do; front-end should ignore. */
		return 0;
	}

	for ( pGetPlugin = tmpPlugin; *pGetPlugin != NULL; pGetPlugin++ ) {
		/*
		 * -1: no attribute matched requested type
		 *  0: one attribute matched
		 * >0: error happened
		 */
		rc = (*pGetPlugin)( c, type, e, outputfn );
		if ( rc > 0 ) {
			break;
		}
	}

	slapi_ch_free( (void **)&tmpPlugin );

	return rc;
}

int
compute_rewrite_search_filter( Slapi_PBlock *pb )
{
	if ( pb == NULL || pb->pb_op == NULL )
		return LDAP_PARAM_ERROR;

	return slapi_int_call_plugins( pb->pb_op->o_bd, SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN, pb );
}

/*
 * New API to provide the plugin with access to the search
 * pblock. Have informed Sun DS team.
 */
int
slapi_x_compute_get_pblock(computed_attr_context *c, Slapi_PBlock **pb)
{
	if ( c == NULL )
		return -1;

	if ( c->cac_pb == NULL )
		return -1;

	*pb = c->cac_pb;

	return 0;
}

Slapi_Mutex *slapi_new_mutex( void )
{
	Slapi_Mutex *m;

	m = (Slapi_Mutex *)slapi_ch_malloc( sizeof(*m) );
	if ( ldap_pvt_thread_mutex_init( &m->mutex ) != 0 ) {
		slapi_ch_free( (void **)&m );
		return NULL;
	}

	return m;
}

void slapi_destroy_mutex( Slapi_Mutex *mutex )
{
	if ( mutex != NULL ) {
		ldap_pvt_thread_mutex_destroy( &mutex->mutex );
		slapi_ch_free( (void **)&mutex);
	}
}

void slapi_lock_mutex( Slapi_Mutex *mutex )
{
	ldap_pvt_thread_mutex_lock( &mutex->mutex );
}

int slapi_unlock_mutex( Slapi_Mutex *mutex )
{
	return ldap_pvt_thread_mutex_unlock( &mutex->mutex );
}

Slapi_CondVar *slapi_new_condvar( Slapi_Mutex *mutex )
{
	Slapi_CondVar *cv;

	if ( mutex == NULL ) {
		return NULL;
	}

	cv = (Slapi_CondVar *)slapi_ch_malloc( sizeof(*cv) );
	if ( ldap_pvt_thread_cond_init( &cv->cond ) != 0 ) {
		slapi_ch_free( (void **)&cv );
		return NULL;
	}

	cv->mutex = mutex->mutex;

	return cv;
}

void slapi_destroy_condvar( Slapi_CondVar *cvar )
{
	if ( cvar != NULL ) {
		ldap_pvt_thread_cond_destroy( &cvar->cond );
		slapi_ch_free( (void **)&cvar );
	}
}

int slapi_wait_condvar( Slapi_CondVar *cvar, struct timeval *timeout )
{
	if ( cvar == NULL ) {
		return -1;
	}

	return ldap_pvt_thread_cond_wait( &cvar->cond, &cvar->mutex );
}

int slapi_notify_condvar( Slapi_CondVar *cvar, int notify_all )
{
	if ( cvar == NULL ) {
		return -1;
	}

	if ( notify_all ) {
		return ldap_pvt_thread_cond_broadcast( &cvar->cond );
	}

	return ldap_pvt_thread_cond_signal( &cvar->cond );
}

int slapi_int_access_allowed( Operation *op,
	Entry *entry,
	AttributeDescription *desc,
	struct berval *val,
	slap_access_t access,
	AccessControlState *state )
{
	int rc, slap_access = 0;
	slapi_acl_callback_t *pGetPlugin, *tmpPlugin;
	Slapi_PBlock *pb;

	pb = SLAPI_OPERATION_PBLOCK( op );
	if ( pb == NULL ) {
		/* internal operation */
		return 1;
	}

	switch ( access ) {
	case ACL_COMPARE:
                slap_access |= SLAPI_ACL_COMPARE;
		break;
	case ACL_SEARCH:
		slap_access |= SLAPI_ACL_SEARCH;
		break;
	case ACL_READ:
		slap_access |= SLAPI_ACL_READ;
		break;
	case ACL_WRITE:
		slap_access |= SLAPI_ACL_WRITE;
		break;
	case ACL_WDEL:
		slap_access |= SLAPI_ACL_DELETE;
		break;
	case ACL_WADD:
		slap_access |= SLAPI_ACL_ADD;
		break;
	default:
		break;
        }

	rc = slapi_int_get_plugins( frontendDB, SLAPI_PLUGIN_ACL_ALLOW_ACCESS, (SLAPI_FUNC **)&tmpPlugin );
	if ( rc != LDAP_SUCCESS || tmpPlugin == NULL ) {
		/* nothing to do; allowed access */
		return 1;
	}

	rc = 1; /* default allow policy */

	for ( pGetPlugin = tmpPlugin; *pGetPlugin != NULL; pGetPlugin++ ) {
		/*
		 * 0	access denied
		 * 1	access granted
		 */
		rc = (*pGetPlugin)( pb, entry, desc->ad_cname.bv_val,
				    val, slap_access, (void *)state );
		if ( rc == 0 ) {
			break;
		}
	}

	slapi_ch_free( (void **)&tmpPlugin );

	return rc;
}

/*
 * There is no documentation for this.
 */
int slapi_rdn2typeval( char *rdn, char **type, struct berval *bv )
{
	LDAPRDN lrdn;
	LDAPAVA *ava;
	int rc;
	char *p;

	*type = NULL;

	bv->bv_len = 0;
	bv->bv_val = NULL;

	rc = ldap_str2rdn( rdn, &lrdn, &p, LDAP_DN_FORMAT_LDAPV3 );
	if ( rc != LDAP_SUCCESS ) {
		return -1;
	}

	if ( lrdn[1] != NULL ) {
		return -1; /* not single valued */
	}

	ava = lrdn[0];

	*type = slapi_ch_strdup( ava->la_attr.bv_val );
	ber_dupbv( bv, &ava->la_value );

	ldap_rdnfree(lrdn);

	return 0;
}

char *slapi_dn_plus_rdn( const char *dn, const char *rdn )
{
	struct berval new_dn, parent_dn, newrdn;

	new_dn.bv_val = NULL;

	parent_dn.bv_val = (char *)dn;
	parent_dn.bv_len = strlen( dn );

	newrdn.bv_val = (char *)rdn;
	newrdn.bv_len = strlen( rdn );

	build_new_dn( &new_dn, &parent_dn, &newrdn, NULL );

	return new_dn.bv_val;
}

int slapi_entry_schema_check( Slapi_PBlock *pb, Slapi_Entry *e )
{
	Backend *be_orig;
	const char *text;
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;
	int rc = LDAP_SUCCESS;

	PBLOCK_ASSERT_OP( pb, 0 );

	be_orig = pb->pb_op->o_bd;

	pb->pb_op->o_bd = select_backend( &e->e_nname, 0 );
	if ( pb->pb_op->o_bd != NULL ) {
		rc = entry_schema_check( pb->pb_op, e, NULL, 0, 0, NULL,
			&text, textbuf, textlen );
	}
	pb->pb_op->o_bd = be_orig;

	return ( rc == LDAP_SUCCESS ) ? 0 : 1;
}

int slapi_entry_rdn_values_present( const Slapi_Entry *e )
{
	LDAPDN dn;
	int rc;
	int i = 0, match = 0;

	rc = ldap_bv2dn( &((Entry *)e)->e_name, &dn, LDAP_DN_FORMAT_LDAPV3 );
	if ( rc != LDAP_SUCCESS ) {
		return 0;
	}

	if ( dn[0] != NULL ) {
		LDAPRDN rdn = dn[0];

		for ( i = 0; rdn[i] != NULL; i++ ) {
			LDAPAVA *ava = &rdn[0][i];
			Slapi_Attr *a = NULL;

			if ( slapi_entry_attr_find( (Slapi_Entry *)e, ava->la_attr.bv_val, &a ) == 0 &&
			     slapi_attr_value_find( a, &ava->la_value ) == 0 )
				match++;
		}
	}

	ldap_dnfree( dn );

	return ( i == match );
}

int slapi_entry_add_rdn_values( Slapi_Entry *e )
{
	LDAPDN dn;
	int i, rc;

	rc = ldap_bv2dn( &e->e_name, &dn, LDAP_DN_FORMAT_LDAPV3 );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( dn[0] != NULL ) {
		LDAPRDN rdn = dn[0];
		struct berval *vals[2];

		for ( i = 0; rdn[i] != NULL; i++ ) {
			LDAPAVA *ava = &rdn[0][i];
			Slapi_Attr *a = NULL;

			if ( slapi_entry_attr_find( e, ava->la_attr.bv_val, &a ) == 0 &&
			     slapi_attr_value_find( a, &ava->la_value ) == 0 )
				continue;

			vals[0] = &ava->la_value;
			vals[1] = NULL;

			slapi_entry_attr_merge( e, ava->la_attr.bv_val, vals );
		}
	}

	ldap_dnfree( dn );

	return LDAP_SUCCESS;
}

const char *slapi_entry_get_uniqueid( const Slapi_Entry *e )
{
	Attribute *attr;

	attr = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
	if ( attr == NULL ) {
		return NULL;
	}

	if ( attr->a_vals != NULL && attr->a_vals[0].bv_len != 0 ) {
		return slapi_value_get_string( &attr->a_vals[0] );
	}

	return NULL;
}

void slapi_entry_set_uniqueid( Slapi_Entry *e, char *uniqueid )
{
	struct berval bv;

	attr_delete ( &e->e_attrs, slap_schema.si_ad_entryUUID );

	bv.bv_val = uniqueid;
	bv.bv_len = strlen( uniqueid );
	attr_merge_normalize_one( e, slap_schema.si_ad_entryUUID, &bv, NULL );
}

LDAP *slapi_ldap_init( char *ldaphost, int ldapport, int secure, int shared )
{
	LDAP *ld;
	char *url;
	size_t size;
	int rc;

	size = sizeof("ldap:///");
	if ( secure ) {
		size++;
	}
	size += strlen( ldaphost );
	if ( ldapport != 0 ) {
		size += 32;
	}

	url = slapi_ch_malloc( size );

	if ( ldapport != 0 ) {
		rc = snprintf( url, size, "ldap%s://%s:%d/", ( secure ? "s" : "" ), ldaphost, ldapport );
	} else {
		rc = snprintf( url, size, "ldap%s://%s/", ( secure ? "s" : "" ), ldaphost );
	}

	if ( rc > 0 && (size_t) rc < size ) {
		rc = ldap_initialize( &ld, url );
	} else {
		ld = NULL;
	}

	slapi_ch_free_string( &url );

	return ( rc == LDAP_SUCCESS ) ? ld : NULL;
}

void slapi_ldap_unbind( LDAP *ld )
{
	ldap_unbind_ext_s( ld, NULL, NULL );
}

int slapi_x_backend_get_flags( const Slapi_Backend *be, unsigned long *flags )
{
	if ( be == NULL )
		return LDAP_PARAM_ERROR;

	*flags = SLAP_DBFLAGS(be);

	return LDAP_SUCCESS;
}

int
slapi_int_count_controls( LDAPControl **ctrls )
{
	size_t i;

	if ( ctrls == NULL )
		return 0;

	for ( i = 0; ctrls[i] != NULL; i++ )
		;

	return i;
}

int
slapi_op_abandoned( Slapi_PBlock *pb )
{
	if ( pb->pb_op == NULL )
		return 0;

	return ( pb->pb_op->o_abandon );
}

char *
slapi_op_type_to_string(unsigned long type)
{
	char *str;

	switch (type) {
	case SLAPI_OPERATION_BIND:
		str = "bind";
		break;
	case SLAPI_OPERATION_UNBIND:
		str = "unbind";
		break;
	case SLAPI_OPERATION_SEARCH:
		str = "search";
		break;
	case SLAPI_OPERATION_MODIFY:
		str = "modify";
		break;
	case SLAPI_OPERATION_ADD:
		str = "add";
		break;
	case SLAPI_OPERATION_DELETE:
		str = "delete";
		break;
	case SLAPI_OPERATION_MODDN:
		str = "modrdn";
		break;
	case SLAPI_OPERATION_COMPARE:
		str = "compare";
		break;
	case SLAPI_OPERATION_ABANDON:
		str = "abandon";
		break;
	case SLAPI_OPERATION_EXTENDED:
		str = "extended";
		break;
	default:
		str = "unknown operation type";
		break;
	}
	return str;
}

unsigned long
slapi_op_get_type(Slapi_Operation * op)
{
	unsigned long type;

	switch ( op->o_tag ) {
	case LDAP_REQ_BIND:
		type = SLAPI_OPERATION_BIND;
		break;
	case LDAP_REQ_UNBIND:
		type = SLAPI_OPERATION_UNBIND;
		break;
	case LDAP_REQ_SEARCH:
		type = SLAPI_OPERATION_SEARCH;
		break;
	case LDAP_REQ_MODIFY:
		type = SLAPI_OPERATION_MODIFY;
		break;
	case LDAP_REQ_ADD:
		type = SLAPI_OPERATION_ADD;
		break;
	case LDAP_REQ_DELETE:
		type = SLAPI_OPERATION_DELETE;
		break;
	case LDAP_REQ_MODRDN:
		type = SLAPI_OPERATION_MODDN;
		break;
	case LDAP_REQ_COMPARE:
		type = SLAPI_OPERATION_COMPARE;
		break;
	case LDAP_REQ_ABANDON:
		type = SLAPI_OPERATION_ABANDON;
		break;
	case LDAP_REQ_EXTENDED:
		type = SLAPI_OPERATION_EXTENDED;
		break;
	default:
		type = SLAPI_OPERATION_NONE;
		break;
	}
	return type;
}

void slapi_be_set_readonly( Slapi_Backend *be, int readonly )
{
	if ( be == NULL )
		return;

	if ( readonly )
		be->be_restrictops |= SLAP_RESTRICT_OP_WRITES;
	else
		be->be_restrictops &= ~(SLAP_RESTRICT_OP_WRITES);
}

int slapi_be_get_readonly( Slapi_Backend *be )
{
	if ( be == NULL )
		return 0;

	return ( (be->be_restrictops & SLAP_RESTRICT_OP_WRITES) == SLAP_RESTRICT_OP_WRITES );
}

const char *slapi_x_be_get_updatedn( Slapi_Backend *be )
{
	if ( be == NULL )
		return NULL;

	return be->be_update_ndn.bv_val;
}

Slapi_Backend *slapi_be_select( const Slapi_DN *sdn )
{
	Slapi_Backend *be;

	slapi_sdn_get_ndn( sdn );

	be = select_backend( (struct berval *)&sdn->ndn, 0 );

	return be;
}

#if 0
void
slapi_operation_set_flag(Slapi_Operation *op, unsigned long flag)
{
}

void
slapi_operation_clear_flag(Slapi_Operation *op, unsigned long flag)
{
}

int
slapi_operation_is_flag_set(Slapi_Operation *op, unsigned long flag)
{
}
#endif

#endif /* LDAP_SLAPI */

