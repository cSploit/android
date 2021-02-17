/* Testing large objects */
/* Test from Sebastien Flaesch */

#include "common.h"
#include <ctype.h>
#include <assert.h>

static char software_version[] = "$Id: blob1.c,v 1.24 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define NBYTES 10000u

static int failed = 0;

static void
fill_chars(char *buf, size_t len, unsigned int start, unsigned int step)
{
	size_t n;

	for (n = 0; n < len; ++n)
		buf[n] = 'a' + ((start+n) * step % ('z' - 'a' + 1));
}

static void
fill_hex(char *buf, size_t len, unsigned int start, unsigned int step)
{
	size_t n;

	for (n = 0; n < len; ++n)
		sprintf(buf + 2*n, "%2x", (unsigned int)('a' + ((start+n) * step % ('z' - 'a' + 1))));
}


static int
check_chars(const char *buf, size_t len, unsigned int start, unsigned int step)
{
	size_t n;

	for (n = 0; n < len; ++n)
		if (buf[n] != 'a' + ((start+n) * step % ('z' - 'a' + 1)))
			return 0;

	return 1;
}

static int
check_hex(const char *buf, size_t len, unsigned int start, unsigned int step)
{
	size_t n;
	char symbol[3];

	for (n = 0; n < len; ++n) {
		sprintf(symbol, "%2x", (unsigned int)('a' + ((start+n) / 2 * step % ('z' - 'a' + 1))));
		if (tolower((unsigned char) buf[n]) != symbol[(start+n) % 2])
			return 0;
	}

	return 1;
}

#define MAX_TESTS 10
typedef struct {
	unsigned num;
	SQLSMALLINT c_type, sql_type;
	const char *db_type;
	unsigned gen1, gen2;
	SQLLEN vind;
	char *buf;
} test_info;

static test_info test_infos[MAX_TESTS];
static unsigned num_tests = 0;

static void
dump(FILE* out, const char* start, void* buf, unsigned len)
{
	unsigned n;
	char s[17];
	if (len >= 16)
		len = 16;
	fprintf(out, "%s", start);
	for (n = 0; n < len; ++n) {
		unsigned char c = ((unsigned char*)buf)[n];
		fprintf(out, " %02X", c);
		s[n] = (c >= 0x20 && c < 127) ? (char) c : '.';
	}
	s[len] = 0;
	fprintf(out, " - %s\n", s);
}

static void
readBlob(test_info *t)
{
	SQLRETURN rc;
	char buf[4096];
	SQLLEN len, total = 0;
	int i = 0;
	int check;

	printf(">> readBlob field %d\n", t->num);
	while (1) {
		i++;
		rc = CHKGetData(t->num, SQL_C_BINARY, (SQLPOINTER) buf, (SQLINTEGER) sizeof(buf), &len, "SINo");
		if (rc == SQL_NO_DATA || len <= 0)
			break;
		if (len > (SQLLEN) sizeof(buf))
			len = (SQLLEN) sizeof(buf);
		printf(">>     step %d: %d bytes readed\n", i, (int) len);
		check = check_chars(buf, len, t->gen1 + total, t->gen2);
		if (!check) {
			fprintf(stderr, "Wrong buffer content\n");
			dump(stderr, " buf ", buf, len);
			failed = 1;
		}
		total += len;
	}
	printf(">>   total bytes read = %d \n", (int) total);
	if (total != 10000) {
		fprintf(stderr, "Wrong buffer length, expected 20000\n");
		failed = 1;
	}
}

static void
readBlobAsChar(test_info *t, int step, int wide)
{
	SQLRETURN rc = SQL_SUCCESS_WITH_INFO;
	char buf[8192];
	SQLLEN len, total = 0, len2;
	int i = 0;
	int check;
	int bufsize;

	SQLSMALLINT type = SQL_C_CHAR;
	unsigned int char_len = 1;
	if (wide) {
		char_len = sizeof(SQLWCHAR);
		type = SQL_C_WCHAR;
	}
	
	if (step%2) bufsize = sizeof(buf) - char_len;
	else bufsize = sizeof(buf);

	printf(">> readBlobAsChar field %d\n", t->num);
	while (rc == SQL_SUCCESS_WITH_INFO) {
		i++;
		rc = CHKGetData(t->num, type, (SQLPOINTER) buf, (SQLINTEGER) bufsize, &len, "SINo");
		if (rc == SQL_SUCCESS_WITH_INFO && len == SQL_NO_TOTAL) {
			len = bufsize - char_len;
			rc = SQL_SUCCESS;
		}
		if (rc == SQL_NO_DATA || len <= 0)
			break;
		rc = CHKGetData(t->num, type, (SQLPOINTER) buf, 0, &len2, "SINo");
		if (rc == SQL_SUCCESS_WITH_INFO && len2 != SQL_NO_TOTAL)
			len = len - len2;
#if 0
		if (len > (SQLLEN) (bufsize - char_len))
			len = (SQLLEN) (bufsize - char_len);
		len -= len % (2u * char_len);
#endif
		printf(">>     step %d: %d bytes readed\n", i, (int) len);

		if (wide) {
			len /= sizeof(SQLWCHAR);
			odbc_from_sqlwchar((char *) buf, (SQLWCHAR *) buf, len + 1);
		}

		check =	check_hex(buf, len, 2*t->gen1 + total, t->gen2);
		if (!check) {
			fprintf(stderr, "Wrong buffer content\n");
			dump(stderr, " buf ", buf, len);
			failed = 1;
		}
		total += len;
	}
	printf(">>   total bytes read = %d \n", (int) total);
	if (total != 20000) {
		fprintf(stderr, "Wrong buffer length, expected 20000\n");
		failed = 1;
	}
}

static void
add_test(SQLSMALLINT c_type, SQLSMALLINT sql_type, const char *db_type, unsigned gen1, unsigned gen2)
{
	test_info *t = NULL;
	size_t buf_len;

	if (num_tests >= MAX_TESTS) {
		fprintf(stderr, "too max tests\n");
		exit(1);
	}

	t = &test_infos[num_tests++];
	t->num = num_tests;
	t->c_type = c_type;
	t->sql_type = sql_type;
	t->db_type = db_type;
	t->gen1 = gen1;
	t->gen2 = gen2;
	t->vind = 0;
	switch (c_type) {
	case SQL_C_CHAR:
		buf_len =  NBYTES*2+1;
		break;
	case SQL_C_WCHAR:
		buf_len = (NBYTES*2+1) * sizeof(SQLWCHAR);
		break;
	default:
		buf_len = NBYTES;
	}
	t->buf = (char*) malloc(buf_len);
	if (!t->buf) {
		fprintf(stderr, "memory error\n");
		exit(1);
	}
	if (c_type != SQL_C_CHAR && c_type != SQL_C_WCHAR)
		fill_chars(t->buf, NBYTES, t->gen1, t->gen2);
	else
		memset(t->buf, 0, buf_len);
	t->vind = SQL_LEN_DATA_AT_EXEC(buf_len);
}

static void
free_tests(void)
{
	while (num_tests > 0) {
		test_info *t = &test_infos[--num_tests];
		free(t->buf);
		t->buf = NULL;
	}
}

int
main(int argc, char **argv)
{
	SQLRETURN RetCode;
	SQLHSTMT old_odbc_stmt = SQL_NULL_HSTMT;
	int i;

	int key;
	SQLLEN vind0;
	int cnt = 2, wide;
	char sql[256];
	test_info *t = NULL;

	odbc_use_version3 = 1;
	odbc_connect();

	/* tests (W)CHAR/BINARY -> (W)CHAR/BINARY (9 cases) */
	add_test(SQL_C_BINARY, SQL_LONGVARCHAR,   "TEXT",  123, 1 );
	add_test(SQL_C_BINARY, SQL_LONGVARBINARY, "IMAGE", 987, 25);
	add_test(SQL_C_CHAR,   SQL_LONGVARBINARY, "IMAGE", 987, 25);
	add_test(SQL_C_CHAR,   SQL_LONGVARCHAR,   "TEXT",  343, 47);
	add_test(SQL_C_WCHAR,  SQL_LONGVARBINARY, "IMAGE", 561, 29);
	add_test(SQL_C_WCHAR,  SQL_LONGVARCHAR,   "TEXT",  698, 24);
	if (odbc_db_is_microsoft()) {
		add_test(SQL_C_BINARY, SQL_WLONGVARCHAR, "NTEXT", 765, 12);
		add_test(SQL_C_CHAR,   SQL_WLONGVARCHAR, "NTEXT", 237, 71);
		add_test(SQL_C_WCHAR,  SQL_WLONGVARCHAR, "NTEXT", 687, 68);
	}

	strcpy(sql, "CREATE TABLE #tt(k INT");
	for (t = test_infos; t < test_infos+num_tests; ++t)
		sprintf(strchr(sql, 0), ",f%u %s", t->num, t->db_type);
	strcat(sql, ",v INT)");
	odbc_command(sql);

	old_odbc_stmt = odbc_stmt;
	odbc_stmt = SQL_NULL_HSTMT;

	/* Insert rows ... */

	for (i = 0; i < cnt; i++) {
		/* MS do not save correctly char -> binary */
		if (!odbc_driver_is_freetds() && i)
			continue;

		CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &odbc_stmt, "S");

		strcpy(sql, "INSERT INTO #tt VALUES(?");
		for (t = test_infos; t < test_infos+num_tests; ++t)
			strcat(sql, ",?");
		strcat(sql, ",?)");
		CHKPrepare(T(sql), SQL_NTS, "S");

		CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &key, 0, &vind0, "S");
		for (t = test_infos; t < test_infos+num_tests; ++t)
			CHKBindParameter(t->num+1, SQL_PARAM_INPUT, t->c_type, t->sql_type, 0x10000000, 0, t->buf, 0, &t->vind, "S");

		CHKBindParameter(num_tests+2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &key, 0, &vind0, "S");

		key = i;
		vind0 = 0;

		printf(">> insert... %d\n", i);
		RetCode = CHKExecute("SINe");
		while (RetCode == SQL_NEED_DATA) {
			char *p;

			RetCode = CHKParamData((SQLPOINTER) & p, "SINe");
			printf(">> SQLParamData: ptr = %p  RetCode = %d\n", (void *) p, RetCode);
			if (RetCode == SQL_NEED_DATA) {
				for (t = test_infos; t < test_infos+num_tests && t->buf != p; ++t)
					;
				assert(t < test_infos+num_tests);
				if (t->c_type == SQL_C_CHAR || t->c_type == SQL_C_WCHAR) {
					unsigned char_len = 1;

					fill_hex(p, NBYTES, t->gen1, t->gen2);
					if (t->c_type == SQL_C_WCHAR) {
						char_len = sizeof(SQLWCHAR);
						odbc_to_sqlwchar((SQLWCHAR*) p, p, NBYTES * 2);
					}

					CHKPutData(p, (NBYTES - (i&1)) * char_len, "S");

					printf(">> param %p: total bytes written = %d\n", (void *) p, NBYTES - (i&1));

					CHKPutData(p + (NBYTES - (i&1)) * char_len, (NBYTES + (i&1)) * char_len, "S");

					printf(">> param %p: total bytes written = %d\n", (void *) p, NBYTES + (i&1));
				} else {
					CHKPutData(p, NBYTES, "S");

					printf(">> param %p: total bytes written = %d\n", (void *) p, NBYTES);
				}
			}
		}

		CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) odbc_stmt, "S");
		odbc_stmt = SQL_NULL_HSTMT;
	}

	/* Now fetch rows ... */

	for (wide = 0; wide < 2; ++wide)
	for (i = 0; i < cnt; i++) {
		/* MS do not save correctly char -> binary */
		if (!odbc_driver_is_freetds() && i)
			continue;


		CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &odbc_stmt, "S");

		if (odbc_db_is_microsoft()) {
			CHKSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_NONSCROLLABLE, SQL_IS_UINTEGER, "S");
			CHKSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY, (SQLPOINTER) SQL_SENSITIVE, SQL_IS_UINTEGER, "S");
		}

		strcpy(sql, "SELECT ");
		for (t = test_infos; t < test_infos+num_tests; ++t)
			sprintf(strchr(sql, 0), "f%u,", t->num);
		strcat(sql, "v FROM #tt WHERE k = ?");
		CHKPrepare(T(sql), SQL_NTS, "S");

		CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &i, 0, &vind0, "S");

		for (t = test_infos; t < test_infos+num_tests; ++t) {
			t->vind = SQL_DATA_AT_EXEC;
			CHKBindCol(t->num, SQL_C_BINARY, NULL, 0, &t->vind, "S");
		}
		CHKBindCol(num_tests+1, SQL_C_LONG, &key, 0, &vind0, "S");

		vind0 = 0;

		CHKExecute("S");

		CHKFetchScroll(SQL_FETCH_NEXT, 0, "S");
		printf(">> fetch... %d\n", i);

		for (t = test_infos; t < test_infos+num_tests; ++t) {
			if (t->c_type == SQL_C_CHAR || t->c_type == SQL_C_WCHAR)
				readBlobAsChar(t, i, wide);
			else
				readBlob(t);
		}

		CHKCloseCursor("S");
		CHKFreeHandle(SQL_HANDLE_STMT, (SQLHANDLE) odbc_stmt, "S");
		odbc_stmt = SQL_NULL_HSTMT;
	}

	odbc_stmt = old_odbc_stmt;

	free_tests();
	odbc_disconnect();

	if (!failed)
		printf("ok!\n");

	return failed ? 1 : 0;
}

