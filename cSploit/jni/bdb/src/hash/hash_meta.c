/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

/*
 * Acquire the meta-data page.
 *
 * PUBLIC: int __ham_get_meta __P((DBC *));
 */
int
__ham_get_meta(dbc)
	DBC *dbc;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	HASH *hashp;
	HASH_CURSOR *hcp;
	u_int32_t revision;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hashp = dbp->h_internal;
	hcp = (HASH_CURSOR *)dbc->internal;

again:
	revision = hashp->revision;
	if ((ret = __db_lget(dbc, 0,
	     hashp->meta_pgno, DB_LOCK_READ, 0, &hcp->hlock)) != 0)
		return (ret);

	if ((ret = __memp_fget(mpf, &hashp->meta_pgno,
	    dbc->thread_info, dbc->txn, DB_MPOOL_CREATE, &hcp->hdr)) != 0) {
		(void)__LPUT(dbc, hcp->hlock);
		return (ret);
	}

	if (F_ISSET(dbp, DB_AM_SUBDB) &&
	    (revision != dbp->mpf->mfp->revision ||
	    (TYPE(hcp->hdr) != P_HASHMETA &&
	    !IS_RECOVERING(dbp->env) && !F_ISSET(dbp, DB_AM_RECOVER)))) {
		ret = __LPUT(dbc, hcp->hlock);
		t_ret =
		    __memp_fput(mpf, dbc->thread_info, hcp->hdr, dbc->priority);
		hcp->hdr = NULL;
		if (ret != 0)
			return (ret);
		if (t_ret != 0)
			return (t_ret);
		if ((ret = __db_reopen(dbc)) != 0)
			return (ret);
		goto again;
	}

	return (ret);
}

/*
 * Release the meta-data page.
 *
 * PUBLIC: int __ham_release_meta __P((DBC *));
 */
int
__ham_release_meta(dbc)
	DBC *dbc;
{
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	int ret;

	mpf = dbc->dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;

	if (hcp->hdr != NULL) {
		if ((ret = __memp_fput(mpf,
		    dbc->thread_info, hcp->hdr, dbc->priority)) != 0)
			return (ret);
		hcp->hdr = NULL;
	}

	ret = __TLPUT(dbc, hcp->hlock);
	hcp->hlock.mode = DB_LOCK_NG;
	return (ret);
}

/*
 * Mark the meta-data page dirty.
 *
 * PUBLIC: int __ham_dirty_meta __P((DBC *, u_int32_t));
 */
int
__ham_dirty_meta(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB_MPOOLFILE *mpf;
	HASH *hashp;
	HASH_CURSOR *hcp;
	int ret;

	if (F_ISSET(dbc, DBC_OPD))
		dbc = dbc->internal->pdbc;
	hashp = dbc->dbp->h_internal;
	hcp = (HASH_CURSOR *)dbc->internal;
	if (hcp->hlock.mode == DB_LOCK_WRITE)
		return (0);

	mpf = dbc->dbp->mpf;

	if ((ret = __db_lget(dbc, LCK_COUPLE, hashp->meta_pgno,
	     DB_LOCK_WRITE, DB_LOCK_NOWAIT, &hcp->hlock)) != 0) {
		if (ret != DB_LOCK_NOTGRANTED && ret != DB_LOCK_DEADLOCK)
			return (ret);
		if ((ret = __memp_fput(mpf,
		    dbc->thread_info, hcp->hdr, dbc->priority)) != 0)
			return (ret);
		hcp->hdr = NULL;
		if ((ret = __db_lget(dbc, LCK_COUPLE, hashp->meta_pgno,
		     DB_LOCK_WRITE, 0, &hcp->hlock)) != 0)
			return (ret);
		ret = __memp_fget(mpf, &hashp->meta_pgno,
		    dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &hcp->hdr);
		return (ret);
	}

	return (__memp_dirty(mpf,
	    &hcp->hdr, dbc->thread_info, dbc->txn, dbc->priority, flags));
}

/*
 * Return the meta data page if it is saved in the cursor.
 *
 * PUBLIC: int __ham_return_meta __P((DBC *, u_int32_t, DBMETA **));
 */
 int
 __ham_return_meta(dbc, flags, metap)
	DBC *dbc;
	u_int32_t flags;
	DBMETA **metap;
{
	HASH_CURSOR *hcp;
	int ret;

	*metap = NULL;
	if (F_ISSET(dbc, DBC_OPD))
		dbc = dbc->internal->pdbc;

	hcp = (HASH_CURSOR *)dbc->internal;
	if (hcp->hdr == NULL || PGNO(hcp->hdr) != PGNO_BASE_MD)
		return (0);

	if (LF_ISSET(DB_MPOOL_DIRTY) &&
	    (ret = __ham_dirty_meta(dbc, flags)) != 0)
		return (ret);

	*metap = (DBMETA *)hcp->hdr;
	return (0);
}
