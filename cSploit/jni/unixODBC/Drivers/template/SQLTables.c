/**********************************************************************
 * SQLTables
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

/****************************
 * STANDARD COLUMNS RETURNED BY SQLTables
 ***************************/
enum nSQLTables
{
	TABLE_CAT		= 1,
	TABLE_SCHEM,
	TABLE_NAME,
	TABLE_TYPE,
	REMARKS,
	COL_MAX
};

/****************************
 * COLUMN HEADERS (1st row of result set)
 ***************************/

/****************************
 * replace this with init of some struct (see same func for MiniSQL driver) */
char *aSQLTables[] =
{
	"one",
	"two"
};
/****************************/

SQLRETURN SQLTables(  	SQLHSTMT    hDrvStmt,
						SQLCHAR     *szCatalogName,
						SQLSMALLINT nCatalogNameLength,
						SQLCHAR     *szSchemaName,
						SQLSMALLINT nSchemaNameLength,
						SQLCHAR     *szTableName,
						SQLSMALLINT nTableNameLength,
						SQLCHAR     *szTableType,
						SQLSMALLINT nTableTypeLength )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;
	COLUMNHDR	*pColumnHeader;			
	int			nColumn;
	long		nResultMemory;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

    /**************************
	 * close any existing result
     **************************/
	if ( hStmt->hStmtExtras->aResults )
		_FreeResults( hStmt->hStmtExtras );

	if ( hStmt->pszQuery != NULL )
		free( hStmt->pszQuery );

	hStmt->pszQuery							= NULL;
	
    /************************
     * generate a result set listing tables
     ************************/

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/

    /**************************
	 * gather column header information (save col 0 for bookmarks)
     **************************/

	/************************
	 * gather data (save col 0 for bookmarks)
	 ************************/

    /**************************
	 * free the snapshot
     **************************/

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}



