/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_ksc.h
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

/*
*	KSC-5601 -> unicode
*	% KSC-5601 is same to EUC cs1(codeset 1). Then converting
*	KSC-5601 to EUC is not needed.
*/

/* These macros have a duplicate in lc_ksc.cpp */
#define	KSC1(uc)	((uc) & 0x80)
#define	KSC2(uc)	((uc) >= 0x41)

#define	GEN_HAN(b1, b2)	((0xb0 <= (b1) && (b1) <= 0xc8) && (0xa1 <= (b2) && (b2) <= 0xfe))

#define	SPE_HAN(b1, b2)	(((b1) == 0xa4) && (((b2) == 0xa2) || ((b2) == 0xa4) || ((b2) == 0xa7) || ((b2) == 0xa8) || ((b2) == 0xa9) || ((b2) == 0xb1) || ((b2) == 0xb2) || ((b2) == 0xb3) || ((b2) == 0xb5) || ((b2) == 0xb6) || ((b2) == 0xb7) || ((b2) == 0xb8) || ((b2) == 0xb9) || ((b2) == 0xba) || ((b2) == 0xbb) || ((b2) == 0xbc) || ((b2) == 0xbd) || ((b2) == 0xbe)))

ULONG CVKSC_ksc_to_unicode(csconvert* obj, ULONG ksc_len, const UCHAR* ksc_str,
						   ULONG dest_len, UCHAR* dest_ptr,
						   USHORT* err_code, ULONG* err_position);

ULONG CVKSC_unicode_to_ksc(csconvert* obj, ULONG unicode_len, const UCHAR* unicode_str,
						   ULONG ksc_len, UCHAR* ksc_str,
						   USHORT* err_code, ULONG* err_position);

INTL_BOOL CVKSC_check_ksc(charset* cs, ULONG ksc_len, const UCHAR* ksc_str, ULONG* offending_position);
