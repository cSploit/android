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
 *  Copyright (c) 2008 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef COMMON_PUBLIC_HANDLE
#define COMMON_PUBLIC_HANDLE

#include "../common/classes/init.h"
#include "../common/classes/array.h"

namespace Firebird
{
	class RWLock;

	class PublicHandle
	{
	public:
		PublicHandle();
		~PublicHandle();

		bool isKnownHandle() const;

	private:
		static GlobalPtr<Array<const void*> > handles;
		static GlobalPtr<RWLock> sync;
	};

} // namespace

#endif // COMMON_PUBLIC_HANDLE
