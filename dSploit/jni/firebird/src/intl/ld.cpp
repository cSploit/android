/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		ld.cpp
 *	DESCRIPTION:	Language Driver lookup & support routines
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
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "../jrd/IntlUtil.h"
#include "../intl/ldcommon.h"
#include "../intl/ld_proto.h"
#include "../intl/cs_icu.h"
#include "../intl/lc_icu.h"
#include "fb_exception.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h> // for MAXPATHLEN
#endif
#include <stdio.h>

using namespace Firebird;

// Commented out to make Linux version work because it is inaccessiable on all
// known platforms. Nickolay Samofatov, 10 Sept 2002
// void gds__log(UCHAR*, ...);


#define	EXTERN_texttype(name)	INTL_BOOL name (texttype*, charset*, const ASCII*, const ASCII*, USHORT, const UCHAR*, ULONG, const ASCII*)
// #define EXTERN_convert(name)	INTL_BOOL name (csconvert*, const ASCII*, const ASCII*)
#define EXTERN_charset(name)	INTL_BOOL name (charset*, const ASCII*)

EXTERN_texttype(DOS101_init);
EXTERN_texttype(DOS101_c2_init);
EXTERN_texttype(DOS101_c3_init);
EXTERN_texttype(DOS101_c4_init);
EXTERN_texttype(DOS101_c5_init);
EXTERN_texttype(DOS101_c6_init);
EXTERN_texttype(DOS101_c7_init);
EXTERN_texttype(DOS101_c8_init);
EXTERN_texttype(DOS101_c9_init);
EXTERN_texttype(DOS101_c10_init);

EXTERN_texttype(DOS102_init);
EXTERN_texttype(DOS105_init);
EXTERN_texttype(DOS106_init);

EXTERN_texttype(DOS107_init);
EXTERN_texttype(DOS107_c1_init);
EXTERN_texttype(DOS107_c2_init);
EXTERN_texttype(DOS107_c3_init);

EXTERN_texttype(DOS852_c0_init);
EXTERN_texttype(DOS852_c1_init);
EXTERN_texttype(DOS852_c2_init);
EXTERN_texttype(DOS852_c4_init);
EXTERN_texttype(DOS852_c5_init);
EXTERN_texttype(DOS852_c6_init);
EXTERN_texttype(DOS852_c7_init);
EXTERN_texttype(DOS852_c8_init);

EXTERN_texttype(DOS857_c0_init);
EXTERN_texttype(DOS857_c1_init);

EXTERN_texttype(DOS860_c0_init);
EXTERN_texttype(DOS860_c1_init);

EXTERN_texttype(DOS861_c0_init);
EXTERN_texttype(DOS861_c1_init);

EXTERN_texttype(DOS863_c0_init);
EXTERN_texttype(DOS863_c1_init);

EXTERN_texttype(DOS737_c0_init);
EXTERN_texttype(DOS775_c0_init);
EXTERN_texttype(DOS858_c0_init);
EXTERN_texttype(DOS862_c0_init);
EXTERN_texttype(DOS864_c0_init);
EXTERN_texttype(DOS866_c0_init);
EXTERN_texttype(DOS869_c0_init);

EXTERN_texttype(CYRL_c0_init);
EXTERN_texttype(CYRL_c1_init);
EXTERN_texttype(CYRL_c2_init);

// Latin 1 character set
EXTERN_texttype(ISO88591_cp_init);

// Latin 1 collations
EXTERN_texttype(ISO88591_39_init);
EXTERN_texttype(ISO88591_40_init);
EXTERN_texttype(ISO88591_41_init);
EXTERN_texttype(ISO88591_42_init);
EXTERN_texttype(ISO88591_43_init);
EXTERN_texttype(ISO88591_44_init);
EXTERN_texttype(ISO88591_45_init);
EXTERN_texttype(ISO88591_46_init);
EXTERN_texttype(ISO88591_48_init);
EXTERN_texttype(ISO88591_49_init);
EXTERN_texttype(ISO88591_51_init);
EXTERN_texttype(ISO88591_52_init);
EXTERN_texttype(ISO88591_53_init);
EXTERN_texttype(ISO88591_54_init);
EXTERN_texttype(ISO88591_55_init);
EXTERN_texttype(ISO88591_56_init);

// Latin 2 character set
EXTERN_texttype (ISO88592_cp_init);

// latin 2 collations
EXTERN_texttype (ISO88592_c1_init);
EXTERN_texttype (ISO88592_c2_init);
EXTERN_texttype (ISO88592_c3_init);

EXTERN_texttype (ISO88593_cp_init);
EXTERN_texttype (ISO88594_cp_init);
EXTERN_texttype (ISO88595_cp_init);
EXTERN_texttype (ISO88596_cp_init);
EXTERN_texttype (ISO88597_cp_init);
EXTERN_texttype (ISO88598_cp_init);
EXTERN_texttype (ISO88599_cp_init);

EXTERN_texttype (ISO885913_cp_init);
EXTERN_texttype (ISO885913_c1_init);

EXTERN_texttype(WIN1250_c0_init);
EXTERN_texttype(WIN1250_c1_init);
EXTERN_texttype(WIN1250_c2_init);
EXTERN_texttype(WIN1250_c3_init);
EXTERN_texttype(WIN1250_c4_init);
EXTERN_texttype(WIN1250_c5_init);
EXTERN_texttype(WIN1250_c6_init);
EXTERN_texttype(WIN1250_c7_init);
EXTERN_texttype(WIN1250_c8_init);

EXTERN_texttype(WIN1251_c0_init);
EXTERN_texttype(WIN1251_c1_init);
EXTERN_texttype(WIN1251_c2_init);

EXTERN_texttype(WIN1252_c0_init);
EXTERN_texttype(WIN1252_c1_init);
EXTERN_texttype(WIN1252_c2_init);
EXTERN_texttype(WIN1252_c3_init);
EXTERN_texttype(WIN1252_c4_init);
EXTERN_texttype(WIN1252_c5_init);
EXTERN_texttype(WIN1252_c6_init);
EXTERN_texttype(WIN1252_c7_init);

EXTERN_texttype(WIN1253_c0_init);
EXTERN_texttype(WIN1253_c1_init);

EXTERN_texttype(WIN1254_c0_init);
EXTERN_texttype(WIN1254_c1_init);

EXTERN_texttype(WIN1255_c0_init);
EXTERN_texttype(WIN1256_c0_init);

EXTERN_texttype(WIN1257_c0_init);
EXTERN_texttype(WIN1257_c1_init);
EXTERN_texttype(WIN1257_c2_init);
EXTERN_texttype(WIN1257_c3_init);

EXTERN_texttype(NEXT_c0_init);
EXTERN_texttype(NEXT_c1_init);
EXTERN_texttype(NEXT_c2_init);
EXTERN_texttype(NEXT_c3_init);
EXTERN_texttype(NEXT_c4_init);

EXTERN_texttype(DOS160_init);
EXTERN_texttype(DOS160_c1_init);
EXTERN_texttype(DOS160_c2_init);
EXTERN_texttype(DOS160_c3_init);
EXTERN_texttype(DOS160_c4_init);
EXTERN_texttype(DOS160_c5_init);
EXTERN_texttype(DOS160_c6_init);
EXTERN_texttype(DOS160_c7_init);
EXTERN_texttype(DOS160_c8_init);
EXTERN_texttype(DOS160_c9_init);
EXTERN_texttype(DOS160_c10_init);

EXTERN_texttype(UNI200_init);
EXTERN_texttype(UNI201_init);

EXTERN_texttype(JIS220_init);
EXTERN_texttype(JIS230_init);

EXTERN_texttype(KOI8R_c0_init);
EXTERN_texttype(KOI8R_c1_init);

EXTERN_texttype(KOI8U_c0_init);
EXTERN_texttype(KOI8U_c1_init);

EXTERN_texttype(WIN1258_c0_init);

EXTERN_charset(CS_iso_ISO8859_1);
EXTERN_charset(CS_iso_ISO8859_2);
EXTERN_charset(CS_iso_ISO8859_3);
EXTERN_charset(CS_iso_ISO8859_4);
EXTERN_charset(CS_iso_ISO8859_5);
EXTERN_charset(CS_iso_ISO8859_6);
EXTERN_charset(CS_iso_ISO8859_7);
EXTERN_charset(CS_iso_ISO8859_8);
EXTERN_charset(CS_iso_ISO8859_9);
EXTERN_charset(CS_iso_ISO8859_13);
EXTERN_charset(CS_win1250);
EXTERN_charset(CS_win1251);
EXTERN_charset(CS_win1252);
EXTERN_charset(CS_win1253);
EXTERN_charset(CS_win1254);
EXTERN_charset(CS_win1255);
EXTERN_charset(CS_win1256);
EXTERN_charset(CS_win1257);
EXTERN_charset(CS_next);
EXTERN_charset(CS_cyrl);
EXTERN_charset(CS_dos_437);
EXTERN_charset(CS_dos_850);
EXTERN_charset(CS_dos_852);
EXTERN_charset(CS_dos_857);
EXTERN_charset(CS_dos_860);
EXTERN_charset(CS_dos_861);
EXTERN_charset(CS_dos_863);
EXTERN_charset(CS_dos_865);
EXTERN_charset(CS_dos_737);
EXTERN_charset(CS_dos_775);
EXTERN_charset(CS_dos_858);
EXTERN_charset(CS_dos_862);
EXTERN_charset(CS_dos_864);
EXTERN_charset(CS_dos_866);
EXTERN_charset(CS_dos_869);
EXTERN_charset(CS_unicode_ucs2);
EXTERN_charset(CS_unicode_fss);
EXTERN_charset(CS_sjis);
EXTERN_charset(CS_euc_j);

EXTERN_charset(CS_big_5);
EXTERN_charset(CS_gb_2312);
EXTERN_charset(CS_ksc_5601);

EXTERN_charset(CS_koi8r);
EXTERN_charset(CS_koi8u);

EXTERN_charset(CS_win1258);

EXTERN_texttype(BIG5_init);
EXTERN_texttype(KSC_5601_init);
EXTERN_texttype(ksc_5601_dict_init);
EXTERN_texttype(GB_2312_init);


// ASF: FB 2.0 doesn't call LD_version function, then
// version should be INTL_VERSION_1 by default.
USHORT version = INTL_VERSION_1;


struct
{
	const char* charSetName;
	EXTERN_charset((*ptr));
} static const charSets[] =
{
	// builtin charsets should not be listed here
	{"SJIS_0208", CS_sjis},
	{"EUCJ_0208", CS_euc_j},
	{"DOS437", CS_dos_437},
	{"DOS850", CS_dos_850},
	{"DOS865", CS_dos_865},
	{"ISO8859_1", CS_iso_ISO8859_1},
	{"ISO8859_2", CS_iso_ISO8859_2},
	{"ISO8859_3", CS_iso_ISO8859_3},
	{"ISO8859_4", CS_iso_ISO8859_4},
	{"ISO8859_5", CS_iso_ISO8859_5},
	{"ISO8859_6", CS_iso_ISO8859_6},
	{"ISO8859_7", CS_iso_ISO8859_7},
	{"ISO8859_8", CS_iso_ISO8859_8},
	{"ISO8859_9", CS_iso_ISO8859_9},
	{"ISO8859_13", CS_iso_ISO8859_13},
	{"DOS852", CS_dos_852},
	{"DOS857", CS_dos_857},
	{"DOS860", CS_dos_860},
	{"DOS861", CS_dos_861},
	{"DOS863", CS_dos_863},
	{"CYRL", CS_cyrl},
	{"DOS737", CS_dos_737},
	{"DOS775", CS_dos_775},
	{"DOS858", CS_dos_858},
	{"DOS862", CS_dos_862},
	{"DOS864", CS_dos_864},
	{"DOS866", CS_dos_866},
	{"DOS869", CS_dos_869},
	{"WIN1250", CS_win1250},
	{"WIN1251", CS_win1251},
	{"WIN1252", CS_win1252},
	{"WIN1253", CS_win1253},
	{"WIN1254", CS_win1254},
	{"NEXT", CS_next},
	{"WIN1255", CS_win1255},
	{"WIN1256", CS_win1256},
	{"WIN1257", CS_win1257},
	{"KSC_5601", CS_ksc_5601},
	{"BIG_5", CS_big_5},
	{"GB_2312", CS_gb_2312},
	{"KOI8R", CS_koi8r},
	{"KOI8U", CS_koi8u},
	{"WIN1258", CS_win1258},
	// ICU charsets should not be listed here
	{NULL, NULL}
};


struct
{
	const char* charSetName;
	const char* collationName;
	EXTERN_texttype((*ptr));
} static const collations[] =
{
	// builtin collations should not be listed here
	{"SJIS_0208", "SJIS_0208", JIS220_init},
	{"EUCJ_0208", "EUCJ_0208", JIS230_init},
	{"DOS437", "DOS437", DOS101_init},
	{"DOS437", "PDOX_ASCII", DOS101_init},
	{"DOS437", "PDOX_INTL", DOS102_init},
	{"DOS437", "PDOX_SWEDFIN", DOS106_init},
	{"DOS437", "DB_DEU437", DOS101_c2_init},
	{"DOS437", "DB_ESP437", DOS101_c3_init},
	{"DOS437", "DB_FIN437", DOS101_c4_init},
	{"DOS437", "DB_FRA437", DOS101_c5_init},
	{"DOS437", "DB_ITA437", DOS101_c6_init},
	{"DOS437", "DB_NLD437", DOS101_c7_init},
	{"DOS437", "DB_SVE437", DOS101_c8_init},
	{"DOS437", "DB_UK437", DOS101_c9_init},
	{"DOS437", "DB_US437", DOS101_c10_init},
	{"DOS850", "DOS850", DOS160_init},
	{"DOS850", "DB_FRC850", DOS160_c1_init},
	{"DOS850", "DB_DEU850", DOS160_c2_init},
	{"DOS850", "DB_ESP850", DOS160_c3_init},
	{"DOS850", "DB_FRA850", DOS160_c4_init},
	{"DOS850", "DB_ITA850", DOS160_c5_init},
	{"DOS850", "DB_NLD850", DOS160_c6_init},
	{"DOS850", "DB_PTB850", DOS160_c7_init},
	{"DOS850", "DB_SVE850", DOS160_c8_init},
	{"DOS850", "DB_UK850", DOS160_c9_init},
	{"DOS850", "DB_US850", DOS160_c10_init},
	{"DOS865", "DOS865", DOS107_init},
	{"DOS865", "PDOX_NORDAN4", DOS107_c1_init},
	{"DOS865", "DB_DAN865", DOS107_c2_init},
	{"DOS865", "DB_NOR865", DOS107_c3_init},
	{"ISO8859_1", "ISO8859_1", ISO88591_cp_init},
	{"ISO8859_1", "DA_DA", ISO88591_39_init},
	{"ISO8859_1", "DU_NL", ISO88591_40_init},
	{"ISO8859_1", "FI_FI", ISO88591_41_init},
	{"ISO8859_1", "FR_FR", ISO88591_42_init},
	{"ISO8859_1", "FR_CA", ISO88591_43_init},
	{"ISO8859_1", "DE_DE", ISO88591_44_init},
	{"ISO8859_1", "IS_IS", ISO88591_45_init},
	{"ISO8859_1", "IT_IT", ISO88591_46_init},
	{"ISO8859_1", "NO_NO", ISO88591_48_init},
	{"ISO8859_1", "ES_ES", ISO88591_49_init},
	{"ISO8859_1", "SV_SV", ISO88591_51_init},
	{"ISO8859_1", "EN_UK", ISO88591_52_init},
	{"ISO8859_1", "EN_US", ISO88591_53_init},
	{"ISO8859_1", "PT_PT", ISO88591_54_init},
	{"ISO8859_1", "PT_BR", ISO88591_55_init},
	{"ISO8859_1", "ES_ES_CI_AI", ISO88591_56_init},
	{"ISO8859_1", "FR_FR_CI_AI", ISO88591_42_init},
	{"ISO8859_2", "ISO8859_2", ISO88592_cp_init},
	{"ISO8859_2", "CS_CZ", ISO88592_c1_init},
	{"ISO8859_2", "ISO_HUN", ISO88592_c2_init},
	{"ISO8859_2", "ISO_PLK", ISO88592_c3_init},
	{"ISO8859_3", "ISO8859_3", ISO88593_cp_init},
	{"ISO8859_4", "ISO8859_4", ISO88594_cp_init},
	{"ISO8859_5", "ISO8859_5", ISO88595_cp_init},
	{"ISO8859_6", "ISO8859_6", ISO88596_cp_init},
	{"ISO8859_7", "ISO8859_7", ISO88597_cp_init},
	{"ISO8859_8", "ISO8859_8", ISO88598_cp_init},
	{"ISO8859_9", "ISO8859_9", ISO88599_cp_init},
	{"ISO8859_13", "ISO8859_13", ISO885913_cp_init},
	{"ISO8859_13", "LT_LT", ISO885913_c1_init},
	{"DOS852", "DOS852", DOS852_c0_init},
	{"DOS852", "DB_CSY", DOS852_c1_init},
	{"DOS852", "DB_PLK", DOS852_c2_init},
	{"DOS852", "DB_SLO", DOS852_c4_init},
	{"DOS852", "PDOX_CSY", DOS852_c5_init},
	{"DOS852", "PDOX_PLK", DOS852_c6_init},
	{"DOS852", "PDOX_HUN", DOS852_c7_init},
	{"DOS852", "PDOX_SLO", DOS852_c8_init},
	{"DOS857", "DOS857", DOS857_c0_init},
	{"DOS857", "DB_TRK", DOS857_c1_init},
	{"DOS860", "DOS860", DOS860_c0_init},
	{"DOS860", "DB_PTG860", DOS860_c1_init},
	{"DOS861", "DOS861", DOS861_c0_init},
	{"DOS861", "PDOX_ISL", DOS861_c1_init},
	{"DOS863", "DOS863", DOS863_c0_init},
	{"DOS863", "DB_FRC863", DOS863_c1_init},
	{"CYRL", "CYRL", CYRL_c0_init},
	{"CYRL", "DB_RUS", CYRL_c1_init},
	{"CYRL", "PDOX_CYRL", CYRL_c2_init},
	{"DOS737", "DOS737", DOS737_c0_init},
	{"DOS737", "DOS775", DOS775_c0_init},
	{"DOS858", "DOS858", DOS858_c0_init},
	{"DOS862", "DOS862", DOS862_c0_init},
	{"DOS864", "DOS864", DOS864_c0_init},
	{"DOS866", "DOS866", DOS866_c0_init},
	{"DOS869", "DOS869", DOS869_c0_init},
	{"WIN1250", "WIN1250", WIN1250_c0_init},
	{"WIN1250", "PXW_CSY", WIN1250_c1_init},
	{"WIN1250", "PXW_HUNDC", WIN1250_c2_init},
	{"WIN1250", "PXW_PLK", WIN1250_c3_init},
	{"WIN1250", "PXW_SLOV", WIN1250_c4_init},
	{"WIN1250", "PXW_HUN", WIN1250_c5_init},
	{"WIN1250", "BS_BA", WIN1250_c6_init},
	{"WIN1250", "WIN_CZ", WIN1250_c7_init},
	{"WIN1250", "WIN_CZ_CI_AI", WIN1250_c8_init},
	{"WIN1251", "WIN1251", WIN1251_c0_init},
	{"WIN1251", "PXW_CYRL", WIN1251_c1_init},
	{"WIN1251", "WIN1251_UA", WIN1251_c2_init},
	{"WIN1252", "WIN1252", WIN1252_c0_init},
	{"WIN1252", "PXW_INTL", WIN1252_c1_init},
	{"WIN1252", "PXW_INTL850", WIN1252_c2_init},
	{"WIN1252", "PXW_NORDAN4", WIN1252_c3_init},
	{"WIN1252", "PXW_SPAN", WIN1252_c4_init},
	{"WIN1252", "PXW_SWEDFIN", WIN1252_c5_init},
	{"WIN1252", "WIN_PTBR", WIN1252_c6_init},
	{"WIN1253", "WIN1253", WIN1253_c0_init},
	{"WIN1253", "PXW_GREEK", WIN1253_c1_init},
	{"WIN1254", "WIN1254", WIN1254_c0_init},
	{"WIN1254", "PXW_TURK", WIN1254_c1_init},
	{"NEXT", "NEXT", NEXT_c0_init},
	{"NEXT", "NXT_US", NEXT_c1_init},
	{"NEXT", "NXT_DEU", NEXT_c2_init},
	{"NEXT", "NXT_FRA", NEXT_c3_init},
	{"NEXT", "NXT_ITA", NEXT_c4_init},
	{"NEXT", "NXT_ESP", NEXT_c0_init},
	{"WIN1255", "WIN1255", WIN1255_c0_init},
	{"WIN1256", "WIN1256", WIN1256_c0_init},
	{"WIN1257", "WIN1257", WIN1257_c0_init},
	{"WIN1257", "WIN1257_EE", WIN1257_c1_init},
	{"WIN1257", "WIN1257_LT", WIN1257_c2_init},
	{"WIN1257", "WIN1257_LV", WIN1257_c3_init},
	{"KSC_5601", "KSC_5601", KSC_5601_init},
	{"KSC_5601", "KSC_DICTIONARY", ksc_5601_dict_init},
	{"BIG_5", "BIG_5", BIG5_init},
	{"GB_2312", "GB_2312", GB_2312_init},
	{"KOI8R", "KOI8R", KOI8R_c0_init},
	{"KOI8R", "KOI8R_RU", KOI8R_c1_init},
	{"KOI8U", "KOI8U", KOI8U_c0_init},
	{"KOI8U", "KOI8U_UA", KOI8U_c1_init},
	{"WIN1258", "WIN1258", WIN1258_c0_init},
	// ICU collations should not be listed here
	{NULL, NULL, NULL}
};


INTL_BOOL FB_DLL_EXPORT LD_lookup_charset(charset* cs, const ASCII* name, const ASCII* /*config_info*/)
{
	// ASF: We can't read config_info if version < INTL_VERSION_2,
	// since it wasn't pushed in the stack by the engine.

	try
	{
		for (int i = 0; charSets[i].charSetName; ++i)
		{
			if (strcmp(charSets[i].charSetName, name) == 0)
				return charSets[i].ptr(cs, name);
		}

		return CSICU_charset_init(cs, name);
	}
	catch (Firebird::BadAlloc)
	{
		fb_assert(false);
		return false;
	}
}


INTL_BOOL FB_DLL_EXPORT LD_lookup_texttype(texttype* tt, const ASCII* texttype_name, const ASCII* charset_name,
										   USHORT attributes, const UCHAR* specific_attributes,
										   ULONG specific_attributes_length, INTL_BOOL ignore_attributes,
										   const ASCII* config_info)
{
	const ASCII* configInfo;

	// ASF: We can't read config_info if version < INTL_VERSION_2,
	// since it wasn't pushed in the stack by the engine.
	if (version >= INTL_VERSION_2)
		configInfo = config_info;
	else
		configInfo = "";

	if (ignore_attributes)
	{
		attributes = TEXTTYPE_ATTR_PAD_SPACE;
		specific_attributes = NULL;
		specific_attributes_length = 0;
	}

	try
	{
		for (int i = 0; collations[i].collationName; ++i)
		{
			if (strcmp(collations[i].charSetName, charset_name) == 0 &&
				strcmp(collations[i].collationName, texttype_name) == 0)
			{
				charset cs;
				memset(&cs, 0, sizeof(cs));
				int j;

				for (j = 0; charSets[j].charSetName; ++j)
				{
					if (strcmp(charSets[j].charSetName, charset_name) == 0)
					{
						if (LD_lookup_charset(&cs, charset_name, configInfo))
							break;

						return false;
					}
				}

				fb_assert(charSets[j].charSetName);

				INTL_BOOL ret = collations[i].ptr(tt, &cs, texttype_name, charset_name,
					attributes, specific_attributes, specific_attributes_length, config_info);

				if (cs.charset_fn_destroy)
					cs.charset_fn_destroy(&cs);

				return ret;
			}
		}

		return LCICU_texttype_init(
			tt, texttype_name, charset_name, attributes, specific_attributes,
			specific_attributes_length, configInfo);
	}
	catch (Firebird::BadAlloc)
	{
		fb_assert(false);
		return false;
	}
}


ULONG FB_DLL_EXPORT LD_setup_attributes(
	const ASCII* textTypeName, const ASCII* charSetName, const ASCII* configInfo,
	ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst)
{
	Firebird::string specificAttributes((const char*) src, srcLen);
	Firebird::string newSpecificAttributes = specificAttributes;

	if (!LCICU_setup_attributes(textTypeName, charSetName, configInfo,
			specificAttributes, newSpecificAttributes))
	{
		return INTL_BAD_STR_LENGTH;
	}

	if (dstLen == 0)
		return newSpecificAttributes.length();

	if (newSpecificAttributes.length() <= dstLen)
	{
		memcpy(dst, newSpecificAttributes.begin(), newSpecificAttributes.length());
		return newSpecificAttributes.length();
	}

	return INTL_BAD_STR_LENGTH;
}


void FB_DLL_EXPORT LD_version(USHORT* version)
{
	// We support version 1 and 2.
	if (*version != INTL_VERSION_1)
		*version = INTL_VERSION_2;

	::version = *version;
}
