/* 
 * Purpose: Test Some conversion, check trimming error and results
 * Functions: dbconvert dberrhandle dbmsghandle dbinit dbexit
 */

#include "common.h"
#include <ctype.h>

static char software_version[] = "$Id: t0019.c,v 1.13 2007-01-15 19:43:09 jklowden Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int failure = 0;

static const char *cur_result = "";
static const char *cur_test = "";
static int cur_line = 0;

int test(int srctype, const void *srcdata, int srclen, int dsttype, int dstlen);
int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);

int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	/*
	 * For server messages, cancel the query and rely on the
	 * message handler to spew the appropriate error messages out.
	 */
	if (dberr == SYBESMSG)
		return INT_CANCEL;

	if (dberr == 20049) {
		fprintf(stderr, "OK: anticipated error %d (%s) arrived\n", dberr, dberrstr);
	} else {
	fprintf(stderr,
		"DB-LIBRARY error (severity %d, dberr %d, oserr %d, dberrstr %s, oserrstr %s):\n",
		severity, dberr, oserr, dberrstr ? dberrstr : "(null)", oserrstr ? oserrstr : "(null)");
	}
	fflush(stderr);

	return INT_CANCEL;
}

int
test(int srctype, const void *srcdata, int srclen, int dsttype, int dstlen)
{
	DBCHAR buf[10];
	char s[20], *p;
	int i, len, correct;
	char out[256];

	memset(buf, '*', sizeof(buf));
	len = dbconvert(NULL, srctype, (const BYTE*) srcdata, srclen, dsttype, (BYTE*) buf, dstlen);

	/* build result string */
	sprintf(out, "len=%d", len);
	p = s;
	for (i = 0; i < sizeof(buf); ++i) {
		*p++ = isprint((unsigned char) buf[i]) ? buf[i] : '.';
		sprintf(strchr(out, 0), " %02X", (unsigned char) buf[i]);
	}
	*p = 0;

	correct = 0;
	if (len == -1 || len == FAIL) {
		if (strcmp(cur_result, "error") == 0)
			correct = 1;
	} else {
		if (strcmp(cur_result, out) == 0)
			correct = 1;
	}
	if (!correct) {
		failure = 1;
		printf("\nline: %d test: %s\n" "%s\n%s\n" "failed :( should be '%s'\n", cur_line, cur_test, s, out, cur_result);
	}
	return 0;
}

#define TEST(s,out) \
	{ cur_result = out; cur_line = __LINE__; cur_test = #s; test s; }

int
main(int argc, char *argv[])
{
	if (dbinit() == FAIL)
		return 1;

	dberrhandle(err_handler);
	dbmsghandle(syb_msg_handler);

	TEST((SYBBINARY, "ciao\0\0", 6, SYBBINARY, -2), "len=6 63 69 61 6F 00 00 2A 2A 2A 2A");
	TEST((SYBCHAR, "ciao  ", 6, SYBCHAR, -2), "len=6 63 69 61 6F 20 20 00 2A 2A 2A");
	TEST((SYBCHAR, "ciao\0\0", 6, SYBCHAR, -2), "len=6 63 69 61 6F 00 00 00 2A 2A 2A");
	TEST((SYBCHAR, "ciao  ", 6, SYBCHAR, -1), "len=4 63 69 61 6F 00 2A 2A 2A 2A 2A");
	TEST((SYBCHAR, "ciao\0\0", 6, SYBCHAR, -1), "len=6 63 69 61 6F 00 00 00 2A 2A 2A");
	TEST((SYBCHAR, "ciao  ", 6, SYBCHAR, 8), "len=6 63 69 61 6F 20 20 20 20 2A 2A");
	TEST((SYBCHAR, "ciao\0\0", 6, SYBCHAR, 8), "len=6 63 69 61 6F 00 00 20 20 2A 2A");
	TEST((SYBCHAR, "ciao  ", 6, SYBCHAR, 4), "error");
	TEST((SYBCHAR, "ciao\0\0", 6, SYBCHAR, 4), "error");
	TEST((SYBCHAR, "ciao  ", 6, SYBCHAR, 6), "len=6 63 69 61 6F 20 20 2A 2A 2A 2A");
	TEST((SYBCHAR, "ciao\0\0", 6, SYBCHAR, 6), "len=6 63 69 61 6F 00 00 2A 2A 2A 2A");

	dbexit();
	if (!failure)
		printf("All tests passed!\n");
	return failure;
}
