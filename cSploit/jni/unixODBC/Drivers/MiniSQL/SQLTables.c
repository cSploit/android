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

enum nSQLTables
{
	TABLE_CAT		= 1,
	TABLE_SCHEM,
	TABLE_NAME,
	TABLE_TYPE,
	REMARKS,
	COL_MAX
};

m_field aSQLTables[] =
{
	"",					"",		0,			0,	0,	/* keep things 1 based */
	"TABLE_CAT",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_SCHEM",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_NAME",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_TYPE",		"sys", CHAR_TYPE,	50,	0,
	"REMARKS",			"sys", CHAR_TYPE,	50,	0
};
	

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
    m_result   	*pResults;						/* mSQL DATA	*/
	m_field		*pField;						/* mSQL field	*/
	COLUMNHDR	*pColumnHeader;			
    m_row      	rowResult;						/* mSQL ROW		*/
	int			nColumn;
	long		nResultMemory;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

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
	pResults = msqlListTables( ((HDRVDBC)hStmt->hDbc)->hDbcExtras->hServer );
    if ( pResults == NULL )
	{
        sprintf( hStmt->szSqlMsg, "SQL_ERROR Query failed. %s", msqlErrMsg );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
        return SQL_ERROR;
	}

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/
	hStmt->hStmtExtras->nCols 		= COL_MAX-1;
    hStmt->hStmtExtras->nRows		= msqlNumRows( pResults );
    hStmt->hStmtExtras->nRow		= 0;
    nResultMemory = sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1);
	hStmt->hStmtExtras->aResults 	=  malloc( nResultMemory );
	if ( hStmt->hStmtExtras->aResults == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "Not enough memory. (malloc failed)" );
        hStmt->hStmtExtras->nRows = 0;
        hStmt->hStmtExtras->nCols = 0;	
		msqlFreeResult( pResults );
        return SQL_ERROR;
	}

    /**************************
	 * gather column header information (save col 0 for bookmarks)
     **************************/
	for ( nColumn = 1; nColumn < COL_MAX; nColumn++ )
	{
        (hStmt->hStmtExtras->aResults)[nColumn] = malloc( sizeof(COLUMNHDR) );
        pColumnHeader = (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nColumn];
		memset( pColumnHeader, 0, sizeof(COLUMNHDR) );
		_NativeToSQLColumnHeader( pColumnHeader, &(aSQLTables[nColumn]) );
	}

	/************************
	 * gather data (save col 0 for bookmarks)
	 ************************/
    hStmt->hStmtExtras->nRow = 0;
    while ( (rowResult = msqlFetchRow( pResults )) != NULL )
	{
        hStmt->hStmtExtras->nRow++;
		msqlFieldSeek( pResults, 0 );
		for ( nColumn = 1; nColumn <= hStmt->hStmtExtras->nCols; nColumn++ )
		{
			switch ( nColumn )
			{
			case TABLE_NAME:
				(hStmt->hStmtExtras->aResults)[hStmt->hStmtExtras->nRow*hStmt->hStmtExtras->nCols+nColumn] = (char *)strdup( rowResult[0] );
				break;
			default:
				(hStmt->hStmtExtras->aResults)[hStmt->hStmtExtras->nRow*hStmt->hStmtExtras->nCols+nColumn] = NULL;
			}
		}
	}
	hStmt->hStmtExtras->nRow = 0;

    /**************************
	 * free the snapshot
     **************************/
	msqlFreeResult( pResults );

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}



