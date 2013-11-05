/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		preparse.cpp
 *	DESCRIPTION:	Dynamic SQL pre parser / parser on client side.
 *			This module will probably change to a YACC parser.
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
#include <stdlib.h>
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../dsql/chars.h"
#include "../dsql/sqlda.h"
#include "../dsql/prepa_proto.h"
#include "../dsql/utld_proto.h"
#include "../jrd/gds_proto.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/StatusArg.h"

enum pp_vals {
	PP_CREATE = 0,
	PP_DATABASE = 1,
	PP_SCHEMA = 2,
	PP_PAGE_SIZE = 3,
	PP_USER = 4,
	PP_PASSWORD = 5,
	PP_PAGESIZE = 6,
	PP_LENGTH = 7,
	PP_PAGES = 8,
	PP_PAGE = 9,
	PP_SET = 10,
	PP_NAMES = 11
};


const size_t MAX_TOKEN_SIZE = 1024;
static void generate_error(ISC_STATUS*, const Firebird::string&, SSHORT, SSHORT);
static SSHORT get_next_token(const SCHAR**, const SCHAR*, Firebird::string&);
static SSHORT get_token(ISC_STATUS*, SSHORT, bool, const SCHAR**, const SCHAR* const,
	Firebird::string&);

struct pp_table
{
	SCHAR symbol[10];
	USHORT length;
	SSHORT code;
};

static const pp_table pp_symbols[] =
{
	{"CREATE", 6, PP_CREATE},
	{"DATABASE", 8, PP_DATABASE},
	{"SCHEMA", 6, PP_SCHEMA},
	{"PAGE_SIZE", 9, PP_PAGE_SIZE},
	{"USER", 4, PP_USER},
	{"PASSWORD", 8, PP_PASSWORD},
	{"PAGESIZE", 8, PP_PAGESIZE},
	{"LENGTH", 6, PP_LENGTH},
	{"PAGES", 5, PP_PAGES},
	{"PAGE", 4, PP_PAGE},
	{"SET", 3, PP_SET},
	{"NAMES", 5, PP_NAMES},
	{"", 0, 0}
};

// define the tokens

enum token_vals {
	NO_MORE_TOKENS = -1,
	TOKEN_TOO_LONG = -2,
	UNEXPECTED_END_OF_COMMAND = -3,
	UNEXPECTED_TOKEN = -4,
	STRING = 257,
	NUMERIC = 258,
	SYMBOL = 259
};

using namespace Firebird;


/**

 	PREPARSE_execute

    @brief

    @param user_status
    @param db_handle
    @param trans_handle
    @param stmt_length
    @param stmt
    @param stmt_eaten
    @param dialect

 **/
bool PREPARSE_execute(ISC_STATUS* user_status, FB_API_HANDLE* db_handle,
					  FB_API_HANDLE*, // trans_handle,
					  USHORT stmt_length, const SCHAR* stmt, bool* stmt_eaten, USHORT dialect)
{
	// no use creating separate pool for a couple of strings
	ContextPoolHolder context(getDefaultMemoryPool());

	try
	{
		if (!stmt)
		{
			Arg::Gds(isc_command_end_err).raise();
		}

		if (!stmt_length)
			stmt_length = strlen(stmt);
		const char* const stmt_end = stmt + stmt_length;
		string token;

		if (get_token(user_status, SYMBOL, false, &stmt, stmt_end, token) ||
			token.length() != pp_symbols[PP_CREATE].length || token != pp_symbols[PP_CREATE].symbol)
		{
			return false;
		}

		if (get_token(user_status, SYMBOL, false, &stmt, stmt_end, token) ||
			(token.length() != pp_symbols[PP_DATABASE].length &&
				token.length() != pp_symbols[PP_SCHEMA].length) ||
			(token != pp_symbols[PP_DATABASE].symbol && token != pp_symbols[PP_SCHEMA].symbol))
		{
			return false;
		}

		if (get_token(user_status, STRING, false, &stmt, stmt_end, token))
		{
			return true;
		}

		PathName file_name(token.ToPathName());
		*stmt_eaten = false;
		ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);

		dpb.insertByte(isc_dpb_overwrite, 0);
		dpb.insertInt(isc_dpb_sql_dialect, dialect);

		SLONG page_size = 0;
		bool matched;
		do {
			const SSHORT result = get_next_token(&stmt, stmt_end, token);
			if (result == NO_MORE_TOKENS) {
				*stmt_eaten = true;
				break;
			}

			if (result < 0)
				break;

			matched = false;
			for (int i = 3; pp_symbols[i].length && !matched; i++)
			{
				if (token.length() == pp_symbols[i].length && token == pp_symbols[i].symbol)
				{
					bool get_out = false;
					// CVC: What's strange, this routine doesn't check token.length()
					// but it proceeds blindly, trying to exhaust the token itself.

					switch (pp_symbols[i].code)
					{
					case PP_PAGE_SIZE:
					case PP_PAGESIZE:
						if (get_token(user_status, '=', true, &stmt, stmt_end, token) ||
							get_token(user_status, NUMERIC, false, &stmt, stmt_end, token))
						{
							get_out = true;
							break;
						}
						page_size = atol(token.c_str());
						dpb.insertInt(isc_dpb_page_size, page_size);
						matched = true;
						break;

					case PP_USER:
						if (get_token(user_status, STRING, false, &stmt, stmt_end, token))
						{
							get_out = true;
							break;
						}

						dpb.insertString(isc_dpb_user_name, token);
						matched = true;
						break;

					case PP_PASSWORD:
						if (get_token(user_status, STRING, false, &stmt, stmt_end, token))
						{
							get_out = true;
							break;
						}

						dpb.insertString(isc_dpb_password, token);
						matched = true;
						break;

					case PP_SET:
						if (get_token(user_status, SYMBOL, false, &stmt, stmt_end, token) ||
							token.length() != pp_symbols[PP_NAMES].length ||
							token != pp_symbols[PP_NAMES].symbol ||
							get_token(user_status, STRING, false, &stmt, stmt_end, token))
						{
							get_out = true;
							break;
						}

						dpb.insertString(isc_dpb_lc_ctype, token);
						matched = true;
						break;

					case PP_LENGTH:
						// Skip a token for value

						if (get_token(user_status, '=', true, &stmt, stmt_end, token) ||
							get_token(user_status, NUMERIC, false, &stmt, stmt_end, token))
						{
							get_out = true;
							break;
						}

						matched = true;
						break;

					case PP_PAGE:
					case PP_PAGES:
						matched = true;
						break;
					}

					if (get_out) {
						return true;
					}
				} // if
			} // for

		} while (matched);

		// This code is because 3.3 server does not recognize isc_dpb_overwrite.
		FB_API_HANDLE temp_db_handle = 0;
		if (!isc_attach_database(user_status, 0, file_name.c_str(), &temp_db_handle,
				dpb.getBufferLength(), reinterpret_cast<const ISC_SCHAR*>(dpb.getBuffer())) ||
			(user_status[1] != isc_io_error && user_status[1] != isc_conf_access_denied))
		{
			if (!user_status[1])
			{
				// Swallow status from detach.
				ISC_STATUS_ARRAY temp_status;
				isc_detach_database(temp_status, &temp_db_handle);
			}
			if (!user_status[1] || user_status[1] == isc_bad_db_format)
			{
				user_status[0] = isc_arg_gds;
				user_status[1] = isc_io_error;
				user_status[2] = isc_arg_string;
				user_status[3] = (ISC_STATUS) "open";
				user_status[4] = isc_arg_string;
				user_status[5] = (ISC_STATUS) file_name.c_str();
				user_status[6] = isc_arg_gds;
				user_status[7] = isc_db_or_file_exists;
				user_status[8] = isc_arg_end;
				UTLD_save_status_strings(user_status);
			}
			return true;
		}

		isc_create_database(user_status, 0, file_name.c_str(), db_handle,
							dpb.getBufferLength(), reinterpret_cast<const ISC_SCHAR*>(dpb.getBuffer()),
							0);
	}
	catch (const Exception& ex)
	{
		ex.stuff_exception(user_status);
		return true;
	}

	return true;
}


/**

 	generate_error

    @brief

    @param user_status
    @param token
    @param error
    @param result

 **/
static void generate_error(ISC_STATUS* user_status, const string& token, SSHORT error, SSHORT result)
{
	string err_string;

	user_status[0] = isc_arg_gds;
	user_status[1] = isc_sqlerr;
	user_status[2] = isc_arg_number;
	user_status[3] = -104;
	user_status[4] = isc_arg_gds;

	switch (error)
	{
	case UNEXPECTED_END_OF_COMMAND:
		user_status[5] = isc_command_end_err;
		user_status[6] = isc_arg_end;
		break;

	case UNEXPECTED_TOKEN:
	case TOKEN_TOO_LONG:
		if (result) {
			err_string.assign(1, (TEXT) result);
			err_string += token;
			err_string += (TEXT) result;
		}
		else
			err_string = token;
		user_status[5] = isc_token_err;
		user_status[6] = isc_arg_gds;
		user_status[7] = isc_random;
		user_status[8] = isc_arg_string;
		user_status[9] = (ISC_STATUS)(err_string.c_str());
		user_status[10] = isc_arg_end;
		UTLD_save_status_strings(user_status);
		break;
	}
}


/**

 	get_next_token

    @brief

    @param stmt
    @param stmt_end
    @param token

 **/
static SSHORT get_next_token(const SCHAR** stmt, const SCHAR* stmt_end, string& token)
{
	UCHAR c, char_class = 0;

	token.erase();
	const SCHAR* s = *stmt;

	for (;;)
	{
		if (s >= stmt_end)
		{
			*stmt = s;
			return NO_MORE_TOKENS;
		}

		c = *s++;

		if (c == '/' && s < stmt_end && *s == '*')
		{
			s++;
			while (s < stmt_end) {
				c = *s++;
				if (c == '*' && s < stmt_end && *s == '/')
					break;
			}
			s++;
			continue;
		}

		// CVC: Dmitry told me to leave this in peace, but if somebody wants
		// to experiment ignoring single line comments, here's an idea.
		if (c == '-' && s < stmt_end && *s == '-')
		{
			s++;
			while (s < stmt_end)
			{
				c = *s++;
				if (c == '\n')
					break;
			}
			continue;
		}
		// CVC: End modification.

		char_class = classes(c);
		if (!(char_class & CHR_WHITE))
			break;
	}

	// At this point c contains character and class contains character class.
	// s is pointing to next character.

	const SCHAR* const start_of_token = s - 1;

	// In here we handle only 4 cases, STRING, INTEGER, arbitrary
	// SYMBOL and single character punctuation.

	if (char_class & CHR_QUOTE)
	{
		for (;;)
		{
			if (s >= stmt_end)
				return UNEXPECTED_END_OF_COMMAND;

			// *s is quote - if next != quote we're at the end

			if ((*s == c) && ((++s == stmt_end) || (*s != c)))
				break;
			token += *s++;
		}
		*stmt = s;
		if (token.length() > MAX_TOKEN_SIZE) {
			// '=' used as then there is no place for null termination
			token.erase(MAX_TOKEN_SIZE);
			return TOKEN_TOO_LONG;
		}
		return STRING;
	}

	// Is it an integer?

	if (char_class & CHR_DIGIT)
	{
		for (; s < stmt_end && (classes(c = *s) & CHR_DIGIT); ++s); // empty body
		fb_assert(s >= start_of_token);
		const size_t length = (s - start_of_token);
		*stmt = s;
		if (length > MAX_TOKEN_SIZE) {
			token.assign(start_of_token, MAX_TOKEN_SIZE);
			return TOKEN_TOO_LONG;
		}
		token.assign(start_of_token, length);
		return NUMERIC;
	}

	// Is is a symbol?

	if (char_class & CHR_LETTER)
	{
		token += UPPER(c);
		for (; s < stmt_end && (classes(*s) & CHR_IDENT); s++) {
			token += UPPER(*s);
		}

		*stmt = s;
		if (token.length() > MAX_TOKEN_SIZE) {
			token.erase(MAX_TOKEN_SIZE);
			return TOKEN_TOO_LONG;
		}
		return SYMBOL;
	}

	// What remains at this point for us is the single character punctuation.

	*stmt = s;

	return (c == ';' ? NO_MORE_TOKENS : c);
}


/**

 	get_token

    @brief

    @param status
    @param token_type
    @param optional
    @param stmt
    @param stmt_end
    @param token

 **/
static SSHORT get_token(ISC_STATUS* status,
						SSHORT token_type,
						bool optional,
						const SCHAR** stmt,
						const SCHAR* const stmt_end,
						string& token)
{
	const SCHAR* temp_stmt = *stmt;
	const SSHORT result = get_next_token(&temp_stmt, stmt_end, token);

	switch (result)
	{
	case NO_MORE_TOKENS:
		*stmt = temp_stmt;
		generate_error(status, token, UNEXPECTED_END_OF_COMMAND, 0);
		return FB_FAILURE;

	case UNEXPECTED_END_OF_COMMAND:
	case TOKEN_TOO_LONG:
		*stmt = temp_stmt;

		// generate error here

		generate_error(status, token, result, 0);
		return FB_FAILURE;
	}

	// Some token was found

	if (result == token_type) {
		*stmt = temp_stmt;
		return FB_SUCCESS;
	}

	if (optional)
		return FB_SUCCESS;

	// generate error here and return failure;

	*stmt = temp_stmt;
	generate_error(status, token, UNEXPECTED_TOKEN,
				   (result == STRING) ? *(temp_stmt - 1) : 0);
	return FB_FAILURE;
}

