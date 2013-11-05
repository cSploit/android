/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_narrow.h
 *	DESCRIPTION:	Codeset conversion for narrow character sets.
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

void CV_convert_init(csconvert* csptr,
		pfn_INTL_convert cvt_fn, const void *datatable, const void *datatable2);

ULONG CV_wc_to_wc(csconvert* obj, ULONG src_len, const UCHAR* src_ptr,
				  ULONG dest_len, UCHAR* dest_ptr,
				  USHORT* err_code, ULONG* err_position);

ULONG CV_unicode_to_nc(csconvert* obj, ULONG src_len, const BYTE* src_ptr,
					   ULONG dest_len, BYTE* dest_ptr,
					   USHORT* err_code, ULONG* err_position);

ULONG CV_nc_to_unicode(csconvert* obj, ULONG src_len, const BYTE* src_ptr,
					   ULONG dest_len, BYTE* dest_ptr,
					   USHORT* err_code, ULONG* err_position);

ULONG CV_wc_copy(csconvert* obj, ULONG src_len, const BYTE* src_ptr,
				 ULONG dest_len, BYTE* dest_ptr,
				 USHORT* err_code, ULONG* err_position);

ULONG eight_bit_convert(csconvert* obj, ULONG src_len, const BYTE* src_ptr,
						ULONG dest_len, BYTE* dest_ptr,
						USHORT* err_code, ULONG* err_position);

