/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_ksc.cpp
 *	DESCRIPTION:	Codeset conversion for KSC-5601 codesets
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
#include "cv_narrow.h"
#include "cv_ksc.h"
#include "ld_proto.h"

// KSC-5601 -> unicode
// % KSC-5601 is same to EUC cs1(codeset 1). Then converting
// KSC-5601 to EUC is not needed.


ULONG CVKSC_ksc_to_unicode(csconvert* obj,
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
	fb_assert(obj->csconvert_fn_convert == CVKSC_ksc_to_unicode);
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
	while (src_len && (dest_len > 1))
	{
		if (*src_ptr & 0x80)
		{
			const UCHAR c1 = *src_ptr++;

			if (KSC1(c1))
			{	// first byte is KSC
				if (src_len == 1) {
					*err_code = CS_BAD_INPUT;
					break;
				}
				const UCHAR c2 = *src_ptr++;
				if (!(KSC2(c2))) {	// Bad second byte
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

		// Convert from KSC to UNICODE
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


ULONG CVKSC_unicode_to_ksc(csconvert* obj,
						   ULONG unicode_len,
						   const UCHAR* p_unicode_str,
						   ULONG ksc_len,
						   UCHAR* ksc_str,
						   USHORT* err_code,
						   ULONG* err_position)
{
	fb_assert(obj != NULL);

	CsConvertImpl* impl = obj->csconvert_impl;

	fb_assert(p_unicode_str != NULL || ksc_str == NULL);
	fb_assert(err_code != NULL);
	fb_assert(err_position != NULL);
	fb_assert(obj->csconvert_fn_convert == CVKSC_unicode_to_ksc);
	fb_assert(impl->csconvert_datatable != NULL);
	fb_assert(impl->csconvert_misc != NULL);

	const ULONG src_start = unicode_len;
	*err_code = 0;

	// See if we're only after a length estimate
	if (ksc_str == NULL)
		return (unicode_len);

	Firebird::Aligner<USHORT> s(p_unicode_str, unicode_len);
	const USHORT* unicode_str = s;

	const UCHAR* const start = ksc_str;
	while (ksc_len && unicode_len > 1)
	{
		const USHORT wide = *unicode_str++;

		const USHORT ksc_ch = ((const USHORT*) impl->csconvert_datatable)
			[((const USHORT*) impl->csconvert_misc)[(USHORT) wide / 256] + (wide % 256)];
		if ((ksc_ch == CS_CANT_MAP) && !(wide == CS_CANT_MAP)) {
			*err_code = CS_CONVERT_ERROR;
			break;
		}

		const int tmp1 = ksc_ch / 256;
		const int tmp2 = ksc_ch % 256;
		if (tmp1 == 0)
		{		// ASCII character

			fb_assert((UCHAR(tmp2) & 0x80) == 0);

			*ksc_str++ = tmp2;
			ksc_len--;
			unicode_len -= sizeof(*unicode_str);
			continue;
		}
		if (ksc_len < 2)
		{
			*err_code = CS_TRUNCATION_ERROR;
			break;
		}
		else
		{
			fb_assert(KSC1(tmp1));
			fb_assert(KSC2(tmp2));
			*ksc_str++ = tmp1;
			*ksc_str++ = tmp2;
			unicode_len -= sizeof(*unicode_str);
			ksc_len -= 2;
		}
	}							// end-while
	if (unicode_len && !*err_code) {
		*err_code = CS_TRUNCATION_ERROR;
	}
	*err_position = src_start - unicode_len;
	return ((ksc_str - start) * sizeof(*ksc_str));
}


INTL_BOOL CVKSC_check_ksc(charset*, // cs,
						  ULONG ksc_len,
						  const UCHAR* ksc_str,
						  ULONG* offending_position)
{
	const UCHAR* ksc_str_start = ksc_str;

	while (ksc_len--)
	{
		const UCHAR c1 = *ksc_str;
		if (KSC1(c1))
		{			// Is it KSC-5601 ?
			if (ksc_len == 0)	// truncated KSC
			{
				if (offending_position)
					*offending_position = ksc_str - ksc_str_start;
				return (false);
			}

			ksc_str += 2;
			ksc_len -= 1;
		}
		else if (c1 > 0x7f)		// error
		{
			if (offending_position)
				*offending_position = ksc_str - ksc_str_start;
			return false;
		}
		else					// ASCII
			ksc_str++;
	}
	return (true);
}
