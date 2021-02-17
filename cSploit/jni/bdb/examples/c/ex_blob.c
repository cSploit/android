/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_LEN 25
#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

int db_setup __P((const char *, FILE *, const char *));
int db_teardown __P((const char *, FILE *, const char *));
static int usage __P((void));

const char *progname = "ex_blob";		/* Program name. */

/*
 * An example of a program that stores data items in Berkeley DB blob
 * files.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	char *cmd;
	const char *home;

	int ch;
	home = "EX_BLOBDIR";
	while ((ch = getopt(argc, argv, "h:")) != EOF)
		switch (ch) {
		case 'h':
			home = optarg;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	if ((cmd = malloc(strlen(home) + CMD_LEN)) == NULL)
		return (EXIT_FAILURE);

#ifdef _WIN32
	sprintf(cmd, "rmdir %s /q/s", home);
#else
	sprintf(cmd, "rm -rf %s", home);
#endif
	system(cmd);
	sprintf(cmd, "mkdir %s", home);
	system(cmd);
	free(cmd);

	printf("Setup env\n");
	if (db_setup(home, stderr, progname) != 0)
		return (EXIT_FAILURE);

	return (EXIT_SUCCESS);
}

int
db_setup(home, errfp, progname)
	const char *home, *progname;
	FILE *errfp;
{
	DBC *dbc;
	DB_ENV *dbenv;
	DB *dbp;
	DB_STREAM *dbs;
	DB_TXN *txn;
	DBT data, key;
	int ret;
	db_off_t size;
	unsigned int i;

	dbc = NULL;
	dbenv = NULL;
	dbp = NULL;
	dbs = NULL;
	txn = NULL;
	memset (&data, 0, sizeof(DBT));

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}
	dbenv->set_errfile(dbenv, errfp);
	dbenv->set_errpfx(dbenv, progname);

	/*
	 * We want to specify the shared memory buffer pool cachesize.
	 */
	if ((ret = dbenv->set_cachesize(dbenv, 0, 64 * 1024 *1024, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		goto err;
	}

	/* Enable blob files and set the size threshold. */
	if ((ret = dbenv->set_blob_threshold(dbenv, 1000, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_blob_threshold");
		goto err;
	}

	/* Open the environment with full transactional support. */
	if ((ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL |
	    DB_INIT_TXN, 0644)) != 0) {
		dbenv->err(dbenv, ret, "environment open: %s", home);
		goto err;
	}

	/* Open a database in the environment. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "database create");
		goto err;
	}

	/* Open a database with btree access method. */
	if ((ret = dbp->open(dbp, NULL, "exblob_db1.db", NULL,
	    DB_BTREE, DB_CREATE|DB_AUTO_COMMIT,0644)) != 0) {
		dbenv->err(dbenv, ret, "database open");
		goto err;
	}

	/* Insert an item that will overflow. */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	key.ulen = key.size = sizeof(int);
	key.flags = DB_DBT_USERMEM;

	data.size = data.ulen = 4000;
	data.data = malloc(data.size);
	for (i = 0; i < data.size; i++)
		((char *)data.data)[i] = 'a' + (i % 26);
	data.flags = DB_DBT_USERMEM;

#define COUNT 5000
	for (i = 1; i < COUNT; i++) {
		key.data = &i;
		/* Use a mixture of blob and onpage items. */
		if (i%23 == 0)
			data.size = 20;
		if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0) {
			dbenv->err(dbenv, ret, "blob put");
			goto err;
		}
		if (i%23 == 0)
			data.size = data.ulen;
	}

	for (i = 1; i < COUNT; i += 2) {
		key.data = &i;
		memset(data.data, 0, data.size);
		if ((ret = dbp->get(dbp, NULL, &key, &data, 0)) != 0) {
			dbenv->err(dbenv, ret, "blob get");
			goto err;
		}
		if (strncmp((char *)data.data, "abcdefghijklmno", 15) != 0)
			printf("blob data mismatch\n");
	}
	
	/* Blobs can also be accessed using the DB_STREAM API. */
	if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0){
		dbenv->err(dbenv, ret, "txn");
		goto err;
	}

	if ((ret = dbp->cursor(dbp, txn, &dbc, 0)) != 0) {
		dbenv->err(dbenv, ret, "cursor");
		goto err;
	}

	/*
	 * Set the cursor to a blob.  Use DB_DBT_PARTIAL with
	 * dlen == 0 to avoid getting any blob data.
	 */
	data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
	data.dlen = 0;
	if ((ret = dbc->get(dbc, &key, &data, DB_FIRST)) != 0) {
		dbenv->err(dbenv, ret, "Not a blob");
		goto err;
	}
	data.flags = DB_DBT_USERMEM;

	/* Create a stream on the blob the cursor points to. */
	if ((ret = dbc->db_stream(dbc, &dbs, DB_STREAM_WRITE)) != 0) {
		dbenv->err(dbenv, 0, "Creating stream.");
		goto err;
	}
	/* Get the size of the blob. */
	if ((ret = dbs->size(dbs, &size, 0)) != 0) {
		dbenv->err(dbenv, 0, "Stream size.");
		goto err;
	}
	/* Read from the blob. */
	if ((ret = dbs->read(dbs, &data, 0, (u_int32_t)size, 0)) != 0) {
		dbenv->err(dbenv, 0, "Stream read.");
		goto err;
	}
	/* Write data to the blob, increasing its size. */
	if ((ret = dbs->write(dbs, &data, size/2, 0)) != 0) {
		dbenv->err(dbenv, 0, "Stream write.");
		goto err;
	}
	/* Close the stream. */
	if ((ret = dbs->close(dbs, 0)) != 0) {
		dbenv->err(dbenv, 0, "Stream close.");
		goto err;
	}
	dbs = NULL;
	dbc->close(dbc);
	dbc = NULL;
	txn->commit(txn, 0);
	txn = NULL;
	free(data.data);
	data.data = NULL;

	for (i = 3; i < COUNT; i += 2) {
		key.data = &i;
		if ((ret = dbp->del(dbp, NULL, &key, 0)) != 0) {
			dbenv->err(dbenv, ret, "blob del");
			goto err;
		}
	}

	/* Close the database handle. */
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr, "database close: %s\n", db_strerror(ret));
		return (1);
	}

	/* Close the environment handle. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
		return (1);
	}
	return (0);

err:	if (data.data != NULL)
		free(data.data);
	if (dbs != NULL)
		dbs->close(dbs, 0);
	if (txn != NULL)
		txn->abort(txn);
	if (dbc != NULL)
		dbc->close(dbc);
	if (dbp != NULL)
		dbp->close(dbp, 0);
	if (dbenv != NULL)
		dbenv->close(dbenv, 0);

	return (ret);
}

static int
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-h home] \n", progname);
	return (EXIT_FAILURE);
}
