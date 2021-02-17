/**********************************************************************
 * SQLConnect
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

SQLRETURN SQLConnect(	SQLHDBC        hDrvDbc,
						SQLCHAR        *szDataSource,
						SQLSMALLINT    nDataSourceLength,
						SQLCHAR        *szUID,
						SQLSMALLINT    nUIDLength,
						SQLCHAR        *szPWD,
						SQLSMALLINT    nPWDLength
                          )
{
	HDRVDBC 	hDbc	= (HDRVDBC)hDrvDbc;
    char    	szDATABASE[INI_MAX_PROPERTY_VALUE+1];
    char    	szCONFIGFILE[INI_MAX_PROPERTY_VALUE+1];
    char    	szHOST[INI_MAX_PROPERTY_VALUE+1];

    /* SANITY CHECKS */
    if( SQL_NULL_HDBC == hDbc )
		return SQL_INVALID_HANDLE;

	sprintf( hDbc->szSqlMsg, "hDbc=$%08lX szDataSource=(%s)", hDbc, szDataSource );
	logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );

    if( hDbc->bConnected == 1 )
    {
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Already connected" );
        return SQL_ERROR;
    }

    if ( strlen( szDataSource ) > ODBC_FILENAME_MAX+INI_MAX_OBJECT_NAME )
    {
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Given Data Source is too long. I consider it suspect." );
        return SQL_ERROR;
    }

    /********************
     * gather and use any required DSN properties
	 * - CONFIGFILE - for msqlLoadConfigFile() (has precedence over other properties)
	 * - DATABASE
	 * - HOST (localhost assumed if not supplied)
     ********************/
    szDATABASE[0] 		= '\0';
    szHOST[0] 			= '\0';
    szCONFIGFILE[0] 	= '\0';
	SQLGetPrivateProfileString( szDataSource, "DATABASE", "", szDATABASE, sizeof(szDATABASE), ".odbc.ini" );
	if ( szDATABASE[0] == '\0' )
	{
		sprintf( hDbc->szSqlMsg, "SQL_ERROR Could not find Driver entry for %s in system information", szDataSource );
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
		return SQL_ERROR;
	}
	SQLGetPrivateProfileString( szDataSource, "HOST", "", szHOST, sizeof(szHOST), ".odbc.ini" );
	SQLGetPrivateProfileString( szDataSource, "CONFIGFILE", "", szCONFIGFILE, sizeof(szCONFIGFILE), ".odbc.ini" );

    /********************
     * 1. initialise structures
     * 2. try connection with database using your native calls
     * 3. store your server handle in the extras somewhere
     * 4. set connection state
     *      hDbc->bConnected = TRUE;
     ********************/
	if ( szCONFIGFILE[0] != '\0' )
	{
		if ( msqlLoadConfigFile( szCONFIGFILE ) != 0 )
		{
			sprintf( hDbc->szSqlMsg, "SQL_ERROR %s", msqlErrMsg );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
			sprintf( hDbc->szSqlMsg, "SQL_WARNING Failed to use (%s)", szCONFIGFILE );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
			return SQL_ERROR;
		}
	}

	if ( szHOST[0] == '\0' )
	{
		hDbc->hDbcExtras->hServer = msqlConnect( NULL );    /* this is faster than using localhost */
		strcpy( szHOST, "localhost" );
	}
	else
		hDbc->hDbcExtras->hServer = msqlConnect( szHOST );

	if ( hDbc->hDbcExtras->hServer < 0 )
	{
		sprintf( hDbc->szSqlMsg, "SQL_ERROR %s", msqlErrMsg );
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
		sprintf( hDbc->szSqlMsg, "SQL_ERROR Failed to connect to (%s)", szHOST );
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
		return SQL_ERROR;
	}

	hDbc->bConnected = 1;

	/* SET DATABASE */
	if ( szDATABASE[0] != '\0' )
	{
		if ( msqlSelectDB( hDbc->hDbcExtras->hServer, szDATABASE ) == -1 )
		{
			sprintf( hDbc->szSqlMsg, "SQL_WARNING %s", msqlErrMsg );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
			sprintf( hDbc->szSqlMsg, "SQL_WARNING Connected to server but failed to use database (%s)", szDATABASE );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
		}
		else
		{
			sprintf( hDbc->szSqlMsg, "SQL_INFO DATABASE=%s", szDATABASE );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
		}

	}

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}


