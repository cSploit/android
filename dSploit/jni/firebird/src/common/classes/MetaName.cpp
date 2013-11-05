/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		MetaName.cpp
 *	DESCRIPTION:	metadata name holder
 *
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
 *  The Original Code was created by Alexander Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2005 Alexander Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"

#include <stdarg.h>

#include "../common/classes/MetaName.h"

namespace Firebird {

	MetaName& MetaName::assign(const char* s, size_t l)
	{
		init();
		if (s)
		{
			adjustLength(s, l);
			count = l;
			memcpy(data, s, l);
		}
		else {
			count = 0;
		}
		return *this;
	}

	int MetaName::compare(const char* s, size_t l) const
	{
		if (s)
		{
			adjustLength(s, l);
			size_t x = length() < l ? length() : l;
			int rc = memcmp(c_str(), s, x);
			if (rc)
			{
				return rc;
			}
		}
		return length() - l;
	}

	void MetaName::adjustLength(const char* const s, size_t& l)
	{
		fb_assert(s);
		if (l > MAX_SQL_IDENTIFIER_LEN)
		{
			l = MAX_SQL_IDENTIFIER_LEN;
		}
		while (l)
		{
			if (s[l - 1] != ' ')
			{
				break;
			}
			--l;
		}
	}

	void MetaName::upper7()
	{
		for (char* p = data; *p; p++)
		{
			*p = UPPER7(*p);
		}
	}

	void MetaName::lower7()
	{
		for (char* p = data; *p; p++)
		{
			*p = LOWWER7(*p);
		}
	}

	void MetaName::printf(const char* format, ...)
	{
		init();
		va_list params;
		va_start(params, format);
		int l = VSNPRINTF(data, MAX_SQL_IDENTIFIER_LEN, format, params);
		if (l < 0 || size_t(l) > MAX_SQL_IDENTIFIER_LEN)
		{
			l = MAX_SQL_IDENTIFIER_LEN;
		}
		data[l] = 0;
		count = l;
		va_end(params);
	}

	char* MetaName::getBuffer(const size_t l)
	{
		fb_assert (l < MAX_SQL_IDENTIFIER_SIZE);
		init();
		count = l;
		return data;
	}

} // namespace Firebird

