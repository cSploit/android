/**********************************************************************
 * SQLGetEnvAttr
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

SQLRETURN  SQLGetEnvAttr(    SQLHENV             EnvironmentHandle,
                             SQLINTEGER          Attribute,
                             SQLPOINTER          Value,
                             SQLINTEGER          BufferLength,
                             SQLINTEGER         *StringLength
                        )
{

    return SQL_ERROR;
}


