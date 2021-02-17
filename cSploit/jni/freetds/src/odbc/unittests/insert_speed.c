#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: insert_speed.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define SQL_QUERY_LENGTH 80

/* test correctly inserted data after insert */

/* I don't remember where this test came ... - freddy77 */

static void
insert_test_auto(void)
{
	SQLLEN sql_nts = SQL_NTS;
	SQLINTEGER id = 0;
	char string[64];

	odbc_reset_statement();

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, sizeof(id), 0, &id, 0, &sql_nts, "SI");
	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, sizeof(string), 0, string, 0, &sql_nts, "SI");

	CHKPrepare(T("insert into test values (?, ?)"), SQL_NTS, "SI");
	for (id = 0; id < 20; id++) {
		sprintf(string, "This is a test (%d)", (int) id);
		CHKExecute("SI");
	}

	odbc_reset_statement();
}


static void
insert_test_man(void)
{
	SQLLEN sql_nts = SQL_NTS;
	SQLINTEGER commit_off = SQL_AUTOCOMMIT_OFF;
	SQLINTEGER commit_on = SQL_AUTOCOMMIT_ON;
	SQLINTEGER id = 0;

	char string[64];

	CHKSetConnectAttr(SQL_ATTR_AUTOCOMMIT, int2ptr(commit_off), SQL_IS_INTEGER, "SI");

	odbc_reset_statement();

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, sizeof(id), 0, &id, 0, &sql_nts, "SI");
	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, sizeof(string), 0, string, 0, &sql_nts, "SI");

	CHKPrepare(T("insert into test values (?, ?)"), SQL_NTS, "SI");
	for (id = 0; id < 20; id++) {
		sprintf(string, "This is a test (%d)", (int) id);
		CHKExecute("SI");
	}

	SQLEndTran(SQL_HANDLE_DBC, odbc_conn, SQL_COMMIT);
	SQLSetConnectAttr(odbc_conn, SQL_ATTR_AUTOCOMMIT, int2ptr(commit_on), SQL_IS_INTEGER);
	odbc_reset_statement();
}

int
main(int argc, char **argv)
{
	odbc_connect();

	odbc_command_with_result(odbc_stmt, "DROP TABLE test");
	odbc_command("CREATE TABLE test(i int, c varchar(40))");

	insert_test_man();

	odbc_command("DELETE FROM test");

	insert_test_auto();

	odbc_command("DROP TABLE test");

	odbc_disconnect();

	return 0;
}
