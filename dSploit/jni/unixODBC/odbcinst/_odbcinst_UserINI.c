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
#ifdef HAVE_PWD_H
    #include <pwd.h>
#endif

#include <odbcinstext.h>

#ifdef VMS

BOOL _odbcinst_UserINI( char *pszFileName, BOOL bVerify )
{
    FILE            *hFile;
    char            *szEnv_INIUSER              = getvmsenv("ODBCINI");
    struct passwd   *pPasswd                    = NULL;
    char            *pHomeDir                   = NULL;

    pszFileName[0] = '\0';

    if ( szEnv_INIUSER )
    {
        strncpy( pszFileName, szEnv_INIUSER, ODBC_FILENAME_MAX );
    }
    else
    {
        sprintf( pszFileName, "SYS$LOGIN:ODBC.INI" );
    }

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

BOOL _odbcinst_UserINI( char *pszFileName, BOOL bVerify )
{
    FILE            *hFile;
    char            *szEnv_INIUSER              = getenv("ODBCINI");
#ifdef HAVE_GETUID
    uid_t           nUserID                     = getuid();
#else
    uid_t           nUserID                     = 0;
#endif
    struct passwd   *pPasswd                    = NULL;
    char            *pHomeDir                   = NULL;

    pHomeDir    = "/home";                              
#ifdef HAVE_GETPWUID
    pPasswd     = (struct passwd *)getpwuid(nUserID);   
#endif

    pszFileName[0] = '\0';

#ifdef HAVE_PWD_H
    if ( pPasswd != NULL )
        if ( ( char *)pPasswd->pw_dir != NULL )
            pHomeDir    = pPasswd->pw_dir;
#else
    pHomeDir = getenv("HOME");
#endif

    if ( szEnv_INIUSER )
    {
        strncpy( pszFileName, szEnv_INIUSER, ODBC_FILENAME_MAX );
    }
    if ( pszFileName[0] == '\0' )
    {
        sprintf( pszFileName, "%s%s", pHomeDir, "/.odbc.ini" );
    }

#ifdef DHAVE_ENDPWENT
    /*
     * close the password file
     */
    endpwent();
#endif

    if ( bVerify )
    {
        /*
         * create it of it doesn't exist
         */

        hFile = uo_fopen( pszFileName, "a" );
        if ( hFile )
            uo_fclose( hFile );
        else
            return FALSE;
    }

    return TRUE;
}

#endif

BOOL _odbcinst_FileINI( char *pszPath )
{
	char b1[ 256 ];

    /* we need a viable buffer (with space for FILENAME_MAX chars)... */
    if ( !pszPath )
        return FALSE;

    /* system configured to use a special location... */
    *pszPath = '\0';
    SQLGetPrivateProfileString( "ODBC", "FileDSNPath", "", pszPath, FILENAME_MAX - 2, "odbcinst.ini" );
    if ( *pszPath )
        return TRUE;

    /* default location... */
    sprintf( pszPath, "%s/ODBCDataSources", odbcinst_system_file_path( b1 ));

    return TRUE;
}


