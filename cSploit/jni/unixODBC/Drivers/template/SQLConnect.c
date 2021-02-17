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
    char    	szHOST[INI_MAX_PROPERTY_VALUE+1];
    char    	szPORT[INI_MAX_PROPERTY_VALUE+1];
    char    	szFLAG[INI_MAX_PROPERTY_VALUE+1];

    /* SANITY CHECKS */
    if( SQL_NULL_HDBC == hDbc )
		return SQL_INVALID_HANDLE;

	sprintf((char*) hDbc->szSqlMsg, "hDbc=$%08lX 3zDataSource=(%s)", (long)hDbc, szDataSource );
	logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );

    if( hDbc->bConnected == 1 )
    {
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Already connected" );
        return SQL_ERROR;
    }

    if ( nDataSourceLength == SQL_NTS )
    {
        if ( strlen((char*) szDataSource ) > ODBC_FILENAME_MAX+INI_MAX_OBJECT_NAME )
        {
            logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Given Data Source is too long. I consider it suspect." );
            return SQL_ERROR;
        }
    }
    else
    {
        if ( nDataSourceLength > ODBC_FILENAME_MAX+INI_MAX_OBJECT_NAME )
        {
            logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Given Data Source is too long. I consider it suspect." );
            return SQL_ERROR;
        }
    }

    /********************
     * gather and use any required DSN properties
	 * - DATABASE
	 * - HOST (localhost assumed if not supplied)
     ********************/
    szDATABASE[0] 		= '\0';
    szHOST[0] 			= '\0';
    szPORT[0] 			= '\0';
    szFLAG[0] 			= '\0';
	SQLGetPrivateProfileString((char*) szDataSource, "DATABASE", "", szDATABASE, sizeof(szDATABASE), "odbc.ini" );
	if ( szDATABASE[0] == '\0' )
	{
		sprintf((char*) hDbc->szSqlMsg, "SQL_ERROR Could not find Driver entry for %s in system information", szDataSource );
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );
		return SQL_ERROR;
	}
	SQLGetPrivateProfileString((char*) szDataSource, "HOST", "localhost", szHOST, sizeof(szHOST), "odbc.ini" );
	SQLGetPrivateProfileString((char*) szDataSource, "PORT", "0", szPORT, sizeof(szPORT), "odbc.ini" );
	SQLGetPrivateProfileString((char*) szDataSource, "FLAG", "0", szFLAG, sizeof(szFLAG), "odbc.ini" );
	
    /********************
     * 1. initialise structures
     * 2. try connection with database using your native calls
     * 3. store your server handle in the extras somewhere
     * 4. set connection state
     *      hDbc->bConnected = TRUE;
     ********************/

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}


