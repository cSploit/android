#include "common.h"

/* Test SQLFetchScroll with a non-unitary rowset, using bottom-up direction */

static char software_version[] = "$Id: cursor7.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
Test(void)
{
	enum { ROWS=5 };
	struct data_t {
		SQLINTEGER i;
		SQLLEN ind_i;
		char c[20];
		SQLLEN ind_c;
	} data[ROWS];
	SQLUSMALLINT statuses[ROWS];
	SQLULEN num_row;

	int i;
	SQLRETURN RetCode;

	odbc_reset_statement();

	CHKSetStmtAttr(SQL_ATTR_CONCURRENCY, int2ptr(SQL_CONCUR_READ_ONLY), 0, "S");
	CHKSetStmtAttr(SQL_ATTR_CURSOR_TYPE, int2ptr(SQL_CURSOR_STATIC), 0, "S");

	CHKPrepare(T("SELECT c, i FROM #cursor7_test"), SQL_NTS, "S");
	CHKExecute("S");
	CHKSetStmtAttr(SQL_ATTR_ROW_BIND_TYPE, int2ptr(sizeof(data[0])), 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE, int2ptr(ROWS), 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROW_STATUS_PTR, statuses, 0, "S");
	CHKSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR, &num_row, 0, "S");

	CHKBindCol(1, SQL_C_CHAR, &data[0].c, sizeof(data[0].c), &data[0].ind_c, "S");
	CHKBindCol(2, SQL_C_LONG, &data[0].i, sizeof(data[0].i), &data[0].ind_i, "S");

	/* Read records from last to first */
	printf("\n\nReading records from last to first:\n");
	RetCode = CHKFetchScroll(SQL_FETCH_LAST, -ROWS, "SINo");
	while (RetCode != SQL_NO_DATA) {
		SQLULEN RowNumber;

		/* Print this set of rows */
		for (i = ROWS - 1; i >= 0; i--) {
			if (statuses[i] != SQL_ROW_NOROW)
				printf("\t %d, %s\n", (int) data[i].i, data[i].c);
		}
		printf("\n");

		CHKGetStmtAttr(SQL_ROW_NUMBER, (SQLPOINTER)(&RowNumber), sizeof(RowNumber), NULL, "S");
		printf("---> We are in record No: %u\n", (unsigned int) RowNumber);

		/* Read next rowset */
		if ( (RowNumber>1) && (RowNumber<ROWS) ) {
			RetCode = CHKFetchScroll(SQL_FETCH_RELATIVE, 1-RowNumber, "SINo"); 
			for (i=RowNumber-1; i<ROWS; ++i)
				statuses[i] = SQL_ROW_NOROW;
		} else {
			RetCode = CHKFetchScroll(SQL_FETCH_RELATIVE, -ROWS, "SINo");
		}
	}
}

static void
Init(void)
{
	int i;
	char sql[128];

	printf("\n\nCreating table #cursor7_test with 12 records.\n");

	odbc_command("\tCREATE TABLE #cursor7_test (i INT, c VARCHAR(20))");
	for (i = 1; i <= 12; ++i) {
		sprintf(sql, "\tINSERT INTO #cursor7_test(i,c) VALUES(%d, 'a%db%dc%d')", i, i, i, i);
		odbc_command(sql);
	}

}

int
main(int argc, char *argv[])
{
	odbc_use_version3 = 1;
	odbc_connect();

	odbc_check_cursor();

	Init();

	Test();

	odbc_disconnect();

	return 0;
}
