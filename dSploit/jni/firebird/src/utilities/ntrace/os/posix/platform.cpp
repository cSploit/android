/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		TracePluginImpl.h
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
#include "../platform.h"
#include "../common/classes/fb_tls.h"
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#define TEST
//#undef TEST

#ifndef TEST
class MallocClear
{
public:
	static void clear(char* error_string)
	{
		free(error_string);
	}
};

Firebird::TlsValue<char*, MallocClear> error_value;

const char* get_error_string()
{
	return error_value.get();
}

void set_error_string(const char* str)
{
	char* org_str = error_value.get();
	if (org_str)
	{
		free(org_str);
		error_value.set(NULL);
	}
	if (str)
	{
		const size_t size = strlen(str) + 1;
		char* new_str = (char*) malloc(size);
		if (new_str)
		{
			memcpy(new_str, str, size);
			error_value.set(new_str);
		}
	}
}
#else
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
		free(org_str);
		TLS_SET(error_string, NULL);
	}
	if (str)
	{
		const size_t size = strlen(str) + 1;
		char* new_str = (char*) malloc(size);
		if (new_str)
		{
			memcpy(new_str, str, size);
			TLS_SET(error_string, new_str);
		}
	}
}
#endif

SLONG get_process_id()
{
	return getpid();
}
