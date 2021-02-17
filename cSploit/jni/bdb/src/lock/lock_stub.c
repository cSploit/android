/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"

/*
 * If the library wasn't compiled with locking support, various routines
 * aren't available.  Stub them here, returning an appropriate error.
 */
static int __db_nolocking __P((ENV *));

/*
 * __db_nolocking --
 *	Error when a Berkeley DB build doesn't include the locking subsystem.
 */
static int
__db_nolocking(env)
	ENV *env;
{
	__db_errx(env, DB_STR("2054",
	    "library build did not include support for locking"));
	return (DB_OPNOTSUP);
}

int
__lock_env_create(dbenv)
	DB_ENV *dbenv;
{
	COMPQUIET(dbenv, 0);
	return (0);
}

void
__lock_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	COMPQUIET(dbenv, 0);
}

int
__lock_get_lk_conflicts(dbenv, lk_conflictsp, lk_modesp)
	DB_ENV *dbenv;
	const u_int8_t **lk_conflictsp;
	int *lk_modesp;
{
	COMPQUIET(lk_conflictsp, NULL);
	COMPQUIET(lk_modesp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_detect(dbenv, lk_detectp)
	DB_ENV *dbenv;
	u_int32_t *lk_detectp;
{
	COMPQUIET(lk_detectp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_init_lockers(dbenv, lk_initp)
	DB_ENV *dbenv;
	u_int32_t *lk_initp;
{
	COMPQUIET(lk_initp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_init_locks(dbenv, lk_initp)
	DB_ENV *dbenv;
	u_int32_t *lk_initp;
{
	COMPQUIET(lk_initp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_init_objects(dbenv, lk_initp)
	DB_ENV *dbenv;
	u_int32_t *lk_initp;
{
	COMPQUIET(lk_initp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_max_lockers(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	COMPQUIET(lk_maxp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_max_locks(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	COMPQUIET(lk_maxp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_max_objects(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	COMPQUIET(lk_maxp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_partitions(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	COMPQUIET(lk_maxp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_tablesize(dbenv, lk_tablesizep)
	DB_ENV *dbenv;
	u_int32_t *lk_tablesizep;
{
	COMPQUIET(lk_tablesizep, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_tablesize(dbenv, lk_tablesize)
	DB_ENV *dbenv;
	u_int32_t lk_tablesize;
{
	COMPQUIET(lk_tablesize, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_lk_priority(dbenv, lockid, priorityp)
	DB_ENV *dbenv;
	u_int32_t lockid, *priorityp;
{
	COMPQUIET(lockid, 0);
	COMPQUIET(priorityp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_priority(dbenv, lockid, priority)
	DB_ENV *dbenv;
	u_int32_t lockid, priority;
{
	COMPQUIET(lockid, 0);
	COMPQUIET(priority, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_env_timeout(dbenv, timeoutp, flag)
	DB_ENV *dbenv;
	db_timeout_t *timeoutp;
	u_int32_t flag;
{
	COMPQUIET(timeoutp, NULL);
	COMPQUIET(flag, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_detect_pp(dbenv, flags, atype, abortp)
	DB_ENV *dbenv;
	u_int32_t flags, atype;
	int *abortp;
{
	COMPQUIET(flags, 0);
	COMPQUIET(atype, 0);
	COMPQUIET(abortp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_get_pp(dbenv, locker, flags, obj, lock_mode, lock)
	DB_ENV *dbenv;
	u_int32_t locker, flags;
	DBT *obj;
	db_lockmode_t lock_mode;
	DB_LOCK *lock;
{
	COMPQUIET(locker, 0);
	COMPQUIET(flags, 0);
	COMPQUIET(obj, NULL);
	COMPQUIET(lock_mode, 0);
	COMPQUIET(lock, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_id_pp(dbenv, idp)
	DB_ENV *dbenv;
	u_int32_t *idp;
{
	COMPQUIET(idp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_id_free_pp(dbenv, id)
	DB_ENV *dbenv;
	u_int32_t id;
{
	COMPQUIET(id, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_put_pp(dbenv, lock)
	DB_ENV *dbenv;
	DB_LOCK *lock;
{
	COMPQUIET(lock, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOCK_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_vec_pp(dbenv, locker, flags, list, nlist, elistp)
	DB_ENV *dbenv;
	u_int32_t locker, flags;
	int nlist;
	DB_LOCKREQ *list, **elistp;
{
	COMPQUIET(locker, 0);
	COMPQUIET(flags, 0);
	COMPQUIET(list, NULL);
	COMPQUIET(nlist, 0);
	COMPQUIET(elistp, NULL);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_conflicts(dbenv, lk_conflicts, lk_modes)
	DB_ENV *dbenv;
	u_int8_t *lk_conflicts;
	int lk_modes;
{
	COMPQUIET(lk_conflicts, NULL);
	COMPQUIET(lk_modes, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_detect(dbenv, lk_detect)
	DB_ENV *dbenv;
	u_int32_t lk_detect;
{
	COMPQUIET(lk_detect, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_max_locks(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	COMPQUIET(lk_max, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_max_lockers(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	COMPQUIET(lk_max, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_max_objects(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	COMPQUIET(lk_max, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_lk_partitions(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	COMPQUIET(lk_max, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_set_env_timeout(dbenv, timeout, flags)
	DB_ENV *dbenv;
	db_timeout_t timeout;
	u_int32_t flags;
{
	COMPQUIET(timeout, 0);
	COMPQUIET(flags, 0);
	return (__db_nolocking(dbenv->env));
}

int
__lock_open(env)
	ENV *env;
{
	return (__db_nolocking(env));
}

u_int32_t
__lock_region_mutex_count(env)
	ENV *env;
{
	return (__db_nolocking(env));
}

u_int32_t
__lock_region_mutex_max(env)
	ENV *env;
{
	return (__db_nolocking(env));
}

size_t
__lock_region_max(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

size_t
__lock_region_size(env, other_alloc)
	ENV *env;
	size_t other_alloc;
{
	COMPQUIET(env, NULL);
	COMPQUIET(other_alloc, 0);
	return (0);
}

int
__lock_id_free(env, sh_locker)
	ENV *env;
	DB_LOCKER *sh_locker;
{
	COMPQUIET(env, NULL);
	COMPQUIET(sh_locker, 0);
	return (0);
}

int
__lock_env_refresh(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

int
__lock_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	COMPQUIET(env, NULL);
	COMPQUIET(flags, 0);
	return (0);
}

int
__lock_put(env, lock)
	ENV *env;
	DB_LOCK *lock;
{
	COMPQUIET(env, NULL);
	COMPQUIET(lock, NULL);
	return (0);
}

int
__lock_vec(env, sh_locker, flags, list, nlist, elistp)
	ENV *env;
	DB_LOCKER *sh_locker;
	u_int32_t flags;
	int nlist;
	DB_LOCKREQ *list, **elistp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(sh_locker, 0);
	COMPQUIET(flags, 0);
	COMPQUIET(list, NULL);
	COMPQUIET(nlist, 0);
	COMPQUIET(elistp, NULL);
	return (0);
}

int
__lock_get(env, locker, flags, obj, lock_mode, lock)
	ENV *env;
	DB_LOCKER *locker;
	u_int32_t flags;
	const DBT *obj;
	db_lockmode_t lock_mode;
	DB_LOCK *lock;
{
	COMPQUIET(env, NULL);
	COMPQUIET(locker, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(obj, NULL);
	COMPQUIET(lock_mode, 0);
	COMPQUIET(lock, NULL);
	return (0);
}

int
__lock_id(env, idp, lkp)
	ENV *env;
	u_int32_t *idp;
	DB_LOCKER **lkp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(idp, NULL);
	COMPQUIET(lkp, NULL);
	return (0);
}

int
__lock_inherit_timeout(env, parent, locker)
	ENV *env;
	DB_LOCKER *parent, *locker;
{
	COMPQUIET(env, NULL);
	COMPQUIET(parent, NULL);
	COMPQUIET(locker, NULL);
	return (0);
}

int
__lock_set_timeout(env, locker, timeout, op)
	ENV *env;
	DB_LOCKER *locker;
	db_timeout_t timeout;
	u_int32_t op;
{
	COMPQUIET(env, NULL);
	COMPQUIET(locker, NULL);
	COMPQUIET(timeout, 0);
	COMPQUIET(op, 0);
	return (0);
}

int
__lock_addfamilylocker(env, pid, id, is_family)
	ENV *env;
	u_int32_t pid, id, is_family;
{
	COMPQUIET(env, NULL);
	COMPQUIET(pid, 0);
	COMPQUIET(id, 0);
	COMPQUIET(is_family, 0);
	return (0);
}

int
__lock_freelocker(lt, sh_locker)
	DB_LOCKTAB *lt;
	DB_LOCKER *sh_locker;
{
	COMPQUIET(lt, NULL);
	COMPQUIET(sh_locker, NULL);
	return (0);
}

int
__lock_familyremove(lt, sh_locker)
	DB_LOCKTAB *lt;
	DB_LOCKER *sh_locker;
{
	COMPQUIET(lt, NULL);
	COMPQUIET(sh_locker, NULL);
	return (0);
}

int
__lock_downgrade(env, lock, new_mode, flags)
	ENV *env;
	DB_LOCK *lock;
	db_lockmode_t new_mode;
	u_int32_t flags;
{
	COMPQUIET(env, NULL);
	COMPQUIET(lock, NULL);
	COMPQUIET(new_mode, 0);
	COMPQUIET(flags, 0);
	return (0);
}

int
__lock_locker_same_family(env, locker1, locker2, retp)
	ENV *env;
	DB_LOCKER *locker1;
	DB_LOCKER *locker2;
	int *retp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(locker1, NULL);
	COMPQUIET(locker2, NULL);

	*retp = 1;
	return (0);
}

void
__lock_set_thread_id(lref, pid, tid)
	void *lref;
	pid_t pid;
	db_threadid_t tid;
{
	COMPQUIET(lref, NULL);
	COMPQUIET(pid, 0);
	COMPQUIET(tid, 0);
}

int
__lock_failchk(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

int
__lock_get_list(env, locker, flags, lock_mode, list)
	ENV *env;
	DB_LOCKER *locker;
	u_int32_t flags;
	db_lockmode_t lock_mode;
	DBT *list;
{
	COMPQUIET(env, NULL);
	COMPQUIET(locker, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(lock_mode, 0);
	COMPQUIET(list, NULL);
	return (0);
}

void
__lock_list_print(env, mbp, list)
	ENV *env;
	DB_MSGBUF *mbp;
	DBT *list;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mbp, NULL);
	COMPQUIET(list, NULL);
}

int
__lock_getlocker(lt, locker, create, retp)
	DB_LOCKTAB *lt;
	u_int32_t locker;
	int create;
	DB_LOCKER **retp;
{
	COMPQUIET(locker, 0);
	COMPQUIET(create, 0);
	COMPQUIET(retp, NULL);
	return (__db_nolocking(lt->env));
}

int
__lock_id_set(env, cur_id, max_id)
	ENV *env;
	u_int32_t cur_id, max_id;
{
	COMPQUIET(env, NULL);
	COMPQUIET(cur_id, 0);
	COMPQUIET(max_id, 0);
	return (0);
}

int
__lock_wakeup(env, obj)
	ENV *env;
	const DBT *obj;
{
	COMPQUIET(obj, NULL);
	return (__db_nolocking(env));
}

int
__lock_change(env, old_lock, new_lock)
	ENV *env;
	DB_LOCK *old_lock, *new_lock;
{
	COMPQUIET(old_lock, NULL);
	COMPQUIET(new_lock, NULL);
	return (__db_nolocking(env));
}
