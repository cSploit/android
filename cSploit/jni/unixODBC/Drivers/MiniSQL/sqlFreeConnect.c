/**********************************************************************
 * sqlFreeConnect
 *
 * Do not try to Free Dbc if there are Stmts... return an error. Let the
 * Driver Manager do a recursive clean up if its wants.
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

SQLRETURN sqlFreeConnect( SQLHDBC hDrvDbc )
{
	HDRVDBC	hDbc	= (HDRVDBC)hDrvDbc;
	int		nReturn;

	/* SANITY CHECKS */
    if( NULL == hDbc )
        return SQL_INVALID_HANDLE;

	sprintf( hDbc->szSqlMsg, "hDbc = $%08lX", hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );

    if( hDbc->hDbcExtras->hServer > -1 )
    {
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Connection is active" );
		return SQL_ERROR;
    }

	if ( hDbc->hFirstStmt != NULL )
	{
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Connection has allocated statements" );
		return SQL_ERROR;
	}

	nReturn = _FreeDbc( hDbc );

	return nReturn;

}


