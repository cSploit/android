lib_LTLIBRARIES = libnn.la

INCLUDES = -I@top_srcdir@/include -I.

libnn_la_LDFLAGS = -no-undefined  -version-info 1:0:0 -module

EXTRA_DIST = \
	nnconfig.h \
	connect.h \
	convert.h \
	driver.h \
	herr.h \
	hstmt.h \
	isql.h \
	isqlext.h \
	nncol.h \
	nndate.h \
	nnsql.h \
	nntp.h \
	stmt.h \
	yyenv.h \
	yyerr.h \
	yylex.h \
	yyparse.tab.h \
	yystmt.h \
	herr.ci \
	nncol.ci \
	nntp.ci \
	yyerr.ci \
	yylex.ci \
	README

libnn_la_SOURCES = \
	SQLAllocConnect.c \
	SQLAllocEnv.c \
	SQLAllocStmt.c \
	SQLBindCol.c \
	SQLBindParameter.c \
	SQLCancel.c \
	SQLConnect.c \
	SQLDescribeCol.c \
	SQLDisconnect.c \
	SQLDriverConnect.c \
	SQLError.c \
	SQLExecDirect.c \
	SQLExecute.c \
	SQLFetch.c \
	SQLFreeConnect.c \
	SQLFreeEnv.c \
	SQLFreeStmt.c \
	SQLGetConnectOption.c \
	SQLGetData.c \
	SQLNumParams.c \
	SQLNumResultCols.c \
	SQLParamData.c \
	SQLPrepare.c \
	SQLPutData.c \
	SQLRowCount.c \
	SQLSetConnectOption.c \
	SQLSetParam.c \
	misc.c \
	connect.c \
	convert.c \
	execute.c \
	herr.c \
	prepare.c \
	yyparse.y \
	yylex.c \
	yystmt.c \
	yyerr.c \
	yyevl.c \
	yytchk.c \
	nncol.c \
	nndate.c \
	nntp.c

