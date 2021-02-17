/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * Create dozens of writer threads to insert data in parallel.
 * DBSQL will not call the busy callback. It blocks instead.
 */

typedef struct {
	const char* db_name;        /* The filename of db. */
	int num_of_records;   /* The number of records to insert. */
	int thread_sn;              /* Serial number of thread. */
} thread_attr;

/*
 * Define the writer thread's workload.
 * The writer would insert 5000 records in its thread. Commit if succeeded 
 * and rollback if failed.
 */
static void*
writer(arg)
	void *arg;
{
	const char* sql;
	int txn_begin;
	int num_of_records;
	int thread_sn;
	int i, rc;
	sqlite3* db;
	sqlite3_stmt* stmt;

	txn_begin = 0; /* Mark that explicit txn does not begin yet. */

	/* Open database. */
	sqlite3_open(((thread_attr *)arg)->db_name, &db);
	error_handler(db);

	/* Fetch attributes. */
	num_of_records = ((thread_attr *)arg)->num_of_records;
	thread_sn = ((thread_attr *)arg)->thread_sn;

	/* Prepare the statement for use, many times over. */
	sql = "INSERT INTO university VALUES"
	      "(147, 'Tsinghua University China', 'tsinghua.edu.cn',"
	      "'cn', 'Asia', 237,63,432,303);";
	rc = sqlite3_prepare_v2(db, sql, (int)strlen(sql), &stmt, NULL);
	if (rc != SQLITE_OK)
		goto cleanup;

	/* 
	 * When we insert data many times over, we shall use explicit
	 * transaction to speed up the operations.
	 */
	rc = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, NULL);
	if (rc != SQLITE_OK)
		goto cleanup;
	txn_begin = 1; /* Mark that explicit txn began. */

	for (i = 0; i < num_of_records; i++) {
		rc = sqlite3_step(stmt);
		/*
		 * Even if we encounter errors, the statement still has 
		 * to be reset. Otherwise following rollback always 
		 * hits SQLITE_BUSY 
		 */
		sqlite3_reset(stmt);
		if (rc != SQLITE_DONE) {
			/* We can not return here. Rollback is required. */
			goto cleanup;
			break;
		}
	}

	/* Commit if no errors. */
	rc = sqlite3_exec(db, "COMMIT TRANSACTION", NULL, 0, NULL);
	if (rc != SQLITE_OK)
		goto cleanup;

cleanup:
	/* Error handle. */
	if (rc != SQLITE_OK && rc != SQLITE_DONE) {
		fprintf(stderr, "ERROR: %s. ERRCODE: %d.\n",
			sqlite3_errmsg(db), rc);
		/* Rollback if explict txn had begined. */
		if (txn_begin)
			sqlite3_exec(db, "ROLLBACK TRANSACTION", NULL, 0, NULL);
	}

	/* Final cleanup. */
	sqlite3_finalize(stmt);

	sqlite3_close(db);
	return NULL;
}

/* Example body. */
static int
ex_sql_multi_thread(db, db_name)
	db_handle *db;
	const char* db_name;
{
	const char* sql;
	int nthreads;
	int ninsert;
	int i;
	thread_attr attr;
	os_thread_t pid;

	nthreads = 20;
	ninsert  = 5000;

	/* Display current status. */
	echo_info("Check existing record number of the table");
	sql = "SELECT count(*) FROM university;"; 
	exec_sql(db, sql);

	/*
	 * Create n threads and write in parallel.
	 */
	echo_info("Now we begin to insert records by multi-writers.");
	attr.db_name = db_name;
	attr.num_of_records = ninsert;

	for (i = 0; i < nthreads; i++) {
		attr.thread_sn = i;
		if (os_thread_create(&pid, writer, (void *)&attr)) {
			register_thread_id(pid);
			printf("%02dth writer starts to write %d rows\n",
				i + 1, ninsert);
			sqlite3_sleep(20);	/* Milliseconds. */
		} else {
			fprintf(stderr, "Failed to create thread\n");
		}
	}
	join_threads();

	/* Display result. */
	echo_info("Check existing record number of the table");
	sql = "SELECT count(*) FROM university;"; 
	exec_sql(db, sql);

	return 0;
}

int
main()
{
	db_handle *db;
	const char* db_name = "ex_sql_multi_thread.db";

	/* Check if current lib is threadsafe. */
	if(!sqlite3_threadsafe()) {
		fprintf(stderr,
			"ERROR: The libsqlite version is NOT threadsafe!\n");
		exit(EXIT_FAILURE);
	}

	/* Setup environment and preload data. */
	db = setup(db_name);
	load_table_from_file(db, university_sample_data, 1/* Silent */);

	/* Run example. */
	ex_sql_multi_thread(db, db_name);

	/* End. */
	cleanup(db);
	return 0;
}

