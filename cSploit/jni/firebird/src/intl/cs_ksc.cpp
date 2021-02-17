/*
 *	PROGRAM:        InterBase International support
 *	MODULE: 		cs_ksc.cpp
 *	DESCRIPTION:	Character set definitions for KSC-5601.
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
#include "cv_ksc.h"
#include "cv_narrow.h"

CHARSET_ENTRY(CS_ksc_5601)
{
	static const char space = 0x20;

#include "../intl/charsets/cs_ksc5601.h"

	csptr->charset_version = CHARSET_VERSION_1;
	csptr->charset_name = "KSC_5601";
	csptr->charset_flags |= CHARSET_LEGACY_SEMANTICS | CHARSET_ASCII_BASED;
	csptr->charset_min_bytes_per_char = 1;
	csptr->charset_max_bytes_per_char = 2;
	csptr->charset_space_length = 1;
	csptr->charset_space_character = (const BYTE*) &space;
	csptr->charset_fn_well_formed = CVKSC_check_ksc;

	CV_convert_init(&csptr->charset_to_unicode,
					CVKSC_ksc_to_unicode,
					to_unicode_mapping_array,
					to_unicode_map);
	CV_convert_init(&csptr->charset_from_unicode,
					CVKSC_unicode_to_ksc,
					from_unicode_mapping_array,
					from_unicode_map);

	CHARSET_RETURN;
}
