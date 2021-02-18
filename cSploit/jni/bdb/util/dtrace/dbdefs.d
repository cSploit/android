/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 *	dbdefs.d - DTrace declarations of DB data structures.
 */
#ifndef _DBDEFS_D
#define _DBDEFS_D

typedef D`uint32_t db_pgno_t;
typedef D`uint16_t db_indx_t;

typedef D`uintptr_t roff_t;
typedef D`int32_t db_atomic_t;

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
typedef struct __sh_dbt {
	uint32_t	size;			/* key/data length */
	roff_t		off;			/* Region offset. */
} SH_DBT;


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

typedef struct {
	uint32_t	file;
	uint32_t	offset;
} DB_LSN;

typedef struct {
	D`ssize_t stqe_next;	/* relative offset of next element */
	ssize_t stqe_prev;	/* relative offset of prev's next */
} SH_TAILQ_ENTRY;

typedef struct {
	ssize_t sce_next;	/* relative offset to next element */
	ssize_t sce_prev;	/* relative offset of prev element */
} SH_CHAIN_ENTRY;

typedef struct _db_page {
	DB_LSN	  lsn;		/* 00-07: Log sequence number. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	db_pgno_t prev_pgno;	/* 12-15: Previous page number. */
	db_pgno_t next_pgno;	/* 16-19: Next page number. */
	db_indx_t entries;	/* 20-21: Number of items on the page. */
	db_indx_t hf_offset;	/* 22-23: High free byte page offset. */

#define	LEAFLEVEL	  1
#define	MAXBTREELEVEL	255
	D`uint8_t   level;	/*    24: Btree tree level. */
	uint8_t   type;		/*    25: Page type. */
} PAGE;

typedef struct __bh {
	uint32_t	mtx_buf;	/* Shared/Exclusive mutex */
	db_atomic_t	ref;		/* Reference count. */
#define	BH_CALLPGIN	0x001		/* Convert the page before use. */
#define	BH_DIRTY	0x002		/* Page is modified. */
#define	BH_DIRTY_CREATE	0x004		/* Page is modified. */
#define	BH_DISCARD	0x008		/* Page is useless. */
#define	BH_EXCLUSIVE	0x010		/* Exclusive access acquired. */
#define	BH_FREED	0x020		/* Page was freed. */
#define	BH_FROZEN	0x040		/* Frozen buffer: allocate & re-read. */
#define	BH_TRASH	0x080		/* Page is garbage. */
#define	BH_THAWED	0x100		/* Page was thawed. */
	uint16_t	flags;

	uint32_t	priority;	/* Priority. */
	SH_TAILQ_ENTRY	hq;		/* MPOOL hash bucket queue. */

	db_pgno_t	pgno;		/* Underlying MPOOLFILE page number. */
	roff_t		mf_offset;	/* Associated MPOOLFILE offset. */
	uint32_t	bucket;		/* Hash bucket containing header. */
	int		region;		/* Region containing header. */

	roff_t		td_off;		/* MVCC: creating TXN_DETAIL offset. */
	SH_CHAIN_ENTRY	vc;		/* MVCC: version chain. */
#ifdef DIAG_MVCC
	uint16_t	align_off;	/* Alignment offset for diagnostics.*/
#endif

	/*
	 * !!!
	 * This array must be at least size_t aligned -- the DB access methods
	 * put PAGE and other structures into it, and then access them directly.
	 * (We guarantee size_t alignment to applications in the documentation,
	 * too.)
	 */
	uint8_t   	buf[1];		/* Variable length data. */
} BH;

typedef enum {
	DB_LOCK_NG=0,			/* Not granted. */
	DB_LOCK_READ=1,			/* Shared/read. */
	DB_LOCK_WRITE=2,		/* Exclusive/write. */
	DB_LOCK_WAIT=3,			/* Wait for event */
	DB_LOCK_IWRITE=4,		/* Intent exclusive/write. */
	DB_LOCK_IREAD=5,		/* Intent to share/read. */
	DB_LOCK_IWR=6,			/* Intent to read and write. */
	DB_LOCK_READ_UNCOMMITTED=7,	/* Degree 1 isolation. */
	DB_LOCK_WWRITE=8		/* Was Written. */
} db_lockmode_t;

#endif
