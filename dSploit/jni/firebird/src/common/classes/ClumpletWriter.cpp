/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		ClumpletWriter.cpp
 *	DESCRIPTION:	Secure handling of clumplet buffers
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *

 */

#include "firebird.h"

#include "../common/classes/ClumpletWriter.h"
#include "fb_exception.h"

#include "../jrd/ibase.h"

namespace Firebird {

ClumpletWriter::ClumpletWriter(Kind k, size_t limit, UCHAR tag) :
	ClumpletReader(k, NULL, 0), sizeLimit(limit), dynamic_buffer(getPool())
{
	initNewBuffer(tag);
	rewind();
}

ClumpletWriter::ClumpletWriter(MemoryPool& given_pool, Kind k, size_t limit, UCHAR tag) :
	ClumpletReader(given_pool, k, NULL, 0), sizeLimit(limit), dynamic_buffer(getPool())
{
	initNewBuffer(tag);
	rewind();
}

const UCHAR* ClumpletWriter::getBuffer() const
{
	return dynamic_buffer.begin();
}

const UCHAR* ClumpletWriter::getBufferEnd() const
{
	return dynamic_buffer.end();
}

void ClumpletWriter::initNewBuffer(UCHAR tag)
{
	switch (kind)
	{
		case SpbAttach:
			if (tag != isc_spb_version1)
			{
				dynamic_buffer.push(isc_spb_version);
			}
			dynamic_buffer.push(tag);
			break;
		case Tagged:
		case Tpb:
		case WideTagged:
			dynamic_buffer.push(tag);
			break;
		default:
			break;
	}
}

ClumpletWriter::ClumpletWriter(Kind k, size_t limit, const UCHAR* buffer, size_t buffLen, UCHAR tag) :
	ClumpletReader(k, NULL, 0), sizeLimit(limit), dynamic_buffer(getPool())
{
	if (buffer && buffLen) {
		dynamic_buffer.push(buffer, buffLen);
	}
	else {
		initNewBuffer(tag);
	}
	rewind();
}

ClumpletWriter::ClumpletWriter(MemoryPool& given_pool, Kind k, size_t limit,
							   const UCHAR* buffer, size_t buffLen, UCHAR tag) :
	ClumpletReader(given_pool, k, NULL, 0), sizeLimit(limit), dynamic_buffer(getPool())
{
	if (buffer && buffLen) {
		dynamic_buffer.push(buffer, buffLen);
	}
	else {
		initNewBuffer(tag);
	}
	rewind();
}

void ClumpletWriter::reset(UCHAR tag)
{
	dynamic_buffer.shrink(0);
	initNewBuffer(tag);
	rewind();
}

void ClumpletWriter::reset(const UCHAR* buffer, const size_t buffLen)
{
	dynamic_buffer.clear();
	if (buffer && buffLen) {
		dynamic_buffer.push(buffer, buffLen);
	}
	else
	{
		UCHAR tag = (kind == SpbStart || kind == UnTagged || kind == WideUnTagged) ? getBufferTag() : 0;
		initNewBuffer(tag);
	}
	rewind();
}

void ClumpletWriter::size_overflow()
{
	fatal_exception::raise("Clumplet buffer size limit reached");
}

void ClumpletWriter::toVaxInteger(UCHAR* ptr, size_t length, const SINT64 value)
{
	fb_assert(ptr && length > 0 && length < 9); // We can't handle numbers bigger than int64.
	int shift = 0;
	while (length--)
	{
		*ptr++ = (UCHAR) (value >> shift);
		shift += 8;
	}
}

void ClumpletWriter::insertInt(UCHAR tag, const SLONG value)
{
	UCHAR bytes[sizeof(SLONG)];

	toVaxInteger(bytes, sizeof(bytes), value);
	insertBytesLengthCheck(tag, bytes, sizeof(bytes));
}

void ClumpletWriter::insertBigInt(UCHAR tag, const SINT64 value)
{
	UCHAR bytes[sizeof(SINT64)];

	toVaxInteger(bytes, sizeof(bytes), value);
	insertBytesLengthCheck(tag, bytes, sizeof(bytes));
}

void ClumpletWriter::insertDouble(UCHAR tag, const double value)
{
	union {
		double temp_double;
		SLONG temp_long[2];
	} temp;

	fb_assert(sizeof(double) == sizeof(temp));

	temp.temp_double = value;
	UCHAR bytes[sizeof(double)];
	toVaxInteger(bytes, sizeof(SLONG), temp.temp_long[FB_LONG_DOUBLE_FIRST]);
	toVaxInteger(bytes + sizeof(SLONG), sizeof(SLONG), temp.temp_long[FB_LONG_DOUBLE_SECOND]);
	insertBytesLengthCheck(tag, bytes, sizeof(bytes));
}

void ClumpletWriter::insertTimeStamp(UCHAR tag, const ISC_TIMESTAMP value)
{
	UCHAR bytes[sizeof(ISC_TIMESTAMP)];
	toVaxInteger(bytes, sizeof(SLONG), value.timestamp_date);
	toVaxInteger(bytes + sizeof(SLONG), sizeof(SLONG), value.timestamp_time);
	insertBytesLengthCheck(tag, bytes, sizeof(bytes));
}

void ClumpletWriter::insertString(UCHAR tag, const string& str)
{
	insertString(tag, str.c_str(), str.length());
}

void ClumpletWriter::insertPath(UCHAR tag, const PathName& str)
{
	insertString(tag, str.c_str(), str.length());
}

void ClumpletWriter::insertString(UCHAR tag, const char* str, size_t length)
{
	insertBytesLengthCheck(tag, reinterpret_cast<const UCHAR*>(str), length);
}

void ClumpletWriter::insertBytes(UCHAR tag, const UCHAR* bytes, size_t length)
{
	insertBytesLengthCheck(tag, bytes, length);
}

void ClumpletWriter::insertByte(UCHAR tag, const UCHAR byte)
{
	insertBytesLengthCheck(tag, &byte, 1);
}

void ClumpletWriter::insertBytesLengthCheck(UCHAR tag, const UCHAR* bytes, const size_t length)
{
	// Check that we're not beyond the end of buffer.
	// We get there when we set end marker.
	if (cur_offset > dynamic_buffer.getCount())
	{
		usage_mistake("write past EOF");
		return;
	}

	// Check length according to clumplet type
	const ClumpletType t = getClumpletType(tag);
	UCHAR lenSize = 0;
	switch (t)
	{
	case Wide:
		if (length > MAX_ULONG)
		{
			string m;
			m.printf("attempt to store %d bytes in a clumplet", length);
			usage_mistake(m.c_str());
			return;
		}
		lenSize = 4;
		break;
	case TraditionalDpb:
		if (length > MAX_UCHAR)
		{
			string m;
			m.printf("attempt to store %d bytes in a clumplet with maximum size 255 bytes", length);
			usage_mistake(m.c_str());
			return;
		}
		lenSize = 1;
		break;
	case SingleTpb:
		if (length > 0)
		{
        	usage_mistake("attempt to store data in dataless clumplet");
			return;
		}
		break;
	case StringSpb:
		if (length > MAX_USHORT)
		{
			string m;
			m.printf("attempt to store %d bytes in a clumplet", length);
			usage_mistake(m.c_str());
			return;
		}
		lenSize = 2;
		break;
	case IntSpb:
		if (length != 4)
		{
			string m;
			m.printf("attempt to store %d bytes in a clumplet, need 4", length);
			usage_mistake(m.c_str());
			return;
		}
		break;
	case ByteSpb:
		if (length != 1)
		{
			string m;
			m.printf("attempt to store %d bytes in a clumplet, need 1", length);
			usage_mistake(m.c_str());
			return;
		}
		break;
	}

	// Check that resulting data doesn't overflow size limit
	if (dynamic_buffer.getCount() + length + lenSize + 1 > sizeLimit) {
		size_overflow();
	}

	// Insert the data
	const size_t saved_offset = cur_offset;
	dynamic_buffer.insert(cur_offset++, tag);
	switch (lenSize)
	{
	case 1:
		dynamic_buffer.insert(cur_offset++, static_cast<UCHAR>(length));
		break;
	case 2:
		{
			UCHAR b[2];
			toVaxInteger(b, sizeof(b), length);
			dynamic_buffer.insert(cur_offset, b, sizeof(b));
			cur_offset += 2;
		}
		break;
	case 4:
		{
			UCHAR b[4];
			toVaxInteger(b, sizeof(b), length);
			dynamic_buffer.insert(cur_offset, b, sizeof(b));
			cur_offset += 4;
		}
		break;
	}
	dynamic_buffer.insert(cur_offset, bytes, length);
	const size_t new_offset = cur_offset + length;
	cur_offset = saved_offset;
    adjustSpbState();
	cur_offset = new_offset;
}

void ClumpletWriter::insertTag(UCHAR tag)
{
	insertBytesLengthCheck(tag, 0, 0);
}


void ClumpletWriter::insertEndMarker(UCHAR tag)
{
	// Check that we're not beyond the end of buffer.
	// We get there when we set end marker.
	if (cur_offset > dynamic_buffer.getCount())
	{
		usage_mistake("write past EOF");
		return;
	}

	// Check that resulting data doesn't overflow size limit
	if (cur_offset + 1 > sizeLimit) {
		size_overflow();
	}

	dynamic_buffer.shrink(cur_offset);
	dynamic_buffer.push(tag);

	cur_offset += 2; // Go past EOF to indicate we set the marker
}

void ClumpletWriter::deleteClumplet()
{
	const UCHAR* clumplet = getBuffer() + cur_offset;
	const UCHAR* buffer_end = getBufferEnd();

	// Check for EOF
	if (clumplet >= buffer_end)
	{
		usage_mistake("write past EOF");
		return;
	}

	if (buffer_end - clumplet < 2)
	{
		// It appears we're erasing EOF marker
		dynamic_buffer.shrink(cur_offset);
	}
	else {
		dynamic_buffer.removeCount(cur_offset, getClumpletSize(true, true, true));
	}
}

bool ClumpletWriter::deleteWithTag(UCHAR tag)
{
   bool rc = false;
   while (find(tag))
   {
       rc = true;
       deleteClumplet();
   }

   return rc;
}

} // namespace
