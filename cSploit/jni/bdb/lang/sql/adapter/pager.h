/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include <db.h>

/* Avoid a name clash */
#undef DB_TIMEOUT

typedef struct Pager Pager;
typedef struct PgHdr DbPage;
typedef db_pgno_t Pgno;

/*
** Allowed values for the flags parameter to sqlite3PagerOpen().
**
** NOTE: These values must match the corresponding BTREE_ values in btree.h.
*/
#define	PAGER_OMIT_JOURNAL  0x0001    /* Do not use a rollback journal */
#define	PAGER_NO_READLOCK   0x0002    /* Omit readlocks on readonly files */

/*
** Valid values for the second argument to sqlite3PagerLockingMode().
*/
#define	PAGER_LOCKINGMODE_QUERY      -1
#define	PAGER_LOCKINGMODE_NORMAL      0
#define	PAGER_LOCKINGMODE_EXCLUSIVE   1

/*
** Valid values for the second argument to sqlite3PagerJournalMode().
*/
#define	PAGER_JOURNALMODE_QUERY      -1
#define	PAGER_JOURNALMODE_DELETE      0   /* Commit by deleting journal file */
#define	PAGER_JOURNALMODE_PERSIST     1   /* Commit by zeroing journal header */
#define	PAGER_JOURNALMODE_OFF         2   /* Journal omitted.  */
#define	PAGER_JOURNALMODE_TRUNCATE    3   /* Commit by truncating journal */
#define	PAGER_JOURNALMODE_MEMORY      4   /* In-memory journal file */
#define	PAGER_JOURNALMODE_WAL         5   /* Use write-ahead logging */

/*
** Default maximum size for log files. This value may be overridden using the
** sqlite3PagerJournalSizeLimit() API. See also "PRAGMA journal_size_limit".
*/
#ifndef SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT
  #define SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT 2 * 1024 * 1024
#endif

u8 sqlite3PagerIsreadonly(Pager*);
void sqlite3PagerShrink(Pager*);
void sqlite3PagerCacheStat(Pager *, int, int, int *);
int sqlite3PagerExclusiveLock(Pager *pPager);
sqlite3_file *sqlite3PagerFile(Pager*);
const char *sqlite3PagerFilename(Pager *pPager, int);
int sqlite3PagerGetJournalMode(Pager *pPager);
int sqlite3PagerIsMemdb(Pager*);
int sqlite3PagerJournalMode(Pager *, int);
i64 sqlite3PagerJournalSizeLimit(Pager *, i64);
int sqlite3PagerLockingMode(Pager *, int);
int sqlite3PagerMemUsed(Pager *pPager);
int sqlite3PagerOkToChangeJournalMode(Pager *pPager);
int sqlite3PagerPagecount(Pager*, int*);
int sqlite3PagerSetJournalMode(Pager *pPager, int eMode);

#ifndef SQLITE_OMIT_WAL
int sqlite3PagerWalCallback(Pager *pPager);
int sqlite3PagerCheckpoint(Pager *pPager);
int sqlite3PagerCloseWal(Pager *pPager);
int sqlite3PagerWalSupported(Pager *pPager);
#endif

#define	disable_simulated_io_errors()
#define	enable_simulated_io_errors()
#ifdef SQLITE_TEST
int *sqlite3PagerStats(Pager*);
#endif
