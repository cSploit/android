/*
 *  This file contains the ODBCINSTGetProperties function required by
 *  unixODBC (http://www.unixodbc.org) to define tds DSNs.
 *
 * This may be used for the FreeTDS driver or some other driver so
 * be careful about removing any properties. - PAH
 *
 *  $Log: tdsS.c,v $
 *  Revision 1.4  2009/02/18 17:59:08  lurcher
 *  Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 *  Revision 1.3  2003/11/13 15:12:53  lurcher
 *
 *  small change to ODBCConfig to have the password field in the driver
 *  properties hide the password
 *  Make both # and ; comments in ini files
 *
 *  Revision 1.2  2002/02/22 17:27:20  peteralexharvey
 *  -
 *
 *  Revision 1.1.1.1  2001/10/17 16:40:01  lurcher
 *
 *  First upload to SourceForge
 *
 *  Revision 1.1.1.1  2000/09/04 16:42:52  nick
 *  Imported Sources
 *
 *  Revision 1.1  2000/07/09 22:17:52  ngorham
 *
 *  Added tds setup lib
 *
 */

#include <config.h>
#include <odbcinstext.h>

static const char *aYesNo[] =
{
	"Yes",
	"No",
	NULL
};

int ODBCINSTGetProperties(
    HODBCINSTPROPERTY hLastProperty)
{
    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Servername", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = (char*)strdup( "Name of server to connect to. This should\nmatch the name used in the interfaces\nor freetds.conf file." );

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = (char*)strdup( "Default database" );

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "UID", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = (char*)strdup( "User ID" );

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT_PASSWORD;
    strncpy( hLastProperty->szName, "PWD", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = (char*)strdup( "Password" );

    hLastProperty->pNext = 
    (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Port", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "4100", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = (char*)strdup( "Port to connect to (some drivers do not use)" );

    return 1;
}

