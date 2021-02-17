/* test win64 consistency */
#include "common.h"

static char software_version[] = "$Id: test64.c,v 1.11 2010-12-30 18:18:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/*
set ipd processed_ptr with
SQLParamOptions/SQLSetDescField/SQL_ATTR_PARAMS_PROCESSED_PTR
check always same value IPD->processed_ptr attr 
*/

static void
check_ipd_params(void)
{
	void *ptr, *ptr2;
	SQLHDESC desc;
	SQLINTEGER ind;

	CHKGetStmtAttr(SQL_ATTR_PARAMS_PROCESSED_PTR, &ptr, sizeof(ptr), NULL, "S");

	/* get IPD */
	CHKGetStmtAttr(SQL_ATTR_IMP_PARAM_DESC, &desc, sizeof(desc), &ind, "S");

	CHKR(SQLGetDescField, (desc, 0, SQL_DESC_ROWS_PROCESSED_PTR, &ptr2, sizeof(ptr2), &ind), "S");

	if (ptr != ptr2) {
		fprintf(stderr, "IPD inconsistency ptr %p ptr2 %p\n", ptr, ptr2);
		exit(1);
	}
}

static void
set_ipd_params1(SQLULEN *ptr)
{
	CHKSetStmtAttr(SQL_ATTR_PARAMS_PROCESSED_PTR, ptr, 0, "S");
}

static void
set_ipd_params2(SQLULEN *ptr)
{
	SQLHDESC desc;
	SQLINTEGER ind;

	/* get IPD */
	CHKGetStmtAttr(SQL_ATTR_IMP_PARAM_DESC, &desc, sizeof(desc), &ind, "S");

	CHKR(SQLSetDescField, (desc, 1, SQL_DESC_ROWS_PROCESSED_PTR, ptr, 0), "S");
}

static void
set_ipd_params3(SQLULEN *ptr)
{
	CHKParamOptions(2, ptr, "S");
}

typedef void (*rows_set_t)(SQLULEN*);

static const rows_set_t param_set[] = {
	set_ipd_params1,
	set_ipd_params2,
	set_ipd_params3,
	NULL
};

#define MALLOC_N(t, n) (t*) malloc(n*sizeof(t))

static void
test_params(void)
{
#define ARRAY_SIZE 2
	const rows_set_t *p;
	SQLULEN len;
	SQLUINTEGER *ids = MALLOC_N(SQLUINTEGER,ARRAY_SIZE);
	SQLLEN *id_lens = MALLOC_N(SQLLEN,ARRAY_SIZE);
	unsigned long int h, l;
	unsigned int n;

	for (n = 0; n < ARRAY_SIZE; ++n) {
		ids[n] = n;
		id_lens[n] = 0;
	}

	/* test setting just some test pointers */
	set_ipd_params1(int2ptr(0x01020304));
	check_ipd_params();
	set_ipd_params2(int2ptr(0xabcdef12));
	check_ipd_params();
	set_ipd_params3(int2ptr(0x87654321));
	check_ipd_params();

	/* now see results */
	for (p = param_set; *p != NULL; ++p) {
		odbc_reset_statement();
		len = 0xdeadbeef;
		len <<= 16;
		len <<= 16;
		len |= 12345678;

		(*p)(&len);
		check_ipd_params();

		CHKSetStmtAttr(SQL_ATTR_PARAMSET_SIZE, (void *) int2ptr(ARRAY_SIZE), 0, "S");
		CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 5, 0, ids, 0, id_lens, "S");

		odbc_command("INSERT INTO #tmp1(i) VALUES(?)");
		SQLMoreResults(odbc_stmt);
		for (n = 0; n < ARRAY_SIZE; ++n)
			SQLMoreResults(odbc_stmt);
		l = len;
		len >>= 16;
		h = len >> 16;
		l &= 0xfffffffflu;
		if (h != 0 || l != 2) {
			fprintf(stderr, "Wrong number returned in param rows high %lu low %lu\n", h, l);
			exit(1);
		}
	}

	free(ids);
	free(id_lens);
}

/*
set ird processed_ptr with
SQLExtendedFetch/SQLSetDescField/SQL_ATTR_ROWS_FETCHED_PTR
check always same value IRD->processed_ptr attr 
*/

static void
check_ird_params(void)
{
	void *ptr, *ptr2;
	SQLHDESC desc;
	SQLINTEGER ind;

	CHKGetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR, &ptr, sizeof(ptr), NULL, "S");

	/* get IRD */
	CHKGetStmtAttr(SQL_ATTR_IMP_ROW_DESC, &desc, sizeof(desc), &ind, "S");

	CHKR(SQLGetDescField, (desc, 0, SQL_DESC_ROWS_PROCESSED_PTR, &ptr2, sizeof(ptr2), &ind), "S");

	if (ptr != ptr2) {
		fprintf(stderr, "IRD inconsistency ptr %p ptr2 %p\n", ptr, ptr2);
		exit(1);
	}
}

static void
set_ird_params1(SQLULEN *ptr)
{
	CHKSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR, ptr, 0, "S");
}

static void
set_ird_params2(SQLULEN *ptr)
{
	SQLHDESC desc;
	SQLINTEGER ind;

	/* get IRD */
	CHKGetStmtAttr(SQL_ATTR_IMP_ROW_DESC, &desc, sizeof(desc), &ind, "S");

	CHKR(SQLSetDescField, (desc, 1, SQL_DESC_ROWS_PROCESSED_PTR, ptr, 0), "S");
}

static const rows_set_t row_set[] = {
	set_ird_params1,
	set_ird_params2,
	NULL
};

#define MALLOC_N(t, n) (t*) malloc(n*sizeof(t))

static void
test_rows(void)
{
	const rows_set_t *p;
	SQLULEN len;
	SQLUINTEGER *ids = MALLOC_N(SQLUINTEGER,ARRAY_SIZE);
	SQLLEN *id_lens = MALLOC_N(SQLLEN,ARRAY_SIZE);
	unsigned long int h, l;
	unsigned int n;

	for (n = 0; n < ARRAY_SIZE; ++n) {
		ids[n] = n;
		id_lens[n] = 0;
	}

	/* test setting just some test pointers */
	set_ird_params1(int2ptr(0x01020304));
	check_ird_params();
	set_ird_params2(int2ptr(0xabcdef12));
	check_ird_params();

	/* now see results */
	for (p = row_set; ; ++p) {
		const char *test_name = NULL;

		odbc_reset_statement();
		len = 0xdeadbeef;
		len <<= 16;
		len <<= 16;
		len |= 12345678;
		if (*p)
			(*p)(&len);
		check_ird_params();

#if 0
		CHKSetStmtAttr(SQL_ATTR_PARAMSET_SIZE, (void *) int2ptr(ARRAY_SIZE), 0, "S");
		CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 5, 0, ids, 0, id_lens, "S");
#endif

		CHKBindCol(1, SQL_C_ULONG, ids, 0, id_lens, "S");
		if (*p) {
			CHKSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE, (void *) int2ptr(ARRAY_SIZE), 0, "S");

			odbc_command("SELECT DISTINCT i FROM #tmp1");
			SQLFetch(odbc_stmt);
			test_name = "SQLSetStmtAttr";
		} else {
			CHKSetStmtAttr(SQL_ROWSET_SIZE, (void *) int2ptr(ARRAY_SIZE), 0, "S");
			odbc_command("SELECT DISTINCT i FROM #tmp1");
			CHKExtendedFetch(SQL_FETCH_NEXT, 0, &len, NULL, "S");
			test_name = "SQLExtendedFetch";
		}
		SQLMoreResults(odbc_stmt);

		l = len;
		len >>= 16;
		h = len >> 16;
		l &= 0xfffffffflu;
		if (h != 0 || l != 2) {
			fprintf(stderr, "Wrong number returned in rows high %lu(0x%lx) low %lu(0x%lx) test %s\n", h, h, l, l, test_name);
			exit(1);
		}

		if (!*p)
			break;
	}

	free(ids);
	free(id_lens);
}

int
main(void)
{
	if (sizeof(SQLLEN) != 8) {
		printf("Not possible for this platform.\n");
		return 0;
	}

	odbc_use_version3 = 1;
	odbc_connect();

	odbc_command("create table #tmp1 (i int)");

	test_params();
	test_rows();

	odbc_disconnect();
	printf("Done\n");
	return 0;
}

