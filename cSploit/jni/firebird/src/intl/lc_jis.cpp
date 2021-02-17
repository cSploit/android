/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_jis.cpp
 *	DESCRIPTION:	Language Drivers in the JIS family.
 *
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
 */

#include "firebird.h"
#include "../intl/ldcommon.h"
#include "../intl/kanji.h"

#include "lc_ascii.h"
#include "cv_jis.h"
#include "ld_proto.h"

//static ULONG sjis_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);
//static ULONG sjis_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);

static inline bool FAMILY_MULTIBYTE(texttype* cache,
									SSHORT country,
									const ASCII* POSIX,
									USHORT attributes,
									const UCHAR* /*specific_attributes*/,
									ULONG specific_attributes_length)
//#define FAMILY_MULTIBYTE(id_number, name, charset, country)
{
	if ((attributes & ~TEXTTYPE_ATTR_PAD_SPACE) || specific_attributes_length)
		return false;

	cache->texttype_version			= TEXTTYPE_VERSION_1;
	cache->texttype_name			= POSIX;
	cache->texttype_country			= country;
	cache->texttype_pad_option		= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	cache->texttype_fn_key_length	= famasc_key_length;
	cache->texttype_fn_string_to_key= famasc_string_to_key;
	cache->texttype_fn_compare		= famasc_compare;

	return true;
}


TEXTTYPE_ENTRY3(JIS220_init)
{
	static const ASCII POSIX[] = "C.SJIS";

	if (FAMILY_MULTIBYTE(cache, CC_C, POSIX, attributes, specific_attributes, specific_attributes_length))
	{
		//cache->texttype_fn_str_to_upper = sjis_str_to_upper;
		//cache->texttype_fn_str_to_lower = sjis_str_to_lower;
		return true;
	}

	return false;
}


TEXTTYPE_ENTRY3(JIS230_init)
{
	static const ASCII POSIX[] = "C.EUC_J";

	if (FAMILY_MULTIBYTE(cache, CC_C, POSIX, attributes, specific_attributes, specific_attributes_length))
	{
		//cache->texttype_fn_str_to_upper = famasc_str_to_upper;
		//cache->texttype_fn_str_to_lower = famasc_str_to_lower;
		return true;
	}

	return false;
}


#ifdef NOT_USED_OR_REPLACED
/*
 *	Returns INTL_BAD_STR_LENGTH if output buffer was too small
 *	Note: This function expects Multibyte input
 */
static ULONG sjis_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	bool waiting_for_sjis2 = false;

	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		BYTE c = *pStr++;
		if (waiting_for_sjis2 || SJIS1(c))
		{
			waiting_for_sjis2 = !waiting_for_sjis2;
		}
		else
		{
			if (c >= ASCII_LOWER_A && c <= ASCII_LOWER_Z)
				c = (c - ASCII_LOWER_A + ASCII_UPPER_A);
		}
		*pOutStr++ = c;
		iLen--;
		iOutLen--;
	}
	if (iLen != 0)
		return (INTL_BAD_STR_LENGTH);			// Must have ran out of output space
	return (pOutStr - p);
}


/*
 *	Returns INTL_BAD_STR_LENGTH if output buffer was too small
 *	Note: This function expects Multibyte input
 */
static ULONG sjis_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	bool waiting_for_sjis2 = false;

	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		BYTE c = *pStr++;
		if (waiting_for_sjis2 || SJIS1(c))
		{
			waiting_for_sjis2 = !waiting_for_sjis2;
		}
		else
		{
			if (c >= ASCII_UPPER_A && c <= ASCII_UPPER_Z)
				c = (c - ASCII_UPPER_A + ASCII_LOWER_A);
		}
		*pOutStr++ = c;
		iLen--;
		iOutLen--;
	}
	if (iLen != 0)
		return (INTL_BAD_STR_LENGTH);			// Must have ran out of output space
	return (pOutStr - p);
}
#endif
