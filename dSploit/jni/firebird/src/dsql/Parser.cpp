/*
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
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include <ctype.h>
#include "../dsql/Parser.h"
#include "../jrd/jrd.h"

using namespace Firebird;
using namespace Jrd;


Parser::Parser(MemoryPool& pool, USHORT aClientDialect, USHORT aDbDialect, USHORT aParserVersion,
			const TEXT* string, USHORT length, SSHORT characterSet)
	: PermanentStorage(pool),
	  client_dialect(aClientDialect),
	  db_dialect(aDbDialect),
	  parser_version(aParserVersion),
	  transformedString(pool),
	  strMarks(pool),
	  stmt_ambiguous(false)
{
	yyps = 0;
	yypath = 0;
	yylvals = 0;
	yylvp = 0;
	yylve = 0;
	yylvlim = 0;
	yylpsns = 0;
	yylpp = 0;
	yylpe = 0;
	yylplim = 0;
	yylexp = 0;
	yylexemes = 0;

	lex.start = string;
	lex.line_start = lex.ptr = string;
	lex.end = string + length;
	lex.lines = 1;
	lex.att_charset = characterSet;
	lex.line_start_bk = lex.line_start;
	lex.lines_bk = lex.lines;
	lex.param_number = 1;
	lex.prev_keyword = -1;
#ifdef DSQL_DEBUG
	if (DSQL_debug & 32)
		dsql_trace("Source DSQL string:\n%.*s", (int) length, string);
#endif
}


Parser::~Parser()
{
	while (yyps)
	{
		yyparsestate* p = yyps;
		yyps = p->save;
		yyFreeState(p);
	}

	while (yypath)
	{
		yyparsestate* p = yypath;
		yypath = p->save;
		yyFreeState(p);
	}

	delete[] yylvals;
	delete[] yylpsns;
	delete[] yylexemes;
}


Parser::YYSTYPE Parser::parse()
{
	if (parseAux() != 0)
		return NULL;

	transformString(lex.start, lex.end - lex.start, transformedString);

	return DSQL_parse;
}


// Transform strings (or substrings) prefixed with introducer (_charset) to ASCII equivalent.
void Parser::transformString(const char* start, unsigned length, string& dest)
{
	const static char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F'};

	const unsigned fromBegin = start - lex.start;
	HalfStaticArray<char, 256> buffer;
	const char* pos = start;

	// We need only the "introduced" strings, in the bounds of "start" and "length" and in "pos"
	// order. Let collect them.

	SortedArray<StrMark> introducedMarks;

	GenericMap<NonPooled<dsql_str*, StrMark> >::ConstAccessor accessor(&strMarks);
	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		const StrMark& mark = accessor.current()->second;
		if (mark.introduced && mark.pos >= fromBegin && mark.pos < fromBegin + length)
			introducedMarks.add(mark);
	}

	for (size_t i = 0; i < introducedMarks.getCount(); ++i)
	{
		const StrMark& mark = introducedMarks[i];

		const char* s = lex.start + mark.pos;
		buffer.add(pos, s - pos);

		if (!isspace(UCHAR(pos[s - pos - 1])))
			buffer.add(' ');	// fix _charset'' becoming invalid syntax _charsetX''

		const size_t count = buffer.getCount();
		const size_t newSize = count + 2 + mark.str->str_length * 2 + 1;
		buffer.grow(newSize);
		char* p = buffer.begin() + count;

		*p++ = 'X';
		*p++ = '\'';

		const char* s2 = mark.str->str_data;

		for (const char* end = s2 + mark.str->str_length; s2 < end; ++s2)
		{
			*p++ = HEX_DIGITS[UCHAR(*s2) >> 4];
			*p++ = HEX_DIGITS[UCHAR(*s2) & 0xF];
		}

		*p = '\'';
		fb_assert(p < buffer.begin() + newSize);

		pos = s + mark.length;
	}

	fb_assert(start + length - pos >= 0);
	buffer.add(pos, start + length - pos);

	dest = string(buffer.begin(), MIN(string::max_length(), buffer.getCount()));
}
