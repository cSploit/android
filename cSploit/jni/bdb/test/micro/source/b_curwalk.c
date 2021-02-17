/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int usage(void);

int
b_curwalk(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DBTYPE type;
	DBC *dbc;
	DBT key, data;
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
	DB_HEAP_RID rid;
#endif
	db_recno_t recno;
	u_int32_t cachesize, pagesize, walkflags;
	int ch, i, count, dupcount, j;
	int prev, ret, skipdupwalk, sorted, walkcount;
	char *ts, dbuf[32], kbuf[32];

	type = DB_BTREE;
	cachesize = 10 * MEGABYTE;
	pagesize = 16 * 1024;
	count = 100000;
	dupcount = prev = skipdupwalk = sorted = 0;
	walkcount = 1000;
	ts = "Btree";
	while ((ch = getopt(argc, argv, "C:c:d:P:pSst:w:")) != EOF)
		switch (ch) {
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'd':
			dupcount = atoi(optarg);
			break;
		case 'P':
			pagesize = (u_int32_t)atoi(optarg);
			break;
		case 'p':
			prev = 1;
			break;
		case 'S':
			skipdupwalk = 1;
			break;
		case 's':
			sorted = 1;
			break;
		case 't':
			switch (optarg[0]) {
			case 'B': case 'b':
				ts = "Btree";
				type = DB_BTREE;
				break;
			case 'H': case 'h':
				if (optarg[1] == 'E' || optarg[1] == 'e') {
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
					if (b_util_have_heap())
						return (0);
					ts = "Heap";
					type = DB_HEAP;
#else
					fprintf(stderr,
				"b_curwalk: Heap is not supported! \n");
					return (EXIT_SUCCESS);
#endif
				} else {
					if (b_util_have_hash())
						return (0);
					ts = "Hash";
					type = DB_HASH;
				}
				break;
			case 'Q': case 'q':
				if (b_util_have_queue())
					return (0);
				ts = "Queue";
				type = DB_QUEUE;
				break;
			case 'R': case 'r':
				ts = "Recno";
				type = DB_RECNO;
				break;
			default:
				return (usage());
			}
			break;
		case 'w':
			walkcount = atoi(optarg);
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (usage());

	/*
	 * Queue and Recno don't support duplicates.
	 */
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
	if (dupcount != 0 && 
	    (type == DB_QUEUE || type == DB_RECNO || type == DB_HEAP)) {
		fprintf(stderr,
	"b_curwalk: Queue, Recno and Heap don't support duplicates\n");
		return (usage());
	}
#else
	if (dupcount != 0 && (type == DB_QUEUE || type == DB_RECNO)) {
		fprintf(stderr,
		    "b_curwalk: Queue and Recno don't support duplicates\n");
		return (usage());
	}
#endif

#if DB_VERSION_MAJOR < 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 0
#define	DB_PREV_NODUP	0
	/*
	 * DB_PREV_NODUP wasn't available until after 3.0.55.
	 *
	 * For some reason, testing sorted duplicates doesn't work either.
	 * I don't really care about 3.0.55 any more, just ignore it.
	 */
	return (0);
#endif
	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	DB_BENCH_ASSERT(dbp->set_cachesize(dbp, 0, cachesize, 0) == 0);
	DB_BENCH_ASSERT(dbp->set_pagesize(dbp, pagesize) == 0);
	dbp->set_errfile(dbp, stderr);

	/* Set record length for Queue. */
	if (type == DB_QUEUE)
		DB_BENCH_ASSERT(dbp->set_re_len(dbp, 20) == 0);

	/* Set duplicates flag. */
	if (dupcount != 0)
		DB_BENCH_ASSERT(
		    dbp->set_flags(dbp, sorted ? DB_DUPSORT : DB_DUP) == 0);

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbp->open(
	    dbp, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#endif

	/* Initialize the data. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/* Insert count in-order key/data pairs. */
	data.data = dbuf;
	data.size = 20;
	if (type == DB_BTREE || type == DB_HASH) {
		key.size = 10;
		key.data = kbuf;
		for (i = 0; i < count; ++i) {
			(void)snprintf(kbuf, sizeof(kbuf), "%010d", i);
			for (j = 0; j <= dupcount; ++j) {
				(void)snprintf(dbuf, sizeof(dbuf), "%020d", j);
				DB_BENCH_ASSERT(
				    dbp->put(dbp, NULL, &key, &data, 0) == 0);
			}
		}
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
	} else if (type == DB_HEAP) {
		key.data = &rid;
		key.size = sizeof(rid);
		for (i = 0; i < count; ++i)
			DB_BENCH_ASSERT(
			    dbp->put(dbp, NULL, &key, &data, DB_APPEND) == 0);
#endif
	} else {
		key.data = &recno;
		key.size = sizeof(recno);
		for (i = 0, recno = 1; i < count; ++i, ++recno)
			DB_BENCH_ASSERT(
			    dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}

	walkflags = prev ?
	    (skipdupwalk ? DB_PREV_NODUP : DB_PREV) :
	    (skipdupwalk ? DB_NEXT_NODUP : DB_NEXT);

	/* Walk the cursor through the tree N times. */
	TIMER_START;
	for (i = 0; i < walkcount; ++i) {
		DB_BENCH_ASSERT(dbp->cursor(dbp, NULL, &dbc, 0) == 0);
		while ((ret = dbc->c_get(dbc, &key, &data, walkflags)) == 0)
			;
		DB_BENCH_ASSERT(ret == DB_NOTFOUND);
		DB_BENCH_ASSERT(dbc->c_close(dbc) == 0);
	}
	TIMER_STOP;

	printf("# %d %s %s cursor of %d 10/20 byte key/data items",
	    walkcount, ts, prev ?
	    (skipdupwalk ? "DB_PREV_NODUP" : "DB_PREV") :
	    (skipdupwalk ? "DB_NEXT_NODUP" : "DB_NEXT"),
	    count);
	if (dupcount != 0)
		printf(" with %d dups", dupcount);
	printf("\n");

	/*
	 * An "operation" is traversal of a single key/data pair -- not a
	 * return of the key/data pair, since some versions of this test
	 * skip duplicate key/data pairs.
	 *
	 * Use a "double" so we don't overflow.
	 */
	TIMER_DISPLAY((double)count * walkcount);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	return (EXIT_SUCCESS);
}

static int
usage()
{
	(void)fprintf(stderr, "%s\n\t%s\n",
	    "usage: b_curwalk [-pSs] [-C cachesz]",
	    "[-c cnt] [-d dupcnt] [-P pagesz] [-t type] [-w walkcnt]");
	return (EXIT_FAILURE);
}
