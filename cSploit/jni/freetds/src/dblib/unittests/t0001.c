/* 
 * Purpose: Log in, create a table, insert a few rows, select them, and log out.   
 * Functions: dbbind dbcmd dbcolname dberrhandle dbisopt dbmsghandle dbnextrow dbnumcols dbopen dbresults dbsetlogintime dbsqlexec dbuse 
 */

#include "common.h"

static char software_version[] = "$Id: t0001.c,v 1.29 2009-02-27 15:52:48 freddy77 Exp $";
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
	DBINT testint, erc;

	set_malloc_options();

	read_login_info(argc, argv);
	if (argc > 1) {
		argc -= optind;
		argv += optind;
	}

	fprintf(stdout, "Starting %s\n", argv[0]);
	add_bread_crumb();

	/* Fortify_EnterScope(); */
	dbinit();

	add_bread_crumb();
	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	fprintf(stdout, "About to logon as \"%s\"\n", USER);

	add_bread_crumb();
	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "t0001");

	if (argc > 1) {
		printf("server and login timeout overrides (%s and %s) detected\n", argv[0], argv[1]);
		strcpy(SERVER, argv[0]);
		i = atoi(argv[1]);
		if (i) {
			i = dbsetlogintime(i);
			printf("dbsetlogintime returned %s.\n", (i == SUCCEED)? "SUCCEED" : "FAIL");
		}
	}

	fprintf(stdout, "About to open \"%s\"\n", SERVER);

	add_bread_crumb();

	dbproc = dbopen(login, SERVER);
	if (!dbproc) {
		fprintf(stderr, "Unable to connect to %s\n", SERVER);
		return 1;
	}
	add_bread_crumb();
	dbloginfree(login);  
	add_bread_crumb();

	fprintf(stdout, "Using database \"%s\"\n", DATABASE);
	if (strlen(DATABASE)) {
		erc = dbuse(dbproc, DATABASE);
		assert(erc == SUCCEED);
	}
	add_bread_crumb();

#ifdef DBQUOTEDIDENT
	fprintf(stdout, "QUOTED_IDENTIFIER is %s\n", (dbisopt(dbproc, DBQUOTEDIDENT, NULL))? "ON":"OFF");
#endif
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}

	for (i = 0; i < rows_to_add && sql_cmd(dbproc) == SUCCEED; i++) {
		dbsqlexec(dbproc);
		while (dbresults(dbproc) == SUCCEED) {
			/* nop */
		}
	}

	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	add_bread_crumb();


	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		failed = 1;
		fprintf(stderr, "error: expected a result set, none returned.\n");
		exit(1);
	}
	add_bread_crumb();

	for (i = 1; i <= dbnumcols(dbproc); i++) {
		add_bread_crumb();
		printf("col %d is %s\n", i, dbcolname(dbproc, i));
		add_bread_crumb();
	}

	add_bread_crumb();
	if (SUCCEED != dbbind(dbproc, 1, INTBIND, -1, (BYTE *) & testint)) {
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
		memset(teststr, 'x', sizeof(teststr));
		teststr[0] = 0;
		teststr[sizeof(teststr) - 1] = 0;
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
		printf("Read a row of data -> %d |%s|\n", (int) testint, teststr);
	}


	add_bread_crumb();
	if (dbnextrow(dbproc) != NO_MORE_ROWS) {
		failed = 1;
		fprintf(stderr, "Was expecting no more rows\n");
		exit(1);
	}

	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
