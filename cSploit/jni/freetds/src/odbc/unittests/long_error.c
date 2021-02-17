#define UNICODE 1
#define _UNICODE 1
#include "common.h"


/*
Demonstration of triggered assert when invoking this stored procedure
using FreeTDS odbc driver:

create procedure proc_longerror as
begin
    raiserror('reallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylongreallylong error', 16, 1)
end

*/

static void extract_error(SQLHANDLE handle, SQLSMALLINT type);

int
main(void)
{
	int i;
	char cmd[128 + 110*10];

	printf("SQLTCHAR size is: %d\n", (int) sizeof(SQLTCHAR));

	odbc_use_version3 = 1;
	odbc_connect();

	/* this test do not work with Sybase */
	if (!odbc_db_is_microsoft()) {
		odbc_disconnect();
		return 0;
	}

	strcpy(cmd, "create procedure #proc_longerror as\nbegin\nraiserror('");
	for (i = 0; i < 110; ++i)
		strcat(cmd, "reallylong");
	strcat(cmd, " error', 16, 1)\nend\n");
	odbc_command(cmd);

	CHKExecDirect(T("{CALL #proc_longerror}"), SQL_NTS, "E");

	extract_error(odbc_stmt, SQL_HANDLE_STMT);

	odbc_disconnect();
	return 0;
}

static void
extract_error(SQLHANDLE handle, SQLSMALLINT type)
{
	SQLINTEGER i = 0;
	SQLINTEGER native;
	SQLWCHAR state[7];
	SQLWCHAR text[256];
	SQLSMALLINT len;
	SQLRETURN ret;

	fprintf(stderr, "\n" "The driver reported the following diagnostics\n");

	do {
		ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, 256, &len);
		state[5] = 0;
		if (SQL_SUCCEEDED(ret))
			printf("%s:%ld:%ld:%s\n", C(state), (long) i, (long) native, C(text));
	}
	while (ret == SQL_SUCCESS);
}

