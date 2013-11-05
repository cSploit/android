/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_iso8859_1.cpp
 *	DESCRIPTION:	Language Drivers in the is8859_1 family.
 *			(full International collation)
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
#include "../intl/ldcommon.h"
#include "../intl/ld_proto.h"
#include "../jrd/CharSet.h"
#include "lc_narrow.h"
#include "lc_dos.h"


TEXTTYPE_ENTRY(KOI8R_c1_init)
{
	static const ASCII POSIX[] = "KOI8R_RU.KOI8R";

#include "../intl/collations/koi8r_ru.h"

	return LC_NARROW_family3(cache, cs, CC_RUSSIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(KOI8U_c1_init)
{
	static const ASCII POSIX[] = "KOI8U_UA.KOI8U";

#include "../intl/collations/koi8u_ua.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_39_init)
{
	static const ASCII POSIX[] = "da_DA.ISO8859_1";

#include "../intl/collations/bl88591da0.h"

	return LC_NARROW_family3(cache, cs, CC_DENMARK, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_40_init)
{
	static const ASCII POSIX[] = "du_NL.ISO8859_1";

#include "../intl/collations/bl88591nl0.h"

	return LC_NARROW_family3(cache, cs, CC_NEDERLANDS, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_41_init)
{
	static const ASCII POSIX[] = "fi_FI.ISO8859_1";

#include "../intl/collations/bl88591fi0.h"

	return LC_NARROW_family3(cache, cs, CC_FINLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_42_init)
{
	static const ASCII POSIX[] = "fr_FR.ISO8859_1";

#include "../intl/collations/bl88591fr0.h"

	return LC_NARROW_family3(cache, cs, CC_FRANCE, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_43_init)
{
	static const ASCII POSIX[] = "fr_CA.ISO8859_1";

#include "../intl/collations/bl88591ca0.h"

	return LC_NARROW_family3(cache, cs, CC_FRENCHCAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_44_init)
{
	static const ASCII POSIX[] = "de_DE.ISO8859_1";

#include "../intl/collations/bl88591de0.h"

	return LC_NARROW_family3(cache, cs, CC_GERMANY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_45_init)
{
	static const ASCII POSIX[] = "is_IS.ISO8859_1";

#include "../intl/collations/bl88591is0.h"

	return LC_NARROW_family3(cache, cs, CC_ICELAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_46_init)
{
	static const ASCII POSIX[] = "it_IT.ISO8859_1";

#include "../intl/collations/bl88591it0.h"

	return LC_NARROW_family3(cache, cs, CC_ITALY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}



TEXTTYPE_ENTRY(ISO88591_48_init)
{
	static const ASCII POSIX[] = "no_NO.ISO8859_1";

#include "../intl/collations/bl88591no0.h"

	return LC_NARROW_family3(cache, cs, CC_NORWAY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_49_init)
{
	static const ASCII POSIX[] = "es_ES.ISO8859_1";

#include "../intl/collations/bl88591es0.h"

	return LC_NARROW_family3(cache, cs, CC_SPAIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_51_init)
{
	static const ASCII POSIX[] = "sv_SV.ISO8859_1";

#include "../intl/collations/bl88591sv0.h"

	return LC_NARROW_family3(cache, cs, CC_SWEDEN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_52_init)
{
	static const ASCII POSIX[] = "en_UK.ISO8859_1";

#include "../intl/collations/bl88591uk0.h"

	return LC_NARROW_family3(cache, cs, CC_UK, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_53_init)
{
	static const ASCII POSIX[] = "en_US.ISO8859_1";

#include "../intl/collations/bl88591us0.h"

	return LC_NARROW_family3(cache, cs, CC_US, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_54_init)
{
	static const ASCII POSIX[] = "pt_PT.ISO8859_1";

#include "../intl/collations/bl88591pt0.h"

	return LC_NARROW_family3(cache, cs, CC_PORTUGAL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_55_init)
{
	static const ASCII POSIX[] = "pt_BR.ISO8859_1";

#include "../intl/collations/bl88591ptbr0.h"

	return LC_NARROW_family3(cache, cs, CC_BRAZIL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(ISO88591_56_init)
{
	static const ASCII POSIX[] = "es_ES_CI_AI.ISO8859_1";

#include "../intl/collations/bl88591es0.h"

	return LC_NARROW_family3(cache, cs, CC_SPAIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c1_init)
{
	static const ASCII POSIX[] = "PXW_CSY.WIN1250";

#include "../intl/collations/pw1250czech.h"

	return LC_NARROW_family3(cache, cs, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c2_init)
{
	static const ASCII POSIX[] = "PXW_HUNDC.WIN1250";

#include "../intl/collations/pw1250hundc.h"

	return LC_NARROW_family3(cache, cs, CC_HUNGARY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c3_init)
{
	static const ASCII POSIX[] = "PXW_PLK.WIN1250";

#include "../intl/collations/pw1250polish.h"

	return LC_NARROW_family3(cache, cs, CC_POLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c4_init)
{
	static const ASCII POSIX[] = "PXW_SLOV.WIN1250";

#include "../intl/collations/pw1250slov.h"

	return LC_NARROW_family3(cache, cs, CC_YUGOSLAVIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY (WIN1250_c5_init)
{
	static const ASCII	  POSIX[] = "PXW_HUN.WIN1250";

#include "../intl/collations/pw1250hun.h"

	return LC_NARROW_family3(cache, cs, CC_HUNGARY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c6_init)
{
	static const ASCII POSIX[] = "BS_BA.WIN1250";

#include "../intl/collations/win1250bsba.h"

	return LC_NARROW_family3(cache, cs, CC_YUGOSLAVIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c7_init)
{
	static const ASCII POSIX[] = "WIN_CZ.WIN1250";

#include "../intl/collations/win_cz.h"

	return LC_NARROW_family3(cache, cs, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1250_c8_init)
{
	static const ASCII POSIX[] = "WIN_CZ_CI_AI.WIN1250";

#include "../intl/collations/win_cz_ci_ai.h"

	return LC_NARROW_family3(cache, cs, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1251_c1_init)
{
	static const ASCII POSIX[] = "PXW_CYRL.WIN1251";

#include "../intl/collations/pw1251cyrr.h"

	return LC_NARROW_family3(cache, cs, CC_RUSSIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1251_c2_init)
{
	static const ASCII POSIX[] = "WIN1251_UA.WIN1251";

#include "../intl/collations/xx1251_ua.h"

	return LC_NARROW_family3(cache, cs, CC_RUSSIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c1_init)
{
	static const ASCII POSIX[] = "PXW_INTL.WIN1252";

#include "../intl/collations/pw1252intl.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c2_init)
{
	static const ASCII POSIX[] = "PXW_INTL850.WIN1252";

#include "../intl/collations/pw1252i850.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c3_init)
{
	static const ASCII POSIX[] = "PXW_NORDAN4.WIN1252";

#include "../intl/collations/pw1252nor4.h"

	return LC_NARROW_family3(cache, cs, CC_NORDAN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c4_init)
{
	static const ASCII POSIX[] = "PXW_SPAN.WIN1252";

#include "../intl/collations/pw1252span.h"

	return LC_NARROW_family3(cache, cs, CC_SPAIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c5_init)
{
	static const ASCII POSIX[] = "PXW_SWEDFIN.WIN1252";

#include "../intl/collations/pw1252swfn.h"

	return LC_NARROW_family3(cache, cs, CC_SWEDFIN, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1252_c6_init)
{
	static const ASCII POSIX[] = "WIN_PTBR.WIN1252";

#include "../intl/collations/pw1252ptbr.h"

	return LC_NARROW_family3(cache, cs, CC_BRAZIL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1253_c1_init)
{
	static const ASCII POSIX[] = "PXW_GREEK.WIN1253";

#include "../intl/collations/pw1253greek1.h"

	return LC_NARROW_family3(cache, cs, CC_GREECE, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1254_c1_init)
{
	static const ASCII POSIX[] = "PXW_TURK.WIN1254";

#include "../intl/collations/pw1254turk.h"

	return LC_NARROW_family3(cache, cs, CC_TURKEY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1257_c1_init)
{
	static const ASCII POSIX[] = "WIN1257_EE.WIN1257";

#include "../intl/collations/win1257_ee.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1257_c2_init)
{
	static const ASCII POSIX[] = "WIN1257_LT.WIN1257";

#include "../intl/collations/win1257_lt.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(WIN1257_c3_init)
{
	static const ASCII POSIX[] = "WIN1257_LV.WIN1257";

#include "../intl/collations/win1257_lv.h"

	return LC_NARROW_family3(cache, cs, CC_INTL, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(NEXT_c1_init)
{
	static const ASCII POSIX[] = "NXT_US.NEXT";

#include "../intl/collations/blNEXTus0.h"

	return LC_NARROW_family3(cache, cs, CC_US, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(NEXT_c2_init)
{
	static const ASCII POSIX[] = "NXT_DEU.NEXT";

#include "../intl/collations/blNEXTde0.h"

	return LC_NARROW_family3(cache, cs, CC_GERMANY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(NEXT_c3_init)
{
	static const ASCII POSIX[] = "NXT_FRA.NEXT";

#include "../intl/collations/blNEXTfr0.h"

	return LC_NARROW_family3(cache, cs, CC_FRANCE, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(NEXT_c4_init)
{
	static const ASCII POSIX[] = "NXT_ITA.NEXT";

#include "../intl/collations/blNEXTit0.h"

	return LC_NARROW_family3(cache, cs, CC_ITALY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}
