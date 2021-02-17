/* Tests 2 active statements */
#include "common.h"

static char software_version[] = "$Id: cursor3.c,v 1.11 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char **argv)
{
	SQLHSTMT stmt1 = SQL_NULL_HSTMT;
	SQLHSTMT stmt2 = SQL_NULL_HSTMT;
	SQLHSTMT old_odbc_stmt;
	char buff[64];
	SQLLEN ind;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_check_cursor();

	odbc_command("CREATE TABLE #t1 ( k INT, c VARCHAR(20))");
	odbc_command("INSERT INTO #t1 VALUES (1, 'aaa')");
	odbc_command("INSERT INTO #t1 VALUES (2, 'bbbbb')");
	odbc_command("INSERT INTO #t1 VALUES (3, 'ccccccccc')");
	odbc_command("INSERT INTO #t1 VALUES (4, 'dd')");

	old_odbc_stmt = odbc_stmt;

	CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &stmt1, "S");
	CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &stmt2, "S");


	odbc_stmt = stmt1;
/*	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_NONSCROLLABLE, SQL_IS_UINTEGER, "S"); */
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY, (SQLPOINTER) SQL_SENSITIVE, SQL_IS_UINTEGER, "S");
/*	CHKSetStmtAttr(SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_LOCK, SQL_IS_UINTEGER, "S"); */


	odbc_stmt = stmt2;
/*	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_NONSCROLLABLE, SQL_IS_UINTEGER, "S"); */
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY, (SQLPOINTER) SQL_SENSITIVE, SQL_IS_UINTEGER, "S");
/*	CHKSetStmtAttr(SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_LOCK, SQL_IS_UINTEGER, "S"); */

	odbc_stmt = stmt1;
	CHKSetCursorName(T("c1"), SQL_NTS, "S");

	odbc_stmt = stmt2;
	CHKSetCursorName(T("c2"), SQL_NTS, "S");

	odbc_stmt = stmt1;
	CHKPrepare(T("SELECT * FROM #t1 ORDER BY k"), SQL_NTS, "S");

	odbc_stmt = stmt2;
	CHKPrepare(T("SELECT * FROM #t1 ORDER BY k DESC"), SQL_NTS, "S");

	odbc_stmt = stmt1;
	CHKExecute("S");

	odbc_stmt = stmt2;
	CHKExecute("S");

	odbc_stmt = stmt1;
	CHKFetch("S");

	CHKGetData(2, SQL_C_CHAR, (SQLPOINTER) buff, sizeof(buff), &ind, "S");
	printf(">> Fetch from 1: [%s]\n", buff);

	odbc_stmt = stmt2;
	CHKFetch("S");

	CHKGetData(2, SQL_C_CHAR, (SQLPOINTER) buff, sizeof(buff), &ind, "S");
	printf(">> Fetch from 2: [%s]\n", buff);

	/*
	 * this should check a problem with SQLGetData 
	 * fetch a data on stmt2 than fetch on stmt1 and try to get data on first one
	 */
	CHKFetch("S");	/* "ccccccccc" */
	odbc_stmt = stmt1;
	CHKFetch("S");  /* "bbbbb" */
	odbc_stmt = stmt2;
	CHKGetData(2, SQL_C_CHAR, (SQLPOINTER) buff, sizeof(buff), &ind, "S");
	printf(">> Fetch from 2: [%s]\n", buff);
	if (strcmp(buff, "ccccccccc") != 0)
		ODBC_REPORT_ERROR("Invalid results from SQLGetData");
	odbc_stmt = stmt1;
	CHKGetData(2, SQL_C_CHAR, (SQLPOINTER) buff, sizeof(buff), &ind, "S");
	printf(">> Fetch from 1: [%s]\n", buff);
	if (strcmp(buff, "bbbbb") != 0)
		ODBC_REPORT_ERROR("Invalid results from SQLGetData");

	odbc_stmt = stmt1;
	CHKCloseCursor("SI");
	odbc_stmt = stmt2;
	CHKCloseCursor("SI");

	odbc_stmt = stmt1;
	CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) stmt1, "S");
	odbc_stmt = stmt2;
	CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) stmt2, "S");

	odbc_stmt = old_odbc_stmt;
	odbc_disconnect();

	return 0;
}

