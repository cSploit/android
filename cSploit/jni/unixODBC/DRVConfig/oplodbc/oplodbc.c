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

static const char *szHelpDatabase = "Databases on Server. I can try to present a list if Host, Login ID and Password are given.";
static const char *szHelpPassword = "Your Password will be used to gain additional information from the DBMS and will not be saved anywhere.";

/**********************************************
 * STATIC LOOKUP VALUES
 **********************************************/
static const char *aHost[] =
{
	"localhost",
	NULL
};

static const char *aServerType[] =
{
	"DB2",
	"Informix 5",
	"Informix 6",
	"Informix 7",
	"Ingres 6",
	"Odbc",
	"OpenIngres 1",
	"OpenIngres 2",
	"Oracle 6",
	"Oracle 7",
	"Oracle 8",
	"Postgres 95",
	"Progress 63C",
	"Progress 63E",
	"Progress 63F",
	"Progress 72D",
	"Progress 73A",
	"Progress 73C",
	"Progress 73D",
	"Progress 73E",
	"Progress 81A",
	"Progress 82A",
	"Progress 82C",
	"Progress 83A",
	"Progress 90A",
	"Proxy",
	"Solid",
	"Sybase 4",
	"Sybase 10",
	"Sybase 11",
	"SQLServer",
	"Unify",
	"Velocis",
	"Virtuoso",
	NULL
};

static const char *aProtocol[] =
{
	"TCP/IP",
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
	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "ServerOptions", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "Options", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
    hLastProperty->pszHelp				= malloc( strlen(szHelpDatabase)+1 );
    strcpy( hLastProperty->pszHelp, szHelpDatabase );
	strncpy( hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData			= malloc( sizeof( aHost ) );
	memcpy( hLastProperty->aPromptData, aHost, sizeof( aHost ) );
	strncpy( hLastProperty->szName, "Host", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "UserName", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT_PASSWORD;
    hLastProperty->pszHelp				= malloc( strlen(szHelpPassword)+1 );
    strcpy( hLastProperty->pszHelp, szHelpPassword );
	strncpy( hLastProperty->szName, "Password", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aServerType) );
	memcpy( hLastProperty->aPromptData, aServerType, sizeof(aServerType) );
	strncpy( hLastProperty->szName, "ServerType", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aProtocol) );
	memcpy( hLastProperty->aPromptData, aProtocol, sizeof(aProtocol) );
	strncpy( hLastProperty->szName, "Protocol", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "LastUser", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aYesNo) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
	strncpy( hLastProperty->szName, "ReadOnly", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
    hLastProperty->aPromptData			= malloc( sizeof(aTrueFalse) );
	memcpy( hLastProperty->aPromptData, aTrueFalse, sizeof(aTrueFalse) );
	strncpy( hLastProperty->szName, "NoLoginBox", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "FetchBufferSize", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	return 1;
}

