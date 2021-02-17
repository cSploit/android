/**********************************************************************
 * SQLDriverConnect
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 **********************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN SQLDriverConnect(		SQLHDBC            hDrvDbc,
								SQLHWND            hWnd,
								SQLCHAR            *szConnStrIn,
								SQLSMALLINT        nConnStrIn,
								SQLCHAR            *szConnStrOut,
								SQLSMALLINT        cbConnStrOutMax,
								SQLSMALLINT        *pnConnStrOut,
								SQLUSMALLINT       nDriverCompletion
                                )
{
	HDRVDBC	hDbc	= (HDRVDBC)hDrvDbc;
    char    	szDSN[INI_MAX_PROPERTY_VALUE+1]			= "";
    char    	szDRIVER[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szUID[INI_MAX_PROPERTY_VALUE+1]			= "";
    char    	szPWD[INI_MAX_PROPERTY_VALUE+1]			= "";
    char    	szDATABASE[INI_MAX_PROPERTY_VALUE+1]	= "";
    char    	szHOST[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szPORT[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szSOCKET[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szFLAG[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szNameValue[INI_MAX_PROPERTY_VALUE+1]	= "";
    char    	szName[INI_MAX_PROPERTY_VALUE+1]		= "";
    char    	szValue[INI_MAX_PROPERTY_VALUE+1]		= "";
	int			nOption									= 0;

	/* SANITY CHECKS */
    if( NULL == hDbc )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hDbc->szSqlMsg, "hDbc = $%08lX", (long)hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );


    if( hDbc->bConnected == 1 )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Already connected" );
        return SQL_ERROR;
    }

    if( !szConnStrIn )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Bad argument" );
        return SQL_ERROR;
    }

    switch( nDriverCompletion )
    {
        case SQL_DRIVER_PROMPT:
        case SQL_DRIVER_COMPLETE:
        case SQL_DRIVER_COMPLETE_REQUIRED:
        case SQL_DRIVER_NOPROMPT:
        default:
			sprintf((char*) hDbc->szSqlMsg, "Invalid nDriverCompletion=%d", nDriverCompletion );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );
            break;
    }

    /*************************
     * 1. parse nConnStrIn for connection options. Format is;
     *      Property=Value;...
     * 2. we may not have all options so handle as per DM request
     * 3. fill as required szConnStrOut
     *************************/
	for ( nOption = 1; iniElement((char*) szConnStrIn, ';', '\0', nOption, szNameValue, sizeof(szNameValue) ) == INI_SUCCESS ; nOption++ )
	{
		szName[0]	= '\0';
		szValue[0]	= '\0';
		iniElement( szNameValue, '=', '\0', 0, szName, sizeof(szName) );
		iniElement( szNameValue, '=', '\0', 1, szValue, sizeof(szValue) );
		if ( strcasecmp( szName, "DSN" ) == 0 )
			strcpy( szDSN, szValue );
		else if ( strcasecmp( szName, "DRIVER" ) == 0 )
			strcpy( szDRIVER, szValue );
		else if ( strcasecmp( szName, "UID" ) == 0 )
			strcpy( szUID, szValue );
		else if ( strcasecmp( szName, "PWD" ) == 0 )
			strcpy( szPWD, szValue );
		else if ( strcasecmp( szName, "SERVER" ) == 0 )
			strcpy( szHOST, szValue );
		else if ( strcasecmp( szName, "DB" ) == 0 )
			strcpy( szDATABASE, szValue );
		else if ( strcasecmp( szName, "SOCKET" ) == 0 )
			strcpy( szSOCKET, szValue );
		else if ( strcasecmp( szName, "PORT" ) == 0 )
			strcpy( szPORT, szValue );
		else if ( strcasecmp( szName, "OPTION" ) == 0 )
			strcpy( szFLAG, szValue );
	}

	/****************************
	 * return the connect string we are using
	 ***************************/
    if( !szConnStrOut )
    {
    }


    /*************************
     * 4. try to connect
     * 5. set gathered options (ie USE Database or whatever)
     * 6. set connection state
     *      hDbc->bConnected = TRUE;
     *************************/
	hDbc->bConnected 			= 1;

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported." );

    return SQL_SUCCESS;
}


