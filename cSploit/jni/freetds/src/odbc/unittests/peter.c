#undef NDEBUG
#include "common.h"
#include <assert.h>

/*
	Test SQLNumResultCols after SQLFreeStmt
	This test on Sybase should not raise an error
*/

static char software_version[] = "$Id: peter.c,v 1.2 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLSMALLINT num_params, cols;
	SQLLEN count;
	SQLINTEGER id;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("create table #tester (id int not null, name varchar(20) not null)");
	odbc_command("insert into #tester(id, name) values(1, 'abc')");
	odbc_command("insert into #tester(id, name) values(2, 'duck')");

	CHKPrepare(T("SELECT * FROM #tester WHERE id = ?"), SQL_NTS, "S");

	CHKR(SQLNumParams, (odbc_stmt, &num_params), "S");
	assert(num_params == 1);

	id = 1;
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, sizeof(id), NULL, "S");

	CHKExecute("S");

	CHKR(SQLFreeStmt, (odbc_stmt, SQL_RESET_PARAMS), "S");

	CHKRowCount(&count, "S");

	CHKNumResultCols(&cols, "S");
	assert(cols == 2);

	odbc_disconnect();
	return 0;
}

