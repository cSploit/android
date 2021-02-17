/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../dsql/Nodes.h"
#include "../dsql/ExprNodes.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/Attachment.h"

#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// ------------------------
// Data access: aggregation
// ------------------------

// Note that we can have NULL order here, in case of window function with shouldCallWinPass
// returning true, with partition, and without order. Example: ROW_NUMBER() OVER (PARTITION BY N).
AggregatedStream::AggregatedStream(thread_db* tdbb, CompilerScratch* csb, StreamType stream,
			const NestValueArray* group, MapNode* map, BaseBufferedStream* next,
			const NestValueArray* order)
	: RecordStream(csb, stream),
	  m_bufferedStream(next),
	  m_next(m_bufferedStream),
	  m_group(group),
	  m_map(map),
	  m_order(order),
	  m_winPassSources(csb->csb_pool),
	  m_winPassTargets(csb->csb_pool)
{
	init(tdbb, csb);
}

AggregatedStream::AggregatedStream(thread_db* tdbb, CompilerScratch* csb, StreamType stream,
			const NestValueArray* group, MapNode* map, RecordSource* next)
	: RecordStream(csb, stream),
	  m_bufferedStream(NULL),
	  m_next(next),
	  m_group(group),
	  m_map(map),
	  m_order(NULL),
	  m_winPassSources(csb->csb_pool),
	  m_winPassTargets(csb->csb_pool)
{
	init(tdbb, csb);
}

void AggregatedStream::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	impure->state = STATE_GROUPING;
	impure->pending = 0;
	VIO_record(tdbb, &request->req_rpb[m_stream], m_format, tdbb->getDefaultPool());

	unsigned impureCount = m_group ? m_group->getCount() : 0;
	impureCount += m_order ? m_order->getCount() : 0;

	if (!impure->impureValues && impureCount > 0)
	{
		impure->impureValues = FB_NEW(*tdbb->getDefaultPool()) impure_value[impureCount];
		memset(impure->impureValues, 0, sizeof(impure_value) * impureCount);
	}

	m_next->open(tdbb);
}

void AggregatedStream::close(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	invalidateRecords(request);

	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (impure->irsb_flags & irsb_open)
	{
		impure->irsb_flags &= ~irsb_open;

		m_next->close(tdbb);
	}
}

bool AggregatedStream::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	record_param* const rpb = &request->req_rpb[m_stream];
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
	{
		rpb->rpb_number.setValid(false);
		return false;
	}

	if (m_bufferedStream)	// Is that a window stream?
	{
		const FB_UINT64 position = m_bufferedStream->getPosition(request);

		if (impure->pending == 0)
		{
			if (impure->state == STATE_PENDING)
			{
				if (!m_bufferedStream->getRecord(tdbb))
					fb_assert(false);
			}

			impure->state = evaluateGroup(tdbb, impure->state);

			if (impure->state == STATE_PROCESS_EOF)
			{
				rpb->rpb_number.setValid(false);
				return false;
			}

			impure->pending = m_bufferedStream->getPosition(request) - position -
				(impure->state == STATE_EOF_FOUND ? 0 : 1);
			m_bufferedStream->locate(tdbb, position);
		}

		if (m_winPassSources.hasData())
		{
			SlidingWindow window(tdbb, m_bufferedStream, m_group, request);
			dsc* desc;

			const NestConst<ValueExprNode>* const sourceEnd = m_winPassSources.end();

			for (const NestConst<ValueExprNode>* source = m_winPassSources.begin(),
					*target = m_winPassTargets.begin();
				 source != sourceEnd;
				 ++source, ++target)
			{
				const AggNode* aggNode = (*source)->as<AggNode>();

				const FieldNode* field = (*target)->as<FieldNode>();
				const USHORT id = field->fieldId;
				Record* record = request->req_rpb[field->fieldStream].rpb_record;

				desc = aggNode->winPass(tdbb, request, &window);

				if (!desc)
					record->setNull(id);
				else
				{
					MOV_move(tdbb, desc, EVL_assign_to(tdbb, *target));
					record->clearNull(id);
				}
			}
		}

		if (impure->pending > 0)
			--impure->pending;

		if (!m_bufferedStream->getRecord(tdbb))
		{
			rpb->rpb_number.setValid(false);
			return false;
		}

		// If there is no group, we should reassign the map items.
		if (!m_group)
		{
			const NestConst<ValueExprNode>* const sourceEnd = m_map->sourceList.end();

			for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin(),
					*target = m_map->targetList.begin();
				 source != sourceEnd;
				 ++source, ++target)
			{
				const AggNode* aggNode = (*source)->as<AggNode>();

				if (!aggNode)
					EXE_assignment(tdbb, *source, *target);
			}
		}
	}
	else
	{
		impure->state = evaluateGroup(tdbb, impure->state);

		if (impure->state == STATE_PROCESS_EOF)
		{
			rpb->rpb_number.setValid(false);
			return false;
		}
	}

	rpb->rpb_number.setValid(true);
	return true;
}

bool AggregatedStream::refetchRecord(thread_db* tdbb) const
{
	return m_next->refetchRecord(tdbb);
}

bool AggregatedStream::lockRecord(thread_db* /*tdbb*/) const
{
	status_exception::raise(Arg::Gds(isc_record_lock_not_supp));
	return false; // compiler silencer
}

void AggregatedStream::print(thread_db* tdbb, string& plan,
							 bool detailed, unsigned level) const
{
	if (detailed)
	{
		plan += printIndent(++level) + (m_bufferedStream ? "Window" : "Aggregate");
	}

	m_next->print(tdbb, plan, detailed, level);
}

void AggregatedStream::markRecursive()
{
	m_next->markRecursive();
}

void AggregatedStream::invalidateRecords(jrd_req* request) const
{
	m_next->invalidateRecords(request);
}

void AggregatedStream::findUsedStreams(StreamList& streams, bool expandAll) const
{
	RecordStream::findUsedStreams(streams);

	if (expandAll)
		m_next->findUsedStreams(streams, true);

	if (m_bufferedStream)
		m_bufferedStream->findUsedStreams(streams, expandAll);
}

void AggregatedStream::nullRecords(thread_db* tdbb) const
{
	RecordStream::nullRecords(tdbb);

	if (m_bufferedStream)
		m_bufferedStream->nullRecords(tdbb);
}

void AggregatedStream::init(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(m_map && m_next);
	m_impure = CMP_impure(csb, sizeof(Impure));

	m_map->aggPostRse(tdbb, csb);

	// Separate nodes that requires the winPass call.

	const NestConst<ValueExprNode>* const sourceEnd = m_map->sourceList.end();

	for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin(),
			*target = m_map->targetList.begin();
		 source != sourceEnd;
		 ++source, ++target)
	{
		const AggNode* aggNode = (*source)->as<AggNode>();

		if (aggNode && aggNode->shouldCallWinPass())
		{
			m_winPassSources.add(*source);
			m_winPassTargets.add(*target);
		}
	}
}

// Compute the next aggregated record of a value group. evlGroup is driven by, and returns, a state
// variable.
AggregatedStream::State AggregatedStream::evaluateGroup(thread_db* tdbb, State state) const
{
	jrd_req* const request = tdbb->getRequest();

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	Impure* const impure = request->getImpure<Impure>(m_impure);

	// if we found the last record last time, we're all done

	if (state == STATE_EOF_FOUND)
		return STATE_PROCESS_EOF;

	try
	{
		const NestConst<ValueExprNode>* const sourceEnd = m_map->sourceList.end();

		// If there isn't a record pending, open the stream and get one

		if (!m_order || state == STATE_PROCESS_EOF || state == STATE_GROUPING)
		{
			// Initialize the aggregate record

			for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin(),
					*target = m_map->targetList.begin();
				 source != sourceEnd;
				 ++source, ++target)
			{
				const AggNode* aggNode = (*source)->as<AggNode>();

				if (aggNode)
					aggNode->aggInit(tdbb, request);
				else if ((*source)->is<LiteralNode>())
					EXE_assignment(tdbb, *source, *target);
			}
		}

		if (state == STATE_PROCESS_EOF || state == STATE_GROUPING)
		{
			if (!m_next->getRecord(tdbb))
			{
				if (m_group)
				{
					finiDistinct(tdbb, request);
					return STATE_PROCESS_EOF;
				}

				state = STATE_EOF_FOUND;
			}
		}

		cacheValues(tdbb, request, m_group, 0);

		if (state != STATE_EOF_FOUND)
			cacheValues(tdbb, request, m_order, (m_group ? m_group->getCount() : 0));

		// Loop thru records until either a value change or EOF

		while (state != STATE_EOF_FOUND)
		{
			state = STATE_PENDING;

			// go through and compute all the aggregates on this record

			for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin(),
					*target = m_map->targetList.begin();
				 source != sourceEnd;
				 ++source, ++target)
			{
				const AggNode* aggNode = (*source)->as<AggNode>();

				if (aggNode)
				{
					if (aggNode->aggPass(tdbb, request))
					{
						// If a max or min has been mapped to an index, then the first record is the EOF.
						if (aggNode->indexed)
							state = STATE_EOF_FOUND;
					}
				}
				else
					EXE_assignment(tdbb, *source, *target);
			}

			if (state != STATE_EOF_FOUND && m_next->getRecord(tdbb))
			{
				// In the case of a group by, look for a change in value of any of
				// the columns; if we find one, stop aggregating and return what we have.

				if (lookForChange(tdbb, request, m_group, 0))
				{
					if (m_order)
						state = STATE_GROUPING;
					break;
				}

				if (lookForChange(tdbb, request, m_order, (m_group ? m_group->getCount() : 0)))
					break;
			}
			else
				state = STATE_EOF_FOUND;
		}

		// Finish up any residual computations and get out

		for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin(),
				*target = m_map->targetList.begin();
			 source != sourceEnd;
			 ++source, ++target)
		{
			const AggNode* aggNode = (*source)->as<AggNode>();

			if (aggNode)
			{
				const FieldNode* field = (*target)->as<FieldNode>();
				const USHORT id = field->fieldId;
				Record* record = request->req_rpb[field->fieldStream].rpb_record;

				dsc* desc = aggNode->execute(tdbb, request);
				if (!desc || !desc->dsc_dtype)
					record->setNull(id);
				else
				{
					MOV_move(tdbb, desc, EVL_assign_to(tdbb, *target));
					record->clearNull(id);
				}
			}
		}
	}
	catch (const Exception&)
	{
		finiDistinct(tdbb, request);
		throw;
	}

	return state;
}

// Cache the values of a group/order in the impure.
inline void AggregatedStream::cacheValues(thread_db* tdbb, jrd_req* request,
	const NestValueArray* group, unsigned impureOffset) const
{
	if (!group)
		return;

	Impure* const impure = request->getImpure<Impure>(m_impure);

	for (const NestConst<ValueExprNode>* ptrValue = group->begin(), *endValue = group->end();
		 ptrValue != endValue;
		 ++ptrValue, ++impureOffset)
	{
		const ValueExprNode* from = *ptrValue;
		impure_value* target = &impure->impureValues[impureOffset];

		dsc* desc = EVL_expr(tdbb, request, from);

		if (request->req_flags & req_null)
			target->vlu_desc.dsc_address = NULL;
		else
			EVL_make_value(tdbb, desc, target);
	}
}

// Look for change in the values of a group/order.
inline bool AggregatedStream::lookForChange(thread_db* tdbb, jrd_req* request,
	const NestValueArray* group, unsigned impureOffset) const
{
	if (!group)
		return false;

	Impure* const impure = request->getImpure<Impure>(m_impure);

	for (const NestConst<ValueExprNode>* ptrValue = group->begin(), *endValue = group->end();
		 ptrValue != endValue;
		 ++ptrValue, ++impureOffset)
	{
		const ValueExprNode* from = *ptrValue;
		impure_value* vtemp = &impure->impureValues[impureOffset];

		dsc* desc = EVL_expr(tdbb, request, from);

		if (request->req_flags & req_null)
		{
			if (vtemp->vlu_desc.dsc_address)
				return true;
		}
		else if (!vtemp->vlu_desc.dsc_address || MOV_compare(&vtemp->vlu_desc, desc) != 0)
			return true;
	}

	return false;
}

// Finalize a sort for distinct aggregate
void AggregatedStream::finiDistinct(thread_db* tdbb, jrd_req* request) const
{
	const NestConst<ValueExprNode>* const sourceEnd = m_map->sourceList.end();

	for (const NestConst<ValueExprNode>* source = m_map->sourceList.begin();
		 source != sourceEnd;
		 ++source)
	{
		const AggNode* aggNode = (*source)->as<AggNode>();

		if (aggNode)
			aggNode->aggFinish(tdbb, request);
	}
}


SlidingWindow::SlidingWindow(thread_db* aTdbb, const BaseBufferedStream* aStream,
			const NestValueArray* aGroup, jrd_req* aRequest)
	: tdbb(aTdbb),	// Note: instanciate the class only as local variable
	  stream(aStream),
	  group(aGroup),
	  request(aRequest),
	  moved(false)
{
	savedPosition = stream->getPosition(request);
}

SlidingWindow::~SlidingWindow()
{
	if (!moved)
		return;

	for (impure_value* impure = partitionKeys.begin(); impure != partitionKeys.end(); ++impure)
		delete impure->vlu_string;

	// Position the stream where we received it.
	stream->locate(tdbb, savedPosition);
}

// Move in the window without pass partition boundaries.
bool SlidingWindow::move(SINT64 delta)
{
	const SINT64 newPosition = SINT64(savedPosition) + delta;

	// If we try to go out of bounds, no need to check the partition.
	if (newPosition < 0 || newPosition >= (SINT64) stream->getCount(tdbb))
		return false;

	if (!group)
	{
		// No partition, we may go everywhere.

		moved = true;

		stream->locate(tdbb, newPosition);

		if (!stream->getRecord(tdbb))
		{
			fb_assert(false);
			return false;
		}

		return true;
	}

	if (!moved)
	{
		// This is our first move. We should cache the partition values, so subsequente moves didn't
		// need to evaluate them again.

		if (!stream->getRecord(tdbb))
		{
			fb_assert(false);
			return false;
		}

		try
		{
			impure_value* impure = partitionKeys.getBuffer(group->getCount());
			memset(impure, 0, sizeof(impure_value) * group->getCount());

			const NestConst<ValueExprNode>* const end = group->end();
			dsc* desc;

			for (const NestConst<ValueExprNode>* ptr = group->begin(); ptr < end; ++ptr, ++impure)
			{
				const ValueExprNode* from = *ptr;
				desc = EVL_expr(tdbb, request, from);

				if (request->req_flags & req_null)
					impure->vlu_desc.dsc_address = NULL;
				else
					EVL_make_value(tdbb, desc, impure);
			}
		}
		catch (const Exception&)
		{
			stream->locate(tdbb, savedPosition);	// Reposition for a new try.
			throw;
		}

		moved = true;
	}

	stream->locate(tdbb, newPosition);

	if (!stream->getRecord(tdbb))
	{
		fb_assert(false);
		return false;
	}

	// Verify if we're still inside the same partition.

	impure_value* impure = partitionKeys.begin();
	dsc* desc;
	const NestConst<ValueExprNode>* const end = group->end();

	for (const NestConst<ValueExprNode>* ptr = group->begin(); ptr != end; ++ptr, ++impure)
	{
		const ValueExprNode* from = *ptr;
		desc = EVL_expr(tdbb, request, from);

		if (request->req_flags & req_null)
		{
			if (impure->vlu_desc.dsc_address)
				return false;
		}
		else
		{
			if (!impure->vlu_desc.dsc_address || MOV_compare(&impure->vlu_desc, desc) != 0)
				return false;
		}
	}

	return true;
}
