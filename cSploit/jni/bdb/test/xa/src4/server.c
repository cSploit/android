/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This code is for servers used in test 4.  Server 1 inserts
 * into database 1 and Server 2 inserts into database 2, then
 * sleeps for 30 seconds to cause a timeout error in the client.
 */
#include <sys/types.h>

#include "../utilities/bdb_xa_util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <atmi.h>
#include <fml1632.h>
#include <fml32.h>
#include <tx.h>
#include <xa.h>

#include <db.h>

/*
 * The two servers are largely identical, #ifdef the source code.
 */
#ifdef SERVER1
#define	TXN_FUNC		TestTxn1
#define	TXN_STRING		"TestTxn1"
#endif
#ifdef SERVER2
#define	TXN_FUNC		TestTxn2
#define	TXN_STRING		"TestTxn2"
#endif
void TXN_FUNC(TPSVCINFO *);

#define	HOME	"../data"
#define NUMDB	2

char *progname;					/* Server run-time name. */

/*
 * Called when each server is started.  It creates and opens a database.
 */
int
tpsvrinit(int argc, char* argv[])
{
	progname = argv[0];
	return (init_xa_server(NUMDB, progname, 0));
}

/* Called when the servers are shutdown.  This closes the database. */
void
tpsvrdone()
{
	close_xa_server(NUMDB, progname);
}

/* 
 * This function is called by the client.  Here, on Server 1 data is
 * inserted into table 1 and it returns successfully.  On Server 2 data
 * is inserted into table 2, but then it sleeps for 30 seconds then
 * returns, in order to force a timeout and to make sure that the data
 * is rolled back in both databases. 
 */
void
TXN_FUNC(TPSVCINFO *msg)
{
	DBT data;
	DBT key;
	int ret, val, i;
	DB *db;
	const char *name;
#ifdef SERVER1
	i = 0;
#else
	i = 1;
#endif

	db = dbs[i];
	name = db_names[i];

        val = 1;

	memset(&key, 0, sizeof(key));
	key.data = &val;
	key.size = sizeof(val);
	memset(&data, 0, sizeof(data));
	data.data = &val;
	data.size = sizeof(val);

	if (verbose) {
		printf("put: key in %s: %i\n", val, db_names[0]);
		printf("put: data in %s: %i\n", val, db_names[0]);
	}
	if ((ret = db->put(db, NULL, &key, &data, 0)) != 0) {
		if (ret == DB_LOCK_DEADLOCK)
			goto abort;
		fprintf(stderr, "%s: %s: %s->put: %s\n",
		    progname, TXN_STRING, name, 
		    db_strerror(ret));
		goto err;
	}

        /* Sleep for 30 seconds to force a timeout error. */
#ifdef SERVER2
        sleep(30);
#endif

	tpreturn(TPSUCCESS, 0L, 0, 0L, 0);
        if (0) {
abort:		if (verbose)
			printf("%s: %s: abort\n", progname, TXN_STRING);
		tpreturn(TPSUCCESS, 1L, 0, 0L, 0);
	}
	return;
err:
	tpreturn(TPFAIL, 0L, 0, 0L, 0);
}

