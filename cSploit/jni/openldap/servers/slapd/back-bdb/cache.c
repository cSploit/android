/* cache.c - routines to maintain an in-core cache of entries */
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

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#include "back-bdb.h"

#include "ldap_rq.h"

#ifdef BDB_HIER
#define bdb_cache_lru_purge	hdb_cache_lru_purge
#endif
static void bdb_cache_lru_purge( struct bdb_info *bdb );

static int	bdb_cache_delete_internal(Cache *cache, EntryInfo *e, int decr);
#ifdef LDAP_DEBUG
#define SLAPD_UNUSED
#ifdef SLAPD_UNUSED
static void	bdb_lru_print(Cache *cache);
static void	bdb_idtree_print(Cache *cache);
#endif
#endif

/* For concurrency experiments only! */
#if 0
#define	ldap_pvt_thread_rdwr_wlock(a)	0
#define	ldap_pvt_thread_rdwr_wunlock(a)	0
#define	ldap_pvt_thread_rdwr_rlock(a)	0
#define	ldap_pvt_thread_rdwr_runlock(a)	0
#endif

#if 0
#define ldap_pvt_thread_mutex_trylock(a) 0
#endif

static EntryInfo *
bdb_cache_entryinfo_new( Cache *cache )
{
	EntryInfo *ei = NULL;

	if ( cache->c_eifree ) {
		ldap_pvt_thread_mutex_lock( &cache->c_eifree_mutex );
		if ( cache->c_eifree ) {
			ei = cache->c_eifree;
			cache->c_eifree = ei->bei_lrunext;
			ei->bei_finders = 0;
			ei->bei_lrunext = NULL;
		}
		ldap_pvt_thread_mutex_unlock( &cache->c_eifree_mutex );
	}
	if ( !ei ) {
		ei = ch_calloc(1, sizeof(EntryInfo));
		ldap_pvt_thread_mutex_init( &ei->bei_kids_mutex );
	}

	ei->bei_state = CACHE_ENTRY_REFERENCED;

	return ei;
}

static void
bdb_cache_entryinfo_free( Cache *cache, EntryInfo *ei )
{
	free( ei->bei_nrdn.bv_val );
	BER_BVZERO( &ei->bei_nrdn );
#ifdef BDB_HIER
	free( ei->bei_rdn.bv_val );
	BER_BVZERO( &ei->bei_rdn );
	ei->bei_modrdns = 0;
	ei->bei_ckids = 0;
	ei->bei_dkids = 0;
#endif
	ei->bei_parent = NULL;
	ei->bei_kids = NULL;
	ei->bei_lruprev = NULL;

#if 0
	ldap_pvt_thread_mutex_lock( &cache->c_eifree_mutex );
	ei->bei_lrunext = cache->c_eifree;
	cache->c_eifree = ei;
	ldap_pvt_thread_mutex_unlock( &cache->c_eifree_mutex );
#else
	ldap_pvt_thread_mutex_destroy( &ei->bei_kids_mutex );
	ch_free( ei );
#endif
}

#define LRU_DEL( c, e ) do { \
	if ( e == e->bei_lruprev ) { \
		(c)->c_lruhead = (c)->c_lrutail = NULL; \
	} else { \
		if ( e == (c)->c_lruhead ) (c)->c_lruhead = e->bei_lruprev; \
		if ( e == (c)->c_lrutail ) (c)->c_lrutail = e->bei_lruprev; \
		e->bei_lrunext->bei_lruprev = e->bei_lruprev; \
		e->bei_lruprev->bei_lrunext = e->bei_lrunext; \
	} \
	e->bei_lruprev = NULL; \
} while ( 0 )

/* Note - we now use a Second-Chance / Clock algorithm instead of
 * Least-Recently-Used. This tremendously improves concurrency
 * because we no longer need to manipulate the lists every time an
 * entry is touched. We only need to lock the lists when adding
 * or deleting an entry. It's now a circular doubly-linked list.
 * We always append to the tail, but the head traverses the circle
 * during a purge operation.
 */
static void
bdb_cache_lru_link( struct bdb_info *bdb, EntryInfo *ei )
{

	/* Already linked, ignore */
	if ( ei->bei_lruprev )
		return;

	/* Insert into circular LRU list */
	ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_lru_mutex );

	ei->bei_lruprev = bdb->bi_cache.c_lrutail;
	if ( bdb->bi_cache.c_lrutail ) {
		ei->bei_lrunext = bdb->bi_cache.c_lrutail->bei_lrunext;
		bdb->bi_cache.c_lrutail->bei_lrunext = ei;
		if ( ei->bei_lrunext )
			ei->bei_lrunext->bei_lruprev = ei;
	} else {
		ei->bei_lrunext = ei->bei_lruprev = ei;
		bdb->bi_cache.c_lruhead = ei;
	}
	bdb->bi_cache.c_lrutail = ei;
	ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_lru_mutex );
}

#ifdef NO_THREADS
#define NO_DB_LOCK
#endif

/* #define NO_DB_LOCK 1 */
/* Note: The BerkeleyDB locks are much slower than regular
 * mutexes or rdwr locks. But the BDB implementation has the
 * advantage of using a fixed size lock table, instead of
 * allocating a lock object per entry in the DB. That's a
 * key benefit for scaling. It also frees us from worrying
 * about undetectable deadlocks between BDB activity and our
 * own cache activity. It's still worth exploring faster
 * alternatives though.
 */

/* Atomically release and reacquire a lock */
int
bdb_cache_entry_db_relock(
	struct bdb_info *bdb,
	DB_TXN *txn,
	EntryInfo *ei,
	int rw,
	int tryOnly,
	DB_LOCK *lock )
{
#ifdef NO_DB_LOCK
	return 0;
#else
	int	rc;
	DBT	lockobj;
	DB_LOCKREQ list[2];

	if ( !lock ) return 0;

	DBTzero( &lockobj );
	lockobj.data = &ei->bei_id;
	lockobj.size = sizeof(ei->bei_id) + 1;

	list[0].op = DB_LOCK_PUT;
	list[0].lock = *lock;
	list[1].op = DB_LOCK_GET;
	list[1].lock = *lock;
	list[1].mode = rw ? DB_LOCK_WRITE : DB_LOCK_READ;
	list[1].obj = &lockobj;
	rc = bdb->bi_dbenv->lock_vec(bdb->bi_dbenv, TXN_ID(txn), tryOnly ? DB_LOCK_NOWAIT : 0,
		list, 2, NULL );

	if (rc && !tryOnly) {
		Debug( LDAP_DEBUG_TRACE,
			"bdb_cache_entry_db_relock: entry %ld, rw %d, rc %d\n",
			ei->bei_id, rw, rc );
	} else {
		*lock = list[1].lock;
	}
	return rc;
#endif
}

static int
bdb_cache_entry_db_lock( struct bdb_info *bdb, DB_TXN *txn, EntryInfo *ei,
	int rw, int tryOnly, DB_LOCK *lock )
{
#ifdef NO_DB_LOCK
	return 0;
#else
	int       rc;
	DBT       lockobj;
	int       db_rw;

	if ( !lock ) return 0;

	if (rw)
		db_rw = DB_LOCK_WRITE;
	else
		db_rw = DB_LOCK_READ;

	DBTzero( &lockobj );
	lockobj.data = &ei->bei_id;
	lockobj.size = sizeof(ei->bei_id) + 1;

	rc = LOCK_GET(bdb->bi_dbenv, TXN_ID(txn), tryOnly ? DB_LOCK_NOWAIT : 0,
					&lockobj, db_rw, lock);
	if (rc && !tryOnly) {
		Debug( LDAP_DEBUG_TRACE,
			"bdb_cache_entry_db_lock: entry %ld, rw %d, rc %d\n",
			ei->bei_id, rw, rc );
	}
	return rc;
#endif /* NO_DB_LOCK */
}

int
bdb_cache_entry_db_unlock ( struct bdb_info *bdb, DB_LOCK *lock )
{
#ifdef NO_DB_LOCK
	return 0;
#else
	int rc;

	if ( !lock || lock->mode == DB_LOCK_NG ) return 0;

	rc = LOCK_PUT ( bdb->bi_dbenv, lock );
	return rc;
#endif
}

void
bdb_cache_return_entry_rw( struct bdb_info *bdb, Entry *e,
	int rw, DB_LOCK *lock )
{
	EntryInfo *ei;
	int free = 0;

	ei = e->e_private;
	if ( ei && ( ei->bei_state & CACHE_ENTRY_NOT_CACHED )) {
		bdb_cache_entryinfo_lock( ei );
		if ( ei->bei_state & CACHE_ENTRY_NOT_CACHED ) {
			/* Releasing the entry can only be done when
			 * we know that nobody else is using it, i.e we
			 * should have an entry_db writelock.  But the
			 * flag is only set by the thread that loads the
			 * entry, and only if no other threads has found
			 * it while it was working.  All other threads
			 * clear the flag, which mean that we should be
			 * the only thread using the entry if the flag
			 * is set here.
			 */
			ei->bei_e = NULL;
			ei->bei_state ^= CACHE_ENTRY_NOT_CACHED;
			free = 1;
		}
		bdb_cache_entryinfo_unlock( ei );
	}
	bdb_cache_entry_db_unlock( bdb, lock );
	if ( free ) {
		e->e_private = NULL;
		bdb_entry_return( e );
	}
}

static int
bdb_cache_entryinfo_destroy( EntryInfo *e )
{
	ldap_pvt_thread_mutex_destroy( &e->bei_kids_mutex );
	free( e->bei_nrdn.bv_val );
#ifdef BDB_HIER
	free( e->bei_rdn.bv_val );
#endif
	free( e );
	return 0;
}

/* Do a length-ordered sort on normalized RDNs */
static int
bdb_rdn_cmp( const void *v_e1, const void *v_e2 )
{
	const EntryInfo *e1 = v_e1, *e2 = v_e2;
	int rc = e1->bei_nrdn.bv_len - e2->bei_nrdn.bv_len;
	if (rc == 0) {
		rc = strncmp( e1->bei_nrdn.bv_val, e2->bei_nrdn.bv_val,
			e1->bei_nrdn.bv_len );
	}
	return rc;
}

static int
bdb_id_cmp( const void *v_e1, const void *v_e2 )
{
	const EntryInfo *e1 = v_e1, *e2 = v_e2;
	return e1->bei_id - e2->bei_id;
}

static int
bdb_id_dup_err( void *v1, void *v2 )
{
	EntryInfo *e2 = v2;
	e2->bei_lrunext = v1;
	return -1;
}

/* Create an entryinfo in the cache. Caller must release the locks later.
 */
static int
bdb_entryinfo_add_internal(
	struct bdb_info *bdb,
	EntryInfo *ei,
	EntryInfo **res )
{
	EntryInfo *ei2 = NULL;

	*res = NULL;

	ei2 = bdb_cache_entryinfo_new( &bdb->bi_cache );

	bdb_cache_entryinfo_lock( ei->bei_parent );
	ldap_pvt_thread_rdwr_wlock( &bdb->bi_cache.c_rwlock );

	ei2->bei_id = ei->bei_id;
	ei2->bei_parent = ei->bei_parent;
#ifdef BDB_HIER
	ei2->bei_rdn = ei->bei_rdn;
#endif
#ifdef SLAP_ZONE_ALLOC
	ei2->bei_bdb = bdb;
#endif

	/* Add to cache ID tree */
	if (avl_insert( &bdb->bi_cache.c_idtree, ei2, bdb_id_cmp,
		bdb_id_dup_err )) {
		EntryInfo *eix = ei2->bei_lrunext;
		bdb_cache_entryinfo_free( &bdb->bi_cache, ei2 );
		ei2 = eix;
#ifdef BDB_HIER
		/* It got freed above because its value was
		 * assigned to ei2.
		 */
		ei->bei_rdn.bv_val = NULL;
#endif
	} else {
		int rc;

		bdb->bi_cache.c_eiused++;
		ber_dupbv( &ei2->bei_nrdn, &ei->bei_nrdn );

		/* This is a new leaf node. But if parent had no kids, then it was
		 * a leaf and we would be decrementing that. So, only increment if
		 * the parent already has kids.
		 */
		if ( ei->bei_parent->bei_kids || !ei->bei_parent->bei_id )
			bdb->bi_cache.c_leaves++;
		rc = avl_insert( &ei->bei_parent->bei_kids, ei2, bdb_rdn_cmp,
			avl_dup_error );
#ifdef BDB_HIER
		/* it's possible for hdb_cache_find_parent to beat us to it */
		if ( !rc ) {
			ei->bei_parent->bei_ckids++;
		}
#endif
	}

	*res = ei2;
	return 0;
}

/* Find the EntryInfo for the requested DN. If the DN cannot be found, return
 * the info for its closest ancestor. *res should be NULL to process a
 * complete DN starting from the tree root. Otherwise *res must be the
 * immediate parent of the requested DN, and only the RDN will be searched.
 * The EntryInfo is locked upon return and must be unlocked by the caller.
 */
int
bdb_cache_find_ndn(
	Operation	*op,
	DB_TXN		*txn,
	struct berval	*ndn,
	EntryInfo	**res )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	EntryInfo	ei, *eip, *ei2;
	int rc = 0;
	char *ptr;

	/* this function is always called with normalized DN */
	if ( *res ) {
		/* we're doing a onelevel search for an RDN */
		ei.bei_nrdn.bv_val = ndn->bv_val;
		ei.bei_nrdn.bv_len = dn_rdnlen( op->o_bd, ndn );
		eip = *res;
	} else {
		/* we're searching a full DN from the root */
		ptr = ndn->bv_val + ndn->bv_len - op->o_bd->be_nsuffix[0].bv_len;
		ei.bei_nrdn.bv_val = ptr;
		ei.bei_nrdn.bv_len = op->o_bd->be_nsuffix[0].bv_len;
		/* Skip to next rdn if suffix is empty */
		if ( ei.bei_nrdn.bv_len == 0 ) {
			for (ptr = ei.bei_nrdn.bv_val - 2; ptr > ndn->bv_val
				&& !DN_SEPARATOR(*ptr); ptr--) /* empty */;
			if ( ptr >= ndn->bv_val ) {
				if (DN_SEPARATOR(*ptr)) ptr++;
				ei.bei_nrdn.bv_len = ei.bei_nrdn.bv_val - ptr;
				ei.bei_nrdn.bv_val = ptr;
			}
		}
		eip = &bdb->bi_cache.c_dntree;
	}
	
	for ( bdb_cache_entryinfo_lock( eip ); eip; ) {
		eip->bei_state |= CACHE_ENTRY_REFERENCED;
		ei.bei_parent = eip;
		ei2 = (EntryInfo *)avl_find( eip->bei_kids, &ei, bdb_rdn_cmp );
		if ( !ei2 ) {
			DBC *cursor;
			int len = ei.bei_nrdn.bv_len;
				
			if ( BER_BVISEMPTY( ndn )) {
				*res = eip;
				return LDAP_SUCCESS;
			}

			ei.bei_nrdn.bv_len = ndn->bv_len -
				(ei.bei_nrdn.bv_val - ndn->bv_val);
			eip->bei_finders++;
			bdb_cache_entryinfo_unlock( eip );

			BDB_LOG_PRINTF( bdb->bi_dbenv, NULL, "slapd Reading %s",
				ei.bei_nrdn.bv_val );

			cursor = NULL;
			rc = bdb_dn2id( op, &ei.bei_nrdn, &ei, txn, &cursor );
			if (rc) {
				bdb_cache_entryinfo_lock( eip );
				eip->bei_finders--;
				if ( cursor ) cursor->c_close( cursor );
				*res = eip;
				return rc;
			}

			BDB_LOG_PRINTF( bdb->bi_dbenv, NULL, "slapd Read got %s(%d)",
				ei.bei_nrdn.bv_val, ei.bei_id );

			/* DN exists but needs to be added to cache */
			ei.bei_nrdn.bv_len = len;
			rc = bdb_entryinfo_add_internal( bdb, &ei, &ei2 );
			/* add_internal left eip and c_rwlock locked */
			eip->bei_finders--;
			ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );
			if ( cursor ) cursor->c_close( cursor );
			if ( rc ) {
				*res = eip;
				return rc;
			}
		}
		bdb_cache_entryinfo_lock( ei2 );
		if ( ei2->bei_state & CACHE_ENTRY_DELETED ) {
			/* In the midst of deleting? Give it a chance to
			 * complete.
			 */
			bdb_cache_entryinfo_unlock( ei2 );
			bdb_cache_entryinfo_unlock( eip );
			ldap_pvt_thread_yield();
			bdb_cache_entryinfo_lock( eip );
			*res = eip;
			return DB_NOTFOUND;
		}
		bdb_cache_entryinfo_unlock( eip );

		eip = ei2;

		/* Advance to next lower RDN */
		for (ptr = ei.bei_nrdn.bv_val - 2; ptr > ndn->bv_val
			&& !DN_SEPARATOR(*ptr); ptr--) /* empty */;
		if ( ptr >= ndn->bv_val ) {
			if (DN_SEPARATOR(*ptr)) ptr++;
			ei.bei_nrdn.bv_len = ei.bei_nrdn.bv_val - ptr - 1;
			ei.bei_nrdn.bv_val = ptr;
		}
		if ( ptr < ndn->bv_val ) {
			*res = eip;
			break;
		}
	}

	return rc;
}

#ifdef BDB_HIER
/* Walk up the tree from a child node, looking for an ID that's already
 * been linked into the cache.
 */
int
hdb_cache_find_parent(
	Operation *op,
	DB_TXN	*txn,
	ID id,
	EntryInfo **res )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	EntryInfo ei, eip, *ei2 = NULL, *ein = NULL, *eir = NULL;
	int rc, add;

	ei.bei_id = id;
	ei.bei_kids = NULL;
	ei.bei_ckids = 0;

	for (;;) {
		rc = hdb_dn2id_parent( op, txn, &ei, &eip.bei_id );
		if ( rc ) break;

		/* Save the previous node, if any */
		ei2 = ein;

		/* Create a new node for the current ID */
		ein = bdb_cache_entryinfo_new( &bdb->bi_cache );
		ein->bei_id = ei.bei_id;
		ein->bei_kids = ei.bei_kids;
		ein->bei_nrdn = ei.bei_nrdn;
		ein->bei_rdn = ei.bei_rdn;
		ein->bei_ckids = ei.bei_ckids;
#ifdef SLAP_ZONE_ALLOC
		ein->bei_bdb = bdb;
#endif
		ei.bei_ckids = 0;
		add = 1;
		
		/* This node is not fully connected yet */
		ein->bei_state |= CACHE_ENTRY_NOT_LINKED;

		/* If this is the first time, save this node
		 * to be returned later.
		 */
		if ( eir == NULL ) {
			eir = ein;
			ein->bei_finders++;
		}

again:
		/* Insert this node into the ID tree */
		ldap_pvt_thread_rdwr_wlock( &bdb->bi_cache.c_rwlock );
		if ( avl_insert( &bdb->bi_cache.c_idtree, (caddr_t)ein,
			bdb_id_cmp, bdb_id_dup_err ) ) {
			EntryInfo *eix = ein->bei_lrunext;

			if ( bdb_cache_entryinfo_trylock( eix )) {
				ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );
				ldap_pvt_thread_yield();
				goto again;
			}
			ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );

			/* Someone else created this node just before us.
			 * Free our new copy and use the existing one.
			 */
			bdb_cache_entryinfo_free( &bdb->bi_cache, ein );

			/* if it was the node we were looking for, just return it */
			if ( eir == ein ) {
				*res = eix;
				rc = 0;
				break;
			}

			ein = ei2;
			ei2 = eix;
			add = 0;

			/* otherwise, link up what we have and return */
			goto gotparent;
		}

		/* If there was a previous node, link it to this one */
		if ( ei2 ) ei2->bei_parent = ein;

		/* Look for this node's parent */
par2:
		if ( eip.bei_id ) {
			ei2 = (EntryInfo *) avl_find( bdb->bi_cache.c_idtree,
					(caddr_t) &eip, bdb_id_cmp );
		} else {
			ei2 = &bdb->bi_cache.c_dntree;
		}
		if ( ei2 && bdb_cache_entryinfo_trylock( ei2 )) {
			ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );
			ldap_pvt_thread_yield();
			ldap_pvt_thread_rdwr_wlock( &bdb->bi_cache.c_rwlock );
			goto par2;
		}
		if ( add )
			bdb->bi_cache.c_eiused++;
		if ( ei2 && ( ei2->bei_kids || !ei2->bei_id ))
			bdb->bi_cache.c_leaves++;
		ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );

gotparent:
		/* Got the parent, link in and we're done. */
		if ( ei2 ) {
			bdb_cache_entryinfo_lock( eir );
			ein->bei_parent = ei2;

			if ( avl_insert( &ei2->bei_kids, (caddr_t)ein, bdb_rdn_cmp,
				avl_dup_error) == 0 )
				ei2->bei_ckids++;

			/* Reset all the state info */
			for (ein = eir; ein != ei2; ein=ein->bei_parent)
				ein->bei_state &= ~CACHE_ENTRY_NOT_LINKED;

			bdb_cache_entryinfo_unlock( ei2 );
			eir->bei_finders--;

			*res = eir;
			break;
		}
		ei.bei_kids = NULL;
		ei.bei_id = eip.bei_id;
		ei.bei_ckids = 1;
		avl_insert( &ei.bei_kids, (caddr_t)ein, bdb_rdn_cmp,
			avl_dup_error );
	}
	return rc;
}

/* Used by hdb_dn2idl when loading the EntryInfo for all the children
 * of a given node
 */
int hdb_cache_load(
	struct bdb_info *bdb,
	EntryInfo *ei,
	EntryInfo **res )
{
	EntryInfo *ei2;
	int rc;

	/* See if we already have this one */
	bdb_cache_entryinfo_lock( ei->bei_parent );
	ei2 = (EntryInfo *)avl_find( ei->bei_parent->bei_kids, ei, bdb_rdn_cmp );
	bdb_cache_entryinfo_unlock( ei->bei_parent );

	if ( !ei2 ) {
		/* Not found, add it */
		struct berval bv;

		/* bei_rdn was not malloc'd before, do it now */
		ber_dupbv( &bv, &ei->bei_rdn );
		ei->bei_rdn = bv;

		rc = bdb_entryinfo_add_internal( bdb, ei, res );
		bdb_cache_entryinfo_unlock( ei->bei_parent );
		ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );
	} else {
		/* Found, return it */
		*res = ei2;
		return 0;
	}
	return rc;
}
#endif

/* This is best-effort only. If all entries in the cache are
 * busy, they will all be kept. This is unlikely to happen
 * unless the cache is very much smaller than the working set.
 */
static void
bdb_cache_lru_purge( struct bdb_info *bdb )
{
	DB_LOCK		lock, *lockp;
	EntryInfo *elru, *elnext = NULL;
	int islocked;
	ID eicount, ecount;
	ID count, efree, eifree = 0;
#ifdef LDAP_DEBUG
	int iter;
#endif

	/* Wait for the mutex; we're the only one trying to purge. */
	ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_lru_mutex );

	if ( bdb->bi_cache.c_cursize > bdb->bi_cache.c_maxsize ) {
		efree = bdb->bi_cache.c_cursize - bdb->bi_cache.c_maxsize;
		efree += bdb->bi_cache.c_minfree;
	} else {
		efree = 0;
	}

	/* maximum number of EntryInfo leaves to cache. In slapcat
	 * we always free all leaf nodes.
	 */

	if ( slapMode & SLAP_TOOL_READONLY ) {
		eifree = bdb->bi_cache.c_leaves;
	} else if ( bdb->bi_cache.c_eimax &&
		bdb->bi_cache.c_leaves > bdb->bi_cache.c_eimax ) {
		eifree = bdb->bi_cache.c_minfree * 10;
		if ( eifree >= bdb->bi_cache.c_leaves )
			eifree /= 2;
	}

	if ( !efree && !eifree ) {
		ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_lru_mutex );
		bdb->bi_cache.c_purging = 0;
		return;
	}

	if ( bdb->bi_cache.c_txn ) {
		lockp = &lock;
	} else {
		lockp = NULL;
	}

	count = 0;
	eicount = 0;
	ecount = 0;
#ifdef LDAP_DEBUG
	iter = 0;
#endif

	/* Look for an unused entry to remove */
	for ( elru = bdb->bi_cache.c_lruhead; elru; elru = elnext ) {
		elnext = elru->bei_lrunext;

		if ( bdb_cache_entryinfo_trylock( elru ))
			goto bottom;

		/* This flag implements the clock replacement behavior */
		if ( elru->bei_state & ( CACHE_ENTRY_REFERENCED )) {
			elru->bei_state &= ~CACHE_ENTRY_REFERENCED;
			bdb_cache_entryinfo_unlock( elru );
			goto bottom;
		}

		/* If this node is in the process of linking into the cache,
		 * or this node is being deleted, skip it.
		 */
		if (( elru->bei_state & ( CACHE_ENTRY_NOT_LINKED |
			CACHE_ENTRY_DELETED | CACHE_ENTRY_LOADING |
			CACHE_ENTRY_ONELEVEL )) ||
			elru->bei_finders > 0 ) {
			bdb_cache_entryinfo_unlock( elru );
			goto bottom;
		}

		if ( bdb_cache_entryinfo_trylock( elru->bei_parent )) {
			bdb_cache_entryinfo_unlock( elru );
			goto bottom;
		}

		/* entryinfo is locked */
		islocked = 1;

		/* If we can successfully writelock it, then
		 * the object is idle.
		 */
		if ( bdb_cache_entry_db_lock( bdb,
			bdb->bi_cache.c_txn, elru, 1, 1, lockp ) == 0 ) {

			/* Free entry for this node if it's present */
			if ( elru->bei_e ) {
				ecount++;

				/* the cache may have gone over the limit while we
				 * weren't looking, so double check.
				 */
				if ( !efree && ecount > bdb->bi_cache.c_maxsize )
					efree = bdb->bi_cache.c_minfree;

				if ( count < efree ) {
					elru->bei_e->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
					bdb_entry_return( bdb, elru->bei_e, elru->bei_zseq );
#else
					bdb_entry_return( elru->bei_e );
#endif
					elru->bei_e = NULL;
					count++;
				} else {
					/* Keep this node cached, skip to next */
					bdb_cache_entry_db_unlock( bdb, lockp );
					goto next;
				}
			}
			bdb_cache_entry_db_unlock( bdb, lockp );

			/* 
			 * If it is a leaf node, and we're over the limit, free it.
			 */
			if ( elru->bei_kids ) {
				/* Drop from list, we ignore it... */
				LRU_DEL( &bdb->bi_cache, elru );
			} else if ( eicount < eifree ) {
				/* Too many leaf nodes, free this one */
				bdb_cache_delete_internal( &bdb->bi_cache, elru, 0 );
				bdb_cache_delete_cleanup( &bdb->bi_cache, elru );
				islocked = 0;
				eicount++;
			}	/* Leave on list until we need to free it */
		}

next:
		if ( islocked ) {
			bdb_cache_entryinfo_unlock( elru );
			bdb_cache_entryinfo_unlock( elru->bei_parent );
		}

		if ( count >= efree && eicount >= eifree )
			break;
bottom:
		if ( elnext == bdb->bi_cache.c_lruhead )
			break;
#ifdef LDAP_DEBUG
		iter++;
#endif
	}

	if ( count || ecount > bdb->bi_cache.c_cursize ) {
		ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_count_mutex );
		/* HACK: we seem to be losing track, fix up now */
		if ( ecount > bdb->bi_cache.c_cursize )
			bdb->bi_cache.c_cursize = ecount;
		bdb->bi_cache.c_cursize -= count;
		ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_count_mutex );
	}
	bdb->bi_cache.c_lruhead = elnext;
	ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_lru_mutex );
	bdb->bi_cache.c_purging = 0;
}

/*
 * cache_find_id - find an entry in the cache, given id.
 * The entry is locked for Read upon return. Call with flag ID_LOCKED if
 * the supplied *eip was already locked.
 */

int
bdb_cache_find_id(
	Operation *op,
	DB_TXN	*tid,
	ID				id,
	EntryInfo	**eip,
	int		flag,
	DB_LOCK		*lock )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	Entry	*ep = NULL;
	int	rc = 0, load = 0;
	EntryInfo ei = { 0 };

	ei.bei_id = id;

#ifdef SLAP_ZONE_ALLOC
	slap_zh_rlock(bdb->bi_cache.c_zctx);
#endif
	/* If we weren't given any info, see if we have it already cached */
	if ( !*eip ) {
again:	ldap_pvt_thread_rdwr_rlock( &bdb->bi_cache.c_rwlock );
		*eip = (EntryInfo *) avl_find( bdb->bi_cache.c_idtree,
			(caddr_t) &ei, bdb_id_cmp );
		if ( *eip ) {
			/* If the lock attempt fails, the info is in use */
			if ( bdb_cache_entryinfo_trylock( *eip )) {
				int del = (*eip)->bei_state & CACHE_ENTRY_DELETED;
				ldap_pvt_thread_rdwr_runlock( &bdb->bi_cache.c_rwlock );
				/* If this node is being deleted, treat
				 * as if the delete has already finished
				 */
				if ( del ) {
					return DB_NOTFOUND;
				}
				/* otherwise, wait for the info to free up */
				ldap_pvt_thread_yield();
				goto again;
			}
			/* If this info isn't hooked up to its parent yet,
			 * unlock and wait for it to be fully initialized
			 */
			if ( (*eip)->bei_state & CACHE_ENTRY_NOT_LINKED ) {
				bdb_cache_entryinfo_unlock( *eip );
				ldap_pvt_thread_rdwr_runlock( &bdb->bi_cache.c_rwlock );
				ldap_pvt_thread_yield();
				goto again;
			}
			flag |= ID_LOCKED;
		}
		ldap_pvt_thread_rdwr_runlock( &bdb->bi_cache.c_rwlock );
	}

	/* See if the ID exists in the database; add it to the cache if so */
	if ( !*eip ) {
#ifndef BDB_HIER
		rc = bdb_id2entry( op->o_bd, tid, id, &ep );
		if ( rc == 0 ) {
			rc = bdb_cache_find_ndn( op, tid,
				&ep->e_nname, eip );
			if ( *eip ) flag |= ID_LOCKED;
			if ( rc ) {
				ep->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
				bdb_entry_return( bdb, ep, (*eip)->bei_zseq );
#else
				bdb_entry_return( ep );
#endif
				ep = NULL;
			}
		}
#else
		rc = hdb_cache_find_parent(op, tid, id, eip );
		if ( rc == 0 ) flag |= ID_LOCKED;
#endif
	}

	/* Ok, we found the info, do we have the entry? */
	if ( rc == 0 ) {
		if ( !( flag & ID_LOCKED )) {
			bdb_cache_entryinfo_lock( *eip );
			flag |= ID_LOCKED;
		}

		if ( (*eip)->bei_state & CACHE_ENTRY_DELETED ) {
			rc = DB_NOTFOUND;
		} else {
			(*eip)->bei_finders++;
			(*eip)->bei_state |= CACHE_ENTRY_REFERENCED;
			if ( flag & ID_NOENTRY ) {
				bdb_cache_entryinfo_unlock( *eip );
				return 0;
			}
			/* Make sure only one thread tries to load the entry */
load1:
#ifdef SLAP_ZONE_ALLOC
			if ((*eip)->bei_e && !slap_zn_validate(
					bdb->bi_cache.c_zctx, (*eip)->bei_e, (*eip)->bei_zseq)) {
				(*eip)->bei_e = NULL;
				(*eip)->bei_zseq = 0;
			}
#endif
			if ( !(*eip)->bei_e && !((*eip)->bei_state & CACHE_ENTRY_LOADING)) {
				load = 1;
				(*eip)->bei_state |= CACHE_ENTRY_LOADING;
				flag |= ID_CHKPURGE;
			}

			if ( !load ) {
				/* Clear the uncached state if we are not
				 * loading it, i.e it is already cached or
				 * another thread is currently loading it.
				 */
				if ( (*eip)->bei_state & CACHE_ENTRY_NOT_CACHED ) {
					(*eip)->bei_state ^= CACHE_ENTRY_NOT_CACHED;
					flag |= ID_CHKPURGE;
				}
			}

			if ( flag & ID_LOCKED ) {
				bdb_cache_entryinfo_unlock( *eip );
				flag ^= ID_LOCKED;
			}
			rc = bdb_cache_entry_db_lock( bdb, tid, *eip, load, 0, lock );
			if ( (*eip)->bei_state & CACHE_ENTRY_DELETED ) {
				rc = DB_NOTFOUND;
				bdb_cache_entry_db_unlock( bdb, lock );
				bdb_cache_entryinfo_lock( *eip );
				(*eip)->bei_finders--;
				bdb_cache_entryinfo_unlock( *eip );
			} else if ( rc == 0 ) {
				if ( load ) {
					if ( !ep) {
						rc = bdb_id2entry( op->o_bd, tid, id, &ep );
					}
					if ( rc == 0 ) {
						ep->e_private = *eip;
#ifdef BDB_HIER
						while ( (*eip)->bei_state & CACHE_ENTRY_NOT_LINKED )
							ldap_pvt_thread_yield();
						bdb_fix_dn( ep, 0 );
#endif
						bdb_cache_entryinfo_lock( *eip );

						(*eip)->bei_e = ep;
#ifdef SLAP_ZONE_ALLOC
						(*eip)->bei_zseq = *((ber_len_t *)ep - 2);
#endif
						ep = NULL;
						if ( flag & ID_NOCACHE ) {
							/* Set the cached state only if no other thread
							 * found the info while we were loading the entry.
							 */
							if ( (*eip)->bei_finders == 1 ) {
								(*eip)->bei_state |= CACHE_ENTRY_NOT_CACHED;
								flag ^= ID_CHKPURGE;
							}
						}
						bdb_cache_entryinfo_unlock( *eip );
						bdb_cache_lru_link( bdb, *eip );
					}
					if ( rc == 0 ) {
						/* If we succeeded, downgrade back to a readlock. */
						rc = bdb_cache_entry_db_relock( bdb, tid,
							*eip, 0, 0, lock );
					} else {
						/* Otherwise, release the lock. */
						bdb_cache_entry_db_unlock( bdb, lock );
					}
				} else if ( !(*eip)->bei_e ) {
					/* Some other thread is trying to load the entry,
					 * wait for it to finish.
					 */
					bdb_cache_entry_db_unlock( bdb, lock );
					bdb_cache_entryinfo_lock( *eip );
					flag |= ID_LOCKED;
					goto load1;
#ifdef BDB_HIER
				} else {
					/* Check for subtree renames
					 */
					rc = bdb_fix_dn( (*eip)->bei_e, 1 );
					if ( rc ) {
						bdb_cache_entry_db_relock( bdb,
							tid, *eip, 1, 0, lock );
						/* check again in case other modifier did it already */
						if ( bdb_fix_dn( (*eip)->bei_e, 1 ) )
							rc = bdb_fix_dn( (*eip)->bei_e, 2 );
						bdb_cache_entry_db_relock( bdb,
							tid, *eip, 0, 0, lock );
					}
#endif
				}
				bdb_cache_entryinfo_lock( *eip );
				(*eip)->bei_finders--;
				if ( load )
					(*eip)->bei_state ^= CACHE_ENTRY_LOADING;
				bdb_cache_entryinfo_unlock( *eip );
			}
		}
	}
	if ( flag & ID_LOCKED ) {
		bdb_cache_entryinfo_unlock( *eip );
	}
	if ( ep ) {
		ep->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
		bdb_entry_return( bdb, ep, (*eip)->bei_zseq );
#else
		bdb_entry_return( ep );
#endif
	}
	if ( rc == 0 ) {
		int purge = 0;

		if (( flag & ID_CHKPURGE ) || bdb->bi_cache.c_eimax ) {
			ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_count_mutex );
			if ( flag & ID_CHKPURGE ) {
				bdb->bi_cache.c_cursize++;
				if ( !bdb->bi_cache.c_purging && bdb->bi_cache.c_cursize > bdb->bi_cache.c_maxsize ) {
					purge = 1;
					bdb->bi_cache.c_purging = 1;
				}
			} else if ( !bdb->bi_cache.c_purging && bdb->bi_cache.c_eimax && bdb->bi_cache.c_leaves > bdb->bi_cache.c_eimax ) {
				purge = 1;
				bdb->bi_cache.c_purging = 1;
			}
			ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_count_mutex );
		}
		if ( purge )
			bdb_cache_lru_purge( bdb );
	}

#ifdef SLAP_ZONE_ALLOC
	if (rc == 0 && (*eip)->bei_e) {
		slap_zn_rlock(bdb->bi_cache.c_zctx, (*eip)->bei_e);
	}
	slap_zh_runlock(bdb->bi_cache.c_zctx);
#endif
	return rc;
}

int
bdb_cache_children(
	Operation *op,
	DB_TXN *txn,
	Entry *e )
{
	int rc;

	if ( BEI(e)->bei_kids ) {
		return 0;
	}
	if ( BEI(e)->bei_state & CACHE_ENTRY_NO_KIDS ) {
		return DB_NOTFOUND;
	}
	rc = bdb_dn2id_children( op, txn, e );
	if ( rc == DB_NOTFOUND ) {
		BEI(e)->bei_state |= CACHE_ENTRY_NO_KIDS | CACHE_ENTRY_NO_GRANDKIDS;
	}
	return rc;
}

/* Update the cache after a successful database Add. */
int
bdb_cache_add(
	struct bdb_info *bdb,
	EntryInfo *eip,
	Entry *e,
	struct berval *nrdn,
	DB_TXN *txn,
	DB_LOCK *lock )
{
	EntryInfo *new, ei;
	int rc, purge = 0;
#ifdef BDB_HIER
	struct berval rdn = e->e_name;
#endif

	ei.bei_id = e->e_id;
	ei.bei_parent = eip;
	ei.bei_nrdn = *nrdn;
	ei.bei_lockpad = 0;

#if 0
	/* Lock this entry so that bdb_add can run to completion.
	 * It can only fail if BDB has run out of lock resources.
	 */
	rc = bdb_cache_entry_db_lock( bdb, txn, &ei, 0, 0, lock );
	if ( rc ) {
		bdb_cache_entryinfo_unlock( eip );
		return rc;
	}
#endif

#ifdef BDB_HIER
	if ( nrdn->bv_len != e->e_nname.bv_len ) {
		char *ptr = ber_bvchr( &rdn, ',' );
		assert( ptr != NULL );
		rdn.bv_len = ptr - rdn.bv_val;
	}
	ber_dupbv( &ei.bei_rdn, &rdn );
	if ( eip->bei_dkids ) eip->bei_dkids++;
#endif

	if (eip->bei_parent) {
		bdb_cache_entryinfo_lock( eip->bei_parent );
		eip->bei_parent->bei_state &= ~CACHE_ENTRY_NO_GRANDKIDS;
		bdb_cache_entryinfo_unlock( eip->bei_parent );
	}

	rc = bdb_entryinfo_add_internal( bdb, &ei, &new );
	/* bdb_csn_commit can cause this when adding the database root entry */
	if ( new->bei_e ) {
		new->bei_e->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
		bdb_entry_return( bdb, new->bei_e, new->bei_zseq );
#else
		bdb_entry_return( new->bei_e );
#endif
	}
	new->bei_e = e;
	e->e_private = new;
	new->bei_state |= CACHE_ENTRY_NO_KIDS | CACHE_ENTRY_NO_GRANDKIDS;
	eip->bei_state &= ~CACHE_ENTRY_NO_KIDS;
	bdb_cache_entryinfo_unlock( eip );

	ldap_pvt_thread_rdwr_wunlock( &bdb->bi_cache.c_rwlock );
	ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_count_mutex );
	++bdb->bi_cache.c_cursize;
	if ( bdb->bi_cache.c_cursize > bdb->bi_cache.c_maxsize &&
		!bdb->bi_cache.c_purging ) {
		purge = 1;
		bdb->bi_cache.c_purging = 1;
	}
	ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_count_mutex );

	new->bei_finders = 1;
	bdb_cache_lru_link( bdb, new );

	if ( purge )
		bdb_cache_lru_purge( bdb );

	return rc;
}

void bdb_cache_deref(
	EntryInfo *ei
	)
{
	bdb_cache_entryinfo_lock( ei );
	ei->bei_finders--;
	bdb_cache_entryinfo_unlock( ei );
}

int
bdb_cache_modify(
	struct bdb_info *bdb,
	Entry *e,
	Attribute *newAttrs,
	DB_TXN *txn,
	DB_LOCK *lock )
{
	EntryInfo *ei = BEI(e);
	int rc;
	/* Get write lock on data */
	rc = bdb_cache_entry_db_relock( bdb, txn, ei, 1, 0, lock );

	/* If we've done repeated mods on a cached entry, then e_attrs
	 * is no longer contiguous with the entry, and must be freed.
	 */
	if ( ! rc ) {
		if ( (void *)e->e_attrs != (void *)(e+1) ) {
			attrs_free( e->e_attrs ); 
		}
		e->e_attrs = newAttrs;
	}
	return rc;
}

/*
 * Change the rdn in the entryinfo. Also move to a new parent if needed.
 */
int
bdb_cache_modrdn(
	struct bdb_info *bdb,
	Entry *e,
	struct berval *nrdn,
	Entry *new,
	EntryInfo *ein,
	DB_TXN *txn,
	DB_LOCK *lock )
{
	EntryInfo *ei = BEI(e), *pei;
	int rc;
#ifdef BDB_HIER
	struct berval rdn;
#endif

	/* Get write lock on data */
	rc =  bdb_cache_entry_db_relock( bdb, txn, ei, 1, 0, lock );
	if ( rc ) return rc;

	/* If we've done repeated mods on a cached entry, then e_attrs
	 * is no longer contiguous with the entry, and must be freed.
	 */
	if ( (void *)e->e_attrs != (void *)(e+1) ) {
		attrs_free( e->e_attrs );
	}
	e->e_attrs = new->e_attrs;
	if( e->e_nname.bv_val < e->e_bv.bv_val ||
		e->e_nname.bv_val > e->e_bv.bv_val + e->e_bv.bv_len )
	{
		ch_free(e->e_name.bv_val);
		ch_free(e->e_nname.bv_val);
	}
	e->e_name = new->e_name;
	e->e_nname = new->e_nname;

	/* Lock the parent's kids AVL tree */
	pei = ei->bei_parent;
	bdb_cache_entryinfo_lock( pei );
	avl_delete( &pei->bei_kids, (caddr_t) ei, bdb_rdn_cmp );
	free( ei->bei_nrdn.bv_val );
	ber_dupbv( &ei->bei_nrdn, nrdn );

#ifdef BDB_HIER
	free( ei->bei_rdn.bv_val );

	rdn = e->e_name;
	if ( nrdn->bv_len != e->e_nname.bv_len ) {
		char *ptr = ber_bvchr(&rdn, ',');
		assert( ptr != NULL );
		rdn.bv_len = ptr - rdn.bv_val;
	}
	ber_dupbv( &ei->bei_rdn, &rdn );

	/* If new parent, decrement kid counts */
	if ( ein ) {
		pei->bei_ckids--;
		if ( pei->bei_dkids ) {
			pei->bei_dkids--;
			if ( pei->bei_dkids < 2 )
				pei->bei_state |= CACHE_ENTRY_NO_KIDS | CACHE_ENTRY_NO_GRANDKIDS;
		}
	}
#endif

	if (!ein) {
		ein = ei->bei_parent;
	} else {
		ei->bei_parent = ein;
		bdb_cache_entryinfo_unlock( pei );
		bdb_cache_entryinfo_lock( ein );

		/* new parent now has kids */
		if ( ein->bei_state & CACHE_ENTRY_NO_KIDS )
			ein->bei_state ^= CACHE_ENTRY_NO_KIDS;
		/* grandparent has grandkids */
		if ( ein->bei_parent )
			ein->bei_parent->bei_state &= ~CACHE_ENTRY_NO_GRANDKIDS;
#ifdef BDB_HIER
		/* parent might now have grandkids */
		if ( ein->bei_state & CACHE_ENTRY_NO_GRANDKIDS &&
			!(ei->bei_state & CACHE_ENTRY_NO_KIDS))
			ein->bei_state ^= CACHE_ENTRY_NO_GRANDKIDS;

		ein->bei_ckids++;
		if ( ein->bei_dkids ) ein->bei_dkids++;
#endif
	}

#ifdef BDB_HIER
	/* Record the generation number of this change */
	ldap_pvt_thread_mutex_lock( &bdb->bi_modrdns_mutex );
	bdb->bi_modrdns++;
	ei->bei_modrdns = bdb->bi_modrdns;
	ldap_pvt_thread_mutex_unlock( &bdb->bi_modrdns_mutex );
#endif

	avl_insert( &ein->bei_kids, ei, bdb_rdn_cmp, avl_dup_error );
	bdb_cache_entryinfo_unlock( ein );
	return rc;
}
/*
 * cache_delete - delete the entry e from the cache. 
 *
 * returns:	0	e was deleted ok
 *		1	e was not in the cache
 *		-1	something bad happened
 */
int
bdb_cache_delete(
	struct bdb_info *bdb,
    Entry		*e,
    DB_TXN *txn,
    DB_LOCK	*lock )
{
	EntryInfo *ei = BEI(e);
	int	rc, busy = 0, counter = 0;

	assert( e->e_private != NULL );

	/* Lock the entry's info */
	bdb_cache_entryinfo_lock( ei );

	/* Set this early, warn off any queriers */
	ei->bei_state |= CACHE_ENTRY_DELETED;

	if (( ei->bei_state & ( CACHE_ENTRY_NOT_LINKED |
		CACHE_ENTRY_LOADING | CACHE_ENTRY_ONELEVEL )) ||
		ei->bei_finders > 0 )
		busy = 1;

	bdb_cache_entryinfo_unlock( ei );

	while ( busy && counter < 1000) {
		ldap_pvt_thread_yield();
		busy = 0;
		bdb_cache_entryinfo_lock( ei );
		if (( ei->bei_state & ( CACHE_ENTRY_NOT_LINKED |
			CACHE_ENTRY_LOADING | CACHE_ENTRY_ONELEVEL )) ||
			ei->bei_finders > 0 )
			busy = 1;
		bdb_cache_entryinfo_unlock( ei );
		counter ++;
	}
	if( busy ) {
		bdb_cache_entryinfo_lock( ei );
		ei->bei_state ^= CACHE_ENTRY_DELETED;
		bdb_cache_entryinfo_unlock( ei );
		return DB_LOCK_DEADLOCK;
	}

	/* Get write lock on the data */
	rc = bdb_cache_entry_db_relock( bdb, txn, ei, 1, 0, lock );
	if ( rc ) {
		bdb_cache_entryinfo_lock( ei );
		/* couldn't lock, undo and give up */
		ei->bei_state ^= CACHE_ENTRY_DELETED;
		bdb_cache_entryinfo_unlock( ei );
		return rc;
	}

	Debug( LDAP_DEBUG_TRACE, "====> bdb_cache_delete( %ld )\n",
		e->e_id, 0, 0 );

	/* set lru mutex */
	ldap_pvt_thread_mutex_lock( &bdb->bi_cache.c_lru_mutex );

	bdb_cache_entryinfo_lock( ei->bei_parent );
	bdb_cache_entryinfo_lock( ei );
	rc = bdb_cache_delete_internal( &bdb->bi_cache, e->e_private, 1 );
	bdb_cache_entryinfo_unlock( ei );

	/* free lru mutex */
	ldap_pvt_thread_mutex_unlock( &bdb->bi_cache.c_lru_mutex );

	return( rc );
}

void
bdb_cache_delete_cleanup(
	Cache *cache,
	EntryInfo *ei )
{
	/* Enter with ei locked */

	/* already freed? */
	if ( !ei->bei_parent ) return;

	if ( ei->bei_e ) {
		ei->bei_e->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
		bdb_entry_return( ei->bei_bdb, ei->bei_e, ei->bei_zseq );
#else
		bdb_entry_return( ei->bei_e );
#endif
		ei->bei_e = NULL;
	}

	bdb_cache_entryinfo_unlock( ei );
	bdb_cache_entryinfo_free( cache, ei );
}

static int
bdb_cache_delete_internal(
    Cache	*cache,
    EntryInfo		*e,
    int		decr )
{
	int rc = 0;	/* return code */
	int decr_leaf = 0;

	/* already freed? */
	if ( !e->bei_parent ) {
		assert(0);
		return -1;
	}

#ifdef BDB_HIER
	e->bei_parent->bei_ckids--;
	if ( decr && e->bei_parent->bei_dkids ) e->bei_parent->bei_dkids--;
#endif
	/* dn tree */
	if ( avl_delete( &e->bei_parent->bei_kids, (caddr_t) e, bdb_rdn_cmp )
		== NULL )
	{
		rc = -1;
		assert(0);
	}
	if ( e->bei_parent->bei_kids )
		decr_leaf = 1;

	ldap_pvt_thread_rdwr_wlock( &cache->c_rwlock );
	/* id tree */
	if ( avl_delete( &cache->c_idtree, (caddr_t) e, bdb_id_cmp )) {
		cache->c_eiused--;
		if ( decr_leaf )
			cache->c_leaves--;
	} else {
		rc = -1;
		assert(0);
	}
	ldap_pvt_thread_rdwr_wunlock( &cache->c_rwlock );
	bdb_cache_entryinfo_unlock( e->bei_parent );

	if ( rc == 0 ){
		/* lru */
		LRU_DEL( cache, e );

		if ( e->bei_e ) {
			ldap_pvt_thread_mutex_lock( &cache->c_count_mutex );
			cache->c_cursize--;
			ldap_pvt_thread_mutex_unlock( &cache->c_count_mutex );
		}
	}

	return( rc );
}

static void
bdb_entryinfo_release( void *data )
{
	EntryInfo *ei = (EntryInfo *)data;
	if ( ei->bei_kids ) {
		avl_free( ei->bei_kids, NULL );
	}
	if ( ei->bei_e ) {
		ei->bei_e->e_private = NULL;
#ifdef SLAP_ZONE_ALLOC
		bdb_entry_return( ei->bei_bdb, ei->bei_e, ei->bei_zseq );
#else
		bdb_entry_return( ei->bei_e );
#endif
	}
	bdb_cache_entryinfo_destroy( ei );
}

void
bdb_cache_release_all( Cache *cache )
{
	/* set cache write lock */
	ldap_pvt_thread_rdwr_wlock( &cache->c_rwlock );
	/* set lru mutex */
	ldap_pvt_thread_mutex_lock( &cache->c_lru_mutex );

	Debug( LDAP_DEBUG_TRACE, "====> bdb_cache_release_all\n", 0, 0, 0 );

	avl_free( cache->c_dntree.bei_kids, NULL );
	avl_free( cache->c_idtree, bdb_entryinfo_release );
	for (;cache->c_eifree;cache->c_eifree = cache->c_lruhead) {
		cache->c_lruhead = cache->c_eifree->bei_lrunext;
		bdb_cache_entryinfo_destroy(cache->c_eifree);
	}
	cache->c_cursize = 0;
	cache->c_eiused = 0;
	cache->c_leaves = 0;
	cache->c_idtree = NULL;
	cache->c_lruhead = NULL;
	cache->c_lrutail = NULL;
	cache->c_dntree.bei_kids = NULL;

	/* free lru mutex */
	ldap_pvt_thread_mutex_unlock( &cache->c_lru_mutex );
	/* free cache write lock */
	ldap_pvt_thread_rdwr_wunlock( &cache->c_rwlock );
}

#ifdef LDAP_DEBUG
static void
bdb_lru_count( Cache *cache )
{
	EntryInfo	*e;
	int ei = 0, ent = 0, nc = 0;

	for ( e = cache->c_lrutail; ; ) {
		ei++;
		if ( e->bei_e ) {
			ent++;
			if ( e->bei_state & CACHE_ENTRY_NOT_CACHED )
				nc++;
			fprintf( stderr, "ei %d entry %p dn %s\n", ei, (void *) e->bei_e, e->bei_e->e_name.bv_val );
		}
		e = e->bei_lrunext;
		if ( e == cache->c_lrutail )
			break;
	}
	fprintf( stderr, "counted %d entryInfos and %d entries, %d notcached\n",
		ei, ent, nc );
	ei = 0;
	for ( e = cache->c_lrutail; ; ) {
		ei++;
		e = e->bei_lruprev;
		if ( e == cache->c_lrutail )
			break;
	}
	fprintf( stderr, "counted %d entryInfos (on lruprev)\n", ei );
}

#ifdef SLAPD_UNUSED
static void
bdb_lru_print( Cache *cache )
{
	EntryInfo	*e;

	fprintf( stderr, "LRU circle head: %p\n", (void *) cache->c_lruhead );
	fprintf( stderr, "LRU circle (tail forward):\n" );
	for ( e = cache->c_lrutail; ; ) {
		fprintf( stderr, "\t%p, %p id %ld rdn \"%s\"\n",
			(void *) e, (void *) e->bei_e, e->bei_id, e->bei_nrdn.bv_val );
		e = e->bei_lrunext;
		if ( e == cache->c_lrutail )
			break;
	}
	fprintf( stderr, "LRU circle (tail backward):\n" );
	for ( e = cache->c_lrutail; ; ) {
		fprintf( stderr, "\t%p, %p id %ld rdn \"%s\"\n",
			(void *) e, (void *) e->bei_e, e->bei_id, e->bei_nrdn.bv_val );
		e = e->bei_lruprev;
		if ( e == cache->c_lrutail )
			break;
	}
}

static int
bdb_entryinfo_print(void *data, void *arg)
{
	EntryInfo *e = data;
	fprintf( stderr, "\t%p, %p id %ld rdn \"%s\"\n",
		(void *) e, (void *) e->bei_e, e->bei_id, e->bei_nrdn.bv_val );
	return 0;
}

static void
bdb_idtree_print(Cache *cache)
{
	avl_apply( cache->c_idtree, bdb_entryinfo_print, NULL, -1, AVL_INORDER );
}
#endif
#endif

static void
bdb_reader_free( void *key, void *data )
{
	/* DB_ENV *env = key; */
	DB_TXN *txn = data;

	if ( txn ) TXN_ABORT( txn );
}

/* free up any keys used by the main thread */
void
bdb_reader_flush( DB_ENV *env )
{
	void *data;
	void *ctx = ldap_pvt_thread_pool_context();

	if ( !ldap_pvt_thread_pool_getkey( ctx, env, &data, NULL ) ) {
		ldap_pvt_thread_pool_setkey( ctx, env, NULL, 0, NULL, NULL );
		bdb_reader_free( env, data );
	}
}

int
bdb_reader_get( Operation *op, DB_ENV *env, DB_TXN **txn )
{
	int i, rc;
	void *data;
	void *ctx;

	if ( !env || !txn ) return -1;

	/* If no op was provided, try to find the ctx anyway... */
	if ( op ) {
		ctx = op->o_threadctx;
	} else {
		ctx = ldap_pvt_thread_pool_context();
	}

	/* Shouldn't happen unless we're single-threaded */
	if ( !ctx ) {
		*txn = NULL;
		return 0;
	}

	if ( ldap_pvt_thread_pool_getkey( ctx, env, &data, NULL ) ) {
		for ( i=0, rc=1; rc != 0 && i<4; i++ ) {
			rc = TXN_BEGIN( env, NULL, txn, DB_READ_COMMITTED );
			if (rc) ldap_pvt_thread_yield();
		}
		if ( rc != 0) {
			return rc;
		}
		data = *txn;
		if ( ( rc = ldap_pvt_thread_pool_setkey( ctx, env,
			data, bdb_reader_free, NULL, NULL ) ) ) {
			TXN_ABORT( *txn );
			Debug( LDAP_DEBUG_ANY, "bdb_reader_get: err %s(%d)\n",
				db_strerror(rc), rc, 0 );

			return rc;
		}
	} else {
		*txn = data;
	}
	return 0;
}
