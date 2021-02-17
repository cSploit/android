/*
 *	PROGRAM:	Windows NT service control panel installation program
 *	MODULE:		services.cpp
 *	DESCRIPTION:	Functions which update the Windows service manager for FB
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
 * 01-Feb-2002 Paul Reeves: Removed hard-coded registry path
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <windows.h>
#include <ntsecapi.h>
#include <aclapi.h>
#include "../utilities/install/install_nt.h"
#include "../utilities/install/servi_proto.h"
#include "../utilities/install/registry.h"


USHORT SERVICES_install(SC_HANDLE manager,
						const char* service_name,
						const char* display_name,
						const char* display_description,
						const char* executable,
						const char* directory,
						const char* switches,
						const char* dependencies,
						USHORT sw_startup,
						const char* nt_user_name,
						const char* nt_user_password,
						bool interactive_mode,
						bool auto_restart,
						pfnSvcError err_handler)
{
/**************************************
 *
 *	S E R V I C E S _ i n s t a l l
 *
 **************************************
 *
 * Functional description
 *	Install a service in the service control panel.
 *
 **************************************/

	char exe_name[MAX_PATH];
	size_t len = strlen(directory);
	const char last_char = len ? directory[len - 1] : '\\';
	const char* exe_format = (last_char == '\\' || last_char == '/') ? "%s%s.exe" : "%s\\%s.exe";

	int rc = snprintf(exe_name, sizeof(exe_name), exe_format, directory, executable);
	if (rc == sizeof(exe_name) || rc < 0) {
		return (*err_handler) (0, "service executable path name is too long", 0);
	}

	char path_name[MAX_PATH * 2];
	const char* path_format = (strchr(exe_name, ' ') ? "\"%s\"" : "%s");
	sprintf(path_name, path_format, exe_name);

	if (switches)
	{
		len = sizeof(path_name) - strlen(path_name) - 1;
		if (len < strlen(switches) + 1) {
			return (*err_handler) (0, "service command line is too long", 0);
		}
		strcat(path_name, " ");
		strcat(path_name, switches);
	}

	DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	if (nt_user_name != 0)
	{
		if (nt_user_password == 0)
			nt_user_password = "";
	}
	else if (interactive_mode)
	{
		dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
	}

	SC_HANDLE service = CreateService(manager,
							service_name,
							display_name,
							SERVICE_CHANGE_CONFIG | SERVICE_START,
							dwServiceType,
							(sw_startup == STARTUP_DEMAND) ?
								SERVICE_DEMAND_START : SERVICE_AUTO_START,
							SERVICE_ERROR_NORMAL,
							path_name, NULL, NULL, dependencies,
							nt_user_name, nt_user_password);

	if (service == NULL)
	{
		const DWORD errnum = GetLastError();
		if (errnum == ERROR_DUP_NAME || errnum == ERROR_SERVICE_EXISTS)
			return IB_SERVICE_ALREADY_DEFINED;

		return (*err_handler) (errnum, "CreateService", NULL);
	}

	// Now enter the description string and failure actions into the service
	// config, if this is available on the current platform.
	HMODULE advapi32 = LoadLibrary("ADVAPI32.DLL");
	if (advapi32 != 0)
	{
		typedef BOOL __stdcall proto_config2(SC_HANDLE, DWORD, LPVOID);

		proto_config2* const config2 =
			(proto_config2*) GetProcAddress(advapi32, "ChangeServiceConfig2A");

		if (config2 != 0)
		{
			// This system supports the ChangeServiceConfig2 API.
			// LPSTR descr = display_description;
			//(*config2)(service, SERVICE_CONFIG_DESCRIPTION, &descr);
			(*config2)(service, SERVICE_CONFIG_DESCRIPTION, &display_description);

			if (auto_restart)
			{
				SERVICE_FAILURE_ACTIONS fa;
				SC_ACTION	acts;

				memset(&fa, 0, sizeof(fa));
				fa.dwResetPeriod = 0;
				fa.cActions = 1;
				fa.lpsaActions = &acts;

				memset(&acts, 0, sizeof(acts));
				acts.Delay = 0;
				acts.Type = SC_ACTION_RESTART;

				(*config2)(service, SERVICE_CONFIG_FAILURE_ACTIONS, &fa);
			}
		}
		FreeLibrary(advapi32);
	}

	CloseServiceHandle(service);

	return FB_SUCCESS;
}


USHORT SERVICES_remove(SC_HANDLE manager,
					   const char* service_name,
					   //const char* display_name,
					   pfnSvcError err_handler)
{
/**************************************
 *
 *	S E R V I C E S _ r e m o v e
 *
 **************************************
 *
 * Functional description
 *	Remove a service from the service control panel.
 *
 **************************************/
	SERVICE_STATUS service_status;

	SC_HANDLE service = OpenService(manager, service_name, SERVICE_QUERY_STATUS | DELETE);
	if (service == NULL)
		return (*err_handler) (GetLastError(), "OpenService", NULL);

	if (!QueryServiceStatus(service, &service_status))
		return (*err_handler) (GetLastError(), "QueryServiceStatus", service);

	if (service_status.dwCurrentState != SERVICE_STOPPED)
	{
		CloseServiceHandle(service);
		return IB_SERVICE_RUNNING;
	}

	if (!DeleteService(service))
		return (*err_handler) (GetLastError(), "DeleteService", service);

	CloseServiceHandle(service);

	// Let's loop until the service is actually confirmed deleted
	while (true)
	{
		service = OpenService(manager, service_name, GENERIC_READ);
		if (service == NULL)
		{
			if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
				break;
		}
		else
			CloseServiceHandle(service);

		Sleep(100);	// A small nap is always good for health :)
	}

	return FB_SUCCESS;
}


USHORT SERVICES_start(SC_HANDLE manager,
					  const char* service_name,
					  //const char* display_name,
					  USHORT sw_mode,
					  pfnSvcError err_handler)
{
/**************************************
 *
 *	S E R V I C E S _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start an installed service.
 *
 **************************************/
	const SC_HANDLE service = OpenService(manager, service_name, SERVICE_START | SERVICE_QUERY_STATUS);

	if (service == NULL)
		return (*err_handler) (GetLastError(), "OpenService", NULL);

	const TEXT* mode;
	switch (sw_mode)
	{
		case DEFAULT_PRIORITY:
			mode = NULL;
			break;
		case NORMAL_PRIORITY:
			mode = "-r";
			break;
		case HIGH_PRIORITY:
			mode = "-b";
			break;
	}

	if (!StartService(service, mode ? 1 : 0, &mode))
	{
		const DWORD errnum = GetLastError();
		CloseServiceHandle(service);
		if (errnum == ERROR_SERVICE_ALREADY_RUNNING)
			return FB_SUCCESS;

		return (*err_handler) (errnum, "StartService", NULL);
	}

	// Wait for the service to actually start before returning.
	SERVICE_STATUS service_status;

	do
	{
		if (!QueryServiceStatus(service, &service_status))
			return (*err_handler) (GetLastError(), "QueryServiceStatus", service);
		Sleep(100);	// Don't loop too quickly (would be useless)
	} while (service_status.dwCurrentState == SERVICE_START_PENDING);

	if (service_status.dwCurrentState != SERVICE_RUNNING)
		return (*err_handler) (0, "Service failed to complete its startup sequence.", service);

	CloseServiceHandle(service);

	return FB_SUCCESS;
}


USHORT SERVICES_stop(SC_HANDLE manager,
					 const char* service_name,
					 //const char* display_name,
					 pfnSvcError err_handler)
{
/**************************************
 *
 *	S E R V I C E S _ s t o p
 *
 **************************************
 *
 * Functional description
 *	Stop a running service.
 *
 **************************************/
	const SC_HANDLE service = OpenService(manager, service_name, SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (service == NULL)
		return (*err_handler) (GetLastError(), "OpenService", NULL);

	SERVICE_STATUS service_status;

	if (!ControlService(service, SERVICE_CONTROL_STOP, &service_status))
	{
		const DWORD errnum = GetLastError();
		CloseServiceHandle(service);
		if (errnum == ERROR_SERVICE_NOT_ACTIVE)
			return FB_SUCCESS;

		return (*err_handler) (errnum, "ControlService", NULL);
	}

	// Wait for the service to actually stop before returning.
	do
	{
		if (!QueryServiceStatus(service, &service_status))
			return (*err_handler) (GetLastError(), "QueryServiceStatus", service);
		Sleep(100);	// Don't loop too quickly (would be useless)
	} while (service_status.dwCurrentState == SERVICE_STOP_PENDING);

	if (service_status.dwCurrentState != SERVICE_STOPPED)
		return (*err_handler) (0, "Service failed to complete its stop sequence", service);

	CloseServiceHandle(service);

	return FB_SUCCESS;
}

USHORT SERVICES_status (const char* service_name)
{
/**************************************
 *
 *	S E R V I C E S _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *  Returns the Running/Stopped/Pending status of a service.
 *  This API does not use an error handler because it has not real error
 *  to ever report. For instance, a non-existing service is not reported as
 *  an error, but as a status info through the function return value.
 *
 **************************************/

	SC_HANDLE manager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (manager == NULL)
		return FB_SERVICE_STATUS_UNKNOWN;

	SC_HANDLE service = OpenService(manager, service_name, GENERIC_READ);
	if (service == NULL)
	{
		CloseServiceHandle(manager);
		return FB_SERVICE_STATUS_NOT_INSTALLED;
	}

	SERVICE_STATUS service_status;
	if (!QueryServiceStatus(service, &service_status))
	{
		CloseServiceHandle(service);
		CloseServiceHandle(manager);
		return FB_SERVICE_STATUS_UNKNOWN;
	}
	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	USHORT status = FB_SERVICE_STATUS_UNKNOWN;
	switch (service_status.dwCurrentState)
	{
		case SERVICE_RUNNING : status = FB_SERVICE_STATUS_RUNNING; break;
		case SERVICE_STOPPED : status = FB_SERVICE_STATUS_STOPPED; break;
		case SERVICE_START_PENDING :	// fall over the next case
		case SERVICE_STOP_PENDING : status = FB_SERVICE_STATUS_PENDING; break;
	}

	return status;
}

USHORT SERVICES_grant_privilege(const TEXT* account, pfnSvcError err_handler, const WCHAR* privilege)
{
/***************************************************
 *
 * S E R V I C E _ g r a n t _ l o g o n _ r i g h t
 *
 ***************************************************
 *
 * Functional description
 *  Grants the "Log on as a service" right to account.
 *  This is a Windows NT, 2000, XP, 2003 security thing.
 *  To run a service under an account other than LocalSystem, the account
 *  must have this right. To succeed granting the right, the current user
 *  must be an Administrator.
 *  Returns FB_SUCCESS when actually granted the right.
 *  Returns FB_LOGON_SRVC_RIGHT_ALREADY_DEFINED if right was already granted
 *  to the user.
 *  Returns FB_FAILURE on any error.
 *
 *  OM - AUG 2003 - Initial implementation
 *  OM - SEP 2003 - Control flow revision, no functional change
 *
 ***************************************************/

	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
	LSA_HANDLE PolicyHandle;

	// Open the policy on the local machine.
	ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	NTSTATUS lsaErr = LsaOpenPolicy(NULL, &ObjectAttributes,
		POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &PolicyHandle);
	if (lsaErr != (NTSTATUS) 0)
		return (*err_handler)(LsaNtStatusToWinError(lsaErr), "LsaOpenPolicy", NULL);

	// Obtain the SID of the user/group.
	// First, dummy call to LookupAccountName to get the required buffer sizes.
	DWORD cbSid;
	DWORD cchDomain;
	cbSid = cchDomain = 0;
	SID_NAME_USE peUse;
	LookupAccountName(NULL, account, NULL, &cbSid, NULL, &cchDomain, &peUse);
	PSID pSid = (PSID) LocalAlloc(LMEM_ZEROINIT, cbSid);
	if (pSid == 0)
	{
		DWORD err = GetLastError();
		LsaClose(PolicyHandle);
		return (*err_handler)(err, "LocalAlloc(Sid)", NULL);
	}
	TEXT* pDomain = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, cchDomain);
	if (pDomain == 0)
	{
		DWORD err = GetLastError();
		LsaClose(PolicyHandle);
		LocalFree(pSid);
		return (*err_handler)(err, "LocalAlloc(Domain)", NULL);
	}
	// Now, really obtain the SID of the user/group.
	if (LookupAccountName(NULL, account, pSid, &cbSid, pDomain, &cchDomain, &peUse) == 0)
	{
		DWORD err = GetLastError();
		LsaClose(PolicyHandle);
		LocalFree(pSid);
		LocalFree(pDomain);
		return (*err_handler)(err, "LookupAccountName", NULL);
	}

	PLSA_UNICODE_STRING UserRights;
	ULONG CountOfRights = 0;
	NTSTATUS ntStatus = LsaEnumerateAccountRights(PolicyHandle, pSid, &UserRights, &CountOfRights);
	if (ntStatus == (NTSTATUS) 0xC0000034L) //STATUS_OBJECT_NAME_NOT_FOUND
		CountOfRights = 0;
	// Check if the seServiceLogonRight is already granted
	ULONG i;
	for (i = 0; i < CountOfRights; i++)
	{
		if (wcscmp(UserRights[i].Buffer, privilege) == 0)
			break;
	}
	LsaFreeMemory(UserRights); // Don't leak
	LSA_UNICODE_STRING PrivilegeString;
	if (CountOfRights == 0 || i == CountOfRights)
	{
		// Grant the SeServiceLogonRight to users represented by pSid.
		const int string_buff_size = 100;
		WCHAR tempStr[string_buff_size];
		wcsncpy(tempStr, privilege, string_buff_size - 1);
		tempStr[string_buff_size - 1] = 0;

		PrivilegeString.Buffer = tempStr;
		PrivilegeString.Length = wcslen(tempStr) * sizeof(WCHAR);
		PrivilegeString.MaximumLength = sizeof(tempStr);
		if ((lsaErr = LsaAddAccountRights(PolicyHandle, pSid, &PrivilegeString, 1)) != (NTSTATUS) 0)
		{
			LsaClose(PolicyHandle);
			LocalFree(pSid);
			LocalFree(pDomain);
			return (*err_handler)(LsaNtStatusToWinError(lsaErr), "LsaAddAccountRights", NULL);
		}
	}
	else
	{
		LsaClose(PolicyHandle);
		LocalFree(pSid);
		LocalFree(pDomain);
		return FB_PRIVILEGE_ALREADY_GRANTED;
	}

	LsaClose(PolicyHandle);
	LocalFree(pSid);
	LocalFree(pDomain);

	return FB_SUCCESS;
}


USHORT SERVICES_grant_access_rights(const char* service_name, const TEXT* account, pfnSvcError err_handler)
{
/*********************************************************
 *
 * S E R V I C E S _ g r a n t _ a c c e s s _ r i g h t s
 *
 *********************************************************
 *
 * Functional description
 *
 * Grant access rights to service 'service_name' so that user 'account'
 * can control it (start, stop, query).
 * Intended to be called after SERVICES_install().
 * By doing so to the Firebird server service object, we can set the Guardian
 * to run as the same specific user, yet still be able to start the Firebird
 * server from the Guardian.
 * 'account' is of format : DOMAIN\User or SERVER\User.
 * Returns FB_SUCCESS or FB_FAILURE.
 *
 * OM - SEP 2003 - Initial implementation
 *
 *********************************************************/

	PACL pOldDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;

	// Get Security Information on the service. Will of course fail if we're
	// not allowed to do this. Administrators should be allowed, by default.
	// CVC: Only GetNamedSecurityInfoEx has the first param declared const, so we need
	// to make the compiler happy after Blas' cleanup.
	if (GetNamedSecurityInfo(const_cast<CHAR*>(service_name), SE_SERVICE, DACL_SECURITY_INFORMATION,
		NULL /*Owner Sid*/, NULL /*Group Sid*/,
		&pOldDACL, NULL /*Sacl*/, &pSD) != ERROR_SUCCESS)
	{
		return (*err_handler)(GetLastError(), "GetNamedSecurityInfo", NULL);
	}

	// Initialize an EXPLICIT_ACCESS structure.
	EXPLICIT_ACCESS ea;
	ZeroMemory(&ea, sizeof(ea));
	ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = const_cast<char*>(account); // safe

	// Create a new DACL, adding this right to whatever exists.
	PACL pNewDACL = NULL;
	if (SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS)
	{
		DWORD err = GetLastError();
		LocalFree(pSD);
		return (*err_handler)(err, "SetEntriesInAcl", NULL);
	}

	// Updates the new rights in the object
	if (SetNamedSecurityInfo(const_cast<CHAR*>(service_name), SE_SERVICE, DACL_SECURITY_INFORMATION,
		NULL /*Owner Sid*/, NULL /*Group Sid*/,
		pNewDACL, NULL /*Sacl*/) != ERROR_SUCCESS)
	{
		DWORD err = GetLastError();
		LocalFree(pSD);
		LocalFree(pNewDACL);
		return (*err_handler)(err, "SetNamedSecurityInfo", NULL);
	}

	return FB_SUCCESS;
}

//
// Until the fb_assert could be converted to a function/object linked with each module
// we need this ugly workaround.
//
extern "C" void API_ROUTINE gds__log(const TEXT* text, ...)
{
	va_list ptr;

	va_start(ptr, text);
	vprintf(text, ptr);
	va_end(ptr);
	printf("\n\n");
}

//
// EOF
//
