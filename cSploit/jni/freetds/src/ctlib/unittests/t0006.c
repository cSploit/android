#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>

static char software_version[] = "$Id: t0006.c,v 1.15 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

CS_CONTEXT *ctx;
static int allSuccess = 1;

typedef const char *STR;

static int
DoTest(
	      /* source information */
	      CS_INT fromtype, void *fromdata, CS_INT fromlen,
	      /* to information */
	      CS_INT totype, CS_INT tomaxlen,
	      /* expected result */
	      CS_RETCODE tores, void *todata, CS_INT tolen,
	      /* fields in string format */
	      STR sdecl,
	      STR sfromtype, STR sfromdata, STR sfromlen, STR stotype, STR stomaxlen, STR stores, STR stodata, STR stolen,
	      /* source line number for error reporting */
	      int line)
{
	CS_DATAFMT destfmt, srcfmt;
	CS_INT reslen;
	CS_RETCODE retcode;
	int i;
	char buffer[1024];
	const char *err = "";

	memset(&destfmt, 0, sizeof(destfmt));
	destfmt.datatype = totype;
	destfmt.maxlength = tomaxlen;

	memset(&srcfmt, 0, sizeof(srcfmt));
	srcfmt.datatype = fromtype;
	srcfmt.maxlength = fromlen;

	/*
	 * FIXME this fix some thing but if error cs_convert should return
	 * CS_UNUSED; note that this is defined 5.. a valid result ...
	 */
	reslen = 0;

	/*
	 * TODO: add special case for CS_CHAR_TYPE and give different
	 * flags and len
	 */

	/* do convert */
	memset(buffer, 23, sizeof(buffer));
	retcode = cs_convert(ctx, &srcfmt, fromdata, &destfmt, buffer, &reslen);

	/* test result of convert */
	if (tores != retcode) {
		err = "result";
		goto Failed;
	}

	/* test result len */
	if (reslen != tolen) {
		err = "result length";
		goto Failed;
	}

	/* test buffer */
	if (todata && memcmp(todata, buffer, tolen) != 0) {
		unsigned n;
		for (n = 0; n < tolen; ++n)
			printf("%02x ", ((unsigned char*)todata)[n]);
		printf("\n");
		for (n = 0; n < tolen; ++n)
			printf("%02x ", ((unsigned char*)buffer)[n]);
		printf("\n");

		err = "result data";
		goto Failed;
	}

	/* test other part of buffer */
	if (todata)
		memset(buffer, 23, tolen);
	for (i = 0; i < sizeof(buffer); ++i)
		if (buffer[i] != 23) {
			err = "buffer left";
			goto Failed;
		}

	/* success */
	return 0;
      Failed:
	fprintf(stderr, "Test %s failed (ret=%d len=%d)\n", err, (int) retcode, (int) reslen);
	fprintf(stderr, "line: %d\n  DO_TEST(%s,\n"
		"\t   %s,%s,%s,\n"
		"\t   %s,%s,\n"
		"\t   %s,%s,%s);\n", line, sdecl, sfromtype, sfromdata, sfromlen, stotype, stomaxlen, stores, stodata, stolen);
	allSuccess = 0;
	return 1;
}

#define DO_TEST(decl,fromtype,fromdata,fromlen,totype,tomaxlen,tores,todata,tolen) { \
 decl; \
 DoTest(fromtype,fromdata,fromlen,totype,tomaxlen,tores,todata,tolen,\
  #decl,#fromtype,#fromdata,#fromlen,#totype,#tomaxlen,#tores,#todata,#tolen,\
  __LINE__);\
}

int
main(int argc, char **argv)
{
	CS_RETCODE ret;
#ifdef tds_sysdep_int64_type
	volatile tds_sysdep_int64_type one = 1;
#endif
	int verbose = 1;

	fprintf(stdout, "%s: Testing conversion\n", __FILE__);

	ret = cs_ctx_alloc(CS_VERSION_100, &ctx);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Init failed\n");
		return 1;
	}

	/* TODO For each conversion test different values of fromlen and tolen */

	/* 
	 * * INT to everybody 
	 */
	DO_TEST(CS_INT test = 12345;
		CS_INT test2 = 12345,
		CS_INT_TYPE, &test, sizeof(test), CS_INT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = 12345;
		CS_INT test2 = 12345,
		CS_INT_TYPE, &test, sizeof(test), CS_INT_TYPE, sizeof(test2) * 2, CS_SUCCEED, &test2, sizeof(test2));
	/* FIXME: correct ?? */
	DO_TEST(CS_INT test = 12345;
		CS_INT test2 = 12345, 
		CS_INT_TYPE, &test, sizeof(test), CS_INT_TYPE, sizeof(CS_SMALLINT), CS_SUCCEED, &test2, sizeof(test2));

	DO_TEST(CS_INT test = 1234;
		CS_SMALLINT test2 = 1234, CS_INT_TYPE, &test, sizeof(test), CS_SMALLINT_TYPE, 1, CS_SUCCEED, &test2, sizeof(test2));
	/* biggest and smallest SMALLINT */
	DO_TEST(CS_INT test = 32767;
		CS_SMALLINT test2 = 32767,
		CS_INT_TYPE, &test, sizeof(test), CS_SMALLINT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = -32768;
		CS_SMALLINT test2 = -32768,
		CS_INT_TYPE, &test, sizeof(test), CS_SMALLINT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	/* overflow */
	DO_TEST(CS_INT test = 32768;
		CS_SMALLINT test2 = 12345, CS_INT_TYPE, &test, sizeof(test), CS_SMALLINT_TYPE, sizeof(test2), CS_FAIL, NULL, 0);
	DO_TEST(CS_INT test = -32769;
		CS_SMALLINT test2 = 12345, CS_INT_TYPE, &test, sizeof(test), CS_SMALLINT_TYPE, sizeof(test2), CS_FAIL, NULL, 0);

	/* biggest and smallest TINYINT */
	DO_TEST(CS_INT test = 255;
		CS_TINYINT test2 = 255,
		CS_INT_TYPE, &test, sizeof(test), CS_TINYINT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = 0;
		CS_TINYINT test2 = 0,
		CS_INT_TYPE, &test, sizeof(test), CS_TINYINT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	/* overflow */
	DO_TEST(CS_INT test = 256;
		CS_TINYINT test2 = 1, CS_INT_TYPE, &test, sizeof(test), CS_TINYINT_TYPE, sizeof(test2), CS_FAIL, NULL, 0);
	DO_TEST(CS_INT test = -1;
		CS_TINYINT test2 = 1, CS_INT_TYPE, &test, sizeof(test), CS_TINYINT_TYPE, sizeof(test2), CS_FAIL, NULL, 0);

	/* biggest and smallest BIT */
	DO_TEST(CS_INT test = 1;
		CS_BYTE test2 = 1, CS_INT_TYPE, &test, sizeof(test), CS_BIT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = 0;
		CS_BYTE test2 = 0, CS_INT_TYPE, &test, sizeof(test), CS_BIT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	/* overflow FIXME: or 1 if != 0 ?? */
	DO_TEST(CS_INT test = 2;
		CS_BYTE test2 = 1, CS_INT_TYPE, &test, sizeof(test), CS_BIT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = -1;
		CS_BYTE test2 = 1, CS_INT_TYPE, &test, sizeof(test), CS_BIT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	DO_TEST(CS_INT test = 1234;
		CS_REAL test2 = 1234.0,
		CS_INT_TYPE, &test, sizeof(test), CS_REAL_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = -8765;
		CS_REAL test2 = -8765.0,
		CS_INT_TYPE, &test, sizeof(test), CS_REAL_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	DO_TEST(CS_INT test = 1234;
		CS_FLOAT test2 = 1234.0,
		CS_INT_TYPE, &test, sizeof(test), CS_FLOAT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = -8765;
		CS_FLOAT test2 = -8765.0,
		CS_INT_TYPE, &test, sizeof(test), CS_FLOAT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	DO_TEST(CS_INT test = 1234678; CS_MONEY4 test2 = {
		1234678}
		, CS_INT_TYPE, &test, sizeof(test), CS_MONEY4_TYPE, sizeof(test2), CS_FAIL, NULL, 0);
	DO_TEST(CS_INT test = -8765; CS_MONEY4 test2 = {
		-8765 * 10000}
		, CS_INT_TYPE, &test, sizeof(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	/* strange money formatting */
	DO_TEST(CS_CHAR test[] = ""; CS_MONEY4 test2 = {
		0}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "."; CS_MONEY4 test2 = {
		0}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = ".12"; CS_MONEY4 test2 = {
		1200}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "++++-123"; CS_MONEY4 test2 = {
		-123 * 10000}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_FAIL, NULL, 0);
	DO_TEST(CS_CHAR test[] = "   -123"; CS_MONEY4 test2 = {
		-123 * 10000}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "   +123"; CS_MONEY4 test2 = {
		123 * 10000}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "+123.1234"; CS_MONEY4 test2 = {
		1231234}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "+123.123411"; CS_MONEY4 test2 = {
		1231234}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_CHAR test[] = "+123.12.3411"; CS_MONEY4 test2 = {
		1231234}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_FAIL, NULL, 0);
	DO_TEST(CS_CHAR test[] = "pippo"; CS_MONEY4 test2 = {
		0}
		, CS_CHAR_TYPE, test, strlen(test), CS_MONEY4_TYPE, sizeof(test2), CS_FAIL, NULL, 0);

	/* not terminated money  */
	DO_TEST(CS_CHAR test[] = "-1234567"; CS_MONEY4 test2 = {
		-1230000}
		, CS_CHAR_TYPE, test, 4, CS_MONEY4_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

#ifdef tds_sysdep_int64_type
	DO_TEST(CS_INT test = 1234678;
		CS_MONEY test2;
		test2.mnyhigh = ((one * 1234678) * 10000) >> 32;
		test2.mnylow = (CS_UINT) ((one * 1234678) * 10000),
		CS_INT_TYPE, &test, sizeof(test), CS_MONEY_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
	DO_TEST(CS_INT test = -8765;
		CS_MONEY test2;
		test2.mnyhigh = ((one * -8765) * 10000) >> 32;
		test2.mnylow = (CS_UINT) ((one * -8765) * 10000),
		CS_INT_TYPE, &test, sizeof(test), CS_MONEY_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));
#endif

	DO_TEST(CS_INT test = 12345;
		CS_CHAR test2[] = "12345",
		CS_INT_TYPE, &test, sizeof(test), CS_CHAR_TYPE, sizeof(test2), CS_SUCCEED, test2, sizeof(test2) - 1);

	{ CS_VARCHAR test2 = { 5, "12345"};
	memset(test2.str+5, 23, 251);
	DO_TEST(CS_INT test = 12345,
		CS_INT_TYPE,&test,sizeof(test),
		CS_VARCHAR_TYPE,sizeof(test2),
		CS_SUCCEED,&test2,sizeof(test2));
	}

	DO_TEST(CS_CHAR test[] = "12345";
		CS_INT test2 = 12345, CS_CHAR_TYPE, test, 5, CS_INT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	/* unterminated number */
	DO_TEST(CS_CHAR test[] = " - 12345";
		CS_INT test2 = -12, CS_CHAR_TYPE, test, 5, CS_INT_TYPE, sizeof(test2), CS_SUCCEED, &test2, sizeof(test2));

	/* to binary */
	DO_TEST(CS_CHAR test[] = "abc";
		CS_CHAR test2[] = "abc", CS_BINARY_TYPE, test, 3, CS_BINARY_TYPE, 3, CS_SUCCEED, test2, 3);
#if 0
	DO_TEST(CS_CHAR test[] = "abcdef";
		CS_CHAR test2[] = "ab", CS_BINARY_TYPE, test, 6, CS_BINARY_TYPE, 2, CS_FAIL, test2, 2);
	DO_TEST(CS_CHAR test[] = "abc";
		CS_CHAR test2[] = "ab", CS_BINARY_TYPE, test, 3, CS_BINARY_TYPE, 2, CS_FAIL, test2, 2);
#endif
	DO_TEST(CS_CHAR test[] = "616263";
		CS_CHAR test2[] = "abc", CS_CHAR_TYPE, test, 6, CS_BINARY_TYPE, 3, CS_SUCCEED, test2, 3);
	DO_TEST(CS_CHAR test[] = "616263646566";
		CS_CHAR test2[] = "abc", CS_CHAR_TYPE, test, 12, CS_BINARY_TYPE, 3, CS_FAIL, test2, 3);

	/* to char */
	DO_TEST(CS_INT test = 1234567;
		CS_CHAR test2[] = "1234567", CS_INT_TYPE, &test, sizeof(test), CS_CHAR_TYPE, 7, CS_SUCCEED, test2, 7);
	DO_TEST(CS_CHAR test[] = "abc";
		CS_CHAR test2[] = "ab", CS_CHAR_TYPE, test, 3, CS_CHAR_TYPE, 2, CS_FAIL, test2, 2);
	DO_TEST(CS_CHAR test[] = "abc";
		CS_CHAR test2[] = "616263", CS_BINARY_TYPE, test, 3, CS_CHAR_TYPE, 6, CS_SUCCEED, test2, 6);
	DO_TEST(CS_CHAR test[] = "abcdef";
		CS_CHAR test2[] = "616263", CS_BINARY_TYPE, test, 6, CS_CHAR_TYPE, 6, CS_FAIL, test2, 6);

	ret = cs_ctx_drop(ctx);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Drop failed\n");
		return 2;
	}

	if (verbose && allSuccess) {
		fprintf(stdout, "Test succeded\n");
	}
	return allSuccess ? 0 : 1;
}
