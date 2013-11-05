/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_ksc.cpp
 *	DESCRIPTION:	Language Drivers in the KSC-5601.
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
#include "lc_ascii.h"
#include "cv_ksc.h"
#include "ld_proto.h"

static USHORT LCKSC_string_to_key(texttype* obj, USHORT iInLen, const BYTE* pInChar,
	USHORT iOutLen, BYTE *pOutChar, USHORT);
static USHORT LCKSC_key_length(texttype* obj, USHORT inLen);
static SSHORT LCKSC_compare(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2, INTL_BOOL* error_flag);

static int GetGenHanNdx(UCHAR b1, UCHAR b2);
static int GetSpeHanNdx(UCHAR b2);

static inline bool FAMILY_MULTIBYTE(texttype* cache,
									SSHORT country,
									const ASCII* POSIX,
									USHORT attributes,
									const UCHAR*, // specific_attributes,
									ULONG specific_attributes_length)
{
	//static inline void FAMILY_MULTIBYTE(id_number, name, charset, country)
	if ((attributes & ~TEXTTYPE_ATTR_PAD_SPACE) || specific_attributes_length)
		return false;

	cache->texttype_version			= TEXTTYPE_VERSION_1;
	cache->texttype_name			= POSIX;
	cache->texttype_country			= country;
	cache->texttype_pad_option		= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	cache->texttype_fn_key_length	= famasc_key_length;
	cache->texttype_fn_string_to_key= famasc_string_to_key;
	cache->texttype_fn_compare		= famasc_compare;
	//cache->texttype_fn_str_to_upper = famasc_str_to_upper;
	//cache->texttype_fn_str_to_lower = famasc_str_to_lower;

	return true;
}

TEXTTYPE_ENTRY3(KSC_5601_init)
{
	static const ASCII POSIX[] = "C.KSC_5601";

	return FAMILY_MULTIBYTE(cache, CC_C, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(ksc_5601_dict_init)
{
	static const ASCII POSIX[] = "HANGUL.KSC_5601";

	if (FAMILY_MULTIBYTE(cache, CC_KOREA, POSIX, attributes, specific_attributes, specific_attributes_length))
	{
		cache->texttype_fn_key_length = LCKSC_key_length;
		cache->texttype_fn_string_to_key = LCKSC_string_to_key;
		cache->texttype_fn_compare = LCKSC_compare;
		return true;
	}

	return false;
}


const UCHAR spe_han[18][2] =
{
	// special hangul -> character sets with dictionary collation
	{ 0xa4, 0xa2 },
	{ 0xa4, 0xa4 },
	{ 0xa4, 0xa7 },
	{ 0xa4, 0xa8 },
	{ 0xa4, 0xa9 },
	{ 0xa4, 0xb1 },
	{ 0xa4, 0xb2 },
	{ 0xa4, 0xb3 },
	{ 0xa4, 0xb5 },
	{ 0xa4, 0xb6 },
	{ 0xa4, 0xb7 },
	{ 0xa4, 0xb8 },
	{ 0xa4, 0xb9 },
	{ 0xa4, 0xba },
	{ 0xa4, 0xbb },
	{ 0xa4, 0xbc },
	{ 0xa4, 0xbd },
	{ 0xa4, 0xbe }
};

const UCHAR gen_han[18][2] =
{
	// general hangul -> character sets with binary collation
	{ 0xb1, 0xed },
	{ 0xb3, 0xa9 },
	{ 0xb4, 0xd8 },
	{ 0xb5, 0xfa },
	{ 0xb6, 0xf2 },
	{ 0xb8, 0xb5 },
	{ 0xb9, 0xd8 },
	{ 0xba, 0xfb },
	{ 0xbb, 0xe6 },
	{ 0xbd, 0xcd },
	{ 0xbe, 0xc5 },
	{ 0xc0, 0xd9 },
	{ 0xc2, 0xa4 },
	{ 0xc2, 0xf6 },
	{ 0xc4, 0xaa },
	{ 0xc5, 0xb7 },
	{ 0xc6, 0xc3 },
	{ 0xc7, 0xce }
};

const USHORT LANGKSC_MAX_KEY	= 4096;
const BYTE	ASCII_SPACE	= 32;


static USHORT LCKSC_string_to_key(texttype* obj, USHORT iInLen, const BYTE* pInChar,
	USHORT iOutLen, BYTE *pOutChar,
	USHORT /*key_type*/)
{
	fb_assert(pOutChar != NULL);
	fb_assert(pInChar != NULL);
	fb_assert(iInLen <= LANGKSC_MAX_KEY);
	fb_assert(iOutLen <= LANGKSC_MAX_KEY);
	fb_assert(iOutLen >= LCKSC_key_length(obj, iInLen));

	const BYTE* inbuff = pInChar + iInLen - 1;
	while ((inbuff >= pInChar) && (*inbuff == ASCII_SPACE))
		inbuff--;
	iInLen = (inbuff - pInChar + 1);

	BYTE* outbuff = pOutChar;

	for (USHORT i = 0; i < iInLen && iOutLen; i++, pInChar++)
	{
		if (GEN_HAN(*pInChar, *(pInChar + 1)))
		{	// general hangul
			const int idx = GetGenHanNdx(*pInChar, *(pInChar + 1));
			if (idx >= 0)
			{
				if (iOutLen < 3)
					break;

				*outbuff++ = gen_han[idx][0];
				*outbuff++ = gen_han[idx][1];
				*outbuff++ = 1;
				iOutLen -= 3;
			}
			else
			{
				if (iOutLen < 2)
					break;

				*outbuff++ = *pInChar;
				*outbuff++ = *(pInChar + 1);
				iOutLen -= 2;
			}
			pInChar += 1;
			i++;
		}
		else if (SPE_HAN(*pInChar, *(pInChar + 1)))
		{	// special hangul
			const int idx = GetSpeHanNdx(*(pInChar + 1));
			fb_assert(idx >= 0);

			if (iOutLen < 3)
				break;

			*outbuff++ = gen_han[idx][0];
			*outbuff++ = gen_han[idx][1];
			*outbuff++ = 2;
			iOutLen -= 3;
			pInChar += 1;
			i++;
		}
		else
		{					// ascii or rest -> in case with binary collation

			*outbuff++ = *pInChar;
			iOutLen--;
			fb_assert(KSC1(*pInChar) || (*pInChar < 0x80));
			if (KSC1(*pInChar))
			{	// the rest characters of KSC_5601 table
				fb_assert(KSC2(*(pInChar + 1)));
				if (!iOutLen)
					break;
				*outbuff++ = *(pInChar + 1);
				iOutLen--;
				pInChar += 1;
				i++;
			}
			else				// ascii
				continue;
		}
	}

	return (outbuff - pOutChar);
}


/*
*	function name	:	GetGenHanNdx
*	description	:	in case of gen_han, get the index number from gen_han table
*/

static int GetGenHanNdx(UCHAR b1, UCHAR b2)
{
	for (int i = 0; i < 18; i++)
	{
		if (gen_han[i][0] == b1 && b2 == gen_han[i][1])
			return i;
	}
	return -1;
}


/*
*	function name	:	GetSpeHanNdx
*	description	:	in case of spe_han, get index from spe_han table
*/

static int GetSpeHanNdx(const UCHAR b2)
{
	for (int i = 0; i < 18; i++)
	{
		if (b2 == spe_han[i][1])
			return i;
	}
	return -1;
}


static USHORT LCKSC_key_length(texttype* /*obj*/, USHORT inLen)
{
	const USHORT len = inLen + (inLen / 2);

	return (MIN(len, LANGKSC_MAX_KEY));
}


/*
*	function name	:	LCKSC_compare
*	description	:	compare two string
*/
static SSHORT LCKSC_compare(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2, INTL_BOOL* error_flag)
{
	fb_assert(error_flag != NULL);

	BYTE key1[LANGKSC_MAX_KEY];
	BYTE key2[LANGKSC_MAX_KEY];

	*error_flag = false;

	const ULONG len1 = LCKSC_string_to_key(obj, l1, s1, sizeof(key1), key1, 0);
	const ULONG len2 = LCKSC_string_to_key(obj, l2, s2, sizeof(key2), key2, 0);
	const ULONG len = MIN(len1, len2);
	for (ULONG i = 0; i < len; i++)
	{
		if (key1[i] == key2[i])
			continue;
		if (key1[i] < key2[i])
			return -1;

		return 1;
	}
	if (len1 < len2)
		return -1;
	if (len1 > len2)
		return 1;

	return 0;
}
