/**********************************************
 * driverextras.h
 *
 * Purpose:
 *
 * To define driver specifc extras such as structs to be included
 * along side the common ODBC includes.
 *
 * Description:
 *
 * The short-term storage a driver requires as infrastructure varies somewhat from
 * DBMS to DBMS. The ODBC DriverManager requires predictable storage and it is defined
 * in drivermanager.h. The Driver also requires predictable storage and
 * it is defined in driver.h. Storage *specific to a type of driver* is defined here.
 *
 * The three main storage items are the ENV, DBC, and STMT structs. These are defined
 * as type void * in isql.h.
 *
 * So if your driver requires extra storage (and it probably does) then define
 * the storage within these structs, allocate/initialize as required. Cast them
 * to and from the standard names as required.
 *
 * For example;
 *
 * App			DM				|DRV				
 *				(as per hdbc.h)	|(as per driver.h)	
 *====================================================================
 *	hDbc=void*	hDbc=SQLHDBC	hDbc=HDBCEXTRAS
 *--------------------------------------------------------------------
 *
 * DO NOT FORGET TO FREE ANY ALLOCATED MEMORY (at some point)
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 **********************************************************************/

#ifndef DRIVEREXTRAS_H
#define DRIVEREXTRAS_H

/**********************************************
 * KEEP IT SIMPLE; PUT ALL DRIVER INCLUDES HERE THEN EACH DRIVER MODULE JUST INCLUDES THIS ONE FILE
 **********************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/****************************
 * include DBMS specific includes here and make any required defines here as well */




/***************************/


/**********************************************
 * ENVIRONMENT: DRIVER SPECIFIC STUFF THAT NEEDS TO BE STORED IN THE DRIVERS ENVIRONMENT
 **********************************************/
typedef struct tENVEXTRAS
{
	/****************************
	 * this is usually left empty
	 ***************************/
    int     nDummy;

} ENVEXTRAS, *HENVEXTRAS;

/**********************************************
 * CONNECTION: DRIVER SPECIFIC STUFF THAT NEEDS TO BE STORED IN THE DRIVERS CONNECTIONS
 **********************************************/

typedef struct tDBCEXTRAS
{
	/****************************
	 * replace dummy with your DBMS specific stuff. This often means such things as a socket handle.
	 ***************************/
    int     nDummy;

} DBCEXTRAS, *HDBCEXTRAS;

/**********************************************
 * STATEMENT: DRIVER SPECIFIC STUFF THAT NEEDS TO BE STORED IN THE DRIVERS STATEMENTS
 **********************************************/
typedef struct tCOLUMNHDR							/* each element of the 1st row of results is one of these (except col 0) */
{
	/* EVERYTHING YOU WOULD EVER WANT TO KNOW ABOUT THE COLUMN. INIT THIS BY CALLING 			*/
	/* _NativeToSQLColumnHeader AS SOON AS YOU HAVE COLUMN INFO. THIS MAKES THE COL HDR LARGER	*/
	/* BUT GENERALIZES MORE CODE. see SQLColAttribute()											*/
	int		bSQL_DESC_AUTO_UNIQUE_VALUE;			/* IS AUTO INCREMENT COL?					*/
	char	*pszSQL_DESC_BASE_COLUMN_NAME;			/* empty string if N/A						*/
	char	*pszSQL_DESC_BASE_TABLE_NAME;			/* empty string if N/A						*/
	int		bSQL_DESC_CASE_SENSITIVE;				/* IS CASE SENSITIVE COLUMN?				*/
	char	*pszSQL_DESC_CATALOG_NAME;				/* empty string if N/A						*/
	int		nSQL_DESC_CONCISE_TYPE;					/* ie SQL_CHAR, SQL_TYPE_TIME...			*/
	int		nSQL_DESC_DISPLAY_SIZE;					/* max digits required to display			*/
	int		bSQL_DESC_FIXED_PREC_SCALE;				/* has data source specific precision?		*/
	char	*pszSQL_DESC_LABEL;						/* display label, col name or empty string	*/
	int		nSQL_DESC_LENGTH;						/* strlen or bin size						*/
	char	*pszSQL_DESC_LITERAL_PREFIX;			/* empty string if N/A						*/
	char	*pszSQL_DESC_LITERAL_SUFFIX;			/* empty string if N/A						*/
	char	*pszSQL_DESC_LOCAL_TYPE_NAME;			/* empty string if N/A						*/
	char	*pszSQL_DESC_NAME;						/* col alias, col name or empty string		*/
	int		nSQL_DESC_NULLABLE;						/* SQL_NULLABLE, _NO_NULLS or _UNKNOWN		*/
	int		nSQL_DESC_NUM_PREC_RADIX;				/* 2, 10, or if N/A... 0 					*/
	int		nSQL_DESC_OCTET_LENGTH;					/* max size									*/
	int		nSQL_DESC_PRECISION;					/*											*/
	int		nSQL_DESC_SCALE;						/*											*/
	char	*pszSQL_DESC_SCHEMA_NAME;				/* empty string if N/A						*/
	int		nSQL_DESC_SEARCHABLE;					/* can be in a filter ie SQL_PRED_NONE...	*/
	char	*pszSQL_DESC_TABLE_NAME;                /* empty string if N/A						*/
	int		nSQL_DESC_TYPE;							/* SQL data type ie SQL_CHAR, SQL_INTEGER..	*/
	char	*pszSQL_DESC_TYPE_NAME;					/* DBMS data type ie VARCHAR, MONEY...		*/
	int		nSQL_DESC_UNNAMED;						/* qualifier for SQL_DESC_NAME ie SQL_NAMED	*/
	int		bSQL_DESC_UNSIGNED;						/* if signed FALSE else TRUE				*/
	int		nSQL_DESC_UPDATABLE;					/* ie SQL_ATTR_READONLY, SQL_ATTR_WRITE...	*/

	/* BINDING INFO		*/
	short			nTargetType;					/* BIND: C DATA TYPE ie SQL_C_CHAR			*/
	char 			*pTargetValue;					/* BIND: POINTER FROM APPLICATION TO COPY TO*/
	long			nTargetValueMax;				/* BIND: MAX SPACE IN pTargetValue			*/
	long			*pnLengthOrIndicator;			/* BIND: TO RETURN LENGTH OR NULL INDICATOR	*/

} COLUMNHDR;

typedef struct tSTMTEXTRAS
{
	char			**aResults;						/* nRows x nCols OF CHAR POINTERS. Row 0= ptrs to COLUMNHDR. Col 0=Bookmarks				*/
	int				nCols;							/* # OF VALID COLUMNS IN aColumns				*/
	int				nRows;							/* # OF ROWS IN aResults						*/
	int				nRow;							/* CURRENT ROW									*/

} STMTEXTRAS, *HSTMTEXTRAS;


/*
 * Shadow functions
 *
 * There are times when a function needs to call another function to use common functionality. When the
 * called function is part of the ODBC API (an entry point to the driver) bad things can happen. The 
 * linker will get confused and call the same-named function elsewhere in the namespace (ie the Driver 
 * Manager). To get around this you create a shadow function - a function with a slightly different name
 * that will not be confused with others - and then put most if not all of the functionality in there for
 * common use.
 *
 */
SQLRETURN  SQLGetDiagRec_(   SQLSMALLINT    nHandleType,
                             SQLHANDLE      hHandle,
                             SQLSMALLINT    nRecordNumber,
                             SQLCHAR *      pszState,
                             SQLINTEGER *   pnNativeError,
                             SQLCHAR *      pszMessageText,
                             SQLSMALLINT    nBufferLength,
                             SQLSMALLINT *  pnStringLength
                        );

SQLRETURN	_GetData(					SQLHSTMT      	hDrvStmt,
										SQLUSMALLINT  	nCol,
										SQLSMALLINT   	nTargetType,
										SQLPOINTER    	pTarget,
										SQLLEN       	nTargetLength,
										SQLLEN       	*pnLengthOrIndicator );

/*
 * Internal Support Functions
 */
SQLRETURN	_NativeToSQLColumnHeader(	COLUMNHDR 		*pColumnHeader,
										void 			*pNativeColumnHeader );
int 		_NativeToSQLType( 			void 			*pNativeColumnHeader );
char 		*_NativeTypeDesc(          	char 			*pszTypeName,
										int 			nType );
int			_NativeTypeLength( 			void 			*pNativeColumnHeader );
int			_NativeTypePrecision( 		void 			*pNativeColumnHeader );

SQLRETURN template_SQLPrepare(	SQLHSTMT    hDrvStmt,
						SQLCHAR     *szSqlStr,
						SQLINTEGER  nSqlStrLength );

#endif


