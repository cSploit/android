/**********************************************************************
 * SQLColAttributes (this function has been deprecated)
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

SQLRETURN SQLColAttributes(		SQLHSTMT    hDrvStmt,
								UWORD       nCol,
								UWORD       nDescType,
								PTR    		pszDesc,
								SWORD       nDescMax,
								SWORD   	*pcbDesc,
								SDWORD   	*pfDesc )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

    /**************************
     * 1. verify we have result set
     * 2. verify col is within range
     **************************/


    /**************************
     * 3. process request
     **************************/
    switch( nDescType )
    {
/* enum these
    case SQL_COLUMN_AUTO_INCREMENT:
    case SQL_COLUMN_CASE_SENSITIVE:
    case SQL_COLUMN_COUNT:
    case SQL_COLUMN_DISPLAY_SIZE:
    case SQL_COLUMN_LENGTH:
    case SQL_COLUMN_MONEY:
    case SQL_COLUMN_NULLABLE:
    case SQL_COLUMN_PRECISION:
    case SQL_COLUMN_SCALE:
    case SQL_COLUMN_SEARCHABLE:
    case SQL_COLUMN_TYPE:
    case SQL_COLUMN_UNSIGNED:
    case SQL_COLUMN_UPDATABLE:
    case SQL_COLUMN_CATALOG_NAME:
    case SQL_COLUMN_QUALIFIER_NAME:
    case SQL_COLUMN_DISTINCT_TYPE:
    case SQL_COLUMN_LABEL:
    case SQL_COLUMN_NAME:
    case SQL_COLUMN_SCHEMA_NAME:
    case SQL_COLUMN_OWNER_NAME:
    case SQL_COLUMN_TABLE_NAME:
    case SQL_COLUMN_TYPE_NAME:
*/	
    default:
		sprintf( hStmt->szSqlMsg, "SQL_ERROR nDescType=%d", nDescType );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
        return SQL_ERROR;
    }

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );

    return SQL_ERROR;
}



