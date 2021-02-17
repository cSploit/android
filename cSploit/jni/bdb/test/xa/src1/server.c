/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This is the code for the two servers used in the first XA test.  Server 1
 * and Server 2 are both called by the client to insert data into table 1
 * using XA transactions.  Server 1 also can forward requests to Server 2.
 */
#include <sys/types.h>

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

#include "datafml.h"
#include "hdbrec.h"
#include "htimestampxa.h"
#include "../utilities/bdb_xa_util.h"

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

#define NUMDB 2

int cnt_forward;				/* Forwarded requests. */
int cnt_request;				/* Total requests. */

char *progname;				/* Server run-time name. */

/*
 * Called when each server is started.  It creates and opens the
 * two database handles.
 */
int
tpsvrinit(int argc, char* argv[])
{
	progname = argv[0];
	return (init_xa_server(NUMDB, progname, 0));
}

/* Called when the servers are shutdown.  This closes the databases. */
void
tpsvrdone()
{
	close_xa_server(NUMDB, progname);
	printf("%s: %d requests, %d requests forwarded to the other server\n",
	    progname, cnt_request, cnt_forward);
}

/* 
 * This function is called by the client.  Here Server 1 and Server 2 insert
 * data into table 1 using XA transactions.  Server 1 can also forward its 
 * request to Server 2.
 */
void
TXN_FUNC(TPSVCINFO *msg)
{
	DBT data;
	DBT key;
	FBFR *replyBuf;
	HDbRec rcrd;
	long replyLen, seqNo;
	int ret, i;

	++cnt_request;

#ifdef SERVER1
	/*
	 * Test that servers can forward to other servers.  Randomly forward
	 * half of server #1's requests to server #2.
	 */
	if (rand() % 2 > 0) {
		++cnt_forward;

		replyLen = 1024;
		if ((replyBuf =
		    (FBFR*)tpalloc("FML32", NULL, replyLen)) == NULL ||
		    tpcall("TestTxn2", msg->data,
		    0, (char**)&replyBuf, &replyLen, TPSIGRSTRT) == -1) {
			fprintf(stderr, "%s: TUXEDO ERROR: %s (code %d)\n",
			    progname, tpstrerror(tperrno), tperrno);
			tpfree((char*)replyBuf);
			tpreturn(TPFAIL, 0L, 0, 0L, 0);
		} else {
			tpfree((char*)replyBuf);
			tpreturn(TPSUCCESS, tpurcode, 0, 0L, 0);
		}
		return;
	}
#endif
						/* Read the record. */
	if (Fget((FBFR*)msg->data, SEQ_NO, 0, (char *)&rcrd.SeqNo, 0) == -1)
		goto fml_err;
	if (Fget((FBFR*)msg->data, TS_SEC, 0, (char *)&rcrd.Ts.Sec, 0) == -1)
		goto fml_err;
	if (Fget(
	    (FBFR*)msg->data, TS_USEC, 0, (char *)&rcrd.Ts.Usec, 0) == -1) {
fml_err:	fprintf(stderr, "%s: FML ERROR: %s (code %d)\n",
		    progname, Fstrerror(Ferror), Ferror);
		goto err;
	}

	seqNo = rcrd.SeqNo;			/* Update the record. */
	memset(&key, 0, sizeof(key));
	key.data = &seqNo;
	key.size = sizeof(seqNo);
	memset(&data, 0, sizeof(data));
	data.data = &seqNo;
	data.size = sizeof(seqNo);

	if (verbose) {
		 __db_prdbt(&key, 0, "put: key: %s\n", stdout, 
		     pr_callback, 0, 0);
		 __db_prdbt(&data, 0, "put: data: %s\n", stdout, 
		     pr_callback, 0, 0);
	}

	for (i = 0; i < NUMDB; i++) {
		strcpy(rcrd.Msg, db_names[i]);	
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

	/*
	 * Decide if the client is going to commit the global transaction or
	 * not, testing the return-value path back to the client; this is the
	 * path we'd use to resolve deadlock, for example.  Commit 80% of the
	 * time.  Returning 0 causes the client to commit, 1 to abort.
	 */
	if (rand() % 10 > 7) {
		if (verbose)
			printf("%s: %s: commit\n", progname, TXN_STRING);
		tpreturn(TPSUCCESS, 0L, 0, 0L, 0);
	} else {
abort:		if (verbose)
			printf("%s: %s: abort\n", progname, TXN_STRING);
		tpreturn(TPSUCCESS, 1L, 0, 0L, 0);
	}
	return;

err:	tpreturn(TPFAIL, 1L, 0, 0L, 0);
}

