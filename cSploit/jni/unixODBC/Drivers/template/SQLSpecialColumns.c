/********************************************************************
 * SQLSpecialColumns
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

/* THIS IS CORRECT...
SQLRETURN SQLSpecialColumns(  SQLHSTMT        hDrvStmt,
									SQLSMALLINT     nColumnType,
									SQLCHAR         *szCatalogName,
									SQLSMALLINT     nCatalogNameLength,
									SQLCHAR         *szSchemaName,
									SQLSMALLINT     nSchemaNameLength,
									SQLCHAR         *szTableName,
									SQLSMALLINT     nTableNameLength,
									SQLSMALLINT     nScope,
									SQLSMALLINT     nNullable )
THIS WORKS... */
SQLRETURN SQLSpecialColumns(  SQLHSTMT        hDrvStmt,
									UWORD     nColumnType,
									UCHAR         *szCatalogName,
									SWORD     nCatalogNameLength,
									UCHAR         *szSchemaName,
									SWORD     nSchemaNameLength,
									UCHAR         *szTableName,
									SWORD     nTableNameLength,
									UWORD     nScope,
									UWORD     nNullable )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );


    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );


	return SQL_ERROR;
}



