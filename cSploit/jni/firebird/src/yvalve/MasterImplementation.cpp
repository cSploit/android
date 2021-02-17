/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			MasterImplementation.cpp
 *	DESCRIPTION:	Main firebird interface, used to get other interfaces.
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include "firebird/Interface.h"
#include "firebird/Timer.h"

#include <string.h>

#include "../yvalve/MasterImplementation.h"
#include "../common/classes/init.h"
#include "../common/MsgMetadata.h"
#include "../common/StatusHolder.h"
#include "../yvalve/PluginManager.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/fb_pair.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/semaphore.h"
#include "../common/isc_proto.h"
#include "../common/ThreadStart.h"
#include "../common/utils_proto.h"
#include "../jrd/ibase.h"
#include "../yvalve/utl_proto.h"

using namespace Firebird;

namespace Why {

//
// getStatus()
//

class UserStatus FB_FINAL : public Firebird::DisposeIface<Firebird::BaseStatus, FB_STATUS_VERSION>
{
private:
	// IStatus implementation
	void FB_CARG dispose()
	{
		delete this;
	}
};

Static<Dtc> MasterImplementation::dtc;

Firebird::IStatus* FB_CARG MasterImplementation::getStatus()
{
	return new UserStatus;
}

//
// getDispatcher()
//

IProvider* FB_CARG MasterImplementation::getDispatcher()
{
	IProvider* dispatcher = new Dispatcher;
	dispatcher->addRef();
	return dispatcher;
}

//
// getDtc()
//

Dtc* FB_CARG MasterImplementation::getDtc()
{
	return &dtc;
}

//
// getPluginManager()
//

IPluginManager* FB_CARG MasterImplementation::getPluginManager()
{
	static Static<PluginManager> manager;

	return &manager;
}

//
// upgradeInterface()
//

namespace
{
	typedef void function();
	typedef function* FunctionPtr;

	struct CVirtualClass
	{
		FunctionPtr* vTab;
	};

	class UpgradeKey
	{
	public:
		UpgradeKey(FunctionPtr* aFunc, IPluginModule* aModuleA, IPluginModule* aModuleB)
			: func(aFunc), moduleA(aModuleA), moduleB(aModuleB)
		{ }

		UpgradeKey(const UpgradeKey& el)
			: func(el.func), moduleA(el.moduleA), moduleB(el.moduleB)
		{ }

		UpgradeKey()
			: func(NULL), moduleA(NULL), moduleB(NULL)
		{ }

		bool operator<(const UpgradeKey& el) const
		{
			return memcmp(this, &el, sizeof(UpgradeKey)) < 0;
		}

		bool operator>(const UpgradeKey& el) const
		{
			return memcmp(this, &el, sizeof(UpgradeKey)) > 0;
		}

		bool contains(IPluginModule* module)
		{
			return moduleA == module || moduleB == module;
		}

	private:
		FunctionPtr* func;
		IPluginModule* moduleA;
		IPluginModule* moduleB;
	};

	typedef Firebird::Pair<Firebird::NonPooled<UpgradeKey, FunctionPtr*> > FunctionPair;
	GlobalPtr<GenericMap<FunctionPair> > functionMap;
	GlobalPtr<RWLock> mapLock;
}

int FB_CARG MasterImplementation::upgradeInterface(IVersioned* toUpgrade,
												   int desiredVersion,
												   struct UpgradeInfo* upgradeInfo)
{
	if (!toUpgrade)
		return 0;

	int existingVersion = toUpgrade->getVersion();

	if (existingVersion >= desiredVersion)
		return 0;

	FunctionPtr* newTab = NULL;
	try
	{
		CVirtualClass* target = (CVirtualClass*) toUpgrade;

		UpgradeKey key(target->vTab, toUpgrade->getModule(), upgradeInfo->clientModule);

		{ // sync scope
			ReadLockGuard sync(mapLock, FB_FUNCTION);
			if (functionMap->get(key, newTab))
			{
				target->vTab = newTab;
				return 0;
			}
		}

		WriteLockGuard sync(mapLock, FB_FUNCTION);

		if (!functionMap->get(key, newTab))
		{
			CVirtualClass* miss = (CVirtualClass*) (upgradeInfo->missingFunctionClass);
			newTab = FB_NEW(*getDefaultMemoryPool()) FunctionPtr[desiredVersion];

			for (int i = 0; i < desiredVersion; ++i)
			{
				newTab[i] = i < existingVersion ? target->vTab[i] : miss->vTab[0];
			}

			functionMap->put(key, newTab);
		}

		target->vTab = newTab;
	}
	catch (const Exception& ex)
	{
		ISC_STATUS_ARRAY s;
		ex.stuff_exception(s);
		iscLogStatus("upgradeInterface", s);
		if (newTab)
		{
			delete[] newTab;
		}
		return -1;
	}

	return 0;
}

void releaseUpgradeTabs(IPluginModule* module)
{
	HalfStaticArray<UpgradeKey, 16> removeList;

	WriteLockGuard sync(mapLock, FB_FUNCTION);

	GenericMap<FunctionPair>::Accessor scan(&functionMap);

	if (scan.getFirst())
	{
		do
		{
			UpgradeKey& cur(scan.current()->first);

			if (cur.contains(module))
				removeList.add(cur);
		} while (scan.getNext());
	}

	for(unsigned int i = 0; i < removeList.getCount(); ++i)
		functionMap->remove(removeList[i]);
}

int FB_CARG MasterImplementation::same(IVersioned* first, IVersioned* second)
{
	CVirtualClass* i1 = (CVirtualClass*) first;
	CVirtualClass* i2 = (CVirtualClass*) second;
	return i1->vTab == i2->vTab ? 1 : 0;
}

IMetadataBuilder* MasterImplementation::getMetadataBuilder(IStatus* status, unsigned fieldCount)
{
	try
	{
		IMetadataBuilder* bld = new MetadataBuilder(fieldCount);
		bld->addRef();
		return bld;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
		return NULL;
	}
}

IDebug* FB_CARG MasterImplementation::getDebug()
{
#ifdef DEV_BUILD
	return getImpDebug();
#else
	return NULL;
#endif
}

int FB_CARG MasterImplementation::serverMode(int mode)
{
	static int currentMode = -1;
	if (mode >= 0)
		currentMode = mode;
	return currentMode;
}

} // namespace Why

//
// circularAlloc()
//

#ifdef WIN_NT
#include <windows.h>
#else
#include "fb_pthread.h"
#include <signal.h>
#include <setjmp.h>
#include "../common/classes/fb_tls.h"
#include "../common/os/SyncSignals.h"
#endif

namespace {

class StringsBuffer
{
private:
	class ThreadBuffer : public Firebird::GlobalStorage
	{
	private:
		const static size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		char* buffer_ptr;
		ThreadId thread;

	public:
		explicit ThreadBuffer(ThreadId thr) : buffer_ptr(buffer), thread(thr) { }

		const char* alloc(const char* string, size_t length)
		{
			// if string is already in our buffer - return it
			// it was already saved in our buffer once
			if (string >= buffer && string < &buffer[BUFFER_SIZE])
				return string;

			// if string too long, truncate it
			if (length > BUFFER_SIZE / 4)
				length = BUFFER_SIZE / 4;

			// If there isn't any more room in the buffer, start at the beginning again
			if (buffer_ptr + length + 1 > buffer + BUFFER_SIZE)
				buffer_ptr = buffer;

			char* new_string = buffer_ptr;
			memcpy(new_string, string, length);
			new_string[length] = 0;
			buffer_ptr += length + 1;

			return new_string;
		}

		bool thisThread(ThreadId currTID)
		{
#ifdef WIN_NT
			if (thread != currTID)
			{
				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, false, thread);
				// commented exit code check - looks like OS does not return handle
				// for already exited thread
				//DWORD exitCode = STILL_ACTIVE;
				if (hThread)
				{
					//GetExitCodeThread(hThread, &exitCode);
					CloseHandle(hThread);
				}

				//if ((!hThread) || (exitCode != STILL_ACTIVE))
				if (!hThread)
				{
					// Thread does not exist any more
					thread = currTID;
				}
			}
#else
			if (thread != currTID)
			{
				sigjmp_buf sigenv;
				if (sigsetjmp(sigenv, 1) == 0)
				{
					Firebird::syncSignalsSet(&sigenv);
					if (pthread_kill(thread, 0) == ESRCH)
					{
						// Thread does not exist any more
						thread = currTID;
					}
				}
				else
				{
					// segfault in pthread_kill() - thread does not exist any more
					thread = currTID;
				}
				Firebird::syncSignalsReset();
			}
#endif

			return thread == currTID;
		}
	};

	typedef Firebird::Array<ThreadBuffer*> ProcessBuffer;

	ProcessBuffer processBuffer;
	Firebird::Mutex mutex;

public:
	explicit StringsBuffer(Firebird::MemoryPool& p)
		: processBuffer(p)
	{
	}

	~StringsBuffer()
	{ }

private:
	size_t position(ThreadId thr)
	{
		// mutex should be locked when this function is called

		for (size_t i = 0; i < processBuffer.getCount(); ++i)
		{
			if (processBuffer[i]->thisThread(thr))
			{
				return i;
			}
		}

		return processBuffer.getCount();
	}

	ThreadBuffer* getThreadBuffer(ThreadId thr)
	{
		Firebird::MutexLockGuard guard(mutex, FB_FUNCTION);

		size_t p = position(thr);
		if (p < processBuffer.getCount())
		{
			return processBuffer[p];
		}

		ThreadBuffer* b = new ThreadBuffer(thr);
		processBuffer.add(b);
		return b;
	}

	void cleanup()
	{
		Firebird::MutexLockGuard guard(mutex, FB_FUNCTION);

		size_t p = position(getThreadId());
		if (p >= processBuffer.getCount())
		{
			return;
		}

		delete processBuffer[p];
		processBuffer.remove(p);
	}

	static void cleanupAllStrings(void* toClean)
	{
		static_cast<StringsBuffer*>(toClean)->cleanup();
	}

public:
	const char* alloc(const char* s, size_t len, ThreadId thr = getThreadId())
	{
		return getThreadBuffer(thr)->alloc(s, len);
	}
};

Firebird::GlobalPtr<StringsBuffer> allStrings;

} // anonymous namespace

namespace Why {

const char* FB_CARG MasterImplementation::circularAlloc(const char* s, size_t len, intptr_t thr)
{
	return allStrings->alloc(s, len, (ThreadId) thr);
}

} // namespace Why


//
// timer
//

namespace Why {

namespace {

// Protects timerQueue array
GlobalPtr<Mutex> timerAccess;
// Protects from races during module unload process
// Should be taken before timerAccess
GlobalPtr<Mutex> timerPause;

GlobalPtr<Semaphore> timerWakeup;
// Should use atomic flag for thread stop to provide correct membar
AtomicCounter stopTimerThread(0);
Thread::Handle timerThreadHandle = 0;

struct TimerEntry
{
	TimerDelay fireTime;
	ITimer* timer;

	static const TimerDelay generate(const TimerEntry& item) { return item.fireTime; }
	static THREAD_ENTRY_DECLARE timeThread(THREAD_ENTRY_PARAM);

	static void init()
	{
		Thread::start(timeThread, 0, 0, &timerThreadHandle);
	}

	static void cleanup();
};

typedef SortedArray<TimerEntry, InlineStorage<TimerEntry, 64>, TimerDelay, TimerEntry> TimerQueue;
GlobalPtr<TimerQueue> timerQueue;

InitMutex<TimerEntry> timerHolder("TimerHolder");

void TimerEntry::cleanup()
{
	{
		MutexLockGuard guard(timerAccess, FB_FUNCTION);

		stopTimerThread.setValue(1);
		timerWakeup->release();
	}
	Thread::waitForCompletion(timerThreadHandle);

	while (timerQueue->hasData())
	{
		ITimer* timer = NULL;
		{
			MutexLockGuard guard(timerAccess, FB_FUNCTION);

			TimerEntry* e = timerQueue->end();
			timer = (--e)->timer;
			timerQueue->remove(e);
		}
		timer->release();
	}
}

TimerDelay curTime()
{
	return 1000000 * fb_utils::query_performance_counter() / fb_utils::query_performance_frequency();
}

TimerEntry* getTimer(ITimer* timer)
{
	fb_assert(timerAccess->locked());

	for (unsigned int i = 0; i < timerQueue->getCount(); ++i)
	{
		TimerEntry& e(timerQueue->operator[](i));
		if (e.timer == timer)
		{
			return &e;
		}
	}

	return NULL;
}

THREAD_ENTRY_DECLARE TimerEntry::timeThread(THREAD_ENTRY_PARAM)
{
	while (stopTimerThread.value() == 0)
	{
		TimerDelay microSeconds = 0;

		{
			MutexLockGuard pauseGuard(timerPause, FB_FUNCTION);
			MutexLockGuard guard(timerAccess, FB_FUNCTION);

			const TimerDelay cur = curTime();

			while (timerQueue->getCount() > 0)
			{
				TimerEntry e(timerQueue->operator[](0));

				if (e.fireTime <= cur)
				{
					timerQueue->remove((size_t) 0);

					// We must leave timerAccess mutex here to avoid deadlocks
					MutexUnlockGuard ug(timerAccess, FB_FUNCTION);

					e.timer->handler();
					e.timer->release();
				}
				else
				{
					microSeconds = e.fireTime - cur;
					break;
				}
			}
		}

		if (microSeconds)
		{
			timerWakeup->tryEnter(0, microSeconds / 1000);
		}
		else
		{
			timerWakeup->enter();
		}
	}

	return 0;
}

} // namespace

class TimerImplementation : public AutoIface<ITimerControl, FB_TIMER_CONTROL_VERSION>
{
public:
	// ITimerControl implementation
	void FB_CARG start(ITimer* timer, TimerDelay microSeconds)
	{
		MutexLockGuard guard(timerAccess, FB_FUNCTION);

		if (stopTimerThread.value() != 0)
		{
			// Ignore an attempt to start timer - anyway thread to make it fire is down

			// We must leave timerAccess mutex here to avoid deadlocks
			MutexUnlockGuard ug(timerAccess, FB_FUNCTION);

			timer->addRef();
			timer->release();
			return;
		}

		timerHolder.init();

		TimerEntry* curTimer = getTimer(timer);
		if (!curTimer)
		{
			TimerEntry newTimer;

			newTimer.timer = timer;
			newTimer.fireTime = curTime() + microSeconds;
			timerQueue->add(newTimer);
			timer->addRef();
		}
		else
		{
			curTimer->fireTime = curTime() + microSeconds;
		}

		timerWakeup->release();
	}

	void FB_CARG stop(ITimer* timer)
	{
		MutexLockGuard guard(timerAccess, FB_FUNCTION);

		TimerEntry* curTimer = getTimer(timer);
		if (curTimer)
		{
			curTimer->timer->release();
			timerQueue->remove(curTimer);
		}
	}
};

ITimerControl* FB_CARG MasterImplementation::getTimerControl()
{
	static Static<TimerImplementation> timer;

	return &timer;
}

void shutdownTimers()
{
	timerHolder.cleanup();
}

Mutex& pauseTimer()
{
	return timerPause;
}

} // namespace Why


//
// Utl (misc calls)
//

namespace Why {

	extern UtlInterface utlInterface;		// Implemented in utl.cpp

	Firebird::IUtl* FB_CARG MasterImplementation::getUtlInterface()
	{
		return &utlInterface;
	}

} // namespace Why


//
// ConfigManager (config info access)
//

namespace Firebird {

	extern IConfigManager* iConfigManager;

} // namespace Firebird

namespace Why {

	Firebird::IConfigManager* FB_CARG MasterImplementation::getConfigManager()
	{
		return Firebird::iConfigManager;
	}

} // namespace Why


//
// get master
//

typedef Firebird::IMaster* IMasterPtr;
IMasterPtr API_ROUTINE fb_get_master_interface()
{
	static Static<Why::MasterImplementation> instance;
	return &instance;
}
