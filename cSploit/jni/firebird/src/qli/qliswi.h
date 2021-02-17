/*
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
 *  The Original Code was created by Claudio Valderrama on 15-Jul-2009
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


#ifndef QLI_QLISWI_H
#define QLI_QLISWI_H

#include "../jrd/ibase.h"
#include "../jrd/constants.h"

enum qli_switches
{
	IN_SW_QLI_0					= 0,
	IN_SW_QLI_APP_SCRIPT		= 1,
	IN_SW_QLI_BUFFERS			= 2,
	IN_SW_QLI_FETCH_PASSWORD	= 3,
	IN_SW_QLI_INITIAL_SCRIPT	= 4,
	IN_SW_QLI_NOBANNER			= 5,
	IN_SW_QLI_PASSWORD			= 6,
	IN_SW_QLI_TRACE				= 7,
#ifdef TRUSTED_AUTH
	IN_SW_QLI_TRUSTED_AUTH		= 8,
#endif
	IN_SW_QLI_USER				= 9,
	IN_SW_QLI_VERIFY			= 10,
	IN_SW_QLI_X					= 11,	// Internal switch: do not present in help
	IN_SW_QLI_Y					= 12,	// Internal switch: do not present in help
	IN_SW_QLI_Z					= 13,
	IN_SW_QLI_HELP				= 14,
	IN_SW_QLI_NODBTRIGGERS		= 15
};


static const Switches::in_sw_tab_t qli_in_sw_table[] =
{
	{IN_SW_QLI_APP_SCRIPT		, 0, "APPSCRIPT"		, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_APP_SCRIPT		, 0, "APP_SCRIPT"		, 0, 0, 0, false, 516,	1, NULL},
	{IN_SW_QLI_BUFFERS			, 0, "BUFFERS"			, 0, 0, 0, false, 517,	1, NULL},
	{IN_SW_QLI_FETCH_PASSWORD	, 0, "FETCH_PASSWORD"	, 0, 0, 0, false, 518,	1, NULL},
	{IN_SW_QLI_INITIAL_SCRIPT	, 0, "INITSCRIPT"		, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_INITIAL_SCRIPT	, 0, "INIT_SCRIPT"		, 0, 0, 0, false, 519,	1, NULL},
	{IN_SW_QLI_NODBTRIGGERS		, 0, "NODBTRIGGERS"		, 0, 0, 0, false, 531,	3, NULL},
	{IN_SW_QLI_NOBANNER			, 0, "NOBANNER"			, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_NOBANNER			, 0, "NO_BANNER"		, 0, 0, 0, false, 520,	1, NULL},
	{IN_SW_QLI_PASSWORD			, 0, "PASSWORD"			, 0, 0, 0, false, 521,	1, NULL},
	{IN_SW_QLI_TRACE			, 0, "TRACE"			, 0, 0, 0, false, 522,	3, NULL},
#ifdef TRUSTED_AUTH
	{IN_SW_QLI_TRUSTED_AUTH		, 0, "TRUSTED_AUTH"		, 0, 0, 0, false, 523,	3, NULL},
#endif
	{IN_SW_QLI_USER				, 0, "USER"				, 0, 0, 0, false, 524,	1, NULL},
	{IN_SW_QLI_VERIFY			, 0, "VERIFY"			, 0, 0, 0, false, 525,	1, NULL},
	{IN_SW_QLI_X				, 0, "X"				, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_Y				, 0, "Y"				, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_Z				, 0, "Z"				, 0, 0, 0, false, 526,	1, NULL},
	{IN_SW_QLI_HELP				, 0, "?"				, 0, 0, 0, false, 0,	1, NULL},
	{IN_SW_QLI_0				, 0, NULL				, 0, 0, 0, false, 0,	0, NULL}
};

#endif // QLI_QLISWI_H
