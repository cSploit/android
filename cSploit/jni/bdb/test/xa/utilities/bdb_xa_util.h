/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include <pthread.h>
#include <db.h>

/* Maximum number of db handles that init/close_xa_server will handle. */
#define MAX_NUMDB	4

/* Names for the databases. */
static char *db_names[] = {"table1.db", "table2.db", "table3.db", "table4.db"};

/* Debugging output. */
#ifdef VERBOSE
static int verbose = 1;				
#else
static int verbose = 0;
#endif

/* Table handles. */				
DB *dbs[MAX_NUMDB]; 

/*
 * This function syncs all threads threads by putting them to
 * sleep until the last thread enters and sync function and
 * forces the rest to wake up.
 * counter - A counter starting at 0 shared by all threads
 * num_threads - the number of threads being synced.
 */
int sync_thr(int *counter, int num_threads, pthread_cond_t *cond_var);

/*
 * Print callback for __db_prdbt.
 */
int pr_callback(void *handle, const void *str_arg);
/*
 * Initialize an XA server and allocates and opens database handles.  
 * The number of database handles it opens is the value of the argument
 *  num_db.
 */
int init_xa_server(int num_db, const char *progname, int use_mvcc);

/* 
 * Called when the servers are shutdown.  This closes all open 
 * database handles. num_db is the number of open database handles.
 */
void close_xa_server(int num_db, const char *progname);

/*
 * check_data --
 *	Compare data between two databases to ensure that they are identical.
 */
int check_data(DB_ENV *dbenv1, const char *name1, DB_ENV *dbenv2, 
	       const char *name2, const char *progname);
