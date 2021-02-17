#include "common.h"

/*
 * 1) Test cursor do not give error for statement that do not return rows
 * 2) Test cursor returns results on language RPCs
 */

static char software_version[] = "$Id: cursor2.c,v 1.12 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLTCHAR sqlstate[6];
	SQLTCHAR msg[256];

	odbc_connect();
	odbc_check_cursor();

	odbc_command("CREATE TABLE #cursor2_test (i INT)");

	odbc_reset_statement();
	CHKSetStmtAttr(SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_DYNAMIC, SQL_IS_INTEGER, "S");

	/* this should not fail or return warnings */
	odbc_command("DROP TABLE #cursor2_test");

	CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, 1, sqlstate, NULL, msg, ODBC_VECTOR_SIZE(msg), NULL, "No");


	odbc_reset_statement();
	odbc_command_with_result(odbc_stmt, "if object_id('sp_test') is not null drop proc sp_test");
	odbc_command("create proc sp_test @name varchar(30) as select 0 as pippo select 1 as 'test', @name as 'nome'");

	odbc_reset_statement();
	CHKSetStmtAttr(SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_STATIC, SQL_IS_INTEGER, "S");

	odbc_command(" exec sp_test 'ciao'");

	CHKFetch("S");

	odbc_reset_statement();
	odbc_command("drop proc sp_test");

	odbc_disconnect();
	
	return 0;
}
