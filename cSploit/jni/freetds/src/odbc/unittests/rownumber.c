#include "common.h"

/* Test for SQL_ATTR_ROW_NUMBER */

/*
 * This works under MS ODBC however I don't know if it's correct... 
 * TODO make it work and add to Makefile.am
 */

static char software_version[] = "$Id: rownumber.c,v 1.6 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
CheckRowNum(int n, int line)
{
	SQLUINTEGER value;

	if (n < 0) {
		CHKGetStmtAttr(SQL_ATTR_ROW_NUMBER, &value, sizeof(value), NULL, "E");
		return;
	}

	CHKGetStmtAttr(SQL_ATTR_ROW_NUMBER, &value, sizeof(value), NULL, "S");
	if (value != n) {
		fprintf(stderr, "Expected %d rows returned %d line %d\n", n, (int) value, line);
		exit(1);
	}
}

#undef ODBC_CHECK_ROWS
#define ODBC_CHECK_ROWS(n) CheckRowNum(n,__LINE__)

static void
DoTest()
{
	int n = 0;
	static const char query[] = "SELECT * FROM #tmp1 ORDER BY i SELECT * FROM #tmp1 WHERE i < 3 ORDER BY i";

	/* execute a batch command and check row number */
	CHKExecDirect((SQLCHAR *) query, SQL_NTS, "S");

	ODBC_CHECK_ROWS(-1);
	printf("Result %d\n", ++n);
	CHKFetch("S");
	ODBC_CHECK_ROWS(0);
	CHKFetch("S");
	ODBC_CHECK_ROWS(0);
	CHKFetch("S");
	ODBC_CHECK_ROWS(0);
	CHKFetch("No");
	ODBC_CHECK_ROWS(-1);
	CHKMoreResults("S");
	ODBC_CHECK_ROWS(-1);

	printf("Result %d\n", ++n);
	CHKFetch("S");
	ODBC_CHECK_ROWS(0);
	CHKFetch("S");
	ODBC_CHECK_ROWS(0);
	CHKFetch("No");
	ODBC_CHECK_ROWS(-1);
	CHKMoreResults("No");
	ODBC_CHECK_ROWS(-1);
}

int
main(int argc, char *argv[])
{
	odbc_use_version3 = 1;

	odbc_connect();

	odbc_command("create table #tmp1 (i int)");
	odbc_command("create table #tmp2 (i int)");
	odbc_command("insert into #tmp1 values(1)");
	odbc_command("insert into #tmp1 values(2)");
	odbc_command("insert into #tmp1 values(5)");

	DoTest();

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
