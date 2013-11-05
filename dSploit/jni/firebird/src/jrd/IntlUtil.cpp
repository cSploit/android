/*
 *	PROGRAM:	JRD International support
 *	MODULE:		IntlUtil.cpp
 *	DESCRIPTION:	INTL Utility functions
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/IntlUtil.h"
#include "../jrd/unicode_util.h"
#include "../jrd/intl_classes.h"
#include "../intl/country_codes.h"
#include "../common/classes/auto.h"
#include "../common/classes/Aligner.h"


using Jrd::UnicodeUtil;


namespace
{
	struct TextTypeImpl
	{
		TextTypeImpl(charset* a_cs, UnicodeUtil::Utf16Collation* a_collation)
			: cs(a_cs),
			  collation(a_collation)
		{
		}

		~TextTypeImpl()
		{
			if (cs->charset_fn_destroy)
				cs->charset_fn_destroy(cs);

			delete cs;
			delete collation;
		}

		charset* cs;
		UnicodeUtil::Utf16Collation* collation;
	};
}


namespace Firebird {


static void unicodeDestroy(texttype* tt);
static USHORT unicodeKeyLength(texttype* tt, USHORT len);
static USHORT unicodeStrToKey(texttype* tt, USHORT srcLen, const UCHAR* src,
	USHORT dstLen, UCHAR* dst, USHORT keyType);
static SSHORT unicodeCompare(texttype* tt, ULONG len1, const UCHAR* str1,
	ULONG len2, const UCHAR* str2, INTL_BOOL* errorFlag);
static ULONG unicodeCanonical(texttype* tt, ULONG srcLen, const UCHAR* src,
	ULONG dstLen, UCHAR* dst);


GlobalPtr<IntlUtil::Utf8CharSet> IntlUtil::utf8CharSet;

IntlUtil::Utf8CharSet::Utf8CharSet(MemoryPool& pool)
{
	IntlUtil::initUtf8Charset(&obj);
	charSet = Jrd::CharSet::createInstance(pool, CS_UTF8, &obj);
}


string IntlUtil::generateSpecificAttributes(Jrd::CharSet* cs, SpecificAttributesMap& map)
{
	SpecificAttributesMap::Accessor accessor(&map);

	bool found = accessor.getFirst();
	string s;

	while (found)
	{
		UCHAR c[sizeof(ULONG)];
		ULONG size;

		SpecificAttribute* attribute = accessor.current();

		s += escapeAttribute(cs, attribute->first);

		const USHORT equalChar = '=';

		size = cs->getConvFromUnicode().convert(
			sizeof(equalChar), (const UCHAR*) &equalChar, sizeof(c), c);

		s += string((const char*) &c, size);

		s += escapeAttribute(cs, attribute->second);

		found = accessor.getNext();

		if (found)
		{
			const USHORT semiColonChar = ';';
			size = cs->getConvFromUnicode().convert(
				sizeof(semiColonChar), (const UCHAR*) &semiColonChar, sizeof(c), c);

			s += string((const char*) &c, size);
		}
	}

	return s;
}


bool IntlUtil::parseSpecificAttributes(Jrd::CharSet* cs, ULONG len, const UCHAR* s,
									   SpecificAttributesMap* map)
{
	// Note that the map isn't cleared.
	// Old attributes will be combined with the new ones.

	const UCHAR* p = s;
	const UCHAR* const end = s + len;
	ULONG size = 0;

	readAttributeChar(cs, &p, end, &size, true);

	while (p < end)
	{
		while (p < end && size == cs->getSpaceLength() &&
			memcmp(p, cs->getSpace(), cs->getSpaceLength()) == 0)
		{
			if (!readAttributeChar(cs, &p, end, &size, true))
				return true;
		}

		const UCHAR* start = p;

		UCHAR uc[sizeof(ULONG)];
		ULONG uSize;

		while (p < end)
		{
			uSize = cs->getConvToUnicode().convert(size, p, sizeof(uc), uc);

			if (uSize == 2 &&
				((*(USHORT*) uc >= 'A' && *(USHORT*) uc <= 'Z') ||
					(*(USHORT*) uc >= 'a' && *(USHORT*) uc <= 'z') ||
					*(USHORT*) uc == '-' || *(USHORT*) uc == '_'))
			{
				if (!readAttributeChar(cs, &p, end, &size, true))
					return false;
			}
			else
				break;
		}

		if (p - start == 0)
			return false;

		string name = string((const char*)start, p - start);
		name = unescapeAttribute(cs, name);

		while (p < end && size == cs->getSpaceLength() &&
			memcmp(p, cs->getSpace(), cs->getSpaceLength()) == 0)
		{
			if (!readAttributeChar(cs, &p, end, &size, true))
				return false;
		}

		uSize = cs->getConvToUnicode().convert(size, p, sizeof(uc), uc);

		if (uSize != 2 || *(USHORT*)uc != '=')
			return false;

		string value;

		if (readAttributeChar(cs, &p, end, &size, true))
		{
			while (p < end && size == cs->getSpaceLength() &&
				memcmp(p, cs->getSpace(), cs->getSpaceLength()) == 0)
			{
				if (!readAttributeChar(cs, &p, end, &size, true))
					return false;
			}

			const UCHAR* endNoSpace = start = p;

			while (p < end)
			{
				uSize = cs->getConvToUnicode().convert(size, p, sizeof(uc), uc);

				if (uSize != 2 || *(USHORT*)uc != ';')
				{
					if (!(size == cs->getSpaceLength() &&
							memcmp(p, cs->getSpace(), cs->getSpaceLength()) == 0))
					{
						endNoSpace = p + size;
					}

					if (!readAttributeChar(cs, &p, end, &size, true))
						break;
				}
				else
					break;
			}

			value = unescapeAttribute(cs, string((const char*)start, endNoSpace - start));

			if (p < end)
				readAttributeChar(cs, &p, end, &size, true);	// skip the semicolon
		}

		if (value.isEmpty())
			map->remove(name);
		else
			map->put(name, value);
	}

	return true;
}


string IntlUtil::convertAsciiToUtf16(const string& ascii)
{
	string s;
	const char* end = ascii.c_str() + ascii.length();

	for (const char* p = ascii.c_str(); p < end; ++p)
	{
		USHORT c = *(UCHAR*) p;
		s.append((char*) &c, sizeof(c));
	}

	return s;
}


string IntlUtil::convertUtf16ToAscii(const string& utf16, bool* error)
{
	fb_assert(utf16.length() % sizeof(USHORT) == 0);

	string s;
	const USHORT* end = (const USHORT*) (utf16.c_str() + utf16.length());

	for (const USHORT* p = (const USHORT*) utf16.c_str(); p < end; ++p)
	{
		if (*p <= 0xFF)
			s.append(1, (UCHAR) *p);
		else
		{
			*error = true;
			return "";
		}
	}

	*error = false;

	return s;
}


ULONG IntlUtil::cvtAsciiToUtf16(csconvert* obj, ULONG nSrc, const UCHAR* pSrc,
	ULONG nDest, UCHAR* ppDest, USHORT* err_code, ULONG* err_position)
{
/**************************************
 *
 *      c v t A s c i i T o U t f 1 6
 *
 **************************************
 *
 * Functional description
 *      Convert CHARACTER SET ASCII to UTF-16.
 *      Byte values below 128 treated as ASCII.
 *      Byte values >= 128 create BAD_INPUT
 *
 *************************************/
	fb_assert(obj != NULL);
	fb_assert((pSrc != NULL) || (ppDest == NULL));
	fb_assert(err_code != NULL);

	*err_code = 0;
	if (ppDest == NULL)			/* length estimate needed? */
		return (2 * nSrc);

	Firebird::OutAligner<USHORT> d(ppDest, nDest);
	USHORT* pDest = d;

	const USHORT* const pStart = pDest;
	const UCHAR* const pStart_src = pSrc;
	while (nDest >= sizeof(*pDest) && nSrc >= sizeof(*pSrc))
	{
		if (*pSrc > 127)
		{
			*err_code = CS_BAD_INPUT;
			break;
		}
		*pDest++ = *pSrc++;
		nDest -= sizeof(*pDest);
		nSrc -= sizeof(*pSrc);
	}
	if (!*err_code && nSrc) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = (pSrc - pStart_src) * sizeof(*pSrc);

	return ((pDest - pStart) * sizeof(*pDest));
}


ULONG IntlUtil::cvtUtf16ToAscii(csconvert* obj, ULONG nSrc, const UCHAR* ppSrc,
	ULONG nDest, UCHAR* pDest, USHORT* err_code, ULONG* err_position)
{
/**************************************
 *
 *      c v t U t f 1 6 T o A s c i i
 *
 **************************************
 *
 * Functional description
 *      Convert UTF16 to CHARACTER SET ASCII.
 *      Byte values below 128 treated as ASCII.
 *      Byte values >= 128 create CONVERT_ERROR
 *
 *************************************/
	fb_assert(obj != NULL);
	fb_assert((ppSrc != NULL) || (pDest == NULL));
	fb_assert(err_code != NULL);

	*err_code = 0;
	if (pDest == NULL)			/* length estimate needed? */
		return (nSrc / 2);

	Firebird::Aligner<USHORT> s(ppSrc, nSrc);
	const USHORT* pSrc = s;

	const UCHAR* const pStart = pDest;
	const USHORT* const pStart_src = pSrc;
	while (nDest >= sizeof(*pDest) && nSrc >= sizeof(*pSrc))
	{
		if (*pSrc > 127)
		{
			*err_code = CS_CONVERT_ERROR;
			break;
		}
		*pDest++ = *pSrc++;
		nDest -= sizeof(*pDest);
		nSrc -= sizeof(*pSrc);
	}
	if (!*err_code && nSrc) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = (pSrc - pStart_src) * sizeof(*pSrc);

	return ((pDest - pStart) * sizeof(*pDest));
}


ULONG IntlUtil::cvtUtf8ToUtf16(csconvert* obj, ULONG nSrc, const UCHAR* pSrc,
	ULONG nDest, UCHAR* ppDest, USHORT* err_code, ULONG* err_position)
{
	fb_assert(obj != NULL);
	fb_assert(obj->csconvert_fn_convert == cvtUtf8ToUtf16);
	return UnicodeUtil::utf8ToUtf16(nSrc, pSrc,
		nDest, Firebird::OutAligner<USHORT>(ppDest, nDest), err_code, err_position);
}


ULONG IntlUtil::cvtUtf16ToUtf8(csconvert* obj, ULONG nSrc, const UCHAR* ppSrc,
	ULONG nDest, UCHAR* pDest, USHORT* err_code, ULONG* err_position)
{
	fb_assert(obj != NULL);
	fb_assert(obj->csconvert_fn_convert == cvtUtf16ToUtf8);
	return UnicodeUtil::utf16ToUtf8(nSrc, Firebird::Aligner<USHORT>(ppSrc, nSrc),
		nDest, pDest, err_code, err_position);
}


INTL_BOOL IntlUtil::utf8WellFormed(charset* cs, ULONG len, const UCHAR* str,
	ULONG* offendingPos)
{
	fb_assert(cs != NULL);
	return UnicodeUtil::utf8WellFormed(len, str, offendingPos);
}


void IntlUtil::initAsciiCharset(charset* cs)
{
	initNarrowCharset(cs, "ASCII");
	initConvert(&cs->charset_to_unicode, cvtAsciiToUtf16);
	initConvert(&cs->charset_from_unicode, cvtUtf16ToAscii);
}


void IntlUtil::initUtf8Charset(charset* cs)
{
	initNarrowCharset(cs, "UTF8");
	cs->charset_max_bytes_per_char = 4;
	cs->charset_fn_well_formed = utf8WellFormed;

	initConvert(&cs->charset_to_unicode, cvtUtf8ToUtf16);
	initConvert(&cs->charset_from_unicode, cvtUtf16ToUtf8);
}


void IntlUtil::initConvert(csconvert* cvt, pfn_INTL_convert func)
{
	memset(cvt, 0, sizeof(*cvt));
	cvt->csconvert_version = CSCONVERT_VERSION_1;
	cvt->csconvert_name = "DIRECT";
	cvt->csconvert_fn_convert = func;
}


void IntlUtil::initNarrowCharset(charset* cs, const ASCII* name)
{
	memset(cs, 0, sizeof(*cs));
	cs->charset_version = CHARSET_VERSION_1;
	cs->charset_name = name;
	cs->charset_flags |= CHARSET_ASCII_BASED;
	cs->charset_min_bytes_per_char = 1;
	cs->charset_max_bytes_per_char = 1;
	cs->charset_space_length = 1;
	cs->charset_space_character = (const BYTE*) " ";
	cs->charset_fn_well_formed = NULL;
}


bool IntlUtil::initUnicodeCollation(texttype* tt, charset* cs, const ASCII* name,
	USHORT attributes, const UCharBuffer& specificAttributes, const string& configInfo)
{
	memset(tt, 0, sizeof(*tt));

	// name comes from stack. Copy it.
	ASCII* nameCopy = new ASCII[strlen(name) + 1];
	strcpy(nameCopy, name);
	tt->texttype_name = nameCopy;

	tt->texttype_version = TEXTTYPE_VERSION_1;
	tt->texttype_country = CC_INTL;
	tt->texttype_canonical_width = 4;	// UTF-32
	tt->texttype_fn_destroy = unicodeDestroy;
	tt->texttype_fn_compare = unicodeCompare;
	tt->texttype_fn_key_length = unicodeKeyLength;
	tt->texttype_fn_string_to_key = unicodeStrToKey;
	tt->texttype_fn_canonical = unicodeCanonical;

	IntlUtil::SpecificAttributesMap map;

	Jrd::CharSet* charSet = NULL;

	try
	{
		charSet = Jrd::CharSet::createInstance(*getDefaultMemoryPool(), 0, cs);
		IntlUtil::parseSpecificAttributes(charSet, specificAttributes.getCount(),
			specificAttributes.begin(), &map);
		delete charSet;
	}
	catch (...)
	{
		delete charSet;
		return false;
	}

	IntlUtil::SpecificAttributesMap map16;

	SpecificAttributesMap::Accessor accessor(&map);

	bool found = accessor.getFirst();

	while (found)
	{
		UCharBuffer s1, s2;
		USHORT errCode;
		ULONG errPosition;

		s1.resize(cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode, accessor.current()->first.length(), NULL, 0, NULL, &errCode, &errPosition));
		s1.resize(cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode, accessor.current()->first.length(), (UCHAR*) accessor.current()->first.c_str(),
			s1.getCapacity(), s1.begin(), &errCode, &errPosition));

		s2.resize(cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode, accessor.current()->second.length(), NULL, 0, NULL, &errCode, &errPosition));
		s2.resize(cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode, accessor.current()->second.length(), (UCHAR*) accessor.current()->second.c_str(),
			s2.getCapacity(), s2.begin(), &errCode, &errPosition));

		map16.put(string((char*) s1.begin(), s1.getCount()), string((char*) s2.begin(), s2.getCount()));

		found = accessor.getNext();
	}

	UnicodeUtil::Utf16Collation* collation =
		UnicodeUtil::Utf16Collation::create(tt, attributes, map16, configInfo);

	if (!collation)
		return false;

	tt->texttype_impl = new TextTypeImpl(cs, collation);

	return true;
}


ULONG IntlUtil::toLower(Jrd::CharSet* cs, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst,
	const ULONG* exceptions)
{
	const ULONG utf16_length = cs->getConvToUnicode().convertLength(srcLen);
	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> utf16_str;
	UCHAR* utf16_ptr;

	if (dstLen >= utf16_length)	// if dst buffer is sufficient large, use it as intermediate
		utf16_ptr = dst;
	else
		utf16_ptr = utf16_str.getBuffer(utf16_length);

	// convert to UTF-16
	srcLen = cs->getConvToUnicode().convert(srcLen, src, utf16_length, utf16_ptr);

	// convert to lowercase
	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> lower_str;
	srcLen = UnicodeUtil::utf16LowerCase(srcLen, Firebird::Aligner<USHORT>(utf16_ptr, srcLen),
		utf16_length, Firebird::OutAligner<USHORT>(lower_str.getBuffer(utf16_length), utf16_length),
		exceptions);

	// convert to original character set
	return cs->getConvFromUnicode().convert(srcLen, lower_str.begin(), dstLen, dst);
}


ULONG IntlUtil::toUpper(Jrd::CharSet* cs, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst,
	const ULONG* exceptions)
{
	const ULONG utf16_length = cs->getConvToUnicode().convertLength(srcLen);
	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> utf16_str;
	UCHAR* utf16_ptr;

	if (dstLen >= utf16_length)	// if dst buffer is sufficient large, use it as intermediate
		utf16_ptr = dst;
	else
		utf16_ptr = utf16_str.getBuffer(utf16_length);

	// convert to UTF-16
	srcLen = cs->getConvToUnicode().convert(srcLen, src, utf16_length, utf16_ptr);

	// convert to uppercase
	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> upper_str;
	srcLen = UnicodeUtil::utf16UpperCase(srcLen, Firebird::Aligner<USHORT>(utf16_ptr, srcLen),
		utf16_length, Firebird::OutAligner<USHORT>(upper_str.getBuffer(utf16_length), utf16_length),
		exceptions);

	// convert to original character set
	return cs->getConvFromUnicode().convert(srcLen, upper_str.begin(), dstLen, dst);
}


void IntlUtil::toUpper(Jrd::CharSet* cs, string& s)
{
	HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;
	size_t len = s.length();
	ULONG count = toUpper(cs, len, (const UCHAR*) s.c_str(), len * 4, buffer.getBuffer(len * 4), NULL);

	if (count != INTL_BAD_STR_LENGTH)
		s.assign((const char*) buffer.begin(), count);
	else
		fb_assert(false);
}


bool IntlUtil::readOneChar(Jrd::CharSet* cs, const UCHAR** s, const UCHAR* end, ULONG* size)
{
	(*s) += *size;

	if (*s >= end)
	{
		(*s) = end;
		*size = 0;
		return false;
	}

	UCHAR c[sizeof(ULONG)];
	*size = cs->substring(end - *s, *s, sizeof(c), c, 0, 1);

	return true;
}


// Transform ICU-VERSION attribute (given by the user) in COLL-VERSION (to be stored).
bool IntlUtil::setupIcuAttributes(charset* cs, const string& specificAttributes,
	const string& configInfo, string& newSpecificAttributes)
{
	AutoPtr<Jrd::CharSet> charSet(Jrd::CharSet::createInstance(*getDefaultMemoryPool(), 0, cs));

	IntlUtil::SpecificAttributesMap map;
	if (!IntlUtil::parseSpecificAttributes(charSet, specificAttributes.length(),
			(const UCHAR*) specificAttributes.begin(), &map))
	{
		return false;
	}

	string icuVersion;
	map.get("ICU-VERSION", icuVersion);

	string collVersion;
	if (!UnicodeUtil::getCollVersion(icuVersion, configInfo, collVersion))
		return false;

	map.remove("ICU-VERSION");
	map.remove("COLL-VERSION");

	if (collVersion.hasData())
		map.put("COLL-VERSION", collVersion);

	newSpecificAttributes = IntlUtil::generateSpecificAttributes(charSet, map);
	return true;
}


string IntlUtil::escapeAttribute(Jrd::CharSet* cs, const string& s)
{
	string ret;
	const UCHAR* p = (const UCHAR*)s.begin();
	const UCHAR* end = (const UCHAR*)s.end();
	ULONG size = 0;

	while (readOneChar(cs, &p, end, &size))
	{
		ULONG l;
		UCHAR* uc = (UCHAR*)(&l);

		const ULONG uSize = cs->getConvToUnicode().convert(size, p, sizeof(uc), uc);

		if (uSize == 2)
		{
			if (*(USHORT*) uc == '\\' || *(USHORT*) uc == '=' || *(USHORT*) uc == ';')
			{
				*(USHORT*) uc = '\\';
				UCHAR bytes[sizeof(ULONG)];

				ULONG bytesSize = cs->getConvFromUnicode().convert(
					sizeof(USHORT), uc, sizeof(bytes), bytes);

				ret.append(string((const char*)bytes, bytesSize));
			}
		}

		ret.append(string((const char*)p, size));
	}

	return ret;
}


string IntlUtil::unescapeAttribute(Jrd::CharSet* cs, const string& s)
{
	string ret;
	const UCHAR* p = (const UCHAR*)s.begin();
	const UCHAR* end = (const UCHAR*)s.end();
	ULONG size = 0;

	while (readAttributeChar(cs, &p, end, &size, false))
		ret.append(string((const char*)p, size));

	return ret;
}


bool IntlUtil::isAttributeEscape(Jrd::CharSet* cs, const UCHAR* s, ULONG size)
{
	UCHAR uc[sizeof(ULONG)];
	const ULONG uSize = cs->getConvToUnicode().convert(size, s, sizeof(uc), uc);

	return (uSize == 2 && *(USHORT*) uc == '\\');
}


bool IntlUtil::readAttributeChar(Jrd::CharSet* cs, const UCHAR** s, const UCHAR* end, ULONG* size, bool returnEscape)
{
	if (readOneChar(cs, s, end, size))
	{
		if (isAttributeEscape(cs, *s, *size))
		{
			const UCHAR* p = *s;
			ULONG firstSize = *size;

			if (readOneChar(cs, s, end, size))
			{
				if (returnEscape)
				{
					*s = p;
					*size += firstSize;
				}
			}
			else
				return false;
		}

		return true;
	}

	return false;
}


static void unicodeDestroy(texttype* tt)
{
	delete[] const_cast<ASCII*>(tt->texttype_name);
	delete static_cast<TextTypeImpl*>(tt->texttype_impl);
}


static USHORT unicodeKeyLength(texttype* tt, USHORT len)
{
	TextTypeImpl* impl = static_cast<TextTypeImpl*>(tt->texttype_impl);
	return impl->collation->keyLength(len / impl->cs->charset_max_bytes_per_char * 4);
}


static USHORT unicodeStrToKey(texttype* tt, USHORT srcLen, const UCHAR* src,
	USHORT dstLen, UCHAR* dst, USHORT keyType)
{
	TextTypeImpl* impl = static_cast<TextTypeImpl*>(tt->texttype_impl);

	try
	{
		charset* cs = impl->cs;

		HalfStaticArray<UCHAR, BUFFER_SMALL> utf16Str;
		USHORT errorCode;
		ULONG offendingPos;

		utf16Str.getBuffer(
			cs->charset_to_unicode.csconvert_fn_convert(
				&cs->charset_to_unicode,
				srcLen,
				src,
				0,
				NULL,
				&errorCode,
				&offendingPos));

		ULONG utf16Len = cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode,
			srcLen,
			src,
			utf16Str.getCapacity(),
			utf16Str.begin(),
			&errorCode,
			&offendingPos);

		return impl->collation->stringToKey(utf16Len, (USHORT*)utf16Str.begin(), dstLen, dst, keyType);
	}
	catch (BadAlloc)
	{
		fb_assert(false);
		return INTL_BAD_KEY_LENGTH;
	}
}


static SSHORT unicodeCompare(texttype* tt, ULONG len1, const UCHAR* str1,
	ULONG len2, const UCHAR* str2, INTL_BOOL* errorFlag)
{
	TextTypeImpl* impl = static_cast<TextTypeImpl*>(tt->texttype_impl);

	try
	{
		*errorFlag = false;

		charset* cs = impl->cs;

		HalfStaticArray<UCHAR, BUFFER_SMALL> utf16Str1;
		HalfStaticArray<UCHAR, BUFFER_SMALL> utf16Str2;
		USHORT errorCode;
		ULONG offendingPos;

		utf16Str1.getBuffer(
			cs->charset_to_unicode.csconvert_fn_convert(
				&cs->charset_to_unicode,
				len1,
				str1,
				0,
				NULL,
				&errorCode,
				&offendingPos));

		const ULONG utf16Len1 = cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode,
			len1,
			str1,
			utf16Str1.getCapacity(),
			utf16Str1.begin(),
			&errorCode,
			&offendingPos);

		utf16Str2.getBuffer(
			cs->charset_to_unicode.csconvert_fn_convert(
				&cs->charset_to_unicode,
				len2,
				str2,
				0,
				NULL,
				&errorCode,
				&offendingPos));

		const ULONG utf16Len2 = cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode,
			len2,
			str2,
			utf16Str2.getCapacity(),
			utf16Str2.begin(),
			&errorCode,
			&offendingPos);

		return impl->collation->compare(utf16Len1, (USHORT*)utf16Str1.begin(),
			utf16Len2, (USHORT*)utf16Str2.begin(), errorFlag);
	}
	catch (BadAlloc)
	{
		fb_assert(false);
		return 0;
	}
}


static ULONG unicodeCanonical(texttype* tt, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst)
{
	TextTypeImpl* impl = static_cast<TextTypeImpl*>(tt->texttype_impl);

	try
	{
		charset* cs = impl->cs;

		HalfStaticArray<UCHAR, BUFFER_SMALL> utf16Str;
		USHORT errorCode;
		ULONG offendingPos;

		utf16Str.getBuffer(
			cs->charset_to_unicode.csconvert_fn_convert(
				&cs->charset_to_unicode,
				srcLen,
				src,
				0,
				NULL,
				&errorCode,
				&offendingPos));

		const ULONG utf16Len = cs->charset_to_unicode.csconvert_fn_convert(
			&cs->charset_to_unicode,
			srcLen,
			src,
			utf16Str.getCapacity(),
			utf16Str.begin(),
			&errorCode,
			&offendingPos);

		return impl->collation->canonical(
			utf16Len, Firebird::Aligner<USHORT>(utf16Str.begin(), utf16Len),
			dstLen, Firebird::OutAligner<ULONG>(dst, dstLen), NULL);
	}
	catch (BadAlloc)
	{
		fb_assert(false);
		return INTL_BAD_KEY_LENGTH;
	}
}


}	// namespace Firebird
