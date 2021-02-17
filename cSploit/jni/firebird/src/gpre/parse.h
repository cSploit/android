/*
 *	PROGRAM:	Language Preprocessor
 *	MODULE:		parse.h
 *	DESCRIPTION:	Common parser definitions
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

#ifndef GPRE_PARSE_H
#define GPRE_PARSE_H

#include "../gpre/words.h"
#include "../gpre/gpre.h"

// Token block, used to hold a lexical token.

enum tok_t {
	tok_ident,
	tok_number,
	tok_sglquoted,
	tok_punct,
	tok_introducer,
	tok_dblquoted
};

struct tok
{
	tok_t tok_type;				// type of token
	gpre_sym* tok_symbol;		// hash block if recognized
	kwwords_t tok_keyword;		// keyword number, if recognized
	SLONG tok_position;			// byte number in input stream
	USHORT tok_length;
	USHORT tok_white_space;
	SCHAR tok_string[MAX_SYM_SIZE];
	bool tok_first;				// is it the first token in a statement?
	gpre_sym* tok_charset;		// Character set of token
};

const size_t TOK_LEN = sizeof(tok);

// CVC: This function doesn't unescape embedded quotes.
inline void strip_quotes(tok& tkn)
{
	int ij;
	for (ij = 1; ij < tkn.tok_length - 1; ij++)
		tkn.tok_string[ij - 1] = tkn.tok_string[ij];
	--ij;
	tkn.tok_string[ij] = 0;
	tkn.tok_length = ij;
}

inline bool isQuoted(const int typ)
{
	return (typ == tok_sglquoted || typ == tok_dblquoted);
}

#endif // GPRE_PARSE_H

