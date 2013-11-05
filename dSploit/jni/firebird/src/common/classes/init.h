/*
 *	PROGRAM:	Common Access Method
 *	MODULE:		init.h
 *	DESCRIPTION:	InitMutex, InitInstance - templates to help with initialization
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
 *  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_INIT_INSTANCE_H
#define CLASSES_INIT_INSTANCE_H

#include "fb_types.h"
#include "../common/classes/alloc.h"

namespace Firebird {

namespace StaticMutex
{
	// Support for common mutex for various inits
	extern Mutex* mutex;
	void create();
	void release();
}

// InstanceControl - interface for almost all global variables

class InstanceControl
{
public:
	enum DtorPriority
	{
		PRIORITY_REGULAR,
		PRIORITY_TLS_KEY
	};

    InstanceControl();

	//
	// GlobalPtr should not be directly derived from class with virtual functions -
	// virtual table for its instances may become invalid in the moment,
	// when cleanup is needed. Therefore indirect link via InstanceList and
	// InstanceLink is established. This means more calls to memory allocator,
	// but who cares for 100 global variables?
	//

	class InstanceList
	{
	public:
		explicit InstanceList(DtorPriority p);
		virtual ~InstanceList();
		static void destructors();

	private:
		InstanceList* next;
		DtorPriority priority;
		virtual void dtor() = 0;
	};

	template <typename T, InstanceControl::DtorPriority P = InstanceControl::PRIORITY_REGULAR>
	class InstanceLink : private InstanceList, public GlobalStorage
	{
	private:
		T* link;

	public:
		explicit InstanceLink(T* l)
			: InstanceControl::InstanceList(P), link(l)
		{
			fb_assert(link);
		}

		void dtor()
		{
			fb_assert(link);
			if (link)
			{
				link->dtor();
				link = NULL;
			}
		}
	};

public:
	static void destructors();
	static void registerGdsCleanup(FPTR_VOID cleanup);
	static void registerShutdown(FPTR_VOID shutdown);
};


// GlobalPtr - template to help declaring global varables

template <typename T>
class GlobalPtr : private InstanceControl
{
private:
	T* instance;

public:
	void dtor()
	{
		delete instance;
		instance = 0;
	}

	GlobalPtr()
	{
		// This means - for objects with ctors/dtors that want to be global,
		// provide ctor with MemoryPool& parameter. Even if it is ignored!
		instance = FB_NEW(*getDefaultMemoryPool()) T(*getDefaultMemoryPool());
		// Put ourself into linked list for cleanup.
		// Allocated pointer is saved by InstanceList::constructor.
		new InstanceControl::InstanceLink<GlobalPtr>(this);
	}

	T* operator->() throw()
	{
		return instance;
	}
	operator T&() throw()
	{
		return *instance;
	}
	T* operator&() throw()
	{
		return instance;
	}
};

// InitMutex - executes static void C::init() once and only once

template <typename C>
class InitMutex
{
private:
	volatile bool flag;
public:
	InitMutex()
		: flag(false) { }
	void init()
	{
		if (!flag) {
			MutexLockGuard guard(*StaticMutex::mutex);
			if (!flag) {
				C::init();
				flag = true;
			}
		}
	}
	void cleanup()
	{
		if (flag) {
			MutexLockGuard guard(*StaticMutex::mutex);
			if (flag) {
				C::cleanup();
				flag = false;
			}
		}
	}
};

// InitInstance - initialize pointer to class once and only once,
// DefaultInit uses default memory pool for it.

template <typename T>
class DefaultInit
{
public:
	static T* init()
	{
		return FB_NEW(*getDefaultMemoryPool()) T(*getDefaultMemoryPool());
	}
};

template <typename T,
	typename I = DefaultInit<T> >
class InitInstance
{
private:
	T* instance;
	volatile bool flag;
public:
	InitInstance()
		: flag(false) { }
	T& operator()()
	{
		if (!flag) {
			MutexLockGuard guard(*StaticMutex::mutex);
			if (!flag) {
				instance = I::init();
				flag = true;
			}
		}
		return *instance;
	}
};

} //namespace Firebird

#endif // CLASSES_INIT_INSTANCE_H
