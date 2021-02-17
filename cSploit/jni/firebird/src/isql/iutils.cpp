/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		iutils.h
 *	DESCRIPTION:	Functions independent of preprocessing, used in several isql modules.
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

#include "iutils_proto.h"
#include "../yvalve/keywords.h"
#include "../yvalve/gds_proto.h"
//#if defined(WIN_NT)
//#include <windows.h>
//#endif
#include <ctype.h>
#include "../jrd/ibase.h"
#include "isql.h"
#include "../common/classes/MsgPrint.h"
#include <stdarg.h>

using MsgFormat::SafeArg;


#ifdef NOT_USED_OR_REPLACED
// Same as fb_utils::exact_name, but writes the output to the second argument.
TEXT* IUTILS_blankterm2(const TEXT* input, TEXT* output)
{
	TEXT* q = output - 1;
	for (TEXT* p = output; (*p = *input) != 0; ++p, ++input)
	{
		if (*p != BLANK)
			q = p;
	}
	*(q + 1) = 0;
	return output;
}
#endif


void IUTILS_copy_SQL_id(const TEXT* in_str, TEXT* output_str, TEXT escape_char)
{
/**************************************
 *
 *	I U T I L S _ c o p y _ S Q L _ i d
 *
 **************************************
 *
 * Functional description
 *
 *	Copy/rebuild the SQL identifier by adding escape double quote if
 *	double quote is part of the SQL identifier and wraps around the
 *	SQL identifier with delimited double quotes
 *
 **************************************/

	/* CVC: Try to detect if we can get rid of double quotes as
	   requested by Ann. Notice empty names need double quotes.
	   Assume the caller invoked previously ISQL_blankterm() that's
	   just another function like DYN_terminate, MET_exact_name, etc.
	   ISQL_blankterm has been replaced by fb_utils::exact_name. */

	if (escape_char == DBL_QUOTE)
	{
		// Cannot rely on ANSI functions that may be localized.
		bool need_quotes = *in_str < 'A' || *in_str > 'Z';
		TEXT* q1 = output_str;
		for (const TEXT* p1 = in_str; *p1 && !need_quotes; ++p1, ++q1)
		{
			if ((*p1 < 'A' || *p1 > 'Z') && (*p1 < '0' || *p1 > '9') && *p1 != '_' && *p1 != '$')
			{
				need_quotes = true;
				break;
			}
			*q1 = *p1;
		}
		if (!need_quotes && !KEYWORD_stringIsAToken(in_str))
		{
			*q1 = '\0';
			return;
		}
	}

	TEXT* q1 = output_str;
	*q1++ = escape_char;

	for (const TEXT* p1 = in_str; *p1; p1++)
	{
		*q1++ = *p1;
		if (*p1 == escape_char) {
			*q1++ = escape_char;
		}
	}
	*q1++ = escape_char;
	*q1 = '\0';
}

void IUTILS_make_upper(TEXT* str)
{
/**************************************
 *
 *	I U T I L S _ m a k e _ u p p e r
 *
 **************************************
 *
 *	Force the name of a metadata object to
 *	uppercase.
 *
 **************************************/
	if (!str)
		return;

	for (UCHAR* p = reinterpret_cast<UCHAR*>(str); *p; p++)
		*p = UPPER7(*p);
}


void IUTILS_msg_get(USHORT number, TEXT* msg, const SafeArg& args)
{
/**************************************
 *
 *	I U T I L S _ m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file
 *
 **************************************/

	fb_msg_format(NULL, ISQL_MSG_FAC, number, MSG_LENGTH, msg, args);
}


void IUTILS_msg_get(USHORT number, USHORT size, TEXT* msg, const SafeArg& args)
{
/**************************************
 *
 *	I U T I L S _ m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file
 *
 **************************************/

	fb_msg_format(NULL, ISQL_MSG_FAC, number, size, msg, args);
}

void IUTILS_printf(FILE* fp, const char* buffer)
{
/**************************************
 *
 *	I U T I L S _ p r i n t f
 *
 **************************************
 *
 *	Centralized printing facility
 *
 **************************************/
	fprintf(fp, "%s", buffer);
	fflush(fp); // John's fix.
}


void IUTILS_printf2(FILE* fp, const char* buffer, ...)
{
/**************************************
 *
 *	I U T I L S _ p r i n t f 2
 *
 **************************************
 *
 *	Centralized printing facility, more flexible
 *
 **************************************/
	va_list args;
	va_start(args, buffer);
	vfprintf(fp, buffer, args);
	va_end(args);
	fflush(fp); // John's fix.
}


// I U T I L S _ p u t _ e r r m s g
// Retrives a message and prints it as an error.
void IUTILS_put_errmsg(USHORT number, const SafeArg& args)
{
	TEXT errbuf[MSG_LENGTH];
	IUTILS_msg_get(number, errbuf, args);
	STDERROUT(errbuf);
}


void IUTILS_remove_and_unescape_quotes(TEXT* string, const char quote)
{
/**************************************
 *
 *	I U T I L S _ r e m o v e _ a n d _ u n e s c a p e _ q u o t e s
 *
 **************************************
 *
 * Functional description
 *	Remove the delimited quotes. Blanks could be part of
 *	delimited SQL identifier. It has to deal with embedded quotes, too.
 *
 **************************************/
	const size_t cmd_len = strlen(string);
	TEXT* q = string;
	const TEXT* p = q;
	const TEXT* const end_of_str = p + cmd_len;

	for (size_t cnt = 1; cnt < cmd_len && p < end_of_str; cnt++)
	{
		p++;
		if (cnt < cmd_len - 1)
		{
			*q = *p;
			if (p + 1 < end_of_str)
			{
				if (*(p + 1) == quote)	// skip the escape double quote
					p++;
			}
			else
			{
				p++;
				*q = '\0';
			}
		}
		else
			*q = '\0';

		q++;
	}
	*q = '\0';
}


void IUTILS_truncate_term(TEXT* str, USHORT len)
{
/**************************************
 *
 *	I U T I L S _ t r u n c a t e _ t e r m
 *
 **************************************
 *
 * Functional description
 *	Truncates the rightmost contiguous spaces on a string.
 * CVC: Notice isspace may be influenced by locales.
 **************************************/
	int i = len - 1;
	while (i >= 0 && (isspace(UCHAR(str[i])) || (str[i] == 0)))
		--i;
	str[i + 1] = 0;
}
