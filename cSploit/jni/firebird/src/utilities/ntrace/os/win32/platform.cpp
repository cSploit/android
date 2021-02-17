/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		platform.cpp
 *	DESCRIPTION:	Platform specifics (Win32)
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include <windows.h>
#include "../platform.h"
#include "../common/classes/fb_tls.h"

TLS_DECLARE(char*, error_string);

const char* get_error_string()
{
	return TLS_GET(error_string);
}

void set_error_string(const char* str)
{
	char* org_str = TLS_GET(error_string);
	if (org_str)
	{
		LocalFree(org_str);
		TLS_SET(error_string, NULL);
	}

	if (str)
	{
		const size_t size = strlen(str) + 1;
		char* new_str = (char*) LocalAlloc(LMEM_FIXED, size);
		if (new_str)
		{
			memcpy(new_str, str, size);
			TLS_SET(error_string, new_str);
		}
	}
}

SLONG get_process_id()
{
	return GetCurrentProcessId();
}


BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
	if (fdwReason == DLL_THREAD_DETACH)
	{
		char* str = TLS_GET(error_string);
		if (str)
		{
			LocalFree(str);
			TLS_SET(error_string, NULL);
		}
	}
	return TRUE;
}
