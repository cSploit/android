/*
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../dsql/Nodes.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/par_proto.h"
#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// ------------------------------
// Data access: window expression
// ------------------------------

namespace
{
	// This stream makes possible to reuse a BufferedStream, so each usage maintains a different
	// cursor position.
	class BufferedStreamWindow : public BaseBufferedStream
	{
		struct Impure : public RecordSource::Impure
		{
			FB_UINT64 irsb_position;
		};

	public:
		BufferedStreamWindow(CompilerScratch* csb, BufferedStream* next);

		void open(thread_db* tdbb) const;
		void close(thread_db* tdbb) const;

		bool getRecord(thread_db* tdbb) const;
		bool refetchRecord(thread_db* tdbb) const;
		bool lockRecord(thread_db* tdbb) const;

		void print(thread_db* tdbb, Firebird::string& plan, bool detailed, unsigned level) const;

		void markRecursive();
		void invalidateRecords(jrd_req* request) const;

		void findUsedStreams(StreamList& streams, bool expandAll) const;
		void nullRecords(thread_db* tdbb) const;

		void locate(thread_db* tdbb, FB_UINT64 position) const
		{
			jrd_req* const request = tdbb->getRequest();
			Impure* const impure = request->getImpure<Impure>(m_impure);
			impure->irsb_position = position;
		}

		FB_UINT64 getCount(thread_db* tdbb) const
		{
			return m_next->getCount(tdbb);
		}

		FB_UINT64 getPosition(jrd_req* request) const
		{
			Impure* const impure = request->getImpure<Impure>(m_impure);
			return impure->irsb_position;
		}

	public:
		NestConst<BufferedStream> m_next;
	};

#ifdef NOT_USED_OR_REPLACED
	// Make join between outer stream and already sorted (aggregated) partition.
	class WindowJoin : public RecordSource
	{
		struct DscNull
		{
			dsc* desc;
			bool null;
		};

		struct Impure : public RecordSource::Impure
		{
			FB_UINT64 innerRecordCount;
		};

	public:
		WindowJoin(CompilerScratch* csb, RecordSource* outer, RecordSource* inner,
			const SortNode* outerKeys, const SortNode* innerKeys);

		void open(thread_db* tdbb) const;
		void close(thread_db* tdbb) const;

		bool getRecord(thread_db* tdbb) const;
		bool refetchRecord(thread_db* tdbb) const;
		bool lockRecord(thread_db* tdbb) const;

		void print(thread_db* tdbb, Firebird::string& plan, bool detailed, unsigned level) const;

		void markRecursive();
		void invalidateRecords(jrd_req* request) const;

		void findUsedStreams(StreamList& streams, bool expandAll) const;
		void nullRecords(thread_db* tdbb) const;

	private:
		int compareKeys(thread_db* tdbb, jrd_req* request, DscNull* outerValues) const;

		RecordSource* const m_outer;
		BufferedStream* const m_inner;
		const SortNode* const m_outerKeys;
		const SortNode* const m_innerKeys;
	};
#endif	// NOT_USED_OR_REPLACED

	// BufferedStreamWindow implementation

	BufferedStreamWindow::BufferedStreamWindow(CompilerScratch* csb, BufferedStream* next)
		: m_next(next)
	{
		m_impure = CMP_impure(csb, sizeof(Impure));
	}

	void BufferedStreamWindow::open(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();
		Impure* const impure = request->getImpure<Impure>(m_impure);

		impure->irsb_flags = irsb_open;
		impure->irsb_position = 0;
	}

	void BufferedStreamWindow::close(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();

		invalidateRecords(request);

		Impure* const impure = request->getImpure<Impure>(m_impure);

		if (impure->irsb_flags & irsb_open)
			impure->irsb_flags &= ~irsb_open;
	}

	bool BufferedStreamWindow::getRecord(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();
		Impure* const impure = request->getImpure<Impure>(m_impure);

		if (!(impure->irsb_flags & irsb_open))
			return false;

		m_next->locate(tdbb, impure->irsb_position);
		if (!m_next->getRecord(tdbb))
			return false;

		++impure->irsb_position;
		return true;
	}

	bool BufferedStreamWindow::refetchRecord(thread_db* tdbb) const
	{
		return m_next->refetchRecord(tdbb);
	}

	bool BufferedStreamWindow::lockRecord(thread_db* tdbb) const
	{
		return m_next->lockRecord(tdbb);
	}

	void BufferedStreamWindow::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
	{
		m_next->print(tdbb, plan, detailed, level);
	}

	void BufferedStreamWindow::markRecursive()
	{
		m_next->markRecursive();
	}

	void BufferedStreamWindow::findUsedStreams(StreamList& streams, bool expandAll) const
	{
		m_next->findUsedStreams(streams, expandAll);
	}

	void BufferedStreamWindow::invalidateRecords(jrd_req* request) const
	{
		m_next->invalidateRecords(request);
	}

	void BufferedStreamWindow::nullRecords(thread_db* tdbb) const
	{
		m_next->nullRecords(tdbb);
	}

#ifdef NOT_USED_OR_REPLACED
	// WindowJoin implementation

	WindowJoin::WindowJoin(CompilerScratch* csb, RecordSource* outer, RecordSource* inner,
					   const SortNode* outerKeys,
					   const SortNode* innerKeys)
		: m_outer(outer), m_inner(FB_NEW(csb->csb_pool) BufferedStream(csb, inner)),
		  m_outerKeys(outerKeys), m_innerKeys(innerKeys)
	{
		fb_assert(m_outerKeys && m_innerKeys);
		fb_assert(m_outer && m_inner &&
			m_innerKeys->expressions.getCount() == m_outerKeys->expressions.getCount());

		m_impure = CMP_impure(csb, sizeof(Impure));
	}

	void WindowJoin::open(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();
		Impure* const impure = request->getImpure<Impure>(m_impure);

		impure->irsb_flags = irsb_open;

		// Read and cache the inner stream. Also gets its total number of records.

		m_inner->open(tdbb);
		FB_UINT64 position = 0;

		while (m_inner->getRecord(tdbb))
			++position;

		impure->innerRecordCount = position;

		m_outer->open(tdbb);
	}

	void WindowJoin::close(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();

		invalidateRecords(request);

		Impure* const impure = request->getImpure<Impure>(m_impure);

		if (impure->irsb_flags & irsb_open)
		{
			impure->irsb_flags &= ~irsb_open;

			m_outer->close(tdbb);
			m_inner->close(tdbb);
		}
	}

	bool WindowJoin::getRecord(thread_db* tdbb) const
	{
		jrd_req* const request = tdbb->getRequest();
		Impure* const impure = request->getImpure<Impure>(m_impure);

		if (!(impure->irsb_flags & irsb_open))
			return false;

		if (!m_outer->getRecord(tdbb))
			return false;

		// Evaluate the outer stream keys.

		HalfStaticArray<DscNull, 8> outerValues;
		DscNull* outerValue = outerValues.getBuffer(m_outerKeys->expressions.getCount());

		for (unsigned i = 0; i < m_outerKeys->expressions.getCount(); ++i)
		{
			outerValue->desc = EVL_expr(tdbb, m_outerKeys->expressions[i]);
			outerValue->null = (request->req_flags & req_null);
			++outerValue;
		}

		outerValue -= m_outerKeys->expressions.getCount();	// go back to begin

		// Join the streams. That should be a 1-to-1 join.

		SINT64 start = 0;
		SINT64 finish = impure->innerRecordCount;
		SINT64 pos = finish / 2;

		while (pos >= start && pos < finish)
		{
			m_inner->locate(tdbb, pos);
			if (!m_inner->getRecord(tdbb))
			{
				fb_assert(false);
				return false;
			}

			int cmp = compareKeys(tdbb, request, outerValue);

			if (cmp == 0)
				return true;

			if (cmp < 0)
			{
				finish = pos;
				pos -= MAX(1, (finish - start) / 2);
			}
			else //if (cmp > 0)
			{
				start = pos;
				pos += MAX(1, (finish - start) / 2);
			}
		}

		fb_assert(false);

		return false;
	}

	bool WindowJoin::refetchRecord(thread_db* tdbb) const
	{
		return true;
	}

	bool WindowJoin::lockRecord(thread_db* tdbb) const
	{
		status_exception::raise(Arg::Gds(isc_record_lock_not_supp));
		return false; // compiler silencer
	}

	void WindowJoin::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
	{
		if (detailed)
		{
			plan += printIndent(level) + "Window Join";
			m_outer->print(tdbb, plan, true, level + 1);
			m_inner->print(tdbb, plan, true, level + 1);
		}
		else
		{
			if (!level)
			{
				plan += "(";
			}

			m_outer->print(tdbb, plan, false, level + 1);

			plan += ", ";

			m_inner->print(tdbb, plan, false, level + 1);

			if (!level)
			{
				plan += ")";
			}
		}
	}

	void WindowJoin::markRecursive()
	{
		m_outer->markRecursive();
		m_inner->markRecursive();
	}

	void WindowJoin::findUsedStreams(StreamList& streams) const
	{
		m_outer->findUsedStreams(streams);
		m_inner->findUsedStreams(streams);
	}

	void WindowJoin::invalidateRecords(jrd_req* request) const
	{
		m_outer->invalidateRecords(request);
		m_inner->invalidateRecords(request);
	}

	void WindowJoin::nullRecords(thread_db* tdbb) const
	{
		m_outer->nullRecords(tdbb);
		m_inner->nullRecords(tdbb);
	}

	int WindowJoin::compareKeys(thread_db* tdbb, jrd_req* request, DscNull* outerValues) const
	{
		int cmp;

		for (size_t i = 0; i < m_innerKeys->expressions.getCount(); i++)
		{
			const DscNull& outerValue = outerValues[i];
			const dsc* const innerDesc = EVL_expr(tdbb, m_innerKeys->expressions[i]);
			const bool innerNull = (request->req_flags & req_null);

			if (outerValue.null && !innerNull)
				return -1;

			if (!outerValue.null && innerNull)
				return 1;

			if (!outerValue.null && !innerNull && (cmp = MOV_compare(outerValue.desc, innerDesc)) != 0)
				return cmp;
		}

		return 0;
	}
#endif	// NOT_USED_OR_REPLACED
}	// namespace

// ------------------------------

WindowedStream::WindowedStream(thread_db* tdbb, CompilerScratch* csb,
			ObjectsArray<WindowSourceNode::Partition>& partitions, RecordSource* next)
	: m_next(FB_NEW(csb->csb_pool) BufferedStream(csb, next)),
	  m_joinedStream(NULL)
{
	m_impure = CMP_impure(csb, sizeof(Impure));

	// Process the unpartioned and unordered map, if existent.

	for (ObjectsArray<WindowSourceNode::Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		// While here, verify not supported functions/clauses.

		const NestConst<ValueExprNode>* source = partition->map->sourceList.begin();

		for (const NestConst<ValueExprNode>* const end = partition->map->sourceList.end();
			 source != end;
			 ++source)
		{
			const AggNode* aggNode = (*source)->as<AggNode>();

			if (partition->order && aggNode)
				aggNode->checkOrderedWindowCapable();
		}

		if (!partition->group && !partition->order)
		{
			fb_assert(!m_joinedStream);

			m_joinedStream = FB_NEW(csb->csb_pool) AggregatedStream(tdbb, csb, partition->stream,
				NULL, partition->map, FB_NEW(csb->csb_pool) BufferedStreamWindow(csb, m_next),
				NULL);

			OPT_gen_aggregate_distincts(tdbb, csb, partition->map);
		}
	}

	if (!m_joinedStream)
		m_joinedStream = FB_NEW(csb->csb_pool) BufferedStreamWindow(csb, m_next);

	// Process ordered partitions.

	StreamList streams;

	for (ObjectsArray<WindowSourceNode::Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		// Refresh the stream list based on the last m_joinedStream.
		streams.clear();
		m_joinedStream->findUsedStreams(streams);

		// Build the sort key. It's the order items following the partition items.

		SortNode* partitionOrder;

		if (partition->group)
		{
			partitionOrder = FB_NEW(csb->csb_pool) SortNode(csb->csb_pool);
			partitionOrder->expressions.join(partition->group->expressions);
			partitionOrder->descending.join(partition->group->descending);
			partitionOrder->nullOrder.join(partition->group->nullOrder);

			if (partition->order)
			{
				partitionOrder->expressions.join(partition->order->expressions);
				partitionOrder->descending.join(partition->order->descending);
				partitionOrder->nullOrder.join(partition->order->nullOrder);
			}
		}
		else
			partitionOrder = partition->order;

		if (partitionOrder)
		{
			SortedStream* sortedStream = OPT_gen_sort(tdbb, csb, streams, NULL,
				m_joinedStream, partitionOrder, false);

			m_joinedStream = FB_NEW(csb->csb_pool) AggregatedStream(tdbb, csb, partition->stream,
				(partition->group ? &partition->group->expressions : NULL),
				partition->map,
				FB_NEW(csb->csb_pool) BufferedStream(csb, sortedStream),
				(partition->order ? &partition->order->expressions : NULL));

			OPT_gen_aggregate_distincts(tdbb, csb, partition->map);
		}
	}
}

void WindowedStream::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	m_next->open(tdbb);
	m_joinedStream->open(tdbb);
}

void WindowedStream::close(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	invalidateRecords(request);

	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (impure->irsb_flags & irsb_open)
	{
		impure->irsb_flags &= ~irsb_open;
		m_joinedStream->close(tdbb);
		m_next->close(tdbb);
	}
}

bool WindowedStream::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
		return false;

	if (!m_joinedStream->getRecord(tdbb))
		return false;

	return true;
}

bool WindowedStream::refetchRecord(thread_db* tdbb) const
{
	return m_joinedStream->refetchRecord(tdbb);
}

bool WindowedStream::lockRecord(thread_db* /*tdbb*/) const
{
	status_exception::raise(Arg::Gds(isc_record_lock_not_supp));
	return false; // compiler silencer
}

void WindowedStream::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
{
	m_joinedStream->print(tdbb, plan, detailed, level);
}

void WindowedStream::markRecursive()
{
	m_joinedStream->markRecursive();
}

void WindowedStream::invalidateRecords(jrd_req* request) const
{
	m_joinedStream->invalidateRecords(request);
}

void WindowedStream::findUsedStreams(StreamList& streams, bool expandAll) const
{
	m_joinedStream->findUsedStreams(streams, expandAll);
}

void WindowedStream::nullRecords(thread_db* tdbb) const
{
	m_joinedStream->nullRecords(tdbb);
}
