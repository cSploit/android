/* 
 * Purpose: Test retrieving compute rows
 * Functions: dbaltbind dbaltcolid dbaltop dbalttype dbnumalts
 */

#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: t0023.c,v 1.16 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };


int failed = 0;


int
main(int argc, char *argv[])
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	int i;
	DBINT rowint;
	DBCHAR rowchar[2];
	DBCHAR rowdate[32];

	DBINT rowtype;
	DBINT computeint;
	DBCHAR computedate[32];

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
	DBSETLAPP(login, "t0023");

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
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}

	fprintf(stdout, "insert\n");

	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}

	fprintf(stdout, "select\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	add_bread_crumb();


	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		failed = 1;
		fprintf(stdout, "Was expecting a result set.\n");
		exit(1);
	}
	add_bread_crumb();


	for (i = 1; i <= dbnumcols(dbproc); i++) {
		add_bread_crumb();
		printf("col %d is %s\n", i, dbcolname(dbproc, i));
		add_bread_crumb();
	}

	add_bread_crumb();
	fprintf(stdout, "binding row columns\n");
	if (SUCCEED != dbbind(dbproc, 1, INTBIND, 0, (BYTE *) & rowint)) {
		failed = 1;
		fprintf(stderr, "Had problem with bind col1\n");
		abort();
	}
	add_bread_crumb();
	if (SUCCEED != dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *) rowchar)) {
		failed = 1;
		fprintf(stderr, "Had problem with bind col2\n");
		abort();
	}
	add_bread_crumb();
	if (SUCCEED != dbbind(dbproc, 3, STRINGBIND, 0, (BYTE *) rowdate)) {
		failed = 1;
		fprintf(stderr, "Had problem with bind col3\n");
		abort();
	}

	add_bread_crumb();

	fprintf(stdout, "testing compute clause 1\n");

	if (dbnumalts(dbproc, 1) != 1) {
		failed = 1;
		fprintf(stderr, "Had problem with dbnumalts 1\n");
		abort();
	}

	if (dbalttype(dbproc, 1, 1) != SYBINT4) {
		failed = 1;
		fprintf(stderr, "Had problem with dbalttype 1, 1\n");
		abort();
	}

	if (dbaltcolid(dbproc, 1, 1) != 1) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltcolid 1, 1\n");
		abort();
	}

	if (dbaltop(dbproc, 1, 1) != SYBAOPSUM) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltop 1, 1\n");
		abort();
	}

	if (SUCCEED != dbaltbind(dbproc, 1, 1, INTBIND, 0, (BYTE *) & computeint)) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltbind 1, 1\n");
		abort();
	}


	add_bread_crumb();

	fprintf(stdout, "testing compute clause 2\n");

	if (dbnumalts(dbproc, 2) != 1) {
		failed = 1;
		fprintf(stderr, "Had problem with dbnumalts 2\n");
		abort();
	}

	if (dbalttype(dbproc, 2, 1) != SYBDATETIME) {
		failed = 1;
		fprintf(stderr, "Had problem with dbalttype 2, 1\n");
		abort();
	}

	if (dbaltcolid(dbproc, 2, 1) != 3) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltcolid 2, 1\n");
		abort();
	}

	if (dbaltop(dbproc, 2, 1) != SYBAOPMAX) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltop 2, 1\n");
		abort();
	}

	if (SUCCEED != dbaltbind(dbproc, 2, 1, STRINGBIND, -1, (BYTE *) computedate)) {
		failed = 1;
		fprintf(stderr, "Had problem with dbaltbind 2, 1\n");
		abort();
	}

	add_bread_crumb();

	while ((rowtype = dbnextrow(dbproc)) != NO_MORE_ROWS) {

		if (rowtype == REG_ROW) {
			printf("gotten a regular row\n");
		}

		if (rowtype == 1) {
			printf("gotten a compute row for clause 1\n");
			printf("value of sum(col1) = %d\n", computeint);
		}

		if (rowtype == 2) {
			printf("gotten a compute row for clause 2\n");
			printf("value of max(col3) = %s\n", computedate);

		}
	}

	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
