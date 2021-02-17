/*
 *  Copyright (c) 1999 Easysoft Ltd. All rights reserved.
 * 
 *  This file contains the ODBCINSTGetProperties function required by
 *  unixODBC (http://www.unixodbc.org) to define Easysoft ODBC-ODBC Bridge
 *  DSNs.
 *
 *  $Id: esoobS.c,v 1.2 2009/02/18 17:59:08 lurcher Exp $
 *
 *  $Log: esoobS.c,v $
 *  Revision 1.2  2009/02/18 17:59:08  lurcher
 *  Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 *  Revision 1.1.1.1  2001/10/17 16:40:01  lurcher
 *
 *  First upload to SourceForge
 *
 *  Revision 1.1.1.1  2000/09/04 16:42:52  nick
 *  Imported Sources
 *
 *  Revision 1.3  2000/06/19 15:54:53  ngorham
 *
 *  Added DisguiseWide attribute to esoob.
 *
 *  Revision 1.2  2001/05/27 10:35:18  ngorham
 *
 *  Added MetaDataBlockFetch connection attribute to esoob driver.
 *
 *  Revision 1.1  2000/02/20 10:21:08  ngorham
 *
 *  Setup files for Easysoft ODBC-ODBC Bridge
 *
 *  Revision 1.4  2000/02/10 17:53:50  martin
 *  Change protocol to transport.
 *
 *  Revision 1.3  1999/09/06 10:58:55  martin
 *  Add description and copyright.
 *
 *  Revision 1.2  1999/09/06 10:49:55  martin
 *  Add BlockFetchSize, MetaData_ID_Identifier and Unquote_Catalog_Fns.
 *
 *  Revision 1.1  1999/09/06 09:44:37  martin
 *  Initial revision
 *
 */

#include <config.h>
#include <odbcinstext.h>

char *help_strings[] = 
{
    "Name of the server to connect to.",
    "Name of the network transport to use.",
    "Number of the port the server is listening on.",
    "Name of the remote DSN to connect to.",
    "Name of the user to log on to server box as.",
    "Password of the user to log on to server box as.",
    "Name of the remote database user.",
    "Password of the remote datbase user.",
    "Number of rows to bind in Block-Fetch-Mode.",
    "Remove quotes on catalog functions (Applix's AXData).",
    "Tell driver to treat strings as literal and treat wildcard "
    "characters as literals.",
    "Enable (1) or Disable (0)  OOB's metadata blockfetchmode",
    "Enable (1) or Disable (0)  disguise wide characters mode" 
};

int ODBCINSTGetProperties(
    HODBCINSTPROPERTY hLastProperty)
{
    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Server", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[0]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[0]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Transport", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "TCP/IP", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[1]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[1]);
    
    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Port", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "8888", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[2]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[2]);

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "TargetDSN", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[3]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[3]);

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "LogonUser", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[4]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[4]);

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "LogonAuth", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[5]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[5]);

    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "TargetUser", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[6]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[6]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "TargetAuth", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[7]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[7]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "BlockFetchSize", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "0", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[8]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[8]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "Unquote_Catalog_Fns", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "0", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[9]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[9]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "MetaData_ID_Identifiers", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "0", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[10]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[10]);

    hLastProperty->pNext =
        (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "MetaDataBlockFetch", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "1", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[11]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[11]);


    hLastProperty->pNext = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY));
    hLastProperty = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, "DisguiseWide", INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "0", INI_MAX_PROPERTY_VALUE );
    hLastProperty->pszHelp = malloc(strlen(help_strings[12]) + 1);
    strcpy(hLastProperty->pszHelp, help_strings[12]);

    return 1;
}

