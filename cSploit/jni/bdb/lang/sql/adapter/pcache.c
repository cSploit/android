/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include "sqliteInt.h"

int sqlite3PcacheInitialize(void){ return SQLITE_OK; }
void sqlite3PcacheShutdown(void){}
void sqlite3PCacheBufferSetup(void *p, int sz, int n) {}
void sqlite3PCacheSetDefault(void){}
#ifdef SQLITE_TEST
void sqlite3PcacheStats(int *a,int *b,int *c,int *d) {}
#endif
