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
 * Global data
 */

/*
 * This becomes the running context for subsequent calls to
 * rewrite_parse; it can be altered only by a 
 * rewriteContext config line or by a change in info.
 */
struct rewrite_context *rewrite_int_curr_context = NULL;

/*
 * Inits the info
 */
struct rewrite_info *
rewrite_info_init(
		int mode
)
{
	struct rewrite_info *info;
	struct rewrite_context *context;

	switch ( mode ) {
	case REWRITE_MODE_ERR:
	case REWRITE_MODE_OK:
	case REWRITE_MODE_COPY_INPUT:
	case REWRITE_MODE_USE_DEFAULT:
		break;
	default:
		mode = REWRITE_MODE_USE_DEFAULT;
		break;
		/* return NULL */
	}

	/*
	 * Resets the running context for parsing ...
	 */
	rewrite_int_curr_context = NULL;

	info = calloc( sizeof( struct rewrite_info ), 1 );
	if ( info == NULL ) {
		return NULL;
	}

	info->li_state = REWRITE_DEFAULT;
	info->li_max_passes = REWRITE_MAX_PASSES;
	info->li_max_passes_per_rule = REWRITE_MAX_PASSES;
	info->li_rewrite_mode = mode;

	/*
	 * Add the default (empty) rule
	 */
	context = rewrite_context_create( info, REWRITE_DEFAULT_CONTEXT );
	if ( context == NULL ) {
		free( info );
		return NULL;
	}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	if ( ldap_pvt_thread_rdwr_init( &info->li_cookies_mutex ) ) {
		avl_free( info->li_context, rewrite_context_free );
		free( info );
		return NULL;
	}
	if ( ldap_pvt_thread_rdwr_init( &info->li_params_mutex ) ) {
		ldap_pvt_thread_rdwr_destroy( &info->li_cookies_mutex );
		avl_free( info->li_context, rewrite_context_free );
		free( info );
		return NULL;
	}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	
	return info;
}

/*
 * Cleans up the info structure
 */
int
rewrite_info_delete(
		struct rewrite_info **pinfo
)
{
	struct rewrite_info	*info;

	assert( pinfo != NULL );
	assert( *pinfo != NULL );

	info = *pinfo;
	
	if ( info->li_context ) {
		avl_free( info->li_context, rewrite_context_free );
	}
	info->li_context = NULL;

	if ( info->li_maps ) {
		avl_free( info->li_maps, rewrite_builtin_map_free );
	}
	info->li_maps = NULL;

	rewrite_session_destroy( info );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_destroy( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	rewrite_param_destroy( info );
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_destroy( &info->li_params_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	free( info );
	*pinfo = NULL;

	return REWRITE_SUCCESS;
}

/*
 * Rewrites a string according to context.
 * If the engine is off, OK is returned, but the return string will be NULL.
 * In case of 'unwilling to perform', UNWILLING is returned, and the
 * return string will also be null. The same in case of error.
 * Otherwise, OK is returned, and result will hold a newly allocated string
 * with the rewriting.
 * 
 * What to do in case of non-existing rewrite context is still an issue.
 * Four possibilities:
 * 	- error, 
 * 	- ok with NULL result, 
 * 	- ok with copy of string as result,
 * 	- use the default rewrite context.
 */
int
rewrite(
		struct rewrite_info *info,
		const char *rewriteContext,
		const char *string,
		char **result
)
{
	return rewrite_session( info, rewriteContext, 
			string, NULL, result );
}

int
rewrite_session(
		struct rewrite_info *info,
		const char *rewriteContext,
		const char *string,
		const void *cookie,
		char **result
)
{
	struct rewrite_context *context;
	struct rewrite_op op = { 0, 0, NULL, NULL, NULL };
	int rc;
	
	assert( info != NULL );
	assert( rewriteContext != NULL );
	assert( string != NULL );
	assert( result != NULL );

	/*
	 * cookie can be null; means: don't care about session stuff
	 */

	*result = NULL;
	op.lo_cookie = cookie;
	
	/*
	 * Engine not on means no failure, but explicit no rewriting
	 */
	if ( info->li_state != REWRITE_ON ) {
		rc = REWRITE_REGEXEC_OK;
		goto rc_return;
	}
	
	/*
	 * Undefined context means no rewriting also
	 * (conservative, are we sure it's what we want?)
	 */
	context = rewrite_context_find( info, rewriteContext );
	if ( context == NULL ) {
		switch ( info->li_rewrite_mode ) {
		case REWRITE_MODE_ERR:
			rc = REWRITE_REGEXEC_ERR;
			goto rc_return;
			
		case REWRITE_MODE_OK:
			rc = REWRITE_REGEXEC_OK;
			goto rc_return;

		case REWRITE_MODE_COPY_INPUT:
			*result = strdup( string );
			rc = ( *result != NULL ) ? REWRITE_REGEXEC_OK : REWRITE_REGEXEC_ERR;
			goto rc_return;

		case REWRITE_MODE_USE_DEFAULT:
			context = rewrite_context_find( info,
					REWRITE_DEFAULT_CONTEXT );
			break;
		}
	}

#if 0 /* FIXME: not used anywhere! (debug? then, why strdup?) */
	op.lo_string = strdup( string );
	if ( op.lo_string == NULL ) {
		rc = REWRITE_REGEXEC_ERR;
		goto rc_return;
	}
#endif
	
	/*
	 * Applies rewrite context
	 */
	rc = rewrite_context_apply( info, &op, context, string, result );
	assert( op.lo_depth == 0 );

#if 0 /* FIXME: not used anywhere! (debug? then, why strdup?) */	
	free( op.lo_string );
#endif
	
	switch ( rc ) {
	/*
	 * Success
	 */
	case REWRITE_REGEXEC_OK:
	case REWRITE_REGEXEC_STOP:
		/*
		 * If rewrite succeeded return OK regardless of how
		 * the successful rewriting was obtained!
		 */
		rc = REWRITE_REGEXEC_OK;
		break;
		
	
	/*
	 * Internal or forced error, return = NULL; rc already OK.
	 */
	case REWRITE_REGEXEC_UNWILLING:
	case REWRITE_REGEXEC_ERR:
		if ( *result != NULL ) {
			if ( *result != string ) {
				free( *result );
			}
			*result = NULL;
		}

	default:
		break;
	}

rc_return:;
	if ( op.lo_vars ) {
		rewrite_var_delete( op.lo_vars );
	}
	
	return rc;
}

