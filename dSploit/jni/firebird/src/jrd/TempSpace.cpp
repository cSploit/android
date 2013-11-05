/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"

#include "iberror.h"
#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../jrd/gdsassert.h"
#include "../jrd/gds_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/path_utils.h"

#include "../jrd/TempSpace.h"

using Firebird::TempFile;

// Static definitions/initializations

const size_t MIN_TEMP_BLOCK_SIZE	= 64 * 1024;

Firebird::GlobalPtr<Firebird::Mutex> TempSpace::initMutex;
Firebird::TempDirectoryList* TempSpace::tempDirs = NULL;
size_t TempSpace::minBlockSize = 0;
offset_t TempSpace::globalCacheUsage = 0;

//
// In-memory block class
//

size_t TempSpace::MemoryBlock::read(offset_t offset, void* buffer, size_t length)
{
	if (offset + length > size)
	{
		length = size - offset;
	}
	memcpy(buffer, ptr + offset, length);
	return length;
}

size_t TempSpace::MemoryBlock::write(offset_t offset, const void* buffer, size_t length)
{
	if (offset + length > size)
	{
		length = size - offset;
	}
	memcpy(ptr + offset, buffer, length);
	return length;
}

//
// On-disk block class
//

size_t TempSpace::FileBlock::read(offset_t offset, void* buffer, size_t length)
{
	if (offset + length > size)
	{
		length = size - offset;
	}
	offset += seek;
	return file->read(offset, buffer, length);
}

size_t TempSpace::FileBlock::write(offset_t offset, const void* buffer, size_t length)
{
	if (offset + length > size)
	{
		length = size - offset;
	}
	offset += seek;
	return file->write(offset, buffer, length);
}

//
// TempSpace::TempSpace
//
// Constructor
//

TempSpace::TempSpace(MemoryPool& p, const Firebird::PathName& prefix, bool dynamic)
		: pool(p), filePrefix(p, prefix),
		  logicalSize(0), physicalSize(0), localCacheUsage(0),
		  head(NULL), tail(NULL), tempFiles(p),
		  initialBuffer(p), initiallyDynamic(dynamic),
		  freeSegments(p)
{
	if (!tempDirs)
	{
		Firebird::MutexLockGuard guard(initMutex);
		if (!tempDirs)
		{
			MemoryPool& def_pool = *getDefaultMemoryPool();
			tempDirs = FB_NEW(def_pool) Firebird::TempDirectoryList(def_pool);
			minBlockSize = Config::getTempBlockSize();

			if (minBlockSize < MIN_TEMP_BLOCK_SIZE)
				minBlockSize = MIN_TEMP_BLOCK_SIZE;
			else
				minBlockSize = FB_ALIGN(minBlockSize, MIN_TEMP_BLOCK_SIZE);
		}
	}
}

//
// TempSpace::~TempSpace
//
// Destructor
//

TempSpace::~TempSpace()
{
	while (head)
	{
		Block* temp = head->next;
		delete head;
		head = temp;
	}

	globalCacheUsage -= localCacheUsage;

	while (tempFiles.getCount())
	{
		delete tempFiles.pop();
	}
}

//
// TempSpace::read
//
// Reads bytes from the temporary space
//

size_t TempSpace::read(offset_t offset, void* buffer, size_t length)
{
	fb_assert(offset + length <= logicalSize);

	if (length)
	{
		// search for the first needed block
		Block* block = findBlock(offset);

		UCHAR* p = static_cast<UCHAR*>(buffer);
		size_t l = length;

		// read data from the block chain
		for (Block* itr = block; itr && l; itr = itr->next, offset = 0)
		{
			const size_t n = itr->read(offset, p, l);
			p += n;
			l -= n;
		}

		fb_assert(!l);
	}

	return length;
}

//
// TempSpace::write
//
// Writes bytes to the temporary space
//

size_t TempSpace::write(offset_t offset, const void* buffer, size_t length)
{
	fb_assert(offset <= logicalSize);

	if (offset + length > logicalSize)
	{
		// not enough space, allocate one more block
		extend(offset + length - logicalSize);
	}

	if (length)
	{
		// search for the first needed block
		Block* const block = findBlock(offset);

		const UCHAR* p = static_cast<const UCHAR*>(buffer);
		size_t l = length;

		// write data to as many blocks as necessary
		for (Block* itr = block; itr && l; itr = itr->next, offset = 0)
		{
			const size_t n = itr->write(offset, p, l);
			p += n;
			l -= n;
		}

		fb_assert(!l);
	}

	return length;
}

//
// TempSpace::extend
//
// Increases size of the temporary space
//

void TempSpace::extend(size_t size)
{
	logicalSize += size;

	if (logicalSize > physicalSize)
	{
		const size_t initialSize = initialBuffer.getCount();

		// If the dynamic mode is specified, then we allocate new blocks
		// by growing the same initial memory block in the specified chunks.
		// Once the limit (64KB) is reached, we switch to the generic algorithm
		// (1MB blocks), copy the existing data there and free the initial buffer.
		//
		// This mode should not be used if the caller never works with small blocks.
		// Also, it MUST NOT be used if the caller deals with inMemory() or allocateBatch()
		// routines and caches the pointers to use them later. These pointers may become
		// invalid after resizing the initial block or after switching to large blocks.

		if (initiallyDynamic && logicalSize < MIN_TEMP_BLOCK_SIZE)
		{
			// allocate or extend the initial dynamic block, it will grow up to 64KB
			if (!initialSize)
			{
				fb_assert(!head && !tail);
				head = tail = FB_NEW(pool) InitialBlock(initialBuffer.getBuffer(size), size);
			}
			else
			{
				fb_assert(head == tail);
				size += initialSize;
				initialBuffer.resize(size);
				new (head) InitialBlock(initialBuffer.begin(), size);
			}

			physicalSize = size;
			return;
		}

		if (initialSize)
		{
			fb_assert(head == tail);
			delete head;
			head = tail = NULL;
			size = FB_ALIGN(logicalSize, minBlockSize);
			physicalSize = size;
		}
		else
		{
			size = FB_ALIGN(logicalSize - physicalSize, minBlockSize);
			physicalSize += size;
		}

		Block* block = NULL;

		if (globalCacheUsage + size <= size_t(Config::getTempCacheLimit()))
		{
			try
			{
				// allocate block in virtual memory
				block = FB_NEW(pool) MemoryBlock(FB_NEW(pool) UCHAR[size], tail, size);
				localCacheUsage += size;
				globalCacheUsage += size;
			}
			catch (const Firebird::BadAlloc&)
			{
				// not enough memory
			}
		}

		if (!block)
		{
			// allocate block in the temp file
			TempFile* const file = setupFile(size);
			fb_assert(file);
			if (tail && tail->sameFile(file))
			{
				fb_assert(!initialSize);
				tail->size += size;
				return;
			}
			block = FB_NEW(pool) FileBlock(file, tail, size);
		}

		// preserve the initial contents, if any
		if (initialSize)
		{
			block->write(0, initialBuffer.begin(), initialSize);
			initialBuffer.free();
		}

		// append new block to the chain
		if (!head)
		{
			head = block;
		}
		tail = block;
	}
}

//
// TempSpace::findBlock
//
// Locates the space block corresponding to the given global offset
//

TempSpace::Block* TempSpace::findBlock(offset_t& offset) const
{
	fb_assert(offset <= logicalSize);

	Block* block = NULL;

	if (offset < physicalSize / 2)
	{
		// walk forward
		block = head;
		while (block && offset >= block->size)
		{
			offset -= block->size;
			block = block->next;
		}
		fb_assert(block);
	}
	else
	{
		// walk backward
		block = tail;
		while (block && physicalSize - offset > block->size)
		{
			offset += block->size;
			block = block->prev;
		}
		fb_assert(block);
		offset -= physicalSize - block->size;
	}

	fb_assert(offset <= block->size);
	return block;
}

//
// TempSpace::setupFile
//
// Allocates the required space in some temporary file
//

TempFile* TempSpace::setupFile(size_t size)
{
	ISC_STATUS_ARRAY status_vector = {0};

	for (size_t i = 0; i < tempDirs->getCount(); i++)
	{
		TempFile* file = NULL;

		Firebird::PathName directory = (*tempDirs)[i];
		PathUtils::ensureSeparator(directory);

		for (size_t j = 0; j < tempFiles.getCount(); j++)
		{
			Firebird::PathName dirname, filename;
			PathUtils::splitLastComponent(dirname, filename, tempFiles[j]->getName());
			PathUtils::ensureSeparator(dirname);
			if (!directory.compare(dirname))
			{
				file = tempFiles[j];
				break;
			}
		}

		try
		{
			if (!file)
			{
				file = FB_NEW(pool) TempFile(pool, filePrefix, directory);
				tempFiles.add(file);
			}

			file->extend(size);
		}
		catch (const Firebird::system_error& ex)
		{
			ex.stuff_exception(status_vector);
			continue;
		}

		return file;
	}

	// no room in all directories
	Firebird::Arg::Gds status(isc_out_of_temp_space);
	status.append(Firebird::Arg::StatusVector(status_vector));
	iscLogStatus(NULL, status.value());
	status.raise();

	return NULL; // compiler silencer
}

//
// TempSpace::allocateSpace
//
// Allocate available space in free segments. Extend file if necessary
//

offset_t TempSpace::allocateSpace(size_t size)
{
	// Find the best available space. This is defined as the smallest free space
	// that is big enough. This preserves large blocks.
	Segment* best = NULL;

	// Search through the available space in the not used segments list
	for (bool found = freeSegments.getFirst(); found; found = freeSegments.getNext())
	{
		Segment* const space = &freeSegments.current();
		// If this is smaller than our previous best, use it
		if (space->size >= size && (!best || (space->size < best->size))) {
			best = space;
		}
	}

	// If we didn't find any space, allocate it at the end of the file
	if (!best)
	{
		extend(size);
		return getSize() - size;
	}

	// Set up the return parameters
	const offset_t position = best->position;
	best->size -= size;
	best->position += size;

	// If the hunk was an exact fit, remove the segment from the list
	if (!best->size)
	{
		if (!freeSegments.locate(best->position))
			fb_assert(false);

		freeSegments.fastRemove();
	}

	return position;
}

//
// TempSpace::releaseSpace
//
// Return previously allocated segment back into not used segments list and
// join it with adjacent segments if found
//

void TempSpace::releaseSpace(offset_t position, size_t size)
{
	fb_assert(size > 0);
	fb_assert(position < getSize());	// Block starts in file
	const offset_t end = position + size;
	fb_assert(end <= getSize());		// Block ends in file

	if (freeSegments.locate(Firebird::locEqual, end))
	{
		// The next segment is found to be adjacent
		Segment* const next_seg = &freeSegments.current();
		next_seg->position -= size;
		next_seg->size += size;

		if (freeSegments.getPrev())
		{
			// Check the prior segment for being adjacent
			Segment* const prior_seg = &freeSegments.current();
			if (position == prior_seg->position + prior_seg->size)
			{
				next_seg->position -= prior_seg->size;
				next_seg->size += prior_seg->size;
				freeSegments.fastRemove();
			}
		}

		return;
	}
	
	if (freeSegments.locate(Firebird::locLess, position))
	{
		// Check the prior segment for being adjacent
		Segment* const prior_seg = &freeSegments.current();
		if (position == prior_seg->position + prior_seg->size)
		{
			prior_seg->size += size;
			return;
		}
	}

	freeSegments.add(Segment(position, size));
}

//
// TempSpace::inMemory
//
// Return contiguous chunk of memory if present at given location
//

UCHAR* TempSpace::inMemory(offset_t begin, size_t size) const
{
	const Block* block = findBlock(begin);
	return block ? block->inMemory(begin, size) : NULL;
}

//
// TempSpace::findMemory
//
// Return contiguous chunk of memory and adjust starting offset
// of search range if found
//

UCHAR* TempSpace::findMemory(offset_t& begin, offset_t end, size_t size) const
{
	offset_t local_offset = begin;
	const offset_t save_begin = begin;
	const Block* block = findBlock(local_offset);

	while (block && (begin + size <= end))
	{
		UCHAR* const mem = block->inMemory(local_offset, size);
		if (mem)
		{
			return mem;
		}

		begin += block->size - local_offset;
		local_offset = 0;
		block = block->next;
	}

	begin = save_begin;
	return NULL;
}

//
// TempSpace::validate
//
// Validate internal lists for consistency and return back to caller
// amount of available free space
//

bool TempSpace::validate(offset_t& free) const
{
	free = 0;
	FreeSegmentTree::ConstAccessor accessor(&freeSegments);
	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		const offset_t size = accessor.current().size;
		fb_assert(size != 0);
		free += size;
	}

	offset_t disk = 0;
	for (size_t i = 0; i < tempFiles.getCount(); i++)
		disk += tempFiles[i]->getSize();

	return ((initialBuffer.getCount() + localCacheUsage + disk) == physicalSize);
}


//
// TempSpace::allocateBatch
//
// Allocate up to 'count' contiguous chunks of memory available in free
// segments if any. Adjust size of chunks between minSize and maxSize
// accordingly to available free space (assuming all of the free space
// is in memory blocks). Algorithm is very simple and can be improved in future
//

size_t TempSpace::allocateBatch(size_t count, size_t minSize, size_t maxSize, Segments& segments)
{
	// adjust passed chunk size to amount of free memory we have and number
	// of runs still not allocated.
	offset_t freeMem = 0;

	for (bool found = freeSegments.getFirst(); found; found = freeSegments.getNext())
		freeMem += freeSegments.current().size;

	freeMem = MIN(freeMem / count, maxSize);
	freeMem = MAX(freeMem, minSize);
	freeMem = MIN(freeMem, minBlockSize);
	freeMem &= ~(FB_ALIGNMENT - 1);

	bool is_positioned = freeSegments.getFirst();
	while (segments.getCount() < count && is_positioned)
	{
		Segment* freeSpace = &freeSegments.current();
		offset_t freeSeek = freeSpace->position;
		const offset_t freeEnd = freeSpace->position + freeSpace->size;

		UCHAR* const mem = findMemory(freeSeek, freeEnd, freeMem);

		if (mem)
		{
			fb_assert(freeSeek + freeMem <= freeEnd);
#ifdef DEV_BUILD
			offset_t seek1 = freeSeek;
			UCHAR* const p = findMemory(seek1, freeEnd, freeMem);
			fb_assert(p == mem);
			fb_assert(seek1 == freeSeek);
#endif
			if (freeSeek != freeSpace->position)
			{
				const offset_t skip_size = freeSeek - freeSpace->position;
				const Segment skip_space(freeSpace->position, skip_size);

				freeSpace->position += skip_size;
				freeSpace->size -= skip_size;
				fb_assert(freeSpace->size != 0);

				if (!freeSegments.add(skip_space))
					fb_assert(false);

				if (!freeSegments.locate(skip_space.position + skip_size))
					fb_assert(false);

				freeSpace = &freeSegments.current();
			}

			SegmentInMemory seg;
			seg.memory = mem;
			seg.position = freeSeek;
			seg.size = freeMem;
			segments.add(seg);

			freeSpace->position += freeMem;
			freeSpace->size -= freeMem;

			if (!freeSpace->size)
			{
				is_positioned = freeSegments.fastRemove();
			}
		}
		else
		{
			is_positioned = freeSegments.getNext();
		}
	}

	return segments.getCount();
}
