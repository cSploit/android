/**********************************************************************
 * _FreeStmtList
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

SQLRETURN _FreeStmtList( SQLHDBC hDrvDbc )
{
	HDRVDBC hDbc	= (HDRVDBC)hDrvDbc;

	if ( hDbc == SQL_NULL_HDBC )
		return SQL_SUCCESS;

	if ( hDbc->hFirstStmt == NULL )
		return SQL_SUCCESS;
	
	while ( _FreeStmt( hDbc->hFirstStmt ) == SQL_SUCCESS )
	{
	}

	return SQL_SUCCESS;
}



