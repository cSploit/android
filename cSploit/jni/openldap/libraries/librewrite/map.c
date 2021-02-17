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

#include "rewrite-int.h"
#include "rewrite-map.h"

static int num_mappers;
static const rewrite_mapper **mappers;
#define	MAPPER_ALLOC	8

struct rewrite_map *
rewrite_map_parse(
		struct rewrite_info *info,
		const char *string,
		const char **currpos
)
{
	struct rewrite_map *map = NULL;
	struct rewrite_subst *subst = NULL;
	char *s, *begin = NULL, *end;
	const char *p;
	int l, cnt, mtx = 0, rc = 0;

	assert( info != NULL );
	assert( string != NULL );
	assert( currpos != NULL );

	*currpos = NULL;

	/*
	 * Go to the end of the map invocation (the right closing brace)
	 */
	for ( p = string, cnt = 1; p[ 0 ] != '\0' && cnt > 0; p++ ) {
		if ( IS_REWRITE_SUBMATCH_ESCAPE( p[ 0 ] ) ) {
			/*
			 * '%' marks the beginning of a new map
			 */
			if ( p[ 1 ] == '{' ) {
				cnt++;
			/*
			 * '%' followed by a digit may mark the beginning
			 * of an old map
			 */
			} else if ( isdigit( (unsigned char) p[ 1 ] ) && p[ 2 ] == '{' ) {
				cnt++;
				p++;
			}

			if ( p[ 1 ] != '\0' ) {
				p++;
			}

		} else if ( p[ 0 ] == '}' ) {
			cnt--;
		}
	}
	if ( cnt != 0 ) {
		return NULL;
	}
	*currpos = p;
	
	/*
	 * Copy the map invocation
	 */
	l = p - string - 1;
	s = calloc( sizeof( char ), l + 1 );
	if ( s == NULL ) {
		return NULL;
	}
	AC_MEMCPY( s, string, l );
	s[ l ] = 0;

	/*
	 * Isolate the map name (except for variable deref)
	 */
	switch ( s[ 0 ] ) {
	case REWRITE_OPERATOR_VARIABLE_GET:
	case REWRITE_OPERATOR_PARAM_GET:
		break;

	default:
		begin = strchr( s, '(' );
		if ( begin == NULL ) {
			rc = -1;
			goto cleanup;
		}
		begin[ 0 ] = '\0';
		begin++;
		break;
	}

	/*
	 * Check for special map types
	 */
	p = s;
	switch ( p[ 0 ] ) {
	case REWRITE_OPERATOR_SUBCONTEXT:
	case REWRITE_OPERATOR_COMMAND:
	case REWRITE_OPERATOR_VARIABLE_SET:
	case REWRITE_OPERATOR_VARIABLE_GET:
	case REWRITE_OPERATOR_PARAM_GET:
		p++;
		break;
	}

	/*
	 * Variable set and get may be repeated to indicate session-wide
	 * instead of operation-wide variables
	 */
	switch ( p[ 0 ] ) {
        case REWRITE_OPERATOR_VARIABLE_SET:
	case REWRITE_OPERATOR_VARIABLE_GET:
		p++;
		break;
	}

	/*
	 * Variable get token can be appended to variable set to mean store
	 * AND rewrite
	 */
	if ( p[ 0 ] == REWRITE_OPERATOR_VARIABLE_GET ) {
		p++;
	}
	
	/*
	 * Check the syntax of the variable name
	 */
	if ( !isalpha( (unsigned char) p[ 0 ] ) ) {
		rc = -1;
		goto cleanup;
	}
	for ( p++; p[ 0 ] != '\0'; p++ ) {
		if ( !isalnum( (unsigned char) p[ 0 ] ) ) {
			rc = -1;
			goto cleanup;
		}
	}

	/*
	 * Isolate the argument of the map (except for variable deref)
	 */
	switch ( s[ 0 ] ) {
	case REWRITE_OPERATOR_VARIABLE_GET:
	case REWRITE_OPERATOR_PARAM_GET:
		break;

	default:
		end = strrchr( begin, ')' );
		if ( end == NULL ) {
			rc = -1;
			goto cleanup;
		}
		end[ 0 ] = '\0';

		/*
	 	 * Compile the substitution pattern of the map argument
	 	 */
		subst = rewrite_subst_compile( info, begin );
		if ( subst == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;
	}

	/*
	 * Create the map
	 */
	map = calloc( sizeof( struct rewrite_map ), 1 );
	if ( map == NULL ) {
		rc = -1;
		goto cleanup;
	}
	memset( map, 0, sizeof( struct rewrite_map ) );
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
        if ( ldap_pvt_thread_mutex_init( &map->lm_mutex ) ) {
		rc = -1;
		goto cleanup;
	}
	++mtx;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
			
	/*
	 * No subst for variable deref
	 */
	switch ( s[ 0 ] ) {
	case REWRITE_OPERATOR_VARIABLE_GET:
	case REWRITE_OPERATOR_PARAM_GET:
		break;

	default:
		map->lm_subst = subst;
		break;
	}

	/*
	 * Parses special map types
	 */
	switch ( s[ 0 ] ) {
	
	/*
	 * Subcontext
	 */
	case REWRITE_OPERATOR_SUBCONTEXT:		/* '>' */

		/*
		 * Fetch the rewrite context
		 * it MUST have been defined previously
		 */
		map->lm_type = REWRITE_MAP_SUBCONTEXT;
		map->lm_name = strdup( s + 1 );
		if ( map->lm_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		map->lm_data = rewrite_context_find( info, s + 1 );
		if ( map->lm_data == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;

	/*
	 * External command (not implemented yet)
	 */
	case REWRITE_OPERATOR_COMMAND:		/* '|' */
		rc = -1;
		goto cleanup;
	
	/*
	 * Variable set
	 */
	case REWRITE_OPERATOR_VARIABLE_SET:	/* '&' */
		if ( s[ 1 ] == REWRITE_OPERATOR_VARIABLE_SET ) {
			if ( s[ 2 ] == REWRITE_OPERATOR_VARIABLE_GET ) {
				map->lm_type = REWRITE_MAP_SETW_SESN_VAR;
				map->lm_name = strdup( s + 3 );
			} else {
				map->lm_type = REWRITE_MAP_SET_SESN_VAR;
				map->lm_name = strdup( s + 2 );
			}
		} else {
			if ( s[ 1 ] == REWRITE_OPERATOR_VARIABLE_GET ) {
				map->lm_type = REWRITE_MAP_SETW_OP_VAR;
				map->lm_name = strdup( s + 2 );
			} else {
				map->lm_type = REWRITE_MAP_SET_OP_VAR;
				map->lm_name = strdup( s + 1 );
			}
		}
		if ( map->lm_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;
	
	/*
	 * Variable dereference
	 */
	case REWRITE_OPERATOR_VARIABLE_GET:	/* '*' */
		if ( s[ 1 ] == REWRITE_OPERATOR_VARIABLE_GET ) {
			map->lm_type = REWRITE_MAP_GET_SESN_VAR;
			map->lm_name = strdup( s + 2 );
		} else {
			map->lm_type = REWRITE_MAP_GET_OP_VAR;
			map->lm_name = strdup( s + 1 );
		}
		if ( map->lm_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;
	
	/*
	 * Parameter
	 */
	case REWRITE_OPERATOR_PARAM_GET:		/* '$' */
		map->lm_type = REWRITE_MAP_GET_PARAM;
		map->lm_name = strdup( s + 1 );
		if ( map->lm_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;
	
	/*
	 * Built-in map
	 */
	default:
		map->lm_type = REWRITE_MAP_BUILTIN;
		map->lm_name = strdup( s );
		if ( map->lm_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		map->lm_data = rewrite_builtin_map_find( info, s );
		if ( map->lm_data == NULL ) {
			rc = -1;
			goto cleanup;
		}
		break;

	}

cleanup:
	free( s );
	if ( rc ) {
		if ( subst != NULL ) {
			free( subst );
		}
		if ( map ) {
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		        if ( mtx ) {
				ldap_pvt_thread_mutex_destroy( &map->lm_mutex );
			}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

			if ( map->lm_name ) {
				free( map->lm_name );
				map->lm_name = NULL;
			}
			free( map );
			map = NULL;
		}
	}

	return map;
}

/*
 * Applies the new map type
 */
int
rewrite_map_apply(
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
	case REWRITE_MAP_SUBCONTEXT:
		rc = rewrite_context_apply( info, op, 
				( struct rewrite_context * )map->lm_data,
				key->bv_val, &val->bv_val );
		if ( val->bv_val != NULL ) {
			if ( val->bv_val == key->bv_val ) {
				val->bv_len = key->bv_len;
				key->bv_val = NULL;
			} else {
				val->bv_len = strlen( val->bv_val );
			}
		}
		break;

	case REWRITE_MAP_SET_OP_VAR:
	case REWRITE_MAP_SETW_OP_VAR:
		rc = rewrite_var_set( &op->lo_vars, map->lm_name,
				key->bv_val, 1 )
			? REWRITE_SUCCESS : REWRITE_ERR;
		if ( rc == REWRITE_SUCCESS ) {
			if ( map->lm_type == REWRITE_MAP_SET_OP_VAR ) {
				val->bv_val = strdup( "" );
			} else {
				val->bv_val = strdup( key->bv_val );
				val->bv_len = key->bv_len;
			}
			if ( val->bv_val == NULL ) {
				rc = REWRITE_ERR;
			}
		}
		break;
	
	case REWRITE_MAP_GET_OP_VAR: {
		struct rewrite_var *var;

		var = rewrite_var_find( op->lo_vars, map->lm_name );
		if ( var == NULL ) {
			rc = REWRITE_ERR;
		} else {
			val->bv_val = strdup( var->lv_value.bv_val );
			val->bv_len = var->lv_value.bv_len;
			if ( val->bv_val == NULL ) {
				rc = REWRITE_ERR;
			}
		}
		break;	
	}

	case REWRITE_MAP_SET_SESN_VAR:
	case REWRITE_MAP_SETW_SESN_VAR:
		if ( op->lo_cookie == NULL ) {
			rc = REWRITE_ERR;
			break;
		}
		rc = rewrite_session_var_set( info, op->lo_cookie, 
				map->lm_name, key->bv_val );
		if ( rc == REWRITE_SUCCESS ) {
			if ( map->lm_type == REWRITE_MAP_SET_SESN_VAR ) {
				val->bv_val = strdup( "" );
			} else {
				val->bv_val = strdup( key->bv_val );
				val->bv_len = key->bv_len;
			}
			if ( val->bv_val == NULL ) {
				rc = REWRITE_ERR;
			}
		}
		break;

	case REWRITE_MAP_GET_SESN_VAR:
		rc = rewrite_session_var_get( info, op->lo_cookie,
				map->lm_name, val );
		break;		

	case REWRITE_MAP_GET_PARAM:
		rc = rewrite_param_get( info, map->lm_name, val );
		break;

	case REWRITE_MAP_BUILTIN: {
		struct rewrite_builtin_map *bmap = map->lm_data;

		if ( bmap->lb_mapper && bmap->lb_mapper->rm_apply )
			rc = bmap->lb_mapper->rm_apply( bmap->lb_private, key->bv_val,
				val );
		else
			rc = REWRITE_ERR;
			break;
		break;
	}

	default:
		rc = REWRITE_ERR;
		break;
	}

	return rc;
}

void
rewrite_builtin_map_free(
		void *tmp
)
{
	struct rewrite_builtin_map *map = ( struct rewrite_builtin_map * )tmp;

	assert( map != NULL );

	if ( map->lb_mapper && map->lb_mapper->rm_destroy )
		map->lb_mapper->rm_destroy( map->lb_private );

	free( map->lb_name );
	free( map );
}

int
rewrite_map_destroy(
		struct rewrite_map **pmap
)
{
	struct rewrite_map *map;
	
	assert( pmap != NULL );
	assert( *pmap != NULL );

	map = *pmap;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_mutex_lock( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	if ( map->lm_name ) {
		free( map->lm_name );
		map->lm_name = NULL;
	}

	if ( map->lm_subst ) {
		rewrite_subst_destroy( &map->lm_subst );
	}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_mutex_unlock( &map->lm_mutex );
	ldap_pvt_thread_mutex_destroy( &map->lm_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	free( map );
	*pmap = NULL;
	
	return 0;
}

/* ldapmap.c */
extern const rewrite_mapper rewrite_ldap_mapper;

const rewrite_mapper *
rewrite_mapper_find(
	const char *name
)
{
	int i;

	if ( !strcasecmp( name, "ldap" ))
		return &rewrite_ldap_mapper;

	for (i=0; i<num_mappers; i++)
		if ( !strcasecmp( name, mappers[i]->rm_name ))
			return mappers[i];
	return NULL;
}

int
rewrite_mapper_register(
	const rewrite_mapper *map
)
{
	if ( num_mappers % MAPPER_ALLOC == 0 ) {
		const rewrite_mapper **mnew;
		mnew = realloc( mappers, (num_mappers + MAPPER_ALLOC) *
			sizeof( rewrite_mapper * ));
		if ( mnew )
			mappers = mnew;
		else
			return -1;
	}
	mappers[num_mappers++] = map;
	return 0;
}

int
rewrite_mapper_unregister(
	const rewrite_mapper *map
)
{
	int i;

	for (i = 0; i<num_mappers; i++) {
		if ( mappers[i] == map ) {
			num_mappers--;
			mappers[i] = mappers[num_mappers];
			mappers[num_mappers] = NULL;
			return 0;
		}
	}
	/* not found */
	return -1;
}
