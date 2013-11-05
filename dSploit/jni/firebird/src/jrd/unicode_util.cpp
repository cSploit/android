/*
 *	PROGRAM:	JRD International support
 *	MODULE:		unicode_util.h
 *	DESCRIPTION:	Unicode functions
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
 *  Copyright (c) 2004 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/classes/alloc.h"
#include "../jrd/constants.h"
#include "../jrd/unicode_util.h"
#include "../jrd/CharSet.h"
#include "../jrd/IntlUtil.h"
#include "../jrd/gdsassert.h"
#include "../common/classes/auto.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/init.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/rwlock.h"
#include "unicode/ustring.h"
#include "unicode/utrans.h"
#include "unicode/uchar.h"
#include "unicode/ucnv.h"
#include "unicode/ucol.h"


using namespace Firebird;


namespace Jrd {


const char* const UnicodeUtil::DEFAULT_ICU_VERSION =
	STRINGIZE(U_ICU_VERSION_MAJOR_NUM)"."STRINGIZE(U_ICU_VERSION_MINOR_NUM);


// encapsulate ICU collations libraries
struct UnicodeUtil::ICU
{
private:
	ICU(const ICU&);				// not implemented
	ICU& operator =(const ICU&);	// not implemented

public:
	ICU(int aMajorVersion, int aMinorVersion)
		: majorVersion(aMajorVersion),
		  minorVersion(aMinorVersion),
		  inModule(NULL),
		  ucModule(NULL)
	{
	}

	~ICU()
	{
		delete ucModule;
		delete inModule;
	}

	template <typename T> void getEntryPoint(const char* name, ModuleLoader::Module* module, T& ptr)
	{
		string symbol;

		symbol.printf("%s_%d_%d", name, majorVersion, minorVersion);
		module->findSymbol(symbol, ptr);
		if (ptr)
			return;

		symbol.printf("%s_%d%d", name, majorVersion, minorVersion);
		module->findSymbol(symbol, ptr);
	}

	int majorVersion;
	int minorVersion;
	ModuleLoader::Module* inModule;
	ModuleLoader::Module* ucModule;
	UVersionInfo collVersion;

	void (U_EXPORT2 *uInit)(UErrorCode* status);
	void (U_EXPORT2 *uVersionToString)(UVersionInfo versionArray, char* versionString);

	int32_t (U_EXPORT2 *ulocCountAvailable)();
	const char* (U_EXPORT2 *ulocGetAvailable)(int32_t n);

	void (U_EXPORT2 *usetClose)(USet* set);
	int32_t (U_EXPORT2 *usetGetItem)(const USet* set, int32_t itemIndex,
		UChar32* start, UChar32* end, UChar* str, int32_t strCapacity, UErrorCode* ec);
	int32_t (U_EXPORT2 *usetGetItemCount)(const USet* set);
	USet* (U_EXPORT2 *usetOpen)(UChar32 start, UChar32 end);

	void (U_EXPORT2 *ucolClose)(UCollator* coll);
	int32_t (U_EXPORT2 *ucolGetContractions)(const UCollator* coll, USet* conts, UErrorCode* status);
	int32_t (U_EXPORT2 *ucolGetSortKey)(const UCollator* coll, const UChar* source,
		int32_t sourceLength, uint8_t* result, int32_t resultLength);
	UCollator* (U_EXPORT2 *ucolOpen)(const char* loc, UErrorCode* status);
	void (U_EXPORT2 *ucolSetAttribute)(UCollator* coll, UColAttribute attr,
		UColAttributeValue value, UErrorCode* status);
	UCollationResult (U_EXPORT2 *ucolStrColl)(const UCollator* coll, const UChar* source,
		int32_t sourceLength, const UChar* target, int32_t targetLength);
	void (U_EXPORT2 *ucolGetVersion)(const UCollator* coll, UVersionInfo info);

	void (U_EXPORT2 *utransClose)(UTransliterator* trans);
	UTransliterator* (U_EXPORT2 *utransOpen)(
		const char* id,
		UTransDirection dir,
		const UChar* rules,         /* may be Null */
		int32_t rulesLength,        /* -1 if null-terminated */
		UParseError* parseError,    /* may be Null */
		UErrorCode* status);
	void (U_EXPORT2 *utransTransUChars)(
		const UTransliterator* trans,
		UChar* text,
		int32_t* textLength,
		int32_t textCapacity,
		int32_t start,
		int32_t* limit,
		UErrorCode* status);
};


// cache ICU module instances to not load and unload many times
class UnicodeUtil::ICUModules
{
	typedef GenericMap<Pair<Left<string, ICU*> > > ModulesMap;

public:
	explicit ICUModules(MemoryPool&)
	{
	}

	~ICUModules()
	{
		ModulesMap::Accessor modulesAccessor(&modules());
		for (bool found = modulesAccessor.getFirst(); found; found = modulesAccessor.getNext())
			delete modulesAccessor.current()->second;
	}

	InitInstance<ModulesMap> modules;
	RWLock lock;
};

namespace {
	GlobalPtr<UnicodeUtil::ICUModules> icuModules;
}


static const char* const COLL_30_VERSION = "41.128.4.4";	// ICU 3.0 collator version


static void getVersions(const string& configInfo, ObjectsArray<string>& versions)
{
	charset cs;
	IntlUtil::initAsciiCharset(&cs);

	AutoPtr<CharSet> ascii(Jrd::CharSet::createInstance(*getDefaultMemoryPool(), 0, &cs));

	IntlUtil::SpecificAttributesMap config;
	IntlUtil::parseSpecificAttributes(ascii, configInfo.length(),
		(const UCHAR*) configInfo.c_str(), &config);

	string versionsStr;
	if (config.get("icu_versions", versionsStr))
		versionsStr.trim();
	else
		versionsStr = "default";

	versions.clear();

	size_t start = 0;
	size_t n;

	for (size_t i = versionsStr.find(' '); i != versionsStr.npos;
		start = i + 1, i = versionsStr.find(' ', start))
	{
		if ((n = versionsStr.find_first_not_of(' ', start)) != versionsStr.npos)
			start = n;
		versions.add(versionsStr.substr(start, i - start));
	}

	if ((n = versionsStr.find_first_not_of(' ', start)) != versionsStr.npos)
		start = n;
	versions.add(versionsStr.substr(start));
}


// BOCU-1
USHORT UnicodeUtil::utf16KeyLength(USHORT len)
{
	return (len / 2) * 4;
}


// BOCU-1
USHORT UnicodeUtil::utf16ToKey(USHORT srcLen, const USHORT* src, USHORT dstLen, UCHAR* dst)
{
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	if (dstLen < srcLen / sizeof(*src) * 4)
		return INTL_BAD_KEY_LENGTH;

	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("BOCU-1", &status);
	fb_assert(U_SUCCESS(status));

	const int32_t len = ucnv_fromUChars(conv, reinterpret_cast<char*>(dst), dstLen,
		// safe cast - alignment not changed
		reinterpret_cast<const UChar*>(src), srcLen / sizeof(*src), &status);
	fb_assert(U_SUCCESS(status));

	ucnv_close(conv);

	return len;
}


ULONG UnicodeUtil::utf16LowerCase(ULONG srcLen, const USHORT* src, ULONG dstLen, USHORT* dst,
	const ULONG* exceptions)
{
	// this is more correct but we don't support completely yet
	/***
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	memcpy(dst, src, srcLen);

	UErrorCode errorCode = U_ZERO_ERROR;
	UTransliterator* trans = utrans_open("Any-Lower", UTRANS_FORWARD, NULL, 0, NULL, &errorCode);
	//// TODO: add exceptions in this way: Any-Lower[^\\u03BC] - for U+03BC

	if (errorCode <= 0)
	{
		int32_t capacity = dstLen;
		int32_t len = srcLen / sizeof(USHORT);
		int32_t limit = len;

		utrans_transUChars(trans, reinterpret_cast<UChar*>(dst), &len, capacity, 0, &limit, &errorCode);
		utrans_close(trans);

		len *= sizeof(USHORT);
		if (len > dstLen)
			len = INTL_BAD_STR_LENGTH;

		return len;
	}
	else
		return INTL_BAD_STR_LENGTH;
	***/

	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	srcLen /= sizeof(*src);
	dstLen /= sizeof(*dst);

	ULONG n = 0;

	for (ULONG i = 0; i < srcLen;)
	{
		uint32_t c;
		U16_NEXT(src, i, srcLen, c);

		if (!exceptions)
			c = u_tolower(c);
		else
		{
			const ULONG* p = exceptions;
			while (*p && *p != c)
				++p;

			if (*p == 0)
				c = u_tolower(c);
		}

		bool error;
		U16_APPEND(dst, n, dstLen, c, error);
	}

	return n * sizeof(*dst);
}


ULONG UnicodeUtil::utf16UpperCase(ULONG srcLen, const USHORT* src, ULONG dstLen, USHORT* dst,
	const ULONG* exceptions)
{
	// this is more correct but we don't support completely yet
	/***
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	memcpy(dst, src, srcLen);

	UErrorCode errorCode = U_ZERO_ERROR;
	UTransliterator* trans = utrans_open("Any-Upper", UTRANS_FORWARD, NULL, 0, NULL, &errorCode);
	//// TODO: add exceptions in this way: Any-Upper[^\\u03BC] - for U+03BC

	if (errorCode <= 0)
	{
		int32_t capacity = dstLen;
		int32_t len = srcLen / sizeof(USHORT);
		int32_t limit = len;

		utrans_transUChars(trans, reinterpret_cast<UChar*>(dst), &len, capacity, 0, &limit, &errorCode);
		utrans_close(trans);

		len *= sizeof(USHORT);
		if (len > dstLen)
			len = INTL_BAD_STR_LENGTH;

		return len;
	}
	else
		return INTL_BAD_STR_LENGTH;
	***/

	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	srcLen /= sizeof(*src);
	dstLen /= sizeof(*dst);

	ULONG n = 0;

	for (ULONG i = 0; i < srcLen;)
	{
		uint32_t c;
		U16_NEXT(src, i, srcLen, c);

		if (!exceptions)
			c = u_toupper(c);
		else
		{
			const ULONG* p = exceptions;
			while (*p && *p != c)
				++p;

			if (*p == 0)
				c = u_toupper(c);
		}

		bool error;
		U16_APPEND(dst, n, dstLen, c, error);
	}

	return n * sizeof(*dst);
}


ULONG UnicodeUtil::utf16ToUtf8(ULONG srcLen, const USHORT* src, ULONG dstLen, UCHAR* dst,
							   USHORT* err_code, ULONG* err_position)
{
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL || dst == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);

	*err_code = 0;

	if (dst == NULL)
		return srcLen / sizeof(*src) * 4;

	srcLen /= sizeof(*src);

	const UCHAR* const dstStart = dst;
	const UCHAR* const dstEnd = dst + dstLen;

	for (ULONG i = 0; i < srcLen; )
	{
		if (dstEnd - dst == 0)
		{
			*err_code = CS_TRUNCATION_ERROR;
			*err_position = i * sizeof(*src);
			break;
		}

		UChar32 c = src[i++];

		if (c <= 0x7F)
			*dst++ = c;
		else
		{
			*err_position = (i - 1) * sizeof(*src);

			if (UTF_IS_SURROGATE(c))
			{
				UChar32 c2;

				if (UTF_IS_SURROGATE_FIRST(c) && i < srcLen && UTF_IS_TRAIL(c2 = src[i]))
				{
					++i;
					c = UTF16_GET_PAIR_VALUE(c, c2);
				}
				else
				{
					*err_code = CS_BAD_INPUT;
					break;
				}
			}

			if (U8_LENGTH(c) <= dstEnd - dst)
			{
				int j = 0;
				U8_APPEND_UNSAFE(dst, j, c);
				dst += j;
			}
			else
			{
				*err_code = CS_TRUNCATION_ERROR;
				break;
			}
		}
	}

	return (dst - dstStart) * sizeof(*dst);
}


ULONG UnicodeUtil::utf8ToUtf16(ULONG srcLen, const UCHAR* src, ULONG dstLen, USHORT* dst,
							   USHORT* err_code, ULONG* err_position)
{
	fb_assert(src != NULL || dst == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);

	*err_code = 0;

	if (dst == NULL)
		return srcLen * sizeof(*dst);

	const USHORT* const dstStart = dst;
	const USHORT* const dstEnd = dst + dstLen / sizeof(*dst);

	for (ULONG i = 0; i < srcLen; )
	{
		if (dstEnd - dst == 0)
		{
			*err_code = CS_TRUNCATION_ERROR;
			*err_position = i;
			break;
		}

		UChar32 c = src[i++];

		if (c <= 0x7F)
			*dst++ = c;
		else
		{
			*err_position = i - 1;

			c = utf8_nextCharSafeBody(src, reinterpret_cast<int32_t*>(&i), srcLen, c, -1);

			if (c < 0)
			{
				*err_code = CS_BAD_INPUT;
				break;
			}
			else if (c <= 0xFFFF)
				*dst++ = c;
			else
			{
				if (dstEnd - dst > 1)
				{
					*dst++ = UTF16_LEAD(c);
					*dst++ = UTF16_TRAIL(c);
				}
				else
				{
					*err_code = CS_TRUNCATION_ERROR;
					break;
				}
			}
		}
	}

	return (dst - dstStart) * sizeof(*dst);
}


ULONG UnicodeUtil::utf16ToUtf32(ULONG srcLen, const USHORT* src, ULONG dstLen, ULONG* dst,
								USHORT* err_code, ULONG* err_position)
{
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL || dst == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);

	*err_code = 0;

	if (dst == NULL)
		return srcLen / sizeof(*src) * sizeof(*dst);

	// based on u_strToUTF32 from ICU
	const USHORT* const srcStart = src;
	const ULONG* const dstStart = dst;
	const USHORT* const srcEnd = src + srcLen / sizeof(*src);
	const ULONG* const dstEnd = dst + dstLen / sizeof(*dst);

	while (src < srcEnd && dst < dstEnd)
	{
		ULONG ch = *src++;

		if (UTF_IS_LEAD(ch))
		{
			ULONG ch2;
			if (src < srcEnd && UTF_IS_TRAIL(ch2 = *src))
			{
				ch = UTF16_GET_PAIR_VALUE(ch, ch2);
				++src;
			}
			else
			{
				*err_code = CS_BAD_INPUT;
				--src;
				break;
			}
		}

		*(dst++) = ch;
	}

	*err_position = (src - srcStart) * sizeof(*src);

	if (*err_code == 0 && src < srcEnd)
		*err_code = CS_TRUNCATION_ERROR;

	return (dst - dstStart) * sizeof(*dst);
}


ULONG UnicodeUtil::utf32ToUtf16(ULONG srcLen, const ULONG* src, ULONG dstLen, USHORT* dst,
								USHORT* err_code, ULONG* err_position)
{
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL || dst == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);

	*err_code = 0;

	if (dst == NULL)
		return srcLen;

	// based on u_strFromUTF32 from ICU
	const ULONG* const srcStart = src;
	const USHORT* const dstStart = dst;
	const ULONG* const srcEnd = src + srcLen / sizeof(*src);
	const USHORT* const dstEnd = dst + dstLen / sizeof(*dst);

	while (src < srcEnd && dst < dstEnd)
	{
		const ULONG ch = *src++;

		if (ch <= 0xFFFF)
			*(dst++) = ch;
		else if (ch <= 0x10FFFF)
		{
			*(dst++) = UTF16_LEAD(ch);

			if (dst < dstEnd)
				*(dst++) = UTF16_TRAIL(ch);
			else
			{
				*err_code = CS_TRUNCATION_ERROR;
				--dst;
				break;
			}
		}
		else
		{
			*err_code = CS_BAD_INPUT;
			--src;
			break;
		}
	}

	*err_position = (src - srcStart) * sizeof(*src);

	if (*err_code == 0 && src < srcEnd)
		*err_code = CS_TRUNCATION_ERROR;

	return (dst - dstStart) * sizeof(*dst);
}


SSHORT UnicodeUtil::utf16Compare(ULONG len1, const USHORT* str1, ULONG len2, const USHORT* str2,
								 INTL_BOOL* error_flag)
{
	fb_assert(len1 % sizeof(*str1) == 0);
	fb_assert(len2 % sizeof(*str2) == 0);
	fb_assert(str1 != NULL);
	fb_assert(str2 != NULL);
	fb_assert(error_flag != NULL);

	*error_flag = false;

	// safe casts - alignment not changed
	int32_t cmp = u_strCompare(reinterpret_cast<const UChar*>(str1), len1 / sizeof(*str1),
		reinterpret_cast<const UChar*>(str2), len2 / sizeof(*str2), true);

	return (cmp < 0 ? -1 : (cmp > 0 ? 1 : 0));
}


ULONG UnicodeUtil::utf16Length(ULONG len, const USHORT* str)
{
	fb_assert(len % sizeof(*str) == 0);
	// safe cast - alignment not changed
	return u_countChar32(reinterpret_cast<const UChar*>(str), len / sizeof(*str));
}


ULONG UnicodeUtil::utf16Substring(ULONG srcLen, const USHORT* src, ULONG dstLen, USHORT* dst,
								  ULONG startPos, ULONG length)
{
	fb_assert(srcLen % sizeof(*src) == 0);
	fb_assert(src != NULL && dst != NULL);

	if (length == 0)
		return 0;

	const USHORT* const dstStart = dst;
	const USHORT* const srcEnd = src + srcLen / sizeof(*src);
	const USHORT* const dstEnd = dst + dstLen / sizeof(*dst);
	ULONG pos = 0;

	while (src < srcEnd && dst < dstEnd && pos < startPos)
	{
		const ULONG ch = *src++;

		if (UTF_IS_LEAD(ch))
		{
			if (src < srcEnd && UTF_IS_TRAIL(*src))
				++src;
		}

		++pos;
	}

	while (src < srcEnd && dst < dstEnd && pos < startPos + length)
	{
		const ULONG ch = *src++;

		*(dst++) = ch;

		if (UTF_IS_LEAD(ch))
		{
			ULONG ch2;
			if (src < srcEnd && UTF_IS_TRAIL(ch2 = *src))
			{
				*(dst++) = ch2;
				++src;
			}
		}

		++pos;
	}

	return (dst - dstStart) * sizeof(*dst);
}


INTL_BOOL UnicodeUtil::utf8WellFormed(ULONG len, const UCHAR* str, ULONG* offending_position)
{
	fb_assert(str != NULL);

	for (ULONG i = 0; i < len; )
	{
		UChar32 c = str[i++];

		if (c > 0x7F)
		{
			const ULONG save_i = i - 1;

			c = utf8_nextCharSafeBody(str, reinterpret_cast<int32_t*>(&i), len, c, -1);

			if (c < 0)
			{
				if (offending_position)
					*offending_position = save_i;
				return false;	// malformed
			}
		}
	}

	return true;	// well-formed
}


INTL_BOOL UnicodeUtil::utf16WellFormed(ULONG len, const USHORT* str, ULONG* offending_position)
{
	fb_assert(str != NULL);
	fb_assert(len % sizeof(*str) == 0);

	len /= sizeof(*str);

	for (ULONG i = 0; i < len;)
	{
		const ULONG save_i = i;

		uint32_t c;
		U16_NEXT(str, i, len, c);

		if (!U_IS_SUPPLEMENTARY(c) && (U16_IS_LEAD(c) || U16_IS_TRAIL(c)))
		{
			if (offending_position)
				*offending_position = save_i * sizeof(*str);
			return false;	// malformed
		}
	}

	return true;	// well-formed
}


INTL_BOOL UnicodeUtil::utf32WellFormed(ULONG len, const ULONG* str, ULONG* offending_position)
{
	fb_assert(str != NULL);
	fb_assert(len % sizeof(*str) == 0);

	const ULONG* strStart = str;

	while (len)
	{
		if (!U_IS_UNICODE_CHAR(*str))
		{
			if (offending_position)
				*offending_position = (str - strStart) * sizeof(*str);
			return false;	// malformed
		}

		++str;
		len -= sizeof(*str);
	}

	return true;	// well-formed
}


UnicodeUtil::ICU* UnicodeUtil::loadICU(const Firebird::string& icuVersion,
	const Firebird::string& configInfo)
{
#if defined(WIN_NT)
	const char* const inTemplate = "icuin%d%d.dll";
	const char* const ucTemplate = "icuuc%d%d.dll";
#elif defined(DARWIN)
	const char* const inTemplate = "/Library/Frameworks/Firebird.framework/Versions/A/Libraries/libicui18n.dylib";
	const char* const ucTemplate = "/Library/Frameworks/Firebird.framework/versions/A/Libraries/libicuuc.dylib";
#elif defined(HPUX)
	const char* const inTemplate = "libicui18n.sl.%d%d";
	const char* const ucTemplate = "libicuuc.sl.%d%d";
#else
	const char* const inTemplate = "libicui18n.so.%d%d";
	const char* const ucTemplate = "libicuuc.so.%d%d";
#endif

	ObjectsArray<string> versions;
	getVersions(configInfo, versions);

	string version = icuVersion.isEmpty() ? versions[0] : icuVersion;
	if (version == "default")
	{
		version.printf("%d.%d", U_ICU_VERSION_MAJOR_NUM, U_ICU_VERSION_MINOR_NUM);
	}

	for (ObjectsArray<string>::const_iterator i(versions.begin()); i != versions.end(); ++i)
	{
		int majorVersion, minorVersion;

		if (*i == "default")
		{
			majorVersion = U_ICU_VERSION_MAJOR_NUM;
			minorVersion = U_ICU_VERSION_MINOR_NUM;
		}
		else if (sscanf(i->c_str(), "%d.%d", &majorVersion, &minorVersion) != 2)
			continue;

		string configVersion;
		configVersion.printf("%d.%d", majorVersion, minorVersion);

		if (version != configVersion)
			continue;

		ReadLockGuard readGuard(icuModules->lock);

		ICU* icu;
		if (icuModules->modules().get(version, icu))
			return icu;

		PathName filename;
		filename.printf(ucTemplate, majorVersion, minorVersion);

		icu = FB_NEW(*getDefaultMemoryPool()) ICU(majorVersion, minorVersion);

		icu->ucModule = ModuleLoader::loadModule(filename);
		if (!icu->ucModule)
		{
			ModuleLoader::doctorModuleExtention(filename);
			icu->ucModule = ModuleLoader::loadModule(filename);
		}

		if (!icu->ucModule)
		{
			delete icu;
			continue;
		}

		filename.printf(inTemplate, majorVersion, minorVersion);

		icu->inModule = ModuleLoader::loadModule(filename);
		if (!icu->inModule)
		{
			ModuleLoader::doctorModuleExtention(filename);
			icu->inModule = ModuleLoader::loadModule(filename);
		}

		if (!icu->inModule)
		{
			delete icu;
			continue;
		}

		icu->getEntryPoint("u_init", icu->ucModule, icu->uInit);
		icu->getEntryPoint("u_versionToString", icu->ucModule, icu->uVersionToString);
		icu->getEntryPoint("uloc_countAvailable", icu->ucModule, icu->ulocCountAvailable);
		icu->getEntryPoint("uloc_getAvailable", icu->ucModule, icu->ulocGetAvailable);
		icu->getEntryPoint("uset_close", icu->ucModule, icu->usetClose);
		icu->getEntryPoint("uset_getItem", icu->ucModule, icu->usetGetItem);
		icu->getEntryPoint("uset_getItemCount", icu->ucModule, icu->usetGetItemCount);
		icu->getEntryPoint("uset_open", icu->ucModule, icu->usetOpen);

		icu->getEntryPoint("ucol_close", icu->inModule, icu->ucolClose);
		icu->getEntryPoint("ucol_getContractions", icu->inModule, icu->ucolGetContractions);
		icu->getEntryPoint("ucol_getSortKey", icu->inModule, icu->ucolGetSortKey);
		icu->getEntryPoint("ucol_open", icu->inModule, icu->ucolOpen);
		icu->getEntryPoint("ucol_setAttribute", icu->inModule, icu->ucolSetAttribute);
		icu->getEntryPoint("ucol_strcoll", icu->inModule, icu->ucolStrColl);
		icu->getEntryPoint("ucol_getVersion", icu->inModule, icu->ucolGetVersion);
		icu->getEntryPoint("utrans_open", icu->inModule, icu->utransOpen);
		icu->getEntryPoint("utrans_close", icu->inModule, icu->utransClose);
		icu->getEntryPoint("utrans_transUChars", icu->inModule, icu->utransTransUChars);

		if (/*!icu->uInit ||*/ !icu->uVersionToString || !icu->ulocCountAvailable ||
			!icu->ulocGetAvailable || !icu->usetClose || !icu->usetGetItem ||
			!icu->usetGetItemCount || !icu->usetOpen || !icu->ucolClose ||
			!icu->ucolGetContractions || !icu->ucolGetSortKey || !icu->ucolOpen ||
			!icu->ucolSetAttribute || !icu->ucolStrColl || !icu->ucolGetVersion ||
			!icu->utransOpen || !icu->utransClose || !icu->utransTransUChars)
		{
			delete icu;
			continue;
		}

		UErrorCode status = U_ZERO_ERROR;

		if (icu->uInit)
		{
			icu->uInit(&status);
			if (status != U_ZERO_ERROR)
			{
				delete icu;
				continue;
			}
		}

		UCollator* collator = icu->ucolOpen("", &status);
		if (!collator)
		{
			delete icu;
			continue;
		}

		icu->ucolGetVersion(collator, icu->collVersion);
		icu->ucolClose(collator);

		// RWLock don't allow lock upgrade (read->write) so we
		// release read and acquire a write lock.
		readGuard.release();
		WriteLockGuard writeGuard(icuModules->lock);

		// In this small amount of time, one may already loaded the
		// same version, so within the write lock we verify again.
		ICU* icu2;
		if (icuModules->modules().get(version, icu2))
		{
			delete icu;
			return icu2;
		}

		icuModules->modules().put(version, icu);
		return icu;
	}

	return NULL;
}


bool UnicodeUtil::getCollVersion(const Firebird::string& icuVersion,
	const Firebird::string& configInfo, Firebird::string& collVersion)
{
	ICU* icu = loadICU(icuVersion, configInfo);

	if (!icu)
		return false;

	char version[U_MAX_VERSION_STRING_LENGTH];
	icu->uVersionToString(icu->collVersion, version);

	if (string(COLL_30_VERSION) == version)
		collVersion = "";
	else
		collVersion = version;

	return true;
}

UnicodeUtil::Utf16Collation* UnicodeUtil::Utf16Collation::create(
	texttype* tt, USHORT attributes,
	Firebird::IntlUtil::SpecificAttributesMap& specificAttributes, const Firebird::string& configInfo)
{
	int attributeCount = 0;
	bool error;

	string locale;
	if (specificAttributes.get(IntlUtil::convertAsciiToUtf16("LOCALE"), locale))
		++attributeCount;

	string collVersion;
	if (specificAttributes.get(IntlUtil::convertAsciiToUtf16("COLL-VERSION"), collVersion))
	{
		++attributeCount;

		collVersion = IntlUtil::convertUtf16ToAscii(collVersion, &error);
		if (error)
			return NULL;
	}

	string numericSort;
	if (specificAttributes.get(IntlUtil::convertAsciiToUtf16("NUMERIC-SORT"), numericSort))
	{
		++attributeCount;

		numericSort = IntlUtil::convertUtf16ToAscii(numericSort, &error);
		if (error || !(numericSort == "0" || numericSort == "1"))
			return NULL;
	}

	locale = IntlUtil::convertUtf16ToAscii(locale, &error);
	if (error)
		return NULL;

	if ((attributes & ~(TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE |
			TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ||
		((attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ==
			TEXTTYPE_ATTR_ACCENT_INSENSITIVE) ||
		(specificAttributes.count() - attributeCount) != 0)
	{
		return NULL;
	}

	if (collVersion.isEmpty())
		collVersion = COLL_30_VERSION;

	tt->texttype_pad_option = (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;

	ICU* icu = loadICU(collVersion, locale, configInfo);
	if (!icu)
		return NULL;

	UErrorCode status = U_ZERO_ERROR;

	UCollator* compareCollator = icu->ucolOpen(locale.c_str(), &status);
	if (!compareCollator)
		return NULL;

	UCollator* partialCollator = icu->ucolOpen(locale.c_str(), &status);
	if (!partialCollator)
	{
		icu->ucolClose(compareCollator);
		return NULL;
	}

	UCollator* sortCollator = icu->ucolOpen(locale.c_str(), &status);
	if (!sortCollator)
	{
		icu->ucolClose(compareCollator);
		icu->ucolClose(partialCollator);
		return NULL;
	}

	icu->ucolSetAttribute(partialCollator, UCOL_STRENGTH, UCOL_PRIMARY, &status);

	if ((attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ==
		(TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE))
	{
		icu->ucolSetAttribute(compareCollator, UCOL_STRENGTH, UCOL_PRIMARY, &status);
		tt->texttype_flags |= TEXTTYPE_SEPARATE_UNIQUE;
		tt->texttype_canonical_width = 4;	// UTF-32
	}
	else if (attributes & TEXTTYPE_ATTR_CASE_INSENSITIVE)
	{
		icu->ucolSetAttribute(compareCollator, UCOL_STRENGTH, UCOL_SECONDARY, &status);
		tt->texttype_flags |= TEXTTYPE_SEPARATE_UNIQUE;
		tt->texttype_canonical_width = 4;	// UTF-32
	}
	else
		tt->texttype_flags = TEXTTYPE_DIRECT_MATCH;

	const bool isNumericSort = numericSort == "1";
	if (isNumericSort)
	{
		icu->ucolSetAttribute(compareCollator, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);
		icu->ucolSetAttribute(partialCollator, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);
		icu->ucolSetAttribute(sortCollator, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);
	}

	USet* contractions = icu->usetOpen(0, 0);
	// status not verified here.
	icu->ucolGetContractions(partialCollator, contractions, &status);

	Utf16Collation* obj = new Utf16Collation();
	obj->icu = icu;
	obj->tt = tt;
	obj->attributes = attributes;
	obj->compareCollator = compareCollator;
	obj->partialCollator = partialCollator;
	obj->sortCollator = sortCollator;
	obj->contractions = contractions;
	obj->contractionsCount = icu->usetGetItemCount(contractions);
	obj->numericSort = isNumericSort;

	return obj;
}


UnicodeUtil::Utf16Collation::~Utf16Collation()
{
	icu->usetClose(contractions);

	icu->ucolClose(compareCollator);
	icu->ucolClose(partialCollator);
	icu->ucolClose(sortCollator);

	// ASF: we should not "delete icu"
}


USHORT UnicodeUtil::Utf16Collation::keyLength(USHORT len) const
{
	return (len / 4) * 6;
}


USHORT UnicodeUtil::Utf16Collation::stringToKey(USHORT srcLen, const USHORT* src,
												USHORT dstLen, UCHAR* dst,
												USHORT key_type) const
{
	fb_assert(src != NULL && dst != NULL);
	fb_assert(srcLen % sizeof(*src) == 0);

	if (dstLen < keyLength(srcLen))
	{
		fb_assert(false);
		return INTL_BAD_KEY_LENGTH;
	}

	srcLen /= sizeof(*src);

	if (tt->texttype_pad_option)
	{
		const USHORT* pad;

		for (pad = src + srcLen - 1; pad >= src; --pad)
		{
			if (*pad != 32)
				break;
		}

		srcLen = pad - src + 1;
	}

	const UCollator* coll = NULL;

	switch (key_type)
	{
		case INTL_KEY_PARTIAL:
		{
			coll = partialCollator;

			// Remove last bytes of key if they are start of a contraction
			// to correctly find in the index.
			for (int i = 0; i < contractionsCount; ++i)
			{
				UChar str[10];
				UErrorCode status = U_ZERO_ERROR;
				int len = icu->usetGetItem(contractions, i, NULL, NULL, str, sizeof(str), &status);

				if (len > srcLen)
					len = srcLen;
				else
					--len;

				// safe cast - alignment not changed
				if (u_strCompare(str, len, reinterpret_cast<const UChar*>(src) + srcLen - len, len, true) == 0)
				{
					srcLen -= len;
					break;
				}
			}

			if (numericSort)
			{
				// ASF: Wee need to remove trailing numbers to return sub key that
				// matches full key. Example: "abc1" becomes "abc" to match "abc10".
				const USHORT* p = src + srcLen - 1;

				for (; p >= src; --p)
				{
					if (!(*p >= '0' && *p <= '9'))
						break;
				}

				srcLen = p - src + 1;
			}

			break;
		}

		case INTL_KEY_UNIQUE:
			coll = compareCollator;
			break;

		case INTL_KEY_SORT:
			coll = sortCollator;
			break;

		default:
			fb_assert(false);
			return INTL_BAD_KEY_LENGTH;
	}

	if (srcLen == 0)
		return 0;

	return icu->ucolGetSortKey(coll,
		reinterpret_cast<const UChar*>(src), srcLen, dst, dstLen);
}


SSHORT UnicodeUtil::Utf16Collation::compare(ULONG len1, const USHORT* str1,
											ULONG len2, const USHORT* str2,
											INTL_BOOL* error_flag) const
{
	fb_assert(len1 % sizeof(*str1) == 0 && len2 % sizeof(*str2) == 0);
	fb_assert(str1 != NULL && str2 != NULL);
	fb_assert(error_flag != NULL);

	*error_flag = false;

	len1 /= sizeof(*str1);
	len2 /= sizeof(*str2);

	if (tt->texttype_pad_option)
	{
		const USHORT* pad;

		for (pad = str1 + len1 - 1; pad >= str1; --pad)
		{
			if (*pad != 32)
				break;
		}

		len1 = pad - str1 + 1;

		for (pad = str2 + len2 - 1; pad >= str2; --pad)
		{
			if (*pad != 32)
				break;
		}

		len2 = pad - str2 + 1;
	}

	return (SSHORT)icu->ucolStrColl(compareCollator,
		// safe casts - alignment not changed
		reinterpret_cast<const UChar*>(str1), len1,
		reinterpret_cast<const UChar*>(str2), len2);
}


ULONG UnicodeUtil::Utf16Collation::canonical(ULONG srcLen, const USHORT* src, ULONG dstLen, ULONG* dst,
	const ULONG* exceptions)
{
	HalfStaticArray<USHORT, BUFFER_SMALL / 2> upperStr;

	if ((attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ==
		(TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE))
	{
		fb_assert(srcLen % sizeof(*src) == 0);

		memcpy(upperStr.getBuffer(srcLen / sizeof(USHORT)), src, srcLen);

		UErrorCode errorCode = U_ZERO_ERROR;
		UTransliterator* trans = icu->utransOpen("Any-Upper; NFD; [:Nonspacing Mark:] Remove; NFC",
			UTRANS_FORWARD, NULL, 0, NULL, &errorCode);

		if (errorCode <= 0)
		{
			const int32_t capacity = upperStr.getCount();
			int32_t len = srcLen / sizeof(USHORT);
			int32_t limit = len;

			icu->utransTransUChars(trans, reinterpret_cast<UChar*>(upperStr.begin()),
				&len, capacity, 0, &limit, &errorCode);
			icu->utransClose(trans);

			len *= sizeof(USHORT);
			if (ULONG(len) > dstLen)
				len = INTL_BAD_STR_LENGTH;

			srcLen = len;
			src = upperStr.begin();
		}
		else
			return INTL_BAD_STR_LENGTH;
	}
	else if (attributes & TEXTTYPE_ATTR_CASE_INSENSITIVE)
	{
		srcLen = utf16UpperCase(srcLen, src,
			srcLen, upperStr.getBuffer(srcLen / sizeof(USHORT)), exceptions);
		src = upperStr.begin();
	}

	// convert UTF-16 to UTF-32
	USHORT errCode;
	ULONG errPosition;
	return utf16ToUtf32(srcLen, src, dstLen, dst, &errCode, &errPosition) / sizeof(ULONG);
}


UnicodeUtil::ICU* UnicodeUtil::Utf16Collation::loadICU(
	const Firebird::string& collVersion, const Firebird::string& locale,
	const Firebird::string& configInfo)
{
	ObjectsArray<string> versions;
	getVersions(configInfo, versions);

	for (ObjectsArray<string>::const_iterator i(versions.begin()); i != versions.end(); ++i)
	{
		ICU* icu = UnicodeUtil::loadICU(*i, configInfo);
		if (!icu)
			continue;

		if (locale.hasData())
		{
			int avail = icu->ulocCountAvailable();

			while (--avail >= 0)
			{
				if (locale == icu->ulocGetAvailable(avail))
					break;
			}

			if (avail < 0)
				continue;
		}

		char version[U_MAX_VERSION_STRING_LENGTH];
		icu->uVersionToString(icu->collVersion, version);

		if (collVersion != version)
			continue;

		return icu;
	}

	return NULL;
}


}	// namespace Jrd
