/*
 *	PROGRAM:		Windows Win32 Client Libraries Installation
 *	MODULE:			install_client.cpp
 *	DESCRIPTION:	Program which install the FBCLIENT.DLL or GDS32.DLL
 *
 *  The contents of this file are subject to the Initial Developer's
 *  Public License Version 1.0 (the "License"); you may not use this
 *  file except in compliance with the License. You may obtain a copy
 *  of the License here:
 *
 *    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code is (C) 2003 Olivier Mascia.
 *
 *  The Initial Developer of the Original Code is Olivier Mascia.
 *
 *  All Rights Reserved.
 *
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "../jrd/license.h"
#include "../utilities/install/install_nt.h"
#include "../utilities/install/install_proto.h"

static USHORT inst_error(ULONG, const TEXT*);
static void usage_exit();

static const struct
{
	const TEXT* name;
	USHORT abbrev;
	USHORT code;
} commands[] =
{
	{"INSTALL", 1, COMMAND_INSTALL},
	{"REMOVE", 1, COMMAND_REMOVE},
	{"QUERY", 1, COMMAND_QUERY},
	{NULL, 0, 0}
};

static const struct
{
	const TEXT* name;
	USHORT abbrev;
	USHORT code;
} clients[] =
{
	{"FBCLIENT", 1, CLIENT_FB},
	{"GDS32", 1, CLIENT_GDS},
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
 *	Installs the FBCLIENT.DLL or GDS32.DLL in the Windows system directory.
 *
 **************************************/

	USHORT sw_command = COMMAND_NONE;
	USHORT sw_client = CLIENT_NONE;
	bool sw_force = false;
	bool sw_version = false;

	// Let's get the root directory from the instance path of this program.
	// argv[0] is only _mostly_ guaranteed to give this info,
	// so we GetModuleFileName()
	TEXT directory[MAXPATHLEN];
	USHORT len = GetModuleFileName(NULL, directory, sizeof(directory));
	if (len == 0)
		return inst_error(GetLastError(), "GetModuleFileName");

	// Get to the last '\' (this one precedes the filename part). There is
	// always one after a call to GetModuleFileName().
	TEXT* p = directory + len;
	do {--p;} while (*p != '\\');

/*	Instclient no longer strips the bin\\ part. This section can be removed after fb2.1.0 beta2
	// Get to the previous '\' (this one should precede the supposed 'bin\\' part).
	// There is always an additional '\' OR a ':'.
	do {--p;} while (*p != '\\' && *p != ':');
*/
	*p = '\0';

	const TEXT* const* const end = argv + argc;
	while (++argv < end)
	{
		if (**argv != '-')
		{
			if (sw_command == COMMAND_NONE)
			{
				const TEXT* cmd;
				int i;
				for (i = 0; cmd = commands[i].name; i++)
				{
					const TEXT* q;
					for (p = *argv, q = cmd; *p && UPPER(*p) == *q; p++, q++);
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
				const TEXT* cln;
				int i;
				for (i = 0; cln = clients[i].name; i++)
				{
					const TEXT* q;
					for (p = *argv, q = cln; *p && UPPER(*p) == *q; p++, q++);
					if (!*p && clients[i].abbrev <= (USHORT) (q - cln))
						break;
				}
				if (!cln)
				{
					printf("Unknown library \"%s\"\n", *argv);
					usage_exit();
				}
				sw_client = clients[i].code;
			}
		}
		else
		{
			p = *argv + 1;
			switch (UPPER(*p))
			{
				case 'F':
					sw_force = true;
					break;

				case 'Z':
					sw_version = true;
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
		printf("instclient version %s\n", FB_VERSION);

	if (sw_command == COMMAND_NONE || sw_client == CLIENT_NONE)
	{
		usage_exit();
	}

	const TEXT* clientname = (sw_client == CLIENT_GDS) ? GDS32_NAME : FBCLIENT_NAME;

	USHORT status;
	switch (sw_command)
	{

		case COMMAND_INSTALL:
			status = CLIENT_install(directory, sw_client, sw_force, inst_error);
			switch (status)
			{
				case FB_INSTALL_SAME_VERSION_FOUND :
					printf("Existing %s (same version) found.\n", clientname);
					printf("No update needed.\n");
					break;
				case FB_INSTALL_NEWER_VERSION_FOUND :
					printf("Existing %s (newer version) found.\n", clientname);
					printf("You can force replacing the DLL with -f[orce] switch.\n");
					printf("Though this could break some other InterBase(R) "
						"or Firebird installation.\n");
					break;
				case FB_INSTALL_COPY_REQUIRES_REBOOT :
					printf("%s has been scheduled for installation "
						"at the next system reboot.\n", clientname);
					break;
				case FB_SUCCESS :
					printf("%s has been installed to the "
						"System directory.\n", clientname);
					break;
			}
			break;

		case COMMAND_REMOVE:
			status = CLIENT_remove(directory, sw_client, sw_force, inst_error);
			switch (status)
			{
				case FB_INSTALL_FILE_NOT_FOUND :
					printf("%s was not found in the System directory.\n",
						clientname);
					break;
				case FB_INSTALL_CANT_REMOVE_ALIEN_VERSION :
					printf("The installed %s appears to be from an "
						"unsupported version.\n", clientname);
					printf("It probably belongs to another version of "
						"Firebird or InterBase(R).\n");
					break;
				case FB_INSTALL_FILE_PROBABLY_IN_USE :
					printf("The %s can't be removed. It is probably "
						"currently in use.\n", clientname);
					break;
				case FB_SUCCESS :
					printf("The %s has been removed from the "
						"System directory.\n", clientname);
					break;
			}
			break;

		case COMMAND_QUERY:
			ULONG verMS, verLS, sharedCount;
			status = CLIENT_query(sw_client, verMS, verLS, sharedCount, inst_error);
			switch (status)
			{
				case FB_INSTALL_FILE_NOT_FOUND :
					printf("%s was not found in the System directory.\n",
						clientname);
					break;
				case FB_SUCCESS :
					if (sharedCount)
					{
						printf("Installed %s version : %u.%u.%u.%u "
							"(shared DLL count %d)\n",
							clientname,
							verMS >> 16, verMS & 0x0000ffff,
							verLS >> 16, verLS & 0x0000ffff,
							sharedCount);
					}
					else
					{
						printf("Installed %s version : %u.%u.%u.%u\n",
							clientname,
							verMS >> 16, verMS & 0x0000ffff,
							verLS >> 16, verLS & 0x0000ffff);
					}
					break;
			}
			break;
	}

	switch (status)
	{
		case FB_INSTALL_COPY_REQUIRES_REBOOT :
			return -FB_INSTALL_COPY_REQUIRES_REBOOT;
		case FB_SUCCESS :
			return FINI_OK;
		default :
			return FINI_ERROR;
	}
}

static USHORT inst_error(ULONG status, const TEXT* string)
{
/**************************************
 *
 *	i n s t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Report an error and punt.
 *
 **************************************/
	TEXT buffer[512];

	if (status == 0)
	{
		// Allows to report non System errors
		printf("%s\n", string);
	}
	else
	{
		printf("Error %u occurred during \"%s\".\n", status, string);

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

	printf("Usage:\n");
	printf("  instclient i[nstall] [ -f[orce] ] library\n");
	printf("             q[uery] library\n");
	printf("             r[emove] library\n");
	printf("\n");
	printf("  where library is:  f[bclient] | g[ds32] \n");
	printf("\n");
	printf("  This utility should be located and run from the 'bin' directory\n");
	printf("  of your Firebird installation.\n");
	printf("  '-z' can be used with any other option, prints version\n");
	printf("\n");
	printf("Purpose:\n");
	printf("  This utility manages deployment of the Firebird client library \n");
	printf("  into the Windows system directory. It caters for two installation\n");
	printf("  scenarios:\n");
	printf("\n");
	printf("    Deployment of the native fbclient.dll.\n");
	printf("    Deployment of gds32.dll to support legacy applications.\n");
	printf("\n");
	printf("  Version information and shared library counts are handled \n");
	printf("  automatically. You may provide the -f[orce] option to override\n");
	printf("  version checks.\n");
	printf("\n");
	printf("  Please, note that if you -f[orce] the installation, you might have\n");
	printf("  to reboot the machine in order to finalize the copy and you might\n");
	printf("  break some other Firebird or InterBase(R) version on the system.\n");

	exit(FINI_ERROR);
}

