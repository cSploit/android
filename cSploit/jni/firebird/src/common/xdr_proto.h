/*
 *	PROGRAM:	JRD Remote Server
 *	MODULE:		xdr_proto.h
 *	DESCRIPTION:	Prototype Header file for xdr.cpp
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

#ifndef REMOTE_XDR_PROTO_H
#define REMOTE_XDR_PROTO_H

#include "../common/dsc.h"
#include "../common/xdr.h"

// 15 Jan 2003. Nickolay Samofatov
// Functions below need to have C++ linkage to avoid name clash with
// standard XDR. Firebird XDR is NOT compatible with Sun XDR at interface level

bool_t	xdr_datum(XDR*, const dsc*, UCHAR*);
bool_t	xdr_double(XDR*, double*);
bool_t	xdr_enum(XDR*, xdr_op*);
bool_t	xdr_float(XDR*, float*);
bool_t	xdr_int(XDR*, int*);
bool_t	xdr_long(XDR*, SLONG*);
bool_t	xdr_opaque(XDR*, SCHAR*, u_int);
bool_t	xdr_quad(XDR*, SQUAD*);
bool_t	xdr_short(XDR*, SSHORT*);
bool_t	xdr_string(XDR*, SCHAR**, u_int);
bool_t	xdr_u_int(XDR*, u_int*);
bool_t	xdr_u_long(XDR*, ULONG*);
bool_t	xdr_u_short(XDR*, u_short*);
bool_t	xdr_wrapstring(XDR*, SCHAR**);
bool_t	xdr_hyper(XDR*, void*);
bool_t	xdrmem_create(XDR*, SCHAR*, u_int, xdr_op);
SLONG	xdr_peek_long(const XDR*, const void* data, size_t size);

#endif	// REMOTE_XDR_PROTO_H
