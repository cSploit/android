/* 
 * Purpose: Test NULL behavior using dbbind
 */

#include "common.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

static char software_version[] = "$Id: null2.c,v 1.8 2010-10-26 08:12:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static DBPROCESS *dbproc = NULL;
static int failed = 0;

static int
ignore_msg_handler(DBPROCESS * dbproc, DBINT msgno, int state, int severity, char *text, char *server, char *proc, int line)
{
	return 0;
}

static int
ignore_err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	return INT_CANCEL;
}

static void
query(const char *query)
{
	printf("query: %s\n", query);
	dbcmd(dbproc, (char *) query);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
}

static int use_nullbind = 0;

static void
test0(int n,  const char * expected)
{
	DBINT ind, expected_ind;
	char text_buf[16];

	dbfcmd(dbproc, "select c from #null where n = %d", n);

	dbsqlexec(dbproc);

	if (dbresults(dbproc) != SUCCEED) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
		return;
	}

	dbbind(dbproc, 1, NTBSTRINGBIND, 0, (BYTE *)text_buf);
	if (use_nullbind)
		dbnullbind(dbproc, 1, &ind);

	memset(text_buf, 'a', sizeof(text_buf));
	ind = -5;

	if (dbnextrow(dbproc) != REG_ROW) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
		return;
	}

	text_buf[sizeof(text_buf) - 1] = 0;
	printf("ind %d text_buf -%s-\n", (int) ind, text_buf);

	expected_ind = 0;
	if (strcmp(expected, "aaaaaaaaaaaaaaa") == 0)
		expected_ind = -1;

	/* do not check indicator if not binded */
	if (!use_nullbind)
		ind = expected_ind;
	if (ind != expected_ind || strcmp(expected, text_buf) != 0) {
		fprintf(stderr, "expected_ind %d expected -%s-\n", (int) expected_ind, expected);
		failed = 1;
		dbcancel(dbproc);
		return;
	}

	if (dbnextrow(dbproc) != NO_MORE_ROWS) {
		fprintf(stderr, "Only one row expected\n");
		dbcancel(dbproc);
		failed = 1;
	}

	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
}


static void
test(const char *type, int give_err)
{
	RETCODE ret;

	query("if object_id('#null') is not NULL drop table #null");

	dberrhandle(ignore_err_handler);
	dbmsghandle(ignore_msg_handler);

	printf("create table #null (n int, c %s NULL)\n", type);
	dbfcmd(dbproc, "create table #null (n int, c %s NULL)", type);
	dbsqlexec(dbproc);

	ret = dbresults(dbproc);

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	if (ret != SUCCEED) {
		dbcancel(dbproc);
		if (!give_err)
			return;
		fprintf(stdout, "Was expecting a result set.\n");
		failed = 1;
		return;
	}

	query("insert into #null values(1, '')");
	query("insert into #null values(2, NULL)");
	query("insert into #null values(3, ' ')");
	query("insert into #null values(4, 'foo')");

	use_nullbind = 1;
	test0(1, "");
	test0(2, "aaaaaaaaaaaaaaa");
	test0(3, "");
	test0(4, "foo");

	use_nullbind = 0;
	test0(1, "");
	test0(2, "");
	test0(3, "");
	test0(4, "foo");

	query("drop table #null");
}

int
main(int argc, char **argv)
{
	LOGINREC *login;

	read_login_info(argc, argv);

	fprintf(stdout, "Starting %s\n", argv[0]);

	dbinit();

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	fprintf(stdout, "About to logon\n");

	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "thread");

	fprintf(stdout, "About to open \"%s\"\n", SERVER);

	dbproc = dbopen(login, SERVER);
	if (!dbproc) {
		fprintf(stderr, "Unable to connect to %s\n", SERVER);
		return 1;
	}

	dbloginfree(login);

	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);

	test("VARCHAR(10)", 1);
	test("CHAR(10)", 1);
	test("TEXT", 1);

	test("NVARCHAR(10)", 0);
#ifndef DBNTWIN32
	if (dbtds(dbproc) >= DBTDS_7_0)
		test("NTEXT", 0);
#endif

	test("VARCHAR(MAX)", 0);
#ifndef DBNTWIN32
	if (dbtds(dbproc) >= DBTDS_7_0)
		test("NVARCHAR(MAX)", 0);
#endif

	dbexit();

	return failed ? 1 : 0;
}

