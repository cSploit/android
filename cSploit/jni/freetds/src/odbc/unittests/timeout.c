#include "common.h"
#include <assert.h>

/* Test timeout of query */

static char software_version[] = "$Id: timeout.c,v 1.14 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
AutoCommit(int onoff)
{
	CHKSetConnectAttr(SQL_ATTR_AUTOCOMMIT, int2ptr(onoff), 0, "S");
}

static void
EndTransaction(SQLSMALLINT type)
{
	CHKEndTran(SQL_HANDLE_DBC, odbc_conn, type, "S");
}

int
main(int argc, char *argv[])
{
	HENV env;
	HDBC dbc;
	HSTMT stmt;
	SQLINTEGER i;

	odbc_connect();

	/* here we can't use temporary table cause we use two connection */
	odbc_command_with_result(odbc_stmt, "drop table test_timeout");
	odbc_command("create table test_timeout(n numeric(18,0) primary key, t varchar(30))");
	AutoCommit(SQL_AUTOCOMMIT_OFF);

	odbc_command("insert into test_timeout(n, t) values(1, 'initial')");
	EndTransaction(SQL_COMMIT);

	odbc_command("update test_timeout set t = 'second' where n = 1");

	/* save this connection and do another */
	env = odbc_env;
	dbc = odbc_conn;
	stmt = odbc_stmt;
	odbc_env = SQL_NULL_HENV;
	odbc_conn = SQL_NULL_HDBC;
	odbc_stmt = SQL_NULL_HSTMT;

	odbc_connect();

	AutoCommit(SQL_AUTOCOMMIT_OFF);
	CHKSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER) 2, 0, "S");

	i = 1;
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &i, 0, NULL, "S");

	CHKPrepare(T("update test_timeout set t = 'bad' where n = ?"), SQL_NTS, "S");
	CHKExecute("E");
	EndTransaction(SQL_ROLLBACK);

	/* TODO should return error S1T00 Timeout expired, test error message */
	odbc_command2("update test_timeout set t = 'bad' where n = 1", "E");

	EndTransaction(SQL_ROLLBACK);

	odbc_disconnect();

	odbc_env = env;
	odbc_conn = dbc;
	odbc_stmt = stmt;

	EndTransaction(SQL_COMMIT);

	/* Sybase do not accept DROP TABLE during a transaction */
	AutoCommit(SQL_AUTOCOMMIT_ON);
	odbc_command("drop table test_timeout");

	odbc_disconnect();

	return 0;
}
