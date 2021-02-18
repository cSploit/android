#!/usr/sbin/dtrace -s
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 * 
 * apitrace.d - Trace the primary BDB API calls
 *
 * This script displays the entry to and return from each of the main API calls.
 *
 * The optional integer maxcount parameter directs the script to exit once that
 * many functions have been displayed.
 *
 * On a multiprocessor or multicore machine it is possible to see results which
 * are slightly out of order when a thread changes switches to another cpu.
 * The output can be sent through "sort -t@ -k2n" to order the lines by time.
 *
 * usage: apitrace.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 *
 */
#pragma D option quiet
#pragma D option flowindent
#pragma D option bufsize=8m
#pragma D option defaultargs

int eventcount;
int maxcount;
unsigned long long epoch;

self unsigned long long start;
this unsigned long long now;
this unsigned long long duration;

dtrace:::BEGIN
{
	eventcount = 0;
	maxcount = $1 != 0 ? $1 : -1;
	epoch = timestamp;
	printf("DB API call trace of process %d\n", $target);
	printf("Interrupt to display summary\n");
}

pid$target::db_*create:entry,
pid$target::__*_pp:entry
{
	self->start = timestamp;
	printf("called with (%x, %x, %x...) @ %u\n", arg0, arg1, arg2, self->start - epoch);
}

pid$target::db_*create:return,
pid$target::__*_pp:return
/self->start != 0/
{
	this->now = timestamp;
	this->duration = this->now - self->start;
	self->start = 0;
	eventcount++;
	printf("returns %d after %d ns @ %u\n", arg1, this->duration, this->now - epoch);
}

pid$target::db_*create:return,
pid$target::__*_pp:return
/eventcount == maxcount/
{
	printf("Exiting %s:%s count %d\n", probefunc, probename, eventcount);
	exit(0);
}
