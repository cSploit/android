/**********************************************************************
 * SQLGetDiagRec
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

SQLRETURN  SQLGetDiagRec(    SQLSMALLINT         HandleType,
                             SQLHANDLE          Handle,
                             SQLSMALLINT         RecordNumber,
                             SQLCHAR            *Sqlstate,
                             SQLINTEGER         *NativeError,
                             SQLCHAR            *MessageText,
                             SQLSMALLINT         BufferLength,
                             SQLSMALLINT        *StringLength
                        )
{

    return SQL_ERROR;
}


