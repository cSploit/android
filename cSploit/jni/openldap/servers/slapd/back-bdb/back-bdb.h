/* back-bdb.h - bdb back-end header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _BACK_BDB_H_
#define _BACK_BDB_H_

#include <portable.h>
#include "slap.h"
#include <db.h>
#include "alock.h"

LDAP_BEGIN_DECL

#define DB_VERSION_FULL ((DB_VERSION_MAJOR << 24) | (DB_VERSION_MINOR << 16) | DB_VERSION_PATCH)

#define DN_BASE_PREFIX		SLAP_INDEX_EQUALITY_PREFIX
#define DN_ONE_PREFIX	 	'%'
#define DN_SUBTREE_PREFIX 	'@'

#define DBTzero(t)			(memset((t), 0, sizeof(DBT)))
#define DBT2bv(t,bv)		((bv)->bv_val = (t)->data, \
								(bv)->bv_len = (t)->size)
#define bv2DBT(bv,t)		((t)->data = (bv)->bv_val, \
								(t)->size = (bv)->bv_len )

#define BDB_TXN_RETRIES		16

#define BDB_MAX_ADD_LOOP	30

#define BDB_SUFFIX		".bdb"
#define BDB_ID2ENTRY	0
#define BDB_DN2ID		1
#define BDB_NDB			2

/* The bdb on-disk entry format is pretty space-inefficient. Average
 * sized user entries are 3-4K each. You need at least two entries to
 * fit into a single database page, more is better. 64K is BDB's
 * upper bound. Smaller pages are better for concurrency.
 */
#ifndef BDB_ID2ENTRY_PAGESIZE
#define	BDB_ID2ENTRY_PAGESIZE	16384
#endif

#define DEFAULT_CACHE_SIZE     1000

/* The default search IDL stack cache depth */
#define DEFAULT_SEARCH_STACK_DEPTH	16

/* The minimum we can function with */
#define MINIMUM_SEARCH_STACK_DEPTH	8

typedef struct bdb_idl_cache_entry_s {
	struct berval kstr;
	ID      *idl;
	DB      *db;
	int		idl_flags;
	struct bdb_idl_cache_entry_s* idl_lru_prev;
	struct bdb_idl_cache_entry_s* idl_lru_next;
} bdb_idl_cache_entry_t;

/* BDB backend specific entry info */
typedef struct bdb_entry_info {
	struct bdb_entry_info *bei_parent;
	ID bei_id;

	/* we use the bei_id as a lockobj, but we need to make the size != 4
	 * to avoid conflicting with BDB's internal locks. So add a byte here
	 * that is always zero.
	 */
	short bei_lockpad;

	short bei_state;
#define	CACHE_ENTRY_DELETED	1
#define	CACHE_ENTRY_NO_KIDS	2
#define	CACHE_ENTRY_NOT_LINKED	4
#define CACHE_ENTRY_NO_GRANDKIDS	8
#define	CACHE_ENTRY_LOADING	0x10
#define	CACHE_ENTRY_WALKING	0x20
#define	CACHE_ENTRY_ONELEVEL	0x40
#define	CACHE_ENTRY_REFERENCED	0x80
#define	CACHE_ENTRY_NOT_CACHED	0x100
	int bei_finders;

	/*
	 * remaining fields require backend cache lock to access
	 */
	struct berval bei_nrdn;
#ifdef BDB_HIER
	struct berval bei_rdn;
	int	bei_modrdns;	/* track renames */
	int	bei_ckids;	/* number of kids cached */
	int	bei_dkids;	/* number of kids on-disk, plus 1 */
#endif
	Entry	*bei_e;
	Avlnode	*bei_kids;
#ifdef SLAP_ZONE_ALLOC
	struct bdb_info *bei_bdb;
	int bei_zseq;	
#endif
	ldap_pvt_thread_mutex_t	bei_kids_mutex;
	
	struct bdb_entry_info	*bei_lrunext;	/* for cache lru list */
	struct bdb_entry_info	*bei_lruprev;
} EntryInfo;
#undef BEI
#define BEI(e)	((EntryInfo *) ((e)->e_private))

/* for the in-core cache of entries */
typedef struct bdb_cache {
	EntryInfo	*c_eifree;	/* free list */
	Avlnode		*c_idtree;
	EntryInfo	*c_lruhead;	/* lru - add accessed entries here */
	EntryInfo	*c_lrutail;	/* lru - rem lru entries from here */
	EntryInfo	c_dntree;
	ID		c_maxsize;
	ID		c_cursize;
	ID		c_minfree;
	ID		c_eimax;
	ID		c_eiused;	/* EntryInfo's in use */
	ID		c_leaves;	/* EntryInfo leaf nodes */
	int		c_purging;
	DB_TXN	*c_txn;	/* used by lru cleaner */
	ldap_pvt_thread_rdwr_t c_rwlock;
	ldap_pvt_thread_mutex_t c_lru_mutex;
	ldap_pvt_thread_mutex_t c_count_mutex;
	ldap_pvt_thread_mutex_t c_eifree_mutex;
#ifdef SLAP_ZONE_ALLOC
	void *c_zctx;
#endif
} Cache;
 
#define CACHE_READ_LOCK                0
#define CACHE_WRITE_LOCK       1
 
#define BDB_INDICES		128

struct bdb_db_info {
	struct berval	bdi_name;
	DB			*bdi_db;
};

struct bdb_db_pgsize {
	struct bdb_db_pgsize *bdp_next;
	struct berval	bdp_name;
	int	bdp_size;
};

#define BDB_MONITOR_IDX

typedef struct bdb_monitor_t {
	void		*bdm_cb;
	struct berval	bdm_ndn;
} bdb_monitor_t;

/* From ldap_rq.h */
struct re_s;

struct bdb_info {
	DB_ENV		*bi_dbenv;

	/* DB_ENV parameters */
	/* The DB_ENV can be tuned via DB_CONFIG */
	char		*bi_dbenv_home;
	u_int32_t	bi_dbenv_xflags; /* extra flags */
	int			bi_dbenv_mode;

	int			bi_ndatabases;
	int		bi_db_opflags;	/* db-specific flags */
	struct bdb_db_info **bi_databases;
	ldap_pvt_thread_mutex_t	bi_database_mutex;
	struct bdb_db_pgsize *bi_pagesizes;

	slap_mask_t	bi_defaultmask;
	Cache		bi_cache;
	struct bdb_attrinfo		**bi_attrs;
	int			bi_nattrs;
	void		*bi_search_stack;
	int		bi_search_stack_depth;
	int		bi_linear_index;

	int			bi_txn_cp;
	u_int32_t	bi_txn_cp_min;
	u_int32_t	bi_txn_cp_kbyte;
	struct re_s		*bi_txn_cp_task;
	struct re_s		*bi_index_task;

	u_int32_t		bi_lock_detect;
	long		bi_shm_key;

	ID			bi_lastid;
	ldap_pvt_thread_mutex_t	bi_lastid_mutex;
	ID	bi_idl_cache_max_size;
	ID		bi_idl_cache_size;
	Avlnode		*bi_idl_tree;
	bdb_idl_cache_entry_t	*bi_idl_lru_head;
	bdb_idl_cache_entry_t	*bi_idl_lru_tail;
	ldap_pvt_thread_rdwr_t bi_idl_tree_rwlock;
	ldap_pvt_thread_mutex_t bi_idl_tree_lrulock;
	alock_info_t	bi_alock_info;
	char		*bi_db_config_path;
	BerVarray	bi_db_config;
	char		*bi_db_crypt_file;
	struct berval	bi_db_crypt_key;
	bdb_monitor_t	bi_monitor;

#ifdef BDB_MONITOR_IDX
	ldap_pvt_thread_mutex_t	bi_idx_mutex;
	Avlnode		*bi_idx;
#endif /* BDB_MONITOR_IDX */

	int		bi_flags;
#define	BDB_IS_OPEN		0x01
#define	BDB_HAS_CONFIG	0x02
#define	BDB_UPD_CONFIG	0x04
#define	BDB_DEL_INDEX	0x08
#define	BDB_RE_OPEN		0x10
#define BDB_CHKSUM		0x20
#ifdef BDB_HIER
	int		bi_modrdns;		/* number of modrdns completed */
	ldap_pvt_thread_mutex_t	bi_modrdns_mutex;
#endif
};

#define bi_id2entry	bi_databases[BDB_ID2ENTRY]
#define bi_dn2id	bi_databases[BDB_DN2ID]


struct bdb_lock_info {
	struct bdb_lock_info *bli_next;
	DB_LOCK	bli_lock;
	ID		bli_id;
	int		bli_flag;
};
#define	BLI_DONTFREE	1

struct bdb_op_info {
	OpExtra boi_oe;
	DB_TXN*		boi_txn;
	struct bdb_lock_info *boi_locks;	/* used when no txn */
	u_int32_t	boi_err;
	char		boi_acl_cache;
	char		boi_flag;
};
#define BOI_DONTFREE	1

#define	DB_OPEN(db, file, name, type, flags, mode) \
	((db)->open)(db, file, name, type, flags, mode)

#if DB_VERSION_MAJOR < 4
#define LOCK_DETECT(env,f,t,a)		lock_detect(env, f, t, a)
#define LOCK_GET(env,i,f,o,m,l)		lock_get(env, i, f, o, m, l)
#define LOCK_PUT(env,l)			lock_put(env, l)
#define TXN_CHECKPOINT(env,k,m,f)	txn_checkpoint(env, k, m, f)
#define TXN_BEGIN(env,p,t,f)		txn_begin((env), p, t, f)
#define TXN_PREPARE(txn,gid)		txn_prepare((txn), (gid))
#define TXN_COMMIT(txn,f)			txn_commit((txn), (f))
#define	TXN_ABORT(txn)				txn_abort((txn))
#define TXN_ID(txn)					txn_id(txn)
#define XLOCK_ID(env, locker)		lock_id(env, locker)
#define XLOCK_ID_FREE(env, locker)	lock_id_free(env, locker)
#else
#define LOCK_DETECT(env,f,t,a)		(env)->lock_detect(env, f, t, a)
#define LOCK_GET(env,i,f,o,m,l)		(env)->lock_get(env, i, f, o, m, l)
#define LOCK_PUT(env,l)			(env)->lock_put(env, l)
#define TXN_CHECKPOINT(env,k,m,f)	(env)->txn_checkpoint(env, k, m, f)
#define TXN_BEGIN(env,p,t,f)		(env)->txn_begin((env), p, t, f)
#define TXN_PREPARE(txn,g)			(txn)->prepare((txn), (g))
#define TXN_COMMIT(txn,f)			(txn)->commit((txn), (f))
#define TXN_ABORT(txn)				(txn)->abort((txn))
#define TXN_ID(txn)					(txn)->id(txn)
#define XLOCK_ID(env, locker)		(env)->lock_id(env, locker)
#define XLOCK_ID_FREE(env, locker)	(env)->lock_id_free(env, locker)

/* BDB 4.1.17 adds txn arg to db->open */
#if DB_VERSION_FULL >= 0x04010011
#undef DB_OPEN
#define	DB_OPEN(db, file, name, type, flags, mode) \
	((db)->open)(db, NULL, file, name, type, flags, mode)
#endif

/* #undef BDB_LOG_DEBUG */

#ifdef BDB_LOG_DEBUG

/* env->log_printf appeared in 4.4 */
#if DB_VERSION_FULL >= 0x04040000
#define	BDB_LOG_PRINTF(env,txn,fmt,...)	(env)->log_printf((env),(txn),(fmt),__VA_ARGS__)
#else
extern int __db_logmsg(const DB_ENV *env, DB_TXN *txn, const char *op, u_int32_t flags,
	const char *fmt,...);
#define	BDB_LOG_PRINTF(env,txn,fmt,...)	__db_logmsg((env),(txn),"DIAGNOSTIC",0,(fmt),__VA_ARGS__)
#endif

/* !BDB_LOG_DEBUG */
#elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
	(defined(__GNUC__) && __GNUC__ >= 3 && !defined(__STRICT_ANSI__))
#define BDB_LOG_PRINTF(a,b,c,...)
#else
#define BDB_LOG_PRINTF (void)	/* will evaluate and discard the arguments */

#endif /* BDB_LOG_DEBUG */

#endif

#ifndef DB_BUFFER_SMALL
#define DB_BUFFER_SMALL			ENOMEM
#endif

#define BDB_CSN_COMMIT	0
#define BDB_CSN_ABORT	1
#define BDB_CSN_RETRY	2

/* Copy an ID "src" to pointer "dst" in big-endian byte order */
#define BDB_ID2DISK( src, dst )	\
	do { int i0; ID tmp; unsigned char *_p;	\
		tmp = (src); _p = (unsigned char *)(dst);	\
		for ( i0=sizeof(ID)-1; i0>=0; i0-- ) {	\
			_p[i0] = tmp & 0xff; tmp >>= 8;	\
		} \
	} while(0)

/* Copy a pointer "src" to a pointer "dst" from big-endian to native order */
#define BDB_DISK2ID( src, dst ) \
	do { unsigned i0; ID tmp = 0; unsigned char *_p;	\
		_p = (unsigned char *)(src);	\
		for ( i0=0; i0<sizeof(ID); i0++ ) {	\
			tmp <<= 8; tmp |= *_p++;	\
		} *(dst) = tmp; \
	} while (0)

LDAP_END_DECL

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct bdb_attrinfo {
	AttributeDescription *ai_desc; /* attribute description cn;lang-en */
	slap_mask_t ai_indexmask;	/* how the attr is indexed	*/
	slap_mask_t ai_newmask;	/* new settings to replace old mask */
#ifdef LDAP_COMP_MATCH
	ComponentReference* ai_cr; /*component indexing*/
#endif
} AttrInfo;

/* These flags must not clash with SLAP_INDEX flags or ops in slap.h! */
#define	BDB_INDEX_DELETING	0x8000U	/* index is being modified */
#define	BDB_INDEX_UPDATE_OP	0x03	/* performing an index update */

/* For slapindex to record which attrs in an entry belong to which
 * index database 
 */
typedef struct AttrList {
	struct AttrList *next;
	Attribute *attr;
} AttrList;

typedef struct IndexRec {
	AttrInfo *ai;
	AttrList *attrs;
} IndexRec;

#include "proto-bdb.h"

#endif /* _BACK_BDB_H_ */
