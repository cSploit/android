/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		intl_proto.h
 *	DESCRIPTION:	Prototype Header file for intl.cpp
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

#ifndef JRD_INTL_PROTO_H
#define JRD_INTL_PROTO_H

#include "../jrd/intl_classes.h"

namespace Jrd {
	class thread_db;
	class Lock;
}

struct dsc;
struct SubtypeInfo;

CHARSET_ID	INTL_charset(Jrd::thread_db*, USHORT);
int			INTL_compare(Jrd::thread_db*, const dsc*, const dsc*, ErrorFunction);
ULONG		INTL_convert_bytes(Jrd::thread_db*, CHARSET_ID, UCHAR*, ULONG, CHARSET_ID,
								const BYTE*, ULONG, ErrorFunction);
Jrd::CsConvert	INTL_convert_lookup(Jrd::thread_db*, CHARSET_ID, CHARSET_ID);
int			INTL_convert_string(dsc*, const dsc*, ErrorFunction);
bool		INTL_data(const dsc*);
bool		INTL_data_or_binary(const dsc*);
bool		INTL_defined_type(Jrd::thread_db*, USHORT);
USHORT		INTL_key_length(Jrd::thread_db*, USHORT, USHORT);
Jrd::CharSet*	INTL_charset_lookup(Jrd::thread_db* tdbb, USHORT parm1);
Jrd::Collation*	INTL_texttype_lookup(Jrd::thread_db* tdbb, USHORT parm1);
void		INTL_texttype_unload(Jrd::thread_db*, USHORT);
bool		INTL_texttype_validate(Jrd::thread_db*, const SubtypeInfo*);
void		INTL_pad_spaces(Jrd::thread_db*, dsc*, UCHAR*, ULONG);
USHORT		INTL_string_to_key(Jrd::thread_db*, USHORT, const dsc*, dsc*, USHORT);

// Built-in charsets/texttypes interface
INTL_BOOL INTL_builtin_lookup_charset(charset* cs, const ASCII* charset_name, const ASCII* config_info);
INTL_BOOL INTL_builtin_lookup_texttype(texttype* tt, const ASCII* texttype_name, const ASCII* charset_name,
									   USHORT attributes, const UCHAR* specific_attributes,
									   ULONG specific_attributes_length, INTL_BOOL ignore_attributes,
									   const ASCII* config_info);
ULONG INTL_builtin_setup_attributes(const ASCII* textTypeName, const ASCII* charSetName,
	const ASCII* configInfo, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst);

#endif // JRD_INTL_PROTO_H

