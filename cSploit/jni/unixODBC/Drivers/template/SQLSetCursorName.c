/**********************************************************************
 * SQLSetCursorName
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

SQLRETURN SQLSetCursorName(		SQLHSTMT      	hDrvStmt,
								SQLCHAR     	*szCursor,
								SQLSMALLINT 	nCursorLength )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

	if ( NULL == szCursor || 0 == isalpha(*szCursor) )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Invalid cursor name" );
		return SQL_ERROR;
	}

	/* COPY CURSOR */
	if ( SQL_NTS == nCursorLength )
	{
		strncpy((char*) hStmt->szCursorName,(char*) szCursor, SQL_MAX_CURSOR_NAME );
	}
	else
	{
/*
		strncpy( hStmt->szCursorName, szCursor, MIN(SQL_MAX_CURSOR_NAME - 1, nCursorLength) );
		hStmt->szCursorName[ MIN( SQL_MAX_CURSOR_NAME - 1, nCursorLength) ] = '\0';
*/		
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}


