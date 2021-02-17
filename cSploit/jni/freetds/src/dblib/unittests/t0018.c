/* 
 * Purpose: Test behaviour of DBCOUNT() for updates and inserts
 * Functions: DBCOUNT dbbind dbnextrow dbnumcols dbopen dbresults dbsqlexec 
 */

#include "common.h"

static char software_version[] = "$Id: t0018.c,v 1.21 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };



int failed = 0;


int
main(int argc, char **argv)
{
	const int rows_to_add = 50;
	LOGINREC *login;
	DBPROCESS *dbproc;
	int i;
	char teststr[1024];
	DBINT testint;

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
	DBSETLAPP(login, "t0018");

	fprintf(stdout, "About to open\n");

	add_bread_crumb();
	dbproc = dbopen(login, SERVER);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);
	add_bread_crumb();
	dbloginfree(login);
	add_bread_crumb();

	fprintf(stdout, "creating table\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	fprintf(stdout, "insert\n");
	for (i = 0; i < rows_to_add; i++) {
		sql_cmd(dbproc);
		dbsqlexec(dbproc);
		while (dbresults(dbproc) != NO_MORE_RESULTS) {
			/* nop */
		}
		if (DBCOUNT(dbproc) != 1) {
			failed = 1;
			fprintf(stdout, "Was expecting a rows affect to be 1.");
			exit(1);
		}
	}

	fprintf(stdout, "select\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	add_bread_crumb();

	fprintf(stdout, "Checking for an empty result set.\n");
	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		failed = 1;
		fprintf(stdout, "Was expecting a result set.\n");
		exit(1);
	}
	add_bread_crumb();

	if(DBROWS(dbproc) != FAIL) {
		add_bread_crumb();
		failed = 1;
		fprintf(stdout, "Was expecting no rows to be available.\n");
		exit(1);
	}
	add_bread_crumb();

	fprintf(stdout, "Checking for a result set with content.\n");
	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		failed = 1;
		fprintf(stdout, "Was expecting a result set.");
		exit(1);
	}
	add_bread_crumb();

	for (i = 1; i <= dbnumcols(dbproc); i++) {
		add_bread_crumb();
		printf("col %d is %s\n", i, dbcolname(dbproc, i));
		add_bread_crumb();
	}

	add_bread_crumb();
	if (SUCCEED != dbbind(dbproc, 1, INTBIND, 0, (BYTE *) & testint)) {
		failed = 1;
		fprintf(stderr, "Had problem with bind\n");
		abort();
	}
	add_bread_crumb();
	if (SUCCEED != dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *) teststr)) {
		failed = 1;
		fprintf(stderr, "Had problem with bind\n");
		abort();
	}
	add_bread_crumb();

	add_bread_crumb();

	for (i = 0; i < rows_to_add; i++) {
	char expected[1024];

		sprintf(expected, "row %03d", i);

		add_bread_crumb();
		if (REG_ROW != dbnextrow(dbproc)) {
			failed = 1;
			fprintf(stderr, "Failed.  Expected a row\n");
			exit(1);
		}
		add_bread_crumb();
		if (testint != i) {
			failed = 1;
			fprintf(stderr, "Failed.  Expected i to be %d, was %d\n", i, (int) testint);
			abort();
		}
		if (0 != strncmp(teststr, expected, strlen(expected))) {
			failed = 1;
			fprintf(stdout, "Failed.  Expected s to be |%s|, was |%s|\n", expected, teststr);
			abort();
		}
		printf("Read a row of data -> %d %s\n", (int) testint, teststr);
	}


	add_bread_crumb();
	if (dbnextrow(dbproc) != NO_MORE_ROWS) {
		failed = 1;
		fprintf(stderr, "Was expecting no more rows\n");
		exit(1);
	}
	if (DBCOUNT(dbproc) != rows_to_add) {
		failed = 1;
		fprintf(stdout, "Was expecting a rows affect to be %d was %d.\n", rows_to_add, DBCOUNT(dbproc));
		exit(1);
	}

	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	add_bread_crumb();
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}
	if (DBCOUNT(dbproc) != rows_to_add) {
		failed = 1;
		fprintf(stdout, "Was expecting a rows affect to be %d was %d.\n", rows_to_add, DBCOUNT(dbproc));
		exit(1);
	} else {
		fprintf(stdout, "Number of rows affected by update = %d.\n", DBCOUNT(dbproc));
	}

	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
