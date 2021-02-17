/********************************************************************
 * SQLColumns
 *
 * This is something of a mess. Part of the problem here is that msqlListFields
 * are returned as mSQL 'column headers'... we want them to be returned as a
 * row for each. So we have to turn the results on their side <groan>.
 *
 * Another problem is that msqlListFields will also return indexs. So we have
 * to make sure that these are not included here.
 *
 * The end result is more code than what would usually be found here.
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

enum nSQLColumns
{
	TABLE_CAT		= 1,
	TABLE_SCHEM,
	TABLE_NAME,
	COLUMN_NAME,
	DATA_TYPE,
	TYPE_NAME,
	COLUMN_SIZE,
	BUFFER_LENGTH,
	DECIMAL_DIGITS,
	NUM_PREC_RADIX,
	NULLABLE,
	REMARKS,
	COLUMN_DEF,
	SQL_DATA_TYPE,
	SQL_DATETIME_SUB,
	CHAR_OCTET_LENGTH,
   	ORDINAL_POSITION,
	IS_NULLABLE,
	COL_MAX
};

/****************************
 * replace this with init of some struct (see same func for MiniSQL driver) */
char *aSQLColumns[] =
{
	"one",
	"two"
};
 /***************************/

SQLRETURN SQLColumns( 	SQLHSTMT    hDrvStmt,
						SQLCHAR     *szCatalogName,
						SQLSMALLINT nCatalogNameLength,
						SQLCHAR     *szSchemaName,
						SQLSMALLINT nSchemaNameLength,
						SQLCHAR     *szTableName,
						SQLSMALLINT nTableNameLength,
						SQLCHAR     *szColumnName,
						SQLSMALLINT nColumnNameLength )
{
    HDRVSTMT	hStmt		= (HDRVSTMT)hDrvStmt;

	COLUMNHDR	*pColumnHeader;			
	int			nColumn;
	long		nCols;
	long		nRow;
	char		szBuffer[101];


    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long)hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

	if ( szTableName == NULL || szTableName[0] == '\0' )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Must supply a valid table name" );
		return SQL_ERROR;
	}

    /**************************
	 * close any existing result
     **************************/
	if ( hStmt->hStmtExtras->aResults )
		_FreeResults( hStmt->hStmtExtras );

	if ( hStmt->pszQuery != NULL )
		free( hStmt->pszQuery );

    /************************
     * generate a result set listing columns
     ************************/

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/

    /**************************
	 * gather column header information (save col 0 for bookmarks)
     **************************/

	/************************
	 * gather data (save col 0 for bookmarks and factor out index columns)
	 ************************/

    /**************************
	 * free the snapshot
     **************************/

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
    return SQL_SUCCESS;
}



