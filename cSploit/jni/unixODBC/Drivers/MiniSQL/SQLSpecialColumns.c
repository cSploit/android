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

enum nSQLSpecialColumns
{
	COLUMN_NAME		= 1,
	COL_MAX
};

m_field aSQLSpecialColumns[] =
{
	"",					"",		0,			0,	0,
	"COLUMN_NAME",		"sys", CHAR_TYPE,	50,	0
};
	

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
	COLUMNHDR	*pColumnHeader;			
	int			nColumn;
	long		nCols;
	long		nRow;
	long		nCurRow;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

    if ( !szTableName )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	/* validate nColumnType */
	switch ( nColumnType )
	{
	case SQL_BEST_ROWID:
		break;
	case SQL_ROWVER:
	default:
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );
		return SQL_ERROR;
	}
	
	/* validate nScope */
	switch ( nScope )
	{
	case SQL_SCOPE_CURROW:
	case SQL_SCOPE_TRANSACTION:
	case SQL_SCOPE_SESSION:
		break;
	default:
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );
		return SQL_ERROR;
	}

	/* validate nNullable */
	switch ( nNullable )
	{
	case SQL_NO_NULLS:
	case SQL_NULLABLE:
		break;
	default:
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );
		return SQL_ERROR;
	}

	/* only one scenario supported: nColumnType=SQL_BEST_ROWID and nScope=ignored nNullable=ignored */

    /**************************
	 * close any existing result
     **************************/
	if ( hStmt->hStmtExtras->aResults )
		_FreeResults( hStmt->hStmtExtras );

	if ( hStmt->pszQuery != NULL )
		free( hStmt->pszQuery );

	hStmt->pszQuery							= NULL;

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/
	hStmt->hStmtExtras->nCols 		= COL_MAX-1;
	hStmt->hStmtExtras->nRows 		= 1;
    hStmt->hStmtExtras->nRow		= 0;
	hStmt->hStmtExtras->aResults 	=  malloc( sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );
	memset( hStmt->hStmtExtras->aResults, 0, sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );
	if ( hStmt->hStmtExtras->aResults == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "Not enough memory. (malloc failed)" );
        hStmt->hStmtExtras->nRows = 0;
        hStmt->hStmtExtras->nCols = 0;	
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
		_NativeToSQLColumnHeader( pColumnHeader, &(aSQLSpecialColumns[nColumn]) );
	}

	/************************
	 * gather data (save col 0 for bookmarks and factor out index columns)
	 ************************/
	nCols 						= hStmt->hStmtExtras->nCols;
	hStmt->hStmtExtras->nRow	= 0;
    nCurRow 					= 1;
	hStmt->hStmtExtras->nRow++;
	nRow						= hStmt->hStmtExtras->nRow;
	for ( nColumn = 1; nColumn < COL_MAX; nColumn++ )
	{
		switch ( nColumn )
		{
		case COLUMN_NAME:
			(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char*)strdup( "_rowid" );
			break;
		default:
			(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = NULL;
		} /* switch nColumn */
	} /* for nColumn */
	hStmt->hStmtExtras->nRow  = 0;

    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}



