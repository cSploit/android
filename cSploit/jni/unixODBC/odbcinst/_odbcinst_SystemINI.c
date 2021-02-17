/**************************************************
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 28.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
#include <config.h>
#include <odbcinstext.h>

/*
 * Add the historic ODBCINI value, mainly for applix.
 */

#ifdef VMS

char *odbcinst_system_file_path( char *buffer )
{
    char *path;

    if (( path = getvmsenv( "ODBCSYSINI" ))) {
		strcpy( buffer, path );
        return buffer;
	}
#ifdef SYSTEM_FILE_PATH
    else {
        return SYSTEM_FILE_PATH;
	}
#else
    else {
        return "ODBC_LIBDIR:";
	}
#endif
}

char *odbcinst_system_file_name( char *buffer )
{
    char *path;

    if (( path = getvmsenv( "ODBCINSTINI" ))) {
		strcpy( buffer, path );
        return path;
	}
	else {
        return "ODBCINST.INI";
	}
}

char *odbcinst_user_file_path( char *buffer )
{
	return "ODBC_LIBDIR:";
}

char *odbcinst_user_file_name( char *buffer )
{
	return "ODBCINST.INI";
}

BOOL _odbcinst_SystemINI( char *pszFileName, BOOL bVerify )
{
	FILE			*hFile;
	char			b1[ 256 ];

    sprintf( pszFileName, "%s:odbc.ini", odbcinst_system_file_path( b1 ));
	
	if ( bVerify )
	{
		hFile = uo_fopen( pszFileName, "r" );
		if ( hFile )
			uo_fclose( hFile );
		else
			return FALSE;
	}

	return TRUE;
}

#else

char *odbcinst_system_file_name( char *buffer )
{
    char *path;
    static char save_path[ 512 ];
    static int saved = 0;
	
    if ( saved ) {
	    return save_path;
    }

    if (( path = getenv( "ODBCINSTINI" ))) {
		strcpy( buffer, path );
	strcpy( save_path, buffer );
	saved = 1;
        return buffer;
	}
	else {
	strcpy( save_path, "odbcinst.ini" );
	saved = 1;
        return "odbcinst.ini";
	}
}

char *odbcinst_system_file_path( char *buffer )
{
    char *path;
    static char save_path[ 512 ];
    static int saved = 0;

    if ( saved ) {
	    return save_path;
    }

    if (( path = getenv( "ODBCSYSINI" ))) {
		strcpy( buffer, path );
	strcpy( save_path, buffer );
	saved = 1;
        return buffer;
	}
#ifdef SYSTEM_FILE_PATH
    else {
	strcpy( save_path, SYSTEM_FILE_PATH );
	saved = 1;
        return SYSTEM_FILE_PATH;
	}
#else
    else {
	strcpy( save_path, "/etc" );
	saved = 1;
        return "/etc";
	}
#endif
}

char *odbcinst_user_file_name( char *buffer )
{
	return ".odbcinst.ini";
}

char *odbcinst_user_file_path( char *buffer )
{
    char *path;
    static char save_path[ 512 ];
    static int saved = 0;

    if ( saved ) {
	    return save_path;
    }

    if (( path = getenv( "HOME" ))) {
		strcpy( buffer, path );
		strcpy( save_path, buffer );
		saved = 1;
        return buffer;
	}
	else {
        return "/home";
	}
}

BOOL _odbcinst_SystemINI( char *pszFileName, BOOL bVerify )
{
	FILE			*hFile;
	char			b1[ 256 ];

    sprintf( pszFileName, "%s/odbc.ini", odbcinst_system_file_path( b1 ));
	
	if ( bVerify )
	{
        /* try opening for read */
		hFile = uo_fopen( pszFileName, "r" );
		if ( hFile )
			uo_fclose( hFile );
		else
        {
            /* does not exist so try creating it */
            hFile = uo_fopen( pszFileName, "w" );
            if ( hFile )
                uo_fclose( hFile );
            else
                return FALSE;
        }
	}

	return TRUE;
}

#endif

