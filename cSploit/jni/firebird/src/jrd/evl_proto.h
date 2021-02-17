/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		evl_proto.h
 *	DESCRIPTION:	Prototype header file for evl.cpp
 *
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

#ifndef JRD_EVL_PROTO_H
#define JRD_EVL_PROTO_H

#include "../jrd/intl_classes.h"
#include "../jrd/req.h"

namespace Jrd
{
	class InversionNode;
	struct Item;
	struct ItemInfo;
}

dsc*		EVL_assign_to(Jrd::thread_db* tdbb, const Jrd::ValueExprNode*);
Jrd::RecordBitmap**	EVL_bitmap(Jrd::thread_db* tdbb, const Jrd::InversionNode*, Jrd::RecordBitmap*);
bool		EVL_field(Jrd::jrd_rel*, Jrd::Record*, USHORT, dsc*);
void		EVL_make_value(Jrd::thread_db* tdbb, const dsc*, Jrd::impure_value*, MemoryPool* pool = NULL);
void		EVL_validate(Jrd::thread_db*, const Jrd::Item&, const Jrd::ItemInfo*, dsc*, bool);

namespace Jrd
{
	// Evaluate a value expression.
	inline dsc* EVL_expr(thread_db* tdbb, jrd_req* request, const ValueExprNode* node)
	{
		if (!node)
			BUGCHECK(303);	// msg 303 Invalid expression for evaluation

		SET_TDBB(tdbb);

		if (--tdbb->tdbb_quantum < 0)
			JRD_reschedule(tdbb, 0, true);

		request->req_flags &= ~req_null;

		dsc* desc = node->execute(tdbb, request);

		if (desc)
			request->req_flags &= ~req_null;
		else
			request->req_flags |= req_null;

		return desc;
	}
}

#endif // JRD_EVL_PROTO_H
