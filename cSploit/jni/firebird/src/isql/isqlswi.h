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
 *  The Original Code was created by Claudio Valderrama on 23-Jul-2009
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


#ifndef ISQL_ISQLSWI_H
#define ISQL_ISQLSWI_H

#include "../jrd/ibase.h"
#include "../jrd/constants.h"

enum isql_switches
{
	IN_SW_ISQL_0			= 0,
	IN_SW_ISQL_EXTRACTALL	= 1,
	IN_SW_ISQL_BAIL 		= 2,
	IN_SW_ISQL_CACHE		= 3,
	IN_SW_ISQL_CHARSET		= 4,
	IN_SW_ISQL_DATABASE 	= 5,
	IN_SW_ISQL_ECHO 		= 6,
	IN_SW_ISQL_EXTRACT		= 7,
	IN_SW_ISQL_FETCHPASS	= 8,
	IN_SW_ISQL_INPUT		= 9,
	IN_SW_ISQL_MERGE		= 10,
	IN_SW_ISQL_MERGE2		= 11,
	IN_SW_ISQL_NOAUTOCOMMIT = 12,
	IN_SW_ISQL_NODBTRIGGERS = 13,
	IN_SW_ISQL_NOWARN		= 14,
	IN_SW_ISQL_OUTPUT		= 15,
	IN_SW_ISQL_PAGE 		= 16,
	IN_SW_ISQL_PASSWORD 	= 17,
	IN_SW_ISQL_QUIET		= 18,
	IN_SW_ISQL_ROLE 		= 19,
	IN_SW_ISQL_ROLE2		= 20,
	IN_SW_ISQL_SQLDIALECT	= 21,
	IN_SW_ISQL_TERM 		= 22,
#ifdef TRUSTED_AUTH
	IN_SW_ISQL_TRUSTED		= 23,
#endif
	IN_SW_ISQL_USER 		= 24,
	IN_SW_ISQL_VERSION		= 25,
#ifdef DEV_BUILD
	IN_SW_ISQL_EXTRACTTBL	= 26,
#endif
	IN_SW_ISQL_HELP 		= 27
};


enum IsqlOptionType { iqoArgNone, iqoArgInteger, iqoArgString };

static const Switches::in_sw_tab_t isql_in_sw_table[] =
{
	{IN_SW_ISQL_EXTRACTALL	, 0, "ALL"				, 0, 0, 0, false, 11	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_BAIL 		, 0, "BAIL"				, 0, 0, 0, false, 104	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_CACHE		, 0, "CACHE"			, 0, 0, 0, false, 111	, 1, NULL, iqoArgInteger},
	{IN_SW_ISQL_CHARSET		, 0, "CHARSET"			, 0, 0, 0, false, 122	, 2, NULL, iqoArgString},
	{IN_SW_ISQL_DATABASE 	, 0, "DATABASE"			, 0, 0, 0, false, 123	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_ECHO 		, 0, "ECHO"				, 0, 0, 0, false, 124	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_EXTRACT		, 0, "EXTRACT"			, 0, 0, 0, false, 125	, 2, NULL, iqoArgNone},
	{IN_SW_ISQL_FETCHPASS	, 0, "FETCH_PASSWORD"	, 0, 0, 0, false, 161	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_INPUT		, 0, "INPUT"			, 0, 0, 0, false, 126	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_MERGE		, 0, "MERGE"			, 0, 0, 0, false, 127	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_MERGE2		, 0, "M2"				, 0, 0, 0, false, 128	, 2, NULL, iqoArgNone},
	{IN_SW_ISQL_NOAUTOCOMMIT, 0, "NOAUTOCOMMIT"		, 0, 0, 0, false, 129	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_NODBTRIGGERS, 0, "NODBTRIGGERS"		, 0, 0, 0, false, 154	, 3, NULL, iqoArgNone},
	{IN_SW_ISQL_NOWARN		, 0, "NOWARNINGS"		, 0, 0, 0, false, 130	, 3, NULL, iqoArgNone},
	{IN_SW_ISQL_OUTPUT		, 0, "OUTPUT"			, 0, 0, 0, false, 131	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_PAGE 		, 0, "PAGELENGTH"		, 0, 0, 0, false, 132	, 3, NULL, iqoArgInteger},
	{IN_SW_ISQL_PASSWORD 	, 0, "PASSWORD"			, 0, 0, 0, false, 133	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_QUIET		, 0, "QUIET"			, 0, 0, 0, false, 134	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_ROLE 		, 0, "ROLE"				, 0, 0, 0, false, 135	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_ROLE2		, 0, "R2"				, 0, 0, 0, false, 136	, 2, NULL, iqoArgString},
	{IN_SW_ISQL_SQLDIALECT	, 0, "SQLDIALECT"		, 0, 0, 0, false, 137	, 1, NULL, iqoArgInteger},
	{IN_SW_ISQL_SQLDIALECT	, 0, "SQL_DIALECT"		, 0, 0, 0, false, 0		, 1, NULL, iqoArgInteger},
	{IN_SW_ISQL_TERM 		, 0, "TERMINATOR"		, 0, 0, 0, false, 138	, 1, NULL, iqoArgString},
#ifdef TRUSTED_AUTH
	{IN_SW_ISQL_TRUSTED		, 0, "TRUSTED"			, 0, 0, 0, false, 155	, 2, NULL, iqoArgNone},
#endif
	{IN_SW_ISQL_USER 		, 0, "USER"				, 0, 0, 0, false, 139	, 1, NULL, iqoArgString},
	{IN_SW_ISQL_EXTRACT		, 0, "X"				, 0, 0, 0, false, 140	, 1, NULL, iqoArgNone},
#ifdef DEV_BUILD
	{IN_SW_ISQL_EXTRACTTBL	, 0, "XT"				, 0, 0, 0, false, 0		, 2, NULL, iqoArgNone},
#endif
	{IN_SW_ISQL_VERSION		, 0, "Z"				, 0, 0, 0, false, 141	, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_HELP 		, 0, "?"				, 0, 0, 0, false, 0		, 1, NULL, iqoArgNone},
	{IN_SW_ISQL_0			, 0, NULL				, 0, 0, 0, false, 0		, 0, NULL, iqoArgNone}
};

#endif // ISQL_ISQLSWI_H
