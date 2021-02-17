/* Testing result column numbers having hidden columns */
/* Test from Sebastien Flaesch */

#include "common.h"

static char software_version[] = "$Id: hidden.c,v 1.9 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char **argv)
{
	SQLSMALLINT cnt = 0;
	int failed = 0;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("CREATE TABLE #t1 ( k INT, c CHAR(10), vc VARCHAR(10) )");
	odbc_command("CREATE TABLE #tmp1 (i NUMERIC(10,0) IDENTITY PRIMARY KEY, b VARCHAR(20) NULL, c INT NOT NULL)");

	/* test hidden column with FOR BROWSE */
	odbc_reset_statement();

	odbc_command("SELECT c, b FROM #tmp1");

	CHKNumResultCols(&cnt, "S");

	if (cnt != 2) {
		fprintf(stderr, "Wrong number of columns in result set: %d\n", (int) cnt);
		failed = 1;
	}
	odbc_reset_statement();

	/* test hidden column with cursors*/
	odbc_check_cursor();

	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_NONSCROLLABLE, SQL_IS_UINTEGER, "S");
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY, (SQLPOINTER) SQL_SENSITIVE, SQL_IS_UINTEGER, "S");

	CHKPrepare(T("SELECT * FROM #t1"), SQL_NTS, "S");

	CHKExecute("S");

	CHKNumResultCols(&cnt, "S");

	if (cnt != 3) {
		fprintf(stderr, "Wrong number of columns in result set: %d\n", (int) cnt);
		failed = 1;
	}

	odbc_disconnect();

	return failed ? 1: 0;
}
