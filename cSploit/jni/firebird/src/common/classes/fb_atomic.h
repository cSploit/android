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

// IMPORTANT !
// Most of the interlocked functions returns "old" value of operand (except of
// InterlockedIncrement and InterlockedDecrement on Windows) and this is correct
// as "old" value impossible to restore from "new" value and operation parameters
// and "new" value could be changed at time when code looks at it.
//   This (returning of original value) is not how operators such as "=", "+=",
// "&=" etc, usually works. Therefore overloaded operators in AtomicCounter class
// are void and all of them have corresponding equivalent functions returning
// "old" value of operand.
//   The only exceptions from this rule is unary increment and decrement (for
// historical reasons). Use it with care ! If one need old value of just
// incremented atomic variable he should use exchangeAdd, not operator++.

#if defined(WIN_NT)

#include <windows.h>
#include <intrin.h>

namespace Firebird {

// Win95 is not supported unless compiled conditionally and
// redirected to generic version below
class PlatformAtomicCounter
{
public:
#ifdef _WIN64
	typedef LONGLONG counter_type;
#else
	typedef LONG counter_type;
#endif

	explicit PlatformAtomicCounter(counter_type val = 0) : counter(val) {}
	~PlatformAtomicCounter() {}

	// returns old value
	counter_type exchangeAdd(counter_type val)
	{
#ifdef _WIN64
		return _InterlockedExchangeAdd64(&counter, val);
#else
		return _InterlockedExchangeAdd(&counter, val);
#endif
	}

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
#ifdef _WIN64
		return (_InterlockedCompareExchange64(&counter, newVal, oldVal) == oldVal);
#else
		return (_InterlockedCompareExchange(&counter, newVal, oldVal) == oldVal);
#endif
	}

	// returns old value
	counter_type setValue(counter_type val)
	{
#ifdef _WIN64
		return _InterlockedExchange64(&counter, val);
#else
		return _InterlockedExchange(&counter, val);
#endif
	}

protected:
#ifndef MINGW
	volatile
#endif
		counter_type counter;
};


class PlatformAtomicPointer
{
public:
	explicit PlatformAtomicPointer(void* val = NULL) : pointer(val) {}
	~PlatformAtomicPointer() {}

	void* platformValue() const { return (void*) pointer; }

	// returns old value
	void* platformSetValue(void* val)
	{
#ifdef _WIN64
		return _InterlockedExchangePointer((volatile PVOID*) &pointer, val);
#else
		//InterlockedExchangePointer((volatile PVOID*)&pointer, val);
	    return (void*) _InterlockedExchange((LONG volatile*) &pointer, (LONG) val);
#endif
	}

	bool platformCompareExchange(void* oldVal, void* newVal)
	{
#ifdef _WIN64
		return (_InterlockedCompareExchangePointer(&pointer, newVal, oldVal) == oldVal);
#else
		//return (InterlockedCompareExchangePointer((PVOID volatile*) &pointer, newVal, oldVal) == oldVal);
		return ((PVOID)(LONG_PTR) _InterlockedCompareExchange((LONG volatile*) &pointer, (LONG) newVal, (LONG) oldVal)) == oldVal;
#endif
	}

private:
	void* volatile pointer;
};

} // namespace Firebird

#elif defined(AIX)

#if SIZEOF_VOID_P != 8
#error: Only 64-bit mode supported on AIX
#endif

#include <sys/atomic_op.h>

namespace Firebird {

// AIX version - uses AIX atomic API
class PlatformAtomicCounter
{
public:
	typedef long counter_type;

	explicit PlatformAtomicCounter(AtomicType value = 0) : counter(value) {}
	~PlatformAtomicCounter() {}

	counter_type exchangeAdd(AtomicType value)
	{
		return fetch_and_addlp(&counter, static_cast<unsigned long>(value));
	}

	counter_type value() const { return counter; }

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		return compare_and_swaplp(&counter, &oldVal, newVal);
	}

	counter_type setValue(AtomicType val)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!compare_and_swaplp(&counter, &old, val));
		return old;
	}

protected:
	counter_type counter;
};

#define ATOMIC_FLUSH_CACHE 1
inline void FlushCache()
{
	asm_sync();
}
inline void WaitForFlushCache()
{
	asm_isync();
}

} // namespace Firebird

#elif defined(HPUX) && defined(HAVE_ATOMIC_H)

#include <atomic.h>

namespace Firebird {

// HPUX version - uses atomic API library
class PlatformAtomicCounter
{
public:
	typedef uint64_t counter_type;

	explicit PlatformAtomicCounter(counter_type value = 0) : counter(value) {}
	~PlatformAtomicCounter() {}

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

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		return atomic_cas_64(&counter, oldVal, newVal) == oldVal;
	}

	counter_type setValue(counter_type val)
	{
		return atomic_swap_64(&counter, val);
	}

protected:
	volatile counter_type counter;
};

} // namespace Firebird

#elif defined(SOLARIS) && defined(HAVE_ATOMIC_H)

#include <atomic.h>

extern "C"
{
extern void membar_flush(void);
extern void membar_wait(void);
}

namespace Firebird {

// Solaris version - uses Solaris atomic_ops
class PlatformAtomicCounter
{
public:
	typedef ulong_t counter_type;
	typedef long delta_type;

	explicit PlatformAtomicCounter(counter_type value = 0) : counter(value) {}
	~PlatformAtomicCounter() {}

	counter_type exchangeAdd(delta_type value)
	{
		return atomic_add_long_nv(&counter, value);
	}

	counter_type setValue(delta_type value)
	{
		return atomic_swap_ulong(&counter, value);
	}

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		TODO: implement method for Solaris
	}

protected:
	volatile counter_type counter;
};

#define ATOMIC_FLUSH_CACHE 1
inline void FlushCache()
{
	membar_flush();
}
inline void WaitForFlushCache()
{
	membar_wait();
}

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

class PlatformAtomicCounter
{
public:
	typedef uint_t counter_type;
	typedef int delta_type;

	explicit PlatformAtomicCounter(counter_type value = 0) : counter(value) {}
	~PlatformAtomicCounter() {}

	counter_type exchangeAdd(delta_type value)
	{
		return fetch_and_add_il(&counter, value);
	}

	counter_type setValue(delta_type value)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!compare_and_swap_il(&counter, &old, value));
		return old;
	}

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		return compare_and_swap_il(&counter, &oldVal, newVal);
	}

protected:
	counter_type counter;
};

} // namespace Firebird

#elif defined(__GNUC__) //&& (defined(i386) || defined(I386) || defined(_M_IX86) || defined(AMD64) || defined(__x86_64__))

namespace Firebird {

class PlatformAtomicCounter
{
public:
	typedef AtomicType counter_type;

	explicit PlatformAtomicCounter(AtomicType value = 0) : counter(value) {}
	~PlatformAtomicCounter() {}

	AtomicType exchangeAdd(AtomicType val)
	{
		return __sync_fetch_and_add(&counter, val);
	}

	AtomicType setValue(AtomicType val)
	{
		// This may require special processing at least on HPPA because:
		// Many targets have only minimal support for such locks, and do not support a full exchange operation.
		// In this case, a target may support reduced functionality here by which the only valid value to store
		// is the immediate constant 1. The exact value actually stored in *ptr is implementation defined.
#ifdef DEFINE_ACCORDING_IMPLEMENTATION_SPECIFIC
		counter_type old;
		do
		{
			old = counter;
		} while (!__sync_bool_compare_and_swap(&counter, old, val));
		return old;
#else
		return __sync_lock_test_and_set(&counter, val);
#endif
	}

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		return __sync_bool_compare_and_swap(&counter, oldVal, newVal);
	}

protected:
	volatile counter_type counter;
};

class PlatformAtomicPointer
{
public:
	explicit PlatformAtomicPointer(void* val = NULL) : pointer(val) {}
	~PlatformAtomicPointer() {}

	void* platformValue() const { return (void*)pointer; }

	void* platformSetValue(void* val)
	{
#ifdef DEFINE_ACCORDING_IMPLEMENTATION_SPECIFIC
		void* old;
		do
		{
			old = pointer;
		} while (!__sync_bool_compare_and_swap(&pointer, old, val));
		return old;
#else
		return __sync_lock_test_and_set(&pointer, val);
#endif
	}

	bool platformCompareExchange(void* oldVal, void* newVal)
	{
		return __sync_bool_compare_and_swap(&pointer, oldVal, newVal);
	}

private:
	void* volatile pointer;
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
}

namespace Firebird {

class PlatformAtomicCounter
{
public:
	typedef AO_t counter_type;		// AO_t should match maximum native 'int' type

	explicit PlatformAtomicCounter(counter_type value = 0)
		: counter(value)
	{
#ifdef DEV_BUILD
		// check that AO_t size is as we expect (can't fb_assert here)

		if (sizeof(AtomicType) != sizeof(AO_t) || sizeof(void*) != sizeof(AO_t))
			abort();
#endif
	}

	~PlatformAtomicCounter()
	{
	}

	AtomicType exchangeAdd(AtomicType value)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!AO_compare_and_swap_full(&counter, old, old + counter_type(value)));
		return AtomicType(old);
	}

	AtomicType setValue(AtomicType val)
	{
		counter_type old;
		do
		{
			old = counter;
		} while (!AO_compare_and_swap_full(&counter, old, counter_type(val)));
		return AtomicType(old);
	}

	bool compareExchange(counter_type oldVal, counter_type newVal)
	{
		return AO_compare_and_swap_full(&counter, oldVal, newVal);
	}

protected:
	counter_type counter;
};

class PlatformAtomicPointer
{
public:
	explicit PlatformAtomicPointer(void* val = NULL)
		: pointer((AO_t) val)
	{
#ifdef DEV_BUILD
		// check that AO_t size is as we expect (can't fb_assert here)
		if (sizeof(void*) != sizeof(AO_t))
			abort();
#endif
	}

	~PlatformAtomicPointer()
	{
	}

	void* platformValue() const
	{
		return (void*) pointer;
	}

	void* platformSetValue(void* val)
	{
		AO_t old;
		do
		{
			old = pointer;
		} while (!AO_compare_and_swap_full(&pointer, old, (AO_T) val));
		return (void*) old;
	}

	bool platformCompareExchange(void* oldVal, void* newVal)
	{
		return AO_compare_and_swap_full(&pointer, (AO_t)oldVal, (AO_t)newVal);
	}

private:
	AO_t pointer;
};


} // namespace Firebird

#else

#error AtomicCounter: implement appropriate code for your platform!

#endif

// platform-independent code

namespace Firebird {

class AtomicCounter : public PlatformAtomicCounter
{
public:
	explicit AtomicCounter(counter_type value = 0) : PlatformAtomicCounter(value) {}
	~AtomicCounter() {}

	counter_type value() const { return counter; }

	// returns old value
	counter_type exchangeBitAnd(counter_type val)
	{
		while (true)
		{
			volatile counter_type oldVal = counter;

			if (compareExchange(oldVal, oldVal & val))
				return oldVal;
		}
	}

	// returns old value
	counter_type exchangeBitOr(counter_type val)
	{
		while (true)
		{
			volatile counter_type oldVal = counter;

			if (compareExchange(oldVal, oldVal | val))
				return oldVal;
		}
	}

	// returns old value
	counter_type exchangeGreater(counter_type val)
	{
		while (true)
		{
			volatile counter_type oldVal = counter;

			if (oldVal >= val)
				return oldVal;

			if (compareExchange(oldVal, val))
				return oldVal;
		}
	}

	operator counter_type () const
	{
		return value();
	}

	void operator =(counter_type val)
	{
		setValue(val);
	}

	// returns new value !
	counter_type operator ++()
	{
		return exchangeAdd(1) + 1;
	}

	// returns new value !
	counter_type operator --()
	{
		return exchangeAdd(-1) - 1;
	}

	void operator +=(counter_type val)
	{
		exchangeAdd(val);
	}

	void operator -=(counter_type val)
	{
		exchangeAdd(-val);
	}

	void operator &=(counter_type val)
	{
		exchangeBitAnd(val);
	}

	void operator |=(counter_type val)
	{
		exchangeBitOr(val);
	}

};

template <typename T>
class AtomicPointer : public PlatformAtomicPointer
{
public:
	explicit AtomicPointer(T* val = NULL) : PlatformAtomicPointer(val) {}
	~AtomicPointer() {}

	T* value() const
	{
		return (T*)platformValue();
	}

	void setValue(T* val)
	{
		platformSetValue(val);
	}

	bool compareExchange(T* oldVal, T* newVal)
	{
		return platformCompareExchange(oldVal, newVal);
	}

	operator T* () const
	{
		return value();
	}

	T* operator ->() const
	{
		return value();
	}

	void operator =(T* val)
	{
		setValue(val);
	}
};

#ifndef ATOMIC_FLUSH_CACHE
inline void FlushCache() { }
inline void WaitForFlushCache() { }
#endif

} // namespace Firebird

#endif // CLASSES_FB_ATOMIC_H
