/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dsc_proto.h
 *	DESCRIPTION:	Prototype header file for dsc.cpp
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

#ifndef JRD_DSC_PROTO_H
#define JRD_DSC_PROTO_H

#include "../jrd/dsc.h"

int			DSC_string_length(const struct dsc*);
const TEXT*	DSC_dtype_tostring(UCHAR);
void		DSC_get_dtype_name(const dsc*, TEXT*, USHORT);
bool		DSC_make_descriptor(dsc*, USHORT, SSHORT,
										   USHORT, SSHORT, SSHORT, SSHORT);
USHORT		DSC_convert_to_text_length(USHORT dsc_type);

extern const BYTE DSC_add_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX];
extern const BYTE DSC_sub_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX];
extern const BYTE DSC_multiply_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX];
extern const BYTE DSC_multiply_blr4_result[DTYPE_TYPE_MAX][DTYPE_TYPE_MAX];

#endif // JRD_DSC_PROTO_H

