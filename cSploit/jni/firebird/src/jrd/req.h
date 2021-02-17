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
#include "../jrd/Attachment.h"
#include "../jrd/JrdStatement.h"
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
class ValueListNode;
template <typename T> class vec;
class jrd_tra;
class Savepoint;
class RecordSource;
class Cursor;
class thread_db;

// record parameter block

struct record_param
{
	record_param()
		: rpb_transaction_nr(0), rpb_relation(0), rpb_record(NULL), rpb_prior(NULL),
		  rpb_undo(NULL), rpb_format_number(0),
		  rpb_page(0), rpb_line(0),
		  rpb_f_page(0), rpb_f_line(0),
		  rpb_b_page(0), rpb_b_line(0),
		  rpb_address(NULL), rpb_length(0), rpb_flags(0), rpb_stream_flags(0),
		  rpb_org_scans(0),
		  rpb_window(DB_PAGE_SPACE, -1)
	{
	}

	RecordNumber rpb_number;		// record number in relation
	TraNumber	rpb_transaction_nr;	// transaction number
	jrd_rel*	rpb_relation;		// relation of record
	Record*		rpb_record;			// final record block
	Record*		rpb_prior;			// prior record block if this is a delta record
	Record*		rpb_undo;			// our first version of data if this is a second modification
	USHORT rpb_format_number;		// format number in relation

	ULONG rpb_page;					// page number
	USHORT rpb_line;				// line number on page

	ULONG rpb_f_page;				// fragment page number
	USHORT rpb_f_line;				// fragment line number on page

	ULONG rpb_b_page;				// back page
	USHORT rpb_b_line;				// back line

	UCHAR* rpb_address;				// address of record sans header
	ULONG rpb_length;				// length of record
	USHORT rpb_flags;				// record ODS flags replica
	USHORT rpb_stream_flags;		// stream flags
	SSHORT rpb_org_scans;			// relation scan count at stream open

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
// rpb_large = 64 is missing
const USHORT rpb_damaged	= 128;		// record is busted
const USHORT rpb_gc_active	= 256;		// garbage collecting dead record version
const USHORT rpb_uk_modified= 512;		// record key field values are changed

// Stream flags

const USHORT RPB_s_refetch		= 0x01;	// re-fetch required due to sort
const USHORT RPB_s_update		= 0x02;	// input stream fetched for update
const USHORT RPB_s_no_data		= 0x04;	// nobody is going to access the data
const USHORT RPB_s_undo_data	= 0x08;	// data got from undo log
const USHORT RPB_s_sweeper		= 0x10;	// garbage collector - skip swept pages
const USHORT RPB_s_refetch_no_undo	= 0x20;	// re-fetch required due to modify\erase, don't use undo data

const unsigned int MAX_DIFFERENCES	= 1024;	// Max length of generated Differences string
											// between two records

// Record block (holds data, remember data?)

class Record : public pool_alloc_rpt<SCHAR, type_rec>
{
public:
	explicit Record(MemoryPool& p) : rec_pool(p), rec_precedence(p) { }
	// ASF: Record is memcopied in realloc_record (vio.cpp), starting at rec_format.
	// rec_precedence has destructor, so don't move it to after rec_format.
	MemoryPool& rec_pool;		// pool where record to be expanded
	PageStack rec_precedence;	// stack of higher precedence pages/transactions
	const Format* rec_format;	// what the data looks like
	ULONG rec_length;			// how much there is
	const Format* rec_fmt_bk;   // backup format to cope with Borland's ill null signaling
	UCHAR rec_flags;			// misc record flags
	RecordNumber rec_number;	// original record_param number - used for undoing multiple updates
	union
	{
		double rec_dummy;			// this is to force next field to a double boundary
		UCHAR rec_data[1];			// THIS VARIABLE MUST BE ALIGNED ON A DOUBLE BOUNDARY
	};

	void setNull(USHORT id)
	{
		rec_data[id >> 3] |= (1 << (id & 7));
	}

	void clearNull(USHORT id)
	{
		rec_data[id >> 3] &= ~(1 << (id & 7));
	}

	bool isNull(USHORT id) const
	{
		return ((rec_data[id >> 3] & (1 << (id & 7))) != 0);
	}

	void nullify()
	{
		// Zero the record buffer and initialize all fields to NULLs
		const size_t null_bytes = (rec_format->fmt_count + 7) >> 3;
		memset(rec_data, 0xFF, null_bytes);
		memset(rec_data + null_bytes, 0, rec_length - null_bytes);
	}
};

// rec_flags

const UCHAR REC_same_tx		= 1;	// record inserted/updated and deleted by same tx
const UCHAR REC_gc_active	= 2;	// relation garbage collect record block in use
const UCHAR REC_new_version	= 4;	// savepoint created new record version and deleted it

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

class jrd_req : public pool_alloc<type_req>
{
public:
	jrd_req(Attachment* attachment, /*const*/ JrdStatement* aStatement,
			Firebird::MemoryStats* parent_stats)
		: statement(aStatement),
		  req_pool(statement->pool),
		  req_memory_stats(parent_stats),
		  req_blobs(req_pool),
		  req_stats(*req_pool),
		  req_base_stats(*req_pool),
		  req_ext_stmt(NULL),
		  req_cursors(*req_pool),
		  req_ext_resultset(NULL),
		  req_domain_validation(NULL),
		  req_auto_trans(*req_pool),
		  req_sorts(*req_pool),
		  req_rpb(*req_pool),
		  impureArea(*req_pool)
	{
		fb_assert(statement);
		setAttachment(attachment);
		req_rpb = statement->rpbsSetup;
		impureArea.grow(statement->impureSize);
	}

	JrdStatement* getStatement()
	{
		return statement;
	}

	const JrdStatement* getStatement() const
	{
		return statement;
	}

	bool hasInternalStatement() const
	{
		return statement->flags & JrdStatement::FLAG_INTERNAL;
	}

	bool hasPowerfulStatement() const
	{
		return statement->flags & JrdStatement::FLAG_POWERFUL;
	}

	void setAttachment(Attachment* newAttachment)
	{
		req_attachment = newAttachment;
		charSetId = statement->flags & JrdStatement::FLAG_INTERNAL ?
			CS_METADATA : req_attachment->att_charset;
	}

private:
	JrdStatement* const statement;

public:
	MemoryPool* req_pool;
	Attachment*	req_attachment;			// database attachment
	SLONG		req_id;					// request identifier
	USHORT		req_incarnation;		// incarnation number
	Firebird::MemoryStats req_memory_stats;

	// Transaction pointer and doubly linked list pointers for requests in this
	// transaction. Maintained by TRA_attach_request/TRA_detach_request.
	jrd_tra*	req_transaction;
	jrd_req*	req_tra_next;
	jrd_req*	req_tra_prev;

	jrd_req*	req_caller;				// Caller of this request
										// This field may be used to reconstruct the whole call stack
	TempBlobIdTree req_blobs;			// Temporary BLOBs owned by this request
	const StmtNode*	req_message;		// Current message for send/receive

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

	const StmtNode*	req_next;			// next node for execution
	EDS::Statement*	req_ext_stmt;		// head of list of active dynamic statements
	Firebird::Array<const Cursor*>	req_cursors;	// named cursors
	ExtEngineManager::ResultSet*	req_ext_resultset;	// external result set
	USHORT		req_label;				// label for leave
	ULONG		req_flags;				// misc request flags
	Savepoint*	req_proc_sav_point;		// procedure savepoint list
	Firebird::TimeStamp	req_timestamp;	// Start time of request

	Firebird::AutoPtr<Jrd::RuntimeStatistics> req_fetch_baseline; // State of request performance counters when we reported it last time
	SINT64 req_fetch_elapsed;	// Number of clock ticks spent while fetching rows for this request since we reported it last time
	SINT64 req_fetch_rowcount;	// Total number of rows returned by this request
	jrd_req* req_proc_caller;	// Procedure's caller request
	const ValueListNode* req_proc_inputs;	// and its node with input parameters

	ULONG req_src_line;
	ULONG req_src_column;

	dsc*			req_domain_validation;	// Current VALUE for constraint validation
	Firebird::Stack<jrd_tra*> req_auto_trans;	// Autonomous transactions
	SortOwner req_sorts;
	Firebird::Array<record_param> req_rpb;	// record parameter blocks
	Firebird::Array<UCHAR> impureArea;		// impure area
	USHORT charSetId;						// "client" character set of the request

	enum req_ta {
		// Order should be maintained because the numbers are stored in BLR
		// and should be in sync with ExternalTrigger::Action.
		req_trigger_insert			= 1,
		req_trigger_update			= 2,
		req_trigger_delete			= 3,
		req_trigger_connect			= 4,
		req_trigger_disconnect		= 5,
		req_trigger_trans_start		= 6,
		req_trigger_trans_commit	= 7,
		req_trigger_trans_rollback	= 8,
		req_trigger_ddl				= 9
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

	template <typename T> T* getImpure(unsigned offset)
	{
		return reinterpret_cast<T*>(&impureArea[offset]);
	}

	void adjustCallerStats()
	{
		if (req_caller) {
			req_caller->req_stats.adjust(req_base_stats, req_stats);
		}
		req_base_stats.assign(req_stats);
	}
};

// Flags for req_flags
const ULONG req_active			= 0x1L;
const ULONG req_stall			= 0x2L;
const ULONG req_leave			= 0x4L;
const ULONG req_null			= 0x8L;
const ULONG req_abort			= 0x10L;
const ULONG req_error_handler	= 0x20L;		// looper is called to handle error
const ULONG req_warning			= 0x40L;
const ULONG req_in_use			= 0x80L;
const ULONG req_continue_loop	= 0x100L;		// PSQL continue statement
const ULONG req_proc_fetch		= 0x200L;		// Fetch from procedure in progress
const ULONG req_same_tx_upd		= 0x400L;		// record was updated by same transaction
const ULONG req_reserved		= 0x800L;		// Request reserved for client

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
