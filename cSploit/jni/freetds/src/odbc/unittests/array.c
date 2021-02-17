#include "common.h"
#include <assert.h>

/* Test using array binding */

static char software_version[] = "$Id: array.c,v 1.18 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static SQLTCHAR *test_query = NULL;
static int multiply = 90;
static int failure = 0;

#define XMALLOC_N(t, n) (t*) ODBC_GET(n*sizeof(t))

static void
query_test(int prepare, SQLRETURN expected, const char *expected_status)
{
#define DESC_LEN 51
#define ARRAY_SIZE 10

	SQLUINTEGER *ids = XMALLOC_N(SQLUINTEGER,ARRAY_SIZE);
	typedef SQLCHAR desc_t[DESC_LEN];
	desc_t *descs = XMALLOC_N(desc_t, ARRAY_SIZE);
	SQLLEN *id_lens = XMALLOC_N(SQLLEN,ARRAY_SIZE), *desc_lens = XMALLOC_N(SQLLEN,ARRAY_SIZE);
	SQLUSMALLINT i, *statuses = XMALLOC_N(SQLUSMALLINT,ARRAY_SIZE);
	unsigned *num_errors = XMALLOC_N(unsigned, ARRAY_SIZE);
	SQLULEN processed;
	RETCODE ret;
	char status[20];
	SQLTCHAR *err = ODBC_GET(sizeof(odbc_err)*sizeof(SQLTCHAR));
	SQLTCHAR *state = ODBC_GET(sizeof(odbc_sqlstate)*sizeof(SQLTCHAR));

	assert(odbc_stmt != SQL_NULL_HSTMT);
	odbc_reset_statement();

	odbc_command_with_result(odbc_stmt, "drop table #tmp1");
	odbc_command("create table #tmp1 (id tinyint, value char(20))");

	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_PARAMSET_SIZE, (void *) ARRAY_SIZE, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_PARAM_STATUS_PTR, statuses, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &processed, 0);
	SQLBindParameter(odbc_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 5, 0, ids, 0, id_lens);
	SQLBindParameter(odbc_stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, DESC_LEN - 1, 0, descs, DESC_LEN, desc_lens);

	processed = ARRAY_SIZE + 1;
	for (i = 0; i < ARRAY_SIZE; i++) {
		statuses[i] = SQL_PARAM_DIAG_UNAVAILABLE;
		ids[i] = (i + 1) * multiply;
		sprintf((char *) descs[i], "data %d", i * 7);
		id_lens[i] = 0;
		desc_lens[i] = SQL_NTS;
		num_errors[i] = 0;
	}
	multiply = 90;

	if (!prepare) {
		ret = SQLExecDirect(odbc_stmt, test_query, SQL_NTS);
	} else {
		SQLPrepare(odbc_stmt, test_query, SQL_NTS);
		ret = SQLExecute(odbc_stmt);
	}

	if (processed > ARRAY_SIZE) {
		char buf[256];

		sprintf(buf, "Invalid processed number: %d", (int) processed);
		ODBC_REPORT_ERROR(buf);
	}

	for (i = 1; CHKGetDiagRec(SQL_HANDLE_STMT, odbc_stmt, i, state, NULL, err, sizeof(odbc_err), NULL, "SINo") != SQL_NO_DATA; ++i) {
		SQLINTEGER row;

		strcpy(odbc_err, C(err));
		strcpy(odbc_sqlstate, C(state));
		CHKGetDiagField(SQL_HANDLE_STMT, odbc_stmt, i, SQL_DIAG_ROW_NUMBER, &row, sizeof(row), NULL, "S");

		if (row == SQL_ROW_NUMBER_UNKNOWN) continue;
		if (row < 1 || row > ARRAY_SIZE) {
			fprintf(stderr, "invalid row %d returned reading error number %d\n", (int) row, i);
			exit(1);
		}
		++num_errors[row-1];
		printf("for row %2d returned '%s' %s\n", (int) row, odbc_sqlstate, odbc_err);
	}

	for (i = 0; i < processed; ++i) {
		int has_diag = 0;

		switch (statuses[i]) {
		case SQL_PARAM_SUCCESS_WITH_INFO:
			has_diag = 1;
		case SQL_PARAM_SUCCESS:
			status[i] = 'V';
			break;

		case SQL_PARAM_ERROR:
			has_diag = 1;
			status[i] = '!';
			break;

		case SQL_PARAM_UNUSED:
			status[i] = ' ';
			break;

		case SQL_PARAM_DIAG_UNAVAILABLE:
			status[i] = '?';
			break;
		default:
			fprintf(stderr, "Invalid status returned %d\n", statuses[i]);
			exit(1);
		}

		if (has_diag) {
			if (!num_errors[i]) {
				fprintf(stderr, "Diagnostics not returned for status %d\n", i);
				failure = 1;
			}
		} else {
			if (num_errors[i]) {
				fprintf(stderr, "Diagnostics returned for status %d\n", i);
				failure = 1;
			}
		}
	}
	status[i] = 0;

	if (ret != expected || strcmp(expected_status, status) != 0) {
		fprintf(stderr, "Invalid result: got %d \"%s\" expected %d \"%s\" processed %d\n",
			ret, status, expected, expected_status, (int) processed);
		if (ret != SQL_SUCCESS)
			odbc_read_error();
		failure = 1;
		odbc_reset_statement();
		return;
	}

	odbc_reset_statement();
}

int
main(int argc, char *argv[])
{
	odbc_use_version3 = 1;
	odbc_connect();

	if (odbc_db_is_microsoft()) {
		/* all successes */
		test_query = T("INSERT INTO #tmp1 (id, value) VALUES (?, ?)");
		multiply = 1;
		query_test(0, SQL_SUCCESS, "VVVVVVVVVV");
		multiply = 1;
		query_test(1, SQL_SUCCESS, "VVVVVVVVVV");

		/* all errors */
		test_query = T("INSERT INTO #tmp1 (id, value) VALUES (?, ?)");
		multiply = 257;
		query_test(0, SQL_ERROR, "!!!!!!!!!!");
		multiply = 257;
		query_test(1, SQL_SUCCESS_WITH_INFO, "!!!!!!!!!!");

		test_query = T("INSERT INTO #tmp1 (id, value) VALUES (?, ?)");
		query_test(0, SQL_ERROR, "VV!!!!!!!!");
		query_test(1, SQL_SUCCESS_WITH_INFO, "VV!!!!!!!!");

		test_query = T("INSERT INTO #tmp1 (id, value) VALUES (900-?, ?)");
		query_test(0, SQL_SUCCESS_WITH_INFO, "!!!!!!!VVV");
		query_test(1, SQL_SUCCESS_WITH_INFO, "!!!!!!!VVV");

		test_query = T("INSERT INTO #tmp1 (id) VALUES (?) UPDATE #tmp1 SET value = ?");
		query_test(0, SQL_SUCCESS_WITH_INFO, "VVVV!V!V!V");
		query_test(1, SQL_SUCCESS_WITH_INFO, "VV!!!!!!!!");

#ifdef ENABLE_DEVELOPING
		/* with result, see how SQLMoreResult work */
		test_query = T("INSERT INTO #tmp1 (id) VALUES (?) SELECT * FROM #tmp1 UPDATE #tmp1 SET value = ?");
		/* IMHO our driver is better here -- freddy77 */
		query_test(0, SQL_SUCCESS, odbc_driver_is_freetds() ? "VVVVV!V!V!" : "VVVVVV!VVV");
		query_test(1, SQL_SUCCESS, "VVVVVVVVVV");
#endif
	} else {
		/* Sybase test for conversions before executing */
		test_query = T("INSERT INTO #tmp1 (id, value) VALUES (?/8, ?)");
		query_test(0, SQL_SUCCESS, "VVVVVVVVVV");
	}

	/* TODO record binding, array fetch, sqlputdata */

	odbc_disconnect();

	printf(failure ? "Failed :(\n" : "Success!\n");
	return failure;
}

