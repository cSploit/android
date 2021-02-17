/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

typedef struct PgHdr PgHdr;
/* Initialize and shutdown the page cache subsystem */
struct PgHdr {
	int empty; /* DB doesn't use this structure. */
};

int sqlite3PcacheInitialize(void);
void sqlite3PcacheShutdown(void);
void sqlite3PCacheBufferSetup(void *p, int sz, int n);
void sqlite3PCacheSetDefault(void);
#ifdef SQLITE_TEST
void sqlite3PcacheStats(int *a,int *b,int *c,int *d);
#endif

