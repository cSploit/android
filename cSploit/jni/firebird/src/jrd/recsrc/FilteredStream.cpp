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
#include "../jrd/req.h"
#include "../dsql/BoolNodes.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/evl_proto.h"

#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// ------------------------------------
// Data access: predicate driven filter
// ------------------------------------

FilteredStream::FilteredStream(CompilerScratch* csb, RecordSource* next, BoolExprNode* boolean)
	: m_next(next), m_boolean(boolean), m_anyBoolean(NULL),
	  m_ansiAny(false), m_ansiAll(false), m_ansiNot(false)
{
	fb_assert(m_next && m_boolean);

	m_impure = CMP_impure(csb, sizeof(Impure));
}

void FilteredStream::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	m_next->open(tdbb);
}

void FilteredStream::close(thread_db* tdbb) const
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

bool FilteredStream::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
		return false;

	if (!evaluateBoolean(tdbb))
	{
		invalidateRecords(request);
		return false;
	}

	return true;
}

bool FilteredStream::refetchRecord(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	return m_next->refetchRecord(tdbb) &&
		m_boolean->execute(tdbb, request);
}

bool FilteredStream::lockRecord(thread_db* tdbb) const
{
	return m_next->lockRecord(tdbb);
}

void FilteredStream::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
{
	if (detailed)
		plan += printIndent(++level) + "Filter";

	m_next->print(tdbb, plan, detailed, level);
}

void FilteredStream::markRecursive()
{
	m_next->markRecursive();
}

void FilteredStream::findUsedStreams(StreamList& streams, bool expandAll) const
{
	m_next->findUsedStreams(streams, expandAll);
}

void FilteredStream::invalidateRecords(jrd_req* request) const
{
	m_next->invalidateRecords(request);
}

void FilteredStream::nullRecords(thread_db* tdbb) const
{
	m_next->nullRecords(tdbb);
}

bool FilteredStream::evaluateBoolean(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	// For ANY and ALL clauses (ALL is handled as a negated ANY),
	// we must first detect them, and then make sure that the returned
	// results are correct. This mainly entails making sure that
	// there are in fact records in the source stream to test against.
	// If there were none, the response must be FALSE.
	// Also, if the result of the column comparison is always
	// NULL, this must also be returned as NULL.
	// (Note that normally, an AND of a NULL and a FALSE would be FALSE, not NULL).

	// This all depends on evl.cpp putting the unoptimized expression
	// in the rsb. The unoptimized expression always has the
	// select expression on the left, and the column comparison
	// on the right.

	// ANY/ALL select node pointer
	const BoolExprNode* select_node;

	// ANY/ALL column node pointer
	const BoolExprNode* column_node = m_anyBoolean;

	if (column_node && (m_ansiAny || m_ansiAll))
	{
		// see if there's a select node to work with

		const BinaryBoolNode* booleanNode = column_node->as<BinaryBoolNode>();

		if (booleanNode && booleanNode->blrOp == blr_and)
		{
			select_node = booleanNode->arg1;
			column_node = booleanNode->arg2;
		}
		else
			select_node = NULL;
	}

	if (column_node && m_ansiAny)
	{
		if (m_ansiNot)
		{
			// do NOT ANY
			// if the subquery was the empty set
			// (numTrue + numFalse + numUnknown = 0)
			// or if all were false
			// (numTrue + numUnknown = 0),
			// NOT ANY is true

			bool any_null = false;	// some records true for ANY/ALL
			bool any_true = false;	// some records true for ANY/ALL

			while (m_next->getRecord(tdbb))
			{
				if (m_boolean->execute(tdbb, request))
				{
					// found a TRUE value

					any_true = true;
					break;
				}

				// check for select stream and nulls

				if (!select_node)
				{
					if (request->req_flags & req_null)
					{
						any_null = true;
						break;
					}
				}
				else
				{
					request->req_flags &= ~req_null;

					// select for ANY/ALL processing
					const bool select_value = select_node->execute(tdbb, request);

					// see if any record in select stream

					if (select_value)
					{
						// see if any nulls

						request->req_flags &= ~req_null;
						column_node->execute(tdbb, request);

						// see if any record is null

						if (request->req_flags & req_null)
						{
							any_null = true;
							break;
						}
					}
				}
			}

			request->req_flags &= ~req_null;

			return any_null || any_true;
		}

		// do ANY
		// if the subquery was true for any comparison, ANY is true

		bool result = false;
		while (m_next->getRecord(tdbb))
		{
			if (m_boolean->execute(tdbb, request))
			{
				result = true;
				break;
			}
		}
		request->req_flags &= ~req_null;

		return result;
	}

	if (column_node && m_ansiAll)
	{
		if (m_ansiNot)
		{
			// do NOT ALL
			// if the subquery was false for any comparison, NOT ALL is true

			bool any_false = false;	// some records false for ANY/ALL
			while (m_next->getRecord(tdbb))
			{
				request->req_flags &= ~req_null;

				// look for a FALSE (and not null either)

				if (!m_boolean->execute(tdbb, request) && !(request->req_flags & req_null))
				{
					// make sure it wasn't FALSE because there's no select stream record

					if (select_node)
					{
						request->req_flags &= ~req_null;

						// select for ANY/ALL processing
						const bool select_value = select_node->execute(tdbb, request);
						if (select_value)
						{
							any_false = true;
							break;
						}
					}
					else
					{
						any_false = true;
						break;
					}
				}
			}

			request->req_flags &= ~req_null;

			return !any_false;
		}

		// do ALL
		// if the subquery was the empty set
		// (numTrue + numFalse + numUnknown = 0)
		// or if all were true
		// (numFalse + numUnknown = 0),
		// ALL is true

		bool any_false = false;	// some records false for ANY/ALL
		while (m_next->getRecord(tdbb))
		{
			request->req_flags &= ~req_null;

			// look for a FALSE or null

			if (!m_boolean->execute(tdbb, request))
			{
				// make sure it wasn't FALSE because there's no select stream record

				if (select_node)
				{
					request->req_flags &= ~req_null;

					// select for ANY/ALL processing
					const bool select_value = select_node->execute(tdbb, request);
					if (select_value)
					{
						any_false = true;
						break;
					}
				}
				else
				{
					any_false = true;
					break;
				}
			}
		}

		request->req_flags &= ~req_null;

		return !any_false;
	}

	bool nullFlag = false;
	bool result = false;
	while (m_next->getRecord(tdbb))
	{
		if (m_boolean->execute(tdbb, request))
		{
			result = true;
			break;
		}

		if (request->req_flags & req_null)
			nullFlag = true;
	}

	if (nullFlag)
		request->req_flags |= req_null;

	return result;
}
