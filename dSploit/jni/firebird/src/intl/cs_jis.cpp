/*
 *      PROGRAM:        InterBase International support
 *      MODULE:         cs_jis.cpp
 *      DESCRIPTION:    Character Set definitions for JIS family.
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
#include "cv_jis.h"
#include "cv_narrow.h"

CHARSET_ENTRY(CS_jis_0208_1990)
{
	static const USHORT space = 0x0020;

#include "../intl/charsets/cs_jis_0208_1990.h"

	csptr->charset_version = CHARSET_VERSION_1;
	csptr->charset_name = "JIS_0208_1990";
	csptr->charset_flags |= CHARSET_ASCII_BASED;
	csptr->charset_min_bytes_per_char = 2;
	csptr->charset_max_bytes_per_char = 2;
	csptr->charset_space_length = 2;
	csptr->charset_space_character = (const BYTE*) &space;	// 0x20
	csptr->charset_fn_well_formed = NULL;

	CV_convert_init(&csptr->charset_to_unicode,
					CV_wc_to_wc,
					to_unicode_mapping_array, to_unicode_map);
	CV_convert_init(&csptr->charset_from_unicode,
					CV_wc_to_wc,
					from_unicode_mapping_array,
					from_unicode_map);
	CHARSET_RETURN;
}


CHARSET_ENTRY(CS_sjis)
{
	CS_jis_0208_1990(csptr, NULL); //, cs_name); Second param is unused
	csptr->charset_name = "SJIS";
	csptr->charset_flags |= CHARSET_LEGACY_SEMANTICS;
	csptr->charset_min_bytes_per_char = 1;
	csptr->charset_space_length = 1;
	csptr->charset_space_character = (const BYTE*) " ";	// 0x20
	csptr->charset_to_unicode.csconvert_fn_convert = CVJIS_sjis_to_unicode;
	csptr->charset_from_unicode.csconvert_fn_convert = CVJIS_unicode_to_sjis;
	csptr->charset_fn_well_formed = CVJIS_check_sjis;
	CHARSET_RETURN;
}


CHARSET_ENTRY(CS_euc_j)
{
	CS_jis_0208_1990(csptr, NULL); //cs_name); Second param is unused
	csptr->charset_name = "EUC-J";
	csptr->charset_flags |= CHARSET_LEGACY_SEMANTICS;
	csptr->charset_min_bytes_per_char = 1;
	csptr->charset_space_length = 1;
	csptr->charset_space_character = (const BYTE*) " ";	// 0x20
	csptr->charset_to_unicode.csconvert_fn_convert = CVJIS_eucj_to_unicode;
	csptr->charset_from_unicode.csconvert_fn_convert = CVJIS_unicode_to_eucj;
	csptr->charset_fn_well_formed = CVJIS_check_euc;
	CHARSET_RETURN;
}
