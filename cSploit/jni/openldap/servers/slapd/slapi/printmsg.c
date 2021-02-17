/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002-2003 IBM Corporation.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by IBM Corporation for use in
 * IBM products and subsequently ported to OpenLDAP Software by
 * Steve Omrani.
 */

#include <portable.h>
#include <stdio.h>
#include <ac/string.h>
#include <ac/stdarg.h>
#include <ac/unistd.h>
#include <fcntl.h>
#include <ac/errno.h>

#include <ldap.h>
#include <ldap_config.h>
#include <slap.h>
#include <slapi.h>

#include <ldap_pvt_thread.h>

/* Single threads access to routine */
ldap_pvt_thread_mutex_t slapi_printmessage_mutex; 
char			*slapi_log_file = NULL;
int			slapi_log_level = SLAPI_LOG_PLUGIN;

int 
slapi_int_log_error(
	int		level, 	
	char		*subsystem, 
	char		*fmt, 
	va_list		arglist ) 
{
	int		rc = 0;
	FILE		*fp = NULL;

	char		timeStr[100];
	struct tm	*ltm;
	time_t		currentTime;

	assert( subsystem != NULL );
	assert( fmt != NULL );

	ldap_pvt_thread_mutex_lock( &slapi_printmessage_mutex ) ;

	/* for now, we log all severities */
	if ( level <= slapi_log_level ) {
		fp = fopen( slapi_log_file, "a" );
		if ( fp == NULL) {
			rc = -1;
			goto done;
		}

		/*
		 * FIXME: could block
		 */
		while ( lockf( fileno( fp ), F_LOCK, 0 ) != 0 ) {
			/* DO NOTHING */ ;
		}

		time( &currentTime );
		ltm = localtime( &currentTime );
		strftime( timeStr, sizeof(timeStr), "%x %X", ltm );
		fputs( timeStr, fp );

		fprintf( fp, " %s: ", subsystem );
		vfprintf( fp, fmt, arglist );
		if ( fmt[ strlen( fmt ) - 1 ] != '\n' ) {
			fputs( "\n", fp );
		}
		fflush( fp );

		lockf( fileno( fp ), F_ULOCK, 0 );

		fclose( fp );

	} else {
		rc = -1;
	}

done:
	ldap_pvt_thread_mutex_unlock( &slapi_printmessage_mutex );

	return rc;
}
