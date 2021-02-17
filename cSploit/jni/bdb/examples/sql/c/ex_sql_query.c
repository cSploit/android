/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example demonstrates how to execute queries.
 * Example 1. Single Select.
 * Example 2. Using 'WHERE' and 'ORDER BY'.
 * Example 3. Using 'GROUP BY'.
 * Example 4. Subquery.
 * Example 5. SQL function.
 */

/* Example body. */
static int
ex_sql_query(db)
	db_handle *db;
{
	const char* sql;

	/*
	 * Example 1: Single Select.
	 * -  Select rank and name from table university.
	 */
	echo_info("1. Single Select");
	sql = "\tSELECT rank, name from university;";
	exec_sql(db, sql);

	/*
	 * Example 2: Use 'WHERE' and 'ORDER BY' to arrange results
	 * - Select rank, name and country from table university where 
	 *   region is Europe. ORDER by country.
	 */
	echo_info("2. Using 'WHERE' and 'ORDER BY'");
	sql = "\tSELECT rank, country, name \n"
	      "\tFROM university \n"
	      "\tWHERE region = 'Europe' \n"
	      "\tORDER BY country;";
	exec_sql(db, sql);

	/*
	 * Example 3: Using 'Group by'.
	 */
	echo_info("3. Group by");
	sql = "\tSELECT region, count(*) from university \n"
	      "\tGROUP BY region \n"	
	      "\tORDER BY region";
	exec_sql(db, sql);

	/*
	 * Example 4 Subquery
	 * - Select rank, name and country from table university where 
	 *   country's fullname is 'USA'.
	 */
	echo_info("4. Subquery");
	sql = "\tSELECT rank, country, name from university \n"
	      "\tWHERE country = (SELECT abbr from country \n"
 				"\t\t\tWHERE country = 'USA');";
	exec_sql(db, sql);

	/*
	 * Example 5: SQL functions
	 * - Output current system date.
	 */
	echo_info("5. SQL functions: Echo current date");
	sql = "\tSELECT date('now');";
	exec_sql(db, sql);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment and preload data. */
	db = setup("./ex_sql_query.db");
	load_table_from_file(db, university_sample_data, 1/* Silent */);
	load_table_from_file(db, country_sample_data, 1/* Silent */);

	/* Run example. */
	ex_sql_query(db);

	/* End. */
	cleanup(db);
	return 0;
}

