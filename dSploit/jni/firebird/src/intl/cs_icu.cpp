/*
 *	PROGRAM:	Firebird International support
 *	MODULE:		cs_icu.cpp
 *	DESCRIPTION:	Character set definitions for ICU character sets.
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
#include "../intl/ldcommon.h"
#include "cs_icu.h"
#include "cv_icu.h"
#include "unicode/ucnv.h"


static void charset_destroy(charset* cs)
{
	delete[] const_cast<ASCII*>(cs->charset_name);
	delete[] const_cast<BYTE*>(cs->charset_space_character);
}


bool CSICU_charset_init(charset* cs,
						const ASCII* charSetName)
{
	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = ucnv_open(charSetName, &status);

	if (U_SUCCESS(status))
	{
		// charSetName comes from stack. Copy it.
		ASCII* p = new ASCII[strlen(charSetName) + 1];
		cs->charset_name = p;
		strcpy(p, charSetName);

		cs->charset_version = CHARSET_VERSION_1;
		cs->charset_flags |= CHARSET_ASCII_BASED;
		cs->charset_min_bytes_per_char = ucnv_getMinCharSize(conv);
		cs->charset_max_bytes_per_char = ucnv_getMaxCharSize(conv);
		cs->charset_fn_destroy = charset_destroy;
		cs->charset_fn_well_formed = NULL;

		const UChar unicodeSpace = 32;

		BYTE* p2 = new BYTE[cs->charset_max_bytes_per_char];
		cs->charset_space_character = p2;
		cs->charset_space_length = ucnv_fromUChars(conv, reinterpret_cast<char*>(p2),
			cs->charset_max_bytes_per_char, &unicodeSpace, 1, &status);
		fb_assert(U_SUCCESS(status));

		ucnv_close(conv);

		CVICU_convert_init(cs);
	}

	return U_SUCCESS(status);
}
