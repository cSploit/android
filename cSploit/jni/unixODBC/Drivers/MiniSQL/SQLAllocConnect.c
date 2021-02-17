/**************************************************
 * SQLAllocConnect
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

SQLRETURN SQLAllocConnect(	SQLHENV    hDrvEnv,
							SQLHDBC    *phDrvDbc )
{
    return _AllocConnect( hDrvEnv, phDrvDbc );
}


