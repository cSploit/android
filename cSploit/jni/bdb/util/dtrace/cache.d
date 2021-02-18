#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * cache.d - Display DB cache activity
 *
 * usage: cache.d { -p <pid> | -c "<program> [<args]" } \
 *		[interval=1#seconds)] [iterations=60] [delay #seconds]
 *
 *	For each 'interval' seconds display overall and per-file cache stats:
 *		hits
 *		misses
 *		evictions
 *	Empty lines are displayed when nothing happened for that file & counter 
 *	during that interval.
 *
 *	The delay parameter delays the start of sampling until that many
 *	seconds have passed. 
 *
 * Static probes used:
 *	mpool-miss(unsigned misses, char *file, unsigned pgno)
 *	mpool-hit(unsigned hits, char *file, unsigned pgno)
 *	mpool-evict(char *file, unsigned pgno, BH *buf)
 */
#pragma D option defaultargs

dtrace:::BEGIN
{
	interval = $1 != 0 ? $1 : 10;
	maxtick = $2 != 0 ? $2 : 60;
	warmup = $3 != 0 ? $3 : 0;
	hits = misses = evictions = 0;
	secs = interval;

	tick = 0;
}

/* mpool-miss(unsigned misses, char *file, unsigned pgno)
 */
bdb$target:::mpool-miss	
/tick >= warmup/
{
	misses++;
	@misses[/*arg1 == 0 ? "<null!>" : */copyinstr(arg1)] = count();
}

/* mpool-hit(unsigned hits, char *file, unsigned pgno) */
bdb$target:::mpool-hit	
/tick >= warmup/
{
	hits++;
	@hits[arg1 == 0 ? "<null!>" : copyinstr(arg1)] = count();
}

/* mpool-evict(char *file, unsigned pgno, BH *buf */
bdb$target:::mpool-evict
/tick >= warmup/
{
	evictions++;
	@evictions[arg0 == 0 ? "<null!>" : copyinstr(arg0)] = count();
}

profile:::tick-1sec
{
	tick++;
	secs--;
}

/*
 * Print a banner when starting the measurement period.
 */
profile:::tick-1sec
/tick == warmup/
{
	printf("Cache info: %8s %8s %8s starting @ %Y\n", "hits", "misses",
	    "evictions", walltimestamp);
	secs = interval;
}

profile:::tick-1sec
/secs == 0 && tick >= warmup/
{
	printf(" %6d  %8d %8d %8d\n", tick, hits, misses, evictions);
	hits = misses = evictions = 0;
	printa("Hits for %20s %@u\n", @hits);
	printa("Misses for %20s %@u\n", @misses);
	printa("Evictions for %20s %@u\n", @evictions);
	trunc(@hits);
	trunc(@misses);
	trunc(@evictions);
	secs = interval;
}

profile:::tick-1sec
/tick == maxtick/
{
	exit(0);
}

dtrace:::END
{
	printf("\n");
	printa("Hits for %20s %@u\n", @hits);
	printa("Misses for %20s %@u\n", @misses);
	printa("Evictions for %20s %@u\n", @evictions);
}
