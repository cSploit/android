/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This is the multithreaded test for XA.  The client creates several threads
 * and uses each thread to send requests to the servers, which are also
 * multithreaded.  There are two tests.  The first one runs the client with
 * two threads that sends requests to the servers then exit.  In the second
 * test the client creates 3 threads.  The first 2 execute the same as in
 * the first test, but the third thread calls the servers with a command
 * to kill that server.  This is done to test that the environment and
 * servers can recover from a thread failure.
 */
#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <tx.h>
#include <atmi.h>
#include <fml32.h>
#include <fml1632.h>

#include <db.h>

#include "../utilities/bdb_xa_util.h"

#define	HOME	"../data"
#define	TABLE1	"../data/table1.db"
#define	TABLE2	"../data/table2.db"
#define NUM_SERVERS 3

static int expected_error = 0;

char *progname;					/* Client run-time name. */

int
usage()
{
	fprintf(stderr, "usage: %s [-v] [-k]\n", progname);
	return (EXIT_FAILURE);
}

static int thread_error = 1; 

/*
 * This function is called by the client threads.  Threads 1 and 2 randomly 
 * call each of the servers.  If thread 3 is created it calls one of the
 * servers and orders it to exit.
 */
void *
call_server_thread(void *server_name)
{
	FBFR *replyBuf;
	long replyLen;
	char *server_names[NUM_SERVERS];
	char *name, *thread_name, *kill_thread;
	int commit, j, iterations;
	void *result = NULL;
	TPINIT *initBuf = NULL;
	kill_thread = NULL;
	iterations = 100;
	replyBuf = NULL;

	/* Names of the function to call in the servers. */
	server_names[0] = "TestThread1";
	server_names[1] = "TestThread2";
	thread_name = (char *)server_name;
	
	/* Allocate init buffer */
	if ((initBuf = (TPINIT *)tpalloc("TPINIT", NULL, TPINITNEED(0))) == 0)
		goto tuxedo_err;
	initBuf->flags = TPMULTICONTEXTS;

	if (tpinit(initBuf) == -1)
		goto tuxedo_err;
	if (verbose)
	        printf("%s:%s: tpinit() OK\n", progname, thread_name);

	/* Create the command to kill the server. */
	if (strcmp(thread_name, "3") == 0) {
		kill_thread = (char *)tpalloc("STRING", NULL, 1);
		if (kill_thread == NULL)
		  	goto tuxedo_err;
		iterations = 1;
	} else if (expected_error)
		sleep(30);   

	for (j = 0; j < iterations; j++) {
	  	commit = 1;
		if (replyBuf != NULL)
			tpfree((char *)replyBuf);

		/* Randomly select a server. */
		name = server_names[j % 2];

		/* Allocate reply buffer. */
		replyLen = 1024;
		replyBuf = NULL;
		if ((replyBuf = (FBFR*)tpalloc("FML32", NULL, replyLen)) 
		    == NULL) 
			goto tuxedo_err;
		if (verbose)
		        printf("%s:%s: tpalloc(\"FML32\"), reply buffer OK\n", 
			    progname, thread_name);

		/* Begin the XA transaction. */
		if (tpbegin(60L, 0L) == -1)
			goto tuxedo_err;
		if (verbose)
		        printf("%s:%s: tpbegin() OK\n", progname, thread_name);
		/* Call the server to kill it. */
		if (kill_thread != NULL) {
		  	tpcall(name, kill_thread,  1L, (char **)&replyBuf, 
			    &replyLen, 0L);
			goto abort;
		} else {
		        if (tpcall(name, NULL, 0L, (char **)&replyBuf, 
			    &replyLen, TPSIGRSTRT) == -1) 
			  
			        /* 
				 * When one of the servers is killed TPNOENT or 
				 * TPESVCERR is an expected error.
				 */
			  if (expected_error && (tperrno == TPESVCERR || tperrno == TPENOENT || tperrno == TPETIME)) 
			                goto abort;
				else
			                goto tuxedo_err;
		}

		/* 
		 * Commit or abort the transaction depending the what the 
		 * server returns. 
		 */
		commit = !tpurcode;
		if (commit) {
commit:			if (verbose) {
			        printf("%s:%s: txn success\n", progname, 
				    thread_name);
			}
			if (tpcommit(0L) == -1) {
			  	if (expected_error && tperrno == TPETIME) 
			      	  	continue;
			  	else if (tperrno == TPEABORT)
			  	  	continue;
				else
				    	goto tuxedo_err;
			}
			if (verbose) {
				printf("%s:%s: tpcommit() OK\n", progname, 
				    thread_name);
			}
		} else {
abort:			if (verbose) {
				printf("%s:%s: txn failure\n", progname, 
				    thread_name);
			}
		  	if (tpabort(0L) == -1) {
			  	if (expected_error && tperrno == TPETIME) 
			    		continue;
			  	else
			  		goto tuxedo_err;
		  	}
			if (verbose) {
				printf("%s:%s: tpabort() OK\n", progname, 
				    thread_name);
			}
			if (strcmp(thread_name, "3") == 0) 
			  	break;
		}
	}

	if (0) {
tuxedo_err:	fprintf(stderr, "%s:%s: TUXEDO ERROR: %s (code %d)\n",
		    progname, thread_name, tpstrerror(tperrno), tperrno);
	  	result = (void *)&thread_error;
	}
end:	tpterm();
	if (verbose)
		printf("%s:%s: tpterm() OK\n", progname, thread_name);	

	if (replyBuf != NULL)
		tpfree((char *)replyBuf);
	if (initBuf != NULL)
		tpfree((char *)initBuf);
	if(kill_thread != NULL)
		tpfree((char *)kill_thread);	  

	return(result);
}

/*
 * Create the threads to call the servers, and check that data in the two
 * databases is identical.
 */
int
main(int argc, char* argv[])
{
  int ch, i, ret, num_threads;
	pthread_t threads[NUM_SERVERS];
	void *results = NULL;
	char *names[NUM_SERVERS];
	DB_ENV *dbenv;
	u_int32_t flags = DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN |
	  DB_INIT_LOCK | DB_CREATE | DB_THREAD | DB_RECOVER | DB_REGISTER;

	names[0] = "1";
	names[1] = "2";
	names[2] = "3";
	progname = argv[0];
	num_threads = 2;
	dbenv = NULL;

	while ((ch = getopt(argc, argv, "n:vk")) != EOF)
		switch (ch) {
		case 'k':
		  	num_threads = 3;
			expected_error = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (verbose)
		printf("%s: called\n", progname);

	
	/* Create threads for different contexts*/
	for (i = 0; i < num_threads; i++) {
		if (verbose)
		       printf("calling server thread\n");
		ret = pthread_create(&threads[i], NULL, 
		    call_server_thread, names[i]);
		if (ret) {
		       fprintf(stderr, "%s: failed to create thread %s.\n",
			    progname, ret);
		       goto err;
		}
        }

	/* Wait for each thread to finish. */
	for (i = 0; i < num_threads; i++) {
		if ((ret = pthread_join(threads[i], &results)) != 0) {
			fprintf(stderr, "%s: failed to join thread %s.\n",
			    progname, ret);
			goto err;
		}
		if (results != NULL)
		 	goto err; 
	}	

	/* If the kill thread was not used, check the data in the two tables.*/
	if (num_threads < NUM_SERVERS) {
	    /* Join the DB environment. */
		if ((ret = db_env_create(&dbenv, 0)) != 0 ||
		    (ret = dbenv->open(dbenv, HOME, flags, 0)) != 0) {
			fprintf(stderr,
			    "%s: %s: %s\n", progname, HOME, db_strerror(ret));
			goto err;
		}
		ret = check_data(dbenv, TABLE1, dbenv, TABLE2, progname);
	}

	if (0) {
err:		ret = EXIT_FAILURE;
	}

	if (dbenv != NULL)
		dbenv->close(dbenv, 0);
	return (ret);
}

