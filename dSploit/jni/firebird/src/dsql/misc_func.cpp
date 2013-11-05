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
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../dsql/dsql.h"
#include "../dsql/misc_func.h"

using namespace Jrd;

const InternalInfo::InfoAttr InternalInfo::attr_array[max_internal_id] =
{
	{"<UNKNOWN>", 0},
	{"CURRENT_CONNECTION", 0},
	{"CURRENT_TRANSACTION", 0},
	{"GDSCODE", REQ_block},
	{"SQLCODE", REQ_block},
	{"ROW_COUNT", REQ_block},
	{"INSERTING/UPDATING/DELETING", REQ_trigger},
	{"SQLSTATE", REQ_block}
};

const char* InternalInfo::getAlias(internal_info_id info_id)
{
	return attr_array[info_id].alias_name;
}

USHORT InternalInfo::getMask(internal_info_id info_id)
{
	return attr_array[info_id].req_mask;
}

