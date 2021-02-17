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

#include "../common/gdsassert.h"
#include "../common/dsc.h"
#include "../jrd/err_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/obj.h"
#include "../jrd/val.h"

#include "../common/classes/fb_atomic.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/NestConst.h"
#include "../common/classes/array.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/stack.h"
#include "../common/classes/timestamp.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/Synchronize.h"
#include "../common/utils_proto.h"
#include "../common/StatusHolder.h"
#include "../jrd/RandomGenerator.h"
#include "../common/os/guid.h"
#include "../jrd/sbm.h"
#include "../jrd/scl.h"
#include "../jrd/Routine.h"
#include "../jrd/ExtEngineManager.h"
#include "../jrd/Attachment.h"
#include "firebird/Provider.h"

#ifdef DEV_BUILD
//#define DEBUG                   if (debug) DBG_supervisor(debug);
#else // PROD
//#define DEBUG
#endif
#define DEBUG

#define BUGCHECK(number)        ERR_bugcheck (number, __FILE__, __LINE__)
#define CORRUPT(number)         ERR_corrupt (number)
#define IBERROR(number)         ERR_error (number)


#define BLKCHK(blk, type)       if (!blk->checkHandle()) BUGCHECK(147)

#define DEV_BLKCHK(blk, type)	do { } while (false)	// nothing


// Thread data block / IPC related data blocks
#include "../common/ThreadData.h"

// Definition of block types for data allocation in JRD
#include "../include/fb_blk.h"

#include "../jrd/blb.h"

// Definition of DatabasePlugins
#include "../jrd/flu.h"

#include "../jrd/pag.h"

#include "../jrd/RuntimeStatistics.h"
#include "../jrd/Database.h"
#include "../jrd/lck.h"

// Error codes
#include "gen/iberror.h"

struct dsc;

namespace EDS {
	class Connection;
}

namespace Jrd {

const int QUANTUM				= 100;	// Default quantum
const int SWEEP_QUANTUM			= 10;	// Make sweeps less disruptive
const unsigned MAX_CALLBACKS	= 50;

// fwd. decl.
class thread_db;
class Attachment;
class jrd_tra;
class jrd_req;
class JrdStatement;
class jrd_file;
class Format;
class BufferDesc;
class SparseBitmap;
class jrd_rel;
class ExternalFile;
class ViewContext;
class IndexBlock;
class IndexLock;
class ArrayField;
struct sort_context;
class vcl;
class TextType;
class Parameter;
class jrd_fld;
class dsql_dbb;
class PreparedStatement;
class TraceManager;
class MessageNode;

// The database block, the topmost block in the metadata
// cache for a database

// Relation trigger definition

class Trigger
{
public:
	Firebird::HalfStaticArray<UCHAR, 128> blr;	// BLR code
	bid			dbg_blob_id;					// RDB$DEBUG_INFO
	JrdStatement* statement;			// Compiled statement
	bool		compile_in_progress;
	bool		sys_trigger;
	FB_UINT64	type;						// Trigger type
	USHORT		flags;						// Flags as they are in RDB$TRIGGERS table
	jrd_rel*	relation;					// Trigger parent relation
	Firebird::MetaName	name;				// Trigger name
	Firebird::MetaName	engine;				// External engine name
	Firebird::string	entryPoint;			// External trigger entrypoint
	Firebird::string	extBody;			// External trigger body
	ExtEngineManager::Trigger* extTrigger;	// External trigger

	void compile(thread_db*);				// Ensure that trigger is compiled
	void release(thread_db*);				// Try to free trigger request

	explicit Trigger(MemoryPool& p)
		: blr(p),
		  name(p),
		  engine(p),
		  entryPoint(p),
		  extBody(p),
		  extTrigger(NULL)
	{
		dbg_blob_id.clear();
	}
};


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
const int VAL_SCNS_PAGE_INCONSISTENT	= 26;
const int VAL_PAG_WRONG_SCN				= 27;
const int VAL_BLOB_UNKNOWN_LEVEL		= 28;
const int VAL_INDEX_BAD_LEFT_SIBLING	= 29;
const int VAL_INDEX_MISSES_NODE			= 30;
const int VAL_MAX_ERROR					= 31;


// Procedure block

class jrd_prc : public Routine
{
public:
	const Format*	prc_record_format;
	prc_t		prc_type;					// procedure type

	const ExtEngineManager::Procedure* getExternal() const { return prc_external; }
	void setExternal(ExtEngineManager::Procedure* value) { prc_external = value; }

private:
	const ExtEngineManager::Procedure* prc_external;

public:
	explicit jrd_prc(MemoryPool& p)
		: Routine(p),
		  prc_record_format(NULL),
		  prc_type(prc_legacy),
		  prc_external(NULL)
	{
	}

public:
	virtual int getObjectType() const
	{
		return obj_procedure;
	}

	virtual SLONG getSclType() const
	{
		return SCL_object_procedure;
	}

	virtual void releaseFormat()
	{
		delete prc_record_format;
		prc_record_format = NULL;
	}

	virtual bool checkCache(thread_db* tdbb) const;
	virtual void clearCache(thread_db* tdbb);
};


// Parameter block

class Parameter : public pool_alloc<type_prm>
{
public:
	USHORT		prm_number;
	dsc			prm_desc;
	NestConst<ValueExprNode>	prm_default_value;
	bool		prm_nullable;
	prm_mech_t	prm_mechanism;
	Firebird::MetaName prm_name;			// asciiz name
	Firebird::MetaName prm_field_source;
	FUN_T		prm_fun_mechanism;

public:
	explicit Parameter(MemoryPool& p)
		: prm_name(p),
		  prm_field_source(p)
	{
	}
};

// Index block to cache index information

class IndexBlock : public pool_alloc<type_idb>
{
public:
	IndexBlock*	idb_next;
	ValueExprNode* idb_expression;			// node tree for index expression
	JrdStatement* idb_expression_statement;	// statement for index expression evaluation
	dsc			idb_expression_desc;		// descriptor for expression result
	Lock*		idb_lock;					// lock to synchronize changes to index
	USHORT		idb_id;
};


//
// Transaction element block
//
struct teb
{
	Attachment* teb_database;
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
	class BufferDesc* win_bdb;
	SSHORT win_scans;
	USHORT win_flags;
	explicit win(const PageNumber& wp)
		: win_page(wp), win_bdb(NULL), win_flags(0)
	{}
	win(const USHORT pageSpaceID, const ULONG pageNum)
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
// may get the default value of ~0 to "wp".
struct win_for_array: public win
{
	win_for_array()
		: win(DB_PAGE_SPACE, ~0)
	{}
};

// win_flags

const USHORT WIN_large_scan			= 1;	// large sequential scan
const USHORT WIN_secondary			= 2;	// secondary stream
const USHORT WIN_garbage_collector	= 4;	// garbage collector's window
const USHORT WIN_garbage_collect	= 8;	// scan left a page for garbage collector


// Thread specific database block

// tdbb_flags

const USHORT TDBB_sweeper				= 1;	// Thread sweeper or garbage collector
const USHORT TDBB_no_cache_unwind		= 2;	// Don't unwind page buffer cache
const USHORT TDBB_backup_write_locked	= 4;    // BackupManager has write lock on LCK_backup_database
const USHORT TDBB_stack_trace_done		= 8;	// PSQL stack trace is added into status-vector
const USHORT TDBB_shutdown_manager		= 16;	// Server shutdown thread
const USHORT TDBB_dont_post_dfw			= 32;	// dont post DFW tasks as deferred work is performed now
const USHORT TDBB_sys_error				= 64;	// error shouldn't be handled by the looper
const USHORT TDBB_verb_cleanup			= 128;	// verb cleanup is in progress
const USHORT TDBB_use_db_page_space		= 256;	// use database (not temporary) page space in GTT operations
const USHORT TDBB_detaching				= 512;	// detach is in progress
const USHORT TDBB_wait_cancel_disable	= 1024;	// don't cancel current waiting operation
const USHORT TDBB_cache_unwound			= 2048;	// page cache was unwound

class thread_db : public Firebird::ThreadData
{
private:
	MemoryPool*	defaultPool;
	void setDefaultPool(MemoryPool* p)
	{
		defaultPool = p;
	}
	friend class Firebird::SubsystemContextPoolHolder <Jrd::thread_db, MemoryPool>;
	Database*	database;
	Attachment*	attachment;
	jrd_tra*	transaction;
	jrd_req*	request;
	RuntimeStatistics *reqStat, *traStat, *attStat, *dbbStat;
	thread_db	*priorThread, *nextThread;

public:
	explicit thread_db(ISC_STATUS* status)
		: ThreadData(ThreadData::tddDBB),
		  defaultPool(NULL),
		  database(NULL),
		  attachment(NULL),
		  transaction(NULL),
		  request(NULL),
		  priorThread(NULL),
		  nextThread(NULL),
		  tdbb_status_vector(status),
		  tdbb_quantum(QUANTUM),
		  tdbb_flags(0),
		  tdbb_temp_traid(0),
		  tdbb_bdbs(*getDefaultMemoryPool()),
		  tdbb_thread(Firebird::ThreadSync::getThread("thread_db"))
	{
		reqStat = traStat = attStat = dbbStat = RuntimeStatistics::getDummy();
		fb_utils::init_status(tdbb_status_vector);
	}

	~thread_db()
	{
#ifdef DEV_BUILD
		for (size_t n = 0; n < tdbb_bdbs.getCount(); ++n)
		{
			fb_assert(tdbb_bdbs[n] == NULL);
		}
#endif
	}

	ISC_STATUS*	tdbb_status_vector;
	SSHORT		tdbb_quantum;		// Cycles remaining until voluntary schedule
	USHORT		tdbb_flags;

	TraNumber	tdbb_temp_traid;	// current temporary table scope

	// BDB's held by thread
	Firebird::HalfStaticArray<BufferDesc*, 16> tdbb_bdbs;
	Firebird::ThreadSync* tdbb_thread;

	MemoryPool* getDefaultPool()
	{
		return defaultPool;
	}

	Database* getDatabase()
	{
		return database;
	}

	const Database* getDatabase() const
	{
		return database;
	}

	void setDatabase(Database* val);

	Attachment* getAttachment()
	{
		return attachment;
	}

	const Attachment* getAttachment() const
	{
		return attachment;
	}

	void setAttachment(Attachment* val);

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

	SSHORT getCharSet() const;

	void bumpStats(const RuntimeStatistics::StatType index)
	{
		reqStat->bumpValue(index);
		traStat->bumpValue(index);
		attStat->bumpValue(index);
		dbbStat->bumpValue(index);
	}

	void bumpRelStats(const RelStatType index, SLONG relation_id)
	{
		reqStat->bumpRelValue(index, relation_id);
		traStat->bumpRelValue(index, relation_id);
		attStat->bumpRelValue(index, relation_id);
		//dbbStat->bumpRelValue(index, relation_id);
	}

	bool checkCancelState(bool punt);

	void registerBdb(BufferDesc* bdb)
	{
		if (tdbb_bdbs.isEmpty()) {
			tdbb_flags &= ~TDBB_cache_unwound;
		}
		fb_assert(!(tdbb_flags & TDBB_cache_unwound));

		size_t pos;
		if (tdbb_bdbs.find(NULL, pos))
			tdbb_bdbs[pos] = bdb;
		else
			tdbb_bdbs.add(bdb);
	}

	bool clearBdb(BufferDesc* bdb)
	{
		if (tdbb_bdbs.isEmpty())
		{
			// hvlad: the only legal case when thread holds no latches but someone
			// tried to release latch is when CCH_unwind was called (and released
			// all latches) but caller is unaware about it. See CORE-3034, for example.
			// Else it is bug and should be BUGCHECK'ed.

			if (tdbb_flags & TDBB_cache_unwound)
				return false;
		}
		fb_assert(!(tdbb_flags & TDBB_cache_unwound));

		size_t pos;
		if (!tdbb_bdbs.find(bdb, pos))
			BUGCHECK(300);	// can't find shared latch

		tdbb_bdbs[pos] = NULL;

		if (pos == tdbb_bdbs.getCount() - 1)
		{
			while (true)
			{
				if (tdbb_bdbs[pos] != NULL)
				{
					tdbb_bdbs.shrink(pos + 1);
					break;
				}

				if (pos == 0)
				{
					tdbb_bdbs.shrink(0);
					break;
				}

				--pos;
			}
		}

		return true;
	}

	void activate()
	{
		fb_assert(!priorThread && !nextThread);

		if (database)
		{
			Firebird::SyncLockGuard sync(&database->dbb_threads_sync, Firebird::SYNC_EXCLUSIVE,
										 "thread_db::activate");

			if (database->dbb_active_threads)
			{
				fb_assert(!database->dbb_active_threads->priorThread);
				database->dbb_active_threads->priorThread = this;
				nextThread = database->dbb_active_threads;
			}

			database->dbb_active_threads = this;
		}
	}

	void deactivate()
	{
		if (database)
		{
			Firebird::SyncLockGuard sync(&database->dbb_threads_sync, Firebird::SYNC_EXCLUSIVE,
										 "thread_db::deactivate");

			if (nextThread)
			{
				fb_assert(nextThread->priorThread == this);
				nextThread->priorThread = priorThread;
			}

			if (priorThread)
			{
				fb_assert(priorThread->nextThread == this);
				priorThread->nextThread = nextThread;
			}
			else
			{
				fb_assert(database->dbb_active_threads == this);
				database->dbb_active_threads = nextThread;
			}
		}

		priorThread = nextThread = NULL;
	}
};


class ThreadContextHolder
{
public:
	explicit ThreadContextHolder(ISC_STATUS* status = NULL)
		: context(status ? status : local_status), externStatus(NULL)
	{
		context.putSpecific();
	}

	explicit ThreadContextHolder(Firebird::IStatus* status)
		: context(local_status), externStatus(status)
	{
		context.putSpecific();
		externStatus->init();
	}

	ThreadContextHolder(Database* dbb, Jrd::Attachment* att, ISC_STATUS* status = NULL)
		: context(status ? status : local_status), externStatus(NULL)
	{
		context.putSpecific();
		context.setDatabase(dbb);
		context.setAttachment(att);
	}

	~ThreadContextHolder()
	{
		unsigned l = fb_utils::statusLength(context.tdbb_status_vector);
		if (externStatus && externStatus->isSuccess() && (l > 2 || context.tdbb_status_vector[1]))
		{
			externStatus->set(l, context.tdbb_status_vector);
		}
		Firebird::ThreadData::restoreSpecific();
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
	Firebird::IStatus* externStatus;
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


// duplicate context of firebird string
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
	Firebird::ThreadData* p1 = Firebird::ThreadData::getSpecific();
	if (p1 && p1->getType() == Firebird::ThreadData::tddDBB)
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
	fb_assert(tdbb && (tdbb->getType() == Firebird::ThreadData::tddDBB) &&
		(!tdbb->getDatabase() || tdbb->getDatabase()->checkHandle()));
}

inline void CHECK_DBB(const Jrd::Database* dbb)
{
	fb_assert(dbb && dbb->checkHandle());
}

#else // PROD_BUILD

inline Jrd::thread_db* JRD_get_thread_data()
{
	return (Jrd::thread_db*) Firebird::ThreadData::getSpecific();
}

inline void CHECK_DBB(const Jrd::Database*)
{
}

inline void CHECK_TDBB(const Jrd::thread_db*)
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
inline void SET_TDBB(Jrd::thread_db*& tdbb)
{
	if (tdbb == NULL) {
		tdbb = JRD_get_thread_data();
	}
	CHECK_TDBB(tdbb);
}

inline void SET_DBB(Jrd::Database*& dbb)
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

	class DatabaseContextHolder : public Jrd::ContextPoolHolder
	{
	public:
		explicit DatabaseContextHolder(thread_db* tdbb)
			: Jrd::ContextPoolHolder(tdbb, tdbb->getDatabase()->dbb_permanent),
			  savedTdbb(tdbb)
		{
			savedTdbb->activate();
		}

		~DatabaseContextHolder()
		{
			savedTdbb->deactivate();
		}

	private:
		// copying is prohibited
		DatabaseContextHolder(const DatabaseContextHolder&);
		DatabaseContextHolder& operator=(const DatabaseContextHolder&);

		thread_db* const savedTdbb;
	};

	class BackgroundContextHolder : public ThreadContextHolder, public DatabaseContextHolder,
		public Jrd::Attachment::SyncGuard
	{
	public:
		BackgroundContextHolder(Database* dbb, Jrd::Attachment* att, ISC_STATUS* status, const char* f)
			: ThreadContextHolder(dbb, att, status),
			  DatabaseContextHolder(operator thread_db*()),
			  Jrd::Attachment::SyncGuard(att, f)
		{}

	private:
		// copying is prohibited
		BackgroundContextHolder(const BackgroundContextHolder&);
		BackgroundContextHolder& operator=(const BackgroundContextHolder&);
	};

	class AstLockHolder : public Firebird::ReadLockGuard
	{
	public:
		AstLockHolder(Database* dbb, const char* f)
			: Firebird::ReadLockGuard(dbb->dbb_ast_lock, f)
		{
			if (dbb->dbb_flags & DBB_no_ast)
			{
				// usually to be swallowed by the AST, but it allows to skip its execution
				Firebird::status_exception::raise(Firebird::Arg::Gds(isc_unavailable));
			}
		}
	};

	class AsyncContextHolder : public AstLockHolder, public Jrd::Attachment::SyncGuard,
		public ThreadContextHolder, public DatabaseContextHolder
	{
	public:
		AsyncContextHolder(Database* dbb, const char* f, Lock* lck = NULL)
			: AstLockHolder(dbb, f),
			  Jrd::Attachment::SyncGuard(lck ?
				lck->getLockInterface() : Firebird::RefPtr<JAttachment>(), f, true),
			  ThreadContextHolder(dbb, lck ? lck->getLockAttachment() : NULL),
			  DatabaseContextHolder(operator thread_db*())
		{}

	private:
		// copying is prohibited
		AsyncContextHolder(const AsyncContextHolder&);
		AsyncContextHolder& operator=(const AsyncContextHolder&);
	};
}

#endif // JRD_JRD_H
