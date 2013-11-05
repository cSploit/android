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

#include "firebird.h"
#include "../jrd/gdsassert.h"
#include "rwlock.h"
#include "PublicHandle.h"

namespace Firebird
{
	GlobalPtr<Array<const void*> > PublicHandle::handles;
	GlobalPtr<RWLock> PublicHandle::sync;

	PublicHandle::PublicHandle()
	{
		WriteLockGuard guard(sync);

		if (handles->exist(this))
		{
			fb_assert(false);
		}
		else
		{
			handles->add(this);
		}
	}

	PublicHandle::~PublicHandle()
	{
		WriteLockGuard guard(sync);

		size_t pos;
		if (handles->find(this, pos))
		{
			handles->remove(pos);
		}
		else
		{
			fb_assert(false);
		}
	}

	bool PublicHandle::isKnownHandle() const
	{
		if (!this)
		{
			return false;
		}

		ReadLockGuard guard(sync);
		return handles->exist(this);
	}

} // namespace
