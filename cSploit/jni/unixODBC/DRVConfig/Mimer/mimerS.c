/*
 *  This file contains the ODBCINSTGetProperties function.
 *
 *  It is required by unixODBC (http://www.unixodbc.org) to define DSNs.
 *
 *  This version is the setup for Mimer SQL (http://developer.mimer.se).
 *
 *  Revision 1.1  2004/01/21 per.bengtsson@mimer.se
 *
 */

#include <config.h>
#include <odbcinstext.h>

static const char *vHost[] =
{
	"localhost",
	NULL
};

static const char *vPort[] =
{
	"1360",
	NULL
};

static const char *vUser[] =
{
	"SYSADM",
	NULL
};

static const char *vDescr[] =
{
	"Mimer SQL",
	NULL
};

static const char *vYesNo[] =
{
        "Yes",
        "No",
        NULL
};

int ODBCINSTGetProperties(HODBCINSTPROPERTY hLastProperty)
{
/*
 *   Database name
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy(hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("The name of the Mimer SQL database server");
/*
 *   Host name
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData = malloc(sizeof(vHost));
	memcpy(hLastProperty->aPromptData, vHost, sizeof(vHost));
	strncpy(hLastProperty->szName, "Host", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("Hostname or IP address of computer running the Mimer SQL database server"); 
/*
 *   Port number
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData = malloc(sizeof(vPort));
	memcpy(hLastProperty->aPromptData, vPort, sizeof(vPort));
	strncpy(hLastProperty->szName, "Port", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("Port number (default is 1360)");
/*
 *   User name
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData = malloc(sizeof(vUser));
	memcpy(hLastProperty->aPromptData, vUser, sizeof(vUser));
	strncpy(hLastProperty->szName, "User", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("Database user to connect as (optional)");
/*
 *   Password
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy(hLastProperty->szName, "Password", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("Password for the database user (optional)");
/*
 *   Trace option
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData = malloc(sizeof(vYesNo));
	memcpy(hLastProperty->aPromptData, vYesNo, sizeof(vYesNo));
	strncpy(hLastProperty->szName, "Trace", INI_MAX_PROPERTY_NAME);
	strcpy(hLastProperty->szValue, "No");
	hLastProperty->pszHelp = strdup("ODBC trace feature (optional)");
/*
 *   Trace file name
 */
	hLastProperty->pNext = (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	memset(hLastProperty, 0, sizeof(ODBCINSTPROPERTY));
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_FILENAME;
	strncpy(hLastProperty->szName, "TraceFile", INI_MAX_PROPERTY_NAME);
	strncpy(hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = strdup("ODBC trace file name (used if Trace is enabled)");

	return 1;
}

/**************************************************/
