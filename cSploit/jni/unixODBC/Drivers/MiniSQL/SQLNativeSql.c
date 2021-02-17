/**********************************************************************
 * SQLNativeSql
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

SQLRETURN SQLNativeSql(   SQLHSTMT    hDrvStmt,
                                SQLCHAR     *szSqlStrIn,
                                SQLINTEGER  cbSqlStrIn,
                                SQLCHAR     *szSqlStr,
                                SQLINTEGER  cbSqlStrMax,
                                SQLINTEGER  *pcbSqlStr )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );


    return SQL_ERROR;
}

