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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef COMMON_BYTE_CHUNK_H
#define COMMON_BYTE_CHUNK_H

#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"

namespace Firebird {

// Wrapper for different kinds of byte buffers.
struct ByteChunk
{
	// Separate pointer/length buffer.
	ByteChunk(const UCHAR* aData, size_t aLength)
		: data(aData),
		  length(aLength)
	{
	}

	// Array<UCHAR> buffer.
	// This constructor is intentionally not-explicit.
	template <typename Storage>
	ByteChunk(const Firebird::Array<UCHAR, Storage>& array)
		: data(array.begin()),
		  length(array.getCount())
	{
	}

	// String buffer.
	ByteChunk(string& str)
		: data((UCHAR*) str.c_str()),
		  length(str.length())
	{
	}

	// Empty.
	ByteChunk()
		: data(NULL),
		  length(0)
	{
	}

	const UCHAR* data;
	size_t length;
};

}	// namespace Firebird

#endif	// COMMON_BYTE_CHUNK_H
