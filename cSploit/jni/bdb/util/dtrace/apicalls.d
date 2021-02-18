#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * apicalls.d - Summarize DB API function calls
 *
 * This script graphs the count of the main API calls grouped by thread.
 *
 * The optional integer maxcount parameter directs the script to exit once
 * that many functions calls have been accumulated.
 *
 * usage: apicalls.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 */
#pragma D option defaultargs

dtrace:::BEGIN
{
	maxcount = $1 > 0 ? $1 : -1;
	functioncount = 0;
	printf("DB API call counts of process %d; interrupt to display summary\n", $target);
}

pid$target::db*_create:return,
pid$target::__*_pp:return
{
	@calls[tid, probefunc] = count();
	functioncount++;
}

pid$target::db*_create:return,
pid$target::__*_pp:return
/functioncount == maxcount/
{
	exit(0);
}
