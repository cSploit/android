/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		TraceManager.h
 *	DESCRIPTION:	Trace API manager
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2008 Khorsun Vladyslav
 */

#ifndef JRD_TRACEMANAGER_H
#define JRD_TRACEMANAGER_H

#include <time.h>
#include "../../jrd/ntrace.h"
#include "../../common/classes/array.h"
#include "../../common/classes/fb_string.h"
#include "../../common/classes/init.h"
#include "../../common/classes/locks.h"
#include "../../jrd/trace/TraceConfigStorage.h"
#include "../../jrd/trace/TraceSession.h"

namespace Jrd {

class Database;
class Attachment;
class jrd_tra;
class Service;

class TraceManager
{
public:
    /* Initializes plugins. */
	explicit TraceManager(Attachment* in_att);
	explicit TraceManager(Service* in_svc);
	explicit TraceManager(const char* in_filename);

	/* Finalize plugins. Called when database is closed by the engine */
	~TraceManager();

	static ConfigStorage* getStorage()
	{ return storageInstance.getStorage(); }

	static size_t pluginsCount()
	{ return modules->getCount(); }

	void event_attach(TraceDatabaseConnection* connection, bool create_db,
		ntrace_result_t att_result);

	void event_detach(TraceDatabaseConnection* connection, bool drop_db);

	/* Start/end transaction */
	void event_transaction_start(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		size_t tpb_length, const ntrace_byte_t* tpb, ntrace_result_t tra_result);

	void event_transaction_end(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		bool commit, bool retain_context, ntrace_result_t tra_result);

	void event_set_context(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceContextVariable* variable);

	void event_proc_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceProcedure* procedure, bool started, ntrace_result_t proc_result);

	void event_trigger_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceTrigger* trigger, bool started, ntrace_result_t trig_result);

	void event_blr_compile(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result);

	void event_blr_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_result_t req_result);

	void event_dyn_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceDYNRequest* request,
		ntrace_counter_t time_millis, ntrace_result_t req_result);

	void event_service_attach(TraceServiceConnection* service, ntrace_result_t att_result);

	void event_service_start(TraceServiceConnection* service,
		size_t switches_length, const char* switches,
		ntrace_result_t start_result);

	void event_service_query(TraceServiceConnection* service,
		size_t send_item_length, const ntrace_byte_t* send_items,
		size_t recv_item_length, const ntrace_byte_t* recv_items,
		ntrace_result_t query_result);

	void event_service_detach(TraceServiceConnection* service, ntrace_result_t detach_result);

	void event_error(TraceBaseConnection* connection, TraceStatusVector* status, const char* function);

	void event_sweep(TraceDatabaseConnection* connection, TraceSweepInfo* sweep, 
		ntrace_process_state_t sweep_state);

	struct NotificationNeeds
	{
		// Set if event is tracked
		bool event_attach;
		bool event_detach;
		bool event_transaction_start;
		bool event_transaction_end;
		bool event_set_context;
		bool event_proc_execute;
		bool event_trigger_execute;
		bool event_dsql_prepare;
		bool event_dsql_free;
		bool event_dsql_execute;
		bool event_blr_compile;
		bool event_blr_execute;
		bool event_dyn_execute;
		bool event_service_attach;
		bool event_service_start;
		bool event_service_query;
		bool event_service_detach;
		bool event_error;
		bool event_sweep;
	};

	inline const NotificationNeeds& needs()
	{
		if (changeNumber != getStorage()->getChangeNumber())
			update_sessions();
		return trace_needs;
	}

	/* DSQL-friendly routines to call Trace API hooks.
       Needed because DSQL cannot include JRD for the current engine */
	static bool need_dsql_prepare(Attachment* att);
	static bool need_dsql_free(Attachment* att);
	static bool need_dsql_execute(Attachment* att);

	static void event_dsql_prepare(Attachment* att, jrd_tra* transaction, TraceSQLStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result);

	static void event_dsql_free(Attachment* att, TraceSQLStatement* statement,
		unsigned short option);

	static void event_dsql_execute(Attachment* att, jrd_tra* transaction, TraceSQLStatement* statement,
		bool started, ntrace_result_t req_result);

private:
	Attachment*	attachment;
	Service* service;
	const char* filename;
	NotificationNeeds trace_needs;

	// This structure should be POD-like to be stored in Array
	struct ModuleInfo
	{
		ModuleInfo()
		{
			ntrace_attach = NULL;
			memset(module, 0, sizeof(module));
		}

		ntrace_attach_t ntrace_attach;
		char module[MAXPATHLEN];
	};
	static Firebird::GlobalPtr<Firebird::Array<ModuleInfo> > modules;
	static Firebird::GlobalPtr<Firebird::Mutex> init_modules_mtx;
	static volatile bool init_modules;

	struct SessionInfo
	{
		const TracePlugin* plugin;
		ModuleInfo* module_info;
		ULONG ses_id;

		static ULONG generate(const void*, const SessionInfo& item)
		{ return item.ses_id; }
	};
	Firebird::SortedArray<SessionInfo, Firebird::EmptyStorage<SessionInfo>,
		ULONG, SessionInfo> trace_sessions;

	void init();
	void load_modules();
	void update_sessions();
	void update_session(const Firebird::TraceSession& session);

	bool check_result(const TracePlugin* plugin, const char* module, const char* function, bool result);

	/* DSQL statement lifecycle. To be moved to public and used directly when DSQL becomes a part of JRD */
	void event_dsql_prepare(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result);

	void event_dsql_free(TraceDatabaseConnection* connection,
		TraceSQLStatement* statement, unsigned short option);

	void event_dsql_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement,
		bool started, ntrace_result_t req_result);

	static StorageInstance storageInstance;

	ULONG changeNumber;
};

}

#endif
