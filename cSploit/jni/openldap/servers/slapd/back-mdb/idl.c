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

#include "back-mdb.h"
#include "idl.h"

#define IDL_MAX(x,y)	( (x) > (y) ? (x) : (y) )
#define IDL_MIN(x,y)	( (x) < (y) ? (x) : (y) )
#define IDL_CMP(x,y)	( (x) < (y) ? -1 : (x) > (y) )

#if IDL_DEBUG > 0
static void idl_check( ID *ids )
{
	if( MDB_IDL_IS_RANGE( ids ) ) {
		assert( MDB_IDL_RANGE_FIRST(ids) <= MDB_IDL_RANGE_LAST(ids) );
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
	if( MDB_IDL_IS_RANGE( ids ) ) {
		Debug( LDAP_DEBUG_ANY,
			"IDL: range ( %ld - %ld )\n",
			(long) MDB_IDL_RANGE_FIRST( ids ),
			(long) MDB_IDL_RANGE_LAST( ids ) );

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

unsigned mdb_idl_search( ID *ids, ID id )
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

int mdb_idl_insert( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "insert: %04lx at %d\n", (long) id, x, 0 );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (MDB_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= MDB_IDL_RANGE_FIRST(ids) && id <= MDB_IDL_RANGE_LAST(ids))
			return -1;
		if (id < MDB_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > MDB_IDL_RANGE_LAST(ids))
			ids[2] = id;
		return 0;
	}

	x = mdb_idl_search( ids, id );
	assert( x > 0 );

	if( x < 1 ) {
		/* internal error */
		return -2;
	}

	if ( x <= ids[0] && ids[x] == id ) {
		/* duplicate */
		return -1;
	}

	if ( ++ids[0] >= MDB_IDL_DB_MAX ) {
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

static int mdb_idl_delete( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "delete: %04lx at %d\n", (long) id, x, 0 );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (MDB_IDL_IS_RANGE( ids )) {
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

	x = mdb_idl_search( ids, id );
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
mdb_show_key(
	char		*buf,
	void		*val,
	size_t		len )
{
	if ( len == 4 /* LUTIL_HASH_BYTES */ ) {
		unsigned char *c = val;
		sprintf( buf, "[%02x%02x%02x%02x]", c[0], c[1], c[2], c[3] );
		return buf;
	} else {
		return val;
	}
}

int
mdb_idl_fetch_key(
	BackendDB	*be,
	MDB_txn		*txn,
	MDB_dbi		dbi,
	MDB_val		*key,
	ID			*ids,
	MDB_cursor	**saved_cursor,
	int			get_flag )
{
	MDB_val data, key2, *kptr;
	MDB_cursor *cursor;
	ID *i;
	size_t len;
	int rc;
	MDB_cursor_op opflag;

	char keybuf[16];

	Debug( LDAP_DEBUG_ARGS,
		"mdb_idl_fetch_key: %s\n", 
		mdb_show_key( keybuf, key->mv_data, key->mv_size ), 0, 0 );

	assert( ids != NULL );

	if ( saved_cursor && *saved_cursor ) {
		opflag = MDB_NEXT;
	} else if ( get_flag == LDAP_FILTER_GE ) {
		opflag = MDB_SET_RANGE;
	} else if ( get_flag == LDAP_FILTER_LE ) {
		opflag = MDB_FIRST;
	} else {
		opflag = MDB_SET;
	}

	/* If we're not reusing an existing cursor, get a new one */
	if( opflag != MDB_NEXT ) {
		rc = mdb_cursor_open( txn, dbi, &cursor );
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY, "=> mdb_idl_fetch_key: "
				"cursor failed: %s (%d)\n", mdb_strerror(rc), rc, 0 );
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
		key2.mv_data = keybuf;
		key2.mv_size = key->mv_size;
		AC_MEMCPY( keybuf, key->mv_data, key->mv_size );
		kptr = &key2;
	} else {
		kptr = key;
	}
	len = key->mv_size;
	rc = mdb_cursor_get( cursor, kptr, &data, opflag );

	/* skip presence key on range inequality lookups */
	while (rc == 0 && kptr->mv_size != len) {
		rc = mdb_cursor_get( cursor, kptr, &data, MDB_NEXT_NODUP );
	}
	/* If we're doing a LE compare and the new key is greater than
	 * our search key, we're done
	 */
	if (rc == 0 && get_flag == LDAP_FILTER_LE && memcmp( kptr->mv_data,
		key->mv_data, key->mv_size ) > 0 ) {
		rc = MDB_NOTFOUND;
	}
	if (rc == 0) {
		i = ids+1;
		rc = mdb_cursor_get( cursor, key, &data, MDB_GET_MULTIPLE );
		while (rc == 0) {
			memcpy( i, data.mv_data, data.mv_size );
			i += data.mv_size / sizeof(ID);
			rc = mdb_cursor_get( cursor, key, &data, MDB_NEXT_MULTIPLE );
		}
		if ( rc == MDB_NOTFOUND ) rc = 0;
		ids[0] = i - &ids[1];
		/* On disk, a range is denoted by 0 in the first element */
		if (ids[1] == 0) {
			if (ids[0] != MDB_IDL_RANGE_SIZE) {
				Debug( LDAP_DEBUG_ANY, "=> mdb_idl_fetch_key: "
					"range size mismatch: expected %d, got %ld\n",
					MDB_IDL_RANGE_SIZE, ids[0], 0 );
				mdb_cursor_close( cursor );
				return -1;
			}
			MDB_IDL_RANGE( ids, ids[2], ids[3] );
		}
		data.mv_size = MDB_IDL_SIZEOF(ids);
	}

	if ( saved_cursor && rc == 0 ) {
		if ( !*saved_cursor )
			*saved_cursor = cursor;
	}
	else
		mdb_cursor_close( cursor );

	if( rc == MDB_NOTFOUND ) {
		return rc;

	} else if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "=> mdb_idl_fetch_key: "
			"get failed: %s (%d)\n",
			mdb_strerror(rc), rc, 0 );
		return rc;

	} else if ( data.mv_size == 0 || data.mv_size % sizeof( ID ) ) {
		/* size not multiple of ID size */
		Debug( LDAP_DEBUG_ANY, "=> mdb_idl_fetch_key: "
			"odd size: expected %ld multiple, got %ld\n",
			(long) sizeof( ID ), (long) data.mv_size, 0 );
		return -1;

	} else if ( data.mv_size != MDB_IDL_SIZEOF(ids) ) {
		/* size mismatch */
		Debug( LDAP_DEBUG_ANY, "=> mdb_idl_fetch_key: "
			"get size mismatch: expected %ld, got %ld\n",
			(long) ((1 + ids[0]) * sizeof( ID )), (long) data.mv_size, 0 );
		return -1;
	}

	return rc;
}

int
mdb_idl_insert_keys(
	BackendDB	*be,
	MDB_cursor	*cursor,
	struct berval *keys,
	ID			id )
{
	struct mdb_info *mdb = be->be_private;
	MDB_val key, data;
	ID lo, hi, *i;
	char *err;
	int	rc = 0, k;
	unsigned int flag = MDB_NODUPDATA;
#ifndef	MISALIGNED_OK
	int kbuf[2];
#endif

	{
		char buf[16];
		Debug( LDAP_DEBUG_ARGS,
			"mdb_idl_insert_keys: %lx %s\n", 
			(long) id, mdb_show_key( buf, keys->bv_val, keys->bv_len ), 0 );
	}

	assert( id != NOID );

#ifndef MISALIGNED_OK
	if (keys[0].bv_len & ALIGNER)
		kbuf[1] = 0;
#endif
	for ( k=0; keys[k].bv_val; k++ ) {
	/* Fetch the first data item for this key, to see if it
	 * exists and if it's a range.
	 */
#ifndef MISALIGNED_OK
	if (keys[k].bv_len & ALIGNER) {
		key.mv_size = sizeof(kbuf);
		key.mv_data = kbuf;
		memcpy(key.mv_data, keys[k].bv_val, keys[k].bv_len);
	} else
#endif
	{
		key.mv_size = keys[k].bv_len;
		key.mv_data = keys[k].bv_val;
	}
	rc = mdb_cursor_get( cursor, &key, &data, MDB_SET );
	err = "c_get";
	if ( rc == 0 ) {
		i = data.mv_data;
		memcpy(&lo, data.mv_data, sizeof(ID));
		if ( lo != 0 ) {
			/* not a range, count the number of items */
			size_t count;
			rc = mdb_cursor_count( cursor, &count );
			if ( rc != 0 ) {
				err = "c_count";
				goto fail;
			}
			if ( count >= MDB_IDL_DB_MAX ) {
			/* No room, convert to a range */
				lo = *i;
				rc = mdb_cursor_get( cursor, &key, &data, MDB_LAST_DUP );
				if ( rc != 0 && rc != MDB_NOTFOUND ) {
					err = "c_get last_dup";
					goto fail;
				}
				i = data.mv_data;
				hi = *i;
				/* Update hi/lo if needed */
				if ( id < lo ) {
					lo = id;
				} else if ( id > hi ) {
					hi = id;
				}
				/* delete the old key */
				rc = mdb_cursor_del( cursor, MDB_NODUPDATA );
				if ( rc != 0 ) {
					err = "c_del dups";
					goto fail;
				}
				/* Store the range */
				data.mv_size = sizeof(ID);
				data.mv_data = &id;
				id = 0;
				rc = mdb_cursor_put( cursor, &key, &data, 0 );
				if ( rc != 0 ) {
					err = "c_put range";
					goto fail;
				}
				id = lo;
				rc = mdb_cursor_put( cursor, &key, &data, 0 );
				if ( rc != 0 ) {
					err = "c_put lo";
					goto fail;
				}
				id = hi;
				rc = mdb_cursor_put( cursor, &key, &data, 0 );
				if ( rc != 0 ) {
					err = "c_put hi";
					goto fail;
				}
			} else {
			/* There's room, just store it */
				if (id == mdb->mi_nextid)
					flag |= MDB_APPENDDUP;
				goto put1;
			}
		} else {
			/* It's a range, see if we need to rewrite
			 * the boundaries
			 */
			lo = i[1];
			hi = i[2];
			if ( id < lo || id > hi ) {
				/* position on lo */
				rc = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_DUP );
				if ( rc != 0 ) {
					err = "c_get lo";
					goto fail;
				}
				if ( id > hi ) {
					/* position on hi */
					rc = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_DUP );
					if ( rc != 0 ) {
						err = "c_get hi";
						goto fail;
					}
				}
				data.mv_size = sizeof(ID);
				data.mv_data = &id;
				/* Replace the current lo/hi */
				rc = mdb_cursor_put( cursor, &key, &data, MDB_CURRENT );
				if ( rc != 0 ) {
					err = "c_put lo/hi";
					goto fail;
				}
			}
		}
	} else if ( rc == MDB_NOTFOUND ) {
		flag &= ~MDB_APPENDDUP;
put1:	data.mv_data = &id;
		data.mv_size = sizeof(ID);
		rc = mdb_cursor_put( cursor, &key, &data, flag );
		/* Don't worry if it's already there */
		if ( rc == MDB_KEYEXIST )
			rc = 0;
		if ( rc ) {
			err = "c_put id";
			goto fail;
		}
	} else {
		/* initial c_get failed, nothing was done */
fail:
		Debug( LDAP_DEBUG_ANY, "=> mdb_idl_insert_keys: "
			"%s failed: %s (%d)\n", err, mdb_strerror(rc), rc );
		break;
	}
	}
	return rc;
}

int
mdb_idl_delete_keys(
	BackendDB	*be,
	MDB_cursor	*cursor,
	struct berval *keys,
	ID			id )
{
	int	rc = 0, k;
	MDB_val key, data;
	ID lo, hi, tmp, *i;
	char *err;
#ifndef	MISALIGNED_OK
	int kbuf[2];
#endif

	{
		char buf[16];
		Debug( LDAP_DEBUG_ARGS,
			"mdb_idl_delete_keys: %lx %s\n", 
			(long) id, mdb_show_key( buf, keys->bv_val, keys->bv_len ), 0 );
	}
	assert( id != NOID );

#ifndef MISALIGNED_OK
	if (keys[0].bv_len & ALIGNER)
		kbuf[1] = 0;
#endif
	for ( k=0; keys[k].bv_val; k++) {
	/* Fetch the first data item for this key, to see if it
	 * exists and if it's a range.
	 */
#ifndef MISALIGNED_OK
	if (keys[k].bv_len & ALIGNER) {
		key.mv_size = sizeof(kbuf);
		key.mv_data = kbuf;
		memcpy(key.mv_data, keys[k].bv_val, keys[k].bv_len);
	} else
#endif
	{
		key.mv_size = keys[k].bv_len;
		key.mv_data = keys[k].bv_val;
	}
	rc = mdb_cursor_get( cursor, &key, &data, MDB_SET );
	err = "c_get";
	if ( rc == 0 ) {
		memcpy( &tmp, data.mv_data, sizeof(ID) );
		i = data.mv_data;
		if ( tmp != 0 ) {
			/* Not a range, just delete it */
			data.mv_data = &id;
			rc = mdb_cursor_get( cursor, &key, &data, MDB_GET_BOTH );
			if ( rc != 0 ) {
				err = "c_get id";
				goto fail;
			}
			rc = mdb_cursor_del( cursor, 0 );
			if ( rc != 0 ) {
				err = "c_del id";
				goto fail;
			}
		} else {
			/* It's a range, see if we need to rewrite
			 * the boundaries
			 */
			lo = i[1];
			hi = i[2];
			if ( id == lo || id == hi ) {
				ID lo2 = lo, hi2 = hi;
				if ( id == lo ) {
					lo2++;
				} else if ( id == hi ) {
					hi2--;
				}
				if ( lo2 >= hi2 ) {
				/* The range has collapsed... */
					rc = mdb_cursor_del( cursor, MDB_NODUPDATA );
					if ( rc != 0 ) {
						err = "c_del dup";
						goto fail;
					}
				} else {
					/* position on lo */
					rc = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_DUP );
					if ( id == lo )
						data.mv_data = &lo2;
					else {
						/* position on hi */
						rc = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_DUP );
						data.mv_data = &hi2;
					}
					/* Replace the current lo/hi */
					data.mv_size = sizeof(ID);
					rc = mdb_cursor_put( cursor, &key, &data, MDB_CURRENT );
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
		if ( rc == MDB_NOTFOUND )
			rc = 0;
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY, "=> mdb_idl_delete_key: "
				"%s failed: %s (%d)\n", err, mdb_strerror(rc), rc );
			break;
		}
	}
	}
	return rc;
}


/*
 * idl_intersection - return a = a intersection b
 */
int
mdb_idl_intersection(
	ID *a,
	ID *b )
{
	ID ida, idb;
	ID idmax, idmin;
	ID cursora = 0, cursorb = 0, cursorc;
	int swap = 0;

	if ( MDB_IDL_IS_ZERO( a ) || MDB_IDL_IS_ZERO( b ) ) {
		a[0] = 0;
		return 0;
	}

	idmin = IDL_MAX( MDB_IDL_FIRST(a), MDB_IDL_FIRST(b) );
	idmax = IDL_MIN( MDB_IDL_LAST(a), MDB_IDL_LAST(b) );
	if ( idmin > idmax ) {
		a[0] = 0;
		return 0;
	} else if ( idmin == idmax ) {
		a[0] = 1;
		a[1] = idmin;
		return 0;
	}

	if ( MDB_IDL_IS_RANGE( a ) ) {
		if ( MDB_IDL_IS_RANGE(b) ) {
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
	if ( MDB_IDL_IS_RANGE( b )
		&& MDB_IDL_RANGE_FIRST( b ) <= MDB_IDL_FIRST( a )
		&& MDB_IDL_RANGE_LAST( b ) >= MDB_IDL_LLAST( a ) ) {
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
	ida = mdb_idl_first( a, &cursora );
	idb = mdb_idl_first( b, &cursorb );
	cursorc = 0;

	while( ida <= idmax || idb <= idmax ) {
		if( ida == idb ) {
			a[++cursorc] = ida;
			ida = mdb_idl_next( a, &cursora );
			idb = mdb_idl_next( b, &cursorb );
		} else if ( ida < idb ) {
			ida = mdb_idl_next( a, &cursora );
		} else {
			idb = mdb_idl_next( b, &cursorb );
		}
	}
	a[0] = cursorc;
done:
	if (swap)
		MDB_IDL_CPY( b, a );

	return 0;
}


/*
 * idl_union - return a = a union b
 */
int
mdb_idl_union(
	ID	*a,
	ID	*b )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0, cursorc;

	if ( MDB_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( MDB_IDL_IS_ZERO( a ) ) {
		MDB_IDL_CPY( a, b );
		return 0;
	}

	if ( MDB_IDL_IS_RANGE( a ) || MDB_IDL_IS_RANGE(b) ) {
over:		ida = IDL_MIN( MDB_IDL_FIRST(a), MDB_IDL_FIRST(b) );
		idb = IDL_MAX( MDB_IDL_LAST(a), MDB_IDL_LAST(b) );
		a[0] = NOID;
		a[1] = ida;
		a[2] = idb;
		return 0;
	}

	ida = mdb_idl_first( a, &cursora );
	idb = mdb_idl_first( b, &cursorb );

	cursorc = b[0];

	/* The distinct elements of a are cat'd to b */
	while( ida != NOID || idb != NOID ) {
		if ( ida < idb ) {
			if( ++cursorc > MDB_IDL_UM_MAX ) {
				goto over;
			}
			b[cursorc] = ida;
			ida = mdb_idl_next( a, &cursora );

		} else {
			if ( ida == idb )
				ida = mdb_idl_next( a, &cursora );
			idb = mdb_idl_next( b, &cursorb );
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
 * mdb_idl_notin - return a intersection ~b (or a minus b)
 */
int
mdb_idl_notin(
	ID	*a,
	ID	*b,
	ID *ids )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0;

	if( MDB_IDL_IS_ZERO( a ) ||
		MDB_IDL_IS_ZERO( b ) ||
		MDB_IDL_IS_RANGE( b ) )
	{
		MDB_IDL_CPY( ids, a );
		return 0;
	}

	if( MDB_IDL_IS_RANGE( a ) ) {
		MDB_IDL_CPY( ids, a );
		return 0;
	}

	ida = mdb_idl_first( a, &cursora ),
	idb = mdb_idl_first( b, &cursorb );

	ids[0] = 0;

	while( ida != NOID ) {
		if ( idb == NOID ) {
			/* we could shortcut this */
			ids[++ids[0]] = ida;
			ida = mdb_idl_next( a, &cursora );

		} else if ( ida < idb ) {
			ids[++ids[0]] = ida;
			ida = mdb_idl_next( a, &cursora );

		} else if ( ida > idb ) {
			idb = mdb_idl_next( b, &cursorb );

		} else {
			ida = mdb_idl_next( a, &cursora );
			idb = mdb_idl_next( b, &cursorb );
		}
	}

	return 0;
}
#endif

ID mdb_idl_first( ID *ids, ID *cursor )
{
	ID pos;

	if ( ids[0] == 0 ) {
		*cursor = NOID;
		return NOID;
	}

	if ( MDB_IDL_IS_RANGE( ids ) ) {
		if( *cursor < ids[1] ) {
			*cursor = ids[1];
		}
		return *cursor;
	}

	if ( *cursor == 0 )
		pos = 1;
	else
		pos = mdb_idl_search( ids, *cursor );

	if( pos > ids[0] ) {
		return NOID;
	}

	*cursor = pos;
	return ids[pos];
}

ID mdb_idl_next( ID *ids, ID *cursor )
{
	if ( MDB_IDL_IS_RANGE( ids ) ) {
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

/* Add one ID to an unsorted list. We ensure that the first element is the
 * minimum and the last element is the maximum, for fast range compaction.
 *   this means IDLs up to length 3 are always sorted...
 */
int mdb_idl_append_one( ID *ids, ID id )
{
	if (MDB_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= MDB_IDL_RANGE_FIRST(ids) && id <= MDB_IDL_RANGE_LAST(ids))
			return -1;
		if (id < MDB_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > MDB_IDL_RANGE_LAST(ids))
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
	if ( ids[0] >= MDB_IDL_UM_MAX ) {
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
int mdb_idl_append( ID *a, ID *b )
{
	ID ida, idb, tmp, swap = 0;

	if ( MDB_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( MDB_IDL_IS_ZERO( a ) ) {
		MDB_IDL_CPY( a, b );
		return 0;
	}

	ida = MDB_IDL_LAST( a );
	idb = MDB_IDL_LAST( b );
	if ( MDB_IDL_IS_RANGE( a ) || MDB_IDL_IS_RANGE(b) ||
		a[0] + b[0] >= MDB_IDL_UM_MAX ) {
		a[2] = IDL_MAX( ida, idb );
		a[1] = IDL_MIN( a[1], b[1] );
		a[0] = NOID;
		return 0;
	}

	if ( b[0] > 1 && ida > idb ) {
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

	if ( b[0] > 1 ) {
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
mdb_idl_sort( ID *ids, ID *tmp )
{
	int *istack = (int *)tmp; /* Private stack, not used by caller */
	int i,j,k,l,ir,jstack;
	ID a, itmp;

	if ( MDB_IDL_IS_RANGE( ids ))
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
mdb_idl_sort( ID *ids, ID *tmp )
{
	int count, soft_limit, phase = 0, size = ids[0];
	ID *idls[2];
	unsigned char *maxv = (unsigned char *)&ids[size];

 	if ( MDB_IDL_IS_RANGE( ids ))
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

unsigned mdb_id2l_search( ID2L ids, ID id )
{
	/*
	 * binary search of id in ids
	 * if found, returns position of id
	 * if not found, returns first position greater than id
	 */
	unsigned base = 0;
	unsigned cursor = 1;
	int val = 0;
	unsigned n = ids[0].mid;

	while( 0 < n ) {
		unsigned pivot = n >> 1;
		cursor = base + pivot + 1;
		val = IDL_CMP( id, ids[cursor].mid );

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
}

int mdb_id2l_insert( ID2L ids, ID2 *id )
{
	unsigned x, i;

	x = mdb_id2l_search( ids, id->mid );
	assert( x > 0 );

	if( x < 1 ) {
		/* internal error */
		return -2;
	}

	if ( x <= ids[0].mid && ids[x].mid == id->mid ) {
		/* duplicate */
		return -1;
	}

	if ( ids[0].mid >= MDB_IDL_UM_MAX ) {
		/* too big */
		return -2;

	} else {
		/* insert id */
		ids[0].mid++;
		for (i=ids[0].mid; i>x; i--)
			ids[i] = ids[i-1];
		ids[x] = *id;
	}

	return 0;
}
