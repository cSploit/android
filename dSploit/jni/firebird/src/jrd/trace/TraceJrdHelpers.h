/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceJrdHelpers.h
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

#ifndef JRD_TRACE_JRD_HELPERS_H
#define JRD_TRACE_JRD_HELPERS_H

#include "../../jrd/jrd.h"
#include "../../jrd/trace/TraceObjects.h"

namespace Jrd {

class TraceTransactionEnd
{
public:
	TraceTransactionEnd(jrd_tra* transaction, bool commit, bool retain) :
		m_commit(commit),
		m_retain(retain),
		m_transaction(transaction),
		m_baseline(NULL)
	{
		Attachment* attachment = m_transaction->tra_attachment;
		m_need_trace = attachment->att_trace_manager->needs().event_transaction_end;
		if (!m_need_trace)
			return;

		m_start_clock = fb_utils::query_performance_counter();
		MemoryPool* pool = MemoryPool::getContextPool();
		m_baseline = FB_NEW(*pool) RuntimeStatistics(*pool, m_transaction->tra_stats);
	}

	~TraceTransactionEnd()
	{
		finish(res_failed);
	}

	void finish(ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;

		const Attachment* attachment = m_transaction->tra_attachment;

		TraceRuntimeStats stats(attachment->att_database, m_baseline, &m_transaction->tra_stats,
			fb_utils::query_performance_counter() - m_start_clock, 0);

		TraceConnectionImpl conn(attachment);
		TraceTransactionImpl tran(m_transaction, stats.getPerf());

		attachment->att_trace_manager->event_transaction_end(&conn, &tran, m_commit, m_retain, result);
		m_baseline = NULL;
	}

private:
	bool m_need_trace;
	const bool m_commit;
	const bool m_retain;
	jrd_tra* const m_transaction;
	SINT64 m_start_clock;
	Firebird::AutoPtr<RuntimeStatistics> m_baseline;
};


class TraceProcExecute
{
public:
	TraceProcExecute(thread_db* tdbb, jrd_req* request, jrd_req* caller, jrd_nod* inputs) :
		m_tdbb(tdbb),
		m_request(request)
	{
		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		m_need_trace = trace_mgr->needs().event_proc_execute;
		if (!m_need_trace)
			return;

		m_request->req_proc_inputs = inputs;
		m_request->req_proc_caller = caller;

		{	// scope
			TraceConnectionImpl conn(m_tdbb->getAttachment());
			TraceTransactionImpl tran(m_tdbb->getTransaction());
			TraceProcedureImpl proc(m_request, NULL);

			trace_mgr->event_proc_execute(&conn, &tran, &proc, true, res_successful);
		}

		m_start_clock = fb_utils::query_performance_counter();

		m_request->req_fetch_elapsed = 0;
		m_request->req_fetch_rowcount = 0;
		fb_assert(!m_request->req_fetch_baseline);
		m_request->req_fetch_baseline = NULL;

		MemoryPool* pool = MemoryPool::getContextPool();
		m_request->req_fetch_baseline = FB_NEW(*pool) RuntimeStatistics(*pool, m_request->req_stats);
	}

	~TraceProcExecute()
	{
		finish(false, res_failed);
	}

	void finish(bool have_cursor, ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;
		if (have_cursor)
		{
			m_request->req_fetch_elapsed = fb_utils::query_performance_counter() - m_start_clock;
			return;
		}

		Database* dbb = m_tdbb->getDatabase();
		TraceRuntimeStats stats(dbb, m_request->req_fetch_baseline, &m_request->req_stats,
			fb_utils::query_performance_counter() - m_start_clock,
			m_request->req_fetch_rowcount);

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());
		TraceProcedureImpl proc(m_request, stats.getPerf());

		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		trace_mgr->event_proc_execute(&conn, &tran, &proc,  false, result);

		m_request->req_proc_inputs = NULL;
		m_request->req_proc_caller = NULL;
		m_request->req_fetch_baseline = NULL;
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	jrd_req* const m_request;
	SINT64 m_start_clock;
};

class TraceProcFetch
{
public:
	TraceProcFetch(thread_db* tdbb, jrd_req* request) :
		m_tdbb(tdbb),
		m_request(request)
	{
		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		m_need_trace = (request->req_flags & req_proc_fetch) && trace_mgr->needs().event_proc_execute;
		if (!m_need_trace)
			return;

		m_start_clock = fb_utils::query_performance_counter();
	}

	~TraceProcFetch()
	{
		fetch(true, res_failed);
	}

	void fetch(bool eof, ntrace_result_t result)
	{
		if (!m_need_trace)
		{
			m_request->req_fetch_baseline = NULL;
			return;
		}

		m_need_trace = false;
		m_request->req_fetch_elapsed += fb_utils::query_performance_counter() - m_start_clock;
		if (!eof)
		{
			m_request->req_fetch_rowcount++;
			return;
		}

		Database* dbb = m_tdbb->getDatabase();
		TraceRuntimeStats stats(dbb, m_request->req_fetch_baseline, &m_request->req_stats,
			m_request->req_fetch_elapsed, m_request->req_fetch_rowcount);

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());
		TraceProcedureImpl proc(m_request, stats.getPerf());

		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		trace_mgr->event_proc_execute(&conn, &tran, &proc, false, result);

		m_request->req_proc_inputs = NULL;
		m_request->req_proc_caller = NULL;
		m_request->req_fetch_elapsed = 0;
		m_request->req_fetch_baseline = NULL;
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	jrd_req* const m_request;
	SINT64 m_start_clock;
};


class TraceTrigExecute
{
public:
	TraceTrigExecute(thread_db* tdbb, jrd_req* trigger, int which_trig) :
		m_tdbb(tdbb),
		m_request(trigger),
		m_which_trig(which_trig)
	{
		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		m_need_trace = !(m_request->req_flags & req_sys_trigger) &&
						trace_mgr->needs().event_trigger_execute;

		if (!m_need_trace)
			return;

		{	// scope
			TraceConnectionImpl conn(m_tdbb->getAttachment());
			TraceTransactionImpl tran(m_tdbb->getTransaction());
			TraceTriggerImpl trig(m_request, m_which_trig, NULL);

			trace_mgr->event_trigger_execute(&conn, &tran, &trig, true, res_successful);
		}

		fb_assert(!m_request->req_fetch_baseline);
		m_request->req_fetch_baseline = NULL;

		MemoryPool* pool = MemoryPool::getContextPool();
		m_request->req_fetch_baseline = FB_NEW(*pool) RuntimeStatistics(*pool, m_request->req_stats);
		m_start_clock = fb_utils::query_performance_counter();
	}

	void finish(ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;

		Database* dbb = m_tdbb->getDatabase();
		TraceRuntimeStats stats(dbb, m_request->req_fetch_baseline, &m_request->req_stats,
			fb_utils::query_performance_counter() - m_start_clock, 0);

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());
		TraceTriggerImpl trig(m_request, m_which_trig, stats.getPerf());

		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		trace_mgr->event_trigger_execute(&conn, &tran, &trig, false, result);

		m_request->req_fetch_baseline = NULL;
	}

	~TraceTrigExecute()
	{
		finish(res_failed);
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	jrd_req* const m_request;
	SINT64 m_start_clock;
	const int m_which_trig;
};


class TraceBlrCompile
{
public:
	TraceBlrCompile(thread_db* tdbb, size_t blr_length, const UCHAR* blr) :
		m_tdbb(tdbb),
		m_blr_length(blr_length),
		m_blr(blr)
	{
		Attachment* attachment = m_tdbb->getAttachment();

		m_need_trace = attachment->att_trace_manager->needs().event_blr_compile &&
			m_blr_length && m_blr &&
			!(attachment->att_flags & ATT_gstat_attachment) &&
			!(attachment->att_flags & ATT_gbak_attachment) &&
			!(attachment->att_flags & ATT_gfix_attachment);

		if (!m_need_trace)
			return;

		m_start_clock = fb_utils::query_performance_counter();
	}

	void finish(jrd_req* request, ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;

		m_start_clock = (fb_utils::query_performance_counter() - m_start_clock) * 1000 /
						 fb_utils::query_performance_frequency();
		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());

		if (request)
		{
			TraceBLRStatementImpl stmt(request, NULL);
			trace_mgr->event_blr_compile(&conn, m_tdbb->getTransaction() ? &tran : NULL, &stmt,
				m_start_clock, result);
		}
		else
		{
			TraceFailedBLRStatement stmt(m_blr, m_blr_length);
			trace_mgr->event_blr_compile(&conn, m_tdbb->getTransaction() ? &tran : NULL, &stmt,
				m_start_clock, result);
		}
	}

	~TraceBlrCompile()
	{
		finish(NULL, res_failed);
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	SINT64 m_start_clock;
	const size_t m_blr_length;
	const UCHAR* const m_blr;
};


class TraceBlrExecute
{
public:
	TraceBlrExecute(thread_db* tdbb, jrd_req* request) :
		m_tdbb(tdbb),
		m_request(request)
	{
		Attachment* attachment = m_tdbb->getAttachment();

		m_need_trace = attachment->att_trace_manager->needs().event_blr_execute &&
			!m_request->req_sql_text &&
			!(m_request->req_flags & req_internal) &&
			!(attachment->att_flags & ATT_gstat_attachment) &&
			!(attachment->att_flags & ATT_gbak_attachment) &&
			!(attachment->att_flags & ATT_gfix_attachment);

		if (!m_need_trace)
			return;

		fb_assert(!m_request->req_fetch_baseline);
		m_request->req_fetch_baseline = NULL;

		MemoryPool* pool = MemoryPool::getContextPool();
		m_request->req_fetch_baseline = FB_NEW(*pool) RuntimeStatistics(*pool, m_request->req_stats);

		m_start_clock = fb_utils::query_performance_counter();
	}

	void finish(ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;

		Database* dbb = m_tdbb->getDatabase();
		TraceRuntimeStats stats(dbb, m_request->req_fetch_baseline, &m_request->req_stats,
			fb_utils::query_performance_counter() - m_start_clock,
			m_request->req_fetch_rowcount);

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());
		TraceBLRStatementImpl stmt(m_request, stats.getPerf());

		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		trace_mgr->event_blr_execute(&conn, &tran, &stmt, result);

		m_request->req_fetch_baseline = NULL;
	}

	~TraceBlrExecute()
	{
		finish(res_failed);
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	jrd_req* const m_request;
	SINT64 m_start_clock;
};


class TraceDynExecute
{
public:
	TraceDynExecute(thread_db* tdbb, size_t ddl_length, const UCHAR* ddl) :
		m_tdbb(tdbb),
		m_ddl_length(ddl_length),
		m_ddl(ddl)
	{
		Attachment* attachment = m_tdbb->getAttachment();

		m_need_trace = attachment->att_trace_manager->needs().event_dyn_execute &&
			m_ddl_length && m_ddl;

		if (!m_need_trace)
			return;

		m_start_clock = fb_utils::query_performance_counter();
	}

	void finish(ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;

		m_start_clock = (fb_utils::query_performance_counter() - m_start_clock) * 1000 /
						 fb_utils::query_performance_frequency();

		TraceConnectionImpl conn(m_tdbb->getAttachment());
		TraceTransactionImpl tran(m_tdbb->getTransaction());
		TraceDYNRequestImpl request(m_ddl_length, m_ddl);

		TraceManager* trace_mgr = m_tdbb->getAttachment()->att_trace_manager;
		trace_mgr->event_dyn_execute(&conn, m_tdbb->getTransaction() ? &tran : NULL, &request,
			m_start_clock, result);
	}

	~TraceDynExecute()
	{
		finish(res_failed);
	}

private:
	bool m_need_trace;
	thread_db* const m_tdbb;
	SINT64 m_start_clock;
	const size_t m_ddl_length;
	const UCHAR* const m_ddl;
};


class TraceSweepEvent
{
	// implementation is at tra.cpp
public:
	TraceSweepEvent(thread_db* tdbb);

	~TraceSweepEvent();

	void update(const Ods::header_page* header)
	{
		m_sweep_info.update(header);
	}

	void beginSweepRelation(jrd_rel* relation);
	void endSweepRelation(jrd_rel* relation);

	void finish()
	{
		report(process_state_finished);
	}

private:
	void report(ntrace_process_state_t state);

	bool				m_need_trace;
	thread_db*			m_tdbb;
	TraceSweepImpl		m_sweep_info;
	SINT64				m_start_clock;
	SINT64				m_relation_clock;
	jrd_req				m_request;
};


} // namespace Jrd

#endif // JRD_TRACE_JRD_HELPERS_H
