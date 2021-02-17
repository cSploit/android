#include "common.h"
#include <assert.h>

/* Test compute results */

/*
 * This it's quite important cause it test different result types
 * mssql odbc have also some extension not supported by FreeTDS
 * and declared in odbcss.h
 */

static char software_version[] = "$Id: compute.c,v 1.14 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static char col1[256], col2[256];
static SQLLEN ind1, ind2;

static int main_line;

static void
TestName(SQLSMALLINT index, const char *expected_name)
{
	SQLTCHAR name[128];
	char buf[256];
	SQLSMALLINT len, type;

#define NAME_TEST \
	do { \
		if (strcmp(C(name), expected_name) != 0) \
		{ \
			sprintf(buf, "line %d: wrong name in column %d expected '%s' got '%s'", \
				main_line, index, expected_name, C(name)); \
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

static void
CheckFetch(const char *c1name, const char *c1, const char *c2)
{
	int error = 0;

	TestName(1, c1name);

	CHKFetch("S");

	if (strlen(c1) != ind1 || strcmp(c1, col1) != 0) {
		fprintf(stderr, "%s:%d: Column 1 error '%s' (%d) expected '%s' (%d)\n", __FILE__, main_line, col1, (int) ind1, c1,
			(int) strlen(c1));
		error = 1;
	}

	if (strlen(c2) != ind2 || strcmp(c2, col2) != 0) {
		fprintf(stderr, "%s:%d: Column 2 error '%s' (%d) expected '%s' (%d)\n", __FILE__, main_line, col2, (int) ind2, c2,
			(int) strlen(c2));
		error = 1;
	}

	if (error)
		exit(1);
}

#define CheckFetch main_line = __LINE__; CheckFetch

int
main(int argc, char *argv[])
{
	odbc_connect();

	odbc_command("create table #tmp1 (c varchar(20), i int)");
	odbc_command("insert into #tmp1 values('pippo', 12)");
	odbc_command("insert into #tmp1 values('pippo', 34)");
	odbc_command("insert into #tmp1 values('pluto', 1)");
	odbc_command("insert into #tmp1 values('pluto', 2)");
	odbc_command("insert into #tmp1 values('pluto', 3)");


	/* 
	 * TODO skip rows/column on compute (compute.c)
	 * TODO check rows/column after moreresults after compute
	 */


	/* select * from #tmp1 compute sum(i) */
	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	odbc_command("select * from #tmp1 order by c, i compute sum(i)");
	CheckFetch("c", "pippo", "12");
	CheckFetch("c", "pippo", "34");
	CheckFetch("c", "pluto", "1");
	CheckFetch("c", "pluto", "2");
	CheckFetch("c", "pluto", "3");
	CHKFetch("No");
	CHKMoreResults("S");

	/* why I need to rebind ?? ms bug of feature ?? */
	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	col2[0] = '@';
	CheckFetch("sum", "52", "@");
	CHKFetch("No");
	CHKMoreResults("No");




	/* select * from #tmp1 order by c compute sum(i) by c */
	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	odbc_command("select c as mao, i from #tmp1 order by c, i compute sum(i) by c compute max(i)");
	CheckFetch("mao", "pippo", "12");
	CheckFetch("mao", "pippo", "34");
	CHKFetch("No");
	CHKMoreResults("S");

	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	strcpy(col2, "##");
	CheckFetch("sum", "46", "##");
	CHKFetch("No");
	CHKMoreResults("S");

	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	CheckFetch("mao", "pluto", "1");
	CheckFetch("mao", "pluto", "2");
	CheckFetch("mao", "pluto", "3");
	CHKFetch("No");
	CHKMoreResults("S");

	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	strcpy(col2, "%");
	CheckFetch("sum", "6", "%");
	CHKFetch("No");
	CHKMoreResults("S");

	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	strcpy(col2, "&");
	CheckFetch("max", "34", "&");
	CHKFetch("No");
	CHKMoreResults("No");



	/* test skip recordset with computed rows */

	/* select * from #tmp1 where i = 2 compute min(i) */
	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	odbc_command("select * from #tmp1 where i = 2 or i = 34 order by c, i compute min(i) by c");
	CheckFetch("c", "pippo", "34");
	CHKFetch("No");
	CHKMoreResults("S");

	/* here just skip results, before a row */
	CHKMoreResults("S");

	SQLBindCol(odbc_stmt, 1, SQL_C_CHAR, col1, sizeof(col1), &ind1);
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, col2, sizeof(col2), &ind2);
	CheckFetch("c", "pluto", "2");
	CHKFetch("No");
	CHKMoreResults("S");

	/* here just skip results, before done */
	CHKMoreResults("No");


	odbc_disconnect();
	return 0;
}
