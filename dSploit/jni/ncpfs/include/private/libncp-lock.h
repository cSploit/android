/* original came from bits/libc-lock.h in libc-2.1.1 */

/* libc-internal interface for mutex locks.  LinuxThreads version.
   Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _PRIVATE_LIBNCP_LOCK_H
#define _PRIVATE_LIBNCP_LOCK_H 1

#ifdef _REENTRANT

#include <pthread.h>

typedef pthread_mutex_t ncpt_mutex_t;
typedef pthread_once_t ncpt_once_t;
typedef pthread_key_t ncpt_key_t;

#define NCPT_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */
#define ncpt_mutex_init(NAME) \
  (pthread_mutex_init ? pthread_mutex_init (NAME, NULL) : 0);

/* Same as last but this time we initialize a recursive mutex.  */
#define ncpt_mutex_init_recursive(NAME) \
  do {									      \
    if (pthread_mutex_init && pthread_mutexattr_init &&			      \
        pthread_mutexattr_settype && pthread_mutexattr_destroy)		      \
      {									      \
	pthread_mutexattr_t __attr;					      \
	pthread_mutexattr_init (&__attr);				      \
	pthread_mutexattr_settype (&__attr, PTHREAD_MUTEX_RECURSIVE_NP);      \
	pthread_mutex_init (NAME, &__attr);				      \
	pthread_mutexattr_destroy (&__attr);				      \
      }									      \
  } while (0);

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
#define ncpt_mutex_destroy(NAME) \
  (pthread_mutex_destroy ? pthread_mutex_destroy (NAME) : 0);

/* Lock the named lock variable.  */
#define ncpt_mutex_lock(NAME) \
  (pthread_mutex_lock ? pthread_mutex_lock (NAME) : 0);

/* Lock the recursive named lock variable.  */
#define ncpt_mutex_lock_recursive(NAME) ncpt_mutex_lock (NAME)

/* Try to lock the named lock variable.  */
#define ncpt_mutex_trylock(NAME) \
  (pthread_mutex_trylock ? pthread_mutex_trylock (NAME) : 0)

/* Try to lock the recursive named lock variable.  */
#define ncpt_mutex_trylock_recursive(NAME) ncpt_mutex_trylock (NAME)

/* Unlock the named lock variable.  */
#define ncpt_mutex_unlock(NAME) \
  (pthread_mutex_unlock ? pthread_mutex_unlock (NAME) : 0);

/* Unlock the recursive named lock variable.  */
#define ncpt_mutex_unlock_recursive(NAME) ncpt_mutex_unlock (NAME)

#define NCPT_ONCE_INIT PTHREAD_ONCE_INIT

/* Call handler iff the first call.  */
#define ncpt_once(ONCE_CONTROL, INIT_FUNCTION) \
  do {									      \
    if (pthread_once)						      \
      pthread_once ((ONCE_CONTROL), (INIT_FUNCTION));		      \
    else if (*(ONCE_CONTROL) == NCPT_ONCE_INIT) {					      \
      INIT_FUNCTION ();							      \
      *(ONCE_CONTROL) = 1;						      \
    }									      \
  } while (0)


/* Start critical region with cleanup.  */
#define __libc_cleanup_region_start(FCT, ARG) \
  { struct _pthread_cleanup_buffer _buffer;				      \
    int _avail = _pthread_cleanup_push_defer != NULL;			      \
    if (_avail) {							      \
      _pthread_cleanup_push_defer (&_buffer, (FCT), (ARG));		      \
    }

/* End critical region with cleanup.  */
#define __libc_cleanup_region_end(DOIT) \
    if (_avail) {							      \
      _pthread_cleanup_pop_restore (&_buffer, (DOIT));			      \
    }									      \
  }

/* Sometimes we have to exit the block in the middle.  */
#define __libc_cleanup_end(DOIT) \
    if (_avail) {							      \
      _pthread_cleanup_pop_restore (&_buffer, (DOIT));			      \
    }

#if 0
/* Create thread-specific key.  */
static inline int ncpt_key_create(ncpt_key_t* KEY, void (*DESTRUCTOR)(void*)) {
	ncpt_key_t k;
	
	if (pthread_key_create)
		return pthread_key_create(KEY, DESTRUCTOR);
	k = (ncpt_key_t)malloc(sizeof(void*));
	if (!k)
		return ENOMEM;
	*(void**)k = NULL;
	*KEY = k;
	return 0;
}

/* Get thread-specific data.  */
static inline void* ncpt_getspecific(ncpt_key_t KEY) {
	if (pthread_getspecific)
		return pthread_getspecific(KEY);
	if (KEY)
		return *(void**)KEY;
	return NULL;
}

/* Set thread-specific data.  */
static inline int ncpt_setspecific(ncpt_key_t KEY, const void* VALUE) {
	if (pthread_setspecific)
		return pthread_setspecific(KEY, VALUE);
	if (!KEY)
		return EINVAL;
	*(void**)KEY = VALUE;
	return 0;
}
#endif

/* Register handlers to execute before and after `fork'.  */
#define ncpt_atfork(PREPARE, PARENT, CHILD) \
  (pthread_atfork ? pthread_atfork (PREPARE, PARENT, CHILD) : 0)


/* Make the pthread functions weak so that we can elide them from
   single-threaded processes.  */
#ifndef __NO_WEAK_PTHREAD_ALIASES
# ifdef weak_extern
weak_extern (pthread_mutex_init)
weak_extern (pthread_mutex_destroy)
weak_extern (pthread_mutex_lock)
weak_extern (pthread_mutex_trylock)
weak_extern (pthread_mutex_unlock)
weak_extern (pthread_mutexattr_init)
weak_extern (pthread_mutexattr_destroy)
weak_extern (pthread_mutexattr_settype)
weak_extern (pthread_key_create)
weak_extern (pthread_setspecific)
weak_extern (pthread_getspecific)
weak_extern (pthread_once)
weak_extern (pthread_initialize)
weak_extern (pthread_atfork)
weak_extern (_pthread_cleanup_push_defer)
weak_extern (_pthread_cleanup_pop_restore)
# else
#  pragma weak pthread_mutex_init
#  pragma weak pthread_mutex_destroy
#  pragma weak pthread_mutex_lock
#  pragma weak pthread_mutex_trylock
#  pragma weak pthread_mutex_unlock
#  pragma weak pthread_mutexattr_init
#  pragma weak pthread_mutexattr_destroy
#  pragma weak pthread_mutexattr_settype
#  pragma weak pthread_key_create
#  pragma weak pthread_setspecific
#  pragma weak pthread_getspecific
#  pragma weak pthread_once
#  pragma weak pthread_initialize
#  pragma weak pthread_atfork
#  pragma weak _pthread_cleanup_push_defer
#  pragma weak _pthread_cleanup_pop_restore
# endif
#endif

#else	/* _REENTRANT */

typedef unsigned int ncpt_mutex_t;
typedef unsigned int ncpt_once_t;
typedef void**       ncpt_key_t;

#define NCPT_MUTEX_INITIALIZER (0)

/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */
static inline int ncpt_mutex_init(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Same as last but this time we initialize a recursive mutex.  */
static inline int ncpt_mutex_init_recursive(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
static inline int ncpt_mutex_destroy(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Lock the named lock variable.  */
static inline int ncpt_mutex_lock(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Lock the recursive named lock variable.  */
#define ncpt_mutex_lock_recursive(NAME) ncpt_mutex_lock(NAME)

/* Try to lock the named lock variable.  */
static inline int ncpt_mutex_trylock(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Try to lock the recursive named lock variable.  */
#define ncpt_mutex_trylock_recursive(NAME) ncpt_mutex_trylock(NAME)

/* Unlock the named lock variable.  */
static inline int ncpt_mutex_unlock(ncpt_mutex_t* mutex) {
	return 0;
	(void)mutex;
}

/* Unlock the recursive named lock variable.  */
#define ncpt_mutex_unlock_recursive(NAME) ncpt_mutex_unlock(NAME)

#define NCPT_ONCE_INIT (0)

/* Call handler iff the first call.  */
#define ncpt_once(ONCE_CONTROL, INIT_FUNCTION) \
  do {								      \
    if (*(ONCE_CONTROL) == NCPT_ONCE_INIT) {					      \
      INIT_FUNCTION ();						      \
      *(ONCE_CONTROL) = 1;					      \
    }								      \
  } while (0)


/* Start critical region with cleanup.  */
#define __libc_cleanup_region_start(FCT, ARG) \
  { struct _pthread_cleanup_buffer _buffer;				      \
    int _avail = _pthread_cleanup_push_defer != NULL;			      \
    if (_avail) {							      \
      _pthread_cleanup_push_defer (&_buffer, (FCT), (ARG));		      \
    }

/* End critical region with cleanup.  */
#define __libc_cleanup_region_end(DOIT) \
    if (_avail) {							      \
      _pthread_cleanup_pop_restore (&_buffer, (DOIT));			      \
    }									      \
  }

/* Sometimes we have to exit the block in the middle.  */
#define __libc_cleanup_end(DOIT) \
    if (_avail) {							      \
      _pthread_cleanup_pop_restore (&_buffer, (DOIT));			      \
    }

#if 0
/* Create thread-specific key.  */
static inline int ncpt_key_create(ncpt_key_t* KEY, void (*DESTRUCTOR)(void*)) {
	ncpt_key_t k = (ncpt_key_t)malloc(sizeof(void*));
	if (!k)
		return ENOMEM;
	*k = NULL;
	*KEY = k;
	return 0;
}

/* Get thread-specific data.  */
static inline void* ncpt_getspecific(ncpt_key_t KEY) {
	return KEY ? *KEY : NULL;
}

/* Set thread-specific data.  */
static inline int ncpt_setspecific(ncpt_key_t KEY, const void* VALUE) {
	if (!KEY)
		return EINVAL;
	*KEY = VALUE;
	return 0;
}
#endif

/* Register handlers to execute before and after `fork'.  */
#define ncpt_atfork(PREPARE, PARENT, CHILD) (0)

#endif	/* _REENTRANT */

#endif	/* private/libncp-lock.h */
