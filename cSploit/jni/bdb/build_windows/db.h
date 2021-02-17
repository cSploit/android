/* DO NOT EDIT: automatically built by dist/s_windows. */
/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 *
 * db.h include file layout:
 *	General.
 *	Database Environment.
 *	Locking subsystem.
 *	Logging subsystem.
 *	Shared buffer cache (mpool) subsystem.
 *	Transaction subsystem.
 *	Access methods.
 *	Access method cursors.
 *	Dbm/Ndbm, Hsearch historic interfaces.
 */

#ifndef _DB_H_
#define	_DB_H_

#ifndef	__NO_SYSTEM_INCLUDES
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#endif

/*
 * Turn off inappropriate compiler warnings
 */
#ifdef _MSC_VER
/*
 * This warning is explicitly disabled in Visual C++ by default.
 * It is necessary to explicitly enable the /Wall flag to generate this
 * warning.
 * Since this is a shared include file it should compile without warnings
 * at the highest warning level, so third party applications can use
 * higher warning levels cleanly.
 *
 * 4820: 'bytes' bytes padding added after member 'member'
 *       The type and order of elements caused the compiler to
 *       add padding to the end of a struct.
 */
#pragma warning(push)
#pragma warning(disable: 4820)
#endif /* _MSC_VER */
#if defined(__cplusplus)
extern "C" {
#endif


#undef __P
#define	__P(protos)	protos

/*
 * Berkeley DB version information.
 */
#define	DB_VERSION_FAMILY	12
#define	DB_VERSION_RELEASE	1
#define	DB_VERSION_MAJOR	6
#define	DB_VERSION_MINOR	0
#define	DB_VERSION_PATCH	30
#define	DB_VERSION_STRING	"Berkeley DB 6.0.30: (January 23, 2014)"
#define	DB_VERSION_FULL_STRING	"Berkeley DB 12c Release 1, library version 12.1.6.0.30: (January 23, 2014)"

/*
 * !!!
 * Berkeley DB uses specifically sized types.  If they're not provided by
 * the system, typedef them here.
 *
 * We protect them against multiple inclusion using __BIT_TYPES_DEFINED__,
 * as does BIND and Kerberos, since we don't know for sure what #include
 * files the user is using.
 *
 * !!!
 * We also provide the standard u_int, u_long etc., if they're not provided
 * by the system.
 */
#ifndef	__BIT_TYPES_DEFINED__
#define	__BIT_TYPES_DEFINED__
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 u_int64_t;
#endif

#ifndef _WINSOCKAPI_
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif
typedef unsigned short u_short;

/*
 * Missing ANSI types.
 *
 * uintmax_t --
 * Largest unsigned type, used to align structures in memory.  We don't store
 * floating point types in structures, so integral types should be sufficient
 * (and we don't have to worry about systems that store floats in other than
 * power-of-2 numbers of bytes).  Additionally this fixes compilers that rewrite
 * structure assignments and ANSI C memcpy calls to be in-line instructions
 * that happen to require alignment.
 *
 * uintptr_t --
 * Unsigned type that's the same size as a pointer.  There are places where
 * DB modifies pointers by discarding the bottom bits to guarantee alignment.
 * We can't use uintmax_t, it may be larger than the pointer, and compilers
 * get upset about that.  So far we haven't run on any machine where there's
 * no unsigned type the same size as a pointer -- here's hoping.
 */
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef u_int32_t uintmax_t;
#else
typedef u_int64_t uintmax_t;
#endif
#ifdef _WIN64
typedef u_int64_t uintptr_t;
#else
typedef u_int32_t uintptr_t;
#endif

/*
 * Windows defines off_t to long (i.e., 32 bits).  We need to pass 64-bit
 * file offsets, so we declare our own.
 */
#define	off_t	__db_off_t
typedef int64_t off_t;
typedef int64_t db_off_t;
typedef int32_t pid_t;
#ifdef HAVE_MIXED_SIZE_ADDRESSING
typedef u_int32_t db_size_t;
#else
typedef size_t db_size_t;
#endif
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
#ifdef HAVE_MIXED_SIZE_ADDRESSING
typedef int32_t db_ssize_t;
#else
typedef ssize_t db_ssize_t;
#endif

/*
 * Sequences are only available on machines with 64-bit integral types.
 */
typedef int64_t db_seq_t;

/* Thread and process identification. */
typedef u_int32_t db_threadid_t;

/* Basic types that are exported or quasi-exported. */
typedef	u_int32_t	db_pgno_t;	/* Page number type. */
typedef	u_int16_t	db_indx_t;	/* Page offset type. */
#define	DB_MAX_PAGES	0xffffffff	/* >= # of pages in a file */

typedef	u_int32_t	db_recno_t;	/* Record number type. */
#define	DB_MAX_RECORDS	0xffffffff	/* >= # of records in a tree */

typedef u_int32_t	db_timeout_t;	/* Type of a timeout. */

/*
 * Region offsets are the difference between a pointer in a region and the
 * region's base address.  With private environments, both addresses are the
 * result of calling malloc, and we can't assume anything about what malloc
 * will return, so region offsets have to be able to hold differences between
 * arbitrary pointers.
 */
typedef	db_size_t	roff_t;

/*
 * Forward structure declarations, so we can declare pointers and
 * applications can get type checking.
 */
struct __channel;	typedef struct __channel CHANNEL;
struct __db;		typedef struct __db DB;
struct __db_bt_stat;	typedef struct __db_bt_stat DB_BTREE_STAT;
struct __db_channel;	typedef struct __db_channel DB_CHANNEL;
struct __db_cipher;	typedef struct __db_cipher DB_CIPHER;
struct __db_compact;	typedef struct __db_compact DB_COMPACT;
struct __db_dbt;	typedef struct __db_dbt DBT;
struct __db_distab;	typedef struct __db_distab DB_DISTAB;
struct __db_env;	typedef struct __db_env DB_ENV;
struct __db_h_stat;	typedef struct __db_h_stat DB_HASH_STAT;
struct __db_heap_rid;	typedef struct __db_heap_rid DB_HEAP_RID;
struct __db_heap_stat;	typedef struct __db_heap_stat DB_HEAP_STAT;
struct __db_ilock;	typedef struct __db_ilock DB_LOCK_ILOCK;
struct __db_lock_hstat;	typedef struct __db_lock_hstat DB_LOCK_HSTAT;
struct __db_lock_pstat;	typedef struct __db_lock_pstat DB_LOCK_PSTAT;
struct __db_lock_stat;	typedef struct __db_lock_stat DB_LOCK_STAT;
struct __db_lock_u;	typedef struct __db_lock_u DB_LOCK;
struct __db_locker;	typedef struct __db_locker DB_LOCKER;
struct __db_lockreq;	typedef struct __db_lockreq DB_LOCKREQ;
struct __db_locktab;	typedef struct __db_locktab DB_LOCKTAB;
struct __db_log;	typedef struct __db_log DB_LOG;
struct __db_log_cursor;	typedef struct __db_log_cursor DB_LOGC;
struct __db_log_stat;	typedef struct __db_log_stat DB_LOG_STAT;
struct __db_lsn;	typedef struct __db_lsn DB_LSN;
struct __db_mpool;	typedef struct __db_mpool DB_MPOOL;
struct __db_mpool_fstat;typedef struct __db_mpool_fstat DB_MPOOL_FSTAT;
struct __db_mpool_stat;	typedef struct __db_mpool_stat DB_MPOOL_STAT;
struct __db_mpoolfile;	typedef struct __db_mpoolfile DB_MPOOLFILE;
struct __db_mutex_stat;	typedef struct __db_mutex_stat DB_MUTEX_STAT;
struct __db_mutex_t;	typedef struct __db_mutex_t DB_MUTEX;
struct __db_mutexmgr;	typedef struct __db_mutexmgr DB_MUTEXMGR;
struct __db_preplist;	typedef struct __db_preplist DB_PREPLIST;
struct __db_qam_stat;	typedef struct __db_qam_stat DB_QUEUE_STAT;
struct __db_rep;	typedef struct __db_rep DB_REP;
struct __db_rep_stat;	typedef struct __db_rep_stat DB_REP_STAT;
struct __db_repmgr_conn_err;
	typedef struct __db_repmgr_conn_err DB_REPMGR_CONN_ERR;
struct __db_repmgr_site;typedef struct __db_repmgr_site DB_REPMGR_SITE;
struct __db_repmgr_stat;typedef struct __db_repmgr_stat DB_REPMGR_STAT;
struct __db_seq_record; typedef struct __db_seq_record DB_SEQ_RECORD;
struct __db_seq_stat;	typedef struct __db_seq_stat DB_SEQUENCE_STAT;
struct __db_stream;	typedef struct __db_stream DB_STREAM;
struct __db_site;	typedef struct __db_site DB_SITE;
struct __db_sequence;	typedef struct __db_sequence DB_SEQUENCE;
struct __db_thread_info;typedef struct __db_thread_info DB_THREAD_INFO;
struct __db_txn;	typedef struct __db_txn DB_TXN;
struct __db_txn_active;	typedef struct __db_txn_active DB_TXN_ACTIVE;
struct __db_txn_stat;	typedef struct __db_txn_stat DB_TXN_STAT;
struct __db_txn_token;	typedef struct __db_txn_token DB_TXN_TOKEN;
struct __db_txnmgr;	typedef struct __db_txnmgr DB_TXNMGR;
struct __dbc;		typedef struct __dbc DBC;
struct __dbc_internal;	typedef struct __dbc_internal DBC_INTERNAL;
struct __env;		typedef struct __env ENV;
struct __fh_t;		typedef struct __fh_t DB_FH;
struct __fname;		typedef struct __fname FNAME;
struct __key_range;	typedef struct __key_range DB_KEY_RANGE;
struct __mpoolfile;	typedef struct __mpoolfile MPOOLFILE;
struct __db_logvrfy_config;
typedef struct __db_logvrfy_config DB_LOG_VERIFY_CONFIG;

/*
 * The Berkeley DB API flags are automatically-generated -- the following flag
 * names are no longer used, but remain for compatibility reasons.
 */
#define	DB_DEGREE_2	      DB_READ_COMMITTED
#define	DB_DIRTY_READ	      DB_READ_UNCOMMITTED
#define	DB_JOINENV	      0x0

/* Key/data structure -- a Data-Base Thang. */
struct __db_dbt {
	void	 *data;			/* Key/data */
	u_int32_t size;			/* key/data length */

	u_int32_t ulen;			/* RO: length of user buffer. */
	u_int32_t dlen;			/* RO: get/put record length. */
	u_int32_t doff;			/* RO: get/put record offset. */

	void *app_data;

#define	DB_DBT_APPMALLOC	0x0001	/* Callback allocated memory. */
#define	DB_DBT_BULK		0x0002	/* Internal: Insert if duplicate. */
#define	DB_DBT_DUPOK		0x0004	/* Internal: Insert if duplicate. */
#define	DB_DBT_ISSET		0x0008	/* Lower level calls set value. */
#define	DB_DBT_MALLOC		0x0010	/* Return in malloc'd memory. */
#define	DB_DBT_MULTIPLE		0x0020	/* References multiple records. */
#define	DB_DBT_PARTIAL		0x0040	/* Partial put/get. */
#define	DB_DBT_REALLOC		0x0080	/* Return in realloc'd memory. */
#define	DB_DBT_READONLY		0x0100	/* Readonly, don't update. */
#define	DB_DBT_STREAMING	0x0200	/* Internal: DBT is being streamed. */
#define	DB_DBT_USERCOPY		0x0400	/* Use the user-supplied callback. */
#define	DB_DBT_USERMEM		0x0800	/* Return in user's memory. */
#define	DB_DBT_BLOB		0x1000	/* Data item is a blob. */
#define	DB_DBT_BLOB_REC		0x2000	/* Internal: Blob database record. */
	u_int32_t flags;
};

/*******************************************************
 * Mutexes.
 *******************************************************/
/* 
 * When mixed size addressing is supported mutexes need to be the same size
 * independent of the process address size is.
 */
#ifdef HAVE_MIXED_SIZE_ADDRESSING
typedef db_size_t	db_mutex_t;
#else
typedef uintptr_t	db_mutex_t;
#endif

struct __db_mutex_stat { /* SHARED */
	/* The following fields are maintained in the region's copy. */
	u_int32_t st_mutex_align;	/* Mutex alignment */
	u_int32_t st_mutex_tas_spins;	/* Mutex test-and-set spins */
	u_int32_t st_mutex_init;	/* Initial mutex count */
	u_int32_t st_mutex_cnt;		/* Mutex count */
	u_int32_t st_mutex_max;		/* Mutex max */
	u_int32_t st_mutex_free;	/* Available mutexes */
	u_int32_t st_mutex_inuse;	/* Mutexes in use */
	u_int32_t st_mutex_inuse_max;	/* Maximum mutexes ever in use */

	/* The following fields are filled-in from other places. */
#ifndef __TEST_DB_NO_STATISTICS
	uintmax_t st_region_wait;	/* Region lock granted after wait. */
	uintmax_t st_region_nowait;	/* Region lock granted without wait. */
	roff_t	  st_regsize;		/* Region size. */
	roff_t	  st_regmax;		/* Region max. */
#endif
};

/* This is the length of the buffer passed to DB_ENV->thread_id_string() */
#define	DB_THREADID_STRLEN	128

/*******************************************************
 * Locking.
 *******************************************************/
#define	DB_LOCKVERSION	1

#define	DB_FILE_ID_LEN		20	/* Unique file ID length. */

/*
 * Deadlock detector modes; used in the DB_ENV structure to configure the
 * locking subsystem.
 */
#define	DB_LOCK_NORUN		0
#define	DB_LOCK_DEFAULT		1	/* Default policy. */
#define	DB_LOCK_EXPIRE		2	/* Only expire locks, no detection. */
#define	DB_LOCK_MAXLOCKS	3	/* Select locker with max locks. */
#define	DB_LOCK_MAXWRITE	4	/* Select locker with max writelocks. */
#define	DB_LOCK_MINLOCKS	5	/* Select locker with min locks. */
#define	DB_LOCK_MINWRITE	6	/* Select locker with min writelocks. */
#define	DB_LOCK_OLDEST		7	/* Select oldest locker. */
#define	DB_LOCK_RANDOM		8	/* Select random locker. */
#define	DB_LOCK_YOUNGEST	9	/* Select youngest locker. */

/*
 * Simple R/W lock modes and for multi-granularity intention locking.
 *
 * !!!
 * These values are NOT random, as they are used as an index into the lock
 * conflicts arrays, i.e., DB_LOCK_IWRITE must be == 3, and DB_LOCK_IREAD
 * must be == 4.
 */
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

/*
 * Request types.
 */
typedef enum {
	DB_LOCK_DUMP=0,			/* Display held locks. */
	DB_LOCK_GET=1,			/* Get the lock. */
	DB_LOCK_GET_TIMEOUT=2,		/* Get lock with a timeout. */
	DB_LOCK_INHERIT=3,		/* Pass locks to parent. */
	DB_LOCK_PUT=4,			/* Release the lock. */
	DB_LOCK_PUT_ALL=5,		/* Release locker's locks. */
	DB_LOCK_PUT_OBJ=6,		/* Release locker's locks on obj. */
	DB_LOCK_PUT_READ=7,		/* Release locker's read locks. */
	DB_LOCK_TIMEOUT=8,		/* Force a txn to timeout. */
	DB_LOCK_TRADE=9,		/* Trade locker ids on a lock. */
	DB_LOCK_UPGRADE_WRITE=10	/* Upgrade writes for dirty reads. */
} db_lockop_t;

/*
 * Status of a lock.
 */
typedef enum  {
	DB_LSTAT_ABORTED=1,		/* Lock belongs to an aborted txn. */
	DB_LSTAT_EXPIRED=2,		/* Lock has expired. */
	DB_LSTAT_FREE=3,		/* Lock is unallocated. */
	DB_LSTAT_HELD=4,		/* Lock is currently held. */
	DB_LSTAT_PENDING=5,		/* Lock was waiting and has been
					 * promoted; waiting for the owner
					 * to run and upgrade it to held. */
	DB_LSTAT_WAITING=6		/* Lock is on the wait queue. */
}db_status_t;

/* Lock statistics structure. */
struct __db_lock_stat { /* SHARED */
	u_int32_t st_id;		/* Last allocated locker ID. */
	u_int32_t st_cur_maxid;		/* Current maximum unused ID. */
	u_int32_t st_initlocks;		/* Initial number of locks in table. */
	u_int32_t st_initlockers;	/* Initial num of lockers in table. */
	u_int32_t st_initobjects;	/* Initial num of objects in table. */
	u_int32_t st_locks;		/* Current number of locks in table. */
	u_int32_t st_lockers;		/* Current num of lockers in table. */
	u_int32_t st_objects;		/* Current num of objects in table. */
	u_int32_t st_maxlocks;		/* Maximum number of locks in table. */
	u_int32_t st_maxlockers;	/* Maximum num of lockers in table. */
	u_int32_t st_maxobjects;	/* Maximum num of objects in table. */
	u_int32_t st_partitions;	/* number of partitions. */
	u_int32_t st_tablesize;		/* Size of object hash table. */
	int32_t   st_nmodes;		/* Number of lock modes. */
	u_int32_t st_nlockers;		/* Current number of lockers. */
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_nlocks;		/* Current number of locks. */
	u_int32_t st_maxnlocks;		/* Maximum number of locks so far. */
	u_int32_t st_maxhlocks;		/* Maximum number of locks in any bucket. */
	uintmax_t st_locksteals;	/* Number of lock steals so far. */
	uintmax_t st_maxlsteals;	/* Maximum number steals in any partition. */
	u_int32_t st_maxnlockers;	/* Maximum number of lockers so far. */
	u_int32_t st_nobjects;		/* Current number of objects. */
	u_int32_t st_maxnobjects;	/* Maximum number of objects so far. */
	u_int32_t st_maxhobjects;	/* Maximum number of objectsin any bucket. */
	uintmax_t st_objectsteals;	/* Number of objects steals so far. */
	uintmax_t st_maxosteals;	/* Maximum number of steals in any partition. */
	uintmax_t st_nrequests;		/* Number of lock gets. */
	uintmax_t st_nreleases;		/* Number of lock puts. */
	uintmax_t st_nupgrade;		/* Number of lock upgrades. */
	uintmax_t st_ndowngrade;	/* Number of lock downgrades. */
	uintmax_t st_lock_wait;		/* Lock conflicts w/ subsequent wait */
	uintmax_t st_lock_nowait;	/* Lock conflicts w/o subsequent wait */
	uintmax_t st_ndeadlocks;	/* Number of lock deadlocks. */
	db_timeout_t st_locktimeout;	/* Lock timeout. */
	uintmax_t st_nlocktimeouts;	/* Number of lock timeouts. */
	db_timeout_t st_txntimeout;	/* Transaction timeout. */
	uintmax_t st_ntxntimeouts;	/* Number of transaction timeouts. */
	uintmax_t st_part_wait;		/* Partition lock granted after wait. */
	uintmax_t st_part_nowait;	/* Partition lock granted without wait. */
	uintmax_t st_part_max_wait;	/* Max partition lock granted after wait. */
	uintmax_t st_part_max_nowait;	/* Max partition lock granted without wait. */
	uintmax_t st_objs_wait;	/* 	Object lock granted after wait. */
	uintmax_t st_objs_nowait;	/* Object lock granted without wait. */
	uintmax_t st_lockers_wait;	/* Locker lock granted after wait. */
	uintmax_t st_lockers_nowait;	/* Locker lock granted without wait. */
	uintmax_t st_region_wait;	/* Region lock granted after wait. */
	uintmax_t st_region_nowait;	/* Region lock granted without wait. */
	uintmax_t st_nlockers_hit;	/* Lockers found in thread info. */
	uintmax_t st_nlockers_reused;	/* Lockers reallocated from thread info. */
	u_int32_t st_hash_len;		/* Max length of bucket. */
	roff_t	  st_regsize;		/* Region size. */
#endif
};

struct __db_lock_hstat { /* SHARED */
	uintmax_t st_nrequests;		/* Number of lock gets. */
	uintmax_t st_nreleases;		/* Number of lock puts. */
	uintmax_t st_nupgrade;		/* Number of lock upgrades. */
	uintmax_t st_ndowngrade;	/* Number of lock downgrades. */
	u_int32_t st_nlocks;		/* Current number of locks. */
	u_int32_t st_maxnlocks;		/* Maximum number of locks so far. */
	u_int32_t st_nobjects;		/* Current number of objects. */
	u_int32_t st_maxnobjects;	/* Maximum number of objects so far. */
	uintmax_t st_lock_wait;		/* Lock conflicts w/ subsequent wait */
	uintmax_t st_lock_nowait;	/* Lock conflicts w/o subsequent wait */
	uintmax_t st_nlocktimeouts;	/* Number of lock timeouts. */
	uintmax_t st_ntxntimeouts;	/* Number of transaction timeouts. */
	u_int32_t st_hash_len;		/* Max length of bucket. */
};

struct __db_lock_pstat { /* SHARED */
	u_int32_t st_nlocks;		/* Current number of locks. */
	u_int32_t st_maxnlocks;		/* Maximum number of locks so far. */
	u_int32_t st_nobjects;		/* Current number of objects. */
	u_int32_t st_maxnobjects;	/* Maximum number of objects so far. */
	uintmax_t st_locksteals;	/* Number of lock steals so far. */
	uintmax_t st_objectsteals;	/* Number of objects steals so far. */
};

/*
 * DB_LOCK_ILOCK --
 *	Internal DB access method lock.
 */
struct __db_ilock { /* SHARED */
	db_pgno_t pgno;			/* Page being locked. */
	u_int8_t fileid[DB_FILE_ID_LEN];/* File id. */
#define	DB_HANDLE_LOCK		1
#define	DB_RECORD_LOCK		2
#define	DB_PAGE_LOCK		3
#define	DB_DATABASE_LOCK	4
	u_int32_t type;			/* Type of lock. */
};

/*
 * DB_LOCK --
 *	The structure is allocated by the caller and filled in during a
 *	lock_get request (or a lock_vec/DB_LOCK_GET).
 */
struct __db_lock_u { /* SHARED */
	roff_t		off;		/* Offset of the lock in the region */
	u_int32_t	ndx;		/* Index of the object referenced by
					 * this lock; used for locking. */
	u_int32_t	gen;		/* Generation number of this lock. */
	db_lockmode_t	mode;		/* mode of this lock. */
};

/* Lock request structure. */
struct __db_lockreq {
	db_lockop_t	 op;		/* Operation. */
	db_lockmode_t	 mode;		/* Requested mode. */
	db_timeout_t	 timeout;	/* Time to expire lock. */
	DBT		*obj;		/* Object being locked. */
	DB_LOCK		 lock;		/* Lock returned. */
};

/*******************************************************
 * Logging.
 *******************************************************/
#define	DB_LOGVERSION	21		/* Current log version. */
#define	DB_LOGVERSION_LATCHING 15	/* Log version using latching: db-4.8 */
#define	DB_LOGCHKSUM	12		/* Check sum headers: db-4.5 */
#define	DB_LOGOLDVER	8		/* Oldest version supported: db-4.2 */
#define	DB_LOGMAGIC	0x040988

/*
 * A DB_LSN has two parts, a fileid which identifies a specific file, and an
 * offset within that file.  The fileid is an unsigned 4-byte quantity that
 * uniquely identifies a file within the log directory -- currently a simple
 * counter inside the log.  The offset is also an unsigned 4-byte value.  The
 * log manager guarantees the offset is never more than 4 bytes by switching
 * to a new log file before the maximum length imposed by an unsigned 4-byte
 * offset is reached.
 */
struct __db_lsn { /* SHARED */
	u_int32_t	file;		/* File ID. */
	u_int32_t	offset;		/* File offset. */
};

/*
 * Application-specified log record types start at DB_user_BEGIN, and must not
 * equal or exceed DB_debug_FLAG.
 *
 * DB_debug_FLAG is the high-bit of the u_int32_t that specifies a log record
 * type.  If the flag is set, it's a log record that was logged for debugging
 * purposes only, even if it reflects a database change -- the change was part
 * of a non-durable transaction.
 */
#define	DB_user_BEGIN		10000
#define	DB_debug_FLAG		0x80000000

/*
 * DB_LOGC --
 *	Log cursor.
 */
struct __db_log_cursor {
	ENV	 *env;			/* Environment */

	DB_FH	 *fhp;			/* File handle. */
	DB_LSN	  lsn;			/* Cursor: LSN */
	u_int32_t len;			/* Cursor: record length */
	u_int32_t prev;			/* Cursor: previous record's offset */

	DBT	  dbt;			/* Return DBT. */
	DB_LSN    p_lsn;		/* Persist LSN. */
	u_int32_t p_version;		/* Persist version. */

	u_int8_t *bp;			/* Allocated read buffer. */
	u_int32_t bp_size;		/* Read buffer length in bytes. */
	u_int32_t bp_rlen;		/* Read buffer valid data length. */
	DB_LSN	  bp_lsn;		/* Read buffer first byte LSN. */

	u_int32_t bp_maxrec;		/* Max record length in the log file. */

	/* DB_LOGC PUBLIC HANDLE LIST BEGIN */
	int (*close) __P((DB_LOGC *, u_int32_t));
	int (*get) __P((DB_LOGC *, DB_LSN *, DBT *, u_int32_t));
	int (*version) __P((DB_LOGC *, u_int32_t *, u_int32_t));
	/* DB_LOGC PUBLIC HANDLE LIST END */

#define	DB_LOG_DISK		0x01	/* Log record came from disk. */
#define	DB_LOG_LOCKED		0x02	/* Log region already locked */
#define	DB_LOG_SILENT_ERR	0x04	/* Turn-off error messages. */
	u_int32_t flags;
};

/* Log statistics structure. */
struct __db_log_stat { /* SHARED */
	u_int32_t st_magic;		/* Log file magic number. */
	u_int32_t st_version;		/* Log file version number. */
	int32_t   st_mode;		/* Log file permissions mode. */
	u_int32_t st_lg_bsize;		/* Log buffer size. */
	u_int32_t st_lg_size;		/* Log file size. */
	u_int32_t st_wc_bytes;		/* Bytes to log since checkpoint. */
	u_int32_t st_wc_mbytes;		/* Megabytes to log since checkpoint. */
	u_int32_t st_fileid_init;	/* Initial allocation for fileids. */
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_nfileid;		/* Current number of fileids. */
	u_int32_t st_maxnfileid;	/* Maximum number of fileids used. */
	uintmax_t st_record;		/* Records entered into the log. */
	u_int32_t st_w_bytes;		/* Bytes to log. */
	u_int32_t st_w_mbytes;		/* Megabytes to log. */
	uintmax_t st_wcount;		/* Total I/O writes to the log. */
	uintmax_t st_wcount_fill;	/* Overflow writes to the log. */
	uintmax_t st_rcount;		/* Total I/O reads from the log. */
	uintmax_t st_scount;		/* Total syncs to the log. */
	uintmax_t st_region_wait;	/* Region lock granted after wait. */
	uintmax_t st_region_nowait;	/* Region lock granted without wait. */
	u_int32_t st_cur_file;		/* Current log file number. */
	u_int32_t st_cur_offset;	/* Current log file offset. */
	u_int32_t st_disk_file;		/* Known on disk log file number. */
	u_int32_t st_disk_offset;	/* Known on disk log file offset. */
	u_int32_t st_maxcommitperflush;	/* Max number of commits in a flush. */
	u_int32_t st_mincommitperflush;	/* Min number of commits in a flush. */
	roff_t	  st_regsize;		/* Region size. */
#endif
};

/*
 * We need to record the first log record of a transaction.  For user
 * defined logging this macro returns the place to put that information,
 * if it is need in rlsnp, otherwise it leaves it unchanged.  We also
 * need to track the last record of the transaction, this returns the
 * place to put that info.
 */
#define	DB_SET_TXN_LSNP(txn, blsnp, llsnp)		\
	((txn)->set_txn_lsnp(txn, blsnp, llsnp))

/*
 * Definition of the structure which specifies marshalling of log records.
 */
typedef enum {
	LOGREC_Done,
	LOGREC_ARG,
	LOGREC_HDR,
	LOGREC_DATA,
	LOGREC_DB,
	LOGREC_DBOP,
	LOGREC_DBT,
	LOGREC_LOCKS,
	LOGREC_OP,
	LOGREC_PGDBT,
	LOGREC_PGDDBT,
	LOGREC_PGLIST,
	LOGREC_POINTER,
	LOGREC_TIME,
	LOGREC_LONGARG
} log_rec_type_t;

typedef const struct __log_rec_spec {
	log_rec_type_t	type;
	u_int32_t	offset;
	const char 	*name;
	const char	fmt[4];
} DB_LOG_RECSPEC;

/*
 * Size of a DBT in a log record.
 */
#define	LOG_DBT_SIZE(dbt)						\
    (sizeof(u_int32_t) + ((dbt) == NULL ? 0 : (dbt)->size))

/*******************************************************
 * Shared buffer cache (mpool).
 *******************************************************/
/* Priority values for DB_MPOOLFILE->{put,set_priority}. */
typedef enum {
	DB_PRIORITY_UNCHANGED=0,
	DB_PRIORITY_VERY_LOW=1,
	DB_PRIORITY_LOW=2,
	DB_PRIORITY_DEFAULT=3,
	DB_PRIORITY_HIGH=4,
	DB_PRIORITY_VERY_HIGH=5
} DB_CACHE_PRIORITY;

/* Per-process DB_MPOOLFILE information. */
struct __db_mpoolfile {
	DB_FH	  *fhp;			/* Underlying file handle. */

	/*
	 * !!!
	 * The ref, pinref and q fields are protected by the region lock.
	 */
	u_int32_t  ref;			/* Reference count. */

	u_int32_t pinref;		/* Pinned block reference count. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_mpoolfile) q;
	 */
	struct {
		struct __db_mpoolfile *tqe_next;
		struct __db_mpoolfile **tqe_prev;
	} q;				/* Linked list of DB_MPOOLFILE's. */

	/*
	 * !!!
	 * The rest of the fields (with the exception of the MP_FLUSH flag)
	 * are not thread-protected, even when they may be modified at any
	 * time by the application.  The reason is the DB_MPOOLFILE handle
	 * is single-threaded from the viewpoint of the application, and so
	 * the only fields needing to be thread-protected are those accessed
	 * by checkpoint or sync threads when using DB_MPOOLFILE structures
	 * to flush buffers from the cache.
	 */
	ENV	       *env;		/* Environment */
	MPOOLFILE      *mfp;		/* Underlying MPOOLFILE. */

	u_int32_t	clear_len;	/* Cleared length on created pages. */
	u_int8_t			/* Unique file ID. */
			fileid[DB_FILE_ID_LEN];
	int		ftype;		/* File type. */
	int32_t		lsn_offset;	/* LSN offset in page. */
	u_int32_t	gbytes, bytes;	/* Maximum file size. */
	DBT	       *pgcookie;	/* Byte-string passed to pgin/pgout. */
	int32_t		priority;	/* Cache priority. */

	void	       *addr;		/* Address of mmap'd region. */
	size_t		len;		/* Length of mmap'd region. */

	u_int32_t	config_flags;	/* Flags to DB_MPOOLFILE->set_flags. */

	/* DB_MPOOLFILE PUBLIC HANDLE LIST BEGIN */
	int (*close) __P((DB_MPOOLFILE *, u_int32_t));
	int (*get)
	    __P((DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, u_int32_t, void *));
	int (*get_clear_len) __P((DB_MPOOLFILE *, u_int32_t *));
	int (*get_fileid) __P((DB_MPOOLFILE *, u_int8_t *));
	int (*get_flags) __P((DB_MPOOLFILE *, u_int32_t *));
	int (*get_ftype) __P((DB_MPOOLFILE *, int *));
	int (*get_last_pgno) __P((DB_MPOOLFILE *, db_pgno_t *));
	int (*get_lsn_offset) __P((DB_MPOOLFILE *, int32_t *));
	int (*get_maxsize) __P((DB_MPOOLFILE *, u_int32_t *, u_int32_t *));
	int (*get_pgcookie) __P((DB_MPOOLFILE *, DBT *));
	int (*get_priority) __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY *));
	int (*open) __P((DB_MPOOLFILE *, const char *, u_int32_t, int, size_t));
	int (*put) __P((DB_MPOOLFILE *, void *, DB_CACHE_PRIORITY, u_int32_t));
	int (*set_clear_len) __P((DB_MPOOLFILE *, u_int32_t));
	int (*set_fileid) __P((DB_MPOOLFILE *, u_int8_t *));
	int (*set_flags) __P((DB_MPOOLFILE *, u_int32_t, int));
	int (*set_ftype) __P((DB_MPOOLFILE *, int));
	int (*set_lsn_offset) __P((DB_MPOOLFILE *, int32_t));
	int (*set_maxsize) __P((DB_MPOOLFILE *, u_int32_t, u_int32_t));
	int (*set_pgcookie) __P((DB_MPOOLFILE *, DBT *));
	int (*set_priority) __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY));
	int (*sync) __P((DB_MPOOLFILE *));
	/* DB_MPOOLFILE PUBLIC HANDLE LIST END */

	/*
	 * MP_FILEID_SET, MP_OPEN_CALLED and MP_READONLY do not need to be
	 * thread protected because they are initialized before the file is
	 * linked onto the per-process lists, and never modified.
	 *
	 * MP_FLUSH is thread protected because it is potentially read/set by
	 * multiple threads of control.
	 */
#define	MP_FILEID_SET	0x001		/* Application supplied a file ID. */
#define	MP_FLUSH	0x002		/* Was used to flush a buffer. */
#define	MP_FOR_FLUSH	0x004		/* Was opened to flush a buffer. */
#define	MP_MULTIVERSION	0x008		/* Opened for multiversion access. */
#define	MP_OPEN_CALLED	0x010		/* File opened. */
#define	MP_READONLY	0x020		/* File is readonly. */
#define	MP_DUMMY	0x040		/* File is dummy for __memp_fput. */
	u_int32_t  flags;
};

/* Mpool statistics structure. */
struct __db_mpool_stat { /* SHARED */
	u_int32_t st_gbytes;		/* Total cache size: GB. */
	u_int32_t st_bytes;		/* Total cache size: B. */
	u_int32_t st_ncache;		/* Number of cache regions. */
	u_int32_t st_max_ncache;	/* Maximum number of regions. */
	db_size_t st_mmapsize;		/* Maximum file size for mmap. */
	int32_t st_maxopenfd;		/* Maximum number of open fd's. */
	int32_t st_maxwrite;		/* Maximum buffers to write. */
	db_timeout_t st_maxwrite_sleep;	/* Sleep after writing max buffers. */
	u_int32_t st_pages;		/* Total number of pages. */
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_map;		/* Pages from mapped files. */
	uintmax_t st_cache_hit;	/* Pages found in the cache. */
	uintmax_t st_cache_miss;	/* Pages not found in the cache. */
	uintmax_t st_page_create;	/* Pages created in the cache. */
	uintmax_t st_page_in;		/* Pages read in. */
	uintmax_t st_page_out;		/* Pages written out. */
	uintmax_t st_ro_evict;		/* Clean pages forced from the cache. */
	uintmax_t st_rw_evict;		/* Dirty pages forced from the cache. */
	uintmax_t st_page_trickle;	/* Pages written by memp_trickle. */
	u_int32_t st_page_clean;	/* Clean pages. */
	u_int32_t st_page_dirty;	/* Dirty pages. */
	u_int32_t st_hash_buckets;	/* Number of hash buckets. */
	u_int32_t st_hash_mutexes;	/* Number of hash bucket mutexes. */
	u_int32_t st_pagesize;		/* Assumed page size. */
	u_int32_t st_hash_searches;	/* Total hash chain searches. */
	u_int32_t st_hash_longest;	/* Longest hash chain searched. */
	uintmax_t st_hash_examined;	/* Total hash entries searched. */
	uintmax_t st_hash_nowait;	/* Hash lock granted with nowait. */
	uintmax_t st_hash_wait;		/* Hash lock granted after wait. */
	uintmax_t st_hash_max_nowait;	/* Max hash lock granted with nowait. */
	uintmax_t st_hash_max_wait;	/* Max hash lock granted after wait. */
	uintmax_t st_region_nowait;	/* Region lock granted with nowait. */
	uintmax_t st_region_wait;	/* Region lock granted after wait. */
	uintmax_t st_mvcc_frozen;	/* Buffers frozen. */
	uintmax_t st_mvcc_thawed;	/* Buffers thawed. */
	uintmax_t st_mvcc_freed;	/* Frozen buffers freed. */
	uintmax_t st_mvcc_reused;	/* Outdated invisible buffers reused. */
	uintmax_t st_alloc;		/* Number of page allocations. */
	uintmax_t st_alloc_buckets;	/* Buckets checked during allocation. */
	uintmax_t st_alloc_max_buckets;/* Max checked during allocation. */
	uintmax_t st_alloc_pages;	/* Pages checked during allocation. */
	uintmax_t st_alloc_max_pages;	/* Max checked during allocation. */
	uintmax_t st_io_wait;		/* Thread waited on buffer I/O. */
	uintmax_t st_sync_interrupted;	/* Number of times sync interrupted. */
	u_int32_t st_oddfsize_detect;	/* Odd file size detected. */
	u_int32_t st_oddfsize_resolve;	/* Odd file size resolved. */
	roff_t	  st_regsize;		/* Region size. */
	roff_t	  st_regmax;		/* Region max. */
#endif
};

/*
 * Mpool file statistics structure.
 * The first fields in this structure must mirror the __db_mpool_fstat_int
 * structure, since content is mem copied between the two.
 */
struct __db_mpool_fstat {
	u_int32_t st_pagesize;		/* Page size. */
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_map;		/* Pages from mapped files. */
	uintmax_t st_cache_hit;	/* Pages found in the cache. */
	uintmax_t st_cache_miss;	/* Pages not found in the cache. */
	uintmax_t st_page_create;	/* Pages created in the cache. */
	uintmax_t st_page_in;		/* Pages read in. */
	uintmax_t st_page_out;		/* Pages written out. */
	uintmax_t st_backup_spins;	/* Number of spins during a copy. */
#endif
	char *file_name;	/* File name. */
};

/*******************************************************
 * Transactions and recovery.
 *******************************************************/
#define	DB_TXNVERSION	1

typedef enum {
	DB_TXN_ABORT=0,			/* Public. */
	DB_TXN_APPLY=1,			/* Public. */
	DB_TXN_BACKWARD_ROLL=3,		/* Public. */
	DB_TXN_FORWARD_ROLL=4,		/* Public. */
	DB_TXN_OPENFILES=5,		/* Internal. */
	DB_TXN_POPENFILES=6,		/* Internal. */
	DB_TXN_PRINT=7,			/* Public. */
	DB_TXN_LOG_VERIFY=8		/* Internal. */
} db_recops;

/*
 * BACKWARD_ALLOC is used during the forward pass to pick up any aborted
 * allocations for files that were created during the forward pass.
 * The main difference between _ALLOC and _ROLL is that the entry for
 * the file not exist during the rollforward pass.
 */
#define	DB_UNDO(op)	((op) == DB_TXN_ABORT || (op) == DB_TXN_BACKWARD_ROLL)
#define	DB_REDO(op)	((op) == DB_TXN_FORWARD_ROLL || (op) == DB_TXN_APPLY)

struct __db_txn {
	DB_TXNMGR	*mgrp;		/* Pointer to transaction manager. */
	DB_TXN		*parent;	/* Pointer to transaction's parent. */
	DB_THREAD_INFO	*thread_info;	/* Pointer to thread information. */

	u_int32_t	txnid;		/* Unique transaction id. */
	char		*name;		/* Transaction name. */
	DB_LOCKER	*locker;	/* Locker for this txn. */

	void		*td;		/* Detail structure within region. */
	db_timeout_t	lock_timeout;	/* Timeout for locks for this txn. */
	void		*txn_list;	/* Undo information for parent. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_txn) links;
	 */
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} links;			/* Links transactions off manager. */

	/*
	 * !!!
	 * Explicit representations of structures from shqueue.h.
	 * SH_TAILQ_ENTRY xa_links;
	 * These links link together transactions that are active in
	 * the same thread of control.
	 */
	struct {
		db_ssize_t stqe_next;
		db_ssize_t stqe_prev;
	} xa_links;			/* Links XA transactions. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__kids, __db_txn) kids;
	 */
	struct __kids {
		struct __db_txn *tqh_first;
		struct __db_txn **tqh_last;
	} kids;

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__events, __txn_event) events;
	 */
	struct {
		struct __txn_event *tqh_first;
		struct __txn_event **tqh_last;
	} events;			/* Links deferred events. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * STAILQ_HEAD(__logrec, __txn_logrec) logs;
	 */
	struct {
		struct __txn_logrec *stqh_first;
		struct __txn_logrec **stqh_last;
	} logs;				/* Links in memory log records. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_txn) klinks;
	 */
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} klinks;			/* Links of children in parent. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__my_cursors, __dbc) my_cursors;
	 */
	struct __my_cursors {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} my_cursors;

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__femfs, MPOOLFILE) femfs;
	 *
	 * These are DBs involved in file extension in this transaction.
	 */
	struct __femfs {
		DB *tqh_first;
		DB **tqh_last;
	} femfs;

	DB_TXN_TOKEN	*token_buffer;	/* User's commit token buffer. */
	void	*api_internal;		/* C++ API private. */
	void	*xml_internal;		/* XML API private. */

	u_int32_t	cursors;	/* Number of cursors open for txn */

	/* DB_TXN PUBLIC HANDLE LIST BEGIN */
	int	  (*abort) __P((DB_TXN *));
	int	  (*commit) __P((DB_TXN *, u_int32_t));
	int	  (*discard) __P((DB_TXN *, u_int32_t));
	int	  (*get_name) __P((DB_TXN *, const char **));
	int	  (*get_priority) __P((DB_TXN *, u_int32_t *));
	u_int32_t (*id) __P((DB_TXN *));
	int	  (*prepare) __P((DB_TXN *, u_int8_t *));
	int	  (*set_commit_token) __P((DB_TXN *, DB_TXN_TOKEN *));
	int	  (*set_name) __P((DB_TXN *, const char *));
	int	  (*set_priority) __P((DB_TXN *, u_int32_t));
	int	  (*set_timeout) __P((DB_TXN *, db_timeout_t, u_int32_t));
	/* DB_TXN PUBLIC HANDLE LIST END */

	/* DB_TXN PRIVATE HANDLE LIST BEGIN */
	void	  (*set_txn_lsnp) __P((DB_TXN *txn, DB_LSN **, DB_LSN **));
	/* DB_TXN PRIVATE HANDLE LIST END */

#define	TXN_XA_THREAD_NOTA		0
#define	TXN_XA_THREAD_ASSOCIATED	1
#define	TXN_XA_THREAD_SUSPENDED		2
#define	TXN_XA_THREAD_UNASSOCIATED 	3
	u_int32_t	xa_thr_status;

#define	TXN_CHILDCOMMIT		0x00001	/* Txn has committed. */
#define	TXN_COMPENSATE		0x00002	/* Compensating transaction. */
#define	TXN_DEADLOCK		0x00004	/* Txn has deadlocked. */
#define	TXN_FAMILY		0x00008	/* Cursors/children are independent. */
#define	TXN_IGNORE_LEASE	0x00010	/* Skip lease check at commit time. */
#define	TXN_INFAMILY		0x00020	/* Part of a transaction family. */
#define	TXN_LOCKTIMEOUT		0x00040	/* Txn has a lock timeout. */
#define	TXN_MALLOC		0x00080	/* Structure allocated by TXN system. */
#define	TXN_NOSYNC		0x00100	/* Do not sync on prepare and commit. */
#define	TXN_NOWAIT		0x00200	/* Do not wait on locks. */
#define	TXN_PRIVATE		0x00400	/* Txn owned by cursor. */
#define	TXN_READONLY		0x00800	/* CDS group handle. */
#define	TXN_READ_COMMITTED	0x01000	/* Txn has degree 2 isolation. */
#define	TXN_READ_UNCOMMITTED	0x02000	/* Txn has degree 1 isolation. */
#define	TXN_RESTORED		0x04000	/* Txn has been restored. */
#define	TXN_SNAPSHOT		0x08000	/* Snapshot Isolation. */
#define	TXN_SYNC		0x10000	/* Write and sync on prepare/commit. */
#define	TXN_WRITE_NOSYNC	0x20000	/* Write only on prepare/commit. */
#define TXN_BULK		0x40000 /* Enable bulk loading optimization. */
	u_int32_t	flags;
};

#define	TXN_SYNC_FLAGS (TXN_SYNC | TXN_NOSYNC | TXN_WRITE_NOSYNC)

/*
 * Structure used for two phase commit interface.
 * We set the size of our global transaction id (gid) to be 128 in order
 * to match that defined by the XA X/Open standard.
 */
#define	DB_GID_SIZE	128
struct __db_preplist {
	DB_TXN	*txn;
	u_int8_t gid[DB_GID_SIZE];
};

/* Transaction statistics structure. */
struct __db_txn_active {
	u_int32_t txnid;		/* Transaction ID */
	u_int32_t parentid;		/* Transaction ID of parent */
	pid_t     pid;			/* Process owning txn ID */
	db_threadid_t tid;		/* Thread owning txn ID */

	DB_LSN	  lsn;			/* LSN when transaction began */

	DB_LSN	  read_lsn;		/* Read LSN for MVCC */
	u_int32_t mvcc_ref;		/* MVCC reference count */

	u_int32_t priority;		/* Deadlock resolution priority */

#define	TXN_ABORTED		1
#define	TXN_COMMITTED		2
#define	TXN_NEED_ABORT		3
#define	TXN_PREPARED		4
#define	TXN_RUNNING		5
	u_int32_t status;		/* Status of the transaction */

#define	TXN_XA_ACTIVE		1
#define	TXN_XA_DEADLOCKED	2
#define	TXN_XA_IDLE		3
#define	TXN_XA_PREPARED		4
#define	TXN_XA_ROLLEDBACK	5
	u_int32_t xa_status;		/* XA status */

	u_int8_t  gid[DB_GID_SIZE];	/* Global transaction ID */
	char	  name[51];		/* 50 bytes of name, nul termination */
};

struct __db_txn_stat {
	u_int32_t st_nrestores;		/* number of restored transactions
					   after recovery. */
#ifndef __TEST_DB_NO_STATISTICS
	DB_LSN	  st_last_ckp;		/* lsn of the last checkpoint */
	time_t	  st_time_ckp;		/* time of last checkpoint */
	u_int32_t st_last_txnid;	/* last transaction id given out */
	u_int32_t st_inittxns;		/* inital txns allocated */
	u_int32_t st_maxtxns;		/* maximum txns possible */
	uintmax_t st_naborts;		/* number of aborted transactions */
	uintmax_t st_nbegins;		/* number of begun transactions */
	uintmax_t st_ncommits;		/* number of committed transactions */
	u_int32_t st_nactive;		/* number of active transactions */
	u_int32_t st_nsnapshot;		/* number of snapshot transactions */
	u_int32_t st_maxnactive;	/* maximum active transactions */
	u_int32_t st_maxnsnapshot;	/* maximum snapshot transactions */
	uintmax_t st_region_wait;	/* Region lock granted after wait. */
	uintmax_t st_region_nowait;	/* Region lock granted without wait. */
	roff_t	  st_regsize;		/* Region size. */
	DB_TXN_ACTIVE *st_txnarray;	/* array of active transactions */
#endif
};

#define	DB_TXN_TOKEN_SIZE		20
struct __db_txn_token {
	u_int8_t buf[DB_TXN_TOKEN_SIZE];
};

/*******************************************************
 * Replication.
 *******************************************************/
/* Special, out-of-band environment IDs. */
#define	DB_EID_BROADCAST	-1
#define	DB_EID_INVALID		-2
#define	DB_EID_MASTER		-3

#define	DB_REP_DEFAULT_PRIORITY		100

/* Acknowledgement policies; 0 reserved as OOB. */
#define	DB_REPMGR_ACKS_ALL		1
#define	DB_REPMGR_ACKS_ALL_AVAILABLE	2
#define	DB_REPMGR_ACKS_ALL_PEERS	3
#define	DB_REPMGR_ACKS_NONE		4
#define	DB_REPMGR_ACKS_ONE		5
#define	DB_REPMGR_ACKS_ONE_PEER		6
#define	DB_REPMGR_ACKS_QUORUM		7

/* Replication timeout configuration values. */
#define	DB_REP_ACK_TIMEOUT		1	/* RepMgr acknowledgements. */
#define	DB_REP_CHECKPOINT_DELAY		2	/* Master checkpoint delay. */
#define	DB_REP_CONNECTION_RETRY		3	/* RepMgr connections. */
#define	DB_REP_ELECTION_RETRY		4	/* RepMgr elect retries. */
#define	DB_REP_ELECTION_TIMEOUT		5	/* Rep normal elections. */
#define	DB_REP_FULL_ELECTION_TIMEOUT	6	/* Rep full elections. */
#define	DB_REP_HEARTBEAT_MONITOR	7	/* RepMgr client HB monitor. */
#define	DB_REP_HEARTBEAT_SEND		8	/* RepMgr master send freq. */
#define	DB_REP_LEASE_TIMEOUT		9	/* Master leases. */

/*
 * Event notification types.  (Tcl testing interface currently assumes there are
 * no more than 32 of these.)
 */
#define	DB_EVENT_PANIC			 0
#define	DB_EVENT_REG_ALIVE		 1
#define	DB_EVENT_REG_PANIC		 2
#define	DB_EVENT_REP_AUTOTAKEOVER_FAILED 3
#define	DB_EVENT_REP_CLIENT		 4
#define	DB_EVENT_REP_CONNECT_BROKEN	 5
#define	DB_EVENT_REP_CONNECT_ESTD	 6
#define	DB_EVENT_REP_CONNECT_TRY_FAILED	 7
#define	DB_EVENT_REP_DUPMASTER		 8
#define	DB_EVENT_REP_ELECTED		 9
#define	DB_EVENT_REP_ELECTION_FAILED	10
#define	DB_EVENT_REP_INIT_DONE		11
#define	DB_EVENT_REP_JOIN_FAILURE	12
#define	DB_EVENT_REP_LOCAL_SITE_REMOVED	13
#define	DB_EVENT_REP_MASTER		14
#define	DB_EVENT_REP_MASTER_FAILURE	15
#define	DB_EVENT_REP_NEWMASTER		16
#define	DB_EVENT_REP_PERM_FAILED	17
#define	DB_EVENT_REP_SITE_ADDED		18
#define	DB_EVENT_REP_SITE_REMOVED	19
#define	DB_EVENT_REP_STARTUPDONE	20
#define	DB_EVENT_REP_WOULD_ROLLBACK	21	/* Undocumented; C API only. */
#define	DB_EVENT_WRITE_FAILED		22
#define	DB_EVENT_NO_SUCH_EVENT		 0xffffffff /* OOB sentinel value */

/* Replication Manager site status. */
struct __db_repmgr_site {
	int eid;
	char *host;
	u_int port;

#define	DB_REPMGR_CONNECTED	1
#define	DB_REPMGR_DISCONNECTED	2
	u_int32_t status;

#define	DB_REPMGR_ISPEER	0x01
#define	DB_REPMGR_ISVIEW	0x02
	u_int32_t flags;
};

/* Replication statistics. */
struct __db_rep_stat { /* SHARED */
	/* !!!
	 * Many replication statistics fields cannot be protected by a mutex
	 * without an unacceptable performance penalty, since most message
	 * processing is done without the need to hold a region-wide lock.
	 * Fields whose comments end with a '+' may be updated without holding
	 * the replication or log mutexes (as appropriate), and thus may be
	 * off somewhat (or, on unreasonable architectures under unlucky
	 * circumstances, garbaged).
	 */
	u_int32_t st_startup_complete;	/* Site completed client sync-up. */
	u_int32_t st_view;		/* Site is a view. */
#ifndef __TEST_DB_NO_STATISTICS
	uintmax_t st_log_queued;	/* Log records currently queued.+ */
	u_int32_t st_status;		/* Current replication status. */
	DB_LSN st_next_lsn;		/* Next LSN to use or expect. */
	DB_LSN st_waiting_lsn;		/* LSN we're awaiting, if any. */
	DB_LSN st_max_perm_lsn;		/* Maximum permanent LSN. */
	db_pgno_t st_next_pg;		/* Next pg we expect. */
	db_pgno_t st_waiting_pg;	/* pg we're awaiting, if any. */

	u_int32_t st_dupmasters;	/* # of times a duplicate master
					   condition was detected.+ */
	db_ssize_t st_env_id;		/* Current environment ID. */
	u_int32_t st_env_priority;	/* Current environment priority. */
	uintmax_t st_bulk_fills;	/* Bulk buffer fills. */
	uintmax_t st_bulk_overflows;	/* Bulk buffer overflows. */
	uintmax_t st_bulk_records;	/* Bulk records stored. */
	uintmax_t st_bulk_transfers;	/* Transfers of bulk buffers. */
	uintmax_t st_client_rerequests;/* Number of forced rerequests. */
	uintmax_t st_client_svc_req;	/* Number of client service requests
					   received by this client. */
	uintmax_t st_client_svc_miss;	/* Number of client service requests
					   missing on this client. */
	u_int32_t st_gen;		/* Current generation number. */
	u_int32_t st_egen;		/* Current election gen number. */
	uintmax_t st_lease_chk;		/* Lease validity checks. */
	uintmax_t st_lease_chk_misses;	/* Lease checks invalid. */
	uintmax_t st_lease_chk_refresh;	/* Lease refresh attempts. */
	uintmax_t st_lease_sends;	/* Lease messages sent live. */

	uintmax_t st_log_duplicated;	/* Log records received multiply.+ */
	uintmax_t st_log_queued_max;	/* Max. log records queued at once.+ */
	uintmax_t st_log_queued_total;	/* Total # of log recs. ever queued.+ */
	uintmax_t st_log_records;	/* Log records received and put.+ */
	uintmax_t st_log_requested;	/* Log recs. missed and requested.+ */
	db_ssize_t st_master;		/* Env. ID of the current master. */
	uintmax_t st_master_changes;	/* # of times we've switched masters. */
	uintmax_t st_msgs_badgen;	/* Messages with a bad generation #.+ */
	uintmax_t st_msgs_processed;	/* Messages received and processed.+ */
	uintmax_t st_msgs_recover;	/* Messages ignored because this site
					   was a client in recovery.+ */
	uintmax_t st_msgs_send_failures;/* # of failed message sends.+ */
	uintmax_t st_msgs_sent;	/* # of successful message sends.+ */
	uintmax_t st_newsites;		/* # of NEWSITE msgs. received.+ */
	u_int32_t st_nsites;		/* Current number of sites we will
					   assume during elections. */
	uintmax_t st_nthrottles;	/* # of times we were throttled. */
	uintmax_t st_outdated;		/* # of times we detected and returned
					   an OUTDATED condition.+ */
	uintmax_t st_pg_duplicated;	/* Pages received multiply.+ */
	uintmax_t st_pg_records;	/* Pages received and stored.+ */
	uintmax_t st_pg_requested;	/* Pages missed and requested.+ */
	uintmax_t st_txns_applied;	/* # of transactions applied.+ */
	uintmax_t st_startsync_delayed;/* # of STARTSYNC msgs delayed.+ */

	/* Elections generally. */
	uintmax_t st_elections;	/* # of elections held.+ */
	uintmax_t st_elections_won;	/* # of elections won by this site.+ */

	/* Statistics about an in-progress election. */
	db_ssize_t st_election_cur_winner;	/* Current front-runner. */
	u_int32_t st_election_gen;	/* Election generation number. */
	u_int32_t st_election_datagen;	/* Election data generation number. */
	DB_LSN st_election_lsn;		/* Max. LSN of current winner. */
	u_int32_t st_election_nsites;	/* # of "registered voters". */
	u_int32_t st_election_nvotes;	/* # of "registered voters" needed. */
	u_int32_t st_election_priority;	/* Current election priority. */
	int32_t   st_election_status;	/* Current election status. */
	u_int32_t st_election_tiebreaker;/* Election tiebreaker value. */
	u_int32_t st_election_votes;	/* Votes received in this round. */
	u_int32_t st_election_sec;	/* Last election time seconds. */
	u_int32_t st_election_usec;	/* Last election time useconds. */
	u_int32_t st_max_lease_sec;	/* Maximum lease timestamp seconds. */
	u_int32_t st_max_lease_usec;	/* Maximum lease timestamp useconds. */

	/* Undocumented statistics only used by the test system. */
#ifdef	CONFIG_TEST
	u_int32_t st_filefail_cleanups;	/* # of FILE_FAIL cleanups done. */
	uintmax_t st_log_futuredup;	/* Future log records that are dups. */
#endif
#endif
};

/* Replication Manager statistics. */
struct __db_repmgr_stat { /* SHARED */
	uintmax_t st_perm_failed;	/* # of insufficiently ack'ed msgs. */
	uintmax_t st_msgs_queued;	/* # msgs queued for network delay. */
	uintmax_t st_msgs_dropped;	/* # msgs discarded due to excessive
					   queue length. */
	uintmax_t st_connection_drop;	/* Existing connections dropped. */
	uintmax_t st_connect_fail;	/* Failed new connection attempts. */
	u_int32_t st_elect_threads;	/* # of active election threads. */
	u_int32_t st_max_elect_threads;	/* Max concurrent e-threads ever. */
	u_int32_t st_site_participants;	/* # of repgroup participant sites. */
	u_int32_t st_site_total;	/* # of repgroup total sites. */
	u_int32_t st_site_views;	/* # of repgroup view sites. */
	uintmax_t st_takeovers;		/* # of automatic listener takeovers. */
	u_int32_t st_incoming_queue_size; /* Undoc: # msgs currently queued. */
};

/* Replication Manager connection error. */
struct __db_repmgr_conn_err {
	int		eid;		/* Replication Environment ID. */
	int		error;		/* System networking error code. */
};

/*******************************************************
 * Sequences.
 *******************************************************/
/*
 * The storage record for a sequence.
 */
struct __db_seq_record {
	u_int32_t	seq_version;	/* Version size/number. */
	u_int32_t	flags;		/* DB_SEQ_XXX Flags. */
	db_seq_t	seq_value;	/* Current value. */
	db_seq_t	seq_max;	/* Max permitted. */
	db_seq_t	seq_min;	/* Min permitted. */
};

/*
 * Handle for a sequence object.
 */
struct __db_sequence {
	DB		*seq_dbp;	/* DB handle for this sequence. */
	db_mutex_t	mtx_seq;	/* Mutex if sequence is threaded. */
	DB_SEQ_RECORD	*seq_rp;	/* Pointer to current data. */
	DB_SEQ_RECORD	seq_record;	/* Data from DB_SEQUENCE. */
	u_int32_t	seq_cache_size; /* Number of values cached. */
	db_seq_t	seq_last_value;	/* Last value cached. */
	db_seq_t	seq_prev_value;	/* Last value returned. */
	DBT		seq_key;	/* DBT pointing to sequence key. */
	DBT		seq_data;	/* DBT pointing to seq_record. */

	/* API-private structure: used by C++ and Java. */
	void		*api_internal;

	/* DB_SEQUENCE PUBLIC HANDLE LIST BEGIN */
	int		(*close) __P((DB_SEQUENCE *, u_int32_t));
	int		(*get) __P((DB_SEQUENCE *,
			      DB_TXN *, u_int32_t, db_seq_t *, u_int32_t));
	int		(*get_cachesize) __P((DB_SEQUENCE *, u_int32_t *));
	int		(*get_db) __P((DB_SEQUENCE *, DB **));
	int		(*get_flags) __P((DB_SEQUENCE *, u_int32_t *));
	int		(*get_key) __P((DB_SEQUENCE *, DBT *));
	int		(*get_range) __P((DB_SEQUENCE *,
			     db_seq_t *, db_seq_t *));
	int		(*initial_value) __P((DB_SEQUENCE *, db_seq_t));
	int		(*open) __P((DB_SEQUENCE *,
			    DB_TXN *, DBT *, u_int32_t));
	int		(*remove) __P((DB_SEQUENCE *, DB_TXN *, u_int32_t));
	int		(*set_cachesize) __P((DB_SEQUENCE *, u_int32_t));
	int		(*set_flags) __P((DB_SEQUENCE *, u_int32_t));
	int		(*set_range) __P((DB_SEQUENCE *, db_seq_t, db_seq_t));
	int		(*stat) __P((DB_SEQUENCE *,
			    DB_SEQUENCE_STAT **, u_int32_t));
	int		(*stat_print) __P((DB_SEQUENCE *, u_int32_t));
	/* DB_SEQUENCE PUBLIC HANDLE LIST END */
};

struct __db_seq_stat { /* SHARED */
	uintmax_t st_wait;		/* Sequence lock granted w/o wait. */
	uintmax_t st_nowait;		/* Sequence lock granted after wait. */
	db_seq_t  st_current;		/* Current value in db. */
	db_seq_t  st_value;		/* Current cached value. */
	db_seq_t  st_last_value;	/* Last cached value. */
	db_seq_t  st_min;		/* Minimum value. */
	db_seq_t  st_max;		/* Maximum value. */
	u_int32_t st_cache_size;	/* Cache size. */
	u_int32_t st_flags;		/* Flag value. */
};

/*******************************************************
 * Access methods.
 *******************************************************/
/*
 * Any new methods need to retain the original numbering.  The type
 * is written in a log record so must be maintained.
 */
typedef enum {
	DB_BTREE=1,
	DB_HASH=2,
	DB_HEAP=6,
	DB_RECNO=3,
	DB_QUEUE=4,
	DB_UNKNOWN=5			/* Figure it out on open. */
} DBTYPE;

#define	DB_RENAMEMAGIC	0x030800	/* File has been renamed. */

#define	DB_BTREEVERSION	10		/* Current btree version. */
#define	DB_BTREEOLDVER	8		/* Oldest btree version supported. */
#define	DB_BTREEMAGIC	0x053162

#define	DB_HASHVERSION	10		/* Current hash version. */
#define	DB_HASHOLDVER	7		/* Oldest hash version supported. */
#define	DB_HASHMAGIC	0x061561

#define	DB_HEAPVERSION	2		/* Current heap version. */
#define	DB_HEAPOLDVER	1		/* Oldest heap version supported. */
#define	DB_HEAPMAGIC	0x074582

#define	DB_QAMVERSION	4		/* Current queue version. */
#define	DB_QAMOLDVER	3		/* Oldest queue version supported. */
#define	DB_QAMMAGIC	0x042253

#define	DB_SEQUENCE_VERSION 2		/* Current sequence version. */
#define	DB_SEQUENCE_OLDVER  1		/* Oldest sequence version supported. */

/*
 * DB access method and cursor operation values.  Each value is an operation
 * code to which additional bit flags are added.
 */
#define	DB_AFTER		 1	/* Dbc.put */
#define	DB_APPEND		 2	/* Db.put */
#define	DB_BEFORE		 3	/* Dbc.put */
#define	DB_CONSUME		 4	/* Db.get */
#define	DB_CONSUME_WAIT		 5	/* Db.get */
#define	DB_CURRENT		 6	/* Dbc.get, Dbc.put, DbLogc.get */
#define	DB_FIRST		 7	/* Dbc.get, DbLogc->get */
#define	DB_GET_BOTH		 8	/* Db.get, Dbc.get */
#define	DB_GET_BOTHC		 9	/* Dbc.get (internal) */
#define	DB_GET_BOTH_RANGE	10	/* Db.get, Dbc.get */
#define	DB_GET_RECNO		11	/* Dbc.get */
#define	DB_JOIN_ITEM		12	/* Dbc.get; don't do primary lookup */
#define	DB_KEYFIRST		13	/* Dbc.put */
#define	DB_KEYLAST		14	/* Dbc.put */
#define	DB_LAST			15	/* Dbc.get, DbLogc->get */
#define	DB_NEXT			16	/* Dbc.get, DbLogc->get */
#define	DB_NEXT_DUP		17	/* Dbc.get */
#define	DB_NEXT_NODUP		18	/* Dbc.get */
#define	DB_NODUPDATA		19	/* Db.put, Dbc.put */
#define	DB_NOOVERWRITE		20	/* Db.put */
#define	DB_OVERWRITE_DUP	21	/* Dbc.put, Db.put; no DB_KEYEXIST */
#define	DB_POSITION		22	/* Dbc.dup */
#define	DB_PREV			23	/* Dbc.get, DbLogc->get */
#define	DB_PREV_DUP		24	/* Dbc.get */
#define	DB_PREV_NODUP		25	/* Dbc.get */
#define	DB_SET			26	/* Dbc.get, DbLogc->get */
#define	DB_SET_RANGE		27	/* Dbc.get */
#define	DB_SET_RECNO		28	/* Db.get, Dbc.get */
#define	DB_UPDATE_SECONDARY	29	/* Dbc.get, Dbc.del (internal) */
#define	DB_SET_LTE		30	/* Dbc.get (internal) */
#define	DB_GET_BOTH_LTE		31	/* Dbc.get (internal) */

/* This has to change when the max opcode hits 255. */
#define	DB_OPFLAGS_MASK	0x000000ff	/* Mask for operations flags. */

/*
 * DB (user visible) error return codes.
 *
 * !!!
 * We don't want our error returns to conflict with other packages where
 * possible, so pick a base error value that's hopefully not common.  We
 * document that we own the error name space from -30,800 to -30,999.
 */
/* DB (public) error return codes. */
#define	DB_BUFFER_SMALL		(-30999)/* User memory too small for return. */
#define	DB_DONOTINDEX		(-30998)/* "Null" return from 2ndary callbk. */
#define	DB_FOREIGN_CONFLICT	(-30997)/* A foreign db constraint triggered. */
#define	DB_HEAP_FULL		(-30996)/* No free space in a heap file. */
#define	DB_KEYEMPTY		(-30995)/* Key/data deleted or never created. */
#define	DB_KEYEXIST		(-30994)/* The key/data pair already exists. */
#define	DB_LOCK_DEADLOCK	(-30993)/* Deadlock. */
#define	DB_LOCK_NOTGRANTED	(-30992)/* Lock unavailable. */
#define	DB_LOG_BUFFER_FULL	(-30991)/* In-memory log buffer full. */
#define	DB_LOG_VERIFY_BAD	(-30990)/* Log verification failed. */
#define	DB_NOSERVER		(-30989)/* Server panic return. */
#define	DB_NOTFOUND		(-30988)/* Key/data pair not found (EOF). */
#define	DB_OLD_VERSION		(-30987)/* Out-of-date version. */
#define	DB_PAGE_NOTFOUND	(-30986)/* Requested page not found. */
#define	DB_REP_DUPMASTER	(-30985)/* There are two masters. */
#define	DB_REP_HANDLE_DEAD	(-30984)/* Rolled back a commit. */
#define	DB_REP_HOLDELECTION	(-30983)/* Time to hold an election. */
#define	DB_REP_IGNORE		(-30982)/* This msg should be ignored.*/
#define	DB_REP_ISPERM		(-30981)/* Cached not written perm written.*/
#define	DB_REP_JOIN_FAILURE	(-30980)/* Unable to join replication group. */
#define	DB_REP_LEASE_EXPIRED	(-30979)/* Master lease has expired. */
#define	DB_REP_LOCKOUT		(-30978)/* API/Replication lockout now. */
#define	DB_REP_NEWSITE		(-30977)/* New site entered system. */
#define	DB_REP_NOTPERM		(-30976)/* Permanent log record not written. */
#define	DB_REP_UNAVAIL		(-30975)/* Site cannot currently be reached. */
#define	DB_REP_WOULDROLLBACK	(-30974)/* UNDOC: rollback inhibited by app. */
#define	DB_RUNRECOVERY		(-30973)/* Panic return. */
#define	DB_SECONDARY_BAD	(-30972)/* Secondary index corrupt. */
#define	DB_TIMEOUT		(-30971)/* Timed out on read consistency. */
#define	DB_VERIFY_BAD		(-30970)/* Verify failed; bad format. */
#define	DB_VERSION_MISMATCH	(-30969)/* Environment version mismatch. */

/* DB (private) error return codes. */
#define	DB_ALREADY_ABORTED	(-30899)
#define	DB_CHKSUM_FAIL		(-30898)/* Checksum failed. */
#define	DB_DELETED		(-30897)/* Recovery file marked deleted. */
#define	DB_EVENT_NOT_HANDLED	(-30896)/* Forward event to application. */
#define	DB_NEEDSPLIT		(-30895)/* Page needs to be split. */
#define	DB_REP_BULKOVF		(-30894)/* Rep bulk buffer overflow. */
#define	DB_REP_LOGREADY		(-30893)/* Rep log ready for recovery. */
#define	DB_REP_NEWMASTER	(-30892)/* We have learned of a new master. */
#define	DB_REP_PAGEDONE		(-30891)/* This page was already done. */
#define	DB_SURPRISE_KID		(-30890)/* Child commit where parent
					   didn't know it was a parent. */
#define	DB_SWAPBYTES		(-30889)/* Database needs byte swapping. */
#define	DB_TXN_CKP		(-30888)/* Encountered ckp record in log. */
#define	DB_VERIFY_FATAL		(-30887)/* DB->verify cannot proceed. */

/* Database handle. */
struct __db {
	/*******************************************************
	 * Public: owned by the application.
	 *******************************************************/
	u_int32_t pgsize;		/* Database logical page size. */
	DB_CACHE_PRIORITY priority;	/* Database priority in cache. */

					/* Callbacks. */
	int (*db_append_recno) __P((DB *, DBT *, db_recno_t));
	void (*db_feedback) __P((DB *, int, int));
	int (*dup_compare) __P((DB *, const DBT *, const DBT *, size_t *));

	void	*app_private;		/* Application-private handle. */

	/*******************************************************
	 * Private: owned by DB.
	 *******************************************************/
	DB_ENV	*dbenv;			/* Backing public environment. */
	ENV	*env;			/* Backing private environment. */

	DBTYPE	 type;			/* DB access method type. */

	DB_MPOOLFILE *mpf;		/* Backing buffer pool. */

	db_mutex_t mutex;		/* Synchronization for free threading */

	char *fname, *dname;		/* File/database passed to DB->open. */
	const char *dirname;		/* Directory of DB file. */
	u_int32_t open_flags;		/* Flags passed to DB->open. */

	u_int8_t fileid[DB_FILE_ID_LEN];/* File's unique ID for locking. */

	u_int32_t adj_fileid;		/* File's unique ID for curs. adj. */

	u_int32_t blob_threshold;	/* Blob threshold record size. */

#define	DB_LOGFILEID_INVALID	-1
	FNAME *log_filename;		/* File's naming info for logging. */

	db_pgno_t meta_pgno;		/* Meta page number */
	DB_LOCKER *locker;		/* Locker for handle locking. */
	DB_LOCKER *cur_locker;		/* Current handle lock holder. */
	DB_TXN *cur_txn;		/* Opening transaction. */
	DB_LOCKER *associate_locker;	/* Locker for DB->associate call. */
	DB_LOCK	 handle_lock;		/* Lock held on this handle. */

	time_t	 timestamp;		/* Handle timestamp for replication. */
	u_int32_t fid_gen;		/* Rep generation number for fids. */

	/*
	 * Returned data memory for DB->get() and friends.
	 */
	DBT	 my_rskey;		/* Secondary key. */
	DBT	 my_rkey;		/* [Primary] key. */
	DBT	 my_rdata;		/* Data. */

	/*
	 * !!!
	 * Some applications use DB but implement their own locking outside of
	 * DB.  If they're using fcntl(2) locking on the underlying database
	 * file, and we open and close a file descriptor for that file, we will
	 * discard their locks.  The DB_FCNTL_LOCKING flag to DB->open is an
	 * undocumented interface to support this usage which leaves any file
	 * descriptors we open until DB->close.  This will only work with the
	 * DB->open interface and simple caches, e.g., creating a transaction
	 * thread may open/close file descriptors this flag doesn't protect.
	 * Locking with fcntl(2) on a file that you don't own is a very, very
	 * unsafe thing to do.  'Nuff said.
	 */
	DB_FH	*saved_open_fhp;	/* Saved file handle. */

	/*
	 * Linked list of DBP's, linked from the ENV, used to keep track
	 * of all open db handles for cursor adjustment.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db) dblistlinks;
	 */
	struct {
		struct __db *tqe_next;
		struct __db **tqe_prev;
	} dblistlinks;

	/*
	 * Cursor queues.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__cq_fq, __dbc) free_queue;
	 * TAILQ_HEAD(__cq_aq, __dbc) active_queue;
	 * TAILQ_HEAD(__cq_jq, __dbc) join_queue;
	 */
	struct __cq_fq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} free_queue;
	struct __cq_aq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} active_queue;
	struct __cq_jq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} join_queue;

	/*
	 * Secondary index support.
	 *
	 * Linked list of secondary indices -- set in the primary.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_HEAD(s_secondaries, __db);
	 */
	struct {
		struct __db *lh_first;
	} s_secondaries;

	/*
	 * List entries for secondaries, and reference count of how many
	 * threads are updating this secondary (see Dbc.put).
	 *
	 * !!!
	 * Note that these are synchronized by the primary's mutex, but
	 * filled in in the secondaries.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_ENTRY(__db) s_links;
	 */
	struct {
		struct __db *le_next;
		struct __db **le_prev;
	} s_links;
	u_int32_t s_refcnt;

	/* Secondary callback and free functions -- set in the secondary. */
	int	(*s_callback) __P((DB *, const DBT *, const DBT *, DBT *));

	/* Reference to primary -- set in the secondary. */
	DB	*s_primary;

#define	DB_ASSOC_IMMUTABLE_KEY    0x00000001 /* Secondary key is immutable. */
#define	DB_ASSOC_CREATE    0x00000002 /* Secondary db populated on open. */

	/* Flags passed to associate -- set in the secondary. */
	u_int32_t s_assoc_flags;

	/*
	 * Foreign key support.
	 *
	 * Linked list of primary dbs -- set in the foreign db
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_HEAD(f_primaries, __db);
	 */
	struct {
		struct __db_foreign_info *lh_first;
	} f_primaries;

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db) felink;
	 *
	 * Links in a list of DBs involved in file extension
	 * during a transaction.  These are to be used only while the
	 * metadata is locked.
	 */
	struct {
		struct __db *tqe_next;
		struct __db **tqe_prev;
	} felink;

	/* Reference to foreign -- set in the secondary. */
	DB      *s_foreign;

	DB		*blob_meta_db;	/* Databases holding blob metadata. */
	DB_SEQUENCE	*blob_seq;	/* Sequence of blob ids. */
	char		*blob_sub_dir;	/* Subdirectory for blob files */
	db_seq_t	blob_file_id;	/* Id of the file blob directory. */
	db_seq_t	blob_sdb_id;	/* Id of the subdb blob directory. */

	/* API-private structure: used by DB 1.85, C++, Java, Perl and Tcl */
	void	*api_internal;

	/* Subsystem-private structure. */
	void	*bt_internal;		/* Btree/Recno access method. */
	void	*h_internal;		/* Hash access method. */
	void	*heap_internal;		/* Heap access method. */
	void	*p_internal;		/* Partition informaiton. */
	void	*q_internal;		/* Queue access method. */

	/* DB PUBLIC HANDLE LIST BEGIN */
	int  (*associate) __P((DB *, DB_TXN *, DB *,
		int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
	int  (*associate_foreign) __P((DB *, DB *,
		int (*)(DB *, const DBT *, DBT *, const DBT *, int *),
		u_int32_t));
	int  (*close) __P((DB *, u_int32_t));
	int  (*compact) __P((DB *,
		DB_TXN *, DBT *, DBT *, DB_COMPACT *, u_int32_t, DBT *));
	int  (*cursor) __P((DB *, DB_TXN *, DBC **, u_int32_t));
	int  (*del) __P((DB *, DB_TXN *, DBT *, u_int32_t));
	void (*err) __P((DB *, int, const char *, ...));
	void (*errx) __P((DB *, const char *, ...));
	int  (*exists) __P((DB *, DB_TXN *, DBT *, u_int32_t));
	int  (*fd) __P((DB *, int *));
	int  (*get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*get_alloc) __P((DB *, void *(**)(size_t),
		void *(**)(void *, size_t), void (**)(void *)));
	int  (*get_append_recno) __P((DB *, int (**)(DB *, DBT *, db_recno_t)));
	int  (*get_assoc_flags) __P((DB *, u_int32_t *));
	int  (*get_blob_dir) __P((DB *, const char **));
	int  (*get_blob_sub_dir) __P((DB *, const char **));
	int  (*get_blob_threshold) __P((DB *, u_int32_t *));
	int  (*get_bt_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*get_bt_compress) __P((DB *,
		int (**)(DB *,
		const DBT *, const DBT *, const DBT *, const DBT *, DBT *),
		int (**)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
	int  (*get_bt_minkey) __P((DB *, u_int32_t *));
	int  (*get_bt_prefix)
		__P((DB *, size_t (**)(DB *, const DBT *, const DBT *)));
	int  (*get_byteswapped) __P((DB *, int *));
	int  (*get_cachesize) __P((DB *, u_int32_t *, u_int32_t *, int *));
	int  (*get_create_dir) __P((DB *, const char **));
	int  (*get_dbname) __P((DB *, const char **, const char **));
	int  (*get_dup_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*get_encrypt_flags) __P((DB *, u_int32_t *));
	DB_ENV *(*get_env) __P((DB *));
	void (*get_errcall) __P((DB *,
		void (**)(const DB_ENV *, const char *, const char *)));
	void (*get_errfile) __P((DB *, FILE **));
	void (*get_errpfx) __P((DB *, const char **));
	int  (*get_feedback) __P((DB *, void (**)(DB *, int, int)));
	int  (*get_flags) __P((DB *, u_int32_t *));
	int  (*get_h_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*get_h_ffactor) __P((DB *, u_int32_t *));
	int  (*get_h_hash)
		__P((DB *, u_int32_t (**)(DB *, const void *, u_int32_t)));
	int  (*get_h_nelem) __P((DB *, u_int32_t *));
	int  (*get_heapsize) __P((DB *, u_int32_t *, u_int32_t *));
	int  (*get_heap_regionsize) __P((DB *, u_int32_t *));
	int  (*get_lk_exclusive) __P((DB *, int *, int *));
	int  (*get_lorder) __P((DB *, int *));
	DB_MPOOLFILE *(*get_mpf) __P((DB *));
	void (*get_msgcall) __P((DB *,
	    void (**)(const DB_ENV *, const char *)));
	void (*get_msgfile) __P((DB *, FILE **));
	int  (*get_multiple) __P((DB *));
	int  (*get_open_flags) __P((DB *, u_int32_t *));
	int  (*get_pagesize) __P((DB *, u_int32_t *));
	int  (*get_partition_callback) __P((DB *,
		u_int32_t *, u_int32_t (**)(DB *, DBT *key)));
	int  (*get_partition_dirs) __P((DB *, const char ***));
	int  (*get_partition_keys) __P((DB *, u_int32_t *, DBT **));
	int  (*get_priority) __P((DB *, DB_CACHE_PRIORITY *));
	int  (*get_q_extentsize) __P((DB *, u_int32_t *));
	int  (*get_re_delim) __P((DB *, int *));
	int  (*get_re_len) __P((DB *, u_int32_t *));
	int  (*get_re_pad) __P((DB *, int *));
	int  (*get_re_source) __P((DB *, const char **));
	int  (*get_transactional) __P((DB *));
	int  (*get_type) __P((DB *, DBTYPE *));
	int  (*join) __P((DB *, DBC **, DBC **, u_int32_t));
	int  (*key_range)
		__P((DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t));
	int  (*open) __P((DB *,
		DB_TXN *, const char *, const char *, DBTYPE, u_int32_t, int));
	int  (*pget) __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
	int  (*put) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*remove) __P((DB *, const char *, const char *, u_int32_t));
	int  (*rename) __P((DB *,
		const char *, const char *, const char *, u_int32_t));
	int  (*set_alloc) __P((DB *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	int  (*set_append_recno) __P((DB *, int (*)(DB *, DBT *, db_recno_t)));
	int  (*set_blob_dir) __P((DB *, const char *));
	int  (*set_blob_threshold) __P((DB *, u_int32_t, u_int32_t));
	int  (*set_bt_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*set_bt_compress) __P((DB *,
		int (*)(DB *, const DBT *, const DBT *, const DBT *, const DBT *, DBT *),
		int (*)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
	int  (*set_bt_minkey) __P((DB *, u_int32_t));
	int  (*set_bt_prefix)
		__P((DB *, size_t (*)(DB *, const DBT *, const DBT *)));
	int  (*set_cachesize) __P((DB *, u_int32_t, u_int32_t, int));
	int  (*set_create_dir) __P((DB *, const char *));
	int  (*set_dup_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*set_encrypt) __P((DB *, const char *, u_int32_t));
	void (*set_errcall) __P((DB *,
		void (*)(const DB_ENV *, const char *, const char *)));
	void (*set_errfile) __P((DB *, FILE *));
	void (*set_errpfx) __P((DB *, const char *));
	int  (*set_feedback) __P((DB *, void (*)(DB *, int, int)));
	int  (*set_flags) __P((DB *, u_int32_t));
	int  (*set_h_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *, size_t *)));
	int  (*set_h_ffactor) __P((DB *, u_int32_t));
	int  (*set_h_hash)
		__P((DB *, u_int32_t (*)(DB *, const void *, u_int32_t)));
	int  (*set_h_nelem) __P((DB *, u_int32_t));
	int  (*set_heapsize) __P((DB *, u_int32_t, u_int32_t, u_int32_t));
	int  (*set_heap_regionsize) __P((DB *, u_int32_t));
	int  (*set_lk_exclusive) __P((DB *, int));
	int  (*set_lorder) __P((DB *, int));
	void (*set_msgcall) __P((DB *, void (*)(const DB_ENV *, const char *)));
	void (*set_msgfile) __P((DB *, FILE *));
	int  (*set_pagesize) __P((DB *, u_int32_t));
	int  (*set_paniccall) __P((DB *, void (*)(DB_ENV *, int)));
	int  (*set_partition) __P((DB *,
		u_int32_t, DBT *, u_int32_t (*)(DB *, DBT *key)));
	int  (*set_partition_dirs) __P((DB *, const char **));
	int  (*set_priority) __P((DB *, DB_CACHE_PRIORITY));
	int  (*set_q_extentsize) __P((DB *, u_int32_t));
	int  (*set_re_delim) __P((DB *, int));
	int  (*set_re_len) __P((DB *, u_int32_t));
	int  (*set_re_pad) __P((DB *, int));
	int  (*set_re_source) __P((DB *, const char *));
	int  (*sort_multiple) __P((DB *, DBT *, DBT *, u_int32_t));
	int  (*stat) __P((DB *, DB_TXN *, void *, u_int32_t));
	int  (*stat_print) __P((DB *, u_int32_t));
	int  (*sync) __P((DB *, u_int32_t));
	int  (*truncate) __P((DB *, DB_TXN *, u_int32_t *, u_int32_t));
	int  (*upgrade) __P((DB *, const char *, u_int32_t));
	int  (*verify)
		__P((DB *, const char *, const char *, FILE *, u_int32_t));
	/* DB PUBLIC HANDLE LIST END */

	/* DB PRIVATE HANDLE LIST BEGIN */
	int  (*dump) __P((DB *, const char *,
		int (*)(void *, const void *), void *, int, int));
	int  (*db_am_remove) __P((DB *, DB_THREAD_INFO *,
		DB_TXN *, const char *, const char *, u_int32_t));
	int  (*db_am_rename) __P((DB *, DB_THREAD_INFO *,
		DB_TXN *, const char *, const char *, const char *));
	/* DB PRIVATE HANDLE LIST END */

	/*
	 * Never called; these are a place to save function pointers
	 * so that we can undo an associate.
	 */
	int  (*stored_get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*stored_close) __P((DB *, u_int32_t));

	/* Alternative handle close function, used by C++ API. */
	int  (*alt_close) __P((DB *, u_int32_t));

#define	DB_OK_BTREE	0x01
#define	DB_OK_HASH	0x02
#define	DB_OK_HEAP	0x04
#define	DB_OK_QUEUE	0x08
#define	DB_OK_RECNO	0x10
	u_int32_t	am_ok;		/* Legal AM choices. */

	/*
	 * This field really ought to be an AM_FLAG, but we have
	 * have run out of bits.  If/when we decide to split up
	 * the flags, we can incorporate it.
	 */
	int	 preserve_fid;		/* Do not free fileid on close. */

#define	DB_AM_CHKSUM		0x00000001 /* Checksumming */
#define	DB_AM_COMPENSATE	0x00000002 /* Created by compensating txn */
#define	DB_AM_COMPRESS		0x00000004 /* Compressed BTree */
#define	DB_AM_CREATED		0x00000008 /* Database was created upon open */
#define	DB_AM_CREATED_MSTR	0x00000010 /* Encompassing file was created */
#define	DB_AM_DBM_ERROR		0x00000020 /* Error in DBM/NDBM database */
#define	DB_AM_DELIMITER		0x00000040 /* Variable length delimiter set */
#define	DB_AM_DISCARD		0x00000080 /* Discard any cached pages */
#define	DB_AM_DUP		0x00000100 /* DB_DUP */
#define	DB_AM_DUPSORT		0x00000200 /* DB_DUPSORT */
#define	DB_AM_ENCRYPT		0x00000400 /* Encryption */
#define	DB_AM_FIXEDLEN		0x00000800 /* Fixed-length records */
#define	DB_AM_INMEM		0x00001000 /* In-memory; no sync on close */
#define	DB_AM_INORDER		0x00002000 /* DB_INORDER */
#define	DB_AM_IN_RENAME		0x00004000 /* File is being renamed */
#define	DB_AM_NOT_DURABLE	0x00008000 /* Do not log changes */
#define	DB_AM_OPEN_CALLED	0x00010000 /* DB->open called */
#define	DB_AM_PAD		0x00020000 /* Fixed-length record pad */
#define	DB_AM_PARTDB		0x00040000 /* Handle for a database partition */
#define	DB_AM_PGDEF		0x00080000 /* Page size was defaulted */
#define	DB_AM_RDONLY		0x00100000 /* Database is readonly */
#define	DB_AM_READ_UNCOMMITTED	0x00200000 /* Support degree 1 isolation */
#define	DB_AM_RECNUM		0x00400000 /* DB_RECNUM */
#define	DB_AM_RECOVER		0x00800000 /* DB opened by recovery routine */
#define	DB_AM_RENUMBER		0x01000000 /* DB_RENUMBER */
#define	DB_AM_REVSPLITOFF	0x02000000 /* DB_REVSPLITOFF */
#define	DB_AM_SECONDARY		0x04000000 /* Database is a secondary index */
#define	DB_AM_SNAPSHOT		0x08000000 /* DB_SNAPSHOT */
#define	DB_AM_SUBDB		0x10000000 /* Subdatabases supported */
#define	DB_AM_SWAP		0x20000000 /* Pages need to be byte-swapped */
#define	DB_AM_TXN		0x40000000 /* Opened in a transaction */
#define	DB_AM_VERIFYING		0x80000000 /* DB handle is in the verifier */
	u_int32_t orig_flags;		   /* Flags at  open, for refresh */
	u_int32_t flags;

#define DB2_AM_EXCL		0x00000001 /* Exclusively lock the handle */ 
#define DB2_AM_INTEXCL		0x00000002 /* Internal exclusive lock. */
#define DB2_AM_NOWAIT		0x00000004 /* Do not wait for handle lock */ 
	u_int32_t orig_flags2;		   /* Second flags word; for refresh */ 
	u_int32_t flags2;		   /* Second flags word */
};

/* 
 * Stream interface for blob files.
 */
struct __db_stream {
	DBC		*dbc;	/* Cursor pointing to the db blob record. */
	DB_FH		*fhp;

	/* DB_STREAM PUBLIC HANDLE LIST BEGIN */
	int  (*close) __P((DB_STREAM *, u_int32_t));
	int  (*read) __P((DB_STREAM *, DBT *, db_off_t, u_int32_t, u_int32_t));
	int  (*size) __P((DB_STREAM *, db_off_t *, u_int32_t));
	int  (*write) __P((DB_STREAM *, DBT *, db_off_t, u_int32_t));
	/* DB_STREAM PUBLIC HANDLE LIST END */

	u_int32_t	flags;
#define	DB_STREAM_READ		0x00000001 /* Stream is read only. */
#define	DB_STREAM_WRITE		0x00000002 /* Stream is writeable. */
#define	DB_STREAM_SYNC_WRITE	0x00000004 /* Sync file on each write. */
	db_seq_t	blob_id;
	db_off_t	file_size;
};

/*
 * Macros for bulk operations.  These are only intended for the C API.
 * For C++, use DbMultiple*Iterator or DbMultiple*Builder.
 *
 * Bulk operations store multiple entries into a single DBT structure. The
 * following macros assist with creating and reading these Multiple DBTs.
 *
 * The basic layout for single data items is:
 *
 * -------------------------------------------------------------------------
 * | data1 | ... | dataN | ..... |-1 | dNLen | dNOff | ... | d1Len | d1Off |
 * -------------------------------------------------------------------------
 *
 * For the DB_MULTIPLE_KEY* macros, the items are in key/data pairs, so data1
 * would be a key, and data2 its corresponding value (N is always even).
 *
 * For the DB_MULTIPLE_RECNO* macros, the record number is stored along with
 * the len/off pair in the "header" section, and the list is zero terminated
 * (since -1 is a valid record number):
 *
 * --------------------------------------------------------------------------
 * | d1 |..| dN |..| 0 | dNLen | dNOff | recnoN |..| d1Len | d1Off | recno1 |
 * --------------------------------------------------------------------------
 */
#define	DB_MULTIPLE_INIT(pointer, dbt)					\
	(pointer = (u_int8_t *)(dbt)->data +				\
	    (dbt)->ulen - sizeof(u_int32_t))

#define	DB_MULTIPLE_NEXT(pointer, dbt, retdata, retdlen)		\
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)-1) {				\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
		if (retdlen == 0 && retdata == (u_int8_t *)(dbt)->data)	\
			retdata = NULL;					\
	} while (0)

#define	DB_MULTIPLE_KEY_NEXT(pointer, dbt, retkey, retklen, retdata, retdlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)-1) {				\
			retdata = NULL;					\
			retkey = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retkey = (u_int8_t *)(dbt)->data + *__p--;		\
		retklen = *__p--;					\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
	} while (0)

#define	DB_MULTIPLE_RECNO_NEXT(pointer, dbt, recno, retdata, retdlen)   \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)0) {				\
			recno = 0;					\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		recno = *__p--;						\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
	} while (0)

#define DB_MULTIPLE_WRITE_INIT(pointer, dbt)				\
	do {								\
		(dbt)->flags |= DB_DBT_BULK;				\
		pointer = (u_int8_t *)(dbt)->data +			\
		    (dbt)->ulen - sizeof(u_int32_t);			\
		*(u_int32_t *)(pointer) = (u_int32_t)-1;		\
	} while (0)

#define DB_MULTIPLE_RESERVE_NEXT(pointer, dbt, writedata, writedlen)	\
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) ==	(u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ?  0 : __p[1] + __p[2];\
		if ((u_int8_t *)(dbt)->data + __off + (writedlen) >	\
		    (u_int8_t *)(__p - 2))				\
			writedata = NULL;				\
		else {							\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (u_int32_t)(writedlen);		\
			__p[-2] = (u_int32_t)-1;			\
			pointer = __p - 2;				\
		}							\
	} while (0)

#define DB_MULTIPLE_WRITE_NEXT(pointer, dbt, writedata, writedlen)	\
	do {								\
		void *__destd;						\
		DB_MULTIPLE_RESERVE_NEXT((pointer), (dbt),		\
		    __destd, (writedlen));				\
		if (__destd == NULL)					\
			pointer = NULL;					\
		else							\
			memcpy(__destd, (writedata), (writedlen));	\
	} while (0)

#define DB_MULTIPLE_KEY_RESERVE_NEXT(pointer, dbt, writekey, writeklen, writedata, writedlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) == (u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ?  0 : __p[1] + __p[2];\
		if ((u_int8_t *)(dbt)->data + __off + (writeklen) +	\
		    (writedlen) > (u_int8_t *)(__p - 4)) {		\
			writekey = NULL;				\
			writedata = NULL;				\
		} else {						\
			writekey = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (u_int32_t)(writeklen);		\
			__p -= 2;					\
			__off += (u_int32_t)(writeklen);		\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (u_int32_t)(writedlen);		\
			__p[-2] = (u_int32_t)-1;			\
			pointer = __p - 2;				\
		}							\
	} while (0)

#define DB_MULTIPLE_KEY_WRITE_NEXT(pointer, dbt, writekey, writeklen, writedata, writedlen) \
	do {								\
		void *__destk, *__destd;				\
		DB_MULTIPLE_KEY_RESERVE_NEXT((pointer), (dbt),		\
		    __destk, (writeklen), __destd, (writedlen));	\
		if (__destk == NULL)					\
			pointer = NULL;					\
		else {							\
			memcpy(__destk, (writekey), (writeklen));	\
			if (__destd != NULL)				\
				memcpy(__destd, (writedata), (writedlen));\
		}							\
	} while (0)

#define DB_MULTIPLE_RECNO_WRITE_INIT(pointer, dbt)			\
	do {								\
		(dbt)->flags |= DB_DBT_BULK;				\
		pointer = (u_int8_t *)(dbt)->data +			\
		    (dbt)->ulen - sizeof(u_int32_t);			\
		*(u_int32_t *)(pointer) = 0;				\
	} while (0)

#define DB_MULTIPLE_RECNO_RESERVE_NEXT(pointer, dbt, recno, writedata, writedlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) == (u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ? 0 : __p[1] + __p[2]; \
		if (((u_int8_t *)(dbt)->data + __off) + (writedlen) >	\
		    (u_int8_t *)(__p - 3))				\
			writedata = NULL;				\
		else {							\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = (u_int32_t)(recno);			\
			__p[-1] = __off;				\
			__p[-2] = (u_int32_t)(writedlen);		\
			__p[-3] = 0;					\
			pointer = __p - 3;				\
		}							\
	} while (0)

#define DB_MULTIPLE_RECNO_WRITE_NEXT(pointer, dbt, recno, writedata, writedlen)\
	do {								\
		void *__destd;						\
		DB_MULTIPLE_RECNO_RESERVE_NEXT((pointer), (dbt),	\
		    (recno), __destd, (writedlen));			\
		if (__destd == NULL)					\
			pointer = NULL;					\
		else if ((writedlen) != 0)				\
			memcpy(__destd, (writedata), (writedlen));	\
	} while (0)

struct __db_heap_rid {
	db_pgno_t pgno;			/* Page number. */
	db_indx_t indx;			/* Index in the offset table. */
};
#define DB_HEAP_RID_SZ	(sizeof(db_pgno_t) + sizeof(db_indx_t))

/*******************************************************
 * Access method cursors.
 *******************************************************/
struct __dbc {
	DB *dbp;			/* Backing database */
	DB_ENV *dbenv;			/* Backing environment */
	ENV *env;			/* Backing environment */

	DB_THREAD_INFO *thread_info;	/* Thread that owns this cursor. */
	DB_TXN	 *txn;			/* Associated transaction. */
	DB_CACHE_PRIORITY priority;	/* Priority in cache. */

	/*
	 * Active/free cursor queues.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__dbc) links;
	 */
	struct {
		DBC *tqe_next;
		DBC **tqe_prev;
	} links;

	/*
	 * Cursor queue of the owning transaction.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__dbc) txn_cursors;
	 */
	struct {
		DBC *tqe_next;	/* next element */
		DBC **tqe_prev;	/* address of previous next element */
	} txn_cursors;

	/*
	 * The DBT *'s below are used by the cursor routines to return
	 * data to the user when DBT flags indicate that DB should manage
	 * the returned memory.  They point at a DBT containing the buffer
	 * and length that will be used, and "belonging" to the handle that
	 * should "own" this memory.  This may be a "my_*" field of this
	 * cursor--the default--or it may be the corresponding field of
	 * another cursor, a DB handle, a join cursor, etc.  In general, it
	 * will be whatever handle the user originally used for the current
	 * DB interface call.
	 */
	DBT	 *rskey;		/* Returned secondary key. */
	DBT	 *rkey;			/* Returned [primary] key. */
	DBT	 *rdata;		/* Returned data. */

	DBT	  my_rskey;		/* Space for returned secondary key. */
	DBT	  my_rkey;		/* Space for returned [primary] key. */
	DBT	  my_rdata;		/* Space for returned data. */

	DB_LOCKER *lref;		/* Reference to default locker. */
	DB_LOCKER *locker;		/* Locker for this operation. */
	DBT	  lock_dbt;		/* DBT referencing lock. */
	DB_LOCK_ILOCK lock;		/* Object to be locked. */
	DB_LOCK	  mylock;		/* CDB lock held on this cursor. */

	DBTYPE	  dbtype;		/* Cursor type. */

	DBC_INTERNAL *internal;		/* Access method private. */

	/* DBC PUBLIC HANDLE LIST BEGIN */
	int (*close) __P((DBC *));
	int (*cmp) __P((DBC *, DBC *, int *, u_int32_t));
	int (*count) __P((DBC *, db_recno_t *, u_int32_t));
	int (*db_stream) __P((DBC *, DB_STREAM **, u_int32_t));
	int (*del) __P((DBC *, u_int32_t));
	int (*dup) __P((DBC *, DBC **, u_int32_t));
	int (*get) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*get_priority) __P((DBC *, DB_CACHE_PRIORITY *));
	int (*pget) __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
	int (*put) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*set_priority) __P((DBC *, DB_CACHE_PRIORITY));
	/* DBC PUBLIC HANDLE LIST END */

	/* The following are the method names deprecated in the 4.6 release. */
	int (*c_close) __P((DBC *));
	int (*c_count) __P((DBC *, db_recno_t *, u_int32_t));
	int (*c_del) __P((DBC *, u_int32_t));
	int (*c_dup) __P((DBC *, DBC **, u_int32_t));
	int (*c_get) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*c_pget) __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
	int (*c_put) __P((DBC *, DBT *, DBT *, u_int32_t));

	/* DBC PRIVATE HANDLE LIST BEGIN */
	int (*am_bulk) __P((DBC *, DBT *, u_int32_t));
	int (*am_close) __P((DBC *, db_pgno_t, int *));
	int (*am_del) __P((DBC *, u_int32_t));
	int (*am_destroy) __P((DBC *));
	int (*am_get) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*am_put) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*am_writelock) __P((DBC *));
	/* DBC PRIVATE HANDLE LIST END */

/*
 * DBC_DONTLOCK and DBC_RECOVER are used during recovery and transaction
 * abort.  If a transaction is being aborted or recovered then DBC_RECOVER
 * will be set and locking and logging will be disabled on this cursor.  If
 * we are performing a compensating transaction (e.g. free page processing)
 * then DB_DONTLOCK will be set to inhibit locking, but logging will still
 * be required. DB_DONTLOCK is also used if the whole database is locked.
 */
#define	DBC_ACTIVE		0x00001	/* Cursor in use. */
#define	DBC_BULK		0x00002	/* Bulk update cursor. */
#define	DBC_DONTLOCK		0x00004	/* Don't lock on this cursor. */
#define	DBC_DOWNREV		0x00008	/* Down rev replication master. */
#define	DBC_DUPLICATE		0x00010	/* Create a duplicate cursor. */
#define	DBC_ERROR		0x00020	/* Error in this request. */
#define	DBC_FAMILY		0x00040 /* Part of a locker family. */
#define	DBC_FROM_DB_GET		0x00080 /* Called from the DB->get() method. */
#define	DBC_MULTIPLE		0x00100	/* Return Multiple data. */
#define	DBC_MULTIPLE_KEY	0x00200	/* Return Multiple keys and data. */
#define	DBC_OPD			0x00400	/* Cursor references off-page dups. */
#define	DBC_OWN_LID		0x00800	/* Free lock id on destroy. */
#define	DBC_PARTITIONED		0x01000	/* Cursor for a partitioned db. */
#define	DBC_READ_COMMITTED	0x02000	/* Cursor has degree 2 isolation. */
#define	DBC_READ_UNCOMMITTED	0x04000	/* Cursor has degree 1 isolation. */
#define	DBC_RECOVER		0x08000	/* Recovery cursor; don't log/lock. */
#define	DBC_RMW			0x10000	/* Acquire write flag in read op. */
#define	DBC_TRANSIENT		0x20000	/* Cursor is transient. */
#define	DBC_WAS_READ_COMMITTED	0x40000	/* Cursor holds a read commited lock. */
#define	DBC_WRITECURSOR		0x80000	/* Cursor may be used to write (CDB). */
#define	DBC_WRITER	       0x100000	/* Cursor immediately writing (CDB). */
	u_int32_t flags;
};

/* Key range statistics structure */
struct __key_range {
	double less;
	double equal;
	double greater;
};

/* Btree/Recno statistics structure. */
struct __db_bt_stat { /* SHARED */
	u_int32_t bt_magic;		/* Magic number. */
	u_int32_t bt_version;		/* Version number. */
	u_int32_t bt_metaflags;		/* Metadata flags. */
	u_int32_t bt_nkeys;		/* Number of unique keys. */
	u_int32_t bt_ndata;		/* Number of data items. */
	u_int32_t bt_pagecnt;		/* Page count. */
	u_int32_t bt_pagesize;		/* Page size. */
	u_int32_t bt_minkey;		/* Minkey value. */
	u_int32_t bt_nblobs;		/* Number of blobs. */
	u_int32_t bt_re_len;		/* Fixed-length record length. */
	u_int32_t bt_re_pad;		/* Fixed-length record pad. */
	u_int32_t bt_levels;		/* Tree levels. */
	u_int32_t bt_int_pg;		/* Internal pages. */
	u_int32_t bt_leaf_pg;		/* Leaf pages. */
	u_int32_t bt_dup_pg;		/* Duplicate pages. */
	u_int32_t bt_over_pg;		/* Overflow pages. */
	u_int32_t bt_empty_pg;		/* Empty pages. */
	u_int32_t bt_free;		/* Pages on the free list. */
	uintmax_t bt_int_pgfree;	/* Bytes free in internal pages. */
	uintmax_t bt_leaf_pgfree;	/* Bytes free in leaf pages. */
	uintmax_t bt_dup_pgfree;	/* Bytes free in duplicate pages. */
	uintmax_t bt_over_pgfree;	/* Bytes free in overflow pages. */
};

struct __db_compact {
	/* Input Parameters. */
	u_int32_t	compact_fillpercent;	/* Desired fillfactor: 1-100 */
	db_timeout_t	compact_timeout;	/* Lock timeout. */
	u_int32_t	compact_pages;		/* Max pages to process. */
	/* Output Stats. */
	u_int32_t	compact_empty_buckets;	/* Empty hash buckets found. */
	u_int32_t	compact_pages_free;	/* Number of pages freed. */
	u_int32_t	compact_pages_examine;	/* Number of pages examine. */
	u_int32_t	compact_levels;		/* Number of levels removed. */
	u_int32_t	compact_deadlock;	/* Number of deadlocks. */
	db_pgno_t	compact_pages_truncated; /* Pages truncated to OS. */
	/* Internal. */
	db_pgno_t	compact_truncate;	/* Exchange pages above here. */
};

/* Hash statistics structure. */
struct __db_h_stat { /* SHARED */
	u_int32_t hash_magic;		/* Magic number. */
	u_int32_t hash_version;		/* Version number. */
	u_int32_t hash_metaflags;	/* Metadata flags. */
	u_int32_t hash_nkeys;		/* Number of unique keys. */
	u_int32_t hash_ndata;		/* Number of data items. */
	u_int32_t hash_nblobs;		/* Number of blobs. */
	u_int32_t hash_pagecnt;		/* Page count. */
	u_int32_t hash_pagesize;	/* Page size. */
	u_int32_t hash_ffactor;		/* Fill factor specified at create. */
	u_int32_t hash_buckets;		/* Number of hash buckets. */
	u_int32_t hash_free;		/* Pages on the free list. */
	uintmax_t hash_bfree;		/* Bytes free on bucket pages. */
	u_int32_t hash_bigpages;	/* Number of big key/data pages. */
	uintmax_t hash_big_bfree;	/* Bytes free on big item pages. */
	u_int32_t hash_overflows;	/* Number of overflow pages. */
	uintmax_t hash_ovfl_free;	/* Bytes free on ovfl pages. */
	u_int32_t hash_dup;		/* Number of dup pages. */
	uintmax_t hash_dup_free;	/* Bytes free on duplicate pages. */
};

/* Heap statistics structure. */
struct __db_heap_stat { /* SHARED */
	u_int32_t heap_magic;		/* Magic number. */
	u_int32_t heap_version;		/* Version number. */
	u_int32_t heap_metaflags;	/* Metadata flags. */
	u_int32_t heap_nblobs;		/* Number of blobs. */
	u_int32_t heap_nrecs;		/* Number of records. */
	u_int32_t heap_pagecnt;		/* Page count. */
	u_int32_t heap_pagesize;	/* Page size. */
	u_int32_t heap_nregions;	/* Number of regions. */
	u_int32_t heap_regionsize;	/* Number of pages in a region. */
};

/* Queue statistics structure. */
struct __db_qam_stat { /* SHARED */
	u_int32_t qs_magic;		/* Magic number. */
	u_int32_t qs_version;		/* Version number. */
	u_int32_t qs_metaflags;		/* Metadata flags. */
	u_int32_t qs_nkeys;		/* Number of unique keys. */
	u_int32_t qs_ndata;		/* Number of data items. */
	u_int32_t qs_pagesize;		/* Page size. */
	u_int32_t qs_extentsize;	/* Pages per extent. */
	u_int32_t qs_pages;		/* Data pages. */
	u_int32_t qs_re_len;		/* Fixed-length record length. */
	u_int32_t qs_re_pad;		/* Fixed-length record pad. */
	u_int32_t qs_pgfree;		/* Bytes free in data pages. */
	u_int32_t qs_first_recno;	/* First not deleted record. */
	u_int32_t qs_cur_recno;		/* Next available record number. */
};

/*******************************************************
 * Environment.
 *******************************************************/
#define	DB_REGION_MAGIC	0x120897	/* Environment magic number. */

/*
 * Database environment structure.
 *
 * This is the public database environment handle.  The private environment
 * handle is the ENV structure.   The user owns this structure, the library
 * owns the ENV structure.  The reason there are two structures is because
 * the user's configuration outlives any particular DB_ENV->open call, and
 * separate structures allows us to easily discard internal information without
 * discarding the user's configuration.
 *
 * Fields in the DB_ENV structure should normally be set only by application
 * DB_ENV handle methods.
 */

/*
 * Memory configuration types.
 */
typedef enum {
	DB_MEM_LOCK=1,
	DB_MEM_LOCKOBJECT=2,
	DB_MEM_LOCKER=3,
	DB_MEM_LOGID=4,
	DB_MEM_TRANSACTION=5,
	DB_MEM_THREAD=6
} DB_MEM_CONFIG;

/*
 * Backup configuration types.
 */
typedef enum {
	DB_BACKUP_READ_COUNT=1,
	DB_BACKUP_READ_SLEEP=2,
	DB_BACKUP_SIZE=3,
	DB_BACKUP_WRITE_DIRECT=4
} DB_BACKUP_CONFIG;

struct __db_env {
	ENV *env;			/* Linked ENV structure */

	/*
	 * The DB_ENV structure can be used concurrently, so field access is
	 * protected.
	 */
	db_mutex_t mtx_db_env;		/* DB_ENV structure mutex */

					/* Error message callback */
	void (*db_errcall) __P((const DB_ENV *, const char *, const char *));
	FILE		*db_errfile;	/* Error message file stream */
	const char	*db_errpfx;	/* Error message prefix */

					/* Other message callback */
	void (*db_msgcall) __P((const DB_ENV *, const char *));
	FILE		*db_msgfile;	/* Other message file stream */

	/* Other application callback functions */
	int   (*app_dispatch) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
	void  (*db_event_func) __P((DB_ENV *, u_int32_t, void *));
	void  (*db_feedback) __P((DB_ENV *, int, int));
	void  (*db_free) __P((void *));
	void  (*db_paniccall) __P((DB_ENV *, int));
	void *(*db_malloc) __P((size_t));
	void *(*db_realloc) __P((void *, size_t));
	int   (*is_alive) __P((DB_ENV *, pid_t, db_threadid_t, u_int32_t));
	void  (*thread_id) __P((DB_ENV *, pid_t *, db_threadid_t *));
	char *(*thread_id_string) __P((DB_ENV *, pid_t, db_threadid_t, char *));

	/* Application specified paths */
	char	*db_blob_dir;		/* Blob file directory */
	char	*db_log_dir;		/* Database log file directory */
	char	*db_md_dir;		/* Persistent metadata directory */
	char	*db_tmp_dir;		/* Database tmp file directory */

	char    *db_create_dir;		/* Create directory for data files */
	char   **db_data_dir;		/* Database data file directories */
	int	 data_cnt;		/* Database data file slots */
	int	 data_next;		/* Next database data file slot */

	char	*intermediate_dir_mode;	/* Intermediate directory perms */

	long	 shm_key;		/* shmget key */

	char	*passwd;		/* Cryptography support */
	size_t	 passwd_len;

	/* Private handle references */
	void	*app_private;		/* Application-private handle */
	void	*api1_internal;		/* C++, Perl API private */
	void	*api2_internal;		/* Java API private */

	u_int32_t	verbose;	/* DB_VERB_XXX flags */

	u_int32_t	blob_threshold;	/* Blob threshold record size */

	/* Mutex configuration */
	u_int32_t	mutex_align;	/* Mutex alignment */
	u_int32_t	mutex_cnt;	/* Number of mutexes to configure */
	u_int32_t	mutex_inc;	/* Number of mutexes to add */
	u_int32_t	mutex_max;	/* Max number of mutexes */
	u_int32_t	mutex_tas_spins;/* Test-and-set spin count */

	/* Locking configuration */
	u_int8_t       *lk_conflicts;	/* Two dimensional conflict matrix */
	int		lk_modes;	/* Number of lock modes in table */
	u_int32_t	lk_detect;	/* Deadlock detect on all conflicts */
	u_int32_t	lk_max;	/* Maximum number of locks */
	u_int32_t	lk_max_lockers;/* Maximum number of lockers */
	u_int32_t	lk_max_objects;/* Maximum number of locked objects */
	u_int32_t	lk_init;	/* Initial number of locks */
	u_int32_t	lk_init_lockers;/* Initial number of lockers */
	u_int32_t	lk_init_objects;/* Initial number of locked objects */
	u_int32_t	lk_partitions ;/* Number of object partitions */
	db_timeout_t	lk_timeout;	/* Lock timeout period */
	/* Used during initialization */
	u_int32_t	locker_t_size;	/* Locker hash table size. */
	u_int32_t	object_t_size;	/* Object hash table size. */

	/* Logging configuration */
	u_int32_t	lg_bsize;	/* Buffer size */
	u_int32_t	lg_fileid_init;	/* Initial allocation for fname structs */
	int		lg_filemode;	/* Log file permission mode */
	u_int32_t	lg_regionmax;	/* Region size */
	u_int32_t	lg_size;	/* Log file size */
	u_int32_t	lg_flags;	/* Log configuration */

	/* Memory pool configuration */
	u_int32_t	mp_gbytes;	/* Cache size: GB */
	u_int32_t	mp_bytes;	/* Cache size: bytes */
	u_int32_t	mp_max_gbytes;	/* Maximum cache size: GB */
	u_int32_t	mp_max_bytes;	/* Maximum cache size: bytes */
	size_t		mp_mmapsize;	/* Maximum file size for mmap */
	int		mp_maxopenfd;	/* Maximum open file descriptors */
	int		mp_maxwrite;	/* Maximum buffers to write */
	u_int		mp_ncache;	/* Initial number of cache regions */
	u_int32_t	mp_pagesize;	/* Average page size */
	u_int32_t	mp_tablesize;	/* Approximate hash table size */
	u_int32_t	mp_mtxcount;	/* Number of mutexs */
					/* Sleep after writing max buffers */
	db_timeout_t	mp_maxwrite_sleep;

	/* Transaction configuration */
	u_int32_t	tx_init;	/* Initial number of transactions */
	u_int32_t	tx_max;		/* Maximum number of transactions */
	time_t		tx_timestamp;	/* Recover to specific timestamp */
	db_timeout_t	tx_timeout;	/* Timeout for transactions */

	/* Thread tracking configuration */
	u_int32_t	thr_init;	/* Thread count */
	u_int32_t	thr_max;	/* Thread max */
	roff_t		memory_max;	/* Maximum region memory */

	/*
	 * The following fields are not strictly user-owned, but they outlive
	 * the ENV structure, and so are stored here.
	 */
	DB_FH		*registry;	/* DB_REGISTER file handle */
	u_int32_t	registry_off;	/*
					 * Offset of our slot.  We can't use
					 * off_t because its size depends on
					 * build settings.
					 */
        db_timeout_t	envreg_timeout; /* DB_REGISTER wait timeout */

#define	DB_ENV_AUTO_COMMIT	0x00000001 /* DB_AUTO_COMMIT */
#define	DB_ENV_CDB_ALLDB	0x00000002 /* CDB environment wide locking */
#define	DB_ENV_FAILCHK		0x00000004 /* Failchk is running */
#define	DB_ENV_DIRECT_DB	0x00000008 /* DB_DIRECT_DB set */
#define	DB_ENV_DSYNC_DB		0x00000010 /* DB_DSYNC_DB set */
#define	DB_ENV_DATABASE_LOCKING	0x00000020 /* Try database-level locking */
#define	DB_ENV_MULTIVERSION	0x00000040 /* DB_MULTIVERSION set */
#define	DB_ENV_NOLOCKING	0x00000080 /* DB_NOLOCKING set */
#define	DB_ENV_NOMMAP		0x00000100 /* DB_NOMMAP set */
#define	DB_ENV_NOPANIC		0x00000200 /* Okay if panic set */
#define	DB_ENV_OVERWRITE	0x00000400 /* DB_OVERWRITE set */
#define	DB_ENV_REGION_INIT	0x00000800 /* DB_REGION_INIT set */
#define	DB_ENV_TIME_NOTGRANTED	0x00001000 /* DB_TIME_NOTGRANTED set */
#define	DB_ENV_TXN_NOSYNC	0x00002000 /* DB_TXN_NOSYNC set */
#define	DB_ENV_TXN_NOWAIT	0x00004000 /* DB_TXN_NOWAIT set */
#define	DB_ENV_TXN_SNAPSHOT	0x00008000 /* DB_TXN_SNAPSHOT set */
#define	DB_ENV_TXN_WRITE_NOSYNC	0x00010000 /* DB_TXN_WRITE_NOSYNC set */
#define	DB_ENV_YIELDCPU		0x00020000 /* DB_YIELDCPU set */
#define DB_ENV_HOTBACKUP	0x00040000 /* DB_HOTBACKUP_IN_PROGRESS set */
#define DB_ENV_NOFLUSH		0x00080000 /* DB_NOFLUSH set */
	u_int32_t flags;

	/* DB_ENV PUBLIC HANDLE LIST BEGIN */
	int  (*add_data_dir) __P((DB_ENV *, const char *));
	int  (*backup)	__P((DB_ENV *, const char *, u_int32_t));
	int  (*cdsgroup_begin) __P((DB_ENV *, DB_TXN **));
	int  (*close) __P((DB_ENV *, u_int32_t));
	int  (*dbbackup) __P((DB_ENV *, const char *, const char *, u_int32_t));
	int  (*dbremove) __P((DB_ENV *,
		DB_TXN *, const char *, const char *, u_int32_t));
	int  (*dbrename) __P((DB_ENV *,
		DB_TXN *, const char *, const char *, const char *, u_int32_t));
	void (*err) __P((const DB_ENV *, int, const char *, ...));
	void (*errx) __P((const DB_ENV *, const char *, ...));
	int  (*failchk) __P((DB_ENV *, u_int32_t));
	int  (*fileid_reset) __P((DB_ENV *, const char *, u_int32_t));
	int  (*get_alloc) __P((DB_ENV *, void *(**)(size_t),
		void *(**)(void *, size_t), void (**)(void *)));
	int  (*get_app_dispatch)
		__P((DB_ENV *, int (**)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
	int  (*get_blob_dir) __P((DB_ENV *, const char **));
	int  (*get_blob_threshold) __P((DB_ENV*, u_int32_t *));
	int  (*get_cache_max) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*get_cachesize) __P((DB_ENV *, u_int32_t *, u_int32_t *, int *));
	int  (*get_create_dir) __P((DB_ENV *, const char **));
	int  (*get_data_dirs) __P((DB_ENV *, const char ***));
	int  (*get_data_len) __P((DB_ENV *, u_int32_t *));
	int  (*get_backup_callbacks) __P((DB_ENV *,
		int (**)(DB_ENV *, const char *, const char *, void **),
		int (**)(DB_ENV *, u_int32_t, u_int32_t, u_int32_t, u_int8_t *, void *),
		int (**)(DB_ENV *, const char *, void *)));
	int  (*get_backup_config) __P((DB_ENV *, DB_BACKUP_CONFIG, u_int32_t *));
	int  (*get_encrypt_flags) __P((DB_ENV *, u_int32_t *));
	void (*get_errcall) __P((DB_ENV *,
		void (**)(const DB_ENV *, const char *, const char *)));
	void (*get_errfile) __P((DB_ENV *, FILE **));
	void (*get_errpfx) __P((DB_ENV *, const char **));
	int  (*get_flags) __P((DB_ENV *, u_int32_t *));
	int  (*get_feedback) __P((DB_ENV *, void (**)(DB_ENV *, int, int)));
	int  (*get_home) __P((DB_ENV *, const char **));
	int  (*get_intermediate_dir_mode) __P((DB_ENV *, const char **));
	int  (*get_isalive) __P((DB_ENV *,
		int (**)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
	int  (*get_lg_bsize) __P((DB_ENV *, u_int32_t *));
	int  (*get_lg_dir) __P((DB_ENV *, const char **));
	int  (*get_lg_filemode) __P((DB_ENV *, int *));
	int  (*get_lg_max) __P((DB_ENV *, u_int32_t *));
	int  (*get_lg_regionmax) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_conflicts) __P((DB_ENV *, const u_int8_t **, int *));
	int  (*get_lk_detect) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_lockers) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_locks) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_objects) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_partitions) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_priority) __P((DB_ENV *, u_int32_t, u_int32_t *));
	int  (*get_lk_tablesize) __P((DB_ENV *, u_int32_t *));
	int  (*get_memory_init) __P((DB_ENV *, DB_MEM_CONFIG, u_int32_t *));
	int  (*get_memory_max) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*get_metadata_dir) __P((DB_ENV *, const char **));
	int  (*get_mp_max_openfd) __P((DB_ENV *, int *));
	int  (*get_mp_max_write) __P((DB_ENV *, int *, db_timeout_t *));
	int  (*get_mp_mmapsize) __P((DB_ENV *, size_t *));
	int  (*get_mp_mtxcount) __P((DB_ENV *, u_int32_t *));
	int  (*get_mp_pagesize) __P((DB_ENV *, u_int32_t *));
	int  (*get_mp_tablesize) __P((DB_ENV *, u_int32_t *));
	void (*get_msgcall)
		__P((DB_ENV *, void (**)(const DB_ENV *, const char *)));
	void (*get_msgfile) __P((DB_ENV *, FILE **));
	int  (*get_open_flags) __P((DB_ENV *, u_int32_t *));
	int  (*get_shm_key) __P((DB_ENV *, long *));
	int  (*get_thread_count) __P((DB_ENV *, u_int32_t *));
	int  (*get_thread_id_fn)
		__P((DB_ENV *, void (**)(DB_ENV *, pid_t *, db_threadid_t *)));
	int  (*get_thread_id_string_fn) __P((DB_ENV *,
		char *(**)(DB_ENV *, pid_t, db_threadid_t, char *)));
	int  (*get_timeout) __P((DB_ENV *, db_timeout_t *, u_int32_t));
	int  (*get_tmp_dir) __P((DB_ENV *, const char **));
	int  (*get_tx_max) __P((DB_ENV *, u_int32_t *));
	int  (*get_tx_timestamp) __P((DB_ENV *, time_t *));
	int  (*get_verbose) __P((DB_ENV *, u_int32_t, int *));
	int  (*is_bigendian) __P((void));
	int  (*lock_detect) __P((DB_ENV *, u_int32_t, u_int32_t, int *));
	int  (*lock_get) __P((DB_ENV *,
		u_int32_t, u_int32_t, DBT *, db_lockmode_t, DB_LOCK *));
	int  (*lock_id) __P((DB_ENV *, u_int32_t *));
	int  (*lock_id_free) __P((DB_ENV *, u_int32_t));
	int  (*lock_put) __P((DB_ENV *, DB_LOCK *));
	int  (*lock_stat) __P((DB_ENV *, DB_LOCK_STAT **, u_int32_t));
	int  (*lock_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*lock_vec) __P((DB_ENV *,
		u_int32_t, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
	int  (*log_archive) __P((DB_ENV *, char **[], u_int32_t));
	int  (*log_cursor) __P((DB_ENV *, DB_LOGC **, u_int32_t));
	int  (*log_file) __P((DB_ENV *, const DB_LSN *, char *, size_t));
	int  (*log_flush) __P((DB_ENV *, const DB_LSN *));
	int  (*log_get_config) __P((DB_ENV *, u_int32_t, int *));
	int  (*log_printf) __P((DB_ENV *, DB_TXN *, const char *, ...));
	int  (*log_put) __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
	int  (*log_put_record) __P((DB_ENV *, DB *, DB_TXN *, DB_LSN *,
		u_int32_t, u_int32_t, u_int32_t, u_int32_t,
		DB_LOG_RECSPEC *, ...));
	int  (*log_read_record) __P((DB_ENV *, DB **,
		void *, void *, DB_LOG_RECSPEC *, u_int32_t, void **));
	int  (*log_set_config) __P((DB_ENV *, u_int32_t, int));
	int  (*log_stat) __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
	int  (*log_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*log_verify) __P((DB_ENV *, const DB_LOG_VERIFY_CONFIG *));
	int  (*lsn_reset) __P((DB_ENV *, const char *, u_int32_t));
	int  (*memp_fcreate) __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
	int  (*memp_register) __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t,
		void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
	int  (*memp_stat) __P((DB_ENV *,
		DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
	int  (*memp_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*memp_sync) __P((DB_ENV *, DB_LSN *));
	int  (*memp_trickle) __P((DB_ENV *, int, int *));
	int  (*mutex_alloc) __P((DB_ENV *, u_int32_t, db_mutex_t *));
	int  (*mutex_free) __P((DB_ENV *, db_mutex_t));
	int  (*mutex_get_align) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_increment) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_init) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_max) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_tas_spins) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_lock) __P((DB_ENV *, db_mutex_t));
	int  (*mutex_set_align) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_increment) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_init) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_max) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_tas_spins) __P((DB_ENV *, u_int32_t));
	int  (*mutex_stat) __P((DB_ENV *, DB_MUTEX_STAT **, u_int32_t));
	int  (*mutex_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*mutex_unlock) __P((DB_ENV *, db_mutex_t));
	int  (*open) __P((DB_ENV *, const char *, u_int32_t, int));
	int  (*remove) __P((DB_ENV *, const char *, u_int32_t));
	int  (*rep_elect) __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
	int  (*rep_flush) __P((DB_ENV *));
	int  (*rep_get_clockskew) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_config) __P((DB_ENV *, u_int32_t, int *));
	int  (*rep_get_limit) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_nsites) __P((DB_ENV *, u_int32_t *));
	int  (*rep_get_priority) __P((DB_ENV *, u_int32_t *));
	int  (*rep_get_request) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_timeout) __P((DB_ENV *, int, u_int32_t *));
	int  (*rep_process_message)
		__P((DB_ENV *, DBT *, DBT *, int, DB_LSN *));
	int  (*rep_set_clockskew) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_config) __P((DB_ENV *, u_int32_t, int));
	int  (*rep_set_limit) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_nsites) __P((DB_ENV *, u_int32_t));
	int  (*rep_set_priority) __P((DB_ENV *, u_int32_t));
	int  (*rep_set_request) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_timeout) __P((DB_ENV *, int, db_timeout_t));
	int  (*rep_set_transport) __P((DB_ENV *, int, int (*)(DB_ENV *,
		const DBT *, const DBT *, const DB_LSN *, int, u_int32_t)));
	int  (*rep_set_view) __P((DB_ENV *, int (*)(DB_ENV *,
		const char *, int *, u_int32_t)));
	int  (*rep_start) __P((DB_ENV *, DBT *, u_int32_t));
	int  (*rep_stat) __P((DB_ENV *, DB_REP_STAT **, u_int32_t));
	int  (*rep_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*rep_sync) __P((DB_ENV *, u_int32_t));
	int  (*repmgr_channel) __P((DB_ENV *, int, DB_CHANNEL **, u_int32_t));
	int  (*repmgr_get_ack_policy) __P((DB_ENV *, int *));
	int  (*repmgr_get_incoming_queue_max)
		__P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*repmgr_local_site) __P((DB_ENV *, DB_SITE **));
	int  (*repmgr_msg_dispatch) __P((DB_ENV *,
		void (*)(DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t),
		u_int32_t));
	int  (*repmgr_set_ack_policy) __P((DB_ENV *, int));
	int  (*repmgr_set_incoming_queue_max)
		__P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*repmgr_site)
		__P((DB_ENV *, const char *, u_int, DB_SITE**, u_int32_t));
	int  (*repmgr_site_by_eid) __P((DB_ENV *, int, DB_SITE**));
	int  (*repmgr_site_list) __P((DB_ENV *, u_int *, DB_REPMGR_SITE **));
	int  (*repmgr_start) __P((DB_ENV *, int, u_int32_t));
	int  (*repmgr_stat) __P((DB_ENV *, DB_REPMGR_STAT **, u_int32_t));
	int  (*repmgr_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*set_alloc) __P((DB_ENV *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	int  (*set_app_dispatch)
		__P((DB_ENV *, int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
	int  (*set_blob_dir) __P((DB_ENV *, const char *));
	int  (*set_blob_threshold) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_cache_max) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_cachesize) __P((DB_ENV *, u_int32_t, u_int32_t, int));
	int  (*set_create_dir) __P((DB_ENV *, const char *));
	int  (*set_data_dir) __P((DB_ENV *, const char *));
	int  (*set_data_len) __P((DB_ENV *, u_int32_t));
	int  (*set_backup_callbacks) __P((DB_ENV *,
		int (*)(DB_ENV *, const char *, const char *, void **),
		int (*)(DB_ENV *, u_int32_t,
		    u_int32_t, u_int32_t, u_int8_t *, void *),
		int (*)(DB_ENV *, const char *, void *)));
	int  (*set_backup_config) __P((DB_ENV *, DB_BACKUP_CONFIG, u_int32_t));
	int  (*set_encrypt) __P((DB_ENV *, const char *, u_int32_t));
	void (*set_errcall) __P((DB_ENV *,
		void (*)(const DB_ENV *, const char *, const char *)));
	void (*set_errfile) __P((DB_ENV *, FILE *));
	void (*set_errpfx) __P((DB_ENV *, const char *));
	int  (*set_event_notify)
		__P((DB_ENV *, void (*)(DB_ENV *, u_int32_t, void *)));
	int  (*set_feedback) __P((DB_ENV *, void (*)(DB_ENV *, int, int)));
	int  (*set_flags) __P((DB_ENV *, u_int32_t, int));
	int  (*set_intermediate_dir_mode) __P((DB_ENV *, const char *));
	int  (*set_isalive) __P((DB_ENV *,
		int (*)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
	int  (*set_lg_bsize) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_dir) __P((DB_ENV *, const char *));
	int  (*set_lg_filemode) __P((DB_ENV *, int));
	int  (*set_lg_max) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_regionmax) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_conflicts) __P((DB_ENV *, u_int8_t *, int));
	int  (*set_lk_detect) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_lockers) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_locks) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_objects) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_partitions) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_priority) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_lk_tablesize) __P((DB_ENV *, u_int32_t));
	int  (*set_memory_init) __P((DB_ENV *, DB_MEM_CONFIG, u_int32_t));
	int  (*set_memory_max) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_metadata_dir) __P((DB_ENV *, const char *));
	int  (*set_mp_max_openfd) __P((DB_ENV *, int));
	int  (*set_mp_max_write) __P((DB_ENV *, int, db_timeout_t));
	int  (*set_mp_mmapsize) __P((DB_ENV *, size_t));
	int  (*set_mp_mtxcount) __P((DB_ENV *, u_int32_t));
	int  (*set_mp_pagesize) __P((DB_ENV *, u_int32_t));
	int  (*set_mp_tablesize) __P((DB_ENV *, u_int32_t));
	void (*set_msgcall)
		__P((DB_ENV *, void (*)(const DB_ENV *, const char *)));
	void (*set_msgfile) __P((DB_ENV *, FILE *));
	int  (*set_paniccall) __P((DB_ENV *, void (*)(DB_ENV *, int)));
	int  (*set_shm_key) __P((DB_ENV *, long));
	int  (*set_thread_count) __P((DB_ENV *, u_int32_t));
	int  (*set_thread_id)
		__P((DB_ENV *, void (*)(DB_ENV *, pid_t *, db_threadid_t *)));
	int  (*set_thread_id_string) __P((DB_ENV *,
		char *(*)(DB_ENV *, pid_t, db_threadid_t, char *)));
	int  (*set_timeout) __P((DB_ENV *, db_timeout_t, u_int32_t));
	int  (*set_tmp_dir) __P((DB_ENV *, const char *));
	int  (*set_tx_max) __P((DB_ENV *, u_int32_t));
	int  (*set_tx_timestamp) __P((DB_ENV *, time_t *));
	int  (*set_verbose) __P((DB_ENV *, u_int32_t, int));
	int  (*txn_applied) __P((DB_ENV *,
		DB_TXN_TOKEN *, db_timeout_t, u_int32_t));
	int  (*stat_print) __P((DB_ENV *, u_int32_t));
	int  (*txn_begin) __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
	int  (*txn_checkpoint) __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
	int  (*txn_recover) __P((DB_ENV *,
		DB_PREPLIST *, long, long *, u_int32_t));
	int  (*txn_stat) __P((DB_ENV *, DB_TXN_STAT **, u_int32_t));
	int  (*txn_stat_print) __P((DB_ENV *, u_int32_t));
	/* DB_ENV PUBLIC HANDLE LIST END */

	/* DB_ENV PRIVATE HANDLE LIST BEGIN */
	int  (*prdbt) __P((DBT *, int, const char *, void *,
		int (*)(void *, const void *), int, int, int));
	/* DB_ENV PRIVATE HANDLE LIST END */
};

/*
 * Dispatch structure for recovery, log verification and print routines. Since
 * internal and external routines take different arguments (ENV versus DB_ENV),
 * we need something more elaborate than a single pointer and size.
 */
struct __db_distab {
	int   (**int_dispatch) __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t	int_size;
	int   (**ext_dispatch) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
	size_t	ext_size;
};

/*
 * Log verification configuration structure.
 */
struct __db_logvrfy_config {
	int continue_after_fail, verbose;
	u_int32_t cachesize;
	const char *temp_envhome;
	const char *dbfile, *dbname;
	DB_LSN start_lsn, end_lsn;
	time_t start_time, end_time;
};

struct __db_channel {
	CHANNEL *channel;	/* Pointer to internal state details. */
	int eid;		/* Env. ID passed in constructor. */
	db_timeout_t timeout;

	/* DB_CHANNEL PUBLIC HANDLE LIST BEGIN */
	int (*close) __P((DB_CHANNEL *, u_int32_t));
	int (*send_msg) __P((DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
	int (*send_request) __P((DB_CHANNEL *,
		DBT *, u_int32_t, DBT *, db_timeout_t, u_int32_t));
	int  (*set_timeout) __P((DB_CHANNEL *, db_timeout_t));
	/* DB_CHANNEL PUBLIC HANDLE LIST END */
};

struct __db_site {
	ENV *env;
	int eid;
	const char *host;
	u_int port;
	u_int32_t flags;

	/* DB_SITE PUBLIC HANDLE LIST BEGIN */
	int (*get_address) __P((DB_SITE *, const char **, u_int *));
	int (*get_config) __P((DB_SITE *, u_int32_t, u_int32_t *));
	int (*get_eid) __P((DB_SITE *, int *));
	int (*set_config) __P((DB_SITE *, u_int32_t, u_int32_t));
	int (*remove) __P((DB_SITE *));
	int (*close) __P((DB_SITE *));
	/* DB_SITE PUBLIC HANDLE LIST END */
};

#if DB_DBM_HSEARCH != 0
/*******************************************************
 * Dbm/Ndbm historic interfaces.
 *******************************************************/
typedef struct __db DBM;

#define	DBM_INSERT	0		/* Flags to dbm_store(). */
#define	DBM_REPLACE	1

/*
 * The DB support for ndbm(3) always appends this suffix to the
 * file name to avoid overwriting the user's original database.
 */
#define	DBM_SUFFIX	".db"

#if defined(_XPG4_2)
typedef struct {
	char *dptr;
	size_t dsize;
} datum;
#else
typedef struct {
	char *dptr;
	int dsize;
} datum;
#endif

/*
 * Translate NDBM calls into DB calls so that DB doesn't step on the
 * application's name space.
 */
#define	dbm_clearerr(a)		__db_ndbm_clearerr(a)
#define	dbm_close(a)		__db_ndbm_close(a)
#define	dbm_delete(a, b)	__db_ndbm_delete(a, b)
#define	dbm_dirfno(a)		__db_ndbm_dirfno(a)
#define	dbm_error(a)		__db_ndbm_error(a)
#define	dbm_fetch(a, b)		__db_ndbm_fetch(a, b)
#define	dbm_firstkey(a)		__db_ndbm_firstkey(a)
#define	dbm_nextkey(a)		__db_ndbm_nextkey(a)
#define	dbm_open(a, b, c)	__db_ndbm_open(a, b, c)
#define	dbm_pagfno(a)		__db_ndbm_pagfno(a)
#define	dbm_rdonly(a)		__db_ndbm_rdonly(a)
#define	dbm_store(a, b, c, d) \
	__db_ndbm_store(a, b, c, d)

/*
 * Translate DBM calls into DB calls so that DB doesn't step on the
 * application's name space.
 *
 * The global variables dbrdonly, dirf and pagf were not retained when 4BSD
 * replaced the dbm interface with ndbm, and are not supported here.
 */
#define	dbminit(a)	__db_dbm_init(a)
#define	dbmclose	__db_dbm_close
#if !defined(__cplusplus)
#define	delete(a)	__db_dbm_delete(a)
#endif
#define	fetch(a)	__db_dbm_fetch(a)
#define	firstkey	__db_dbm_firstkey
#define	nextkey(a)	__db_dbm_nextkey(a)
#define	store(a, b)	__db_dbm_store(a, b)

/*******************************************************
 * Hsearch historic interface.
 *******************************************************/
typedef enum {
	FIND, ENTER
} ACTION;

typedef struct entry {
	char *key;
	char *data;
} ENTRY;

#define	hcreate(a)	__db_hcreate(a)
#define	hdestroy	__db_hdestroy
#define	hsearch(a, b)	__db_hsearch(a, b)

#endif /* DB_DBM_HSEARCH */

#if defined(__cplusplus)
}
#endif

/* Restore default compiler warnings */
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif /* !_DB_H_ */
/* DO NOT EDIT: automatically built by dist/s_apiflags. */
#define	DB_AGGRESSIVE				0x00000001
#define	DB_ARCH_ABS				0x00000001
#define	DB_ARCH_DATA				0x00000002
#define	DB_ARCH_LOG				0x00000004
#define	DB_ARCH_REMOVE				0x00000008
#define	DB_AUTO_COMMIT				0x00000100
#define	DB_BACKUP_CLEAN				0x00000002
#define	DB_BACKUP_FILES				0x00000008
#define	DB_BACKUP_NO_LOGS			0x00000010
#define	DB_BACKUP_SINGLE_DIR			0x00000020
#define	DB_BACKUP_UPDATE			0x00000040
#define	DB_BOOTSTRAP_HELPER			0x00000001
#define	DB_CDB_ALLDB				0x00000040
#define	DB_CHKSUM				0x00000008
#define	DB_CKP_INTERNAL				0x00000002
#define	DB_CREATE				0x00000001
#define	DB_CURSOR_BULK				0x00000001
#define	DB_CURSOR_TRANSIENT			0x00000008
#define	DB_CXX_NO_EXCEPTIONS			0x00000002
#define	DB_DATABASE_LOCKING			0x00000080
#define	DB_DIRECT				0x00000020
#define	DB_DIRECT_DB				0x00000200
#define	DB_DSYNC_DB				0x00000400
#define	DB_DUP					0x00000010
#define	DB_DUPSORT				0x00000002
#define	DB_DURABLE_UNKNOWN			0x00000040
#define	DB_ENCRYPT				0x00000001
#define	DB_ENCRYPT_AES				0x00000001
#define	DB_EXCL					0x00000004
#define	DB_EXTENT				0x00000100
#define	DB_FAILCHK				0x00000010
#define	DB_FAILCHK_ISALIVE			0x00000040
#define	DB_FAST_STAT				0x00000001
#define	DB_FCNTL_LOCKING			0x00000800
#define	DB_FLUSH				0x00000002
#define	DB_FORCE				0x00000001
#define	DB_FORCESYNC				0x00000001
#define	DB_FOREIGN_ABORT			0x00000001
#define	DB_FOREIGN_CASCADE			0x00000002
#define	DB_FOREIGN_NULLIFY			0x00000004
#define	DB_FREELIST_ONLY			0x00000001
#define	DB_FREE_SPACE				0x00000002
#define	DB_GROUP_CREATOR			0x00000002
#define	DB_HOTBACKUP_IN_PROGRESS		0x00000800
#define	DB_IGNORE_LEASE				0x00001000
#define	DB_IMMUTABLE_KEY			0x00000002
#define	DB_INIT_CDB				0x00000080
#define	DB_INIT_LOCK				0x00000100
#define	DB_INIT_LOG				0x00000200
#define	DB_INIT_MPOOL				0x00000400
#define	DB_INIT_MUTEX				0x00000800
#define	DB_INIT_REP				0x00001000
#define	DB_INIT_TXN				0x00002000
#define	DB_INORDER				0x00000020
#define	DB_INTERNAL_BLOB_DB			0x00001000
#define	DB_INTERNAL_PERSISTENT_DB		0x00002000
#define	DB_INTERNAL_TEMPORARY_DB		0x00004000
#define	DB_JOIN_NOSORT				0x00000001
#define	DB_LEGACY				0x00000004
#define	DB_LOCAL_SITE				0x00000008
#define	DB_LOCKDOWN				0x00004000
#define	DB_LOCK_CHECK				0x00000001
#define	DB_LOCK_IGNORE_REC			0x00000002
#define	DB_LOCK_NOWAIT				0x00000004
#define	DB_LOCK_RECORD				0x00000008
#define	DB_LOCK_SET_TIMEOUT			0x00000010
#define	DB_LOCK_SWITCH				0x00000020
#define	DB_LOCK_UPGRADE				0x00000040
#define	DB_LOG_AUTO_REMOVE			0x00000001
#define	DB_LOG_BLOB				0x00000002
#define	DB_LOG_CHKPNT				0x00000001
#define	DB_LOG_COMMIT				0x00000004
#define	DB_LOG_DIRECT				0x00000004
#define	DB_LOG_DSYNC				0x00000008
#define	DB_LOG_IN_MEMORY			0x00000010
#define	DB_LOG_NOCOPY				0x00000008
#define	DB_LOG_NOT_DURABLE			0x00000010
#define	DB_LOG_NO_DATA				0x00000002
#define	DB_LOG_VERIFY_CAF			0x00000001
#define	DB_LOG_VERIFY_DBFILE			0x00000002
#define	DB_LOG_VERIFY_ERR			0x00000004
#define	DB_LOG_VERIFY_FORWARD			0x00000008
#define	DB_LOG_VERIFY_INTERR			0x00000010
#define	DB_LOG_VERIFY_PARTIAL			0x00000020
#define	DB_LOG_VERIFY_VERBOSE			0x00000040
#define	DB_LOG_VERIFY_WARNING			0x00000080
#define	DB_LOG_WRNOSYNC				0x00000020
#define	DB_LOG_ZERO				0x00000020
#define	DB_MPOOL_CREATE				0x00000001
#define	DB_MPOOL_DIRTY				0x00000002
#define	DB_MPOOL_DISCARD			0x00000001
#define	DB_MPOOL_EDIT				0x00000004
#define	DB_MPOOL_FREE				0x00000008
#define	DB_MPOOL_LAST				0x00000010
#define	DB_MPOOL_NEW				0x00000020
#define	DB_MPOOL_NOFILE				0x00000001
#define	DB_MPOOL_NOLOCK				0x00000004
#define	DB_MPOOL_TRY				0x00000040
#define	DB_MPOOL_UNLINK				0x00000002
#define	DB_MULTIPLE				0x00000800
#define	DB_MULTIPLE_KEY				0x00004000
#define	DB_MULTIVERSION				0x00000008
#define	DB_MUTEX_ALLOCATED			0x00000001
#define	DB_MUTEX_LOCKED				0x00000002
#define	DB_MUTEX_LOGICAL_LOCK			0x00000004
#define	DB_MUTEX_PROCESS_ONLY			0x00000008
#define	DB_MUTEX_SELF_BLOCK			0x00000010
#define	DB_MUTEX_SHARED				0x00000020
#define	DB_NOERROR				0x00008000
#define	DB_NOFLUSH				0x00001000
#define	DB_NOLOCKING				0x00002000
#define	DB_NOMMAP				0x00000010
#define	DB_NOORDERCHK				0x00000002
#define	DB_NOPANIC				0x00004000
#define	DB_NOSYNC				0x00000001
#define	DB_NO_AUTO_COMMIT			0x00010000
#define	DB_NO_CHECKPOINT			0x00008000
#define	DB_ODDFILESIZE				0x00000080
#define	DB_ORDERCHKONLY				0x00000004
#define	DB_OVERWRITE				0x00008000
#define	DB_PANIC_ENVIRONMENT			0x00010000
#define	DB_PRINTABLE				0x00000008
#define	DB_PRIVATE				0x00010000
#define	DB_PR_PAGE				0x00000010
#define	DB_PR_RECOVERYTEST			0x00000020
#define	DB_RDONLY				0x00000400
#define	DB_RDWRMASTER				0x00020000
#define	DB_READ_COMMITTED			0x00000400
#define	DB_READ_UNCOMMITTED			0x00000200
#define	DB_RECNUM				0x00000040
#define	DB_RECOVER				0x00000002
#define	DB_RECOVER_FATAL			0x00020000
#define	DB_REGION_INIT				0x00020000
#define	DB_REGISTER				0x00040000
#define	DB_RENUMBER				0x00000080
#define	DB_REPMGR_CONF_2SITE_STRICT		0x00000001
#define	DB_REPMGR_CONF_ELECTIONS		0x00000002
#define	DB_REPMGR_NEED_RESPONSE			0x00000001
#define	DB_REPMGR_PEER				0x00000010
#define	DB_REP_ANYWHERE				0x00000001
#define	DB_REP_CLIENT				0x00000001
#define	DB_REP_CONF_AUTOINIT			0x00000004
#define	DB_REP_CONF_AUTOROLLBACK		0x00000008
#define	DB_REP_CONF_BULK			0x00000010
#define	DB_REP_CONF_DELAYCLIENT			0x00000020
#define	DB_REP_CONF_INMEM			0x00000040
#define	DB_REP_CONF_LEASE			0x00000080
#define	DB_REP_CONF_NOWAIT			0x00000100
#define	DB_REP_ELECTION				0x00000004
#define	DB_REP_MASTER				0x00000002
#define	DB_REP_NOBUFFER				0x00000002
#define	DB_REP_PERMANENT			0x00000004
#define	DB_REP_REREQUEST			0x00000008
#define	DB_REVSPLITOFF				0x00000100
#define	DB_RMW					0x00002000
#define	DB_SALVAGE				0x00000040
#define	DB_SA_SKIPFIRSTKEY			0x00000080
#define	DB_SA_UNKNOWNKEY			0x00000100
#define	DB_SEQ_DEC				0x00000001
#define	DB_SEQ_INC				0x00000002
#define	DB_SEQ_RANGE_SET			0x00000004
#define	DB_SEQ_WRAP				0x00000008
#define	DB_SEQ_WRAPPED				0x00000010
#define	DB_SET_LOCK_TIMEOUT			0x00000001
#define	DB_SET_REG_TIMEOUT			0x00000004
#define	DB_SET_TXN_NOW				0x00000008
#define	DB_SET_TXN_TIMEOUT			0x00000002
#define	DB_SHALLOW_DUP				0x00000100
#define	DB_SNAPSHOT				0x00000200
#define	DB_STAT_ALL				0x00000004
#define	DB_STAT_ALLOC				0x00000008
#define	DB_STAT_CLEAR				0x00000001
#define	DB_STAT_LOCK_CONF			0x00000010
#define	DB_STAT_LOCK_LOCKERS			0x00000020
#define	DB_STAT_LOCK_OBJECTS			0x00000040
#define	DB_STAT_LOCK_PARAMS			0x00000080
#define	DB_STAT_MEMP_HASH			0x00000010
#define	DB_STAT_MEMP_NOERROR			0x00000020
#define	DB_STAT_SUBSYSTEM			0x00000002
#define	DB_STAT_SUMMARY				0x00000010
#define	DB_ST_DUPOK				0x00000200
#define	DB_ST_DUPSET				0x00000400
#define	DB_ST_DUPSORT				0x00000800
#define	DB_ST_IS_RECNO				0x00001000
#define	DB_ST_OVFL_LEAF				0x00002000
#define	DB_ST_RECNUM				0x00004000
#define	DB_ST_RELEN				0x00008000
#define	DB_ST_TOPLEVEL				0x00010000
#define	DB_SYSTEM_MEM				0x00080000
#define	DB_THREAD				0x00000020
#define	DB_TIME_NOTGRANTED			0x00040000
#define	DB_TRUNCATE				0x00040000
#define	DB_TXN_BULK				0x00000010
#define	DB_TXN_FAMILY				0x00000040
#define	DB_TXN_NOSYNC				0x00000001
#define	DB_TXN_NOT_DURABLE			0x00000004
#define	DB_TXN_NOWAIT				0x00000002
#define	DB_TXN_SNAPSHOT				0x00000004
#define	DB_TXN_SYNC				0x00000008
#define	DB_TXN_WAIT				0x00000080
#define	DB_TXN_WRITE_NOSYNC			0x00000020
#define	DB_UNREF				0x00020000
#define	DB_UPGRADE				0x00000001
#define	DB_USE_ENVIRON				0x00000004
#define	DB_USE_ENVIRON_ROOT			0x00000008
#define	DB_VERB_BACKUP				0x00000001
#define	DB_VERB_DEADLOCK			0x00000002
#define	DB_VERB_FILEOPS				0x00000004
#define	DB_VERB_FILEOPS_ALL			0x00000008
#define	DB_VERB_MVCC				0x00000010
#define	DB_VERB_RECOVERY			0x00000020
#define	DB_VERB_REGISTER			0x00000040
#define	DB_VERB_REPLICATION			0x00000080
#define	DB_VERB_REPMGR_CONNFAIL			0x00000100
#define	DB_VERB_REPMGR_MISC			0x00000200
#define	DB_VERB_REP_ELECT			0x00000400
#define	DB_VERB_REP_LEASE			0x00000800
#define	DB_VERB_REP_MISC			0x00001000
#define	DB_VERB_REP_MSGS			0x00002000
#define	DB_VERB_REP_SYNC			0x00004000
#define	DB_VERB_REP_SYSTEM			0x00008000
#define	DB_VERB_REP_TEST			0x00010000
#define	DB_VERB_WAITSFOR			0x00020000
#define	DB_VERIFY				0x00000002
#define	DB_VERIFY_PARTITION			0x00040000
#define	DB_WRITECURSOR				0x00000010
#define	DB_WRITELOCK				0x00000020
#define	DB_WRITEOPEN				0x00080000
#define	DB_XA_CREATE				0x00000001
#define	DB_YIELDCPU				0x00080000

/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_DB_EXT_PROT_IN_
#define	_DB_EXT_PROT_IN_

#if defined(__cplusplus)
extern "C" {
#endif

int db_copy __P((DB_ENV *, const char *, const char *, const char *));
int db_create __P((DB **, DB_ENV *, u_int32_t));
char *db_strerror __P((int));
int db_env_set_func_assert __P((void (*)(const char *, const char *, int)));
int db_env_set_func_close __P((int (*)(int)));
int db_env_set_func_dirfree __P((void (*)(char **, int)));
int db_env_set_func_dirlist __P((int (*)(const char *, char ***, int *)));
int db_env_set_func_exists __P((int (*)(const char *, int *)));
int db_env_set_func_free __P((void (*)(void *)));
int db_env_set_func_fsync __P((int (*)(int)));
int db_env_set_func_ftruncate __P((int (*)(int, off_t)));
int db_env_set_func_ioinfo __P((int (*)(const char *, int, u_int32_t *, u_int32_t *, u_int32_t *)));
int db_env_set_func_malloc __P((void *(*)(size_t)));
int db_env_set_func_file_map __P((int (*)(DB_ENV *, char *, size_t, int, void **), int (*)(DB_ENV *, void *)));
int db_env_set_func_region_map __P((int (*)(DB_ENV *, char *, size_t, int *, void **), int (*)(DB_ENV *, void *)));
int db_env_set_func_pread __P((ssize_t (*)(int, void *, size_t, off_t)));
int db_env_set_func_pwrite __P((ssize_t (*)(int, const void *, size_t, off_t)));
int db_env_set_func_open __P((int (*)(const char *, int, ...)));
int db_env_set_func_read __P((ssize_t (*)(int, void *, size_t)));
int db_env_set_func_realloc __P((void *(*)(void *, size_t)));
int db_env_set_func_rename __P((int (*)(const char *, const char *)));
int db_env_set_func_seek __P((int (*)(int, off_t, int)));
int db_env_set_func_unlink __P((int (*)(const char *)));
int db_env_set_func_write __P((ssize_t (*)(int, const void *, size_t)));
int db_env_set_func_yield __P((int (*)(u_long, u_long)));
int db_env_create __P((DB_ENV **, u_int32_t));
char *db_version __P((int *, int *, int *));
char *db_full_version __P((int *, int *, int *, int *, int *));
int log_compare __P((const DB_LSN *, const DB_LSN *));
#if defined(DB_WIN32) && !defined(DB_WINCE)
int db_env_set_win_security __P((SECURITY_ATTRIBUTES *sa));
#endif
int db_sequence_create __P((DB_SEQUENCE **, DB *, u_int32_t));
#if DB_DBM_HSEARCH != 0
int	 __db_ndbm_clearerr __P((DBM *));
void	 __db_ndbm_close __P((DBM *));
int	 __db_ndbm_delete __P((DBM *, datum));
int	 __db_ndbm_dirfno __P((DBM *));
int	 __db_ndbm_error __P((DBM *));
datum __db_ndbm_fetch __P((DBM *, datum));
datum __db_ndbm_firstkey __P((DBM *));
datum __db_ndbm_nextkey __P((DBM *));
DBM	*__db_ndbm_open __P((const char *, int, int));
int	 __db_ndbm_pagfno __P((DBM *));
int	 __db_ndbm_rdonly __P((DBM *));
int	 __db_ndbm_store __P((DBM *, datum, datum, int));
int	 __db_dbm_close __P((void));
int	 __db_dbm_delete __P((datum));
datum __db_dbm_fetch __P((datum));
datum __db_dbm_firstkey __P((void));
int	 __db_dbm_init __P((char *));
datum __db_dbm_nextkey __P((datum));
int	 __db_dbm_store __P((datum, datum));
#endif
#if DB_DBM_HSEARCH != 0
int __db_hcreate __P((size_t));
ENTRY *__db_hsearch __P((ENTRY, ACTION));
void __db_hdestroy __P((void));
#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_EXT_PROT_IN_ */
