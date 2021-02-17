/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This server is only called by bdb1.  It takes the data sent by bdb1 and 
 * inserts it into db2.
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
#include "../utilities/bdb_xa_util.h"

#define NUMDB 2
static char *progname;

/* Write the given data into the given database. */
static int writedb(DB * dbp, void *buf, u_int32_t size){
	DBT key, data;
	size_t len;
	int ch, ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = buf;
	data.data = buf;
	data.size = key.size = size;
	ret = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
	switch (ret) {
	case 0:
		return (EXIT_SUCCESS);
	case DB_KEYEXIST:
	        return (EXIT_SUCCESS);
	case DB_LOCK_DEADLOCK:
	        return (EXIT_SUCCESS);
	default:
		userlog("put: %s", db_strerror(ret));
		return (-1);
	}
}

/* Open the databases used by this server when it is started. */
int
tpsvrinit(argc, argv)
int argc;
char **argv;
{
	progname = argv[0];

	return (init_xa_server(NUMDB, progname, 0));
}

/* Close the database when the server is shutdown. */
void
tpsvrdone(void)
{
	close_xa_server(NUMDB,progname);	
}

/* 
 * Get the data the calling server just inserted into db1 and insert it into
 * db2.  Fgets32 is used to get the key passed by the calling server. 
 */
int
WRITE2(rqst)
TPSVCINFO *rqst;

{
	char buf[100];
	DBT key, data;
	FBFR32 *reqbuf = (FBFR32*)rqst->data;
	int ret;
	
	Fgets32(reqbuf, TA_REPOSPARAM, 0, buf);
	userlog("buf:[%s]", buf);

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
     
	key.data = buf;
	key.size = (u_int32_t)strlen(buf);
	data.flags = DB_DBT_MALLOC;
		
	/* Get the data that the calling server inserted into db1 */
	switch (ret = dbs[0]->get(dbs[0], NULL, &key, &data, DB_READ_UNCOMMITTED)){
	case 0:
		break;
	case DB_LOCK_DEADLOCK:
	        tpreturn(TPSUCCESS, 1L, rqst->data, 0L, 0);
	default:
		userlog("get: %s", db_strerror(ret));
		tpreturn(TPFAIL, 0L, rqst->data, 0L, 0);
	}
	
	/* Write the data to db2 */
	if(writedb(dbs[1], data.data, data.size) != 0){
		tpreturn(TPSUCCESS, 1L, rqst->data, 0L, 0);
	}
	tpreturn(TPSUCCESS, 0, rqst->data, 0L, 0);
}

