/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		kanji.cpp
 *	DESCRIPTION:
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
#include <stdio.h>
#include "../intl/ldcommon.h"
#include "kanji.h"
#include "kanji_proto.h"


static void S2E(const UCHAR s1, const UCHAR s2, UCHAR& j1, UCHAR& j2)
{
	if (s2 >= 0x9f)
	{
		if (s1 >= 0xe0)
			j1 = (s1 * 2 - 0xe0);
    	else
			j1 = (s1 * 2 - 0x60);
		j2 = (s2 + 2);
	}
	else
	{
		if (s1 >= 0xe0)
			j1 = (s1 * 2 - 0xe1);
		else
			j1 = (s1 * 2 - 0x61);
		if (s2 >= 0x7f)
			j2 = (s2 + 0x60);
		else
			j2 = (s2 +  0x61);
	}
}


USHORT KANJI_check_euc(const UCHAR* euc_str, USHORT euc_len)
{
/**************************************
 *
 *      K A N J I _ c h e c k _ e u c
 *
 **************************************
 *
 * Functional description
 *	This is a cousin of the KANJI_check_sjis routine.
 *      Make sure that the euc string does not have any truncated 2 byte
 *      character at the end. * If we have a truncated character then,
 *          return 1.
 *          else return(0);
 **************************************/
	while (euc_len--)
	{
		if (*euc_str & 0x80)
		{
			// Is it  EUC
			if (euc_len == 0) {
				// truncated kanji
				return (1);
			}

			euc_str += 2;
			euc_len -= 1;
		}
		else {
			// it is a ASCII
			euc_str++;
		}
	}
	return (0);
}


USHORT KANJI_check_sjis(const UCHAR* sjis_str, USHORT sjis_len)
{
/**************************************
 *
 *      K A N J I _ c h e c k _ s j i s
 *
 **************************************
 *
 * Functional description
 *	This is a cousin of the KANJI_check_euc routine.
 *      Make sure that the sjis string does not have any truncated 2 byte
 *	character at the end. *	If we have a truncated character then,
 *	    return 1.
 *	    else return(0);
 **************************************/
	while (sjis_len--)
	{
		if (*sjis_str & 0x80)
		{
			// Is it  SJIS
			if SJIS1(*sjis_str)
			{
				// It is a KANJI
				if (sjis_len == 0) {
					// truncated KANJI
					return (1);
				}

				sjis_str += 2;
				sjis_len -= 1;
			}
			else {
				//It is a KANA
				sjis_str++;
			}
		}
		else {
			// it is a ASCII
			sjis_str++;
		}
	}
	return (0);
}



USHORT KANJI_euc2sjis(const UCHAR* euc_str,
					  USHORT euc_len,
					  UCHAR* sjis_str,
					  USHORT sjis_buf_len, USHORT* sjis_len)
{
/**************************************
 *
 *      K A N J I _ e u c 2 s j i s
 *
 **************************************
 *
 * Functional description
 *      Convert euc_len number of bytes in euc_str to sjis_str .
 *	sjis_buf_len is the maximum size of the sjis buffer.
 *      sjis_len is set to the number of bytes put in the sjis_str.
 * 	If a kanji conversion error occurs a 1 is returned.
 *
 **************************************/
	*sjis_len = 0;
	while (euc_len)
	{
		if (*euc_str & 0x80)
		{
			// Non-Ascii - High bit set
			if (*sjis_len >= sjis_buf_len)	// buffer full
				return (1);

			UCHAR c1 = *euc_str++;
			euc_len--;

			if (EUC1(c1))
			{
				// It is a EUC
				if (euc_len == 0)
					return (1);	// truncated EUC
				UCHAR c2 = *euc_str++;
				euc_len--;
				if (!(EUC2(c2)))
					return (1);	// Bad EUC
				if (c1 == 0x8e)
				{
					// Kana
					*sjis_len += 1;
					*sjis_str++ = c2;
				}
				else
				{
					// Kanji
					*sjis_len += 2;
					if (*sjis_len > sjis_buf_len)	// buffer full
						return (1);
					c1 ^= 0x80;
					c2 ^= 0x80;
					*sjis_str++ = (c1 - 0x21) / 2 + ((c1 <= 0x5e) ? 0x81 : 0xc1);
					if (c1 & 1)	// odd
						*sjis_str++ = c2 + ((c2 <= 0x5f) ? 0x1f : 0x20);
					else
						*sjis_str++ = c2 + 0x7e;
				}
			}
			else				// It is some bad character
				return (1);
		}
		else
		{
			// ASCII
			euc_len--;
			*sjis_len += 1;
			*sjis_str++ = *euc_str++;
		}
	}
	return (0);
}



USHORT KANJI_euc_byte2short(const UCHAR* src, USHORT* dst, USHORT len)
{
/**************************************
 *
 *      K A N J I _ e u c _ b y t e 2 s h o r t
 *
 **************************************
 *
 * Functional description
 *      Convert len number of bytes of EUC string in
 *	src (SCHAR-based buffer) into dst (short-based buffer).
 *	This routine merges:
 *		1-byte ASCII into 1 sshort, and
 *		2-byte EUC kanji into 1 sshort.
 *	Return the number of "characters" in dst.
 *
 **************************************/
	USHORT l;
	for (l = 0; len-- > 0; l++)
	{
		USHORT x = (EUC1(*src)) ? (len--, (*src++ << 8)) : 0;
		x |= *src++;
		*dst++ = x;
	}
	return l;
}



USHORT KANJI_euc_len(const UCHAR* sjis_str, USHORT sjis_len, USHORT* euc_len)
{
/**************************************
 *
 *      K A N J I _ e u c _ l e n
 *
 **************************************
 *
 * Functional description
 *      Return the number of euc bytes corresponding to a given sjis string.
 *	Returns 1 if invalid kanji is encountered.
 *
 **************************************/
	*euc_len = 0;
	while (sjis_len)
	{
		if (*sjis_str & 0x80)
		{
			// Non-Ascii - High bit set
			const UCHAR c1 = *sjis_str++;
			sjis_len--;

			if (SJIS1(c1))
			{
				// First byte is a KANJI
				if (sjis_len == 0)
					return (1);	// truncated KANJI
				const UCHAR c2 = *sjis_str++;
				sjis_len--;
				if (!(SJIS2(c2)))
					return (1);	// Bad second byte
				*euc_len += 2;	// Good Kanji
			}
			else if (SJIS_SINGLE(c1))
				*euc_len += 2;	// Kana
			else
				return (1);		// It is some bad character
		}
		else
		{
			// it is a ASCII

			sjis_len--;
			*euc_len += 1;
			sjis_str++;
		}
	}
	return (0);
}



USHORT KANJI_sjis2euc(const UCHAR* sjis_str,
					  USHORT sjis_len,
					  UCHAR* euc_str, USHORT euc_buf_len, USHORT* euc_len)
{
/**************************************
 *
 *      K A N J I _ s j i s 2 e u c
 *
 **************************************
 *
 * Functional description
 *      Convert sjis_len number of bytes in sjis_str to euc_str .
 *      euc_len is set to the number of bytes put in the euc_str.
 * 	If a kanji conversion error occurs a 1 is returned.
 *
 **************************************/
	*euc_len = 0;
	while (sjis_len)
	{
		if (*euc_len >= euc_buf_len)	// buffer full
			return (1);

		if (*sjis_str & 0x80)
		{
			// Non-Ascii - High bit set
			const UCHAR c1 = *sjis_str++;
			sjis_len--;

			if (SJIS1(c1))
			{
				// First byte is a KANJI
				if (sjis_len == 0)
					return (1);	// truncated KANJI
				const UCHAR c2 = *sjis_str++;
				sjis_len--;
				if (!(SJIS2(c2)))
					return (1);	// Bad second byte
				*euc_len += 2;	// Good Kanji
				if (*euc_len > euc_buf_len)	// buffer full
					return (1);
				S2E(c1, c2, *euc_str, *(euc_str + 1));
				euc_str += 2;
			}
			else if (SJIS_SINGLE(c1))
			{
				*euc_len += 2;	// Kana
				if (*euc_len > euc_buf_len)	// buffer full
					return (1);
				*euc_str++ = 0x8e;
				*euc_str++ = c1;
			}
			else
				return (1);		// It is some bad character
		}
		else
		{
			// it is a ASCII
			*euc_len += 1;
			sjis_len--;
			*euc_str++ = *sjis_str++;
		}
	}
	return (0);
}



USHORT KANJI_sjis_byte2short(const UCHAR* src,
							 USHORT* dst, USHORT len)
{
/**************************************
 *
 *      K A N J I _ s j i s _ b y t e 2 s h o r t
 *
 **************************************
 *
 * Functional description
 *      Convert len number of bytes of SJIS string in
 *	src (SCHAR-based buffer) into dst (short-based buffer).
 *	This routine merges:
 *		1-byte ASCII into 1 sshort,
 *		1-byte SJIS kana 1 sshort, and
 *		2-byte SJIS kanji into 1 sshort.
 *	Return the number of "characters" in dst.
 *
 **************************************/
	USHORT l;
	for (l = 0; len-- > 0; l++) {
		USHORT x = (SJIS1(*src)) ? (len--, (*src++ << 8)) : 0;
		x |= *src++;
		*dst++ = x;
	}
	return l;
}



USHORT KANJI_sjis2euc5(const UCHAR* sjis_str,
					   USHORT sjis_len,
					   UCHAR* euc_str,
					   USHORT euc_buf_len,
					   USHORT* euc_len, USHORT* ib_sjis, USHORT* ib_euc)
{
/**************************************
 *
 *      K A N J I _ s j i s 2 e u c 5
 *
 **************************************
 *
 * Functional description
 *	Similar to KANJI_sjis2euc().
 *	Differences:
 *	(1) when buffer is full, returns (1).
 *	(2) when there's invalid SCHAR, returns(2).
 *	(3) two additional parameters are:
 *		ib_sjis		number of in-bound sjis character
 *		ib_euc		number of in-bound euc character
 * This function is designed to convert blob segment from SJIS to EUC
 * on retrieval. The reason we have ib_sjis and ib_euc is to keep
 * track of splitting fragments from segments.
 *
 **************************************/
	*euc_len = 0;
	*ib_sjis = *ib_euc = 0;
	while (sjis_len)
	{
		if (*euc_len >= euc_buf_len)	// buffer full
			return (1);

		if (*sjis_str & 0x80)
		{
			// Non-Ascii - High bit set
			const UCHAR c1 = *sjis_str++;
			sjis_len--;

			if (SJIS1(c1))
			{
				// First byte is a KANJI
				if (sjis_len == 0)
					return (2);	// truncated KANJI
				const UCHAR c2 = *sjis_str++;
				sjis_len--;
				if (!(SJIS2(c2)))
					return (2);	// Bad second byte
				*euc_len += 2;	// Good Kanji
				if (*euc_len > euc_buf_len)	// buffer full
					return (1);
				S2E(c1, c2, *euc_str, *(euc_str + 1));
				euc_str += 2;
				*ib_sjis += 2;
				*ib_euc += 2;
			}
			else if (SJIS_SINGLE(c1))
			{
				*euc_len += 2;	// Kana
				if (*euc_len > euc_buf_len)	// buffer full
					return (1);
				*euc_str++ = 0x8e;
				*euc_str++ = c1;
				(*ib_sjis)++;
				*ib_euc += 2;
			}
			else
				return (2);		// It is some bad character
		}
		else
		{
			// it is a ASCII
			*euc_len += 1;
			sjis_len--;
			*euc_str++ = *sjis_str++;
			(*ib_sjis)++;
			(*ib_euc)++;
		}
	}
	return (0);
}



USHORT KANJI_sjis_len(const UCHAR* euc_str, USHORT euc_len, USHORT* sjis_len)
{
/**************************************
 *
 *      K A N J I _ s j i s _ l e n
 *
 **************************************
 *
 * Functional description
 *      Find the number of sjis bytes corresponding to a given euc string.
 *	Returns 1 if invalid kanji is encountered.
 *
 **************************************/
	*sjis_len = 0;
	while (euc_len)
	{
		if (*euc_str & 0x80)
		{
			// Non-Ascii - High bit set
			const UCHAR c1 = *euc_str++;
			euc_len--;

			if (EUC1(c1))
			{
				// It is a EUC
				if (euc_len == 0)
					return (1);	// truncated EUC
				const UCHAR c2 = *euc_str++;
				euc_len--;
				if (!(EUC2(c2)))
					return (1);	// Bad EUC
				if (c1 == 0x8e)
					*sjis_len += 1;	// Kana
				else
					*sjis_len += 2;	// Kanji
			}
			else				// It is some bad character
				return (1);
		}
		else
		{
			// ASCII
			euc_len--;
			*sjis_len += 1;
			euc_str++;
		}
	}
	return (0);
}

