/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * Test setting and getting the configuration before handle open [#21552]
 *
 * It tests all the settings on the following handles:
 *   1. DB_ENV
 *   2. DB
 *   3. DB_MPOOLFILE
 *   4. DB_SEQUENCE
 * These handles have separate steps for 'create' and 'open', so that
 * we can do pre-open configuration.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

/*
 * Test functions like DB_ENV->set_lk_max_lockers and
 * and DB_ENV->get_lk_max_lockers, among which the setter function accepts a
 * number as the second argument while the getter function accepts a pointer
 * to number.
 */
#define CHECK_1_DIGIT_VALUE(handle, setter, getter, type, v) do {	\
	type vs, vg;							\
	vs = (type)(v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg) == 0);			\
	CuAssert(ct, #getter"=="#setter, vs == vg);			\
} while(0)

/*
 * Test all possible values for functions like DB_ENV->set_lk_detect and
 * DB_ENV->get_lk_detect. The values for these functions are in a small set.
 */
#define CHECK_1_DIGIT_VALUES(handle, setter, getter, type, values) do {	\
	size_t cnt, i;							\
	cnt = sizeof(values) / sizeof(values[0]);			\
	for (i = 0; i < cnt; i++) {					\
		CHECK_1_DIGIT_VALUE(handle, setter, getter, type,	\
		    values[i]);						\
	}								\
} while(0)

/*
 * Test functions like DB_ENV->set_backup_config and DB_ENV->get_backup_config,
 * among which both getter and setter functions accept an option as the second
 * argument, and for the third argument the setter accepts a number while
 * getter accepts a pointer to number.
 */
#define CHECK_1_DIGIT_CONFIG_VALUE(handle, setter, getter, opt, type, v)\
	do {								\
	type vs, vg;							\
	vs = (type)(v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), (opt), vs) == 0);		\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), (opt), &vg) == 0);		\
	CuAssert(ct, #getter"=="#setter, vs == vg);			\
} while(0)

/*
 * Test all the options for functions satisfying CHECK_1_DIGIT_CONFIG_VALUE.
 * The configuration value has no specific limit.
 */
#define CHECK_1_DIGIT_CONFIG_VALUES(handle, setter, getter, configs, type)\
	do {								\
	size_t cnt, i;							\
	cnt = sizeof(configs) / sizeof(configs[0]);			\
	for (i = 0; i < cnt; i++) {					\
		CHECK_1_DIGIT_CONFIG_VALUE(handle, setter, getter,	\
		    configs[i], type, rand());				\
	}								\
} while(0)

/*
 * Test turning on/off all the options for functions like DB_ENV->set_verbose
 * and DB_ENV->get_verbose.
 */
#define CHECK_ONOFF(handle, setter, getter, options, opt_cnt, type) do {\
	size_t i;							\
	for (i = 0; i < (opt_cnt); i++) {				\
		CHECK_1_DIGIT_CONFIG_VALUE(handle, setter, getter,	\
		    (options)[i], type, 1);				\
		CHECK_1_DIGIT_CONFIG_VALUE(handle, setter, getter,	\
		    (options)[i], type, 0);		 		\
	}								\
} while(0)

/*
 * Like CHECK_1_DIGIT_CONFIG_VALUE, but the number or pointer to number
 * is the second argument while the option is the third argument.
 */
#define CHECK_1_DIGIT_CONFIG_VALUE2(handle, setter, getter, opt, type, v)\
	do {								\
	type vs, vg;						 	\
	vs = (type)(v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs, (opt)) == 0);		\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg, (opt)) == 0);		\
	CuAssert(ct, #getter"=="#setter, vs == vg);			\
} while(0)

/*
 * Test all the options for functions satisfying CHECK_1_DIGIT_CONFIG_VALUE2.
 * The configuration value has no specific limit.
 */
#define CHECK_1_DIGIT_CONFIG_VALUES2(handle, setter, getter, configs, type)\
	do {								\
	size_t cnt, i;							\
	cnt = sizeof(configs) / sizeof(configs[0]);			\
	for (i = 0; i < cnt; i++) {					\
		CHECK_1_DIGIT_CONFIG_VALUE2(handle, setter, getter,	\
		    configs[i], type, rand());				\
	}								\
} while(0)

/*
 * Test functions like DB_ENV->set_create_dir and and DB_ENV->get_create_dir,
 * among which the setter function accepts a string(const char *) as the
 * second argument, and the getter function accepts a pointer to string.
 */
#define CHECK_1_STR_VALUE(handle, setter, getter, v) do {		\
	const char *vs, *vg;						\
	vs = (v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg) == 0);			\
	CuAssert(ct, #getter"=="#setter, strcmp(vs, vg) == 0);		\
} while(0)

/*
 * Like CHECK_1_STR_VALUE, but both setter and getter functions do
 * not return anything.
 */
#define CHECK_1_STR_VALUE_VOID(handle, setter, getter, v) do {		\
	const char *vs, *vg;						\
	vs = (v);							\
	(handle)->setter((handle), vs);					\
	(handle)->getter((handle), &vg);				\
	CuAssert(ct, #getter"=="#setter, strcmp(vs, vg) == 0);		\
} while(0)

/*
 * Test functions like DB_ENV->set_errfile and and DB_ENV->get_errfile,
 * among which the setter function accepts a pointer as the second
 * argument, and the getter function accepts a pointer to pointer.
 */
#define CHECK_1_PTR_VALUE(handle, setter, getter, type, v) do {		\
	type *vs, *vg;							\
	vs = (v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg) == 0);			\
	CuAssert(ct, #getter"=="#setter, vs == vg);			\
} while(0)

/*
 * Like CHECK_1_PTR_VALUE, but both setter and getter functions do
 * not return anything.
 */
#define CHECK_1_PTR_VALUE_VOID(handle, setter, getter, type, v) do {	\
	type *vs, *vg;							\
	vs = (v);							\
	(handle)->setter((handle), vs);					\
	(handle)->getter((handle), &vg);				\
	CuAssert(ct, #getter"=="#setter, vs == vg);			\
} while(0)

/*
 * Test functions like DB_ENV->set_memory_max and and DB_ENV->get_memory_max,
 * among which the setter function accepts two numbers the second and third
 * argument, while the getter function accepts two pointers to number.
 */
#define CHECK_2_DIGIT_VALUES(handle, setter, getter, type1, v1, type2, v2)\
	do {								\
	type1 vs1, vg1;							\
	type2 vs2, vg2;							\
	vs1 = (type1)(v1);						\
	vs2 = (type2)(v2);						\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs1, vs2) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg1, &vg2) == 0);		\
	CuAssert(ct, #getter"=="#setter, vs1 == vg1);			\
	CuAssert(ct, #getter"=="#setter, vs2 == vg2);			\
} while(0)

/*
 * Test functions like DB_ENV->set_cachesize and and DB_ENV->get_cachesize,
 * among which the setter function accepts three numbers as the second and
 * third and fourth argument, while the getter function accepts three pointers
 * to number.
 */
#define CHECK_3_DIGIT_VALUES(handle, setter, getter, type1, v1, type2,	\
    v2, type3, v3) do {							\
	type1 vs1, vg1;							\
	type2 vs2, vg2;							\
	type3 vs3, vg3;							\
	vs1 = (type1)(v1);						\
	vs2 = (type2)(v2);						\
	vs3 = (type3)(v3);						\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs1, vs2, v3) == 0);		\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg1, &vg2, &vg3) == 0);		\
	CuAssert(ct, #getter"=="#setter, vs1 == vg1);			\
	CuAssert(ct, #getter"=="#setter, vs2 == vg2);			\
	CuAssert(ct, #getter"=="#setter, vs3 == vg3);			\
} while(0)

/*
 * Test functions like DB->set_flags and DB->get_flags, among which the setter
 * function accepts an inclusive'OR of some individual options while the getter
 * accepts a pointer to store the options composition.
 * In this case, the getter may not return the exact value set by setter.
 */
#define CHECK_FLAG_VALUE(handle, setter, getter, type, v) do {		\
	type vs, vg;							\
	vs = (type)(v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg) == 0);			\
	CuAssert(ct, #getter"=="#setter, (vs & vg) == vs);		\
} while(0)

/*
 * Test functions like DB_ENV->set_flags and DB_ENV->get_flags, among which
 * the setter function accepts an individual option and an on/off value(1/0)
 * while the getter accepts a pointer to store the options composition.
 */
#define CHECK_FLAG_VALUE_ONOFF(handle, setter, getter, type, v, on) do {\
	type vs, vg;							\
	vs = (type)(v);							\
	CuAssert(ct, #handle"->"#setter,				\
	    (handle)->setter((handle), vs, on) == 0);			\
	CuAssert(ct, #handle"->"#getter,				\
	    (handle)->getter((handle), &vg) == 0);			\
	if (on) {							\
		CuAssert(ct, #getter"=="#setter, (vs & vg) == vs);	\
	} else {							\
		CuAssert(ct, #getter"=="#setter, (vs & vg) == 0);	\
	}								\
} while(0)

/*
 * Test turning on/off all the options for DB_ENV->set_flags.
 */
#define CHECK_ENV_FLAGS(handle, values) do {				\
	size_t i, cnt;							\
	cnt = sizeof(values) / sizeof(values[0]);			\
	for (i = 0; i < cnt; i++) {					\
		/* We only set the direct I/O if the os supports it. */	\
		if ((values[i] & DB_DIRECT_DB) != 0 &&			\
		    __os_support_direct_io() == 0)			\
			continue;					\
		CHECK_FLAG_VALUE_ONOFF(handle, set_flags,		\
		    get_flags, u_int32_t, values[i], 1); 		\
		CHECK_FLAG_VALUE_ONOFF(handle, set_flags,		\
		    get_flags, u_int32_t, values[i], 0);		\
	} 								\
} while(0)

struct handlers {
	DB_ENV *dbenvp;
	DB *dbp;
	DB_MPOOLFILE *mp;
	DB_SEQUENCE *seqp;
};
static struct handlers info;
static const char *data_dirs[] = {
	"data_dir1",
	"data_dir2",
	"data_dir3",
	"data_dir4",
	NULL
};
const char *passwd = "passwd1";
static FILE *errfile, *msgfile;

static int add_dirs_to_dbenv(DB_ENV *dbenv, const char **dirs);
static int close_db_handle(DB *dbp);
static int close_dbenv_handle(DB_ENV *dbenvp);
static int close_mp_handle(DB_MPOOLFILE *mp);
static int close_seq_handle(DB_SEQUENCE *seqp);
static int cmp_dirs(const char **dirs1, const char **dirs2);
static int create_db_handle(DB **dbpp, DB_ENV *dbenv);
static int create_dbenv_handle(DB_ENV **dbenvpp);
static int create_mp_handle(DB_MPOOLFILE **mpp, DB_ENV *dbenv);
static int create_seq_handle(DB_SEQUENCE **seqpp, DB *dbp);

int TestPreOpenSetterAndGetterSuiteSetup(CuSuite *suite) {
	srand((unsigned int)time(NULL));
	return (0);
}

int TestPreOpenSetterAndGetterSuiteTeardown(CuSuite *suite) {
	return (0);
}

int TestPreOpenSetterAndGetterTestSetup(CuTest *ct) {
	char buf[DB_MAXPATHLEN];

	setup_envdir(TEST_ENV, 1);
	sprintf(buf, "%s/%s", TEST_ENV, "errfile");
	errfile = fopen(buf, "w");
	CuAssert(ct, "open errfile", errfile != NULL);
	sprintf(buf, "%s/%s", TEST_ENV, "msgfile");
	msgfile = fopen(buf, "w");
	CuAssert(ct, "open msgfile", msgfile != NULL);

	info.dbenvp = NULL;
	info.dbp = NULL;
	info.mp = NULL;
	info.seqp = NULL;

	return (0);
}

int TestPreOpenSetterAndGetterTestTeardown(CuTest *ct) {
	CuAssert(ct, "close errfile", fclose(errfile) == 0);
	CuAssert(ct, "close msgfile", fclose(msgfile) == 0);
	/*
	 * Close the handle in case failure happens.
	 */
	if (info.seqp != NULL)
		CuAssert(ct, "seqp->close",
		    info.seqp->close(info.seqp, 0) == 0);
	if (info.mp != NULL)
		CuAssert(ct, "mp->close", info.mp->close(info.mp, 0) == 0);
	if (info.dbp != NULL)
		CuAssert(ct, "dbp->close",
		    info.dbp->close(info.dbp, 0) == 0);
	if (info.dbenvp != NULL)
		CuAssert(ct, "dbenvp->close",
		    info.dbenvp->close(info.dbenvp, 0) == 0);
	return (0);
}

/*
 * For most number arguments, if there is no special requirement, we use
 * a random value, since most checks are done during open and we do not
 * do handle open in all the following tests.
 *
 * If a configuration has many options, we will cover all options. If
 * it accepts inclusive'OR of the options, we will test some OR's as well.
 */

int TestEnvPreOpenSetterAndGetter(CuTest *ct) {
	DB_ENV *dbenv, *repmgr_dbenv;
	const char **dirs;
	const u_int8_t *lk_get_conflicts;
	u_int8_t lk_set_conflicts[] = {1, 0, 0, 0};
	u_int32_t backup_configs[] = {
		/*
		 * DB_BACKUP_WRITE_DIRECT is not listed here, since the
		 * value(only 1/0) for this configuration is different
		 * from others. So we test it separately.
		 */
		DB_BACKUP_READ_COUNT,
		DB_BACKUP_READ_SLEEP,
		DB_BACKUP_SIZE
	};
	u_int32_t env_flags[] = {
		DB_CDB_ALLDB,
		DB_AUTO_COMMIT | DB_MULTIVERSION | DB_REGION_INIT |
		    DB_TXN_SNAPSHOT,
		DB_DSYNC_DB | DB_NOLOCKING | DB_NOMMAP | DB_NOPANIC,
		DB_OVERWRITE | DB_TIME_NOTGRANTED | DB_TXN_NOSYNC |
		    DB_YIELDCPU,
		DB_TXN_NOWAIT | DB_TXN_WRITE_NOSYNC,
		DB_DIRECT_DB
	};
	u_int32_t env_timeouts[] = {
		DB_SET_LOCK_TIMEOUT,
		DB_SET_REG_TIMEOUT,
		DB_SET_TXN_TIMEOUT
	};
	u_int32_t lk_detect_values[] = {
		DB_LOCK_DEFAULT,
		DB_LOCK_EXPIRE,
		DB_LOCK_MAXLOCKS,
		DB_LOCK_MAXWRITE,
		DB_LOCK_MINLOCKS,
		DB_LOCK_MINWRITE,
		DB_LOCK_OLDEST,
		DB_LOCK_RANDOM,
		DB_LOCK_YOUNGEST
	};
	u_int32_t log_configs[] = {
		DB_LOG_DIRECT,
		DB_LOG_DSYNC,
		DB_LOG_AUTO_REMOVE,
		DB_LOG_IN_MEMORY,
		DB_LOG_ZERO
	};
	DB_MEM_CONFIG mem_configs[] = {
		DB_MEM_LOCK,
		DB_MEM_LOCKOBJECT,
		DB_MEM_LOCKER,
		DB_MEM_LOGID,
		DB_MEM_TRANSACTION,
		DB_MEM_THREAD
	};
	u_int32_t rep_configs[] = {
		DB_REP_CONF_AUTOINIT,
		DB_REP_CONF_AUTOROLLBACK,
		DB_REP_CONF_BULK,
		DB_REP_CONF_DELAYCLIENT,
		DB_REP_CONF_INMEM,
		DB_REP_CONF_LEASE,
		DB_REP_CONF_NOWAIT,
		DB_REPMGR_CONF_ELECTIONS,
		DB_REPMGR_CONF_2SITE_STRICT
	};
	u_int32_t rep_timeouts[] = {
		DB_REP_CHECKPOINT_DELAY,
		DB_REP_ELECTION_TIMEOUT,
		DB_REP_FULL_ELECTION_TIMEOUT,
		DB_REP_LEASE_TIMEOUT
	};
	u_int32_t repmgr_timeouts[] = {
		DB_REP_ACK_TIMEOUT,
		DB_REP_CONNECTION_RETRY,
		DB_REP_ELECTION_RETRY,
		DB_REP_HEARTBEAT_MONITOR,
		DB_REP_HEARTBEAT_SEND
	};
	u_int32_t verbose_flags[] = {
		DB_VERB_BACKUP,
		DB_VERB_DEADLOCK,
		DB_VERB_FILEOPS,
		DB_VERB_FILEOPS_ALL,
		DB_VERB_RECOVERY,
		DB_VERB_REGISTER,
		DB_VERB_REPLICATION,
		DB_VERB_REP_ELECT,
		DB_VERB_REP_LEASE,
		DB_VERB_REP_MISC,
		DB_VERB_REP_MSGS,
		DB_VERB_REP_SYNC,
		DB_VERB_REP_SYSTEM,
		DB_VERB_REPMGR_CONNFAIL,
		DB_VERB_REPMGR_MISC,
		DB_VERB_WAITSFOR
	};
	u_int32_t encrypt_flags;
	size_t log_configs_cnt, rep_configs_cnt, verbose_flags_cnt;
	int lk_get_nmodes, lk_set_nmodes;
	time_t tx_get_timestamp, tx_set_timestamp;

	verbose_flags_cnt = sizeof(verbose_flags) / sizeof(verbose_flags[0]);
	log_configs_cnt = sizeof(log_configs) / sizeof(log_configs[0]);
	rep_configs_cnt = sizeof(rep_configs) / sizeof(rep_configs[0]);
	
	CuAssert(ct, "db_env_create", create_dbenv_handle(&dbenv) == 0);

	/* Test the DB_ENV->add_data_dir(), DB_ENV->get_data_dirs(). */
	CuAssert(ct, "add_dirs_to_dbenv",
	    add_dirs_to_dbenv(dbenv, data_dirs) == 0);
	CuAssert(ct, "dbenv->get_data_dirs",
	    dbenv->get_data_dirs(dbenv, &dirs) == 0);
	CuAssert(ct, "cmp_dirs", cmp_dirs(data_dirs, dirs) == 0);

	/* Test DB_ENV->set_backup_config(), DB_ENV->get_backup_config(). */
	CHECK_1_DIGIT_CONFIG_VALUE(dbenv, set_backup_config,
	    get_backup_config, DB_BACKUP_WRITE_DIRECT, u_int32_t, 1);
	CHECK_1_DIGIT_CONFIG_VALUE(dbenv, set_backup_config,
	    get_backup_config, DB_BACKUP_WRITE_DIRECT, u_int32_t, 0);
	CHECK_1_DIGIT_CONFIG_VALUES(dbenv, set_backup_config, get_backup_config,
	    backup_configs, u_int32_t);
	
	/* Test DB_ENV->set_create_dir(), DB_ENV->get_create_dir(). */
	CHECK_1_STR_VALUE(dbenv, set_create_dir, get_create_dir, data_dirs[1]);

	/* Test DB_ENV->set_encrypt(), DB_ENV->get_encrypt_flags(). */
	CuAssert(ct, "dbenv->set_encrypt",
	    dbenv->set_encrypt(dbenv, passwd, DB_ENCRYPT_AES) == 0);
	CuAssert(ct, "dbenv->get_encrypt_flags",
	    dbenv->get_encrypt_flags(dbenv, &encrypt_flags) == 0);
	CuAssert(ct, "check encrypt flags", encrypt_flags == DB_ENCRYPT_AES);

	/* Test DB_ENV->set_errfile(), DB_ENV->get_errfile(). */
	CHECK_1_PTR_VALUE_VOID(dbenv, set_errfile, get_errfile, FILE, errfile);

	/* Test DB_ENV->set_errpfx(), DB_ENV->get_errpfx(). */
	CHECK_1_STR_VALUE_VOID(dbenv, set_errpfx, get_errpfx, "dbenv0");

	/* Test DB_ENV->set_flags(), DB_ENV->get_flags(). */
	CHECK_ENV_FLAGS(dbenv, env_flags);

	/*
	 * Test DB_ENV->set_intermediate_dir_mode(),
	 * DB_ENV->get_intermediate_dir_mode().
	 */
	CHECK_1_STR_VALUE(dbenv,
	    set_intermediate_dir_mode, get_intermediate_dir_mode, "rwxr-xr--");

	/* Test DB_ENV->set_memory_init(), DB_ENV->get_memory_init(). */
	CHECK_1_DIGIT_CONFIG_VALUES(dbenv, set_memory_init, get_memory_init,
	    mem_configs, u_int32_t);

	/*
	 * Test DB_ENV->set_memory_max(), DB_ENV->get_memory_max().
	 * The code will adjust the values if necessary, and using
	 * random values can not guarantee the returned values
	 * are exactly what we set. So we will not use random values here.
	 */
	CHECK_2_DIGIT_VALUES(dbenv, set_memory_max, get_memory_max,
	    u_int32_t, 2, u_int32_t, 1048576);

	/* Test DB_ENV->set_metadata_dir(), DB_ENV->get_metadata_dir(). */
	CHECK_1_STR_VALUE(dbenv,
	    set_metadata_dir, get_metadata_dir, data_dirs[2]);

	/* Test DB_ENV->set_msgfile(), DB_ENV->get_msgfile(). */
	CHECK_1_PTR_VALUE_VOID(dbenv, set_msgfile, get_msgfile, FILE, msgfile);

	/* Test DB_ENV->set_shm_key(), DB_ENV->get_shm_key(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_shm_key, get_shm_key, long, rand());

	/* Test DB_ENV->set_timeout(), DB_ENV->get_timeout(). */
	CHECK_1_DIGIT_CONFIG_VALUES2(dbenv, set_timeout, get_timeout,
	    env_timeouts, db_timeout_t);

	/* Test DB_ENV->set_tmp_dir(), DB_ENV->get_tmp_dir(). */
	CHECK_1_STR_VALUE(dbenv, set_tmp_dir, get_tmp_dir, "/temp");

	/* Test DB_ENV->set_verbose(), DB_ENV->get_verbose(). */
	CHECK_ONOFF(dbenv, set_verbose, get_verbose, verbose_flags,
	    verbose_flags_cnt, int);

	/* ==================== Lock Configuration ===================== */

	/* Test DB_ENV->set_lk_conflicts(), DB_ENV->get_lk_conflicts(). */
	lk_set_nmodes = 2;
	CuAssert(ct, "dbenv->set_lk_conflicts", dbenv->set_lk_conflicts(dbenv,
	    lk_set_conflicts, lk_set_nmodes) == 0);
	CuAssert(ct, "dbenv->get_lk_conflicts", dbenv->get_lk_conflicts(dbenv,
	    &lk_get_conflicts, &lk_get_nmodes) == 0);
	CuAssert(ct, "check lock conflicts", memcmp(lk_set_conflicts,
	    lk_get_conflicts, sizeof(lk_set_conflicts)) == 0);
	CuAssert(ct, "check lock nomdes", lk_set_nmodes == lk_get_nmodes);

	/* DB_ENV->set_lk_detect(), DB_ENV->get_lk_detect(). */
	CHECK_1_DIGIT_VALUES(dbenv, set_lk_detect, get_lk_detect, u_int32_t,
	    lk_detect_values);

	/* Test DB_ENV->set_lk_max_lockers(), DB_ENV->get_lk_max_lockers(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lk_max_lockers, get_lk_max_lockers,
	    u_int32_t, rand());

	/* Test DB_ENV->set_lk_max_locks(), DB_ENV->get_lk_max_locks(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lk_max_locks, get_lk_max_locks,
	    u_int32_t, rand());

	/* Test DB_ENV->set_lk_max_objects(), DB_ENV->get_lk_max_objects(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lk_max_objects, get_lk_max_objects,
	    u_int32_t, rand());

	/* Test DB_ENV->set_lk_partitions(), DB_ENV->get_lk_partitions(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lk_partitions, get_lk_partitions,
	    u_int32_t, rand());

	/* Test DB_ENV->set_lk_tablesize(), DB_ENV->get_lk_tablesize(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lk_tablesize, get_lk_tablesize,
	    u_int32_t, rand());

	/* ==================== Log Configuration ===================== */

	/*
	 * Test DB_ENV->log_set_config(), DB_ENV->log_get_config().
	 * Direct I/O setting can only be performed if os supports it.
	 */
	if (__os_support_direct_io()) {
		CHECK_ONOFF(dbenv, log_set_config, log_get_config,
		    log_configs, log_configs_cnt, int);
	} else {
		CHECK_ONOFF(dbenv, log_set_config, log_get_config,
		    &log_configs[1], log_configs_cnt - 1, int);
	}

	/* Test DB_ENV->set_lg_bsize(), DB_ENV->get_lg_bsize(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lg_bsize, get_lg_bsize,
	    u_int32_t, rand());
	
	/* Test DB_ENV->set_lg_dir(), DB_ENV->get_lg_dir(). */
	CHECK_1_STR_VALUE(dbenv, set_lg_dir, get_lg_dir, "/logdir");

	/* Test DB_ENV->set_lg_filemode(), DB_ENV->get_lg_filemode(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lg_filemode, get_lg_filemode,
	    int, 0640);

	/* Test DB_ENV->set_lg_max(), DB_ENV->get_lg_max(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_lg_max, get_lg_max, u_int32_t, rand());

	/*
	 * Test DB_ENV->set_lg_regionmax(), DB_ENV->get_lg_regionmax().
	 * The value must be bigger than LG_BASE_REGION_SIZE(130000).
	 */
	CHECK_1_DIGIT_VALUE(dbenv, set_lg_regionmax, get_lg_regionmax,
	    u_int32_t, 12345678);

	/* ==================== Mpool Configuration ===================== */

	/*
	 * Test DB_ENV->set_cache_max(), DB_ENV->get_cache_max().
	 * The values could be ajusted, so we use specific values to avoid
	 * adjustment.
	 */
	CHECK_2_DIGIT_VALUES(dbenv, set_cache_max, get_cache_max,
	    u_int32_t, 3, u_int32_t, 131072);

	/*
	 * Test DB_ENV->set_cachesize() and DB_ENV->get_cachesize().
	 * The values could be ajusted, so we use specific values to avoid
	 * adjustment.
	 */
	CHECK_3_DIGIT_VALUES(dbenv, set_cachesize, get_cachesize,
	    u_int32_t, 3, u_int32_t, 1048576, int, 5);

	/* Test DB_ENV->set_mp_max_openfd(), DB_ENV->get_mp_max_openfd(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_mp_max_openfd, get_mp_max_openfd,
	    int, rand());

	/* Test DB_ENV->set_mp_max_write(), DB_ENV->get_mp_max_write(). */
	CHECK_2_DIGIT_VALUES(dbenv, set_mp_max_write, get_mp_max_write,
	    int, rand(), db_timeout_t , rand());

	/* Test DB_ENV->set_mp_mmapsize(), DB_ENV->get_mp_mmapsize(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_mp_mmapsize, get_mp_mmapsize,
	    size_t, rand());

	/* Test DB_ENV->set_mp_mtxcount(), DB_ENV->get_mp_mtxcount(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_mp_mtxcount, get_mp_mtxcount,
	    u_int32_t, rand());

	/*
	 * Test DB_ENV->set_mp_pagesize(), DB_ENV->get_mp_pagesize().
	 * The pagesize should be between 512 and 65536 and be power of two.
	 */
	CHECK_1_DIGIT_VALUE(dbenv, set_mp_pagesize, get_mp_pagesize,
	    u_int32_t, 65536);

	/* Test DB_ENV->set_mp_tablesize(), DB_ENV->get_mp_tablesize(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_mp_tablesize, get_mp_tablesize,
	    u_int32_t, rand());

	/* ==================== Mutex Configuration ===================== */

	/*
	 * Test DB_ENV->mutex_set_align(), DB_ENV->mutex_get_align().
	 * The mutex align value should be power of two.
	 */
	CHECK_1_DIGIT_VALUE(dbenv, mutex_set_align, mutex_get_align,
	    u_int32_t, 32);
	
	/* Test DB_ENV->mutex_set_increment(), DB_ENV->mutex_get_increment(). */
	CHECK_1_DIGIT_VALUE(dbenv, mutex_set_increment, mutex_get_increment,
	    u_int32_t, rand());

	/*
	 * Test DB_ENV->mutex_set_init(), DB_ENV->mutex_get_init().
	 * We should make sure the init value is not bigger than max, otherwise,
	 * the returned max value will not be correct.
	 */
	CHECK_1_DIGIT_VALUE(dbenv, mutex_set_init, mutex_get_init,
	    u_int32_t, 131072);

	/* Test DB_ENV->mutex_set_max(), DB_ENV->mutex_get_max(). */
	CHECK_1_DIGIT_VALUE(dbenv, mutex_set_max, mutex_get_max,
	    u_int32_t, 503276);

	/*
	 * Test DB_ENV->mutex_set_tas_spins(), DB_ENV->mutex_get_tas_spins().
	 * The value should be between 1 and 1000000 .
	 */
	CHECK_1_DIGIT_VALUE(dbenv, mutex_set_tas_spins, mutex_get_tas_spins,
	    u_int32_t, 1234);

	/* =================== Replication Configuration ==================== */

	/*
	 * Test DB_ENV->rep_set_clockskew(), DB_ENV->rep_get_clockskew().
	 * The fast_clock should be bigger than slow_clock.
	 */
	CHECK_2_DIGIT_VALUES(dbenv, rep_set_clockskew, rep_get_clockskew,
	    u_int32_t, 12345, u_int32_t, 11111);

	/* Test DB_ENV->rep_set_config(), DB_ENV->rep_get_config(). */
	CHECK_ONOFF(dbenv, rep_set_config, rep_get_config, rep_configs,
	    rep_configs_cnt - 2, int);

	/*
	 * Test DB_ENV->rep_set_limit(), DB_ENV->rep_get_limit().
	 * We use specific values to avoid adjustment.
	 */
	CHECK_2_DIGIT_VALUES(dbenv, rep_set_limit, rep_get_limit,
	    u_int32_t, 2, u_int32_t, 2345678);

	/* Test DB_ENV->rep_set_nsites(), DB_ENV->rep_get_nsites(). */
	CHECK_1_DIGIT_VALUE(dbenv, rep_set_nsites, rep_get_nsites,
	    u_int32_t, rand());

	/* Test DB_ENV->rep_set_priority(), DB_ENV->rep_get_priority(). */
	CHECK_1_DIGIT_VALUE(dbenv, rep_set_priority, rep_get_priority,
	    u_int32_t, rand());

	/*
	 * Test DB_ENV->rep_set_request(), DB_ENV->rep_get_request().
	 * The max should be bigger than min.
	 */
	CHECK_2_DIGIT_VALUES(dbenv, rep_set_request, rep_get_request,
	    u_int32_t, 100001, u_int32_t, 1234567);

	/* Test DB_ENV->rep_set_timeout(), DB_ENV->rep_get_timeout(). */
	CHECK_1_DIGIT_CONFIG_VALUES(dbenv, rep_set_timeout, rep_get_timeout,
	    rep_timeouts, u_int32_t);

	/* Test DB_ENV->set_tx_max(), DB_ENV->get_tx_max(). */
	CHECK_1_DIGIT_VALUE(dbenv, set_tx_max, get_tx_max, u_int32_t, rand());

	/*
	 * Test DB_ENV->set_tx_timestamp(), DB_ENV->get_tx_timestamp().
	 * We specify the timestamp to be one hour ago.
	 */
	tx_set_timestamp = time(NULL);
	tx_set_timestamp -= 3600;
	CuAssert(ct, "dbenv->set_tx_timestamp",
	    dbenv->set_tx_timestamp(dbenv, &tx_set_timestamp) == 0);
	CuAssert(ct, "dbenv->get_tx_timestamp",
	    dbenv->get_tx_timestamp(dbenv, &tx_get_timestamp) == 0);
	CuAssert(ct, "check tx timestamp",
	    tx_set_timestamp == tx_get_timestamp);

	CuAssert(ct, "dbenv->close", close_dbenv_handle(dbenv) == 0);

	/*
	 * The follwoing configurations are only valid for environment
	 * using replication manager API.
	 */
	CuAssert(ct, "db_env_create", create_dbenv_handle(&repmgr_dbenv) == 0);

	/* Test DB_ENV->rep_set_config(), DB_ENV->rep_get_config() */
	CHECK_ONOFF(repmgr_dbenv, rep_set_config, rep_get_config,
	    rep_configs + rep_configs_cnt - 3, 2, int);

	/* Test DB_ENV->rep_set_timeout(), DB_ENV->rep_get_timeout() */
	CHECK_1_DIGIT_CONFIG_VALUES(repmgr_dbenv, rep_set_timeout,
	    rep_get_timeout, repmgr_timeouts, u_int32_t);

	CuAssert(ct, "repmgr_dbenv->close",
	    close_dbenv_handle(repmgr_dbenv) == 0);

	return (0);
}

int TestDbPreOpenSetterAndGetter(CuTest *ct) {
	DB_ENV *dbenv;
	DB *db, *env_db, *hash_db, *heap_db, *queue_db, *recno_db;
	const char **part_dirs;
	u_int32_t encrypt_flags, heap_set_bytes, heap_set_gbytes,
	    heap_get_bytes, heap_get_gbytes;
	int nowait, onoff;
	
	/* ================ General and Btree Configuration =============== */

	CuAssert(ct, "db_create", create_db_handle(&db, NULL) == 0);

	/*
	 * Test DB->set_cachesize(), DB->get_cachesize().
	 * We use specific values to avoid adjustment.
	 */
	CHECK_3_DIGIT_VALUES(db, set_cachesize, get_cachesize,
	    u_int32_t, 3, u_int32_t, 1048576, int, 5);

	/* Test DB->set_encrypt(), DB->get_encrypt_flags(). */
	CuAssert(ct, "db->set_encrypt",
	    db->set_encrypt(db, passwd, DB_ENCRYPT_AES) == 0);
	CuAssert(ct, "db->get_encrypt_flags",
	    db->get_encrypt_flags(db, &encrypt_flags) == 0);
	CuAssert(ct, "check encrypt flags", encrypt_flags == DB_ENCRYPT_AES);

	/* Test DB->set_errfile(), DB->get_errfile(). */
	CHECK_1_PTR_VALUE_VOID(db, set_errfile, get_errfile, FILE, errfile);

	/* Test DB->set_errpfx(), DB->get_errpfx().*/
	CHECK_1_STR_VALUE_VOID(db, set_errpfx, get_errpfx, "dbp1");

	/* Test DB->set_flags(), DB->get_flags(). */
	CHECK_FLAG_VALUE(db, set_flags, get_flags,
	    u_int32_t, DB_CHKSUM | DB_RECNUM | DB_REVSPLITOFF);

	/* Test DB->set_lk_exclusive(), DB->get_lk_exclusive(). */
	CuAssert(ct, "db->set_lk_exclusive", db->set_lk_exclusive(db, 1) == 0);
	CuAssert(ct, "db->get_lk_exclusive",
	    db->get_lk_exclusive(db, &onoff, &nowait) == 0);
	CuAssert(ct, "check lk_exclusive onoff", onoff == 1);
	CuAssert(ct, "check lk_exclusive nowait", nowait == 1);

	/*
	 * Test DB->set_lorder(), DB->get_lorder().
	 * The only acceptable values are 1234 and 4321.
	 */
	CHECK_1_DIGIT_VALUE(db, set_lorder, get_lorder, int, 1234);
	CHECK_1_DIGIT_VALUE(db, set_lorder, get_lorder, int, 4321);

	/* Test DB->set_msgfile(), DB->get_msgfile(). */
	CHECK_1_PTR_VALUE_VOID(db, set_msgfile, get_msgfile, FILE, msgfile);

	/*
	 * Test DB->set_pagesize(), DB->get_pagesize().
	 * The pagesize should be 512-55536, and be power of two.
	 */
	CHECK_1_DIGIT_VALUE(db, set_pagesize, get_pagesize, u_int32_t, 512);
	CHECK_1_DIGIT_VALUE(db, set_pagesize, get_pagesize, u_int32_t, 65536);

	/*
	 * Test DB->set_bt_minkey(), DB->get_bt_minkey().
	 * The minkey value should be 2 at least.
	 */
	CHECK_1_DIGIT_VALUE(db, set_bt_minkey, get_bt_minkey, u_int32_t, 17);

	CuAssert(ct, "db->close", close_db_handle(db) == 0);

	/* =================== Recno-only Configuration ===================== */

	CuAssert(ct, "db_create", create_db_handle(&recno_db, NULL) == 0);

	/* Test DB->set_flags(), DB->get_flags(). */
	CHECK_FLAG_VALUE(recno_db, set_flags, get_flags,
	    u_int32_t, DB_RENUMBER | DB_SNAPSHOT);

	/* Test DB->set_re_delim(), DB->get_re_delim(). */
	CHECK_1_DIGIT_VALUE(recno_db, set_re_delim, get_re_delim,
	    int, rand());

	/* Test DB->set_re_len(), DB->get_re_len(). */
	CHECK_1_DIGIT_VALUE(recno_db, set_re_len, get_re_len,
	    u_int32_t, rand());

	/* Test DB->set_re_pad(), DB->get_re_pad(). */
	CHECK_1_DIGIT_VALUE(recno_db, set_re_pad, get_re_pad, int, rand());

	/* Test DB->set_re_source(), DB->get_re_source(). */
	CHECK_1_STR_VALUE(recno_db, set_re_source, get_re_source, "re_source1");

	CuAssert(ct, "recno_db->close", close_db_handle(recno_db) == 0);

	/* ==================== Hash-only Configuration ===================== */

	CuAssert(ct, "db_create", create_db_handle(&hash_db, NULL) == 0);

	/* Test DB->set_flags(), DB->get_flags(). */
	CHECK_FLAG_VALUE(hash_db, set_flags, get_flags,
	    u_int32_t, DB_DUP | DB_DUPSORT | DB_REVSPLITOFF);

	/* Test DB->set_h_ffactor(), DB->get_h_ffactor(). */
	CHECK_1_DIGIT_VALUE(hash_db, set_h_ffactor, get_h_ffactor,
	    u_int32_t, rand());

	/* Test DB->set_h_nelem(), DB->get_h_nelem(). */
	CHECK_1_DIGIT_VALUE(hash_db, set_h_nelem, get_h_nelem,
	    u_int32_t, rand());

	CuAssert(ct, "hash_db->close", close_db_handle(hash_db) == 0);

	/* =================== Queue-only Configuration ===================== */

	CuAssert(ct, "db_create", create_db_handle(&queue_db, NULL) == 0);

	/* Test DB->set_flags(), DB->get_flags(). */
	CHECK_FLAG_VALUE(queue_db, set_flags, get_flags, u_int32_t, DB_INORDER);

	/* Test DB->set_q_extentsize(), DB->get_q_extentsize(). */
	CHECK_1_DIGIT_VALUE(queue_db, set_q_extentsize, get_q_extentsize,
	    u_int32_t, rand());

	CuAssert(ct, "queue_db->close", close_db_handle(queue_db) == 0);

	/* ==================== Heap-only Configuration ===================== */
	CuAssert(ct, "db_create", create_db_handle(&heap_db, NULL) == 0);

	/* Test DB->set_heapsize(), DB->get_heapsize(). */
	heap_set_gbytes = 3;
	heap_set_bytes = 1048576;
	heap_get_gbytes = heap_get_bytes = 0;
	CuAssert(ct, "DB->set_heapsize", heap_db->set_heapsize(heap_db,
	    heap_set_gbytes, heap_set_bytes, 0) == 0);
	CuAssert(ct, "DB->get_heapsize", heap_db->get_heapsize(heap_db,
	    &heap_get_gbytes, &heap_get_bytes) == 0);
	CuAssert(ct, "Check heap gbytes", heap_set_gbytes == heap_get_gbytes);
	CuAssert(ct, "Check heap bytes", heap_set_bytes == heap_get_bytes);

	/* Test DB->set_heap_regionsize(), DB->get_heap_regionsize(). */
	CHECK_1_DIGIT_VALUE(heap_db, set_heap_regionsize, get_heap_regionsize,
	    u_int32_t, rand());

	CuAssert(ct, "heap_db->close", close_db_handle(heap_db) == 0);

	/*
	 * The following configurations require the database
	 * be opened in an environment.
	 */
	CuAssert(ct, "db_env_create", create_dbenv_handle(&dbenv) == 0);
	CuAssert(ct, "dbenv->set_flags(DB_ENCRYPT)", dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES) == 0);	
	CuAssert(ct, "add_dirs_to_dbenv",
	    add_dirs_to_dbenv(dbenv, data_dirs) == 0);
	CuAssert(ct, "dbenv->open", dbenv->open(dbenv, TEST_ENV,
	    DB_CREATE | DB_INIT_MPOOL | DB_INIT_TXN, 0644) == 0);
	CuAssert(ct, "db_create", create_db_handle(&env_db, dbenv) == 0);

	/* Test DB->set_flags(), DB->get_flags(). */
	CHECK_FLAG_VALUE(env_db, set_flags, get_flags,
	    u_int32_t, DB_ENCRYPT | DB_TXN_NOT_DURABLE);

	/* Test DB->set_create_dir(), DB->get_create_dir(). */
	CHECK_1_STR_VALUE(env_db, set_create_dir, get_create_dir, data_dirs[0]);

	/* Test DB->set_partition_dirs(), DB->get_partition_dirs(). */
	CuAssert(ct, "env_db->set_partition_dirs",
	    env_db->set_partition_dirs(env_db, &data_dirs[1]) == 0);
	CuAssert(ct, "env_db->get_partition_dirs",
	    env_db->get_partition_dirs(env_db, &part_dirs) == 0);
	CuAssert(ct, "cmp_dirs", cmp_dirs(&data_dirs[1], part_dirs) == 0);

	CuAssert(ct, "env_db->close", close_db_handle(env_db) == 0);
	CuAssert(ct, "dbenv->close", close_dbenv_handle(dbenv) == 0);

	return (0);
}

int TestMpoolFilePreOpenSetterAndGetter(CuTest *ct) {
	DB_ENV *dbenv;
	DB_MPOOLFILE *mpf;
	u_int8_t get_fileid[DB_FILE_ID_LEN], set_fileid[DB_FILE_ID_LEN];
	DB_CACHE_PRIORITY cache_priorities[] = {
		DB_PRIORITY_VERY_LOW,
		DB_PRIORITY_LOW,
		DB_PRIORITY_DEFAULT,
		DB_PRIORITY_HIGH,
		DB_PRIORITY_VERY_HIGH
	};
	u_int32_t mpool_flags;
	size_t len;
	DBT pgcookie_get, pgcookie_set;
	
	CuAssert(ct, "db_env_create", create_dbenv_handle(&dbenv) == 0);
	CuAssert(ct, "dbenv->open", dbenv->open(dbenv, TEST_ENV,
	    DB_CREATE | DB_INIT_MPOOL, 0644) == 0);
	CuAssert(ct, "dbenv->memp_fcreate", create_mp_handle(&mpf, dbenv) == 0);

	/* Test DB_MPOOLFILE->set_clear_len(), DB_MPOOLFILE->get_clear_len(). */
	CHECK_1_DIGIT_VALUE(mpf, set_clear_len, get_clear_len,
	    u_int32_t, rand());

	/* Test DB_MPOOLFILE->set_fileid(), DB_MPOOLFILE->get_fileid(). */
	len = sizeof(DB_ENV) > DB_FILE_ID_LEN ? DB_FILE_ID_LEN : sizeof(DB_ENV);
	memset(get_fileid, 0, DB_FILE_ID_LEN);
	memcpy(set_fileid, dbenv, len);
	CuAssert(ct, "mpf->set_fileid", mpf->set_fileid(mpf, set_fileid) == 0);
	CuAssert(ct, "mpf->get_fileid", mpf->get_fileid(mpf, get_fileid) == 0);
	CuAssert(ct, "check fileid", memcmp(set_fileid, get_fileid, len) == 0);

	/* Test DB_MPOOLFILE->set_flags(), DB_MPOOLFILE->get_flags(). */
	mpool_flags = 0;
	CuAssert(ct, "mpf->set_flags",
	    mpf->set_flags(mpf, DB_MPOOL_NOFILE, 1) == 0);
	CuAssert(ct, "mpf->set_flags",
	    mpf->set_flags(mpf, DB_MPOOL_UNLINK, 1) == 0);
	CuAssert(ct, "mpf->get_flags",
	    mpf->get_flags(mpf, &mpool_flags) == 0);
	CuAssert(ct, "check flags",
	    mpool_flags == (DB_MPOOL_NOFILE | DB_MPOOL_UNLINK));
	CuAssert(ct, "mpf->set_flags",
	    mpf->set_flags(mpf, DB_MPOOL_NOFILE, 0) == 0);
	CuAssert(ct, "mpf->set_flags",
	    mpf->set_flags(mpf, DB_MPOOL_UNLINK, 0) == 0);
	CuAssert(ct, "mpf->get_flags", mpf->get_flags(mpf, &mpool_flags) == 0);
	CuAssert(ct, "check flags", mpool_flags == 0);

	/* Test DB_MPOOLFILE->set_ftype(), DB_MPOOLFILE->get_ftype(). */
	CHECK_1_DIGIT_VALUE(mpf, set_ftype, get_ftype, int, rand());

	/*
	 * Test DB_MPOOLFILE->set_lsn_offset(),
	 * DB_MPOOLFILE->get_lsn_offset().
	 */
	CHECK_1_DIGIT_VALUE(mpf, set_lsn_offset, get_lsn_offset,
	    int32_t, rand());

	/*
	 * Test DB_MPOOLFILE->set_maxsize(), DB_MPOOLFILE->get_maxsize().
	 * We use specific values to avoid adjustment.
	 */
	CHECK_2_DIGIT_VALUES(mpf, set_maxsize, get_maxsize,
	    u_int32_t, 2, u_int32_t, 1048576);

	/* Test DB_MPOOLFILE->set_pgcookie(), DB_MPOOLFILE->get_pgcookie(). */
	memset(&pgcookie_set, 0, sizeof(DBT));
	memset(&pgcookie_get, 0, sizeof(DBT));
	pgcookie_set.data = set_fileid;
	pgcookie_set.size = DB_FILE_ID_LEN;
	CuAssert(ct, "mpf->set_pgcookie",
	    mpf->set_pgcookie(mpf, &pgcookie_set) == 0);
	CuAssert(ct, "mpf->get_pgcookie",
	    mpf->get_pgcookie(mpf, &pgcookie_get) == 0);
	CuAssert(ct, "check pgcookie size",
	    pgcookie_get.size == pgcookie_set.size);
	CuAssert(ct, "check pgcookie data", memcmp(pgcookie_get.data,
	    pgcookie_set.data, pgcookie_set.size) == 0);

	/* Test DB_MPOOLFILE->set_priority(), DB_MPOOLFILE->get_priority(). */
	CHECK_1_DIGIT_VALUES(mpf, set_priority, get_priority, DB_CACHE_PRIORITY,
	    cache_priorities);

	CuAssert(ct, "mpf->close", close_mp_handle(mpf) == 0);
	CuAssert(ct, "dbenv->close", close_dbenv_handle(dbenv) == 0);
	return (0);
}

int TestSequencePreOpenSetterAndGetter(CuTest *ct) {
	DB_ENV *dbenv;
	DB *dbp;
	DB_SEQUENCE *seq;
	u_int32_t seq_flags;

	CuAssert(ct, "db_env_create", create_dbenv_handle(&dbenv) == 0);
	CuAssert(ct, "dbenv->open", dbenv->open(dbenv,
	    TEST_ENV, DB_CREATE | DB_INIT_MPOOL, 0644) == 0);
	CuAssert(ct, "db_create", create_db_handle(&dbp, dbenv) == 0);
	CuAssert(ct, "dbp->open", dbp->open(dbp,
	    NULL, "seq.db", NULL, DB_BTREE, DB_CREATE, 0644) == 0);
	CuAssert(ct, "db_sequence_create",
	    create_seq_handle(&seq, dbp) == 0);

	/* Test DB_SEQUENCE->set_cachesize(), DB_SEQUENCE->get_cachesize(). */
	CHECK_1_DIGIT_VALUE(seq, set_cachesize, get_cachesize,
	    u_int32_t, rand());

	/* Test DB_SEQUENCE->set_flags(), DB_SEQUENCE->get_flags(). */
	seq_flags = 0;
	CHECK_1_DIGIT_VALUE(seq, set_flags, get_flags,
	    u_int32_t, DB_SEQ_DEC | DB_SEQ_WRAP);
	/* We make sure the DB_SEQ_DEC is cleared if we set DB_SEQ_INC. */
	CuAssert(ct, "seq->set_flags", seq->set_flags(seq, DB_SEQ_INC) == 0);
	CuAssert(ct, "seq->get_flags", seq->get_flags(seq, &seq_flags) == 0);
	CuAssert(ct, "check seq flags",
	    seq_flags == (DB_SEQ_INC | DB_SEQ_WRAP));

	/*
	 * Test DB_SEQUENCE->set_range(), DB_SEQUENCE->get_range().
	 * The max should be bigger than min.
	 */
	CHECK_2_DIGIT_VALUES(seq, set_range, get_range,
	    db_seq_t, 2, db_seq_t, 1048576);
	
	CuAssert(ct, "seq->close", close_seq_handle(seq) == 0);
	CuAssert(ct, "dbp->close", close_db_handle(dbp) == 0);
	CuAssert(ct, "dbenv->close", close_dbenv_handle(dbenv) == 0);

	return (0);
}

static int create_dbenv_handle(DB_ENV **dbenvpp) {
	int ret;
	if ((ret = db_env_create(dbenvpp, 0)) == 0)
		info.dbenvp = *dbenvpp;
	return ret;
}

static int close_dbenv_handle(DB_ENV *dbenvp) {
	info.dbenvp = NULL;
	return dbenvp->close(dbenvp, 0);
}

static int create_db_handle(DB **dbpp, DB_ENV *dbenvp) {
	int ret;
	if ((ret = db_create(dbpp, dbenvp, 0)) == 0)
		info.dbp = *dbpp;
	return ret;
}

static int close_db_handle(DB *dbp) {
	info.dbp = NULL;
	return dbp->close(dbp, 0);
}

static int create_mp_handle(DB_MPOOLFILE **mpp, DB_ENV *dbenv) {
	int ret;
	if ((ret = dbenv->memp_fcreate(dbenv, mpp, 0)) == 0)
		info.mp = *mpp;
	return ret;
}

static int close_mp_handle(DB_MPOOLFILE *mp) {
	info.mp = NULL;
	return mp->close(mp, 0);
}

static int create_seq_handle(DB_SEQUENCE **seqpp, DB *dbp) {
	int ret;
	if ((ret = db_sequence_create(seqpp, dbp, 0)) == 0)
		info.seqp = *seqpp;
	return ret;
}

static int close_seq_handle(DB_SEQUENCE *seqp) {
	info.seqp = NULL;
	return seqp->close(seqp, 0);
}

static int add_dirs_to_dbenv(DB_ENV *dbenv, const char **dirs) {
	int ret;
	const char *dir;

	if (dirs == NULL)
		return (0);

	ret = 0;
	while (ret == 0 && (dir = *dirs++) != NULL)
		ret = dbenv->add_data_dir(dbenv, dir);
	return ret;
}

/*
 * Compare the directory list reprensented by dirs1 and dirs2.
 * Both dirs1 and dirs2 use NULL pointer as terminator.
 */
static int cmp_dirs(const char **dirs1, const char **dirs2) {
	int ret;
	const char *dir1, *dir2;

	if (dirs1 == NULL || *dirs1 == NULL) {
		if (dirs2 == NULL || *dirs2 == NULL)
			return (0);
		else
			return (-1);
	} else if (dirs2 == NULL || *dirs2 == NULL)
		return (1);

	ret = 0;
	while (ret == 0) {
		dir1 = *dirs1++;
		dir2 = *dirs2++;
		if (dir1 == NULL || dir2 == NULL)
			break;
		ret = strcmp(dir1, dir2);
	}
	if (ret == 0) {
		if (dir1 != NULL)
			ret = 1;
		else if (dir2 != NULL)
			ret = -1;
	}
	
	return ret;
}

