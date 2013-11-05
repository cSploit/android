/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// Lex.h: interface for the Lex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEX_H__89737590_2C93_4F77_99AD_4C3881906C96__INCLUDED_)
#define AFX_LEX_H__89737590_2C93_4F77_99AD_4C3881906C96__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Stream.h"
#include "../common/classes/alloc.h"

START_NAMESPACE

class InputStream;
class InputFile;
class Stream;

class Lex : public Firebird::GlobalStorage
{
public:
	enum LEX_flags { LEX_none = 0, LEX_trace = 1, LEX_list = 2, /* LEX_verbose = 4, */ LEX_upcase = 8 };

	enum TokenType
	{
		END_OF_STREAM,
		TT_PUNCT,
		TT_NAME,
		//QUOTED_NAME,
		TT_NUMBER,
		//TT_END,
		QUOTED_STRING,
		SINGLE_QUOTED_STRING,
		//DECIMAL_NUMBER,
		//IP_ADDRESS,
		TT_NONE
	};

	Lex(const char* punctuation, const LEX_flags debugFlags);
	virtual ~Lex();

	void captureStuff();
	int& charTable(int ch);
	bool getSegment();
	void pushStream (InputStream *stream);
	void setContinuationChar (char c);
	virtual void syntaxError (const char* expected);
	Firebird::string getName();
	Firebird::PathName reparseFilename();
	bool match (const char *word);
	bool isKeyword (const char *word) const;
	void setCommentString (const char *start, const char *end);
	void setLineComment (const char *string);
	void setCharacters (int type, const char *characters);
	void getToken();
	static bool match (const char *pattern, const char *string);
	void skipWhite();
protected:
	LEX_flags		flags;
	TokenType		tokenType;
	int				priorLineNumber;
	bool			eol;
	InputStream*	inputStream;
	InputStream*	priorInputStream;
private:
	enum { MAXTOKEN = 4096 };
	InputStream*	tokenInputStream;
	Stream			stuff;
	int				tokenOffset;
	char			captureStart;
	char			captureEnd;
	char			token [MAXTOKEN];
	int				lineNumber;
	int				tokenLineNumber;
	const char*		ptr;
	const char*		end;
	const char*		lineComment;
	const char*		commentStart;
	const char*		commentEnd;
	char			continuationChar;
	int				charTableArray [256];	// Don't use directly. Use through charTable.
};

END_NAMESPACE

#endif // !defined(AFX_LEX_H__89737590_2C93_4F77_99AD_4C3881906C96__INCLUDED_)
