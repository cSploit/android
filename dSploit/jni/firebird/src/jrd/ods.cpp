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
 *  Copyright (c) 2005 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/ods.h"
#include "../jrd/ods_proto.h"

namespace Ods {

bool isSupported(USHORT majorVersion, USHORT minorVersion)
{
	const bool isFirebird = (majorVersion & ODS_FIREBIRD_FLAG);
	majorVersion &= ~ODS_FIREBIRD_FLAG;

#ifdef ODS_8_TO_CURRENT
	// Support InterBase major ODS numbers from 8 to 10
	if (!isFirebird)
	{
		return (majorVersion >= ODS_VERSION8 &&
				majorVersion <= ODS_VERSION10);
	}

	// This is for future ODS versions
	if (majorVersion > ODS_VERSION10 &&
		majorVersion < ODS_VERSION)
	{
		return true;
	}
#endif

	// Support current ODS of the engine
	if (majorVersion == ODS_VERSION &&
		minorVersion >= ODS_RELEASED &&
		minorVersion <= ODS_CURRENT)
	{
		return true;
	}

	// Do not support anything else
	return false;
}

} // namespace
