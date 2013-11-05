/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_narrow.cpp
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

#include "firebird.h"
#include "../common/classes/alloc.h"
#include "../intl/ldcommon.h"
#include "../jrd/CharSet.h"
#include "../jrd/IntlUtil.h"
#include "lc_narrow.h"
#include "ld_proto.h"
#include <limits.h>
#include <math.h>

using namespace Firebird;


static bool LC_NARROW_family2(texttype* tt, charset* cs, SSHORT country, USHORT flags,
	const SortOrderTblEntry* noCaseOrderTbl, const BYTE* toUpperConversionTbl,
	const BYTE* toLowerConversionTbl, const CompressPair* compressTbl, const ExpandChar* expansionTbl,
	const ASCII* name, USHORT attributes, const UCHAR* specificAttributes, ULONG specificAttributesLength);
static ULONG fam2_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);
static ULONG fam2_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);


const USHORT LANGFAM2_MAX_KEY	= MAX_KEY;
const BYTE ASCII_SPACE			= 32;
const UINT16 NULL_WEIGHT		= 0;
const UINT16 NULL_SECONDARY		= 0;
const UINT16 NULL_TERTIARY		= 0;


/*
 * key_length (obj, inLen)
 *
 * For an input string of (inLen) bytes, return the maximum
 * key buffer length.
 *
 * This is used for index buffer allocation within the
 * Engine.
 *
 *  Note:
 *	Strings containing (m) expand chars need 2*(m)*3
 *	bytes for key values of the expansion.  This is
 *	offset by string values that don't have corresponding
 *	secondary or tertiary key values.
 *	Let:
 *		n = length of input string
 *		np = count of "simple" bytes in the string, alphabetic
 *		     no secondary and no tertiary.
 *		ns = Has secondary or tertiary, but not both
 *		nt = Has tertiary and secondary.
 *		nc = is a COMPRESSED value.
 *		ne = Has an EXPAND value.
 *		nsp = Is a special value.
 *
 *		n = np + ns + nt + nc + ne + nsp
 *
 *		Key_length(n) =
 *			np
 *			+ 2 * ns
 *			+ 3 * nt
 *			+ 3 * (nc/2)
 *			+ 3 * 2 * ne
 *			+ 2 * nsp
 *			+ 1 (if nsp > 0, for separating keys from special keys)
 *
 *	Clearly this is maximized when the string consists solely of
 *	EXPAND characters.  This degenerate case doesn't occur in
 *	standard text usage, except for short strings (1-2 characters).
 *
 *	Therefore, we compute the keylength based on the "normal" case
 *	of the (nt) term.  It is likely we could choose a probabilistic value
 *	(such as 2.5 * n) for the length of the key.
 *
 *	The degenerate case of short strings is handled by a minimal key
 *	length.
 */
USHORT LC_NARROW_key_length(texttype* obj, USHORT inLen)
{
	// fb_assert (inLen <= LANGFAM2_MAX_KEY); // almost certainly an error

	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);

	// is it the first time key_length is called?
	if (impl->texttype_bytes_per_key == 0)
	{
		BYTE bytesPerChar = 3;

		// scan the table to identify what weights are used
		bool useSecondary = false;
		bool useTertiary = false;

		for (int ch = 0; ch <= 255 && !(useSecondary && useTertiary); ++ch)
		{
			const SortOrderTblEntry* coll =
				&((const SortOrderTblEntry*) impl->texttype_collation_table)[ch];

			if (coll->Secondary != NULL_SECONDARY)
				useSecondary = true;

			if (coll->Tertiary != NULL_TERTIARY)
				useTertiary = true;
		}

		if (!useSecondary)
			--bytesPerChar;

		if (!useTertiary)
			--bytesPerChar;

		if (impl->texttype_flags & TEXTTYPE_non_multi_level)
		{
			if (useSecondary && (impl->texttype_flags & TEXTTYPE_secondary_insensitive))
				--bytesPerChar;

			if (useTertiary && (impl->texttype_flags & TEXTTYPE_tertiary_insensitive))
				--bytesPerChar;
		}

		impl->texttype_bytes_per_key = bytesPerChar;
	}

	USHORT len = impl->texttype_bytes_per_key * MAX(inLen, 2);

	if (impl->texttype_expand_table && ((const ExpandChar*) impl->texttype_expand_table)[0].Ch)
	{
		len += (USHORT) log10(inLen + 1.0) * 4 * impl->texttype_bytes_per_key;
	}

	return (MIN(len, LANGFAM2_MAX_KEY));
}



#ifdef DEBUG
#include <stdio.h>
static ULONG do_debug = 0;
#endif

/*
 *
 *  Convert a user string to a sequence that will collate bytewise.
 *
 * RETURN:
 *		Length, in bytes, of returned key
 */
USHORT LC_NARROW_string_to_key(texttype* obj, USHORT iInLen, const BYTE* pInChar, USHORT iOutLen, BYTE *pOutChar,
							   USHORT key_type)
{
	fb_assert(pOutChar != NULL);
	fb_assert(pInChar != NULL);
	// fb_assert (iInLen   <= LANGFAM2_MAX_KEY);
	fb_assert(iOutLen <= LANGFAM2_MAX_KEY);
	fb_assert(iOutLen >= LC_NARROW_key_length(obj, iInLen));

	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);

#ifdef DEBUG
	// Dump out the input string
	if (do_debug)
	{
		printf("string: (%02d) '%*s'\n", iInLen, iInLen, pInChar);
		fflush(stdout);
	}
#endif	// DEBUG

	BYTE* outbuff = pOutChar;
	USHORT lprimary = 0;
	USHORT lsecondary = 0;
	USHORT ltertiary = 0;
	USHORT lspecial = 0;

	BYTE secondary[LANGFAM2_MAX_KEY];
	BYTE tertiary[LANGFAM2_MAX_KEY];
	BYTE special[LANGFAM2_MAX_KEY * 2];

	// point inbuff at last character
	const BYTE* inbuff = pInChar + iInLen - 1;

	if (obj->texttype_pad_option)
	{
		// skip backwards over all spaces & reset input length
		while ((inbuff >= pInChar) && (*inbuff == ASCII_SPACE))
			inbuff--;
	}

	iInLen = (inbuff - pInChar + 1);

	for (USHORT i = 0; i < iInLen; i++, pInChar++)
	{
		fb_assert(lprimary < iOutLen);
		fb_assert(lsecondary < sizeof(secondary));
		fb_assert(ltertiary < sizeof(tertiary));
		fb_assert(lspecial < sizeof(special));

		const SortOrderTblEntry* coll =
			&((const SortOrderTblEntry*) impl->texttype_collation_table)[*pInChar];

		if (coll->IsExpand && coll->IsCompress)
		{
			// Both flags set indicate a special value

			if (impl->texttype_flags & TEXTTYPE_specials_first)
			{
				if (coll->Primary != NULL_WEIGHT && lprimary < iOutLen)
					outbuff[lprimary++] = coll->Primary + impl->ignore_sum;
				if (coll->Secondary != NULL_SECONDARY && lsecondary < sizeof(secondary))
					secondary[lsecondary++] = coll->Secondary;
				if (coll->Tertiary != NULL_TERTIARY && ltertiary < sizeof(tertiary))
					tertiary[ltertiary++] = coll->Tertiary;
			}
			else
			{
				if ((coll->Primary != NULL_WEIGHT) &&
					!(impl->texttype_flags & TEXTTYPE_ignore_specials) &&
					 lspecial + 1u < sizeof(special))
				{
					special[lspecial++] = (i + 1);	// position
					special[lspecial++] = coll->Primary;
				}
			}
		}
		else if (!((coll->IsExpand && !(impl->texttype_flags & TEXTTYPE_disable_expansions)) ||
				   (coll->IsCompress && !(impl->texttype_flags & TEXTTYPE_disable_compressions))))
		{
			if (coll->Primary != NULL_WEIGHT && lprimary < iOutLen)
				outbuff[lprimary++] = coll->Primary + impl->primary_sum;
			if (coll->Secondary != NULL_SECONDARY && lsecondary < sizeof(secondary))
				secondary[lsecondary++] = coll->Secondary;
			if (coll->Tertiary != NULL_TERTIARY && ltertiary < sizeof(tertiary))
				tertiary[ltertiary++] = coll->Tertiary;
		}
		else if (coll->IsExpand)
		{
			const ExpandChar* exp = &((const ExpandChar*) impl->texttype_expand_table)[0];
			while (exp->Ch && exp->Ch != *pInChar)
				exp++;
			fb_assert(exp->Ch == *pInChar);
			for (int j = 0; j < 2; j++)
			{
				if (j)
					coll = &((const SortOrderTblEntry*) impl->texttype_collation_table)[exp->ExpCh2];
				if (coll->Primary != NULL_WEIGHT && lprimary < iOutLen)
					outbuff[lprimary++] = coll->Primary + impl->primary_sum;
				if (coll->Secondary != NULL_SECONDARY && lsecondary < sizeof(secondary))
					secondary[lsecondary++] = coll->Secondary;
				if (coll->Tertiary != NULL_TERTIARY && ltertiary < sizeof(tertiary))
					tertiary[ltertiary++] = coll->Tertiary;
			}
		}
		else
		{					// (col->IsCompress)
			const bool complete = (USHORT) (i + 1) < iInLen;

			if (complete)
			{
				const CompressPair* cmp = &((const CompressPair*) impl->texttype_compress_table)[0];
				while (cmp->CharPair[0])
				{
					if ((cmp->CharPair[0] == *pInChar) &&
						(cmp->CharPair[1] == *(pInChar + 1)))
					{
						// Gobble the two-to-1 entry
						coll = &cmp->NoCaseWeight;
						pInChar++;
						i++;
						break;
					}
					cmp++;
				}
			}

			// ASF: If key_type == INTL_KEY_PARTIAL and the last CompressPair
			// isn't complete, don't put the byte in the sortkey. If we put,
			// incorrect results occur when using index.
			if (key_type != INTL_KEY_PARTIAL || complete)
			{
				if (coll->Primary != NULL_WEIGHT && lprimary < iOutLen)
					outbuff[lprimary++] = coll->Primary + impl->primary_sum;
				if (coll->Secondary != NULL_SECONDARY && lsecondary < sizeof(secondary))
					secondary[lsecondary++] = coll->Secondary;
				if (coll->Tertiary != NULL_TERTIARY && ltertiary < sizeof(tertiary))
					tertiary[ltertiary++] = coll->Tertiary;
			}
		}
	}

	// primary keys are already in output key

	outbuff += lprimary;
	iOutLen -= lprimary;

	if (key_type == INTL_KEY_PARTIAL)
	{
		// return length of key
		return (outbuff - pOutChar);
	}

	bool useLevel = !(impl->texttype_flags & TEXTTYPE_secondary_insensitive);

	if (!(impl->texttype_flags & TEXTTYPE_non_multi_level))	// multi-level
	{
		if (key_type == INTL_KEY_SORT)
			useLevel = true;
	}

	if (useLevel)
	{
		// put secondary keys into output key
		if (impl->texttype_flags & TEXTTYPE_reverse_secondary)
		{
			for (USHORT i = 0; i < lsecondary && iOutLen; i++) {
				*outbuff++ = secondary[lsecondary - i - 1];
				iOutLen--;
			}
		}
		else
		{
			for (USHORT i = 0; i < lsecondary && iOutLen; i++) {
				*outbuff++ = secondary[i];
				iOutLen--;
			}
		}
	}

	useLevel = !(impl->texttype_flags & TEXTTYPE_tertiary_insensitive);

	if (!(impl->texttype_flags & TEXTTYPE_non_multi_level))	// multi-level
	{
		if (key_type == INTL_KEY_SORT)
			useLevel = true;
	}

	if (useLevel)
	{
		// put tertiary keys into output key
		for (USHORT i = 0; i < ltertiary && iOutLen; i++) {
			*outbuff++ = tertiary[i];
			iOutLen--;
		}
	}

	// put special keys into output key
	if ((lspecial && iOutLen) && !(impl->texttype_flags & TEXTTYPE_ignore_specials))
	{
		// Insert the marker-byte
		*outbuff++ = 0;
		iOutLen--;
		for (USHORT i = 0; i < lspecial && iOutLen; i++) {
			*outbuff++ = special[i];
			iOutLen--;
		}
	}

#ifdef DEBUG
	// Dump out the computed key
	if (do_debug)
	{
		printf("   key: (%02d) ", (outbuff - pOutChar));
		for (const UCHAR* p = pOutChar; p < outbuff; p++)
			printf("%2x ", *p);
		printf("\n");
		fflush(stdout);
	}
#endif

	// return length of key
	return (outbuff - pOutChar);
}



#define	LC_HAVE_WAITING	1
#define LC_HAVE_SPECIAL	2

// expansion char go before the expansion.
// eg: S-set collates before ss

struct coltab_status
{
	USHORT stat_flags;
	const SortOrderTblEntry* stat_waiting;
};




static SSHORT special_scan(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2)
{
	const SortOrderTblEntry* col1 = 0;
	const SortOrderTblEntry* col2 = 0;

	ULONG index1 = 0;
	ULONG index2 = 0;

	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);
	const bool noSpecialsFirst = !(impl->texttype_flags & TEXTTYPE_specials_first);

	while (true)
	{
		// Scan to find ignore char from l1
		while (l1)
		{
			col1 = &((const SortOrderTblEntry*) impl->texttype_collation_table)[*s1];

			if (col1->IsExpand && col1->IsCompress && noSpecialsFirst)
			{
				break;
			}

			l1--;
			s1++;
			index1++;
		}

		// Scan to find ignore char from l2
		while (l2)
		{
			col2 = &((const SortOrderTblEntry*) impl->texttype_collation_table)[*s2];
			if (col2->IsExpand && col2->IsCompress && noSpecialsFirst)
			{
				break;
			}

			l2--;
			s2++;
			index2++;
		}
		if (!l1 && !l2)			// All out of ignore characters
			return 0;
		if (l1 && !l2)			// Out in l2 only
			return 1000;
		if (!l1 && l2)			// Out in l1 only
			return -1000;
		if (index1 < index2)	// l1 has ignore ch before l2
			return -2000;
		if (index1 > index2)	// l2 has ignore ch before l1
			return 2000;
		if (col1->Primary != col2->Primary)
			return (col1->Primary - col2->Primary);
		l1--;
		l2--;
		s1++;
		s2++;
		index1++;
		index2++;
	}
}


static const SortOrderTblEntry* get_coltab_entry(texttype* obj, const UCHAR** p,
	ULONG* l, coltab_status* stat, int* sum)
{
	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);

	*sum = impl->primary_sum;

	if (stat->stat_flags & LC_HAVE_WAITING)
	{
		--*l;
		++*p;
		stat->stat_flags &= ~LC_HAVE_WAITING;
		fb_assert(stat->stat_waiting);
		return stat->stat_waiting;
	}

	stat->stat_waiting = NULL;
	while (*l)
	{
		const SortOrderTblEntry* col =
			&((const SortOrderTblEntry*) impl->texttype_collation_table)[**p];

		if (col->IsExpand && col->IsCompress)
		{
			if (impl->texttype_flags & TEXTTYPE_specials_first)
			{
				*sum = impl->ignore_sum;

				// Have col
				--*l;
				++*p;
				return col;
			}

			// Both flags set indicate a special value
			// Need a new col
			--*l;
			++*p;
			stat->stat_flags |= LC_HAVE_SPECIAL;
			continue;
		}

		if (!((col->IsExpand && !(impl->texttype_flags & TEXTTYPE_disable_expansions)) ||
				   (col->IsCompress && !(impl->texttype_flags & TEXTTYPE_disable_compressions))))
		{
			// Have col
			--*l;
			++*p;
			return col;
		}

		if (col->IsExpand)
		{
			const ExpandChar* exp = &((const ExpandChar*) impl->texttype_expand_table)[0];
			while (exp->Ch && exp->Ch != **p)
				exp++;
			fb_assert(exp->Ch == **p);
			// Have coll1
			// Have waiting

			stat->stat_waiting = &((const SortOrderTblEntry*) impl->texttype_collation_table)[exp->ExpCh2];
			stat->stat_flags |= LC_HAVE_WAITING;
			return col;
		}

		// (col->IsCompress)
		if (*l > 1)
		{
			const CompressPair* cmp = &((const CompressPair*) impl->texttype_compress_table)[0];
			while (cmp->CharPair[0])
			{
				if ((cmp->CharPair[0] == **p) &&
					(cmp->CharPair[1] == *(*p + 1)))
				{
					// Have Col
					col = &cmp->NoCaseWeight;
					(*l) -= 2;
					(*p) += 2;
					return col;
				}
				cmp++;
			}
		}
		// Have col
		--*l;
		++*p;
		return col;
	}
	return NULL;
}



#define XOR	^					// C bitwise XOR operator - defined for readability

SSHORT LC_NARROW_compare(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2,
	INTL_BOOL* error_flag)
{
	fb_assert(obj != NULL);
	fb_assert(s1 != NULL);
	fb_assert(s2 != NULL);
	fb_assert(error_flag != NULL);

	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);

	*error_flag = false;

	if (obj->texttype_pad_option)
	{
		// Start at EOS, scan backwards to find non-space
		const BYTE* p = s1 + l1 - 1;
		while ((p >= s1) && (*p == ASCII_SPACE))
			p--;
		l1 = (p - s1 + 1);

		p = s2 + l2 - 1;
		while ((p >= s2) && (*p == ASCII_SPACE))
			p--;
		l2 = (p - s2 + 1);
	}

	const ULONG save_l1 = l1;
	const ULONG save_l2 = l2;
	const BYTE* const save_s1 = s1;
	const BYTE* const save_s2 = s2;
	SSHORT save_secondary = 0;
	SSHORT save_tertiary = 0;
	SSHORT save_quandary = 0;

	coltab_status stat1, stat2;
	stat1.stat_flags = 0;
	stat2.stat_flags = 0;

	const SortOrderTblEntry* col1 = 0;
	const SortOrderTblEntry* col2 = 0;

	while (true)
	{
		int sum1, sum2;

		col1 = get_coltab_entry(obj, &s1, &l1, &stat1, &sum1);
		col2 = get_coltab_entry(obj, &s2, &l2, &stat2, &sum2);

		if (!col1 || !col2)
			break;
		if (col1->Primary + sum1 != col2->Primary + sum2)
			return ((col1->Primary + sum1) - (col2->Primary + sum2));
		if ((impl->texttype_flags & TEXTTYPE_secondary_insensitive) == 0 &&
			col1->Secondary != col2->Secondary)
		{
			if ((impl->texttype_flags & TEXTTYPE_reverse_secondary) || !save_secondary)
			{
				save_secondary = (col1->Secondary - col2->Secondary);
			}
		}
		else if ((impl->texttype_flags & TEXTTYPE_tertiary_insensitive) == 0 &&
				 col1->Tertiary != col2->Tertiary)
		{
			if (!save_tertiary)
				save_tertiary = (col1->Tertiary - col2->Tertiary);
		}
		else if (((stat1.stat_flags & LC_HAVE_WAITING) XOR
				  (stat2.stat_flags & LC_HAVE_WAITING)) && !save_quandary)
		{
			if (impl->texttype_flags & TEXTTYPE_expand_before)
				save_quandary = (stat1.stat_flags & LC_HAVE_WAITING) ? -1 : 1;
			else
				save_quandary = (stat1.stat_flags & LC_HAVE_WAITING) ? 1 : -1;
		}
	}

	// One of the strings ended
	fb_assert(l1 == 0 || l2 == 0);
	fb_assert(col1 == NULL || col2 == NULL);

	if (col1 && !col2)
		return 500;

	if (!col1 && col2)
		return -500;

	if (l1 == 0 && l2 == 0)
	{
		if (save_secondary)
			return save_secondary;
		if (save_tertiary)
			return save_tertiary;
		if (save_quandary)
			return save_quandary;
		if (((stat1.stat_flags & LC_HAVE_SPECIAL) ||
				(stat2.stat_flags & LC_HAVE_SPECIAL)) &&
			!(impl->texttype_flags & TEXTTYPE_ignore_specials) &&
			!(impl->texttype_flags & TEXTTYPE_specials_first))
		{
			return special_scan(obj, save_l1, save_s1, save_l2, save_s2);
		}
		return 0;
	}

	if (l1)
		return 600;
	return -600;
}



#ifdef DEBUG_COMPARE

/*
 * Debugging only
 * Routine used for comparing results from comparison algorithm
 * to results from key creation algorithm
 */
static SSHORT old_fam2_compare(texttype* obj, ULONG l1, const BYTE* s1,
	ULONG l2, const BYTE* s2, INTL_BOOL* error_flag)
{
	BYTE key1[LANGFAM2_MAX_KEY];
	BYTE key2[LANGFAM2_MAX_KEY];

	fb_assert(obj != NULL);
	fb_assert(s1 != NULL);
	fb_assert(s2 != NULL);

	const ULONG len1 = LC_NARROW_string_to_key(obj, l1, s1, sizeof(key1), key1, INTL_KEY_SORT);
	const ULONG len2 = LC_NARROW_string_to_key(obj, l2, s2, sizeof(key2), key2, INTL_KEY_SORT);
	const ULONG len = MIN(len1, len2);
	for (ULONG i = 0; i < len; i++)
	{
		if (key1[i] == key2[i])
			continue;
		if (key1[i] < key2[i])
			return (-1);

		return (1);
	}

	if (len1 < len2)
		return (-1);
	if (len1 > len2)
		return (1);

	return (0);
}
#endif	// DEBUG_COMPARE


#ifdef DEBUG_COMPARE

#define	SIGN(x)	(((x) < 0) ? -1 : (((x) == 0) ? 0 : 1))

/*
 * Debugging only
 * Routine used for comparing results from comparision algorithm
 * to results from key creation algorithm
 */
static SSHORT fam2_compare(texttype* obj, ULONG l1, const BYTE* s1,
	ULONG l2, const BYTE* s2, INTL_BOOL* error_flag)
{
	SSHORT res1 = old_fam2_compare(obj, l1, s1, l2, s2);
	SSHORT res2 = LC_NARROW_compare(obj, l1, s1, l2, s2);

	if (SIGN(res1) != SIGN(res2))
	{
		printf("different compares:\n%d %s\n%d %s\nold = %d new = %d\n",
				  l1, s1, l2, s2, res1, res2);
		fflush(stdout);
		do_debug = 1;
		res1 = old_fam2_compare(obj, l1, s1, l2, s2);
		res2 = LC_NARROW_compare(obj, l1, s1, l2, s2);
		do_debug = 0;
	}
	return res2;
}

#endif	// DEBUG_COMPARE

ULONG LC_NARROW_canonical(texttype* obj, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst)
{
	fb_assert(dst != NULL);
	fb_assert(src != NULL);
	fb_assert(dstLen >= obj->texttype_canonical_width * srcLen);

	TextTypeImpl* impl = static_cast<TextTypeImpl*>(obj->texttype_impl);
	const BYTE* const inbuff = src;

	for (ULONG i = 0; i < srcLen; i++, src++)
	{
		const SortOrderTblEntry* coll =
			&((const SortOrderTblEntry*) impl->texttype_collation_table)[*src];

		USHORT primary = coll->Primary;

		if (coll->IsExpand && coll->IsCompress)
			primary += impl->ignore_sum_canonic;
		else
			primary += impl->primary_sum_canonic;

		if ((impl->texttype_flags & (TEXTTYPE_secondary_insensitive | TEXTTYPE_tertiary_insensitive)) == 0)
		{
			put(dst, (USHORT) ((primary << 8) | (coll->Secondary << 4) | coll->Tertiary));
		}
		else if ((impl->texttype_flags & TEXTTYPE_secondary_insensitive) == 0)
		{
			put(dst, (USHORT) ((primary << 8) | coll->Secondary));
		}
		else if ((impl->texttype_flags & TEXTTYPE_tertiary_insensitive) == 0)
		{
			put(dst, (USHORT) ((primary << 8) | coll->Tertiary));
		}
		else
			*dst++ = primary;
	}

	return src - inbuff;
}


void LC_NARROW_destroy(texttype* obj)
{
	delete static_cast<TextTypeImpl*>(obj->texttype_impl);
}



bool LC_NARROW_family2(
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
	ULONG specificAttributesLength)
{
	if (attributes & ~TEXTTYPE_ATTR_PAD_SPACE)
		return false;

	TextTypeImpl* impl = FB_NEW(*getDefaultMemoryPool()) TextTypeImpl;

	tt->texttype_version			= TEXTTYPE_VERSION_1;
	tt->texttype_name				= name;
	tt->texttype_country			= country;
	tt->texttype_pad_option			= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	tt->texttype_fn_key_length		= LC_NARROW_key_length;
	tt->texttype_fn_string_to_key	= LC_NARROW_string_to_key;
	tt->texttype_fn_compare			= LC_NARROW_compare;
	tt->texttype_fn_str_to_upper	= fam2_str_to_upper;
	tt->texttype_fn_str_to_lower	= fam2_str_to_lower;
	tt->texttype_fn_destroy			= LC_NARROW_destroy;

	tt->texttype_impl				= impl;
	impl->texttype_collation_table	= (const BYTE*) noCaseOrderTbl;
	impl->texttype_toupper_table	= toUpperConversionTbl;
	impl->texttype_tolower_table	= toLowerConversionTbl;
	impl->texttype_compress_table	= (const BYTE*) compressTbl;
	impl->texttype_expand_table		= (const BYTE*) expansionTbl;
	impl->texttype_flags			= ((flags) & REVERSE) ? TEXTTYPE_reverse_secondary : 0;
	impl->texttype_bytes_per_key	= 0;

	IntlUtil::SpecificAttributesMap map;
	Jrd::CharSet* charSet = NULL;

	try
	{
		charSet = Jrd::CharSet::createInstance(*getDefaultMemoryPool(), 0, cs);

		if (!IntlUtil::parseSpecificAttributes(charSet, specificAttributesLength, specificAttributes, &map))
		{
			delete charSet;
			return false;
		}

		delete charSet;
	}
	catch (...)
	{
		delete charSet;
		return false;
	}

	int validAttributeCount = 0;
	string value;

	if (map.get("DISABLE-COMPRESSIONS", value) && (value == "0" || value == "1"))
	{
		++validAttributeCount;
		if (value == "1")
			impl->texttype_flags |= TEXTTYPE_disable_compressions;
	}

	if (map.get("DISABLE-EXPANSIONS", value) && (value == "0" || value == "1"))
	{
		++validAttributeCount;
		if (value == "1")
			impl->texttype_flags |= TEXTTYPE_disable_expansions;
	}

	int minPrimary = INT_MAX;
	int maxIgnore = 0;
	int maxPrimary = 0;

	if (!(impl->texttype_flags & TEXTTYPE_disable_compressions))
	{
		while (compressTbl->CharPair[0])
		{
			if (compressTbl->NoCaseWeight.Primary > maxPrimary)
				maxPrimary = compressTbl->NoCaseWeight.Primary;

			if (compressTbl->NoCaseWeight.Primary < minPrimary)
				minPrimary = compressTbl->NoCaseWeight.Primary;

			++compressTbl;
		}
	}

	for (int ch = 0; ch <= 255; ++ch)
	{
		const SortOrderTblEntry* coll =
			&((const SortOrderTblEntry*) impl->texttype_collation_table)[ch];

		if (coll->IsExpand && coll->IsCompress)
		{
			if (coll->Primary > maxIgnore)
				maxIgnore = coll->Primary;
		}
		else if (!((coll->IsExpand && !(impl->texttype_flags & TEXTTYPE_disable_expansions)) ||
				   (coll->IsCompress && !(impl->texttype_flags & TEXTTYPE_disable_compressions))))
		{
			if (coll->Primary > maxPrimary)
				maxPrimary = coll->Primary;

			if (coll->Primary < minPrimary)
				minPrimary = coll->Primary;
		}
	}

	if (map.get("SPECIALS-FIRST", value) && (value == "0" || value == "1"))
	{
		if (maxIgnore > 0 && maxPrimary + maxIgnore - 1 <= 255)
		{
			++validAttributeCount;

			if (value == "1")
			{
				impl->texttype_flags |= TEXTTYPE_specials_first;
				impl->ignore_sum = minPrimary - 1;
				impl->primary_sum = maxIgnore - 1;
			}
		}
	}

	if (maxIgnore > 0 && maxPrimary + maxIgnore - 1 <= 255)
	{
		impl->ignore_sum_canonic = minPrimary - 1;
		impl->primary_sum_canonic = maxIgnore - 1;
	}

	if (map.count() - validAttributeCount != 0)
		return false;

	return true;
}


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
	ULONG specificAttributesLength)
{
	bool multiLevel = false;

	IntlUtil::SpecificAttributesMap map;
	Jrd::CharSet* charSet = NULL;
	string newSpecificAttributes;

	try
	{
		charSet = Jrd::CharSet::createInstance(*getDefaultMemoryPool(), 0, cs);

		if (!IntlUtil::parseSpecificAttributes(charSet, specificAttributesLength, specificAttributes, &map))
		{
			delete charSet;
			return false;
		}

		string value;
		if (map.get("MULTI-LEVEL", value))
		{
			if (value == "0")
				multiLevel = false;
			else if (value == "1")
				multiLevel = true;
			else
				return false;

			map.remove("MULTI-LEVEL");
		}

		newSpecificAttributes = IntlUtil::generateSpecificAttributes(charSet, map);
		specificAttributes = (const UCHAR*)newSpecificAttributes.begin();
		specificAttributesLength = newSpecificAttributes.length();

		delete charSet;
	}
	catch (...)
	{
		delete charSet;
		return false;
	}

	if (LC_NARROW_family2(tt, cs, country, flags, noCaseOrderTbl,
				toUpperConversionTbl, toLowerConversionTbl, compressTbl, expansionTbl, name,
				attributes & ~(TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE),
				specificAttributes, specificAttributesLength))
	{
		TextTypeImpl* impl = static_cast<TextTypeImpl*>(tt->texttype_impl);

		if (!multiLevel)
			impl->texttype_flags |= TEXTTYPE_non_multi_level;

		if (attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE))
		{
			impl->texttype_flags |= TEXTTYPE_ignore_specials;

			if (multiLevel)
			{
				tt->texttype_flags |= TEXTTYPE_SEPARATE_UNIQUE;

				if ((attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ==
					TEXTTYPE_ATTR_ACCENT_INSENSITIVE)
				{
					tt->texttype_flags |= TEXTTYPE_UNSORTED_UNIQUE;
				}
			}

			if ((attributes & (TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE)) ==
				(TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE))
			{
				tt->texttype_canonical_width = 1;
			}
			else
				tt->texttype_canonical_width = 2;

			tt->texttype_fn_canonical = LC_NARROW_canonical;

			if (attributes & TEXTTYPE_ATTR_ACCENT_INSENSITIVE)
				impl->texttype_flags |= TEXTTYPE_secondary_insensitive;

			if (attributes & TEXTTYPE_ATTR_CASE_INSENSITIVE)
				impl->texttype_flags |= TEXTTYPE_tertiary_insensitive;
		}

		return true;
	}

	return false;
}


/*
 *	Returns INTL_BAD_STR_LENGTH if output buffer was too small
 */
static ULONG fam2_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		*pOutStr++ = static_cast<TextTypeImpl*>(obj->texttype_impl)->texttype_toupper_table[(unsigned) *pStr];
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
static ULONG fam2_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr)
{
	fb_assert(pStr != NULL);
	fb_assert(pOutStr != NULL);
	fb_assert(iOutLen >= iLen);
	const BYTE* const p = pOutStr;
	while (iLen && iOutLen)
	{
		*pOutStr++ = static_cast<TextTypeImpl*>(obj->texttype_impl)->texttype_tolower_table[(unsigned) *pStr];
		pStr++;
		iLen--;
		iOutLen--;
	}
	if (iLen != 0)
		return (INTL_BAD_STR_LENGTH);
	return (pOutStr - p);
}
