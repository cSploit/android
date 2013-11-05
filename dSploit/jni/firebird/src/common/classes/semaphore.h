/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			semaphore.h
 *	DESCRIPTION:	Semaphore lock
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef CLASSES_SEMAPHORE_H
#define CLASSES_SEMAPHORE_H

#include "../jrd/gdsassert.h"

#ifdef WIN_NT
// Note: Windows does not need signal safe version of the class

#include <windows.h>
#include <limits.h>

namespace Firebird
{

class MemoryPool;

class Semaphore
{
private:
	HANDLE hSemaphore;
	void init()
	{
		hSemaphore = CreateSemaphore(NULL, 0 /*initial count*/, INT_MAX, NULL);
		if (hSemaphore == NULL)
			system_call_failed::raise("CreateSemaphore");
	}

public:
	Semaphore() { init(); }
	explicit Semaphore(MemoryPool&) { init(); }

	~Semaphore()
	{
		if (hSemaphore && !CloseHandle(hSemaphore))
			system_call_failed::raise("CloseHandle");
	}

#define CLASSES_SEMAPHORE_H_HAS_TRYENTER 1
	bool tryEnter(const int seconds = 0, int milliseconds = 0)
	{
		milliseconds += seconds * 1000;
		DWORD result = WaitForSingleObject(hSemaphore, milliseconds >= 0 ? milliseconds : INFINITE);
		if (result == WAIT_FAILED)
			system_call_failed::raise("WaitForSingleObject");
		return result != WAIT_TIMEOUT;
	}

	void enter()
	{
		tryEnter(-1);
	}

	void release(SLONG count = 1)
	{
		if (!ReleaseSemaphore(hSemaphore, count, NULL))
			system_call_failed::raise("ReleaseSemaphore");
	}
};

} // namespace Firebird

#else // WINNT

#ifdef DARWIN

// Mach semaphore
#define COMMON_CLASSES_SEMAPHORE_MACH
#include <dispatch/dispatch.h>

namespace Firebird
{

class MemoryPool;

class SignalSafeSemaphore
{
private:
	dispatch_semaphore_t semaphore;

	void init();

public:
	SignalSafeSemaphore() { init(); }
	explicit SignalSafeSemaphore(MemoryPool&) { init(); }

	~SignalSafeSemaphore();

	void enter()
	{
		dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
	}

	void release(SLONG count = 1)
	{
		fb_assert(count >= 0);
		while (count--)
		{
			dispatch_semaphore_signal(semaphore);
		}
	}
};

} // namespace Firebird


#else // DARWIN

#ifdef HAVE_SEMAPHORE_H

#define COMMON_CLASSES_SEMAPHORE_POSIX_RT
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#ifndef WORKING_SEM_INIT
#include <fcntl.h>
#endif // WORKING_SEM_INIT

namespace Firebird
{

class SignalSafeSemaphore
{
private:
#ifdef WORKING_SEM_INIT
	sem_t sem[1];
#else
	sem_t* sem;
#endif // WORKING_SEM_INIT

	void init();

public:
	SignalSafeSemaphore() { init(); }
	explicit SignalSafeSemaphore(MemoryPool&) { init(); }

	~SignalSafeSemaphore();
	void enter();

	void release(SLONG count = 1)
	{
		for (int i = 0; i < count; i++)
		{
			if (sem_post(sem) == -1)
			{
				system_call_failed::raise("semaphore.h: release: sem_post()");
			}
		}
	}

#ifdef HAVE_SEM_TIMEDWAIT
	// In case when sem_timedwait() is implemented by host OS,
	// class SignalSafeSemaphore may have this function:
#define CLASSES_SEMAPHORE_H_HAS_TRYENTER 1
	bool tryEnter(const int seconds = 0, int milliseconds = 0);
#endif // HAVE_SEM_TIMEDWAIT

};

} // namespace Firebird

#endif // HAVE_SEMAPHORE_H

#endif // DARWIN

#ifdef CLASSES_SEMAPHORE_H_HAS_TRYENTER

// In case when sem_timedwait() is implemented by host OS,
// SignalSafeSemaphore and Semaphore are just the same
namespace Firebird
{
typedef SignalSafeSemaphore Semaphore;
}

#else // CLASSES_SEMAPHORE_H_HAS_TRYENTER

// Should implement Semaphore independent from SignalSafeSemaphore.
// In the worst case no SignalSafeSemaphore at all (and no SS for that platform).
#define COMMON_CLASSES_SEMAPHORE_COND_VAR
#include <pthread.h>
#include <errno.h>

namespace Firebird
{

class Semaphore
{
private:
	pthread_mutex_t	mu;
	pthread_cond_t	cv;
	int value;

	void init();
	void mtxLock();
	void mtxUnlock();

public:
	Semaphore() { init(); }
	explicit Semaphore(MemoryPool&) { init(); }

	~Semaphore();

	bool tryEnter(const int seconds = 0, int milliseconds = 0);
	void enter();
	void release(SLONG count = 1);
};

} // namespace Firebird

#endif // CLASSES_SEMAPHORE_H_HAS_TRYENTER

#endif // WIN_NT

#endif // CLASSES_SEMAPHORE_H
