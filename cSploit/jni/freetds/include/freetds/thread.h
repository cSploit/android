/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 *
 * Copyright (C) 2005 Liam Widdowson
 * Copyright (C) 2010-2012 Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef TDSTHREAD_H
#define TDSTHREAD_H 1

/* $Id: tdsthread.h,v 1.13 2011-09-09 08:50:47 freddy77 Exp $ */

#undef TDS_HAVE_MUTEX

#if defined(_THREAD_SAFE) && defined(TDS_HAVE_PTHREAD_MUTEX)

#include <pthread.h>

typedef pthread_mutex_t tds_mutex;
#define TDS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline void tds_mutex_lock(tds_mutex *mtx)
{
	pthread_mutex_lock(mtx);
}

static inline int tds_mutex_trylock(tds_mutex *mtx)
{
	return pthread_mutex_trylock(mtx);
}

static inline void tds_mutex_unlock(tds_mutex *mtx)
{
	pthread_mutex_unlock(mtx);
}

static inline int tds_mutex_init(tds_mutex *mtx)
{
	return pthread_mutex_init(mtx, NULL);
}

static inline void tds_mutex_free(tds_mutex *mtx)
{
	pthread_mutex_destroy(mtx);
}

typedef pthread_cond_t tds_condition;

static inline int tds_cond_init(tds_condition *cond)
{
	return pthread_cond_init(cond, NULL);
}
static inline int tds_cond_destroy(tds_condition *cond)
{
	return pthread_cond_destroy(cond);
}
static inline int tds_cond_signal(tds_condition *cond)
{
	return pthread_cond_signal(cond);
}
static inline int tds_cond_wait(tds_condition *cond, pthread_mutex_t *mtx)
{
	return pthread_cond_wait(cond, mtx);
}
int tds_cond_timedwait(tds_condition *cond, pthread_mutex_t *mtx, int timeout_sec);

#define TDS_HAVE_MUTEX 1

typedef pthread_t tds_thread;
typedef void *(*tds_thread_proc)(void *arg);
#define TDS_THREAD_PROC_DECLARE(name, arg) \
	void *name(void *arg)

static inline int tds_thread_create(tds_thread *ret, tds_thread_proc proc, void *arg)
{
	return pthread_create(ret, NULL, proc, arg);
}

static inline int tds_thread_join(tds_thread th, void **ret)
{
	return pthread_join(th, ret);
}

#elif defined(_WIN32)

struct ptw32_mcs_node_t_;

typedef struct {
	struct ptw32_mcs_node_t_ *lock;
	LONG done;
	CRITICAL_SECTION crit;
} tds_mutex;

#define TDS_MUTEX_INITIALIZER { NULL, 0 }

static inline int
tds_mutex_init(tds_mutex *mtx)
{
	mtx->lock = NULL;
	mtx->done = 0;
	return 0;
}

void tds_win_mutex_lock(tds_mutex *mutex);

static inline void tds_mutex_lock(tds_mutex *mtx)
{
	if ((mtx)->done)
		EnterCriticalSection(&(mtx)->crit);
	else
		tds_win_mutex_lock(mtx);
}

int tds_mutex_trylock(tds_mutex *mtx);

static inline void tds_mutex_unlock(tds_mutex *mtx)
{
	LeaveCriticalSection(&(mtx)->crit);
}

static inline void tds_mutex_free(tds_mutex *mtx)
{
	if ((mtx)->done) {
		DeleteCriticalSection(&(mtx)->crit);
		(mtx)->done = 0;
	}
}

#define TDS_HAVE_MUTEX 1

/* easy way, only single signal supported */
typedef void *TDS_CONDITION_VARIABLE;
typedef union {
	HANDLE ev;
	TDS_CONDITION_VARIABLE cv;
} tds_condition;

extern int (*tds_cond_init)(tds_condition *cond);
extern int (*tds_cond_destroy)(tds_condition *cond);
extern int (*tds_cond_signal)(tds_condition *cond);
extern int (*tds_cond_timedwait)(tds_condition *cond, tds_mutex *mtx, int timeout_sec);
static inline int tds_cond_wait(tds_condition *cond, tds_mutex *mtx)
{
	return tds_cond_timedwait(cond, mtx, -1);
}

typedef HANDLE tds_thread;
typedef void *(WINAPI *tds_thread_proc)(void *arg);
#define TDS_THREAD_PROC_DECLARE(name, arg) \
	void *WINAPI name(void *arg)

static inline int tds_thread_create(tds_thread *ret, tds_thread_proc proc, void *arg)
{
	*ret = CreateThread(NULL, 0, (DWORD_PTR (WINAPI *)(void*)) proc, arg, 0, NULL);
	return *ret != NULL ? 0 : 11 /* EAGAIN */;
}

static inline int tds_thread_join(tds_thread th, void **ret)
{
	if (WaitForSingleObject(th, INFINITE) == WAIT_OBJECT_0) {
		DWORD_PTR r;
		if (GetExitCodeThread(th, &r)) {
			*ret = (void*) r;
			CloseHandle(th);
			return 0;
		}
	}
	CloseHandle(th);
	return 22 /* EINVAL */;
}

#else

/* define noops as "successful" */
typedef struct {
} tds_mutex;

#define TDS_MUTEX_INITIALIZER {}

static inline void tds_mutex_lock(tds_mutex *mtx)
{
}

static inline int tds_mutex_trylock(tds_mutex *mtx)
{
	return 0;
}

static inline void tds_mutex_unlock(tds_mutex *mtx)
{
}

static inline int tds_mutex_init(tds_mutex *mtx)
{
	return 0;
}

static inline void tds_mutex_free(tds_mutex *mtx)
{
}

typedef struct {
} tds_condition;

static inline int tds_cond_init(tds_condition *cond)
{
	return 0;
}
static inline int tds_cond_destroy(tds_condition *cond)
{
	return 0;
}
#define tds_cond_signal(cond) \
	FreeTDS_Condition_not_compiled

#define tds_cond_wait(cond, mtx) \
	FreeTDS_Condition_not_compiled

#define tds_cond_timedwait(cond, mtx, timeout_sec) \
	FreeTDS_Condition_not_compiled

typedef struct {
} tds_thread;

typedef void *(*tds_thread_proc)(void *arg);
#define TDS_THREAD_PROC_DECLARE(name, arg) \
	void *name(void *arg)

#define tds_thread_create(ret, proc, arg) \
	FreeTDS_Thread_not_compiled

#define tds_thread_join(th, ret) \
	FreeTDS_Thread_not_compiled

#endif

#endif
