#include "common.h"

static char software_version[] = "$Id: done_handling.c,v 1.11 2010-09-22 07:03:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/*
 * This test try do discovery how dblib process token looking for state
 * at every iteration. It issue a query to server and check
 * - row count (valid number means DONE processed)
 * - if possible to send another query (means state IDLE)
 * - if error readed (means ERROR token readed)
 * - if status present (PARAMRESULT token readed)
 * - if parameter present (PARAM token readed)
 * It try these query types:
 * - normal row
 * - normal row with no count
 * - normal row without rows
 * - error query
 * - store procedure call with output parameters
 */

/* Forward declarations of the error handler and message handler. */
static int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
static int msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname,
		       int line);

static DBPROCESS *dbproc;
static int silent = 0;
static int check_idle = 0;

/* print functions adapted from src/dblib/dblib.c */
static const char *
prdbretcode(int retcode)
{
	static char unknown[24];
	switch(retcode) {
	case REG_ROW:		return "REG_ROW/MORE_ROWS";
	case NO_MORE_ROWS:	return "NO_MORE_ROWS";
	case BUF_FULL:		return "BUF_FULL";
	case NO_MORE_RESULTS:	return "NO_MORE_RESULTS";
	case SUCCEED:		return "SUCCEED";
	case FAIL:		return "FAIL";
	default:
		sprintf(unknown, "oops: %u ??", retcode);
	}
	return unknown;
}

static const char *
prretcode(int retcode)
{
	static char unknown[24];
	switch(retcode) {
	case SUCCEED:		return "SUCCEED";
	case FAIL:		return "FAIL";
	case NO_MORE_RESULTS:	return "NO_MORE_RESULTS";
	default:
		sprintf(unknown, "oops: %u ??", retcode);
	}
	return unknown;
}

static void
query(const char comment[])
{
	if (comment)
		printf("%s\n", comment);
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
}

typedef const char* (*prfunc)(int);

static void
check_state(const char name[], prfunc print, int erc)
{
	printf("State %-15s %-20s ", name, print(erc));
	if (dbnumcols(dbproc) > 0)
		printf("COLS(%d) ", dbnumcols(dbproc));
	/* row count */
	if (dbcount(dbproc) >= 0)
		printf("ROWS(%d) ", (int) dbcount(dbproc));
	silent = 1;
	if (dbdata(dbproc, 1))
		printf("DATA ");
	silent = 0;
	/* if status present */
	if (dbretstatus(dbproc) == TRUE)
		printf("STATUS %d ", (int) dbretstatus(dbproc));
	/* if parameter present */
	if (dbnumrets(dbproc) > 0)
		printf("PARAMS ");
	/*
	 * if possible to send another query
	 * NOTE this must be the last
	 */
	if (check_idle) {
		silent = 1;
		dbcmd(dbproc, "declare @i int ");
		if (FAIL != dbsqlexec(dbproc))
			printf("IDLE ");
		silent = 0;
	}
	printf("\n");
}

static void
do_test(const char comment[])
{
	int ret;
	prfunc print_with = NULL;

	if (comment)
		printf("%s\n", comment);
	sql_cmd(dbproc);

	check_state("sqlexec ", prretcode, dbsqlexec(dbproc));

	check_state("nextrow ", prdbretcode, dbnextrow(dbproc));
	check_state("nextrow ", prdbretcode, dbnextrow(dbproc));
	check_state("results ", prretcode,   dbresults(dbproc));
	check_state("nextrow ", prdbretcode, dbnextrow(dbproc));
	check_state("nextrow ", prdbretcode, dbnextrow(dbproc));

	check_idle = 0;
	for (;;) {
		ret = dbresults(dbproc);
		check_state("results ", prretcode, ret);
		if (ret != SUCCEED) {
			print_with = prretcode;
			break;
		}

		do {
			ret = dbnextrow(dbproc);
			check_state("nextrow ", prdbretcode, ret);
		} while (ret == REG_ROW);
		print_with = prdbretcode;
	}
	check_state("more results?", print_with, ret);
}

int
main(int argc, char *argv[])
{
	const static int invalid_column_name = 207;
	LOGINREC *login;	/* Our login information. */
	int i;

	setbuf(stdout, NULL);
	read_login_info(argc, argv);

	if (dbinit() == FAIL)
		exit(1);

	dberrhandle(err_handler);
	dbmsghandle(msg_handler);

#if 0
	/* 
	 * FIXME: Should be able to use the common err/msg handlers, but something about
	 * the IDLE checking causes them to fail.  Not sure about purpose of IDLE checking.
	 * -- jkl January 2009
	 */
	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);
#endif

	login = dblogin();
	DBSETLUSER(login, USER);
	DBSETLPWD(login, PASSWORD);
	DBSETLAPP(login, __FILE__);

	dbproc = dbopen(login, SERVER);
	dbloginfree(login);
	if (!dbproc)
		exit(1);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);

	for (i=0; i < 6; i++)
		query(NULL);
#if 0	
	check_state("setup done ", prretcode, erc);
	
	printf("wasting results\n");
	while ((erc = dbresults(dbproc)) == SUCCEED) {
		while (dbnextrow(dbproc) == REG_ROW); /* no-op */
	}	
#endif
	check_idle = 1;

	do_test("normal row with rowcount on");
	query("turn rowcount off");
	do_test("normal row with rowcount off");
	query("turn rowcount back on");
	do_test("normal row without rows");
	dbsetuserdata(dbproc, (BYTE*) &invalid_column_name);
	do_test("error query");
	do_test("stored procedure call with output parameters");

	do_test("execute done2");

	query("drop done_test");

	query("drop done_test2");

	dbexit();
	return 0;
}

static int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	if (silent)
		return INT_CANCEL;

	fflush(stdout);
	fprintf(stderr, "DB-Library error (severity %d):\n\t%s\n", severity, dberrstr);

	if (oserr != DBNOERR)
		fprintf(stderr, "Operating-system error:\n\t%s\n", oserrstr ? oserrstr : "(null)");
	fflush(stderr);

	return INT_CANCEL;
}

static int
msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	if (silent)
		return 0;

	fflush(stdout);
	fprintf(stderr, "Msg %d, Level %d, State %d\n", (int) msgno, severity, msgstate);

	if (strlen(srvname) > 0)
		fprintf(stderr, "Server '%s', ", srvname);
	if (procname && strlen(procname) > 0) {
		fprintf(stderr, "Procedure '%s', ", procname);
		if (line > 0)
			fprintf(stderr, "Line %d", line);
	}

	fprintf(stderr, "\n\t%s\n", msgtext);
	fflush(stderr);

	return 0;
}
