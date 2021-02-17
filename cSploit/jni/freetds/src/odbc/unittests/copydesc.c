#include "common.h"

/* Test SQLCopyDesc and SQLAllocHandle(SQL_HANDLE_DESC) */

static char software_version[] = "$Id: copydesc.c,v 1.7 2010-07-05 09:20:32 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	SQLHDESC ard, ard2, ard3;
	SQLINTEGER id;
	SQLLEN ind1, ind2;
	char name[64];

	odbc_connect();

	CHKGetStmtAttr(SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL, "S");

	CHKBindCol(1, SQL_C_SLONG, &id, sizeof(SQLINTEGER), &ind1, "S");
	CHKBindCol(2, SQL_C_CHAR, name, sizeof(name), &ind2, "S");

	CHKAllocHandle(SQL_HANDLE_DESC, odbc_conn, &ard2, "S");

	/*
	 * this is an additional test to test additional allocation 
	 * As of 0.64 for a bug in SQLAllocDesc we only allow to allocate one
	 */
	CHKAllocHandle(SQL_HANDLE_DESC, odbc_conn, &ard3, "S");

	CHKR(SQLCopyDesc, (ard, ard2), "S");

	CHKFreeHandle(SQL_HANDLE_DESC, ard3, "S");

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
