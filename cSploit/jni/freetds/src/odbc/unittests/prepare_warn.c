/*
 * Author           : sf@4js.com
 * FreeTDS version  : 0.91RC2 (20110429)
 * Platform         : Linux 32b (Debian Lenny)
 * Fetching when there is a NULL value in table gives invalid cursor state error.
 * Is this because of Warning SQLSTATE 01003?
 */

#include "common.h"

static char software_version[] = "$Id: prepare_warn.c,v 1.2 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char **argv)
{
#define ARRAY_SIZE 10
	SQLCHAR v_dec[ARRAY_SIZE][21];
	SQLLEN v_ind[ARRAY_SIZE];
	SQLULEN nrows;

	odbc_use_version3 = 1;
	odbc_connect();
	odbc_check_cursor();

	odbc_command("IF OBJECT_ID('mytab1') IS NOT NULL DROP TABLE mytab1");
	odbc_command("CREATE TABLE mytab1 ( k INT, d DECIMAL(10,2))");
	odbc_command("INSERT INTO mytab1 VALUES ( 201, 111.11 )");
	/*SQLExecDirect(m_hstmt, (SQLCHAR *) "insert into mytab1 values ( 202, 222.22 )", SQL_NTS); */
	odbc_command("INSERT INTO mytab1 VALUES ( 202, null )");

	odbc_reset_statement();

	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_NONSCROLLABLE, SQL_IS_UINTEGER, "S");
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY, (SQLPOINTER) SQL_SENSITIVE, SQL_IS_UINTEGER, "S");

	CHKSetStmtAttr(SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER) SQL_BIND_BY_COLUMN, SQL_IS_UINTEGER, "S");
	CHKSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) ARRAY_SIZE, SQL_IS_UINTEGER, "S");
	CHKSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER) & (nrows), SQL_IS_UINTEGER, "S");

	CHKPrepare(T("SELECT SUM(d) FROM mytab1"), SQL_NTS, "S");

	CHKExecute("I");

#if 0
	CHKMoreResults("S");	/* skip warning*/
#endif

	CHKBindCol(1, SQL_C_CHAR, v_dec, 21, v_ind, "S");

	CHKFetch("S");

	printf("fetch 1: rows fetched = %d\n", (int) nrows);
	printf("fetch 1: value = [%s]\n", v_dec[0]);

	CHKFetch("No");

	CHKMoreResults("No");

	odbc_command("drop table mytab1");

	odbc_disconnect();

	return 0;
}

