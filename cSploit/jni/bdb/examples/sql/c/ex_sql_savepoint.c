/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * Transactions created using BEGIN...COMMIT do not nest. For nested
 * transactions, use the SAVEPOINT and RELEASE commands. The "TO SAVEPOINT
 * name" clause of the ROLLBACK command is only applicable to SAVEPOINT
 * transactions.
 *
 * This example shows how to begin/commit/rollback to a savepoint instead of 
 * operating whole transaction.
 */

/* Example body. */
static int
ex_sql_savepoint(db)
	db_handle *db;
{
	const char* disp_result;
	const char* info;
	disp_result = \
		"SELECT rank, region, name from university WHERE rank <= 3;";

	/*
	 * BEGIN a transaction for all following operations.
	 */
	echo_info("Example1. BEGIN and COMMIT a transaction explicitly");
	exec_sql(db, "BEGIN"); 

	/*
	 * Start to execute given sql expression. A savepoint will be started 
	 * in the beginning of each step. We can define multi-savepoints by 
	 * different names.
	 */
	info =  "1. Declare savepoint sp1. Delete the rank 1 record then "
		"release savepoint sp1. We can see the record has been "
		"removed.";
	echo_info(info);
	exec_sql(db, "SAVEPOINT sp1");
	exec_sql(db, disp_result);	/* Display status before deleting. */
	exec_sql(db, "DELETE FROM university WHERE rank = '1';");
	exec_sql(db, "RELEASE sp1");
	exec_sql(db, disp_result);	/* Display status after deleting. */

	info =  "2. Declare savepoint sp2. Then remove the rank 2 record and "
		"rollback. We can see the record was recovered by rollback to "
		"savepoint sp2.";
	echo_info(info);
	exec_sql(db, "SAVEPOINT sp2");
	exec_sql(db, "DELETE FROM university WHERE rank = '2';");
	exec_sql(db, disp_result);	/* Display status before rollback. */
	exec_sql(db, "ROLLBACK TO sp2");
	exec_sql(db, disp_result);	/* Display status after rollback. */

	/* Commit the whole transaction. */
	exec_sql(db, "COMMIT TRANSACTION"); 
	info = "3. Here is the final result. We can see only the savepoint "
	       "sp1 was released.";
	echo_info(info);
	exec_sql(db, disp_result);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_savepoint.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */);

	/* Run example. */
	ex_sql_savepoint(db);

	/* End. */
	cleanup(db);
	return 0;
}

