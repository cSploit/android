/*!
 * \file
 *
 * \author  Peter Harvey <pharvey@peterharvey.org> www.peterharvey.org
 * \author  \sa AUTHORS file
 * \version 1
 * \date    2007
 * \license Copyright unixODBC Project 2007-2008, LGPL
 */

/*! 
 * \mainpage    ODBC Trace Manager
 *
 * \section intro_sec Introduction
 *
 *              This library provides an interface to trace code which is cross-platform and supports plugin trace
 *              handlers. Drivers can use this to simplify support for producing trace output.
 *
 *              odbctrac is an example of a trace plugin which can be used with this trace interface.
 *
 *              odbctxt is an example of a Driver which uses this trace interface.
 */

#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <sqlext.h>

typedef SQLHANDLE *HTRACECALL;                  /*!< internal call handle of trace plugin   */

/*!
 * \brief   Trace handle.
 *
 *          Create an instance of this at the desired scope. The scope could be 1 for driver or 
 *          1 for each driver handle (env, dbc, stmt, desc) or some other scope.
 *
 *          Use #traceAlloc to allocate the instance or otherwise init the struct. Then use #traceOpen
 *          and #traceClose to open/close the trace plugin. Use #traceFree when your done with the 
 *          trace handle.
 *
\code
// init
SQLHDBC pConnection;
HTRACE  pTrace = traceAlloc();
{
    char szTrace[50];
    SQLGetPrivateProfileString( "odbctxt", "TraceFile", "/tmp/sql.log", pTrace->szTraceFile, FILENAME_MAX - 2, "odbcinst.ini" );
    SQLGetPrivateProfileString( "odbctxt", "TraceLibrary", "libodbctrac.so", pTrace->szFileName, FILENAME_MAX - 2, "odbcinst.ini" );
    SQLGetPrivateProfileString( "odbctxt", "Trace", "No", szTrace, sizeof( szTrace ), "odbcinst.ini" );
    if ( szTrace[ 0 ] == '1' || toupper( szTrace[ 0 ] ) == 'Y' || ( toupper( szTrace[ 0 ] ) == 'O' && toupper( szTrace[ 1 ] ) == 'N' ) )
        traceOpen( pTrace );
}
// use (SQLConnect as example)
{
    HTRACECALL hCall = traceConnect( pTrace, pConnection, "DataSource", SQL_NTS, "UID", SQL_NTS, "PWD", SQL_NTS );

    // do connect here

    return traceReturn( pTrace, hCall, nReturn );
}
// fini
{
    traceFree( pTrace );
}
\endcode
 */
typedef struct tTRACE
{
    void *      hPlugIn;                        /*!< library handle of trace plugin         */
    char        szFileName[FILENAME_MAX];       /*!< file name of trace plugin              */
    char        szTraceFile[FILENAME_MAX];      /*!< SQL_ATTR_TRACEFILE                     */
    SQLUINTEGER nTrace;                         /*!< SQL_ATTR_TRACE                         */
    SQLHANDLE   hPlugInInternal;                /*!< internal handle of trace plugin        */

    SQLHANDLE   (*pTraceAlloc)();
    void        (*pTraceFree)();
    SQLRETURN   (*pTraceReturn)();
    RETCODE     (*pTraceOpenLogFile)();
    RETCODE     (*pTraceCloseLogFile)();

    HTRACECALL (*pTraceSQLAlloConnect)();       
    HTRACECALL (*pTraceSQLAllocEnv)();          
    HTRACECALL (*pTraceSQLAllocHandle)();       
    HTRACECALL (*pTraceSQLAllocHandleStd)();    
    HTRACECALL (*pTraceSQLAllocStmt)();         
    HTRACECALL (*pTraceSQLBindCol)();           
    HTRACECALL (*pTraceSQLBindParam)();         
    HTRACECALL (*pTraceSQLBindParameter)();     
    HTRACECALL (*pTraceSQLBrowseConnect)();     
    HTRACECALL (*pTraceSQLBrowseConnectw)();    
    HTRACECALL (*pTraceSQLBulkOperations)();    
    HTRACECALL (*pTraceSQLCancel)();            
    HTRACECALL (*pTraceSQLCloseCursor)();       
    HTRACECALL (*pTraceSQLColAttribute)();      
    HTRACECALL (*pTraceSQLColAttributes)();     
    HTRACECALL (*pTraceSQLColAttributesW)();    
    HTRACECALL (*pTraceSQLColAttributeW)();     
    HTRACECALL (*pTraceSQLColumnPrivileges)();  
    HTRACECALL (*pTraceSQLColumnPrivilegesW)(); 
    HTRACECALL (*pTraceSQLColumns)();           
    HTRACECALL (*pTraceSQLColumnsW)();          
    HTRACECALL (*pTraceSQLConnect)();           
    HTRACECALL (*pTraceSQLConnectW)();           
    HTRACECALL (*pTraceSQLCopyDesc)();          
    HTRACECALL (*pTraceSQLDataSources)();       
    HTRACECALL (*pTraceSQLDataSourcesW)();      
    HTRACECALL (*pTraceSQLDescribeCol)();       
    HTRACECALL (*pTraceSQLDescribeColW)();      
    HTRACECALL (*pTraceSQLDescribeParam)();     
    HTRACECALL (*pTraceSQLDisconnect)();        
    HTRACECALL (*pTraceSQLDriverConnect)();     
    HTRACECALL (*pTraceSQLDriverConnectW)();    
    HTRACECALL (*pTraceSQLDrivers)();           
    HTRACECALL (*pTraceSQLDriversW)();          
    HTRACECALL (*pTraceSQLEndTran)();           
    HTRACECALL (*pTraceSQLError)();             
    HTRACECALL (*pTraceSQLErrorW)();            
    HTRACECALL (*pTraceSQLExecDirect)();        
    HTRACECALL (*pTraceSQLExecDirectW)();       
    HTRACECALL (*pTraceSQLExecute)();           
    HTRACECALL (*pTraceSQLExtendedFetch)();     
    HTRACECALL (*pTraceSQLFetch)();             
    HTRACECALL (*pTraceSQLFetchScroll)();       
    HTRACECALL (*pTraceSQLForeignKeys)();       
    HTRACECALL (*pTraceSQLForeignKeysW)();      
    HTRACECALL (*pTraceSQLFreeConnect)();       
    HTRACECALL (*pTraceSQLFreeEnv)();           
    HTRACECALL (*pTraceSQLFreeHandle)();        
    HTRACECALL (*pTraceSQLFreeStmt)();          
    HTRACECALL (*pTraceSQLGetConnectAttr)();    
    HTRACECALL (*pTraceSQLGetConnectAttrW)();   
    HTRACECALL (*pTraceSQLGetConnectOption)();  
    HTRACECALL (*pTraceSQLGetConnectOptionW)(); 
    HTRACECALL (*pTraceSQLGetCursorName)();     
    HTRACECALL (*pTraceSQLGetCursorNameW)();    
    HTRACECALL (*pTraceSQLGetData)();           
    HTRACECALL (*pTraceSQLGetDescField)();      
    HTRACECALL (*pTraceSQLGetDescFieldw)();     
    HTRACECALL (*pTraceSQLGetDescRec)();        
    HTRACECALL (*pTraceSQLGetDescRecW)();       
    HTRACECALL (*pTraceSQLGetDiagField)();      
    HTRACECALL (*pTraceSQLGetDiagFieldW)();     
    HTRACECALL (*pTraceSQLGetDiagRec)();        
    HTRACECALL (*pTraceSQLGetDiagRecW)();       
    HTRACECALL (*pTraceSQLGetEnvAttr)();        
    HTRACECALL (*pTraceSQLGetFunctions)();      
    HTRACECALL (*pTraceSQLGetInfo)();           
    HTRACECALL (*pTraceSQLGetInfoW)();          
    HTRACECALL (*pTraceSQLGetStmtAttr)();       
    HTRACECALL (*pTraceSQLGetStmtAttrW)();      
    HTRACECALL (*pTraceSQLGetStmtOption)();     
    HTRACECALL (*pTraceSQLGetTypeInfo)();       
    HTRACECALL (*pTraceSQLGetTypeInfoW)();      
    HTRACECALL (*pTraceSQLMoreResults)();       
    HTRACECALL (*pTraceSQLNativeSql)();         
    HTRACECALL (*pTraceSQLNativeSqlW)();        
    HTRACECALL (*pTraceSQLNumParams)();         
    HTRACECALL (*pTraceSQLNumResultCols)();     
    HTRACECALL (*pTraceSQLParamData)();         
    HTRACECALL (*pTraceSQLParamOptions)();      
    HTRACECALL (*pTraceSQLPrepare)();           
    HTRACECALL (*pTraceSQLPrepareW)();          
    HTRACECALL (*pTraceSQLPrimaryKeys)();       
    HTRACECALL (*pTraceSQLPrimaryKeysw)();      
    HTRACECALL (*pTraceSQLProcedureColumns)();  
    HTRACECALL (*pTraceSQLProcedureColumnsw)(); 
    HTRACECALL (*pTraceSQLProcedures)();        
    HTRACECALL (*pTraceSQLProceduresW)();       
    HTRACECALL (*pTraceSQLPutData)();           
    HTRACECALL (*pTraceSQLRowCount)();          
    HTRACECALL (*pTraceSQLSetConnectAttr)();    
    HTRACECALL (*pTraceSQLSetConnectAttrW)();   
    HTRACECALL (*pTraceSQLSetConnectoption)();  
    HTRACECALL (*pTraceSQLSetConnectoptionW)(); 
    HTRACECALL (*pTraceSQLSetCursorName)();     
    HTRACECALL (*pTraceSQLSetCursorNameW)();    
    HTRACECALL (*pTraceSQLSetDescField)();      
    HTRACECALL (*pTraceSQLSetDescFieldW)();     
    HTRACECALL (*pTraceSQLSetDescRec)();        
    HTRACECALL (*pTraceSQLSetDescRecW)();       
    HTRACECALL (*pTraceSQLSetEnvAttr)();        
    HTRACECALL (*pTraceSQLSetParam)();          
    HTRACECALL (*pTraceSQLSetPos)();            
    HTRACECALL (*pTraceSQLSetScrollOptions)();  
    HTRACECALL (*pTraceSQLSetStmtAttr)();       
    HTRACECALL (*pTraceSQLSetStmtAttrW)();      
    HTRACECALL (*pTraceSQLSetStmtOption)();     
    HTRACECALL (*pTraceSQLSetStmtOptionW)();    
    HTRACECALL (*pTraceSQLSpecialColumns)();    
    HTRACECALL (*pTraceSQLSpecialColumnsW)();   
    HTRACECALL (*pTraceSQLStatistics)();        
    HTRACECALL (*pTraceSQLStatisticsW)();       
    HTRACECALL (*pTraceSQLTablePrivileges)();   
    HTRACECALL (*pTraceSQLTablePrivilegesW)();  
    HTRACECALL (*pTraceSQLTables)();            
    HTRACECALL (*pTraceSQLTablesW)();           
    HTRACECALL (*pTraceSQLTransact)();          

} TRACE, *HTRACE;

/*! 
 * \brief   Allocates a trace handle.
 * 
 *          Tracing is turned off and no trace plugin is loaded.
 *
 * \return  HTRACE
 */
HTRACE traceAlloc();

/*! 
 * \brief   Frees trace handle.
 *
 *          Compliments #traceAlloc. Will call #traceClose as needed.
 *
 *          This will call #traceClose as needed.
 * 
 * \param   pTrace
 */
void traceFree( HTRACE pTrace );

/*! 
 * \brief   Opens trace plugin.
 * 
 *          This will open/load the trace plugin/library specified in pTrace->szFileName. The
 *          trace output will go to pTrace->szTraceFile. pTrace->nTrace is set to
 *          SQL_OPT_TRACE_ON but can be set to SQL_OPT_TRACE_OFF at any time to 'pause' tracing.
 *          Set it back to SQL_OPT_TRACE_ON to resume tracing.
 *
 * \param   pTrace
 * \param   pszApplication
 * 
 * \return  int
 */
int traceOpen( HTRACE pTrace, char *pszApplication );

/*! 
 * \brief   Close trace plugin.
 * 
 *          This will close/unload the trace plugin/library. pTrace->nTrace is set to SQL_OPT_TRACE_OFF
 *          but pTrace->szTraceFile and pTrace->szFileName remain unchanged.
 *
 *          The #traceFree function will silently call #traceClose as needed.
 *
 * \param   pTrace
 */
void traceClose( HTRACE pTrace );

/* this is just to shorten the following macros */
#define traceOn ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON )

/* these macros are used to call trace plugin functions - they help by calling ONLY if relevant */
#define traceAllocConnect( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLAllocConnect( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceAllocEnv( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLAllocEnv( pTrace->hPlugInInternal, B ) : NULL );
#define traceAllocHandle( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLAllocHandle( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceAllocStmt( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLAllocStmt( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceBindCol( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBindCol( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceBindParam( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBindParam( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceBindParameter( pTrace, B, C, D, E, F, G, H, I, J, K ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBindParameter( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K ) : NULL );
#define traceBrowseConnect( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBrowseConnect( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceBrowseConnectW( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBrowseConnectW( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceBulkOperations( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLBulkOperations( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceCancel( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLCancel( pTrace->hPlugInInternal, B ) : NULL );
#define traceCloseCursor( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLCloseCursor( pTrace->hPlugInInternal, B ) : NULL );
#define traceColAttribute( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColAttribute( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceColAttributes( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColAttributes( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceColAttributesW( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColAttributesW( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceColAttributeW( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColAttributeW( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceColumnPrivileges( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColumnPrivileges( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceColumnPrivilegesW( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColumnPrivilegesW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceColumns( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColumns( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceColumnsW( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLColumnsW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceConnect( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLConnect( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceConnectW( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLConnectW( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceCopyDesc( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLCopyDesc( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceDataSources( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDataSources( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceDataSourcesW( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDataSourcesW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceDescribeCol( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDescribeCol( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceDescribeColW( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDescribeColW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceDescribeParam( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDescribeParam( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceDisconnect( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDisconnect( pTrace->hPlugInInternal, B ) : NULL );
#define traceDriverConnect( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDriverConnect( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceDriverConnectW( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDriverConnectW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceDrivers( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDrivers( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceDriversW( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLDriversW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceEndTran( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLEndTran( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceError( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLError( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceErrorW( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLErrorW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceExecDirect( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLExecDirect( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceExecDirectW( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLExecDirectW( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceExecute( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLExecute( pTrace->hPlugInInternal, B ) : NULL );
#define traceExtendedFetch( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLExtendedFetch( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceFetch( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFetch( pTrace->hPlugInInternal, B ) : NULL );
#define traceFetchScroll( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFetchScroll( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceForeignKeys( pTrace, B, C, D, E, F, G, H, I, J, K, L, M, N ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLForeignKeys( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K, L, M, N ) : NULL );
#define traceForeignKeysW( pTrace, B, C, D, E, F, G, H, I, J, K, L, M, N ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLForeignKeysW( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K, L, M, N ) : NULL );
#define traceFreeConnect( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFreeConnect( pTrace->hPlugInInternal, B ) : NULL );
#define traceFreeEnv( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFreeEnv( pTrace->hPlugInInternal, B ) : NULL );
#define traceFreeHandle( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFreeHandle( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceFreeStmt( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLFreeStmt( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceGetConnectAttr( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetConnectAttr( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceGetCursorName( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetCursorName( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceGetData( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetData( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceGetDescField( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetDescField( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceGetDescRec( pTrace, B, C, D, E, F, G, H, I, J, K, L ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetDescRec( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K, L ) : NULL );
#define traceGetDiagField( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetDiagField( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceGetDiagRec( pTrace, B, C, D, E, F, G, H, I ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetDiagRec( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I ) : NULL );
#define traceGetEnvAttr( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetEnvAttr( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceGetFunctions( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetFunctions( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceGetInfo( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetInfo( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceGetStmtAttr( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetStmtAttr( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceGetTypeInfo( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLGetTypeInfo( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceMoreResults( pTrace, B ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLMoreResults( pTrace->hPlugInInternal, B ) : NULL );
#define traceNativeSql( pTrace, B, C, D, E, F, G ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLNativeSql( pTrace->hPlugInInternal, B, C, D, E, F, G ) : NULL );
#define traceNumParams( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLNumParams( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceNumResultCols( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLNumResultCols( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceNumParamData( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLParamData( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceParamOptions( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLParamOptions( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define tracePrepare( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLPrepare( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define tracePrimaryKeys( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLPrimaryKeys( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceProcedureColumns( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLProcedureColumns( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceProcedures( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLProcedures( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define tracePutData( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLPutData( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceRowCount( pTrace, B, C ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLRowCount( pTrace->hPlugInInternal, B, C ) : NULL );
#define traceSetConnectAttr( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetConnectAttr( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceSetCursorName( pTrace, B, C, D ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetCursorName( pTrace->hPlugInInternal, B, C, D ) : NULL );
#define traceSetDescField( pTrace, B, C, D, E, F ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetDescField( pTrace->hPlugInInternal, B, C, D, E, F ) : NULL );
#define traceSetDescRec( pTrace, B, C, D, E, F, G, H, I, J, K ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetDescRec( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K ) : NULL );
#define traceSetEnvAttr( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetEnvAttr( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceSetPos( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetPos( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceSetScrollOptions( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetScrollOptions( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceSetStmtAttr( pTrace, B, C, D, E ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSetStmtAttr( pTrace->hPlugInInternal, B, C, D, E ) : NULL );
#define traceSpecialColumns( pTrace, B, C, D, E, F, G, H, I, J, K ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLSpecialColumns( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J, K ) : NULL );
#define traceStatistics( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLStatistics( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );
#define traceTablePrivileges( pTrace, B, C, D, E, F, G, H ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLTablePrivileges( pTrace->hPlugInInternal, B, C, D, E, F, G, H ) : NULL );
#define traceTables( pTrace, B, C, D, E, F, G, H, I, J ) ( pTrace->hPlugIn && pTrace->nTrace == SQL_OPT_TRACE_ON ? pTrace->pTraceSQLTables( pTrace->hPlugInInternal, B, C, D, E, F, G, H, I, J ) : NULL );

#define traceReturn( pTrace, pCall, nReturn ) ( pCall ? pTrace->pTraceReturn( pCall, nReturn ) : nReturn );

#endif

