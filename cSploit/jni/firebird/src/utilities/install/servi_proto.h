/*
 *	PROGRAM:	Windows NT installation utilities
 *	MODULE:		servi_proto.h
 *	DESCRIPTION:	Prototypes for NT service functions
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

#ifndef UTILITIES_SERVI_PROTO_H
#define UTILITIES_SERVI_PROTO_H

// OM Jan '04 : Huh ? Who decided to drop the parameters names from these proto ?
// Is there a consensus that removing them makes the code more self-documenting ?

typedef USHORT (*pfnSvcError)(SLONG, const TEXT*, SC_HANDLE);

USHORT SERVICES_install(SC_HANDLE manager,
						const char* service_name,
						const char* display_name,
						const char* display_description,
						const char* executable,
						const char* directory,
						const char* switches,
						const char* dependencies,
						USHORT		sw_startup,
						const char* nt_user_name,
						const char* nt_user_password,
						bool		interactive_mode,
						bool		auto_restart,
						pfnSvcError err_handler);

USHORT	SERVICES_remove(SC_HANDLE, const char*, /*const char*,*/ pfnSvcError);

USHORT	SERVICES_start(SC_HANDLE, const char*, /*const char*,*/ USHORT, pfnSvcError);

USHORT	SERVICES_stop(SC_HANDLE, const char*, /*const char*,*/ pfnSvcError);

USHORT	SERVICES_status(const char*);

USHORT	SERVICES_grant_privilege(const TEXT* account, pfnSvcError err_handler, const WCHAR* privilege);

USHORT	SERVICES_grant_access_rights(const char*, const TEXT*, pfnSvcError);

#endif // UTILITIES_SERVI_PROTO_H
