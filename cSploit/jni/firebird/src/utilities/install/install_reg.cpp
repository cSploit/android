/*
 *	PROGRAM:		Windows NT registry installation program
 *	MODULE:			install_reg.cpp
 *	DESCRIPTION:	Registry installation program
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
#include "../jrd/license.h"
#include "../utilities/install/install_nt.h"
#include "../utilities/install/regis_proto.h"

static USHORT reg_error(SLONG, const TEXT*, HKEY);
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
	{NULL, 0, 0}
};


int CLIB_ROUTINE main( int argc, char** argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Install or remove Firebird.
 *
 **************************************/
	TEXT directory[MAXPATHLEN];

	USHORT sw_command = COMMAND_NONE;
	bool sw_version = false;

	// Let's get the root directory from the instance path of this program.
	// argv[0] is only _mostly_ guaranteed to give this info,
	// so we GetModuleFileName()
	const USHORT len = GetModuleFileName(NULL, directory, sizeof(directory));
	if (len == 0)
		return reg_error(GetLastError(), "GetModuleFileName", NULL);

	// Get to the last '\' (this one precedes the filename part). There is
	// always one after a call to GetModuleFileName().
	TEXT* p = directory + len;
	do {--p;} while (*p != '\\');

	*p = '\0';

	const TEXT* const* const end = argv + argc;
	while (++argv < end)
	{
		if (**argv != '-')
		{
			const TEXT* cmd;
			USHORT i;
			for (i = 0; cmd = commands[i].name; i++)
			{
				const TEXT* q = cmd;
				for (p = *argv; *p && UPPER(*p) == *q; ++p, ++q)
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
		printf("instreg version %s\n", FB_VERSION);

	if (sw_command == COMMAND_NONE)
		usage_exit();

	HKEY hkey_node = HKEY_LOCAL_MACHINE;

	USHORT ret;
	switch (sw_command)
	{
		case COMMAND_INSTALL:
			ret = REGISTRY_install(hkey_node, directory, reg_error);
			if (ret != FB_SUCCESS)
				printf ("Firebird has not been installed in the registry.\n");
			else
				printf("Firebird has been successfully installed in the registry.\n");
			break;

		case COMMAND_REMOVE:
			ret = REGISTRY_remove(hkey_node, false, reg_error);
			if (ret != FB_SUCCESS)
				printf("Firebird has not been deleted from the registry.\n");
			else
				printf("Firebird has been successfully deleted from the registry.\n");
			break;

		default:
			ret = FB_SUCCESS;
	}

	if (hkey_node != HKEY_LOCAL_MACHINE)
		RegCloseKey(hkey_node);

	return (ret == FB_SUCCESS) ? FINI_OK : FINI_ERROR;
}

static USHORT reg_error( SLONG status, const TEXT* string, HKEY hkey)
{
/**************************************
 *
 *	r e g _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Report an error and punt.
 *
 **************************************/
	TEXT buffer[512];

	if (hkey != NULL && hkey != HKEY_LOCAL_MACHINE)
		RegCloseKey(hkey);

	if (status == 0)
	{
		// Allows to report non System errors
		printf("%s\n", string);
	}
	else
	{
		printf("Error occurred during \"%s\"\n", string);

		const SSHORT l = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								status,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								buffer,
								sizeof(buffer),
								NULL);
		if (!l) {
			printf("Windows NT error %"SLONGFORMAT"\n", status);
		}
		else
		{
			CharToOem(buffer, buffer);
			printf("%s\n", buffer);
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
	printf("  instreg i[nstall]\n");
	printf("          r[emove]\n\n");
	printf("  This utility should be located and run from the root directory\n");
	printf("  of your Firebird installation.\n\n");
	printf("  '-z' can be used with any other option, prints version\n");

	exit(FINI_ERROR);
}

