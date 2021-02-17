/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This server is called by the client.  It inserts a value into db1 then
 * calls bdb2 to insert a value into db2.
 */

#include <db.h>
#include <xa.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <atmi.h>	/* TUXEDO Header File */
#include <userlog.h>	/* TUXEDO Header File */
#include <string.h>
#include <time.h>
#include <tpadm.h>
#include <unistd.h>
#include "../utilities/bdb_xa_util.h"

#define	NUMDB 1

static int times = 1;
static long seq = 1;

static char *progname;

/* Open the database when the server is started. */
int
tpsvrinit(argc, argv)
int argc;
char **argv;
{
	char ch;

	progname = argv[0];

	/* Some compilers warn if argc and argv aren't used. */
	while ((ch = getopt(argc, argv, "t:")) != EOF)
		switch (ch) {
		case 't':
			times = atoi(optarg);
			break;
		}
	

	return (init_xa_server(NUMDB, progname, 0));
}

/* Close the database when the server is shutdown. */
void
tpsvrdone(void)
{
	close_xa_server(NUMDB, progname);
}

/* Insert a value into db1, then call bdb2 to insert that value into db2. */
int
WRITE(rqst)
TPSVCINFO *rqst;
{
	long rcvlen;
	int ret, i;
	DBT key, data;
	size_t len;
	int ch;
	char *p, *t, buf[1024];
	DB *dbp = dbs[0];

	tpbegin(10,0);

	for(i=0; i<times; i++){
		/* Insert random records into the db1*/
		memset(&key, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
		sprintf(buf, "%ld %ld",random(), seq++);

		len = strlen(buf);
		key.data = buf;
		data.data = buf;
		data.size = key.size = (u_int32_t)len ;
	
		switch (ret = dbp->put(dbp, NULL, &key, &data, 
		    DB_NOOVERWRITE)) {
		case 0:
			break;
		case DB_LOCK_DEADLOCK:
			if(tpabort(0) == -1){
				userlog("tpabort() fail:%s", 
				    tpstrerror(tperrno));
			}
			tpreturn(TPSUCCESS, 1, rqst->data, 0L, 0);
		default:
			userlog("put: %s", db_strerror(ret));
			if(tpabort(0) == -1){
				userlog("tpabort() fail:%s", 
				    tpstrerror(tperrno));
			}
			tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
		}
	}

	/* 
	 * Insert the key into a queue and call bdb2 where it will read
	 * the key out of the queue.
	 */
	FBFR32 *reqbuf;
	char * rcvbuf;

	if((reqbuf = (FBFR32*) tpalloc("FML32", NULL, 100)) == NULL) {
	   tpabort(0);
           userlog("alloc fail");
	   tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
	}

	if((rcvbuf = (char *) tpalloc("STRING", NULL, 100)) == NULL) {
	   tpabort(0);
           userlog("alloc fail");
	   tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
	}

	Fadd32(reqbuf, TA_REPOSPARAM, buf, 0);

	/* Call bdb2 */
	ret = tpcall("WRITE2", (char *)reqbuf, 0, &rcvbuf, &rcvlen,TPSIGRSTRT);
	tpfree((char*)reqbuf);
	tpfree((char*)rcvbuf);
	if(ret == -1){
           userlog("call WRITE2 fail");
	   if(-1 == tpabort(0))
		userlog("tpabort() fail:%s", tpstrerror(tperrno));
	   tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
	}
	/* Commit for a return value of 0, otherwise abort. */
	if (tpurcode == 0) {
		if(tpcommit(0) == -1){
           		userlog("tpcommit fail");
			tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
		}
	} else {
		if(tpabort(0) == -1){
           		userlog("tpabort fail");
			tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
		}
	}

	tpreturn(TPSUCCESS, 0, rqst->data, 0L, 0);
}

/* Iterates the database with a cursor. */
int
CURSOR(rqst)
TPSVCINFO *rqst;
{
	int ret,count;
	DBT key, value;
	DBC *cursorp;
	DB *dbp = dbs[0];
	
	tpbegin(60*10,0);

	/* Get the cursor */
	ret = dbp->cursor(dbp, NULL, &cursorp, 0);
	if (ret != 0) {
		userlog("count_records: cursor open failed.");
		tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
	}

	/* Get the key DBT used for the database read */
	memset(&key, 0, sizeof(DBT));
	memset(&value, 0, sizeof(DBT));
	do {
	    ret = cursorp->c_get(cursorp, &key, &value, DB_NEXT);
	    switch (ret) {
		    case 0:
			    count++;
			    break;
		    case DB_NOTFOUND:
			    break;
		    case DB_LOCK_DEADLOCK:
			    if(tpcommit(0) == -1)
				    userlog("tpcommit() fail:%s", 
					tpstrerror(tperrno));
    			    cursorp->c_close(cursorp);
			    tpreturn(TPSUCCESS, 1L, rqst->data, 0L, 0);
			    break;
		    default:
			    if(tpabort(0) == -1)
				    userlog("tpabort() fail:%s", 
					tpstrerror(tperrno));
    			    cursorp->c_close(cursorp);
			    dbp->err(dbp, ret, 
				"Count records unspecified error");
			    tpreturn(TPFAIL, 0, rqst->data, 0L, 0);
	    }
	} while (ret == 0);
	cursorp->c_close(cursorp);
	tpcommit(0);
	tpreturn(TPSUCCESS, 0, rqst->data, 0L, 0);
    }
