/**************************************************
 * _FreeStmtList
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 31.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
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



