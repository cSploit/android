/**************************************************
 *
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 18.FEB.99
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

/**********************************************
 * STATIC LOOKUP VALUES
 **********************************************/


int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{
	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_FILENAME;
	strncpy( hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	return 1;
}

