/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * Multi-threaded servers that insert data into databases 1 and 2 at the
 * request of the client, and can die at the request of the client.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <atmi.h>

#include <tx.h>
#include <xa.h>

#include <db.h>

#include "../utilities/bdb_xa_util.h"

/*
 * The two servers are largely identical, #ifdef the source code.
 */
#ifdef SERVER1
#define	TXN_FUNC		TestThread1
#define	TXN_STRING		"TestThread1"
#endif
#ifdef SERVER2
#define	TXN_FUNC		TestThread2
#define	TXN_STRING		"TestThread2"
#endif
void TXN_FUNC(TPSVCINFO *);

#define	HOME	"../data"
#define NUMDB	2

int cnt_request;				/* Total requests. */

char *progname;					/* Server run-time name. */

/* Called once when the server is started. Creates and opens 2 databases. */
int
tpsvrinit(int argc, char* argv[])
{
	progname = argv[0];
	return (init_xa_server(NUMDB, progname, 0));
}

/* Called once when the servers are shutdown.  Closes the databases. */
void
tpsvrdone()
{
	close_xa_server(NUMDB, progname);
}
/* 
 * Called by the client to insert data into the databases.  Also can kill this
 * thread if commanded to by the client.
 */
void
TXN_FUNC(TPSVCINFO *msg)
{
  	int ret, i, commit, key_value, data_value;
	DBT key, data;

	memset(&key, 0, sizeof key);
	memset(&data, 0, sizeof data);
	commit = 1;
	++cnt_request;

#ifdef SERVER1
	key_value = data_value = cnt_request + 1;
#else
	key_value = data_value = (rand() % 1000) + 1;
#endif
	data.data = &data_value;
	data.size = sizeof(data_value);
	key.data = &key_value;
	key.size = sizeof(key_value);

	/* Kill the server to see what happens. */
	if (msg->data != NULL) {
	  	pthread_exit(NULL);
	}

	/* Insert data into the tables. */
	if (verbose) {
		printf("put: key: n");
		printf("pu1: data:\n");
	}
	/* Insert data into the tables. */
	for (i = 0; i < NUMDB; i++) {
		if ((ret = dbs[i]->put(dbs[i], NULL, &key, 
		    &data, 0)) != 0) {
			if (ret == DB_LOCK_DEADLOCK)
				goto abort;
			fprintf(stderr, "%s: %s: %s->put: %s\n",
			    progname, TXN_STRING, db_names[i], 
			    db_strerror(ret));
			goto err;
		}
	}

	/* Returns a commit or abort command to the client. */
	if (verbose)
		printf("%s: %s: commit\n", progname, TXN_STRING);
	tpreturn(TPSUCCESS, 0L, 0, 0L, 0);
	if (0) {
abort:		if (verbose)
			printf("%s: %s: abort\n", progname, TXN_STRING);
		tpreturn(TPSUCCESS, 1L, 0, 0L, 0);
	}
	return;

err:	tpreturn(TPFAIL, 1L, 0, 0L, 0);
}

