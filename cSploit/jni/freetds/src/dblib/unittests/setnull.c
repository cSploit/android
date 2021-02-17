/* 
 * Purpose: dbnull behavior
 */

#include "common.h"
#include "replacements.h"

static char software_version[] = "$Id: setnull.c,v 1.9 2008-11-25 22:58:29 jklowden Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int failed = 0;
static DBPROCESS *dbproc = NULL;

static void
char_test(const char *null, int bindlen, const char *expected)
{
	char db_c[16];
	RETCODE ret;

	if (null) {
		fprintf(stderr, "\tdbsetnull(CHARBIND, %u, '%s').\n", (unsigned int) strlen(null), null);
		ret = dbsetnull(dbproc, CHARBIND, strlen(null), (BYTE *) null);
		if (ret != SUCCEED) {
			fprintf(stderr, "dbsetnull returned error %d\n", (int) ret);
			failed = 1;
		}
	}

	memset(db_c, '_', sizeof(db_c));
	strcpy(db_c, "123456");
	dbcmd(dbproc, "select convert(char(20), null)");

	dbsqlexec(dbproc);

	if (dbresults(dbproc) != SUCCEED) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
	}

	fprintf(stderr, "dbbind(CHARBIND, bindlen= %d).\n", bindlen);
	dbbind(dbproc, 1, CHARBIND, bindlen, (BYTE *) &db_c);
	db_c[sizeof(db_c)-1] = 0;
	printf("buffer before/after dbnextrow: '%s'/", db_c);

	if (dbnextrow(dbproc) != REG_ROW) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
	}
	db_c[sizeof(db_c)-1] = 0;
	printf("'%s'\n", db_c);

	if (dbnextrow(dbproc) != NO_MORE_ROWS) {
		fprintf(stderr, "Only one row expected\n");
		dbcancel(dbproc);
		failed = 1;
	}

	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}

	if (strcmp(db_c, expected) != 0) {
		fprintf(stderr, "Invalid NULL '%s' returned expected '%s' (%s:%d)\n", db_c, expected, tds_basename(__FILE__), __LINE__);
		failed = 1;
	}
}

int
main(int argc, char **argv)
{
	LOGINREC *login;
	DBINT db_i;
	RETCODE ret;
	
	read_login_info(argc, argv);

	printf("Starting %s\n", argv[0]);
	dbinit();

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "setnull");


	printf("About to open %s.%s\n", SERVER, DATABASE);

	dbproc = dbopen(login, SERVER);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);
	dbloginfree(login);

	/* try to set an int */
	db_i = 0xdeadbeef;
	ret = dbsetnull(dbproc, INTBIND, 0, (BYTE *) &db_i);
	if (ret != SUCCEED) {
		fprintf(stderr, "dbsetnull returned error %d\n", (int) ret);
		failed = 1;
	}

	ret = dbsetnull(dbproc, INTBIND, 1, (BYTE *) &db_i);
	if (ret != SUCCEED) {
		fprintf(stderr, "dbsetnull returned error %d\n", (int) ret);
		failed = 1;
	}

	/* check result */
	db_i = 0;
	dbcmd(dbproc, "select convert(int, null)");

	dbsqlexec(dbproc);

	if (dbresults(dbproc) != SUCCEED) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
	}

	dbbind(dbproc, 1, INTBIND, 0, (BYTE *) &db_i);
	printf("db_i = %ld\n", (long int) db_i);

	if (dbnextrow(dbproc) != REG_ROW) {
		fprintf(stderr, "Was expecting a row.\n");
		failed = 1;
		dbcancel(dbproc);
	}
	printf("db_i = %ld\n", (long int) db_i);

	if (dbnextrow(dbproc) != NO_MORE_ROWS) {
		fprintf(stderr, "Only one row expected\n");
		dbcancel(dbproc);
		failed = 1;
	}

	while (dbresults(dbproc) == SUCCEED) {
		/* nop */
	}

	if (db_i != 0xdeadbeef) {
		fprintf(stderr, "Invalid NULL %ld returned (%s:%d)\n", (long int) db_i, tds_basename(__FILE__), __LINE__);
		failed = 1;
	}

	/* try if dbset null consider length */
	for (db_i = 1; db_i > 0; db_i <<= 1) {
		printf("db_i = %ld\n", (long int) db_i);
		ret = dbsetnull(dbproc, INTBIND, db_i, (BYTE *) &db_i);
		if (ret != SUCCEED) {
			fprintf(stderr, "dbsetnull returned error %d for bindlen %ld\n", (int) ret, (long int) db_i);
			failed = 1;
			break;
		}
	}

	char_test(NULL, -1, "123456");
	char_test(NULL, 0,  "123456");
	char_test(NULL, 2,  "  3456");

	char_test("foo", -1,  "foo456");
	char_test("foo", 0,   "foo456");
/*	char_test("foo", 2,   ""); */
	char_test("foo", 5,   "foo  6");

	char_test("foo ", -1,  "foo 56");
	char_test("foo ", 0,   "foo 56");
/*	char_test("foo ", 2,   ""); */
	char_test("foo ", 5,   "foo  6");

	char_test("foo ", 10,  "foo       _____");

	printf("dblib %s on %s\n", (failed ? "failed!" : "okay"), __FILE__);
	dbexit();

	return failed ? 1 : 0;
}
