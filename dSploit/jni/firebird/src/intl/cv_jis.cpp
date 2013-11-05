/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_jis.cpp
 *	DESCRIPTION:	Codeset conversion for JIS family codesets
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
#include "../intl/charsets/cs_sjis.h"
#include "../intl/kanji.h"
#include "cv_jis.h"
#include "cv_narrow.h"
#include "ld_proto.h"

ULONG CVJIS_eucj_to_unicode(csconvert* obj,
							ULONG src_len,
							const UCHAR* src_ptr,
							ULONG dest_len,
							UCHAR* p_dest_ptr,
							USHORT* err_code,
							ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(src_ptr != NULL || p_dest_ptr == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVJIS_eucj_to_unicode);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = src_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (p_dest_ptr == NULL)
		return sizeof(USHORT) * src_len;

	Firebird::OutAligner<USHORT> d(p_dest_ptr, dest_len);
	USHORT* dest_ptr = d;

	USHORT ch;
	USHORT wide;
	USHORT this_len;
	const USHORT* const start = dest_ptr;
	while ((src_len) && (dest_len > 1))
	{
		const UCHAR ch1 = *src_ptr++;

		// Step 1: Convert from EUC to JIS
		if (!(ch1 & 0x80))
		{	// 1 byte SCHAR
			// Plane 0 of EUC-J is defined as ASCII
			wide = ch1;
			this_len = 1;

			// Step 2: Convert from ASCII to UNICODE
			ch = ch1;
		}
		else if (src_len == 1 || !(*src_ptr & 0x80))
		{
			// We received a multi-byte indicator, but either
			// there isn't a 2nd byte or the 2nd byte isn't marked
			*err_code = CS_BAD_INPUT;
			break;
		}
		else
		{
			wide = ((ch1 << 8) + (*src_ptr++)) & ~0x8080;
			this_len = 2;

			// Step 2: Convert from JIS to UNICODE
			ch = ((const USHORT*) impl->csconvert_datatable)
				[((const USHORT*) impl->csconvert_misc)
					[(USHORT)wide /	256] + (wide % 256)];
		}


		// No need to check for CS_CONVERT_ERROR -
		// EUCJ must convert to Unicode

		*dest_ptr++ = ch;
		dest_len -= sizeof(*dest_ptr);
		src_len -= this_len;
	}
	if (src_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - src_len;
	return ((dest_ptr - start) * sizeof(*dest_ptr));
}


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


ULONG CVJIS_sjis_to_unicode(csconvert* obj,
							ULONG sjis_len,
							const UCHAR* sjis_str,
							ULONG dest_len,
							UCHAR* p_dest_ptr,
							USHORT* err_code,
							ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(sjis_str != NULL || p_dest_ptr == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVJIS_sjis_to_unicode);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = sjis_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (p_dest_ptr == NULL)
		return sjis_len * 2;	// worst case - all ascii input

	Firebird::OutAligner<USHORT> d(p_dest_ptr, dest_len);
	USHORT* dest_ptr = d;

	USHORT table;
	USHORT this_len;
	USHORT wide;
	const USHORT* const start = dest_ptr;
	while (sjis_len && dest_len > 1)
	{
		// Step 1: Convert from SJIS to JIS code
		if (*sjis_str & 0x80)
		{	// Non-Ascii - High bit set
			const UCHAR c1 = *sjis_str++;

			if (SJIS1(c1))
			{	// First byte is a KANJI
				if (sjis_len == 1) {	// truncated KANJI
					*err_code = CS_BAD_INPUT;
					break;
				}
				const UCHAR c2 = *sjis_str++;
				if (!(SJIS2(c2))) {	// Bad second byte
					*err_code = CS_BAD_INPUT;
					break;
				}
				// Step 1b: Convert 2 byte SJIS to EUC-J
				UCHAR tmp1, tmp2;
				S2E(c1, c2, tmp1, tmp2);

				// Step 2b: Convert 2 byte EUC-J to JIS
				wide = ((tmp1 << 8) + tmp2) & ~0x8080;
				this_len = 2;
				table = 1;
			}
			else if (SJIS_SINGLE(c1))
			{
				wide = c1;
				this_len = 1;
				table = 2;
			}
			else
			{				// It is some bad character

				*err_code = CS_BAD_INPUT;
				break;
			}
		}
		else
		{					// it is a ASCII

			wide = *sjis_str++;
			this_len = 1;
			table = 2;
		}

		// Step 2: Convert from JIS code (in wide) to UNICODE
		USHORT ch;
		if (table == 1)
		{
			ch = ((const USHORT*) impl->csconvert_datatable)
				[((const USHORT*) impl->csconvert_misc)
					[(USHORT)wide /	256] + (wide % 256)];
		}
		else
		{
			fb_assert(table == 2);
			fb_assert(wide <= 255);
			ch = sjis_to_unicode_mapping_array
				[sjis_to_unicode_map[(USHORT) wide / 256] + (wide % 256)];
		}

		// This is only important for bad-SJIS in input stream
		if ((ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}
		*dest_ptr++ = ch;
		dest_len -= sizeof(*dest_ptr);
		sjis_len -= this_len;
	}
	if (sjis_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - sjis_len;
	return ((dest_ptr - start) * sizeof(*dest_ptr));
}


/*
From tate.a-t.com!uunet!Eng.Sun.COM!adobe!adobe.com!lunde Tue Nov 19 16:11:24 1991
Return-Path: <uunet!Eng.Sun.COM!adobe!adobe.com!lunde@tate.a-t.com>
Received: by dbase.a-t.com (/\==/\ Smail3.1.21.1 #21.5)
	id <m0kjfXI-0004qKC@dbase.a-t.com>; Tue, 19 Nov 91 16:11 PST
Received: by tate.a-t.com (/\==/\ Smail3.1.21.1 #21.1)
	id <m0kjfPU-000Gf0C@tate.a-t.com>; Tue, 19 Nov 91 16:03 PST
Received: from Sun.COM by relay1.UU.NET with SMTP
	(5.61/UUNET-internet-primary) id AA21144; Tue, 19 Nov 91 18:45:19 -0500
Received: from Eng.Sun.COM (zigzag-bb.Corp.Sun.COM) by Sun.COM (4.1/SMI-4.1)
	id AA04289; Tue, 19 Nov 91 15:40:59 PST
Received: from cairo.Eng.Sun.COM by Eng.Sun.COM (4.1/SMI-4.1)
	id AA12852; Tue, 19 Nov 91 15:39:48 PST
Received: from snail.Sun.COM by cairo.Eng.Sun.COM (4.1/SMI-4.1)
	id AA23752; Tue, 19 Nov 91 15:36:04 PST
Received: from sun.Eng.Sun.COM by snail.Sun.COM (4.1/SMI-4.1)
	id AA18716; Tue, 19 Nov 91 15:40:34 PST
Received: from adobe.UUCP by sun.Eng.Sun.COM (4.1/SMI-4.1)
	id AA25005; Tue, 19 Nov 91 15:39:21 PST
Received: from lynton.adobe.com by adobe.com (4.0/SMI-4.0)
	id AA22639; Tue, 19 Nov 91 15:39:57 PST
Received: by lynton.adobe.com (4.0/SMI-4.0)
	id AA08147; Tue, 19 Nov 91 15:39:45 PST
Date: Tue, 19 Nov 91 15:39:45 PST
From: tate!uunet!adobe.com!lunde (Ken Lunde)
Message-Id: <9111192339.AA08147@lynton.adobe.com>
To: unicode@Sun.COM
Subject: JIS -> Shift-JIS algorithm...
Status: OR

Hi!

     This posting is in response to Asmus' algorithm for converting
JIS to Shift-JIS.


     This is an explanation of the JIS->Shift-JIS conversion algorithm.
I will first show my function, and then follow-up with a step-by-step
description.
     Note that all values listed are ASCII decimal. Here is the C coded
function:

C O D E:
*/

#define	isodd(x)	((x) & 1)

static void seven2eight(USHORT *p1, USHORT *p2)
{
	if (isodd(*p1))
		*p2 += 31;
	else
		*p2 += 126;

	if ((*p2 >= 127) && (*p2 < 158))
		(*p2)++;

	if ((*p1 >= 33) && (*p1 <= 94))
	{
		if (isodd(*p1))
			*p1 = ((*p1 - 1) / 2) + 113;
		else
			*p1 = (*p1 / 2) + 112;
	}
	else if ((*p1 >= 95) && (*p1 <= 126))
	{
		if (isodd(*p1))
			*p1 = ((*p1 - 1) / 2) + 177;
		else
			*p1 = (*p1 / 2) + 176;
	}
}

/*

E X P L A N A T I O N:

ORIGINAL CODE VALUES:
JIS 1st:   76
JIS 2nd:   36

SJIS 1st: 150
SJIS 2nd: 162

STEPS:
1) If the 1st JIS byte is odd, then add 31 to the 2nd byte. Otherwise,
   add 126 to the 2nd byte.

   EX: JIS 1st = 76 (is NOT odd), so JIS 2nd = 162 (36 + 126)

   JIS 1st:   76
   JIS 2nd:  162
   SJIS 1st: 150
   SJIS 2nd: 162

   Note that the second byte has been converted to the target code
   already.

2) If the 2nd JIS byte is greater than or equal to 127 AND less than
   158, then add 1 to the 2nd byte.

   EX: JIS 2nd = 162 (not in the range), so nothing is done.

3) If the 1st JIS byte is greater than or equal to 33 AND less than
   or equal to 94, then do (a). Otherwise, if the 1st JIS byte
   is greater than or equal to 95 AND less than or equal to 126, then
   do step (b).

   a) If the 1st JIS byte is odd, then subtract 1 from its value, divide
      the result by 2, and finally add 113. Otherwise, divide the 1st JIS
      byte by 2 and add 112.

   b) If the 1st JIS byte is odd, then subtract 1 from its value, divide
      the result by 2, and finally add 177. Otherwise, divide the 1st JIS
      byte by 2 and add 176.

EX: JIS 1st is in the range 33-94, so we execute step 3(a). JIS 1st = 76
    (is NOT odd), so JIS 2nd = 150 ((76/2) + 112)

JIS 1st:  150
JIS 2nd:  162

SJIS 1st: 150
SJIS 2nd: 162


Here are the specs for JIS:

                 DECIMAL            OCTAL              HEXADECIMAL
1st Byte         033-126            041-176            21-7E
2nd Byte         033-126            041-176            21-7E

Here are the specs for Shift-JIS:

                 DECIMAL            OCTAL              HEXADECIMAL
TYPE 1:
   1st Byte      129-159            201-237            81-9F
   2nd Byte      064-158 (not 127)  100-236 (not 177)  40-9E (not 7F)
TYPE 2:
   1st Byte      224-239            340-357            E0-EF
   2nd Byte      064-158 (not 127)  100-236 (not 177)  40-9E (not 7F)
TYPE 3:
   1st Byte      129-159            201-237            81-9F
   2nd Byte      159-252            237-374            9F-FC
TYPE 4:
   1st Byte      224-239            340-357            E0-EF
   2nd Byte      159-252            237-374            9F-FC
Half-size katakana:
   1st Byte      161-223            241-337            A1-DF

I hope this helps in the discussion.

-- Ken R. Lunde
   Adobe Systems Incorporated
   Japanese Font Production
   lunde@adobe.com
*/


ULONG CVJIS_unicode_to_sjis(csconvert* obj,
							ULONG unicode_len,
							const UCHAR* p_unicode_str,
							ULONG sjis_len,
							UCHAR* sjis_str,
							USHORT* err_code,
							ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(p_unicode_str != NULL || sjis_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVJIS_unicode_to_sjis);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = unicode_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (sjis_str == NULL)
		return unicode_len;	// worst case - all han character input

	Firebird::Aligner<USHORT> s(p_unicode_str, unicode_len);
	const USHORT* unicode_str = s;

	const UCHAR* const start = sjis_str;
	while ((sjis_len) && (unicode_len > 1))
	{
		// Step 1: Convert from UNICODE to JIS code
		const USHORT wide = *unicode_str++;

		USHORT jis_ch = ((const USHORT*) impl->csconvert_datatable)
			[((const USHORT*) impl->csconvert_misc)[(USHORT)wide / 256] + (wide % 256)];

		if ((jis_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP))
		{

			// Handle the non-JIS codes in SJIS (ASCII & half-width Kana)
			jis_ch = sjis_from_unicode_mapping_array
					[sjis_from_unicode_map[(USHORT) wide / 256] + (wide % 256)];
			if ((jis_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
				*err_code = CS_CONVERT_ERROR;
				break;
			}
		}

		// Step 2: Convert from JIS code to SJIS
		USHORT tmp1 = jis_ch / 256;
		USHORT tmp2 = jis_ch % 256;
		if (tmp1 == 0)
		{		// ASCII character
			*sjis_str++ = tmp2;
			sjis_len--;
			unicode_len -= sizeof(*unicode_str);
			continue;
		}
		seven2eight(&tmp1, &tmp2);
		if (tmp1 == 0)
		{		// half-width kana ?
			fb_assert(SJIS_SINGLE(tmp2));
			*sjis_str++ = tmp2;
			unicode_len -= sizeof(*unicode_str);
			sjis_len--;
		}
		else if (sjis_len < 2)
		{
			*err_code = CS_TRUNCATION_ERROR;
			break;
		}
		else
		{
			fb_assert(SJIS1(tmp1));
			fb_assert(SJIS2(tmp2));
			*sjis_str++ = tmp1;
			*sjis_str++ = tmp2;
			unicode_len -= sizeof(*unicode_str);
			sjis_len -= 2;
		}
	}
	if (unicode_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - unicode_len;
	return ((sjis_str - start) * sizeof(*sjis_str));
}


ULONG CVJIS_unicode_to_eucj(csconvert* obj, ULONG unicode_len, const UCHAR* p_unicode_str,
							ULONG eucj_len, UCHAR* eucj_str,
							USHORT* err_code, ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(p_unicode_str != NULL || eucj_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVJIS_unicode_to_eucj);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = unicode_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (eucj_str == NULL)
		return (unicode_len);	// worst case - all han character input

	Firebird::Aligner<USHORT> s(p_unicode_str, unicode_len);
	const USHORT* unicode_str = s;

	const UCHAR* const start = eucj_str;
	while (eucj_len && unicode_len > 1)
	{
		// Step 1: Convert from UNICODE to JIS code
		const USHORT wide = *unicode_str++;

		// ASCII range characters map directly -- others go to the table
		USHORT jis_ch;
		if (wide <= 0x007F)
			jis_ch = wide;
		else
			jis_ch = ((const USHORT*) impl->csconvert_datatable)
					[((const USHORT*) impl->csconvert_misc)
						[(USHORT)wide /	256] + (wide % 256)];
		if ((jis_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}

		// Step 2: Convert from JIS code to EUC-J
		const USHORT tmp1 = jis_ch / 256;
		const USHORT tmp2 = jis_ch % 256;
		if (tmp1 == 0)
		{		// ASCII character
			fb_assert(!(tmp2 & 0x80));
			*eucj_str++ = tmp2;
			eucj_len--;
			unicode_len -= sizeof(*unicode_str);
			continue;
		}
		if (eucj_len < 2)
		{
			*err_code = CS_TRUNCATION_ERROR;
			break;
		}
		else
		{
			fb_assert(!(tmp1 & 0x80));
			fb_assert(!(tmp2 & 0x80));
			*eucj_str++ = tmp1 | 0x80;
			*eucj_str++ = tmp2 | 0x80;
			unicode_len -= sizeof(*unicode_str);
			eucj_len -= 2;
		}
	}
	if (unicode_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - unicode_len;
	return ((eucj_str - start) * sizeof(*eucj_str));
}


INTL_BOOL CVJIS_check_euc(charset* /*cs*/, ULONG euc_len, const UCHAR* euc_str, ULONG* offending_position)
{
/**************************************
 *
 *      C V J I S _ c h e c k _ e u c
 *
 **************************************
 *
 * Functional description
 *	This is a cousin of the KANJI_check_sjis routine.
 *      Make sure that the euc string does not have any truncated 2 byte
 *      character at the end. * If we have a truncated character then,
 *          return false.
 *          else return true;
 **************************************/
	const UCHAR* start = euc_str;

	while (euc_len--)
	{
		if (*euc_str & 0x80)	// Is it EUC
		{
			if (euc_len == 0)	// truncated kanji
			{
				*offending_position = euc_str - start;
				return false;
			}

			euc_str += 2;
			euc_len -= 1;
		}
		else	// it is a ASCII
			euc_str++;
	}

	return true;
}


INTL_BOOL CVJIS_check_sjis(charset* /*cs*/, ULONG sjis_len, const UCHAR* sjis_str, ULONG* offending_position)
{
/**************************************
 *
 *      C V J I S _ c h e c k _ s j i s
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
	const UCHAR* start = sjis_str;

	while (sjis_len--)
	{
		if (*sjis_str & 0x80)	// Is it SJIS
		{
			const UCHAR c1 = *sjis_str;
			if (SJIS1(c1))	// It is a KANJI
			{
				if (sjis_len == 0)	// truncated KANJI
				{
					*offending_position = sjis_str - start;
					return false;
				}

				sjis_str += 2;
				sjis_len -= 1;
			}
			else	// It is a KANA
				sjis_str++;
		}
		else	// it is a ASCII
			sjis_str++;
	}

	return true;
}


#ifdef NOT_USED_OR_REPLACED
static USHORT CVJIS_euc2sjis(csconvert* obj, UCHAR *sjis_str, USHORT sjis_len,
							const UCHAR* euc_str,
							 USHORT euc_len, SSHORT *err_code, USHORT *err_position)
{
/**************************************
 *
 *      K A N J I _ e u c 2 s j i s
 *
 **************************************
 *
 * Functional description
 *      Convert euc_len number of bytes in euc_str to sjis_str .
 *	sjis_len is the maximum size of the sjis buffer.
 *
 **************************************/
	fb_assert(euc_str != NULL || sjis_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj != NULL);

	const USHORT src_start = euc_len;
	*err_code = 0;

	// Length estimate needed?
	if (sjis_str == NULL)
		return euc_len;		// worst case

	const UCHAR* const sjis_start = sjis_str;
	while (euc_len && sjis_len)
	{
		if (*euc_str & 0x80)
		{	// Non-Ascii - High bit set

			UCHAR c1 = *euc_str++;

			if (EUC1(c1))
			{		// It is a EUC
				if (euc_len == 1) {
					*err_code = CS_BAD_INPUT;	// truncated EUC
					break;
				}
				UCHAR c2 = *euc_str++;
				if (!(EUC2(c2))) {
					*err_code = CS_BAD_INPUT;	// Bad EUC
					break;
				}
				if (c1 == 0x8e)
				{	// Kana
					sjis_len--;
					*sjis_str++ = c2;
					euc_len -= 2;
				}
				else
				{			// Kanji

					if (sjis_len < 2) {	// buffer full
						*err_code = CS_TRUNCATION_ERROR;
						break;
					}
					sjis_len -= 2;
					euc_len -= 2;
					c1 ^= 0x80;
					c2 ^= 0x80;
					*sjis_str++ = (USHORT) (c1 - 0x21) / 2 + ((c1 <= 0x5e) ? 0x81 : 0xc1);
					if (c1 & 1)	// odd
						*sjis_str++ = c2 + ((c2 <= 0x5f) ? 0x1f : 0x20);
					else
						*sjis_str++ = c2 + 0x7e;
				}
			}
			else
			{				// It is some bad character

				*err_code = CS_BAD_INPUT;
				break;
			}
		}
		else
		{					// ASCII
			euc_len--;
			sjis_len--;
			*sjis_str++ = *euc_str++;
		}
	}
	if (euc_len && !*err_code)
		*err_code = CS_TRUNCATION_ERROR;
	*err_position = src_start - euc_len;
	return (sjis_str - sjis_start);
}


static USHORT CVJIS_sjis2euc(csconvert* obj, UCHAR *euc_str, USHORT euc_len,
							const UCHAR* sjis_str,
							 USHORT sjis_len, SSHORT *err_code, USHORT *err_position)
{
/**************************************
 *
 *      K A N J I _ s j i s 2 e u c
 *
 **************************************
 *
 * Functional description
 *      Convert sjis_len number of bytes in sjis_str to euc_str .
 *
 **************************************/
	fb_assert(sjis_str != NULL || euc_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj != NULL);

	const USHORT src_start = sjis_len;
	*err_code = 0;
	if (euc_str == NULL)
		return 2 * sjis_len;	// worst case

	const UCHAR* const euc_start = euc_str;
	while (sjis_len && euc_len)
	{

		if (*sjis_str & 0x80)
		{	// Non-Ascii - High bit set
			const UCHAR c1 = *sjis_str++;
			if (SJIS1(c1))
			{	// First byte is a KANJI
				if (sjis_len == 1) {	// truncated KANJI
					*err_code = CS_BAD_INPUT;
					break;
				}
				const UCHAR c2 = *sjis_str++;
				if (!(SJIS2(c2))) {	// Bad second byte
					*err_code = CS_BAD_INPUT;
					break;
				}
				if (euc_len < 2) {	// buffer full
					*err_code = CS_TRUNCATION_ERROR;
					break;
				}
				S2E(c1, c2, *euc_str, *(euc_str + 1));
				euc_str += 2;
				euc_len -= 2;
				sjis_len -= 2;
			}
			else if (SJIS_SINGLE(c1))
			{
				if (euc_len < 2) {	// buffer full
					*err_code = CS_TRUNCATION_ERROR;
					break;
				}
				euc_len -= 2;	// Kana
				sjis_len--;
				*euc_str++ = 0x8e;
				*euc_str++ = c1;
			}
			else
			{				// It is some bad character
				*err_code = CS_BAD_INPUT;
				break;
			}
		}
		else
		{					// it is a ASCII
			euc_len--;
			sjis_len--;
			*euc_str++ = *sjis_str++;
		}
	}
	if (sjis_len && !*err_code)
		*err_code = CS_TRUNCATION_ERROR;
	*err_position = src_start - sjis_len;
	return (euc_str - euc_start);
}
#endif


#ifdef NOT_USED_OR_REPLACED
CONVERT_ENTRY(CS_SJIS, CS_EUCJ, CVJIS_sjis_x_eucj)
{
	if (dest_cs == CS_EUCJ)
		CV_convert_init(csptr, dest_cs, source_cs,
						CVJIS_sjis2euc,
						NULL, NULL);
	else
		CV_convert_init(csptr, dest_cs, source_cs,
						CVJIS_euc2sjis,
						NULL, NULL);

	CONVERT_RETURN;
}
#endif
