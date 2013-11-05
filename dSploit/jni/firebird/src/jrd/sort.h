/*
 *	PROGRAM:	JRD Sort
 *	MODULE:		sort.h
 *	DESCRIPTION:	Sort package definitions
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
 */

#ifndef JRD_SORT_H
#define JRD_SORT_H

#include "../jrd/common.h"
#include "../jrd/fil.h"

#include "../include/fb_blk.h"
#include "../jrd/TempSpace.h"

namespace Jrd {

// Forward declaration
struct sort_work_file;
class Attachment;
struct irsb_sort;
struct merge_control;
class SortOwner;

// SORTP is used throughout sort.c as a pointer into arrays of
// longwords(32 bits).
// Use this definition whenever doing pointer arithmetic, as
// Firebird variables (eg. scb->scb_longs) are in 32 - bit longwords.

typedef ULONG SORTP;

// since the first part of the record contains a back_pointer, whose
// size depends on the platform (ie 16, 32 or 64 bits.).
// This pointer data_type is defined by platform specific sort_ptr_t.

typedef IPTR sort_ptr_t;

#define PREV_RUN_RECORD(record) (((SORTP*) record - scb->scb_longs))
#define NEXT_RUN_RECORD(record) (((SORTP*) record + scb->scb_longs))

// a macro to goto the key_id part of a particular record.
// Pls. refer to the SR structure in sort.h for an explanation of the record structure.

#define KEYOF(record) ((SORTP*)(((SR*)record)->sr_sort_record.sort_record_key))

// macro to point to the next/previous  record for sorting.
// still using scb_longs as we cannot do record++ *
//#define PREV_RECORD(record) ((SR*)((SORTP*) record + scb->scb_longs))
#define NEXT_RECORD(record) ((SR*)((SORTP*) record - scb->scb_longs))

// structure containing the key and data part of the sort record,
// the back pointer is not written to the disk, this is how the records
// look in the run files.

struct sort_record
{
	ULONG sort_record_key[1];
	/* Sorting key.  Mangled by diddle_key to
	   compare using ULONG word compares (size
	   is rounded upwards if necessary).
	   Min length of 1 ULONG, max indeterminate
	   For current sort, length is stored as
	   scb_key_length.
	   Keys are created by BTR_* routines as
	   sequences of ULONG - this representation
	   cannot be easily changed. */

/*  sort_record_data  is here to explain the sort record.
    To get to the data part of a record add scb->scb_key_length to a pointer
    pointing to the start of the sort_record_key.

    ULONG       sort_record_data [1];
                                   Data values, not part of key,
                                   that are pumped through sort.
                                   Min length of 0, max indeterminate,
                                   byte data, but starts on ULONG boundary and
                                   rounded up to ULONG size
                                   Sizeof sr_data array would be
                                   (scb_longs - scb_key_length)*sizeof(ULONG) -
                                                 sizeof(sr_bckptr)
*/

};

const ULONG MAX_SORT_RECORD		= 65535;	// bytes

// the record struct actually contains the keyids etc, and the back_pointer
// which points to the sort_record structure.
typedef struct sr
{
	sort_record** sr_bckptr;	// Pointer back to sort list entry
	union {
		sort_record sr_sort_record;
		FB_UINT64 dummy_alignment_force;
	};
} SR;

// scb_longs includes the size of sr_bckptr.

/* The sort memory pool is laid out as follows during sorting:

struct sort_memory
{
        struct sr       *records [X1];
        ULONG           empty [X2];
        struct sr       data [X1];
};

We pack items into sort_memory, inserting the first pointer into
records [0], and the first data value into data[X1-1];  Continuing
until we are out of records to sort or memory.
(eg: X1*(sizeof(struct sr*) + scb->scb_longs) + X2*sizeof(ULONG) == MAX_MEMORY
*/


// Sort key definition block

struct sort_key_def
{
	UCHAR	skd_dtype;			// Data type
	UCHAR	skd_flags;			// Flags
	USHORT	skd_length;			// Length if string
	USHORT	skd_offset;			// Offset from beginning
	USHORT	skd_vary_offset;	// Offset to varying/cstring length
};


// skd_dtype

const int SKD_long			= 1;
const int SKD_ulong			= 2;
const int SKD_short			= 3;
const int SKD_ushort		= 4;
const int SKD_text			= 5;
const int SKD_float			= 6;
const int SKD_double		= 7;
const int SKD_quad			= 8;
const int SKD_timestamp1	= 9;		// Timestamp as Float
const int SKD_bytes			= 10;
const int SKD_varying		= 11;		// non-international
const int SKD_cstring		= 12;		// non-international

const int SKD_sql_time		= 13;
const int SKD_sql_date		= 14;
const int SKD_timestamp2	= 15;		// Timestamp as Quad

const int SKD_int64			= 16;

// Historical alias for pre V6 code
const int SKD_date	= SKD_timestamp1;

// skd_flags

const UCHAR SKD_ascending		= 0;	// default initializer
const UCHAR SKD_descending		= 1;
//const UCHAR SKD_insensitive	= 2;
const UCHAR SKD_binary			= 4;


// Run/merge common block header

struct run_merge_hdr
{
	SSHORT			rmh_type;
	merge_control*	rmh_parent;
};

// rmh_type

const int RMH_TYPE_RUN	= 0;
const int RMH_TYPE_MRG	= 1;


// Run control block

struct run_control
{
	run_merge_hdr	run_header;
	run_control*	run_next;			// Next (actually last) run
	ULONG			run_records;		// Records (remaining) in run
#ifdef SCROLLABLE_CURSORS
	ULONG			run_max_records;	// total number of records in run
#endif
	USHORT			run_depth;			// Number of "elementary" runs
	FB_UINT64		run_seek;			// Offset in file of run
	FB_UINT64		run_size;			// Length of run in work file
#ifdef SCROLLABLE_CURSORS
	FB_UINT64		run_cached;			// amount of cached data from run file
#endif
	sort_record*	run_record;			// Next record in run
	SORTP*			run_buffer;			// Run buffer
	SORTP*			run_end_buffer;		// End of buffer
	bool			run_buff_alloc;		// Allocated buffer flag
	bool			run_buff_cache;		// run buffer is already in cache
	FB_UINT64		run_mem_seek;		// position of run's buffer in in-memory part of sort file
	ULONG			run_mem_size;		// size of run's buffer in in-memory part of sort file
};

// Merge control block

struct merge_control
{
	run_merge_hdr	mrg_header;
	sort_record*	mrg_record_a;
	run_merge_hdr*	mrg_stream_a;
	sort_record*	mrg_record_b;
	run_merge_hdr*	mrg_stream_b;
};


// Sort Context Block
// Context or Control???

// Used by SORT_init
typedef bool (*FPTR_REJECT_DUP_CALLBACK)(const UCHAR*, const UCHAR*, void*);

struct sort_context
{
	Database*			scb_dbb;			// Database
	SortOwner*			scb_owner;			// Sort owner
	SORTP*				scb_memory;			// ALLOC: Memory for sort
	SORTP*				scb_end_memory;		// End of memory
	ULONG				scb_size_memory;	// Bytes allocated
	SR*					scb_last_record;	// Address of last record
	sort_record**		scb_first_pointer;	// Memory for sort
	sort_record**		scb_next_pointer;	// Address for next pointer
#ifdef SCROLLABLE_CURSORS
	sort_record**		scb_last_pointer;	// Address for last pointer in block
#endif
	//USHORT			scb_length;			// Record length. Unused.
	USHORT				scb_longs;			// Length of record in longwords
	ULONG				scb_keys;			// Number of keys
	ULONG				scb_key_length;		// Key length
	ULONG				scb_unique_length;	// Unique key length, used when duplicates eliminated
	ULONG				scb_records;		// Number of records
	//FB_UINT64			scb_max_records;	// Maximum number of records to store. Unused.
	TempSpace*			scb_space;			// temporary space for scratch file
	run_control*		scb_runs;			// ALLOC: Run on scratch file, if any
	merge_control*		scb_merge;			// Top level merge block
	run_control*		scb_free_runs;		// ALLOC: Currently unused run blocks
	SORTP*				scb_merge_space;	// ALLOC: memory space to do merging
	ULONG				scb_flags;			// see flag bits below
	FPTR_REJECT_DUP_CALLBACK scb_dup_callback;	// Duplicate handling callback
	void*				scb_dup_callback_arg;	// Duplicate handling callback arg
	merge_control*		scb_merge_pool;		// ALLOC: pool of merge_control blocks
	sort_key_def		scb_description[1];
};

// flags as set in scb_flags

const int scb_initialized	= 1;
const int scb_sorted		= 2;	// stream has been sorted

#define SCB_LEN(n_k)	(sizeof (sort_context) + (SLONG)(n_k) * sizeof (sort_key_def))

class SortOwner
{
public:
	explicit SortOwner(MemoryPool& p)
		: pool(p), sorts(p)
	{}

	~SortOwner();

	void linkSort(sort_context* scb)
	{
		fb_assert(scb);

		if (!sorts.exist(scb))
		{
			sorts.add(scb);
		}
	}

	void unlinkSort(sort_context* scb)
	{
		fb_assert(scb);

		size_t pos;
		if (sorts.find(scb, pos))
		{
			sorts.remove(pos);
		}
	}

	MemoryPool& getPool() const
	{
		return pool;
	}

private:
	MemoryPool& pool;
	Firebird::SortedArray<sort_context*> sorts;
};

} //namespace Jrd

#endif // JRD_SORT_H
