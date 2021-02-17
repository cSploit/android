/* 
 * Purpose: Test bcp in, with dbvarylen()
 * Functions: bcp_colfmt bcp_columns bcp_exec bcp_init dbvarylen 
 */

#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: t0017.c,v 1.29 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int failed = 0;

int
main(int argc, char *argv[])
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	int i;
	RETCODE ret;
	int big_endian = 1;

	char *out_file = "t0017.out";
	static const char in_file_le[] = FREETDS_SRCDIR "/t0017.in";
	static const char in_file_be[] = FREETDS_SRCDIR "/t0017.in.be";
	const char *in_file = in_file_le;
	const char *err_file = "t0017.err";
	DBINT rows_copied;
	int num_cols = 0;
	int col_type[256];
	DBBOOL col_varylen[256];
	int prefix_len;

	if (((char *) &big_endian)[0] == 1)
		big_endian = 0;
	if (big_endian)
		in_file = in_file_be;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	set_malloc_options();

	read_login_info(argc, argv);
	printf("Starting %s\n", software_version);
	dbinit();

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	printf("About to logon ... ");

	login = dblogin();
	assert(login);
	BCP_SETL(login, TRUE);
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "t0017");
	printf("done\n");

	printf("Opening \"%s\" for \"%s\" ... ", SERVER, USER);
	dbproc = dbopen(login, SERVER);
	assert(dbproc);
	if (strlen(DATABASE)) {
		dbuse(dbproc, DATABASE);
	}
	dbloginfree(login);
	printf("done\n");

	printf("Creating table ... ");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}
	printf("done\n");

	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	/* BCP out */
	printf("bcp_init... ");
	ret = bcp_init(dbproc, "#dblib0017", out_file, err_file, DB_OUT);
	if (ret != SUCCEED)
		failed = 1;
	printf("done\n");

	printf("Issuing SELECT ... ");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	printf("done\nFetching metadata ... ");
	if (dbresults(dbproc) != FAIL) {
		num_cols = dbnumcols(dbproc);
		for (i = 0; i < num_cols; ++i) {
			col_type[i] = dbcoltype(dbproc, i + 1);
			col_varylen[i] = dbvarylen(dbproc, i + 1);
		}
		while (dbnextrow(dbproc) != NO_MORE_ROWS) {
		}
	}
	printf("done\n");

	printf("bcp_columns ... ");
	ret = bcp_columns(dbproc, num_cols);
	if (ret != SUCCEED)
		failed = 1;
	for (i = 0; i < num_cols; i++) {
		prefix_len = 0;
		if (col_type[i] == SYBIMAGE || col_type[i] == SYBTEXT) {
			prefix_len = 4;
		} else if (col_varylen[i]) {
			prefix_len = 1;
		}
		printf("bind %d prefix %d col_type %s\n", i, prefix_len, col_type[i] == SYBIMAGE ? "image" : "other");
		ret = bcp_colfmt(dbproc, i + 1, col_type[i], prefix_len, -1, NULL, 0, i + 1);
		if (ret == FAIL) {
			fprintf(stderr, "return from bcp_colfmt = %d\n", ret);
			failed = 1;
		}
	}
	printf("done\n");

	rows_copied = -1;
	printf("bcp_exec ... ");
	ret = bcp_exec(dbproc, &rows_copied);
	if (ret != SUCCEED || rows_copied != 1)
		failed = 1;

	printf("%d rows copied\n", rows_copied);

	/* delete rows */
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	/* 
	 * BCP in 
	 */
	printf("bcp_init... ");
	ret = bcp_init(dbproc, "#dblib0017", in_file, err_file, DB_IN);
	if (ret != SUCCEED)
		failed = 1;
	printf("done\n");

	printf("Issuing SELECT ... ");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	printf("done\nFetching metadata ... ");
	if (dbresults(dbproc) != FAIL) {
		num_cols = dbnumcols(dbproc);
		for (i = 0; i < num_cols; i++) {
			col_type[i] = dbcoltype(dbproc, i + 1);
			col_varylen[i] = dbvarylen(dbproc, i + 1);
		}
		while (dbnextrow(dbproc) != NO_MORE_ROWS) {
		}
	}
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}
	printf("done\n");

	printf("bcp_columns ... ");
	ret = bcp_columns(dbproc, num_cols);
	if (ret != SUCCEED)
		failed = 1;
	for (i = 0; i < num_cols; i++) {
		prefix_len = 0;
		if (col_type[i] == SYBIMAGE || col_type[i] == SYBTEXT) {
			prefix_len = 4;
		} else if (col_varylen[i]) {
			prefix_len = 1;
		}
		ret = bcp_colfmt(dbproc, i + 1, col_type[i], prefix_len, -1, NULL, 0, i + 1);
		if (ret == FAIL) {
			fprintf(stderr, "return from bcp_colfmt = %d\n", ret);
			failed = 1;
		}
	}
	printf("done\n");

	printf("bcp_exec ... ");
	rows_copied = -1;
	ret = bcp_exec(dbproc, &rows_copied);
	if (ret != SUCCEED || rows_copied != 1)
		failed = 1;
	printf("done\n");


	/* test we inserted correctly row */
	if (!failed) {
		sql_cmd(dbproc);
		dbsqlexec(dbproc);
		while (dbresults(dbproc) != NO_MORE_RESULTS) {
			while ((ret=dbnextrow(dbproc)) != NO_MORE_ROWS) {
				fprintf(stderr, "Invalid dbnextrow result %d executing query\n", ret);
				failed = 1;
			}
		}
	}

	printf("%d rows copied\n", rows_copied);
	dbclose(dbproc);
	dbexit();

	printf("dblib %s on %s\n", (failed ? "failed!" : "okay"), __FILE__);
	return failed ? 1 : 0;
}
