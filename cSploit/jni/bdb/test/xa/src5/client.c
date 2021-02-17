/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This is the MVCC test for XA.  It runs 3 tests, which are as follows:
 * 1. No MVCC
 * 2. MVCC enabled by DB_CONFIG
 * 3. MVCC enabled by flags
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
#define NUM_TESTS 4
#define NUM_THREADS 2
#define TIMEOUT 60

static int error = 1;

char *progname;					/* Client run-time name. */

int
usage()
{
	fprintf(stderr, "usage: %s [-v] -t[0|1|2]\n", progname);
	return (EXIT_FAILURE);
}

enum test_type{NO_MVCC, MVCC_DBCONFIG, MVCC_FLAG};

struct thread_args {
	char *thread_name;
	int type_test;
};

/* Used to sync the threads. */
static pthread_cond_t cond_vars[NUM_TESTS*3];
static int counters[NUM_TESTS*3];

void init_tests(char *ops[], int success[], int type, int thread_num);

/*
 * 
 */
void *
call_server_thread(void *arguments)
{
	char *thread_name, *ops[NUM_TESTS];
	int ttype;
	struct thread_args args;
	int commit, j, success[NUM_TESTS], thread_num, ret;
	void *result = NULL;
	TPINIT *initBuf = NULL;
	FBFR *replyBuf = NULL;
	long replyLen = 0;


	args = *((struct thread_args *)arguments);
	thread_name =  args.thread_name;
	thread_num = atoi(thread_name);
	ttype = args.type_test;
	init_tests(ops, success, ttype, thread_num);

	if (verbose)
		printf("%s:%s: starting thread %i\n", progname, thread_name,
		    thread_num);
	
	/* Allocate init buffer */
	if ((initBuf = (TPINIT *)tpalloc("TPINIT", NULL, TPINITNEED(0))) == 0)
		goto tuxedo_err;
	initBuf->flags = TPMULTICONTEXTS;

	if (tpinit(initBuf) == -1)
		goto tuxedo_err;
	if (verbose)
		printf("%s:%s: tpinit() OK\n", progname, thread_name);

	/* Allocate reply buffer. */
	replyLen = 1024;
	if ((replyBuf = (FBFR*)tpalloc("FML32", NULL, replyLen)) == NULL) 
		goto tuxedo_err;
	if (verbose)
		printf("%s:%s: tpalloc(\"FML32\"), reply buffer OK\n", 
		    progname, thread_name);

	for (j = 0; j < NUM_TESTS; j++) {
		commit = 1;

		/* Sync both threads. */
		if ((ret = sync_thr(&(counters[j*3]), NUM_THREADS,
		     &(cond_vars[j*3]))) != 0) {
			fprintf(stderr, 
			    "%s:%s: Error syncing threads: %i \n",
			    progname, thread_name, ret);
			goto end;
		}

		/* Begin the XA transaction. */
		if (tpbegin(TIMEOUT, 0L) == -1)
			goto tuxedo_err;
		if (verbose)
			printf("%s:%s: tpbegin() OK\n", progname, thread_name);

		/* Force thread 2 to wait till thread 1 does its operation.*/
		if (thread_num == 2) {
			if ((ret = sync_thr(&(counters[(j*3)+1]), NUM_THREADS, 
			    &(cond_vars[(j*3)+1]))) != 0) {
				fprintf(stderr, 
				    "%s:%s: Error syncing thread 1: %i \n",
				    progname, thread_name, ret);
				goto end;
			}
		}
		if (verbose)
			printf("%s:%s: calling server %s\n", progname, 
			    thread_name, ops[j]);

		/* Read or insert into the database. */
		if (tpcall(ops[j], NULL, 0L, (char **)&replyBuf, 
		    &replyLen, 0) == -1) 
			goto tuxedo_err;

		/* Wake up thread 2.*/
		if (thread_num == 1) {
			if ((ret = sync_thr(&(counters[(j*3)+1]), NUM_THREADS,
			     &(cond_vars[(j*3)+1]))) != 0) {
				fprintf(stderr, 
				    "%s:%s: Error syncing thread 1: %i \n",
				    progname, thread_name, ret);
				goto end;
			}
		}

		/* Sync both threads. */
		if ((ret = sync_thr(&(counters[(j*3)+2]), NUM_THREADS,
		    &(cond_vars[(j*3)+2]))) != 0) {
			fprintf(stderr, 
			    "%s:%s: Error syncing threads: %i \n",
			    progname, thread_name, ret);
			goto end;
		}

		/* 
		 * Commit or abort the transaction depending the what the 
		 * server returns. Check that it matched expectation
		 * (We abort on LOCK_NOTGRANTED and DEADLOCK errors, and
		 * commit otherwise.  Other errors result in returning
		 * without committing or aborting.
		 */
		commit = !tpurcode;
		if (commit != success[j]) {
			fprintf(stderr, 
			    "%s:%s: Expected: %i Got: %i.\n",
			    progname, thread_name, success[j], commit);
			if (verbose) {
				printf("%s:%s: Expected: %i Got: %i.\n",
				    progname, thread_name, success[j], commit);
			}
		}
			
		if (commit) {
			if (tpcommit(0L) == -1) 
				goto tuxedo_err;
			if (verbose) {
				printf("%s:%s: tpcommit() OK\n", progname, 
				    thread_name);
			}
		} else {
			if (tpabort(0L) == -1) {
				goto tuxedo_err;
			}
			if (verbose) {
				printf("%s:%s: tpabort() OK\n", progname, 
				    thread_name);
			}
		}
	}

	if (0) {
tuxedo_err:	fprintf(stderr, "%s:%s: TUXEDO ERROR: %s (code %d)\n",
		    progname, thread_name, tpstrerror(tperrno), tperrno);
		/* 
		 * Does not matter what result is, as long as it is not 
		 * NULL on error.
		 */
		result = (void *)&error;
	}
end:	tpterm();
	if (verbose)
		printf("%s:%s: tpterm() OK\n", progname, thread_name);

	if (initBuf != NULL)
		tpfree((char *)initBuf);

	if (replyBuf != NULL)
		tpfree((char *)replyBuf);

	return(result);
}

/*
 * Create the threads to call the servers, and check that data in the two
 * databases is identical.
 */
int
main(int argc, char* argv[])
{
	int ch, i, ret, ttype;
	pthread_t threads[NUM_THREADS];
	struct thread_args args[NUM_THREADS];
	void *results = NULL;
	char *names[] = {"1", "2"};

	progname = argv[0];
	i = 1;
	verbose = 0;

	while ((ch = getopt(argc, argv, "vt")) != EOF) {
		switch (ch) {
		case 't':
			ttype = atoi(argv[++i]);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
		i++;
	}
	argc -= optind;
	argv += optind;

	if (ttype > 2 || ttype < 0)
		return (usage());

	if (verbose)
		printf("%s: called with type %i\n", progname, ttype);

	for(i = 0; i < (NUM_TESTS*3); i++) {
		pthread_cond_init(&cond_vars[i], NULL);
		counters[i] = 0;
	}

	/* Create threads for different contexts*/
	for (i = 0; i < NUM_THREADS; i++) {
		args[i].thread_name = names[i];
		args[i].type_test = ttype;
		if (verbose)
			printf("calling server thread\n");
		if ((ret = pthread_create(&threads[i], NULL, 
		    call_server_thread, &(args[i]))) != 0) {
			fprintf(stderr, "%s: failed to create thread %s.\n",
			    progname, ret);
			goto err;
		}
	}

	/* Wait for each thread to finish. */
	for (i = 0; i < NUM_THREADS; i++) {
		if ((ret = pthread_join(threads[i], &results)) != 0) {
			fprintf(stderr, "%s: failed to join thread %s.\n",
			    progname, ret);
			goto err;
		}
		if (results != NULL)
			goto err; 
	}	

	if (0) {
err:		ret = EXIT_FAILURE;
	}

	for(i = 0; i < (NUM_TESTS*2); i++)
		pthread_cond_destroy(&cond_vars[i]);

	return (ret);
}

static char *rdb1 = "read_db1";
static char *wdb1 = "write_db1";

/*
 *	    Operation		    Success
 * Thread 1	Thread 2	no MVCC		MVCC
 * write    	write		no		no
 * write    	read		no		yes
 * read		read		yes		yes
 * read		write		no		yes
 */
void init_tests(char *ops[], int success[], int type, int thread_num) 
{
	if (thread_num == 1) {
		ops[0] = wdb1;
		ops[1] = wdb1;
		ops[2] = rdb1;
		ops[3] = rdb1;
		/* Thread 1 is always successful. */
		success[0] = 1;
		success[1] = 1;
		success[2] = 1;
		success[3] = 1;
	} else {
		ops[0] = wdb1;
		ops[1] = rdb1;
		ops[2] = rdb1;
		ops[3] = wdb1;
		if (type == NO_MVCC) {
			success[0] = 0;
			success[1] = 0;
			success[2] = 1;
			success[3] = 0;
		} else {
			success[0] = 0;
			success[1] = 1;
			success[2] = 1;
			success[3] = 1;
		}
	}
}
