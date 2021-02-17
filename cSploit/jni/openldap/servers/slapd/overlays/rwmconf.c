/* rwmconf.c - rewrite/map configuration file routines */
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
#include "lutil.h"

int
rwm_map_config(
		struct ldapmap	*oc_map,
		struct ldapmap	*at_map,
		const char	*fname,
		int		lineno,
		int		argc,
		char		**argv )
{
	struct ldapmap		*map;
	struct ldapmapping	*mapping;
	char			*src, *dst;
	int			is_oc = 0;
	int			rc = 0;

	if ( argc < 3 || argc > 4 ) {
		Debug( LDAP_DEBUG_ANY,
	"%s: line %d: syntax is \"map {objectclass | attribute} [<local> | *] {<foreign> | *}\"\n",
			fname, lineno, 0 );
		return 1;
	}

	if ( strcasecmp( argv[1], "objectclass" ) == 0 ) {
		map = oc_map;
		is_oc = 1;

	} else if ( strcasecmp( argv[1], "attribute" ) == 0 ) {
		map = at_map;

	} else {
		Debug( LDAP_DEBUG_ANY, "%s: line %d: syntax is "
			"\"map {objectclass | attribute} [<local> | *] "
			"{<foreign> | *}\"\n",
			fname, lineno, 0 );
		return 1;
	}

	if ( !is_oc && map->map == NULL ) {
		/* only init if required */
		if ( rwm_map_init( map, &mapping ) != LDAP_SUCCESS ) {
			return 1;
		}
	}

	if ( strcmp( argv[2], "*" ) == 0 ) {
		if ( argc < 4 || strcmp( argv[3], "*" ) == 0 ) {
			map->drop_missing = ( argc < 4 );
			goto success_return;
		}
		src = dst = argv[3];

	} else if ( argc < 4 ) {
		src = "";
		dst = argv[2];

	} else {
		src = argv[2];
		dst = ( strcmp( argv[3], "*" ) == 0 ? src : argv[3] );
	}

	if ( ( map == at_map )
			&& ( strcasecmp( src, "objectclass" ) == 0
			|| strcasecmp( dst, "objectclass" ) == 0 ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"%s: line %d: objectclass attribute cannot be mapped\n",
			fname, lineno, 0 );
		return 1;
	}

	mapping = (struct ldapmapping *)ch_calloc( 2,
		sizeof(struct ldapmapping) );
	if ( mapping == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"%s: line %d: out of memory\n",
			fname, lineno, 0 );
		return 1;
	}
	ber_str2bv( src, 0, 1, &mapping[0].m_src );
	ber_str2bv( dst, 0, 1, &mapping[0].m_dst );
	mapping[1].m_src = mapping[0].m_dst;
	mapping[1].m_dst = mapping[0].m_src;

	mapping[0].m_flags = RWMMAP_F_NONE;
	mapping[1].m_flags = RWMMAP_F_NONE;

	/*
	 * schema check
	 */
	if ( is_oc ) {
		if ( src[0] != '\0' ) {
			mapping[0].m_src_oc = oc_bvfind( &mapping[0].m_src );
			if ( mapping[0].m_src_oc == NULL ) {
				Debug( LDAP_DEBUG_ANY,
	"%s: line %d: warning, source objectClass '%s' "
	"should be defined in schema\n",
					fname, lineno, src );

				/*
				 * FIXME: this should become an err
				 */
				mapping[0].m_src_oc = ch_malloc( sizeof( ObjectClass ) );
				memset( mapping[0].m_src_oc, 0, sizeof( ObjectClass ) );
				mapping[0].m_src_oc->soc_cname = mapping[0].m_src;
				mapping[0].m_flags |= RWMMAP_F_FREE_SRC;
			}
			mapping[1].m_dst_oc = mapping[0].m_src_oc;
		}

		mapping[0].m_dst_oc = oc_bvfind( &mapping[0].m_dst );
		if ( mapping[0].m_dst_oc == NULL ) {
			Debug( LDAP_DEBUG_ANY,
	"%s: line %d: warning, destination objectClass '%s' "
	"is not defined in schema\n",
				fname, lineno, dst );

			mapping[0].m_dst_oc = oc_bvfind_undef( &mapping[0].m_dst );
			if ( mapping[0].m_dst_oc == NULL ) {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: unable to mimic destination objectClass '%s'\n",
					fname, lineno, dst );
				goto error_return;
			}
		}
		mapping[1].m_src_oc = mapping[0].m_dst_oc;

		mapping[0].m_flags |= RWMMAP_F_IS_OC;
		mapping[1].m_flags |= RWMMAP_F_IS_OC;

	} else {
		int			rc;
		const char		*text = NULL;

		if ( src[0] != '\0' ) {
			rc = slap_bv2ad( &mapping[0].m_src,
					&mapping[0].m_src_ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
	"%s: line %d: warning, source attributeType '%s' "
	"should be defined in schema\n",
					fname, lineno, src );

				/*
				 * we create a fake "proxied" ad 
				 * and add it here.
				 */

				rc = slap_bv2undef_ad( &mapping[0].m_src,
						&mapping[0].m_src_ad, &text,
						SLAP_AD_PROXIED );
				if ( rc != LDAP_SUCCESS ) {
					char prefix[1024];
					snprintf( prefix, sizeof(prefix),
	"%s: line %d: source attributeType '%s': %d",
						fname, lineno, src, rc );
					Debug( LDAP_DEBUG_ANY, "%s (%s)\n",
						prefix, text ? text : "null", 0 );
					goto error_return;
				}

			}
			mapping[1].m_dst_ad = mapping[0].m_src_ad;
		}

		rc = slap_bv2ad( &mapping[0].m_dst, &mapping[0].m_dst_ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
	"%s: line %d: warning, destination attributeType '%s' "
	"is not defined in schema\n",
				fname, lineno, dst );

			rc = slap_bv2undef_ad( &mapping[0].m_dst,
					&mapping[0].m_dst_ad, &text,
					SLAP_AD_PROXIED );
			if ( rc != LDAP_SUCCESS ) {
				char prefix[1024];
				snprintf( prefix, sizeof(prefix), 
	"%s: line %d: destination attributeType '%s': %d",
					fname, lineno, dst, rc );
				Debug( LDAP_DEBUG_ANY, "%s (%s)\n",
					prefix, text ? text : "null", 0 );
				goto error_return;
			}
		}
		mapping[1].m_src_ad = mapping[0].m_dst_ad;
	}

	if ( ( src[0] != '\0' && avl_find( map->map, (caddr_t)mapping, rwm_mapping_cmp ) != NULL)
			|| avl_find( map->remap, (caddr_t)&mapping[1], rwm_mapping_cmp ) != NULL)
	{
		Debug( LDAP_DEBUG_ANY,
			"%s: line %d: duplicate mapping found.\n",
			fname, lineno, 0 );
		/* FIXME: free stuff */
		goto error_return;
	}

	if ( src[0] != '\0' ) {
		avl_insert( &map->map, (caddr_t)&mapping[0],
					rwm_mapping_cmp, rwm_mapping_dup );
	}
	avl_insert( &map->remap, (caddr_t)&mapping[1],
				rwm_mapping_cmp, rwm_mapping_dup );

success_return:;
	return rc;

error_return:;
	if ( mapping ) {
		rwm_mapping_free( mapping );
	}

	return 1;
}

static char *
rwm_suffix_massage_regexize( const char *s )
{
	char *res, *ptr;
	const char *p, *r;
	int i;

	if ( s[0] == '\0' ) {
		return ch_strdup( "^(.+)$" );
	}

	for ( i = 0, p = s; 
			( r = strchr( p, ',' ) ) != NULL; 
			p = r + 1, i++ )
		;

	res = ch_calloc( sizeof( char ), strlen( s )
			+ STRLENOF( "((.+),)?" )
			+ STRLENOF( "[ ]?" ) * i
			+ STRLENOF( "$" ) + 1 );

	ptr = lutil_strcopy( res, "((.+),)?" );
	for ( i = 0, p = s;
			( r = strchr( p, ',' ) ) != NULL;
			p = r + 1 , i++ ) {
		ptr = lutil_strncopy( ptr, p, r - p + 1 );
		ptr = lutil_strcopy( ptr, "[ ]?" );

		if ( r[ 1 ] == ' ' ) {
			r++;
		}
	}
	ptr = lutil_strcopy( ptr, p );
	ptr[0] = '$';
	ptr[1] = '\0';

	return res;
}

static char *
rwm_suffix_massage_patternize( const char *s, const char *p )
{
	ber_len_t	len;
	char		*res, *ptr;

	len = strlen( p );

	if ( s[ 0 ] == '\0' ) {
		len++;
	}

	res = ch_calloc( sizeof( char ), len + STRLENOF( "%1" ) + 1 );
	if ( res == NULL ) {
		return NULL;
	}

	ptr = lutil_strcopy( res, ( p[0] == '\0' ? "%2" : "%1" ) );
	if ( s[ 0 ] == '\0' ) {
		ptr[ 0 ] = ',';
		ptr++;
	}
	lutil_strcopy( ptr, p );

	return res;
}

int
rwm_suffix_massage_config( 
		struct rewrite_info *info,
		struct berval *pvnc,
		struct berval *nvnc,
		struct berval *prnc,
		struct berval *nrnc
)
{
	char *rargv[ 5 ];
	int line = 0;

	rargv[ 0 ] = "rewriteEngine";
	rargv[ 1 ] = "on";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "default";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteRule";
	rargv[ 1 ] = rwm_suffix_massage_regexize( pvnc->bv_val );
	rargv[ 2 ] = rwm_suffix_massage_patternize( pvnc->bv_val, prnc->bv_val );
	rargv[ 3 ] = ":";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	ch_free( rargv[ 1 ] );
	ch_free( rargv[ 2 ] );
	
	if ( BER_BVISEMPTY( pvnc ) ) {
		rargv[ 0 ] = "rewriteRule";
		rargv[ 1 ] = "^$";
		rargv[ 2 ] = prnc->bv_val;
		rargv[ 3 ] = ":";
		rargv[ 4 ] = NULL;
		rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	}

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchEntryDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );
	
	rargv[ 0 ] = "rewriteRule";
	rargv[ 1 ] = rwm_suffix_massage_regexize( prnc->bv_val );
	rargv[ 2 ] = rwm_suffix_massage_patternize( prnc->bv_val, pvnc->bv_val );
	rargv[ 3 ] = ":";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	ch_free( rargv[ 1 ] );
	ch_free( rargv[ 2 ] );

	if ( BER_BVISEMPTY( prnc ) ) {
		rargv[ 0 ] = "rewriteRule";
		rargv[ 1 ] = "^$";
		rargv[ 2 ] = pvnc->bv_val;
		rargv[ 3 ] = ":";
		rargv[ 4 ] = NULL;
		rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	}

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "matchedDN";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );

#ifdef RWM_REFERRAL_REWRITE
	/* FIXME: we don't want this on by default, do we? */
	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "referralDN";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
#else /* ! RWM_REFERRAL_REWRITE */
	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "referralAttrDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "referralDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );
#endif /* ! RWM_REFERRAL_REWRITE */

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchAttrDN";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );

	return 0;
}

#endif /* SLAPD_OVER_RWM */
