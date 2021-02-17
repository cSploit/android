/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * Test a situation ([#19345]) where a deadlock is returned via
 * a secondary index when doing and update.
 *
 * TestKeyExistErrorReturn.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"

#ifdef _WIN32
#include <windows.h>
#define	PATHD '\\'
extern int getopt(int, char * const *, const char *);
extern char *optarg;

/* Wrap Windows thread API to make it look POSIXey. */
typedef HANDLE thread_t;
#define	thread_create(thrp, attr, func, arg)				\
    (((*(thrp) = CreateThread(NULL, 0,					\
    (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define	thread_join(thr, statusp)					\
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&		\
    ((statusp == NULL) ? 0 :						\
    (GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)))

typedef HANDLE mutex_t;
#define	mutex_init(m, attr)						\
    (((*(m) = CreateMutex(NULL, FALSE, NULL)) != NULL) ? 0 : -1)
#define	mutex_lock(m)							\
    ((WaitForSingleObject(*(m), INFINITE) == WAIT_OBJECT_0) ? 0 : -1)
#define	mutex_unlock(m)		(ReleaseMutex(*(m)) ? 0 : -1)
#else
#include <pthread.h>
#include <unistd.h>
#define	PATHD '/'

typedef pthread_t thread_t;
#define	thread_create(thrp, attr, func, arg)				\
    pthread_create((thrp), (attr), (func), (arg))
#define	thread_join(thr, statusp) pthread_join((thr), (statusp))

typedef pthread_mutex_t mutex_t;
#define	mutex_init(m, attr)	pthread_mutex_init((m), (attr))
#define	mutex_lock(m)		pthread_mutex_lock(m)
#define	mutex_unlock(m)		pthread_mutex_unlock(m)
#endif

#define	NUMWRITERS 5

/* Forward declarations */
int count_records __P((DB *, DB_TXN *));
int assoc_callback __P((DB *, const DBT *, const DBT *, DBT *));
void *writer_thread __P((void *));

int global_thread_num;
mutex_t thread_num_lock;

int TestKeyExistErrorReturn(CuTest *ct) {
	DB *pdbp;
	DB *sdbp;
	DB_ENV *dbenv;

	const char *sec_db_file = "secondary.db";
	const char *pri_db_file = "primary.db";
	const char *env_dir = "TESTDIR";
	int i;
	thread_t writer_threads[NUMWRITERS];
	u_int32_t db_flags, env_flags;

	pdbp = sdbp = NULL;
	dbenv = NULL;
	db_flags = DB_CREATE | DB_AUTO_COMMIT | DB_READ_UNCOMMITTED;
	env_flags = DB_CREATE | DB_RECOVER | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN | DB_THREAD;

	TestEnvConfigTestSetup(ct);

	CuAssert(ct, "db_env_create", db_env_create(&dbenv, 0) == 0);

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, "TestKeyExistErrorReturn");

	/* Run deadlock detector on every lock conflict. */
	CuAssert(ct, "dbenv->set_lk_detect",
	    dbenv->set_lk_detect(dbenv, DB_LOCK_MINWRITE) == 0);

	CuAssert(ct, "dbenv->open",
	    dbenv->open(dbenv, env_dir, env_flags, 0) == 0);

	CuAssert(ct, "db_create", db_create(&pdbp, dbenv, 0) == 0);
	CuAssert(ct, "pdbp->open", pdbp->open(pdbp, NULL,
	    pri_db_file, NULL, DB_BTREE, db_flags, 0) == 0);

	CuAssert(ct, "db_create", db_create(&sdbp, dbenv, 0) == 0);
	CuAssert(ct, "sdbp->set_flags", sdbp->set_flags(sdbp,
	    DB_DUPSORT) == 0);
	CuAssert(ct, "sdbp->open", sdbp->open(sdbp, NULL, sec_db_file,
	    NULL, DB_BTREE, db_flags, 0) == 0);

	CuAssert(ct, "DB->associate", pdbp->associate(pdbp, NULL, sdbp,
	    assoc_callback, DB_AUTO_COMMIT) == 0);

	/* Initialize a mutex. Used to help provide thread ids. */
	(void)mutex_init(&thread_num_lock, NULL);

	for (i = 0; i < NUMWRITERS; ++i)
		(void)thread_create(&writer_threads[i], NULL,
		    writer_thread, (void *)pdbp);

	for (i = 0; i < NUMWRITERS; ++i)
		(void)thread_join(writer_threads[i], NULL);

	if (sdbp != NULL) 
		CuAssert(ct, "sdbp->close", sdbp->close(sdbp, 0) == 0);
	if (pdbp != NULL)
		CuAssert(ct, "pdbp->close", pdbp->close(pdbp, 0) == 0);
	if (dbenv != NULL)
		CuAssert(ct, "dbenv->close", dbenv->close(dbenv, 0) == 0);

	TestEnvConfigTestTeardown(ct);

	return (EXIT_SUCCESS);
}

void *
writer_thread(void *args)
{
	DB *dbp;
	DB_ENV *dbenv;
	DBT key, data;
	DB_TXN *txn;

	char *key_strings[] = {"001", "002", "003", "004", "005",
	    "006", "007", "008", "009", "010"};
	int i, j, payload, ret, thread_num;
	int retry_count, max_retries = 20;

	dbp = (DB *)args;
	dbenv = dbp->dbenv;

	/* Get the thread number */
	(void)mutex_lock(&thread_num_lock);
	global_thread_num++;
	thread_num = global_thread_num;
	(void)mutex_unlock(&thread_num_lock);

	/* Initialize the random number generator */
	srand(thread_num);

	/* Write 50 times and then quit */
	for (i = 0; i < 50; i++) {
		retry_count = 0; /* Used for deadlock retries */
retry:
		ret = dbenv->txn_begin(dbenv, NULL, &txn, 0);
		if (ret != 0) {
			dbenv->err(dbenv, ret, "txn_begin failed");
			return ((void *)EXIT_FAILURE);
		}

		memset(&key, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
		for (j = 0; j < 10; j++) {
			/* Set up our key and data DBTs. */
			data.data = key_strings[j];
			data.size = (u_int32_t)strlen(key_strings[j]) + 1;

			payload = rand() + i;
			key.data = &payload;
			key.size = sizeof(int);

			switch (ret = dbp->put(dbp, txn, &key, &data,
			    DB_NOOVERWRITE)) {
			case 0:
				break;
			case DB_KEYEXIST:
				break;
			case DB_LOCK_DEADLOCK:
				(void)txn->abort(txn);
				if (retry_count < max_retries) {
					retry_count++;
					goto retry;
				}
				return ((void *)EXIT_FAILURE);
			default:
				dbenv->err(dbenv, ret, "db put failed");
				ret = txn->abort(txn);

				if (ret != 0)
					dbenv->err(dbenv, ret,
					    "txn abort failed");
				return ((void *)EXIT_FAILURE);
			}
		}

		if ((ret = txn->commit(txn, 0)) != 0) {
			dbenv->err(dbenv, ret, "txn commit failed");
			return ((void *)EXIT_FAILURE);
		}
	}
	return ((void *)EXIT_SUCCESS);
}

int
count_records(DB *dbp, DB_TXN *txn)
{
	DBT key, data;
	DBC *cursorp;
	int count, ret;

	cursorp = NULL;
	count = 0;

	/* Get the cursor */
	ret = dbp->cursor(dbp, txn, &cursorp, DB_READ_UNCOMMITTED);
	if (ret != 0) {
		dbp->err(dbp, ret, "count_records: cursor open failed.");
		goto cursor_err;
	}

	/* Get the key DBT used for the database read */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT)); 
	do {
		ret = cursorp->get(cursorp, &key, &data, DB_NEXT);
		switch (ret) {
		case 0:
			count++;
		break;
		case DB_NOTFOUND:
			break;
		default:
			dbp->err(dbp, ret, "Count records unspecified error");
			goto cursor_err;
		}
	} while (ret == 0);

cursor_err:
	if (cursorp != NULL) {
		ret = cursorp->close(cursorp);
		if (ret != 0) {
			dbp->err(dbp, ret,
			    "count_records: cursor close failed.");
		}
	}

	return (count);
}

int
assoc_callback(pdbp, pkey, pdata, skey)
	DB *pdbp;
	const DBT *pkey;
	const DBT *pdata;
	DBT *skey;
{
	memset(skey, 0, sizeof(DBT));
	skey->data = pdata->data;
	skey->size = pdata->size;

	return (EXIT_SUCCESS);
}
