/**********************************************************************
 * sqlFreeEnv
 *
 * Do not try to Free Env if there are Dbcs... return an error. Let the
 * Driver Manager do a recursive clean up if it wants.
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

SQLRETURN sqlFreeEnv( SQLHENV hDrvEnv )
{
	HDRVENV hEnv	= (HDRVENV)hDrvEnv;

	/* SANITY CHECKS */
    if( hEnv == SQL_NULL_HENV )
        return SQL_INVALID_HANDLE;

	sprintf( hEnv->szSqlMsg, "hEnv = $%08lX", hEnv );
    logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hEnv->szSqlMsg );

	if ( hEnv->hFirstDbc != NULL )
	{
		logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR There are allocated Connections" );
		return SQL_ERROR;
	}

/************
 * 	!!! ADD CODE TO FREE DRIVER SPECIFIC MEMORY (hidden in hEnvExtras) HERE !!!
 ************/
    free( hEnv->hEnvExtras );
	logPushMsg( hEnv->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	logClose( hEnv->hLog );
    free( hEnv );

	return SQL_SUCCESS;
}


