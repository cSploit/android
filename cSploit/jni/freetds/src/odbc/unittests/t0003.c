#include "common.h"

/* Test for SQLMoreResults */

static char software_version[] = "$Id: t0003.c,v 1.21 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
DoTest(int prepared)
{
	odbc_command("create table #odbctestdata (i int)");

	/* test that 2 empty result set are returned correctly */
	if (!prepared) {
		odbc_command("select * from #odbctestdata select * from #odbctestdata");
	} else {
		CHKPrepare(T("select * from #odbctestdata select * from #odbctestdata"), SQL_NTS, "S");
		CHKExecute("S");
	}

	CHKFetch("No");

	CHKMoreResults("S");
	printf("Getting next recordset\n");

	CHKFetch("No");

	CHKMoreResults("No");

	/* test that skipping a no empty result go to other result set */
	odbc_command("insert into #odbctestdata values(123)");
	if (!prepared) {
		odbc_command("select * from #odbctestdata select * from #odbctestdata");
	} else {
		CHKPrepare(T("select * from #odbctestdata select * from #odbctestdata"), SQL_NTS, "S");
		CHKExecute("S");
	}

	CHKMoreResults("S");
	printf("Getting next recordset\n");

	CHKFetch("S");

	CHKFetch("No");

	CHKMoreResults("No");

	odbc_command("drop table #odbctestdata");

	ODBC_FREE();
}

int
main(int argc, char *argv[])
{
	odbc_connect();

	DoTest(0);
	DoTest(1);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
