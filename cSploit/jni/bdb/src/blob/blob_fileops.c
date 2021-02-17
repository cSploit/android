/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/blob.h"
#include "dbinc/fop.h"
#include "dbinc/mp.h"

/*
 * __blob_file_create --
 *	Blobs are orginaized in a directory sturcture consisting of
 *	<DB_HOME>/__db_bl/<blob_sub_dir>/.  Below that, the blob_id
 *	is used to construct a path to the blob file, and to name
 *	the blob file.  blob_id=1 would result in __db.bl001.
 *	blob_id=12002 would result in 012/__db.bl012002.
 *
 * PUBLIC: int __blob_file_create __P
 * PUBLIC:  ((DBC *, DB_FH **, db_seq_t *));
 */
int
__blob_file_create(dbc, fhpp, blob_id)
	DBC *dbc;
	DB_FH **fhpp;
	db_seq_t *blob_id;
{
	DB  *dbp;
	DB_FH *fhp;
	ENV *env;
	int ret;
	char *ppath;
	const char *dir;

	dbp = dbc->dbp;
	env = dbp->env;
	fhp = *fhpp = NULL;
	ppath = NULL;
	dir = NULL;
	DB_ASSERT(env, !DB_IS_READONLY(dbc->dbp));

	if ((ret = __blob_generate_id(dbp, dbc->txn, blob_id)) != 0)
		goto err;

	if ((ret = __blob_id_to_path(
	    env, dbp->blob_sub_dir, *blob_id, &ppath)) != 0)
		goto err;

	if ((ret = __fop_create(env, dbc->txn,
	    &fhp, ppath, &dir, DB_APP_BLOB, env->db_mode,
	    (F_ISSET(dbc->dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0)))
	    != 0) {
		__db_errx(env, DB_STR_A("0228",
		    "Error creating blob file: %llu.", "%llu"),
		    (unsigned long long)*blob_id);
		goto err;
	}

err:	if (ppath != NULL)
		__os_free(env, ppath);
	if (ret == 0)
		*fhpp = fhp;
	return (ret);
}

/*
 * __blob_file_close --
 *
 * PUBLIC: int  __blob_file_close __P ((DBC *, DB_FH *, u_int32_t));
 */
int
__blob_file_close(dbc, fhp, flags)
	DBC *dbc;
	DB_FH *fhp;
	u_int32_t flags;
{
	ENV *env;
	int ret, t_ret;

	env = dbc->env;
	ret = t_ret = 0;
	if (fhp != NULL) {
		/* Only sync if the file was open for writing. */
		if (LF_ISSET(DB_FOP_WRITE))
			t_ret = __os_fsync(env, fhp);
		ret = __os_closehandle(env, fhp);
		if (t_ret != 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __blob_file_delete --
 *	Delete a blob file.
 *
 * PUBLIC: int __blob_file_delete __P((DBC *, db_seq_t));
 */
int
__blob_file_delete(dbc, blob_id)
	DBC *dbc;
	db_seq_t blob_id;
{
	ENV *env;
	char *blob_name, *full_path;
	int ret;

	env = dbc->dbp->env;
	blob_name = full_path = NULL;

	if ((ret = __blob_id_to_path(
	    env, dbc->dbp->blob_sub_dir, blob_id, &blob_name)) != 0) {
		__db_errx(env, DB_STR_A("0229",
		   "Failed to construct path for blob file %llu.",
		   "%llu"), (unsigned long long)blob_id);
		goto err;
	}

	/* Log the file remove event. */
	if (!IS_REAL_TXN(dbc->txn)) {
		if ((ret = __db_appname(
		    env, DB_APP_BLOB, blob_name, NULL, &full_path)) != 0)
			goto err;
		ret = __os_unlink(env, full_path, 0);
	} else {
		ret = __fop_remove(
		    env, dbc->txn, NULL, blob_name, NULL, DB_APP_BLOB, 0);
	}

	if (ret != 0) {
		__db_errx(env, DB_STR_A("0230",
		    "Failed to remove blob file while deleting: %s.",
		    "%s"), blob_name);
		goto err;
	}

err:	if (blob_name != NULL)
		__os_free(env, blob_name);
	if (full_path != NULL)
		__os_free(env, full_path);
	return (ret);
}

/*
 * __blob_file_open --
 *
 * PUBLIC: int __blob_file_open __P((DB *, DB_FH **, db_seq_t, u_int32_t));
 */
int
__blob_file_open(dbp, fhpp, blob_id, flags)
	DB *dbp;
	DB_FH **fhpp;
	db_seq_t blob_id;
	u_int32_t flags;
{
	ENV *env;
	int ret;
	u_int32_t oflags;
	char *path, *ppath, *dir;
	const char *blob_sub_dir;

	dir = NULL;
	env = dbp->env;
	*fhpp = NULL;
	ppath = path = NULL;
	oflags = 0;
	blob_sub_dir = dbp->blob_sub_dir;

	if ((ret = __blob_id_to_path(
	    env, dbp->blob_sub_dir, blob_id, &ppath)) != 0)
		goto err;

	if ((ret = __db_appname(
	    env, DB_APP_BLOB, ppath, NULL, &path)) != 0) {
		__db_errx(env, DB_STR_A("0231",
		    "Failed to get path to blob file: %llu.", "%llu"),
		    (unsigned long long)blob_id);
		goto err;
	}

	if (LF_ISSET(DB_FOP_READONLY) || DB_IS_READONLY(dbp))
		oflags |= DB_OSO_RDONLY;
	if ((ret = __os_open(env, path, 0, oflags, 0, fhpp)) != 0) {
		__db_errx(env, DB_STR_A("0232",
		    "Error opening blob file: %s.", "%s"), path);
		goto err;
	}

err:	if (path != NULL)
		__os_free(env, path);
	if (ppath != NULL)
		__os_free(env, ppath);
	return (ret);
}

/*
 * __blob_file_read --
 *
 * PUBLIC: int __blob_file_read
 * PUBLIC:	__P((ENV *, DB_FH *, DBT *, off_t, u_int32_t));
 */
int
__blob_file_read(env, fhp, dbt, offset, size)
	ENV *env;
	DB_FH *fhp;
	DBT *dbt;
	off_t offset;
	u_int32_t size;
{
	int ret;
	size_t bytes;
	void *buf;

	bytes = 0;
	buf = NULL;

	if ((ret = __os_seek(env, fhp, 0, 0, offset)) != 0)
		goto err;

	if (F_ISSET(dbt, DB_DBT_USERCOPY)) {
		if ((ret = __os_malloc(env, size, &buf)) != 0)
			goto err;
	} else
		buf = dbt->data;

	if ((ret = __os_read(env, fhp, buf, size, &bytes)) != 0) {
		__db_errx(env, DB_STR("0233", "Error reading blob file."));
		goto err;
	}
	/* Should never read more than what can fit in u_int32_t. */
	DB_ASSERT(env, bytes <= UINT32_MAX);
	/*
	 * It is okay to read off the end of the file, in which case less bytes
	 * will be returned than requested.  This is also how the code behaves
	 * in the DB_DBT_PARTIAL API.
	 */
	dbt->size = (u_int32_t)bytes;

	if (F_ISSET(dbt, DB_DBT_USERCOPY)  && dbt->size != 0) {
		ret = env->dbt_usercopy(
		    dbt, 0, buf, dbt->size, DB_USERCOPY_SETDATA);
	}

err:	if (buf != NULL && buf != dbt->data)
		__os_free(env, buf);
	return (ret);
}

/*
 * __blob_file_write --
 *
 * PUBLIC: int __blob_file_write
 * PUBLIC: __P((DBC *, DB_FH *, DBT *,
 * PUBLIC:    off_t, db_seq_t, off_t *, u_int32_t));
 */
int
__blob_file_write(dbc, fhp, buf, offset, blob_id, file_size, flags)
	DBC *dbc;
	DB_FH *fhp;
	DBT *buf;
	off_t offset;
	db_seq_t blob_id;
	off_t *file_size;
	u_int32_t flags;
{
	ENV *env;
	off_t size, write_offset;
	char *dirname, *name;
	int ret, blob_lg;
	size_t data_size;
	void *ptr;

	env = dbc->env;
	dirname = name = NULL;
	size = 0;
	write_offset = offset;
	DB_ASSERT(env, !DB_IS_READONLY(dbc->dbp));

	/* File size is used to tell if the write is extending the file. */
	size = *file_size;

	if (DBENV_LOGGING(env)) {
		if ((ret = __log_get_config(
		    env->dbenv, DB_LOG_BLOB, &blob_lg)) != 0)
			goto err;
		if (blob_lg == 0)
			LF_SET(DB_FOP_PARTIAL_LOG);
		if (!LF_ISSET(DB_FOP_CREATE) && (size <= offset))
			LF_SET(DB_FOP_APPEND);
	}

	if ((ret = __blob_id_to_path(
	    env, dbc->dbp->blob_sub_dir, blob_id, &name)) != 0)
		goto err;

	if ((ret = __dbt_usercopy(env, buf)) != 0)
		goto err;

	/*
	 * If the write overwrites some of the file, and writes off the end
	 * of the file, break the write into two writes, one that overwrites
	 * data, and an append.  Otherwise if the write is aborted, the
	 * data written past the end of the file will not be erased.
	 */
	if (offset < size && (offset + buf->size) > size) {
		ptr = buf->data;
		data_size = (size_t)(size - offset);
		if ((ret = __fop_write_file(env, dbc->txn, name, dirname,
		    DB_APP_BLOB, fhp, offset, ptr, data_size, flags)) != 0) {
			__db_errx(env, DB_STR_A("0235",
			    "Error writing blob file: %s.", "%s"), name);
				goto err;
		}
		LF_SET(DB_FOP_APPEND);
		ptr = (u_int8_t *)ptr + data_size;
		data_size = buf->size - data_size;
		write_offset = size;
	} else {
		if (!LF_ISSET(DB_FOP_CREATE) && (offset >= size))
			LF_SET(DB_FOP_APPEND);
		ptr = buf->data;
		data_size = buf->size;
	}

	if ((ret = __fop_write_file(env, dbc->txn, name, dirname,
	    DB_APP_BLOB, fhp, write_offset, ptr, data_size, flags)) != 0) {
		__db_errx(env, DB_STR_A("0236",
		    "Error writing blob file: %s.", "%s"), name);
		goto err;
	}

	if (LF_ISSET(DB_FOP_SYNC_WRITE))
		if ((ret = __os_fsync(env, fhp)) != 0)
			goto err;

	/* Update the size of the file. */
	if ((offset + (off_t)buf->size) > size)
		*file_size = offset + (off_t)buf->size;

err:	if (name != NULL)
		__os_free(env, name);

	return (ret);
}
