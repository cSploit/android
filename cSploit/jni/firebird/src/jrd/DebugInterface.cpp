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
#include "../jrd/Attachment.h"
#include "../jrd/DebugInterface.h"
#include "../jrd/blb_proto.h"

using namespace Jrd;
using namespace Firebird;

void DBG_parse_debug_info(thread_db* tdbb, bid* blob_id, DbgInfo& dbgInfo)
{
	Jrd::Attachment* attachment = tdbb->getAttachment();

	blb* blob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);
	const ULONG length = blob->blb_length;
	HalfStaticArray<UCHAR, 128> tmp;

	UCHAR* temp = tmp.getBuffer(length);
	blob->BLB_get_data(tdbb, temp, length);

	DBG_parse_debug_info(length, temp, dbgInfo);
}

void DBG_parse_debug_info(ULONG length, const UCHAR* data, DbgInfo& dbgInfo)
{
	const UCHAR* const end = data + length;
	bool bad_format = false;

	if ((*data++ != fb_dbg_version) || (end[-1] != fb_dbg_end))
		bad_format = true;

	UCHAR version = UCHAR(0);

	if (!bad_format)
	{
		version = *data++;

		if (!version || version > CURRENT_DBG_INFO_VERSION)
			bad_format = true;
	}

	while (!bad_format && (data < end))
	{
		UCHAR code = *data++;

		switch (code)
		{
		case fb_dbg_map_src2blr:
			{
				const unsigned length =
					(version == DBG_INFO_VERSION_1) ? 6 : 12;

				if (data + length > end)
				{
					bad_format = true;
					break;
				}

				MapBlrToSrcItem i;

				i.mbs_src_line = *data++;
				i.mbs_src_line |= *data++ << 8;

				if (version > DBG_INFO_VERSION_1)
				{
					i.mbs_src_line |= *data++ << 16;
					i.mbs_src_line |= *data++ << 24;
				}

				i.mbs_src_col = *data++;
				i.mbs_src_col |= *data++ << 8;

				if (version > DBG_INFO_VERSION_1)
				{
					i.mbs_src_col |= *data++ << 16;
					i.mbs_src_col |= *data++ << 24;
				}

				i.mbs_offset = *data++;
				i.mbs_offset |= *data++ << 8;

				if (version > DBG_INFO_VERSION_1)
				{
					i.mbs_offset |= *data++ << 16;
					i.mbs_offset |= *data++ << 24;
				}

				dbgInfo.blrToSrc.add(i);
			}
			break;

		case fb_dbg_map_varname:
			{
				if (data + 3 > end)
				{
					bad_format = true;
					break;
				}

				// variable number
				USHORT index = *data++;
				index |= *data++ << 8;

				// variable name string length
				USHORT length = *data++;

				if (data + length > end)
				{
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
				if (data + 4 > end)
				{
					bad_format = true;
					break;
				}

				ArgumentInfo info;

				// argument type
				info.type = *data++;

				// argument number
				info.index = *data++;
				info.index |= *data++ << 8;

				// argument name string length
				USHORT length = *data++;

				if (data + length > end)
				{
					bad_format = true;
					break;
				}

				dbgInfo.argInfoToName.put(info, MetaName((const TEXT*) data, length));

				// variable name string
				data += length;
			}
			break;

		case fb_dbg_subproc:
		case fb_dbg_subfunc:
			{
				if (version == DBG_INFO_VERSION_1 || data >= end)
				{
					bad_format = true;
					break;
				}

				// argument name string length
				ULONG length = *data++;

				if (data + length >= end)
				{
					bad_format = true;
					break;
				}

				MetaName name((const TEXT*) data, length);
				data += length;

				if (data + 4 >= end)
				{
					bad_format = true;
					break;
				}

				length = *data++;
				length |= *data++ << 8;
				length |= *data++ << 16;
				length |= *data++ << 24;

				if (data + length >= end)
				{
					bad_format = true;
					break;
				}

				AutoPtr<DbgInfo> sub(FB_NEW(dbgInfo.getPool()) DbgInfo(dbgInfo.getPool()));
				DBG_parse_debug_info(length, data, *sub);
				data += length;

				if (code == fb_dbg_subproc)
					dbgInfo.subProcs.put(name, sub.release());
				else
					dbgInfo.subFuncs.put(name, sub.release());

				break;
			}

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
