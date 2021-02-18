#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 * 
 * lockstimesi.d - Display lock wait times grouped by fileid.
 *
 * This script graphs the time spent waiting for DB page locks.
 *
 * The optional integer maxcount parameter directs the script to exit once that
 * many page lock waits have been measured.
 *
 * usage: locktimesid.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 *
 * The result times in nanoseconds are grouped by (fileid, pgno, lock_mode).
 *
 * Static probes used:
 *	lock-suspend(struct __db_dbt *, db_lockmode_t lock_ mode)
 *	lock-resume(struct __db_dbt *, db_lockmode_t lock_ mode)
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

typedef struct __db_lock_ilock {
 	uint32_t	pgno;
 	FILEID		fileid;
	uint32_t	type;
} DB_ILOCK;


self int suspend;
this DBT *dbt;
this DB_ILOCK *ilock;


dtrace:::BEGIN
{
	maxcount = $1 != 0 ? $1 : -1;
	lockcount = 0;
	printf("DB lock wait times grouped by (binary fileid, pgno, ");
	printf("lock_mode) for process %d;\ninterrupt to display summary\n", $target);
}

bdb$target:::lock-suspend
{
	self->suspend = timestamp;
}


/* lock-resume(DBT *lockobj, db_lockmode_t lock_mode) */
bdb$target:::lock-resume
/self->suspend > 0/
{
	this->duration = timestamp - self->suspend;
	self->suspend = 0;
	this->dbt = copyin(arg0, sizeof(struct __db_dbt));
	this->ilock = copyin(this->dbt->data, sizeof(DB_ILOCK));
	@locktimes[this->ilock->fileid, this->ilock->pgno, arg1] =
	    quantize(this->duration);
	lockcount++
}

bdb$target:::lock-resume
/lockcount == maxcount/
{
	exit(0);
}

dtrace:::END
{
	/* This example uses the default display format for @locktimes. */
}
