/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_dos.cpp
 *	DESCRIPTION:	Language Drivers for compatibility with DOS products.
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
#include "lc_dos.h"
#include "lc_narrow.h"
#include "ld_proto.h"

static inline bool FAMILY1(texttype* cache,
							SSHORT country,
							USHORT flags,
							const SortOrderTblEntry* NoCaseOrderTbl,
							const BYTE* ToUpperConversionTbl,
							const BYTE* ToLowerConversionTbl,
							const CompressPair* CompressTbl,
							const ExpandChar* ExpansionTbl,
							const ASCII* POSIX,
							USHORT attributes,
							const UCHAR*, // specific_attributes,
							ULONG specific_attributes_length)
//#define FAMILY1(id_number, name, charset, country)
{
	if ((attributes & ~TEXTTYPE_ATTR_PAD_SPACE) || specific_attributes_length)
		return false;

	TextTypeImpl* impl = new TextTypeImpl;

	cache->texttype_version			= TEXTTYPE_VERSION_1;
	cache->texttype_name			= POSIX;
	cache->texttype_country			= country;
	cache->texttype_pad_option		= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	cache->texttype_fn_key_length	= LC_NARROW_key_length;
	cache->texttype_fn_string_to_key= LC_NARROW_string_to_key;
	cache->texttype_fn_compare		= LC_NARROW_compare;
	cache->texttype_fn_str_to_upper = fam1_str_to_upper;
	cache->texttype_fn_str_to_lower = fam1_str_to_lower;
	cache->texttype_fn_destroy		= LC_NARROW_destroy;
	cache->texttype_impl 			= impl;
	impl->texttype_collation_table	= (const BYTE*) NoCaseOrderTbl;
	impl->texttype_toupper_table	= ToUpperConversionTbl;
	impl->texttype_tolower_table	= ToLowerConversionTbl;
	impl->texttype_compress_table	= (const BYTE*) CompressTbl;
	impl->texttype_expand_table		= (const BYTE*) ExpansionTbl;
	impl->texttype_flags			= ((flags) & REVERSE) ? (TEXTTYPE_reverse_secondary | TEXTTYPE_ignore_specials) : 0;

	return true;
}



TEXTTYPE_ENTRY3(DOS102_init)
{
	static const ASCII POSIX[] = "INTL.DOS437";

#include "../intl/collations/pd437intl.h"

	return FAMILY1(cache, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS105_init)
{
	static const ASCII POSIX[] = "NORDAN4.DOS437";

#include "../intl/collations/pd865nordan40.h"

	return FAMILY1(cache, CC_NORDAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS106_init)
{
	static const ASCII POSIX[] = "SWEDFIN.DOS437";

#include "../intl/collations/pd437swedfin.h"

	return FAMILY1(cache, CC_SWEDFIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c2_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437de0.h"

	return FAMILY1(cache, CC_GERMANY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c3_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437es1.h"

	return FAMILY1(cache, CC_SPAIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c4_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437fi0.h"

	return FAMILY1(cache, CC_FINLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c5_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437fr0.h"

	return FAMILY1(cache, CC_FRANCE, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c6_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437it0.h"

	return FAMILY1(cache, CC_ITALY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c7_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437nl0.h"

	return FAMILY1(cache, CC_NEDERLANDS, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c8_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437sv0.h"

	return FAMILY1(cache, CC_SWEDEN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c9_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437uk0.h"

	return FAMILY1(cache, CC_UK, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS101_c10_init)
{
	static const ASCII POSIX[] = "DBASE.DOS437";

#include "../intl/collations/db437us0.h"

	return FAMILY1(cache, CC_US, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c1_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850cf0.h"

	return FAMILY1(cache, CC_FRENCHCAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c2_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850de0.h"

	return FAMILY1(cache, CC_GERMANY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c3_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850es0.h"

	return FAMILY1(cache, CC_SPAIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c4_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850fr0.h"

	return FAMILY1(cache, CC_FRANCE, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c5_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850it1.h"

	return FAMILY1(cache, CC_ITALY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c6_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850nl0.h"

	return FAMILY1(cache, CC_NEDERLANDS, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c7_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850pt0.h"

	return FAMILY1(cache, CC_PORTUGAL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c8_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850sv1.h"

	return FAMILY1(cache, CC_SWEDEN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c9_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850uk0.h"

	return FAMILY1(cache, CC_UK, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS160_c10_init)
{
	static const ASCII POSIX[] = "DBASE.DOS850";

#include "../intl/collations/db850us0.h"

	return FAMILY1(cache, CC_US, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS107_c1_init)
{
	static const ASCII POSIX[] = "PDOX.DOS865";

#include "../intl/collations/pd865nordan40.h"

	return FAMILY1(cache, CC_NORDAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS107_c2_init)
{
	static const ASCII POSIX[] = "DBASE.DOS865";

#include "../intl/collations/db865da0.h"

	return FAMILY1(cache, CC_DENMARK, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS107_c3_init)
{
	static const ASCII POSIX[] = "DBASE.DOS865";

#include "../intl/collations/db865no0.h"

	return FAMILY1(cache, CC_NORWAY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c1_init)
{
	static const ASCII POSIX[] = "DBASE.DOS852";

#include "../intl/collations/db852cz0.h"

	return FAMILY1(cache, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c2_init)
{
	static const ASCII POSIX[] = "DBASE.DOS852";

#include "../intl/collations/db852po0.h"

	return FAMILY1(cache, CC_POLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c4_init)
{
	static const ASCII POSIX[] = "DBASE.DOS852";

#include "../intl/collations/db852sl0.h"

	return FAMILY1(cache, CC_YUGOSLAVIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c5_init)
{
	static const ASCII POSIX[] = "PDOX.DOS852";

#include "../intl/collations/pd852czech.h"

	return FAMILY1(cache, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c6_init)
{
	static const ASCII POSIX[] = "PDOX.DOS852";

#include "../intl/collations/pd852polish.h"

	return FAMILY1(cache, CC_POLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c7_init)
{
	static const ASCII POSIX[] = "PDOX.DOS852";

#include "../intl/collations/pd852hundc.h"

	return FAMILY1(cache, CC_HUNGARY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS852_c8_init)
{
	static const ASCII POSIX[] = "PDOX.DOS852";

#include "../intl/collations/pd852slovene.h"

	return FAMILY1(cache, CC_YUGOSLAVIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS857_c1_init)
{
	static const ASCII POSIX[] = "DBASE.DOS857";

#include "../intl/collations/db857tr0.h"

	return FAMILY1(cache, CC_TURKEY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS860_c1_init)
{
	static const ASCII POSIX[] = "DBASE.DOS860";

#include "../intl/collations/db860pt0.h"

	return FAMILY1(cache, CC_PORTUGAL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS861_c1_init)
{
	static const ASCII POSIX[] = "PDOX.DOS861";

#include "../intl/collations/pd861iceland.h"

	return FAMILY1(cache, CC_ICELAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(DOS863_c1_init)
{
	static const ASCII POSIX[] = "DBASE.DOS863";

#include "../intl/collations/db863cf1.h"

	return FAMILY1(cache, CC_FRENCHCAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(CYRL_c1_init)
{
	static const ASCII POSIX[] = "DBASE.CYRL";

#include "../intl/collations/db866ru0.h"

	return FAMILY1(cache, CC_RUSSIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY3(CYRL_c2_init)
{
	static const ASCII POSIX[] = "PDOX.CYRL";

#include "../intl/collations/pd866cyrr.h"

	return FAMILY1(cache, CC_RUSSIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


/*
 * Generic base for InterBase 4.0 Language Driver - family1
 *	Paradox DOS/Windows compatible
 */


#define	LOCALE_UPPER(ch)	\
	(static_cast<TextTypeImpl*>(obj->texttype_impl)->texttype_toupper_table[(unsigned)(ch)])
#define	LOCALE_LOWER(ch)	\
	(static_cast<TextTypeImpl*>(obj->texttype_impl)->texttype_tolower_table[(unsigned)(ch)])



/*
 *	Returns INTL_BAD_STR_LENGTH if output buffer was too small
 */
ULONG fam1_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		*pOutStr++ = LOCALE_UPPER(*pStr);
		pStr++;
		iLen--;
		iOutLen--;
	}
	if (iLen != 0)
		return (INTL_BAD_STR_LENGTH);
	return (pOutStr - p);
}


/*
 *	Returns INTL_BAD_STR_LENGTH if output buffer was too small
 */
ULONG fam1_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		*pOutStr++ = LOCALE_LOWER(*pStr);
		pStr++;
		iLen--;
		iOutLen--;
	}
	if (iLen != 0)
		return (INTL_BAD_STR_LENGTH);
	return (pOutStr - p);
}
