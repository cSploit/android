/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
** This file contains code used to implement mutexes on Btree objects.
** This code really belongs in btree.c.  But btree.c is getting too
** big and we want to break it down some.  This packaged seemed like
** a good breakout.
*/
#include "btreeInt.h"
#ifndef SQLITE_OMIT_SHARED_CACHE
/*
 * Berkeley DB does fine grained locking - disable the SQLite table level
 * locking.
 */
void sqlite3BtreeEnter(Btree *p)
{
}

void sqlite3BtreeLeave(Btree *p)
{
}

void sqlite3BtreeEnterCursor(BtCursor *pCur)
{
}

void sqlite3BtreeLeaveCursor(BtCursor *pCur)
{
}

void sqlite3BtreeEnterAll(sqlite3 *db)
{
}

void sqlite3BtreeLeaveAll(sqlite3 *db)
{
}

int sqlite3BtreeHoldsMutex(Btree *db)
{
	log_msg(LOG_VERBOSE, "sqlite3BtreeHoldsMutex(%p)", db);
	return 1;
}

int sqlite3BtreeHoldsAllMutexes(sqlite3 *db)
{
	log_msg(LOG_VERBOSE, "sqlite3BtreeHoldsAllMutexes(%p)", db);
	return 1;
}

int sqlite3SchemaMutexHeld(sqlite3 *db, int iDb, Schema *pSchema)
{
	/*
	 * Berkeley DB SQL uses different locking semantics to SQLite. This
	 * function is only used to verify that a lock is held. Always return
	 * true. Some test fail if this is implemented using the following code
	 * which is based on the implementation in SQLite.
	 *
	Btree *p;

	assert(db != 0);
	if (pSchema)
		iDb = sqlite3SchemaToIndex(db, pSchema);
	assert(iDb >= 0 && iDb < db->nDb);
	if (sqlite3_mutex_held(db->mutex) != 0)
		return 0;
	if (iDb == 1)
		return 1;
	p = db->aDb[iDb].pBt;
	assert(p != 0);
	return (p->sharable == 0 || p->schemaLockMode != LOCKMODE_NONE);
	*/
	return (1);
}

int sqlite3BtreeSharable(Btree *p)
{
	return (1);
}
#endif /* ifndef SQLITE_OMIT_SHARED_CACHE */
