/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_dos.h
 *	DESCRIPTION:	Language Drivers for compatibility with DOS products.
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

ULONG fam1_str_to_upper(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);
ULONG fam1_str_to_lower(texttype* obj, ULONG iLen, const BYTE* pStr, ULONG iOutLen, BYTE *pOutStr);
