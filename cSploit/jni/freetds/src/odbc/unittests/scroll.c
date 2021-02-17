#include "common.h"

/* Test cursors */

static char software_version[] = "$Id: scroll.c,v 1.11 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
#define ROWS 3
#define C_LEN 10

	SQLUINTEGER n[ROWS];
	char c[ROWS][C_LEN];
	SQLLEN c_len[ROWS], n_len[ROWS];

	SQLUSMALLINT statuses[ROWS];
	SQLUSMALLINT i;
	SQLULEN num_row;
	int i_test;

	typedef struct _test
	{
		SQLUSMALLINT type;
		SQLINTEGER irow;
		int start;
		int num;
	} TEST;

	static const TEST tests[] = {
		{SQL_FETCH_NEXT, 0, 1, 3},
		{SQL_FETCH_NEXT, 0, 4, 2},
		{SQL_FETCH_PRIOR, 0, 1, 3},
		{SQL_FETCH_NEXT, 0, 4, 2},
		{SQL_FETCH_NEXT, 0, -1, -1},
		{SQL_FETCH_FIRST, 0, 1, 3},
		{SQL_FETCH_ABSOLUTE, 3, 3, 3},
		{SQL_FETCH_RELATIVE, 1, 4, 2},
		{SQL_FETCH_LAST, 0, 3, 3}
	};
	const int num_tests = sizeof(tests) / sizeof(TEST);

	odbc_use_version3 = 1;

	odbc_connect();
	odbc_check_cursor();

	/* create test table */
	odbc_command("IF OBJECT_ID('tempdb..#test') IS NOT NULL DROP TABLE #test");
	odbc_command("CREATE TABLE #test(i int, c varchar(6))");
	odbc_command("INSERT INTO #test(i, c) VALUES(1, 'a')");
	odbc_command("INSERT INTO #test(i, c) VALUES(2, 'bb')");
	odbc_command("INSERT INTO #test(i, c) VALUES(3, 'ccc')");
	odbc_command("INSERT INTO #test(i, c) VALUES(4, 'dddd')");
	odbc_command("INSERT INTO #test(i, c) VALUES(5, 'eeeee')");

	/* set cursor options */
	odbc_reset_statement();
	CHKSetStmtAttr(SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_DYNAMIC, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) ROWS, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER) statuses, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR, &num_row, 0, "S");

	/* */
	CHKExecDirect(T("SELECT i, c FROM #test"), SQL_NTS, "S");

	/* bind some rows at a time */
	CHKBindCol(1, SQL_C_ULONG, n, 0, n_len, "S");
	CHKBindCol(2, SQL_C_CHAR, c, C_LEN, c_len, "S");

	for (i_test = 0; i_test < num_tests; ++i_test) {
		const TEST *t = &tests[i_test];

		printf("Test %d\n", i_test + 1);

		if (t->start == -1) {
			CHKFetchScroll(t->type, t->irow, "No");
		} else {
			CHKFetchScroll(t->type, t->irow, "S");

			if (t->start < 1) {
				fprintf(stderr, "Rows not expected\n");
				exit(1);
			}

			/* print, just for debug */
			for (i = 0; i < num_row; ++i)
				printf("row %d i %d c %s\n", (int) (i + 1), (int) n[i], c[i]);
			printf("---\n");

			if (num_row != t->num) {
				fprintf(stderr, "Expected %d rows, got %d\n", t->num, (int) num_row);
				exit(1);
			}

			for (i = 0; i < num_row; ++i) {
				char name[10];

				memset(name, 0, sizeof(name));
				memset(name, 'a' - 1 + i + t->start, i + t->start);
				if (n[i] != i + t->start || c_len[i] != strlen(name) || strcmp(c[i], name) != 0) {
					fprintf(stderr, "Wrong row returned\n");
					fprintf(stderr, "\tn %d %d\n", (int) n[i], i + t->start);
					fprintf(stderr, "\tc len %d %d\n", (int) c_len[i], (int) strlen(name));
					fprintf(stderr, "\tc %s %s\n", c[i], name);
					exit(1);
				}
			}
		}
	}

	odbc_reset_statement();

	odbc_disconnect();
	return 0;
}
