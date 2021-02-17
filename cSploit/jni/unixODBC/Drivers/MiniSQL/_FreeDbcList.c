/**************************************************
 * _FreeDbcList
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

SQLRETURN _FreeDbcList( SQLHENV hDrvEnv )
{
	HDRVENV hEnv	= (HDRVENV)hDrvEnv;

	if ( hEnv == SQL_NULL_HENV )
		return SQL_SUCCESS;

	while ( _FreeDbc( hEnv->hFirstDbc ) == SQL_SUCCESS )
	{
	}

	return SQL_SUCCESS;
}



