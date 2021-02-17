/**********************************************************************
 * SQLDriverConnect
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

SQLRETURN SQLDriverConnect(		SQLHDBC            hDrvDbc,
								SQLHWND            hWnd,
								SQLCHAR            *szConnStrIn,
								SQLSMALLINT        nConnStrIn,
								SQLCHAR            *szConnStrOut,
								SQLSMALLINT        cbConnStrOutMax,
								SQLSMALLINT        *pnConnStrOut,
								SQLUSMALLINT       nDriverCompletion
                                )
{
	HDRVDBC	hDbc	= (HDRVDBC)hDrvDbc;

	/* SANITY CHECKS */
    if( NULL == hDbc )
        return SQL_INVALID_HANDLE;

	sprintf( hDbc->szSqlMsg, "hDbc = $%08lX", hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );


    if( hDbc->bConnected == 1 )
    {
        logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Already connected" );
        return SQL_ERROR;
    }

    /*************************
     * 1. parse nConnStrIn for connection options. Format is;
     *      Property=Value;...
     * 2. we may not have all options so handle as per DM request
     * 3. fill as required szConnStrOut
     *************************/
    switch( nDriverCompletion )
    {
        case SQL_DRIVER_PROMPT:
        case SQL_DRIVER_COMPLETE:
        case SQL_DRIVER_COMPLETE_REQUIRED:
        case SQL_DRIVER_NOPROMPT:
        default:
			sprintf( hDbc->szSqlMsg, "Invalid nDriverCompletion=%d", nDriverCompletion );
			logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, hDbc->szSqlMsg );
            break;
    }

    /*************************
     * 4. try to connect
     * 5. set gathered options (ie USE Database or whatever)
     * 6. set connection state
     *      hDbc->bConnected = TRUE;
     *      hDbc->ciActive = 0;
     *************************/

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported." );

    return SQL_ERROR;
}


