/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int b_load_usage(void);

int
b_load(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB *dbp;
	DBTYPE type;
	DBT key, data;
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
	DB_HEAP_RID rid;
#endif
	db_recno_t recno;
	u_int32_t cachesize;
	int ch, i, count, duplicate;
	char *ts, buf[32];

	type = DB_BTREE;
	cachesize = MEGABYTE;
	count = 100000;
	duplicate = 0;
	ts = "Btree";
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "C:c:dt:")) != EOF)
		switch (ch) {
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'd':
			duplicate = 1;
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
				return (b_load_usage());
			}
			break;
		case '?':
		default:
			return (b_load_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (b_load_usage());

	/* Usage. */
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
	if (duplicate && 
	    (type == DB_QUEUE || type == DB_RECNO || type == DB_HEAP)) {
		fprintf(stderr,
	"b_load: Queue, Recno and Heap don't support duplicates\n");
		return (b_load_usage());
	}
#else
	if (duplicate && (type == DB_QUEUE || type == DB_RECNO)) {
		fprintf(stderr,
		    "b_load: Queue an Recno don't support duplicates\n");
		return (b_load_usage());
	}
#endif

#if DB_VERSION_MAJOR < 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1
	/*
	 * DB versions prior to 3.1.17 didn't have off-page duplicates, so
	 * this test can run forever.
	 */
	if (duplicate)
		return (0);
#endif

	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	DB_BENCH_ASSERT(dbp->set_cachesize(dbp, 0, cachesize, 0) == 0);
	if (duplicate)
		DB_BENCH_ASSERT(dbp->set_flags(dbp, DB_DUP) == 0);
	dbp->set_errfile(dbp, stderr);

	/* Set record length for Queue. */
	if (type == DB_QUEUE)
		DB_BENCH_ASSERT(dbp->set_re_len(dbp, 20) == 0);

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
	DB_BENCH_ASSERT(
	    dbp->open(dbp, NULL, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(
	    dbp->open(dbp, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#endif

	/* Initialize the data. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/* Insert count in-order key/data pairs. */
	TIMER_START;
	if (duplicate) {
		key.size = 10;
		key.data = "01234567890123456789";
		data.data = buf;
		data.size = 20;
		for (i = 0; i < count; ++i) {
			(void)snprintf(buf, sizeof(buf), "%020d", i);
			DB_BENCH_ASSERT(
			    dbp->put(dbp, NULL, &key, &data, 0) == 0);
		}
	} else {
		data.data = buf;
		data.size = 20;
		if (type == DB_BTREE || type == DB_HASH) {
			key.size = 10;
			key.data = buf;
			for (i = 0; i < count; ++i) {
				(void)snprintf(buf, sizeof(buf), "%010d", i);
				DB_BENCH_ASSERT(
				    dbp->put(dbp, NULL, &key, &data, 0) == 0);
			}
#if DB_VERSION_MAJOR > 5 || (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
		} else if (type == DB_HEAP) {
			key.data = &rid;
			key.size = sizeof(rid);
			for (i = 0; i < count; ++i)
				DB_BENCH_ASSERT(dbp->put(dbp, 
				    NULL, &key, &data, DB_APPEND) == 0);
#endif
		} else {
			key.data = &recno;
			key.size = sizeof(recno);
			for (i = 0, recno = 1; i < count; ++i, ++recno)
				DB_BENCH_ASSERT(
				    dbp->put(dbp, NULL, &key, &data, 0) == 0);
		}
	}

	TIMER_STOP;

	printf("# %d %s database in-order put of 10/20 byte key/data %sitems\n",
	    count, ts, duplicate ? "duplicate " : "");
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	return (0);
}

static int
b_load_usage()
{
	(void)fprintf(stderr,
	    "usage: b_load [-d] [-C cachesz] [-c count] [-t type]\n");
	return (EXIT_FAILURE);
}
