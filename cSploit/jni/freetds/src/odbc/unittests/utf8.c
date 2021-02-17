#include "common.h"

/* test binding with UTF-8 encoding */
static char software_version[] = "$Id: utf8.c,v 1.13 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void init_connect(void);

static void
init_connect(void)
{
	CHKAllocEnv(&odbc_env, "S");
	SQLSetEnvAttr(odbc_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) (SQL_OV_ODBC3), SQL_IS_UINTEGER);
	CHKAllocConnect(&odbc_conn, "S");
}

static void
CheckNoRow(const char *query)
{
	SQLRETURN rc;

	rc = CHKExecDirect(T(query), SQL_NTS, "SINo");
	if (rc == SQL_NO_DATA)
		return;

	do {
		SQLSMALLINT cols;

		CHKNumResultCols(&cols, "S");
		if (cols != 0) {
			fprintf(stderr, "Data not expected here, query:\n\t%s\n", query);
			exit(1);
		}
	} while (CHKMoreResults("SNo") == SQL_SUCCESS);
}

/* test table name, it contains two japanese characters */
static const char table_name[] = "mytab\xe7\x8e\x8b\xe9\xb4\xbb";

static const char * const strings[] = {
	/* ascii */
	"aaa", "aaa",
	/* latin 1*/
	"abc\xc3\xa9\xc3\xa1\xc3\xb4", "abc\xc3\xa9\xc3\xae\xc3\xb4",
	/* Japanese... */
	"abc\xe7\x8e\x8b\xe9\xb4\xbb", "abc\xe7\x8e\x8b\xe9\xb4\xbb\xe5\x82\x91\xe7\x8e\x8b\xe9\xb4\xbb\xe5\x82\x91",
	NULL, NULL
};

/* same strings in hex */
static const char * const strings_hex[] = {
	/* ascii */
	"0x610061006100", "0x610061006100",
	/* latin 1*/
	"0x610062006300e900e100f400", "0x610062006300e900ee00f400",
	/* Japanese... */
	"0x6100620063008b733b9d", "0x6100620063008b733b9d91508b733b9d9150",
	NULL, NULL
};

static char tmp[2048];

static void
TestBinding(int minimun)
{
	const char * const*p;
	SQLINTEGER n;
	SQLLEN n_len;

	sprintf(tmp, "DELETE FROM %s", table_name);
	odbc_command(tmp);

	/* insert with SQLPrepare/SQLBindParameter/SQLExecute */
	sprintf(tmp, "INSERT INTO %s VALUES(?,?,?)", table_name);
	CHKPrepare(T(tmp), SQL_NTS, "S");
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG,
		SQL_INTEGER, 0, 0, &n, 0, &n_len, "S");
	n_len = sizeof(n);
	
	for (n = 1, p = strings; p[0] && p[1]; p += 2, ++n) {
		SQLLEN s1_len, s2_len;
		unsigned int len;

		len = minimun ? (strlen(strings_hex[p-strings]) - 2) /4 : 40;
		CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_CHAR,
			SQL_WCHAR, len, 0, (void *) p[0], 0, &s1_len, "S");
		len = minimun ? (strlen(strings_hex[p+1-strings]) - 2) /4 : 40;
		/* FIXME this with SQL_VARCHAR produce wrong protocol data */
		CHKBindParameter(3, SQL_PARAM_INPUT, SQL_C_CHAR,
			SQL_WVARCHAR, len, 0, (void *) p[1], 0, &s2_len, "S");
		s1_len = strlen(p[0]);
		s2_len = strlen(p[1]);
		printf("insert #%d\n", (int) n);
		CHKExecute("S");
	}

	/* check rows */
	for (n = 1, p = strings_hex; p[0] && p[1]; p += 2, ++n) {
		sprintf(tmp, "IF NOT EXISTS(SELECT * FROM %s WHERE k = %d AND c = %s AND vc = %s) SELECT 1", table_name, (int) n, p[0], p[1]);
		CheckNoRow(tmp);
	}

	odbc_reset_statement();
}

int
main(int argc, char *argv[])
{
	SQLSMALLINT len;
	const char * const*p;
	SQLINTEGER n;

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
	sprintf(tmp, "IF OBJECT_ID(N'%s') IS NOT NULL DROP TABLE %s", table_name, table_name);
	odbc_command(tmp);
	sprintf(tmp, "CREATE TABLE %s (k int, c NCHAR(10), vc NVARCHAR(10))", table_name);
	odbc_command(tmp);

	/* insert with INSERT statements */
	for (n = 1, p = strings; p[0] && p[1]; p += 2, ++n) {
		sprintf(tmp, "INSERT INTO %s VALUES (%d,N'%s',N'%s')", table_name, (int) n, p[0], p[1]);
		odbc_command(tmp);
	}

	/* check rows */
	for (n = 1, p = strings_hex; p[0] && p[1]; p += 2, ++n) {
		sprintf(tmp, "IF NOT EXISTS(SELECT * FROM %s WHERE k = %d AND c = %s AND vc = %s) SELECT 1", table_name, (int) n, p[0], p[1]);
		CheckNoRow(tmp);
	}

	TestBinding(0);

	TestBinding(1);

	/* cleanup */
	sprintf(tmp, "IF OBJECT_ID(N'%s') IS NOT NULL DROP TABLE %s", table_name, table_name);
	odbc_command(tmp);

	odbc_disconnect();
	printf("Done.\n");
	return 0;
}

