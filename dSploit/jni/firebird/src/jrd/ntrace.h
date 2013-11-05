/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ntrace.h
 *	DESCRIPTION:	Trace API header
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
 *
 */

#ifndef FIREBIRD_NTRACE_H
#define FIREBIRD_NTRACE_H

// to define SINT64
#include "../jrd/common.h"

/* Version of API, used for version fields in TracePlugin structure */
#define NTRACE_VERSION 4

// plugin entry point
static const char* const NTRACE_ATTACH = "trace_create";

/* Database objects*/

struct PerformanceInfo;

enum ntrace_connection_kind_t
{
	connection_database	= 1,
	connection_service
};

enum ntrace_process_state_t
{
	process_state_started	= 1,
	process_state_finished,
	process_state_failed,
	process_state_progress
};

class TraceBaseConnection
{
public:
	virtual ntrace_connection_kind_t getKind() = 0;

	virtual int getProcessID() = 0;
	virtual const char* getUserName() = 0;
	virtual const char* getRoleName() = 0;
	virtual const char* getCharSet() = 0;
	virtual const char* getRemoteProtocol() = 0;
	virtual const char* getRemoteAddress() = 0;
	virtual int getRemoteProcessID() = 0;
	virtual const char* getRemoteProcessName() = 0;
};

class TraceDatabaseConnection : public TraceBaseConnection
{
public:
	virtual int getConnectionID() = 0;
	virtual const char* getDatabaseName() = 0;
};

enum ntrace_tra_isolation_t
{
	tra_iso_consistency = 1,
	tra_iso_concurrency,
	tra_iso_read_committed_recver,
	tra_iso_read_committed_norecver
};

class TraceTransaction
{
public:
	virtual int getTransactionID() = 0;
	virtual bool getReadOnly() = 0;
	virtual int getWait() = 0;
	virtual ntrace_tra_isolation_t getIsolation() = 0;
	virtual PerformanceInfo* getPerf() = 0;
};

typedef int ntrace_relation_t;

class TraceParams
{
public:
	virtual size_t getCount() = 0;
	virtual const struct dsc* getParam(size_t idx) = 0;
};

class TraceStatement
{
public:
	virtual int getStmtID() = 0;
	virtual PerformanceInfo* getPerf() = 0;
};

class TraceSQLStatement : public TraceStatement
{
public:
	virtual const char* getText() = 0;
	virtual const char* getPlan() = 0;
	virtual TraceParams* getInputs() = 0;
	virtual const char* getTextUTF8() = 0;
};

class TraceBLRStatement : public TraceStatement
{
public:
	virtual const unsigned char* getData() = 0;
	virtual size_t getDataLength() = 0;
	virtual const char* getText() = 0;
};

class TraceDYNRequest
{
public:
	virtual const unsigned char* getData() = 0;
	virtual size_t getDataLength() = 0;
	virtual const char* getText() = 0;
};

class TraceContextVariable
{
public:
	virtual const char* getNameSpace() = 0;
	virtual const char* getVarName() = 0;
	virtual const char* getVarValue() = 0;
};

class TraceProcedure
{
public:
	virtual const char* getProcName() = 0;
	virtual TraceParams* getInputs() = 0;
	virtual PerformanceInfo* getPerf() = 0;
};

class TraceTrigger
{
public:
	virtual const char* getTriggerName() = 0;
	virtual const char* getRelationName() = 0;
	virtual int getAction() = 0;
	virtual int getWhich() = 0;
	virtual PerformanceInfo* getPerf() = 0;
};

typedef void* ntrace_service_t;

class TraceServiceConnection  : public TraceBaseConnection
{
public:
	virtual ntrace_service_t getServiceID() = 0;
	virtual const char* getServiceMgr() = 0;
	virtual const char* getServiceName() = 0;
};

class TraceStatusVector
{
public:
	virtual bool hasError() = 0;
	virtual bool hasWarning() = 0;
	virtual const ISC_STATUS* getStatus() = 0;
	virtual const char* getText() = 0;
};

class TraceSweepInfo
{
public:
	virtual ISC_LONG getOIT() = 0;
	virtual ISC_LONG getOST() = 0;
	virtual ISC_LONG getOAT() = 0;
	virtual ISC_LONG getNext() = 0;
	virtual PerformanceInfo* getPerf() = 0;
};

/* Plugin-specific argument. Passed by the engine to each hook */
typedef void* ntrace_object_t;

/* Structure version*/
typedef int ntrace_version_t;

/* Boolean type */
typedef int ntrace_boolean_t;

/* Performance counter */
typedef SINT64 ntrace_counter_t;

/* Used for arrays with binary data */
typedef unsigned char ntrace_byte_t;


/* Event completion: 0 - successful, 1 - unsuccessful, 2 - unauthorized access */
enum ntrace_result_t
{
	res_successful		= 0,
	res_failed			= 1,
	res_unauthorized	= 2
};

enum ntrace_trigger_type_t
{
	trg_all				= 0,
	trg_before			= 1,
	trg_after			= 2
};

const int DBB_max_rel_count = 8; // must be the same as DBB_max_count from jrd.h

/* Performance counters for entire database */

enum {
	DBB_fetches_count = 0,
	DBB_reads_count,
	DBB_marks_count,
	DBB_writes_count,
	DBB_max_dbb_count
};

/* Performance counters for individual table */
struct TraceCounts
{
	ntrace_relation_t	trc_relation_id; /* Relation ID */
	const char*			trc_relation_name; /* Relation name */
	const ntrace_counter_t*	trc_counters; /* Pointer to allow easy addition of new counters */
};

/* Performance statistics for operation */
struct PerformanceInfo
{
	ntrace_counter_t pin_time;		/* Total operation time in milliseconds */
	ntrace_counter_t* pin_counters;	/* Pointer to allow easy addition of new counters */

	size_t pin_count;				/* Number of relations involved in analysis */
	struct TraceCounts* pin_tables; /* Pointer to array with table stats */

	ntrace_counter_t pin_records_fetched;	// records fetched from statement/procedure
};

/* Get error string for hook failure that happened */
typedef const char* (*ntrace_get_error_t)(const struct TracePlugin* tpl_plugin);

/* Finalize plugin interface for this particular database */
typedef ntrace_boolean_t (*ntrace_shutdown_t)(const struct TracePlugin* tpl_plugin);

/* Create/close attachment */
typedef ntrace_boolean_t (*ntrace_event_attach_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, ntrace_boolean_t create_db, ntrace_result_t att_result);
typedef ntrace_boolean_t (*ntrace_event_detach_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, ntrace_boolean_t drop_db);

/* Start/end transaction */
typedef ntrace_boolean_t (*ntrace_event_transaction_start_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	size_t tpb_length, const ntrace_byte_t* tpb, ntrace_result_t tra_result);
typedef ntrace_boolean_t (*ntrace_event_transaction_end_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	ntrace_boolean_t commit, ntrace_boolean_t retain_context, ntrace_result_t tra_result);

/* Assignment to context variables */
typedef ntrace_boolean_t (*ntrace_event_set_context_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceContextVariable* variable);

/* Stored procedure and triggers executing */
typedef	ntrace_boolean_t (*ntrace_event_proc_execute_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceProcedure* procedure,
	bool started, ntrace_result_t proc_result);

typedef ntrace_boolean_t (*ntrace_event_trigger_execute_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceTrigger* trigger,
	bool started, ntrace_result_t trig_result);

/* DSQL statement lifecycle */
typedef ntrace_boolean_t (*ntrace_event_dsql_prepare_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceSQLStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
typedef ntrace_boolean_t (*ntrace_event_dsql_free_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceSQLStatement* statement, unsigned short option);
typedef ntrace_boolean_t (*ntrace_event_dsql_execute_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction, TraceSQLStatement* statement,
	bool started, ntrace_result_t req_result);

/* BLR requests */
typedef ntrace_boolean_t (*ntrace_event_blr_compile_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceBLRStatement* statement, ntrace_counter_t time_millis, ntrace_result_t req_result);
typedef ntrace_boolean_t (*ntrace_event_blr_execute_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceBLRStatement* statement, ntrace_result_t req_result);

/* DYN requests */
typedef ntrace_boolean_t (*ntrace_event_dyn_execute_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceTransaction* transaction,
	TraceDYNRequest* request, ntrace_counter_t time_millis,
	ntrace_result_t req_result);

/* Using the services */
typedef ntrace_boolean_t (*ntrace_event_service_attach_t)(const struct TracePlugin* tpl_plugin,
	TraceServiceConnection* service, ntrace_result_t att_result);
typedef ntrace_boolean_t (*ntrace_event_service_start_t)(const struct TracePlugin* tpl_plugin,
	TraceServiceConnection* service, size_t switches_length, const char* switches,
	ntrace_result_t start_result);
typedef ntrace_boolean_t (*ntrace_event_service_query_t)(const struct TracePlugin* tpl_plugin,
	TraceServiceConnection* service, size_t send_item_length,
	const ntrace_byte_t* send_items, size_t recv_item_length,
	const ntrace_byte_t* recv_items, ntrace_result_t query_result);
typedef ntrace_boolean_t (*ntrace_event_service_detach_t)(const struct TracePlugin* tpl_plugin,
	TraceServiceConnection* service, ntrace_result_t detach_result);

/* Errors happened */
typedef ntrace_boolean_t (*ntrace_event_error_t)(const struct TracePlugin* tpl_plugin,
	TraceBaseConnection* connection, TraceStatusVector* status, const char* function);

/* Sweep activity */
typedef ntrace_boolean_t (*ntrace_event_sweep_t)(const struct TracePlugin* tpl_plugin,
	TraceDatabaseConnection* connection, TraceSweepInfo* sweep, ntrace_process_state_t sweep_state);


/* API of trace plugin. Used to deliver notifications for each database */
struct TracePlugin
{
	/* API version */
	ntrace_version_t tpl_version;
	/* Driver-specific object pointer */
	ntrace_object_t tpl_object;

	/* Destroy plugin. Called when database is closed by the engine */
	ntrace_shutdown_t tpl_shutdown;

	/* Function to return error string for hook failure */
	ntrace_get_error_t tpl_get_error;


	/* Events supported */
	ntrace_event_attach_t tpl_event_attach;
	ntrace_event_detach_t tpl_event_detach;
	ntrace_event_transaction_start_t tpl_event_transaction_start;
	ntrace_event_transaction_end_t tpl_event_transaction_end;

	ntrace_event_proc_execute_t tpl_event_proc_execute;
	ntrace_event_trigger_execute_t tpl_event_trigger_execute;
	ntrace_event_set_context_t tpl_event_set_context;

	ntrace_event_dsql_prepare_t tpl_event_dsql_prepare;
	ntrace_event_dsql_free_t tpl_event_dsql_free;
	ntrace_event_dsql_execute_t tpl_event_dsql_execute;

	ntrace_event_blr_compile_t tpl_event_blr_compile;
	ntrace_event_blr_execute_t tpl_event_blr_execute;
	ntrace_event_dyn_execute_t tpl_event_dyn_execute;

	ntrace_event_service_attach_t tpl_event_service_attach;
	ntrace_event_service_start_t tpl_event_service_start;
	ntrace_event_service_query_t tpl_event_service_query;
	ntrace_event_service_detach_t tpl_event_service_detach;

	ntrace_event_error_t tpl_event_error;

	ntrace_event_sweep_t tpl_event_sweep;

	/* Some space for future extension of Trace API interface,
       must be zero-initialized by the plugin */
	void* reserved_for_interface[22];

	/* Some space which may be freely used by Trace API driver.
       If driver needs more space it may allocate and return larger TracePlugin structure. */
	void* reserved_for_driver[1];
};

class TraceLogWriter
{
public:
	virtual size_t write(const void* buf, size_t size) = 0;
	virtual void release() = 0;

	virtual ~TraceLogWriter() { }
};

class TraceInitInfo
{
public:
	virtual const char* getConfigText() = 0;
	virtual int getTraceSessionID() = 0;
	virtual const char* getTraceSessionName() = 0;
	virtual const char* getFirebirdRootDirectory() = 0;
	virtual const char* getDatabaseName() = 0;
	virtual TraceDatabaseConnection* getConnection() = 0;
	virtual TraceLogWriter* getLogWriter() = 0;

	virtual ~TraceInitInfo() { }
};

/* Trace API plugin entrypoint type */
typedef ntrace_boolean_t (*ntrace_attach_t)(const TraceInitInfo* initInfo, const TracePlugin** plugin);

#endif	// FIREBIRD_NTRACE_H
