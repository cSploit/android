/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		TracePluginImpl.h
 *	DESCRIPTION:	Plugin implementation
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

#ifndef TRACEPLUGINIMPL_H
#define TRACEPLUGINIMPL_H

#include "../../jrd/ntrace.h"
#include "TracePluginConfig.h"
#include "TraceUnicodeUtils.h"
#include "../../jrd/intl_classes.h"
#include "../../jrd/evl_string.h"
#include "../../jrd/TextType.h"
#include "../../jrd/SimilarToMatcher.h"
#include "../../common/classes/rwlock.h"
#include "../../common/classes/GenericMap.h"
#include "../../common/classes/locks.h"

// Bring in off_t
#include <sys/types.h>


class TracePluginImpl
{
public:
	// Create skeletal plugin (to report initialization error)
	static TracePlugin* createSkeletalPlugin();

	// Create trace plugin for particular database
	static TracePlugin* createFullPlugin(const TracePluginConfig& configuration, TraceInitInfo* initInfo);

	// Serialize exception to TLS buffer to return it to user
	static const char* marshal_exception(const Firebird::Exception& ex);

	// Data for tracked (active) connections
	struct ConnectionData
	{
        int id;
		Firebird::string* description;

		// Deallocate memory used by objects hanging off this structure
		void deallocate_references()
		{
			delete description;
			description = NULL;
		}

		static const int& generate(const void* /*sender*/, const ConnectionData& item)
		{
			return item.id;
		}
	};

	typedef Firebird::BePlusTree<ConnectionData, int, Firebird::MemoryPool, ConnectionData>
		ConnectionsTree;

	// Data for tracked (active) transactions
	struct TransactionData
	{
		int id;
		Firebird::string* description;

		// Deallocate memory used by objects hanging off this structure
		void deallocate_references()
		{
			delete description;
			description = NULL;
		}

		static const int& generate(const void* /*sender*/, const TransactionData& item)
		{
			return item.id;
		}
	};

	typedef Firebird::BePlusTree<TransactionData, int, Firebird::MemoryPool, TransactionData>
		TransactionsTree;

	// Data for tracked (active) statements
	struct StatementData
	{
		unsigned int id;
		Firebird::string* description; // NULL in this field indicates that tracing of this statement is not desired

		static const unsigned int& generate(const void* /*sender*/, const StatementData& item)
		{
			return item.id;
		}
	};

	typedef Firebird::BePlusTree<StatementData, unsigned int, Firebird::MemoryPool, StatementData>
		StatementsTree;

	struct ServiceData
	{
		ntrace_service_t id;
		Firebird::string* description;
		bool enabled;

		// Deallocate memory used by objects hanging off this structure
		void deallocate_references()
		{
			delete description;
			description = NULL;
		}

		static const ntrace_service_t& generate(const void* /*sender*/, const ServiceData& item)
		{
			return item.id;
		}
	};

	typedef Firebird::BePlusTree<ServiceData, ntrace_service_t, Firebird::MemoryPool, ServiceData>
		ServicesTree;

private:
	TracePluginImpl(const TracePluginConfig& configuration, TraceInitInfo* initInfo);
	~TracePluginImpl();

	bool operational; // Set if plugin is fully initialized and is ready for logging
					  // Keep this member field first to ensure its correctness
					  // when destructor is called
	const int session_id;				// trace session ID, set by Firebird
	Firebird::string session_name;		// trace session name, set by Firebird
	TraceLogWriter* logWriter;
	TracePluginConfig config;	// Immutable, thus thread-safe
	Firebird::string record;

	// Data for currently active connections, transactions, statements
	Firebird::RWLock connectionsLock;
	ConnectionsTree connections;

	Firebird::RWLock transactionsLock;
	TransactionsTree transactions;

	Firebird::RWLock statementsLock;
	StatementsTree statements;

	Firebird::RWLock servicesLock;
	ServicesTree services;

	// Lock for log rotation
	Firebird::RWLock renameLock;

	UnicodeCollationHolder unicodeCollation;
	Firebird::AutoPtr<Firebird::SimilarToMatcher<Jrd::UpcaseConverter<Jrd::NullStrConverter>, UCHAR> >
		include_matcher, exclude_matcher;

	void appendGlobalCounts(const PerformanceInfo* info);
	void appendTableCounts(const PerformanceInfo* info);
	void appendParams(TraceParams* params);
	void appendServiceQueryParams(size_t send_item_length, const ntrace_byte_t* send_items,
								  size_t recv_item_length, const ntrace_byte_t* recv_items);
	void formatStringArgument(Firebird::string& result, const UCHAR* str, size_t len);

	// register various objects
	void register_connection(TraceDatabaseConnection* connection);
	void register_transaction(TraceTransaction* transaction);
	void register_sql_statement(TraceSQLStatement* statement);
	void register_blr_statement(TraceBLRStatement* statement);
	void register_service(TraceServiceConnection* service);
	
	bool checkServiceFilter(TraceServiceConnection* service, bool started);

	// Write message to text log file
	void logRecord(const char* action);
	void logRecordConn(const char* action, TraceDatabaseConnection* connection);
	void logRecordTrans(const char* action, TraceDatabaseConnection* connection,
		TraceTransaction* transaction);
	void logRecordProc(const char* action, TraceDatabaseConnection* connection,
		TraceTransaction* transaction, const char* proc_name);
	void logRecordStmt(const char* action, TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceStatement* statement,
		bool isSQL);
	void logRecordServ(const char* action, TraceServiceConnection* service);
	void logRecordError(const char* action, TraceBaseConnection* connection, TraceStatusVector* status);

	/* Methods which do logging of events to file */
	void log_init();
	void log_finalize();

	void log_event_attach(
		TraceDatabaseConnection* connection, ntrace_boolean_t create_db,
		ntrace_result_t att_result);
	void log_event_detach(
		TraceDatabaseConnection* connection, ntrace_boolean_t drop_db);

	void log_event_transaction_start(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		size_t tpb_length, const ntrace_byte_t* tpb, ntrace_result_t tra_result);
	void log_event_transaction_end(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		ntrace_boolean_t commit, ntrace_boolean_t retain_context, ntrace_result_t tra_result);

	void log_event_set_context(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceContextVariable* variable);

	void log_event_proc_execute(
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceProcedure* procedure,
		bool started, ntrace_result_t proc_result);

	void log_event_trigger_execute(
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceTrigger* trigger,
		bool started, ntrace_result_t trig_result);

	void log_event_dsql_prepare(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
	void log_event_dsql_free(
		TraceDatabaseConnection* connection, TraceSQLStatement* statement, unsigned short option);
	void log_event_dsql_execute(
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceSQLStatement* statement,
		bool started, ntrace_result_t req_result);

	void log_event_blr_compile(
		TraceDatabaseConnection* connection,	TraceTransaction* transaction,
		TraceBLRStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
	void log_event_blr_execute(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceBLRStatement* statement, ntrace_result_t req_result);

	void log_event_dyn_execute(
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceDYNRequest* request, ntrace_counter_t time_millis,
		ntrace_result_t req_result);

	void log_event_service_attach(TraceServiceConnection* service, ntrace_result_t att_result);
	void log_event_service_start(TraceServiceConnection* service, size_t switches_length, const char* switches,
								 ntrace_result_t start_result);
	void log_event_service_query(TraceServiceConnection* service, size_t send_item_length,
								 const ntrace_byte_t* send_items, size_t recv_item_length,
								 const ntrace_byte_t* recv_items, ntrace_result_t query_result);
	void log_event_service_detach(TraceServiceConnection* service, ntrace_result_t detach_result);

	void log_event_error(TraceBaseConnection* connection, TraceStatusVector* status, const char* function);

	void log_event_sweep(TraceDatabaseConnection* connection, TraceSweepInfo* sweep, 
						 ntrace_process_state_t sweep_state);

	/* Finalize plugin. Called when database is closed by the engine */
	static ntrace_boolean_t ntrace_shutdown(const TracePlugin* tpl_plugin);

	/* Function to return error string for hook failure */
	static const char* ntrace_get_error(const TracePlugin* tpl_plugin);

	/* Create/close attachment */
	static ntrace_boolean_t ntrace_event_attach(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, ntrace_boolean_t create_db,
		ntrace_result_t att_result);
	static ntrace_boolean_t ntrace_event_detach(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, ntrace_boolean_t drop_db);

	/* Start/end transaction */
	static ntrace_boolean_t ntrace_event_transaction_start(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		size_t tpb_length, const ntrace_byte_t* tpb, ntrace_result_t tra_result);
	static ntrace_boolean_t ntrace_event_transaction_end(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		ntrace_boolean_t commit, ntrace_boolean_t retain_context, ntrace_result_t tra_result);

	/* Assignment to context variables */
	static ntrace_boolean_t ntrace_event_set_context(const struct TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceContextVariable* variable);

	/* Stored procedure executing */
	static ntrace_boolean_t ntrace_event_proc_execute(const struct TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceProcedure* procedure,
		bool started, ntrace_result_t proc_result);

	static ntrace_boolean_t ntrace_event_trigger_execute(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceTrigger* trigger,
		bool started, ntrace_result_t trig_result);

	/* DSQL statement lifecycle */
	static ntrace_boolean_t ntrace_event_dsql_prepare(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceSQLStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
	static ntrace_boolean_t ntrace_event_dsql_free(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceSQLStatement* statement, unsigned short option);
	static ntrace_boolean_t ntrace_event_dsql_execute(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceSQLStatement* statement,
		bool started, ntrace_result_t req_result);

	/* BLR requests */
	static ntrace_boolean_t ntrace_event_blr_compile(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceBLRStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
	static ntrace_boolean_t ntrace_event_blr_execute(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceBLRStatement* statement, ntrace_result_t req_result);

	/* DYN requests */
	static ntrace_boolean_t ntrace_event_dyn_execute(const TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceDYNRequest* request, ntrace_counter_t time_millis,
		ntrace_result_t req_result);

	/* Using the services */
	static ntrace_boolean_t ntrace_event_service_attach(const TracePlugin* tpl_plugin,
		TraceServiceConnection* service, ntrace_result_t att_result);
	static ntrace_boolean_t ntrace_event_service_start(const TracePlugin* tpl_plugin,
		TraceServiceConnection* service, size_t switches_length, const char* switches,
		ntrace_result_t start_result);
	static ntrace_boolean_t ntrace_event_service_query(const TracePlugin* tpl_plugin,
		TraceServiceConnection* service, size_t send_item_length,
		const ntrace_byte_t* send_items, size_t recv_item_length,
		const ntrace_byte_t* recv_items, ntrace_result_t query_result);
	static ntrace_boolean_t ntrace_event_service_detach(const TracePlugin* tpl_plugin,
		TraceServiceConnection* service, ntrace_result_t detach_result);

	static ntrace_boolean_t ntrace_event_error(const struct TracePlugin* tpl_plugin,
		TraceBaseConnection* connection, TraceStatusVector* status, const char* function);

	static ntrace_boolean_t ntrace_event_sweep(const struct TracePlugin* tpl_plugin,
		TraceDatabaseConnection* connection, TraceSweepInfo* sweep, ntrace_process_state_t sweep_state);
};

#endif // TRACEPLUGINIMPL_H
