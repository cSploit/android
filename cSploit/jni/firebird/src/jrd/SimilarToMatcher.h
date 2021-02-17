/*
 *	PROGRAM:	JRD International support
 *	MODULE:		SimilarToMatcher.h
 *	DESCRIPTION:	SIMILAR TO predicate
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
 *  Copyright (c) 2007 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_SIMILAR_TO_EVALUATOR_H
#define JRD_SIMILAR_TO_EVALUATOR_H

#include "../jrd/intl_classes.h"
#include "../jrd/evl_string.h"

// #define DEBUG_SIMILAR

#ifdef DEBUG_SIMILAR
// #define RECURSIVE_SIMILAR	// useless in production due to stack overflow
#endif

namespace Firebird
{

template <typename CharType, typename StrConverter = Jrd::CanonicalConverter<> >
class SimilarToMatcher : public Jrd::PatternMatcher
{
private:
	typedef Jrd::CharSet CharSet;
	typedef Jrd::TextType TextType;

	// This class is based on work of Zafir Anjum
	// http://www.codeguru.com/Cpp/Cpp/string/regex/article.php/c2791
	// which has been derived from work by Henry Spencer.
	//
	// The original copyright notice follows:
	//
	// Copyright (c) 1986, 1993, 1995 by University of Toronto.
	// Written by Henry Spencer.  Not derived from licensed software.
	//
	// Permission is granted to anyone to use this software for any
	// purpose on any computer system, and to redistribute it in any way,
	// subject to the following restrictions:
	//
	// 1. The author is not responsible for the consequences of use of
	// this software, no matter how awful, even if they arise
	// from defects in it.
	//
	// 2. The origin of this software must not be misrepresented, either
	// by explicit claim or by omission.
	//
	// 3. Altered versions must be plainly marked as such, and must not
	// be misrepresented (by explicit claim or omission) as being
	// the original software.
	//
	// 4. This notice must not be removed or altered.
	class Evaluator : private StaticAllocator
	{
	public:
		Evaluator(MemoryPool& pool, TextType* aTextType,
			const UCHAR* patternStr, SLONG patternLen,
			CharType aEscapeChar, bool aUseEscape);

		~Evaluator()
		{
			delete[] branches;
		}

		bool getResult();
		bool processNextChunk(const UCHAR* data, SLONG dataLen);
		void reset();

	private:
		enum Op
		{
			opRepeat,
			opRepeatEnd,
			opBranch,
			opStart,
			opEnd,
			opRef,
			opNothing,
			opAny,
			opAnyOf,
			opExactly,
			opExactlyOne,	// optimization for opExactly with a single character
			opRet			// implementation detail of the non-recursive match
			// If new codes are added, shifts in MatchState codes may need to change.
		};

		struct Node
		{
			explicit Node(Op aOp, const CharType* aStr = NULL, SLONG aLen = 0)
				: op(aOp),
				  str(aStr),
				  len(aLen),
				  str2(NULL),
				  len2(0),
				  str3(aStr),
				  len3(aLen),
				  str4(NULL),
				  len4(0),
				  ref(0),
				  branchNum(-1)
			{
			}

			Node(Op aOp, SLONG aLen1, SLONG aLen2, int aRef)
				: op(aOp),
				  str(NULL),
				  len(aLen1),
				  str2(NULL),
				  len2(aLen2),
				  str3(NULL),
				  len3(0),
				  str4(NULL),
				  len4(0),
				  ref(aRef),
				  branchNum(-1)
			{
			}

			Node(Op aOp, int aRef)
				: op(aOp),
				  str(NULL),
				  len(0),
				  str2(NULL),
				  len2(0),
				  str3(NULL),
				  len3(0),
				  str4(NULL),
				  len4(0),
				  ref(aRef),
				  branchNum(-1)
			{
			}

			Node(const Node& node)
				: op(node.op),
				  str(node.str),
				  len(node.len),
				  str2(node.str2),
				  len2(node.len2),
				  str3(node.str3),
				  len3(node.len3),
				  str4(node.str4),
				  len4(node.len4),
				  ref(node.ref),
				  branchNum(node.branchNum)
			{
			}

#ifdef DEBUG_SIMILAR
			void dump(string& text, int i) const
			{
				string temp;

				switch (op)
				{
					case opRepeat:
						temp.printf("opRepeat(%d, %d, %d)", len, len2, i + ref);
						break;

					case opRepeatEnd:
						temp.printf("opRepeatEnd(%d)", i + ref);
						break;

					case opBranch:
						if (branchNum == -1)
							temp.printf("opBranch(%d)", i + ref);
						else
							temp.printf("opBranch(%d, %d)", i + ref, branchNum);
						break;

					case opStart:
						temp = "opStart";
						break;

					case opEnd:
						temp = "opEnd";
						break;

					case opRef:
						if (branchNum == -1)
							temp.printf("opRef(%d)", i + ref);
						else
							temp.printf("opRef(%d, %d)", i + ref, branchNum);
						break;

					case opNothing:
						temp = "opNothing";
						break;

					case opAny:
						temp = "opAny";
						break;

					case opAnyOf:
						temp.printf("opAnyOf(%.*s, %d, %.*s, %d, %.*s, %d, %.*s, %d)",
							len, str, len, len2, str2, len2, len3, str3, len3, len4, str4, len4);
						break;

					case opExactly:
						temp.printf("opExactly(%.*s, %d)", len, str, len);
						break;

					case opExactlyOne:
						temp.printf("opExactlyOne(%.*s)", len, str);
						break;

					default:
						temp = "unknown";
						break;
				}

				text.printf("%d: %s", i, temp.c_str());
			}
#endif	// DEBUG_SIMILAR

			Op op;
			const CharType* str;
			SLONG len;
			const UCHAR* str2;
			SLONG len2;
			const CharType* str3;
			SLONG len3;
			const UCHAR* str4;
			SLONG len4;
			int ref;
			int branchNum;
		};

#ifndef RECURSIVE_SIMILAR
		// Struct used to evaluate expressions without recursion.
		// Represents local variables to implement a "virtual stack".
		struct Scope
		{
			inline explicit Scope(const Node* ai)
				: i(ai),
				  save(NULL)
			{
			}

			inline void operator =(const Node* ai)
			{
				i = ai;
				save = NULL;
			}

			const Node* i;
			const CharType* save;
		};

		// Stack for recursion emulation.
		template <typename T>
		class SimpleStack
		{
		public:
			SimpleStack()
				: size(INCREASE_FACTOR)
			{
				data = FB_NEW(*getDefaultMemoryPool()) UCHAR[(size + 1) * sizeof(T)];
				back = (T*) FB_ALIGN((U_IPTR)(UCHAR*) data, sizeof(T));
				end = back + size;

				// 'back' starts before initial element, then always points to the last pushed element.
				--back;
			}

			template <typename T2>
			inline void push(T2 node)
			{
				// If the limit is reached, resize.
				if (++back == end)
				{
					unsigned newSize = size + INCREASE_FACTOR;
					UCHAR* newData = FB_NEW(*getDefaultMemoryPool()) UCHAR[(newSize + 1) * sizeof(T)];

					T* p = (T*) FB_ALIGN((U_IPTR)(UCHAR*) newData, sizeof(T));
					memcpy(p, end - size, size * sizeof(T));

					back = p + size;
					end = p + newSize;
					size = newSize;

					data.reset(newData);
				}

				*back = node;
			}

			inline void pop()
			{
				--back;
			}

		public:
			T* back;

		private:
			static const unsigned INCREASE_FACTOR = 50;
			unsigned size;
			AutoPtr<UCHAR> data;
			T* end;
		};
#endif	// RECURSIVE_SIMILAR

		static const int FLAG_NOT_EMPTY	= 1;	// known never to match empty string
		static const int FLAG_EXACTLY	= 2;	// non-escaped string

	private:
		void parseExpr(int* flagp);
		void parseTerm(int* flagp);
		void parseFactor(int* flagp);
		void parsePrimary(int* flagp);
		bool isRep(CharType c) const;

		CharType canonicalChar(int ch) const
		{
			return *reinterpret_cast<const CharType*>(textType->getCanonicalChar(ch));
		}

#ifdef DEBUG_SIMILAR
		void dump() const;
#endif

	private:
#ifdef RECURSIVE_SIMILAR
		bool match(int start);
#else
		bool match();
#endif

	private:
		static SLONG notInSet(const CharType* str, SLONG strLen,
			const CharType* set, SLONG setLen);

	private:
		struct Range
		{
			unsigned start;
			unsigned length;
		};

#ifdef DEBUG_SIMILAR
		Array<char> debugLog;
		int debugLevel;
#endif

		TextType* textType;
		CharType escapeChar;
		bool useEscape;
		HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;
		const UCHAR* originalPatternStr;
		SLONG originalPatternLen;
		StrConverter patternCvt;
		CharSet* charSet;
		Array<Node> nodes;
		const CharType* patternStart;
		const CharType* patternEnd;
		const CharType* patternPos;
		const CharType* bufferStart;
		const CharType* bufferEnd;
		const CharType* bufferPos;
		CharType metaCharacters[15];

	public:
		unsigned branchNum;
		Range* branches;
	};

public:
	SimilarToMatcher(MemoryPool& pool, TextType* ttype, const UCHAR* str,
				SLONG strLen, CharType escape, bool useEscape)
		: PatternMatcher(pool, ttype),
		  evaluator(pool, ttype, str, strLen, escape, useEscape)
	{
	}

	void reset()
	{
		evaluator.reset();
	}

	bool result()
	{
		return evaluator.getResult();
	}

	bool process(const UCHAR* str, SLONG length)
	{
		return evaluator.processNextChunk(str, length);
	}

	unsigned getNumBranches()
	{
		return evaluator.branchNum;
	}

	void getBranchInfo(unsigned n, unsigned* start, unsigned* length)
	{
		fb_assert(n <= evaluator.branchNum);
		*start = evaluator.branches[n].start;
		*length = evaluator.branches[n].length;
	}

	static SimilarToMatcher* create(MemoryPool& pool, TextType* ttype,
		const UCHAR* str, SLONG length, const UCHAR* escape, SLONG escapeLen)
	{
		StrConverter cvt_escape(pool, ttype, escape, escapeLen);

		return FB_NEW(pool) SimilarToMatcher(pool, ttype, str, length,
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escapeLen != 0);
	}

	static bool evaluate(MemoryPool& pool, TextType* ttype, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escapeLen)
	{
		StrConverter cvt_escape(pool, ttype, escape, escapeLen);

		Evaluator evaluator(pool, ttype, p, pl,
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escapeLen != 0);
		evaluator.processNextChunk(s, sl);
		return evaluator.getResult();
	}

private:
	Evaluator evaluator;
};


template <typename CharType, typename StrConverter>
SimilarToMatcher<CharType, StrConverter>::Evaluator::Evaluator(
			MemoryPool& pool, TextType* aTextType,
			const UCHAR* patternStr, SLONG patternLen,
			CharType aEscapeChar, bool aUseEscape)
	: StaticAllocator(pool),
#ifdef DEBUG_SIMILAR
	  debugLog(pool),
	  debugLevel(-1),
#endif
	  textType(aTextType),
	  escapeChar(aEscapeChar),
	  useEscape(aUseEscape),
	  buffer(pool),
	  originalPatternStr(patternStr),
	  originalPatternLen(patternLen),
	  patternCvt(pool, textType, patternStr, patternLen),
	  charSet(textType->getCharSet()),
	  nodes(pool),
	  branchNum(0)
{
	fb_assert(patternLen % sizeof(CharType) == 0);
	patternLen /= sizeof(CharType);

	CharType* p = metaCharacters;
	*p++ = canonicalChar(TextType::CHAR_CIRCUMFLEX);
	*p++ = canonicalChar(TextType::CHAR_MINUS);
	*p++ = canonicalChar(TextType::CHAR_UNDERLINE);
	*p++ = canonicalChar(TextType::CHAR_PERCENT);
	*p++ = canonicalChar(TextType::CHAR_OPEN_BRACKET);
	*p++ = canonicalChar(TextType::CHAR_CLOSE_BRACKET);
	*p++ = canonicalChar(TextType::CHAR_OPEN_PAREN);
	*p++ = canonicalChar(TextType::CHAR_CLOSE_PAREN);
	*p++ = canonicalChar(TextType::CHAR_OPEN_BRACE);
	*p++ = canonicalChar(TextType::CHAR_CLOSE_BRACE);
	*p++ = canonicalChar(TextType::CHAR_VERTICAL_BAR);
	*p++ = canonicalChar(TextType::CHAR_QUESTION_MARK);
	*p++ = canonicalChar(TextType::CHAR_PLUS);
	*p++ = canonicalChar(TextType::CHAR_ASTERISK);
	if (useEscape)
		*p++ = escapeChar;
	else
		*p++ = canonicalChar(TextType::CHAR_ASTERISK);	// just repeat something
	fb_assert(p - metaCharacters == FB_NELEM(metaCharacters));

	patternStart = patternPos = (const CharType*) patternStr;
	patternEnd = patternStart + patternLen;

	nodes.push(Node(opStart));

	int flags;
	parseExpr(&flags);

	nodes.push(Node(opEnd));

#ifdef DEBUG_SIMILAR
	dump();
#endif

	// Check for proper termination.
	if (patternPos < patternEnd)
		status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

	branches = FB_NEW(pool) Range[branchNum + 1];

	reset();
}


template <typename CharType, typename StrConverter>
bool SimilarToMatcher<CharType, StrConverter>::Evaluator::getResult()
{
	const UCHAR* str = buffer.begin();
	SLONG len = buffer.getCount();

	// note that StrConverter changes str and len variables
	StrConverter cvt(pool, textType, str, len);
	fb_assert(len % sizeof(CharType) == 0);

	bufferStart = bufferPos = (const CharType*) str;
	bufferEnd = bufferStart + len / sizeof(CharType);

#ifdef DEBUG_SIMILAR
	debugLog.clear();
	debugLevel = -1;
#endif

	const bool matched =
#ifdef RECURSIVE_SIMILAR
		match(0);
#else
		match();
#endif

#ifdef DEBUG_SIMILAR
	if (matched)
	{
		for (unsigned i = 0; i <= branchNum; ++i)
		{
			string x;
			x.printf("%d: %d, %d\n", i, branches[i].start, branches[i].length);
			debugLog.add(x.c_str(), x.length());
		}

		debugLog.add('\0');

		gds__log("\n%s", debugLog.begin());
	}
#endif	// DEBUG_SIMILAR

	return matched;
}


template <typename CharType, typename StrConverter>
bool SimilarToMatcher<CharType, StrConverter>::Evaluator::processNextChunk(const UCHAR* data, SLONG dataLen)
{
	const size_t pos = buffer.getCount();
	memcpy(buffer.getBuffer(pos + dataLen) + pos, data, dataLen);
	return true;
}


template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::reset()
{
	buffer.shrink(0);

	memset(branches, 0, sizeof(Range) * (branchNum + 1));
}


template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::parseExpr(int* flagp)
{
	*flagp = FLAG_NOT_EMPTY;

	bool first = true;
	Array<int> refs;
	int start;

	while (first || (patternPos < patternEnd && *patternPos == canonicalChar(TextType::CHAR_VERTICAL_BAR)))
	{
		if (first)
			first = false;
		else
			++patternPos;

		int thisBranchNum = branchNum;
		start = nodes.getCount();
		nodes.push(Node(opBranch));
		nodes.back().branchNum = thisBranchNum;

		int flags;
		parseTerm(&flags);
		*flagp &= ~(~flags & FLAG_NOT_EMPTY);
		*flagp |= flags;

		refs.push(nodes.getCount());
		nodes.push(Node(opRef));
		nodes.back().branchNum = thisBranchNum;

		nodes[start].ref = nodes.getCount() - start;
	}

	nodes[start].ref = 0;

	for (Array<int>::iterator i = refs.begin(); i != refs.end(); ++i)
		nodes[*i].ref = nodes.getCount() - *i;
}


template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::parseTerm(int* flagp)
{
	*flagp = 0;

	bool first = true;
	CharType c;
	int flags;

	while ((patternPos < patternEnd) &&
		   (c = *patternPos) != canonicalChar(TextType::CHAR_VERTICAL_BAR) &&
		   c != canonicalChar(TextType::CHAR_CLOSE_PAREN))
	{
		parseFactor(&flags);

		*flagp |= flags & FLAG_NOT_EMPTY;

		if (first)
		{
			*flagp |= flags;
			first = false;
		}
	}

	if (first)
		nodes.push(Node(opNothing));
}


template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::parseFactor(int* flagp)
{
	int atomPos = nodes.getCount();

	int flags;
	parsePrimary(&flags);

	CharType op;

	if (patternPos >= patternEnd || !isRep((op = *patternPos)))
	{
		*flagp = flags;
		return;
	}

	if (!(flags & FLAG_NOT_EMPTY) && op != canonicalChar(TextType::CHAR_QUESTION_MARK))
		status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

	// If the last primary is a string, split the last character
	if (flags & FLAG_EXACTLY)
	{
		fb_assert(nodes.back().op == opExactly || nodes.back().op == opExactlyOne);

		if (nodes.back().op == opExactly && nodes.back().len > 1)
		{
			Node last = nodes.back();
			last.op = opExactlyOne;
			last.str += nodes.back().len - 1;
			last.len = 1;

			--nodes.back().len;
			atomPos = nodes.getCount();
			nodes.push(last);
		}
	}

	fb_assert(
		op == canonicalChar(TextType::CHAR_ASTERISK) ||
		op == canonicalChar(TextType::CHAR_PLUS) ||
		op == canonicalChar(TextType::CHAR_QUESTION_MARK) ||
		op == canonicalChar(TextType::CHAR_OPEN_BRACE));

	if (op == canonicalChar(TextType::CHAR_ASTERISK))
	{
		*flagp = 0;
		nodes.insert(atomPos, Node(opBranch, nodes.getCount() - atomPos + 2));
		nodes.push(Node(opRef, atomPos - nodes.getCount()));
		nodes.push(Node(opBranch));
	}
	else if (op == canonicalChar(TextType::CHAR_PLUS))
	{
		*flagp = FLAG_NOT_EMPTY;
		nodes.push(Node(opBranch, 2));
		nodes.push(Node(opRef, atomPos - nodes.getCount()));
		nodes.push(Node(opBranch));
	}
	else if (op == canonicalChar(TextType::CHAR_QUESTION_MARK))
	{
		*flagp = 0;
		nodes.insert(atomPos, Node(opBranch, nodes.getCount() - atomPos + 1));
		nodes.push(Node(opBranch));
	}
	else if (op == canonicalChar(TextType::CHAR_OPEN_BRACE))
	{
		++patternPos;

		UCharBuffer dummy;
		const UCHAR* p = originalPatternStr +
			charSet->substring(originalPatternLen, originalPatternStr,
							   originalPatternLen, dummy.getBuffer(originalPatternLen),
							   1, patternPos - patternStart);
		ULONG size = 0;
		bool comma = false;
		string s1, s2;
		bool ok;

		while ((ok = IntlUtil::readOneChar(charSet, &p, originalPatternStr + originalPatternLen, &size)))
		{
			if (*patternPos == canonicalChar(TextType::CHAR_CLOSE_BRACE))
			{
				if (s1.isEmpty())
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
				break;
			}
			else if (*patternPos == canonicalChar(TextType::CHAR_COMMA))
			{
				if (comma)
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
				comma = true;
			}
			else
			{
				ULONG ch = 0;
				charSet->getConvToUnicode().convert(size, p, sizeof(ch), reinterpret_cast<UCHAR*>(&ch));

				if (ch >= '0' && ch <= '9')
				{
					if (comma)
						s2 += (char) ch;
					else
						s1 += (char) ch;
				}
				else
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
			}

			++patternPos;
		}

		if (!ok || s1.length() > 9 || s2.length() > 9)
			status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

		const int n1 = atoi(s1.c_str());
		const int n2 = s2.isEmpty() ? (comma ? INT_MAX : n1) : atoi(s2.c_str());

		if (n2 < n1)
			status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

		*flagp = n1 == 0 ? 0 : FLAG_NOT_EMPTY;

		nodes.insert(atomPos, Node(opRepeat, n1, n2, nodes.getCount() - atomPos + 1));
		nodes.add(Node(opRepeatEnd, atomPos - nodes.getCount()));
	}

	++patternPos;

	if (patternPos < patternEnd && isRep(*patternPos))
		status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
}


template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::parsePrimary(int* flagp)
{
	*flagp = 0;

	const CharType op = *patternPos++;

	if (op == canonicalChar(TextType::CHAR_UNDERLINE))
	{
		nodes.push(Node(opAny));
		*flagp |= FLAG_NOT_EMPTY;
	}
	else if (op == canonicalChar(TextType::CHAR_PERCENT))
	{
		nodes.push(Node(opBranch, 3));
		nodes.push(Node(opAny));
		nodes.push(Node(opRef, -2));
		nodes.push(Node(opBranch));

		*flagp = 0;
		return;
	}
	else if (op == canonicalChar(TextType::CHAR_OPEN_BRACKET))
	{
		nodes.push(Node(opAnyOf));

		HalfStaticArray<CharType, BUFFER_SMALL> charsBuffer;
		HalfStaticArray<UCHAR, BUFFER_SMALL> rangeBuffer;

		Node& node = nodes.back();
		const CharType** nodeChars = &node.str;
		SLONG* nodeCharsLen = &node.len;
		const UCHAR** nodeRange = &node.str2;
		SLONG* nodeRangeLen = &node.len2;

		bool but = false;

		do
		{
			if (patternPos >= patternEnd)
				status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

			bool range = false;
			bool charClass = false;

			if (useEscape && *patternPos == escapeChar)
			{
				if (++patternPos >= patternEnd)
					status_exception::raise(Arg::Gds(isc_escape_invalid));

				if (*patternPos != escapeChar &&
					notInSet(patternPos, 1, metaCharacters, FB_NELEM(metaCharacters)) != 0)
				{
					status_exception::raise(Arg::Gds(isc_escape_invalid));
				}

				if (patternPos + 1 < patternEnd)
					range = (patternPos[1] == canonicalChar(TextType::CHAR_MINUS));
			}
			else
			{
				if (*patternPos == canonicalChar(TextType::CHAR_OPEN_BRACKET))
					charClass = true;
				else if (*patternPos == canonicalChar(TextType::CHAR_CIRCUMFLEX))
				{
					if (but)
						status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

					but = true;

					CharType* p = (CharType*) alloc(charsBuffer.getCount() * sizeof(CharType));
					memcpy(p, charsBuffer.begin(), charsBuffer.getCount() * sizeof(CharType));
					*nodeChars = p;

					*nodeCharsLen = charsBuffer.getCount();

					if (rangeBuffer.getCount() > 0)
					{
						UCHAR* p = (UCHAR*) alloc(rangeBuffer.getCount());
						memcpy(p, rangeBuffer.begin(), rangeBuffer.getCount());
						*nodeRange = p;
					}

					*nodeRangeLen = rangeBuffer.getCount();

					charsBuffer.clear();
					rangeBuffer.clear();

					nodeChars = &node.str3;
					nodeCharsLen = &node.len3;
					nodeRange = &node.str4;
					nodeRangeLen = &node.len4;

					++patternPos;
					continue;
				}
				else if (patternPos + 1 < patternEnd)
					range = (patternPos[1] == canonicalChar(TextType::CHAR_MINUS));
			}

			if (charClass)
			{
				if (++patternPos >= patternEnd || *patternPos != canonicalChar(TextType::CHAR_COLON))
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

				const CharType* start = ++patternPos;

				while (patternPos < patternEnd && *patternPos != canonicalChar(TextType::CHAR_COLON))
					++patternPos;

				const SLONG len = patternPos++ - start;

				if (patternPos >= patternEnd || *patternPos++ != canonicalChar(TextType::CHAR_CLOSE_BRACKET))
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

				typedef const UCHAR* (TextType::*GetCanonicalFunc)(int*) const;

				static const GetCanonicalFunc alNum[] = {&TextType::getCanonicalUpperLetters,
					&TextType::getCanonicalLowerLetters, &TextType::getCanonicalNumbers, NULL};
				static const GetCanonicalFunc alpha[] = {&TextType::getCanonicalUpperLetters,
					&TextType::getCanonicalLowerLetters, NULL};
				static const GetCanonicalFunc digit[] = {&TextType::getCanonicalNumbers, NULL};
				static const GetCanonicalFunc lower[] = {&TextType::getCanonicalLowerLetters, NULL};
				static const GetCanonicalFunc space[] = {&TextType::getCanonicalSpace, NULL};
				static const GetCanonicalFunc upper[] = {&TextType::getCanonicalUpperLetters, NULL};
				static const GetCanonicalFunc whitespace[] = {&TextType::getCanonicalWhiteSpaces, NULL};

				struct
				{
					const GetCanonicalFunc* funcs;
					const ULONG nameLen; // in bytes, not characters because all functions accept length in bytes
					const USHORT name[10];
				} static const classes[] =
					{	// Names are in utf16 in order not to convert them every time for comparison and thus save some CPU
						{alNum, 10, {'A','L','N','U','M'}},
						{alpha, 10, {'A','L','P','H','A'}},
						{digit, 10, {'D','I','G','I','T'}},
						{lower, 10, {'L','O','W','E','R'}},
						{space, 10, {'S','P','A','C','E'}},
						{upper, 10, {'U','P','P','E','R'}},
						{whitespace, 20, {'W','H','I','T','E','S','P','A','C','E'}}
					};

				// Get the exact original substring correspondent to the canonical bytes.
				HalfStaticArray<UCHAR, 10 * sizeof(ULONG)> classNameStr(
					len * charSet->maxBytesPerChar());
				ULONG classNameStrLen = charSet->substring(originalPatternLen, originalPatternStr,
					classNameStr.getCapacity(), classNameStr.begin(), start - patternStart, len);

				// And then convert it to UTF-16.
				HalfStaticArray<USHORT, 10 * sizeof(ULONG) / sizeof(USHORT)> classNameUtf16(
					len * sizeof(ULONG));
				ULONG classNameUtf16Len = charSet->getConvToUnicode().convert(
					classNameStrLen, classNameStr.begin(),
					classNameUtf16.getCapacity() * sizeof(USHORT), classNameUtf16.begin());

				// Bring class name to uppercase for case-insensitivity.
				// Do it in UTF-16 because original collation can have no uppercase conversion.
				classNameUtf16Len = Jrd::UnicodeUtil::utf16UpperCase(
					classNameUtf16Len, classNameUtf16.begin(),
					classNameUtf16.getCapacity() * sizeof(USHORT), classNameUtf16.begin(), NULL);
				int classN;

				for (classN = 0; classN < FB_NELEM(classes); ++classN)
				{
					INTL_BOOL errorFlag;

					if (Jrd::UnicodeUtil::utf16Compare(classNameUtf16Len, classNameUtf16.begin(),
							classes[classN].nameLen, classes[classN].name, &errorFlag) == 0)
					{
						for (const GetCanonicalFunc* func = classes[classN].funcs; *func; ++func)
						{
							int count;
							const CharType* canonic = (const CharType*) (textType->**func)(&count);
							charsBuffer.push(canonic, count);
						}

						break;
					}
				}

				if (classN >= FB_NELEM(classes))
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
			}
			else
			{
				charsBuffer.push(*patternPos++);

				if (range)
				{
					--patternPos;	// go back to first char

					UCHAR c[sizeof(ULONG)];
					ULONG len = charSet->substring(originalPatternLen, originalPatternStr,
												   sizeof(c), c, patternPos - patternStart, 1);

					rangeBuffer.push(len);
					size_t rangeCount = rangeBuffer.getCount();
					memcpy(rangeBuffer.getBuffer(rangeCount + len) + rangeCount, &c, len);

					++patternPos;	// character
					++patternPos;	// minus

					if (patternPos >= patternEnd)
						status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

					if (useEscape && *patternPos == escapeChar)
					{
						if (++patternPos >= patternEnd)
							status_exception::raise(Arg::Gds(isc_escape_invalid));

						if (*patternPos != escapeChar &&
							notInSet(patternPos, 1, metaCharacters, FB_NELEM(metaCharacters)) != 0)
						{
							status_exception::raise(Arg::Gds(isc_escape_invalid));
						}
					}

					len = charSet->substring(originalPatternLen, originalPatternStr,
											 sizeof(c), c, patternPos - patternStart, 1);

					rangeBuffer.push(len);
					rangeCount = rangeBuffer.getCount();
					memcpy(rangeBuffer.getBuffer(rangeCount + len) + rangeCount, &c, len);

					charsBuffer.push(*patternPos++);
				}
			}

			if (patternPos >= patternEnd)
				status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
		} while (*patternPos != canonicalChar(TextType::CHAR_CLOSE_BRACKET));

		CharType* p = (CharType*) alloc(charsBuffer.getCount() * sizeof(CharType));
		memcpy(p, charsBuffer.begin(), charsBuffer.getCount() * sizeof(CharType));
		*nodeChars = p;

		*nodeCharsLen = charsBuffer.getCount();

		if (rangeBuffer.getCount() > 0)
		{
			UCHAR* r = (UCHAR*) alloc(rangeBuffer.getCount());
			memcpy(r, rangeBuffer.begin(), rangeBuffer.getCount());
			*nodeRange = r;
		}

		*nodeRangeLen = rangeBuffer.getCount();

		++patternPos;
		*flagp |= FLAG_NOT_EMPTY;
	}
	else if (op == canonicalChar(TextType::CHAR_OPEN_PAREN))
	{
		int flags;
		parseExpr(&flags);

		++branchNum;	// This is used for the trace stuff.

		if (patternPos >= patternEnd || *patternPos++ != canonicalChar(TextType::CHAR_CLOSE_PAREN))
			status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

		*flagp |= flags & FLAG_NOT_EMPTY;
	}
	else if (useEscape && op == escapeChar)
	{
		if (patternPos >= patternEnd)
			status_exception::raise(Arg::Gds(isc_escape_invalid));

		if (*patternPos != escapeChar &&
			notInSet(patternPos, 1, metaCharacters, FB_NELEM(metaCharacters)) != 0)
		{
			status_exception::raise(Arg::Gds(isc_escape_invalid));
		}

		nodes.push(Node(opExactlyOne, patternPos++, 1));
		*flagp |= FLAG_NOT_EMPTY;
	}
	else
	{
		--patternPos;

		const SLONG len = notInSet(patternPos, patternEnd - patternPos,
			metaCharacters, FB_NELEM(metaCharacters));

		if (len == 0)
			status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

		*flagp |= FLAG_NOT_EMPTY | FLAG_EXACTLY;

		nodes.push(Node((len == 1 ? opExactlyOne : opExactly), patternPos, len));
		patternPos += len;
	}
}


template <typename CharType, typename StrConverter>
bool SimilarToMatcher<CharType, StrConverter>::Evaluator::isRep(CharType c) const
{
	return (c == canonicalChar(TextType::CHAR_ASTERISK) ||
			c == canonicalChar(TextType::CHAR_PLUS) ||
			c == canonicalChar(TextType::CHAR_QUESTION_MARK) ||
			c == canonicalChar(TextType::CHAR_OPEN_BRACE));
}


#ifdef DEBUG_SIMILAR
template <typename CharType, typename StrConverter>
void SimilarToMatcher<CharType, StrConverter>::Evaluator::dump() const
{
	string text;

	for (unsigned i = 0; i < nodes.getCount(); ++i)
	{
		string type;
		nodes[i].dump(type, i);

		string s;
		s.printf("%s%s", (i > 0 ? ", " : ""), type.c_str());

		text += s;
	}

	gds__log("%s", text.c_str());
}
#endif	// DEBUG_SIMILAR


template <typename CharType, typename StrConverter>
#ifdef RECURSIVE_SIMILAR
bool SimilarToMatcher<CharType, StrConverter>::Evaluator::match(int start)
{
#ifdef DEBUG_SIMILAR
	AutoSetRestore<int> autoDebugLevel(&debugLevel, debugLevel + 1);
#endif

	for (int i = start;; ++i)
	{
		const Node* node = &nodes[i];

#ifdef DEBUG_SIMILAR
		string s;
		node->dump(s, i);

		for (int debugLevelI = 0; debugLevelI < debugLevel; ++debugLevelI)
			s = "   " + s;

		s = "\n" + s;
		debugLog.add(s.c_str(), s.length());
#endif

		switch (node->op)
		{
			//// FIXME: Recursive opRepeat has problems!
			case opRepeat:
				for (int j = 0; j < node->len; ++j)
				{
					if (!match(i + 1))
						return false;
				}

				for (int j = node->len; j < node->len2; ++j)
				{
					const CharType* save = bufferPos;

					if (match(i + 1 + node->ref))
						return true;

					bufferPos = save;

					if (!match(i + 1))
						return false;
				}

				i += 1 + node->ref;
				break;

			case opRepeatEnd:
				return true;

			case opBranch:
			{
				const CharType* const save = bufferPos;

				while (true)
				{
					if (node->branchNum != -1)
						branches[node->branchNum].start = save - bufferStart;

					if (match(i + 1))
						return true;

					bufferPos = save;

					if (node->ref == 0)
						return false;

					i += node->ref;
					node = &nodes[i];

					if (node->ref == 0)
						break;

#ifdef DEBUG_SIMILAR
					node->dump(s, i);

					for (int debugLevelI = 0; debugLevelI < debugLevel; ++debugLevelI)
						s = "   " + s;

					s = "\n" + s;
					debugLog.add(s.c_str(), s.length());
#endif
				}

				break;
			}

			case opStart:
				if (bufferPos != bufferStart)
					return false;
				break;

			case opEnd:
				return (bufferPos == bufferEnd);

			case opRef:
				if (node->branchNum != -1)
				{
					fb_assert(unsigned(node->branchNum) <= branchNum);
					branches[node->branchNum].length =
						bufferPos - bufferStart - branches[node->branchNum].start;
				}

				if (node->ref == 1)	// avoid recursion
					break;
				return match(i + node->ref);

			case opNothing:
				break;

			case opAny:
#ifdef DEBUG_SIMILAR
				if (bufferPos >= bufferEnd)
					s = " -> <end>";
				else
					s.printf(" -> %d", *bufferPos);
				debugLog.add(s.c_str(), s.length());
#endif

				if (bufferPos >= bufferEnd)
					return false;
				++bufferPos;
				break;

			case opAnyOf:
#ifdef DEBUG_SIMILAR
				if (bufferPos >= bufferEnd)
					s = " -> <end>";
				else
					s.printf(" -> %d", *bufferPos);
				debugLog.add(s.c_str(), s.length());
#endif

				if (bufferPos >= bufferEnd)
					return false;

				if (notInSet(bufferPos, 1, node->str, node->len) != 0)
				{
					const UCHAR* const end = node->str2 + node->len2;
					const UCHAR* p = node->str2;

					while (p < end)
					{
						UCHAR c[sizeof(ULONG)];
						ULONG len = charSet->substring(buffer.getCount(), buffer.begin(),
													   sizeof(c), c, bufferPos - bufferStart, 1);

						if (textType->compare(len, c, p[0], p + 1) >= 0 &&
							textType->compare(len, c, p[1 + p[0]], p + 2 + p[0]) <= 0)
						{
							break;
						}

						p += 2 + p[0] + p[1 + p[0]];
					}

					if (node->len + node->len2 != 0 && p >= end)
						return false;
				}

				if (notInSet(bufferPos, 1, node->str3, node->len3) == 0)
					return false;
				else
				{
					const UCHAR* const end = node->str4 + node->len4;
					const UCHAR* p = node->str4;

					while (p < end)
					{
						UCHAR c[sizeof(ULONG)];
						const ULONG len = charSet->substring(buffer.getCount(), buffer.begin(),
														sizeof(c), c, bufferPos - bufferStart, 1);

						if (textType->compare(len, c, p[0], p + 1) >= 0 &&
							textType->compare(len, c, p[1 + p[0]], p + 2 + p[0]) <= 0)
						{
							break;
						}

						p += 2 + p[0] + p[1 + p[0]];
					}

					if (p < end)
						return false;
				}

				++bufferPos;
				break;

			case opExactly:
				if (bufferEnd - bufferPos >= node->len &&
					memcmp(node->str, bufferPos, node->len * sizeof(CharType)) == 0)
				{
					bufferPos += node->len;
					break;
				}
				else
					return false;

			case opExactlyOne:
				if (bufferEnd - bufferPos >= 1 && *node->str == *bufferPos)
				{
					bufferPos += node->len;
					break;
				}
				else
					return false;

			default:
				fb_assert(false);
				return false;
		}
	}

	return true;
}
#else
bool SimilarToMatcher<CharType, StrConverter>::Evaluator::match()
{
	// Left shift by 4 to OR MatchState's and Op's without additional runtime shifts.
	static const unsigned MATCH_STATE_SHIFT = 4;

	enum MatchState
	{
		msIterating			= 0x00 << MATCH_STATE_SHIFT,
		msReturningFalse	= 0x01 << MATCH_STATE_SHIFT,
		msReturningTrue		= 0x02 << MATCH_STATE_SHIFT,
		msReturningMask		= (msReturningFalse | msReturningTrue)
	};

	SimpleStack<Scope> scopeStack;

	// Add special node to return without needing additional comparison after popping
	// the stack on each return.
	Node nodeRet(opRet);
	scopeStack.push(&nodeRet);

	scopeStack.push(nodes.begin());

	MatchState state = msIterating;

	SimpleStack<SLONG> repeatStack;

	while (true)
	{
		Scope* const scope = scopeStack.back;
		const Node* const node = scope->i;

#define ENCODE_OP_STATE(op, state) ((op) | (state))

		// Go directly to op and state with a single switch.

		switch (ENCODE_OP_STATE(node->op, state))
		{
			case ENCODE_OP_STATE(opRepeat, msIterating):
				repeatStack.push(0);
				scopeStack.push(scope->i + node->ref);
				continue;

			case ENCODE_OP_STATE(opRepeat, msReturningFalse):
			case ENCODE_OP_STATE(opRepeat, msReturningTrue):
				repeatStack.pop();
				break;

			case ENCODE_OP_STATE(opRepeatEnd, msIterating):
			{
				const Node* repeatNode = scope->i + node->ref;
				SLONG* repeatCount = repeatStack.back;

				if (*repeatCount < repeatNode->len2)
				{
					++*repeatCount;
					scopeStack.push(repeatNode + 1);
					continue;
				}
				else
					break;
			}

			case ENCODE_OP_STATE(opRepeatEnd, msReturningFalse):
			{
				const Node* repeatNode = scope->i + node->ref;
				SLONG* repeatCount = repeatStack.back;

				if (--*repeatCount >= repeatNode->len)
					state = msIterating;

				break;
			}

			case ENCODE_OP_STATE(opRepeatEnd, msReturningTrue):
				break;

			case ENCODE_OP_STATE(opBranch, msIterating):
				if (node->branchNum != -1)
					branches[node->branchNum].start = bufferPos - bufferStart;

				scope->save = bufferPos;

				scopeStack.push(scope->i + 1);
				continue;

			case ENCODE_OP_STATE(opBranch, msReturningFalse):
				bufferPos = scope->save;

				if (node->ref != 0)
				{
					state = msIterating;

					scope->i += node->ref;

					if (scope->i->ref != 0)
					{
						scope->save = bufferPos;

						scopeStack.push(scope->i + 1);
						continue;
					}
				}

				break;

			case ENCODE_OP_STATE(opBranch, msReturningTrue):
				break;

			case ENCODE_OP_STATE(opStart, msIterating):
				if (bufferPos != bufferStart)
					state = msReturningFalse;
				break;

			case ENCODE_OP_STATE(opEnd, msIterating):
				state = (bufferPos == bufferEnd ? msReturningTrue : msReturningFalse);
				break;

			case ENCODE_OP_STATE(opRef, msIterating):
				if (node->branchNum != -1)
				{
					fb_assert(unsigned(node->branchNum) <= branchNum);
					branches[node->branchNum].length =
						bufferPos - bufferStart - branches[node->branchNum].start;
				}

				scope->i += node->ref;
				scope->save = NULL;
				continue;

			case ENCODE_OP_STATE(opRef, msReturningFalse):
			case ENCODE_OP_STATE(opRef, msReturningTrue):
				break;

			case ENCODE_OP_STATE(opNothing, msIterating):
			case ENCODE_OP_STATE(opNothing, msReturningFalse):
			case ENCODE_OP_STATE(opNothing, msReturningTrue):
				break;

			case ENCODE_OP_STATE(opAny, msIterating):
				if (bufferPos >= bufferEnd)
					state = msReturningFalse;
				else
					++bufferPos;
				break;

			case ENCODE_OP_STATE(opAnyOf, msIterating):
				if (bufferPos >= bufferEnd)
					state = msReturningFalse;
				else
				{
					if (notInSet(bufferPos, 1, node->str, node->len) != 0)
					{
						const UCHAR* const end = node->str2 + node->len2;
						const UCHAR* p = node->str2;

						while (p < end)
						{
							UCHAR c[sizeof(ULONG)];
							const ULONG len = charSet->substring(buffer.getCount(), buffer.begin(),
													sizeof(c), c, bufferPos - bufferStart, 1);

							if (textType->compare(len, c, p[0], p + 1) >= 0 &&
								textType->compare(len, c, p[1 + p[0]], p + 2 + p[0]) <= 0)
							{
								break;
							}

							p += 2 + p[0] + p[1 + p[0]];
						}

						if (node->len + node->len2 != 0 && p >= end)
						{
							state = msReturningFalse;
							break;
						}
					}

					if (notInSet(bufferPos, 1, node->str3, node->len3) == 0)
						state = msReturningFalse;
					else
					{
						const UCHAR* const end = node->str4 + node->len4;
						const UCHAR* p = node->str4;

						while (p < end)
						{
							UCHAR c[sizeof(ULONG)];
							const ULONG len = charSet->substring(
								buffer.getCount(), buffer.begin(),
								sizeof(c), c, bufferPos - bufferStart, 1);

							if (textType->compare(len, c, p[0], p + 1) >= 0 &&
								textType->compare(len, c, p[1 + p[0]], p + 2 + p[0]) <= 0)
							{
								break;
							}

							p += 2 + p[0] + p[1 + p[0]];
						}

						if (p < end)
							state = msReturningFalse;
					}
				}

				if (state == msIterating)
					++bufferPos;
				break;

			case ENCODE_OP_STATE(opExactly, msIterating):
				if (bufferEnd - bufferPos >= node->len &&
					memcmp(node->str, bufferPos, node->len * sizeof(CharType)) == 0)
				{
					bufferPos += node->len;
				}
				else
					state = msReturningFalse;
				break;

			case ENCODE_OP_STATE(opExactlyOne, msIterating):
				if (bufferEnd - bufferPos >= 1 && *node->str == *bufferPos)
					++bufferPos;
				else
					state = msReturningFalse;
				break;

			case ENCODE_OP_STATE(opRet, msReturningFalse):
			case ENCODE_OP_STATE(opRet, msReturningTrue):
				return state == msReturningTrue;

			default:
				fb_assert(false);
				return false;
		}

#undef ENCODE_OP_STATE

		switch (state)
		{
			case msIterating:
				++scope->i;
				break;

			case msReturningFalse:
			case msReturningTrue:
				scopeStack.pop();
				break;
		}
	}

	fb_assert(false);
	return false;
}
#endif


// Returns the number of characters up to first one present in set.
template <typename CharType, typename StrConverter>
SLONG SimilarToMatcher<CharType, StrConverter>::Evaluator::notInSet(
	const CharType* str, SLONG strLen, const CharType* set, SLONG setLen)
{
	for (const CharType* begin = str; str - begin < strLen; ++str)
	{
		for (const CharType* p = set; p - set < setLen; ++p)
		{
			if (*p == *str)
				return str - begin;
		}
	}

	return strLen;
}


// Given a regular expression R1<escape>#R2#<escape>R3 and the string S:
// - Find the shortest substring of S that matches R1 while the remainder (S23) matches R2R3;
// - Find the longest (S2) substring of S23 that matches R2 while the remainder matches R3;
// - Return S2.
template <typename CharType, typename StrConverter = Jrd::CanonicalConverter<> >
class SubstringSimilarMatcher : public Jrd::BaseSubstringSimilarMatcher
{
private:
	typedef Jrd::CharSet CharSet;
	typedef Jrd::TextType TextType;

public:
	SubstringSimilarMatcher(MemoryPool& pool, TextType* ttype,
				const UCHAR* patternStr, SLONG patternLen, CharType aEscapeChar)
		: BaseSubstringSimilarMatcher(pool, ttype),
		  escapeChar(aEscapeChar),
		  originalPatternStr(patternStr),
		  originalPatternLen(patternLen),
		  patternCvt(pool, textType, patternStr, patternLen),
		  buffer(pool)
	{
		CharSet* charSet = textType->getCharSet();

		// Make a new string without the <escape><quote>. While doing it, get the byte
		// length of each segment.

		UCharBuffer newExpr(originalPatternLen);
		UCHAR* newExprPos = newExpr.begin();

		const UCHAR* originalPatternEnd = originalPatternStr + originalPatternLen;
		const UCHAR* originalPatternPos = originalPatternStr;

		const CharType* lastStart = reinterpret_cast<const CharType*>(patternStr);
		const CharType* end = lastStart + patternLen;
		unsigned lengths[3];
		unsigned lengthsNum = 0;
		UCHAR dummy[sizeof(ULONG) * 2];

		for (const CharType* p = lastStart; p < end; ++p)
		{
			if (*p != escapeChar)
				continue;

			if (++p >= end)
				status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

			if (*p == canonicalChar(TextType::CHAR_DOUBLE_QUOTE))
			{
				if (lengthsNum >= 2)
					status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

				// Get the byte length since the last segment.
				ULONG len = charSet->substring(originalPatternEnd - originalPatternPos,
					originalPatternPos, newExpr.begin() + originalPatternLen - newExprPos,
					newExprPos, 0, p - lastStart - 1);

				lengths[lengthsNum++] = len;
				newExprPos += len;
				originalPatternPos += len;

				// Advance two (<escape><quote>) characters.
				originalPatternPos += charSet->substring(originalPatternEnd - originalPatternPos,
					originalPatternPos, sizeof(dummy), dummy, 0, 2);

				lastStart = p + 1;	// Register the start of the next segment.
			}
		}

		if (lengthsNum != 2)
			status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));

		// Get the byte length of the last segment.
		lengths[2] = charSet->substring(originalPatternEnd - originalPatternPos,
			originalPatternPos, newExpr.begin() + originalPatternLen - newExprPos,
			newExprPos, 0, end - lastStart);

		// Construct the needed regular expressions.

		r1 = FB_NEW(pool) SimilarToMatcher<CharType, StrConverter>(pool, ttype,
				newExpr.begin(), lengths[0], escapeChar, true);

		r2 = FB_NEW(pool) SimilarToMatcher<CharType, StrConverter>(pool, ttype,
				newExpr.begin() + lengths[0], lengths[1], escapeChar, true);

		r3 = FB_NEW(pool) SimilarToMatcher<CharType, StrConverter>(pool, ttype,
				newExpr.begin() + lengths[0] + lengths[1], lengths[2], escapeChar, true);

		r23 = FB_NEW(pool) SimilarToMatcher<CharType, StrConverter>(pool, ttype,
				newExpr.begin() + lengths[0], lengths[1] + lengths[2], escapeChar, true);
	}

	static SubstringSimilarMatcher* create(MemoryPool& pool, TextType* ttype,
		const UCHAR* str, SLONG length, const UCHAR* escape, SLONG escapeLen)
	{
		StrConverter cvt_escape(pool, ttype, escape, escapeLen);

		return FB_NEW(pool) SubstringSimilarMatcher(pool, ttype, str, length,
			*reinterpret_cast<const CharType*>(escape));
	}

	void reset()
	{
		buffer.shrink(0);

		r1->reset();
		r2->reset();
		r3->reset();
		r23->reset();
	}

	bool result()
	{
		CharSet* charSet = textType->getCharSet();
		const UCHAR* p = buffer.begin();
		UCharBuffer temp(buffer.getCount());
		UCHAR dummy[sizeof(ULONG)];

		// Find the shortest substring that matches R1 while the full expression matches R1R2R3.
		do
		{
			r1->reset();
			r1->process(buffer.begin(), p - buffer.begin());

			if (r1->result())
			{
				// We have a initial substring matching R1. Let's see if the remainder matches R2R3.

				r23->reset();
				r23->process(p, buffer.end() - p);

				if (r23->result())
				{
					// Now we start to find the longest substring that matches R2 while the
					// remainder matches R3. Once we found it, it's the result string.

					// We already know its start, based on the substring that matched R1.
					matchedStart = p - buffer.begin();

					const UCHAR* p3 = buffer.end();
					SLONG charLen23 = -1;
					memcpy(temp.begin(), p, p3 - p);

					while (true)
					{
						r2->reset();
						r2->process(temp.begin(), p3 - p);

						if (r2->result())
						{
							r3->reset();
							r3->process(p3, buffer.end() - p3);

							if (r3->result())
							{
								matchedLength = p3 - buffer.begin() - matchedStart;
								return true;
							}
						}

						if (charLen23 == -1)
							charLen23 = charSet->length(p3 - p, p, true);

						if (charLen23-- == 0)
							break;

						// Shrink in one character the string to match R2.
						// Move back one character to match R3.
						p3 = p + charSet->substring(buffer.end() - p, p, temp.getCapacity(),
							temp.begin(), 0, charLen23);
					}
				}
			}

			// Advance a character.
			p += charSet->substring(buffer.end() - p, p, sizeof(dummy), dummy, 0, 1);
		} while (p < buffer.end());

		return false;
	}

	bool process(const UCHAR* str, SLONG length)
	{
		const size_t pos = buffer.getCount();
		memcpy(buffer.getBuffer(pos + length) + pos, str, length);
		return true;
	}

	// We return byte-base start and length.
	void getResultInfo(unsigned* start, unsigned* length)
	{
		*start = matchedStart;
		*length = matchedLength;
	}

private:
	CharType canonicalChar(int ch) const
	{
		return *reinterpret_cast<const CharType*>(textType->getCanonicalChar(ch));
	}

private:
	CharType escapeChar;
	const UCHAR* originalPatternStr;
	SLONG originalPatternLen;
	StrConverter patternCvt;
	HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;
	AutoPtr<PatternMatcher> r1, r2, r3, r23;
	unsigned matchedStart;
	unsigned matchedLength;
};


} // namespace Firebird

#endif	// JRD_SIMILAR_TO_EVALUATOR_H
