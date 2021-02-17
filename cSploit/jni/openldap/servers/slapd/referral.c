/* referral.c - muck with referrals */
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

#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "slap.h"

/*
 * This routine generates the DN appropriate to return in
 * an LDAP referral.
 */
static char * referral_dn_muck(
	const char * refDN,
	struct berval * baseDN,
	struct berval * targetDN )
{
	int rc;
	struct berval bvin;
	struct berval nrefDN = BER_BVNULL;
	struct berval nbaseDN = BER_BVNULL;
	struct berval ntargetDN = BER_BVNULL;

	if( !baseDN ) {
		/* no base, return target */
		return targetDN ? ch_strdup( targetDN->bv_val ) : NULL;
	}

	if( refDN ) {
		bvin.bv_val = (char *)refDN;
		bvin.bv_len = strlen( refDN );

		rc = dnPretty( NULL, &bvin, &nrefDN, NULL );
		if( rc != LDAP_SUCCESS ) {
			/* Invalid refDN */
			return NULL;
		}
	}

	if( !targetDN ) {
		/* continuation reference
		 *	if refDN present return refDN
		 *  else return baseDN
		 */
		return nrefDN.bv_len ? nrefDN.bv_val : ch_strdup( baseDN->bv_val );
	}

	rc = dnPretty( NULL, targetDN, &ntargetDN, NULL );
	if( rc != LDAP_SUCCESS ) {
		/* Invalid targetDN */
		ch_free( nrefDN.bv_val );
		return NULL;
	}

	if( nrefDN.bv_len ) {
		rc = dnPretty( NULL, baseDN, &nbaseDN, NULL );
		if( rc != LDAP_SUCCESS ) {
			/* Invalid baseDN */
			ch_free( nrefDN.bv_val );
			ch_free( ntargetDN.bv_val );
			return NULL;
		}

		if( dn_match( &nbaseDN, &nrefDN ) ) {
			ch_free( nrefDN.bv_val );
			ch_free( nbaseDN.bv_val );
			return ntargetDN.bv_val;
		}

		{
			struct berval muck;

			if( ntargetDN.bv_len < nbaseDN.bv_len ) {
				ch_free( nrefDN.bv_val );
				ch_free( nbaseDN.bv_val );
				return ntargetDN.bv_val;
			}

			rc = strcasecmp(
				&ntargetDN.bv_val[ntargetDN.bv_len-nbaseDN.bv_len],
				nbaseDN.bv_val );
			if( rc ) {
				/* target not subordinate to base */
				ch_free( nrefDN.bv_val );
				ch_free( nbaseDN.bv_val );
				return ntargetDN.bv_val;
			}

			muck.bv_len = ntargetDN.bv_len + nrefDN.bv_len - nbaseDN.bv_len;
			muck.bv_val = ch_malloc( muck.bv_len + 1 );

			strncpy( muck.bv_val, ntargetDN.bv_val,
				ntargetDN.bv_len-nbaseDN.bv_len );
			strcpy( &muck.bv_val[ntargetDN.bv_len-nbaseDN.bv_len],
				nrefDN.bv_val );

			ch_free( nrefDN.bv_val );
			ch_free( nbaseDN.bv_val );
			ch_free( ntargetDN.bv_val );

			return muck.bv_val;
		}
	}

	ch_free( nrefDN.bv_val );
	return ntargetDN.bv_val;
}


/* validate URL for global referral use
 *   LDAP URLs must not have:
 *     DN, attrs, scope, nor filter
 *   Any non-LDAP URL is okay
 *
 *   XXYYZ: should return an error string
 */
int validate_global_referral( const char *url )
{
	int rc;
	LDAPURLDesc *lurl;

	rc = ldap_url_parse_ext( url, &lurl, LDAP_PVT_URL_PARSE_NONE );

	switch( rc ) {
	case LDAP_URL_SUCCESS:
		break;

	case LDAP_URL_ERR_BADSCHEME:
		/* not LDAP hence valid */
		Debug( LDAP_DEBUG_CONFIG, "referral \"%s\": not LDAP.\n", url, 0, 0 );
		return 0;

	default:
		/* other error, bail */
		Debug( LDAP_DEBUG_ANY,
			"referral: invalid URL (%s): %s (%d)\n",
			url, "" /* ldap_url_error2str(rc) */, rc );
		return 1;
	}

	rc = 0;

	if( lurl->lud_dn && *lurl->lud_dn ) {
		Debug( LDAP_DEBUG_ANY,
			"referral: URL (%s): contains DN\n",
			url, 0, 0 );
		rc = 1;

	} else if( lurl->lud_attrs ) {
		Debug( LDAP_DEBUG_ANY,
			"referral: URL (%s): requests attributes\n",
			url, 0, 0 );
		rc = 1;

	} else if( lurl->lud_scope != LDAP_SCOPE_DEFAULT ) {
		Debug( LDAP_DEBUG_ANY,
			"referral: URL (%s): contains explicit scope\n",
			url, 0, 0 );
		rc = 1;

	} else if( lurl->lud_filter ) {
		Debug( LDAP_DEBUG_ANY,
			"referral: URL (%s): contains explicit filter\n",
			url, 0, 0 );
		rc = 1;
	}

	ldap_free_urldesc( lurl );
	return rc;
}

BerVarray referral_rewrite(
	BerVarray in,
	struct berval *base,
	struct berval *target,
	int scope )
{
	int		i;
	BerVarray	refs;
	struct berval	*iv, *jv;

	if ( in == NULL ) {
		return NULL;
	}

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ ) {
		/* just count them */
	}

	if ( i < 1 ) {
		return NULL;
	}

	refs = ch_malloc( ( i + 1 ) * sizeof( struct berval ) );

	for ( iv = in, jv = refs; !BER_BVISNULL( iv ); iv++ ) {
		LDAPURLDesc	*url;
		char		*dn;
		int		rc;
		
		rc = ldap_url_parse_ext( iv->bv_val, &url, LDAP_PVT_URL_PARSE_NONE );
		if ( rc == LDAP_URL_ERR_BADSCHEME ) {
			ber_dupbv( jv++, iv );
			continue;

		} else if ( rc != LDAP_URL_SUCCESS ) {
			continue;
		}

		dn = url->lud_dn;
		url->lud_dn = referral_dn_muck( ( dn && *dn ) ? dn : NULL,
				base, target );
		ldap_memfree( dn );

		if ( url->lud_scope == LDAP_SCOPE_DEFAULT ) {
			url->lud_scope = scope;
		}

		jv->bv_val = ldap_url_desc2str( url );
		if ( jv->bv_val != NULL ) {
			jv->bv_len = strlen( jv->bv_val );

		} else {
			ber_dupbv( jv, iv );
		}
		jv++;

		ldap_free_urldesc( url );
	}

	if ( jv == refs ) {
		ch_free( refs );
		refs = NULL;

	} else {
		BER_BVZERO( jv );
	}

	return refs;
}


BerVarray get_entry_referrals(
	Operation *op,
	Entry *e )
{
	Attribute *attr;
	BerVarray refs;
	unsigned i;
	struct berval *iv, *jv;

	AttributeDescription *ad_ref = slap_schema.si_ad_ref;

	attr = attr_find( e->e_attrs, ad_ref );

	if( attr == NULL ) return NULL;

	for( i=0; attr->a_vals[i].bv_val != NULL; i++ ) {
		/* count references */
	}

	if( i < 1 ) return NULL;

	refs = ch_malloc( (i + 1) * sizeof(struct berval));

	for( iv=attr->a_vals, jv=refs; iv->bv_val != NULL; iv++ ) {
		unsigned k;
		ber_dupbv( jv, iv );

		/* trim the label */
		for( k=0; k<jv->bv_len; k++ ) {
			if( isspace( (unsigned char) jv->bv_val[k] ) ) {
				jv->bv_val[k] = '\0';
				jv->bv_len = k;
				break;
			}
		}

		if(	jv->bv_len > 0 ) {
			jv++;
		} else {
			free( jv->bv_val );
		}
	}

	if( jv == refs ) {
		free( refs );
		refs = NULL;

	} else {
		jv->bv_val = NULL;
	}

	/* we should check that a referral value exists... */
	return refs;
}


int get_alias_dn(
	Entry *e,
	struct berval *ndn,
	int *err,
	const char **text )
{	
	Attribute *a;
	AttributeDescription *aliasedObjectName
		= slap_schema.si_ad_aliasedObjectName;

	a = attr_find( e->e_attrs, aliasedObjectName );

	if( a == NULL ) {
		/*
		 * there was an aliasedobjectname defined but no data.
		 */
		*err = LDAP_ALIAS_PROBLEM;
		*text = "alias missing aliasedObjectName attribute";
		return -1;
	}

	/* 
	 * aliasedObjectName should be SINGLE-VALUED with a single value. 
	 */			
	if ( a->a_vals[0].bv_val == NULL ) {
		/*
		 * there was an aliasedobjectname defined but no data.
		 */
		*err = LDAP_ALIAS_PROBLEM;
		*text = "alias missing aliasedObjectName value";
		return -1;
	}

	if( a->a_nvals[1].bv_val != NULL ) {
		*err = LDAP_ALIAS_PROBLEM;
		*text = "alias has multivalued aliasedObjectName";
		return -1;
	}

	*ndn = a->a_nvals[0];

	return 0;
}

