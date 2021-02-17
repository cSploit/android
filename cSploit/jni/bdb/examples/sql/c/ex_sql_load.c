/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example demonstrates how to create a table and add data to it.
 * Example 1. Create table university and insert data from data/university.csv.
 * Example 2. Create table country and insert data from data/country.csv.
 */

/* Example body. */
static int
ex_sql_load(db)
	db_handle *db;
{
	/* Create table university. */
	echo_info("STEP1. Create Table university");
	/* Load data from table data source. */
	load_table_from_file(db, university_sample_data, 0); 

	/* Create table contries. */
	echo_info("STEP2. Create Table country");
	/* Load data from table data source. */
	load_table_from_file(db, country_sample_data, 0); 

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment */
	db = setup("./ex_sql_load.db");

	/* Run example. */
	ex_sql_load(db);

	/* End. */
	cleanup(db);
	return 0;
}

