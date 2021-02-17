/* init.c - initialize bdb backend */
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
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "back-bdb.h"
#include <lutil.h>
#include <ldap_rq.h>
#include "alock.h"
#include "config.h"

static const struct bdbi_database {
	char *file;
	struct berval name;
	int type;
	int flags;
} bdbi_databases[] = {
	{ "id2entry" BDB_SUFFIX, BER_BVC("id2entry"), DB_BTREE, 0 },
	{ "dn2id" BDB_SUFFIX, BER_BVC("dn2id"), DB_BTREE, 0 },
	{ NULL, BER_BVNULL, 0, 0 }
};

typedef void * db_malloc(size_t);
typedef void * db_realloc(void *, size_t);

#define bdb_db_init	BDB_SYMBOL(db_init)
#define bdb_db_open BDB_SYMBOL(db_open)
#define bdb_db_close BDB_SYMBOL(db_close)

static int
bdb_db_init( BackendDB *be, ConfigReply *cr )
{
	struct bdb_info	*bdb;
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_db_init) ": Initializing " BDB_UCTYPE " database\n",
		0, 0, 0 );

	/* allocate backend-database-specific stuff */
	bdb = (struct bdb_info *) ch_calloc( 1, sizeof(struct bdb_info) );

	/* DBEnv parameters */
	bdb->bi_dbenv_home = ch_strdup( SLAPD_DEFAULT_DB_DIR );
	bdb->bi_dbenv_xflags = DB_TIME_NOTGRANTED;
	bdb->bi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;

	bdb->bi_cache.c_maxsize = DEFAULT_CACHE_SIZE;
	bdb->bi_cache.c_minfree = 1;

	bdb->bi_lock_detect = DB_LOCK_DEFAULT;
	bdb->bi_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;
	bdb->bi_search_stack = NULL;

	ldap_pvt_thread_mutex_init( &bdb->bi_database_mutex );
	ldap_pvt_thread_mutex_init( &bdb->bi_lastid_mutex );
#ifdef BDB_HIER
	ldap_pvt_thread_mutex_init( &bdb->bi_modrdns_mutex );
#endif
	ldap_pvt_thread_mutex_init( &bdb->bi_cache.c_lru_mutex );
	ldap_pvt_thread_mutex_init( &bdb->bi_cache.c_count_mutex );
	ldap_pvt_thread_mutex_init( &bdb->bi_cache.c_eifree_mutex );
	ldap_pvt_thread_mutex_init( &bdb->bi_cache.c_dntree.bei_kids_mutex );
	ldap_pvt_thread_rdwr_init ( &bdb->bi_cache.c_rwlock );
	ldap_pvt_thread_rdwr_init( &bdb->bi_idl_tree_rwlock );
	ldap_pvt_thread_mutex_init( &bdb->bi_idl_tree_lrulock );

	be->be_private = bdb;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

#ifndef BDB_MULTIPLE_SUFFIXES
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_ONE_SUFFIX;
#endif

	rc = bdb_monitor_db_init( be );

	return rc;
}

static int
bdb_db_close( BackendDB *be, ConfigReply *cr );

static int
bdb_db_open( BackendDB *be, ConfigReply *cr )
{
	int rc, i;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	struct stat stat1, stat2;
	u_int32_t flags;
	char path[MAXPATHLEN];
	char *dbhome;
	Entry *e = NULL;
	int do_recover = 0, do_alock_recover = 0;
	int alockt, quick = 0;
	int do_retry = 1;

	if ( be->be_suffix == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": need suffix.\n",
			1, 0, 0 );
		return -1;
	}

	Debug( LDAP_DEBUG_ARGS,
		LDAP_XSTRING(bdb_db_open) ": \"%s\"\n",
		be->be_suffix[0].bv_val, 0, 0 );

	/* Check existence of dbenv_home. Any error means trouble */
	rc = stat( bdb->bi_dbenv_home, &stat1 );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"cannot access database directory \"%s\" (%d).\n",
			be->be_suffix[0].bv_val, bdb->bi_dbenv_home, errno );
		return -1;
	}

	/* Perform database use arbitration/recovery logic */
	alockt = (slapMode & SLAP_TOOL_READONLY) ? ALOCK_LOCKED : ALOCK_UNIQUE;
	if ( slapMode & SLAP_TOOL_QUICK ) {
		alockt |= ALOCK_NOSAVE;
		quick = 1;
	}

	rc = alock_open( &bdb->bi_alock_info, 
				"slapd", 
				bdb->bi_dbenv_home, alockt );

	/* alockt is TRUE if the existing environment was created in Quick mode */
	alockt = (rc & ALOCK_NOSAVE) ? 1 : 0;
	rc &= ~ALOCK_NOSAVE;

	if( rc == ALOCK_RECOVER ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"unclean shutdown detected; attempting recovery.\n", 
			be->be_suffix[0].bv_val, 0, 0 );
		do_alock_recover = 1;
		do_recover = DB_RECOVER;
	} else if( rc == ALOCK_BUSY ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"database already in use.\n", 
			be->be_suffix[0].bv_val, 0, 0 );
		return -1;
	} else if( rc != ALOCK_CLEAN ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"alock package is unstable.\n", 
			be->be_suffix[0].bv_val, 0, 0 );
		return -1;
	}
	if ( rc == ALOCK_CLEAN )
		be->be_flags |= SLAP_DBFLAG_CLEAN;

	/*
	 * The DB_CONFIG file may have changed. If so, recover the
	 * database so that new settings are put into effect. Also
	 * note the possible absence of DB_CONFIG in the log.
	 */
	if( stat( bdb->bi_db_config_path, &stat1 ) == 0 ) {
		if ( !do_recover ) {
			char *ptr = lutil_strcopy(path, bdb->bi_dbenv_home);
			*ptr++ = LDAP_DIRSEP[0];
			strcpy( ptr, "__db.001" );
			if( stat( path, &stat2 ) == 0 ) {
				if( stat2.st_mtime < stat1.st_mtime ) {
					Debug( LDAP_DEBUG_ANY,
						LDAP_XSTRING(bdb_db_open) ": DB_CONFIG for suffix \"%s\" has changed.\n",
							be->be_suffix[0].bv_val, 0, 0 );
					if ( quick ) {
						Debug( LDAP_DEBUG_ANY,
							"Cannot use Quick mode; perform manual recovery first.\n",
							0, 0, 0 );
						slapMode ^= SLAP_TOOL_QUICK;
						rc = -1;
						goto fail;
					} else {
						Debug( LDAP_DEBUG_ANY,
							"Performing database recovery to activate new settings.\n",
							0, 0, 0 );
					}
					do_recover = DB_RECOVER;
				}
			}
		}
	}
	else {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": warning - no DB_CONFIG file found "
			"in directory %s: (%d).\n"
			"Expect poor performance for suffix \"%s\".\n",
			bdb->bi_dbenv_home, errno, be->be_suffix[0].bv_val );
	}

	/* Always let slapcat run, regardless of environment state.
	 * This can be used to cause a cache flush after an unclean
	 * shutdown.
	 */
	if ( do_recover && ( slapMode & SLAP_TOOL_READONLY )) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"recovery skipped in read-only mode. "
			"Run manual recovery if errors are encountered.\n",
			be->be_suffix[0].bv_val, 0, 0 );
		do_recover = 0;
		do_alock_recover = 0;
		quick = alockt;
	}

	/* An existing environment in Quick mode has nothing to recover. */
	if ( alockt && do_recover ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"cannot recover, database must be reinitialized.\n", 
			be->be_suffix[0].bv_val, 0, 0 );
		rc = -1;
		goto fail;
	}

	rc = db_env_create( &bdb->bi_dbenv, 0 );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
			"db_env_create failed: %s (%d).\n",
			be->be_suffix[0].bv_val, db_strerror(rc), rc );
		goto fail;
	}

#ifdef HAVE_EBCDIC
	strcpy( path, bdb->bi_dbenv_home );
	__atoe( path );
	dbhome = path;
#else
	dbhome = bdb->bi_dbenv_home;
#endif

	/* If existing environment is clean but doesn't support
	 * currently requested modes, remove it.
	 */
	if ( !do_recover && ( alockt ^ quick )) {
shm_retry:
		rc = bdb->bi_dbenv->remove( bdb->bi_dbenv, dbhome, DB_FORCE );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
				"dbenv remove failed: %s (%d).\n",
				be->be_suffix[0].bv_val, db_strerror(rc), rc );
			bdb->bi_dbenv = NULL;
			goto fail;
		}
		rc = db_env_create( &bdb->bi_dbenv, 0 );
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
				"db_env_create failed: %s (%d).\n",
				be->be_suffix[0].bv_val, db_strerror(rc), rc );
			goto fail;
		}
	}

	bdb->bi_dbenv->set_errpfx( bdb->bi_dbenv, be->be_suffix[0].bv_val );
	bdb->bi_dbenv->set_errcall( bdb->bi_dbenv, bdb_errcall );

	bdb->bi_dbenv->set_lk_detect( bdb->bi_dbenv, bdb->bi_lock_detect );

	if ( !BER_BVISNULL( &bdb->bi_db_crypt_key )) {
		rc = bdb->bi_dbenv->set_encrypt( bdb->bi_dbenv, bdb->bi_db_crypt_key.bv_val,
			DB_ENCRYPT_AES );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
				"dbenv set_encrypt failed: %s (%d).\n",
				be->be_suffix[0].bv_val, db_strerror(rc), rc );
			goto fail;
		}
	}

	/* One long-lived TXN per thread, two TXNs per write op */
	bdb->bi_dbenv->set_tx_max( bdb->bi_dbenv, connection_pool_max * 3 );

	if( bdb->bi_dbenv_xflags != 0 ) {
		rc = bdb->bi_dbenv->set_flags( bdb->bi_dbenv,
			bdb->bi_dbenv_xflags, 1);
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
				"dbenv_set_flags failed: %s (%d).\n",
				be->be_suffix[0].bv_val, db_strerror(rc), rc );
			goto fail;
		}
	}

#define	BDB_TXN_FLAGS	(DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN)

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_db_open) ": database \"%s\": "
		"dbenv_open(%s).\n",
		be->be_suffix[0].bv_val, bdb->bi_dbenv_home, 0);

	flags = DB_INIT_MPOOL | DB_CREATE | DB_THREAD;

	if ( !quick )
		flags |= BDB_TXN_FLAGS;

	/* If a key was set, use shared memory for the BDB environment */
	if ( bdb->bi_shm_key ) {
		bdb->bi_dbenv->set_shm_key( bdb->bi_dbenv, bdb->bi_shm_key );
		flags |= DB_SYSTEM_MEM;
	}
	rc = (bdb->bi_dbenv->open)( bdb->bi_dbenv, dbhome,
			flags | do_recover, bdb->bi_dbenv_mode );

	if ( rc ) {
		/* Regular open failed, probably a missing shm environment.
		 * Start over, do a recovery.
		 */
		if ( !do_recover && bdb->bi_shm_key && do_retry ) {
			bdb->bi_dbenv->close( bdb->bi_dbenv, 0 );
			rc = db_env_create( &bdb->bi_dbenv, 0 );
			if( rc == 0 ) {
				Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_db_open)
					": database \"%s\": "
					"shared memory env open failed, assuming stale env.\n",
					be->be_suffix[0].bv_val, 0, 0 );
				do_retry = 0;
				goto shm_retry;
			}
		}
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\" cannot be %s, err %d. "
			"Restore from backup!\n",
			be->be_suffix[0].bv_val, do_recover ? "recovered" : "opened", rc );
		goto fail;
	}

	if ( do_alock_recover && alock_recover (&bdb->bi_alock_info) != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": database \"%s\": alock_recover failed\n",
			be->be_suffix[0].bv_val, 0, 0 );
		rc = -1;
		goto fail;
	}

#ifdef SLAP_ZONE_ALLOC
	if ( bdb->bi_cache.c_maxsize ) {
		bdb->bi_cache.c_zctx = slap_zn_mem_create(
			SLAP_ZONE_INITSIZE, SLAP_ZONE_MAXSIZE,
			SLAP_ZONE_DELTA, SLAP_ZONE_SIZE);
	}
#endif

	/* dncache defaults to 0 == unlimited
	 * must be >= entrycache
	 */
	if ( bdb->bi_cache.c_eimax && bdb->bi_cache.c_eimax < bdb->bi_cache.c_maxsize ) {
		bdb->bi_cache.c_eimax = bdb->bi_cache.c_maxsize;
	}

	if ( bdb->bi_idl_cache_max_size ) {
		bdb->bi_idl_tree = NULL;
		bdb->bi_idl_cache_size = 0;
	}

	flags = DB_THREAD | bdb->bi_db_opflags;

#ifdef DB_AUTO_COMMIT
	if ( !quick )
		flags |= DB_AUTO_COMMIT;
#endif

	bdb->bi_databases = (struct bdb_db_info **) ch_malloc(
		BDB_INDICES * sizeof(struct bdb_db_info *) );

	/* open (and create) main database */
	for( i = 0; bdbi_databases[i].name.bv_val; i++ ) {
		struct bdb_db_info *db;

		db = (struct bdb_db_info *) ch_calloc(1, sizeof(struct bdb_db_info));

		rc = db_create( &db->bdi_db, bdb->bi_dbenv, 0 );
		if( rc != 0 ) {
			snprintf(cr->msg, sizeof(cr->msg),
				"database \"%s\": db_create(%s) failed: %s (%d).",
				be->be_suffix[0].bv_val, 
				bdb->bi_dbenv_home, db_strerror(rc), rc );
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": %s\n",
				cr->msg, 0, 0 );
			goto fail;
		}

		if( !BER_BVISNULL( &bdb->bi_db_crypt_key )) {
			rc = db->bdi_db->set_flags( db->bdi_db, DB_ENCRYPT );
			if ( rc ) {
				snprintf(cr->msg, sizeof(cr->msg),
					"database \"%s\": db set_flags(DB_ENCRYPT)(%s) failed: %s (%d).",
					be->be_suffix[0].bv_val, 
					bdb->bi_dbenv_home, db_strerror(rc), rc );
				Debug( LDAP_DEBUG_ANY,
					LDAP_XSTRING(bdb_db_open) ": %s\n",
					cr->msg, 0, 0 );
				goto fail;
			}
		}

		if( bdb->bi_flags & BDB_CHKSUM ) {
			rc = db->bdi_db->set_flags( db->bdi_db, DB_CHKSUM );
			if ( rc ) {
				snprintf(cr->msg, sizeof(cr->msg),
					"database \"%s\": db set_flags(DB_CHKSUM)(%s) failed: %s (%d).",
					be->be_suffix[0].bv_val, 
					bdb->bi_dbenv_home, db_strerror(rc), rc );
				Debug( LDAP_DEBUG_ANY,
					LDAP_XSTRING(bdb_db_open) ": %s\n",
					cr->msg, 0, 0 );
				goto fail;
			}
		}

		rc = bdb_db_findsize( bdb, (struct berval *)&bdbi_databases[i].name );

		if( i == BDB_ID2ENTRY ) {
			if ( !rc ) rc = BDB_ID2ENTRY_PAGESIZE;
			rc = db->bdi_db->set_pagesize( db->bdi_db, rc );

			if ( slapMode & SLAP_TOOL_MODE )
				db->bdi_db->mpf->set_priority( db->bdi_db->mpf,
					DB_PRIORITY_VERY_LOW );

			if ( slapMode & SLAP_TOOL_READMAIN ) {
				flags |= DB_RDONLY;
			} else {
				flags |= DB_CREATE;
			}
		} else {
			/* Use FS default size if not configured */
			if ( rc )
				rc = db->bdi_db->set_pagesize( db->bdi_db, rc );

			rc = db->bdi_db->set_flags( db->bdi_db, 
				DB_DUP | DB_DUPSORT );
#ifndef BDB_HIER
			if ( slapMode & SLAP_TOOL_READONLY ) {
				flags |= DB_RDONLY;
			} else {
				flags |= DB_CREATE;
			}
#else
			rc = db->bdi_db->set_dup_compare( db->bdi_db,
				bdb_dup_compare );
			if ( slapMode & (SLAP_TOOL_READONLY|SLAP_TOOL_READMAIN) ) {
				flags |= DB_RDONLY;
			} else {
				flags |= DB_CREATE;
			}
#endif
		}

#ifdef HAVE_EBCDIC
		strcpy( path, bdbi_databases[i].file );
		__atoe( path );
		rc = DB_OPEN( db->bdi_db,
			path,
		/*	bdbi_databases[i].name, */ NULL,
			bdbi_databases[i].type,
			bdbi_databases[i].flags | flags,
			bdb->bi_dbenv_mode );
#else
		rc = DB_OPEN( db->bdi_db,
			bdbi_databases[i].file,
		/*	bdbi_databases[i].name, */ NULL,
			bdbi_databases[i].type,
			bdbi_databases[i].flags | flags,
			bdb->bi_dbenv_mode );
#endif

		if ( rc != 0 ) {
			snprintf( cr->msg, sizeof(cr->msg), "database \"%s\": "
				"db_open(%s/%s) failed: %s (%d).", 
				be->be_suffix[0].bv_val, 
				bdb->bi_dbenv_home, bdbi_databases[i].file,
				db_strerror(rc), rc );
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_db_open) ": %s\n",
				cr->msg, 0, 0 );
			db->bdi_db->close( db->bdi_db, 0 );
			goto fail;
		}

		flags &= ~(DB_CREATE | DB_RDONLY);
		db->bdi_name = bdbi_databases[i].name;
		bdb->bi_databases[i] = db;
	}

	bdb->bi_databases[i] = NULL;
	bdb->bi_ndatabases = i;

	/* get nextid */
	rc = bdb_last_id( be, NULL );
	if( rc != 0 ) {
		snprintf( cr->msg, sizeof(cr->msg), "database \"%s\": "
			"last_id(%s) failed: %s (%d).",
			be->be_suffix[0].bv_val, bdb->bi_dbenv_home,
			db_strerror(rc), rc );
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(bdb_db_open) ": %s\n",
			cr->msg, 0, 0 );
		goto fail;
	}

	if ( !quick ) {
		int txflag = DB_READ_COMMITTED;
		/* avoid deadlocks in server; tools should
		 * wait since they have no deadlock retry mechanism.
		 */
		if ( slapMode & SLAP_SERVER_MODE )
			txflag |= DB_TXN_NOWAIT;
		TXN_BEGIN(bdb->bi_dbenv, NULL, &bdb->bi_cache.c_txn, txflag);
	}

	entry_prealloc( bdb->bi_cache.c_maxsize );
	attr_prealloc( bdb->bi_cache.c_maxsize * 20 );

	/* setup for empty-DN contexts */
	if ( BER_BVISEMPTY( &be->be_nsuffix[0] )) {
		rc = bdb_id2entry( be, NULL, 0, &e );
	}
	if ( !e ) {
		struct berval gluebv = BER_BVC("glue");
		Operation op = {0};
		Opheader ohdr = {0};
		e = entry_alloc();
		e->e_id = 0;
		ber_dupbv( &e->e_name, (struct berval *)&slap_empty_bv );
		ber_dupbv( &e->e_nname, (struct berval *)&slap_empty_bv );
		attr_merge_one( e, slap_schema.si_ad_objectClass,
			&gluebv, NULL );
		attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
			&gluebv, NULL );
		op.o_hdr = &ohdr;
		op.o_bd = be;
		op.ora_e = e;
		op.o_dn = be->be_rootdn;
		op.o_ndn = be->be_rootndn;
		slap_add_opattrs( &op, NULL, NULL, 0, 0 );
	}
	e->e_ocflags = SLAP_OC_GLUE|SLAP_OC__END;
	e->e_private = &bdb->bi_cache.c_dntree;
	bdb->bi_cache.c_dntree.bei_e = e;

	/* monitor setup */
	rc = bdb_monitor_db_open( be );
	if ( rc != 0 ) {
		goto fail;
	}

	bdb->bi_flags |= BDB_IS_OPEN;

	return 0;

fail:
	bdb_db_close( be, NULL );
	return rc;
}

static int
bdb_db_close( BackendDB *be, ConfigReply *cr )
{
	int rc;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	struct bdb_db_info *db;
	bdb_idl_cache_entry_t *entry, *next_entry;

	/* monitor handling */
	(void)bdb_monitor_db_close( be );

	{
		Entry *e = bdb->bi_cache.c_dntree.bei_e;
		if ( e ) {
			bdb->bi_cache.c_dntree.bei_e = NULL;
			e->e_private = NULL;
			bdb_entry_return( e );
		}
	}

	bdb->bi_flags &= ~BDB_IS_OPEN;

	ber_bvarray_free( bdb->bi_db_config );
	bdb->bi_db_config = NULL;

	if( bdb->bi_dbenv ) {
		/* Free cache locker if we enabled locking.
		 * TXNs must all be closed before DBs...
		 */
		if ( !( slapMode & SLAP_TOOL_QUICK ) && bdb->bi_cache.c_txn ) {
			TXN_ABORT( bdb->bi_cache.c_txn );
			bdb->bi_cache.c_txn = NULL;
		}
		bdb_reader_flush( bdb->bi_dbenv );
	}

	while( bdb->bi_databases && bdb->bi_ndatabases-- ) {
		db = bdb->bi_databases[bdb->bi_ndatabases];
		rc = db->bdi_db->close( db->bdi_db, 0 );
		/* Lower numbered names are not strdup'd */
		if( bdb->bi_ndatabases >= BDB_NDB )
			free( db->bdi_name.bv_val );
		free( db );
	}
	free( bdb->bi_databases );
	bdb->bi_databases = NULL;

	bdb_cache_release_all (&bdb->bi_cache);

	if ( bdb->bi_idl_cache_size ) {
		avl_free( bdb->bi_idl_tree, NULL );
		bdb->bi_idl_tree = NULL;
		entry = bdb->bi_idl_lru_head;
		do {
			next_entry = entry->idl_lru_next;
			if ( entry->idl )
				free( entry->idl );
			free( entry->kstr.bv_val );
			free( entry );
			entry = next_entry;
		} while ( entry != bdb->bi_idl_lru_head );
		bdb->bi_idl_lru_head = bdb->bi_idl_lru_tail = NULL;
	}

	/* close db environment */
	if( bdb->bi_dbenv ) {
		/* force a checkpoint, but not if we were ReadOnly,
		 * and not in Quick mode since there are no transactions there.
		 */
		if ( !( slapMode & ( SLAP_TOOL_QUICK|SLAP_TOOL_READONLY ))) {
			rc = TXN_CHECKPOINT( bdb->bi_dbenv, 0, 0, DB_FORCE );
			if( rc != 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"bdb_db_close: database \"%s\": "
					"txn_checkpoint failed: %s (%d).\n",
					be->be_suffix[0].bv_val, db_strerror(rc), rc );
			}
		}

		rc = bdb->bi_dbenv->close( bdb->bi_dbenv, 0 );
		bdb->bi_dbenv = NULL;
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_close: database \"%s\": "
				"close failed: %s (%d)\n",
				be->be_suffix[0].bv_val, db_strerror(rc), rc );
			return rc;
		}
	}

	rc = alock_close( &bdb->bi_alock_info, slapMode & SLAP_TOOL_QUICK );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_close: database \"%s\": alock_close failed\n",
			be->be_suffix[0].bv_val, 0, 0 );
		return -1;
	}

	return 0;
}

static int
bdb_db_destroy( BackendDB *be, ConfigReply *cr )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;

	/* stop and remove checkpoint task */
	if ( bdb->bi_txn_cp_task ) {
		struct re_s *re = bdb->bi_txn_cp_task;
		bdb->bi_txn_cp_task = NULL;
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
			ldap_pvt_runqueue_stoptask( &slapd_rq, re );
		ldap_pvt_runqueue_remove( &slapd_rq, re );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	/* monitor handling */
	(void)bdb_monitor_db_destroy( be );

	if( bdb->bi_dbenv_home ) ch_free( bdb->bi_dbenv_home );
	if( bdb->bi_db_config_path ) ch_free( bdb->bi_db_config_path );

	bdb_attr_index_destroy( bdb );

	ldap_pvt_thread_rdwr_destroy ( &bdb->bi_cache.c_rwlock );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_cache.c_lru_mutex );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_cache.c_count_mutex );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_cache.c_eifree_mutex );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_cache.c_dntree.bei_kids_mutex );
#ifdef BDB_HIER
	ldap_pvt_thread_mutex_destroy( &bdb->bi_modrdns_mutex );
#endif
	ldap_pvt_thread_mutex_destroy( &bdb->bi_lastid_mutex );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_database_mutex );
	ldap_pvt_thread_rdwr_destroy( &bdb->bi_idl_tree_rwlock );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_idl_tree_lrulock );

	ch_free( bdb );
	be->be_private = NULL;

	return 0;
}

int
bdb_back_initialize(
	BackendInfo	*bi )
{
	int rc;

	static char *controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
		LDAP_CONTROL_PAGEDRESULTS,
		LDAP_CONTROL_PRE_READ,
		LDAP_CONTROL_POST_READ,
		LDAP_CONTROL_SUBENTRIES,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
#ifdef LDAP_X_TXN
		LDAP_CONTROL_X_TXN_SPEC,
#endif
		NULL
	};

	/* initialize the underlying database system */
	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(bdb_back_initialize) ": initialize " 
		BDB_UCTYPE " backend\n", 0, 0, 0 );

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_SUBENTRIES |
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	{	/* version check */
		int major, minor, patch, ver;
		char *version = db_version( &major, &minor, &patch );
#ifdef HAVE_EBCDIC
		char v2[1024];

		/* All our stdio does an ASCII to EBCDIC conversion on
		 * the output. Strings from the BDB library are already
		 * in EBCDIC; we have to go back and forth...
		 */
		strcpy( v2, version );
		__etoa( v2 );
		version = v2;
#endif

		ver = (major << 24) | (minor << 16) | patch;
		if( ver != DB_VERSION_FULL ) {
			/* fail if a versions don't match */
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(bdb_back_initialize) ": "
				"BDB library version mismatch:"
				" expected " DB_VERSION_STRING ","
				" got %s\n", version, 0, 0 );
			return -1;
		}

		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_back_initialize)
			": %s\n", version, 0, 0 );
	}

	db_env_set_func_free( ber_memfree );
	db_env_set_func_malloc( (db_malloc *)ber_memalloc );
	db_env_set_func_realloc( (db_realloc *)ber_memrealloc );
#if !defined(NO_THREAD) && DB_VERSION_FULL <= 0x04070000
	/* This is a no-op on a NO_THREAD build. Leave the default
	 * alone so that BDB will sleep on interprocess conflicts.
	 * Don't bother on BDB 4.7...
	 */
	db_env_set_func_yield( ldap_pvt_thread_yield );
#endif

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = bdb_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = bdb_db_open;
	bi->bi_db_close = bdb_db_close;
	bi->bi_db_destroy = bdb_db_destroy;

	bi->bi_op_add = bdb_add;
	bi->bi_op_bind = bdb_bind;
	bi->bi_op_compare = bdb_compare;
	bi->bi_op_delete = bdb_delete;
	bi->bi_op_modify = bdb_modify;
	bi->bi_op_modrdn = bdb_modrdn;
	bi->bi_op_search = bdb_search;

	bi->bi_op_unbind = 0;

	bi->bi_extended = bdb_extended;

	bi->bi_chk_referrals = bdb_referrals;
	bi->bi_operational = bdb_operational;
	bi->bi_has_subordinates = bdb_hasSubordinates;
	bi->bi_entry_release_rw = bdb_entry_release;
	bi->bi_entry_get_rw = bdb_entry_get;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = bdb_tool_entry_open;
	bi->bi_tool_entry_close = bdb_tool_entry_close;
	bi->bi_tool_entry_first = backend_tool_entry_first;
	bi->bi_tool_entry_first_x = bdb_tool_entry_first_x;
	bi->bi_tool_entry_next = bdb_tool_entry_next;
	bi->bi_tool_entry_get = bdb_tool_entry_get;
	bi->bi_tool_entry_put = bdb_tool_entry_put;
	bi->bi_tool_entry_reindex = bdb_tool_entry_reindex;
	bi->bi_tool_sync = 0;
	bi->bi_tool_dn2id_get = bdb_tool_dn2id_get;
	bi->bi_tool_entry_modify = bdb_tool_entry_modify;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	rc = bdb_back_init_cf( bi );

	return rc;
}

#if	(SLAPD_BDB == SLAPD_MOD_DYNAMIC && !defined(BDB_HIER)) || \
	(SLAPD_HDB == SLAPD_MOD_DYNAMIC && defined(BDB_HIER))

/* conditionally define the init_module() function */
#ifdef BDB_HIER
SLAP_BACKEND_INIT_MODULE( hdb )
#else /* !BDB_HIER */
SLAP_BACKEND_INIT_MODULE( bdb )
#endif /* !BDB_HIER */

#endif /* SLAPD_[BH]DB == SLAPD_MOD_DYNAMIC */

