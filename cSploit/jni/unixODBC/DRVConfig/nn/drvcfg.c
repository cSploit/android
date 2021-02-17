/**************************************************
 *
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 13.MAY.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
#include <config.h>
#include <odbcinstext.h>

static const char *aServer[] =
{
	"localhost",				/* put some public news servers here */
	NULL
};


int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{
	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData			= malloc( sizeof( aServer ) );
	memcpy( hLastProperty->aPromptData, aServer, sizeof( aServer ) );
	strncpy( hLastProperty->szName, "Server", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	return 1;
}

