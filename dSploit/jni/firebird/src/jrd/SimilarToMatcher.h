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

// #define DEBUG_SIMILAR

#ifdef DEBUG_SIMILAR
// #define RECURSIVE_SIMILAR	// useless in production due to stack overflow
#endif


namespace Firebird
{

template <typename StrConverter, typename CharType>
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
		Evaluator(MemoryPool& pool, TextType* textType,
			const UCHAR* patternStr, SLONG patternLen,
			CharType escapeChar, bool useEscape);

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
			opBranch,
			opStart,
			opEnd,
			opRef,
			opNothing,
			opAny,
			opAnyOf,
			opExactly
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

		// Struct used to evaluate expressions without recursion.
		// Represents local variables to implement a "virtual stack".
		struct Scope
		{
			Scope(int ai, int aLimit)
				: i(ai),
				  limit(aLimit),
				  save(NULL),
				  j(0),
				  flag(false)
			{
			}

			// variables used in the recursive commented out function
			int i;
			int limit;
			const CharType* save;
			int j;
			bool flag;	// aux. variable to make non-recursive logic
		};

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
		bool match(int limit, int start);
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

		TextType* textType;
		CharType escapeChar;
		bool useEscape;
		HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;
		const UCHAR* originalPatternStr;
		SLONG originalPatternLen;
		StrConverter patternCvt;
		CharSet* charSet;
		Array<Node> nodes;
		Array<Scope> scopes;
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
			SLONG str_len, CharType escape, bool use_escape)
		: PatternMatcher(pool, ttype),
		  evaluator(pool, ttype, str, str_len, escape, use_escape)
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
		const UCHAR* str, SLONG length, const UCHAR* escape, SLONG escape_length)
	{
		StrConverter cvt_escape(pool, ttype, escape, escape_length);

		return FB_NEW(pool) SimilarToMatcher(pool, ttype, str, length,
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escape_length != 0);
	}

	static bool evaluate(MemoryPool& pool, TextType* ttype, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length)
	{
		StrConverter cvt_escape(pool, ttype, escape, escape_length);

		Evaluator evaluator(pool, ttype, p, pl,
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escape_length != 0);
		evaluator.processNextChunk(s, sl);
		return evaluator.getResult();
	}

private:
	Evaluator evaluator;
};


template <typename StrConverter, typename CharType>
SimilarToMatcher<StrConverter, CharType>::Evaluator::Evaluator(
			MemoryPool& pool, TextType* textType,
			const UCHAR* patternStr, SLONG patternLen,
			CharType escapeChar, bool useEscape)
	: StaticAllocator(pool),
	  textType(textType),
	  escapeChar(escapeChar),
	  useEscape(useEscape),
	  buffer(pool),
	  originalPatternStr(patternStr),
	  originalPatternLen(patternLen),
	  patternCvt(pool, textType, patternStr, patternLen),
	  charSet(textType->getCharSet()),
	  nodes(pool),
	  scopes(pool),
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


template <typename StrConverter, typename CharType>
bool SimilarToMatcher<StrConverter, CharType>::Evaluator::getResult()
{
	const UCHAR* str = buffer.begin();
	SLONG len = buffer.getCount();

	// note that StrConverter changes str and len variables
	StrConverter cvt(pool, textType, str, len);
	fb_assert(len % sizeof(CharType) == 0);

	bufferStart = bufferPos = (const CharType*) str;
	bufferEnd = bufferStart + len / sizeof(CharType);

	const bool matched =
#ifdef RECURSIVE_SIMILAR
		match(nodes.getCount(), 0);
#else
		match();
#endif

#ifdef DEBUG_SIMILAR
	if (matched)
	{
		string s;
		for (unsigned i = 0; i <= branchNum; ++i)
		{
			string x;
			x.printf("%d: %d, %d\n\t", i, branches[i].start, branches[i].length);
			s += x;
		}

		gds__log("%s", s.c_str());
	}
#endif	// DEBUG_SIMILAR

	return matched;
}


template <typename StrConverter, typename CharType>
bool SimilarToMatcher<StrConverter, CharType>::Evaluator::processNextChunk(const UCHAR* data, SLONG dataLen)
{
	const size_t pos = buffer.getCount();
	memcpy(buffer.getBuffer(pos + dataLen) + pos, data, dataLen);
	return true;
}


template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::reset()
{
	buffer.shrink(0);
	scopes.shrink(0);

	memset(branches, 0, sizeof(Range) * (branchNum + 1));
}


template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::parseExpr(int* flagp)
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


template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::parseTerm(int* flagp)
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


template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::parseFactor(int* flagp)
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
		fb_assert(nodes.back().op == opExactly);

		if (nodes.back().len > 1)
		{
			Node last = nodes.back();
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

		nodes.insert(atomPos, Node(opRepeat, n1, n2, nodes.getCount() - atomPos));
	}

	++patternPos;

	if (patternPos < patternEnd && isRep(*patternPos))
		status_exception::raise(Arg::Gds(isc_invalid_similar_pattern));
}


template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::parsePrimary(int* flagp)
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
					const char* name;
					const GetCanonicalFunc* funcs;
				} static const classes[] =
					{
						{"ALNUM", alNum},
						{"ALPHA", alpha},
						{"DIGIT", digit},
						{"LOWER", lower},
						{"SPACE", space},
						{"UPPER", upper},
						{"WHITESPACE", whitespace}
					};

				UCharBuffer className;

				className.getBuffer(len);
				className.resize(charSet->substring(originalPatternLen, originalPatternStr,
													className.getCapacity(), className.begin(),
													start - patternStart, len));

				int classN;
				UCharBuffer buffer;

				for (classN = 0; classN < FB_NELEM(classes); ++classN)
				{
					const string s = IntlUtil::convertAsciiToUtf16(classes[classN].name);
					charSet->getConvFromUnicode().convert(s.length(), (const UCHAR*) s.c_str(), buffer);

					if (textType->compare(className.getCount(), className.begin(),
										  buffer.getCount(), buffer.begin()) == 0)
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

					const int previousRangeBufferCount = rangeBuffer.getCount();
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
			UCHAR* p = (UCHAR*) alloc(rangeBuffer.getCount());
			memcpy(p, rangeBuffer.begin(), rangeBuffer.getCount());
			*nodeRange = p;
		}

		*nodeRangeLen = rangeBuffer.getCount();

		++patternPos;
		*flagp |= FLAG_NOT_EMPTY;
	}
	else if (op == canonicalChar(TextType::CHAR_OPEN_PAREN))
	{
		++branchNum;

		int flags;
		parseExpr(&flags);

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

		nodes.push(Node(opExactly, patternPos++, 1));
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

		nodes.push(Node(opExactly, patternPos, len));
		patternPos += len;
	}
}


template <typename StrConverter, typename CharType>
bool SimilarToMatcher<StrConverter, CharType>::Evaluator::isRep(CharType c) const
{
	return (c == canonicalChar(TextType::CHAR_ASTERISK) ||
			c == canonicalChar(TextType::CHAR_PLUS) ||
			c == canonicalChar(TextType::CHAR_QUESTION_MARK) ||
			c == canonicalChar(TextType::CHAR_OPEN_BRACE));
}


#ifdef DEBUG_SIMILAR
template <typename StrConverter, typename CharType>
void SimilarToMatcher<StrConverter, CharType>::Evaluator::dump() const
{
	string text;

	for (unsigned i = 0; i < nodes.getCount(); ++i)
	{
		string type;

		switch (nodes[i].op)
		{
			case opRepeat:
				type.printf("opRepeat(%d, %d, %d)", nodes[i].len, nodes[i].len2, nodes[i].ref);
				break;

			case opBranch:
				if (nodes[i].branchNum == -1)
					type.printf("opBranch(%d)", i + nodes[i].ref);
				else
					type.printf("opBranch(%d, %d)", i + nodes[i].ref, nodes[i].branchNum);
				break;

			case opStart:
				type = "opStart";
				break;

			case opEnd:
				type = "opEnd";
				break;

			case opRef:
				if (nodes[i].branchNum == -1)
					type.printf("opRef(%d)", i + nodes[i].ref);
				else
					type.printf("opRef(%d, %d)", i + nodes[i].ref, nodes[i].branchNum);
				break;

			case opNothing:
				type = "opNothing";
				break;

			case opAny:
				type = "opAny";
				break;

			case opAnyOf:
				type.printf("opAnyOf(%.*s, %d, %.*s, %d, %.*s, %d, %.*s, %d)",
					nodes[i].len, nodes[i].str, nodes[i].len,
					nodes[i].len2, nodes[i].str2, nodes[i].len2,
					nodes[i].len3, nodes[i].str3, nodes[i].len3,
					nodes[i].len4, nodes[i].str4, nodes[i].len4);
				break;

			case opExactly:
				type.printf("opExactly(%.*s, %d)", nodes[i].len, nodes[i].str, nodes[i].len);
				break;

			default:
				type = "unknown";
				break;
		}

		string s;
		s.printf("%s%d:%s", (i > 0 ? ", " : ""), i, type.c_str());

		text += s;
	}

	gds__log("%s", text.c_str());
}
#endif	// DEBUG_SIMILAR


template <typename StrConverter, typename CharType>
#ifdef RECURSIVE_SIMILAR
bool SimilarToMatcher<StrConverter, CharType>::Evaluator::match(int limit, int start)
{
	for (int i = start; i < limit; ++i)
	{
		const Node* node = &nodes[i];

		switch (node->op)
		{
			case opRepeat:
				for (int j = 0; j < node->len; ++j)
				{
					if (!match(i + 1 + node->ref, i + 1))
						return false;
				}

				for (int j = node->len; j < node->len2; ++j)
				{
					const CharType* save = bufferPos;

					if (match(limit, i + 1 + node->ref))
						return true;

					bufferPos = save;

					if (!match(i + 1 + node->ref, i + 1))
						return false;
				}

				++i;
				break;

			case opBranch:
			{
				const CharType* const save = bufferPos;

				while (true)
				{
					if (node->branchNum != -1)
						branches[node->branchNum].start = save - bufferStart;

					if (match(limit, i + 1))
						return true;

					bufferPos = save;

					if (node->ref == 0)
						return false;

					i += node->ref;
					node = &nodes[i];

					if (node->ref == 0)
						break;
				}

				break;
			}

			case opStart:
				if (bufferPos != bufferStart)
					return false;
				break;

			case opEnd:
				if (bufferPos != bufferEnd)
					return false;
				break;

			case opRef:
				if (node->branchNum != -1)
				{
					fb_assert(unsigned(node->branchNum) <= branchNum);
					branches[node->branchNum].length =
						bufferPos - bufferStart - branches[node->branchNum].start;
				}

				if (node->ref == 1)	// avoid recursion
					break;
				return match(limit, i + node->ref);

			case opNothing:
				break;

			case opAny:
				if (bufferPos >= bufferEnd)
					return false;
				++bufferPos;
				break;

			case opAnyOf:
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
				if (node->len > bufferEnd - bufferPos ||
					memcmp(node->str, bufferPos, node->len * sizeof(CharType)) != 0)
				{
					return false;
				}
				bufferPos += node->len;
				break;

			default:
				fb_assert(false);
				return false;
		}
	}

	return true;
}
#else
bool SimilarToMatcher<StrConverter, CharType>::Evaluator::match()
{
	//
	// state  description
	// ----------------------
	//   0    recursing
	//   1    iteration (for)
	//   2    returning
	enum MatchState { msRecursing, msIterating, msReturning };

	int start = 0;
	MatchState state = msRecursing;
	int limit = nodes.getCount();
	bool ret = true;

	do
	{
		if (state == msRecursing)
		{
			if (start >= limit)
				state = msReturning;
			else
			{
				scopes.push(Scope(start, limit));
				state = msIterating;
			}
		}

		while (state != 0 && scopes.getCount() != 0)
		{
			Scope* scope = &scopes.back();
			if (scope->i >= scope->limit)
				break;

			const Node* node = &nodes[scope->i];

			switch (node->op)
			{
				case opRepeat:
					fb_assert(state == msIterating || state == msReturning);

					if (state == msIterating)
						scope->j = 0;
					else if (state == msReturning)
					{
						if (scope->j < node->len)
						{
							if (!ret)
								break;
						}
						else if (scope->j < node->len2)
						{
							if ((!scope->flag && ret) || (scope->flag && !ret))
								break;

							if (!scope->flag)
							{
								bufferPos = scope->save;

								scope->flag = true;
								start = scope->i + 1;
								limit = scope->i + 1 + node->ref;
								state = msRecursing;

								break;
							}
						}
						++scope->j;
					}

					if (scope->j < node->len)
					{
						start = scope->i + 1;
						limit = scope->i + 1 + node->ref;
						state = msRecursing;
					}
					else if (scope->j < node->len2)
					{
						scope->save = bufferPos;
						scope->flag = false;
						start = scope->i + 1 + node->ref;
						limit = scope->limit;
						state = msRecursing;
					}
					else
					{
						scope->i += node->ref;
						state = msIterating;
					}

					break;

				case opBranch:
					if (state == msIterating)
					{
						if (node->branchNum != -1)
							branches[node->branchNum].start = bufferPos - bufferStart;

						scope->save = bufferPos;
						start = scope->i + 1;
						limit = scope->limit;
						state = msRecursing;
					}
					else
					{
						fb_assert(state == msReturning);

						if (!ret)
						{
							bufferPos = scope->save;

							if (node->ref == 0)
								ret = false;
							else
							{
								scope->i += node->ref;
								node = &nodes[scope->i];

								if (node->ref == 0)
									state = msIterating;
								else
								{
									scope->save = bufferPos;
									start = scope->i + 1;
									limit = scope->limit;
									state = msRecursing;
								}
							}
						}
					}
					break;

				case opStart:
					fb_assert(state == msIterating);
					if (bufferPos != bufferStart)
					{
						ret = false;
						state = msReturning;
					}
					break;

				case opEnd:
					fb_assert(state == msIterating);
					if (bufferPos != bufferEnd)
					{
						ret = false;
						state = msReturning;
					}
					break;

				case opRef:
					fb_assert(state == msIterating || state == msReturning);
					if (state == msIterating)
					{
						if (node->branchNum != -1)
						{
							fb_assert(unsigned(node->branchNum) <= branchNum);
							branches[node->branchNum].length =
								bufferPos - bufferStart - branches[node->branchNum].start;
						}

						if (node->ref != 1)
						{
							state = msRecursing;
							start = scope->i + node->ref;
							limit = scope->limit;
						}
					}
					break;

				case opNothing:
					break;

				case opAny:
					fb_assert(state == msIterating);
					if (bufferPos >= bufferEnd)
					{
						ret = false;
						state = msReturning;
					}
					else
						++bufferPos;
					break;

				case opAnyOf:
					fb_assert(state == msIterating);
					if (bufferPos >= bufferEnd)
					{
						ret = false;
						state = msReturning;
					}
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
								ret = false;
								state = msReturning;
								break;
							}
						}

						if (notInSet(bufferPos, 1, node->str3, node->len3) == 0)
						{
							ret = false;
							state = msReturning;
						}
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
							{
								ret = false;
								state = msReturning;
							}
						}
					}

					if (state == msIterating)
						++bufferPos;
					break;

				case opExactly:
					fb_assert(state == msIterating);
					if (node->len > bufferEnd - bufferPos ||
						memcmp(node->str, bufferPos, node->len * sizeof(CharType)) != 0)
					{
						ret = false;
						state = msReturning;
					}
					else
						bufferPos += node->len;
					break;

				default:
					fb_assert(false);
					return false;
			}

			if (state == msIterating)
			{
				++scope->i;
				if (scope->i >= scope->limit)
				{
					ret = true;
					state = msReturning;
				}
			}

			if (state == msReturning)
				scopes.pop();
		}
	} while (scopes.getCount() != 0);

	return ret;
}
#endif


// Returns the number of characters up to first one present in set.
template <typename StrConverter, typename CharType>
SLONG SimilarToMatcher<StrConverter, CharType>::Evaluator::notInSet(
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

} // namespace Firebird


#endif	// JRD_SIMILAR_TO_EVALUATOR_H
