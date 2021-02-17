/* thr_stub.c - stubs for the threads */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#if defined( NO_THREADS )

#include "ldap_pvt_thread.h" /* Get the thread interface */
#define LDAP_THREAD_IMPLEMENTATION
#define LDAP_THREAD_POOL_IMPLEMENTATION
#include "ldap_thr_debug.h"  /* May rename the symbols defined below */

/***********************************************************************
 *                                                                     *
 * no threads package defined for this system - fake ok returns from   *
 * all threads routines (making it single-threaded).                   *
 *                                                                     *
 ***********************************************************************/

int
ldap_int_thread_initialize( void )
{
	return 0;
}

int
ldap_int_thread_destroy( void )
{
	return 0;
}

static void* ldap_int_status = NULL;

int 
ldap_pvt_thread_create( ldap_pvt_thread_t * thread, 
	int detach,
	void *(*start_routine)(void *),
	void *arg)
{
	if( ! detach ) ldap_int_status = NULL;
	start_routine( arg );
	return 0;
}

void 
ldap_pvt_thread_exit( void *retval )
{
	if( retval != NULL ) {
		ldap_int_status = retval;
	}
	return;
}

int 
ldap_pvt_thread_join( ldap_pvt_thread_t thread, void **status )
{
	if(status != NULL) *status = ldap_int_status;
	return 0;
}

int 
ldap_pvt_thread_kill( ldap_pvt_thread_t thread, int signo )
{
	return 0;
}

int 
ldap_pvt_thread_yield( void )
{
	return 0;
}

int 
ldap_pvt_thread_cond_init( ldap_pvt_thread_cond_t *cond )
{
	return 0;
}

int 
ldap_pvt_thread_cond_destroy( ldap_pvt_thread_cond_t *cond )
{
	return 0;
}

int 
ldap_pvt_thread_cond_signal( ldap_pvt_thread_cond_t *cond )
{
	return 0;
}

int 
ldap_pvt_thread_cond_broadcast( ldap_pvt_thread_cond_t *cond )
{
	return 0;
}

int 
ldap_pvt_thread_cond_wait( ldap_pvt_thread_cond_t *cond,
			  ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

int 
ldap_pvt_thread_mutex_init( ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

int 
ldap_pvt_thread_mutex_destroy( ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

int 
ldap_pvt_thread_mutex_lock( ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

int 
ldap_pvt_thread_mutex_trylock( ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

int 
ldap_pvt_thread_mutex_unlock( ldap_pvt_thread_mutex_t *mutex )
{
	return 0;
}

/*
 * NO_THREADS requires a separate tpool implementation since
 * generic ldap_pvt_thread_pool_wrapper loops forever.
 */
int
ldap_pvt_thread_pool_init (
	ldap_pvt_thread_pool_t *pool_out,
	int max_concurrency, int max_pending )
{
	*pool_out = (ldap_pvt_thread_pool_t) 0;
	return(0);
}

int
ldap_pvt_thread_pool_submit (
	ldap_pvt_thread_pool_t *pool,
	ldap_pvt_thread_start_t *start_routine, void *arg )
{
	(start_routine)(NULL, arg);
	return(0);
}

int
ldap_pvt_thread_pool_retract (
	ldap_pvt_thread_pool_t *pool,
	ldap_pvt_thread_start_t *start_routine, void *arg )
{
	return(0);
}

int
ldap_pvt_thread_pool_maxthreads ( ldap_pvt_thread_pool_t *tpool, int max_threads )
{
	return(0);
}

int
ldap_pvt_thread_pool_query( ldap_pvt_thread_pool_t *tpool,
	ldap_pvt_thread_pool_param_t param, void *value )
{
	*(int *)value = -1;
	return(-1);
}

int
ldap_pvt_thread_pool_backload (
	ldap_pvt_thread_pool_t *pool )
{
	return(0);
}

int
ldap_pvt_thread_pool_destroy (
	ldap_pvt_thread_pool_t *pool, int run_pending )
{
	return(0);
}

void
ldap_pvt_thread_pool_idle ( ldap_pvt_thread_pool_t *pool )
{
	return;
}

void
ldap_pvt_thread_pool_unidle ( ldap_pvt_thread_pool_t *pool )
{
	return;
}

int ldap_pvt_thread_pool_getkey (
	void *ctx, void *key, void **data, ldap_pvt_thread_pool_keyfree_t **kfree )
{
	return(0);
}

int ldap_pvt_thread_pool_setkey (
	void *ctx, void *key,
	void *data, ldap_pvt_thread_pool_keyfree_t *kfree,
	void **olddatap, ldap_pvt_thread_pool_keyfree_t **oldkfreep )
{
	if ( olddatap ) *olddatap = NULL;
	if ( oldkfreep ) *oldkfreep = 0;
	return(0);
}

void ldap_pvt_thread_pool_purgekey( void *key )
{
}

int ldap_pvt_thread_pool_pause ( 
	ldap_pvt_thread_pool_t *tpool )
{
	return(0);
}

int ldap_pvt_thread_pool_resume ( 
	ldap_pvt_thread_pool_t *tpool )
{
	return(0);
}

int ldap_pvt_thread_pool_pausing( ldap_pvt_thread_pool_t *tpool )
{
	return(0);
}

ldap_pvt_thread_pool_pausecheck( ldap_pvt_thread_pool_t *tpool )
{
	return(0);
}
	
void *ldap_pvt_thread_pool_context( )
{
	return(NULL);
}

void ldap_pvt_thread_pool_context_reset( void *vctx )
{
}

ldap_pvt_thread_t
ldap_pvt_thread_self( void )
{
	return(0);
}

int
ldap_pvt_thread_key_create( ldap_pvt_thread_key_t *key )
{
	return(0);
}

int
ldap_pvt_thread_key_destroy( ldap_pvt_thread_key_t key )
{
	return(0);
}

int
ldap_pvt_thread_key_setdata( ldap_pvt_thread_key_t key, void *data )
{
	return(0);
}

int
ldap_pvt_thread_key_getdata( ldap_pvt_thread_key_t key, void **data )
{
	return(0);
}

ldap_pvt_thread_t
ldap_pvt_thread_pool_tid( void *vctx )
{

	return(0);
}

#endif /* NO_THREADS */
