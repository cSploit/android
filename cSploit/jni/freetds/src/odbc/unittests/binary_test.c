/**
 * Summary: Freetds binary patch test.
 * Author:  Gerhard Esterhuizen <ge AT swistgroup.com>
 * Date:    April 2003
 */

#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: binary_test.c,v 1.10 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define ERR_BUF_SIZE 256
/* 
   Name of table used by the test. Should contain single column,
   of IMAGE type and should contain NO rows at time of program invocation.
*/
#define TEST_TABLE_NAME "#binary_test"

/*
  Size (in bytes) of the test pattern written to and read from
  the database.
*/
#define TEST_BUF_LEN (1024*128)


static unsigned char *buf;

static void
show_error(const char *where, const char *what)
{
	printf("ERROR in %s: %s.\n", where, what);
}

static void
clean_up(void)
{
	free(buf);
	odbc_disconnect();
}

static int
test_insert(void *buf, SQLLEN buflen)
{
	SQLHSTMT odbc_stmt = SQL_NULL_HSTMT;
	SQLLEN strlen_or_ind;
	const char *qstr = "insert into " TEST_TABLE_NAME " values (?)";

	assert(odbc_conn);
	assert(odbc_env);

	/* allocate new statement handle */
	CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &odbc_stmt, "SI");

	/* execute query */
	CHKPrepare(T(qstr), SQL_NTS, "SI");

	strlen_or_ind = buflen;
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, (SQLUINTEGER) (-1), 0, buf, buflen,
			       &strlen_or_ind, "SI");

	CHKExecute("SI");

	/* this command shouldn't fail */
	odbc_command("DECLARE @i INT");

	SQLFreeHandle(SQL_HANDLE_STMT, odbc_stmt);
	return 0;
}


static int
test_select(void *buf, SQLINTEGER buflen, SQLLEN * bytes_returned)
{
	SQLHSTMT odbc_stmt = SQL_NULL_HSTMT;
	SQLLEN strlen_or_ind = 0;
	const char *qstr = "select * from " TEST_TABLE_NAME;

	assert(odbc_conn);
	assert(odbc_env);

	/* allocate new statement handle */
	CHKAllocHandle(SQL_HANDLE_STMT, odbc_conn, &odbc_stmt, "SI");

	/* execute query */
	CHKExecDirect(T(qstr), SQL_NTS, "SINo");

	/* fetch first page of first result row of unbound column */
	CHKFetch("S");

	strlen_or_ind = 0;
	CHKGetData(1, SQL_C_BINARY, buf, buflen, &strlen_or_ind, "SINo");

	*bytes_returned = ((strlen_or_ind > buflen || (strlen_or_ind == SQL_NO_TOTAL)) ? buflen : strlen_or_ind);

	/* ensure that this was the only row */
	CHKFetch("No");

	SQLFreeHandle(SQL_HANDLE_STMT, odbc_stmt);
	ODBC_FREE();
	return 0;
}

#define BYTE_AT(n) (((n) * 123) & 0xff)

int
main(int argc, char **argv)
{
	int i;
	SQLLEN bytes_returned;

	/* do not allocate so big memory in stack */
	buf = (unsigned char *) malloc(TEST_BUF_LEN);

	odbc_connect();

	odbc_command("create table " TEST_TABLE_NAME " (im IMAGE)");
	odbc_command("SET TEXTSIZE 1000000");

	/* populate test buffer with ramp */
	for (i = 0; i < TEST_BUF_LEN; i++) {
		buf[i] = BYTE_AT(i);
	}

	/* insert test pattern into database */
	if (test_insert(buf, TEST_BUF_LEN) == -1) {
		clean_up();
		return -1;
	}

	memset(buf, 0, TEST_BUF_LEN);

	/* read test pattern from database */
	if (test_select(buf, TEST_BUF_LEN, &bytes_returned) == -1) {
		clean_up();
		return -1;
	}

	/* compare inserted and read back test patterns */
	if (bytes_returned != TEST_BUF_LEN) {
		show_error("main(): comparing buffers", "Mismatch in input and output pattern sizes.");
		clean_up();
		return -1;
	}

	for (i = 0; i < TEST_BUF_LEN; ++i) {
		if (buf[i] != BYTE_AT(i)) {
			printf("mismatch at pos %d %d != %d\n", i, buf[i], BYTE_AT(i));
			show_error("main(): comparing buffers", "Mismatch in input and output patterns.");
			clean_up();
			return -1;
		}
	}

	printf("Input and output buffers of length %d match.\nTest passed.\n", TEST_BUF_LEN);
	clean_up();
	return 0;
}
