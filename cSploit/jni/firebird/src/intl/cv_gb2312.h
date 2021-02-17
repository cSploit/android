/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_gb2312.h
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

ULONG CVGB_gb2312_to_unicode(csconvert* obj, ULONG src_len, const UCHAR* src_ptr,
							 ULONG dest_len, UCHAR *dest_ptr,
							 USHORT *err_code, ULONG *err_position);

ULONG CVGB_unicode_to_gb2312(csconvert* obj, ULONG unicode_len, const UCHAR* unicode_str,
							 ULONG gb_len, UCHAR *gb_str,
							 USHORT *err_code, ULONG *err_position);

INTL_BOOL CVGB_check_gb2312(charset* cs, ULONG gb_len, const UCHAR* gb_str, ULONG* offending_position);
