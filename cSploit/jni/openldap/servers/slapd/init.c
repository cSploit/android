/* init.c - initialize various things */
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
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lber_pvt.h"

#include "ldap_rq.h"

/*
 * read-only global variables or variables only written by the listener
 * thread (after they are initialized) - no need to protect them with a mutex.
 */
int		slap_debug = 0;

#ifdef LDAP_DEBUG
int		ldap_syslog = LDAP_DEBUG_STATS;
#else
int		ldap_syslog;
#endif

#ifdef LOG_DEBUG
int		ldap_syslog_level = LOG_DEBUG;
#endif

BerVarray default_referral = NULL;

/*
 * global variables that need mutex protection
 */
ldap_pvt_thread_pool_t	connection_pool;
int		connection_pool_max = SLAP_MAX_WORKER_THREADS;
int		connection_pool_queues = 1;
int		slap_tool_thread_max = 1;

slap_counters_t			slap_counters, *slap_counters_list;

static const char* slap_name = NULL;
int slapMode = SLAP_UNDEFINED_MODE;

int
slap_init( int mode, const char *name )
{
	int rc;

	assert( mode );

	if ( slapMode != SLAP_UNDEFINED_MODE ) {
		/* Make sure we write something to stderr */
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		 "%s init: init called twice (old=%d, new=%d)\n",
		 name, slapMode, mode );

		return 1;
	}

	slapMode = mode;

	slap_op_init();

#ifdef SLAPD_MODULES
	if ( module_init() != 0 ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: module_init failed\n",
			name, 0, 0 );
		return 1;
	}
#endif

	if ( slap_schema_init( ) != 0 ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: slap_schema_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( filter_init() != 0 ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: filter_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( entry_init() != 0 ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: entry_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	switch ( slapMode & SLAP_MODE ) {
	case SLAP_SERVER_MODE:
		root_dse_init();

		/* FALLTHRU */
	case SLAP_TOOL_MODE:
		Debug( LDAP_DEBUG_TRACE,
			"%s init: initiated %s.\n",	name,
			(mode & SLAP_MODE) == SLAP_TOOL_MODE ? "tool" : "server",
			0 );

		slap_name = name;

		ldap_pvt_thread_pool_init_q( &connection_pool,
				connection_pool_max, 0, connection_pool_queues);

		slap_counters_init( &slap_counters );

		ldap_pvt_thread_mutex_init( &slapd_rq.rq_mutex );
		LDAP_STAILQ_INIT( &slapd_rq.task_list );
		LDAP_STAILQ_INIT( &slapd_rq.run_list );

		slap_passwd_init();

		rc = slap_sasl_init();

		if( rc == 0 ) {
			rc = backend_init( );
		}
		if ( rc )
			return rc;

		break;

	default:
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
			"%s init: undefined mode (%d).\n", name, mode, 0 );

		rc = 1;
		break;
	}

	if ( slap_controls_init( ) != 0 ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: slap_controls_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( frontend_init() ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: frontend_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( overlay_init() ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: overlay_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	if ( glue_sub_init() ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: glue/subordinate init failed\n",
		    name, 0, 0 );

		return 1;
	}

	if ( acl_init() ) {
		slap_debug |= LDAP_DEBUG_NONE;
		Debug( LDAP_DEBUG_ANY,
		    "%s: acl_init failed\n",
		    name, 0, 0 );
		return 1;
	}

	return rc;
}

int slap_startup( Backend *be )
{
	int rc;
	Debug( LDAP_DEBUG_TRACE,
		"%s startup: initiated.\n",
		slap_name, 0, 0 );

	rc = backend_startup( be );
	if ( !rc && ( slapMode & SLAP_SERVER_MODE ))
		slapMode |= SLAP_SERVER_RUNNING;
	return rc;
}

int slap_shutdown( Backend *be )
{
	Debug( LDAP_DEBUG_TRACE,
		"%s shutdown: initiated\n",
		slap_name, 0, 0 );

	/* let backends do whatever cleanup they need to do */
	return backend_shutdown( be ); 
}

int slap_destroy(void)
{
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		"%s destroy: freeing system resources.\n",
		slap_name, 0, 0 );

	if ( default_referral ) {
		ber_bvarray_free( default_referral );
	}

	/* clear out any thread-keys for the main thread */
	ldap_pvt_thread_pool_context_reset( ldap_pvt_thread_pool_context());

	rc = backend_destroy();

	slap_sasl_destroy();

	/* rootdse destroy goes before entry_destroy()
	 * because it may use entry_free() */
	root_dse_destroy();
	entry_destroy();

	switch ( slapMode & SLAP_MODE ) {
	case SLAP_SERVER_MODE:
	case SLAP_TOOL_MODE:
		slap_counters_destroy( &slap_counters );
		break;

	default:
		Debug( LDAP_DEBUG_ANY,
			"slap_destroy(): undefined mode (%d).\n", slapMode, 0, 0 );

		rc = 1;
		break;

	}

	slap_op_destroy();

	ldap_pvt_thread_destroy();

	/* should destroy the above mutex */
	return rc;
}

void slap_counters_init( slap_counters_t *sc )
{
	int i;

	ldap_pvt_thread_mutex_init( &sc->sc_mutex );
	ldap_pvt_mp_init( sc->sc_bytes );
	ldap_pvt_mp_init( sc->sc_pdu );
	ldap_pvt_mp_init( sc->sc_entries );
	ldap_pvt_mp_init( sc->sc_refs );

	ldap_pvt_mp_init( sc->sc_ops_initiated );
	ldap_pvt_mp_init( sc->sc_ops_completed );

#ifdef SLAPD_MONITOR
	for ( i = 0; i < SLAP_OP_LAST; i++ ) {
		ldap_pvt_mp_init( sc->sc_ops_initiated_[ i ] );
		ldap_pvt_mp_init( sc->sc_ops_completed_[ i ] );
	}
#endif /* SLAPD_MONITOR */
}

void slap_counters_destroy( slap_counters_t *sc )
{
	int i;

	ldap_pvt_thread_mutex_destroy( &sc->sc_mutex );
	ldap_pvt_mp_clear( sc->sc_bytes );
	ldap_pvt_mp_clear( sc->sc_pdu );
	ldap_pvt_mp_clear( sc->sc_entries );
	ldap_pvt_mp_clear( sc->sc_refs );

	ldap_pvt_mp_clear( sc->sc_ops_initiated );
	ldap_pvt_mp_clear( sc->sc_ops_completed );

#ifdef SLAPD_MONITOR
	for ( i = 0; i < SLAP_OP_LAST; i++ ) {
		ldap_pvt_mp_clear( sc->sc_ops_initiated_[ i ] );
		ldap_pvt_mp_clear( sc->sc_ops_completed_[ i ] );
	}
#endif /* SLAPD_MONITOR */
}

