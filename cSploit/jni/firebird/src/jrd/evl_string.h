/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			evl_string.h
 *	DESCRIPTION:	Algorithms for streamed string functions
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_EVL_STRING_H
#define JRD_EVL_STRING_H

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"

// Number of pattern items statically allocated
const int STATIC_PATTERN_ITEMS	= 16;

// Number of pattern items that are matched in parallel statically allocated
const int STATIC_STATUS_ITEMS	= 16;

// Size of internal static buffer used for allocation
// This buffer is used for KMP masks and string unescaping
#ifdef TESTING_ONLY
const int STATIC_PATTERN_BUFFER		= 16;
#else
const int STATIC_PATTERN_BUFFER		= 256;
#endif

namespace Firebird {

template <typename CharType>
static void preKmp(const CharType *x, int m, SLONG kmpNext[])
{
	SLONG i = 0;
	SLONG j = kmpNext[0] = -1;

	while (i < m - 1)
	{
		while (j > -1 && x[i] != x[j])
			j = kmpNext[j];
		i++;
		j++;
		if (x[i] == x[j])
			kmpNext[i] = kmpNext[j];
		else
			kmpNext[i] = j;
	}

	while (j > -1 && x[i] != x[j])
		j = kmpNext[j];

	kmpNext[++i] = ++j;
}

class StaticAllocator
{
public:
	explicit StaticAllocator(MemoryPool& _pool)
		: pool(_pool), chunksToFree(_pool), allocated(0)
	{
	}

	~StaticAllocator()
	{
		for (size_t i = 0; i < chunksToFree.getCount(); i++)
			pool.deallocate(chunksToFree[i]);
	}

	void* alloc(SLONG count)
	{
		void* result;
		const SLONG localCount = ROUNDUP(count, FB_ALIGNMENT);
		if (allocated + localCount <= STATIC_PATTERN_BUFFER)
		{
			result = allocBuffer + allocated;
			allocated += localCount;
		}
		else
		{
			result = pool.allocate(count);
			chunksToFree.add(result);
		}
		return result;
	}

protected:
	MemoryPool& pool;

private:
	Array<void*> chunksToFree;
	char allocBuffer[STATIC_PATTERN_BUFFER];
	int allocated;
};

template <typename CharType>
class StartsEvaluator : private StaticAllocator
{
public:
	StartsEvaluator(MemoryPool& _pool, const CharType* _pattern_str, SLONG _pattern_len)
		: StaticAllocator(_pool), pattern_len(_pattern_len)
	{
		CharType* temp = static_cast<CharType*>(alloc(_pattern_len * sizeof(CharType)));
		memcpy(temp, _pattern_str, _pattern_len * sizeof(CharType));
		pattern_str = temp;

		reset();
	}

	void reset()
	{
		result = true;
		offset = 0;
	}

	bool getResult() const
	{
		return offset >= pattern_len && result;
	}

	bool processNextChunk(const CharType* data, SLONG data_len)
	{
		// Should work fine when called with data_len equal to zero
		if (!result || offset >= pattern_len)
			return false;

		const SLONG comp_length = data_len < pattern_len - offset ? data_len : pattern_len - offset;
		if (memcmp(data, pattern_str + offset, sizeof(CharType) * comp_length) != 0)
		{
			result = false;
			return false;
		}
		offset += comp_length;
		return offset < pattern_len;
	}

private:
	SLONG offset;
	const CharType* pattern_str;
	SLONG pattern_len;
	bool result;
};

template <typename CharType>
class ContainsEvaluator : private StaticAllocator
{
public:
	ContainsEvaluator(MemoryPool& _pool, const CharType* _pattern_str, SLONG _pattern_len)
		: StaticAllocator(_pool), pattern_len(_pattern_len)
	{
		CharType* temp = static_cast<CharType*>(alloc(_pattern_len * sizeof(CharType)));
		memcpy(temp, _pattern_str, _pattern_len * sizeof(CharType));
		pattern_str = temp;
		kmpNext = static_cast<SLONG*>(alloc((_pattern_len + 1) * sizeof(SLONG)));
		preKmp<CharType>(_pattern_str, _pattern_len, kmpNext);
		reset();
	}

	void reset()
	{
		offset = 0;
		result = (pattern_len == 0);
	}

	bool getResult() const
	{
		return result;
	}

	bool processNextChunk(const CharType* data, SLONG data_len)
	{
		// Should work fine when called with data_len equal to zero
		if (result)
			return false;

		SLONG data_pos = 0;
		while (data_pos < data_len)
		{
			while (offset > -1 && pattern_str[offset] != data[data_pos])
				offset = kmpNext[offset];
			offset++;
			data_pos++;
			if (offset >= pattern_len) {
				result = true;
				return false;
			}
		}
		return true;
	}

private:
	const CharType* pattern_str;
	SLONG pattern_len;
	SLONG offset;
	bool result;
	SLONG *kmpNext;
};

enum PatternItemType
{
	piNone = 0,
	piSearch,
	piSkipFixed,
	piDirectMatch,
	// Used only during compilation phase to indicate that string is pending cleanup
	piEscapedString,
	// Used to optimize subpatterns like "%_%____%_", not used during matching phase
	piSkipMore
};

enum MatchType
{
	MATCH_NONE = 0,
	MATCH_FIXED,
	MATCH_ANY
};

template <typename CharType>
class LikeEvaluator : private StaticAllocator
{
public:
	LikeEvaluator(MemoryPool& _pool, const CharType* _pattern_str,
		SLONG pattern_len, CharType escape_char, bool use_escape, CharType sql_match_any,
		CharType sql_match_one);

	void reset()
	{
		fb_assert(patternItems.getCount());
		branches.shrink(0);
		if (patternItems[0].type == piNone) {
			match_type = (patternItems[0].match_any ? MATCH_ANY : MATCH_FIXED);
		}
		else {
			BranchItem temp = {&patternItems[0], 0};
			branches.add(temp);
			match_type = MATCH_NONE;
		}
	}

	bool getResult() const
	{
		return match_type != MATCH_NONE;
	}

	// Returns true if more data can change the result of evaluation
	bool processNextChunk(const CharType* data, SLONG data_len);

private:
	struct PatternItem
	{
		PatternItemType type;
		struct str_struct
		{
			SLONG length;
			CharType* data;
			SLONG* kmpNext; // Jump table for Knuth-Morris-Pratt algorithm
		};
		union // anonymous union
		{
			str_struct str;
			SLONG skipCount;
		};
		bool match_any;
	};

	struct BranchItem
	{
		PatternItem* pattern;
		SLONG offset; // Match offset inside this pattern
	};

	HalfStaticArray<PatternItem, STATIC_PATTERN_ITEMS> patternItems;
	HalfStaticArray<BranchItem, STATIC_STATUS_ITEMS> branches;

	MatchType match_type;
};

template <typename CharType>
LikeEvaluator<CharType>::LikeEvaluator(
	MemoryPool& _pool, const CharType* _pattern_str, SLONG pattern_len,
	CharType escape_char, bool use_escape, CharType sql_match_any, CharType sql_match_one)
: StaticAllocator(_pool), patternItems(_pool), branches(_pool), match_type(MATCH_NONE)
{
	// Create local copy of the string.
	CharType* pattern_str = static_cast<CharType*>(alloc(pattern_len*sizeof(CharType)));
	memcpy(pattern_str, _pattern_str, pattern_len * sizeof(CharType));

	patternItems.grow(1);
	// PASS1. Parse pattern.
	SLONG pattern_pos = 0;
	PatternItem *item = patternItems.begin();
	while (pattern_pos < pattern_len)
	{
		CharType c = pattern_str[pattern_pos++];
		// Escaped symbol
		if (use_escape && c == escape_char)
		{
			if (pattern_pos < pattern_len)
			{
				c = pattern_str[pattern_pos++];
				/* Note: SQL II says <escape_char><escape_char> is error condition */
				if (c == escape_char ||
					(sql_match_any && c == sql_match_any) ||
					(sql_match_one && c == sql_match_one))
				{
					switch (item->type)
					{
					case piSkipFixed:
					case piSkipMore:
						patternItems.grow(patternItems.getCount() + 1);
						item = patternItems.end() - 1;
						// Note: fall into
					case piNone:
						item->type = piEscapedString;
						item->str.data = const_cast<CharType*>(pattern_str + pattern_pos - 2);
						item->str.length = 1;
						break;
					case piSearch:
						item->type = piEscapedString;
						// Note: fall into
					case piEscapedString:
						item->str.length++;
						break;
					}
					continue;
				}
			}
			Firebird::Arg::Gds(isc_escape_invalid).raise();
		}
		// percent sign
		if (sql_match_any && c == sql_match_any)
		{
			switch (item->type)
			{
			case piSearch:
			case piEscapedString:
				patternItems.grow(patternItems.getCount() + 1);
				item = patternItems.end() - 1;
				// Note: fall into
			case piSkipFixed:
			case piNone:
				item->type = piSkipMore;
				break;
			}
			continue;
		}
		// underscore
		if (sql_match_one && c == sql_match_one)
		{
			switch (item->type)
			{
			case piSearch:
			case piEscapedString:
				patternItems.grow(patternItems.getCount() + 1);
				item = patternItems.end() - 1;
				// Note: fall into
			case piNone:
				item->type = piSkipFixed;
				item->skipCount = 1;
				break;
			case piSkipFixed:
			case piSkipMore:
				item->skipCount++;
				break;
			}
			continue;
		}
		// anything else
		switch (item->type)
		{
		case piSkipFixed:
		case piSkipMore:
			patternItems.grow(patternItems.getCount() + 1);
			item = patternItems.end() - 1;
			// Note: fall into
		case piNone:
			item->type = piSearch;
			item->str.data = const_cast<CharType*>(pattern_str + pattern_pos - 1);
			item->str.length = 1;
			break;
		case piSearch:
		case piEscapedString:
			item->str.length++;
			break;
		}
	}

	// PASS2. Compilation/Optimization
	// Unescape strings, mark direct match items, pre-compile KMP tables and
	// optimize out piSkipMore nodes
	bool directMatch = true;
	for (size_t i = 0; i < patternItems.getCount();)
	{
		PatternItem *itemL = &patternItems[i];
		switch (itemL->type)
		{
		case piEscapedString:
			{
				const CharType *curPos = itemL->str.data;
				itemL->str.data = static_cast<CharType*>(alloc(itemL->str.length * sizeof(CharType)));
				for (SLONG j = 0; j < itemL->str.length; j++) {
					if (use_escape && *curPos == escape_char)
						curPos++;
					itemL->str.data[j] = *curPos++;
				}
				itemL->type = piSearch;
				// Note: fall into
			}
		case piSearch:
			if (directMatch)
				itemL->type = piDirectMatch;
			else {
				itemL->str.kmpNext =
					static_cast<SLONG*>(alloc((itemL->str.length + 1) * sizeof(SLONG)));
				preKmp<CharType>(itemL->str.data, itemL->str.length, itemL->str.kmpNext);
				directMatch = true;
			}
			break;
		case piSkipMore:
			// Optimize out piSkipMore
			directMatch = false;
			if (itemL->skipCount != 0)
			{
				// Convert this node to SkipFixed if possible
				itemL->type = piSkipFixed;
				itemL->match_any = true;
			}
			else
			{
				if (i > 0) {
					// Mark previous node if it exists
					patternItems[i - 1].match_any = true;
					patternItems.remove(i);
					continue;
				}
				// Remove node if we have other nodes
				if (patternItems.getCount() != 1) {
					patternItems.remove(i);
					continue;
				}
				// Our pattern is single %
				itemL->type = piNone;
				itemL->match_any = true;
			}
			break;
		}
		i++;
	}

	// Get ready for parsing
	reset();
}

template <typename CharType>
bool LikeEvaluator<CharType>::processNextChunk(const CharType* data, SLONG data_len)
{
	fb_assert(patternItems.getCount());

	// If called with empty buffer just return if more data can change the result of evaluation
	if (!data_len) {
		return branches.getCount() != 0 || match_type == MATCH_FIXED;
	}

	if (match_type == MATCH_FIXED)
		match_type = MATCH_NONE;

	if (branches.getCount() == 0)
		return false;

	SLONG data_pos = 0;
	SLONG finishCandidate = -1;
	while (data_pos < data_len)
	{

		size_t branch_number = 0;
		while (branch_number < branches.getCount())
		{
			BranchItem *current_branch = &branches[branch_number];
			PatternItem *current_pattern = current_branch->pattern;
			switch (current_pattern->type)
			{
			case piDirectMatch:
				if (data[data_pos] != current_pattern->str.data[current_branch->offset])
				{
					// Terminate matching branch
					branches.remove(branch_number);
					if (branches.getCount() == 0)
						return false;
					continue;
				}
				// Note: fall into
			case piSkipFixed:
				current_branch->offset++;
				if (current_branch->offset >= current_pattern->str.length)
				{
					// Switch to next subpattern or finish matching
					if (current_pattern->match_any)
					{
						current_pattern++;
						if (current_pattern >= patternItems.end()) {
							branches.shrink(0);
							match_type = MATCH_ANY;
							return false;
						}
						branches.shrink(1);
						branches[0].pattern = current_pattern;
						branches[0].offset = 0;
						branch_number = 0;
						break;
					}
					current_pattern++;
					if (current_pattern >= patternItems.end())
					{
						finishCandidate = data_pos;
						branches.remove(branch_number);
						if (branches.getCount() == 0)
						{
							if (data_pos == data_len - 1) {
								match_type = MATCH_FIXED;
								return true;
							}
							return false;
						}
						continue;
					}
					current_branch->pattern = current_pattern;
					current_branch->offset = 0;
				}
				break;
			case piSearch:
				// Knuth-Morris-Pratt search algorithm
				while (current_branch->offset >= 0 &&
					   current_pattern->str.data[current_branch->offset] != data[data_pos])
				{
			    	current_branch->offset = current_pattern->str.kmpNext[current_branch->offset];
				}
				current_branch->offset++;
				if (current_branch->offset >= current_pattern->str.length)
				{
					PatternItem *next_pattern = current_pattern + 1;
					if (next_pattern >= patternItems.end())
					{
						if (current_pattern->match_any) {
							branches.shrink(0);
							match_type = MATCH_ANY;
							return false;
						}
						// We are looking for the pattern at the end of string
						current_branch->offset = current_pattern->str.kmpNext[current_branch->offset];
						finishCandidate = data_pos;
					}
					else
					{
						if (next_pattern->type == piSearch) {
							// Search for the next pattern
							current_branch->pattern = next_pattern;
							current_branch->offset = 0;
						}
						else
						{
							// Try to apply further non-search patterns and continue searching
							current_branch->offset = current_pattern->str.kmpNext[current_branch->offset];
							BranchItem temp = {next_pattern, 0};
							branches.insert(branch_number + 1, temp); // +1 is to reduce movement effort :)
							branch_number++; // Skip newly inserted branch in this cycle
						}
					}
				}
				break;
			}
			branch_number++;
		}
		data_pos++;
	}
	if (finishCandidate == data_len - 1)
		match_type = MATCH_FIXED;
	return true;
}

} // namespace Firebird

#endif // JRD_EVL_STRING_H
