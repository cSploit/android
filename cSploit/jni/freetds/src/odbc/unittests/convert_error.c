/*
 * test SQLBindParameter with text and Sybase 
 * test from Keith Woodard (bug #885122)
 */
#include "common.h"

static char software_version[] = "$Id: convert_error.c,v 1.12 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int test_num = 0;

static void
Test(const char *bind1, SQLSMALLINT type1, const char *bind2, SQLSMALLINT type2)
{
	char sql[512];
	char *val = "test";
	SQLLEN ind = 4;
	int id = 1;
	ODBC_BUF *odbc_buf = NULL;

	SQLFreeStmt(odbc_stmt, SQL_RESET_PARAMS);

	++test_num;
	sprintf(sql, "insert into #test_output values (%s, %s)", bind1, bind2);

	CHKPrepare(T(sql), strlen(sql), "S");
	if (bind1[0] == '?')
		CHKBindParameter(id++, SQL_PARAM_INPUT, SQL_C_LONG, type1, 3, 0, &test_num, 0, &ind, "S");
	if (bind2[0] == '?')
		CHKBindParameter(id++, SQL_PARAM_INPUT, SQL_C_CHAR, type2, strlen(val) + 1, 0, (SQLCHAR *) val,
					 0, &ind, "S");
	CHKExecute("S");
	ODBC_FREE();
}

int
main(int argc, char **argv)
{
	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("create table #test_output (id int, msg text)");

	Test("?", SQL_INTEGER, "?", SQL_LONGVARCHAR);
	Test("123", SQL_INTEGER, "?", SQL_LONGVARCHAR);
	Test("?", SQL_INTEGER, "'foo'", SQL_LONGVARCHAR);
	Test("?", SQL_INTEGER, "?", SQL_VARCHAR);

	/*
	 * Sybase cannot pass this test without complicated query parsing.
	 * Query with blob columns cannot be prepared so prepared query must
	 * be emulated loosing column informations from server and Sybase do
	 * not convert implicitly VARCHAR to INT
	 */
	if (odbc_db_is_microsoft())
		Test("?", SQL_VARCHAR, "?", SQL_LONGVARCHAR);
	else
		++test_num;

	odbc_disconnect();

	return 0;
}
