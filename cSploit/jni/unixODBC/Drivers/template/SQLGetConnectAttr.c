/**********************************************************************
 * SQLGetConnectAttr
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

SQLRETURN  SQLGetConnectAttr(
                             SQLHDBC             hDrvDbc,
                             SQLINTEGER          Attribute,
                             SQLPOINTER          Value,
                             SQLINTEGER          BufferLength,
                             SQLINTEGER         *StringLength
                            )
{
	HDRVDBC	hDbc	= (HDRVDBC)hDrvDbc;

	/* SANITY CHECKS */
    if( NULL == hDbc )
        return SQL_INVALID_HANDLE;

	sprintf((char*) hDbc->szSqlMsg, "hDbc = $%08lX", (long)hDbc );
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hDbc->szSqlMsg );

    /************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );


    return SQL_ERROR;
}


