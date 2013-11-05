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
 *  Contributor(s):
 *
 *		Alex Peshkoff <peshkoff@mail.ru>
 *				added PermanentStorage and AutoStorage classes.
 *
 *
 */

#ifndef CLASSES_ALLOC_H
#define CLASSES_ALLOC_H

#include "firebird.h"
#include "fb_types.h"

#include <stdio.h>
#include "../jrd/common.h"
#include "../common/classes/fb_atomic.h"
#include "../common/classes/auto.h"
#include "../common/classes/tree.h"
#include "../common/classes/locks.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* XPG: prototypes for malloc/free have to be in
					   stdlib.h (EKU) */
#endif

// MSVC does not support exception specification, so it's unknown if that will be correct or not
// from its POV. For now, use it and disable the C4290 warning.
//
//#if defined (_MSC_VER)
//#define THROW_BAD_ALLOC
//#else
#define THROW_BAD_ALLOC throw (std::bad_alloc)
//#endif

#ifdef USE_VALGRIND

// Size of Valgrind red zone applied before and after memory block allocated for user
#define VALGRIND_REDZONE 8

// When memory block is deallocated by user from the pool it must pass queue of this
// length before it is actually deallocated and access protection from it removed.
#define DELAYED_FREE_COUNT 1024

// When memory extent is deallocated when pool is destroying it must pass through
// queue of this length before it is actually returned to system
#define DELAYED_EXTENT_COUNT 32

#endif

namespace Firebird {

// Maximum number of B+ tree pages kept spare for tree allocation
// Since we store only unique fragment lengths in our tree there
// shouldn't be more than 16K elements in it. This is why MAX_TREE_DEPTH
// equal to 4 is more than enough
const int MAX_TREE_DEPTH = 4;

// Alignment for all memory blocks. Sizes of memory blocks in headers are measured in this units
const size_t ALLOC_ALIGNMENT = FB_ALIGNMENT;

static inline size_t MEM_ALIGN(size_t value)
{
	return FB_ALIGN(value, ALLOC_ALIGNMENT);
}

// Flags for memory block
const USHORT MBK_LARGE = 1; // Block is large, allocated from OS directly
const USHORT MBK_PARENT = 2; // Block is allocated from parent pool
const USHORT MBK_USED = 4; // Block is used
const USHORT MBK_LAST = 8; // Block is last in the extent
const USHORT MBK_DELAYED = 16; // Block is pending in the delayed free queue

struct FreeMemoryBlock
{
	FreeMemoryBlock* fbk_next_fragment;
};

// Block header.
// Has size of 12 bytes for 32-bit targets and 16 bytes on 64-bit ones
struct MemoryBlock
{
	USHORT mbk_flags;
	SSHORT mbk_type;
	struct mbk_small_struct
	{
	  // Length and offset are measured in bytes thus memory extent size is limited to 64k
	  // Larger extents are not needed now, but this may be icreased later via using allocation units
	  USHORT mbk_length; // Actual block size: header not included, redirection list is included if applicable
	  USHORT mbk_prev_length;
	};
	union // anonymous union
	{
		ULONG mbk_large_length; // Measured in bytes
		mbk_small_struct mbk_small;
	};
#ifdef DEBUG_GDS_ALLOC
	const char* mbk_file;
	int mbk_line;
#endif
	union
	{
		class MemoryPool* mbk_pool;
		FreeMemoryBlock* mbk_prev_fragment;
	};
#if defined(USE_VALGRIND) && (VALGRIND_REDZONE != 0)
	const char mbk_valgrind_redzone[VALGRIND_REDZONE];
#endif
};


// This structure is appended to the end of block redirected to parent pool or operating system
// It is a doubly-linked list which we are going to use when our pool is going to be deleted
struct MemoryRedirectList
{
	MemoryBlock* mrl_prev;
	MemoryBlock* mrl_next;
};

const SSHORT TYPE_POOL = -1;
const SSHORT TYPE_EXTENT = -2;
const SSHORT TYPE_LEAFPAGE = -3;
const SSHORT TYPE_TREEPAGE = -4;

// We store BlkInfo structures instead of BlkHeader pointers to get benefits from
// processor cache-hit optimizations
struct BlockInfo
{
	size_t bli_length;
	FreeMemoryBlock* bli_fragments;
	inline static const size_t& generate(const void* /*sender*/, const BlockInfo& i)
	{
		return i.bli_length;
	}
};

struct MemoryExtent
{
	MemoryExtent *mxt_next;
	MemoryExtent *mxt_prev;
};

struct PendingFreeBlock
{
	PendingFreeBlock *next;
};

class MemoryStats
{
public:
	explicit MemoryStats(MemoryStats* parent = NULL)
		: mst_parent(parent), mst_usage(0), mst_mapped(0), mst_max_usage(0), mst_max_mapped(0)
	{}

	~MemoryStats()
	{}

	size_t getCurrentUsage() const { return mst_usage.value(); }
	size_t getMaximumUsage() const { return mst_max_usage; }
	size_t getCurrentMapping() const { return mst_mapped.value(); }
	size_t getMaximumMapping() const { return mst_max_mapped; }

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
	// this is why we don't synchronize them on Windows
	size_t mst_max_usage;
	size_t mst_max_mapped;

	friend class MemoryPool;
};


// Memory pool based on B+ tree of free memory blocks

// We are going to have two target architectures:
// 1. Multi-process server with customizable lock manager
// 2. Multi-threaded server with single process (SUPERSERVER)
//
// MemoryPool inheritance looks weird because we cannot use
// any pointers to functions in shared memory. VMT usage in
// MemoryPool and its descendants is prohibited
class MemoryPool
{
private:
	class InternalAllocator
	{
	public:
		void* allocate(size_t size)
		{
			return ((MemoryPool*)this)->tree_alloc(size);
		}
		void deallocate(void* block)
		{
			((MemoryPool*)this)->tree_free(block);
		}
	};
	typedef BePlusTree<BlockInfo, size_t, InternalAllocator, BlockInfo> FreeBlocksTree;

	// We keep most of our structures uninitialized as long we redirect
	// our allocations to parent pool
	bool parent_redirect;

	// B+ tree ordered by length
	FreeBlocksTree freeBlocks;

	MemoryExtent* extents_os;		// Linked list of extents allocated from OS
	MemoryExtent* extents_parent;	// Linked list of extents allocated from parent pool

	Vector<void*, 2> spareLeafs;
	Vector<void*, MAX_TREE_DEPTH + 1> spareNodes;
	bool needSpare;
	PendingFreeBlock *pendingFree;

    // Synchronization of this object is a little bit tricky. Allocations
	// redirected to parent pool are not protected with our mutex and not
	// accounted locally, i.e. redirect_amount and parent_redirected linked list
	// are synchronized with parent pool mutex only. All other pool members are
	// synchronized with this mutex.
	Mutex lock;

	// Current usage counters for pool. Used to move pool to different statistics group
	AtomicCounter used_memory;

	size_t mapped_memory;

	MemoryPool *parent; // Parent pool. Used to redirect small allocations there
	MemoryBlock *parent_redirected, *os_redirected;
	size_t redirect_amount; // Amount of memory redirected to parent
							// It is protected by parent pool mutex along with redirect list
	// Statistics group for the pool
	MemoryStats *stats;

#ifdef USE_VALGRIND
	// Circular FIFO buffer of read/write protected blocks pending free operation
	void* delayedFree[DELAYED_FREE_COUNT];
	int delayedFreeHandles[DELAYED_FREE_COUNT];
	size_t delayedFreeCount;
	size_t delayedFreePos;
#endif

	/* Returns NULL in case it cannot allocate requested chunk */
	static void* external_alloc(size_t &size);

	static void external_free(void* blk, size_t &size, bool pool_destroying, bool use_cache = true);

	void* tree_alloc(size_t size);

	void tree_free(void* block);

	void updateSpare();

	inline void addFreeBlock(MemoryBlock* blk);

	void removeFreeBlock(MemoryBlock* blk);

	void free_blk_extent(MemoryBlock* blk);

	// Allocates small block from this pool. Pool must be locked during call
	void* internal_alloc(size_t size, size_t upper_size, SSHORT type
#ifdef DEBUG_GDS_ALLOC
		, const char* file = NULL, int line = 0
#endif
	);

	// Deallocates small block from this pool. Pool must be locked during this call
	void internal_deallocate(void* block);

	// variable size extents support
	void* getExtent(size_t& size);		// pass desired minimum size, return actual extent size

	// Forbid copy constructor and assignment operator
	MemoryPool(const MemoryPool&);
	MemoryPool& operator=(const MemoryPool&);

	// Used by pools to track memory usage.

	// These 2 methods are thread-safe due to usage of atomic counters only
	inline void increment_usage(size_t size);
	inline void decrement_usage(size_t size);

	inline void increment_mapping(size_t size);
	inline void decrement_mapping(size_t size);

protected:
	// Do not allow to create and destroy pool directly from outside
	MemoryPool(MemoryPool* _parent, MemoryStats &_stats, void* first_extent, void* root_page);

	// This should never be called
	~MemoryPool() {}

public:
	// Default statistics group for process
	static MemoryStats* default_stats_group;

	// Pool created for process
	static MemoryPool* processMemoryPool;

	// Create memory pool instance
	static MemoryPool* createPool(MemoryPool* parent = NULL, MemoryStats& stats = *default_stats_group);

	// Set context pool for current thread of execution
	static MemoryPool* setContextPool(MemoryPool* newPool);

	// Get context pool for current thread of execution
	static MemoryPool* getContextPool();

	// Set statistics group for pool. Usage counters will be decremented from
	// previously set group and added to new
	void setStatsGroup(MemoryStats& stats);

	// Deallocate pool and all its contents
	static void deletePool(MemoryPool* pool);

	// Just a helper for AutoPtr. Does the same as above.
	static void clear(MemoryPool* pool)
	{
		deletePool(pool);
	}

#ifdef POOL_DUMP
	static void printAll();
#endif

	// Allocate memory block. Result is not zero-initialized.
	// It case of problems this method throws Firebird::BadAlloc
	void* allocate(size_t size
#ifdef DEBUG_GDS_ALLOC
		, const char* file = NULL, int line = 0
#endif
	);

	// Allocate memory block. In case of problems this method returns NULL
	void* allocate_nothrow(size_t size, size_t upper_size = 0
#ifdef DEBUG_GDS_ALLOC
		, const char* file = NULL, int line = 0
#endif
	);

	void deallocate(void* block);

	// Check pool for internal consistent. When enabled, call is very expensive
	bool verify_pool(bool fast_checks_only = false);

	// Print out pool contents. This is debugging routine
	void print_contents(FILE*, bool = false, const char* filter_path = 0);

	// The same routine, but more easily callable from the debugger
	void print_contents(const char* filename, bool = false, const char* filter_path = 0);

	// This method is needed when C++ runtime can call
	// redefined by us operator new before initialization of global variables.
#ifdef LIBC_CALLS_NEW
	static void* globalAlloc(size_t s) THROW_BAD_ALLOC;
#else // LIBC_CALLS_NEW
	static void* globalAlloc(size_t s) THROW_BAD_ALLOC
	{
		return processMemoryPool->allocate(s
#ifdef DEBUG_GDS_ALLOC
	  		,__FILE__, __LINE__
#endif
		);
	}
#endif // LIBC_CALLS_NEW

	// Deallocate memory block. Pool is derived from block header
	static void globalFree(void* block)
	{
#ifdef LIBC_CALLS_NEW
		if (!processMemoryPool)
		{
			// the best we can do when invoked after destruction of globals
			return;
		}
#endif // LIBC_CALLS_NEW
	    if (block)
		{
			((MemoryBlock*) ((char*) block - MEM_ALIGN(sizeof(MemoryBlock))))->mbk_pool->deallocate(block);
		}
	}

	// Allocate zero-initialized block of memory
	void* calloc(size_t size
#ifdef DEBUG_GDS_ALLOC
		, const char* file = NULL, int line = 0
#endif
	) {
		void* result = allocate(size
#ifdef DEBUG_GDS_ALLOC
			, file, line
#endif
		);
		memset(result, 0, size);
		return result;
	}

	// Initialize and finalize global memory pool
	static void init();
	static void cleanup();

	friend class InternalAllocator;
};

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

inline static MemoryPool* getDefaultMemoryPool() { return Firebird::MemoryPool::processMemoryPool; }

// Global versions of operators new and delete
inline void* operator new(size_t s) THROW_BAD_ALLOC
{
	return Firebird::MemoryPool::globalAlloc(s);
}
inline void* operator new[](size_t s) THROW_BAD_ALLOC
{
	return Firebird::MemoryPool::globalAlloc(s);
}

inline void operator delete(void* mem) throw()
{
	Firebird::MemoryPool::globalFree(mem);
}
inline void operator delete[](void* mem) throw()
{
	Firebird::MemoryPool::globalFree(mem);
}

#ifdef DEBUG_GDS_ALLOC
inline void* operator new(size_t s, Firebird::MemoryPool& pool, const char* file, int line)
{
	return pool.allocate(s, file, line);
}
inline void* operator new[](size_t s, Firebird::MemoryPool& pool, const char* file, int line)
{
	return pool.allocate(s, file, line);
}
#define FB_NEW(pool) new(pool, __FILE__, __LINE__)
#define FB_NEW_RPT(pool, count) new(pool, count, __FILE__, __LINE__)
#else
inline void* operator new(size_t s, Firebird::MemoryPool& pool)
{
	return pool.allocate(s);
}
inline void* operator new[](size_t s, Firebird::MemoryPool& pool)
{
	return pool.allocate(s);
}
#define FB_NEW(pool) new(pool)
#define FB_NEW_RPT(pool, count) new(pool, count)
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
	private:
		MemoryPool& pool;
	protected:
		explicit PermanentStorage(MemoryPool& p) : pool(p) { }
		MemoryPool& getPool() const { return pool; }
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
