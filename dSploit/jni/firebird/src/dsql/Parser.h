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

#ifndef DSQL_PARSER_H
#define DSQL_PARSER_H

#include "../jrd/common.h"
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../dsql/DdlNodes.h"
#include "../dsql/StmtNodes.h"
#include "../common/classes/stack.h"

namespace Jrd {

class dsql_nod;

class Parser : public Firebird::PermanentStorage
{
private:
	typedef int Yshort;
	typedef dsql_nod* YYSTYPE;
	typedef int YYPOSN;	// user-defined text position type

	struct yyparsestate
	{
	  yyparsestate* save;		// Previously saved parser state
	  int state;
	  int errflag;
	  Yshort* ssp;				// state stack pointer
	  YYSTYPE* vsp;				// value stack pointer
	  YYPOSN* psp;				// position stack pointer
	  YYSTYPE val;				// value as returned by actions
	  YYPOSN pos;				// position as returned by universal action
	  Yshort* ss;				// state stack base
	  YYSTYPE* vs;				// values stack base
	  YYPOSN* ps;				// position stack base
	  int lexeme;				// index of the conflict lexeme in the lexical queue
	  unsigned int stacksize;	// current maximum stack size
	  Yshort ctry;				// index in yyctable[] for this conflict
	};

	struct LexerState
	{
		// This is, in fact, parser state. Not used in lexer itself
		dsql_fld* g_field;
		dsql_fil* g_file;
		YYSTYPE g_field_name;
		int dsql_debug;

		// Actual lexer state begins from here

		// hvlad: if at some day 16 levels of nesting would be not enough
		// then someone must add LexerState constructor and pass memory
		// pool into Stack's constructor or change Capacity value in template
		// instantiation below
		Firebird::Stack<const TEXT*> beginnings;
		const TEXT* ptr;
		const TEXT* end;
		const TEXT* last_token;
		const TEXT* start;
		const TEXT* line_start;
		const TEXT* last_token_bk;
		const TEXT* line_start_bk;
		SSHORT lines, att_charset;
		SSHORT lines_bk;
		int prev_keyword;
		USHORT param_number;
	};

	struct StrMark
	{
		StrMark()
			: introduced(false),
			  pos(0),
			  length(0),
			  str(NULL)
		{
		}

		bool operator >(const StrMark& o) const
		{
			return pos > o.pos;
		}

		bool introduced;
		unsigned pos;
		unsigned length;
		dsql_str* str;
	};

public:
	Parser(MemoryPool& pool, USHORT aClientDialect, USHORT aDbDialect, USHORT aParserVersion,
		const TEXT* string, USHORT length, SSHORT characterSet);
	~Parser();

public:
	YYSTYPE parse();

	const Firebird::string& getTransformedString() const
	{
		return transformedString;
	}

	bool isStmtAmbiguous() const
	{
		return stmt_ambiguous;
	}

private:
	void transformString(const char* start, unsigned length, Firebird::string& dest);

// start - defined in btyacc_fb.ske
private:
	static void yySCopy(YYSTYPE* to, YYSTYPE* from, int size);
	static void yyPCopy(YYPOSN* to, YYPOSN* from, int size);
	static void yyMoreStack(yyparsestate* yyps);
	static yyparsestate* yyNewState(int size);
	static void yyFreeState(yyparsestate* p);

private:
	int parseAux();
	int yylex1();
	int yyexpand();
// end - defined in btyacc_fb.ske

// start - defined in parse.y
private:
	int yylex();
	int yylexAux();

	void yyerror(const TEXT* error_string);
	void yyerror_detailed(const TEXT* error_string, int yychar, YYSTYPE&, YYPOSN&);

	const TEXT* lex_position();
	YYSTYPE make_list (YYSTYPE node);
	YYSTYPE make_parameter();
	YYSTYPE make_node(Dsql::nod_t type, int count, ...);
	YYSTYPE makeClassNode(Node* node);
	YYSTYPE make_flag_node(Dsql::nod_t type, SSHORT flag, int count, ...);
// end - defined in parse.y

private:
	USHORT client_dialect;
	USHORT db_dialect;
	USHORT parser_version;

	Firebird::string transformedString;
	Firebird::GenericMap<Firebird::NonPooled<dsql_str*, StrMark> > strMarks;
	bool stmt_ambiguous;
	YYSTYPE DSQL_parse;

	// These value/posn are taken from the lexer
	YYSTYPE yylval;
	YYPOSN yyposn;

	// These value/posn of the root non-terminal are returned to the caller
	YYSTYPE yyretlval;
	int yyretposn;

	int yynerrs;

	// Current parser state
	yyparsestate* yyps;
	// yypath!=NULL: do the full parse, starting at *yypath parser state.
	yyparsestate* yypath;
	// Base of the lexical value queue
	YYSTYPE* yylvals;
	// Current posistion at lexical value queue
	YYSTYPE* yylvp;
	// End position of lexical value queue
	YYSTYPE* yylve;
	// The last allocated position at the lexical value queue
	YYSTYPE* yylvlim;
	// Base of the lexical position queue
	int* yylpsns;
	// Current posistion at lexical position queue
	int* yylpp;
	// End position of lexical position queue
	int* yylpe;
	// The last allocated position at the lexical position queue
	int* yylplim;
	// Current position at lexical token queue
	Yshort* yylexp;
	Yshort* yylexemes;

public:
	LexerState lex;
};

} // namespace

#endif	// DSQL_PARSER_H
