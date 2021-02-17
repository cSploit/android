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
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/req.h"
#include "../dsql/ExprNodes.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/vio_proto.h"

#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// -----------------------------
// Data access: external sorting
// -----------------------------

SortedStream::SortedStream(CompilerScratch* csb, RecordSource* next, SortMap* map)
	: m_next(next), m_map(map)
{
	fb_assert(m_next && m_map);

	m_impure = CMP_impure(csb, sizeof(Impure));
}

void SortedStream::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	// Get rid of the old sort areas if this request has been used already.
	// Null the pointer before calling init() because it may throw.
	delete impure->irsb_sort;
	impure->irsb_sort = NULL;

	impure->irsb_sort = init(tdbb);
}

void SortedStream::close(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	invalidateRecords(request);

	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (impure->irsb_flags & irsb_open)
	{
		impure->irsb_flags &= ~irsb_open;

		delete impure->irsb_sort;
		impure->irsb_sort = NULL;

		m_next->close(tdbb);
	}
}

bool SortedStream::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
		return false;

	UCHAR* const data = getData(tdbb);

	if (!data)
		return false;

	mapData(tdbb, request, data);

	return true;
}

bool SortedStream::refetchRecord(thread_db* tdbb) const
{
	return m_next->refetchRecord(tdbb);
}

bool SortedStream::lockRecord(thread_db* tdbb) const
{
	return m_next->lockRecord(tdbb);
}

void SortedStream::print(thread_db* tdbb, string& plan,
						 bool detailed, unsigned level) const
{
	if (detailed)
	{
		string extras;
		extras.printf(" (record length: %"ULONGFORMAT", key length: %"ULONGFORMAT")",
					  m_map->length, m_map->keyLength);

		plan += printIndent(++level) +
			((m_map->flags & FLAG_PROJECT) ? "Unique Sort" : "Sort") + extras;

		m_next->print(tdbb, plan, true, level);
	}
	else
	{
		level++;
		plan += "SORT (";
		m_next->print(tdbb, plan, false, level);
		plan += ")";
	}
}

void SortedStream::markRecursive()
{
	m_next->markRecursive();
}

void SortedStream::findUsedStreams(StreamList& streams, bool expandAll) const
{
	m_next->findUsedStreams(streams, expandAll);
}

void SortedStream::invalidateRecords(jrd_req* request) const
{
	m_next->invalidateRecords(request);
}

void SortedStream::nullRecords(thread_db* tdbb) const
{
	m_next->nullRecords(tdbb);
}

Sort* SortedStream::init(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	m_next->open(tdbb);
	ULONG records = 0;

	// Initialize for sort. If this is really a project operation,
	// establish a callback routine to reject duplicate records.

	AutoPtr<Sort> scb(FB_NEW(request->req_sorts.getPool())
		Sort(tdbb->getDatabase(), &request->req_sorts,
			 m_map->length, m_map->keyItems.getCount(), m_map->keyItems.getCount(),
			 m_map->keyItems.begin(),
			 ((m_map->flags & FLAG_PROJECT) ? rejectDuplicate : NULL), 0));

	// Pump the input stream dry while pushing records into sort. For
	// each record, map all fields into the sort record. The reverse
	// mapping is done in get_sort().

	dsc to, temp;

	while (m_next->getRecord(tdbb))
	{
		records++;

		// "Put" a record to sort. Actually, get the address of a place
		// to build a record.

		UCHAR* data = NULL;
		scb->put(tdbb, reinterpret_cast<ULONG**>(&data));

		// Zero out the sort key. This solves a multitude of problems.

		memset(data, 0, m_map->length);

		// Loop thru all field (keys and hangers on) involved in the sort.
		// Be careful to null field all unused bytes in the sort key.

		const SortMap::Item* const end_item = m_map->items.begin() + m_map->items.getCount();
		for (const SortMap::Item* item = m_map->items.begin(); item < end_item; item++)
		{
			to = item->desc;
			to.dsc_address = data + (IPTR) to.dsc_address;
			bool flag = false;
			dsc* from = NULL;

			if (item->node)
			{
				from = EVL_expr(tdbb, request, item->node);
				if (request->req_flags & req_null)
					flag = true;
			}
			else
			{
				from = &temp;

				record_param* const rpb = &request->req_rpb[item->stream];

				if (item->fieldId < 0)
				{
					switch (item->fieldId)
					{
					case ID_TRANS:
						*reinterpret_cast<ULONG*>(to.dsc_address) = rpb->rpb_transaction_nr;
						break;
					case ID_DBKEY:
						*reinterpret_cast<SINT64*>(to.dsc_address) = rpb->rpb_number.getValue();
						break;
					case ID_DBKEY_VALID:
						*to.dsc_address = (UCHAR) rpb->rpb_number.isValid();
						break;
					default:
						fb_assert(false);
					}
					continue;
				}

				if (!EVL_field(rpb->rpb_relation, rpb->rpb_record, item->fieldId, from))
					flag = true;
			}

			*(data + item->flagOffset) = flag ? TRUE : FALSE;

			if (!flag)
			{
				// If moving a TEXT item into the key portion of the sort record,
				// then want to sort by language dependent order.

				if (IS_INTL_DATA(&item->desc) &&
					(ULONG)(IPTR) item->desc.dsc_address < m_map->keyLength)
				{
					INTL_string_to_key(tdbb, INTL_INDEX_TYPE(&item->desc), from, &to,
						(m_map->flags & FLAG_UNIQUE ? INTL_KEY_UNIQUE : INTL_KEY_SORT));
				}
				else
				{
					MOV_move(tdbb, from, &to);
				}
			}
		}
	}

	scb->sort(tdbb);

	return scb.release();
}

UCHAR* SortedStream::getData(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	ULONG* data = NULL;
	impure->irsb_sort->get(tdbb, &data);

	return reinterpret_cast<UCHAR*>(data);
}

void SortedStream::mapData(thread_db* tdbb, jrd_req* request, UCHAR* data) const
{
	StreamType stream = INVALID_STREAM;
	dsc from, to;

	const SortMap::Item* const end_item = m_map->items.begin() + m_map->items.getCount();

	for (const SortMap::Item* item = m_map->items.begin(); item < end_item; item++)
	{
		const bool flag = (*(data + item->flagOffset) == TRUE);
		from = item->desc;
		from.dsc_address = data + (IPTR) from.dsc_address;

		if (item->node && !item->node->is<FieldNode>())
			continue;

		// if moving a TEXT item into the key portion of the sort record,
		// then want to sort by language dependent order

		// in the case below a nod_field is being converted to
		// a sort key, there is a later nod_field in the item
		// list that contains the data to send back

		if (IS_INTL_DATA(&item->desc) &&
			(ULONG)(IPTR) item->desc.dsc_address < m_map->keyLength)
		{
			continue;
		}

		record_param* const rpb = &request->req_rpb[item->stream];

		const SSHORT id = item->fieldId;

		if (id < 0)
		{
			switch (id)
			{
			case ID_TRANS:
				rpb->rpb_transaction_nr = *reinterpret_cast<ULONG*>(from.dsc_address);
				break;
			case ID_DBKEY:
				rpb->rpb_number.setValue(*reinterpret_cast<SINT64*>(from.dsc_address));
				break;
			case ID_DBKEY_VALID:
				rpb->rpb_number.setValid(*from.dsc_address != 0);
				break;
			default:
				fb_assert(false);
			}

			rpb->rpb_stream_flags |= RPB_s_refetch;
			continue;
		}

		if (item->stream != stream)
		{
			stream = item->stream;

			// For the sake of prudence, set all record parameter blocks to contain
			// the most recent format. This will guarantee that all fields mapped
			// back to records by get_sort() have homes in the target record.

			// dimitr:	I've added the check for !isValid to ensure that we don't overwrite
			//			the format for an active rpb (i.e. the one having some record fetched).
			//			See CORE-3806 for example.
			if (rpb->rpb_relation && !rpb->rpb_number.isValid())
				VIO_record(tdbb, rpb, MET_current(tdbb, rpb->rpb_relation), tdbb->getDefaultPool());
		}

		Record* const record = rpb->rpb_record;

		if (record && !flag && !record->rec_format)
		{
			fb_assert(record->rec_fmt_bk);
			record->rec_format = record->rec_fmt_bk;
		}

		if (flag)
			record->setNull(id);
		else
		{
			EVL_field(rpb->rpb_relation, record, id, &to);

			MOV_move(tdbb, &from, &to);
			record->clearNull(id);
		}
	}
}
