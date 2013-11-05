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
#include "../../jrd/dsc.h"
#include "../../jrd/isc.h"
#include "../../jrd/req.h"
#include "../../jrd/svc.h"
#include "../../jrd/tra.h"
#include "../../jrd/RuntimeStatistics.h"
#include "../../jrd/trace/TraceSession.h"

namespace Jrd {

class Database;
class Attachment;
class jrd_tra;

class TraceConnectionImpl : public TraceDatabaseConnection
{
public:
	TraceConnectionImpl(const Attachment* att) :
		m_att(att)
	{}

	virtual int getConnectionID();
	virtual int getProcessID();

	virtual ntrace_connection_kind_t getKind();
	virtual const char* getDatabaseName();
	virtual const char* getUserName();
	virtual const char* getRoleName();
	virtual const char* getCharSet();
	virtual const char* getRemoteProtocol();
	virtual const char* getRemoteAddress();
	virtual int getRemoteProcessID();
	virtual const char* getRemoteProcessName();

private:
	const Attachment* const m_att;
};


class TraceTransactionImpl : public TraceTransaction
{
public:
	TraceTransactionImpl(const jrd_tra* tran, PerformanceInfo* perf = NULL) :
		m_tran(tran),
		m_perf(perf)
	{}

	virtual int getTransactionID();
	virtual bool getReadOnly();
	virtual int getWait();
	virtual ntrace_tra_isolation_t getIsolation();
	virtual PerformanceInfo* getPerf()	{ return m_perf; }

private:
	const jrd_tra* const m_tran;
	PerformanceInfo* const m_perf;
};


class TraceDYNRequestImpl : public TraceDYNRequest
{
public:
	TraceDYNRequestImpl(size_t length, const unsigned char* ddl) :
		m_ddl(ddl),
		m_length(length),
		m_text(*getDefaultMemoryPool())
	{}

	virtual const unsigned char* getData()	{ return m_ddl; }
	virtual size_t getDataLength()	{ return m_length; }
	virtual const char* getText();

private:
	static void print_dyn(void* arg, SSHORT offset, const char* line);

	const unsigned char* const m_ddl;
	const size_t m_length;
	Firebird::string m_text;
};


class BLRPrinter : public TraceBLRStatement
{
public:
	BLRPrinter(const unsigned char* blr, size_t length) :
		m_blr(blr),
		m_length(length),
		m_text(*getDefaultMemoryPool())
	{}

	virtual const unsigned char* getData()	{ return m_blr; }
	virtual size_t getDataLength()	{ return m_length; }
	virtual const char* getText();

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
		BLRPrinter(stmt->req_blr.begin(), stmt->req_blr.getCount()),
		m_stmt(stmt),
		m_perf(perf)
	{}

	virtual int getStmtID()				{ return m_stmt->req_id; }
	virtual PerformanceInfo* getPerf()	{ return m_perf; }

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

	virtual int getStmtID()				{ return 0; }
	virtual PerformanceInfo* getPerf()	{ return NULL; }
};


class TraceSQLStatementImpl : public TraceSQLStatement
{
public:
	TraceSQLStatementImpl(const dsql_req* stmt, PerformanceInfo* perf) :
		m_stmt(stmt),
		m_perf(perf),
		m_plan(NULL),
		m_inputs(*getDefaultMemoryPool(), m_stmt->req_send ? m_stmt->req_send->msg_parameters : NULL)
	{}

	~TraceSQLStatementImpl();

	virtual int getStmtID();
	virtual PerformanceInfo* getPerf();
	virtual TraceParams* getInputs();
	virtual const char* getText();
	virtual const char* getPlan();
	virtual const char* getTextUTF8();

private:
	class DSQLParamsImpl : public TraceParams
	{
	public:
		DSQLParamsImpl(Firebird::MemoryPool &pool, dsql_par* params) :
			m_params(params),
			m_descs(pool)
		{}

		virtual size_t getCount();
		virtual const dsc* getParam(size_t idx);

	private:
		void fillParams();

		dsql_par* m_params;
		Firebird::HalfStaticArray<dsc, 16> m_descs;
	};

	const dsql_req* const m_stmt;
	PerformanceInfo* const m_perf;
	char* m_plan;
	DSQLParamsImpl m_inputs;
	Firebird::string m_textUTF8;
};


class TraceFailedSQLStatement : public TraceSQLStatement
{
public:
	TraceFailedSQLStatement(Firebird::string &text) :
		m_text(text)
	{}

	virtual int getStmtID()				{ return 0; }
	virtual PerformanceInfo* getPerf()	{ return NULL; }
	virtual TraceParams* getInputs()	{ return NULL; }
	virtual const char* getText()		{ return m_text.c_str(); }
	virtual const char* getPlan()		{ return ""; }
	virtual const char* getTextUTF8();

private:
	Firebird::string &m_text;
	Firebird::string m_textUTF8;
};


class TraceContextVarImpl : public TraceContextVariable
{
public:
	TraceContextVarImpl(const char* ns, const char* name, const char* value) :
		m_namespace(ns),
		m_name(name),
		m_value(value)
	{}

	virtual const char* getNameSpace()	{ return m_namespace; }
	virtual const char* getVarName()	{ return m_name; }
	virtual const char* getVarValue()	{ return m_value; }

private:
	const char* const m_namespace;
	const char* const m_name;
	const char* const m_value;
};

class TraceProcedureImpl : public TraceProcedure
{
public:
	TraceProcedureImpl(const jrd_req* request, PerformanceInfo* perf) :
		m_request(request),
		m_perf(perf),
		m_inputs(*getDefaultMemoryPool(), request->req_proc_caller, request->req_proc_inputs)
	{}

	virtual const char* getProcName()	{ return m_request->req_procedure->prc_name.c_str(); }
	virtual TraceParams* getInputs()	{ return &m_inputs; }
	virtual PerformanceInfo* getPerf()	{ return m_perf; };

private:
	class JrdParamsImpl : public TraceParams
	{
	public:
		JrdParamsImpl(Firebird::MemoryPool& pool, const jrd_req* request, const jrd_nod* params) :
			m_request(request),
			m_params(params),
			m_descs(pool)
		{}

		virtual size_t getCount();
		virtual const dsc* getParam(size_t idx);

	private:
		void fillParams();

		const jrd_req* m_request;
		const jrd_nod* m_params;
		Firebird::HalfStaticArray<dsc, 16> m_descs;
	};

	const jrd_req* const m_request;
	PerformanceInfo* const m_perf;
	JrdParamsImpl m_inputs;
};


class TraceTriggerImpl : public TraceTrigger
{
public:
	TraceTriggerImpl(const jrd_req* trig, SSHORT which, PerformanceInfo* perf) :
	  m_trig(trig),
	  m_which(which),
	  m_perf(perf)
	{}

	virtual const char* getTriggerName();
	virtual const char* getRelationName();
	virtual int getAction()				{ return m_trig->req_trigger_action; }
	virtual int getWhich()				{ return m_which; }
	virtual PerformanceInfo* getPerf()	{ return m_perf; }

private:
	const jrd_req* const m_trig;
	const SSHORT m_which;
	PerformanceInfo* const m_perf;
};


class TraceServiceImpl : public TraceServiceConnection
{
public:
	TraceServiceImpl(const Service* svc) :
		m_svc(svc)
	{}

	virtual ntrace_service_t getServiceID();
	virtual const char* getServiceMgr();
	virtual const char* getServiceName();

	virtual ntrace_connection_kind_t getKind();
	virtual int getProcessID();
	virtual const char* getUserName();
	virtual const char* getRoleName();
	virtual const char* getCharSet();
	virtual const char* getRemoteProtocol();
	virtual const char* getRemoteAddress();
	virtual int getRemoteProcessID();
	virtual const char* getRemoteProcessName();

private:
	const Service* const m_svc;
};


class TraceRuntimeStats
{
public:
	TraceRuntimeStats(Database* dbb, RuntimeStatistics* baseline, RuntimeStatistics* stats,
		SINT64 clock, SINT64 records_fetched);

	PerformanceInfo* getPerf()	{ return &m_info; }

private:
	PerformanceInfo m_info;
	TraceCountsArray m_counts;
	static SINT64 m_dummy_counts[RuntimeStatistics::TOTAL_ITEMS];	// Zero-initialized array with zero counts
};


class TraceInitInfoImpl : public TraceInitInfo
{
public:
	TraceInitInfoImpl(const Firebird::TraceSession &session, const Attachment* att,
					const char* filename) :
		m_session(session),
		m_trace_conn(att),
		m_filename(filename),
		m_attachment(att)
	{
		if (m_attachment && !m_attachment->att_filename.empty()) {
			m_filename = m_attachment->att_filename.c_str();
		}
		m_logWriter = NULL;
	}

	virtual const char* getConfigText()			{ return m_session.ses_config.c_str(); }
	virtual int getTraceSessionID()				{ return m_session.ses_id; }
	virtual const char* getTraceSessionName()	{ return m_session.ses_name.c_str(); }

	virtual const char* getFirebirdRootDirectory();
	virtual const char* getDatabaseName()		{ return m_filename; }

	virtual TraceDatabaseConnection* getConnection()
	{
		if (m_attachment)
			return &m_trace_conn;

		return NULL;
	}

	virtual TraceLogWriter* getLogWriter();

private:
	const Firebird::TraceSession& m_session;
	TraceLogWriter* m_logWriter;
	TraceConnectionImpl m_trace_conn;
	const char* m_filename;
	const Attachment* const m_attachment;
};


class TraceStatusVectorImpl : public TraceStatusVector
{
public:
	TraceStatusVectorImpl(const ISC_STATUS* status) :
		m_status(status)
	{
	}

	virtual bool hasError() 
	{
		return m_status && (m_status[1] != 0);
	}

	virtual bool hasWarning()
	{
		return m_status && (m_status[1] == 0) && (m_status[2] == isc_arg_warning);
	}

	virtual const ISC_STATUS* getStatus() 
	{
		return m_status;
	}

	virtual const char* getText();

private:
	const ISC_STATUS* m_status;
	Firebird::string m_error;
};

class TraceSweepImpl : public TraceSweepInfo
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

	virtual ISC_LONG getOIT()	{ return m_oit; };
	virtual ISC_LONG getOST()	{ return m_ost; };
	virtual ISC_LONG getOAT()	{ return m_oat; };
	virtual ISC_LONG getNext()	{ return m_next; };
	virtual PerformanceInfo* getPerf()	{ return m_perf; };

private:
	SLONG m_oit;
	SLONG m_ost;
	SLONG m_oat;
	SLONG m_next;
	PerformanceInfo* m_perf;
};

} // namespace Jrd

#endif // JRD_TRACE_OBJECTS_H
