/**********************************************************************
 * SQLDisconnect
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

SQLRETURN SQLDisconnect( SQLHDBC    hDrvDbc )
{
	HDRVDBC hDbc	= (HDRVDBC)hDrvDbc;

	/* SANITY CHECKS */
    if( NULL == hDbc )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hDbc->szSqlMsg, "hDbc = $%08lX", (long)hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, (char*)hDbc->szSqlMsg );

    if( hDbc->bConnected == 0 )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_SUCCESS_WITH_INFO Connection not open" );
        return SQL_SUCCESS_WITH_INFO;
    }

	if ( hDbc->hFirstStmt != SQL_NULL_HSTMT )
	{
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Active Statements exist. Can not disconnect." );
		return SQL_ERROR;
	}

    /****************************
     * 1. do driver specific close here
     ****************************/
    hDbc->bConnected 		= 0;

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}


