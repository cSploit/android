/* 
 * Purpose: Test to see if row buffering and blobs works correctly.
 * Functions: dbbind dbnextrow dbopen dbresults dbsqlexec dbgetrow
 */

#include "common.h"

static char software_version[] = "$Id: text_buffer.c,v 1.6 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };


int
main(int argc, char **argv)
{
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
	DBSETLAPP(login, "text_buffer");
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
	dbsetopt(dbproc, DBBUFFER, "100");
#else
	dbsetopt(dbproc, DBBUFFER, "100", 0);
#endif
	add_bread_crumb();

	fprintf(stdout, "creating table\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	fprintf(stdout, "insert\n");
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}
	sql_cmd(dbproc);
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}


	fprintf(stdout, "select\n");
	dbcmd(dbproc, "select * from #dblib order by i");
	dbsqlexec(dbproc);
	add_bread_crumb();


	if (dbresults(dbproc) != SUCCEED) {
		add_bread_crumb();
		fprintf(stdout, "Was expecting a result set.");
		return 1;
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

	add_bread_crumb();


	if (REG_ROW != dbnextrow(dbproc)) {
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}
	if (dbdatlen(dbproc, 2) != 6 || 0 != strcmp("ABCDEF", teststr)) {
		fprintf(stderr, "Expected |%s|, found |%s|\n", "ABCDEF", teststr);
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}

	if (REG_ROW != dbnextrow(dbproc)) {
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}
	if (dbdatlen(dbproc, 2) != 3 || 0 != strcmp("abc", teststr)) {
		fprintf(stderr, "Expected |%s|, found |%s|\n", "abc", teststr);
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}

	/* get again row 1 */
	dbgetrow(dbproc, 1);

	/* here length and string should be ok */
	if (dbdatlen(dbproc, 2) != 6 || 0 != strcmp("ABCDEF", teststr)) {
		fprintf(stderr, "Expected |%s|, found |%s|\n", "ABCDEF", teststr);
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}

	dbgetrow(dbproc, 2);
	if (dbdatlen(dbproc, 2) != 3 || 0 != strcmp("abc", teststr)) {
		fprintf(stderr, "Expected |%s|, found |%s|\n", "abc", teststr);
		fprintf(stderr, "dblib failed for %s:%d\n", __FILE__, __LINE__);
		return 1;
	}

	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (0 ? "failed!" : "OK"));
	free_bread_crumb();
	return 0;
}
