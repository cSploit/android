/**************************************************
 *
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 10.APR.01
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
static const char *aColumnSeperators[] =
{
	"|",
    ",",
	NULL
};

static const char *aYesNo[] =
{
	"Yes",
	"No",
	NULL
};


int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{
	hLastProperty->pNext 				= (HODBCINSTPROPERTY)calloc( 1, sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "Directory", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
	hLastProperty->pszHelp				= strdup( "Directory where table files can/will be found.\nLeave blank for default (~/.odbctxt)." );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)calloc( 1, sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData          = malloc( sizeof( aYesNo ) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof( aYesNo ) ); 
	strncpy( hLastProperty->szName, "ReadOnly", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );
	hLastProperty->pszHelp				= strdup( "Set this to Yes if you do not want anyone changing the data." );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)calloc( 1, sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData          = malloc( sizeof( aYesNo ) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof( aYesNo ) ); 
	strncpy( hLastProperty->szName, "CaseSensitive", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "Yes", INI_MAX_PROPERTY_VALUE );
	hLastProperty->pszHelp				= strdup( "Yes if data is compared as case-sensitive." );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)calloc( 1, sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData          = malloc( sizeof( aYesNo ) );
	memcpy( hLastProperty->aPromptData, aYesNo, sizeof( aYesNo ) ); 
	strncpy( hLastProperty->szName, "Catalog", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "No", INI_MAX_PROPERTY_VALUE );
	hLastProperty->pszHelp				= strdup( "Yes if you want to use a special file to describe all tables/columns.\n\nNo if column names on the first row are enough." );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)calloc( 1, sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData          = malloc( sizeof( aColumnSeperators ) );
	memcpy( hLastProperty->aPromptData, aColumnSeperators, sizeof( aColumnSeperators ) ); 
	strncpy( hLastProperty->szName, "ColumnSeperator", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "|", INI_MAX_PROPERTY_VALUE );
	hLastProperty->pszHelp				= strdup( "Column seperator character used in table files.\nCAN NOT EXIST IN COLUMN VALUES." );

	return 1;
}


