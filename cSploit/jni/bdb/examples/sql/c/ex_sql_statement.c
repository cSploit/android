/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * Use sqlite3_prepare() and sqlite3_step() to query and iterate query result.
 *
 * To execute an SQL query, it must first be compiled into a byte-code program 
 * using one of routines sqlite3_prepare*(). The sqlite3_exec() interface is a 
 * wrapper around sqlite3_prepare_v2(), sqlite3_step(), and 
 * sqlite3_finalize(), so it's much more effective to use sqlite3_prepare() to 
 * prepare the statement for use many times over.
 *
 * The sqlite3_prepare_v2() and sqlite3_prepare16_v2() interfaces are 
 * recommended for all new programs. The two older interfaces are retained 
 * for backwards compatibility, but their use is discouraged. 
 *
 * You also can use sqlite3_get_table() to fetch result into memory data 
 * structure directly. But it is subject to the size of result set and 
 * available memory of system.
 */

/* Iterate query result. */
static void
iterate_query_result(db, stmt)
		db_handle *db;
		sqlite3_stmt* stmt;
{
	int i, k, ncols;
	int rc;

	for (i=0; ;i++) {
		rc = sqlite3_step(stmt);
		error_handler(db);

		if (rc == SQLITE_ROW) {
			ncols = sqlite3_column_count(stmt);
			for(k=0; k < ncols; k++) {
				printf("%s\t", sqlite3_column_text(stmt, k));
			}
			printf("\n");
		} else if (rc == SQLITE_DONE)
			break;
	}
}


/* Example body. */
static int
ex_sql_statment(db)
	db_handle *db;
{
	sqlite3_stmt* stmt;
	const char* sql;

	/* Prepare the statement for use, many times over. */
	sql = "SELECT rank, name FROM university;";
	echo_info(sql);
	sqlite3_prepare_v2(db, sql, (int)strlen(sql), &stmt, NULL);
	error_handler(db);

	echo_info("a. Iterate query result with prepared statement");
	iterate_query_result(db, stmt);

	/* We can reset the statment and use it again for many times. 
	 * The sqlite3_reset() function is called to reset a prepared 
	 * statement object back to its initial state, ready to be 
	 * re-executed. Any SQL statement variables that had values 
	 * bound to them using the sqlite3_bind_*() API retain their values.
	 */
	echo_info("b. Reset the statement and iterate query result again");
	sqlite3_reset(stmt);
	error_handler(db);
	iterate_query_result(db, stmt);

	/* Final cleanup. */
	sqlite3_finalize(stmt);
	error_handler(db);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_statement.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */);

	/* Run example. */
	ex_sql_statment(db);

	/* End. */
	cleanup(db);
	return 0;
}

