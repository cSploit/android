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

/**********************************************
 * STATIC LOOKUP VALUES
 **********************************************/
static const char *aHost[] =
{
    "localhost",
    NULL
};

static const char *aDatabase[] =
{
    "test",
    "mysql",
    NULL
};

int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{
    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData          = malloc( sizeof( aHost ) );
    memcpy( hLastProperty->aPromptData, aHost, sizeof( aHost ) );
    strncpy( hLastProperty->szName, "Server", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "Host name or IP address of the machine running the MySQL server." ); 

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->aPromptData          = malloc( sizeof( aDatabase ) );
    memcpy( hLastProperty->aPromptData, aDatabase, sizeof( aDatabase ) ); 
    strncpy( hLastProperty->szName, "Database", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "test", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "The database you want to connect to.\nYou can use test or mysql to test this DSN." );

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Port", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "Port number. Leave blank to accept the default." );

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Socket", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "Socket number. Leave blank to accept the default." );

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Option", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "\
Add the desired option values and enter resulting number here...\n\n\
     1 The client can't handle that MyODBC returns the real width of a column.\n \
     2 The client can't handle that MySQL returns the true value of affected rows\n \
     4 Make a debug log. \
     8 Don't set any packet limit for results and parameters.\n \
    16 Don't prompt for questions even if driver would like to prompt\n \
    32 Enable or disable the dynamic cursor support. This is not allowed in MyODBC.\n \
    64 Ignore use of database name in 'database.table.column'.\n \
   128 Force use of ODBC manager cursors (experimental).\n \
   256 Disable the use of extended fetch (experimental).\n \
   512 Pad CHAR fields to full column length.\n \
  1024 SQLDescribeCol() will return fully qualified column names\n \
  2048 Use the compressed server/client protocol\n \
  4096 Tell server to ignore space after function name and before '('\n \
  8192 Connect with named pipes to a mysqld server running on NT.\n \
 16384 Change LONGLONG columns to INT columns\n \
 32768 Return 'user' as Table_qualifier and Table_owner from SQLTables\n \
 65536 Read parameters from the client and odbc groups from `my.cnf'\n \
131072 Add some extra safety checks (should not bee needed but...)" );

    hLastProperty->pNext                = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                       = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType          = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Stmt", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp              = strdup( "statement to execute upon connect" );

    return 1;
}

