#include "common.h"

static SQLINTEGER v_int_3;
static SQLLEN v_ind_3_1;

static char v_char_3[21];
static SQLLEN v_ind_3_2;

static int result = 0;

static void
doFetch(int dir, int pos, int expected)
{
	SQLRETURN RetCode;

	RetCode = CHKFetchScroll(dir, pos, "SINo");

	if (RetCode != SQL_NO_DATA)
		printf(">> fetch %2d %10d : %d [%s]\n", dir, pos, v_ind_3_1 ? (int) v_int_3 : -1, v_ind_3_2 ? v_char_3 : "null");
	else
		printf(">> fetch %2d %10d : no data found\n", dir, pos);

	if (expected != (RetCode == SQL_NO_DATA ? -1 : v_int_3))
		result = 1;
}

int
main(int argc, char **argv)
{
	odbc_use_version3 = 1;
	odbc_connect();
	odbc_check_cursor();

	CHKSetConnectAttr(SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER, "S");

	odbc_command("create table #mytab1 (k int, c char(30))");
	odbc_command("insert into #mytab1 values (1,'aaa')");
	odbc_command("insert into #mytab1 values (2,'bbb')");
	odbc_command("insert into #mytab1 values (3,'ccc')");

	odbc_reset_statement();
/*	CHKSetStmtAttr(SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_STATIC, 0, "S");	*/
	CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE, SQL_IS_UINTEGER, "S");

	CHKPrepare(T("select k, c from #mytab1 order by k"), SQL_NTS, "SI");

	CHKBindCol(1, SQL_C_LONG, &v_int_3, 0, &v_ind_3_1, "S");
	CHKBindCol(2, SQL_C_CHAR, v_char_3, sizeof(v_char_3), &v_ind_3_2, "S");

	CHKExecute("SI");

	doFetch(SQL_FETCH_LAST, 0, 3);
	doFetch(SQL_FETCH_PRIOR, 0, 2);
	doFetch(SQL_FETCH_PRIOR, 0, 1);
	doFetch(SQL_FETCH_PRIOR, 0, -1);
	doFetch(SQL_FETCH_NEXT, 0, 1);
	doFetch(SQL_FETCH_NEXT, 0, 2);
	doFetch(SQL_FETCH_NEXT, 0, 3);
	doFetch(SQL_FETCH_NEXT, 0, -1);
	doFetch(SQL_FETCH_FIRST, 0, 1);
	doFetch(SQL_FETCH_NEXT, 0, 2);
	doFetch(SQL_FETCH_NEXT, 0, 3);
	doFetch(SQL_FETCH_ABSOLUTE, 3, 3);
	doFetch(SQL_FETCH_RELATIVE, -2, 1);
	doFetch(SQL_FETCH_RELATIVE, -2, -1);
	doFetch(SQL_FETCH_RELATIVE, 5, -1);

	CHKCloseCursor("SI");

	odbc_disconnect();
	return result;
}
