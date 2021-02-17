/**********************************************************************
 * SQLGetCursorName
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

SQLRETURN SQLGetCursorName(   SQLHSTMT    hDrvStmt,
							  SQLCHAR     *szCursor,
							  SQLSMALLINT nCursorMaxLength,
							  SQLSMALLINT *pnCursorLength )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	int     ci;								 /* counter variable         */

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );
	
	if ( NULL == szCursor )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No cursor name." );
		return SQL_ERROR;
	}

	/*
	** copy cursor name
	*/
	strncpy((char*) szCursor, (char*)hStmt->szCursorName, nCursorMaxLength );

	/*
	** set length of transfered data
	*/
	ci = strlen((char*) hStmt->szCursorName );
/*
	if ( NULL != pnCursorLength )
		*pnCursorLength = MIN( ci, nCursorMaxLength );
*/
	if ( nCursorMaxLength < ci )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_SUCCESS_WITH_INFO Cursor was truncated" );
		return SQL_SUCCESS_WITH_INFO;
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}


