/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cv_big5.h
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

#include "../intl/ldcommon.h"

/* These macros have a duplicate in lc_big5.cpp */
#define	BIG51(uc)	((UCHAR)((uc)&0xff)>=0xa1 && \
			 (UCHAR)((uc)&0xff)<=0xfe)	/* BIG-5 1st-byte */
#define	BIG52(uc)	((UCHAR)((uc)&0xff)>=0x40 && \
			 (UCHAR)((uc)&0xff)<=0xfe)	/* BIG-5 2nd-byte */

ULONG CVBIG5_big5_to_unicode(csconvert* obj, ULONG src_len, const UCHAR* src_ptr,
							 ULONG dest_len, UCHAR *dest_ptr,
							 USHORT *err_code, ULONG *err_position);
ULONG CVBIG5_unicode_to_big5(csconvert* obj, ULONG unicode_len, const UCHAR* unicode_str,
							 ULONG big5_len, UCHAR *big5_str,
							 USHORT *err_code, ULONG *err_position);
USHORT CVBIG5_check_big5(charset* cs, ULONG big5_len, const UCHAR* big5_str, ULONG* offending_position);
