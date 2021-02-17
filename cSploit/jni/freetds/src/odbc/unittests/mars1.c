#include "common.h"

/* first MARS test, test 2 concurrent recordset */

#define SET_STMT(n) do { \
	if (pcur_stmt != &n) { \
		if (pcur_stmt) *pcur_stmt = odbc_stmt; \
		pcur_stmt = &n; \
		odbc_stmt = *pcur_stmt; \
	} \
} while(0)

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


static void
my_attrs(void)
{
	SQLSetConnectAttr(odbc_conn, 1224 /*SQL_COPT_SS_MARS_ENABLED*/, (SQLPOINTER) 1 /*SQL_MARS_ENABLED_YES*/, SQL_IS_UINTEGER);
}

int
main(int argc, char *argv[])
{
	SQLINTEGER len, out;
	int i;
	SQLHSTMT stmt1, stmt2;
	SQLHSTMT *pcur_stmt = NULL;

	odbc_use_version3 = 1;
	odbc_set_conn_attr = my_attrs;
	odbc_connect();

	stmt1 = odbc_stmt;

	out = 0;
	len = sizeof(out);
	CHKGetConnectAttr(1224, (SQLPOINTER) &out, sizeof(out), &len, "S");

	/* test we really support MARS on this connection */
	/* TODO should out be correct ?? */
	printf("Following row can contain an error due to MARS detection (is expected)\n");
	if (!out || odbc_command2("BEGIN TRANSACTION", "SNoE") != SQL_ERROR) {
		printf("MARS not supported for this connection\n");
		odbc_disconnect();
		return 0;
	}
	odbc_read_error();
	if (!strstr(odbc_err, "MARS")) {
		printf("Error message invalid \"%s\"\n", odbc_err);
		return 1;
	}

	/* create a test table with some data */
	odbc_command("create table #mars1 (n int, v varchar(100))");
	for (i = 0; i < 60; ++i) {
		char cmd[120], buf[80];
		memset(buf, 'a' + (i % 26), sizeof(buf));
		buf[i * 7 % 73] = 0;
		sprintf(cmd, "insert into #mars1 values(%d, '%s')", i, buf);
		odbc_command(cmd);
	}

	/* and another to avid locking problems */
	odbc_command("create table #mars2 (n int, v varchar(100))");

	AutoCommit(SQL_AUTOCOMMIT_OFF);

	/* try to do a select which return a lot of data (to test server didn't cache everything) */
	odbc_command("select a.n, b.n, a.v from #mars1 a, #mars1 b order by a.n, b.n");
	CHKFetch("S");

	/* try to do other commands */
	CHKAllocStmt(&stmt2, "S");
	SET_STMT(stmt2);
	odbc_command("insert into #mars2 values(1, 'foo')");
	SET_STMT(stmt1);

	CHKFetch("S");

	/* reset statements */
	odbc_reset_statement();
	SET_STMT(stmt2);
	odbc_reset_statement();

	/* now to 2 select with prepare/execute */
	CHKPrepare(T("select a.n, b.n, a.v from #mars1 a, #mars1 b order by a.n, b.n"), SQL_NTS, "S");
	SET_STMT(stmt1);
	CHKPrepare(T("select a.n, b.n, a.v from #mars1 a, #mars1 b order by a.n desc, b.n"), SQL_NTS, "S");
	SET_STMT(stmt2);
	CHKExecute("S");
	SET_STMT(stmt1);
	CHKExecute("S");
	SET_STMT(stmt2);
	CHKFetch("S");
	SET_STMT(stmt1);
	CHKFetch("S");
	SET_STMT(stmt2);
	CHKFetch("S");
	odbc_reset_statement();
	SET_STMT(stmt1);
	CHKFetch("S");
	odbc_reset_statement();

	EndTransaction(SQL_COMMIT);

	/* TODO test receiving large data should not take much memory */

	odbc_disconnect();
	return 0;
}

