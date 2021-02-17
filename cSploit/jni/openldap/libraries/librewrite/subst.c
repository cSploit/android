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

/*
 * Compiles a substitution pattern
 */
struct rewrite_subst *
rewrite_subst_compile(
		struct rewrite_info *info,
		const char *str
)
{
	size_t subs_len;
	struct berval *subs = NULL, *tmps;
	struct rewrite_submatch *submatch = NULL;

	struct rewrite_subst *s = NULL;

	char *result, *begin, *p;
	int nsub = 0, l;

	assert( info != NULL );
	assert( str != NULL );

	result = strdup( str );
	if ( result == NULL ) {
		return NULL;
	}

	/*
	 * Take care of substitution string
	 */
	for ( p = begin = result, subs_len = 0; p[ 0 ] != '\0'; p++ ) {

		/*
		 * Keep only single escapes '%'
		 */
		if (  !IS_REWRITE_SUBMATCH_ESCAPE( p[ 0 ] ) ) {
			continue;
		} 

		if (  IS_REWRITE_SUBMATCH_ESCAPE( p[ 1 ] ) ) {
			/* Pull &p[1] over p, including the trailing '\0' */
			AC_MEMCPY((char *)p, &p[ 1 ], strlen( p ) );
			continue;
		}

		tmps = ( struct berval * )realloc( subs,
				sizeof( struct berval )*( nsub + 1 ) );
		if ( tmps == NULL ) {
			goto cleanup;
		}
		subs = tmps;
		
		/*
		 * I think an `if l > 0' at runtime is better outside than
		 * inside a function call ...
		 */
		l = p - begin;
		if ( l > 0 ) {
			subs_len += l;
			subs[ nsub ].bv_len = l;
			subs[ nsub ].bv_val = malloc( l + 1 );
			if ( subs[ nsub ].bv_val == NULL ) {
				goto cleanup;
			}
			AC_MEMCPY( subs[ nsub ].bv_val, begin, l );
			subs[ nsub ].bv_val[ l ] = '\0';
		} else {
			subs[ nsub ].bv_val = NULL;
			subs[ nsub ].bv_len = 0;
		}
		
		/*
		 * Substitution pattern
		 */
		if ( isdigit( (unsigned char) p[ 1 ] ) ) {
			struct rewrite_submatch *tmpsm;
			int d = p[ 1 ] - '0';

			/*
			 * Add a new value substitution scheme
			 */

			tmpsm = ( struct rewrite_submatch * )realloc( submatch,
					sizeof( struct rewrite_submatch )*( nsub + 1 ) );
			if ( tmpsm == NULL ) {
				goto cleanup;
			}
			submatch = tmpsm;
			submatch[ nsub ].ls_submatch = d;

			/*
			 * If there is no argument, use default
			 * (substitute substring as is)
			 */
			if ( p[ 2 ] != '{' ) {
				submatch[ nsub ].ls_type = 
					REWRITE_SUBMATCH_ASIS;
				submatch[ nsub ].ls_map = NULL;
				begin = ++p + 1;

			} else {
				struct rewrite_map *map;

				submatch[ nsub ].ls_type =
					REWRITE_SUBMATCH_XMAP;

				map = rewrite_xmap_parse( info,
						p + 3, (const char **)&begin );
				if ( map == NULL ) {
					goto cleanup;
				}
				submatch[ nsub ].ls_map = map;
				p = begin - 1;
			}

		/*
		 * Map with args ...
		 */
		} else if ( p[ 1 ] == '{' ) {
			struct rewrite_map *map;
			struct rewrite_submatch *tmpsm;

			map = rewrite_map_parse( info, p + 2,
					(const char **)&begin );
			if ( map == NULL ) {
				goto cleanup;
			}
			p = begin - 1;

			/*
			 * Add a new value substitution scheme
			 */
			tmpsm = ( struct rewrite_submatch * )realloc( submatch,
					sizeof( struct rewrite_submatch )*( nsub + 1 ) );
			if ( tmpsm == NULL ) {
				goto cleanup;
			}
			submatch = tmpsm;
			submatch[ nsub ].ls_type =
				REWRITE_SUBMATCH_MAP_W_ARG;
			submatch[ nsub ].ls_map = map;

		/*
		 * Escape '%' ...
		 */
		} else if ( p[ 1 ] == '%' ) {
			AC_MEMCPY( &p[ 1 ], &p[ 2 ], strlen( &p[ 1 ] ) );
			continue;

		} else {
			goto cleanup;
		}

		nsub++;
	}
	
	/*
	 * Last part of string
	 */
	tmps = (struct berval * )realloc( subs, sizeof( struct berval )*( nsub + 1 ) );
	if ( tmps == NULL ) {
		/*
		 * XXX need to free the value subst stuff!
		 */
		free( subs );
		goto cleanup;
	}
	subs = tmps;
	l = p - begin;
	if ( l > 0 ) {
		subs_len += l;
		subs[ nsub ].bv_len = l;
		subs[ nsub ].bv_val = malloc( l + 1 );
		if ( subs[ nsub ].bv_val == NULL ) {
			free( subs );
			goto cleanup;
		}
		AC_MEMCPY( subs[ nsub ].bv_val, begin, l );
		subs[ nsub ].bv_val[ l ] = '\0';
	} else {
		subs[ nsub ].bv_val = NULL;
		subs[ nsub ].bv_len = 0;
	}

	s = calloc( sizeof( struct rewrite_subst ), 1 );
	if ( s == NULL ) {
		goto cleanup;
	}

	s->lt_subs_len = subs_len;
        s->lt_subs = subs;
        s->lt_num_submatch = nsub;
        s->lt_submatch = submatch;

cleanup:;
	free( result );

	return s;
}

/*
 * Copies the match referred to by submatch and fetched in string by match.
 * Helper for rewrite_rule_apply.
 */
static int
submatch_copy(
		struct rewrite_submatch *submatch,
		const char *string,
		const regmatch_t *match,
		struct berval *val
)
{
	int		c, l;
	const char	*s;

	assert( submatch != NULL );
	assert( submatch->ls_type == REWRITE_SUBMATCH_ASIS
			|| submatch->ls_type == REWRITE_SUBMATCH_XMAP );
	assert( string != NULL );
	assert( match != NULL );
	assert( val != NULL );
	assert( val->bv_val == NULL );
	
	c = submatch->ls_submatch;
	s = string + match[ c ].rm_so;
	l = match[ c ].rm_eo - match[ c ].rm_so;
	
	val->bv_len = l;
	val->bv_val = malloc( l + 1 );
	if ( val->bv_val == NULL ) {
		return REWRITE_ERR;
	}
	
	AC_MEMCPY( val->bv_val, s, l );
	val->bv_val[ l ] = '\0';
	
	return REWRITE_SUCCESS;
}

/*
 * Substitutes a portion of rewritten string according to substitution
 * pattern using submatches
 */
int
rewrite_subst_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_subst *subst,
		const char *string,
		const regmatch_t *match,
		struct berval *val
)
{
	struct berval *submatch = NULL;
	char *res = NULL;
	int n = 0, l, cl;
	int rc = REWRITE_REGEXEC_OK;

	assert( info != NULL );
	assert( op != NULL );
	assert( subst != NULL );
	assert( string != NULL );
	assert( match != NULL );
	assert( val != NULL );

	assert( val->bv_val == NULL );

	val->bv_val = NULL;
	val->bv_len = 0;

	/*
	 * Prepare room for submatch expansion
	 */
	if ( subst->lt_num_submatch > 0 ) {
		submatch = calloc( sizeof( struct berval ),
				subst->lt_num_submatch );
		if ( submatch == NULL ) {
			return REWRITE_REGEXEC_ERR;
		}
	}
	
	/*
	 * Resolve submatches (simple subst, map expansion and so).
	 */
	for ( n = 0, l = 0; n < subst->lt_num_submatch; n++ ) {
		struct berval	key = { 0, NULL };

		submatch[ n ].bv_val = NULL;
		
		/*
		 * Get key
		 */
		switch ( subst->lt_submatch[ n ].ls_type ) {
		case REWRITE_SUBMATCH_ASIS:
		case REWRITE_SUBMATCH_XMAP:
			rc = submatch_copy( &subst->lt_submatch[ n ],
					string, match, &key );
			if ( rc != REWRITE_SUCCESS ) {
				rc = REWRITE_REGEXEC_ERR;
				goto cleanup;
			}
			break;
			
		case REWRITE_SUBMATCH_MAP_W_ARG:
			switch ( subst->lt_submatch[ n ].ls_map->lm_type ) {
			case REWRITE_MAP_GET_OP_VAR:
			case REWRITE_MAP_GET_SESN_VAR:
			case REWRITE_MAP_GET_PARAM:
				rc = REWRITE_SUCCESS;
				break;

			default:
				rc = rewrite_subst_apply( info, op, 
					subst->lt_submatch[ n ].ls_map->lm_subst,
					string, match, &key);
			}
			
			if ( rc != REWRITE_SUCCESS ) {
				goto cleanup;
			}
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "Not Implemented\n", 0, 0, 0 );
			rc = REWRITE_ERR;
			break;
		}
		
		if ( rc != REWRITE_SUCCESS ) {
			rc = REWRITE_REGEXEC_ERR;
			goto cleanup;
		}

		/*
		 * Resolve key
		 */
		switch ( subst->lt_submatch[ n ].ls_type ) {
		case REWRITE_SUBMATCH_ASIS:
			submatch[ n ] = key;
			rc = REWRITE_SUCCESS;
			break;
			
		case REWRITE_SUBMATCH_XMAP:
			rc = rewrite_xmap_apply( info, op,
					subst->lt_submatch[ n ].ls_map,
					&key, &submatch[ n ] );
			free( key.bv_val );
			key.bv_val = NULL;
			break;
			
		case REWRITE_SUBMATCH_MAP_W_ARG:
			rc = rewrite_map_apply( info, op,
					subst->lt_submatch[ n ].ls_map,
					&key, &submatch[ n ] );
			free( key.bv_val );
			key.bv_val = NULL;
			break;

		default:
			/*
			 * When implemented, this might return the
                         * exit status of a rewrite context,
                         * which may include a stop, or an
                         * unwilling to perform
                         */
			rc = REWRITE_ERR;
			break;
		}

		if ( rc != REWRITE_SUCCESS ) {
			rc = REWRITE_REGEXEC_ERR;
			goto cleanup;
		}
		
		/*
                 * Increment the length of the resulting string
                 */
		l += submatch[ n ].bv_len;
	}
	
	/*
         * Alloc result buffer
         */
	l += subst->lt_subs_len;
	res = malloc( l + 1 );
	if ( res == NULL ) {
		rc = REWRITE_REGEXEC_ERR;
		goto cleanup;
	}

	/*
	 * Apply submatches (possibly resolved thru maps)
	 */
        for ( n = 0, cl = 0; n < subst->lt_num_submatch; n++ ) {
		if ( subst->lt_subs[ n ].bv_val != NULL ) {
                	AC_MEMCPY( res + cl, subst->lt_subs[ n ].bv_val,
					subst->lt_subs[ n ].bv_len );
			cl += subst->lt_subs[ n ].bv_len;
		}
		AC_MEMCPY( res + cl, submatch[ n ].bv_val, 
				submatch[ n ].bv_len );
		cl += submatch[ n ].bv_len;
	}
	if ( subst->lt_subs[ n ].bv_val != NULL ) {
		AC_MEMCPY( res + cl, subst->lt_subs[ n ].bv_val,
				subst->lt_subs[ n ].bv_len );
		cl += subst->lt_subs[ n ].bv_len;
	}
	res[ cl ] = '\0';

	val->bv_val = res;
	val->bv_len = l;

cleanup:;
	if ( submatch ) {
        	for ( ; --n >= 0; ) {
			if ( submatch[ n ].bv_val ) {
				free( submatch[ n ].bv_val );
			}
		}
		free( submatch );
	}

	return rc;
}

/*
 * frees data
 */
int
rewrite_subst_destroy(
		struct rewrite_subst **psubst
)
{
	int			n;
	struct rewrite_subst	*subst;

	assert( psubst != NULL );
	assert( *psubst != NULL );

	subst = *psubst;

	for ( n = 0; n < subst->lt_num_submatch; n++ ) {
		if ( subst->lt_subs[ n ].bv_val ) {
			free( subst->lt_subs[ n ].bv_val );
			subst->lt_subs[ n ].bv_val = NULL;
		}

		switch ( subst->lt_submatch[ n ].ls_type ) {
		case REWRITE_SUBMATCH_ASIS:
			break;

		case REWRITE_SUBMATCH_XMAP:
			rewrite_xmap_destroy( &subst->lt_submatch[ n ].ls_map );
			break;

		case REWRITE_SUBMATCH_MAP_W_ARG:
			rewrite_map_destroy( &subst->lt_submatch[ n ].ls_map );
			break;

		default:
			break;
		}
	}

	free( subst->lt_submatch );
	subst->lt_submatch = NULL;

	/* last one */
	if ( subst->lt_subs[ n ].bv_val ) {
		free( subst->lt_subs[ n ].bv_val );
		subst->lt_subs[ n ].bv_val = NULL;
	}

	free( subst->lt_subs );
	subst->lt_subs = NULL;

	free( subst );
	*psubst = NULL;

	return 0;
}

