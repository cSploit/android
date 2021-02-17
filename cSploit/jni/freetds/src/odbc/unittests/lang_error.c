#include "common.h"

/* Test if SQLExecDirect return error if a error in row is returned */

static char software_version[] = "$Id: lang_error.c,v 1.6 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	odbc_connect();

	/* issue print statement and test message returned */
	odbc_command2("SELECT DATEADD(dd,-100000,getdate())", "E");

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
