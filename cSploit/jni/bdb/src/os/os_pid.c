/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_id --
 *	Return the current process ID.
 *
 * PUBLIC: void __os_id __P((DB_ENV *, pid_t *, db_threadid_t*));
 */
void
__os_id(dbenv, pidp, tidp)
	DB_ENV *dbenv;
	pid_t *pidp;
	db_threadid_t *tidp;
{
	/*
	 * We can't depend on dbenv not being NULL, this routine is called
	 * from places where there's no DB_ENV handle.
	 *
	 * We cache the pid in the ENV handle, getting the process ID is a
	 * fairly slow call on lots of systems.
	 */
	if (pidp != NULL) {
		if (dbenv == NULL) {
#if defined(HAVE_VXWORKS)
			*pidp = taskIdSelf();
#else
			*pidp = getpid();
#endif
		} else
			*pidp = dbenv->env->pid_cache;
	}

/*
 * When building on MinGW, we define both HAVE_PTHREAD_SELF and DB_WIN32,
 * and we are using pthreads instead of Windows threads implementation.
 * So here, we need to check the thread implementations before checking
 * the platform.
 */
	if (tidp != NULL) {
#if defined(HAVE_PTHREAD_SELF)
		*tidp = pthread_self();
#elif defined(HAVE_MUTEX_UI_THREADS)
		*tidp = thr_self();
#elif defined(DB_WIN32)
		*tidp = GetCurrentThreadId();
#else
		/*
		 * Default to just getpid.
		 */
		DB_THREADID_INIT(*tidp);
#endif
	}
}
