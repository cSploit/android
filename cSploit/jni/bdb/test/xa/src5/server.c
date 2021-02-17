/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
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

#define KEY_VALUE		0
#define READ_STRING		"read_db1"
#define WRITE_STRING		"write_db1"

void write_db1(TPSVCINFO *);
void read_db1(TPSVCINFO *);

#define	HOME	"../data"
#define NUMDB	1

char *progname;					/* Server run-time name. */

/* Called once when the server is started. Creates and opens 2 databases. */
int
tpsvrinit(int argc, char* argv[])
{
	int mvcc = 0;

#ifdef MVCC_FLAG
	mvcc = 1;
#endif
	progname = argv[0];
	return (init_xa_server(NUMDB, progname, mvcc));
}

/* Called once when the servers are shutdown.  Closes the databases. */
void
tpsvrdone()
{
	close_xa_server(NUMDB, progname);
}


void
write_db1(TPSVCINFO *msg)
{
	int ret, commit, key_value, data_value;
	DBT key, data;
	DB *db;

	db = dbs[0];
	memset(&key, 0, sizeof key);
	memset(&data, 0, sizeof data);
	commit = 1;

	key_value = data_value = KEY_VALUE;
	data.data = &data_value;
	data.size = sizeof(data_value);
	key.data = &key_value;
	key.size = sizeof(key_value);

	/* Insert data into the tables. */
	if (verbose) {
		printf("put: key: %i ", key_value);
		printf("put: data: %i\n", data_value);
	}
	/* Insert data into the tables. */
	if ((ret = db->put(db, NULL, &key, 
	    &data, 0)) != 0) {
		if (ret == DB_LOCK_DEADLOCK || 
		    ret == DB_LOCK_NOTGRANTED)
			goto abort;
		fprintf(stderr, "%s: %s: %s->put: %s\n",
		    progname, WRITE_STRING, db_names[0], 
		    db_strerror(ret));
		goto err;
	}

	/* Returns a commit or abort command to the client. */
	if (verbose)
		printf("%s: %s: commit\n", progname, WRITE_STRING);
	tpreturn(TPSUCCESS, 0L, 0, 0L, 0);
	if (0) {
abort:		if (verbose)
			printf("%s: %s: abort\n", progname, WRITE_STRING);
		tpreturn(TPSUCCESS, 1L, 0, 0L, 0);
	}
	return;

err:	tpreturn(TPFAIL, 1L, 0, 0L, 0);
}

void
read_db1(TPSVCINFO *msg)
{
	int ret, db_num, commit, key_value;
	DBT key, data;
	DB *db;

	db = dbs[0];
	memset(&key, 0, sizeof key);
	memset(&data, 0, sizeof data);
	commit = 1;

	db_num = key_value = KEY_VALUE;
	key.data = &key_value;
	key.size = sizeof(key_value);
	data.flags = DB_DBT_MALLOC;

	/* Insert data into the tables. */
	if (verbose) {
		printf("get: key: %i ", key_value);
		printf("get: data.\n");
	}
	/* Get data from the table. */
	if ((ret = db->get(db, NULL, &key, &data, 0)) != 0) {
		if (ret == DB_LOCK_DEADLOCK || 
		    ret == DB_LOCK_NOTGRANTED)
			goto abort;
		fprintf(stderr, "%s: %s: %s->get: %s\n",
		    progname, READ_STRING, db_names[0], 
		    db_strerror(ret));
		goto err;
	}
	if (data.data != NULL)
		free(data.data);

	/* Returns a commit or abort command to the client. */
	if (verbose)
		printf("%s: %s: commit\n", progname, READ_STRING);
	tpreturn(TPSUCCESS, 0L, 0, 0L, 0);
	if (0) {
abort:		if (verbose)
			printf("%s: %s: abort\n", progname, READ_STRING);
		tpreturn(TPSUCCESS, 1L, 0, 0L, 0);
	}
	return;

err:	tpreturn(TPFAIL, 1L, 0, 0L, 0);
}

