/**********************************************************************
 * SQLGetDescRec
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

SQLRETURN  SQLGetDescRec(    SQLHDESC            DescriptorHandle,
                             SQLSMALLINT         RecordNumber,
                             SQLCHAR            *Name,
                             SQLSMALLINT         BufferLength,
                             SQLSMALLINT        *StringLength,
                             SQLSMALLINT        *Type,
                             SQLSMALLINT        *SubType,
                             SQLLEN             *Length,
                             SQLSMALLINT        *Precision,
                             SQLSMALLINT        *Scale,
                             SQLSMALLINT        *Nullable
                        )
{

    return SQL_ERROR;
}


