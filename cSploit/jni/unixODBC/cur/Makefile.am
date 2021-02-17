lib_LTLIBRARIES = libodbccr.la

INCLUDES = -I@top_srcdir@/include \
            -I@top_srcdir@/DriverManager $(LTDLINCL)

EXTRA_DIST = \
	cursorlibrary.h \
	cur.exp

libodbccr_la_LDFLAGS = \
	-no-undefined \
	-version-info @LIB_VERSION@ \
	-export-symbols @srcdir@/cur.exp -export-dynamic 

libodbccr_la_SOURCES = \
    SQLAllocHandle.c \
    SQLAllocHandleStd.c \
    SQLAllocStmt.c \
    SQLBindCol.c \
    SQLBindParam.c \
    SQLBindParameter.c \
    SQLCancel.c \
    SQLCloseCursor.c \
    SQLColAttribute.c \
    SQLColAttributes.c \
    SQLColumnPrivileges.c \
    SQLColumns.c \
    SQLConnect.c \
    SQLCopyDesc.c \
    SQLDescribeCol.c \
    SQLDescribeParam.c \
    SQLEndTran.c \
    SQLError.c \
    SQLExecDirect.c \
    SQLExecute.c \
    SQLExtendedFetch.c \
    SQLFetch.c \
    SQLFetchScroll.c \
    SQLForeignKeys.c \
    SQLFreeHandle.c \
    SQLFreeStmt.c \
    SQLGetConnectAttr.c \
    SQLGetConnectOption.c \
    SQLGetCursorName.c \
    SQLGetData.c \
    SQLGetDescField.c \
    SQLGetDescRec.c \
    SQLGetDiagRec.c \
    SQLGetDiagField.c \
    SQLGetInfo.c \
    SQLGetStmtAttr.c \
    SQLGetStmtOption.c \
    SQLGetTypeInfo.c \
    SQLMoreResults.c \
    SQLNativeSql.c \
    SQLNumParams.c \
    SQLNumResultCols.c \
    SQLParamData.c \
    SQLParamOptions.c \
    SQLPrepare.c \
    SQLPrimaryKeys.c \
    SQLProcedureColumns.c \
    SQLProcedures.c \
    SQLPutData.c \
    SQLRowCount.c \
    SQLSetConnectAttr.c \
    SQLSetConnectOption.c \
    SQLSetCursorName.c \
    SQLSetDescRec.c \
    SQLSetDescField.c \
    SQLSetParam.c \
    SQLSetPos.c \
    SQLSetScrollOptions.c \
    SQLSetStmtAttr.c \
    SQLSetStmtOption.c \
    SQLSpecialColumns.c \
    SQLStatistics.c \
    SQLTablePrivileges.c \
    SQLTables.c \
    SQLTransact.c
