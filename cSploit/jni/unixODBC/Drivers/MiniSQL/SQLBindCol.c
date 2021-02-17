/**********************************************************************
 * SQLBindCol
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

/* THIS IS THE CORRECT SYNTAX....
SQLRETURN SQLBindCol(	SQLHSTMT        hDrvStmt,
						SQLUSMALLINT    nCol,
						SQLSMALLINT     nTargetType,
						SQLPOINTER      pTargetValue,
						SQLINTEGER      nTargetValueMax,
						SQLINTEGER      *pnLengthOrIndicator )
AND THIS IS WHAT WORKS... */						
SQLRETURN SQLBindCol(	SQLHSTMT        hDrvStmt,
						UWORD    nCol,
						SWORD     nTargetType,
						PTR      pTargetValue,
						SDWORD      nTargetValueMax,
						SDWORD      *pnLengthOrIndicator )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;
	COLUMNHDR		*pColumnHeader;			

    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt=$%08lX nCol=%5d", hStmt, nCol );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	if ( hStmt->hStmtExtras->nRows == 0 )
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
	if ( pTargetValue == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Invalid data pointer" );
		return SQL_ERROR;
	}

	if ( pnLengthOrIndicator != NULL )
		*pnLengthOrIndicator = 0;

	/* SET DEFAULTS */

	/* validate length etc */

	/* store app col pointer */
	pColumnHeader	= (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nCol];
	pColumnHeader->nTargetType			= nTargetType;
	pColumnHeader->nTargetValueMax		= nTargetValueMax;
	pColumnHeader->pnLengthOrIndicator	= pnLengthOrIndicator;
	pColumnHeader->pTargetValue			= pTargetValue;

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
    return SQL_SUCCESS;
}



