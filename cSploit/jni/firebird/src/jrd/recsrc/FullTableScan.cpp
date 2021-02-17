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
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/Attachment.h"

#include "RecordSource.h"

using namespace Firebird;
using namespace Jrd;

// -------------------------------------------
// Data access: sequential complete table scan
// -------------------------------------------

FullTableScan::FullTableScan(CompilerScratch* csb, const string& name, StreamType stream)
	: RecordStream(csb, stream), m_name(csb->csb_pool, name)
{
	m_impure = CMP_impure(csb, sizeof(Impure));
}

void FullTableScan::open(thread_db* tdbb) const
{
	Database* const dbb = tdbb->getDatabase();
	Attachment* const attachment = tdbb->getAttachment();
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open;

	record_param* const rpb = &request->req_rpb[m_stream];
	rpb->getWindow(tdbb).win_flags = 0;

	// Unless this is the only attachment, limit the cache flushing
	// effect of large sequential scans on the page working sets of
	// other attachments

	if (attachment && (attachment != dbb->dbb_attachments || attachment->att_next))
	{
		// If the relation has more data pages than the number of
		// pages in the buffer cache then mark the input window
		// block as a large scan so that a data page is released
		// to the LRU tail after its last record is fetched.
		//
		// A database backup treats everything as a large scan
		// because the cumulative effect of scanning all relations
		// is equal to that of a single large relation.

		BufferControl* const bcb = dbb->dbb_bcb;

		if ((attachment->att_flags & ATT_gbak_attachment) ||
			DPM_data_pages(tdbb, rpb->rpb_relation) > bcb->bcb_count)
		{
			rpb->getWindow(tdbb).win_flags = WIN_large_scan;
			rpb->rpb_org_scans = rpb->rpb_relation->rel_scan_count++;
		}
	}

	RLCK_reserve_relation(tdbb, request->req_transaction, rpb->rpb_relation, false);

	rpb->rpb_number.setValue(BOF_NUMBER);
}

void FullTableScan::close(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();

	invalidateRecords(request);

	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (impure->irsb_flags & irsb_open)
	{
		impure->irsb_flags &= ~irsb_open;

		record_param* const rpb = &request->req_rpb[m_stream];
		if ((rpb->getWindow(tdbb).win_flags & WIN_large_scan) &&
			rpb->rpb_relation->rel_scan_count)
		{
			rpb->rpb_relation->rel_scan_count--;
		}
	}
}

bool FullTableScan::getRecord(thread_db* tdbb) const
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

	if (VIO_next_record(tdbb, rpb, request->req_transaction, request->req_pool, false))
	{
		rpb->rpb_number.setValid(true);
		return true;
	}

	rpb->rpb_number.setValid(false);
	return false;
}

void FullTableScan::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
{
	if (detailed)
	{
		plan += printIndent(++level) + "Table \"" + printName(tdbb, m_name) + "\" Full Scan";
	}
	else
	{
		if (!level)
			plan += "(";

		plan += printName(tdbb, m_name) + " NATURAL";

		if (!level)
			plan += ")";
	}
}
