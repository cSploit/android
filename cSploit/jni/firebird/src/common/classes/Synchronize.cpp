/*
*
*     The contents of this file are subject to the Initial
*     Developer's Public License Version 1.0 (the "License");
*     you may not use this file except in compliance with the
*     License. You may obtain a copy of the License at
*     http://www.ibphoenix.com/idpl.html.
*
*     Software distributed under the License is distributed on
*     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
*     express or implied.  See the License for the specific
*     language governing rights and limitations under the License.
*
*     The contents of this file or any work derived from this file
*     may not be distributed under any other license whatsoever
*     without the express prior written permission of the original
*     author.
*
*
*  The Original Code was created by James A. Starkey for IBPhoenix.
*
*  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
*  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
*  All Rights Reserved.
*
*  The Code was ported into Firebird Open Source RDBMS project by
*  Vladyslav Khorsun at 2010
*
*  Contributor(s):
*/

#include "firebird.h"
#include "fb_tls.h"
#include "../thd.h"
#include "SyncObject.h"
#include "Synchronize.h"

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#define NANO		1000000000
#define MICRO		1000000

namespace Firebird {

Synchronize::Synchronize()
	: shutdownInProgress(false),
	  sleeping(false),
	  wakeup(false)
{
#ifdef WIN_NT
	evnt = CreateEvent(NULL, false, false, NULL);
#else
	int ret = pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condition, NULL);
#endif
}

Synchronize::~Synchronize()
{
#ifdef WIN_NT
	CloseHandle(evnt);
#else
	int ret = pthread_mutex_destroy(&mutex);
	ret = pthread_cond_destroy(&condition);
#endif
}

void Synchronize::sleep()
{
	sleeping = true;

#ifdef WIN_NT

#ifdef DEV_BUILD
	for (;;)
	{
		const int n = WaitForSingleObject(evnt, 10000);
		if (n != WAIT_TIMEOUT)
			break;
	}
#else
	sleep(INFINITE);
#endif

#else
	int ret = pthread_mutex_lock(&mutex);
	if (ret)
		system_call_failed::raise("pthread_mutex_lock", ret);

	while (!wakeup)
		pthread_cond_wait(&condition, &mutex);

	wakeup = false;
	ret = pthread_mutex_unlock(&mutex);
	if (ret)
		system_call_failed::raise("pthread_mutex_unlock", ret);
#endif

	sleeping = false;
}

bool Synchronize::sleep(int milliseconds)
{
	sleeping = true;

#ifdef WIN_NT
	const int n = WaitForSingleObject(evnt, milliseconds);
	sleeping = false;

	return n != WAIT_TIMEOUT;
#else
	struct timeval microTime;
	int ret = gettimeofday(&microTime, NULL);
	SINT64 nanos = (SINT64) microTime.tv_sec * NANO + microTime.tv_usec * 1000 +
		(SINT64) milliseconds * 1000000;
	struct timespec nanoTime;
	nanoTime.tv_sec = nanos / NANO;
	nanoTime.tv_nsec = nanos % NANO;

	ret = pthread_mutex_lock (&mutex);
	if (ret)
		system_call_failed::raise("pthread_mutex_lock", ret);

	int seconds = nanoTime.tv_sec - microTime.tv_sec;

	while (!wakeup)
	{
#ifdef MVS
		ret = pthread_cond_timedwait(&condition, &mutex, &nanoTime);
		if (ret == -1 && errno == EAGAIN)
			ret = ETIMEDOUT;
		break;
#else
		ret = pthread_cond_timedwait(&condition, &mutex, &nanoTime);
		if (ret == ETIMEDOUT)
			break;
#endif
		/***
		if (!wakeup)
			Log::debug("Synchronize::sleep(milliseconds): unexpected wakeup, ret %d\n", ret);
		***/
	}

	sleeping = false;
	wakeup = false;
	pthread_mutex_unlock(&mutex);
	return ret != ETIMEDOUT;
#endif
}

void Synchronize::wake()
{
#ifdef WIN_NT
	SetEvent(evnt);
#else
	int ret = pthread_mutex_lock(&mutex);
	if (ret)
		system_call_failed::raise("pthread_mutex_lock", ret);

	wakeup = true;
	pthread_cond_broadcast(&condition);

	ret = pthread_mutex_unlock(&mutex);
	if (ret)
		system_call_failed::raise("pthread_mutex_unlock", ret);
#endif
}

void Synchronize::shutdown()
{
	shutdownInProgress = true;
	wake();
}


/// ThreadSync

TLS_DECLARE(ThreadSync*, threadIndex);

ThreadSync::ThreadSync(const char* desc)
	: threadId(getCurrentThreadId()), nextWaiting(NULL), prevWaiting(NULL),
	  lockType(SYNC_NONE), lockGranted(false), lockPending(NULL), locks(NULL),
	  description(desc)
{
	setThread(this);
}

ThreadSync::~ThreadSync()
{
	setThread(NULL);
}

ThreadSync* ThreadSync::findThread()
{
	return TLS_GET(threadIndex);
}

ThreadSync* ThreadSync::getThread(const char* desc)
{
	ThreadSync* thread = findThread();

	if (!thread)
	{
		thread = new ThreadSync(desc);

		fb_assert(thread == findThread());
	}

	return thread;
}

void ThreadSync::setThread(ThreadSync* thread)
{
	TLS_SET(threadIndex, thread);
}

ThreadId ThreadSync::getCurrentThreadId()
{
	return getThreadId();
}

const char* ThreadSync::getWhere() const
{
	if (lockPending && lockPending->where)
		return lockPending->where;

	return "";
}

void ThreadSync::grantLock(SyncObject* lock)
{
	fb_assert(!lockGranted);
	fb_assert(!lockPending || lockPending->syncObject == lock);

	lockGranted = true;
	lockPending = NULL;

	wake();
}

} // namespace Firebird
