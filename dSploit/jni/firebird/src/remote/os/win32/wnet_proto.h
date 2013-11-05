/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		wnet_proto.h
 *	DESCRIPTION:	Prototpe header file for wnet.cpp
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

#ifndef REMOTE_WNET_PROTO_H
#define REMOTE_WNET_PROTO_H

#include "../common/classes/fb_string.h"

#ifdef __cplusplus
extern "C" {
#endif


rem_port*	WNET_analyze(const Firebird::PathName&, ISC_STATUS*, const TEXT*, /*const TEXT*,*/ bool);
rem_port*	WNET_connect(const TEXT*, struct packet*, ISC_STATUS*, USHORT);
rem_port*	WNET_reconnect(HANDLE, ISC_STATUS*);


#ifdef __cplusplus
}	/* extern "C" */
#endif


#endif // REMOTE_WNET_PROTO_H
