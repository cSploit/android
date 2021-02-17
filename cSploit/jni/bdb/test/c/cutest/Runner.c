/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * A simple main function that gives a command line to the CuTest suite.
 * It lets users run the entire suites (default), or choose individual
 * suite(s), or individual tests.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "db.h"

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#endif

int append_case(char **cases, int *num_cases, const char *this_case);
int usage();
void CuTestAssertForDb(const char *msg, const char *file, int line);

const char *progname;

int main(int argc, char **argv)
{
#define MAX_CASES 1000
	extern char *optarg;
	extern int optind;
	char *suites[MAX_CASES], *tests[MAX_CASES];
	int ch, failed, i, num_suites, num_tests, verbose;
	char *test;

	progname = argv[0];

	num_suites = num_tests = verbose = 0;
	while ((ch = getopt(argc, argv, "s:t:v")) != EOF)
		switch (ch) {
		case 's':
			append_case(suites, &num_suites, optarg);
			break;
		case 't':
			append_case(tests, &num_tests, optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	/* Setup the assert to override the default DB one. */
	db_env_set_func_assert(CuTestAssertForDb);
	failed = 0;
	if (num_tests == 0 && num_suites == 0)
		failed = RunAllSuites();
	else {
		for(i = 0; i < num_suites; i++)
			failed += RunSuite(suites[i]);
		for(i = 0; i < num_tests; i++) {
			test = strchr(tests[i], ':');
			if (test == NULL) {
				fprintf(stderr, "Invalid test case: %s\n", tests[i]);
				continue;
			}
			/*
			 * Replace the ':' with NULL, to split the current
			 * value into two strings.
			 */
			*test = '\0';
			++test;
			failed += RunTest(tests[i], test);
		}
	}
	while(num_suites != 0)
		free(suites[--num_suites]);
	while(num_tests != 0)
		free(tests[--num_tests]);
	if (failed > 0)
		return (1);
	else
		return (0);
}

int append_case(char **cases, int *pnum_cases, const char *this_case)
{
	int num_cases;

	num_cases = *pnum_cases;

	if (num_cases >= MAX_CASES)
		return (1);

	cases[num_cases] = strdup(this_case);

	if (cases[num_cases] == NULL)
		return (1);

	++(*pnum_cases);
	return (0);
}

void CuTestAssertForDb(const char *msg, const char *file, int line)
{
	CuFail_Line(NULL, file, line, NULL, msg);
}

int
usage()
{
	(void)fprintf(stderr, "usage: %s %s\n", progname,
	    "[-s suite] [-t test] -v. Multiple test and suite args allowed.");
	return (EXIT_FAILURE);
}

