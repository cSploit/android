#include "common.h"

/*
 * Try to make core dump using SQLBindParameter
 */

static char software_version[] = "$Id: paramcore.c,v 1.9 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define SP_TEXT "{call sp_paramcore_test(?)}"
#define OUTSTRING_LEN 20

int
main(int argc, char *argv[])
{
	SQLLEN cb = SQL_NTS;

	odbc_use_version3 = 1;

	odbc_connect();

	odbc_command_with_result(odbc_stmt, "drop proc sp_paramcore_test");
	odbc_command("create proc sp_paramcore_test @s varchar(100) output as select @s = '12345'");

	/* here we pass a NULL buffer for input SQL_NTS */
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, OUTSTRING_LEN, 0, NULL, OUTSTRING_LEN, &cb, "S");

	cb = SQL_NTS;
	CHKExecDirect(T(SP_TEXT), SQL_NTS, "E");
	odbc_reset_statement();

	/* here we pass a NULL buffer for input */
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_VARCHAR, 18, 0, NULL, OUTSTRING_LEN, &cb, "S");

	cb = 1;
	CHKExecDirect(T(SP_TEXT), SQL_NTS, "E");
	odbc_reset_statement();

	odbc_command("drop proc sp_paramcore_test");
	odbc_command("create proc sp_paramcore_test @s numeric(10,2) output as select @s = 12345.6");
	odbc_reset_statement();

#if 0	/* this fails even on native platforms */
	/* here we pass a NULL buffer for output */
	cb = sizeof(SQL_NUMERIC_STRUCT);
	SQLBindParameter(odbc_stmt, 1, SQL_PARAM_OUTPUT, SQL_C_NUMERIC, SQL_NUMERIC, 18, 0, NULL, OUTSTRING_LEN, &cb);
	odbc_read_error();

	cb = 1;
	odbc_command_with_result(odbc_stmt, SP_TEXT);
	odbc_read_error();
	odbc_reset_statement();
#endif

	odbc_command("drop proc sp_paramcore_test");

	odbc_disconnect();

	printf("Done successfully!\n");
	return 0;
}
