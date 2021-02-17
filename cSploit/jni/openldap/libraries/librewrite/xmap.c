/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#include <portable.h>

#include <stdio.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#define LDAP_DEPRECATED 1
#include "rewrite-int.h"
#include "rewrite-map.h"

/*
 * Global data
 */
#ifdef USE_REWRITE_LDAP_PVT_THREADS
ldap_pvt_thread_mutex_t xpasswd_mutex;
static int xpasswd_mutex_init = 0;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

/*
 * Map parsing
 * NOTE: these are old-fashion maps; new maps will be parsed on separate
 * config lines, and referred by name.
 */
struct rewrite_map *
rewrite_xmap_parse(
		struct rewrite_info *info,
		const char *s,
		const char **currpos
)
{
	struct rewrite_map *map;

	assert( info != NULL );
	assert( s != NULL );
	assert( currpos != NULL );

	Debug( LDAP_DEBUG_ARGS, "rewrite_xmap_parse: %s\n%s%s",
			s, "", "" );

	*currpos = NULL;

	map = calloc( sizeof( struct rewrite_map ), 1 );
	if ( map == NULL ) {
		Debug( LDAP_DEBUG_ANY, "rewrite_xmap_parse:"
				" calloc failed\n%s%s%s", "", "", "" );
		return NULL;
	}

	/*
	 * Experimental passwd map:
	 * replaces the uid with the matching gecos from /etc/passwd file 
	 */
	if ( strncasecmp(s, "xpasswd", 7 ) == 0 ) {
		map->lm_type = REWRITE_MAP_XPWDMAP;
		map->lm_name = strdup( "xpasswd" );
		if ( map->lm_name == NULL ) {
			free( map );
			return NULL;
		}

		assert( s[7] == '}' );
		*currpos = s + 8;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		if ( !xpasswd_mutex_init ) {
			if ( ldap_pvt_thread_mutex_init( &xpasswd_mutex ) ) {
				free( map );
				return NULL;
			}
		}
		++xpasswd_mutex_init;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		/* Don't really care if fails */
		return map;
	
	/*
	 * Experimental file map:
	 * looks up key in a `key value' ascii file
	 */
	} else if ( strncasecmp( s, "xfile", 5 ) == 0 ) {
		char *filename;
		const char *p;
		int l;
		int c = 5;
		
		map->lm_type = REWRITE_MAP_XFILEMAP;
		
		if ( s[ c ] != '(' ) {
			free( map );
			return NULL;
		}

		/* Must start with '/' for security concerns */
		c++;
		if ( s[ c ] != '/' ) {
			free( map );
			return NULL;
		}

		for ( p = s + c; p[ 0 ] != '\0' && p[ 0 ] != ')'; p++ );
		if ( p[ 0 ] != ')' ) {
			free( map );
			return NULL;
		}

		l = p - s - c;
		filename = calloc( sizeof( char ), l + 1 );
		if ( filename == NULL ) {
			free( map );
			return NULL;
		}
		AC_MEMCPY( filename, s + c, l );
		filename[ l ] = '\0';
		
		map->lm_args = ( void * )fopen( filename, "r" );
		free( filename );

		if ( map->lm_args == NULL ) {
			free( map );
			return NULL;
		}

		*currpos = p + 1;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
                if ( ldap_pvt_thread_mutex_init( &map->lm_mutex ) ) {
			fclose( ( FILE * )map->lm_args );
			free( map );
			return NULL;
		}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */	
		
		return map;

	/*
         * Experimental ldap map:
         * looks up key on the fly (not implemented!)
         */
        } else if ( strncasecmp(s, "xldap", 5 ) == 0 ) {
		char *p;
		char *url;
		int l, rc;
		int c = 5;
		LDAPURLDesc *lud;

		if ( s[ c ] != '(' ) {
			free( map );
			return NULL;
		}
		c++;
		
		p = strchr( s, '}' );
		if ( p == NULL ) {
			free( map );
			return NULL;
		}
		p--;

		*currpos = p + 2;
	
		/*
		 * Add two bytes for urlencoding of '%s'
		 */
		l = p - s - c;
		url = calloc( sizeof( char ), l + 3 );
		if ( url == NULL ) {
			free( map );
			return NULL;
		}
		AC_MEMCPY( url, s + c, l );
		url[ l ] = '\0';

		/*
		 * Urlencodes the '%s' for ldap_url_parse
		 */
		p = strchr( url, '%' );
		if ( p != NULL ) {
			AC_MEMCPY( p + 3, p + 1, strlen( p + 1 ) + 1 );
			p[ 1 ] = '2';
			p[ 2 ] = '5';
		}

		rc =  ldap_url_parse( url, &lud );
		free( url );

		if ( rc != LDAP_SUCCESS ) {
			free( map );
			return NULL;
		}
		assert( lud != NULL );

		map->lm_args = ( void * )lud;
		map->lm_type = REWRITE_MAP_XLDAPMAP;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
                if ( ldap_pvt_thread_mutex_init( &map->lm_mutex ) ) {
			ldap_free_urldesc( lud );
			free( map );
			return NULL;
		}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		return map;
	
	/* Unhandled map */
	}

	free( map );
	return NULL;
}

/*
 * Map key -> value resolution
 * NOTE: these are old-fashion maps; new maps will be parsed on separate
 * config lines, and referred by name.
 */
int
rewrite_xmap_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_map *map,
		struct berval *key,
		struct berval *val
)
{
	int rc = REWRITE_SUCCESS;
	
	assert( info != NULL );
	assert( op != NULL );
	assert( map != NULL );
	assert( key != NULL );
	assert( val != NULL );
	
	val->bv_val = NULL;
	val->bv_len = 0;
	
	switch ( map->lm_type ) {
#ifdef HAVE_GETPWNAM
	case REWRITE_MAP_XPWDMAP: {
		struct passwd *pwd;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &xpasswd_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
		
		pwd = getpwnam( key->bv_val );
		if ( pwd == NULL ) {

#ifdef USE_REWRITE_LDAP_PVT_THREADS
			ldap_pvt_thread_mutex_unlock( &xpasswd_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

			rc = LDAP_NO_SUCH_OBJECT;
			break;
		}

#ifdef HAVE_STRUCT_PASSWD_PW_GECOS
		if ( pwd->pw_gecos != NULL && pwd->pw_gecos[0] != '\0' ) {
			int l = strlen( pwd->pw_gecos );
			
			val->bv_val = strdup( pwd->pw_gecos );
			val->bv_len = l;
		} else
#endif /* HAVE_STRUCT_PASSWD_PW_GECOS */
		{
			val->bv_val = strdup( key->bv_val );
			val->bv_len = key->bv_len;
		}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_unlock( &xpasswd_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		if ( val->bv_val == NULL ) {
			rc = REWRITE_ERR;
		}
		break;
	}
#endif /* HAVE_GETPWNAM*/
	
	case REWRITE_MAP_XFILEMAP: {
		char buf[1024];
		
		if ( map->lm_args == NULL ) {
			rc = REWRITE_ERR;
			break;
		}
		
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		rewind( ( FILE * )map->lm_args );
		
		while ( fgets( buf, sizeof( buf ), ( FILE * )map->lm_args ) ) {
			char *p;
			int blen;
			
			blen = strlen( buf );
			if ( buf[ blen - 1 ] == '\n' ) {
				buf[ blen - 1 ] = '\0';
			}
			
			p = strtok( buf, " " );
			if ( p == NULL ) {
#ifdef USE_REWRITE_LDAP_PVT_THREADS
				ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
				rc = REWRITE_ERR;
				goto rc_return;
			}
			if ( strcasecmp( p, key->bv_val ) == 0 
					&& ( p = strtok( NULL, "" ) ) ) {
				val->bv_val = strdup( p );
				if ( val->bv_val == NULL ) {
					return REWRITE_ERR;
				}

				val->bv_len = strlen( p );
				
#ifdef USE_REWRITE_LDAP_PVT_THREADS
				ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
				
				goto rc_return;
			}
		}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		rc = REWRITE_ERR;
		
		break;
	}

	case REWRITE_MAP_XLDAPMAP: {
		LDAP *ld;
		char filter[1024];
		LDAPMessage *res = NULL, *entry;
		LDAPURLDesc *lud = ( LDAPURLDesc * )map->lm_args;
		int attrsonly = 0;
		char **values;

		assert( lud != NULL );

		/*
		 * No mutex because there is no write on the map data
		 */
		
		ld = ldap_init( lud->lud_host, lud->lud_port );
		if ( ld == NULL ) {
			rc = REWRITE_ERR;
			goto rc_return;
		}

		snprintf( filter, sizeof( filter ), lud->lud_filter,
				key->bv_val );

		if ( strcasecmp( lud->lud_attrs[ 0 ], "dn" ) == 0 ) {
			attrsonly = 1;
		}
		rc = ldap_search_s( ld, lud->lud_dn, lud->lud_scope,
				filter, lud->lud_attrs, attrsonly, &res );
		if ( rc != LDAP_SUCCESS ) {
			ldap_unbind( ld );
			rc = REWRITE_ERR;
			goto rc_return;
		}

		if ( ldap_count_entries( ld, res ) != 1 ) {
			ldap_unbind( ld );
			rc = REWRITE_ERR;
			goto rc_return;
		}

		entry = ldap_first_entry( ld, res );
		if ( entry == NULL ) {
			ldap_msgfree( res );
			ldap_unbind( ld );
			rc = REWRITE_ERR;
			goto rc_return;
		}
		if ( attrsonly == 1 ) {
			val->bv_val = ldap_get_dn( ld, entry );

		} else {
			values = ldap_get_values( ld, entry,
					lud->lud_attrs[0] );
			if ( values != NULL ) {
				val->bv_val = strdup( values[ 0 ] );
				ldap_value_free( values );
			}
		}

		ldap_msgfree( res );
		ldap_unbind( ld );
		
		if ( val->bv_val == NULL ) {
			rc = REWRITE_ERR;
			goto rc_return;
		}
		val->bv_len = strlen( val->bv_val );

		rc = REWRITE_SUCCESS;
	} break;
	}

rc_return:;
	return rc;
}

int
rewrite_xmap_destroy(
		struct rewrite_map **pmap
)
{
	struct rewrite_map *map;

	assert( pmap != NULL );
	assert( *pmap != NULL );

	map = *pmap;

	switch ( map->lm_type ) {
	case REWRITE_MAP_XPWDMAP:
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		--xpasswd_mutex_init;
		if ( !xpasswd_mutex_init ) {
			ldap_pvt_thread_mutex_destroy( &xpasswd_mutex );
		}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		break;

	case REWRITE_MAP_XFILEMAP:
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		if ( map->lm_args ) {
			fclose( ( FILE * )map->lm_args );
			map->lm_args = NULL;
		}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
		ldap_pvt_thread_mutex_destroy( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
		break;

	case REWRITE_MAP_XLDAPMAP:
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		if ( map->lm_args ) {
			ldap_free_urldesc( ( LDAPURLDesc * )map->lm_args );
			map->lm_args = NULL;
		}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
		ldap_pvt_thread_mutex_destroy( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
		break;

	default:
		break;

	}

	free( map->lm_name );
	free( map );
	*pmap = NULL;

	return 0;
}

