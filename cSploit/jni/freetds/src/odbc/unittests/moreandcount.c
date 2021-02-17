#include "common.h"

/* Test for SQLMoreResults and SQLRowCount on batch */

static char software_version[] = "$Id: moreandcount.c,v 1.20 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
DoTest(int prepare)
{
	int n = 0;
	static const char query[] =
		/* on prepared this recordset should be skipped */
		"DECLARE @b INT SELECT @b = 25 "
		"SELECT * FROM #tmp1 WHERE i <= 3 "
		/* on prepare we cannot distinguish these recordset */
		"INSERT INTO #tmp2 SELECT * FROM #tmp1 WHERE i = 1 "
		"INSERT INTO #tmp2 SELECT * FROM #tmp1 WHERE i <= 3 "
		"SELECT * FROM #tmp1 WHERE i = 1 "
		/* but FreeTDS can detect last recordset */
		"UPDATE #tmp1 SET i=i+1 WHERE i >= 2";

	if (prepare) {
		CHKPrepare(T(query), SQL_NTS, "S");
		CHKExecute("S");
	} else {

		/* execute a batch command select insert insert select and check rows */
		CHKExecDirect(T(query), SQL_NTS, "S");
	}
	if (!prepare) {
		printf("Result %d\n", ++n);
		ODBC_CHECK_COLS(0);
		ODBC_CHECK_ROWS(1);
		CHKMoreResults("S");
	}
	printf("Result %d\n", ++n);
	ODBC_CHECK_COLS(1);
	ODBC_CHECK_ROWS(-1);
	CHKFetch("S");
	CHKFetch("S");
	ODBC_CHECK_COLS(1);
	ODBC_CHECK_ROWS(-1);
	CHKFetch("No");
	ODBC_CHECK_COLS(1);
	ODBC_CHECK_ROWS(2);
	CHKMoreResults("S");
	if (!prepare) {
		printf("Result %d\n", ++n);
		ODBC_CHECK_COLS(0);
		ODBC_CHECK_ROWS(1);
		CHKMoreResults("S");
		printf("Result %d\n", ++n);
		ODBC_CHECK_COLS(0);
		ODBC_CHECK_ROWS(2);
		CHKMoreResults("S");
	}
	printf("Result %d\n", ++n);
	ODBC_CHECK_COLS(1);
	ODBC_CHECK_ROWS(-1);
	CHKFetch("S");
	ODBC_CHECK_COLS(1);
	ODBC_CHECK_ROWS(-1);
	CHKFetch("No");
	ODBC_CHECK_COLS(1);
	if (prepare) {
		/* collapse 2 recordset... after a lot of testing this is the behavior! */
		if (odbc_driver_is_freetds())
			ODBC_CHECK_ROWS(2);
	} else {
		ODBC_CHECK_ROWS(1);
		CHKMoreResults("S");
		ODBC_CHECK_COLS(0);
		ODBC_CHECK_ROWS(2);
	}

	CHKMoreResults("No");
#ifndef TDS_NO_DM
	if (!prepare)
		ODBC_CHECK_COLS(-1);
	ODBC_CHECK_ROWS(-2);
#endif
	ODBC_FREE();
}

int
main(int argc, char *argv[])
{
	odbc_connect();

	odbc_command("create table #tmp1 (i int)");
	odbc_command("create table #tmp2 (i int)");
	odbc_command("insert into #tmp1 values(1)");
	odbc_command("insert into #tmp1 values(2)");
	odbc_command("insert into #tmp1 values(5)");

	printf("Use direct statement\n");
	DoTest(0);

	printf("Use prepared statement\n");
	DoTest(1);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
