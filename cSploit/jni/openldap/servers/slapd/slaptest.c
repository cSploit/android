/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ac/unistd.h>
#include <ac/errno.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>

#include "slapcommon.h"

#ifndef S_IWRITE
#define S_IWRITE	S_IWUSR
#endif

static int
test_file( const char *fname, const char *ftype )
{
	struct stat	st;
	int		save_errno;

	switch ( stat( fname, &st ) ) {
	case 0:
		if ( !( st.st_mode & S_IWRITE ) ) {
			Debug( LDAP_DEBUG_ANY, "%s file "
				"\"%s\" exists, but user does not have access\n",
				ftype, fname, 0 );
			return -1;
		}
		break;

	case -1:
	default:
		save_errno = errno;
		if ( save_errno == ENOENT ) {
			FILE		*fp = fopen( fname, "w" );

			if ( fp == NULL ) {
				save_errno = errno;

				Debug( LDAP_DEBUG_ANY, "unable to open file "
					"\"%s\": %d (%s)\n",
					fname,
					save_errno, strerror( save_errno ) );

				return -1;
			}
			fclose( fp );
			unlink( fname );
			break;
		}

		Debug( LDAP_DEBUG_ANY, "unable to stat file "
			"\"%s\": %d (%s)\n",
			slapd_pid_file,
			save_errno, strerror( save_errno ) );
		return -1;
	}

	return 0;
}

int
slaptest( int argc, char **argv )
{
	int			rc = EXIT_SUCCESS;
	const char		*progname = "slaptest";

	slap_tool_init( progname, SLAPTEST, argc, argv );

	if ( slapd_pid_file != NULL ) {
		if ( test_file( slapd_pid_file, "pid" ) ) {
			return EXIT_FAILURE;
		}
	}

	if ( slapd_args_file != NULL ) {
		if ( test_file( slapd_args_file, "args" ) ) {
			return EXIT_FAILURE;
		}
	}

	if ( !quiet ) {
		fprintf( stderr, "config file testing succeeded\n");
	}

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;

	return rc;
}
