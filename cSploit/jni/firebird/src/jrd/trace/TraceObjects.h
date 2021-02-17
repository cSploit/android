/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceObjects.h
 *	DESCRIPTION:	Trace API manager support
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_TRACE_OBJECTS_H
#define JRD_TRACE_OBJECTS_H

#include <time.h>
#include "../../common/classes/array.h"
#include "../../common/classes/fb_string.h"
#include "../../dsql/dsql.h"
#include "../../jrd/ntrace.h"
#include "../../common/dsc.h"
#include "../../common/isc_s_proto.h"
#include "../../jrd/req.h"
#include "../../jrd/svc.h"
#include "../../jrd/tra.h"
#include "../../jrd/Function.h"
#include "../../jrd/RuntimeStatistics.h"
#include "../../jrd/trace/TraceSession.h"
#include "../../common/classes/ImplementHelper.h"

//// TODO: DDL triggers, packages and external procedures and functions support
namespace Jrd {

class Database;
class Attachment;
class jrd_tra;

class TraceConnectionImpl : public Firebird::AutoIface<TraceDatabaseConnection, FB_TRACE_CONNECTION_VERSION>
{
public:
	TraceConnectionImpl(const Attachment* att) :
		m_att(att)
	{}

	// TraceBaseConnection implementation
	virtual ntrace_connection_kind_t FB_CARG getKind();
	virtual int FB_CARG getProcessID();
	virtual const char* FB_CARG getUserName();
	virtual const char* FB_CARG getRoleName();
	virtual const char* FB_CARG getCharSet();
	virtual const char* FB_CARG getRemoteProtocol();
	virtual const char* FB_CARG getRemoteAddress();
	virtual int FB_CARG getRemoteProcessID();
	virtual const char* FB_CARG getRemoteProcessName();

	// TraceDatabaseConnection implementation
	virtual int FB_CARG getConnectionID();
	virtual const char* FB_CARG getDatabaseName();
private:
	const Attachment* const m_att;
};


class TraceTransactionImpl : public Firebird::AutoIface<TraceTransaction, FB_TRACE_TRANSACTION_VERSION>
{
public:
	TraceTransactionImpl(const jrd_tra* tran, PerformanceInfo* perf = NULL) :
		m_tran(tran),
		m_perf(perf)
	{}

	// TraceTransaction implementation
	virtual unsigned FB_CARG getTransactionID();
	virtual bool FB_CARG getReadOnly();
	virtual int FB_CARG getWait();
	virtual ntrace_tra_isolation_t FB_CARG getIsolation();
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; }

private:
	const jrd_tra* const m_tran;
	PerformanceInfo* const m_perf;
};


class BLRPrinter : public Firebird::AutoIface<TraceBLRStatement, FB_TRACE_BLR_STATEMENT_VERSION>
{
public:
	BLRPrinter(const unsigned char* blr, size_t length) :
		m_blr(blr),
		m_length(length),
		m_text(*getDefaultMemoryPool())
	{}

	// TraceBLRStatement implementation
	virtual const unsigned char* FB_CARG getData()	{ return m_blr; }
	virtual size_t FB_CARG getDataLength()	{ return m_length; }
	virtual const char* FB_CARG getText();

private:
	static void print_blr(void* arg, SSHORT offset, const char* line);

	const unsigned char* const m_blr;
	const size_t m_length;
	Firebird::string m_text;
};


class TraceBLRStatementImpl : public BLRPrinter
{
public:
	TraceBLRStatementImpl(const jrd_req* stmt, PerformanceInfo* perf) :
		BLRPrinter(stmt->getStatement()->blr.begin(), stmt->getStatement()->blr.getCount()),
		m_stmt(stmt),
		m_perf(perf)
	{}

	virtual int FB_CARG getStmtID()				{ return m_stmt->req_id; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; }

private:
	const jrd_req* const m_stmt;
	PerformanceInfo* const m_perf;
};


class TraceFailedBLRStatement : public BLRPrinter
{
public:
	TraceFailedBLRStatement(const unsigned char* blr, size_t length) :
		BLRPrinter(blr, length)
	{}

	virtual int FB_CARG getStmtID()				{ return 0; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return NULL; }
};


class TraceSQLStatementImpl : public Firebird::AutoIface<TraceSQLStatement, FB_TRACE_SQL_STATEMENT_VERSION>
{
public:
	TraceSQLStatementImpl(const dsql_req* stmt, PerformanceInfo* perf) :
		m_stmt(stmt),
		m_perf(perf),
		m_inputs(*getDefaultMemoryPool(), m_stmt)
	{}

	// TraceSQLStatement implementation
	virtual int FB_CARG getStmtID();
	virtual PerformanceInfo* FB_CARG getPerf();
	virtual TraceParams* FB_CARG getInputs();
	virtual const char* FB_CARG getText();
	virtual const char* FB_CARG getPlan();
	virtual const char* FB_CARG getTextUTF8();

private:
	class DSQLParamsImpl : public Firebird::AutoIface<TraceParams, FB_TRACE_PARAMS_VERSION>
	{
	public:
		DSQLParamsImpl(Firebird::MemoryPool& pool, const dsql_req* const stmt) :
			m_stmt(stmt),
			m_params(NULL),
			m_descs(pool)
		{
			const dsql_msg* msg = m_stmt->getStatement()->getSendMsg();
			if (msg)
				m_params = &msg->msg_parameters;
		}

		virtual size_t FB_CARG getCount();
		virtual const dsc* FB_CARG getParam(size_t idx);

	private:
		void fillParams();

		const dsql_req* const m_stmt;
		const Firebird::Array<dsql_par*>* m_params;
		Firebird::HalfStaticArray<dsc, 16> m_descs;
	};

	const dsql_req* const m_stmt;
	PerformanceInfo* const m_perf;
	Firebird::string m_plan;
	DSQLParamsImpl m_inputs;
	Firebird::string m_textUTF8;
};


class TraceFailedSQLStatement : public Firebird::AutoIface<TraceSQLStatement, FB_TRACE_SQL_STATEMENT_VERSION>
{
public:
	TraceFailedSQLStatement(Firebird::string& text) :
		m_text(text)
	{}

	// TraceSQLStatement implementation
	virtual int FB_CARG getStmtID()				{ return 0; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return NULL; }
	virtual TraceParams* FB_CARG getInputs()	{ return NULL; }
	virtual const char* FB_CARG getText()		{ return m_text.c_str(); }
	virtual const char* FB_CARG getPlan()		{ return ""; }
	virtual const char* FB_CARG getTextUTF8();

private:
	Firebird::string& m_text;
	Firebird::string m_textUTF8;
};


class TraceContextVarImpl : public Firebird::AutoIface<TraceContextVariable, FB_TRACE_CONTEXT_VARIABLE_VERSION>
{
public:
	TraceContextVarImpl(const char* ns, const char* name, const char* value) :
		m_namespace(ns),
		m_name(name),
		m_value(value)
	{}

	// TraceContextVariable implementation
	virtual const char* FB_CARG getNameSpace()	{ return m_namespace; }
	virtual const char* FB_CARG getVarName()	{ return m_name; }
	virtual const char* FB_CARG getVarValue()	{ return m_value; }

private:
	const char* const m_namespace;
	const char* const m_name;
	const char* const m_value;
};


// forward declaration
class TraceDescriptors;

class TraceParamsImpl : public Firebird::AutoIface<TraceParams, FB_TRACE_PARAMS_VERSION>
{
public:
	explicit TraceParamsImpl(TraceDescriptors *descs) :
		m_descs(descs)
	{}

	// TraceParams implementation
	virtual size_t FB_CARG getCount();
	virtual const dsc* FB_CARG getParam(size_t idx);

private:
	TraceDescriptors* m_descs;
};


class TraceDescriptors
{
public:
	explicit TraceDescriptors(Firebird::MemoryPool& pool) :
		m_descs(pool),
		m_traceParams(this)
	{
	}

	size_t getCount()
	{
		fillParams();
		return m_descs.getCount();
	}

	const dsc* getParam(size_t idx)
	{
		fillParams();

		if (/*idx >= 0 &&*/ idx < m_descs.getCount())
			return &m_descs[idx];

		return NULL;
	}

	operator TraceParams* ()
	{
		return &m_traceParams;
	}

protected:
	virtual void fillParams() = 0;

	Firebird::HalfStaticArray<dsc, 16> m_descs;

private:
	TraceParamsImpl	m_traceParams;
};


class TraceDscFromValues : public TraceDescriptors
{
public:
	TraceDscFromValues(Firebird::MemoryPool& pool, jrd_req* request, const ValueListNode* params) :
		TraceDescriptors(pool),
		m_request(request),
		m_params(params)
	{}

protected:
	virtual void fillParams();

private:
	jrd_req* m_request;
	const ValueListNode* m_params;
};


class TraceDscFromMsg : public TraceDescriptors
{
public:
	TraceDscFromMsg(Firebird::MemoryPool& pool, const Format* format,
		const UCHAR* inMsg, ULONG inMsgLength) :
		TraceDescriptors(pool),
		m_format(format),
		m_inMsg(inMsg),
		m_inMsgLength(inMsgLength)
	{}

protected:
	virtual void fillParams();

private:
	const Format* m_format;
	const UCHAR* m_inMsg;
	ULONG m_inMsgLength;
};


class TraceDscFromDsc : public TraceDescriptors
{
public:
	TraceDscFromDsc(Firebird::MemoryPool& pool, const dsc* desc) :
		TraceDescriptors(pool)
	{
		if (desc)
			m_descs.add(*desc);
		else
		{
			m_descs.grow(1);
			m_descs[0].setNull();
		}
	}

protected:
	virtual void fillParams() {}
};


class TraceProcedureImpl : public Firebird::AutoIface<TraceProcedure, FB_TRACE_PROCEDURE_VERSION>
{
public:
	TraceProcedureImpl(jrd_req* request, PerformanceInfo* perf) :
		m_request(request),
		m_perf(perf),
		m_inputs(*getDefaultMemoryPool(), request->req_proc_caller, request->req_proc_inputs),
		m_name(m_request->getStatement()->procedure->getName().toString())

	{}

	// TraceProcedure implementation
	virtual const char* FB_CARG getProcName()
	{
		return m_name.c_str();
	}

	virtual TraceParams* FB_CARG getInputs()	{ return m_inputs; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; };

private:
	jrd_req* const m_request;
	PerformanceInfo* const m_perf;
	TraceDscFromValues m_inputs;
	Firebird::string m_name;
};


class TraceFunctionImpl : public Firebird::AutoIface<TraceFunction, FB_TRACE_FUNCTION_VERSION>
{
public:
	TraceFunctionImpl(jrd_req* request, TraceParams* inputs, PerformanceInfo* perf, const dsc* value) :
		m_request(request),
		m_perf(perf),
		m_inputs(inputs),
		m_value(*getDefaultMemoryPool(), value),
		m_name(m_request->getStatement()->function->getName().toString())
	{}

	// TraceFunction implementation
	virtual const char* FB_CARG getFuncName()
	{
		return m_name.c_str();
	}

	virtual TraceParams* FB_CARG getInputs()	{ return m_inputs; }
	virtual TraceParams* FB_CARG getResult()	{ return m_value; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; };

private:
	jrd_req* const m_request;
	PerformanceInfo* const m_perf;
	TraceParams* m_inputs;
	TraceDscFromDsc m_value;
	Firebird::string m_name;
};


class TraceTriggerImpl : public Firebird::AutoIface<TraceTrigger, FB_TRACE_TRIGGER_VERSION>
{
public:
	TraceTriggerImpl(const jrd_req* trig, SSHORT which, PerformanceInfo* perf) :
	  m_trig(trig),
	  m_which(which),
	  m_perf(perf)
	{}

	// TraceTrigger implementation
	virtual const char* FB_CARG getTriggerName();
	virtual const char* FB_CARG getRelationName();
	virtual int FB_CARG getAction()				{ return m_trig->req_trigger_action; }
	virtual int FB_CARG getWhich()				{ return m_which; }
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; }

private:
	const jrd_req* const m_trig;
	const SSHORT m_which;
	PerformanceInfo* const m_perf;
};


class TraceServiceImpl : public Firebird::AutoIface<TraceServiceConnection, FB_TRACE_SERVICE_VERSION>
{
public:
	TraceServiceImpl(const Service* svc) :
		m_svc(svc)
	{}

	// TraceBaseConnection implementation
	virtual ntrace_connection_kind_t FB_CARG getKind();
	virtual const char* FB_CARG getUserName();
	virtual const char* FB_CARG getRoleName();
	virtual const char* FB_CARG getCharSet();
	virtual int FB_CARG getProcessID();
	virtual const char* FB_CARG getRemoteProtocol();
	virtual const char* FB_CARG getRemoteAddress();
	virtual int FB_CARG getRemoteProcessID();
	virtual const char* FB_CARG getRemoteProcessName();

	// TraceServiceConnection implementation
	virtual ntrace_service_t FB_CARG getServiceID();
	virtual const char* FB_CARG getServiceMgr();
	virtual const char* FB_CARG getServiceName();
private:
	const Service* const m_svc;
};


class TraceRuntimeStats
{
public:
	TraceRuntimeStats(Attachment* att, RuntimeStatistics* baseline, RuntimeStatistics* stats,
		SINT64 clock, SINT64 records_fetched);

	PerformanceInfo* getPerf()	{ return &m_info; }

private:
	PerformanceInfo m_info;
	TraceCountsArray m_counts;
	static SINT64 m_dummy_counts[RuntimeStatistics::TOTAL_ITEMS];	// Zero-initialized array with zero counts
};


class TraceInitInfoImpl : public Firebird::AutoIface<TraceInitInfo, FB_TRACE_INIT_INFO_VERSION>
{
public:
	TraceInitInfoImpl(const Firebird::TraceSession& session, const Attachment* att,
					const char* filename) :
		m_session(session),
		m_trace_conn(att),
		m_filename(filename),
		m_attachment(att)
	{
		if (m_attachment && !m_attachment->att_filename.empty()) {
			m_filename = m_attachment->att_filename.c_str();
		}
	}

	// TraceInitInfo implementation
	virtual const char* FB_CARG getConfigText()			{ return m_session.ses_config.c_str(); }
	virtual int FB_CARG getTraceSessionID()				{ return m_session.ses_id; }
	virtual const char* FB_CARG getTraceSessionName()	{ return m_session.ses_name.c_str(); }

	virtual const char* FB_CARG getFirebirdRootDirectory();
	virtual const char* FB_CARG getDatabaseName()		{ return m_filename; }

	virtual TraceDatabaseConnection* FB_CARG getConnection()
	{
		if (m_attachment)
			return &m_trace_conn;

		return NULL;
	}

	virtual TraceLogWriter* FB_CARG getLogWriter();

private:
	const Firebird::TraceSession& m_session;
	Firebird::RefPtr<TraceLogWriter> m_logWriter;
	TraceConnectionImpl m_trace_conn;
	const char* m_filename;
	const Attachment* const m_attachment;
};


class TraceStatusVectorImpl : public Firebird::AutoIface<TraceStatusVector, FB_TRACE_STATUS_VERSION>
{
public:
	explicit TraceStatusVectorImpl(const ISC_STATUS* status) :
		m_status(status)
	{
	}

	virtual bool FB_CARG hasError()
	{
		return m_status && (m_status[1] != 0);
	}

	virtual bool FB_CARG hasWarning()
	{
		return m_status && (m_status[1] == 0) && (m_status[2] == isc_arg_warning);
	}

	virtual const ISC_STATUS* FB_CARG getStatus()
	{
		return m_status;
	}

	virtual const char* FB_CARG getText();

private:
	const ISC_STATUS* m_status;
	Firebird::string m_error;
};

class TraceSweepImpl : public Firebird::AutoIface<TraceSweepInfo, FB_TRACE_SWEEP_INFO_VERSION>
{
public:
	TraceSweepImpl()
	{
		m_oit = 0;
		m_ost = 0;
		m_oat = 0;
		m_next = 0;
		m_perf = 0;
	}

	void update(const Ods::header_page* header)
	{
		m_oit = header->hdr_oldest_transaction;
		m_ost = header->hdr_oldest_snapshot;
		m_oat = header->hdr_oldest_active;
		m_next = header->hdr_next_transaction;
	}

	void setPerf(PerformanceInfo* perf)
	{
		m_perf = perf;
	}

	virtual TraNumber FB_CARG getOIT()	{ return m_oit; };
	virtual TraNumber FB_CARG getOST()	{ return m_ost; };
	virtual TraNumber FB_CARG getOAT()	{ return m_oat; };
	virtual TraNumber FB_CARG getNext()	{ return m_next; };
	virtual PerformanceInfo* FB_CARG getPerf()	{ return m_perf; };

private:
	TraNumber m_oit;
	TraNumber m_ost;
	TraNumber m_oat;
	TraNumber m_next;
	PerformanceInfo* m_perf;
};

} // namespace Jrd

#endif // JRD_TRACE_OBJECTS_H
