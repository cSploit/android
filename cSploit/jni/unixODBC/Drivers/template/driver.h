/**********************************************
 * Driver.h
 *
 * Description:
 *
 * This is all of the stuff that is common among ALL drivers (but not to the DriverManager).
 *
 * Make sure that your driver specific driverextras.h exists!
 *
 * Creating a new driver? It is unlikely that you will need to change this
 * but take a look at driverextras.h
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 **********************************************/		
#ifndef _H_DRIVER
#define _H_DRIVER

/*****************************************************************************
 * ODBC VERSION (THAT THIS DRIVER COMPLIES WITH)
 *****************************************************************************/
#define ODBCVER 0x0351

#include <log.h>
#include <ini.h>
#include <sqlext.h>
#include <odbcinstext.h>
#include "driverextras.h"

#define SQL_MAX_CURSOR_NAME 100

/*****************************************************************************
 * STATEMENT
 *****************************************************************************/
typedef struct     tDRVSTMT
{
	struct tDRVSTMT	*pPrev;									/* prev struct or null		*/
	struct tDRVSTMT	*pNext;									/* next struct or null		*/
    SQLPOINTER 		hDbc;									/* pointer to DB context    */

    SQLCHAR    		szCursorName[SQL_MAX_CURSOR_NAME];		/* name of cursor           */
    SQLCHAR    		*pszQuery;								/* query string             */

	SQLCHAR     	szSqlMsg[LOG_MSG_MAX];					/* buff to format msgs		*/
	HLOG			hLog;									/* handle to msg logs		*/

    HSTMTEXTRAS 	hStmtExtras;							/* DRIVER SPECIFIC STORAGE  */

} DRVSTMT, *HDRVSTMT;

/*****************************************************************************
 * CONNECTION
 *****************************************************************************/
typedef struct     tDRVDBC
{
	struct tDRVDBC	*pPrev;								/* prev struct or null		*/
	struct tDRVDBC	*pNext;								/* next struct or null		*/
    SQLPOINTER		hEnv; 	                           	/* pointer to ENV structure */
	HDRVSTMT		hFirstStmt;							/* first in list or null	*/
	HDRVSTMT		hLastStmt;							/* last in list or null		*/

	SQLCHAR     	szSqlMsg[LOG_MSG_MAX];				/* buff to format msgs		*/
	HLOG			hLog;								/* handle to msg logs		*/

    int	        	bConnected;                         /* TRUE on open connection  */
    HDBCEXTRAS  	hDbcExtras;                         /* DRIVER SPECIFIC DATA     */

} DRVDBC, *HDRVDBC;

/*****************************************************************************
 * ENVIRONMENT
 *****************************************************************************/
typedef struct     tDRVENV
{
	HDRVDBC		hFirstDbc;							/* first in list or null	*/
	HDRVDBC		hLastDbc;							/* last in list or null		*/

    SQLCHAR     szSqlMsg[LOG_MSG_MAX];				/* buff to format msgs		*/
	HLOG		hLog;								/* handle to msg logs		*/

    HENVEXTRAS  hEnvExtras;

} DRVENV, *HDRVENV;

#endif


