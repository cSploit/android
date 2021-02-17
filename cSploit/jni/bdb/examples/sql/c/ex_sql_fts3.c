/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example demonstrates the usage of full-text search.
 * Example 1: Query by rowid. 
 * Example 2: Full-text Query.  
 * Example 3: Token prefix query. 
 * Example 4: Table column specified token query. 
 * Example 5: Phrase query.  
 * Example 6: Using operator. 
 * Example 7: NEAR query. 
 * Example 8: Using auxiliary functions. 
 */

const sample_data sms_sample_data = {
	"sms",
	"\tDROP TABLE IF EXISTS sms;\n"
	"\tCREATE VIRTUAL TABLE sms\n"
	"\tUSING fts3(\n"
		"\t\tnumber	varchar(10) NOT NULL,\n"
		"\t\tname	varchar(20),\n"
		"\t\tcontent	TEXT\n"
	 "\t, tokenize=simple);",
	"../examples/sql/data/sms.csv", 3};

/* Example body. */
static int
ex_sql_fts3(db)
	db_handle *db;
{
	const char* sql;

	/*
	 * Example 1: Query by rowid. 
	 */
	echo_info("Example 1: Query by rowid.");
	sql = "SELECT rowid, * FROM sms WHERE rowid=10;";
	exec_sql(db, sql);

	/*
	 * Example 2: Full-text Query. 
	 */
	echo_info("Example 2: Full-text Query.");
	sql = "SELECT * FROM sms WHERE name MATCH 'Aaron';";
	exec_sql(db, sql);

	/*
	 * Example 3: Token prefix query. 
	 */
	echo_info("Example 3: Token prefix query.");
	sql = "SELECT * FROM sms WHERE sms MATCH 'Jack*';";
	exec_sql(db, sql);

	/*
	 * Example 4: Table column specified token query.
	 */
	echo_info("Example 4: Table column specified token query.");
	sql = "SELECT * FROM sms WHERE content match 'name:Demitrius grey';";
	exec_sql(db, sql);

	/*
	 * Example 5: Phrase query. 
	 */
	echo_info("Example 5: Phrase query.");
	sql = "SELECT * FROM sms WHERE content MATCH '\"eag* beav*\"';";
	exec_sql(db, sql);

	/*
	 * Example 6: Using operator.
	 */
	echo_info("Example 6: Using operator.");
	sql = "SELECT * FROM sms WHERE sms MATCH 'Hedin AND english';";
	exec_sql(db, sql);

	/*
	 * Example 7: NEAR query. 
	 */
	echo_info("Example 7: NEAR query.");
	sql = "SELECT * FROM sms WHERE sms MATCH 'clear NEAR/2 air';";
	exec_sql(db, sql);

	/*
	 * Example 8: Using auxiliary functions.
	 */
	echo_info("Example 8: Using auxiliary functions.");
	sql = "SELECT snippet(sms) FROM sms WHERE sms MATCH 'Elymas';";
	exec_sql(db, sql);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_fts3.db");
	load_table_from_file(db, sms_sample_data, 1);

	/* Run example. */
	ex_sql_fts3(db);

	/* End. */
	cleanup(db);
	return 0;
}
