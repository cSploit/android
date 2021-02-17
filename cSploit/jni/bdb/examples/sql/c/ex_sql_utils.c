/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This file contain the common utilities for sql examples. Includes:
 * 1. Setup/Clean up Enviornment.
 * 2. Output control.
 * 3. Common SQL executor and error handler.
 * 4. A simple multi-threaded manager.
 */

/* Setup database environment. */
db_handle *
setup(db_name)
	const char* db_name;
{
	db_handle *db;
	/* Open database. */
	sqlite3_open(db_name, &db);
	error_handler(db);

	return db;
}

/* Clean up the database environment. */
void
cleanup(db)
	db_handle *db;
{
	int rc;
	/* Close database and end program. */
	rc = sqlite3_close(db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "DB CLOSE ERROR. ERRCODE: %d.\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("DONE.\n");
}

/*
 * Output Control Functions.
 */
/* Output message with line between. */
void
echo_info(info)
	const char* info;
{
	int i;
	char ch = '=';

	for (i = 0; i < 80; i++)
		printf("%c", ch);
	
	printf("\n%s\n", info);
}

/*
 * Output columns for following exec_sql(). We've to use ANSI C declaration 
 * here to eliminate warnings in Visual Studio.
 */
static int
print_column_callback(void *data, int n_columns,
		      char **col_values, char **col_names)
{
	int i;

	printf("  "); /* Display indent. */
	for (i = 0; i < n_columns; i++) {
		printf("%s\t", 
		    col_values[i] == NULL ? "" : col_values[i]);
	}
	printf("\n");

	return 0;
}

/*
 * Execute a given sql expression and print result automatically. This
 * function will print error and exit if any error occurs.
 *
 * This function always return sqlite result code.
 */
static int
exec_sql_internal(db, sql, silent)
	db_handle* db;
	const char* sql;
	int silent;
{
	int rc; /* Result code. */

	if (!silent)
		printf("SQL: %s\n", sql);
	/*
	 * Execute a sql expression. The result will be printed by
	 * the callback function.
	 *
	 * The 5th argument of sqlite3_exec() is errmsg buffer. In out case, 
	 * the program does not use it and use sqlite3_errmsg() to get the most 
	 * recent error message in error_handler(). The advantage is that we 
	 * do not need to manage those errmsg buffers by hand. But if we share 
	 * a connection object(sqlite3*) in multi-threads, the error message 
	 * of one thread may be overwritten by other threads. However, sharing 
	 * a connection object between multi-threads is not a recommended 
	 * method in sqlite.
	 */
	if (!silent)
		rc = sqlite3_exec(db, sql, print_column_callback, 0, NULL);
	else
		rc = sqlite3_exec(db, sql, NULL, 0, NULL);

	error_handler(db);

	return rc;
}

int
exec_sql(db, sql)
	db_handle* db;
	const char* sql;
{
	return exec_sql_internal(db, sql, 0);
}

/*
 * This is the default error handler for all examples. It always return 
 * the recent sqlite result code.
 *
 * You have to use sqlite3_extended_result_codes() instead of sqlite3_errcode() 
 * when extended result codes are enabled. These examples does not use extended 
 * code.
 */
int
error_handler(db)
	db_handle *db;
{
	int err_code = sqlite3_errcode(db);

	switch(err_code) {
	case SQLITE_OK:
	case SQLITE_DONE:
	case SQLITE_ROW:
		/* Do nothing. */
		break;
	default:
		fprintf(stderr, "ERROR: %s. ERRCODE: %d.\n",
			sqlite3_errmsg(db), err_code);
		exit(err_code);
	}
	return err_code;
}



/*
 * Pre-load database, create a table and insert rows by given a csv file.
 */
#define BUF_SIZE		1024
#define TABLE_COLS		16
#define SQL_COMMAND_SIZE	4096

/*
 * Here is the definition of the university sample table. As we can see from 
 * below, the university table has 9 columns with types including int and 
 * varchar.
 */
const sample_data university_sample_data = {
	"university",
	"\tDROP TABLE IF EXISTS university;\n"
	"\tCREATE TABLE university\n"
	"\t(\n"
		"\t\trank       int,\n"
		"\t\tname       varchar(75),\n"
		"\t\tdomains    varchar(75),\n"
		"\t\tcountry    varchar(30),\n"
		"\t\tregion     varchar(30),\n"
		"\t\tsize       int,\n"
		"\t\tvisibility int,\n"
		"\t\trich       int,\n"
		"\t\tscholar    int\n"
	 "\t);",
	"../examples/sql/data/university.csv", 9};

/*
 * Here is the definition of the country sample table. As we can see from 
 * below, the country table has 6 columns with types including int and 
 * varchar.
 */
const sample_data country_sample_data = {
	"country",
	"\tDROP TABLE IF EXISTS country;\n"
	"\tCREATE TABLE country\n"
	"\t(\n"
		"\tcountry    varchar(30),\n"
		"\tabbr       varchar(10),\n"
		"\tTop_100    int,\n"
		"\tTop_200    int,\n"
		"\tTop_500    int,\n"
		"\tTop_1000   int\n"
	"\t);\n",
	"../examples/sql/data/country.csv", 6};

static char items[TABLE_COLS][BUF_SIZE];

/* Open the sample csv file handle. */
static FILE*
open_csv_file(data_source)
	const char* data_source;
{
	FILE* fp;

	fp = fopen(data_source, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s open error.", data_source);
		exit(EXIT_FAILURE);
	}

	return fp;
}

/* Get data line-by-line and insert it into the database. */
static int
iterate_csv_file(fp, n_cols)
		FILE* fp;
		int n_cols;
{
	static char file_line[BUF_SIZE * TABLE_COLS];
	const char delims[] = ",";
	char *result;
	int i;

	/* Skip header row. */
	if (ftell(fp) == 0) {
		fgets(file_line, sizeof(file_line), fp);
	}

	/* Get one line. */
	if (fgets(file_line, sizeof(file_line), fp) == NULL) {
		fclose(fp);
		return 0;
	}

	/* Token sentence by delimiters "," */
	result = strtok(file_line, delims);
	for (i = 0; result != NULL && i < n_cols; i++) {
		strcpy(items[i], result);
		result = strtok(NULL, delims);
	}

	return 1;
}

/* Common utility: load given csv file into database. */
void
load_table_from_file(db, data, silent)
	sqlite3* db;
	sample_data data;
	int silent;
{
	FILE *fp;
	int i, n;
	char buf[SQL_COMMAND_SIZE];

	sprintf(buf, "Load data source %s into database.",
		data.source_file);
	echo_info(buf);
	/* Create table by given SQL expression. */
	exec_sql_internal(db, data.sql_create, silent);

	fp = open_csv_file(data.source_file);
	while (iterate_csv_file(fp, data.ncolumn) != 0) {
		/* Get data line by line and put it into database. */
		i = sprintf(buf, "INSERT INTO %s VALUES(", data.table_name);
		if (data.ncolumn > 0 ) {
			for (n = 0; n < data.ncolumn; n++) {
				i += sprintf(buf + i, "'%s',", items[n]);
			}
			/* -3 to delete last "\n'," */
			sprintf(buf + i - 3, "');");
			exec_sql_internal(db, buf, silent);
		}
	}
	printf("Load done.\n");
}


/*
 * A very simple multi-threaded manager for concurrent examples.
 */
static os_thread_t thread_stack[MAX_THREAD];
static int pstack = 0;

/* All created thread ids will be pushed into stack for managing. */
void
register_thread_id(pid)
		os_thread_t pid;
{
	/* Push pid into stack. */
	if (pstack < MAX_THREAD) {
		thread_stack[pstack++] = pid;
	} else {
		fprintf(stderr, "Error: Too many threads!\n");
	}
}

/* Join for all threads in stack when finished. */
int
join_threads()
{ 
	int i;
	int status = 0;
#if defined(WIN32)
#else
	void *retp;
#endif

	for (i = 0; i < pstack ; i++) {
#if defined(WIN32)
		if (WaitForSingleObject(thread_stack[i], INFINITE) == WAIT_FAILED) {
			status = 1;
			printf("join_threads: child %ld exited with error %s\n",
			(long)i, strerror(GetLastError()));
		}
#else
		pthread_join(thread_stack[i], &retp);
		if (retp != NULL) {
			status = 1;
			printf("join_threads: child %ld exited with error\n",
			(long)i);
		}
#endif
	}

	pstack = 0;
	return status;
}

