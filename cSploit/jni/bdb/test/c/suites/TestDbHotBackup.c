/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * A C Unit test for db_hotbackup APIs [#20451]
 *
 * Test casess:
 * 1. btree database without any environment/database configured, backup only
 * the database file without callbacks;
 * 2. btree database without any environment/database configured, backup only
 * the database file with callbacks;
 * 3. btree database without any environment/database configured, backup only
 * the database file with backup configurations;
 * 4. btree partitioned database, backup the whole environment into a single
 * directory;
 * 5. btree database with multiple add_data_dir configured, backup the whole
 * environment including DB_CONFIG and maintain its directory structure in
 * the backup directory;
 * 6. btree database with set_lg_dir configured, backup the whole environment
 * with callbacks including DB_CONFIG and maintain its directory structure in
 * the backup directory;
 * 7. queue database having multiple queue extent files, backup only the
 * database file without callbacks;
 * 8. heap database, backup only the database file without callbacks.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

/* microseconds in a second */
#define	US_PER_SEC	1000000

struct handlers {
	DB_ENV *dbenvp;
	DB *dbp;
};

static int backup_close(DB_ENV *, const char *, void *);
static int backup_db(CuTest *, DB_ENV *, const char *, u_int32_t, int);
static int backup_env(CuTest *, DB_ENV *, u_int32_t, int);
static int backup_open(DB_ENV *, const char *, const char *, void **);
static int backup_write(DB_ENV *, u_int32_t,
    u_int32_t, u_int32_t, u_int8_t *, void *);
static int cleanup_test(DB_ENV *, DB *);
static int cmp_files(const char *, const char *);
static int make_dbconfig(const char *);
static int open_dbp(DB_ENV **, DB **, DBTYPE,
    u_int32_t, char **, const char *, const char *, u_int32_t, DBT *);
static int setup_dir(u_int32_t, char **);
static int store_records(DB *, u_int32_t);
static int test_backup_onlydbfile(CuTest *, DBTYPE, int);
static int verify_db_log(DBTYPE, u_int32_t,
    u_int32_t, const char *, const char *);
static int verify_dbconfig(u_int32_t);

#define BACKUP_DIR "BACKUP"
#define BACKUP_DB "backup.db"
#define LOG_DIR "LOG"

char *data_dirs[3] = {"DATA1", "DATA2", NULL};

int TestDbHotBackupSuiteSetup(CuSuite *suite) {
	return (0);
}

int TestDbHotBackupSuiteTeardown(CuSuite *suite) {
	return (0);
}

int TestDbHotBackupTestSetup(CuTest *ct) {
	struct handlers *info;

	if ((info = calloc(1, sizeof(*info))) == NULL)
		return (ENOMEM);
	ct->context = info;
	setup_envdir(TEST_ENV, 1);
	setup_envdir(BACKUP_DIR, 1);
	return (0);
}

int TestDbHotBackupTestTeardown(CuTest *ct) {
	struct handlers *info;
	DB_ENV *dbenv;
	DB *dbp;

	if (ct->context == NULL)
		return (EINVAL);
	info = ct->context;
	dbenv = info->dbenvp;
	dbp = info->dbp;
	/* Close all handles and clean the directories. */
	CuAssert(ct, "cleanup_test", cleanup_test(dbenv, dbp) == 0);
	free(info);
	ct->context = NULL;
	return (0);
}

int TestBackupSimpleEnvNoCallback(CuTest *ct) {
	CuAssertTrue(ct, test_backup_onlydbfile(ct, DB_BTREE, 0) == 0);

	return (0);
}

int TestBackupSimpleEnvWithCallback(CuTest *ct) {
	CuAssertTrue(ct, test_backup_onlydbfile(ct, DB_BTREE, 1) == 0);

	return (0);
}

int TestBackupSimpleEnvWithConfig(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBTYPE dtype;
	struct handlers *info;
	char **names;
	int cnt, has_callback;
	time_t end_time, secs1, secs2, start_time;
	u_int32_t flag, value;

	dtype = DB_BTREE;
	info = ct->context;
	has_callback = 0;
	flag = DB_EXCL;
	end_time = secs1 = secs2 = start_time = 0;

	/* Step 1: set up directories. */
	CuAssert(ct, "setup_dir", setup_dir(0, NULL) == 0);

	/* Step 2: open db handle. */
	CuAssert(ct, "open_dbp", open_dbp(&dbenv,
	    &dbp, dtype, 0, NULL, NULL, NULL, 0, NULL) == 0);
	info->dbenvp = dbenv;
	info->dbp = dbp;

	/*
	 * Step 3: store records into db so that there is more than
	 * 1 data page in the db.
	 */
	CuAssert(ct, "store_records", store_records(dbp, 10) == 0);
	CuAssert(ct, "DB->sync", dbp->sync(dbp, 0) == 0);

	/*
	 * Step 4: verify the backup handle is NULL,
	 * since we never configure the backup.
	 */
	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv,
	    DB_BACKUP_WRITE_DIRECT, &value) == EINVAL);

	/*
	 * Step 5: backup without any backup configs.
	 * 5a: backup only the db file without callbacks and record the time.
	 */
	start_time = time(NULL);
	CuAssert(ct, "backup_db",
	    backup_db(ct, dbenv, BACKUP_DB, flag, has_callback) == 0);
	end_time = time(NULL);
	secs1 = end_time - start_time;

	/* 5b: verify db file is in BACKUP_DIR. */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 0, NULL, NULL) == 0);

	/* 5c: verify that no other files are in BACKUP_DIR. */
	CuAssert(ct, "__os_dirlist",
	    __os_dirlist(NULL, BACKUP_DIR, 0, &names, &cnt) == 0);
	CuAssert(ct, "too many files in backupdir", cnt == 1);

	/* Clean up the backup directory. */
	setup_envdir(BACKUP_DIR, 1);

	/*
	 * Step 6: backup with backup configs.
	 * 6a: configure the backup handle: use direct I/O to write pages to
	 * the disk, the backup buffer size is 256 bytes (which is smaller
	 * than the db page size), the number of pages
	 * to read before pausing is 1, and the number of seconds to sleep
	 * between batches of reads is 1.
	 */
	CuAssert(ct, "DB_ENV->set_backup_config",
	    dbenv->set_backup_config(dbenv, DB_BACKUP_WRITE_DIRECT, 1) == 0);
	CuAssert(ct, "DB_ENV->set_backup_config",
	    dbenv->set_backup_config(dbenv, DB_BACKUP_SIZE, 256) == 0);
	CuAssert(ct, "DB_ENV->set_backup_config",
	    dbenv->set_backup_config(dbenv, DB_BACKUP_READ_COUNT, 1) == 0);
	CuAssert(ct, "DB_ENV->set_backup_config",
	    dbenv->set_backup_config(dbenv,
	    DB_BACKUP_READ_SLEEP, US_PER_SEC / 2) == 0);

	/*
	 * 6b: backup only the db file without callbacks and
	 * record the time.
	 */
	start_time = time(NULL);
	CuAssert(ct, "backup_db",
	    backup_db(ct, dbenv, BACKUP_DB, flag, has_callback) == 0);
	end_time = time(NULL);
	secs2 = end_time - start_time;

	/* 6c: verify db file is in BACKUP_DIR. */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 0, NULL, NULL) == 0);

	/* 6d: no other files are in BACKUP_DIR. */
	CuAssert(ct, "__os_dirlist",
	    __os_dirlist(NULL, BACKUP_DIR, 0, &names, &cnt) == 0);
	CuAssert(ct, "too many files in backupdir", cnt == 1);

	/* 6e: verify the backup config. */
	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv,
	    DB_BACKUP_READ_SLEEP, &value) == 0);
	CuAssertTrue(ct, value == US_PER_SEC / 2);
	/*
	 * Verify the backup config DB_BACKUP_READ_SLEEP works. That is with
	 * the configuration, backup pauses for a number of microseconds
	 * between batches of reads. So for the same backup content, the backup
	 * time with the configuration should be longer than that without it.
	 */
	CuAssertTrue(ct, secs2 > secs1);

	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv,
	    DB_BACKUP_READ_COUNT, &value) == 0);
	CuAssertTrue(ct, value == 1);
	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv, DB_BACKUP_SIZE, &value) == 0);
	CuAssertTrue(ct, value == 256);
	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv,
	    DB_BACKUP_WRITE_DIRECT, &value) == 0);
	CuAssertTrue(ct, value == 1);

	/*
	 * Step 7: re-configure the backup write direct config and
	 * verify the new config value.
	 */
	CuAssert(ct, "DB_ENV->set_backup_config",
	    dbenv->set_backup_config(dbenv, DB_BACKUP_WRITE_DIRECT, 0) == 0);
	CuAssert(ct, "DB_ENV->get_backup_config",
	    dbenv->get_backup_config(dbenv,
	    DB_BACKUP_WRITE_DIRECT, &value) == 0);
	CuAssertTrue(ct, value == 0);

	return (0);
}

int TestBackupPartitionDB(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBT key1, key2, keys[2];
	DBTYPE dtype;
	struct handlers *info;
	int has_callback;
	u_int32_t flag, value1, value2;

	dtype = DB_BTREE;
	info = ct->context;
	has_callback = 0;
	flag = DB_BACKUP_CLEAN | DB_CREATE | DB_BACKUP_SINGLE_DIR;

	/* Step 1: set up directories and make DB_CONFIG. */
	CuAssert(ct, "setup_dir", setup_dir(1, data_dirs) == 0);
	CuAssert(ct, "make_dbconfig",
	    make_dbconfig("set_data_dir DATA1") == 0);

	/* Make the partition keys. */
	memset(&key1, 0, sizeof(DBT));
	memset(&key2, 0, sizeof(DBT));
	value1 = 8;
	key1.data = &value1;
	key1.size = sizeof(value1);
	value2 = 16;
	key2.data = &value2;
	key2.size = sizeof(value2);
	keys[0] = key1;
	keys[1] = key2;

	/* Step 2: open db handle. */
	CuAssert(ct,"open_dbp", open_dbp(&dbenv,
	    &dbp, dtype, 1, data_dirs, data_dirs[0], NULL, 3, keys) == 0);
	info->dbenvp = dbenv;
	info->dbp = dbp;

	/* Step 3: store records into db. */
	CuAssert(ct, "store_records", store_records(dbp, 1) == 0);
	CuAssert(ct, "DB->sync", dbp->sync(dbp, 0) == 0);

	/* Step 4: backup the whole environment into a single directory. */
	CuAssert(ct, "backup_env",
	    backup_env(ct, dbenv, flag, has_callback) == 0);

	/*
	 * Step 5: check backup result.
	 * 5a: verify db files are in BACKUP/DATA1.
	 */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 1, 0, data_dirs[0], NULL) == 0);

	/* 5b: verify that creation directory is not in BACKUPD_DIR. */
	CuAssert(ct, "__os_exist", __os_exists(NULL, "BACKUP/DATA", 0) != 0);

	/* 5c: verify log files are in BACKUP_DIR. */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 1, NULL, NULL) == 0);

	/* 5d: verify that DB_CONFIG is not in BACKUP_DIR. */
	CuAssert(ct, "verify_dbconfig", verify_dbconfig(0) == 0);

	return (0);
}

int TestBackupMultiDataDir(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBTYPE dtype;
	struct handlers *info;
	int has_callback;
	u_int32_t flag;

	dtype = DB_BTREE;
	info = ct->context;
	has_callback = 0;
	flag = DB_BACKUP_CLEAN | DB_CREATE | DB_BACKUP_FILES;

	/* Step 1: set up directories and make DB_CONFIG. */
	CuAssert(ct, "setup_dir", setup_dir(2, data_dirs) == 0);
	CuAssert(ct, "make_dbconfig",
	    make_dbconfig("set_data_dir DATA1") == 0);

	/* Step 2: open db handle. */
	CuAssert(ct,"open_dbp", open_dbp(&dbenv, &dbp,
	    dtype, 2, data_dirs, data_dirs[0], NULL, 0, NULL) == 0);
	info->dbenvp = dbenv;
	info->dbp = dbp;

	/* Step 3: store records into db. */
	CuAssert(ct, "store_records", store_records(dbp, 1) == 0);
	CuAssert(ct, "DB->sync", dbp->sync(dbp, 0) == 0);

	/* Step 4: backup the whole environment without callbacks. */
	CuAssert(ct, "backup_env",
	    backup_env(ct, dbenv, flag, has_callback) == 0);	

	/*
	 * Step 5: check backup result.
	 * 5a: verify db files are in BACKUP/DATA1.
	 */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 0, data_dirs[0], data_dirs[0]) == 0);

	/* 5b: verify that data_dirs are in backupdir. */
	CuAssert(ct, "__os_exist", __os_exists(NULL, "BACKUP/DATA1", 0) == 0);
	CuAssert(ct, "__os_exist", __os_exists(NULL, "BACKUP/DATA2", 0) == 0);

	/* 5c: verify that log files are in BACKUP_DIR. */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 1, NULL, NULL) == 0);

	/* 5d: verify that DB_CONFIG is in BACKUP_DIR. */
	CuAssert(ct, "verify_dbconfig", verify_dbconfig(1) == 0);

	return (0);
}

int TestBackupSetLogDir(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DBTYPE dtype;
	struct handlers *info;
	char *dirs[2];
	int has_callback = 1;
	u_int32_t flag;

	dtype = DB_BTREE;
	info = ct->context;
	has_callback = 1;
	flag = DB_BACKUP_CLEAN | DB_CREATE | DB_BACKUP_FILES;
	dirs[0] = LOG_DIR;
	dirs[1] = NULL;

	/* Step 1: set up directories and make DB_CONFIG. */
	CuAssert(ct, "setup_dir", setup_dir(1, dirs) == 0);
	CuAssert(ct, "make_dbconfig", make_dbconfig("set_lg_dir LOG") == 0);

	/* Step 2: open db handle. */
	CuAssert(ct,"open_dbp", open_dbp(&dbenv, &dbp,
	    dtype, 0, NULL, NULL, LOG_DIR, 0, NULL) == 0);
	info->dbenvp = dbenv;
	info->dbp = dbp;

	/* Step 3: store records into db. */
	CuAssert(ct, "store_records", store_records(dbp, 1) == 0);
	CuAssert(ct, "DB->sync", dbp->sync(dbp, 0) == 0);

	/* Step 4: backup the whole environment with callbacks. */
	CuAssert(ct, "backup_env",
	    backup_env(ct, dbenv, flag, has_callback) == 0);

	/*
	 * Step 5: check backup result.
	 * 5a: verify the db file is in BACKUP_DIR.
	 */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 0, NULL, NULL) == 0);

	/* 5b: verify that log files are in BACKUP/LOG. */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dtype, 0, 1, LOG_DIR, LOG_DIR) == 0);

	/* 5c: verify that DB_CONFIG is in BACKUP_DIR. */
	CuAssert(ct, "verify_dbconfig", verify_dbconfig(1) == 0);

	return (0);
}

int TestBackupQueueDB(CuTest *ct) {
	CuAssertTrue(ct, test_backup_onlydbfile(ct, DB_QUEUE, 0) == 0);

	return (0);
}

int TestBackupHeapDB(CuTest *ct) {
	CuAssertTrue(ct, test_backup_onlydbfile(ct, DB_HEAP, 0) == 0);

	return (0);
}

static int
setup_dir(len, dirs)
	u_int32_t len;
	char **dirs;
{
	char path[1024];
	u_int32_t i;
	int ret;

	/* Make related directories. */
	if (len > 0) {
		for (i = 0; i < len; i++) {
			ret = snprintf(path, sizeof(path),"%s%c%s",
			    TEST_ENV, PATH_SEPARATOR[0], dirs[i]);
			if (ret <= 0 || ret >= sizeof(path)) {
				ret = EINVAL;
				return (ret);
			}
			if ((ret = setup_envdir(path, 1)) != 0)
				return (ret);
		}
	}

	return (0);
}

/*
 * open_dbp:
 * 	DB_ENV **dbenvp.
 * 	DB **dbpp.
 * 	DBTYPE dtype: the database type to create.
 * 	u_int32_t ddir_len: the number of data directories.
 * 	char **data_dir: data directories to add.
 * 	const char *create_dir: database creation diretory.
 * 	const char *lg_dir: log directory.
 * 	u_int32_t nparts: the number of partitions.
 * 	DBT *part_key: the partition keys.
 */
static int
open_dbp(dbenvp, dbpp, dtype,
    ddir_len, data_dir, create_dir, lg_dir, nparts, part_key)
	DB_ENV **dbenvp;
	DB **dbpp;
	DBTYPE dtype;
	u_int32_t ddir_len, nparts;
	char **data_dir;
	const char *create_dir, *lg_dir;
	DBT *part_key;
{
	DB_ENV *dbenv;
	DB *dbp;
	const char *part_dir[2];
	u_int32_t i;
	int ret;

	dbenv = NULL;
	dbp = NULL;
	ret = 0;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "db_env_create: %s\n", db_strerror(ret));
		return (ret);
	}

	*dbenvp = dbenv;

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, "TestDbHotBackup");

	/* Add data directories. */
	if (ddir_len > 0 && data_dir != NULL) {
		for (i = 0; i < ddir_len; i++) {
			if ((ret = dbenv->add_data_dir(dbenv,
			    data_dir[i])) != 0) {
				fprintf(stderr, "DB_ENV->add_data_dir: %s\n",
				    db_strerror(ret));
				return (ret);
			}
		}
	}

	/* Set log directory. */
	if (lg_dir != NULL && (ret = dbenv->set_lg_dir(dbenv, lg_dir)) != 0) {
		fprintf(stderr, "DB_ENV->set_lg_dir: %s\n",
		    db_strerror(ret));
		return (ret);
	}

	/* Open the environment. */
	if ((ret = dbenv->open(dbenv, TEST_ENV, DB_CREATE | DB_INIT_LOCK |
	    DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN, 0)) != 0) {
		fprintf(stderr, "DB_ENV->open: %s\n", db_strerror(ret));
		return (ret);
	}

	/* Configure the db handle. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		fprintf(stderr, "db_create: %s\n", db_strerror(ret));
		return (ret);
	}

	*dbpp = dbp;

	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, "TestDbHotBackup");

	/* Set database creation directory. */
	if (create_dir != NULL &&
	    (ret = dbp->set_create_dir(dbp, create_dir)) != 0) {
		fprintf(stderr, "DB_ENV->add_data_dir: %s\n",
		    db_strerror(ret));
		return (ret);
	}

	/* Set partition. */
	if (dtype == DB_BTREE && nparts > 0 && part_key != NULL) {
		if ((ret = dbp->set_partition(dbp,
		    nparts, part_key, NULL)) != 0) {
			dbp->err(dbp, ret, "DB->set_partition");
			return (ret);
		}
		if (create_dir != NULL) {
			part_dir[0]= create_dir;
			part_dir[1] = NULL;
			if ((ret =
			    dbp->set_partition_dirs(dbp, part_dir)) != 0)
				return (ret);
		}
	}

	/* Set queue record length and extent size. */
	if (dtype == DB_QUEUE) {
		if ((ret = dbp->set_re_len(dbp, 50)) != 0) {
			dbp->err(dbp, ret, "DB->set_re_len");
			return (ret);
		}
		if ((ret = dbp->set_q_extentsize(dbp, 1)) != 0) {
			dbp->err(dbp, ret, "DB->set_q_extentsize");
			return (ret);
		}
	} else if (dtype == DB_BTREE) {
		/* Set flag for Btree. */
		if ((ret = dbp->set_flags(dbp, DB_DUPSORT)) != 0) {
			dbp->err(dbp, ret, "DB->set_flags");
			return (ret);
		}
	} else if (dtype == DB_HEAP) {
		/* Set heap region size.  */
		if ((ret = dbp->set_heap_regionsize(dbp, 1)) != 0) {
			dbp->err(dbp, ret, "DB->set_heap_regionsize");
			return (ret);
		}
	}
	
	if ((ret = dbp->set_pagesize(dbp, 512)) != 0) {
		dbp->err(dbp, ret, "DB->set_pagesize");
		return (ret);
	}

	/*Open the db handle. */
	if ((ret = dbp->open(dbp, NULL, BACKUP_DB,
		    NULL, dtype, DB_CREATE, 0644)) != 0) {
		dbp->err(dbp, ret, "%s: DB->open", BACKUP_DB);
		return (ret);
	}

	return (0);
}

/*
 * store_records:
 * 	DB **dbpp.
 * 	u_int32_t iter: put 26 * dups key/data pairs into the database.
 */
static int
store_records(dbp, dups)
	DB *dbp;
	u_int32_t dups;
{
	DBT key, data;
	DBTYPE dtype;
	u_int32_t flag, i, j, num;
	int ret;

	char *buf = "abcdefghijklmnopqrstuvwxyz";
	num = (u_int32_t)strlen(buf);
	flag = 0;

	/* Only accepts dups which is between 1 and 26 inclusively. */
	if (dups < 1 || dups > num)
		return (EINVAL);

	if ((dbp->get_type(dbp, &dtype)) != 0)
		return (EINVAL);
	if (dtype == DB_HEAP || dtype == DB_QUEUE || dtype == DB_RECNO)
		flag = DB_APPEND;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	for (i = 0; i < num; i++) {
		key.data = &i;
		key.size = sizeof(i);

		/*
		 * If "dups" > 1, we are putting duplicate records into the db
		 * and the number of records for each key is "dups". We already
		 * set DB_DUPSORT when opening the db.
		 */
		for (j = 1; j <= dups; j++) {
			data.data = &buf[0];
			data.size = j * sizeof(char);

			if ((ret = dbp->put(dbp,
			    NULL, &key, &data, flag)) != 0) {
				dbp->err(dbp, ret, "DB->put");
				return (ret);
			}
		}
	}
	return (ret);
}

static int
cleanup_test(dbenv, dbp)
	DB_ENV *dbenv;
	DB *dbp;
{
	int ret, t_ret;

	ret = 0;;
	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0) 
		fprintf(stderr, "DB->close: %s\n", db_strerror(ret));

	if (dbenv != NULL && (t_ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(t_ret));
		ret = t_ret;
	}

	teardown_envdir(TEST_ENV);
	teardown_envdir(BACKUP_DIR);

	return (ret);
}

/*
 * backup_env:
 * 	CuTest *ct.
 * 	DB_ENV *dbenv: the environment to backup.
 * 	u_int32_t flags: hotbackup flags.
 * 	int has_callback: 0 if not use callback, 1 otherwise.
 */
static int
backup_env(ct, dbenv, flags, has_callback)
	CuTest *ct;
	DB_ENV *dbenv;
	u_int32_t flags;
	int has_callback;
{
	if (has_callback != 0) {
		CuAssert(ct, "DB_ENV->set_backup_callbacks",
		    dbenv->set_backup_callbacks(dbenv, backup_open,
		    backup_write, backup_close) == 0);
	}
	CuAssert(ct, "DB_ENV->backup",
	    dbenv->backup(dbenv, BACKUP_DIR, flags) == 0);
	return (0);
}

/*
 * backup_db:
 * 	CuTest *ct.
 * 	DB_ENV *dbenv: the environment to backup.
 * 	const char *dname: the name of db file to backup.
 * 	u_int32_t flags: hot_backup flags.
 * 	int has_callback: 0 if not use callback, 1 otherwise.
 */
static int
backup_db(ct, dbenv, dname, flags, has_callback)
	CuTest *ct;
	DB_ENV *dbenv;
	const char *dname;
	u_int32_t flags;
	int has_callback;
{
	if (has_callback != 0) {
		CuAssert(ct, "DB_ENV->set_backup_callbacks",
		    dbenv->set_backup_callbacks(dbenv, backup_open,
		    backup_write, backup_close) == 0);
	}
	CuAssert(ct, "DB_ENV->dbbackup",
	    dbenv->dbbackup(dbenv, dname, BACKUP_DIR, flags) == 0);
	return (0);
}

/*
 * verify_db_log:
 * 	DBTYPE dtype: the database type.
 * 	u_int32_t is_part: 0 if the database is not partitioned, 1 otherwise.
 * 	u_int32_t is_lg: 1 if verifying the log files, 0 otherwise.
 * 	const char *test_cmpdir: the db creation directory or log directory
 * 	    under TEST_ENV.
 * 	const char *backup_cmpdir: the db creation directory or log directory
 * 	    under BACKUP_DIR.
 */
static int
verify_db_log(dtype, is_part, is_lg, test_cmpdir, backup_cmpdir)
	DBTYPE dtype;
	u_int32_t is_part, is_lg;
	const char *test_cmpdir, *backup_cmpdir;
{
	char *buf1, *buf2, *path1, *path2, *pfx;
	char **names1, **names2;
	int cnt1, cnt2, i, m_cnt, ret, t_cnt1, t_cnt2;

	buf1 = buf2 = path1 = path2 = pfx = NULL;
	names1 = names2 = NULL;
	cnt1 = cnt2 = i = m_cnt = ret = t_cnt1 = t_cnt2  = 0;

	/* Either verify db files or log files. */
	if (is_part != 0 &&
	    (is_lg != 0 || (dtype != DB_BTREE && dtype != DB_HASH)))
		return (EINVAL);

	/* Get the data or log directory paths. */
	if ((ret = __os_calloc(NULL, 100, 1, &path1)) != 0)
		goto err;
	if ((ret = __os_calloc(NULL, 100, 1, &path2)) != 0)
		goto err;

	if (test_cmpdir != NULL) {
		ret = snprintf(path1, 100, "%s%c%s",
		    TEST_ENV, PATH_SEPARATOR[0], test_cmpdir);
		if (ret <= 0 || ret >= 100) {
			ret = EINVAL;
			goto err;
		}
		ret = 0;
	} else
		snprintf(path1, 100, "%s", TEST_ENV);

	if (backup_cmpdir != NULL) {
		ret = snprintf(path2, 100, "%s%c%s",
		    BACKUP_DIR, PATH_SEPARATOR[0], backup_cmpdir);
		if (ret <= 0 || ret >= 100) {
			ret = EINVAL;
			goto err;
		}
		ret = 0;
	} else
		snprintf(path2, 100, "%s", BACKUP_DIR);

	/* Define the prefix of partition db, queue extent or log files. */
	if ((ret = __os_calloc(NULL, 10, 1, &pfx)) != 0)
		goto err;
	if (is_lg != 0)
		snprintf(pfx, 10, "%s", "log.");
	else if (is_part != 0)
		snprintf(pfx, 10, "%s", "__dbp.");
	else if (dtype == DB_QUEUE)
		snprintf(pfx, 10, "%s", "__dbq.");
	else
		pfx[0] = '\0';

	/* Get the lists of db file, partition files, queue extent or logs. */
	if ((ret = __os_dirlist(NULL, path1, 0, &names1, &cnt1)) != 0)
		return (ret);
	if ((ret = __os_dirlist(NULL, path2, 0, &names2, &cnt2)) != 0)
		return (ret);

	/* Get the file numbers. */
	m_cnt = cnt1 > cnt2 ? cnt1 : cnt2;
	t_cnt1 = cnt1;
	t_cnt2 = cnt2;
	for (i = 0; i < m_cnt; i++) {
		if (i < cnt1 && ((is_lg != 0 &&
		    strncmp(names1[i], pfx, strlen(pfx)) != 0) ||
		    strncmp(names1[i], BACKUP_DB, strlen(BACKUP_DB)) != 0 &&
		    (strlen(pfx) > 0 ?
		    strncmp(names1[i], pfx, strlen(pfx)) != 0 : 1))) {
			t_cnt1--;
			names1[i] = NULL;
		}
		if (i < cnt2 && ((is_lg != 0 &&
		    strncmp(names2[i], pfx, strlen(pfx)) != 0) ||
		    strncmp(names2[i], BACKUP_DB, strlen(BACKUP_DB)) != 0 &&
		    (strlen(pfx) > 0 ?
		    strncmp(names2[i], pfx, strlen(pfx)) != 0 : 1))) {
			t_cnt2--;
			names2[i] = NULL;
		}
	}
	if ((ret = t_cnt1 == t_cnt2 ? 0 : EXIT_FAILURE) != 0)
		goto err;

	/* Compare each file. */
	if ((ret = __os_calloc(NULL, 100, 1, &buf1)) != 0)
		goto err;
	if ((ret = __os_calloc(NULL, 100, 1, &buf2)) != 0)
		goto err;
	for (i = 0; i < cnt1; i++) {
		if (names1[i] == NULL)
			continue;
		snprintf(buf1, 100, "%s%c%s",
		    path1, PATH_SEPARATOR[0], names1[i]);
		snprintf(buf2, 100, "%s%c%s",
		    path2, PATH_SEPARATOR[0], names1[i]);
		if ((ret = cmp_files(buf1, buf2)) != 0)
			break;
	}

err:	if (buf1 != NULL)
		__os_free(NULL, buf1);
	if (buf2 != NULL)
		__os_free(NULL, buf2);
	if (path1 != NULL)
		__os_free(NULL, path1);
	if (path2 != NULL)
		__os_free(NULL, path2);
	if (pfx != NULL)
		__os_free(NULL, pfx);
	return (ret);
}

/*
 * verify_dbconfig:
 * 	u_int32_t is_exist: 1 if DB_CONFIG is expected to exist
 * 	    in BACKUP_DIR, 0 otherwise.
 */
static int
verify_dbconfig(is_exist)
	u_int32_t is_exist;
{
	char *path1, *path2;
	int ret;

	path1 = path2 = NULL;
	ret = 0;

	if ((ret = __os_calloc(NULL, 100, 1, &path1)) != 0)
		goto err;
	if ((ret = __os_calloc(NULL, 100, 1, &path2)) != 0)
		goto err;

	if (is_exist == 0) {
		if ((ret = __os_exists(NULL, "BACKUP/DB_CONFIG", 0)) != 0) {
			ret = 0;
			goto err;
		} else {
			ret = EXIT_FAILURE;
			goto err;
		}
	} else {
		snprintf(path1, 100, "%s%c%s",
		    TEST_ENV, PATH_SEPARATOR[0], "DB_CONFIG");
		snprintf(path2, 100, "%s%c%s",
		    BACKUP_DIR, PATH_SEPARATOR[0], "DB_CONFIG");
		if ((ret = cmp_files(path1, path2)) != 0)
			goto err;
	}

err:	if (path1 != NULL)
		__os_free(NULL, path1);
	if (path2 != NULL)
		__os_free(NULL, path2);
	return (ret);
}

static int
backup_open(dbenv, dbname, target, handle)
	DB_ENV *dbenv;
	const char *dbname;
	const char *target;
	void **handle;
{
	DB_FH *fp;
	const char *t_target;
	char str[1024];
	u_int32_t flags;
	int ret;

	fp = NULL;
	flags = DB_OSO_CREATE;
	ret = 0;

	if (target == NULL)
		t_target = BACKUP_DIR;
	else
		t_target = target;
	sprintf(str, "%s%s%s", t_target, "/", dbname);

	if ((ret = __os_open(NULL, str, 0, flags, DB_MODE_600, &fp)) != 0)
		return (ret);

	*handle = fp;

	return (ret);
}

static int
backup_write(dbenv, gigs, offset, size, buf, handle)
	DB_ENV *dbenv;
	u_int32_t gigs, offset, size;
	u_int8_t *buf;
	void *handle;
{
	DB_FH *fp;
	size_t nw;
	int ret;

	ret = 0;
	fp = (DB_FH *)handle;

	if (size <= 0)
		return (EINVAL);

	if ((ret = __os_write(NULL, fp, buf, size, &nw)) != 0)
		return (ret);
	if (nw != size)
		ret = EIO;

	return (ret);
}

static int
backup_close(dbenv, dbname, handle)
	DB_ENV *dbenv;
	const char *dbname;
	void *handle;
{
	DB_FH *fp;
	int ret;

	fp = (DB_FH *)handle;
	ret = 0;

	ret = __os_closehandle(NULL, fp);

	return (ret);

}

static int
make_dbconfig(content)
	const char * content;
{
	FILE *fp;
	char *str;
	int ret, size;

	ret = 0;

	if (content == NULL)
		return (0);
	if ((fp = fopen("TESTDIR/DB_CONFIG", "w")) == NULL)
		return (EINVAL);

	if ((ret = __os_calloc(NULL, 1024, 1, &str)) != 0)
		goto err;
	size = snprintf(str, 1024, "%s", content);
	if (size < 0 || size >= 1024) {
		ret = EINVAL;
		goto err;
	}

	if (fputs(str, fp) == EOF)
		ret = EXIT_FAILURE;

err:	if (fclose(fp) == EOF)
		ret = EXIT_FAILURE;
	if (str != NULL)
		__os_free(NULL, str);

	return (ret);
}

static int
cmp_files(name1, name2)
	const char *name1;
	const char *name2;
{
	DB_FH *fhp1, *fhp2;
	void *buf1, *buf2;
	size_t nr1, nr2;
	int ret, t_ret;

	fhp1 = fhp2 = NULL;
	buf1 = buf2 = NULL;
	nr1 = nr2 = 0;
	ret = t_ret = 0;

	/* Allocate memory for buffers. */
	if ((ret = __os_calloc(NULL, MEGABYTE, 1, &buf1)) != 0)
		goto err;
	if ((ret = __os_calloc(NULL, MEGABYTE, 1, &buf2)) != 0)
		goto err;

	/* Open the input files. */
	if ((ret = __os_open(NULL, name1, 0, DB_OSO_RDONLY, 0, &fhp1)) != 0 ||
	    (t_ret = __os_open(NULL, name2, 0,
	    DB_OSO_RDONLY, 0, &fhp2)) != 0) {
		if (ret == 0)
			ret = t_ret;
		goto err;
	}

	/* Read and compare the file content. */
	while ((ret = __os_read(NULL, fhp1, buf1, MEGABYTE, &nr1)) == 0 &&
	    nr1 > 0 && (ret = __os_read(NULL, fhp2,
	    buf2, MEGABYTE, &nr2)) == 0 && nr2 > 0) {
		if (nr1 != nr2) {
			ret = EXIT_FAILURE;
			break;
		}
		if ((ret = memcmp(buf1, buf2, nr1)) != 0)
			break;
	}
	if(ret == 0 && nr1 > 0 && nr2 > 0 && nr1 != nr2)
		ret = EXIT_FAILURE;

err:	if (buf1 != NULL)
		__os_free(NULL, buf1);
	if (buf2 != NULL)
		__os_free(NULL, buf2);
	if (fhp1 != NULL && (t_ret = __os_closehandle(NULL, fhp1)) != 0)
		ret = t_ret;
	if (fhp2 != NULL && (t_ret = __os_closehandle(NULL, fhp2)) != 0)
		ret = t_ret;
	return (ret);
}

static int
test_backup_onlydbfile(ct, dbtype, has_callback)
	CuTest *ct;
	DBTYPE dbtype;
	int has_callback;
{
	DB_ENV *dbenv;
	DB *dbp;
	struct handlers *info;
	char **names;
	int (*closep)(DB_ENV *, const char *, void *);
	int (*openp)(DB_ENV *, const char *, const char *, void **);
	int (*writep)(DB_ENV *,u_int32_t,
	    u_int32_t, u_int32_t, u_int8_t *, void *);
	int cnt, i, t_cnt;
	u_int32_t flag;

	info = ct->context;
	flag = DB_EXCL;
	closep = NULL;
	openp = NULL;
	writep = NULL;

	/* Step 1: set up directories. */
	CuAssert(ct, "setup_dir", setup_dir(0, NULL) == 0);

	/* Step 2: open db handle. */
	CuAssert(ct,"open_dbp", open_dbp(&dbenv, &dbp,
	    dbtype, 0, NULL, NULL, NULL, 0, NULL) == 0);
	info->dbenvp = dbenv;
	info->dbp = dbp;

	/* Step 3: store records into db. */
	CuAssert(ct, "store_records", store_records(dbp, 10) == 0);
	CuAssert(ct, "DB->sync", dbp->sync(dbp, 0) == 0);

	/* Step 4: backup only the db file. */
	CuAssert(ct, "backup_db",
	    backup_db(ct, dbenv, BACKUP_DB, flag, has_callback) == 0);

	/*
	 * Step 5: check backup result.
	 * 5a: verify db file is in BACKUP_DIR.
	 */
	CuAssert(ct, "verify_db_log",
	    verify_db_log(dbtype, 0, 0, NULL, NULL) == 0);

	/* 5b: verify no other files are in BACKUP_DIR. */
	CuAssert(ct, "__os_dirlist",
	    __os_dirlist(NULL, BACKUP_DIR, 0, &names, &cnt) == 0);
	if (dbtype != DB_QUEUE)
		CuAssert(ct, "too many files in backupdir", cnt == 1);
	else {
		t_cnt = cnt;
		for (i = 0; i < t_cnt; i++) {
			if (strncmp(names[i], "__dbq.", 6) == 0)
				cnt--;
		}
		CuAssert(ct, "too many files in backupdir", cnt == 1);
	}

	/* Step 6: verify the backup callback. */
	CuAssert(ct, "DB_ENV->get_backup_callbacks",
	    dbenv->get_backup_callbacks(dbenv,
	    &openp, &writep, &closep) == (has_callback != 0 ? 0 : EINVAL));
	if (has_callback != 0) {
		CuAssertTrue(ct, openp == backup_open);
		CuAssertTrue(ct, writep == backup_write);
		CuAssertTrue(ct, closep == backup_close);
	}

	return (0);
}
