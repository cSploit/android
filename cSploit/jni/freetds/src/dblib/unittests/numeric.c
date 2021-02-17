#define MSDBLIB 1
#include "common.h"

static void
dump_addr(FILE *out, const char *msg, const void *p, size_t len)
{
	size_t n;
	if (msg)
		fprintf(out, "%s", msg);
	for (n = 0; n < len; ++n)
		fprintf(out, " %02X", ((unsigned char*) p)[n]);
	fprintf(out, "\n");
}

static void
chk(RETCODE ret, const char *msg)
{
	printf("res %d\n", ret);
	if (ret == SUCCEED)
		return;
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

static void
test(int bind_type)
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	DBNUMERIC *num = NULL, *num2 = NULL;
	RETCODE ret;
	int i;

	sql_rewind();
	login = dblogin();

	DBSETLUSER(login, USER);
	DBSETLPWD(login, PASSWORD);
	DBSETLAPP(login, "numeric");
	dbsetmaxprocs(25);
	DBSETLHOST(login, SERVER);

	dbproc = dbopen(login, SERVER);
	dbloginfree(login);
	login = NULL;
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);

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

	if (DBTDS_5_0 < DBTDS(dbproc)) {
		ret = dbcmd(dbproc,
			    "SET ARITHABORT ON;"
			    "SET CONCAT_NULL_YIELDS_NULL ON;"
			    "SET ANSI_NULLS ON;"
			    "SET ANSI_NULL_DFLT_ON ON;"
			    "SET ANSI_PADDING ON;"
			    "SET ANSI_WARNINGS ON;"
			    "SET ANSI_NULL_DFLT_ON ON;"
			    "SET CURSOR_CLOSE_ON_COMMIT ON;"
			    "SET QUOTED_IDENTIFIER ON");
		chk(ret, "dbcmd");
		ret = dbsqlexec(dbproc);
		chk(ret, "dbsqlexec");

		ret = dbcancel(dbproc);
		chk(ret, "dbcancel");
	}

	ret = dbrpcinit(dbproc, "testDecimal", 0);
	chk(ret, "dbrpcinit");

	num = (DBDECIMAL *) calloc(1, sizeof(DBDECIMAL));
	num->scale = 5;
	num->precision = 16;
	dbconvert(dbproc, SYBVARCHAR, (const BYTE *) "123.45", -1, SYBDECIMAL, (BYTE *) num, sizeof(*num));

	ret = dbrpcparam(dbproc, "@idecimal", 0, SYBDECIMAL, -1, sizeof(DBDECIMAL), (BYTE *) num);
	chk(ret, "dbrpcparam");
	ret = dbrpcsend(dbproc);
	chk(ret, "dbrpcsend");
	ret = dbsqlok(dbproc);
	chk(ret, "dbsqlok");

	/* TODO check MS/Sybase format */
	num2 = (DBDECIMAL *) calloc(1, sizeof(DBDECIMAL));
	num2->precision = 20;
	num2->scale = 10;
	dbconvert(dbproc, SYBVARCHAR, (const BYTE *) "246.9", -1, SYBDECIMAL, (BYTE *) num2, sizeof(*num2));

	for (i=0; (ret = dbresults(dbproc)) != NO_MORE_RESULTS; ++i) {
		RETCODE row_code;

		switch (ret) {
		case SUCCEED:
			if (DBROWS(dbproc) == FAIL)
				continue;
			assert(DBROWS(dbproc) == SUCCEED);
			printf("dbrows() returned SUCCEED, processing rows\n");

			memset(num, 0, sizeof(*num));
			num->precision = num2->precision;
			num->scale = num2->scale;
			dbbind(dbproc, 1, bind_type, 0, (BYTE *) num);

			while ((row_code = dbnextrow(dbproc)) != NO_MORE_ROWS) {
				if (row_code == REG_ROW) {
					if (memcmp(num, num2, sizeof(*num)) != 0) {
						fprintf(stderr, "Failed. Output results does not match\n");
						dump_addr(stderr, "numeric: ", num, sizeof(*num));
						dump_addr(stderr, "numeric2:", num2, sizeof(*num2));
						exit(1);
					}
				} else {
					/* not supporting computed rows in this unit test */
					fprintf(stderr, "Failed.  Expected a row\n");
					exit(1);
				}
			}
			break;
		case FAIL:
			fprintf(stderr, "dbresults returned FAIL\n");
			exit(1);
		default:
			fprintf(stderr, "unexpected return code %d from dbresults\n", ret);
			exit(1);
		}
	} /* while dbresults */

	sql_cmd(dbproc);

	free(num2);
	free(num);

	dbclose(dbproc);
}

int
main(int argc, char **argv)
{
	read_login_info(argc, argv);

	dbinit();

	test(DECIMALBIND);
	test(NUMERICBIND);

	dbexit();

	printf("Succeed\n");
	return 0;
}

