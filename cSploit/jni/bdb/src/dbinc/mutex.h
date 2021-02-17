/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_MUTEX_H_
#define	_DB_MUTEX_H_

#ifdef HAVE_MUTEX_SUPPORT
/* The inlined trylock calls need access to the details of mutexes. */
#define	LOAD_ACTUAL_MUTEX_CODE
#include "dbinc/mutex_int.h"

#ifndef HAVE_SHARED_LATCHES
 #error "Shared latches are required in DB 4.8 and above"
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * These specify the default spin parameters for test-and-set mutexes. A single
 * processor system spins just once, a multiprocessor system spins 50 times per
 * processor up to a default maximum of 200. This limit reduces excessive
 * busy-waiting on machines with many hyperthreads. We have anecdotal evidence
 * that these are reasonable default values.
 */
#define	MUTEX_SPINS_PER_PROCESSOR	50
#define	MUTEX_SPINS_DEFAULT_MAX		200

/*
 * Mutexes are represented by unsigned, 32-bit integral values.  As the
 * OOB value is 0, mutexes can be initialized by zero-ing out the memory
 * in which they reside.
 */
#define	MUTEX_INVALID	0

/*
 * We track mutex allocations by ID.
 */
#define	MTX_APPLICATION		 1
#define	MTX_ATOMIC_EMULATION	 2
#define	MTX_DB_HANDLE		 3
#define	MTX_ENV_DBLIST		 4
#define	MTX_ENV_EXCLDBLIST	 5
#define	MTX_ENV_HANDLE		 6
#define	MTX_ENV_REGION		 7
#define	MTX_LOCK_REGION		 8
#define	MTX_LOGICAL_LOCK	 9
#define	MTX_LOG_FILENAME	10
#define	MTX_LOG_FLUSH		11
#define	MTX_LOG_HANDLE		12
#define	MTX_LOG_REGION		13
#define	MTX_MPOOLFILE_HANDLE	14
#define	MTX_MPOOL_BH		15
#define	MTX_MPOOL_FH		16
#define	MTX_MPOOL_FILE_BUCKET	17
#define	MTX_MPOOL_HANDLE	18
#define	MTX_MPOOL_HASH_BUCKET	19
#define	MTX_MPOOL_REGION	20
#define	MTX_MUTEX_REGION	21
#define	MTX_MUTEX_TEST		22
#define	MTX_REP_CHKPT		23
#define	MTX_REP_DATABASE	24
#define	MTX_REP_DIAG		25
#define	MTX_REP_EVENT		26
#define	MTX_REP_REGION		27
#define	MTX_REP_START		28
#define	MTX_REP_WAITER		29
#define	MTX_REPMGR		30
#define	MTX_SEQUENCE		31
#define	MTX_TWISTER		32
#define	MTX_TCL_EVENTS		33
#define	MTX_TXN_ACTIVE		34
#define	MTX_TXN_CHKPT		35
#define	MTX_TXN_COMMIT		36
#define	MTX_TXN_MVCC		37
#define	MTX_TXN_REGION		38

#define	MTX_MAX_ENTRY		38

/* The following macros are defined on some platforms, e.g. QNX. */
#undef __mutex_init
#undef __mutex_lock
#undef __mutex_timedlock
#undef __mutex_unlock
#undef __mutex_destroy
#undef __mutex_trylock

/* Redirect mutex calls to the correct functions. */
#if !defined(HAVE_MUTEX_HYBRID) && (					\
    defined(HAVE_MUTEX_PTHREADS) ||					\
    defined(HAVE_MUTEX_SOLARIS_LWP) ||					\
    defined(HAVE_MUTEX_UI_THREADS))
#define	__mutex_init(a, b, c)		__db_pthread_mutex_init(a, b, c)
#define	__mutex_lock(a, b)		__db_pthread_mutex_lock(a, b, 0)
#define	__mutex_timedlock(a, b, c)	__db_pthread_mutex_lock(a, b, c)
#define	__mutex_unlock(a, b)		__db_pthread_mutex_unlock(a, b)
#define	__mutex_destroy(a, b)		__db_pthread_mutex_destroy(a, b)
#define	__mutex_trylock(a, b)		__db_pthread_mutex_trylock(a, b)
/*
 * These trylock versions do not support DB_ENV_FAILCHK. Callers which loop
 * checking mutexes which are held by dead processes or threads might spin.
 * These have ANSI-style definitions because this file can be included by
 * C++ files, and extern "C" affects linkage only, not argument typing.
 */
static inline int __db_pthread_mutex_trylock(ENV *env, db_mutex_t mutex)
{
	int ret;
	DB_MUTEX *mutexp;
	if (!MUTEX_ON(env) || F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);
	mutexp = MUTEXP_SET(env, mutex);
#ifdef HAVE_SHARED_LATCHES
	if (F_ISSET(mutexp, DB_MUTEX_SHARED))
		ret = pthread_rwlock_trywrlock(&mutexp->u.rwlock);
	else
#endif
		ret = pthread_mutex_trylock(&mutexp->u.m.mutex);
	if (ret == EBUSY)
		ret = DB_LOCK_NOTGRANTED;
	else if (ret == 0) {
		F_SET(mutexp, DB_MUTEX_LOCKED);
		env->dbenv->thread_id(env->dbenv, &mutexp->pid, &mutexp->tid);
		STAT_INC(env,
		    mutex, set_nowait, mutexp->mutex_set_nowait, mutex);
	}
	return (ret);
}
#ifdef HAVE_SHARED_LATCHES
#define	__mutex_rdlock(a, b)		__db_pthread_mutex_readlock(a, b)
#define	__mutex_tryrdlock(a, b)		__db_pthread_mutex_tryreadlock(a, b)
static inline int __db_pthread_mutex_tryreadlock(ENV *env, db_mutex_t mutex)
{
	int ret;
	DB_MUTEX *mutexp;
	if (!MUTEX_ON(env) || F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);
	mutexp = MUTEXP_SET(env, mutex);
	if (F_ISSET(mutexp, DB_MUTEX_SHARED))
		ret = pthread_rwlock_tryrdlock(&mutexp->u.rwlock);
	else
		return (EINVAL);
	if (ret == EBUSY)
		ret = DB_LOCK_NOTGRANTED;
#ifdef HAVE_STATISTICS
	if (ret == 0)
		STAT_INC(env,
		    mutex, set_rd_nowait, mutexp->mutex_set_nowait, mutex);
#endif
	return (ret);
}
#endif
#elif defined(HAVE_MUTEX_WIN32) || defined(HAVE_MUTEX_WIN32_GCC)
#define	__mutex_init(a, b, c)		__db_win32_mutex_init(a, b, c)
#define	__mutex_lock(a, b)		__db_win32_mutex_lock(a, b, 0)
#define	__mutex_timedlock(a, b, c)	__db_win32_mutex_lock(a, b, c)
#define	__mutex_trylock(a, b)		__db_win32_mutex_trylock(a, b)
#define	__mutex_unlock(a, b)		__db_win32_mutex_unlock(a, b)
#define	__mutex_destroy(a, b)		__db_win32_mutex_destroy(a, b)
#ifdef HAVE_SHARED_LATCHES
#define	__mutex_rdlock(a, b)		__db_win32_mutex_readlock(a, b)
#define	__mutex_tryrdlock(a, b)		__db_win32_mutex_tryreadlock(a, b)
#endif
#elif defined(HAVE_MUTEX_FCNTL)
#define	__mutex_init(a, b, c)		__db_fcntl_mutex_init(a, b, c)
#define	__mutex_lock(a, b)		__db_fcntl_mutex_lock(a, b, 0)
#define	__mutex_timedlock(a, b, c)	__db_fcntl_lock(a, b, c)
#define	__mutex_trylock(a, b)		__db_fcntl_mutex_trylock(a, b)
#define	__mutex_unlock(a, b)		__db_fcntl_mutex_unlock(a, b)
#define	__mutex_destroy(a, b)		__db_fcntl_mutex_destroy(a, b)
#else
#define	__mutex_init(a, b, c)		__db_tas_mutex_init(a, b, c)
#define	__mutex_lock(a, b)		__db_tas_mutex_lock(a, b, 0)
#define	__mutex_timedlock(a, b, c)	__db_tas_mutex_lock(a, b, c)
#define	__mutex_trylock(a, b)		__db_tas_mutex_trylock(a, b)
#define	__mutex_unlock(a, b)		__db_tas_mutex_unlock(a, b)
#define	__mutex_destroy(a, b)		__db_tas_mutex_destroy(a, b)
#if defined(HAVE_SHARED_LATCHES)
#define	__mutex_rdlock(a, b)		__db_tas_mutex_readlock(a, b)
#define	__mutex_tryrdlock(a,b)		__db_tas_mutex_tryreadlock(a, b)
#endif
#endif

/*
 * When there is no method to get a shared latch, fall back to
 * implementing __mutex_rdlock() as getting an exclusive one.
 * This occurs either when !HAVE_SHARED_LATCHES or HAVE_MUTEX_FCNTL.
 */
#ifndef __mutex_rdlock
#define	__mutex_rdlock(a, b)		__mutex_lock(a, b)
#endif
#ifndef __mutex_tryrdlock
#define	__mutex_tryrdlock(a, b)		__mutex_trylock(a, b)
#endif

/*
 * Lock/unlock a mutex.  If the mutex was never required, the thread of
 * control can proceed without it.
 *
 * We never fail to acquire or release a mutex without panicing.  Simplify
 * the macros to always return a panic value rather than saving the actual
 * return value of the mutex routine.
 */
#ifdef HAVE_MUTEX_SUPPORT
#define	MUTEX_LOCK(env, mutex) do {					\
	if ((mutex) != MUTEX_INVALID &&					\
	    __mutex_lock(env, mutex) != 0)				\
		return (DB_RUNRECOVERY);				\
} while (0)

/*
 * Always check the return value of MUTEX_TRYLOCK()!  Expect 0 on success,
 * or DB_LOCK_NOTGRANTED, or possibly DB_RUNRECOVERY for failchk.
 */
#define	MUTEX_TRYLOCK(env, mutex)					\
	(((mutex) == MUTEX_INVALID) ? 0 : __mutex_trylock(env, mutex))

/*
 * Acquire a DB_MUTEX_SHARED "mutex" in shared mode.
 */
#define	MUTEX_READLOCK(env, mutex) do {					\
	if ((mutex) != MUTEX_INVALID &&					\
	    __mutex_rdlock(env, mutex) != 0)				\
		return (DB_RUNRECOVERY);				\
} while (0)
#define	MUTEX_TRY_READLOCK(env, mutex)					\
	((mutex) != MUTEX_INVALID ? __mutex_tryrdlock(env, mutex) : 0)

#define	MUTEX_UNLOCK(env, mutex) do {					\
	if ((mutex) != MUTEX_INVALID &&					\
	    __mutex_unlock(env, mutex) != 0)				\
		return (DB_RUNRECOVERY);				\
} while (0)

#define	MUTEX_WAIT(env, mutex, duration) do {			      \
	int __ret;						      \
	if ((mutex) != MUTEX_INVALID &&				      \
	    (__ret = __mutex_timedlock(env, mutex, duration)) != 0 && \
	    __ret != DB_TIMEOUT)				      \
		return (DB_RUNRECOVERY);			      \
} while (0)

/*
 * Check that a particular mutex is exclusively held at least by someone, not
 * necessarily the current thread.
 */
#define	MUTEX_IS_OWNED(env, mutex)					\
	(mutex == MUTEX_INVALID || !MUTEX_ON(env) ||			\
	F_ISSET(env->dbenv, DB_ENV_NOLOCKING) ||			\
	F_ISSET(MUTEXP_SET(env, mutex), DB_MUTEX_LOCKED))
#else
/*
 * There are calls to lock/unlock mutexes outside of #ifdef's -- replace
 * the call with something the compiler can discard, but which will make
 * if-then-else blocks work correctly, and suppress unused variable messages.
 */
#define	MUTEX_LOCK(env, mutex)		{ env = (env); mutex = (mutex); }
#define	MUTEX_TRYLOCK(env, mutex)	( env = (env), mutex = (mutex), 0)
#define	MUTEX_READLOCK(env, mutex)	{ env = (env); mutex = (mutex); }
#define	MUTEX_TRY_READLOCK(env, mutex)	( env = (env), mutex = (mutex), 0 )
#define	MUTEX_UNLOCK(env, mutex)	{ env = (env); mutex = (mutex); }
#define	MUTEX_REQUIRED(env, mutex)	{ env = (env); mutex = (mutex); }
#define	MUTEX_REQUIRED_READ(env, mutex)	{ env = (env); mutex = (mutex); }
#define	MUTEX_WAIT(env, mutex, duration)	{			\
	(env) = (env); (mutex) = (mutex); (duration) = (duration);	\
}

/*
 * Every MUTEX_IS_OWNED() caller expects to own it. When there is no mutex
 * support, act as if we have ownership.
 */
#define	MUTEX_IS_OWNED(env, mutex)	1
#endif

/*
 * Bulk initialization of mutexes in regions.
 */

#define MUTEX_BULK_INIT(env, region, start, howmany) do {		\
	DB_MUTEX *__mutexp;						\
	db_mutex_t __i = start;						\
	u_int32_t __n = howmany;					\
	for (__mutexp = MUTEXP_SET(env, __i);				\
	    --__n > 0;							\
	    __mutexp = MUTEXP_SET(env, __i)) {				\
		__mutexp->flags = 0;					\
		__i = (F_ISSET(env, ENV_PRIVATE)) ?			\
		    ((uintptr_t)__mutexp + region->mutex_size) : __i + 1; \
		__mutexp->mutex_next_link = __i;			\
	}								\
	__mutexp->flags = 0;						\
	__mutexp->mutex_next_link = MUTEX_INVALID;			\
} while (0)

/*
 * Berkeley DB ports may require single-threading at places in the code.
 */
#ifdef HAVE_MUTEX_VXWORKS
#include "taskLib.h"
/*
 * Use the taskLock() mutex to eliminate a race where two tasks are
 * trying to initialize the global lock at the same time.
 */
#define	DB_BEGIN_SINGLE_THREAD do {					\
	if (DB_GLOBAL(db_global_init))					\
		(void)semTake(DB_GLOBAL(db_global_lock), WAIT_FOREVER);	\
	else {								\
		taskLock();						\
		if (DB_GLOBAL(db_global_init)) {			\
			taskUnlock();					\
			(void)semTake(DB_GLOBAL(db_global_lock),	\
			    WAIT_FOREVER);				\
			continue;					\
		}							\
		DB_GLOBAL(db_global_lock) =				\
		    semBCreate(SEM_Q_FIFO, SEM_EMPTY);			\
		if (DB_GLOBAL(db_global_lock) != NULL)			\
			DB_GLOBAL(db_global_init) = 1;			\
		taskUnlock();						\
	}								\
} while (DB_GLOBAL(db_global_init) == 0)
#define	DB_END_SINGLE_THREAD	(void)semGive(DB_GLOBAL(db_global_lock))
#endif

/*
 * Single-threading defaults to a no-op.
 */
#ifndef DB_BEGIN_SINGLE_THREAD
#define	DB_BEGIN_SINGLE_THREAD
#endif
#ifndef DB_END_SINGLE_THREAD
#define	DB_END_SINGLE_THREAD
#endif

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/mutex_ext.h"
#endif /* !_DB_MUTEX_H_ */
