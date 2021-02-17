/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		alloc.h
 *	DESCRIPTION:	Memory Pool Manager (based on B+ tree)
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
 *  STL allocator is based on one by Mike Nordell and John Bellardo
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 *
 *  Contributor(s):
 *
 *		Alex Peshkoff <peshkoff@mail.ru>
 *				added PermanentStorage and AutoStorage classes.
 *				merged parts of Nickolay and Jim code to be used together
 *
 */

#ifndef CLASSES_ALLOC_H
#define CLASSES_ALLOC_H

#include "firebird.h"
#include "fb_types.h"
#include "../common/classes/locks.h"
#include "../common/classes/auto.h"
#include "../common/classes/fb_atomic.h"

#include <stdio.h>

#if defined(MVS) || defined (DARWIN)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include <memory.h>

#undef MEM_DEBUG
#ifdef DEBUG_GDS_ALLOC
#define MEM_DEBUG
#endif

#ifdef USE_VALGRIND
// Size of Valgrind red zone applied before and after memory block allocated for user
#define VALGRIND_REDZONE 0 //8
// When memory block is deallocated by user from the pool it must pass queue of this
// length before it is actually deallocated and access protection from it removed.
#define DELAYED_FREE_COUNT 1024
// When memory extent is deallocated when pool is destroying it must pass through
// queue of this length before it is actually returned to system
#define DELAYED_EXTENT_COUNT 32
#undef MEM_DEBUG	// valgrind works instead
#else
#define VALGRIND_REDZONE 8
#endif

#ifdef USE_SYSTEM_NEW
#define OOM_EXCEPTION std::bad_alloc
#else
#define OOM_EXCEPTION Firebird::BadAlloc
#endif


namespace Firebird {

// Alignment for all memory blocks. Sizes of memory blocks in headers are measured in this units
const size_t ALLOC_ALIGNMENT = FB_ALIGNMENT;

static inline size_t MEM_ALIGN(size_t value)
{
	return FB_ALIGN(value, ALLOC_ALIGNMENT);
}

static const unsigned int DEFAULT_ROUNDING = 8;
static const unsigned int DEFAULT_CUTOFF = 4096;
static const size_t DEFAULT_ALLOCATION = 65536;

class MemoryPool;
class MemoryStats
{
public:
	explicit MemoryStats(MemoryStats* parent = NULL)
		: mst_parent(parent), mst_usage(0), mst_mapped(0), mst_max_usage(0), mst_max_mapped(0)
	{}

	~MemoryStats()
	{}

	size_t getCurrentUsage() const throw () { return mst_usage.value(); }
	size_t getMaximumUsage() const throw () { return mst_max_usage; }
	size_t getCurrentMapping() const throw () { return mst_mapped.value(); }
	size_t getMaximumMapping() const throw () { return mst_max_mapped; }

private:
	// Forbid copying/assignment
	MemoryStats(const MemoryStats&);
	MemoryStats& operator=(const MemoryStats&);

	MemoryStats* mst_parent;

	// Currently allocated memory (without allocator overhead)
	// Useful for monitoring engine memory leaks
	AtomicCounter mst_usage;
	// Amount of memory mapped (including all overheads)
	// Useful for monitoring OS memory consumption
	AtomicCounter mst_mapped;

	// We don't particularily care about extreme precision of these max values,
	// this is why we don't synchronize them
	size_t mst_max_usage;
	size_t mst_max_mapped;

	// These methods are thread-safe due to usage of atomic counters only
	void increment_usage(size_t size) throw ()
	{
		for (MemoryStats* statistics = this; statistics; statistics = statistics->mst_parent)
		{
			const size_t temp = statistics->mst_usage.exchangeAdd(size) + size;
			if (temp > statistics->mst_max_usage)
				statistics->mst_max_usage = temp;
		}
	}

	void decrement_usage(size_t size) throw ()
	{
		for (MemoryStats* statistics = this; statistics; statistics = statistics->mst_parent)
		{
			statistics->mst_usage -= size;
		}
	}

	void increment_mapping(size_t size) throw ()
	{
		for (MemoryStats* statistics = this; statistics; statistics = statistics->mst_parent)
		{
			const size_t temp = statistics->mst_mapped.exchangeAdd(size) + size;
			if (temp > statistics->mst_max_mapped)
				statistics->mst_max_mapped = temp;
		}
	}

	void decrement_mapping(size_t size) throw ()
	{
		for (MemoryStats* statistics = this; statistics; statistics = statistics->mst_parent)
		{
			statistics->mst_mapped -= size;
		}
	}

	friend class MemoryPool;
};

typedef SLONG INT32;

class MemBlock;

class MemHeader
{
public:
	union
	{
		MemoryPool*	pool;
		MemBlock*	next;
	};
	size_t		length;
#ifdef DEBUG_GDS_ALLOC
	INT32		lineNumber;
	const char	*fileName;
#endif
#if defined(USE_VALGRIND) && (VALGRIND_REDZONE != 0)
    const char mbk_valgrind_redzone[VALGRIND_REDZONE];
#endif
};

class MemBlock : public MemHeader
{
public:
	UCHAR		body;
};

class MemBigObject;

class MemBigHeader
{
public:
	MemBigObject	*next;
	MemBigObject	*prior;
};

class MemBigObject : public MemBigHeader
{
public:
	MemHeader		memHeader;
};


class MemFreeBlock : public MemBigObject
{
public:
	MemFreeBlock	*nextLarger;
	MemFreeBlock	*priorSmaller;
	MemFreeBlock	*nextTwin;
	MemFreeBlock	*priorTwin;
};


class MemSmallHunk
{
public:
	MemSmallHunk	*nextHunk;
	size_t			length;
	UCHAR			*memory;
	size_t			spaceRemaining;
};

class MemBigHunk
{
public:
	MemBigHunk		*nextHunk;
	size_t			length;
	MemBigHeader	blocks;
};

class MemoryPool
{
private:
	MemoryPool(MemoryPool& parent, MemoryStats& stats,
			   bool shared = true, int rounding = DEFAULT_ROUNDING,
			   int cutoff = DEFAULT_CUTOFF, int minAllocation = DEFAULT_ALLOCATION);
	explicit MemoryPool(bool shared = true, int rounding = DEFAULT_ROUNDING,
			   int cutoff = DEFAULT_CUTOFF, int minAllocation = DEFAULT_ALLOCATION);
	void init(void* memory, size_t length);
	virtual ~MemoryPool(void);

public:
	static MemoryPool* defaultMemoryManager;

private:
	size_t			roundingSize, threshold, minAllocation;
	//int			headerSize;
	typedef			AtomicPointer<MemBlock>	FreeChainPtr;
	FreeChainPtr	*freeObjects;
	MemBigHunk		*bigHunks;
	MemSmallHunk	*smallHunks;
	MemFreeBlock	freeBlocks;
	MemFreeBlock	junk;
	Mutex			mutex;
	int				blocksAllocated;
	int				blocksActive;
	bool			threadShared;		// Shared across threads, requires locking
	bool			pool_destroying;

	// Default statistics group for process
	static MemoryStats* default_stats_group;
	// Statistics group for the pool
	MemoryStats* stats;
	// Parent pool if present
	MemoryPool* parent;
	// Memory used
	AtomicCounter used_memory, mapped_memory;

protected:
	MemBlock* alloc(const size_t length) throw (OOM_EXCEPTION);
	void releaseBlock(MemBlock *block) throw ();

public:
	void* allocate(size_t size
#ifdef DEBUG_GDS_ALLOC
		, const char* fileName = NULL, int line = 0
#endif
	) throw (OOM_EXCEPTION);

protected:
	static void corrupt(const char* text) throw ();

private:
	virtual void memoryIsExhausted(void) throw (OOM_EXCEPTION);
	void remove(MemFreeBlock* block) throw ();
	void insert(MemFreeBlock* block) throw ();
	void* allocRaw(size_t length) throw (OOM_EXCEPTION);
	void validateFreeList(void) throw ();
	void validateBigBlock(MemBigObject* block) throw ();
	static void release(void* block) throw ();
	static void releaseRaw(bool destroying, void *block, size_t size, bool use_cache = true) throw ();

#ifdef USE_VALGRIND
	// Circular FIFO buffer of read/write protected blocks pending free operation
	MemBlock* delayedFree[DELAYED_FREE_COUNT];
	size_t delayedFreeCount;
	size_t delayedFreePos;
#endif

public:
	static void deletePool(MemoryPool* pool);
	static void globalFree(void* block) throw ();
	void* calloc(size_t size
#ifdef DEBUG_GDS_ALLOC
		, const char* fileName, int line
#endif
				) throw (OOM_EXCEPTION);
	static void deallocate(void* block) throw ();
	void validate(void) throw ();

#ifdef LIBC_CALLS_NEW
	static void* globalAlloc(size_t s) throw (OOM_EXCEPTION);
#else
	static void* globalAlloc(size_t s) throw (OOM_EXCEPTION)
	{
		return defaultMemoryManager->allocate(s
#ifdef DEBUG_GDS_ALLOC
				, __FILE__, __LINE__
#endif
									);
	}
#endif // LIBC_CALLS_NEW

	// Create memory pool instance
	static MemoryPool* createPool(MemoryPool* parent = NULL, MemoryStats& stats = *default_stats_group);

	// Set context pool for current thread of execution
	static MemoryPool* setContextPool(MemoryPool* newPool);

	// Get context pool for current thread of execution
	static MemoryPool* getContextPool();

	// Set statistics group for pool. Usage counters will be decremented from
	// previously set group and added to new
	void setStatsGroup(MemoryStats& stats) throw ();

	// Just a helper for AutoPtr.
	static void clear(MemoryPool* pool)
	{
		deletePool(pool);
	}

	// Initialize and finalize global memory pool
	static void init();
	static void cleanup();

	// Statistics
	void increment_usage(size_t size) throw ()
	{
		stats->increment_usage(size);
		used_memory += size;
	}

	void decrement_usage(size_t size) throw ()
	{
		stats->decrement_usage(size);
		used_memory -= size;
	}

	void increment_mapping(size_t size) throw ()
	{
		stats->increment_mapping(size);
		mapped_memory += size;
	}

	void decrement_mapping(size_t size) throw ()
	{
		stats->decrement_mapping(size);
		mapped_memory -= size;
	}

	// Print out pool contents. This is debugging routine
	void print_contents(FILE*, bool = false, const char* filter_path = 0) throw ();
	// The same routine, but more easily callable from the debugger
	void print_contents(const char* filename, bool = false, const char* filter_path = 0) throw ();
};

} // namespace Firebird

static inline Firebird::MemoryPool* getDefaultMemoryPool() throw()
{
	fb_assert(Firebird::MemoryPool::defaultMemoryManager);
	return Firebird::MemoryPool::defaultMemoryManager;
}

namespace Firebird {

// Class intended to manage execution context pool stack
// Declare instance of this class when you need to set new context pool and it
// will be restored automatically as soon holder variable gets out of scope
class ContextPoolHolder
{
public:
	explicit ContextPoolHolder(MemoryPool* newPool)
	{
		savedPool = MemoryPool::setContextPool(newPool);
	}
	~ContextPoolHolder()
	{
		MemoryPool::setContextPool(savedPool);
	}
private:
	MemoryPool* savedPool;
};

// template enabling common use of old and new pools control code
// to be dropped when old-style code goes away
template <typename SubsystemThreadData, typename SubsystemPool>
class SubsystemContextPoolHolder : public ContextPoolHolder
{
public:
	SubsystemContextPoolHolder <SubsystemThreadData, SubsystemPool>
	(
		SubsystemThreadData* subThreadData,
		SubsystemPool* newPool
	)
		: ContextPoolHolder(newPool),
		savedThreadData(subThreadData),
		savedPool(savedThreadData->getDefaultPool())
	{
		savedThreadData->setDefaultPool(newPool);
	}
	~SubsystemContextPoolHolder()
	{
		savedThreadData->setDefaultPool(savedPool);
	}
private:
	SubsystemThreadData* savedThreadData;
	SubsystemPool* savedPool;
};

} // namespace Firebird

using Firebird::MemoryPool;

// Global versions of operators new and delete
inline void* operator new(size_t s) throw (OOM_EXCEPTION)
{
	return MemoryPool::globalAlloc(s);
}
inline void* operator new[](size_t s) throw (OOM_EXCEPTION)
{
	return MemoryPool::globalAlloc(s);
}

inline void operator delete(void* mem) throw()
{
	MemoryPool::globalFree(mem);
}
inline void operator delete[](void* mem) throw()
{
	MemoryPool::globalFree(mem);
}

#ifdef DEBUG_GDS_ALLOC
inline void* operator new(size_t s, Firebird::MemoryPool& pool, const char* file, int line) throw (OOM_EXCEPTION)
{
	return pool.allocate(s, file, line);
}
inline void* operator new[](size_t s, Firebird::MemoryPool& pool, const char* file, int line) throw (OOM_EXCEPTION)
{
	return pool.allocate(s, file, line);
}
#define FB_NEW(pool) new(pool, __FILE__, __LINE__)
#define FB_NEW_RPT(pool, count) new(pool, count, __FILE__, __LINE__)
#else
inline void* operator new(size_t s, Firebird::MemoryPool& pool) throw (OOM_EXCEPTION)
{
	return pool.allocate(s);
}
inline void* operator new[](size_t s, Firebird::MemoryPool& pool) throw (OOM_EXCEPTION)
{
	return pool.allocate(s);
}
#define FB_NEW(pool) new(pool)
#define FB_NEW_RPT(pool, count) new(pool, count)
#endif

#ifndef USE_SYSTEM_NEW
// We must define placement operators NEW & DELETE ourselves
inline void* operator new(size_t s, void* place) throw ()
{
	return place;
}
inline void* operator new[](size_t s, void* place) throw ()
{
	return place;
}
inline void operator delete(void*, void*) throw()
{ }
inline void operator delete[](void*, void*) throw()
{ }
#endif

namespace Firebird
{
	// Global storage makes it possible to use new and delete for classes,
	// based on it, to behave traditionally, i.e. get memory from permanent pool.
	class GlobalStorage
	{
	public:
		void* operator new(size_t size)
		{
			return getDefaultMemoryPool()->allocate(size);
		}

		void operator delete(void* mem)
		{
			getDefaultMemoryPool()->deallocate(mem);
		}

		MemoryPool& getPool() const
		{
			return *getDefaultMemoryPool();
		}
	};


	// Permanent storage is used as base class for all objects,
	// performing memory allocation in methods other than
	// constructors of this objects. Permanent means that pool,
	// which will be later used for such allocations, must
	// be explicitly passed in all constructors of such object.
	class PermanentStorage
	{
	protected:
		explicit PermanentStorage(MemoryPool& p) : pool(p) { }

	public:
		MemoryPool& getPool() const { return pool; }

	private:
		MemoryPool& pool;
	};

	// Automatic storage is used as base class for objects,
	// that may have constructors without explicit MemoryPool
	// parameter. In this case AutoStorage sends AutoMemoryPool
	// to PermanentStorage. To ensure this operation to be safe
	// such trick possible only for local (on stack) variables.
	class AutoStorage : public PermanentStorage
	{
	private:
#if defined(DEV_BUILD)
		void ProbeStack() const;
#endif
	public:
		static MemoryPool& getAutoMemoryPool();
	protected:
		AutoStorage()
			: PermanentStorage(getAutoMemoryPool())
		{
#if defined(DEV_BUILD)
			ProbeStack();
#endif
		}
		explicit AutoStorage(MemoryPool& p) : PermanentStorage(p) { }
	};

	typedef AutoPtr<MemoryPool, MemoryPool> AutoMemoryPool;

} // namespace Firebird


#endif // CLASSES_ALLOC_H
