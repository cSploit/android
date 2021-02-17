/**********************************************************************
 * _FreeDbcList
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



