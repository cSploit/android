/*
 *	PROGRAM:	Client/Server common code
 *	MODULE:		condition.h
 *	DESCRIPTION:	Condition variable
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
 *  The Original Code was created by Roman Simakov
 *  for the Red Soft Corp and Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Roman Simakov
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s):
 *	________________________________________
 *
 */
#ifndef CLASSES_CONDITION_H
#define CLASSES_CONDITION_H

#include "../jrd/gdsassert.h"

#ifdef WIN_NT

#include <windows.h>
#include <limits.h>

namespace Firebird
{

class MemoryPool;

class Condition
{
private:
	AtomicCounter waiters;

	enum
	{
		SIGNAL = 0,
		BROADCAST,
		MAX_EVENTS
	};

	HANDLE events[MAX_EVENTS];

	void init()
	{
		events[SIGNAL] = CreateEvent(NULL,  // no security
			                         FALSE, // auto-reset event
									 FALSE, // non-signaled initially
    	                             NULL); // unnamed

		// Create a manual-reset event.
		events[BROADCAST] = CreateEvent(NULL,  // no security
    	                                TRUE,  // manual-reset
    	                                FALSE, // non-signaled initially
    	                                NULL); // unnamed

		if (!events[SIGNAL] || !events[BROADCAST])
			system_call_failed::raise("CreateCondition(Event)");
	}

public:
	Condition()	{ init(); }
	explicit Condition(MemoryPool&) { init(); }

	~Condition()
	{
		if (events[SIGNAL] && !CloseHandle(events[SIGNAL]))
			system_call_failed::raise("CloseHandle(SIGNAL)");
		if (events[BROADCAST] && !CloseHandle(events[BROADCAST]))
			system_call_failed::raise("CloseHandle(BROADCAST)");
	}

	void wait(Mutex& m)
	{
		++waiters;

		m.leave();

		if (WaitForMultipleObjects((DWORD) MAX_EVENTS, events, FALSE, INFINITE) == WAIT_FAILED)
			system_call_failed::raise("WaitForMultipleObjects");

		if (--waiters == 0)
		{
			if (!ResetEvent(events[BROADCAST]))
				system_call_failed::raise("ResetEvent(BROADCAST)");
		}

		m.enter();
	}

	void notifyOne()
	{
		if (waiters.value() > 0)
		{
			if (!SetEvent(events[SIGNAL]))
				system_call_failed::raise("SetEvent(SIGNAL)");
		}
	}

	void notifyAll()
	{
		if (waiters.value() > 0)
		{
			if (!SetEvent(events[BROADCAST]))
				system_call_failed::raise("SetEvent");
		}
	}
};

} // namespace Firebird

#else // WIN_NT

#include <pthread.h>
#include <errno.h>

namespace Firebird
{

class Condition
{
private:
	pthread_cond_t cv;

	void init()
	{
		int err = pthread_cond_init(&cv, NULL);
		if (err != 0) {
			//gds__log("Error on semaphore.h: constructor");
			system_call_failed::raise("pthread_cond_init", err);
		}
	}

public:
	Condition() { init(); }
	explicit Condition(MemoryPool&) { init(); }

	~Condition()
	{
		int err = pthread_cond_destroy(&cv);
		if (err != 0) {
			//gds__log("Error on semaphore.h: destructor");
			//system_call_failed::raise("pthread_cond_destroy", err);
		}
	}

	void notifyOne()
	{
		int err = pthread_cond_signal(&cv);
		if (err != 0)
			system_call_failed::raise("pthread_cond_broadcast", err);
	}

	void notifyAll()
	{
		int err = pthread_cond_broadcast(&cv);
		if (err != 0)
			system_call_failed::raise("pthread_cond_broadcast", err);
	}

	void wait(Mutex& m)
	{
		int err = pthread_cond_wait(&cv, &m.mlock);
		if (err != 0)
			system_call_failed::raise("pthread_mutex_lock", err);
	}
};

} // namespace Firebird

#endif // WIN_NT

#endif // CLASSES_SEMAPHORE_H
