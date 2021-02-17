#include "common.h"

static char software_version[] = "$Id: stats.c,v 1.4 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static SQLLEN cnamesize;
static char output[256];

static void
ReadCol(int i)
{
	memset(output, 'x', sizeof(output));
	strcpy(output, "NULL");
	CHKGetData(i, SQL_C_CHAR, output, sizeof(output), &cnamesize, "S");
}

static const char *catalog = NULL;
static const char *schema = NULL;
static const char *proc = "stat_proc";
static const char *table = "stat_proc";
static const char *column = "@t";

#define LEN(x) (x) ? strlen(x) : 0

static void
TestProc(const char *type, const char *expected)
{
	char sql[256];

	odbc_command("IF OBJECT_ID('stat_proc') IS NOT NULL DROP PROC stat_proc");

	sprintf(sql, "CREATE PROC stat_proc(@t %s) AS RETURN 0", type);
	odbc_command(sql);

	column = "@t";
	CHKProcedureColumns(T(catalog), LEN(catalog), T(schema), LEN(schema), T(proc), LEN(proc), T(column), LEN(column), "SI");

	CHKFetch("SI");

	ReadCol(6);
	if (strcmp(output, expected) != 0) {
		fprintf(stderr, "Got \"%s\" expected \"%s\"\n", output, expected);
		odbc_disconnect();
		exit(1);
	}

	CHKCloseCursor("SI");
	ODBC_FREE();
}

static void
TestTable(const char *type, const char *expected)
{
	char sql[256];

	odbc_command("IF OBJECT_ID('stat_t') IS NOT NULL DROP TABLE stat_t");

	sprintf(sql, "CREATE TABLE stat_t(t %s)", type);
	odbc_command(sql);

	column = "t";
	table = "stat_t";
	CHKColumns(T(catalog), LEN(catalog), T(schema), LEN(schema), T(table), LEN(table), T(column), LEN(column), "SI");

	CHKFetch("SI");

	ReadCol(5);
	if (strcmp(output, expected) != 0) {
		fprintf(stderr, "Got \"%s\" expected \"%s\"\n", output, expected);
		odbc_disconnect();
		exit(1);
	}

	CHKCloseCursor("SI");
	ODBC_FREE();
}


#define STR(n) str(int_buf, n)

static const char *
str(char *buf, int n)
{
	sprintf(buf, "%d", n);
	return buf;
}

int
main(int argc, char *argv[])
{
	char int_buf[32];

	odbc_use_version3 = 0;
	odbc_connect();

	TestProc("DATETIME", STR(SQL_TIMESTAMP));
	TestTable("DATETIME", STR(SQL_TIMESTAMP));

	odbc_disconnect();


	odbc_use_version3 = 1;
	odbc_connect();

	TestProc("DATETIME", STR(SQL_TYPE_TIMESTAMP));
	TestTable("DATETIME", STR(SQL_TYPE_TIMESTAMP));

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
