/**********************************************************************
 * SQLFreeConnect
 *
 * Do not try to Free Dbc if there are Stmts... return an error. Let the
 * Driver Manager do a recursive clean up if its wants.
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

SQLRETURN SQLFreeConnect( SQLHDBC hDrvDbc )
{
	return sqlFreeConnect( hDrvDbc );
}


