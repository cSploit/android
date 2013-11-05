/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_narrow.h
 *	DESCRIPTION:	Common base for Narrow language drivers
 *			(full International collation)
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

#define TEXTTYPE_reverse_secondary				0x001	// Reverse order of secondary keys
#define TEXTTYPE_ignore_specials				0x002	// Do not put special values in keys
#define TEXTTYPE_expand_before					0x004	// Expansion weights before litagure
#define TEXTTYPE_secondary_insensitive			0x008	// Don't use secondary level for comparisions
#define TEXTTYPE_tertiary_insensitive			0x010	// Don't use tertiary level for comparisions
#define TEXTTYPE_non_multi_level				0x020	// Sortkey isn't more precise than equivalence class
#define TEXTTYPE_specials_first					0x040	// Sort special characters in the primary level
#define TEXTTYPE_disable_compressions			0x080	// Disable compression characters
#define TEXTTYPE_disable_expansions				0x100	// Disable expansion characters

namespace
{
	struct TextTypeImpl
	{
		TextTypeImpl()
			: texttype_flags(0),
			  texttype_bytes_per_key(0),
			  texttype_collation_table(NULL),
			  texttype_expand_table(NULL),
			  texttype_compress_table(NULL),
			  texttype_toupper_table(NULL),
			  texttype_tolower_table(NULL),
			  ignore_sum(0),
			  primary_sum(0),
			  ignore_sum_canonic(0),
			  primary_sum_canonic(0)
		{
		}

		USHORT texttype_flags;
		BYTE texttype_bytes_per_key;
		const BYTE* texttype_collation_table;
		const BYTE* texttype_expand_table;
		const BYTE* texttype_compress_table;
		const BYTE* texttype_toupper_table;
		const BYTE* texttype_tolower_table;
		int ignore_sum;
		int primary_sum;
		int ignore_sum_canonic;
		int primary_sum_canonic;
	};
}

USHORT LC_NARROW_key_length(texttype* obj, USHORT inLen);
USHORT LC_NARROW_string_to_key(texttype* obj, USHORT iInLen, const BYTE* pInChar,
	USHORT iOutLen, BYTE *pOutChar, USHORT partial);
SSHORT LC_NARROW_compare(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2,
	INTL_BOOL* error_flag);
ULONG LC_NARROW_canonical(texttype* obj, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst);
void LC_NARROW_destroy(texttype* obj);

bool LC_NARROW_family3(
	texttype* tt,
	charset* cs,
	SSHORT country,
	USHORT flags,
	const SortOrderTblEntry* noCaseOrderTbl,
	const BYTE* toUpperConversionTbl,
	const BYTE* toLowerConversionTbl,
	const CompressPair* compressTbl,
	const ExpandChar* expansionTbl,
	const ASCII* name,
	USHORT attributes,
	const UCHAR* specificAttributes,
	ULONG specificAttributesLength);
