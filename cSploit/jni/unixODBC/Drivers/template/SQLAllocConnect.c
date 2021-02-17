/**********************************************************************
 * SQLAllocConnect
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


SQLRETURN _AllocConnect(	SQLHENV    hDrvEnv,
							SQLHDBC    *phDrvDbc )
{
	HDRVENV	hEnv	= (HDRVENV)hDrvEnv;
	HDRVDBC *phDbc	= (HDRVDBC*)phDrvDbc;

	
    /************************
     * SANITY CHECKS
     ************************/
    if( SQL_NULL_HENV == hEnv )
        return SQL_INVALID_HANDLE;

    sprintf((char*) hEnv->szSqlMsg, "hEnv = $%08lX phDbc = $%08lX",	(long)hEnv, (long)phDbc );
    logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hEnv->szSqlMsg );

    if( SQL_NULL_HDBC == phDbc )
    {
        logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR *phDbc is NULL" );
        return SQL_ERROR;
    }

    /************************
     * OK LETS DO IT
     ************************/

    /* allocate database access structure */
    *phDbc = (HDRVDBC)malloc( sizeof(DRVDBC) );
    if( SQL_NULL_HDBC == *phDbc )
    {
		*phDbc = SQL_NULL_HDBC;
        logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR malloc error" );
        return SQL_ERROR;
    }

    /* initialize structure */
    memset( *phDbc, 0, sizeof(DRVDBC) );

    (*phDbc)->bConnected	= 0;
    (*phDbc)->hDbcExtras	= NULL;
    (*phDbc)->hFirstStmt	= NULL;
    (*phDbc)->hLastStmt		= NULL;
    (*phDbc)->pNext			= NULL;
    (*phDbc)->pPrev			= NULL;
    (*phDbc)->hEnv			= (SQLPOINTER)hEnv;
    

	/* start logging		*/
    if ( !logOpen( &(*phDbc)->hLog, "[template]", NULL, 50 ) )
		(*phDbc)->hLog = NULL;
	logOn( (*phDbc)->hLog, 1 );

	/* ADD TO END OF LIST */
	if ( hEnv->hFirstDbc == NULL )
	{
		/* 1st is null so the list is empty right now */
        hEnv->hFirstDbc			= (*phDbc);
        hEnv->hLastDbc			= (*phDbc);
	}
	else
	{
		/* at least one node in list */
		hEnv->hLastDbc->pNext	= (SQLPOINTER)(*phDbc);
		(*phDbc)->pPrev			= (SQLPOINTER)hEnv->hLastDbc;
        hEnv->hLastDbc			= (*phDbc);
	}

/********************************************************/
/* ALLOCATE AND INIT EXTRAS HERE 						*/
    (*phDbc)->hDbcExtras = (HDBCEXTRAS)malloc( sizeof(DBCEXTRAS) );
    memset( (*phDbc)->hDbcExtras, 0, sizeof(DBCEXTRAS) );
/********************************************************/

    logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}

SQLRETURN SQLAllocConnect(	SQLHENV    hDrvEnv,
				SQLHDBC    *phDrvDbc )
{
  return _AllocConnect( hDrvEnv, phDrvDbc );
}

