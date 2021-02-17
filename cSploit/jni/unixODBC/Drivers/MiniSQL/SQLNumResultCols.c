/**********************************************************************
 * SQLNumResultCols
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

SQLRETURN SQLNumResultCols(   SQLHSTMT    hDrvStmt,
                                    SQLSMALLINT *pnColumnCount )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;
	
	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	if ( hStmt->hStmtExtras->nRows < 0 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No result set." );
		return SQL_ERROR;
	}

    /********************
     * get number of columns in result set
     ********************/
	*pnColumnCount = hStmt->hStmtExtras->nCols;

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
    return SQL_SUCCESS;
}



