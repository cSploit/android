/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_ascii.h
 *	DESCRIPTION:	Language Drivers in the binary collation family.
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


USHORT famasc_key_length(texttype* obj, USHORT inLen);
USHORT famasc_string_to_key(texttype* obj, USHORT iInLen, const BYTE* pInChar,
	USHORT iOutLen, BYTE *pOutChar, USHORT partial);
SSHORT famasc_compare(texttype* obj, ULONG l1, const BYTE* s1, ULONG l2, const BYTE* s2,
	INTL_BOOL* error_flag);

ULONG cp1251_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE* pOutStr);
ULONG cp1251_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE* pOutStr);

/*
 * Generic base for InterBase 4.0 Language Driver - ASCII family (binary
 * 8 bit sorting)
 */

/* CVC: Redefined in lc_ascii.cpp, the only place where this macro seems to be used.
//#define LANGASCII_MAX_KEY	(MAX_KEY)

//#define ASCII_SPACE	32			// ASCII code for space
//
//#define	ASCII7_UPPER(ch) \
//	((((UCHAR) (ch) >= (UCHAR) ASCII_LOWER_A) && ((UCHAR) (ch) <= (UCHAR) ASCII_LOWER_Z)) \
//		? (UCHAR) ((ch) - ASCII_LOWER_A + ASCII_UPPER_A) \
//		: (UCHAR) (ch))
//#define	ASCII7_LOWER(ch) \
//	((((UCHAR) (ch) >= (UCHAR) ASCII_UPPER_A) && ((UCHAR) (ch) <= (UCHAR) ASCII_UPPER_Z)) \
//		? (UCHAR) ((ch) - ASCII_UPPER_A + ASCII_LOWER_A) \
//		: (UCHAR) (ch))
*/

#define ASCII_UPPER_A	65		// ASCII code for 'A'
#define ASCII_LOWER_A	(ASCII_UPPER_A + 32)	// ASCII code for 'a'
#define ASCII_UPPER_Z	90		// ASCII code for 'Z'
#define ASCII_LOWER_Z	(ASCII_UPPER_Z + 32)	// ASCII code for 'z'
