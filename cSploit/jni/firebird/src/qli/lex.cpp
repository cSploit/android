/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		lex.cpp
 *	DESCRIPTION:	Lexical analyser
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
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../jrd/ibase.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/hsh_proto.h"
#include "../qli/lex_proto.h"
#include "../qli/proc_proto.h"
#include "../yvalve/utl_proto.h"
#include "../common/gdsassert.h"
#include "../common/utils_proto.h"
#include "../common/classes/TempFile.h"

using Firebird::TempFile;
using MsgFormat::SafeArg;


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

//#ifdef HAVE_CTYPES_H
//#include <ctypes.h>
//#endif

#ifdef HAVE_IO_H
#include <io.h> // isatty
#endif

static const char* const SCRATCH = "fb_query_";

const char* FOPEN_INPUT_TYPE = "r";

static bool get_line(FILE*, TEXT*, USHORT);
static int nextchar(const bool);
static void next_line(const bool);
static void retchar();
static bool scan_number(SSHORT, TEXT**);
static int skip_white();

static qli_lls* QLI_statements;
static int QLI_position;
static FILE *input_file = NULL, *trace_file = NULL;
static char trace_file_name[MAXPATHLEN];
static SLONG trans_limit;

const SLONG TRANS_LIMIT	= 10;

const char CHR_ident	= char(1);
const char CHR_letter	= char(2);
const char CHR_digit	= char(4);
const char CHR_quote	= char(8);
const char CHR_white	= char(16);
const char CHR_eol		= char(32);

const char CHR_IDENT	= CHR_ident;
const char CHR_LETTER	= CHR_letter | CHR_ident;
const char CHR_DIGIT	= CHR_digit | CHR_ident;
const char CHR_QUOTE	= CHR_quote;
const char CHR_WHITE	= CHR_white;
const char CHR_EOL		= CHR_eol;

static const char* const eol_string = "end of line";
static const char* const eof_string = "*end_of_file*";

// Do not reference the array directly; use the functions below.

static const char classes_array[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, CHR_WHITE, CHR_EOL, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	CHR_WHITE, 0, CHR_QUOTE, CHR_IDENT, CHR_LETTER, 0, 0, CHR_QUOTE,
	0, 0, 0, 0, 0, 0, 0, 0,
	CHR_DIGIT, CHR_DIGIT, CHR_DIGIT, CHR_DIGIT, CHR_DIGIT, CHR_DIGIT,
		CHR_DIGIT, CHR_DIGIT,
	CHR_DIGIT, CHR_DIGIT, 0, 0, 0, 0, 0, 0,
	0, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER, CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER, CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, 0, 0, 0, 0, CHR_IDENT,
	0, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER, CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER, CHR_LETTER,
		CHR_LETTER, CHR_LETTER,
	CHR_LETTER, CHR_LETTER, CHR_LETTER, 0
};

inline char classes(int idx)
{
	return classes_array[(UCHAR) idx];
}

inline char classes(UCHAR idx)
{
	return classes_array[idx];
}



bool LEX_active_procedure()
{
/**************************************
 *
 *	L E X _ a c t i v e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Return true if we're running out of a
 *	procedure and false otherwise.  Somebody
 *	somewhere may care.
 *
 **************************************/

	return (QLI_line->line_type == line_blob);
}


void LEX_edit(SLONG start, SLONG stop)
{
/**************************************
 *
 *	L E X _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Dump the last full statement into a scratch file, then
 *	push the scratch file on the input stack.
 *
 **************************************/
	const Firebird::PathName filename = TempFile::create(SCRATCH);
	FILE* scratch = fopen(filename.c_str(), "w+b");
	if (!scratch)
		IBERROR(61);			// Msg 61 couldn't open scratch file

#ifdef WIN_NT
	stop--;
#endif

	if (fseek(trace_file, start, 0))
	{
		fseek(trace_file, 0, 2);
		IBERROR(59);			// Msg 59 fseek failed
	}

	while (++start <= stop)
	{
		const SSHORT c = getc(trace_file);
		if (c == EOF)
			break;
		putc(c, scratch);
	}

	fclose(scratch);

	if (gds__edit(filename.c_str(), TRUE))
		LEX_push_file(filename.c_str(), true);

	unlink(filename.c_str());

	fseek(trace_file, 0, 2);
}


qli_tok* LEX_edit_string()
{
/**************************************
 *
 *	L E X _ e d i t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Parse the next token as an edit_string.
 *
 **************************************/
	SSHORT c;
	qli_tok* token = QLI_token;

	do {
		c = skip_white();
	} while (c == '\n');
	TEXT* p = token->tok_string;
	*p = c;

	if (!QLI_line)
	{
		token->tok_symbol = NULL;
		token->tok_keyword = KW_none;
		return NULL;
	}

	while (!(classes(c) & (CHR_white | CHR_eol)))
	{
		*p++ = c;
		if (classes(c) & CHR_quote)
		{
			for (;;)
			{
				const SSHORT d = nextchar(false);
				if (d == '\n')
				{
					retchar();
					break;
				}
				*p++ = d;
				if (d == c)
					break;
			}
		}
		c = nextchar(true);
	}

	retchar();

	if (p[-1] == ',')
	{
		retchar();
		--p;
	}

	token->tok_length = p - token->tok_string;
	*p = 0;
	token->tok_symbol = NULL;
	token->tok_keyword = KW_none;

	if (sw_trace)
		puts(token->tok_string);

	return token;
}


qli_tok* LEX_filename()
{
/**************************************
 *
 *	L E X _ f i l e n a m e
 *
 **************************************
 *
 * Functional description
 *	Parse the next token as a filename.
 *
 **************************************/
	SSHORT c;

	qli_tok* token = QLI_token;

	do {
		c = skip_white();
	} while (c == '\n');
	TEXT* p = token->tok_string;
	*p++ = c;

	// If there isn't a line, we're all done

	if (!QLI_line)
	{
		token->tok_symbol = NULL;
		token->tok_keyword = KW_none;
		return NULL;
	}

	// notice if this looks like a quoted file name

	SSHORT save = 0;
	if (classes(c) & CHR_quote)
	{
		token->tok_type = tok_quoted;
		save = c;
	}

	// Look for white space or end of line, allowing embedded quoted strings.

	for (;;)
	{
		c = nextchar(true);
		char char_class = classes(c);
		if (c == '"' && c != save)
		{
			*p++ = c;
			for (;;)
			{
				c = nextchar(true);
				char_class = classes(c);
				if ((char_class & CHR_eol) || c == '"')
					break;
				*p++ = c;
			}
		}

		if (char_class & (CHR_white | CHR_eol))
			break;
		*p++ = c;
	}

	retchar();

	// Drop trailing semi-colon to avoid confusion

	if (p[-1] == ';')
	{
		retchar();
		--p;
	}

	// complain on unterminated quoted string

	if ((token->tok_type == tok_quoted) && (p[-1] != save))
		IBERROR(60);			// Msg 60 unterminated quoted string

	token->tok_length = p - token->tok_string;
	*p = 0;
	token->tok_symbol = NULL;
	token->tok_keyword = KW_none;

	if (sw_trace)
		puts(token->tok_string);

	return token;
}


void LEX_fini()
{
/**************************************
 *
 *	L E X _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Shut down LEX in a more or less clean way.
 *
 **************************************/

	if (trace_file)
	{
		fclose(trace_file);
		unlink(trace_file_name);
	}
}


void LEX_flush()
{
/**************************************
 *
 *	L E X _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Flush the input stream after an error.
 *
 **************************************/

	trans_limit = 0;

	if (!QLI_line)
		return;

	// Pop off line sources until we're down to the last one.

	while (QLI_line->line_next)
		LEX_pop_line();

	// Look for a semi-colon

	if (QLI_semi)
	{
		while (QLI_line && QLI_token->tok_keyword != KW_SEMI)
			LEX_token();
	}
	else
	{
		while (QLI_line && QLI_token->tok_type != tok_eol)
			LEX_token();
	}
}


bool LEX_get_line(const TEXT* prompt, TEXT* buffer, int size)
{
/**************************************
 *
 *	L E X _ g e t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Give a prompt and read a line.  If the line is terminated by
 *	an EOL, return true.  If the buffer is exhausted and non-blanks
 *	would be discarded, return an error.  If EOF is detected,
 *	return false.  Regardless, a null terminated string is returned.
 *
 **************************************/
	// UNIX flavor

	if (prompt)
		printf("%s", prompt);

	errno = 0;
	TEXT* p = buffer;

	bool overflow_flag = false;
	SSHORT c;

	while (true)
	{
		c = getc(input_file);
		if (c == EOF)
		{
			if (SYSCALL_INTERRUPTED(errno) && !QLI_abort)
			{
				errno = 0;
				continue;
			}

			// The check for this actually being a terminal that is at
			//   end of file is to prevent looping through a redirected
			//   stdin (e.g., a script file).

			if (prompt && isatty(fileno(stdin)))
			{
				rewind(stdin);
				putchar('\n');
			}
			if (QLI_abort)
				continue;

			break;
		}
		if (--size > 0)
			*p++ = c;
		else if (c != ' ' && c != '\n')
			overflow_flag = true;
		if (c == '\n')
			break;
	}

	*p = 0;
	if (c == EOF)
		return false;

	if (overflow_flag)
	{
		buffer[0] = 0;
		IBERROR(476);			// Msg 476 input line too long
	}

	if (sw_verify)
		fputs(buffer, stdout);

	return true;
}


void LEX_init()
{
/**************************************
 *
 *	L E X _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize for lexical scanning.  While we're at it, open a
 *	scratch trace file to keep all input.
 *
 **************************************/
	const Firebird::PathName filename = TempFile::create(SCRATCH);
	strcpy(trace_file_name, filename.c_str());
	trace_file = fopen(trace_file_name, "w+b");
#ifdef UNIX
	unlink(trace_file_name);
#endif
	if (!trace_file)
		IBERROR(61);			// Msg 61 couldn't open scratch file

	QLI_token = (qli_tok*) ALLOCPV(type_tok, MAXSYMLEN);

	QLI_line = (qli_line*) ALLOCPV(type_line, 0);
	QLI_line->line_size = sizeof(QLI_line->line_data);
	QLI_line->line_ptr = QLI_line->line_data;
	QLI_line->line_type = line_stdin;
	QLI_line->line_source_file = stdin;

	QLI_semi = false;
	input_file = stdin;
	HSH_init();
}


void LEX_mark_statement()
{
/**************************************
 *
 *	L E X _ m a r k _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Push a position in the trace file onto
 *	the statement stack.
 *
 **************************************/
	qli_line* temp;

	for (temp = QLI_line; temp->line_next && QLI_statements; temp = temp->line_next)
	{
		if (temp->line_next->line_position == (IPTR) QLI_statements->lls_object)
			return;
	}

	qli_lls* statement = (qli_lls*) ALLOCP(type_lls);
	statement->lls_object = (BLK)(IPTR) temp->line_position;
	statement->lls_next = QLI_statements;
	QLI_statements = statement;
}


void LEX_pop_line()
{
/**************************************
 *
 *	L E X _ p o p _ l i n e
 *
 **************************************
 *
 * Functional description
 *	We're done with the current line source.  Close it out
 *	and release the line block.
 *
 **************************************/
	qli_line* const temp = QLI_line;
	QLI_line = temp->line_next;

	if (temp->line_type == line_blob)
		PRO_close(temp->line_database, temp->line_source_blob);
	else if (temp->line_type == line_file)
		fclose(temp->line_source_file);

	ALLQ_release((qli_frb*) temp);
}


void LEX_procedure( qli_dbb* database, FB_API_HANDLE blob)
{
/**************************************
 *
 *	L E X _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Push a blob-resident procedure onto the input line source
 *	stack;
 *
 **************************************/
	qli_line* temp = (qli_line*) ALLOCPV(type_line, QLI_token->tok_length);
	temp->line_source_blob = blob;
	strncpy(temp->line_source_name, QLI_token->tok_string, QLI_token->tok_length);
	temp->line_type = line_blob;
	temp->line_database = database;
	temp->line_size = sizeof(temp->line_data);
	temp->line_ptr = temp->line_data;
	temp->line_position = QLI_position;
	temp->line_next = QLI_line;
	QLI_line = temp;
}


bool LEX_push_file(const TEXT* filename, const bool error_flag)
{
/**************************************
 *
 *	L E X _ p u s h _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Open a command file.  If the open succeedes, push the input
 *	source onto the source stack.  If the open fails, give an error
 *	if the error flag is set, otherwise return quietly.
 *
 **************************************/
	FILE *file = fopen(filename, FOPEN_INPUT_TYPE);
	if (!file)
	{
	    TEXT buffer[64];
		sprintf(buffer, "%s.com", filename);
		if (!(file = fopen(buffer, FOPEN_INPUT_TYPE)))
		{
			if (error_flag)
				ERRQ_msg_put(67, filename);
				// Msg 67 can't open command file \"%s\"\n
			return false;
		}
	}

	qli_line* line = (qli_line*) ALLOCPV(type_line, strlen(filename));
	line->line_type = line_file;
	line->line_source_file = file;
	line->line_size = sizeof(line->line_data);
	line->line_ptr = line->line_data;
	*line->line_ptr = 0;
	strcpy(line->line_source_name, filename);
	line->line_next = QLI_line;
	QLI_line = line;

	return true;
}


bool LEX_push_string(const TEXT* const string)
{
/**************************************
 *
 *	L E X _ p u s h _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Push a simple string on as an input source.
 *
 **************************************/
	qli_line* line = (qli_line*) ALLOCPV(type_line, 0);
	line->line_type = line_string;
	line->line_size = strlen(string);
	line->line_ptr = line->line_data;
	strcpy(line->line_data, string);
	line->line_data[line->line_size] = '\n';
	line->line_next = QLI_line;
	QLI_line = line;

	return true;
}


void LEX_put_procedure(FB_API_HANDLE blob, SLONG start, SLONG stop)
{
/**************************************
 *
 *	L E X _ p u t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Write text from the scratch trace file into a blob.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;

	if (fseek(trace_file, start, 0))
	{
		fseek(trace_file, 0, 2);
		IBERROR(62);			// Msg 62 fseek failed
	}

	int length = stop - start;

    TEXT buffer[1024];

	while (length)
	{
		TEXT* p = buffer;
		while (length)
		{
			--length;
			const SSHORT c = getc(trace_file);
			*p++ = c;
			if (c == '\n')
			{
#ifdef WIN_NT
				// account for the extra line-feed on OS/2 and Windows NT

				if (length)
					--length;
#endif
				break;
			}
		}
		const SSHORT l = p - buffer;
		if (l && isc_put_segment(status_vector, &blob, l, buffer))
			ERRQ_bugcheck(58);	// Msg 58 isc_put_segment failed
	}

	fseek(trace_file, 0, 2);
}


void LEX_real()
{
/**************************************
 *
 *	L E X _ r e a l
 *
 **************************************
 *
 * Functional description
 *	If the token is an end of line, get the next token.
 *
 **************************************/

	while (QLI_token->tok_type == tok_eol)
		LEX_token();
}


qli_lls* LEX_statement_list()
{
/**************************************
 *
 *	L E X _ s t a t e m e n t _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Somebody outside needs to know
 *	where the top of the statement list
 *	is.
 *
 **************************************/

	return QLI_statements;
}


qli_tok* LEX_token()
{
/**************************************
 *
 *	L E X _ t o k e n
 *
 **************************************
 *
 * Functional description
 *	Parse and return the next token.
 *
 **************************************/
	qli_tok* token = QLI_token;
	TEXT* p = token->tok_string;

	// Get next significant byte.  If it's the last EOL of a blob, throw it away

	SSHORT c;

	for (;;)
	{
		c = skip_white();
		if (c != '\n' || QLI_line->line_type != line_blob)
			break;
		qli_line* prior = QLI_line;
		next_line(true);
		if (prior == QLI_line)
			break;
	}

	// If we hit end of file, make up a phoney token

	if (!QLI_line)
	{
		const TEXT* q = eof_string;
		while (*p++ = *q++);
		token->tok_type = tok_eof;
		token->tok_keyword = KW_none;
		return NULL;
	}

	*p++ = c;
	QLI_token->tok_position = QLI_line->line_position + QLI_line->line_ptr - QLI_line->line_data - 1;

	// On end of file, generate furious but phone end of line tokens

	char char_class = classes(c);

	if (char_class & CHR_letter)
	{
		for (c = nextchar(true); classes(c) & CHR_ident; c = nextchar(true))
			*p++ = c;
		retchar();
		token->tok_type = tok_ident;
	}
	else if (((char_class & CHR_digit) || c == '.') && scan_number(c, &p))
		token->tok_type = tok_number;
	else if (char_class & CHR_quote)
	{
		token->tok_type = tok_quoted;
		while (true)
		{
			const SSHORT next = nextchar(false);
			if (!next || next == '\n')
			{
				retchar();
				IBERROR(63);	// Msg 63 unterminated quoted string
				break;
			}
			*p++ = next;
			if ((p - token->tok_string) >= MAXSYMLEN)
				ERRQ_msg_put(470, SafeArg() << MAXSYMLEN);	// Msg 470 literal too long

			// If there are 2 quotes in a row, interpret 2nd as a literal

			if (next == c)
			{
				const SSHORT peek = nextchar(false);
				retchar();
				if (peek != c)
					break;
				nextchar(false);
			}
		}
	}
	else if (c == '\n')
	{
	    // end of line, signal it properly with a phoney token.
		token->tok_type = tok_eol;
		--p;
		const TEXT* q = eol_string;
		while (*q)
			*p++ = *q++;
	}
	else
	{
		token->tok_type = tok_punct;
		*p++ = nextchar(true);
		if (!HSH_lookup(token->tok_string, 2))
		{
			retchar();
			--p;
		}
	}

	token->tok_length = p - token->tok_string;
	*p = '\0';

	if (token->tok_string[0] == '$' && trans_limit < TRANS_LIMIT)
	{
		Firebird::string s;
		if (fb_utils::readenv(token->tok_string + 1, s))
		{
			LEX_push_string(s.c_str());
			++trans_limit;
			token = LEX_token();
			--trans_limit;
			return token;
		}
	}

    qli_symbol* symbol = HSH_lookup(token->tok_string, token->tok_length);
	token->tok_symbol = symbol;
	if (symbol && symbol->sym_type == SYM_keyword)
		token->tok_keyword = (kwwords) symbol->sym_keyword;
	else
		token->tok_keyword = KW_none;

	if (sw_trace)
		puts(token->tok_string);

	return token;
}


static bool get_line(FILE* file, TEXT* buffer, USHORT size)
{
/**************************************
 *
 *	g e t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Read a line.  If the line is terminated by
 *	an EOL, return true.  If the buffer is exhausted and non-blanks
 *	would be discarded, return an error.  If EOF is detected,
 *	return false.  Regardless, a null terminated string is returned.
 *
 **************************************/
	bool overflow_flag = false;
	SSHORT c;

	errno = 0;
	TEXT* p = buffer;
	SLONG length = size;

	while (true)
	{
		c = getc(file);
		if (c == EOF)
		{
			if (SYSCALL_INTERRUPTED(errno) && !QLI_abort)
			{
				errno = 0;
				continue;
			}
			if (QLI_abort)
				continue;

			break;
		}
		if (--length > 0)
			*p++ = c;
		else if (c != ' ' && c != '\n')
			overflow_flag = true;
		if (c == '\n')
			break;
	}

	*p = 0;
	if (c == EOF)
		return false;

	if (overflow_flag)
		IBERROR(477);			// Msg 477 input line too long

	if (sw_verify)
		fputs(buffer, stdout);

	return true;
}


static int nextchar(const bool eof_ok)
{
/**************************************
 *
 *	n e x t c h a r
 *
 **************************************
 *
 * Functional description
 *	Get the next character from the input stream.
 *
 **************************************/
	// Get the next character in the current line.  If we run out,
	// get the next line.  If the line source runs out, pop the
	// line source.  If we run out of line sources, we are distinctly
	// at end of file.

	while (QLI_line)
	{
		const SSHORT c = *QLI_line->line_ptr++;
		if (c)
			return c;
		next_line(eof_ok);
	}

	return 0;
}


static void next_line(const bool eof_ok)
{
/**************************************
 *
 *	n e x t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Get the next line from the input stream.
 *
 **************************************/
	TEXT* p;

	while (QLI_line)
	{
		bool flag = false;

		// Get next line from where ever.  If it comes from either the terminal
		//   or command file, check for another command file.

		if (QLI_line->line_type == line_blob)
		{
			// If the current blob segment contains another line, use it

			if ((p = QLI_line->line_ptr) != QLI_line->line_data && p[-1] == '\n' && *p)
			{
				flag = true;
			}
			else
			{
				// Initialize line block for retrieval

				p = QLI_line->line_data;
				QLI_line->line_ptr = QLI_line->line_data;

				flag = PRO_get_line(QLI_line->line_source_blob, p, QLI_line->line_size);
				if (flag && QLI_echo)
					printf("%s", QLI_line->line_data);
			}
		}
		else
		{
			// Initialize line block for retrieval

			QLI_line->line_ptr = QLI_line->line_data;
			p = QLI_line->line_data;

			if (QLI_line->line_type == line_stdin)
				flag = LEX_get_line(QLI_prompt, p, (int) QLI_line->line_size);
			else if (QLI_line->line_type == line_file)
			{
				flag = get_line(QLI_line->line_source_file, p, QLI_line->line_size);
				if (QLI_echo)
					printf("%s", QLI_line->line_data);
			}
			if (flag)
			{
				TEXT* q;
				for (q = p; classes(*q) & CHR_white; q++);
				if (*q == '@')
				{
					TEXT filename[MAXPATHLEN];
					for (p = q + 1, q = filename; *p && *p != '\n';)
						*q++ = *p++;
					*q = 0;
					QLI_line->line_ptr = p;
					LEX_push_file(filename, true);
					continue;
				}
			}
		}

		// If get got a line, we're almost done

		if (flag)
			break;

		// We hit end of file.  Either complain about the circumstances
		//   or just close out the current input source.  Don't close the
		//   input source if it's the terminal and we're at a continuation
		//   prompt.

		if (eof_ok && (QLI_line->line_next || QLI_prompt != QLI_cont_string))
		{
			LEX_pop_line();
			return;
		}

		// this is an unexpected end of file

		switch (QLI_line->line_type)
		{
		case line_blob:
			ERRQ_print_error(64, QLI_line->line_source_name);
			// Msg 64 unexpected end of procedure in procedure %s
			break;
		case line_file:
			ERRQ_print_error(65, QLI_line->line_source_name);
			// Msg 65 unexpected end of file in file %s
			break;
		default:
			if (QLI_line->line_type == line_string)
				LEX_pop_line();
			IBERROR(66);		// Msg 66 unexpected eof
		}
	}

	if (!QLI_line)
		return;

	QLI_line->line_position = QLI_position;

	// Dump output to the trace file

	if (QLI_line->line_type == line_blob)
	{
		while (*p)
			p++;
	}
	else
	{
		while (*p)
			putc(*p++, trace_file);
		QLI_position += (TEXT*) p - QLI_line->line_data;
#ifdef WIN_NT
		// account for the extra line-feed on OS/2 and Windows NT
		//   to determine file position

		QLI_position++;
#endif
	}

	QLI_line->line_length = (TEXT*) p - QLI_line->line_data;
}


static void retchar()
{
/**************************************
 *
 *	r e t c h a r
 *
 **************************************
 *
 * Functional description
 *	Return a character to the input stream.
 *
 **************************************/

	// CVC: Too naive implementation: what if the pointer is at the beginning?
	fb_assert(QLI_line);
	--QLI_line->line_ptr;
}


static bool scan_number(SSHORT c, TEXT** ptr)
{
/**************************************
 *
 *	s c a n _ n u m b e r
 *
 **************************************
 *
 * Functional description
 *	Pass on a possible numeric literal.
 *
 **************************************/
	bool dot = false;

	// If this is a leading decimal point, check that the next
	//   character is really a digit, otherwise backout

	if (c == '.')
	{
		c = nextchar(true);
		retchar();
		if (!(classes(c) & CHR_digit))
			return false;
		dot = true;
	}

	TEXT* p = *ptr;

	// Gobble up digits up to a single decimal point

	for (;;)
	{
		c = nextchar(true);
		if (classes(c) & CHR_digit)
			*p++ = c;
		else if (!dot && c == '.')
		{
			*p++ = c;
			dot = true;
		}
		else
			break;
	}

	// If this is an exponential, eat the exponent sign and digits

	if (UPPER(c) == 'E')
	{
		*p++ = c;
		c = nextchar(true);
		if (c == '+' || c == '-')
		{
			*p++ = c;
			c = nextchar(true);
		}
		while (classes(c) & CHR_digit)
		{
			*p++ = c;
			c = nextchar(true);
		}
	}

	retchar();
	*ptr = p;

	return true;
}


static int skip_white()
{
/**************************************
 *
 *	s k i p _ w h i t e
 *
 **************************************
 *
 * Functional description
 *	Skip over while space and comments in input stream
 *
 **************************************/
	SSHORT c;

	while (true)
	{
		c = nextchar(true);
		const char char_class = classes(c);
		if (char_class & CHR_white)
			continue;
		if (c == '/')
		{
		    SSHORT next = nextchar(true);
			if (next != '*')
			{
				retchar();
				return c;
			}
			c = nextchar(false);
			while ((next = nextchar(false)) && !(c == '*' && next == '/'))
				c = next;
			continue;
		}
		break;
	}

	return c;
}


