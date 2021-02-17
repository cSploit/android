/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * Transactions define boundaries around a group of SQL commands such that 
 * they either all successfully execute together or not at all. 
 *
 * SQLite automatically runs each command in its own transaction, and if the 
 * command does not fail, its changes are automatically committed. This mode 
 * of operation (implicit transactions) is referred to as autocommit mode.
 *
 * This example shows how to use explicit transaction to appoint transaction 
 * scopes. It can reduces transaction number(compare with autocommit mode) 
 * and improve system efficiency. 
 *
 * Example1: Commit a transaction explicitly.
 * Example2: Rollback a transaction explicitly.
 *
 * Transactions created using BEGIN...COMMIT do not nest. For nested
 * transactions, use the SAVEPOINT and RELEASE commands.
 *
 */

/* Example body. */
static int
ex_sql_transaction(db)
	db_handle *db;
{
	const char* disp_result;
	disp_result = "SELECT rank, region, name from university;";

	/*
	 * Example1: BEGIN and COMMIT a transaction explicitly.
	 */
	echo_info("Example1. BEGIN and COMMIT a transaction explicitly");
	exec_sql(db, "BEGIN TRANSACTION");
	exec_sql(db, "DELETE FROM university WHERE region = 'Asia';");
	exec_sql(db, "COMMIT TRANSACTION");
	exec_sql(db, disp_result);

	/*
	 * Example2: Rollback a transaction explicitly.
	 */
	echo_info("Example2. Rollback a transaction explicitly");
	exec_sql(db, "BEGIN TRANSACTION");
	exec_sql(db, "DELETE FROM university WHERE region = 'Europe';");
	exec_sql(db, disp_result);
	exec_sql(db, "ROLLBACK TRANSACTION");
	exec_sql(db, disp_result);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_transaction.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */);

	/* Run example. */
	ex_sql_transaction(db);

	/* End. */
	cleanup(db);
	return 0;
}

