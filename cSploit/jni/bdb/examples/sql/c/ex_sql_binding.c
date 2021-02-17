/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example shows how to use prepare + binding + transaction to do bulk
 * insert.
 *
 * Here we insert 100,000 records using a prepared statement and binding.
 */

/* Example body. */
static int
ex_sql_binding(db)
	db_handle *db;
{
	const char *sql;
	int i, rc;
	char uname[64];
	int errflag;
	int num_of_records = 100000;
	sqlite3_stmt* stmt;

	/*
	 * Prepare the statement for insert many times over. The '?' in sql
	 * expression is the variable for binding.
	 */
	sql = "INSERT INTO university VALUES"
	      "(?, ?, 'xxxxx.edu.cn', 'cn', 'Asia', 999,99,999,999);";

	/*
	 * The sqlite3_prepare_v2() interfaces is recommended for all new 
	 * programs. It supports binding and is compatible with 
	 * sqlite3_prepare().
	 */
	sqlite3_prepare_v2(db, sql, (int)strlen(sql), &stmt, NULL);
	error_handler(db);

	/* 
	 * When we insert data many times over, we shall use explicit
	 * transaction to combine the operations so they execute faster. Also, 
	 * using sqlite3_prepare and sqlite3_bind* is a good choice.
	 */
	exec_sql(db, "BEGIN TRANSACTION");
	errflag = 0;
	for (i = 1; i <= num_of_records && !errflag; i++) {
		/*
		 * We can bind int, blob, int64, null, text, text16, value 
		 * and zero blob by API.
		 */
		/* i -> #1 variable */
		sqlite3_bind_int(stmt, 1, i);
		error_handler(db);

		/* uname -> #2 variable */
		sprintf(uname, "%d_university", i);
		sqlite3_bind_text(stmt, 2, uname, (int)strlen(uname), NULL);
		error_handler(db);

		/* Execute the query expression */
		sqlite3_step(stmt);

		/* Reset stmt when SQLITE_DONE. The sqlite3_reset() function 
		 * is called to reset a prepared statement object back to its 
		 * initial state, ready to be re-executed. Any SQL statement 
		 * variables that had values bound to them using the 
		 * sqlite3_bind_*() API retain their values.
		 */
		rc = sqlite3_errcode(db);
		switch(rc) {
		case SQLITE_DONE:
			sqlite3_reset(stmt);
			break;
		default:
			fprintf(stderr, "ERROR: %s. ERRCODE: %d\n",
				sqlite3_errmsg(db), rc);
			errflag = 1;
			break;
		}

		if (i%10000 == 0)
			printf("Inserted %d rows\n", i);
	}

	/* Commit if succeed and rollback if failed. */
	sql = (errflag) ? "ROLLBACK TRANSACTION" : "COMMIT TRANSACTION";
	exec_sql(db, sql);

	/* Final cleanup. */
	sqlite3_finalize(stmt);
	error_handler(db);

	/* Display result. */
	sql = "SELECT count(*) FROM university;"; 
	exec_sql(db, sql);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_binding.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */); 

	/* Run example. */
	ex_sql_binding(db);

	/* End. */
	cleanup(db);
	return 0;
}

