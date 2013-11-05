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
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_FILE_H
#define CLASSES_FILE_H

#include "../common/classes/array.h"

#if !defined(SOLARIS) && !defined(AIX)
typedef FB_UINT64 offset_t;
#endif

namespace Firebird {

class File
{
public:
	virtual ~File() {}

	virtual size_t read(offset_t, void*, size_t) = 0;
	virtual size_t write(offset_t, const void*, size_t) = 0;

	virtual void unlink() = 0;

	virtual offset_t getSize() const = 0;
};

class ZeroBuffer
{
	static const size_t DEFAULT_SIZE = 1024 * 256;

public:
	explicit ZeroBuffer(MemoryPool& p, size_t size = DEFAULT_SIZE)
		: buffer(p)
	{
		memset(buffer.getBuffer(size), 0, size);
	}

	const char* getBuffer() const { return buffer.begin(); }
	size_t getSize() const { return buffer.getCount(); }

private:
	Firebird::Array<char> buffer;
};

} // namespace

#endif // CLASSES_FILE_H
