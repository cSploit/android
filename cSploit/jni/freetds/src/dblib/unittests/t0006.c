/* 
 * Purpose: Test fetching 50 rows in two result sets with a 5000-row buffer.
 * Functions: dbbind dbcmd dbnextrow dbnumcols dbresults dbsetopt dbsqlexec
 */

#include "common.h"


static char software_version[] = "$Id: t0006.c,v 1.19 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };


static char teststr[1024];
static DBINT testint;

static int failed = 0;


static void
get_results(DBPROCESS * dbproc, int start)
{
	int current = start - 1;

	while (REG_ROW == dbnextrow(dbproc)) {
	char expected[1024];

		current++;
		sprintf(expected, "row %04d", current);

		add_bread_crumb();

		if (testint != current) {
			fprintf(stderr, "Failed.  Expected i to be %d, was %d\n", current, (int) testint);
			abort();
		}
		if (0 != strncmp(teststr, expected, strlen(expected))) {
			fprintf(stdout, "Failed.  Expected s to be |%s|, was |%s|\n", expected, teststr);
			abort();
		}
		printf("Read a row of data -> %d %s\n", (int) testint, teststr);
	}
}


int
main(int argc, char **argv)
{
	RETCODE rc;
	const int rows_to_add = 50;
	LOGINREC *login;
	DBPROCESS *dbproc;
	int i;

	set_malloc_options();

	read_login_info(argc, argv);
	fprintf(stdout, "Starting %s\n", argv[0]);
	add_bread_crumb();

	/* Fortify_EnterScope(); */
	dbinit();

	add_bread_crumb();
	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	fprintf(stdout, "About to logon\n");

	add_bread_crumb();
	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "t0006");
	DBSETLHOST(login, "ntbox.dntis.ro");

	fprintf(stdout, "About to open\n");

	add_bread_crumb();
	dbproc = dbopen(login, SERVER);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);
	add_bread_crumb();
	dbloginfree(login);
	add_bread_crumb();

#ifdef MICROSOFT_DBLIB
	dbsetopt(dbproc, DBBUFFER, "5000");
#else
	dbsetopt(dbproc, DBBUFFER, "5000", 0);
#endif

	add_bread_crumb();

	fprintf(stdout, "creating table\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	fprintf(stdout, "insert\n");
	for (i = 1; i < rows_to_add; i++) {
		sql_cmd(dbproc);
		dbsqlexec(dbproc);
		while (dbresults(dbproc) != NO_MORE_RESULTS) {
			/* nop */
		}
	}

	fprintf(stdout, "first select\n");
	if (SUCCEED != sql_cmd(dbproc)) {
		fprintf(stderr, "%s:%d: dbcmd failed\n", __FILE__, __LINE__);
		failed = 1;
	}
	if (SUCCEED != dbsqlexec(dbproc)) {
		fprintf(stderr, "%s:%d: dbsqlexec failed\n", __FILE__, __LINE__);
		failed = 1;
	}
	add_bread_crumb();


	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		fprintf(stdout, "%s:%d: Was expecting a result set.", __FILE__, __LINE__);
		failed = 1;
		exit(1);
	}
	add_bread_crumb();

	for (i = 1; i <= dbnumcols(dbproc); i++) {
		add_bread_crumb();
		printf("col %d is %s\n", i, dbcolname(dbproc, i));
		add_bread_crumb();
	}

	add_bread_crumb();
	dbbind(dbproc, 1, INTBIND, 0, (BYTE *) & testint);
	add_bread_crumb();
	dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *) teststr);
	add_bread_crumb();

	get_results(dbproc, 1);
	add_bread_crumb();

	testint = -1;
	strcpy(teststr, "bogus");
	fprintf(stdout, "second select\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	add_bread_crumb();


	if ((rc = dbresults(dbproc)) != SUCCEED) {
		add_bread_crumb();
		fprintf(stdout, "%s:%d: Was expecting a result set. (rc=%d)\n", __FILE__, __LINE__, rc);
		failed = 1;
	}

	if (!failed) {
		add_bread_crumb();

		add_bread_crumb();
		dbbind(dbproc, 1, INTBIND, 0, (BYTE *) & testint);
		add_bread_crumb();
		dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *) teststr);
		add_bread_crumb();

		get_results(dbproc, 25);
	}
	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
