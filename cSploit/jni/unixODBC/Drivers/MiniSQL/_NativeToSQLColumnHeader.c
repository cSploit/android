/**************************************************
 * _NativeToSQLColumnHeader
 *
 * We want to make the driver code as general as possible. This isolates
 * the translation from the native client to the SQL format for column
 * header information.
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 31.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN _NativeToSQLColumnHeader( COLUMNHDR *pColumnHeader, void *pNativeColumnHeader )
{
	m_field		*pField	= (m_field*)pNativeColumnHeader;
	char		szBuffer[501];

	if ( !pNativeColumnHeader )
		return SQL_ERROR;
	if ( !pColumnHeader )
		return SQL_ERROR;

    /* IS AUTO INCREMENT COL?					*/
	pColumnHeader->bSQL_DESC_AUTO_UNIQUE_VALUE = 0;

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_BASE_COLUMN_NAME = (char*)strdup( pField->name );	

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_BASE_TABLE_NAME = pField->table  ? (char*)strdup( pField->table ) : (char*)strdup( "" );

    /* IS CASE SENSITIVE COLUMN?				*/
	pColumnHeader->bSQL_DESC_CASE_SENSITIVE = 0;

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_CATALOG_NAME = (char*)strdup( "" );

    /* ie SQL_CHAR, SQL_TYPE_TIME...			*/
	pColumnHeader->nSQL_DESC_CONCISE_TYPE = _NativeToSQLType( pField );

    /* max digits required to display			*/
	pColumnHeader->nSQL_DESC_DISPLAY_SIZE = pField->length;

    /* has data source specific precision?		*/
	pColumnHeader->bSQL_DESC_FIXED_PREC_SCALE = 0;

    /* display label, col name or empty string	*/
	pColumnHeader->pszSQL_DESC_LABEL = (char*)strdup( pField->name );	

    /* strlen or bin size						*/
	pColumnHeader->nSQL_DESC_LENGTH = _NativeTypeLength( pField );

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_LITERAL_PREFIX = (char*)strdup( "" );

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_LITERAL_SUFFIX = (char*)strdup( "" );			

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_LOCAL_TYPE_NAME = (char*)strdup( "" );

    /* col alias, col name or empty string		*/
	pColumnHeader->pszSQL_DESC_NAME = (char*)strdup( pField->name );	

    /* SQL_NULLABLE, _NO_NULLS or _UNKNOWN		*/
	pColumnHeader->nSQL_DESC_NULLABLE = IS_NOT_NULL( pField->flags ) ? SQL_NO_NULLS : SQL_NULLABLE;

    /* 2, 10, or if N/A... 0 					*/
	pColumnHeader->nSQL_DESC_NUM_PREC_RADIX = 0;

    /* max size									*/
	pColumnHeader->nSQL_DESC_OCTET_LENGTH = pField->length;

    /*											*/
	pColumnHeader->nSQL_DESC_PRECISION = _NativeTypePrecision( pField );

    /*											*/
	pColumnHeader->nSQL_DESC_SCALE = 4;

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_SCHEMA_NAME = (char*)strdup( "" );

    /* can be in a filter ie SQL_PRED_NONE...	*/
	pColumnHeader->nSQL_DESC_SEARCHABLE = SQL_PRED_SEARCHABLE;

    /* empty string if N/A						*/
	pColumnHeader->pszSQL_DESC_TABLE_NAME = pField->table ? (char*)strdup( pField->table ) : (char*)strdup( "" );

    /* SQL data type ie SQL_CHAR, SQL_INTEGER..	*/
	pColumnHeader->nSQL_DESC_TYPE = _NativeToSQLType( pField );

    /* DBMS data type ie VARCHAR, MONEY...		*/
	pColumnHeader->pszSQL_DESC_TYPE_NAME = (char*)strdup( _NativeTypeDesc( szBuffer, pField->type ) );

    /* qualifier for SQL_DESC_NAME ie SQL_NAMED	*/
	pColumnHeader->nSQL_DESC_UNNAMED = SQL_NAMED;

    /* if signed FALSE else TRUE				*/
	pColumnHeader->bSQL_DESC_UNSIGNED = 0;

    /* ie SQL_ATTR_READONLY, SQL_ATTR_WRITE...	*/
	pColumnHeader->nSQL_DESC_UPDATABLE = SQL_ATTR_READONLY;

	return SQL_SUCCESS;
}




