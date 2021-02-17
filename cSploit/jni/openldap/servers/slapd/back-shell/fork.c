/* fork.c - fork and exec a process, connecting stdin/out w/pipes */
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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slap.h"
#include "shell.h"

pid_t
forkandexec(
    char	**args,
    FILE	**rfp,
    FILE	**wfp
)
{
	int	p2c[2] = { -1, -1 }, c2p[2];
	pid_t	pid;

	if ( pipe( p2c ) != 0 || pipe( c2p ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "pipe failed\n", 0, 0, 0 );
		close( p2c[0] );
		close( p2c[1] );
		return( -1 );
	}

	/*
	 * what we're trying to set up looks like this:
	 *	parent *wfp -> p2c[1] | p2c[0] -> stdin child
	 *	parent *rfp <- c2p[0] | c2p[1] <- stdout child
	 */

	fflush( NULL );
# ifdef HAVE_THR
	pid = fork1();
# else
	pid = fork();
# endif
	if ( pid == 0 ) {		/* child */
		/*
		 * child could deadlock here due to resources locked
		 * by our parent
		 *
		 * If so, configure --without-threads.
		 */
		if ( dup2( p2c[0], 0 ) == -1 || dup2( c2p[1], 1 ) == -1 ) {
			Debug( LDAP_DEBUG_ANY, "dup2 failed\n", 0, 0, 0 );
			exit( EXIT_FAILURE );
		}
	}
	close( p2c[0] );
	close( c2p[1] );
	if ( pid <= 0 ) {
		close( p2c[1] );
		close( c2p[0] );
	}
	switch ( pid ) {
	case 0:
		execv( args[0], args );

		Debug( LDAP_DEBUG_ANY, "execv failed\n", 0, 0, 0 );
		exit( EXIT_FAILURE );

	case -1:	/* trouble */
		Debug( LDAP_DEBUG_ANY, "fork failed\n", 0, 0, 0 );
		return( -1 );
	}

	/* parent */
	if ( (*rfp = fdopen( c2p[0], "r" )) == NULL || (*wfp = fdopen( p2c[1],
	    "w" )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "fdopen failed\n", 0, 0, 0 );
		if ( *rfp ) {
			fclose( *rfp );
			*rfp = NULL;
		} else {
			close( c2p[0] );
		}
		close( p2c[1] );

		return( -1 );
	}

	return( pid );
}
