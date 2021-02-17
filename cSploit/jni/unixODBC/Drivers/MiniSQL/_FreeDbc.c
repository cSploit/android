/**************************************************
 * _FreeDbc
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

SQLRETURN _FreeDbc( SQLHDBC hDrvDbc )
{
	HDRVDBC		hDbc	= (HDRVDBC)hDrvDbc;
	HDRVDBC		hPrevDbc;
	SQLRETURN	nReturn;

	if ( hDbc == SQL_NULL_HDBC )
		return SQL_ERROR;

	/* TRY TO FREE STATEMENTS						*/
	/* THIS IS JUST IN CASE; SHOULD NOT BE REQUIRED */
	nReturn = _FreeStmtList( hDbc );
	if ( nReturn != SQL_SUCCESS )
		return nReturn;

	/* SPECIAL CHECK FOR FIRST IN LIST 				*/
	if ( ((HDRVENV)hDbc->hEnv)->hFirstDbc == hDbc )
		((HDRVENV)hDbc->hEnv)->hFirstDbc = hDbc->pNext;

	/* SPECIAL CHECK FOR LAST IN LIST 				*/
	if ( ((HDRVENV)hDbc->hEnv)->hLastDbc == hDbc )
		((HDRVENV)hDbc->hEnv)->hLastDbc = hDbc->pPrev;

	/* EXTRACT SELF FROM LIST						*/
	if ( hDbc->pPrev != SQL_NULL_HDBC )
		hDbc->pPrev->pNext = hDbc->pNext;
	if ( hDbc->pNext != SQL_NULL_HDBC )
		hDbc->pNext->pPrev = hDbc->pPrev;


/**********************************************/
/* 	!!! CODE TO FREE DRIVER SPECIFIC MEMORY (hidden in hStmtExtras) HERE !!!	*/
	if ( hDbc->hDbcExtras->hServer > -1 )
		msqlClose( hDbc->hDbcExtras->hServer );

    free( hDbc->hDbcExtras );
/**********************************************/

    logPushMsg( hDbc->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );
	logClose( hDbc->hLog );
	free( hDbc );

	return SQL_SUCCESS;
}




