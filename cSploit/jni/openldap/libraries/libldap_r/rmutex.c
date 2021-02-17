/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2006-2014 The OpenLDAP Foundation.
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
/* This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

/*
 * This is an implementation of recursive mutexes.
 */

#include "portable.h"

#include <ac/stdlib.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"
#include "ldap_pvt_thread.h" /* Get the thread interface */

struct ldap_int_thread_rmutex_s {
	ldap_pvt_thread_mutex_t ltrm_mutex;
	ldap_pvt_thread_cond_t ltrm_cond;
	ldap_pvt_thread_t ltrm_owner;
	int ltrm_valid;
#define LDAP_PVT_THREAD_RMUTEX_VALID	0x0cdb
	int ltrm_depth;
	int ltrm_waits;
};

static const ldap_pvt_thread_t tid_zero;

int 
ldap_pvt_thread_rmutex_init( ldap_pvt_thread_rmutex_t *rmutex )
{
	struct ldap_int_thread_rmutex_s *rm;

	assert( rmutex != NULL );

	rm = (struct ldap_int_thread_rmutex_s *) LDAP_CALLOC( 1,
		sizeof( struct ldap_int_thread_rmutex_s ) );
	if ( !rm )
		return LDAP_NO_MEMORY;

	/* we should check return results */
	ldap_pvt_thread_mutex_init( &rm->ltrm_mutex );
	ldap_pvt_thread_cond_init( &rm->ltrm_cond );

	rm->ltrm_valid = LDAP_PVT_THREAD_RMUTEX_VALID;

	*rmutex = rm;
	return 0;
}

int 
ldap_pvt_thread_rmutex_destroy( ldap_pvt_thread_rmutex_t *rmutex )
{
	struct ldap_int_thread_rmutex_s *rm;

	assert( rmutex != NULL );
	rm = *rmutex;

	assert( rm != NULL );
	assert( rm->ltrm_valid == LDAP_PVT_THREAD_RMUTEX_VALID );

	if( rm->ltrm_valid != LDAP_PVT_THREAD_RMUTEX_VALID )
		return LDAP_PVT_THREAD_EINVAL;

	ldap_pvt_thread_mutex_lock( &rm->ltrm_mutex );

	assert( rm->ltrm_depth >= 0 );
	assert( rm->ltrm_waits >= 0 );

	/* in use? */
	if( rm->ltrm_depth > 0 || rm->ltrm_waits > 0 ) {
		ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );
		return LDAP_PVT_THREAD_EBUSY;
	}

	rm->ltrm_valid = 0;

	ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );

	ldap_pvt_thread_mutex_destroy( &rm->ltrm_mutex );
	ldap_pvt_thread_cond_destroy( &rm->ltrm_cond );

	LDAP_FREE(rm);
	*rmutex = NULL;
	return 0;
}

int ldap_pvt_thread_rmutex_lock( ldap_pvt_thread_rmutex_t *rmutex,
	ldap_pvt_thread_t owner )
{
	struct ldap_int_thread_rmutex_s *rm;

	assert( rmutex != NULL );
	rm = *rmutex;

	assert( rm != NULL );
	assert( rm->ltrm_valid == LDAP_PVT_THREAD_RMUTEX_VALID );

	if( rm->ltrm_valid != LDAP_PVT_THREAD_RMUTEX_VALID )
		return LDAP_PVT_THREAD_EINVAL;

	ldap_pvt_thread_mutex_lock( &rm->ltrm_mutex );

	assert( rm->ltrm_depth >= 0 );
	assert( rm->ltrm_waits >= 0 );

	if( rm->ltrm_depth > 0 ) {
		/* already locked */
		if ( !ldap_pvt_thread_equal( rm->ltrm_owner, owner )) {
			rm->ltrm_waits++;
			do {
				ldap_pvt_thread_cond_wait( &rm->ltrm_cond,
					&rm->ltrm_mutex );
			} while( rm->ltrm_depth > 0 );

			rm->ltrm_waits--;
			assert( rm->ltrm_waits >= 0 );
			rm->ltrm_owner = owner;
		}
	} else {
		rm->ltrm_owner = owner;
	}

	rm->ltrm_depth++;

	ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );

	return 0;
}

int ldap_pvt_thread_rmutex_trylock( ldap_pvt_thread_rmutex_t *rmutex,
	ldap_pvt_thread_t owner )
{
	struct ldap_int_thread_rmutex_s *rm;

	assert( rmutex != NULL );
	rm = *rmutex;

	assert( rm != NULL );
	assert( rm->ltrm_valid == LDAP_PVT_THREAD_RMUTEX_VALID );

	if( rm->ltrm_valid != LDAP_PVT_THREAD_RMUTEX_VALID )
		return LDAP_PVT_THREAD_EINVAL;

	ldap_pvt_thread_mutex_lock( &rm->ltrm_mutex );

	assert( rm->ltrm_depth >= 0 );
	assert( rm->ltrm_waits >= 0 );

	if( rm->ltrm_depth > 0 ) {
		if ( !ldap_pvt_thread_equal( owner, rm->ltrm_owner )) {
			ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );
			return LDAP_PVT_THREAD_EBUSY;
		}
	} else {
		rm->ltrm_owner = owner;
	}

	rm->ltrm_depth++;

	ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );

	return 0;
}

int ldap_pvt_thread_rmutex_unlock( ldap_pvt_thread_rmutex_t *rmutex,
	ldap_pvt_thread_t owner )
{
	struct ldap_int_thread_rmutex_s *rm;

	assert( rmutex != NULL );
	rm = *rmutex;

	assert( rm != NULL );
	assert( rm->ltrm_valid == LDAP_PVT_THREAD_RMUTEX_VALID );

	if( rm->ltrm_valid != LDAP_PVT_THREAD_RMUTEX_VALID )
		return LDAP_PVT_THREAD_EINVAL;

	ldap_pvt_thread_mutex_lock( &rm->ltrm_mutex );

	if( !ldap_pvt_thread_equal( owner, rm->ltrm_owner )) {
		ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );
		return LDAP_PVT_THREAD_EINVAL;
	}

	rm->ltrm_depth--;
	if ( !rm->ltrm_depth )
		rm->ltrm_owner = tid_zero;

	assert( rm->ltrm_depth >= 0 );
	assert( rm->ltrm_waits >= 0 );

	if ( !rm->ltrm_depth && rm->ltrm_waits ) {
		ldap_pvt_thread_cond_signal( &rm->ltrm_cond );
	}

	ldap_pvt_thread_mutex_unlock( &rm->ltrm_mutex );

	return 0;
}

