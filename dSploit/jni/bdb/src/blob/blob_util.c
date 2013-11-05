/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013 Oracle and/or its affiliates.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_verify.h"
#include "dbinc/db_am.h"
#include "dbinc/blob.h"
#include "dbinc/fop.h"
#include "dbinc/mp.h"
#include "dbinc_auto/sequence_ext.h"

static int __blob_open_meta_db __P((
    DB *, DB_TXN *, DB **, DB_SEQUENCE **, int));
static int __blob_clean_dir __P((DB *, DB_TXN *, const char *, int));
static int __blob_copy_dir __P((DB *, const char *, const char *));

#define	BLOB_ID_KEY "blob_id"
#define	BLOB_SEQ_DB_NAME "blob_id_seq"
#define	BLOB_DIR_ID_KEY "blob_dir_id"
#define	BLOB_DIR_SEQ_DB_NAME "blob_dir_id_seq"

/*
 * __blob_make_sub_dir --
 *	Create the name of the subdirectory in the blob directory
 * for the given database file and subdatabase ids.
 *
 * PUBLIC: int __blob_make_sub_dir __P((ENV *, char **, uintmax_t, uintmax_t));
 */
int
__blob_make_sub_dir(env, blob_sub_dir, file_id, db_id)
	ENV *env;
	char **blob_sub_dir;
	uintmax_t file_id;
	uintmax_t db_id;
{
	char fname[MAX_BLOB_PATH_SZ], dname[MAX_BLOB_PATH_SZ];
	int ret;
	size_t len;

	*blob_sub_dir = NULL;
	memset(fname, 0, MAX_BLOB_PATH_SZ);
	memset(dname, 0, MAX_BLOB_PATH_SZ);

	if (db_id == 0 && file_id == 0)
		return (0);

	/* The master db has no subdb id. */
	if (db_id != 0)
		sprintf(dname,
		    "%s%llu", BLOB_DIR_PREFIX, (unsigned long long)db_id);
	sprintf(fname, "%s%llu", BLOB_DIR_PREFIX,
	    (unsigned long long)file_id);

	len = strlen(fname) + (db_id ? strlen(dname) : 0) + 3;
	if ((ret = __os_malloc(env, len, blob_sub_dir)) != 0)
		goto err;
	if (db_id != 0)
		sprintf(*blob_sub_dir, "%s%c%s%c", fname,
		    PATH_SEPARATOR[0], dname, PATH_SEPARATOR[0]);
	else
		sprintf(*blob_sub_dir, "%s%c", fname, PATH_SEPARATOR[0]);

	return (0);

err:	if (*blob_sub_dir != NULL)
		__os_free(env, *blob_sub_dir);

	return (ret);
}

/*
 * __blob_make_meta_fname --
 *	Construct a (usually partial) path name of a blob metadata data file.
 *	It usually is relative to the environment home directory; only when a
 *	blob directory is configured and is an absolute path does this make a
 *	full path.
 *
 *	When dbp is set it constructs the blob metadata filename for that db;
 *	otherwise it constructs the environment-wide directory id filename.
 *
 * PUBLIC: int __blob_make_meta_fname __P((ENV *, DB *, char **));
 */
int
__blob_make_meta_fname(env, dbp, meta_fname)
	ENV *env;
	DB *dbp;
	char **meta_fname;
{
	char *fname, *sub_dir;
	int ret;
	size_t len;

	fname = NULL;
	len = strlen(BLOB_META_FILE_NAME) + 1;
	if (dbp == NULL) {
		sub_dir = "";
	} else {
		sub_dir = dbp->blob_sub_dir;
		DB_ASSERT(env, sub_dir != NULL);
		len += strlen(sub_dir);
	}
	if ((ret = __os_malloc(env, len, &fname)) != 0)
		goto err;

	snprintf(fname, len, "%s%s", sub_dir, BLOB_META_FILE_NAME);
	*meta_fname = fname;
	return (0);
err:
	if (fname != NULL)
		__os_free(env, fname);
	return (ret);
}

/*
 * __blob_get_dir --
 *	Get the root directory of this database's blob files.
 *
 * PUBLIC: int __blob_get_dir __P((DB *, char **));
 */
int
__blob_get_dir(dbp, dirp)
	DB *dbp;
	char **dirp;
{
	char *blob_dir;
	int ret;

	*dirp = NULL;

	if (dbp->blob_sub_dir == NULL)
		return (0);

	/* Get the path of the blob directory for this database. */
	if ((ret = __db_appname(dbp->env,
	    DB_APP_BLOB, dbp->blob_sub_dir, NULL, &blob_dir)) != 0)
		goto err;

	*dirp = blob_dir;
	return (0);

err:	if (blob_dir != NULL)
		__os_free(dbp->env, blob_dir);

	return (ret);
}

/*
 * __blob_open_meta_db --
 *	Open or create a blob meta database. This can be either
 *	the environment-wide db used to generate blob directory ids (__db1), or
 *	the per-db db used to generate blob ids (__db.bl001).
 */
static int
__blob_open_meta_db(dbp, txn, meta_db, seq, file)
	DB *dbp;
	DB_TXN *txn;
	DB **meta_db;
	DB_SEQUENCE **seq;
	int file;
{
#ifdef HAVE_64BIT_TYPES
	ENV *env;
	DB *blob_meta_db;
	DBT key;
	DB_SEQUENCE *blob_seq;
	DB_THREAD_INFO *ip;
	DB_TXN *local_txn;
	char *fullname, *fname, *dname, *path;
	int free_paths, ret;
	u_int32_t flags, oflags;

	flags = 0;
	fullname = fname = NULL;
	blob_meta_db = NULL;
	blob_seq = NULL;
	local_txn = NULL;
	env = dbp->env;
	free_paths = 0;
	memset(&key, 0, sizeof(DBT));

	/*
	 * Get the directory of the database, the meta db file name,
	 * and the sub-db name.
	 * file: blob directory/meta-file-name
	 * else: blob directory/per-db-blobdir/meta-file-name
	 */
	if (file) {
		key.data = BLOB_DIR_ID_KEY;
		key.size = (u_int32_t)strlen(BLOB_DIR_ID_KEY);
		dname = BLOB_DIR_SEQ_DB_NAME;
		fname = BLOB_META_FILE_NAME;
	} else {
		key.data = BLOB_ID_KEY;
		key.size = (u_int32_t)strlen(BLOB_ID_KEY);
		dname = BLOB_SEQ_DB_NAME;
		if ((ret = __blob_make_meta_fname(env,
		    file ? NULL : dbp, &fname)) < 0)
			goto err;
		free_paths = 1;
		if ((ret = __db_get_open_flags(dbp, &oflags)) != 0)
			goto err;
		if (oflags & DB_THREAD)
			LF_SET(DB_THREAD);
	}

	if ((ret = __db_appname(env, DB_APP_BLOB, fname, NULL, &fullname)) != 0)
		goto err;

	path = fullname;
#ifdef DB_WIN32
	/*
	 * Absolute paths on windows can result in it creating a "C" or "D"
	 * directory in the working directory.
	 */
	if (__os_abspath(path))
		path += 2;
#endif
	/*
	 * Create the blob, database file, and database name directories. The
	 * mkdir isn't logged, so __fop_create_recover needs to do this as well.
	 */
	if (__os_exists(env, fullname, NULL) != 0 &&
	    (ret = __db_mkpath(env, path)) != 0)
		goto err;

	/* Create the blob meta db if it doesn't exist. */
	LF_SET(DB_CREATE);
	if ((ret = __db_create_internal(&blob_meta_db, env, 0)) != 0)
		goto err;

	/* Disable blobs in the blob meta databases themselves. */
	if ((ret = __db_set_blob_threshold(blob_meta_db, 0, 0)) != 0)
		goto err;

	ENV_GET_THREAD_INFO(env, ip);
	if (IS_REAL_TXN(txn) &&
	    (ret = __db_txn_auto_init(env, ip, &local_txn)) != 0)
		goto err;
	if ((ret = __db_open(blob_meta_db, ip, local_txn, fname, dname,
	    DB_BTREE, flags | DB_INTERNAL_BLOB_DB, 0, PGNO_BASE_MD)) != 0)
		goto err;

	/* Open the sequence that holds the blob ids. */
	if ((ret = db_sequence_create(&blob_seq, blob_meta_db, 0)) != 0)
		goto err;

	/* No-op if already initialized, 0 is an invalid value for blob ids. */
	if ((ret = __seq_initial_value(blob_seq, 1)) != 0)
		goto err;
	if ((ret = __seq_open(blob_seq, local_txn, &key, flags)) != 0)
		goto err;

	if (local_txn != NULL &&
	    (ret = __db_txn_auto_resolve(env, local_txn, 0, ret)) != 0) {
		local_txn = NULL;
		goto err;
	}
	__os_free(env, fullname);
	if (free_paths)
		__os_free(env, fname);
	*meta_db = blob_meta_db;
	*seq = blob_seq;
	return (0);

err:
	if (fullname)
		__os_free(env, fullname);
	if (fname != NULL && free_paths)
		__os_free(env, fname);
	if (local_txn != NULL)
		(void)__db_txn_auto_resolve(env, local_txn, 0, ret);
	if (blob_seq != NULL)
		__seq_close(blob_seq, 0);
	if (blob_meta_db != NULL)
		__db_close(blob_meta_db, NULL, 0);
	return (ret);

#else /*HAVE_64BIT_TYPES*/
	__db_errx(dbp->env, DB_STR("0217",
	    "library build did not include support for blobs"));
	return (DB_OPNOTSUP);
#endif
}

/*
 * __blob_generate_dir_ids --
 *
 * Generate the unique ids used to create a blob directory for the database.
 * Only one argument is needed.  Files with one database only need the
 * file id.  The master database only needs the file id, and
 * subdatabases inherit the file id from the master, so they only need the
 * subdatabase id.
 *
 * PUBLIC: int __blob_generate_dir_ids
 * PUBLIC:	__P((DB *, DB_TXN *, uintmax_t *));
 */
int
__blob_generate_dir_ids(dbp, txn, id)
	DB *dbp;
	DB_TXN *txn;
	uintmax_t *id;
{
	DB *blob_meta_db;
	DB_SEQUENCE *blob_seq;
	int ret;
	u_int32_t flags;
	db_seq_t val;

#ifdef HAVE_64BIT_TYPES
	flags = 0;
	blob_meta_db = NULL;
	blob_seq = NULL;

	if ((ret = __blob_open_meta_db(
	    dbp, txn, &blob_meta_db, &blob_seq, 1)) != 0)
		goto err;

	if (IS_REAL_TXN(txn))
		LF_SET(DB_AUTO_COMMIT | DB_TXN_NOSYNC);

	/*
	 * __seq_get returns a signed 64 bit integer instead of uint, but
	 * it will throw an error before it rolls over, so there is no
	 * rollover danger here.
	 */
	DB_ASSERT(dbp->env, id != NULL);
	if (*id == 0) {
		if ((ret = __seq_get(blob_seq, 0, 1, &val, flags)) != 0)
			goto err;
		*id = (uintmax_t)val;
	}

err:	if (blob_seq != NULL)
		__seq_close(blob_seq, 0);
	if (blob_meta_db != NULL)
		__db_close(blob_meta_db, NULL, 0);
	return (ret);
#else /*HAVE_64BIT_TYPES*/
	COMPQUIET(dbp, NULL);
	COMPQUIET(txn, NULL);
	__db_errx(dbp->env, DB_STR("0218",
	    "library build did not include support for blobs"));
	return (DB_OPNOTSUP);
#endif
}

/*
 * __blob_generate_id --
 * Generate a new blob ID.
 *
 * PUBLIC: int __blob_generate_id __P((DB *, DB_TXN *, uintmax_t *));
 */
int
__blob_generate_id(dbp, txn, blob_id)
	DB *dbp;
	DB_TXN *txn;
	uintmax_t *blob_id;
{
#ifdef HAVE_64BIT_TYPES
	int ret;
	u_int32_t flags;
	db_seq_t val;

	flags = 0;

	if (dbp->blob_seq == NULL) {
		if ((ret = __blob_open_meta_db(dbp, txn,
		    &dbp->blob_meta_db, &dbp->blob_seq, 0)) != 0)
			goto err;
	}

	if (IS_REAL_TXN(txn))
		LF_SET(DB_AUTO_COMMIT | DB_TXN_NOSYNC);

	/*
	 * __seq_get returns a signed 64 bit integer instead of uint, but
	 * it will throw an error before it rolls over, so there is no
	 * rollover danger here.
	 */
	if ((ret = __seq_get(dbp->blob_seq, 0, 1, &val, flags)) != 0)
		goto err;
	*blob_id = (uintmax_t)val;

err:	return (ret);
#else /*HAVE_64BIT_TYPES*/
	COMPQUIET(blob_id, NULL);
	__db_errx(dbp->env, DB_STR("0219",
	    "library build did not include support for blobs"));
	return (DB_OPNOTSUP);
#endif
}

/*
 * __blob_id_to_path --
 * Generate the file name and blob specific part of the path for a particular
 * blob_id. The __db_appname API is used to generate a fully qualified path.
 * The caller must deallocate the path.
 *
 * PUBLIC: int __blob_id_to_path __P((ENV *, const char *, uintmax_t, char **));
 */
int
__blob_id_to_path(env, blob_sub_dir, blob_id, ppath)
	ENV *env;
	const char *blob_sub_dir;
	uintmax_t blob_id;
	char **ppath;
{
	char *path, *tmp_path;
	int depth, i, name_len, ret;
	size_t len;
	uintmax_t factor, tmp;

	name_len = 0;
	path = tmp_path = *ppath = NULL;

	len = MAX_BLOB_PATH_SZ + strlen(blob_sub_dir) + 1;
	if ((ret = __os_malloc(env, len, &path)) != 0)
		goto err;

	memset(path, 0, len);
	name_len += sprintf(path, "%s", blob_sub_dir);

	/* Populate the directories. */
	factor = 1;
	for (depth = 0, tmp = blob_id/BLOB_DIR_ELEMS;
	    tmp != 0; tmp = tmp/BLOB_DIR_ELEMS, depth++)
		factor *= BLOB_DIR_ELEMS;

	for (i = depth; i > 0; i--) {
		tmp = (blob_id / factor) % BLOB_DIR_ELEMS;
		factor /= BLOB_DIR_ELEMS;
		name_len += sprintf(path + name_len,
			"%03llu%c", (unsigned long long)tmp, PATH_SEPARATOR[0]);
	}

	/*
	 * Populate the file name. Ensure there are 3 digits for each directory
	 * level (even if they are 0).
	 */
	sprintf(path + name_len, "%s%0*llu",
	    BLOB_FILE_PREFIX, (depth + 1) * 3, (unsigned long long)blob_id);

	/* If this is the first file in the directory, ensure it exists. */
	if (blob_id % BLOB_DIR_ELEMS == 0 && depth > 0) {
		__db_appname(env, DB_APP_BLOB, path, NULL, &tmp_path);

		if ((ret = __db_mkpath(env, tmp_path)) != 0) {
			__db_errx(env, DB_STR("0221",
			    "Error creating blob directory."));
			ret = EINVAL;
			goto err;
		}
		__os_free(env, tmp_path);
	}

	*ppath = path;
	return (0);

err:
	if (tmp_path != NULL)
		__os_free(env, tmp_path);
	if (path != NULL)
		__os_free(env, path);

	return (ret);
}

/*
 * __blob_salvage --
 *
 * Print a blob file during salvage.  The function assumes the DBT already has
 * a buffer large enough to hold "size" bytes.
 *
 * PUBLIC: int __blob_salvage __P((ENV *, uintmax_t, off_t, size_t,
 * PUBLIC:	uintmax_t, uintmax_t, DBT *));
 */
int
__blob_salvage(env, blob_id, offset, size, file_id, sdb_id, dbt)
	ENV *env;
	uintmax_t blob_id;
	off_t offset;
	size_t size;
	uintmax_t file_id;
	uintmax_t sdb_id;
	DBT *dbt;
{
	DB_FH *fhp;
	char *blob_sub_dir, *dir, *path;
	int ret;
	size_t bytes;

	blob_sub_dir = dir = path = NULL;
	fhp = NULL;

	if (file_id == 0 && sdb_id == 0) {
		ret = ENOENT;
		goto err;
	}

	if ((ret = __blob_make_sub_dir(
	    env, &blob_sub_dir, file_id, sdb_id)) != 0)
		goto err;

	if ((ret = __blob_id_to_path(env, blob_sub_dir, blob_id, &dir)) != 0)
		goto err;

	if ((ret = __db_appname(env, DB_APP_BLOB, dir, NULL, &path)) != 0)
		goto err;

	if ((ret = __os_open(env, path, 0, DB_OSO_RDONLY, 0, &fhp)) != 0)
		goto err;

	if ((ret = __os_seek(env, fhp, 0, 0, offset)) != 0)
		goto err;

	if ((ret = __os_read(env, fhp, dbt->data, size, &bytes)) != 0)
		goto err;

	dbt->size = (u_int32_t)bytes;
	if (bytes != size)
		ret = EIO;

err:	if (fhp != NULL)
		__os_closehandle(env, fhp);
	if (dir != NULL)
		__os_free(env, dir);
	if (path != NULL)
		__os_free(env, path);
	if (blob_sub_dir != NULL)
		__os_free(env, blob_sub_dir);
	return (ret);
}

/*
 * __blob_vrfy --
 *
 * Checks that a blob file for the given blob id exists, and is the given size.
 *
 * PUBLIC: int __blob_vrfy __P((ENV *, uintmax_t, off_t,
 * PUBLIC:	uintmax_t, uintmax_t, db_pgno_t, u_int32_t));
 */
int
__blob_vrfy(env, blob_id, blob_size, file_id, sdb_id, pgno, flags)
	ENV *env;
	uintmax_t blob_id;
	off_t blob_size;
	uintmax_t file_id;
	uintmax_t sdb_id;
	db_pgno_t pgno;
	u_int32_t flags;
{
	DB_FH *fhp;
	char *blob_sub_dir, *dir, *path;
	int isdir, ret, t_ret;
	off_t actual_size;
	u_int32_t mbytes, bytes;

	blob_sub_dir = dir = path = NULL;
	fhp = NULL;
	isdir = 0;
	ret = DB_VERIFY_BAD;

	if ((t_ret = __blob_make_sub_dir(
	    env, &blob_sub_dir, file_id, sdb_id)) != 0)
		goto err;

	if ((t_ret = __blob_id_to_path(
	    env, blob_sub_dir, blob_id, &dir)) != 0) {
		EPRINT((env, DB_STR_A("0222",
		    "Page %lu: Error getting path to blob file for %llu",
		    "%lu %llu"), (u_long)pgno, (unsigned long long)blob_id));
		goto err;
	}
	if ((t_ret = __db_appname(env, DB_APP_BLOB, dir, NULL, &path)) != 0) {
		EPRINT((env, DB_STR_A("0223",
		    "Page %lu: Error getting path to blob file for %llu",
		    "%lu %llu"), (u_long)pgno, (unsigned long long)blob_id));
		goto err;
	}
	if ((t_ret = __os_exists(env, path, &isdir)) != 0 || isdir != 0) {
		EPRINT((env, DB_STR_A("0224",
		    "Page %lu: blob file does not exist at %s",
		    "%lu %s"), (u_long)pgno, path));
		goto err;
	}
	if ((t_ret = __os_open(env, path, 0, DB_OSO_RDONLY, 0, &fhp)) != 0) {
		EPRINT((env, DB_STR_A("0225",
		    "Page %lu: Error opening blob file at %s",
		    "%lu %s"), (u_long)pgno, path));
		goto err;
	}
	if ((t_ret = __os_ioinfo(
	    env, path, fhp, &mbytes, &bytes, NULL)) != 0) {
		EPRINT((env, DB_STR_A("0226",
		    "Page %lu: Error getting blob file size at %s",
		    "%lu %s"), (u_long)pgno, path));
		goto err;
	}

	actual_size = ((off_t)mbytes * (off_t)MEGABYTE) + bytes;
	if (blob_size != actual_size) {
		EPRINT((env, DB_STR_A("0227",
"Page %lu: blob file size does not match size in database record: %llu %llu",
		    "%lu %llu %llu"), (u_long)pgno,
		    (unsigned long long)actual_size,
		    (unsigned long long)blob_size));
		goto err;
	}

	ret = 0;

err:	if (fhp != NULL)
		__os_closehandle(env, fhp);
	if (dir != NULL)
		__os_free(env, dir);
	if (path != NULL)
		__os_free(env, path);
	if (blob_sub_dir != NULL)
		__os_free(env, blob_sub_dir);
	return (ret);
}

/*
 * __blob_del_all --
 *
 * Deletes all the blob files and meta databases in a database's blob
 * directory.  Does not delete the directories.
 *
 * PUBLIC: int __blob_del_all __P((DB *, DB_TXN *, int));
 */
int
__blob_del_all(dbp, txn, istruncate)
	DB *dbp;
	DB_TXN *txn;
	int istruncate;
{
#ifdef HAVE_64BIT_TYPES
	ENV *env;
	char *path;
	int close, isdir, ret;

	env = dbp->env;
	path = NULL;
	ret = 0;
	close = 0;

	if (dbp->blob_sub_dir == NULL) {
		if ((ret = __blob_make_sub_dir(env, &dbp->blob_sub_dir,
		    dbp->blob_file_id, dbp->blob_sdb_id)) != 0)
			goto err;
	}

	/* Do nothing if blobs are not enabled. */
	if (dbp->blob_sub_dir == NULL || dbp->blob_threshold == 0)
		goto err;

	if ((ret = __blob_get_dir(dbp, &path)) != 0)
		goto err;

	/* Close the blob meta data databases, they are about to be deleted. */
	if (!istruncate) {
		if (dbp->blob_seq != NULL) {
			__seq_close(dbp->blob_seq, 0);
			dbp->blob_seq = NULL;
		}
		if (dbp->blob_meta_db != NULL) {
			__db_close(dbp->blob_meta_db, NULL, 0);
			dbp->blob_meta_db = NULL;
		}
	}

	/*
	 * The blob directory may not exist if blobs were enabled,
	 * but none were created.
	 */
	if (__os_exists(env, path, &isdir) != 0)
		goto err;

	if ((ret = __blob_clean_dir(dbp, txn, path, istruncate)) != 0)
		goto err;

err:	if (path != NULL)
		__os_free(env, path);
	return (ret);

#else /*HAVE_64BIT_TYPES*/
        __db_errx(dbp->env, DB_STR("0220",
            "library build did not include support for blobs"));
        return (DB_OPNOTSUP);
#endif

}

/*
 * __blob_clean_dir --
 *
 * Delete all files in the given directory, and all files
 * in all sub-directories.  Does not remove directories.
 */
static int
__blob_clean_dir(dbp, txn, dir, istruncate)
	DB *dbp;
	DB_TXN *txn;
	const char *dir;
	int istruncate;
{
	DB *meta;
	DB_THREAD_INFO *ip;
	ENV *env;
	char **dirs, *fname, full_path[DB_MAXPATHLEN];
	int count, i, isdir, ret, t_ret;

	env = dbp->env;
	count = 0;
	dirs = NULL;
	fname = NULL;
	meta = NULL;

	/* Get a list of all files in the directory. */
	if ((ret = __os_dirlist(env, dir, 1, &dirs, &count)) != 0)
		goto err;

	for (i = 0; i < count; i++) {
		sprintf(full_path, "%s%c%s%c",
		    dir, PATH_SEPARATOR[0], dirs[i], '\0');

		if (__os_exists(env, full_path, &isdir) != 0)
			continue;

		/* If it is a directory, clean it.  Else remove the file. */
		if (isdir) {
			if ((ret = __blob_clean_dir(
			    dbp, txn, full_path, istruncate)) != 0)
				goto err;
		} else if (strcmp(dirs[i], BLOB_META_FILE_NAME) == 0 ) {
			/* Ignore the meta db when truncating. */
			if (istruncate)
				continue;
			__blob_make_meta_fname(env, dbp, &fname);
			ENV_GET_THREAD_INFO(env, ip);
			if ((ret = __db_create_internal(&meta, env, 0)) != 0)
				goto err;
			if ((ret = __db_remove_int(meta,
			    ip, txn, fname, NULL, 0)) != 0)
				goto err;
			/*
			 * Closing the local DB handle releases the transaction
			 * locks, but those have to remain until the
			 * transaction is resolved, so NULL the DB locker.
			 * See __env_dbremove_pp for more details.
			 */
			if (IS_REAL_TXN(txn))
				meta->locker = NULL;
			if ((t_ret = __db_close(
			    meta, NULL, DB_NOSYNC)) != 0 && ret == 0)
				ret = t_ret;
			meta = NULL;
			if (ret != 0)
				goto err;
		} else {
			if (!IS_REAL_TXN(txn))
				ret = __os_unlink(env, full_path, 0);
			else {
				ret = __fop_remove(env, txn, NULL,
				    strstr(full_path, dbp->blob_sub_dir),
				    NULL, DB_APP_BLOB, 0);
			}
			if (ret != 0)
				goto err;
		}
	}
err:	if (meta != NULL) {
		if ((t_ret = __db_close(
		    meta, NULL, DB_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
	}
	if (fname != NULL)
		__os_free(env, fname);

	return (ret);
}

/*
 * __blob_copy_all --
 *	Copy all files in the blob directory.
 *
 * PUBLIC: int __blob_copy_all __P((DB*, const char *));
 */
int __blob_copy_all(dbp, target)
	DB *dbp;
	const char *target;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	char *fullname, new_target[DB_MAXPATHLEN], *metafname, *path;
	int ret;

	fullname = NULL;
	env = dbp->env;
	metafname = NULL;
	path = NULL;
	ret = 0;

	/* Do nothing if blobs are not enabled. */
	if (dbp->blob_sub_dir == NULL || dbp->blob_threshold == 0)
		return (0);

	/* Create the directory structure in the target directory. */
	sprintf(new_target, "%s%c%s%c%c", target,
	    PATH_SEPARATOR[0], BLOB_DEFAULT_DIR, PATH_SEPARATOR[0], '\0');
	path = new_target;
#ifdef DB_WIN32
	/*
	 * Absolute paths on windows can result in it creating a "C" or "D"
	 * directory in the working directory.
	 */
	if (__os_abspath(path))
		path += 2;
#endif
	if ((ret = __db_mkpath(env, path)) != 0)
		goto err;

	/* Copy the directory id database. */
	if ((ret = __blob_make_meta_fname(env, dbp, &metafname)) != 0)
		goto err;
	ret = __db_appname(env, DB_APP_BLOB, metafname, NULL, &fullname);
	__os_free(env, metafname);
	metafname = NULL;
	if (ret != 0)
		goto err;
	ENV_GET_THREAD_INFO(env, ip);
	if ((ret = __db_dbbackup(
	    dbp->dbenv, ip, fullname, new_target, 0, 0, NULL)) != 0)
		    goto err;

	if ((ret = __blob_get_dir(dbp, &path)) != 0)
		goto err;

	/*
	 * The blob directory may not exist if blobs were enabled,
	 * but none were created.
	 */
	if (__os_exists(env, path, NULL) != 0)
		goto err;

	sprintf(new_target, "%s%c%s%c%s%c",
	    target, PATH_SEPARATOR[0], BLOB_DEFAULT_DIR,
	    PATH_SEPARATOR[0], dbp->blob_sub_dir, '\0');
	if ((ret = __blob_copy_dir(dbp, path, new_target)) != 0)
		goto err;

err:	if (path != NULL)
		__os_free(env, path);
	if (fullname != NULL)
		__os_free(env, fullname);
	return (ret);
}

/*
 * __blob_copy_dir --
 *	Copy all files in the given directory, and all files
 *	in all sub-directories.
 */
static int
__blob_copy_dir(dbp, dir, target)
	DB *dbp;
	const char *dir;
	const char *target;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	char **dirs, full_path[DB_MAXPATHLEN], new_target[DB_MAXPATHLEN];
	int count, i, isdir, ret;

	env = dbp->env;
	count = 0;
	dirs = NULL;

	/* Create the directory sturcture in the target directory. */
	if ((ret = __db_mkpath(env, target)) != 0)
		goto err;

	ENV_GET_THREAD_INFO(env, ip);
	/* Get a list of all files in the directory. */
	if ((ret = __os_dirlist(env, dir, 1, &dirs, &count)) != 0)
		goto err;

	for (i = 0; i < count; i++) {
		sprintf(full_path, "%s%c%s%c",
		    dir, PATH_SEPARATOR[0], dirs[i], '\0');

		if (__os_exists(env, full_path, &isdir) != 0)
			continue;

		/*
		 * If it is a directory, copy the files in it.
		 * Else if it is the meta database, call __db_dbbackup, else
		 * copy the file.
		 */
		if (isdir) {
			sprintf(new_target,
			    "%s%c%s%c%c", target, PATH_SEPARATOR[0],
			    dirs[i], PATH_SEPARATOR[0], '\0');
			if ((ret = __blob_copy_dir(
			    dbp, full_path, new_target)) != 0)
				goto err;
		} else {
			if (strcmp(dirs[i], BLOB_META_FILE_NAME) == 0) {
				sprintf(full_path, "%s%c%s%c",
				    dbp->blob_sub_dir,
				    PATH_SEPARATOR[0], dirs[i], '\0');
				if ((ret = __db_dbbackup(dbp->dbenv, ip,
				    full_path, target, 0, 0,
				    BLOB_META_FILE_NAME)) != 0)
					goto err;
			} else {
				if ((ret = backup_data_copy(
				    dbp->dbenv, dirs[i], dir, target, 0)) != 0)
					goto err;
			}
		}
	}
err:	return (ret);
}
