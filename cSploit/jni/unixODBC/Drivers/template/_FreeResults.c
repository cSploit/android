/**********************************************************************
 * _FreeResults
 *
 **********************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN _FreeResults( HSTMTEXTRAS hStmt )
{
	COLUMNHDR	*pColumnHeader;
	int 		nCurColumn;

	if ( hStmt == NULL )
		return SQL_ERROR;
	if ( hStmt->aResults == NULL )
		return SQL_SUCCESS;

	/* COLUMN HDRS (col=0 is not used) */
	for ( nCurColumn = 1; nCurColumn <= hStmt->nCols; nCurColumn++ )
	{
        pColumnHeader	= (COLUMNHDR*)(hStmt->aResults)[nCurColumn];
		free( pColumnHeader->pszSQL_DESC_BASE_COLUMN_NAME );
		free( pColumnHeader->pszSQL_DESC_BASE_TABLE_NAME );
		free( pColumnHeader->pszSQL_DESC_CATALOG_NAME );
		free( pColumnHeader->pszSQL_DESC_LABEL );
		free( pColumnHeader->pszSQL_DESC_LITERAL_PREFIX );
		free( pColumnHeader->pszSQL_DESC_LITERAL_SUFFIX );
		free( pColumnHeader->pszSQL_DESC_LOCAL_TYPE_NAME );
		free( pColumnHeader->pszSQL_DESC_NAME );
		free( pColumnHeader->pszSQL_DESC_SCHEMA_NAME );
		free( pColumnHeader->pszSQL_DESC_TABLE_NAME );
		free( pColumnHeader->pszSQL_DESC_TYPE_NAME );
		free( (hStmt->aResults)[nCurColumn] );
	}

	/* RESULT DATA (col=0 is bookmark) */
	for ( hStmt->nRow = 1; hStmt->nRow <= hStmt->nRows; hStmt->nRow++ )
	{
		for ( nCurColumn = 1; nCurColumn <= hStmt->nCols; nCurColumn++ )
		{
			free( (hStmt->aResults)[hStmt->nRow*hStmt->nCols+nCurColumn] );
		}
	}
	free( hStmt->aResults );
    hStmt->aResults = NULL;
    hStmt->nCols	= 0;
    hStmt->nRows	= 0;
    hStmt->nRow		= 0;
	
	return SQL_SUCCESS;
}



