/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		TraceManager.cpp
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

#include "firebird.h"

#include "../../jrd/trace/TraceManager.h"
#include "../../jrd/trace/TraceObjects.h"
#include "../../common/os/path_utils.h"
#include "../../common/ScanDir.h"
#include "../../common/isc_proto.h"
#include "../../common/classes/GetPlugins.h"

#ifdef WIN_NT
#include <process.h>
#endif

using namespace Firebird;

namespace
{
	static const char* const NTRACE_PREFIX = "fbtrace";

	// This may be used when old plugin, missing some newer events is used.
	// Reasonable action here is to log once and ignore next times.
	class IgnoreMissing
	{
	public:
		virtual int FB_CARG noEvent()
		{
			static bool flagFirst = true;

			if (flagFirst)
			{
				flagFirst = false;
				gds__log("Old version of trace plugin is used - new types of events are ignored");
			}

			return 1;
		}
	};

	MakeUpgradeInfo<IgnoreMissing> upgradePlugin;
	MakeUpgradeInfo<> upgradeFactory;
}

namespace Jrd {

GlobalPtr<StorageInstance, InstanceControl::PRIORITY_DELETE_FIRST> TraceManager::storageInstance;
TraceManager::Factories* TraceManager::factories = NULL;
GlobalPtr<Mutex> TraceManager::init_factories_mtx;
volatile bool TraceManager::init_factories;


bool TraceManager::check_result(TracePlugin* plugin, const char* module, const char* function,
	bool result)
{
	if (result)
		return true;

	if (!plugin)
	{
		gds__log("Trace plugin %s returned error on call %s, "
			"did not create plugin and provided no additional details on reasons of failure",
			module, function);
		return false;
	}

	const char* errorStr = plugin->trace_get_error();

	if (!errorStr)
	{
		gds__log("Trace plugin %s returned error on call %s, "
			"but provided no additional details on reasons of failure", module, function);
		return false;
	}

	gds__log("Trace plugin %s returned error on call %s.\n\tError details: %s",
		module, function, errorStr);
	return false;
}

TraceManager::TraceManager(Attachment* in_att) :
	attachment(in_att),
	service(NULL),
	filename(NULL),
	trace_sessions(*in_att->att_pool)
{
	init();
}

TraceManager::TraceManager(Service* in_svc) :
	attachment(NULL),
	service(in_svc),
	filename(NULL),
	trace_sessions(in_svc->getPool())
{
	init();
}

TraceManager::TraceManager(const char* in_filename) :
	attachment(NULL),
	service(NULL),
	filename(in_filename),
	trace_sessions(*getDefaultMemoryPool())
{
	init();
}

TraceManager::~TraceManager()
{
}

void TraceManager::init()
{
	// ensure storage is initialized
	getStorage();
	load_plugins();
	changeNumber = 0;
}

void TraceManager::load_plugins()
{
	// Initialize all trace needs to false
	trace_needs = 0;

	if (init_factories)
		return;

	MutexLockGuard guard(init_factories_mtx, FB_FUNCTION);
	if (init_factories)
		return;

	init_factories = true;

	factories = FB_NEW(*getDefaultMemoryPool()) TraceManager::Factories(*getDefaultMemoryPool());
	for (GetPlugins<TraceFactory> traceItr(PluginType::Trace, FB_TRACE_FACTORY_VERSION, upgradeFactory);
		 traceItr.hasData(); traceItr.next())
	{
		FactoryInfo info;
		info.factory = traceItr.plugin();
		info.factory->addRef();
		string name(traceItr.name());
		name.copyTo(info.name, sizeof(info.name));
		factories->add(info);
	}
}


void TraceManager::shutdown()
{
	if (init_factories)
	{
		MutexLockGuard guard(init_factories_mtx, FB_FUNCTION);

		if (init_factories)
		{
			delete factories;
			factories = NULL;
			init_factories = false;
		}
	}
}


void TraceManager::update_sessions()
{
	SortedArray<ULONG> liveSessions(*getDefaultMemoryPool());

	{	// scope
		ConfigStorage* storage = getStorage();

		StorageGuard guard(storage);
		storage->restart();

		TraceSession session(*getDefaultMemoryPool());
		while (storage->getNextSession(session))
		{
			if ((session.ses_flags & trs_active) && !(session.ses_flags & trs_log_full))
			{
				update_session(session);
				liveSessions.add(session.ses_id);
			}
		}

		changeNumber = storage->getChangeNumber();
	}

	// remove sessions not present in storage
	size_t i = 0;
	while (i < trace_sessions.getCount())
	{
		size_t pos;
		if (liveSessions.find(trace_sessions[i].ses_id, pos)) {
			i++;
		}
		else
		{
			trace_sessions[i].plugin->release();
			trace_sessions.remove(i);
		}
	}

	// nothing to trace, clear needs
	if (trace_sessions.getCount() == 0)
	{
		trace_needs = 0;
	}
}

void TraceManager::update_session(const TraceSession& session)
{
	// if this session is already known, nothing to do
	size_t pos;
	if (trace_sessions.find(session.ses_id, pos)) {
		return;
	}

	// if this session is not from administrator, it may trace connections
	// only created by the same user
	if (!(session.ses_flags & trs_admin))
	{
		if (attachment)
		{
			if (!attachment->att_user || attachment->att_user->usr_user_name != session.ses_user)
				return;
		}
		else if (service)
		{
			if (session.ses_user != service->getUserName())
				return;
		}
		else
		{
			// failed attachment attempts traced by admin trace only
			return;
		}
	}

	MasterInterfacePtr master;

	for (FactoryInfo* info = factories->begin(); info != factories->end(); ++info)
	{
		TraceInitInfoImpl attachInfo(session, attachment, filename);
		LocalStatus status;
		TracePlugin* plugin = info->factory->trace_create(&status, &attachInfo);

		if (plugin)
		{
			master->upgradeInterface(plugin, FB_TRACE_PLUGIN_VERSION, upgradePlugin);

			plugin->addRef();
			SessionInfo sesInfo;
			sesInfo.plugin = plugin;
			sesInfo.factory_info = info;
			sesInfo.ses_id = session.ses_id;
			trace_sessions.add(sesInfo);

			trace_needs |= info->factory->trace_needs();
		}
		else if (!status.isSuccess())
		{
			string header;
			header.printf("Trace plugin %s returned error on call trace_create.", info->name);
			iscLogStatus(header.c_str(), status.get());
		}
	}
}

bool TraceManager::need_dsql_prepare(Attachment* att)
{
	return att->att_trace_manager->needs(TRACE_EVENT_DSQL_PREPARE);
}

bool TraceManager::need_dsql_free(Attachment* att)
{
	return att->att_trace_manager->needs(TRACE_EVENT_DSQL_FREE);
}

bool TraceManager::need_dsql_execute(Attachment* att)
{
	return att->att_trace_manager->needs(TRACE_EVENT_DSQL_EXECUTE);
}

void TraceManager::event_dsql_prepare(Attachment* att, jrd_tra* transaction,
		TraceSQLStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	TraceConnectionImpl conn(att);
	TraceTransactionImpl tran(transaction);

	att->att_trace_manager->event_dsql_prepare(&conn, transaction ? &tran : NULL, statement,
											   time_millis, req_result);
}

void TraceManager::event_dsql_free(Attachment* att,	TraceSQLStatement* statement,
		unsigned short option)
{
	TraceConnectionImpl conn(att);

	att->att_trace_manager->event_dsql_free(&conn, statement, option);
}

void TraceManager::event_dsql_execute(Attachment* att, jrd_tra* transaction,
	TraceSQLStatement* statement, bool started, ntrace_result_t req_result)
{
	TraceConnectionImpl conn(att);
	TraceTransactionImpl tran(transaction);

	att->att_trace_manager->event_dsql_execute(&conn, &tran, statement, started, req_result);
}


#define EXECUTE_HOOKS(METHOD, PARAMS) \
	size_t i = 0; \
	while (i < trace_sessions.getCount()) \
	{ \
		SessionInfo* plug_info = &trace_sessions[i]; \
		if (check_result(plug_info->plugin, plug_info->factory_info->name, #METHOD, \
			plug_info->plugin->METHOD PARAMS)) \
		{ \
			i++; /* Move to next plugin */ \
		} \
		else { \
			trace_sessions.remove(i); /* Remove broken plugin from the list */ \
		} \
	}


void TraceManager::event_attach(TraceDatabaseConnection* connection,
		bool create_db, ntrace_result_t att_result)
{
	EXECUTE_HOOKS(trace_attach,
		(connection, create_db, att_result));
}

void TraceManager::event_detach(TraceDatabaseConnection* connection, bool drop_db)
{
	EXECUTE_HOOKS(trace_detach, (connection, drop_db));
}

void TraceManager::event_transaction_start(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, size_t tpb_length, const ntrace_byte_t* tpb,
		ntrace_result_t tra_result)
{
	EXECUTE_HOOKS(trace_transaction_start,
		(connection, transaction, tpb_length, tpb, tra_result));
}

void TraceManager::event_transaction_end(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, bool commit, bool retain_context,
		ntrace_result_t tra_result)
{
	EXECUTE_HOOKS(trace_transaction_end,
		(connection, transaction, commit, retain_context, tra_result));
}

void TraceManager::event_set_context(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceContextVariable* variable)
{
	EXECUTE_HOOKS(trace_set_context,
		(connection, transaction, variable));
}

 void TraceManager::event_proc_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceProcedure* procedure, bool started, ntrace_result_t proc_result)
{
	EXECUTE_HOOKS(trace_proc_execute,
		(connection, transaction, procedure, started, proc_result));
}

 void TraceManager::event_func_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceFunction* function, bool started, ntrace_result_t func_result)
 {
	EXECUTE_HOOKS(trace_func_execute,
		(connection, transaction, function, started, func_result));
 }

void TraceManager::event_trigger_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceTrigger* trigger, bool started, ntrace_result_t trig_result)
{
	EXECUTE_HOOKS(trace_trigger_execute,
		(connection, transaction, trigger, started, trig_result));
}

void TraceManager::event_dsql_prepare(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(trace_dsql_prepare,
		(connection, transaction, statement,
		 time_millis, req_result));
}

void TraceManager::event_dsql_free(TraceDatabaseConnection* connection,
		TraceSQLStatement* statement, unsigned short option)
{
	EXECUTE_HOOKS(trace_dsql_free,
		(connection, statement, option));
}

void TraceManager::event_dsql_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceSQLStatement* statement, bool started, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(trace_dsql_execute,
		(connection, transaction, statement, started, req_result));
}


void TraceManager::event_blr_compile(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(trace_blr_compile,
		(connection, transaction, statement,
		 time_millis, req_result));
}

void TraceManager::event_blr_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_result_t req_result)
{
	EXECUTE_HOOKS(trace_blr_execute,
		(connection, transaction, statement, req_result));
}

void TraceManager::event_dyn_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceDYNRequest* request,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(trace_dyn_execute,
		(connection, transaction, request, time_millis,
			req_result));
}

void TraceManager::event_service_attach(TraceServiceConnection* service, ntrace_result_t att_result)
{
	EXECUTE_HOOKS(trace_service_attach,
		(service, att_result));
}

void TraceManager::event_service_start(TraceServiceConnection* service,
		size_t switches_length, const char* switches,
		ntrace_result_t start_result)
{
	EXECUTE_HOOKS(trace_service_start,
		(service, switches_length, switches, start_result));
}

void TraceManager::event_service_query(TraceServiceConnection* service,
		size_t send_item_length, const ntrace_byte_t* send_items,
		size_t recv_item_length, const ntrace_byte_t* recv_items,
		ntrace_result_t query_result)
{
	EXECUTE_HOOKS(trace_service_query,
		(service, send_item_length, send_items,
		 recv_item_length, recv_items, query_result));
}

void TraceManager::event_service_detach(TraceServiceConnection* service, ntrace_result_t detach_result)
{
	EXECUTE_HOOKS(trace_service_detach,
		(service, detach_result));
}

void TraceManager::event_error(TraceBaseConnection* connection, TraceStatusVector* status, const char* function)
{
	EXECUTE_HOOKS(trace_event_error,
		(connection, status, function));
}


void TraceManager::event_sweep(TraceDatabaseConnection* connection, TraceSweepInfo* sweep,
		ntrace_process_state_t sweep_state)
{
	EXECUTE_HOOKS(trace_event_sweep,
		(connection, sweep, sweep_state));
}

}
