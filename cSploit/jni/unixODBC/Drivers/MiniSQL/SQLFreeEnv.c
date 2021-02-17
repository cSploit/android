/**********************************************************************
 * SQLFreeEnv
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

SQLRETURN SQLFreeEnv( SQLHENV hDrvEnv )
{
	return sqlFreeEnv( hDrvEnv );
}


