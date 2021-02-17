/*
 *  This file contains the ODBCINSTGetProperties function required by
 *  unixODBC (http://www.unixodbc.org) to define tds DSNs.
 *
 *  $Log: oraodbcS.c,v $
 *  Revision 1.4  2009/02/18 17:59:08  lurcher
 *  Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 *  Revision 1.3  2003/07/21 17:10:09  lurcher
 *
 *  Start new version and add missing comma to oracle setup
 *
 *  Revision 1.2  2002/12/20 11:36:46  lurcher
 *
 *  Update DMEnvAttr code to allow setting in the odbcinst.ini entry
 *
 *  Revision 1.1.1.1  2001/10/17 16:40:01  lurcher
 *
 *  First upload to SourceForge
 *
 *  Revision 1.1.1.1  2000/09/04 16:42:52  nick
 *  Imported Sources
 *
 *  Revision 1.1  2000/08/11 12:45:38  ngorham
 *
 *  Add Oracle setup
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

static char *help_strings[] = 
{
    "Name of the server to connect to.",
    "User name to connect with.",
    "Password of user.",
    "Path name of the Oracle version to use.",
    "Path name of the TNS files."
};

int ODBCINSTGetProperties(
    HODBCINSTPROPERTY hLastProperty)
{
    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "DB", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[0]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[0]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "USER", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[1]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[1]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "PASSWORD", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[2]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[2]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_FILENAME;
    strncpy( hLastProperty->szName, "ORACLE_HOME", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[3]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[3]);
    
    /* Idea for the future:
     * make the nPromptType an ODBCINST_PROMPTTYPE_COMBOBOX and
     * present the user with aPromptData containing
     * the current value of the ORACLE_HOME environment variable
     * same for TNS_ADMIN below
     */
    
    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_FILENAME;
    strncpy( hLastProperty->szName, "TNS_ADMIN", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[4]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[4]);

    return 1;
}

