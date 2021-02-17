/**************************************************
 * _FreeStmt
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 31.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
#include <config.h>
#include "driver.h"

SQLRETURN _FreeStmt( SQLHSTMT hDrvStmt )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;
	HDRVSTMT	hPrevStmt;
	SQLRETURN	nReturn;

	if ( hStmt == SQL_NULL_HDBC )
		return SQL_ERROR;

	/* SPECIAL CHECK FOR FIRST IN LIST 				*/
	if ( ((HDRVDBC)hStmt->hDbc)->hFirstStmt == hStmt )
		((HDRVDBC)hStmt->hDbc)->hFirstStmt = hStmt->pNext;
	
	/* SPECIAL CHECK FOR LAST IN LIST 				*/
	if ( ((HDRVDBC)hStmt->hDbc)->hLastStmt == hStmt )
		((HDRVDBC)hStmt->hDbc)->hLastStmt = hStmt->pPrev;

	/* EXTRACT SELF FROM LIST						*/
	if ( hStmt->pPrev != SQL_NULL_HSTMT )
		hStmt->pPrev->pNext = hStmt->pNext;
	if ( hStmt->pNext != SQL_NULL_HSTMT )
		hStmt->pNext->pPrev = hStmt->pPrev;

	/* FREE STANDARD MEMORY */
	if( NULL != hStmt->pszQuery ) free( hStmt->pszQuery );

/*********************************************************************/
/* 	!!! FREE DRIVER SPECIFIC MEMORY (hidden in hStmtExtras) HERE !!! */
	_FreeResults( hStmt->hStmtExtras );
	free( hStmt->hStmtExtras );
/*********************************************************************/

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	logClose( hStmt->hLog );
	free( hStmt );

	return SQL_SUCCESS;
}



