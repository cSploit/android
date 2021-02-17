/* idl.c - ldap id list handling routines */
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
#include <ac/string.h>

#include "back-bdb.h"
#include "idl.h"

#define IDL_MAX(x,y)	( (x) > (y) ? (x) : (y) )
#define IDL_MIN(x,y)	( (x) < (y) ? (x) : (y) )
#define IDL_CMP(x,y)	( (x) < (y) ? -1 : (x) > (y) )

#define IDL_LRU_DELETE( bdb, e ) do { \
	if ( (e) == (bdb)->bi_idl_lru_head ) { \
		if ( (e)->idl_lru_next == (bdb)->bi_idl_lru_head ) { \
			(bdb)->bi_idl_lru_head = NULL; \
		} else { \
			(bdb)->bi_idl_lru_head = (e)->idl_lru_next; \
		} \
	} \
	if ( (e) == (bdb)->bi_idl_lru_tail ) { \
		if ( (e)->idl_lru_prev == (bdb)->bi_idl_lru_tail ) { \
			assert( (bdb)->bi_idl_lru_head == NULL ); \
			(bdb)->bi_idl_lru_tail = NULL; \
		} else { \
			(bdb)->bi_idl_lru_tail = (e)->idl_lru_prev; \
		} \
	} \
	(e)->idl_lru_next->idl_lru_prev = (e)->idl_lru_prev; \
	(e)->idl_lru_prev->idl_lru_next = (e)->idl_lru_next; \
} while ( 0 )

static int
bdb_idl_entry_cmp( const void *v_idl1, const void *v_idl2 )
{
	const bdb_idl_cache_entry_t *idl1 = v_idl1, *idl2 = v_idl2;
	int rc;

	if ((rc = SLAP_PTRCMP( idl1->db, idl2->db ))) return rc;
	if ((rc = idl1->kstr.bv_len - idl2->kstr.bv_len )) return rc;
	return ( memcmp ( idl1->kstr.bv_val, idl2->kstr.bv_val , idl1->kstr.bv_len ) );
}

#if IDL_DEBUG > 0
static void idl_check( ID *ids )
{
	if( BDB_IDL_IS_RANGE( ids ) ) {
		assert( BDB_IDL_RANGE_FIRST(ids) <= BDB_IDL_RANGE_LAST(ids) );
	} else {
		ID i;
		for( i=1; i < ids[0]; i++ ) {
			assert( ids[i+1] > ids[i] );
		}
	}
}

#if IDL_DEBUG > 1
static void idl_dump( ID *ids )
{
	if( BDB_IDL_IS_RANGE( ids ) ) {
		Debug( LDAP_DEBUG_ANY,
			"IDL: range ( %ld - %ld )\n",
			(long) BDB_IDL_RANGE_FIRST( ids ),
			(long) BDB_IDL_RANGE_LAST( ids ) );

	} else {
		ID i;
		Debug( LDAP_DEBUG_ANY, "IDL: size %ld", (long) ids[0], 0, 0 );

		for( i=1; i<=ids[0]; i++ ) {
			if( i % 16 == 1 ) {
				Debug( LDAP_DEBUG_ANY, "\n", 0, 0, 0 );
			}
			Debug( LDAP_DEBUG_ANY, "  %02lx", (long) ids[i], 0, 0 );
		}

		Debug( LDAP_DEBUG_ANY, "\n", 0, 0, 0 );
	}

	idl_check( ids );
}
#endif /* IDL_DEBUG > 1 */
#endif /* IDL_DEBUG > 0 */

unsigned bdb_idl_search( ID *ids, ID id )
{
#define IDL_BINARY_SEARCH 1
#ifdef IDL_BINARY_SEARCH
	/*
	 * binary search of id in ids
	 * if found, returns position of id
	 * if not found, returns first postion greater than id
	 */
	unsigned base = 0;
	unsigned cursor = 1;
	int val = 0;
	unsigned n = ids[0];

#if IDL_DEBUG > 0
	idl_check( ids );
#endif

	while( 0 < n ) {
		unsigned pivot = n >> 1;
		cursor = base + pivot + 1;
		val = IDL_CMP( id, ids[cursor] );

		if( val < 0 ) {
			n = pivot;

		} else if ( val > 0 ) {
			base = cursor;
			n -= pivot + 1;

		} else {
			return cursor;
		}
	}
	
	if( val > 0 ) {
		++cursor;
	}
	return cursor;

#else
	/* (reverse) linear search */
	int i;

#if IDL_DEBUG > 0
	idl_check( ids );
#endif

	for( i=ids[0]; i; i-- ) {
		if( id > ids[i] ) {
			break;
		}
	}

	return i+1;
#endif
}

int bdb_idl_insert( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "insert: %04lx at %d\n", (long) id, x, 0 );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (BDB_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= BDB_IDL_RANGE_FIRST(ids) && id <= BDB_IDL_RANGE_LAST(ids))
			return -1;
		if (id < BDB_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > BDB_IDL_RANGE_LAST(ids))
			ids[2] = id;
		return 0;
	}

	x = bdb_idl_search( ids, id );
	assert( x > 0 );

	if( x < 1 ) {
		/* internal error */
		return -2;
	}

	if ( x <= ids[0] && ids[x] == id ) {
		/* duplicate */
		return -1;
	}

	if ( ++ids[0] >= BDB_IDL_DB_MAX ) {
		if( id < ids[1] ) {
			ids[1] = id;
			ids[2] = ids[ids[0]-1];
		} else if ( ids[ids[0]-1] < id ) {
			ids[2] = id;
		} else {
			ids[2] = ids[ids[0]-1];
		}
		ids[0] = NOID;
	
	} else {
		/* insert id */
		AC_MEMCPY( &ids[x+1], &ids[x], (ids[0]-x) * sizeof(ID) );
		ids[x] = id;
	}

#if IDL_DEBUG > 1
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	return 0;
}

int bdb_idl_delete( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "delete: %04lx at %d\n", (long) id, x, 0 );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (BDB_IDL_IS_RANGE( ids )) {
		/* If deleting a range boundary, adjust */
		if ( ids[1] == id )
			ids[1]++;
		else if ( ids[2] == id )
			ids[2]--;
		/* deleting from inside a range is a no-op */

		/* If the range has collapsed, re-adjust */
		if ( ids[1] > ids[2] )
			ids[0] = 0;
		else if ( ids[1] == ids[2] )
			ids[1] = 1;
		return 0;
	}

	x = bdb_idl_search( ids, id );
	assert( x > 0 );

	if( x <= 0 ) {
		/* internal error */
		return -2;
	}

	if( x > ids[0] || ids[x] != id ) {
		/* not found */
		return -1;

	} else if ( --ids[0] == 0 ) {
		if( x != 1 ) {
			return -3;
		}

	} else {
		AC_MEMCPY( &ids[x], &ids[x+1], (1+ids[0]-x) * sizeof(ID) );
	}

#if IDL_DEBUG > 1
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	return 0;
}

static char *
bdb_show_key(
	DBT		*key,
	char		*buf )
{
	if ( key->size == 4 /* LUTIL_HASH_BYTES */ ) {
		unsigned char *c = key->data;
		sprintf( buf, "[%02x%02x%02x%02x]", c[0], c[1], c[2], c[3] );
		return buf;
	} else {
		return key->data;
	}
}

/* Find a db/key pair in the IDL cache. If ids is non-NULL,
 * copy the cached IDL into it, otherwise just return the status.
 */
int
bdb_idl_cache_get(
	struct bdb_info	*bdb,
	DB			*db,
	DBT			*key,
	ID			*ids )
{
	bdb_idl_cache_entry_t idl_tmp;
	bdb_idl_cache_entry_t *matched_idl_entry;
	int rc = LDAP_NO_SUCH_OBJECT;

	DBT2bv( key, &idl_tmp.kstr );
	idl_tmp.db = db;
	ldap_pvt_thread_rdwr_rlock( &bdb->bi_idl_tree_rwlock );
	matched_idl_entry = avl_find( bdb->bi_idl_tree, &idl_tmp,
				      bdb_idl_entry_cmp );
	if ( matched_idl_entry != NULL ) {
		if ( matched_idl_entry->idl && ids )
			BDB_IDL_CPY( ids, matched_idl_entry->idl );
		matched_idl_entry->idl_flags |= CACHE_ENTRY_REFERENCED;
		if ( matched_idl_entry->idl )
			rc = LDAP_SUCCESS;
		else
			rc = DB_NOTFOUND;
	}
	ldap_pvt_thread_rdwr_runlock( &bdb->bi_idl_tree_rwlock );

	return rc;
}

void
bdb_idl_cache_put(
	struct bdb_info	*bdb,
	DB			*db,
	DBT			*key,
	ID			*ids,
	int			rc )
{
	bdb_idl_cache_entry_t idl_tmp;
	bdb_idl_cache_entry_t *ee, *eprev;

	if ( rc == DB_NOTFOUND || BDB_IDL_IS_ZERO( ids ))
		return;

	DBT2bv( key, &idl_tmp.kstr );

	ee = (bdb_idl_cache_entry_t *) ch_malloc(
		sizeof( bdb_idl_cache_entry_t ) );
	ee->db = db;
	ee->idl = (ID*) ch_malloc( BDB_IDL_SIZEOF ( ids ) );
	BDB_IDL_CPY( ee->idl, ids );

	ee->idl_lru_prev = NULL;
	ee->idl_lru_next = NULL;
	ee->idl_flags = 0;
	ber_dupbv( &ee->kstr, &idl_tmp.kstr );
	ldap_pvt_thread_rdwr_wlock( &bdb->bi_idl_tree_rwlock );
	if ( avl_insert( &bdb->bi_idl_tree, (caddr_t) ee,
		bdb_idl_entry_cmp, avl_dup_error ))
	{
		ch_free( ee->kstr.bv_val );
		ch_free( ee->idl );
		ch_free( ee );
		ldap_pvt_thread_rdwr_wunlock( &bdb->bi_idl_tree_rwlock );
		return;
	}
	ldap_pvt_thread_mutex_lock( &bdb->bi_idl_tree_lrulock );
	/* LRU_ADD */
	if ( bdb->bi_idl_lru_head ) {
		assert( bdb->bi_idl_lru_tail != NULL );
		assert( bdb->bi_idl_lru_head->idl_lru_prev != NULL );
		assert( bdb->bi_idl_lru_head->idl_lru_next != NULL );

		ee->idl_lru_next = bdb->bi_idl_lru_head;
		ee->idl_lru_prev = bdb->bi_idl_lru_head->idl_lru_prev;
		bdb->bi_idl_lru_head->idl_lru_prev->idl_lru_next = ee;
		bdb->bi_idl_lru_head->idl_lru_prev = ee;
	} else {
		ee->idl_lru_next = ee->idl_lru_prev = ee;
		bdb->bi_idl_lru_tail = ee;
	}
	bdb->bi_idl_lru_head = ee;

	if ( bdb->bi_idl_cache_size >= bdb->bi_idl_cache_max_size ) {
		int i;
		eprev = bdb->bi_idl_lru_tail;
		for ( i = 0; (ee = eprev) != NULL && i < 10; i++ ) {
			eprev = ee->idl_lru_prev;
			if ( eprev == ee ) {
				eprev = NULL;
			}
			if ( ee->idl_flags & CACHE_ENTRY_REFERENCED ) {
				ee->idl_flags ^= CACHE_ENTRY_REFERENCED;
				continue;
			}
			if ( avl_delete( &bdb->bi_idl_tree, (caddr_t) ee,
				    bdb_idl_entry_cmp ) == NULL ) {
				Debug( LDAP_DEBUG_ANY, "=> bdb_idl_cache_put: "
					"AVL delete failed\n",
					0, 0, 0 );
			}
			IDL_LRU_DELETE( bdb, ee );
			i++;
			--bdb->bi_idl_cache_size;
			ch_free( ee->kstr.bv_val );
			ch_free( ee->idl );
			ch_free( ee );
		}
		bdb->bi_idl_lru_tail = eprev;
		assert( bdb->bi_idl_lru_tail != NULL
			|| bdb->bi_idl_lru_head == NULL );
	}
	bdb->bi_idl_cache_size++;
	ldap_pvt_thread_mutex_unlock( &bdb->bi_idl_tree_lrulock );
	ldap_pvt_thread_rdwr_wunlock( &bdb->bi_idl_tree_rwlock );
}

void
bdb_idl_cache_del(
	struct bdb_info	*bdb,
	DB			*db,
	DBT			*key )
{
	bdb_idl_cache_entry_t *matched_idl_entry, idl_tmp;
	DBT2bv( key, &idl_tmp.kstr );
	idl_tmp.db = db;
	ldap_pvt_thread_rdwr_wlock( &bdb->bi_idl_tree_rwlock );
	matched_idl_entry = avl_find( bdb->bi_idl_tree, &idl_tmp,
				      bdb_idl_entry_cmp );
	if ( matched_idl_entry != NULL ) {
		if ( avl_delete( &bdb->bi_idl_tree, (caddr_t) matched_idl_entry,
				    bdb_idl_entry_cmp ) == NULL ) {
			Debug( LDAP_DEBUG_ANY, "=> bdb_idl_cache_del: "
				"AVL delete failed\n",
				0, 0, 0 );
		}
		--bdb->bi_idl_cache_size;
		ldap_pvt_thread_mutex_lock( &bdb->bi_idl_tree_lrulock );
		IDL_LRU_DELETE( bdb, matched_idl_entry );
		ldap_pvt_thread_mutex_unlock( &bdb->bi_idl_tree_lrulock );
		free( matched_idl_entry->kstr.bv_val );
		if ( matched_idl_entry->idl )
			free( matched_idl_entry->idl );
		free( matched_idl_entry );
	}
	ldap_pvt_thread_rdwr_wunlock( &bdb->bi_idl_tree_rwlock );
}

void
bdb_idl_cache_add_id(
	struct bdb_info	*bdb,
	DB			*db,
	DBT			*key,
	ID			id )
{
	bdb_idl_cache_entry_t *cache_entry, idl_tmp;
	DBT2bv( key, &idl_tmp.kstr );
	idl_tmp.db = db;
	ldap_pvt_thread_rdwr_wlock( &bdb->bi_idl_tree_rwlock );
	cache_entry = avl_find( bdb->bi_idl_tree, &idl_tmp,
				      bdb_idl_entry_cmp );
	if ( cache_entry != NULL ) {
		if ( !BDB_IDL_IS_RANGE( cache_entry->idl ) &&
			cache_entry->idl[0] < BDB_IDL_DB_MAX ) {
			size_t s = BDB_IDL_SIZEOF( cache_entry->idl ) + sizeof(ID);
			cache_entry->idl = ch_realloc( cache_entry->idl, s );
		}
		bdb_idl_insert( cache_entry->idl, id );
	}
	ldap_pvt_thread_rdwr_wunlock( &bdb->bi_idl_tree_rwlock );
}

void
bdb_idl_cache_del_id(
	struct bdb_info	*bdb,
	DB			*db,
	DBT			*key,
	ID			id )
{
	bdb_idl_cache_entry_t *cache_entry, idl_tmp;
	DBT2bv( key, &idl_tmp.kstr );
	idl_tmp.db = db;
	ldap_pvt_thread_rdwr_wlock( &bdb->bi_idl_tree_rwlock );
	cache_entry = avl_find( bdb->bi_idl_tree, &idl_tmp,
				      bdb_idl_entry_cmp );
	if ( cache_entry != NULL ) {
		bdb_idl_delete( cache_entry->idl, id );
		if ( cache_entry->idl[0] == 0 ) {
			if ( avl_delete( &bdb->bi_idl_tree, (caddr_t) cache_entry,
						bdb_idl_entry_cmp ) == NULL ) {
				Debug( LDAP_DEBUG_ANY, "=> bdb_idl_cache_del: "
					"AVL delete failed\n",
					0, 0, 0 );
			}
			--bdb->bi_idl_cache_size;
			ldap_pvt_thread_mutex_lock( &bdb->bi_idl_tree_lrulock );
			IDL_LRU_DELETE( bdb, cache_entry );
			ldap_pvt_thread_mutex_unlock( &bdb->bi_idl_tree_lrulock );
			free( cache_entry->kstr.bv_val );
			free( cache_entry->idl );
			free( cache_entry );
		}
	}
	ldap_pvt_thread_rdwr_wunlock( &bdb->bi_idl_tree_rwlock );
}

int
bdb_idl_fetch_key(
	BackendDB	*be,
	DB			*db,
	DB_TXN		*txn,
	DBT			*key,
	ID			*ids,
	DBC                     **saved_cursor,
	int                     get_flag )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	int rc;
	DBT data, key2, *kptr;
	DBC *cursor;
	ID *i;
	void *ptr;
	size_t len;
	int rc2;
	int flags = bdb->bi_db_opflags | DB_MULTIPLE;
	int opflag;

	/* If using BerkeleyDB 4.0, the buf must be large enough to
	 * grab the entire IDL in one get(), otherwise BDB will leak
	 * resources on subsequent get's.  We can safely call get()
	 * twice - once for the data, and once to get the DB_NOTFOUND
	 * result meaning there's no more data. See ITS#2040 for details.
	 * This bug is fixed in BDB 4.1 so a smaller buffer will work if
	 * stack space is too limited.
	 *
	 * configure now requires Berkeley DB 4.1.
	 */
#if DB_VERSION_FULL < 0x04010000
#	define BDB_ENOUGH 5
#else
	/* We sometimes test with tiny IDLs, and BDB always wants buffers
	 * that are at least one page in size.
	 */
# if BDB_IDL_DB_SIZE < 4096
#   define BDB_ENOUGH 2048
# else
#	define BDB_ENOUGH 1
# endif
#endif
	ID buf[BDB_IDL_DB_SIZE*BDB_ENOUGH];

	char keybuf[16];

	Debug( LDAP_DEBUG_ARGS,
		"bdb_idl_fetch_key: %s\n", 
		bdb_show_key( key, keybuf ), 0, 0 );

	assert( ids != NULL );

	if ( saved_cursor && *saved_cursor ) {
		opflag = DB_NEXT;
	} else if ( get_flag == LDAP_FILTER_GE ) {
		opflag = DB_SET_RANGE;
	} else if ( get_flag == LDAP_FILTER_LE ) {
		opflag = DB_FIRST;
	} else {
		opflag = DB_SET;
	}

	/* only non-range lookups can use the IDL cache */
	if ( bdb->bi_idl_cache_size && opflag == DB_SET ) {
		rc = bdb_idl_cache_get( bdb, db, key, ids );
		if ( rc != LDAP_NO_SUCH_OBJECT ) return rc;
	}

	DBTzero( &data );

	data.data = buf;
	data.ulen = sizeof(buf);
	data.flags = DB_DBT_USERMEM;

	/* If we're not reusing an existing cursor, get a new one */
	if( opflag != DB_NEXT ) {
		rc = db->cursor( db, txn, &cursor, bdb->bi_db_opflags );
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
				"cursor failed: %s (%d)\n", db_strerror(rc), rc, 0 );
			return rc;
		}
	} else {
		cursor = *saved_cursor;
	}
	
	/* If this is a LE lookup, save original key so we can determine
	 * when to stop. If this is a GE lookup, save the key since it
	 * will be overwritten.
	 */
	if ( get_flag == LDAP_FILTER_LE || get_flag == LDAP_FILTER_GE ) {
		DBTzero( &key2 );
		key2.flags = DB_DBT_USERMEM;
		key2.ulen = sizeof(keybuf);
		key2.data = keybuf;
		key2.size = key->size;
		AC_MEMCPY( keybuf, key->data, key->size );
		kptr = &key2;
	} else {
		kptr = key;
	}
	len = key->size;
	rc = cursor->c_get( cursor, kptr, &data, flags | opflag );

	/* skip presence key on range inequality lookups */
	while (rc == 0 && kptr->size != len) {
		rc = cursor->c_get( cursor, kptr, &data, flags | DB_NEXT_NODUP );
	}
	/* If we're doing a LE compare and the new key is greater than
	 * our search key, we're done
	 */
	if (rc == 0 && get_flag == LDAP_FILTER_LE && memcmp( kptr->data,
		key->data, key->size ) > 0 ) {
		rc = DB_NOTFOUND;
	}
	if (rc == 0) {
		i = ids;
		while (rc == 0) {
			u_int8_t *j;

			DB_MULTIPLE_INIT( ptr, &data );
			while (ptr) {
				DB_MULTIPLE_NEXT(ptr, &data, j, len);
				if (j) {
					++i;
					BDB_DISK2ID( j, i );
				}
			}
			rc = cursor->c_get( cursor, key, &data, flags | DB_NEXT_DUP );
		}
		if ( rc == DB_NOTFOUND ) rc = 0;
		ids[0] = i - ids;
		/* On disk, a range is denoted by 0 in the first element */
		if (ids[1] == 0) {
			if (ids[0] != BDB_IDL_RANGE_SIZE) {
				Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
					"range size mismatch: expected %d, got %ld\n",
					BDB_IDL_RANGE_SIZE, ids[0], 0 );
				cursor->c_close( cursor );
				return -1;
			}
			BDB_IDL_RANGE( ids, ids[2], ids[3] );
		}
		data.size = BDB_IDL_SIZEOF(ids);
	}

	if ( saved_cursor && rc == 0 ) {
		if ( !*saved_cursor )
			*saved_cursor = cursor;
		rc2 = 0;
	}
	else
		rc2 = cursor->c_close( cursor );
	if (rc2) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
			"close failed: %s (%d)\n", db_strerror(rc2), rc2, 0 );
		return rc2;
	}

	if( rc == DB_NOTFOUND ) {
		return rc;

	} else if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
			"get failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
		return rc;

	} else if ( data.size == 0 || data.size % sizeof( ID ) ) {
		/* size not multiple of ID size */
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
			"odd size: expected %ld multiple, got %ld\n",
			(long) sizeof( ID ), (long) data.size, 0 );
		return -1;

	} else if ( data.size != BDB_IDL_SIZEOF(ids) ) {
		/* size mismatch */
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_fetch_key: "
			"get size mismatch: expected %ld, got %ld\n",
			(long) ((1 + ids[0]) * sizeof( ID )), (long) data.size, 0 );
		return -1;
	}

	if ( bdb->bi_idl_cache_max_size ) {
		bdb_idl_cache_put( bdb, db, key, ids, rc );
	}

	return rc;
}


int
bdb_idl_insert_key(
	BackendDB	*be,
	DB			*db,
	DB_TXN		*tid,
	DBT			*key,
	ID			id )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	int	rc;
	DBT data;
	DBC *cursor;
	ID lo, hi, nlo, nhi, nid;
	char *err;

	{
		char buf[16];
		Debug( LDAP_DEBUG_ARGS,
			"bdb_idl_insert_key: %lx %s\n", 
			(long) id, bdb_show_key( key, buf ), 0 );
	}

	assert( id != NOID );

	DBTzero( &data );
	data.size = sizeof( ID );
	data.ulen = data.size;
	data.flags = DB_DBT_USERMEM;

	BDB_ID2DISK( id, &nid );

	rc = db->cursor( db, tid, &cursor, bdb->bi_db_opflags );
	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_insert_key: "
			"cursor failed: %s (%d)\n", db_strerror(rc), rc, 0 );
		return rc;
	}
	data.data = &nlo;
	/* Fetch the first data item for this key, to see if it
	 * exists and if it's a range.
	 */
	rc = cursor->c_get( cursor, key, &data, DB_SET );
	err = "c_get";
	if ( rc == 0 ) {
		if ( nlo != 0 ) {
			/* not a range, count the number of items */
			db_recno_t count;
			rc = cursor->c_count( cursor, &count, 0 );
			if ( rc != 0 ) {
				err = "c_count";
				goto fail;
			}
			if ( count >= BDB_IDL_DB_MAX ) {
			/* No room, convert to a range */
				DBT key2 = *key;
				db_recno_t i;

				key2.dlen = key2.ulen;
				key2.flags |= DB_DBT_PARTIAL;

				BDB_DISK2ID( &nlo, &lo );
				data.data = &nhi;

				rc = cursor->c_get( cursor, &key2, &data, DB_NEXT_NODUP );
				if ( rc != 0 && rc != DB_NOTFOUND ) {
					err = "c_get next_nodup";
					goto fail;
				}
				if ( rc == DB_NOTFOUND ) {
					rc = cursor->c_get( cursor, key, &data, DB_LAST );
					if ( rc != 0 ) {
						err = "c_get last";
						goto fail;
					}
				} else {
					rc = cursor->c_get( cursor, key, &data, DB_PREV );
					if ( rc != 0 ) {
						err = "c_get prev";
						goto fail;
					}
				}
				BDB_DISK2ID( &nhi, &hi );
				/* Update hi/lo if needed, then delete all the items
				 * between lo and hi
				 */
				if ( id < lo ) {
					lo = id;
					nlo = nid;
				} else if ( id > hi ) {
					hi = id;
					nhi = nid;
				}
				data.data = &nid;
				/* Don't fetch anything, just position cursor */
				data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
				data.dlen = data.ulen = 0;
				rc = cursor->c_get( cursor, key, &data, DB_SET );
				if ( rc != 0 ) {
					err = "c_get 2";
					goto fail;
				}
				rc = cursor->c_del( cursor, 0 );
				if ( rc != 0 ) {
					err = "c_del range1";
					goto fail;
				}
				/* Delete all the records */
				for ( i=1; i<count; i++ ) {
					rc = cursor->c_get( cursor, &key2, &data, DB_NEXT_DUP );
					if ( rc != 0 ) {
						err = "c_get next_dup";
						goto fail;
					}
					rc = cursor->c_del( cursor, 0 );
					if ( rc != 0 ) {
						err = "c_del range";
						goto fail;
					}
				}
				/* Store the range marker */
				data.size = data.ulen = sizeof(ID);
				data.flags = DB_DBT_USERMEM;
				nid = 0;
				rc = cursor->c_put( cursor, key, &data, DB_KEYFIRST );
				if ( rc != 0 ) {
					err = "c_put range";
					goto fail;
				}
				nid = nlo;
				rc = cursor->c_put( cursor, key, &data, DB_KEYLAST );
				if ( rc != 0 ) {
					err = "c_put lo";
					goto fail;
				}
				nid = nhi;
				rc = cursor->c_put( cursor, key, &data, DB_KEYLAST );
				if ( rc != 0 ) {
					err = "c_put hi";
					goto fail;
				}
			} else {
			/* There's room, just store it */
				goto put1;
			}
		} else {
			/* It's a range, see if we need to rewrite
			 * the boundaries
			 */
			hi = id;
			data.data = &nlo;
			rc = cursor->c_get( cursor, key, &data, DB_NEXT_DUP );
			if ( rc != 0 ) {
				err = "c_get lo";
				goto fail;
			}
			BDB_DISK2ID( &nlo, &lo );
			if ( id > lo ) {
				data.data = &nhi;
				rc = cursor->c_get( cursor, key, &data, DB_NEXT_DUP );
				if ( rc != 0 ) {
					err = "c_get hi";
					goto fail;
				}
				BDB_DISK2ID( &nhi, &hi );
			}
			if ( id < lo || id > hi ) {
				/* Delete the current lo/hi */
				rc = cursor->c_del( cursor, 0 );
				if ( rc != 0 ) {
					err = "c_del";
					goto fail;
				}
				data.data = &nid;
				rc = cursor->c_put( cursor, key, &data, DB_KEYFIRST );
				if ( rc != 0 ) {
					err = "c_put lo/hi";
					goto fail;
				}
			}
		}
	} else if ( rc == DB_NOTFOUND ) {
put1:		data.data = &nid;
		rc = cursor->c_put( cursor, key, &data, DB_NODUPDATA );
		/* Don't worry if it's already there */
		if ( rc != 0 && rc != DB_KEYEXIST ) {
			err = "c_put id";
			goto fail;
		}
	} else {
		/* initial c_get failed, nothing was done */
fail:
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_insert_key: "
			"%s failed: %s (%d)\n", err, db_strerror(rc), rc );
		cursor->c_close( cursor );
		return rc;
	}
	/* If key was added (didn't already exist) and using IDL cache,
	 * update key in IDL cache.
	 */
	if ( !rc && bdb->bi_idl_cache_max_size ) {
		bdb_idl_cache_add_id( bdb, db, key, id );
	}
	rc = cursor->c_close( cursor );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_insert_key: "
			"c_close failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
	}
	return rc;
}

int
bdb_idl_delete_key(
	BackendDB	*be,
	DB			*db,
	DB_TXN		*tid,
	DBT			*key,
	ID			id )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	int	rc;
	DBT data;
	DBC *cursor;
	ID lo, hi, tmp, nid, nlo, nhi;
	char *err;

	{
		char buf[16];
		Debug( LDAP_DEBUG_ARGS,
			"bdb_idl_delete_key: %lx %s\n", 
			(long) id, bdb_show_key( key, buf ), 0 );
	}
	assert( id != NOID );

	if ( bdb->bi_idl_cache_size ) {
		bdb_idl_cache_del( bdb, db, key );
	}

	BDB_ID2DISK( id, &nid );

	DBTzero( &data );
	data.data = &tmp;
	data.size = sizeof( id );
	data.ulen = data.size;
	data.flags = DB_DBT_USERMEM;

	rc = db->cursor( db, tid, &cursor, bdb->bi_db_opflags );
	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_delete_key: "
			"cursor failed: %s (%d)\n", db_strerror(rc), rc, 0 );
		return rc;
	}
	/* Fetch the first data item for this key, to see if it
	 * exists and if it's a range.
	 */
	rc = cursor->c_get( cursor, key, &data, DB_SET );
	err = "c_get";
	if ( rc == 0 ) {
		if ( tmp != 0 ) {
			/* Not a range, just delete it */
			if (tmp != nid) {
				/* position to correct item */
				tmp = nid;
				rc = cursor->c_get( cursor, key, &data, DB_GET_BOTH );
				if ( rc != 0 ) {
					err = "c_get id";
					goto fail;
				}
			}
			rc = cursor->c_del( cursor, 0 );
			if ( rc != 0 ) {
				err = "c_del id";
				goto fail;
			}
		} else {
			/* It's a range, see if we need to rewrite
			 * the boundaries
			 */
			data.data = &nlo;
			rc = cursor->c_get( cursor, key, &data, DB_NEXT_DUP );
			if ( rc != 0 ) {
				err = "c_get lo";
				goto fail;
			}
			BDB_DISK2ID( &nlo, &lo );
			data.data = &nhi;
			rc = cursor->c_get( cursor, key, &data, DB_NEXT_DUP );
			if ( rc != 0 ) {
				err = "c_get hi";
				goto fail;
			}
			BDB_DISK2ID( &nhi, &hi );
			if ( id == lo || id == hi ) {
				if ( id == lo ) {
					id++;
					lo = id;
				} else if ( id == hi ) {
					id--;
					hi = id;
				}
				if ( lo >= hi ) {
				/* The range has collapsed... */
					rc = db->del( db, tid, key, 0 );
					if ( rc != 0 ) {
						err = "del";
						goto fail;
					}
				} else {
					if ( id == lo ) {
						/* reposition on lo slot */
						data.data = &nlo;
						cursor->c_get( cursor, key, &data, DB_PREV );
					}
					rc = cursor->c_del( cursor, 0 );
					if ( rc != 0 ) {
						err = "c_del";
						goto fail;
					}
				}
				if ( lo <= hi ) {
					BDB_ID2DISK( id, &nid );
					data.data = &nid;
					rc = cursor->c_put( cursor, key, &data, DB_KEYFIRST );
					if ( rc != 0 ) {
						err = "c_put lo/hi";
						goto fail;
					}
				}
			}
		}
	} else {
		/* initial c_get failed, nothing was done */
fail:
		if ( rc != DB_NOTFOUND ) {
		Debug( LDAP_DEBUG_ANY, "=> bdb_idl_delete_key: "
			"%s failed: %s (%d)\n", err, db_strerror(rc), rc );
		}
		cursor->c_close( cursor );
		return rc;
	}
	rc = cursor->c_close( cursor );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"=> bdb_idl_delete_key: c_close failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
	}

	return rc;
}


/*
 * idl_intersection - return a = a intersection b
 */
int
bdb_idl_intersection(
	ID *a,
	ID *b )
{
	ID ida, idb;
	ID idmax, idmin;
	ID cursora = 0, cursorb = 0, cursorc;
	int swap = 0;

	if ( BDB_IDL_IS_ZERO( a ) || BDB_IDL_IS_ZERO( b ) ) {
		a[0] = 0;
		return 0;
	}

	idmin = IDL_MAX( BDB_IDL_FIRST(a), BDB_IDL_FIRST(b) );
	idmax = IDL_MIN( BDB_IDL_LAST(a), BDB_IDL_LAST(b) );
	if ( idmin > idmax ) {
		a[0] = 0;
		return 0;
	} else if ( idmin == idmax ) {
		a[0] = 1;
		a[1] = idmin;
		return 0;
	}

	if ( BDB_IDL_IS_RANGE( a ) ) {
		if ( BDB_IDL_IS_RANGE(b) ) {
		/* If both are ranges, just shrink the boundaries */
			a[1] = idmin;
			a[2] = idmax;
			return 0;
		} else {
		/* Else swap so that b is the range, a is a list */
			ID *tmp = a;
			a = b;
			b = tmp;
			swap = 1;
		}
	}

	/* If a range completely covers the list, the result is
	 * just the list. If idmin to idmax is contiguous, just
	 * turn it into a range.
	 */
	if ( BDB_IDL_IS_RANGE( b )
		&& BDB_IDL_RANGE_FIRST( b ) <= BDB_IDL_FIRST( a )
		&& BDB_IDL_RANGE_LAST( b ) >= BDB_IDL_LLAST( a ) ) {
		if (idmax - idmin + 1 == a[0])
		{
			a[0] = NOID;
			a[1] = idmin;
			a[2] = idmax;
		}
		goto done;
	}

	/* Fine, do the intersection one element at a time.
	 * First advance to idmin in both IDLs.
	 */
	cursora = cursorb = idmin;
	ida = bdb_idl_first( a, &cursora );
	idb = bdb_idl_first( b, &cursorb );
	cursorc = 0;

	while( ida <= idmax || idb <= idmax ) {
		if( ida == idb ) {
			a[++cursorc] = ida;
			ida = bdb_idl_next( a, &cursora );
			idb = bdb_idl_next( b, &cursorb );
		} else if ( ida < idb ) {
			ida = bdb_idl_next( a, &cursora );
		} else {
			idb = bdb_idl_next( b, &cursorb );
		}
	}
	a[0] = cursorc;
done:
	if (swap)
		BDB_IDL_CPY( b, a );

	return 0;
}


/*
 * idl_union - return a = a union b
 */
int
bdb_idl_union(
	ID	*a,
	ID	*b )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0, cursorc;

	if ( BDB_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( BDB_IDL_IS_ZERO( a ) ) {
		BDB_IDL_CPY( a, b );
		return 0;
	}

	if ( BDB_IDL_IS_RANGE( a ) || BDB_IDL_IS_RANGE(b) ) {
over:		ida = IDL_MIN( BDB_IDL_FIRST(a), BDB_IDL_FIRST(b) );
		idb = IDL_MAX( BDB_IDL_LAST(a), BDB_IDL_LAST(b) );
		a[0] = NOID;
		a[1] = ida;
		a[2] = idb;
		return 0;
	}

	ida = bdb_idl_first( a, &cursora );
	idb = bdb_idl_first( b, &cursorb );

	cursorc = b[0];

	/* The distinct elements of a are cat'd to b */
	while( ida != NOID || idb != NOID ) {
		if ( ida < idb ) {
			if( ++cursorc > BDB_IDL_UM_MAX ) {
				goto over;
			}
			b[cursorc] = ida;
			ida = bdb_idl_next( a, &cursora );

		} else {
			if ( ida == idb )
				ida = bdb_idl_next( a, &cursora );
			idb = bdb_idl_next( b, &cursorb );
		}
	}

	/* b is copied back to a in sorted order */
	a[0] = cursorc;
	cursora = 1;
	cursorb = 1;
	cursorc = b[0]+1;
	while (cursorb <= b[0] || cursorc <= a[0]) {
		if (cursorc > a[0])
			idb = NOID;
		else
			idb = b[cursorc];
		if (cursorb <= b[0] && b[cursorb] < idb)
			a[cursora++] = b[cursorb++];
		else {
			a[cursora++] = idb;
			cursorc++;
		}
	}

	return 0;
}


#if 0
/*
 * bdb_idl_notin - return a intersection ~b (or a minus b)
 */
int
bdb_idl_notin(
	ID	*a,
	ID	*b,
	ID *ids )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0;

	if( BDB_IDL_IS_ZERO( a ) ||
		BDB_IDL_IS_ZERO( b ) ||
		BDB_IDL_IS_RANGE( b ) )
	{
		BDB_IDL_CPY( ids, a );
		return 0;
	}

	if( BDB_IDL_IS_RANGE( a ) ) {
		BDB_IDL_CPY( ids, a );
		return 0;
	}

	ida = bdb_idl_first( a, &cursora ),
	idb = bdb_idl_first( b, &cursorb );

	ids[0] = 0;

	while( ida != NOID ) {
		if ( idb == NOID ) {
			/* we could shortcut this */
			ids[++ids[0]] = ida;
			ida = bdb_idl_next( a, &cursora );

		} else if ( ida < idb ) {
			ids[++ids[0]] = ida;
			ida = bdb_idl_next( a, &cursora );

		} else if ( ida > idb ) {
			idb = bdb_idl_next( b, &cursorb );

		} else {
			ida = bdb_idl_next( a, &cursora );
			idb = bdb_idl_next( b, &cursorb );
		}
	}

	return 0;
}
#endif

ID bdb_idl_first( ID *ids, ID *cursor )
{
	ID pos;

	if ( ids[0] == 0 ) {
		*cursor = NOID;
		return NOID;
	}

	if ( BDB_IDL_IS_RANGE( ids ) ) {
		if( *cursor < ids[1] ) {
			*cursor = ids[1];
		}
		return *cursor;
	}

	if ( *cursor == 0 )
		pos = 1;
	else
		pos = bdb_idl_search( ids, *cursor );

	if( pos > ids[0] ) {
		return NOID;
	}

	*cursor = pos;
	return ids[pos];
}

ID bdb_idl_next( ID *ids, ID *cursor )
{
	if ( BDB_IDL_IS_RANGE( ids ) ) {
		if( ids[2] < ++(*cursor) ) {
			return NOID;
		}
		return *cursor;
	}

	if ( ++(*cursor) <= ids[0] ) {
		return ids[*cursor];
	}

	return NOID;
}

#ifdef BDB_HIER

/* Add one ID to an unsorted list. We ensure that the first element is the
 * minimum and the last element is the maximum, for fast range compaction.
 *   this means IDLs up to length 3 are always sorted...
 */
int bdb_idl_append_one( ID *ids, ID id )
{
	if (BDB_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= BDB_IDL_RANGE_FIRST(ids) && id <= BDB_IDL_RANGE_LAST(ids))
			return -1;
		if (id < BDB_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > BDB_IDL_RANGE_LAST(ids))
			ids[2] = id;
		return 0;
	}
	if ( ids[0] ) {
		ID tmp;

		if (id < ids[1]) {
			tmp = ids[1];
			ids[1] = id;
			id = tmp;
		}
		if ( ids[0] > 1 && id < ids[ids[0]] ) {
			tmp = ids[ids[0]];
			ids[ids[0]] = id;
			id = tmp;
		}
	}
	ids[0]++;
	if ( ids[0] >= BDB_IDL_UM_MAX ) {
		ids[0] = NOID;
		ids[2] = id;
	} else {
		ids[ids[0]] = id;
	}
	return 0;
}

/* Append sorted list b to sorted list a. The result is unsorted but
 * a[1] is the min of the result and a[a[0]] is the max.
 */
int bdb_idl_append( ID *a, ID *b )
{
	ID ida, idb, tmp, swap = 0;

	if ( BDB_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( BDB_IDL_IS_ZERO( a ) ) {
		BDB_IDL_CPY( a, b );
		return 0;
	}

	if ( b[0] == 1 ) {
		return bdb_idl_append_one( a, BDB_IDL_FIRST( b ));
	}

	ida = BDB_IDL_LAST( a );
	idb = BDB_IDL_LAST( b );
	if ( BDB_IDL_IS_RANGE( a ) || BDB_IDL_IS_RANGE(b) ||
		a[0] + b[0] >= BDB_IDL_UM_MAX ) {
		a[2] = IDL_MAX( ida, idb );
		a[1] = IDL_MIN( a[1], b[1] );
		a[0] = NOID;
		return 0;
	}

	if ( ida > idb ) {
		swap = idb;
		a[a[0]] = idb;
		b[b[0]] = ida;
	}

	if ( b[1] < a[1] ) {
		tmp = a[1];
		a[1] = b[1];
	} else {
		tmp = b[1];
	}
	a[0]++;
	a[a[0]] = tmp;

	{
		int i = b[0] - 1;
		AC_MEMCPY(a+a[0]+1, b+2, i * sizeof(ID));
		a[0] += i;
	}
	if ( swap ) {
		b[b[0]] = swap;
	}
	return 0;
}

#if 1

/* Quicksort + Insertion sort for small arrays */

#define SMALL	8
#define	SWAP(a,b)	itmp=(a);(a)=(b);(b)=itmp

void
bdb_idl_sort( ID *ids, ID *tmp )
{
	int *istack = (int *)tmp;
	int i,j,k,l,ir,jstack;
	ID a, itmp;

	if ( BDB_IDL_IS_RANGE( ids ))
		return;

	ir = ids[0];
	l = 1;
	jstack = 0;
	for(;;) {
		if (ir - l < SMALL) {	/* Insertion sort */
			for (j=l+1;j<=ir;j++) {
				a = ids[j];
				for (i=j-1;i>=1;i--) {
					if (ids[i] <= a) break;
					ids[i+1] = ids[i];
				}
				ids[i+1] = a;
			}
			if (jstack == 0) break;
			ir = istack[jstack--];
			l = istack[jstack--];
		} else {
			k = (l + ir) >> 1;	/* Choose median of left, center, right */
			SWAP(ids[k], ids[l+1]);
			if (ids[l] > ids[ir]) {
				SWAP(ids[l], ids[ir]);
			}
			if (ids[l+1] > ids[ir]) {
				SWAP(ids[l+1], ids[ir]);
			}
			if (ids[l] > ids[l+1]) {
				SWAP(ids[l], ids[l+1]);
			}
			i = l+1;
			j = ir;
			a = ids[l+1];
			for(;;) {
				do i++; while(ids[i] < a);
				do j--; while(ids[j] > a);
				if (j < i) break;
				SWAP(ids[i],ids[j]);
			}
			ids[l+1] = ids[j];
			ids[j] = a;
			jstack += 2;
			if (ir-i+1 >= j-l) {
				istack[jstack] = ir;
				istack[jstack-1] = i;
				ir = j-1;
			} else {
				istack[jstack] = j-1;
				istack[jstack-1] = l;
				l = i;
			}
		}
	}
}

#else

/* 8 bit Radix sort + insertion sort
 * 
 * based on code from http://www.cubic.org/docs/radix.htm
 * with improvements by ebackes@symas.com and hyc@symas.com
 *
 * This code is O(n) but has a relatively high constant factor. For lists
 * up to ~50 Quicksort is slightly faster; up to ~100 they are even.
 * Much faster than quicksort for lists longer than ~100. Insertion
 * sort is actually superior for lists <50.
 */

#define BUCKETS	(1<<8)
#define SMALL	50

void
bdb_idl_sort( ID *ids, ID *tmp )
{
	int count, soft_limit, phase = 0, size = ids[0];
	ID *idls[2];
	unsigned char *maxv = (unsigned char *)&ids[size];

 	if ( BDB_IDL_IS_RANGE( ids ))
 		return;

	/* Use insertion sort for small lists */
	if ( size <= SMALL ) {
		int i,j;
		ID a;

		for (j=1;j<=size;j++) {
			a = ids[j];
			for (i=j-1;i>=1;i--) {
				if (ids[i] <= a) break;
				ids[i+1] = ids[i];
			}
			ids[i+1] = a;
		}
		return;
	}

	tmp[0] = size;
	idls[0] = ids;
	idls[1] = tmp;

#if BYTE_ORDER == BIG_ENDIAN
    for (soft_limit = 0; !maxv[soft_limit]; soft_limit++);
#else
    for (soft_limit = sizeof(ID)-1; !maxv[soft_limit]; soft_limit--);
#endif

	for (
#if BYTE_ORDER == BIG_ENDIAN
	count = sizeof(ID)-1; count >= soft_limit; --count
#else
	count = 0; count <= soft_limit; ++count
#endif
	) {
		unsigned int num[BUCKETS], * np, n, sum;
		int i;
        ID *sp, *source, *dest;
        unsigned char *bp, *source_start;

		source = idls[phase]+1;
		dest = idls[phase^1]+1;
		source_start =  ((unsigned char *) source) + count;

        np = num;
        for ( i = BUCKETS; i > 0; --i ) *np++ = 0;

		/* count occurences of every byte value */
		bp = source_start;
        for ( i = size; i > 0; --i, bp += sizeof(ID) )
				num[*bp]++;

		/* transform count into index by summing elements and storing
		 * into same array
		 */
        sum = 0;
        np = num;
        for ( i = BUCKETS; i > 0; --i ) {
                n = *np;
                *np++ = sum;
                sum += n;
        }

		/* fill dest with the right values in the right place */
		bp = source_start;
        sp = source;
        for ( i = size; i > 0; --i, bp += sizeof(ID) ) {
                np = num + *bp;
                dest[*np] = *sp++;
                ++(*np);
        }
		phase ^= 1;
	}

	/* copy back from temp if needed */
	if ( phase ) {
		ids++; tmp++;
		for ( count = 0; count < size; ++count ) 
			*ids++ = *tmp++;
	}
}
#endif	/* Quick vs Radix */

#endif	/* BDB_HIER */
