/**********************************************************************
 * SQLPrepare
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

SQLRETURN _Prepare(	SQLHSTMT    hDrvStmt,
						SQLCHAR     *szSqlStr,
						SQLINTEGER  nSqlStrLength )
{	
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
	if ( NULL == hStmt )
		return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

	if ( szSqlStr == NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No SQL to process" );
		return SQL_ERROR;
	}

	if ( hStmt->pszQuery != NULL )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Statement already in use." );
		return SQL_ERROR;
	}

	/* allocate and copy statement to buffer (process escape sequences and parameter tokens as required) */
	hStmt->pszQuery = (char *)strdup( szSqlStr );
	if ( NULL == hStmt->pszQuery )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Memory allocation error" );
		return SQL_ERROR;
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}



