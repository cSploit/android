/**********************************************************************
 * sqlFreeStmt
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

SQLRETURN sqlFreeStmt(	SQLHSTMT        hDrvStmt,
						SQLUSMALLINT    nOption )
{
    HDRVSTMT hStmt	= (HDRVSTMT)hDrvStmt;

	/* SANITY CHECKS */
    if( hStmt == SQL_NULL_HSTMT )
        return SQL_INVALID_HANDLE;

	sprintf( hStmt->szSqlMsg, "hStmt = $%08lX", hStmt );
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );

    /*********
     * RESET PARAMS
     *********/
    switch( nOption )
    {
	case SQL_CLOSE:
		break;

    case SQL_DROP:
        return _FreeStmt( hStmt );

	case SQL_UNBIND:
		break;

    case SQL_RESET_PARAMS:
        break;

    default:
		sprintf( hStmt->szSqlMsg, "SQL_ERROR Invalid nOption=%d", nOption );
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hStmt->szSqlMsg );
        return SQL_ERROR;
    }

    return SQL_SUCCESS;
}


