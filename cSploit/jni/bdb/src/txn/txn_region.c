/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int __txn_init __P((ENV *, DB_TXNMGR *));
static int lsn_hi_to_low __P((const void *, const void *));

/*
 * __txn_open --
 *	Open a transaction region.
 *
 * PUBLIC: int __txn_open __P((ENV *));
 */
int
__txn_open(env)
	ENV *env;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int ret;

	/* Create/initialize the transaction manager structure. */
	if ((ret = __os_calloc(env, 1, sizeof(DB_TXNMGR), &mgr)) != 0)
		return (ret);
	TAILQ_INIT(&mgr->txn_chain);
	mgr->env = env;

	/* Join/create the txn region. */
	if ((ret = __env_region_share(env, &mgr->reginfo)) != 0)
		goto err;

	/* If we created the region, initialize it. */
	if (F_ISSET(&mgr->reginfo, REGION_CREATE))
		if ((ret = __txn_init(env, mgr)) != 0)
			goto err;

	/* Set the local addresses. */
	region = mgr->reginfo.primary =
	    R_ADDR(&mgr->reginfo,
	    ((REGENV *)env->reginfo->primary)->tx_primary);

	/* If threaded, acquire a mutex to protect the active TXN list. */
	if ((ret = __mutex_alloc(
	    env, MTX_TXN_ACTIVE, DB_MUTEX_PROCESS_ONLY, &mgr->mutex)) != 0)
		goto err;

	mgr->reginfo.mtx_alloc = region->mtx_region;
	env->tx_handle = mgr;
	return (0);

err:	(void)__mutex_free(env, &mgr->mutex);
	(void)__txn_region_detach(env, mgr);

	return (ret);
}

/*
 * __txn_region_detach --
 *
 * PUBLIC: int __txn_region_detach __P((ENV *, DB_TXNMGR *));
 */
int
__txn_region_detach(env, mgr)
	ENV *env;
	DB_TXNMGR *mgr;
{
	int ret;

	ret = 0;
	if (mgr != NULL) {
		ret = __env_region_detach(env, &mgr->reginfo, 0);
		__os_free(env, mgr);
		env->tx_handle = NULL;
	}
	return (ret);
}

/*
 * __txn_init --
 *	Initialize a transaction region in shared memory.
 */
static int
__txn_init(env, mgr)
	ENV *env;
	DB_TXNMGR *mgr;
{
	DB_ENV *dbenv;
	DB_LSN last_ckp;
	DB_TXNREGION *region;
	int ret;

	dbenv = env->dbenv;

	/*
	 * Find the last checkpoint in the log.
	 */
	ZERO_LSN(last_ckp);
	if (LOGGING_ON(env)) {
		/*
		 * The log system has already walked through the last
		 * file.  Get the LSN of a checkpoint it may have found.
		 */
		if ((ret = __log_get_cached_ckp_lsn(env, &last_ckp)) != 0)
			return (ret);

		/*
		 * If that didn't work, look backwards from the beginning of
		 * the last log file until we find the last checkpoint.
		 */
		if (IS_ZERO_LSN(last_ckp) &&
		    (ret = __txn_findlastckp(env, &last_ckp, NULL)) != 0)
			return (ret);
	}

	if ((ret = __env_alloc(&mgr->reginfo,
	    sizeof(DB_TXNREGION), &mgr->reginfo.primary)) != 0) {
		__db_errx(env, DB_STR("4508",
		    "Unable to allocate memory for the transaction region"));
		return (ret);
	}
	((REGENV *)env->reginfo->primary)->tx_primary =
	     R_OFFSET(&mgr->reginfo, mgr->reginfo.primary);
	region = mgr->reginfo.primary;
	memset(region, 0, sizeof(*region));

	/* We share the region so we need the same mutex. */
	region->mtx_region = ((REGENV *)env->reginfo->primary)->mtx_regenv;
	mgr->reginfo.mtx_alloc = region->mtx_region;

	region->maxtxns = dbenv->tx_max;
	region->inittxns = dbenv->tx_init;
	region->last_txnid = TXN_MINIMUM;
	region->cur_maxid = TXN_MAXIMUM;

	if ((ret = __mutex_alloc(
	    env, MTX_TXN_CHKPT, 0, &region->mtx_ckp)) != 0)
		return (ret);
	region->last_ckp = last_ckp;
	region->time_ckp = time(NULL);

	memset(&region->stat, 0, sizeof(region->stat));
#ifdef HAVE_STATISTICS
	region->stat.st_maxtxns = region->maxtxns;
	region->stat.st_inittxns = region->inittxns;
#endif

	SH_TAILQ_INIT(&region->active_txn);
	SH_TAILQ_INIT(&region->mvcc_txn);
	return (ret);
}

/*
 * __txn_findlastckp --
 *	Find the last checkpoint in the log, walking backwards from the
 *	max_lsn given or the beginning of the last log file.  (The
 *	log system looked through the last log file when it started up.)
 *
 * PUBLIC: int __txn_findlastckp __P((ENV *, DB_LSN *, DB_LSN *));
 */
int
__txn_findlastckp(env, lsnp, max_lsn)
	ENV *env;
	DB_LSN *lsnp;
	DB_LSN *max_lsn;
{
	DBT dbt;
	DB_LOGC *logc;
	DB_LSN lsn;
	int ret, t_ret;
	u_int32_t rectype;

	ZERO_LSN(*lsnp);

	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);

	/* Get the last LSN. */
	memset(&dbt, 0, sizeof(dbt));
	if (max_lsn != NULL) {
		lsn = *max_lsn;
		if ((ret = __logc_get(logc, &lsn, &dbt, DB_SET)) != 0)
			goto err;
	} else {
		if ((ret = __logc_get(logc, &lsn, &dbt, DB_LAST)) != 0)
			goto err;
		/*
		 * Twiddle the last LSN so it points to the beginning of the
		 * last file; we know there's no checkpoint after that, since
		 * the log system already looked there.
		 */
		lsn.offset = 0;
	}

	/* Read backwards, looking for checkpoints. */
	while ((ret = __logc_get(logc, &lsn, &dbt, DB_PREV)) == 0) {
		if (dbt.size < sizeof(u_int32_t))
			continue;
		LOGCOPY_32(env, &rectype, dbt.data);
		if (rectype == DB___txn_ckp) {
			*lsnp = lsn;
			break;
		}
	}

err:	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Not finding a checkpoint is not an error;  there may not exist
	 * one in the log.
	 */
	return ((ret == 0 || ret == DB_NOTFOUND) ? 0 : ret);
}

/*
 * __txn_env_refresh --
 *	Clean up after the transaction system on a close or failed open.
 *
 * PUBLIC: int __txn_env_refresh __P((ENV *));
 */
int
__txn_env_refresh(env)
	ENV *env;
{
	DB_TXN *txn;
	DB_TXNMGR *mgr;
	REGINFO *reginfo;
	u_int32_t txnid;
	int aborted, ret, t_ret;

	ret = 0;
	mgr = env->tx_handle;
	reginfo = &mgr->reginfo;

	/*
	 * This function can only be called once per process (i.e., not
	 * once per thread), so no synchronization is required.
	 *
	 * The caller is probably doing something wrong if close is called with
	 * active transactions.  Try and abort any active transactions that are
	 * not prepared, but it's quite likely the aborts will fail because
	 * recovery won't find open files.  If we can't abort any of the
	 * unprepared transaction, panic, we have to run recovery to get back
	 * to a known state.
	 */
	aborted = 0;
	if (TAILQ_FIRST(&mgr->txn_chain) != NULL) {
		while ((txn = TAILQ_FIRST(&mgr->txn_chain)) != NULL) {
			/* Prepared transactions are OK. */
			txnid = txn->txnid;
			if (((TXN_DETAIL *)txn->td)->status == TXN_PREPARED) {
				if ((ret = __txn_discard_int(txn, 0)) != 0) {
					__db_err(env, ret, DB_STR_A("4509",
					    "unable to discard txn %#lx",
					    "%#lx"), (u_long)txnid);
					break;
				}
				continue;
			}
			aborted = 1;
			if ((t_ret = __txn_abort(txn)) != 0) {
				__db_err(env, t_ret, DB_STR_A("4510",
				    "unable to abort transaction %#lx", "%#lx"),
				    (u_long)txnid);
				ret = __env_panic(env, t_ret);
				break;
			}
		}
		if (aborted) {
			__db_errx(env, DB_STR("4511",
	"Error: closing the transaction region with active transactions"));
			if (ret == 0)
				ret = EINVAL;
		}
	}

	/* Discard the per-thread lock. */
	if ((t_ret = __mutex_free(env, &mgr->mutex)) != 0 && ret == 0)
		ret = t_ret;

	/* Detach from the region. */
	if (F_ISSET(env, ENV_PRIVATE))
		reginfo->mtx_alloc = MUTEX_INVALID;
	if ((t_ret = __env_region_detach(env, reginfo, 0)) != 0 && ret == 0)
		ret = t_ret;

	__os_free(env, mgr);

	env->tx_handle = NULL;
	return (ret);
}

/*
 * __txn_region_mutex_count --
 *	Return the number of mutexes the txn region will need.
 *
 * PUBLIC: u_int32_t __txn_region_mutex_count __P((ENV *));
 */
u_int32_t
__txn_region_mutex_count(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	/*
	 * We need  a mutex for DB_TXNMGR structure, two mutexes for
	 * the DB_TXNREGION structure.
	 */
	return (1 + 2);
}
/*
 * __txn_region_mutex_max --
 *	Return the number of additional mutexes the txn region will need.
 *
 * PUBLIC: u_int32_t __txn_region_mutex_max __P((ENV *));
 */
u_int32_t
__txn_region_mutex_max(env)
	ENV *env;
{
	DB_ENV *dbenv;
	u_int32_t count;

	dbenv = env->dbenv;

	if ((count = dbenv->tx_max) == 0)
		count = DEF_MAX_TXNS;
	/* We may need a mutex for each MVCC txn. */
	return (count > dbenv->tx_init ? count - dbenv->tx_init : 0);
}

/*
 * __txn_region_size --
 *	 Return the amount of space needed for the txn region.
 * PUBLIC:  size_t __txn_region_size __P((ENV *));
 */
size_t
__txn_region_size(env)
	ENV *env;
{
	DB_ENV *dbenv;
	size_t s;

	dbenv = env->dbenv;

	/*
	 * Make the region large enough to hold the primary transaction region
	 * structure, txn_init transaction detail structures, txn_init chunks of
	 * overhead required by the underlying shared region allocator for each
	 * chunk of memory, txn_max transaction names, at an average of 20
	 * bytes each, and 10KB for safety.
	 */
	s = sizeof(DB_TXNREGION) + dbenv->tx_init *
	    (sizeof(TXN_DETAIL) + __env_alloc_overhead() + 20) + 10 * 1024;
	return (s);
}

/*
 * __txn_region_max --
 *	 Return the additional amount of space needed for the txn region.
 * PUBLIC:  size_t __txn_region_max __P((ENV *));
 */
size_t
__txn_region_max(env)
	ENV *env;
{
	DB_ENV *dbenv;
	size_t s;
	u_int32_t count;

	dbenv = env->dbenv;

	if ((count = dbenv->tx_max) == 0)
		count = DEF_MAX_TXNS;
	if (count <= dbenv->tx_init)
		return (0);
	s = (count - dbenv->tx_init) *
	    (sizeof(TXN_DETAIL) + __env_alloc_overhead() + 20);
	return (s);
}

/*
 * __txn_id_set --
 *	Set the current transaction ID and current maximum unused ID (for
 *	testing purposes only).
 *
 * PUBLIC: int __txn_id_set __P((ENV *, u_int32_t, u_int32_t));
 */
int
__txn_id_set(env, cur_txnid, max_txnid)
	ENV *env;
	u_int32_t cur_txnid, max_txnid;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int ret;

	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_id_set", DB_INIT_TXN);

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;
	region->last_txnid = cur_txnid;
	region->cur_maxid = max_txnid;

	ret = 0;
	if (cur_txnid < TXN_MINIMUM) {
		__db_errx(env, DB_STR_A("4512",
		    "Current ID value %lu below minimum", "%lu"),
		    (u_long)cur_txnid);
		ret = EINVAL;
	}
	if (max_txnid < TXN_MINIMUM) {
		__db_errx(env, DB_STR_A("4513",
		    "Maximum ID value %lu below minimum", "%lu"),
		    (u_long)max_txnid);
		ret = EINVAL;
	}
	return (ret);
}

/*
 * lsn_hi_to_low --
 *	Compare lsns, sorting them from high to low. This is the opposite of
 *	__rep_lsn_cmp.
 */
static int
lsn_hi_to_low(lsn1, lsn2)
	const void *lsn1, *lsn2;
{
	return (LOG_COMPARE((DB_LSN *)lsn2, (DB_LSN *)lsn1));
}

/*
 * __txn_get_readers --
 *	Find the read LSN of all active transactions.
 *	MVCC versions older than the oldest active transaction can safely be
 *	discarded from the cache. MVCC versions not quite so old can be
 *      discarded if they are not visible to any active transaction.
 *
 * Returns:
 *	An error code, or 0.
 *	If 0 was returned, *readers has been filled in with an __os_malloc()'d
 *	array of active transactions with read_lsns, sorted from newest
 *      (largest) to oldest (smallest). *ntxnsp indicates how many are there.
 *	The last lsn is that of the oldest active mvcc-supporting transaction.
 *	The caller must __os_free() *readers whenever it is non-NULL.
 *
 * PUBLIC: int __txn_get_readers __P((ENV *, DB_LSN **, int *));
 */
#define	TXN_READERS_SIZE	64 /* Initial number of LSNs to allocate. */
int
__txn_get_readers(env, readers, ntxnsp)
	ENV *env;
	DB_LSN **readers;
	int *ntxnsp;
{
	DB_LSN current, *lsns;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;
	int cmp, is_sorted, ret;
	unsigned count, txnmax;

	*ntxnsp = 0;
	*readers = NULL;

	if ((mgr = env->tx_handle) == NULL)
		return (0);
	region = mgr->reginfo.primary;
	lsns = NULL;

	if ((ret = __log_current_lsn_int(env, &current, NULL, NULL)) != 0)
		return (ret);

	txnmax = TXN_READERS_SIZE;
	if ((ret = __os_malloc(env, txnmax * sizeof(lsns[0]), &lsns)) != 0)
		return (ret);

	TXN_SYSTEM_LOCK(env);
	/* The array always has at least the current lsn. */
	lsns[0] = current;
	count = 1;
	is_sorted = TRUE;

	/*
	 * Build up our array in most-recent (largest) to first-started (oldest)
	 * order. Delete adjacent dups. Detect when the txns need to be sorted.
	 */
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail) {
		if (IS_MAX_LSN(td->read_lsn) ||
		    (cmp = LOG_COMPARE(&td->read_lsn, &lsns[count - 1])) == 0)
			continue;
		if (cmp > 0)
			is_sorted = FALSE;
		if (count >= txnmax) {
			txnmax += txnmax;
			if ((ret = __os_realloc(env,
			    txnmax * sizeof(lsns[0]), &lsns)) != 0)
				goto err;
		}
		lsns[count] = td->read_lsn;
		count++;
	}

err:
	TXN_SYSTEM_UNLOCK(env);

	if (ret != 0)
		__os_free(env, lsns);
	else {
		if (!is_sorted)
			qsort(lsns, count, sizeof(lsns[0]), lsn_hi_to_low);
		*ntxnsp = (int)count;
		*readers = lsns;
	}
	return (ret);
}

/*
 * __txn_add_buffer --
 *	Add to the count of buffers created by the given transaction.
 *
 * PUBLIC: int __txn_add_buffer __P((ENV *, TXN_DETAIL *));
 */
int
__txn_add_buffer(env, td)
	ENV *env;
	TXN_DETAIL *td;
{
	DB_ASSERT(env, td != NULL);

	MUTEX_LOCK(env, td->mvcc_mtx);
	DB_ASSERT(env, td->mvcc_ref < UINT32_MAX);
	++td->mvcc_ref;
	MUTEX_UNLOCK(env, td->mvcc_mtx);

	COMPQUIET(env, NULL);
	return (0);
}

/*
 * __txn_remove_buffer --
 *	Remove a buffer from a transaction -- free the transaction if necessary.
 *
 * PUBLIC: int __txn_remove_buffer __P((ENV *, TXN_DETAIL *, db_mutex_t));
 */
int
__txn_remove_buffer(env, td, hash_mtx)
	ENV *env;
	TXN_DETAIL *td;
	db_mutex_t hash_mtx;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int need_free, ret;

	DB_ASSERT(env, td != NULL);
	ret = 0;
	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	MUTEX_LOCK(env, td->mvcc_mtx);
	DB_ASSERT(env, td->mvcc_ref > 0);

	/*
	 * We free the transaction detail here only if this is the last
	 * reference and td is on the list of committed snapshot transactions
	 * with active pages.
	 */
	need_free = (--td->mvcc_ref == 0) && F_ISSET(td, TXN_DTL_SNAPSHOT);
	MUTEX_UNLOCK(env, td->mvcc_mtx);

	if (need_free) {
		MUTEX_UNLOCK(env, hash_mtx);

		ret = __mutex_free(env, &td->mvcc_mtx);
		td->mvcc_mtx = MUTEX_INVALID;

		TXN_SYSTEM_LOCK(env);
		SH_TAILQ_REMOVE(&region->mvcc_txn, td, links, __txn_detail);
		STAT_DEC(env,
		    txn, nsnapshot, region->stat.st_nsnapshot, td->txnid);
		__env_alloc_free(&mgr->reginfo, td);
		TXN_SYSTEM_UNLOCK(env);

		MUTEX_READLOCK(env, hash_mtx);
	}

	return (ret);
}
