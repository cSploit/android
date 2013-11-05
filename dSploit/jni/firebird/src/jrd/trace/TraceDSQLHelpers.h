/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceDSQLHelpers.h
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

#ifndef JRD_TRACE_DSQL_HELPERS_H
#define JRD_TRACE_DSQL_HELPERS_H

#include "../../jrd/trace/TraceObjects.h"

namespace Jrd {

class TraceDSQLPrepare
{
public:
	TraceDSQLPrepare(Attachment* attachemnt, USHORT string_length, const TEXT* string) :
		m_attachment(attachemnt),
		m_request(NULL),
		m_string_len(string_length),
		m_string(string)
	{
		m_need_trace = TraceManager::need_dsql_prepare(m_attachment);
		if (!m_need_trace)
			return;

		m_start_clock = fb_utils::query_performance_counter();

		static const char empty_string[] = "";
		if (!m_string_len || !string)
		{
			m_string = empty_string;
			m_string_len = 0;
		}
	}

	~TraceDSQLPrepare()
	{
		prepare(res_failed);
	}

	void setStatement(dsql_req* request)
	{
		m_request = request;
	}

	void prepare(ntrace_result_t result)
	{
		if (m_request) {
			m_need_trace = (m_need_trace && m_request->req_traced);
		}
		if (!m_need_trace)
			return;

		m_need_trace = false;
		const SINT64 millis = (fb_utils::query_performance_counter() - m_start_clock) * 1000 /
			fb_utils::query_performance_frequency();

		if ((result == res_successful) && m_request)
		{
			TraceSQLStatementImpl stmt(m_request, NULL);
			TraceManager::event_dsql_prepare(m_attachment, m_request->req_transaction,
				&stmt, millis, result);
		}
		else
		{
			Firebird::string str(*getDefaultMemoryPool(), m_string, m_string_len);

			TraceFailedSQLStatement stmt(str);
			TraceManager::event_dsql_prepare(m_attachment, m_request ? m_request->req_transaction : NULL,
				&stmt, millis, result);
		}
	}

private:
	bool m_need_trace;
	Attachment* const m_attachment;
	dsql_req* m_request;
	SINT64 m_start_clock;
	size_t m_string_len;
	const TEXT* m_string;
};


class TraceDSQLExecute
{
public:
	TraceDSQLExecute(Attachment* attachment, dsql_req* request) :
		m_attachment(attachment),
		m_request(request)
	{
		m_need_trace = m_request->req_traced && TraceManager::need_dsql_execute(m_attachment);
		if (!m_need_trace)
			return;

		{	// scope
			TraceSQLStatementImpl stmt(request, NULL);
			TraceManager::event_dsql_execute(m_attachment, request->req_transaction, &stmt, true, res_successful);
		}

		m_start_clock = fb_utils::query_performance_counter();

		m_request->req_fetch_elapsed = 0;
		m_request->req_fetch_rowcount = 0;
		fb_assert(!m_request->req_fetch_baseline);
		m_request->req_fetch_baseline = NULL;

		jrd_req* jrd_request = m_request->req_request;
		if (jrd_request)
		{
			MemoryPool* pool = MemoryPool::getContextPool();
			m_request->req_fetch_baseline = FB_NEW(*pool) RuntimeStatistics(*pool, jrd_request->req_stats);
		}
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

		TraceRuntimeStats stats(m_attachment->att_database, m_request->req_fetch_baseline,
			&m_request->req_request ? &m_request->req_request->req_stats : NULL,
			fb_utils::query_performance_counter() - m_start_clock,
			m_request->req_fetch_rowcount);

		TraceSQLStatementImpl stmt(m_request, stats.getPerf());
		TraceManager::event_dsql_execute(m_attachment, m_request->req_transaction, &stmt, false, result);

		m_request->req_fetch_baseline = NULL;
	}

	~TraceDSQLExecute()
	{
		finish(false, res_failed);
	}

private:
	bool m_need_trace;
	Attachment* const m_attachment;
	dsql_req* const m_request;
	SINT64 m_start_clock;
};

class TraceDSQLFetch
{
public:
	TraceDSQLFetch(Attachment* attachment, dsql_req* request) :
		m_attachment(attachment),
		m_request(request)
	{
		m_need_trace = m_request->req_traced && TraceManager::need_dsql_execute(m_attachment) && 
					   m_request->req_request && (m_request->req_request->req_flags & req_active);

		if (!m_need_trace)
		{
			m_request->req_fetch_baseline = NULL;
			return;
		}

		m_start_clock = fb_utils::query_performance_counter();
	}

	~TraceDSQLFetch()
	{
		fetch(true, res_failed);
	}

	void fetch(bool eof, ntrace_result_t result)
	{
		if (!m_need_trace)
			return;

		m_need_trace = false;
		m_request->req_fetch_elapsed += fb_utils::query_performance_counter() - m_start_clock;
		if (!eof)
		{
			m_request->req_fetch_rowcount++;
			return;
		}

		TraceRuntimeStats stats(m_attachment->att_database, m_request->req_fetch_baseline,
			m_request->req_request ? &m_request->req_request->req_stats : NULL, 
			m_request->req_fetch_elapsed,
			m_request->req_fetch_rowcount);

		TraceSQLStatementImpl stmt(m_request, stats.getPerf());

		TraceManager::event_dsql_execute(m_attachment, m_request->req_transaction,
			&stmt, false, result);

		m_request->req_fetch_elapsed = 0;
		m_request->req_fetch_baseline = NULL;
	}

private:
	bool m_need_trace;
	Attachment* const m_attachment;
	dsql_req* const m_request;
	SINT64 m_start_clock;
};


} // namespace Jrd

#endif // JRD_TRACE_DSQL_HELPERS_H
