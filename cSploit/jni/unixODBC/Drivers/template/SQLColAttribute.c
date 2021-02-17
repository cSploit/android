/**********************************************************************
 * SQLColAttribute
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 **********************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN SQLColAttribute(	SQLHSTMT        hDrvStmt,
							SQLUSMALLINT    nCol,
							SQLUSMALLINT    nFieldIdentifier,
							SQLPOINTER      pszValue,
							SQLSMALLINT     nValueLengthMax,
							SQLSMALLINT     *pnValueLength,
							SQLLEN      	*pnValue )
{
    HDRVSTMT	hStmt	= (HDRVSTMT)hDrvStmt;
	COLUMNHDR	*pColumnHeader;
	int			nValue = 0;

    /* SANITY CHECKS */
    if( !hStmt )
        return SQL_INVALID_HANDLE;
	if ( !hStmt->hStmtExtras )
		return SQL_INVALID_HANDLE;

	if ( hStmt->hStmtExtras->nRows < 1 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No result set." );
		return SQL_ERROR;
	}
	if ( nCol < 1 || nCol > hStmt->hStmtExtras->nCols )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR Invalid column" );
		return SQL_ERROR;
	}

    /* OK */
    pColumnHeader = (COLUMNHDR*)(hStmt->hStmtExtras->aResults)[nCol];

    switch( nFieldIdentifier )
    {
	case SQL_DESC_AUTO_UNIQUE_VALUE:
		nValue = pColumnHeader->bSQL_DESC_AUTO_UNIQUE_VALUE;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_BASE_COLUMN_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_BASE_TABLE_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_BASE_TABLE_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_CASE_SENSITIVE:
		nValue = pColumnHeader->bSQL_DESC_CASE_SENSITIVE;
		break;
	case SQL_DESC_CATALOG_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_CATALOG_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_CONCISE_TYPE:
		nValue = pColumnHeader->nSQL_DESC_CONCISE_TYPE;
		break;
	case SQL_DESC_COUNT:
		nValue = hStmt->hStmtExtras->nCols;
		break;
	case SQL_DESC_DISPLAY_SIZE:
		nValue = pColumnHeader->nSQL_DESC_DISPLAY_SIZE;
		break;
	case SQL_DESC_FIXED_PREC_SCALE:
		nValue = pColumnHeader->bSQL_DESC_FIXED_PREC_SCALE;
		break;
	case SQL_DESC_LABEL:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_LABEL, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_LENGTH:
		nValue = pColumnHeader->nSQL_DESC_LENGTH;
		break;
	case SQL_DESC_LITERAL_PREFIX:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_LITERAL_PREFIX, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_LITERAL_SUFFIX:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_LITERAL_SUFFIX, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_LOCAL_TYPE_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_LOCAL_TYPE_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_NULLABLE:
		nValue = pColumnHeader->nSQL_DESC_NULLABLE;
		break;
	case SQL_DESC_NUM_PREC_RADIX:
		nValue = pColumnHeader->nSQL_DESC_NUM_PREC_RADIX;
		break;
	case SQL_DESC_OCTET_LENGTH:
		nValue = pColumnHeader->nSQL_DESC_OCTET_LENGTH;
		break;
	case SQL_DESC_PRECISION:
		nValue = pColumnHeader->nSQL_DESC_PRECISION;
		break;
	case SQL_DESC_SCALE:
		nValue = pColumnHeader->nSQL_DESC_SCALE;
		break;
	case SQL_DESC_SCHEMA_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_SCHEMA_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_SEARCHABLE:
		nValue = pColumnHeader->nSQL_DESC_SEARCHABLE;
		break;
	case SQL_DESC_TABLE_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_TABLE_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_TYPE:
		nValue = pColumnHeader->nSQL_DESC_TYPE;
		break;
	case SQL_DESC_TYPE_NAME:
		strncpy( pszValue, pColumnHeader->pszSQL_DESC_TYPE_NAME, nValueLengthMax );
		if ( pnValueLength )
            *pnValueLength = strlen( pszValue );
		break;
	case SQL_DESC_UNNAMED:
		nValue = pColumnHeader->nSQL_DESC_UNNAMED;
		break;
	case SQL_DESC_UNSIGNED:
		nValue = pColumnHeader->bSQL_DESC_UNSIGNED;
		break;
	case SQL_DESC_UPDATABLE:
		nValue = pColumnHeader->nSQL_DESC_UPDATABLE;
		break;
    default:
			sprintf((char*) hStmt->szSqlMsg, "Invalid nFieldIdentifier value of %d", nFieldIdentifier );
			logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );
            return SQL_ERROR;
    }

	if ( pnValue )
		*(int*)pnValue = nValue;

	/************************
     * REPLACE THIS COMMENT WITH SOMETHING USEFULL
     ************************/
    logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR This function not supported" );

    return SQL_ERROR;
}



