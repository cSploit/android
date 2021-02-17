/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		class_perf.cpp
 *	DESCRIPTION:	Class library performance measurements
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

#include "tree.h"
#include "alloc.h"
//#include "../memory/memory_pool.h"
#include <stdio.h>
#include <time.h>
#include <set>

clock_t t;

void start() {
	t = clock();
}

const int TEST_ITEMS	= 5000000;

void report(int scaleNode, int scaleTree)
{
	clock_t d = clock();
	printf("Add+remove %d elements from tree of scale %d/%d took %d milliseconds. \n",
		TEST_ITEMS,	scaleNode, scaleTree, (int)(d-t)*1000/CLOCKS_PER_SEC);
}

using namespace Firebird;

static void testTree()
{
	printf("Fill array with test data (%d items)...", TEST_ITEMS);
	Vector<int, TEST_ITEMS> *v = new Vector<int, TEST_ITEMS>();
	int n = 0;
	int i;
	for (i = 0; i < TEST_ITEMS; i++) {
		n = n * 45578 - 17651;
		// Fill it with quasi-random values in range 0...TEST_ITEMS-1
		v->add(((i+n) % TEST_ITEMS + TEST_ITEMS)/2);
	}
	printf(" DONE\n");
	MallocAllocator temp;

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 30, 30> tree30(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree30.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree30.locate((*v)[i]))
			tree30.fastRemove();
	}
	report(30, 30);

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 50, 50> tree50(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree50.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree50.locate((*v)[i]))
			tree50.fastRemove();
	}
	report(50, 50);

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 75, 75> tree75(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree75.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree75.locate((*v)[i]))
			tree75.fastRemove();
	}
	report(75, 75);

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 100, 100> tree100(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree100.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree100.locate((*v)[i]))
			tree100.fastRemove();
	}
	report(100, 100);

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 100, 250> tree100_250(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree100_250.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree100_250.locate((*v)[i]))
			tree100_250.fastRemove();
	}
	report(100, 250);

	start();
	BePlusTree<int, int, MallocAllocator, DefaultKeyValue<int>,
		DefaultComparator<int>, 200, 200> tree200(NULL);
	for (i = 0; i < TEST_ITEMS; i++)
		tree200.add((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++) {
		if (tree200.locate((*v)[i]))
			tree200.fastRemove();
	}
	report(250, 250);

	std::set<int> stlTree;
	start();
	for (i = 0; i < TEST_ITEMS; i++)
		stlTree.insert((*v)[i]);
	for (i = 0; i < TEST_ITEMS; i++)
		stlTree.erase((*v)[i]);
	clock_t d = clock();
	printf("Just a reference: add+remove %d elements from STL tree took %d milliseconds. \n",
		TEST_ITEMS,	(int)(d-t)*1000/CLOCKS_PER_SEC);
}


void report()
{
	clock_t d = clock();
	printf("Operation took %d milliseconds.\n", (int)(d-t)*1000/CLOCKS_PER_SEC);
}

const int ALLOC_ITEMS	= 5000000;
const int MAX_ITEM_SIZE = 50;
const int BIG_ITEMS		= ALLOC_ITEMS / 10;
const int BIG_SIZE		= MAX_ITEM_SIZE * 5;

struct AllocItem
{
	int order;
	void *item;
	static bool greaterThan(const AllocItem &i1, const AllocItem &i2)
	{
		return i1.order > i2.order || (i1.order == i2.order && i1.item > i2.item);
	}
};

static void testAllocatorOverhead()
{
	printf("Calculating measurement overhead...\n");
	start();
	MallocAllocator allocator;
	BePlusTree<AllocItem, AllocItem, MallocAllocator, DefaultKeyValue<AllocItem>, AllocItem> items(&allocator),
		bigItems(&allocator);
	// Allocate small items
	int n = 0;
	int i;
	for (i = 0; i < ALLOC_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, (void*)(long)i};
		items.add(temp);
	}
	// Deallocate half of small items
	n = 0;
	if (items.getFirst()) do {
		items.current();
		n++;
	} while (n < ALLOC_ITEMS / 2 && items.getNext());
	// Allocate big items
	for (i = 0; i < BIG_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, (void*)(long)i};
		bigItems.add(temp);
	}
	// Deallocate the rest of small items
	while (items.getNext()) {
		items.current();
	}
	// Deallocate big items
	if (bigItems.getFirst()) do {
		bigItems.current();
	} while (bigItems.getNext());
	report();
}

static void testAllocatorMemoryPool()
{
	printf("Test run for Firebird::MemoryPool...\n");
	start();
	Firebird::MemoryPool* pool = Firebird::MemoryPool::createPool();
	MallocAllocator allocator;
	BePlusTree<AllocItem, AllocItem, MallocAllocator, DefaultKeyValue<AllocItem>, AllocItem> items(&allocator),
		bigItems(&allocator);
	// Allocate small items
	int i, n = 0;
	for (i = 0; i < ALLOC_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, pool->allocate((n % MAX_ITEM_SIZE + MAX_ITEM_SIZE) / 2 + 1)};
		items.add(temp);
	}
	// Deallocate half of small items
	n = 0;
	if (items.getFirst()) do {
		pool->deallocate(items.current().item);
		n++;
	} while (n < ALLOC_ITEMS / 2 && items.getNext());
	// Allocate big items
	for (i = 0; i < BIG_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, pool->allocate((n % BIG_SIZE + BIG_SIZE) / 2 + 1)};
		bigItems.add(temp);
	}
	// Deallocate the rest of small items
	while (items.getNext()) {
		pool->deallocate(items.current().item);
	}
	// Deallocate big items
	if (bigItems.getFirst()) do {
		pool->deallocate(bigItems.current().item);
	} while (bigItems.getNext());
	Firebird::MemoryPool::deletePool(pool);
	report();
}

static void testAllocatorMalloc()
{
	printf("Test reference run for ::malloc...\n");
	start();
	MallocAllocator allocator;
	BePlusTree<AllocItem, AllocItem, MallocAllocator, DefaultKeyValue<AllocItem>, AllocItem> items(&allocator),
		bigItems(&allocator);
	// Allocate small items
	int i, n = 0;
	for (i = 0; i < ALLOC_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, malloc((n % MAX_ITEM_SIZE + MAX_ITEM_SIZE) / 2 + 1)};
		items.add(temp);
	}
	// Deallocate half of small items
	n = 0;
	if (items.getFirst()) do {
		free(items.current().item);
		n++;
	} while (n < ALLOC_ITEMS / 2 && items.getNext());
	// Allocate big items
	for (i = 0; i < BIG_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, malloc((n % BIG_SIZE + BIG_SIZE) / 2 + 1)};
		bigItems.add(temp);
	}
	// Deallocate the rest of small items
	while (items.getNext()) {
		free(items.current().item);
	}
	// Deallocate big items
	if (bigItems.getFirst()) do {
		free(bigItems.current().item);
	} while (bigItems.getNext());
	report();
}

/*static void testAllocatorOldPool()
{
	printf("Test run for old MemoryPool...\n");
	start();
	::MemoryPool *pool = new ::MemoryPool(0, getDefaultMemoryPool());
	MallocAllocator allocator;
	BePlusTree<AllocItem, AllocItem, MallocAllocator, DefaultKeyValue<AllocItem>, AllocItem> items(&allocator),
		bigItems(&allocator);
	// Allocate small items
	int n = 0;
	int i;
	for (i = 0; i < ALLOC_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, pool->allocate((n % MAX_ITEM_SIZE + MAX_ITEM_SIZE) / 2 + 1, 0)};
		items.add(temp);
	}
	// Deallocate half of small items
	n = 0;
	if (items.getFirst()) do {
		pool->deallocate(items.current().item);
		n++;
	} while (n < ALLOC_ITEMS / 2 && items.getNext());
	// Allocate big items
	for (i = 0; i < BIG_ITEMS; i++) {
		n = n * 47163 - 57412;
		AllocItem temp = {n, pool->allocate((n % BIG_SIZE + BIG_SIZE) / 2 + 1, 0)};
		bigItems.add(temp);
	}
	// Deallocate the rest of small items
	while (items.getNext()) {
		pool->deallocate(items.current().item);
	}
	// Deallocate big items
	if (bigItems.getFirst()) do {
		pool->deallocate(bigItems.current().item);
	} while (bigItems.getNext());
	delete pool;
	report();
}*/

int main()
{
	testTree();
	testAllocatorOverhead();
	testAllocatorMemoryPool();
	testAllocatorMalloc();
	// testAllocatorOldPool();
}
