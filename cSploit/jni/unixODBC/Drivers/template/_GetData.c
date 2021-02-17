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
#include "driver.h"

SQLRETURN _GetData(		SQLHSTMT      hDrvStmt,
						SQLUSMALLINT  nCol,
						SQLSMALLINT   nTargetType,				/* C DATA TYPE */
						SQLPOINTER    pTarget,
						SQLLEN        nTargetLength,
						SQLLEN       *pnLengthOrIndicator )
{
    HDRVSTMT	hStmt			= (HDRVSTMT)hDrvStmt;
	char    	*pSourceData    = NULL;

	/* SANITY CHECKS */
	if ( hStmt == SQL_NULL_HSTMT )
		return SQL_INVALID_HANDLE;
	if ( hStmt->hStmtExtras == SQL_NULL_HSTMT )
		return SQL_INVALID_HANDLE;

	if ( hStmt->hStmtExtras->nRows == 0 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No result set." );
		return SQL_ERROR;
	}

	/**********************************************************************
	 * GET pSourceData FOR NORMAL RESULT SETS
	 **********************************************************************/
	if ( hStmt->hStmtExtras->nRow > hStmt->hStmtExtras->nRows || hStmt->hStmtExtras->nRow < 1 )
	{
		logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING, "SQL_ERROR No current row" );
		return SQL_ERROR;
	}

	pSourceData = (hStmt->hStmtExtras->aResults)[hStmt->hStmtExtras->nRow*hStmt->hStmtExtras->nCols+nCol];

	/****************************
	 * ALL cols are stored as SQL_CHAR... bad for storage... good for code
	 * SO no need to determine the source type when translating to destination
	 ***************************/	
	if ( pSourceData == NULL )
	{
		/*********************
		 * Now get the col if value = NULL
		 *********************/
		if ( pnLengthOrIndicator != NULL )
			*pnLengthOrIndicator = SQL_NULL_DATA;

		switch ( nTargetType )
		{
		case SQL_C_LONG:
			memset( pTarget, 0, sizeof(int) );
			break;

		case SQL_C_FLOAT:
			memset( pTarget, 0, sizeof(float) );
			break;

		case SQL_C_CHAR:
			*((char *)pTarget) = '\0';
			break;

		default:
			sprintf((char*) hStmt->szSqlMsg, "SQL_ERROR Unknown target type %d", nTargetType );
			logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );
		}
	}
	else
	{
		/*********************
		 * Now get the col when we have a value
		 *********************/
		switch ( nTargetType )
		{
		case SQL_C_LONG:
			*((int *)pTarget) = atoi(pSourceData);
			if ( NULL != pnLengthOrIndicator )
				*pnLengthOrIndicator = sizeof( int );
			break;

		case SQL_C_FLOAT:
			sscanf( pSourceData, "%g", (float*)pTarget );
			if ( NULL != pnLengthOrIndicator )
				*pnLengthOrIndicator = sizeof( float );
			break;

		case SQL_C_CHAR:
			strncpy( pTarget, pSourceData, nTargetLength );
			if ( NULL != pnLengthOrIndicator )
				*pnLengthOrIndicator = strlen(pTarget);
			break;

		default:
			sprintf((char*) hStmt->szSqlMsg, "SQL_ERROR Unknown target type %d", nTargetType );
			logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_WARNING, LOG_WARNING,(char*) hStmt->szSqlMsg );
		}
	}

	logPushMsg( hStmt->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	return SQL_SUCCESS;
}


