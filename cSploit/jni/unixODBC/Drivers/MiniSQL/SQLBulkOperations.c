/**********************************************************************
 * SQLBulkOperations
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

SQLRETURN SQLBulkOperations(	SQLHSTMT        hDrvStmt,
								SQLSMALLINT    	nOperation )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

    /* OK */
    switch ( nOperation )
    {
    case SQL_ADD:
        break;
    case SQL_UPDATE_BY_BOOKMARK:
        break;
    case SQL_DELETE_BY_BOOKMARK:
        break;
    case SQL_FETCH_BY_BOOKMARK:
        break;
	default:
		sprintf( hStmt->szSqlMsg, "SQL_ERROR Unknown nOperation=%d", nOperation );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
		return SQL_ERROR;
    }

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not currently supported" );

    return SQL_ERROR;
}


