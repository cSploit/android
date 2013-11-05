/*
 *	PROGRAM:	Security data base manager
 *	MODULE:		secur_proto.h
 *	DESCRIPTION:	Prototype header file for security.epp
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

#ifndef UTILITIES_SECUR_PROTO_H
#define UTILITIES_SECUR_PROTO_H

typedef void (*FPTR_SECURITY_CALLBACK)(void*, const internal_user_data*, bool);
SSHORT	SECURITY_exec_line (ISC_STATUS* isc_status, FB_API_HANDLE DB, FB_API_HANDLE trans,
							internal_user_data* io_user_data, FPTR_SECURITY_CALLBACK display_func,
							void* callback_arg);
SSHORT	SECURITY_exec_line (ISC_STATUS* isc_status, FB_API_HANDLE DB,
							internal_user_data* io_user_data, FPTR_SECURITY_CALLBACK display_func,
							void* callback_arg);

#endif // UTILITIES_SECUR_PROTO_H

