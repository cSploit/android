/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996
 *	The President and Fellows of Harvard University.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/hmac.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

#define	LOG_FLAGS(txn)						\
		(DB_LOG_COMMIT | (F_ISSET(txn, TXN_SYNC) ?	\
		DB_FLUSH : (F_ISSET(txn, TXN_WRITE_NOSYNC) ?	\
		DB_LOG_WRNOSYNC : 0)))

/*
 * __txn_isvalid enumerated types.  We cannot simply use the transaction
 * statuses, because different statuses need to be handled differently
 * depending on the caller.
 */
typedef enum {
	TXN_OP_ABORT,
	TXN_OP_COMMIT,
	TXN_OP_DISCARD,
	TXN_OP_PREPARE
} txnop_t;

static int  __txn_abort_pp __P((DB_TXN *));
static int  __txn_applied __P((ENV *,
    DB_THREAD_INFO *, DB_COMMIT_INFO *, db_timeout_t));
static void __txn_build_token __P((DB_TXN *, DB_LSN *));
static int  __txn_begin_int __P((DB_TXN *));
static int  __txn_close_cursors __P((DB_TXN *));
static int  __txn_commit_pp __P((DB_TXN *, u_int32_t));
static int  __txn_discard __P((DB_TXN *, u_int32_t));
static int  __txn_dispatch_undo
		__P((ENV *, DB_TXN *, DBT *, DB_LSN *, DB_TXNHEAD *));
static int  __txn_end __P((DB_TXN *, int));
static int  __txn_isvalid __P((const DB_TXN *, txnop_t));
static int  __txn_undo __P((DB_TXN *));
static int __txn_set_commit_token __P((DB_TXN *txn, DB_TXN_TOKEN *));
static void __txn_set_txn_lsnp __P((DB_TXN *, DB_LSN **, DB_LSN **));

#define	TxnAlloc "Unable to allocate a transaction handle"

/*
 * __txn_begin_pp --
 *	ENV->txn_begin pre/post processing.
 *
 * PUBLIC: int __txn_begin_pp __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
 */
int
__txn_begin_pp(dbenv, parent, txnpp, flags)
	DB_ENV *dbenv;
	DB_TXN *parent, **txnpp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_check, ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env, env->tx_handle, "txn_begin", DB_INIT_TXN);

	if ((ret = __db_fchk(env,
	    "txn_begin", flags,
	    DB_IGNORE_LEASE |DB_READ_COMMITTED | DB_READ_UNCOMMITTED |
	    DB_TXN_FAMILY | DB_TXN_NOSYNC | DB_TXN_SNAPSHOT | DB_TXN_SYNC |
	    DB_TXN_WAIT | DB_TXN_WRITE_NOSYNC | DB_TXN_NOWAIT |
	    DB_TXN_BULK)) != 0)
		return (ret);
	if ((ret = __db_fcchk(env, "txn_begin", flags,
	    DB_TXN_WRITE_NOSYNC | DB_TXN_NOSYNC, DB_TXN_SYNC)) != 0)
		return (ret);
	if ((ret = __db_fcchk(env, "txn_begin",
	    flags, DB_TXN_WRITE_NOSYNC, DB_TXN_NOSYNC)) != 0)
		return (ret);
	if (parent != NULL && LF_ISSET(DB_TXN_FAMILY)) {
		__db_errx(env, DB_STR("4521",
		    "Family transactions cannot have parents"));
		return (EINVAL);
	} else if (IS_REAL_TXN(parent) &&
	    !F_ISSET(parent, TXN_SNAPSHOT) && LF_ISSET(DB_TXN_SNAPSHOT)) {
		__db_errx(env, DB_STR("4522",
		    "Child transaction snapshot setting must match parent"));
		return (EINVAL);
	}

	ENV_ENTER(env, ip);

	/* Replication accounts for top-level transactions. */
	rep_check = IS_ENV_REPLICATED(env) &&
	    !IS_REAL_TXN(parent) && !LF_ISSET(DB_TXN_FAMILY);

	if (rep_check && (ret = __op_rep_enter(env, 0, 1)) != 0)
		goto err;

	ret = __txn_begin(env, ip, parent, txnpp, flags);

	/*
	 * We only decrement the count if the operation fails.
	 * Otherwise the count will be decremented when the
	 * txn is resolved by txn_commit, txn_abort, etc.
	 */
	if (ret != 0 && rep_check)
		(void)__op_rep_exit(env);

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_begin --
 *	ENV->txn_begin.
 *
 * This is a wrapper to the actual begin process.  We allocate a DB_TXN
 * structure for the caller and then call into __txn_begin_int code.
 *
 * Internally, we use TXN_DETAIL structures, but the DB_TXN structure
 * provides access to the transaction ID and the offset in the transaction
 * region of the TXN_DETAIL structure.
 *
 * PUBLIC: int __txn_begin __P((ENV *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DB_TXN **, u_int32_t));
 */
int
__txn_begin(env, ip, parent, txnpp, flags)
	ENV *env;
	DB_THREAD_INFO *ip;
	DB_TXN *parent, **txnpp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	DB_LOCKREGION *region;
	DB_TXN *txn;
	TXN_DETAIL *ptd, *td;
	int ret;

	if (F_ISSET(env, ENV_FORCE_TXN_BULK))
		flags |= DB_TXN_BULK;

	*txnpp = NULL;
	if ((ret = __os_calloc(env, 1, sizeof(DB_TXN), &txn)) != 0) {
		__db_errx(env, TxnAlloc);
		return (ret);
	}

	dbenv = env->dbenv;
	txn->mgrp = env->tx_handle;
	txn->parent = parent;
	if (parent != NULL && F_ISSET(parent, TXN_FAMILY))
		parent = NULL;
	TAILQ_INIT(&txn->kids);
	TAILQ_INIT(&txn->events);
	STAILQ_INIT(&txn->logs);
	TAILQ_INIT(&txn->my_cursors);
	TAILQ_INIT(&txn->femfs);
	txn->flags = TXN_MALLOC;
	txn->thread_info =
	     ip != NULL ? ip : (parent != NULL ? parent->thread_info : NULL);

	/*
	 * Set the sync mode for commit.  Any local bits override those
	 * in the environment.  SYNC is the default.
	 */
	if (LF_ISSET(DB_TXN_SYNC))
		F_SET(txn, TXN_SYNC);
	else if (LF_ISSET(DB_TXN_NOSYNC))
		F_SET(txn, TXN_NOSYNC);
	else if (LF_ISSET(DB_TXN_WRITE_NOSYNC))
		F_SET(txn, TXN_WRITE_NOSYNC);
	else if (F_ISSET(dbenv, DB_ENV_TXN_NOSYNC))
		F_SET(txn, TXN_NOSYNC);
	else if (F_ISSET(dbenv, DB_ENV_TXN_WRITE_NOSYNC))
		F_SET(txn, TXN_WRITE_NOSYNC);
	else
		F_SET(txn, TXN_SYNC);

	if (LF_ISSET(DB_TXN_NOWAIT) ||
	    (F_ISSET(dbenv, DB_ENV_TXN_NOWAIT) && !LF_ISSET(DB_TXN_WAIT)))
		F_SET(txn, TXN_NOWAIT);
	if (LF_ISSET(DB_READ_COMMITTED))
		F_SET(txn, TXN_READ_COMMITTED);
	if (LF_ISSET(DB_READ_UNCOMMITTED))
		F_SET(txn, TXN_READ_UNCOMMITTED);
	if (LF_ISSET(DB_TXN_FAMILY))
		F_SET(txn, TXN_FAMILY | TXN_INFAMILY | TXN_READONLY);
	if (LF_ISSET(DB_TXN_SNAPSHOT) || F_ISSET(dbenv, DB_ENV_TXN_SNAPSHOT) ||
	    (parent != NULL && F_ISSET(parent, TXN_SNAPSHOT))) {
		if (IS_REP_CLIENT(env)) {
			__db_errx(env, DB_STR("4572",
		"DB_TXN_SNAPSHOT may not be used on a replication client"));
			ret = (EINVAL);
			goto err;
		} else
			F_SET(txn, TXN_SNAPSHOT);
	}
	if (LF_ISSET(DB_IGNORE_LEASE))
		F_SET(txn, TXN_IGNORE_LEASE);

	/*
	 * We set TXN_BULK only for the outermost transaction.  This
	 * is a temporary limitation; in the future we will allow it
	 * for nested transactions as well.  See #17669 for details.
	 *
	 * Also, ignore requests for DB_TXN_BULK if replication is enabled.
	 */
	if (LF_ISSET(DB_TXN_BULK) && parent == NULL && !REP_ON(txn->mgrp->env))
		F_SET(txn, TXN_BULK);

	if ((ret = __txn_begin_int(txn)) != 0)
		goto err;
	td = txn->td;

	if (parent != NULL) {
		ptd = parent->td;
		TAILQ_INSERT_HEAD(&parent->kids, txn, klinks);
		SH_TAILQ_INSERT_HEAD(&ptd->kids, td, klinks, __txn_detail);
	}

	if (LOCKING_ON(env)) {
		region = env->lk_handle->reginfo.primary;
		if (parent != NULL) {
			ret = __lock_inherit_timeout(env,
			    parent->locker, txn->locker);
			/* No parent locker set yet. */
			if (ret == EINVAL) {
				parent = NULL;
				ret = 0;
			}
			if (ret != 0)
				goto err;
		}

		/*
		 * Parent is NULL if we have no parent
		 * or it has no timeouts set.
		 */
		if (parent == NULL && region->tx_timeout != 0)
			if ((ret = __lock_set_timeout(env, txn->locker,
			    region->tx_timeout, DB_SET_TXN_TIMEOUT)) != 0)
				goto err;
	}

	*txnpp = txn;
	PERFMON2(env, txn, begin, txn->txnid, flags);
	return (0);

err:
	__os_free(env, txn);
	return (ret);
}

/*
 * __txn_recycle_id --
 *	Find a range of useable transaction ids.
 *
 * PUBLIC: int __txn_recycle_id __P((ENV *, int));
 */
int
__txn_recycle_id(env, locked)
	ENV *env;
	int locked;
{
	DB_LSN null_lsn;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;
	u_int32_t *ids;
	int nids, ret;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	if ((ret = __os_malloc(env,
	    sizeof(u_int32_t) * region->curtxns, &ids)) != 0) {
		__db_errx(env, DB_STR("4523",
		    "Unable to allocate transaction recycle buffer"));
		return (ret);
	}
	nids = 0;
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail)
		ids[nids++] = td->txnid;
	region->last_txnid = TXN_MINIMUM - 1;
	region->cur_maxid = TXN_MAXIMUM;
	if (nids != 0)
		__db_idspace(ids, nids,
		    &region->last_txnid, &region->cur_maxid);
	__os_free(env, ids);

	/*
	 * Check LOGGING_ON rather than DBENV_LOGGING as we want to emit this
	 * record at the end of recovery.
	 */
	if (LOGGING_ON(env)) {
		if (locked)
			TXN_SYSTEM_UNLOCK(env);
		ret = __txn_recycle_log(env, NULL, &null_lsn,
		     0, region->last_txnid + 1, region->cur_maxid);
		/* Make it simple on the caller, if error we hold the lock. */
		if (locked && ret != 0)
			TXN_SYSTEM_LOCK(env);
	}

	return (ret);
}

/*
 * __txn_begin_int --
 *	Normal DB version of txn_begin.
 */
static int
__txn_begin_int(txn)
	DB_TXN *txn;
{
	DB_ENV *dbenv;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	ENV *env;
	TXN_DETAIL *td;
	u_int32_t id;
	int inserted, ret;

	mgr = txn->mgrp;
	env = mgr->env;
	dbenv = env->dbenv;
	region = mgr->reginfo.primary;
	td = NULL;
	inserted = 0;

	TXN_SYSTEM_LOCK(env);
	if (!F_ISSET(txn, TXN_COMPENSATE) && F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_errx(env, DB_STR("4524",
		    "operation not permitted during recovery"));
		ret = EINVAL;
		goto err;
	}

	/*
	 * Allocate a new transaction id. Our current valid range can span
	 * the maximum valid value, so check for it and wrap manually.
	 */
	if (region->last_txnid == TXN_MAXIMUM &&
	    region->cur_maxid != TXN_MAXIMUM)
		region->last_txnid = TXN_MINIMUM - 1;

	/* Allocate a new transaction detail structure. */
	if ((ret =
	    __env_alloc(&mgr->reginfo, sizeof(TXN_DETAIL), &td)) != 0) {
		__db_errx(env, DB_STR("4525",
		    "Unable to allocate memory for transaction detail"));
		goto err;
	}

	id = ++region->last_txnid;

#ifdef HAVE_STATISTICS
	STAT_INC(env, txn, nbegins, region->stat.st_nbegins, id);
	STAT_INC(env, txn, nactive, region->stat.st_nactive, id);
	if (region->stat.st_nactive > region->stat.st_maxnactive)
		STAT_SET(env, txn, maxnactive,
		    region->stat.st_maxnactive, region->stat.st_nactive, id);
#endif

	td->txnid = id;
	dbenv->thread_id(dbenv, &td->pid, &td->tid);

	ZERO_LSN(td->last_lsn);
	ZERO_LSN(td->begin_lsn);
	SH_TAILQ_INIT(&td->kids);
	if (txn->parent != NULL && !F_ISSET(txn->parent, TXN_FAMILY))
		td->parent = R_OFFSET(&mgr->reginfo, txn->parent->td);
	else
		td->parent = INVALID_ROFF;
	td->name = INVALID_ROFF;
	MAX_LSN(td->read_lsn);
	MAX_LSN(td->visible_lsn);
	td->mvcc_ref = 0;
	td->mvcc_mtx = MUTEX_INVALID;
	td->status = TXN_RUNNING;
	td->flags = F_ISSET(txn, TXN_NOWAIT) ? TXN_DTL_NOWAIT : 0;
	td->nlog_dbs = 0;
	td->nlog_slots = TXN_NSLOTS;
	td->log_dbs = R_OFFSET(&mgr->reginfo, td->slots);

	/* XA specific fields. */
	td->xa_ref = 1;
	td->xa_br_status = TXN_XA_IDLE;

	/* Place transaction on active transaction list. */
	SH_TAILQ_INSERT_HEAD(&region->active_txn, td, links, __txn_detail);
	region->curtxns++;

	/* Increment bulk transaction counter while holding transaction lock. */
	if (F_ISSET(txn, TXN_BULK))
		((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_bulk_txn++;

	inserted = 1;

	if (region->last_txnid == region->cur_maxid) {
		if ((ret = __txn_recycle_id(env, 1)) != 0)
			goto err;
	} else
		TXN_SYSTEM_UNLOCK(env);

	txn->txnid = id;
	txn->td  = td;

	/* Allocate a locker for this txn. */
	if (LOCKING_ON(env) && (ret =
		__lock_getlocker(env->lk_handle, id, 1, &txn->locker)) != 0)
			goto err;

	txn->abort = __txn_abort_pp;
	txn->commit = __txn_commit_pp;
	txn->discard = __txn_discard;
	txn->get_name = __txn_get_name;
	txn->get_priority = __txn_get_priority;
	txn->id = __txn_id;
	txn->prepare = __txn_prepare;
	txn->set_commit_token = __txn_set_commit_token;
	txn->set_txn_lsnp = __txn_set_txn_lsnp;
	txn->set_name = __txn_set_name;
	txn->set_priority = __txn_set_priority;
	txn->set_timeout = __txn_set_timeout;

	/* We can't call __txn_set_priority until txn->td is set. */
	if (LOCKING_ON(env) && (ret = __txn_set_priority(txn,
		txn->parent == NULL ?
		TXN_PRIORITY_DEFAULT : txn->parent->locker->priority)) != 0)
		goto err;
	else
		td->priority = 0;

	/*
	 * If this is a transaction family, we must link the child to the
	 * maximal grandparent in the lock table for deadlock detection.
	 */
	if (txn->parent != NULL) {
		if (LOCKING_ON(env) && (ret = __lock_addfamilylocker(env,
		    txn->parent->txnid, txn->txnid,
		    F_ISSET(txn->parent, TXN_FAMILY))) != 0)
			goto err;

		/*
		 * If the parent is only used to establish compatability, do
		 * not reference it again.
		 */
		if (F_ISSET(txn->parent, TXN_FAMILY)) {
			txn->parent = NULL;
			F_SET(txn, TXN_INFAMILY);
		}
	}

	if (F_ISSET(txn, TXN_MALLOC)) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_INSERT_TAIL(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);
	}

	return (0);

err:	if (inserted) {
		TXN_SYSTEM_LOCK(env);
		SH_TAILQ_REMOVE(&region->active_txn, td, links, __txn_detail);
		region->curtxns--;
		if (F_ISSET(txn, TXN_BULK))
			((DB_TXNREGION *)
			 env->tx_handle->reginfo.primary)->n_bulk_txn--;
	}
	if (td != NULL)
		__env_alloc_free(&mgr->reginfo, td);
	TXN_SYSTEM_UNLOCK(env);
	return (ret);
}

/*
 * __txn_continue
 *	Fill in the fields of the local transaction structure given
 *	the detail transaction structure.  Optionally link transactions
 *	to transaction manager list.
 *
 * PUBLIC: int __txn_continue __P((ENV *,
 * PUBLIC:     DB_TXN *, TXN_DETAIL *, DB_THREAD_INFO *, int));
 */
int
__txn_continue(env, txn, td, ip, add_to_list)
	ENV *env;
	DB_TXN *txn;
	TXN_DETAIL *td;
	DB_THREAD_INFO *ip;
	int add_to_list;
{
	DB_LOCKREGION *region;
	DB_TXNMGR *mgr;
	int ret;

	ret = 0;

	/*
	 * This code follows the order of the structure definition so it
	 * is relatively easy to make sure that we are setting everything.
	 */
	mgr = txn->mgrp = env->tx_handle;
	txn->parent = NULL;
	txn->thread_info = ip;
	txn->txnid = td->txnid;
	txn->name = NULL;
	txn->td = td;
	td->xa_ref++;

	/* This never seems to be used: txn->expire */
	txn->txn_list = NULL;

	TAILQ_INIT(&txn->kids);
	TAILQ_INIT(&txn->events);
	STAILQ_INIT(&txn->logs);

	/*
	 * These fields should never persist across different processes as we
	 * require that cursors be opened/closed within the same service routine
	 * and we disallow file level operations in XA transactions.
	 */
	TAILQ_INIT(&txn->my_cursors);
	TAILQ_INIT(&txn->femfs);

	/* Put the transaction onto the transaction manager's list. */
	if (add_to_list) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_INSERT_TAIL(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);
	}

	txn->token_buffer = 0;
	txn->cursors = 0;

	txn->abort = __txn_abort_pp;
	txn->commit = __txn_commit_pp;
	txn->discard = __txn_discard;
	txn->get_name = __txn_get_name;
	txn->get_priority = __txn_get_priority;
	txn->id = __txn_id;
	txn->prepare = __txn_prepare;
	txn->set_commit_token = __txn_set_commit_token;
	txn->set_name = __txn_set_name;
	txn->set_priority = __txn_set_priority;
	txn->set_timeout = __txn_set_timeout;
	txn->set_txn_lsnp = __txn_set_txn_lsnp;

	txn->flags = TXN_MALLOC | TXN_SYNC |
	    (F_ISSET(td, TXN_DTL_NOWAIT) ? TXN_NOWAIT : 0);
	txn->xa_thr_status = TXN_XA_THREAD_NOTA;

	/*
	 * If this is a restored transaction, we need to propagate that fact
	 * to the process-local structure.  However, if it's not a restored
	 * transaction, we need to make sure that we have a locker associated
	 * with this transaction.
	 */
	if (F_ISSET(td, TXN_DTL_RESTORED))
		F_SET(txn, TXN_RESTORED);
	else
		if ((ret = __lock_getlocker(env->lk_handle,
		    txn->txnid, 0, &txn->locker)) == 0)
			ret = __txn_set_priority(txn, td->priority);

	if (LOCKING_ON(env)) {
		region = env->lk_handle->reginfo.primary;
		if (region->tx_timeout != 0 &&
		    (ret = __lock_set_timeout(env, txn->locker,
		    region->tx_timeout, DB_SET_TXN_TIMEOUT)) != 0)
			return (ret);
		txn->lock_timeout = region->tx_timeout;
	}

	return (ret);
}

/*
 * __txn_commit_pp --
 *	Interface routine to TXN->commit.
 */
static int
__txn_commit_pp(txn, flags)
	DB_TXN *txn;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_check, ret, t_ret;

	env = txn->mgrp->env;
	rep_check = IS_ENV_REPLICATED(env) &&
	    txn->parent == NULL && IS_REAL_TXN(txn);

	ENV_ENTER(env, ip);
	ret = __txn_commit(txn, flags);
	if (rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_commit --
 *	Commit a transaction.
 *
 * PUBLIC: int __txn_commit __P((DB_TXN *, u_int32_t));
 */
int
__txn_commit(txn, flags)
	DB_TXN *txn;
	u_int32_t flags;
{
	DBT list_dbt;
	DB_LOCKREQ request;
	DB_TXN *kid;
	ENV *env;
	REGENV *renv;
	REGINFO *infop;
	TXN_DETAIL *td;
	DB_LSN token_lsn;
	u_int32_t id;
	int ret, t_ret;

	env = txn->mgrp->env;
	td = txn->td;
	PERFMON2(env, txn, commit, txn->txnid, flags);

	DB_ASSERT(env, txn->xa_thr_status == TXN_XA_THREAD_NOTA ||
	    td->xa_ref == 1);
	/*
	 * A common mistake in Berkeley DB programs is to mis-handle deadlock
	 * return.  If the transaction deadlocked, they want abort, not commit.
	 */
	if (F_ISSET(txn, TXN_DEADLOCK)) {
		ret = __db_txn_deadlock_err(env, txn);
		goto err;
	}

	/* Close registered cursors before committing. */
	if ((ret = __txn_close_cursors(txn)) != 0)
		goto err;

	if ((ret = __txn_isvalid(txn, TXN_OP_COMMIT)) != 0)
		return (ret);

	/*
	 * Check for master leases at the beginning.  If we are a master and
	 * cannot have valid leases now, we error and abort this txn.  There
	 * should always be a perm record in the log because the master updates
	 * the LSN history system database in rep_start() (with IGNORE_LEASE
	 * set).
	 *
	 * Only check leases if this txn writes to the log file
	 * (i.e. td->last_lsn).
	 */
	if (txn->parent == NULL && IS_REP_MASTER(env) &&
	    IS_USING_LEASES(env) && !F_ISSET(txn, TXN_IGNORE_LEASE) &&
	    !IS_ZERO_LSN(td->last_lsn) &&
	    (ret = __rep_lease_check(env, 1)) != 0) {
		DB_ASSERT(env, ret != DB_NOTFOUND);
		goto err;
	}

	infop = env->reginfo;
	renv = infop->primary;
	/*
	 * No mutex is needed as envid is read-only once it is set.
	 */
	id = renv->envid;

	/*
	 * We clear flags that are incorrect, ignoring any flag errors, and
	 * default to synchronous operations.  By definition, transaction
	 * handles are dead when we return, and this error should never
	 * happen, but we don't want to fail in the field 'cause the app is
	 * specifying the wrong flag for some reason.
	 */
	if (__db_fchk(env, "DB_TXN->commit", flags,
	    DB_TXN_NOSYNC | DB_TXN_SYNC | DB_TXN_WRITE_NOSYNC) != 0)
		flags = DB_TXN_SYNC;
	if (__db_fcchk(env, "DB_TXN->commit", flags,
	    DB_TXN_SYNC, DB_TXN_NOSYNC | DB_TXN_WRITE_NOSYNC) != 0)
		flags = DB_TXN_SYNC;

	if (LF_ISSET(DB_TXN_WRITE_NOSYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_WRITE_NOSYNC);
	}
	if (LF_ISSET(DB_TXN_NOSYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_NOSYNC);
	}
	if (LF_ISSET(DB_TXN_SYNC)) {
		F_CLR(txn, TXN_SYNC_FLAGS);
		F_SET(txn, TXN_SYNC);
	}

	DB_ASSERT(env, F_ISSET(txn, TXN_SYNC_FLAGS));

	/*
	 * Commit any unresolved children.  If anyone fails to commit,
	 * then try to abort the rest of the kids and then abort the parent.
	 * Abort should never fail; if it does, we bail out immediately.
	 */
	while ((kid = TAILQ_FIRST(&txn->kids)) != NULL)
		if ((ret = __txn_commit(kid, flags)) != 0)
			while ((kid = TAILQ_FIRST(&txn->kids)) != NULL)
				if ((t_ret = __txn_abort(kid)) != 0)
					return (__env_panic(env, t_ret));

	/*
	 * If there are any log records, write a log record and sync the log,
	 * else do no log writes.  If the commit is for a child transaction,
	 * we do not need to commit the child synchronously since it may still
	 * abort (if its parent aborts), and otherwise its parent or ultimate
	 * ancestor will write synchronously.
	 */
	ZERO_LSN(token_lsn);
	if (DBENV_LOGGING(env) && (!IS_ZERO_LSN(td->last_lsn) ||
	    STAILQ_FIRST(&txn->logs) != NULL)) {
		if (txn->parent == NULL) {
			/*
			 * We are about to free all the read locks for this
			 * transaction below.  Some of those locks might be
			 * handle locks which should not be freed, because
			 * they will be freed when the handle is closed. Check
			 * the events and preprocess any trades now so we don't
			 * release the locks below.
			 */
			if ((ret =
			    __txn_doevents(env, txn, TXN_COMMIT, 1)) != 0)
				goto err;

			memset(&request, 0, sizeof(request));
			if (LOCKING_ON(env)) {
				request.op = DB_LOCK_PUT_READ;
				if (IS_REP_MASTER(env) &&
				    !IS_ZERO_LSN(td->last_lsn)) {
					memset(&list_dbt, 0, sizeof(list_dbt));
					request.obj = &list_dbt;
				}
				ret = __lock_vec(env,
				    txn->locker, 0, &request, 1, NULL);
			}

			if (ret == 0 && !IS_ZERO_LSN(td->last_lsn)) {
				ret = __txn_flush_fe_files(txn);
				if (ret == 0)
					ret = __txn_regop_log(env, txn,
					    &td->visible_lsn, LOG_FLAGS(txn),
					    TXN_COMMIT,
					    (int32_t)time(NULL), id,
					    request.obj);
				if (ret == 0)
					token_lsn = td->last_lsn =
					    td->visible_lsn;
#ifdef DIAGNOSTIC
				if (ret == 0) {
					DB_LSN s_lsn;

					DB_ASSERT(env, __log_current_lsn_int(
					    env, &s_lsn, NULL, NULL) == 0);
					DB_ASSERT(env, LOG_COMPARE(
					    &td->visible_lsn, &s_lsn) <= 0);
					COMPQUIET(s_lsn.file, 0);
				}
#endif
			}

			if (request.obj != NULL && request.obj->data != NULL)
				__os_free(env, request.obj->data);
			if (ret != 0)
				goto err;
		} else {
			/* Log the commit in the parent! */
			if (!IS_ZERO_LSN(td->last_lsn) &&
			    (ret = __txn_child_log(env, txn->parent,
			    &((TXN_DETAIL *)txn->parent->td)->last_lsn,
			    0, txn->txnid, &td->last_lsn)) != 0) {
				goto err;
			}
			if (STAILQ_FIRST(&txn->logs) != NULL) {
				/*
				 * Put the child first so we back it out first.
				 * All records are undone in reverse order.
				 */
				STAILQ_CONCAT(&txn->logs, &txn->parent->logs);
				txn->parent->logs = txn->logs;
				STAILQ_INIT(&txn->logs);
			}

			F_SET(txn->parent, TXN_CHILDCOMMIT);
		}
	}
	if (txn->token_buffer != NULL && ret == 0 && DBENV_LOGGING(env))
		__txn_build_token(txn, &token_lsn);

	if (txn->txn_list != NULL) {
		__db_txnlist_end(env, txn->txn_list);
		txn->txn_list = NULL;
	}

	if (ret != 0)
		goto err;

	/*
	 * Check for master leases at the end of only a normal commit.
	 * If we're a child, that is not a perm record.  If we are a
	 * master and cannot get valid leases now, something happened
	 * during the commit.  The only thing to do is panic.
	 *
	 * Only check leases if this txn writes to the log file
	 * (i.e. td->last_lsn).
	 */
	if (txn->parent == NULL && IS_REP_MASTER(env) &&
	    IS_USING_LEASES(env) && !F_ISSET(txn, TXN_IGNORE_LEASE) &&
	    !IS_ZERO_LSN(td->last_lsn) &&
	    (ret = __rep_lease_check(env, 1)) != 0)
		return (__env_panic(env, ret));

	/*
	 * This is here rather than in __txn_end because __txn_end is
	 * called too late during abort.  So commit and abort each
	 * call it independently.
	 */
	__txn_reset_fe_watermarks(txn);

	/* This is OK because __txn_end can only fail with a panic. */
	return (__txn_end(txn, 1));

err:	/*
	 * If we are prepared, then we "must" be able to commit.  We panic here
	 * because even though the coordinator might be able to retry it is not
	 * clear it would know to do that.  Otherwise  we'll try to abort.  If
	 * that is successful, then we return whatever was in ret (that is, the
	 * reason we failed).  If the abort was unsuccessful, abort probably
	 * returned DB_RUNRECOVERY and we need to propagate that up.
	 */
	if (td->status == TXN_PREPARED)
		return (__env_panic(env, ret));

	if ((t_ret = __txn_abort(txn)) != 0)
		ret = t_ret;
	return (ret);
}

/*
 * __txn_close_cursors
 *	Close a transaction's registered cursors, all its cursors are
 *	guaranteed to be closed.
 */
static int
__txn_close_cursors(txn)
	DB_TXN *txn;
{
	int ret, tret;
	DBC *dbc;

	ret = tret = 0;
	dbc = NULL;

	if (txn == NULL)
		return (0);

	while ((dbc = TAILQ_FIRST(&txn->my_cursors)) != NULL) {

		DB_ASSERT(dbc->env, txn == dbc->txn);

		/*
		 * Unregister the cursor from its transaction, regardless
		 * of return.
		 */
		TAILQ_REMOVE(&(txn->my_cursors), dbc, txn_cursors);
		dbc->txn_cursors.tqe_next = NULL;
		dbc->txn_cursors.tqe_prev = NULL;

		/* Removed from the active queue here. */
		if (F_ISSET(dbc, DBC_ACTIVE))
			ret = __dbc_close(dbc);

		dbc->txn = NULL;

		/* We have to close all cursors anyway, so continue on error. */
		if (ret != 0) {
			__db_err(dbc->env, ret, "__dbc_close");
			if (tret == 0)
				tret = ret;
		}
	}
	txn->my_cursors.tqh_first = NULL;
	txn->my_cursors.tqh_last = NULL;

	return (tret);/* Return the first error if any. */
}

/*
 * __txn_set_commit_token --
 *	Store a pointer to user's commit token buffer, for later use.
 */
static int
__txn_set_commit_token(txn, tokenp)
	DB_TXN *txn;
	DB_TXN_TOKEN *tokenp;
{
	ENV *env;

	env = txn->mgrp->env;
	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_TXN->set_commit_token", DB_INIT_LOG);
	if (txn->parent != NULL) {
		__db_errx(env, DB_STR("4526",
		    "commit token unavailable for nested txn"));
		return (EINVAL);
	}
	if (IS_REP_CLIENT(env)) {
		__db_errx(env, DB_STR("4527",
		    "may not be called on a replication client"));
		return (EINVAL);
	}

	txn->token_buffer = tokenp;

#ifdef DIAGNOSTIC
	/*
	 * Applications may rely on the contents of the token buffer becoming
	 * valid only after a successful commit().  So it is not strictly
	 * necessary to initialize the buffer here.  But in case they get
	 * confused we initialize it here to a recognizably invalid value.
	 */
	memset(tokenp, 0, DB_TXN_TOKEN_SIZE);
#endif

	return (0);
}

/*
 * __txn_build_token --
 *	Stash a token describing the committing transaction into the buffer
 * previously designated by the user.  Called only in the case where the user
 * has indeed supplied a buffer address.
 */
static void
__txn_build_token(txn, lsnp)
	DB_TXN *txn;
	DB_LSN *lsnp;
{
	ENV *env;
	REGENV *renv;
	u_int8_t *bp;
	u_int32_t gen, version;

	bp = txn->token_buffer->buf;
	env = txn->mgrp->env;
	renv = env->reginfo->primary;

	/* Marshal the information into external form. */
	version = REP_COMMIT_TOKEN_FMT_VERSION;
	gen = REP_ON(env) ? env->rep_handle->region->gen : 0;
	DB_HTONL_COPYOUT(env, bp, version);
	DB_HTONL_COPYOUT(env, bp, gen);
	DB_HTONL_COPYOUT(env, bp, renv->envid);
	DB_HTONL_COPYOUT(env, bp, lsnp->file);
	DB_HTONL_COPYOUT(env, bp, lsnp->offset);
}

/*
 * __txn_abort_pp --
 *	Interface routine to TXN->abort.
 */
static int
__txn_abort_pp(txn)
	DB_TXN *txn;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_check, ret, t_ret;

	env = txn->mgrp->env;
	rep_check = IS_ENV_REPLICATED(env) &&
	    txn->parent == NULL && IS_REAL_TXN(txn);

	ENV_ENTER(env, ip);
	ret = __txn_abort(txn);
	if (rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_abort --
 *	Abort a transaction.
 *
 * PUBLIC: int __txn_abort __P((DB_TXN *));
 */
int
__txn_abort(txn)
	DB_TXN *txn;
{
	DB_LOCKREQ request;
	DB_TXN *kid;
	ENV *env;
	REGENV *renv;
	REGINFO *infop;
	TXN_DETAIL *td;
	u_int32_t id;
	int ret;

	env = txn->mgrp->env;
	td = txn->td;
	/*
	 * Do not abort an XA transaction if another process is still using
	 * it, however make sure that it is aborted when the last process
	 * tries to abort it.
	 */
	if (txn->xa_thr_status != TXN_XA_THREAD_NOTA &&  td->xa_ref > 1) {
		td->status = TXN_NEED_ABORT;
		return (0);
	}

	PERFMON1(env, txn, abort, txn->txnid);
	/*
	 * Close registered cursors before the abort. Even if the call fails,
	 * all cursors are closed.
	 */
	if ((ret = __txn_close_cursors(txn)) != 0)
		return (__env_panic(env, ret));

	/* Ensure that abort always fails fatally. */
	if ((ret = __txn_isvalid(txn, TXN_OP_ABORT)) != 0)
		return (__env_panic(env, ret));

	/*
	 * Clear the watermarks now.  Can't do this in __txn_end because
	 * __db_refresh, called from undo, will free the DB_MPOOLFILEs.
	 */
	__txn_reset_fe_watermarks(txn);

	/*
	 * Try to abort any unresolved children.
	 *
	 * Abort either succeeds or panics the region.  As soon as we
	 * see any failure, we just get out of here and return the panic
	 * up.
	 */
	while ((kid = TAILQ_FIRST(&txn->kids)) != NULL)
		if ((ret = __txn_abort(kid)) != 0)
			return (ret);

	infop = env->reginfo;
	renv = infop->primary;
	/*
	 * No mutex is needed as envid is read-only once it is set.
	 */
	id = renv->envid;

	/*
	 * Fast path -- no need to do anything fancy if there were no
	 * modifications (e.g., log records) for this transaction.
	 * We still call txn_undo to cleanup the txn_list from our
	 * children.
	 */
	if (IS_ZERO_LSN(td->last_lsn) && STAILQ_FIRST(&txn->logs) == NULL) {
		if (txn->txn_list == NULL)
			goto done;
		else
			goto undo;
	}

	if (LOCKING_ON(env)) {
		/* Allocate a locker for this restored txn if necessary. */
		if (txn->locker == NULL &&
		    (ret = __lock_getlocker(env->lk_handle,
		    txn->txnid, 1, &txn->locker)) != 0)
			return (__env_panic(env, ret));
		/*
		 * We are about to free all the read locks for this transaction
		 * below.  Some of those locks might be handle locks which
		 * should not be freed, because they will be freed when the
		 * handle is closed.  Check the events and preprocess any
		 * trades now so that we don't release the locks below.
		 */
		if ((ret = __txn_doevents(env, txn, TXN_ABORT, 1)) != 0)
			return (__env_panic(env, ret));

		/* Turn off timeouts. */
		if ((ret = __lock_set_timeout(env,
		    txn->locker, 0, DB_SET_TXN_TIMEOUT)) != 0)
			return (__env_panic(env, ret));

		if ((ret = __lock_set_timeout(env,
		    txn->locker, 0, DB_SET_LOCK_TIMEOUT)) != 0)
			return (__env_panic(env, ret));

		request.op = DB_LOCK_UPGRADE_WRITE;
		request.obj = NULL;
		if ((ret = __lock_vec(
		    env, txn->locker, 0, &request, 1, NULL)) != 0)
			return (__env_panic(env, ret));
	}
undo:	if ((ret = __txn_undo(txn)) != 0)
		return (__env_panic(env, ret));

	/*
	 * Normally, we do not need to log aborts.  However, if we
	 * are a distributed transaction (i.e., we have a prepare),
	 * then we log the abort so we know that this transaction
	 * was actually completed.
	 */
done:	 if (DBENV_LOGGING(env) && td->status == TXN_PREPARED &&
	    (ret = __txn_regop_log(env, txn, &td->last_lsn,
	    LOG_FLAGS(txn), TXN_ABORT, (int32_t)time(NULL), id, NULL)) != 0)
		return (__env_panic(env, ret));

	/* __txn_end always panics if it errors, so pass the return along. */
	return (__txn_end(txn, 0));
}

/*
 * __txn_discard --
 *	Interface routine to TXN->discard.
 */
static int
__txn_discard(txn, flags)
	DB_TXN *txn;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_check, ret, t_ret;

	env = txn->mgrp->env;
	rep_check = IS_ENV_REPLICATED(env) &&
	    txn->parent == NULL && IS_REAL_TXN(txn);

	ENV_ENTER(env, ip);
	ret = __txn_discard_int(txn, flags);
	if (rep_check && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_discard --
 *	Free the per-process resources associated with this txn handle.
 *
 * PUBLIC: int __txn_discard_int __P((DB_TXN *, u_int32_t flags));
 */
int
__txn_discard_int(txn, flags)
	DB_TXN *txn;
	u_int32_t flags;
{
	DB_TXNMGR *mgr;
	ENV *env;
	int ret;

	COMPQUIET(flags, 0);

	mgr = txn->mgrp;
	env = mgr->env;

	/* Close registered cursors. */
	if ((ret = __txn_close_cursors(txn)) != 0)
		return (ret);

	if ((ret = __txn_isvalid(txn, TXN_OP_DISCARD)) != 0)
		return (ret);

	/* Should be no children. */
	DB_ASSERT(env, TAILQ_FIRST(&txn->kids) == NULL);

	/* Free the space. */
	MUTEX_LOCK(env, mgr->mutex);
	mgr->n_discards++;
	if (F_ISSET(txn, TXN_MALLOC)) {
		TAILQ_REMOVE(&mgr->txn_chain, txn, links);
	}
	MUTEX_UNLOCK(env, mgr->mutex);
	if (F_ISSET(txn, TXN_MALLOC) &&
	    txn->xa_thr_status != TXN_XA_THREAD_ASSOCIATED)
		__os_free(env, txn);

	return (0);
}

/*
 * __txn_prepare --
 *	Flush the log so a future commit is guaranteed to succeed.
 *
 * PUBLIC: int __txn_prepare __P((DB_TXN *, u_int8_t *));
 */
int
__txn_prepare(txn, gid)
	DB_TXN *txn;
	u_int8_t *gid;
{
	DBT list_dbt, gid_dbt;
	DB_LOCKREQ request;
	DB_THREAD_INFO *ip;
	DB_TXN *kid;
	ENV *env;
	TXN_DETAIL *td;
	u_int32_t lflags;
	int ret;

	env = txn->mgrp->env;
	td = txn->td;
	PERFMON2(env, txn, prepare, txn->txnid, gid);
	DB_ASSERT(env, txn->xa_thr_status == TXN_XA_THREAD_NOTA ||
	    td->xa_ref == 1);
	ENV_ENTER(env, ip);

	/* Close registered cursors. */
	if ((ret = __txn_close_cursors(txn)) != 0)
		goto err;

	if ((ret = __txn_isvalid(txn, TXN_OP_PREPARE)) != 0)
		goto err;
	if (F_ISSET(txn, TXN_DEADLOCK)) {
		ret = __db_txn_deadlock_err(env, txn);
		goto err;
	}

	/* Commit any unresolved children. */
	while ((kid = TAILQ_FIRST(&txn->kids)) != NULL)
		if ((ret = __txn_commit(kid, DB_TXN_NOSYNC)) != 0)
			goto err;

	/* We must set the global transaction ID here.  */
	memcpy(td->gid, gid, DB_GID_SIZE);
	if ((ret = __txn_doevents(env, txn, TXN_PREPARE, 1)) != 0)
		goto err;
	memset(&request, 0, sizeof(request));
	if (LOCKING_ON(env)) {
		request.op = DB_LOCK_PUT_READ;
		if (!IS_ZERO_LSN(td->last_lsn)) {
			memset(&list_dbt, 0, sizeof(list_dbt));
			request.obj = &list_dbt;
		}
		if ((ret = __lock_vec(env,
		    txn->locker, 0, &request, 1, NULL)) != 0)
			goto err;

	}
	if (DBENV_LOGGING(env)) {
		memset(&gid_dbt, 0, sizeof(gid));
		gid_dbt.data = gid;
		gid_dbt.size = DB_GID_SIZE;
		lflags = DB_LOG_COMMIT | DB_FLUSH;
		if ((ret = __txn_prepare_log(env,
		    txn, &td->last_lsn, lflags, TXN_PREPARE,
		    &gid_dbt, &td->begin_lsn, request.obj)) != 0)
			__db_err(env, ret, DB_STR("4528",
			    "DB_TXN->prepare: log_write failed"));

		if (request.obj != NULL && request.obj->data != NULL)
			__os_free(env, request.obj->data);
		if (ret != 0)
			goto err;

	}

	MUTEX_LOCK(env, txn->mgrp->mutex);
	td->status = TXN_PREPARED;
	MUTEX_UNLOCK(env, txn->mgrp->mutex);
err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_id --
 *	Return the transaction ID.
 *
 * PUBLIC: u_int32_t __txn_id __P((DB_TXN *));
 */
u_int32_t
__txn_id(txn)
	DB_TXN *txn;
{
	return (txn->txnid);
}

/*
 * __txn_get_name --
 *	Get a descriptive string from a transaction.
 *
 * PUBLIC: int __txn_get_name __P((DB_TXN *, const char **));
 */
int
__txn_get_name(txn, namep)
	DB_TXN *txn;
	const char **namep;
{
	*namep = txn->name;

	return (0);
}

/*
 * __txn_set_name --
 *	Set a descriptive string for a transaction.
 *
 * PUBLIC: int __txn_set_name __P((DB_TXN *, const char *));
 */
int
__txn_set_name(txn, name)
	DB_TXN *txn;
	const char *name;
{
	DB_THREAD_INFO *ip;
	DB_TXNMGR *mgr;
	ENV *env;
	TXN_DETAIL *td;
	size_t len;
	int ret;
	char *p;

	mgr = txn->mgrp;
	env = mgr->env;
	td = txn->td;
	len = strlen(name) + 1;

	if ((ret = __os_realloc(env, len, &txn->name)) != 0)
		return (ret);
	memcpy(txn->name, name, len);

	ENV_ENTER(env, ip);
	TXN_SYSTEM_LOCK(env);
	if (td->name != INVALID_ROFF) {
		__env_alloc_free(
		    &mgr->reginfo, R_ADDR(&mgr->reginfo, td->name));
		td->name = INVALID_ROFF;
	}
	if ((ret = __env_alloc(&mgr->reginfo, len, &p)) != 0) {
		TXN_SYSTEM_UNLOCK(env);
		__db_errx(env, DB_STR("4529",
		    "Unable to allocate memory for transaction name"));

		__os_free(env, txn->name);
		txn->name = NULL;

		ENV_LEAVE(env, ip);
		return (ret);
	}
	TXN_SYSTEM_UNLOCK(env);
	td->name = R_OFFSET(&mgr->reginfo, p);
	memcpy(p, name, len);

#ifdef DIAGNOSTIC
	/*
	 * If DIAGNOSTIC is set, map the name into the log so users can track
	 * operations through the log.
	 */
	if (DBENV_LOGGING(env))
		(void)__log_printf(env, txn, "transaction %#lx named %s",
		    (u_long)txn->txnid, name);
#endif

	ENV_LEAVE(env, ip);
	return (0);
}

/*
 * __txn_get_priority --
 *	Get a transaction's priority level
 * PUBLIC: int __txn_get_priority __P((DB_TXN *, u_int32_t *));
 */
int
__txn_get_priority(txn, priorityp)
	DB_TXN *txn;
	u_int32_t *priorityp;
{
	if (txn->locker == NULL)
		return EINVAL;

	*priorityp = txn->locker->priority;
	return (0);
}

/*
 * __txn_set_priority --
 *	Assign a transaction a priority level
 * PUBLIC: int __txn_set_priority __P((DB_TXN *, u_int32_t));
 */
int
__txn_set_priority(txn, priority)
	DB_TXN *txn;
	u_int32_t priority;
{
	if (txn->locker == NULL)
		return EINVAL;

	txn->locker->priority = priority;
	((TXN_DETAIL *)txn->td)->priority = priority;

	return (0);
}

/*
 * __txn_set_timeout --
 *	ENV->set_txn_timeout.
 * PUBLIC: int  __txn_set_timeout __P((DB_TXN *, db_timeout_t, u_int32_t));
 */
int
__txn_set_timeout(txn, timeout, op)
	DB_TXN *txn;
	db_timeout_t timeout;
	u_int32_t op;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = txn->mgrp->env;

	if (op != DB_SET_TXN_TIMEOUT && op != DB_SET_LOCK_TIMEOUT)
		return (__db_ferr(env, "DB_TXN->set_timeout", 0));

	ENV_ENTER(env, ip);
	ret = __lock_set_timeout( env, txn->locker, timeout, op);
	ENV_LEAVE(txn->mgrp->env, ip);
	return (ret);
}

/*
 * __txn_isvalid --
 *	Return 0 if the DB_TXN is reasonable, otherwise panic.
 */
static int
__txn_isvalid(txn, op)
	const DB_TXN *txn;
	txnop_t op;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	ENV *env;
	TXN_DETAIL *td;

	mgr = txn->mgrp;
	env = mgr->env;
	region = mgr->reginfo.primary;

	/* Check for recovery. */
	if (!F_ISSET(txn, TXN_COMPENSATE) &&
	    F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_errx(env, DB_STR("4530",
		    "operation not permitted during recovery"));
		goto err;
	}

	/* Check for live cursors. */
	if (txn->cursors != 0) {
		__db_errx(env, DB_STR("4531",
		    "transaction has active cursors"));
		goto err;
	}

	/* Check transaction's state. */
	td = txn->td;

	/* Handle any operation specific checks. */
	switch (op) {
	case TXN_OP_DISCARD:
		/*
		 * Since we're just tossing the per-process space; there are
		 * a lot of problems with the transaction that we can tolerate.
		 */

		/* Transaction is already been reused. */
		if (txn->txnid != td->txnid)
			return (0);

		/*
		 * What we've got had better be either a prepared or
		 * restored transaction.
		 */
		if (td->status != TXN_PREPARED &&
		    !F_ISSET(td, TXN_DTL_RESTORED)) {
			__db_errx(env, DB_STR("4532",
			    "not a restored transaction"));
			return (__env_panic(env, EINVAL));
		}

		return (0);
	case TXN_OP_PREPARE:
		if (txn->parent != NULL) {
			/*
			 * This is not fatal, because you could imagine an
			 * application that simply prepares everybody because
			 * it doesn't distinguish between children and parents.
			 * I'm not arguing this is good, but I could imagine
			 * someone doing it.
			 */
			__db_errx(env, DB_STR("4533",
			    "Prepare disallowed on child transactions"));
			return (EINVAL);
		}
		break;
	case TXN_OP_ABORT:
	case TXN_OP_COMMIT:
	default:
		break;
	}

	switch (td->status) {
	case TXN_PREPARED:
		if (op == TXN_OP_PREPARE) {
			__db_errx(env, DB_STR("4534",
			    "transaction already prepared"));
			/*
			 * Txn_prepare doesn't blow away the user handle, so
			 * in this case, give the user the opportunity to
			 * abort or commit.
			 */
			return (EINVAL);
		}
		break;
	case TXN_RUNNING:
	case TXN_NEED_ABORT:
		break;
	case TXN_ABORTED:
	case TXN_COMMITTED:
	default:
		__db_errx(env, DB_STR_A("4535",
		    "transaction already %s", "%s"),
		    td->status == TXN_COMMITTED ?
		    DB_STR_P("committed") : DB_STR_P("aborted"));
		goto err;
	}

	return (0);

err:	/*
	 * If there's a serious problem with the transaction, panic.  TXN
	 * handles are dead by definition when we return, and if you use
	 * a cursor you forgot to close, we have no idea what will happen.
	 */
	return (__env_panic(env, EINVAL));
}

/*
 * __txn_end --
 *	Internal transaction end routine.
 */
static int
__txn_end(txn, is_commit)
	DB_TXN *txn;
	int is_commit;
{
	DB_LOCKREQ request;
	DB_TXNLOGREC *lr;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	ENV *env;
	TXN_DETAIL *ptd, *td;
	db_mutex_t mvcc_mtx;
	int do_closefiles, ret;

	mgr = txn->mgrp;
	env = mgr->env;
	region = mgr->reginfo.primary;
	do_closefiles = 0;

	/* Process commit events. */
	if ((ret = __txn_doevents(env,
	    txn, is_commit ? TXN_COMMIT : TXN_ABORT, 0)) != 0)
		return (__env_panic(env, ret));

	/* End the transaction. */
	td = txn->td;
	if (td->nlog_dbs != 0 &&
	     (ret = __txn_dref_fname(env, txn)) != 0 && ret != EIO)
		return (__env_panic(env, ret));

	if (td->mvcc_ref != 0 && IS_MAX_LSN(td->visible_lsn)) {
		/*
		 * Some pages were dirtied but nothing was logged.  This can
		 * happen easily if we are aborting, but there are also cases
		 * in the compact code where pages are dirtied unconditionally
		 * and then we find out that there is no work to do.
		 *
		 * We need to make sure that the versions become visible to
		 * future transactions.  We need to set visible_lsn before
		 * setting td->status to ensure safe reads of visible_lsn in
		 * __memp_fget.
		 */
		if ((ret = __log_current_lsn_int(env, &td->visible_lsn,
		    NULL, NULL)) != 0)
			return (__env_panic(env, ret));
	}

	/*
	 * Release the locks.
	 *
	 * __txn_end cannot return an simple error, we MUST return
	 * success/failure from commit or abort, ignoring any internal
	 * errors.  So, we panic if something goes wrong.  We can't
	 * deadlock here because we're not acquiring any new locks,
	 * so DB_LOCK_DEADLOCK is just as fatal as any other error.
	 */
	if (LOCKING_ON(env)) {
		/* Allocate a locker for this restored txn if necessary. */
		if (txn->locker == NULL &&
		    (ret = __lock_getlocker(env->lk_handle,
		    txn->txnid, 1, &txn->locker)) != 0)
			return (__env_panic(env, ret));
		request.op = txn->parent == NULL ||
		    is_commit == 0 ? DB_LOCK_PUT_ALL : DB_LOCK_INHERIT;
		request.obj = NULL;
		if ((ret = __lock_vec(env,
		    txn->locker, 0, &request, 1, NULL)) != 0)
			return (__env_panic(env, ret));
	}

	TXN_SYSTEM_LOCK(env);
	td->status = is_commit ? TXN_COMMITTED : TXN_ABORTED;
	SH_TAILQ_REMOVE(&region->active_txn, td, links, __txn_detail);
	region->curtxns--;
	if (F_ISSET(td, TXN_DTL_RESTORED)) {
		region->stat.st_nrestores--;
		do_closefiles = region->stat.st_nrestores == 0;
	}

	if (td->name != INVALID_ROFF) {
		__env_alloc_free(&mgr->reginfo,
		    R_ADDR(&mgr->reginfo, td->name));
		td->name = INVALID_ROFF;
	}
	if (td->nlog_slots != TXN_NSLOTS)
		__env_alloc_free(&mgr->reginfo,
		    R_ADDR(&mgr->reginfo, td->log_dbs));

	if (txn->parent != NULL) {
		ptd = txn->parent->td;
		SH_TAILQ_REMOVE(&ptd->kids, td, klinks, __txn_detail);
	} else if ((mvcc_mtx = td->mvcc_mtx) != MUTEX_INVALID) {
		MUTEX_LOCK(env, mvcc_mtx);
		if (td->mvcc_ref != 0) {
			SH_TAILQ_INSERT_HEAD(&region->mvcc_txn,
			    td, links, __txn_detail);

			/*
			 * The transaction has been added to the list of
			 * committed snapshot transactions with active pages.
			 * It needs to be freed when the last page is evicted.
			 */
			F_SET(td, TXN_DTL_SNAPSHOT);
#ifdef HAVE_STATISTICS
			STAT_INC(env, txn,
			    nsnapshot, region->stat.st_nsnapshot, txn->txnid);
			if (region->stat.st_nsnapshot >
			    region->stat.st_maxnsnapshot)
				    STAT_SET(env, txn, maxnsnapshot,
					region->stat.st_maxnsnapshot,
					region->stat.st_nsnapshot,
					txn->txnid);
#endif
			td = NULL;
		}
		MUTEX_UNLOCK(env, mvcc_mtx);
		if (td != NULL)
			if ((ret = __mutex_free(env, &td->mvcc_mtx)) != 0)
				return (__env_panic(env, ret));
	}

	if (td != NULL)
		__env_alloc_free(&mgr->reginfo, td);

#ifdef HAVE_STATISTICS
	if (is_commit)
		STAT_INC(env,
		    txn, ncommits, region->stat.st_ncommits, txn->txnid);
	else
		STAT_INC(env,
		    txn, naborts, region->stat.st_naborts, txn->txnid);
	STAT_DEC(env, txn, nactive, region->stat.st_nactive, txn->txnid);
#endif

	/* Increment bulk transaction counter while holding transaction lock. */
	if (F_ISSET(txn, TXN_BULK))
		((DB_TXNREGION *)env->tx_handle->reginfo.primary)->n_bulk_txn--;

	TXN_SYSTEM_UNLOCK(env);

	/*
	 * The transaction cannot get more locks, remove its locker info,
	 * if any.
	 */
	if (LOCKING_ON(env) && (ret =
	    __lock_freelocker(env->lk_handle, txn->locker)) != 0)
		return (__env_panic(env, ret));
	if (txn->parent != NULL)
		TAILQ_REMOVE(&txn->parent->kids, txn, klinks);

	/* Free the space. */
	while ((lr = STAILQ_FIRST(&txn->logs)) != NULL) {
		STAILQ_REMOVE(&txn->logs, lr, __txn_logrec, links);
		__os_free(env, lr);
	}
	if (txn->name != NULL) {
		__os_free(env, txn->name);
		txn->name = NULL;
	}

	/*
	 * Free the transaction structure if we allocated it and if we are
	 * not in an XA transaction that will be freed when we exit the XA
	 * wrapper routines.
	 */
	if (F_ISSET(txn, TXN_MALLOC) &&
	    txn->xa_thr_status != TXN_XA_THREAD_ASSOCIATED) {
		MUTEX_LOCK(env, mgr->mutex);
		TAILQ_REMOVE(&mgr->txn_chain, txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);

		__os_free(env, txn);
	}

	if (do_closefiles) {
		/*
		 * Otherwise, we have resolved the last outstanding prepared
		 * txn and need to invalidate the fileids that were left
		 * open for those txns and then close them.
		 */
		(void)__dbreg_invalidate_files(env, 1);
		(void)__dbreg_close_files(env, 1);
		if (IS_REP_MASTER(env))
			F_CLR(env->rep_handle, DBREP_OPENFILES);
		F_CLR(env->lg_handle, DBLOG_OPENFILES);
		mgr->n_discards = 0;
		(void)__txn_checkpoint(env, 0, 0,
		    DB_CKP_INTERNAL | DB_FORCE);
	}

	return (0);
}

static int
__txn_dispatch_undo(env, txn, rdbt, key_lsn, txnlist)
	ENV *env;
	DB_TXN *txn;
	DBT *rdbt;
	DB_LSN *key_lsn;
	DB_TXNHEAD *txnlist;
{
	int ret;

	txnlist->td = txn->td;
	ret = __db_dispatch(env, &env->recover_dtab,
	    rdbt, key_lsn, DB_TXN_ABORT, txnlist);
	if (ret == DB_SURPRISE_KID) {
		F_SET(txn, TXN_CHILDCOMMIT);
		ret = 0;
	}
	if (ret == 0 && F_ISSET(txn, TXN_CHILDCOMMIT) && IS_ZERO_LSN(*key_lsn))
		ret = __db_txnlist_lsnget(env, txnlist, key_lsn, 0);

	return (ret);
}

/*
 * __txn_undo --
 *	Undo the transaction with id txnid.
 */
static int
__txn_undo(txn)
	DB_TXN *txn;
{
	DBT rdbt;
	DB_LOGC *logc;
	DB_LSN key_lsn;
	DB_TXN *ptxn;
	DB_TXNHEAD *txnlist;
	DB_TXNLOGREC *lr;
	DB_TXNMGR *mgr;
	ENV *env;
	int ret, t_ret;

	mgr = txn->mgrp;
	env = mgr->env;
	logc = NULL;
	txnlist = NULL;
	ret = 0;

	if (!LOGGING_ON(env))
		return (0);

	/*
	 * This is the simplest way to code this, but if the mallocs during
	 * recovery turn out to be a performance issue, we can do the
	 * allocation here and use DB_DBT_USERMEM.
	 */
	memset(&rdbt, 0, sizeof(rdbt));

	/*
	 * Allocate a txnlist for children and aborted page allocs.
	 * We need to associate the list with the maximal parent
	 * so that aborted pages are recovered when that transaction
	 * is committed or aborted.
	 */
	for (ptxn = txn->parent; ptxn != NULL && ptxn->parent != NULL;)
		ptxn = ptxn->parent;

	if (ptxn != NULL && ptxn->txn_list != NULL)
		txnlist = ptxn->txn_list;
	else if (txn->txn_list != NULL)
		txnlist = txn->txn_list;
	else if ((ret = __db_txnlist_init(env,
	    txn->thread_info, 0, 0, NULL, &txnlist)) != 0)
		return (ret);
	else if (ptxn != NULL)
		ptxn->txn_list = txnlist;

	/*
	 * Take log records from the linked list stored in the transaction,
	 * then from the log.
	 */
	STAILQ_FOREACH(lr, &txn->logs, links) {
		rdbt.data = lr->data;
		rdbt.size = 0;
		LSN_NOT_LOGGED(key_lsn);
		ret =
		    __txn_dispatch_undo(env, txn, &rdbt, &key_lsn, txnlist);
		if (ret != 0) {
			__db_err(env, ret, DB_STR("4536",
			    "DB_TXN->abort: in-memory log undo failed"));
			goto err;
		}
	}

	key_lsn = ((TXN_DETAIL *)txn->td)->last_lsn;

	if (!IS_ZERO_LSN(key_lsn) &&
	     (ret = __log_cursor(env, &logc)) != 0)
		goto err;

	while (!IS_ZERO_LSN(key_lsn)) {
		/*
		 * The dispatch routine returns the lsn of the record
		 * before the current one in the key_lsn argument.
		 */
		if ((ret = __logc_get(logc, &key_lsn, &rdbt, DB_SET)) == 0) {
			ret = __txn_dispatch_undo(env,
			    txn, &rdbt, &key_lsn, txnlist);
		}

		if (ret != 0) {
			__db_err(env, ret, DB_STR_A("4537",
		    "DB_TXN->abort: log undo failed for LSN: %lu %lu",
			    "%lu %lu"), (u_long)key_lsn.file,
			    (u_long)key_lsn.offset);
			goto err;
		}
	}

err:	if (logc != NULL && (t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;

	if (ptxn == NULL && txnlist != NULL)
		__db_txnlist_end(env, txnlist);
	return (ret);
}

/*
 * __txn_activekids --
 *	Return if this transaction has any active children.
 *
 * PUBLIC: int __txn_activekids __P((ENV *, u_int32_t, DB_TXN *));
 */
int
__txn_activekids(env, rectype, txn)
	ENV *env;
	u_int32_t rectype;
	DB_TXN *txn;
{
	/*
	 * On a child commit, we know that there are children (i.e., the
	 * committing child at the least.  In that case, skip this check.
	 */
	if (F_ISSET(txn, TXN_COMPENSATE) || rectype == DB___txn_child)
		return (0);

	if (TAILQ_FIRST(&txn->kids) != NULL) {
		__db_errx(env, DB_STR("4538",
		    "Child transaction is active"));
		return (EPERM);
	}
	return (0);
}

/*
 * __txn_force_abort --
 *	Force an abort record into the log if the commit record
 *	failed to get to disk.
 *
 * PUBLIC: int __txn_force_abort __P((ENV *, u_int8_t *));
 */
int
__txn_force_abort(env, buffer)
	ENV *env;
	u_int8_t *buffer;
{
	DB_CIPHER *db_cipher;
	HDR hdr, *hdrp;
	u_int32_t offset, opcode, sum_len;
	u_int8_t *bp, *key;
	size_t hdrsize, rec_len;
	int ret;

	db_cipher = env->crypto_handle;

	/*
	 * This routine depends on the layout of HDR and the __txn_regop
	 * record in txn.src.  We are passed the beginning of the commit
	 * record in the log buffer and overwrite the commit with an abort
	 * and recalculate the checksum.
	 */
	hdrsize = CRYPTO_ON(env) ? HDR_CRYPTO_SZ : HDR_NORMAL_SZ;

	hdrp = (HDR *)buffer;
	memcpy(&hdr.prev, buffer + SSZ(HDR, prev), sizeof(hdr.prev));
	memcpy(&hdr.len, buffer + SSZ(HDR, len), sizeof(hdr.len));
	if (LOG_SWAPPED(env))
		__log_hdrswap(&hdr, CRYPTO_ON(env));
	rec_len = hdr.len - hdrsize;

	offset = sizeof(u_int32_t) + sizeof(u_int32_t) + sizeof(DB_LSN);
	if (CRYPTO_ON(env)) {
		key = db_cipher->mac_key;
		sum_len = DB_MAC_KEY;
		if ((ret = db_cipher->decrypt(env, db_cipher->data,
		    &hdrp->iv[0], buffer + hdrsize, rec_len)) != 0)
			return (__env_panic(env, ret));
	} else {
		key = NULL;
		sum_len = sizeof(u_int32_t);
	}
	bp = buffer + hdrsize + offset;
	opcode = TXN_ABORT;
	LOGCOPY_32(env, bp, &opcode);

	if (CRYPTO_ON(env) &&
	    (ret = db_cipher->encrypt(env,
	    db_cipher->data, &hdrp->iv[0], buffer + hdrsize, rec_len)) != 0)
		return (__env_panic(env, ret));

#ifdef HAVE_LOG_CHECKSUM
	__db_chksum(&hdr, buffer + hdrsize, rec_len, key, NULL);
	if (LOG_SWAPPED(env))
		__log_hdrswap(&hdr, CRYPTO_ON(env));
	memcpy(buffer + SSZA(HDR, chksum), hdr.chksum, sum_len);
#endif

	return (0);
}

/*
 * __txn_preclose --
 *	Before we can close an environment, we need to check if we were in the
 *	middle of taking care of restored transactions.  If so, close the files
 *	we opened.
 *
 * PUBLIC: int __txn_preclose __P((ENV *));
 */
int
__txn_preclose(env)
	ENV *env;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int do_closefiles, ret;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;
	do_closefiles = 0;

	TXN_SYSTEM_LOCK(env);
	if (region != NULL &&
	    region->stat.st_nrestores <= mgr->n_discards &&
	    mgr->n_discards != 0)
		do_closefiles = 1;
	TXN_SYSTEM_UNLOCK(env);

	if (do_closefiles) {
		/*
		 * Set the DBLOG_RECOVER flag while closing these files so they
		 * do not create additional log records that will confuse future
		 * recoveries.
		 */
		F_SET(env->lg_handle, DBLOG_RECOVER);
		ret = __dbreg_close_files(env, 0);
		F_CLR(env->lg_handle, DBLOG_RECOVER);
	} else
		ret = 0;

	return (ret);
}

/*
 * __txn_reset --
 *	Reset the last txnid to its minimum value, and log the reset.
 *
 * PUBLIC: int __txn_reset __P((ENV *));
 */
int
__txn_reset(env)
	ENV *env;
{
	DB_LSN scrap;
	DB_TXNREGION *region;

	region = env->tx_handle->reginfo.primary;
	region->last_txnid = TXN_MINIMUM;

	DB_ASSERT(env, LOGGING_ON(env));
	return (__txn_recycle_log(env,
	    NULL, &scrap, 0, TXN_MINIMUM, TXN_MAXIMUM));
}

/*
 * txn_set_txn_lsnp --
 *	Set the pointer to the begin_lsn field if that field is zero.
 *	Set the pointer to the last_lsn field.
 */
static void
__txn_set_txn_lsnp(txn, blsnp, llsnp)
	DB_TXN *txn;
	DB_LSN **blsnp, **llsnp;
{
	TXN_DETAIL *td;

	td = txn->td;
	*llsnp = &td->last_lsn;

	while (txn->parent != NULL)
		txn = txn->parent;

	td = txn->td;
	if (IS_ZERO_LSN(td->begin_lsn))
		*blsnp = &td->begin_lsn;
}

/*
 * PUBLIC: int __txn_applied_pp __P((DB_ENV *,
 * PUBLIC:     DB_TXN_TOKEN *, db_timeout_t, u_int32_t));
 */
int
__txn_applied_pp(dbenv, token, timeout, flags)
	DB_ENV *dbenv;
	DB_TXN_TOKEN *token;
	db_timeout_t timeout;
	u_int32_t flags;
{
	ENV *env;
	DB_THREAD_INFO *ip;
	DB_COMMIT_INFO commit_info;
	u_int8_t *bp;
	int ret;

	env = dbenv->env;

	if (flags != 0)
		return (__db_ferr(env, "DB_ENV->txn_applied", 0));

	/* Unmarshal the token from its stored form. */
	bp = token->buf;
	DB_NTOHL_COPYIN(env, commit_info.version, bp);
	DB_ASSERT(env, commit_info.version == REP_COMMIT_TOKEN_FMT_VERSION);
	DB_NTOHL_COPYIN(env, commit_info.gen, bp);
	DB_NTOHL_COPYIN(env, commit_info.envid, bp);
	DB_NTOHL_COPYIN(env, commit_info.lsn.file, bp);
	DB_NTOHL_COPYIN(env, commit_info.lsn.offset, bp);

	/*
	 * Check for a token representing a transaction that committed without
	 * any log records having been written.  Ideally an application should
	 * be smart enough to avoid trying to use a token from such an "empty"
	 * transaction.  But in some cases it might be difficult for them to
	 * keep track, so we don't really forbid it.
	 */
	if (IS_ZERO_LSN(commit_info.lsn))
		return (DB_KEYEMPTY);

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->txn_applied", DB_INIT_LOG);

	ENV_ENTER(env, ip);
	ret = __txn_applied(env, ip, &commit_info, timeout);
	ENV_LEAVE(env, ip);
	return (ret);
}

static int
__txn_applied(env, ip, commit_info, timeout)
	ENV *env;
	DB_THREAD_INFO *ip;
	DB_COMMIT_INFO *commit_info;
	db_timeout_t timeout;
{
	LOG *lp;
	DB_LSN lsn;
	REGENV *renv;

	/*
	 * The lockout protection scope between __op_handle_enter and
	 * __env_db_rep_exit is handled within __rep_txn_applied, and is not
	 * needed here since the rest of this function only runs in a
	 * non-replication env.
	 */
	if (REP_ON(env))
		return (__rep_txn_applied(env, ip, commit_info, timeout));

	if (commit_info->gen != 0) {
		__db_errx(env, DB_STR("4539",
		    "replication commit token in non-replication env"));
		return (EINVAL);
	}

	lp = env->lg_handle->reginfo.primary;
	LOG_SYSTEM_LOCK(env);
	lsn = lp->lsn;
	LOG_SYSTEM_UNLOCK(env);

	renv = env->reginfo->primary;

	if (renv->envid == commit_info->envid &&
	    LOG_COMPARE(&commit_info->lsn, &lsn) <= 0)
		return (0);
	return (DB_NOTFOUND);
}
