/*****************************************************************************
 * SQLFetchScroll
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 *****************************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN SQLFetchScroll( SQLHSTMT    hDrvStmt,
                                SQLSMALLINT nOrientation,
                                SQLINTEGER  nOffset )
{
    HDRVSTMT 	hStmt		= (HDRVSTMT)hDrvStmt;
	int			nColumn		= -1;
	COLUMNHDR	*pColumnHeader;			

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	/* how much do we want to move */
	switch ( nOrientation )
	{
	case SQL_FETCH_NEXT:
		if ( hStmt->hStmtExtras->nRow >= hStmt->hStmtExtras->nRows )
			return SQL_NO_DATA;
		hStmt->hStmtExtras->nRow++;
		break;

	case SQL_FETCH_PRIOR:
		if ( hStmt->hStmtExtras->nRow == 0 )
			return SQL_NO_DATA;
		hStmt->hStmtExtras->nRow--;
		break;

	case SQL_FETCH_FIRST:
		hStmt->hStmtExtras->nRow = 0;
		break;

	case SQL_FETCH_LAST:
		hStmt->hStmtExtras->nRow = hStmt->hStmtExtras->nRows-1;
		break;

	case SQL_FETCH_ABSOLUTE:
		if ( nOffset < 0 || nOffset >= hStmt->hStmtExtras->nRows )
			return SQL_NO_DATA;
		hStmt->hStmtExtras->nRow = nOffset;
		break;

	case SQL_FETCH_RELATIVE:
		if ( hStmt->hStmtExtras->nRow+nOffset < 0 || hStmt->hStmtExtras->nRow+nOffset >= hStmt->hStmtExtras->nRows )
			return SQL_NO_DATA;
		hStmt->hStmtExtras->nRow += nOffset;
		break;

	case SQL_FETCH_BOOKMARK:
		return SQL_ERROR;
		break;

	default:
		return SQL_ERROR;
	}

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
				sprintf( hStmt->szSqlMsg, "SQL_ERROR Failed to get data for column %d", nColumn );
				logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
				return SQL_ERROR;
			}
		}
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
    return SQL_SUCCESS;
}


