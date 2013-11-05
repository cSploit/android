/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		ld_proto.h
 *	DESCRIPTION:	Prototype header file for ld.c & ld2.c
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
 * Adriano dos Santos Fernandes
 */

#ifndef _INTL_LD_PROTO_H_
#define _INTL_LD_PROTO_H_

#ifdef __cplusplus
extern "C" {
#endif

struct CsConvertImpl
{
	struct charset* cs;
	const BYTE* csconvert_datatable;
	const BYTE* csconvert_misc;
};

extern USHORT version;

INTL_BOOL FB_DLL_EXPORT LD_lookup_charset(charset* cs, const ASCII* name, const ASCII* config_info);
INTL_BOOL FB_DLL_EXPORT LD_lookup_texttype(texttype* tt, const ASCII* texttype_name, const ASCII* charset_name,
										   USHORT attributes, const UCHAR* specific_attributes,
										   ULONG specific_attributes_length, INTL_BOOL ignore_attributes,
										   const ASCII* config_info);
void FB_DLL_EXPORT LD_version(USHORT* version);
ULONG FB_DLL_EXPORT LD_setup_attributes(
	const ASCII* textTypeName, const ASCII* charSetName, const ASCII* configInfo,
	ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _INTL_LD_PROTO_H_ */
