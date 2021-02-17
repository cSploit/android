#include "common.h"
#include <assert.h>

/* Test transaction types */

static char software_version[] = "$Id: transaction2.c,v 1.11 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
ReadErrorConn(void)
{
	ODBC_BUF *odbc_buf = NULL;
	SQLTCHAR *err = ODBC_GET(sizeof(odbc_err)*sizeof(SQLTCHAR));
	SQLTCHAR *state = ODBC_GET(sizeof(odbc_sqlstate)*sizeof(SQLTCHAR));

	memset(odbc_err, 0, sizeof(odbc_err));
	memset(odbc_sqlstate, 0, sizeof(odbc_sqlstate));
	CHKGetDiagRec(SQL_HANDLE_DBC, odbc_conn, 1, state, NULL, err, sizeof(odbc_err), NULL, "SI");
	strcpy(odbc_err, C(err));
	strcpy(odbc_sqlstate, C(state));
	ODBC_FREE();
	printf("Message: '%s' %s\n", odbc_sqlstate, odbc_err);
}

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

#define SWAP(t,a,b) do { t xyz = a; a = b; b = xyz; } while(0)
#define SWAP_CONN() do { SWAP(HENV,env,odbc_env); SWAP(HDBC,dbc,odbc_conn); SWAP(HSTMT,stmt,odbc_stmt);} while(0)

static HENV env = SQL_NULL_HENV;
static HDBC dbc = SQL_NULL_HDBC;
static HSTMT stmt = SQL_NULL_HSTMT;

static int
CheckDirtyRead(void)
{
	SQLRETURN RetCode;

	/* transaction 1 try to change a row but not commit */
	odbc_command("UPDATE test_transaction SET t = 'second' WHERE n = 1");

	SWAP_CONN();

	/* second transaction try to fetch uncommited row */
	RetCode = odbc_command2("SELECT * FROM test_transaction WHERE t = 'second' AND n = 1", "SE");
	if (RetCode == SQL_ERROR) {
		EndTransaction(SQL_ROLLBACK);
		SWAP_CONN();
		EndTransaction(SQL_ROLLBACK);
		return 0;	/* no dirty read */
	}

	CHKFetch("S");
	CHKFetch("No");
	SQLMoreResults(odbc_stmt);
	EndTransaction(SQL_ROLLBACK);
	SWAP_CONN();
	EndTransaction(SQL_ROLLBACK);
	return 1;
}

static int
CheckNonrepeatableRead(void)
{
	SQLRETURN RetCode;

	/* transaction 2 read a row */
	SWAP_CONN();
	odbc_command("SELECT * FROM test_transaction WHERE t = 'initial' AND n = 1");
	SQLMoreResults(odbc_stmt);

	/* transaction 1 change a row and commit */
	SWAP_CONN();
	RetCode = odbc_command2("UPDATE test_transaction SET t = 'second' WHERE n = 1", "SE");
	if (RetCode == SQL_ERROR) {
		EndTransaction(SQL_ROLLBACK);
		SWAP_CONN();
		EndTransaction(SQL_ROLLBACK);
		SWAP_CONN();
		return 0;	/* no dirty read */
	}
	EndTransaction(SQL_COMMIT);

	SWAP_CONN();

	/* second transaction try to fetch commited row */
	odbc_command("SELECT * FROM test_transaction WHERE t = 'second' AND n = 1");

	CHKFetch("S");
	CHKFetch("No");
	SQLMoreResults(odbc_stmt);
	EndTransaction(SQL_ROLLBACK);
	SWAP_CONN();
	odbc_command("UPDATE test_transaction SET t = 'initial' WHERE n = 1");
	EndTransaction(SQL_COMMIT);
	return 1;
}

static int
CheckPhantom(void)
{
	SQLRETURN RetCode;

	/* transaction 2 read a row */
	SWAP_CONN();
	odbc_command("SELECT * FROM test_transaction WHERE t = 'initial'");
	SQLMoreResults(odbc_stmt);

	/* transaction 1 insert a row that match critera */
	SWAP_CONN();
	RetCode = odbc_command2("INSERT INTO test_transaction(n, t) VALUES(2, 'initial')", "SE");
	if (RetCode == SQL_ERROR) {
		EndTransaction(SQL_ROLLBACK);
		SWAP_CONN();
		EndTransaction(SQL_ROLLBACK);
		SWAP_CONN();
		return 0;	/* no dirty read */
	}
	EndTransaction(SQL_COMMIT);

	SWAP_CONN();

	/* second transaction try to fetch commited row */
	odbc_command("SELECT * FROM test_transaction WHERE t = 'initial'");

	CHKFetch("S");
	CHKFetch("S");
	CHKFetch("No");
	SQLMoreResults(odbc_stmt);
	EndTransaction(SQL_ROLLBACK);
	SWAP_CONN();
	odbc_command("DELETE test_transaction WHERE n = 2");
	EndTransaction(SQL_COMMIT);
	return 1;
}

static int test_with_connect = 0;

static int global_txn;

static int hide_error;

static void
my_attrs(void)
{
	CHKSetConnectAttr(SQL_ATTR_TXN_ISOLATION, int2ptr(global_txn), 0, "S");
	AutoCommit(SQL_AUTOCOMMIT_OFF);
}

static void
ConnectWithTxn(int txn)
{
	global_txn = txn;
	odbc_set_conn_attr = my_attrs;
	odbc_connect();
	odbc_set_conn_attr = NULL;
}

static int
Test(int txn, const char *expected)
{
	int dirty, repeatable, phantom;
	char buf[128];

	SWAP_CONN();
	if (test_with_connect) {
		odbc_disconnect();
		ConnectWithTxn(txn);
		CHKSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER) 2, 0, "S");
	} else {
		CHKSetConnectAttr(SQL_ATTR_TXN_ISOLATION, int2ptr(txn), 0, "S");
	}
	SWAP_CONN();

	dirty = CheckDirtyRead();
	repeatable = CheckNonrepeatableRead();
	phantom = CheckPhantom();

	sprintf(buf, "dirty %d non repeatable %d phantom %d", dirty, repeatable, phantom);
	if (strcmp(buf, expected) != 0) {
		if (hide_error) {
			hide_error = 0;
			return 0;
		}
		fprintf(stderr, "detected wrong TXN\nexpected '%s' got '%s'\n", expected, buf);
		exit(1);
	}
	hide_error = 0;
	return 1;
}

int
main(int argc, char *argv[])
{
	odbc_use_version3 = 1;
	odbc_connect();

	/* Invalid argument value */
	CHKSetConnectAttr(SQL_ATTR_TXN_ISOLATION, int2ptr(SQL_TXN_REPEATABLE_READ | SQL_TXN_READ_COMMITTED), 0, "E");
	ReadErrorConn();
	if (strcmp(odbc_sqlstate, "HY024") != 0) {
		odbc_disconnect();
		fprintf(stderr, "Unexpected success\n");
		return 1;
	}

	/* here we can't use temporary table cause we use two connection */
	odbc_command("IF OBJECT_ID('test_transaction') IS NOT NULL DROP TABLE test_transaction");
	odbc_command("CREATE TABLE test_transaction(n NUMERIC(18,0) PRIMARY KEY, t VARCHAR(30))");

	CHKSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER) 2, 0, "S");

	AutoCommit(SQL_AUTOCOMMIT_OFF);
	odbc_command("INSERT INTO test_transaction(n, t) VALUES(1, 'initial')");

#ifdef ENABLE_DEVELOPING
	/* test setting with active transaction "Operation invalid at this time" */
	CHKSetConnectAttr(SQL_ATTR_TXN_ISOLATION, int2ptr(SQL_TXN_REPEATABLE_READ), 0, "E");
	ReadErrorConn();
	if (strcmp(odbc_sqlstate, "HY011") != 0) {
		odbc_disconnect();
		fprintf(stderr, "Unexpected success\n");
		return 1;
	}
#endif

	EndTransaction(SQL_COMMIT);

	odbc_command("SELECT * FROM test_transaction");

	/* test setting with pending data */
	CHKSetConnectAttr(SQL_ATTR_TXN_ISOLATION, int2ptr(SQL_TXN_REPEATABLE_READ), 0, "E");
	ReadErrorConn();
	if (strcmp(odbc_sqlstate, "HY011") != 0) {
		odbc_disconnect();
		fprintf(stderr, "Unexpected success\n");
		return 1;
	}

	SQLMoreResults(odbc_stmt);

	EndTransaction(SQL_COMMIT);


	/* save this connection and do another */
	SWAP_CONN();

	odbc_connect();

	CHKSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER) 2, 0, "S");
	AutoCommit(SQL_AUTOCOMMIT_OFF);

	SWAP_CONN();

	for (test_with_connect = 0; test_with_connect <= 1; ++test_with_connect) {
		Test(SQL_TXN_READ_UNCOMMITTED, "dirty 1 non repeatable 1 phantom 1");
		Test(SQL_TXN_READ_COMMITTED, "dirty 0 non repeatable 1 phantom 1");
		if (odbc_db_is_microsoft()) {
			Test(SQL_TXN_REPEATABLE_READ, "dirty 0 non repeatable 0 phantom 1");
		} else {
			hide_error = 1;
			if (!Test(SQL_TXN_REPEATABLE_READ, "dirty 0 non repeatable 0 phantom 1"))
				Test(SQL_TXN_REPEATABLE_READ, "dirty 0 non repeatable 0 phantom 0");
		}
		Test(SQL_TXN_SERIALIZABLE, "dirty 0 non repeatable 0 phantom 0");
	}

	odbc_disconnect();

	SWAP_CONN();

	EndTransaction(SQL_COMMIT);

	/* Sybase do not accept DROP TABLE during a transaction */
	AutoCommit(SQL_AUTOCOMMIT_ON);
	odbc_command("DROP TABLE test_transaction");

	odbc_disconnect();
	return 0;
}
