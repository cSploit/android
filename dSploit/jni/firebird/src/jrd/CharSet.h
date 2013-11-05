/*
 *	PROGRAM:	JRD International support
 *	MODULE:		CharSet.h
 *	DESCRIPTION:	International text handling definitions
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
 *  2006.10.10 Adriano dos Santos Fernandes - refactored from intl_classes.h
 *
 */

#ifndef JRD_CHARSET_H
#define JRD_CHARSET_H

#include "CsConvert.h"

namespace Jrd {

class CharSet
{
public:
	class Delete
	{
	public:
		static void clear(charset* cs)
		{
			if (cs->charset_fn_destroy)
				cs->charset_fn_destroy(cs);
			delete cs;
		}
	};

	static CharSet* createInstance(Firebird::MemoryPool& pool, USHORT id, charset* cs);

protected:
	CharSet(USHORT _id, charset* _cs)
		: id(_id),
		  cs(_cs)
	{
		sqlMatchAnyLength = getConvFromUnicode().convert(
			sizeof(SQL_MATCH_ANY_CHARS), &SQL_MATCH_ANY_CHARS, sizeof(sqlMatchAny), sqlMatchAny);
		sqlMatchOneLength = getConvFromUnicode().convert(
			sizeof(SQL_MATCH_1_CHAR), &SQL_MATCH_1_CHAR, sizeof(sqlMatchOne), sqlMatchOne);
	}

private:
	CharSet(const CharSet&);	// Not implemented

public:
	virtual ~CharSet() {}

	USHORT getId() const { return id; }
	const char* getName() const { return cs->charset_name; }
	UCHAR minBytesPerChar() const { return cs->charset_min_bytes_per_char; }
	UCHAR maxBytesPerChar() const { return cs->charset_max_bytes_per_char; }
	UCHAR getSpaceLength() const { return cs->charset_space_length; }
	const UCHAR* getSpace() const { return cs->charset_space_character; }
	USHORT getFlags() const { return cs->charset_flags; }
	bool shouldCheckWellFormedness() const { return cs->charset_fn_well_formed != NULL; }

	bool isMultiByte() const
	{
		return cs->charset_min_bytes_per_char != cs->charset_max_bytes_per_char;
	}

	bool wellFormed(ULONG len, const UCHAR* str, ULONG* offendingPos = NULL) const
	{
		ULONG offendingPos2;

		if (offendingPos == NULL)
			offendingPos = &offendingPos2;

		if (cs->charset_fn_well_formed)
			return cs->charset_fn_well_formed(cs, len, str, offendingPos);

		return true;
	}

	CsConvert getConvToUnicode() const { return CsConvert(cs, NULL); }
	CsConvert getConvFromUnicode() const { return CsConvert(NULL, cs); }

	void destroy()
	{
		if (cs->charset_fn_destroy)
			cs->charset_fn_destroy(cs);
	}

	const UCHAR* getSqlMatchAny() const { return sqlMatchAny; }
	const UCHAR* getSqlMatchOne() const { return sqlMatchOne; }
	BYTE getSqlMatchAnyLength() const { return sqlMatchAnyLength; }
	BYTE getSqlMatchOneLength() const { return sqlMatchOneLength; }

	charset* getStruct() const { return cs; }

	ULONG removeTrailingSpaces(ULONG srcLen, const UCHAR* src) const
	{
		const UCHAR* p = src + srcLen - getSpaceLength();

		while (p >= src && memcmp(p, getSpace(), getSpaceLength()) == 0)
			p -= getSpaceLength();

		p += getSpaceLength();

		return p - src;
	}

	virtual ULONG length(ULONG srcLen, const UCHAR* src, bool countTrailingSpaces) const = 0;
	virtual ULONG substring(ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst,
							ULONG startPos, ULONG length) const = 0;

private:
	USHORT id;
	charset* cs;
	UCHAR sqlMatchAny[sizeof(ULONG)];
	UCHAR sqlMatchOne[sizeof(ULONG)];
	BYTE sqlMatchAnyLength;
	BYTE sqlMatchOneLength;
};

}	// namespace Jrd


#endif	// JRD_CHARSET_H
