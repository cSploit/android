/**********************************************************************
 * SQLExecute
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

SQLRETURN _Execute( SQLHSTMT  hDrvStmt )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;
	int				nColumn;
	int				nCols;
	int				nRow;
    m_result    	*pResults;						/* mSQL DATA	*/
    m_row       	rowResult;						/* mSQL ROW		*/
    m_field     	*pField;						/* mSQL COL HDR	*/
	COLUMNHDR		*pColumnHeader;			

	/* SANITY CHECKS */
    if( NULL == hStmt )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

    if( hStmt->pszQuery == NULL )
    {
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No prepared statement" );
        return SQL_ERROR;
    }

    /**************************
	 * Free any current results
     **************************/
	if ( hStmt->hStmtExtras->aResults )
		_FreeResults( hStmt->hStmtExtras );

    /**************************
	 * send prepared query to server
     **************************/
    if ( (hStmt->hStmtExtras->nRows = msqlQuery( ((HDRVDBC)hStmt->hDbc)->hDbcExtras->hServer, hStmt->pszQuery )) == -1 )
    {
        sprintf( hStmt->szSqlMsg, "SQL_ERROR Query failed. %s", msqlErrMsg );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
        return SQL_ERROR;
    }

    /**************************
	 * snapshot our results (assume no results means UPDATE, DELETE or INSERT
     **************************/
    pResults = msqlStoreResult();
    if ( !pResults )
        return SQL_SUCCESS;

    /**************************
	 * allocate memory for columns headers and result data (row 0 is column header while col 0 is reserved for bookmarks)
     **************************/
    hStmt->hStmtExtras->nRows		= msqlNumRows( pResults );
    hStmt->hStmtExtras->nCols 		= msqlNumFields( pResults );
	hStmt->hStmtExtras->aResults 	=  malloc( sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );
	if ( hStmt->hStmtExtras->aResults == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "Not enough memory. (malloc failed)" );
        hStmt->hStmtExtras->nRows = 0;
        hStmt->hStmtExtras->nCols = 0;	
		msqlFreeResult( pResults );
        return SQL_ERROR;
	}
	memset( hStmt->hStmtExtras->aResults, 0, sizeof(char*) * (hStmt->hStmtExtras->nRows+1) * (hStmt->hStmtExtras->nCols+1) );

    /**************************
	 * gather column header information (save col 0 for bookmarks)
     **************************/
	for ( nColumn = 1; nColumn <= hStmt->hStmtExtras->nCols; nColumn++ )
	{
		pField = msqlFetchField( pResults );
        (hStmt->hStmtExtras->aResults)[nColumn] = malloc( sizeof(COLUMNHDR) );
		memset( (hStmt->hStmtExtras->aResults)[nColumn], 0, sizeof(COLUMNHDR) );
        pColumnHeader = (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nColumn];
		_NativeToSQLColumnHeader( pColumnHeader, pField  );
	}

	/************************
	 * gather data (save col 0 for bookmarks)
	 ************************/
	nCols 	= hStmt->hStmtExtras->nCols;
	nRow	= 0;
    while ( (rowResult = msqlFetchRow( pResults )) != NULL )
	{
        nRow++;
		msqlFieldSeek( pResults, 0 );
		for ( nColumn=1; nColumn <= nCols; nColumn++ )
		{
			if ( rowResult[nColumn-1] )
				(hStmt->hStmtExtras->aResults)[nRow*nCols+nColumn] = (char *)strdup( rowResult[nColumn-1] );
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




