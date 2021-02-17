/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * Test setting and getting callbacks on the DB_ENV or DB handle. [#21553]
 *
 * It tests the callback setters/getters. These setters/getters are
 * divided into the following two sets:
 *   a. The callback setters and getters on DB_ENV handle.
 *   b. The callback setters and getters on DB handle.
 * The general flow for each callback setting/getting test is:
 *   1. Create the handle.
 *   2. Set the callback on the handle.
 *   3. Get the callback and verify it.
 *   4. Issue the open call on the handle.
 *   5. Get the callback again and verify it.
 *   6. Close the handle.
 * The callbacks we provide do not guarantee to work, but they guarantee
 * the handle can issue a call to open successfully.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

/*
 * The callbacks for DB_ENV handle.
 * Some of the callbacks are shared by DB handle as well, and there will be
 * comments for them. The order follows:
 * https://sleepycat.oracle.com/support.web/doc_builds/newdocs.db/api_reference/C/env.html
 * so that checking code is easier.
 */
/* For DB_ENV->get_alloc & DB->get_alloc */
typedef void *(*app_malloc_fcn)(size_t);
typedef void *(*app_realloc_fcn)(void *, size_t);
typedef void (*app_free_fcn)(void *);
/* For DB_ENV->get_app_dispatch */
typedef int (*tx_recover_fcn)(DB_ENV *dbenv, DBT *log_rec,
    DB_LSN *lsn, db_recops op);
/* For DB_ENV->get_backup_callbacks */
typedef int (*open_func)(DB_ENV *, const char *dbname,
    const char *target, void **handle);
typedef int (*write_func)(DB_ENV *, u_int32_t offset_gbytes,
    u_int32_t offset_bytes, u_int32_t size, u_int8_t *buf, void *handle);
typedef int (*close_func)(DB_ENV *, const char *dbname, void *handle);
/* For DB_ENV->get_errcall & DB->get_errcall */
typedef void (*db_errcall_fcn)(const DB_ENV *dbenv,
    const char *errpfx, const char *msg);
/* For DB_ENV->get_feedback */
typedef void (*dbenv_feedback_fcn)(DB_ENV *dbenv, int opcode, int percent);
/* For DB_ENV->get_isalive */
typedef int (*is_alive_fcn)(DB_ENV *dbenv, pid_t pid,
    db_threadid_t tid, u_int32_t flags);
/* For DB_ENV->get_msgcall & DB->get_msgcall */
typedef void (*db_msgcall_fcn)(const DB_ENV *dbenv, const char *msg);
/* For DB_ENV->get_thread_id_fn */
typedef void (*thread_id_fcn)(DB_ENV *dbenv, pid_t *pid, db_threadid_t *tid);
/* For DB_ENV->get_thread_id_string_fn */
typedef char *(*thread_id_string_fcn)(DB_ENV *dbenv, pid_t pid,
    db_threadid_t tid, char *buf);

/*
 * The callbacks for DB handle.
 * If the DB handle shares a callback with DB_ENV handle, it will not be
 * listed here, since it has been listed above. The order follows:
 * https://sleepycat.oracle.com/support.web/doc_builds/newdocs.db/api_reference/C/db.html
 * so that checking code is easier.
 */
/* For DB->get_dup_compare */
typedef int (*dup_compare_fcn)(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp);
/* For DB->get_feedback */
typedef void (*db_feedback_fcn)(DB *dbp, int opcode, int percent);
/* For DB->get_partition_callback */
typedef u_int32_t (*db_partition_fcn) (DB *db, DBT *key);
/* For DB->get_append_recno */
typedef int (*db_append_recno_fcn)(DB *dbp, DBT *data, db_recno_t recno);
/* For DB->get_bt_compare */
typedef int (*bt_compare_fcn)(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp);
/* For DB->get_bt_compress */
typedef int (*bt_compress_fcn)(DB *db, const DBT *prevKey,
    const DBT *prevData, const DBT *key, const DBT *data, DBT *dest);
typedef int (*bt_decompress_fcn)(DB *db, const DBT *prevKey,
    const DBT *prevData, DBT *compressed, DBT *destKey, DBT *destData);
/* For DB->get_bt_prefix */
typedef size_t (*bt_prefix_fcn)(DB *, const DBT *dbt1, const DBT *dbt2);
/* For DB->get_h_compare */
typedef int (*h_compare_fcn)(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp);
/* For DB->get_h_hash */
typedef u_int32_t (*h_hash_fcn)(DB *dbp, const void *bytes, u_int32_t length);

/*
 * The order for declarations follows above, so that checking code is easier.
 * Their definitions follow the same order, testing order follows it as well.
 */
static void *t_malloc(size_t sz);
static void *t_realloc(void *addr, size_t sz);
static void t_free(void *addr);
static int t_app_dispatch(DB_ENV *dbenv,
    DBT *log_rec, DB_LSN *lsn, db_recops op);
static int t_open_func(DB_ENV *, const char *dbname,
    const char *target, void **handle);
static int t_write_func(DB_ENV *, u_int32_t offset_gbytes,
    u_int32_t offset_bytes, u_int32_t size, u_int8_t *buf, void *handle);
static int t_close_func(DB_ENV *, const char *dbname, void *handle);
static void t_errcall(const DB_ENV *dbenv,
    const char *errpfx, const char *msg);
static void t_dbenv_callback(DB_ENV *dbenv, int opcode, int percent);
static int t_is_alive(DB_ENV *dbenv,
    pid_t pid, db_threadid_t tid, u_int32_t flags);
static void t_msgcall(const DB_ENV *dbenv, const char *msg);
static void t_thread_id(DB_ENV *dbenv, pid_t *pid, db_threadid_t *tid);
static char *t_thread_id_string(DB_ENV *dbenv,
    pid_t pid, db_threadid_t tid, char *buf);
static int t_dup_compare(DB *db, const DBT *dbt1, const DBT *dbt2, size_t *locp);
static void t_db_feedback(DB *dbp, int opcode, int percent);
static u_int32_t t_db_partition(DB *db, DBT *key);
static int t_append_recno(DB *dbp, DBT *data, db_recno_t recno);
static int t_bt_compare(DB *db, const DBT *dbt1, const DBT *dbt2, size_t *locp) ;
static int t_compress(DB *db, const DBT *prevKey, const DBT *prevData,
    const DBT *key, const DBT *data, DBT *dest);
static int t_decompress(DB *db, const DBT *prevKey,const DBT *prevData,
    DBT *compressed, DBT *destKey, DBT *destData);
static size_t t_bt_prefix(DB *db, const DBT *dbt1, const DBT *dbt2);
static int t_h_compare(DB *db, const DBT *dbt1, const DBT *dbt2, size_t *locp);
static u_int32_t t_h_hash(DB *dbp, const void *bytes, u_int32_t length);

/*
 * Common head routine for functions setting one callback.
 */
#define TEST_FUNCTION_1ARG_HEAD(type)					\
	type func_rt = NULL

/*
 * Common pre-open routine for functions setting one callback.
 * We get the callback after setting, and check the callback.
 */
#define TEST_FUNCTION_1ARG_PREOPEN(handle, setter, getter, func)	\
	CuAssert(ct, #handle"->"#setter,				\
	    handle->setter(handle, func) == 0);				\
	CuAssert(ct, "preopen: "#handle"->"#getter, 			\
	    handle->getter(handle, &func_rt) == 0);			\
	CuAssert(ct, "preopen: check "#func, func == func_rt)

/*
 * Common post-open routine for functions setting one callback.
 * After object(DB_ENV/DB) open, we check if we still can get the callback
 * and check the callback. Also, we close the handle.
 */
#define TEST_FUNCTION_1ARG_POSTOPEN(handle, getter, func)		\
	CuAssert(ct, "postopen: "#handle"->"#getter,			\
	    handle->getter(handle, &func_rt) == 0);			\
	CuAssert(ct, "postopen: check "#func, func == func_rt);		\
	info.handle = NULL;						\
	CuAssert(ct, #handle"->close", handle->close(handle, 0) == 0)

/*
 * Like TEST_FUNCTION_1ARG_PREOPEN, but both setter and getter have no return.
 */
#define TEST_FUNCTION_1ARG_PREOPEN_VOID(handle, setter, getter, func)	\
	handle->setter(handle, func);					\
	handle->getter(handle, &func_rt);				\
	CuAssert(ct, "preopen: check "#func, func == func_rt)

/*
 * Like TEST_FUNCTION_1ARG_POSTOPEN, but both setter and getter have no return.
 */
#define TEST_FUNCTION_1ARG_POSTOPEN_VOID(handle, getter, func) 		\
	handle->getter(handle, &func_rt);				\
	CuAssert(ct, "postopen: check "#func, func == func_rt);		\
	info.handle = NULL;						\
	CuAssert(ct, #handle"->close", handle->close(handle, 0) == 0)

/*
 * Common head routine for functions setting two callbacks.
 */
#define TEST_FUNCTION_2ARG_HEAD(type1, type2)				\
	type1 func_rt1 = NULL;						\
	type2 func_rt2 = NULL

/*
 * Common pre-open routine for functions setting two callbacks.
 * We get the callbacks after setting, and check the callbacks.
 */
#define TEST_FUNCTION_2ARG_PREOPEN(handle, setter, getter, func1, func2)\
	CuAssert(ct, #handle"->"#setter,				\
	    handle->setter(handle, func1, func2) == 0);			\
	CuAssert(ct, "preopen: "#handle"->"#getter, 			\
	    handle->getter(handle, &func_rt1, &func_rt2) == 0);		\
	CuAssert(ct, "preopen: check "#func1, func1 == func_rt1);	\
	CuAssert(ct, "preopen: check "#func2, func2 == func_rt2)

/*
 * Common post-open routine for functions setting two callbacks.
 * After object(DB_ENV/DB) open, we check if we still can get the callbacks
 * and check the callbacks. Also, we close the handle.
 */
#define TEST_FUNCTION_2ARG_POSTOPEN(handle, getter, func1, func2) 	\
	CuAssert(ct, "postopen: "#handle"->"#getter, 			\
	    handle->getter(handle, &func_rt1, &func_rt2) == 0);		\
	CuAssert(ct, "postopen: check "#func1, func1 == func_rt1);	\
	CuAssert(ct, "postopen: check "#func2, func2 == func_rt2);	\
	info.handle = NULL;						\
	CuAssert(ct, #handle"->close", handle->close(handle, 0) == 0)

/*
 * Common head routine for functions setting three callbacks.
 */
#define TEST_FUNCTION_3ARG_HEAD(type1, type2, type3)			\
	type1 func_rt1 = NULL;						\
	type2 func_rt2 = NULL;						\
	type3 func_rt3 = NULL

/*
 * Common pre-open routine for functions setting three callback.
 * We get the callbacks after setting, and check the callbacks.
 */
#define TEST_FUNCTION_3ARG_PREOPEN(handle, setter, getter, func1, func2,\
    func3)								\
	CuAssert(ct, #handle"->"#setter, 				\
	    handle->setter(handle, func1, func2, func3) == 0);		\
	CuAssert(ct, "preopen: "#handle"->"#getter, handle->getter(	\
	    handle, &func_rt1, &func_rt2, &func_rt3) == 0);		\
	CuAssert(ct, "preopen: check "#func1, func1 == func_rt1); 	\
	CuAssert(ct, "preopen: check "#func2, func2 == func_rt2); 	\
	CuAssert(ct, "preopen: check "#func3, func3 == func_rt3)

/*
 * Common post-open routine for functions setting three callbacks.
 * After object(DB_ENV/DB) open, we check if we still can get the callbacks
 * and check the callbacks. Also, we close the handle.
 */
#define TEST_FUNCTION_3ARG_POSTOPEN(handle, getter, func1, func2, func3)\
	CuAssert(ct, "postopen: "#handle"->"#getter, handle->getter(	\
	    handle, &func_rt1, &func_rt2, &func_rt3) == 0); 		\
	CuAssert(ct, "postopen: check "#func1, func1 == func_rt1); 	\
	CuAssert(ct, "postopen: check "#func2, func2 == func_rt2); 	\
	CuAssert(ct, "postopen: check "#func3, func3 == func_rt3); 	\
	info.handle = NULL;						\
	CuAssert(ct, #handle"->close", handle->close(handle, 0) == 0)

/*
 * Test DB_ENV's functions setting one callback.
 */
#define TEST_ENV_FUNCTIONS_1ARG(setter, getter, type, func) do { 	\
	DB_ENV *dbenvp; 						\
	TEST_FUNCTION_1ARG_HEAD(type); 					\
	CuAssert(ct, "db_env_create", db_env_create(&dbenvp, 0) == 0); 	\
	info.dbenvp = dbenvp;						\
	TEST_FUNCTION_1ARG_PREOPEN(dbenvp, setter, getter, func);	\
	CuAssert(ct, "dbenvp->open", dbenvp->open(dbenvp, TEST_ENV, 	\
	    DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | 			\
	    DB_INIT_LOG | DB_INIT_TXN, 0644) == 0); 			\
	TEST_FUNCTION_1ARG_POSTOPEN(dbenvp, getter, func);		\
} while(0)

/*
 * Test DB_ENV's functions setting one callback, both setter and getter
 * have no return.
 */
#define TEST_ENV_FUNCTIONS_1ARG_VOID(setter, getter, type, func) do {	\
	DB_ENV *dbenvp; 						\
	TEST_FUNCTION_1ARG_HEAD(type);					\
	CuAssert(ct, "db_env_create", db_env_create(&dbenvp, 0) == 0); 	\
	info.dbenvp = dbenvp;						\
	TEST_FUNCTION_1ARG_PREOPEN_VOID(dbenvp, setter, getter, func);	\
	CuAssert(ct, "dbenvp->open", dbenvp->open(dbenvp, TEST_ENV, 	\
	    DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | 			\
	    DB_INIT_LOG | DB_INIT_TXN, 0644) == 0); 			\
	TEST_FUNCTION_1ARG_POSTOPEN_VOID(dbenvp, getter, func); 	\
} while(0)

/*
 * Test DB_ENV's functions setting two callbacks.
 */
#define TEST_ENV_FUNCTIONS_2ARG(setter, getter, type1, func1, type2, func2)\
	do {								\
	DB_ENV *dbenvp; 						\
	TEST_FUNCTION_2ARG_HEAD(type1, type2);				\
	CuAssert(ct, "db_env_create", db_env_create(&dbenvp, 0) == 0); 	\
	info.dbenvp = dbenvp;						\
	TEST_FUNCTION_2ARG_PREOPEN(dbenvp, setter, getter,		\
	    func1, func2);						\
	CuAssert(ct, "dbenvp->open", dbenvp->open(dbenvp, TEST_ENV, 	\
	    DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | 			\
	    DB_INIT_LOG | DB_INIT_TXN, 0644) == 0); 			\
	TEST_FUNCTION_2ARG_POSTOPEN(dbenvp, getter, func1, func2);	\
} while(0)

/*
 * Test DB_ENV's functions setting three callbacks.
 */
#define TEST_ENV_FUNCTIONS_3ARG(setter, getter, type1, func1, type2,	\
    func2, type3, func3) do { 						\
	DB_ENV *dbenvp; 						\
	TEST_FUNCTION_3ARG_HEAD(type1, type2, type3);			\
	CuAssert(ct, "db_env_create", db_env_create(&dbenvp, 0) == 0); 	\
	info.dbenvp = dbenvp;						\
	TEST_FUNCTION_3ARG_PREOPEN(dbenvp, setter, getter, func1, func2,\
	    func3);							\
	CuAssert(ct, "dbenvp->open", dbenvp->open(dbenvp, TEST_ENV, 	\
	    DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_LOG | 	\
	    DB_INIT_TXN, 0644) == 0); 					\
	TEST_FUNCTION_3ARG_POSTOPEN(dbenvp, getter, func1, func2, 	\
	    func3);							\
} while(0)

/*
 * Macro for opening database handle.
 */
#define TEST_DB_OPEN(dbtype) if (dbtype == DB_BTREE) { 			\
		CuAssert(ct, "dbp->set_flags(DB_DUPSORT)", 		\
		    dbp->set_flags(dbp, DB_DUPSORT) == 0); 		\
	} 								\
	sprintf(buf, "%s/%d.db", TEST_ENV, indx++); 			\
	CuAssert(ct, "dbp->open", dbp->open(dbp, NULL, buf, NULL, 	\
	    dbtype, DB_CREATE, 0644) == 0)

/*
 * Test DB's functions setting one callback.
 */
#define TEST_DB_FUNCTIONS_1ARG(setter, getter, dbtype, type, func) do {	\
	DB *dbp; 							\
	char buf[DB_MAXPATHLEN]; 					\
	TEST_FUNCTION_1ARG_HEAD(type);					\
	CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0); 	\
	info.dbp = dbp;							\
	TEST_FUNCTION_1ARG_PREOPEN(dbp, setter, getter, func);		\
	TEST_DB_OPEN(dbtype);						\
	TEST_FUNCTION_1ARG_POSTOPEN(dbp, getter, func);			\
} while(0)

/*
 * Test DB's functions setting one callback, both setter and getter
 * have no return.
 */
#define TEST_DB_FUNCTIONS_1ARG_VOID(setter, getter, dbtype, type, func)	\
	do { 								\
	DB *dbp; 							\
	char buf[DB_MAXPATHLEN]; 					\
	TEST_FUNCTION_1ARG_HEAD(type);				 	\
	CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0); 	\
	info.dbp = dbp;							\
	TEST_FUNCTION_1ARG_PREOPEN_VOID(dbp, setter, getter, func);	\
	TEST_DB_OPEN(dbtype); 						\
	TEST_FUNCTION_1ARG_POSTOPEN_VOID(dbp, getter, func);		\
} while(0)

/*
 * Test DB's functions setting two callbacks.
 */
#define TEST_DB_FUNCTIONS_2ARG(setter, getter, dbtype, 			\
    type1, func1, type2, func2) do { 					\
	DB *dbp; 							\
	char buf[DB_MAXPATHLEN]; 					\
	TEST_FUNCTION_2ARG_HEAD(type1, type2);				\
	CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0); 	\
	info.dbp = dbp;							\
	TEST_FUNCTION_2ARG_PREOPEN(dbp, setter, getter, func1, func2);	\
	TEST_DB_OPEN(dbtype); 						\
	TEST_FUNCTION_2ARG_POSTOPEN(dbp, getter, func1, func2);		\
} while(0)

/*
 * Test DB's functions setting three callbacks.
 */
#define TEST_DB_FUNCTIONS_3ARG(setter, getter, dbtype, type1, func1,	\
    type2, func2, type3, func3) do { 					\
	DB *dbp; 							\
	char buf[DB_MAXPATHLEN]; 					\
	TEST_FUNCTION_3ARG_HEAD(type1, type2, type3);			\
	CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0); 	\
	info.dbp = dbp;							\
	TEST_FUNCTION_3ARG_PREOPEN(dbp, setter, getter, func1, func2,	\
	    func3);							\
	TEST_DB_OPEN(dbtype); 						\
	TEST_FUNCTION_3ARG_POSTOPEN(dbp, getter, func1, func2, func3);	\
} while(0)
	

struct handlers {
	DB_ENV *dbenvp;
	DB *dbp;
};
static struct handlers info;
static u_int32_t nparts = 5;

int TestCallbackSetterAndGetterSuiteSetup(CuSuite *suite) {
	return (0);
}

int TestCallbackSetterAndGetterSuiteTeardown(CuSuite *suite) {
	return (0);
}

int TestCallbackSetterAndGetterTestSetup(CuTest *ct) {
	setup_envdir(TEST_ENV, 1);
	info.dbenvp = NULL;
	info.dbp = NULL;
	return (0);
}

int TestCallbackSetterAndGetterTestTeardown(CuTest *ct) {
	if (info.dbp != NULL)
		CuAssert(ct, "dbp->close",
		    info.dbp->close(info.dbp, 0) == 0);
	if (info.dbenvp != NULL)
		CuAssert(ct, "dbenvp->close",
		    info.dbenvp->close(info.dbenvp, 0) == 0);
	return (0);
}


int TestEnvCallbacks(CuTest *ct) {

	TEST_ENV_FUNCTIONS_3ARG(set_alloc, get_alloc, app_malloc_fcn,
	    t_malloc, app_realloc_fcn, t_realloc, app_free_fcn, t_free);
	TEST_ENV_FUNCTIONS_1ARG(set_app_dispatch, get_app_dispatch,
	    tx_recover_fcn, t_app_dispatch);
	TEST_ENV_FUNCTIONS_3ARG(set_backup_callbacks, get_backup_callbacks,
	    open_func, t_open_func, write_func, t_write_func, close_func,
	    t_close_func);
	TEST_ENV_FUNCTIONS_1ARG_VOID(set_errcall, get_errcall,
	    db_errcall_fcn, t_errcall);
	TEST_ENV_FUNCTIONS_1ARG(set_feedback, get_feedback,
	    dbenv_feedback_fcn, t_dbenv_callback);

	/*
	 * The DB_ENV->set_is_alive requires the thread area be created,
	 * so we call DB_ENV->set_thread_count to enable the creation
	 * during environment open.
	 */
	{
		DB_ENV *dbenvp;
		TEST_FUNCTION_1ARG_HEAD(is_alive_fcn);
		setup_envdir(TEST_ENV, 1);
		CuAssert(ct, "db_env_create", db_env_create(&dbenvp, 0) == 0);
		info.dbenvp = dbenvp;
		TEST_FUNCTION_1ARG_PREOPEN(dbenvp, set_isalive, get_isalive,
		    t_is_alive);
		CuAssert(ct, "dbenvp->set_thread_count",
		    dbenvp->set_thread_count(dbenvp, 50) == 0);
		CuAssert(ct, "dbenvp->open", dbenvp->open(dbenvp, TEST_ENV,
		    DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK |
		    DB_INIT_LOG | DB_INIT_TXN, 0644) == 0);
		TEST_FUNCTION_1ARG_POSTOPEN(dbenvp, get_isalive, t_is_alive);
		setup_envdir(TEST_ENV, 1);
	}

	TEST_ENV_FUNCTIONS_1ARG_VOID(set_msgcall, get_msgcall,
	    db_msgcall_fcn, t_msgcall);
	TEST_ENV_FUNCTIONS_1ARG(set_thread_id, get_thread_id_fn,
	    thread_id_fcn, t_thread_id);
	TEST_ENV_FUNCTIONS_1ARG(set_thread_id_string,
	    get_thread_id_string_fn, thread_id_string_fcn, t_thread_id_string);

	return (0);
}

int TestDbCallbacks(CuTest *ct) {
	int indx;

	indx = 0;
	TEST_DB_FUNCTIONS_3ARG(set_alloc, get_alloc, DB_BTREE, app_malloc_fcn,
	    t_malloc, app_realloc_fcn, t_realloc, app_free_fcn, t_free);
	TEST_DB_FUNCTIONS_1ARG(set_dup_compare, get_dup_compare, DB_BTREE,
	    dup_compare_fcn, t_dup_compare);
	TEST_DB_FUNCTIONS_1ARG_VOID(set_errcall, get_errcall, DB_BTREE,
	    db_errcall_fcn, t_errcall);
	TEST_DB_FUNCTIONS_1ARG(set_feedback, get_feedback, DB_BTREE,
	    db_feedback_fcn, t_db_feedback);
	TEST_DB_FUNCTIONS_1ARG_VOID(set_msgcall, get_msgcall, DB_BTREE,
	    db_msgcall_fcn, t_msgcall);

	/*
	 * Test DB->set_partition and DB->get_partition_callbacks.
	 * Like others, we do setting before DB->open, and do
	 * getting before and after DB->open.
	 */
	{
		DB *dbp;
		char buf[DB_MAXPATHLEN];
		u_int32_t nparts_rt;
		db_partition_fcn func_rt;

		nparts_rt = 0;
		func_rt = NULL;
		CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0);
		info.dbp = dbp;
		CuAssert(ct, "dbp->set_partition", dbp->set_partition(dbp,
		    nparts, NULL, t_db_partition) == 0);
		CuAssert(ct, "dbp->get_partition_callbacks",
		    dbp->get_partition_callback(dbp,
		    &nparts_rt, &func_rt) == 0);
		CuAssert(ct, "check nparts", nparts_rt == nparts);
		CuAssert(ct, "check partition callback",
		    func_rt == t_db_partition);
		sprintf(buf, "%s/%d.db", TEST_ENV, indx++);
		CuAssert(ct, "dbp->open", dbp->open(dbp, NULL, buf, NULL,
		    DB_BTREE, DB_CREATE, 0644) == 0);
		CuAssert(ct, "dbp->get_partition_callbacks",
		    dbp->get_partition_callback(dbp,
		    &nparts_rt, &func_rt) == 0);
		CuAssert(ct, "check nparts", nparts_rt == nparts);
		CuAssert(ct, "check partition callback",
		    func_rt == t_db_partition);
		info.dbp = NULL;
		CuAssert(ct, "dbp->close", dbp->close(dbp, 0) == 0);
	}

	TEST_DB_FUNCTIONS_1ARG(set_append_recno, get_append_recno, DB_RECNO,
	    db_append_recno_fcn, t_append_recno);
	TEST_DB_FUNCTIONS_1ARG(set_bt_compare, get_bt_compare, DB_BTREE,
	    bt_compare_fcn, t_bt_compare);
	TEST_DB_FUNCTIONS_2ARG(set_bt_compress, get_bt_compress, DB_BTREE,
	    bt_compress_fcn, t_compress, bt_decompress_fcn, t_decompress);

	/*
	 * DB->set_bt_prefix requires DB do not use the default comparision
	 * function, so we call DB->set_bt_compare to set the comparision
	 * callback first.
	 */
	{
		DB *dbp;
		char buf[DB_MAXPATHLEN];
		TEST_FUNCTION_1ARG_HEAD(bt_prefix_fcn);
		CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0);
		info.dbp = dbp;
		TEST_FUNCTION_1ARG_PREOPEN(dbp, set_bt_prefix, get_bt_prefix,
		    t_bt_prefix);
		CuAssert(ct, "dbp->set_bt_compare",
		    dbp->set_bt_compare(dbp, t_bt_compare) == 0);
		TEST_DB_OPEN(DB_BTREE);
		TEST_FUNCTION_1ARG_POSTOPEN(dbp, get_bt_prefix, t_bt_prefix);
	}

	TEST_DB_FUNCTIONS_1ARG(set_h_compare, get_h_compare, DB_HASH,
	    h_compare_fcn, t_h_compare);
	TEST_DB_FUNCTIONS_1ARG(set_h_hash, get_h_hash, DB_HASH,
	    h_hash_fcn, t_h_hash);

	return (0);
}

static void *t_malloc(size_t sz) {
	void *p;
	int ret;

	if ((ret = __os_malloc(NULL, sz, &p)) != 0)
		p = NULL;
	return p;
}

static void *t_realloc(void *addr, size_t sz) {
	void *p;
	int ret;

	p = addr;
	if ((ret = __os_realloc(NULL, sz, &p)) != 0)
		p = NULL;
	return p;
}

static void t_free(void *addr) {
	__os_free(NULL, addr);
}

static  int t_app_dispatch(DB_ENV *dbenv,
    DBT *log_rec, DB_LSN *lsn, db_recops op) {
	return 0;
}

static int t_open_func(DB_ENV *dbenv, const char *dbname,
    const char *target, void **handle) {
	return 0;
}

static int t_write_func(DB_ENV *dbenv, u_int32_t offset_gbytes,
    u_int32_t offset_bytes, u_int32_t size, u_int8_t *buf, void *handle) {
	return 0;
}

static int t_close_func(DB_ENV *dbenv, const char *dbname, void *handle) {
	return 0;
}

static void t_errcall(const DB_ENV *dbenv,
    const char *errpfx, const char *msg) {
	return;
}

static void t_dbenv_callback(DB_ENV *dbenv, int opcode, int percent) {
	return;
}

static int t_is_alive(DB_ENV *dbenv,
    pid_t pid, db_threadid_t tid, u_int32_t flags) {
	return 1;
}

static void t_msgcall(const DB_ENV *dbenv, const char *msg) {
	return;
}

static void t_thread_id(DB_ENV *dbenv, pid_t *pid, db_threadid_t *tid) {
	__os_id(dbenv, pid, tid);
}

static char *t_thread_id_string(DB_ENV *dbenv,
    pid_t pid, db_threadid_t tid, char *buf) {
	buf[0] = '\0';
	return buf;
}

static int t_dup_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp) {
	return t_bt_compare(db, dbt1, dbt2, locp);
}

static void t_db_feedback(DB *dbp, int opcode, int percent) {
	return;
}

static u_int32_t t_db_partition(DB *db, DBT *key) {
	return (key->size % nparts);
}

static int t_append_recno(DB *dbp, DBT *data, db_recno_t recno) {
	size_t sz;
	sz = sizeof(recno) > data->size ? data->size : sizeof(recno);
	memcpy(data->data, &recno, sz);
	return 0;
}

static int t_bt_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp) {
	u_int32_t len;
	int ret;

	locp = NULL;
	len = dbt1->size > dbt2->size ? dbt2->size : dbt1->size;
	if ((ret = memcmp(dbt1->data, dbt2->data, (size_t)len)) == 0) {
		if (dbt1->size != dbt2->size)
			ret = dbt1->size > dbt2->size ? 1 : -1;
	}
	return ret;
}

static int t_compress(DB *db, const DBT *prevKey, const DBT *prevData,
    const DBT *key, const DBT *data, DBT *dest) {
	return 0;
}

static int t_decompress(DB *db, const DBT *prevKey,const DBT *prevData,
    DBT *compressed, DBT *destKey, DBT *destData) {
	return 0;
}

static size_t t_bt_prefix(DB *db, const DBT *dbt1, const DBT *dbt2) {
	u_int32_t len;

	len = dbt1->size > dbt2->size ? dbt2->size : dbt1->size;
	if (dbt1->size != dbt2->size)
		len++;
	return (size_t)len;
}

static int t_h_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp) {
	return t_bt_compare(db, dbt1, dbt2, locp);
}

static u_int32_t t_h_hash(DB *dbp, const void *bytes, u_int32_t length) {
	return length;
}

