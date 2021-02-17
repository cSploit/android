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

#include "../dsql/dsql.h"
#include "../dsql/DdlNodes.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/AggNodes.h"
#include "../dsql/WinNodes.h"
#include "../dsql/PackageNodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/RecordSourceNodes.h"
#include "../common/classes/Nullable.h"
#include "../common/classes/stack.h"

#include "gen/parse.h"

namespace Jrd {

class Parser : public Firebird::PermanentStorage
{
private:
	// User-defined text position type.
	struct Position
	{
		ULONG firstLine;
		ULONG firstColumn;
		ULONG lastLine;
		ULONG lastColumn;
		const char* firstPos;
		const char* lastPos;
	};

	typedef Position YYPOSN;
	typedef int Yshort;

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
		int dsql_debug;

		// Actual lexer state begins from here

		const TEXT* ptr;
		const TEXT* end;
		const TEXT* last_token;
		const TEXT* start;
		const TEXT* line_start;
		const TEXT* last_token_bk;
		const TEXT* line_start_bk;
		SSHORT att_charset;
		SLONG lines, lines_bk;
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
		IntlString* str;
	};

public:
	static const int MAX_TOKEN_LEN = 256;

public:
	Parser(MemoryPool& pool, DsqlCompilerScratch* aScratch, USHORT aClientDialect,
		USHORT aDbDialect, USHORT aParserVersion, const TEXT* string, size_t length,
		SSHORT characterSet);
	~Parser();

public:
	dsql_req* parse();

	const Firebird::string& getTransformedString() const
	{
		return transformedString;
	}

	bool isStmtAmbiguous() const
	{
		return stmt_ambiguous;
	}

	Firebird::string* newString(const Firebird::string& s)
	{
		return FB_NEW(getPool()) Firebird::string(getPool(), s);
	}

	IntlString* newIntlString(const Firebird::string& s, const char* charSet = NULL)
	{
		return FB_NEW(getPool()) IntlString(getPool(), s, charSet);
	}

	// newNode overloads

	template <typename T>
	T* newNode()
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool()));
	}

	template <typename T, typename T1>
	T* newNode(T1 a1)
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool(), a1));
	}

	template <typename T, typename T1, typename T2>
	T* newNode(T1 a1, T2 a2)
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool(), a1, a2));
	}

	template <typename T, typename T1, typename T2, typename T3>
	T* newNode(T1 a1, T2 a2, T3 a3)
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool(), a1, a2, a3));
	}

	template <typename T, typename T1, typename T2, typename T3, typename T4>
	T* newNode(T1 a1, T2 a2, T3 a3, T4 a4)
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool(), a1, a2, a3, a4));
	}

	template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
	T* newNode(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
	{
		return setupNode<T>(FB_NEW(getPool()) T(getPool(), a1, a2, a3, a4, a5));
	}

private:
	template <typename T> T* setupNode(Node* node)
	{
		node->line = (ULONG) lex.lines_bk;
		node->column = (ULONG) (lex.last_token_bk - lex.line_start_bk + 1);
		return static_cast<T*>(node);
	}

	// Overload to not make LineColumnNode data wrong after its construction.
	template <typename T> T* setupNode(LineColumnNode* node)
	{
		return node;
	}

	// Overload for non-Node classes constructed with newNode.
	template <typename T> T* setupNode(void* node)
	{
		return static_cast<T*>(node);
	}

	BoolExprNode* valueToBool(ValueExprNode* value)
	{
		BoolAsValueNode* node = value->as<BoolAsValueNode>();
		if (node)
			return node->boolean;

		ComparativeBoolNode* cmpNode = newNode<ComparativeBoolNode>(
			blr_eql, value, MAKE_constant("1", CONSTANT_BOOLEAN));
		cmpNode->dsqlWasValue = true;
		return cmpNode;
	}

	void yyReducePosn(YYPOSN& ret, YYPOSN* termPosns, YYSTYPE* termVals,
		int termNo, int stkPos, int yychar, YYPOSN& yyposn, void*);

	int yylex();
	bool yylexSkipSpaces();
	int yylexAux();

	void yyerror(const TEXT* error_string);
	void yyerror_detailed(const TEXT* error_string, int yychar, YYSTYPE&, YYPOSN&);
	void yyerrorIncompleteCmd();

	void check_bound(const char* const to, const char* const string);
	void check_copy_incr(char*& to, const char ch, const char* const string);

	void yyabandon(SLONG, ISC_STATUS);

	Firebird::MetaName optName(Firebird::MetaName* name)
	{
		return (name ? *name : Firebird::MetaName());
	}

	void transformString(const char* start, unsigned length, Firebird::string& dest);
	Firebird::string makeParseStr(const Position& p1, const Position& p2);
	ParameterNode* make_parameter();

	// Set the value of a clause, checking if it was already specified.

	template <typename T>
	void setClause(T& clause, const char* duplicateMsg, const T& value)
	{
		checkDuplicateClause(clause, duplicateMsg);
		clause = value;
	}

	template <typename T, typename Delete>
	void setClause(Firebird::AutoPtr<T, Delete>& clause, const char* duplicateMsg, T* value)
	{
		checkDuplicateClause(clause, duplicateMsg);
		clause = value;
	}

	template <typename T>
	void setClause(Nullable<T>& clause, const char* duplicateMsg, const T& value)
	{
		checkDuplicateClause(clause, duplicateMsg);
		clause = value;
	}

	template <typename T1, typename T2>
	void setClause(NestConst<T1>& clause, const char* duplicateMsg, const T2& value)
	{
		setClause(*clause.getAddress(), duplicateMsg, value);
	}

	void setClause(bool& clause, const char* duplicateMsg)
	{
		setClause(clause, duplicateMsg, true);
	}

	template <typename T>
	void checkDuplicateClause(const T& clause, const char* duplicateMsg)
	{
		using namespace Firebird;
		if (isDuplicateClause(clause))
		{
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
				Arg::Gds(isc_dsql_duplicate_spec) << duplicateMsg);
		}
	}

	template <typename T>
	bool isDuplicateClause(const T& clause)
	{
		return clause != 0;
	}

	bool isDuplicateClause(const Firebird::MetaName& clause)
	{
		return clause.hasData();
	}

	template <typename T>
	bool isDuplicateClause(const Nullable<T>& clause)
	{
		return clause.specified;
	}

	template <typename T>
	bool isDuplicateClause(const Firebird::Array<T>& clause)
	{
		return clause.hasData();
	}

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

private:
	DsqlCompilerScratch* scratch;
	USHORT client_dialect;
	USHORT db_dialect;
	USHORT parser_version;

	Firebird::string transformedString;
	Firebird::GenericMap<Firebird::NonPooled<IntlString*, StrMark> > strMarks;
	bool stmt_ambiguous;
	dsql_req* DSQL_parse;

	// These value/posn are taken from the lexer
	YYSTYPE yylval;
	YYPOSN yyposn;

	// These value/posn of the root non-terminal are returned to the caller
	YYSTYPE yyretlval;
	Position yyretposn;

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
	Position* yylpsns;
	// Current posistion at lexical position queue
	Position* yylpp;
	// End position of lexical position queue
	Position* yylpe;
	// The last allocated position at the lexical position queue
	Position* yylplim;
	// Current position at lexical token queue
	Yshort* yylexp;
	Yshort* yylexemes;

public:
	LexerState lex;
};

} // namespace

#endif	// DSQL_PARSER_H
