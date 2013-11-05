/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			locks.cpp
 *	DESCRIPTION:	Darwin specific semaphore support
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/alloc.h"
#include "gen/iberror.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if defined(COMMON_CLASSES_SEMAPHORE_POSIX_RT) || defined(COMMON_CLASSES_SEMAPHORE_COND_VAR)

static timespec getCurrentTime()
{
	timespec rc;

#ifdef HAVE_GETTIMEOFDAY
	struct timeval tp;
	GETTIMEOFDAY(&tp);
	rc.tv_sec = tp.tv_sec;
	rc.tv_nsec = tp.tv_usec * 1000;
#else
	struct timeb time_buffer;
	ftime(&time_buffer);
	rc.tv_sec = time_buffer.time;
	rc.tv_nsec = time_buffer.millitm * 1000000;
#endif

	return rc;
}

#endif	// semaphore kind defined



namespace Firebird {

#ifdef COMMON_CLASSES_SEMAPHORE_MACH

	void SignalSafeSemaphore::init()
	{
		semaphore = dispatch_semaphore_create(0);
		if (!semaphore)	// With const zero parameter this means OOM
		{
			BadAlloc::raise();
		}
	}

	SignalSafeSemaphore::~SignalSafeSemaphore()
	{
		dispatch_release(semaphore);
	}

#endif  // COMMON_CLASSES_SEMAPHORE_MACH



#ifdef COMMON_CLASSES_SEMAPHORE_POSIX_RT

#ifndef WORKING_SEM_INIT
static const char* semName = "/firebird_temp_sem";
#endif

#ifndef SEM_FAILED
#define SEM_FAILED ((sem_t*) (-1))
#endif

	void SignalSafeSemaphore::init()
	{
#ifdef WORKING_SEM_INIT
		if (sem_init(sem, 0, 0) == -1) {
			system_call_failed::raise("sem_init");
		}
#else
		sem = sem_open(semName, O_CREAT | O_EXCL, 0700, 0);
		if (sem == (sem_t*) SEM_FAILED) {
			system_call_failed::raise("sem_open");
		}
		sem_unlink(semName);
#endif
	}

	SignalSafeSemaphore::~SignalSafeSemaphore()
	{
#ifdef WORKING_SEM_INIT
		if (sem_destroy(sem) == -1) {
			system_call_failed::raise("sem_destroy");
		}
#else
		if (sem_close(sem) == -1) {
			system_call_failed::raise("sem_close");
		}
#endif
	}

	void SignalSafeSemaphore::enter()
	{
		do {
			if (sem_wait(sem) != -1)
				return;
		} while (errno == EINTR);
		system_call_failed::raise("semaphore.h: enter: sem_wait()");
	}

#ifdef HAVE_SEM_TIMEDWAIT
	bool SignalSafeSemaphore::tryEnter(const int seconds, int milliseconds)
	{
		// Return true in case of success
		milliseconds += seconds * 1000;
		if (milliseconds == 0)
		{
			// Instant try
			do {
				if (sem_trywait(sem) != -1)
					return true;
			} while (errno == EINTR);
			if (errno == EAGAIN)
				return false;
			system_call_failed::raise("sem_trywait");
		}
		if (milliseconds < 0)
		{
			// Unlimited wait, like enter()
			do {
				if (sem_wait(sem) != -1)
					return true;
			} while (errno == EINTR);
			system_call_failed::raise("sem_wait");
		}
		// Wait with timeout
		timespec timeout = getCurrentTime();
		timeout.tv_sec += milliseconds / 1000;
		timeout.tv_nsec += (milliseconds % 1000) * 1000000;
		timeout.tv_sec += (timeout.tv_nsec / 1000000000l);
		timeout.tv_nsec %= 1000000000l;
		int errcode = 0;
		do {
			int rc = sem_timedwait(sem, &timeout);
			if (rc == 0)
				return true;
			// fix for CORE-988, also please see
			// http://carcino.gen.nz/tech/linux/glibc_sem_timedwait_errors.php
			errcode = rc > 0 ? rc : errno;
		} while (errcode == EINTR);
		if (errcode == ETIMEDOUT) {
			return false;
		}
		system_call_failed::raise("sem_timedwait", errcode);
		return false;	// avoid warnings
	}
#endif // HAVE_SEM_TIMEDWAIT

#endif // COMMON_CLASSES_SEMAPHORE_POSIX_RT



#ifdef COMMON_CLASSES_SEMAPHORE_COND_VAR

	void Semaphore::init()
	{
		value = 0;
		int err = pthread_mutex_init(&mu, NULL);
		if (err != 0) {
			//gds__log("Error on semaphore.h: constructor");
			system_call_failed::raise("pthread_mutex_init", err);
		}
		err = pthread_cond_init(&cv, NULL);
		if (err != 0) {
			//gds__log("Error on semaphore.h: constructor");
			system_call_failed::raise("pthread_cond_init", err);
		}
	}

	void Semaphore::mtxLock()
	{
		int err = pthread_mutex_lock(&mu);
		if (err != 0)
		{
			system_call_failed::raise("pthread_mutex_lock", err);
		}
	}

	void Semaphore::mtxUnlock()
	{
		int err = pthread_mutex_unlock(&mu);
		if (err != 0)
		{
			system_call_failed::raise("pthread_mutex_unlock", err);
		}
	}

	Semaphore::~Semaphore()
	{
		fb_assert(value == 0);
		int err = pthread_mutex_destroy(&mu);
		if (err != 0) {
			//gds__log("Error on semaphore.h: destructor");
			//system_call_failed::raise("pthread_mutex_destroy", err);
		}
		err = pthread_cond_destroy(&cv);
		if (err != 0) {
			//gds__log("Error on semaphore.h: destructor");
			//system_call_failed::raise("pthread_cond_destroy", err);
		}
	}

	bool Semaphore::tryEnter(const int seconds, int milliseconds)
	{
		bool rt = false;	// Return true in case of success
		int err = 0;
		milliseconds += seconds * 1000;

		if (milliseconds == 0)
		{
			// Instant try
			err = pthread_mutex_trylock(&mu);
			if (err != 0)
			{
				if (err == EBUSY)
				{
					return false;
				}
				system_call_failed::raise("pthread_mutex_trylock", err);
			}

			if (value > 0)
			{
				--value;
				rt = true;
			}

			mtxUnlock();
			return rt;
		}

		if (milliseconds < 0)
		{
			// Unlimited wait
			enter();
			return true;
		}

		// Wait with timeout
		mtxLock();

		if (--value >= 0)
		{
			mtxUnlock();
			return true;
		}

		timespec timeout = getCurrentTime();
		timeout.tv_sec += milliseconds / 1000;
		timeout.tv_nsec += (milliseconds % 1000) * 1000000;
		timeout.tv_sec += (timeout.tv_nsec / 1000000000l);
		timeout.tv_nsec %= 1000000000l;
		err = pthread_cond_timedwait(&cv, &mu, &timeout);

		mtxUnlock();

		if (err == ETIMEDOUT)
		{
			++value;
		}
		else if (err != 0)
		{
			system_call_failed::raise("pthread_cond_timedwait", err);
		}
		else
		{
			rt = true;
		}

		return rt;
	}

	void Semaphore::enter()
	{
		mtxLock();

		if (--value >= 0)
		{
			mtxUnlock();
			return;
		}

		int err = pthread_cond_wait(&cv, &mu);

		mtxUnlock();

		if (err != 0)
		{
			system_call_failed::raise("pthread_cond_timedwait", err);
		}
	}

	void Semaphore::release(SLONG count)
	{
		int err = 0;
		for (int i = 0; i < count; i++)
		{
			mtxLock();

			if (++value > 0)
			{
				mtxUnlock();
				continue;
			}

			err = pthread_cond_signal(&cv);

			mtxUnlock();

			if (err != 0)
			{
				system_call_failed::raise("pthread_cond_broadcast", err);
			}
		}
	}

#endif // COMMON_CLASSES_SEMAPHORE_COND_VAR

}

