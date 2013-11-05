/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		rse.h
 *	DESCRIPTION:	Record source block definitions
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
 * 2001.07.28: Added rsb_t.rsb_skip to support LIMIT functionality.
 */

#ifndef JRD_RSE_H
#define JRD_RSE_H

// This is really funny: class rse is not defined here but in exe.h!!!

#include "../include/fb_blk.h"

#include "../common/classes/array.h"
#include "../jrd/constants.h"

#include "../jrd/dsc.h"
#include "../jrd/lls.h"
#include "../jrd/sbm.h"

#include "../jrd/RecordBuffer.h"

struct dsc;

namespace Jrd {

class jrd_req;
class jrd_rel;
class jrd_nod;
struct sort_key_def;
struct sort_work_file;
struct sort_context;
class CompilerScratch;
class jrd_prc;
class Format;
class VaryingStr;
class BtrPageGCLock;

// Record source block (RSB) types

enum rsb_t
{
	rsb_boolean,						// predicate (logical condition)
	rsb_cross,							// inner join as a nested loop
	rsb_first,							// retrieve first n records
	rsb_skip,							// skip n records
	rsb_indexed,						// access via an index
	rsb_merge,							// join via a sort merge
	rsb_sequential,						// natural scan access
	rsb_sort,							// sort
	rsb_union,							// union
	rsb_aggregate,						// aggregation
	rsb_ext_sequential,					// external sequential access
	rsb_ext_indexed,					// external indexed access
	rsb_ext_dbkey,						// external DB_KEY access
	rsb_navigate,						// navigational walk on an index
	rsb_left_cross,						// left outer join as a nested loop
	rsb_procedure,						// stored procedure
	rsb_virt_sequential,				// sequential access to a virtual table
	rsb_recursive_union					// Recursive union
};


// Array which stores relative pointers to impure areas of invariant nodes
typedef Firebird::SortedArray<SLONG> VarInvariantArray;

// Record source block

class RecordSource : public pool_alloc_rpt<RecordSource*, type_rsb>
{
public:
	RecordSource() : rsb_left_inner_streams(0),
		rsb_left_streams(0), rsb_left_rsbs(0) { }
	rsb_t rsb_type;						// type of rsb
	UCHAR rsb_stream;					// stream, if appropriate
	USHORT rsb_count;					// number of sub arguments
	USHORT rsb_flags;
	ULONG rsb_impure;					// offset to impure area
	ULONG rsb_cardinality;				// estimated cardinality of stream, unused for now.
	ULONG rsb_record_count;				// count of records returned from rsb (not candidate records processed)
	RecordSource* rsb_next;				// next rsb, if appropriate
	jrd_rel*	rsb_relation;			// relation, if appropriate
	VaryingString*	rsb_alias;			// SQL alias for relation
	jrd_prc*	rsb_procedure;			// procedure, if appropriate
	Format*		rsb_format;				// format, if appropriate
	jrd_nod*	rsb_any_boolean;		// any/all boolean

	// AP:	stop saving memory with the price of awful conversions,
	//		later may be union will help this, because no ~ are
	//		needed - pool destroyed as whole entity.
	StreamStack*	rsb_left_inner_streams;
	StreamStack*	rsb_left_streams;
	RsbStack*		rsb_left_rsbs;
	VarInvariantArray *rsb_invariants; /* Invariant nodes bound to top-level RSB */

	RecordSource* rsb_arg[1];
};

// bits for the rsb_flags field

const USHORT rsb_singular = 1;			// singleton select, expect 0 or 1 records
//const USHORT rsb_stream_type = 2;		// rsb is for stream type request, PC_ENGINE
const USHORT rsb_descending = 4;		// an ascending index is being used for a descending sort or vice versa
const USHORT rsb_project = 8;			// projection on this stream is requested
const USHORT rsb_writelock = 16;		// records should be locked for writing
const USHORT rsb_recursive = 32;		// this rsb is a sub_rsb of recursive rsb

// special argument positions within the RecordSource

const int RSB_PRC_inputs		= 0;
const int RSB_PRC_in_msg		= 1;
const int RSB_PRC_count			= 2;

const int RSB_NAV_index			= 0;
const int RSB_NAV_inversion		= 1;
const int RSB_NAV_key_length	= 2;
const int RSB_NAV_idx_offset	= 3;
const int RSB_NAV_count			= 4;

const int RSB_LEFT_outer		= 0;
const int RSB_LEFT_inner		= 1;
const int RSB_LEFT_boolean		= 2;
const int RSB_LEFT_inner_boolean	= 3;
const int RSB_LEFT_count			= 4;


// Merge (equivalence) file block

struct merge_file
{
	TempSpace*	mfb_space;				// merge file uses SORT I/O routines
	ULONG mfb_equal_records;			// equality group cardinality
	ULONG mfb_record_size;				// matches sort map length
	ULONG mfb_current_block;			// current merge block in buffer
	ULONG mfb_block_size;				// merge block I/O size
	ULONG mfb_blocking_factor;			// merge equality records per block
	UCHAR*	mfb_block_data;				// merge block I/O buffer
};

const ULONG MERGE_BLOCK_SIZE	= 65536;

// forward declaration
struct irsb_recurse;

class RSBRecurse
{
public:
	static void open(thread_db*, RecordSource*, irsb_recurse*);
	static bool get(thread_db*, RecordSource*, irsb_recurse*);
	static void close(thread_db*, RecordSource*, irsb_recurse*);

	enum mode {
		root,
		recurse
	};

	static const USHORT MAX_RECURSE_LEVEL;

private:
	static void cleanup_level(jrd_req*, RecordSource*, irsb_recurse*);
};

// Impure area formats for the various RecordSource types

struct irsb
{
	ULONG irsb_flags;
	USHORT irsb_count;
};

typedef irsb *IRSB;

struct irsb_recurse
{
	ULONG	irsb_flags;
	USHORT	irsb_level;
	RSBRecurse::mode irsb_mode;
	char*	irsb_stack;
	char*	irsb_data;
};

struct irsb_first_n
{
	ULONG irsb_flags;
    SINT64 irsb_count;
};

struct irsb_skip_n
{
    ULONG irsb_flags;
    SINT64 irsb_count;
};

struct irsb_index
{
	ULONG irsb_flags;
#ifdef SUPERSERVER_V2
	SLONG irsb_prefetch_number;
#endif
	RecordBitmap**	irsb_bitmap;
};

struct irsb_sort
{
	ULONG irsb_flags;
	sort_context*	irsb_sort_handle;
};

struct irsb_procedure
{
	ULONG 		irsb_flags;
	jrd_req*	irsb_req_handle;
	VaryingString*		irsb_message;
};

struct irsb_mrg
{
	ULONG irsb_flags;
	USHORT irsb_mrg_count;				// next stream in group
	struct irsb_mrg_repeat
	{
		SLONG irsb_mrg_equal;			// queue of equal records
		SLONG irsb_mrg_equal_end;		// end of the equal queue
		SLONG irsb_mrg_equal_current;	// last fetched record from equal queue
		SLONG irsb_mrg_last_fetched;	// first sort merge record of next group
		SSHORT irsb_mrg_order;			// logical merge order by substream
		merge_file irsb_mrg_file;		// merge equivalence file
	} irsb_mrg_rpt[1];
};

struct irsb_virtual
{
	ULONG irsb_flags;
	RecordBuffer* irsb_record_buffer;
};

/* CVC: Unused as of Nov-2005.
struct irsb_sim
{
	ULONG irsb_flags;
	USHORT irsb_sim_rid;				// next relation id
	USHORT irsb_sim_fid;				// next field id
	jrd_req *irsb_sim_req1;				// request handle #1
	jrd_req *irsb_sim_req2;				// request handle #2
};

const ULONG irsb_sim_alias = 32;		// duplicate relation but w/o user name
const ULONG irsb_sim_eos = 64;			// encountered end of stream
const ULONG irsb_sim_active = 128;		// remote simulated stream request is active
*/

// impure area format for navigational rsb type,
// which holds information used to get back to
// the current location within an index

struct irsb_nav
{
	ULONG irsb_flags;
	SLONG irsb_nav_expanded_offset;			// page offset of current index node on expanded index page
	RecordNumber irsb_nav_number;			// last record number
	SLONG irsb_nav_page;					// index page number
	SLONG irsb_nav_incarnation;				// buffer/page incarnation counter
	ULONG irsb_nav_count;					// record count of last record returned
	RecordBitmap**	irsb_nav_bitmap;			// bitmap for inversion tree
	RecordBitmap*	irsb_nav_records_visited;	// bitmap of records already retrieved
	BtrPageGCLock*	irsb_nav_btr_gc_lock;		// lock to prevent removal of currently walked index page
	USHORT irsb_nav_offset;					// page offset of current index node
	USHORT irsb_nav_lower_length;			// length of lower key value
	USHORT irsb_nav_upper_length;			// length of upper key value
	USHORT irsb_nav_length;					// length of expanded key
	UCHAR irsb_nav_data[1];					// expanded key, upper bound, and index desc
};

typedef irsb_nav *IRSB_NAV;

// flags for the irsb_flags field

const ULONG irsb_first = 1;
const ULONG irsb_joined = 2;				// set in left join when current record has been joined to something
const ULONG irsb_mustread = 4;				// set in left join when must read a record from left stream
const ULONG irsb_open = 8;					// indicated rsb is open
#ifdef SCROLLABLE_CURSORS
const ULONG irsb_backwards = 16;			// backwards navigation has been performed on this stream
#endif
const ULONG irsb_in_opened = 32;			// set in outer join when inner stream has been opened
const ULONG irsb_join_full = 64;			// set in full join when left join has completed
const ULONG irsb_checking_singular = 128;	// fetching to verify singleton select
const ULONG irsb_singular_processed = 256;	// singleton stream already delivered one record
#ifdef SCROLLABLE_CURSORS
const ULONG irsb_last_backwards = 512;		// rsb was last scrolled in the backward direction
const ULONG irsb_bof = 1024;				// rsb is at beginning of stream
const ULONG irsb_eof = 2048;				// rsb is at end of stream
#endif
const ULONG irsb_key_changed = 4096;		// key has changed since record last returned from rsb
// The below flag duplicates the one from the disabled SCROLLABLE_CURSORS
// implementation, but it's used slightly differently (at the top RSB levels).
// To be renamed if the SCROLLABLE_CURSORS code will ever be enabled.
const ULONG irsb_eof = 8192;				// rsb is at end of stream


// Sort map block

struct smb_repeat
{
	DSC smb_desc;				// relative descriptor
	USHORT smb_flag_offset;		// offset of missing flag
	USHORT smb_stream;			// stream for field id
	SSHORT smb_field_id;		// id for field (-1 if dbkey)
	jrd_nod*	smb_node;		// expression node
};

class SortMap : public pool_alloc_rpt<smb_repeat, type_smb>
{
public:
	USHORT smb_keys;			// number of keys
	USHORT smb_count;			// total number of fields
	USHORT smb_length;			// sort record length
	USHORT smb_key_length;		// key length in longwords
	sort_key_def* smb_key_desc;	// address of key descriptors
	USHORT smb_flags;			// misc sort flags
    smb_repeat smb_rpt[1];
};

// values for smb_field_id

const SSHORT SMB_DBKEY = -1;		// dbkey value
const SSHORT SMB_DBKEY_VALID = -2;	// dbkey valid flag
const SSHORT SMB_TRANS_ID = -3;		// transaction id of record

// bits for the smb_flags field

const USHORT SMB_project = 1;		// sort is really a project
//const USHORT SMB_tag = 2;			// beast is a tag sort. What is this?
const USHORT SMB_unique_sort = 4;	// sorts using unique key - for distinct and group by


// Blocks used to compute optimal join order:
// indexed relationships block (IRL) holds
// information about potential join orders

class IndexedRelationship : public pool_alloc<type_irl>
{
public:
	IndexedRelationship*	irl_next;		// next IRL block for stream
	USHORT					irl_stream;		// stream reachable by relation
	bool					irl_unique;		// is this stream reachable by unique index?
};



// Must be less then MAX_SSHORT. Not used for static arrays.
const int MAX_CONJUNCTS	= 32000;

// Note that MAX_STREAMS currently MUST be <= MAX_UCHAR.
// Here we should really have a compile-time fb_assert, since this hard-coded
// limit is NOT negotiable so long as we use an array of UCHAR, where index 0
// tells how many streams are in the array (and the streams themselves are
// identified by a UCHAR).
const int MAX_STREAMS	= 255;

// This is number of ULONG's needed to store bit-mapped flags for all streams
// OPT_STREAM_BITS = (MAX_STREAMS + 1) / sizeof(ULONG)
// This value cannot be increased simple way. Decrease is possible, but it is also
// hardcoded in several places such as TEST_DEP_ARRAYS macro
const int OPT_STREAM_BITS	= 8;

// Number of streams, conjuncts, indices that will be statically allocated
// in various arrays. Larger numbers will have to be allocated dynamically
const int OPT_STATIC_ITEMS = 16;


// General optimizer block

class OptimizerBlk : public pool_alloc<type_opt>
{
public:
	CompilerScratch*	opt_csb;					// compiler scratch block
	SLONG opt_combinations;					// number of partial orders considered
	double opt_best_cost;					// cost of best join order
	USHORT opt_best_count;					// longest length of indexable streams
	USHORT opt_base_conjuncts;				// number of conjuncts in our rse, next conjuncts are distributed parent
	USHORT opt_base_parent_conjuncts;		// number of conjuncts in our rse + distributed with parent, next are parent
	USHORT opt_base_missing_conjuncts;		// number of conjuncts in our and parent rse, but without missing
	//USHORT opt_g_flags;						// global flags
	// 01 Oct 2003. Nickolay Samofatov: this static array takes as much as 256 bytes.
	// This is nothing compared to original Firebird 1.5 OptimizerBlk structure size of ~180k
	// All other arrays had been converted to dynamic to preserve memory
	// and improve performance
	struct opt_segment
	{
		// Index segments and their options
		jrd_nod* opt_lower;			// lower bound on index value
		jrd_nod* opt_upper;			// upper bound on index value
		jrd_nod* opt_match;			// conjunct which matches index segment
	} opt_segments[MAX_INDEX_SEGMENTS];
	struct opt_conjunct
	{
		// Conjunctions and their options
		jrd_nod* opt_conjunct_node;	// conjunction
		// Stream dependencies to compute conjunct
		ULONG opt_dependencies[(MAX_STREAMS + 1) / 32];
		UCHAR opt_conjunct_flags;
	};
	struct opt_stream
	{
		// Streams and their options
		IndexedRelationship* opt_relationships;	// streams directly reachable by index
		double opt_best_stream_cost;			// best cost of retrieving first n = streams
		USHORT opt_best_stream;					// stream in best join order seen so far
		USHORT opt_stream_number;				// stream in position of join order
		UCHAR opt_stream_flags;
	};
	Firebird::HalfStaticArray<opt_conjunct, OPT_STATIC_ITEMS> opt_conjuncts;
	Firebird::HalfStaticArray<opt_stream, OPT_STATIC_ITEMS> opt_streams;
	OptimizerBlk(MemoryPool* pool) : opt_conjuncts(*pool), opt_streams(*pool) {}
};

// values for opt_stream_flags

const USHORT opt_stream_used = 1;			// stream is used

// values for opt_conjunct_flags

const USHORT opt_conjunct_used = 1;			// conjunct is used
const USHORT opt_conjunct_matched = 2;		// conjunct matches an index segment


// global optimizer bits used in opt_g_flags

// Obsolete: it was for PC_ENGINE
//const USHORT opt_g_stream = 1;				// indicate that this is a blr_stream


// River block - used to hold temporary information about a group of streams
// CVC: River is a "secret" of opt.cpp, maybe a new opt.h would be adequate.

class River : public pool_alloc_rpt<SCHAR, type_riv>
{
public:
	RecordSource* riv_rsb;				// record source block for river
	USHORT riv_number;			// temporary number for river
	UCHAR riv_count;			// count of streams
	UCHAR riv_streams[1];		// actual streams
};


// types for navigating through a stream

enum rse_get_mode
{
	RSE_get_forward
#ifdef SCROLLABLE_CURSORS
	,
	RSE_get_backward,
	RSE_get_current,
	RSE_get_first,
	RSE_get_last,
	RSE_get_next
#endif
};

} //namespace Jrd

#endif // JRD_RSE_H

