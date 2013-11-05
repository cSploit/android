/*
 *	PROGRAM:	JRD International support
 *	MODULE:		TextType.h
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

#ifndef JRD_TEXTTYPE_H
#define JRD_TEXTTYPE_H

struct texttype;

namespace Jrd {

class CharSet;

class TextType
{
public:
	TextType(TTYPE_ID _type, texttype *_tt, CharSet* _cs);

private:
	TextType(const TextType&);	// Not implemented

public:
	virtual ~TextType() {}

	USHORT key_length(USHORT len);

	USHORT string_to_key(USHORT srcLen,
						 const UCHAR* src,
						 USHORT dstLen,
						 UCHAR* dst,
						 USHORT key_type);

	SSHORT compare(ULONG len1,
				   const UCHAR* str1,
				   ULONG len2,
				   const UCHAR* str2);

	ULONG str_to_upper(ULONG srcLen,
					   const UCHAR* src,
					   ULONG dstLen,
					   UCHAR* dst);

	ULONG str_to_lower(ULONG srcLen,
					   const UCHAR* src,
					   ULONG dstLen,
					   UCHAR* dst);

	ULONG canonical(ULONG srcLen,
					const UCHAR* src,
					ULONG dstLen,
					UCHAR* dst);

	USHORT getType() const
	{
		return type;
	}

	CharSet* getCharSet() const
	{
		return cs;
	}

	BYTE getCanonicalWidth() const
	{
		return tt->texttype_canonical_width;
	}

	USHORT getFlags() const
	{
		return tt->texttype_flags;
	}

public:
	Firebird::MetaName name;

protected:
	texttype* tt;
	CharSet* cs;

private:
	TTYPE_ID type;

public:
	enum
	{
		CHAR_ASTERISK = 0,
		CHAR_AT,
		CHAR_CIRCUMFLEX,
		CHAR_COLON,
		CHAR_COMMA,
		CHAR_EQUAL,
		CHAR_MINUS,
		CHAR_PERCENT,
		CHAR_PLUS,
		CHAR_QUESTION_MARK,
		CHAR_SPACE,
		CHAR_TILDE,
		CHAR_UNDERLINE,
		CHAR_VERTICAL_BAR,
		CHAR_OPEN_BRACE,
		CHAR_CLOSE_BRACE,
		CHAR_OPEN_BRACKET,
		CHAR_CLOSE_BRACKET,
		CHAR_OPEN_PAREN,
		CHAR_CLOSE_PAREN,
		CHAR_LOWER_S,
		CHAR_UPPER_S,
		CHAR_COUNT,		// last

		// aliases
		CHAR_SQL_MATCH_ANY = CHAR_PERCENT,
		CHAR_SQL_MATCH_ONE = CHAR_UNDERLINE
	};

	const UCHAR* getCanonicalChar(unsigned int ch) const
	{
		return reinterpret_cast<const UCHAR*>(&canonicalChars[ch]);
	}

	const UCHAR* getCanonicalNumbers(int* count = NULL) const
	{
		if (count)
			*count = 10;
		return reinterpret_cast<const UCHAR*>(canonicalNumbers);
	}

	const UCHAR* getCanonicalLowerLetters(int* count = NULL) const
	{
		if (count)
			*count = 26;
		return reinterpret_cast<const UCHAR*>(canonicalLowerLetters);
	}

	const UCHAR* getCanonicalUpperLetters(int* count = NULL) const
	{
		if (count)
			*count = 26;
		return reinterpret_cast<const UCHAR*>(canonicalUpperLetters);
	}

	const UCHAR* getCanonicalWhiteSpaces(int* count = NULL) const
	{
		if (count)
			*count = 6;
		return reinterpret_cast<const UCHAR*>(canonicalWhiteSpaces);
	}

	const UCHAR* getCanonicalSpace(int* count = NULL) const
	{
		if (count)
			*count = 1;
		return getCanonicalChar(CHAR_SPACE);
	}

private:
	ULONG canonicalChars[CHAR_COUNT];
	ULONG canonicalNumbers[10];
	ULONG canonicalLowerLetters[26];
	ULONG canonicalUpperLetters[26];
	ULONG canonicalWhiteSpaces[6];
};

}	// namespace Jrd


#endif	// JRD_TEXTTYPE_H
