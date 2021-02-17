/* 
 * Purpose: Test buffering
 * Functions: dbclrbuf dbgetrow dbsetopt 
 */
#if 0
	# Find functions with:
	sed -ne'/db/ s/.*\(db[[:alnum:]_]*\)(.*/\1/gp' src/dblib/unittests/t0002.c |sort -u |fmt
#endif

#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: t0002.c,v 1.25 2009-04-23 09:36:26 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int failed = 0;

static void
verify(int i, int testint, char *teststr)
{
	char expected[1024];

	sprintf(expected, "row %03d", i);

	if (testint != i) {
		failed = 1;
		fprintf(stderr, "Failed.  Expected i to be %d, was %d\n", i, testint);
		abort();
	}
	if (0 != strncmp(teststr, expected, strlen(expected))) {
		failed = 1;
		fprintf(stderr, "Failed.  Expected s to be |%s|, was |%s|\n", expected, teststr);
		abort();
	}
	printf("Read a row of data -> %d %s\n", testint, teststr);
}

int
main(int argc, char **argv)
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	DBINT testint;
	STATUS rc;
	int i, iresults;
	char teststr[1024];
	int rows_in_buffer, limit_rows;

	const int buffer_count = 10;
	const int rows_to_add = 50;

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
	DBSETLAPP(login, "t0002");

	fprintf(stdout, "About to open %s..%s\n", SERVER, DATABASE);

	add_bread_crumb();
	dbproc = dbopen(login, SERVER);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);
	add_bread_crumb();
	dbloginfree(login);
	add_bread_crumb();

	fprintf(stdout, "Setting row buffer to 10 rows\n");
#ifdef MICROSOFT_DBLIB
	dbsetopt(dbproc, DBBUFFER, "10");
#else
	dbsetopt(dbproc, DBBUFFER, "10", 0);
#endif
	add_bread_crumb();

	add_bread_crumb();
	sql_cmd(dbproc); /* drop table if exists */
	add_bread_crumb();
	dbsqlexec(dbproc);
	add_bread_crumb();
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}
	if (dbresults(dbproc) != NO_MORE_RESULTS) {
		fprintf(stdout, "Failed: dbresults call after NO_MORE_RESULTS should return NO_MORE_RESULTS.\n");
		failed = 1;
	}
	add_bread_crumb();

	sql_cmd(dbproc); /* create table */
	dbsqlexec(dbproc);
	while (dbresults(dbproc) != NO_MORE_RESULTS) {
		/* nop */
	}

	for (i = 1; i <= rows_to_add; i++) {
		sql_cmd(dbproc);
		dbsqlexec(dbproc);
		while (dbresults(dbproc) != NO_MORE_RESULTS) {
			/* nop */
		}
	}

	sql_cmd(dbproc);	/* two result sets */
	dbsqlexec(dbproc);
	add_bread_crumb();

	for (iresults=1; iresults <= 2; iresults++ ) {
		fprintf(stdout, "fetching resultset %i\n", iresults);
		if (dbresults(dbproc) != SUCCEED) {
			add_bread_crumb();
			fprintf(stderr, "Was expecting a result set %d.\n", iresults);
			if( iresults == 2 )
				fprintf(stderr, "Buffering with multiple resultsets is broken.\n");
			exit(1);
		}
		rows_in_buffer = 0;
		add_bread_crumb();

		for (i = 1; i <= dbnumcols(dbproc); i++) {
			add_bread_crumb();
			printf("col %d is [%s]\n", i, dbcolname(dbproc, i));
			add_bread_crumb();
		}

		add_bread_crumb();
		dbbind(dbproc, 1, INTBIND, 0, (BYTE *) & testint);
		add_bread_crumb();
		dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *) teststr);
		add_bread_crumb();

		/* Fetch a result set */
		/* Second resultset stops at row 46 */
		limit_rows = rows_to_add - (iresults == 2 ? 4 : 0);
		for (i=0; i < limit_rows;) {

			fprintf(stdout, "clearing %d rows from buffer\n", rows_in_buffer ? buffer_count - 1 : buffer_count);
#ifdef MICROSOFT_DBLIB
			if (i == 0) {
				rc = dbnextrow(dbproc);
				assert(rc == REG_ROW);
				++i;
				rows_in_buffer = 1;
			}
#endif
			dbclrbuf(dbproc, buffer_count);
			rows_in_buffer = rows_in_buffer ? 1 : 0;

			do {
				int rc;

				i++;
				add_bread_crumb();
				if (REG_ROW != (rc = dbnextrow(dbproc))) {
					failed = 1;
					fprintf(stderr, "Failed: Expected a row (%s:%d)\n", __FILE__, __LINE__);
					if (rc == BUF_FULL)
						fprintf(stderr, "Failed: dbnextrow returned BUF_FULL (%d).  Fix dbclrbuf.\n", rc);
					exit(1);
				}
				++rows_in_buffer;
				add_bread_crumb();
				verify(i, testint, teststr);
			} while (rows_in_buffer < buffer_count && i < limit_rows);

			if (rows_in_buffer == buffer_count) {
				/* The buffer should be full */
				assert(BUF_FULL == dbnextrow(dbproc));
			}
		}
		if (iresults == 1) {
			fprintf(stdout, "clearing %d rows from buffer\n", buffer_count);
			dbclrbuf(dbproc, buffer_count);
			while (dbnextrow(dbproc) != NO_MORE_ROWS) {
				abort(); /* All rows were read: should not enter loop */
			}
		}
	}
	printf("\n");

	/* 
	 * Now test the buffered rows.  
	 * Should be operating on rows 37-46 of 2nd resultset 
	 */
	rc = dbgetrow(dbproc, 1);
	add_bread_crumb();
	if(rc != NO_MORE_ROWS)	/* row 1 is not among the 31-40 in the buffer */
		fprintf(stderr, "Failed: dbgetrow returned %d.\n", rc);
	assert(rc == NO_MORE_ROWS);

	rc = dbgetrow(dbproc, 37);
	add_bread_crumb();
	if(rc != REG_ROW)
		fprintf(stderr, "Failed: dbgetrow returned %d.\n", rc);
	assert(rc == REG_ROW);
	verify(37, testint, teststr);	/* first buffered row should be 37 */

	rc = dbnextrow(dbproc);
	add_bread_crumb();
	if(rc != REG_ROW)
		fprintf(stderr, "Failed: dbgetrow returned %d.\n", rc);
	assert(rc == REG_ROW);
	verify(38, testint, teststr);	/* next buffered row should be 38 */

	rc = dbgetrow(dbproc, 11);
	add_bread_crumb();
	assert(rc == NO_MORE_ROWS);	/* only 10 (not 11) rows buffered */

	rc = dbgetrow(dbproc, 46);
	add_bread_crumb();
	assert(rc == REG_ROW);
	verify(46, testint, teststr);	/* last buffered row should be 46 */

	/* Attempt dbnextrow when buffer has no space (10 out of 10 in use). */
	rc = dbnextrow(dbproc);
	assert(rc == BUF_FULL);

	dbclrbuf(dbproc, 3);		/* remove rows 37, 38, and 39 */

	rc = dbnextrow(dbproc);
	add_bread_crumb();
	assert(rc == REG_ROW);
	verify(47, testint, teststr);	/* fetch row from database, should be 47 */

	rc = dbnextrow(dbproc);
	add_bread_crumb();
	assert(rc == REG_ROW);
	verify(48, testint, teststr);	/* fetch row from database, should be 48 */

	/* buffer contains 8 rows (40-47) try removing 10 rows */
	dbclrbuf(dbproc, buffer_count);

	while (dbnextrow(dbproc) != NO_MORE_ROWS) {
		/* waste rows 49-50 */
	}

	dbclose(dbproc); /* close while buffer not cleared: OK */

	add_bread_crumb();
	dbexit();
	add_bread_crumb();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
