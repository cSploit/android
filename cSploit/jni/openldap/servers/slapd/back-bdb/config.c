/* config.c - bdb backend configuration file routine */
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
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/errno.h>

#include "back-bdb.h"

#include "config.h"

#include "lutil.h"
#include "ldap_rq.h"

#ifdef DB_DIRTY_READ
#	define	SLAP_BDB_ALLOW_DIRTY_READ
#endif

#define bdb_cf_gen		BDB_SYMBOL(cf_gen)
#define	bdb_cf_cleanup		BDB_SYMBOL(cf_cleanup)
#define bdb_checkpoint		BDB_SYMBOL(checkpoint)
#define bdb_online_index	BDB_SYMBOL(online_index)

static ConfigDriver bdb_cf_gen;

enum {
	BDB_CHKPT = 1,
	BDB_CONFIG,
	BDB_CRYPTFILE,
	BDB_CRYPTKEY,
	BDB_DIRECTORY,
	BDB_NOSYNC,
	BDB_DIRTYR,
	BDB_INDEX,
	BDB_LOCKD,
	BDB_SSTACK,
	BDB_MODE,
	BDB_PGSIZE,
	BDB_CHECKSUM
};

static ConfigTable bdbcfg[] = {
	{ "directory", "dir", 2, 2, 0, ARG_STRING|ARG_MAGIC|BDB_DIRECTORY,
		bdb_cf_gen, "( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
			"DESC 'Directory for database content' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "cachefree", "size", 2, 2, 0, ARG_ULONG|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_cache.c_minfree),
		"( OLcfgDbAt:1.11 NAME 'olcDbCacheFree' "
			"DESC 'Number of extra entries to free when max is reached' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "cachesize", "size", 2, 2, 0, ARG_ULONG|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_cache.c_maxsize),
		"( OLcfgDbAt:1.1 NAME 'olcDbCacheSize' "
			"DESC 'Entry cache size in entries' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "checkpoint", "kbyte> <min", 3, 3, 0, ARG_MAGIC|BDB_CHKPT,
		bdb_cf_gen, "( OLcfgDbAt:1.2 NAME 'olcDbCheckpoint' "
			"DESC 'Database checkpoint interval in kbytes and minutes' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",NULL, NULL },
	{ "checksum", NULL, 1, 2, 0, ARG_ON_OFF|ARG_MAGIC|BDB_CHECKSUM,
		bdb_cf_gen, "( OLcfgDbAt:1.16 NAME 'olcDbChecksum' "
			"DESC 'Enable database checksum validation' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "cryptfile", "file", 2, 2, 0, ARG_STRING|ARG_MAGIC|BDB_CRYPTFILE,
		bdb_cf_gen, "( OLcfgDbAt:1.13 NAME 'olcDbCryptFile' "
			"DESC 'Pathname of file containing the DB encryption key' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",NULL, NULL },
	{ "cryptkey", "key", 2, 2, 0, ARG_BERVAL|ARG_MAGIC|BDB_CRYPTKEY,
		bdb_cf_gen, "( OLcfgDbAt:1.14 NAME 'olcDbCryptKey' "
			"DESC 'DB encryption key' "
			"SYNTAX OMsOctetString SINGLE-VALUE )",NULL, NULL },
	{ "dbconfig", "DB_CONFIG setting", 1, 0, 0, ARG_MAGIC|BDB_CONFIG,
		bdb_cf_gen, "( OLcfgDbAt:1.3 NAME 'olcDbConfig' "
			"DESC 'BerkeleyDB DB_CONFIG configuration directives' "
			"SYNTAX OMsIA5String X-ORDERED 'VALUES' )", NULL, NULL },
	{ "dbnosync", NULL, 1, 2, 0, ARG_ON_OFF|ARG_MAGIC|BDB_NOSYNC,
		bdb_cf_gen, "( OLcfgDbAt:1.4 NAME 'olcDbNoSync' "
			"DESC 'Disable synchronous database writes' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "dbpagesize", "db> <size", 3, 3, 0, ARG_MAGIC|BDB_PGSIZE,
		bdb_cf_gen, "( OLcfgDbAt:1.15 NAME 'olcDbPageSize' "
			"DESC 'Page size of specified DB, in Kbytes' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "dirtyread", NULL, 1, 2, 0,
#ifdef SLAP_BDB_ALLOW_DIRTY_READ
		ARG_ON_OFF|ARG_MAGIC|BDB_DIRTYR, bdb_cf_gen,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgDbAt:1.5 NAME 'olcDbDirtyRead' "
		"DESC 'Allow reads of uncommitted data' "
		"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "dncachesize", "size", 2, 2, 0, ARG_ULONG|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_cache.c_eimax),
		"( OLcfgDbAt:1.12 NAME 'olcDbDNcacheSize' "
			"DESC 'DN cache size' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "idlcachesize", "size", 2, 2, 0, ARG_ULONG|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_idl_cache_max_size),
		"( OLcfgDbAt:1.6 NAME 'olcDbIDLcacheSize' "
		"DESC 'IDL cache size in IDLs' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "index", "attr> <[pres,eq,approx,sub]", 2, 3, 0, ARG_MAGIC|BDB_INDEX,
		bdb_cf_gen, "( OLcfgDbAt:0.2 NAME 'olcDbIndex' "
		"DESC 'Attribute index parameters' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "linearindex", NULL, 1, 2, 0, ARG_ON_OFF|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_linear_index), 
		"( OLcfgDbAt:1.7 NAME 'olcDbLinearIndex' "
		"DESC 'Index attributes one at a time' "
		"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "lockdetect", "policy", 2, 2, 0, ARG_MAGIC|BDB_LOCKD,
		bdb_cf_gen, "( OLcfgDbAt:1.8 NAME 'olcDbLockDetect' "
		"DESC 'Deadlock detection algorithm' "
		"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "mode", "mode", 2, 2, 0, ARG_MAGIC|BDB_MODE,
		bdb_cf_gen, "( OLcfgDbAt:0.3 NAME 'olcDbMode' "
		"DESC 'Unix permissions of database files' "
		"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "searchstack", "depth", 2, 2, 0, ARG_INT|ARG_MAGIC|BDB_SSTACK,
		bdb_cf_gen, "( OLcfgDbAt:1.9 NAME 'olcDbSearchStack' "
		"DESC 'Depth of search stack in IDLs' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "shm_key", "key", 2, 2, 0, ARG_LONG|ARG_OFFSET,
		(void *)offsetof(struct bdb_info, bi_shm_key), 
		"( OLcfgDbAt:1.10 NAME 'olcDbShmKey' "
		"DESC 'Key for shared memory region' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs bdbocs[] = {
	{
#ifdef BDB_HIER
		"( OLcfgDbOc:1.2 "
		"NAME 'olcHdbConfig' "
		"DESC 'HDB backend configuration' "
#else
		"( OLcfgDbOc:1.1 "
		"NAME 'olcBdbConfig' "
		"DESC 'BDB backend configuration' "
#endif
		"SUP olcDatabaseConfig "
		"MUST olcDbDirectory "
		"MAY ( olcDbCacheSize $ olcDbCheckpoint $ olcDbConfig $ "
		"olcDbCryptFile $ olcDbCryptKey $ "
		"olcDbNoSync $ olcDbDirtyRead $ olcDbIDLcacheSize $ "
		"olcDbIndex $ olcDbLinearIndex $ olcDbLockDetect $ "
		"olcDbMode $ olcDbSearchStack $ olcDbShmKey $ "
		"olcDbCacheFree $ olcDbDNcacheSize $ olcDbPageSize ) )",
		 	Cft_Database, bdbcfg },
	{ NULL, 0, NULL }
};

static slap_verbmasks bdb_lockd[] = {
	{ BER_BVC("default"), DB_LOCK_DEFAULT },
	{ BER_BVC("oldest"), DB_LOCK_OLDEST },
	{ BER_BVC("random"), DB_LOCK_RANDOM },
	{ BER_BVC("youngest"), DB_LOCK_YOUNGEST },
	{ BER_BVC("fewest"), DB_LOCK_MINLOCKS },
	{ BER_BVNULL, 0 }
};

/* perform periodic checkpoints */
static void *
bdb_checkpoint( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	struct bdb_info *bdb = rtask->arg;
	
	TXN_CHECKPOINT( bdb->bi_dbenv, bdb->bi_txn_cp_kbyte,
		bdb->bi_txn_cp_min, 0 );
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}

/* reindex entries on the fly */
static void *
bdb_online_index( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	BackendDB *be = rtask->arg;
	struct bdb_info *bdb = be->be_private;

	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;

	DBC *curs;
	DBT key, data;
	DB_TXN *txn;
	DB_LOCK lock;
	ID id, nid;
	EntryInfo *ei;
	int rc, getnext = 1;
	int i;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	op->o_bd = be;

	DBTzero( &key );
	DBTzero( &data );
	
	id = 1;
	key.data = &nid;
	key.size = key.ulen = sizeof(ID);
	key.flags = DB_DBT_USERMEM;

	data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
	data.dlen = data.ulen = 0;

	while ( 1 ) {
		if ( slapd_shutdown )
			break;

		rc = TXN_BEGIN( bdb->bi_dbenv, NULL, &txn, bdb->bi_db_opflags );
		if ( rc ) 
			break;
		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(bdb_online_index) ": txn id: %x\n",
			txn->id(txn), 0, 0 );
		if ( getnext ) {
			getnext = 0;
			BDB_ID2DISK( id, &nid );
			rc = bdb->bi_id2entry->bdi_db->cursor(
				bdb->bi_id2entry->bdi_db, txn, &curs, bdb->bi_db_opflags );
			if ( rc ) {
				TXN_ABORT( txn );
				break;
			}
			rc = curs->c_get( curs, &key, &data, DB_SET_RANGE );
			curs->c_close( curs );
			if ( rc ) {
				TXN_ABORT( txn );
				if ( rc == DB_NOTFOUND )
					rc = 0;
				if ( rc == DB_LOCK_DEADLOCK ) {
					ldap_pvt_thread_yield();
					continue;
				}
				break;
			}
			BDB_DISK2ID( &nid, &id );
		}

		ei = NULL;
		rc = bdb_cache_find_id( op, txn, id, &ei, 0, &lock );
		if ( rc ) {
			TXN_ABORT( txn );
			if ( rc == DB_LOCK_DEADLOCK ) {
				ldap_pvt_thread_yield();
				continue;
			}
			if ( rc == DB_NOTFOUND ) {
				id++;
				getnext = 1;
				continue;
			}
			break;
		}
		if ( ei->bei_e ) {
			rc = bdb_index_entry( op, txn, BDB_INDEX_UPDATE_OP, ei->bei_e );
			if ( rc ) {
				TXN_ABORT( txn );
				if ( rc == DB_LOCK_DEADLOCK ) {
					ldap_pvt_thread_yield();
					continue;
				}
				break;
			}
			rc = TXN_COMMIT( txn, 0 );
			txn = NULL;
		}
		id++;
		getnext = 1;
	}

	for ( i = 0; i < bdb->bi_nattrs; i++ ) {
		if ( bdb->bi_attrs[ i ]->ai_indexmask & BDB_INDEX_DELETING
			|| bdb->bi_attrs[ i ]->ai_newmask == 0 )
		{
			continue;
		}
		bdb->bi_attrs[ i ]->ai_indexmask = bdb->bi_attrs[ i ]->ai_newmask;
		bdb->bi_attrs[ i ]->ai_newmask = 0;
	}

	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	bdb->bi_index_task = NULL;
	ldap_pvt_runqueue_remove( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

/* Cleanup loose ends after Modify completes */
static int
bdb_cf_cleanup( ConfigArgs *c )
{
	struct bdb_info *bdb = c->be->be_private;
	int rc = 0;
	BerVarray bva;

	if ( bdb->bi_flags & BDB_DEL_INDEX ) {
		bdb_attr_flush( bdb );
		bdb->bi_flags ^= BDB_DEL_INDEX;
	}

	if ( bdb->bi_flags & BDB_RE_OPEN ) {
		bdb->bi_flags ^= BDB_RE_OPEN;
		bva = bdb->bi_db_config;
		bdb->bi_db_config = NULL;
		rc = c->be->bd_info->bi_db_close( c->be, &c->reply );
		if ( rc == 0 ) {
			if ( bdb->bi_flags & BDB_UPD_CONFIG ) {
				if ( bva ) {
					int i;
					FILE *f = fopen( bdb->bi_db_config_path, "w" );
					if ( f ) {
						bdb->bi_db_config = bva;
						bva = NULL;
						for (i=0; bdb->bi_db_config[i].bv_val; i++)
							fprintf( f, "%s\n", bdb->bi_db_config[i].bv_val );
						fclose( f );
					} else {
						ber_bvarray_free( bva );
					}
				} else {
					unlink( bdb->bi_db_config_path );
				}
				bdb->bi_flags ^= BDB_UPD_CONFIG;
			}
			rc = c->be->bd_info->bi_db_open( c->be, &c->reply );
		}
		/* If this fails, we need to restart */
		if ( rc ) {
			slapd_shutdown = 2;
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"failed to reopen database, rc=%d", rc );
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_cf_cleanup)
				": %s\n", c->cr_msg, 0, 0 );
			rc = LDAP_OTHER;
		}
	}
	return rc;
}

static int
bdb_cf_gen( ConfigArgs *c )
{
	struct bdb_info *bdb = c->be->be_private;
	int rc;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		rc = 0;
		switch( c->type ) {
		case BDB_MODE: {
			char buf[64];
			struct berval bv;
			bv.bv_len = snprintf( buf, sizeof(buf), "0%o", bdb->bi_dbenv_mode );
			if ( bv.bv_len > 0 && bv.bv_len < sizeof(buf) ) {
				bv.bv_val = buf;
				value_add_one( &c->rvalue_vals, &bv );
			} else {
				rc = 1;
			}
			} break;

		case BDB_CHKPT:
			if ( bdb->bi_txn_cp ) {
				char buf[64];
				struct berval bv;
				bv.bv_len = snprintf( buf, sizeof(buf), "%ld %ld",
					(long) bdb->bi_txn_cp_kbyte, (long) bdb->bi_txn_cp_min );
				if ( bv.bv_len > 0 && bv.bv_len < sizeof(buf) ) {
					bv.bv_val = buf;
					value_add_one( &c->rvalue_vals, &bv );
				} else {
					rc = 1;
				}
			} else {
				rc = 1;
			}
			break;

		case BDB_CRYPTFILE:
			if ( bdb->bi_db_crypt_file ) {
				c->value_string = ch_strdup( bdb->bi_db_crypt_file );
			} else {
				rc = 1;
			}
			break;

		/* If a crypt file has been set, its contents are copied here.
		 * But we don't want the key to be incorporated here.
		 */
		case BDB_CRYPTKEY:
			if ( !bdb->bi_db_crypt_file && !BER_BVISNULL( &bdb->bi_db_crypt_key )) {
				value_add_one( &c->rvalue_vals, &bdb->bi_db_crypt_key );
			} else {
				rc = 1;
			}
			break;

		case BDB_DIRECTORY:
			if ( bdb->bi_dbenv_home ) {
				c->value_string = ch_strdup( bdb->bi_dbenv_home );
			} else {
				rc = 1;
			}
			break;

		case BDB_CONFIG:
			if ( !( bdb->bi_flags & BDB_IS_OPEN )
				&& !bdb->bi_db_config )
			{
				char	buf[SLAP_TEXT_BUFLEN];
				FILE *f = fopen( bdb->bi_db_config_path, "r" );
				struct berval bv;

				if ( f ) {
					bdb->bi_flags |= BDB_HAS_CONFIG;
					while ( fgets( buf, sizeof(buf), f )) {
						ber_str2bv( buf, 0, 1, &bv );
						if ( bv.bv_len > 0 && bv.bv_val[bv.bv_len-1] == '\n' ) {
							bv.bv_len--;
							bv.bv_val[bv.bv_len] = '\0';
						}
						/* shouldn't need this, but ... */
						if ( bv.bv_len > 0 && bv.bv_val[bv.bv_len-1] == '\r' ) {
							bv.bv_len--;
							bv.bv_val[bv.bv_len] = '\0';
						}
						ber_bvarray_add( &bdb->bi_db_config, &bv );
					}
					fclose( f );
				}
			}
			if ( bdb->bi_db_config ) {
				int i;
				struct berval bv;

				bv.bv_val = c->log;
				for (i=0; !BER_BVISNULL(&bdb->bi_db_config[i]); i++) {
					bv.bv_len = sprintf( bv.bv_val, "{%d}%s", i,
						bdb->bi_db_config[i].bv_val );
					value_add_one( &c->rvalue_vals, &bv );
				}
			}
			if ( !c->rvalue_vals ) rc = 1;
			break;

		case BDB_NOSYNC:
			if ( bdb->bi_dbenv_xflags & DB_TXN_NOSYNC )
				c->value_int = 1;
			break;
			
		case BDB_CHECKSUM:
			if ( bdb->bi_flags & BDB_CHKSUM )
				c->value_int = 1;
			break;

		case BDB_INDEX:
			bdb_attr_index_unparse( bdb, &c->rvalue_vals );
			if ( !c->rvalue_vals ) rc = 1;
			break;

		case BDB_LOCKD:
			rc = 1;
			if ( bdb->bi_lock_detect != DB_LOCK_DEFAULT ) {
				int i;
				for (i=0; !BER_BVISNULL(&bdb_lockd[i].word); i++) {
					if ( bdb->bi_lock_detect == (u_int32_t)bdb_lockd[i].mask ) {
						value_add_one( &c->rvalue_vals, &bdb_lockd[i].word );
						rc = 0;
						break;
					}
				}
			}
			break;

		case BDB_SSTACK:
			c->value_int = bdb->bi_search_stack_depth;
			break;

		case BDB_PGSIZE: {
				struct bdb_db_pgsize *ps;
				char buf[SLAP_TEXT_BUFLEN];
				struct berval bv;
				int rc = 1;

				bv.bv_val = buf;
				for ( ps = bdb->bi_pagesizes; ps; ps = ps->bdp_next ) {
					bv.bv_len = sprintf( buf, "%s %d", ps->bdp_name.bv_val,
						ps->bdp_size / 1024 );
					value_add_one( &c->rvalue_vals, &bv );
					rc = 0;

				}
				break;
			}
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		rc = 0;
		switch( c->type ) {
		case BDB_MODE:
#if 0
			/* FIXME: does it make any sense to change the mode,
			 * if we don't exec a chmod()? */
			bdb->bi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;
			break;
#endif

		/* single-valued no-ops */
		case BDB_LOCKD:
		case BDB_SSTACK:
			break;

		case BDB_CHKPT:
			if ( bdb->bi_txn_cp_task ) {
				struct re_s *re = bdb->bi_txn_cp_task;
				bdb->bi_txn_cp_task = NULL;
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
					ldap_pvt_runqueue_stoptask( &slapd_rq, re );
				ldap_pvt_runqueue_remove( &slapd_rq, re );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
			bdb->bi_txn_cp = 0;
			break;
		case BDB_CONFIG:
			if ( c->valx < 0 ) {
				ber_bvarray_free( bdb->bi_db_config );
				bdb->bi_db_config = NULL;
			} else {
				int i = c->valx;
				ch_free( bdb->bi_db_config[i].bv_val );
				for (; bdb->bi_db_config[i].bv_val; i++)
					bdb->bi_db_config[i] = bdb->bi_db_config[i+1];
			}
			bdb->bi_flags |= BDB_UPD_CONFIG|BDB_RE_OPEN;
			c->cleanup = bdb_cf_cleanup;
			break;
		/* Doesn't really make sense to change these on the fly;
		 * the entire DB must be dumped and reloaded
		 */
		case BDB_CRYPTFILE:
			if ( bdb->bi_db_crypt_file ) {
				ch_free( bdb->bi_db_crypt_file );
				bdb->bi_db_crypt_file = NULL;
			}
			/* FALLTHRU */
		case BDB_CRYPTKEY:
			if ( !BER_BVISNULL( &bdb->bi_db_crypt_key )) {
				ch_free( bdb->bi_db_crypt_key.bv_val );
				BER_BVZERO( &bdb->bi_db_crypt_key );
			}
			break;
		case BDB_DIRECTORY:
			bdb->bi_flags |= BDB_RE_OPEN;
			bdb->bi_flags ^= BDB_HAS_CONFIG;
			ch_free( bdb->bi_dbenv_home );
			bdb->bi_dbenv_home = NULL;
			ch_free( bdb->bi_db_config_path );
			bdb->bi_db_config_path = NULL;
			c->cleanup = bdb_cf_cleanup;
			ldap_pvt_thread_pool_purgekey( bdb->bi_dbenv );
			break;
		case BDB_NOSYNC:
			bdb->bi_dbenv->set_flags( bdb->bi_dbenv, DB_TXN_NOSYNC, 0 );
			break;
		case BDB_CHECKSUM:
			bdb->bi_flags &= ~BDB_CHKSUM;
			break;
		case BDB_INDEX:
			if ( c->valx == -1 ) {
				int i;

				/* delete all (FIXME) */
				for ( i = 0; i < bdb->bi_nattrs; i++ ) {
					bdb->bi_attrs[i]->ai_indexmask |= BDB_INDEX_DELETING;
				}
				bdb->bi_flags |= BDB_DEL_INDEX;
				c->cleanup = bdb_cf_cleanup;

			} else {
				struct berval bv, def = BER_BVC("default");
				char *ptr;

				for (ptr = c->line; !isspace( (unsigned char) *ptr ); ptr++);

				bv.bv_val = c->line;
				bv.bv_len = ptr - bv.bv_val;
				if ( bvmatch( &bv, &def )) {
					bdb->bi_defaultmask = 0;

				} else {
					int i;
					char **attrs;
					char sep;

					sep = bv.bv_val[ bv.bv_len ];
					bv.bv_val[ bv.bv_len ] = '\0';
					attrs = ldap_str2charray( bv.bv_val, "," );

					for ( i = 0; attrs[ i ]; i++ ) {
						AttributeDescription *ad = NULL;
						const char *text;
						AttrInfo *ai;

						slap_str2ad( attrs[ i ], &ad, &text );
						/* if we got here... */
						assert( ad != NULL );

						ai = bdb_attr_mask( bdb, ad );
						/* if we got here... */
						assert( ai != NULL );

						ai->ai_indexmask |= BDB_INDEX_DELETING;
						bdb->bi_flags |= BDB_DEL_INDEX;
						c->cleanup = bdb_cf_cleanup;
					}

					bv.bv_val[ bv.bv_len ] = sep;
					ldap_charray_free( attrs );
				}
			}
			break;
		/* doesn't make sense on the fly; the DB file must be
		 * recreated
		 */
		case BDB_PGSIZE: {
				struct bdb_db_pgsize *ps, **prev;
				int i;

				for ( i = 0, prev = &bdb->bi_pagesizes, ps = *prev; ps;
					prev = &ps->bdp_next, ps = ps->bdp_next, i++ ) {
					if ( c->valx == -1 || i == c->valx ) {
						*prev = ps->bdp_next;
						ch_free( ps );
						ps = *prev;
						if ( i == c->valx ) break;
					}
				}
			}
			break;
		}
		return rc;
	}

	switch( c->type ) {
	case BDB_MODE:
		if ( ASCII_DIGIT( c->argv[1][0] ) ) {
			long mode;
			char *next;
			errno = 0;
			mode = strtol( c->argv[1], &next, 0 );
			if ( errno != 0 || next == c->argv[1] || next[0] != '\0' ) {
				fprintf( stderr, "%s: "
					"unable to parse mode=\"%s\".\n",
					c->log, c->argv[1] );
				return 1;
			}
			bdb->bi_dbenv_mode = mode;

		} else {
			char *m = c->argv[1];
			int who, what, mode = 0;

			if ( strlen( m ) != STRLENOF("-rwxrwxrwx") ) {
				return 1;
			}

			if ( m[0] != '-' ) {
				return 1;
			}

			m++;
			for ( who = 0; who < 3; who++ ) {
				for ( what = 0; what < 3; what++, m++ ) {
					if ( m[0] == '-' ) {
						continue;
					} else if ( m[0] != "rwx"[what] ) {
						return 1;
					}
					mode += ((1 << (2 - what)) << 3*(2 - who));
				}
			}
			bdb->bi_dbenv_mode = mode;
		}
		break;
	case BDB_CHKPT: {
		long	l;
		bdb->bi_txn_cp = 1;
		if ( lutil_atolx( &l, c->argv[1], 0 ) != 0 ) {
			fprintf( stderr, "%s: "
				"invalid kbyte \"%s\" in \"checkpoint\".\n",
				c->log, c->argv[1] );
			return 1;
		}
		bdb->bi_txn_cp_kbyte = l;
		if ( lutil_atolx( &l, c->argv[2], 0 ) != 0 ) {
			fprintf( stderr, "%s: "
				"invalid minutes \"%s\" in \"checkpoint\".\n",
				c->log, c->argv[2] );
			return 1;
		}
		bdb->bi_txn_cp_min = l;
		/* If we're in server mode and time-based checkpointing is enabled,
		 * submit a task to perform periodic checkpoints.
		 */
		if ((slapMode & SLAP_SERVER_MODE) && bdb->bi_txn_cp_min ) {
			struct re_s *re = bdb->bi_txn_cp_task;
			if ( re ) {
				re->interval.tv_sec = bdb->bi_txn_cp_min * 60;
			} else {
				if ( c->be->be_suffix == NULL || BER_BVISNULL( &c->be->be_suffix[0] ) ) {
					fprintf( stderr, "%s: "
						"\"checkpoint\" must occur after \"suffix\".\n",
						c->log );
					return 1;
				}
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				bdb->bi_txn_cp_task = ldap_pvt_runqueue_insert( &slapd_rq,
					bdb->bi_txn_cp_min * 60, bdb_checkpoint, bdb,
					LDAP_XSTRING(bdb_checkpoint), c->be->be_suffix[0].bv_val );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
		}
		} break;

	case BDB_CONFIG: {
		char *ptr = c->line;
		struct berval bv;

		if ( c->op == SLAP_CONFIG_ADD ) {
			ptr += STRLENOF("dbconfig");
			while (!isspace((unsigned char)*ptr)) ptr++;
			while (isspace((unsigned char)*ptr)) ptr++;
		}

		if ( bdb->bi_flags & BDB_IS_OPEN ) {
			bdb->bi_flags |= BDB_UPD_CONFIG|BDB_RE_OPEN;
			c->cleanup = bdb_cf_cleanup;
		} else {
		/* If we're just starting up...
		 */
			FILE *f;
			/* If a DB_CONFIG file exists, or we don't know the path
			 * to the DB_CONFIG file, ignore these directives
			 */
			if (( bdb->bi_flags & BDB_HAS_CONFIG ) || !bdb->bi_db_config_path )
				break;
			f = fopen( bdb->bi_db_config_path, "a" );
			if ( f ) {
				/* FIXME: EBCDIC probably needs special handling */
				fprintf( f, "%s\n", ptr );
				fclose( f );
			}
		}
		ber_str2bv( ptr, 0, 1, &bv );
		ber_bvarray_add( &bdb->bi_db_config, &bv );
		}
		break;

	case BDB_CRYPTFILE:
		rc = lutil_get_filed_password( c->value_string, &bdb->bi_db_crypt_key );
		if ( rc == 0 ) {
			bdb->bi_db_crypt_file = c->value_string;
		}
		break;

	/* Cannot set key if file was already set */
	case BDB_CRYPTKEY:
		if ( bdb->bi_db_crypt_file ) {
			rc = 1;
		} else {
			bdb->bi_db_crypt_key = c->value_bv;
		}
		break;

	case BDB_DIRECTORY: {
		FILE *f;
		char *ptr, *testpath;
		int len;

		len = strlen( c->value_string );
		testpath = ch_malloc( len + STRLENOF(LDAP_DIRSEP) + STRLENOF("DUMMY") + 1 );
		ptr = lutil_strcopy( testpath, c->value_string );
		*ptr++ = LDAP_DIRSEP[0];
		strcpy( ptr, "DUMMY" );
		f = fopen( testpath, "w" );
		if ( f ) {
			fclose( f );
			unlink( testpath );
		}
		ch_free( testpath );
		if ( !f ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: invalid path: %s",
				c->log, strerror( errno ));
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}

		if ( bdb->bi_dbenv_home )
			ch_free( bdb->bi_dbenv_home );
		bdb->bi_dbenv_home = c->value_string;

		/* See if a DB_CONFIG file already exists here */
		if ( bdb->bi_db_config_path )
			ch_free( bdb->bi_db_config_path );
		bdb->bi_db_config_path = ch_malloc( len +
			STRLENOF(LDAP_DIRSEP) + STRLENOF("DB_CONFIG") + 1 );
		ptr = lutil_strcopy( bdb->bi_db_config_path, bdb->bi_dbenv_home );
		*ptr++ = LDAP_DIRSEP[0];
		strcpy( ptr, "DB_CONFIG" );

		f = fopen( bdb->bi_db_config_path, "r" );
		if ( f ) {
			bdb->bi_flags |= BDB_HAS_CONFIG;
			fclose(f);
		}
		}
		break;

	case BDB_NOSYNC:
		if ( c->value_int )
			bdb->bi_dbenv_xflags |= DB_TXN_NOSYNC;
		else
			bdb->bi_dbenv_xflags &= ~DB_TXN_NOSYNC;
		if ( bdb->bi_flags & BDB_IS_OPEN ) {
			bdb->bi_dbenv->set_flags( bdb->bi_dbenv, DB_TXN_NOSYNC,
				c->value_int );
		}
		break;

	case BDB_CHECKSUM:
		if ( c->value_int )
			bdb->bi_flags |= BDB_CHKSUM;
		else
			bdb->bi_flags &= ~BDB_CHKSUM;
		break;

	case BDB_INDEX:
		rc = bdb_attr_index_config( bdb, c->fname, c->lineno,
			c->argc - 1, &c->argv[1], &c->reply);

		if( rc != LDAP_SUCCESS ) return 1;
		if (( bdb->bi_flags & BDB_IS_OPEN ) && !bdb->bi_index_task ) {
			/* Start the task as soon as we finish here. Set a long
			 * interval (10 hours) so that it only gets scheduled once.
			 */
			if ( c->be->be_suffix == NULL || BER_BVISNULL( &c->be->be_suffix[0] ) ) {
				fprintf( stderr, "%s: "
					"\"index\" must occur after \"suffix\".\n",
					c->log );
				return 1;
			}
			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			bdb->bi_index_task = ldap_pvt_runqueue_insert( &slapd_rq, 36000,
				bdb_online_index, c->be,
				LDAP_XSTRING(bdb_online_index), c->be->be_suffix[0].bv_val );
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
		}
		break;

	case BDB_LOCKD:
		rc = verb_to_mask( c->argv[1], bdb_lockd );
		if ( BER_BVISNULL(&bdb_lockd[rc].word) ) {
			fprintf( stderr, "%s: "
				"bad policy (%s) in \"lockDetect <policy>\" line\n",
				c->log, c->argv[1] );
			return 1;
		}
		bdb->bi_lock_detect = (u_int32_t)rc;
		break;

	case BDB_SSTACK:
		if ( c->value_int < MINIMUM_SEARCH_STACK_DEPTH ) {
			fprintf( stderr,
		"%s: depth %d too small, using %d\n",
			c->log, c->value_int, MINIMUM_SEARCH_STACK_DEPTH );
			c->value_int = MINIMUM_SEARCH_STACK_DEPTH;
		}
		bdb->bi_search_stack_depth = c->value_int;
		break;

	case BDB_PGSIZE: {
		struct bdb_db_pgsize *ps, **prev;
		int i, s;
		
		s = atoi(c->argv[2]);
		if ( s < 1 || s > 64 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s: size must be > 0 and <= 64: %d",
				c->log, s );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		i = strlen(c->argv[1]);
		ps = ch_malloc( sizeof(struct bdb_db_pgsize) + i + 1 );
		ps->bdp_next = NULL;
		ps->bdp_name.bv_len = i;
		ps->bdp_name.bv_val = (char *)(ps+1);
		strcpy( ps->bdp_name.bv_val, c->argv[1] );
		ps->bdp_size = s * 1024;
		for ( prev = &bdb->bi_pagesizes; *prev; prev = &(*prev)->bdp_next )
			;
		*prev = ps;
		}
		break;
	}
	return 0;
}

int bdb_back_init_cf( BackendInfo *bi )
{
	int rc;
	bi->bi_cf_ocs = bdbocs;

	rc = config_register_schema( bdbcfg, bdbocs );
	if ( rc ) return rc;
	return 0;
}
