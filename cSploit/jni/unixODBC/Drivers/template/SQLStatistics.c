/********************************************************************
 * SQLStatistics
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

enum nSQLStatistics
{
	TABLE_CAT		= 1,
	TABLE_SCHEM,
	TABLE_NAME,
	NON_UNIQUE,
	INDEX_QUALIFIER,
	INDEX_NAME,
	TYPE,
	ORDINAL_POSITION,
	COLUMN_NAME,
	ASC_OR_DESC,
	CARDINALITY,
	PAGES,
	FILTER_CONDITION,
	COL_MAX
};


/****************************
 * replace this with init of some struct (see same func for MiniSQL driver) */
char *aSQLStatistics[] =
{
	"one",
	"two"
};
/****************************/


SQLRETURN SQLStatistics(	SQLHSTMT        hDrvStmt,
							SQLCHAR         *szCatalogName,
							SQLSMALLINT     nCatalogNameLength,
							SQLCHAR         *szSchemaName,
							SQLSMALLINT     nSchemaNameLength,
							SQLCHAR         *szTableName,      		/* MUST BE SUPPLIED */
							SQLSMALLINT     nTableNameLength,
							SQLUSMALLINT    nTypeOfIndex,
							SQLUSMALLINT    nReserved    )
{
    HDRVSTMT 	hStmt	= (HDRVSTMT)hDrvStmt;
	int			nColumn;
	int			nCols;
	int			nRow;
	COLUMNHDR	*pColumnHeader;			
	char		szSQL[200];

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hStmt->szSqlMsg, "hStmt = $%08lX", (long) hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );

	if ( szTableName == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No table name" );
		return SQL_ERROR;
	}

	if ( szTableName[0] == '\0'  )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No table name" );
		return SQL_ERROR;
	}

    /**************************
	 * close any existing result
     **************************/
	if ( hStmt->hStmtExtras->aResults )
		_FreeResults( hStmt->hStmtExtras );

	if ( hStmt->pszQuery != NULL )
		free( hStmt->pszQuery );
	hStmt->pszQuery = NULL;

    /**************************
	 * EXEC QUERY TO GET KEYS
     **************************/

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


