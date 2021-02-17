#include "common.h"

/* Test for SQLMoreResults */

static char software_version[] = "$Id: t0004.c,v 1.19 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
Test(int use_indicator)
{
	char buf[128];
	SQLLEN ind;
	SQLLEN *pind = use_indicator ? &ind : NULL;

	strcpy(buf, "I don't exist");
	ind = strlen(buf);

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, buf, 128, pind, "S");

	CHKPrepare(T("SELECT id, name FROM master..sysobjects WHERE name = ?"), SQL_NTS, "S");

	CHKExecute("S");

	CHKFetch("No");

	CHKMoreResults("No");

	/* use same binding above */
	strcpy(buf, "sysobjects");
	ind = strlen(buf);

	CHKExecute("S");

	CHKFetch("S");

	CHKFetch("No");

	CHKMoreResults("No");

	ODBC_FREE();
}

int
main(int argc, char *argv[])
{
	odbc_connect();

	Test(1);
	Test(0);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
