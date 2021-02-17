/*
 *	PROGRAM:		Windows NT service control panel installation program
 *	MODULE:			install_svc.cpp
 *	DESCRIPTION:	Service control panel installation program
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

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "../jrd/license.h"
#include "../utilities/install/install_nt.h"
#include "../utilities/install/servi_proto.h"
#include "../utilities/install/registry.h"
#include "../common/config/config.h"

static void svc_query(const char*, const char*, SC_HANDLE manager);
static USHORT svc_query_ex(SC_HANDLE manager);
static USHORT svc_error(SLONG, const TEXT*, SC_HANDLE);
static void usage_exit();

static const struct
{
	const TEXT *name;
	USHORT abbrev;
	USHORT code;
} commands[] =
{
	{"INSTALL", 1, COMMAND_INSTALL},
	{"REMOVE", 1, COMMAND_REMOVE},
	{"START", 3, COMMAND_START},
	{"STOP", 3, COMMAND_STOP},
	{"QUERY", 1, COMMAND_QUERY},
	{NULL, 0, 0}
};

int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Install or remove a Firebird service.
 *
 **************************************/
	USHORT sw_command = COMMAND_NONE;
	bool sw_version = false;
	USHORT sw_startup = STARTUP_AUTO;
	USHORT sw_mode = DEFAULT_PRIORITY;
	USHORT sw_guardian = NO_GUARDIAN;
	USHORT sw_arch = ARCH_SS;
	bool sw_interactive = false;

	const TEXT* instance = FB_DEFAULT_INSTANCE;

	const TEXT* username = NULL;
	const TEXT* password = NULL;

	// Let's get the root directory from the instance path of this program.
	// argv[0] is only _mostly_ guaranteed to give this info,
	// so we GetModuleFileName()
	TEXT directory[MAXPATHLEN];
	const USHORT len = GetModuleFileName(NULL, directory, sizeof(directory));
	if (len == 0)
		return svc_error(GetLastError(), "GetModuleFileName", NULL);

	fb_assert(len <= sizeof(directory));

	// Get to the last '\' (this one precedes the filename part). There is
	// always one after a call to GetModuleFileName().
	TEXT* p = directory + len;

	while (p != directory)
	{
		--p;

		if ((*p) == '\\')
			break;
	}
	*p = '\0';

	TEXT full_username[128];
	TEXT oem_username[128];
	TEXT keyb_password[64];

	const TEXT* const* const end = argv + argc;
	while (++argv < end)
	{
		if (**argv != '-')
		{
			int i;
			const TEXT* cmd;
			for (i = 0; cmd = commands[i].name; i++)
			{
				const TEXT* q;
				for (p = *argv, q = cmd; *p && UPPER(*p) == *q; p++, q++)
					;
				if (!*p && commands[i].abbrev <= (USHORT) (q - cmd))
					break;
			}
			if (!cmd)
			{
				printf("Unknown command \"%s\"\n", *argv);
				usage_exit();
			}
			sw_command = commands[i].code;
		}
		else
		{
			p = *argv + 1;
			switch (UPPER(*p))
			{
				case 'A':
					sw_startup = STARTUP_AUTO;
					break;

				case 'D':
					sw_startup = STARTUP_DEMAND;
					break;

				/*
				case 'R':
					sw_mode = NORMAL_PRIORITY;
					break;
				*/
				case 'B':
					sw_mode = HIGH_PRIORITY;
					break;

				case 'Z':
					sw_version = true;
					break;

				case 'G':
					sw_guardian = USE_GUARDIAN;
					break;

				case 'S':
					sw_arch = ARCH_SS;
					break;

				case 'C':
					sw_arch = ARCH_CS;
					break;

				case 'L':
					if (++argv < end)
						username = *argv;
					if (++argv < end)
					{
						if (**argv == '-')	// Next switch
							--argv;
						else
							password = *argv;
					}
					break;

				case 'I':
					sw_interactive = true;
					break;

				case 'N':
					if (++argv < end)
						instance = *argv;
					break;

				case '?':
					usage_exit();

				default:
					printf("Unknown switch \"%s\"\n", p);
					usage_exit();
			}
		}
	}

	if (sw_version)
		printf("instsvc version %s\n", FB_VERSION);

	if (sw_command == COMMAND_NONE || (username && sw_command != COMMAND_INSTALL))
	{
		usage_exit();
	}

	if (sw_command == COMMAND_INSTALL && username != 0)
	{
		if (sw_interactive)
		{
			printf("\"Interact with desktop\" mode can be set for LocalSystem account only");
			exit(FINI_ERROR);
		}

		const char* limit = username;
		while (*limit != '\0' && *limit != '\\')
			++limit;

		if (!*limit)
		{
			DWORD cnlen = sizeof(full_username) - 1;
			GetComputerName(full_username, &cnlen);
			strcat(full_username, "\\");
			strncat(full_username, username, sizeof(full_username) - (cnlen + 1));
		}
		else
		{
			strncpy(full_username, username, sizeof(full_username));
		}

		full_username[sizeof(full_username) - 1] = '\0';

		CharToOem(full_username, oem_username);

		username = full_username;

		if (password == 0)
		{
			printf("Enter %s user password : ", oem_username);
			p = keyb_password;
			const TEXT* const pass_end = p + sizeof(keyb_password) - 1;	// keep room for '\0'

			while (p < pass_end && (*p++ = getch()) != '\r')
				putch('*'); // Win32 only

			*(p - 1) = '\0';	// Cuts at '\r'

			printf("\n");
			OemToChar(keyb_password, keyb_password);
			password = keyb_password;
		}

		// Let's grant "Logon as a Service" right to the -login user
		switch (SERVICES_grant_privilege(full_username, svc_error, L"SeServiceLogonRight"))
		{
			case FB_PRIVILEGE_ALREADY_GRANTED:
				/*
				// OM - I think it is better not to bother the admin with this message.
				printf("The 'Logon as a Service' right was already granted to %s\n", oem_username);
				*/
				break;
			case FB_SUCCESS:
				printf("The 'Logon as a Service' right has been granted to %s\n", oem_username);
				break;
			case FB_FAILURE:
			default:
				printf("Failed granting the 'Logon as a Service' right to %s\n", oem_username);
				exit(FINI_ERROR);
				break;
		}

		// Let's grant "Adjust memory quotas for a process" right to the -login user
		switch (SERVICES_grant_privilege(full_username, svc_error, L"SeIncreaseQuotaPrivilege"))
		{
			case FB_PRIVILEGE_ALREADY_GRANTED:
				break;
			case FB_SUCCESS:
				printf("The 'Adjust memory quotas for a process' right has been granted to %s\n", oem_username);
				break;
			case FB_FAILURE:
			default:
				printf("Failed granting the 'Adjust memory quotas for a process' right to %s\n", oem_username);
				exit(FINI_ERROR);
				break;
		}
	}

	DWORD dwScmManagerAccess = SC_MANAGER_ALL_ACCESS;

	switch (sw_command)
	{
		case COMMAND_INSTALL:
		case COMMAND_REMOVE:
			dwScmManagerAccess = SC_MANAGER_CREATE_SERVICE;
			break;

		case COMMAND_START:
		case COMMAND_STOP:
			dwScmManagerAccess = SC_MANAGER_CONNECT;
			break;

		case COMMAND_QUERY:
			dwScmManagerAccess = SC_MANAGER_ENUMERATE_SERVICE;
			break;
    }

	const SC_HANDLE manager = OpenSCManager(NULL, NULL, dwScmManagerAccess);
	if (manager == NULL)
	{
		svc_error(GetLastError(), "OpenSCManager", NULL);
		exit(FINI_ERROR);
	}

	USHORT status, status2;
	SC_HANDLE service;

	Firebird::string guard_service_name, guard_display_name;
	guard_service_name.printf(ISCGUARD_SERVICE, instance);
	guard_display_name.printf(ISCGUARD_DISPLAY_NAME, instance);

	Firebird::string remote_service_name, remote_display_name;
	remote_service_name.printf(REMOTE_SERVICE, instance);
	remote_display_name.printf(REMOTE_DISPLAY_NAME, instance);

	Firebird::string switches;
	if (strchr(instance, ' '))
		switches.printf("-s \"%s\"", instance);
	else
		switches.printf("-s %s", instance);

	if (sw_arch == ARCH_SS)
		switches += " -m";

	switch (sw_command)
	{
		case COMMAND_INSTALL:
			// First, lets do the guardian, if it has been specified
			if (sw_guardian)
			{
				status = SERVICES_install(manager,
										  guard_service_name.c_str(),
										  guard_display_name.c_str(),
										  ISCGUARD_DISPLAY_DESCR,
										  ISCGUARD_EXECUTABLE,
										  directory,
										  switches.c_str(),
										  NULL,
										  sw_startup,
										  username,
										  password,
										  false, // interactive_mode
										  true, // auto_restart
										  svc_error);

				status2 = FB_SUCCESS;

				if (username != 0)
				{
					status2 =
						SERVICES_grant_access_rights(guard_service_name.c_str(), username, svc_error);
				}

				if (status == FB_SUCCESS && status2 == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully created.\n", guard_display_name.c_str());
				}

				// Set sw_startup to manual in preparation for install the service
				sw_startup = STARTUP_DEMAND;
			}

			// do the install of the server
			status = SERVICES_install(manager,
									  remote_service_name.c_str(),
									  remote_display_name.c_str(),
									  REMOTE_DISPLAY_DESCR,
									  REMOTE_EXECUTABLE,
									  directory,
									  switches.c_str(),
									  NULL,
									  sw_startup,
									  username,
									  password,
									  sw_interactive,
									  !sw_guardian,
									  svc_error);

			status2 = FB_SUCCESS;

			if (username != 0)
			{
				status2 =
					SERVICES_grant_access_rights(remote_service_name.c_str(), username, svc_error);
			}

			if (status == FB_SUCCESS && status2 == FB_SUCCESS)
			{
				printf("Service \"%s\" successfully created.\n", remote_display_name.c_str());
			}

			break;

		case COMMAND_REMOVE:
			service = OpenService(manager, guard_service_name.c_str(), SERVICE_ALL_ACCESS);
			if (service)
			{
				CloseServiceHandle(service);

				status = SERVICES_remove(manager, guard_service_name.c_str(),
										 /*guard_display_name.c_str(),*/ svc_error);

				if (status == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully deleted.\n", guard_display_name.c_str());
				}
				else if (status == IB_SERVICE_RUNNING)
				{
					printf("Service \"%s\" not deleted.\n", guard_display_name.c_str());
					printf("You must stop it before attempting to delete it.\n\n");
				}
			}
			else
			{
				status = (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) ? FB_SUCCESS : FB_FAILURE;
			}

			service = OpenService(manager, remote_service_name.c_str(), SERVICE_ALL_ACCESS);
			if (service)
			{
				CloseServiceHandle(service);

				status2 = SERVICES_remove(manager, remote_service_name.c_str(),
										  /*remote_display_name.c_str(),*/ svc_error);

				if (status2 == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully deleted.\n", remote_display_name.c_str());
				}
				else if (status2 == IB_SERVICE_RUNNING)
				{
					printf("Service \"%s\" not deleted.\n", remote_display_name.c_str());
					printf("You must stop it before attempting to delete it.\n\n");
				}
			}
			else
			{
				status2 = (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) ? FB_SUCCESS : FB_FAILURE;
			}

			if (status != FB_SUCCESS && status2 != FB_SUCCESS)
				status = FB_FAILURE;

			break;

		case COMMAND_START:
			// Test for use of the guardian. If so, start the guardian else start the server
			service = OpenService(manager, guard_service_name.c_str(), SERVICE_START);
			if (service)
			{
				CloseServiceHandle(service);

				status = SERVICES_start(manager, guard_service_name.c_str(),
										/*guard_display_name.c_str(),*/ sw_mode, svc_error);

				if (status == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully started.\n", guard_display_name.c_str());
				}
			}
			else
			{
				CloseServiceHandle(service);

				status = SERVICES_start(manager, remote_service_name.c_str(),
										/*remote_display_name.c_str(),*/ sw_mode, svc_error);

				if (status == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully started.\n", remote_display_name.c_str());
				}
			}
			break;

		case COMMAND_STOP:
			// Test for use of the guardian. If so, stop the guardian else stop the server
			service = OpenService(manager, guard_service_name.c_str(), SERVICE_STOP);
			if (service)
			{
				CloseServiceHandle(service);

				status = SERVICES_stop(manager, guard_service_name.c_str(),
									   /*guard_display_name.c_str(),*/ svc_error);

				if (status == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully stopped.\n", guard_display_name.c_str());
				}
			}
			else
			{
				CloseServiceHandle(service);

				status = SERVICES_stop(manager, remote_service_name.c_str(),
									   /*remote_display_name.c_str(),*/ svc_error);

				if (status == FB_SUCCESS)
				{
					printf("Service \"%s\" successfully stopped.\n", remote_display_name.c_str());
				}
			}
			break;

		case COMMAND_QUERY:
			if (svc_query_ex(manager) == FB_FAILURE)
			{
				svc_query(guard_service_name.c_str(), guard_display_name.c_str(), manager);
				svc_query(remote_service_name.c_str(), remote_display_name.c_str(), manager);
			}

			status = FB_SUCCESS;
			break;

		default:
			status = FB_SUCCESS;
	}

	CloseServiceHandle(manager);

	return (status == FB_SUCCESS) ? FINI_OK : FINI_ERROR;
}

static USHORT svc_query_ex(SC_HANDLE manager)
{
/**********************************************
 *
 *	s v c _ q u e r y _ e x
 *
 **********************************************
 *
 * Functional description
 *	Report (print) the status and configuration
 *  of all installed Firebird services.
 *  If none are installed return FB_FAILURE
 *  so as to allow a call to svc_query for the
 *  status of the default instance.
 *
 **********************************************/
	if (manager == NULL)
		return FB_FAILURE;

	DWORD lpServicesReturned = 0;
	DWORD lpResumeHandle = 0;
	DWORD pcbBytesNeeded = 0;
	USHORT rc = FB_FAILURE;

	EnumServicesStatus(manager, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0,
		&pcbBytesNeeded, &lpServicesReturned, &lpResumeHandle);

    if ( GetLastError() == ERROR_MORE_DATA )
	{
		const DWORD dwBytes = pcbBytesNeeded + sizeof(ENUM_SERVICE_STATUS);
		ENUM_SERVICE_STATUS* service_data = new ENUM_SERVICE_STATUS [dwBytes];
		EnumServicesStatus(manager, SERVICE_WIN32, SERVICE_STATE_ALL, service_data, dwBytes,
			&pcbBytesNeeded, &lpServicesReturned, &lpResumeHandle);

		if (lpServicesReturned == 0)
			delete[] service_data;
		else
		{
			Firebird::string serverServiceName;
			bool firebirdServicesInstalled = false;

			for ( DWORD i = 0; i < lpServicesReturned; i++ )
			{
				serverServiceName = service_data[i].lpServiceName;
				if ( serverServiceName.substr(0, 8) == "Firebird" )
				{
					svc_query(service_data[i].lpServiceName, service_data[i].lpDisplayName, manager);

					firebirdServicesInstalled = true;
				}
			}

			delete[] service_data;

			if ( firebirdServicesInstalled )
				rc = FB_SUCCESS;
			else
				printf("\nNo named Firebird service instances are installed.\n");
		}
	}

	return rc;
}


static void svc_query(const char* name, const char* display_name, SC_HANDLE manager)
{
/**************************************
 *
 *	s v c _ q u e r y
 *
 **************************************
 *
 * Functional description
 *	Report (print) the status and configuration of a service.
 *
 **************************************/
	if (manager == NULL)
		return;

	SC_HANDLE service = OpenService(manager, name, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);

	if (service)
	{
		printf("\n%s IS installed.\n", display_name);
		SERVICE_STATUS service_status;
		if (QueryServiceStatus(service, &service_status))
		{
			printf("  Status  : ");
			switch (service_status.dwCurrentState)
			{
				case SERVICE_STOPPED:
					printf("stopped\n");
					break;
				case SERVICE_START_PENDING:
					printf("starting\n");
					break;
				case SERVICE_STOP_PENDING:
					printf("stopping\n");
					break;
				case SERVICE_RUNNING:
					printf("running\n");
					break;
				default:
					printf("unknown state\n");
					break;
			}
		}
		else
			svc_error(GetLastError(), "QueryServiceStatus", NULL);

		ULONG uSize;
		QueryServiceConfig(service, NULL, 0, &uSize);
		QUERY_SERVICE_CONFIG* qsc = (QUERY_SERVICE_CONFIG*) new UCHAR[uSize];
		if (qsc && QueryServiceConfig(service, qsc, uSize, &uSize))
		{
			CharToOem(qsc->lpBinaryPathName, qsc->lpBinaryPathName);
			CharToOem(qsc->lpServiceStartName, qsc->lpServiceStartName);
			printf("  Path    : %s\n", qsc->lpBinaryPathName);
			printf("  Startup : ");
			switch (qsc->dwStartType)
			{
				case SERVICE_AUTO_START:
					printf("automatic\n");
					break;
				case SERVICE_DEMAND_START:
					printf("manual\n");
					break;
				case SERVICE_DISABLED:
					printf("disabled\n");
					break;
				default:
					printf("invalid setting\n");
					break;
			}
			if (! qsc->lpServiceStartName)
				printf("  Run as  : LocalSystem");
			else
				printf("  Run as  : %s", qsc->lpServiceStartName);

			if (qsc->dwServiceType & SERVICE_INTERACTIVE_PROCESS)
				printf(" (Interactive)\n");
			else
				printf("\n");
		}
		else
			svc_error(GetLastError(), "QueryServiceConfig", NULL);

		delete[] (UCHAR*) qsc;

		CloseServiceHandle(service);
	}
	else
		printf("\n%s is NOT installed.\n", display_name);

	return;
}

static USHORT svc_error( SLONG status, const TEXT* string, SC_HANDLE service)
{
/**************************************
 *
 *	s v c _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Report an error and punt.
 *
 **************************************/
	TEXT buffer[512];

	if (service != NULL)
		CloseServiceHandle(service);

	if (status == 0)
	{
		// Allows to report non System errors
		printf("%s\n", string);
	}
	else
	{
		printf("Error occurred during \"%s\".\n", string);

		if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							status,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							buffer,
							sizeof(buffer),
							NULL))
		{
			printf("Windows NT error %"SLONGFORMAT"\n", status);
		}
		else
		{
			CharToOem(buffer, buffer);
			printf("%s", buffer);	// '\n' is included in system messages
		}
	}
	return FB_FAILURE;
}

static void usage_exit()
{
/**************************************
 *
 *	u s a g e _ e x i t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	printf("\nUsage:\n");
	printf("  instsvc i[nstall] [ -s[uperserver]* | -c[lassic] ]\n");
	printf("                    [ -a[uto]* | -d[emand] ]\n");
	printf("                    [ -g[uardian] ]\n");
	printf("                    [ -l[ogin] username [password] ]\n");
	printf("                    [ -n[ame] instance ]\n");
	printf("                    [ -i[nteractive] ]\n\n");
	printf("          sta[rt]   [ -b[oostpriority] ]\n");
	printf("                    [ -n[ame] instance ]\n");
	printf("          sto[p]    [ -n[ame] instance ]\n");
	printf("          q[uery]\n");
	printf("          r[emove]  [ -n[ame] instance ]\n\n");
	printf("  This utility should be located and run from the root directory\n");
	printf("  of your Firebird installation.\n\n");
	printf("  '*' denotes the default values\n");
	printf("  '-z' can be used with any other option, prints version\n");
	printf("  'username' refers by default to a local account on this machine.\n");
	printf("  Use the format 'domain\\username' or 'server\\username' if appropriate.\n");

	exit(FINI_ERROR);
}
