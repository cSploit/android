/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_iso8859_13.c
 *	DESCRIPTION:	Language Drivers in the iso8859_13 family.
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
 * Contributor(s): Jonas Jasas
 */

#include "firebird.h"
#include "../intl/ldcommon.h"
#include "ld_proto.h"
#include "lc_narrow.h"


TEXTTYPE_ENTRY (ISO885913_c1_init)
{
	static const ASCII	POSIX[] = "lt_LT.ISO8859_13";

#include "../intl/collations/xx885913lt.h"

	return LC_NARROW_family3(cache, cs, CC_LITHUANIA, LDRV_TIEBREAK,
			NoCaseOrderTbl, ToUpperConversionTbl, ToLowerConversionTbl,
			CompressTbl, ExpansionTbl, POSIX, attributes, specific_attributes, specific_attributes_length);
}
