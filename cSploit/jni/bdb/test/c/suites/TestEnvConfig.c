/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

struct context {
	FILE *fp;
	char *path;
};

#define	ENV {								\
	if (dbenv != NULL)						\
		CuAssertTrue(ct, dbenv->close(dbenv, 0) == 0);		\
	CuAssertTrue(ct, db_env_create(&dbenv, 0) == 0);		\
	dbenv->set_errfile(dbenv, stderr);				\
}

int TestEnvConfigSuiteSetup(CuSuite *ct) {
	return (0);
}

int TestEnvConfigSuiteTeardown(CuSuite *ct) {
	return (0);
}

int TestEnvConfigTestSetup(CuTest *ct) {
	struct context *info;

	if ((info = calloc(1, sizeof(*info))) == NULL)
		return (ENOMEM);
	ct->context = info;
	setup_envdir(TEST_ENV, 1);
	return (0);
}

int TestEnvConfigTestTeardown(CuTest *ct) {
	struct context *info;
	FILE *fp;
	char *path;

	info = ct->context;
	assert(info != NULL);
	fp = info->fp;
	path = info->path;
	if (fp != NULL)
		fclose(fp);
	if (path != NULL)
		free(path);
	free(info);
	ct->context = NULL;
	teardown_envdir(TEST_ENV);
	return (0);
}

int TestSetTxMax(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* tx_max: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_tx_max(dbenv, 37) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_TXN, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_tx_max(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37);
	ENV
	CuAssertTrue(ct, dbenv->set_tx_max(dbenv, 63) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_tx_max(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37);
	return (0);
}

int TestSetLogMax(CuTest *ct) {
	DB_ENV *dbenv;
	struct context *info;
	FILE *msgfile;
	char *path;
	u_int32_t v;

	dbenv = NULL;
	if ((path = calloc(100, sizeof(char))) == NULL)
		return (ENOMEM);
	snprintf(path, 100, "%s%c%s", TEST_ENV, PATH_SEPARATOR[0], "msgfile");
	if ((msgfile = fopen(path, "w")) == NULL)
		return (EINVAL);
	info = ct->context;
	info->fp = msgfile;
	info->path = path;
	/* lg_max: reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lg_max(dbenv, 37 * 1024 * 1024) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOG, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_max(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37 * 1024 * 1024);
	ENV
	/* New log maximum size is ignored when joining the environment. */
	CuAssertTrue(ct, dbenv->set_lg_max(dbenv, 63 * 1024 * 1024) == 0);
	/* Redirect the error message to suppress the warning. */
	dbenv->set_msgfile(dbenv, msgfile);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_max(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37 * 1024 * 1024);
	/* Direct the error message back to the standard output. */
	dbenv->set_msgfile(dbenv, NULL);
	/* Re-config the log maximum size after opening the environment. */
	CuAssertTrue(ct, dbenv->set_lg_max(dbenv, 63 * 1024 * 1024) == 0);
	CuAssertTrue(ct, dbenv->get_lg_max(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 63 * 1024 * 1024);
	return (0);
}

int TestSetLogBufferSize(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lg_bsize: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lg_bsize(dbenv, 37 * 1024) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOG, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_bsize(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37 * 1024);
	ENV
	CuAssertTrue(ct, dbenv->set_lg_bsize(dbenv, 63 * 1024) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_bsize(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37 * 1024);
	return (0);
}

int TestSetLogRegionSize(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lg_regionmax: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lg_regionmax(dbenv, 137 * 1024) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOG, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_regionmax(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 137 * 1024);
	ENV
	CuAssertTrue(ct, dbenv->set_lg_regionmax(dbenv, 163 * 1024) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lg_regionmax(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 137 * 1024);
	return (0);
}

int TestGetLockConflicts(CuTest *ct) {
	DB_ENV *dbenv;
	const u_int8_t *lk_conflicts;
	int lk_modes, nmodes;
	u_int8_t conflicts[40];

	dbenv = NULL;
	/* lk_get_lk_conflicts: NOT reset at run-time. */
	ENV
	memset(conflicts, 'a', sizeof(conflicts));
	nmodes = 6;
	CuAssertTrue(ct,
	    dbenv->set_lk_conflicts(dbenv, conflicts, nmodes) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_lk_conflicts(dbenv, &lk_conflicts, &lk_modes) == 0);
	CuAssertTrue(ct, lk_conflicts[0] == 'a');
	CuAssertTrue(ct, lk_modes == 6);
	ENV
	memset(conflicts, 'b', sizeof(conflicts));
	nmodes = 8;
	CuAssertTrue(ct,
	    dbenv->set_lk_conflicts(dbenv, conflicts, nmodes) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_lk_conflicts(dbenv, &lk_conflicts, &lk_modes) == 0);
	CuAssertTrue(ct, lk_conflicts[0] == 'a');
	CuAssertTrue(ct, lk_modes == 6);

	return (0);
}

int TestSetLockDetect(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lk_detect: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lk_detect(dbenv, DB_LOCK_MAXLOCKS) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_detect(dbenv, &v) == 0);
	CuAssertTrue(ct, v == DB_LOCK_MAXLOCKS);
	ENV
	CuAssertTrue(ct, dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_detect(dbenv, &v) == 0);
	CuAssertTrue(ct, v == DB_LOCK_MAXLOCKS);
	return (0);
}

int TestLockMaxLocks(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lk_max_locks: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_locks(dbenv, 1037) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_locks(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 1037);
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_locks(dbenv, 1063) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_locks(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 1037);
	return (0);
}

int TestLockMaxLockers(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lk_max_lockers: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_lockers(dbenv, 37) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_lockers(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37);
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_lockers(dbenv, 63) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_lockers(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 37);
	return (0);
}

int TestSetLockMaxObjects(CuTest *ct) {
	DB_ENV *dbenv;
	u_int32_t v;

	dbenv = NULL;
	/* lk_max_objects: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_objects(dbenv, 1037) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_objects(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 1037);
	ENV
	CuAssertTrue(ct, dbenv->set_lk_max_objects(dbenv, 1063) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_lk_max_objects(dbenv, &v) == 0);
	CuAssertTrue(ct, v == 1037);
	return (0);
}

int TestSetLockTimeout(CuTest *ct) {
	DB_ENV *dbenv;
	struct context *info;
	FILE *msgfile;
	char *path;
	db_timeout_t timeout;

	dbenv = NULL;
	if ((path = calloc(100, sizeof(char))) == NULL)
		return (ENOMEM);
	snprintf(path, 100, "%s%c%s", TEST_ENV, PATH_SEPARATOR[0], "msgfile");
	if ((msgfile = fopen(path, "w")) == NULL)
		return (EINVAL);
	info = ct->context;
	info->fp = msgfile;
	info->path = path;
	/* lock timeout: reset at run-time. */
	ENV
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 37, DB_SET_LOCK_TIMEOUT) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_LOCK_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 37);
	ENV
	/* New lock timeout is ignored when joining the environment. */
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 63, DB_SET_LOCK_TIMEOUT) == 0);
	/* Redirect the error message to suppress the warning. */
	dbenv->set_msgfile(dbenv, msgfile);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_LOCK_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 37);
	/* Direct the error message back to the standard output. */
	dbenv->set_msgfile(dbenv, NULL);
	/* Re-config the lock timeout after opening the environment. */
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 63, DB_SET_LOCK_TIMEOUT) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_LOCK_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 63);
	return (0);
}

int TestSetTransactionTimeout(CuTest *ct) {
	DB_ENV *dbenv;
	struct context *info;
	FILE *msgfile;
	char *path;
	db_timeout_t timeout;

	dbenv = NULL;
	if ((path = calloc(100, sizeof(char))) == NULL)
		return (ENOMEM);
	snprintf(path, 100, "%s%c%s", TEST_ENV, PATH_SEPARATOR[0], "msgfile");
	if ((msgfile = fopen(path, "w")) == NULL)
		return (EINVAL);
	info = ct->context;
	info->fp = msgfile;
	info->path = path;
	/* txn timeout: reset at run-time. */
	ENV
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 37, DB_SET_TXN_TIMEOUT) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_TXN_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 37);
	ENV
	/* New transaction timeout is ignored when joining the environment. */
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 63, DB_SET_TXN_TIMEOUT) == 0);
	/* Redirect the error message to suppress the warning. */
	dbenv->set_msgfile(dbenv, msgfile);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_TXN_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 37);
	/* Direct the error message back to the standard output. */
	dbenv->set_msgfile(dbenv, NULL);
	/* Re-config the transaction timeout after opening the environment. */
	CuAssertTrue(ct,
	    dbenv->set_timeout(dbenv, 63, DB_SET_TXN_TIMEOUT) == 0);
	CuAssertTrue(ct,
	    dbenv->get_timeout(dbenv, &timeout, DB_SET_TXN_TIMEOUT) == 0);
	CuAssertTrue(ct, timeout == 63);
	return (0);
}

int TestSetCachesize(CuTest *ct) {
	DB_ENV *dbenv;
	int ncache;
	u_int32_t a, b;

	dbenv = NULL;
	/* cache size: NOT reset at run-time. */
	ENV
	CuAssertTrue(ct, dbenv->set_cachesize(dbenv, 1, 131072, 3) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_MPOOL, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_cachesize(dbenv, &a, &b, &ncache) == 0);
	CuAssertTrue(ct, dbenv->get_cachesize(dbenv, &a, &b, &ncache) == 0);
	CuAssertTrue(ct, a == 1 && b == 131072 && ncache == 3);
	ENV
	CuAssertTrue(ct, dbenv->set_cachesize(dbenv, 2, 262144, 1) == 0);
	CuAssertTrue(ct, dbenv->open(dbenv, TEST_ENV, DB_JOINENV, 0666) == 0);
	CuAssertTrue(ct, dbenv->get_cachesize(dbenv, &a, &b, &ncache) == 0);
	CuAssertTrue(ct, a == 1 && b == 131072 && ncache == 3);
	return (0);
}
