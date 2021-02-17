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

#include "rewrite-int.h"
#include "rewrite-map.h"

/*
 * Parses a plugin map
 */
static int
rewrite_parse_builtin_map(
		struct rewrite_info *info,
		const char *fname,
		int lineno,
		int argc,
		char **argv
);

/*
 * Parses a config line and takes actions to fit content in rewrite structure;
 * lines handled are of the form:
 *
 *      rewriteEngine 		{on|off}
 *      rewriteMaxPasses        numPasses [numPassesPerRule]
 *      rewriteContext 		contextName [alias aliasedContextName]
 *      rewriteRule 		pattern substPattern [ruleFlags]
 *      rewriteMap 		mapType mapName [mapArgs]
 *      rewriteParam		paramName paramValue
 */
int
rewrite_parse(
		struct rewrite_info *info,
		const char *fname,
		int lineno,
		int argc,
		char **argv
)
{
	int rc = -1;

	assert( info != NULL );
	assert( fname != NULL );
	assert( argv != NULL );
	assert( argc > 0 );
	
	/*
	 * Switch on the rewrite engine
	 */
	if ( strcasecmp( argv[ 0 ], "rewriteEngine" ) == 0 ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteEngine needs 'state'\n%s",
					fname, lineno, "" );
			return -1;

		} else if ( argc > 2 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] extra fields in rewriteEngine"
					" will be discarded\n%s",
					fname, lineno, "" );
		}

		if ( strcasecmp( argv[ 1 ], "on" ) == 0 ) {
			info->li_state = REWRITE_ON;

		} else if ( strcasecmp( argv[ 1 ], "off" ) == 0 ) {
			info->li_state = REWRITE_OFF;

		} else {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] unknown 'state' in rewriteEngine;"
					" assuming 'on'\n%s",
					fname, lineno, "" );
			info->li_state = REWRITE_ON;
		}
		rc = REWRITE_SUCCESS;
	
	/*
	 * Alter max passes
	 */
	} else if ( strcasecmp( argv[ 0 ], "rewriteMaxPasses" ) == 0 ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteMaxPasses needs 'value'\n%s",
					fname, lineno, "" );
			return -1;
		}

		if ( lutil_atoi( &info->li_max_passes, argv[ 1 ] ) != 0 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] unable to parse rewriteMaxPasses=\"%s\"\n",
					fname, lineno, argv[ 1 ] );
			return -1;
		}

		if ( info->li_max_passes <= 0 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] negative or null rewriteMaxPasses\n",
					fname, lineno, 0 );
			return -1;
		}

		if ( argc > 2 ) {
			if ( lutil_atoi( &info->li_max_passes_per_rule, argv[ 2 ] ) != 0 ) {
				Debug( LDAP_DEBUG_ANY,
						"[%s:%d] unable to parse rewriteMaxPassesPerRule=\"%s\"\n",
						fname, lineno, argv[ 2 ] );
				return -1;
			}

			if ( info->li_max_passes_per_rule <= 0 ) {
				Debug( LDAP_DEBUG_ANY,
						"[%s:%d] negative or null rewriteMaxPassesPerRule\n",
						fname, lineno, 0 );
				return -1;
			}

		} else {
			info->li_max_passes_per_rule = info->li_max_passes;
		}
		rc = REWRITE_SUCCESS;
	
	/*
	 * Start a new rewrite context and set current context
	 */
	} else if ( strcasecmp( argv[ 0 ], "rewriteContext" ) == 0 ) {
		if ( argc < 2 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteContext needs 'name'\n%s",
					fname, lineno, "" );
			return -1;
		} 

		/*
		 * Checks for existence (lots of contexts should be
		 * available by default ...)
		 */
		 rewrite_int_curr_context = rewrite_context_find( info, argv[ 1 ] );
		 if ( rewrite_int_curr_context == NULL ) {
			 rewrite_int_curr_context = rewrite_context_create( info,
					 argv[ 1 ] );                       
		 }
		 if ( rewrite_int_curr_context == NULL ) {
			 return -1;
		 }
						
		 if ( argc > 2 ) {

			 /*
			  * A context can alias another (e.g., the `builtin'
			  * contexts for backend operations, if not defined,
			  * alias the `default' rewrite context (with the
			  * notable exception of the searchResult context,
			  * which can be undefined)
			  */
			 if ( strcasecmp( argv[ 2 ], "alias" ) == 0 ) {
				 struct rewrite_context *aliased;
				 
				 if ( argc == 3 ) {
					 Debug( LDAP_DEBUG_ANY,
							 "[%s:%d] rewriteContext"
							 " needs 'name' after"
							 " 'alias'\n%s",
							 fname, lineno, "" );
					 return -1;

				 } else if ( argc > 4 ) {
					 Debug( LDAP_DEBUG_ANY,
							 "[%s:%d] extra fields in"
							 " rewriteContext"
							 " after aliased name"
							 " will be"
							 " discarded\n%s",
							 fname, lineno, "" );
				 }
				 
				 aliased = rewrite_context_find( info, 
						 argv[ 3 ] );
				 if ( aliased == NULL ) {
					 Debug( LDAP_DEBUG_ANY,
							 "[%s:%d] aliased"
							 " rewriteContext '%s'"
							 " does not exists\n",
							 fname, lineno,
							 argv[ 3 ] );
					 return -1;
				 }
				 
				 rewrite_int_curr_context->lc_alias = aliased;
				 rewrite_int_curr_context = aliased;

			 } else {
				 Debug( LDAP_DEBUG_ANY,
						 "[%s:%d] extra fields"
						 " in rewriteContext"
						 " will be discarded\n%s",
						 fname, lineno, "" );
			 }
		 }
		 rc = REWRITE_SUCCESS;
		 
	/*
	 * Compile a rule in current context
	 */
	} else if ( strcasecmp( argv[ 0 ], "rewriteRule" ) == 0 ) {
		if ( argc < 3 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteRule needs 'pattern'"
					" 'subst' ['flags']\n%s",
					fname, lineno, "" );
			return -1;

		} else if ( argc > 4 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] extra fields in rewriteRule"
					" will be discarded\n%s",
					fname, lineno, "" );
		}

		if ( rewrite_int_curr_context == NULL ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteRule outside a"
					" context; will add to default\n%s",
					fname, lineno, "" );
			rewrite_int_curr_context = rewrite_context_find( info,
					REWRITE_DEFAULT_CONTEXT );

			/*
			 * Default context MUST exist in a properly initialized
			 * struct rewrite_info
			 */
			assert( rewrite_int_curr_context != NULL );
		}
		
		rc = rewrite_rule_compile( info, rewrite_int_curr_context, argv[ 1 ],
				argv[ 2 ], ( argc == 4 ? argv[ 3 ] : "" ) );
	
	/*
	 * Add a plugin map to the map tree
	 */
	} else if ( strcasecmp( argv[ 0 ], "rewriteMap" ) == 0 ) {
		if ( argc < 3 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteMap needs at least 'type'"
					" and 'name' ['args']\n%s",
					fname, lineno, "" );
			return -1;
		}

		rc = rewrite_parse_builtin_map( info, fname, lineno,
				argc, argv );

	/*
	 * Set the value of a global scope parameter
	 */
	} else if ( strcasecmp( argv[ 0 ], "rewriteParam" ) == 0 ) {
		if ( argc < 3 ) {
			Debug( LDAP_DEBUG_ANY,
					"[%s:%d] rewriteParam needs 'name'"
					" and 'value'\n%s",
					fname, lineno, "" );
			return -1;
		}

		rc = rewrite_param_set( info, argv[ 1 ], argv[ 2 ] );
		
	/*
	 * Error
	 */
	} else {
		Debug( LDAP_DEBUG_ANY,
				"[%s:%d] unknown command '%s'\n",
				fname, lineno, "" );
		return -1;
	}

	return rc;
}

/*
 * Compares two maps
 */
static int
rewrite_builtin_map_cmp(
		const void *c1,
                const void *c2
)
{
	const struct rewrite_builtin_map *m1, *m2;

        m1 = ( const struct rewrite_builtin_map * )c1;
        m2 = ( const struct rewrite_builtin_map * )c2;

        assert( m1 != NULL );
        assert( m2 != NULL );
        assert( m1->lb_name != NULL );
        assert( m2->lb_name != NULL );

        return strcasecmp( m1->lb_name, m2->lb_name );
}

/*
 * Duplicate map ?
 */
static int
rewrite_builtin_map_dup(
	                void *c1,
	                void *c2
)
{
        struct rewrite_builtin_map *m1, *m2;

        m1 = ( struct rewrite_builtin_map * )c1;
        m2 = ( struct rewrite_builtin_map * )c2;

        assert( m1 != NULL );
        assert( m2 != NULL );
        assert( m1->lb_name != NULL );
        assert( m2->lb_name != NULL );

        return ( strcasecmp( m1->lb_name, m2->lb_name ) == 0 ? -1 : 0 );
}

/*
 * Adds a map to the info map tree
 */
static int
rewrite_builtin_map_insert(
		struct rewrite_info *info,
		struct rewrite_builtin_map *map
)
{
	/*
	 * May need a mutex?
	 */
	return avl_insert( &info->li_maps, ( caddr_t )map,
			rewrite_builtin_map_cmp,
		       	rewrite_builtin_map_dup );
}

/*
 * Retrieves a map
 */
struct rewrite_builtin_map *
rewrite_builtin_map_find(
		struct rewrite_info *info,
		const char *name
)
{
	struct rewrite_builtin_map tmp;

	assert( info != NULL );
	assert( name != NULL );

	tmp.lb_name = ( char * )name;

	return ( struct rewrite_builtin_map * )avl_find( info->li_maps,
			( caddr_t )&tmp, rewrite_builtin_map_cmp );
}

/*
 * Parses a plugin map
 */
static int
rewrite_parse_builtin_map(
		struct rewrite_info *info,
		const char *fname,
		int lineno,
		int argc,
		char **argv
)
{
	struct rewrite_builtin_map *map;
	
#define MAP_TYPE	1
#define MAP_NAME	2
	
	assert( info != NULL );
	assert( fname != NULL );
	assert( argc > 2 );
	assert( argv != NULL );
	assert( strcasecmp( argv[ 0 ], "rewriteMap" ) == 0 );

	map = calloc( sizeof( struct rewrite_builtin_map ), 1 );
	if ( map == NULL ) {
		return REWRITE_ERR;
	}

	map->lb_name = strdup( argv[ MAP_NAME ] );
	if ( map->lb_name == NULL ) {
		free( map );
		return REWRITE_ERR;
	}
	
	/*
	 * Built-in ldap map
	 */
	if (( map->lb_mapper = rewrite_mapper_find( argv[ MAP_TYPE ] ))) {
		map->lb_type = REWRITE_BUILTIN_MAP;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		if ( ldap_pvt_thread_mutex_init( & map->lb_mutex ) ) {
			free( map->lb_name );
			free( map );
			return REWRITE_ERR;
		}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
		
		map->lb_private = map->lb_mapper->rm_config( fname, lineno,
				argc - 3, argv + 3 );
		
	/* 
	 * Error
	 */	
	} else {
		free( map );
		Debug( LDAP_DEBUG_ANY, "[%s:%d] unknown map type\n%s",
				fname, lineno, "" );
		return -1;
	}

	return rewrite_builtin_map_insert( info, map );
}
