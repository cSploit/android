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

#include "../../jrd/common.h"
#include "../../jrd/trace/TraceManager.h"
#include "../../jrd/trace/TraceObjects.h"
#include "../../jrd/os/path_utils.h"
#include "../config/ScanDir.h"

#ifdef WIN_NT
#include <process.h>
#endif

using namespace Firebird;

namespace
{
	static const char* const NTRACE_PREFIX = "fbtrace";
}

namespace Jrd {

GlobalPtr<Array<TraceManager::ModuleInfo> > TraceManager::modules;
GlobalPtr<Mutex> TraceManager::init_modules_mtx;
volatile bool TraceManager::init_modules = false;

StorageInstance TraceManager::storageInstance;

bool TraceManager::check_result(const TracePlugin* plugin, const char* module, const char* function,
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

	const char* errorStr = plugin->tpl_get_error(plugin);

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
	// Destroy all plugins
	for (SessionInfo* info = trace_sessions.begin(); info < trace_sessions.end(); info++)
	{
		check_result(NULL, info->module_info->module, "tpl_shutdown",
			info->plugin->tpl_shutdown(info->plugin));
	}
}

void TraceManager::init()
{
	// ensure storage is initialized
	getStorage();
	load_modules();
	changeNumber = 0;
}

void TraceManager::load_modules()
{
	// Initialize all trace needs to false
	memset(&trace_needs, 0, sizeof(trace_needs));

	if (init_modules)
		return;

	MutexLockGuard guard(init_modules_mtx);
	if (init_modules)
		return;

	init_modules = true;

	PathName plugdir = fb_utils::getPrefix(fb_utils::FB_DIR_PLUGINS, "");
	ScanDir plugins(plugdir.c_str(), "*.*");

	while (plugins.next())
	{
		const PathName modLib(plugins.getFileName());
		if (modLib.find(NTRACE_PREFIX) == PathName::npos)
			continue;

		PathName fullModName;
		PathUtils::concatPath(fullModName, plugdir, modLib);

		ModuleLoader::Module* m = ModuleLoader::loadModule(fullModName);
		if (!m)
			continue;

		ntrace_attach_t ntrace_attach_func = (ntrace_attach_t) (m->findSymbol(NTRACE_ATTACH));
		if (!ntrace_attach_func)
			continue;

		ModuleInfo info;
		info.ntrace_attach = ntrace_attach_func;
		modLib.copyTo(info.module, sizeof(info.module));
		modules->add(info);
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
			const TracePlugin* p = trace_sessions[i].plugin;
			ModuleInfo* info = trace_sessions[i].module_info;

			check_result(p, info->module, "tpl_shutdown", p->tpl_shutdown(p));

			trace_sessions.remove(i);
		}
	}

	// nothing to trace, clear needs
	if (trace_sessions.getCount() == 0) {
		memset(&trace_needs, 0, sizeof(trace_needs));
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

	ModuleInfo* info = modules->begin();
	for (; info < modules->end(); info++)
	{
		TraceInitInfoImpl attachInfo(session, attachment, filename);

		const TracePlugin* plugin = NULL;
		const bool attached = info->ntrace_attach(&attachInfo, &plugin);
		if (!attached || !plugin)
		{
			// Put error into firebird.log only if no plugin was created or this
			// is a system audit session, else assume plugin already put error 
			// into trace log
			if (!plugin || (session.ses_flags & trs_system))
				check_result(plugin, info->module, NTRACE_ATTACH, attached);

			// This was a skeletal plugin to return an error
			if (plugin && plugin->tpl_shutdown)
				plugin->tpl_shutdown(plugin);

			plugin = NULL;
		}

		if (plugin && plugin->tpl_version != NTRACE_VERSION)
		{
			gds__log("Module \"%s\" created trace plugin version %d. Supported version %d",
				info->module, plugin->tpl_version, NTRACE_VERSION);

			// plugin->tpl_shutdown(plugin);
			plugin = NULL;
			modules->remove(info);

			info--;
		}

		if (plugin)
		{
			SessionInfo sesInfo;
			sesInfo.plugin = plugin;
			sesInfo.module_info = info;
			sesInfo.ses_id = session.ses_id;
			trace_sessions.add(sesInfo);

			if (plugin->tpl_event_attach)
				trace_needs.event_attach = true;
			if (plugin->tpl_event_detach)
				trace_needs.event_detach = true;
			if (plugin->tpl_event_transaction_start)
				trace_needs.event_transaction_start = true;
			if (plugin->tpl_event_transaction_end)
				trace_needs.event_transaction_end = true;
			if (plugin->tpl_event_set_context)
				trace_needs.event_set_context = true;
			if (plugin->tpl_event_proc_execute)
				trace_needs.event_proc_execute = true;
			if (plugin->tpl_event_dsql_prepare)
				trace_needs.event_dsql_prepare = true;
			if (plugin->tpl_event_dsql_free)
				trace_needs.event_dsql_free = true;
			if (plugin->tpl_event_dsql_execute)
				trace_needs.event_dsql_execute = true;
			if (plugin->tpl_event_blr_compile)
				trace_needs.event_blr_compile = true;
			if (plugin->tpl_event_blr_execute)
				trace_needs.event_blr_execute = true;
			if (plugin->tpl_event_dyn_execute)
				trace_needs.event_dyn_execute = true;
			if (plugin->tpl_event_service_attach)
				trace_needs.event_service_attach = true;
			if (plugin->tpl_event_service_start)
				trace_needs.event_service_start = true;
			if (plugin->tpl_event_service_query)
				trace_needs.event_service_query = true;
			if (plugin->tpl_event_service_detach)
				trace_needs.event_service_detach = true;
			if (plugin->tpl_event_trigger_execute)
				trace_needs.event_trigger_execute = true;
			if (plugin->tpl_event_error)
				trace_needs.event_error = true;
			if (plugin->tpl_event_sweep)
				trace_needs.event_sweep = true;
		}
	}
}

bool TraceManager::need_dsql_prepare(Attachment* att)
{
	return att->att_trace_manager->needs().event_dsql_prepare;
}

bool TraceManager::need_dsql_free(Attachment* att)
{
	return att->att_trace_manager->needs().event_dsql_free;
}

bool TraceManager::need_dsql_execute(Attachment* att)
{
	return att->att_trace_manager->needs().event_dsql_execute;
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
		if (!plug_info->plugin->METHOD || \
			check_result(plug_info->plugin, plug_info->module_info->module, #METHOD, \
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
	EXECUTE_HOOKS(tpl_event_attach,
		(plug_info->plugin, connection, create_db, att_result));
}

void TraceManager::event_detach(TraceDatabaseConnection* connection, bool drop_db)
{
	EXECUTE_HOOKS(tpl_event_detach, (plug_info->plugin, connection, drop_db));
}

void TraceManager::event_transaction_start(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, size_t tpb_length, const ntrace_byte_t* tpb,
		ntrace_result_t tra_result)
{
	EXECUTE_HOOKS(tpl_event_transaction_start,
		(plug_info->plugin, connection, transaction, tpb_length, tpb, tra_result));
}

void TraceManager::event_transaction_end(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, bool commit, bool retain_context,
		ntrace_result_t tra_result)
{
	EXECUTE_HOOKS(tpl_event_transaction_end,
		(plug_info->plugin, connection, transaction, commit, retain_context, tra_result));
}

void TraceManager::event_set_context(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceContextVariable* variable)
{
	EXECUTE_HOOKS(tpl_event_set_context,
		(plug_info->plugin, connection, transaction, variable));
}

 void TraceManager::event_proc_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceProcedure* procedure, bool started, ntrace_result_t proc_result)
{
	EXECUTE_HOOKS(tpl_event_proc_execute,
		(plug_info->plugin, connection, transaction, procedure, started, proc_result));
}

void TraceManager::event_trigger_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceTrigger* trigger, bool started, ntrace_result_t trig_result)
{
	EXECUTE_HOOKS(tpl_event_trigger_execute,
		(plug_info->plugin, connection, transaction, trigger, started, trig_result));
}

void TraceManager::event_dsql_prepare(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(tpl_event_dsql_prepare,
		(plug_info->plugin, connection, transaction, statement,
		 time_millis, req_result));
}

void TraceManager::event_dsql_free(TraceDatabaseConnection* connection,
		TraceSQLStatement* statement, unsigned short option)
{
	EXECUTE_HOOKS(tpl_event_dsql_free,
		(plug_info->plugin, connection, statement, option));
}

void TraceManager::event_dsql_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceSQLStatement* statement, bool started, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(tpl_event_dsql_execute,
		(plug_info->plugin, connection, transaction, statement, started, req_result));
}


void TraceManager::event_blr_compile(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(tpl_event_blr_compile,
		(plug_info->plugin, connection, transaction, statement,
		 time_millis, req_result));
}

void TraceManager::event_blr_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_result_t req_result)
{
	EXECUTE_HOOKS(tpl_event_blr_execute,
		(plug_info->plugin, connection, transaction, statement, req_result));
}

void TraceManager::event_dyn_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceDYNRequest* request,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	EXECUTE_HOOKS(tpl_event_dyn_execute,
		(plug_info->plugin, connection, transaction, request, time_millis,
			req_result));
}

void TraceManager::event_service_attach(TraceServiceConnection* service, ntrace_result_t att_result)
{
	EXECUTE_HOOKS(tpl_event_service_attach,
		(plug_info->plugin, service, att_result));
}

void TraceManager::event_service_start(TraceServiceConnection* service,
		size_t switches_length, const char* switches,
		ntrace_result_t start_result)
{
	EXECUTE_HOOKS(tpl_event_service_start,
		(plug_info->plugin, service, switches_length, switches, start_result));
}

void TraceManager::event_service_query(TraceServiceConnection* service,
		size_t send_item_length, const ntrace_byte_t* send_items,
		size_t recv_item_length, const ntrace_byte_t* recv_items,
		ntrace_result_t query_result)
{
	EXECUTE_HOOKS(tpl_event_service_query,
		(plug_info->plugin, service, send_item_length, send_items,
		 recv_item_length, recv_items, query_result));
}

void TraceManager::event_service_detach(TraceServiceConnection* service, ntrace_result_t detach_result)
{
	EXECUTE_HOOKS(tpl_event_service_detach,
		(plug_info->plugin, service, detach_result));
}


void TraceManager::event_error(TraceBaseConnection* connection, TraceStatusVector* status, const char* function)
{
	EXECUTE_HOOKS(tpl_event_error,
		(plug_info->plugin, connection, status, function));
}

void TraceManager::event_sweep(TraceDatabaseConnection* connection, TraceSweepInfo* sweep, 
	ntrace_process_state_t sweep_state)
{
	EXECUTE_HOOKS(tpl_event_sweep,
		(plug_info->plugin, connection, sweep, sweep_state));
}

}
