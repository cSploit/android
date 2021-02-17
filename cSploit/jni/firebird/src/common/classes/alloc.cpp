/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		alloc.cpp
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

//  PLEASE, DO NOT CONSTIFY THIS MODULE !!!

#include "firebird.h"
#include "../common/classes/alloc.h"

#ifndef WIN_NT
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "../common/classes/fb_tls.h"
#include "../common/classes/locks.h"
#include "../common/classes/init.h"
#include "../common/classes/vector.h"
#include "gen/iberror.h"

#ifdef USE_VALGRIND
#include <valgrind/memcheck.h>

#ifndef VALGRIND_MAKE_WRITABLE	// Valgrind 3.3
#define VALGRIND_MAKE_WRITABLE	VALGRIND_MAKE_MEM_UNDEFINED
#define VALGRIND_MAKE_NOACCESS	VALGRIND_MAKE_MEM_NOACCESS
#endif

#define VALGRIND_FIX_IT		// overrides suspicious valgrind behavior
#endif	// USE_VALGRIND

namespace {

/*** emergency debugging stuff
static const char* lastFileName;
static int lastLine;
static void* lastBlock;
static void* stopAddress = (void*) 0x2254938;
***/

#ifdef MEM_DEBUG
static const int GUARD_BYTES	= Firebird::ALLOC_ALIGNMENT; // * 2048;
static const UCHAR INIT_BYTE	= 0xCC;
static const UCHAR GUARD_BYTE	= 0xDD;
static const UCHAR DEL_BYTE		= 0xEE;
#else
static const int GUARD_BYTES = 0;
#endif

template <typename T>
T absVal(T n) throw ()
{
	return n < 0 ? -n : n;
}

#ifdef USE_VALGRIND
// Circular FIFO buffer of read/write protected extents pending free operation
// Race protected via cache_mutex.
struct DelayedExtent
{
	void* memory; // Extent pointer
	size_t size;  // Size of extent
	int handle;   // Valgrind handle of protected extent block
};

DelayedExtent delayedExtents[DELAYED_EXTENT_COUNT];
size_t delayedExtentCount = 0;
size_t delayedExtentsPos = 0;
#endif

} // anonymous namespace

namespace Firebird {

// This is required for modules that do not define any GlobalPtr themself
GlobalPtr<Mutex> forceCreationOfDefaultMemoryPool;

MemoryPool*		MemoryPool::defaultMemoryManager = NULL;
MemoryStats*	MemoryPool::default_stats_group = NULL;
Mutex*			cache_mutex = NULL;



namespace {

// We cache this amount of extents to avoid memory mapping overhead
const int MAP_CACHE_SIZE = 16; // == 1 MB

Vector<void*, MAP_CACHE_SIZE> extents_cache;

volatile size_t map_page_size = 0;
int dev_zero_fd = 0;

#if defined(WIN_NT)
size_t get_page_size()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwPageSize;
}
#else
size_t get_page_size()
{
	return sysconf(_SC_PAGESIZE);
}
#endif

inline size_t get_map_page_size()
{
	if (!map_page_size)
	{
		MutexLockGuard guard(*cache_mutex, "get_map_page_size");
		if (!map_page_size)
			map_page_size = get_page_size();
	}
	return map_page_size;
}

}

// Initialize process memory pool (called from InstanceControl).

void MemoryPool::init()
{
	static char mtxBuffer[sizeof(Mutex) + ALLOC_ALIGNMENT];
	cache_mutex = new((void*)(IPTR) MEM_ALIGN((size_t)(IPTR) mtxBuffer)) Mutex;

	static char msBuffer[sizeof(MemoryStats) + ALLOC_ALIGNMENT];
	default_stats_group =
		new((void*)(IPTR) MEM_ALIGN((size_t)(IPTR) msBuffer)) MemoryStats;

	static char mpBuffer[sizeof(MemoryPool) + ALLOC_ALIGNMENT];
	defaultMemoryManager =
		new((void*)(IPTR) MEM_ALIGN((size_t)(IPTR) mpBuffer)) MemoryPool;
}

// Should be last routine, called by InstanceControl,
// being therefore the very last routine in firebird module.

void MemoryPool::cleanup()
{
#ifdef VALGRIND_FIX_IT
	VALGRIND_DISCARD(
		VALGRIND_MAKE_MEM_DEFINED(cache_mutex, sizeof(Mutex)));
	VALGRIND_DISCARD(
		VALGRIND_MAKE_MEM_DEFINED(default_stats_group, sizeof(MemoryStats)));
	VALGRIND_DISCARD(
		VALGRIND_MAKE_MEM_DEFINED(defaultMemoryManager, sizeof(MemoryPool)));
#endif

	if (defaultMemoryManager)
	{
		defaultMemoryManager->~MemoryPool();
		defaultMemoryManager = NULL;
	}

	if (default_stats_group)
	{
		default_stats_group->~MemoryStats();
		default_stats_group = NULL;
	}

	while (extents_cache.getCount())
		releaseRaw(true, extents_cache.pop(), DEFAULT_ALLOCATION, false);

	if (cache_mutex)
	{
		cache_mutex->~Mutex();
		cache_mutex = NULL;
	}
}

MemoryPool::MemoryPool(bool shared, int rounding, int cutoff, int minAlloc)
  :	roundingSize(rounding), threshold(cutoff), minAllocation(minAlloc),
	threadShared(shared), pool_destroying(false), stats(default_stats_group), parent(NULL)
{
	const size_t vecSize = (cutoff + rounding) / rounding;
	init(allocRaw(vecSize * sizeof(void*)), vecSize);
}

MemoryPool::MemoryPool(MemoryPool& p, MemoryStats& s, bool shared, int rounding, int cutoff, int minAlloc)
  :	roundingSize(rounding), threshold(cutoff), minAllocation(minAlloc),
	threadShared(shared), pool_destroying(false), stats(&s), parent(&p)
{
	const size_t vecSize = (cutoff + rounding) / rounding;
	init(parent->allocate(vecSize * sizeof(void*)), vecSize);
}

void MemoryPool::init(void* memory, size_t length)
{
	// hvlad: we not used placement new[] there as :
	// a) by standard placement new[] could add some (unknown!) overhead and use
	// part of allocated memory for own use. For example MSVC reserved first array
	// slot and stored items number in it returning advanced pointer. In our case
	// it results in that freeObjects != memory and AV when freeObjects's memory is
	// deallocated as freeObjects don't points to a parent pool anymore.
	// b) constructor of AtomicPointer does nothing except of zero'ing memory, plain
	// memset will do it much faster. destructor of AtomicPointer is empty and we
	// don't need to call it. This behavior is unlikely to be changed.
	//
	// While we can workaround (a) storing memory to release it correctly later,
	// we can't predict in portable way how much overhead is necessary to allocate
	// memory correctly.

	freeObjects = (FreeChainPtr*) memory;
	memset(freeObjects, 0, length * sizeof(void*));
	bigHunks = NULL;
	smallHunks = NULL;
	freeBlocks.nextLarger = freeBlocks.priorSmaller = &freeBlocks;
	junk.nextLarger = junk.priorSmaller = &junk;
	blocksAllocated = 0;
	blocksActive = 0;

#ifdef USE_VALGRIND
	delayedFreeCount = 0;
	delayedFreePos = 0;

	VALGRIND_CREATE_MEMPOOL(this, VALGRIND_REDZONE, 0);
#endif
}

MemoryPool::~MemoryPool(void)
{
	pool_destroying = true;

	decrement_usage(used_memory.value());
	decrement_mapping(mapped_memory.value());

#ifdef USE_VALGRIND
	VALGRIND_DESTROY_MEMPOOL(this);

	// Do not forget to discard stack traces for delayed free blocks
	for (size_t i = 0; i < delayedFreeCount; i++)
	{
		MemBlock* block = delayedFree[i];
		void* object = &block->body;

		VALGRIND_DISCARD(
            VALGRIND_MAKE_MEM_DEFINED(block, OFFSET(MemBlock*, body)));
		VALGRIND_DISCARD(
            VALGRIND_MAKE_WRITABLE(object, block->length));
	}
#endif

	if (parent)
	{
		MemoryPool::release(freeObjects);
	}
	else
	{
		releaseRaw(pool_destroying, freeObjects, ((threshold + roundingSize) / roundingSize) * sizeof(void*));
	}
	freeObjects = NULL;

	for (MemSmallHunk* hunk; hunk = smallHunks;)
	{
		smallHunks = hunk->nextHunk;
		releaseRaw(pool_destroying, hunk, minAllocation);
	}

	for (MemBigHunk* hunk; hunk = bigHunks;)
	{
		bigHunks = hunk->nextHunk;
		releaseRaw(pool_destroying, hunk, hunk->length);
	}
}

MemoryPool* MemoryPool::createPool(MemoryPool* parentPool, MemoryStats& stats)
{
	if (!parentPool)
	{
		parentPool = getDefaultMemoryPool();
	}
	return FB_NEW(*parentPool) MemoryPool(*parentPool, stats);
}

void MemoryPool::setStatsGroup(MemoryStats& newStats) throw ()
{
	MutexLockGuard guard(mutex, "MemoryPool::setStatsGroup");

	const size_t sav_used_memory = used_memory.value();
	const size_t sav_mapped_memory = mapped_memory.value();

	stats->decrement_mapping(sav_mapped_memory);
	stats->decrement_usage(sav_used_memory);

	this->stats = &newStats;

	stats->increment_mapping(sav_mapped_memory);
	stats->increment_usage(sav_used_memory);
}

MemBlock* MemoryPool::alloc(const size_t length) throw (OOM_EXCEPTION)
{
	MutexLockGuard guard(mutex, "MemoryPool::alloc");

	// If this is a small block, look for it there

	if (length <= threshold)
	{
		unsigned int slot = length / roundingSize;
		MemBlock* block;

		if (threadShared)
		{
			while (block = freeObjects[slot])
			{
				if (freeObjects[slot].compareExchange(block, block->next))
				{
#ifdef MEM_DEBUG
					if (slot != block->length / roundingSize)
						corrupt("length trashed for block in slot");
#endif
					return block;
				}
			}
		}
		else
		{
			block = freeObjects[slot];

			if (block)
			{
				freeObjects[slot] = (MemBlock*) block->pool;

#ifdef MEM_DEBUG
				if (slot != block->length / roundingSize)
					corrupt("length trashed for block in slot");
#endif
				return block;
			}
		}

		// See if some other hunk has unallocated space to use

		MemSmallHunk* hunk;

		for (hunk = smallHunks; hunk; hunk = hunk->nextHunk)
		{
			if (length <= hunk->spaceRemaining)
			{
				MemBlock* block = (MemBlock*) hunk->memory;
				hunk->memory += length;
				hunk->spaceRemaining -= length;
				block->length = length;

				return block;
			}
		}

		// No good so far.  Time for a new hunk

		hunk = (MemSmallHunk*) allocRaw(minAllocation);
		hunk->length = minAllocation - 16;
		hunk->nextHunk = smallHunks;
		smallHunks = hunk;

		size_t l = ROUNDUP(sizeof(MemSmallHunk), sizeof(double));
		block = (MemBlock*) ((UCHAR*) hunk + l);
		hunk->spaceRemaining = minAllocation - length - l;
		hunk->memory = (UCHAR*) block + length;
		block->length = length;

		return block;
	}

	/*
	 *  OK, we've got a "big block" on on hands.  To maximize confusing, the indicated
	 *  length of a free big block is the length of MemHeader plus body, explicitly
	 *  excluding the MemFreeBlock and MemBigHeader fields.

                         [MemHeader::length]

		                <---- MemBlock ---->

		*--------------*----------*---------*
		| MemBigHeader | MemHeader |  Body  |
		*--------------*----------*---------*

		 <---- MemBigObject ----->

		*--------------*----------*---------------*
		| MemBigHeader | MemHeader | MemFreeBlock |
		*--------------*----------*---------------*

		 <--------------- MemFreeBlock ---------->
	 */


	MemFreeBlock* freeBlock;

	for (freeBlock = freeBlocks.nextLarger; freeBlock != &freeBlocks; freeBlock = freeBlock->nextLarger)
	{
		if (freeBlock->memHeader.length >= length)
		{
			remove(freeBlock);
			MemBlock* block = (MemBlock*) &freeBlock->memHeader;

			// Compute length (MemHeader + body) for new free block

			unsigned int tail = block->length - length;

			// If there isn't room to split off a new free block, allocate the whole thing

			if (tail < sizeof(MemFreeBlock))
			{
				block->pool = this;
				return block;
			}

			// Otherwise, chop up the block

			MemBigObject* newBlock = freeBlock;
			freeBlock = (MemFreeBlock*) ((UCHAR*) block + length);
			freeBlock->memHeader.length = tail - sizeof(MemBigObject);
			block->length = length;
			block->pool = this;

			if (freeBlock->next = newBlock->next)
				freeBlock->next->prior = freeBlock;

			newBlock->next = freeBlock;
			freeBlock->prior = newBlock;
			freeBlock->memHeader.pool = NULL;		// indicate block is free
			insert(freeBlock);

			return block;
		}
	}

	// Didn't find existing space -- allocate new hunk

	size_t hunkLength = sizeof(MemBigHunk) + sizeof(MemBigHeader) + length;
	size_t freeSpace = 0;

	// If the hunk size is sufficient below minAllocation, allocate extra space

	if (hunkLength + sizeof(MemBigObject) + threshold < minAllocation)
	{
		hunkLength = minAllocation;
		//freeSpace = hunkLength - 2 * sizeof(MemBigObject) - length;
		freeSpace = hunkLength - sizeof(MemBigHunk) - 2 * sizeof(MemBigHeader) - length;
	}

	// Allocate the new hunk

	MemBigHunk* hunk = (MemBigHunk*) allocRaw(hunkLength);
	hunk->nextHunk = bigHunks;
	bigHunks = hunk;
	hunk->length = hunkLength;

	// Create the new block

	MemBigObject* newBlock = (MemBigObject*) &hunk->blocks;
	newBlock->prior = NULL;
	newBlock->next = NULL;

	MemBlock* block = (MemBlock*) &newBlock->memHeader;
	block->pool = this;
	block->length = length;

	// If there is space left over, create a free block

	if (freeSpace)
	{
		freeBlock = (MemFreeBlock*) ((UCHAR*) block + length);
		freeBlock->memHeader.length = freeSpace;
		freeBlock->memHeader.pool = NULL;
		freeBlock->next = NULL;
		freeBlock->prior = newBlock;
		newBlock->next = freeBlock;
		insert(freeBlock);
	}

	return block;
}

void* MemoryPool::allocate(size_t size
#ifdef DEBUG_GDS_ALLOC
	, const char* fileName, int line
#endif
) throw (OOM_EXCEPTION)
{
	size_t length = ROUNDUP(size + VALGRIND_REDZONE, roundingSize) + OFFSET(MemBlock*, body) + GUARD_BYTES;
	MemBlock* memory = alloc(length);

#ifdef USE_VALGRIND
	VALGRIND_MEMPOOL_ALLOC(this, &memory->body, size);
#endif

	memory->pool = this;

#ifdef DEBUG_GDS_ALLOC
	memory->fileName = fileName;
	memory->lineNumber = line;
#endif
#ifdef MEM_DEBUG
	memset(&memory->body, INIT_BYTE, size);
	memset(&memory->body + size, GUARD_BYTE, memory->length - size - OFFSET(MemBlock*,body));
#endif

	++blocksAllocated;
	++blocksActive;

	increment_usage(memory->length);

	return &memory->body;
}


void MemoryPool::release(void* object) throw ()
{
	if (object)
	{
		MemBlock* block = (MemBlock*) ((UCHAR*) object - OFFSET(MemBlock*, body));
		MemoryPool* pool = block->pool;

#ifdef USE_VALGRIND
		// Synchronize delayed free queue using pool mutex
		MutexLockGuard guard(pool->mutex, "MemoryPool::deallocate USE_VALGRIND");

		// Notify Valgrind that block is freed from the pool
		VALGRIND_MEMPOOL_FREE(pool, object);

		// block is placed in delayed buffer - mark as NOACCESS for that time
		VALGRIND_DISCARD(VALGRIND_MAKE_NOACCESS(block, OFFSET(MemBlock*, body)));

		// Extend circular buffer if possible
		if (pool->delayedFreeCount < FB_NELEM(pool->delayedFree))
		{
			pool->delayedFree[pool->delayedFreeCount] = block;
			pool->delayedFreeCount++;
			return;
		}

		// Shift circular buffer pushing out oldest item
		MemBlock* requested_block = block;

		block = pool->delayedFree[pool->delayedFreePos];
		object = &block->body;

		// Re-enable access to MemBlock
		VALGRIND_DISCARD(VALGRIND_MAKE_MEM_DEFINED(block, OFFSET(MemBlock*, body)));

		// Remove protection from memory block
#ifdef VALGRIND_FIX_IT
		VALGRIND_DISCARD(
			VALGRIND_MAKE_MEM_DEFINED(object, block->length - VALGRIND_REDZONE));
#else
		VALGRIND_DISCARD(
			VALGRIND_MAKE_WRITABLE(object, block->length - VALGRIND_REDZONE));
#endif

		// Replace element in circular buffer
		pool->delayedFree[pool->delayedFreePos] = requested_block;

		// Move queue pointer to next element and cycle if needed
		if (++(pool->delayedFreePos) >= FB_NELEM(pool->delayedFree))
			pool->delayedFreePos = 0;
#endif

		size_t size = block->length;
#ifdef DEBUG_GDS_ALLOC
		block->fileName = NULL;
#endif
		pool->releaseBlock(block);
		pool->decrement_usage(size);
	}
}

void MemoryPool::releaseBlock(MemBlock* block) throw ()
{
	if (!freeObjects)
		return;

	if (block->pool != this)
		corrupt("bad block released");

#ifdef MEM_DEBUG
	for (const UCHAR* end = (UCHAR*) block + block->length, *p = end - GUARD_BYTES; p < end;)
	{
		if (*p++ != GUARD_BYTE)
			corrupt("guard bytes overwritten");
	}
#endif

	--blocksActive;
	const size_t length = block->length;

	// If length is less than threshold, this is a small block

	if (length <= threshold)
	{
#ifdef MEM_DEBUG
		memset(&block->body, DEL_BYTE, length - OFFSET(MemBlock*, body));
#endif

		if (threadShared)
		{
			for (int slot = length / roundingSize;;)
			{
				block->next = freeObjects[slot];

				if (freeObjects[slot].compareExchange(block->next, block))
					return;
			}
		}

		int slot = length / roundingSize;
		void* next = freeObjects[slot];
		block->pool = (MemoryPool*) next;
		freeObjects[slot] = block;

		return;
	}

	// OK, this is a large block.  Try recombining with neighbors

	MutexLockGuard guard(mutex, "MemoryPool::release");

#ifdef MEM_DEBUG
	memset(&block->body, DEL_BYTE, length - OFFSET(MemBlock*, body));
#endif

	MemFreeBlock* freeBlock = (MemFreeBlock*) ((UCHAR*) block - sizeof(MemBigHeader));
	block->pool = NULL;

	if (freeBlock->next && !freeBlock->next->memHeader.pool)
	{
		MemFreeBlock* next = (MemFreeBlock*) freeBlock->next;
		remove(next);
		freeBlock->memHeader.length += next->memHeader.length + sizeof(MemBigHeader);

		if (freeBlock->next = next->next)
			freeBlock->next->prior = freeBlock;
	}

	if (freeBlock->prior && !freeBlock->prior->memHeader.pool)
	{
		MemFreeBlock* prior = (MemFreeBlock*) freeBlock->prior;
		remove(prior);
		prior->memHeader.length += freeBlock->memHeader.length + sizeof(MemBigHeader);

		if (prior->next = freeBlock->next)
			prior->next->prior = prior;

		freeBlock = prior;
	}

	// If the block has no neighbors, the entire hunk is empty and can be unlinked and
	// released

	if (freeBlock->prior == NULL && freeBlock->next == NULL)
	{
		for (MemBigHunk** ptr = &bigHunks, *hunk; hunk = *ptr; ptr = &hunk->nextHunk)
		{
			if (&hunk->blocks == freeBlock)
			{
				*ptr = hunk->nextHunk;
				decrement_mapping(hunk->length);
				releaseRaw(pool_destroying, hunk, hunk->length);
				return;
			}
		}

		corrupt("can't find big hunk");
	}

	insert(freeBlock);
}

void MemoryPool::corrupt(const char* text) throw ()
{
#ifdef DEV_BUILD
	fprintf(stderr, "%s\n", text);
	abort();
#endif
}

void MemoryPool::memoryIsExhausted(void) throw (OOM_EXCEPTION)
{
	Firebird::BadAlloc::raise();
}

void MemoryPool::remove(MemFreeBlock* block) throw ()
{
	// If this is junk, chop it out and be done with it

	if (block->memHeader.length < threshold)
		return;

	// If we're a twin, take out of the twin list

	if (!block->nextLarger)
	{
		block->nextTwin->priorTwin = block->priorTwin;
		block->priorTwin->nextTwin = block->nextTwin;
		return;
	}

	// We're in the primary list.  If we have twin, move him in

	MemFreeBlock* twin = block->nextTwin;

	if (twin != block)
	{
		block->priorTwin->nextTwin = twin;
		twin->priorTwin = block->priorTwin;
		twin->priorSmaller = block->priorSmaller;
		twin->nextLarger = block->nextLarger;
		twin->priorSmaller->nextLarger = twin;
		twin->nextLarger->priorSmaller = twin;
		return;
	}

	// No twins.  Just take the guy out of the list

	block->priorSmaller->nextLarger = block->nextLarger;
	block->nextLarger->priorSmaller = block->priorSmaller;
}

void MemoryPool::insert(MemFreeBlock* freeBlock) throw ()
{
	// If this is junk (too small for pool), stick it in junk

	if (freeBlock->memHeader.length < threshold)
		return;

	// Start by finding insertion point

	MemFreeBlock* block;

	for (block = freeBlocks.nextLarger;
		 block != &freeBlocks && freeBlock->memHeader.length >= block->memHeader.length;
		 block = block->nextLarger)
	{
		if (block->memHeader.length == freeBlock->memHeader.length)
		{
			// If this is a "twin" (same size block), hang off block
			freeBlock->nextTwin = block->nextTwin;
			freeBlock->nextTwin->priorTwin = freeBlock;
			freeBlock->priorTwin = block;
			block->nextTwin = freeBlock;
			freeBlock->nextLarger = NULL;
			return;
		}
	}

	// OK, then, link in after insertion point

	freeBlock->nextLarger = block;
	freeBlock->priorSmaller = block->priorSmaller;
	block->priorSmaller->nextLarger = freeBlock;
	block->priorSmaller = freeBlock;

	freeBlock->nextTwin = freeBlock->priorTwin = freeBlock;
}


void* MemoryPool::allocRaw(size_t size) throw (OOM_EXCEPTION)
{
#ifndef USE_VALGRIND
	if (size == DEFAULT_ALLOCATION)
	{
		MutexLockGuard guard(*cache_mutex, "MemoryPool::allocRaw");
		if (extents_cache.hasData())
		{
			// Use most recently used object to encourage caching
			increment_mapping(size);
			return extents_cache.pop();
		}
	}
#endif

	size = FB_ALIGN(size, get_map_page_size());

#ifdef WIN_NT

	void* result = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	if (!result)

#else // WIN_NT

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifdef MAP_ANONYMOUS

	void* result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

#else // MAP_ANONYMOUS

	if (dev_zero_fd < 0)
		dev_zero_fd = open("/dev/zero", O_RDWR);
	void* result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, dev_zero_fd, 0);

#endif // MAP_ANONYMOUS

	if (result == MAP_FAILED)

#endif // WIN_NT

	{
		// failure happens!
		memoryIsExhausted();
		return NULL;
	}

#ifdef USE_VALGRIND
	// Let Valgrind forget that block was zero-initialized
	VALGRIND_DISCARD(VALGRIND_MAKE_WRITABLE(result, size));
#endif

	increment_mapping(size);
	return result;
}

void MemoryPool::validateFreeList(void) throw ()
{
	size_t len = 0;
	int count = 0;
	MemFreeBlock* block;

	for (block = freeBlocks.nextLarger; block != &freeBlocks;  block = block->nextLarger)
	{
		if (block->memHeader.length <= len)
			corrupt("bad free list\n");
		len = block->memHeader.length;
		++count;
	}

	len += 1;

	for (block = freeBlocks.priorSmaller; block != &freeBlocks; block = block->priorSmaller)
	{
		if (block->memHeader.length >= len)
			corrupt("bad free list\n");
		len = block->memHeader.length;
	}
}

void MemoryPool::validateBigBlock(MemBigObject* block) throw ()
{
	MemBigObject* neighbor;

	if (neighbor = block->prior)
	{
		if ((UCHAR*) &neighbor->memHeader + neighbor->memHeader.length != (UCHAR*) block)
			corrupt("bad neighbors");
	}

	if (neighbor = block->next)
	{
		if ((UCHAR*) &block->memHeader + block->memHeader.length != (UCHAR*) neighbor)
			corrupt("bad neighbors");
	}
}

void MemoryPool::releaseRaw(bool destroying, void* block, size_t size, bool use_cache) throw ()
{
#ifndef USE_VALGRIND
	if (use_cache && (size == DEFAULT_ALLOCATION))
	{
		MutexLockGuard guard(*cache_mutex, "MemoryPool::releaseRaw");
		if (extents_cache.getCount() < extents_cache.getCapacity())
		{
			extents_cache.push(block);
			return;
		}
	}
#else
	// Set access protection for block to prevent memory from deleted pool being accessed
	int handle = /* //VALGRIND_MAKE_NOACCESS */ VALGRIND_MAKE_MEM_DEFINED(block, size);

	size = FB_ALIGN(size, get_map_page_size());

	void* unmapBlockPtr = block;
	size_t unmapBlockSize = size;

	// Employ extents delayed free logic only when pool is destroying.
	// In normal case all blocks pass through queue of sufficent length by themselves
	if (destroying)
	{
		// Synchronize delayed free queue using extents mutex
		MutexLockGuard guard(*cache_mutex, "MemoryPool::releaseRaw");

		// Extend circular buffer if possible
		if (delayedExtentCount < FB_NELEM(delayedExtents))
		{
			DelayedExtent* item = &delayedExtents[delayedExtentCount];
			item->memory = block;
			item->size = size;
			item->handle = handle;
			delayedExtentCount++;
			return;
		}

		DelayedExtent* item = &delayedExtents[delayedExtentsPos];

		// Free message associated with old extent in Valgrind
		VALGRIND_DISCARD(item->handle);

		// Set up the block we are going to unmap
		unmapBlockPtr = item->memory;
		unmapBlockSize = item->size;

		// Replace element in circular buffer
		item->memory = block;
		item->handle = handle;
		item->size = size;

		// Move queue pointer to next element and cycle if needed
		delayedExtentsPos++;
		if (delayedExtentsPos >= FB_NELEM(delayedExtents))
			delayedExtentsPos = 0;
	}
	else
	{
		// Let Valgrind forget about unmapped block
		VALGRIND_DISCARD(handle);
	}
#endif

	size = FB_ALIGN(size, get_map_page_size());
#ifdef WIN_NT
	if (!VirtualFree(block, 0, MEM_RELEASE))
#else // WIN_NT
#if (defined SOLARIS) && (defined HAVE_CADDR_T)
	if (munmap((caddr_t) block, size))
#else
	if (munmap(block, size))
#endif
#endif // WIN_NT
		corrupt("OS memory deallocation error");
}

void MemoryPool::globalFree(void* block) throw ()
{
	deallocate(block);
}

void* MemoryPool::calloc(size_t size
#ifdef DEBUG_GDS_ALLOC
	, const char* fileName, int line
#endif
) throw (OOM_EXCEPTION)
{
	void* block = allocate((int) size
#ifdef DEBUG_GDS_ALLOC
					 , fileName, line
#endif
									 );
	memset(block, 0, size);

	return block;
}

void MemoryPool::deallocate(void* block) throw ()
{
	release(block);
}

void MemoryPool::deletePool(MemoryPool* pool)
{
	delete pool;
}

void MemoryPool::validate(void) throw ()
{
	unsigned int slot = 3;

	for (const MemBlock* block = freeObjects [slot]; block; block = (MemBlock*) block->pool)
	{
		if (slot != block->length / roundingSize)
			corrupt("length trashed for block in slot");
	}
}

void MemoryPool::print_contents(const char* filename, bool used_only, const char* filter_path) throw ()
{
	FILE* out = fopen(filename, "w");
	if (!out)
		return;

	print_contents(out, used_only, filter_path);
	fclose(out);
}

#ifdef MEM_DEBUG
static void print_block(bool used, FILE* file, MemHeader* blk, bool used_only,
	const char* filter_path, const size_t filter_len) throw ()
{
	if (used || !used_only)
	{
		bool filter = filter_path != NULL;

		if (used && filter && blk->fileName)
			filter = strncmp(filter_path, blk->fileName, filter_len) != 0;

		if (!filter)
		{
			if (used)
			{
				fprintf(file, "USED %p: size=%" SIZEFORMAT " allocated at %s:%d\n",
					blk, blk->length, blk->fileName, blk->lineNumber);
			}
			else
				fprintf(file, "FREE %p: size=%" SIZEFORMAT "\n", blk, blk->length);
		}
	}
}
#endif

// This member function can't be const because there are calls to the mutex.
void MemoryPool::print_contents(FILE* file, bool used_only, const char* filter_path) throw ()
{
#ifdef MEM_DEBUG
	MutexLockGuard guard(mutex, "MemoryPool::print_contents");

	fprintf(file, "********* Printing contents of pool %p used=%ld mapped=%ld\n",
		this, (long) used_memory.value(), (long) mapped_memory.value());

	if (!used_only)
	{
		filter_path = NULL;
	}
	const size_t filter_len = filter_path ? strlen(filter_path) : 0;

	// Print small hunks
	for (MemSmallHunk* hunk = smallHunks; hunk; hunk = hunk->nextHunk)
	{
		int l = ROUNDUP(sizeof(MemSmallHunk), sizeof(double));
		UCHAR* ptr = ((UCHAR*) hunk) + l;
		size_t used = hunk->length - hunk->spaceRemaining;
		fprintf(file, "\nSmall hunk %p size=%ld used=%ld remain=%ld\n",
				hunk, hunk->length, used, hunk->spaceRemaining);
		while (ptr < hunk->memory)
		{
			MemHeader* m = (MemHeader*) ptr;
			print_block(m->fileName != NULL, file, m, used_only, filter_path, filter_len);
			ptr += m->length;
		}
	}

	// Print big hunks
	for (MemBigHunk* hunk = bigHunks; hunk; hunk = hunk->nextHunk)
	{
		fprintf(file, "\nBig hunk %p size=%ld\n", hunk, hunk->length);
		for (MemBigObject* block = (MemBigObject*) &hunk->blocks; block; block = block->next)
		{
			print_block(block->memHeader.pool != NULL, file, &block->memHeader, used_only, filter_path, filter_len);
		}
	}
#endif
}

// Declare thread-specific variable for context memory pool
TLS_DECLARE(MemoryPool*, contextPool);

MemoryPool* MemoryPool::setContextPool(MemoryPool* newPool)
{
	MemoryPool* const old = TLS_GET(contextPool);
	TLS_SET(contextPool, newPool);
	return old;
}

MemoryPool* MemoryPool::getContextPool()
{
	return TLS_GET(contextPool);
}

MemoryPool& AutoStorage::getAutoMemoryPool()
{
	MemoryPool* p = MemoryPool::getContextPool();
	if (!p)
	{
		p = getDefaultMemoryPool();
		fb_assert(p);
	}

	return *p;
}

#ifdef LIBC_CALLS_NEW
void* MemoryPool::globalAlloc(size_t s) THROW_BAD_ALLOC
{
	if (!processMemoryPool)
	{
		// this will do all required init, including processMemoryPool creation
		static Firebird::InstanceControl dummy;
		fb_assert(processMemoryPool);
	}

	return processMemoryPool->allocate(s
#ifdef DEBUG_GDS_ALLOC
			,__FILE__, __LINE__
#endif
	);
}
#endif // LIBC_CALLS_NEW

#if defined(DEV_BUILD)
void AutoStorage::ProbeStack() const
{
	//
	// AutoStorage() default constructor can be used only
	// for objects on the stack. ProbeStack() uses the
	// following assumptions to check it:
	//	1. One and only one stack is used for all kind of variables.
	//	2. Objects don't grow > 128K.
	//
	char probeVar = '\0';
	const char* myStack = &probeVar;
	const char* thisLocation = (const char*) this;
	ptrdiff_t distance = thisLocation - myStack;
	fb_assert(absVal(distance) < 128 * 1024);
}
#endif

} // namespace Firebird
