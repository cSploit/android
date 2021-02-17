#undef NDEBUG
#include "common.h"
#include <assert.h>

/*
	Test for a bug executing after a not successfully execute
*/

int
main(int argc, char *argv[])
{
	SQLSMALLINT num_params;
	SQLLEN sql_nts = SQL_NTS;
	char string[20];
	SQLINTEGER id;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("create table #tester (id int not null primary key, name varchar(20) not null)");
	odbc_command("insert into #tester(id, name) values(1, 'abc')");
	odbc_command("insert into #tester(id, name) values(2, 'duck')");

	odbc_reset_statement();

	CHKPrepare(T("insert into #tester(id, name) values(?,?)"), SQL_NTS, "S");

	CHKR(SQLNumParams, (odbc_stmt, &num_params), "S");
	assert(num_params == 2);

	/* now this is going to fail as id is duplicated, causing statement to not be prepared */
	id = 1;
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, &sql_nts, "S");
	strcpy(string, "test");
	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, sizeof(string), 0, string, 0, &sql_nts, "S");
	CHKExecute("E");

	/* this should success */
	id = 4;
	strcpy(string, "test2");
	CHKExecute("S");

	odbc_disconnect();
	return 0;
}

