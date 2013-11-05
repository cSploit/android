/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		cs_unicode.cpp
 *	DESCRIPTION:	Character set definition for Unicode
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
#include "cv_narrow.h"

CHARSET_ENTRY(CS_unicode_ucs2)
{
	static const USHORT space = 0x0020;

	csptr->charset_version = CHARSET_VERSION_1;
	csptr->charset_name = "UNICODE";
	csptr->charset_flags |= CHARSET_ASCII_BASED;
	csptr->charset_min_bytes_per_char = 2;
	csptr->charset_max_bytes_per_char = 2;
	csptr->charset_space_length = sizeof(space);
	csptr->charset_space_character = (const BYTE*) &space;	// 0x0020
	csptr->charset_fn_well_formed = NULL;
	CV_convert_init(&csptr->charset_to_unicode,
					CV_wc_copy,
					NULL, NULL);
	CV_convert_init(&csptr->charset_from_unicode,
					CV_wc_copy,
					NULL, NULL);
	CHARSET_RETURN;
}
