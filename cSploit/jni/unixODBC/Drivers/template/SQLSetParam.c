/********************************************************************
 * SQLSetParam (deprecated)
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 ********************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN   SQLSetParam(SQLHSTMT hDrvStmt,
           SQLUSMALLINT nPar, SQLSMALLINT nType,
           SQLSMALLINT nSqlType, SQLULEN nColDef,
           SQLSMALLINT nScale, SQLPOINTER pValue,
           SQLLEN *pnValue)
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

	if ( NULL == hStmt->pszQuery )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No prepared statement to work with" );
		return SQL_ERROR;
	}

	/******************
	 * 1. Your param storage is in hStmt->hStmtExtras
	 *    so you will have to code for it. Do it here.
	 ******************/
	

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );


	return SQL_ERROR;
}


