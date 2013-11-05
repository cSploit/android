/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2013 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/heap.h"

/*
 * __db_ret --
 *	Build return DBT.
 *
 * PUBLIC: int __db_ret __P((DBC *,
 * PUBLIC:    PAGE *, u_int32_t, DBT *, void **, u_int32_t *));
 */
int
__db_ret(dbc, h, indx, dbt, memp, memsize)
	DBC *dbc;
	PAGE *h;
	u_int32_t indx;
	DBT *dbt;
	void **memp;
	u_int32_t *memsize;
{
	BBLOB bl;
	BKEYDATA *bk;
	BOVERFLOW *bo;
	DB *dbp;
	ENV *env;
	HBLOB hblob;
	HEAPBLOBHDR bhdr;
	HEAPHDR *hdr;
	uintmax_t blob_id;
	int ret;
	HOFFPAGE ho;
	off_t blob_size;
	u_int32_t len;
	u_int8_t *hk;
	void *data;

	if (F_ISSET(dbt, DB_DBT_READONLY))
		return (0);
	ret = 0;
	dbp = dbc->dbp;
	env = dbp->env;

	switch (TYPE(h)) {
	case P_HASH_UNSORTED:
	case P_HASH:
		hk = P_ENTRY(dbp, h, indx);
		if (HPAGE_PTYPE(hk) == H_OFFPAGE) {
			memcpy(&ho, hk, sizeof(HOFFPAGE));
			return (__db_goff(dbc, dbt,
			    ho.tlen, ho.pgno, memp, memsize));
		} else if (HPAGE_PTYPE(hk) == H_BLOB) {
			/* Get the record instead of the blob item. */
			if (F_ISSET(dbt, DB_DBT_BLOB_REC)) {
				data = P_ENTRY(dbp, h, indx);
				len = HBLOB_SIZE;
				break;
			}
			memcpy(&hblob, hk, HBLOB_SIZE);
			GET_BLOB_ID(env, hblob, blob_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB_SIZE(env, hblob, blob_size, ret);
			if (ret != 0)
				return (ret);
			return (__blob_get(
			    dbc, dbt, blob_id, blob_size, memp, memsize));
		}
		len = LEN_HKEYDATA(dbp, h, dbp->pgsize, indx);
		data = HKEYDATA_DATA(hk);
		break;
	case P_HEAP:
		hdr = (HEAPHDR *)P_ENTRY(dbp, h, indx);
		if (F_ISSET(hdr,(HEAP_RECSPLIT | HEAP_RECFIRST)))
			return (__heapc_gsplit(dbc, dbt, memp, memsize));
		else if (F_ISSET(hdr, HEAP_RECBLOB)) {
			/* Get the record instead of the blob item. */
			if (F_ISSET(dbt, DB_DBT_BLOB_REC)) {
				data = P_ENTRY(dbp, h, indx);
				len = HEAPBLOBREC_SIZE;
				break;
			}
			memcpy(&bhdr, hdr, HEAPBLOBREC_SIZE);
			GET_BLOB_ID(env, bhdr, blob_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB_SIZE(env, bhdr, blob_size, ret);
			if (ret != 0)
				return (ret);
			return (__blob_get(
			    dbc, dbt, blob_id, blob_size, memp, memsize));
		}
		len = hdr->size;
		data = (u_int8_t *)hdr + sizeof(HEAPHDR);
		break;
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		bk = GET_BKEYDATA(dbp, h, indx);
		if (B_TYPE(bk->type) == B_OVERFLOW) {
			bo = (BOVERFLOW *)bk;
			return (__db_goff(dbc, dbt,
			    bo->tlen, bo->pgno, memp, memsize));
		} else if (B_TYPE(bk->type) == B_BLOB) {
			/* Get the record instead of the blob item. */
			if (F_ISSET(dbt, DB_DBT_BLOB_REC)) {
				data = P_ENTRY(dbp, h, indx);
				len = BBLOB_SIZE;
				break;
			}
			memcpy(&bl, bk, BBLOB_SIZE);
			GET_BLOB_ID(env, bl, blob_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB_SIZE(env, bl, blob_size, ret);
			if (ret != 0)
				return (ret);
			return (__blob_get(
			    dbc, dbt, blob_id, blob_size, memp, memsize));
		}
		len = bk->len;
		data = bk->data;
		break;
	default:
		return (__db_pgfmt(dbp->env, h->pgno));
	}

	return (__db_retcopy(dbp->env, dbt, data, len, memp, memsize));
}

/*
 * __db_retcopy --
 *	Copy the returned data into the user's DBT, handling special flags.
 *
 * PUBLIC: int __db_retcopy __P((ENV *, DBT *,
 * PUBLIC:    void *, u_int32_t, void **, u_int32_t *));
 */
int
__db_retcopy(env, dbt, data, len, memp, memsize)
	ENV *env;
	DBT *dbt;
	void *data;
	u_int32_t len;
	void **memp;
	u_int32_t *memsize;
{
	int ret;

	if (F_ISSET(dbt, DB_DBT_READONLY))
		return (0);
	ret = 0;

	/* If returning a partial record, reset the length. */
	if (F_ISSET(dbt, DB_DBT_PARTIAL)) {
		data = (u_int8_t *)data + dbt->doff;
		if (len > dbt->doff) {
			len -= dbt->doff;
			if (len > dbt->dlen)
				len = dbt->dlen;
		} else
			len = 0;
	}

	/*
	 * Allocate memory to be owned by the application: DB_DBT_MALLOC,
	 * DB_DBT_REALLOC.
	 *
	 * !!!
	 * We always allocate memory, even if we're copying out 0 bytes. This
	 * guarantees consistency, i.e., the application can always free memory
	 * without concern as to how many bytes of the record were requested.
	 *
	 * Use the memory specified by the application: DB_DBT_USERMEM.
	 *
	 * !!!
	 * If the length we're going to copy is 0, the application-supplied
	 * memory pointer is allowed to be NULL.
	 */
	if (F_ISSET(dbt, DB_DBT_USERCOPY)) {
		dbt->size = len;
		return (len == 0 ? 0 : env->dbt_usercopy(dbt, 0, data,
		    len, DB_USERCOPY_SETDATA));

	} else if (F_ISSET(dbt, DB_DBT_MALLOC))
		ret = __os_umalloc(env, len, &dbt->data);
	else if (F_ISSET(dbt, DB_DBT_REALLOC)) {
		if (dbt->data == NULL || dbt->size == 0 || dbt->size < len)
			ret = __os_urealloc(env, len, &dbt->data);
	} else if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (len != 0 && (dbt->data == NULL || dbt->ulen < len))
			ret = DB_BUFFER_SMALL;
	} else if (memp == NULL || memsize == NULL)
		ret = EINVAL;
	else {
		if (len != 0 && (*memsize == 0 || *memsize < len)) {
			if ((ret = __os_realloc(env, len, memp)) == 0)
				*memsize = len;
			else
				*memsize = 0;
		}
		if (ret == 0)
			dbt->data = *memp;
	}

	if (ret == 0 && len != 0)
		memcpy(dbt->data, data, len);

	/*
	 * Return the length of the returned record in the DBT size field.
	 * This satisfies the requirement that if we're using user memory
	 * and insufficient memory was provided, return the amount necessary
	 * in the size field.
	 */
	dbt->size = len;

	return (ret);
}

/*
 * __db_dbt_clone --
 *	Clone a DBT from another DBT.
 * The input dest DBT must be a zero initialized DBT that will be populated.
 * The function does not allocate a dest DBT to allow for cloning into stack
 * or locally allocated variables. It is the callers responsibility to free
 * the memory allocated in dest->data.
 *
 * PUBLIC: int __db_dbt_clone __P((ENV *, DBT *, const DBT *));
 */
int
__db_dbt_clone(env, dest, src)
	ENV *env;
	DBT *dest;
	const DBT *src;
{
	u_int32_t err_flags;
	int ret;

	DB_ASSERT(env, dest->data == NULL);

	ret = 0;

	/* The function does not support the following DBT flags. */
	err_flags = DB_DBT_MALLOC | DB_DBT_REALLOC |
	    DB_DBT_MULTIPLE | DB_DBT_PARTIAL;
	if (F_ISSET(src, err_flags)) {
		__db_errx(env, DB_STR("0758",
		    "Unsupported flags when cloning the DBT."));
		return (EINVAL);
	}

	if ((ret = __os_malloc(env, src->size, &dest->data)) != 0)
		return (ret);

	memcpy(dest->data, src->data, src->size);
	dest->ulen = src->size;
	dest->size = src->size;
	dest->flags = DB_DBT_USERMEM;

	return (ret);
}

/*
 * __db_dbt_clone_free --
 *	Free a DBT cloned by __db_dbt_clone
 *
 * PUBLIC: int __db_dbt_clone_free __P((ENV *, DBT *));
 */
int
__db_dbt_clone_free(env, dbt)
	ENV *env;
	DBT *dbt;
{
	/* Currently only DB_DBT_USERMEM is supported. */
	if (dbt->flags != DB_DBT_USERMEM) {
		__db_errx(env, DB_STR("0759",
		    "Unsupported flags when freeing the cloned DBT."));
		return (EINVAL);
	}

	if (dbt->data != NULL)
		__os_free(env, dbt->data);
	dbt->size = dbt->ulen = 0;

	return (0);
}
