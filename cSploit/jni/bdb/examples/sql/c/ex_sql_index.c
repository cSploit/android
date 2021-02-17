/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example demonstrates the use of indexes.
 * Step 1. Explain a query and output the query plan
 * Step 2. Create an index 
 * Step 3. Explain above query again to see how query plan is simplified 
 * Step 4. Drop the index 
 */

/* Example body. */
static int
ex_sql_index(db)
	db_handle *db;
{
	const char* sql_1; 
	const char* sql_2;

	/*
	 * Explain a query expression to get the query plan. 
	 */
	echo_info("STEP1. Explain a query expression");
	sql_1 = "\tEXPLAIN \n"
		"\tSELECT rank, country, name \n"
		"\tfrom university \n"
		"\tWHERE region = 'Europe' \n"
		"\tORDER BY country;";
	exec_sql(db, sql_1);

	/*
	 * Now create index.
	 */
	echo_info("STEP2. Create an index on university(region, country)");
	sql_2 = "\tCREATE INDEX university_geo \n"
		"\tON university (region, country);";
	exec_sql(db, sql_2);

	/*
	 * Now we check query plan again. We can see reasonable indexes
	 * can simplify query plan.
	 */
	echo_info("STEP3. Explain the query again, we can see the difference");
	exec_sql(db, sql_1);

	/*
	 * Drop the index.
	 */
	echo_info("STEP4. Drop the index");
	sql_2 = "DROP INDEX university_geo;";
	exec_sql(db, sql_2);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_index.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */); 

	/* Run example. */
	ex_sql_index(db);

	/* End. */
	cleanup(db);
	return 0;
}

