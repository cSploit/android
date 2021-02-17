/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include "sqliteInt.h"
#include "btreeInt.h"

#ifndef	INT32_MAX
#define	INT32_MAX		2147483647
#endif

extern int __log_current_lsn(ENV *, DB_LSN *, u_int32_t *, u_int32_t *);
static sqlite3_file nullfile;

int sqlite3PagerIsMemdb(Pager *pPager) { return 0; }
int sqlite3PagerJournalMode(Pager *pPager, int mode) { return 0; }
i64 sqlite3PagerJournalSizeLimit(Pager *pPager, i64 limit)
{
	Btree *p = (Btree *)pPager;
	BtShared *pBt = p->pBt;
	u_int32_t oldSize = pBt->logFileSize;
	int ret;
	/* Choose an 8k min, since it's twice the common default page size. */
	if ( limit == -1 || limit >= 8192) {
		if (limit == -1) {
			pBt->logFileSize =
					SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT;
		} else
			pBt->logFileSize = (u_int32_t)limit;

		if (pBt->transactional) {
			ret = pBt->dbenv->set_lg_max(pBt->dbenv,
			    pBt->logFileSize);
			/* Restore the previous log size if it cannot be set. */
			if (ret != 0) {
				pBt->logFileSize = oldSize;
				pBt->dbenv->set_lg_max(pBt->dbenv,
				    pBt->logFileSize);
			}
		}
	}
	return pBt->logFileSize;
}

int sqlite3PagerLockingMode(Pager *pPager, int mode) {
	return 0;
}

int sqlite3PagerPagecount(Pager *pPager, int *pCount) {
	pCount = 0;
	return SQLITE_OK;
}

sqlite3_file *sqlite3PagerFile(Pager *pPager) {
	return &nullfile;
}

/*
** Return the full pathname of the database file.
*/
const char *sqlite3PagerFilename(Pager *pPager, int nullIfMemDb) {
	Btree *p = (Btree *)pPager;
	BtShared *pBt = p->pBt;
	return ((nullIfMemDb && (pBt->dbStorage == DB_STORE_TMP
	    || pBt->dbStorage == DB_STORE_INMEM)) ? "" : pBt->orig_name);
}

/*
** Return TRUE if the database file is opened read-only.  Return FALSE
** if the database is (in theory) writable.
*/
u8 sqlite3PagerIsreadonly(Pager *pPager){
	Btree *p = (Btree *)pPager;
	return (p->readonly ? 1 : 0);
}

/*
** Free as much memory as possible from the pager.
*/
void sqlite3PagerShrink(Pager *pPager){
	/***************IMPLEMENT***************/
}

/*
** Return the current journal mode.
*/
int sqlite3PagerGetJournalMode(Pager *pPager) {
	return (PAGER_JOURNALMODE_WAL);
}
/*
** Return the approximate number of bytes of memory currently
** used by the pager and its associated cache.
*/
int sqlite3PagerMemUsed(Pager *pPager) {
	return (0);
}

/*
** Return TRUE if the pager is in a state where it is OK to change the
** journalmode.  Journalmode changes can only happen when the database
** is unmodified.
*/
int sqlite3PagerOkToChangeJournalMode(Pager *pPager) {
	return (0);
}

int sqlite3PagerSetJournalMode(Pager *pPager, int eMode) {
	return (SQLITE_OK);
}

/*
** This function may only be called while a write-transaction is active in
** rollback. If the connection is in WAL mode, this call is a no-op.
** Otherwise, if the connection does not already have an EXCLUSIVE lock on
** the database file, an attempt is made to obtain one.
**
** If the EXCLUSIVE lock is already held or the attempt to obtain it is
** successful, or the connection is in WAL mode, SQLITE_OK is returned.
** Otherwise, either SQLITE_BUSY or an SQLITE_IOERR_XXX error code is
** returned.
*/
int sqlite3PagerExclusiveLock(Pager *pPager) {
	/* Berkeley DB is always WAL, so no-op it. */
	return (SQLITE_OK);
}

#ifndef SQLITE_OMIT_WAL

int sqlite3PagerWalCallback(Pager *pPager)
{
	Btree *p = (Btree *)pPager;
	BtShared *pBt = p->pBt;
	DB_LSN lsn;
	i64 total;
	u_int32_t bytes, mbytes;

	if (pBt == NULL || !pBt->env_opened || !pBt->transactional)
		return (0);

	/*
	 * Using log_current_lsn seems like an odd mechanism for retrieving the
	 * amount of data written to logs since the last checkpoint. It's the
	 * cheapest function in Berkeley DB that can do that though.
	 * Alternatively we could call DB_ENV->log_stat and use the st_wc_bytes
	 * and st_wc_mbytes fields.
	 */
	if (__log_current_lsn(pBt->dbenv->env, &lsn, &mbytes, &bytes) != 0)
		return (0);
	total = (mbytes * 1048576) + bytes;
	/*
	 * SQLite expects this to be a frame count, as near as DB gets is a
	 * number of database pages.
	 */
	total = total/SQLITE_DEFAULT_PAGE_SIZE;
	/* SQLite never overflows a 32 bit value, so nor will we. */
	if (total != (u32)total)
		total = INT32_MAX;

	return ((int)total);
}

int sqlite3PagerCheckpoint(Pager *pPager) {
	Btree *p;
	BtShared *pBt;
	int needUnlock;

	p = (Btree *)pPager;
	pBt = p->pBt;
	needUnlock = 0;

	if (pBt == NULL || !pBt->env_opened || !pBt->transactional)
		return (SQLITE_OK);

	/* Do a checkpoint immediately. */
	if (pBt->transactional && pBt->env_opened) {
#ifdef BDBSQL_SHARE_PRIVATE
		/* Ensure we have a write lock for checkpoint. */
		if (!btreeHasFileLock(p, 1)) {
			btreeScopedFileLock(p, 1, 0);
			needUnlock = 1;
		}
#endif
		pBt->dbenv->txn_checkpoint(pBt->dbenv, 0, 0, 0);
#ifdef BDBSQL_SHARE_PRIVATE
		if (needUnlock)
			btreeScopedFileUnlock(p, 1);
#endif
	}
	return (SQLITE_OK);
}

/*
** This function is called to close the connection to the log file prior
** to switching from WAL to rollback mode.
**
** Before closing the log file, this function attempts to take an
** EXCLUSIVE lock on the database file. If this cannot be obtained, an
** error (SQLITE_BUSY) is returned and the log connection is not closed.
** If successful, the EXCLUSIVE lock is not released before returning.
*/
int sqlite3PagerCloseWal(Pager *pPager) {
	/*
	 * TODO: Should we checkpoint here, or return SQLITE_BUSY?
	 * Berkeley DB always operates in WAL mode, so it's not a useful op.
	 */
	return (SQLITE_OK);
}

/*
** Return true if the underlying VFS for the given pager supports the
** primitives necessary for write-ahead logging.
*/
int sqlite3PagerWalSupported(Pager *pPager) {
	return (1);
}

#endif /* SQLITE_OMIT_WAL */

/*
** Parameter eStat must be either SQLITE_DBSTATUS_CACHE_HIT or
** SQLITE_DBSTATUS_CACHE_MISS. Before returning, *pnVal is incremented by the
** current cache hit or miss count, according to the value of eStat. If the 
** reset parameter is non-zero, the cache hit or miss count is zeroed before 
** returning.
*/
void sqlite3PagerCacheStat(Pager *pPager, int eStat, int reset, int *pnVal){
	Btree *p;
	BtShared *pBt;
	DB_MPOOL_STAT *mpstat;
	uintmax_t stat;

	p = (Btree *)pPager;
	pBt = p->pBt;

	assert(eStat == SQLITE_DBSTATUS_CACHE_HIT ||
	    eStat==SQLITE_DBSTATUS_CACHE_MISS);
	/*
	 * TODO: reset differs from SQLite here. We clear all mpool stats, they
	 * clear only the particular field queried.
	 */
	if (pBt->dbenv->memp_stat(
	    pBt->dbenv, &mpstat, NULL, reset ? DB_STAT_CLEAR : 0) != 0)
		return;
	if (eStat == SQLITE_DBSTATUS_CACHE_HIT)
		stat = mpstat->st_cache_hit;
	else
		stat = mpstat->st_cache_miss;
	*pnVal += (int)stat;
	sqlite3_free(mpstat);
}

#ifdef SQLITE_TEST
int sqlite3_pager_readdb_count = 0;    /* Number of full pages read from DB */
int sqlite3_pager_writedb_count = 0;   /* Number of full pages written to DB */
int sqlite3_pager_writej_count = 0;    /* Number of pages written to journal */
int sqlite3_opentemp_count = 0;
/*
** This routine is used for testing and analysis only.
** Some cheesy manipulation of the values in a is done so
** that the incrblob 2.* tests pass, even though auto_vacuum
** is not implemented for DB SQLITE.
*/
int *sqlite3PagerStats(Pager *pPager) {
	static int a[11];
	static int count = 0;
	if (count > 3) {
		a[9] = 4;
	} else {
		memset(&a[0], 0, sizeof(a));
		a[9] = 30;
		a[10] = 2;
	}
	count++;
	return a;
}

/* SQLite redefines sqlite3PagerAcquire for this implementation. */
int sqlite3PagerGet(Pager *pPager, Pgno pgno, DbPage **ppPage)
{
	return SQLITE_NOMEM;
}
void *sqlite3PagerGetData(DbPage *pPg)
{
	return NULL;
}
void sqlite3PagerUnref(DbPage *pPg)
{}
#endif

#ifdef ANDROID
SQLITE_API void sqlite3_get_pager_stats(sqlite3_int64 *totalBytesOut,
    sqlite3_int64 *referencedBytesOut, sqlite3_int64 *dbBytesOut,
    int *numPagersOut)
{
  *totalBytesOut = 0;
  *referencedBytesOut = 0;
  *dbBytesOut = 0;
  *numPagersOut = 0;
}
#endif
