/*
 *      PROGRAM:        JRD access method
 *      MODULE:         Attachment.h
 *      DESCRIPTION:    JRD Attachment class
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
 */

#ifndef JRD_ATTACHMENT_H
#define JRD_ATTACHMENT_H

#include "firebird.h"
// Definition of block types for data allocation in JRD
#include "../include/fb_blk.h"
#include "../jrd/scl.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/RandomGenerator.h"
#include "../jrd/RuntimeStatistics.h"

#include "../common/classes/ByteChunk.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/SyncObject.h"
#include "../common/classes/array.h"
#include "../common/classes/stack.h"
#include "../common/classes/timestamp.h"

#include "../jrd/EngineInterface.h"

namespace EDS {
	class Connection;
}

namespace Firebird {
	class ICryptKeyCallback;
}

class CharSetContainer;

namespace Jrd
{
	class thread_db;
	class Database;
	class jrd_tra;
	class jrd_req;
	class Lock;
	class jrd_file;
	class Format;
	class BufferControl;
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
	template <typename T> class vec;
	class jrd_rel;
	class jrd_prc;
	class Trigger;
	typedef Firebird::ObjectsArray<Trigger> trig_vec;
	class Function;
	class JrdStatement;

struct DSqlCacheItem
{
	Lock* lock;
	bool locked;
	bool obsolete;
};

typedef Firebird::GenericMap<Firebird::Pair<Firebird::Left<
	Firebird::string, DSqlCacheItem> > > DSqlCache;


struct DdlTriggerContext
{
	DdlTriggerContext()
		: eventType(*getDefaultMemoryPool()),
		  objectType(*getDefaultMemoryPool()),
		  objectName(*getDefaultMemoryPool()),
		  sqlText(*getDefaultMemoryPool())
	{
	}

	Firebird::string eventType;
	Firebird::string objectType;
	Firebird::MetaName objectName;
	Firebird::string sqlText;
};


struct bid;

//
// the attachment block; one is created for each attachment to a database
//
class Attachment : public pool_alloc<type_att>
{
public:
	class SyncGuard
	{
	public:
		SyncGuard(JAttachment* ja, const char* f, bool optional = false)
			: jAtt(ja)
		{
			init(f, optional);
		}

		SyncGuard(Attachment* att, const char* f, bool optional = false)
			: jAtt(att ? att->att_interface : NULL)
		{
			init(f, optional);
		}

		~SyncGuard()
		{
			if (jAtt)
				jAtt->getMutex()->leave();
		}

	private:
		// copying is prohibited
		SyncGuard(const SyncGuard&);
		SyncGuard& operator=(const SyncGuard&);

		void init(const char* f, bool optional);

		Firebird::RefPtr<JAttachment> jAtt;
	};

	class Checkout
	{
	public:
		Checkout(Attachment* att, const char* f, bool optional = false)
#ifdef DEV_BUILD
			: from(f)
#define FB_LOCKED_FROM from
#else
#define FB_LOCKED_FROM NULL
#endif
		{
			if (att && att->att_interface)
			{
				m_ref = att->att_interface;
			}

			fb_assert(optional || m_ref.hasData());

			if (m_ref.hasData())
				m_ref->getMutex()->leave();
		}

		~Checkout()
		{
			if (m_ref.hasData())
				m_ref->getMutex()->enter(FB_LOCKED_FROM);
		}

	private:
		// copying is prohibited
		Checkout(const Checkout&);
		Checkout& operator=(const Checkout&);

		Firebird::RefPtr<JAttachment> m_ref;
#ifdef DEV_BUILD
		const char* from;
#endif
	};
#undef FB_LOCKED_FROM

	class CheckoutLockGuard
	{
	public:
		CheckoutLockGuard(Attachment* att, Firebird::Mutex& mutex, const char* f, bool optional = false)
			: m_mutex(mutex)
		{
			if (!m_mutex.tryEnter(f))
			{
				Checkout attCout(att, f, optional);
				m_mutex.enter(f);
			}
		}

		~CheckoutLockGuard()
		{
			m_mutex.leave();
		}

	private:
		// copying is prohibited
		CheckoutLockGuard(const CheckoutLockGuard&);
		CheckoutLockGuard& operator=(const CheckoutLockGuard&);

		Firebird::Mutex& m_mutex;
	};

	class CheckoutSyncGuard
	{
	public:
		CheckoutSyncGuard(Attachment* att, Firebird::SyncObject& sync, Firebird::SyncType type, const char* f)
			: m_sync(&sync, f)
		{
			if (!m_sync.lockConditional(type, f))
			{
				Checkout attCout(att, f);
				m_sync.lock(type);
			}
		}

	private:
		// copying is prohibited
		CheckoutSyncGuard(const CheckoutSyncGuard&);
		CheckoutSyncGuard& operator=(const CheckoutSyncGuard&);

		Firebird::Sync m_sync;
	};

public:
	static Attachment* create(Database* dbb);
	static void destroy(Attachment* const attachment);

	MemoryPool* const att_pool;					// Memory pool
	Firebird::MemoryStats att_memory_stats;

	Database*	att_database;				// Parent database block
	Attachment*	att_next;					// Next attachment to database
	UserId*		att_user;					// User identification
	jrd_tra*	att_transactions;			// Transactions belonging to attachment
	jrd_tra*	att_dbkey_trans;			// transaction to control db-key scope
	TraNumber	att_oldest_snapshot;		// GTT's record versions older than this can be garbage-collected

private:
	jrd_tra*	att_sys_transaction;		// system transaction

public:
	Firebird::SortedArray<jrd_req*> att_requests;	// Requests belonging to attachment
	Lock*		att_id_lock;				// Attachment lock (if any)
	SLONG		att_attachment_id;			// Attachment ID
	Lock*		att_cancel_lock;			// Lock to cancel the active request
	const ULONG	att_lock_owner_id;			// ID for the lock manager
	SLONG		att_lock_owner_handle;		// Handle for the lock manager
	ULONG		att_backup_state_counter;	// Counter of backup state locks for attachment
	SLONG		att_event_session;			// Event session id, if any
	SecurityClass*	att_security_class;		// security class for database
	SecurityClassList*	att_security_classes;	// security classes
	RuntimeStatistics	att_stats;
	ULONG		att_flags;					// Flags describing the state of the attachment
	SSHORT		att_client_charset;			// user's charset specified in dpb
	SSHORT		att_charset;				// current (client or external) attachment charset
	Lock*		att_long_locks;				// outstanding two phased locks
	Lock*		att_wait_lock;				// lock at which attachment waits currently
	vec<Lock*>*	att_compatibility_table;	// hash table of compatible locks
	vcl*		att_val_errors;
	Firebird::PathName	att_working_directory;	// Current working directory is cached
	Firebird::PathName	att_filename;			// alias used to attach the database
	const Firebird::TimeStamp	att_timestamp;	// Connection date and time
	Firebird::StringMap att_context_vars;	// Context variables for the connection
	Firebird::Stack<DdlTriggerContext> ddlTriggersContext;	// Context variables for DDL trigger event
	Firebird::string att_network_protocol;	// Network protocol used by client for connection
	Firebird::string att_remote_address;	// Protocol-specific address of remote client
	SLONG att_remote_pid;					// Process id of remote client
	Firebird::PathName att_remote_process;	// Process name of remote client
	Firebird::string att_client_version;	// Version of the client library
	Firebird::string att_remote_protocol;	// Details about the remote protocol
	Firebird::string att_remote_host;		// Host name of remote client
	Firebird::string att_remote_os_user;	// OS user name of remote client
	RandomGenerator att_random_generator;	// Random bytes generator
	Lock*		att_temp_pg_lock;			// temporary pagespace ID lock
	DSqlCache att_dsql_cache;	// DSQL cache locks
	Firebird::SortedArray<void*> att_udf_pointers;
	dsql_dbb* att_dsql_instance;
	bool att_in_use;						// attachment in use (can't be detached or dropped)
	int att_use_count;						// number of API calls running except of asynchronous ones

	EDS::Connection* att_ext_connection;	// external connection executed by this attachment
	ULONG att_ext_call_depth;				// external connection call depth, 0 for user attachment
	TraceManager* att_trace_manager;		// Trace API manager

	JAttachment* att_interface;

	/// former Database members - start

	vec<jrd_rel*>*					att_relations;			// relation vector
	Firebird::Array<jrd_prc*>		att_procedures;			// scanned procedures
	trig_vec*						att_triggers[DB_TRIGGER_MAX];
	trig_vec*						att_ddl_triggers;
	Firebird::Array<Function*>		att_functions;			// User defined functions

	Firebird::Array<JrdStatement*>	att_internal;			// internal statements
	Firebird::Array<JrdStatement*>	att_dyn_req;			// internal dyn statements
	Firebird::ICryptKeyCallback*	att_crypt_callback;		// callback for DB crypt

	jrd_req* findSystemRequest(thread_db* tdbb, USHORT id, USHORT which);

	Firebird::Array<CharSetContainer*>	att_charsets;		// intl character set descriptions
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, USHORT> > > att_charset_ids;	// Character set ids

	void releaseIntlObjects(thread_db* tdbb);			// defined in intl.cpp
	void destroyIntlObjects(thread_db* tdbb);			// defined in intl.cpp

	void releaseLocks(thread_db* tdbb);

	Firebird::Array<MemoryPool*>	att_pools;		// pools

	MemoryPool* createPool();
	void deletePool(MemoryPool* pool);

	/// former Database members - end

	bool locksmith() const;
	jrd_tra* getSysTransaction();
	void setSysTransaction(jrd_tra* trans);	// used only by TRA_init
	bool isRWGbak() const;
	bool isUtility() const; // gbak, gfix and gstat.

	PreparedStatement* prepareStatement(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::string& text, Firebird::MemoryPool* pool = NULL);
	PreparedStatement* prepareStatement(thread_db* tdbb, jrd_tra* transaction,
		const PreparedStatement::Builder& builder, Firebird::MemoryPool* pool = NULL);

	PreparedStatement* prepareUserStatement(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::string& text, Firebird::MemoryPool* pool = NULL);

	Firebird::MetaName nameToMetaCharSet(thread_db* tdbb, const Firebird::MetaName& name);
	Firebird::MetaName nameToUserCharSet(thread_db* tdbb, const Firebird::MetaName& name);
	Firebird::string stringToMetaCharSet(thread_db* tdbb, const Firebird::string& str,
		const char* charSet = NULL);
	Firebird::string stringToUserCharSet(thread_db* tdbb, const Firebird::string& str);

	void storeMetaDataBlob(thread_db* tdbb, jrd_tra* transaction,
		bid* blobId, const Firebird::string& text, USHORT fromCharSet = CS_METADATA);
	void storeBinaryBlob(thread_db* tdbb, jrd_tra* transaction, bid* blobId,
		const Firebird::ByteChunk& chunk);

	void signalCancel();
	void signalShutdown();

	void detachLocksFromAttachment();

	bool backupStateWriteLock(thread_db* tdbb, SSHORT wait);
	void backupStateWriteUnLock(thread_db* tdbb);
	bool backupStateReadLock(thread_db* tdbb, SSHORT wait);
	void backupStateReadUnLock(thread_db* tdbb);

private:
	Attachment(MemoryPool* pool, Database* dbb);
	~Attachment();
};


// Attachment flags

const ULONG ATT_no_cleanup			= 0x00001L;	// Don't expunge, purge, or garbage collect
const ULONG ATT_shutdown			= 0x00002L;	// attachment has been shutdown
const ULONG ATT_shutdown_manager	= 0x00004L;	// attachment requesting shutdown
const ULONG ATT_exclusive			= 0x00008L;	// attachment wants exclusive database access
const ULONG ATT_attach_pending		= 0x00010L;	// Indicate attachment is only pending
const ULONG ATT_exclusive_pending	= 0x00020L;	// Indicate exclusive attachment pending
const ULONG ATT_gbak_attachment		= 0x00040L;	// Indicate GBAK attachment
const ULONG ATT_notify_gc			= 0x00080L;	// Notify garbage collector to expunge, purge ..
const ULONG ATT_garbage_collector	= 0x00100L;	// I'm a garbage collector
const ULONG ATT_cancel_raise		= 0x00200L;	// Cancel currently running operation
const ULONG ATT_cancel_disable		= 0x00400L;	// Disable cancel operations
const ULONG ATT_gfix_attachment		= 0x00800L;	// Indicate a GFIX attachment
const ULONG ATT_gstat_attachment	= 0x01000L;	// Indicate a GSTAT attachment
const ULONG ATT_no_db_triggers		= 0x02000L;	// Don't execute database triggers
const ULONG ATT_manual_lock			= 0x04000L;	// Was locked manually
const ULONG ATT_async_manual_lock	= 0x08000L;	// Async mutex was locked manually
const ULONG ATT_purge_started		= 0x10000L; // Purge already started - avoid 2 purges at once
const ULONG ATT_system				= 0x20000L; // Special system attachment
const ULONG ATT_creator				= 0x40000L; // This attachment created the DB.

// Composed flags (mixtures of the previous values).
const ULONG ATT_NO_CLEANUP			= (ATT_no_cleanup | ATT_notify_gc);
const ULONG ATT_RW_GBAK				= (ATT_gbak_attachment | ATT_creator);
const ULONG ATT_ANY_UTILITY			= (ATT_gbak_attachment | ATT_gfix_attachment | ATT_gstat_attachment);


inline bool Attachment::locksmith() const
{
	return att_user && att_user->locksmith();
}

inline jrd_tra* Attachment::getSysTransaction()
{
	return att_sys_transaction;
}

inline void Attachment::setSysTransaction(jrd_tra* trans)
{
	att_sys_transaction = trans;
}

// Gbak changes objects when it's restoring (creating) a db.
// Other attempts are fake. Gbak reconnects to change R/O status and other db-wide settings,
// but it doesn't modify generators or tables that seconds time.
inline bool Attachment::isRWGbak() const
{
	return (att_flags & ATT_RW_GBAK) == ATT_RW_GBAK;
}

// Any of the three original utilities: gbak, gfix or gstat.
inline bool Attachment::isUtility() const
{
	return att_flags & ATT_ANY_UTILITY;
}

// This class holds references to all attachments it contains

class AttachmentsRefHolder
{
	friend class Iterator;

public:
	class Iterator
	{
	public:
		explicit Iterator(AttachmentsRefHolder& list)
			: m_list(list), m_index(0)
		{}

		JAttachment* operator*()
		{
			if (m_index < m_list.m_attachments.getCount())
				return m_list.m_attachments[m_index];

			return NULL;
		}

		void operator++()
		{
			m_index++;
		}

		void remove()
		{
			if (m_index < m_list.m_attachments.getCount())
			{
				AttachmentsRefHolder::debugHelper(FB_FUNCTION);
				m_list.m_attachments[m_index]->release();
				m_list.m_attachments.remove(m_index);
			}
		}

	private:
		// copying is prohibited
		Iterator(const Iterator&);
		Iterator& operator=(const Iterator&);

		AttachmentsRefHolder& m_list;
		size_t m_index;
	};

	explicit AttachmentsRefHolder(MemoryPool& p)
		: m_attachments(p)
	{}

	AttachmentsRefHolder& operator=(const AttachmentsRefHolder& other)
	{
		this->~AttachmentsRefHolder();

		for (size_t i = 0; i < other.m_attachments.getCount(); i++)
			add(other.m_attachments[i]);

		return *this;
	}

	~AttachmentsRefHolder()
	{
		while (m_attachments.hasData())
		{
			debugHelper(FB_FUNCTION);
			m_attachments.pop()->release();
		}
	}

	void add(JAttachment* jAtt)
	{
		if (jAtt)
		{
			jAtt->addRef();
			m_attachments.add(jAtt);
		}
	}

	void remove(Iterator& iter)
	{
		iter.remove();
	}

private:
	AttachmentsRefHolder(const AttachmentsRefHolder&);

	static void debugHelper(const char* from);

	Firebird::HalfStaticArray<JAttachment*, 128> m_attachments;
};

} // namespace Jrd

#endif // JRD_ATTACHMENT_H
