#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 * 
 * locktimesid.d - Display lock wait times grouped by filename.
 *
 * This script graphs the time spent waiting for DB page locks.
 *
 * The optional integer maxcount parameter directs the script to exit once that
 * many page lock waits have been measured.
 *
 * usage: locktimes.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 *
 * The result times in nanoseconds are grouped by (filename, pgno, lock_mode).
 *
 * Static probes used:
 *	lock-suspend(DBT *lockobj, db_lockmode_t lock_ mode)
 *	lock-resume(DBT *lockobj, db_lockmode_t mode)
 *	db-open(char *file, char *db, uint32_t flags, uint8_t fileid[20])
 *	db-cursor(char *file, char *db, unsigned txnid, unsigned flags,
 *		uint8_t fileid[20])
 *
 *	The DBT locates the locked object in shared memory.  As long as
 *	application-specific locks are not being used, the DBT points to
 *	a DB_ILOCK:
 *		db_pgno_t	pgno;
 *		u_int8_t	fileid[20];
 *		u_int32_t	type;
 *	Type is 1 for handle, 2 for page, 3 for a queue's record number.
 *	The mode paramteris the enum db_lockmode_t
 *		notgranted,read,write,wait,iread,iwrite,iwr,read_unc,wwrite
 *
 *	This script may display unexpected results when application-specific
 *	locks are present.
 *
 */
#pragma D option defaultargs

typedef D`uint32_t db_pgno_t;

/* A DBT for the DB API */
typedef struct __db_dbt {
	uintptr_t	data;			/* Key/data */
	uint32_t	size;			/* key/data length */

	uint32_t	ulen;			/* RO: length of user buffer. */
	uint32_t	dlen;			/* RO: get/put record length. */
	uint32_t	doff;			/* RO: get/put record offset. */

	uintptr_t	app_data;
	uint32_t	flags;
} DBT;

/* A DBT in shared memory */

/*
 * DB fileids are actually uint8_t fileid[20]; in D it is easier to handle them
 * as a struct of 5 four-byte integers.
 */
typedef struct fileid
{
	uint32_t id1;
	uint32_t id2;
	uint32_t id3;
	uint32_t id4;
	uint32_t id5;
} FILEID;

typedef struct __db_ilock {
 	uint32_t	pgno;
 	FILEID		fileid;
	uint32_t	type;
} DB_ILOCK;

this DBT *dbt;
this DB_ILOCK *ilock;
this FILEID *fileid;
this string filename;

string filenames[unsigned, unsigned, unsigned, unsigned, unsigned];

self int suspend;

dtrace:::BEGIN
{
	maxcount = $1 != 0 ? $1 : -1;
	lockcount = 0;
	printf("DB lock wait times by (file, pgno) for process %d\n", $target);
	printf("Interrupt to display summary\n");
	modes[0] = "NOTGRANTED";
	modes[1] = "READ";
	modes[2] = "WRITE";
	modes[3] = "WAIT";
	modes[4] = "INTENT_WRITE";
	modes[5] = "INTENT_READ";
	modes[6] = "INTENT_WR";
	modes[7] = "READ_UNCOMMITTED";
	modes[8] = "WAS_WRITE";

}

/* lock-suspend(DBT *lockobj, int lock_mode) */
bdb$target:::lock-suspend
{
	self->suspend = timestamp;
}


/* lock-resume(DBT *lockobj, int lock_mode) */
bdb$target:::lock-resume
/self->suspend > 0/
{
	this->duration = timestamp - self->suspend;
	self->suspend = 0;
	this->dbt = copyin(arg0, sizeof(DBT));
	this->ilock = copyin(this->dbt->data, sizeof(DB_ILOCK));
	this->filename = filenames[this->ilock->fileid.id1,
	    this->ilock->fileid.id2, this->ilock->fileid.id3,
	    this->ilock->fileid.id4, this->ilock->fileid.id5];
	@locktimes[this->filename, this->ilock->pgno, modes[arg1]] =
	    quantize(this->duration);
	lockcount++;
}

bdb$target:::lock-resume
/lockcount == maxcount/
{
	exit(0);
}

/* db-open(char *file, char *db, uint32_t flags, uint8_t fileid[20])
 *
 * Watch db-open probes in order to get the fileid -> file name mapping.
 */
bdb$target:::db-open
/arg0 != 0/
{
	this->filename = copyinstr(arg0);
	this->fileid = (FILEID *) copyin(arg3, 20);
	filenames[this->fileid->id1, this->fileid->id2, this->fileid->id3,
	    this->fileid->id4, this->fileid->id5] = this->filename;
}

/* db-cursor(char *file, char *db, unsigned txnid, unsigned flags, uint8_t fileid[20])
 *
 * Watch cursor creation probes in order to get the fileid -> file name mapping.
 */
bdb$target:::db-cursor
/arg0 != 0/
{
	this->filename = (string) copyinstr(arg0);
	this->fileid = (FILEID *) copyin(arg4, 20);
	filenames[this->fileid->id1, this->fileid->id2, this->fileid->id3,
	    this->fileid->id4, this->fileid->id5] = this->filename;
}

dtrace:::END
{
	printa("Wait time for file %s page %u %s locks in nanoseconds %@a\n", @locktimes);
}
