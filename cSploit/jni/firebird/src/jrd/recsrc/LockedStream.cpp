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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Dmitry Yemanov <dimitr@firebirdsql.org>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/cmp_proto.h"

#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// ------------------------------------
// Data access: stream locked for write
// ------------------------------------

LockedStream::LockedStream(CompilerScratch* csb, RecordSource* next)
	: m_next(next)
{
	fb_assert(m_next);

	m_impure = CMP_impure(csb, sizeof(Impure));
}

void LockedStream::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	m_next->open(tdbb);
}

void LockedStream::close(thread_db* tdbb) const
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

bool LockedStream::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
		return false;

	while (m_next->getRecord(tdbb))
	{
		do {
			// Attempt to lock the record
			if (m_next->lockRecord(tdbb))
				return true;	// locked

			// Refetch the record and ensure it still fulfils the search condition
		} while (m_next->refetchRecord(tdbb));
	}

	return false;
}

bool LockedStream::refetchRecord(thread_db* tdbb) const
{
	return m_next->refetchRecord(tdbb);
}

bool LockedStream::lockRecord(thread_db* tdbb) const
{
	return m_next->lockRecord(tdbb);
}

void LockedStream::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
{
	if (detailed)
		plan += printIndent(++level) + "Write Lock";

	m_next->print(tdbb, plan, detailed, level);
}

void LockedStream::markRecursive()
{
	m_next->markRecursive();
}

void LockedStream::findUsedStreams(StreamList& streams, bool expandAll) const
{
	m_next->findUsedStreams(streams, expandAll);
}

void LockedStream::invalidateRecords(jrd_req* request) const
{
	m_next->invalidateRecords(request);
}

void LockedStream::nullRecords(thread_db* tdbb) const
{
	m_next->nullRecords(tdbb);
}
