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

static int __db_stream_close __P((DB_STREAM *, u_int32_t));
static int __db_stream_read
    __P((DB_STREAM *, DBT *, db_off_t, u_int32_t, u_int32_t));
static int __db_stream_size __P((DB_STREAM *, db_off_t *, u_int32_t));
static int __db_stream_write __P((DB_STREAM *, DBT *, db_off_t, u_int32_t));

/*
 * __db_stream_init
 *	DB_STREAM initializer.
 *
 * PUBLIC: int __db_stream_init __P((DBC *, DB_STREAM **, u_int32_t));
 */
int
__db_stream_init(dbc, dbsp, flags)
	DBC *dbc;
	DB_STREAM **dbsp;
	u_int32_t flags;
{
	DB_STREAM *dbs;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;
	off_t size;

	dbs = NULL;
	env = dbc->env;

	if ((ret = __os_malloc(env, sizeof(DB_STREAM), &dbs)) != 0)
		return (ret);
	memset(dbs, 0, sizeof(DB_STREAM));

	ENV_ENTER(env, ip);
	/* Should the copy be transient? */
	if ((ret = __dbc_idup(dbc, &dbs->dbc, DB_POSITION)) != 0)
		goto err;
	dbs->flags = flags;

	/*
	 * Make sure we have a write lock on the db record if writing
	 * to the blob.
	 */
	if (F_ISSET(dbs, DB_FOP_WRITE))
		F_SET(dbc, DBC_RMW);

	if ((ret = __dbc_get_blob_id(dbs->dbc, &dbs->blob_id)) != 0) {
		if (ret == EINVAL)
			__db_errx(env, DB_STR("0211",
			    "Error, cursor does not point to a blob."));
		goto err;
	}

	if ((ret = __dbc_get_blob_size(dbs->dbc, &size)) != 0)
		goto err;
	dbs->file_size = size;

	if ((ret = __blob_file_open(
	    dbs->dbc->dbp, &dbs->fhp, dbs->blob_id, flags)) != 0)
		goto err;
	ENV_LEAVE(env, ip);

	dbs->close = __db_stream_close;
	dbs->read = __db_stream_read;
	dbs->size = __db_stream_size;
	dbs->write = __db_stream_write;

	*dbsp = dbs;
	return (0);

err:	if (dbs != NULL && dbs->dbc != NULL)
		__dbc_close(dbs->dbc);
	ENV_LEAVE(env, ip);
	if (dbs != NULL)
		__os_free(env, dbs);
	return (ret);
}

/*
 * __db_stream_close --
 *
 * DB_STREAM->close
 */
static int
__db_stream_close(dbs, flags)
	DB_STREAM *dbs;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbs->dbc->env;

	if ((ret = __db_fchk(env, "DB_STREAM->close", flags, 0)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	ret = __db_stream_close_int(dbs);

	ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __db_stream_close_int --
 *	Close a DB_STREAM object.
 *
 * PUBLIC: int __db_stream_close_int __P ((DB_STREAM *));
 */
int
__db_stream_close_int(dbs)
	DB_STREAM *dbs;
{
	DBC *dbc;
	ENV *env;
	int ret, t_ret;

	dbc = dbs->dbc;
	env = dbc->env;

	ret = __blob_file_close(dbc, dbs->fhp, dbs->flags);

	if ((t_ret = __dbc_close(dbs->dbc)) != 0 && ret == 0)
		ret = t_ret;

	__os_free(env, dbs);

	return (ret);
}

/*
 * __db_stream_read --
 *
 * DB_STREAM->read
 */
static int
__db_stream_read(dbs, data, offset, size, flags)
	DB_STREAM *dbs;
	DBT *data;
	db_off_t offset;
	u_int32_t size;
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	int ret;
	u_int32_t needed, start;

	dbc = dbs->dbc;
	env = dbc->dbp->env;
	ret = 0;

	if ((ret = __db_fchk(env, "DB_STREAM->read", flags, 0)) != 0)
		return (ret);

	if (F_ISSET(data, DB_DBT_PARTIAL)) {
		ret = EINVAL;
		__db_errx(env, DB_STR("0212",
		    "Error, do not use DB_DBT_PARTIAL with DB_STREAM."));
		goto err;
	}

	if (offset > dbs->file_size) {
		data->size = 0;
		goto err;
	}

	if ((ret = __db_alloc_dbt(
	    env, data, size, &needed, &start, NULL, NULL)) != 0)
		goto err;
	data->size = needed;

	if (needed == 0)
		goto err;

	ret = __blob_file_read(env, dbs->fhp, data, offset, size);

err:	return (ret);
}

/*
 * __db_stream_size --
 *
 * DB_STREAM->size
 */
static int
__db_stream_size(dbs, size, flags)
	DB_STREAM *dbs;
	db_off_t *size;
	u_int32_t flags;
{
	int ret;

	if ((ret = __db_fchk(dbs->dbc->env, "DB_STREAM->size", flags, 0)) != 0)
		return (ret);

	*size = dbs->file_size;

	return (0);
}

/*
 * __db_stream_write --
 *
 * DB_STREAM->write
 */
static int
__db_stream_write(dbs, data, offset, flags)
	DB_STREAM *dbs;
	DBT *data;
	db_off_t offset;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;
	off_t file_size;
	u_int32_t wflags;

	env = dbs->dbc->env;

	if ((ret = __db_fchk(
	    env, "DB_STREAM->write", flags, DB_STREAM_SYNC_WRITE)) != 0)
		return (ret);

	if (F_ISSET(dbs, DB_FOP_READONLY)) {
		ret = EINVAL;
		__db_errx(env, DB_STR("0213", "Error, blob is read only."));
		return (ret);
	}
	if (F_ISSET(data, DB_DBT_PARTIAL)) {
		ret = EINVAL;
		__db_errx(env, DB_STR("0214",
		    "Error, do not use DB_DBT_PARTIAL with DB_STREAM."));
		return (ret);
	}
	if (offset < 0 ) {
		ret = EINVAL;
		__db_errx(env, DB_STR_A("0215",
		    "Error, invalid offset value: %lld", "%lld"),
		    (long long)offset);
		return (ret);
	}
	/* Catch overflow. */
	if (offset + (off_t)data->size < offset) {
		ret = EINVAL;
		__db_errx(env, DB_STR_A("0216",
	"Error, this write will exceed the maximum blob size: %lu %lld",
		"%lu %lld"), (u_long)data->size, (long long)offset);
		return (ret);
	}

	ENV_ENTER(env, ip);
	wflags = dbs->flags;
	if (LF_ISSET(DB_STREAM_SYNC_WRITE))
		wflags |= DB_FOP_SYNC_WRITE;
	file_size = dbs->file_size;
	if ((ret = __blob_file_write(dbs->dbc, dbs->fhp,
	    data, offset, dbs->blob_id, &file_size, wflags)) != 0)
		goto err;
	if (file_size != dbs->file_size) {
		dbs->file_size = file_size;
		if ((ret = __dbc_set_blob_size(dbs->dbc, dbs->file_size)) != 0)
			goto err;
	}
err:	ENV_LEAVE(env, ip);

	return (ret);
}
