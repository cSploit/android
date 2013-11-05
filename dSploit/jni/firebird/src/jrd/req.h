/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		req.h
 *	DESCRIPTION:	Request block definitions
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
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 */

#ifndef JRD_REQ_H
#define JRD_REQ_H

#include "../include/fb_blk.h"

#include "../jrd/exe.h"
#include "../jrd/sort.h"
#include "../jrd/RecordNumber.h"
#include "../common/classes/stack.h"
#include "../common/classes/timestamp.h"

namespace EDS {
class Statement;
}

namespace Jrd {

class Lock;
class Format;
class jrd_rel;
class jrd_prc;
class Record;
class jrd_nod;
class SaveRecordParam;
template <typename T> class vec;
class jrd_tra;
class Savepoint;
class RecordSource;
class thread_db;

// record parameter block

struct record_param
{
	record_param() :
		rpb_relation(0),
		rpb_window(DB_PAGE_SPACE, -1)
		{}
	RecordNumber rpb_number;		// record number in relation
	SLONG rpb_transaction_nr;		// transaction number
	jrd_rel*	rpb_relation;		// relation of record
	Record*		rpb_record;			// final record block
	Record*		rpb_prior;			// prior record block if this is a delta record
	SaveRecordParam*	rpb_copy;	// record_param copy for singleton verification
	Record*		rpb_undo;			// our first version of data if this is a second modification
	USHORT rpb_format_number;		// format number in relation

	SLONG rpb_page;					// page number
	USHORT rpb_line;				// line number on page

	SLONG rpb_f_page;				// fragment page number
	USHORT rpb_f_line;				// fragment line number on page

	SLONG rpb_b_page;				// back page
	USHORT rpb_b_line;				// back line

	UCHAR* rpb_address;				// address of record sans header
	USHORT rpb_length;				// length of record
	USHORT rpb_flags;				// record ODS flags replica
	USHORT rpb_stream_flags;		// stream flags
	SSHORT rpb_org_scans;			// relation scan count at stream open

	FB_UINT64 rpb_ext_pos;			// position in external file

	inline WIN& getWindow(thread_db* tdbb)
	{
		if (rpb_relation) {
			rpb_window.win_page.setPageSpaceID(rpb_relation->getPages(tdbb)->rel_pg_space_id);
		}

		return rpb_window;
	}

private:
	struct win rpb_window;
};

// Record flags must be an exact replica of ODS record header flags

const USHORT rpb_deleted	= 1;
const USHORT rpb_chained	= 2;
const USHORT rpb_fragment	= 4;
const USHORT rpb_incomplete	= 8;
const USHORT rpb_blob		= 16;
const USHORT rpb_delta		= 32;		// prior version is a differences record
const USHORT rpb_damaged	= 128;		// record is busted
const USHORT rpb_gc_active	= 256;		// garbage collecting dead record version
const USHORT rpb_uk_modified= 512;		// record key field values are changed

// Stream flags

const USHORT RPB_s_refetch	= 0x1;		// re-fetch required due to sort
const USHORT RPB_s_update	= 0x2;		// input stream fetched for update
const USHORT RPB_s_no_data	= 0x4;		// nobody is going to access the data

#define SET_NULL(record, id)	record->rec_data [id >> 3] |=  (1 << (id & 7))
#define CLEAR_NULL(record, id)	record->rec_data [id >> 3] &= ~(1 << (id & 7))
#define TEST_NULL(record, id)	record->rec_data [id >> 3] &   (1 << (id & 7))

const int MAX_DIFFERENCES	= 1024;	// Max length of generated Differences string
									// between two records

// Store allocation policy types.  Parameter to DPM_store()

const USHORT DPM_primary	= 1;	// New primary record
const USHORT DPM_secondary	= 2;	// Chained version of primary record
const USHORT DPM_other		= 3;	// Independent (or don't care) record

// Record block (holds data, remember data?)

class Record : public pool_alloc_rpt<SCHAR, type_rec>
{
public:
	explicit Record(MemoryPool& p) : rec_pool(p), rec_precedence(p) { }
	// ASF: Record is memcopied in realloc_record (vio.cpp), starting at rec_format.
	// rec_precedence has destructor, so don't move it to after rec_format.
	MemoryPool& rec_pool;		// pool where record to be expanded
	PageStack rec_precedence;	// stack of higher precedence pages
	const Format* rec_format;	// what the data looks like
	USHORT rec_length;			// how much there is
	const Format* rec_fmt_bk;   // backup format to cope with Borland's ill null signaling
	UCHAR rec_flags;			// misc record flags
	RecordNumber rec_number;	// original record_param number - used for undoing multiple updates
	double rec_dummy;			// this is to force next field to a double boundary
	UCHAR rec_data[1];			// THIS VARIABLE MUST BE ALIGNED ON A DOUBLE BOUNDARY
};

// rec_flags

const UCHAR REC_same_tx		= 1;	// record inserted/updated and deleted by same tx
const UCHAR REC_gc_active	= 2;	// relation garbage collect record block in use
const UCHAR REC_new_version	= 4;	// savepoint created new record version and deleted it

// save record_param block

class SaveRecordParam : public pool_alloc<type_srpb>
{
public:
	record_param srpb_rpb[1];		// record parameter blocks
};

// List of active blobs controlled by request

typedef Firebird::BePlusTree<ULONG, ULONG, MemoryPool> TempBlobIdTree;

// Affected rows counter class

class AffectedRows
{
public:
	AffectedRows();

	void clear();
	void bumpFetched();
	void bumpModified(bool);

	int getCount() const;

private:
	bool writeFlag;
	int fetchedRows;
	int modifiedRows;
};

// request block

class jrd_req : public pool_alloc_rpt<record_param, type_req>
{
public:
	jrd_req(MemoryPool* pool, Firebird::MemoryStats* parent_stats)
	:	req_pool(pool), req_memory_stats(parent_stats),
		req_blobs(pool), req_external(*pool), req_access(*pool), req_resources(*pool),
		req_trg_name(*pool), req_stats(*pool), req_base_stats(*pool), req_fors(*pool),
		req_exec_sta(*pool), req_ext_stmt(NULL), req_invariants(*pool),
		req_blr(*pool), req_domain_validation(NULL),
		req_map_field_info(*pool), req_map_item_info(*pool), req_auto_trans(*pool),
		req_sorts(*pool)
	{}

	Attachment*	req_attachment;			// database attachment
	SLONG		req_id;					// request identifier
	USHORT		req_count;				// number of streams
	USHORT		req_incarnation;		// incarnation number
	ULONG		req_impure_size;		// size of impure area
	MemoryPool* req_pool;
	Firebird::MemoryStats req_memory_stats;
	vec<jrd_req*>*	req_sub_requests;	// vector of sub-requests

	// Transaction pointer and doubly linked list pointers for requests in this
	// transaction. Maintained by TRA_attach_request/TRA_detach_request.
	jrd_tra*	req_transaction;
	jrd_req*	req_tra_next;
	jrd_req*	req_tra_prev;

	jrd_req*	req_request;			// next request in Database
	jrd_req*	req_caller;				// Caller of this request
										// This field may be used to reconstruct the whole call stack
	TempBlobIdTree req_blobs;			// Temporary BLOBs owned by this request
	ExternalAccessList req_external;	// Access to procedures/triggers to be checked
	AccessItemList req_access;			// Access items to be checked
	//vec<jrd_nod*>*	req_variables;	// Vector of variables, if any CVC: UNUSED
	ResourceList req_resources;			// Resources (relations and indices)
	jrd_nod*	req_message;			// Current message for send/receive
#ifdef SCROLLABLE_CURSORS
	jrd_nod*	req_async_message;		// Asynchronous message (used in scrolling)
#endif
	jrd_prc*	req_procedure;			// procedure, if any
	Firebird::MetaName	req_trg_name;	// name of request (trigger), if any
	//USHORT		req_length;			// message length for send/receive
	//USHORT		req_nmsgs;			// number of message types
	//USHORT		req_mmsg;			// highest message type
	//USHORT		req_msend;			// longest send message
	//USHORT		req_mreceive;		// longest receive message

	ULONG		req_records_selected;	// count of records selected by request (meeting selection criteria)
	ULONG		req_records_inserted;	// count of records inserted by request
	ULONG		req_records_updated;	// count of records updated by request
	ULONG		req_records_deleted;	// count of records deleted by request
	RuntimeStatistics	req_stats;
	RuntimeStatistics	req_base_stats;
	AffectedRows req_records_affected;	// records affected by the last statement

	USHORT req_view_flags;				// special flags for virtual ops on views
	jrd_rel* 	req_top_view_store;		// the top view in store(), if any
	jrd_rel*	req_top_view_modify;	// the top view in modify(), if any
	jrd_rel*	req_top_view_erase;		// the top view in erase(), if any

	jrd_nod*	req_top_node;			// top of execution tree
	jrd_nod*	req_next;				// next node for execution
	Firebird::Array<RecordSource*> req_fors;	// Vector of for loops, if any
	Firebird::Array<jrd_nod*>	req_exec_sta;	// Array of exec_into nodes
	EDS::Statement*	req_ext_stmt;		// head of list of active dynamic statements
	vec<RecordSource*>* 		req_cursors;	// Vector of named cursors, if any
	Firebird::Array<jrd_nod*>	req_invariants;	// Vector of invariant nodes, if any
	USHORT		req_label;				// label for leave
	ULONG		req_flags;				// misc request flags
	Savepoint*	req_proc_sav_point;		// procedure savepoint list
	Firebird::TimeStamp	req_timestamp;	// Start time of request
	Firebird::RefStrPtr req_sql_text;	// SQL text
	Firebird::Array<UCHAR> req_blr;		// BLR for non-SQL query

	Firebird::AutoPtr<Jrd::RuntimeStatistics> req_fetch_baseline; // State of request performance counters when we reported it last time
	SINT64 req_fetch_elapsed;	// Number of clock ticks spent while fetching rows for this request since we reported it last time
	SINT64 req_fetch_rowcount;	// Total number of rows returned by this request
	jrd_req* req_proc_caller;	// Procedure's caller request
	jrd_nod* req_proc_inputs;	// and its node with input parameters

	USHORT	req_src_line;
	USHORT	req_src_column;

	dsc*			req_domain_validation;	// Current VALUE for constraint validation
	MapFieldInfo	req_map_field_info;		// Map field name to field info
	MapItemInfo		req_map_item_info;		// Map item to item info
	Firebird::Stack<jrd_tra*> req_auto_trans;	// Autonomous transactions
	SortOwner		req_sorts;

	enum req_ta {
		// order should be maintained because the numbers are stored in BLR
		req_trigger_insert			= 1,
		req_trigger_update			= 2,
		req_trigger_delete			= 3,
		req_trigger_connect			= 4,
		req_trigger_disconnect		= 5,
		req_trigger_trans_start		= 6,
		req_trigger_trans_commit	= 7,
		req_trigger_trans_rollback	= 8
	} req_trigger_action;			// action that caused trigger to fire

	enum req_s {
		req_evaluate,
		req_return,
		req_receive,
		req_send,
		req_proceed,
		req_sync,
		req_unwind
	} req_operation;				// operation for next node

	StatusXcp req_last_xcp;			// last known exception

	record_param req_rpb[1];		// record parameter blocks

	void adjustCallerStats()
	{
		if (req_caller) {
			req_caller->req_stats.adjust(req_base_stats, req_stats);
		}
		req_base_stats.assign(req_stats);
	}
};

// Size of request without rpb items at the tail. Used to calculate impure area size
//
// 24-Mar-2004, Nickolay Samofatov.
// Note it may be not accurate on 64-bit RISC targets with 32-bit pointers due to
// alignment quirks, but from quick glance on code it looks like it should not be
// causing problems. Good fix for this kludgy behavior is to use some C++ means
// to manage impure area and array of record parameter blocks
const size_t REQ_SIZE = sizeof(jrd_req) - sizeof(jrd_req::blk_repeat_type);

// Flags for req_flags
const ULONG req_active			= 0x1L;
const ULONG req_stall			= 0x2L;
const ULONG req_leave			= 0x4L;
#ifdef SCROLLABLE_CURSORS
const ULONG req_async_processing= 0x8L;
#endif
const ULONG req_null			= 0x10L;
//const ULONG req_broken			= 0x20L;
const ULONG req_abort			= 0x40L;
const ULONG req_internal		= 0x80L;
const ULONG req_warning			= 0x100L;
const ULONG req_in_use			= 0x200L;
const ULONG req_sys_trigger		= 0x400L;		// request is a system trigger
//const ULONG req_count_records	= 0x800L;		// count records accessed
const ULONG req_proc_fetch		= 0x1000L;		// Fetch from procedure in progress
const ULONG req_ansi_any		= 0x2000L;		// Request is processing ANSI ANY
const ULONG req_same_tx_upd		= 0x4000L;		// record was updated by same transaction
const ULONG req_ansi_all		= 0x8000L;		// Request is processing ANSI ANY
const ULONG req_ansi_not		= 0x10000L;		// Request is processing ANSI ANY
const ULONG req_reserved		= 0x20000L;		// Request reserved for client
const ULONG req_ignore_perm		= 0x40000L;		// ignore permissions checks
const ULONG req_fetch_required	= 0x80000L;		// need to fetch next record
const ULONG req_error_handler	= 0x100000L;	// looper is called to handle error
const ULONG req_blr_version4	= 0x200000L;	// Request is of blr_version4

// Mask for flags preserved in a clone of a request
const ULONG REQ_FLAGS_CLONE_MASK = (req_sys_trigger | req_internal | req_ignore_perm | req_blr_version4);

// Mask for flags preserved on initialization of a request
const ULONG REQ_FLAGS_INIT_MASK = (req_in_use | req_internal | req_sys_trigger | req_ignore_perm | req_blr_version4);

// Flags for req_view_flags
enum {
	req_first_store_return = 0x1,
	req_first_modify_return = 0x2,
	req_first_erase_return = 0x4
};


// Index lock block

class IndexLock : public pool_alloc<type_idl>
{
public:
	IndexLock*	idl_next;		// Next index lock block for relation
	Lock*		idl_lock;		// Lock block
	jrd_rel*	idl_relation;	// Parent relation
	USHORT		idl_id;			// Index id
	USHORT		idl_count;		// Use count
};


} //namespace Jrd

#endif // JRD_REQ_H
