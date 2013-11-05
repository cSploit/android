/*
 *	PROGRAM:	Server Code
 *	MODULE:		rpb_chain.cpp
 *	DESCRIPTION:	Keeps track of rpb's, updated_in_place in
 *	        		single transaction
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
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/rpb_chain.h"

using namespace Jrd;

#ifdef DEBUG_GDS_ALLOC
#define ExecAssert(x) fb_assert(x)
#else  //DEBUG_GDS_ALLOC
#define ExecAssert(x) x
#endif //DEBUG_GDS_ALLOC

// rpb_chain.h includes req.h => struct record_param.

int traRpbList::PushRpb(record_param* value)
{
	if (value->rpb_relation->rel_view_rse ||	// this is view
		value->rpb_relation->rel_file ||		// this is external file
		value->rpb_relation->isVirtual() ||		// this is virtual table
		value->rpb_number.isBof())				// recno is a BOF marker
	{
		return -1;
	}
	int pos = add(traRpbListElement(value, ~0));
	int level = -1;
	if (pos-- > 0)
	{
		traRpbListElement& prev = (*this)[pos];
		if (prev.lr_rpb->rpb_relation->rel_id == value->rpb_relation->rel_id &&
			prev.lr_rpb->rpb_number == value->rpb_number)
		{
			// we got the same record once more - mark for refetch
			level = prev.level;
			fb_assert(pos >= level);
			fb_assert((*this)[pos - level].level == 0);
			prev.lr_rpb->rpb_stream_flags |= RPB_s_refetch;
		}
	}
	(*this)[++pos].level = ++level;
	return level;
}

bool traRpbList::PopRpb(record_param* value, int Level)
{
	if (Level < 0) {
		return false;
	}
	size_t pos;
	ExecAssert(find(traRpbListElement(value, Level), pos));
	const bool rc = (*this)[pos].lr_rpb->rpb_stream_flags & RPB_s_refetch;
	remove(pos);
	return rc;
}

