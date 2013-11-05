/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013 Oracle and/or its affiliates.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/blob.h"
#include "dbinc/fop.h"
#include "dbinc/mp.h"

/*
 * Blob file data item code.
 *
 * Blob file data entries are stored on linked lists of pages.  The initial
 * reference is a structure with an encoded version of the path where the file
 * is stored. The blob file contains only the users data.
 */

/*
 * __blob_bulk --
 *	Dump blob file into buffer.
 *	The space requirements have already been checked, if the blob is
 *	larger than UINT32MAX then DB_BUFFER_SMALL would have already
 *	been returned.
 * PUBLIC: int __blob_bulk
 * PUBLIC:    __P((DBC *, u_int32_t, uintmax_t, u_int8_t *));
 */
int
__blob_bulk(dbc, len, blob_id, dp)
	DBC *dbc;
	u_int32_t len;
	uintmax_t blob_id;
	u_int8_t *dp;
{
	DBT dbt;
	DB_FH *fhp;
	ENV *env;
	int ret, t_ret;

	env = dbc->dbp->env;
	fhp = NULL;
	memset(&dbt, 0, sizeof(dbt));
	F_SET(&dbt, DB_DBT_USERMEM);
	dbt.ulen = len;
	dbt.data = (void *)dp;

	if ((ret = __blob_file_open(
	    dbc->dbp, &fhp, blob_id, DB_FOP_READONLY)) != 0)
		goto err;

	if ((ret = __blob_file_read(env, fhp, &dbt, 0, len)) != 0)
		goto err;

	/* Close any open file descriptors. */
err:	if (fhp != NULL) {
		t_ret = __blob_file_close(dbc, fhp, 0);
		if (ret == 0)
			ret = t_ret;
	}
	return (ret);
}

/*
 * __blob_get --
 *	Get a blob file item. Analogous to db_overflow.c:__db_goff.
 *
 * PUBLIC: int __blob_get __P((DBC *,
 * PUBLIC:     DBT *, uintmax_t, off_t, void **, u_int32_t *));
 */
int
__blob_get(dbc, dbt, blob_id, file_size, bpp, bpsz)
	DBC *dbc;
	DBT *dbt;
	uintmax_t blob_id;
	off_t file_size;
	void **bpp;
	u_int32_t *bpsz;
{
	DB_FH *fhp;
	ENV *env;
	int ret, t_ret;
	u_int32_t needed, start, tlen;

	env = dbc->dbp->env;
	fhp = NULL;
	ret = 0;

	/*
	 * Blobs larger than UINT32_MAX can only be read using
	 * the DB_STREAM API, or the DB_DBT_PARTIAL API.
	 */
	if (file_size > UINT32_MAX) {
		if (!F_ISSET(dbt, DB_DBT_PARTIAL)) {
			dbt->size = UINT32_MAX;
			ret = DB_BUFFER_SMALL;
			goto err;
		} else
			tlen = UINT32_MAX;
	} else
		tlen = (u_int32_t)file_size;

	if (((ret = __db_alloc_dbt(
	    env, dbt, tlen, &needed, &start, bpp, bpsz)) != 0) || needed == 0)
		goto err;
	dbt->size = needed;

	if ((ret = __blob_file_open(
	    dbc->dbp, &fhp, blob_id, DB_FOP_READONLY)) != 0)
		goto err;

	if ((ret = __blob_file_read(env, fhp, dbt, dbt->doff, needed)) != 0)
		goto err;

	/* Close any open file descriptors. */
err:	if (fhp != NULL) {
		t_ret = __blob_file_close(dbc, fhp, 0);
		if (ret == 0)
			ret = t_ret;
	}
	/* Does the dbt need to be cleaned on error? */
	return (ret);
}

/*
 * __blob_put --
 *	Put a blob file item.
 *
 * PUBLIC: int __blob_put __P((
 * PUBLIC:    DBC *, DBT *, uintmax_t *, off_t *size, DB_LSN *));
 */
int
__blob_put(dbc, dbt, blob_id, size, plsn)
	DBC *dbc;
	DBT *dbt;
	uintmax_t *blob_id;
	off_t *size;
	DB_LSN *plsn;
{
	DBT partial;
	DB_FH *fhp;
	ENV *env;
	int ret, t_ret;
	off_t offset;

	env = dbc->dbp->env;
	fhp = NULL;
	offset = 0;
	DB_ASSERT(env, blob_id != NULL);
	DB_ASSERT(env, *blob_id == 0);

	ZERO_LSN(*plsn);

	/* If the id didn't refer to an existing blob generate a new one. */
	if ((ret = __blob_file_create(dbc, &fhp, blob_id)) != 0)
		goto err;

	/*
	 * If doing a partial put with dbt->doff == 0, then treat like
	 * a normal put.  Otherwise write NULLs into the file up to doff, which
	 * is required by the PARTIAL API.  Since the file is being created,
	 * its size is always 0.
	 */
	DB_ASSERT(env, *size == 0);
	if (F_ISSET(dbt, DB_DBT_PARTIAL) && dbt->doff > 0) {
		memset(&partial, 0, sizeof(partial));
		if ((ret = __os_malloc(env, dbt->doff, &partial.data)) != 0)
			goto err;
		memset(partial.data, 0, dbt->doff);
		partial.size = dbt->doff;
		ret = __blob_file_write(
		    dbc, fhp, &partial, 0, *blob_id, size, DB_FOP_CREATE);
		offset = dbt->doff;
		__os_free(env, partial.data);
		if (ret != 0)
			goto err;
	}

	if ((ret = __blob_file_write(
	    dbc, fhp, dbt, offset, *blob_id, size, DB_FOP_CREATE)) != 0)
		goto err;

	/* Close any open file descriptors. */
err:	if (fhp != NULL) {
		t_ret = __blob_file_close(dbc, fhp, DB_FOP_WRITE);
		if (ret == 0)
			ret = t_ret;
	}
	return (ret);
}

/*
 * __blob_repl --
 *	Replace a blob file contents.  It would be nice if this could be done
 *	by truncating the file and writing in the new data, but undoing a
 *	truncate would require a lot of logging, so it is performed by
 *	deleting the old blob file, and creating a new one.
 *
 * PUBLIC: int __blob_repl __P((DBC *, DBT *, uintmax_t, uintmax_t *,off_t *));
 */
int
__blob_repl(dbc, nval, blob_id, new_blob_id, size)
	DBC *dbc;
	DBT *nval;
	uintmax_t blob_id;
	uintmax_t *new_blob_id;
	off_t *size;
{
	DBT partial;
	DB_FH *fhp, *new_fhp;
	DB_LSN lsn;
	ENV *env;
	int ret, t_ret;
	off_t current, old_size;

	fhp = new_fhp = NULL;
	*new_blob_id = 0;
	old_size = *size;
	env = dbc->env;
	memset(&partial, 0, sizeof(partial));

	/*
	 * Handling partial replace.
	 * 1. doff > blob file size : Pad the end of the blob file with NULLs
	 *	up to doff, then append the data.
	 * 2. doff == size: Write the data to the existing blob file.
	 * 3. dlen == size: Write the data to the existing blob file.
	 * 4. Create a new blob file.  Copy old blob data up to doff
	 *	to the new file.  Append the new data.  Append data
	 *	from the old file from doff + dlen to the end of the
	 *	old file to the new file.  Delete the old file.
	 */
	if (F_ISSET(nval, DB_DBT_PARTIAL)) {
		if ((nval->doff > *size) ||
		    ((nval->doff == *size) || (nval->dlen == nval->size))) {
			/* Open the file for appending. */
			if ((ret = __blob_file_open(
			    dbc->dbp, &fhp, blob_id, 0)) != 0)
				goto err;
			*new_blob_id = blob_id;

			/* Pad the end of the blob with NULLs. */
			if (nval->doff > *size) {
				partial.size = nval->doff - (u_int32_t)*size;
				if ((ret = __os_malloc(
				    env, partial.size, &partial.data)) != 0)
					goto err;
				memset(partial.data, 0, partial.size);
				if ((ret = __blob_file_write(dbc, fhp,
				    &partial, *size, blob_id, size, 0)) != 0)
					goto err;
			}

			/* Write in the data. */
			if ((ret = __blob_file_write(dbc, fhp,
			    nval, nval->doff, blob_id, size, 0)) != 0)
				goto err;

			/* Close the file */
			ret = __blob_file_close(dbc, fhp, DB_FOP_WRITE);
			fhp = NULL;
			if (ret != 0)
				goto err;
		} else {
			/* Open the old blob file. */
			if ((ret = __blob_file_open(
			    dbc->dbp, &fhp, blob_id, DB_FOP_READONLY)) != 0)
				goto err;
			/* Create the new blob file. */
			if ((ret = __blob_file_create(
			    dbc, &new_fhp, new_blob_id)) != 0)
				goto err;

			*size = 0;
			/* Copy data to the new file up to doff. */
			if (nval->doff != 0) {
				partial.ulen = partial.size = nval->doff;
				if ((ret = __os_malloc(
				    env, partial.ulen, &partial.data)) != 0)
					goto err;
				if ((ret = __blob_file_read(
				    env, fhp, &partial, 0, partial.size)) != 0)
					goto err;
				if ((ret = __blob_file_write(
				    dbc, new_fhp, &partial, 0,
				    *new_blob_id, size, DB_FOP_CREATE)) != 0)
					goto err;
			}

			/* Write the partial data into the new file. */
			if ((ret = __blob_file_write(
			    dbc, new_fhp, nval, nval->doff,
			    *new_blob_id, size, DB_FOP_CREATE)) != 0)
				goto err;

			/* Copy remaining blob data into the new file. */
			current = nval->doff + nval->dlen;
			while (current < old_size) {
				if (partial.ulen < MEGABYTE) {
					if ((ret = __os_realloc(env,
					    MEGABYTE, &partial.data)) != 0)
						goto err;
					partial.size = partial.ulen = MEGABYTE;
				}
				if ((old_size - current) < partial.ulen) {
					partial.size =
					(u_int32_t)(old_size - current);
				} else
					partial.size = MEGABYTE;

				if ((ret = __blob_file_read(env, fhp,
				    &partial, current, partial.size)) != 0)
					goto err;
				if ((ret = __blob_file_write(
				    dbc, new_fhp, &partial, *size,
				    *new_blob_id, size, DB_FOP_CREATE)) != 0)
					goto err;
				current += partial.size;
			}

			/* Close the old file. */
			ret = __blob_file_close(dbc, fhp, 0);
			fhp = NULL;
			if (ret != 0)
				goto err;

			/* Delete the old blob file. */
			if ((ret = __blob_del(dbc, blob_id)) != 0)
				goto err;
		}
		goto err;
	}

	if ((ret = __blob_del(dbc, blob_id)) != 0)
		goto err;

	*size = 0;
	if ((ret = __blob_put(dbc, nval, new_blob_id, size, &lsn)) != 0)
		goto err;

err:	if (fhp != NULL) {
		t_ret = __blob_file_close(dbc, fhp, DB_FOP_WRITE);
		if (ret == 0)
			ret = t_ret;
	}
	if (new_fhp != NULL) {
		t_ret = __blob_file_close(dbc, new_fhp, DB_FOP_WRITE);
		if (ret == 0)
			ret = t_ret;
	}
	if (partial.data != NULL)
		__os_free(env, partial.data);
	return (ret);
}

/*
 * __blob_del --
 *	Delete a blob file. The onpage record is handled separately..
 *
 * PUBLIC: int __blob_del __P((DBC *, uintmax_t));
 */
int
__blob_del(dbc, blob_id)
	DBC *dbc;
	uintmax_t blob_id;
{
	int ret;

	ret = __blob_file_delete(dbc, blob_id);

	return (ret);
}
