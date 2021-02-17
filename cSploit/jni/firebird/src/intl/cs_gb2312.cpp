/*
 *	PROGRAM:		InterBase International support
 *	MODULE:			cs_gb2312.cpp
 *	DESCRIPTION:	Character Set definitions for GB2312 family.
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
#include "../intl/cv_gb2312.h"
#include "../intl/cv_narrow.h"

CHARSET_ENTRY(CS_gb_2312)
{
#include "../intl/charsets/cs_gb2312.h"
	static const ASCII POSIX[] = "GB_2312";

	csptr->charset_version = CHARSET_VERSION_1;
	csptr->charset_name = POSIX;
	csptr->charset_flags |= CHARSET_LEGACY_SEMANTICS | CHARSET_ASCII_BASED;
	csptr->charset_min_bytes_per_char = 1;
	csptr->charset_max_bytes_per_char = 2;
	csptr->charset_space_length = 1;
	csptr->charset_space_character = (const BYTE*) " ";	// 0x20
	csptr->charset_fn_well_formed = CVGB_check_gb2312;

	CV_convert_init(&csptr->charset_to_unicode,
					CVGB_gb2312_to_unicode,
					to_unicode_mapping_array,
					to_unicode_map);
	CV_convert_init(&csptr->charset_from_unicode,
					CVGB_unicode_to_gb2312,
					from_unicode_mapping_array,
					from_unicode_map);

	CHARSET_RETURN;
}
