/*
 *	PROGRAM:	JRD International support
 *	MODULE:		IntlManager.cpp
 *	DESCRIPTION:	INTL Manager
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/IntlManager.h"
#include "../jrd/os/mod_loader.h"
#include "../jrd/intlobj_new.h"
#include "../jrd/intl_proto.h"
#include "../common/config/config.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/init.h"

#include "../config/ConfigFile.h"
#include "../config/ConfObj.h"
#include "../config/ConfObject.h"
#include "../config/Element.h"
#include "../config/ScanDir.h"
#include "../config/AdminException.h"

using namespace Firebird;


namespace
{
	class ModulesMap : public GenericMap<Pair<Left<PathName, ModuleLoader::Module*> > >
	{
	public:
		explicit ModulesMap(MemoryPool& p)
			: GenericMap<Pair<Left<PathName, ModuleLoader::Module*> > >(p)
		{
		}

		~ModulesMap()
		{
			// unload modules
			Accessor accessor(this);
			for (bool found = accessor.getFirst(); found; found = accessor.getNext())
				delete accessor.current()->second;
		}
	};
}

namespace Jrd {


struct ExternalInfo
{
	ExternalInfo(const PathName& a_moduleName, const string& a_name, const string& a_configInfo)
		: moduleName(a_moduleName),
		  name(a_name),
		  configInfo(a_configInfo)
	{
	}

	ExternalInfo(MemoryPool& p, const ExternalInfo& externalInfo)
		: moduleName(p, externalInfo.moduleName),
		  name(p, externalInfo.name),
		  configInfo(p, externalInfo.configInfo)
	{
	}

	ExternalInfo()
	{
	}

	PathName moduleName;
	string name;
	string configInfo;
};


static GlobalPtr<ModulesMap> modules;
static GlobalPtr<GenericMap<Pair<Full<string, ExternalInfo> > > > charSetCollations;


const IntlManager::CharSetDefinition IntlManager::defaultCharSets[] =
{
	{"NONE", CS_NONE, 1},
	{"OCTETS", CS_BINARY, 1},
	{"ASCII", CS_ASCII, 1},
	{"UNICODE_FSS", CS_UNICODE_FSS, 3},
	{"UTF8", CS_UTF8, 4},
#ifdef FB_NEW_INTL_ALLOW_NOT_READY
	{"UTF16", CS_UTF16, 4},
	{"UTF32", CS_UTF32, 4},
#endif	// FB_NEW_INTL_NOT_READY
	{"SJIS_0208", CS_SJIS, 2},
	{"EUCJ_0208", CS_EUCJ, 2},
	{"DOS437", CS_DOS_437, 1},
	{"DOS850", CS_DOS_850, 1},
	{"DOS865", CS_DOS_865, 1},
	{"ISO8859_1", CS_ISO8859_1, 1},
	{"ISO8859_2", CS_ISO8859_2, 1},
	{"ISO8859_3", CS_ISO8859_3, 1},
	{"ISO8859_4", CS_ISO8859_4, 1},
	{"ISO8859_5", CS_ISO8859_5, 1},
	{"ISO8859_6", CS_ISO8859_6, 1},
	{"ISO8859_7", CS_ISO8859_7, 1},
	{"ISO8859_8", CS_ISO8859_8, 1},
	{"ISO8859_9", CS_ISO8859_9, 1},
	{"ISO8859_13", CS_ISO8859_13, 1},
	{"DOS852", CS_DOS_852, 1},
	{"DOS857", CS_DOS_857, 1},
	{"DOS860", CS_DOS_860, 1},
	{"DOS861", CS_DOS_861, 1},
	{"DOS863", CS_DOS_863, 1},
	{"CYRL", CS_CYRL, 1},
	{"DOS737", CS_DOS_737, 1},
	{"DOS775", CS_DOS_775, 1},
	{"DOS858", CS_DOS_858, 1},
	{"DOS862", CS_DOS_862, 1},
	{"DOS864", CS_DOS_864, 1},
	{"DOS866", CS_DOS_866, 1},
	{"DOS869", CS_DOS_869, 1},
	{"WIN1250", CS_WIN1250, 1},
	{"WIN1251", CS_WIN1251, 1},
	{"WIN1252", CS_WIN1252, 1},
	{"WIN1253", CS_WIN1253, 1},
	{"WIN1254", CS_WIN1254, 1},
	{"NEXT", CS_NEXT, 1},
	{"WIN1255", CS_WIN1255, 1},
	{"WIN1256", CS_WIN1256, 1},
	{"WIN1257", CS_WIN1257, 1},
	{"KSC_5601", CS_KSC5601, 2},
	{"BIG_5", CS_BIG5, 2},
	{"GB_2312", CS_GB2312, 2},
	{"KOI8R", CS_KOI8R, 1},
	{"KOI8U", CS_KOI8U, 1},
	{"WIN1258", CS_WIN1258, 1},
	{"TIS620", CS_TIS620, 1},
	{"GBK", CS_GBK, 2},
	{"CP943C", CS_CP943C, 2},
	{"GB18030", CS_GB18030, 4},
	{NULL, 0, 0}
};

const IntlManager::CharSetAliasDefinition IntlManager::defaultCharSetAliases[] =
{
	{"BINARY", CS_BINARY},
	{"USASCII", CS_ASCII},
	{"ASCII7", CS_ASCII},
	{"UTF_FSS", CS_UNICODE_FSS},
	{"SQL_TEXT", CS_UNICODE_FSS},
	{"UTF-8", CS_UTF8},
#ifdef FB_NEW_INTL_ALLOW_NOT_READY
	{"UTF-16", CS_UTF16},
	{"UTF-32", CS_UTF32},
#endif	// FB_NEW_INTL_NOT_READY
	{"SJIS", CS_SJIS},
	{"EUCJ", CS_EUCJ},
	{"DOS_437", CS_DOS_437},
	{"DOS_850", CS_DOS_850},
	{"DOS_865", CS_DOS_865},
	{"ISO8859_1", CS_ISO8859_1},
	{"ISO88591", CS_ISO8859_1},
	{"LATIN1", CS_ISO8859_1},
	{"ANSI", CS_ISO8859_1},
	{"ISO8859_2", CS_ISO8859_2},
	{"ISO88592", CS_ISO8859_2},
	{"LATIN2", CS_ISO8859_2},
	{"ISO-8859-2", CS_ISO8859_2},	// Prefered MIME name
	{"ISO8859_3", CS_ISO8859_3},
	{"ISO88593", CS_ISO8859_3},
	{"LATIN3", CS_ISO8859_3},
	{"ISO-8859-3", CS_ISO8859_3},	// Prefered MIME name
	{"ISO8859_4", CS_ISO8859_4},
	{"ISO88594", CS_ISO8859_4},
	{"LATIN4", CS_ISO8859_4},
	{"ISO-8859-4", CS_ISO8859_4},	// Prefered MIME name
	{"ISO8859_5", CS_ISO8859_5},
	{"ISO88595", CS_ISO8859_5},
	{"ISO-8859-5", CS_ISO8859_5},	// Prefered MIME name
	{"ISO8859_6", CS_ISO8859_6},
	{"ISO88596", CS_ISO8859_6},
	{"ISO-8859-6", CS_ISO8859_6},	// Prefered MIME name
	{"ISO8859_7", CS_ISO8859_7},
	{"ISO88597", CS_ISO8859_7},
	{"ISO-8859-7", CS_ISO8859_7},	// Prefered MIME name
	{"ISO8859_8", CS_ISO8859_8},
	{"ISO88598", CS_ISO8859_8},
	{"ISO-8859-8", CS_ISO8859_8},	// Prefered MIME name
	{"ISO8859_9", CS_ISO8859_9},
	{"ISO88599", CS_ISO8859_9},
	{"LATIN5", CS_ISO8859_9},
	{"ISO-8859-9", CS_ISO8859_9},	// Prefered MIME name
	{"ISO8859_13", CS_ISO8859_13},
	{"ISO885913", CS_ISO8859_13},
	{"LATIN7", CS_ISO8859_13},
	{"ISO-8859-13", CS_ISO8859_13},	// Prefered MIME name
	{"DOS_852", CS_DOS_852},
	{"DOS_857", CS_DOS_857},
	{"DOS_860", CS_DOS_860},
	{"DOS_861", CS_DOS_861},
	{"DOS_863", CS_DOS_863},
	{"DOS_737", CS_DOS_737},
	{"DOS_775", CS_DOS_775},
	{"DOS_858", CS_DOS_858},
	{"DOS_862", CS_DOS_862},
	{"DOS_864", CS_DOS_864},
	{"DOS_866", CS_DOS_866},
	{"DOS_869", CS_DOS_869},
	{"WIN_1250", CS_WIN1250},
	{"WIN_1251", CS_WIN1251},
	{"WIN_1252", CS_WIN1252},
	{"WIN_1253", CS_WIN1253},
	{"WIN_1254", CS_WIN1254},
	{"WIN_1255", CS_WIN1255},
	{"WIN_1256", CS_WIN1256},
	{"WIN_1257", CS_WIN1257},
	{"WIN_1258", CS_WIN1258},
	{"KSC5601", CS_KSC5601},
	{"DOS_949", CS_KSC5601},
	{"WIN_949", CS_KSC5601},
	{"BIG5", CS_BIG5},
	{"DOS_950", CS_BIG5},
	{"WIN_950", CS_BIG5},
	{"GB2312", CS_GB2312},
	{"DOS_936", CS_GB2312},
	{"WIN_936", CS_GB2312},
	{NULL, 0}
};

const IntlManager::CollationDefinition IntlManager::defaultCollations[] =
{
	{CS_NONE, 0, "NONE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_BINARY, 0, "OCTETS", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ASCII, 0, "ASCII", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UNICODE_FSS, 0, "UNICODE_FSS", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF8, 0, "UTF8", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF8, 1, "UCS_BASIC", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF8, 2, "UNICODE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF8, 3, "UNICODE_CI", "UNICODE",
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE, NULL},
	{CS_UTF8, 4, "UNICODE_CI_AI", "UNICODE",
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		NULL},
#ifdef FB_NEW_INTL_ALLOW_NOT_READY
	{CS_UTF16, 0, "UTF16", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF16, 1, "UCS_BASIC", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF32, 0, "UTF32", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_UTF32, 1, "UCS_BASIC", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
#endif	// FB_NEW_INTL_NOT_READY
	{CS_SJIS, 0, "SJIS_0208", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_EUCJ, 0, "EUCJ_0208", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 0, "DOS437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 1, "PDOX_ASCII", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 2, "PDOX_INTL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 3, "PDOX_SWEDFIN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 4, "DB_DEU437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 5, "DB_ESP437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 6, "DB_FIN437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 7, "DB_FRA437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 8, "DB_ITA437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 9, "DB_NLD437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 10, "DB_SVE437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 11, "DB_UK437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_437, 12, "DB_US437", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 0, "DOS850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 1, "DB_FRC850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 2, "DB_DEU850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 3, "DB_ESP850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 4, "DB_FRA850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 5, "DB_ITA850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 6, "DB_NLD850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 7, "DB_PTB850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 8, "DB_SVE850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 9, "DB_UK850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_850, 10, "DB_US850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_865, 0, "DOS865", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_865, 1, "PDOX_NORDAN4", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_865, 2, "DB_DAN865", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_865, 3, "DB_NOR865", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 0, "ISO8859_1", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 1, "DA_DA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 2, "DU_NL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 3, "FI_FI", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 4, "FR_FR", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 5, "FR_CA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 6, "DE_DE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 7, "IS_IS", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 8, "IT_IT", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 9, "NO_NO", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 10, "ES_ES", NULL, TEXTTYPE_ATTR_PAD_SPACE, "DISABLE-COMPRESSIONS=1;SPECIALS-FIRST=1"},
	{CS_ISO8859_1, 11, "SV_SV", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 12, "EN_UK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 14, "EN_US", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 15, "PT_PT", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_1, 16, "PT_BR", NULL,
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		NULL},
	{CS_ISO8859_1, 17, "ES_ES_CI_AI", NULL,
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		"DISABLE-COMPRESSIONS=1;SPECIALS-FIRST=1"},
	{CS_ISO8859_1, 18, "FR_FR_CI_AI", "FR_FR",
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		"SPECIALS-FIRST=1"},
	{CS_ISO8859_2, 0, "ISO8859_2", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_2, 1, "CS_CZ", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_2, 2, "ISO_HUN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_2, 3, "ISO_PLK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_3, 0, "ISO8859_3", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_4, 0, "ISO8859_4", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_5, 0, "ISO8859_5", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_6, 0, "ISO8859_6", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_7, 0, "ISO8859_7", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_8, 0, "ISO8859_8", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_9, 0, "ISO8859_9", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_13, 0, "ISO8859_13", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_ISO8859_13, 1, "LT_LT", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 0, "DOS852", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 1, "DB_CSY", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 2, "DB_PLK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 4, "DB_SLO", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 5, "PDOX_CSY", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 6, "PDOX_PLK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 7, "PDOX_HUN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_852, 8, "PDOX_SLO", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_857, 0, "DOS857", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_857, 1, "DB_TRK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_860, 0, "DOS860", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_860, 1, "DB_PTG860", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_861, 0, "DOS861", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_861, 1, "PDOX_ISL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_863, 0, "DOS863", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_863, 1, "DB_FRC863", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_CYRL, 0, "CYRL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_CYRL, 1, "DB_RUS", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_CYRL, 2, "PDOX_CYRL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_737, 0, "DOS737", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_775, 0, "DOS775", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_858, 0, "DOS858", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_862, 0, "DOS862", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_864, 0, "DOS864", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_866, 0, "DOS866", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_DOS_869, 0, "DOS869", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 0, "WIN1250", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 1, "PXW_CSY", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 2, "PXW_HUNDC", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 3, "PXW_PLK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 4, "PXW_SLOV", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 5, "PXW_HUN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 6, "BS_BA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1250, 7, "WIN_CZ", NULL, TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE, NULL},
	{CS_WIN1250, 8, "WIN_CZ_CI_AI", NULL,
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		NULL},
	{CS_WIN1251, 0, "WIN1251", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1251, 1, "PXW_CYRL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1251, 2, "WIN1251_UA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 0, "WIN1252", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 1, "PXW_INTL", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 2, "PXW_INTL850", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 3, "PXW_NORDAN4", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 4, "PXW_SPAN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 5, "PXW_SWEDFIN", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1252, 6, "WIN_PTBR", NULL,
		TEXTTYPE_ATTR_PAD_SPACE | TEXTTYPE_ATTR_CASE_INSENSITIVE | TEXTTYPE_ATTR_ACCENT_INSENSITIVE,
		NULL},
	{CS_WIN1253, 0, "WIN1253", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1253, 1, "PXW_GREEK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1254, 0, "WIN1254", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1254, 1, "PXW_TURK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 0, "NEXT", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 1, "NXT_US", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 2, "NXT_DEU", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 3, "NXT_FRA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 4, "NXT_ITA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_NEXT, 5, "NXT_ESP", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1255, 0, "WIN1255", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1256, 0, "WIN1256", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1257, 0, "WIN1257", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1257, 1, "WIN1257_EE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1257, 2, "WIN1257_LT", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1257, 3, "WIN1257_LV", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KSC5601, 0, "KSC_5601", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KSC5601, 1, "KSC_DICTIONARY", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_BIG5, 0, "BIG_5", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_GB2312, 0, "GB_2312", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KOI8R, 0, "KOI8R", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KOI8R, 1, "KOI8R_RU", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KOI8U, 0, "KOI8U", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_KOI8U, 1, "KOI8U_UA", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_WIN1258, 0, "WIN1258", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_TIS620, 0, "TIS620", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_TIS620, 1, "TIS620_UNICODE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_GBK, 0, "GBK", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_GBK, 1, "GBK_UNICODE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_CP943C, 0, "CP943C", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_CP943C, 1, "CP943C_UNICODE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_GB18030, 0, "GB18030", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{CS_GB18030, 1, "GB18030_UNICODE", NULL, TEXTTYPE_ATTR_PAD_SPACE, NULL},
	{0, 0, NULL, NULL, 0, NULL}
};


bool IntlManager::initialize()
{
	bool ok = true;
	ObjectsArray<string> conflicts;
	string builtinConfig;

	Firebird::PathName intlPath = fb_utils::getPrefix(fb_utils::FB_DIR_INTL, "");

	ScanDir dir(intlPath.c_str(), "*.conf");

	try
	{
		while (dir.next())
		{
			ConfigFile configFile(dir.getFilePath(), ConfigFile::LEX_none);

			ConfObj builtinModule(configFile.findObject("intl_module", "builtin"));
			string s = getConfigInfo(builtinModule);
			if (s.hasData())
				builtinConfig = s;

			for (const Element* el = configFile.getObjects()->children; el; el = el->sibling)
			{
				if (el->name == "charset")
				{
					const string charSetName = el->getAttributeName(0);
					PathName filename;
					string configInfo;

					const Element* module = el->findChild("intl_module");
					if (module)
					{
						const Firebird::string moduleName(module->getAttributeName(0));
						ConfObj objModule(configFile.findObject("intl_module", moduleName.c_str()));
						filename = objModule->getValue("filename", "");
						configInfo = getConfigInfo(objModule);

						if (!modules->exist(filename))
						{
							ModuleLoader::Module* mod = ModuleLoader::loadModule(filename);
							if (!mod)
							{
								ModuleLoader::doctorModuleExtention(filename);
								mod = ModuleLoader::loadModule(filename);
							}
							if (mod)
							{
								// Negotiate version
								pfn_INTL_version versionFunction;
								USHORT version;

								if (mod->findSymbol(STRINGIZE(INTL_VERSION_ENTRYPOINT), versionFunction))
								{
									version = INTL_VERSION_2;
									versionFunction(&version);
								}
								else
									version = INTL_VERSION_1;

								if (version != INTL_VERSION_1 && version != INTL_VERSION_2)
								{
									string err_msg;
									err_msg.printf("INTL module '%s' is of incompatible version number %d",
										filename.c_str(), version);
									gds__log(err_msg.c_str());
									ok = false;
								}
								else
									modules->put(filename, mod);
							}
							else
							{
								gds__log((string("Can't load INTL module '") +
									filename.c_str() + "'").c_str());
								ok = false;
							}
						}
					}

					for (const Element* el2 = el->children; el2; el2 = el2->sibling)
					{
						if (el2->name == "collation")
						{
							const string collationName = el2->getAttributeName(0);
							const string charSetCollation = charSetName + ":" + collationName;
							const char* externalName = el2->getAttributeName(1);

							if (!registerCharSetCollation(charSetCollation, filename,
								(externalName ? externalName : collationName), configInfo))
							{
								conflicts.add(charSetCollation);
								ok = false;
							}
						}
					}
				}
			}
		}
	}
	catch (AdminException& ex)
	{
		gds__log((string("Error in INTL plugin config file '") + dir.getFilePath() + "': " + ex.getText()).c_str());
		ok = false;
	}

	registerCharSetCollation("NONE:NONE", "", "NONE", builtinConfig);
	registerCharSetCollation("OCTETS:OCTETS", "", "OCTETS", builtinConfig);
	registerCharSetCollation("ASCII:ASCII", "", "ASCII", builtinConfig);
	registerCharSetCollation("UNICODE_FSS:UNICODE_FSS", "", "UNICODE_FSS", builtinConfig);
	registerCharSetCollation("UTF8:UTF8", "", "UTF8", builtinConfig);
	registerCharSetCollation("UTF8:UCS_BASIC", "", "UCS_BASIC", builtinConfig);
	registerCharSetCollation("UTF8:UNICODE", "", "UNICODE", builtinConfig);

	registerCharSetCollation("UTF16:UTF16", "", "UTF16", builtinConfig);
#ifdef FB_NEW_INTL_ALLOW_NOT_READY
	registerCharSetCollation("UTF16:UCS_BASIC", "", "UCS_BASIC", builtinConfig);
	registerCharSetCollation("UTF32:UTF32", "", "UTF32", builtinConfig);
	registerCharSetCollation("UTF32:UCS_BASIC", "", "UCS_BASIC", builtinConfig);
#endif

	for (ObjectsArray<string>::const_iterator name(conflicts.begin()); name != conflicts.end(); ++name)
		charSetCollations->remove(*name);

	return ok;
}


bool IntlManager::collationInstalled(const Firebird::string& collationName,
	const Firebird::string& charSetName)
{
	return charSetCollations->exist(charSetName + ":" + collationName);
}


bool IntlManager::lookupCharSet(const Firebird::string& charSetName, charset* cs)
{
	ExternalInfo externalInfo;

	if (charSetCollations->get(charSetName + ":" + charSetName, externalInfo))
	{
		pfn_INTL_lookup_charset lookupFunction = NULL;

		if (externalInfo.moduleName.isEmpty())
			lookupFunction = INTL_builtin_lookup_charset;
		else
		{
			ModuleLoader::Module* module;

			if (modules->get(externalInfo.moduleName, module) && module)
				module->findSymbol(STRINGIZE(CHARSET_ENTRYPOINT), lookupFunction);
		}

		if (lookupFunction && (*lookupFunction)(cs, externalInfo.name.c_str(),
				externalInfo.configInfo.c_str()))
		{
			return validateCharSet(charSetName, cs);
		}
	}

	return false;
}


bool IntlManager::lookupCollation(const Firebird::string& collationName,
								  const Firebird::string& charSetName,
								  USHORT attributes, const UCHAR* specificAttributes,
								  ULONG specificAttributesLen, bool ignoreAttributes,
								  texttype* tt)
{
	ExternalInfo charSetExternalInfo;
	ExternalInfo collationExternalInfo;

	if (charSetCollations->get(charSetName + ":" + charSetName, charSetExternalInfo) &&
		charSetCollations->get(charSetName + ":" + collationName, collationExternalInfo))
	{
		pfn_INTL_lookup_texttype lookupFunction = NULL;

		if (collationExternalInfo.moduleName.isEmpty())
			lookupFunction = INTL_builtin_lookup_texttype;
		else
		{
			ModuleLoader::Module* module;

			if (modules->get(collationExternalInfo.moduleName, module) && module)
				module->findSymbol(STRINGIZE(TEXTTYPE_ENTRYPOINT), lookupFunction);
		}

		if (lookupFunction &&
			(*lookupFunction)(tt, collationExternalInfo.name.c_str(), charSetExternalInfo.name.c_str(),
							  attributes, specificAttributes, specificAttributesLen, ignoreAttributes,
							  collationExternalInfo.configInfo.c_str()))
		{
			return true;
		}
	}

	return false;
}


bool IntlManager::setupCollationAttributes(
	const Firebird::string& collationName, const Firebird::string& charSetName,
	const Firebird::string& specificAttributes, Firebird::string& newSpecificAttributes)
{
	ExternalInfo charSetExternalInfo;
	ExternalInfo collationExternalInfo;

	newSpecificAttributes = specificAttributes;

	if (charSetCollations->get(charSetName + ":" + charSetName, charSetExternalInfo) &&
		charSetCollations->get(charSetName + ":" + collationName, collationExternalInfo))
	{
		pfn_INTL_setup_attributes attributesFunction = NULL;

		if (collationExternalInfo.moduleName.isEmpty())
			attributesFunction = INTL_builtin_setup_attributes;
		else
		{
			ModuleLoader::Module* module;

			if (modules->get(collationExternalInfo.moduleName, module) && module)
				module->findSymbol(STRINGIZE(INTL_SETUP_ATTRIBUTES_ENTRYPOINT), attributesFunction);
		}

		if (attributesFunction)
		{
			HalfStaticArray<UCHAR, BUFFER_MEDIUM> buffer;

			// first try with the static buffer
			ULONG len = (*attributesFunction)(collationExternalInfo.name.c_str(),
				charSetExternalInfo.name.c_str(), collationExternalInfo.configInfo.c_str(),
				specificAttributes.length(), (const UCHAR*) specificAttributes.begin(),
				buffer.getCapacity(), buffer.begin());

			if (len == INTL_BAD_STR_LENGTH)
			{
				// ask the right buffer size
				len = (*attributesFunction)(collationExternalInfo.name.c_str(),
					charSetExternalInfo.name.c_str(), collationExternalInfo.configInfo.c_str(),
					specificAttributes.length(), (const UCHAR*) specificAttributes.begin(),
					0, NULL);

				if (len != INTL_BAD_STR_LENGTH)
				{
					// try again
					len = (*attributesFunction)(collationExternalInfo.name.c_str(),
						charSetExternalInfo.name.c_str(), collationExternalInfo.configInfo.c_str(),
						specificAttributes.length(), (const UCHAR*) specificAttributes.begin(),
						len, buffer.getBuffer(len));
				}
			}

			if (len != INTL_BAD_STR_LENGTH)
				newSpecificAttributes = string((const char*) buffer.begin(), len);
			else
				return false;
		}

		return true;
	}

	return false;
}


Firebird::string IntlManager::getConfigInfo(const ConfObj& confObj)
{
	if (!confObj.hasObject())
		return "";

	string configInfo;

	for (const Element* el = confObj->object->children; el; el = el->sibling)
	{
		string values;

		for (int i = 0; el->getAttributeName(i); ++i)
		{
			if (i > 0)
				values.append(" ");

			values.append(el->getAttributeName(i));
		}

		if (configInfo.hasData())
			configInfo.append(";");
		configInfo.append(string(el->name.c_str()) + "=" + values);
	}

	return configInfo;
}


bool IntlManager::registerCharSetCollation(const Firebird::string& name, const Firebird::PathName& filename,
	const Firebird::string& externalName, const Firebird::string& configInfo
)
{
	ExternalInfo conflict;

	if (charSetCollations->get(name, conflict))
	{
		gds__log((string("INTL plugin conflict: ") + name + " defined in " +
			(conflict.moduleName.isEmpty() ? "<builtin>" : conflict.moduleName.c_str()) +
			" and " + filename.c_str()).c_str());
		return false;
	}

	charSetCollations->put(name, ExternalInfo(filename, externalName, configInfo));
	return true;
}


bool IntlManager::validateCharSet(const Firebird::string& charSetName, charset* cs)
{
	bool valid = true;
	string s;

	string unsupportedMsg;
	unsupportedMsg.printf("Unsupported character set %s.", charSetName.c_str());

	if (!(cs->charset_flags & CHARSET_ASCII_BASED))
	{
		valid = false;
		s.printf("%s. Only ASCII-based character sets are supported yet.", unsupportedMsg.c_str());
		gds__log(s.c_str());
	}

	if (cs->charset_min_bytes_per_char != 1)
	{
		valid = false;
		s.printf("%s. Wide character sets are not supported yet.", unsupportedMsg.c_str());
		gds__log(s.c_str());
	}

	/***
	if (cs->charset_space_length != 1 || *cs->charset_space_character != ' ')
	{
		valid = false;
		s.printf("%s. Only ASCII space is supported in charset_space_character yet.",
			unsupportedMsg.c_str());
		gds__log(s.c_str());
	}
	***/
	if (cs->charset_space_length != 1)
	{
		valid = false;
		s.printf("%s. Wide space is not supported yet.", unsupportedMsg.c_str());
		gds__log(s.c_str());
	}

	return valid;
}


}	// namespace Jrd
