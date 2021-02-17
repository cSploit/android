#include "common.h"

static char software_version[] = "$Id: norowset.c,v 1.9 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Test that a select following a store procedure execution return results */

int
main(int argc, char *argv[])
{
	char output[256];
	SQLLEN dataSize;

	odbc_connect();

	odbc_command_with_result(odbc_stmt, "drop proc sp_norowset_test");

	odbc_command("create proc sp_norowset_test as begin declare @i int end");

	odbc_command("exec sp_norowset_test");

	/* note, mssql 2005 seems to not return row for tempdb, use always master */
	odbc_command("select name from master..sysobjects where name = 'sysobjects'");
	CHKFetch("S");

	CHKGetData(1, SQL_C_CHAR, output, sizeof(output), &dataSize, "S");

	if (strcmp(output, "sysobjects") != 0) {
		printf("Unexpected result\n");
		exit(1);
	}

	CHKFetch("No");

	CHKMoreResults("No");

	odbc_command("drop proc sp_norowset_test");

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
