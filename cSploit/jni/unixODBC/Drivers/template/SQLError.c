/**********************************************************************
 * SQLError (deprecated see SQLGetDiagRec)
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

/*! 
 * \brief   Get oldest error for the given handle.
 *
 *          This is deprecated - use SQLGetDiagRec instead. This is mapped
 *          to SQLGetDiagRec. The main difference between this and 
 *          SQLGetDiagRec is that this call will delete the error message to
 *          allow multiple calls here to work their way through all of the
 *          errors even with the lack of an ability to pass a specific message
 *          number to be returned.
 * 
 * \param   hDrvEnv
 * \param   hDrvDbc
 * \param   hDrvStmt
 * \param   szSqlState
 * \param   pfNativeError
 * \param   szErrorMsg
 * \param   nErrorMsgMax
 * \param   pcbErrorMsg
 * 
 * \return  SQLRETURN
 *
 * \sa      SQLGetDiagRec
 */
SQLRETURN SQLError( SQLHENV     hDrvEnv,
					SQLHDBC     hDrvDbc,
					SQLHSTMT    hDrvStmt,
					SQLCHAR   	*szSqlState,
					SQLINTEGER  *pfNativeError,
					SQLCHAR   	*szErrorMsg,
					SQLSMALLINT nErrorMsgMax,
					SQLSMALLINT	*pcbErrorMsg )
{
    SQLSMALLINT nHandleType;
    SQLHANDLE   hHandle;
    SQLRETURN   nReturn;
    HLOG        hLog;

    /* map call to SQLGetDiagRec */
    if ( hDrvEnv )
    {
        nHandleType = SQL_HANDLE_ENV;
        hHandle     = hDrvEnv;
        hLog        = ((HDRVENV)hDrvEnv)->hLog;
    }
    else if ( hDrvDbc )
    {
        nHandleType = SQL_HANDLE_DBC;
        hHandle     = hDrvDbc;
        hLog        = ((HDRVDBC)hDrvDbc)->hLog;
    }
    else if ( hDrvStmt )
    {
        nHandleType = SQL_HANDLE_STMT;
        hHandle     = hDrvStmt;
        hLog        = ((HDRVSTMT)hDrvStmt)->hLog;
    }
    else
        return SQL_INVALID_HANDLE;

    nReturn = SQLGetDiagRec_( nHandleType, hHandle, 1, szSqlState, pfNativeError, szErrorMsg, nErrorMsgMax, pcbErrorMsg ); 

    /* unlike SQLGetDiagRec - we delete the message returned */
    if ( SQL_SUCCEEDED( nReturn ) )
        logPopMsg( hLog );

    return nReturn;
}


