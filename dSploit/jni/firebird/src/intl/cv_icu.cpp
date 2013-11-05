/*
 *	PROGRAM:	Firebird International support
 *	MODULE:		cv_icu.cpp
 *	DESCRIPTION:	Codeset conversion for ICU character sets.
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
#include "ld_proto.h"
#include "cv_icu.h"
#include "unicode/ucnv.h"


static UConverter* create_converter(csconvert* cv, UErrorCode* status)
{
	UConverter* conv = ucnv_open(cv->csconvert_impl->cs->charset_name, status);
	const void* oldContext;

	UConverterFromUCallback oldFromAction;
	ucnv_setFromUCallBack(
		conv,
		UCNV_FROM_U_CALLBACK_STOP,
		NULL,
		&oldFromAction,
		&oldContext,
		status);

	UConverterToUCallback oldToAction;
	ucnv_setToUCallBack(
		conv,
		UCNV_TO_U_CALLBACK_STOP,
		NULL,
		&oldToAction,
		&oldContext,
		status);

	fb_assert(U_SUCCESS(*status));

	return conv;
}


static void convert_destroy(csconvert* cv)
{
	delete cv->csconvert_impl;
}


static ULONG unicode_to_icu(csconvert* cv,
							ULONG srcLen,
							const BYTE* src,
							ULONG dstLen,
							BYTE* dst,
							USHORT* errCode,
							ULONG* errPosition)
{
	fb_assert(srcLen % sizeof(UChar) == 0);

	*errCode = 0;
	*errPosition = 0;

	if (!dst)
		return srcLen / sizeof(UChar) * cv->csconvert_impl->cs->charset_max_bytes_per_char;

	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = create_converter(cv, &status);

	Firebird::Aligner<UChar> alignedSource(src, srcLen);
	const UChar* source = alignedSource;
	char* target = reinterpret_cast<char*>(dst);
	ucnv_fromUnicode(conv, &target, target + dstLen, &source,
		source + srcLen / sizeof(UChar), NULL, TRUE, &status);

	*errPosition = (source - alignedSource) * sizeof(UChar);

	if (!U_SUCCESS(status))
	{
		switch (status)
		{
		case U_INVALID_CHAR_FOUND:
		case U_ILLEGAL_CHAR_FOUND:
			*errCode = CS_CONVERT_ERROR;
			break;
		case U_TRUNCATED_CHAR_FOUND:
			// If this assert fails, it means we should use the same logic as in icu_to_unicode.
			fb_assert(*errPosition < srcLen);
			*errCode = CS_BAD_INPUT;
			break;
		case U_BUFFER_OVERFLOW_ERROR:
			*errCode = CS_TRUNCATION_ERROR;
			break;
		default:
			fb_assert(false);
			*errCode = CS_CONVERT_ERROR;
		}
	}

	ucnv_close(conv);

	return target - reinterpret_cast<char*>(dst);
}


static ULONG icu_to_unicode(csconvert* cv,
							ULONG srcLen,
							const BYTE* src,
							ULONG dstLen,
							BYTE* dst,
							USHORT* errCode,
							ULONG* errPosition)
{
	fb_assert(dstLen % sizeof(UChar) == 0);

	*errCode = 0;
	*errPosition = 0;

	if (!dst)
		return srcLen / cv->csconvert_impl->cs->charset_min_bytes_per_char * sizeof(UChar);

	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = create_converter(cv, &status);

	const char* source = reinterpret_cast<const char*>(src);
	Firebird::OutAligner<UChar> alignedTarget(dst, dstLen);
	UChar* target = alignedTarget;
	ucnv_toUnicode(conv, &target, target + dstLen / sizeof(UChar), &source,
		source + srcLen, NULL, TRUE, &status);

	*errPosition = source - reinterpret_cast<const char*>(src);

	if (!U_SUCCESS(status))
	{
		switch (status)
		{
		case U_INVALID_CHAR_FOUND:
		case U_ILLEGAL_CHAR_FOUND:
			*errCode = CS_CONVERT_ERROR;
			break;
		case U_TRUNCATED_CHAR_FOUND:
			{
				// ICU advances the source pointer to after the truncated character, hence we can't
				// continue converting when we have more input. So we have to subtract from
				// errPosition the number of invalid bytes.
				status = U_ZERO_ERROR;
				char errBytes[16];
				int8_t errLen = sizeof(errBytes);
				ucnv_getInvalidChars(conv, errBytes, &errLen, &status);
				if (!U_SUCCESS(status))
					*errCode = CS_CONVERT_ERROR;
				else
				{
					fb_assert((int) errLen <= (int) *errPosition);
					*errPosition -= errLen;
					*errCode = CS_BAD_INPUT;
				}
			}
			break;
		case U_BUFFER_OVERFLOW_ERROR:
			*errCode = CS_TRUNCATION_ERROR;
			break;
		default:
			fb_assert(false);
			*errCode = CS_CONVERT_ERROR;
		}
	}

	ucnv_close(conv);

	return (target - alignedTarget) * sizeof(UChar);
}


void CVICU_convert_init(charset* cs)
{
	cs->charset_to_unicode.csconvert_version = CSCONVERT_VERSION_1;
	cs->charset_to_unicode.csconvert_name = "ICU->UNICODE";
	cs->charset_to_unicode.csconvert_fn_convert = icu_to_unicode;
	cs->charset_to_unicode.csconvert_fn_destroy = convert_destroy;
	cs->charset_to_unicode.csconvert_impl = new CsConvertImpl();
	cs->charset_to_unicode.csconvert_impl->cs = cs;

	cs->charset_from_unicode.csconvert_version = CSCONVERT_VERSION_1;
	cs->charset_from_unicode.csconvert_name = "UNICODE->ICU";
	cs->charset_from_unicode.csconvert_fn_convert = unicode_to_icu;
	cs->charset_from_unicode.csconvert_fn_destroy = convert_destroy;
	cs->charset_from_unicode.csconvert_impl = new CsConvertImpl();
	cs->charset_from_unicode.csconvert_impl->cs = cs;
}
