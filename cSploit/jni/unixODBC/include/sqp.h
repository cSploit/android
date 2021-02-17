#ifndef _SQP_H
#define _SQP_H

#include <lst.h>

/* #define SQPDEBUG */

/***[ PARSED, SUPPORTED, SQL PARTS ]**********************************************/
typedef enum sqpStatementType
{
    sqpcreatetable,
    sqpdroptable,
    sqpselect,
    sqpdelete,
    sqpinsert,
    sqpupdate
} sqpStatementType;

typedef enum sqpOrder
{
    sqpnone,
    sqpasc,
    sqpdesc
} sqpOrder;

typedef struct tSQPTABLE
{
    char *pszOwner;
    char *pszTable;

} SQPTABLE, *HSQPTABLE;

typedef struct tSQPCOLUMN
{
    char    *pszTable;
    char    *pszColumn;

    int     nColumn;            /* index into row data for col value	*/

} SQPCOLUMN, *HSQPCOLUMN;

typedef struct tSQPCOMPARISON
{
    char *pszLValue;            /* must be a column name 				*/
    char *pszOperator;          /* > < >= <= = LIKE NOTLIKE				*/
    char *pszRValue;            /* must be a string (quotes removed)	*/
	char cEscape;				/* escape char for LIKE operator		*/

    int  nLColumn;              /* index into row data for col value	*/

} SQPCOMPARISON, *HSQPCOMPARISON;

typedef struct tSQPASSIGNMENT
{
    char *pszColumn;
    char *pszValue;         

    int  nColumn;               /* index into row data for col value	*/

} SQPASSIGNMENT, *HSQPASSIGNMENT;

typedef struct tSQPDATATYPE
{
    char *pszType;
    short nType;          
    int  nPrecision;            
    int  nScale;

} SQPDATATYPE, *HSQPDATATYPE;

typedef struct tSQPCOLUMNDEF
{
    char *          pszColumn;
    HSQPDATATYPE    hDataType;
    int             bNulls;

} SQPCOLUMNDEF, *HSQPCOLUMNDEF;

typedef struct tSQPPARAM
{
    char *pszValue;         

} SQPPARAM, *HSQPPARAM;

typedef enum sqpCondType
{
	sqpor,
	sqpand,
	sqpnot,
	sqppar,
	sqpcomp
} sqpCondType;

typedef struct tSQPCOND
{
	sqpCondType nType;
	struct tSQPCOND *hLCond;
	struct tSQPCOND *hRCond;
	HSQPCOMPARISON   hComp;

} SQPCOND, *HSQPCOND;

/***[ PARSED, SUPPORTED, SQL STATEMENTS ]**********************************************/
typedef struct tSQPCREATETABLE
{
    char        *pszTable;
    HLST        hColumnDefs;    /* list of HSQPCOLUMNDEF                */

} SQPCREATETABLE, *HSQPCREATETABLE;

typedef char SQPDROPTABLE;
typedef SQPDROPTABLE * HSQPDROPTABLE;

typedef struct tSQPSELECT
{
    HLST        hColumns;       /* list of HSQPCOLUMN                   */
    char        *pszTable;      
    HSQPCOND    hWhere;         /* tree of HSQPCOND                     */
    HLST        hOrderBy;       /* list of HSQPCOLUMN                   */
    sqpOrder    nOrderDirection;

} SQPSELECT, *HSQPSELECT;

typedef struct tSQPDELETE
{
    char        *pszTable;
    HSQPCOND    hWhere;         /* tree of HSQPCOND                     */
    char        *pszCursor;

} SQPDELETE, *HSQPDELETE;

typedef struct tSQPINSERT
{
    HLST        hColumns;       /* list of HSQPCOLUMN                   */
    char        *pszTable;
    HLST        hValues;       /* list of strings                        */

} SQPINSERT, *HSQPINSERT;

typedef struct tSQPUPDATE
{
    char        *pszTable;
    HLST        hAssignments;   /* list of HSQPASSIGNMENT               */
    HSQPCOND    hWhere;         /* tree of HSQPCOND                     */
    char        *pszCursor;

} SQPUPDATE, *HSQPUPDATE;

/***[ TOP LEVEL STRUCT ]**********************************************/
typedef struct tSQPPARSEDSQL
{
    sqpStatementType nType;
    union
    {
        HSQPCREATETABLE hCreateTable;
        HSQPDROPTABLE   hDropTable;
        HSQPSELECT      hSelect;
        HSQPDELETE      hDelete;
        HSQPINSERT      hInsert;
        HSQPUPDATE      hUpdate;
    } h;

} SQPPARSEDSQL, *HSQPPARSEDSQL;

/***********************
 * GLOBALS (yuck... gotta get rid of them):
 *
 * TEMPS USED WHEN LEX/YACC DO THEIR THING
 ***********************/
extern char             g_szError[1024];

extern HSQPPARSEDSQL    g_hParsedSQL;

extern char *           g_pszTable;
extern char *           g_pszType;
extern HLST             g_hColumns;
extern HSQPDATATYPE     g_hDataType;
extern HLST             g_hColumnDefs;
extern HLST             g_hValues;
extern HLST             g_hAssignments;
extern char *           g_pszCursor;
extern HLST             g_hOrderBy;
extern sqpOrder         g_nOrderDirection;
extern int              g_nNulls;

extern char *           g_pszSQLCursor;     /* yyparse position init to start of SQL string before yyparse	*/
extern char *           g_pszSQLLimit;      /* ptr to NULL terminator of SQL string (yyparse stops here)	*/
extern int              g_nLineNo;

extern HLST             g_hParams;
extern HSQPCOND         g_hConds;

/*********************************************************************************************
 * PUBLIC INTERFACE
 *********************************************************************************************/
#if defined(__cplusplus)
extern  "C" {
#endif

/***********************
 * sqpOpen
 *
 * Inits parser globals. Must call this before any other functions
 * and sqpClose MUST be called before next sqpOpen.
 *
 * pszFirstChar     - pointer to 1st char in sql string
 * pszLastChar      - pointer to last char in sql string (typically a pointer to a '\0' char)
 * hParams          - list of bound parameters
 *
 ***********************/
void sqpOpen( char *pszFirstChar, char *pszLastChar, HLST hParams );

/***********************
 * sqpParse
 *
 * Attempts to parse the sql given in sqpOpen. 
 * Returns true if success else error. Use sqpError to get exact error after call.
 * Only call this once per sqpOpen.
 *
 ***********************/
int sqpParse();

/***********************
 * sqpError
 *
 * Returns the last error message (i.e. why a parse failed). Will
 * be an empty string if no error.
 *
 ***********************/
char * sqpError();

/***********************
 * sqpClose
 *
 * Cleans up globals in prep for next call to sqpOpen.
 *
 ***********************/
void sqpClose();

/***********************
 * sqpAdoptParsedSQL
 *
 * Caller adopts the top level pointer to the parsed sql. This means
 * that the caller must also call sqpFreeParsedSQL when done with it!
 *
 ***********************/
HSQPPARSEDSQL sqpAdoptParsedSQL();

/***********************
 * sqpFreeParsedSQL
 *
 * Frees the parsed sql from memory.
 * If this is being called as a result if a prior call to
 * sqpAdoptParsedSQL (which is the only reason it should be called in
 * this interface) then it can be called even after the sqpClose.
 *
 ***********************/
int  sqpFreeParsedSQL( HSQPPARSEDSQL hParsedSQL );

/***********************
 * sqpFreeParam
 *
 * Frees a bound param from memory.
 *
 ***********************/
void sqpFreeParam( void *pData );

#if defined(__cplusplus)
}
#endif

/*********************************************************************************************
 * INTERNAL FUNCS
 *********************************************************************************************/
int     my_yyinput(char *buf, int max_size);
void    yyerror( char *s );
int     yyparse();
int     yywrap();

short sqpStringTypeToSQLTYPE (char * pszType);

void sqpFreeAssignment( void *pData );
void sqpFreeColumn( void *pData );
void sqpFreeColumnDef( void *pData );
void sqpFreeDataType( void *pData );
void sqpFreeComparison( void *pData );
void sqpFreeCond( void *pData );
void sqpFreeCreateTable( void *pData );
void sqpFreeDropTable( void *pData );
void sqpFreeDelete( void *pData );
void sqpFreeInsert( void *pData );
void sqpFreeSelect( void *pData );
void sqpFreeUpdate( void *pData );
void sqpStoreAssignment( char *pszColumn, char *pszValue );
void sqpStoreColumn( HLST *ph, char *pszColumn, int nColumn );
void sqpStoreColumnDef( char *pszColumn );
void sqpStoreDataType( char *pszType, int nPrecision, int nScale );
HSQPCOMPARISON sqpStoreComparison( char *pszLValue, char *pszOperator, char *pszRValue, char *pszEscape );
HSQPCOND sqpStoreCond( sqpCondType nType, HSQPCOND pLCond, HSQPCOND pRCond, HSQPCOMPARISON pComp );
void sqpStoreCreateTable();
void sqpStoreDropTable();
void sqpStoreDelete();
void sqpStoreInsert();
void sqpStorePositioned( char *pszCursor );
void sqpStoreSelect();
void sqpStoreTable( char *pszTable );
void sqpStoreUpdate();
void sqpStoreValue( char *pszValue );

#endif

