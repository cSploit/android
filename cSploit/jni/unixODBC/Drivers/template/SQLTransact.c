/********************************************************************
 * SQLTransact (deprecated)
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

SQLRETURN SQLTransact(	SQLHENV hDrvEnv,
						SQLHDBC hDrvDbc,
						UWORD   nType)
{
	HDRVENV hEnv	= (HDRVENV)hDrvEnv;
	HDRVDBC hDbc	= (HDRVDBC)hDrvDbc;

	/* SANITY CHECKS */
	if ( hDbc == SQL_NULL_HDBC )
		return SQL_INVALID_HANDLE;

	sprintf((char*) hDbc->szSqlMsg, "hDbc = $%08lX", (long)hDbc );
	logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );

	
	switch ( nType )
	{
	case SQL_COMMIT:
		break;
	case SQL_ROLLBACK:
		break;
	default:
		sprintf((char*) hDbc->szSqlMsg, "SQL_ERROR Invalid nType=%d", nType );
		logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );
		return SQL_ERROR;
	}

	logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Function not supported" );
	return SQL_ERROR;
}


