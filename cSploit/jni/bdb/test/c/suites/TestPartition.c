/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * A C Unit test for DB->set_partition API. [#21373]
 *
 * Test cases:
 * One of the key DBTs has no data;
 * Two of the key DBTs have no data;
 * Two key DBTs have the same non-NULL data;
 * The key DBTs are not sorted;
 * The partition number is not equal to key array size plus 1;
 * Both partition key and callback are set;
 * Neither partition key nor callback are set.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

static int close_db(DB_ENV *, DB *, CuTest *);
static int create_db(DB_ENV **, DB **dbpp, int bigcache, CuTest *);
static u_int32_t partitionCallback(DB *, DBT *);
static int put_data(DB *);

static FILE *errfp;
static char *content;
static u_int32_t nparts;

int TestPartitionSuiteSetup(CuSuite *suite) {
	errfp = NULL;
	content = "abcdefghijklmnopqrstuvwxyz";
	nparts = 5;
	return (0);
}

int TestPartitionSuiteTeardown(CuSuite *suite) {
	return (0);
}

int TestPartitionTestSetup(CuTest *ct) {
	if (errfp != NULL)
		fclose(errfp);
	setup_envdir(TEST_ENV, 1);
	errfp = fopen("TESTDIR/errfile", "w");
	return (0);
}

int TestPartitionTestTeardown(CuTest *ct) {
	if (errfp != NULL) {
		fclose(errfp);
		errfp = NULL;
	}
	return (0);
}

int TestPartOneKeyNoData(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	/* Allocate the memory from stack. */
	DBT keys[4];
	u_int32_t i;

	dbenv = NULL;
	dbp = NULL;
	nparts = 5;

	/* Do not assign any data to the first DBT. */
	memset(&keys[0], 0, sizeof(DBT));
	for (i = 1 ; i < (nparts - 1); i++) {
		memset(&keys[i], 0, sizeof(DBT));
		keys[i].data = &content[(i + 1) * (strlen(content) / nparts)];
		keys[i].size = sizeof(char);
	}

	/* Do not set any database flags. */
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	/*
	 * Verify that before the database is opened, DB->set_partition can
	 * be called multiple times regardless of its return code.
	 */
	keys[0].flags = DB_DBT_MALLOC;
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) != 0);
	keys[0].flags = 0;
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUPSORT flags. */
	setup_envdir(TEST_ENV, 1);
	errfp = fopen("TESTDIR/errfile", "w");
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUPSORT) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUP flags. */
	setup_envdir(TEST_ENV, 1);
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUP) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	return (0);
}

int TestPartTwoKeyNoData(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT *keys;
	u_int32_t i;

	dbenv = NULL;
	dbp = NULL;
	keys = NULL;
	nparts = 5;

	CuAssertTrue(ct, (keys = malloc((nparts - 1) * sizeof(DBT))) != NULL);
	memset(keys, 0, (nparts - 1) * sizeof(DBT));
	/* Do not assign any data to the first 2 DBTs. */
	keys[0].size = keys[1].size = 0;
	for (i = 2 ; i < (nparts - 1); i++) {
		keys[i].data = &content[(i + 1) * (strlen(content) / nparts)];
		keys[i].size = sizeof(char);
	}

	/* Do not set any database flags. */
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUPSORT flags. */
	setup_envdir(TEST_ENV, 1);
	errfp = fopen("TESTDIR/errfile", "w");
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUPSORT) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUP flags. */
	setup_envdir(TEST_ENV, 1);
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUP) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	free(keys);
	return (0);
}

int TestPartDuplicatedKey(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT *keys;
	u_int32_t i;

	dbenv = NULL;
	dbp = NULL;
	keys = NULL;
	nparts = 5;

	CuAssertTrue(ct, (keys = malloc((nparts - 1) * sizeof(DBT))) != NULL);
	memset(keys, 0, (nparts - 1) * sizeof(DBT));
	/* Assign the same data to the first 2 DBTs. */
	for (i = 0 ; i < (nparts - 1); i++) {
		if (i < 2)
			keys[i].data = &content[strlen(content) / nparts];
		else
			keys[i].data = &content[(i + 1) *
			    (strlen(content) / nparts)];
		keys[i].size = sizeof(char);
	}

	/* Do not set any database flags. */
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUPSORT flags. */
	setup_envdir(TEST_ENV, 1);
	errfp = fopen("TESTDIR/errfile", "w");
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUPSORT) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set DB_DUP flags. */
	setup_envdir(TEST_ENV, 1);
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->set_flags(dbp, DB_DUP) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	free(keys);
	return (0);
}

int TestPartUnsortedKey(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT *keys;
	u_int32_t i, indx;

	dbenv = NULL;
	dbp = NULL;
	keys = NULL;
	nparts = 6;

	CuAssertTrue(ct, (keys = malloc((nparts - 1) * sizeof(DBT))) != NULL);
	memset(keys, 0, (nparts - 1) * sizeof(DBT));
	/* Assign unsorted keys to the array. */
	for (i = 0, indx = 0; i < (nparts - 1); i++) {
		if (i == (nparts - 2) && i % 2 == 0)
			indx = i;
		else if (i % 2 != 0)
			indx = i - 1;
		else
			indx = i + 1;
		keys[i].data =
		    &content[(indx + 1) * (strlen(content) / nparts)];
		keys[i].size = sizeof(char);
	}

	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct,
	    dbp->set_partition(dbp, nparts - 1, &keys[1], NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, dbp->close(dbp, 0) == 0);

	/*
	 * Reconfig with a different partition number and
	 * re-open the database.
	 */
	CuAssertTrue(ct, db_create(&dbp, dbenv, 0) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, 0, 0644) != 0);
	CuAssertTrue(ct, dbp->close(dbp, 0) == 0);

	/*
	 * Reconfig with a different set of partition keys and
	 * re-open the database.
	 */
	CuAssertTrue(ct, db_create(&dbp, dbenv, 0) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts - 1, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, 0, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	free(keys);
	return (0);
}

int TestPartNumber(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT *keys;
	u_int32_t i;

	dbenv = NULL;
	dbp = NULL;
	keys = NULL;
	nparts = 1000000;

	CuAssertTrue(ct, (keys = malloc((nparts - 1) * sizeof(DBT))) != NULL);
	memset(keys, 0, (nparts - 1) * sizeof(DBT));
	/* Assign data to the keys. */
	for (i = 0 ; i < (nparts - 1); i++) {
		CuAssertTrue(ct,
		    (keys[i].data = malloc(sizeof(u_int32_t))) != NULL);
		memcpy(keys[i].data, &i, sizeof(u_int32_t));
		keys[i].size = sizeof(u_int32_t);
	}

	/* Partition number is less than 2. */
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 1, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, 1, keys, NULL) != 0);

	/* Partition number is bigger than the limit 1000000. */
	CuAssertTrue(ct,
	    dbp->set_partition(dbp, nparts + 1, keys, NULL) == EINVAL);

	/* Partition number is equal to the limit 1000000. */
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);

	/* Partition keys do not fix into a single database page. */
	CuAssertTrue(ct, dbp->set_pagesize(dbp, 512) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, 800, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);

	for (i = 0 ; i < (nparts - 1); i++)
		free(keys[i].data);
	free(keys);
	return (0);
}

int TestPartKeyCallBothSet(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT *keys;
	u_int32_t i;

	dbenv = NULL;
	dbp = NULL;
	keys = NULL;
	nparts = 5;

	CuAssertTrue(ct, (keys = malloc((nparts - 1) * sizeof(DBT))) != NULL);
	memset(keys, 0, (nparts - 1) * sizeof(DBT));
	/* Do not assign any data to the first DBT. */
	for (i = 0 ; i < (nparts - 1); i++) {
		keys[i].data = &content[(i + 1) * (strlen(content) / nparts)];
		keys[i].size = sizeof(char);
	}

	/* Set both partition key and callback, expect it fails. */
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct,
	    dbp->set_partition(dbp, nparts, keys, partitionCallback) != 0);

	/* Set partition by key and open the database. */
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, dbp->close(dbp, 0) == 0);

	/* Reconfig the partition with callback, expect it fails. */
	CuAssertTrue(ct, db_create(&dbp, dbenv, 0) == 0);
	CuAssertTrue(ct,
	    dbp->set_partition(dbp, nparts, NULL, partitionCallback) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, 0, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	fclose(errfp);
	errfp = NULL;

	/* Set partition by callback and open the database. */
	setup_envdir(TEST_ENV, 1);
	errfp = fopen("TESTDIR/errfile", "w");
	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct,
	    dbp->set_partition(dbp, nparts, NULL, partitionCallback) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssertTrue(ct, put_data(dbp) == 0);
	CuAssertTrue(ct, dbp->close(dbp, 0) == 0);

	/* Reconfig the partition with key, expect it fails. */
	CuAssertTrue(ct, db_create(&dbp, dbenv, 0) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, keys, NULL) == 0);
	CuAssertTrue(ct, dbp->open(dbp, NULL,
	    "test.db", NULL, DB_BTREE, 0, 0644) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);
	free(keys);
	return (0);
}

int TestPartKeyCallNeitherSet(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;

	dbenv = NULL;
	dbp = NULL;

	CuAssertTrue(ct, create_db(&dbenv, &dbp, 0, ct) == 0);
	CuAssertTrue(ct, dbp->set_partition(dbp, nparts, NULL, NULL) != 0);
	CuAssertTrue(ct, close_db(dbenv, dbp, ct) == 0);

	return (0);
}

static int
create_db(dbenvp, dbpp, bigcache, ct)
	DB_ENV **dbenvp;
	DB **dbpp;
	int bigcache;
	CuTest *ct;
{
	DB_ENV *dbenv;
	DB *dbp;

	dbenv = NULL;
	dbp = NULL;

	CuAssertTrue(ct, db_env_create(&dbenv, 0) == 0);
	*dbenvp = dbenv;
	/* Big cache size is needed in some test case. */
	if (bigcache != 0 )
		CuAssertTrue(ct, dbenv->set_cachesize(dbenv,
		0, 128 * 1048576, 1) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV,
	    DB_CREATE | DB_INIT_LOCK |DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN, 0666) == 0);

	CuAssertTrue(ct, db_create(&dbp, dbenv, 0) == 0);
	*dbpp = dbp;
	dbp->set_errfile(dbp, errfp != NULL ? errfp : stdout);
	dbp->set_errpfx(dbp, "TestPartition");

	return (0);
}

static int
close_db(dbenv, dbp, ct)
	DB_ENV *dbenv;
	DB *dbp;
	CuTest *ct;
{
	if (dbp != NULL)
		CuAssertTrue(ct, dbp->close(dbp, 0) == 0);
	if (dbenv != NULL)
		CuAssertTrue(ct, dbenv->close(dbenv, 0) == 0);
	return (0);
}

static u_int32_t
partitionCallback(dbp, key)
	DB *dbp;
	DBT *key;
{
	char *a, *b;
	u_int32_t i, len;

	a = (char *)key->data;
	b = NULL;
	len = nparts % strlen(content);

	for (i = 0; i < len; i++) {
		b = &content[(i + 1) * (strlen(content) / len)];
		if ((*a - *b) < 0)
			break;
	}

	return (i);
}

static int
put_data(dbp)
	DB *dbp;
{
	DBT key, data;
	u_int32_t i;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	for (i = 0; i < strlen(content); i++) {
		key.data = &content[i];
		key.size = sizeof(char);

		data.data = &content[i];
		data.size = sizeof(char);

		if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0) {
			dbp->err(dbp, ret, "DB->put");
			return (ret);
		}
	}
	return (ret);
}

