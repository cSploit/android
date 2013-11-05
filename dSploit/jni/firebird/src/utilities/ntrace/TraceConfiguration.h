/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceConfiguration.h
 *	DESCRIPTION:	Trace Configuration Reader
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef TRACE_CONFIGURATION
#define TRACE_CONFIGURATION

#include "firebird.h"
#include "../../common/classes/auto.h"
#include "../../common/classes/fb_string.h"
#include "../../config/ConfigFile.h"
#include "../../config/ConfObj.h"
#include "../../config/ConfObject.h"
#include "../../config/Element.h"
#include "../../config/AdminException.h"
#include "TracePluginConfig.h"

#include <sys/types.h>


class TraceCfgReader
{
public:
	static void readTraceConfiguration(const char* text, const Firebird::PathName& databaseName, TracePluginConfig& config);

private:
	struct MatchPos
	{
		int start;
		int end;
	};

private:
	TraceCfgReader(const char* text, const Firebird::PathName& databaseName, TracePluginConfig& config) :
		m_text(text),
		m_databaseName(databaseName),
		m_config(config)
	{}

	void readConfig();

	void expandPattern(const Element* el, Firebird::string& valueToExpand);
	bool parseBoolean(const Element* el) const;
	ULONG parseUInteger(const Element* el) const;

	const char* const m_text;
	const Firebird::PathName& m_databaseName;
	MatchPos m_subpatterns[10];
	TracePluginConfig& m_config;
};

#endif // TRACE_CONFIGURATION
