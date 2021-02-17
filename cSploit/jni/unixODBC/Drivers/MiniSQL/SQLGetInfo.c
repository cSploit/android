/**********************************************************************
 * SQLGetInfo
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

SQLRETURN SQLGetInfo(
        SQLHDBC         hDbc,
        SQLUSMALLINT    nInfoType,
        SQLPOINTER      pInfoValue,
        SQLSMALLINT     nInfoValueMax,
        SQLSMALLINT     *pnLength)
{
    SQLRETURN rc=SQL_ERROR;

    switch (nInfoType)
    {
        case SQL_DBMS_NAME:
            if (nInfoValueMax < 5)
                break;
            strcpy( pInfoValue, "mSQL");
            *pnLength = 4;
            rc = SQL_SUCCESS;
            break;
        case SQL_DBMS_VER:
        {
            SQLPOINTER ver=(SQLPOINTER)msqlGetServerInfo();
            SQLSMALLINT len=strlen(ver);
            if (nInfoValueMax < len+1)
                break;
            strcpy( pInfoValue, ver);
            *pnLength = len;
            rc = SQL_SUCCESS;
            break;
        }
        case SQL_TXN_CAPABLE:
        {
            SQLSMALLINT supp=0;
            if (nInfoValueMax < sizeof(SQLSMALLINT))
                break;
            pInfoValue = (SQLPOINTER)&supp;
            *pnLength = sizeof(SQLSMALLINT);
            rc = SQL_SUCCESS;
            break;
        }
    }
    return rc;
}


