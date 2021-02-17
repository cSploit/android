/**********************************************************************
 * SQLFetch
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

SQLRETURN SQLFetch( SQLHSTMT  hDrvStmt)
{
    HDRVSTMT 	hStmt		= (HDRVSTMT)hDrvStmt;
	int			nColumn		= -1;
	COLUMNHDR	*pColumnHeader;			

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

	if ( hStmt->hStmtExtras->nRows < 1 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No result set." );
		return SQL_ERROR;
	}

	/************************
	 * goto next row
	 ************************/
    if ( hStmt->hStmtExtras->nRow < 0 )
		return SQL_NO_DATA;

    if ( hStmt->hStmtExtras->nRow >= hStmt->hStmtExtras->nRows )
		return SQL_NO_DATA;

	hStmt->hStmtExtras->nRow++;

	/************************
	 * transfer bound column values to bound storage as required
	 ************************/
	for ( nColumn=1; nColumn <= hStmt->hStmtExtras->nCols; nColumn++ )
	{
		pColumnHeader = (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nColumn];
		if ( pColumnHeader->pTargetValue != NULL )
		{
			if ( _GetData(	hDrvStmt,
								nColumn,
								pColumnHeader->nTargetType,
								pColumnHeader->pTargetValue,
								pColumnHeader->nTargetValueMax,
								pColumnHeader->pnLengthOrIndicator ) != SQL_SUCCESS )
			{
				sprintf((char*) hStmt->szSqlMsg, "SQL_ERROR Failed to get data for column %d", nColumn );
				logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );
				return SQL_ERROR;
			}
		}
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}


