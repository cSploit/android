/**********************************************************************
 * SQLBrowseConnect
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

SQLRETURN SQLBrowseConnect(
                             SQLHDBC        hDrvDbc,
                             SQLCHAR        *szConnStrIn,
                             SQLSMALLINT    cbConnStrIn,
                             SQLCHAR        *szConnStrOut,
                             SQLSMALLINT    cbConnStrOutMax,
                             SQLSMALLINT    *pcbConnStrOut
                                )
{
	HDRVDBC hDbc	= (HDRVDBC)hDrvDbc;

	if ( hDbc == NULL )
		return SQL_INVALID_HANDLE;
	
	sprintf( hDbc->szSqlMsg, "hDbc = $%08lX", hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );

	logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not currently supported" );

    return SQL_ERROR;
}


