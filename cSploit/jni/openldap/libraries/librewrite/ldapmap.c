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

#define LDAP_DEPRECATED 1
#include "rewrite-int.h"
#include "rewrite-map.h"

typedef enum {
	MAP_LDAP_UNKNOWN,
	MAP_LDAP_EVERYTIME,
	MAP_LDAP_NOW,
	MAP_LDAP_LATER
} bindwhen_t;

/*
 * LDAP map data structure
 */
struct ldap_map_data {
	char                           *lm_url;
	LDAPURLDesc                    *lm_lud;
	int				lm_version;
	char                           *lm_binddn;
	struct berval			lm_cred;

	bindwhen_t			lm_when;

	LDAP                           *lm_ld;

	int                             lm_wantdn;
	char				*lm_attrs[ 2 ];

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_mutex_t         lm_mutex;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
};

static void
map_ldap_free(
		struct ldap_map_data *data
)
{
	assert( data != NULL );

	if ( data->lm_url != NULL ) {
		free( data->lm_url );
	}

	if ( data->lm_lud != NULL ) {
		ldap_free_urldesc( data->lm_lud );
	}

	if ( data->lm_binddn != NULL ) {
		free( data->lm_binddn );
	}

	if ( data->lm_cred.bv_val != NULL ) {
		memset( data->lm_cred.bv_val, 0, data->lm_cred.bv_len );
		free( data->lm_cred.bv_val );
		data->lm_cred.bv_val = NULL;
		data->lm_cred.bv_len = 0;
	}

	if ( data->lm_when != MAP_LDAP_EVERYTIME && data->lm_ld != NULL ) {
		ldap_unbind_ext( data->lm_ld, NULL, NULL );
	}

	free( data );
}

static void *
map_ldap_parse(
		const char *fname,
		int lineno,
		int argc,
		char **argv
)
{
	struct ldap_map_data *data;
	char *p, *uri;

	assert( fname != NULL );
	assert( argv != NULL );

	data = calloc( sizeof( struct ldap_map_data ), 1 );
	if ( data == NULL ) {
		return NULL;
	}

	if ( argc < 1 ) {
		Debug( LDAP_DEBUG_ANY,
				"[%s:%d] ldap map needs URI\n%s",
				fname, lineno, "" );
		free( data );
		return NULL;
	}

	uri = argv[ 0 ];
	if ( strncasecmp( uri, "uri=", STRLENOF( "uri=" ) ) == 0 ) {
		uri += STRLENOF( "uri=" );
	}

	data->lm_url = strdup( uri );
	if ( data->lm_url == NULL ) {
		map_ldap_free( data );
		return NULL;
	}
	
	if ( ldap_url_parse( uri, &data->lm_lud ) != REWRITE_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
				"[%s:%d] illegal URI '%s'\n",
				fname, lineno, argv[ 0 ] );
		map_ldap_free( data );
		return NULL;
	}

	/* trim everything after [host][:port] */
	p = strchr( data->lm_url, '/' );
	assert( p[ 1 ] == '/' );
	if ( ( p = strchr( p + 2, '/' ) ) != NULL ) {
		p[ 0 ] = '\0';
	}

	if ( data->lm_lud->lud_attrs == NULL ) {
		data->lm_attrs[ 0 ] = LDAP_NO_ATTRS;
		data->lm_wantdn = 1;

	} else {
		if ( data->lm_lud->lud_attrs[ 1 ] != NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"[%s:%d] only one attribute allowed in URI\n",
				fname, lineno, 0 );
			map_ldap_free( data );
			return NULL;
		}

		if ( strcasecmp( data->lm_lud->lud_attrs[ 0 ], "dn" ) == 0
			|| strcasecmp( data->lm_lud->lud_attrs[ 0 ], "entryDN" ) == 0 )
		{
			ldap_memfree( data->lm_lud->lud_attrs[ 0 ] );
			ldap_memfree( data->lm_lud->lud_attrs );
			data->lm_lud->lud_attrs = NULL;
			data->lm_attrs[ 0 ] = LDAP_NO_ATTRS;
			data->lm_wantdn = 1;

		} else {
			data->lm_attrs[ 0 ] = data->lm_lud->lud_attrs[ 0 ];
		}
	}

	data->lm_attrs[ 1 ] = NULL;

	/* safe defaults */
	data->lm_version = LDAP_VERSION3;

	for ( argc--, argv++; argc > 0; argc--, argv++ ) {
		if ( strncasecmp( argv[ 0 ], "binddn=", STRLENOF( "binddn=" ) ) == 0 ) {
			char *p = argv[ 0 ] + STRLENOF( "binddn=" );
			int l;

			if ( p[ 0 ] == '\"' || p [ 0 ] == '\'' ) {
				l = strlen( p ) - 2;
				p++;
				if ( p[ l ] != p[ 0 ] ) {
					map_ldap_free( data );
					return NULL;
				}
			} else {
				l = strlen( p );
			}
			
			data->lm_binddn = strdup( p );			
			if ( data->lm_binddn == NULL ) {
				map_ldap_free( data );
				return NULL;
			}

			if ( data->lm_binddn[ l ] == '\"' 
					|| data->lm_binddn[ l ] == '\'' ) {
				data->lm_binddn[ l ] = '\0';
			}

			/* deprecated */
		} else if ( strncasecmp( argv[ 0 ], "bindpw=", STRLENOF( "bindpw=" ) ) == 0 ) {
			ber_str2bv( argv[ 0 ] + STRLENOF( "bindpw=" ), 0, 1, &data->lm_cred );
			if ( data->lm_cred.bv_val == NULL ) {
				map_ldap_free( data );
				return NULL;
			}

		} else if ( strncasecmp( argv[ 0 ], "credentials=", STRLENOF( "credentials=" ) ) == 0 ) {
			ber_str2bv( argv[ 0 ] + STRLENOF( "credentials=" ), 0, 1, &data->lm_cred );
			if ( data->lm_cred.bv_val == NULL ) {
				map_ldap_free( data );
				return NULL;
			}

		} else if ( strncasecmp( argv[ 0 ], "bindwhen=", STRLENOF( "bindwhen=" ) ) == 0 ) {
			char *p = argv[ 0 ] + STRLENOF( "bindwhen=" );

			if ( strcasecmp( p, "now" ) == 0 ) {
				int rc;
				
				data->lm_when = MAP_LDAP_NOW;
				
				/*
				 * Init LDAP handler ...
				 */
				rc = ldap_initialize( &data->lm_ld, data->lm_url );
				if ( rc != LDAP_SUCCESS ) {
					map_ldap_free( data );
					return NULL;
				}

				ldap_set_option( data->lm_ld,
					LDAP_OPT_PROTOCOL_VERSION,
					(void *)&data->lm_version );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
				ldap_pvt_thread_mutex_init( &data->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

			} else if ( strcasecmp( p, "later" ) == 0 ) {
				data->lm_when = MAP_LDAP_LATER;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
				ldap_pvt_thread_mutex_init( &data->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

			} else if ( strcasecmp( p, "everytime" ) == 0 ) {
				data->lm_when = MAP_LDAP_EVERYTIME;
			} else {
				/* ignore ... */
			}

		} else if ( strncasecmp( argv[ 0 ], "version=", STRLENOF( "version=" ) ) == 0 ) {
			if ( lutil_atoi( &data->lm_version, argv[ 0 ] + STRLENOF( "version=" ) ) ) {
				map_ldap_free( data );
				return NULL;
			}

			switch ( data->lm_version ) {
			case LDAP_VERSION2:
			case LDAP_VERSION3:
				break;

			default:
				Debug( LDAP_DEBUG_ANY,
					"[%s:%d] unknown version %s\n",
					fname, lineno, p );
				map_ldap_free( data );
				return NULL;
			}

		} else {
			Debug( LDAP_DEBUG_ANY,
				"[%s:%d] unknown option %s (ignored)\n",
				fname, lineno, argv[0] );
		}
	}

	if ( data->lm_when == MAP_LDAP_UNKNOWN ) {
		data->lm_when = MAP_LDAP_EVERYTIME;
	}

	return ( void * )data;
}

static int
map_ldap_apply(
		void *private,
		const char *filter,
		struct berval *val

)
{
	LDAP *ld;
	LDAPMessage *res = NULL, *entry;
	int rc;
	struct ldap_map_data *data = private;
	LDAPURLDesc *lud = data->lm_lud;
	
	int first_try = 1, set_version = 0;

	assert( private != NULL );
	assert( filter != NULL );
	assert( val != NULL );

	val->bv_val = NULL;
	val->bv_len = 0;

	if ( data->lm_when == MAP_LDAP_EVERYTIME ) {
		rc = ldap_initialize( &ld, data->lm_url );
		set_version = 1;

	} else {
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &data->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		rc = LDAP_SUCCESS;

		if ( data->lm_when == MAP_LDAP_LATER && data->lm_ld == NULL ) {
			rc = ldap_initialize( &data->lm_ld, data->lm_url );
			set_version = 1;
		}
		
		ld = data->lm_ld;
	}

	if ( rc != LDAP_SUCCESS ) {
		rc = REWRITE_ERR;
		goto rc_return;
	}

do_bind:;
	if ( set_version ) {
		ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION,
			(void *)&data->lm_version );
		set_version = 0;
	}

	if ( data->lm_binddn != NULL ) {
		rc = ldap_sasl_bind_s( ld, data->lm_binddn,
			LDAP_SASL_SIMPLE, &data->lm_cred,
			NULL, NULL, NULL );
		if ( rc == LDAP_SERVER_DOWN && first_try ) {
			first_try = 0;
			if ( ldap_initialize( &ld, data->lm_url ) != LDAP_SUCCESS ) {
				rc = REWRITE_ERR;
				goto rc_return;
			}
			set_version = 1;
			goto do_bind;

		} else if ( rc != REWRITE_SUCCESS ) {
			rc = REWRITE_ERR;
			goto rc_return;
		}
	}

	rc = ldap_search_ext_s( ld, lud->lud_dn, lud->lud_scope, ( char * )filter,
			data->lm_attrs, 0, NULL, NULL, NULL, 1, &res );
	if ( rc == LDAP_SERVER_DOWN && first_try ) {
		first_try = 0;
                if ( ldap_initialize( &ld, data->lm_url ) != LDAP_SUCCESS ) {
			rc = REWRITE_ERR;
			goto rc_return;
		}
		set_version = 1;
		goto do_bind;

	} else if ( rc != LDAP_SUCCESS ) {
		rc = REWRITE_ERR;
		goto rc_return;
	}

	if ( ldap_count_entries( ld, res ) != 1 ) {
		ldap_msgfree( res );
		rc = REWRITE_ERR;
		goto rc_return;
	}

	entry = ldap_first_entry( ld, res );
	assert( entry != NULL );

	if ( data->lm_wantdn == 1 ) {
		/*
		 * dn is newly allocated, so there's no need to strdup it
		 */
		val->bv_val = ldap_get_dn( ld, entry );
		val->bv_len = strlen( val->bv_val );

	} else {
		struct berval **values;

		values = ldap_get_values_len( ld, entry, data->lm_attrs[ 0 ] );
		if ( values != NULL ) {
			if ( values[ 0 ] != NULL && values[ 0 ]->bv_val != NULL ) {
#if 0
				/* NOTE: in principle, multiple values
				 * should not be acceptable according
				 * to the current API; ignore by now */
				if ( values[ 1 ] != NULL ) {
					/* error */				
				}
#endif
				ber_dupbv( val, values[ 0 ] );
			}
			ldap_value_free_len( values );
		}
	}
	
	ldap_msgfree( res );

	if ( val->bv_val == NULL ) {
		rc = REWRITE_ERR;
		goto rc_return;
	}

rc_return:;
	if ( data->lm_when == MAP_LDAP_EVERYTIME ) {
		if ( ld != NULL ) {
			ldap_unbind_ext( ld, NULL, NULL );
		}

	} else {
		data->lm_ld = ld;
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_unlock( &data->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	}
	
	return rc;
}

static int
map_ldap_destroy(
		void *private
)
{
	struct ldap_map_data *data = private;

	assert( private != NULL );
	
	map_ldap_free( data );

	return 0;
}

const rewrite_mapper rewrite_ldap_mapper = {
	"ldap",
	map_ldap_parse,
	map_ldap_apply,
	map_ldap_destroy
};

