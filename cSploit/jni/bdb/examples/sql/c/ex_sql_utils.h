/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#ifndef __EX_SQL_UTILS_H
#define __EX_SQL_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

typedef sqlite3 db_handle;
/*
 * Common database environment setup/clean up. 
 */
db_handle* setup(const char* db_name);       /* Setup database environment. */
void cleanup(db_handle *db);          /* Clean up the database environment. */

/*
 * Common sql execution and error handler functions
 * error_handler():  Common error handler of examples. Report and exit if 
 * 		     any unexpected error occur.
 * exec_sql():       Execute a given sql expression, print the result 
 * 		     automatically and call above error_handler().
 * echo_info():      Echo the given message with line header.
 */
int error_handler(db_handle*);			  /* Default error handler. */
int exec_sql(db_handle*, const char*);		   /* Common sql execution. */
void echo_info(const char*);	   	 /* Echo message with Line Between. */

/*
 * Table source definition. Here we use university and country tables 
 * in these examples.
 */
typedef struct {
	const char* table_name;		      /* Table name of sample data. */
	const char* sql_create;	  /* SQL expression for creating the table. */
	const char* source_file;	  /* File name to sample data file. */
	int ncolumn;			          /* number of table column */
} sample_data;
extern const sample_data university_sample_data;/* Sample table, university. */
extern const sample_data country_sample_data;      /* Sample table, country. */

/* Load table from a given csv file. */
void load_table_from_file(sqlite3*, sample_data, int);

/*
 * A very simple multi-threaded manager that creates, joins, stops multiple 
 * threads that are used in the ex_sql_multi_thread example. This manager 
 * can works on both Windows and UNIX.
 */
#define MAX_THREAD 1024
#if !defined(HAVE_VXWORKS) && !defined(WIN32)
#include <pthread.h>
#endif

#ifdef WIN32
  #include <windows.h>
  typedef HANDLE          os_thread_t;
  #define os_thread_id()  GetCurrentThreadId()
  #define os_thread_create(pid, func, arg)                    \
       ((*pid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, \
       arg, 0, NULL)) != NULL)
  #define S_ISDIR(m) ((m) & _S_IFDIR)
#else
  typedef pthread_t       os_thread_t;
  #define os_thread_id()  pthread_self()
  #define os_thread_create(pid, func, arg)                    \
       (0 == pthread_create(pid, NULL, func, arg))
#endif

/* Program should register the thread id for following join_threads(). */
void register_thread_id(os_thread_t pid); 
/* Join all registered threads. */
int join_threads();

#endif

