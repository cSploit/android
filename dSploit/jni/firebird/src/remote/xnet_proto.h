/*
 *	PROGRAM:	JRD Remote Interface/Server
 *      MODULE:         xnet_proto.h
 *      DESCRIPTION:    Prototpe header file for xnet.cpp
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
 *
 * 2003.05.01 Victor Seryodkin, Dmitry Yemanov: Completed XNET implementation
 */

#ifndef REMOTE_XNET_PROTO_H
#define REMOTE_XNET_PROTO_H

#include "../common/classes/fb_string.h"

#ifdef NO_PORT
#define rem_port void
#endif

rem_port* XNET_analyze(const Firebird::PathName&, ISC_STATUS*, /*const TEXT*, const TEXT*,*/ bool);
rem_port* XNET_connect(/*const TEXT*,*/ struct packet*, ISC_STATUS*, USHORT);

#ifndef SUPERCLIENT
rem_port* XNET_reconnect(ULONG, ISC_STATUS*);
#endif

#endif // REMOTE_XNET_PROTO_H

