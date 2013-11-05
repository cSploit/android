/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_gb2312.cpp
 *	DESCRIPTION:	Codeset conversion for GB2312 family codesets
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
#include "../intl/cv_gb2312.h"
#include "../intl/cv_narrow.h"
#include "ld_proto.h"

#define	GB1(uc)	((UCHAR)((uc) & 0xff) >= 0xa1 && \
			 (UCHAR)((uc) & 0xff) <= 0xfe)	// GB2312 1st-byte
#define	GB2(uc)	((UCHAR)((uc) & 0xff) >= 0xa1 && \
			 (UCHAR)((uc) & 0xff) <= 0xfe)	// GB2312 2nd-byte


ULONG CVGB_gb2312_to_unicode(csconvert* obj,
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
	fb_assert(obj->csconvert_fn_convert == CVGB_gb2312_to_unicode);
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
	while (src_len && dest_len > 1)
	{
		if (*src_ptr & 0x80)
		{
			const UCHAR c1 = *src_ptr++;

			if (GB1(c1))
			{		// first byte is GB2312
				if (src_len == 1) {
					*err_code = CS_BAD_INPUT;
					break;
				}
				const UCHAR c2 = *src_ptr++;
				if (!(GB2(c2))) {	// Bad second byte
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

		// Convert from GB2312 to UNICODE
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


ULONG CVGB_unicode_to_gb2312(csconvert* obj,
							 ULONG unicode_len,
							 const UCHAR* p_unicode_str,
							 ULONG gb_len,
							 UCHAR* gb_str,
							 USHORT* err_code,
							 ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(p_unicode_str != NULL || gb_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVGB_unicode_to_gb2312);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = unicode_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (gb_str == NULL)
		return unicode_len;	// worst case - all han character input

	Firebird::Aligner<USHORT> s(p_unicode_str, unicode_len);
	const USHORT* unicode_str = s;

	const UCHAR* const start = gb_str;
	while (gb_len && unicode_len > 1)
	{
		// Convert from UNICODE to GB2312 code
		const USHORT wide = *unicode_str++;

		const USHORT gb_ch = ((const USHORT*) impl->csconvert_datatable)
			[((const USHORT*) impl->csconvert_misc)[(USHORT)wide / 256] + (wide % 256)];
		if ((gb_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}

		const int tmp1 = gb_ch / 256;
		const int tmp2 = gb_ch % 256;
		if (tmp1 == 0)
		{		// ASCII character

			fb_assert((UCHAR(tmp2) & 0x80) == 0);

			*gb_str++ = tmp2;
			gb_len--;
			unicode_len -= sizeof(*unicode_str);
			continue;
		}
		if (gb_len < 2)
		{
			*err_code = CS_TRUNCATION_ERROR;
			break;
		}
		else
		{
			fb_assert(GB1(tmp1));
			fb_assert(GB2(tmp2));
			*gb_str++ = tmp1;
			*gb_str++ = tmp2;
			unicode_len -= sizeof(*unicode_str);
			gb_len -= 2;
		}
	}
	if (unicode_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - unicode_len;
	return ((gb_str - start) * sizeof(*gb_str));
}


INTL_BOOL CVGB_check_gb2312(charset* /*cs*/, ULONG gb_len, const UCHAR *gb_str, ULONG* offending_position)
{
/**************************************
 * Functional description
 *      Make sure that the GB2312 string does not have any truncated 2 byte
 *      character at the end.
 * If we have a truncated character then,
 *          return false.
 *          else return(true);
 **************************************/
	const UCHAR* gb_str_start = gb_str;

	while (gb_len--)
	{
		const UCHAR c1 = *gb_str;

		if (c1 & 0x80)	// it is not an ASCII char
		{
			if (GB1(c1))	// first byte is GB2312
			{
				if (gb_len == 0 ||		// truncated GB2312
					!GB2(gb_str[1]))	// bad second byte
				{
					if (offending_position)
						*offending_position = gb_str - gb_str_start;

					return false;
				}

				gb_str += 2;
				gb_len -= 1;
			}
			else	// bad first byte
			{
				if (offending_position)
					*offending_position = gb_str - gb_str_start;

				return false;
			}
		}
		else	// it is an ASCII char
			gb_str++;
	}

	return true;
}
