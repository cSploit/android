/**********************************************************************
 * SQLFreeHandle
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

SQLRETURN  SQLFreeHandle(    SQLSMALLINT    nHandleType,
                             SQLHANDLE      nHandle
                        )
{
    switch( nHandleType )
    {
    case SQL_HANDLE_ENV:
        return _FreeEnv( (SQLHENV)nHandle );

    case SQL_HANDLE_DBC:
        return _FreeConnect( (SQLHDBC)nHandle );

    case SQL_HANDLE_STMT:
        return _FreeStmt( (SQLHSTMT)nHandle );

    case SQL_HANDLE_DESC:
        break;

	default:
		return SQL_ERROR;
    }

    return SQL_ERROR;
}

