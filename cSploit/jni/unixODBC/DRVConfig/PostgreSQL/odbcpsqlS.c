/**************************************************
 *
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 31.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
#include <config.h>
#include <odbcinstext.h>

/**********************************************
 * HELP
 **********************************************/

static const char *szHelpPassword = "Your Password will be used to gain additional information from the DBMS and will not be saved anywhere.";

/**********************************************
 * STATIC LOOKUP VALUES
 **********************************************/
static const char *aServer[] =
{
	"localhost",
	NULL
};

static const char *aPort[] =
{
	"5432",
	NULL
};

static const char *aProtocol[] =
{
	"6.4",
	"6.3",
	"6.2",
	NULL
};

static const char *aYesNo[] =
{
	"Yes",
	"No",
	NULL
};

static const char *aTrueFalse[] =
{
	"True",
	"False",
	NULL
};

int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{ 
    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData          = malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, "Trace", INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "No" );

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_FILENAME;
    strncpy( hLastProperty->szName, "TraceFile", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData			= malloc( sizeof( aServer ) );
	memcpy( hLastProperty->aPromptData, aServer, sizeof( aServer ) );
	strncpy( hLastProperty->szName, "Servername", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "localhost", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "Username", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT_PASSWORD;
    hLastProperty->pszHelp				= (char *)strdup( szHelpPassword );
	strncpy( hLastProperty->szName, "Password", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aPort) );
	memcpy( hLastProperty->aPromptData, aPort, sizeof(aPort) );
	strncpy( hLastProperty->szName, "Port", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "5432", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aProtocol) );
	memcpy( hLastProperty->aPromptData, aProtocol, sizeof(aProtocol) );
	strncpy( hLastProperty->szName, "Protocol", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "6.4", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "ReadOnly", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "RowVersioning", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "ShowSystemTables", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "ShowOidColumn", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "FakeOidIndex", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "ConnSettings", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	return 1;
}

