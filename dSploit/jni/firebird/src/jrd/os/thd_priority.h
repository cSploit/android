/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		thd_priority.h
 *	DESCRIPTION:	Thread priorities scheduler
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
 *  Copyright (c) 2002 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 * 2002.10.20 Alexander Peshkoff: Created this scheduler, changing
 *   priorities of Win32 threads. to avoid side effects of Windows
 *   native priorities scheduling.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef JRD_OS_THD_PRIORITY_H
#define JRD_OS_THD_PRIORITY_H

#include "firebird.h"

#ifdef THREAD_PSCHED


#include "../common/classes/alloc.h"
#include "../common/classes/init.h"
#include "../common/classes/array.h"
#include "../jrd/ThreadStart.h"

#include <windows.h>
#include <process.h>

// thps_flags values
const UCHAR THPS_PSCHED		= 1;	// thread controlled by priority scheduler
const UCHAR THPS_UP			= 2;	// candidate for priority boost
const UCHAR THPS_LOW		= 4;	// candidate for priority decrement
const UCHAR THPS_BOOSTED	= 8;	// thread priority raised by priority scheduler

const int highPriority = THREAD_PRIORITY_HIGHEST;
const int lowPriority = THREAD_PRIORITY_NORMAL;

#endif // THREAD_PSCHED

class ThreadPriorityScheduler
{
#ifdef THREAD_PSCHED

private:
	typedef Firebird::SortedArray<ThreadPriorityScheduler*,
		Firebird::InlineStorage<ThreadPriorityScheduler*, 16> > TpsPointers;
	enum OperationMode {Running, Stopping, ShutdownComplete};

	static Firebird::GlobalPtr<Firebird::Mutex> mutex;	// locks modification of thps chains
	static ThreadPriorityScheduler* chain;	// where starts thps chain
	static Firebird::InitMutex<ThreadPriorityScheduler> initialized;
	static OperationMode opMode;	// current mode
	static TpsPointers* toDetach;	// instances to be detached
	static HANDLE masterHandle;		// handle of the master thread
	static bool active;				// may be turned off in firebird.conf

	ThreadEntryPoint* routine;		// real thread entrypoint
	void* arg;						// arg to pass to it
	ThreadPriorityScheduler* next;		// next thread in list
	HANDLE handle;			// thread handle for SetPriority
	// use separate bytes for flags in order to guarantee there
	// modification independence by different threads without
	// affecting each other
	bool inside;			// executing between ENTER and EXIT
	bool gonein;			// pass through ENTER since last scheduling
	UCHAR flags;			// flags that can't be modified concurrently

	// Scheduler Thread
	static unsigned int __stdcall schedulerMain(LPVOID);

	// Get instance for current thread
	static ThreadPriorityScheduler *get();

	// process instances, stored in toDetach array
	static void doDetach();
	// Add current instance to the chain
	void attach();

	~ThreadPriorityScheduler()
	{
		if (active)
		{
			CloseHandle(handle);
		}
	}

public:
	ThreadPriorityScheduler(ThreadEntryPoint* r, void* a, UCHAR f)
		: routine(r), arg(a), inside(false), gonein(false), flags(f)
	{}

	// Unregister thread from priorities scheduler
	void detach();

	// Initialize priority scheduler
	static void init();
	// Stop priority scheduler
	static void cleanup();

	// This CAPITAL_letter-starting methods use InitMutex
	static void Init()
	{
		initialized.init();
	}
	static void Cleanup(void*);

	// Goes to low priority zone
	static void enter();

	// Goes from low priority zone
	static void exit();

	// Check whether current thread has high priority
	static bool boosted()
	{
		return get()->flags & THPS_BOOSTED;
	}

	// Checks, wheather thread's priority 'p'
	// should be altered in scheduler
	static UCHAR adjustPriority(int& p)
	{
		UCHAR flags = 0;
// priority scheduling is done for threads with initially normal priority
		if (active && (p == THREAD_PRIORITY_NORMAL))
		{
			flags = THPS_PSCHED | THPS_BOOSTED;
			p = highPriority;
		}
		return flags;
	}

	// Starts thread under scheduler control
	void run();

#else // THREAD_PSCHED

public:
	static void enter() {}
	static void exit() {}

	static bool boosted()
	{
		return false;
	}

#endif // THREAD_PSCHED
};

#endif // JRD_OS_THD_PRIORITY_H

