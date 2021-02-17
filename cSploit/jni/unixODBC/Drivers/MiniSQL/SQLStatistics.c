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

m_field aSQLStatistics[] =
{
	"",					"",		0,			0,	0,
	"TABLE_CAT",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_SCHEM",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_NAME",		"sys", CHAR_TYPE,	50,	0,
	"NON_UNIQUE",		"sys", INT_TYPE,	3,	0,
	"INDEX_QUALIFIER",	"sys", CHAR_TYPE,	50,	0,
	"INDEX_NAME",		"sys", CHAR_TYPE,	50,	0,
	"TYPE",				"sys", INT_TYPE,	3,	0,
   	"ORDINAL_POSITION",	"sys", CHAR_TYPE,	50,	0,
	"COLUMN_NAME",		"sys", CHAR_TYPE,	50,	0,
	"ASC_OR_DESC",		"sys", INT_TYPE,	3,	0,
	"CARDINALITY",		"sys", INT_TYPE,	3,	0,
	"PAGES",			"sys", INT_TYPE,	3,	0,
	"FILTER_CONDITION",	"sys", CHAR_TYPE,	50,	0
};
	

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
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;
    m_result   	*pResults;						/* mSQL DATA	*/
	m_field		*pField;						/* mSQL field	*/
	COLUMNHDR	*pColumnHeader;			
	int			nColumn;
	long		nCols;
	long		nCurRow;
	long		nRow;
	char		szBuffer[101];
	int			nIndexColumns 		= 0;
	int			nNormalColumns		= 0;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

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

	hStmt->pszQuery							= NULL;
	
    /************************
     * generate a result set listing columns (these will be our rows)
     ************************/
	pResults = msqlListFields( ((HDRVDBC)hStmt->hDbc)->hDbcExtras->hServer, szTableName );
    if ( pResults == NULL )
	{
        sprintf( hStmt->szSqlMsg, "SQL_ERROR Query failed. %s", msqlErrMsg );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
        return SQL_ERROR;
	}

    /************************
     * how many columns are indexs?
     ************************/
    nIndexColumns = 0;
	while ( pField = msqlFetchField( pResults ) )
	{
		if ( pField->type == IDX_TYPE)
            nIndexColumns++;
	}
	msqlFieldSeek( pResults, 0 );
	nNormalColumns = msqlNumFields( pResults ) - nIndexColumns;

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/
	hStmt->hStmtExtras->nCols 		= COL_MAX-1;
	hStmt->hStmtExtras->nRows 		= nIndexColumns;
    hStmt->hStmtExtras->nRow		= 0;
	hStmt->hStmtExtras->aResults 	=  malloc( sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );
	memset( hStmt->hStmtExtras->aResults, 0, sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );
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
		_NativeToSQLColumnHeader( pColumnHeader, &(aSQLStatistics[nColumn]) );
	}

	/************************
	 * gather data (save col 0 for bookmarks and factor out normal columns)
	 ************************/
	nCols 						= hStmt->hStmtExtras->nCols;
	hStmt->hStmtExtras->nRow 	= 0;
    for ( nCurRow = 1; nCurRow <= msqlNumFields( pResults ); nCurRow++ )
	{
		pField = msqlFetchField( pResults );
        if ( pField->type == IDX_TYPE)
        {
			hStmt->hStmtExtras->nRow++;
			nRow = hStmt->hStmtExtras->nRow;
			for ( nColumn = 1; nColumn < COL_MAX; nColumn++ )
			{
				switch ( nColumn )
				{
				case TABLE_NAME:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( szTableName );
					break;
				case INDEX_NAME:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( pField->name );
					break;
				case NON_UNIQUE:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char *)strdup( IS_NOT_NULL( pField->flags ) ? "0" : "1" );
					break;
				default:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = NULL;
				} /* switch nColumn */
			} /* for nColumn */
		} /* if */
	} /* for nRow */
	hStmt->hStmtExtras->nRow  = 0;

    /**************************
	 * free the snapshot
     **************************/
	msqlFreeResult( pResults );

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
    return SQL_SUCCESS;
}


