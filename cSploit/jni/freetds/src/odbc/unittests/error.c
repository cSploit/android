#include "common.h"

/* some tests on error reporting */

static char software_version[] = "$Id: error.c,v 1.11 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLRETURN RetCode;
	HSTMT stmt;

	odbc_connect();

	/* create a test table */
	odbc_command("create table #tmp (i int)");
	odbc_command("insert into #tmp values(3)");
	odbc_command("insert into #tmp values(4)");
	odbc_command("insert into #tmp values(5)");
	odbc_command("insert into #tmp values(6)");
	odbc_command("insert into #tmp values(7)");

	/* issue our command */
	RetCode = odbc_command2("select 100 / (i - 5) from #tmp order by i", "SE");

	/* special case, early Sybase detect error early */
	if (RetCode != SQL_ERROR) {

		/* TODO when multiple row fetch available test for error on some columns */
		CHKFetch("S");
		CHKFetch("S");
		CHKFetch("E");
	}

	odbc_read_error();
	if (!strstr(odbc_err, "zero")) {
		fprintf(stderr, "Message invalid\n");
		return 1;
	}

	SQLFetch(odbc_stmt);
	SQLFetch(odbc_stmt);
	SQLFetch(odbc_stmt);
	SQLMoreResults(odbc_stmt);

	CHKAllocStmt(&stmt, "S");

	odbc_command("SELECT * FROM sysobjects");

	odbc_stmt = stmt;

	/* a statement is already active so you get error... */
	if (odbc_command2("SELECT *  FROM sysobjects", "SE") == SQL_SUCCESS) {
		SQLMoreResults(odbc_stmt);
		/* ...or we are using MARS! */
		odbc_command2("BEGIN TRANSACTION", "E");
	}

	odbc_read_error();

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
