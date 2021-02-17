#include "common.h"

/* test RAISERROR in a store procedure, from Tom Rogers tests */

/* TODO add support for Sybase */

static char software_version[] = "$Id: raiserror.c,v 1.26 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define SP_TEXT "{?=call #tmp1(?,?,?)}"
#define OUTSTRING_LEN 20
#define INVALID_RETURN (-12345)

static const char create_proc[] =
	"CREATE PROCEDURE #tmp1\n"
	"    @InParam int,\n"
	"    @OutParam int OUTPUT,\n"
	"    @OutString varchar(20) OUTPUT\n"
	"AS\n"
	"%s"
	"     SET @OutParam = @InParam\n"
	"     SET @OutString = 'This is bogus!'\n"
	"     SELECT 'Here is the first row' AS FirstResult\n"
	"     RAISERROR('An error occurred.', @InParam, 1)\n"
	"%s"
	"     RETURN (0)";

static SQLSMALLINT ReturnCode;
static char OutString[OUTSTRING_LEN];
static int g_nocount, g_second_select;

#ifdef TDS_NO_DM
static const int tds_no_dm = 1;
#else
static const int tds_no_dm = 0;
#endif

static void
TestResult(SQLRETURN result0, int level, const char *func)
{
	ODBC_BUF *odbc_buf = NULL;
	SQLTCHAR SqlState[6];
	SQLINTEGER NativeError;
	SQLTCHAR MessageText[1000];
	SQLSMALLINT TextLength;
	SQLRETURN result = result0, rc;

	if (result == SQL_NO_DATA && strcmp(func, "SQLFetch") == 0)
		result = SQL_SUCCESS_WITH_INFO;

	if ((level <= 10 && result != SQL_SUCCESS_WITH_INFO) || (level > 10 && result != SQL_ERROR) || ReturnCode != INVALID_RETURN) {
		fprintf(stderr, "%s failed!\n", func);
		exit(1);
	}

	/*
	 * unixODBC till 2.2.11 do not support getting error if SQL_NO_DATA
	 */
	if (!tds_no_dm && result0 == SQL_NO_DATA && strcmp(func, "SQLFetch") == 0)
		return;

	SqlState[0] = 0;
	MessageText[0] = 0;
	NativeError = 0;
	rc = CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, 1, SqlState, &NativeError, MessageText, ODBC_VECTOR_SIZE(MessageText), &TextLength, "SI");
	printf("Func=%s Result=%d DIAG REC 1: State=%s Error=%d: %s\n", func, (int) rc, C(SqlState), (int) NativeError, C(MessageText));

	if (strstr(C(MessageText), "An error occurred") == NULL) {
		fprintf(stderr, "Wrong error returned!\n");
		fprintf(stderr, "Error returned: %s\n", C(MessageText));
		exit(1);
	}
	ODBC_FREE();
}

#define MY_ERROR(msg) odbc_report_error(msg, line, __FILE__)

static void
CheckData(const char *s, int line)
{
	char buf[80];
	SQLLEN ind;
	SQLRETURN rc;

	rc = CHKGetData(1, SQL_C_CHAR, buf, sizeof(buf), &ind, "SE");

	if (rc == SQL_ERROR) {
		buf[0] = 0;
		ind = 0;
	}

	if (strlen(s) != ind || strcmp(buf, s) != 0)
		MY_ERROR("Invalid result");
}

#define CheckData(s) CheckData(s, __LINE__)

static void
CheckReturnCode(SQLRETURN result, SQLSMALLINT expected, int line)
{
	if (ReturnCode == expected && (expected != INVALID_RETURN || strcmp(OutString, "Test") == 0)
	    && (expected == INVALID_RETURN || strcmp(OutString, "This is bogus!") == 0))
		return;

	printf("SpDateTest Output:\n");
	printf("   Result = %d\n", (int) result);
	printf("   Return Code = %d\n", (int) ReturnCode);
	printf("   OutString = \"%s\"\n", OutString);
	MY_ERROR("Invalid ReturnCode");
}

#define CheckReturnCode(res, exp) CheckReturnCode(res, exp, __LINE__)

static void
Test(int level)
{
	SQLRETURN result;
	SQLSMALLINT InParam = level;
	SQLSMALLINT OutParam = 1;
	SQLLEN cbReturnCode = 0, cbInParam = 0, cbOutParam = 0;
	SQLLEN cbOutString = SQL_NTS;

	char sql[80];

	printf("ODBC %d nocount %s select %s level %d\n", odbc_use_version3 ? 3 : 2,
	       g_nocount ? "yes" : "no", g_second_select ? "yes" : "no", level);

	ReturnCode = INVALID_RETURN;
	memset(&OutString, 0, sizeof(OutString));

	/* test with SQLExecDirect */
	sprintf(sql, "RAISERROR('An error occurred.', %d, 1)", level);
	result = odbc_command_with_result(odbc_stmt, sql);

	TestResult(result, level, "SQLExecDirect");

	/* test with SQLPrepare/SQLExecute */
	if (!SQL_SUCCEEDED(SQLPrepare(odbc_stmt, T(SP_TEXT), strlen(SP_TEXT)))) {
		fprintf(stderr, "SQLPrepare failure!\n");
		exit(1);
	}

	SQLBindParameter(odbc_stmt, 1, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &ReturnCode, 0, &cbReturnCode);
	SQLBindParameter(odbc_stmt, 2, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &InParam, 0, &cbInParam);
	SQLBindParameter(odbc_stmt, 3, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &OutParam, 0, &cbOutParam);
	strcpy(OutString, "Test");
	SQLBindParameter(odbc_stmt, 4, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, OUTSTRING_LEN, 0, OutString, OUTSTRING_LEN,
			 &cbOutString);

	CHKExecute("S");

	CheckData("");
	CHKFetch("S");
	CheckData("Here is the first row");

	result = SQLFetch(odbc_stmt);
	if (odbc_use_version3) {
		SQLTCHAR SqlState[6];
		SQLINTEGER NativeError;
		SQLTCHAR MessageText[1000];
		SQLSMALLINT TextLength;
		SQLRETURN expected;

		if (result != SQL_NO_DATA)
			ODBC_REPORT_ERROR("SQLFetch should return NO DATA");
		CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, 1, SqlState, &NativeError, MessageText,
				       ODBC_VECTOR_SIZE(MessageText), &TextLength, "No");
		result = SQLMoreResults(odbc_stmt);
		expected = level > 10 ? SQL_ERROR : SQL_SUCCESS_WITH_INFO;
		if (result != expected)
			ODBC_REPORT_ERROR("SQLMoreResults returned unexpected result");
		if (odbc_use_version3 && !g_second_select && g_nocount) {
			CheckReturnCode(result, 0);
			ReturnCode = INVALID_RETURN;
			TestResult(result, level, "SQLMoreResults");
			ReturnCode = 0;
		} else {
			TestResult(result, level, "SQLMoreResults");
		}
		ODBC_CHECK_ROWS(-1);
	} else {
		TestResult(result, level, "SQLFetch");
	}

	if (odbc_driver_is_freetds())
		CheckData("");

	if (!g_second_select) {
		ODBC_CHECK_ROWS(-1);
		CheckReturnCode(result, g_nocount ? 0 : INVALID_RETURN);

		result = SQLMoreResults(odbc_stmt);
#ifdef ENABLE_DEVELOPING
		if (result != SQL_NO_DATA)
			ODBC_REPORT_ERROR("SQLMoreResults should return NO DATA");

		ODBC_CHECK_ROWS(-2);
#endif
		CheckReturnCode(result, 0);
		return;
	}

	if (!odbc_use_version3 || !g_nocount) {
		/* mssql 2008 return SUCCESS_WITH_INFO with previous error */
		CHKMoreResults("S");
		result = SQL_SUCCESS;
	}

	CheckReturnCode(result, INVALID_RETURN);

	CheckData("");
	CHKFetch("S");
	CheckData("Here is the last row");

	CHKFetch("No");
	CheckData("");

	if (!odbc_use_version3 || g_nocount)
		CheckReturnCode(result, 0);
#ifdef ENABLE_DEVELOPING
	else
		CheckReturnCode(result, INVALID_RETURN);
#endif

	/* FIXME how to handle return in store procedure ??  */
	result = SQLMoreResults(odbc_stmt);
#ifdef ENABLE_DEVELOPING
	if (result != SQL_NO_DATA)
		ODBC_REPORT_ERROR("SQLMoreResults return other data");
#endif

	CheckReturnCode(result, 0);

	CheckData("");
	ODBC_FREE();
}

static void
Test2(int nocount, int second_select)
{
	char sql[512];

	g_nocount = nocount;
	g_second_select = second_select;

	/* this test do not work with Sybase */
	if (!odbc_db_is_microsoft())
		return;

	sprintf(sql, create_proc, nocount ? "     SET NOCOUNT ON\n" : "",
		second_select ? "     SELECT 'Here is the last row' AS LastResult\n" : "");
	odbc_command(sql);

	Test(5);

	Test(11);

	odbc_command("DROP PROC #tmp1");
}

int
main(int argc, char *argv[])
{
	odbc_connect();

	Test2(0, 1);

	Test2(1, 1);

	odbc_disconnect();

	odbc_use_version3 = 1;

	odbc_connect();

	Test2(0, 1);
	Test2(1, 1);

	Test2(0, 0);
	Test2(1, 0);

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
