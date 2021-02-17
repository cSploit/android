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
 * Compares two cookies
 */
static int
rewrite_cookie_cmp(
                const void *c1,
                const void *c2
)
{
	const struct rewrite_session *s1, *s2;

	s1 = ( const struct rewrite_session * )c1;
	s2 = ( const struct rewrite_session * )c2;

	assert( s1 != NULL );
	assert( s2 != NULL );
	assert( s1->ls_cookie != NULL );
	assert( s2->ls_cookie != NULL );

        return ( ( s1->ls_cookie < s2->ls_cookie ) ? -1 :
			( ( s1->ls_cookie > s2->ls_cookie ) ? 1 : 0 ) );
}

/*
 * Duplicate cookies?
 */
static int
rewrite_cookie_dup(
                void *c1,
                void *c2
)
{
	struct rewrite_session *s1, *s2;

	s1 = ( struct rewrite_session * )c1;
	s2 = ( struct rewrite_session * )c2;
	
	assert( s1 != NULL );
	assert( s2 != NULL );
	assert( s1->ls_cookie != NULL );
	assert( s2->ls_cookie != NULL );
	
	assert( s1->ls_cookie != s2->ls_cookie );
	
        return ( ( s1->ls_cookie == s2->ls_cookie ) ? -1 : 0 );
}

/*
 * Inits a session
 */
struct rewrite_session *
rewrite_session_init(
		struct rewrite_info *info,
		const void *cookie
)
{
	struct rewrite_session 	*session, tmp;
	int			rc;

	assert( info != NULL );
	assert( cookie != NULL );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	tmp.ls_cookie = ( void * )cookie;
	session = ( struct rewrite_session * )avl_find( info->li_cookies, 
			( caddr_t )&tmp, rewrite_cookie_cmp );
	if ( session ) {
		session->ls_count++;
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_rdwr_wunlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
		return session;
	}
		
	session = calloc( sizeof( struct rewrite_session ), 1 );
	if ( session == NULL ) {
		return NULL;
	}
	session->ls_cookie = ( void * )cookie;
	session->ls_count = 1;
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	if ( ldap_pvt_thread_mutex_init( &session->ls_mutex ) ) {
		free( session );
		return NULL;
	}
	if ( ldap_pvt_thread_rdwr_init( &session->ls_vars_mutex ) ) {
		ldap_pvt_thread_mutex_destroy( &session->ls_mutex );
		free( session );
		return NULL;
	}
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	rc = avl_insert( &info->li_cookies, ( caddr_t )session,
			rewrite_cookie_cmp, rewrite_cookie_dup );
	info->li_num_cookies++;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
        ldap_pvt_thread_rdwr_wunlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	
	if ( rc != 0 ) {
#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_rdwr_destroy( &session->ls_vars_mutex );
		ldap_pvt_thread_mutex_destroy( &session->ls_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

		free( session );
		return NULL;
	}
	
	return session;
}

/*
 * Fetches a session
 */
struct rewrite_session *
rewrite_session_find(
		struct rewrite_info *info,
		const void *cookie
)
{
	struct rewrite_session *session, tmp;

	assert( info != NULL );
	assert( cookie != NULL );
	
	tmp.ls_cookie = ( void * )cookie;
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_rlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	session = ( struct rewrite_session * )avl_find( info->li_cookies,
			( caddr_t )&tmp, rewrite_cookie_cmp );
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	if ( session ) {
		ldap_pvt_thread_mutex_lock( &session->ls_mutex );
		session->ls_count++;
	}
	ldap_pvt_thread_rdwr_runlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return session;
}

/*
 * Returns a session
 */
void
rewrite_session_return(
		struct rewrite_info *info,
		struct rewrite_session *session
)
{
	assert( session != NULL );
	session->ls_count--;
	ldap_pvt_thread_mutex_unlock( &session->ls_mutex );
}

/*
 * Defines and inits a var with session scope
 */
int
rewrite_session_var_set_f(
		struct rewrite_info *info,
		const void *cookie,
		const char *name,
		const char *value,
		int flags
)
{
	struct rewrite_session *session;
	struct rewrite_var *var;

	assert( info != NULL );
	assert( cookie != NULL );
	assert( name != NULL );
	assert( value != NULL );

	session = rewrite_session_find( info, cookie );
	if ( session == NULL ) {
		session = rewrite_session_init( info, cookie );
		if ( session == NULL ) {
			return REWRITE_ERR;
		}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
		ldap_pvt_thread_mutex_lock( &session->ls_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	var = rewrite_var_find( session->ls_vars, name );
	if ( var != NULL ) {
		assert( var->lv_value.bv_val != NULL );

		(void)rewrite_var_replace( var, value, flags );

	} else {
		var = rewrite_var_insert_f( &session->ls_vars, name, value, flags );
		if ( var == NULL ) {
#ifdef USE_REWRITE_LDAP_PVT_THREADS
			ldap_pvt_thread_rdwr_wunlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
			rewrite_session_return( info, session );
			return REWRITE_ERR;
		}
	}	
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	rewrite_session_return( info, session );

	return REWRITE_SUCCESS;
}

/*
 * Gets a var with session scope
 */
int
rewrite_session_var_get(
		struct rewrite_info *info,
		const void *cookie,
		const char *name,
		struct berval *value
)
{
	struct rewrite_session *session;
	struct rewrite_var *var;
	int rc = REWRITE_SUCCESS;

	assert( info != NULL );
	assert( cookie != NULL );
	assert( name != NULL );
	assert( value != NULL );

	value->bv_val = NULL;
	value->bv_len = 0;
	
	if ( cookie == NULL ) {
		return REWRITE_ERR;
	}

	session = rewrite_session_find( info, cookie );
	if ( session == NULL ) {
		return REWRITE_ERR;
	}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_rlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	
	var = rewrite_var_find( session->ls_vars, name );
	if ( var != NULL ) {
		value->bv_val = strdup( var->lv_value.bv_val );
		value->bv_len = var->lv_value.bv_len;
	}

	if ( var == NULL || value->bv_val == NULL ) {
		rc = REWRITE_ERR;
	}

#ifdef USE_REWRITE_LDAP_PVT_THREADS
        ldap_pvt_thread_rdwr_runlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	rewrite_session_return( info, session );

	return rc;
}

static void
rewrite_session_clean( void *v_session )
{
	struct rewrite_session	*session = (struct rewrite_session *)v_session;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &session->ls_vars_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	rewrite_var_delete( session->ls_vars );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &session->ls_vars_mutex );
	ldap_pvt_thread_rdwr_destroy( &session->ls_vars_mutex );
	ldap_pvt_thread_mutex_unlock( &session->ls_mutex );
	ldap_pvt_thread_mutex_destroy( &session->ls_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
}

static void
rewrite_session_free( void *v_session )
{
	struct rewrite_session	*session = (struct rewrite_session *)v_session;

	ldap_pvt_thread_mutex_lock( &session->ls_mutex );
	rewrite_session_clean( v_session );
	free( v_session );
}

/*
 * Deletes a session
 */
int
rewrite_session_delete(
		struct rewrite_info *info,
		const void *cookie
)
{
	struct rewrite_session *session, tmp = { 0 };

	assert( info != NULL );
	assert( cookie != NULL );

	session = rewrite_session_find( info, cookie );

	if ( session == NULL ) {
		return REWRITE_SUCCESS;
	}

	if ( --session->ls_count > 0 ) {
		rewrite_session_return( info, session );
		return REWRITE_SUCCESS;
	}

	rewrite_session_clean( session );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	assert( info->li_num_cookies > 0 );
	info->li_num_cookies--;
	
	/*
	 * There is nothing to delete in the return value
	 */
	tmp.ls_cookie = ( void * )cookie;
	avl_delete( &info->li_cookies, ( caddr_t )&tmp, rewrite_cookie_cmp );

	free( session );

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return REWRITE_SUCCESS;
}

/*
 * Destroys the cookie tree
 */
int
rewrite_session_destroy(
		struct rewrite_info *info
)
{
	int count;

	assert( info != NULL );
	
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	/*
	 * Should call per-session destruction routine ...
	 */
	
	count = avl_free( info->li_cookies, rewrite_session_free );
	info->li_cookies = NULL;

#if 0
	fprintf( stderr, "count = %d; num_cookies = %d\n", 
			count, info->li_num_cookies );
#endif
	
	assert( count == info->li_num_cookies );
	info->li_num_cookies = 0;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_wunlock( &info->li_cookies_mutex );
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	return REWRITE_SUCCESS;
}

