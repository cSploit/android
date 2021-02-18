#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * apitimes.d - Summarize time spent in DB API functions
 *
 * This script graphs the time spent in the main API calls, grouped by thread.
 *
 * The optional integer maxcount parameter directs the script to exit after
 * that many functions have been accumulated.
 *
 * usage: apitimes.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 */
#pragma D option quiet
#pragma D option defaultargs

self unsigned long long start;

dtrace:::BEGIN
{
	maxcount = $1 > 0 ? $1 : -1;
	functioncount = 0;
	printf("DB API times of process %d grouped by function; ", $target);
	printf("interrupt to display summary\n");
}

pid$target::db*_create:entry,
pid$target::__*_pp:entry
{
	self->start = timestamp;
}

pid$target::db*_create:return,
pid$target::__*_pp:return
/self->start != 0/
{
	@calltimes[tid, probefunc] = quantize(timestamp - self->start);
	self->start = 0;
	functioncount++;
}

pid$target::db*_create:return,
pid$target::__*_pp:return
/functioncount == maxcount/
{
	exit(0);
}

dtrace:::END
{
	printf("\n");
	printa("Times that thread %x spent in %s in nanoseconds %@a", @calltimes);
}
