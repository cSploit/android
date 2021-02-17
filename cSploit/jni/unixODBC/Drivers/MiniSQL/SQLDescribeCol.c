/**********************************************************************
 * SQLDescribeCol
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

SQLRETURN SQLDescribeCol( SQLHSTMT    	hDrvStmt,
						  SQLUSMALLINT	nCol,
						  SQLCHAR     	*szColName,
						  SQLSMALLINT 	nColNameMax,
						  SQLSMALLINT 	*pnColNameLength,
						  SQLSMALLINT 	*pnSQLDataType,
						  SQLUINTEGER 	*pnColSize,
						  SQLSMALLINT 	*pnDecDigits,
						  SQLSMALLINT 	*pnNullable )
{
    HDRVSTMT 	hStmt			= (HDRVSTMT)hDrvStmt;
	COLUMNHDR	*pColumnHeader;

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	if ( hStmt->hStmtExtras->nRows < 1 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No result set." );
		return SQL_ERROR;
	}
	if ( nCol < 1 || nCol > hStmt->hStmtExtras->nCols )
	{
		sprintf( hStmt->szSqlMsg, "SQL_ERROR Column %d is out of range. Range is 1 - %s", nCol, hStmt->hStmtExtras->nCols );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
		return SQL_ERROR;
	}

	/****************************
	 * Ok
	 ***************************/
	pColumnHeader = (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nCol];
	if ( szColName )
		strncpy( szColName, pColumnHeader->pszSQL_DESC_NAME, nColNameMax );
	if ( pnColNameLength )
		*pnColNameLength = strlen( szColName );
	if ( pnSQLDataType )
		*pnSQLDataType = pColumnHeader->nSQL_DESC_TYPE;
	if ( pnColSize )
		*pnColSize = pColumnHeader->nSQL_DESC_LENGTH;
	if ( pnDecDigits )
		*pnDecDigits = pColumnHeader->nSQL_DESC_SCALE;
	if ( pnNullable )
		*pnNullable = pColumnHeader->nSQL_DESC_NULLABLE;


	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}

