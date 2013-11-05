/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cvt2_proto.h
 *	DESCRIPTION:	Prototype header file for cvt2.cpp
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

#ifndef JRD_CVT2_PROTO_H
#define JRD_CVT2_PROTO_H

#include "../jrd/jrd.h"

bool	CVT2_get_binary_comparable_desc(dsc*, const dsc*, const dsc*);
SSHORT	CVT2_compare(const dsc*, const dsc*);
SSHORT	CVT2_blob_compare(const dsc*, const dsc*);
void	CVT2_get_name(const dsc*, TEXT*);
USHORT	CVT2_make_string2(const dsc*, USHORT, UCHAR**, Jrd::MoveBuffer&);

#endif // JRD_CVT2_PROTO_H
