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
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../jrd/common.h"
#include "../jrd/ThreadStart.h"
#include "../jrd/os/thd_priority.h"
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
#include "../jrd/fil.h"
#include "../jrd/intl.h"
#include "../jrd/sbm.h"
#include "../jrd/svc.h"
#include "../jrd/sdw.h"
#include "../jrd/lls.h"
#include "../jrd/cch.h"
#include "../intl/charsets.h"
#include "../jrd/sort.h"
#include "../jrd/PreparedStatement.h"

#include "../jrd/blb_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dbg_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/ini_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/shut_proto.h"
#include "../jrd/sort_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/val_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/file_params.h"
#include "../jrd/event_proto.h"
#include "../jrd/why_proto.h"
#include "../jrd/flags.h"

#include "../jrd/Database.h"

#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../jrd/db_alias.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceObjects.h"
#include "../jrd/trace/TraceJrdHelpers.h"
#include "../jrd/IntlManager.h"
#include "../common/classes/fb_tls.h"
#include "../common/classes/ClumpletReader.h"
#include "../common/classes/RefMutex.h"
#include "../common/utils_proto.h"
#include "../jrd/DebugInterface.h"

#include "../dsql/dsql.h"
#include "../dsql/dsql_proto.h"
#include "../common/dllinst.h"

using namespace Jrd;
using namespace Firebird;

const SSHORT WAIT_PERIOD	= -1;

#ifdef SUPPORT_RAW_DEVICES
#define unlink PIO_unlink
#endif

#ifdef DEV_BUILD
int debug;
#endif

namespace
{
	Database* databases = NULL;
	GlobalPtr<Mutex> databases_mutex;
	bool engineShuttingDown = false;

	class EngineStartup
	{
	public:
		static void init()
		{
			IbUtil::initialize();
			IntlManager::initialize();
		}

		static void cleanup()
		{
		}
	};

	InitMutex<EngineStartup> engineStartup;

	inline void validateHandle(thread_db* tdbb, Attachment* const attachment)
	{
		if (attachment && attachment == tdbb->getAttachment())
			return;

		if (!attachment->checkHandle() || !attachment->att_database->checkHandle())
		{
			status_exception::raise(Arg::Gds(isc_bad_db_handle));
		}

		tdbb->setAttachment(attachment);
		tdbb->setDatabase(attachment->att_database);
	}

	inline void validateHandle(thread_db* tdbb, jrd_tra* const transaction)
	{
		if (!transaction->checkHandle())
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));

		validateHandle(tdbb, transaction->tra_attachment);

		tdbb->setTransaction(transaction);
	}

	inline void validateHandle(thread_db* tdbb, jrd_req* const request)
	{
		if (!request->checkHandle())
			status_exception::raise(Arg::Gds(isc_bad_req_handle));

		validateHandle(tdbb, request->req_attachment);
	}

	inline void validateHandle(thread_db* tdbb, dsql_req* const statement)
	{
		if (!statement->checkHandle())
			status_exception::raise(Arg::Gds(isc_bad_req_handle));

		validateHandle(tdbb, statement->req_dbb->dbb_attachment);
	}

	inline void validateHandle(thread_db* tdbb, blb* blob)
	{
		if (!blob->checkHandle())
			status_exception::raise(Arg::Gds(isc_bad_segstr_handle));

		validateHandle(tdbb, blob->blb_transaction);
		validateHandle(tdbb, blob->blb_attachment);
	}

	inline void validateHandle(Service* service)
	{
		if (service && service->checkHandle())
			return;

		status_exception::raise(Arg::Gds(isc_bad_svc_handle));
	}

	class AttachmentHolder
	{
	public:
		AttachmentHolder(thread_db* arg, bool lockAtt)
			: tdbb(arg)
		{
			Attachment* attachment = tdbb->getAttachment();
			if (lockAtt && attachment)
			{
				if (engineShuttingDown)
					status_exception::raise(Arg::Gds(isc_att_shutdown));

				attachment->att_mutex.enter();
				attLocked = true;
			}
			else
				attLocked = false;
		}

		~AttachmentHolder()
		{
			Attachment* attachment = tdbb->getAttachment();
			if (attLocked && attachment)
				attachment->att_mutex.leave();
		}

	private:
		thread_db* tdbb;
		bool attLocked;

	private:
		// copying is prohibited
		AttachmentHolder(const AttachmentHolder&);
		AttachmentHolder& operator =(const AttachmentHolder&);
	};

	class DatabaseContextHolder : public AttachmentHolder, Database::SyncGuard,
		public Jrd::ContextPoolHolder
	{
	public:
		explicit DatabaseContextHolder(thread_db* arg, bool lockAtt = true)
			: AttachmentHolder(arg, lockAtt),
			  Database::SyncGuard(arg->getDatabase()),
			  Jrd::ContextPoolHolder(arg, arg->getDatabase()->dbb_permanent),
			  tdbb(arg)
		{
			Database* dbb = tdbb->getDatabase();
			++dbb->dbb_use_count;
		}

		~DatabaseContextHolder()
		{
			Database* dbb = tdbb->getDatabase();
			if (dbb->checkHandle())
			{
				--dbb->dbb_use_count;
			}
		}

	private:
		// copying is prohibited
		DatabaseContextHolder(const DatabaseContextHolder&);
		DatabaseContextHolder& operator=(const DatabaseContextHolder&);

		thread_db* tdbb;
	};

	void validateAccess(const Attachment* attachment)
	{
		if (!attachment->locksmith())
		{
			ERR_post(Arg::Gds(isc_adm_task_denied));
		}
	}

} // anonymous

#ifdef  WIN_NT
#include <windows.h>
// these should stop a most annoying warning
#undef TEXT
#define TEXT    SCHAR
#endif	// WIN_NT

void Jrd::Trigger::compile(thread_db* tdbb)
{
	if (!request /*&& !compile_in_progress*/)
	{
		SET_TDBB(tdbb);

		Database* dbb = tdbb->getDatabase();

		Database::CheckoutLockGuard guard(dbb, dbb->dbb_meta_mutex);

		if (request)
		{
			return;
		}

		compile_in_progress = true;
		// Allocate statement memory pool
		MemoryPool* new_pool = dbb->createPool();
		// Trigger request is not compiled yet. Lets do it now
		USHORT par_flags = (USHORT) (flags & TRG_ignore_perm) ? csb_ignore_perm : 0;
		if (type & 1)
			par_flags |= csb_pre_trigger;
		else
			par_flags |= csb_post_trigger;

		try {
			Jrd::ContextPoolHolder context(tdbb, new_pool);

			AutoPtr<CompilerScratch> csb(CompilerScratch::newCsb(*dbb->dbb_permanent, 5));
			csb->csb_g_flags |= par_flags;

			if (!dbg_blob_id.isEmpty())
				DBG_parse_debug_info(tdbb, &dbg_blob_id, csb->csb_dbg_info);

			PAR_blr(tdbb, relation, blr.begin(), (ULONG) blr.getCount(), NULL, csb, &request,
				(relation ? true : false), par_flags);
		}
		catch (const Exception&)
		{
			compile_in_progress = false;

			if (request)
			{
				CMP_release(tdbb, request);
				request = NULL;
			}
			else {
				dbb->deletePool(new_pool);
			}

			throw;
		}

		request->req_trg_name = name;

		if (sys_trigger)
		{
			request->req_flags |= req_sys_trigger;
		}
		if (flags & TRG_ignore_perm)
		{
			request->req_flags |= req_ignore_perm;
		}

		compile_in_progress = false;
	}
}

void Jrd::Trigger::release(thread_db* tdbb)
{
	if (blr.getCount() == 0 || !request || CMP_clone_is_active(request))
	{
		return; // FALSE;
	}

	CMP_release(tdbb, request);
	request = NULL;
	return; // TRUE;
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
	bool	dpb_gsec_attach;
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
	bool	dpb_trusted_role;
	bool	dpb_utf8_filename;
	ULONG	dpb_ext_call_depth;
	ULONG	dpb_flags;			// to OR'd with dbb_flags

	// here begin compound objects
	// for constructor to work properly dpb_sys_user_name
	// MUST be FIRST
	string	dpb_sys_user_name;
	string	dpb_user_name;
	string	dpb_password;
	string	dpb_password_enc;
	string	dpb_role_name;
	string	dpb_journal;
	string	dpb_key;
	string	dpb_lc_ctype;
	PathName	dpb_working_directory;
	string	dpb_set_db_charset;
	string	dpb_network_protocol;
	string	dpb_remote_address;
	string	dpb_trusted_login;
	PathName	dpb_remote_process;
	PathName	dpb_org_filename;

public:
	DatabaseOptions()
	{
		memset(this, 0,
			reinterpret_cast<char*>(&this->dpb_sys_user_name) - reinterpret_cast<char*>(this));
	}
	void get(const UCHAR*, USHORT, bool&);

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

class TraceFailedConnection : public TraceDatabaseConnection
{
public:
	TraceFailedConnection(const char* filename, const DatabaseOptions* options) :
	  m_filename(filename),
	  m_options(options)
	{}

	virtual int getConnectionID()				{ return 0; }
	virtual int getProcessID()					{ return m_options->dpb_remote_pid; }
	virtual const char* getDatabaseName()		{ return m_filename; }

	virtual const char* getUserName()
	{
		if (m_options->dpb_user_name.empty())
			return m_options->dpb_trusted_login.c_str();

		return m_options->dpb_user_name.c_str();
	}

	virtual ntrace_connection_kind_t getKind()	{ return connection_database; }
	virtual const char* getRoleName()			{ return m_options->dpb_role_name.c_str(); }
	virtual const char* getCharSet()			{ return m_options->dpb_lc_ctype.c_str(); }
	virtual const char* getRemoteProtocol()		{ return m_options->dpb_network_protocol.c_str(); }
	virtual const char* getRemoteAddress()		{ return m_options->dpb_remote_address.c_str(); }
	virtual int getRemoteProcessID()			{ return m_options->dpb_remote_pid; }
	virtual const char* getRemoteProcessName()	{ return m_options->dpb_remote_process.c_str(); }

private:
	const char* m_filename;
	const DatabaseOptions* m_options;
};

static void			cancel_attachments();
static void			check_database(thread_db* tdbb);
static void			check_transaction(thread_db*, jrd_tra*);
static void			commit(thread_db*, jrd_tra*, const bool);
static bool			drop_files(const jrd_file*);
static void			find_intl_charset(thread_db*, Attachment*, const DatabaseOptions*);
static jrd_tra*		find_transaction(thread_db*, ISC_STATUS);
static void			init_database_lock(thread_db*);
static void			init_monitoring_lock(thread_db*);
static ISC_STATUS	handle_error(ISC_STATUS*, ISC_STATUS);
static void			run_commit_triggers(thread_db* tdbb, jrd_tra* transaction);
static void			verify_request_synchronization(jrd_req*& request, SSHORT level);
static unsigned int purge_transactions(thread_db*, Attachment*, const bool, const ULONG);
namespace {
	enum VdnResult {VDN_FAIL, VDN_OK, VDN_SECURITY};
}
static VdnResult	verifyDatabaseName(const PathName&, ISC_STATUS*, bool);
static ISC_STATUS	unwindAttach(const Exception& ex,
								 ISC_STATUS* userStatus,
								 thread_db* tdbb,
								 Attachment* attachment,
								 Database* dbb);
#ifdef WIN_NT
static void		ExtractDriveLetter(const TEXT*, ULONG*);
#endif

static Database*	init(thread_db*, const PathName&, bool);
static void		prepare(thread_db*, jrd_tra*, USHORT, const UCHAR*);
static void		release_attachment(thread_db*, Attachment*);
static void		detachLocksFromAttachment(Attachment*);
static void		rollback(thread_db*, jrd_tra*, const bool);
static void		shutdown_database(Database*, const bool);
static void		strip_quotes(string&);
static void		purge_attachment(thread_db*, Attachment*, const bool);
static void		getUserInfo(UserId&, const DatabaseOptions&);
static bool		shutdown_dbb(thread_db*, Database*);

static THREAD_ENTRY_DECLARE shutdown_thread(THREAD_ENTRY_PARAM);


static void cancel_attachments()
{
	MutexLockGuard guard(databases_mutex);
	engineShuttingDown = true;

	for (Database* dbb = databases; dbb; dbb = dbb->dbb_next)
	{
		if ( !(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use | DBB_security_db)) )
		{
			Database::SyncGuard dsGuard(dbb);
			Attachment* lockedAtt = NULL;
			Attachment* att = dbb->dbb_attachments;

			while (att)
			{
				// Try to cancel attachment and lock it. Handle case when attachment
				// deleted while waiting for lock.
				while (true)
				{
					if (att->att_mutex.tryEnter() || (att->att_flags & ATT_purge_error))
					{
						lockedAtt = att;
						break;
					}

					{
						const bool cancel_disable = (att->att_flags & ATT_cancel_disable);
						Database::Checkout dcoHolder(dbb);
						if (!cancel_disable)
						{
							ISC_STATUS_ARRAY status;
							jrd8_cancel_operation(status, &att, fb_cancel_enable);
							jrd8_cancel_operation(status, &att, fb_cancel_raise);
						}
						THREAD_SLEEP(10);
					}

					// check if attachment still exist
					if (lockedAtt && lockedAtt->att_next != att) {
						break;
					}
					if (dbb->dbb_attachments != att) {
						break;
					}
				}
				att = lockedAtt ? lockedAtt->att_next : dbb->dbb_attachments;
			}
		}
	}
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


static ISC_STATUS successful_completion(ISC_STATUS* status, ISC_STATUS return_code = FB_SUCCESS)
{
	fb_assert(status);

	// This assert validates whether we really have a successful status vector
	fb_assert(status[0] != isc_arg_gds || status[1] == FB_SUCCESS);

	// Clear the status vector if it doesn't contain a warning
	if (status[0] != isc_arg_gds || status[1] != FB_SUCCESS || status[2] != isc_arg_warning)
	{
		fb_utils::init_status(status);
	}

	return return_code;
}


const int SWEEP_INTERVAL		= 20000;

const char DBL_QUOTE			= '\042';
const char SINGLE_QUOTE			= '\'';

#define GDS_ATTACH_DATABASE		jrd8_attach_database
#define GDS_BLOB_INFO			jrd8_blob_info
#define GDS_CANCEL_BLOB			jrd8_cancel_blob
#define GDS_CANCEL_EVENTS		jrd8_cancel_events
#define FB_CANCEL_OPERATION		jrd8_cancel_operation
#define GDS_CLOSE_BLOB			jrd8_close_blob
#define GDS_COMMIT				jrd8_commit_transaction
#define GDS_COMMIT_RETAINING	jrd8_commit_retaining
#define GDS_COMPILE				jrd8_compile_request
#define GDS_CREATE_BLOB2		jrd8_create_blob2
#define GDS_CREATE_DATABASE		jrd8_create_database
#define GDS_DATABASE_INFO		jrd8_database_info
#define GDS_DDL					jrd8_ddl
#define GDS_DETACH				jrd8_detach_database
#define GDS_DROP_DATABASE		jrd8_drop_database
#define GDS_GET_SEGMENT			jrd8_get_segment
#define GDS_GET_SLICE			jrd8_get_slice
#define GDS_OPEN_BLOB2			jrd8_open_blob2
#define GDS_PREPARE				jrd8_prepare_transaction
#define GDS_PUT_SEGMENT			jrd8_put_segment
#define GDS_PUT_SLICE			jrd8_put_slice
#define GDS_QUE_EVENTS			jrd8_que_events
#define GDS_RECONNECT			jrd8_reconnect_transaction
#define GDS_RECEIVE				jrd8_receive
#define GDS_RELEASE_REQUEST		jrd8_release_request
#define GDS_REQUEST_INFO		jrd8_request_info
#define GDS_ROLLBACK			jrd8_rollback_transaction
#define GDS_ROLLBACK_RETAINING	jrd8_rollback_retaining
#define GDS_SEEK_BLOB			jrd8_seek_blob
#define GDS_SEND				jrd8_send
#define GDS_SERVICE_ATTACH		jrd8_service_attach
#define GDS_SERVICE_DETACH		jrd8_service_detach
#define GDS_SERVICE_QUERY		jrd8_service_query
#define GDS_SERVICE_START		jrd8_service_start
#define GDS_START_AND_SEND		jrd8_start_and_send
#define GDS_START				jrd8_start_request
#define GDS_START_MULTIPLE		jrd8_start_multiple
#define GDS_START_TRANSACTION	jrd8_start_transaction
#define GDS_TRANSACT_REQUEST	jrd8_transact_request
#define GDS_TRANSACTION_INFO	jrd8_transaction_info
#define GDS_UNWIND				jrd8_unwind_request
#define GDS_SHUTDOWN			jrd8_shutdown_all
#define GDS_PING				jrd8_ping_attachment

#define GDS_DSQL_ALLOCATE			jrd8_allocate_statement
#define GDS_DSQL_EXECUTE			jrd8_execute
#define GDS_DSQL_EXECUTE_IMMEDIATE	jrd8_execute_immediate
#define GDS_DSQL_FETCH				jrd8_fetch
#define GDS_DSQL_FREE				jrd8_free_statement
#define GDS_DSQL_INSERT				jrd8_insert
#define GDS_DSQL_PREPARE			jrd8_prepare
#define GDS_DSQL_SET_CURSOR			jrd8_set_cursor
#define GDS_DSQL_SQL_INFO			jrd8_sql_info

#define ENTRYPOINT_NAME(func)	STRINGIZE(func)


// External hook definitions

/* dimitr: just uncomment the following line to use this feature.
		   Requires support from the PIO modules. Only Win32 is 100% ready
		   for this so far. Note that the database encryption code in the
		   PIO layer seems to be incompatible with the SUPERSERVER_V2 code.
		   2003.02.09 */
//#define ISC_DATABASE_ENCRYPTION

static const char* CRYPT_IMAGE = "fbcrypt";
static const char* ENCRYPT = "encrypt";
static const char* DECRYPT = "decrypt";


static ISC_STATUS trace_error(thread_db* tdbb, const Exception& ex, ISC_STATUS* user_status, const char* func)
{
	const ISC_STATUS ret = ex.stuff_exception(user_status);

	Attachment* att = tdbb->getAttachment();
	if (ret == isc_bad_db_handle || !att)
		return ret;

	if (att->att_trace_manager->needs().event_error)
	{
		TraceConnectionImpl conn(att);
		TraceStatusVectorImpl traceStatus(user_status);
		
		att->att_trace_manager->event_error(&conn, &traceStatus, func);
	}

	return ret;
}

static void trace_warning(thread_db* tdbb, ISC_STATUS* user_status, const char* func)
{
	Attachment* att = tdbb->getAttachment();
	if (!att)
		return;

	if (att->att_trace_manager->needs().event_error)
	{
		TraceStatusVectorImpl traceStatus(user_status);
		
		if (traceStatus.hasWarning())
		{
			TraceConnectionImpl conn(att);
			att->att_trace_manager->event_error(&conn, &traceStatus, func);
		}
	}
}

static void trace_failed_attach(TraceManager* traceManager, const char* filename,
	const DatabaseOptions& options, bool create, ISC_STATUS* status)
{
	// Report to Trace API that attachment has not been created
	const char* origFilename = filename;
	if (options.dpb_org_filename.hasData())
		origFilename = options.dpb_org_filename.c_str();

	TraceFailedConnection conn(origFilename, &options);
	TraceStatusVectorImpl traceStatus(status);

	const bool no_priv = (status[1] == isc_login || status[1] == isc_no_priv);
	const char* func = create ? ENTRYPOINT_NAME(GDS_CREATE_DATABASE) : 
		ENTRYPOINT_NAME(GDS_ATTACH_DATABASE);

	if (!traceManager)
	{
		TraceManager tempMgr(origFilename);

		if (tempMgr.needs().event_attach)
			tempMgr.event_attach(&conn, create, no_priv ? res_unauthorized : res_failed);

		if (tempMgr.needs().event_error)
			tempMgr.event_error(&conn, &traceStatus, func);
	}
	else
	{
		if (traceManager->needs().event_attach)
			traceManager->event_attach(&conn, create, no_priv ? res_unauthorized : res_failed);

		if (traceManager->needs().event_error)
			traceManager->event_error(&conn, &traceStatus, func);
	}
}


ISC_STATUS GDS_ATTACH_DATABASE(ISC_STATUS* user_status,
								const TEXT* filename,
								Attachment** handle,
								SSHORT dpb_length,
								const UCHAR* dpb)
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
	ThreadContextHolder tdbb(user_status);

	if (*handle)
	{
		return handle_error(user_status, isc_bad_db_handle);
	}

	UserId userId;
	DatabaseOptions options;
	bool invalid_client_SQL_dialect = false;
	PathName file_name, expanded_name;
	bool is_alias = false;

	try
	{
		// Process database parameter block
		options.get(dpb, dpb_length, invalid_client_SQL_dialect);

		if (options.dpb_org_filename.hasData())
			file_name = options.dpb_org_filename;
		else
		{
			file_name = filename;

			if (!options.dpb_utf8_filename)
				ISC_systemToUtf8(file_name);

			ISC_unescape(file_name);
		}

		ISC_utf8ToSystem(file_name);

		// Resolve given alias name
		is_alias = ResolveDatabaseAlias(file_name, expanded_name);
		if (is_alias)
		{
			ISC_systemToUtf8(expanded_name);
			ISC_unescape(expanded_name);
			ISC_utf8ToSystem(expanded_name);
			ISC_expand_filename(expanded_name, false);
		}
		else
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
			return handle_error(user_status, isc_unavailable);
		}

		// Check for correct credentials supplied
		getUserInfo(userId, options);
	}
	catch (const DelayFailedLogin& ex)
	{
		const ISC_STATUS ret = ex.stuff_exception(user_status);
		trace_failed_attach(NULL, filename, options, false, user_status);

		ex.sleep();
		return ret;
	}
	catch (const Exception& ex)
	{
		const ISC_STATUS ret = ex.stuff_exception(user_status);
		trace_failed_attach(NULL, filename, options, false, user_status);
		return ret;
	}

	// Check database against conf file.
	const VdnResult vdn = verifyDatabaseName(expanded_name, user_status, is_alias);
	if (!is_alias && vdn == VDN_FAIL)
	{
		trace_failed_attach(NULL, filename, options, false, user_status);
		return user_status[1];
	}

	Database* dbb = NULL;
	MutexEnsureUnlock guardDatabases(databases_mutex);
	guardDatabases.enter();

	try
	{
		// Unless we're already attached, do some initialization
		dbb = init(tdbb, expanded_name, true);
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	fb_assert(dbb);

	tdbb->setDatabase(dbb);
	DatabaseContextHolder dbbHolder(tdbb);

	dbb->dbb_flags |= DBB_being_opened;

	// Initialize special error handling

	Attachment* attachment = NULL;

	bool initing_security = false;

	try {

/* If database to be opened is security database, then only
   gsec or SecurityDatabase may open it. This protects from use
   of old gsec to write wrong password hashes into it. */
	if (vdn == VDN_SECURITY && !options.dpb_gsec_attach && !options.dpb_sec_attach)
	{
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("direct") <<
										  Arg::Str("security database") <<
										  Arg::Str(file_name));
	}

	// Worry about encryption key

	if (dbb->dbb_decrypt)
	{
		if (dbb->dbb_filename.hasData() &&
			(dbb->dbb_encrypt_key.hasData() || options.dpb_key.hasData()))
		{
			if ((dbb->dbb_encrypt_key.hasData() && options.dpb_key.isEmpty()) ||
				(dbb->dbb_encrypt_key.empty() && options.dpb_key.hasData()) ||
				(dbb->dbb_encrypt_key != options.dpb_key))
			{
				ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("encryption") <<
												  Arg::Str("database") <<
												  Arg::Str(file_name));
			}
		}
		else if (options.dpb_key.hasData())
		{
			dbb->dbb_encrypt_key = options.dpb_key;
		}
	}

	attachment = Attachment::create(dbb);
	tdbb->setAttachment(attachment);
	attachment->att_filename = is_alias ? file_name : expanded_name;
	attachment->att_network_protocol = options.dpb_network_protocol;
	attachment->att_remote_address = options.dpb_remote_address;
	attachment->att_remote_pid = options.dpb_remote_pid;
	attachment->att_remote_process = options.dpb_remote_process;
	attachment->att_next = dbb->dbb_attachments;
	attachment->att_ext_call_depth = options.dpb_ext_call_depth;

	// make attachment keep sec_db fini info
	attachment->att_fini_sec_db = userId.usr_fini_sec_db;
	userId.usr_fini_sec_db = false;

	dbb->dbb_attachments = attachment;
	dbb->dbb_flags &= ~DBB_being_opened;
	dbb->dbb_sys_trans->tra_attachment = attachment;

	attachment->att_charset = options.dpb_interp;

	if (options.dpb_no_garbage)
		attachment->att_flags |= ATT_no_cleanup;

	if (options.dpb_gbak_attach)
		attachment->att_flags |= ATT_gbak_attachment;

	if (options.dpb_gstat_attach)
		attachment->att_flags |= ATT_gstat_attachment;

	if (options.dpb_gfix_attach)
		attachment->att_flags |= ATT_gfix_attachment;

	if (options.dpb_working_directory.hasData()) {
		attachment->att_working_directory = options.dpb_working_directory;
	}

	// If we're a not a secondary attachment, initialize some stuff

	bool first = false;

	if (dbb->dbb_filename.empty())
	{
#if defined(DEV_BUILD) && defined(SUPERSERVER)
		// make sure we do not reopen same DB twice
		for (Database* d = databases; d; d = d->dbb_next)
		{
			if (d->dbb_filename == expanded_name)
			{
				fatal_exception::raise(("Attempt to reopen " + expanded_name).c_str());
			}
		}
#endif
		first = true;
		dbb->dbb_filename = expanded_name;
		dbb->dbb_flags |= options.dpb_flags;

		// NS: Use alias as database ID only if accessing database using file name is not possible.
		//
		// This way we:
		// 1. Ensure uniqueness of ID even in presence of multiple processes
		// 2. Make sure that ID value can be used to connect back to database
		//
		if (is_alias && vdn == VDN_FAIL)
			dbb->dbb_database_name = file_name;
		else
			dbb->dbb_database_name = expanded_name;

		PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
		pageSpace->file = PIO_open(dbb, expanded_name, file_name, false);

		// Initialize the lock manager
		dbb->dbb_lock_mgr = LockManager::create(dbb->getUniqueFileId());

		LCK_init(tdbb, LCK_OWNER_database);
		dbb->dbb_flags |= DBB_lck_init_done;

		LCK_init(tdbb, LCK_OWNER_attachment);
		attachment->att_flags |= ATT_lck_init_done;

		// Initialize the database lock
		init_database_lock(tdbb);

		INI_init(tdbb);
		SHUT_init(tdbb);
		PAG_header_init(tdbb);
		INI_init2(tdbb);
		PAG_init(tdbb);

		if (options.dpb_set_page_buffers)
		{
#ifdef SUPERSERVER
			// Here we do not let anyone except SYSDBA (like DBO) to change dbb_page_buffers,
			// cause other flags is UserId can be set only when DB is opened.
			// No idea how to test for other cases before init is complete.
			if (userId.locksmith())
#endif
				dbb->dbb_page_buffers = options.dpb_page_buffers;
		}

		CCH_init(tdbb, options.dpb_buffers);

		// Initialize backup difference subsystem. This must be done before WAL and shadowing
		// is enabled because nbackup it is a lower level subsystem
		dbb->dbb_backup_manager = FB_NEW(*dbb->dbb_permanent) BackupManager(tdbb, dbb, nbak_state_unknown);

		PAG_init2(tdbb, 0);
		PAG_header(tdbb, false);

		// initialize shadowing as soon as the database is ready for it
		// but before any real work is done
		SDW_init(tdbb, options.dpb_activate_shadow, options.dpb_delete_shadow);

		// Turn monitoring on
		init_monitoring_lock(tdbb);
	}
	else
	{
		if ((dbb->dbb_flags & options.dpb_flags) != options.dpb_flags)
		{
			// looks like someone tries to attach incompatibly
			status_exception::raise(Arg::Gds(isc_bad_dpb_content));
		}

		fb_assert(dbb->dbb_lock_mgr);

		LCK_init(tdbb, LCK_OWNER_attachment);
		attachment->att_flags |= ATT_lck_init_done;
	}

	// Attachments to a ReadOnly database need NOT do garbage collection
	if (dbb->dbb_flags & DBB_read_only) {
		attachment->att_flags |= ATT_no_cleanup;
	}

	if (options.dpb_disable_wal)
	{
		ERR_post(Arg::Gds(isc_lock_timeout) <<
				 Arg::Gds(isc_obj_in_use) << Arg::Str(file_name));
	}

	if (options.dpb_buffers && !dbb->dbb_page_buffers) {
		CCH_expand(tdbb, options.dpb_buffers);
	}

	if (!options.dpb_verify && CCH_exclusive(tdbb, LCK_PW, LCK_NO_WAIT))
	{
		TRA_cleanup(tdbb);
	}

	initing_security = true;

	if (invalid_client_SQL_dialect)
	{
		ERR_post(Arg::Gds(isc_inv_client_dialect_specified) << Arg::Num(options.dpb_sql_dialect) <<
				 Arg::Gds(isc_valid_client_dialects) << Arg::Str("1, 2 or 3"));
	}

	if (userId.usr_sql_role_name.hasData())
	{
		attachment->att_requested_role = userId.usr_sql_role_name;

		CharSet* utf8CharSet = IntlUtil::getUtf8CharSet();

		switch (options.dpb_sql_dialect)
		{
		case 0:
			// V6 Client --> V6 Server, dummy client SQL dialect 0 was passed
			// It means that client SQL dialect was not set by user
			// and takes DB SQL dialect as client SQL dialect
			if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_10_0)
			{
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
			}
			else {
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

		switch (options.dpb_sql_dialect)
		{
		case SQL_DIALECT_V5:
			{
				strip_quotes(userId.usr_sql_role_name);
				IntlUtil::toUpper(utf8CharSet, userId.usr_sql_role_name);
			}
			break;
		case SQL_DIALECT_V6_TRANSITION:
		case SQL_DIALECT_V6:
			{
				string& role = userId.usr_sql_role_name;
				if (role.hasData() && (role[0] == DBL_QUOTE || role[0] == SINGLE_QUOTE))
				{
					const char end_quote = role[0];
					// remove the delimited quotes and escape quote
					// from ROLE name
					role.erase(0, 1);
					for (string::iterator p = role.begin(); p < role.end(); ++p)
					{
						if (*p == end_quote)
						{
							if (++p < role.end() && *p == end_quote)
							{
								// skip the escape quote here
								role.erase(p--);
							}
							else
							{
								// delimited done
								role.erase(--p, role.end());
							}
						}
					}
				}
				else
					IntlUtil::toUpper(utf8CharSet, role);
			}
			break;
		default:
			break;
		}
	}

	options.dpb_sql_dialect = 0;

	SCL_init(tdbb, false, userId);

	initing_security = false;

	// This pair (SHUT_database/SHUT_online) checks itself for valid user name
	if (options.dpb_shutdown)
	{
		SHUT_database(tdbb, options.dpb_shutdown, options.dpb_shutdown_delay);
	}
	if (options.dpb_online)
	{
		SHUT_online(tdbb, options.dpb_online);
	}

#ifdef SUPERSERVER
/* Check if another attachment has or is requesting exclusive database access.
   If this is an implicit attachment for the security (password) database, don't
   try to get exclusive attachment to avoid a deadlock condition which happens
   when a client tries to connect to the security database itself. */

	if (!options.dpb_sec_attach)
	{
		bool attachment_succeeded = true;
		if (dbb->dbb_ast_flags & DBB_shutdown_single)
			attachment_succeeded = CCH_exclusive_attachment(tdbb, LCK_none, -1);
		else
			CCH_exclusive_attachment(tdbb, LCK_none, LCK_WAIT);
		if (attachment->att_flags & ATT_shutdown)
		{
			if (dbb->dbb_ast_flags & DBB_shutdown) {
				ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(file_name));
			}
			else {
				ERR_post(Arg::Gds(isc_att_shutdown));
			}
		}
		if (!attachment_succeeded) {
			ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(file_name));
		}
	}
#endif

	// If database is shutdown then kick 'em out.

	if (dbb->dbb_ast_flags & (DBB_shut_attach | DBB_shut_tran))
	{
		ERR_post(Arg::Gds(isc_shutinprog) << Arg::Str(file_name));
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
				allow_access = CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD);
			}
		}
		if (!allow_access)
		{
			// Note we throw exception here when entering full-shutdown mode
			ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(file_name));
		}
	}

	// Figure out what character set & collation this attachment prefers

	find_intl_charset(tdbb, attachment, &options);

/*
 * if the attachment is through gbak and this attachment is not by owner
 * or sysdba then return error. This has been added here to allow for the
 * GBAK security feature of only allowing the owner or sysdba to backup a
 * database. smistry 10/5/98
 */

	if ((attachment->att_flags & ATT_gbak_attachment) ||
		(attachment->att_flags & ATT_gfix_attachment) ||
		(attachment->att_flags & ATT_gstat_attachment))
	{
		validateAccess(attachment);
	}

	if (options.dpb_verify)
	{
		validateAccess(attachment);
		if (!CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD)) {
			ERR_post(Arg::Gds(isc_bad_dpb_content) << Arg::Gds(isc_cant_validate));
		}

#ifdef GARBAGE_THREAD
		// Can't allow garbage collection during database validation.

		VIO_fini(tdbb);
#endif
		if (!VAL_validate(tdbb, options.dpb_verify)) {
			ERR_punt();
		}
	}

	if (options.dpb_journal.hasData()) {
		ERR_post(Arg::Gds(isc_bad_dpb_content) << Arg::Gds(isc_cant_start_journal));
	}

	if (options.dpb_wal_action)
	{
		// No WAL anymore. We deleted it.
		ERR_post(Arg::Gds(isc_no_wal));
	}

	if (((attachment->att_flags & ATT_gfix_attachment) ||
		(attachment->att_flags & ATT_gstat_attachment)))
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

	if (options.dpb_sweep_interval != -1)
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
#ifdef SUPERSERVER
		validateAccess(attachment);
#else
		if (attachment->locksmith())
#endif
			PAG_set_page_buffers(tdbb, options.dpb_page_buffers);
	}

	if (options.dpb_set_db_readonly)
	{
		validateAccess(attachment);
		if (!CCH_exclusive(tdbb, LCK_EX, WAIT_PERIOD))
		{
			ERR_post(Arg::Gds(isc_lock_timeout) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str(file_name));
		}
		PAG_set_db_readonly(tdbb, options.dpb_db_readonly);
	}

	PAG_attachment_id(tdbb);

#ifdef GARBAGE_THREAD
	VIO_init(tdbb);
#endif

	CCH_release_exclusive(tdbb);

	// if there was an error, the status vector is all set

	guardDatabases.leave();

	if (options.dpb_sweep & isc_dpb_records)
	{
		if (!TRA_sweep(tdbb, 0)) {
			ERR_punt();
		}
	}

	if (options.dpb_dbkey_scope) {
		attachment->att_dbkey_trans = TRA_start(tdbb, 0, 0);
	}

	// Recover database after crash during backup difference file merge
	dbb->dbb_backup_manager->endBackup(tdbb, true); // true = do recovery

	if (attachment->att_trace_manager->needs().event_attach)
	{
		TraceConnectionImpl conn(attachment);
		attachment->att_trace_manager->event_attach(&conn, false, res_successful);
	}

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

			const trig_vec* trig_connect = dbb->dbb_triggers[DB_TRIGGER_CONNECT];
			if (trig_connect && !trig_connect->isEmpty())
			{
				// Start a transaction to execute ON CONNECT triggers.
				// Ensure this transaction can't trigger auto-sweep.
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

	// guardDatabases.leave();

	*handle = attachment;
	attachment->att_mutex.leave();

	}	// try
	catch (const Exception& ex)
	{
		ex.stuff_exception(user_status);
		trace_failed_attach(attachment ? attachment->att_trace_manager : NULL,
			filename, options, false, user_status);

		return unwindAttach(ex, user_status, tdbb, attachment, dbb);
	}

	return FB_SUCCESS;
}


ISC_STATUS GDS_BLOB_INFO(ISC_STATUS*	user_status,
						blb**	blob_handle,
						SSHORT	item_length,
						const SCHAR*	items,
						SSHORT	buffer_length,
						SCHAR*	buffer)
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
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			const UCHAR* items2 = reinterpret_cast<const UCHAR*>(items);
			UCHAR* buffer2 = reinterpret_cast<UCHAR*>(buffer);
			INF_blob_info(blob, items2, item_length, buffer2, buffer_length);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_BLOB_INFO));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_CANCEL_BLOB(ISC_STATUS* user_status, blb** blob_handle)
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
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			BLB_cancel(tdbb, blob);
			*blob_handle = NULL;
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_CANCEL_BLOB));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_CANCEL_EVENTS(ISC_STATUS* user_status, Attachment** handle, SLONG* id)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			Database* const dbb = tdbb->getDatabase();

			if (dbb->dbb_event_mgr)
			{
				dbb->dbb_event_mgr->cancelEvents(*id);
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_CANCEL_EVENTS));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS FB_CANCEL_OPERATION(ISC_STATUS* user_status, Attachment** handle, USHORT option)
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
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb, false);

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
			{
				attachment->att_flags |= ATT_cancel_raise;
				attachment->cancelExternalConnection(tdbb);
				LCK_cancel_wait(attachment);
			}
			break;

		default:
			fb_assert(false);
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_CLOSE_BLOB(ISC_STATUS* user_status, blb** blob_handle)
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
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			BLB_close(tdbb, blob);
			*blob_handle = NULL;
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_CLOSE_BLOB));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_COMMIT(ISC_STATUS* user_status, jrd_tra** tra_handle)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			JRD_commit_transaction(tdbb, tra_handle);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_COMMIT));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_COMMIT_RETAINING(ISC_STATUS* user_status, jrd_tra** tra_handle)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			JRD_commit_retaining(tdbb, tra_handle);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_COMMIT_RETAINING));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_COMPILE(ISC_STATUS* user_status,
						Attachment** db_handle,
						jrd_req** req_handle,
						SSHORT blr_length,
						const SCHAR* blr)
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
	try
	{
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		check_database(tdbb);

		TraceBlrCompile trace(tdbb, blr_length, (UCHAR*) blr);
		try
		{
			JRD_compile(tdbb, attachment, req_handle,
						(USHORT) blr_length, reinterpret_cast<const UCHAR*>(blr),
						RefStrPtr(), 0, NULL);

			fb_assert(*req_handle);
			trace.finish(*req_handle, res_successful);
		}
		catch (const Exception& ex)
		{
			const ISC_LONG exc = ex.stuff_exception(user_status);
			const bool no_priv = (exc == isc_no_priv);
			trace.finish(NULL, no_priv ? res_unauthorized : res_failed);

			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_COMPILE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_CREATE_BLOB2(ISC_STATUS* user_status,
							Attachment** db_handle,
							jrd_tra** tra_handle,
							blb** blob_handle,
							bid* blob_id,
							USHORT bpb_length,
							const UCHAR* bpb)
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
	try
	{
		if (*blob_handle)
		{
			status_exception::raise(Arg::Gds(isc_bad_segstr_handle));
		}

		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *db_handle);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			jrd_tra* const transaction = find_transaction(tdbb, isc_segstr_wrong_db);

			*blob_handle = BLB_create2(tdbb, transaction, blob_id, bpb_length, bpb);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_CREATE_BLOB2));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_CREATE_DATABASE(ISC_STATUS* user_status,
							   const TEXT* filename,
							   Attachment** handle,
							   USHORT dpb_length,
							   const UCHAR* dpb)
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
	ThreadContextHolder tdbb(user_status);

	if (*handle)
	{
		return handle_error(user_status, isc_bad_db_handle);
	}

	UserId userId;
	DatabaseOptions options;
	PathName file_name, expanded_name;
	bool is_alias = false;

	try {
		// Process database parameter block
		bool invalid_client_SQL_dialect = false;
		options.get(dpb, dpb_length, invalid_client_SQL_dialect);
		if (!invalid_client_SQL_dialect && options.dpb_sql_dialect == 99) {
			options.dpb_sql_dialect = 0;
		}

		if (options.dpb_org_filename.hasData())
			file_name = options.dpb_org_filename;
		else
		{
			file_name = filename;

			if (!options.dpb_utf8_filename)
				ISC_systemToUtf8(file_name);

			ISC_unescape(file_name);
		}

		ISC_utf8ToSystem(file_name);

		// Resolve given alias name
		is_alias = ResolveDatabaseAlias(file_name, expanded_name);
		if (is_alias)
		{
			ISC_systemToUtf8(expanded_name);
			ISC_unescape(expanded_name);
			ISC_utf8ToSystem(expanded_name);
			ISC_expand_filename(expanded_name, false);
		}
		else
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
			return handle_error(user_status, isc_unavailable);
		}

		// Check for correct credentials supplied
		getUserInfo(userId, options);
	}
	catch (const DelayFailedLogin& ex)
	{
		const ISC_STATUS ret = ex.stuff_exception(user_status);
		trace_failed_attach(NULL, filename, options, true, user_status);

		ex.sleep();
		return ret; 
	}
	catch (const Exception& ex)
	{
		const ISC_STATUS ret = ex.stuff_exception(user_status);
		trace_failed_attach(NULL, filename, options, true, user_status);
		return ret; 
	}

	// Check database against conf file.
	const VdnResult vdn = verifyDatabaseName(expanded_name, user_status, is_alias);
	if (!is_alias && vdn == VDN_FAIL)
	{
		trace_failed_attach(NULL, filename, options, true, false);
		return user_status[1];
	}

	Database* dbb = NULL;
	MutexEnsureUnlock guardDatabases(databases_mutex);
	guardDatabases.enter();

	try
	{
		dbb = init(tdbb, expanded_name, false);
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	fb_assert(dbb);

	tdbb->setDatabase(dbb);
	DatabaseContextHolder dbbHolder(tdbb);

	dbb->dbb_flags |= (DBB_being_opened | options.dpb_flags);

	Attachment* attachment = NULL;

	bool initing_security = false;

	try {

	if (options.dpb_key.hasData())
	{
		dbb->dbb_encrypt_key = options.dpb_key;
	}

	attachment = Attachment::create(dbb);
	tdbb->setAttachment(attachment);
	attachment->att_filename = is_alias ? file_name : expanded_name;
	attachment->att_network_protocol = options.dpb_network_protocol;
	attachment->att_remote_address = options.dpb_remote_address;
	attachment->att_remote_pid = options.dpb_remote_pid;
	attachment->att_remote_process = options.dpb_remote_process;
	attachment->att_ext_call_depth = options.dpb_ext_call_depth;
	attachment->att_next = dbb->dbb_attachments;

	// make attachment keep sec_db fini info
	attachment->att_fini_sec_db = userId.usr_fini_sec_db;
	userId.usr_fini_sec_db = false;

	dbb->dbb_attachments = attachment;
	dbb->dbb_flags &= ~DBB_being_opened;
	dbb->dbb_sys_trans->tra_attachment = attachment;

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
	case 0:
		// This can be issued by QLI, GDEF and old BDE clients.
		// In this case assume dialect 1
		options.dpb_sql_dialect = SQL_DIALECT_V5;
	case SQL_DIALECT_V5:
		break;
	case SQL_DIALECT_V6:
		dbb->dbb_flags |= DBB_DB_SQL_dialect_3;
		break;
	default:
		ERR_post(Arg::Gds(isc_database_create_failed) << Arg::Str(expanded_name) <<
				 Arg::Gds(isc_inv_dialect_specified) << Arg::Num(options.dpb_sql_dialect) <<
				 Arg::Gds(isc_valid_db_dialects) << Arg::Str("1 and 3"));
		break;
	}

	attachment->att_charset = options.dpb_interp;

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

	initing_security = false;

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	try
	{
		// try to create with overwrite = false
		pageSpace->file = PIO_create(dbb, expanded_name, false, false, false);
	}
	catch (status_exception)
	{
		if (options.dpb_overwrite)
		{
			if (GDS_ATTACH_DATABASE(user_status, filename, handle, dpb_length, dpb) == isc_adm_task_denied)
			{
				throw;
			}

			bool allow_overwrite = false;

			if (*handle)
			{
				allow_overwrite = (*handle)->att_user->locksmith();
				GDS_DETACH(user_status, handle);
			}
			else
			{
				// clear status after failed attach
				fb_utils::init_status(user_status);
				allow_overwrite = true;
			}

			if (allow_overwrite)
			{
				// file is a database and the user (SYSDBA or owner) has right to overwrite
				pageSpace->file = PIO_create(dbb, expanded_name, options.dpb_overwrite, false, false);
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

	const jrd_file* const first_dbb_file = pageSpace->file;

	// Initialize the lock manager
	dbb->dbb_lock_mgr = LockManager::create(dbb->getUniqueFileId());

	LCK_init(tdbb, LCK_OWNER_database);
	dbb->dbb_flags |= DBB_lck_init_done;

	LCK_init(tdbb, LCK_OWNER_attachment);
	attachment->att_flags |= ATT_lck_init_done;

	// Initialize the database lock
	init_database_lock(tdbb);

	INI_init(tdbb);
	PAG_init(tdbb);
	initing_security = true;

	attachment->att_requested_role = userId.usr_sql_role_name;
	SCL_init(tdbb, true, userId);

	if (options.dpb_set_page_buffers)
		dbb->dbb_page_buffers = options.dpb_page_buffers;

	CCH_init(tdbb, options.dpb_buffers);

#ifdef WIN_NT
	dbb->dbb_filename.assign(first_dbb_file->fil_string);
#else
	dbb->dbb_filename = expanded_name;
#endif

	// NS: Use alias as database ID only if accessing database using file name is not possible.
	//
	// This way we:
	// 1. Ensure uniqueness of ID even in presence of multiple processes
	// 2. Make sure that ID value can be used to connect back to database
	//
	if (is_alias && vdn == VDN_FAIL)
		dbb->dbb_database_name = file_name;
	else
		dbb->dbb_database_name = dbb->dbb_filename;

	// Initialize backup difference subsystem. This must be done before WAL and shadowing
	// is enabled because nbackup it is a lower level subsystem
	dbb->dbb_backup_manager = FB_NEW(*dbb->dbb_permanent) BackupManager(tdbb, dbb, nbak_state_normal);

	dbb->dbb_backup_manager->dbCreating = true;
	PAG_format_header(tdbb);
	INI_init2(tdbb);
	PAG_format_log(tdbb);
	PAG_format_pip(tdbb, *pageSpace);

	if (options.dpb_set_page_buffers)
		PAG_set_page_buffers(tdbb, options.dpb_page_buffers);

	if (options.dpb_set_no_reserve)
		PAG_set_no_reserve(tdbb, options.dpb_no_reserve);

	INI_format(attachment->att_user->usr_user_name.c_str(), options.dpb_set_db_charset.c_str());

	// There is no point to move database online at database creation since it is online by default.
	// We do not allow to create database that is fully shut down.
	if (options.dpb_online || (options.dpb_shutdown & isc_dpb_shut_mode_mask) == isc_dpb_shut_full)
		ERR_post(Arg::Gds(isc_bad_shutdown_mode) << Arg::Str(file_name));

	if (options.dpb_shutdown) {
		SHUT_database(tdbb, options.dpb_shutdown, options.dpb_shutdown_delay);
	}

	if (options.dpb_sweep_interval != -1)
	{
		PAG_sweep_interval(tdbb, options.dpb_sweep_interval);
		dbb->dbb_sweep_interval = options.dpb_sweep_interval;
	}

	if (options.dpb_set_force_write)
		PAG_set_force_write(tdbb, options.dpb_force_write);

	// initialize shadowing semaphore as soon as the database is ready for it
	// but before any real work is done

	SDW_init(tdbb, options.dpb_activate_shadow, options.dpb_delete_shadow);

#ifdef GARBAGE_THREAD
	VIO_init(tdbb);
#endif

	// Turn monitoring on
	init_monitoring_lock(tdbb);

    if (options.dpb_set_db_readonly)
    {
        if (!CCH_exclusive (tdbb, LCK_EX, WAIT_PERIOD))
        {
            ERR_post(Arg::Gds(isc_lock_timeout) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str(file_name));
		}

        PAG_set_db_readonly(tdbb, options.dpb_db_readonly);
    }

	PAG_attachment_id(tdbb);

	CCH_release_exclusive(tdbb);

	// Figure out what character set & collation this attachment prefers

	find_intl_charset(tdbb, attachment, &options);

	CCH_flush(tdbb, FLUSH_FINI, 0);

	dbb->dbb_backup_manager->dbCreating = false;

	// Report that we created attachment to Trace API
	if (attachment->att_trace_manager->needs().event_attach)
	{
		TraceConnectionImpl conn(attachment);
		attachment->att_trace_manager->event_attach(&conn, true, res_successful);
	}

	guardDatabases.leave();

	*handle = attachment;
	attachment->att_mutex.leave();

	}	// try
	catch (const Exception& ex)
	{
		ex.stuff_exception(user_status);
		trace_failed_attach(attachment ? attachment->att_trace_manager : NULL,
			filename, options, true, user_status);

		return unwindAttach(ex, user_status, tdbb, attachment, dbb);
	}

	return FB_SUCCESS;
}


ISC_STATUS GDS_DATABASE_INFO(ISC_STATUS* user_status,
							Attachment** handle,
							SSHORT item_length,
							const SCHAR* items,
							SSHORT buffer_length,
							SCHAR* buffer)
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
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			const UCHAR* items2 = reinterpret_cast<const UCHAR*>(items);
			UCHAR* buffer2 = reinterpret_cast<UCHAR*>(buffer);
			INF_database_info(items2, item_length, buffer2, buffer_length);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DATABASE_INFO));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DDL(ISC_STATUS* user_status,
					Attachment** db_handle,
					jrd_tra** tra_handle,
					USHORT ddl_length,
					const SCHAR* ddl)
{
/**************************************
 *
 *	g d s _ $ d d l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		check_database(tdbb);

		jrd_tra* const transaction = find_transaction(tdbb, isc_segstr_wrong_db);

		TraceDynExecute trace(tdbb, ddl_length, (UCHAR*) ddl);
		try
		{
			JRD_ddl(tdbb, /*attachment,*/ transaction, ddl_length, reinterpret_cast<const UCHAR*>(ddl));

			trace.finish(res_successful);
		}
		catch (const Exception& ex)
		{
			const ISC_STATUS exc = ex.stuff_exception(user_status);
			trace.finish(exc == FB_SUCCESS ? res_successful : res_failed);

			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DDL));
		}
	}
	catch (const Exception& ex) {
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DETACH(ISC_STATUS* user_status, Attachment** handle)
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
	try
	{
		ThreadContextHolder tdbb(user_status);

		{ // scope
			MutexLockGuard guard(databases_mutex);

			Attachment* const attachment = *handle;
			validateHandle(tdbb, attachment);

			{ // holder scope
				DatabaseContextHolder dbbHolder(tdbb);

				Database* dbb = tdbb->getDatabase();

				// if this is the last attachment, mark dbb as not in use
				if (dbb->dbb_attachments == attachment && !attachment->att_next &&
					!(dbb->dbb_flags & DBB_being_opened))
				{
					dbb->dbb_flags |= DBB_not_in_use;
				}

				try
				{
					// Purge attachment, don't rollback open transactions
					attachment->att_flags |= ATT_cancel_disable;
					purge_attachment(tdbb, attachment, false);
				}
				catch (const Exception&)
				{
					dbb->dbb_flags &= ~DBB_not_in_use;
					throw;
				}
			}
		}

		*handle = NULL;
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DROP_DATABASE(ISC_STATUS* user_status, Attachment** handle)
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
		ThreadContextHolder tdbb(user_status);

		MutexLockGuard guard(databases_mutex);

		Attachment* const attachment = *handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			Database* const dbb = tdbb->getDatabase();

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

			if (!CCH_exclusive(tdbb, LCK_PW, WAIT_PERIOD))
			{
				ERR_post(Arg::Gds(isc_lock_timeout) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(file_name));
			}

			// Check if same process has more attachments

			if (dbb->dbb_attachments && dbb->dbb_attachments->att_next)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
			}

			// Forced release of all transactions
			purge_transactions(tdbb, attachment, true, attachment->att_flags);

			attachment->att_flags |= ATT_cancel_disable;

			// Here we have database locked in exclusive mode.
			// Just mark the header page with an 0 ods version so that no other
			// process can attach to this database once we release our exclusive
			// lock and start dropping files.

   			WIN window(HEADER_PAGE_NUMBER);
			Ods::header_page* header = (Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
			CCH_MARK_MUST_WRITE(tdbb, &window);
			header->hdr_ods_version = 0;
			CCH_RELEASE(tdbb, &window);

			// This point on database is useless
			// mark the dbb unusable

			dbb->dbb_flags |= DBB_not_in_use;
			*handle = NULL;

			PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			const jrd_file* file = pageSpace->file;
			const Shadow* shadow = dbb->dbb_shadow;

			// Notify Trace API manager about successful drop of database
			if (attachment->att_trace_manager->needs().event_detach)
			{
				TraceConnectionImpl conn(attachment);
				attachment->att_trace_manager->event_detach(&conn, true);
			}

			// Unlink attachment from database
			release_attachment(tdbb, attachment);

			shutdown_database(dbb, false);

			// drop the files here
			bool err = drop_files(file);
			for (; shadow; shadow = shadow->sdw_next)
			{
				err = err || drop_files(shadow->sdw_file);
			}

			tdbb->setDatabase(NULL);
			Database::destroy(dbb);

			if (err) {
				ERR_build_status(user_status, Arg::Gds(isc_drdb_completed_with_errs));
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DROP_DATABASE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_GET_SEGMENT(ISC_STATUS* user_status,
							blb** blob_handle,
							USHORT* length,
							USHORT buffer_length,
							UCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			*length = BLB_get_segment(tdbb, blob, buffer, buffer_length);

		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_GET_SEGMENT));
		}

		// Don't trace errors below as it is not real errors but kind of return value

		if (blob->blb_flags & BLB_eof) {
			status_exception::raise(Arg::Gds(isc_segstr_eof));
		}
		else if (blob->blb_fragment_size) {
			status_exception::raise(Arg::Gds(isc_segment));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_GET_SLICE(ISC_STATUS* user_status,
						Attachment** db_handle,
						jrd_tra** tra_handle,
						ISC_QUAD* array_id,
						USHORT /*sdl_length*/,
						const UCHAR* sdl,
						USHORT param_length,
						const UCHAR* param,
						SLONG slice_length,
						UCHAR* slice,
						SLONG* return_length)
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
	try
	{
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *db_handle);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			jrd_tra* const transaction = find_transaction(tdbb, isc_segstr_wrong_db);

			if (!array_id->gds_quad_low && !array_id->gds_quad_high)
			{
				MOVE_CLEAR(slice, slice_length);
				*return_length = 0;
			}
			else
			{
				*return_length = BLB_get_slice(tdbb, transaction, reinterpret_cast<bid*>(array_id),
											   sdl, param_length, param, slice_length, slice);
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_GET_SLICE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_OPEN_BLOB2(ISC_STATUS* user_status,
						Attachment** db_handle,
						jrd_tra** tra_handle,
						blb** blob_handle,
						bid* blob_id,
						USHORT bpb_length,
						const UCHAR* bpb)
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
	try
	{
		if (*blob_handle)
		{
			status_exception::raise(Arg::Gds(isc_bad_segstr_handle));
		}

		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *db_handle);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			jrd_tra* const transaction = find_transaction(tdbb, isc_segstr_wrong_db);

			*blob_handle = BLB_open2(tdbb, transaction, blob_id, bpb_length, bpb, true);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_OPEN_BLOB2));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_PREPARE(ISC_STATUS* user_status, jrd_tra** tra_handle, USHORT length, const UCHAR* msg)
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
		ThreadContextHolder tdbb(user_status);

		jrd_tra* const transaction = *tra_handle;
		validateHandle(tdbb, transaction);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			prepare(tdbb, transaction, length, msg);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_PREPARE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_PUT_SEGMENT(ISC_STATUS* user_status,
							blb** blob_handle,
							USHORT buffer_length,
							const UCHAR* buffer)
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
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			BLB_put_segment(tdbb, blob, buffer, buffer_length);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_PUT_SEGMENT));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_PUT_SLICE(ISC_STATUS* user_status,
						Attachment** db_handle,
						jrd_tra** tra_handle,
						ISC_QUAD* array_id,
						USHORT /*sdl_length*/,
						const UCHAR* sdl,
						USHORT param_length,
						const UCHAR* param,
						SLONG slice_length,
						UCHAR* slice)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *db_handle);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			jrd_tra* const transaction = find_transaction(tdbb, isc_segstr_wrong_db);

			BLB_put_slice(tdbb, transaction, reinterpret_cast<bid*>(array_id),
						  sdl, param_length, param, slice_length, slice);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_PUT_SLICE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_QUE_EVENTS(ISC_STATUS* user_status,
							Attachment** handle,
							SLONG* id,
							SSHORT length,
							const UCHAR* items,
							FPTR_EVENT_CALLBACK ast,
							void* arg)
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
	try
	{
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			Database* const dbb = tdbb->getDatabase();
			Lock* const lock = dbb->dbb_lock;

			EventManager::init(attachment);

			*id = dbb->dbb_event_mgr->queEvents(attachment->att_event_session,
												lock->lck_length, (const TEXT*) &lock->lck_key,
												length, items,
												ast, arg);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_QUE_EVENTS));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_RECEIVE(ISC_STATUS* user_status,
						jrd_req** req_handle,
						USHORT msg_type,
						USHORT msg_length,
						SCHAR* msg,
						SSHORT level
#ifdef SCROLLABLE_CURSORS
						, USHORT direction,
						ULONG offset
#endif
	)
{
/**************************************
 *
 *	g d s _ $ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);
			check_transaction(tdbb, request->req_transaction);

			JRD_receive(tdbb, request, msg_type, msg_length, reinterpret_cast<UCHAR*>(msg), level
#ifdef SCROLLABLE_CURSORS
				, direction, offset
#endif
				);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_RECEIVE));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_RECONNECT(ISC_STATUS* user_status,
						Attachment** db_handle,
						jrd_tra** tra_handle,
						SSHORT length,
						const UCHAR* id)
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
	try
	{
		if (*tra_handle)
		{
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));
		}

		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			*tra_handle = TRA_reconnect(tdbb, id, length);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_RECONNECT));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_RELEASE_REQUEST(ISC_STATUS* user_status, jrd_req** req_handle)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			CMP_release(tdbb, request);
			*req_handle = NULL;
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_RELEASE_REQUEST));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_REQUEST_INFO(ISC_STATUS* user_status,
							jrd_req** req_handle,
							SSHORT level,
							SSHORT item_length,
							const SCHAR* items,
							SSHORT buffer_length,
							SCHAR* buffer)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			// I can't change the GDS_REQUEST_INFO's signature, so I do the casts here.
			const UCHAR* items2 = reinterpret_cast<const UCHAR*>(items);
			UCHAR* buffer2 = reinterpret_cast<UCHAR*>(buffer);
			SLONG buffer_length2 = (ULONG)(USHORT) buffer_length;
			JRD_request_info(tdbb, request, level, item_length, items2, buffer_length2, buffer2);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_REQUEST_INFO));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_ROLLBACK_RETAINING(ISC_STATUS* user_status, jrd_tra** tra_handle)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			JRD_rollback_retaining(tdbb, tra_handle);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_ROLLBACK_RETAINING));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_ROLLBACK(ISC_STATUS* user_status, jrd_tra** tra_handle)
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
		ThreadContextHolder tdbb(user_status);

		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			JRD_rollback_transaction(tdbb, tra_handle);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_ROLLBACK));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SEEK_BLOB(ISC_STATUS* user_status,
						blb** blob_handle,
						SSHORT mode,
						SLONG offset,
						SLONG* result)
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
	try
	{
		ThreadContextHolder tdbb(user_status);

		blb* const blob = *blob_handle;
		validateHandle(tdbb, blob);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			*result = BLB_lseek(blob, mode, offset);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_SEEK_BLOB));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SEND(ISC_STATUS* user_status,
					jrd_req** req_handle,
					USHORT msg_type,
					USHORT msg_length,
					SCHAR* msg,
					SSHORT level)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* request = *req_handle;
		validateHandle(tdbb, request);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);
			check_transaction(tdbb, request->req_transaction);

			verify_request_synchronization(request, level);

			EXE_send(tdbb, request, msg_type, msg_length, reinterpret_cast<UCHAR*>(msg));

			check_autocommit(request, tdbb);

			if (request->req_flags & req_warning)
			{
				request->req_flags &= ~req_warning;
				ERR_punt();
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_SEND));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SERVICE_ATTACH(ISC_STATUS* user_status,
							  const TEXT* service_name,
							  Service** svc_handle,
							  USHORT spb_length,
							  const SCHAR* spb)
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
	try
	{
		if (*svc_handle)
		{
			status_exception::raise(Arg::Gds(isc_bad_svc_handle));
		}

		ThreadContextHolder tdbb(user_status);

		*svc_handle = new Service(service_name, spb_length, reinterpret_cast<const UCHAR*>(spb));
	}
	catch (const DelayFailedLogin& ex)
	{
		ex.sleep();
		return ex.stuff_exception(user_status);
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SERVICE_DETACH(ISC_STATUS* user_status, Service** svc_handle)
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

		Service* const service = *svc_handle;
		validateHandle(service);

		service->detach();
		*svc_handle = NULL;
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SERVICE_QUERY(ISC_STATUS*	user_status,
							Service**	svc_handle,
							ULONG*	/*reserved*/,
							USHORT	send_item_length,
							const SCHAR*	send_items,
							USHORT	recv_item_length,
							const SCHAR*	recv_items,
							USHORT	buffer_length,
							SCHAR*	buffer)
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

		Service* const service = *svc_handle;
		validateHandle(service);

		const UCHAR* send_items2 = reinterpret_cast<const UCHAR*>(send_items);
		const UCHAR* recv_items2 = reinterpret_cast<const UCHAR*>(recv_items);
		UCHAR* buffer2 = reinterpret_cast<UCHAR*>(buffer);

		if (service->getVersion() == isc_spb_version1)
		{
			service->query(send_item_length, send_items2, recv_item_length,
					recv_items2, buffer_length, buffer2);
		}
		else
		{
			// For SVC_query2, we are going to completly dismantle user_status (since at this point it is
			// meaningless anyway).  The status vector returned by this function can hold information about
			// the call to query the service manager and/or a service thread that may have been running.

			service->query2(tdbb, send_item_length, send_items2,
					recv_item_length, recv_items2, buffer_length, buffer2);

			// If there is a status vector from a service thread, copy it into the thread status
			int len, warning;
			PARSE_STATUS(service->getStatus(), len, warning);
			if (len)
			{
				memcpy(user_status, service->getStatus(), sizeof(ISC_STATUS) * len);
				// Empty out the service status vector
				service->initStatus();
				return user_status[1];
			}
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_SERVICE_START(ISC_STATUS*	user_status,
							Service**	svc_handle,
							ULONG*	/*reserved*/,
							USHORT	spb_length,
							const SCHAR*	spb)
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

		Service* const service = *svc_handle;
		validateHandle(service);

		service->start(spb_length, reinterpret_cast<const UCHAR*>(spb));

		if (service->getStatus()[1])
		{
			memcpy(user_status, service->getStatus(), sizeof(ISC_STATUS_ARRAY));
			return user_status[1];
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_START_AND_SEND(ISC_STATUS* user_status,
							jrd_req** req_handle,
							jrd_tra** tra_handle,
							USHORT msg_type,
							USHORT msg_length,
							SCHAR* msg,
							SSHORT level)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);
			check_transaction(tdbb, request->req_transaction);

			jrd_tra* const transaction = find_transaction(tdbb, isc_req_wrong_db);

			TraceBlrExecute trace(tdbb, request);
			try
			{
				JRD_start_and_send(tdbb, request, transaction, msg_type,
									msg_length, reinterpret_cast<UCHAR*>(msg), level);

				// Notify Trace API about blr execution
				trace.finish(res_successful);
			}
			catch (const Exception& ex)
			{
				const ISC_LONG exc = ex.stuff_exception(user_status);
				const bool no_priv = (exc == isc_login || exc == isc_no_priv);
				trace.finish(no_priv ? res_unauthorized : res_failed);

				throw;
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_START_AND_SEND));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_START(ISC_STATUS* user_status, jrd_req** req_handle, jrd_tra** tra_handle, SSHORT level)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);
			check_transaction(tdbb, request->req_transaction);

			jrd_tra* const transaction = find_transaction(tdbb, isc_req_wrong_db);

			TraceBlrExecute trace(tdbb, request);
			try
			{
				JRD_start(tdbb, request, transaction, level);
				trace.finish(res_successful);
			}
			catch (const Exception& ex)
			{
				const ISC_LONG exc = stuff_exception(user_status, ex);
				const bool no_priv = (exc == isc_login || exc == isc_no_priv);
				trace.finish(no_priv ? res_unauthorized : res_failed);

				throw;
			}
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_START));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


int GDS_SHUTDOWN(unsigned int timeout)
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
		ThreadContextHolder tdbb;

		ULONG attach_count, database_count, svc_count;
		JRD_num_attachments(NULL, 0, JRD_info_none, &attach_count, &database_count, &svc_count);

		if (attach_count > 0 || svc_count > 0)
		{
			gds__log("Shutting down the server with %d active connection(s) to %d database(s), "
					 "%d active service(s)",
				attach_count, database_count, svc_count);
		}

#ifdef WIN_NT
		if (timeout && !bEmbedded)
#else
		if (timeout)
#endif
		{
			Semaphore shutdown_semaphore;

			ThreadStart::start(shutdown_thread, &shutdown_semaphore, THREAD_medium, 0);

			if (!shutdown_semaphore.tryEnter(0, timeout))
			{
				status_exception::raise(Arg::Gds(isc_shutdown_timeout));
			}
		}
		else
		{
			shutdown_thread(NULL);
		}
	}
	catch (const Exception& ex)
	{
	 	ISC_STATUS_ARRAY status;
		ex.stuff_exception(status);
		gds__log_status(NULL, status);
	}

	return 0;
}


ISC_STATUS GDS_START_MULTIPLE(ISC_STATUS* user_status, jrd_tra** tra_handle, USHORT count, TEB* vector)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ m u l t i p l e
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	try
	{
		ThreadContextHolder tdbb(user_status);

		JRD_start_multiple(tdbb, tra_handle, count, vector);
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_START_TRANSACTION(ISC_STATUS* user_status, jrd_tra** tra_handle, SSHORT count, ...)
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
	try
	{
		if (count < 1 || USHORT(count) > MAX_DB_PER_TRANS)
		{
			status_exception::raise(Arg::Gds(isc_max_db_per_trans_allowed) <<
									Arg::Num(MAX_DB_PER_TRANS));
		}

		HalfStaticArray<TEB, 16> tebs;
		tebs.grow(count);

		va_list ptr;
		va_start(ptr, count);

		for (TEB* teb_iter = tebs.begin(); teb_iter < tebs.end(); teb_iter++)
		{
			teb_iter->teb_database = va_arg(ptr, Attachment**);
			teb_iter->teb_tpb_length = va_arg(ptr, int);
			teb_iter->teb_tpb = va_arg(ptr, UCHAR*);
		}

		va_end(ptr);

		ThreadContextHolder tdbb(user_status);

		JRD_start_multiple(tdbb, tra_handle, count, tebs.begin());
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_TRANSACT_REQUEST(ISC_STATUS*	user_status,
								Attachment**		db_handle,
								jrd_tra**		tra_handle,
								USHORT	blr_length,
								const SCHAR*	blr,
								USHORT	in_msg_length,
								const SCHAR*	in_msg,
								USHORT	out_msg_length,
								SCHAR*	out_msg)
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
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		validateHandle(tdbb, *tra_handle);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			Database* const dbb = tdbb->getDatabase();

			jrd_tra* const transaction = find_transaction(tdbb, isc_req_wrong_db);

			jrd_nod* in_message = NULL;
			jrd_nod* out_message = NULL;

			jrd_req* request = NULL;
			MemoryPool* new_pool = dbb->createPool();

			try
			{
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				AutoPtr<CompilerScratch> csb;
				PAR_parse(tdbb, csb, reinterpret_cast<const UCHAR*>(blr), blr_length, false);

				request = CMP_make_request(tdbb, csb, false);
				CMP_verify_access(tdbb, request);

				jrd_nod* node;
				for (size_t i = 0; i < csb->csb_rpt.getCount(); i++)
				{
					if ( (node = csb->csb_rpt[i].csb_message) )
					{
						if ((int) (IPTR) node->nod_arg[e_msg_number] == 0)
						{
							in_message = node;
						}
						else if ((int) (IPTR) node->nod_arg[e_msg_number] == 1)
						{
							out_message = node;
						}
					}
				}
			}
			catch (const Exception&)
			{
				if (request)
					CMP_release(tdbb, request);
				else
					dbb->deletePool(new_pool);

				throw;
			}

			request->req_attachment = attachment;

			USHORT len;
			if (in_msg_length)
			{
				if (in_message)
				{
					const Format* format = (Format*) in_message->nod_arg[e_msg_format];
					len = format->fmt_length;
				}
				else {
					len = 0;
				}

				if (in_msg_length != len)
				{
					ERR_post(Arg::Gds(isc_port_len) << Arg::Num(in_msg_length) <<
													   Arg::Num(len));
				}

				memcpy((SCHAR*) request + in_message->nod_impure, in_msg, in_msg_length);
			}

			EXE_start(tdbb, request, transaction);

			if (out_message)
			{
				const Format* format = (Format*) out_message->nod_arg[e_msg_format];
				len = format->fmt_length;
			}
			else {
				len = 0;
			}

			if (out_msg_length != len)
			{
				ERR_post(Arg::Gds(isc_port_len) << Arg::Num(out_msg_length) <<
												   Arg::Num(len));
			}

			if (out_msg_length) {
				memcpy(out_msg, (SCHAR*) request + out_message->nod_impure, out_msg_length);
			}

			check_autocommit(request, tdbb);

			CMP_release(tdbb, request);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_TRANSACT_REQUEST));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_TRANSACTION_INFO(ISC_STATUS* user_status,
								jrd_tra** tra_handle,
								SSHORT item_length,
								const SCHAR* items,
								SSHORT buffer_length,
								SCHAR* buffer)
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
		ThreadContextHolder tdbb(user_status);

		jrd_tra* const transaction = *tra_handle;
		validateHandle(tdbb, transaction);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			const UCHAR* items2 = reinterpret_cast<const UCHAR*>(items);
			UCHAR* buffer2 = reinterpret_cast<UCHAR*>(buffer);
			INF_transaction_info(transaction, items2, item_length, buffer2, buffer_length);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_TRANSACTION_INFO));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_UNWIND(ISC_STATUS* user_status, jrd_req** req_handle, SSHORT level)
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
		ThreadContextHolder tdbb(user_status);

		jrd_req* const request = *req_handle;
		validateHandle(tdbb, request);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			JRD_unwind_request(tdbb, request, level);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_UNWIND));
		}
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_ALLOCATE(ISC_STATUS* user_status, Attachment** db_handle, dsql_req** stmt_handle)
{
	try
	{
		if (*stmt_handle)
		{
			status_exception::raise(Arg::Gds(isc_bad_req_handle));
		}

		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			*stmt_handle = DSQL_allocate_statement(tdbb, attachment);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_ALLOCATE));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_ALLOCATE));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_EXECUTE(ISC_STATUS* user_status,
							jrd_tra** tra_handle,
							dsql_req** stmt_handle,
							USHORT in_blr_length, const SCHAR* in_blr,
							USHORT in_msg_type, USHORT in_msg_length, const SCHAR* in_msg,
							USHORT out_blr_length, SCHAR* out_blr,
							USHORT /*out_msg_type*/, USHORT out_msg_length, SCHAR* out_msg)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		if (*tra_handle)
		{
			validateHandle(tdbb, *tra_handle);
		}
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_execute(tdbb, tra_handle, statement,
						 in_blr_length, reinterpret_cast<const UCHAR*>(in_blr),
						 in_msg_type, in_msg_length, reinterpret_cast<const UCHAR*>(in_msg),
						 out_blr_length, reinterpret_cast<UCHAR*>(out_blr),
						 /*out_msg_type,*/ out_msg_length, reinterpret_cast<UCHAR*>(out_msg));
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_EXECUTE));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_EXECUTE));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_EXECUTE_IMMEDIATE(ISC_STATUS* user_status,
									  Attachment** db_handle,
									  jrd_tra** tra_handle,
									  USHORT length, const TEXT* string, USHORT dialect,
									  USHORT in_blr_length, const SCHAR* in_blr,
									  USHORT /*in_msg_type*/, USHORT in_msg_length, const SCHAR* in_msg,
									  USHORT out_blr_length, SCHAR* out_blr,
									  USHORT /*out_msg_type*/, USHORT out_msg_length, SCHAR* out_msg)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		if (*tra_handle)
		{
			validateHandle(tdbb, *tra_handle);
		}
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_execute_immediate(tdbb, attachment, tra_handle,
								   length, string, dialect,
								   in_blr_length, reinterpret_cast<const UCHAR*>(in_blr),
								   /*in_msg_type,*/ in_msg_length, reinterpret_cast<const UCHAR*>(in_msg),
								   out_blr_length, reinterpret_cast<UCHAR*>(out_blr),
								   /*out_msg_type,*/ out_msg_length, reinterpret_cast<UCHAR*>(out_msg));
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_EXECUTE_IMMEDIATE));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_EXECUTE_IMMEDIATE));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_FETCH(ISC_STATUS* user_status,
						  dsql_req** stmt_handle,
						  USHORT blr_length, const SCHAR* blr,
						  USHORT /*msg_type*/, USHORT msg_length, SCHAR* dsql_msg_buf
#ifdef SCROLLABLE_CURSORS
						  , USHORT direction, SLONG offset
#endif
						  )
{
	ISC_STATUS return_code = FB_SUCCESS;

	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		validateHandle(tdbb, statement->req_transaction);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			return_code = DSQL_fetch(tdbb, statement, blr_length, reinterpret_cast<const UCHAR*>(blr),
							/*msg_type,*/ msg_length, reinterpret_cast<UCHAR*>(dsql_msg_buf)
#ifdef SCROLLABLE_CURSORS
							  , direction, offset
#endif
							  );
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_FETCH));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_FETCH));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status, return_code);
}


ISC_STATUS GDS_DSQL_FREE(ISC_STATUS* user_status, dsql_req** stmt_handle, USHORT option)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_free_statement(tdbb, statement, option);

			if (option & DSQL_drop)
				*stmt_handle = NULL;
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_FREE));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_FREE));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_INSERT(ISC_STATUS* user_status,
						   dsql_req** stmt_handle,
						   USHORT blr_length, const SCHAR* blr,
						   USHORT /*msg_type*/, USHORT msg_length, const SCHAR*	dsql_msg_buf)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_insert(tdbb, statement, blr_length, reinterpret_cast<const UCHAR*>(blr),
						/*msg_type,*/ msg_length, reinterpret_cast<const UCHAR*>(dsql_msg_buf));
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_INSERT));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_INSERT));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_PREPARE(ISC_STATUS* user_status,
							jrd_tra** tra_handle,
							dsql_req** stmt_handle,
							USHORT length, const TEXT* string, USHORT dialect,
							USHORT item_length, const SCHAR* items,
							USHORT buffer_length, SCHAR* buffer)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		if (*tra_handle)
		{
			validateHandle(tdbb, *tra_handle);
		}
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_prepare(tdbb, *tra_handle, stmt_handle, length, string, dialect,
						 item_length, reinterpret_cast<const UCHAR*>(items),
						 buffer_length, reinterpret_cast<UCHAR*>(buffer));
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_PREPARE));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_PREPARE));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_SET_CURSOR(ISC_STATUS* user_status,
							   dsql_req** stmt_handle,
							   const TEXT* cursor,
							   USHORT /*type*/)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_set_cursor(tdbb, statement, cursor); //, type);
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_SET_CURSOR));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_SET_CURSOR));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


ISC_STATUS GDS_DSQL_SQL_INFO(ISC_STATUS* user_status,
							 dsql_req** stmt_handle,
							 USHORT item_length, const SCHAR* items,
							 USHORT info_length, SCHAR* info)
{
	try
	{
		ThreadContextHolder tdbb(user_status);

		dsql_req* const statement = *stmt_handle;
		validateHandle(tdbb, statement);
		DatabaseContextHolder dbbHolder(tdbb);
		try
		{
			check_database(tdbb);

			DSQL_sql_info(tdbb, statement,
						  item_length, reinterpret_cast<const UCHAR*>(items),
						  info_length, reinterpret_cast<UCHAR*>(info));
		}
		catch (const Exception& ex)
		{
			return trace_error(tdbb, ex, user_status, ENTRYPOINT_NAME(GDS_DSQL_SQL_INFO));
		}
		trace_warning(tdbb, user_status, ENTRYPOINT_NAME(GDS_DSQL_SQL_INFO));
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}


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

	Firebird::string fname = fb_utils::getPrefix(fb_utils::FB_DIR_LOG, "proc_info.log");
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
							procedure->prc_name->hasData() ? procedure->prc_name->c_str() : "NULL",
							procedure->prc_id, procedure->prc_flags, procedure->prc_use_count,
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
	if (tdbb->tdbb_latch_count && !(tdbb->tdbb_flags & TDBB_no_cache_unwind))
	{
		if (tdbb->tdbb_quantum < 0)
			tdbb->tdbb_quantum = 0;
		return false;
	}
	
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_sync->hasContention())
	{
		Database::Checkout dcoHolder(dbb);
		THREAD_YIELD();
	}

	try {
		tdbb->checkCancelState(punt);
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

	// Enable signal handler for the monitoring stuff

	if (dbb->dbb_ast_flags & DBB_monitor_off)
	{
		dbb->dbb_ast_flags &= ~DBB_monitor_off;
		dbb->dbb_flags |= DBB_monitor_locking;
		LCK_lock(tdbb, dbb->dbb_monitor_lock, LCK_SR, LCK_WAIT);
		dbb->dbb_flags &= ~DBB_monitor_locking;

		// While waiting for return from LCK_lock call above the blocking AST (see 
		// DatabaseSnapshot::blockingAst) was called and set DBB_monitor_off flag 
		// again. But it not released lock as lck_id was unknown at that moment. 
		// Do it now to not block another process waiting for a monitoring lock.

		if (dbb->dbb_ast_flags & DBB_monitor_off)
			LCK_release(tdbb, dbb->dbb_monitor_lock);
	}

	tdbb->tdbb_quantum = (tdbb->tdbb_quantum <= 0) ?
#ifdef SUPERSERVER
		(quantum ? quantum : (ThreadPriorityScheduler::boosted() ?
			Config::getPriorityBoost() : 1) * QUANTUM) :
#else
		(quantum ? quantum : QUANTUM) :
#endif
		tdbb->tdbb_quantum;

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


static void check_database(thread_db* tdbb)
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

	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

	const Attachment* attach = dbb->dbb_attachments;
	while (attach && attach != attachment)
		attach = attach->att_next;

	if (!attach)
		status_exception::raise(Arg::Gds(isc_bad_db_handle));

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

	if ((attachment->att_flags & ATT_cancel_raise) && !(attachment->att_flags & ATT_cancel_disable))
	{
		attachment->att_flags &= ~ATT_cancel_raise;
		status_exception::raise(Arg::Gds(isc_cancelled));
	}

	// Enable signal handler for the monitoring stuff.
	// See also comments in JRD_reshedule.

	if (dbb->dbb_ast_flags & DBB_monitor_off)
	{
		dbb->dbb_ast_flags &= ~DBB_monitor_off;
		dbb->dbb_flags |= DBB_monitor_locking;
		LCK_lock(tdbb, dbb->dbb_monitor_lock, LCK_SR, LCK_WAIT);
		dbb->dbb_flags &= ~DBB_monitor_locking;

		if (dbb->dbb_ast_flags & DBB_monitor_off)
			LCK_release(tdbb, dbb->dbb_monitor_lock);
	}
}


static void check_transaction(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	c h e c k _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Check transaction for not being interrupted
 *  in the meantime.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (transaction && (transaction->tra_flags & TRA_cancel_request))
	{
		transaction->tra_flags &= ~TRA_cancel_request;
		status_exception::raise(Arg::Gds(isc_cancelled));
	}
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

	if (transaction->tra_sibling && !(transaction->tra_flags & TRA_prepared))
	{
		prepare(tdbb, transaction, 0, NULL);
	}

	const Attachment* const attachment = tdbb->getAttachment();

	if (!(attachment->att_flags & ATT_no_db_triggers) && !(transaction->tra_flags & TRA_prepared))
	{
		// run ON TRANSACTION COMMIT triggers
		run_commit_triggers(tdbb, transaction);
	}

	jrd_tra* next = transaction;

	while ( (transaction = next) )
	{
		next = transaction->tra_sibling;
		validateHandle(tdbb, transaction->tra_attachment);
		tdbb->setTransaction(transaction);
		check_database(tdbb);
		TRA_commit(tdbb, transaction, retaining_flag);
	}
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


static jrd_tra* find_transaction(thread_db* tdbb, ISC_STATUS error_code)
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

	const Attachment* const attachment = tdbb->getAttachment();

	for (jrd_tra* transaction = tdbb->getTransaction(); transaction;
		transaction = transaction->tra_sibling)
	{
		if (transaction->tra_attachment == attachment)
		{
			return transaction;
		}
	}

	status_exception::raise(Arg::Gds(error_code));
	return NULL;	// Added to remove compiler warnings
}


static void find_intl_charset(thread_db* tdbb, Attachment* attachment, const DatabaseOptions* options)
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
		attachment->att_charset = DEFAULT_ATTACHMENT_CHARSET;
		return;
	}

	USHORT id;

	const UCHAR* lc_ctype = reinterpret_cast<const UCHAR*>(options->dpb_lc_ctype.c_str());

	if (MET_get_char_coll_subtype(tdbb, &id, lc_ctype, options->dpb_lc_ctype.length()) &&
		INTL_defined_type(tdbb, id & 0xFF) && ((id & 0xFF) != CS_BINARY))
	{
		attachment->att_charset = id & 0xFF;
	}
	else
	{
		// Report an error - we can't do what user has requested
		ERR_post(Arg::Gds(isc_bad_dpb_content) <<
				 Arg::Gds(isc_charset_not_found) << Arg::Str(options->dpb_lc_ctype));
	}
}


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

	ULONG page_cache_size = Config::getDefaultDbCachePages();
	if (page_cache_size < MIN_PAGE_BUFFERS)
		page_cache_size = MIN_PAGE_BUFFERS;
	if (page_cache_size > MAX_PAGE_BUFFERS)
		page_cache_size = MAX_PAGE_BUFFERS;

	dpb_buffers = page_cache_size;
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

	ClumpletReader rdr(ClumpletReader::Tagged, dpb, dpb_length);

	if (rdr.getBufferTag() != isc_dpb_version1)
	{
		ERR_post(Arg::Gds(isc_bad_dpb_form) <<
				 Arg::Gds(isc_wrodpbver));
	}

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
				ERR_post(Arg::Gds(isc_bad_dpb_content));
			}
			dpb_set_page_buffers = true;
			break;

#ifndef SUPERSERVER
		case isc_dpb_num_buffers:
			dpb_buffers = rdr.getInt();
			if (dpb_buffers < 10)
			{
				ERR_post(Arg::Gds(isc_bad_dpb_content));
			}
			break;
#endif

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

		case isc_dpb_sys_user_name:
			getString(rdr, dpb_sys_user_name);
			break;

		case isc_dpb_sql_role_name:
			if (! dpb_trusted_role)
			{
			    getString(rdr, dpb_role_name);
			}
			break;

		case isc_dpb_user_name:
			getString(rdr, dpb_user_name);
			break;

		case isc_dpb_password:
			getString(rdr, dpb_password);
			break;

		case isc_dpb_password_enc:
			rdr.getString(dpb_password_enc);
			break;

		case isc_dpb_trusted_auth:
			getString(rdr, dpb_trusted_login);
			break;

		case isc_dpb_trusted_role:
			dpb_trusted_role = true;
			getString(rdr, dpb_role_name);
			break;

		case isc_dpb_encrypt_key:
#ifdef ISC_DATABASE_ENCRYPTION
			rdr.getString(dpb_key);
#else
			// Just in case there WAS a customer using this unsupported
			// feature - post an error when they try to access it in 4.0
			ERR_post(Arg::Gds(isc_uns_ext) <<
					 Arg::Gds(isc_random) << Arg::Str("Encryption not supported"));
#endif
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

		case isc_dpb_sec_attach:
			dpb_sec_attach = rdr.getInt() != 0;
			dpb_buffers = 50;
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

		case isc_dpb_gsec_attach:
			dpb_gsec_attach = rdr.getBoolean();
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

		default:
			break;
		}
	}

	if (! rdr.isEof())
		ERR_post(Arg::Gds(isc_bad_dpb_form));
}


static ISC_STATUS handle_error(ISC_STATUS* user_status, ISC_STATUS code)
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
		ERR_build_status(user_status, Arg::Gds(code));
	}

	return code;
}


static Database* init(thread_db* tdbb,
					  const PathName& expanded_filename, // only for SS
					  bool attach_flag) // only for SS
{
/**************************************
 *
 *	i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize for database access.  First call from both CREATE and
 *	OPEN.
 *	Upon entry mutex databases_mutex must be locked.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Initialize standard random generator.
	// MSVC (at least since version 7) have per-thread random seed.
	// As we don't know who uses per-thread seed, this should work for both cases.
	static bool first_rand = true;
	static int first_rand_value = rand();

	if (first_rand || (rand() == first_rand_value))
		srand(time(NULL));

	first_rand = false;

	engineStartup.init();

	Database* dbb = NULL;

	// Check to see if the database is already actively attached

#ifdef SUPERSERVER
	for (dbb = databases; dbb; dbb = dbb->dbb_next)
	{
		if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use)) &&
			(dbb->dbb_filename == expanded_filename))
		{
			if (attach_flag)
				return dbb;

			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
		}
	}
#endif

	dbb = Database::create();
	tdbb->setDatabase(dbb);

	dbb->dbb_bufferpool = dbb->createPool();

	// provide context pool for the rest stuff
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);

	dbb->dbb_next = databases;
	databases = dbb;

	dbb->dbb_flags |= DBB_exclusive;
	dbb->dbb_sweep_interval = SWEEP_INTERVAL;

	dbb->dbb_monitoring_id = fb_utils::genUniqueId();

	// set a garbage collection policy

	if ((dbb->dbb_flags & (DBB_gc_cooperative | DBB_gc_background)) == 0)
	{
		string gc_policy = Config::getGCPolicy();
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
		else // config value is invalid, use default
		{
			if (GCPolicyDefault == GCPolicyCooperative) {
				dbb->dbb_flags |= DBB_gc_cooperative;
			}
			else if (GCPolicyDefault == GCPolicyBackground) {
				dbb->dbb_flags |= DBB_gc_background;
			}
			else if (GCPolicyDefault == GCPolicyCombined) {
				dbb->dbb_flags |= DBB_gc_cooperative | DBB_gc_background;
			}
			else
				fb_assert(false);
		}
	}

	// Initialize a number of subsystems

	TRA_init(dbb);

#ifdef ISC_DATABASE_ENCRYPTION
	// Lookup some external "hooks"

	PluginManager::Plugin crypt_lib = PluginManager::enginePluginManager().findPlugin(CRYPT_IMAGE);
	if (crypt_lib)
	{
		string encrypt_entrypoint(ENCRYPT);
		string decrypt_entrypoint(DECRYPT);
		dbb->dbb_encrypt = (Database::crypt_routine) crypt_lib.lookupSymbol(encrypt_entrypoint);
		dbb->dbb_decrypt = (Database::crypt_routine) crypt_lib.lookupSymbol(decrypt_entrypoint);
	}
#endif

	return dbb;
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
 *	Initialize the database lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	// Main database lock

	PageSpace* const pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	fb_assert(pageSpace && pageSpace->file);

	UCharBuffer file_id;
	PIO_get_unique_file_id(pageSpace->file, file_id);
	size_t key_length = file_id.getCount();

	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, key_length) Lock;
	dbb->dbb_lock = lock;
	lock->lck_type = LCK_database;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_object = dbb;
	lock->lck_length = key_length;
	lock->lck_dbb = dbb;
	lock->lck_ast = CCH_down_grade_dbb;
	memcpy(lock->lck_key.lck_string, file_id.begin(), key_length);

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
				ERR_post(Arg::Gds(isc_shutdown) << Arg::Str(pageSpace->file->fil_string));
			}
		}
	}
}

static void init_monitoring_lock(thread_db* tdbb)
{
/**************************************
 *
 *	i n i t _ m o n i t o r i n g _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Initialize the monitoring lock.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	// Lock shared by all dbb owners, used to signal other processes
	// to dump their monitoring data and synchronize operations

	Lock* const lock = FB_NEW_RPT(*dbb->dbb_permanent, sizeof(SLONG)) Lock();
	dbb->dbb_monitor_lock = lock;
	lock->lck_type = LCK_monitor;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_parent = dbb->dbb_lock;
	lock->lck_length = sizeof(SLONG);
	lock->lck_dbb = dbb;
	lock->lck_object = dbb;
	lock->lck_ast = DatabaseSnapshot::blockingAst;
	LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);
}


static void prepare(thread_db* tdbb, jrd_tra* transaction, USHORT length, const UCHAR* msg)
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

	if (!(transaction->tra_flags & TRA_prepared))
	{
		// run ON TRANSACTION COMMIT triggers
		run_commit_triggers(tdbb, transaction);
	}

	for (; transaction; transaction = transaction->tra_sibling)
	{
		validateHandle(tdbb, transaction->tra_attachment);
		tdbb->setTransaction(transaction);
		check_database(tdbb);
		TRA_prepare(tdbb, transaction, length, msg);
	}
}


static void release_attachment(thread_db* tdbb, Attachment* attachment)
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

	if (!attachment)
		return;

#ifdef SUPERSERVER
	if (dbb->dbb_relations)
	{
		vec<jrd_rel*>& rels = *dbb->dbb_relations;
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
#endif

	if (dbb->dbb_event_mgr && attachment->att_event_session)
	{
		dbb->dbb_event_mgr->deleteSession(attachment->att_event_session);
	}

	if (attachment->att_id_lock)
		LCK_release(tdbb, attachment->att_id_lock);

#ifndef SUPERSERVER
	if (attachment->att_temp_pg_lock)
		LCK_release(tdbb, attachment->att_temp_pg_lock);

	DSqlCache::Accessor accessor(&attachment->att_dsql_cache);
	for (bool getResult = accessor.getFirst(); getResult; getResult = accessor.getNext())
	{
		LCK_release(tdbb, accessor.current()->second.lock);
	}
#endif

	for (vcl** vector = attachment->att_counts; vector < attachment->att_counts + DBB_max_count;
		++vector)
	{
		delete *vector;
		*vector = NULL;
	}

	// Release any validation error vector allocated

	delete attachment->att_val_errors;
	attachment->att_val_errors = NULL;

	detachLocksFromAttachment(attachment);

	if (attachment->att_flags & ATT_lck_init_done)
	{
		LCK_fini(tdbb, LCK_OWNER_attachment);
		attachment->att_flags &= ~ATT_lck_init_done;
	}

	delete attachment->att_compatibility_table;

	if (attachment->att_dsql_instance)
	{
		MemoryPool* const pool = &attachment->att_dsql_instance->dbb_pool;
		delete attachment->att_dsql_instance;
		dbb->deletePool(pool);
	}

	// remove the attachment block from the dbb linked list

	for (Attachment** ptr = &dbb->dbb_attachments; *ptr; ptr = &(*ptr)->att_next)
	{
		if (*ptr == attachment)
		{
			*ptr = attachment->att_next;
			break;
		}
	}

    // CMP_release() advances the pointer before the deallocation.
	jrd_req* request;
	while ( (request = attachment->att_requests) ) {
		CMP_release(tdbb, request);
	}

	SCL_release_all(attachment->att_security_classes);

	delete attachment->att_user;

	Attachment::destroy(attachment);	// string were re-saved in the beginning of this function,
										// keep that in sync please
	tdbb->setAttachment(NULL);
}


static void detachLocksFromAttachment(Attachment* attachment)
{
/**************************************
 *
 *	d e t a c h L o c k s F r o m A t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 * Bug #7781, need to null out the attachment pointer of all locks which
 * were hung off this attachment block, to ensure that the attachment
 * block doesn't get dereferenced after it is released
 *
 **************************************/
	Lock* long_lock = attachment->att_long_locks;
	while (long_lock)
	{
		Lock* next = long_lock->lck_next;
		long_lock->lck_attachment = NULL;
		long_lock->lck_next = NULL;
		long_lock->lck_prior = NULL;
		long_lock = next;
	}
	attachment->att_long_locks = NULL;
}


bool Attachment::backupStateWriteLock(thread_db* tdbb, SSHORT wait)
{
	if (att_backup_state_counter++)
		return true;

	if (att_database->dbb_backup_manager->lockStateWrite(tdbb, wait))
		return true;

	att_backup_state_counter--;
	return false;
}

void Attachment::backupStateWriteUnLock(thread_db* tdbb)
{
	if (--att_backup_state_counter == 0)
		att_database->dbb_backup_manager->unlockStateWrite(tdbb);
}

bool Attachment::backupStateReadLock(thread_db* tdbb, SSHORT wait)
{
	if (att_backup_state_counter++)
		return true;

	if (att_database->dbb_backup_manager->lockStateRead(tdbb, wait))
		return true;

	att_backup_state_counter--;
	return false;
}

void Attachment::backupStateReadUnLock(thread_db* tdbb)
{
	if (--att_backup_state_counter == 0)
		att_database->dbb_backup_manager->unlockStateRead(tdbb);
}

Attachment::Attachment(MemoryPool* pool, Database* dbb)
:	att_pool(pool),
	att_memory_stats(&dbb->dbb_memory_stats),
	att_database(dbb),
	att_lock_owner_id(Database::getLockOwnerId()),
	att_backup_state_counter(0),
	att_stats(*pool),
	att_working_directory(*pool),
	att_filename(*pool),
	att_timestamp(TimeStamp::getCurrentTimeStamp()),
	att_context_vars(*pool),
	att_network_protocol(*pool),
	att_remote_address(*pool),
	att_remote_process(*pool),
	att_dsql_cache(*pool),
	att_udf_pointers(*pool),
	att_ext_connection(NULL),
	att_ext_call_depth(0),
	att_trace_manager(FB_NEW(*att_pool) TraceManager(this)),
	att_requested_role(*pool)
{
	att_mutex.enter();
}


Attachment::~Attachment()
{
	delete att_trace_manager;

	if (att_fini_sec_db)
	{
		SecurityDatabase::shutdown();
	}

	// For normal attachments that happens in release_attachment(),
	// but for special ones like GC should be done also in dtor -
	// they do not (and should not) call release_attachment().
	// It's no danger calling detachLocksFromAttachment()
	// once more here because it nulls att_long_locks.
	//		AP 2007
	detachLocksFromAttachment(this);
	att_mutex.leave();
}


PreparedStatement* Attachment::prepareStatement(thread_db* tdbb, MemoryPool& pool,
	jrd_tra* transaction, const string& text)
{
	return FB_NEW(pool) PreparedStatement(tdbb, pool, this, transaction, text);
}

void Attachment::cancelExternalConnection(thread_db* tdbb)
{
	if (att_ext_connection) {
		att_ext_connection->cancelExecution(tdbb);
	}
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
	ISC_STATUS_ARRAY user_status = {0};
	ISC_STATUS_ARRAY local_status = {0};
	ISC_STATUS* const orig_status = tdbb->tdbb_status_vector;

	try
	{
		jrd_tra* next = transaction;

		while ( (transaction = next) )
		{
			next = transaction->tra_sibling;

			try
			{
				validateHandle(tdbb, transaction->tra_attachment);
				check_database(tdbb);

				const Database* const dbb = tdbb->getDatabase();
				const Attachment* const attachment = tdbb->getAttachment();

				if (!(attachment->att_flags & ATT_no_db_triggers))
				{
					try
					{
						ISC_STATUS_ARRAY temp_status = {0};
						tdbb->tdbb_status_vector = temp_status;

						// run ON TRANSACTION ROLLBACK triggers
						EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_trans_rollback);
					}
					catch (const Exception& ex)
					{
						if (dbb->dbb_flags & DBB_bugcheck)
							throw;

						ISC_STATUS_ARRAY temp = {0};
						trace_error(tdbb, ex, temp, "TRANSACTION_ROLLBACK_TRIGGER");
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
	}
	catch (const Exception& ex)
	{
		ex.stuff_exception(user_status);
	}

	tdbb->tdbb_status_vector = orig_status;

	if (user_status[1] != FB_SUCCESS)
		status_exception::raise(user_status);
}


static void shutdown_database(Database* dbb, const bool release_pools)
{
/**************************************
 *
 *	s h u t d o w n _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Shutdown physical database environment.
 *	NOTE:  This routine assumes that upon entry,
 *	mutex databases_mutex will be locked.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	// Shutdown file and/or remote connection

#ifdef SUPERSERVER_V2
	TRA_header_write(tdbb, dbb, 0L);	// Update transaction info on header page.
#endif

	MET_clear_cache(tdbb);

#ifdef GARBAGE_THREAD
	VIO_fini(tdbb);
#endif
	CMP_fini(tdbb);
	CCH_fini(tdbb);

	if (dbb->dbb_backup_manager)
		dbb->dbb_backup_manager->shutdown(tdbb);

	if (dbb->dbb_monitor_lock)
		LCK_release(tdbb, dbb->dbb_monitor_lock);

	if (dbb->dbb_shadow_lock)
		LCK_release(tdbb, dbb->dbb_shadow_lock);

	if (dbb->dbb_retaining_lock)
		LCK_release(tdbb, dbb->dbb_retaining_lock);

	dbb->dbb_shared_counter.shutdown(tdbb);
	dbb->destroyIntlObjects();

	// Shut down any extern relations

	if (dbb->dbb_relations)
	{
		vec<jrd_rel*>* vector = dbb->dbb_relations;
		vec<jrd_rel*>::iterator ptr = vector->begin(), end = vector->end();

		while (ptr < end)
		{
			jrd_rel* relation = *ptr++;
			if (relation)
			{
				if (relation->rel_file)
				{
					EXT_fini(relation, false);
				}

				for (IndexBlock* index_block = relation->rel_index_blocks; index_block;
					index_block = index_block->idb_next)
				{
					if (index_block->idb_lock)
						LCK_release(tdbb, index_block->idb_lock);
				}

				delete relation;
			}
		}
	}

	if (dbb->dbb_lock)
		LCK_release(tdbb, dbb->dbb_lock);

	Database** d_ptr;	// Intentionally left outside loop (HP/UX compiler)
	for (d_ptr = &databases; *d_ptr; d_ptr = &(*d_ptr)->dbb_next)
	{
		if (*d_ptr == dbb)
		{
			*d_ptr = dbb->dbb_next;
			break;
		}
	}

	if (dbb->dbb_flags & DBB_lck_init_done)
	{
		dbb->dbb_page_manager.releaseLocks();

		LCK_fini(tdbb, LCK_OWNER_database);
		dbb->dbb_flags &= ~DBB_lck_init_done;
	}

	if (release_pools)
	{
		tdbb->setDatabase(NULL);
		Database::destroy(dbb);
	}
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


static bool shutdown_dbb(thread_db* tdbb, Database* dbb)
{
/**************************************
 *
 *	s h u t d o w n _ d b b
 *
 **************************************
 *
 * Functional description
 *	rollback every transaction,
 *	release every attachment,
 *	and shutdown database.
 *
 **************************************/
	tdbb->setDatabase(dbb);
	tdbb->tdbb_flags |= TDBB_shutdown_manager;
	DatabaseContextHolder dbbHolder(tdbb);

	if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use | DBB_security_db)) &&
		!((dbb->dbb_ast_flags & DBB_shutdown) && (dbb->dbb_ast_flags & DBB_shutdown_locks)))
	{
		Attachment* att_next;

		for (Attachment* attach = dbb->dbb_attachments; attach; attach = att_next)
		{
			att_next = attach->att_next;
			tdbb->setAttachment(attach);

			// purge_attachment() below can do an ERR_post
#pragma FB_COMPILER_MESSAGE("Please review the usage of temp_status v/s purge_attachment")
			// CVC: I see a problem here because purge_attachment doesn't use the passed vector.
			ISC_STATUS_ARRAY temp_status;
			fb_utils::init_status(temp_status);
			tdbb->tdbb_status_vector = temp_status;

			try
			{
				// purge attachment, rollback any open transactions
				purge_attachment(tdbb, attach, true);
			}
			catch (const Exception& ex)
			{
				iscLogException("error while shutting down attachment", ex);
				return false;
			}
		}
	}

	return true;
}


UCHAR* JRD_num_attachments(UCHAR* const buf, USHORT buf_len, JRD_info_tag flag,
						  ULONG* atts, ULONG* dbs, ULONG* svcs)
{
/**************************************
 *
 *	J R D _ n u m _ a t t a c h m e n t s
 *
 **************************************
 *
 * Functional description
 *	Count the number of active databases and
 *	attachments.  If flag is set then put
 *	what it says into buf, if it fits. If it does not fit
 *	then allocate local buffer, put info into there, and
 *	return pointer to caller (in this case a caller must
 *	release memory allocated for local buffer).
 *
 **************************************/

	// protect against NULL value for buf

	UCHAR* lbuf = buf;
	if (!lbuf)
		buf_len = 0;

#ifdef WIN_NT
	// Check that the buffer is big enough for the requested
	// information.  If not, unset the flag

	if (flag == JRD_info_drivemask)
	{
		if (buf_len < sizeof(ULONG))
		{
		    lbuf = (UCHAR*) gds__alloc((SLONG) (sizeof(ULONG)));
			if (!lbuf)
				flag = JRD_info_none;
		}
	}
#endif

	ULONG num_att = 0;
	ULONG drive_mask = 0L;
	ULONG total = 0;
	SortedObjectsArray<PathName> dbFiles(*getDefaultMemoryPool());

	try
	{
		MutexLockGuard guard(databases_mutex);

		// Zip through the list of databases and count the number of local
		// connections.  If buf is not NULL then copy all the database names
		// that will fit into it.

		for (Database* dbb = databases; dbb; dbb = dbb->dbb_next)
		{
			Database::SyncGuard dsGuard(dbb);

#ifdef WIN_NT
			// Get drive letters for db files

			if (flag == JRD_info_drivemask)
			{
				const PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
				for (const jrd_file* files = pageSpace->file; files; files = files->fil_next)
					ExtractDriveLetter(files->fil_string, &drive_mask);
			}
#endif

			if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use | DBB_security_db)) &&
				!((dbb->dbb_ast_flags & DBB_shutdown) && (dbb->dbb_ast_flags & DBB_shutdown_locks)))
			{
				if (!dbFiles.exist(dbb->dbb_filename))
					dbFiles.add(dbb->dbb_filename);
				total += sizeof(USHORT) + dbb->dbb_filename.length();

				for (const Attachment* attach = dbb->dbb_attachments; attach; attach = attach->att_next)
				{
					num_att++;

#ifdef WIN_NT
					// Get drive letters for temp directories

					if (flag == JRD_info_drivemask)
					{
						const TempDirectoryList dirList;
						for (size_t i = 0; i < dirList.getCount(); i++)
						{
							const PathName& path = dirList[i];
							ExtractDriveLetter(path.c_str(), &drive_mask);
						}
					}
#endif
				}
			}
		}
	}
	catch (const Exception&)
	{
		// Here we ignore possible errors from databases_mutex.
		// They were always silently ignored, and for this function
		// we really have no way to notify world about mutex problem.
		//		AP. 2008.
	}

	const ULONG num_dbs = dbFiles.getCount();

	*atts = num_att;
	*dbs = num_dbs;

	if (num_dbs > 0)
	{
		if (flag == JRD_info_dbnames)
		{
			if (buf_len < (sizeof(USHORT) + total))
			{
				lbuf = (UCHAR*) gds__alloc(sizeof(USHORT) + total);
			}
			UCHAR* lbufp = lbuf;
			if (lbufp)
			{
				/*  Put db info into buffer. Format is as follows:

				   number of dbases sizeof (USHORT)
				   1st db name length   sizeof (USHORT)
				   1st db name      sizeof (TEXT) * length
				   2nd db name length
				   2nd db name
				   ...
				   last db name length
				   last db name
				 */

				 fb_assert(num_dbs < MAX_USHORT);
				*lbufp++ = (UCHAR) num_dbs;
				*lbufp++ = (UCHAR) (num_dbs >> 8);

				for (size_t n = 0; n < num_dbs; ++n)
				{
					const USHORT dblen = dbFiles[n].length();
					*lbufp++ = (UCHAR) dblen;
					*lbufp++ = (UCHAR) (dblen >> 8);
					memcpy(lbufp, dbFiles[n].c_str(), dblen);
					lbufp += dblen;
				}
			}
		}
	}

#ifdef WIN_NT
	if (flag == JRD_info_drivemask)
		*(ULONG*) lbuf = drive_mask;
#endif

	// CVC: Apparently, the original condition will leak memory, because flag
	// may be JRD_info_drivemask and memory could be allocated for that purpose,
	// as few as sizeof(ULONG), but a leak is a leak! I added the ifdef below.
	if (num_dbs == 0)
	{
#ifdef WIN_NT
		if (flag == JRD_info_drivemask && lbuf != buf)
		    gds__free(lbuf);
#endif
		lbuf = NULL;
	}

	if (svcs)
	{
		*svcs = Service::totalCount();
	}

	return lbuf;
}


#ifdef WIN_NT
static void ExtractDriveLetter(const TEXT* file_name, ULONG* drive_mask)
{
/**************************************
 *
 *	E x t r a c t D r i v e L e t t e r
 *
 **************************************
 *
 * Functional description
 *	Determine the drive letter of file_name
 *	and set the proper bit in the bit mask.
 *		bit 0 = drive A
 *		bit 1 = drive B and so on...
 *	This function is used to determine drive
 *	usage for use with Plug and Play for
 *	MS Windows 4.0.
 *
 **************************************/
	ULONG mask = 1;

	const SHORT shift = (*file_name - 'A');
	mask <<= shift;
	*drive_mask |= mask;
}
#endif


static unsigned int purge_transactions(thread_db*	tdbb,
									   Attachment*	attachment,
									   const bool	force_flag,
									   const ULONG	att_flags)
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
	Database* dbb = attachment->att_database;
	jrd_tra* trans_dbk = attachment->att_dbkey_trans;

	unsigned int count = 0;
	jrd_tra* next;

	for (jrd_tra* transaction = attachment->att_transactions; transaction; transaction = next)
	{
		next = transaction->tra_next;
		if (transaction != trans_dbk)
		{
			if ((transaction->tra_flags & TRA_prepared) || (dbb->dbb_ast_flags & DBB_shutdown) ||
				(att_flags & ATT_shutdown))
			{
				TraceTransactionEnd trace(transaction, false, false);
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
		return count;
	}

	// If there's a side transaction for db-key scope, get rid of it
	if (trans_dbk)
	{
		attachment->att_dbkey_trans = NULL;
		if ((dbb->dbb_ast_flags & DBB_shutdown) || (att_flags & ATT_shutdown))
		{
			TraceTransactionEnd trace(trans_dbk, false, false);
			TRA_release_transaction(tdbb, trans_dbk, &trace);
		}
		else
		{
			TRA_commit(tdbb, trans_dbk, false);
		}
	}

	return 0;
}


static void purge_attachment(thread_db* tdbb, Attachment* attachment, const bool force_flag)
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
 *	NOTE:  This routine assumes that upon entry,
 *	mutex databases_mutex will be locked.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = attachment->att_database;

	tdbb->tdbb_flags |= TDBB_detaching;

	if (!(dbb->dbb_flags & DBB_bugcheck))
	{
		try
		{
			const trig_vec* trig_disconnect = dbb->dbb_triggers[DB_TRIGGER_DISCONNECT];
			if (!(attachment->att_flags & ATT_no_db_triggers) &&
				!(attachment->att_flags & ATT_shutdown) &&
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
				catch (const Exception& ex)
				{
					attachment->att_flags = save_flags;
					if (dbb->dbb_flags & DBB_bugcheck)
						throw;

					ISC_STATUS_ARRAY temp = {0};
					trace_error(tdbb, ex, temp, "DISCONNECT_TRIGGER");

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
			attachment->att_flags |= (ATT_shutdown | ATT_purge_error);
			throw;
		}
	}

	try
	{
		// allow to free resources used by dynamic statements
		EDS::Manager::jrdAttachmentEnd(tdbb, attachment);

		const ULONG att_flags = attachment->att_flags;
		attachment->att_flags |= ATT_shutdown;

		if (!(dbb->dbb_flags & DBB_bugcheck))
		{
			// Check for any pending transactions
			unsigned int count = purge_transactions(tdbb, attachment, force_flag, att_flags);
			if (count)
			{
				ERR_post(Arg::Gds(isc_open_trans) << Arg::Num(count));
			}
		}
	}
	catch (const Exception&)
	{
		attachment->att_flags |= (ATT_shutdown | ATT_purge_error);
		throw;
	}

	// Notify Trace API manager about disconnect
	if (attachment->att_trace_manager->needs().event_detach)
	{
		TraceConnectionImpl conn(attachment);
		attachment->att_trace_manager->event_detach(&conn, false);
	}

	// Unlink attachment from database

	release_attachment(tdbb, attachment);

	// If there are still attachments, do a partial shutdown

	if (dbb->checkHandle())
	{
		if (!dbb->dbb_attachments && !(dbb->dbb_flags & DBB_being_opened))
		{
			shutdown_database(dbb, true);
		}
	}
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

	if (transaction == tdbb->getDatabase()->dbb_sys_trans)
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
static void verify_request_synchronization(jrd_req*& request, SSHORT level)
{
	const USHORT lev = level;
	if (lev) {
		const vec<jrd_req*>* vector = request->req_sub_requests;
		if (!vector || lev >= vector->count() || !(request = (*vector)[lev]))
		{
			ERR_post(Arg::Gds(isc_req_sync));
		}
	}
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
	// Check for security2.fdb
	static TEXT securityNameBuffer[MAXPATHLEN] = "";
	static GlobalPtr<PathName> expandedSecurityNameBuffer;
	static GlobalPtr<Mutex> mutex;

	MutexLockGuard guard(mutex);

	if (! securityNameBuffer[0]) {
		SecurityDatabase::getPath(securityNameBuffer);
		expandedSecurityNameBuffer->assign(securityNameBuffer);
		ISC_expand_filename(expandedSecurityNameBuffer, false);
	}
	if (name == securityNameBuffer || name == expandedSecurityNameBuffer)
		return VDN_SECURITY;

	// Check for .conf
	if (!JRD_verify_database_access(name)) {
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

    @brief	Checks the userinfo database to validate
    password to that passed in.
    Takes into account possible trusted authentication.
	Fills UserId structure with resulting values.

    @param user
    @param options

 **/
static void getUserInfo(UserId& user, const DatabaseOptions& options)
{
	int id = -1, group = -1;	// CVC: This var contained trash
	int node_id = 0;
	string name;

#ifdef BOOT_BUILD
	bool wheel = true;
#else
	bool wheel = false;
	if (options.dpb_trusted_login.hasData())
	{
		name = options.dpb_trusted_login;
	}
	else
	{
		if (options.dpb_user_name.isEmpty() &&
			options.dpb_network_protocol.isEmpty() &&	// This 2 checks ensure that we are not remote server
			options.dpb_remote_address.isEmpty()) 		// process, i.e. can use unix OS auth.
		{
			string s(options.dpb_sys_user_name);
			ISC_utf8ToSystem(s);
			wheel = ISC_get_user(&name, &id, &group, s.nullStr());
			ISC_systemToUtf8(name);
		}

		if (options.dpb_user_name.hasData() || (id == -1))
		{
			const string remote = options.dpb_network_protocol +
				(options.dpb_network_protocol.isEmpty() || options.dpb_remote_address.isEmpty() ? "" : "/") +
				options.dpb_remote_address;

			SecurityDatabase::initialize();
			user.usr_fini_sec_db = true;
			SecurityDatabase::verifyUser(name,
										 options.dpb_user_name.nullStr(),
										 options.dpb_password.nullStr(),
										 options.dpb_password_enc.nullStr(),
										 &id, &group, &node_id, remote);
		}
	}

	// if the name from the user database is defined as SYSDBA,
	// we define that user id as having system privileges

	name.upper();
	if (name == SYSDBA_USER_NAME)
	{
		wheel = true;
	}
#endif // BOOT_BUILD

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
	user.usr_sql_role_name = options.dpb_role_name;
	user.usr_user_id = id;
	user.usr_group_id = group;
	user.usr_node_id = node_id;
	if (wheel)
	{
		user.usr_flags |= USR_locksmith;
	}

	if (options.dpb_trusted_role)
	{
		user.usr_flags |= USR_trole;
	}
}

static ISC_STATUS unwindAttach(const Exception& ex,
							   ISC_STATUS* userStatus,
							   thread_db* tdbb,
							   Attachment* attachment,
							   Database* dbb)
{
	ex.stuff_exception(userStatus);

	if (engineShuttingDown)
	{
		// this attachment will be released as part of engine shutdown process
		// see also cancel_attachments
		if (attachment)
			attachment->att_mutex.leave();
	}
	else
	{
		try
		{
			ThreadStatusGuard temp_status(tdbb);

			dbb->dbb_flags &= ~DBB_being_opened;

			if (attachment)
			{
				release_attachment(tdbb, attachment);
			}

			if (dbb->checkHandle())
			{
				if (!dbb->dbb_attachments)
				{
					shutdown_database(dbb, true);
				}
			}
		}
		catch (const Exception&)
		{
			// no-op
		}
	}

	return userStatus[1];
}

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

	try
	{
		ThreadContextHolder tdbb;

		{ // scope
			MutexLockGuard guard(databases_mutex);

			cancel_attachments();

			Database* dbb_next;
			for (Database* dbb = databases; dbb; dbb = dbb_next)
			{
				dbb_next = dbb->dbb_next;
				if (!shutdown_dbb(tdbb, dbb))
				{
					success = false;
					break;
				}
			}
		}

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
				 !(request->req_flags & (req_internal | req_sys_trigger))) &&
				(!transaction || !(transaction->tra_flags & TRA_system)))
			{
				if (!punt) 
					return true;

				attachment->att_flags &= ~ATT_cancel_raise;
				status_exception::raise(Arg::Gds(isc_cancelled));
			}
		}
	}

	// Handle request cancellation

	if (transaction && (transaction->tra_flags & TRA_cancel_request))
	{
		if (!punt) 
			return true;

		transaction->tra_flags &= ~TRA_cancel_request;
		status_exception::raise(Arg::Gds(isc_cancelled));
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


void JRD_ddl(thread_db* tdbb, /*Jrd::Attachment* attachment,*/ jrd_tra* transaction,
	USHORT ddl_length, const UCHAR* ddl)
{
/**************************************
 *
 *	J R D _ d d l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (!transaction)
		status_exception::raise(Arg::Gds(isc_segstr_wrong_db));

	DYN_ddl(/*attachment,*/ transaction, ddl_length, ddl);
	JRD_autocommit_ddl(tdbb, transaction);
}


void JRD_receive(thread_db* tdbb, jrd_req* request, USHORT msg_type, USHORT msg_length,
	UCHAR* msg, SSHORT level
#ifdef SCROLLABLE_CURSORS
	, USHORT direction, ULONG offset
#endif
	)
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
	verify_request_synchronization(request, level);

#ifdef SCROLLABLE_CURSORS
	if (direction)
		EXE_seek(tdbb, request, direction, offset);
#endif

	EXE_receive(tdbb, request, msg_type, msg_length, msg, true);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


void JRD_request_info(Jrd::thread_db*, jrd_req* request, SSHORT level, SSHORT item_length,
	const UCHAR* items, SLONG buffer_length, UCHAR* buffer)
{
/**************************************
 *
 *	J R D _ r e q u e s t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Return information about requests.
 *
 **************************************/

	verify_request_synchronization(request, level);

	INF_request_info(request, items, item_length, buffer, buffer_length);
}


void JRD_start(Jrd::thread_db* tdbb, jrd_req* request, jrd_tra* transaction, SSHORT level)
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

	if (level)
		request = CMP_clone_request(tdbb, request, level, false);

	EXE_unwind(tdbb, request);
	EXE_start(tdbb, request, transaction);

	check_autocommit(request, tdbb);

	if (request->req_flags & req_warning)
	{
		request->req_flags &= ~req_warning;
		ERR_punt();
	}
}


void JRD_commit_transaction(thread_db* tdbb, jrd_tra** transaction)
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
	commit(tdbb, *transaction, false);
	*transaction = NULL;
}


void JRD_commit_retaining(thread_db* tdbb, jrd_tra** transaction)
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
	commit(tdbb, *transaction, true);
}


void JRD_rollback_transaction(thread_db* tdbb, jrd_tra** transaction)
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
	rollback(tdbb, *transaction, false);
	*transaction = NULL;
}


void JRD_rollback_retaining(thread_db* tdbb, jrd_tra** transaction)
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
	rollback(tdbb, *transaction, true);
}


void JRD_start_and_send(thread_db* tdbb, jrd_req* request, jrd_tra* transaction, USHORT msg_type,
	USHORT msg_length, UCHAR* msg, SSHORT level)
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
	///jrd_tra* transaction = find_transaction(tdbb, isc_req_wrong_db);

	if (level)
		request = CMP_clone_request(tdbb, request, level, false);

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


void JRD_start_multiple(thread_db* tdbb, jrd_tra** tra_handle, USHORT count, TEB* vector)
{
/**************************************
 *
 *	J R D _ s t a r t _ m u l t i p l e
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	jrd_tra* prior = NULL;
	jrd_tra* transaction = NULL;

	try
	{
		if (*tra_handle)
			status_exception::raise(Arg::Gds(isc_bad_trans_handle));

		if (count < 1 || count > MAX_DB_PER_TRANS)
		{
			status_exception::raise(Arg::Gds(isc_max_db_per_trans_allowed) << Arg::Num(MAX_DB_PER_TRANS));
		}

		if (vector == NULL)
		{
			status_exception::raise(Arg::Gds(isc_bad_teb_form));
		}

		for (TEB* v = vector; v < vector + count; v++)
		{
			Attachment* attachment = *v->teb_database;
			AutoPtr<DatabaseContextHolder> dbbHolder;

			if (attachment != tdbb->getAttachment())
			{
				validateHandle(tdbb, attachment);
				dbbHolder = new DatabaseContextHolder(tdbb);
				check_database(tdbb);
			}

			try
			{
				if (v->teb_tpb_length < 0 || (v->teb_tpb_length > 0 && v->teb_tpb == NULL))
				{
					status_exception::raise(Arg::Gds(isc_bad_tpb_form));
				}

				transaction = TRA_start(tdbb, v->teb_tpb_length, v->teb_tpb);

				transaction->tra_sibling = prior;
				prior = transaction;

				// run ON TRANSACTION START triggers
				EXE_execute_db_triggers(tdbb, transaction, jrd_req::req_trigger_trans_start);
			}
			catch(const Exception& ex)
			{
				ISC_STATUS_ARRAY temp = {0};
				trace_error(tdbb, ex, temp, ENTRYPOINT_NAME(GDS_START_TRANSACTION));

				throw;
			}
		}

		*tra_handle = transaction;
	}
	catch (const Exception&)
	{
		if (prior)
		{
			ThreadStatusGuard temp_status(tdbb);

			try
			{
				validateHandle(tdbb, prior->tra_attachment);
				DatabaseContextHolder dbbHolder(tdbb);
				check_database(tdbb);

				rollback(tdbb, prior, false);
			}
			catch (const Exception&)
			{
			}
		}

		throw;
	}
}


void JRD_start_transaction(thread_db* tdbb, jrd_tra** transaction, SSHORT count, ...)
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
	if (count < 1 || USHORT(count) > MAX_DB_PER_TRANS)
	{
		status_exception::raise(Arg::Gds(isc_max_db_per_trans_allowed) << Arg::Num(MAX_DB_PER_TRANS));
	}

	HalfStaticArray<TEB, 16> tebs;
	tebs.grow(count);

	va_list ptr;
	va_start(ptr, count);

	for (TEB* teb_iter = tebs.begin(); teb_iter < tebs.end(); teb_iter++)
	{
		teb_iter->teb_database = va_arg(ptr, Attachment**);
		teb_iter->teb_tpb_length = va_arg(ptr, int);
		teb_iter->teb_tpb = va_arg(ptr, UCHAR*);
	}

	va_end(ptr);

	JRD_start_multiple(tdbb, transaction, count, tebs.begin());
}


void JRD_unwind_request(thread_db* tdbb, jrd_req* request, SSHORT level)
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
	// Pick up and validate request level
	verify_request_synchronization(request, level);

	// Unwind request. This just tweaks some bits.
	EXE_unwind(tdbb, request);
}


void JRD_compile(thread_db* tdbb,
				 Attachment* attachment,
				 jrd_req** req_handle,
				 ULONG blr_length,
				 const UCHAR* blr,
				 RefStrPtr ref_str,
				 USHORT dbginfo_length, const UCHAR* dbginfo)
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

	jrd_req* request = CMP_compile2(tdbb, blr, blr_length, false, dbginfo_length, dbginfo);

	request->req_attachment = attachment;
	request->req_request = attachment->att_requests;
	attachment->att_requests = request;

	if (!ref_str)
	{
		fb_assert(request->req_blr.isEmpty());

		// hvlad: if\when we implement request's cache in the future and
		// CMP_compile2 will return us previously compiled request with
		// non-empty req_blr, then we must replace assertion by the line below
		// if (!request->req_blr.isEmpty())

		request->req_blr.insert(0, blr, blr_length);
	}
	else {
		request->req_sql_text = ref_str;
	}

	*req_handle = request;
}


namespace {
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


ISC_STATUS GDS_PING(ISC_STATUS* user_status, Attachment** db_handle)
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
		ThreadContextHolder tdbb(user_status);

		Attachment* const attachment = *db_handle;
		validateHandle(tdbb, attachment);
		DatabaseContextHolder dbbHolder(tdbb, false);
		check_database(tdbb);
	}
	catch (const Exception& ex)
	{
		return ex.stuff_exception(user_status);
	}

	return successful_completion(user_status);
}

namespace
{
	typedef Array<const void*> PingQueue;

	THREAD_ENTRY_DECLARE attachmentShutdownThread(THREAD_ENTRY_PARAM arg)
	{
		AutoPtr<PingQueue> queue(static_cast<PingQueue*>(arg));

		while (!queue->isEmpty())
		{
			FB_API_HANDLE public_handle = WHY_get_public_attachment_handle(queue->pop());
			ISC_STATUS_ARRAY local_status = {isc_arg_gds, FB_SUCCESS, isc_arg_end};
			fb_ping(local_status, &public_handle);
		}

		return 0;
	}
} // namespace


void JRD_shutdown_attachments(const Database* dbb)
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
		PingQueue* const queue = FB_NEW(pool) PingQueue(pool);

		for (const Attachment* attachment = dbb->dbb_attachments;
			attachment; attachment = attachment->att_next)
		{
			if (attachment->att_flags & ATT_shutdown)
			{
				queue->add(attachment);
			}
		}

		gds__thread_start(attachmentShutdownThread, queue, 0, 0, NULL);
	}
	catch (const Exception&)
	{} // no-op
}
