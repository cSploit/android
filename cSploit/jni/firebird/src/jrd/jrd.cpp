/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		jrd.cpp
 *	DESCRIPTION:	User visible entrypoints
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 * 2001.07.09 Sean Leyne - Restore default setting to Force Write = "On", for
 *                         Windows NT platform, for new database files. This was changed
 *                         with IB 6.0 to OFF and has introduced many reported database
 *                         corruptions.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * Claudio Valderrama C.
 * Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/ThreadStart.h"
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <errno.h>

#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/irq.h"
#include "../jrd/drq.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/blb.h"
#include "../jrd/lck.h"
#include "../jrd/nbak.h"
#include "../jrd/scl.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/exe.h"
#include "../jrd/extds/ExtDS.h"
#include "../jrd/val.h"
#include "../jrd/rse.h"
#include "../jrd/intl.h"
#include "../jrd/sbm.h"
#include "../jrd/svc.h"
#include "../jrd/sdw.h"
#include "../jrd/lls.h"
#include "../jrd/cch.h"
#include "../intl/charsets.h"
#include "../jrd/sort.h"
#include "../jrd/PreparedStatement.h"
#include "../dsql/StmtNodes.h"

#include "../jrd/blb_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/fun_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/ini_proto.h"
#include "../jrd/intl_proto.h"
#include "../common/isc_f_proto.h"
#include "../common/isc_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/shut_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/tpc_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/val_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/file_params.h"
#include "../jrd/event_proto.h"
#include "../yvalve/why_proto.h"
#include "../jrd/flags.h"
#include "../jrd/Mapping.h"

#include "../jrd/Database.h"

#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../common/db_alias.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceObjects.h"
#include "../jrd/trace/TraceJrdHelpers.h"
#include "../jrd/IntlManager.h"
#include "../common/classes/fb_tls.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/RefMutex.h"
#include "../common/utils_proto.h"
#include "../jrd/DebugInterface.h"
#include "../jrd/EngineInterface.h"
#include "../jrd/CryptoManager.h"

#include "../dsql/dsql.h"
#include "../dsql/dsql_proto.h"

#include "firebird/Crypt.h"

using namespace Jrd;
using namespace Firebird;

const SSHORT WAIT_PERIOD	= -1;

#ifdef SUPPORT_RAW_DEVICES
#define unlink PIO_unlink
#endif

#ifdef DEV_BUILD
int debug;
#endif

namespace Jrd
{

int JBlob::release()
{
	if (--refCounter != 0)
		return 1;

	if (blob)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!blob)
	{
		delete this;
	}

	return 0;
}

int JTransaction::release()
{
	if (--refCounter != 0)
		return 1;

	RefDeb(DEB_RLS_JATT, "JTransaction::release");

	if (transaction)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!transaction)
	{
		delete this;
	}

	return 0;
}

int JStatement::release()
{
	if (--refCounter != 0)
		return 1;

	if (statement)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!statement)
	{
		delete this;
	}

	return 0;
}

int JRequest::release()
{
	if (--refCounter != 0)
		return 1;

	if (rq)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!rq)
	{
		delete this;
	}

	return 0;
}

int JEvents::release()
{
	if (--refCounter != 0)
		return 1;

	if (id >= 0)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (id < 0)
	{
		delete this;
	}

	return 0;
}

JAttachment::JAttachment(Attachment* handle)
	: att(handle)
{
}

void JAttachment::manualLock(ULONG& flags)
{
	fb_assert(!(flags & ATT_manual_lock));
	asyncMutex.enter(FB_FUNCTION);
	mainMutex.enter(FB_FUNCTION);
	flags |= (ATT_manual_lock | ATT_async_manual_lock);
}

void JAttachment::manualUnlock(ULONG& flags)
{
	if (flags & ATT_manual_lock)
	{
		flags &= ~ATT_manual_lock;
		mainMutex.leave();
	}
	manualAsyncUnlock(flags);
}

void JAttachment::manualAsyncUnlock(ULONG& flags)
{
	if (flags & ATT_async_manual_lock)
	{
		flags &= ~ATT_async_manual_lock;
		asyncMutex.leave();
	}
}

//#define DEBUG_ATT_COUNTERS

int JAttachment::release()
{
#ifdef DEBUG_ATT_COUNTERS
	int x = --refCounter;
	ReferenceCounterDebugger* my = ReferenceCounterDebugger::get(DEB_RLS_JATT);
	const char* point = my ? my->rcd_point : " <Unknown> ";
	fprintf(stderr, "Release from <%s> att %p cnt=%d\n", point, this, x);
	if (x != 0)
		return 1;
#else
	if (--refCounter != 0)
		return 1;
#endif

	if (att)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!att)
	{
		delete this;
	}

	return 0;
}

JService::JService(Service* handle)
	: svc(handle)
{
}

int JService::release()
{
	if (--refCounter != 0)
		return 1;

	if (svc)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!svc)
	{
		delete this;
	}

	return 0;
}

int JProvider::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}

	return 1;
}

static void shutdownBeforeUnload()
{
	LocalStatus status;
	JProvider::getInstance()->shutdown(&status, 0, fb_shutrsn_exit_called);
};

class EngineFactory : public AutoIface<IPluginFactory, FB_PLUGIN_FACTORY_VERSION>
{
public:
	// IPluginFactory implementation
	IPluginBase* FB_CARG createPlugin(IPluginConfig* factoryParameter)
	{
		if (myModule->unloadStarted())
		{
			return NULL;
		}

		IPluginBase* p = new JProvider(factoryParameter);
		p->addRef();
		return p;
	}
};

static Static<EngineFactory> engineFactory;

void registerEngine(IPluginManager* iPlugin)
{
	myModule->setCleanup(shutdownBeforeUnload);
	iPlugin->registerPluginFactory(PluginType::Provider, CURRENT_ENGINE, &engineFactory);
	myModule->registerMe();
}

} // namespace Jrd

extern "C" void FB_DLL_EXPORT FB_PLUGIN_ENTRY_POINT(IMaster* master)
{
	CachedMasterInterface::set(master);
	registerEngine(PluginManagerInterfacePtr());
}

namespace
{
	using Jrd::Attachment;

	// Flag engineShutdown guarantees that no new attachment is created after setting it
	// and helps avoid more than 1 shutdown threads running simultaneously.
	bool engineShutdown = false;
	// This flag is protected with 2 mutexes. shutdownMutex is taken by each shutdown thread
	// (for a relatively long time). newAttachmentMutex is taken (for a short time) when
	// shutdown thread is starting shutdown and also when new attachment is created.
	GlobalPtr<Mutex> shutdownMutex, newAttachmentMutex;

	// This mutex is set when new Database block is created. It's global first of all to satisfy
	// SS requirement - avoid 2 Database blocks for same database (file). Also guarantees no
	// half-done Database block in databases linked list. Always taken before databases_mutex.
	GlobalPtr<Mutex> dbInitMutex;

	Database* databases = NULL;
	// This mutex protects linked list of databases
	GlobalPtr<Mutex> databases_mutex;

	// Holder for per-database init/fini mutex
	class RefMutexUnlock
	{
	public:
		RefMutexUnlock()
			: entered(false)
		{ }

		explicit RefMutexUnlock(Database::ExistenceRefMutex* p)
			: ref(p), entered(false)
		{ }

		void enter()
		{
			fb_assert(ref);
			ref->enter();
			entered = true;
		}

		void leave()
		{
			if (entered)
			{
				ref->leave();
				entered = false;
			}
		}

		void linkWith(Database::ExistenceRefMutex* to)
		{
			if (ref == to)
				return;

			leave();
			ref = to;
		}

		void unlinkFromMutex()
		{
			linkWith(NULL);
		}

		Database::ExistenceRefMutex* operator->()
		{
			return ref;
		}

		bool operator!() const
		{
			return !ref;
		}

		~RefMutexUnlock()
		{
			leave();
		}

	private:
		RefPtr<Database::ExistenceRefMutex> ref;
		bool entered;
	};

	// We have 2 more related types of mutexes in database and attachment.
	// Attachment is using reference counted mutex in JAtt, also making it possible
	// to check does object still exist after locking a mutex. This makes great use when
	// checking for correctness of attachment in provider's entrypoints. Attachment mutex
	// is always taken before database's mutex and (except when new attachment is created)
	// when entering inside provider and releases when waiting for something or when rescheduling.
	// Database mutex (dbb_sync) is taken when access to database-wide data (like list of
	// attachments) is accessed. No other mutex from above mentioned here can be taken after
	// dbb_sync with an exception of attachment mutex for new attachment.
	// So finally the order of taking mutexes is:
	//	1. dbInitMutex (in attach/create database) or attachment mutex in other entries
	//	2. databases_mutex (when / if needed)
	//	3. dbb_sync (when / if needed)
	//	4. only for new attachments: attachment mutex when that attachment is created
	// Any of this may be missing when not needed, but order of taking should not be changed.

	class EngineStartup
	{
	public:
		static void init()
		{
			IbUtil::initialize();
			IntlManager::initialize();
			ExtEngineManager::initialize();
		}

		static void cleanup()
		{
		}
	};

	InitMutex<EngineStartup> engineStartup("EngineStartup");

	class OverwriteHolder : public MutexLockGuard
	{
	public:
		explicit OverwriteHolder(Database* to_remove)
			: MutexLockGuard(databases_mutex, FB_FUNCTION), dbb(to_remove)
		{
			if (!dbb)
				return;

			for (Database** d_ptr = &databases; *d_ptr; d_ptr = &(*d_ptr)->dbb_next)
			{
				if (*d_ptr == dbb)
				{
					*d_ptr = dbb->dbb_next;
					dbb->dbb_next = NULL;
					return;
				}
			}

			fb_assert(!dbb);
			dbb = NULL;
		}

		~OverwriteHolder()
		{
			if (dbb)
			{
				dbb->dbb_next = databases;
				databases = dbb;
			}
		}

	private:
		Database* dbb;
	};

	inline void validateHandle(thread_db* tdbb, Jrd::Attachment* const attachment)
	{
		if (attachment && attachment == tdbb->getAttachment())
			return;

		if (!attachment || !attachment->att_database)
			status_exception::raise(Arg::Gds(isc_bad_db_handle));

		tdbb->setAttachment(attachment);
		tdbb->setDatabase(attachment->att_database);
	}

	inline void validateHandle(thread_db* tdbb, jrd_tra* const transaction)
	{
		if (!transaction)
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));

		validateHandle(tdbb, transaction->tra_attachment);

		tdbb->setTransaction(transaction);
	}

	inline void validateHandle(thread_db* tdbb, JrdStatement* const statement)
	{
		if (!statement)
			status_exception::raise(Arg::Gds(isc_bad_req_handle));

		validateHandle(tdbb, statement->requests[0]->req_attachment);
	}

	inline void validateHandle(thread_db* tdbb, dsql_req* const statement)
	{
		if (!statement)
			status_exception::raise(Arg::Gds(isc_bad_req_handle));

		validateHandle(tdbb, statement->req_dbb->dbb_attachment);
	}

	inline void validateHandle(thread_db* tdbb, blb* blob)
	{
		if (!blob)
			status_exception::raise(Arg::Gds(isc_bad_segstr_handle));

		validateHandle(tdbb, blob->getTransaction());
		validateHandle(tdbb, blob->getAttachment());
	}

	inline void validateHandle(Service* service)
	{
		if (!service)
			status_exception::raise(Arg::Gds(isc_bad_svc_handle));
	}

	inline void validateHandle(thread_db* tdbb, JEvents* const events)
	{
		validateHandle(tdbb, events->getAttachment()->getHandle());
	}

	class AttachmentHolder
	{
	public:
		static const unsigned ATT_LOCK_ASYNC	= 1;
		static const unsigned ATT_DONT_LOCK		= 2;

		AttachmentHolder(thread_db* tdbb, JAttachment* ja, unsigned lockFlags, const char* from)
			: jAtt(ja),
			  async(lockFlags & ATT_LOCK_ASYNC),
			  nolock(lockFlags & ATT_DONT_LOCK)
		{
			if (!nolock)
				jAtt->getMutex(async)->enter(from);

			Jrd::Attachment* attachment = jAtt->getHandle();	// Must be done after entering mutex

			try
			{
				if (!attachment || engineShutdown)
				{
					// This shutdown check is an optimization, threads can still enter engine
					// with the flag set cause shutdownMutex mutex is not locked here.
					// That's not a danger cause check of att_use_count
					// in shutdown code makes it anyway safe.
					status_exception::raise(Arg::Gds(isc_att_shutdown));
				}

				tdbb->setAttachment(attachment);
				tdbb->setDatabase(attachment->att_database);

				if (!async)
					attachment->att_use_count++;
			}
			catch (const Firebird::Exception&)
			{
				if (!nolock)
					jAtt->getMutex(async)->leave();
				throw;
			}
		}

		~AttachmentHolder()
		{
			Jrd::Attachment* attachment = jAtt->getHandle();

			if (attachment && !async)
				attachment->att_use_count--;

			if (!nolock)
				jAtt->getMutex(async)->leave();
		}

	private:
		RefPtr<JAttachment> jAtt;
		bool async;
		bool nolock; // if locked manually, no need to take lock recursively

	private:
		// copying is prohibited
		AttachmentHolder(const AttachmentHolder&);
		AttachmentHolder& operator =(const AttachmentHolder&);
	};

	class EngineContextHolder : public ThreadContextHolder, private AttachmentHolder,
		private DatabaseContextHolder
	{
	public:
		template <typename I>
		EngineContextHolder(IStatus* status, I* interfacePtr, const char* from,
							unsigned lockFlags = 0)
			: ThreadContextHolder(status),
			  AttachmentHolder(*this, interfacePtr->getAttachment(), lockFlags, from),
			  DatabaseContextHolder(operator thread_db*())
		{
			validateHandle(*this, interfacePtr->getHandle());
		}

	};

	void validateAccess(const Jrd::Attachment* attachment)
	{
		if (!attachment->locksmith())
		{
			ERR_post(Arg::Gds(isc_adm_task_denied));
		}
	}


	class DefaultCallback : public AutoIface<ICryptKeyCallback, FB_CRYPT_CALLBACK_VERSION>
	{
	public:
		unsigned int FB_CARG callback(unsigned int, const void*, unsigned int, void*)
		{
			return 0;
		}
	};

	DefaultCallback defCallback;

	ICryptKeyCallback* getCryptCallback(ICryptKeyCallback* callback)
	{
		return callback ? callback : &defCallback;
	}
} // anonymous


#ifdef  WIN_NT
#include <windows.h>
// these should stop a most annoying warning
#undef TEXT
#define TEXT    SCHAR
#endif	// WIN_NT

void Trigger::compile(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	Database* dbb = tdbb->getDatabase();
	Jrd::Attachment* const att = tdbb->getAttachment();

	if (extTrigger)
		return;

	if (!statement /*&& !compile_in_progress*/)
	{
		if (statement)
			return;

		compile_in_progress = true;
		// Allocate statement memory pool
		MemoryPool* new_pool = att->createPool();
		// Trigger request is not compiled yet. Lets do it now
		USHORT par_flags = (USHORT) (flags & TRG_ignore_perm) ? csb_ignore_perm : 0;
		if (type & 1)
			par_flags |= csb_pre_trigger;
		else
			par_flags |= csb_post_trigger;

		CompilerScratch* csb = NULL;
		try
		{
			Jrd::ContextPoolHolder context(tdbb, new_pool);

			csb = CompilerScratch::newCsb(*tdbb->getDefaultPool(), 5);
			csb->csb_g_flags |= par_flags;

			if (engine.isEmpty())
			{
				if (!dbg_blob_id.isEmpty())
					DBG_parse_debug_info(tdbb, &dbg_blob_id, *csb->csb_dbg_info);

				PAR_blr(tdbb, relation, blr.begin(), (ULONG) blr.getCount(), NULL, &csb, &statement,
					(relation ? true : false), par_flags);
			}
			else
			{
				dbb->dbb_extManager.makeTrigger(tdbb, csb, this, engine, entryPoint, extBody.c_str(),
					(relation ?
						(type & 1 ? ExternalTrigger::TYPE_BEFORE : ExternalTrigger::TYPE_AFTER) :
						Firebird::ExternalTrigger::TYPE_DATABASE));
			}

			delete csb;
		}
		catch (const Exception&)
		{
			compile_in_progress = false;
			delete csb;
			csb = NULL;

			if (statement)
			{
				statement->release(tdbb);
				statement = NULL;
			}
			else
				att->deletePool(new_pool);

			throw;
		}

		statement->triggerName = name;

		if (sys_trigger)
			statement->flags |= JrdStatement::FLAG_SYS_TRIGGER;

		if (flags & TRG_ignore_perm)
			statement->flags |= JrdStatement::FLAG_IGNORE_PERM;

		compile_in_progress = false;
	}
}

void Trigger::release(thread_db* tdbb)
{
	if (extTrigger)
	{
		delete extTrigger;
		extTrigger = NULL;
	}

	if (blr.getCount() == 0 || !statement || statement->isActive())
		return;

	statement->release(tdbb);
	statement = NULL;
}

// Option block for database parameter block

class DatabaseOptions
{
public:
	USHORT	dpb_wal_action;
	SLONG	dpb_sweep_interval;
	ULONG	dpb_page_buffers;
	bool	dpb_set_page_buffers;
	ULONG	dpb_buffers;
	USHORT	dpb_verify;
	USHORT	dpb_sweep;
	USHORT	dpb_dbkey_scope;
	USHORT	dpb_page_size;
	bool	dpb_activate_shadow;
	bool	dpb_delete_shadow;
	bool	dpb_no_garbage;
	USHORT	dpb_shutdown;
	SSHORT	dpb_shutdown_delay;
	USHORT	dpb_online;
	bool	dpb_force_write;
	bool	dpb_set_force_write;
	bool	dpb_no_reserve;
	bool	dpb_set_no_reserve;
	SSHORT	dpb_interp;
	bool	dpb_single_user;
	bool	dpb_overwrite;
	bool	dpb_sec_attach;
	bool	dpb_disable_wal;
	SLONG	dpb_connect_timeout;
	SLONG	dpb_dummy_packet_interval;
	bool	dpb_db_readonly;
	bool	dpb_set_db_readonly;
	bool	dpb_gfix_attach;
	bool	dpb_gstat_attach;
	USHORT	dpb_sql_dialect;
	USHORT	dpb_set_db_sql_dialect;
	SLONG	dpb_remote_pid;
	bool	dpb_no_db_triggers;
	bool	dpb_gbak_attach;
	bool	dpb_utf8_filename;
	ULONG	dpb_ext_call_depth;
	ULONG	dpb_flags;			// to OR'd with dbb_flags
	bool	dpb_nolinger;

	// here begin compound objects
	// for constructor to work properly dpb_user_name
	// MUST be FIRST
	string	dpb_user_name;
	AuthReader::AuthBlock	dpb_auth_block;
	string	dpb_role_name;
	string	dpb_journal;
	string	dpb_lc_ctype;
	PathName	dpb_working_directory;
	string	dpb_set_db_charset;
	string	dpb_network_protocol;
	string	dpb_remote_address;
	string	dpb_remote_host;
	string	dpb_remote_os_user;
	string	dpb_client_version;
	string	dpb_remote_protocol;
	string	dpb_trusted_login;
	PathName	dpb_remote_process;
	PathName	dpb_org_filename;
	string	dpb_config;

public:
	static const ULONG DPB_FLAGS_MASK = DBB_damaged;

	DatabaseOptions()
	{
		memset(this, 0,
			reinterpret_cast<char*>(&this->dpb_user_name) - reinterpret_cast<char*>(this));
	}

	void get(const UCHAR*, USHORT, bool&);

	void setBuffers(RefPtr<Config> config)
	{
		if (dpb_buffers == 0)
		{
			dpb_buffers = config->getDefaultDbCachePages();

			if (dpb_buffers < MIN_PAGE_BUFFERS)
				dpb_buffers = MIN_PAGE_BUFFERS;
			if (dpb_buffers > MAX_PAGE_BUFFERS)
				dpb_buffers = MAX_PAGE_BUFFERS;
		}
	}

private:
	void getPath(ClumpletReader& reader, PathName& s)
	{
		reader.getPath(s);
		if (!dpb_utf8_filename)
			ISC_systemToUtf8(s);
		ISC_unescape(s);
	}

	void getString(ClumpletReader& reader, string& s)
	{
		reader.getString(s);
		if (!dpb_utf8_filename)
			ISC_systemToUtf8(s);
		ISC_unescape(s);
	}
};

/// trace manager support

class TraceFailedConnection : public AutoIface<TraceDatabaseConnection, FB_TRACE_CONNECTION_VERSION>
{
public:
	TraceFailedConnection(const char* filename, const DatabaseOptions* options);

	// TraceBaseConnection implementation
	virtual ntrace_connection_kind_t FB_CARG getKind()	{ return connection_database; };
	virtual int FB_CARG getProcessID()					{ return m_options->dpb_remote_pid; }
	virtual const char* FB_CARG getUserName()			{ return m_id.usr_user_name.c_str(); }
	virtual const char* FB_CARG getRoleName()			{ return m_options->dpb_role_name.c_str(); }
	virtual const char* FB_CARG getCharSet()			{ return m_options->dpb_lc_ctype.c_str(); }
	virtual const char* FB_CARG getRemoteProtocol()		{ return m_options->dpb_network_protocol.c_str(); }
	virtual const char* FB_CARG getRemoteAddress()		{ return m_options->dpb_remote_address.c_str(); }
	virtual int FB_CARG getRemoteProcessID()			{ return m_options->dpb_remote_pid; }
	virtual const char* FB_CARG getRemoteProcessName()	{ return m_options->dpb_remote_process.c_str(); }

	// TraceDatabaseConnection implementation
	virtual int FB_CARG getConnectionID()				{ return 0; }
	virtual const char* FB_CARG getDatabaseName()		{ return m_filename; }

private:
	const char* m_filename;
	const DatabaseOptions* m_options;
	UserId m_id;
};

static void			check_database(thread_db* tdbb, bool async = false);
static void			commit(thread_db*, jrd_tra*, const bool);
static bool			drop_files(const jrd_file*);
static void			find_intl_charset(thread_db*, Jrd::Attachment*, const DatabaseOptions*);
static jrd_tra*		find_transaction(thread_db*);
static void			init_database_lock(thread_db*);
static void			run_commit_triggers(thread_db* tdbb, jrd_tra* transaction);
static jrd_req*		verify_request_synchronization(JrdStatement* statement, USHORT level);
static void			purge_transactions(thread_db*, Jrd::Attachment*, const bool);

static void 		handle_error(Firebird::IStatus*, ISC_STATUS);

namespace {
	enum VdnResult {VDN_FAIL, VDN_OK/*, VDN_SECURITY*/};
}
static VdnResult	verifyDatabaseName(const PathName&, ISC_STATUS*, bool);

static void			unwindAttach(thread_db* tdbb, const Exception& ex, Firebird::IStatus* userStatus,
	Jrd::Attachment* attachment, Database* dbb);
static JAttachment*	init(thread_db*, const PathName&, const PathName&, RefPtr<Config>, bool,
	const DatabaseOptions&, RefMutexUnlock&, IPluginConfig*);
static JAttachment*	create_attachment(const PathName&, Database*, const DatabaseOptions&, bool newDb);
static void		prepare_tra(thread_db*, jrd_tra*, USHORT, const UCHAR*);
static void		start_transaction(thread_db* tdbb, bool transliterate, jrd_tra** tra_handle,
	Jrd::Attachment* attachment, unsigned int tpb_length, const UCHAR* tpb);
static void		release_attachment(thread_db*, Jrd::Attachment*);
static void		rollback(thread_db*, jrd_tra*, const bool);
static void		strip_quotes(string&);
static void		purge_attachment(thread_db* tdbb, JAttachment* jAtt, unsigned flags = 0);
static void		getUserInfo(UserId&, const DatabaseOptions&, const char*, const char*, const RefPtr<Config>*);

static THREAD_ENTRY_DECLARE shutdown_thread(THREAD_ENTRY_PARAM);

// purge_attachment() flags
static const unsigned PURGE_FORCE	= 0x01;
static const unsigned PURGE_LINGER	= 0x02;

TraceFailedConnection::TraceFailedConnection(const char* filename, const DatabaseOptions* options) :
	m_filename(filename),
	m_options(options)
{
	getUserInfo(m_id, *m_options, m_filename, NULL, NULL);
}


//____________________________________________________________
//
// check whether we need to perform an autocommit;
// do it here to prevent committing every record update
// in a statement
//
static void check_autocommit(jrd_req* request, thread_db* tdbb)
{
	// dimitr: we should ignore autocommit for requests
	// created by EXECUTE STATEMENT
	// AP: also do nothing if request is cancelled and
	// transaction is already missing
	if ((!request->req_transaction) || (request->req_transaction->tra_callback_count > 0))
		return;

	if (request->req_transaction->tra_flags & TRA_perform_autocommit)
	{
		if (!(tdbb->getAttachment()->att_flags & ATT_no_db_triggers) &&
			!(request->req_transaction->tra_flags & TRA_prepared))
		{
			// run ON TRANSACTION COMMIT triggers
			run_commit_triggers(tdbb, request->req_transaction);
		}

		request->req_transaction->tra_flags &= ~TRA_perform_autocommit;
		TRA_commit(tdbb, request->req_transaction, true);
	}
}


static void successful_completion(IStatus* s, ISC_STATUS acceptCode = 0)
{
	fb_assert(s);

	const ISC_STATUS* status = s->get();

	// This assert validates whether we really have a successful status vector
	fb_assert(status[0] != isc_arg_gds || status[1] == FB_SUCCESS || status[1] == acceptCode);

	// Clear the status vector if it doesn't contain a warning
	if (status[0] != isc_arg_gds || status[1] != FB_SUCCESS || status[2] != isc_arg_warning)
	{
		/*if (return_code != FB_SUCCESS)
		{
			s->set(Arg::Gds(return_code).value());
		}
		else
		{*/
			s->init();
		//}
	}
}


// Stuff exception transliterated to the client charset.
ISC_STATUS transliterateException(thread_db* tdbb, const Exception& ex, IStatus* vector,
	const char* func) throw()
{
	ex.stuffException(vector);

	Jrd::Attachment* attachment = tdbb->getAttachment();
	if (func && attachment && attachment->att_trace_manager->needs(TRACE_EVENT_ERROR))
	{
		TraceConnectionImpl conn(attachment);
		TraceStatusVectorImpl traceStatus(vector->get());

		attachment->att_trace_manager->event_error(&conn, &traceStatus, func);
	}


	USHORT charSet;
	if (!attachment || (charSet = attachment->att_client_charset) == CS_METADATA ||
		charSet == CS_NONE)
	{
		return vector->get()[1];
	}

	// OK as long as we do not change vectors length
	// for current way of keeping strings in vector!

	ISC_STATUS* const vectorStart = const_cast<ISC_STATUS*>(vector->get());
	ISC_STATUS* status = vectorStart;
	Array<UCHAR*> buffers;

	try
	{
		bool cont = true;

		while (cont)
		{
			const ISC_STATUS type = *status++;

			switch (type)
			{
			case isc_arg_end:
				cont = false;
				break;

			case isc_arg_cstring:
				{
					size_t len = *status;
					const UCHAR* str = reinterpret_cast<UCHAR*>(status[1]);

					try
					{
						UCHAR* p = new UCHAR[len + 1];
						buffers.add(p);
						len = INTL_convert_bytes(tdbb, charSet, p, len, CS_METADATA, str, len, ERR_post);
						p[len] = '\0';
						str = p;
					}
					catch (const Exception&)
					{
					}

					*status++ = (ISC_STATUS) len;
					*status++ = (ISC_STATUS)(IPTR) str;
				}
				break;

			case isc_arg_string:
			case isc_arg_interpreted:
				{
					const UCHAR* str = reinterpret_cast<UCHAR*>(*status);
					size_t len = strlen((const char*) str);

					try
					{
						UCHAR* p = new UCHAR[len + 1];
						buffers.add(p);
						len = INTL_convert_bytes(tdbb, charSet, p, len, CS_METADATA, str, len, ERR_post);
						p[len] = '\0';
						str = p;
					}
					catch (const Exception&)
					{
					}

					*status++ = (ISC_STATUS)(IPTR) str;
				}
				break;

			default:
				++status;
				break;
			}
		}
	}
	catch (...)
	{
		return ex.stuff_exception(vectorStart);
	}

	makePermanentVector(vectorStart);

	for (Array<UCHAR*>::iterator i = buffers.begin(); i != buffers.end(); ++i)
		delete [] *i;

	return vectorStart[1];
}


const ULONG SWEEP_INTERVAL		= 20000;

const char DBL_QUOTE			= '\042';
const char SINGLE_QUOTE			= '\'';


static void trace_warning(thread_db* tdbb, IStatus* userStatus, const char* func)
{
	Jrd::Attachment* att = tdbb->getAttachment();
	if (!att)
		return;

	if (att->att_trace_manager->needs(TRACE_EVENT_ERROR))
	{
		TraceStatusVectorImpl traceStatus(userStatus->get());

		if (traceStatus.hasWarning())
		{
			TraceConnectionImpl conn(att);
			att->att_trace_manager->event_error(&conn, &traceStatus, func);
		}
	}
}


static void trace_failed_attach(TraceManager* traceManager, const char* filename,
	const DatabaseOptions& options, bool create, const ISC_STATUS* status)
{
	// Report to Trace API that attachment has not been created
	const char* origFilename = filename;
	if (options.dpb_org_filename.hasData())
		origFilename = options.dpb_org_filename.c_str();

	TraceFailedConnection conn(origFilename, &options);
	TraceStatusVectorImpl traceStatus(status);

	const ntrace_result_t result = (status[1] == isc_login || status[1] == isc_no_priv) ?
									res_unauthorized : res_failed;
	const char* func = create ? "JProvider::createDatabase" : "JProvider::attachDatabase";

	if (!traceManager)
	{
		TraceManager tempMgr(origFilename);

		if (tempMgr.needs(TRACE_EVENT_ATTACH))
			tempMgr.event_attach(&conn, create, result);

		if (tempMgr.needs(TRACE_EVENT_ERROR))
			tempMgr.event_error(&conn, &traceStatus, func);
	}
	else
	{
		if (traceManager->needs(TRACE_EVENT_ATTACH))
			traceManager->event_attach(&conn, create, result);

		if (traceManager->needs(TRACE_EVENT_ERROR))
			traceManager->event_error(&conn, &traceStatus, func);
	}
}


namespace Jrd {

JTransaction* JAttachment::getTransactionInterface(IStatus* status, ITransaction* tra)
{
	if (!tra)
		Arg::Gds(isc_bad_trans_handle).raise();

	status->init();

	// If validation is successfull, this means that this attachment and valid transaction
	// use same provider. I.e. the following cast is safe.
	JTransaction* jt = static_cast<JTransaction*>(tra->validate(status, this));
	if (!status->isSuccess())
		status_exception::raise(status->get());
	if (!jt)
		Arg::Gds(isc_bad_trans_handle).raise();

	return jt;
}

jrd_tra* JAttachment::getEngineTransaction(IStatus* status, ITransaction* tra)
{
	return getTransactionInterface(status, tra)->getHandle();
}

static void makeRoleName(Database* dbb, string& userIdRole, DatabaseOptions& options)
{
	if (userIdRole.isEmpty())
		return;

	switch (options.dpb_sql_dialect)
	{
	case 0:
		// V6 Client --> V6 Server, dummy client SQL dialect 0 was passed
		// It means that client SQL dialect was not set by user
		// and takes DB SQL dialect as client SQL dialect
		if (dbb->dbb_flags & DBB_DB_SQL_dialect_3)
		{
			// DB created in IB V6.0 by client SQL dialect 3
			options.dpb_sql_dialect = SQL_DIALECT_V6;
		}
		else
		{
			// old DB was gbaked in IB V6.0
			options.dpb_sql_dialect = SQL_DIALECT_V5;
		}
		break;

	case 99:
		// V5 Client --> V6 Server, old client has no concept of dialect
		options.dpb_sql_dialect = SQL_DIALECT_V5;
		break;

	default:
		// V6 Client --> V6 Server, but client SQL dialect was set
		// by user and was passed.
		break;
	}

	CharSet* utf8CharSet = IntlUtil::getUtf8CharSet();

	switch (options.dpb_sql_dialect)
	{
	case SQL_DIALECT_V5:
		strip_quotes(userIdRole);
		IntlUtil::toUpper(utf8CharSet, userIdRole);
		break;

	case SQL_DIALECT_V6_TRANSITION:
	case SQL_DIALECT_V6:
		if (userIdRole.hasData() && (userIdRole[0] == DBL_QUOTE || userIdRole[0] == SINGLE_QUOTE))
		{
			const char end_quote = userIdRole[0];
			// remove the delimited quotes and escape quote
			// from ROLE name
			userIdRole.erase(0, 1);
			for (string::iterator p = userIdRole.begin(); p < userIdRole.end(); ++p)
			{
				if (*p == end_quote)
				{
					if (++p < userIdRole.end() && *p == end_quote)
					{
						// skip the escape quote here
						userIdRole.erase(p--);
					}
					else
					{
						// delimited done
						userIdRole.erase(--p, userIdRole.end());
					}
				}
			}
		}
		else
			IntlUtil::toUpper(utf8CharSet, userIdRole);

		break;

	default:
		break;
	}
}

JAttachment* FB_CARG JProvider::attachDatabase(IStatus* user_status, const char* filename,
	unsigned int dpb_length, const unsigned char* dpb)
{
/**************************************
 *
 *	g d s _ $ a t t a c h _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Attach a moldy, grungy, old database
 *	sullied by user data.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		UserId userId;
		DatabaseOptions options;
		RefPtr<Config> config;
		bool invalid_client_SQL_dialect = false;
		PathName org_filename, expanded_name;
		bool is_alias = false;
		MutexEnsureUnlock guardDbInit(dbInitMutex, FB_FUNCTION);

		try
		{
			// Process database parameter block
			options.get(dpb, dpb_length, invalid_client_SQL_dialect);

			if (options.dpb_org_filename.hasData())
				org_filename = options.dpb_org_filename;
			else
			{
				org_filename = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(org_filename);

				ISC_unescape(org_filename);
			}

			ISC_utf8ToSystem(org_filename);

			// Resolve given alias name
			is_alias = expandDatabaseName(org_filename, expanded_name, &config);
			if (!is_alias)
			{
				expanded_name = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(expanded_name);

				ISC_unescape(expanded_name);
				ISC_utf8ToSystem(expanded_name);
			}

			// Check to see if the database is truly local
			if (ISC_check_if_remote(expanded_name, true))
			{
				handle_error(user_status, isc_unavailable);
			}

			// Check for correct credentials supplied
			getUserInfo(userId, options, org_filename.c_str(), expanded_name.c_str(), &config);

#ifdef WIN_NT
			guardDbInit.enter();		// Required to correctly expand name of just created database

			// Need to re-expand under lock to take into an account file existance (or not)
			is_alias = expandDatabaseName(org_filename, expanded_name, &config);
			if (!is_alias)
			{
				expanded_name = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(expanded_name);

				ISC_unescape(expanded_name);
				ISC_utf8ToSystem(expanded_name);
			}
#endif
		}
		catch (const Exception& ex)
		{
			ex.stuffException(user_status);
			trace_failed_attach(NULL, filename, options, false, user_status->get());
			throw;
		}

		// Check database against conf file.
		const VdnResult vdn = verifyDatabaseName(expanded_name, tdbb->tdbb_status_vector, is_alias);
		if (!is_alias && vdn == VDN_FAIL)
		{
			trace_failed_attach(NULL, filename, options, false, tdbb->tdbb_status_vector);
			status_exception::raise(tdbb->tdbb_status_vector);
		}

		Database* dbb = NULL;
		Jrd::Attachment* attachment = NULL;
		bool attachTraced = false;

		// Initialize special error handling
		try
		{
			// Check for ability to access requested DB remotely
			if (options.dpb_remote_address.hasData() && !config->getRemoteAccess())
			{
				ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("remote") <<
												  Arg::Str("database") <<
												  Arg::Str(org_filename));
			}

#ifndef	WIN_NT
			guardDbInit.enter();
#endif

			// Unless we're already attached, do some initialization
			RefMutexUnlock initGuard;
			JAttachment* jAtt = init(tdbb, expanded_name, is_alias ? org_filename : expanded_name,
				config, true, options, initGuard, pluginConfig);

			dbb = tdbb->getDatabase();
			fb_assert(dbb);
			attachment = tdbb->getAttachment();
			fb_assert(attachment);

			if (!(dbb->dbb_flags & DBB_new))
			{
				// That's already initialized DBB
				// No need keeping dbInitMutex any more
				guardDbInit.leave();
			}

			EngineContextHolder tdbb(user_status, jAtt, FB_FUNCTION, AttachmentHolder::ATT_DONT_LOCK);

			attachment->att_crypt_callback = getCryptCallback(cryptCallback);
			attachment->att_client_charset = attachment->att_charset = options.dpb_interp;

			if (options.dpb_no_garbage)
				attachment->att_flags |= ATT_no_cleanup;

			if (options.dpb_gbak_attach)
				attachment->att_flags |= ATT_gbak_attachment;

			if (options.dpb_gstat_attach)
				attachment->att_flags |= ATT_gstat_attachment;

			if (options.dpb_gfix_attach)
				attachment->att_flags |= ATT_gfix_attachment;

			if (options.dpb_working_directory.hasData())
				attachment->att_working_directory = options.dpb_working_directory;

			TRA_init(attachment);

			if (dbb->dbb_flags & DBB_new)
			{
				// If we're a not a secondary attachment, initialize some stuff

				// NS: Use alias as database ID only if accessing database using file name is not possible.
				//
				// This way we:
				// 1. Ensure uniqueness of ID even in presence of multiple processes
				// 2. Make sure that ID value can be used to connect back to database
				//
				if (is_alias && vdn == VDN_FAIL)
					dbb->dbb_database_name = org_filename;
				else
					dbb->dbb_database_name = expanded_name;

				PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
				pageSpace->file = PIO_open(dbb, expanded_name, org_filename);

				// Initialize the lock manager
				dbb->dbb_lock_mgr = LockManager::create(dbb->getUniqueFileId(), dbb->dbb_config);

				// Initialize locks
				LCK_init(tdbb, LCK_OWNER_database);
				LCK_init(tdbb, LCK_OWNER_attachment);
				init_database_lock(tdbb);

				jAtt->manualAsyncUnlock(attachment->att_flags);

				INI_init(tdbb);
				SHUT_init(tdbb);
				PAG_header_init(tdbb);
				INI_init2(tdbb);
				PAG_init(tdbb);

				if (options.dpb_set_page_buffers)
				{
					// Here we do not let anyone except SYSDBA (like DBO) to change dbb_page_buffers,
					// cause other flags is UserId can be set only when DB is opened.
					// No idea how to test for other cases before init is complete.
					if (config->getSharedDatabase() ? userId.locksmith() : true)
						dbb->dbb_page_buffers = options.dpb_page_buffers;
				}

				options.setBuffers(dbb->dbb_config);
				CCH_init(tdbb, options.dpb_buffers,
					config->getSharedCache() && !config->getSharedDatabase());

				dbb->dbb_tip_cache = FB_NEW(*dbb->dbb_permanent) TipCache(dbb);

				// Initialize backup difference subsystem. This must be done before WAL and shadowing
				// is enabled because nbackup it is a lower level subsystem
				dbb->dbb_backup_manager = FB_NEW(*dbb->dbb_permanent) BackupManager(tdbb, dbb, nbak_state_unknown);

				PAG_init2(tdbb, 0);
				PAG_header(tdbb, false);

				dbb->dbb_crypto_manager = FB_NEW(*dbb->dbb_permanent) CryptoManager(tdbb);
				dbb->dbb_crypto_manager->attach(tdbb, attachment);

				// initialize monitoring
				DatabaseSnapshot::initialize(tdbb);

				// initialize shadowing as soon as the database is ready for it
				// but before any real work is done
				SDW_init(tdbb, options.dpb_activate_shadow, options.dpb_delete_shadow);

				// linger
				dbb->dbb_linger_seconds = MET_get_linger(tdbb);

				// Init complete - we can release dbInitMutex
				dbb->dbb_flags &= ~DBB_new;
				guardDbInit.leave();
			}
			else
			{
				if ((dbb->dbb_flags & DatabaseOptions::DPB_FLAGS_MASK) !=
					(options.dpb_flags & DatabaseOptions::DPB_FLAGS_MASK))
				{
					// looks like someone tries to attach incompatibly
					Arg::Gds err(isc_bad_dpb_content);
					if ((dbb->dbb_flags & DBB_damaged) != (options.dpb_flags & DBB_damaged))
						err << Arg::Gds(isc_baddpb_damaged_mode);
					err.raise();
				}

				fb_assert(dbb->dbb_lock_mgr);

				LCK_init(tdbb, LCK_OWNER_attachment);
				jAtt->manualAsyncUnlock(attachment->att_flags);

				INI_init(tdbb);
				INI_init2(tdbb);
				PAG_header(tdbb, true);
				dbb->dbb_crypto_manager->attach(tdbb, attachment);
			}

			// Attachments to a ReadOnly database need NOT do garbage collection
			if (dbb->readOnly()) {
				attachment->att_flags |= ATT_no_cleanup;
			}

			if (options.dpb_nolinger)
				dbb->dbb_linger_seconds = 0;

			if (options.dpb_disable_wal)
			{
				ERR_post(Arg::Gds(isc_lock_timeout) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(org_filename));
			}

			if (options.dpb_buffers && !dbb->dbb_page_buffers) {
				CCH_expand(tdbb, options.dpb_buffers);
			}

			if (!options.dpb_verify && CCH_exclusive(tdbb, LCK_PW, LCK_NO_WAIT, NULL))
			{
				TRA_cleanup(tdbb);
			}

			if (invalid_client_SQL_dialect)
			{
				ERR_post(Arg::Gds(isc_inv_client_dialect_specified) << Arg::Num(options.dpb_sql_dialect) <<
						 Arg::Gds(isc_valid_client_dialects) << Arg::Str("1, 2 or 3"));
			}

			makeRoleName(dbb, userId.usr_sql_role_name, options);
			makeRoleName(dbb, userId.usr_trusted_role, options);

			options.dpb_sql_dialect = 0;

			SCL_init(tdbb, false, userId);

			// This pair (SHUT_database/SHUT_online) checks itself for valid user name
			if (options.dpb_shutdown)
			{
				SHUT_database(tdbb, options.dpb_shutdown, options.dpb_shutdown_delay, NULL);
			}
			if (options.dpb_online)
			{
				SHUT_online(tdbb, options.dpb_online, NULL);
			}

			// Check if another attachment has or is requesting exclusive database access.
			// If this is an implicit attachment for the security (password) database, don't
			// try to get exclusive attachment to avoid a deadlock condition which happens
			// when a client tries to connect to the security database itself.

			if (!options.dpb_sec_attach)
			{
				bool attachment_succeeded = true;
				if (dbb->dbb_ast_flags & DBB_shutdown_single)
					attachment_succeeded = CCH_exclusive_attachment(tdbb, LCK_none, -1, NULL);
				else
					CCH_exclusive_attachment(tdbb, LCK_none, LCK_WAIT, NULL);

				if (attachment->att_flags & ATT_shutdown)
				{
					if (dbb->dbb_ast_flags & DBB_shutdown) {
						ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(org_filename));
					}
					else {
						ERR_post(Arg::Gds(isc_att_shutdown));
					}
				}
				if (!attachment_succeeded) {
					ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(org_filename));
				}
			}

			// If database is shutdown then kick 'em out.

			if (dbb->dbb_ast_flags & (DBB_shut_attach | DBB_shut_tran))
			{
				ERR_post(Arg::Gds(isc_shutinprog) << Arg::Str(org_filename));
			}

			if (dbb->dbb_ast_flags & DBB_shutdown)
			{
				// Allow only SYSDBA/owner to access database that is shut down
				bool allow_access = attachment->locksmith();
				// Handle special shutdown modes
				if (allow_access)
				{
					if (dbb->dbb_ast_flags & DBB_shutdown_full)
					{
						// Full shutdown. Deny access always
						allow_access = false;
					}
					else if (dbb->dbb_ast_flags & DBB_shutdown_single)
					{
						// Single user maintenance. Allow access only if we were able to take exclusive lock
						// Note that logic below this exclusive lock differs for SS and CS builds:
						//   - CS keeps PW database lock from releasing in AST in single-user maintenance mode
						//   - for SS this code effectively checks that no other attachments are present
						//     at call point, ATT_exclusive bit is released just before this procedure exits
						// Things are done this way to handle return to online mode nicely.
						allow_access = CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD, NULL);
					}
				}
				if (!allow_access)
				{
					// Note we throw exception here when entering full-shutdown mode
					ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(org_filename));
				}
			}

			// Figure out what character set & collation this attachment prefers

			find_intl_charset(tdbb, attachment, &options);

			// if the attachment is through gbak and this attachment is not by owner
			// or sysdba then return error. This has been added here to allow for the
			// GBAK security feature of only allowing the owner or sysdba to backup a
			// database. smistry 10/5/98

			if (attachment->isUtility())
			{
				validateAccess(attachment);
			}

			if (options.dpb_verify)
			{
				validateAccess(attachment);
				if (!CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD, NULL))
					ERR_post(Arg::Gds(isc_bad_dpb_content) << Arg::Gds(isc_cant_validate));

				// Can't allow garbage collection during database validation.

				VIO_fini(tdbb);

				if (!VAL_validate(tdbb, options.dpb_verify))
					ERR_punt();
			}

			if (options.dpb_journal.hasData())
				ERR_post(Arg::Gds(isc_bad_dpb_content) << Arg::Gds(isc_cant_start_journal));

			if (options.dpb_wal_action)
			{
				// No WAL anymore. We deleted it.
				ERR_post(Arg::Gds(isc_no_wal));
			}

			if (attachment->att_flags & (ATT_gfix_attachment | ATT_gstat_attachment))
			{
				options.dpb_no_db_triggers = true;
			}

			if (options.dpb_no_db_triggers)
			{
				validateAccess(attachment);
				attachment->att_flags |= ATT_no_db_triggers;
			}

			if (options.dpb_set_db_sql_dialect)
			{
				validateAccess(attachment);
				PAG_set_db_SQL_dialect(tdbb, options.dpb_set_db_sql_dialect);
			}

			if (options.dpb_sweep_interval > -1)
			{
				validateAccess(attachment);
				PAG_sweep_interval(tdbb, options.dpb_sweep_interval);
				dbb->dbb_sweep_interval = options.dpb_sweep_interval;
			}

			if (options.dpb_set_force_write)
			{
				validateAccess(attachment);
				PAG_set_force_write(tdbb, options.dpb_force_write);
			}

			if (options.dpb_set_no_reserve)
			{
				validateAccess(attachment);
				PAG_set_no_reserve(tdbb, options.dpb_no_reserve);
			}

			if (options.dpb_set_page_buffers)
			{
				if (dbb->dbb_config->getSharedCache())
					validateAccess(attachment);

				if (attachment->locksmith())
					PAG_set_page_buffers(tdbb, options.dpb_page_buffers);
			}

			if (options.dpb_set_db_readonly)
			{
				validateAccess(attachment);
				if (!CCH_exclusive(tdbb, LCK_EX, WAIT_PERIOD, NULL))
				{
					ERR_post(Arg::Gds(isc_lock_timeout) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str(org_filename));
				}
				PAG_set_db_readonly(tdbb, options.dpb_db_readonly);
			}

			CCH_init2(tdbb);
			VIO_init(tdbb);

			PAG_attachment_id(tdbb);

			CCH_release_exclusive(tdbb);

			initGuard.leave();

			if (attachment->att_trace_manager->needs(TRACE_EVENT_ATTACH))
			{
				TraceConnectionImpl conn(attachment);
				attachment->att_trace_manager->event_attach(&conn, false, res_successful);
			}
			attachTraced = true;

			// Recover database after crash during backup difference file merge
			dbb->dbb_backup_manager->endBackup(tdbb, true); // true = do recovery

			if (options.dpb_sweep & isc_dpb_records) {
				TRA_sweep(tdbb);
			}

			dbb->dbb_crypto_manager->startCryptThread(tdbb);

			if (options.dpb_dbkey_scope)
				attachment->att_dbkey_trans = TRA_start(tdbb, 0, 0);

			if (!(attachment->att_flags & ATT_no_db_triggers))
			{
				jrd_tra* transaction = NULL;
				const ULONG save_flags = attachment->att_flags;

				try
				{
					// load all database triggers
					MET_load_db_triggers(tdbb, DB_TRIGGER_CONNECT);
					MET_load_db_triggers(tdbb, DB_TRIGGER_DISCONNECT);
					MET_load_db_triggers(tdbb, DB_TRIGGER_TRANS_START);
					MET_load_db_triggers(tdbb, DB_TRIGGER_TRANS_COMMIT);
					MET_load_db_triggers(tdbb, DB_TRIGGER_TRANS_ROLLBACK);

					// load DDL triggers
					MET_load_ddl_triggers(tdbb);

					const trig_vec* trig_connect = attachment->att_triggers[DB_TRIGGER_CONNECT];
					if (trig_connect && !trig_connect->isEmpty())
					{
						// Start a transaction to execute ON CONNECT triggers.
						// Ensure this transaction can't trigger auto-sweep.
						//// TODO: register the transaction in y-valve - for external engines
						attachment->att_flags |= ATT_no_cleanup;
						transaction = TRA_start(tdbb, 0, NULL);
						attachment->att_flags = save_flags;

						// run ON CONNECT triggers
						EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_connect);

						// and commit the transaction
						TRA_commit(tdbb, transaction, false);
					}
				}
				catch (const Exception&)
				{
					attachment->att_flags = save_flags;
					if (!(dbb->dbb_flags & DBB_bugcheck) && transaction)
						TRA_rollback(tdbb, transaction, false, false);
					throw;
				}
			}

			jAtt->manualUnlock(attachment->att_flags);

			return jAtt;
		}	// try
		catch (const Exception& ex)
		{
			ex.stuffException(user_status);
			if (attachTraced)
			{
				TraceManager* traceManager = attachment->att_trace_manager;
				TraceConnectionImpl conn(attachment);
				TraceStatusVectorImpl traceStatus(user_status->get());

				if (traceManager->needs(TRACE_EVENT_ERROR))
					traceManager->event_error(&conn, &traceStatus, "JProvider::attachDatabase");

				if (traceManager->needs(TRACE_EVENT_DETACH))
					traceManager->event_detach(&conn, false);
			}
			else
			{
				trace_failed_attach(attachment ? attachment->att_trace_manager : NULL,
					filename, options, false, user_status->get());
			}

			unwindAttach(tdbb, ex, user_status, attachment, dbb);
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
	}

	return NULL;
}


void JBlob::getInfo(IStatus* user_status,
				   unsigned int itemsLength, const unsigned char* items,
				   unsigned int bufferLength, unsigned char* buffer)
{
/**************************************
 *
 *	g d s _ $ b l o b _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on blob object.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			INF_blob_info(getHandle(), itemsLength, items, bufferLength, buffer);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::getInfo");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JBlob::cancel(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


void JBlob::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			getHandle()->BLB_cancel(tdbb);
			blob = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JEvents::cancel(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Cancel an outstanding event.
 *
 **************************************/
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


void JEvents::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Cancel an outstanding event.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			Database* const dbb = tdbb->getDatabase();

			if (dbb->dbb_event_mgr)
			{
				dbb->dbb_event_mgr->cancelEvents(id);
			}

			id = -1;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JEvents::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JAttachment::cancelOperation(IStatus* user_status, int option)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ o p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Try to cancel an operation.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION, AttachmentHolder::ATT_LOCK_ASYNC);

		try
		{
			JRD_cancel_operation(tdbb, getHandle(), option);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::cancelOperation");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JBlob::close(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c l o s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			getHandle()->BLB_close(tdbb);
			blob = NULL;
			release();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::close");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::commit(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c o m m i t
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			JRD_commit_transaction(tdbb, getHandle());
			transaction = NULL;
			release();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::commit");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::commitRetaining(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ c o m m i t _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			JRD_commit_retaining(tdbb, getHandle());
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::commitRetaining");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


ITransaction* FB_CARG JTransaction::join(IStatus* user_status, ITransaction* transaction)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		return DtcInterfacePtr()->join(user_status, this, transaction);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
	}
	return NULL;
}

JTransaction* FB_CARG JTransaction::validate(IStatus* user_status, IAttachment* testAtt)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		// Do not raise error in status - just return NULL if attachment does not match
		return jAtt == testAtt ? this : NULL;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
	}
	return NULL;
}

JTransaction* FB_CARG JTransaction::enterDtc(IStatus* user_status)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		JTransaction* copy = new JTransaction(this);
		copy->addRef();

		transaction = NULL;
		release();

		return copy;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
	}
	return NULL;
}

JRequest* JAttachment::compileRequest(IStatus* user_status,
	unsigned int blr_length, const unsigned char* blr)
{
/**************************************
 *
 *	g d s _ $ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	JrdStatement* stmt = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		TraceBlrCompile trace(tdbb, blr_length, blr);
		try
		{
			jrd_req* request = NULL;
			JRD_compile(tdbb, getHandle(), &request, blr_length, blr, RefStrPtr(), 0, NULL, false);
			stmt = request->getStatement();

			trace.finish(request, res_successful);
		}
		catch (const Exception& ex)
		{
			const ISC_STATUS exc = transliterateException(tdbb, ex, user_status, "JAttachment::compileRequest");
			const bool no_priv = (exc == isc_no_priv);
			trace.finish(NULL, no_priv ? res_unauthorized : res_failed);

			return NULL;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);

	JRequest* jr = new JRequest(stmt, this);
	stmt->interface = jr;
	jr->addRef();
	return jr;
}


JBlob* JAttachment::createBlob(IStatus* user_status, ITransaction* tra, ISC_QUAD* blob_id,
	unsigned int bpb_length, const unsigned char* bpb)
{
/**************************************
 *
 *	g d s _ $ c r e a t e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Create a new blob.
 *
 **************************************/
	blb* blob = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		validateHandle(tdbb, getEngineTransaction(user_status, tra));

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);
			blob = blb::create2(tdbb, transaction, reinterpret_cast<bid*>(blob_id), bpb_length, bpb);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::createBlob");
			return NULL;
		}
	}
	catch (const Exception& ex)
	{
		 ex.stuffException(user_status);
		 return NULL;
	}

	successful_completion(user_status);

	JBlob* jb = new JBlob(blob, this);
	jb->addRef();
	blob->blb_interface = jb;
	return jb;
}


JAttachment* FB_CARG JProvider::createDatabase(IStatus* user_status, const char* filename,
	unsigned int dpb_length, const unsigned char* dpb)
{
/**************************************
 *
 *	g d s _ $ c r e a t e _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Create a nice, squeeky clean database, uncorrupted by user data.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);
		MutexEnsureUnlock guardDbInit(dbInitMutex, FB_FUNCTION);

		UserId userId;
		DatabaseOptions options;
		PathName org_filename, expanded_name;
		bool is_alias = false;
		Firebird::RefPtr<Config> config;

		try
		{
			// Process database parameter block
			bool invalid_client_SQL_dialect = false;
			options.get(dpb, dpb_length, invalid_client_SQL_dialect);
			if (!invalid_client_SQL_dialect && options.dpb_sql_dialect == 99) {
				options.dpb_sql_dialect = 0;
			}

			if (options.dpb_org_filename.hasData())
				org_filename = options.dpb_org_filename;
			else
			{
				org_filename = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(org_filename);

				ISC_unescape(org_filename);
			}

			ISC_utf8ToSystem(org_filename);

			// Resolve given alias name
			is_alias = expandDatabaseName(org_filename, expanded_name, &config);
			if (!is_alias)
			{
				expanded_name = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(expanded_name);

				ISC_unescape(expanded_name);
				ISC_utf8ToSystem(expanded_name);
			}

			// Check to see if the database is truly local or if it just looks
			// that way
			if (ISC_check_if_remote(expanded_name, true))
			{
				handle_error(user_status, isc_unavailable);
			}

			// Check for correct credentials supplied
			getUserInfo(userId, options, org_filename.c_str(), NULL, &config);

#ifdef WIN_NT
			guardDbInit.enter();		// Required to correctly expand name of just created database

			// Need to re-expand under lock to take into an account file existance (or not)
			is_alias = expandDatabaseName(org_filename, expanded_name, &config);
			if (!is_alias)
			{
				expanded_name = filename;

				if (!options.dpb_utf8_filename)
					ISC_systemToUtf8(expanded_name);

				ISC_unescape(expanded_name);
				ISC_utf8ToSystem(expanded_name);
			}
#endif
		}
		catch (const Exception& ex)
		{
			ex.stuffException(user_status);
			trace_failed_attach(NULL, filename, options, true, user_status->get());
			throw;
		}

		// Check database against conf file.
		const VdnResult vdn = verifyDatabaseName(expanded_name, tdbb->tdbb_status_vector, is_alias);
		if (!is_alias && vdn == VDN_FAIL)
		{
			trace_failed_attach(NULL, filename, options, true, tdbb->tdbb_status_vector);
			status_exception::raise(tdbb->tdbb_status_vector);
		}

		Database* dbb = NULL;
		Jrd::Attachment* attachment = NULL;

		// Initialize special error handling
		try
		{
			// Check for ability to access requested DB remotely
			if (options.dpb_remote_address.hasData() && !config->getRemoteAccess())
			{
				ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("remote") <<
												  Arg::Str("database") <<
												  Arg::Str(org_filename));
			}

#ifndef WIN_NT
			guardDbInit.enter();
#endif

			// Unless we're already attached, do some initialization
			RefMutexUnlock initGuard;
			JAttachment* jAtt = init(tdbb, expanded_name, (is_alias ? org_filename : expanded_name),
				config, false, options, initGuard, pluginConfig);

			dbb = tdbb->getDatabase();
			fb_assert(dbb);
			fb_assert(dbb->dbb_flags & DBB_new);
			fb_assert(dbb->dbb_flags & DBB_creating);
			attachment = tdbb->getAttachment();
			fb_assert(attachment);

			Sync dbbGuard(&dbb->dbb_sync, "createDatabase");
			dbbGuard.lock(SYNC_EXCLUSIVE);

			EngineContextHolder tdbb(user_status, jAtt, FB_FUNCTION, AttachmentHolder::ATT_DONT_LOCK);

			attachment->att_crypt_callback = getCryptCallback(cryptCallback);

			if (options.dpb_working_directory.hasData()) {
				attachment->att_working_directory = options.dpb_working_directory;
			}

			if (options.dpb_gbak_attach) {
				attachment->att_flags |= ATT_gbak_attachment;
			}

			if (options.dpb_no_db_triggers)
				attachment->att_flags |= ATT_no_db_triggers;

			switch (options.dpb_sql_dialect)
			{
			case SQL_DIALECT_V5:
				break;
			case 0:
			case SQL_DIALECT_V6:
				dbb->dbb_flags |= DBB_DB_SQL_dialect_3;
				break;
			default:
				ERR_post(Arg::Gds(isc_database_create_failed) << Arg::Str(expanded_name) <<
						 Arg::Gds(isc_inv_dialect_specified) << Arg::Num(options.dpb_sql_dialect) <<
						 Arg::Gds(isc_valid_db_dialects) << Arg::Str("1 and 3"));
				break;
			}

			attachment->att_client_charset = attachment->att_charset = options.dpb_interp;

			if (!options.dpb_page_size) {
				options.dpb_page_size = DEFAULT_PAGE_SIZE;
			}

			USHORT page_size = MIN_NEW_PAGE_SIZE;
			for (; page_size < MAX_PAGE_SIZE; page_size <<= 1)
			{
				if (options.dpb_page_size < page_size << 1)
					break;
			}

			dbb->dbb_page_size = (page_size > MAX_PAGE_SIZE) ? MAX_PAGE_SIZE : page_size;

			TRA_init(attachment);

			PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			try
			{
				// try to create with overwrite = false
				pageSpace->file = PIO_create(dbb, expanded_name, false, false);
			}
			catch (status_exception)
			{
				if (options.dpb_overwrite)
				{
					// isc_dpb_no_db_triggers is required for 2 reasons
					// - it disables non-DBA attaches with isc_adm_task_denied error
					// - it disables any user code to be executed when we later lock
					//   databases_mutex with OverwriteHolder
					ClumpletWriter dpbWriter(ClumpletReader::dpbList, MAX_DPB_SIZE, dpb, dpb_length);
					dpbWriter.insertByte(isc_dpb_no_db_triggers, 1);
					dpb = dpbWriter.getBuffer();
					dpb_length = dpbWriter.getBufferLength();

					OverwriteHolder overwriteCheckHolder(dbb);

					JAttachment* attachment2 = attachDatabase(user_status, filename, dpb_length, dpb);
					if (user_status->get()[1] == isc_adm_task_denied)
					{
						throw;
					}

					bool allow_overwrite = false;

					if (attachment2)
					{
						allow_overwrite = attachment2->getHandle()->att_user->locksmith();
						attachment2->detach(user_status);
					}
					else
					{
						// clear status after failed attach
						user_status->init();
						allow_overwrite = true;
					}

					if (allow_overwrite)
					{
						// file is a database and the user (SYSDBA or owner) has right to overwrite
						pageSpace->file = PIO_create(dbb, expanded_name, options.dpb_overwrite, false);
					}
					else
					{
						ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("overwrite") <<
														  Arg::Str("database") <<
														  Arg::Str(expanded_name));
					}
				}
				else
					throw;
			}

#ifdef WIN_NT
			dbb->dbb_filename.assign(pageSpace->file->fil_string);	// first dbb file
#endif

			// Initialize the lock manager
			dbb->dbb_lock_mgr = LockManager::create(dbb->getUniqueFileId(), dbb->dbb_config);

			// Initialize locks
			LCK_init(tdbb, LCK_OWNER_database);
			LCK_init(tdbb, LCK_OWNER_attachment);
			init_database_lock(tdbb);

			jAtt->manualAsyncUnlock(attachment->att_flags);

			INI_init(tdbb);
			PAG_init(tdbb);

			SCL_init(tdbb, true, userId);

			if (options.dpb_set_page_buffers)
				dbb->dbb_page_buffers = options.dpb_page_buffers;

			options.setBuffers(dbb->dbb_config);
			CCH_init(tdbb, options.dpb_buffers,
				config->getSharedCache() && !config->getSharedDatabase());

			// NS: Use alias as database ID only if accessing database using file name is not possible.
			//
			// This way we:
			// 1. Ensure uniqueness of ID even in presence of multiple processes
			// 2. Make sure that ID value can be used to connect back to database
			//
			if (is_alias && vdn == VDN_FAIL)
				dbb->dbb_database_name = org_filename;
			else
				dbb->dbb_database_name = dbb->dbb_filename;

			dbb->dbb_tip_cache = FB_NEW(*dbb->dbb_permanent) TipCache(dbb);

			// Initialize backup difference subsystem. This must be done before WAL and shadowing
			// is enabled because nbackup it is a lower level subsystem
			dbb->dbb_backup_manager = FB_NEW(*dbb->dbb_permanent) BackupManager(tdbb, dbb, nbak_state_normal);
			dbb->dbb_backup_manager->dbCreating = true;

			PAG_format_header(tdbb);
			INI_init2(tdbb);
			PAG_format_pip(tdbb, *pageSpace);

			dbb->dbb_crypto_manager = FB_NEW(*dbb->dbb_permanent) CryptoManager(tdbb);

			// initialize monitoring
			DatabaseSnapshot::initialize(tdbb);

			if (options.dpb_set_page_buffers)
				PAG_set_page_buffers(tdbb, options.dpb_page_buffers);

			if (options.dpb_set_no_reserve)
				PAG_set_no_reserve(tdbb, options.dpb_no_reserve);

			INI_format(attachment->att_user->usr_user_name.c_str(), options.dpb_set_db_charset.c_str());

			// If we have not allocated first TIP page, do it now.
			if (!dbb->dbb_t_pages || !dbb->dbb_t_pages->count())
				TRA_extend_tip(tdbb, 0);

			// There is no point to move database online at database creation since it is online by default.
			// We do not allow to create database that is fully shut down.
			if (options.dpb_online || (options.dpb_shutdown & isc_dpb_shut_mode_mask) == isc_dpb_shut_full)
				ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(org_filename));

			if (options.dpb_shutdown) {
				SHUT_database(tdbb, options.dpb_shutdown, options.dpb_shutdown_delay, &dbbGuard);
			}

			if (options.dpb_sweep_interval > -1)
			{
				PAG_sweep_interval(tdbb, options.dpb_sweep_interval);
				dbb->dbb_sweep_interval = options.dpb_sweep_interval;
			}

			if (options.dpb_set_force_write)
				PAG_set_force_write(tdbb, options.dpb_force_write);

			// initialize shadowing semaphore as soon as the database is ready for it
			// but before any real work is done

			SDW_init(tdbb, options.dpb_activate_shadow, options.dpb_delete_shadow);

			CCH_init2(tdbb);
			VIO_init(tdbb);

			if (options.dpb_set_db_readonly)
			{
				if (!CCH_exclusive(tdbb, LCK_EX, WAIT_PERIOD, &dbbGuard))
				{
					ERR_post(Arg::Gds(isc_lock_timeout) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str(org_filename));
				}

				PAG_set_db_readonly(tdbb, options.dpb_db_readonly);
			}

			PAG_attachment_id(tdbb);

			CCH_release_exclusive(tdbb);

			// Figure out what character set & collation this attachment prefers

			find_intl_charset(tdbb, attachment, &options);

			CCH_flush(tdbb, FLUSH_FINI, 0);

			if (!options.dpb_set_force_write)
				PAG_set_force_write(tdbb, true);

			dbb->dbb_backup_manager->dbCreating = false;

			// Init complete - we can release dbInitMutex
			dbb->dbb_flags &= ~(DBB_new | DBB_creating);
			guardDbInit.leave();

			// Report that we created attachment to Trace API
			if (attachment->att_trace_manager->needs(TRACE_EVENT_ATTACH))
			{
				TraceConnectionImpl conn(attachment);
				attachment->att_trace_manager->event_attach(&conn, true, res_successful);
			}

			jAtt->manualUnlock(attachment->att_flags);

			return jAtt;
		}	// try
		catch (const Exception& ex)
		{
			ex.stuffException(user_status);
			trace_failed_attach(attachment ? attachment->att_trace_manager : NULL,
				filename, options, true, user_status->get());

			unwindAttach(tdbb, ex, user_status, attachment, dbb);
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
	}

	return NULL;
}


void JAttachment::getInfo(IStatus* user_status, unsigned int item_length, const unsigned char* items,
	unsigned int buffer_length, unsigned char* buffer)
{
/**************************************
 *
 *	g d s _ $ d a t a b a s e _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on database object.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			INF_database_info(tdbb, item_length, items, buffer_length, buffer);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::getInfo");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JAttachment::executeDyn(IStatus* status, ITransaction* /*tra*/, unsigned int /*length*/,
	const unsigned char* /*dyn*/)
{
/**************************************
 *
 *	g d s _ $ d d l
 *
 **************************************
 *
 * This function is deprecated and "removed".
 *
 **************************************/
	status->set((Arg::Gds(isc_feature_removed) << Arg::Str("isc_ddl")).value());
}


void JAttachment::detach(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down a database.
 *
 **************************************/
	freeEngineData(user_status);

	if (user_status->isSuccess())
	{
		RefDeb(DEB_RLS_JATT, "JAttachment::detach");
		release();
	}
}


void JAttachment::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	f r e e E n g i n e D a t a
 *
 **************************************
 *
 * Functional description
 *	Close down a database.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		Jrd::Attachment* const attachment = getHandle();
		Database* const dbb = tdbb->getDatabase();

		try
		{
			if (attachment->att_in_use)
				status_exception::raise(Arg::Gds(isc_attachment_in_use));

			unsigned flags = PURGE_LINGER;

			if (engineShutdown ||
				(dbb->dbb_ast_flags & DBB_shutdown) ||
				(attachment->att_flags & ATT_shutdown))
			{
				flags |= PURGE_FORCE;
			}

			attachment->signalShutdown();
			purge_attachment(tdbb, this, flags);

			att = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JAttachment::dropDatabase(IStatus* user_status)
{
/**************************************
 *
 *	i s c _ d r o p _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Close down and purge a database.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION, AttachmentHolder::ATT_LOCK_ASYNC);
		Attachment* attachment = getHandle();
		Database* const dbb = tdbb->getDatabase();

		try
		{
			MutexEnsureUnlock guard(*getMutex(), FB_FUNCTION);
			if (!guard.tryEnter())
			{
				status_exception::raise(Arg::Gds(isc_attachment_in_use));
			}

			// Prepare to set ODS to 0
   			WIN window(HEADER_PAGE_NUMBER);
			Ods::header_page* header = NULL;

			try
			{
				Sync sync(&dbb->dbb_sync, "JAttachment::dropDatabase()");

				if (attachment->att_in_use || attachment->att_use_count)
					status_exception::raise(Arg::Gds(isc_attachment_in_use));

				const PathName& file_name = attachment->att_filename;

				if (!attachment->locksmith())
				{
					ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("drop") <<
													  Arg::Str("database") <<
													  Arg::Str(file_name));
				}

				if (attachment->att_flags & ATT_shutdown)
				{
					if (dbb->dbb_ast_flags & DBB_shutdown)
					{
						ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(file_name));
					}
					else
					{
						ERR_post(Arg::Gds(isc_att_shutdown));
					}
				}

				if (!CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD, NULL))
				{
					ERR_post(Arg::Gds(isc_lock_timeout) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str(file_name));
				}

				// Lock header page before taking database lock
				header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);

				// Check if same process has more attachments
				sync.lock(SYNC_EXCLUSIVE);
				if (dbb->dbb_attachments && dbb->dbb_attachments->att_next)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
				}

				// dbb->dbb_extManager.closeAttachment(tdbb, attachment);
				// To be reviewed by Adriano - it will be anyway called in release_attachment

				// Forced release of all transactions
				purge_transactions(tdbb, attachment, true);

				tdbb->tdbb_flags |= TDBB_detaching;

				// Here we have database locked in exclusive mode.
				// Just mark the header page with an 0 ods version so that no other
				// process can attach to this database once we release our exclusive
				// lock and start dropping files.
				CCH_MARK_MUST_WRITE(tdbb, &window);
				header->hdr_ods_version = 0;
				header = NULL;		// In case of exception in CCH_RELEASE() do not repeat it in catch
				CCH_RELEASE(tdbb, &window);

				// Notify Trace API manager about successful drop of database
				if (attachment->att_trace_manager->needs(TRACE_EVENT_DETACH))
				{
					TraceConnectionImpl conn(attachment);
					attachment->att_trace_manager->event_detach(&conn, true);
				}
			}
			catch (const Exception&)
			{
				if (header)
				{
					CCH_RELEASE(tdbb, &window);
				}
				CCH_release_exclusive(tdbb);
				throw;
			}

			// Unlink attachment from database
			release_attachment(tdbb, attachment);
			att = NULL;
			attachment = NULL;
			guard.leave();

			PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			const jrd_file* file = pageSpace->file;
			const Shadow* shadow = dbb->dbb_shadow;

			if (JRD_shutdown_database(dbb))
			{
				// This point on database is useless

				// drop the files here
				bool err = drop_files(file);
				for (; shadow; shadow = shadow->sdw_next)
				{
					err = drop_files(shadow->sdw_file) || err;
				}

				tdbb->setDatabase(NULL);
				Database::destroy(dbb);

				if (err)
				{
					user_status->set(Arg::Gds(isc_drdb_completed_with_errs).value());
				}
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::drop");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status, isc_drdb_completed_with_errs);
}


unsigned int JBlob::getSegment(IStatus* user_status, unsigned int buffer_length, void* buffer)
{
/**************************************
 *
 *	g d s _ $ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get a segment from a blob.
 *
 **************************************/
	unsigned int len = 0;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			len = getHandle()->BLB_get_segment(tdbb, buffer, buffer_length);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::getSegment");
			return len;
		}

		// Don't trace errors below as it is not real errors but kind of return value

		if (getHandle()->blb_flags & BLB_eof)
			status_exception::raise(Arg::Gds(isc_segstr_eof));
		else if (getHandle()->getFragmentSize())
			status_exception::raise(Arg::Gds(isc_segment));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return len;
	}

	successful_completion(user_status);

	return len;
}


int JAttachment::getSlice(IStatus* user_status, ITransaction* tra, ISC_QUAD* array_id,
	unsigned int /*sdl_length*/, const unsigned char* sdl, unsigned int param_length,
	const unsigned char* param, int slice_length, unsigned char* slice)
{
/**************************************
 *
 *	g d s _ $ g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Snatch a slice of an array.
 *
 **************************************/
	int return_length = 0;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		validateHandle(tdbb, getEngineTransaction(user_status, tra));

		check_database(tdbb);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);

			if (!array_id->gds_quad_low && !array_id->gds_quad_high)
				MOVE_CLEAR(slice, slice_length);
			else
			{
				return_length = blb::get_slice(tdbb, transaction, reinterpret_cast<bid*>(array_id),
											  sdl, param_length, param, slice_length, slice);
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::getSlice");
			return return_length;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return return_length;
	}

	successful_completion(user_status);

	return return_length;
}


JBlob* JAttachment::openBlob(IStatus* user_status, ITransaction* tra, ISC_QUAD* blob_id,
	unsigned int bpb_length, const unsigned char* bpb)
{
/**************************************
 *
 *	g d s _ $ o p e n _ b l o b 2
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob.
 *
 **************************************/
	blb* blob = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		validateHandle(tdbb, getEngineTransaction(user_status, tra));

		check_database(tdbb);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);
			blob = blb::open2(tdbb, transaction, reinterpret_cast<bid*>(blob_id),
				bpb_length, bpb, true);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::openBlob");
			return NULL;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);

	JBlob* jb = new JBlob(blob, this);
	jb->addRef();
	blob->blb_interface = jb;
	return jb;
}


void JTransaction::prepare(IStatus* user_status, unsigned int msg_length, const unsigned char* msg)
{
/**************************************
 *
 *	g d s _ $ p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	Prepare a transaction for commit.  First phase of a two
 *	phase commit.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			prepare_tra(tdbb, getHandle(), msg_length, msg);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::prepare");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JBlob::putSegment(IStatus* user_status, unsigned int buffer_length, const void* buffer)
{
/**************************************
 *
 *	g d s _ $ p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			getHandle()->BLB_put_segment(tdbb, buffer, buffer_length);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::putSegment");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JAttachment::putSlice(IStatus* user_status, ITransaction* tra, ISC_QUAD* array_id,
	unsigned int /*sdlLength*/, const unsigned char* sdl, unsigned int paramLength,
	const unsigned char* param, int sliceLength, unsigned char* slice)
{
/**************************************
 *
 *	g d s _ $ p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Snatch a slice of an array.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		validateHandle(tdbb, getEngineTransaction(user_status, tra));

		check_database(tdbb);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);
			blb::put_slice(tdbb, transaction, reinterpret_cast<bid*>(array_id),
				sdl, paramLength, param, sliceLength, slice);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::putSlice");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


JEvents* JAttachment::queEvents(IStatus* user_status, IEventCallback* callback,
	unsigned int length, const unsigned char* events)
{
/**************************************
 *
 *	g d s _ $ q u e _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Que a request for event notification.
 *
 **************************************/
	JEvents* ev = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			Database* const dbb = tdbb->getDatabase();

			EventManager::init(getHandle());

			int id = dbb->dbb_event_mgr->queEvents(getHandle()->att_event_session,
												   length, events, callback);
			ev = new JEvents(id, this, callback);
			ev->addRef();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::queEvents");
			return ev;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return ev;
	}

	successful_completion(user_status);

	return ev;
}


void JRequest::receive(IStatus* user_status, int level, unsigned int msg_type,
					   unsigned int msg_length, unsigned char* msg)
{
/**************************************
 *
 *	g d s _ $ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Send a record to the host program.
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		jrd_req* request = verify_request_synchronization(getHandle(), level);

		try
		{
			JRD_receive(tdbb, request, msg_type, msg_length, msg);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::receive");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


JTransaction* JAttachment::reconnectTransaction(IStatus* user_status, unsigned int length,
	const unsigned char* id)
{
/**************************************
 *
 *	g d s _ $ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Connect to a transaction in limbo.
 *
 **************************************/
	jrd_tra* tra = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			tra = TRA_reconnect(tdbb, id, length);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::reconnectTransaction");
			return NULL;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);

	JTransaction* jt = new JTransaction(tra, this);
	tra->setInterface(jt);
	jt->addRef();
	return jt;
}


void JRequest::free(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request.
 *
 **************************************/
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


void JRequest::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			getHandle()->release(tdbb);
			rq = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JRequest::getInfo(IStatus* user_status, int level, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
/**************************************
 *
 *	g d s _ $ r e q u e s t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on blob object.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		jrd_req* request = verify_request_synchronization(getHandle(), level);

		try
		{
			INF_request_info(request, itemsLength, items, bufferLength, buffer);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::getInfo");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::rollbackRetaining(IStatus* user_status)
{
/**************************************
 *
 *	i s c _ r o l l b a c k _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction but keep the environment valid
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			JRD_rollback_retaining(tdbb, getHandle());
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::rollbackRetaining");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::rollback(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ r o l l b a c k
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			JRD_rollback_transaction(tdbb, getHandle());
			transaction = NULL;
			release();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::rollback");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::disconnect(IStatus* user_status)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		// ASF: Looks wrong that this method is ignored in the engine and remote providers.
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


int JBlob::seek(IStatus* user_status, int mode, int offset)
{
/**************************************
 *
 *	g d s _ $ s e e k _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Seek a stream blob.
 *
 **************************************/
	int result = -1;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			result = getHandle()->BLB_lseek(mode, offset);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JBlob::seek");
			return result;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return result;
	}

	successful_completion(user_status);

	return result;
}


void JRequest::send(IStatus* user_status, int level, unsigned int msg_type,
	unsigned int msg_length, const unsigned char* msg)
{
/**************************************
 *
 *	g d s _ $ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		jrd_req* request = verify_request_synchronization(getHandle(), level);

		try
		{
			JRD_send(tdbb, request, msg_type, msg_length, msg);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::send");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


JService* JProvider::attachServiceManager(IStatus* user_status, const char* service_name,
	unsigned int spbLength, const unsigned char* spb)
{
/**************************************
 *
 *	g d s _ $ s e r v i c e _ a t t a c h
 *
 **************************************
 *
 * Functional description
 *	Connect to a Firebird service.
 *
 **************************************/
	JService* jSvc = NULL;

	try
	{
		ThreadContextHolder tdbb(user_status);

		Service* svc = new Service(service_name, spbLength, spb, cryptCallback);
		jSvc = new JService(svc);
		svc->jSvc = jSvc;
		jSvc->addRef();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return jSvc;
	}

	successful_completion(user_status);

	return jSvc;
}


void JService::detach(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ s e r v i c e _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


void JService::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	g d s _ $ s e r v i c e _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		validateHandle(svc);

		svc->detach();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JService::query(IStatus* user_status,
				unsigned int sendLength, const unsigned char* sendItems,
				unsigned int receiveLength, const unsigned char* receiveItems,
				unsigned int bufferLength, unsigned char* buffer)
{
/**************************************
 *
 *	g d s _ $ s e r v i c e _ q u e r y
 *
 **************************************
 *
 * Functional description
 *	Provide information on service object.
 *
 *	NOTE: The parameter RESERVED must not be used
 *	for any purpose as there are networking issues
 *	involved (as with any handle that goes over the
 *	network).  This parameter will be implemented at
 *	a later date.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		validateHandle(svc);

		if (svc->getVersion() == isc_spb_version1)
		{
			svc->query(sendLength, sendItems, receiveLength,
					   receiveItems, bufferLength, buffer);
		}
		else
		{
			// For SVC_query2, we are going to completly dismantle user_status (since at this point it is
			// meaningless anyway).  The status vector returned by this function can hold information about
			// the call to query the service manager and/or a service thread that may have been running.

			svc->query2(tdbb, sendLength, sendItems, receiveLength,
					    receiveItems, bufferLength, buffer);

			// If there is a status vector from a service thread, copy it into the thread status
			size_t len, warning;
			PARSE_STATUS(svc->getStatus(), len, warning);

			if (len)
			{
				user_status->set(len, svc->getStatus());
				// Empty out the service status vector
				svc->initStatus();
				return;
			}
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JService::start(IStatus* user_status, unsigned int spbLength, const unsigned char* spb)
{
/**************************************
 *
 *	g d s _ s e r v i c e _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start the specified service
 *
 *	NOTE: The parameter RESERVED must not be used
 *	for any purpose as there are networking issues
 *  	involved (as with any handle that goes over the
 *   	network).  This parameter will be implemented at
 * 	a later date.
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		validateHandle(svc);

		svc->start(spbLength, spb);

		if (svc->getStatus()[1])
		{
			user_status->set(svc->getStatus());
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JRequest::startAndSend(IStatus* user_status, ITransaction* tra, int level,
	unsigned int msg_type, unsigned int msg_length, const unsigned char* msg)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ a n d _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		validateHandle(tdbb, getAttachment()->getEngineTransaction(user_status, tra));

		check_database(tdbb);

		jrd_req* request = getHandle()->getRequest(tdbb, level);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);

			TraceBlrExecute trace(tdbb, request);
			try
			{
				JRD_start_and_send(tdbb, request, transaction, msg_type, msg_length, msg);

				// Notify Trace API about blr execution
				trace.finish(res_successful);
			}
			catch (const Exception& ex)
			{
				const ISC_STATUS exc = transliterateException(tdbb, ex, user_status, "JRequest::startAndSend");
				const bool no_priv = (exc == isc_login || exc == isc_no_priv);
				trace.finish(no_priv ? res_unauthorized : res_failed);

				return;
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::startAndSend");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JRequest::start(IStatus* user_status, ITransaction* tra, int level)
{
/**************************************
 *
 *	g d s _ $ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		if (!tra)
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));

		validateHandle(tdbb, getAttachment()->getEngineTransaction(user_status, tra));

		check_database(tdbb);

		jrd_req* request = getHandle()->getRequest(tdbb, level);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);

			TraceBlrExecute trace(tdbb, request);
			try
			{
				JRD_start(tdbb, request, transaction);
				trace.finish(res_successful);
			}
			catch (const Exception& ex)
			{
				const ISC_STATUS exc = transliterateException(tdbb, ex, user_status, "JRequest::start");
				const bool no_priv = (exc == isc_login || exc == isc_no_priv);
				trace.finish(no_priv ? res_unauthorized : res_failed);

				return;
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::start");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JProvider::shutdown(IStatus* status, unsigned int timeout, const int reason)
{
/**************************************
 *
 *	G D S _ S H U T D O W N
 *
 **************************************
 *
 * Functional description
 *	Rollback every transaction, release
 *	every attachment, and shutdown every
 *	database.
 *
 **************************************/
	try
	{
		MutexLockGuard guard(shutdownMutex, FB_FUNCTION);

		if (engineShutdown)
		{
			return;
		}
		{ // scope
			MutexLockGuard guard(newAttachmentMutex, FB_FUNCTION);
			engineShutdown = true;
		}

		ThreadContextHolder tdbb;

		ULONG attach_count, database_count, svc_count;
		JRD_enum_attachments(NULL, attach_count, database_count, svc_count);

		if (attach_count > 0 || svc_count > 0)
		{
			gds__log("Shutting down the server with %d active connection(s) to %d database(s), "
					 "%d active service(s)",
				attach_count, database_count, svc_count);
		}

		if (reason == fb_shutrsn_exit_called)
		{
			// Starting threads may fail when task is going to close.
			// This happens at least with some microsoft C runtimes.
			// If people wish to have timeout, they should better call fb_shutdown() themselves.
			// Therefore:
			timeout = 0;
		}

		if (timeout)
		{
			Semaphore shutdown_semaphore;

			Thread::Handle h;
			Thread::start(shutdown_thread, &shutdown_semaphore, THREAD_medium, &h);

			if (!shutdown_semaphore.tryEnter(0, timeout))
			{
				// sad, but we MUST kill shutdown_thread because engine DLL\SO is unloaded
				// else whole process will be crashed
				Thread::kill(h);
				status_exception::raise(Arg::Gds(isc_shutdown_timeout));
			}

			Thread::waitForCompletion(h);
		}
		else
		{
			shutdown_thread(NULL);
		}

		// Do not put it into separate shutdown thread - during shutdown of TraceManager
		// PluginManager wants to lock a mutex, which is sometimes already locked in current thread
		TraceManager::shutdown();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
		gds__log_status(NULL, status->get());
	}
}


void JProvider::setDbCryptCallback(IStatus* status, ICryptKeyCallback* callback)
{
	status->init();
	cryptCallback = callback;
}


JTransaction* JAttachment::startTransaction(IStatus* user_status,
	unsigned int tpbLength, const unsigned char* tpb)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	jrd_tra* tra = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		start_transaction(tdbb, true, &tra, getHandle(), tpbLength, tpb);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);

	JTransaction* jt = new JTransaction(tra, this);
	tra->setInterface(jt);
	jt->addRef();
	return jt;
}


void JAttachment::transactRequest(IStatus* user_status, ITransaction* tra,
	unsigned int blr_length, const unsigned char* blr,
	unsigned int in_msg_length, const unsigned char* in_msg,
	unsigned int out_msg_length, unsigned char* out_msg)
{
/**************************************
 *
 *	i s c _ t r a n s a c t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Execute a procedure.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		validateHandle(tdbb, getEngineTransaction(user_status, tra));

		check_database(tdbb);

		try
		{
			jrd_tra* const transaction = find_transaction(tdbb);
			Jrd::Attachment* const att = transaction->tra_attachment;

			const MessageNode* inMessage = NULL;
			const MessageNode* outMessage = NULL;

			jrd_req* request = NULL;
			MemoryPool* new_pool = att->createPool();

			try
			{
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				CompilerScratch* csb = PAR_parse(tdbb, reinterpret_cast<const UCHAR*>(blr),
					blr_length, false);

				request = JrdStatement::makeRequest(tdbb, csb, false);
				request->getStatement()->verifyAccess(tdbb);

				for (size_t i = 0; i < csb->csb_rpt.getCount(); i++)
				{

					const MessageNode* node = csb->csb_rpt[i].csb_message;
					if (node)
					{
						if (node->messageNumber == 0)
							inMessage = node;
						else if (node->messageNumber == 1)
							outMessage = node;
					}
				}
			}
			catch (const Exception&)
			{
				if (request)
					CMP_release(tdbb, request);
				else
					att->deletePool(new_pool);

				throw;
			}

			request->req_attachment = tdbb->getAttachment();

			if (in_msg_length)
			{
				const ULONG len = inMessage ? inMessage->format->fmt_length : 0;

				if (in_msg_length != len)
				{
					ERR_post(Arg::Gds(isc_port_len) << Arg::Num(in_msg_length) <<
													   Arg::Num(len));
				}

				memcpy(request->getImpure<UCHAR>(inMessage->impureOffset), in_msg, in_msg_length);
			}

			EXE_start(tdbb, request, transaction);

			const ULONG len = outMessage ? outMessage->format->fmt_length : 0;

			if (out_msg_length != len)
			{
				ERR_post(Arg::Gds(isc_port_len) << Arg::Num(out_msg_length) <<
												   Arg::Num(len));
			}

			if (out_msg_length)
			{
				memcpy(out_msg, request->getImpure<UCHAR>(outMessage->impureOffset),
					out_msg_length);
			}

			check_autocommit(request, tdbb);

			CMP_release(tdbb, request);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::transactRequest");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JTransaction::getInfo(IStatus* user_status,
	unsigned int itemsLength, const unsigned char* items,
	unsigned int bufferLength, unsigned char* buffer)
{
/**************************************
 *
 *	g d s _ $ t r a n s a c t i o n _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on blob object.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			INF_transaction_info(getHandle(), itemsLength, items, bufferLength, buffer);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::getInfo");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JRequest::unwind(IStatus* user_status, int level)
{
/**************************************
 *
 *	g d s _ $ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Unwind a running request.  This is potentially nasty since it can
 *	be called asynchronously.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		jrd_req* request = verify_request_synchronization(getHandle(), level);

		try
		{
			JRD_unwind_request(tdbb, request);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JRequest::unwind");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


SysAttachment::SysAttachment(Attachment* handle)
	: JAttachment(handle)
{
	handle->att_flags |= ATT_system;
}


void SysAttachment::initDone()
{
	Jrd::Attachment* attachment = getHandle();
	Database* dbb = attachment->att_database;
	SyncLockGuard guard(&dbb->dbb_sys_attach, SYNC_EXCLUSIVE, "SysAttachment::initDone");

	attachment->att_next = dbb->dbb_sys_attachments;
	dbb->dbb_sys_attachments = attachment;
}


void SysAttachment::destroy(Attachment* attachment)
{
	{
		Database* dbb = attachment->att_database;
		SyncLockGuard guard(&dbb->dbb_sys_attach, SYNC_EXCLUSIVE, "SysAttachment::destroy");

		for (Jrd::Attachment** ptr = &dbb->dbb_sys_attachments; *ptr; ptr = &(*ptr)->att_next)
		{
			if (*ptr == attachment)
			{
				*ptr = attachment->att_next;
				break;
			}
		}
	}

	// Make Attachment::destroy() happy
	MutexLockGuard async(*getMutex(true), FB_FUNCTION);
	MutexLockGuard sync(*getMutex(), FB_FUNCTION);

	Jrd::Attachment::destroy(attachment);
}


ITransaction* JStatement::execute(IStatus* user_status, ITransaction* apiTra,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer)
{
	JTransaction* jt = apiTra ? getAttachment()->getTransactionInterface(user_status, apiTra) : NULL;
	jrd_tra* tra = jt ? jt->getHandle() : NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		if (tra)
			validateHandle(tdbb, tra);

		check_database(tdbb);

		try
		{
			DSQL_execute(tdbb, &tra, getHandle(), false,
				inMetadata, static_cast<UCHAR*>(inBuffer),
				outMetadata, static_cast<UCHAR*>(outBuffer));

			if (jt && !tra)
			{
				jt->setHandle(NULL);
				jt->release();
				jt = NULL;
			}
			else if (tra && !jt)
			{
				apiTra = NULL;		// Get ready for correct return in OOM case
				jt = new JTransaction(tra, getAttachment());
				tra->setInterface(jt);
				jt->addRef();
			}
			else if (tra && jt)
			{
				jt->setHandle(tra);
				tra->setInterface(jt);
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::execute");
			return apiTra;
		}
		trace_warning(tdbb, user_status, "JStatement::execute");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return apiTra;
	}

	successful_completion(user_status);

	return jt;
}


JResultSet* FB_CARG JStatement::openCursor(IStatus* user_status, ITransaction* transaction,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata)
{
	JTransaction* jt = transaction ? getAttachment()->getTransactionInterface(user_status, transaction) : NULL;
	jrd_tra* tra = jt ? jt->getHandle() : NULL;
	JResultSet* rs = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		if (tra)
			validateHandle(tdbb, tra);

		check_database(tdbb);

		try
		{
			RefPtr<IMessageMetadata> defaultOut;
			if (!outMetadata)
			{
				defaultOut.assignRefNoIncr(metadata.getOutputMetadata());
				if (defaultOut)
				{
					outMetadata = defaultOut;
				}
			}

			DSQL_execute(tdbb, &tra, getHandle(), true,
				inMetadata, static_cast<UCHAR*>(inBuffer), outMetadata, NULL);

			rs = new JResultSet(this);
			rs->addRef();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::openCursor");
			return NULL;
		}
		trace_warning(tdbb, user_status, "JStatement::openCursor");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);
	return rs;
}


IResultSet* JAttachment::openCursor(IStatus* user_status, ITransaction* apiTra,
	unsigned int length, const char* string, unsigned int dialect,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata)
{
	IStatement* tmpStatement = prepare(user_status, apiTra, length, string, dialect,
		(outMetadata ? 0 : IStatement::PREPARE_PREFETCH_OUTPUT_PARAMETERS));
	if (!user_status->isSuccess())
	{
		return NULL;
	}

	IResultSet* rs = tmpStatement->openCursor(user_status, apiTra,
		inMetadata, inBuffer, outMetadata);

	tmpStatement->release();
	return rs;
}


ITransaction* JAttachment::execute(IStatus* user_status, ITransaction* apiTra,
	unsigned int length, const char* string, unsigned int dialect,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer)
{
	JTransaction* jt = apiTra ? getTransactionInterface(user_status, apiTra) : NULL;
	jrd_tra* tra = jt ? jt->getHandle() : NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		if (tra)
			validateHandle(tdbb, tra);

		check_database(tdbb);

		try
		{
			DSQL_execute_immediate(tdbb, getHandle(), &tra, length, string, dialect,
				inMetadata, static_cast<UCHAR*>(inBuffer),
				outMetadata, static_cast<UCHAR*>(outBuffer),
				false);

			if (jt && !tra)
			{
				jt->setHandle(NULL);
				jt->release();
				jt = NULL;
			}
			else if (tra && !jt)
			{
				apiTra = NULL;		// Get ready for correct return in OOM case
				jt = new JTransaction(tra, this);
				jt->addRef();
				tra->setInterface(jt);
			}
			else if (tra && jt)
			{
				jt->setHandle(tra);
				tra->setInterface(jt);
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JAttachment::execute");
			return apiTra;
		}
		trace_warning(tdbb, user_status, "JAttachment::execute");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return apiTra;
	}

	successful_completion(user_status);

	return jt;
}


FB_BOOLEAN JResultSet::fetchNext(IStatus* user_status, void* buffer)
{
	bool hasMessage = false;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		dsql_req* req = getStatement()->getHandle();
		validateHandle(tdbb, req->req_transaction);
		check_database(tdbb);

		try
		{
			hasMessage = req->fetch(tdbb, static_cast<UCHAR*>(buffer));
			if (!hasMessage)
			{
				eof = true;
			}
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::fetch");
			return 0;
		}
		trace_warning(tdbb, user_status, "JResultSet::fetch");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return 0;
	}

	successful_completion(user_status);

	return hasMessage ? FB_TRUE : FB_FALSE;
}

FB_BOOLEAN JResultSet::fetchPrior(IStatus* user_status, void* buffer)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}

FB_BOOLEAN JResultSet::fetchFirst(IStatus* user_status, void* buffer)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}

FB_BOOLEAN JResultSet::fetchLast(IStatus* user_status, void* buffer)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}

FB_BOOLEAN JResultSet::fetchAbsolute(IStatus* user_status, unsigned position, void* buffer)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}

FB_BOOLEAN JResultSet::fetchRelative(IStatus* user_status, int offset, void* buffer)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}

int JResultSet::release()
{
	if (--refCounter != 0)
		return 1;

	if (statement)
	{
		LocalStatus status;
		freeEngineData(&status);
	}
	if (!statement)
	{
		delete this;
	}

	return 0;
}


void JResultSet::freeEngineData(IStatus* user_status)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			DSQL_free_statement(tdbb, getHandle(), DSQL_close);
			statement = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JResultSet::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


FB_BOOLEAN JResultSet::isEof(IStatus* user_status)
{
	return eof ? FB_TRUE : FB_FALSE;
}


FB_BOOLEAN JResultSet::isBof(IStatus* user_status)
{
	try
	{
		status_exception::raise(Arg::Gds(isc_wish_list));
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return FB_FALSE;
	}

	successful_completion(user_status);

	return FB_TRUE;
}


IMessageMetadata* JResultSet::getMetadata(IStatus* user_status)
{
	return statement->getOutputMetadata(user_status);
}


void JResultSet::close(IStatus* user_status)
{
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


void JStatement::freeEngineData(IStatus* user_status)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			DSQL_free_statement(tdbb, getHandle(), DSQL_drop);
			statement = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JStatement::free(IStatus* user_status)
{
	freeEngineData(user_status);
	if (user_status->isSuccess())
	{
		release();
	}
}


JStatement* JAttachment::prepare(IStatus* user_status, ITransaction* apiTra,
	unsigned int stmtLength, const char* sqlStmt,
	unsigned int dialect, unsigned int flags)
{
	JStatement* rc = NULL;

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);

		jrd_tra* tra = apiTra ? getEngineTransaction(user_status, apiTra) : NULL;

		if (tra)
			validateHandle(tdbb, tra);

		check_database(tdbb);
		dsql_req* statement = NULL;

		try
		{
			Array<UCHAR> items, buffer;
			StatementMetadata::buildInfoItems(items, flags);

			statement = DSQL_prepare(tdbb, getHandle(), tra, stmtLength, sqlStmt, dialect,
				&items, &buffer, false);
			rc = new JStatement(statement, this, buffer);
			statement->req_interface = rc;
			rc->addRef();

			trace_warning(tdbb, user_status, "JStatement::prepare");
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::prepare");
			if (statement)
			{
				try
				{
					DSQL_free_statement(tdbb, statement, DSQL_drop);
				}
				catch (const Exception&)
				{ }
			}
			return NULL;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return NULL;
	}

	successful_completion(user_status);
	return rc;
}


unsigned JStatement::getType(IStatus* userStatus)
{
	unsigned ret = 0;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getType();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getType");
			return ret;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);

	return ret;
}


unsigned JStatement::getFlags(IStatus* userStatus)
{
	unsigned ret = 0;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getFlags();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getFlags");
			return ret;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);

	return ret;
}


const char* JStatement::getPlan(IStatus* userStatus, FB_BOOLEAN detailed)
{
	const char* ret = NULL;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getPlan(detailed);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getPlan");
			return ret;
		}
		trace_warning(tdbb, userStatus, "JStatement::getPlan");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);

	return ret;
}

IMessageMetadata* JStatement::getInputMetadata(IStatus* userStatus)
{
	IMessageMetadata* ret = NULL;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getInputMetadata();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getInputMetadata");
			return ret;
		}
		trace_warning(tdbb, userStatus, "JStatement::getInputMetadata");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);

	return ret;
}


IMessageMetadata* JStatement::getOutputMetadata(IStatus* userStatus)
{
	IMessageMetadata* ret = NULL;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getOutputMetadata();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getOutputMetadata");
			return ret;
		}
		trace_warning(tdbb, userStatus, "JStatement::getOutputMetadata");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);

	return ret;
}


ISC_UINT64 JStatement::getAffectedRecords(IStatus* userStatus)
{
	ISC_UINT64 ret = 0;

	try
	{
		EngineContextHolder tdbb(userStatus, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			ret = metadata.getAffectedRecords();
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, userStatus, "JStatement::getAffectedRecords");
			return ret;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(userStatus);
		return ret;
	}

	successful_completion(userStatus);
	return ret;
}


void JResultSet::setCursorName(IStatus* user_status, const char* cursor)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			dsql_req* req = getStatement()->getHandle();
			fb_assert(req);
			req->setCursor(tdbb, cursor);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JResultSet::setCursorName");
			return;
		}
		trace_warning(tdbb, user_status, "JResultSet::setCursorName");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JResultSet::setDelayedOutputFormat(IStatus* user_status, Firebird::IMessageMetadata* outMetadata)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			dsql_req* req = getStatement()->getHandle();
			fb_assert(req);
			req->setDelayedFormat(tdbb, outMetadata);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JResultSet::setDelayedOutputFormat");
			return;
		}
		trace_warning(tdbb, user_status, "JResultSet::setDelayedOutputFormat");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


void JStatement::getInfo(IStatus* user_status,
	unsigned int item_length, const unsigned char* items,
	unsigned int buffer_length, unsigned char* buffer)
{
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			DSQL_sql_info(tdbb, getHandle(), item_length, items, buffer_length, buffer);
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JStatement::getInfo");
			return;
		}
		trace_warning(tdbb, user_status, "JStatement::getInfo");
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}

void JAttachment::ping(IStatus* user_status)
{
/**************************************
 *
 *	G D S _ P I N G
 *
 **************************************
 *
 * Functional description
 *	Check the attachment handle for persistent errors.
 *
 **************************************/

	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb, true);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}

} // namespace Jrd

#ifdef DEBUG_PROCS
void JRD_print_procedure_info(thread_db* tdbb, const char* mesg)
{
/*****************************************************
 *
 *	J R D _ p r i n t _ p r o c e d u r e _ i n f o
 *
 *****************************************************
 *
 * Functional description
 *	print name , use_count of all procedures in
 *      cache
 *
 ******************************************************/
	TEXT fname[MAXPATHLEN];

	Firebird::string fname = fb_utils::getPrefix(Firebird::DirType::FB_DIR_LOG, "proc_info.log");
	FILE* fptr = fopen(fname.c_str(), "a+");
	if (!fptr)
	{
		gds__log("Failed to open %s\n", fname.c_str());
		return;
	}

	if (mesg)
		fputs(mesg, fptr);
	fprintf(fptr, "Prc Name      , prc id , flags  ,  Use Count , Alter Count\n");

	vec<jrd_prc*>* procedures = tdbb->getDatabase()->dbb_procedures;
	if (procedures)
	{
		vec<jrd_prc*>::iterator ptr, end;
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ++ptr)
		{
			const jrd_prc* procedure = *ptr;
			if (procedure)
			{
				fprintf(fptr, "%s  ,  %d,  %X,  %d, %d\n",
					(procedure->getName().toString().hasData() ?
						procedure->getName().toString().c_str() : "NULL"),
					procedure->getId(), procedure->flags, procedure->useCount,
					0); // procedure->prc_alter_count
			}
		}
	}
	else
		fprintf(fptr, "No Cached Procedures\n");

	fclose(fptr);

}
#endif // DEBUG_PROCS


bool JRD_reschedule(thread_db* tdbb, SLONG quantum, bool punt)
{
/**************************************
 *
 *	J R D _ r e s c h e d u l e
 *
 **************************************
 *
 * Functional description
 *	Somebody has kindly offered to relinquish
 *	control so that somebody else may run.
 *
 **************************************/
	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	if (!tdbb->checkCancelState(false))
	{
		Jrd::Attachment::Checkout cout(attachment, FB_FUNCTION);
		THREAD_YIELD();
	}

	try
	{
		tdbb->checkCancelState(true);
	}
	catch (const status_exception& ex)
	{
		tdbb->tdbb_flags |= TDBB_sys_error;

		const Arg::StatusVector status(ex.value());

		if (punt)
		{
			CCH_unwind(tdbb, false);
			ERR_post(status);
		}
		else
		{
			ERR_build_status(tdbb->tdbb_status_vector, status);
			return true;
		}
	}

	DatabaseSnapshot::activate(tdbb);

	tdbb->tdbb_quantum = (tdbb->tdbb_quantum <= 0) ?
		(quantum ? quantum : QUANTUM) : tdbb->tdbb_quantum;

	return false;
}


void jrd_vtof(const char* string, char* field, SSHORT length)
{
/**************************************
 *
 *	j r d _ v t o f
 *
 **************************************
 *
 * Functional description
 *	Move a null terminated string to a fixed length
 *	field.
 *	If the length of the string pointed to by 'field'
 *	is less than 'length', this function pads the
 *	destination string with space upto 'length' bytes.
 *
 *	The call is primarily generated  by the preprocessor.
 *
 *	This is the same code as gds__vtof but is used internally.
 *
 **************************************/

	while (*string)
	{
		*field++ = *string++;
		if (--length <= 0) {
			return;
		}
	}

	if (length) {
		memset(field, ' ', length);
	}
}


static void check_database(thread_db* tdbb, bool async)
{
/**************************************
 *
 *	c h e c k _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Check an attachment for validity.
 *
 **************************************/
	SET_TDBB(tdbb);

	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Test for persistent errors

	if (dbb->dbb_flags & DBB_bugcheck)
	{
		static const char string[] = "can't continue after bugcheck";
		status_exception::raise(Arg::Gds(isc_bug_check) << Arg::Str(string));
	}

	if ((attachment->att_flags & ATT_shutdown) ||
		((dbb->dbb_ast_flags & DBB_shutdown) &&
			((dbb->dbb_ast_flags & DBB_shutdown_full) || !attachment->locksmith())))
	{
		if (dbb->dbb_ast_flags & DBB_shutdown)
		{
			const PathName& filename = attachment->att_filename;
			status_exception::raise(Arg::Gds(isc_shutdown) << Arg::Str(filename));
		}
		else
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}
	}

	// No further checks for the async calls

	if (async)
		return;

	// Test for temporary errors

	if ((attachment->att_flags & ATT_cancel_raise) &&
		!(attachment->att_flags & ATT_cancel_disable))
	{
		attachment->att_flags &= ~ATT_cancel_raise;
		status_exception::raise(Arg::Gds(isc_cancelled));
	}

	DatabaseSnapshot::activate(tdbb);
}


static void commit(thread_db* tdbb, jrd_tra* transaction, const bool retaining_flag)
{
/**************************************
 *
 *	c o m m i t
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction.
 *
 **************************************/

	if (transaction->tra_in_use)
		status_exception::raise(Arg::Gds(isc_transaction_in_use));

	const Jrd::Attachment* const attachment = tdbb->getAttachment();

	if (!(attachment->att_flags & ATT_no_db_triggers) && !(transaction->tra_flags & TRA_prepared))
	{
		// run ON TRANSACTION COMMIT triggers
		run_commit_triggers(tdbb, transaction);
	}

	validateHandle(tdbb, transaction->tra_attachment);
	tdbb->setTransaction(transaction);
	TRA_commit(tdbb, transaction, retaining_flag);
}


static bool drop_files(const jrd_file* file)
{
/**************************************
 *
 *	d r o p _ f i l e s
 *
 **************************************
 *
 * Functional description
 *	drop a linked list of files
 *
 **************************************/
	ISC_STATUS_ARRAY status;

	status[1] = FB_SUCCESS;

	for (; file; file = file->fil_next)
	{
		if (unlink(file->fil_string))
		{
			ERR_build_status(status, Arg::Gds(isc_io_error) << Arg::Str("unlink") <<
							   								   Arg::Str(file->fil_string) <<
									 Arg::Gds(isc_io_delete_err) << SYS_ERR(errno));
			Database* dbb = GET_DBB();
			PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			gds__log_status(pageSpace->file->fil_string, status);
		}
	}

	return status[1] ? true : false;
}


static jrd_tra* find_transaction(thread_db* tdbb)
{
/**************************************
 *
 *	f i n d _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Find the element of a possible multiple database transaction
 *	that corresponds to the current database.
 *
 **************************************/
	SET_TDBB(tdbb);

	const Jrd::Attachment* const attachment = tdbb->getAttachment();
	jrd_tra* const transaction = tdbb->getTransaction();
	fb_assert(transaction->tra_attachment == attachment);

	return transaction;
}


static void find_intl_charset(thread_db* tdbb, Jrd::Attachment* attachment, const DatabaseOptions* options)
{
/**************************************
 *
 *	f i n d _ i n t l _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *	Attachment has declared it's prefered character set
 *	as part of LC_CTYPE, passed over with the attachment
 *	block.  Now let's resolve that to an internal subtype id.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (options->dpb_lc_ctype.isEmpty())
	{
		// No declaration of character set, act like 3.x Interbase
		attachment->att_client_charset = attachment->att_charset = DEFAULT_ATTACHMENT_CHARSET;
		return;
	}

	USHORT id;

	const UCHAR* lc_ctype = reinterpret_cast<const UCHAR*>(options->dpb_lc_ctype.c_str());

	if (MET_get_char_coll_subtype(tdbb, &id, lc_ctype, options->dpb_lc_ctype.length()) &&
		INTL_defined_type(tdbb, id & 0xFF) && ((id & 0xFF) != CS_BINARY))
	{
		attachment->att_client_charset = attachment->att_charset = id & 0xFF;
	}
	else
	{
		// Report an error - we can't do what user has requested
		ERR_post(Arg::Gds(isc_bad_dpb_content) <<
				 Arg::Gds(isc_charset_not_found) << Arg::Str(options->dpb_lc_ctype));
	}
}

namespace
{
	void dpbErrorRaise()
	{
		ERR_post(Arg::Gds(isc_bad_dpb_form) <<
				 Arg::Gds(isc_wrodpbver));
	}
} // anonymous

void DatabaseOptions::get(const UCHAR* dpb, USHORT dpb_length, bool& invalid_client_SQL_dialect)
{
/**************************************
 *
 *	D a t a b a s e O p t i o n s : : g e t
 *
 **************************************
 *
 * Functional description
 *	Parse database parameter block picking up options and things.
 *
 **************************************/
	SSHORT num_old_files = 0;

	dpb_buffers = 0;
	dpb_sweep_interval = -1;
	dpb_overwrite = false;
	dpb_sql_dialect = 99;
	invalid_client_SQL_dialect = false;

	if (dpb_length == 0)
	{
		return;
	}
	if (dpb == NULL)
	{
		ERR_post(Arg::Gds(isc_bad_dpb_form));
	}

	ClumpletReader rdr(ClumpletReader::dpbList, dpb, dpb_length, dpbErrorRaise);
	dumpAuthBlock("DatabaseOptions::get()", &rdr, isc_dpb_auth_block);

	dpb_utf8_filename = rdr.find(isc_dpb_utf8_filename);

	for (rdr.rewind(); !rdr.isEof(); rdr.moveNext())
	{
		switch (rdr.getClumpTag())
		{
		case isc_dpb_working_directory:
			getPath(rdr, dpb_working_directory);
			break;

		case isc_dpb_set_page_buffers:
			dpb_page_buffers = rdr.getInt();
			if (dpb_page_buffers &&
				(dpb_page_buffers < MIN_PAGE_BUFFERS || dpb_page_buffers > MAX_PAGE_BUFFERS))
			{
				ERR_post(Arg::Gds(isc_bad_dpb_content) << Arg::Gds(isc_baddpb_buffers_range));
			}
			dpb_set_page_buffers = true;
			break;

		case isc_dpb_num_buffers:
			if (!Config::getSharedCache())
			{
				dpb_buffers = rdr.getInt();
				const int TEMP_LIMIT = 25;
				if (dpb_buffers < TEMP_LIMIT)
				{
					ERR_post(Arg::Gds(isc_bad_dpb_content) <<
							 Arg::Gds(isc_baddpb_temp_buffers) << Arg::Num(TEMP_LIMIT));
				}
			}
			else
				rdr.getInt();
			break;

		case isc_dpb_page_size:
			dpb_page_size = (USHORT) rdr.getInt();
			break;

		case isc_dpb_debug:
			rdr.getInt();
			break;

		case isc_dpb_sweep:
			dpb_sweep = (USHORT) rdr.getInt();
			break;

		case isc_dpb_sweep_interval:
			dpb_sweep_interval = rdr.getInt();
			break;

		case isc_dpb_verify:
			dpb_verify = (USHORT) rdr.getInt();
			if (dpb_verify & isc_dpb_ignore)
				dpb_flags |= DBB_damaged;
			break;

		case isc_dpb_trace:
			rdr.getInt();
			break;

		case isc_dpb_damaged:
			if (rdr.getInt() & 1)
				dpb_flags |= DBB_damaged;
			break;

		case isc_dpb_enable_journal:
			rdr.getString(dpb_journal);
			break;

		case isc_dpb_wal_backup_dir:
			// ignore, skip
			break;

		case isc_dpb_drop_walfile:
			dpb_wal_action = (USHORT) rdr.getInt();
			break;

		case isc_dpb_old_dump_id:
		case isc_dpb_online_dump:
		case isc_dpb_old_file_size:
		case isc_dpb_old_num_files:
		case isc_dpb_old_start_page:
		case isc_dpb_old_start_seqno:
		case isc_dpb_old_start_file:
			// ignore, skip
			break;

		case isc_dpb_old_file:
			//if (num_old_files >= MAX_OLD_FILES) complain here, for now.
				ERR_post(Arg::Gds(isc_num_old_files));
			// following code is never executed now !
			num_old_files++;
			break;

		case isc_dpb_wal_chkptlen:
		case isc_dpb_wal_numbufs:
		case isc_dpb_wal_bufsize:
		case isc_dpb_wal_grp_cmt_wait:
			// ignore, skip
			break;

		case isc_dpb_dbkey_scope:
			dpb_dbkey_scope = (USHORT) rdr.getInt();
			break;

		case isc_dpb_sql_role_name:
			getString(rdr, dpb_role_name);
			break;

		case isc_dpb_auth_block:
			dpb_auth_block.clear();
			dpb_auth_block.add(rdr.getBytes(), rdr.getClumpLength());
			break;

		case isc_dpb_user_name:
			getString(rdr, dpb_user_name);
			break;

		case isc_dpb_trusted_auth:
			getString(rdr, dpb_trusted_login);
			break;

		case isc_dpb_encrypt_key:
			// Just in case there WAS a customer using this unsupported
			// feature - post an error when they try to access it now
			ERR_post(Arg::Gds(isc_uns_ext) <<
					 Arg::Gds(isc_random) << Arg::Str("Passing encryption key in DPB not supported"));
			break;

		case isc_dpb_no_garbage_collect:
			dpb_no_garbage = true;
			break;

		case isc_dpb_activate_shadow:
			dpb_activate_shadow = true;
			break;

		case isc_dpb_delete_shadow:
			dpb_delete_shadow = true;
			break;

		case isc_dpb_force_write:
			dpb_set_force_write = true;
			dpb_force_write = rdr.getInt() != 0;
			break;

		case isc_dpb_begin_log:
			break;

		case isc_dpb_quit_log:
			break;

		case isc_dpb_no_reserve:
			dpb_set_no_reserve = true;
			dpb_no_reserve = rdr.getInt() != 0;
			break;

		case isc_dpb_interp:
			dpb_interp = (SSHORT) rdr.getInt();
			break;

		case isc_dpb_lc_ctype:
			rdr.getString(dpb_lc_ctype);
			break;

		case isc_dpb_shutdown:
			dpb_shutdown = (USHORT) rdr.getInt();
			// Enforce default
			if ((dpb_shutdown & isc_dpb_shut_mode_mask) == isc_dpb_shut_default)
				dpb_shutdown |= isc_dpb_shut_multi;
			break;

		case isc_dpb_shutdown_delay:
			dpb_shutdown_delay = (SSHORT) rdr.getInt();
			break;

		case isc_dpb_online:
			dpb_online = (USHORT) rdr.getInt();
			// Enforce default
			if ((dpb_online & isc_dpb_shut_mode_mask) == isc_dpb_shut_default)
			{
				dpb_online |= isc_dpb_shut_normal;
			}
			break;

		case isc_dpb_reserved:
			{
				string single;
				rdr.getString(single);
				if (single == "YES")
				{
					dpb_single_user = true;
				}
			}
			break;

		case isc_dpb_overwrite:
			dpb_overwrite = rdr.getInt() != 0;
			break;

		case isc_dpb_nolinger:
			dpb_nolinger = true;
			break;

		case isc_dpb_sec_attach:
			dpb_sec_attach = rdr.getInt() != 0;
			if (dpb_sec_attach)
				dpb_flags |= DBB_security_db;
			break;

		case isc_dpb_gbak_attach:
			{
				string gbakStr;
				rdr.getString(gbakStr);
				dpb_gbak_attach = gbakStr.hasData();
			}
			break;

		case isc_dpb_gstat_attach:
			dpb_gstat_attach = true;
			break;

		case isc_dpb_gfix_attach:
			dpb_gfix_attach = true;
			break;

		case isc_dpb_disable_wal:
			dpb_disable_wal = true;
			break;

		case isc_dpb_connect_timeout:
			dpb_connect_timeout = rdr.getInt();
			break;

		case isc_dpb_dummy_packet_interval:
			dpb_dummy_packet_interval = rdr.getInt();
			break;

		case isc_dpb_sql_dialect:
			dpb_sql_dialect = (USHORT) rdr.getInt();
			if (dpb_sql_dialect > SQL_DIALECT_V6)
				invalid_client_SQL_dialect = true;
			break;

		case isc_dpb_set_db_sql_dialect:
			dpb_set_db_sql_dialect = (USHORT) rdr.getInt();
			break;

		case isc_dpb_set_db_readonly:
			dpb_set_db_readonly = true;
			dpb_db_readonly = rdr.getInt() != 0;
			break;

		case isc_dpb_set_db_charset:
			getString(rdr, dpb_set_db_charset);
			break;

		case isc_dpb_address_path:
			{
				ClumpletReader address_stack(ClumpletReader::UnTagged,
											 rdr.getBytes(), rdr.getClumpLength());
				while (!address_stack.isEof())
				{
					if (address_stack.getClumpTag() != isc_dpb_address)
					{
						address_stack.moveNext();
						continue;
					}
					ClumpletReader address(ClumpletReader::UnTagged,
										   address_stack.getBytes(), address_stack.getClumpLength());
					while (!address.isEof())
					{
						switch (address.getClumpTag())
						{
							case isc_dpb_addr_protocol:
								address.getString(dpb_network_protocol);
								break;
							case isc_dpb_addr_endpoint:
								address.getString(dpb_remote_address);
								break;
							default:
								break;
						}
						address.moveNext();
					}
					break;
				}
			}
			break;

		case isc_dpb_process_id:
			dpb_remote_pid = rdr.getInt();
			break;

		case isc_dpb_process_name:
			getPath(rdr, dpb_remote_process);
			break;

		case isc_dpb_host_name:
			getString(rdr, dpb_remote_host);
			break;

		case isc_dpb_os_user:
			getString(rdr, dpb_remote_os_user);
			break;

		case isc_dpb_client_version:
			getString(rdr, dpb_client_version);
			break;

		case isc_dpb_remote_protocol:
			getString(rdr, dpb_remote_protocol);
			break;

		case isc_dpb_no_db_triggers:
			dpb_no_db_triggers = rdr.getInt() != 0;
			break;

		case isc_dpb_org_filename:
			getPath(rdr, dpb_org_filename);
			break;

		case isc_dpb_ext_call_depth:
			dpb_ext_call_depth = (ULONG) rdr.getInt();
			if (dpb_ext_call_depth >= MAX_CALLBACKS)
				ERR_post(Arg::Gds(isc_exec_sql_max_call_exceeded));
			break;

		case isc_dpb_config:
			getString(rdr, dpb_config);
			break;

		default:
			break;
		}
	}

	if (! rdr.isEof())
		ERR_post(Arg::Gds(isc_bad_dpb_form));
}


static void handle_error(IStatus* user_status, ISC_STATUS code)
{
/**************************************
 *
 *	h a n d l e _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An invalid handle has been passed in.  If there is a user status
 *	vector, make it reflect the error.  If not, emulate the routine
 *	"error" and abort.
 *
 **************************************/
 	if (user_status)
	{
		user_status->set(Arg::Gds(code).value());
	}
}


static JAttachment* init(thread_db* tdbb,
						 const PathName& expanded_name,
						 const PathName& alias_name,
						 RefPtr<Config> config,
						 bool attach_flag,		// only for shared cache
						 const DatabaseOptions& options,
						 RefMutexUnlock& initGuard,
						 IPluginConfig* pConf)
{
/**************************************
 *
 *	i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize for database access.  First call from both CREATE and ATTACH.
 *	Upon entry mutex dbInitMutex must be locked.
 *
 **************************************/
	SET_TDBB(tdbb);
	fb_assert(dbInitMutex->locked());

	// make sure that no new attachments arrive after shutdown started
	if (engineShutdown)
	{
		Arg::Gds(isc_att_shutdown).raise();
	}

	// Initialize standard random generator.
	// MSVC (at least since version 7) have per-thread random seed.
	// As we don't know who uses per-thread seed, this should work for both cases.
	static bool first_rand = true;
	static int first_rand_value = rand();

	if (first_rand || (rand() == first_rand_value))
		srand(time(NULL));

	first_rand = false;

	engineStartup.init();

	// Check to see if the database is already attached
	Database* dbb = NULL;
	JAttachment* jAtt;

	{	// scope
		MutexLockGuard listGuard(databases_mutex, FB_FUNCTION);

		if (config->getSharedCache())
		{
			if (config->getSharedDatabase())
			{
				const char* const errorMsg =
					"SharedDatabase and SharedCache settings cannot be both enabled at once";
				ERR_post(Arg::Gds(isc_wish_list) << Arg::Gds(isc_random) << Arg::Str(errorMsg));
			}

			dbb = databases;
			while (dbb)
			{
				if (!(dbb->dbb_flags & DBB_bugcheck) && dbb->dbb_filename == expanded_name)
				{
					if (attach_flag)
					{
						initGuard.linkWith(dbb->dbb_init_fini);

						{   // scope
							MutexUnlockGuard listUnlock(databases_mutex, FB_FUNCTION);
							fb_assert(!databases_mutex->locked());

							// after unlocking databases_mutex we lose control over dbb
							// as long as dbb_init_fini is not locked and its activity is not checked
							initGuard.enter();
							if (initGuard->doesExist())
							{
								Sync dbbGuard(&dbb->dbb_sync, FB_FUNCTION);
								dbbGuard.lock(SYNC_EXCLUSIVE);

								fb_assert(!(dbb->dbb_flags & DBB_new));

								tdbb->setDatabase(dbb);
								jAtt = create_attachment(alias_name, dbb, options, !attach_flag);

								if (dbb->dbb_linger_timer)
									dbb->dbb_linger_timer->reset();

								tdbb->setAttachment(jAtt->getHandle());

								if (options.dpb_config.hasData())
								{
									ERR_post_warning(Arg::Warning(isc_random) <<
										"Secondary attachment - config data from DPB ignored");
								}

								return jAtt;
							}
						}

						// If we reached this point this means that found dbb was removed
						// Forget about it and repeat search
						initGuard.unlinkFromMutex();
						dbb = databases;
						continue;
					}

					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
				}

				dbb = dbb->dbb_next;
			}
		}

		Config::merge(config, &options.dpb_config);

		dbb = Database::create(pConf);
		dbb->dbb_config = config;
		dbb->dbb_filename = expanded_name;

		// safely take init lock on just created database
		initGuard.linkWith(dbb->dbb_init_fini);
		initGuard.enter();

		dbb->dbb_next = databases;
		databases = dbb;

		dbb->dbb_flags |= (DBB_exclusive | DBB_new | options.dpb_flags);
		if (!attach_flag)
			dbb->dbb_flags |= DBB_creating;
		dbb->dbb_sweep_interval = SWEEP_INTERVAL;

		Sync dbbGuard(&dbb->dbb_sync, FB_FUNCTION);
		dbbGuard.lock(SYNC_EXCLUSIVE);

		tdbb->setDatabase(dbb);
		jAtt = create_attachment(alias_name, dbb, options, !attach_flag);
		tdbb->setAttachment(jAtt->getHandle());
	} // end scope

	// provide context pool for the rest stuff
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);

	dbb->dbb_monitoring_id = fb_utils::genUniqueId();

	// set a garbage collection policy

	if ((dbb->dbb_flags & (DBB_gc_cooperative | DBB_gc_background)) == 0)
	{
		if (!dbb->dbb_config->getSharedCache()) {
			dbb->dbb_flags |= DBB_gc_cooperative;
		}
		else
		{
			string gc_policy = dbb->dbb_config->getGCPolicy();
			gc_policy.lower();
			if (gc_policy == GCPolicyCooperative) {
				dbb->dbb_flags |= DBB_gc_cooperative;
			}
			else if (gc_policy == GCPolicyBackground) {
				dbb->dbb_flags |= DBB_gc_background;
			}
			else if (gc_policy == GCPolicyCombined) {
				dbb->dbb_flags |= DBB_gc_cooperative | DBB_gc_background;
			}
			else // config value is invalid
			{
				// this should not happen - means bug in config
				fb_assert(false);
			}
		}
	}

	return jAtt;
}


static JAttachment* create_attachment(const PathName& alias_name,
									  Database* dbb,
									  const DatabaseOptions& options,
									  bool newDb)
{
/**************************************
 *
 *	c r e a t e _ a t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 *	Create attachment and link it to dbb
 *
 **************************************/
	fb_assert(dbb->locked());

	Jrd::Attachment* attachment = NULL;
	{ // scope
		MutexLockGuard guard(newAttachmentMutex, FB_FUNCTION);
		if (engineShutdown)
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}

		attachment = Jrd::Attachment::create(dbb);
		attachment->att_next = dbb->dbb_attachments;
		dbb->dbb_attachments = attachment;
	}

	attachment->att_filename = alias_name;
	attachment->att_network_protocol = options.dpb_network_protocol;
	attachment->att_remote_address = options.dpb_remote_address;
	attachment->att_remote_pid = options.dpb_remote_pid;
	attachment->att_remote_process = options.dpb_remote_process;
	attachment->att_remote_host = options.dpb_remote_host;
	attachment->att_remote_os_user = options.dpb_remote_os_user;
	attachment->att_client_version = options.dpb_client_version;
	attachment->att_remote_protocol = options.dpb_remote_protocol;
	attachment->att_ext_call_depth = options.dpb_ext_call_depth;

	JAttachment* jAtt = new JAttachment(attachment);
	jAtt->addRef();		// See also REF_NO_INCR RefPtr in unwindAttach()
	attachment->att_interface = jAtt;
	jAtt->manualLock(attachment->att_flags);
	if (newDb)
		attachment->att_flags |= ATT_creator;

	return jAtt;
}


static void init_database_lock(thread_db* tdbb)
{
/**************************************
 *
 *	i n i t _ d a t a b a s e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Initialize the main database lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Main database lock

	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0)
		Lock(tdbb, 0, LCK_database, dbb, CCH_down_grade_dbb);
	dbb->dbb_lock = lock;

	// Try to get an exclusive lock on database.
	// If this fails, insist on at least a shared lock.

	dbb->dbb_flags |= DBB_exclusive;
	if (!LCK_lock(tdbb, lock, LCK_EX, LCK_NO_WAIT))
	{
		// Clean status vector from lock manager error code
		fb_utils::init_status(tdbb->tdbb_status_vector);

		dbb->dbb_flags &= ~DBB_exclusive;

		while (!LCK_lock(tdbb, lock, LCK_SW, -1))
		{
			fb_utils::init_status(tdbb->tdbb_status_vector);

			// If we are in a single-threaded maintenance mode then clean up and stop waiting
			SCHAR spare_memory[MIN_PAGE_SIZE * 2];
			SCHAR* header_page_buffer = (SCHAR*) FB_ALIGN((IPTR) spare_memory, MIN_PAGE_SIZE);
			Ods::header_page* const header_page = reinterpret_cast<Ods::header_page*>(header_page_buffer);

			PIO_header(dbb, header_page_buffer, MIN_PAGE_SIZE);

			if ((header_page->hdr_flags & Ods::hdr_shutdown_mask) == Ods::hdr_shutdown_single)
			{
				ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(attachment->att_filename));
			}
		}
	}
}


static void prepare_tra(thread_db* tdbb, jrd_tra* transaction, USHORT length, const UCHAR* msg)
{
/**************************************
 *
 *	p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	Attempt to prepare a transaction.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (transaction->tra_in_use)
		status_exception::raise(Arg::Gds(isc_transaction_in_use));

	if (!(transaction->tra_flags & TRA_prepared))
	{
		// run ON TRANSACTION COMMIT triggers
		run_commit_triggers(tdbb, transaction);
	}

	validateHandle(tdbb, transaction->tra_attachment);
	tdbb->setTransaction(transaction);
	TRA_prepare(tdbb, transaction, length, msg);
}


static void release_attachment(thread_db* tdbb, Jrd::Attachment* attachment)
{
/**************************************
 *
 *	r e l e a s e _ a t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 *	Disconnect attachment block from database block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);
	fb_assert(!dbb->locked());

	if (!attachment)
		return;

	dbb->dbb_extManager.closeAttachment(tdbb, attachment);

	if (!dbb->dbb_config->getSharedDatabase() && attachment->att_relations)
	{
		vec<jrd_rel*>& rels = *attachment->att_relations;
		for (size_t i = 1; i < rels.count(); i++)
		{
			jrd_rel* relation = rels[i];
			if (relation && (relation->rel_flags & REL_temp_conn) &&
				!(relation->rel_flags & (REL_deleted | REL_deleting)) )
			{
				relation->delPages(tdbb);
			}
		}
	}

	if (dbb->dbb_event_mgr && attachment->att_event_session)
	{
		dbb->dbb_event_mgr->deleteSession(attachment->att_event_session);
	}

    // CMP_release() changes att_requests.
	while (!attachment->att_requests.isEmpty())
		CMP_release(tdbb, attachment->att_requests.back());

	MET_clear_cache(tdbb);

	attachment->releaseLocks(tdbb);

	// Shut down any extern relations

	if (attachment->att_relations)
	{
		vec<jrd_rel*>* vector = attachment->att_relations;

		for (vec<jrd_rel*>::iterator ptr = vector->begin(), end = vector->end(); ptr < end; ++ptr)
		{
			jrd_rel* relation = *ptr;

			if (relation)
			{
				if (relation->rel_file)
					EXT_fini(relation, false);

				delete relation;
			}
		}
	}

	// Release any validation error vector allocated

	delete attachment->att_val_errors;
	attachment->att_val_errors = NULL;

	attachment->destroyIntlObjects(tdbb);

	attachment->detachLocksFromAttachment();

	LCK_fini(tdbb, LCK_OWNER_attachment);

	delete attachment->att_compatibility_table;

	if (attachment->att_dsql_instance)
	{
		MemoryPool* const pool = &attachment->att_dsql_instance->dbb_pool;
		delete attachment->att_dsql_instance;
		attachment->deletePool(pool);
	}

	// remove the attachment block from the dbb linked list
	Sync sync(&dbb->dbb_sync, "jrd.cpp: release_attachment");
	sync.lock(SYNC_EXCLUSIVE);

	for (Jrd::Attachment** ptr = &dbb->dbb_attachments; *ptr; ptr = &(*ptr)->att_next)
	{
		if (*ptr == attachment)
		{
			*ptr = attachment->att_next;
			break;
		}
	}

	SCL_release_all(attachment->att_security_classes);

	delete attachment->att_user;

	tdbb->setAttachment(NULL);
	Jrd::Attachment::destroy(attachment);	// string were re-saved in the beginning of this function,
											// keep that in sync please
}


static void rollback(thread_db* tdbb, jrd_tra* transaction, const bool retaining_flag)
{
/**************************************
 *
 *	r o l l b a c k
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction.
 *
 **************************************/
	if (transaction->tra_in_use)
		status_exception::raise(Arg::Gds(isc_transaction_in_use));

	ISC_STATUS_ARRAY user_status = {0};
	ISC_STATUS_ARRAY local_status = {0};
	ISC_STATUS* const orig_status = tdbb->tdbb_status_vector;

	try
	{
		try
		{
			const Database* const dbb = tdbb->getDatabase();
			const Jrd::Attachment* const attachment = tdbb->getAttachment();

			if (!(attachment->att_flags & ATT_no_db_triggers))
			{
				try
				{
					ISC_STATUS_ARRAY temp_status = {0};
					tdbb->tdbb_status_vector = temp_status;

					// run ON TRANSACTION ROLLBACK triggers
					EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_trans_rollback);
				}
				catch (const Exception&)
				{
					if (dbb->dbb_flags & DBB_bugcheck)
						throw;
				}
			}

			tdbb->tdbb_status_vector = user_status;
			tdbb->setTransaction(transaction);
			TRA_rollback(tdbb, transaction, retaining_flag, false);
		}
		catch (const Exception& ex)
		{
			ex.stuff_exception(user_status);
			tdbb->tdbb_status_vector = local_status;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuff_exception(user_status);
	}

	tdbb->tdbb_status_vector = orig_status;

	if (user_status[1] != FB_SUCCESS)
		status_exception::raise(user_status);
}


static void setEngineReleaseDelay(Database* dbb)
{
	unsigned maxLinger = 0;

	{ // scope
		MutexLockGuard listGuardForLinger(databases_mutex, FB_FUNCTION);

		for (Database* d = databases; d; d = d->dbb_next)
		{
			if (!d->dbb_attachments && (d->dbb_linger_end > maxLinger))
				maxLinger = d->dbb_linger_end;
		}
	}

	++maxLinger;	// avoid rounding errors
	time_t t = time(NULL);
	dbb->dbb_plugin_config->setReleaseDelay(maxLinger > t ? (maxLinger - t) * 1000 * 1000 : 0);
}


bool JRD_shutdown_database(Database* dbb, const unsigned flags)
{
/*************************************************
 *
 *	J R D _ s h u t d o w n _ d a t a b a s e
 *
 *************************************************
 *
 * Functional description
 *	Shutdown physical database environment.
 *
 **************************************/
	ThreadContextHolder tdbb(dbb, NULL);

	RefMutexUnlock finiGuard;

	{ // scope
		MutexLockGuard listGuard1(databases_mutex, FB_FUNCTION);

		Database** d_ptr;
		for (d_ptr = &databases; *d_ptr; d_ptr = &(*d_ptr)->dbb_next)
		{
			if (*d_ptr == dbb)
			{
				finiGuard.linkWith(dbb->dbb_init_fini);

				{	// scope
					MutexUnlockGuard listUnlock(databases_mutex, FB_FUNCTION);

					// after unlocking databases_mutex we lose control over dbb
					// as long as dbb_init_fini is not locked and its activity is not checked
					finiGuard.enter();
					if (finiGuard->doesExist())
						break;

					// database to shutdown does not exist
					// looks like somebody else took care to destroy it
					return false;
				}
			}
		}

		// Check - may be database already missing in linked list
		if (!finiGuard)
			return false;
	}

	if (dbb->dbb_attachments)
		return false;

	// Database linger
	if ((flags & SHUT_DBB_LINGER) &&
		(!(engineShutdown || (dbb->dbb_ast_flags & DBB_shutdown))) &&
		(dbb->dbb_linger_seconds > 0) &&
		(MasterInterfacePtr()->serverMode(-1) == 1) &&	// multiuser server
		dbb->dbb_config->getSharedCache())
	{
		if (!dbb->dbb_linger_timer)
			dbb->dbb_linger_timer = new Database::Linger(dbb);

		dbb->dbb_linger_end = time(NULL) + dbb->dbb_linger_seconds;
		dbb->dbb_linger_timer->set(dbb->dbb_linger_seconds);

		setEngineReleaseDelay(dbb);

		return false;
	}

	// Reset provider unload delay if needed
	dbb->dbb_linger_end = 0;
	setEngineReleaseDelay(dbb);

	// Deactivate dbb_init_fini lock
	// Since that moment dbb becomes not reusable
	dbb->dbb_init_fini->destroy();

	fb_assert(!dbb->locked());

	// Disable AST delivery as we're about to release all locks

	{ // scope
		WriteLockGuard astGuard(dbb->dbb_ast_lock, FB_FUNCTION);
		dbb->dbb_flags |= DBB_no_ast;
	}

	// Shutdown file and/or remote connection

#ifdef SUPERSERVER_V2
	TRA_header_write(tdbb, dbb, 0);	// Update transaction info on header page.
#endif

	DatabaseSnapshot::shutdown(tdbb);

	VIO_fini(tdbb);

	if (dbb->dbb_crypto_manager)
		dbb->dbb_crypto_manager->terminateCryptThread(tdbb);

	CCH_fini(tdbb);

	if (dbb->dbb_backup_manager)
		dbb->dbb_backup_manager->shutdown(tdbb);

	if (dbb->dbb_crypto_manager)
		dbb->dbb_crypto_manager->shutdown(tdbb);

	if (dbb->dbb_shadow_lock)
		LCK_release(tdbb, dbb->dbb_shadow_lock);

	if (dbb->dbb_retaining_lock)
		LCK_release(tdbb, dbb->dbb_retaining_lock);

	dbb->dbb_shared_counter.shutdown(tdbb);

	if (dbb->dbb_sweep_lock)
		LCK_release(tdbb, dbb->dbb_sweep_lock);

	if (dbb->dbb_lock)
		LCK_release(tdbb, dbb->dbb_lock);

	LCK_fini(tdbb, LCK_OWNER_database);

	{ // scope
		MutexLockGuard listGuard2(databases_mutex, FB_FUNCTION);

		Database** d_ptr;
		for (d_ptr = &databases; *d_ptr; d_ptr = &(*d_ptr)->dbb_next)
		{
			if (*d_ptr == dbb)
			{
				fb_assert(!dbb->dbb_attachments);

				*d_ptr = dbb->dbb_next;
				dbb->dbb_next = NULL;
				break;
			}
		}
	}

	if (flags & SHUT_DBB_RELEASE_POOLS)
	{
		tdbb->setDatabase(NULL);
		Database::destroy(dbb);
	}

	return true;
}


static void strip_quotes(string& out)
{
/**************************************
 *
 *	s t r i p _ q u o t e s
 *
 **************************************
 *
 * Functional description
 *	Get rid of quotes around strings
 *	Quotes in the middle will confuse this routine!
 *
 **************************************/
	if (out.isEmpty())
	{
		return;
	}

	if (out[0] == DBL_QUOTE || out[0] == SINGLE_QUOTE)
	{
		// Skip any initial quote
		const char quote = out[0];
		out.erase(0, 1);
		// Search for same quote
		size_t pos = out.find(quote);
		if (pos != string::npos)
		{
			out.erase(pos);
		}
	}
}


void JRD_enum_attachments(PathNameList* dbList, ULONG& atts, ULONG& dbs, ULONG& svcs)
{
/**************************************
 *
 *	J R D _ e n u m _ a t t a c h m e n t s
 *
 **************************************
 *
 * Functional description
 *	Count the number of active databases and
 *	attachments.
 *
 **************************************/
	atts = dbs = svcs = 0;

	try
	{
		PathNameList dbFiles(*getDefaultMemoryPool());

		MutexLockGuard guard(databases_mutex, FB_FUNCTION);

		// Zip through the list of databases and count the number of local
		// connections.  If buf is not NULL then copy all the database names
		// that will fit into it.

		for (Database* dbb = databases; dbb; dbb = dbb->dbb_next)
		{
			SyncLockGuard dbbGuard(&dbb->dbb_sync, SYNC_SHARED, "JRD_enum_attachments");

			if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_security_db)))
			{
				if (!dbFiles.exist(dbb->dbb_filename))
					dbFiles.add(dbb->dbb_filename);

				for (const Jrd::Attachment* attach = dbb->dbb_attachments; attach;
					 attach = attach->att_next)
				{
					atts++;
				}
			}
		}

		dbs = (ULONG) dbFiles.getCount();
		svcs = Service::totalCount();

		if (dbList)
		{
			*dbList = dbFiles;
		}
	}
	catch (const Exception&)
	{
		// Here we ignore possible errors from databases_mutex.
		// They were always silently ignored, and for this function
		// we really have no way to notify world about mutex problem.
		//		AP. 2008.
	}
}


void JTransaction::freeEngineData(IStatus* user_status)
{
/**************************************
 *
 *	f r e e E n g i n e D a t a
 *
 **************************************
 *
 * Functional description
 *	Release or rollback transaction depending upon prepared it or not.
 *
 **************************************/
	try
	{
		EngineContextHolder tdbb(user_status, this, FB_FUNCTION);
		check_database(tdbb);

		try
		{
			if (transaction->tra_flags & TRA_prepared)
			{
				TraceTransactionEnd trace(transaction, false, false);
				EDS::Transaction::jrdTransactionEnd(tdbb, transaction, false, false, false);
				TRA_release_transaction(tdbb, transaction, &trace);
			}
			else
				TRA_rollback(tdbb, transaction, false, true);

			transaction = NULL;
		}
		catch (const Exception& ex)
		{
			transliterateException(tdbb, ex, user_status, "JTransaction::freeEngineData");
			return;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(user_status);
		return;
	}

	successful_completion(user_status);
}


static void purge_transactions(thread_db* tdbb, Jrd::Attachment* attachment, const bool force_flag)
{
/**************************************
 *
 *	p u r g e _ t r a n s a c t i o n s
 *
 **************************************
 *
 * Functional description
 *	commit or rollback all transactions
 *	from an attachment
 *
 **************************************/
	Database* const dbb = attachment->att_database;
	jrd_tra* const trans_dbk = attachment->att_dbkey_trans;

	unsigned int count = 0;
	jrd_tra* next;

	for (jrd_tra* transaction = attachment->att_transactions; transaction; transaction = next)
	{
		next = transaction->tra_next;
		if (transaction != trans_dbk)
		{
			if (transaction->tra_flags & TRA_prepared)
			{
				TraceTransactionEnd trace(transaction, false, false); // need ability to indicate prepared (in limbo) transaction
				EDS::Transaction::jrdTransactionEnd(tdbb, transaction, false, false, true);
				TRA_release_transaction(tdbb, transaction, &trace);
			}
			else if (force_flag)
				TRA_rollback(tdbb, transaction, false, true);
			else
				++count;
		}
	}

	if (count)
	{
		ERR_post(Arg::Gds(isc_open_trans) << Arg::Num(count));
	}

	// If there's a side transaction for db-key scope, get rid of it
	if (trans_dbk)
	{
		attachment->att_dbkey_trans = NULL;
		TRA_commit(tdbb, trans_dbk, false);
	}
}


static void purge_attachment(thread_db* tdbb, JAttachment* jAtt, unsigned flags)
{
/**************************************
 *
 *	p u r g e _ a t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 *	Zap an attachment, shutting down the database
 *	if it is the last one.
 *
 **************************************/
	SET_TDBB(tdbb);

	Mutex* const attMutex = jAtt->getMutex();
	fb_assert(attMutex->locked());

	Jrd::Attachment* attachment = jAtt->getHandle();

	while (attachment && (attachment->att_flags & ATT_purge_started))
	{
		attachment->att_use_count--;

		{ // scope
			MutexUnlockGuard cout(*attMutex, FB_FUNCTION);
			// !!!!!!!!!!!!!!!!! - event? semaphore? condvar? (when ATT_purge_started / jAtt->getHandle() changes)

			fb_assert(!attMutex->locked());
			THD_yield();
			THD_sleep(1);
		}

		attachment = jAtt->getHandle();

		if (attachment)
	  		attachment->att_use_count++;
	}

	if (!attachment)
		return;

	fb_assert(attachment->att_flags & ATT_shutdown);
	attachment->att_flags |= ATT_purge_started;

	fb_assert(attachment->att_use_count > 0);
	attachment = jAtt->getHandle();
	while (attachment && attachment->att_use_count > 1)
	{
		attachment->att_use_count--;

		{ // scope
			MutexUnlockGuard cout(*attMutex, FB_FUNCTION);
			// !!!!!!!!!!!!!!!!! - event? semaphore? condvar? (when --att_use_count)

			fb_assert(!attMutex->locked());
			THD_yield();
			THD_sleep(1);
		}

		attachment = jAtt->getHandle();

		if (attachment)
	  		attachment->att_use_count++;
	}

	fb_assert(attMutex->locked());

	if (!attachment)
		return;

	Database* const dbb = attachment->att_database;
	const bool forcedPurge = (flags & PURGE_FORCE);

	tdbb->tdbb_flags |= TDBB_detaching;

	if (!(dbb->dbb_flags & DBB_bugcheck))
	{
		try
		{
			const trig_vec* const trig_disconnect =
				attachment->att_triggers[DB_TRIGGER_DISCONNECT];

			if (!forcedPurge &&
				!(attachment->att_flags & ATT_no_db_triggers) &&
				trig_disconnect && !trig_disconnect->isEmpty())
			{
				ThreadStatusGuard temp_status(tdbb);

				jrd_tra* transaction = NULL;
				const ULONG save_flags = attachment->att_flags;

				try
				{
					// Start a transaction to execute ON DISCONNECT triggers.
					// Ensure this transaction can't trigger auto-sweep.
					attachment->att_flags |= ATT_no_cleanup;
					transaction = TRA_start(tdbb, 0, NULL);
					attachment->att_flags = save_flags;

					// run ON DISCONNECT triggers
					EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_disconnect);

					// and commit the transaction
					TRA_commit(tdbb, transaction, false);
				}
				catch (const Exception&)
				{
					attachment->att_flags = save_flags;

					if (dbb->dbb_flags & DBB_bugcheck)
						throw;

					try
					{
						if (transaction)
							TRA_rollback(tdbb, transaction, false, false);
					}
					catch (const Exception&)
					{
						if (dbb->dbb_flags & DBB_bugcheck)
							throw;
					}
				}
			}
		}
		catch (const Exception&)
		{
			if (!forcedPurge)
			{
				attachment->att_flags &= ~ATT_purge_started;
				throw;
			}
		}
	}

	try
	{
		// allow to free resources used by dynamic statements
		EDS::Manager::jrdAttachmentEnd(tdbb, attachment);

		if (!(dbb->dbb_flags & DBB_bugcheck))
		{
			// Check for any pending transactions
			purge_transactions(tdbb, attachment, forcedPurge);
		}
	}
	catch (const Exception&)
	{
		if (!forcedPurge)
		{
			attachment->att_flags &= ~ATT_purge_started;
			throw;
		}
	}

	// Notify Trace API manager about disconnect
	if (attachment->att_trace_manager->needs(TRACE_EVENT_DETACH))
	{
		TraceConnectionImpl conn(attachment);
		attachment->att_trace_manager->event_detach(&conn, false);
	}

	fb_assert(attMutex->locked());
	Mutex* asyncMutex = jAtt->getMutex(true, true);
	MutexEnsureUnlock asyncGuard(*asyncMutex, FB_FUNCTION);

	{ // scope - ensure correct order of taking both async and main mutexes
		MutexUnlockGuard cout(*attMutex, FB_FUNCTION);
		fb_assert(!attMutex->locked());
		asyncGuard.enter();
	}

	if (!jAtt->getHandle())
		return;

	// Unlink attachment from database
	release_attachment(tdbb, attachment);

	asyncGuard.leave();
	MutexUnlockGuard cout(*attMutex, FB_FUNCTION);

	// Try to close database if there are no attachments
	JRD_shutdown_database(dbb, SHUT_DBB_RELEASE_POOLS | (flags & PURGE_LINGER ? SHUT_DBB_LINGER : 0));
}


static void run_commit_triggers(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	r u n _ c o m m i t _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Run ON TRANSACTION COMMIT triggers of a transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	if (transaction == attachment->getSysTransaction())
		return;

	// start a savepoint to rollback changes of all triggers
	VIO_start_save_point(tdbb, transaction);

	try
	{
		// run ON TRANSACTION COMMIT triggers
		EXE_execute_db_triggers(tdbb, transaction,
								jrd_req::req_trigger_trans_commit);
		VIO_verb_cleanup(tdbb, transaction);
	}
	catch (const Exception&)
	{
		if (!(tdbb->getDatabase()->dbb_flags & DBB_bugcheck))
		{
			// rollbacks the created savepoint
			++transaction->tra_save_point->sav_verb_count;
			VIO_verb_cleanup(tdbb, transaction);
		}
		throw;
	}
}


// verify_request_synchronization
//
// @brief Finds the sub-requests at the given level and replaces it with the
// original passed request (note the pointer by reference). If that specific
// sub-request is not found, throw the dreaded "request synchronization error".
// Notice that at this time, the calling function's "request" pointer has been
// set to null, so remember that if you write a debugging routine.
// This function replaced a chunk of code repeated four times.
//
// @param request The incoming, parent request to be replaced.
// @param level The level of the sub-request we need to find.
static jrd_req* verify_request_synchronization(JrdStatement* statement, USHORT level)
{
	if (level)
	{
		if (level >= statement->requests.getCount() || !statement->requests[level])
			ERR_post(Arg::Gds(isc_req_sync));
	}

	return statement->requests[level];
}


/**

 	verifyDatabaseName

    @brief	Verify database name for open/create
	against given in conf file list of available directories
	and security database name

    @param name
    @param status

 **/
static VdnResult verifyDatabaseName(const PathName& name, ISC_STATUS* status, bool is_alias)
{
	// Check for securityX.fdb
	static GlobalPtr<PathName> securityNameBuffer, expandedSecurityNameBuffer;
	static GlobalPtr<Mutex> mutex;

	MutexLockGuard guard(mutex, FB_FUNCTION);

	if (!securityNameBuffer->hasData())
	{
		const RefPtr<Config> defConf(Config::getDefaultConfig());
		securityNameBuffer->assign(defConf->getSecurityDatabase());
		expandedSecurityNameBuffer->assign(securityNameBuffer);
		ISC_expand_filename(expandedSecurityNameBuffer, false);
	}

	if (name == securityNameBuffer || name == expandedSecurityNameBuffer)
		return VDN_OK;

	// Check for .conf
	if (!JRD_verify_database_access(name))
	{
		if (!is_alias) {
			ERR_build_status(status, Arg::Gds(isc_conf_access_denied) << Arg::Str("database") <<
																		 Arg::Str(name));
		}
		return VDN_FAIL;
	}
	return VDN_OK;
}


/**

	getUserInfo

    @brief	Almost stub-like now.
    Planned to take into an account mapping of users and groups.
	Fills UserId structure with resulting values.

    @param user
    @param options
    @param

 **/
static void getUserInfo(UserId& user, const DatabaseOptions& options,
	const char* aliasName, const char* dbName, const RefPtr<Config>* config)
{
	bool wheel = false;
	int id = -1, group = -1;	// CVC: This var contained trash
	string name, trusted_role, auth_method;

	if (fb_utils::bootBuild())
	{
		wheel = true;
	}
	else
	{
		auth_method = "User name in DPB";
		if (options.dpb_trusted_login.hasData())
		{
			name = options.dpb_trusted_login;
		}
		else if (options.dpb_user_name.hasData())
		{
			name = options.dpb_user_name;
		}
		else if (options.dpb_auth_block.hasData())
		{
			mapUser(name, trusted_role, &auth_method, &user.usr_auth_block, options.dpb_auth_block,
				aliasName, dbName, (config ? (*config)->getSecurityDatabase() : NULL));
			ISC_systemToUtf8(name);
			ISC_systemToUtf8(trusted_role);
		}
		else
		{
			auth_method = "OS user name";
			wheel = ISC_get_user(&name, &id, &group);
			ISC_systemToUtf8(name);
			if (id == 0)
			{
				auth_method = "OS user name / wheel";
				wheel = true;
			}
		}

		ISC_utf8ToSystem(name);
		name.upper();
		ISC_systemToUtf8(name);

		// if the name from the user database is defined as SYSDBA,
		// we define that user id as having system privileges

		if (name == SYSDBA_USER_NAME)
		{
			wheel = true;
		}
	}

	// In case we became WHEEL on an OS that didn't require name SYSDBA,
	// (Like Unix) force the effective Database User name to be SYSDBA

	if (wheel)
	{
		name = SYSDBA_USER_NAME;
	}

	if (name.length() > USERNAME_LENGTH)
	{
		status_exception::raise(Arg::Gds(isc_long_login) << Arg::Num(name.length())
														 << Arg::Num(USERNAME_LENGTH));
	}

	user.usr_user_name = name;
	user.usr_project_name = "";
	user.usr_org_name = "";
	user.usr_auth_method = auth_method;
	user.usr_user_id = id;
	user.usr_group_id = group;

	if (wheel)
	{
		user.usr_flags |= USR_locksmith;
	}

	if (options.dpb_role_name.hasData())
	{
		user.usr_sql_role_name = options.dpb_role_name;
	}

	if (trusted_role.hasData())
	{
		user.usr_trusted_role = trusted_role;
	}
}

static void unwindAttach(thread_db* tdbb, const Exception& ex, IStatus* userStatus,
	Jrd::Attachment* attachment, Database* dbb)
{
	transliterateException(tdbb, ex, userStatus, NULL);

	try
	{
		if (dbb)
		{
			fb_assert(!dbb->locked());
			ThreadStatusGuard temp_status(tdbb);

			if (attachment)
			{
				// A number of holders to make Attachment::destroy() happy
				// See also addRef() in create_attachment()
				RefPtr<JAttachment> jAtt(REF_NO_INCR, attachment->att_interface);

				// This unlocking/locking order guarantees stable release of attachment
				jAtt->manualUnlock(attachment->att_flags);

				ULONG flags = 0;	// att_flags may already not exist here!
				jAtt->manualLock(flags);
				if (jAtt->getHandle())
				{
					attachment->att_flags |= flags;
					release_attachment(tdbb, attachment);
				}
				else
				{
					jAtt->manualUnlock(flags);
				}
			}

			JRD_shutdown_database(dbb, SHUT_DBB_RELEASE_POOLS);
		}
	}
	catch (const Exception&)
	{
		// no-op
	}

	return;
}


namespace
{
	bool shutdownAttachments(AttachmentsRefHolder* arg)
	{
		AutoPtr<AttachmentsRefHolder> queue(arg);
		AttachmentsRefHolder& attachments = *arg;
		bool success = true;

		// Set terminate flag for all attachments
		for (AttachmentsRefHolder::Iterator iter(attachments); *iter; ++iter)
		{
			JAttachment* const jAtt = *iter;

			MutexLockGuard guard(*(jAtt->getMutex(true)), FB_FUNCTION);
			Attachment* attachment = jAtt->getHandle();

			if (attachment)
				attachment->signalShutdown();
		}

		// Purge all attachments
		for (AttachmentsRefHolder::Iterator iter(attachments); *iter; ++iter)
		{
			JAttachment* const jAtt = *iter;

			MutexLockGuard guard(*(jAtt->getMutex()), FB_FUNCTION);
			Attachment* attachment = jAtt->getHandle();

			if (attachment)
			{
				ThreadContextHolder tdbb;
				tdbb->setAttachment(attachment);
				tdbb->setDatabase(attachment->att_database);

				try
				{
					// purge attachment, rollback any open transactions
					attachment->att_use_count++;
					purge_attachment(tdbb, jAtt, PURGE_FORCE);
				}
				catch (const Exception& ex)
				{
					iscLogException("error while shutting down attachment", ex);
					success = false;
				}

				attachment = jAtt->getHandle();

				if (attachment)
					attachment->att_use_count--;
			}
		}

		return success;
	}

	THREAD_ENTRY_DECLARE attachmentShutdownThread(THREAD_ENTRY_PARAM arg)
	{
		try
		{
			MutexLockGuard guard(shutdownMutex, FB_FUNCTION);
			if (engineShutdown)
			{
				// Shutdown was done, all attachmnets are gone
				return 0;
			}

			shutdownAttachments(static_cast<AttachmentsRefHolder*>(arg));
		}
		catch(const Exception& ex)
		{
			ISC_STATUS_ARRAY st;
			ex.stuff_exception(st);
			iscLogException("attachmentShutdownThread", ex);
		}

		return 0;
	}
} // anonymous namespace


static THREAD_ENTRY_DECLARE shutdown_thread(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *	s h u t d o w n _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Shutdown the engine.
 *
 **************************************/
	Semaphore* const semaphore = static_cast<Semaphore*>(arg);

	bool success = true;
	MemoryPool& pool = *getDefaultMemoryPool();
	AttachmentsRefHolder* const attachments = FB_NEW(pool) AttachmentsRefHolder(pool);

	try
	{
		// Shutdown external datasets manager
		EDS::Manager::shutdown();

		{ // scope
			MutexLockGuard guard(databases_mutex, FB_FUNCTION);

			for (Database* dbb = databases; dbb; dbb = dbb->dbb_next)
			{
				if ( !(dbb->dbb_flags & (DBB_bugcheck | DBB_security_db)) )
				{
					Sync dbbGuard(&dbb->dbb_sync, FB_FUNCTION);
					dbbGuard.lock(SYNC_EXCLUSIVE);

					for (const Attachment* att = dbb->dbb_attachments; att; att = att->att_next)
						attachments->add(att->att_interface);
				}
			}
			// No need in databases_mutex any more
		}

		// Shutdown existing attachments
		success = success && shutdownAttachments(attachments);

		{ // scope
			MutexLockGuard guard(databases_mutex, FB_FUNCTION);

			for (Database** ptrDbb = &databases; *ptrDbb;)
			{
				Database* dbb = *ptrDbb;
				JRD_shutdown_database(dbb, SHUT_DBB_RELEASE_POOLS);

				if (dbb == *ptrDbb)
					ptrDbb = &(dbb->dbb_next);
			}

			// No need in databases_mutex any more
		}

		// Extra shutdown operations
		Service::shutdownServices();
	}
	catch (const Exception&)
	{
		success = false;
	}

	if (success && semaphore)
	{
		semaphore->release();
	}

	return 0;
}


// begin thread_db methods

void thread_db::setDatabase(Database* val)
{
	if (database != val)
	{
		const bool wasActive = database && (priorThread || nextThread || database->dbb_active_threads == this);

		if (wasActive)
		{
			deactivate();
		}

		database = val;
		dbbStat = val ? &val->dbb_stats : RuntimeStatistics::getDummy();

		if (wasActive)
		{
			activate();
		}
	}
}

// need the Jrd:: qualifier to not clash with Attachment in FirebirdApi.h
void thread_db::setAttachment(Jrd::Attachment* val)
{
	attachment = val;
	attStat = val ? &val->att_stats : RuntimeStatistics::getDummy();
}

void thread_db::setTransaction(jrd_tra* val)
{
	transaction = val;
	traStat = val ? &val->tra_stats : RuntimeStatistics::getDummy();
}

void thread_db::setRequest(jrd_req* val)
{
	request = val;
	reqStat = val ? &val->req_stats : RuntimeStatistics::getDummy();
}

SSHORT thread_db::getCharSet() const
{
	if (request && request->charSetId != CS_dynamic)
		return request->charSetId;

	return attachment->att_charset;
}

bool thread_db::checkCancelState(bool punt)
{
	// Test various flags and unwind/throw if required.
	// But do that only if we're neither in the verb cleanup state
	// nor currently detaching, as these actions should never be interrupted.
	// Also don't break wait in LM if it is not safe.

	if (tdbb_flags & (TDBB_verb_cleanup | TDBB_detaching | TDBB_wait_cancel_disable))
		return false;

	if (attachment)
	{
		if (attachment->att_flags & ATT_shutdown)
		{
			if (database->dbb_ast_flags & DBB_shutdown)
			{
				if (!punt)
					return true;

				status_exception::raise(Arg::Gds(isc_shutdown) <<
										Arg::Str(attachment->att_filename));
			}
			else if (!(tdbb_flags & TDBB_shutdown_manager))
			{
				if (!punt)
					return true;

				status_exception::raise(Arg::Gds(isc_att_shutdown));
			}
		}

		// If a cancel has been raised, defer its acknowledgement
		// when executing in the context of an internal request or
		// the system transaction.

		if ((attachment->att_flags & ATT_cancel_raise) &&
			!(attachment->att_flags & ATT_cancel_disable))
		{
			if ((!request ||
					!(request->getStatement()->flags &
						// temporary change to fix shutdown
						(/*JrdStatement::FLAG_INTERNAL | */JrdStatement::FLAG_SYS_TRIGGER))) &&
				(!transaction || !(transaction->tra_flags & TRA_system)))
			{
				if (!punt)
					return true;

				attachment->att_flags &= ~ATT_cancel_raise;
				status_exception::raise(Arg::Gds(isc_cancelled));
			}
		}
	}

	// Check the thread state for already posted system errors. If any still persists,
	// then someone tries to ignore our attempts to interrupt him. Let's insist.

	if (tdbb_flags & TDBB_sys_error)
	{
		if (!punt)
			return true;

		status_exception::raise(Arg::Gds(isc_cancelled));
	}

	return false;
}

// end thread_db methods


void JRD_autocommit_ddl(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ a u t o c o m m i t _ d d l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	// Perform an auto commit for autocommit transactions.
	// This is slightly tricky. If the commit retain works,
	// all is well. If TRA_commit() fails, we perform
	// a rollback_retain(). This will backout the
	// effects of the transaction, mark it dead and
	// start a new transaction.

	// Ignore autocommit for requests created by EXECUTE STATEMENT

	if (transaction->tra_callback_count != 0)
		return;

	if (transaction->tra_flags & TRA_perform_autocommit)
	{
		transaction->tra_flags &= ~TRA_perform_autocommit;

		try
		{
			TRA_commit(tdbb, transaction, true);
		}
		catch (const Exception&)
		{
			try
			{
				ThreadStatusGuard temp_status(tdbb);

				TRA_rollback(tdbb, transaction, true, false);
			}
			catch (const Exception&)
			{
				// no-op
			}

			throw;
		}
	}
}


void JRD_receive(thread_db* tdbb, jrd_req* request, USHORT msg_type, ULONG msg_length, UCHAR* msg)
{
/**************************************
 *
 *	J R D _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	EXE_receive(tdbb, request, msg_type, msg_length, msg, true);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


void JRD_send(thread_db* tdbb, jrd_req* request, USHORT msg_type, ULONG msg_length, const UCHAR* msg)
{
/**************************************
 *
 *	J R D _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	EXE_send(tdbb, request, msg_type, msg_length, msg);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


void JRD_start(Jrd::thread_db* tdbb, jrd_req* request, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	EXE_unwind(tdbb, request);
	EXE_start(tdbb, request, transaction);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


void JRD_commit_transaction(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ c o m m i t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction and keep the environment valid.
 *
 **************************************/
	commit(tdbb, transaction, false);
}


void JRD_commit_retaining(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ c o m m i t _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction.
 *
 **************************************/
	commit(tdbb, transaction, true);
}


void JRD_rollback_transaction(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ r o l l b a c k _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction.
 *
 **************************************/
	rollback(tdbb, transaction, false);
}


void JRD_rollback_retaining(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	J R D _ r o l l b a c k _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction but keep the environment valid
 *
 **************************************/
	rollback(tdbb, transaction, true);
}


void JRD_start_and_send(thread_db* tdbb, jrd_req* request, jrd_tra* transaction,
	USHORT msg_type, ULONG msg_length, const UCHAR* msg)
{
/**************************************
 *
 *	J R D _ s t a r t _ a n d _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	EXE_unwind(tdbb, request);
	EXE_start(tdbb, request, transaction);
	EXE_send(tdbb, request, msg_type, msg_length, msg);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


static void start_transaction(thread_db* tdbb, bool transliterate, jrd_tra** tra_handle,
	Jrd::Attachment* attachment, unsigned int tpb_length, const UCHAR* tpb)
{
/**************************************
 *
 *	s t a r t _ m u l t i p l e
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	fb_assert(attachment == tdbb->getAttachment());

	try
	{
		if (*tra_handle)
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));


		try
		{
			if (tpb_length > 0 && !tpb)
				status_exception::raise(Arg::Gds(isc_bad_tpb_form));

			jrd_tra* transaction = TRA_start(tdbb, tpb_length, tpb);

			*tra_handle = transaction;

			// run ON TRANSACTION START triggers
			EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_trans_start);
		}
		catch (const Exception& ex)
		{
			if (transliterate)
			{
				LocalStatus tempStatus;
				transliterateException(tdbb, ex, &tempStatus, "JAttachment::startTransaction");
				status_exception::raise(tempStatus.get());
			}
			throw;
		}
	}
	catch (const Exception&)
	{
		*tra_handle = NULL;
		throw;
	}
}


void JRD_start_transaction(thread_db* tdbb, jrd_tra** transaction,
   Jrd::Attachment* attachment, unsigned int tpb_length, const UCHAR* tpb)
{
/**************************************
 *
 *	J R D _ s t a r t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	start_transaction(tdbb, false, transaction, attachment, tpb_length, tpb);
}


void JRD_unwind_request(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	J R D _ u n w i n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Unwind a running request.  This is potentially nasty since it can
 *	be called asynchronously.
 *
 **************************************/
	// Unwind request. This just tweaks some bits.
	EXE_unwind(tdbb, request);
}


void JRD_compile(thread_db* tdbb,
				 Jrd::Attachment* attachment,
				 jrd_req** req_handle,
				 ULONG blr_length,
				 const UCHAR* blr,
				 RefStrPtr ref_str,
				 ULONG dbginfo_length,
				 const UCHAR* dbginfo,
				 bool isInternalRequest)
{
/**************************************
 *
 *	J R D _ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile a request passing the SQL text and debug information.
 *
 **************************************/
	if (*req_handle)
		status_exception::raise(Arg::Gds(isc_bad_req_handle));

	jrd_req* request = CMP_compile2(tdbb, blr, blr_length, isInternalRequest, dbginfo_length, dbginfo);
	request->req_attachment = attachment;
	attachment->att_requests.add(request);

	JrdStatement* statement = request->getStatement();

	if (!ref_str)
	{
		fb_assert(statement->blr.isEmpty());

		// hvlad: if\when we implement request's cache in the future and
		// CMP_compile2 will return us previously compiled request with
		// non-empty req_blr, then we must replace assertion by the line below
		// if (!statement->req_blr.isEmpty())

		statement->blr.insert(0, blr, blr_length);
	}
	else
		statement->sqlText = ref_str;

	*req_handle = request;
}


namespace
{
	class DatabaseDirectoryList : public DirectoryList
	{
	private:
		const PathName getConfigString() const
		{
			return PathName(Config::getDatabaseAccess());
		}
	public:
		explicit DatabaseDirectoryList(MemoryPool& p)
			: DirectoryList(p)
		{
			initialize();
		}
	};

	InitInstance<DatabaseDirectoryList> iDatabaseDirectoryList;
}


bool JRD_verify_database_access(const PathName& name)
{
/**************************************
 *
 *      J R D _ v e r i f y _ d a t a b a s e _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *      Verify 'name' against DatabaseAccess entry of firebird.conf.
 *
 **************************************/
	return iDatabaseDirectoryList().isPathInList(name);
}


void JRD_shutdown_attachments(Database* dbb)
{
/**************************************
 *
 *      J R D _ s h u t d o w n _ a t t a c h m e n t s
 *
 **************************************
 *
 * Functional description
 *  Schedule the attachments marked as shutdown for disconnection.
 *
 **************************************/
	fb_assert(dbb);

	try
	{
		MemoryPool& pool = *getDefaultMemoryPool();
		AttachmentsRefHolder* queue = FB_NEW(pool) AttachmentsRefHolder(pool);

		{	// scope
			Sync guard(&dbb->dbb_sync, "JRD_shutdown_attachments");
			if (!dbb->dbb_sync.ourExclusiveLock())
				guard.lock(SYNC_SHARED);

			for (const Jrd::Attachment* attachment = dbb->dbb_attachments;
				attachment; attachment = attachment->att_next)
			{
				if (attachment->att_flags & ATT_shutdown)
				{
					fb_assert(attachment->att_interface);
					attachment->att_interface->addRef();
					queue->add(attachment->att_interface);
				}
			}
		}

		Thread::start(attachmentShutdownThread, queue, 0);
	}
	catch (const Exception&)
	{} // no-op
}


void JRD_cancel_operation(thread_db* /*tdbb*/, Jrd::Attachment* attachment, int option)
{
/**************************************
 *
 *	J R D _ c a n c e l _ o p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Try to cancel an operation.
 *
 **************************************/
	switch (option)
	{
	case fb_cancel_disable:
		attachment->att_flags |= ATT_cancel_disable;
		attachment->att_flags &= ~ATT_cancel_raise;
		break;

	case fb_cancel_enable:
		if (attachment->att_flags & ATT_cancel_disable)
		{
			// avoid leaving ATT_cancel_raise set when cleaning ATT_cancel_disable
			// to avoid unexpected CANCEL (though it should not be set, but...)
			attachment->att_flags &= ~(ATT_cancel_disable | ATT_cancel_raise);
		}
		break;

	case fb_cancel_raise:
		if (!(attachment->att_flags & ATT_cancel_disable))
			attachment->signalCancel();
		break;

	default:
		fb_assert(false);
	}
}
