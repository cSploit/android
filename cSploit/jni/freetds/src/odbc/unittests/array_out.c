#include "common.h"
#include <assert.h>

/* Test using array binding */

static char software_version[] = "$Id: array_out.c,v 1.19 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static const char *test_query = NULL;
static int trunc = 0;
static int record_bind = 0;

typedef struct
{
	SQLUINTEGER id;
	SQLLEN id_len;
	SQLLEN desc_len;
} Record;

static void
query_test(const char* expected, const char *expected_status)
{
#define ARRAY_SIZE 10

	ODBC_BUF *odbc_buf = NULL;
	SQLUINTEGER *ids;
	SQLCHAR *descs;
	SQLLEN *id_lens, *desc_lens;
	SQLULEN processed;
	SQLUSMALLINT i, statuses[ARRAY_SIZE];
	int desc_len = trunc ? 4 : 51;
	int rec_size = 0;
	Record *rec = NULL;
	char status[20];

	assert(odbc_stmt != SQL_NULL_HSTMT);
	odbc_reset_statement();

	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_ROW_STATUS_PTR, statuses, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_ROW_ARRAY_SIZE, (void *) ARRAY_SIZE, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_ROWS_FETCHED_PTR, &processed, 0);
	SQLSetStmtAttr(odbc_stmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);

	if (!record_bind) {
		ids = (SQLUINTEGER *) ODBC_GET(sizeof(SQLUINTEGER) * ARRAY_SIZE);
		descs = ODBC_GET(sizeof(SQLCHAR) * ARRAY_SIZE * desc_len);
		desc_lens = (SQLLEN *) ODBC_GET(sizeof(SQLLEN) * ARRAY_SIZE);
		id_lens = (SQLLEN *) ODBC_GET(sizeof(SQLLEN) * ARRAY_SIZE);
		assert(descs && ids && desc_lens && id_lens);
	} else {
		rec_size = (sizeof(Record) + (sizeof(SQLCHAR) * desc_len + sizeof(SQLLEN) - 1)) & ~(sizeof(SQLLEN) - 1);
		SQLSetStmtAttr(odbc_stmt, SQL_ATTR_ROW_BIND_TYPE, int2ptr(rec_size), 0);
		rec = (Record *) ODBC_GET(rec_size * ARRAY_SIZE);
		ids = &rec->id;
		id_lens = &rec->id_len;
		desc_lens = &rec->desc_len;
		descs = (SQLCHAR *) (((char *) rec) + sizeof(Record));
	}
#define REC(f,n) (((char*)f)+rec_size*(n))
#define DESCS(n) (rec ? (SQLCHAR*)REC(descs,n): (descs+(n)*desc_len))
#define IDS(n) *(rec ? (SQLUINTEGER*)REC(ids,n) : &ids[n])
#define ID_LENS(n) *(rec ? (SQLLEN*)REC(id_lens,n) : &id_lens[n])
#define DESC_LENS(n) *(rec ? (SQLLEN*)REC(desc_lens,n) : &desc_lens[n])

	processed = ARRAY_SIZE + 1;
	for (i = 0; i < ARRAY_SIZE; i++) {
		statuses[i] = SQL_ROW_UPDATED;
		IDS(i) = i * 132;
		sprintf((char *) DESCS(i), "aaa");
		ID_LENS(i) = 0;
		DESC_LENS(i) = -i;
	}

	SQLBindCol(odbc_stmt, 1, SQL_C_ULONG, &IDS(0), 0, &ID_LENS(0));
	SQLBindCol(odbc_stmt, 2, SQL_C_CHAR, DESCS(0), desc_len, &DESC_LENS(0));

	CHKExecDirect(T(test_query), SQL_NTS, "S");

	CHKFetch(expected);

	assert(processed <= ARRAY_SIZE);

	for (i = 0; i < processed; ++i) {
		char buf[128];

		sprintf(buf, "%crow number %d", 'a' + i, i * 13);
		if (trunc)
			buf[3] = 0;
		if (IDS(i) != i + 1 || strcmp((char *) DESCS(i), buf) != 0) {
			fprintf(stderr, "Invalid result\n\tgot '%d|%s'\n\texpected '%d|%s'\n", (int) IDS(i), DESCS(i), i + 1, buf);
			exit(1);
		}

		switch (statuses[i]) {
		case SQL_ROW_SUCCESS:
			status[i] = 'V';
			break;

		case SQL_ROW_SUCCESS_WITH_INFO:
			status[i] = 'v';
			break;

		case SQL_ROW_ERROR:
			status[i] = '!';
			break;

		case SQL_ROW_NOROW:
			status[i] = ' ';
			break;

		default:
			fprintf(stderr, "Invalid status returned\n");
			exit(1);
		}
	}
	status[i] = 0;

	if (expected_status && strcmp(expected_status, status) != 0) {
		fprintf(stderr, "Invalid status\n\tgot '%s'\n\texpected '%s'\n", status, expected_status);
		exit(1);
	}

	ODBC_FREE();
}

int
main(int argc, char *argv[])
{
	int i;

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("CREATE TABLE #odbc_test(i INT, t TEXT)");
	for (i = 0; i < 10; ++i) {
		char buf[128];

		sprintf(buf, "INSERT INTO #odbc_test(i, t) VALUES(%d, '%crow number %d')", i + 1, 'a' + i, i * 13);
		odbc_command(buf);
	}

	odbc_reset_statement();

	test_query = "SELECT * FROM #odbc_test ORDER BY i";
	printf("test line %d\n", __LINE__);
	query_test("S", "VVVVVVVVVV");

	test_query = "SELECT * FROM #odbc_test WHERE i < 7 ORDER BY i";
	printf("test line %d\n", __LINE__);
	query_test("S", "VVVVVV");

	/* binding row */
	test_query = "SELECT * FROM #odbc_test ORDER BY i";
	record_bind = 1;
	printf("test line %d\n", __LINE__);
	query_test("S", "VVVVVVVVVV");

	/* row and truncation */
	trunc = 1;
	printf("test line %d\n", __LINE__);
	query_test("I", "!!!!!!!!!!");

	/* TODO bind offset, SQLGetData, no bind, error */

	odbc_disconnect();

	printf("Success!.\n");
	return 0;
}
