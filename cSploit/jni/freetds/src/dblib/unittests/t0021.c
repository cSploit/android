/* 
 * Purpose: Test dbsafestr()
 * Functions: dbsafestr 
 */

#include "common.h"

static char software_version[] = "$Id: t0021.c,v 1.15 2009-03-19 13:11:41 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#ifndef DBNTWIN32

int failed = 0;

/* unsafestr must contain one quote of each type */
const char *unsafestr = "This is a string with ' and \" in it.";

/* safestr must be at least strlen(unsafestr) + 3 */
char safestr[100];

int
main(int argc, char **argv)
{
	int len;
	RETCODE ret;

	set_malloc_options();

	fprintf(stdout, "Starting %s\n", argv[0]);

	/* Fortify_EnterScope(); */
	dbinit();


	len = strlen(unsafestr);
	ret = dbsafestr(NULL, unsafestr, -1, safestr, len, DBSINGLE);
	if (ret != FAIL)
		failed++;
	fprintf(stdout, "short buffer, single\n%s\n", safestr);
	/* plus one for termination and one for the quote */
	ret = dbsafestr(NULL, unsafestr, -1, safestr, len + 2, DBSINGLE);
	if (ret != SUCCEED)
		failed++;
	if (strlen(safestr) != len + 1)
		failed++;
	fprintf(stdout, "single quote\n%s\n", safestr);
	ret = dbsafestr(NULL, unsafestr, -1, safestr, len + 2, DBDOUBLE);
	if (ret != SUCCEED)
		failed++;
	if (strlen(safestr) != len + 1)
		failed++;
	fprintf(stdout, "double quote\n%s\n", safestr);
	ret = dbsafestr(NULL, unsafestr, -1, safestr, len + 3, DBBOTH);
	if (ret != SUCCEED)
		failed++;
	if (strlen(safestr) != len + 2)
		failed++;
	fprintf(stdout, "both quotes\n%s\n", safestr);

	dbexit();

	fprintf(stdout, "%s %s\n", __FILE__, (failed ? "failed!" : "OK"));
	return failed ? 1 : 0;
}
#else
int main(void)
{
	fprintf(stderr, "Not supported by MS DBLib\n");
	return 0;
}
#endif
