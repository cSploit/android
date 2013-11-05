/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			fb_atomic.h
 *	DESCRIPTION:	Atomic counters
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
 */

#ifndef CLASSES_FB_ATOMIC_H
#define CLASSES_FB_ATOMIC_H

// Here we define an atomic type, which is to be used in interface for all OS
#if SIZEOF_VOID_P == 8
typedef SINT64 AtomicType;
#else
typedef SLONG AtomicType;
#endif

#if defined(WIN_NT)

#include <windows.h>

namespace Firebird {

// Win95 is not supported unless compiled conditionally and
// redirected to generic version below
class AtomicCounter
{
public:
#ifdef _WIN64
	typedef LONGLONG counter_type;
#else
	typedef LONG counter_type;
#endif

	explicit AtomicCounter(counter_type val = 0) : counter(val) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(counter_type val)
	{
#ifdef _WIN64
		return InterlockedExchangeAdd64(&counter, val);
#else
		return InterlockedExchangeAdd(&counter, val);
#endif
	}

	counter_type operator +=(counter_type val)
	{
		return exchangeAdd(val) + val;
	}

	counter_type operator -=(counter_type val)
	{
		return exchangeAdd(-val) - val;
	}

	counter_type operator ++()
	{
#ifdef _WIN64
		return InterlockedIncrement64(&counter);
#else
		return InterlockedIncrement(&counter);
#endif
	}

	counter_type operator --()
	{
#ifdef _WIN64
		return InterlockedDecrement64(&counter);
#else
		return InterlockedDecrement(&counter);
#endif
	}

	counter_type value() const { return counter; }

	counter_type setValue(counter_type val)
	{
#ifdef _WIN64
		return InterlockedExchange64(&counter, val);
#else
		return InterlockedExchange(&counter, val);
#endif
	}

private:
# if defined(MINGW)
	counter_type counter;
# else
	volatile counter_type counter;
# endif
};

} // namespace Firebird

#elif defined(AIX)

#if SIZEOF_VOID_P != 8
#error: Only 64-bit mode supported on AIX
#endif

#include <sys/atomic_op.h>

namespace Firebird {

// AIX version - uses AIX atomic API
class AtomicCounter
{
private:
	typedef long counter_type;

public:
	explicit AtomicCounter(AtomicType value = 0) : counter(value) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(AtomicType value)
	{
		return fetch_and_addlp(&counter, static_cast<unsigned long>(value));
	}

	counter_type operator +=(AtomicType value)
	{
		return exchangeAdd(value) + value;
	}

	counter_type operator -=(AtomicType value)
	{
		return exchangeAdd(-value) - value;
	}

	counter_type operator ++()
	{
		return exchangeAdd(1) + 1;
	}

	counter_type operator --()
	{
		return exchangeAdd(-1) - 1;
	}

	counter_type value() const { return counter; }

	counter_type setValue(AtomicType val)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!compare_and_swaplp(&counter, &old, val));
		return old;
	}

private:
	counter_type counter;
};

} // namespace Firebird

#elif defined(HPUX) && defined(HAVE_ATOMIC_H)

#include <atomic.h>

namespace Firebird {

// HPUX version - uses atomic API library
class AtomicCounter
{
public:
	typedef uint64_t counter_type;

	explicit AtomicCounter(counter_type value = 0) : counter(value) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(counter_type value)
	{
		counter_type old = counter;
		do
		{
			old = counter;
			errno = 0;
		} while (atomic_cas_64(&counter, old, old + value) != old);
		return old;
	}

	counter_type operator +=(counter_type value)
	{
		return exchangeAdd(value) + value;
	}

	counter_type operator -=(counter_type value)
	{
		return exchangeAdd(-value) - value;
	}

	counter_type operator ++()
	{
		return atomic_inc_64(&counter) + 1;
	}

	counter_type operator --()
	{
		return atomic_dec_64(&counter) - 1;
	}

	counter_type value() const { return counter; }

	counter_type setValue(counter_type val)
	{
		return atomic_swap_64(&counter, val);
	}

private:
	volatile counter_type counter;
};

} // namespace Firebird

#elif defined(SOLARIS) && defined(HAVE_ATOMIC_H)

#include <atomic.h>

namespace Firebird {

// Solaris version - uses Solaris atomic_ops
class AtomicCounter
{
public:
	typedef volatile ulong_t counter_type;
	typedef long delta_type;

	explicit AtomicCounter(counter_type value = 0) : counter(value) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(delta_type value)
	{
		return atomic_add_long_nv(&counter, value);
	}

	counter_type operator +=(delta_type value)
	{
		return exchangeAdd(value) + value;
	}

	counter_type operator -=(delta_type value)
	{
		return exchangeAdd(-value) - value;
	}

	counter_type operator ++()
	{
		return atomic_inc_ulong_nv(&counter);
	}

	counter_type operator --()
	{
		return atomic_dec_ulong_nv(&counter);
	}

	counter_type value() const { return counter; }

	counter_type setValue(delta_type value)
	{
		return atomic_swap_ulong(&counter, value);
	}

private:
	counter_type counter;
};

} // namespace Firebird


#elif defined(__SUNPRO_CC) && !defined(HAVE_ATOMIC_H) && defined(__sparc)
// Solaris 9 does not have nice <atomic.h> header file, so we provide
// an assembly language, Sun Studio Pro only, implementation.

// assembly versions of fetch_and_add_il, compare_and_swap_il,
// are in fb_atomic_*.il

// No GNU CC implementation on Solaris 9 is planned!
extern "C"
{
	extern int fetch_and_add_il(volatile unsigned int* word_addr, int value);
	extern boolean_t compare_and_swap_il(volatile unsigned int* word_addr,
		unsigned int* old_val_addr, unsigned int new_val);
}

namespace Firebird {

class AtomicCounter
{
public:
	typedef uint_t counter_type;
	typedef int delta_type;

	explicit AtomicCounter(counter_type value = 0) : counter(value) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(delta_type value)
	{
		return fetch_and_add_il(&counter, value);
	}

	counter_type operator +=(delta_type value)
	{
		return exchangeAdd(value) + value;
	}

	counter_type operator -=(delta_type value)
	{
		return exchangeAdd(-value) - value;
	}

	counter_type operator ++()
	{
		return exchangeAdd(1) + 1;
	}

	counter_type operator --()
	{
		return exchangeAdd(-1) - 1;
	}

	counter_type value() const { return counter; }

	counter_type setValue(delta_type value)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!compare_and_swap_il(&counter, &old, value));
		return old;
	}

private:
	counter_type counter;
};

} // namespace Firebird

#elif defined(__GNUC__) && (defined(i386) || defined(I386) || defined(_M_IX86) || defined(AMD64) || defined(__x86_64__))

namespace Firebird {

// Assembler version for x86 and AMD64. Note it uses xaddl thus it requires i486
class AtomicCounter
{
public:
	typedef AtomicType counter_type;

	explicit AtomicCounter(AtomicType value = 0) : counter(value) {}
	~AtomicCounter() {}

	AtomicType exchangeAdd(AtomicType value)
	{
		register counter_type result;
		__asm __volatile (
#if SIZEOF_VOID_P == 8
			"lock; xaddq %0, %1"
#else
			"lock; xaddl %0, %1"
#endif
			 : "=r" (result), "=m" (counter)
			 : "0" (value), "m" (counter));
		return result;
	}

	AtomicType operator +=(AtomicType value)
	{
		return exchangeAdd(value) + value;
	}

	AtomicType operator -=(AtomicType value)
	{
		return exchangeAdd(-value) - value;
	}

	AtomicType operator ++()
	{
		return exchangeAdd(1) + 1;
	}

	AtomicType operator --()
	{
		return exchangeAdd(-1) - 1;
	}

	AtomicType value() const { return counter; }

	AtomicType setValue(AtomicType val)
	{
		register counter_type result;
		__asm __volatile (
#if SIZEOF_VOID_P == 8
			"lock; xchgq %0, %1"
#else
			"lock; xchgl %0, %1"
#endif
			 : "=r" (result), "=m" (counter)
			 : "0" (val), "m" (counter));
		return result;
	}

private:
	volatile counter_type counter;
};

} // namespace Firebird

#elif defined(HAVE_AO_COMPARE_AND_SWAP_FULL) && !defined(HPUX)

// Sometimes in the future it can become the best choice for all platforms.
// Currently far not CPUs/OSs/compilers are supported well.
// Therefore use it as last chance to build successfully.

// wbo 2009-10 - We tested libatomic_ops against Firebird 2.5/HPUX and the
// atomic implementation was incomplete on IA-64/HPPA

extern "C" {
#define AO_REQUIRE_CAS
#include <atomic_ops.h>
};

namespace Firebird {

class AtomicCounter
{
public:
	typedef AO_t counter_type;		// AO_t should match maximum native in type

	explicit AtomicCounter(counter_type value = 0) : counter(value) {}
	~AtomicCounter() {}

	/*
	counter_type exchangeAdd(counter_type value)
	{
		return AO_fetch_and_add_full(&counter, value);
	}
	*/
	AtomicType exchangeAdd(AtomicType value)
	{
#ifdef DEV_BUILD
		// check that AO_t size is as we expect (can't fb_assert here)
		if (sizeof(AtomicType) != sizeof(AO_t)) abort();
#endif

		counter_type old;
		do
		{
			old = counter;
		} while (!AO_compare_and_swap_full(&counter, old, old + counter_type(value)));
		return AtomicType(old);
	}

	AtomicType operator +=(AtomicType value)
	{
		return exchangeAdd(value) + value;
	}

	AtomicType operator -=(AtomicType value)
	{
		return exchangeAdd(-value) - value;
	}

	AtomicType operator ++()
	{
		return exchangeAdd(1) + 1;
	}

	AtomicType operator --()
	{
		return exchangeAdd(-1) - 1;
	}

	AtomicType value() const { return counter; }

	AtomicType setValue(AtomicType val)
	{
#ifdef DEV_BUILD
		// check that AO_t size is as we expect (can't fb_assert here)
		if (sizeof(AtomicType) != sizeof(AO_t)) abort();
#endif

		counter_type old;
		do
		{
			old = counter;
		} while (!AO_compare_and_swap_full(&counter, old, counter_type(val)));
		return AtomicType(old);
	}

private:
	counter_type counter;
};

} // namespace Firebird

#elif defined(ARM)

extern "C" {
#include <sys/atomics.h>
};

namespace Firebird {

class AtomicCounter
{
public:
	typedef volatile int counter_type;

	explicit AtomicCounter(counter_type value = 0) : counter(value) {}
	~AtomicCounter() {}

	counter_type exchangeAdd(counter_type value)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (__sync_val_compare_and_swap(&counter, old, old + value)!=old);
		return old;
	}

	counter_type operator +=(counter_type value)
	{
		return exchangeAdd(value) + value;
	}

	counter_type operator -=(counter_type value)
	{
		return exchangeAdd(-value) - value;
	}

	counter_type operator ++()
	{
		//return exchangeAdd(1) + 1;
		return __atomic_inc(&counter);
	}

	counter_type operator --()
	{
		//return exchangeAdd(-1) - 1;
		return __atomic_dec(&counter);
	}

	counter_type value() const { return counter; }

	counter_type setValue(counter_type val)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (__sync_val_compare_and_swap(&counter, old, counter_type(val)) != old);
		return counter_type(old);
	}

private:
	counter_type counter;
};

} // namespace Firebird

#else

#error AtomicCounter: implement appropriate code for your platform!

#endif
#endif // CLASSES_FB_ATOMIC_H
