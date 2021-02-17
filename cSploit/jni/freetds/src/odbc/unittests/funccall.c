#include "common.h"

/* Test for {?=call store(?)} syntax and run */

static char software_version[] = "$Id: funccall.c,v 1.21 2012-03-03 22:14:00 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void test_with_conversions(void);

int
main(int argc, char *argv[])
{
	SQLINTEGER input, output;
	SQLLEN ind, ind2, ind3, ind4;
	SQLINTEGER out1;
	char out2[30];

	odbc_connect();

	odbc_command("IF OBJECT_ID('simpleresult') IS NOT NULL DROP PROC simpleresult");

	odbc_command("create proc simpleresult @i int as begin return @i end");

	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &input, 0, &ind2, "S");
	CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &output, 0, &ind, "S");

	CHKPrepare(T("{ \n?\t\r= call simpleresult(?)}"), SQL_NTS, "S");

	input = 123;
	ind2 = sizeof(input);
	output = 0xdeadbeef;
	CHKExecute("S");

	if (output != 123) {
		printf("Invalid result\n");
		exit(1);
	}

	/* should return "Invalid cursor state" */
	if (SQLFetch(odbc_stmt) != SQL_ERROR) {
		printf("Data not expected\n");
		exit(1);
	}
	ODBC_CHECK_COLS(0);

	/* just to reset some possible buffers */
	odbc_command("DECLARE @i INT");

	/* same test but with SQLExecDirect and same bindings */
	input = 567;
	ind2 = sizeof(input);
	output = 0xdeadbeef;
	CHKExecDirect(T("{?=call simpleresult(?)}"), SQL_NTS, "S");

	if (output != 567) {
		fprintf(stderr, "Invalid result\n");
		exit(1);
	}

	/* should return "Invalid cursor state" */
	CHKFetch("E");

	odbc_command("drop proc simpleresult");

	odbc_command("IF OBJECT_ID('simpleresult2') IS NOT NULL DROP PROC simpleresult2");

	/* force cursor close */
	SQLCloseCursor(odbc_stmt);

	/* test output parameter */
	odbc_command("create proc simpleresult2 @i int, @x int output, @y varchar(20) output as begin select @x = 6789 select @y = 'test foo' return @i end");

	CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0,  0, &output, 0,            &ind,  "S");
	CHKBindParameter(2, SQL_PARAM_INPUT,  SQL_C_SLONG, SQL_INTEGER, 0,  0, &input,  0,            &ind2, "S");
	CHKBindParameter(3, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0,  0, &out1,   0,            &ind3, "S");
	CHKBindParameter(4, SQL_PARAM_OUTPUT, SQL_C_CHAR,  SQL_VARCHAR, 20, 0, out2,    sizeof(out2), &ind4, "S");

	CHKPrepare(T("{ \n?\t\r= call simpleresult2(?,?,?)}"), SQL_NTS, "S");

	input = 987;
	ind2 = sizeof(input);
	out1 = 888;
	output = 0xdeadbeef;
	ind3 = SQL_DATA_AT_EXEC;
	ind4 = SQL_DEFAULT_PARAM;
	strcpy(out2, "bad!");
	CHKExecute("S");

	if (output != 987 || ind3 <= 0 || ind4 <= 0 || out1 != 6789 || strcmp(out2, "test foo") != 0) {
		printf("ouput = %d ind3 = %d ind4 = %d out1 = %d out2 = %s\n", (int) output, (int) ind3, (int) ind4, (int) out1,
		       out2);
		printf("Invalid result\n");
		exit(1);
	}

	/* should return "Invalid cursor state" */
	CHKFetch("E");

	ODBC_CHECK_COLS(0);

	odbc_command("drop proc simpleresult2");

	/*
	 * test from shiv kumar
	 * Cfr ML 2006-11-21 "specifying a 0 for the StrLen_or_IndPtr in the
	 * SQLBindParameter call is not working on AIX"
	 */
	odbc_command("IF OBJECT_ID('rpc_read') IS NOT NULL DROP PROC rpc_read");

	odbc_reset_statement();

	odbc_command("create proc rpc_read @i int, @x timestamp as begin select 1 return 1234 end");
	SQLCloseCursor(odbc_stmt);

	CHKPrepare(T("{ ? = CALL rpc_read ( ?, ? ) }"), SQL_NTS, "S");

	ind = 0;
	CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &output, 0, &ind, "S");

	ind2 = 0;
	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &input, 0, &ind2, "S");

	ind3 = 8;
	CHKBindParameter(3, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_VARBINARY, 8, 0, out2, 8, &ind3, "S");

	CHKExecute("S");

	CHKFetch("S");

	CHKFetch("No");

	odbc_reset_statement();
	odbc_command("drop proc rpc_read");

	/*
	 * Test from Joao Amaral
	 * This test SQLExecute where a store procedure returns no result
	 * This seems similar to a previous one but use set instead of select
	 * (with is supported only by mssql and do not return all stuff as 
	 * select does)
	 */
	if (odbc_db_is_microsoft()) {

		odbc_reset_statement();

		odbc_command("IF OBJECT_ID('sp_test') IS NOT NULL DROP PROC sp_test");
		odbc_command("create proc sp_test @res int output as set @res = 456");

		odbc_reset_statement();

		CHKPrepare(T("{ call sp_test(?)}"), SQL_NTS, "S");
		CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &output, 0, &ind, "S");

		output = 0xdeadbeef;
		CHKExecute("S");

		if (output != 456) {
			fprintf(stderr, "Invalid result %d(%x)\n", (int) output, (int) output);
			return 1;
		}
		odbc_command("drop proc sp_test");
	}
	odbc_disconnect();

	if (odbc_db_is_microsoft())
		test_with_conversions();

	printf("Done.\n");
	return 0;
}

static void
test_with_conversions(void)
{
	SQLLEN ind, ind2, ind3;
	char out2[30];

	/*
	 * test from Bower, Wayne
	 * Cfr ML 2012-03-02 "[freetds] [unixODBC][Driver Manager]Function sequence error (SQL-HY010)"
	 */
	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("IF OBJECT_ID('TMP_SP_Test_ODBC') IS NOT NULL DROP PROC TMP_SP_Test_ODBC");
	odbc_command("create proc TMP_SP_Test_ODBC @i int,\n@o int output\nas\nset nocount on\nselect @o = 55\nreturn 9\n");
	odbc_reset_statement();

	CHKPrepare(T("{ ? = call TMP_SP_Test_ODBC (?, ?) }"), SQL_NTS, "S");

	ind2 = 2;
	CHKBindParameter(2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 80, 0, "10", 2, &ind2, "S");
	ind3 = SQL_NULL_DATA;
	strcpy(out2, " ");
	CHKBindParameter(3, SQL_PARAM_INPUT_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 1, out2, 29, &ind3, "S");
	ind = 1;
	CHKBindParameter(1, SQL_PARAM_INPUT_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 1, out2, 29, &ind, "S");

	/* mssql returns SUCCESS */
	CHKExecute("No");

	ODBC_CHECK_COLS(0);

	odbc_reset_statement();
	odbc_command("drop proc TMP_SP_Test_ODBC");


	odbc_disconnect();
}
