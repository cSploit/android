/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * Basic smoke test for XA transactions.  The client randomly sends requests
 * to each of the servers to insert data into table 1, and inserts the
 * same data into table 2 using regular transactions.
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

#include "datafml.h"
#include "hdbrec.h"
#include "htimestampxa.h"
#include "../utilities/bdb_xa_util.h"

#define HOME	"../data2"
#define	TABLE1	"../data/table1.db"
#define	TABLE2	"../data2/table2.db"

char *progname;					/* Client run-time name. */

int   usage(void);

int
main(int argc, char* argv[])
{
	DB *dbp2;
	DBT key, data;
	FBFR *buf, *replyBuf;
	HDbRec rec;
	TPINIT *initBuf;
        DB_ENV *dbenv2, *dbenv1;
	long len, replyLen, seqNo;
	int ch, cnt, cnt_abort, cnt_commit, cnt_server1, i, ret;
	char *target;
	char *home = HOME;
	u_int32_t flags = DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN |
	  DB_INIT_LOCK | DB_CREATE | DB_RECOVER | DB_REGISTER;
	u_int32_t dbflags = DB_CREATE;

	progname = argv[0];

	dbenv2 = dbenv1  = NULL;
	dbp2 = NULL;
	buf = replyBuf = NULL;
	initBuf = NULL;
	cnt = 1000;
	cnt_abort = cnt_commit = cnt_server1 = 0;

	while ((ch = getopt(argc, argv, "n:v")) != EOF)
		switch (ch) {
		case 'n':
			cnt = atoi(optarg);
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

	/* Seed random number generator. */
	srand((u_int)(time(NULL) | getpid()));

	if (tpinit((TPINIT *)NULL) == -1)
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpinit() OK\n", progname);

	/* Allocate init buffer */
	if ((initBuf = (TPINIT *)tpalloc("TPINIT", NULL, TPINITNEED(0))) == 0)
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpalloc(\"TPINIT\") OK\n", progname);

	/* Create the DB environment. */
	if ((ret = db_env_create(&dbenv2, 0)) != 0 ||
	    (ret = dbenv2->open(dbenv2, home, flags, 0)) != 0) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, home, db_strerror(ret));
		goto err;
	}
	dbenv2->set_errfile(dbenv2, stderr);
	if (verbose)
		printf("%s: opened %s OK\n", progname, home);

	/* 
	 * Open table #2 -- Data is inserted into table 1 using XA
	 * transactions, and inserted into table 2 using regular transactions.
	 */
	if ((ret = db_create(&dbp2, dbenv2, 0)) != 0 ||
	    (ret = dbp2->open(dbp2,
	    NULL, TABLE2, NULL, DB_BTREE, dbflags, 0660)) != 0) {
		fprintf(stderr,
		    "%s: %s %s\n", progname, TABLE2, db_strerror(ret));
		goto err;
	}
	if (verbose)
		printf("%s: opened %s OK\n", progname, TABLE2);

	/* Allocate send buffer. */
	len = Fneeded(1, 3 * sizeof(long));
	if ((buf = (FBFR*)tpalloc("FML32", NULL, len)) == 0)
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpalloc(\"FML32\"), send buffer OK\n", progname);

	/* Allocate reply buffer. */
	replyLen = 1024;
	if ((replyBuf = (FBFR*)tpalloc("FML32", NULL, replyLen)) == NULL)
		goto tuxedo_err;
	if (verbose)
		printf("%s: tpalloc(\"FML32\"), reply buffer OK\n", progname);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	for (rec.SeqNo = 1, i = 0; i < cnt; ++i, ++rec.SeqNo) {
		GetTime(&rec.Ts);

		if (Fchg(buf, SEQ_NO, 0, (char *)&rec.SeqNo, 0) == -1)
			goto tuxedo_fml_err;
		if (verbose)
			printf("%s: Fchg(), sequence number OK\n", progname);
		if (Fchg(buf, TS_SEC, 0, (char *)&rec.Ts.Sec, 0) == -1)
			goto tuxedo_fml_err;
		if (verbose)
			printf("%s: Fchg(), seconds OK\n", progname);
		if (Fchg(buf, TS_USEC, 0, (char *)&rec.Ts.Usec, 0) == -1)
			goto tuxedo_fml_err;
		if (verbose)
			printf("%s: Fchg(), microseconds OK\n", progname);

		if (tpbegin(60L, 0L) == -1)
			goto tuxedo_err;
		if (verbose)
			printf("%s: tpbegin() OK\n", progname);

		/* Randomly send half of our requests to each server. */
		if (rand() % 2 > 0) {
			++cnt_server1;
			target = "TestTxn1";
		} else
			target = "TestTxn2";
		if (tpcall(target, (char *)buf,
		    0L, (char **)&replyBuf, &replyLen, TPSIGRSTRT) == -1)
			goto tuxedo_err;
		/* Commit for a return value of 0, otherwise abort. */
		if (tpurcode == 0) {
			++cnt_commit;
			if (verbose)
				printf("%s: txn success\n", progname);

			if (tpcommit(0L) == -1)
				goto abort;
			if (verbose)
				printf("%s: tpcommit() OK\n", progname);

			/*
			 * Store a copy of the key/data pair into table #2
			 * on success, we'll compare table #1 and table #2
			 * after the run finishes.
			 */
			seqNo = rec.SeqNo;
			key.data = &seqNo;
			key.size = sizeof(seqNo);
			data.data = &seqNo;
			data.size = sizeof(seqNo);
			if ((ret =
			    dbp2->put(dbp2, NULL, &key, &data, 0)) != 0) {
				fprintf(stderr, "%s: DB->put: %s %s\n",
				    progname, TABLE2, db_strerror(ret));
				goto err;
			}
		} else {
 abort:			++cnt_abort;
			if (verbose)
				printf("%s: txn failure\n", progname);

			if (tpabort(0L) == -1)
				goto tuxedo_err;
			if (verbose)
				printf("%s: tpabort() OK\n", progname);
		}
	}

	printf("%s: %d requests: %d committed, %d aborted\n",
	    progname, cnt, cnt_commit, cnt_abort);
	printf("%s: %d sent to server #1, %d sent to server #2\n",
	    progname, cnt_server1, cnt - cnt_server1);

	/* Check that database 1 and database 2 are identical. */
	if (dbp2 != NULL)
		(void)dbp2->close(dbp2, 0);
	dbp2 = NULL;
	if ((ret = db_env_create(&dbenv1, 0)) != 0 ||
	    (ret = dbenv1->open(dbenv1, "../data", flags, 0)) != 0) 
		goto err;
	ret = check_data(dbenv1, TABLE1, dbenv2, TABLE2, progname);

	if (0) {
tuxedo_err:	fprintf(stderr, "%s: TUXEDO ERROR: %s (code %d)\n",
		    progname, tpstrerror(tperrno), tperrno);
		goto err;
	}
	if (0) {
tuxedo_fml_err:	fprintf(stderr, "%s: FML ERROR: %s (code %d)\n",
		    progname, Fstrerror(Ferror), Ferror);
	}
	if (0) {
err:		ret = EXIT_FAILURE;
	}

	if (replyBuf != NULL)
		tpfree((char *)replyBuf);
	if (buf != NULL)
		tpfree((char *)buf);
	if (initBuf != NULL)
		tpfree((char *)initBuf);
	if (dbp2 != NULL)
		(void)dbp2->close(dbp2, 0);
	if (dbenv1 != NULL)
		(void)dbenv1->close(dbenv1, 0);
	if (dbenv2 != NULL)
		(void)dbenv2->close(dbenv2, 0);

	tpterm();
	if (verbose)
		printf("%s: tpterm() OK\n", progname);

	return (ret);
}

int
usage()
{
	fprintf(stderr, "usage: %s [-v] [-n txn]\n", progname);
	return (EXIT_FAILURE);
}
