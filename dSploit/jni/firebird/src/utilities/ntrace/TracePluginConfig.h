/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		TracePluginConfig.h
 *	DESCRIPTION:	Structure with plugin configuration parameters
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

#ifndef TRACEPLUGINCONFIG_H
#define TRACEPLUGINCONFIG_H

#include "../../common/classes/fb_string.h"

//enum LogFormat { lfText = 0, lfBinary = 1 };

struct TracePluginConfig
{
#define DATABASE_PARAMS
#define SERVICE_PARAMS

#define PATH_PARAMETER(NAME, VALUE) Firebird::PathName NAME;
#define STR_PARAMETER(NAME, VALUE) Firebird::string NAME;
#define BOOL_PARAMETER(NAME, VALUE) bool NAME;
#define UINT_PARAMETER(NAME, VALUE) ULONG NAME;
#include "paramtable.h"
#undef PATH_PARAMETER
#undef STR_PARAMETER
#undef BOOL_PARAMETER
#undef UINT_PARAMETER
	Firebird::PathName db_filename;

	// Default constructor. Pass pool to all string parameters, initialize everything to defaults
	TracePluginConfig() :
#define PATH_PARAMETER(NAME, VALUE) NAME(*getDefaultMemoryPool(), VALUE, strlen(VALUE)),
#define STR_PARAMETER(NAME, VALUE) NAME(*getDefaultMemoryPool(), VALUE, strlen(VALUE)),
#define BOOL_PARAMETER(NAME, VALUE) NAME(VALUE),
#define UINT_PARAMETER(NAME, VALUE) NAME(VALUE),
#include "paramtable.h"
#undef PATH_PARAMETER
#undef STR_PARAMETER
#undef BOOL_PARAMETER
#undef UINT_PARAMETER
		db_filename(*getDefaultMemoryPool())
		{ }

	// Copy constructor
	TracePluginConfig(const TracePluginConfig& from) :
#define PATH_PARAMETER(NAME, VALUE) NAME(*getDefaultMemoryPool(), from.NAME),
#define STR_PARAMETER(NAME, VALUE) NAME(*getDefaultMemoryPool(), from.NAME),
#define BOOL_PARAMETER(NAME, VALUE) NAME(from.NAME),
#define UINT_PARAMETER(NAME, VALUE) NAME(from.NAME),
#include "paramtable.h"
#undef PATH_PARAMETER
#undef STR_PARAMETER
#undef BOOL_PARAMETER
#undef UINT_PARAMETER
		db_filename(*getDefaultMemoryPool(), from.db_filename)
	{ }
private:
	// Not implemented
	TracePluginConfig& operator=(const TracePluginConfig& from);

#undef DATABASE_PARAMS
#undef SERVICE_PARAMS
};

#endif
