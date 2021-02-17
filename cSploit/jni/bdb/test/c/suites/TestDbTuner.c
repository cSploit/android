/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/* 
 * A c unit test program for db_tuner [#18910].
 * 
 * There are total 8 * 3 * 4 * 2 = 192 test cases,
 * which are from:
 * 8 possible pagesize (8),
 * 3 types of dup (default, DB_DUP, DB_DUPSORT) (3),
 * whether key/data item is small or big (4),
 * whether the number of data per key is larger than 1 or not (2).
 * 
 * This test program test db_tuner on all above possible btree databases.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "CuTest.h"

#define random(x) (rand() % x)

int open_db(DB_ENV **, DB **, char *, char *, u_int32_t, int);
int run_test(CuTest *, u_int32_t, int, int, int, int, int);
int store_db(DB *, int, int, int, int);

const char *progname_dbtuner = "TestDbTuner";
int total_cases, success_cases;

int TestDbTuner(CuTest *ct) {
	u_int32_t pgsize;
	int i, j;

	total_cases = success_cases = 0;

	printf("Test db_tuner on various different kinds of btree, %s\n",
	    "so it takes some time.");
	/*
	 * Total 8 * 3 * 4 * 2 = 192 test cases, currently not test on
	 * big key, because producing btree databases with too big key will
	 * cost too much time. So here are 96 test cases.
	 * 
	 * If you want test btree databases with big keys, please add follows:
	 *     run_test(ct, pgsize, 50, 16367, 1, 1000, j);
	 *     run_test(ct, pgsize, 50, 16367, 100, 1000, j);
	 *     run_test(ct, pgsize, 16367, 16367, 1, 1000, j);
	 *     run_test(ct, pgsize, 16367, 16367, 100, 1000, j);
	 */
	for (i = 0; i < 8; i++) {
		pgsize = (1 << i) * 512;
		for (j = 0; j < 3; j++) {
			run_test(ct, pgsize, 50, 0, 1, 1000, j);
			run_test(ct, pgsize, 50, 0, 100, 1000, j);
			run_test(ct, pgsize, 16367, 0, 1, 1000, j);
			run_test(ct, pgsize, 16367, 0, 100, 1000, j);
		}
	}

	printf("\n\nTESTING db_tuner on %d btree databases:\n", total_cases);
	printf(".............................................................."
	    "\t \t%0.2f%% passed (%d/%d).\n",
	    (((double)success_cases/total_cases) * 100),
	    success_cases, total_cases);

	return (EXIT_SUCCESS);
}

/* 
 * run_test:
 *	CuTest *ct:
 *	u_int32_t pgsize: 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
 *	int nrecs: number of unique key, {1000}
 *	int mkeys: max-length of key, {0: small; 16367: large for big key}
 *	int mdatas: max-length of data, {50: small; 16367: large for big data}
 *	int mdups: max-number of data per key, {1, 200}
 *	int duptype: {default, DB_DUP, DB_DUPSORT}
 */
int
run_test(ct, pgsize, mdatas, mkeys, mdups, nrecs, duptype)
	CuTest *ct;
	u_int32_t pgsize;
	int mdatas, mkeys, mdups, nrecs, duptype;
{
	DB *dbp;
	DB_ENV *dbenv;
	char cmd[1000];
	char db_file_name[100], *home;

	home = "TESTDIR";
	total_cases++;

	/* Step 1: Prepare the database based on the parameters. */
	sprintf(db_file_name, "p%d_n%d_k%d_d%d_s%d_D%d.db",
	    pgsize, nrecs, mkeys, mdatas, mdups, duptype);

	TestEnvConfigTestSetup(ct);

	CuAssert(ct, "open_db",
	    open_db(&dbenv, &dbp, db_file_name, home, pgsize, duptype) == 0);

	CuAssert(ct, "store_db",
	    store_db(dbp, nrecs, mkeys, mdatas, mdups) == 0);

	if (dbp != NULL)
		CuAssert(ct, "DB->close", dbp->close(dbp, 0) == 0);

	/* Step 2: Test db_tuner utility. */
#ifdef _WIN32
	sprintf(cmd, "Win32\\Debug\\db_tuner.exe -h %s -d %s -v >/null 2>&1",
	    home, db_file_name);
#else
	sprintf(cmd, "./db_tuner -d %s//%s -v >/dev/null 2>&1",
	    home, db_file_name);
#endif
	system(cmd);

	if (dbenv != NULL)
		CuAssert(ct, "DB_ENV->close failed",
		    dbenv->close(dbenv, 0) == 0);

	TestEnvConfigTestTeardown(ct);
	success_cases++;

	return (0);
}

/* Produce the btree database for the specific test case. */
int
store_db(dbp, nrecs, mkeys, mdatas, mdups)
	DB *dbp;
	int nrecs, mkeys, mdatas, mdups;
{
	DBT key, data;
	db_recno_t recno;
	int i, ret;
	int data_sz, key_sz, dup_rec;
	char str[37] = "abcdefghijklmnopqrstuvwxyz0123456789";
	char *buf;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	buf = (char *)malloc(sizeof(char) * mdatas);
	memset(buf, 0, sizeof(char) * mdatas);
	for (recno = 1; recno <= (db_recno_t)nrecs; recno++) {
		data_sz = random(mdatas);

		for (i = 0; i < data_sz; ++i)
			buf[i] = str[i % 37];
		buf[data_sz] = '\0';

		key_sz = mkeys < sizeof(recno) ? sizeof(recno) : random(mkeys);

		key.data = &recno;
		key.size = key_sz < sizeof(recno) ? sizeof(recno) : key_sz;

		dup_rec = random(mdups);
		for (i = 0; i <= dup_rec; ++i) {
			data.data = buf;
			data.size = (u_int32_t)strlen(buf) + i + 1;

			if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0) {
				dbp->err(dbp, ret, "DB->put"); 
				break;
			}
		}
	}

	return (ret);
}

int
open_db(dbenvp, dbpp, dbname, home, pgsize, duptype)
	DB_ENV **dbenvp;
	DB **dbpp;
	char *dbname, *home;
	u_int32_t pgsize;
	int duptype;
{
	DB *dbp;
	DB_ENV *dbenv;
	int ret;

	dbp = NULL;
	dbenv = NULL;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "db_env_create: %s\n", db_strerror(ret));
		return (EXIT_FAILURE);
	}

	*dbenvp = dbenv;

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname_dbtuner);

	if ((ret =
	    dbenv->set_cachesize(dbenv, (u_int32_t)0,
	     (u_int32_t)262144000 * 2, 1)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_cachesize");
		return (EXIT_FAILURE);
	}

	if ((ret = dbenv->mutex_set_max(dbenv, 10000)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->mutex_set_max");
		return (EXIT_FAILURE);
	}

	if ((ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_PRIVATE | DB_INIT_MPOOL, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		return (EXIT_FAILURE);
	}

	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		return (EXIT_FAILURE);
	}

	*dbpp = dbp;

	if ((ret = dbp->set_pagesize(dbp, pgsize)) != 0) {
		dbenv->err(dbenv, ret, "DB->set_pagesize");
		return (EXIT_FAILURE);
	}

	if (duptype && (ret =
	    dbp->set_flags(dbp, duptype == 1 ? DB_DUP : DB_DUPSORT)) != 0) {
		dbenv->err(dbenv, ret, "DB->set_flags");
		return (EXIT_FAILURE);
	}

	if ((ret =
	    dbp->open(dbp, NULL, dbname, NULL, DB_BTREE, DB_CREATE, 0)) != 0) {
		dbenv->err(dbenv, ret, "%s: DB->open", dbname);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
