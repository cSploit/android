/*
 *	PROGRAM:	Template to define a local vary stack variable.
 *	MODULE:		VaryStr.h
 *	DESCRIPTION:	Both automatic and dynamic memory allocation is available.
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Alex Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef CLASSES_VARYSTR_H
#define CLASSES_VARYSTR_H

#include "../common/classes/alloc.h"

namespace Firebird {

template <size_t x>
class VaryStr : public vary
{
	char vary_tail[x - 1];

public:
	VaryStr()
	{
		vary_length = 0;
		vary_string[0] = 0;
	}
};

template <size_t x>
class DynamicVaryStr : public VaryStr<x>
{
	vary* buffer;

	void clear()
	{
		delete[] reinterpret_cast<char*>(buffer);
	}

public:
	DynamicVaryStr() : buffer(NULL) { }

	// It does not preserve string data! Not hard to do, but not required today. AP, 2009.
	vary* getBuffer(size_t len)
	{
		if (len <= x)
		{
			return this;
		}

		clear();
		buffer = reinterpret_cast<vary*>(FB_NEW(*getDefaultMemoryPool()) char[len + sizeof(USHORT)]);
		buffer->vary_length = 0;
		buffer->vary_string[0] = 0;

		return buffer;
	}

	~DynamicVaryStr()
	{
		clear();
	}
};

}

#endif // CLASSES_VARYSTR_H
