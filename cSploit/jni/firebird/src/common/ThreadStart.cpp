/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ThreadData.cpp
 *	DESCRIPTION:	Thread support routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * Alex Peshkov
 */

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include "../common/ThreadStart.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_s_proto.h"
#include "../common/gdsassert.h"

#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../common/classes/locks.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/Synchronize.h"


namespace
{

// due to same names of parameters for various ThreadData::start(...),
// we may use common macro for various platforms
#define THREAD_ENTRYPOINT threadStart
#define THREAD_ARG static_cast<THREAD_ENTRY_PARAM> (FB_NEW(*getDefaultMemoryPool()) \
		ThreadArgs(reinterpret_cast<THREAD_ENTRY_RETURN (THREAD_ENTRY_CALL *) (THREAD_ENTRY_PARAM)>(routine), \
		static_cast<THREAD_ENTRY_PARAM>(arg)))

class ThreadArgs
{
public:
	typedef THREAD_ENTRY_RETURN (THREAD_ENTRY_CALL *Routine) (THREAD_ENTRY_PARAM);
	typedef THREAD_ENTRY_PARAM Arg;
private:
	Routine routine;
	Arg arg;
public:
	ThreadArgs(Routine r, Arg a) : routine(r), arg(a) { }
	ThreadArgs(const ThreadArgs& t) : routine(t.routine), arg(t.arg) { }
	void run() { routine(arg); }
private:
	ThreadArgs& operator=(const ThreadArgs&);
};

#ifdef __cplusplus
extern "C"
#endif
THREAD_ENTRY_DECLARE threadStart(THREAD_ENTRY_PARAM arg)
{
	fb_assert(arg);
	Firebird::ThreadSync thread("threadStart");
	MemoryPool::setContextPool(getDefaultMemoryPool());
	ThreadArgs localArgs(*static_cast<ThreadArgs*>(arg));
	delete static_cast<ThreadArgs*>(arg);
	localArgs.run();
	return 0;
}

} // anonymous namespace

#ifdef USE_POSIX_THREADS
#define START_THREAD
void Thread::start(ThreadEntryPoint* routine, void* arg, int priority_arg, Handle* p_handle)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( P O S I X )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/
	pthread_t thread;
	pthread_attr_t pattr;
	int state;

#if defined (LINUX) || defined (FREEBSD)
	if (state = pthread_create(&thread, NULL, THREAD_ENTRYPOINT, THREAD_ARG))
		Firebird::system_call_failed::raise("pthread_create", state);

	if (!p_handle)
	{
		if (state = pthread_detach(thread))
			Firebird::system_call_failed::raise("pthread_detach", state);
	}
#else
	state = pthread_attr_init(&pattr);
	if (state)
		Firebird::system_call_failed::raise("pthread_attr_init", state);

#if defined(_AIX) || defined(DARWIN)
// adjust stack size

// For AIX 32-bit compiled applications, the default stacksize is 96 KB,
// see <pthread.h>. For 64-bit compiled applications, the default stacksize
// is 192 KB. This is too small - see HP-UX note above

// For MaxOS default stack is 512 KB, which is also too small in 2012.

	size_t stack_size;
	state = pthread_attr_getstacksize(&pattr, &stack_size);
	if (state)
		Firebird::system_call_failed::raise("pthread_attr_getstacksize");

	if (stack_size < 0x400000L)
	{
		state = pthread_attr_setstacksize(&pattr, 0x400000L);
		if (state)
			Firebird::system_call_failed::raise("pthread_attr_setstacksize", state);
	}
#endif // _AIX

	state = pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	if (state)
		Firebird::system_call_failed::raise("pthread_attr_setscope", state);

	if (!p_handle)
	{
		state = pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
		if (state)
			Firebird::system_call_failed::raise("pthread_attr_setdetachstate", state);
	}
	state = pthread_create(&thread, &pattr, THREAD_ENTRYPOINT, THREAD_ARG);
	int state2 = pthread_attr_destroy(&pattr);
	if (state)
		Firebird::system_call_failed::raise("pthread_create", state);
	if (state2)
		Firebird::system_call_failed::raise("pthread_attr_destroy", state2);

#endif

	if (p_handle)
	{
#ifdef HAVE_PTHREAD_CANCEL
		int dummy;		// We do not want to know old cancel type
		state = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
		if (state)
			 Firebird::system_call_failed::raise("pthread_setcanceltype", state);
#endif
		*p_handle = thread;
	}
}

void Thread::waitForCompletion(Handle& thread)
{
	int state = pthread_join(thread, NULL);
	if (state)
		Firebird::system_call_failed::raise("pthread_join", state);
}

void Thread::kill(Handle& thread)
{
#ifdef HAVE_PTHREAD_CANCEL
	int state = pthread_cancel(thread);
	if (state)
		Firebird::system_call_failed::raise("pthread_cancel", state);
	waitForCompletion(thread);
#endif
}
#endif /* USE_POSIX_THREADS */


#ifdef WIN_NT
#define START_THREAD
void Thread::start(ThreadEntryPoint* routine, void* arg, int priority_arg, Handle* p_handle)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/

	int priority;

	switch (priority_arg)
	{
	case THREAD_critical:
		priority = THREAD_PRIORITY_TIME_CRITICAL;
		break;
	case THREAD_high:
		priority = THREAD_PRIORITY_HIGHEST;
		break;
	case THREAD_medium_high:
		priority = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case THREAD_medium:
		priority = THREAD_PRIORITY_NORMAL;
		break;
	case THREAD_medium_low:
		priority = THREAD_PRIORITY_BELOW_NORMAL;
		break;
	case THREAD_low:
	default:
		priority = THREAD_PRIORITY_LOWEST;
		break;
	}

	/* I have changed the CreateThread here to _beginthreadex() as using
	 * CreateThread() can lead to memory leaks caused by C-runtime library.
	 * Advanced Windows by Richter pg. # 109. */

	unsigned thread_id;
	unsigned long real_handle =
		_beginthreadex(NULL, 0, THREAD_ENTRYPOINT, THREAD_ARG, CREATE_SUSPENDED, &thread_id);
	if (!real_handle)
	{
		Firebird::system_call_failed::raise("_beginthreadex", GetLastError());
	}
	HANDLE handle = reinterpret_cast<HANDLE>(real_handle);

	SetThreadPriority(handle, priority);

	ResumeThread(handle);

	if (p_handle)
	{
		*p_handle = handle;
	}
	else
	{
		CloseHandle(handle);
	}
}

void Thread::waitForCompletion(Handle& handle)
{
	WaitForSingleObject(handle, 500);
	CloseHandle(handle);
	handle = 0;
}

void Thread::kill(Handle& handle)
{
	TerminateThread(handle, -1);
	CloseHandle(handle);
	handle = 0;
}

#endif  // WIN_NT


#ifndef START_THREAD
void Thread::start(ThreadEntryPoint* routine, void* arg, int priority_arg, Handle* p_handle)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *	Wrong attempt to start a new thread.
 *
 **************************************/

}

void Thread::waitForCompletion(Handle&)
{
}

void Thread::kill(Handle&)
{
}
#endif  // START_THREAD
