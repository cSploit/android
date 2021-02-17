/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "CuTest.h"

/*-------------------------------------------------------------------------*
 * Functions added by Berkeley DB, that allow more control over which tests
 * are run. They assume a populated TestSuite array called g_suites
 * is available globally.
 *-------------------------------------------------------------------------*/

extern TestSuite g_suites[];
int RunAllSuites(void)
{
	int failCount, i;
	CuString *output;
	failCount = 0;
	for(i = 0; strlen(g_suites[i].name) != 0; i++) {
		printf("Running suite %s\n", g_suites[i].name);
		output = CuStringNew();
		failCount += g_suites[i].fn(output);
		printf("%s\n", output->buffer);
		CuStringDelete(output);
		printf("Finished suite %s\n", g_suites[i].name);
	}
	return (failCount);
}

int RunSuite(const char * suite)
{
	int failCount, i;
	CuString *output;
	failCount = 0;
	for(i = 0; strlen(g_suites[i].name) != 0; i++)
		if (strcmp(g_suites[i].name, suite) == 0) {
			output = CuStringNew();
			failCount = g_suites[i].fn(output);
			printf("%s\n", output->buffer);
			CuStringDelete(output);
			break;
		}
	
	return (failCount);
}

int RunTest(const char * suite, const char *test)
{
	fprintf(stderr, "TODO: Implement RunTest: %s:%s.\n", suite, test);
	return (1);
}

/*-------------------------------------------------------------------------*
 * CuStr
 *-------------------------------------------------------------------------*/

char* CuStrAlloc(int size)
{
	char* newStr = (char*) malloc( sizeof(char) * (size) );
	return newStr;
}

char* CuStrCopy(const char* old)
{
	int len = (int)strlen(old);
	char* newStr = CuStrAlloc(len + 1);
	if (newStr != NULL)
		strcpy(newStr, old);
	else
		fprintf(stderr, "%s: malloc in CuStrCopy.\n", CU_FAIL_HEADER);
	return newStr;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

void CuStringInit(CuString* str)
{
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (char*) malloc(sizeof(char) * str->size);
	str->buffer[0] = '\0';
}

CuString* CuStringNew(void)
{
	CuString* str = (CuString*) malloc(sizeof(CuString));
	CuStringInit(str);
	return str;
}

void CuStringDelete(CuString *str)
{
        if (!str) return;
        free(str->buffer);
        free(str);
}

int CuStringResize(CuString* str, int newSize)
{
	char *newStr;
	newStr = (char*) realloc(str->buffer, sizeof(char) * newSize);
	if (newStr != 0) {
		str->size = newSize;
		str->buffer = newStr;
		return (0);
	}
	return (ENOMEM);
}

int CuStringAppend(CuString* str, const char* text, int dump)
{
	int length;

	if (text == NULL) {
		text = "NULL";
	}

	length = (int)strlen(text);
	if ((str->length + length + 1 >= str->size)  &&
	    CuStringResize(str, str->length + length + 1 + STRING_INC) != 0) {
		if (dump) {
			fprintf(stderr, "%s:%s\n%s\n", CU_FAIL_HEADER,
			    "String append in test framework failed due to"
			    "malloc failure. Outputting appended text instead.",
			    text);
		}
		return (ENOMEM);
	}
	str->length += length;
	strcat(str->buffer, text);
	return (0);
}

int CuStringAppendChar(CuString* str, char ch, int dump)
{
	char text[2];
	text[0] = ch;
	text[1] = '\0';
	return (CuStringAppend(str, text, dump));
}

int CuStringAppendFormat(CuString* str, int dump, const char* format, ...)
{
	va_list argp;
	char buf[HUGE_STRING_LEN];
	va_start(argp, format);
	vsprintf(buf, format, argp);
	va_end(argp);
	return (CuStringAppend(str, buf, dump));
}

int CuStringInsert(CuString* str, const char* text, int pos, int dump)
{
	int length = (int)strlen(text);
	if (pos > str->length)
		pos = str->length;
	if ((str->length + length + 1 >= str->size) &&
	    CuStringResize(str, str->length + length + 1 + STRING_INC) != 0) {
		if (dump) {
			fprintf(stderr, "%s:%s\n%s\n", CU_FAIL_HEADER,
			    "String append in test framework failed due to"
			    "malloc failure. Outputting appended text instead.",
			    text);
		}
		return (ENOMEM);
	}
	memmove(str->buffer + pos + length, str->buffer + pos,
	    (str->length - pos) + 1);
	str->length += length;
	memcpy(str->buffer + pos, text, length);
	return (0);
}

/*-------------------------------------------------------------------------*
 * CuTest
 *-------------------------------------------------------------------------*/

void CuTestInit(CuTest* t, const char* name, TestFunction function, TestSetupFunction setup, TestTeardownFunction teardown)
{
	t->name = CuStrCopy(name);
	t->failed = 0;
	t->ran = 0;
	t->message = NULL;
	t->function = function;
	t->jumpBuf = NULL;
	t->TestSetup = setup;
	t->TestTeardown = teardown;
}

CuTest* CuTestNew(const char* name, TestFunction function, TestSetupFunction setup, TestTeardownFunction teardown)
{
	CuTest* tc = CU_ALLOC(CuTest);
	if (tc != NULL)
		CuTestInit(tc, name, function, setup, teardown);
	else
		fprintf(stderr, "%s: %s%s\n", CU_FAIL_HEADER,
		    "Error initializing test case: ", name);
	return tc;
}

void CuTestDelete(CuTest *t)
{
        if (!t) return;
        free(t->name);
        free(t);
}

void CuTestRun(CuTest* tc)
{
	jmp_buf buf;
	if (tc->TestSetup != NULL)
		(tc->TestSetup)(tc);
	tc->jumpBuf = &buf;
	if (setjmp(buf) == 0)
	{
		tc->ran = 1;
		(tc->function)(tc);
	}
	if (tc->TestTeardown != NULL)
		(tc->TestTeardown)(tc);
	tc->jumpBuf = 0;
}

static void CuFailInternal(CuTest* tc, const char* file, int line, CuString* string)
{
	char buf[HUGE_STRING_LEN];

	sprintf(buf, "%s:%d: ", file, line);
	(void)CuStringInsert(string, buf, 0, 1);

	if (tc == NULL)
	{
		/*
		 * Output the message now, it's come from overriding
		 * __db_assert.
		 * TODO: It'd be nice to somehow map this onto a CuTest, so the
		 *       assert can be effectively trapped. Since Berkeley DB
		 *       doesn't necessarily return error after an assert, so
		 *       a test case can "pass" even after a __db_assert.
		 *       Could trap this output, and map it back to the test
		 *       case, but I want to be careful about allowing multi-
		 *       threaded use at some stage.
		 */
		fprintf(stderr, "DB internal assert: %s\n", string->buffer);
	} else {
		tc->failed = 1;
		tc->message = string->buffer;
		if (tc->jumpBuf != 0) longjmp(*(tc->jumpBuf), 0);
	}
}

void CuFail_Line(CuTest* tc, const char* file, int line, const char* message2, const char* message)
{
	CuString string;

	CuStringInit(&string);
	if (message2 != NULL) 
	{
		CuStringAppend(&string, message2, 1);
		CuStringAppend(&string, ": ", 1);
	}
	CuStringAppend(&string, message, 1);
	CuFailInternal(tc, file, line, &string);
}

void CuAssert_Line(CuTest* tc, const char* file, int line, const char* message, int condition)
{
	if (condition) return;
	CuFail_Line(tc, file, line, NULL, message);
}

void CuAssertStrEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	const char* expected, const char* actual)
{
	CuString string;
	if ((expected == NULL && actual == NULL) ||
	    (expected != NULL && actual != NULL &&
	     strcmp(expected, actual) == 0))
	{
		return;
	}

	CuStringInit(&string);
	if (message != NULL) 
	{
		CuStringAppend(&string, message, 1);
		CuStringAppend(&string, ": ", 1);
	}
	CuStringAppend(&string, "expected <", 1);
	CuStringAppend(&string, expected, 1);
	CuStringAppend(&string, "> but was <", 1);
	CuStringAppend(&string, actual, 1);
	CuStringAppend(&string, ">", 1);
	CuFailInternal(tc, file, line, &string);
}

void CuAssertIntEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	int expected, int actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected <%d> but was <%d>", expected, actual);
	CuFail_Line(tc, file, line, message, buf);
}

void CuAssertDblEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	double expected, double actual, double delta)
{
	char buf[STRING_MAX];
	if (fabs(expected - actual) <= delta) return;
	sprintf(buf, "expected <%f> but was <%f>", expected, actual); 

	CuFail_Line(tc, file, line, message, buf);
}

void CuAssertPtrEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	void* expected, void* actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected pointer <0x%p> but was <0x%p>", expected, actual);
	CuFail_Line(tc, file, line, message, buf);
}


/*-------------------------------------------------------------------------*
 * CuSuite
 *-------------------------------------------------------------------------*/

void CuSuiteInit(CuSuite* testSuite, const char *name,
    SuiteSetupFunction setup, SuiteTeardownFunction teardown)
{
	testSuite->name = name;
	testSuite->count = 0;
	testSuite->failCount = 0;
	testSuite->SuiteSetup = setup;
	testSuite->SuiteTeardown = teardown;
	testSuite->context = NULL;
        memset(testSuite->list, 0, sizeof(testSuite->list));
}

CuSuite* CuSuiteNew(const char *name,
    SuiteSetupFunction setup, SuiteTeardownFunction teardown)
{
	CuSuite* testSuite = CU_ALLOC(CuSuite);
	if (testSuite != NULL)
		CuSuiteInit(testSuite, name, setup, teardown);
	else
		fprintf(stderr, "%s: %s%s\n", CU_FAIL_HEADER,
		    "Error initializing test suite: ", name);
	return testSuite;
}

void CuSuiteDelete(CuSuite *testSuite)
{
        unsigned int n;
        for (n=0; n < MAX_TEST_CASES; n++)
        {
                if (testSuite->list[n])
                {
                        CuTestDelete(testSuite->list[n]);
                }
        }
        free(testSuite);

}

void CuSuiteAdd(CuSuite* testSuite, CuTest *testCase)
{
	assert(testSuite->count < MAX_TEST_CASES);
	testSuite->list[testSuite->count] = testCase;
	testSuite->count++;
	testCase->suite = testSuite;
}

void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2)
{
	int i;
	for (i = 0 ; i < testSuite2->count ; ++i)
	{
		CuTest* testCase = testSuite2->list[i];
		CuSuiteAdd(testSuite, testCase);
	}
}

void CuSuiteRun(CuSuite* testSuite)
{
	int i;
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		CuTest* testCase = testSuite->list[i];
		if (testSuite->SuiteSetup != NULL)
			(testSuite->SuiteSetup)(testSuite);
		CuTestRun(testCase);
		if (testSuite->SuiteTeardown != NULL)
			(testSuite->SuiteTeardown)(testSuite);
		if (testCase->failed) { testSuite->failCount += 1; }
	}
}

void CuSuiteSummary(CuSuite* testSuite, CuString* summary)
{
	int i;
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		CuTest* testCase = testSuite->list[i];
		CuStringAppend(summary, testCase->failed ? "F" : ".", 1);
	}
	CuStringAppend(summary, "\n\n", 1);
}

void CuSuiteDetails(CuSuite* testSuite, CuString* details)
{
	int i;
	int failCount = 0;

	if (testSuite->failCount == 0)
	{
		int passCount = testSuite->count - testSuite->failCount;
		const char* testWord = passCount == 1 ? "test" : "tests";
		CuStringAppendFormat(details, 1, "OK (%d %s)\n", passCount, testWord);
	}
	else
	{
		if (testSuite->failCount == 1)
			CuStringAppend(details, "There was 1 failure:\n", 1);
		else
			CuStringAppendFormat(details, 1, "There were %d failures:\n", testSuite->failCount);

		for (i = 0 ; i < testSuite->count ; ++i)
		{
			CuTest* testCase = testSuite->list[i];
			if (testCase->failed)
			{
				failCount++;
				CuStringAppendFormat(details, 1, "%d) %s: %s\n",
					failCount, testCase->name, testCase->message);
			}
		}
		CuStringAppend(details, "\n!!!FAILURES!!!\n", 1);

		CuStringAppendFormat(details, 1, "Runs: %d ",   testSuite->count);
		CuStringAppendFormat(details, 1, "Passes: %d ", testSuite->count - testSuite->failCount);
		CuStringAppendFormat(details, 1, "Fails: %d\n",  testSuite->failCount);
	}
}
