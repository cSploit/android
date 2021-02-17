/*
 *      PROGRAM:        JRD Sort
 *      MODULE:         sort.cpp
 *      DESCRIPTION:    Top level sort module
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2001-09-24  SJL - Temporary fix for large sort file bug
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <errno.h>
#include <string.h>
#include "../jrd/jrd.h"
#include "../jrd/sort.h"
#include "gen/iberror.h"
#include "../jrd/intl.h"
#include "../common/gdsassert.h"
#include "../jrd/req.h"
#include "../jrd/rse.h"
#include "../jrd/val.h"
#include "../jrd/err_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/thread_proto.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_IO_H
#include <io.h> // lseek, read, write, close
#endif

const USHORT RUN_GROUP			= 8;
const USHORT MAX_MERGE_LEVEL	= 2;

using namespace Jrd;
using namespace Firebird;

void SortOwner::unlinkAll()
{
	while (sorts.getCount())
		delete sorts.pop();
}

// The sort buffer size should be just under a multiple of the
// hardware memory page size to account for memory allocator
// overhead. On most platorms, this saves 4KB to 8KB per sort
// buffer from being allocated but not used.
//
// dimitr:	this comment is outdated since FB 1.5, where max buffer size
//			of (128KB - overhead) has been replaced with exact 128KB.

const ULONG MIN_SORT_BUFFER_SIZE = 1024 * 16;	// 16KB
const ULONG MAX_SORT_BUFFER_SIZE = 1024 * 128;	// 128KB

// the size of sr_bckptr (everything before sort_record) in bytes
#define SIZEOF_SR_BCKPTR OFFSET(sr*, sr_sort_record)
// the size of sr_bckptr in # of 32 bit longwords
#define SIZEOF_SR_BCKPTR_IN_LONGS static_cast<signed>(SIZEOF_SR_BCKPTR / sizeof(SLONG))
// offset in array of pointers to back record pointer (sr_bckptr)
#define BACK_OFFSET (-static_cast<signed>(SIZEOF_SR_BCKPTR / sizeof(SLONG*)))

//#define DIFF_LONGS(a, b)         ((a) - (b))
#define SWAP_LONGS(a, b, t)       {t = a; a = b; b = t;}

// Compare p and q both SORTP pointers for l 32-bit longwords
// l != 0 if p and q are not equal for l bytes
#define  DO_32_COMPARE(p, q, l)   do if (*p++ != *q++) break; while (--l);

#define MOVE_32(len, from, to)      memcpy(to, from, len * 4)

// These values are not defined as const as they are passed to
// the diddleKey routines which mangles them.
// As the diddleKey routines differ on VAX (little endian) and non VAX
// (big endian) patforms, making the following const caused a core on the
// Intel Platforms, while Solaris was working fine.

static ULONG low_key[] = { 0, 0, 0, 0, 0, 0 };

static ULONG high_key[] =
{
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG,
	MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG, MAX_ULONG
};

#ifdef DEV_BUILD
#define CHECK_FILE(a) checkFile((a));
#else
#define CHECK_FILE(a)
#endif

namespace
{
	static const char* const SCRATCH = "fb_sort_";

	class RunSort
	{
	public:
		explicit RunSort(run_control* irun) : run(irun) {}
		RunSort() : run(NULL) {}

		static FB_UINT64 generate(const RunSort& item)
		{
			return item.run->run_seek;
		}

		run_control* run;
	};

	inline void swap(SORTP** a, SORTP** b)
	{
		((SORTP***) (*a))[BACK_OFFSET] = b;
		((SORTP***) (*b))[BACK_OFFSET] = a;
		SORTP* temp = *a;
		*a = *b;
		*b = temp;
	}
} // namespace


Sort::Sort(Database* dbb,
		   SortOwner* owner,
		   ULONG record_length,
		   size_t keys,
		   size_t unique_keys,
		   const sort_key_def* key_description,
		   FPTR_REJECT_DUP_CALLBACK call_back,
		   void* user_arg,
		   FB_UINT64 max_records)
	: m_dbb(dbb), m_last_record(NULL), m_next_pointer(NULL), m_records(0),
	  m_runs(NULL), m_merge(NULL), m_free_runs(NULL),
	  m_flags(0), m_merge_pool(NULL),
	  m_description(owner->getPool(), keys)
{
/**************************************
 *
 * Initialize for a sort.  All we really need is a description
 * of the sort keys.  Return the address  of a sort context block.
 * If duplicate control is required, the user may specify a call
 * back routine.  If supplied, the call back routine is called
 * with three argument: the two records and the user supplied
 * argument.  If the call back routine returns TRUE, the second
 * duplicate record is eliminated.
 *
 * hvlad: when duplicates are eliminating only first unique_keys will be
 *		  compared. This is used at creation of unique index since sort key
 *		  includes index key (which must be unique) and record numbers.
 *
 **************************************/
	fb_assert(owner);
	fb_assert(unique_keys <= keys);

	try
	{
		// Allocate and setup a sort context block, including copying the
		// key description vector. Round the record length up to the next
		// longword, and add a longword to a pointer back to the pointer slot.

		MemoryPool& pool = owner->getPool();

		m_longs = ROUNDUP(record_length + SIZEOF_SR_BCKPTR, FB_ALIGNMENT) >> SHIFTLONG;
		m_dup_callback = call_back;
		m_dup_callback_arg = user_arg;
		m_max_records = max_records;

		for (size_t i = 0; i < keys; i++)
		{
			m_description.add(key_description[i]);
		}

		const sort_key_def* p = m_description.end() - 1;

		m_key_length = ROUNDUP(p->skd_offset + p->skd_length, sizeof(SLONG)) >> SHIFTLONG;

		while (unique_keys < keys)
		{
			p--;
			unique_keys++;
		}

		m_unique_length = ROUNDUP(p->skd_offset + p->skd_length, sizeof(SLONG)) >> SHIFTLONG;

		// Next, try to allocate a "big block". How big? Big enough!

		allocateBuffer(pool);

		m_end_memory = m_memory + m_size_memory;
		m_first_pointer = (sort_record**) m_memory;

		// Set up the temp space

		try
		{
			m_space = FB_NEW(pool) TempSpace(pool, SCRATCH, false);
		}
		catch (const Exception&)
		{
			releaseBuffer();
			throw;
		}

		// Set up to receive the first record

		init();

		// Link in new sort block

		m_owner = owner;
		owner->linkSort(this);
	}
	catch (const BadAlloc&)
	{
		Firebird::Arg::Gds(isc_sort_mem_err).raise();
	}
	catch (const status_exception& ex)
	{
		Firebird::Arg::Gds status(isc_sort_err);
		status.append(Firebird::Arg::StatusVector(ex.value()));
		status.raise();
	}
}


Sort::~Sort()
{
	// Unlink the sort
	m_owner->unlinkSort(this);

	// Release the temporary space
	delete m_space;

	// If runs are allocated and not in the big block, release them.
	// Then release the big block.

	releaseBuffer();

	// Clean up the runs that were used

	run_control* run;
	while ( (run = m_runs) )
	{
		m_runs = run->run_next;
		if (run->run_buff_alloc)
			delete[] run->run_buffer;
		delete run;
	}

	// Clean up the free runs also

	while ( (run = m_free_runs) )
	{
		m_free_runs = run->run_next;
		if (run->run_buff_alloc)
			delete[] run->run_buffer;
		delete run;
	}

	delete[] m_merge_pool;
}


void Sort::get(thread_db* tdbb, ULONG** record_address)
{
/**************************************
 *
 * Get a record from sort (in order, of course).
 * The address of the record is returned in <record_address>
 * If the stream is exhausted, SORT_get puts NULL in <record_address>.
 *
 **************************************/
	sort_record* record = NULL;

	try
	{
		// If there weren't any runs, everything fit in memory. Just return stuff.

		if (!m_merge)
		{
			while (true)
			{
				if (m_records == 0)
				{
					record = NULL;
					break;
				}
				m_records--;
				if ( (record = *m_next_pointer++) )
					break;
			}
		}
		else
		{
			record = getMerge(m_merge);
		}

		*record_address = (ULONG*) record;

		if (record)
		{
			diddleKey((UCHAR*) record->sort_record_key, false);
		}

		tdbb->bumpStats(RuntimeStatistics::SORT_GETS);
	}
	catch (const BadAlloc&)
	{
		Firebird::Arg::Gds(isc_sort_mem_err).raise();
	}
	catch (const status_exception& ex)
	{
		Firebird::Arg::Gds status(isc_sort_err);
		status.append(Firebird::Arg::StatusVector(ex.value()));
		status.raise();
	}
}


void Sort::put(thread_db* tdbb, ULONG** record_address)
{
/**************************************
 *
 * Allocate space for a record for sort.  The caller is responsible
 * for moving in the record.
 *
 * Records are added from the top (higher addresses) of sort memory going down.  Record
 * pointers are added at the bottom (lower addresses) of sort memory going up.  When
 * they overlap, the records in memory are sorted and written to a "run"
 * in the scratch files.  The runs are eventually merged.
 *
 **************************************/
	try
	{
		// Find the last record passed in, and zap the keys something comparable
		// by unsigned longword compares

		SR* record = m_last_record;

		if (record != (SR*) m_end_memory)
		{
			diddleKey((UCHAR*) (record->sr_sort_record.sort_record_key), true);
		}

		// If there isn't room for the record, sort and write the run.
		// Check that we are not at the beginning of the buffer in addition
		// to checking for space for the record. This avoids the pointer
		// record from underflowing in the second condition.
		if ((UCHAR*) record < m_memory + m_longs ||
			(UCHAR*) NEXT_RECORD(record) <= (UCHAR*) (m_next_pointer + 1))
		{
			putRun();
			while (true)
			{
				run_control* run = m_runs;
				const USHORT depth = run->run_depth;
				if (depth == MAX_MERGE_LEVEL)
					break;
				USHORT count = 1;
				while ((run = run->run_next) && run->run_depth == depth)
					count++;
				if (count < RUN_GROUP)
					break;
				mergeRuns(count);
			}
			init();
			record = m_last_record;
		}

		record = NEXT_RECORD(record);

		// Make sure the first longword of the record points to the pointer
		m_last_record = record;
		record->sr_bckptr = m_next_pointer;

		// Move key_id into *m_next_pointer and then
		// increment m_next_pointer
		*m_next_pointer++ = reinterpret_cast<sort_record*>(record->sr_sort_record.sort_record_key);
		m_records++;
		*record_address = (ULONG*) record->sr_sort_record.sort_record_key;

		tdbb->bumpStats(RuntimeStatistics::SORT_PUTS);
	}
	catch (const BadAlloc&)
	{
		Firebird::Arg::Gds(isc_sort_mem_err).raise();
	}
	catch (const status_exception& ex)
	{
		Firebird::Arg::Gds status(isc_sort_err);
		status.append(Firebird::Arg::StatusVector(ex.value()));
		status.raise();
	}
}


void Sort::sort(thread_db* tdbb)
{
/**************************************
 *
 * Perform any intermediate computing before giving records
 * back.  If there weren't any runs, run sort the buffer.
 * If there were runs, sort and write out the last run_control and
 * build a merge tree.
 *
 **************************************/
	run_control* run;
	merge_control* merge;
	merge_control* merge_pool;

	try
	{
		if (m_last_record != (SR*) m_end_memory)
		{
			diddleKey((UCHAR*) KEYOF(m_last_record), true);
		}

		// If there aren't any runs, things fit nicely in memory. Just sort the mess
		// and we're ready for output.
		if (!m_runs)
		{
			sort();
			m_next_pointer = m_first_pointer + 1;
			m_flags |= scb_sorted;
			tdbb->bumpStats(RuntimeStatistics::SORTS);
			return;
		}

		// Write the last records as a run_control

		putRun();

		CHECK_FILE(NULL);

		// Merge runs of low depth to free memory part of temp space
		// they use and to make total runs count lower. This is fast
		// because low depth runs usually sit in memory
		ULONG run_count = 0, low_depth_cnt = 0;
		for (run = m_runs; run; run = run->run_next)
		{
			++run_count;
			if (run->run_depth < MAX_MERGE_LEVEL)
				low_depth_cnt++;
		}

		if (low_depth_cnt > 1 && low_depth_cnt < run_count)
		{
			mergeRuns(low_depth_cnt);
			CHECK_FILE(NULL);
		}

		// Build a merge tree for the run_control blocks. Start by laying them all out
		// in a vector. This is done to allow us to build a merge tree from the
		// bottom up, ensuring that a balanced tree is built.

		for (run_count = 0, run = m_runs; run; run = run->run_next)
		{
			if (run->run_buff_alloc)
			{
				delete[] run->run_buffer;
				run->run_buff_alloc = false;
			}
			++run_count;
		}

		AutoPtr<run_merge_hdr*, ArrayDelete<run_merge_hdr*> > streams(
			FB_NEW(m_owner->getPool()) run_merge_hdr*[run_count]);

		run_merge_hdr** m1 = streams;
		for (run = m_runs; run; run = run->run_next)
			*m1++ = (run_merge_hdr*) run;
		ULONG count = run_count;

		// We're building a b-tree of the sort merge blocks, we have (count)
		// leaves already, so we *know* we need (count-1) merge blocks.

		if (count > 1)
		{
			fb_assert(!m_merge_pool);	// shouldn't have a pool
			m_merge_pool = FB_NEW(m_owner->getPool()) merge_control[count - 1];
			merge_pool = m_merge_pool;
			memset(merge_pool, 0, (count - 1) * sizeof(merge_control));
		}
		else
		{
			// Merge of 1 or 0 runs doesn't make sense
			fb_assert(false);				// We really shouldn't get here
			merge = (merge_control*) *streams;	// But if we do...
		}

		// Each pass through the vector builds a level of the merge tree
		// by condensing two runs into one.
		// We will continue to make passes until there is a single item.
		//
		// See also kissing cousin of this loop in mergeRuns()

		while (count > 1)
		{
			run_merge_hdr** m2 = m1 = streams;

			// "m1" is used to sequence through the runs being merged,
			// while "m2" points at the new merged run

			while (count >= 2)
			{
				merge = merge_pool++;
				merge->mrg_header.rmh_type = RMH_TYPE_MRG;

				// garbage watch
				fb_assert(((*m1)->rmh_type == RMH_TYPE_MRG) || ((*m1)->rmh_type == RMH_TYPE_RUN));

				(*m1)->rmh_parent = merge;
				merge->mrg_stream_a = *m1++;

				// garbage watch
				fb_assert(((*m1)->rmh_type == RMH_TYPE_MRG) || ((*m1)->rmh_type == RMH_TYPE_RUN));

				(*m1)->rmh_parent = merge;
				merge->mrg_stream_b = *m1++;

				merge->mrg_record_a = NULL;
				merge->mrg_record_b = NULL;

				*m2++ = (run_merge_hdr*) merge;
				count -= 2;
			}

			if (count)
				*m2++ = *m1++;
			count = m2 - streams;
		}

		streams.reset();

		merge->mrg_header.rmh_parent = NULL;
		m_merge = merge;
		m_longs -= SIZEOF_SR_BCKPTR_IN_LONGS;

		// Allocate space for runs. The more memory we assign to each run the
		// faster we will read scratch file and return sorted records to caller.
		// At first try to reuse free memory from temp space. Note that temp space
		// itself allocated memory by at least TempSpace::getMinBlockSize chunks.
		// As we need contiguous memory don't ask for bigger parts
		ULONG allocSize = MAX_SORT_BUFFER_SIZE * RUN_GROUP;
		ULONG allocated = allocate(run_count, allocSize, true);

		if (allocated < run_count)
		{
			const ULONG rec_size = m_longs << SHIFTLONG;
			allocSize = MAX_SORT_BUFFER_SIZE * RUN_GROUP;
			for (run = m_runs; run; run = run->run_next)
			{
				if (!run->run_buffer)
				{
					size_t mem_size = MIN(allocSize / rec_size, run->run_records) * rec_size;
					UCHAR* mem = NULL;
					try
					{
						mem = FB_NEW(m_owner->getPool()) UCHAR[mem_size];
					}
					catch (const BadAlloc&)
					{
						mem_size = (mem_size / (2 * rec_size)) * rec_size;
						if (!mem_size)
							throw;
						mem = FB_NEW(m_owner->getPool()) UCHAR[mem_size];
					}
					run->run_buff_alloc = true;
					run->run_buff_cache = false;

					run->run_buffer = mem;
					mem += mem_size;
					run->run_record = reinterpret_cast<sort_record*>(mem);
					run->run_end_buffer = mem;
				}
			}
		}

		sortRunsBySeek(run_count);

		m_flags |= scb_sorted;
		tdbb->bumpStats(RuntimeStatistics::SORTS);
	}
	catch (const BadAlloc&)
	{
		Firebird::Arg::Gds(isc_sort_mem_err).raise();
	}
	catch (const status_exception& ex)
	{
		Firebird::Arg::Gds status(isc_sort_err);
		status.append(Firebird::Arg::StatusVector(ex.value()));
		status.raise();
	}
}


void Sort::allocateBuffer(MemoryPool& pool)
{
#ifdef DEBUG_MERGE
	// To debug the merge algorithm, force the in-memory pool to be VERY small
	m_size_memory = 2000;
	m_memory = FB_NEW(pool) UCHAR[m_size_memory];
	return;
#endif

	SyncLockGuard guard(&m_dbb->dbb_sortbuf_sync, SYNC_EXCLUSIVE, "Sort::allocateBuffer");

	if (m_dbb->dbb_sort_buffers.hasData())
	{
		// The sort buffer cache has at least one big block, let's use it
		m_size_memory = MAX_SORT_BUFFER_SIZE;
		m_memory = m_dbb->dbb_sort_buffers.pop();
	}
	else
	{
		// Try to get a big chunk of memory, if we can't try smaller and
		// smaller chunks until we can get the memory. If we get down to
		// too small a chunk - punt and report not enough memory.
		//
		// At the first attempt, allocate from the permanent pool in order
		// to have the big block being cached for later reuse. If unsuccessful,
		// switch to the sort owner pool.

		try
		{
			m_size_memory = MAX_SORT_BUFFER_SIZE;
			m_memory = FB_NEW(*m_dbb->dbb_permanent) UCHAR[m_size_memory];
		}
		catch (const BadAlloc&)
		{
			// not enough memory, retry with a smaller buffer

			while (true)
			{
				try
				{
					m_size_memory /= 2;
					m_memory = FB_NEW(pool) UCHAR[m_size_memory];
					break;
				}
				catch (const BadAlloc&)
				{
					if (m_size_memory <= MIN_SORT_BUFFER_SIZE)
						throw;
				}
			}
		}
	}
}


void Sort::releaseBuffer()
{
	// Here we cache blocks to be reused later, but only the biggest ones

	const size_t MAX_CACHED_SORT_BUFFERS = 8; // 1MB

	SyncLockGuard guard(&m_dbb->dbb_sortbuf_sync, SYNC_EXCLUSIVE, "Sort::releaseBuffer");

	if (m_size_memory == MAX_SORT_BUFFER_SIZE &&
		m_dbb->dbb_sort_buffers.getCount() < MAX_CACHED_SORT_BUFFERS)
	{
		m_dbb->dbb_sort_buffers.push(m_memory);
	}
	else
		delete[] m_memory;
}


#ifdef WORDS_BIGENDIAN
void Sort::diddleKey(UCHAR* record, bool direction)
{
/**************************************
 *
 * Perform transformation between the natural form of a record
 * and a form that can be sorted in unsigned comparison order.
 *
 * direction - true for SORT_put() and false for SORT_get()
 *
 **************************************/
	USHORT flag;

	for (sort_key_def* key = m_description.begin(), *end = m_description.end(); key < end; key++)
	{
		UCHAR* p = record + key->skd_offset;
		USHORT n = key->skd_length;
		USHORT complement = key->skd_flags & SKD_descending;

		// This trick replaces possibly negative zero with positive zero, so that both
		// would be transformed into the same sort key and thus properly compared (see CORE-3547).
		// Note that it's done only once, per SORT_put(), i.e. the transformation is not symmetric.
		if (direction)
		{
			if (key->skd_dtype == SKD_double)
			{
				if (*(double*) p == 0)
				{
					*(double*) p = 0;
				}
			}
			else if (key->skd_dtype == SKD_float)
			{
				if (*(float*) p == 0)
				{
					*(float*) p = 0;
				}
			}
		}

		switch (key->skd_dtype)
		{
		case SKD_ulong:
		case SKD_ushort:
		case SKD_bytes:
		case SKD_sql_time:
			break;

			// Stash embedded control info for non-fixed data types in the sort
			// record and zap it so that it doesn't interfere with collation

		case SKD_varying:
			if (direction)
			{
				USHORT& vlen = ((vary*) p)->vary_length;
				if (!(m_flags & scb_sorted))
				{
					*((USHORT*) (record + key->skd_vary_offset)) = vlen;
					const UCHAR fill_char = (key->skd_flags & SKD_binary) ? 0 : ASCII_SPACE;
					UCHAR* fill_pos = p + sizeof(USHORT) + vlen;
					const USHORT fill = n - sizeof(USHORT) - vlen;
					if (fill)
						memset(fill_pos, fill_char, fill);
				}
				vlen = 0;
			}
			break;

		case SKD_cstring:
			if (direction)
			{
				const UCHAR fill_char = (key->skd_flags & SKD_binary) ? 0 : ASCII_SPACE;
				if (!(m_flags & scb_sorted))
				{
					const USHORT l = strlen(reinterpret_cast<char*>(p));
					*((USHORT*) (record + key->skd_vary_offset)) = l;
					UCHAR* fill_pos = p + l;
					const USHORT fill = n - l;
					if (fill)
						memset(fill_pos, fill_char, fill);
				}
				else
				{
					USHORT l = *((USHORT*) (record + key->skd_vary_offset));
					*(p + l) = fill_char;
				}
			}
			break;

		case SKD_text:
			break;

		case SKD_float:
		case SKD_double:
			flag = (direction || !complement) ? direction : TRUE;
			if (flag ^ (*p >> 7))
				*p ^= 1 << 7;
			else
				complement = !complement;
			break;

		case SKD_long:
		case SKD_short:
		case SKD_quad:
		case SKD_timestamp:
		case SKD_sql_date:
		case SKD_int64:
			*p ^= 1 << 7;
			break;

		default:
			fb_assert(false);
			break;
		}

		if (complement && n)
		{
			do
			{
				*p++ ^= -1;
			} while (--n);
		}

		// Flatter but don't complement control info for non-fixed
		// data types when restoring the data

		if (key->skd_dtype == SKD_varying && !direction)
		{
			p = record + key->skd_offset;
			((vary*) p)->vary_length = *((USHORT*) (record + key->skd_vary_offset));
		}

		if (key->skd_dtype == SKD_cstring && !direction)
		{
			p = record + key->skd_offset;
			USHORT l = *((USHORT*) (record + key->skd_vary_offset));
			*(p + l) = 0;
		}
	}
}


#else
void Sort::diddleKey(UCHAR* record, bool direction)
{
/**************************************
 *
 * Perform transformation between the natural form of a record
 * and a form that can be sorted in unsigned comparison order.
 *
 * direction - true for SORT_put() and false for SORT_get()
 *
 **************************************/
	UCHAR c1;
	SSHORT longs, flag;
	ULONG lw;
#ifndef IEEE
	USHORT w;
#endif

	for (sort_key_def* key = m_description.begin(), *end = m_description.end(); key < end; key++)
	{
		UCHAR* p = (UCHAR*) record + key->skd_offset;
		USHORT* wp = (USHORT*) p;
		SORTP* lwp = (SORTP*) p;
		USHORT complement = key->skd_flags & SKD_descending;
		USHORT n = ROUNDUP(key->skd_length, sizeof(SLONG));

		// This trick replaces possibly negative zero with positive zero, so that both
		// would be transformed into the same sort key and thus properly compared (see CORE-3547).
		// Note that it's done only once, per SORT_put(), i.e. the transformation is not symmetric.
		if (direction)
		{
			if (key->skd_dtype == SKD_double)
			{
				if (*(double*) p == 0)
					*(double*) p = 0;
			}
			else if (key->skd_dtype == SKD_float)
			{
				if (*(float*) p == 0)
					*(float*) p = 0;
			}
		}

		switch (key->skd_dtype)
		{
		case SKD_timestamp:
		case SKD_sql_time:
		case SKD_sql_date:
			p[3] ^= 1 << 7;
			break;

		case SKD_ulong:
		case SKD_ushort:
			break;

		case SKD_text:
		case SKD_bytes:
		case SKD_cstring:
		case SKD_varying:

			// Stash embedded control info for non-fixed data types in the sort
			// record and zap it so that it doesn't interfere with collation

			if (key->skd_dtype == SKD_varying && direction)
			{
				USHORT& vlen = ((vary*) p)->vary_length;
				if (!(m_flags & scb_sorted))
				{
					*((USHORT*) (record + key->skd_vary_offset)) = vlen;
					const UCHAR fill_char = (key->skd_flags & SKD_binary) ? 0 : ASCII_SPACE;
					UCHAR* fill_pos = p + sizeof(USHORT) + vlen;
					const USHORT fill = n - sizeof(USHORT) - vlen;
					if (fill)
						memset(fill_pos, fill_char, fill);
				}
				vlen = 0;
			}

			if (key->skd_dtype == SKD_cstring && direction)
			{
				const UCHAR fill_char = (key->skd_flags & SKD_binary) ? 0 : ASCII_SPACE;
				if (!(m_flags & scb_sorted))
				{
					const USHORT l = (USHORT) strlen(reinterpret_cast<char*>(p));
					*((USHORT*) (record + key->skd_vary_offset)) = l;
					UCHAR* fill_pos = p + l;
					const USHORT fill = n - l;
					if (fill)
						memset(fill_pos, fill_char, fill);
				}
				else
				{
					USHORT l = *((USHORT*) (record + key->skd_vary_offset));
					*(p + l) = fill_char;
				}
			}

			longs = n >> SHIFTLONG;
			while (--longs >= 0)
			{
				c1 = p[3];
				p[3] = *p;
				*p++ = c1;
				c1 = p[1];
				p[1] = *p;
				*p = c1;
				p += 3;
			}
			p = (UCHAR*) wp;
			break;

		case SKD_short:
			p[1] ^= 1 << 7;
			break;

		case SKD_long:
			p[3] ^= 1 << 7;
			break;

		case SKD_quad:
			p[7] ^= 1 << 7;
			break;

		case SKD_int64:
			// INT64's fit in TWO LONGS, and hence the SWAP has to happen
			// here for the right order comparison using DO_32_COMPARE
			if (!direction)
				SWAP_LONGS(lwp[0], lwp[1], lw);

			p[7] ^= 1 << 7;

			if (direction)
				SWAP_LONGS(lwp[0], lwp[1], lw);
			break;

#ifdef IEEE
		case SKD_double:
			if (!direction)
			{
				lw = lwp[0];
				lwp[0] = lwp[1];
				lwp[1] = lw;
			}
			flag = (direction || !complement) ? direction : TRUE;
			if (flag ^ (p[7] >> 7))
				p[7] ^= 1 << 7;
			else
				complement = !complement;
			if (direction)
			{
				lw = lwp[0];
				lwp[0] = lwp[1];
				lwp[1] = lw;
			}
			break;

		case SKD_float:
			flag = (direction || !complement) ? direction : TRUE;
			if (flag ^ (p[3] >> 7))
				p[3] ^= 1 << 7;
			else
				complement = !complement;
			break;

#else // IEEE
		case SKD_double:
			w = wp[2];
			wp[2] = wp[3];
			wp[3] = w;

		case SKD_float:
			if (!direction)
			{
				if (complement)
				{
					if (p[3] & 1 << 7)
						complement = !complement;
					else
						p[3] ^= 1 << 7;
				}
				else
				{
					if (p[3] & 1 << 7)
						p[3] ^= 1 << 7;
					else
						complement = !complement;
				}
			}
			w = wp[0];
			wp[0] = wp[1];
			wp[1] = w;
			if (direction)
			{
				if (p[3] & 1 << 7)
					complement = !complement;
				else
					p[3] ^= 1 << 7;
			}
			break;
#endif // IEEE

		default:
			fb_assert(false);
			break;
		}
		if (complement && n)
			do {
				*p++ ^= -1;
			} while (--n);

		// Flatter but don't complement control info for non-fixed
		// data types when restoring the data

		if (key->skd_dtype == SKD_varying && !direction)
		{
			p = (UCHAR*) record + key->skd_offset;
			((vary*) p)->vary_length = *((USHORT*) (record + key->skd_vary_offset));
		}

		if (key->skd_dtype == SKD_cstring && !direction)
		{
			p = (UCHAR*) record + key->skd_offset;
			USHORT l = *((USHORT*) (record + key->skd_vary_offset));
			*(p + l) = 0;
		}
	}
}
#endif


sort_record* Sort::getMerge(merge_control* merge)
{
/**************************************
 *
 * Get next record from a merge tree and/or run_control.
 *
 **************************************/
	SORTP *p;				// no more than 1 SORTP* to a line
	SORTP *q;				// no more than 1 SORTP* to a line
	ULONG l;
	ULONG n;

	sort_record* record = NULL;
	bool eof = false;

	while (merge)
	{
		// If node is a run_control, get the next record (or not) and back to parent

		if (merge->mrg_header.rmh_type == RMH_TYPE_RUN)
		{
			run_control* run = (run_control*) merge;
			merge = run->run_header.rmh_parent;

			// check for end-of-file condition in either direction

			if (run->run_records == 0)
			{
				record = (sort_record*) - 1;
				eof = true;
				continue;
			}

			eof = false;

			// Find the appropriate record in the buffer to return

			if ((record = (sort_record*) run->run_record) < (sort_record*) run->run_end_buffer)
			{
				run->run_record = reinterpret_cast<sort_record*>(NEXT_RUN_RECORD(run->run_record));
				--run->run_records;
				continue;
			}

			// There are records remaining, but the buffer is full.
			// Read a buffer full.

			l = (ULONG) (run->run_end_buffer - run->run_buffer);
			n = run->run_records * m_longs * sizeof(ULONG);
			l = MIN(l, n);
			run->run_seek = readBlock(m_space, run->run_seek, run->run_buffer, l);

			record = reinterpret_cast<sort_record*>(run->run_buffer);
			run->run_record =
				reinterpret_cast<sort_record*>(NEXT_RUN_RECORD(record));
			--run->run_records;

			continue;
		}

		// If've we got a record, somebody asked for it. Find out who.

		if (record)
		{
			if (merge->mrg_stream_a && !merge->mrg_record_a)
			{
				if (eof)
					merge->mrg_stream_a = NULL;
				else
					merge->mrg_record_a = record;
			}
			else if (eof)
				merge->mrg_stream_b = NULL;
			else
				merge->mrg_record_b = record;
		}

		// If either streams need a record and is still active, loop back to pick
		// up the record. If either stream is dry, return the record of the other.
		// If both are dry, indicate eof for this stream.

		record = NULL;
		eof = false;

		if (!merge->mrg_record_a && merge->mrg_stream_a)
		{
			merge = (merge_control*) merge->mrg_stream_a;
			continue;
		}

		if (!merge->mrg_record_b)
		{
			if (merge->mrg_stream_b) {
				merge = (merge_control*) merge->mrg_stream_b;
			}
			else if ( (record = merge->mrg_record_a) )
			{
				merge->mrg_record_a = NULL;
				merge = merge->mrg_header.rmh_parent;
			}
			else
			{
				eof = true;
				record = (sort_record*) - 1;
				merge = merge->mrg_header.rmh_parent;
			}
			continue;
		}

		if (!merge->mrg_record_a)
		{
			record = merge->mrg_record_b;
			merge->mrg_record_b = NULL;
			merge = merge->mrg_header.rmh_parent;
			continue;
		}

		// We have prospective records from each of the sub-streams. Compare them.
		// If equal, offer each to user routine for possible sacrifice.

		p = merge->mrg_record_a->sort_record_key;
		q = merge->mrg_record_b->sort_record_key;
		//l = m_key_length;
		l = m_unique_length;

		DO_32_COMPARE(p, q, l);

		if (l == 0 && m_dup_callback)
		{
			diddleKey((UCHAR*) merge->mrg_record_a, false);
			diddleKey((UCHAR*) merge->mrg_record_b, false);

			if ((*m_dup_callback) ((const UCHAR*) merge->mrg_record_a,
										  (const UCHAR*) merge->mrg_record_b,
										  m_dup_callback_arg))
			{
				merge->mrg_record_a = NULL;
				diddleKey((UCHAR*) merge->mrg_record_b, true);
				continue;
			}
			diddleKey((UCHAR*) merge->mrg_record_a, true);
			diddleKey((UCHAR*) merge->mrg_record_b, true);
		}

		if (l == 0)
		{
			l = m_key_length - m_unique_length;
			if (l != 0)
				DO_32_COMPARE(p, q, l);
		}

		if (p[-1] < q[-1])
		{
			record = merge->mrg_record_a;
			merge->mrg_record_a = NULL;
		}
		else
		{
			record = merge->mrg_record_b;
			merge->mrg_record_b = NULL;
		}

		merge = merge->mrg_header.rmh_parent;
	}

	// Merge pointer is null; we're done. Return either the most
	// recent record, or end of file, as appropriate.

	return eof ? NULL : record;
}


void Sort::init()
{
/**************************************
 *
 * Initialize the sort control block for a quick sort.
 *
 **************************************/

	// If we have run of MAX_MERGE_LEVEL then we have a relatively big sort.
	// Grow sort buffer space to make count of final runs lower and to
	// read\write scratch file by bigger chunks
	// At this point we already allocated some memory for temp space so
	// growing sort buffer space is not a big compared to that

	if (m_size_memory <= MAX_SORT_BUFFER_SIZE && m_runs &&
		m_runs->run_depth == MAX_MERGE_LEVEL)
	{
		const ULONG mem_size = MAX_SORT_BUFFER_SIZE * RUN_GROUP;

		try
		{
			UCHAR* const mem = FB_NEW(m_owner->getPool()) UCHAR[mem_size];

			releaseBuffer();

			m_size_memory = mem_size;
			m_memory = mem;

			m_end_memory = m_memory + m_size_memory;
			m_first_pointer = (sort_record**) m_memory;

			for (run_control *run = m_runs; run; run = run->run_next)
				run->run_depth--;
		}
		catch (const BadAlloc&)
		{} // no-op
	}

	m_next_pointer = m_first_pointer;
	m_last_record = (SR*) m_end_memory;

	*m_next_pointer++ = reinterpret_cast<sort_record*>(low_key);
}


#ifdef DEV_BUILD
void Sort::checkFile(const run_control* temp_run)
{
/**************************************
 *
 * Validate memory and file space allocation
 *
 **************************************/
	FB_UINT64 runs = temp_run ? temp_run->run_size : 0;
	offset_t free = 0;
	FB_UINT64 run_mem = 0;

	fb_assert(m_space->validate(free));

	for (const run_control* run = m_runs; run; run = run->run_next)
	{
		runs += run->run_size;
		run_mem += run->run_mem_size;
	}

	fb_assert((runs + run_mem + free) == m_space->getSize());
}
#endif


ULONG Sort::allocate(ULONG n, ULONG chunkSize, bool useFreeSpace)
{
/**************************************
 *
 * Allocate memory for first n runs
 *
 **************************************/
	const ULONG rec_size = m_longs << SHIFTLONG;
	ULONG allocated = 0, count;
	run_control* run;

	// if some run's already in memory cache - use this memory
	for (run = m_runs, count = 0; count < n; run = run->run_next, count++)
	{
		run->run_buffer = NULL;

		UCHAR* const mem = m_space->inMemory(run->run_seek, run->run_size);

		if (mem)
		{
			run->run_buffer = mem;
			run->run_record = reinterpret_cast<sort_record*>(mem);
			run->run_end_buffer = run->run_buffer + run->run_size;
			run->run_seek += run->run_size; // emulate read
			allocated++;
		}

		run->run_buff_cache = (mem != NULL);
	}

	if (allocated == n || !useFreeSpace)
		return allocated;

	// try to use free blocks from memory cache of work file

	fb_assert(n > allocated);
	TempSpace::Segments segments(m_owner->getPool(), n - allocated);
	allocated += (ULONG) m_space->allocateBatch(n - allocated, MAX_SORT_BUFFER_SIZE, chunkSize, segments);

	if (segments.getCount())
	{
		TempSpace::SegmentInMemory *seg = segments.begin(), *lastSeg = segments.end();
		for (run = m_runs, count = 0; count < n; run = run->run_next, count++)
		{
			if (!run->run_buffer)
			{
				const size_t runSize = MIN(seg->size / rec_size, run->run_records) * rec_size;
				UCHAR* mem = seg->memory;

				run->run_mem_seek = seg->position;
				run->run_mem_size = (ULONG) seg->size;
				run->run_buffer = mem;
				mem += runSize;
				run->run_record = reinterpret_cast<sort_record*>(mem);
				run->run_end_buffer = mem;

				seg++;
				if (seg == lastSeg)
					break;
			}
		}
	}

	return allocated;
}


void Sort::mergeRuns(USHORT n)
{
/**************************************
 *
 * Merge the first n runs hanging off the sort control block, pushing
 * the resulting run back onto the sort control block.
 *
 **************************************/

	// the only place we call mergeRuns with n != RUN_GROUP is SORT_sort
	// and there n < RUN_GROUP * MAX_MERGE_LEVEL
	merge_control blks[RUN_GROUP * MAX_MERGE_LEVEL];

	fb_assert((n - 1) <= FB_NELEM(blks));	// stack var big enough?

	m_longs -= SIZEOF_SR_BCKPTR_IN_LONGS;

	// Make a pass thru the runs allocating buffer space, computing work file
	// space requirements, and filling in a vector of streams with run pointers

	const ULONG rec_size = m_longs << SHIFTLONG;
	UCHAR* buffer = (UCHAR*) m_first_pointer;
	run_control temp_run;
	memset(&temp_run, 0, sizeof(run_control));

	temp_run.run_end_buffer = buffer + (m_size_memory / rec_size) * rec_size;
	temp_run.run_size = 0;
	temp_run.run_buff_alloc = false;

	run_merge_hdr* streams[RUN_GROUP * MAX_MERGE_LEVEL];
	run_merge_hdr** m1 = streams;

	sortRunsBySeek(n);

	// get memory for run's
	run_control* run = m_runs;

	CHECK_FILE(NULL);
	const USHORT allocated = allocate(n, MAX_SORT_BUFFER_SIZE, (run->run_depth > 0));
	CHECK_FILE(NULL);

	const USHORT buffers = m_size_memory / rec_size;
	USHORT count;
	ULONG size = 0;

	if (n > allocated)
		size = rec_size * (buffers / (USHORT) (2 * (n - allocated)));

	for (run = m_runs, count = 0; count < n; run = run->run_next, count++)
	{
		*m1++ = (run_merge_hdr*) run;

		// size = 0 indicates the record is too big to divvy up the
		// big sort buffer, so separate buffers must be allocated

		if (!run->run_buffer)
		{
			if (!size)
			{
				if (!run->run_buff_alloc)
				{
					run->run_buffer = FB_NEW(m_owner->getPool()) UCHAR[rec_size * 2];
					run->run_buff_alloc = true;
				}
				run->run_end_buffer = run->run_buffer + (rec_size * 2);
				run->run_record = reinterpret_cast<sort_record*>(run->run_end_buffer);
			}
			else
			{
				fb_assert(!run->run_buff_alloc);
				run->run_buffer = buffer;
				buffer += size;
				run->run_end_buffer = buffer;
				run->run_record = reinterpret_cast<sort_record*>(run->run_end_buffer);
			}
		}
		temp_run.run_size += run->run_size;
	}
	temp_run.run_record = reinterpret_cast<sort_record*>(buffer);
	temp_run.run_buffer = reinterpret_cast<UCHAR*>(temp_run.run_record);
	temp_run.run_buff_cache = false;

	// Build merge tree bottom up.
	//
	// See also kissing cousin of this loop in SORT_sort()

	merge_control* merge;
	for (count = n, merge = blks; count > 1;)
	{
		run_merge_hdr** m2 = m1 = streams;
		while (count >= 2)
		{
			merge->mrg_header.rmh_type = RMH_TYPE_MRG;

			// garbage watch
			fb_assert(((*m1)->rmh_type == RMH_TYPE_MRG) || ((*m1)->rmh_type == RMH_TYPE_RUN));

			(*m1)->rmh_parent = merge;
			merge->mrg_stream_a = *m1++;

			// garbage watch
			fb_assert(((*m1)->rmh_type == RMH_TYPE_MRG) || ((*m1)->rmh_type == RMH_TYPE_RUN));

			(*m1)->rmh_parent = merge;
			merge->mrg_stream_b = *m1++;

			merge->mrg_record_a = NULL;
			merge->mrg_record_b = NULL;
			*m2++ = (run_merge_hdr*) merge;
			merge++;
			count -= 2;
		}
		if (count)
			*m2++ = *m1++;
		count = m2 - streams;
	}

	--merge;
	merge->mrg_header.rmh_parent = NULL;

	// Merge records into run
	CHECK_FILE(NULL);

	sort_record* q = reinterpret_cast<sort_record*>(temp_run.run_buffer);
	FB_UINT64 seek = temp_run.run_seek = m_space->allocateSpace(temp_run.run_size);
	temp_run.run_records = 0;

	CHECK_FILE(&temp_run);

	const sort_record* p;
	while ( (p = getMerge(merge)) )
	{
		if (q >= (sort_record*) temp_run.run_end_buffer)
		{
			size = (UCHAR*) q - temp_run.run_buffer;
			seek = writeBlock(m_space, seek, temp_run.run_buffer, size);
			q = reinterpret_cast<sort_record*>(temp_run.run_buffer);
		}
		ULONG longs_count = m_longs;
		do {
			*q++ = *p++;
		} while (--longs_count);
		++temp_run.run_records;
	}

	// Write the tail of the new run and return any unused space

	if ( (size = (UCHAR*) q - temp_run.run_buffer) )
		seek = writeBlock(m_space, seek, temp_run.run_buffer, size);

	// If the records did not fill the allocated run (such as when duplicates are
	// rejected), then free the remainder and diminish the size of the run accordingly

	if (seek - temp_run.run_seek < temp_run.run_size)
	{
		m_space->releaseSpace(seek, temp_run.run_seek + temp_run.run_size - seek);
		temp_run.run_size = seek - temp_run.run_seek;
	}

	// Make a final pass thru the runs releasing space, blocks, etc.

	for (count = 0; count < n; count++)
	{
		// Remove run from list of in-use run blocks
		run = m_runs;
		m_runs = run->run_next;
		seek = run->run_seek - run->run_size;

		// Free the sort file space associated with the run

		m_space->releaseSpace(seek, run->run_size);

		if (run->run_mem_size)
		{
			m_space->releaseSpace(run->run_mem_seek, run->run_mem_size);
			run->run_mem_seek = run->run_mem_size = 0;
		}

		run->run_buff_cache = false;
		if (run->run_buff_alloc)
		{
			delete[] run->run_buffer;
			run->run_buff_alloc = false;
		}
		run->run_buffer = NULL;

		// Add run descriptor to list of unused run descriptor blocks

		run->run_next = m_free_runs;
		m_free_runs = run;
	}

	m_free_runs = run->run_next;

	temp_run.run_header.rmh_type = RMH_TYPE_RUN;
	temp_run.run_depth = run->run_depth;
	temp_run.run_buff_cache = false;
	temp_run.run_buffer = NULL;
	*run = temp_run;
	++run->run_depth;
	run->run_next = m_runs;
	m_runs = run;
	m_longs += SIZEOF_SR_BCKPTR_IN_LONGS;

	CHECK_FILE(NULL);
}


void Sort::quick(SLONG size, SORTP** pointers, ULONG length)
{
/**************************************
 *
 * Sort an array of record pointers.  The routine assumes the following:
 *
 * a.  Each element in the array points to the key of a record.
 *
 * b.  Keys can be compared by auto-incrementing unsigned longword
 *     compares.
 *
 * c.  Relative array positions "-1" and "size" point to guard records
 *     containing the least and the greatest possible sort keys.
 *
 * ***************************************************************
 * * Boy, did the assumption below turn out to be pretty stupid! *
 * ***************************************************************
 *
 * Note: For the time being, the key length field is ignored on the
 * assumption that something will eventually stop the comparison.
 *
 * WARNING: THIS ROUTINE DOES NOT MAKE A FINAL PASS TO UNSCRAMBLE
 * PARTITIONS OF SIZE TWO.  THE POINTER ARRAY REQUIRES ADDITIONAL
 * PROCESSING BEFORE IT MAY BE USED!
 *
 **************************************/
	SORTP** stack_lower[50];
	SORTP*** sl = stack_lower;

	SORTP** stack_upper[50];
	SORTP*** su = stack_upper;

	*sl++ = pointers;
	*su++ = pointers + size - 1;

	while (sl > stack_lower)
	{

		// Pick up the next interval off the respective stacks

		SORTP** r = *--sl;
		SORTP** j = *--su;

		// Compute the interval. If two or less, defer the sort to a final pass.

		const SLONG interval = j - r;
		if (interval < 2)
			continue;

		// Go guard against pre-ordered data, swap the first record with the
		// middle record. This isn't perfect, but it is cheap.

		SORTP** i = r + interval / 2;
		swap(i, r);

		// Prepare to do the partition. Pick up the first longword of the
		// key to speed up comparisons.

		i = r + 1;
		const ULONG key = **r;

		// From each end of the interval converge to the middle swapping out of
		// parition records as we go. Stop when we converge.

		while (true)
		{
			while (**i < key)
				i++;
			if (**i == key)
				while (i <= *su)
				{
					const SORTP* p = *i;
					const SORTP* q = *r;
					ULONG tl = length - 1;
					while (tl && *p == *q)
					{
						p++;
						q++;
						tl--;
					}
					if (tl && *p > *q)
						break;
					i++;
				}

			while (**j > key)
				j--;
			if (**j == key)
				while (j != r)
				{
					const SORTP* p = *j;
					const SORTP* q = *r;
					ULONG tl = length - 1;
					while (tl && *p == *q)
					{
						p++;
						q++;
						tl--;
					}
					if (tl && *p < *q)
						break;
					j--;
				}
			if (i >= j)
				break;
			swap(i, j);
			i++;
			j--;
		}

		// We have formed two partitions, separated by a slot for the
		// initial record "r". Exchange the record currently in the
		// slot with "r".

		swap(r, j);

		// Finally, stack the two intervals, longest first

		i = *su;
		if ((j - r) > (i - j + 1))
		{
			*sl++ = r;
			*su++ = j - 1;
			*sl++ = j + 1;
			*su++ = i;
		}
		else
		{
			*sl++ = j + 1;
			*su++ = i;
			*sl++ = r;
			*su++ = j - 1;
		}
	}
}


ULONG Sort::order()
{
/**************************************
 *
 * The memoryfull of record pointers have been sorted, but more
 * records remain, so the run will have to be written to disk.  To
 * speed this up, re-arrange the records in physical order so they
 * can be written with a single disk write.
 *
 **************************************/
	sort_record** ptr = m_first_pointer + 1;	// 1st ptr is low key

	// Last inserted record, also the top of the memory where SORT_RECORDS can
	// be written
	sort_record* output = reinterpret_cast<sort_record*>(m_last_record);
	sort_ptr_t* lower_limit = reinterpret_cast<sort_ptr_t*>(output);

	HalfStaticArray<ULONG, 1024> record_buffer(m_owner->getPool());
	SORTP* buffer = record_buffer.getBuffer(m_longs);

	// Length of the key part of the record
	const ULONG length = m_longs - SIZEOF_SR_BCKPTR_IN_LONGS;

	// m_next_pointer points to the end of pointer memory or the beginning of
	// records
	while (ptr < m_next_pointer)
	{
		// If the next pointer is null, it's record has been eliminated as a
		// duplicate. This is the only easy case.

		SR* record = reinterpret_cast<SR*>(*ptr++);
		if (!record)
			continue;

		// Make record point back to the starting of SR struct,
		// as all scb* pointer point to the key_id locations!
		record = reinterpret_cast<SR*>(((SORTP*) record) - SIZEOF_SR_BCKPTR_IN_LONGS);

		// If the lower limit of live records points to a deleted or used record,
		// advance the lower limit

		while (!*(lower_limit) && (lower_limit < (sort_ptr_t*) m_end_memory))
		{
			lower_limit = reinterpret_cast<sort_ptr_t*>(((SORTP*) lower_limit) + m_longs);
		}

		// If the record we want to move won't interfere with lower active
		// record, just move the record into position

		if (record->sr_sort_record.sort_record_key == (ULONG*) lower_limit)
		{
			MOVE_32(length, record->sr_sort_record.sort_record_key, output);
			output = reinterpret_cast<sort_record*>((SORTP*) output + length);
			continue;
		}

		if (((SORTP*) output) + m_longs - 1 <= (SORTP*) lower_limit)
		{
			// null the bckptr for this record
			record->sr_bckptr = NULL;
			MOVE_32(length, record->sr_sort_record.sort_record_key, output);
			output = reinterpret_cast<sort_record*>((SORTP*) output + length);
			continue;
		}

		// There's another record sitting where we want to put our record. Move
		// the next logical record to a temp, move the lower limit record to the
		// next record's old position (adjusting pointers as we go), then move
		// the current record to output.

		MOVE_32(length, (SORTP*) record->sr_sort_record.sort_record_key, buffer);

		**((sort_ptr_t***) lower_limit) =
			reinterpret_cast<sort_ptr_t*>(record->sr_sort_record.sort_record_key);
		MOVE_32(m_longs, lower_limit, record);
		lower_limit = (sort_ptr_t*) ((SORTP*) lower_limit + m_longs);

		MOVE_32(length, buffer, output);
		output = reinterpret_cast<sort_record*>((sort_ptr_t*) ((SORTP*) output + length));
	}

	return (((SORTP*) output) -
			((SORTP*) m_last_record)) / (m_longs - SIZEOF_SR_BCKPTR_IN_LONGS);
}


void Sort::orderAndSave()
{
/**************************************
 *
 * The memory full of record pointers has been sorted, but more
 * records remain, so the run will have to be written to scratch file.
 * If target run can be allocated in contiguous chunk of memory then
 * just memcpy records into it. Else call more expensive order() to
 * physically rearrange records in sort space and write its run into
 * scratch file as one big chunk
 *
 **************************************/
	run_control* run = m_runs;
	run->run_records = 0;

	sort_record** ptr = m_first_pointer + 1; // 1st ptr is low key
	// m_next_pointer points to the end of pointer memory or the beginning of records
	while (ptr < m_next_pointer)
	{
		// If the next pointer is null, it's record has been eliminated as a
		// duplicate.  This is the only easy case.
		if (!(*ptr++))
			continue;

		run->run_records++;
	}

	const ULONG key_length = (m_longs - SIZEOF_SR_BCKPTR_IN_LONGS) * sizeof(ULONG);
	run->run_size = run->run_records * key_length;
	run->run_seek = m_space->allocateSpace(run->run_size);

	UCHAR* mem = m_space->inMemory(run->run_seek, run->run_size);

	if (mem)
	{
		ptr = m_first_pointer + 1;
		while (ptr < m_next_pointer)
		{
			SR* record = (SR*) (*ptr++);

			if (!record)
				continue;

			// make record point back to the starting of SR struct.
			// as all m_*_pointer point to the key_id locations!
			record = (SR*) (((SORTP*)record) - SIZEOF_SR_BCKPTR_IN_LONGS);

			memcpy(mem, record->sr_sort_record.sort_record_key, key_length);
			mem += key_length;
		}
	}
	else
	{
		order();
		writeBlock(m_space, run->run_seek, (UCHAR*) m_last_record, run->run_size);
	}
}


void Sort::putRun()
{
/**************************************
 *
 * Memory has been exhausted.  Do a sort on what we have and write
 * it to the scratch file.  Keep in mind that since duplicate records
 * may disappear, the number of records in the run may be less than
 * were sorted.
 *
 **************************************/
	run_control* run = m_free_runs;

	if (run) {
		m_free_runs = run->run_next;
	}
	else {
		run = (run_control*) FB_NEW(m_owner->getPool()) run_control;
	}
	memset(run, 0, sizeof(run_control));

	run->run_next = m_runs;
	m_runs = run;
	run->run_header.rmh_type = RMH_TYPE_RUN;
	run->run_depth = 0;

	// Do the in-core sort. The first phase a duplicate handling we be performed
	// in "sort".

	sort();

	// Re-arrange records in physical order so they can be dumped in a single write
	// operation

	orderAndSave();
}


void Sort::sort()
{
/**************************************
 *
 * Set up for and call quick sort.  Quicksort, by design, doesn't
 * order partitions of length 2, so make a pass thru the data to
 * straighten out pairs.  While we at it, if duplicate handling has
 * been requested, detect and handle them.
 *
 **************************************/

	// First, insert a pointer to the high key

	*m_next_pointer = reinterpret_cast<sort_record*>(high_key);

	// Next, call QuickSort. Keep in mind that the first pointer is the
	// low key and not a record.

	SORTP** j = (SORTP**) (m_first_pointer) + 1;
	const ULONG n = (SORTP**) (m_next_pointer) - j;	// calculate # of records

	quick(n, j, m_longs);

	// Scream through and correct any out of order pairs
	// hvlad: don't compare user keys against high_key
	while (j < (SORTP**) m_next_pointer - 1)
	{
		SORTP** i = j;
		j++;
		if (**i >= **j)
		{
			const SORTP* p = *i;
			const SORTP* q = *j;
			ULONG tl = m_longs - 1;
			while (tl && *p == *q)
			{
				p++;
				q++;
				tl--;
			}
			if (tl && *p > *q) {
				swap(i, j);
			}
		}
	}

	// If duplicate handling hasn't been requested, we're done

	if (!m_dup_callback)
		return;

	// Make another pass and eliminate duplicates. It's possible to do this
	// in the same pass the final ordering, but the logic is complicated enough
	// to screw up register optimizations. Better two fast passes than one
	// slow pass, I suppose. Prove me wrong and win a trip for two to
	// Cleveland, Ohio.

	j = reinterpret_cast<SORTP**>(m_first_pointer + 1);

	// hvlad: don't compare user keys against high_key
	while (j < ((SORTP**) m_next_pointer) - 1)
	{
		SORTP** i = j;
		j++;
		if (**i != **j)
			continue;
		const SORTP* p = *i;
		const SORTP* q = *j;

		ULONG l = m_unique_length;
		DO_32_COMPARE(p, q, l);
		if (l == 0)
		{
			diddleKey((UCHAR*) *i, false);
			diddleKey((UCHAR*) *j, false);

			if ((*m_dup_callback) ((const UCHAR*) *i, (const UCHAR*) *j, m_dup_callback_arg))
			{
				((SORTP***) (*i))[BACK_OFFSET] = NULL;
				*i = NULL;
			}
			else
			{
				diddleKey((UCHAR*) *i, true);
			}

			diddleKey((UCHAR*) *j, true);
		}
	}
}


void Sort::sortRunsBySeek(int n)
{
/**************************************
 *
 * Sort first n runs by its seek position in scratch file.
 * This allows to order file reads and make merge faster.
 *
 **************************************/

	SortedArray<RunSort, InlineStorage<RunSort, RUN_GROUP>, FB_UINT64, RunSort>
		runs(m_owner->getPool(), n);

	run_control* run;
	for (run = m_runs; run && n; run = run->run_next, n--) {
		runs.add(RunSort(run));
	}
	run_control* tail = run;

	RunSort* rs = runs.begin();
	run = m_runs = rs->run;
	for (rs++; rs < runs.end(); rs++)
	{
		run->run_next = rs->run;
		run = rs->run;
	}
	run->run_next = tail;
}
