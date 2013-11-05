/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			fb_tls.h
 *	DESCRIPTION:	Thread-local storage handlers
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

#ifndef CLASSES_FB_TLS_H
#define CLASSES_FB_TLS_H

// This unit defines a few handy macros
// TLS_DECLARE is used in place of variable declaration
// TLS_GET gives value of thread-specific variable
// TLS_SET sets value of thread-specific variable
//
// TLS variable type should be smaller than size of pointer to stay portable

#include "../common/classes/init.h"

#if defined(HAVE___THREAD)
// Recent GCC supports __thread keyword. Sun compiler and HP-UX should have it too
# define TLS_DECLARE(TYPE, NAME) __thread TYPE NAME
# define TLS_GET(NAME) NAME
# define TLS_SET(NAME, VALUE) NAME = (VALUE)
#elif defined(WIN_NT)

namespace Firebird {

template <typename T>
class Win32Tls : private InstanceControl
{
public:
	Win32Tls() : InstanceControl()
	{
		if ((key = TlsAlloc()) == MAX_ULONG)
			system_call_failed::raise("TlsAlloc");
		// Allocated pointer is saved by InstanceList::constructor.
		new InstanceControl::InstanceLink<Win32Tls, PRIORITY_TLS_KEY>(this);
	}
	const T get()
	{
		fb_assert(key != MAX_ULONG);
	 	const LPVOID value = TlsGetValue(key);
		if ((value == NULL) && (GetLastError() != NO_ERROR))
			system_call_failed::raise("TlsGetValue");
		return (T) value;
	}
	void set(const T value)
	{
		fb_assert(key != MAX_ULONG);
		if (TlsSetValue(key, (LPVOID) value) == 0)
			system_call_failed::raise("TlsSetValue");
	}
	~Win32Tls()
	{
	}
	void dtor()
	{
		if (TlsFree(key) == 0)
			system_call_failed::raise("TlsFree");
		key = MAX_ULONG;
	}
private:
	DWORD key;
};
} // namespace Firebird
# define TLS_DECLARE(TYPE, NAME) ::Firebird::Win32Tls<TYPE> NAME
# define TLS_GET(NAME) NAME.get()
# define TLS_SET(NAME, VALUE) NAME.set(VALUE)

// 14-Jul-2004 Nickolay Samofatov.
//
// Unfortunately, compiler-assisted TLS doesn't work with dynamic link libraries
// loaded via LoadLibrary - it intermittently crashes and these crashes are
// documented by MS. We may still use it for server binaries, but it requires
// some changes in build environment. Let's defer this till later point and
// think of reliable mean to prevent linking of DLL with code below (if enabled).
//
//# define TLS_DECLARE(TYPE, NAME) __declspec(thread) TYPE NAME
//# define TLS_GET(NAME) NAME
//# define TLS_SET(NAME, VALUE) NAME = (VALUE)
#else

#include "../common/classes/init.h"

#include <pthread.h>

namespace Firebird {

template <typename T>
class TlsValue : private InstanceControl
{
public:
	TlsValue() : InstanceControl()
#ifdef DEV_BUILD
		, keySet(true)
#endif
	{
		int rc = pthread_key_create(&key, NULL);
		if (rc)
			system_call_failed::raise("pthread_key_create", rc);
		// Allocated pointer is saved by InstanceList::constructor.
		new InstanceControl::InstanceLink<TlsValue, PRIORITY_TLS_KEY>(this);
	}

	const T get()
	{
		fb_assert(keySet);
		// We use double C-style cast to allow using scalar datatypes
		// with sizes up to size of pointer without warnings
		return (T)(IPTR) pthread_getspecific(key);
	}
	void set(const T value)
	{
		fb_assert(keySet);
		int rc = pthread_setspecific(key, (void*)(IPTR) value);
		if (rc)
			system_call_failed::raise("pthread_setspecific", rc);
	}
	~TlsValue()
	{
	}
	void dtor()
	{
		int rc = pthread_key_delete(key);
		if (rc)
			system_call_failed::raise("pthread_key_delete", rc);
#ifdef DEV_BUILD
		keySet = false;
#endif
	}
private:
	pthread_key_t key;
#ifdef DEV_BUILD
	bool keySet;	// This is used to avoid conflicts when destroying global variables
#endif
};

} // namespace Firebird

# define TLS_DECLARE(TYPE, NAME) ::Firebird::TlsValue<TYPE> NAME
# define TLS_GET(NAME) NAME.get()
# define TLS_SET(NAME, VALUE) NAME.set(VALUE)

#endif

#endif // CLASSES_FB_TLS_H
