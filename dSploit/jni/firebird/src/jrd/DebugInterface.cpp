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
 *  The Original Code was created by Vlad Horsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Vlad Horsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/DebugInterface.h"
#include "../jrd/blb_proto.h"

using namespace Jrd;
using namespace Firebird;

const UCHAR CURRENT_DBG_INFO_VERSION = UCHAR(1);

void DBG_parse_debug_info(thread_db* tdbb, bid *blob_id, Firebird::DbgInfo& dbgInfo)
{
	Database* dbb = tdbb->getDatabase();
	blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, blob_id);
	const ULONG length = blob->blb_length;
	fb_assert(length < MAX_USHORT); // CVC: Otherwise, we'll overflow the function below.
	Firebird::HalfStaticArray<UCHAR, 128> tmp;

	UCHAR* temp = tmp.getBuffer(length);
	BLB_get_data(tdbb, blob, temp, length);

	DBG_parse_debug_info(length, temp, dbgInfo);
}

void DBG_parse_debug_info(USHORT length, const UCHAR* data, Firebird::DbgInfo& dbgInfo)
{
	const UCHAR* const end = data + length;
	bool bad_format = false;

	if ((*data++ != fb_dbg_version) || (end[-1] != fb_dbg_end) ||
		(*data++ != CURRENT_DBG_INFO_VERSION))
	{
		bad_format = true;
	}

	while (!bad_format && (data < end))
	{
		switch (*data++)
		{
		case fb_dbg_map_src2blr:
			{
				if (data + 6 > end) {
					bad_format = true;
					break;
				}

				MapBlrToSrcItem i;
				i.mbs_src_line = *data++;
				i.mbs_src_line |= *data++ << 8;

				i.mbs_src_col = *data++;
				i.mbs_src_col |= *data++ << 8;

				i.mbs_offset = *data++;
				i.mbs_offset |= *data++ << 8;

				dbgInfo.blrToSrc.add(i);
			}
			break;

		case fb_dbg_map_varname:
			{
				if (data + 3 > end) {
					bad_format = true;
					break;
				}

				// variable number
				USHORT index = *data++;
				index |= *data++;

				// variable name string length
				USHORT length = *data++;

				if (data + length > end) {
					bad_format = true;
					break;
				}

				dbgInfo.varIndexToName.put(index, MetaName((const TEXT*) data, length));

				// variable name string
				data += length;
			}
			break;

		case fb_dbg_map_argument:
			{
				if (data + 4 > end) {
					bad_format = true;
					break;
				}

				ArgumentInfo info;

				// argument type
				info.type = *data++;

				// argument number
				info.index = *data++;
				info.index |= *data++;

				// argument name string length
				USHORT length = *data++;

				if (data + length > end) {
					bad_format = true;
					break;
				}

				dbgInfo.argInfoToName.put(info, MetaName((const TEXT*) data, length));

				// variable name string
				data += length;
			}
			break;

		case fb_dbg_end:
			if (data != end)
				bad_format = true;
			break;

		default:
			bad_format = true;
		}
	}

	if (bad_format || data != end)
	{
		dbgInfo.clear();
		ERR_post_warning(Arg::Warning(isc_bad_debug_format));
	}
}
