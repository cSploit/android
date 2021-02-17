#include "common.h"

/* test error on prepared statement, from Nathaniel Talbott test */

static char software_version[] = "$Id: preperror.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLLEN cbInString = SQL_NTS;
	char buf[256];
	SQLTCHAR msg[256];
	SQLTCHAR sqlstate[6];

	odbc_connect();

	odbc_command("CREATE TABLE #urls ( recdate DATETIME ) ");

	/* test implicit conversion error */
	CHKExecDirect(T("INSERT INTO #urls ( recdate ) VALUES ( '2003-10-1 10:11:1 0' )"), SQL_NTS, "E");

	/* test prepared implicit conversion error */
	CHKPrepare(T("INSERT INTO #urls ( recdate ) VALUES ( ? )"), SQL_NTS, "SI");

	strcpy(buf, "2003-10-1 10:11:1 0");
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 128, 0, buf, sizeof(buf), &cbInString, "SI");

	CHKExecute("E");

	CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, 1, sqlstate, NULL, msg, ODBC_VECTOR_SIZE(buf), NULL, "SI");
	printf("err=%s\n", C(msg));

	/* assure initial state */
	odbc_reset_statement();

	/* try to prepare and execute a statement with error (from DBD::ODBC test) */
	if (CHKPrepare(T("SELECT XXNOTCOLUMN FROM sysobjects"), SQL_NTS, "SE") == SQL_SUCCESS)
		CHKExecute("E");

	CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, 1, sqlstate, NULL, msg, ODBC_VECTOR_SIZE(buf), NULL, "SI");
	printf("err=%s\n", C(msg));


	odbc_disconnect();

	printf("Done.\n");
	ODBC_FREE();
	return 0;
}
