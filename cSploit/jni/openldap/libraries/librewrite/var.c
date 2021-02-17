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
 * Compares two vars
 */
static int
rewrite_var_cmp(
		const void *c1,
		const void *c2
)
{
	const struct rewrite_var *v1, *v2;

	v1 = ( const struct rewrite_var * )c1;
	v2 = ( const struct rewrite_var * )c2;
	
	assert( v1 != NULL );
	assert( v2 != NULL );
	assert( v1->lv_name != NULL );
	assert( v2->lv_name != NULL );

	return strcasecmp( v1->lv_name, v2->lv_name );
}

/*
 * Duplicate var ?
 */
static int
rewrite_var_dup(
		void *c1,
		void *c2
)
{
	struct rewrite_var *v1, *v2;

	v1 = ( struct rewrite_var * )c1;
	v2 = ( struct rewrite_var * )c2;

	assert( v1 != NULL );
	assert( v2 != NULL );
	assert( v1->lv_name != NULL );
	assert( v2->lv_name != NULL );

	return ( strcasecmp( v1->lv_name, v2->lv_name ) == 0 ? -1 : 0 );
}

/*
 * Frees a var
 */
static void 
rewrite_var_free(
		void *v_var
)
{
	struct rewrite_var *var = v_var;
	assert( var != NULL );

	assert( var->lv_name != NULL );
	assert( var->lv_value.bv_val != NULL );

	if ( var->lv_flags & REWRITE_VAR_COPY_NAME )
		free( var->lv_name );
	if ( var->lv_flags & REWRITE_VAR_COPY_VALUE )
		free( var->lv_value.bv_val );
	free( var );
}

/*
 * Deletes a var tree
 */
int
rewrite_var_delete(
		Avlnode *tree
)
{
	avl_free( tree, rewrite_var_free );
	return REWRITE_SUCCESS;
}

/*
 * Finds a var
 */
struct rewrite_var *
rewrite_var_find(
		Avlnode *tree,
		const char *name
)
{
	struct rewrite_var var;

	assert( name != NULL );

	var.lv_name = ( char * )name;
	return ( struct rewrite_var * )avl_find( tree, 
			( caddr_t )&var, rewrite_var_cmp );
}

int
rewrite_var_replace(
		struct rewrite_var *var,
		const char *value,
		int flags
)
{
	ber_len_t	len;

	assert( value != NULL );

	len = strlen( value );

	if ( var->lv_flags & REWRITE_VAR_COPY_VALUE ) {
		if ( flags & REWRITE_VAR_COPY_VALUE ) {
			if ( len <= var->lv_value.bv_len ) {
				AC_MEMCPY(var->lv_value.bv_val, value, len + 1);

			} else {
				free( var->lv_value.bv_val );
				var->lv_value.bv_val = strdup( value );
			}

		} else {
			free( var->lv_value.bv_val );
			var->lv_value.bv_val = (char *)value;
			var->lv_flags &= ~REWRITE_VAR_COPY_VALUE;
		}

	} else {
		if ( flags & REWRITE_VAR_COPY_VALUE ) {
			var->lv_value.bv_val = strdup( value );
			var->lv_flags |= REWRITE_VAR_COPY_VALUE;

		} else {
			var->lv_value.bv_val = (char *)value;
		}
	}

	if ( var->lv_value.bv_val == NULL ) {
		return -1;
	}

	var->lv_value.bv_len = len;

	return 0;
}

/*
 * Inserts a newly created var
 */
struct rewrite_var *
rewrite_var_insert_f(
		Avlnode **tree,
		const char *name,
		const char *value,
		int flags
)
{
	struct rewrite_var *var;
	int rc = 0;

	assert( tree != NULL );
	assert( name != NULL );
	assert( value != NULL );
	
	var = rewrite_var_find( *tree, name );
	if ( var != NULL ) {
		if ( flags & REWRITE_VAR_UPDATE ) {
			(void)rewrite_var_replace( var, value, flags );
			goto cleanup;
		}
		rc = -1;
		goto cleanup;
	}

	var = calloc( sizeof( struct rewrite_var ), 1 );
	if ( var == NULL ) {
		return NULL;
	}

	memset( var, 0, sizeof( struct rewrite_var ) );

	if ( flags & REWRITE_VAR_COPY_NAME ) {
		var->lv_name = strdup( name );
		if ( var->lv_name == NULL ) {
			rc = -1;
			goto cleanup;
		}
		var->lv_flags |= REWRITE_VAR_COPY_NAME;

	} else {
		var->lv_name = (char *)name;
	}

	if ( flags & REWRITE_VAR_COPY_VALUE ) {
		var->lv_value.bv_val = strdup( value );
		if ( var->lv_value.bv_val == NULL ) {
			rc = -1;
			goto cleanup;
		}
		var->lv_flags |= REWRITE_VAR_COPY_VALUE;
		
	} else {
		var->lv_value.bv_val = (char *)value;
	}
	var->lv_value.bv_len = strlen( value );
	rc = avl_insert( tree, ( caddr_t )var,
			rewrite_var_cmp, rewrite_var_dup );

cleanup:;
	if ( rc != 0 && var ) {
		avl_delete( tree, ( caddr_t )var, rewrite_var_cmp );
		rewrite_var_free( var );
		var = NULL;
	}

	return var;
}

/*
 * Sets/inserts a var
 */
struct rewrite_var *
rewrite_var_set_f(
		Avlnode **tree,
		const char *name,
		const char *value,
		int flags
)
{
	struct rewrite_var *var;

	assert( tree != NULL );
	assert( name != NULL );
	assert( value != NULL );
	
	var = rewrite_var_find( *tree, name );
	if ( var == NULL ) {
		if ( flags & REWRITE_VAR_INSERT ) {
			return rewrite_var_insert_f( tree, name, value, flags );

		} else {
			return NULL;
		}

	} else {
		assert( var->lv_value.bv_val != NULL );

		(void)rewrite_var_replace( var, value, flags );
	}

	return var;
}

