/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		proto_proto.h
 *	DESCRIPTION:	Prototype Header file for protocol.cpp
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

#ifndef REMOTE_PROTO_PROTO_H
#define REMOTE_PROTO_PROTO_H

#ifdef DEBUG_XDR_MEMORY
void	xdr_debug_memory (XDR*, enum xdr_op, const void*, const void*, ULONG);
#endif
bool_t	xdr_protocol (XDR*, struct packet*);
ULONG	xdr_protocol_overhead (P_OP);

#endif	//  REMOTE_PROTO_PROTO_H
