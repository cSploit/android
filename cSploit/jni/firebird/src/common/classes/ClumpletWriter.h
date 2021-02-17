/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		ClumpletWriter.h
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

#ifndef CLUMPLETWRITER_H
#define CLUMPLETWRITER_H

#include "../common/classes/ClumpletReader.h"
#include "../common/classes/array.h"

// This setting of maximum dpb size doesn't mean, that we
// can't process larger DBPs! This is just recommended limit
// cause it's hard to imagine sensefull DPB of even this size.
const size_t MAX_DPB_SIZE = 1024 * 1024;

namespace Firebird {

// At the moment you can only declare it on stack, permanent objects are not allowed
class ClumpletWriter : public ClumpletReader
{
public:
	// Create empty clumplet writer.
	ClumpletWriter(Kind k, size_t limit, UCHAR tag = 0);
	ClumpletWriter(MemoryPool& pool, Kind k, size_t limit, UCHAR tag = 0);

	// Create writer from a given buffer
	ClumpletWriter(Kind k, size_t limit, const UCHAR* buffer, size_t buffLen, UCHAR tag = 0);
	ClumpletWriter(MemoryPool& pool, Kind k, size_t limit, const UCHAR* buffer, size_t buffLen, UCHAR tag = 0);

	// Create writer from a given buffer with possibly different clumplet version
	ClumpletWriter(const KindList* kl, size_t limit, const UCHAR* buffer, size_t buffLen);
	ClumpletWriter(MemoryPool& pool, const KindList* kl, size_t limit,
				   const UCHAR* buffer, size_t buffLen);

	// Create empty writer
	ClumpletWriter(const KindList* kl, size_t limit);
	ClumpletWriter(MemoryPool& pool, const KindList* kl, size_t limit);

	// Create a copy of writer
	ClumpletWriter(MemoryPool& pool, const ClumpletWriter& from);
	ClumpletWriter(const ClumpletWriter& from);

	void reset(UCHAR tag = 0);
	void reset(const UCHAR* buffer, const size_t buffLen);
	void clear();

	// Methods to create new clumplet at current position
	void insertInt(UCHAR tag, const SLONG value);
	void insertBigInt(UCHAR tag, const SINT64 value);
	void insertBytes(UCHAR tag, const void* bytes, size_t length);
	void insertString(UCHAR tag, const string& str);
	void insertPath(UCHAR tag, const PathName& str);
	void insertString(UCHAR tag, const char* str, size_t length);
	void insertByte(UCHAR tag, const UCHAR byte);
	void insertTag(UCHAR tag);
	void insertDouble(UCHAR tag, const double value);
	void insertTimeStamp(UCHAR tag, const ISC_TIMESTAMP value);
	void insertTime(UCHAR tag, ISC_TIME value) { insertInt(tag, value); }
	void insertDate(UCHAR tag, ISC_DATE value) { insertInt(tag, value); }
	void insertEndMarker(UCHAR tag);
	void insertClumplet(const SingleClumplet& clumplet);

    // Delete currently selected clumplet from buffer
	void deleteClumplet();

	// Delete all clumplets with given tag
	// Returns true if any found
	bool deleteWithTag(UCHAR tag);

	virtual const UCHAR* getBuffer() const;

protected:
	virtual const UCHAR* getBufferEnd() const;
	virtual void size_overflow();
	void insertBytesLengthCheck(UCHAR tag, const void* bytes, const size_t length);
	bool upgradeVersion();	// upgrade clumplet version - obtain newest from kindList

private:
	// Assignment not implemented.
	ClumpletWriter& operator=(const ClumpletWriter& from);

	size_t sizeLimit;
	const KindList* kindList;
	HalfStaticArray<UCHAR, 128> dynamic_buffer;

	void initNewBuffer(UCHAR tag);
	void create(const UCHAR* buffer, size_t buffLen, UCHAR tag);
	static void toVaxInteger(UCHAR* ptr, size_t length, const SINT64 value);
};

} // namespace Firebird

#endif // CLUMPLETWRITER_H
