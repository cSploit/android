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
 * Defines and inits a variable with global scope
 */
int
rewrite_param_set(
		struct rewrite_info *info,
		const char *name,
		const char *value
)
{
	struct rewrite_var *var;
	int rc = REWRITE_SUCCESS;

	assert( info != NULL );
	assert( name != NULL );
	assert( value != NULL );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	var = rewrite_var_find( info->li_params, name );
	if ( var != NULL ) {
		assert( var->lv_value.bv_val != NULL );
		free( var->lv_value.bv_val );
		var->lv_value.bv_val = strdup( value );
		var->lv_value.bv_len = strlen( value );

	} else {
		var = rewrite_var_insert( &info->li_params, name, value );
	}

	if ( var == NULL || var->lv_value.bv_val == NULL ) {
		rc = REWRITE_ERR;
	}
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return rc;
}

/*
 * Gets a var with global scope
 */
int
rewrite_param_get(
		struct rewrite_info *info,
		const char *name,
		struct berval *value
)
{
	struct rewrite_var *var;
	int rc = REWRITE_SUCCESS;

	assert( info != NULL );
	assert( name != NULL );
	assert( value != NULL );

	value->bv_val = NULL;
	value->bv_len = 0;
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_rlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	
	var = rewrite_var_find( info->li_params, name );
	if ( var != NULL ) {
		value->bv_val = strdup( var->lv_value.bv_val );
		value->bv_len = var->lv_value.bv_len;
	}

	if ( var == NULL || value->bv_val == NULL ) {
		rc = REWRITE_ERR;
	}
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_runlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return REWRITE_SUCCESS;
}

static void
rewrite_param_free(
		void *tmp
)
{
	struct rewrite_var *var = ( struct rewrite_var * )tmp;
	assert( var != NULL );

	assert( var->lv_name != NULL );
	assert( var->lv_value.bv_val != NULL );

	free( var->lv_name );
	free( var->lv_value.bv_val );
	free( var );
}

/*
 * Destroys the parameter tree
 */
int
rewrite_param_destroy(
		struct rewrite_info *info
)
{
	int count;

	assert( info != NULL );
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	
	count = avl_free( info->li_params, rewrite_param_free );
	info->li_params = NULL;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return REWRITE_SUCCESS;
}

