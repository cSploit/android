/*
 * Test reading data with SQLGetData
 */
#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: getdata.c,v 1.21 2011-08-11 09:38:07 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
test_err(const char *data, int c_type, const char *state)
{
	char sql[128];
	SQLLEN ind;
	const unsigned int buf_size = 128;
	char *buf = (char *) malloc(buf_size);

	sprintf(sql, "SELECT '%s'", data);
	odbc_command(sql);
	SQLFetch(odbc_stmt);
	CHKGetData(1, c_type, buf, buf_size, &ind, "E");
	free(buf);
	odbc_read_error();
	if (strcmp(odbc_sqlstate, state) != 0) {
		fprintf(stderr, "Unexpected sql state returned\n");
		odbc_disconnect();
		exit(1);
	}
	odbc_reset_statement();
}

static int lc;
static int type;

static int
mycmp(const char *s1, const char *s2)
{
	SQLWCHAR buf[128], *wp;
	unsigned l;

	if (type == SQL_C_CHAR)
		return strcmp(s1, s2);

	l = strlen(s2);
	assert(l < (sizeof(buf)/sizeof(buf[0])));
	wp = buf;
	do {
		*wp++ = *s2;
	} while (*s2++);

	return memcmp(s1, buf, l * lc + lc);
}

static void
test_split(const char *n_flag)
{
#define CheckLen(x) do { \
	if (len != (x)) { \
		fprintf(stderr, "Wrong len %ld at line %d expected %d\n", (long int) len, __LINE__, (x)); \
		exit(1); \
	} \
	} while(0)

	char sql[80];
	char *buf = NULL;
	SQLLEN len;

	/* TODO test with VARCHAR too */
	sprintf(sql, "SELECT CONVERT(%sTEXT,'Prova' + REPLICATE('x',500))", n_flag);
	odbc_command(sql);

	CHKFetch("S");

	/* these 2 tests test an old severe BUG in FreeTDS */
	buf = ODBC_GET(1);
	CHKGetData(1, type, buf, 0, &len, "I");
	CheckLen(505*lc);
	CHKGetData(1, type, buf, 0, &len, "I");
	CheckLen(505*lc);
	buf = ODBC_GET(3*lc);
	CHKGetData(1, type, buf, 3 * lc, &len, "I");
	CheckLen(505*lc);
	if (mycmp(buf, "Pr") != 0) {
		printf("Wrong data result 1\n");
		exit(1);
	}

	buf = ODBC_GET(16*lc);
	CHKGetData(1, type, buf, 16 * lc, &len, "I");
	CheckLen(503*lc);
	if (mycmp(buf, "ovaxxxxxxxxxxxx") != 0) {
		printf("Wrong data result 2 res = '%s'\n", buf);
		exit(1);
	}

	buf = ODBC_GET(256*lc);
	CHKGetData(1, type, buf, 256 * lc, &len, "I");
	CheckLen(488*lc);
	CHKGetData(1, type, buf, 256 * lc, &len, "S");
	CheckLen(233*lc);
	CHKGetData(1, type, buf, 256 * lc, &len, "No");

	odbc_reset_statement();

	/* test with varchar, not blob but variable */
	sprintf(sql, "SELECT CONVERT(%sVARCHAR(100), 'Other test')", n_flag);
	odbc_command(sql);

	CHKFetch("S");

	buf = ODBC_GET(7*lc);
	CHKGetData(1, type, buf, 7 * lc, NULL, "I");
	if (mycmp(buf, "Other ") != 0) {
		printf("Wrong data result 1\n");
		exit(1);
	}

	buf = ODBC_GET(5*lc);
	CHKGetData(1, type, buf, 20, NULL, "S");
	if (mycmp(buf, "test") != 0) {
		printf("Wrong data result 2 res = '%s'\n", buf);
		exit(1);
	}
	ODBC_FREE();

	odbc_reset_statement();
}

int
main(int argc, char *argv[])
{
	char buf[32];
	SQLINTEGER int_buf;
	SQLLEN len;
	SQLRETURN rc;

	odbc_connect();

	lc = 1;
	type = SQL_C_CHAR;
	test_split("");

	lc = sizeof(SQLWCHAR);
	type = SQL_C_WCHAR;
	test_split("");

	if (odbc_db_is_microsoft() && odbc_db_version_int() >= 0x07000000u) {
		lc = 1;
		type = SQL_C_CHAR;
		test_split("N");

		lc = sizeof(SQLWCHAR);
		type = SQL_C_WCHAR;
		test_split("N");
	}

	/* test with fixed length */
	odbc_command("SELECT CONVERT(INT, 12345)");

	CHKFetch("S");

	int_buf = 0xdeadbeef;
	CHKGetData(1, SQL_C_SLONG, &int_buf, 0, NULL, "S");
	if (int_buf != 12345) {
		printf("Wrong data result\n");
		exit(1);
	}

	CHKGetData(1, SQL_C_SLONG, &int_buf, 0, NULL, "No");
	if (int_buf != 12345) {
		printf("Wrong data result 2 res = %d\n", (int) int_buf);
		exit(1);
	}

	odbc_reset_statement();

	/* test with numeric */
	odbc_command("SELECT CONVERT(NUMERIC(18,5), 1850000000000)");

	CHKFetch("S");

	memset(buf, 'x', sizeof(buf));
	CHKGetData(1, SQL_C_CHAR, buf, 14, NULL, "S");
	buf[sizeof(buf)-1] = 0;
	if (strcmp(buf, "1850000000000") != 0) {
		printf("Wrong data result: %s\n", buf);
		exit(1);
	}

	/* should give NO DATA */
	CHKGetData(1, SQL_C_CHAR, buf, 14, NULL, "No");
	buf[sizeof(buf)-1] = 0;
	if (strcmp(buf, "1850000000000") != 0) {
		printf("Wrong data result 3 res = %s\n", buf);
		exit(1);
	}

	odbc_reset_statement();


	/* test int to truncated string */
	odbc_command("SELECT CONVERT(INTEGER, 12345)");
	CHKFetch("S");

	/* error 22003 */
	memset(buf, 'x', sizeof(buf));
	CHKGetData(1, SQL_C_CHAR, buf, 4, NULL, "E");
#ifdef ENABLE_DEVELOPING
	buf[4] = 0;
	if (strcmp(buf, "xxxx") != 0) {
		fprintf(stderr, "Wrong buffer result buf = %s\n", buf);
		exit(1);
	}
#endif
	odbc_read_error();
	if (strcmp(odbc_sqlstate, "22003") != 0) {
		fprintf(stderr, "Unexpected sql state %s returned\n", odbc_sqlstate);
		odbc_disconnect();
		exit(1);
	}
	CHKGetData(1, SQL_C_CHAR, buf, 2, NULL, "No");
	odbc_reset_statement();

	/* test unique identifier to truncated string */
	rc = odbc_command2("SELECT CONVERT(UNIQUEIDENTIFIER, 'AA7DF450-F119-11CD-8465-00AA00425D90')", "SENo");
	if (rc != SQL_ERROR) {
		CHKFetch("S");

		/* error 22003 */
		CHKGetData(1, SQL_C_CHAR, buf, 17, NULL, "E");
		odbc_read_error();
		if (strcmp(odbc_sqlstate, "22003") != 0) {
			fprintf(stderr, "Unexpected sql state %s returned\n", odbc_sqlstate);
			odbc_disconnect();
			exit(1);
		}
		CHKGetData(1, SQL_C_CHAR, buf, 2, NULL, "No");
	}
	odbc_reset_statement();


	odbc_disconnect();

	odbc_use_version3 = 1;
	odbc_connect();

	/* test error from SQLGetData */
	/* wrong constant, SQL_VARCHAR is invalid as C type */
	test_err("prova 123",           SQL_VARCHAR,     "HY003");
	/* use ARD but no ARD data column */
	test_err("prova 123",           SQL_ARD_TYPE,    "07009");
	/* wrong conversion, int */
	test_err("prova 123",           SQL_C_LONG,      "22018");
	/* wrong conversion, date */
	test_err("prova 123",           SQL_C_TIMESTAMP, "22018");
	/* overflow */
	test_err("1234567890123456789", SQL_C_LONG,      "22003");

	/* test for empty string from mssql */
	if (odbc_db_is_microsoft()) {
		lc = 1;
		type = SQL_C_CHAR;

		for (;;) {
			void *buf = ODBC_GET(lc);

			odbc_command("SELECT CONVERT(TEXT,'')");

			CHKFetch("S");

			len = 1234;
			CHKGetData(1, type, buf, lc, &len, "S");

			if (len != 0) {
				fprintf(stderr, "Wrong len returned, returned %ld\n", (long) len);
				return 1;
			}

			CHKGetData(1, type, buf, lc, NULL, "No");
			odbc_reset_statement();
			ODBC_FREE();

			buf = ODBC_GET(4096*lc);

			odbc_command("SELECT CONVERT(TEXT,'')");

			CHKFetch("S");

			len = 1234;
			CHKGetData(1, type, buf, lc*4096, &len, "S");

			if (len != 0) {
				fprintf(stderr, "Wrong len returned, returned %ld\n", (long) len);
				return 1;
			}

			CHKGetData(1, type, buf, lc*4096, NULL, "No");
			odbc_reset_statement();
			ODBC_FREE();

			if (type != SQL_C_CHAR)
				break;
			lc = sizeof(SQLWCHAR);
			type = SQL_C_WCHAR;
		}	

		odbc_command("SELECT CONVERT(TEXT,'')");

		CHKFetch("S");

		len = 1234;
		CHKGetData(1, SQL_C_BINARY, buf, 1, &len, "S");

		if (len != 0) {
			fprintf(stderr, "Wrong len returned, returned %ld\n", (long) len);
			return 1;
		}

		CHKGetData(1, SQL_C_BINARY, buf, 1, NULL, "No");
	}

	odbc_disconnect();

	printf("Done.\n");
	return 0;
}
