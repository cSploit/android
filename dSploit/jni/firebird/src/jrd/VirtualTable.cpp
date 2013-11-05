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
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "ids.h"
#include "../jrd/constants.h"
#include "../jrd/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/dsc.h"
#include "../jrd/exe.h"
#include "../jrd/ini.h"
#include "../jrd/req.h"
#include "../jrd/rse.h"
#include "../jrd/val.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/vio_proto.h"

#include "../jrd/DatabaseSnapshot.h"
#include "../jrd/RecordBuffer.h"
#include "../jrd/VirtualTable.h"

using namespace Jrd;
using namespace Firebird;


void VirtualTable::close(thread_db* tdbb, RecordSource* rsb)
{
	SET_TDBB(tdbb);

	irsb_virtual* impure = (irsb_virtual*) ((UCHAR *) tdbb->getRequest() + rsb->rsb_impure);

	impure->irsb_record_buffer = NULL;
}


void VirtualTable::erase(thread_db* tdbb, record_param* rpb)
{
	SET_TDBB(tdbb);

	Database* dbb = tdbb->getDatabase();
	fb_assert(dbb);

	jrd_rel* relation = rpb->rpb_relation;
	fb_assert(relation);

	dsc desc;
	lck_t lock_type;

	if (relation->rel_id == rel_mon_attachments)
	{
		// Get attachment id
		if (!EVL_field(relation, rpb->rpb_record, f_mon_att_id, &desc))
			return;
		lock_type = LCK_attachment;
	}
	else if (relation->rel_id == rel_mon_statements)
	{
		// Get transaction id
		if (!EVL_field(relation, rpb->rpb_record, f_mon_stmt_tra_id, &desc))
			return;
		lock_type = LCK_cancel;
	}
	else
	{
		ERR_post(Arg::Gds(isc_read_only));
	}

	const SLONG id = MOV_get_long(&desc, 0);

	// Post a blocking request
	Lock temp_lock;
	temp_lock.lck_dbb = dbb;
	temp_lock.lck_type = lock_type;
	temp_lock.lck_parent = dbb->dbb_lock;
	temp_lock.lck_owner_handle = LCK_get_owner_handle(tdbb, temp_lock.lck_type);
	temp_lock.lck_length = sizeof(SLONG);
	temp_lock.lck_key.lck_long = id;

	ThreadStatusGuard temp_status(tdbb);

	if (LCK_lock(tdbb, &temp_lock, LCK_EX, -1))
		LCK_release(tdbb, &temp_lock);
}


bool VirtualTable::get(thread_db* tdbb, RecordSource* rsb)
{
	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	record_param* const rpb = &request->req_rpb[rsb->rsb_stream];
	irsb_virtual* const impure = (irsb_virtual*) ((UCHAR *) request + rsb->rsb_impure);

	if (!impure->irsb_record_buffer)
		return false;

	rpb->rpb_number.increment();

	return impure->irsb_record_buffer->fetch(rpb->rpb_number.getValue(), rpb->rpb_record);
}


void VirtualTable::modify(thread_db*, record_param* /*org_rpb*/, record_param* /*new_rpb*/)
{
	ERR_post(Arg::Gds(isc_read_only));
}


void VirtualTable::open(thread_db* tdbb, RecordSource* rsb)
{
	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	jrd_rel* const relation = rsb->rsb_relation;
	record_param* const rpb = &request->req_rpb[rsb->rsb_stream];
	irsb_virtual* const impure = (irsb_virtual*) ((UCHAR *) request + rsb->rsb_impure);

	DatabaseSnapshot* const snapshot = DatabaseSnapshot::create(tdbb);
	impure->irsb_record_buffer = snapshot->getData(relation);

	VIO_record(tdbb, rpb, impure->irsb_record_buffer->getFormat(), request->req_pool);

	rpb->rpb_number.setValue(BOF_NUMBER);
}


Jrd::RecordSource* VirtualTable::optimize(thread_db* tdbb, OptimizerBlk* opt, SSHORT stream)
{
	SET_TDBB(tdbb);

	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];

	RecordSource* rsb = FB_NEW_RPT(*tdbb->getDefaultPool(), 0) RecordSource;
	rsb->rsb_type = rsb_virt_sequential;
	rsb->rsb_stream = stream;
	rsb->rsb_relation = csb_tail->csb_relation;
	rsb->rsb_impure = CMP_impure(csb, sizeof(irsb_virtual));

	return rsb;
}


void VirtualTable::store(thread_db*, record_param*)
{
	ERR_post(Arg::Gds(isc_read_only));
}
