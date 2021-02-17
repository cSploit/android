/* Test sp_cursorprepare / sp_cursorexecute usage to support SELECT FOR UPDATE
 * This test compiles and works fine with SQL Server Native Client, and uses
 * the sp_cursor* API Server Cursors ...
 */

#include "common.h"

static char software_version[] = "$Id: cursor4.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
exec_direct(const char *stmt)
{
	SQLHSTMT odbc_stmt = SQL_NULL_HSTMT;

	CHKAllocHandle(SQL_HANDLE_STMT, (SQLHANDLE) odbc_conn, (SQLHANDLE *) & odbc_stmt, "S");
	odbc_command(stmt);
	CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) odbc_stmt, "S");
}

int
main(int argc, char **argv)
{
	char buff[64];
	SQLLEN ind;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_check_cursor();

	exec_direct("CREATE TABLE #t1 ( k INT, c VARCHAR(20))");
	exec_direct("INSERT INTO #t1 VALUES (1, 'aaa')");

	odbc_reset_statement();

	CHKSetStmtAttr(SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_LOCK, SQL_IS_UINTEGER, "S");

	CHKSetCursorName(T("c112"), SQL_NTS, "S");

	CHKSetConnectAttr(SQL_ATTR_AUTOCOMMIT, int2ptr(SQL_AUTOCOMMIT_OFF), 0, "S");

	CHKPrepare(T("SELECT * FROM #t1 FOR UPDATE"), SQL_NTS, "S");

	CHKExecute("S");

	CHKFetch("S");

	exec_direct("UPDATE #t1 SET c = 'xxx' WHERE CURRENT OF c112");

	CHKCloseCursor("SI");

	CHKEndTran(SQL_HANDLE_DBC, odbc_conn, SQL_COMMIT, "S");

	CHKSetConnectAttr(SQL_ATTR_AUTOCOMMIT, int2ptr(SQL_AUTOCOMMIT_ON), 0, "S");

	CHKExecDirect(T("SELECT c FROM #t1 WHERE k = 1"), SQL_NTS, "S");

	CHKFetch("S");

	CHKGetData(1, SQL_C_CHAR, buff, sizeof(buff), &ind, "S");

	printf(">> New value after update = [%s] (should be [xxx]) \n", buff);

	CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) odbc_stmt, "S");
	odbc_stmt = SQL_NULL_HSTMT;

	odbc_disconnect();

	return 0;
}
