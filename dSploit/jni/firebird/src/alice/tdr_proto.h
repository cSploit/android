/*
 *	PROGRAM:	Alice (All Else) Utility
 *	MODULE:		tdr_proto.h
 *	DESCRIPTION:	Prototype header file for tdr.cpp
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

#ifndef ALICE_TDR_PROTO_H
#define ALICE_TDR_PROTO_H

void	TDR_list_limbo(FB_API_HANDLE, const TEXT*, const SINT64);
bool	TDR_reconnect_multiple(FB_API_HANDLE, SLONG, const TEXT*, SINT64);
void	TDR_shutdown_databases(tdr*);
USHORT	TDR_analyze(const tdr*);
bool	TDR_attach_database(ISC_STATUS*, tdr*, const TEXT*);
void	TDR_get_states(tdr*);

#endif // ALICE_TDR_PROTO_H

