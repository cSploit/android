/*
 *	PROGRAM:	JRD Remote server
 *	MODULE:		serve_proto.h
 *	DESCRIPTION:	Prototype Header file for server.cpp
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

#ifndef REMOTE_SERVE_PROTO_H
#define REMOTE_SERVE_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NO_PORT
#define rem_port void
#endif

void SRVR_main(rem_port*, USHORT);
void SRVR_multi_thread(rem_port*, USHORT);

#ifdef __cplusplus
} // extern "C"
#endif


#endif	// REMOTE_SERVE_PROTO_H

