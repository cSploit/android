/**********************************************************************
 * _AllocStmt (deprecated)
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

SQLRETURN _AllocStmt(   SQLHDBC     hDrvDbc,
                          SQLHSTMT    *phDrvStmt )
{
    HDRVDBC     hDbc	= (HDRVDBC)hDrvDbc;
	HDRVSTMT    *phStmt	= (HDRVSTMT*)phDrvStmt;

    /* SANITY CHECKS */
    if( hDbc == SQL_NULL_HDBC )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_INVALID_HANDLE" );
        return SQL_INVALID_HANDLE;
    }

	sprintf( hDbc->szSqlMsg, "hDbc = $%08lX", hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );

    if( NULL == phStmt )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR phStmt=NULL" );
        return SQL_ERROR;
    }

    /* OK */

    /* allocate memory */
    *phStmt = malloc( sizeof(DRVSTMT) );
    if( SQL_NULL_HSTMT == *phStmt )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR memory allocation failure" );
        return SQL_ERROR;
    }

    /* initialize memory */
	sprintf( hDbc->szSqlMsg, "*phstmt = $%08lX", *phStmt );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );

	memset( *phStmt, 0, sizeof(DRVSTMT) );	/* SAFETY */
    (*phStmt)->hDbc			= (SQLPOINTER)hDbc;
    (*phStmt)->hLog			= NULL;
    (*phStmt)->hStmtExtras	= NULL;
    (*phStmt)->pNext		= NULL;
    (*phStmt)->pPrev		= NULL;
    (*phStmt)->pszQuery		= NULL;
    sprintf( (*phStmt)->szCursorName, "CUR_%08lX", *phStmt );

	/* ADD TO DBCs STATEMENT LIST */
	
	/* start logging		*/
    if ( logOpen( &(*phStmt)->hLog, SQL_DRIVER_NAME, NULL, 50 ) )
	{
		logOn( (*phStmt)->hLog, 1 );
		logPushMsg( (*phStmt)->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "Statement logging allocated ok" );
	}
	else
		(*phStmt)->hLog = NULL;

	/* ADD TO END OF LIST */
	if ( hDbc->hFirstStmt == NULL )
	{
		/* 1st is null so the list is empty right now */
        hDbc->hFirstStmt		= (*phStmt);
        hDbc->hLastStmt			= (*phStmt);
	}
	else
	{
		/* at least one node in list */
		hDbc->hLastStmt->pNext	= (SQLPOINTER)(*phStmt);
		(*phStmt)->pPrev		= (SQLPOINTER)hDbc->hLastStmt;
        hDbc->hLastStmt			= (*phStmt);
	}

/****************************************************************************/
/* ALLOCATE AND INIT DRIVER EXTRAS HERE 									*/
    (*phStmt)->hStmtExtras = malloc(sizeof(STMTEXTRAS));
	memset( (*phStmt)->hStmtExtras, 0, sizeof(STMTEXTRAS) ); /* SAFETY */
    (*phStmt)->hStmtExtras->aResults	= NULL;
	(*phStmt)->hStmtExtras->nCols		= 0;
	(*phStmt)->hStmtExtras->nRow		= 0;
	(*phStmt)->hStmtExtras->nRows		= 0;
/****************************************************************************/

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}



