/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_big5.cpp
 *	DESCRIPTION:	Codeset conversion for BIG5 family codesets
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
#include "../intl/cv_big5.h"
#include "../intl/cv_narrow.h"
#include "ld_proto.h"

ULONG CVBIG5_big5_to_unicode(csconvert* obj,
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
	fb_assert(obj->csconvert_fn_convert == CVBIG5_big5_to_unicode);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = src_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (p_dest_ptr == NULL)
		return (src_len * sizeof(USHORT));

	Firebird::OutAligner<USHORT> d(p_dest_ptr, dest_len);
	USHORT* dest_ptr = d;

	USHORT wide;
	USHORT this_len;
	const USHORT* const start = dest_ptr;
	while ((src_len) && (dest_len > 1))
	{
		if (*src_ptr & 0x80)
		{
			const UCHAR c1 = *src_ptr++;

			if (BIG51(c1))
			{	// first byte is Big5
				if (src_len == 1) {
					*err_code = CS_BAD_INPUT;
					break;
				}
				const UCHAR c2 = *src_ptr++;
				if (!(BIG52(c2))) {	// Bad second byte
					*err_code = CS_BAD_INPUT;
					break;
				}
				wide = (c1 << 8) + c2;
				this_len = 2;
			}
			else
			{
				*err_code = CS_BAD_INPUT;
				break;
			}
		}
		else
		{					// it is ASCII

			wide = *src_ptr++;
			this_len = 1;
		}

		// Convert from BIG5 to UNICODE
		const USHORT ch = ((const USHORT*) impl->csconvert_datatable)
			[((const USHORT*) impl->csconvert_misc)[(USHORT) wide / 256] + (wide % 256)];

		if ((ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}

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


ULONG CVBIG5_unicode_to_big5(csconvert* obj,
							 ULONG unicode_len,
							 const UCHAR* p_unicode_str,
							 ULONG big5_len,
							 UCHAR* big5_str,
							 USHORT* err_code,
							 ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(p_unicode_str != NULL || big5_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVBIG5_unicode_to_big5);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = unicode_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (big5_str == NULL)
		return unicode_len;	// worst case - all han character input

	Firebird::Aligner<USHORT> s(p_unicode_str, unicode_len);
	const USHORT* unicode_str = s;

	const UCHAR* const start = big5_str;
	while ((big5_len) && (unicode_len > 1))
	{
		// Convert from UNICODE to BIG5 code
		const USHORT wide = *unicode_str++;

		const USHORT big5_ch = ((const USHORT*) impl->csconvert_datatable)
			[((const USHORT*) impl->csconvert_misc)[(USHORT)wide / 256] + (wide % 256)];
		if ((big5_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}

		// int ???
		const int tmp1 = big5_ch / 256;
		const int tmp2 = big5_ch % 256;
		if (tmp1 == 0)
		{		// ASCII character

			fb_assert((UCHAR(tmp2) & 0x80) == 0);

			*big5_str++ = tmp2;
			big5_len--;
			unicode_len -= sizeof(*unicode_str);
			continue;
		}
		if (big5_len < 2)
		{
			*err_code = CS_TRUNCATION_ERROR;
			break;
		}
		else
		{
			fb_assert(BIG51(tmp1));
			fb_assert(BIG52(tmp2));
			*big5_str++ = tmp1;
			*big5_str++ = tmp2;
			unicode_len -= sizeof(*unicode_str);
			big5_len -= 2;
		}
	}
	if (unicode_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - unicode_len;
	return ((big5_str - start) * sizeof(*big5_str));
}


INTL_BOOL CVBIG5_check_big5(charset*, // cs,
							ULONG big5_len,
							const UCHAR* big5_str,
							ULONG* offending_position)
{
/**************************************
 * Functional description
 *      Make sure that the big5 string does not have any truncated 2 byte
 *      character at the end.
 * If we have a truncated character then,
 *          return false.
 *          else return(true);
 **************************************/
	const UCHAR* big5_str_start = big5_str;

	while (big5_len--)
	{
		const UCHAR c1 = *big5_str;

		if (BIG51(c1))	// Is it  BIG-5
		{
			if (big5_len == 0)	// truncated BIG-5
			{
				if (offending_position)
					*offending_position = big5_str - big5_str_start;
				return (false);
			}

			big5_str += 2;
			big5_len -= 1;
		}
		else	// it is a ASCII
			big5_str++;
	}
	return (true);
}
