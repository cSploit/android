/*
 *	PROGRAM:	Security data base manager
 *	MODULE:		call_service.h
 *	DESCRIPTION:	Invokes remote service manager to work with security DB.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff (peshkoff@mail.ru)
 *  for the Firebird Open Source RDBMS project.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef UTILITIES_GSEC_CALL_SERVICE_H
#define UTILITIES_GSEC_CALL_SERVICE_H

#include "../jrd/ibase.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/secur_proto.h"

isc_svc_handle attachRemoteServiceManager(ISC_STATUS*, const TEXT*, const TEXT*,
							  bool, int, const TEXT*);
isc_svc_handle attachRemoteServiceManager(ISC_STATUS*, const TEXT*, const TEXT*, bool, const TEXT*);
void callRemoteServiceManager(ISC_STATUS*, isc_svc_handle, const internal_user_data&,
							  FPTR_SECURITY_CALLBACK, void*);
void detachRemoteServiceManager(ISC_STATUS*, isc_svc_handle);

#endif // UTILITIES_GSEC_CALL_SERVICE_H
