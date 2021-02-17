#!/usr/bin/env bash

# Auto generate single AllTests file for CuTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.
# Author: Asim Jalis
# Date: 01/08/2003

# Edited 2010 Alex G to match DB tree layout.

if test $# -eq 0 ; then FILES=../suites/*.c ; else FILES=$* ; fi


echo '

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
'

NEXT_STRING=`grep -h "int Test" ${FILES} | sed -e 's/ {/;/' -e 's/^/extern /'` 
echo "$NEXT_STRING
"

# Want to turn TestEnvConfig.c into:
# Function name RunEnvConfigTests
# Suite name TestEnvConfig
for f in `ls ${FILES}`; do
	SUITE_NAME=`basename $f .c`
	RUNNER_NAME=`echo $SUITE_NAME | sed -e 's/Test\(.*\)/Run\1Tests/'`
	TEST_STR=`grep "${SUITE_NAME}SuiteSetup" $f`
	if [ "$TEST_STR"x != "x" ]; then
		SETUP_FN=${SUITE_NAME}SuiteSetup
	else
		SETUP_FN="NULL"
	fi
	TEST_STR=`grep "${SUITE_NAME}SuiteTeardown" $f`
	if [ "$TEST_STR"x != "x" ]; then
		TEARDOWN_FN=${SUITE_NAME}SuiteTeardown
	else
		TEARDOWN_FN="NULL"
	fi
	echo \
"int $RUNNER_NAME(CuString *output)
{
	CuSuite *suite = CuSuiteNew(\"$SUITE_NAME\",
	    $SETUP_FN, $TEARDOWN_FN);
	int count;
"

	TEST_STR=`grep "${SUITE_NAME}TestSetup" $f`
	if [ "$TEST_STR"x != "x" ]; then
		SETUP_FN=${SUITE_NAME}TestSetup
	else
		SETUP_FN="NULL"
	fi
	TEST_STR=`grep "${SUITE_NAME}TestTeardown" $f`
	if [ "$TEST_STR"x != "x" ]; then
		TEARDOWN_FN=${SUITE_NAME}TestTeardown
	else
		TEARDOWN_FN="NULL"
	fi
	NEXT_STRING=`grep -h "int Test" $f | sed -e '/[Suite|Test]Setup/d' -e '/[Suite|Test]Teardown/d' -e 's/^int /\tSUITE_ADD_TEST(suite, /' -e "s/(CuTest \*ct) {/,\n\t    $SETUP_FN, $TEARDOWN_FN);/"`
	echo \
"$NEXT_STRING
"

	echo \
'	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	count = suite->failCount;
	CuSuiteDelete(suite);
	return (count);
}
'
done

echo \
'TestSuite g_suites[] = {'

for f in `ls ${FILES}`; do
	SUITE_NAME=`basename $f .c`
	RUNNER_NAME=`echo $SUITE_NAME | sed -e 's/Test\(.*\)/Run\1Tests/'`
	echo \
"	{ \"$SUITE_NAME\", $RUNNER_NAME },"
done
echo \
'	{ "", NULL },
};
'

