/*
 *      PROGRAM:        JRD access method
 *      MODULE:         jrd.h
 *      DESCRIPTION:    Common descriptions
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * Claudio Valderrama C.
 * Adriano dos Santos Fernandes
 *
 */

#ifndef JRD_JRD_H
#define JRD_JRD_H

#include "../jrd/gdsassert.h"
#include "../jrd/common.h"
#include "../jrd/dsc.h"
#include "../jrd/btn.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/val.h"

#include "../common/classes/fb_atomic.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/array.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/stack.h"
#include "../common/classes/timestamp.h"
#include "../common/classes/GenericMap.h"
#include "../common/utils_proto.h"
#include "../jrd/RandomGenerator.h"
#include "../jrd/os/guid.h"
#include "../jrd/sbm.h"
#include "../jrd/scl.h"

#ifdef DEV_BUILD
#define DEBUG                   if (debug) DBG_supervisor(debug);
//#define VIO_DEBUG				// remove this for production build
#else // PROD
#define DEBUG
#undef VIO_DEBUG
#endif

#define BUGCHECK(number)        ERR_bugcheck (number, __FILE__, __LINE__)
#define CORRUPT(number)         ERR_corrupt (number)
#define IBERROR(number)         ERR_error (number)


#define BLKCHK(blk, type)       if (!blk->checkHandle()) BUGCHECK(147)

#define DEV_BLKCHK(blk, type)	// nothing


// Thread data block / IPC related data blocks
#include "../jrd/ThreadData.h"

// recursive mutexes
#include "../common/thd.h"

// Definition of block types for data allocation in JRD
#include "../include/fb_blk.h"

#include "../jrd/blb.h"

// Definition of DatabasePlugins
#include "../jrd/flu.h"

#include "../jrd/pag.h"

#include "../jrd/RuntimeStatistics.h"
#include "../jrd/Database.h"

// Error codes
#include "../include/gen/iberror.h"

class str;
struct dsc;
struct thread;
struct mod;

namespace EDS {
	class Connection;
}

namespace Jrd {

const int QUANTUM			= 100;	// Default quantum
const int SWEEP_QUANTUM		= 10;	// Make sweeps less disruptive
const int MAX_CALLBACKS		= 50;

// fwd. decl.
class thread_db;
class Attachment;
class jrd_tra;
class jrd_req;
class Lock;
class jrd_file;
class Format;
class jrd_nod;
class BufferControl;
class SparseBitmap;
class jrd_rel;
class ExternalFile;
class ViewContext;
class IndexBlock;
class IndexLock;
class ArrayField;
class Symbol;
struct sort_context;
class RecordSelExpr;
class vcl;
class TextType;
class Parameter;
class jrd_fld;
class dsql_dbb;
class PreparedStatement;
class TraceManager;

// The database block, the topmost block in the metadata
// cache for a database

// Relation trigger definition

class Trigger
{
public:
	Firebird::HalfStaticArray<UCHAR, 128> blr;	// BLR code
	bid			dbg_blob_id;					// RDB$DEBUG_INFO
	jrd_req*	request;					// Compiled request. Gets filled on first invocation
	bool		compile_in_progress;
	bool		sys_trigger;
	UCHAR		type;						// Trigger type
	USHORT		flags;						// Flags as they are in RDB$TRIGGERS table
	jrd_rel*	relation;					// Trigger parent relation
	Firebird::MetaName	name;				// Trigger name
	void compile(thread_db*);				// Ensure that trigger is compiled
	void release(thread_db*);				// Try to free trigger request

	explicit Trigger(MemoryPool& p)
		: blr(p), name(p)
	{
		dbg_blob_id.clear();
	}
};



//
// Database attachments
//
const int DBB_read_seq_count		= 0;
const int DBB_read_idx_count		= 1;
const int DBB_update_count			= 2;
const int DBB_insert_count			= 3;
const int DBB_delete_count			= 4;
const int DBB_backout_count			= 5;
const int DBB_purge_count			= 6;
const int DBB_expunge_count			= 7;
const int DBB_max_count				= 8;

//
// Flags to indicate normal internal requests vs. dyn internal requests
//
const int IRQ_REQUESTS				= 1;
const int DYN_REQUESTS				= 2;


//
// Errors during validation - will be returned on info calls
// CVC: It seems they will be better in a header for val.cpp that's not val.h
//
const int VAL_PAG_WRONG_TYPE			= 0;
const int VAL_PAG_CHECKSUM_ERR			= 1;
const int VAL_PAG_DOUBLE_ALLOC			= 2;
const int VAL_PAG_IN_USE				= 3;
const int VAL_PAG_ORPHAN				= 4;
const int VAL_BLOB_INCONSISTENT			= 5;
const int VAL_BLOB_CORRUPT				= 6;
const int VAL_BLOB_TRUNCATED			= 7;
const int VAL_REC_CHAIN_BROKEN			= 8;
const int VAL_DATA_PAGE_CONFUSED		= 9;
const int VAL_DATA_PAGE_LINE_ERR		= 10;
const int VAL_INDEX_PAGE_CORRUPT		= 11;
const int VAL_P_PAGE_LOST				= 12;
const int VAL_P_PAGE_INCONSISTENT		= 13;
const int VAL_REC_DAMAGED				= 14;
const int VAL_REC_BAD_TID				= 15;
const int VAL_REC_FRAGMENT_CORRUPT		= 16;
const int VAL_REC_WRONG_LENGTH			= 17;
const int VAL_INDEX_ROOT_MISSING		= 18;
const int VAL_TIP_LOST					= 19;
const int VAL_TIP_LOST_SEQUENCE			= 20;
const int VAL_TIP_CONFUSED				= 21;
const int VAL_REL_CHAIN_ORPHANS			= 22;
const int VAL_INDEX_MISSING_ROWS		= 23;
const int VAL_INDEX_ORPHAN_CHILD		= 24;
const int VAL_INDEX_CYCLE				= 25;
const int VAL_MAX_ERROR					= 26;


struct DSqlCacheItem
{
	Lock* lock;
	bool locked;
	bool obsolete;
};

typedef Firebird::GenericMap<Firebird::Pair<Firebird::Left<
	Firebird::string, DSqlCacheItem> > > DSqlCache;


//
// the attachment block; one is created for each attachment to a database
//
class Attachment : public pool_alloc<type_att>, public Firebird::PublicHandle
{
public:
	static Attachment* create(Database* dbb)
	{
		MemoryPool* const pool = dbb->createPool();

		try
		{
			Attachment* const attachment = FB_NEW(*pool) Attachment(pool, dbb);
			pool->setStatsGroup(attachment->att_memory_stats);
			return attachment;
		}
		catch (const Firebird::Exception&)
		{
			dbb->deletePool(pool);
			throw;
		}
	}

	static void destroy(Attachment* const attachment)
	{
		if (attachment)
		{
			Database* const dbb = attachment->att_database;
			MemoryPool* const pool = attachment->att_pool;
			Firebird::MemoryStats temp_stats;
			pool->setStatsGroup(temp_stats);
			delete attachment;
			dbb->deletePool(pool);
		}
	}

/*	Attachment()
	:	att_database(0),
		att_next(0),
		att_blocking(0),
		att_user(0),
		att_transactions(0),
		att_dbkey_trans(0),
		att_requests(0),
		att_active_sorts(0),
		att_id_lock(0),
		att_attachment_id(0),
		att_lock_owner_handle(0),
		att_event_session(0),
		att_security_class(0),
		att_security_classes(0),
		att_flags(0),
		att_charset(0),
		att_long_locks(0),
		att_compatibility_table(0),
		att_val_errors(0),
		att_working_directory(0), 
		att_fini_sec_db(false)
	{
		att_counts[0] = 0;
	}*/

	MemoryPool* const att_pool;					// Memory pool
	Firebird::MemoryStats att_memory_stats;

	Database*	att_database;				// Parent database block
	Attachment*	att_next;					// Next attachment to database
	UserId*		att_user;					// User identification
	jrd_tra*	att_transactions;			// Transactions belonging to attachment
	jrd_tra*	att_dbkey_trans;			// transaction to control db-key scope
	SLONG		att_oldest_snapshot;		// GTT's record versions older than this can be gargage-collected

	jrd_req*	att_requests;				// Requests belonging to attachment
	sort_context*	att_active_sorts;		// Active sorts
	Lock*		att_id_lock;				// Attachment lock (if any)
	SLONG		att_attachment_id;			// Attachment ID
	const ULONG	att_lock_owner_id;			// ID for the lock manager
	SLONG		att_lock_owner_handle;		// Handle for the lock manager
	ULONG		att_backup_state_counter;	// Counter of backup state locks for attachment
	SLONG		att_event_session;			// Event session id, if any
	SecurityClass*	att_security_class;		// security class for database
	SecurityClassList*	att_security_classes;	// security classes
	vcl*		att_counts[DBB_max_count];
	RuntimeStatistics	att_stats;
	ULONG		att_flags;					// Flags describing the state of the attachment
	SSHORT		att_charset;				// user's charset specified in dpb
	Lock*		att_long_locks;				// outstanding two phased locks
	Lock*		att_wait_lock;				// lock at which attachment waits currently
	vec<Lock*>*	att_compatibility_table;	// hash table of compatible locks
	vcl*		att_val_errors;
	Firebird::PathName	att_working_directory;	// Current working directory is cached
	Firebird::PathName	att_filename;			// alias used to attach the database
	const Firebird::TimeStamp	att_timestamp;	// Connection date and time
	Firebird::StringMap att_context_vars;	// Context variables for the connection
	Firebird::string att_network_protocol;	// Network protocol used by client for connection
	Firebird::string att_remote_address;	// Protocol-specific addess of remote client
	SLONG att_remote_pid;					// Process id of remote client
	Firebird::PathName att_remote_process;	// Process name of remote client
	RandomGenerator att_random_generator;	// Random bytes generator
#ifndef SUPERSERVER
	Lock*		att_temp_pg_lock;			// temporary pagespace ID lock
#endif
	DSqlCache att_dsql_cache;	// DSQL cache locks
	Firebird::SortedArray<void*> att_udf_pointers;
	dsql_dbb* att_dsql_instance;
	Firebird::Mutex att_mutex;				// attachment mutex

	EDS::Connection* att_ext_connection;	// external connection executed by this attachment
	ULONG att_ext_call_depth;				// external connection call depth, 0 for user attachment
	TraceManager* att_trace_manager;		// Trace API manager
	bool att_fini_sec_db;
	Firebird::string att_requested_role;	// Role requested by user

	bool locksmith() const;

	PreparedStatement* prepareStatement(thread_db* tdbb, Firebird::MemoryPool& pool,
		jrd_tra* transaction, const Firebird::string& text);

	void cancelExternalConnection(thread_db* tdbb);

	bool backupStateWriteLock(thread_db* tdbb, SSHORT wait);
	void backupStateWriteUnLock(thread_db* tdbb);
	bool backupStateReadLock(thread_db* tdbb, SSHORT wait);
	void backupStateReadUnLock(thread_db* tdbb);

	bool checkHandle() const
	{
		if (!isKnownHandle())
		{
			return false;
		}

		return TypedHandle<type_att>::checkHandle();
	}

private:
	Attachment(MemoryPool* pool, Database* dbb);
	~Attachment();
};


// Attachment flags

const ULONG ATT_no_cleanup			= 1;	// Don't expunge, purge, or garbage collect
const ULONG ATT_shutdown			= 2;	// attachment has been shutdown
const ULONG ATT_purge_error			= 4;	// trouble happened in purge attachment, att_mutex remains locked
const ULONG ATT_shutdown_manager	= 8;	// attachment requesting shutdown
const ULONG ATT_lck_init_done		= 16;	// LCK_init() called for the attachment
const ULONG ATT_exclusive			= 32;	// attachment wants exclusive database access
const ULONG ATT_attach_pending		= 64;	// Indicate attachment is only pending
const ULONG ATT_exclusive_pending	= 128;	// Indicate exclusive attachment pending
const ULONG ATT_gbak_attachment		= 256;	// Indicate GBAK attachment

#ifdef GARBAGE_THREAD
const ULONG ATT_notify_gc			= 1024;	// Notify garbage collector to expunge, purge ..
const ULONG ATT_disable_notify_gc	= 2048;	// Temporarily perform own garbage collection
const ULONG ATT_garbage_collector	= 4096;	// I'm a garbage collector

const ULONG ATT_NO_CLEANUP			= (ATT_no_cleanup | ATT_notify_gc);
#else
const ULONG ATT_NO_CLEANUP			= ATT_no_cleanup;
#endif

const ULONG ATT_cancel_raise		= 8192;		// Cancel currently running operation
const ULONG ATT_cancel_disable		= 16384;	// Disable cancel operations
const ULONG ATT_gfix_attachment		= 32768;	// Indicate a GFIX attachment
const ULONG ATT_gstat_attachment	= 65536;	// Indicate a GSTAT attachment
const ULONG ATT_no_db_triggers		= 131072;	// Don't execute database triggers


inline bool Attachment::locksmith() const
{
	return att_user && att_user->locksmith();
}



// Procedure block

class jrd_prc : public pool_alloc<type_prc>
{
public:
	USHORT prc_id;
	USHORT prc_flags;
	USHORT prc_inputs;
	USHORT prc_defaults;
	USHORT prc_outputs;
	//jrd_nod*	prc_input_msg;				// It's set once by met.epp and never used.
	jrd_nod*	prc_output_msg;
	Format*		prc_input_fmt;
	Format*		prc_output_fmt;
	Format*		prc_format;
	vec<Parameter*>*	prc_input_fields;	// vector of field blocks
	vec<Parameter*>*	prc_output_fields;	// vector of field blocks
	prc_t		prc_type;					// procedure type
	jrd_req*	prc_request;				// compiled procedure request
	USHORT prc_use_count;					// requests compiled with procedure
	SSHORT prc_int_use_count;				// number of procedures compiled with procedure, set and
											// used internally in the MET_clear_cache procedure
											// no code should rely on value of this field
											// (it will usually be 0)
	Lock* prc_existence_lock;				// existence lock, if any
	Firebird::MetaName prc_security_name;	// security class name for procedure
	Firebird::MetaName prc_name;			// ascic name
	USHORT prc_alter_count;					// No. of times the procedure was altered

public:
	explicit jrd_prc(MemoryPool& p)
		: prc_security_name(p), prc_name(p)
	{}
};

// prc_flags

const USHORT PRC_scanned			= 1;	// Field expressions scanned
const USHORT PRC_system				= 2;	// Set in met.epp, never tested.
const USHORT PRC_obsolete			= 4;	// Procedure known gonzo
const USHORT PRC_being_scanned		= 8;	// New procedure needs dependencies during scan
//const USHORT PRC_blocking			= 16;	// Blocking someone from dropping procedure
const USHORT PRC_create				= 32;	// Newly created. Set in met.epp, never tested or disabled.
const USHORT PRC_being_altered		= 64;	// Procedure is getting altered
									// This flag is used to make sure that MET_remove_procedure
									// does not delete and remove procedure block from cache
									// so dfw.epp:modify_procedure() can flip procedure body without
									// invalidating procedure pointers from other parts of metadata cache
const USHORT PRC_check_existence	= 128;	// Existence lock released

const USHORT MAX_PROC_ALTER			= 64;	// No. of times an in-cache procedure can be altered



// Parameter block

class Parameter : public pool_alloc<type_prm>
{
public:
	USHORT		prm_number;
	dsc			prm_desc;
	jrd_nod*	prm_default_value;
	Firebird::MetaName prm_name;			// asciiz name
//public:
	explicit Parameter(MemoryPool& p)
		: prm_name(p)
	{ }
};

// Index block to cache index information

class IndexBlock : public pool_alloc<type_idb>
{
public:
	IndexBlock*	idb_next;
	jrd_nod*	idb_expression;				// node tree for index expression
	jrd_req*	idb_expression_request;		// request in which index expression is evaluated
	dsc			idb_expression_desc;		// descriptor for expression result
	Lock*		idb_lock;					// lock to synchronize changes to index
	USHORT		idb_id;
};



// general purpose vector
template <class T, BlockType TYPE = type_vec>
class vec_base : protected pool_alloc<TYPE>
{
public:
	typedef typename Firebird::Array<T>::iterator iterator;
	typedef typename Firebird::Array<T>::const_iterator const_iterator;
	/*
	static vec_base* newVector(MemoryPool& p, int len)
	{
		return FB_NEW(p) vec_base<T, TYPE>(p, len);
	}
	static vec_base* newVector(MemoryPool& p, const vec_base& base)
	{
		return FB_NEW(p) vec_base<T, TYPE>(p, base);
	}
	*/

	size_t count() const { return v.getCount(); }
	T& operator[](size_t index) { return v[index]; }
	const T& operator[](size_t index) const { return v[index]; }

	iterator begin() { return v.begin(); }
	iterator end() { return v.end(); }

	const_iterator begin() const { return v.begin(); }
	const_iterator end() const { return v.end(); }

	void clear() { v.clear(); }

//	T* memPtr() { return &*(v.begin()); }
	T* memPtr() { return &v[0]; }

	void resize(size_t n, T val = T()) { v.resize(n, val); }

	void operator delete(void* mem) { MemoryPool::globalFree(mem); }

protected:
	vec_base(MemoryPool& p, int len)
		: v(p, len)
	{
		v.resize(len);
	}
	vec_base(MemoryPool& p, const vec_base& base)
		: v(p)
	{
		v = base.v;
	}

private:
	Firebird::Array<T> v;
};

template <typename T>
class vec : public vec_base<T, type_vec>
{
public:
	static vec* newVector(MemoryPool& p, int len)
	{
		return FB_NEW(p) vec<T>(p, len);
	}
	static vec* newVector(MemoryPool& p, const vec& base)
	{
		return FB_NEW(p) vec<T>(p, base);
	}
	static vec* newVector(MemoryPool& p, vec* base, int len)
	{
		if (!base)
			base = FB_NEW(p) vec<T>(p, len);
		else if (len > (int) base->count())
			base->resize(len);
		return base;
	}

private:
	vec(MemoryPool& p, int len) : vec_base<T, type_vec>(p, len) {}
	vec(MemoryPool& p, const vec& base) : vec_base<T, type_vec>(p, base) {}
};

class vcl : public vec_base<SLONG, type_vcl>
{
public:
	static vcl* newVector(MemoryPool& p, int len)
	{
		return FB_NEW(p) vcl(p, len);
	}
	static vcl* newVector(MemoryPool& p, const vcl& base)
	{
		return FB_NEW(p) vcl(p, base);
	}
	static vcl* newVector(MemoryPool& p, vcl* base, int len)
	{
		if (!base)
			base = FB_NEW(p) vcl(p, len);
		else if (len > (int) base->count())
			base->resize(len);
		return base;
	}

private:
	vcl(MemoryPool& p, int len) : vec_base<SLONG, type_vcl>(p, len) {}
	vcl(MemoryPool& p, const vcl& base) : vec_base<SLONG, type_vcl>(p, base) {}
};

//#define TEST_VECTOR(vector, number)      ((vector && number < vector->count()) ?
//					  (*vector)[number] : NULL)


//
// Transaction element block
//
struct teb
{
	Attachment** teb_database;
	int teb_tpb_length;
	const UCHAR* teb_tpb;
};

typedef teb TEB;

// Window block for loading cached pages into
// CVC: Apparently, the only possible values are HEADER_PAGE==0 and LOG_PAGE==2
// and reside in ods.h, although I watched a place with 1 and others with members
// of a struct.

struct win
{
	PageNumber win_page;
	Ods::pag* win_buffer;
	exp_index_buf* win_expanded_buffer;
	class BufferDesc* win_bdb;
	SSHORT win_scans;
	USHORT win_flags;
//	explicit win(SLONG wp) : win_page(wp), win_flags(0) {}
	explicit win(const PageNumber& wp)
		: win_page(wp), win_bdb(NULL), win_flags(0)
	{}
	win(const USHORT pageSpaceID, const SLONG pageNum)
		: win_page(pageSpaceID, pageNum), win_bdb(NULL), win_flags(0)
	{}
};

typedef win WIN;

// This is a compilation artifact: I wanted to be sure I would pick all old "win"
// declarations at the top, so "win" was built with a mandatory argument in
// the constructor. This struct satisfies a single place with an array. The
// alternative would be to initialize 16 elements of the array with 16 calls
// to the constructor: win my_array[n] = {win(-1), ... (win-1)};
// When all places are changed, this class can disappear and win's constructor
// may get the default value of -1 to "wp".
struct win_for_array: public win
{
	win_for_array()
		: win(DB_PAGE_SPACE, -1)
	{}
};

// win_flags

const USHORT WIN_large_scan			= 1;	// large sequential scan
const USHORT WIN_secondary			= 2;	// secondary stream
const USHORT WIN_garbage_collector	= 4;	// garbage collector's window
const USHORT WIN_garbage_collect	= 8;	// scan left a page for garbage collector


// Thread specific database block
class thread_db : public ThreadData
{
private:
	MemoryPool*	tdbb_default;
	void setDefaultPool(MemoryPool* p)
	{
		tdbb_default = p;
	}
	friend class Firebird::SubsystemContextPoolHolder <Jrd::thread_db, MemoryPool>;
	Database*	database;
	Attachment*	attachment;
	jrd_tra*	transaction;
	jrd_req*	request;
	RuntimeStatistics *reqStat, *traStat, *attStat, *dbbStat;

public:
	explicit thread_db(ISC_STATUS* status)
		: ThreadData(ThreadData::tddDBB)
	{
		tdbb_default = NULL;
		database = NULL;
		attachment = NULL;
		transaction = NULL;
		request = NULL;
		tdbb_quantum = QUANTUM;
		tdbb_flags = 0;
		tdbb_temp_traid = 0;
		tdbb_latch_count = 0;
		QUE_INIT(tdbb_latches);
		reqStat = traStat = attStat = dbbStat = RuntimeStatistics::getDummy();

		tdbb_status_vector = status;
		fb_utils::init_status(tdbb_status_vector);
	}

	~thread_db()
	{
		fb_assert(QUE_EMPTY(tdbb_latches));
		fb_assert(tdbb_latch_count == 0);
	}

	ISC_STATUS*	tdbb_status_vector;
	SSHORT		tdbb_quantum;		// Cycles remaining until voluntary schedule
	USHORT		tdbb_flags;

	SLONG		tdbb_temp_traid;	// current temporary table scope

	int			tdbb_latch_count;	// count of all latches held by thread
	que			tdbb_latches;		// shared latches held by thread

	MemoryPool* getDefaultPool()
	{
		return tdbb_default;
	}

	Database* getDatabase()
	{
		return database;
	}

	const Database* getDatabase() const
	{
		return database;
	}

	void setDatabase(Database* val)
	{
		database = val;
		dbbStat = val ? &val->dbb_stats : RuntimeStatistics::getDummy();
	}

	Attachment* getAttachment()
	{
		return attachment;
	}

	const Attachment* getAttachment() const
	{
		return attachment;
	}

	void setAttachment(Attachment* val)
	{
		attachment = val;
		attStat = val ? &val->att_stats : RuntimeStatistics::getDummy();
	}

	jrd_tra* getTransaction()
	{
		return transaction;
	}

	const jrd_tra* getTransaction() const
	{
		return transaction;
	}

	void setTransaction(jrd_tra* val);

	jrd_req* getRequest()
	{
		return request;
	}

	const jrd_req* getRequest() const
	{
		return request;
	}

	void setRequest(jrd_req* val);

	void bumpStats(const RuntimeStatistics::StatType index)
	{
		reqStat->bumpValue(index);
		traStat->bumpValue(index);
		attStat->bumpValue(index);
		dbbStat->bumpValue(index);
	}

	void bumpStats(const RuntimeStatistics::StatType index, SLONG relation_id)
	{
		reqStat->bumpValue(index, relation_id);
		//traStat->bumpValue(index, relation_id);
		//attStat->bumpValue(index, relation_id);
		//dbbStat->bumpValue(index, relation_id);
	}

	bool checkCancelState(bool punt);
};

// tdbb_flags

const USHORT TDBB_sweeper				= 1;	// Thread sweeper or garbage collector
const USHORT TDBB_no_cache_unwind		= 2;	// Don't unwind page buffer cache
const USHORT TDBB_prc_being_dropped		= 4;	// Dropping a procedure
const USHORT TDBB_backup_write_locked	= 8;    // BackupManager has write lock on LCK_backup_database
const USHORT TDBB_stack_trace_done		= 16;	// PSQL stack trace is added into status-vector
const USHORT TDBB_shutdown_manager		= 32;	// Server shutdown thread
const USHORT TDBB_dont_post_dfw			= 64;	// dont post DFW tasks as deferred work is performed now
const USHORT TDBB_sys_error				= 128;	// error shouldn't be handled by the looper
const USHORT TDBB_verb_cleanup			= 256;	// verb cleanup is in progress
const USHORT TDBB_use_db_page_space		= 512;	// use database (not temporary) page space in GTT operations
const USHORT TDBB_detaching				= 1024;	// detach is in progress
const USHORT TDBB_wait_cancel_disable	= 2048;	// don't cancel current waiting operation


class ThreadContextHolder
{
public:
	explicit ThreadContextHolder(ISC_STATUS* status = NULL)
		: context(status ? status : local_status)
	{
		context.putSpecific();
	}

	~ThreadContextHolder()
	{
		ThreadData::restoreSpecific();
	}

	thread_db* operator->()
	{
		return &context;
	}

	operator thread_db*()
	{
		return &context;
	}

private:
	// copying is prohibited
	ThreadContextHolder(const ThreadContextHolder&);
	ThreadContextHolder& operator= (const ThreadContextHolder&);

	ISC_STATUS_ARRAY local_status;
	thread_db context;
};


// CVC: This class was designed to restore the thread's default status vector automatically.
// In several places, tdbb_status_vector is replaced by a local temporary.
class ThreadStatusGuard
{
public:
	explicit ThreadStatusGuard(thread_db* tdbb)
		: m_tdbb(tdbb), m_old_status(tdbb->tdbb_status_vector)
	{
		fb_utils::init_status(m_local_status);
		m_tdbb->tdbb_status_vector = m_local_status;
	}

	~ThreadStatusGuard()
	{
		m_tdbb->tdbb_status_vector = m_old_status;
	}

	//ISC_STATUS* restore()
	//{
	//	return m_tdbb->tdbb_status_vector = m_old_status; // copy, not comparison
	//}

	operator ISC_STATUS*() { return m_local_status; }

	void copyToOriginal()
	{
		memcpy(m_old_status, m_local_status, sizeof(ISC_STATUS_ARRAY));
	}

private:
	thread_db* const m_tdbb;
	ISC_STATUS* const m_old_status;
	ISC_STATUS_ARRAY m_local_status;

	// copying is prohibited
	ThreadStatusGuard(const ThreadStatusGuard&);
	ThreadStatusGuard& operator=(const ThreadStatusGuard&);
};


// duplicate context of firebird string to store in jrd_nod::nod_arg
inline char* stringDup(MemoryPool& p, const Firebird::string& s)
{
	char* rc = (char*) p.allocate(s.length() + 1
#ifdef DEBUG_GDS_ALLOC
		, __FILE__, __LINE__
#endif
		);
	strcpy(rc, s.c_str());
	return rc;
}

inline char* stringDup(MemoryPool& p, const char* s, size_t l)
{
	char* rc = (char*) p.allocate(l + 1
#ifdef DEBUG_GDS_ALLOC
		, __FILE__, __LINE__
#endif
		);
	memcpy(rc, s, l);
	rc[l] = 0;
	return rc;
}

inline char* stringDup(MemoryPool& p, const char* s)
{
	if (! s)
	{
		return 0;
	}
	return stringDup(p, s, strlen(s));
}

// Used in string conversion calls
typedef Firebird::HalfStaticArray<UCHAR, 256> MoveBuffer;

} //namespace Jrd

// Random string block -- as long as impure areas don't have
// constructors and destructors, the need this varying string

class VaryingString : public pool_alloc_rpt<SCHAR, type_str>
{
public:
	USHORT str_length;
	UCHAR str_data[2];			// one byte for ALLOC and one for the NULL
};

// Threading macros

/* Define JRD_get_thread_data off the platform specific version.
 * If we're in DEV mode, also do consistancy checks on the
 * retrieved memory structure.  This was originally done to
 * track down cases of no "PUT_THREAD_DATA" on the NLM.
 *
 * This allows for NULL thread data (which might be an error by itself)
 * If there is thread data,
 * AND it is tagged as being a thread_db.
 * AND it has a non-NULL database field,
 * THEN we validate that the structure there is a database block.
 * Otherwise, we return what we got.
 * We can't always validate the database field, as during initialization
 * there is no database set up.
 */

#if defined(DEV_BUILD)
#include "../jrd/err_proto.h"

inline Jrd::thread_db* JRD_get_thread_data()
{
	ThreadData* p1 = ThreadData::getSpecific();
	if (p1 && p1->getType() == ThreadData::tddDBB)
	{
		Jrd::thread_db* p2 = (Jrd::thread_db*) p1;
		if (p2->getDatabase() && !p2->getDatabase()->checkHandle())
		{
			BUGCHECK(147);
		}
	}
	return (Jrd::thread_db*) p1;
}

inline void CHECK_TDBB(const Jrd::thread_db* tdbb)
{
	fb_assert(tdbb && (tdbb->getType() == ThreadData::tddDBB) &&
		(!tdbb->getDatabase() || tdbb->getDatabase()->checkHandle()));
}

inline void CHECK_DBB(const Jrd::Database* dbb)
{
	fb_assert(dbb->checkHandle());
}

#else // PROD_BUILD

inline Jrd::thread_db* JRD_get_thread_data()
{
	return (Jrd::thread_db*) ThreadData::getSpecific();
}

inline void CHECK_DBB(const Jrd::Database* dbb)
{
}

inline void CHECK_TDBB(const Jrd::thread_db* tdbb)
{
}

#endif

inline Jrd::Database* GET_DBB()
{
	return JRD_get_thread_data()->getDatabase();
}

/*-------------------------------------------------------------------------*
 * macros used to set thread_db and Database pointers when there are not set already *
 *-------------------------------------------------------------------------*/
inline void SET_TDBB(Jrd::thread_db* &tdbb)
{
	if (tdbb == NULL) {
		tdbb = JRD_get_thread_data();
	}
	CHECK_TDBB(tdbb);
}

inline void SET_DBB(Jrd::Database* &dbb)
{
	if (dbb == NULL) {
		dbb = GET_DBB();
	}
	CHECK_DBB(dbb);
}


// global variables for engine

extern int debug;

namespace Jrd {
	typedef Firebird::SubsystemContextPoolHolder <Jrd::thread_db, MemoryPool> ContextPoolHolder;
}

#endif // JRD_JRD_H
