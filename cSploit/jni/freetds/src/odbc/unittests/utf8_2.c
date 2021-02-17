#include "common.h"

/* test conversion of Hebrew characters (which have shift sequences) */
static char software_version[] = "$Id: utf8_2.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void init_connect(void);

static void
init_connect(void)
{
	CHKAllocEnv(&odbc_env, "S");
	SQLSetEnvAttr(odbc_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) (SQL_OV_ODBC3), SQL_IS_UINTEGER);
	CHKAllocConnect(&odbc_conn, "S");
}

static const char * const strings[] = {
	"\xd7\x9e\xd7\x99\xd7\x93\xd7\xa2",
	"info",
	"\xd7\x98\xd7\xa7\xd7\xa1\xd7\x98",
	"\xd7\x90\xd7\x91\xd7\x9b",
	NULL
};

/* same strings in hex */
static const char * const strings_hex[] = {
	"0xde05d905d305e205",
	"0x69006e0066006f00",
	"0xd805e705e105d805",
	"0xd005d105db05",
	NULL
};

int
main(int argc, char *argv[])
{
	char tmp[512];
	char out[32];
	SQLLEN n_len;
	SQLSMALLINT len;
	const char * const*p;
	int n;

	if (odbc_read_login_info())
		exit(1);

	/* connect string using DSN */
	init_connect();
	sprintf(tmp, "DSN=%s;UID=%s;PWD=%s;DATABASE=%s;ClientCharset=UTF-8;", odbc_server, odbc_user, odbc_password, odbc_database);
	CHKDriverConnect(NULL, T(tmp), SQL_NTS, (SQLTCHAR *) tmp, sizeof(tmp)/sizeof(SQLTCHAR), &len, SQL_DRIVER_NOPROMPT, "SI");
	if (!odbc_driver_is_freetds()) {
		odbc_disconnect();
		printf("Driver is not FreeTDS, exiting\n");
		return 0;
	}

	if (!odbc_db_is_microsoft() || odbc_db_version_int() < 0x08000000u) {
		odbc_disconnect();
		printf("Test for MSSQL only\n");
		return 0;
	}

	CHKAllocStmt(&odbc_stmt, "S");

	/* create test table */
	odbc_command("CREATE TABLE #tmpHebrew (i INT, v VARCHAR(10) COLLATE Hebrew_CI_AI)");

	/* insert with INSERT statements */
	for (n = 0, p = strings_hex; p[n]; ++n) {
		sprintf(tmp, "INSERT INTO #tmpHebrew VALUES(%d, CAST(%s AS NVARCHAR(10)))", n+1, p[n]);
		odbc_command(tmp);
	}

	/* test conversions in libTDS */
	odbc_command("SELECT v FROM #tmpHebrew");

	/* insert with SQLPrepare/SQLBindParameter/SQLExecute */
	CHKBindCol(1, SQL_C_CHAR, out, sizeof(out), &n_len, "S");
	for (n = 0, p = strings; p[n]; ++n) {
		CHKFetch("S");
		if (n_len != strlen(p[n]) || strcmp(p[n], out) != 0) {
			fprintf(stderr, "Wrong row %d %s\n", n, out);
			odbc_disconnect();
			return 1;
		}
	}

	odbc_disconnect();
	printf("Done.\n");
	return 0;
}

