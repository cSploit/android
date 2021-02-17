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
 *  Copyright (c) 2009 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef COMMON_CLASSES_BLR_READER_H
#define COMMON_CLASSES_BLR_READER_H

#include "iberror.h"

namespace Firebird {


class BlrReader
{
public:
	BlrReader(const UCHAR* buffer, unsigned maxLen)
		: start(buffer),
		  end(buffer + maxLen),
		  pos(buffer)
	{
		// ASF: A big maxLen like MAX_ULONG could overflow the pointer size and
		// points to something before start. In this case, we set the end to the
		// max possible address.
		if (end < start)
			end = (UCHAR*) ~U_IPTR(0);
	}

	BlrReader()
		: start(NULL),
		  end(NULL),
		  pos(NULL)
	{
	}

public:
	unsigned getLength() const
	{
		return (unsigned) (end - start);
	}

	const UCHAR* getPos() const
	{
		fb_assert(pos);
		return pos;
	}

	void setPos(const UCHAR* newPos)
	{
		fb_assert(newPos >= start && newPos < end);
		pos = newPos;
	}

	void seekBackward(unsigned n)
	{
		setPos(getPos() - n);
	}

	void seekForward(unsigned n)
	{
		setPos(getPos() + n);
	}

	unsigned getOffset() const
	{
		return getPos() - start;
	}

	UCHAR peekByte() const
	{
		fb_assert(pos);

		if (pos >= end)
			(Firebird::Arg::Gds(isc_invalid_blr) << Firebird::Arg::Num(getOffset())).raise();

		return *pos;
	}

	UCHAR getByte()
	{
		UCHAR byte = peekByte();
		++pos;
		return byte;
	}

	USHORT getWord()
	{
		const UCHAR low = getByte();
		const UCHAR high = getByte();

		return high * 256 + low;
	}

	ULONG getLong()
	{
		const UCHAR b1 = getByte();
		const UCHAR b2 = getByte();
		const UCHAR b3 = getByte();
		const UCHAR b4 = getByte();

		return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
	}

	UCHAR checkByte(UCHAR expected)
	{
		using namespace Firebird;

		UCHAR byte = getByte();

		if (byte != expected)
		{
			status_exception::raise(Arg::Gds(isc_syntaxerr) <<
				Arg::Num(expected) <<
				Arg::Num(getOffset() - 1) <<
				Arg::Num(byte));
		}

		return byte;
	}

	USHORT checkWord(USHORT expected)
	{
		using namespace Firebird;

		USHORT word = getWord();

		if (word != expected)
		{
			status_exception::raise(Arg::Gds(isc_syntaxerr) <<
				Arg::Num(expected) <<
				Arg::Num(getOffset() - 2) <<
				Arg::Num(word));
		}

		return word;
	}

private:
	const UCHAR* start;
	const UCHAR* end;
	const UCHAR* pos;
};


}	// namespace

#endif	// COMMON_CLASSES_BLR_READER_H
