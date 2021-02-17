/**************************************************
 * This code was created by Holger Schurig @ mn-logistik.de.
 * Released under LGPL 08.MAR.2001
 *
 * Additional MaxDB ODBC properties added by
 * Martin Kittel <debian @ martin-kittel.de>, 08.05.2005
 * -----------------------------------------------
 **************************************************/
#include <config.h>
#include <odbcinstext.h>

static const char *aHost[] =
{
    "localhost",
    NULL
};

static const char *aSqlModes[] =
{
    "INTERNAL",
    "ORACLE",
    "ANSI",
    "DB2",
    NULL
};

static const char *aIsoLevel[] =
{
    "Uncommitted",
    "Committed",
    "Repeatable",
    "Serializable",
    NULL
};

int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{
    hLastProperty->pNext       = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty              = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "ServerNode", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "localhost", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext       = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty              = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "ServerDB", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext       = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty              = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData = malloc( sizeof( aSqlModes ) );
    memcpy( hLastProperty->aPromptData, aSqlModes, sizeof( aSqlModes ) );
    strncpy( hLastProperty->szName, "SQLMode", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "INTERNAL", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext       = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty              = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData = malloc( sizeof( aIsoLevel ) );
    memcpy( hLastProperty->aPromptData, aIsoLevel, sizeof( aIsoLevel ) );
    strncpy( hLastProperty->szName, "IsolationLevel", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "Committed", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext       = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty              = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_FILENAME;
    strncpy( hLastProperty->szName, "TraceFileName", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    return 1;
}
