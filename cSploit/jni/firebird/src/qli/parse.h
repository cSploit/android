/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		par.h
 *	DESCRIPTION:	Parser definitions
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

#ifndef QLI_PARSE_H
#define QLI_PARSE_H

#include <stdio.h>

const int MAXSYMLEN	= 256;

// Keywords

enum kwwords {
    KW_none = 0,
#include "../qli/symbols.h"
    KW_continuation
};

// Token block, used to hold a lexical token.

enum tok_t {
    tok_ident,
    tok_number,
    tok_quoted,
    tok_punct,
    tok_eol,
    tok_eof
};

struct qli_tok
{
    blk			tok_header;
    tok_t		tok_type;		// type of token
    qli_symbol*	tok_symbol;		// hash block if recognized
    kwwords		tok_keyword;	// keyword number, if recognized
    SLONG		tok_position;	// byte number in input stream
    USHORT		tok_length;
    //qli_tok*	tok_next;
    //qli_tok*	tok_prior;
    TEXT		tok_string [2];
};

// Input line control

enum line_t {
    line_stdin,
    line_blob,
    line_file,
    line_string
    //, line_edit
};

struct qli_line
{
    blk			line_header;
    qli_line*	line_next;
    qli_dbb*	line_database;
    USHORT		line_size;
    USHORT		line_length;
    TEXT*		line_ptr;
    SLONG		line_position;
    FB_API_HANDLE line_source_blob;			// Blob handle
	FILE*		line_source_file;			// File handle
	line_t		line_type;
    TEXT		line_data[256];
    TEXT		line_source_name[2];
};

#endif // QLI_PARSE_H

