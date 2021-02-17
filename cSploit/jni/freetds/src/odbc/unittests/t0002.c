#include "common.h"

static char software_version[] = "$Id: t0002.c,v 1.17 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define SWAP_STMT() do { SQLHSTMT xyz = odbc_stmt; \
	odbc_stmt = old_odbc_stmt; old_odbc_stmt = xyz; } while(0)

int
main(int argc, char *argv[])
{
	HSTMT old_odbc_stmt = SQL_NULL_HSTMT;

	odbc_connect();

	odbc_command("if object_id('tempdb..#odbctestdata') is not null drop table #odbctestdata");

	odbc_command("create table #odbctestdata (i int)");
	odbc_command("insert #odbctestdata values (123)");

	/*
	 * now we allocate another statement, select, get all results
	 * then make another query with first select and drop this statement
	 * result should not disappear (required for DBD::ODBC)
	 */
	SWAP_STMT();
	CHKAllocStmt(&odbc_stmt, "S");

	odbc_command("select * from #odbctestdata where 0=1");

	CHKFetch("No");

	CHKCloseCursor("SI");

	SWAP_STMT();
	odbc_command("select * from #odbctestdata");
	SWAP_STMT();

	/* drop first statement .. data should not disappear */
	CHKFreeStmt(SQL_DROP, "S");
	odbc_stmt = SQL_NULL_HSTMT;
	SWAP_STMT();

	CHKFetch("SI");

	CHKFetch("No");

	CHKCloseCursor("SI");

	odbc_command("drop table #odbctestdata");

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
