/**********************************************************************
 * SQLGetDiagRec
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

SQLRETURN  SQLGetDiagRec_(   SQLSMALLINT    nHandleType,
                             SQLHANDLE      hHandle,
                             SQLSMALLINT    nRecordNumber,
                             SQLCHAR *      pszState,
                             SQLINTEGER *   pnNativeError,
                             SQLCHAR *      pszMessageText,
                             SQLSMALLINT    nBufferLength,
                             SQLSMALLINT *  pnStringLength
                         )
{
    HLOG    hLog = NULL;
    HLOGMSG hMsg = NULL;

    /* sanity checks */
    if ( !hHandle )
        return SQL_INVALID_HANDLE;

    /* clear return values */
    if ( pszState )
        strcpy((char*) pszState, "-----" );

    if ( pnNativeError )
        *pnNativeError = 0;

    if ( pszMessageText )
        memset( pszMessageText, 0, nBufferLength );

    if ( pnStringLength )
        *pnStringLength = 0;

    /* get hLog */
    switch ( nHandleType )
    {
        case SQL_HANDLE_ENV:
            hLog = ((HDRVENV)hHandle)->hLog;
            break;
        case SQL_HANDLE_DBC:
            hLog = ((HDRVDBC)hHandle)->hLog;
            break;
        case SQL_HANDLE_STMT:
            hLog = ((HDRVSTMT)hHandle)->hLog;
            break;
        case SQL_HANDLE_DESC:
        default:
            return SQL_ERROR;
    }

    /* get message */
    if ( logPeekMsg( hLog, 1, &hMsg ) != LOG_SUCCESS )
        return SQL_NO_DATA;

    if ( pnNativeError )
        *pnNativeError = hMsg->nCode;

    if ( pszMessageText )
        strncpy( (char*)pszMessageText, hMsg->pszMessage, nBufferLength-1 );

    if ( pnStringLength )
        *pnStringLength = strlen( (char*)hMsg->pszMessage );

    return SQL_SUCCESS;
}

SQLRETURN  SQLGetDiagRec(    SQLSMALLINT    nHandleType,
                             SQLHANDLE      hHandle,
                             SQLSMALLINT    nRecordNumber,
                             SQLCHAR *      pszState,
                             SQLINTEGER *   pnNativeError,
                             SQLCHAR *      pszMessageText,
                             SQLSMALLINT    nBufferLength,
                             SQLSMALLINT *  pnStringLength
                        )
{
    return SQLGetDiagRec_( nHandleType, hHandle, nRecordNumber, pszState, pnNativeError, pszMessageText, nBufferLength, pnStringLength );
}

