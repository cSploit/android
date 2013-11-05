/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		misc_proto.h
 *	DESCRIPTION:	Prototype Header file for misc.cpp
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

#ifndef BURP_MISC_PROTO_H
#define BURP_MISC_PROTO_H

UCHAR*	MISC_alloc_burp(ULONG);
void	MISC_free_burp(void*);
void	MISC_release_request_silent(isc_req_handle& req_handle);
int		MISC_symbol_length(const TEXT*, ULONG);
void	MISC_terminate(const TEXT*, TEXT*, ULONG, ULONG);

#endif	// BURP_MISC_PROTO_H

