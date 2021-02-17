/**********************************************************************
 * SQLAllocHandle
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

SQLRETURN  SQLAllocHandle(	SQLSMALLINT	nHandleType,
							SQLHANDLE	nInputHandle,
							SQLHANDLE	*pnOutputHandle )
{
	switch ( nHandleType )
	{
	case SQL_HANDLE_ENV:
		return _AllocEnv( (SQLHENV *)pnOutputHandle );
	case SQL_HANDLE_DBC:
		return _AllocConnect( (SQLHENV)nInputHandle, (SQLHDBC *)pnOutputHandle );
	case SQL_HANDLE_STMT:
		return _AllocStmt( (SQLHDBC)nInputHandle, (SQLHSTMT *)pnOutputHandle );
	case SQL_HANDLE_DESC:
		break;
	default:
		return SQL_ERROR;
		break;
	}

	return SQL_ERROR;
}


