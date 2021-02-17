/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

int TestMutexAlignment(CuTest *ct) {
	int procs, threads, alignment, lockloop;
	int max_procs, max_threads;
	char *bin;
	char cmdstr[1000], cmd[1000];

	/* Step 1: Check required binary file existence, set args */
#ifdef DB_WIN32
#ifdef _WIN64
#ifdef DEBUG
	bin = "x64\\Debug\\test_mutex.exe";
#else
	bin = "x64\\Release\\test_mutex.exe";
#endif
#else
#ifdef DEBUG
	bin = "Win32\\Debug\\test_mutex.exe";
#else
	bin = "Win32\\Release\\test_mutex.exe";
#endif
#endif
	sprintf(cmdstr, "%s -p %%d -t %%d -a %%d -n %%d >/nul 2>&1", bin);
	lockloop = 100;
	max_procs = 2;
	max_threads = 2;
#else
	bin = "./test_mutex";
	sprintf(cmdstr, "%s -p %%d -t %%d -a %%d -n %%d >/dev/null 2>&1", bin);
	lockloop = 2000;
	max_procs = 4;
	max_threads = 4;
#endif

	if (__os_exists(NULL, bin, NULL) != 0) {
		printf("Error! Can not find %s. It need to be built in order to\
		    run this test.\n", bin);
		CuAssert(ct, bin, 0);
		return (EXIT_FAILURE);
	}

	/* Step 2: Test with different combinations. */
	for (procs = 1; procs <= max_procs; procs *= 2) {
		for (threads= 1; threads <= max_threads; threads *= 2) {
			if (procs ==1 && threads == 1)
				continue;
			for (alignment = 32; alignment <= 128; alignment *= 2) {
				sprintf(cmd, cmdstr, procs, threads, alignment,
				    lockloop);
				printf("%s\n", cmd);
				CuAssert(ct, cmd, system(cmd) == 0);
			}
		}
	}
	return (EXIT_SUCCESS);
}
