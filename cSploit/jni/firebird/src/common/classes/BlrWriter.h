/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 * Adriano dos Santos Fernandes - refactored from others modules.
 */

#ifndef COMMON_CLASSES_BLR_WRITER_H
#define COMMON_CLASSES_BLR_WRITER_H

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"
#include "../common/StatusArg.h"

namespace Firebird {

// BLR/DYN writer.
class BlrWriter : public Firebird::PermanentStorage
{
public:
	typedef Firebird::HalfStaticArray<UCHAR, 1024> BlrData;

	explicit BlrWriter(MemoryPool& p)
		: PermanentStorage(p),
		  blrData(p),
		  baseOffset(0)
	{
	}

	virtual ~BlrWriter()
	{
	}

	void appendUChar(const UCHAR byte)
	{
		blrData.add(byte);
	}

	// Cram a word into the blr buffer.
	void appendUShort(USHORT word)
	{
		appendUChar(word);
		appendUChar(word >> 8);
	}

	void appendULong(ULONG val)
	{
		appendUShort(val);
		appendUShort(val >> 16);
	}

	void appendUCharRepeated(UCHAR byte, int count)
	{
		for (int i = 0; i < count; ++i)
			appendUChar(byte);
	}

	void appendBytes(const UCHAR* string, size_t len)
	{
		blrData.add(string, len);
	}

	// Write out a string with one byte of length.
	void appendNullString(const char* string)
	{
		size_t len = strlen(string);

		// CVC: Maybe the Release version should truncate "len" to 255?
		fb_assert(len <= MAX_UCHAR);

		appendUChar(static_cast<USHORT>(len));
		appendBytes(reinterpret_cast<const UCHAR*>(string), static_cast<USHORT>(len));
	}

	// Write out a string valued attribute.
	void appendNullString(UCHAR verb, const char* string)
	{
		const USHORT length = string ? static_cast<USHORT>(strlen(string)) : 0;
		appendString(verb, string, length);
	}

	// Write out a string in metadata charset with one byte of length.
	void appendMetaString(const char* string)
	{
		appendString(0, string, static_cast<USHORT>(strlen(string)));
	}

	void appendString(UCHAR verb, const char* string, USHORT len);

	void appendString(UCHAR verb, const Firebird::MetaName& name)
	{
		appendString(verb, name.c_str(), static_cast<USHORT>(name.length()));
	}

	void appendString(UCHAR verb, const Firebird::string& name)
	{
		appendString(verb, name.c_str(), static_cast<USHORT>(name.length()));
	}

	void appendNumber(UCHAR verb, SSHORT number);
	void appendUShortWithLength(USHORT val);
	void appendULongWithLength(ULONG val);
	void appendVersion();

	void beginBlr(UCHAR verb);
	void endBlr();

	BlrData& getBlrData() { return blrData; }

	ULONG getBaseOffset() const { return baseOffset; }
	void setBaseOffset(ULONG value) { baseOffset = value; }

	virtual bool isVersion4() = 0;
	virtual void raiseError(const Arg::StatusVector& vector);

private:
	BlrData blrData;
	ULONG baseOffset;					// place to go back and stuff in blr length
};


} // namespace

#endif // COMMON_CLASSES_BLR_WRITER_H
