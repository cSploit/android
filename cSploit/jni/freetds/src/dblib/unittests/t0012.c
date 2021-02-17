/* 
 * Purpose: Test datetime conversion as well as dbdata() & dbdatlen()
 * Functions: dbcmd dbdata dbdatecrack dbdatlen dbnextrow dbresults dbsqlexec
 */

#include "common.h"

static char software_version[] = "$Id: t0012.c,v 1.24 2009-02-27 15:52:48 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };
int failed = 0;


int
main(int argc, char *argv[])
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	char datestring[256];
	DBDATEREC dateinfo;
	DBDATETIME mydatetime;

	set_malloc_options();

	read_login_info(argc, argv);
	fprintf(stdout, "Starting %s\n", argv[0]);
	dbinit();

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	fprintf(stdout, "About to logon\n");

	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "t0012");

	dbproc = dbopen(login, SERVER);
	if (strlen(DATABASE)) {
		dbuse(dbproc, DATABASE);
	}
	dbloginfree(login);
	fprintf(stdout, "After logon\n");

	fprintf(stdout, "creating table\n");
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
	dbsqlexec(dbproc);
	dbresults(dbproc);

	while (dbnextrow(dbproc) != NO_MORE_ROWS) {
		/* Print the date info  */
		dbconvert(dbproc, dbcoltype(dbproc, 1), dbdata(dbproc, 1), dbdatlen(dbproc, 1), SYBCHAR, (BYTE*) datestring, -1);

		printf("%s\n", datestring);

		/* Break up the creation date into its constituent parts */
		memcpy(&mydatetime, (DBDATETIME *) (dbdata(dbproc, 1)), sizeof(DBDATETIME));
		dbdatecrack(dbproc, &dateinfo, &mydatetime);

		/* Print the parts of the creation date */
#ifdef MSDBLIB
		printf("\tYear = %d.\n", dateinfo.year);
		printf("\tMonth = %d.\n", dateinfo.month);
		printf("\tDay of month = %d.\n", dateinfo.day);
		printf("\tDay of year = %d.\n", dateinfo.dayofyear);
		printf("\tDay of week = %d.\n", dateinfo.weekday);
		printf("\tHour = %d.\n", dateinfo.hour);
		printf("\tMinute = %d.\n", dateinfo.minute);
		printf("\tSecond = %d.\n", dateinfo.second);
		printf("\tMillisecond = %d.\n", dateinfo.millisecond);
#else
		printf("\tYear = %d.\n", dateinfo.dateyear);
		printf("\tMonth = %d.\n", dateinfo.datemonth);
		printf("\tDay of month = %d.\n", dateinfo.datedmonth);
		printf("\tDay of year = %d.\n", dateinfo.datedyear);
		printf("\tDay of week = %d.\n", dateinfo.datedweek);
		printf("\tHour = %d.\n", dateinfo.datehour);
		printf("\tMinute = %d.\n", dateinfo.dateminute);
		printf("\tSecond = %d.\n", dateinfo.datesecond);
		printf("\tMillisecond = %d.\n", dateinfo.datemsecond);
#endif
	}

	dbclose(dbproc);
	dbexit();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	free_bread_crumb();
	return failed ? 1 : 0;
}
