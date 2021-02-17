/**********************************************************************
 * SQLBindParameter
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

SQLRETURN  SQLBindParameter( SQLHSTMT           hDrvStmt,
							 SQLUSMALLINT       nParameterNumber,
							 SQLSMALLINT		nIOType,
							 SQLSMALLINT        nBufferType,
							 SQLSMALLINT        nParamType,
							 SQLULEN            nParamLength,
							 SQLSMALLINT        nScale,
							 SQLPOINTER         pData,
							 SQLLEN             nBufferLength,
							 SQLLEN             *pnLengthOrIndicator
						   )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt=$%08lX nParameterNumber=%d nIOType=%d nBufferType=%d nParamType=%d nParamLength=%ld nScale=%d pData=$%08lX nBufferLength=%ld *pnLengthOrIndicator=$%08lX",(long) hStmt,nParameterNumber,nIOType,nBufferType,nParamType,(long) nParamLength,nScale,(long) pData,(long) nBufferLength, *pnLengthOrIndicator );

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not currently supported" );

    return SQL_ERROR;
}


