/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			RefMutex.h
 *	DESCRIPTION:	Reference-counted mutex
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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Vlad Khorsun <hvlad@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  Alex Peshkov <peshkoff@mail.ru>
 *  Dmitry Yemanov <dimitr@users.sf.net>
 */

#ifndef CLASSES_REF_MUTEX_H
#define CLASSES_REF_MUTEX_H

#include "locks.h"
#include "RefCounted.h"

namespace Firebird
{
	// Mutex with reference counting
	//
	// This class is useful if mutex can be "deleted" by the
	// code between enter and leave calls

	class RefMutex : public RefCounted, public Mutex
	{
	public:
		RefMutex() {}
		explicit RefMutex(MemoryPool& pool) : Mutex(pool) {}
	};

	// RAII holder
	class RefMutexGuard : public Reference
	{
	public:
		RefMutexGuard(RefMutex& alock, const char* f)
			: Reference(alock), lock(&alock)
		{
			lock->enter(f);
		}

		~RefMutexGuard()
		{
			lock->leave();
		}

	private:
		// Forbid copying
		RefMutexGuard(const RefMutexGuard&);
		RefMutexGuard& operator=(const RefMutexGuard&);

		RefMutex* lock;
	};


	template <typename T>
	class DefaultRefCounted
	{
	public:
		static int addRef(T* object)
		{
			return object->addRef();
		}

		static int release(T* object)
		{
			return object->release();
		}
	};

	template <typename T>
	class NotRefCounted
	{
	public:
		static int addRef(T*)
		{
			return 0;
		}

		static int release(T*)
		{
			return 0;
		}
	};

	template <typename Mtx, typename RefCounted = DefaultRefCounted<Mtx> >
	class EnsureUnlock
	{
	public:
		EnsureUnlock(Mtx& mutex, const char* f)
			: m_mutex(&mutex), m_locked(0)
#ifdef DEV_BUILD
			  , from(f)
#define FB_LOCKED_FROM from
#else
#define FB_LOCKED_FROM NULL
#endif
		{
			RefCounted::addRef(m_mutex);
		}

		~EnsureUnlock()
		{
			while (m_locked)
				leave();
			RefCounted::release(m_mutex);
		}

		void enter()
		{
			m_mutex->enter(FB_LOCKED_FROM);
			m_locked++;
		}

		bool tryEnter()
		{
			if (m_mutex->tryEnter(FB_LOCKED_FROM))
			{
				m_locked++;
				return true;
			}
			return false;
		}

		void leave()
		{
			m_mutex->leave();
			m_locked--;
		}

	private:
		Mtx* m_mutex;
		int m_locked;
#ifdef DEV_BUILD
		const char* from;
#endif
	};
#undef FB_LOCKED_FROM

	typedef EnsureUnlock<Mutex, NotRefCounted<Mutex> > MutexEnsureUnlock;
	typedef EnsureUnlock<RefMutex> RefMutexEnsureUnlock;

} // namespace

#endif // CLASSES_REF_MUTEX_H
