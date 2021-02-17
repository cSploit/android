#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <direct.h>
#endif

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>

static char rcsid_common_h[] = "$Id: common.h,v 1.39 2012-03-04 11:33:07 freddy77 Exp $";
static void *no_unused_common_h_warn[] = { rcsid_common_h, no_unused_common_h_warn };

#ifndef HAVE_SQLLEN
#ifndef SQLULEN
#define SQLULEN SQLUINTEGER
#endif
#ifndef SQLLEN
#define SQLLEN SQLINTEGER
#endif
#endif

#define FREETDS_SRCDIR FREETDS_TOPDIR "/src/odbc/unittests"

extern HENV odbc_env;
extern HDBC odbc_conn;
extern HSTMT odbc_stmt;
extern int odbc_use_version3;
extern void (*odbc_set_conn_attr)(void);
extern char odbc_err[512];
extern char odbc_sqlstate[6];


extern char odbc_user[512];
extern char odbc_server[512];
extern char odbc_password[512];
extern char odbc_database[512];
extern char odbc_driver[1024];

int odbc_read_login_info(void);
void odbc_report_error(const char *msg, int line, const char *file);
void odbc_read_error(void);


void odbc_check_cols(int n, int line, const char * file);
void odbc_check_rows(int n, int line, const char * file);
#define ODBC_CHECK_ROWS(n) odbc_check_rows(n, __LINE__, __FILE__)
#define ODBC_CHECK_COLS(n) odbc_check_cols(n, __LINE__, __FILE__)
void odbc_reset_statement_proc(SQLHSTMT *stmt, const char *file, int line);
#define odbc_reset_statement() odbc_reset_statement_proc(&odbc_stmt, __FILE__, __LINE__)
void odbc_check_cursor(void);

#define ODBC_REPORT_ERROR(msg) odbc_report_error(msg, __LINE__, __FILE__)

SQLRETURN odbc_check_res(const char *file, int line, SQLRETURN rc, SQLSMALLINT handle_type, SQLHANDLE handle, const char *func, const char *res);
#define CHKR(func, params, res) \
	odbc_check_res(__FILE__, __LINE__, (func params), 0, 0, #func, res)
#define CHKR2(func, params, type, handle, res) \
	odbc_check_res(__FILE__, __LINE__, (func params), type, handle, #func, res)

SQLSMALLINT odbc_alloc_handle_err_type(SQLSMALLINT type);

#define CHKAllocConnect(a,res) \
	CHKR2(SQLAllocConnect, (odbc_env,a), SQL_HANDLE_ENV, odbc_env, res)
#define CHKAllocEnv(a,res) \
	CHKR2(SQLAllocEnv, (a), 0, 0, res)
#define CHKAllocStmt(a,res) \
	CHKR2(SQLAllocStmt, (odbc_conn,a), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKAllocHandle(a,b,c,res) \
	CHKR2(SQLAllocHandle, (a,b,c), odbc_alloc_handle_err_type(a), b, res)
#define CHKBindCol(a,b,c,d,e,res) \
	CHKR2(SQLBindCol, (odbc_stmt,a,b,c,d,e), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKBindParameter(a,b,c,d,e,f,g,h,i,res) \
	CHKR2(SQLBindParameter, (odbc_stmt,a,b,c,d,e,f,g,h,i), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKCancel(res) \
	CHKR2(SQLCancel, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKCloseCursor(res) \
	CHKR2(SQLCloseCursor, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKColAttribute(a,b,c,d,e,f,res) \
	CHKR2(SQLColAttribute, (odbc_stmt,a,b,c,d,e,f), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKDescribeCol(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLDescribeCol, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKConnect(a,b,c,d,e,f,res) \
	CHKR2(SQLConnect, (odbc_conn,a,b,c,d,e,f), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKDriverConnect(a,b,c,d,e,f,g,res) \
	CHKR2(SQLDriverConnect, (odbc_conn,a,b,c,d,e,f,g), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKEndTran(a,b,c,res) \
	CHKR2(SQLEndTran, (a,b,c), a, b, res)
#define CHKExecDirect(a,b,res) \
	CHKR2(SQLExecDirect, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKExecute(res) \
	CHKR2(SQLExecute, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKExtendedFetch(a,b,c,d,res) \
	CHKR2(SQLExtendedFetch, (odbc_stmt,a,b,c,d), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFetch(res) \
	CHKR2(SQLFetch, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFetchScroll(a,b,res) \
	CHKR2(SQLFetchScroll, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFreeHandle(a,b,res) \
	CHKR2(SQLFreeHandle, (a,b), a, b, res)
#define CHKFreeStmt(a,res) \
	CHKR2(SQLFreeStmt, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetConnectAttr(a,b,c,d,res) \
	CHKR2(SQLGetConnectAttr, (odbc_conn,a,b,c,d), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKGetDiagRec(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLGetDiagRec, (a,b,c,d,e,f,g,h), a, b, res)
#define CHKGetDiagField(a,b,c,d,e,f,g,res) \
	CHKR2(SQLGetDiagField, (a,b,c,d,e,f,g), a, b, res)
#define CHKGetStmtAttr(a,b,c,d,res) \
	CHKR2(SQLGetStmtAttr, (odbc_stmt,a,b,c,d), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetTypeInfo(a,res) \
	CHKR2(SQLGetTypeInfo, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetData(a,b,c,d,e,res) \
	CHKR2(SQLGetData, (odbc_stmt,a,b,c,d,e), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKMoreResults(res) \
	CHKR2(SQLMoreResults, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKNumResultCols(a,res) \
	CHKR2(SQLNumResultCols, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKParamData(a,res) \
	CHKR2(SQLParamData, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKParamOptions(a,b,res) \
	CHKR2(SQLParamOptions, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKPrepare(a,b,res) \
	CHKR2(SQLPrepare, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKPutData(a,b,res) \
	CHKR2(SQLPutData, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKRowCount(a,res) \
	CHKR2(SQLRowCount, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetConnectAttr(a,b,c,res) \
	CHKR2(SQLSetConnectAttr, (odbc_conn,a,b,c), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKSetCursorName(a,b,res) \
	CHKR2(SQLSetCursorName, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetPos(a,b,c,res) \
	CHKR2(SQLSetPos, (odbc_stmt,a,b,c), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetStmtAttr(a,b,c,res) \
	CHKR2(SQLSetStmtAttr, (odbc_stmt,a,b,c), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetStmtOption(a,b,res) \
	CHKR2(SQLSetStmtOption, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKTables(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLTables, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKProcedureColumns(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLProcedureColumns, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKColumns(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLColumns, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetDescRec(a,b,c,d,e,f,g,h,i,j,res) \
	CHKR2(SQLGetDescRec, (Descriptor,a,b,c,d,e,f,g,h,i,j), SQL_HANDLE_STMT, Descriptor, res)

int odbc_connect(void);
int odbc_disconnect(void);
SQLRETURN odbc_command_proc(HSTMT stmt, const char *command, const char *file, int line, const char *res);
#define odbc_command(cmd) odbc_command_proc(odbc_stmt, cmd, __FILE__, __LINE__, "SNo")
#define odbc_command2(cmd, res) odbc_command_proc(odbc_stmt, cmd, __FILE__, __LINE__, res)
SQLRETURN odbc_command_with_result(HSTMT stmt, const char *command);
int odbc_db_is_microsoft(void);
const char *odbc_db_version(void);
unsigned int odbc_db_version_int(void);
int odbc_driver_is_freetds(void);

#define ODBC_VECTOR_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define int2ptr(i) ((void*)(((char*)0)+(i)))
#define ptr2int(p) ((int)(((char*)(p))-((char*)0)))

#if !HAVE_SETENV
void odbc_setenv(const char *name, const char *value, int overwrite);

#define setenv odbc_setenv
#endif

#ifndef _WIN32
int odbc_find_last_socket(void);
#endif

void odbc_c2string(char *out, SQLSMALLINT out_c_type, const void *in, size_t in_len);

int odbc_to_sqlwchar(SQLWCHAR *dst, const char *src, int n);
int odbc_from_sqlwchar(char *dst, const SQLWCHAR *src, int n);

typedef struct odbc_buf{
	struct odbc_buf *next;
	void *buf;
} ODBC_BUF;
extern ODBC_BUF *odbc_buf;
void *odbc_buf_get(ODBC_BUF** buf, size_t s);
void odbc_buf_free(ODBC_BUF** buf);
#define ODBC_GET(s)  odbc_buf_get(&odbc_buf, s)
#define ODBC_FREE() odbc_buf_free(&odbc_buf)

SQLWCHAR *odbc_get_sqlwchar(ODBC_BUF** buf, const char *s);
char *odbc_get_sqlchar(ODBC_BUF** buf, SQLWCHAR *s);

#undef T
#ifdef UNICODE
/* char to TCHAR */
#define T(s) odbc_get_sqlwchar(&odbc_buf, (s))
/* TCHAR to char */
#define C(s) odbc_get_sqlchar(&odbc_buf, (s))
#else
#define T(s) ((SQLCHAR*)(s))
#define C(s) ((char*)(s))
#endif

