/* config.c - mdb backend configuration file routine */
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

#include "back-mdb.h"

#include "config.h"

#include "lutil.h"
#include "ldap_rq.h"

static ConfigDriver mdb_cf_gen;

enum {
	MDB_CHKPT = 1,
	MDB_DIRECTORY,
	MDB_DBNOSYNC,
	MDB_ENVFLAGS,
	MDB_INDEX,
	MDB_MAXREADERS,
	MDB_MAXSIZE,
	MDB_MODE,
	MDB_SSTACK
};

static ConfigTable mdbcfg[] = {
	{ "directory", "dir", 2, 2, 0, ARG_STRING|ARG_MAGIC|MDB_DIRECTORY,
		mdb_cf_gen, "( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
			"DESC 'Directory for database content' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "checkpoint", "kbyte> <min", 3, 3, 0, ARG_MAGIC|MDB_CHKPT,
		mdb_cf_gen, "( OLcfgDbAt:1.2 NAME 'olcDbCheckpoint' "
			"DESC 'Database checkpoint interval in kbytes and minutes' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",NULL, NULL },
	{ "dbnosync", NULL, 1, 2, 0, ARG_ON_OFF|ARG_MAGIC|MDB_DBNOSYNC,
		mdb_cf_gen, "( OLcfgDbAt:1.4 NAME 'olcDbNoSync' "
			"DESC 'Disable synchronous database writes' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "envflags", "flags", 2, 0, 0, ARG_MAGIC|MDB_ENVFLAGS,
		mdb_cf_gen, "( OLcfgDbAt:12.3 NAME 'olcDbEnvFlags' "
			"DESC 'Database environment flags' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "index", "attr> <[pres,eq,approx,sub]", 2, 3, 0, ARG_MAGIC|MDB_INDEX,
		mdb_cf_gen, "( OLcfgDbAt:0.2 NAME 'olcDbIndex' "
		"DESC 'Attribute index parameters' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "maxreaders", "num", 2, 2, 0, ARG_UINT|ARG_MAGIC|MDB_MAXREADERS,
		mdb_cf_gen, "( OLcfgDbAt:12.1 NAME 'olcDbMaxReaders' "
		"DESC 'Maximum number of threads that may access the DB concurrently' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "maxsize", "size", 2, 2, 0, ARG_ULONG|ARG_MAGIC|MDB_MAXSIZE,
		mdb_cf_gen, "( OLcfgDbAt:12.2 NAME 'olcDbMaxSize' "
		"DESC 'Maximum size of DB in bytes' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "mode", "mode", 2, 2, 0, ARG_MAGIC|MDB_MODE,
		mdb_cf_gen, "( OLcfgDbAt:0.3 NAME 'olcDbMode' "
		"DESC 'Unix permissions of database files' "
		"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "searchstack", "depth", 2, 2, 0, ARG_INT|ARG_MAGIC|MDB_SSTACK,
		mdb_cf_gen, "( OLcfgDbAt:1.9 NAME 'olcDbSearchStack' "
		"DESC 'Depth of search stack in IDLs' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs mdbocs[] = {
	{
		"( OLcfgDbOc:12.1 "
		"NAME 'olcMdbConfig' "
		"DESC 'MDB backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST olcDbDirectory "
		"MAY ( olcDbCheckpoint $ olcDbEnvFlags $ "
		"olcDbNoSync $ olcDbIndex $ olcDbMaxReaders $ olcDbMaxsize $ "
		"olcDbMode $ olcDbSearchStack ) )",
		 	Cft_Database, mdbcfg },
	{ NULL, 0, NULL }
};

static slap_verbmasks mdb_envflags[] = {
	{ BER_BVC("nosync"),	MDB_NOSYNC },
	{ BER_BVC("nometasync"),	MDB_NOMETASYNC },
	{ BER_BVC("writemap"),	MDB_WRITEMAP },
	{ BER_BVC("mapasync"),	MDB_MAPASYNC },
	{ BER_BVC("nordahead"),	MDB_NORDAHEAD },
	{ BER_BVNULL, 0 }
};

/* perform periodic syncs */
static void *
mdb_checkpoint( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	struct mdb_info *mdb = rtask->arg;

	mdb_env_sync( mdb->mi_dbenv, 1 );
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}

/* reindex entries on the fly */
static void *
mdb_online_index( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	BackendDB *be = rtask->arg;
	struct mdb_info *mdb = be->be_private;

	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;

	MDB_cursor *curs;
	MDB_val key, data;
	MDB_txn *txn;
	ID id;
	Entry *e;
	int rc, getnext = 1;
	int i;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	op->o_bd = be;

	id = 1;
	key.mv_size = sizeof(ID);

	while ( 1 ) {
		if ( slapd_shutdown )
			break;

		rc = mdb_txn_begin( mdb->mi_dbenv, NULL, 0, &txn );
		if ( rc )
			break;
		rc = mdb_cursor_open( txn, mdb->mi_id2entry, &curs );
		if ( rc ) {
			mdb_txn_abort( txn );
			break;
		}
		if ( getnext ) {
			getnext = 0;
			key.mv_data = &id;
			rc = mdb_cursor_get( curs, &key, &data, MDB_SET_RANGE );
			if ( rc ) {
				mdb_txn_abort( txn );
				if ( rc == MDB_NOTFOUND )
					rc = 0;
				break;
			}
			memcpy( &id, key.mv_data, sizeof( id ));
		}

		rc = mdb_id2entry( op, curs, id, &e );
		mdb_cursor_close( curs );
		if ( rc ) {
			mdb_txn_abort( txn );
			if ( rc == MDB_NOTFOUND ) {
				id++;
				getnext = 1;
				continue;
			}
			break;
		}
		rc = mdb_index_entry( op, txn, MDB_INDEX_UPDATE_OP, e );
		mdb_entry_return( op, e );
		if ( rc == 0 ) {
			rc = mdb_txn_commit( txn );
			txn = NULL;
		} else {
			mdb_txn_abort( txn );
			txn = NULL;
		}
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(mdb_online_index) ": database %s: "
				"txn_commit failed: %s (%d)\n",
				be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
			break;
		}
		id++;
		getnext = 1;
	}

	for ( i = 0; i < mdb->mi_nattrs; i++ ) {
		if ( mdb->mi_attrs[ i ]->ai_indexmask & MDB_INDEX_DELETING
			|| mdb->mi_attrs[ i ]->ai_newmask == 0 )
		{
			continue;
		}
		mdb->mi_attrs[ i ]->ai_indexmask = mdb->mi_attrs[ i ]->ai_newmask;
		mdb->mi_attrs[ i ]->ai_newmask = 0;
	}

	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	mdb->mi_index_task = NULL;
	ldap_pvt_runqueue_remove( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

/* Cleanup loose ends after Modify completes */
static int
mdb_cf_cleanup( ConfigArgs *c )
{
	struct mdb_info *mdb = c->be->be_private;
	int rc = 0;

	if ( mdb->mi_flags & MDB_DEL_INDEX ) {
		mdb_attr_flush( mdb );
		mdb->mi_flags ^= MDB_DEL_INDEX;
	}

	if ( mdb->mi_flags & MDB_RE_OPEN ) {
		mdb->mi_flags ^= MDB_RE_OPEN;
		rc = c->be->bd_info->bi_db_close( c->be, &c->reply );
		if ( rc == 0 )
			rc = c->be->bd_info->bi_db_open( c->be, &c->reply );
		/* If this fails, we need to restart */
		if ( rc ) {
			slapd_shutdown = 2;
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"failed to reopen database, rc=%d", rc );
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_cf_cleanup)
				": %s\n", c->cr_msg, 0, 0 );
			rc = LDAP_OTHER;
		}
	}

	if ( mdb->mi_flags & MDB_OPEN_INDEX ) {
		rc = mdb_attr_dbs_open( c->be, NULL, &c->reply );
		if ( rc )
			rc = LDAP_OTHER;
	}
	return rc;
}

static int
mdb_cf_gen( ConfigArgs *c )
{
	struct mdb_info *mdb = c->be->be_private;
	int rc;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		rc = 0;
		switch( c->type ) {
		case MDB_MODE: {
			char buf[64];
			struct berval bv;
			bv.bv_len = snprintf( buf, sizeof(buf), "0%o", mdb->mi_dbenv_mode );
			if ( bv.bv_len > 0 && bv.bv_len < sizeof(buf) ) {
				bv.bv_val = buf;
				value_add_one( &c->rvalue_vals, &bv );
			} else {
				rc = 1;
			}
			} break;

		case MDB_CHKPT:
			if ( mdb->mi_txn_cp ) {
				char buf[64];
				struct berval bv;
				bv.bv_len = snprintf( buf, sizeof(buf), "%ld %ld",
					(long) mdb->mi_txn_cp_kbyte, (long) mdb->mi_txn_cp_min );
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

		case MDB_DIRECTORY:
			if ( mdb->mi_dbenv_home ) {
				c->value_string = ch_strdup( mdb->mi_dbenv_home );
			} else {
				rc = 1;
			}
			break;

		case MDB_DBNOSYNC:
			if ( mdb->mi_dbenv_flags & MDB_NOSYNC )
				c->value_int = 1;
			break;

		case MDB_ENVFLAGS:
			if ( mdb->mi_dbenv_flags ) {
				mask_to_verbs( mdb_envflags, mdb->mi_dbenv_flags, &c->rvalue_vals );
			}
			if ( !c->rvalue_vals ) rc = 1;
			break;

		case MDB_INDEX:
			mdb_attr_index_unparse( mdb, &c->rvalue_vals );
			if ( !c->rvalue_vals ) rc = 1;
			break;

		case MDB_SSTACK:
			c->value_int = mdb->mi_search_stack_depth;
			break;

		case MDB_MAXREADERS:
			c->value_int = mdb->mi_readers;
			break;

		case MDB_MAXSIZE:
			c->value_ulong = mdb->mi_mapsize;
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		rc = 0;
		switch( c->type ) {
		case MDB_MODE:
#if 0
			/* FIXME: does it make any sense to change the mode,
			 * if we don't exec a chmod()? */
			mdb->bi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;
			break;
#endif

		/* single-valued no-ops */
		case MDB_SSTACK:
		case MDB_MAXREADERS:
		case MDB_MAXSIZE:
			break;

		case MDB_CHKPT:
			if ( mdb->mi_txn_cp_task ) {
				struct re_s *re = mdb->mi_txn_cp_task;
				mdb->mi_txn_cp_task = NULL;
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
					ldap_pvt_runqueue_stoptask( &slapd_rq, re );
				ldap_pvt_runqueue_remove( &slapd_rq, re );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
			mdb->mi_txn_cp = 0;
			break;
		case MDB_DIRECTORY:
			mdb->mi_flags |= MDB_RE_OPEN;
			ch_free( mdb->mi_dbenv_home );
			mdb->mi_dbenv_home = NULL;
			c->cleanup = mdb_cf_cleanup;
			ldap_pvt_thread_pool_purgekey( mdb->mi_dbenv );
			break;
		case MDB_DBNOSYNC:
			mdb_env_set_flags( mdb->mi_dbenv, MDB_NOSYNC, 0 );
			mdb->mi_dbenv_flags &= ~MDB_NOSYNC;
			break;

		case MDB_ENVFLAGS:
			if ( c->valx == -1 ) {
				int i;
				for ( i=0; mdb_envflags[i].mask; i++) {
					if ( mdb->mi_dbenv_flags & mdb_envflags[i].mask ) {
						/* not all flags are runtime resettable */
						rc = mdb_env_set_flags( mdb->mi_dbenv, mdb_envflags[i].mask, 0 );
						if ( rc ) {
							mdb->mi_flags |= MDB_RE_OPEN;
							c->cleanup = mdb_cf_cleanup;
							rc = 0;
						}
						mdb->mi_dbenv_flags ^= mdb_envflags[i].mask;
					}
				}
			} else {
				int i = verb_to_mask( c->line, mdb_envflags );
				if ( mdb_envflags[i].mask & mdb->mi_dbenv_flags ) {
					rc = mdb_env_set_flags( mdb->mi_dbenv, mdb_envflags[i].mask, 0 );
					if ( rc ) {
						mdb->mi_flags |= MDB_RE_OPEN;
						c->cleanup = mdb_cf_cleanup;
						rc = 0;
					}
					mdb->mi_dbenv_flags ^= mdb_envflags[i].mask;
				} else {
					/* unknown keyword */
					rc = 1;
				}
			}
			break;

		case MDB_INDEX:
			if ( c->valx == -1 ) {
				int i;

				/* delete all (FIXME) */
				for ( i = 0; i < mdb->mi_nattrs; i++ ) {
					mdb->mi_attrs[i]->ai_indexmask |= MDB_INDEX_DELETING;
				}
				mdb->mi_flags |= MDB_DEL_INDEX;
				c->cleanup = mdb_cf_cleanup;

			} else {
				struct berval bv, def = BER_BVC("default");
				char *ptr;

				for (ptr = c->line; !isspace( (unsigned char) *ptr ); ptr++);

				bv.bv_val = c->line;
				bv.bv_len = ptr - bv.bv_val;
				if ( bvmatch( &bv, &def )) {
					mdb->mi_defaultmask = 0;

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

						ai = mdb_attr_mask( mdb, ad );
						/* if we got here... */
						assert( ai != NULL );

						ai->ai_indexmask |= MDB_INDEX_DELETING;
						mdb->mi_flags |= MDB_DEL_INDEX;
						c->cleanup = mdb_cf_cleanup;
					}

					bv.bv_val[ bv.bv_len ] = sep;
					ldap_charray_free( attrs );
				}
			}
			break;
		}
		return rc;
	}

	switch( c->type ) {
	case MDB_MODE:
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
			mdb->mi_dbenv_mode = mode;

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
			mdb->mi_dbenv_mode = mode;
		}
		break;
	case MDB_CHKPT: {
		long	l;
		mdb->mi_txn_cp = 1;
		if ( lutil_atolx( &l, c->argv[1], 0 ) != 0 ) {
			fprintf( stderr, "%s: "
				"invalid kbyte \"%s\" in \"checkpoint\".\n",
				c->log, c->argv[1] );
			return 1;
		}
		mdb->mi_txn_cp_kbyte = l;
		if ( lutil_atolx( &l, c->argv[2], 0 ) != 0 ) {
			fprintf( stderr, "%s: "
				"invalid minutes \"%s\" in \"checkpoint\".\n",
				c->log, c->argv[2] );
			return 1;
		}
		mdb->mi_txn_cp_min = l;
		/* If we're in server mode and time-based checkpointing is enabled,
		 * submit a task to perform periodic checkpoints.
		 */
		if ((slapMode & SLAP_SERVER_MODE) && mdb->mi_txn_cp_min ) {
			struct re_s *re = mdb->mi_txn_cp_task;
			if ( re ) {
				re->interval.tv_sec = mdb->mi_txn_cp_min * 60;
			} else {
				if ( c->be->be_suffix == NULL || BER_BVISNULL( &c->be->be_suffix[0] ) ) {
					fprintf( stderr, "%s: "
						"\"checkpoint\" must occur after \"suffix\".\n",
						c->log );
					return 1;
				}
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				mdb->mi_txn_cp_task = ldap_pvt_runqueue_insert( &slapd_rq,
					mdb->mi_txn_cp_min * 60, mdb_checkpoint, mdb,
					LDAP_XSTRING(mdb_checkpoint), c->be->be_suffix[0].bv_val );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
		}
		} break;

	case MDB_DIRECTORY: {
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

		if ( mdb->mi_dbenv_home )
			ch_free( mdb->mi_dbenv_home );
		mdb->mi_dbenv_home = c->value_string;

		}
		break;

	case MDB_DBNOSYNC:
		if ( c->value_int )
			mdb->mi_dbenv_flags |= MDB_NOSYNC;
		else
			mdb->mi_dbenv_flags ^= MDB_NOSYNC;
		if ( mdb->mi_flags & MDB_IS_OPEN ) {
			mdb_env_set_flags( mdb->mi_dbenv, MDB_NOSYNC,
				c->value_int );
		}
		break;

	case MDB_ENVFLAGS: {
		int i, j;
		for ( i=1; i<c->argc; i++ ) {
			j = verb_to_mask( c->argv[i], mdb_envflags );
			if ( mdb_envflags[j].mask ) {
				if ( mdb->mi_flags & MDB_IS_OPEN )
					rc = mdb_env_set_flags( mdb->mi_dbenv, mdb_envflags[j].mask, 1 );
				else
					rc = 0;
				if ( rc ) {
					mdb->mi_flags |= MDB_RE_OPEN;
					c->cleanup = mdb_cf_cleanup;
					rc = 0;
				}
				mdb->mi_dbenv_flags |= mdb_envflags[j].mask;
			} else {
				/* unknown keyword */
				rc = 1;
			}
		}
		}
		break;

	case MDB_INDEX:
		rc = mdb_attr_index_config( mdb, c->fname, c->lineno,
			c->argc - 1, &c->argv[1], &c->reply);

		if( rc != LDAP_SUCCESS ) return 1;
		c->cleanup = mdb_cf_cleanup;
		mdb->mi_flags |= MDB_OPEN_INDEX;
		if (( mdb->mi_flags & MDB_IS_OPEN ) && !mdb->mi_index_task ) {
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
			mdb->mi_index_task = ldap_pvt_runqueue_insert( &slapd_rq, 36000,
				mdb_online_index, c->be,
				LDAP_XSTRING(mdb_online_index), c->be->be_suffix[0].bv_val );
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
		}
		break;

	case MDB_SSTACK:
		if ( c->value_int < MINIMUM_SEARCH_STACK_DEPTH ) {
			fprintf( stderr,
		"%s: depth %d too small, using %d\n",
			c->log, c->value_int, MINIMUM_SEARCH_STACK_DEPTH );
			c->value_int = MINIMUM_SEARCH_STACK_DEPTH;
		}
		mdb->mi_search_stack_depth = c->value_int;
		break;

	case MDB_MAXREADERS:
		mdb->mi_readers = c->value_int;
		if ( mdb->mi_flags & MDB_IS_OPEN ) {
			mdb->mi_flags |= MDB_RE_OPEN;
			c->cleanup = mdb_cf_cleanup;
		}
		break;

	case MDB_MAXSIZE:
		mdb->mi_mapsize = c->value_ulong;
		if ( mdb->mi_flags & MDB_IS_OPEN ) {
			mdb->mi_flags |= MDB_RE_OPEN;
			c->cleanup = mdb_cf_cleanup;
		}
		break;

	}
	return 0;
}

int mdb_back_init_cf( BackendInfo *bi )
{
	int rc;
	bi->bi_cf_ocs = mdbocs;

	rc = config_register_schema( mdbcfg, mdbocs );
	if ( rc ) return rc;
	return 0;
}
