/* back-mdb.h - mdb back-end header file */
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

#ifndef _BACK_MDB_H_
#define _BACK_MDB_H_

#include <portable.h>
#include "slap.h"
#include "lmdb.h"

LDAP_BEGIN_DECL

#undef	MDB_TOOL_IDL_CACHING	/* currently no perf gain */

#define DN_BASE_PREFIX		SLAP_INDEX_EQUALITY_PREFIX
#define DN_ONE_PREFIX	 	'%'
#define DN_SUBTREE_PREFIX 	'@'

#define MDB_AD2ID		0
#define MDB_DN2ID		1
#define MDB_ID2ENTRY	2
#define MDB_NDB			3

/* The default search IDL stack cache depth */
#define DEFAULT_SEARCH_STACK_DEPTH	16

/* The minimum we can function with */
#define MINIMUM_SEARCH_STACK_DEPTH	8

#define MDB_INDICES		128

#define	MDB_MAXADS	65536

/* Default to 10MB max */
#define DEFAULT_MAPSIZE	(10*1048576)

#define MDB_MONITOR_IDX

typedef struct mdb_monitor_t {
	void		*mdm_cb;
	struct berval	mdm_ndn;
} mdb_monitor_t;

/* From ldap_rq.h */
struct re_s;

struct mdb_info {
	MDB_env		*mi_dbenv;

	/* DB_ENV parameters */
	/* The DB_ENV can be tuned via DB_CONFIG */
	char		*mi_dbenv_home;
	uint32_t	mi_dbenv_flags;
	int			mi_dbenv_mode;

	size_t		mi_mapsize;
	ID			mi_nextid;

	slap_mask_t	mi_defaultmask;
	int			mi_nattrs;
	struct mdb_attrinfo		**mi_attrs;
	void		*mi_search_stack;
	int			mi_search_stack_depth;
	int			mi_readers;

	int			mi_txn_cp;
	uint32_t	mi_txn_cp_min;
	uint32_t	mi_txn_cp_kbyte;
	struct re_s		*mi_txn_cp_task;
	struct re_s		*mi_index_task;

	mdb_monitor_t	mi_monitor;

#ifdef MDB_MONITOR_IDX
	ldap_pvt_thread_mutex_t	mi_idx_mutex;
	Avlnode		*mi_idx;
#endif /* MDB_MONITOR_IDX */

	int		mi_flags;
#define	MDB_IS_OPEN		0x01
#define	MDB_OPEN_INDEX	0x02
#define	MDB_DEL_INDEX	0x08
#define	MDB_RE_OPEN		0x10
#define	MDB_NEED_UPGRADE	0x20

	int mi_numads;

	MDB_dbi	mi_dbis[MDB_NDB];
	AttributeDescription *mi_ads[MDB_MAXADS];
	int mi_adxs[MDB_MAXADS];
};

#define mi_id2entry	mi_dbis[MDB_ID2ENTRY]
#define mi_dn2id	mi_dbis[MDB_DN2ID]
#define mi_ad2id	mi_dbis[MDB_AD2ID]

typedef struct mdb_op_info {
	OpExtra		moi_oe;
	MDB_txn*	moi_txn;
	int			moi_ref;
	char		moi_flag;
} mdb_op_info;
#define MOI_READER	0x01
#define MOI_FREEIT	0x02

/* Copy an ID "src" to pointer "dst" in big-endian byte order */
#define MDB_ID2DISK( src, dst )	\
	do { int i0; ID tmp; unsigned char *_p;	\
		tmp = (src); _p = (unsigned char *)(dst);	\
		for ( i0=sizeof(ID)-1; i0>=0; i0-- ) {	\
			_p[i0] = tmp & 0xff; tmp >>= 8;	\
		} \
	} while(0)

/* Copy a pointer "src" to a pointer "dst" from big-endian to native order */
#define MDB_DISK2ID( src, dst ) \
	do { unsigned i0; ID tmp = 0; unsigned char *_p;	\
		_p = (unsigned char *)(src);	\
		for ( i0=0; i0<sizeof(ID); i0++ ) {	\
			tmp <<= 8; tmp |= *_p++;	\
		} *(dst) = tmp; \
	} while (0)

LDAP_END_DECL

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct mdb_attrinfo {
	AttributeDescription *ai_desc; /* attribute description cn;lang-en */
	slap_mask_t ai_indexmask;	/* how the attr is indexed	*/
	slap_mask_t ai_newmask;	/* new settings to replace old mask */
#ifdef LDAP_COMP_MATCH
	ComponentReference* ai_cr; /*component indexing*/
#endif
	Avlnode *ai_root;		/* for tools */
	MDB_cursor *ai_cursor;	/* for tools */
	int ai_idx;	/* position in AI array */
	MDB_dbi ai_dbi;
} AttrInfo;

/* tool threaded indexer state */
typedef struct mdb_attrixinfo {
	OpExtra ai_oe;
	void *ai_flist;
	void *ai_clist;
	AttrInfo *ai_ai;
} AttrIxInfo;

/* These flags must not clash with SLAP_INDEX flags or ops in slap.h! */
#define	MDB_INDEX_DELETING	0x8000U	/* index is being modified */
#define	MDB_INDEX_UPDATE_OP	0x03	/* performing an index update */

/* For slapindex to record which attrs in an entry belong to which
 * index database 
 */
typedef struct AttrList {
	struct AttrList *next;
	Attribute *attr;
} AttrList;

#ifndef CACHELINE
#define CACHELINE	64
#endif

#if defined(__i386) || defined(__x86_64)
#define MISALIGNED_OK	1
#else
#define	ALIGNER	(sizeof(size_t)-1)
#endif

typedef struct IndexRbody {
	AttrInfo *ai;
	AttrList *attrs;
	void *tptr;
	int i;
} IndexRbody;

typedef struct IndexRec {
	union {
		IndexRbody irb;
#define ir_ai	iru.irb.ai
#define ir_attrs	iru.irb.attrs
#define ir_tptr	iru.irb.tptr
#define ir_i	iru.irb.i
		/* cache line alignment */
		char pad[(sizeof(IndexRbody)+CACHELINE-1) & (!CACHELINE-1)];
	} iru;
} IndexRec;

#define MAXRDNS	SLAP_LDAPDN_MAXLEN/4

#include "proto-mdb.h"

#endif /* _BACK_MDB_H_ */
