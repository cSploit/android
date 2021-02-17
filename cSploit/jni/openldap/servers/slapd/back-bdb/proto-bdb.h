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

#ifndef _PROTO_BDB_H
#define _PROTO_BDB_H

LDAP_BEGIN_DECL

#ifdef BDB_HIER
#define	BDB_SYMBOL(x)	LDAP_CONCAT(hdb_,x)
#define BDB_UCTYPE	"HDB"
#else
#define BDB_SYMBOL(x)	LDAP_CONCAT(bdb_,x)
#define BDB_UCTYPE	"BDB"
#endif

/*
 * attr.c
 */

#define bdb_attr_mask				BDB_SYMBOL(attr_mask)
#define bdb_attr_flush				BDB_SYMBOL(attr_flush)
#define bdb_attr_slot				BDB_SYMBOL(attr_slot)
#define bdb_attr_index_config		BDB_SYMBOL(attr_index_config)
#define bdb_attr_index_destroy		BDB_SYMBOL(attr_index_destroy)
#define bdb_attr_index_free			BDB_SYMBOL(attr_index_free)
#define bdb_attr_index_unparse		BDB_SYMBOL(attr_index_unparse)
#define bdb_attr_info_free			BDB_SYMBOL(attr_info_free)

AttrInfo *bdb_attr_mask( struct bdb_info *bdb,
	AttributeDescription *desc );

void bdb_attr_flush( struct bdb_info *bdb );

int bdb_attr_slot( struct bdb_info *bdb,
	AttributeDescription *desc, int *insert );

int bdb_attr_index_config LDAP_P(( struct bdb_info *bdb,
	const char *fname, int lineno,
	int argc, char **argv, struct config_reply_s *cr ));

void bdb_attr_index_unparse LDAP_P(( struct bdb_info *bdb, BerVarray *bva ));
void bdb_attr_index_destroy LDAP_P(( struct bdb_info *bdb ));
void bdb_attr_index_free LDAP_P(( struct bdb_info *bdb,
	AttributeDescription *ad ));

void bdb_attr_info_free( AttrInfo *ai );

/*
 * config.c
 */

#define bdb_back_init_cf				BDB_SYMBOL(back_init_cf)

int bdb_back_init_cf( BackendInfo *bi );

/*
 * dbcache.c
 */
#define bdb_db_cache				BDB_SYMBOL(db_cache)
#define bdb_db_findsize				BDB_SYMBOL(db_findsize)

int
bdb_db_cache(
    Backend	*be,
    struct berval *name,
	DB **db );

int
bdb_db_findsize(
	struct bdb_info *bdb,
	struct berval *name );

/*
 * dn2entry.c
 */
#define bdb_dn2entry				BDB_SYMBOL(dn2entry)

int bdb_dn2entry LDAP_P(( Operation *op, DB_TXN *tid,
	struct berval *dn, EntryInfo **e, int matched,
	DB_LOCK *lock ));

/*
 * dn2id.c
 */
#define bdb_dn2id					BDB_SYMBOL(dn2id)
#define bdb_dn2id_add				BDB_SYMBOL(dn2id_add)
#define bdb_dn2id_delete			BDB_SYMBOL(dn2id_delete)
#define bdb_dn2id_children			BDB_SYMBOL(dn2id_children)
#define bdb_dn2idl					BDB_SYMBOL(dn2idl)

int bdb_dn2id(
	Operation *op,
	struct berval *dn,
	EntryInfo *ei,
	DB_TXN *txn,
	DBC **cursor );

int bdb_dn2id_add(
	Operation *op,
	DB_TXN *tid,
	EntryInfo *eip,
	Entry *e );

int bdb_dn2id_delete(
	Operation *op,
	DB_TXN *tid,
	EntryInfo *eip,
	Entry *e );

int bdb_dn2id_children(
	Operation *op,
	DB_TXN *tid,
	Entry *e );

int bdb_dn2idl(
	Operation *op,
	DB_TXN *txn,
	struct berval *ndn,
	EntryInfo *ei,
	ID *ids,
	ID *stack );

#ifdef BDB_HIER
#define bdb_dn2id_parent			BDB_SYMBOL(dn2id_parent)
#define bdb_dup_compare				BDB_SYMBOL(dup_compare)
#define bdb_fix_dn					BDB_SYMBOL(fix_dn)

int bdb_dn2id_parent(
	Operation *op,
	DB_TXN *txn,
	EntryInfo *ei,
	ID *idp );

int bdb_dup_compare(
	DB *db,
	const DBT *usrkey,
	const DBT *curkey );

int bdb_fix_dn( Entry *e, int checkit );
#endif


/*
 * error.c
 */
#define bdb_errcall					BDB_SYMBOL(errcall)

#if DB_VERSION_FULL < 0x04030000
void bdb_errcall( const char *pfx, char * msg );
#else
#define bdb_msgcall					BDB_SYMBOL(msgcall)
void bdb_errcall( const DB_ENV *env, const char *pfx, const char * msg );
void bdb_msgcall( const DB_ENV *env, const char * msg );
#endif

#ifdef HAVE_EBCDIC
#define ebcdic_dberror				BDB_SYMBOL(ebcdic_dberror)

char *ebcdic_dberror( int rc );
#define db_strerror(x)	ebcdic_dberror(x)
#endif

/*
 * filterentry.c
 */
#define bdb_filter_candidates		BDB_SYMBOL(filter_candidates)

int bdb_filter_candidates(
	Operation *op,
	DB_TXN *txn,
	Filter	*f,
	ID *ids,
	ID *tmp,
	ID *stack );

/*
 * id2entry.c
 */
#define bdb_id2entry				BDB_SYMBOL(id2entry)
#define bdb_id2entry_add			BDB_SYMBOL(id2entry_add)
#define bdb_id2entry_update			BDB_SYMBOL(id2entry_update)
#define bdb_id2entry_delete			BDB_SYMBOL(id2entry_delete)

int bdb_id2entry_add(
	BackendDB *be,
	DB_TXN *tid,
	Entry *e );

int bdb_id2entry_update(
	BackendDB *be,
	DB_TXN *tid,
	Entry *e );

int bdb_id2entry_delete(
	BackendDB *be,
	DB_TXN *tid,
	Entry *e);

#ifdef SLAP_ZONE_ALLOC
#else
int bdb_id2entry(
	BackendDB *be,
	DB_TXN *tid,
	ID id,
	Entry **e);
#endif

#define bdb_entry_free				BDB_SYMBOL(entry_free)
#define bdb_entry_return			BDB_SYMBOL(entry_return)
#define bdb_entry_release			BDB_SYMBOL(entry_release)
#define bdb_entry_get				BDB_SYMBOL(entry_get)

void bdb_entry_free ( Entry *e );
#ifdef SLAP_ZONE_ALLOC
int bdb_entry_return( struct bdb_info *bdb, Entry *e, int seqno );
#else
int bdb_entry_return( Entry *e );
#endif
BI_entry_release_rw bdb_entry_release;
BI_entry_get_rw bdb_entry_get;


/*
 * idl.c
 */

#define bdb_idl_cache_get			BDB_SYMBOL(idl_cache_get)
#define bdb_idl_cache_put			BDB_SYMBOL(idl_cache_put)
#define bdb_idl_cache_del			BDB_SYMBOL(idl_cache_del)
#define bdb_idl_cache_add_id		BDB_SYMBOL(idl_cache_add_id)
#define bdb_idl_cache_del_id		BDB_SYMBOL(idl_cache_del_id)

int bdb_idl_cache_get(
	struct bdb_info *bdb,
	DB *db,
	DBT *key,
	ID *ids );

void
bdb_idl_cache_put(
	struct bdb_info	*bdb,
	DB		*db,
	DBT		*key,
	ID		*ids,
	int		rc );

void
bdb_idl_cache_del(
	struct bdb_info	*bdb,
	DB		*db,
	DBT		*key );

void
bdb_idl_cache_add_id(
	struct bdb_info	*bdb,
	DB		*db,
	DBT		*key,
	ID		id );

void
bdb_idl_cache_del_id(
	struct bdb_info	*bdb,
	DB		*db,
	DBT		*key,
	ID		id );

#define bdb_idl_first				BDB_SYMBOL(idl_first)
#define bdb_idl_next				BDB_SYMBOL(idl_next)
#define bdb_idl_search				BDB_SYMBOL(idl_search)
#define bdb_idl_insert				BDB_SYMBOL(idl_insert)
#define bdb_idl_delete				BDB_SYMBOL(idl_delete)
#define bdb_idl_intersection		BDB_SYMBOL(idl_intersection)
#define bdb_idl_union				BDB_SYMBOL(idl_union)
#define bdb_idl_sort				BDB_SYMBOL(idl_sort)
#define bdb_idl_append				BDB_SYMBOL(idl_append)
#define bdb_idl_append_one			BDB_SYMBOL(idl_append_one)

#define bdb_idl_fetch_key			BDB_SYMBOL(idl_fetch_key)
#define bdb_idl_insert_key			BDB_SYMBOL(idl_insert_key)
#define bdb_idl_delete_key			BDB_SYMBOL(idl_delete_key)

unsigned bdb_idl_search( ID *ids, ID id );

int bdb_idl_fetch_key(
	BackendDB	*be,
	DB			*db,
	DB_TXN		*txn,
	DBT			*key,
	ID			*ids,
	DBC                     **saved_cursor,
	int                     get_flag );

int bdb_idl_insert( ID *ids, ID id );
int bdb_idl_delete( ID *ids, ID id );

int bdb_idl_insert_key(
	BackendDB *be,
	DB *db,
	DB_TXN *txn,
	DBT *key,
	ID id );

int bdb_idl_delete_key(
	BackendDB *be,
	DB *db,
	DB_TXN *txn,
	DBT *key,
	ID id );

int
bdb_idl_intersection(
	ID *a,
	ID *b );

int
bdb_idl_union(
	ID *a,
	ID *b );

ID bdb_idl_first( ID *ids, ID *cursor );
ID bdb_idl_next( ID *ids, ID *cursor );

void bdb_idl_sort( ID *ids, ID *tmp );
int bdb_idl_append( ID *a, ID *b );
int bdb_idl_append_one( ID *ids, ID id );


/*
 * index.c
 */
#define bdb_index_mask				BDB_SYMBOL(index_mask)
#define bdb_index_param				BDB_SYMBOL(index_param)
#define bdb_index_values			BDB_SYMBOL(index_values)
#define bdb_index_entry				BDB_SYMBOL(index_entry)
#define bdb_index_recset			BDB_SYMBOL(index_recset)
#define bdb_index_recrun			BDB_SYMBOL(index_recrun)

extern AttrInfo *
bdb_index_mask LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	struct berval *name ));

extern int
bdb_index_param LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	DB **db,
	slap_mask_t *mask,
	struct berval *prefix ));

extern int
bdb_index_values LDAP_P((
	Operation *op,
	DB_TXN *txn,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int opid ));

extern int
bdb_index_recset LDAP_P((
	struct bdb_info *bdb,
	Attribute *a,
	AttributeType *type,
	struct berval *tags,
	IndexRec *ir ));

extern int
bdb_index_recrun LDAP_P((
	Operation *op,
	struct bdb_info *bdb,
	IndexRec *ir,
	ID id,
	int base ));

int bdb_index_entry LDAP_P(( Operation *op, DB_TXN *t, int r, Entry *e ));

#define bdb_index_entry_add(op,t,e) \
	bdb_index_entry((op),(t),SLAP_INDEX_ADD_OP,(e))
#define bdb_index_entry_del(op,t,e) \
	bdb_index_entry((op),(t),SLAP_INDEX_DELETE_OP,(e))

/*
 * key.c
 */
#define bdb_key_read				BDB_SYMBOL(key_read)
#define bdb_key_change				BDB_SYMBOL(key_change)

extern int
bdb_key_read(
    Backend	*be,
	DB *db,
	DB_TXN *txn,
    struct berval *k,
	ID *ids,
    DBC **saved_cursor,
        int get_flags );

extern int
bdb_key_change(
    Backend	 *be,
    DB *db,
	DB_TXN *txn,
    struct berval *k,
    ID id,
    int	op );
	
/*
 * nextid.c
 */
#define bdb_next_id					BDB_SYMBOL(next_id)
#define bdb_last_id					BDB_SYMBOL(last_id)

int bdb_next_id( BackendDB *be, ID *id );
int bdb_last_id( BackendDB *be, DB_TXN *tid );

/*
 * modify.c
 */
#define bdb_modify_internal			BDB_SYMBOL(modify_internal)

int bdb_modify_internal(
	Operation *op,
	DB_TXN *tid,
	Modifications *modlist,
	Entry *e,
	const char **text,
	char *textbuf,
	size_t textlen );

/*
 * monitor.c
 */

#define bdb_monitor_db_init	BDB_SYMBOL(monitor_db_init)
#define bdb_monitor_db_open	BDB_SYMBOL(monitor_db_open)
#define bdb_monitor_db_close	BDB_SYMBOL(monitor_db_close)
#define bdb_monitor_db_destroy	BDB_SYMBOL(monitor_db_destroy)

int bdb_monitor_db_init( BackendDB *be );
int bdb_monitor_db_open( BackendDB *be );
int bdb_monitor_db_close( BackendDB *be );
int bdb_monitor_db_destroy( BackendDB *be );

#ifdef BDB_MONITOR_IDX
#define bdb_monitor_idx_add	BDB_SYMBOL(monitor_idx_add)
int
bdb_monitor_idx_add(
	struct bdb_info		*bdb,
	AttributeDescription	*desc,
	slap_mask_t		type );
#endif /* BDB_MONITOR_IDX */

/*
 * cache.c
 */
#define bdb_cache_entry_db_unlock	BDB_SYMBOL(cache_entry_db_unlock)
#define bdb_cache_return_entry_rw	BDB_SYMBOL(cache_return_entry_rw)

#define	bdb_cache_entryinfo_lock(e) \
	ldap_pvt_thread_mutex_lock( &(e)->bei_kids_mutex )
#define	bdb_cache_entryinfo_unlock(e) \
	ldap_pvt_thread_mutex_unlock( &(e)->bei_kids_mutex )
#define	bdb_cache_entryinfo_trylock(e) \
	ldap_pvt_thread_mutex_trylock( &(e)->bei_kids_mutex )

/* What a mess. Hopefully the current cache scheme will stabilize
 * and we can trim out all of this stuff.
 */
void bdb_cache_return_entry_rw( struct bdb_info *bdb, Entry *e,
	int rw, DB_LOCK *lock );
#define bdb_cache_return_entry_r(bdb, e, l) \
	bdb_cache_return_entry_rw((bdb), (e), 0, (l))
#define bdb_cache_return_entry_w(bdb, e, l) \
	bdb_cache_return_entry_rw((bdb), (e), 1, (l))
#if 0
void bdb_unlocked_cache_return_entry_rw( struct bdb_info *bdb, Entry *e, int rw );
#else
#define	bdb_unlocked_cache_return_entry_rw( a, b, c )	((void)0)
#endif
#define bdb_unlocked_cache_return_entry_r( c, e ) \
	bdb_unlocked_cache_return_entry_rw((c), (e), 0)
#define bdb_unlocked_cache_return_entry_w( c, e ) \
	bdb_unlocked_cache_return_entry_rw((c), (e), 1)

#define bdb_cache_add				BDB_SYMBOL(cache_add)
#define bdb_cache_children			BDB_SYMBOL(cache_children)
#define bdb_cache_delete			BDB_SYMBOL(cache_delete)
#define bdb_cache_delete_cleanup	BDB_SYMBOL(cache_delete_cleanup)
#define bdb_cache_find_id			BDB_SYMBOL(cache_find_id)
#define bdb_cache_find_ndn			BDB_SYMBOL(cache_find_ndn)
#define bdb_cache_find_parent		BDB_SYMBOL(cache_find_parent)
#define bdb_cache_modify			BDB_SYMBOL(cache_modify)
#define bdb_cache_modrdn			BDB_SYMBOL(cache_modrdn)
#define bdb_cache_release_all		BDB_SYMBOL(cache_release_all)
#define bdb_cache_delete_entry		BDB_SYMBOL(cache_delete_entry)
#define bdb_cache_deref				BDB_SYMBOL(cache_deref)

int bdb_cache_children(
	Operation *op,
	DB_TXN *txn,
	Entry *e
);
int bdb_cache_add(
	struct bdb_info *bdb,
	EntryInfo *pei,
	Entry   *e,
	struct berval *nrdn,
	DB_TXN *txn,
	DB_LOCK *lock
);
int bdb_cache_modrdn(
	struct bdb_info *bdb,
	Entry	*e,
	struct berval *nrdn,
	Entry	*new,
	EntryInfo *ein,
	DB_TXN *txn,
	DB_LOCK *lock
);
int bdb_cache_modify(
	struct bdb_info *bdb,
	Entry *e,
	Attribute *newAttrs,
	DB_TXN *txn,
	DB_LOCK *lock
);
int bdb_cache_find_ndn(
	Operation *op,
	DB_TXN *txn,
	struct berval   *ndn,
	EntryInfo	**res
);

#define	ID_LOCKED	1
#define	ID_NOCACHE	2
#define	ID_NOENTRY	4
#define	ID_CHKPURGE	8
int bdb_cache_find_id(
	Operation *op,
	DB_TXN	*tid,
	ID		id,
	EntryInfo **eip,
	int	flag,
	DB_LOCK		*lock
);
int
bdb_cache_find_parent(
	Operation *op,
	DB_TXN *txn,
	ID id,
	EntryInfo **res
);
int bdb_cache_delete(
	struct bdb_info *bdb,
	Entry	*e,
	DB_TXN *txn,
	DB_LOCK	*lock
);
void bdb_cache_delete_cleanup(
	Cache	*cache,
	EntryInfo *ei
);
void bdb_cache_release_all( Cache *cache );
void bdb_cache_deref( EntryInfo *ei );

#ifdef BDB_HIER
int hdb_cache_load(
	struct bdb_info *bdb,
	EntryInfo *ei,
	EntryInfo **res
);
#endif

#define bdb_cache_entry_db_relock		BDB_SYMBOL(cache_entry_db_relock)
int bdb_cache_entry_db_relock(
	struct bdb_info *bdb,
	DB_TXN *txn,
	EntryInfo *ei,
	int rw,
	int tryOnly,
	DB_LOCK *lock );

int bdb_cache_entry_db_unlock(
	struct bdb_info *bdb,
	DB_LOCK *lock );

#define bdb_reader_get				BDB_SYMBOL(reader_get)
#define bdb_reader_flush			BDB_SYMBOL(reader_flush)
int bdb_reader_get( Operation *op, DB_ENV *env, DB_TXN **txn );
void bdb_reader_flush( DB_ENV *env );

/*
 * trans.c
 */
#define bdb_trans_backoff			BDB_SYMBOL(trans_backoff)

void
bdb_trans_backoff( int num_retries );

/*
 * former external.h
 */

#define bdb_back_initialize		BDB_SYMBOL(back_initialize)
#define bdb_db_config			BDB_SYMBOL(db_config)
#define bdb_add				BDB_SYMBOL(add)
#define bdb_bind			BDB_SYMBOL(bind)
#define bdb_compare			BDB_SYMBOL(compare)
#define bdb_delete			BDB_SYMBOL(delete)
#define bdb_modify			BDB_SYMBOL(modify)
#define bdb_modrdn			BDB_SYMBOL(modrdn)
#define bdb_search			BDB_SYMBOL(search)
#define bdb_extended			BDB_SYMBOL(extended)
#define bdb_referrals			BDB_SYMBOL(referrals)
#define bdb_operational			BDB_SYMBOL(operational)
#define bdb_hasSubordinates		BDB_SYMBOL(hasSubordinates)
#define bdb_tool_entry_open		BDB_SYMBOL(tool_entry_open)
#define bdb_tool_entry_close		BDB_SYMBOL(tool_entry_close)
#define bdb_tool_entry_first_x		BDB_SYMBOL(tool_entry_first_x)
#define bdb_tool_entry_next		BDB_SYMBOL(tool_entry_next)
#define bdb_tool_entry_get		BDB_SYMBOL(tool_entry_get)
#define bdb_tool_entry_put		BDB_SYMBOL(tool_entry_put)
#define bdb_tool_entry_reindex		BDB_SYMBOL(tool_entry_reindex)
#define bdb_tool_dn2id_get		BDB_SYMBOL(tool_dn2id_get)
#define bdb_tool_entry_modify		BDB_SYMBOL(tool_entry_modify)
#define bdb_tool_idl_add		BDB_SYMBOL(tool_idl_add)

extern BI_init				bdb_back_initialize;

extern BI_db_config			bdb_db_config;

extern BI_op_add			bdb_add;
extern BI_op_bind			bdb_bind;
extern BI_op_compare			bdb_compare;
extern BI_op_delete			bdb_delete;
extern BI_op_modify			bdb_modify;
extern BI_op_modrdn			bdb_modrdn;
extern BI_op_search			bdb_search;
extern BI_op_extended			bdb_extended;

extern BI_chk_referrals			bdb_referrals;

extern BI_operational			bdb_operational;

extern BI_has_subordinates 		bdb_hasSubordinates;

/* tools.c */
extern BI_tool_entry_open		bdb_tool_entry_open;
extern BI_tool_entry_close		bdb_tool_entry_close;
extern BI_tool_entry_first_x		bdb_tool_entry_first_x;
extern BI_tool_entry_next		bdb_tool_entry_next;
extern BI_tool_entry_get		bdb_tool_entry_get;
extern BI_tool_entry_put		bdb_tool_entry_put;
extern BI_tool_entry_reindex		bdb_tool_entry_reindex;
extern BI_tool_dn2id_get		bdb_tool_dn2id_get;
extern BI_tool_entry_modify		bdb_tool_entry_modify;

int bdb_tool_idl_add( BackendDB *be, DB *db, DB_TXN *txn, DBT *key, ID id );

LDAP_END_DECL

#endif /* _PROTO_BDB_H */
