/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_jis.h
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

ULONG CVJIS_sjis_to_unicode(csconvert* obj, ULONG sjis_len, const UCHAR* sjis_str,
							ULONG dest_len, UCHAR *dest_ptr,
							USHORT *err_code, ULONG *err_position);

ULONG CVJIS_unicode_to_sjis(csconvert* obj, ULONG unicode_len, const UCHAR* unicode_str,
							ULONG sjis_len, UCHAR *sjis_str,
							USHORT *err_code, ULONG *err_position);

ULONG CVJIS_eucj_to_unicode(csconvert* obj, ULONG src_len, const UCHAR* src_ptr,
							ULONG dest_len, UCHAR *dest_ptr,
							USHORT *err_code, ULONG *err_position);

ULONG CVJIS_unicode_to_eucj(csconvert* obj, ULONG unicode_len, const UCHAR* unicode_str,
							ULONG eucj_len, UCHAR *eucj_str,
							USHORT *err_code, ULONG *err_position);

INTL_BOOL CVJIS_check_euc(charset* cs, ULONG euc_len, const UCHAR* euc_str,
	ULONG* offending_position);
INTL_BOOL CVJIS_check_sjis(charset* cs, ULONG sjis_len, const UCHAR* sjis_str,
	ULONG* offending_position);

/*
static USHORT CVJIS_euc2sjis(csconvert* obj, UCHAR *sjis_str, USHORT sjis_len, UCHAR *euc_str
							, USHORT euc_len, SSHORT *err_code, USHORT *err_position);
static USHORT CVJIS_sjis2euc(csconvert* obj, UCHAR *euc_str, USHORT euc_len, UCHAR *sjis_str
							, USHORT sjis_len, SSHORT *err_code, USHORT *err_position);
*/
