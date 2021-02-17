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

m_field aSQLColumns[] =
{
	"",					"",		0,			0,	0,
	"TABLE_CAT",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_SCHEM",		"sys", CHAR_TYPE,	50,	0,
	"TABLE_NAME",		"sys", CHAR_TYPE,	50,	0,
	"COLUMN_NAME",		"sys", CHAR_TYPE,	50,	0,
	"DATA_TYPE",		"sys", INT_TYPE,	3,	0,
	"TYPE_NAME",		"sys", CHAR_TYPE,	50,	0,
	"COLUMN_SIZE",		"sys", INT_TYPE,	3,	0,
	"BUFFER_LENGTH",	"sys", INT_TYPE,	3,	0,
	"DECIMAL_DIGITS",	"sys", INT_TYPE,	3,	0,
	"NUM_PREC_RADIX",	"sys", INT_TYPE,	3,	0,
	"NULLABLE",			"sys", INT_TYPE,	3,	0,
	"REMARKS",			"sys", CHAR_TYPE,	50,	0,
	"COLUMN_DEF",		"sys", CHAR_TYPE,	50,	0,
	"SQL_DATA_TYPE",	"sys", INT_TYPE,	3,	0,
	"SQL_DATETIME_SUB",	"sys", INT_TYPE,	3,	0,
	"CHAR_OCTET_LENGTH","sys", INT_TYPE,	3,	0,
   	"ORDINAL_POSITION",	"sys", CHAR_TYPE,	50,	0,
	"IS_NULLABLE",		"sys", CHAR_TYPE,	50,	0
};
	

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
    m_result   	*pResults;						/* mSQL DATA	*/
	m_field		*pField;						/* mSQL field	*/
	COLUMNHDR	*pColumnHeader;			
	int			nColumn;
	long		nCols;
	long		nRow;
	long		nCurRow;
	char		szBuffer[101];
	int			nIndexColumns = 0;

    /* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

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

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/
	hStmt->hStmtExtras->nCols 		= COL_MAX-1;
	hStmt->hStmtExtras->nRows 		= msqlNumFields( pResults ) - nIndexColumns;
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
		_NativeToSQLColumnHeader( pColumnHeader, &(aSQLColumns[nColumn]) );
	}

	/************************
	 * gather data (save col 0 for bookmarks and factor out index columns)
	 ************************/
	nCols 	= hStmt->hStmtExtras->nCols;
	hStmt->hStmtExtras->nRow	= 0;
    for ( nCurRow = 1; nCurRow <= msqlNumFields( pResults ); nCurRow++ )
	{
		pField = msqlFetchField( pResults );
        if ( pField->type != IDX_TYPE)
        {
			hStmt->hStmtExtras->nRow++;
			nRow	= hStmt->hStmtExtras->nRow;
			for ( nColumn = 1; nColumn < COL_MAX; nColumn++ )
			{
				switch ( nColumn )
				{
				case TABLE_NAME:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( szTableName );
					break;
				case COLUMN_NAME:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( pField->name );
					break;
				case TYPE_NAME:
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( msqlTypeNames[pField->type] );
					break;
				case COLUMN_SIZE:
					sprintf( szBuffer, "%d", pField->length );
					(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( szBuffer );
					break;
				case NULLABLE:
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



