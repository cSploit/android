/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_iso8859_2.cpp
 *	DESCRIPTION:	Language Drivers in the iso8859_2 family.
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
 * All Rights Reserved.
 * Contributor(s): Michal Bukovjan_________________.
 */

#include "firebird.h"
#include "../intl/ldcommon.h"
#include "lc_narrow.h"
#include "lc_dos.h"
#include "ld_proto.h"


TEXTTYPE_ENTRY(ISO88592_c1_init)
{
	static const ASCII POSIX[] = "cs_CZ.ISO8859_2";

#include "../intl/collations/xx88592czech.h"

	return LC_NARROW_family3(cache, cs, CC_CZECH, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}

TEXTTYPE_ENTRY(ISO88592_c2_init)
{
	static const ASCII POSIX[] = "ISO_HUN.ISO8859_2";

#include "../intl/collations/xx88592hun.h"

	return LC_NARROW_family3(cache, cs, CC_HUNGARY, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}

TEXTTYPE_ENTRY(ISO88592_c3_init)
{
	static const ASCII POSIX[] = "ISO_PLK.ISO8859_2";

#include "../intl/collations/xx88592plk.h"

	return LC_NARROW_family3(cache, cs, CC_POLAND, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}
