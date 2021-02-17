/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * #19612 Data not rolled back when transaction aborted after timout.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tx.h>
#include <atmi.h>
#include <fml32.h>
#include <fml1632.h>

#include <db.h>

#include "../utilities/bdb_xa_util.h"

#define HOME	"../data"
#define	TABLE1	"../data/table1.db"
#define	TABLE2	"../data/table2.db"

DB_ENV *dbenv;
char *progname;					/* Client run-time name. */

int   usage(void);

int
main(int argc, char* argv[])
{
	DB *dbp3;
	DBT key, data;
	TPINIT *initBuf;
        FBFR *replyBuf;
	long replyLen;
	int ch, ret, i;
	char *target;
	char *home = HOME;
	u_int32_t flags = DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN |
	  DB_INIT_LOCK | DB_CREATE | DB_THREAD | DB_RECOVER | DB_REGISTER;
	u_int32_t dbflags = DB_CREATE | DB_THREAD;

	progname = argv[0];

	initBuf = NULL;
        ret = 0;
        replyBuf = NULL;
        replyLen = 1024;

	while ((ch = getopt(argc, argv, "v")) != EOF)
		switch (ch) {
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

	if (tpinit((TPINIT *)NULL) == -1)
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpinit() OK\n", progname);

	/* Create the DB environment. */
	if ((ret = db_env_create(&dbenv, 0)) != 0 ||
	    (ret = dbenv->open(dbenv, home, flags, 0)) != 0) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, home, db_strerror(ret));
		goto err;
	}
	dbenv->set_errfile(dbenv, stderr);
	if (verbose)
		printf("%s: opened %s OK\n", progname, home);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

        /* Allocate reply buffer. */
	if ((replyBuf = (FBFR*)tpalloc("FML32", NULL, replyLen)) 
	    == NULL) 
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpalloc(\"FML32\"), reply buffer OK\n", 
		    progname);
	for (i = 0; i < 2; i++) {
        	if (tpbegin(10L, 0L) == -1)
			goto tuxedo_err;
		if (verbose)
			printf("%s: tpbegin() OK\n", progname);

		if (tpcall("TestTxn1", NULL, 0L, (char **)&replyBuf, 
		    &replyLen, TPSIGRSTRT) == -1) 
			goto tuxedo_err;

        	/* This call will timeout. */
        	tpcall("TestTxn2", NULL, 0L, (char **)&replyBuf, &replyLen, 
            	    TPSIGRSTRT);
        	if (tperrno != TPETIME)
	        	goto tuxedo_err;

		if (i == 0) {
			if (tpabort(0L) == -1)
				goto tuxedo_err;
			if (verbose)
				printf("%s: tpabort() OK\n", progname);
		} else {
		  	/* Commit will fail due to the time out. */
			tpcommit(0L);
			if (tperrno != TPEABORT)
				goto tuxedo_err;
			if (verbose)
				printf("%s: tpcommit() OK\n", progname);
		}


		ret = check_data(dbenv, TABLE1, dbenv, TABLE2, progname);
	}

	if (0) {
tuxedo_err:	fprintf(stderr, "%s: TUXEDO ERROR: %s (code %d)\n",
		    progname, tpstrerror(tperrno), tperrno);
		goto err;
	}
	if (0) {
err:		ret = EXIT_FAILURE;
	}

	if (replyBuf != NULL)
		tpfree((char *)replyBuf);
	if (dbenv != NULL)
		(void)dbenv->close(dbenv, 0);

	tpterm();
	if (verbose)
		printf("%s: tpterm() OK\n", progname);

 	if (verbose && ret == 0)
		printf("%s: test passed.\n", progname);
	else if (verbose)
 		printf("%s: test failed.\n", progname);
	return (ret);
}

int
usage()
{
	fprintf(stderr, "usage: %s [-v] [-n txn]\n", progname);
	return (EXIT_FAILURE);
}
