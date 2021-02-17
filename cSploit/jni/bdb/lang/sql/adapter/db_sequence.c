/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * Add sequence support to the SQLite API via a custom function.
 *
 * Reference implementations of sqlite3_create_function are in:
 * lang/sql/sqlite/src/test1.c
 *
 * The functionality works like:
 * SELECT create_sequence("name", "params");
 * SELECT nextval("name");
 *
 * Parameters to create_sequence can be:
 * * "cache", <num>
 * * "incr", <num>
 * * "minvalue", <num>
 * * "maxvalue", <num>
 *
 * TODO:      Implement setval - I don't think it makes sense.
 *
 * Note: The following code has two different modes, depending on the parameter
 *       to create. If the cache functionality is used, a Berkeley DB sequence
 *       is used as the underlying implementation, and the currval function is
 *       not available. If the cache parameter isn't specified to create, then
 *       the value is stored in the cookie that's saved in the metadata table,
 *       and there is no Berkeley DB sequence used.
 *
 * The implementation is based on documentation from:
 * http://www.postgresql.org/docs/8.0/static/sql-createsequence.html
 * http://www.postgresql.org/docs/8.0/static/functions-sequence.html
 */
#include "sqliteInt.h"
#include "btreeInt.h"
#include "vdbeInt.h"
#include "db.h"
#include <stdarg.h>

#ifdef DB_WINCE
#define	vsnprintf _vsnprintf
#endif
#define	SEQ_HANDLE_OPEN		0x0001
#define	SEQ_HANDLE_CREATE	0x0002

#define SEQ_TXN(c)							\
	(((c)->cache == 0) ? p->savepoint_txn : p->family_txn)

/* Used to differentiate between currval and nextval. */
#define	DB_SEQ_NEXT		0x0000
#define	DB_SEQ_CURRENT		0x0001

#define	MSG_CREATE_FAIL	"Sequence create failed: "
#define	MSG_MALLOC_FAIL	"Malloc failed during sequence operation."

#define CACHE_ENTRY_VALID(_e)						\
	(_e != NULL &&							\
	((((SEQ_COOKIE *)_e->cookie)->cache) == 0 || _e->dbp != NULL))

static int btreeSeqCreate(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie);
static void btreeSeqError(
    sqlite3_context *context, int code, const char *msg, ...);
static int btreeSeqExists(sqlite3_context *context, Btree *p, const char *name);
static int btreeSeqGetCookie(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie, u_int32_t flags);
static int btreeSeqGetHandle(sqlite3_context *context, Btree *p,
    int mode, SEQ_COOKIE *cookie);
static void btreeSeqGetVal(
    sqlite3_context *context, const char *name, int next);
static int btreeSeqOpen(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie);
static int btreeSeqPutCookie(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie, u_int32_t flags);
static int btreeSeqRemoveHandle(
    sqlite3_context *context, Btree *p, CACHED_DB *cache_entry);
static int btreeSeqStartTransaction(
    sqlite3_context *context, Btree *p, int is_write);

static void db_seq_create_func(
    sqlite3_context *context, int argc, sqlite3_value **argv)
{
	Btree *p;
	BtShared *pBt;
	SEQ_COOKIE cookie;
	int i, rc;
	sqlite3 *db;

	if (argc < 1) {
		btreeSeqError(context, SQLITE_ERROR,
		    "wrong number of arguments to function "
		    "create_sequence()");
		return;
	}
	/*
	 * Ensure that the sequence name is OK with our static buffer
	 * size. We need extra characters for "seq_" and "_db".
	 */
	if (strlen((const char *)sqlite3_value_text(argv[0])) >
	    BT_MAX_SEQ_NAME - 8) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Sequence name too long.");
		return;
	}
	db = sqlite3_context_db_handle(context);
	/*
	 * TODO: How do we know which BtShared to use?
	 * What if there are multiple attached DBs, can the user specify which
	 * one to create the sequence on?
	 */
	p = db->aDb[0].pBt;
	pBt = p->pBt;

	if (!p->connected &&
	    (rc = btreeOpenEnvironment(p, 1)) != SQLITE_OK) {
		btreeSeqError(context, SQLITE_ERROR,
		    "%sconnection could not be opened.", MSG_CREATE_FAIL);
		return;
	}

	/* The cookie holds configuration information. */
	memset(&cookie, 0, sizeof(SEQ_COOKIE));
	cookie.incr = 1;

	sqlite3_snprintf(BT_MAX_SEQ_NAME, cookie.name, "seq_%s",
	    sqlite3_value_text(argv[0]));
	cookie.name_len = (int)strlen(cookie.name);
	if (pBt->dbStorage == DB_STORE_NAMED && btreeSeqExists(context, p,
	    cookie.name) == 1) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Attempt to call sequence_create when a sequence "
		    "already exists.");
		return;
	}

	/*
	 * TODO: In the future calling create_sequence when there is already a
	 *       handle in the cache could be used to alter the "cookie" values.
	 *       Don't do that for now, because it requires opening and closing
	 *       the DB handle, which needs care if it's being used by
	 *       multiple threads.
	 */

	/* Set the boundary values to distinguish if user set the values. */
	cookie.min_val = -INT64_MAX;
	cookie.max_val = INT64_MAX;
	cookie.start_val = -INT64_MAX;

	/* Parse options. */
	for (i = 1; i < argc; i++) {
		if (strncmp((char *)sqlite3_value_text(argv[i]),
		    "cache", 5) == 0) {
			if (i == argc ||
			    sqlite3_value_type(argv[++i]) != SQLITE_INTEGER) {
				btreeSeqError(context, SQLITE_ERROR,
				    "%sInvalid parameter.", MSG_CREATE_FAIL);
				goto err;
			}
			cookie.cache = (u32)sqlite3_value_int(argv[i]);			
		} else if (strncmp((char *)sqlite3_value_text(argv[i]),
		    "incr", 4) == 0) {
			if (i == argc ||
			    sqlite3_value_type(argv[++i]) != SQLITE_INTEGER) {
				btreeSeqError(context, SQLITE_ERROR,
				    "%sInvalid parameter.", MSG_CREATE_FAIL);
				goto err;
			}
			cookie.incr = sqlite3_value_int(argv[i]);
		} else if (strncmp((char *)sqlite3_value_text(argv[i]),
		    "maxvalue", 8) == 0) {
			if (i == argc ||
			    sqlite3_value_type(argv[++i]) != SQLITE_INTEGER) {
				btreeSeqError(context, SQLITE_ERROR,
				    "%sInvalid parameter.", MSG_CREATE_FAIL);
				goto err;
			}
			cookie.max_val = sqlite3_value_int(argv[i]);
		} else if (strncmp((char *)sqlite3_value_text(argv[i]),
		    "minvalue", 8) == 0) {
			if (i == argc ||
			    sqlite3_value_type(argv[++i]) != SQLITE_INTEGER) {
				btreeSeqError(context, SQLITE_ERROR,
				    "%sInvalid parameter.", MSG_CREATE_FAIL);
				goto err;
			}
			cookie.min_val = sqlite3_value_int(argv[i]);
		} else if (strncmp((char *)sqlite3_value_text(argv[i]),
		    "start", 5) == 0) {
			if (i == argc ||
			    sqlite3_value_type(argv[++i]) != SQLITE_INTEGER) {
				btreeSeqError(context, SQLITE_ERROR,
				    "%sInvalid parameter.", MSG_CREATE_FAIL);
				goto err;
			}
			cookie.start_val = sqlite3_value_int(argv[i]);
		} else {
			btreeSeqError(context, SQLITE_ERROR,
			    "%sInvalid parameter.", MSG_CREATE_FAIL);
			goto err;
		}
	}

	/*
	 * Setup the cookie. Do this after the parsing so param order doesn't
	 * matter.
	 */
	if (cookie.incr < 0) {
		cookie.decrementing = 1;
		cookie.incr = -cookie.incr;
	}
	/* Attempt to give a reasonable default start value. */
	if (cookie.start_val == -INT64_MAX) {
		/*
		 * Set a reasonable default start value, if
		 * only half of a range has been given.
		 */
		if (cookie.decrementing == 1 &&
		    cookie.max_val != INT64_MAX) {
			cookie.start_val = cookie.max_val;
		} else if (cookie.decrementing == 0 &&
		    cookie.min_val != -INT64_MAX) {
			cookie.start_val = cookie.min_val;
		} else {
			/*
			 * If user does not set start_val, min_val and
			 * max_val, set default start_val to 0 by default.
			 */
			cookie.start_val = 0;
		}
	}

	/* Validate the settings. */
	if (cookie.min_val > cookie.max_val && cookie.max_val != 0) {
		btreeSeqError(context, SQLITE_ERROR,
		    "%sInvalid parameter.", MSG_CREATE_FAIL);
		goto err;
	}

	if (cookie.min_val > cookie.start_val ||
	    cookie.max_val < cookie.start_val) { 
		btreeSeqError(context, SQLITE_ERROR,
		    "%sInvalid parameter.", MSG_CREATE_FAIL);
		goto err;
	}

	if (cookie.cache != 0 && db->autoCommit == 0) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Cannot create caching sequence in a transaction.");
		goto err;
	}

	if ((rc = btreeSeqGetHandle(context, p, SEQ_HANDLE_CREATE, &cookie)) !=
	    SQLITE_OK) {
		if (rc != SQLITE_ERROR)
			btreeSeqError(context, dberr2sqlite(rc, NULL),
			    "Failed to create sequence %s. Error: %s",
			    (const char *)sqlite3_value_text(argv[0]),
			    db_strerror(rc));
		goto err;
	}

	sqlite3_result_int(context, SQLITE_OK);
err:	return;
}

static void db_seq_drop_func(
    sqlite3_context *context, int argc, sqlite3_value **argv)
{
	Btree *p;
	BtShared *pBt;
	CACHED_DB *cache_entry;
	SEQ_COOKIE cookie;
	int mutex_held, rc;
	sqlite3 *db;

	db = sqlite3_context_db_handle(context);
	p = db->aDb[0].pBt;
	pBt = p->pBt;
	mutex_held = 0;
	memset(&cookie, 0, sizeof(cookie));

	if (!p->connected &&
	    (rc = btreeOpenEnvironment(p, 1)) != SQLITE_OK) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Sequence drop failed: connection could not be opened.");
		return;
	}

	sqlite3_snprintf(BT_MAX_SEQ_NAME, cookie.name, "seq_%s",
	    sqlite3_value_text(argv[0]));
	cookie.name_len = (int)strlen(cookie.name);
	rc = btreeSeqGetHandle(context, p, SEQ_HANDLE_OPEN, &cookie);
	
	if (rc != SQLITE_OK) {
		/* If the handle doesn't exist, return an error. */
		if (rc == DB_NOTFOUND) 
			btreeSeqError(context, dberr2sqlite(rc, NULL),
			    "no such sequence: %s", cookie.name + 4);
		else if (rc != SQLITE_ERROR)
			btreeSeqError(context, dberr2sqlite(rc, NULL),
			"Fail to drop sequence %s. Error: %s",
			cookie.name + 4, db_strerror(rc));
		return;
	}

	sqlite3_mutex_enter(pBt->mutex);
	mutex_held = 1;
	cache_entry =
	    sqlite3HashFind(&pBt->db_cache, cookie.name, cookie.name_len);

	if (cache_entry == NULL)
		goto done;

	if (cookie.cache != 0 && db->autoCommit == 0) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Cannot drop caching sequence in a transaction.");
		rc = SQLITE_ERROR;
		goto done;
	}

	/*
	 * Drop the mutex - it's not valid to begin a transaction while
	 * holding the mutex. We can drop it safely because it's use is to
	 * protect handle cache changes.
	 */
	sqlite3_mutex_leave(pBt->mutex);

	if ((rc = btreeSeqStartTransaction(context, p, 1)) != SQLITE_OK) {
			btreeSeqError(context, SQLITE_ERROR,
			    "Could not begin transaction for drop.");
			return;
	}

	sqlite3_mutex_enter(pBt->mutex);
	btreeSeqRemoveHandle(context, p, cache_entry);
done:	sqlite3_mutex_leave(pBt->mutex);

	if (rc == SQLITE_OK)
		sqlite3_result_int(context, SQLITE_OK);
}

static void db_seq_nextval_func(
    sqlite3_context *context, int argc, sqlite3_value **argv)
{
	btreeSeqGetVal(context,
	    (const char *)sqlite3_value_text(argv[0]), DB_SEQ_NEXT);
}
static void db_seq_currval_func(
    sqlite3_context *context, int argc, sqlite3_value **argv)
{
	btreeSeqGetVal(context,
	    (const char *)sqlite3_value_text(argv[0]), DB_SEQ_CURRENT);
}

static void btreeSeqGetVal(
    sqlite3_context *context, const char * name, int mode) {

	Btree *p;
	BtShared *pBt;
	SEQ_COOKIE cookie;
	db_seq_t val;
	int rc, ret;
	sqlite3 *db;

	db = sqlite3_context_db_handle(context);
	p = db->aDb[0].pBt;
	pBt = p->pBt;
	memset(&cookie, 0, sizeof(cookie));

	if (!p->connected &&
	    (rc = btreeOpenEnvironment(p, 1)) != SQLITE_OK) {
		sqlite3_result_error(context,
		    "Sequence open failed: connection could not be opened.",
		    -1);
		return;
	}

	sqlite3_snprintf(BT_MAX_SEQ_NAME, cookie.name, "seq_%s", name);
	cookie.name_len = (int)strlen(cookie.name);
	rc = btreeSeqGetHandle(context, p, SEQ_HANDLE_OPEN, &cookie);

	if (rc != SQLITE_OK) {
		if (rc == DB_NOTFOUND) 
			btreeSeqError(context, dberr2sqlite(rc, NULL),
			    "no such sequence: %s", name);
		else if (rc != SQLITE_ERROR)
			btreeSeqError(context, dberr2sqlite(rc, NULL),
			    "Fail to get next value from seq %s. Error: %s",
			    name, db_strerror(rc));
		return;
	}

	if (cookie.cache == 0) {
		/*
		 * Ensure we see the latest value across connections. Use
		 * DB_RMW to ensure that no other process changes the value
		 * while we're updating it.
		 */
		if ((ret =
		    btreeSeqGetCookie(context, p, &cookie, DB_RMW)) != 0) {
			btreeSeqError(context, SQLITE_ERROR,
			    "Failed to retrieve sequence value. Error: %s",
			    db_strerror(ret));
			return;
		}
		if (mode == DB_SEQ_NEXT) {
			/* Bounds check. */
			if (cookie.used && ((cookie.decrementing &&
			    cookie.val - cookie.incr < cookie.min_val) ||
			    (!cookie.decrementing &&
			    cookie.val + cookie.incr > cookie.max_val))) {
				btreeSeqError(context, SQLITE_ERROR,
				    "Sequence value out of bounds.");
				return;
			}
			if (!cookie.used) {
				cookie.used = 1;
				cookie.val = cookie.start_val;
			} else if (cookie.decrementing)
				cookie.val -= cookie.incr;
			else
				cookie.val += cookie.incr;
			btreeSeqPutCookie(context, p, &cookie, 0);
		} else if (!cookie.used) {
			btreeSeqError(context, SQLITE_ERROR,
			    "Can't call currval on an unused sequence.");
			return ;
		}
		val = cookie.val;
	} else {
		if (mode == DB_SEQ_CURRENT) {
			btreeSeqError(context, SQLITE_ERROR,
			    "Can't call currval on a caching sequence.");
			return;
		}
		/*
		 * Using a cached sequence while an exclusive transaction is
		 * active on this handle causes a hang. Avoid it.
		 */
		if (p->txn_excl == 1) {
			btreeSeqError(context, SQLITE_ERROR,
			    "Can't call nextval on a caching sequence while an"
			    " exclusive transaction is active.");
			return;
		}
		/* Cached gets can't be transactionally protected. */
		if ((ret = cookie.handle->get(cookie.handle, NULL,
		    (u_int32_t)cookie.incr, &val, 0)) != 0) {
			if (ret == EINVAL)
				btreeSeqError(context, SQLITE_ERROR,
				    "Sequence value out of bounds.");
			else
				btreeSeqError(context, SQLITE_ERROR,
				    "Failed sequence get. Error: %s",
				    db_strerror(ret));
			return;
		}
	}

	sqlite3_result_int64(context, val);

}

int add_sequence_functions(sqlite3 *db)
{
	int rc;
	/*
	 * Use the internal sqlite3CreateFunc so that forced malloc failures
	 * during the test runs don't cause failures.
	 */
	rc = sqlite3CreateFunc(db, "create_sequence", -1, SQLITE_ANY, 0,
	    db_seq_create_func, 0, 0, 0);
	rc = sqlite3CreateFunc(db, "drop_sequence", -1, SQLITE_ANY, 0,
	    db_seq_drop_func, 0, 0, 0);
	rc = sqlite3CreateFunc(db, "nextval", 1, SQLITE_ANY, 0,
	    db_seq_nextval_func, 0, 0, 0);
	rc = sqlite3CreateFunc(db, "currval", 1, SQLITE_ANY, 0,
	    db_seq_currval_func, 0, 0, 0);
	return (rc);
}

static void btreeSeqError(
    sqlite3_context *context, int code, const char *fmt, ...)
{
	char buf[BT_MAX_PATH];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, BT_MAX_PATH, fmt, ap);
	va_end(ap);
	sqlite3_result_error(context, buf, -1);
	if (code != SQLITE_ERROR)
		sqlite3_result_error_code(context, code);
}

static int btreeSeqGetHandle(sqlite3_context *context, Btree *p,
    int mode, SEQ_COOKIE *cookie)
{
	BtShared *pBt;
	CACHED_DB *cache_entry, *stale_db;
	int ret;

	cache_entry = NULL;
	ret = SQLITE_OK;
	pBt = p->pBt;

	/* Does not support in-memory db and temp db for now */
	if (pBt->dbStorage != DB_STORE_NAMED) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Sequences do not support in-memory or "
		    "temporary databases.");
		return (SQLITE_ERROR);
	}

	/*
	 * The process here is:
	 * - Is the handle already in the cache - if so, just return.
	 * - Else, does the sequence already exist:
	 *   + No: create the sequence
	 * - open a handle and add it to the cache
	 *
	 * Use the cookie to store the name. If the handle is cached, the value
	 * will be overwritten.
	 */
	sqlite3_mutex_enter(pBt->mutex);
	cache_entry =
	    sqlite3HashFind(&pBt->db_cache, cookie->name, cookie->name_len);
	sqlite3_mutex_leave(pBt->mutex);

	if (CACHE_ENTRY_VALID(cache_entry)) {
		/* Check to see if the handle is no longer valid. */
		if (btreeSeqExists(context, p, cookie->name) == 0) {
			btreeSeqRemoveHandle(context, p, cache_entry);
			return (DB_NOTFOUND);
		}
		if (mode == SEQ_HANDLE_OPEN) {
			assert(cache_entry->cookie != NULL);
			memcpy(cookie, cache_entry->cookie, sizeof(SEQ_COOKIE));
			cookie->handle = (DB_SEQUENCE *)cache_entry->dbp;
			return (SQLITE_OK);
		} else if (mode == SEQ_HANDLE_CREATE) {
			cookie->handle = NULL;
			return (DB_KEYEXIST);
		}
	}

	/*
	 * The handle wasn't found; Get seq handle by open or create the
	 * sequence with the given name.
	 */
	if ((ret = btreeSeqOpen(context, p, cookie)) != 0) {
		if (mode == SEQ_HANDLE_CREATE)
			ret = btreeSeqCreate(context, p, cookie);
	} else if (mode == SEQ_HANDLE_CREATE) {
		return DB_KEYEXIST;
	}

	if (ret != 0) {
		if (cookie->handle != NULL)
			cookie->handle->close(cookie->handle, 0);
		return (ret);
	}

	/*
	 *  We dropped the pBt->mutex after the lookup failed, grab it
	 *  again before inserting into the cache. That means there is a race
	 *  adding the handle to the cache, so we need to deal with that.
	 */
	sqlite3_mutex_enter(pBt->mutex);
	/* Check to see if someone beat us to adding the handle. */
	cache_entry =
	    sqlite3HashFind(&pBt->db_cache, cookie->name, cookie->name_len);
	if (CACHE_ENTRY_VALID(cache_entry)) {
		cookie->handle->close(cookie->handle, 0);
		cookie->handle = (DB_SEQUENCE *)cache_entry->dbp;
		goto err;
	}
	/* Add the new handle to the cache. */
	if ((cache_entry =
	    (CACHED_DB *)sqlite3_malloc(sizeof(CACHED_DB))) == NULL) {
		btreeSeqError(context, SQLITE_NOMEM,
		    "Memory allocation failure during sequence create.");
		ret = SQLITE_NOMEM;
		goto err;
	}
	memset(cache_entry, 0, sizeof(CACHED_DB));
	if ((cache_entry->cookie =
	    (SEQ_COOKIE *)sqlite3_malloc(sizeof(SEQ_COOKIE))) == NULL) {
		btreeSeqError(context, SQLITE_NOMEM,
		    "Memory allocation failure during sequence create.");
		ret = SQLITE_NOMEM;
		goto err;
	}
	sqlite3_snprintf(
	    sizeof(cache_entry->key), cache_entry->key, cookie->name);
	/*
	 * Hack alert!
	 * We're assigning the sequence to the DB pointer in the cached entry
	 * it makes everything simpler, since much of the code in btree.c is
	 * coded assuming dbp will be populated. It just means we need to be a
	 * bit careful using DB handles from the cache now.
	 */
	cache_entry->dbp = (DB *)cookie->handle;
	cache_entry->is_sequence = 1;
	memcpy(cache_entry->cookie, cookie, sizeof(SEQ_COOKIE));
	stale_db = sqlite3HashInsert(&pBt->db_cache, cache_entry->key,
	    cookie->name_len, cache_entry);
	if (stale_db) {
		sqlite3_free(stale_db);
		/*
		 * Hash table out of memory when returned pointer is
		 * same as the original value pointer.
		 */
		if (stale_db == cache_entry) {
			btreeSeqError(context, SQLITE_NOMEM, MSG_MALLOC_FAIL);
			ret = SQLITE_NOMEM;
			goto err;
		}
	}

err:	sqlite3_mutex_leave(pBt->mutex);
	if (ret != 0 && cache_entry != NULL) {
		if (cache_entry->cookie != NULL)
			sqlite3_free(cache_entry->cookie);
		sqlite3_free(cache_entry);
	}
	return (ret);
}

static int btreeSeqRemoveHandle(
    sqlite3_context *context, Btree *p, CACHED_DB *cache_entry)
{
	BtShared *pBt;
	DB_SEQUENCE *seq;
	DBT key;
	SEQ_COOKIE cookie;
	int ret;

	pBt = p->pBt;
	memcpy(&cookie, cache_entry->cookie, sizeof(cookie));

	/* Remove the entry from the hash table. */
	sqlite3HashInsert(&pBt->db_cache, cookie.name, cookie.name_len, NULL);

	if (cookie.cache != 0) {
		seq = (DB_SEQUENCE *)cache_entry->dbp;
		seq->remove(seq, p->savepoint_txn, 0);
	}

	/* Remove the cookie entry from the metadata database. */
	memset(&key, 0, sizeof(key));
	key.data = cookie.name;
	key.size = cookie.name_len;
	if ((ret = pBt->metadb->del(pBt->metadb, p->savepoint_txn, &key, 0))
	    != 0 && ret != DB_NOTFOUND) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Sequence remove incomplete. Couldn't delete metadata."
		    "Error %s.", db_strerror(ret));
	}
	if (cache_entry->cookie != NULL)
		sqlite3_free(cache_entry->cookie);
	sqlite3_free(cache_entry);
	return (ret == 0 ? SQLITE_OK : dberr2sqlite(ret, NULL));
}

static int btreeSeqOpen(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie)
{
	BtShared *pBt;
	DBT key;
	DB_SEQUENCE *seq;
	char seq_key[BT_MAX_SEQ_NAME];
	int ret, seq_len;
	u_int32_t flags;

	pBt = p->pBt;

	if ((ret = btreeSeqGetCookie(context, p, cookie, 0)) != 0)
		return (ret);

	if (cookie->cache != 0) {
		if ((ret = db_sequence_create(&seq, pBt->metadb, 0)) != 0)
			return ret;

		seq->set_cachesize(seq, cookie->cache);
		flags = 0;
#ifdef BDBSQL_SINGLE_THREAD
		flags |= DB_THREAD;
#endif
		sqlite3_snprintf(
		    BT_MAX_SEQ_NAME, seq_key, "%s_db", cookie->name);
		seq_len = (int)strlen(seq_key);
		memset(&key, 0, sizeof(key));
		key.data = seq_key;
		key.size = key.ulen = seq_len;
		key.flags = DB_DBT_USERMEM;

		if ((ret = seq->open(
		    seq, NULL, &key, flags)) != 0) {
			(void)seq->close(seq, 0);
			return ret;
		}
		cookie->handle = seq;
	}

	return (0);
}

static int btreeSeqCreate(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie)
{
	BtShared *pBt;
	DBT key;
	DB_SEQUENCE *seq;
	char seq_key[BT_MAX_SEQ_NAME];
	int ret, seq_len;
	u_int32_t flags;

	pBt = p->pBt;

	if (cookie->cache != 0) {
		if ((ret = db_sequence_create(&seq, pBt->metadb, 0)) != 0)
			return ret;

		if (cookie->cache > 0)
			seq->set_cachesize(seq, cookie->cache);
		if (cookie->decrementing != 0)
			seq->set_flags(seq, DB_SEQ_DEC);

		/* Always set min_val and max_val */
		if ((ret =
		    seq->set_range(seq, cookie->min_val, cookie->max_val)) != 0)
			return ret;
		if ((ret = seq->initial_value(seq, cookie->start_val)) != 0)
			return ret;

		flags = DB_CREATE;
#ifndef BDBSQL_SINGLE_THREAD
		flags |= DB_THREAD;
#endif
		sqlite3_snprintf(
		    BT_MAX_SEQ_NAME, seq_key, "%s_db", cookie->name);
		seq_len = (int)strlen(seq_key);
		memset(&key, 0, sizeof(key));
		key.data = seq_key;
		key.size = key.ulen = seq_len;
		key.flags = DB_DBT_USERMEM;

		if ((ret = seq->open(seq, p->savepoint_txn, &key, flags)) != 0)
			return ret;

		cookie->handle = seq;
	} else {
		cookie->handle = NULL;
		cookie->val = cookie->start_val;
		if ((cookie->decrementing &&
		    cookie->start_val < cookie->min_val) ||
		    (!cookie->decrementing &&
		    cookie->start_val > cookie->max_val) ||
		    cookie->min_val == cookie->max_val)
			return EINVAL;
	}

	if ((ret = btreeSeqPutCookie(context, p, cookie, DB_NOOVERWRITE)) != 0)
		return (ret);

	return (0);
}

static int btreeSeqExists(sqlite3_context *context, Btree *p, const char *name)
{
	DBT key, data;
	DB *dbp;

	dbp = p->pBt->metadb;

	/* If a cookie exists, then the DB exists. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = (void *)name;
	key.size = key.ulen = (u_int32_t)strlen(name);
	key.flags = DB_DBT_USERMEM;
	data.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;

	if (dbp->get(dbp, p->family_txn, &key, &data, 0) == DB_NOTFOUND)
		return (0);
	return (1);
}

/* Retrieve the cookie value from the metadata table. */
static int btreeSeqGetCookie(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie, u_int32_t flags) {
	BtShared *pBt;
	DBT cookie_key, cookie_data;
	int rc, ret;

	pBt = p->pBt;
	/* Ensure a transaction has been started. */
	if (flags == DB_RMW && cookie->cache == 0 &&
	    (rc = btreeSeqStartTransaction(context, p, 1)) != SQLITE_OK) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Could not begin transaction for update.");
		return rc;
	}
	/* Retrieve the cookie value from the metadata table. */
	memset(&cookie_key, 0, sizeof(cookie_key));
	memset(&cookie_data, 0, sizeof(cookie_data));

	cookie_key.data = cookie->name;
	cookie_key.size = cookie_key.ulen = cookie->name_len;
	cookie_key.flags = cookie_data.flags = DB_DBT_USERMEM;

	cookie_data.data = cookie;
	cookie_data.ulen = sizeof(SEQ_COOKIE);

	/*
	 * Use the family txn for this get. Nothing will need to be rolled
	 * back, and it allows us to be consistent for caching and non caching
	 * sequences.
	 */
	if ((ret = pBt->metadb->get(pBt->metadb,
	    flags == DB_RMW ? p->savepoint_txn : p->family_txn,
	    &cookie_key, &cookie_data, flags)) != 0)
		return ret;
	return (0);
}

/* Create or update the cookie entry in the metadata db. */
static int btreeSeqPutCookie(
    sqlite3_context *context, Btree *p, SEQ_COOKIE *cookie, u_int32_t flags) {
	BtShared *pBt;
	DBT cookie_key, cookie_data;
	int rc, ret;
	sqlite3 *db;

	pBt = p->pBt;
	db = sqlite3_context_db_handle(context);
	/* Ensure a transaction has been started. */
	if (cookie->cache == 0 &&
	    (rc = btreeSeqStartTransaction(context, p, 1)) != SQLITE_OK) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Could not begin transaction for create.");
		return rc;
	}
	/* Create the matching cookie entry in the metadata db. */
	memset(&cookie_key, 0, sizeof(cookie_key));
	memset(&cookie_data, 0, sizeof(cookie_data));

	cookie_key.data = cookie->name;
	cookie_key.size = cookie_key.ulen = cookie->name_len;
	cookie_key.flags = cookie_data.flags = DB_DBT_USERMEM;

	cookie_data.data = cookie;
	cookie_data.size = cookie_data.ulen = sizeof(SEQ_COOKIE);
	if ((ret = pBt->metadb->put(pBt->metadb, p->savepoint_txn,
	    &cookie_key, &cookie_data, flags)) != 0)
		return ret;
	return (0);
}

/*
 * SQLite manages explicit transactions by setting a flag when a BEGIN; is
 * issued, then starting an actual transaction in the btree layer when the
 * first operation happens (a read txn if it's a read op, a write txn if write)
 * Then each statement will be contained in a sub-transaction. Since sequences
 * are implemented using a custom function, we need to emulate that
 * functionality. So there are three cases here:
 * - Not in an explicit transaction - start a statement, since we might do
 *   write operations, and thus we need a valid statement_txn.
 * - In an explicit transaction, and the first statement. Start a txn and a
     statement txn.
 * - In an explicit transaction and not the first statemetn. Start a statement
 *   transaction.
 *
 * The SQLite vdbe will take care of closing the statement transaction for us,
 * so we don't need to worry about that.
 *
 * Caching sequences can't be transactionally protected, so it's a no-op in
 * that case (and this function should not be called).
 *
 * It's safe to call this method multiple times since both
 * btreeBeginTransInternal and sqlite3BtreeBeginStmt are no-ops on subsequent
 * calls.
 */
static int btreeSeqStartTransaction(
    sqlite3_context *context, Btree *p, int is_write)
{
	sqlite3 *db;
	Vdbe *vdbe;
	int rc;

	db = sqlite3_context_db_handle(context);
	/*
	 * TODO: This is actually a linked list of VDBEs, not necessarily
	 *       the vdbe we want. I can't see a way to get the correct
	 *       vdbe handle.
	 *       It's possible that there is only one VDBE handle in DB, since
	 *       we use a shared cache.
	 */
	vdbe = db->pVdbe;

	if (!sqlite3BtreeIsInTrans(p) &&
	    (rc = btreeBeginTransInternal(p, 1)) != SQLITE_OK) {
		btreeSeqError(context, SQLITE_ERROR,
		    "Could not begin transaction.");
		return (rc);
	}

	/*
	 * TODO: Do we need logic bumping the VDBE statement count here?
	 *       vdbe.c:OP_Transaction does, but adding it here causes an
	 *       assert failure. It should be OK, because this code is only
	 *       really relevant in the case where there is a single statement.
	 */
	rc = sqlite3BtreeBeginStmt(p, vdbe->iStatement);
	return (rc);
}
