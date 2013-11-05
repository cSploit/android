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

#ifndef DSQL_MISC_FUNC_H
#define DSQL_MISC_FUNC_H

#include "../jrd/misc_func_ids.h"

class InternalInfo
{
private:
	struct InfoAttr
	{
		const char* alias_name;
		unsigned short req_mask;
	};
	static const InfoAttr attr_array[max_internal_id];
public:
	static const char* getAlias(internal_info_id);
	static USHORT getMask(internal_info_id);
};

#endif // DSQL_MISC_FUNC_H

