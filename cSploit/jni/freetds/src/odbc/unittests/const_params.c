#include "common.h"

/* Test for {?=call store(?,123,'foo')} syntax and run */

static char software_version[] = "$Id: const_params.c,v 1.19 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLINTEGER input, output;
	SQLINTEGER out1;
	SQLLEN ind, ind2, ind3;

	odbc_connect();

	if (odbc_command_with_result(odbc_stmt, "drop proc const_param") != SQL_SUCCESS)
		printf("Unable to execute statement\n");

	odbc_command("create proc const_param @in1 int, @in2 int, @in3 datetime, @in4 varchar(10), @out int output as\n"
		"begin\n"
		" set nocount on\n"
		" select @out = 7654321\n"
		" if (@in1 <> @in2 and @in2 is not null) or @in3 <> convert(datetime, '2004-10-15 12:09:08') or @in4 <> 'foo'\n"
		"  select @out = 1234567\n"
		" return 24680\n"
		"end");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &input, 0, &ind, "S");
	CHKBindParameter(2, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &out1, 0, &ind2, "S");

	/* TODO use {ts ...} for date */
	CHKPrepare(T("{call const_param(?, 13579, '2004-10-15 12:09:08', 'foo', ?)}"), SQL_NTS, "S");

	input = 13579;
	ind = sizeof(input);
	out1 = output = 0xdeadbeef;
	CHKExecute("S");

	if (out1 != 7654321) {
		fprintf(stderr, "Invalid output %d (0x%x)\n", (int) out1, (int) out1);
		return 1;
	}

	/* just to reset some possible buffers */
	odbc_command("DECLARE @i INT");

	/* MS ODBC don't support empty parameters altough documented so avoid this test */
	if (odbc_driver_is_freetds()) {

		CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &output, 0, &ind,  "S");
		CHKBindParameter(2, SQL_PARAM_INPUT,  SQL_C_SLONG, SQL_INTEGER, 0, 0, &input,  0, &ind2, "S");
		CHKBindParameter(3, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &out1,   0, &ind3, "S");

		/* TODO use {ts ...} for date */
		CHKPrepare(T("{?=call const_param(?, , '2004-10-15 12:09:08', 'foo', ?)}"), SQL_NTS, "S");

		input = 13579;
		ind2 = sizeof(input);
		out1 = output = 0xdeadbeef;
		CHKExecute("S");

		if (out1 != 7654321) {
			fprintf(stderr, "Invalid output %d (0x%x)\n", (int) out1, (int) out1);
			return 1;
		}

		if (output != 24680) {
			fprintf(stderr, "Invalid result %d (0x%x) expected 24680\n", (int) output, (int) output);
			return 1;
		}
	}

	odbc_command("IF OBJECT_ID('const_param') IS NOT NULL DROP PROC const_param");

	odbc_command("create proc const_param @in1 float, @in2 varbinary(100) as\n"
		"begin\n"
		" if @in1 <> 12.5 or @in2 <> 0x0102030405060708\n"
		"  return 12345\n"
		" return 54321\n"
		"end");

	CHKBindParameter(1, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &output, 0, &ind, "S");

	CHKPrepare(T("{?=call const_param(12.5, 0x0102030405060708)}"), SQL_NTS, "S");

	output = 0xdeadbeef;
	CHKExecute("S");

	if (output != 54321) {
		fprintf(stderr, "Invalid result %d (0x%x) expected 54321\n", (int) output, (int) output);
		return 1;
	}

	odbc_command("drop proc const_param");

	odbc_command("create proc const_param @in varchar(20) as\n"
		"begin\n"
		" if @in = 'value' select 8421\n"
		" select 1248\n"
		"end");

	/* catch problem reported by Peter Deacon */
	output = 0xdeadbeef;
	odbc_command("{CALL const_param('value')}");
	CHKBindCol(1, SQL_C_SLONG, &output, 0, &ind, "S");
	SQLFetch(odbc_stmt);

	if (output != 8421) {
		fprintf(stderr, "Invalid result %d (0x%x)\n", (int) output, (int) output);
		return 1;
	}

	odbc_reset_statement();

	odbc_command("drop proc const_param");

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
