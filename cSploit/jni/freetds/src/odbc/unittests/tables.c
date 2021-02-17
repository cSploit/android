#include "common.h"

static char software_version[] = "$Id: tables.c,v 1.21 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#ifdef _WIN32
#undef strcasecmp
#define strcasecmp stricmp
#endif

static SQLLEN cnamesize;
static char output[256];

static void
ReadCol(int i)
{
	strcpy(output, "NULL");
	CHKGetData(i, SQL_C_CHAR, output, sizeof(output), &cnamesize, "S");
}

static void
TestName(int index, const char *expected_name)
{
	SQLTCHAR name[128];
	char buf[256];
	SQLSMALLINT len, type;

#define NAME_TEST \
	do { \
		if (strcmp(C(name), expected_name) != 0) \
		{ \
			sprintf(buf, "wrong name in column %d expected '%s' got '%s'", index, expected_name, C(name)); \
			ODBC_REPORT_ERROR(buf); \
		} \
	} while(0)

	/* retrieve with SQLDescribeCol */
	CHKDescribeCol(index, name, ODBC_VECTOR_SIZE(name), &len, &type, NULL, NULL, NULL, "S");
	NAME_TEST;

	/* retrieve with SQLColAttribute */
	CHKColAttribute(index, SQL_DESC_NAME, name, ODBC_VECTOR_SIZE(name), &len, NULL, "S");
	if (odbc_db_is_microsoft())
		NAME_TEST;
	CHKColAttribute(index, SQL_DESC_LABEL, name, ODBC_VECTOR_SIZE(name), &len, NULL, "S");
	NAME_TEST;
}

static const char *catalog = NULL;
static const char *schema = NULL;
static const char *table = "sysobjects";
static const char *expect = NULL;
static int expect_col = 3;
static char expected_type[20] = "SYSTEM TABLE";

static void
DoTest(const char *type, int row_returned, int line)
{
	int table_len = SQL_NULL_DATA;
	char table_buf[80];
	int found = 0;

#define LEN(x) (x) ? strlen(x) : SQL_NULL_DATA

	if (table) {
		strcpy(table_buf, table);
		strcat(table_buf, "garbage");
		table_len = strlen(table);
	}

	printf("Test type '%s' %s row at line %d\n", type ? type : "", row_returned ? "with" : "without", line);
	CHKTables(T(catalog), LEN(catalog), T(schema), LEN(schema), T(table_buf), table_len, T(type), LEN(type), "SI");

	/* test column name (for DBD::ODBC) */
	TestName(1, odbc_use_version3 || !odbc_driver_is_freetds() ? "TABLE_CAT" : "TABLE_QUALIFIER");
	TestName(2, odbc_use_version3 || !odbc_driver_is_freetds() ? "TABLE_SCHEM" : "TABLE_OWNER");
	TestName(3, "TABLE_NAME");
	TestName(4, "TABLE_TYPE");
	TestName(5, "REMARKS");

	if (row_returned) {
		CHKFetch("SI");

		if (!expect) {
			ReadCol(1);
			ReadCol(2);
			ReadCol(3);
			if (strcasecmp(output, "sysobjects") != 0) {
				printf("wrong table %s\n", output);
				exit(1);
			}

			ReadCol(4);
			/* under mssql2k5 is a view */
			if (strcmp(output, expected_type) != 0) {
				printf("wrong table type %s\n", output);
				exit(1);
			}
			ReadCol(5);
		}
	}

	if (expect) {
		ReadCol(expect_col);
		if (strcmp(output, expect) == 0)
			found = 1;
	}
	while (CHKFetch("SNo") == SQL_SUCCESS && row_returned > 1) {
		if (expect) {
			ReadCol(expect_col);
			if (strcmp(output, expect) == 0)
				found = 1;
		}
		if (row_returned < 2)
			break;
	}

	if (expect && !found) {
		printf("expected row not found\n");
		exit(1);
	}

	CHKCloseCursor("SI");
	expect = NULL;
	expect_col = 3;
}

#define DoTest(a,b) DoTest(a,b,__LINE__)

int
main(int argc, char *argv[])
{
	char type[32];
	int mssql2005 = 0;

	odbc_use_version3 = 0;
	odbc_connect();

	if (odbc_db_is_microsoft() && odbc_db_version_int() >= 0x09000000u) {
		mssql2005 = 1;
		strcpy(expected_type, "VIEW");
		odbc_command_with_result(odbc_stmt, "USE master");
	}

	DoTest(NULL, 1);
	sprintf(type, "'%s'", expected_type);
	DoTest(type, 1);
	DoTest("'TABLE'", 0);
	DoTest(type, 1);
	DoTest("TABLE", 0);
	DoTest("TABLE,VIEW", mssql2005 ? 1 : 0);
	DoTest("SYSTEM TABLE,'TABLE'", mssql2005 ? 0 : 1);
	sprintf(type, "TABLE,'%s'", expected_type);
	DoTest(type, 1);

	odbc_disconnect();


	odbc_use_version3 = 1;
	odbc_connect();

	if (mssql2005)
		odbc_command_with_result(odbc_stmt, "USE master");

	sprintf(type, "'%s'", expected_type);
	DoTest(type, 1);
	/* TODO this should work even for Sybase and mssql 2005 */
	if (odbc_db_is_microsoft()) {
		/* here table is a name of table */
		catalog = "%";
		schema = NULL;
		DoTest(NULL, 2);
	}

	/*
	 * tests for Jdbc compatiblity
	 */

	/* enum tables */
	catalog = NULL;
	schema = NULL;
	table = "%";
	expect = "sysobjects";
	DoTest(NULL, 2);

	/* enum catalogs */
	catalog = "%";
	schema = "";
	table = "";
	expect = "master";
	expect_col = 1;
	DoTest(NULL, 2);

	/* enum schemas (owners) */
	catalog = "";
	schema = "%";
	table = "";
	expect = "dbo";
	expect_col = 2;
	DoTest(NULL, 2);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
