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
 *  Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "ids.h"
#include "../jrd/constants.h"
#include "../common/gdsassert.h"
#include "../jrd/jrd.h"
#include "../common/dsc.h"
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

		// Ignore attempt to stop system attachment
		dsc sysFlag;
		if (EVL_field(relation, rpb->rpb_record, f_mon_att_sys_flag, &sysFlag) &&
			MOV_get_long(&sysFlag, 0) != 0)
		{
			return;
		}

		lock_type = LCK_attachment;
	}
	else if (relation->rel_id == rel_mon_statements)
	{
		// Get attachment id
		if (!EVL_field(relation, rpb->rpb_record, f_mon_stmt_att_id, &desc))
			return;
		lock_type = LCK_cancel;
	}
	else
	{
		ERR_post(Arg::Gds(isc_read_only));
		return;
	}

	const SLONG id = MOV_get_long(&desc, 0);

	// Post a blocking request
	Lock temp_lock(tdbb, sizeof(SLONG), lock_type);
	temp_lock.lck_key.lck_long = id;

	ThreadStatusGuard temp_status(tdbb);

	if (LCK_lock(tdbb, &temp_lock, LCK_EX, -1))
		LCK_release(tdbb, &temp_lock);
}


void VirtualTable::modify(thread_db* /*tdbb*/, record_param* /*org_rpb*/, record_param* /*new_rpb*/)
{
	ERR_post(Arg::Gds(isc_read_only));
}


void VirtualTable::store(thread_db* /*tdbb*/, record_param* /*rpb*/)
{
	ERR_post(Arg::Gds(isc_read_only));
}
