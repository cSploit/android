/**********************************************************************
 * SQLSetDescRec
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

SQLRETURN  SQLSetDescRec(    SQLHDESC            hDescriptorHandle,
                             SQLSMALLINT         nRecordNumber,
                             SQLSMALLINT         nType,
                             SQLSMALLINT         nSubType,
                             SQLINTEGER          nLength,
                             SQLSMALLINT         nPrecision,
                             SQLSMALLINT         nScale,
                             SQLPOINTER          pData,
                             SQLINTEGER         *pnStringLength,
                             SQLINTEGER 		*pnIndicator
                        )
{

    return SQL_ERROR;
}

