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

#include "portable.h"
#include <stdio.h>
#include <ac/string.h>
#include "slap.h"
#include "lutil.h"

#ifdef HAVE_NT_SERVICE_MANAGER

/* in main.c */
void WINAPI ServiceMain( DWORD argc, LPTSTR *argv );

/* in ntservice.c */
int main( int argc, LPTSTR *argv )
{
	int		length;
	char	filename[MAX_PATH], *fname_start;

	/*
	 * Because the service was registered as SERVICE_WIN32_OWN_PROCESS,
	 * the lpServiceName element of the SERVICE_TABLE_ENTRY will be
	 * ignored.
	 */

	SERVICE_TABLE_ENTRY		DispatchTable[] = {
		{	"",	(LPSERVICE_MAIN_FUNCTION) ServiceMain	},
		{	NULL,			NULL	}
	};

	/*
	 * set the service's current directory to the installation directory
	 * for the service. this way we don't have to write absolute paths
	 * in the configuration files
	 */
	GetModuleFileName( NULL, filename, sizeof( filename ) );
	fname_start = strrchr( filename, *LDAP_DIRSEP );

	if ( argc > 1 ) {
		if ( _stricmp( "install", argv[1] ) == 0 ) 
		{
			char *svcName = SERVICE_NAME;
			char *displayName = "OpenLDAP Directory Service";
			BOOL auto_start = FALSE;

			if ( (argc > 2) && (argv[2] != NULL) )
				svcName = argv[2];

			if ( argc > 3 && argv[3])
				displayName = argv[3];

			if ( argc > 4 && stricmp(argv[4], "auto") == 0)
				auto_start = TRUE;

			strcat(filename, " service");
			if ( !lutil_srv_install(svcName, displayName, filename, auto_start) ) 
			{
				fputs( "service failed installation ...\n", stderr  );
				return EXIT_FAILURE;
			}
			fputs( "service has been installed ...\n", stderr  );
			return EXIT_SUCCESS;
		}

		if ( _stricmp( "remove", argv[1] ) == 0 ) 
		{
			char *svcName = SERVICE_NAME;
			if ( (argc > 2) && (argv[2] != NULL) )
				svcName = argv[2];
			if ( !lutil_srv_remove(svcName, filename) ) 
			{
				fputs( "failed to remove the service ...\n", stderr  );
				return EXIT_FAILURE;
			}
			fputs( "service has been removed ...\n", stderr );
			return EXIT_SUCCESS;
		}
		if ( _stricmp( "service", argv[1] ) == 0 )
		{
			is_NT_Service = 1;
			*fname_start = '\0';
			SetCurrentDirectory( filename );
		}
	}

	if (is_NT_Service)
	{
		StartServiceCtrlDispatcher(DispatchTable);
	} else
	{
		ServiceMain( argc, argv );
	}

	return EXIT_SUCCESS;
}

#endif
