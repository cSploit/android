#include "common.h"

static char software_version[] = "$Id: earlybind.c,v 1.5 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLINTEGER id;
	SQLLEN ind1, ind2;
	char name[64];

	odbc_connect();

	odbc_command("CREATE TABLE #test(id INT, name VARCHAR(100))");
	odbc_command("INSERT INTO #test(id, name) VALUES(8, 'sysobjects')");

	/* bind before select */
	SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &id, sizeof(SQLINTEGER), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, name, sizeof(name), &ind2);

	/* do select */
	odbc_command("SELECT id, name FROM #test WHERE name = 'sysobjects' SELECT 123, 'foo'");

	/* get results */
	id = -1;
	memset(name, 0, sizeof(name));
	SQLFetch(odbc_stmt);

	if (id == -1 || strcmp(name, "sysobjects") != 0) {
		fprintf(stderr, "wrong results\n");
		return 1;
	}

	/* discard others data */
	SQLFetch(odbc_stmt);

	SQLMoreResults(odbc_stmt);

	id = -1;
	memset(name, 0, sizeof(name));
	SQLFetch(odbc_stmt);

	if (id != 123 || strcmp(name, "foo") != 0) {
		fprintf(stderr, "wrong results\n");
		return 1;
	}

	/* discard others data */
	SQLFetch(odbc_stmt);

	SQLMoreResults(odbc_stmt);

	/* other select */
	odbc_command("SELECT 321, 'minni'");

	/* get results */
	id = -1;
	memset(name, 0, sizeof(name));
	SQLFetch(odbc_stmt);

	if (id != 321 || strcmp(name, "minni") != 0) {
		fprintf(stderr, "wrong results\n");
		return 1;
	}

	/* discard others data */
	SQLFetch(odbc_stmt);

	SQLMoreResults(odbc_stmt);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
