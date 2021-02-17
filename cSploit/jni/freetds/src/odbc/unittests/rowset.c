#include "common.h"

static char software_version[] = "$Id: rowset.c,v 1.8 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
test_err(int n)
{
	CHKSetStmtAttr(SQL_ROWSET_SIZE, (SQLPOINTER) int2ptr(n), 0, "E");
	odbc_read_error();
	if (strcmp(odbc_sqlstate, "HY024") != 0) {
		fprintf(stderr, "Unexpected sql state returned\n");
		odbc_disconnect();
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	int i;
	SQLLEN len;
#ifdef HAVE_SQLROWSETSIZE
	SQLROWSETSIZE row_count;
#else
	SQLULEN row_count;
#endif
	SQLUSMALLINT statuses[10];
	char buf[32];

	odbc_use_version3 = 1;
	odbc_connect();

	/* initial value should be 1 */
	CHKGetStmtAttr(SQL_ROWSET_SIZE, &len, sizeof(len), NULL, "S");
	if (len != 1) {
		fprintf(stderr, "len should be 1\n");
		odbc_disconnect();
		return 1;
	}

	/* check invalid parameter values */
	test_err(-123);
	test_err(-1);
	test_err(0);

	odbc_check_cursor();

	/* set some correct values */
	CHKSetStmtAttr(SQL_ROWSET_SIZE, (SQLPOINTER) int2ptr(2), 0, "S");
	CHKSetStmtAttr(SQL_ROWSET_SIZE, (SQLPOINTER) int2ptr(1), 0, "S");

	/* now check that SQLExtendedFetch works as expected */
	odbc_command("CREATE TABLE #rowset(n INTEGER, c VARCHAR(20))");
	for (i = 0; i < 10; ++i) {
		char s[10];
		char sql[128];

		memset(s, 'a' + i, 9);
		s[9] = 0;
		sprintf(sql, "INSERT INTO #rowset(n,c) VALUES(%d,'%s')", i+1, s);
		odbc_command(sql);
	}

	odbc_reset_statement();
	CHKSetStmtOption(SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_DYNAMIC, "S");
	CHKExecDirect(T("SELECT * FROM #rowset ORDER BY n"), SQL_NTS, "SI");

	CHKBindCol(2, SQL_C_CHAR, buf, sizeof(buf), &len, "S");

	row_count = 0xdeadbeef;
	memset(statuses, 0x55, sizeof(statuses));
	CHKExtendedFetch(SQL_FETCH_NEXT, 1, &row_count, statuses, "S");

	if (row_count != 1 || statuses[0] != SQL_ROW_SUCCESS || strcmp(buf, "aaaaaaaaa") != 0) {
		fprintf(stderr, "Invalid result\n");
		odbc_disconnect();
		return 1;
	}

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
